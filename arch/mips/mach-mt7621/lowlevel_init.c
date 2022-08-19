// SPDX-License-Identifier:	GPL-2.0+
/*
 * Copyright (C) 2018 MediaTek Incorporation. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <common.h>
#include <div64.h>
#include <asm/io.h>
#include <asm/addrspace.h>
#include <asm/mipsregs.h>
#include <linux/sizes.h>
#include <mach/mt7621_regs.h>

#include "lowlevel_init.h"
#include "clocks.h"
#include "cps.h"

#include "memtest/memtester.h"

DECLARE_GLOBAL_DATA_PTR;

static void mt7621_xhci_config(void);

void pre_lowlevel_init(void)
{
#if !defined(CONFIG_SPL) || defined(CONFIG_TPL_BUILD) || \
    (!defined(CONFIG_TPL) && defined(CONFIG_SPL_BUILD))
	void __iomem *base;

	/* Initialize Coherent Processing System (CPS) related components */
	mt7621_cps_init();

	/* Set SPI clock to system bus / (5 + 2) */
	base = (void __iomem *) CKSEG1ADDR(MT7621_SPI_BASE);
	clrsetbits_le32(base + MT7621_SPI_SPACE_REG,
		REG_MASK(FS_CLK_SEL),
		REG_SET_VAL(FS_CLK_SEL, 5));

	/* Change CPU ratio from 1/0xA to 1/1 */
	base = (void __iomem *) CKSEG1ADDR(MT7621_RBUS_BASE);
	writel(REG_SET_VAL(CPU_FDIV, 1) | REG_SET_VAL(CPU_FFRAC, 1),
		base + MT7621_RBUS_DYN_CFG0_REG);
#endif
}

void post_lowlevel_init(void)
{
#ifndef CONFIG_TPL_BUILD
	void __iomem *base;

	gd->ram_size = get_ram_size((void *) KSEG1, SZ_512M);

	/* Change CPU PLL from 500MHz to CPU_PLL */
	base = (void __iomem *) CKSEG1ADDR(MT7621_SYSCTL_BASE);
	clrsetbits_le32(base + MT7621_SYS_CLKCFG0_REG,
		REG_MASK(CPU_CLK_SEL), REG_SET_VAL(CPU_CLK_SEL, 1));

	/* Get final CPU clock */
	gd->cpu_clk = 0;
	get_cpu_freq(0);

	/* Setup USB xHCI */
	mt7621_xhci_config();

	/* Do memory test */
#ifdef CONFIG_MT7621_MEMTEST
	memtest(gd->ram_size);
#endif
#endif
}

int arch_early_init_r(void)
{
	void __iomem *base;

	/* Reset Frame Engine SRAM */
	base = (void __iomem *) CKSEG1ADDR(MT7621_FE_BASE);
	setbits_le32(base + MT7621_FE_RST_GLO_REG,
		REG_SET_VAL(FE_PSE_RESET, 1));

	return 0;
}

int dram_init(void)
{
#ifdef CONFIG_SPL
	gd->ram_size = get_ram_size((void *) KSEG1, SZ_512M);
#endif
	return 0;
}

