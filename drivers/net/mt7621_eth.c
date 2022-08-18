// SPDX-License-Identifier:	GPL-2.0+
/*
 * Copyright (C) 2018 MediaTek Incorporation. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <common.h>
#include <dm.h>
#include <miiphy.h>
#include <malloc.h>
#include <reset.h>
#include <wait_bit.h>
#include <linux/err.h>
#include <linux/mii.h>
#include <linux/mdio.h>
#include <linux/ioport.h>
#include <asm/io.h>
#include <asm/addrspace.h>

#include "mt7621_eth.h"

DECLARE_GLOBAL_DATA_PTR;

#define NUM_TX_DESC		24
#define NUM_RX_DESC		24
#define TX_TOTAL_BUF_SIZE	(NUM_TX_DESC * PKTSIZE_ALIGN)
#define RX_TOTAL_BUF_SIZE	(NUM_RX_DESC * PKTSIZE_ALIGN)
#define TOTAL_PKT_BUF_SIZE	(TX_TOTAL_BUF_SIZE + RX_TOTAL_BUF_SIZE)

#define MT7530_NUM_PHYS		5

#define GDMA_FWD_TO_CPU \
	(0x20000000 | \
	REG_SET_VAL(GDM_ICS_EN, 1) | \
	REG_SET_VAL(GDM_TCS_EN, 1) | \
	REG_SET_VAL(GDM_UCS_EN, 1) | \
	REG_SET_VAL(STRP_CRC, 1) | \
	REG_SET_VAL(MYMAC_DP, DP_PDMA) | \
	REG_SET_VAL(BC_DP, DP_PDMA) | \
	REG_SET_VAL(MC_DP, DP_PDMA) | \
	REG_SET_VAL(UN_DP, DP_PDMA))

#define GDMA_FWD_DISCARD \
	(0x20000000 | \
	REG_SET_VAL(GDM_ICS_EN, 1) | \
	REG_SET_VAL(GDM_TCS_EN, 1) | \
	REG_SET_VAL(GDM_UCS_EN, 1) | \
	REG_SET_VAL(STRP_CRC, 1) | \
	REG_SET_VAL(MYMAC_DP, DP_DISCARD) | \
	REG_SET_VAL(BC_DP, DP_DISCARD) | \
	REG_SET_VAL(MC_DP, DP_DISCARD) | \
	REG_SET_VAL(UN_DP, DP_DISCARD))

typedef struct _PDMA_RXD_INFO1_ {
	unsigned int    PDP0;
} PDMA_RXD_INFO1_T;

typedef struct _PDMA_RXD_INFO2_ {
	unsigned int    PLEN1 : 14;
	unsigned int    LS1 : 1;
	unsigned int    UN_USED : 1;
	unsigned int    PLEN0 : 14;
	unsigned int    LS0 : 1;
	unsigned int    DDONE : 1;
} PDMA_RXD_INFO2_T;

typedef struct _PDMA_RXD_INFO3_ {
	unsigned int    PDP1;
} PDMA_RXD_INFO3_T;

typedef struct _PDMA_RXD_INFO4_ {
	unsigned int    FOE_Entry : 14;
	unsigned int    CRSN : 5;
	unsigned int    SP : 3;
	unsigned int    L4F : 1;
	unsigned int    L4VLD : 1;
	unsigned int    TACK : 1;
	unsigned int    IP4F : 1;
	unsigned int    IP4 : 1;
	unsigned int    IP6 : 1;
	unsigned int    UN_USED : 4;
} PDMA_RXD_INFO4_T;

typedef struct PDMA_rxdesc {
	PDMA_RXD_INFO1_T rxd_info1;
	PDMA_RXD_INFO2_T rxd_info2;
	PDMA_RXD_INFO3_T rxd_info3;
	PDMA_RXD_INFO4_T rxd_info4;
} PDMA_rxdesc;

typedef struct _PDMA_TXD_INFO1_ {
	unsigned int    SDP0;
} PDMA_TXD_INFO1_T;

typedef struct _PDMA_TXD_INFO2_ {
	unsigned int    SDL1 : 14;
	unsigned int    LS1 : 1;
	unsigned int    BURST : 1;
	unsigned int    SDL0 : 14;
	unsigned int    LS0 : 1;
	unsigned int    DDONE : 1;
} PDMA_TXD_INFO2_T;

typedef struct _PDMA_TXD_INFO3_ {
	unsigned int    SDP1;
} PDMA_TXD_INFO3_T;

typedef struct _PDMA_TXD_INFO4_ {
	unsigned int    VLAN_TAG : 16;
	unsigned int    INS : 1;
	unsigned int    RESV : 2;
	unsigned int    UDF : 6;
	unsigned int    FPORT : 3;
	unsigned int    TSO : 1;
	unsigned int    TUI_CO : 3;
} PDMA_TXD_INFO4_T;

typedef struct PDMA_txdesc {
	PDMA_TXD_INFO1_T txd_info1;
	PDMA_TXD_INFO2_T txd_info2;
	PDMA_TXD_INFO3_T txd_info3;
	PDMA_TXD_INFO4_T txd_info4;
} PDMA_txdesc;

struct mt7621_eth_priv {
	char pkt_pool[TOTAL_PKT_BUF_SIZE] __aligned(ARCH_DMA_MINALIGN);

	PDMA_txdesc tx_ring[NUM_TX_DESC] __aligned(ARCH_DMA_MINALIGN);
	PDMA_rxdesc rx_ring[NUM_RX_DESC] __aligned(ARCH_DMA_MINALIGN);

	PDMA_txdesc *tx_ring_noc;
	PDMA_rxdesc *rx_ring_noc;

	int rx_dma_owner_idx0;
	int tx_cpu_owner_idx0;

	void __iomem *fe_base;
	void __iomem *gmac_base;

	struct mii_dev *mdio_bus;

#if CONFIG_IS_ENABLED(DM_RESET)
	struct reset_ctl_bulk rst;
#endif
};

static int mt7621_eth_free_pkt(struct udevice *dev, uchar *packet, int length);

static void mt7621_pdma_write(struct mt7621_eth_priv *priv, u32 reg, u32 val)
{
	writel(val, priv->fe_base + PDMA_BASE + reg);
}

static void mt7621_pdma_rmw(struct mt7621_eth_priv *priv, u32 reg, u32 clr,
			    u32 set)
{
	clrsetbits_le32(priv->fe_base + PDMA_BASE + reg, clr, set);
}

static void mt7621_gdma_write(struct mt7621_eth_priv *priv, int no, u32 reg,
				u32 val)
{
	u32 gdma_base;

	if (no == 1)
		gdma_base = GDMA2_BASE;
	else
		gdma_base = GDMA1_BASE;

	writel(val, priv->fe_base + gdma_base + reg);
}

static u32 mt7621_gmac_read(struct mt7621_eth_priv *priv, u32 reg)
{
	u32 val = readl(priv->gmac_base + reg);
	return val;
}

static void mt7621_gmac_write(struct mt7621_eth_priv *priv, u32 reg, u32 val)
{
	writel(val, priv->gmac_base + reg);
}

static u32 mt7621_mdio_rw(struct mt7621_eth_priv *priv, u32 phy, u32 reg,
			  u32 data, u32 cmd)
{
	int ret;
	u32 val;

	val = REG_SET_VAL(MDIO_ST, 1) |
		REG_SET_VAL(MDIO_CMD, cmd) |
		REG_SET_VAL(MDIO_PHY_ADDR, phy) |
		REG_SET_VAL(MDIO_REG_ADDR, reg);

	if (cmd == MDIO_CMD_WRITE)
		val |= REG_SET_VAL(MDIO_RW_DATA, data);

	mt7621_gmac_write(priv, GMAC_PIAC_REG, val);
	mt7621_gmac_write(priv, GMAC_PIAC_REG, val | REG_MASK(PHY_ACS_ST));

	ret = wait_for_bit_le32(priv->gmac_base + GMAC_PIAC_REG,
		REG_MASK(PHY_ACS_ST), 0, 5000, 0);
	if (ret)
		return ret;

	if (cmd == MDIO_CMD_READ) {
		val = mt7621_gmac_read(priv, GMAC_PIAC_REG);
		return REG_GET_VAL(MDIO_RW_DATA, val);
	}

	return 0;
}

static int mt7621_mdio_read(struct mii_dev *bus, int addr, int devad, int reg)
{
	struct mt7621_eth_priv *priv = bus->priv;
	int ret;

	if (devad < 0)
		return mt7621_mdio_rw(priv, addr, reg, 0, MDIO_CMD_READ);

	ret = mt7621_mdio_rw(priv, addr, MII_MMD_ACC_CTL_REG,
		REG_SET_VAL(MMD_OP_MODE, MMD_ADDR) |
		REG_SET_VAL(MMD_DEVAD, devad),
		MDIO_CMD_WRITE);
	if (ret)
		return ret;

	ret = mt7621_mdio_rw(priv, addr, MII_MMD_ADDR_DATA_REG,
		reg, MDIO_CMD_WRITE);
	if (ret)
		return ret;

	ret = mt7621_mdio_rw(priv, addr, MII_MMD_ACC_CTL_REG,
		REG_SET_VAL(MMD_OP_MODE, MMD_DATA) |
		REG_SET_VAL(MMD_DEVAD, devad),
		MDIO_CMD_WRITE);
	if (ret)
		return ret;

	return mt7621_mdio_rw(priv, addr, MII_MMD_ADDR_DATA_REG,
		0, MDIO_CMD_READ);
}

static int mt7621_mdio_write(struct mii_dev *bus, int addr, int devad, int reg,
	u16 val)
{
	struct mt7621_eth_priv *priv = bus->priv;
	int ret;

	if (devad < 0)
		return mt7621_mdio_rw(priv, addr, reg, val, MDIO_CMD_WRITE);

	ret = mt7621_mdio_rw(priv, addr, MII_MMD_ACC_CTL_REG,
		REG_SET_VAL(MMD_OP_MODE, MMD_ADDR) |
		REG_SET_VAL(MMD_DEVAD, devad),
		MDIO_CMD_WRITE);
	if (ret)
		return ret;

	ret = mt7621_mdio_rw(priv, addr, MII_MMD_ADDR_DATA_REG,
		reg, MDIO_CMD_WRITE);
	if (ret)
		return ret;

	ret = mt7621_mdio_rw(priv, addr, MII_MMD_ACC_CTL_REG,
		REG_SET_VAL(MMD_OP_MODE, MMD_DATA) |
		REG_SET_VAL(MMD_DEVAD, devad),
		MDIO_CMD_WRITE);
	if (ret)
		return ret;

	return mt7621_mdio_rw(priv, addr, MII_MMD_ADDR_DATA_REG,
		val, MDIO_CMD_WRITE);
}

static int mt7621_mdio_register(struct udevice *dev)
{
	struct mt7621_eth_priv *priv = dev_get_priv(dev);
	struct mii_dev *mdio_bus = mdio_alloc();
	int ret;

	if (!mdio_bus)
		return -ENOMEM;

	mdio_bus->read = mt7621_mdio_read;
	mdio_bus->write = mt7621_mdio_write;
	snprintf(mdio_bus->name, sizeof(mdio_bus->name), dev->name);

	mdio_bus->priv = (void *)priv;

	ret = mdio_register(mdio_bus);

	if (ret)
		return ret;

	priv->mdio_bus = mdio_bus;

	return 0;
}

/*
 * MT7530 Internal Register Address Bits
 * ---------------------------------------------------------------------------------
 * | 15 | 14 | 13 | 12 | 11 | 10 |  9 |  8 |  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0 |
 * |----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|
 * |  V |  V |  V |  V |  V |  V |  V |  V |  V |  V |  V |  V |  V |  V |  - |  - |
 * |-------------------------------------------------|-------------------|---------|
 * |                   Page Address                  |     Reg Addr.     |  Unused |
 * ---------------------------------------------------------------------------------
 */

