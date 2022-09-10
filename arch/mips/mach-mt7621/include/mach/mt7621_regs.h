/* SPDX-License-Identifier:	GPL-2.0+ */
/*
 * Copyright (C) 2018 MediaTek Incorporation. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#ifndef _MACH_MT7621_REGS_H_
#define _MACH_MT7621_REGS_H_

#define REG_SET_VAL(_name, _val) \
	(((_name ## _M) & (_val)) << (_name ## _S))

#define REG_MASK(_name) \
	((_name ## _M) << (_name ## _S))

#define REG_GET_VAL(_name, _val) \
	(((_val) >> (_name ## _S)) & (_name ## _M))


#define MT7621_SYSCTL_BASE				0x1e000000
#define MT7621_SYSCTL_SIZE				0x100
#define MT7621_RBUS_BASE				0x1e000400
#define MT7621_RBUS_SIZE				0x100
#define MT7621_GPIO_BASE				0x1e000600
#define MT7621_GPIO_SIZE				0x100
#define MT7621_DMA_CFG_ARB_BASE				0x1e000800
#define MT7621_DMA_CFG_ARB_SIZE				0x100
#define MT7621_SPI_BASE					0x1e000b00
#define MT7621_SPI_SIZE					0x100
#define MT7621_UART1_BASE				0x1e000c00
#define MT7621_UART1_SIZE				0x100
#define MT7621_NFI_BASE					0x1e003000
#define MT7621_NFI_SIZE					0x800
#define MT7621_NFI_ECC_BASE				0x1e003800
#define MT7621_NFI_ECC_SIZE				0x800
#define MT7621_DRAMC_BASE				0x1e005000
#define MT7621_DRAMC_SIZE				0x1000
#define MT7621_FE_BASE					0x1e100000
#define MT7621_FE_SIZE					0xe000
#define MT7621_GMAC_BASE				0x1e110000
#define MT7621_GMAC_SIZE				0x8000
#define MT7621_SSUSB_BASE				0x1e1c0000
#define MT7621_SSUSB_SIZE				0x40000

 /* GIC Base Address */
#define MIPS_GIC_BASE					0x1fbc0000

 /* CPC Base Address */
#define MIPS_CPC_BASE					0x1fbf0000

 /* Flash Memory-mapped Base Address */
#define MT7621_FLASH_MMAP_BASE				0x1fc00000


#define MT7621_FE_SRAM_BASE1				0x8000
#define MT7621_FE_SRAM_BASE2				0xa000


/* MT7621_SYSCTL_BASE */
#define MT7621_SYS_CHIP_REV_ID_REG			0x0c
#define   CPU_ID_S					  17
#define   CPU_ID_M					  0x01
#define   PKG_ID_S					  16
#define   PKG_ID_M					  0x01
#define   VER_ID_S					  8
#define   VER_ID_M					  0x0f
#define   ECO_ID_S					  0
#define   ECO_ID_M					  0x0f

#define MT7621_SYS_SYSCFG0_REG				0x10
#define   XTAL_MODE_SEL_S				  6
#define   XTAL_MODE_SEL_M				  0x07
#define   OCP_RATIO_S					  5
#define   OCP_RATIO_M					  0x01
#define   DRAM_TYPE_S					  4
#define   DRAM_TYPE_M					  0x01
#define   CHIP_MODE_S					  0
#define   CHIP_MODE_M					  0x0f

#define MT7621_BOOT_SRAM_BASE_REG			0x20

#define MT7621_SYS_CLKCFG0_REG				0x2c
#define   CPU_CLK_SEL_S					  30
#define   CPU_CLK_SEL_M					  0x03
#define   OSC_1US_DIV_S					  24
#define   OSC_1US_DIV_M					  0x3f
#define   MPLL_CFG_SEL_S				  23
#define   MPLL_CFG_SEL_M				  0x01
#define   REFCLK_FDIV_S					  18
#define   REFCLK_FDIV_M					  0x1f
#define   PCIE_CLK_SEL_S				  17
#define   PCIE_CLK_SEL_M				  0x01
#define   REFCLK_FFRAC_S				  12
#define   REFCLK_FFRAC_M				  0x3f
#define   REFCLK0_RATE_S				  9
#define   REFCLK0_RATE_M				  0x07
#define   TRGMII_CLK_SEL_S				  5
#define   TRGMII_CLK_SEL_M				  0x03
#define   PERI_CLK_SEL_S				  4
#define   PERI_CLK_SEL_M				  0x01