static void __maybe_unused mt7621_xhci_config_40mhz(void)
{
	void __iomem *base;

	base = (void __iomem *) CKSEG1ADDR(MT7621_SSUSB_BASE);

	writel(0x10 |
		REG_SET_VAL(SSUSB_MAC3_SYS_CK_GATE_MASK_TIME, 0x20) |
		REG_SET_VAL(SSUSB_MAC2_SYS_CK_GATE_MASK_TIME, 0x20) |
		REG_SET_VAL(SSUSB_MAC3_SYS_CK_GATE_MODE, 2) |
		REG_SET_VAL(SSUSB_MAC2_SYS_CK_GATE_MODE, 2),
		base + MT7621_SSUSB_MAC_CK_CTRL_REG);

	writel(REG_SET_VAL(SSUSB_PLL_PREDIV_PE1D, 2) |
		REG_SET_VAL(SSUSB_PLL_PREDIV_U3, 1) |
		REG_SET_VAL(SSUSB_PLL_FBKDI, 4),
		base + MT7621_DA_SSUSB_U3PHYA_10_REG);

	writel(REG_SET_VAL(SSUSB_PLL_FBKDIV_PE2H, 0x18) |
		REG_SET_VAL(SSUSB_PLL_FBKDIV_PE1D, 0x18) |
		REG_SET_VAL(SSUSB_PLL_FBKDIV_PE1H, 0x18) |
		REG_SET_VAL(SSUSB_PLL_FBKDIV_U3, 0x1e),
		base + MT7621_DA_SSUSB_PLL_FBKDIV_REG);

	writel(REG_SET_VAL(SSUSB_PLL_PCW_NCPO_U3, 0x1e400000),
		base + MT7621_DA_SSUSB_PLL_PCW_NCPO_REG);

	writel(REG_SET_VAL(SSUSB_PLL_SSC_DELTA1_PE1H, 0x25) |
		REG_SET_VAL(SSUSB_PLL_SSC_DELTA1_U3, 0x73),
		base + MT7621_DA_SSUSB_PLL_SSC_DELTA1_REG);

	writel(REG_SET_VAL(SSUSB_PLL_SSC_DELTA_U3, 0x71) |
		REG_SET_VAL(SSUSB_PLL_SSC_DELTA1_PE2D, 0x4a),
		base + MT7621_DA_SSUSB_U3PHYA_21_REG);

	writel(REG_SET_VAL(SSUSB_PLL_SSC_PRD, 0x140),
		base + MT7621_SSUSB_U3PHYA_9_REG);

	writel(REG_SET_VAL(SSUSB_SYSPLL_PCW_NCPO, 0x11c00000),
		base + MT7621_SSUSB_U3PHYA_3_REG);

	writel(REG_SET_VAL(SSUSB_PCIE_CLKDRV_AMP, 4) |
		REG_SET_VAL(SSUSB_SYSPLL_FBSEL, 1) |
		REG_SET_VAL(SSUSB_SYSPLL_PREDIV, 1),
		base + MT7621_SSUSB_U3PHYA_1_REG);

	writel(REG_SET_VAL(SSUSB_SYSPLL_FBDIV, 0x12) |
		REG_SET_VAL(SSUSB_SYSPLL_VCO_DIV_SEL, 1) |
		REG_SET_VAL(SSUSB_SYSPLL_FPEN, 1) |
		REG_SET_VAL(SSUSB_SYSPLL_MONCK_EN, 1) |
		REG_SET_VAL(SSUSB_SYSPLL_VOD_EN, 1),
		base + MT7621_SSUSB_U3PHYA_2_REG);

	writel(REG_SET_VAL(SSUSB_EQ_CURSEL, 1) |
		REG_SET_VAL(SSUSB_RX_DAC_MUX, 8) |
		REG_SET_VAL(SSUSB_PCIE_SIGDET_VTH, 1) |
		REG_SET_VAL(SSUSB_PCIE_SIGDET_LPF, 1),
		base + MT7621_SSUSB_U3PHYA_11_REG);

	writel(REG_SET_VAL(SSUSB_RING_OSC_CNTEND, 0x1ff) |
		REG_SET_VAL(SSUSB_XTAL_OSC_CNTEND, 0x7f) |
		REG_SET_VAL(SSUSB_RING_BYPASS_DET, 1),
		base + MT7621_SSUSB_B2_ROSC_0_REG);

	writel(REG_SET_VAL(SSUSB_RING_OSC_FRC_RECAL, 3) |
		REG_SET_VAL(SSUSB_RING_OSC_FRC_SEL, 1),
		base + MT7621_SSUSB_B2_ROSC_1_REG);
}

