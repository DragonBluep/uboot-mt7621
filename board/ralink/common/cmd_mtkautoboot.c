// SPDX-License-Identifier:	GPL-2.0+
/*
 * Copyright (C) 2018 MediaTek Incorporation. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <common.h>
#include <asm-generic/gpio.h>

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
	}
};

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
	for (i = 0; i < 5; i++) {
		if (gpio_get_value(MT7621_BUTTON_RESET) != 0)
			break;

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

	if (i == 5) {
		printf("Enter web failsafe mode by pressing reset button\n");
		run_command("httpd", 0);
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