static int mt7530_reg_read(struct mt7621_eth_priv *priv, u32 reg, u32 *data)
{
	int ret, low_word, high_word;

	/* Write page address */
	ret = mt7621_mdio_write(priv->mdio_bus, 0x1f, MDIO_DEVAD_NONE,
		0x1f, REG_GET_VAL(MT7530_REG_PAGE_ADDR, reg));
	if (ret)
		return ret;

	/* Read low word */
	low_word = mt7621_mdio_read(priv->mdio_bus, 0x1f, MDIO_DEVAD_NONE,
		REG_GET_VAL(MT7530_REG_ADDR, reg));
	if (low_word < 0)
		return low_word;

	/* Read high word */
	high_word = mt7621_mdio_read(priv->mdio_bus, 0x1f, MDIO_DEVAD_NONE,
		0x10);
	if (high_word < 0)
		return high_word;

	if (data)
		*data = ((u32) high_word << 16) | ((u32) low_word & 0xffff);

	return 0;
}

static int mt7530_reg_write(struct mt7621_eth_priv *priv, u32 reg, u32 data)
{
	int ret;

	/* Write page address */
	ret = mt7621_mdio_write(priv->mdio_bus, 0x1f, MDIO_DEVAD_NONE,
		0x1f, REG_GET_VAL(MT7530_REG_PAGE_ADDR, reg));
	if (ret)
		return ret;

	/* Write low word */
	ret = mt7621_mdio_write(priv->mdio_bus, 0x1f, MDIO_DEVAD_NONE,
		REG_GET_VAL(MT7530_REG_ADDR, reg), data & 0xffff);
	if (ret)
		return ret;

	/* Write high word */
	return mt7621_mdio_write(priv->mdio_bus, 0x1f, MDIO_DEVAD_NONE,
		0x10, data >> 16);
}

