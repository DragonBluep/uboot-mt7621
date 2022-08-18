/* SPDX-License-Identifier:	GPL-2.0+ */
/*
 * Copyright (C) 2018 MediaTek Incorporation. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#ifndef _MT7621_ETH_H_
#define _MT7621_ETH_H_

#define REG_SET_VAL(_name, _val) \
	(((_name ## _M) & (_val)) << (_name ## _S))

#define REG_GET_VAL(_name, _val) \
	(((_val) >> (_name ## _S)) & (_name ## _M))

#define REG_MASK(_name) \
	((_name ## _M) << (_name ## _S))

/* FE Registers */
#define PDMA_BASE		0x0800
#define GDMA1_BASE		0x0500
#define GDMA2_BASE		0x1500

/* PDMA */
#define TX_BASE_PTR_REG(n)	(0x000 + (n * 0x10))
#define TX_MAX_CNT_REG(n)	(0x004 + (n * 0x10))
#define TX_CTX_IDX_REG(n)	(0x008 + (n * 0x10))
#define TX_DTX_IDX_REG(n)	(0x00c + (n * 0x10))

#define RX_BASE_PTR_REG(n)	(0x100 + (n * 0x10))
#define RX_MAX_CNT_REG(n)	(0x104 + (n * 0x10))
#define RX_CRX_IDX_REG(n)	(0x108 + (n * 0x10))
#define RX_DRX_IDX_REG(n)	(0x10c + (n * 0x10))

#define PDMA_INFO_REG		0x200
#define   INDEX_WIDTH_S		  24
#define   INDEX_WIDTH_M		  0x0f
#define   BASE_PTR_WIDTH_S	  16
#define   BASE_PTR_WIDTH_M	  0xff
#define   RX_RING_NUM_S		  8
#define   RX_RING_NUM_M		  0xff
#define   TX_RING_NUM_S		  8
#define   TX_RING_NUM_M		  0xff

#define PDMA_GLO_CFG_REG	0x204
#define   RX_2B_OFFSET_S	  31
#define   RX_2B_OFFSET_M	  0x01
#define   CSR_CLKGATE_S		  30
#define   CSR_CLKGATE_M		  0x01
#define   BYTE_SWAP_S		  29
#define   BYTE_SWAP_M		  0x01
#define   MULTI_EN_S		  10
#define   MULTI_EN_M		  0x01
#define   EXT_FIFO_EN_S		  9
#define   EXT_FIFO_EN_M		  0x01
#define   DESC_32B_E_S		  8
#define   DESC_32B_E_M		  0x01
#define   BIG_ENDIAN_S		  7
#define   BIG_ENDIAN_M		  0x01
#define   TX_WB_DDONE_S		  6
#define   TX_WB_DDONE_M		  0x01
#define   PDMA_BT_SIZE_S	  4
#define   PDMA_BT_SIZE_M	  0x03
#define     BT_SIZE_16B		    0
#define     BT_SIZE_32B		    1
#define     BT_SIZE_64B		    2
#define   RX_DMA_BUSY_S		  3
#define   RX_DMA_BUSY_M		  0x01
#define   RX_DMA_EN_S		  2
#define   RX_DMA_EN_M		  0x01
#define   TX_DMA_BUSY_S		  1
#define   TX_DMA_BUSY_M		  0x01
#define   TX_DMA_EN_S		  0
#define   TX_DMA_EN_M		  0x01

#define PDMA_RST_IDX_REG	0x208
#define   RST_DRX_IDX1_S	  17
#define   RST_DRX_IDX1_M	  0x01
#define   RST_DRX_IDX0_S	  16
#define   RST_DRX_IDX0_M	  0x01
#define   RST_DTX_IDX3_S	  3
#define   RST_DTX_IDX3_M	  0x01
#define   RST_DTX_IDX2_S	  2
#define   RST_DTX_IDX2_M	  0x01
#define   RST_DTX_IDX1_S	  1
#define   RST_DTX_IDX1_M	  0x01
#define   RST_DTX_IDX0_S	  0
#define   RST_DTX_IDX0_M	  0x01

