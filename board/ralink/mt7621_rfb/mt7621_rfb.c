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
#include <spi.h>
#include <spi_flash.h>
#include <dm.h>
#include <dm/device-internal.h>

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

#ifndef CONFIG_SPL_BUILD
void *mtk_board_get_flash_dev(void)
{
	unsigned int bus = CONFIG_SF_DEFAULT_BUS;
	unsigned int cs = CONFIG_SF_DEFAULT_CS;
	unsigned int speed = CONFIG_SF_DEFAULT_SPEED;
	unsigned int mode = CONFIG_SF_DEFAULT_MODE;
	struct spi_flash *flash = NULL;
	struct udevice *new, *bus_dev;
	int ret;

	/* In DM mode defaults will be taken from DT */
	speed = 0, mode = 0;

	/* Remove the old device, otherwise probe will just be a nop */
	ret = spi_find_bus_and_cs(bus, cs, &bus_dev, &new);
	if (!ret)
		device_remove(new, DM_REMOVE_NORMAL);

	ret = spi_flash_probe_bus_cs(bus, cs, speed, mode, &new);
	if (ret) {
		printf("Failed to initialize SPI flash at %u:%u (error %d)\n",
			bus, cs, ret);
		return NULL;
	}

	flash = dev_get_uclass_priv(new);

	return flash;
}

size_t mtk_board_get_flash_erase_size(void *flashdev)
{
	struct spi_flash *flash = (struct spi_flash *)flashdev;

	return flash->erase_size;
}

int mtk_board_flash_erase(void *flashdev, uint64_t offset, uint64_t len)
{
	struct spi_flash *flash = (struct spi_flash *)flashdev;

	return spi_flash_erase(flash, offset, len);
}

int mtk_board_flash_read(void *flashdev, uint64_t offset, size_t len,
			 void *buf)
{
	struct spi_flash *flash = (struct spi_flash *)flashdev;

	return spi_flash_read(flash, offset, len, buf);
}

int mtk_board_flash_write(void *flashdev, uint64_t offset, size_t len,
			  const void *buf)
{
	struct spi_flash *flash = (struct spi_flash *)flashdev;

	return spi_flash_write(flash, offset, len, buf);
}
#endif
