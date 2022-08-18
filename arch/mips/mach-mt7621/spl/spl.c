// SPDX-License-Identifier:	GPL-2.0+
/*
 * Copyright (C) 2018 MediaTek Incorporation. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <common.h>
#include <spl.h>

DECLARE_GLOBAL_DATA_PTR;

void __noreturn board_init_f(ulong dummy)
{
	gd->malloc_base = CONFIG_SYS_SDRAM_BASE + get_effective_memsize() - \
			  CONFIG_VAL(SYS_MALLOC_F_LEN);

	preloader_console_init();

	board_init_r(NULL, 0);
}