#define MT7621_SYS_RSTCTL_REG				0x34
#define   PPE_RST_S					  31
#define   PPE_RST_M					  0x01
#define   ETH_RST_S					  23
#define   ETH_RST_M					  0x01
#define   UART3_RST_S					  21
#define   UART3_RST_M					  0x01
#define   UART2_RST_S					  20
#define   UART2_RST_M					  0x01
#define   UART1_RST_S					  19
#define   UART1_RST_M					  0x01
#define   SPI_RST_S					  18
#define   SPI_RST_M					  0x01
#define   NFI_RST_S					  15
#define   NFI_RST_M					  0x01
#define   PIO_RST_S					  13
#define   PIO_RST_M					  0x01
#define   MC_RST_S					  10
#define   MC_RST_M					  0x01
#define   FE_RST_S					  6
#define   FE_RST_M					  0x01
#define   MCM_RST_S					  2
#define   MCM_RST_M					  0x01
#define   SYS_RST_S					  0
#define   SYS_RST_M					  0x01

#define MT7621_SYS_CUR_CLK_STS_REG			0x44
#define   SAME_FREQ_S					  20
#define   SAME_FREQ_M					  0x01
#define   CUR_OCP_RATIO_S				  16
#define   CUR_OCP_RATIO_M				  0x07
#define   CUR_CPU_FDIV_S				  8
#define   CUR_CPU_FDIV_M				  0x1f
#define   CUR_CPU_FFRAC_S				  0
#define   CUR_CPU_FFRAC_M				  0x1f

#define MT7621_SYS_GPIO_MODE_REG			0x60
#define   ESWINT_MODE_S					  20
#define   ESWINT_MODE_M					  0x01
#define   SDXC_MODE_S					  18
#define   SDXC_MODE_M					  0x03
#define   SPI_MODE_S					  16
#define   SPI_MODE_M					  0x03
#define   RGMII2_MODE_S					  15
#define   RGMII2_MODE_M					  0x01
#define   RGMII1_MODE_S					  14
#define   RGMII1_MODE_M					  0x01
#define   MDIO_MODE_S					  12
#define   MDIO_MODE_M					  0x03
#define   PERST_MODE_S					  10
#define   PERST_MODE_M					  0x03
#define   WDT_MODE_S					  8
#define   WDT_MODE_M					  0x03
#define   JTAG_MODE_S					  7
#define   JTAG_MODE_M					  0x01
#define   UART2_MODE_S					  5
#define   UART2_MODE_M					  0x03
#define   UART3_MODE_S					  3
#define   UART3_MODE_M					  0x03
#define   I2C_MODE_S					  2
#define   I2C_MODE_M					  0x01
#define   UART1_MODE_S					  1
#define   UART1_MODE_M					  0x01


/* MT7621_RBUS_BASE */
#define MT7621_RBUS_DYN_CFG0_REG			0x0010
#define   CPU_FDIV_S					  8
#define   CPU_FDIV_M					  0x1f
#define   CPU_FFRAC_S					  0
#define   CPU_FFRAC_M					  0x1f


/* MT7621_DMA_CFG_ARB_BASE */
#define MT7621_DMA_ROUTE_REG				0x000c


/* MT7621_SPI_BASE */
#define MT7621_SPI_SPACE_REG				0x003c
#define   FS_SLAVE_SEL_S				  12
#define   FS_SLAVE_SEL_M				  0x07
#define   FS_CLK_SEL_S					  0
#define   FS_CLK_SEL_M					  0xfff


/* MT7621_DRAMC_BASE */
#define MT7621_DRAMC_MEMPLL1_REG			0x0604
#define   RG_MEPL_DIV2_SEL_S				  1
#define   RG_MEPL_DIV2_SEL_M				  0x03

