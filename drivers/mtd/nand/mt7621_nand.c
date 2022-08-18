// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2018 MediaTek Incorporation. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <common.h>
#include <nand.h>
#include <malloc.h>

#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/sizes.h>

#include <mach/mt7621_regs.h>

#include "mt7621_nand.h"


static const mt7621_nfc_timing_t mt7621_nfc_def_timings[] = {
	{{0xec, 0xd3, 0x51, 0x95, 0x58}, 5, 0x44333 }, /* K9K8G8000 */
};

static mt7621_nfc_t nfc_dev;

static int nfc_write_page_raw(struct mtd_info *mtd, struct nand_chip *chip,
	const u8 *buf, int oob_on, int page);

static int nfc_ecc_wait_idle(mt7621_nfc_t *nfc, u32 reg);


static u16 __maybe_unused nfi_read16(mt7621_nfc_t *nfc, u32 addr)
{
	return readw(nfc->nfi_base + addr);
}

static void __maybe_unused nfi_write16(mt7621_nfc_t *nfc, u32 addr, u16 val)
{
	return writew(val, nfc->nfi_base + addr);
}

static void __maybe_unused nfi_setbits16(mt7621_nfc_t *nfc, u32 addr, u16 bits)
{
	u16 val = nfi_read16(nfc, addr);
	return writew(val | bits, nfc->nfi_base + addr);
}

static void __maybe_unused nfi_clrbits16(mt7621_nfc_t *nfc, u32 addr, u16 bits)
{
	u16 val = nfi_read16(nfc, addr);
	return writew(val & ~bits, nfc->nfi_base + addr);
}

static void __maybe_unused nfi_clrsetbits16(mt7621_nfc_t *nfc, u32 addr, u16 clr, u16 set)
{
	u16 val = nfi_read16(nfc, addr);
	return writew((val & ~clr) | set, nfc->nfi_base + addr);
}

static u32 __maybe_unused nfi_read32(mt7621_nfc_t *nfc, u32 addr)
{
	return readl(nfc->nfi_base + addr);
}

static void __maybe_unused nfi_write32(mt7621_nfc_t *nfc, u32 addr, u32 val)
{
	return writel(val, nfc->nfi_base + addr);
}

static void ecc_write16(mt7621_nfc_t *nfc, u32 addr, u16 val)
{
	return writew(val, nfc->ecc_base + addr);
}

static u32 ecc_read32(mt7621_nfc_t *nfc, u32 addr)
{
	return readl(nfc->ecc_base + addr);
}

static void ecc_write32(mt7621_nfc_t *nfc, u32 addr, u32 val)
{
	return writel(val, nfc->ecc_base + addr);
}

static inline mt7621_nfc_sel_t *nand_to_mt7621_chip(struct nand_chip *nand)
{
	return container_of(nand, mt7621_nfc_sel_t, nand);
}

static void nfc_ecc_init(mt7621_nfc_t *nfc, struct nand_ecc_ctrl *ecc)
{
	u32 ecc_cap_bit_cfg, encode_block_size, decode_block_size;

	switch (ecc->strength) {
	case 4:
		ecc_cap_bit_cfg = ECC_CAP_4B;
		break;
	case 6:
		ecc_cap_bit_cfg = ECC_CAP_6B;
		break;
	case 8:
		ecc_cap_bit_cfg = ECC_CAP_8B;
		break;
	case 10:
		ecc_cap_bit_cfg = ECC_CAP_10B;
		break;
	case 12:
		ecc_cap_bit_cfg = ECC_CAP_12B;
		break;
	default:
		ecc_cap_bit_cfg = ECC_CAP_4B;
		break;
	}

	/* Sector + FDM */
	encode_block_size = (ecc->size + NFI_ECC_FDM_SIZE) << 3;
	nfc_ecc_wait_idle(nfc, ECC_ENCIDLE_REG16);
	ecc_write16(nfc, ECC_ENCCON_REG16, 0);
	ecc_write32(nfc, ECC_ENCCNFG_REG32,
			 REG_SET_VAL(ENC_TNUM, ecc_cap_bit_cfg) |
			 REG_SET_VAL(ENC_MODE, ENC_MODE_NFI) |
			 REG_SET_VAL(ENC_CNFG_MSG, encode_block_size));

	/* Sector + FDM + ECC parity bits */
	decode_block_size = ((ecc->size + NFI_ECC_FDM_SIZE) << 3) +
			    ecc->strength * NFI_ECC_PARITY_BITS;
	nfc_ecc_wait_idle(nfc, ECC_DECIDLE_REG16);
	ecc_write16(nfc, ECC_DECCON_REG16, 0);
	ecc_write32(nfc, ECC_DECCNFG_REG32,
			 REG_SET_VAL(DEC_TNUM, ecc_cap_bit_cfg) |
			 REG_SET_VAL(DEC_MODE, DEC_MODE_NFI) |
			 REG_SET_VAL(DEC_CS, decode_block_size) |
			 DEC_EMPTY_EN |
			 REG_SET_VAL(DEC_CON, DEC_CON_EL));

	/* setup FDM register base */
	ecc_write32(nfc, ECC_FDMADDR_REG32,
			 CPHYSADDR((u32) nfc->nfi_base + NFI_FDM0L_REG32));
}

static int nfc_ecc_wait_idle(mt7621_nfc_t *nfc, u32 reg)
{
	u32 val;
	int ret;

	ret = readw_poll_timeout(nfc->ecc_base + reg, val,
		(val & ECC_IDLE), NFI_STATUS_WAIT_TIMEOUT_US);

	if (ret) {
		printf("Warning: ECC engine timed out entering idle mode\n");
		return -EIO;
	}

	return 0;
}