static void mt7530_setup(struct mt7621_eth_priv *priv)
{
	int i;
	u16 phy_val;
	u32 val;

	/* Turn off PHYs */
	for (i = 0; i < MT7530_NUM_PHYS; i++) {
		phy_val = mt7621_mdio_read(priv->mdio_bus,
			i, MDIO_DEVAD_NONE, MII_BMCR);
		phy_val |= BMCR_PDOWN;
		mt7621_mdio_write(priv->mdio_bus,
			i, MDIO_DEVAD_NONE, MII_BMCR, phy_val);
	}

	/* Force MAC link down before reset */
	mt7530_reg_write(priv, PCMR_REG(5),
		REG_SET_VAL(FORCE_MODE, 1));
	mt7530_reg_write(priv, PCMR_REG(6),
		REG_SET_VAL(FORCE_MODE, 1));

	/* MT7530 reset */
	mt7530_reg_write(priv, SYS_CTRL_REG,
		REG_SET_VAL(SW_SYS_RST, 1) |
		REG_SET_VAL(SW_REG_RST, 1));
	udelay(100);

	val = REG_SET_VAL(IPG_CFG, 1) |
		REG_SET_VAL(MAC_MODE, MAC_MODE_MAC) |
		REG_SET_VAL(FORCE_MODE, 1) |
		REG_SET_VAL(MAC_TX_EN, 1) |
		REG_SET_VAL(MAC_RX_EN, 1) |
		REG_SET_VAL(BKOFF_EN, 1) |
		REG_SET_VAL(BACKPR_EN, 1) |
		REG_SET_VAL(FORCE_SPD, SPEED_1000M) |
		REG_SET_VAL(FORCE_DPX, 1) |
		REG_SET_VAL(FORCE_LINK, 1);


	/* MT7621 GMAC1: Forced 1000M/FD, FC ON */
	mt7621_gmac_write(priv, GMAC_PORT_MCR(0),
		REG_SET_VAL(MAX_RX_JUMBO, 2) |
		REG_SET_VAL(FORCE_RX_FC, 1) |
		REG_SET_VAL(FORCE_TX_FC, 1) |
		val);

	/* MT7530 Port6: Forced 1000M/FD, FC disabled */
	mt7530_reg_write(priv, PCMR_REG(6), val);

	/* MT7530 Port5: Forced link down */
	mt7530_reg_write(priv, PCMR_REG(5),
		REG_SET_VAL(FORCE_MODE, 1));

	/* MT7621 GMAC2: Forced link down */
	mt7621_gmac_write(priv, GMAC_PORT_MCR(1),
		REG_SET_VAL(FORCE_MODE, 1));

	/* MT7621 GDMA1: Packets forward to CPU (PDMA) */
	mt7621_gdma_write(priv, 0, GDMA_IG_CTRL_REG, GDMA_FWD_TO_CPU);

	/* MT7621 GDMA2: Packets discard */
	mt7621_gdma_write(priv, 1, GDMA_IG_CTRL_REG, GDMA_FWD_DISCARD);

	/* Hardware Trap: Enable Port6, Disable Port5 */
	mt7530_reg_write(priv, MHWTRAP_REG,
		REG_SET_VAL(CHG_TRAP, 1) |
		REG_SET_VAL(LOOPDET_DIS, 1) |
		REG_SET_VAL(P5_INTF_SEL, P5_INTF_SEL_GMAC5) |
		REG_SET_VAL(SMI_ADDR, 0x03) |
		REG_SET_VAL(XTAL_FSEL, 2) |
		REG_SET_VAL(P5_INTF_MODE, P5_INTF_MODE_RGMII) |
		REG_SET_VAL(P5_INTF_DIS, 1) |
		REG_SET_VAL(CHIP_MODE, 0x0f));

	switch (gd->arch.xtal_clk) {
	case 40 * 1000 * 1000:
		/* Disable MT7530 core clock */
		mt7621_mdio_write(priv->mdio_bus,
			0, 0x1f, MMD_1F_TRGMII_GSW_CLK_CG_REG, 0);

		/* disable MT7530 PLL */
		mt7621_mdio_write(priv->mdio_bus,
			0, 0x1f, MMD_1F_GSWPLL_GCR_1_REG,
			REG_SET_VAL(GSWPLL_POSTDIV_200M, 2) |
			REG_SET_VAL(GSWPLL_FBKDIV_200M, 32));

		/* For MT7530 core clock = 500Mhz */
		mt7621_mdio_write(priv->mdio_bus,
			0, 0x1f, MMD_1F_GSWPLL_GCR_2_REG,
			REG_SET_VAL(GSWPLL_POSTDIV_500M, 1) |
			REG_SET_VAL(GSWPLL_FBKDIV_500M, 25));

		/* Enable MT7530 PLL */
		mt7621_mdio_write(priv->mdio_bus,
			0, 0x1f, MMD_1F_GSWPLL_GCR_1_REG,
			REG_SET_VAL(GSWPLL_POSTDIV_200M, 2) |
			REG_SET_VAL(GSWPLL_EN_PRE, 1) |
			REG_SET_VAL(GSWPLL_FBKDIV_200M, 32));

		udelay(20);

		/* Enable MT7530 core clock */
		mt7621_mdio_write(priv->mdio_bus,
			0, 0x1f, MMD_1F_TRGMII_GSW_CLK_CG_REG,
			REG_SET_VAL(GSWCK_EN, 1));
		break;
	case 20 * 1000 * 1000:
		break;
	case 25 * 1000 * 1000:
		break;
	}

	/* Tx Driving */
	mt7530_reg_write(priv, TRGMII_TD0_ODT_REG,
		REG_SET_VAL(TX_DM_DRVN, 4) | REG_SET_VAL(TX_DM_DRVP, 4));
	mt7530_reg_write(priv, TRGMII_TD1_ODT_REG,
		REG_SET_VAL(TX_DM_DRVN, 4) | REG_SET_VAL(TX_DM_DRVP, 4));
	mt7530_reg_write(priv, TRGMII_TD2_ODT_REG,
		REG_SET_VAL(TX_DM_DRVN, 4) | REG_SET_VAL(TX_DM_DRVP, 4));
	mt7530_reg_write(priv, TRGMII_TD3_ODT_REG,
		REG_SET_VAL(TX_DM_DRVN, 4) | REG_SET_VAL(TX_DM_DRVP, 4));
	mt7530_reg_write(priv, TRGMII_TXCTL_ODT_REG,
		REG_SET_VAL(TX_DM_DRVN, 4) | REG_SET_VAL(TX_DM_DRVP, 4));
	mt7530_reg_write(priv, TRGMII_TCK_ODT_REG,
		REG_SET_VAL(TX_DM_DRVN, 4) | REG_SET_VAL(TX_DM_DRVP, 4));

	/* Turn on PHYs */
	for (i = 0; i < MT7530_NUM_PHYS; i++) {
		phy_val = mt7621_mdio_read(priv->mdio_bus,
			i, MDIO_DEVAD_NONE, MII_BMCR);
		phy_val &= ~BMCR_PDOWN;
		mt7621_mdio_write(priv->mdio_bus,
			i, MDIO_DEVAD_NONE, MII_BMCR, phy_val);
	}

	/* Undocumented */
	mt7530_reg_read(priv, 0x7808, &val);
	val |= 3 << 16; /* Enable INTR */
	mt7530_reg_write(priv, 0x7808, val);

	/* Set port isolation */
	for (i = 0; i < 8; i++) {
		/* Set port matrix mode */
		if (i != 6)
			mt7530_reg_write(priv, PCR_REG(i),
				REG_SET_VAL(PORT_MATRIX, 0x40));
		else
			mt7530_reg_write(priv, PCR_REG(i),
				REG_SET_VAL(PORT_MATRIX, 0x3f));

		/* Set port mode to user port */
		mt7530_reg_write(priv, PVC_REG(i),
			REG_SET_VAL(STAG_VPID, 0x8100) |
			REG_SET_VAL(VLAN_ATTR, VLAN_ATTR_USER));
	}
}