#define DLY_INT_CFG_REG		0x20C
#define   TXDLY_INT_EN_S	  31
#define   TXDLY_INT_EN_M	  0x01
#define   TXMAX_PINT_S		  24
#define   TXMAX_PINT_M		  0x7f
#define   TXMAX_PTIME_S		  16
#define   TXMAX_PTIME_M		  0xff
#define   RXDLY_INT_EN_S	  15
#define   RXDLY_INT_EN_M	  0x01
#define   RXMAX_PINT_S		  8
#define   RXMAX_PINT_M		  0x7f
#define   RXMAX_PTIME_S		  0
#define   RXMAX_PTIME_M		  0xff

#define FREEQ_THRES_REG		0x210
#define   FREEQ_THRES_S		  0
#define   FREEQ_THRES_M		  0x0f

#define INT_STATUS_REG		0x220
#define INT_MASK_REG		0x228
#define   RX_COHERENT_S		  31
#define   RX_COHERENT_M		  0x01
#define   RX_DLY_INT_S		  30
#define   RX_DLY_INT_M		  0x01
#define   TX_COHERENT_S		  29
#define   TX_COHERENT_M		  0x01
#define   TX_DLY_INT_S		  28
#define   TX_DLY_INT_M		  0x01
#define   RX_DONE_INT1_S	  17
#define   RX_DONE_INT1_M	  0x01
#define   RX_DONE_INT0_S	  16
#define   RX_DONE_INT0_M	  0x01
#define   TX_DONE_INT3_S	  3
#define   TX_DONE_INT3_M	  0x01
#define   TX_DONE_INT2_S	  2
#define   TX_DONE_INT2_M	  0x01
#define   TX_DONE_INT1_S	  1
#define   TX_DONE_INT1_M	  0x01
#define   TX_DONE_INT0_S	  0
#define   TX_DONE_INT0_M	  0x01

#define SCH_Q01_CFG_REG		0x280
#define SCH_Q23_CFG_REG		0x284
#define   MAX_BKT_SIZE_H_S	  31
#define   MAX_BKT_SIZE_H_M	  0x01
#define   MAX_RATE_ULMT_H_S	  30
#define   MAX_RATE_ULMT_H_M	  0x01
#define   MAX_WEIGHT_H_S	  28
#define   MAX_WEIGHT_H_M	  0x03
#define   MIN_RATE_RATIO_H_S	  26
#define   MIN_RATE_RATIO_H_M	  0x03
#define   MAX_RATE_H_S		  16
#define   MAX_RATE_H_M		  0x3ff
#define   MAX_BKT_SIZE_L_S	  15
#define   MAX_BKT_SIZE_L_M	  0x01
#define   MAX_RATE_ULMT_L_S	  14
#define   MAX_RATE_ULMT_L_M	  0x01
#define   MAX_WEIGHT_L_S	  12
#define   MAX_WEIGHT_L_M	  0x03
#define   MIN_RATE_RATIO_L_S	  10
#define   MIN_RATE_RATIO_L_M	  0x03
#define   MAX_RATE_L_S		  0
#define   MAX_RATE_L_M		  0x3ff

/* GDMA */
#define GDMA_IG_CTRL_REG	0x000
#define   INSV_EN_S		  25
#define   INSV_EN_M		  0x01
#define   STAG_EN_S		  24
#define   STAG_EN_M		  0x01
#define   GDM_ICS_EN_S		  22
#define   GDM_ICS_EN_M		  0x01
#define   GDM_TCS_EN_S		  21
#define   GDM_TCS_EN_M		  0x01
#define   GDM_UCS_EN_S		  20
#define   GDM_UCS_EN_M		  0x01
#define   DROP_256B_S		  19
#define   DROP_256B_M		  0x01
#define   STRP_CRC_S		  16
#define   STRP_CRC_M		  0x01
#define   MYMAC_DP_S		  12
#define   MYMAC_DP_M		  0x0f
#define   BC_DP_S		  8
#define   BC_DP_M		  0x0f
#define   MC_DP_S		  4
#define   MC_DP_M		  0x0f
#define   UN_DP_S		  0
#define   UN_DP_M		  0x0f
#define     DP_PDMA		    0
#define     DP_GDMA1		    1
#define     DP_GDMA2		    2
#define     DP_PPE		    4
#define     DP_QDMA		    5
#define     DP_DISCARD		    7

