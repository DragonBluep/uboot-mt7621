#include <common.h>
#include <malloc.h>
#include <stdbool.h>
#include <linux/ioport.h>
#include <linux/iopoll.h>

#include <dm.h>
#include <usb.h>

#include "xhci.h"

#define SSUSB_SIFSLV_BASE		0x10000

#define SSUSB_SIFSLV_V1_U2FREQ		0x100	/* shared by u2 phys */

#define SSUSB_SIFSLV_V1_SPLLC		0x000	/* shared by u3 phys */
#define SSUSB_SIFSLV_V1_U3PHYD		0x000
#define SSUSB_SIFSLV_V1_U3PHYA		0x200

#define SSUSB_SIFSLV_V1_IPPC		0x700

/* HS port registers */
#define U3P_USBPHYACR0			0x000
#define PA0_RG_U2PLL_FORCE_ON		BIT(15)
#define PA0_RG_USB20_INTR_EN		BIT(5)

#define U3P_USBPHYACR5			0x014
#define PA5_RG_U2_HSTX_SRCAL_EN		BIT(15)
#define PA5_RG_U2_HSTX_SRCTRL		GENMASK(14, 12)
#define PA5_RG_U2_HSTX_SRCTRL_VAL(x)	((0x7 & (x)) << 12)
#define PA5_RG_U2_HS_100U_U3_EN		BIT(11)

#define U3P_USBPHYACR6			0x018
#define PA6_RG_U2_BC11_SW_EN		BIT(23)
#define PA6_RG_U2_OTG_VBUSCMP_EN	BIT(20)
#define PA6_RG_U2_SQTH			GENMASK(3, 0)
#define PA6_RG_U2_SQTH_VAL(x)		(0xf & (x))

#define U3P_U2PHYACR4			0x020
#define P2C_RG_USB20_GPIO_CTL		BIT(9)
#define P2C_USB20_GPIO_MODE		BIT(8)
#define P2C_U2_GPIO_CTR_MSK		(P2C_RG_USB20_GPIO_CTL | P2C_USB20_GPIO_MODE)

#define U3P_U2PHYDTM0			0x068
#define P2C_FORCE_UART_EN		BIT(26)
#define P2C_FORCE_DATAIN		BIT(23)
#define P2C_FORCE_DM_PULLDOWN		BIT(21)
#define P2C_FORCE_DP_PULLDOWN		BIT(20)
#define P2C_FORCE_XCVRSEL		BIT(19)
#define P2C_FORCE_SUSPENDM		BIT(18)
#define P2C_FORCE_TERMSEL		BIT(17)
#define P2C_RG_DATAIN			GENMASK(13, 10)
#define P2C_RG_DATAIN_VAL(x)		((0xf & (x)) << 10)
#define P2C_RG_DMPULLDOWN		BIT(7)
#define P2C_RG_DPPULLDOWN		BIT(6)
#define P2C_RG_XCVRSEL			GENMASK(5, 4)
#define P2C_RG_XCVRSEL_VAL(x)		((0x3 & (x)) << 4)
#define P2C_RG_SUSPENDM			BIT(3)
#define P2C_RG_TERMSEL			BIT(2)
#define P2C_DTM0_PART_MASK \
		(P2C_FORCE_DATAIN | P2C_FORCE_DM_PULLDOWN | \
		P2C_FORCE_DP_PULLDOWN | P2C_FORCE_XCVRSEL | \
		P2C_FORCE_TERMSEL | P2C_RG_DMPULLDOWN | \
		P2C_RG_DPPULLDOWN | P2C_RG_TERMSEL)

#define U3P_U2PHYDTM1			0x06C
#define P2C_RG_UART_EN			BIT(16)
#define P2C_RG_VBUSVALID		BIT(5)
#define P2C_RG_SESSEND			BIT(4)
#define P2C_RG_AVALID			BIT(2)

/* SSUSB_SIFSLV_V1_SPLLC registers */
#define U3P_SPLLC_XTALCTL3		0x018
#define XC3_RG_U3_XTAL_RX_PWD		BIT(9)
#define XC3_RG_U3_FRC_XTAL_RX_PWD	BIT(8)