static void mt7621_eth_fifo_init(struct mt7621_eth_priv *priv)
{
	int i;
	char *pkt_base = priv->pkt_pool;

	mt7621_pdma_rmw(priv, PDMA_GLO_CFG_REG, 0xffff0000, 0);
	udelay(500);

	memset(priv->tx_ring_noc, 0, NUM_TX_DESC * sizeof(PDMA_txdesc));
	memset(priv->rx_ring_noc, 0, NUM_RX_DESC * sizeof(PDMA_rxdesc));
	memset(priv->pkt_pool, 0, TOTAL_PKT_BUF_SIZE);

	priv->tx_ring_noc = (PDMA_txdesc *) UNCACHED_SDRAM(&priv->tx_ring);
	priv->rx_ring_noc = (PDMA_rxdesc *) UNCACHED_SDRAM(&priv->rx_ring);
	priv->rx_dma_owner_idx0 = 0;
	priv->tx_cpu_owner_idx0 = 0;

	for (i = 0; i < NUM_TX_DESC; i++) {
		priv->tx_ring_noc[i].txd_info2.LS0 = 1;
		priv->tx_ring_noc[i].txd_info2.DDONE = 1;
		priv->tx_ring_noc[i].txd_info4.FPORT = DP_GDMA1;

		priv->tx_ring_noc[i].txd_info1.SDP0 = virt_to_phys(pkt_base);
		pkt_base += PKTSIZE_ALIGN;
	}

	for (i = 0; i < NUM_RX_DESC; i++) {
		priv->rx_ring_noc[i].rxd_info2.PLEN0 = PKTSIZE_ALIGN;
		priv->rx_ring_noc[i].rxd_info1.PDP0 = virt_to_phys(pkt_base);
		pkt_base += PKTSIZE_ALIGN;
	}

	mt7621_pdma_write(priv, TX_BASE_PTR_REG(0),
		virt_to_phys(priv->tx_ring_noc));
	mt7621_pdma_write(priv, TX_MAX_CNT_REG(0), NUM_TX_DESC);
	mt7621_pdma_write(priv, TX_CTX_IDX_REG(0), priv->tx_cpu_owner_idx0);

	mt7621_pdma_write(priv, RX_BASE_PTR_REG(0),
		virt_to_phys(priv->rx_ring_noc));
	mt7621_pdma_write(priv, RX_MAX_CNT_REG(0), NUM_RX_DESC);
	mt7621_pdma_write(priv, RX_CRX_IDX_REG(0), NUM_RX_DESC - 1);

	mt7621_pdma_write(priv, PDMA_RST_IDX_REG,
		REG_SET_VAL(RST_DTX_IDX0, 1) | REG_SET_VAL(RST_DRX_IDX0, 1));
}

