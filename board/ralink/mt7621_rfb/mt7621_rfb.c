// SPDX-License-Identifier:	GPL-2.0+
/*
 * Copyright (C) 2018 MediaTek Incorporation. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <common.h>
#include <environment.h>
#include <led.h>
#include <linux/types.h>
#include <debug_uart.h>
#include <asm/spl.h>
#include <asm/io.h>
#include <asm-generic/gpio.h>
#include <mach/mt7621_regs.h>
#include <spi.h>
#include <spi_flash.h>
#include <dm.h>
#include <dm/device-internal.h>

static inline int gpio_output_init(unsigned gpio, int value, const char *label)
{
	int ret;

	ret = gpio_request(gpio, label);
	if (ret && ret != -EBUSY) {
		printf("gpio: requesting pin %u failed\n", gpio);
		return -1;
	}

	gpio_direction_output(gpio, value);

	return 0;
}

static inline int gpio_input_init(unsigned gpio, const char *label)
{
	int ret;

	ret = gpio_request(gpio, label);
	if (ret && ret != -EBUSY) {
		printf("gpio: requesting pin %u failed\n", gpio);
		return -1;
	}

	gpio_direction_input(gpio);

	return 0;
}

/* Set unused pins to gpio input mode */
static int pins_unused_init(const int *pinlist, unsigned listsize)
{
	int cnt;
	char label[10] = "";

	for (cnt = 0; cnt < listsize; cnt ++) {
		if (cnt < 0 || cnt > 48) {
			printf("unsupported GPIO: gpio#%d", pinlist[cnt]);
			return -1;
		}

		strcpy(label, "gpio#");
		strcat(label, simple_itoa(pinlist[cnt]));
		gpio_input_init(pinlist[cnt], label);
	}

	return 0;
}

#ifdef CONFIG_LAST_STAGE_INIT
int last_stage_init(void)
{
#if defined(CONFIG_BOARD_MT7621_NOR_TEMPLATE)
	#define MT7621_UNUSED_PIN_LIST {}
#ifdef MT7621_LED_STATUS1
	gpio_output_init(MT7621_LED_STATUS1, 0, "led-status");	// led status on
#endif
#ifdef MT7621_BUTTON_RESET
	gpio_input_init(MT7621_BUTTON_RESET, "button-reset");	// init button reset
#endif
#elif defined(CONFIG_BOARD_ASUS_RTAC1200GU)
	#define MT7621_UNUSED_PIN_LIST {0, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, \
		13, 14, 15, 16, 17, 18, 42, 44, 45}
	// gpio_output_init(48, 0, "led-power");	// led power on
	// gpio_output_init(47, 1, "led-usb");	// led usb off
	gpio_input_init(41, "button-reset");	// init button reset
	gpio_input_init(43, "button-wps");	// init button wps
	gpio_output_init(46, 0, "gpio-ledpwr");	// turn on led power
	gpio_output_init(19, 0, "gpio-perst");	// reset pcie
#elif defined(CONFIG_BOARD_DLINK_DIR878A1)
	#define MT7621_UNUSED_PIN_LIST {0, 5, 6, 9, 10, 11, 12, 13, 14, 17, \
		41, 42, 43, 44, 45, 46, 47, 48}
	// gpio_output_init(4, 1, "led-internet_amber");	// led internet amber off
	// gpio_output_init(3, 1, "led-internet_green");	// led internet green off
	// gpio_output_init(8, 1, "led-power_amber");	// led power amber off
	// gpio_output_init(16, 0, "led-power_green");	// led power green on
	gpio_input_init(15, "button-reset");	// init button reset
	gpio_input_init(7, "button-wifi");	// init button wifi
	gpio_input_init(18, "button-wps");	// init button wps
	gpio_output_init(19, 0, "gpio-perst");	// reset pcie
#elif defined(CONFIG_BOARD_LINKSURE_SG5)
	#define MT7621_UNUSED_PIN_LIST {0, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, \
		13, 14, 15, 16, 17, 18, 22, 26, 28, 29, 30, 31, 32, 33, 41, \
		42, 43, 44, 45, 46, 47, 48}
	// gpio_output_init(27, 0, "led-status");	// led system on
	// gpio_output_init(25, 1, "led-wlan0");	// led wlan0 off
	// gpio_output_init(24, 1, "led-wlan1");	// led wlan1 off
	gpio_input_init(23, "button-reset");	// init button reset
	gpio_output_init(19, 0, "gpio-perst");	// reset pcie
#elif defined(CONFIG_BOARD_MERCURY_MAC2600R)
	#define MT7621_UNUSED_PIN_LIST {0, 3, 4, 5, 6, 7, 9, 10, 11, 12, \
		13, 14, 15, 16, 17, 18, 22, 23, 24, 26, 27, 28, 29, 30, 31, \
		32, 33, 41, 42, 43, 44, 45, 46, 47, 48}
	// gpio_output_init(25, 0, "led-status");	// led status on
	gpio_input_init(8, "button-reset");	// init button reset
	gpio_output_init(19, 0, "gpio-perst");	// reset pcie
#elif defined(CONFIG_BOARD_SKSPRUCE_WIA330010)
	#define MT7621_UNUSED_PIN_LIST {0, 3, 5, 6, 7, 8, 9, 10, 11, 12, \
		13, 14, 15, 16, 17, 25, 26, 27, 28, 29, 30, 31, 32, 33, 41, \
		42, 43, 44, 45, 46, 47, 48}
	// gpio_output_init(24, 0, "led-status_green");	// led status green on
	// gpio_output_init(23, 1, "led-status_red");	// led status red off
	// gpio_output_init(22, 1, "led-wlan_blue");	// led wlan blue off
	// gpio_output_init(4, 1, "led-wlan_green");	// led wlan green off
	gpio_input_init(18, "button-reset");	// init button reset
	gpio_output_init(19, 0, "gpio-perst");	// reset pcie
#else
	#define MT7621_UNUSED_PIN_LIST {}
#endif

	int pinlist[] = MT7621_UNUSED_PIN_LIST;

	pins_unused_init(pinlist, sizeof(pinlist)/sizeof(int));

#ifndef CONFIG_BOARD_MT7621_NOR_TEMPLATE
	/* configure the default state (auto-probe) */
	led_default_state();
#endif // CONFIG_BOARD_MT7621_NOR_TEMPLATE

	/* Sort variables alphabetically if the variable "_sortenv" exists */
	if (env_get("_sortenv") != NULL) {
		printf("Sort environment variables alphabetically\n");
		env_set("_sortenv", "");
		env_save();
	}

	return 0;
}
#endif // CONFIG_LAST_STAGE_INIT

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
