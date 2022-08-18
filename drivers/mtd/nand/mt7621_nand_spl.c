// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2018 MediaTek Incorporation. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <common.h>
#include <nand.h>
#include <malloc.h>
#include <asm/addrspace.h>

#include <mach/mt7621_regs.h>

#include "mt7621_nand.h"

static mt7621_nfc_t nfc_dev;
static u8 *buffer;
static int nand_valid;

static void nand_command(struct mtd_info *mtd, unsigned int command,
			    int column, int page_addr)
{
	register struct nand_chip *chip = mtd_to_nand(mtd);
	u32 timeo = (CONFIG_SYS_HZ * 400) / 1000;
	u32 time_start;

	/* Command latch cycle */
	chip->cmd_ctrl(mtd, command, NAND_CLE);

	/* Serially input address */
	if (column != -1) {
		chip->cmd_ctrl(mtd, column, NAND_ALE);
		if (command != NAND_CMD_READID)
			chip->cmd_ctrl(mtd, column >> 8, NAND_ALE);
	}
	if (page_addr != -1) {
		chip->cmd_ctrl(mtd, page_addr, NAND_ALE);
		chip->cmd_ctrl(mtd, page_addr >> 8, NAND_ALE);
		if (chip->options & NAND_ROW_ADDR_3)
			chip->cmd_ctrl(mtd, page_addr >> 16, NAND_ALE);
	}
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);

	if (command == NAND_CMD_READ0)
		chip->cmd_ctrl(mtd, NAND_CMD_READSTART, NAND_CLE);

	/*
	 * Apply this short delay always to ensure that we do wait tWB in
	 * any case on any machine.
	 */
	ndelay(100);

	time_start = get_timer(0);
	/* Wait until command is processed or timeout occurs */
	while (get_timer(time_start) < timeo) {
		if (chip->dev_ready(mtd))
			break;
	}

	if (!chip->dev_ready(mtd))
		pr_warn("timeout while waiting for chip to become ready\n");
}

static int nand_read_page_hwecc(struct mtd_info *mtd, void *buf,
				unsigned int page)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	int ret;

	chip->cmdfunc(mtd, NAND_CMD_READ0, 0x0, page);

	ret = chip->ecc.read_page(mtd, chip, buf, 1, page);
	if (ret < 0)
		return -1;

	return 0;
}

int nand_spl_load_image(uint32_t offs, unsigned int size, void *dest)
{
	mt7621_nfc_sel_t *nfc_sel = &nfc_dev.sels[0];
	struct nand_chip *chip = &nfc_sel->nand;
	struct mtd_info *mtd = &chip->mtd;
	uint32_t col, page, bytes_to_read;
	int curr_block;
	u8 *buf = dest;

	if (!nand_valid)
		return -ENODEV;

	/* Apply timing and pagefmt settings */
	chip->select_chip(mtd, 0);

	curr_block = -1;

	while (size) {
		if ((int)(offs >> mtd->erasesize_shift) > curr_block) {
			curr_block = offs >> mtd->erasesize_shift;

			if (chip->block_bad(mtd, offs)) {
				/* Avoid 4GiB overflow */
				if (offs >= mtd->size - mtd->erasesize)
					return -1;

				offs += mtd->erasesize;
				continue;
			}
		}

		col = offs & mtd->writesize_mask;
		page = offs >> mtd->writesize_shift;
		bytes_to_read = min(mtd->writesize - col, (uint32_t) size);

		if (nand_read_page_hwecc(mtd, buffer, page))
			return -1;

		memcpy(buf, buffer + col, bytes_to_read);

		buf += bytes_to_read;
		offs += bytes_to_read;
		size -= bytes_to_read;
	}

	return 0;
}

int nand_default_bbt(struct mtd_info *mtd)
{
	return 0;
}

unsigned long nand_size(void)
{
	if (nand_valid)
		return nfc_dev.sels[0].nand.chipsize;

	return 0;
}

void nand_deselect(void)
{
}

void nand_init(void)
{
	struct nand_chip *chip;
	struct mtd_info *mtd;

	if (nand_valid)
		return;

	nfc_dev.nfi_base = (void __iomem *) CKSEG1ADDR(MT7621_NFI_BASE);
	nfc_dev.ecc_base = (void __iomem *) CKSEG1ADDR(MT7621_NFI_ECC_BASE);
	nfc_dev.sels[0].acccon_val = NFI_DEFAULT_ACCESS_TIMING;

	mt7621_nfc_spl_init(&nfc_dev, 0);

	chip = &nfc_dev.sels[0].nand;
	mtd = &chip->mtd;
	chip->cmdfunc = nand_command;

	if (mt7621_nfc_spl_post_init(&nfc_dev, 0))
		return;

	if (is_power_of_2(mtd->erasesize))
		mtd->erasesize_shift = ffs(mtd->erasesize) - 1;
	else
		mtd->erasesize_shift = 0;

	if (is_power_of_2(mtd->writesize))
		mtd->writesize_shift = ffs(mtd->writesize) - 1;
	else
		mtd->writesize_shift = 0;

	mtd->erasesize_mask = (1 << mtd->erasesize_shift) - 1;
	mtd->writesize_mask = (1 << mtd->writesize_shift) - 1;

	buffer = (u8 *) malloc(mtd->writesize);
	if (!buffer)
		return;

	nand_valid = 1;
}