static int nfc_ecc_decoder_wait_done(mt7621_nfc_t *nfc, u32 sector_index)
{
	u32 val;
	int ret;

	ret = readw_poll_timeout(nfc->ecc_base + ECC_DECDONE_REG16, val,
		(val & (1 << sector_index)), NFI_STATUS_WAIT_TIMEOUT_US);

	if (ret) {
		printf("Warning: ECC decoder for sector %d timed out\n",
		       sector_index);
		return -ETIMEDOUT;
	}

	return 0;
}

static void nfc_ecc_encoder_start(mt7621_nfc_t *nfc)
{
	nfc_ecc_wait_idle(nfc, ECC_ENCIDLE_REG16);
	ecc_write16(nfc, ECC_ENCCON_REG16, ENC_EN);
}

static void nfc_ecc_encoder_stop(mt7621_nfc_t *nfc)
{
	nfc_ecc_wait_idle(nfc, ECC_ENCIDLE_REG16);
	ecc_write16(nfc, ECC_ENCCON_REG16, 0);
}

static void nfc_ecc_decoder_start(mt7621_nfc_t *nfc)
{
	nfc_ecc_wait_idle(nfc, ECC_DECIDLE_REG16);
	ecc_write16(nfc, ECC_DECCON_REG16, DEC_EN);
}

static void nfc_ecc_decoder_stop(mt7621_nfc_t *nfc)
{
	nfc_ecc_wait_idle(nfc, ECC_DECIDLE_REG16);
	ecc_write16(nfc, ECC_DECCON_REG16, 0);
}

static int nfc_ecc_correct_check(mt7621_nfc_t *nfc, mt7621_nfc_sel_t *nfc_chip,
			  u8 *sector_buf, u8 *fdm_buf, u32 sector_index)
{
	struct nand_chip *nand = &nfc_chip->nand;
	struct mtd_info *mtd = nand_to_mtd(nand);
	int bitflips = 0;
	u32 i;
	u32 decnum, num_error_bits, fdm_start_bits;
	u32 error_locations, error_bit_loc;
	u32 error_byte_pos, error_bit_pos_in_byte;

	decnum = ecc_read32(nfc, ECC_DECENUM_REG32);
	num_error_bits = (decnum >> (sector_index << ERRNUM_S)) & ERRNUM_M;
	fdm_start_bits = (nand->ecc.size + NFI_ECC_FDM_SIZE) << 3;

	if (!num_error_bits)
		return 0;

	if (num_error_bits == ERRNUM_M) {
		mtd->ecc_stats.failed++;
		return -1;
	}

	for (i = 0; i < num_error_bits; i++) {
		error_locations = ecc_read32(nfc, ECC_DECEL_REG32(i / 2));
		error_bit_loc = (error_locations >> ((i % 2) * DEC_EL_ODD_S)) &
				DEC_EL_M;
		error_byte_pos = error_bit_loc >> DEC_EL_BYTE_POS_S;
		error_bit_pos_in_byte = error_bit_loc & DEC_EL_BIT_POS_M;

		if (error_bit_loc < (nand->ecc.size << 3)) {
			sector_buf[error_byte_pos] ^=
				(1 << error_bit_pos_in_byte);
		} else if (error_bit_loc < fdm_start_bits) {
			if (fdm_buf) {
				fdm_buf[error_byte_pos - nand->ecc.size] ^=
					(1 << error_bit_pos_in_byte);
			}
		}

		bitflips++;
	}

	mtd->ecc_stats.corrected += bitflips;

	return bitflips;
}

/*
 * Raw page layout:
 * ---------------------------------------------------------------------------------
 * DAT0 | FDM0 | ECC0 | DAT1 | FDM1 | ECC1 | DAT2 | FDM2 | ECC2 | DAT3 | FDM3 | ECC3
 * 512B |  8B  |  8+B | 512B |  8B  |  8+B | 512B |  8B  |  8+B | 512B |  8B  |  8+B
 * ---------------------------------------------------------------------------------
 *
 * Formatted page layout:
 * ---------------------------------------------------------------------------------
 * DAT0 | DAT1 | DAT2 | DAT3 | FDM0 | FDM1 | FDM2 | FDM3 | ECC0 | ECC1 | ECC2 | ECC3
 * 512B | 512B | 512B | 512B |  8B  |  8B  |  8B  |  8B  |  8+B |  8+B |  8+B |  8+B
 * ---------------------------------------------------------------------------------
 */

static inline u8 *oob_fdm_ptr(struct nand_chip *chip, int sector_index)
{
	return chip->oob_poi + sector_index * NFI_ECC_FDM_SIZE;
}

static inline u8 *oob_ecc_ptr(mt7621_nfc_sel_t *nfc_sel, int sector_index)
{
	struct nand_chip *chip = &nfc_sel->nand;

	return chip->oob_poi + chip->ecc.steps * NFI_ECC_FDM_SIZE +
		sector_index * (nfc_sel->spare_per_sector - NFI_ECC_FDM_SIZE);
}

static inline u8 *page_data_ptr(struct nand_chip *chip, const u8 *buf,
				int sector_index)
{
	return (u8 *) buf + sector_index * chip->ecc.size;
}

