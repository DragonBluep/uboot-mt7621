// SPDX-License-Identifier:	GPL-2.0+
/*
 * Copyright (C) 2018 MediaTek Incorporation. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <common.h>
#include <asm/io.h>
#include <asm/addrspace.h>
#include <asm/types.h>
#include <mach/mt7621_regs.h>

#include "clocks.h"

int print_cpuinfo(void)
{
	void __iomem *base;
	u32 revid, core, pkg, ver, eco;
	u32 cpu_clk, ddr_clk, bus_clk, xtal_clk;

	base = (void __iomem *) CKSEG1ADDR(MT7621_SYSCTL_BASE);

	revid = readl(base + MT7621_SYS_CHIP_REV_ID_REG);

	core = REG_GET_VAL(CPU_ID, revid);
	pkg = REG_GET_VAL(PKG_ID, revid);
	ver = REG_GET_VAL(VER_ID, revid);
	eco = REG_GET_VAL(ECO_ID, revid);

	printf("CPU:   MediaTek MT7621%cT ver %u, eco %u\n",
	       core ? (pkg ? 'A' : 'N') : 'S', ver, eco);

	mt7621_get_clocks(&cpu_clk, &bus_clk, &ddr_clk, &xtal_clk);

	printf("Clocks: CPU: %dMHz, DDR: %dMHz (%dMT/s), Bus: %dMHz, XTAL: %dMHz\n",
		cpu_clk / 1000000, ddr_clk / 1000000, ddr_clk / 500000,
		bus_clk / 1000000, xtal_clk / 1000000);

	return 0;
}