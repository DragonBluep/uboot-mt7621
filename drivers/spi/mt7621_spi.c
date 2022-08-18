// SPDX-License-Identifier:	GPL-2.0+
/*
 * Copyright (C) 2019 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <spi.h>
#include <asm/io.h>
#include <linux/bitops.h>
#include <linux/iopoll.h>

#define SPI_TRANS_REG				0x00
#define SPI_ADDR_SIZE_MASK			  0x03
#define SPI_ADDR_SIZE_SHIFT			  19
#define SPI_MASTER_BUSY				  BIT(16)
#define SPI_MASTER_START			  BIT(8)

#define SPI_OP_ADDR_REG				0x04
#define SPI_DIDO_REG(x)				(0x08 + ((x) << 2))

#define SPI_MASTER_REG				0x28
#define RS_SLAVE_SEL_MASK			  0x7
#define RS_SLAVE_SEL_SHIFT			  29
#define RS_CLK_SEL_MASK				  0xfff
#define RS_CLK_SEL_SHIFT			  16
#define BIDIR_MODE				  BIT(6)
#define CPHA					  BIT(5)
#define CPOL					  BIT(4)
#define LSB_FIRST				  BIT(3)
#define MORE_BUF_MODE				  BIT(2)

#define SPI_MOREBUF_REG				0x2c
#define CMD_BIT_CNT_MASK			  0x3f
#define CMD_BIT_CNT_SHIFT			  24
#define MISO_BIT_CNT_MASK			  0x1ff
#define MISO_BIT_CNT_SHIFT			  12
#define MOSI_BIT_CNT_MASK			  0x1ff
#define MOSI_BIT_CNT_SHIFT			  0

#define SPI_CS_POLAR_REG			0x38

#define MT7621_RX_FIFO_LEN			32
#define MT7621_TX_FIFO_LEN			36

#define MT7621_SPI_MAX_CS_NUM			2

#define MT7621_SPI_POLL_INTERVAL		100000

struct mt7621_spi_priv {
	void __iomem *base;
	u32 bus_freq;
};

static u32 mt7621_spi_get_clk_div(u32 hclk_freq, u32 freq)
{
	u32 div;

	div = (hclk_freq + freq - 1) / freq;

	if (div > (RS_CLK_SEL_MASK + 2))
		return RS_CLK_SEL_MASK;

	if (div < 2)
		return 2;

	return div - 2;
}

static int mt7621_spi_busy_wait(struct mt7621_spi_priv *priv)
{
	u32 val;
	int ret;

	ret = readl_poll_timeout(priv->base + SPI_TRANS_REG, val,
		!(val & SPI_MASTER_BUSY), MT7621_SPI_POLL_INTERVAL);

	return ret;
}

static void mt7621_spi_set_cs(struct mt7621_spi_priv *priv, int cs, int enable)
{
	if (enable) {
		setbits_32(priv->base + SPI_MASTER_REG,
			MORE_BUF_MODE |
			(RS_SLAVE_SEL_MASK << RS_SLAVE_SEL_SHIFT));
		writel(0, priv->base + SPI_CS_POLAR_REG);
		writel(BIT(cs), priv->base + SPI_CS_POLAR_REG);
	} else {
		clrbits_32(priv->base + SPI_MASTER_REG,
			MORE_BUF_MODE |
			(RS_SLAVE_SEL_MASK << RS_SLAVE_SEL_SHIFT));
		clrsetbits_32(priv->base + SPI_TRANS_REG,
			SPI_ADDR_SIZE_MASK << SPI_ADDR_SIZE_SHIFT,
			2 << SPI_ADDR_SIZE_SHIFT);
		writel(0, priv->base + SPI_CS_POLAR_REG);
	}
}

static int mt7621_spi_read(struct mt7621_spi_priv *priv, u8 *buf, size_t len)
{
	size_t rx_len;
	int i, ret;
	u32 val = 0;

	while (len) {
		rx_len = min_t(size_t, len, MT7621_RX_FIFO_LEN);

		writel((rx_len * 8) << MISO_BIT_CNT_SHIFT,
			priv->base + SPI_MOREBUF_REG);
		writel(SPI_MASTER_START, priv->base + SPI_TRANS_REG);

		ret = mt7621_spi_busy_wait(priv);
		if (ret)
			return ret;

		for (i = 0; i < rx_len; i++) {
			if ((i % 4) == 0)
				val = readl(priv->base + SPI_DIDO_REG(i / 4));
			*buf++ = val & 0xff;
			val >>= 8;
		}

		len -= rx_len;
	}

	return ret;
}

static int mt7621_spi_write(struct mt7621_spi_priv *priv, const u8 *buf,
			    size_t len)
{
	size_t tx_len, opcode_len, dido_len;
	int i, ret;
	u32 val;

	while (len) {
		tx_len = min_t(size_t, len, MT7621_TX_FIFO_LEN);

		opcode_len = min_t(size_t, tx_len, 4);
		dido_len = tx_len - opcode_len;

		val = 0;
		for (i = 0; i < opcode_len; i++) {
			val <<= 8;
			val |= *buf++;
		}

		writel(val, priv->base + SPI_OP_ADDR_REG);

		val = 0;
		for (i = 0; i < dido_len; i++) {
			val |= (*buf++) << ((i % 4) * 8);

			if ((i % 4 == 3) || (i == dido_len - 1)) {
				writel(val, priv->base + SPI_DIDO_REG(i / 4));
				val = 0;
			}
		}

		writel(((opcode_len * 8) << CMD_BIT_CNT_SHIFT) |
			((dido_len * 8) << MOSI_BIT_CNT_SHIFT),
			priv->base + SPI_MOREBUF_REG);
		writel(SPI_MASTER_START, priv->base + SPI_TRANS_REG);

		ret = mt7621_spi_busy_wait(priv);
		if (ret)
			return ret;

		len -= tx_len;
	}

	return 0;
}

static int mt7621_spi_xfer(struct udevice *dev, unsigned int bitlen,
		const void *dout, void *din, unsigned long flags)
{
	struct udevice *bus = dev_get_parent(dev);
	struct mt7621_spi_priv *priv = dev_get_priv(bus);
	struct dm_spi_slave_platdata *plat = dev_get_parent_platdata(dev);
	int len, ret = 0;

	if (din && dout)
		return -ENOTSUPP;

	if (flags & SPI_XFER_BEGIN)
		mt7621_spi_set_cs(priv, plat->cs, 1);

	len = (bitlen + 7) / 8;

	if (din)
		ret = mt7621_spi_read(priv, din, len);
	else if (dout)
		ret = mt7621_spi_write(priv, dout, len);

	if (flags & SPI_XFER_END)
		mt7621_spi_set_cs(priv, plat->cs, 0);

	return ret;
}

static int mt7621_spi_set_speed(struct udevice *bus, uint speed)
{
	struct mt7621_spi_priv *priv = dev_get_priv(bus);
	u32 master, clkdiv;

	if ((speed == 0) || (speed > (priv->bus_freq / 2)))
		speed = (priv->bus_freq / 2);

	if (speed < (priv->bus_freq / (RS_CLK_SEL_MASK + 2))) {
		printf("%s: requested speed is too low: %dHz\n",
			__func__, speed);
		return -EINVAL;
	}

	master = readl(priv->base + SPI_MASTER_REG);
	master &= ~(RS_CLK_SEL_MASK << RS_CLK_SEL_SHIFT);

	clkdiv = mt7621_spi_get_clk_div(priv->bus_freq, speed);
	master |= clkdiv << RS_CLK_SEL_SHIFT;

	writel(master, priv->base + SPI_MASTER_REG);

	return 0;
}

static int mt7621_spi_set_mode(struct udevice *bus, uint mode)
{
	struct mt7621_spi_priv *priv = dev_get_priv(bus);
	u32 master;

	if (mode & (SPI_CS_HIGH | SPI_PREAMBLE | SPI_TX_DUAL | SPI_TX_QUAD |
		SPI_RX_DUAL | SPI_RX_QUAD)) {
		printf("%s: requested mode has not supported bit(s)\n",
			__func__);
		return -EINVAL;
	}

	master = readl(priv->base + SPI_MASTER_REG);
	master &= RS_CLK_SEL_MASK << RS_CLK_SEL_SHIFT;

	master |= RS_SLAVE_SEL_MASK << RS_SLAVE_SEL_SHIFT;

	if (mode & SPI_LSB_FIRST)
		master |= LSB_FIRST;

	if (mode & SPI_CPOL)
		master |= CPOL;

	if (mode & SPI_CPHA)
		master |= CPHA;

	if (mode & SPI_3WIRE)
		master |= BIDIR_MODE;

	writel(master, priv->base + SPI_MASTER_REG);

	return 0;
}

static int mt7621_spi_child_pre_probe(struct udevice *dev)
{
	struct dm_spi_slave_platdata *plat = dev_get_parent_platdata(dev);

	if (plat->cs >= MT7621_SPI_MAX_CS_NUM) {
		printf("%s: invalid chip select %u\n", __func__, plat->cs);
		return -ENODEV;
	}

	return 0;
}

static int mt7621_spi_probe(struct udevice *bus)
{
	struct mt7621_spi_priv *priv = dev_get_priv(bus);

	priv->base = (void __iomem *) devfdt_get_addr(bus);
	priv->bus_freq = get_bus_freq(0);

	return 0;
}

static const struct dm_spi_ops mt7621_spi_ops = {
	.xfer       = mt7621_spi_xfer,
	.set_speed  = mt7621_spi_set_speed,
	.set_mode   = mt7621_spi_set_mode,
};

#if CONFIG_IS_ENABLED(OF_CONTROL) && !CONFIG_IS_ENABLED(OF_PLATDATA)
static const struct udevice_id mt7621_spi_ids[] = {
	{ .compatible = "mediatek,mt7621-spi" },
	{ }
};
#endif

U_BOOT_DRIVER(mt7621_spi) = {
	.name   = "mt7621_spi",
	.id = UCLASS_SPI,
#if CONFIG_IS_ENABLED(OF_CONTROL) && !CONFIG_IS_ENABLED(OF_PLATDATA)
	.of_match = mt7621_spi_ids,
#endif
	.ops    = &mt7621_spi_ops,
	.priv_auto_alloc_size = sizeof(struct mt7621_spi_priv),
	.probe  = mt7621_spi_probe,
	.child_pre_probe = mt7621_spi_child_pre_probe,
};