static inline u8 *raw_page_fdm_ptr(mt7621_nfc_sel_t *nfc_sel, int sector_index)
{
	struct nand_chip *chip = &nfc_sel->nand;

	return (u8 *) nfc_sel->page_cache +
		sector_index * nfc_sel->spare_per_sector +
		(sector_index + 1) * chip->ecc.size;
}

static inline u8 *raw_page_ecc_ptr(mt7621_nfc_sel_t *nfc_sel, int sector_index)
{
	return raw_page_fdm_ptr(nfc_sel, sector_index) + NFI_ECC_FDM_SIZE;
}

static inline u8 *raw_page_data_ptr(mt7621_nfc_sel_t *nfc_sel, int sector_index)
{
	struct nand_chip *chip = &nfc_sel->nand;

	return (u8 *) nfc_sel->page_cache +
		sector_index * (chip->ecc.size + nfc_sel->spare_per_sector);
}

static int nfc_dev_ready(struct mtd_info *mtd)
{
	mt7621_nfc_t *nfc = nand_get_controller_data(mtd_to_nand(mtd));

	if (nfi_read32(nfc, NFI_STA_REG32) & BUSY)
		return 0;

	return 1;
}

static int nfc_wait_status_ready(mt7621_nfc_t *nfc, u32 status,
				 const char *st_name)
{
	u32 val;
	int ret;

	ret = readl_poll_timeout(nfc->nfi_base + NFI_STA_REG32, val,
		!(val & status), NFI_STATUS_WAIT_TIMEOUT_US);

	if (ret) {
		printf("Warning: NFI master timed out for %s ready\n",
		       st_name);
		return -EIO;
	}

	return 0;
}

static int nfc_wait_write_completion(mt7621_nfc_t *nfc, struct nand_chip *chip)
{
	int ret;
	u32 reg;

	ret = readl_poll_timeout(nfc->nfi_base + NFI_ADDRCNTR_REG16, reg,
		REG_GET_VAL(SEC_CNTR, reg) >= chip->ecc.steps,
		NFI_STATUS_WAIT_TIMEOUT_US);

	if (ret)
		printf("Error: NFI master timed out for write completion\n");

	return ret;
}



static void nfc_hw_reset(mt7621_nfc_t *nfc)
{
	u32 val, mask;
	int ret;

	mask = REG_MASK(MAS_ADDR) | REG_MASK(MAS_RD) | REG_MASK(MAS_WR) |
	       REG_MASK(MAS_RDDLY);

	/* reset all registers and force the NFI master to terminate */
	nfi_write16(nfc, NFI_CON_REG16, NFI_FIFO_FLUSH | NFI_STM_RST);

	/* wait for the master to finish the last transaction */
	ret = readw_poll_timeout(nfc->nfi_base + NFI_MASTERSTA_REG16, val,
		!(val & mask), NFI_STATUS_WAIT_TIMEOUT_US);

	if (ret)
		printf("Warning: NFI master is still active after reset\n");

	/* ensure any status register affected by the NFI master is reset */
	nfi_write16(nfc, NFI_CON_REG16, NFI_FIFO_FLUSH | NFI_STM_RST);
	nfi_write16(nfc, NFI_STRDATA_REG16, 0);
}

static void nfc_select_chip(struct mtd_info *mtd, int chipnr)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	mt7621_nfc_t *nfc = nand_get_controller_data(chip);
	mt7621_nfc_sel_t *nfc_sel = nand_to_mt7621_chip(chip);

	if (chipnr < 0)
		return;

	nfi_write16(nfc, NFI_PAGEFMT_REG16, nfc_sel->pagefmt_val);
	nfi_write32(nfc, NFI_ACCCON_REG32, nfc_sel->acccon_val);

	nfi_write16(nfc, NFI_CSEL_REG16, nfc_sel->cs);
}

static int mtk_nfc_send_address(mt7621_nfc_t *nfc, int addr)
{
	nfi_write32(nfc, NFI_COLADDR_REG32, addr);
	nfi_write32(nfc, NFI_ROWADDR_REG32, 0);
	nfi_write16(nfc, NFI_ADDRNOB_REG16, REG_SET_VAL(ADDR_COL_NOB, 1));

	return nfc_wait_status_ready(nfc, ADDR, "address mode");
}

static int mtk_nfc_send_command(mt7621_nfc_t *nfc, u8 cmd)
{
	nfi_write32(nfc, NFI_CMD_REG16, cmd);

	return nfc_wait_status_ready(nfc, STA_CMD, "command mode");
}

static void nfc_cmd_ctrl(struct mtd_info *mtd, int dat, unsigned int ctrl)
{
	mt7621_nfc_t *nfc = nand_get_controller_data(mtd_to_nand(mtd));

	if (ctrl & NAND_ALE) {
		mtk_nfc_send_address(nfc, dat & 0xff);
	} else if (ctrl & NAND_CLE) {
		nfc_hw_reset(nfc);
		nfi_write16(nfc, NFI_CNFG_REG16,
			    REG_SET_VAL(OP_MODE, OP_CUSTOM));
		mtk_nfc_send_command(nfc, dat);
	}
}

static int nfc_is_pio_ready(mt7621_nfc_t *nfc)
{
	u32 val;
	int ret;

	ret = readl_poll_timeout(nfc->nfi_base + NFI_PIO_DIRDY_REG16, val,
		(val & PIO_DIRDY), NFI_STATUS_WAIT_TIMEOUT_US);

	if (ret) {
		printf("warning: NFI master PIO mode not ready\n");
		return -EIO;
	}

	return 0;
}