static void __maybe_unused mt7621_xhci_config_25mhz(void)
{
	void __iomem *base;

	base = (void __iomem *) CKSEG1ADDR(MT7621_SSUSB_BASE);

	writel(0x10 |
		REG_SET_VAL(SSUSB_MAC3_SYS_CK_GATE_MASK_TIME, 0x20) |
		REG_SET_VAL(SSUSB_MAC2_SYS_CK_GATE_MASK_TIME, 0x20) |
		REG_SET_VAL(SSUSB_MAC3_SYS_CK_GATE_MODE, 2) |
		REG_SET_VAL(SSUSB_MAC2_SYS_CK_GATE_MODE, 2),
		base + MT7621_SSUSB_MAC_CK_CTRL_REG);

	writel(REG_SET_VAL(SSUSB_PLL_PREDIV_PE1D, 2) |
		REG_SET_VAL(SSUSB_PLL_FBKDI, 4),
		base + MT7621_DA_SSUSB_U3PHYA_10_REG);

	writel(REG_SET_VAL(SSUSB_PLL_FBKDIV_PE2H, 0x18) |
		REG_SET_VAL(SSUSB_PLL_FBKDIV_PE1D, 0x18) |
		REG_SET_VAL(SSUSB_PLL_FBKDIV_PE1H, 0x18) |
		REG_SET_VAL(SSUSB_PLL_FBKDIV_U3, 0x19),
		base + MT7621_DA_SSUSB_PLL_FBKDIV_REG);

	writel(REG_SET_VAL(SSUSB_PLL_PCW_NCPO_U3, 0x18000000),
		base + MT7621_DA_SSUSB_PLL_PCW_NCPO_REG);

	writel(REG_SET_VAL(SSUSB_PLL_SSC_DELTA1_PE1H, 0x25) |
		REG_SET_VAL(SSUSB_PLL_SSC_DELTA1_U3, 0x4a),
		base + MT7621_DA_SSUSB_PLL_SSC_DELTA1_REG);

	writel(REG_SET_VAL(SSUSB_PLL_SSC_DELTA_U3, 0x48) |
		REG_SET_VAL(SSUSB_PLL_SSC_DELTA1_PE2D, 0x4a),
		base + MT7621_DA_SSUSB_U3PHYA_21_REG);

	writel(REG_SET_VAL(SSUSB_PLL_SSC_PRD, 0x190),
		base + MT7621_SSUSB_U3PHYA_9_REG);

	writel(REG_SET_VAL(SSUSB_SYSPLL_PCW_NCPO, 0xe000000),
		base + MT7621_SSUSB_U3PHYA_3_REG);

	writel(REG_SET_VAL(SSUSB_PCIE_CLKDRV_AMP, 4) |
		REG_SET_VAL(SSUSB_SYSPLL_FBSEL, 1),
		base + MT7621_SSUSB_U3PHYA_1_REG);

	writel(REG_SET_VAL(SSUSB_SYSPLL_FBDIV, 0xf) |
		REG_SET_VAL(SSUSB_SYSPLL_VCO_DIV_SEL, 1) |
		REG_SET_VAL(SSUSB_SYSPLL_FPEN, 1) |
		REG_SET_VAL(SSUSB_SYSPLL_MONCK_EN, 1) |
		REG_SET_VAL(SSUSB_SYSPLL_VOD_EN, 1),
		base + MT7621_SSUSB_U3PHYA_2_REG);

	writel(REG_SET_VAL(SSUSB_EQ_CURSEL, 1) |
		REG_SET_VAL(SSUSB_RX_DAC_MUX, 8) |
		REG_SET_VAL(SSUSB_PCIE_SIGDET_VTH, 1) |
		REG_SET_VAL(SSUSB_PCIE_SIGDET_LPF, 1),
		base + MT7621_SSUSB_U3PHYA_11_REG);

	writel(REG_SET_VAL(SSUSB_RING_OSC_CNTEND, 0x1ff) |
		REG_SET_VAL(SSUSB_XTAL_OSC_CNTEND, 0x7f) |
		REG_SET_VAL(SSUSB_RING_BYPASS_DET, 1),
		base + MT7621_SSUSB_B2_ROSC_0_REG);

	writel(REG_SET_VAL(SSUSB_RING_OSC_FRC_RECAL, 3) |
		REG_SET_VAL(SSUSB_RING_OSC_FRC_SEL, 1),
		base + MT7621_SSUSB_B2_ROSC_1_REG);
}

static void __maybe_unused mt7621_xhci_config(void)
{
	switch (gd->arch.xtal_clk) {
	case 40 * 1000 * 1000:
		mt7621_xhci_config_40mhz();
		break;
	case 25 * 1000 * 1000:
		mt7621_xhci_config_25mhz();
		break;
	case 20 * 1000 * 1000:
		break;
	}
}