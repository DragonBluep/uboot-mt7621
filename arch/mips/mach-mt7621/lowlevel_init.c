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

static void mt7621_dram_init(void);
static void mt7621_xhci_config(void);

void lowlevel_init(void)
{
	void __iomem *base;

#if !defined(CONFIG_SPL) || defined(CONFIG_TPL_BUILD) || \
    (!defined(CONFIG_TPL) && defined(CONFIG_SPL_BUILD))
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

#ifndef CONFIG_TPL_BUILD
	/* Get CPU clock first so we can use delay functions */
	get_cpu_freq(0);

	/* Do DRAMC & DDR initialization */
	mt7621_dram_init();

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

static void __maybe_unused mt7621_dram_init(void)
{
	void __iomem *base;
	u32 syscfg0;
	mpll_ddr_config_t cfg;

	memset(&cfg, 0, sizeof (cfg));

	/* DRAM type: 0 for DDR3, 1 for DDR2 */
	base = (void __iomem *) CKSEG1ADDR(MT7621_SYSCTL_BASE);
	syscfg0 = readl(base + MT7621_SYS_SYSCFG0_REG);
	cfg.dram_type = REG_GET_VAL(DRAM_TYPE, syscfg0) ?
			MT7621_DDR2 : MT7621_DDR3;

#ifdef CONFIG_MT7621_DRAM_INIT_USER_PARAM_FIRST
#ifdef CONFIG_MT7621_DRAM_INIT_USER_PARAM_BOARD_PROVIDED
	cfg.init_type = DDR_INIT_AUTO_PROBE;

	if (cfg.dram_type == MT7621_DDR2) {
		if (board_get_ddr2_init_param) {
			board_get_ddr2_init_param(
				&cfg.board_provided_ddr2_param,
				&cfg.board_expected_ddr2_size);

			if (cfg.board_provided_ddr2_param &&
			    cfg.board_expected_ddr2_size)
				cfg.init_type = DDR_INIT_BOARD_PROVIDED_FIRST;
		}
	} else {
		if (board_get_ddr3_init_param) {
			board_get_ddr3_init_param(
				&cfg.board_provided_ddr3_param,
				&cfg.board_expected_ddr3_size);

			if (cfg.board_provided_ddr3_param &&
			    cfg.board_expected_ddr3_size)
				cfg.init_type = DDR_INIT_BOARD_PROVIDED_FIRST;
		}
	}
#else
#ifdef CONFIG_MT7621_DRAM_DDR2_512M
	cfg.pre_defined_ddr2_param = DDR2_512M;
#elif defined (CONFIG_MT7621_DRAM_DDR2_512M_W9751G6KB_A02_1066MHZ)
	cfg.pre_defined_ddr2_param = DDR2_W9751G6KB_A02_1066_512M;
#elif defined (CONFIG_MT7621_DRAM_DDR2_1024M)
	cfg.pre_defined_ddr2_param = DDR2_1024M;
#elif defined (CONFIG_MT7621_DRAM_DDR2_1024M_W971GG6KB25_800MHZ)
	cfg.pre_defined_ddr2_param = DDR2_W971GG6KB25_800_1024M;
#elif defined (CONFIG_MT7621_DRAM_DDR2_1024M_W971GG6KB18_1066MHZ)
	cfg.pre_defined_ddr2_param = DDR2_W971GG6KB18_1066_1024M;
#else
#error Invalid pre-defined DDR2 parameter selection
#endif

#ifdef CONFIG_MT7621_DRAM_DDR3_1024M
	cfg.pre_defined_ddr3_param = DDR3_1024M;
#elif defined (CONFIG_MT7621_DRAM_DDR3_1024M_KGD)
	cfg.pre_defined_ddr3_param = DDR3_1024M_KGD;
#elif defined (CONFIG_MT7621_DRAM_DDR3_2048M)
	cfg.pre_defined_ddr3_param = DDR3_2048M;
#elif defined (CONFIG_MT7621_DRAM_DDR3_4096M)
	cfg.pre_defined_ddr3_param = DDR3_4096M;
#else
#error Invalid pre-defined DDR3 parameter selection
#endif

	cfg.init_type = DDR_INIT_PRE_DEFINED_FIRST;
#endif
#else
	cfg.init_type = DDR_INIT_AUTO_PROBE;
#endif

#ifdef CONFIG_MT7621_DRAM_INIT_AUTO_PROBE
	cfg.auto_probe = 1;
#endif

	cfg.dram_speed = CONFIG_MT7621_DRAM_FREQ;
	cfg.cpu_speed = CONFIG_MT7621_CPU_FREQ;

	/* Base clock used for MPLL */
	mt7621_get_clocks(NULL, NULL, &cfg.xtal_freq);

	/* Change MPLL source from Xtal to CR */
	setbits_le32(base + MT7621_SYS_CLKCFG0_REG,
		     REG_SET_VAL(MPLL_CFG_SEL, 1));

	if (mt7621_dramc_init(&cfg))
#ifdef CONFIG_MT7621_RESET_WHEN_DRAMC_FAIL
		_machine_restart_spl();
#else
		hang();
#endif

	gd->ram_size = cfg.actual_memsize;
}

/* Called during DDR initialization */
void mt7621_dramc_reset(void)
{
	void __iomem *base;

	base = (void __iomem *) CKSEG1ADDR(MT7621_SYSCTL_BASE);

	/* Reset DRAMC */
	setbits_le32(base + MT7621_SYS_RSTCTL_REG, REG_SET_VAL(MC_RST, 1));
	__udelay(100);
	clrbits_le32(base + MT7621_SYS_RSTCTL_REG, REG_SET_VAL(MC_RST, 1));
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