static int mt7621_eth_start(struct udevice *dev)
{
	struct mt7621_eth_priv *priv = dev_get_priv(dev);

	mt7621_eth_fifo_init(priv);

	mt7621_pdma_rmw(priv, PDMA_GLO_CFG_REG, 0,
		REG_SET_VAL(TX_WB_DDONE, 1) |
		REG_SET_VAL(RX_DMA_EN, 1) |
		REG_SET_VAL(TX_DMA_EN, 1));
	udelay(500);

	return 0;
}

static void mt7621_eth_stop(struct udevice *dev)
{
	struct mt7621_eth_priv *priv = dev_get_priv(dev);

	mt7621_pdma_rmw(priv, PDMA_GLO_CFG_REG,
		REG_SET_VAL(TX_WB_DDONE, 1) |
		REG_SET_VAL(RX_DMA_EN, 1) |
		REG_SET_VAL(TX_DMA_EN, 1), 0);
	udelay(500);

	wait_for_bit_le32(priv->fe_base + PDMA_BASE + PDMA_GLO_CFG_REG,
		REG_MASK(RX_DMA_BUSY) | REG_MASK(TX_DMA_BUSY), 0, 5000, 0);
}

static int mt7621_eth_write_hwaddr(struct udevice *dev)
{
	struct eth_pdata *pdata = dev_get_platdata(dev);
	struct mt7621_eth_priv *priv = dev_get_priv(dev);
	unsigned char *mac = pdata->enetaddr;
	u32 macaddr_lsb, macaddr_msb;

	macaddr_msb = ((u32) mac[0] << 8) | (u32) mac[1];
	macaddr_lsb = ((u32) mac[2] << 24) | ((u32) mac[3] << 16) |
		((u32) mac[4] << 8) | (u32) mac[5];

	mt7621_gdma_write(priv, 0, GDMA_MAC_MSB_REG, macaddr_msb);
	mt7621_gdma_write(priv, 0, GDMA_MAC_LSB_REG, macaddr_lsb);

	return 0;
}