static void nfc_pio_write(struct mtd_info *mtd, u32 val, int bw)
{
	mt7621_nfc_t *nfc = nand_get_controller_data(mtd_to_nand(mtd));
	u32 reg;

	reg = REG_GET_VAL(NAND_FSM, nfi_read32(nfc, NFI_STA_REG32));
	if (reg != FSM_CUSTOM_DATA) {
		if (bw)
			nfi_clrsetbits16(nfc, NFI_CNFG_REG16,
					 READ_MODE, BYTE_RW);
		else
			nfi_clrbits16(nfc, NFI_CNFG_REG16,
				      READ_MODE | BYTE_RW);

		nfi_write16(nfc, NFI_CON_REG16, REG_MASK(NFI_SEC) | NFI_BWR);

		nfi_write16(nfc, NFI_STRDATA_REG16, STR_DATA);
	}

	nfc_is_pio_ready(nfc);

	nfi_write32(nfc, NFI_DATAW_REG32, val);
}

static void nfc_write_byte(struct mtd_info *mtd, u8 byte)
{
	nfc_pio_write(mtd, byte, 1);
}

static void nfc_write_buf(struct mtd_info *mtd, const u8 *buf, int len)
{
	int i;
	const u32 *p = (const u32 *) buf;

	if ((size_t) buf % sizeof(u32) || len % sizeof(u32)) {
		for (i = 0; i < len; i++)
			nfc_pio_write(mtd, buf[i], 1);
	} else {
		for (i = 0; i < (len / sizeof (u32)); i++)
			nfc_pio_write(mtd, p[i], 0);
	}
}

static u32 nfc_pio_read(struct mtd_info *mtd, int br)
{
	mt7621_nfc_t *nfc = nand_get_controller_data(mtd_to_nand(mtd));
	u32 reg;

	/* after each byte read, the NFI_STA reg is reset by the hardware */
	reg = REG_GET_VAL(NAND_FSM, nfi_read32(nfc, NFI_STA_REG32));
	if (reg != FSM_CUSTOM_DATA) {
		if (br)
			nfi_setbits16(nfc, NFI_CNFG_REG16,
				      READ_MODE | BYTE_RW);
		else
			nfi_clrsetbits16(nfc, NFI_CNFG_REG16,
					 BYTE_RW, READ_MODE);

		/*
		* set to max sector to allow the HW to continue reading over
		* unaligned accesses
		*/
		nfi_write16(nfc, NFI_CON_REG16, REG_MASK(NFI_SEC) | NFI_BRD);

		/* trigger to fetch data */
		nfi_write16(nfc, NFI_STRDATA_REG16, STR_DATA);
	}

	nfc_is_pio_ready(nfc);

	return nfi_read32(nfc, NFI_DATAR_REG32);
}

static u8 nfc_read_byte(struct mtd_info *mtd)
{
	return nfc_pio_read(mtd, 1) & 0xff;
}

static void nfc_read_buf(struct mtd_info *mtd, u8 *buf, int len)
{
	int i;
	u32 *p = (u32 *) buf;

	if ((size_t) buf % sizeof(u32) || len % sizeof(u32)) {
		for (i = 0; i < len; i++)
			buf[i] = nfc_pio_read(mtd, 1);
	} else {
		for (i = 0; i < (len / sizeof(u32)); i++)
			p[i] = nfc_pio_read(mtd, 0);
	}
}

static void nfc_write_oob(struct nand_chip *chip, void *fdm)
{
	mt7621_nfc_t *nfc = nand_get_controller_data(chip);
	u8 *fdm_buf[68], *p = (u8 *) ALIGN((u32) fdm_buf, sizeof(u32));
	u32 *ptr = (u32 *) p;
	u8 chksum = 0, empty = 1;
	u32 i, j;

	memcpy(p, fdm, chip->ecc.steps * NFI_ECC_FDM_SIZE - 1);

	for (i = 0; i < chip->ecc.steps; i++) {
		for (j = 0; j < chip->ecc.layout->oobfree[i].length; j++) {
			if (p[chip->ecc.layout->oobfree[i].offset + j] != 0xff)
				empty = 0;
			chksum ^= p[chip->ecc.layout->oobfree[i].offset + j];
		}
	}

	if (!empty)
		p[chip->ecc.steps * NFI_ECC_FDM_SIZE - 1] = chksum;
	else
		p[chip->ecc.steps * NFI_ECC_FDM_SIZE - 1] = 0xff;

	for (i = 0; i < chip->ecc.steps; i++) {
		nfi_write32(nfc, NFI_FDML_REG32(i), *ptr++);
		nfi_write32(nfc, NFI_FDMM_REG32(i), *ptr++);
	}
}

static void nfc_read_sector_oob(mt7621_nfc_t *nfc, u32 idx, void *out)
{
	u32 tmp, *p = (u32 *) out;

	if ((size_t) out % sizeof(u32) == 0) {
		p[0] = nfi_read32(nfc, NFI_FDML_REG32(idx));
		p[1] = nfi_read32(nfc, NFI_FDMM_REG32(idx));
	} else {
		tmp = nfi_read32(nfc, NFI_FDML_REG32(idx));
		memcpy(&p[0], &tmp, 4);
		tmp = nfi_read32(nfc, NFI_FDMM_REG32(idx));
		memcpy(&p[1], &tmp, 4);
	}
}