#define GDMA_EG_CTRL_REG	0x004
#define   UNTAG_EN_S		  30
#define   UNTAG_EN_M		  0x01
#define   DIS_PAD_S		  29
#define   DIS_PAD_M		  0x01
#define   DIS_CRC_S		  28
#define   DIS_CRC_M		  0x01
#define   SHPR_EN_S		  24
#define   SHPR_EN_M		  0x01
#define   BK_SIZE_S		  16
#define   BK_SIZE_M		  0xff
#define   TK_TICK_S		  15
#define   TK_TICK_M		  0x01
#define   TK_RATE_S		  0
#define   TK_RATE_M		  0x3fff

#define GDMA_MAC_LSB_REG	0x008
#define   MAC_ADR_31_0_S	  0
#define   MAC_ADR_31_0_M	  0xffffffff

#define GDMA_MAC_MSB_REG	0x00c
#define   MAC_ADR_47_32_S	  0
#define   MAC_ADR_47_32_M	  0xffff

#define GDMA_VLAN_GEN_REG	0x010
#define   GDM_TPID_S		  16
#define   GDM_TPID_M		  0xffff
#define   GDM_PRI_S		  13
#define   GDM_PRI_M		  0x07
#define   GDM_CFI_S		  12
#define   GDM_CFI_M		  0x01
#define   GDM_VID_S		  0
#define   GDM_VID_M		  0xfff


/* GMAC Registers */
#define GMAC_PPSC_REG		0x0000
#define   PHY_AP_EN_S		  31
#define   PHY_AP_EN_M		  0x01
#define   PHY_PRE_EN_S		  30
#define   PHY_PRE_EN_M		  0x01
#define   PHY_MDC_CFG_S		  24
#define   PHY_MDC_CFG_M		  0x3f
#define   EE_AN_EN_S		  16
#define   EE_AN_EN_M		  0x01
#define   PHY_AP_END_ADDR_S	  8
#define   PHY_AP_END_ADDR_M	  0x1f
#define   PHY_AP_START_ADDR_S	  0
#define   PHY_AP_START_ADDR_M	  0x1f

#define GMAC_PIAC_REG		0x0004
#define   PHY_ACS_ST_S		  31
#define   PHY_ACS_ST_M		  0x01
#define   MDIO_REG_ADDR_S	  25
#define   MDIO_REG_ADDR_M	  0x1f
#define   MDIO_PHY_ADDR_S	  20
#define   MDIO_PHY_ADDR_M	  0x1f
#define   MDIO_CMD_S		  18
#define   MDIO_CMD_M		  0x03
#define     MDIO_CMD_WRITE	    0x01
#define     MDIO_CMD_READ	    0x02
#define   MDIO_ST_S		  16
#define   MDIO_ST_M		  0x03
#define   MDIO_RW_DATA_S	  0
#define   MDIO_RW_DATA_M	  0xffff

#define GMAC_GPC_REG		0x0008
#define   P1_TMII_FREQ_S	  21
#define   P1_TMII_FREQ_M	  0x01
#define   P1_TMII_MODE_S	  20
#define   P1_TMII_MODE_M	  0x01
#define   P1_TX_CLK_MODE_S	  19
#define   P1_TX_CLK_MODE_M	  0x01
#define   P1_RX_CLK_MODE_S	  18
#define   P1_RX_CLK_MODE_M	  0x01
#define   P1_RX_SKEW_S		  16
#define   P1_RX_SKEW_M		  0x03
#define   P0_TMII_FREQ_S	  5
#define   P0_TMII_FREQ_M	  0x01
#define   P0_TMII_MODE_S	  4
#define   P0_TMII_MODE_M	  0x01
#define   P0_TX_CLK_MODE_S	  3
#define   P0_TX_CLK_MODE_M	  0x01
#define   P0_RX_CLK_MODE_S	  2
#define   P0_RX_CLK_MODE_M	  0x01
#define   P0_RX_SKEW_S		  0
#define   P0_RX_SKEW_M		  0x03