static int mt7621_eth_send(struct udevice *dev, void *packet, int length)
{
	struct mt7621_eth_priv *priv = dev_get_priv(dev);
	void *pkt_base;

	if (!priv->tx_ring_noc[priv->tx_cpu_owner_idx0].txd_info2.DDONE) {
		printf("mt7621-eth: TX DMA descriptor ring is full\n");
		return -EPERM;
	}

	pkt_base = (void *) phys_to_virt(
		priv->tx_ring_noc[priv->tx_cpu_owner_idx0].txd_info1.SDP0);
	memcpy(pkt_base, packet, length);
	flush_dcache_range((u32) pkt_base, (u32) pkt_base + length);

	priv->tx_ring_noc[priv->tx_cpu_owner_idx0].txd_info2.SDL0 = length;

	priv->tx_ring_noc[priv->tx_cpu_owner_idx0].txd_info2.DDONE = 0;

	priv->tx_cpu_owner_idx0 = (priv->tx_cpu_owner_idx0 + 1) % NUM_TX_DESC;
	mt7621_pdma_write(priv, TX_CTX_IDX_REG(0), priv->tx_cpu_owner_idx0);

	return 0;
}

static int mt7621_eth_recv(struct udevice *dev, int flags, uchar **packetp)
{
	struct mt7621_eth_priv *priv = dev_get_priv(dev);
	uchar *pkt_base;
	u32 length;

	if (!priv->rx_ring_noc[priv->rx_dma_owner_idx0].rxd_info2.DDONE) {
		debug("mt7621-eth: RX DMA descriptor ring is empty\n");
		return -EAGAIN;
	}

	length = priv->rx_ring_noc[priv->rx_dma_owner_idx0].rxd_info2.PLEN0;
	pkt_base = (void *)phys_to_virt(
		priv->rx_ring_noc[priv->rx_dma_owner_idx0].rxd_info1.PDP0);
	invalidate_dcache_range((u32) pkt_base, (u32) pkt_base + length);

	if (packetp)
		*packetp = pkt_base;

	return length;
}