/* SSUSB_SIFSLV_V1_U3PHYD */
#define U3P_U3_PHYD_LFPS1		0x00c
#define P3D_RG_FWAKE_TH			GENMASK(21, 16)
#define P3D_RG_FWAKE_TH_VAL(x)		((0x3f & (x)) << 16)

#define U3P_U3_PHYD_CDR1		0x05c
#define P3D_RG_CDR_BIR_LTD1		GENMASK(28, 24)
#define P3D_RG_CDR_BIR_LTD1_VAL(x)	((0x1f & (x)) << 24)
#define P3D_RG_CDR_BIR_LTD0		GENMASK(12, 8)
#define P3D_RG_CDR_BIR_LTD0_VAL(x)	((0x1f & (x)) << 8)

#define U3P_U3_PHYD_RXDET1		0x128
#define P3D_RG_RXDET_STB2_SET		GENMASK(17, 9)
#define P3D_RG_RXDET_STB2_SET_VAL(x)	((0x1ff & (x)) << 9)

#define U3P_U3_PHYD_RXDET2		0x12c
#define P3D_RG_RXDET_STB2_SET_P3	GENMASK(8, 0)
#define P3D_RG_RXDET_STB2_SET_P3_VAL(x)	(0x1ff & (x))

/* SSUSB_SIFSLV_V1_U3PHYA registers */
#define U3P_U3_PHYA_REG0		0x000
#define P3A_RG_CLKDRV_OFF		GENMASK(3, 2)
#define P3A_RG_CLKDRV_OFF_VAL(x)	((0x3 & (x)) << 2)
#define P3A_RG_SSUSB_VRT_VREF_SEL	GENMASK(9, 6)
#define P3A_RG_SSUSB_VRT_VREF_SEL_VAL(x)	((0xf & (x)) << 6)

#define U3P_U3_PHYA_REG6		0x018
#define P3A_RG_TX_EIDLE_CM		GENMASK(31, 28)
#define P3A_RG_TX_EIDLE_CM_VAL(x)	((0xf & (x)) << 28)

#define U3P_U3_PHYA_REG9		0x024
#define P3A_RG_RX_DAC_MUX		GENMASK(5, 1)
#define P3A_RG_RX_DAC_MUX_VAL(x)	((0x1f & (x)) << 1)

#define U3P_U3_PHYA_DA_REG0		0x100
#define P3A_RG_XTAL_EXT_PE2H		GENMASK(17, 16)
#define P3A_RG_XTAL_EXT_PE2H_VAL(x)	((0x3 & (x)) << 16)
#define P3A_RG_XTAL_EXT_PE1H		GENMASK(13, 12)
#define P3A_RG_XTAL_EXT_PE1H_VAL(x)	((0x3 & (x)) << 12)
#define P3A_RG_XTAL_EXT_EN_U3		GENMASK(11, 10)
#define P3A_RG_XTAL_EXT_EN_U3_VAL(x)	((0x3 & (x)) << 10)

/* SSUSB_SIFSLV_V1_IPPC registers */

#define SSUSB_IP_PW_CTRL0		0x00
#define CTRL0_IP_SW_RST			BIT(0)

#define SSUSB_IP_PW_CTRL1		0x04
#define CTRL1_IP_HOST_PDN		BIT(0)

#define SSUSB_IP_CAP			0x024
#define CAP_U3_PORT_NUM(p)		((p) & 0xff)
#define CAP_U2_PORT_NUM(p)		(((p) >> 8) & 0xff)

#define SSUSB_U3_CTRL(p)		(0x30 + (p * 0x08))
#define CTRL_U3_PORT_DIS		BIT(0)
#define CTRL_U3_PORT_PDN		BIT(1)

#define SSUSB_U2_CTRL(p)		(0x50 + (p * 0x08))
#define CTRL_U2_PORT_DIS		BIT(0)
#define CTRL_U2_PORT_PDN		BIT(1)

/* SSUSB_SIFSLV_V1_U2FREQ registers */
#define U3P_U2FREQ_FMCR0		0x00
#define P2F_RG_MONCLK_SEL		GENMASK(27, 26)
#define P2F_RG_MONCLK_SEL_VAL(x)	((0x3 & (x)) << 26)
#define P2F_RG_FREQDET_EN		BIT(24)
#define P2F_RG_CYCLECNT			GENMASK(23, 0)
#define P2F_RG_CYCLECNT_VAL(x)		((P2F_RG_CYCLECNT) & (x))

