// SPDX-License-Identifier:	GPL-2.0+
/*
* Copyright (C) 2018 MediaTek Incorporation. All Rights Reserved.
*
* Author: Weijie Gao <weijie.gao@mediatek.com>
*/

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <reset-uclass.h>
#include <linux/io.h>

struct mt7621_reset_priv {
	void __iomem *base;
};

static int mt7621_reset_request(struct reset_ctl *reset_ctl)
{
	return 0;
}

static int mt7621_reset_free(struct reset_ctl *reset_ctl)
{
	return 0;
}

static int mt7621_reset_assert(struct reset_ctl *reset_ctl)
{
	struct mt7621_reset_priv *priv = dev_get_priv(reset_ctl->dev);

	setbits_le32(priv->base, BIT(reset_ctl->id));

	return 0;
}

static int mt7621_reset_deassert(struct reset_ctl *reset_ctl)
{
	struct mt7621_reset_priv *priv = dev_get_priv(reset_ctl->dev);

	clrbits_le32(priv->base, BIT(reset_ctl->id));

	return 0;
}

static const struct reset_ops mt7621_reset_ops = {
	.request	= mt7621_reset_request,
	.free		= mt7621_reset_free,
	.rst_assert	= mt7621_reset_assert,
	.rst_deassert	= mt7621_reset_deassert,
};

static int mt7621_reset_probe(struct udevice *dev)
{
	return 0;
}

static int mt7621_reset_ofdata_to_platdata(struct udevice *dev)
{
	struct mt7621_reset_priv *priv = dev_get_priv(dev);

	priv->base = (void *) dev_read_addr(dev);
	if (!priv->base)
		return -EINVAL;

	return 0;
}

static const struct udevice_id mt7621_reset_ids[] = {
	{ .compatible = "mediatek,mt7621-reset" },
	{ }
};

U_BOOT_DRIVER(mt7621_reset) = {
	.name = "mt7621-reset",
	.id = UCLASS_RESET,
	.of_match = mt7621_reset_ids,
	.ofdata_to_platdata = mt7621_reset_ofdata_to_platdata,
	.probe = mt7621_reset_probe,
	.priv_auto_alloc_size = sizeof(struct mt7621_reset_priv),
	.ops = &mt7621_reset_ops,
};