static int mt7621_eth_free_pkt(struct udevice *dev, uchar *packet, int length)
{
	struct mt7621_eth_priv *priv = dev_get_priv(dev);

	priv->rx_ring_noc[priv->rx_dma_owner_idx0].rxd_info2.DDONE = 0;
	priv->rx_ring_noc[priv->rx_dma_owner_idx0].rxd_info2.LS0 = 0;
	priv->rx_ring_noc[priv->rx_dma_owner_idx0].rxd_info2.PLEN0 =
		PKTSIZE_ALIGN;

	mt7621_pdma_write(priv, RX_CRX_IDX_REG(0), priv->rx_dma_owner_idx0);
	priv->rx_dma_owner_idx0 = (priv->rx_dma_owner_idx0 + 1) % NUM_RX_DESC;

	return 0;
}

static int mt7621_eth_probe(struct udevice *dev)
{
	struct mt7621_eth_priv *priv = dev_get_priv(dev);
	int ret;

#if CONFIG_IS_ENABLED(DM_RESET)
	/* Reset device */
	reset_assert_bulk(&priv->rst);
	udelay(200);
	reset_deassert_bulk(&priv->rst);
	mdelay(100);
#endif

	/* MDIO register */
	ret = mt7621_mdio_register(dev);
	if (ret)
		return ret;

	/* Stop possibly started DMA */
	mt7621_eth_stop(dev);

	/* Initialize MT7530 */
	mt7530_setup(priv);

	return 0;
}

