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

DECLARE_GLOBAL_DATA_PTR;

extern u32 mempll_get_clock(void);

int get_serial_clock(void)
{
	return 50000000;
}

void mt7621_get_clocks(u32 *pcpu_clk, u32 *pbus_clk, u32 *pddr_clk, u32 *pxtal_clk)
{
	void __iomem *sys_base, __iomem *dramc_base;
	u32 bs, xtal_sel, clkcfg0, cur_clk, mempll, dividx, fb;
	u32 xtal_clk, xtal_div, ffiv, ffrac, cpu_clk, ddr_clk;
	const static u32 xtal_div_tbl[] = {0, 1, 2, 2};

	sys_base = (void __iomem *) CKSEG1ADDR(MT7621_SYSCTL_BASE);
	dramc_base = (void __iomem *) CKSEG1ADDR(MT7621_DRAMC_BASE);

	bs = readl(sys_base + MT7621_SYS_SYSCFG0_REG);
	clkcfg0 = readl(sys_base + MT7621_SYS_CLKCFG0_REG);
	cur_clk = readl(sys_base + MT7621_SYS_CUR_CLK_STS_REG);

	xtal_sel = REG_GET_VAL(XTAL_MODE_SEL, bs);

	if (xtal_sel <= 2)
		xtal_clk = 20 * 1000 * 1000;
	else if (xtal_sel <= 5)
		xtal_clk = 40 * 1000 * 1000;
	else
		xtal_clk = 25 * 1000 * 1000;

	switch (REG_GET_VAL(CPU_CLK_SEL, clkcfg0)) {
	case 0:
		cpu_clk = 500 * 1000 * 1000;
		break;
	case 1:
		mempll = readl(dramc_base + MT7621_DRAMC_MEMPLL18_REG);
		dividx = REG_GET_VAL(RG_MEPL_PREDIV, mempll);
		fb = REG_GET_VAL(RG_MEPL_FBDIV, mempll);
		xtal_div = 1 << xtal_div_tbl[dividx];
		cpu_clk = (fb + 1) * xtal_clk / xtal_div;
		break;
	default:
		cpu_clk = xtal_clk;
	}

	ffiv = REG_GET_VAL(CUR_CPU_FDIV, cur_clk);
	ffrac = REG_GET_VAL(CUR_CPU_FFRAC, cur_clk);
	cpu_clk = cpu_clk / ffiv * ffrac;

	mempll = readl(dramc_base + MT7621_DRAMC_MEMPLL6_REG);
	dividx = REG_GET_VAL(RG_MEPL_PREDIV, mempll);
	fb = REG_GET_VAL(RG_MEPL_FBDIV, mempll);
	xtal_div = 1 << xtal_div_tbl[dividx];
	ddr_clk = fb * xtal_clk / xtal_div;

	bs = readl(dramc_base + MT7621_DRAMC_MEMPLL1_REG);
	if (REG_GET_VAL(RG_MEPL_DIV2_SEL, bs) == 0)
		ddr_clk *= 2;

	if (pcpu_clk)
		*pcpu_clk = cpu_clk;

	if (pbus_clk)
		*pbus_clk = cpu_clk / 4;

	if (pddr_clk)
		*pddr_clk = ddr_clk;

	if (pxtal_clk)
		*pxtal_clk = xtal_clk;
}

int get_clocks(void)
{
	mt7621_get_clocks((u32 *)&gd->cpu_clk, (u32 *)&gd->bus_clk,
			  (u32 *)&gd->mem_clk, (u32 *)&gd->arch.xtal_clk);

	return 0;
}

ulong get_cpu_freq(ulong dummy)
{
	if (!gd->cpu_clk)
		get_clocks();
	return gd->cpu_clk;
}

ulong get_bus_freq(ulong dummy)
{
	if (!gd->bus_clk)
		get_clocks();
	return gd->bus_clk;
}

ulong get_ddr_freq(ulong dummy)
{
	if (!gd->mem_clk)
		get_clocks();
	return gd->mem_clk;
}

ulong notrace get_tbclk(void)
{
	return get_cpu_freq(0) / 2;
}