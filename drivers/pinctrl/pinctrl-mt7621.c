// SPDX-License-Identifier:	GPL-2.0+
/*
 * Copyright (C) 2018 MediaTek Incorporation. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <common.h>
#include <dm.h>
#include <linux/bitops.h>
#include <dm/pinctrl.h>
#include <linux/io.h>

#include <mach/mt7621_regs.h>

#define PAD_UART1_GPIO0_CFG		0x00
#define PAD_UART3_I2C_CFG		0x04
#define PAD_UART2_JTAG_CFG		0x08
#define PAD_PERST_WDT_CFG		0x0c
#define PAD_RGMII2_MDIO_CFG		0x10
#define PAD_SDXC_SPI_CFG		0x14
#define GPIO_MODE			0x18
#define PAD_BOPT_ESWINT_CFG		0x24
#define PAD_RGMII1_CFG			0x28

#define E4_E2_M				0x03
#define E4_E2_S				4
#define PULL_UP				BIT(3)
#define PULL_DOWN			BIT(2)
#define SMT				BIT(1)
#define SR				BIT(0)

#define MT7621_GPIO_MODE_UART3_GPIO	1
#define MT7621_GPIO_MODE_UART2_GPIO	1
#define MT7621_GPIO_MODE_WDT_GPIO	1
#define MT7621_GPIO_MODE_PCIE_GPIO	1
#define MT7621_GPIO_MODE_MDIO_GPIO	1
#define MT7621_GPIO_MODE_SPI_GPIO	1
#define MT7621_GPIO_MODE_SDHCI_GPIO	1

struct mt7621_pmx_func {
	const char *name;
	int value;

	int pin_first;
	int pin_count;
};

struct mt7621_pmx_group {
	const char *name;

	u32 shift;
	char mask;
	char gpio;

	u32 pconf_reg;
	u32 pconf_shift;

	const struct mt7621_pmx_func *func;
	int func_count;
};

struct mt7621_pinctrl_priv {
	void __iomem *base;

	u32 nfuncs;
	const struct mt7621_pmx_func **funcs;
};

#define FUNC(name, value, pin_first, pin_count) \
	{ name, value, pin_first, pin_count }

#define GRP_DUMMY(_name, _pconf_reg, _pconf_shift) \
	{ .name = _name, .mask = 0, .shift = 0, \
	  .func = NULL, .gpio = 0, \
	  .pconf_reg = _pconf_reg, .pconf_shift = _pconf_shift, \
	  .func_count = 0 }

#define GRP_G(_name, _func, _mask, _gpio, _shift, _pconf_reg, _pconf_shift) \
	{ .name = _name, .mask = _mask, .shift = _shift, \
	  .func = _func, .gpio = _gpio, \
	  .pconf_reg = _pconf_reg, .pconf_shift = _pconf_shift, \
	  .func_count = ARRAY_SIZE(_func) }

#define GRP(_name, _func, _mask, _shift, _pconf_reg, _pconf_shift) \
	GRP_G((_name), (_func), (_mask), (_mask), (_shift), \
	      (_pconf_reg), (_pconf_shift))

static const struct mt7621_pmx_func gpio_grp[] = {
	FUNC("gpio", 1, 0, 0)
};

static const struct mt7621_pmx_func uart1_grp[] = {
	FUNC("uart1", 0, 1, 2)
};

static const struct mt7621_pmx_func i2c_grp[] = {
	FUNC("i2c", 0, 3, 2)
};

static const struct mt7621_pmx_func uart3_grp[] = {
	FUNC("uart3", 0, 5, 4),
	FUNC("i2s", 2, 5, 4),
	FUNC("spdif3", 3, 5, 4),
};

static const struct mt7621_pmx_func uart2_grp[] = {
	FUNC("uart2", 0, 9, 4),
	FUNC("pcm", 2, 9, 4),
	FUNC("spdif2", 3, 9, 4),
};

static const struct mt7621_pmx_func jtag_grp[] = {
	FUNC("jtag", 0, 13, 5)
};

static const struct mt7621_pmx_func wdt_grp[] = {
	FUNC("wdt rst", 0, 18, 1),
	FUNC("wdt refclk", 2, 18, 1),
};

static const struct mt7621_pmx_func pcie_rst_grp[] = {
	FUNC("pcie rst", 0, 19, 1),
	FUNC("pcie refclk", 2, 19, 1)
};

static const struct mt7621_pmx_func mdio_grp[] = {
	FUNC("mdio", 0, 20, 2)
};

static const struct mt7621_pmx_func rgmii2_grp[] = {
	FUNC("rgmii2", 0, 22, 12)
};

static const struct mt7621_pmx_func spi_grp[] = {
	FUNC("spi", 0, 34, 7),
	FUNC("nand1", 2, 34, 7),
};

static const struct mt7621_pmx_func sdhci_grp[] = {
	FUNC("sdhci", 0, 41, 8),
	FUNC("nand2", 2, 41, 8),
};

static const struct mt7621_pmx_func rgmii1_grp[] = {
	FUNC("rgmii1", 0, 49, 12)
};

static const struct mt7621_pmx_func esw_int_grp[] = {
	FUNC("esw int", 0, 61, 1)
};

static const struct mt7621_pmx_group mt7621_pinmux_data[] = {
	GRP_DUMMY("gpio0", PAD_UART1_GPIO0_CFG, 0),
	GRP("uart1", uart1_grp, UART1_MODE_M, UART1_MODE_S,
	    PAD_UART1_GPIO0_CFG, 16),
	GRP("i2c", i2c_grp, I2C_MODE_M, I2C_MODE_S,
	    PAD_UART3_I2C_CFG, 0),
	GRP_G("uart3", uart3_grp, UART3_MODE_M,
	      MT7621_GPIO_MODE_UART3_GPIO, UART3_MODE_S,
	      PAD_UART3_I2C_CFG, 16),
	GRP_G("uart2", uart2_grp, UART2_MODE_M,
	      MT7621_GPIO_MODE_UART2_GPIO, UART2_MODE_S,
	      PAD_UART2_JTAG_CFG, 16),
	GRP("jtag", jtag_grp, JTAG_MODE_M, JTAG_MODE_S,
	    PAD_UART2_JTAG_CFG, 0),
	GRP_G("wdt", wdt_grp, WDT_MODE_M,
	      MT7621_GPIO_MODE_WDT_GPIO, WDT_MODE_S,
	      PAD_PERST_WDT_CFG, 0),
	GRP_G("pcie", pcie_rst_grp, PERST_MODE_M,
	      MT7621_GPIO_MODE_PCIE_GPIO, PERST_MODE_S,
	      PAD_PERST_WDT_CFG, 16),
	GRP_G("mdio", mdio_grp, MDIO_MODE_M,
	      MT7621_GPIO_MODE_MDIO_GPIO, MDIO_MODE_S,
	      PAD_RGMII2_MDIO_CFG, 0),
	GRP("rgmii2", rgmii2_grp, RGMII2_MODE_M, RGMII2_MODE_S,
	    PAD_RGMII2_MDIO_CFG, 16),
	GRP_G("spi", spi_grp, SPI_MODE_M,
	      MT7621_GPIO_MODE_SPI_GPIO, SPI_MODE_S,
	      PAD_SDXC_SPI_CFG, 0),
	GRP_G("sdhci", sdhci_grp, SDXC_MODE_M,
	      MT7621_GPIO_MODE_SDHCI_GPIO, SDXC_MODE_S,
	      PAD_SDXC_SPI_CFG, 16),
	GRP("rgmii1", rgmii1_grp, RGMII1_MODE_M, RGMII1_MODE_S,
	    PAD_RGMII1_CFG, 0),
	GRP("esw", esw_int_grp, ESWINT_MODE_M, ESWINT_MODE_S,
	    PAD_BOPT_ESWINT_CFG, 0),
};

static const u32 mt7621_pconf_drv_strength_tbl[] = {2, 4, 6, 8};

static inline void mt7621_pinctrl_set(struct mt7621_pinctrl_priv *priv,
	u32 reg, u32 shift, u32 mask, u32 value)
{
	u32 val = readl(priv->base + reg);
	val &= ~(mask << shift);
	val |= value << shift;
	writel(val, priv->base + reg);
}

static inline void mt7621_gpio_mode_set(struct mt7621_pinctrl_priv *priv,
	u32 mask, u32 shift, u32 value)
{
	return mt7621_pinctrl_set(priv, GPIO_MODE, shift, mask, value);
}

static int mt7621_get_groups_count(struct udevice *dev)
{
	return sizeof(mt7621_pinmux_data);
}

static const char *mt7621_get_group_name(struct udevice *dev,
	unsigned int selector)
{
	return mt7621_pinmux_data[selector].name;
}

#if CONFIG_IS_ENABLED(PINMUX)
static int mt7621_get_functions_count(struct udevice *dev)
{
	struct mt7621_pinctrl_priv *priv = dev_get_priv(dev);

	return priv->nfuncs;
}

static const char *mt7621_get_function_name(struct udevice *dev,
	unsigned int selector)
{
	struct mt7621_pinctrl_priv *priv = dev_get_priv(dev);

	return priv->funcs[selector]->name;
}

static int mt7621_pinmux_group_set(struct udevice *dev,
	unsigned int group_selector,
	unsigned int func_selector)
{
	struct mt7621_pinctrl_priv *priv = dev_get_priv(dev);
	const struct mt7621_pmx_group *grp =
		&mt7621_pinmux_data[group_selector];
	const struct mt7621_pmx_func *func =
		priv->funcs[func_selector];
	int i;

	if (!grp->func_count)
		return 0;

	/* GPIO mode */
	if (func == &gpio_grp[0]) {
		mt7621_gpio_mode_set(priv, grp->mask, grp->shift, grp->gpio);
		return 0;
	}

	for (i = 0; i < grp->func_count; i++) {
		if (&grp->func[i] == func) {
			mt7621_gpio_mode_set(priv, grp->mask,
					     grp->shift, func->value);
			return 0;
		}
	}

	return -EINVAL;
}
#endif