static int nfc_read_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip,
	u8 *buf, int oob_on, int page)
{
	mt7621_nfc_t *nfc = nand_get_controller_data(chip);
	mt7621_nfc_sel_t *nfc_sel = nand_to_mt7621_chip(chip);
	int bitflips;
	int rc, i;

	nfi_setbits16(nfc, NFI_CNFG_REG16,
		      READ_MODE | AUTO_FMT_EN | HW_ECC_EN);

	nfc_ecc_init(nfc, &chip->ecc);
	nfc_ecc_decoder_start(nfc);

	nfi_write16(nfc, NFI_CON_REG16,
		    NFI_BRD | REG_SET_VAL(NFI_SEC, chip->ecc.steps));

	bitflips = 0;

	for (i = 0; i < chip->ecc.steps; i++) {
		nfc_read_buf(mtd, page_data_ptr(chip, buf, i), chip->ecc.size);

		rc = nfc_ecc_decoder_wait_done(nfc, i);

		nfc_read_sector_oob(nfc, i, oob_fdm_ptr(chip, i));

		if (rc < 0) {
			bitflips = -EIO;
		} else {
			rc = nfc_ecc_correct_check(nfc, nfc_sel,
				page_data_ptr(chip, buf, i),
				oob_fdm_ptr(chip, i), i);

			if (rc < 0) {
				printf("Unrecoverable ECC error at page %u.%u\n",
				       page, i);
				bitflips = -EBADMSG;
			} else if (bitflips >= 0) {
				bitflips += rc;
			}
		}
	}

	nfc_ecc_decoder_stop(nfc);

	nfi_write16(nfc, NFI_CON_REG16, 0);

	return bitflips;
}

static int nfc_read_page_raw(struct mtd_info *mtd, struct nand_chip *chip,
	u8 *buf, int oob_on, int page)
{
	mt7621_nfc_t *nfc = nand_get_controller_data(chip);
	mt7621_nfc_sel_t *nfc_sel = nand_to_mt7621_chip(chip);
	int i;

	nfi_setbits16(nfc, NFI_CNFG_REG16, READ_MODE);

	nfi_write16(nfc, NFI_CON_REG16,
		    NFI_BRD | REG_SET_VAL(NFI_SEC, chip->ecc.steps));

	nfc_read_buf(mtd, nfc_sel->page_cache,
		     mtd->writesize + nfc_sel->oobsize_avail);

	nfi_write16(nfc, NFI_CON_REG16, 0);

	for (i = 0; i < chip->ecc.steps; i++) {
		memcpy(oob_fdm_ptr(chip, i), raw_page_fdm_ptr(nfc_sel, i),
			NFI_ECC_FDM_SIZE);

		memcpy(oob_ecc_ptr(nfc_sel, i), raw_page_ecc_ptr(nfc_sel, i),
			nfc_sel->spare_per_sector - NFI_ECC_FDM_SIZE);
	}

	if (buf) {
		for (i = 0; i < chip->ecc.steps; i++)
			memcpy(page_data_ptr(chip, buf, i),
			       raw_page_data_ptr(nfc_sel, i),
			       chip->ecc.size);
	}

	return 0;
}

static int nfc_read_oob_std(struct mtd_info *mtd, struct nand_chip *chip,
	int page)
{
	mt7621_nfc_sel_t *nfc_sel = nand_to_mt7621_chip(chip);

	chip->cmdfunc(mtd, NAND_CMD_READ0, 0, page);

	return nfc_read_page_hwecc(mtd, chip, nfc_sel->page_cache, 1, page);
}

static int nfc_read_oob_raw(struct mtd_info *mtd, struct nand_chip *chip,
	int page)
{
	chip->cmdfunc(mtd, NAND_CMD_READ0, 0, page);

	return nfc_read_page_raw(mtd, chip, NULL, 1, page);
}

static int nfc_check_empty_page(struct mtd_info *mtd, struct nand_chip *chip,
				const u8 *buf)
{
	uint32_t i, j;
	u8 *oob_ptr;

	for (i = 0; i < mtd->writesize; i++)
		if (buf[i] != 0xff)
			return 0;

	for (i = 0; i < chip->ecc.steps; i++) {
		oob_ptr = oob_fdm_ptr(chip, i);
		for (j = 0; j < NFI_ECC_FDM_SIZE; j++)
			if (oob_ptr[j] != 0xff)
				return 0;
	}

	return 1;
}

static int nfc_write_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip,
	const u8 *buf, int oob_on, int page)
{
	mt7621_nfc_t *nfc = nand_get_controller_data(chip);
	int ret;

	if (nfc_check_empty_page(mtd, chip, buf)) {
		/*
		 * When the entire page is 0xff including oob data,
		 * do not use ecc engine which will write ecc parity code
		 * back to oob region.
		 *
		 * For 4-bit ecc strength, the ecc parity code of a full
		 * 0xff subpage is 26 20 98 1b 87 6e fc
		 *
		 * Use raw mode instead.
		 */
		return nfc_write_page_raw(mtd, chip, NULL, oob_on, page);
	}

	nfi_clrsetbits16(nfc, NFI_CNFG_REG16, READ_MODE,
			 AUTO_FMT_EN | HW_ECC_EN);

	nfc_ecc_init(nfc, &chip->ecc);
	nfc_ecc_encoder_start(nfc);

	nfc_write_oob(chip, chip->oob_poi);

	nfi_write16(nfc, NFI_CON_REG16,
		    NFI_BWR | REG_SET_VAL(NFI_SEC, chip->ecc.steps));

	nfc_write_buf(mtd, buf, mtd->writesize);

	ret = nfc_wait_write_completion(nfc, chip);

	nfc_ecc_encoder_stop(nfc);

	nfi_write16(nfc, NFI_CON_REG16, 0);

	return ret;
}