#define MT7621_DRAMC_MEMPLL6_REG			0x0618
#define MT7621_DRAMC_MEMPLL18_REG			0x0648
#define   RG_MEPL_VODEN_S				  31
#define   RG_MEPL_VODEN_M				  0x01
#define   RG_MEPL_LVRODEN_S				  30
#define   RG_MEPL_LVRODEN_M				  0x01
#define   RG_MEPL_FPEN_S				  29
#define   RG_MEPL_FPEN_M				  0x01
#define   RG_MEPL_FMEN_S				  28
#define   RG_MEPL_FMEN_M				  0x01
#define   RG_MEPL_M4PDIV_S				  26
#define   RG_MEPL_M4PDIV_M				  0x03
#define   RG_MEPL_EN_S					  25
#define   RG_MEPL_EN_M					  0x01
#define   RG_MEPL_MON_EN_S				  24
#define   RG_MEPL_MON_EN_M				  0x01
#define   RG_MEPL_MONVCEN_S				  23
#define   RG_MEPL_MONVCEN_M				  0x01
#define   RG_MEPL_MONCKEN_S				  22
#define   RG_MEPL_MONCKEN_M				  0x01
#define   RG_MEPL_BR_S					  21
#define   RG_MEPL_BR_M					  0x01
#define   RG_MEPL_BP_S					  20
#define   RG_MEPL_BP_M					  0x01
#define   RG_MEPL_HF_S					  19
#define   RG_MEPL_HF_M					  0x01
#define   RG_MEPL_DIVEN_S				  16
#define   RG_MEPL_DIVEN_M				  0x07
#define   RG_MEPL_POSDIV_S				  14
#define   RG_MEPL_POSDIV_M				  0x03
#define   RG_MEPL_PREDIV_S				  12
#define   RG_MEPL_PREDIV_M				  0x03
#define   RG_MEPL_LF_S					  11
#define   RG_MEPL_LF_M					  0x01
#define   RG_MEPL_FBDIV_S				  4
#define   RG_MEPL_FBDIV_M				  0x7f
#define   RG_MEPL_DDSEN_S				  3
#define   RG_MEPL_DDSEN_M				  0x01
#define   RG_MEPL_FBSEL_S				  1
#define   RG_MEPL_FBSEL_M				  0x03
#define   RG_MEPL_VCO_DIV_SEL_S				  0
#define   RG_MEPL_VCO_DIV_SEL_M				  0x01


/* MT7621_FE_BASE */
#define MT7621_FE_RST_GLO_REG				0x0004
#define   FE_PSE_RAM_S					  2
#define   FE_PSE_RAM_M					  0x01
#define   FE_PSE_MEM_EN_S				  1
#define   FE_PSE_MEM_EN_M				  0x01
#define   FE_PSE_RESET_S				  0
#define   FE_PSE_RESET_M				  0x01

/* MT7621_SSUSB_BASE */
#define MT7621_SSUSB_MAC_CK_CTRL_REG			0x10784
#define   SSUSB_MAC3_SYS_CK_GATE_MASK_TIME_S		  16
#define   SSUSB_MAC3_SYS_CK_GATE_MASK_TIME_M		  0xff
#define   SSUSB_MAC2_SYS_CK_GATE_MASK_TIME_S		  8
#define   SSUSB_MAC2_SYS_CK_GATE_MASK_TIME_M		  0xff
#define   SSUSB_MAC3_SYS_CK_GATE_MODE_S			  2
#define   SSUSB_MAC3_SYS_CK_GATE_MODE_M			  0x03
#define   SSUSB_MAC2_SYS_CK_GATE_MODE_S			  0
#define   SSUSB_MAC2_SYS_CK_GATE_MODE_M			  0x03

#define MT7621_SSUSB_B2_ROSC_0_REG			0x10a40
#define   SSUSB_RING_OSC_CNTEND_S			  23
#define   SSUSB_RING_OSC_CNTEND_M			  0x1ff
#define   SSUSB_XTAL_OSC_CNTEND_S			  16
#define   SSUSB_XTAL_OSC_CNTEND_M			  0x7f
#define   SSUSB_RING_OSC_EN_S				  3
#define   SSUSB_RING_OSC_EN_M				  0x01
#define   SSUSB_RING_OSC_FORCE_EN_S			  2
#define   SSUSB_RING_OSC_FORCE_EN_M			  0x01
#define   SSUSB_FRC_RING_BYPASS_DET_S			  1
#define   SSUSB_FRC_RING_BYPASS_DET_M			  0x01
#define   SSUSB_RING_BYPASS_DET_S			  0
#define   SSUSB_RING_BYPASS_DET_M			  0x01

#define MT7621_SSUSB_B2_ROSC_1_REG			0x10a44
#define   SSUSB_RING_OSC_FRC_P3_S			  20
#define   SSUSB_RING_OSC_FRC_P3_M			  0x01
#define   SSUSB_RING_OSC_P3_S				  19
#define   SSUSB_RING_OSC_P3_M				  0x01
#define   SSUSB_RING_OSC_FRC_RECAL_S			  17
#define   SSUSB_RING_OSC_FRC_RECAL_M			  0x03
#define   SSUSB_RING_OSC_RECAL_S			  16
#define   SSUSB_RING_OSC_RECAL_M			  0x01
#define   SSUSB_RING_OSC_SEL_S				  8
#define   SSUSB_RING_OSC_SEL_M				  0xff
#define   SSUSB_RING_OSC_FRC_SEL_S			  0
#define   SSUSB_RING_OSC_FRC_SEL_M			  0x01