#if CONFIG_IS_ENABLED(PINCONF)
static const struct pinconf_param mt7621_conf_params[] = {
	{ "bias-disable", PIN_CONFIG_BIAS_DISABLE, 0 },
	{ "bias-pull-up", PIN_CONFIG_BIAS_PULL_UP, 1 },
	{ "bias-pull-down", PIN_CONFIG_BIAS_PULL_DOWN, 1 },
	{ "input-schmitt-enable", PIN_CONFIG_INPUT_SCHMITT_ENABLE, 1 },
	{ "input-schmitt-disable", PIN_CONFIG_INPUT_SCHMITT_ENABLE, 0 },
	{ "drive-strength", PIN_CONFIG_DRIVE_STRENGTH, 0 },
	{ "slew-rate", PIN_CONFIG_SLEW_RATE, 0 },
};

static int mt7621_pinconf_group_set(struct udevice *dev,
	unsigned int group_selector,
	unsigned int param, unsigned int arg)
{
	struct mt7621_pinctrl_priv *priv = dev_get_priv(dev);
	const struct mt7621_pmx_group *grp =
		&mt7621_pinmux_data[group_selector];
	u32 clr = 0, set = 0;
	int i;

	switch (param) {
	case PIN_CONFIG_BIAS_DISABLE:
		clr = PULL_UP | PULL_DOWN;
		break;
	case PIN_CONFIG_BIAS_PULL_UP:
		clr = PULL_DOWN;
		set = PULL_UP;
		break;
	case PIN_CONFIG_BIAS_PULL_DOWN:
		clr = PULL_UP;
		set = PULL_DOWN;
		break;
	case PIN_CONFIG_INPUT_SCHMITT_ENABLE:
		if (arg)
			set = SMT;
		else
			clr = SMT;
		break;
	case PIN_CONFIG_DRIVE_STRENGTH:
		for (i = 0; i < ARRAY_SIZE(mt7621_pconf_drv_strength_tbl); i++)
			if (mt7621_pconf_drv_strength_tbl[i] == arg)
				break;

		if (i >= ARRAY_SIZE(mt7621_pconf_drv_strength_tbl))
			return -EINVAL;

		clr = E4_E2_M << E4_E2_S;
		set = i << E4_E2_S;
		break;
	case PIN_CONFIG_SLEW_RATE:
		if (arg)
			set = SR;
		else
			clr = SR;
		break;
	default:
		return -EINVAL;
	}

	mt7621_pinctrl_set(priv, grp->pconf_reg, grp->pconf_shift,
		clr, set);

	return 0;
}
#endif

