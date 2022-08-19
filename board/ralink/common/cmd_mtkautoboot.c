// SPDX-License-Identifier:	GPL-2.0+
/*
 * Copyright (C) 2018 MediaTek Incorporation. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <common.h>

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
	}, {
		.desc = "System Enter UBoot to Update Img",
		.cmd = "mtkupgrade fwre"
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
