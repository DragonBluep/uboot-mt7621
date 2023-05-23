// SPDX-License-Identifier:	GPL-2.0+
/*
 * Copyright (C) 2018 MediaTek Incorporation. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <common.h>
#include <asm-generic/gpio.h>
#include <led.h>

#if defined(CONFIG_BOARD_MT7621_NAND_TEMPLATE) || \
	defined(CONFIG_BOARD_MT7621_NOR_TEMPLATE)
#define MT7621_USE_GPIO_LED
#endif

struct mtk_bootmenu_entry {
	const char *desc;
	const char *cmd;
} bootmenu_entries[] = {
	{
		.desc = "Startup system (Default)",
		.cmd = "mtkboardboot"
	}, {
		.desc = "Upgrade firmware",
		.cmd = "mtkupgrade fw"
	}, {
		.desc = "Upgrade bootloader",
		.cmd = "mtkupgrade bl"
	}, {
		.desc = "Upgrade bootloader (advanced mode)",
		.cmd = "mtkupgrade bladv"
	},{
		.desc = "Load image",
		.cmd = "mtkload"
	},{
		.desc = "Reboot",
		.cmd = "reset"
	}
};

#ifdef CONFIG_FAILSAFE_ON_BUTTON
#ifdef MT7621_USE_GPIO_LED
static inline void mtk_blinkled(void)
{
	mdelay(100);
#ifdef MT7621_LED_STATUS1
	gpio_direction_output(MT7621_LED_STATUS1, 1);
#endif // MT7621_LED_STATUS1
#ifdef MT7621_LED_STATUS2
	gpio_direction_output(MT7621_LED_STATUS2, 0);
#endif // MT7621_LED_STATUS2
	mdelay(100);
#ifdef MT7621_LED_STATUS1
	gpio_direction_output(MT7621_LED_STATUS1, 0);
#endif // MT7621_LED_STATUS1
#ifdef MT7621_LED_STATUS2
	gpio_direction_output(MT7621_LED_STATUS2, 1);
#endif // MT7621_LED_STATUS2
}
#else
/* Get led udevice by dts config property name */
static int mtk_getled(const char *prop_name, struct udevice **devp)
{
	const char *name;
	int ret;

	name = fdtdec_get_config_string(gd->fdt_blob, prop_name);
	if (!name)
		return -ENODATA;
	ret = led_get_by_label(name, devp);
	if (ret)
		debug("%s: get=%d\n", __func__, ret);
	return ret;
}

/* Blink LED */
static inline void mtk_blinkled(struct udevice *led1, int flag1,
			struct udevice *led2, int flag2)
{
	if (flag1)
		led_set_state(led1, LEDST_OFF);
	if (flag2)
		led_set_state(led2, LEDST_ON);
	mdelay(100);
	if (flag1)
		led_set_state(led1, LEDST_ON);
	if (flag2)
		led_set_state(led2, LEDST_OFF);
	mdelay(100);
}
#endif // MT7621_USE_GPIO_LED

/* Loop TFTP download mode */
static void mtktftploop(void)
{
	int cnt;
#ifndef MT7621_USE_GPIO_LED
	struct udevice *led1, *led2;
	int flag_led1, flag_led2;

	flag_led1 = !mtk_getled("status1-led", &led1);
	flag_led2 = !mtk_getled("status2-led", &led2);
#endif // MT7621_USE_GPIO_LED

	#define _tostr(a)	#a
	#define tostr(a)	_tostr(a)

	env_set("autostart", "yes");
	env_set("ipaddr", "192.168.1.1");
	env_set("serverip", "192.168.1.2");
	env_set("loadaddr", tostr(CONFIG_SYS_LOAD_ADDR));

	while(1) {
		run_command("tftpboot recovery.bin", 0);
		mdelay(2000);
		for (cnt = 0; cnt < 3; cnt++)
#ifdef MT7621_USE_GPIO_LED
			mtk_blinkled();
#else
			mtk_blinkled(led1, flag_led1, led2, flag_led2);
#endif // MT7621_USE_GPIO_LED
	}
}
#endif

static int do_mtkautoboot(cmd_tbl_t *cmdtp, int flag, int argc,
	char *const argv[])
{
	int i;
	char key[16];
	char val[256];
	char cmd[32];
	const char *delay_str;
	u32 delay = CONFIG_MTKAUTOBOOT_DELAY;

#ifdef CONFIG_FAILSAFE_ON_BUTTON
#ifndef MT7621_USE_GPIO_LED
	struct udevice *led1, *led2;
	int flag_led1, flag_led2;

	flag_led1 = !mtk_getled("status1-led", &led1);
	flag_led2 = !mtk_getled("status2-led", &led2);
#endif // MT7621_USE_GPIO_LED
#ifdef MT7621_BUTTON_WPS
	for (i = 0; i < 5; i++) {
#else
	for (i = 0; i < 30; i++) {
#endif
		if (gpio_get_value(MT7621_BUTTON_RESET) != 0)
			break;

#ifdef MT7621_USE_GPIO_LED
		mtk_blinkled();
#else
		mtk_blinkled(led1, flag_led1, led2, flag_led2);
#endif // MT7621_USE_GPIO_LED
	}

#ifdef MT7621_BUTTON_WPS
	if (i == 5) {
#else
	if (i >= 5 && i < 30) {
#endif
		printf("Enter web failsafe mode by pressing reset button\n");
		run_command("httpd", 0);
	}

#ifdef MT7621_BUTTON_WPS
	for (i = 0; i < 5; i++) {
		if (gpio_get_value(MT7621_BUTTON_WPS) != 0)
			break;

#ifdef MT7621_USE_GPIO_LED
		mtk_blinkled();
#else
		mtk_blinkled(led1, flag_led1, led2, flag_led2);
#endif // MT7621_USE_GPIO_LED
	}
	if (i == 5) {
		printf("Enter TFTP download mode by pressing WPS button\n");
#else
	if (i == 30) {
		printf("Enter TFTP download mode by pressing reset button\n");
#endif
		printf("Load image \"recovery.bin\" to RAM and then boot it\n");
		mtktftploop();
	}
#endif // CONFIG_FAILSAFE_ON_BUTTON

	for (i = 0; i < ARRAY_SIZE(bootmenu_entries); i++) {
		snprintf(key, sizeof(key), "bootmenu_%d", i);
		snprintf(val, sizeof(val), "%s=%s",
			 bootmenu_entries[i].desc, bootmenu_entries[i].cmd);
		env_set(key, val);
	}

	/*
	 * Remove possibly existed `next entry` to force bootmenu command to
	 * stop processing
	 */
	snprintf(key, sizeof(key), "bootmenu_%d", i);
	env_set(key, NULL);

	delay_str = env_get("bootmenu_delay");
	if (delay_str) {
		delay = simple_strtoul(delay_str, NULL, 10);
		if (delay == 0)
			delay = 1;
	}

	snprintf(cmd, sizeof(cmd), "bootmenu %u", delay);
	run_command(cmd, 0);

	return 0;
}

U_BOOT_CMD(mtkautoboot, 1, 0, do_mtkautoboot,
	"Display MediaTek bootmenu", ""
);