static int nfc_write_page_raw(struct mtd_info *mtd, struct nand_chip *chip,
	const u8 *buf, int oob_on, int page)
{
	mt7621_nfc_t *nfc = nand_get_controller_data(chip);
	mt7621_nfc_sel_t *nfc_sel = nand_to_mt7621_chip(chip);
	int i, ret;

	memset(nfc_sel->page_cache, 0xff, mtd->writesize + mtd->oobsize);

	for (i = 0; i < chip->ecc.steps; i++)
		memcpy(raw_page_fdm_ptr(nfc_sel, i), oob_fdm_ptr(chip, i),
			NFI_ECC_FDM_SIZE);

	if (buf) {
		for (i = 0; i < chip->ecc.steps; i++)
			memcpy(raw_page_data_ptr(nfc_sel, i),
			       page_data_ptr(chip, buf, i),
			       chip->ecc.size);
	}

	nfi_clrbits16(nfc, NFI_CNFG_REG16,
		      READ_MODE | AUTO_FMT_EN | HW_ECC_EN);

	nfc_write_buf(mtd, nfc_sel->page_cache,
		      mtd->writesize + nfc_sel->oobsize_avail);

	ret = nfc_wait_write_completion(nfc, chip);

	nfi_write16(nfc, NFI_CON_REG16, 0);

	return ret;
}

static int nfc_write_oob_std(struct mtd_info *mtd, struct nand_chip *chip,
	int page)
{
	mt7621_nfc_sel_t *nfc_sel = nand_to_mt7621_chip(chip);
	int ret;

	memset(nfc_sel->page_cache, 0xff, mtd->writesize + mtd->oobsize);

	chip->cmdfunc(mtd, NAND_CMD_SEQIN, 0x00, page);

	ret = nfc_write_page_hwecc(mtd, chip, nfc_sel->page_cache, 1, page);
	if (ret < 0)
		return -EIO;

	chip->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);
	ret = chip->waitfunc(mtd, chip);

	return ret & NAND_STATUS_FAIL ? -EIO : 0;
}

static int nfc_write_oob_raw(struct mtd_info *mtd, struct nand_chip *chip,
	int page)
{
	int ret;

	chip->cmdfunc(mtd, NAND_CMD_SEQIN, 0x00, page);

	ret = nfc_write_page_raw(mtd, chip, NULL, 1, page);
	if (ret < 0)
		return -EIO;

	chip->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);
	ret = chip->waitfunc(mtd, chip);

	return ret & NAND_STATUS_FAIL ? -EIO : 0;
}

static int nfc_block_bad(struct mtd_info *mtd, loff_t ofs)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	int page, res = 0, i = 0;
	u16 bad;

	if (chip->bbt_options & NAND_BBT_SCANLASTPAGE)
		ofs += mtd->erasesize - mtd->writesize;

	page = (int) (ofs >> chip->page_shift) & chip->pagemask;

	do {
		chip->cmdfunc(mtd, NAND_CMD_READ0,
			chip->ecc.size + chip->badblockpos, page);

		bad = chip->read_byte(mtd);
		res = bad != 0xFF;

		ofs += mtd->writesize;
		page = (int) (ofs >> chip->page_shift) & chip->pagemask;
		i++;
	} while (!res && i < 2 && (chip->bbt_options & NAND_BBT_SCAN2NDPAGE));

	return res;
}

static int nfc_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	mt7621_nfc_sel_t *nfc_sel = nand_to_mt7621_chip(chip);
	loff_t lofs;
	int page, ret = 0, res, i = 0;

	/* Backup original OOB data */
	memcpy(nfc_sel->oob_mb_cache, chip->oob_poi, mtd->oobsize);

	/* Create bad block mark OOD bata */
	memset(chip->oob_poi, 0xff, mtd->oobsize);
	chip->oob_poi[chip->badblockpos] = 0;

	/* For BootROM compatibility, always write to offset 0 */
	chip->oob_poi[0] = 0;

	/* Write to last page(s) if necessary */
	if (chip->bbt_options & NAND_BBT_SCANLASTPAGE) {
		lofs = ofs + mtd->erasesize - mtd->writesize;
		if (chip->bbt_options & NAND_BBT_SCAN2NDPAGE)
			lofs -= mtd->writesize;

		do {
			page = lofs >> mtd->writesize_shift;
			res = nfc_write_oob_raw(mtd, chip, page);
			if (!ret)
				ret = res;

			i++;
			lofs += mtd->writesize;
		} while ((chip->bbt_options & NAND_BBT_SCAN2NDPAGE) && i < 2);
	}

	/* For BootROM and SPL compatibility, always write to first page(s) */
	i = 0;
	do {
		page = ofs >> mtd->writesize_shift;
		res = nfc_write_oob_raw(mtd, chip, page);
		if (!ret)
			ret = res;

		i++;
		ofs += mtd->writesize;
	} while ((chip->bbt_options & NAND_BBT_SCAN2NDPAGE) && i < 2);

	/* Restore original OOB data */
	memcpy(chip->oob_poi, nfc_sel->oob_mb_cache, mtd->oobsize);

	return ret;
}