#define U3P_U2FREQ_VALUE		0x0c

#define U3P_U2FREQ_FMMONR1		0x10
#define P2F_USB_FM_VALID		BIT(0)
#define P2F_RG_FRCK_EN			BIT(8)

#define U3P_REF_CLK			26	/* MHZ */
#define U3P_SLEW_RATE_COEF		28
#define U3P_SR_COEF_DIVISOR		1000
#define U3P_FM_DET_CYCLE_CNT		1024

struct mt7621_usb_phy {
	uint32_t index;
	void __iomem *port_base;
};

struct mt7621_xhci_priv {
	struct xhci_ctrl ctrl;	/* Needs to come first in this struct! */

	void __iomem *membase;
	void __iomem *sifbase;

	uint32_t num_u2_ports;
	uint32_t num_u3_ports;

	struct mt7621_usb_phy *u2p;
	struct mt7621_usb_phy *u3p;
};

struct mt7621_xhci_pdata {
	void __iomem *membase;
};

struct mt7621_usb_phy_base {
	uint32_t base;
	bool is_u3p;
};

static const struct mt7621_usb_phy_base mt7621_usb_phy_bases[] = {
	{
		.base = 0x800
	}, {
		.base = 0x900,
		.is_u3p = true
	}, {
		.base = 0x1000
	}
};

static void xhci_mtk_host_enable(struct mt7621_xhci_priv *priv)
{
	void __iomem *ippc;
	uint32_t i;

	ippc = priv->sifbase + SSUSB_SIFSLV_V1_IPPC;

	setbits_32(ippc + SSUSB_IP_PW_CTRL0, CTRL0_IP_SW_RST);
	clrbits_32(ippc + SSUSB_IP_PW_CTRL0, CTRL0_IP_SW_RST);

	clrbits_32(ippc + SSUSB_IP_PW_CTRL1, CTRL1_IP_HOST_PDN);

	for (i = 0; i < priv->num_u2_ports; i++)
		clrbits_32(ippc + SSUSB_U2_CTRL(i),
			CTRL_U2_PORT_PDN | CTRL_U2_PORT_DIS);

	for (i = 0; i < priv->num_u3_ports; i++)
		clrbits_32(ippc + SSUSB_U3_CTRL(i),
			CTRL_U3_PORT_PDN | CTRL_U3_PORT_DIS);
}

static void xhci_mtk_host_disable(struct mt7621_xhci_priv *priv)
{
	void __iomem *ippc;
	uint32_t i;

	ippc = priv->sifbase + SSUSB_SIFSLV_V1_IPPC;

	for (i = 0; i < priv->num_u2_ports; i++)
		setbits_32(ippc + SSUSB_U2_CTRL(i),
			CTRL_U2_PORT_PDN | CTRL_U2_PORT_DIS);

	for (i = 0; i < priv->num_u3_ports; i++)
		setbits_32(ippc + SSUSB_U3_CTRL(i),
			CTRL_U3_PORT_PDN | CTRL_U3_PORT_DIS);

	setbits_32(ippc + SSUSB_IP_PW_CTRL1, CTRL1_IP_HOST_PDN);
}

static void u3_phy_instance_init(struct mt7621_xhci_priv *priv,
	struct mt7621_usb_phy *phy)
{
	void __iomem *spllc;
	void __iomem *phya;
	void __iomem *phyd;

	spllc = priv->sifbase + SSUSB_SIFSLV_V1_SPLLC;
	phya = phy->port_base + SSUSB_SIFSLV_V1_U3PHYA;
	phyd = phy->port_base + SSUSB_SIFSLV_V1_U3PHYD;

	/* gating PCIe Analog XTAL clock */
	setbits_32(spllc + U3P_SPLLC_XTALCTL3,
		XC3_RG_U3_XTAL_RX_PWD | XC3_RG_U3_FRC_XTAL_RX_PWD);

	/* gating XSQ */
	clrsetbits_32(phya + U3P_U3_PHYA_DA_REG0,
		P3A_RG_XTAL_EXT_EN_U3, P3A_RG_XTAL_EXT_EN_U3_VAL(2));