#define GMAC_PORT_MCR(p)	(0x0100 + (p) * 0x100)
#define   MAX_RX_JUMBO_S	  28
#define   MAX_RX_JUMBO_M	  0x0f
#define   MAC_RX_PKT_LEN_S	  24
#define   MAC_RX_PKT_LEN_M	  0x03
#define   MTCC_LMT_S		  20
#define   MTCC_LMT_M		  0x0f
#define   IPG_CFG_S		  18
#define   IPG_CFG_M		  0x03
#define   MAC_MODE_S		  16
#define   MAC_MODE_M		  0x01
#define     MAC_MODE_PHY	    0
#define     MAC_MODE_MAC	    1
#define   FORCE_MODE_S		  15
#define   FORCE_MODE_M		  0x01
#define   MAC_TX_EN_S		  14
#define   MAC_TX_EN_M		  0x01
#define   MAC_RX_EN_S		  13
#define   MAC_RX_EN_M		  0x01
#define   PRMBL_LMT_EN_S	  10
#define   PRMBL_LMT_EN_M	  0x01
#define   BKOFF_EN_S		  9
#define   BKOFF_EN_M		  0x01
#define   BACKPR_EN_S		  8
#define   BACKPR_EN_M		  0x01
#define   FORCE_EEE1G_S		  7
#define   FORCE_EEE1G_M		  0x01
#define   FORCE_EEE100_S	  6
#define   FORCE_EEE100_M	  0x01
#define   FORCE_RX_FC_S		  5
#define   FORCE_RX_FC_M		  0x01
#define   FORCE_TX_FC_S		  4
#define   FORCE_TX_FC_M		  0x01
#define   FORCE_SPD_S		  2
#define   FORCE_SPD_M		  0x03
#define     SPEED_10M		    0
#define     SPEED_100M		    1
#define     SPEED_1000M		    2
#define   FORCE_DPX_S		  1
#define   FORCE_DPX_M		  0x01
#define   FORCE_LINK_S		  0
#define   FORCE_LINK_M		  0x01

#define GMAC_PORT_EEE(p)	(0x0104 + (p) * 0x100)
#define   WAKEUP_TIME_1000_S	  24
#define   WAKEUP_TIME_1000_M	  0xff
#define   WAKEUP_TIME_100_S	  16
#define   WAKEUP_TIME_100_M	  0xff
#define   LPI_THRESH_S		  4
#define   LPI_THRESH_M		  0xfff
#define   LPI_MODE_S		  0
#define   LPI_MODE_M		  0x01

#define GMAC_PORT_SR(p)		(0x0108 + (p) * 0x100)
#define   EEE1G_STATUS_S	  7
#define   EEE1G_STATUS_M	  0x01
#define   EEE100_STATUS_S	  6
#define   EEE100_STATUS_M	  0x01
#define   RX_FC_STATUS_S	  5
#define   RX_FC_STATUS_M	  0x01
#define   TX_FC_STATUS_S	  4
#define   TX_FC_STATUS_M	  0x01
#define   SPD_STATUS_S		  2
#define   SPD_STATUS_M		  0x03
#define   DPX_STATUS_S		  1
#define   DPX_STATUS_M		  0x01
#define   LINK_STATUS_S		  0
#define   LINK_STATUS_M		  0x01


/* MT7530 Internal Register Access */
#define MT7530_REG_PAGE_ADDR_S	6
#define MT7530_REG_PAGE_ADDR_M	0x3ff
#define MT7530_REG_ADDR_S	2
#define MT7530_REG_ADDR_M	0x0f