static void nfc_set_nand_ecc_layout(mt7621_nfc_sel_t *nfc_sel)
{
	struct nand_chip *chip = &nfc_sel->nand;
	struct mtd_info *mtd = nand_to_mtd(chip);
	u32 fdm_size;
	int i;

	chip->ecc.layout = &nfc_sel->ecc_layout;

	fdm_size = chip->ecc.steps * (NFI_ECC_FDM_SIZE - 1) - 1;

	nfc_sel->ecc_layout.eccbytes = mtd->oobsize - fdm_size;
	nfc_sel->ecc_layout.oobavail = fdm_size;

	for (i = 0; i < nfc_sel->ecc_layout.eccbytes; i++)
		nfc_sel->ecc_layout.eccpos[i] = fdm_size + i;

	for (i = 0; i < chip->ecc.steps; i++) {
		nfc_sel->ecc_layout.oobfree[i].offset = i * NFI_ECC_FDM_SIZE + 1;
		nfc_sel->ecc_layout.oobfree[i].length = NFI_ECC_FDM_SIZE - 1;

		if (i == chip->ecc.steps - 1)
			nfc_sel->ecc_layout.oobfree[i].length--;
	}
}

static int nfc_ooblayout_free(struct mtd_info *mtd, int section,
	struct mtd_oob_region *oob_region)
{
	struct nand_chip *chip = mtd_to_nand(mtd);

	if (section >= chip->ecc.steps)
		return -ERANGE;

	oob_region->offset = section * NFI_ECC_FDM_SIZE + 1;
	oob_region->length = NFI_ECC_FDM_SIZE - 1;

	/* Last byte of last FDM is used to store checksum */
	if (section == chip->ecc.steps - 1)
		oob_region->length--;

	return 0;
}

static int nfc_ooblayout_ecc(struct mtd_info *mtd, int section,
	struct mtd_oob_region *oob_region)
{
	struct nand_chip *chip = mtd_to_nand(mtd);

	if (section)
		return -ERANGE;

	oob_region->offset = chip->ecc.steps * NFI_ECC_FDM_SIZE;
	oob_region->length = mtd->oobsize - oob_region->offset;

	return 0;
}

static const struct mtd_ooblayout_ops nfc_ooblayout_ops = {
	.free = nfc_ooblayout_free,
	.ecc = nfc_ooblayout_ecc,
};

static int nfc_init_chip_config(mt7621_nfc_sel_t *nfc_sel)
{
	struct nand_chip *chip = &nfc_sel->nand;
	u32 spare_bytes;

	chip->ecc.size = NFI_ECC_SECTOR_SIZE;
	chip->ecc.steps = chip->mtd.writesize / chip->ecc.size;

	nfc_sel->spare_per_sector = chip->mtd.oobsize / chip->ecc.steps;

	if (nfc_sel->spare_per_sector >= 28) {
		spare_bytes = PAGEFMT_SPARE_28;
		chip->ecc.strength = 12;
		nfc_sel->spare_per_sector = 28;
	} else if (nfc_sel->spare_per_sector >= 27) {
		spare_bytes = PAGEFMT_SPARE_27;
		chip->ecc.strength = 8;
		nfc_sel->spare_per_sector = 27;
	} else if (nfc_sel->spare_per_sector >= 26) {
		spare_bytes = PAGEFMT_SPARE_26;
		chip->ecc.strength = 8;
		nfc_sel->spare_per_sector = 26;
	} else if (nfc_sel->spare_per_sector >= 16) {
		spare_bytes = PAGEFMT_SPARE_16;
		chip->ecc.strength = 4;
		nfc_sel->spare_per_sector = 16;
	} else {
		debug("Error: Not supported oob size %d\n", chip->mtd.oobsize);
		return -EINVAL;
	}

	nfc_sel->oobsize_avail = chip->ecc.steps * nfc_sel->spare_per_sector;

	chip->ecc.bytes =
		DIV_ROUND_UP(chip->ecc.strength * NFI_ECC_PARITY_BITS, 8);

	switch (chip->mtd.writesize) {
	case SZ_512:
		nfc_sel->pagefmt_val =
			REG_SET_VAL(PAGEFMT_SPARE, PAGEFMT_SPARE_16) |
			PAGEFMT_512;
		break;
	case SZ_2K:
		nfc_sel->pagefmt_val =
			REG_SET_VAL(PAGEFMT_SPARE, spare_bytes) | PAGEFMT_2K;
		break;
	case SZ_4K:
		nfc_sel->pagefmt_val =
			REG_SET_VAL(PAGEFMT_SPARE, spare_bytes) | PAGEFMT_4K;
		break;
	default:
		printf("Error: Not supported page size %d\n",
		       chip->mtd.writesize);
		return -EINVAL;
	}

	nfc_sel->pagefmt_val |=
		REG_SET_VAL(PAGEFMT_FDM_ECC, NFI_ECC_FDM_SIZE) |
		REG_SET_VAL(PAGEFMT_FDM, NFI_ECC_FDM_SIZE);

	return 0;
}

static void nfc_set_def_timing(mt7621_nfc_sel_t *nfc_sel, u8 *id)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mt7621_nfc_def_timings); i++) {
		if (!memcmp(mt7621_nfc_def_timings[i].id, id,
			    mt7621_nfc_def_timings[i].id_len)) {
			nfc_sel->acccon_val =
				mt7621_nfc_def_timings[i].acccon_val;
			return;
		}
	}

	nfc_sel->acccon_val = NFI_DEFAULT_ACCESS_TIMING;
}