	clrsetbits_32(phya + U3P_U3_PHYA_REG9,
		P3A_RG_RX_DAC_MUX, P3A_RG_RX_DAC_MUX_VAL(4));

	clrsetbits_32(phya + U3P_U3_PHYA_REG6,
		P3A_RG_TX_EIDLE_CM, P3A_RG_TX_EIDLE_CM_VAL(0xe));

	clrsetbits_32(phya + U3P_U3_PHYA_REG0,
		P3A_RG_SSUSB_VRT_VREF_SEL, P3A_RG_SSUSB_VRT_VREF_SEL_VAL(0xa));

	clrsetbits_32(phyd + U3P_U3_PHYD_CDR1,
		P3D_RG_CDR_BIR_LTD0 | P3D_RG_CDR_BIR_LTD1,
		P3D_RG_CDR_BIR_LTD0_VAL(0xc) | P3D_RG_CDR_BIR_LTD1_VAL(0x3));

	clrsetbits_32(phyd + U3P_U3_PHYD_LFPS1,
		P3D_RG_FWAKE_TH, P3D_RG_FWAKE_TH_VAL(0x34));

	clrsetbits_32(phyd + U3P_U3_PHYD_RXDET1,
		P3D_RG_RXDET_STB2_SET, P3D_RG_RXDET_STB2_SET_VAL(0x10));

	clrsetbits_32(phyd + U3P_U3_PHYD_RXDET2,
		P3D_RG_RXDET_STB2_SET_P3, P3D_RG_RXDET_STB2_SET_P3_VAL(0x10));
}

static void u2_phy_instance_init(struct mt7621_xhci_priv *priv,
	struct mt7621_usb_phy *phy)
{
	/* switch to USB function. (system register, force ip into usb mode) */
	clrsetbits_32(phy->port_base + U3P_U2PHYDTM0,
		      P2C_FORCE_UART_EN,
		      P2C_RG_XCVRSEL_VAL(1) | P2C_RG_DATAIN_VAL(0));

	clrbits_32(phy->port_base + U3P_U2PHYDTM1, P2C_RG_UART_EN);

	setbits_32(phy->port_base + U3P_USBPHYACR0, PA0_RG_USB20_INTR_EN);

	/* disable switch 100uA current to SSUSB */
	clrbits_32(phy->port_base + U3P_USBPHYACR5, PA5_RG_U2_HS_100U_U3_EN);

	if (!phy->index)
		clrbits_32(phy->port_base + U3P_U2PHYACR4,
			   P2C_U2_GPIO_CTR_MSK);

	/* DP/DM BC1.1 path Disable */
	clrsetbits_32(phy->port_base + U3P_USBPHYACR6,
		PA6_RG_U2_BC11_SW_EN | PA6_RG_U2_SQTH, PA6_RG_U2_SQTH_VAL(2));
}

static void u2_phy_instance_power_on(struct mt7621_xhci_priv *priv,
	struct mt7621_usb_phy *phy)
{
	/* (force_suspendm=0) (let suspendm=1, enable usb 480MHz pll) */
	clrbits_32(phy->port_base + U3P_U2PHYDTM0,
		P2C_FORCE_SUSPENDM | P2C_RG_XCVRSEL |
		P2C_RG_DATAIN | P2C_DTM0_PART_MASK);

	/* OTG Enable */
	setbits_32(phy->port_base + U3P_USBPHYACR6, PA6_RG_U2_OTG_VBUSCMP_EN);

	clrsetbits_32(phy->port_base + U3P_U2PHYDTM1,
		P2C_RG_SESSEND, P2C_RG_VBUSVALID | P2C_RG_AVALID);
}

static void u2_phy_instance_power_off(struct mt7621_xhci_priv *priv,
	struct mt7621_usb_phy *phy)
{
	clrsetbits_32(phy->port_base + U3P_U2PHYDTM0,
		P2C_RG_XCVRSEL | P2C_RG_DATAIN, P2C_FORCE_SUSPENDM);

	/* OTG Disable */
	clrbits_32(phy->port_base + U3P_USBPHYACR6, PA6_RG_U2_OTG_VBUSCMP_EN);

