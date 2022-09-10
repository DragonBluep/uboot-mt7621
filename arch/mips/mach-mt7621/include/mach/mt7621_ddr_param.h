/* SPDX-License-Identifier:	GPL-2.0+ */
/*
 * Copyright (C) 2018 MediaTek Incorporation. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#ifndef _MACH_MT7621_DDR_PARAM_H_
#define _MACH_MT7621_DDR_PARAM_H_

typedef struct ddr_ac_timing_param {
	u32 drvctl0;
	u32 drvctl1;
	u32 __unused1;
	u32 actim0;
	u32 gddr3ctl1;
	u32 conf1;
	u32 ddr2ctl;
	u32 test2_3;

	u32 conf2;
	u32 pd_ctrl;
	u32 padctl3;
	u32 __unused2;
	u32 __unused3;
	u32 __unused4;
	u32 actim1;
	u32 misctl0;

	u32 mrs1;
	u32 mrs2;
	u32 mrs3;
	u32 mrs4;
	u32 padctl1;
	u32 tdsel1;
	u32 __unused5;
	u32 __unused6;
} ddr_ac_timing_param_t;

typedef enum ddr2_act_type {
	DDR2_512M = 0,
	DDR2_W9751G6KB_A02_1066_512M,
	DDR2_1024M,
	DDR2_W971GG6KB25_800_1024M,
	DDR2_W971GG6KB18_1066_1024M
} ddr2_act_t;

typedef enum ddr3_act_type {
	DDR3_1024M = 0,
	DDR3_1024M_KGD,
	DDR3_2048M,
	DDR3_4096M
} ddr3_act_t;

typedef enum ddr_init_type {
	DDR_INIT_AUTO_PROBE = 0,
	DDR_INIT_PRE_DEFINED_FIRST,
	DDR_INIT_BOARD_PROVIDED_FIRST
} ddr_init_type_t;

typedef enum ddr_type {
	MT7621_DDR3 = 0,
	MT7621_DDR2 = 1
} ddr_type_t;

typedef struct mpll_ddr_config {
	u32 xtal_freq;

	ddr_type_t dram_type;

	int mpll_in_lbk;
	int dram_speed;
	int cpu_speed;
	int mpll_3pll;
	int dram_speed_tune;

	ddr_init_type_t init_type;

	const ddr_ac_timing_param_t *board_provided_ddr2_param;
	u32 board_expected_ddr2_size;

	const ddr_ac_timing_param_t *board_provided_ddr3_param;
	u32 board_expected_ddr3_size;

	ddr2_act_t pre_defined_ddr2_param;
	ddr3_act_t pre_defined_ddr3_param;

	u32 actual_memsize;

	int auto_probe;
} mpll_ddr_config_t;

#endif /* _MACH_MT7621_DDR_PARAM_H_ */