#define MT7621_SSUSB_U3PHYA_1_REG			0x10b04
#define   SSUSB_PCIE_CLKDRV_SLEW_S			  30
#define   SSUSB_PCIE_CLKDRV_SLEW_M			  0x03
#define   SSUSB_PCIE_CLKDRV_AMP_S			  27
#define   SSUSB_PCIE_CLKDRV_AMP_M			  0x07
#define   SSUSB_XTAL_TST_A2DCK_EN_S			  26
#define   SSUSB_XTAL_TST_A2DCK_EN_M			  0x01
#define   SSUSB_XTAL_MON_EN_S				  25
#define   SSUSB_XTAL_MON_EN_M				  0x01
#define   SSUSB_XTAL_HYS_S				  24
#define   SSUSB_XTAL_HYS_M				  0x01
#define   SSUSB_XTAL_TOP_RESERVE_S			  8
#define   SSUSB_XTAL_TOP_RESERVE_M			  0xffff
#define   SSUSB_SYSPLL_RESERVE_S			  4
#define   SSUSB_SYSPLL_RESERVE_M			  0x0f
#define   SSUSB_SYSPLL_FBSEL_S				  2
#define   SSUSB_SYSPLL_FBSEL_M				  0x03
#define   SSUSB_SYSPLL_PREDIV_S				  0
#define   SSUSB_SYSPLL_PREDIV_M				  0x03

#define MT7621_SSUSB_U3PHYA_2_REG			0x10b08
#define   SSUSB_SYSPLL_LF_S				  31
#define   SSUSB_SYSPLL_LF_M				  0x01
#define   SSUSB_SYSPLL_FBDIV_S				  24
#define   SSUSB_SYSPLL_FBDIV_M				  0x7f
#define   SSUSB_SYSPLL_POSDIV_S				  22
#define   SSUSB_SYSPLL_POSDIV_M				  0x03
#define   SSUSB_SYSPLL_VCO_DIV_SEL_S			  21
#define   SSUSB_SYSPLL_VCO_DIV_SEL_M			  0x01
#define   SSUSB_SYSPLL_BLP_S				  20
#define   SSUSB_SYSPLL_BLP_M				  0x01
#define   SSUSB_SYSPLL_BP_S				  19
#define   SSUSB_SYSPLL_BP_M				  0x01
#define   SSUSB_SYSPLL_BR_S				  18
#define   SSUSB_SYSPLL_BR_M				  0x01
#define   SSUSB_SYSPLL_BC_S				  17
#define   SSUSB_SYSPLL_BC_M				  0x01
#define   SSUSB_SYSPLL_DIVEN_S				  14
#define   SSUSB_SYSPLL_DIVEN_M				  0x07
#define   SSUSB_SYSPLL_FPEN_S				  13
#define   SSUSB_SYSPLL_FPEN_M				  0x01
#define   SSUSB_SYSPLL_MONCK_EN_S			  12
#define   SSUSB_SYSPLL_MONCK_EN_M			  0x01
#define   SSUSB_SYSPLL_MONVC_EN_S			  11
#define   SSUSB_SYSPLL_MONVC_EN_M			  0x01
#define   SSUSB_SYSPLL_MONREF_EN_S			  10
#define   SSUSB_SYSPLL_MONREF_EN_M			  0x01
#define   SSUSB_SYSPLL_VOD_EN_S				  9
#define   SSUSB_SYSPLL_VOD_EN_M				  0x01
#define   SSUSB_SYSPLL_CK_SEL_S				  8
#define   SSUSB_SYSPLL_CK_SEL_M				  0x01

#define MT7621_SSUSB_U3PHYA_3_REG			0x10b10
#define   SSUSB_SYSPLL_PCW_NCPO_S			  1
#define   SSUSB_SYSPLL_PCW_NCPO_M			  0x7fffffff

#define MT7621_SSUSB_U3PHYA_9_REG			0x10b24
#define   SSUSB_PLL_SSC_PRD_S				  0
#define   SSUSB_PLL_SSC_PRD_M				  0xffff

