// SPDX-License-Identifier:	GPL-2.0+
/*
 * Copyright (C) 2018 MediaTek Incorporation. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <common.h>
#include <linux/types.h>
#include <debug_uart.h>
#include <asm/spl.h>
#include <asm/io.h>
#include <mach/mt7621_regs.h>

#ifdef CONFIG_DEBUG_UART_BOARD_INIT
void board_debug_uart_init(void)
{
#if !defined(CONFIG_SPL) || \
    (defined(CONFIG_SPL_BUILD) && !defined(CONFIG_TPL_BUILD))
	void __iomem *base = (void __iomem *) CKSEG1ADDR(MT7621_SYSCTL_BASE);

	setbits_le32(base + MT7621_SYS_RSTCTL_REG, REG_SET_VAL(UART1_RST, 1));
	clrbits_le32(base + MT7621_SYS_RSTCTL_REG, REG_MASK(UART1_RST));
	clrbits_le32(base + MT7621_SYS_GPIO_MODE_REG, REG_MASK(UART1_MODE));
#endif
}
#endif

#ifdef CONFIG_SPL_BUILD
void board_boot_order(u32 *spl_boot_list)
{
	spl_boot_list[0] = BOOT_DEVICE_MTK_NOR;
	spl_boot_list[1] = BOOT_DEVICE_MTK_UART;
}

ulong get_mtk_image_search_start(void)
{
#ifdef CONFIG_TPL
	struct mtk_spl_rom_cfg *rom_cfg;

	rom_cfg = (struct mtk_spl_rom_cfg *)
		  (CONFIG_SPI_ADDR + MTK_SPL_ROM_CFG_OFFS);

	if (rom_cfg->magic != MTK_ROM_CFG_MAGIC)
		return CONFIG_SPI_ADDR + CONFIG_SPL_ALIGN_TO;

	return CONFIG_SPI_ADDR + rom_cfg->size;
#else
	if (__rom_cfg.magic != MTK_ROM_CFG_MAGIC)
		return CONFIG_SPL_ALIGN_TO;

	return CONFIG_SPI_ADDR + __rom_cfg.size;
#endif
}

ulong get_mtk_image_search_end(void)
{
	return CONFIG_SPI_ADDR + CONFIG_MAX_U_BOOT_SIZE;
}

ulong get_mtk_image_search_sector_size(void)
{
#ifdef CONFIG_TPL
	struct mtk_spl_rom_cfg *rom_cfg;

	rom_cfg = (struct mtk_spl_rom_cfg *)
		  (CONFIG_SPI_ADDR + MTK_SPL_ROM_CFG_OFFS);

	if (rom_cfg->magic != MTK_ROM_CFG_MAGIC)
		return CONFIG_SPL_ALIGN_TO;

	return rom_cfg->align;
#else
	if (__rom_cfg.magic != MTK_ROM_CFG_MAGIC)
		return CONFIG_SPL_ALIGN_TO;

	return __rom_cfg.align;
#endif
}
#endif