/* MT7530 Registers */
#define PCR_REG(p)			(0x2004 + (0x100 * (p)))
#define   MLDV2_EN_S			  30
#define   MLDV2_EN_M			  0x01
#define   EG_TAG_S			  28
#define   EG_TAG_M			  0x03
#define   PORT_PRI_S			  24
#define   PORT_PRI_M			  0x07
#define   PORT_MATRIX_S			  16
#define   PORT_MATRIX_M			  0xff
#define   UP2DSCP_EN_S			  12
#define   UP2DSCP_EN_M			  0x01
#define   UP2TAG_EN_S			  11
#define   UP2TAG_EN_M			  0x01
#define   ACL_EN_S			  10
#define   ACL_EN_M			  0x01
#define   PORT_TX_MIR_S			  9
#define   PORT_TX_MIR_M			  0x01
#define   PORT_RX_MIR_S			  8
#define   PORT_RX_MIR_M			  0x01
#define   ACL_MIR_S			  7
#define   ACL_MIR_M			  0x01
#define   MIS_PORT_FW_S			  4
#define   MIS_PORT_FW_M			  0x07
#define   VLAN_MIS_S			  2
#define   VLAN_MIS_M			  0x01
#define   PORT_VLAN_S			  0
#define   PORT_VLAN_M			  0x03
#define     PORT_VLAN_MATRIX		    0
#define     PORT_VLAN_FALLBAKC		    1
#define     PORT_VLAN_CHECK		    2
#define     PORT_VLAN_SECURITY		    3

#define PVC_REG(p)			(0x2010 + (0x100 * (p)))
#define   STAG_VPID_S			  16
#define   STAG_VPID_M			  0xffff
#define   DIS_PVID_S			  15
#define   DIS_PVID_M			  0x01
#define   FORCE_PVID_S			  14
#define   FORCE_PVID_M			  0x01
#define   PT_VPM_S			  12
#define   PT_VPM_M			  0x01
#define   PT_OPTION_S			  11
#define   PT_OPTION_M			  0x01
#define   PVC_EG_TAG_S			  8
#define   PVC_EG_TAG_M			  0x07
#define   VLAN_ATTR_S			  6
#define   VLAN_ATTR_M			  0x03
#define     VLAN_ATTR_USER		    0
#define     VLAN_ATTR_STACK		    1
#define     VLAN_ATTR_TRANSLATION	    2
#define     VLAN_ATTR_TRANSPARENT	    3
#define   PORT_STAG_S			  5
#define   PORT_STAG_M			  0x01
#define   BC_LKYV_EN_S			  4
#define   BC_LKYV_EN_M			  0x01
#define   MC_LKYV_EN_S			  3
#define   MC_LKYV_EN_M			  0x01
#define   UC_LKYV_EN_S			  2
#define   UC_LKYV_EN_M			  0x01
#define   ACC_FRM_S			  0
#define   ACC_FRM_M			  0x03

#define PCMR_REG(p)			(0x3000 + (0x100 * (p)))
#define   EXT_PHY_S			  17
#define   EXT_PHY_M			  0x01
#define   MAC_PRE_S			  11
#define   MAC_PRE_M			  0x01
/* XXX: other fields are defined under GMAC_PORT_MCR */

#define PMEEECR_REG(p)			(0x3004 + (0x100 * (p)))
/* XXX: all fields are the same as which defined under GMAC_PORT_EEE */

#define PMSR_REG(p)			(0x3008 + (0x100 * (p)))
/* XXX: all fields are the same as which defined under GMAC_PORT_SR */