#define MT7621_SSUSB_U3PHYA_11_REG			0x10b2c
#define   SSUSB_CDR_MONEN_DIG_S				  31
#define   SSUSB_CDR_MONEN_DIG_M				  0x01
#define   SSUSB_CDR_REGOD_S				  29
#define   SSUSB_CDR_REGOD_M				  0x01
#define   SSUSB_RX_DAC_EN_S				  26
#define   SSUSB_RX_DAC_EN_M				  0x01
#define   SSUSB_RX_DAC_PWD_S				  25
#define   SSUSB_RX_DAC_PWD_M				  0x01
#define   SSUSB_EQ_CURSEL_S				  24
#define   SSUSB_EQ_CURSEL_M				  0x01
#define   SSUSB_RX_DAC_MUX_S				  19
#define   SSUSB_RX_DAC_MUX_M				  0x1f
#define   SSUSB_RX_R2T_EN_S				  18
#define   SSUSB_RX_R2T_EN_M				  0x01
#define   SSUSB_RX_T2R_EN_S				  17
#define   SSUSB_RX_T2R_EN_M				  0x01
#define   SSUSB_RX_50_LOWER_S				  14
#define   SSUSB_RX_50_LOWER_M				  0x07
#define   SSUSB_RX_50_TAR_S				  12
#define   SSUSB_RX_50_TAR_M				  0x03
#define   SSUSB_RX_SW_CTRL_S				  7
#define   SSUSB_RX_SW_CTRL_M				  0x0f
#define   SSUSB_PCIE_SIGDET_VTH_S			  5
#define   SSUSB_PCIE_SIGDET_VTH_M			  0x03
#define   SSUSB_PCIE_SIGDET_LPF_S			  3
#define   SSUSB_PCIE_SIGDET_LPF_M			  0x03
#define   SSUSB_LFPS_MON_EN_S				  2
#define   SSUSB_LFPS_MON_EN_M				  0x01

#define MT7621_DA_SSUSB_PLL_FBKDIV_REG			0x10c1c
#define   SSUSB_PLL_FBKDIV_PE2H_S			  24
#define   SSUSB_PLL_FBKDIV_PE2H_M			  0x7f
#define   SSUSB_PLL_FBKDIV_PE1D_S			  16
#define   SSUSB_PLL_FBKDIV_PE1D_M			  0x7f
#define   SSUSB_PLL_FBKDIV_PE1H_S			  8
#define   SSUSB_PLL_FBKDIV_PE1H_M			  0x7f
#define   SSUSB_PLL_FBKDIV_U3_S				  0
#define   SSUSB_PLL_FBKDIV_U3_M				  0x7f

#define MT7621_DA_SSUSB_U3PHYA_10_REG			0x10c20
#define   SSUSB_PLL_PREDIV_PE2D_S			  26
#define   SSUSB_PLL_PREDIV_PE2D_M			  0x03
#define   SSUSB_PLL_PREDIV_PE2H_S			  24
#define   SSUSB_PLL_PREDIV_PE2H_M			  0x03
#define   SSUSB_PLL_PREDIV_PE1D_S			  18
#define   SSUSB_PLL_PREDIV_PE1D_M			  0x03
#define   SSUSB_PLL_PREDIV_PE1H_S			  16
#define   SSUSB_PLL_PREDIV_PE1H_M			  0x03
#define   SSUSB_PLL_PREDIV_U3_S				  8
#define   SSUSB_PLL_PREDIV_U3_M				  0x03
#define   SSUSB_PLL_FBKDI_S				  0
#define   SSUSB_PLL_FBKDI_M				  0x07

#define MT7621_DA_SSUSB_PLL_PCW_NCPO_REG		0x10c24
#define   SSUSB_PLL_PCW_NCPO_U3_S			  0
#define   SSUSB_PLL_PCW_NCPO_U3_M			  0x7fffffff

#define MT7621_DA_SSUSB_PLL_SSC_DELTA1_REG		0x10c38
#define   SSUSB_PLL_SSC_DELTA1_PE1H_S			  16
#define   SSUSB_PLL_SSC_DELTA1_PE1H_M			  0xffff
#define   SSUSB_PLL_SSC_DELTA1_U3_S			  0
#define   SSUSB_PLL_SSC_DELTA1_U3_M			  0xffff

#define MT7621_DA_SSUSB_U3PHYA_21_REG			0x10c40
#define   SSUSB_PLL_SSC_DELTA_U3_S			  16
#define   SSUSB_PLL_SSC_DELTA_U3_M			  0xffff
#define   SSUSB_PLL_SSC_DELTA1_PE2D_S			  0
#define   SSUSB_PLL_SSC_DELTA1_PE2D_M			  0xffff


#endif /* _MACH_MT7621_REGS_H_ */