static void nfc_probe(mt7621_nfc_t *nfc, int cs)
{
	mt7621_nfc_sel_t *nfc_sel = &nfc->sels[cs];
	struct nand_chip *chip = &nfc_sel->nand;
	u8 id[8];
	int ret;

	nand_set_controller_data(chip, nfc);
	nfc_sel->cs = cs;

	chip->options |= NAND_SKIP_BBTSCAN;
	chip->dev_ready = nfc_dev_ready;
	chip->select_chip = nfc_select_chip;
	chip->write_byte = nfc_write_byte;
	chip->write_buf = nfc_write_buf;
	chip->read_byte = nfc_read_byte;
	chip->read_buf = nfc_read_buf;
	chip->cmd_ctrl = nfc_cmd_ctrl;
	chip->block_bad = nfc_block_bad;
	chip->block_markbad = nfc_block_markbad;

	/* set default mode in case dt entry is missing */
	chip->ecc.mode = NAND_ECC_HW;

	chip->ecc.write_page_raw = nfc_write_page_raw;
	chip->ecc.write_page = nfc_write_page_hwecc;
	chip->ecc.write_oob_raw = nfc_write_oob_raw;
	chip->ecc.write_oob = nfc_write_oob_std;

	chip->ecc.read_page_raw = nfc_read_page_raw;
	chip->ecc.read_page = nfc_read_page_hwecc;
	chip->ecc.read_oob_raw = nfc_read_oob_raw;
	chip->ecc.read_oob = nfc_read_oob_std;

	mtd_set_ooblayout(&chip->mtd, &nfc_ooblayout_ops);

	nfc_sel->acccon_val = NFI_DEFAULT_ACCESS_TIMING;

	/* Reset NFI master */
	nfc_hw_reset(nfc);

	nfi_setbits16(nfc, NFI_CNFG_REG16, BYTE_RW);

	ret = nand_scan_ident(&chip->mtd, 1, NULL);
	if (ret)
		return;

	nfc_init_chip_config(nfc_sel);

	nfc_set_nand_ecc_layout(nfc_sel);

	/* set timeout for polling R/B status */
	nfi_write16(nfc, NFI_CNRN_REG16, REG_MASK(CB2R_TIME) | STR_CNRNB);

	ret = nand_scan_tail(&chip->mtd);
	if (ret)
		return;

	nfc_sel->page_cache = memalign(ARCH_DMA_MINALIGN,
				chip->mtd.writesize + chip->mtd.oobsize);
	if (!nfc_sel->page_cache)
		return;

	nfc_sel->oob_mb_cache = memalign(ARCH_DMA_MINALIGN, chip->mtd.oobsize);
	if (!nfc_sel->oob_mb_cache)
		return;

	/* Read ID to retrive preset ACCON settings */
	chip->cmdfunc(&chip->mtd, NAND_CMD_READID, 0x00, -1);
	chip->read_buf(&chip->mtd, id, 8);

	nfc_set_def_timing(nfc_sel, id);

	nand_register(cs, &chip->mtd);
}

void mt7621_nfc_spl_init(mt7621_nfc_t *nfc, int cs)
{
	mt7621_nfc_sel_t *nfc_sel = &nfc->sels[cs];
	struct nand_chip *chip = &nfc_sel->nand;

	nand_set_controller_data(chip, nfc);
	nfc_sel->cs = cs;

	chip->options |= NAND_SKIP_BBTSCAN;
	chip->dev_ready = nfc_dev_ready;
	chip->select_chip = nfc_select_chip;
	chip->read_byte = nfc_read_byte;
	chip->read_buf = nfc_read_buf;
	chip->cmd_ctrl = nfc_cmd_ctrl;
	chip->block_bad = nfc_block_bad;

	chip->ecc.read_page = nfc_read_page_hwecc;

	nfc_sel->acccon_val = NFI_DEFAULT_ACCESS_TIMING;

	/* Reset NFI master */
	nfc_hw_reset(nfc);

	nfi_setbits16(nfc, NFI_CNFG_REG16, BYTE_RW);
}

int mt7621_nfc_spl_post_init(mt7621_nfc_t *nfc, int cs)
{
	mt7621_nfc_sel_t *nfc_sel = &nfc->sels[cs];
	struct nand_chip *chip = &nfc_sel->nand;
	struct nand_flash_dev *type;
	int nand_maf_id, nand_dev_id;
	u8 id[8];

	type = nand_get_flash_type(&chip->mtd, chip, &nand_maf_id,
				   &nand_dev_id, NULL);

	if (IS_ERR(type))
		return PTR_ERR(type);

	chip->numchips = 1;
	chip->mtd.size = chip->chipsize;

	nfc_init_chip_config(nfc_sel);

	/* set timeout for polling R/B status */
	nfi_write16(nfc, NFI_CNRN_REG16, REG_MASK(CB2R_TIME) | STR_CNRNB);

	nfc_sel->page_cache = memalign(ARCH_DMA_MINALIGN,
				chip->mtd.writesize + chip->mtd.oobsize);
	if (!nfc_sel->page_cache)
		return -ENOMEM;

	/* Read ID to retrive preset ACCON settings */
	chip->cmdfunc(&chip->mtd, NAND_CMD_READID, 0x00, -1);
	chip->read_buf(&chip->mtd, id, 8);

	nfc_set_def_timing(nfc_sel, id);

	return 0;
}

void board_nand_init(void)
{
	int i;

	nfc_dev.nfi_base = (void __iomem *) CKSEG1ADDR(MT7621_NFI_BASE);
	nfc_dev.ecc_base = (void __iomem *) CKSEG1ADDR(MT7621_NFI_ECC_BASE);

	for (i = 0; i < CONFIG_SYS_NAND_MAX_CHIPS; i++)
		nfc_probe(&nfc_dev, i);
}