// SPDX-License-Identifier:	GPL-2.0+
/*
 * Copyright (C) 2018 MediaTek Incorporation. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <common.h>
#include <spl.h>
#include <u-boot/crc.h>
#include <linux/sizes.h>
#include <linux/mtd/mtd.h>
#include <asm/addrspace.h>
#include <nand.h>

#include "spl_helper.h"
#include "flash_helper.h"

#define MT7621_IH_NMLEN			12
#define MT7621_IH_CRC_POLYNOMIAL	0x04c11db7

typedef struct nand_header {
	uint8_t ih_name[MT7621_IH_NMLEN];
	uint32_t nand_ac_timing;
	uint32_t ih_stage_offset;
	uint32_t ih_bootloader_offset;
	uint32_t nand_info_1_data;
	uint32_t crc;
} nand_header_t;

static uint32_t crc32tbl[256];

static void crc32_init(void)
{
	if (crc32tbl[1] == MT7621_IH_CRC_POLYNOMIAL)
		return;

	crc32c_be_init(crc32tbl, MT7621_IH_CRC_POLYNOMIAL);
}

static uint32_t crc32_cal(const uint8_t *data, int length)
{
	uint32_t crc = 0;
	char c;

	crc32_init();

	crc = crc32c_be_cal(crc, (char *) data, length, crc32tbl);

	for (; length; length >>= 8) {
		c = length & 0xff;
		crc = crc32c_be_cal(crc, &c, 1, crc32tbl);
	}

	return ~crc;
}

char *get_mtk_stage2_image_ptr(char *data, size_t size)
{
	struct mtk_spl_rom_cfg rom_cfg;
	nand_header_t *mt7621_nhdr;
	struct image_header hdr;
	u32 old_chksum, chksum;
	u32 off;

	memcpy(&hdr, data, sizeof(hdr));

	if (image_get_magic(&hdr) == IH_MAGIC) {
		old_chksum = image_get_hcrc(&hdr);
		hdr.ih_hcrc = 0;
		chksum = crc32(0, (const u8 *) &hdr, sizeof(hdr));

		if (chksum != old_chksum)
			return NULL;

		/* For NAND boot, determine whether this is the SPL part */
		mt7621_nhdr = (nand_header_t *) hdr.ih_name;

		old_chksum = uimage_to_cpu(mt7621_nhdr->crc);
		mt7621_nhdr->crc = 0;
		chksum = crc32_cal((uint8_t *) &hdr, sizeof(hdr));

		if (chksum != old_chksum)
			return data;
	}

	/* Locate stage2 */
	memcpy(&rom_cfg, data + sizeof(hdr) + MTK_SPL_ROM_CFG_OFFS,
	       sizeof(rom_cfg));

	if (rom_cfg.magic != MTK_ROM_CFG_MAGIC)
		return NULL;

	off = rom_cfg.size + sizeof(hdr);

	if (!rom_cfg.align)
		return data + off;

	off = ALIGN(off, rom_cfg.align);

	while (off < size) {
		memcpy(&hdr, data + off, sizeof(hdr));

		if (image_get_magic(&hdr) == IH_MAGIC) {
			old_chksum = image_get_hcrc(&hdr);
			hdr.ih_hcrc = 0;
			chksum = crc32(0, (const u8 *) &hdr, sizeof(hdr));

			if (chksum == old_chksum)
				return data + off;
		}

		off += rom_cfg.align;
	}

	return NULL;
}

int check_mtk_stock_stage2_info(size_t *stage2_off_min, size_t *stage2_off,
				size_t *align_size)
{
#ifdef CONFIG_MAX_U_BOOT_SIZE
	struct mtk_spl_rom_cfg rom_cfg;
	nand_header_t *mt7621_nhdr;
	struct image_header hdr;
	u32 old_chksum, chksum;
	struct mtd_info *mtd;
	u32 off, addr;
	size_t retlen;
	int ret;

	mtd = get_mtk_board_nand_mtd();
	if (!mtd)
		return 0;

	ret = mtd_read(mtd, 0, sizeof(hdr), &retlen, (u8 *) &hdr);
	if ((ret && ret != -EUCLEAN) || (retlen != sizeof(hdr)))
		return 0;

	if (image_get_magic(&hdr) == IH_MAGIC) {
		old_chksum = image_get_hcrc(&hdr);
		hdr.ih_hcrc = 0;
		chksum = crc32(0, (const u8 *) &hdr, sizeof(hdr));

		if (chksum != old_chksum)
			return 0;

		/* For NAND boot, determine whether this is the SPL part */
		mt7621_nhdr = (nand_header_t *) hdr.ih_name;

		old_chksum = uimage_to_cpu(mt7621_nhdr->crc);
		mt7621_nhdr->crc = 0;
		chksum = crc32_cal((uint8_t *) &hdr, sizeof(hdr));

		if (chksum != old_chksum)
			return 0;
	}

	ret = mtd_read(mtd, sizeof(hdr) + MTK_SPL_ROM_CFG_OFFS,
		       sizeof(rom_cfg), &retlen, (u8 *) &rom_cfg);
	if ((ret && ret != -EUCLEAN) || (retlen != sizeof(rom_cfg)))
		return 0;

	if (rom_cfg.magic != MTK_ROM_CFG_MAGIC)
		return 0;

	off = rom_cfg.size + sizeof(hdr);

	if (!rom_cfg.align)
		goto end;

	off = ALIGN(off, rom_cfg.align);

	addr = off;

	while (addr < CONFIG_MAX_U_BOOT_SIZE) {
		ret = mtd_read(mtd, addr, sizeof(hdr), &retlen, (u8 *) &hdr);
		if ((!ret || ret == -EUCLEAN) && (retlen == sizeof(hdr))) {
			if (image_get_magic(&hdr) == IH_MAGIC) {
				old_chksum = image_get_hcrc(&hdr);
				hdr.ih_hcrc = 0;
				chksum = crc32(0, (const u8 *) &hdr,
					       sizeof(hdr));

				if (chksum == old_chksum) {
					*stage2_off_min = off;
					*stage2_off = addr;
					*align_size = rom_cfg.align;
					return 1;
				}
			}
		}

		addr += rom_cfg.align;
	}

end:
	*stage2_off_min = off;
	*stage2_off = off;
	*align_size = rom_cfg.align;

	return 1;
#else
	return 0;
#endif
}