	/* let suspendm=0, set utmi into analog power down */
	clrbits_32(phy->port_base + U3P_U2PHYDTM0, P2C_RG_SUSPENDM);
	udelay(1);

	clrsetbits_32(phy->port_base + U3P_U2PHYDTM1,
		P2C_RG_VBUSVALID | P2C_RG_AVALID, P2C_RG_SESSEND);
}

static void hs_slew_rate_calibrate(struct mt7621_xhci_priv *priv,
	struct mt7621_usb_phy *phy)
{
	void __iomem *fmreg;
	int cal_val;
	int fm_out;
	u32 tmp;

	fmreg = priv->sifbase + SSUSB_SIFSLV_V1_U2FREQ;

	/* enable USB ring oscillator */
	setbits_32(phy->port_base + U3P_USBPHYACR5, PA5_RG_U2_HSTX_SRCAL_EN);
	udelay(1);

	/*enable free run clock */
	setbits_32(fmreg + U3P_U2FREQ_FMMONR1, P2F_RG_FRCK_EN);

	/* set cycle count as 1024, and select u2 channel */
	clrsetbits_32(fmreg + U3P_U2FREQ_FMCR0,
		P2F_RG_CYCLECNT | P2F_RG_MONCLK_SEL,
		P2F_RG_CYCLECNT_VAL(U3P_FM_DET_CYCLE_CNT) |
		P2F_RG_MONCLK_SEL_VAL(phy->index >> 1));

	/* enable frequency meter */
	setbits_32(fmreg + U3P_U2FREQ_FMCR0, P2F_RG_FREQDET_EN);

	/* ignore return value */
	readl_poll_timeout(fmreg + U3P_U2FREQ_FMMONR1, tmp,
		(tmp & P2F_USB_FM_VALID), 2000);

	fm_out = readl(fmreg + U3P_U2FREQ_VALUE);

	/* disable frequency meter */
	clrbits_32(fmreg + U3P_U2FREQ_FMCR0, P2F_RG_FREQDET_EN);

	/*disable free run clock */
	clrbits_32(fmreg + U3P_U2FREQ_FMMONR1, P2F_RG_FRCK_EN);

	if (fm_out) {
		/* ( 1024 / FM_OUT ) x reference clock frequency x 0.028 */
		tmp = U3P_FM_DET_CYCLE_CNT * U3P_REF_CLK * U3P_SLEW_RATE_COEF;
		tmp /= fm_out;
		cal_val = DIV_ROUND_CLOSEST(tmp, U3P_SR_COEF_DIVISOR);
	} else {
		/* if FM detection fail, set default value */
		cal_val = 4;
	}

	/* set HS slew rate */
	clrsetbits_32(phy->port_base + U3P_USBPHYACR5,
		PA5_RG_U2_HSTX_SRCTRL, PA5_RG_U2_HSTX_SRCTRL_VAL(cal_val));

	/* disable USB ring oscillator */
	clrbits_32(phy->port_base + U3P_USBPHYACR5, PA5_RG_U2_HSTX_SRCAL_EN);
}