#define PINT_EN_REG(p)			(0x3010 + (0x100 * (p)))
#define PINT_STS_REG(p)			(0x3014 + (0x100 * (p)))
#define   TX_TFF_UNDR_INT_S		  15
#define   TX_TFF_UNDR_INT_M		  0x01
#define   TX_MISVLAN_ERR_INT_S		  14
#define   TX_MISVLAN_ERR_INT_M		  0x01
#define   TX_MISPAGE_ERR_INT_S		  13
#define   TX_MISPAGE_ERR_INT_M		  0x01
#define   TX_RPAGE_ERR_INT_S		  12
#define   TX_RPAGE_ERR_INT_M		  0x01
#define   TX_RPAGE_TOUT_INT_S		  11
#define   TX_RPAGE_TOUT_INT_M		  0x01
#define   TX_GPAGE_TOUT_INT_S		  10
#define   TX_GPAGE_TOUT_INT_M		  0x01
#define   TX_RDPB_TOUT_INT_S		  9
#define   TX_RDPB_TOUT_INT_M		  0x01
#define   TX_DEQ_TOUT_INT_S		  8
#define   TX_DEQ_TOUT_INT_M		  0x01
#define   RX_AFF_FULL_INT_S		  3
#define   RX_AFF_FULL_INT_M		  0x01
#define   RX_ARL_TOUT_INT_S		  2
#define   RX_ARL_TOUT_INT_M		  0x01
#define   RX_WRPB_TOUT_INT_S		  1
#define   RX_WRPB_TOUT_INT_M		  0x01
#define   RX_GPAGE_TOUT_INT_S		  0
#define   RX_GPAGE_TOUT_INT_M		  0x01

#define SYS_CTRL_REG			0x7000
#define   ACL_TAB_INIT_S		  22
#define   ACL_TAB_INIT_M		  0x01
#define   MAC_TAB_INIT_S		  21
#define   MAC_TAB_INIT_M		  0x01
#define   VLAN_TAB_INIT_S		  20
#define   VLAN_TAB_INIT_M		  0x01
#define   BMU_MEM_INIT_S		  16
#define   BMU_MEM_INIT_M		  0x01
#define   TRTCM_BIST_STS_S		  14
#define   TRTCM_BIST_STS_M		  0x01
#define   MASK_BIST_STS_S		  13
#define   MASK_BIST_STS_M		  0x01
#define   CTRL_BIST_STS_S		  12
#define   CTRL_BIST_STS_M		  0x01
#define   ADDR_BIST_STS_S		  11
#define   ADDR_BIST_STS_M		  0x01
#define   VLN_BIST_STS_S		  10
#define   VLN_BIST_STS_M		  0x01
#define   MIB_BIST_STS_S		  9
#define   MIB_BIST_STS_M		  0x01
#define   PB_BIST_STS_S			  8
#define   PB_BIST_STS_M			  0x01
#define   PL_BIST_STS_S			  7
#define   PL_BIST_STS_M			  0x01
#define   FL_BIST_STS_S			  6
#define   FL_BIST_STS_M			  0x01
#define   MBIST_CMP_S			  5
#define   MBIST_CMP_M			  0x01
#define   MBIST_EN_S			  4
#define   MBIST_EN_M			  0x01
#define   SW_PHY_RST_S			  2
#define   SW_PHY_RST_M			  0x01
#define   SW_SYS_RST_S			  1
#define   SW_SYS_RST_M			  0x01
#define   SW_REG_RST_S			  0
#define   SW_REG_RST_M			  0x01

#define HWTRAP_REG			0x7800
#define MHWTRAP_REG			0x7804
#define   BOND_OPTION_S			  24
#define   BOND_OPTION_M			  0x01
#define   P5_PHY0_SEL_S			  20
#define   P5_PHY0_SEL_M			  0x01
#define   CHG_TRAP_S			  16
#define   CHG_TRAP_M			  0x01
#define   LOOPDET_DIS_S			  14
#define   LOOPDET_DIS_M			  0x01
#define   P5_INTF_SEL_S			  13
#define   P5_INTF_SEL_M			  0x01
#define     P5_INTF_SEL_GPHY		    0
#define     P5_INTF_SEL_GMAC5		    1
#define   SMI_ADDR_S			  11
#define   SMI_ADDR_M			  0x03
#define   XTAL_FSEL_S			  9
#define   XTAL_FSEL_M			  0x03
#define   P6_INTF_DIS_S			  8
#define   P6_INTF_DIS_M			  0x01
#define   P5_INTF_MODE_S		  7
#define   P5_INTF_MODE_M		  0x01
#define     P5_INTF_MODE_GMII_MII	    0
#define     P5_INTF_MODE_RGMII		    1
#define   P5_INTF_DIS_S			  6
#define   P5_INTF_DIS_M			  0x01
#define   C_MDIO_BPS_S			  5
#define   C_MDIO_BPS_M			  0x01
#define   EEPROM_EN_S			  4
#define   EEPROM_EN_M			  0x01
#define   CHIP_MODE_S			  0
#define   CHIP_MODE_M			  0x0f

