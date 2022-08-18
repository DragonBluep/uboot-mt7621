// SPDX-License-Identifier:	GPL-2.0+
/*
 * Copyright (C) 2018 MediaTek Incorporation. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <common.h>
#include <asm/io.h>
#include <asm/addrspace.h>
#include <asm/mipsmtregs.h>
#include <asm/cm.h>
#include <asm/sections.h>
#include <mach/mt7621_regs.h>
#include "launch.h"

/* Cluster Power Controller (CPC) offsets */
#define CPC_CL_OTHER			0x2010
#define CPC_CO_CMD			0x4000

/* CPC_CL_OTHER fields */
#define CPC_CL_OTHER_CORENUM_SHIFT	16
#define CPC_CL_OTHER_CORENUM		GENMASK(23, 16)

/* CPC_CO_CMD */
#define PWR_UP				3


#define NUM_CPUS			4
#define WAIT_CPUS_TIMEOUT		4000

static void __maybe_unused copy_launch_wait_code(void)
{
	memset((void *)KSEG1, 0, 0x100);
	memset((void *)KSEG0, 0, 0x100);

	memcpy((void *)CMP_LAUNCH_WAITCODE_IN_RAM,
		&launch_wait_code_start,
		&launch_wait_code_end - &launch_wait_code_start);
}

static void __maybe_unused bootup_secondary_core(void)
{
	void __iomem *base;
	unsigned long num_cores;
	int i;

	num_cores = mips_cm_numcores();

	base = (void __iomem *) CKSEG1ADDR(MIPS_CPC_BASE);

	for (i = 1; i < num_cores; i++) {
		writel(i << CPC_CL_OTHER_CORENUM_SHIFT, base + CPC_CL_OTHER);
		writel(PWR_UP, base + CPC_CO_CMD);
	}
}

#ifndef CONFIG_SPL
void cpu_secondary_init_r(void)
#else
void cpu_secondary_init(void)
#endif
{
#if !defined(CONFIG_MT7621_SINGLE_CORE) || !defined(CONFIG_MT7621_SINGLE_VPE)
	void __iomem *base;
	u32  dual_core = 0;
	cpulaunch_t *c;
	int i, cpuready = 1, cpumask;
	ulong wait_tick;

	/* Copy LAUNCH wait code used for other VPEs */
	copy_launch_wait_code();

	/* Bootup other core(s) */
	base = (void __iomem *) CKSEG1ADDR(MT7621_SYSCTL_BASE);
#ifndef CONFIG_MT7621_SINGLE_CORE
	dual_core = REG_GET_VAL(CPU_ID,
		readl(base + MT7621_SYS_CHIP_REV_ID_REG));
#endif

	if (dual_core) {
#ifdef CONFIG_MT7621_SINGLE_VPE
		cpumask = 0x05;
#else
		cpumask = 0x0f;
#endif /* CONFIG_MT7621_SINGLE_VPE */
	} else {
		cpumask = 0x03;
	}

	/* Make BootROM redirect Core1's bootup flow to our entry point */
	writel((ulong) &_start, base + MT7621_BOOT_SRAM_BASE_REG);

	/* Only bootup secondary core for MT7621A */
	if (dual_core)
		bootup_secondary_core();

	/* Join the coherent domain */
	join_coherent_domain(dual_core ? 2 : 1);

#ifndef CONFIG_MT7621_SINGLE_VPE
	/* Bootup VPE1 */
	boot_vpe1();
#endif

	/* Wait for all CPU ready */
	wait_tick = get_timer(0) + WAIT_CPUS_TIMEOUT;

	while (time_before(get_timer(0), wait_tick)) {
		/* CPU0 is obviously ready */
		for (i = 1; i < NUM_CPUS; i++) {
			c = (cpulaunch_t *)(CKSEG0ADDR(CPULAUNCH) +
					    (i << LOG2CPULAUNCH));

			if (c->flags & LAUNCH_FREADY)
				cpuready |= BIT(i);
		}

		if ((cpuready & cpumask) == cpumask)
			break;
	}
#endif /* !CONFIG_MT7621_SINGLE_CORE || !CONFIG_MT7621_SINGLE_VPE */
}