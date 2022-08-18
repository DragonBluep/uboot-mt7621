/* SPDX-License-Identifier:	GPL-2.0+ */
/*
 * Copyright (C) 2018 MediaTek Incorporation. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#ifndef _MACH_MT7621_LOWLEVEL_INIT_H_
#define _MACH_MT7621_LOWLEVEL_INIT_H_

#include <mach/mt7621_ddr_param.h>

extern int mt7621_dramc_init(mpll_ddr_config_t *pcfg);

extern void __weak board_get_ddr2_init_param(
	const ddr_ac_timing_param_t **, u32 *);
extern void __weak board_get_ddr3_init_param(
	const ddr_ac_timing_param_t **, u32 *);

extern void _machine_restart_spl(void);

#endif /* _MACH_MT7621_LOWLEVEL_INIT_H_ */