#define TRGMII_TD0_CTRL_REG		0x7a50
#define TRGMII_TD1_CTRL_REG		0x7a58
#define TRGMII_TD2_CTRL_REG		0x7a60
#define TRGMII_TD3_CTRL_REG		0x7a68
#define TRGMII_TXCTL_CTRL_REG		0x7a70
#define TRGMII_TCK_CTRL_REG		0x7a78
#define   TX_DMPEDRV_S			  31
#define   TX_DMPEDRV_M			  0x01
#define   TX_DM_SR_S			  15
#define   TX_DM_SR_M			  0x01
#define   TX_DMERODT_S			  14
#define   TX_DMERODT_M			  0x01
#define   TX_DMOECTL_S			  13
#define   TX_DMOECTL_M			  0x01
#define   TX_TAP_S			  8
#define   TX_TAP_M			  0x0f
#define   TX_TRAIN_WD_S			  0
#define   TX_TRAIN_WD_M			  0xff

#define TRGMII_TD0_ODT_REG		0x7a54
#define TRGMII_TD1_ODT_REG		0x7a5c
#define TRGMII_TD2_ODT_REG		0x7a64
#define TRGMII_TD3_ODT_REG		0x7a6c
#define TRGMII_TXCTL_ODT_REG		0x7574
#define TRGMII_TCK_ODT_REG		0x757c
#define   TX_DM_DRVN_PRE_S		  30
#define   TX_DM_DRVN_PRE_M		  0x03
#define   TX_DM_DRVP_PRE_S		  28
#define   TX_DM_DRVP_PRE_M		  0x03
#define   TX_DM_TDSEL_S			  24
#define   TX_DM_TDSEL_M			  0x0f
#define   TX_ODTEN_S			  23
#define   TX_ODTEN_M			  0x01
#define   TX_DME_PRE_S			  20
#define   TX_DME_PRE_M			  0x01
#define   TX_DM_DRVNT0_S		  19
#define   TX_DM_DRVNT0_M		  0x01
#define   TX_DM_DRVPT0_S		  18
#define   TX_DM_DRVPT0_M		  0x01
#define   TX_DM_DRVNTE_S		  17
#define   TX_DM_DRVNTE_M		  0x01
#define   TX_DM_DRVPTE_S		  16
#define   TX_DM_DRVPTE_M		  0x01
#define   TX_DM_ODTN_S			  12
#define   TX_DM_ODTN_M			  0x07
#define   TX_DM_ODTP_S			  8
#define   TX_DM_ODTP_M			  0x07
#define   TX_DM_DRVN_S			  4
#define   TX_DM_DRVN_M			  0x0f
#define   TX_DM_DRVP_S			  0
#define   TX_DM_DRVP_M			  0x0f


/* MT7530 GPHY MII Registers */
#define MII_MMD_ACC_CTL_REG		0x0d
#define   MMD_OP_MODE_S			  14
#define   MMD_OP_MODE_M			  0x03
#define     MMD_ADDR			    0
#define     MMD_DATA			    1
#define     MMD_DATA_RW_POST_INC	    2
#define     MMD_DATA_W_POST_INC		    3
#define   MMD_DEVAD_S			  0
#define   MMD_DEVAD_M			  0x1f

#define MII_MMD_ADDR_DATA_REG		0x0e