static int xhci_usb_probe(struct udevice *dev)
{
	struct mt7621_xhci_pdata *pdata = dev_get_platdata(dev);
	struct mt7621_xhci_priv *priv = dev_get_priv(dev);
	struct xhci_hccr *hccr;
	struct xhci_hcor *hcor;
	int i, u2p_base_idx = 0, u3p_base_idx = 0;
	void __iomem *ippc;
	uint32_t val;

	if (!pdata->membase)
		return -EINVAL;

	priv->membase = pdata->membase;
	priv->sifbase = priv->membase + SSUSB_SIFSLV_BASE;

	/* Detect number of ports (USB2.0/USB3.0) */
	ippc = priv->sifbase + SSUSB_SIFSLV_V1_IPPC;
	val = readl(ippc + SSUSB_IP_CAP);
	priv->num_u2_ports = CAP_U2_PORT_NUM(val);
	priv->num_u3_ports = CAP_U3_PORT_NUM(val);

	/* Port structure */
	priv->u2p = (struct mt7621_usb_phy *) calloc(priv->num_u2_ports,
		sizeof(struct mt7621_usb_phy));
	if (!priv->u2p)
		return -ENOMEM;

	priv->u3p = (struct mt7621_usb_phy *) calloc(priv->num_u3_ports,
		sizeof(struct mt7621_usb_phy));
	if (!priv->u3p)
		return -ENOMEM;

	for (i = 0; i < ARRAY_SIZE(mt7621_usb_phy_bases); i++) {
		if (mt7621_usb_phy_bases[i].is_u3p == false) {
			priv->u2p[u2p_base_idx].index = i;
			priv->u2p[u2p_base_idx].port_base =
				priv->sifbase + mt7621_usb_phy_bases[i].base;
			u2p_base_idx++;
		} else {
			priv->u3p[u3p_base_idx].index = i;
			priv->u3p[u3p_base_idx].port_base =
				priv->sifbase + mt7621_usb_phy_bases[i].base;
			u3p_base_idx++;
		}
	}

	if (u2p_base_idx < priv->num_u2_ports) {
		printf("mt7621-xhci: no enough hs port register bases "
		       "(expected %d, got %d)\n",
		       priv->num_u2_ports, u2p_base_idx);
		return -EINVAL;
	}

	if (u3p_base_idx < priv->num_u3_ports) {
		printf("mt7621-xhci: no enough ss port register bases "
		       "(expected %d, got %d)\n",
		       priv->num_u3_ports, u3p_base_idx);
		return -EINVAL;
	}

	/* De-assert PHYs reset */
	xhci_mtk_host_enable(priv);

	/* Initialize HS PHYs */
	for (i = 0; i < priv->num_u2_ports; i++) {
		u2_phy_instance_init(priv, &priv->u2p[i]);
		u2_phy_instance_power_on(priv, &priv->u2p[i]);
		hs_slew_rate_calibrate(priv, &priv->u2p[i]);
	}

	/* Initialize SS PHYs */
	for (i = 0; i < priv->num_u3_ports; i++)
		u3_phy_instance_init(priv, &priv->u3p[i]);

	/* Register XHCI */
	hccr = (struct xhci_hccr *) priv->membase;
	hcor = (struct xhci_hcor *)((uint32_t) hccr +
		HC_LENGTH(xhci_readl(&hccr->cr_capbase)));

	return xhci_register(dev, hccr, hcor);
}

static int xhci_usb_remove(struct udevice *dev)
{
	struct mt7621_xhci_priv *priv = dev_get_priv(dev);
	int ret;
	int i;

	ret = xhci_deregister(dev);
	if (ret)
		return ret;

	/* Trun off HS PHYs */
	for (i = 0; i < priv->num_u2_ports; i++)
		u2_phy_instance_power_off(priv, &priv->u2p[i]);

	/* Assert PHYs reset */
	xhci_mtk_host_disable(priv);

	return 0;
}

#if CONFIG_IS_ENABLED(OF_CONTROL) && !CONFIG_IS_ENABLED(OF_PLATDATA)
static int mt7621_xhci_ofdata_to_platdata(struct udevice *dev)
{
	struct mt7621_xhci_pdata *pdata = dev_get_platdata(dev);
	struct resource res;
	int ret;

	ret = dev_read_resource(dev, 0, &res);
	if (ret)
		return ret;

	pdata->membase = ioremap(res.start, resource_size(&res));

	return 0;
}

static const struct udevice_id mt7621_xhci_usb_ids[] = {
	{ .compatible = "mediatek,mt7621-xhci" },
	{}
};
#endif

U_BOOT_DRIVER(mt7621_usb_xhci) = {
	.name	= "mt7621-xhci",
	.id	= UCLASS_USB,
#if CONFIG_IS_ENABLED(OF_CONTROL) && !CONFIG_IS_ENABLED(OF_PLATDATA)
	.of_match = mt7621_xhci_usb_ids,
	.ofdata_to_platdata = mt7621_xhci_ofdata_to_platdata,
	.platdata_auto_alloc_size = sizeof(struct mt7621_xhci_pdata),
#endif
	.probe = xhci_usb_probe,
	.remove = xhci_usb_remove,
	.ops	= &xhci_usb_ops,
	.priv_auto_alloc_size = sizeof(struct mt7621_xhci_priv),
	.flags	= DM_FLAG_ALLOC_PRIV_DMA,
};