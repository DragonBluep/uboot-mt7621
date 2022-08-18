/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * MIPS Coherence Manager (CM) Register Definitions
 *
 * Copyright (c) 2016 Imagination Technologies Ltd.
 */
#ifndef __MIPS_ASM_CM_H__
#define __MIPS_ASM_CM_H__

#ifndef __ASSEMBLY__
#include <linux/bitops.h>
#endif

/* Global Control Register (GCR) offsets */
#define GCR_CONFIG			0x0000
#define GCR_BASE			0x0008
#define GCR_BASE_UPPER			0x000c
#define GCR_ACCESS			0x0020
#define GCR_REV				0x0030
#define GCR_GIC_BASE			0x0080
#define GCR_CPC_BASE			0x0088
#define GCR_REG0_BASE			0x0090
#define GCR_REG0_MASK			0x0098
#define GCR_REG1_BASE			0x00a0
#define GCR_REG1_MASK			0x00a8
#define GCR_REG2_BASE			0x00b0
#define GCR_REG2_MASK			0x00b8
#define GCR_REG3_BASE			0x00c0
#define GCR_REG3_MASK			0x00c8
#define GCR_GIC_STATUS			0x00d0
#define GCR_CPC_STATUS			0x00f0
#define GCR_L2_CONFIG			0x0130
#define GCR_L2_TAG_ADDR			0x0600
#define GCR_L2_TAG_ADDR_UPPER		0x0604
#define GCR_L2_TAG_STATE		0x0608
#define GCR_L2_TAG_STATE_UPPER		0x060c
#define GCR_L2_DATA			0x0610
#define GCR_L2_DATA_UPPER		0x0614
#define GCR_Cx_COHERENCE		0x2008
#define GCR_CL_OTHER			0x2018
#define GCR_CL_ID			0x2028
#define GCR_CO_RESET_RELEASE		0x4000
#define GCR_CO_COHERENCE		0x4008

/* GCR_REV CM versions */
#define GCR_REV_CM3			0x0800

/* GCR_CONFIG fields */
#define GCR_CONFIG_CLUSTER_COH_CAPABLE	BIT_ULL(43)
#define GCR_CONFIG_CLUSTER_ID		GENMASK_ULL(39, 32)
#define GCR_CONFIG_NUM_CLUSTERS		GENMASK(29, 23)
#define GCR_CONFIG_NUMIOCU		GENMASK(15, 8)
#define GCR_CONFIG_NUMIOCU_SHIFT	8
#define GCR_CONFIG_PCORES		GENMASK(7, 0)
#define GCR_CONFIG_PCORES_SHIFT		0
#define GCR_CONFIG_PCORES_WIDTH		8

/* GCR_BASE fields */
#define GCR_BASE_SHIFT				15
#define GCR_BASE_BITS				17
#define CCA_DEFAULT_OVERRIDE_VALUE_MASK		GENMASK(7, 5)
#define CCA_DEFAULT_OVERRIDE_VALUE_SHIFT	5
#define CCA_DEFAULT_OVERRIDE_VALUE_BITS		3
#define CCA_DEFAULT_OVERRIDE_ENABLE		(0x1 << 4)
#define CM_DEFAULT_TARGET_MASK			GENMASK(1, 0)
#define CM_DEFAULT_TARGET_SHIFT			0
#define CM_DEFAULT_TARGET_BITS			2

/* GCR_GIC_BASE fields */
#define GCR_GIC_BASE_ADDRMASK			GENMASK(31, 7)
#define GCR_GIC_BASE_ADDRMASK_SHIFT		7
#define GCR_GIC_EN				(0x1 << 0)

/* GCR_CPC_BASE fields */
#define GCR_CPC_BASE_ADDRMASK			GENMASK(31, 15)
#define GCR_CPC_BASE_ADDRMASK_SHIFT		15
#define GCR_CPC_EN				(0x1 << 0)

