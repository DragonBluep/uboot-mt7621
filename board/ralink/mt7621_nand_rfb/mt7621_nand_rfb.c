// SPDX-License-Identifier:	GPL-2.0+
/*
 * Copyright (C) 2018 MediaTek Incorporation. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <common.h>
#include <image.h>
#include <linux/types.h>
#include <debug_uart.h>
#include <asm/spl.h>
#include <asm/io.h>
#include <nand.h>
#include <linux/mtd/mtd.h>
#include <mach/mt7621_regs.h>

#include <nmbm/nmbm.h>
#include <nmbm/nmbm-mtd.h>

#ifdef CONFIG_ENABLE_NAND_NMBM
static int nmbm_usable;
#endif

#ifdef CONFIG_DEBUG_UART_BOARD_INIT
void board_debug_uart_init(void)
{
#if defined(CONFIG_SPL_BUILD)
	void __iomem *base = (void __iomem *) CKSEG1ADDR(MT7621_SYSCTL_BASE);

	setbits_le32(base + MT7621_SYS_RSTCTL_REG, REG_SET_VAL(UART1_RST, 1));
	clrbits_le32(base + MT7621_SYS_RSTCTL_REG, REG_MASK(UART1_RST));
	clrbits_le32(base + MT7621_SYS_GPIO_MODE_REG, REG_MASK(UART1_MODE));
#endif
}
#endif

int board_nmbm_init(void)
{
#ifdef CONFIG_ENABLE_NAND_NMBM
	struct mtd_info *lower, *upper;
	int ret;

	printf("\n");
	printf("Initializing NMBM ...\n");

	lower = get_nand_dev_by_index(0);
	if (!lower) {
		printf("Failed to create NMBM device due to nand0 not found\n");
		return 0;
	}

	ret = nmbm_attach_mtd(lower, NMBM_F_CREATE, CONFIG_NMBM_MAX_RATIO,
		CONFIG_NMBM_MAX_BLOCKS, &upper);

	printf("\n");

	if (ret)
		return 0;

	add_mtd_device(upper);

	nmbm_usable = 1;
#endif

	return 0;
}

#ifdef CONFIG_SPL_BUILD
void board_boot_order(u32 *spl_boot_list)
{
	spl_boot_list[0] = BOOT_DEVICE_MTK_NAND;
	spl_boot_list[1] = BOOT_DEVICE_MTK_UART;
}

ulong get_mtk_image_search_start(void)
{
	if (__rom_cfg.magic != MTK_ROM_CFG_MAGIC)
		return CONFIG_SPL_ALIGN_TO;

	return __rom_cfg.size + sizeof(struct image_header);
}

ulong get_mtk_image_search_end(void)
{
	return CONFIG_MAX_U_BOOT_SIZE;
}

ulong get_mtk_image_search_sector_size(void)
{
	if (__rom_cfg.magic != MTK_ROM_CFG_MAGIC)
		return CONFIG_SPL_ALIGN_TO;

	return __rom_cfg.align;
}
#endif

struct mtd_info *get_mtk_board_nand_mtd(void)
{
#ifdef CONFIG_ENABLE_NAND_NMBM
	struct mtd_info *mtd;

	if (nmbm_usable) {
		mtd = get_mtd_device_nm("nmbm0");
		if (IS_ERR(mtd))
			return NULL;
		return mtd;
	}
#endif

	return get_nand_dev_by_index(0);
}

#ifndef CONFIG_SPL_BUILD
void *mtk_board_get_flash_dev(void)
{
	return get_mtk_board_nand_mtd();
}

size_t mtk_board_get_flash_erase_size(void *flashdev)
{
	struct mtd_info *mtd = (struct mtd_info *)flashdev;

	return mtd->erasesize;
}

int mtk_board_flash_erase(void *flashdev, uint64_t offset, uint64_t len)
{
	struct mtd_info *mtd = (struct mtd_info *)flashdev;
	struct erase_info instr;
	int ret;

	memset(&instr, 0, sizeof(instr));

	instr.addr = offset;
	instr.len = len;

	ret = mtd_erase(mtd, &instr);
	if (ret)
		return ret;

	if (instr.state != MTD_ERASE_DONE)
		return -1;

	return 0;
}

int mtk_board_flash_read(void *flashdev, uint64_t offset, size_t len,
			 void *buf)
{
	struct mtd_info *mtd = (struct mtd_info *)flashdev;
	size_t retlen;
	int ret;

	ret = mtd_read(mtd, offset, len, &retlen, buf);
	if (ret && ret != -EUCLEAN)
		return ret;

	if (retlen != len)
		return -EIO;

	return 0;
}

int mtk_board_flash_write(void *flashdev, uint64_t offset, size_t len,
			  const void *buf)
{
	struct mtd_info *mtd = (struct mtd_info *)flashdev;
	size_t retlen;

	return mtd_write(mtd, offset, len, &retlen, buf);
}
#endif
