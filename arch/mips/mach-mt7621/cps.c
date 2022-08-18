/* SPDX-License-Identifier:	GPL-2.0+ */
/*
 * Copyright (C) 2018 MediaTek Incorporation. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <common.h>
#include <asm/io.h>
#include <asm/addrspace.h>
#include <asm/mipsregs.h>
#include <mach/mt7621_regs.h>
#include <asm/cm.h>

/* GCR_REGx_BASE */
#define GCR_REG0_BASE_VALUE	0x1c000000
#define GCR_REG1_BASE_VALUE	0x60000000
#define GCR_REG2_BASE_VALUE	0x1c000000
#define GCR_REG3_BASE_VALUE	0x1c000000

/* GCR_REGx_MASK */
#define GCR_REG0_MASK_VALUE	0x0000fc00	/* 64M Palmbus */
#define GCR_REG1_MASK_VALUE	0x0000f000	/* 256M PCIe Memory Access */
#define GCR_REG2_MASK_VALUE	0x0000fc00	/* 64M */
#define GCR_REG3_MASK_VALUE	0x0000fc00	/* 64M */

/* GIC Shared Register Bases */
#define GIC_SH_POL_BASE		0x100
#define GIC_SH_TRIG_BASE	0x180
#define GIC_SH_RMASK_BASE	0x300
#define GIC_SH_SMASK_BASE	0x380
#define GIC_SH_MASK_BASE	0x400
#define GIC_SH_PEND_BASE	0x480
#define GIC_SH_MAP_PIN_BASE	0x500
#define GIC_SH_MAP_VPE_BASE	0x2000

/* GIC Registers */
#define GIC_SH_CONFIG		0

#define GIC_SH_POL31_0		(GIC_SH_POL_BASE + 0x00)
#define GIC_SH_POL63_32		(GIC_SH_POL_BASE + 0x04)

#define GIC_SH_TRIG31_0		(GIC_SH_TRIG_BASE + 0x00)
#define GIC_SH_TRIG63_32	(GIC_SH_TRIG_BASE + 0x04)

#define GIC_SH_RMASK31_0	(GIC_SH_RMASK_BASE + 0x00)
#define GIC_SH_RMASK63_32	(GIC_SH_RMASK_BASE + 0x04)

#define GIC_SH_SMASK31_0	(GIC_SH_SMASK_BASE + 0x00)
#define GIC_SH_SMASK63_32	(GIC_SH_SMASK_BASE + 0x04)

#define GIC_SH_MAP_PIN(n)	(GIC_SH_MAP_PIN_BASE + (n) * 4)

#define GIC_SH_MAP_VPE(n, v)	(GIC_SH_MAP_VPE_BASE + (n) * 0x20 + ((v) / 32) * 4)
#define GIC_SH_MAP_VPE31_0(n)	GIC_SH_MAP_VPE(n, 0)

/* GIC_SH_CONFIG fields */
#define NUMINTERRUPTS_MASK	GENMASK(23, 16)
#define NUMINTERRUPTS_SHIFT	16

/* GIC_SH_MAP_PIN fields */
#define GIC_MAP_TO_PIN		BIT(31)
#define GIC_MAP_TO_NMI		BIT(30)
#define GIC_MAP			GENMASK(5, 0)
#define GIC_MAP_SHIFT		0

static void cm_init(void)
{
	u32 num_cores;
	u32 val;

	num_cores = mips_cm_numcores();

	writel((1 << num_cores) - 1, mips_cm_base() + GCR_ACCESS);

	if (!mips_cm_numiocu())
		return;

	writel(GCR_REG0_BASE_VALUE, mips_cm_base() + GCR_REG0_BASE);
	writel(GCR_REG1_BASE_VALUE, mips_cm_base() + GCR_REG1_BASE);
	writel(GCR_REG2_BASE_VALUE, mips_cm_base() + GCR_REG2_BASE);
	writel(GCR_REG3_BASE_VALUE, mips_cm_base() + GCR_REG3_BASE);

	val = readl(mips_cm_base() + GCR_REG0_MASK);
	val &= ~(GCR_REGn_MASK_ADDRMASK | GCR_REGn_MASK_CMTGT);
	val |= (GCR_REG0_MASK_VALUE << GCR_REGn_MASK_ADDRMASK_SHIFT) |
		GCR_REGn_MASK_CMTGT_IOCU0;
	writel(val, mips_cm_base() + GCR_REG0_MASK);

	val = readl(mips_cm_base() + GCR_REG1_MASK);
	val &= ~(GCR_REGn_MASK_ADDRMASK | GCR_REGn_MASK_CMTGT);
	val |= (GCR_REG1_MASK_VALUE << GCR_REGn_MASK_ADDRMASK_SHIFT) |
		GCR_REGn_MASK_CMTGT_IOCU0;
	writel(val, mips_cm_base() + GCR_REG1_MASK);

	val = readl(mips_cm_base() + GCR_REG2_MASK);
	val &= ~(GCR_REGn_MASK_ADDRMASK | GCR_REGn_MASK_CMTGT);
	val |= (GCR_REG2_MASK_VALUE << GCR_REGn_MASK_ADDRMASK_SHIFT) |
		GCR_REGn_MASK_CMTGT_IOCU0;
	writel(val, mips_cm_base() + GCR_REG2_MASK);

	val = readl(mips_cm_base() + GCR_REG3_MASK);
	val &= ~(GCR_REGn_MASK_ADDRMASK | GCR_REGn_MASK_CMTGT);
	val |= (GCR_REG3_MASK_VALUE << GCR_REGn_MASK_ADDRMASK_SHIFT) |
		GCR_REGn_MASK_CMTGT_IOCU0;
	writel(val, mips_cm_base() + GCR_REG3_MASK);

	val = readl(mips_cm_base() + GCR_BASE);
	val &= ~CM_DEFAULT_TARGET_MASK;
	writel(val, mips_cm_base() + GCR_BASE);

	val = readl(mips_cm_base() + 0x10);
	val |= 0x10000;
	writel(val, mips_cm_base() + 0x10);
}