/* GCR_REG0_MASK fields */
#define GCR_REGn_MASK_ADDRMASK			GENMASK(31, 16)
#define GCR_REGn_MASK_ADDRMASK_SHIFT		16
#define GCR_REGn_MASK_CCAOVR			GENMASK(7, 5)
#define GCR_REGn_MASK_CCAOVR_SHIFT		5
#define GCR_REGn_MASK_CCAOVREN			BIT(4)
#define GCR_REGn_MASK_DROPL2			BIT(2)
#define GCR_REGn_MASK_CMTGT			GENMASK(1, 0)
#define  GCR_REGn_MASK_CMTGT_DISABLED		0x0
#define  GCR_REGn_MASK_CMTGT_MEM		0x1
#define  GCR_REGn_MASK_CMTGT_IOCU0		0x2
#define  GCR_REGn_MASK_CMTGT_IOCU1		0x3

/* GCR_GIC_STATUS fields */
#define GCR_GIC_EX				(0x1 << 0)

/* GCR_CPC_STATUS fields */
#define GCR_CPC_EX				(0x1 << 0)

/* GCR_L2_CONFIG fields */
#define GCR_L2_CONFIG_ASSOC_SHIFT	0
#define GCR_L2_CONFIG_ASSOC_BITS	8
#define GCR_L2_CONFIG_LINESZ_SHIFT	8
#define GCR_L2_CONFIG_LINESZ_BITS	4
#define GCR_L2_CONFIG_SETSZ_SHIFT	12
#define GCR_L2_CONFIG_SETSZ_BITS	4
#define GCR_L2_CONFIG_BYPASS		(1 << 20)

/* GCR_Cx_COHERENCE */
#define GCR_Cx_COHERENCE_DOM_EN		(0xff << 0)
#define GCR_Cx_COHERENCE_EN		(0x1 << 0)

/* GCR_CL_OTHER fields */
#define GCR_CL_OTHER_CORENUM			GENMASK(31, 16)
#define GCR_CL_OTHER_CORENUM_SHIFT		16

/* CCA_DEFAULT_OVERRIDE_VALUE */
#define CCA_WT				0	/* Write through */
#define CCA_UC				2	/* Uncached */
#define CCA_WB				3	/* Write back no-coherent */
#define CCA_CWBE			4	/* Mapped to WB */
#define CCA_CWB				5	/* Mapped to WB */
#define CCA_UCA				7	/* Uncached accelerated */

/* CM_DEFAULT_TARGET */
#define CM_TARGET_MEMORY		0	/* Memory */
#define CM_TARGET_IOCU0			2	/* 1st IOCU */
#define CM_TARGET_IOCU1			3	/* 2nd IOCU */


#ifndef __ASSEMBLY__

#include <asm/io.h>

static inline void *mips_cm_base(void)
{
	return (void *)CKSEG1ADDR(CONFIG_MIPS_CM_BASE);
}

static inline unsigned long mips_cm_l2_line_size(void)
{
	unsigned long l2conf, line_sz;

	l2conf = __raw_readl(mips_cm_base() + GCR_L2_CONFIG);

	line_sz = l2conf >> GCR_L2_CONFIG_LINESZ_SHIFT;
	line_sz &= GENMASK(GCR_L2_CONFIG_LINESZ_BITS - 1, 0);
	return line_sz ? (2 << line_sz) : 0;
}

static inline unsigned long mips_cm_numcores(void)
{
	return ((__raw_readl(mips_cm_base() + GCR_CONFIG)
		& GCR_CONFIG_PCORES) >> GCR_CONFIG_PCORES_SHIFT) + 1;
}

static inline unsigned long mips_cm_numiocu(void)
{
	return ((__raw_readl(mips_cm_base() + GCR_CONFIG)
		& GCR_CONFIG_NUMIOCU) >> GCR_CONFIG_NUMIOCU_SHIFT);
}

#endif /* !__ASSEMBLY__ */

#endif /* __MIPS_ASM_CM_H__ */
