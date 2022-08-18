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

#include "../common/compat_bbt_mtd.h"

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

#ifdef CONFIG_LAST_STAGE_INIT
int last_stage_init(void)
{
#ifdef CONFIG_COMPAT_NAND_BBT
	mt7621_nand_bbt_compat_create("nand0");
#endif

	return 0;
}
#endif

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
#ifdef CONFIG_COMPAT_NAND_BBT
	return get_mtd_device_nm("mnbc0");
#else
	return get_nand_dev_by_index(0);
#endif
}