static int mt7621_eth_remove(struct udevice *dev)
{
	struct mt7621_eth_priv *priv = dev_get_priv(dev);

	/* MDIO unregister */
	mdio_unregister(priv->mdio_bus);
	mdio_free(priv->mdio_bus);

	/* Stop possibly started DMA */
	mt7621_eth_stop(dev);

	return 0;
}

static int mt7621_eth_ofdata_to_platdata(struct udevice *dev)
{
	struct mt7621_eth_priv *priv = dev_get_priv(dev);
	struct eth_pdata *pdata = dev_get_platdata(dev);
	struct resource res;
	int ret;

	pdata->iobase = devfdt_get_addr(dev);
	pdata->phy_interface = PHY_INTERFACE_MODE_RGMII;

	/* Frame Engine Register Base */
	ret = dev_read_resource_byname(dev, "fe", &res);
	if (ret)
		return ret;

	priv->fe_base = ioremap(res.start, resource_size(&res));

	/* GMAC Register Base */
	ret = dev_read_resource_byname(dev, "gmac", &res);
	if (ret)
		return ret;

	priv->gmac_base = ioremap(res.start, resource_size(&res));

#if CONFIG_IS_ENABLED(DM_RESET)
	/* Reset controllers */
	ret = reset_get_bulk(dev, &priv->rst);
	if (ret)
		return ret;
#endif

	return 0;
}

static const struct udevice_id mt7621_eth_ids[] = {
	{ .compatible = "mediatek,mt7621-eth" },
	{}
};

static const struct eth_ops mt7621_eth_ops = {
	.start = mt7621_eth_start,
	.stop = mt7621_eth_stop,
	.send = mt7621_eth_send,
	.recv = mt7621_eth_recv,
	.free_pkt = mt7621_eth_free_pkt,
	.write_hwaddr = mt7621_eth_write_hwaddr,
};

U_BOOT_DRIVER(mt7621_eth) = {
	.name = "mt7621-eth",
	.id = UCLASS_ETH,
	.of_match = mt7621_eth_ids,
	.ofdata_to_platdata = mt7621_eth_ofdata_to_platdata,
	.platdata_auto_alloc_size = sizeof(struct eth_pdata),
	.probe = mt7621_eth_probe,
	.remove = mt7621_eth_remove,
	.ops = &mt7621_eth_ops,
	.priv_auto_alloc_size = sizeof(struct mt7621_eth_priv),
	.flags = DM_FLAG_ALLOC_PRIV_DMA,
};