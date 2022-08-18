// SPDX-License-Identifier:	GPL-2.0+
/*
* Copyright (C) 2018 MediaTek Incorporation. All Rights Reserved.
*
* Author: Weijie Gao <weijie.gao@mediatek.com>
*/

#include <common.h>
#include <dm.h>
#include <asm/gpio.h>
#include <linux/errno.h>
#include <linux/io.h>

struct mt7621_gpio_priv {
	void __iomem *base;
	u32 gpio_count;
};

#define MAX_GPIO		128
#define GPIO_COUNT_PER_REG	32
#define GPIO_OFFS(n)		(n << 2)

#define GPIO_CTRL		0
#define GPIO_DATA		0x20
#define GPIO_DSET		0x30
#define GPIO_DCLR		0x40

#define GPIO_REG(priv, name, offs) \
	((priv)->base + GPIO_ ## name + GPIO_OFFS(offs))

static int mt7621_gpio_direction_input(struct udevice *dev, unsigned offset)
{
	struct mt7621_gpio_priv *priv = dev_get_priv(dev);
	u32 reg, bit;

	reg = offset / GPIO_COUNT_PER_REG;
	bit = offset % GPIO_COUNT_PER_REG;

	clrbits_le32(GPIO_REG(priv, CTRL, reg), 1 << bit);

	return 0;
}

static int mt7621_gpio_direction_output(struct udevice *dev, unsigned offset,
					int value)
{
	struct mt7621_gpio_priv *priv = dev_get_priv(dev);
	u32 reg, bit;

	reg = offset / GPIO_COUNT_PER_REG;
	bit = offset % GPIO_COUNT_PER_REG;

	setbits_le32(GPIO_REG(priv, CTRL, reg), 1 << bit);

	if (value)
		writel(1 << bit, GPIO_REG(priv, DSET, reg));
	else
		writel(1 << bit, GPIO_REG(priv, DCLR, reg));

	return 0;
}

static int mt7621_gpio_get_value(struct udevice *dev, unsigned offset)
{
	struct mt7621_gpio_priv *priv = dev_get_priv(dev);
	u32 reg, bit;

	reg = offset / GPIO_COUNT_PER_REG;
	bit = offset % GPIO_COUNT_PER_REG;

	return (readl(GPIO_REG(priv, DATA, reg)) >> bit) & 1;
}

static int mt7621_gpio_set_value(struct udevice *dev, unsigned offset, int value)
{
	struct mt7621_gpio_priv *priv = dev_get_priv(dev);
	u32 reg, bit;

	reg = offset / GPIO_COUNT_PER_REG;
	bit = offset % GPIO_COUNT_PER_REG;

	if (value)
		writel(1 << bit, GPIO_REG(priv, DSET, reg));
	else
		writel(1 << bit, GPIO_REG(priv, DCLR, reg));

	return 0;
}

static int mt7621_gpio_get_direction(struct udevice *dev, unsigned int offset)
{
	struct mt7621_gpio_priv *priv = dev_get_priv(dev);
	u32 reg, bit;

	reg = offset / GPIO_COUNT_PER_REG;
	bit = offset % GPIO_COUNT_PER_REG;

	return readl(GPIO_REG(priv, CTRL, reg)) & (1 << bit) ?
		GPIOF_OUTPUT : GPIOF_INPUT;
}

static const struct dm_gpio_ops mt7621_gpio_ops = {
	.direction_input = mt7621_gpio_direction_input,
	.direction_output = mt7621_gpio_direction_output,
	.get_value = mt7621_gpio_get_value,
	.set_value = mt7621_gpio_set_value,
	.get_function = mt7621_gpio_get_direction,
};

static int mt7621_gpio_probe(struct udevice *dev)
{
	struct gpio_dev_priv *uc_priv = dev_get_uclass_priv(dev);
	struct mt7621_gpio_priv *priv = dev_get_priv(dev);
	const char *name;

	name = dev_read_string(dev, "bank-name");
	if (!name)
		name = "default-gpio";

	uc_priv->bank_name = name;
	uc_priv->gpio_count = priv->gpio_count;

	return 0;
}

static int mt7621_gpio_ofdata_to_platdata(struct udevice *dev)
{
	struct mt7621_gpio_priv *priv = dev_get_priv(dev);

	priv->base = (void *) dev_read_addr(dev);
	if (!priv->base)
		return -EINVAL;

	priv->gpio_count = dev_get_driver_data(dev);
	if (priv->gpio_count > MAX_GPIO)
		return -EINVAL;

	return 0;
}

static const struct udevice_id mt7621_gpio_ids[] = {
	{ .compatible = "mediatek,mt7621-gpio", .data = 49 },
	{ }
};

U_BOOT_DRIVER(mt7621_gpio) = {
	.name = "mt7621-gpio",
	.id = UCLASS_GPIO,
	.of_match = mt7621_gpio_ids,
	.ofdata_to_platdata = mt7621_gpio_ofdata_to_platdata,
	.probe = mt7621_gpio_probe,
	.ops = &mt7621_gpio_ops,
	.priv_auto_alloc_size = sizeof(struct mt7621_gpio_priv),
};