static void cpc_init(void)
{
	if (readl(mips_cm_base() + GCR_CPC_STATUS) & GCR_CPC_EX)
		writel(MIPS_CPC_BASE | GCR_CPC_EN,
			     mips_cm_base() + GCR_CPC_BASE);
}

static void *mips_gic_base(void)
{
	return (void *)CKSEG1ADDR(MIPS_GIC_BASE);
}

static inline u32 mips_gic_numinterrupts(void)
{
	return ((readl(mips_gic_base() + GIC_SH_CONFIG) & NUMINTERRUPTS_MASK)
		>> NUMINTERRUPTS_SHIFT) + 1;
}

static void gic_init(void)
{
	int i;

	if (!(readl(mips_cm_base() + GCR_GIC_STATUS) & GCR_GIC_EX))
		return;

	writel(MIPS_GIC_BASE | GCR_GIC_EN, mips_cm_base() + GCR_GIC_BASE);

	if (mips_gic_numinterrupts() != 8)
		return;

	/* Interrupt 0..5: Level Trigger, Active High */
	writel(0, mips_gic_base() + GIC_SH_TRIG31_0);
	writel(0x3f, mips_gic_base() + GIC_SH_RMASK31_0);
	writel(0x3f, mips_gic_base() + GIC_SH_POL31_0);
	writel(0x3f, mips_gic_base() + GIC_SH_SMASK31_0);

	/* Interrupt 56..63: Edge Trigger, Rising Edge */
	/* Hardcoded to set up the last 8 external interrupts for IPI. */
	writel(0xff000000, mips_gic_base() + GIC_SH_TRIG63_32);
	writel(0xff000000, mips_gic_base() + GIC_SH_RMASK63_32);
	writel(0xff000000, mips_gic_base() + GIC_SH_POL63_32);
	writel(0xff000000, mips_gic_base() + GIC_SH_SMASK63_32);

	/* Map interrupt source to particular pin */
	/* source {0,1,2,3,4,5} -> pin {0,0,4,3,0,5} */
	writel(GIC_MAP_TO_PIN | 0, mips_gic_base() + GIC_SH_MAP_PIN(0));
	writel(GIC_MAP_TO_PIN | 0, mips_gic_base() + GIC_SH_MAP_PIN(1));
	writel(GIC_MAP_TO_PIN | 4, mips_gic_base() + GIC_SH_MAP_PIN(2));
	writel(GIC_MAP_TO_PIN | 3, mips_gic_base() + GIC_SH_MAP_PIN(3));
	writel(GIC_MAP_TO_PIN | 0, mips_gic_base() + GIC_SH_MAP_PIN(4));
	writel(GIC_MAP_TO_PIN | 5, mips_gic_base() + GIC_SH_MAP_PIN(5));

	/* source 56~59 -> pin 1, 60~63 -> pin 2 */
	writel(GIC_MAP_TO_PIN | 1, mips_gic_base() + GIC_SH_MAP_PIN(56));
	writel(GIC_MAP_TO_PIN | 1, mips_gic_base() + GIC_SH_MAP_PIN(57));
	writel(GIC_MAP_TO_PIN | 1, mips_gic_base() + GIC_SH_MAP_PIN(58));
	writel(GIC_MAP_TO_PIN | 1, mips_gic_base() + GIC_SH_MAP_PIN(59));
	writel(GIC_MAP_TO_PIN | 2, mips_gic_base() + GIC_SH_MAP_PIN(60));
	writel(GIC_MAP_TO_PIN | 2, mips_gic_base() + GIC_SH_MAP_PIN(61));
	writel(GIC_MAP_TO_PIN | 2, mips_gic_base() + GIC_SH_MAP_PIN(62));
	writel(GIC_MAP_TO_PIN | 2, mips_gic_base() + GIC_SH_MAP_PIN(63));

	/* Interrupt map to VPE (bit mask) */
	for (i = 0; i < 32; i++)
		writel(BIT(0), mips_gic_base() + GIC_SH_MAP_VPE31_0(i));

	/*
	 * Direct GIC_int 56..63 to vpe 0..3
	 * MIPS Linux convention that last 16 interrupts implemented be set
	 * aside for IPI signaling.
	 * The actual interrupts are tied low and software sends interrupts
	 * via GIC_SH_WEDGE writes.
	 */
	for (i = 0; i < 4; i++) {
		writel(BIT(i), mips_gic_base() + GIC_SH_MAP_VPE31_0(i + 56));
		writel(BIT(i), mips_gic_base() + GIC_SH_MAP_VPE31_0(i + 60));
	}
}

void mt7621_cps_init(void)
{
	gic_init();
	cpc_init();
	cm_init();
}