/* MT7530 GPHY MDIO MMD Registers */
#define MMD_7_EEE_ADV_REG		0x003c
#define   ADV_EEE_10G_KR_S		  6
#define   ADV_EEE_10G_KR_M		  0x01
#define   ADV_EEE_10G_KX4_S		  5
#define   ADV_EEE_10G_KX4_M		  0x01
#define   ADV_EEE_1G_KX_S		  4
#define   ADV_EEE_1G_KX_M		  0x01
#define   ADV_EEE_10G_T_S		  3
#define   ADV_EEE_10G_T_M		  0x01
#define   ADV_EEE_1G_T_PRE_S		  2
#define   ADV_EEE_1G_T_PRE_M		  0x01
#define   ADV_EEE_100M_TX_PRE_S		  1
#define   ADV_EEE_100M_TX_PRE_M		  0x01

#define MMD_7_EEE_LP_ADV_REG		0x003d
#define   LP_EEE_10G_KR_S		  6
#define   LP_EEE_10G_KR_M		  0x01
#define   LP_EEE_10G_KX4_S		  5
#define   LP_EEE_10G_KX4_M		  0x01
#define   LP_EEE_1G_KX_S		  4
#define   LP_EEE_1G_KX_M		  0x01
#define   LP_EEE_10G_T_S		  3
#define   LP_EEE_10G_T_M		  0x01
#define   LP_EEE_1G_T_S			  2
#define   LP_EEE_1G_T_M			  0x01
#define   LP_EEE_100M_TX_S		  1
#define   LP_EEE_100M_TX_M		  0x01

#define MMD_1F_10M_DRV_1_REG		0x027b
#define   CR_RG_TX_CM_10M_S		  12
#define   CR_RG_TX_CM_10M_M		  0x03
#define   CR_RG_DELAY_SEL_TX_10M_S	  8
#define   CR_RG_DELAY_SEL_TX_10M_M	  0x03
#define   CR_DA_TX_GAIN_10M_EEE_S	  4
#define   CR_DA_TX_GAIN_10M_EEE_M	  0x07
#define   CR_DA_TX_GAIN_10M_S		  0
#define   CR_DA_TX_GAIN_10M_M		  0x07

#define MMD_1F_10M_DRV_2_REG		0x027c
#define   CR_DA_TX_I2MPB_10M_S		  8
#define   CR_DA_TX_I2MPB_10M_M		  0x1f
#define   CR_DA_TX_I2MPB_10M_LP_S	  0
#define   CR_DA_TX_I2MPB_10M_LP_M	  0x1f

#define MMD_1F_GSWPLL_GCR_1_REG		0x040d
#define   GSWPLL_PREDIV_S		  14
#define   GSWPLL_PREDIV_M		  0x03
#define   GSWPLL_POSTDIV_200M_S		  12
#define   GSWPLL_POSTDIV_200M_M		  0x03
#define   GSWPLL_EN_PRE_S		  11
#define   GSWPLL_EN_PRE_M		  0x01
#define   GSWPLL_FBKSEL_S		  10
#define   GSWPLL_FBKSEL_M		  0x01
#define   GSWPLL_BP_S			  9
#define   GSWPLL_BP_M			  0x01
#define   GSWPLL_BR_S			  8
#define   GSWPLL_BR_M			  0x01
#define   GSWPLL_FBKDIV_200M_S		  0
#define   GSWPLL_FBKDIV_200M_M		  0xff

#define MMD_1F_GSWPLL_GCR_2_REG		0x040e
#define   GSWPLL_POSTDIV_500M_S		  8
#define   GSWPLL_POSTDIV_500M_M		  0x03
#define   GSWPLL_FBKDIV_500M_S		  0
#define   GSWPLL_FBKDIV_500M_M		  0xff

#define MMD_1F_TRGMII_GSW_CLK_CG_REG	0x0410
#define   TRGMIICK_EN_S			  1
#define   TRGMIICK_EN_M			  0x01
#define   GSWCK_EN_S			  0
#define   GSWCK_EN_M			  0x01


#endif /* _MT7621_ETH_H_ */