const struct pinctrl_ops mt7621_pinctrl_ops = {
	.get_groups_count = mt7621_get_groups_count,
	.get_group_name = mt7621_get_group_name,
#if CONFIG_IS_ENABLED(PINMUX)
	.get_functions_count = mt7621_get_functions_count,
	.get_function_name = mt7621_get_function_name,
	.pinmux_group_set = mt7621_pinmux_group_set,
#endif
#if CONFIG_IS_ENABLED(PINCONF)
	.pinconf_num_params = ARRAY_SIZE(mt7621_conf_params),
	.pinconf_params = mt7621_conf_params,
	.pinconf_group_set = mt7621_pinconf_group_set,
#endif
	.set_state = pinctrl_generic_set_state,
};

static int mt7621_pinctrl_probe(struct udevice *dev)
{
	struct mt7621_pinctrl_priv *priv = dev_get_priv(dev);
	int i, j, n;

	priv->nfuncs = 1;

	for (i = 0; i < ARRAY_SIZE(mt7621_pinmux_data); i++)
		priv->nfuncs += mt7621_pinmux_data[i].func_count;

	priv->funcs = malloc(priv->nfuncs * sizeof(*priv->funcs));
	if (!priv->funcs)
		return -ENOMEM;

	n = 0;
	priv->funcs[n++] = &gpio_grp[0];

	for (i = 0; i < ARRAY_SIZE(mt7621_pinmux_data); i++) {
		for (j = 0; j < mt7621_pinmux_data[i].func_count; j++)
			priv->funcs[n++] = &mt7621_pinmux_data[i].func[j];
	}

	return 0;
}

static int mt7621_pinctrl_ofdata_to_platdata(struct udevice *dev)
{
	struct mt7621_pinctrl_priv *priv = dev_get_priv(dev);

	priv->base = (void __iomem *) devfdt_get_addr(dev);

	if (!priv->base)
		return -EINVAL;

	return 0;
}

static const struct udevice_id mt7621_pinctrl_ids[] = {
	{ .compatible = "mediatek,mt7621-pinctrl" },
	{ }
};

U_BOOT_DRIVER(mt7621_pinctrl) = {
	.name = "mt7621-pinctrl",
	.id = UCLASS_PINCTRL,
	.of_match = mt7621_pinctrl_ids,
	.ofdata_to_platdata = mt7621_pinctrl_ofdata_to_platdata,
	.ops = &mt7621_pinctrl_ops,
	.probe = mt7621_pinctrl_probe,
	.priv_auto_alloc_size = sizeof(struct mt7621_pinctrl_priv),
};