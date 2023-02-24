// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/gpio.h>
#include <linux/pinctrl/pinconf-generic.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/types.h>
#include <linux/module.h>
#include <dt-bindings/gpio/audio-gpio.h>

#include "../regs.h"
#include "../audio_io.h"
#include "../iomap.h"

#define DRV_NAME "pinctrl-audio-g12a"

#define AUDIO_PIN(x) PINCTRL_PIN(x, #x)

static const struct pinctrl_pin_desc g12a_audio_pins[] = {
	AUDIO_PIN(TDM_SCLK0),
	AUDIO_PIN(TDM_SCLK1),
	AUDIO_PIN(TDM_SCLK2),
	AUDIO_PIN(TDM_LRCLK0),
	AUDIO_PIN(TDM_LRCLK1),
	AUDIO_PIN(TDM_LRCLK2),
};

struct pin_group {
	const char *name;
	const unsigned int *pins;
	const unsigned int num_pins;
};

/* tdm sclk pins */
static const unsigned int tdm_sclk0_pins[] = {TDM_SCLK0};
static const unsigned int tdm_sclk1_pins[] = {TDM_SCLK1};
static const unsigned int tdm_sclk2_pins[] = {TDM_SCLK2};

/* tdm lrclk pins */
static const unsigned int tdm_lrclk0_pins[] = {TDM_LRCLK0};
static const unsigned int tdm_lrclk1_pins[] = {TDM_LRCLK1};
static const unsigned int tdm_lrclk2_pins[] = {TDM_LRCLK2};

#define GROUP(n)  \
	{			\
		.name = #n,	\
		.pins = n ## _pins,	\
		.num_pins = ARRAY_SIZE(n ## _pins),	\
	}

static const struct pin_group g12a_audio_pin_groups[] = {
	GROUP(tdm_sclk0),
	GROUP(tdm_sclk1),
	GROUP(tdm_sclk2),
	GROUP(tdm_lrclk0),
	GROUP(tdm_lrclk1),
	GROUP(tdm_lrclk2),
};

static int g12a_ap_get_groups_count(struct pinctrl_dev *pctldev)
{
	return ARRAY_SIZE(g12a_audio_pin_groups);
}

static const char *g12a_ap_get_group_name(struct pinctrl_dev *pctldev,
	       unsigned int selector)
{
	return g12a_audio_pin_groups[selector].name;
}

static int g12a_ap_get_group_pins(struct pinctrl_dev *pctldev, unsigned int selector,
	       const unsigned int **pins, unsigned int *num_pins)
{
	*pins = (unsigned int *)g12a_audio_pin_groups[selector].pins;
	*num_pins = g12a_audio_pin_groups[selector].num_pins;
	return 0;
}

struct audio_pmx_func {
	const char *name;
	const char * const *groups;
	const unsigned int num_groups;
};

static const char * const tdm_clk_groups[] = {
	"tdm_sclk0",
	"tdm_sclk1",
	"tdm_sclk2",
	"tdm_lrclk0",
	"tdm_lrclk1",
	"tdm_lrclk2"
};

#define FUNCTION(n, g)				\
	{					\
		.name = #n,			\
		.groups = g ## _groups,		\
		.num_groups = ARRAY_SIZE(g ## _groups),	\
	}

static const struct audio_pmx_func g12a_audio_functions[] = {
	FUNCTION(tdm_clk_outa, tdm_clk),
	FUNCTION(tdm_clk_outb, tdm_clk),
	FUNCTION(tdm_clk_outc, tdm_clk),
	FUNCTION(tdm_clk_outd, tdm_clk),
	FUNCTION(tdm_clk_oute, tdm_clk),
	FUNCTION(tdm_clk_outf, tdm_clk),
};

static int g12a_ap_get_functions_count(struct pinctrl_dev *pctldev)
{
	return ARRAY_SIZE(g12a_audio_functions);
}

static const char *g12a_ap_get_fname(struct pinctrl_dev *pctldev,
		unsigned int selector)
{
	return g12a_audio_functions[selector].name;
}

static int g12a_ap_get_groups(struct pinctrl_dev *pctldev, unsigned int selector,
			  const char * const **groups,
			  unsigned int * const num_groups)
{
	*groups = g12a_audio_functions[selector].groups;
	*num_groups = g12a_audio_functions[selector].num_groups;

	return 0;
}

struct ap_data {
	struct aml_audio_controller *actrl;
	struct device *dev;
};

/* refer to g12a_audio_functions[]:selector */
#define FUNC_TDM_CLK_OUT_START     0
#define FUNC_TDM_CLK_OUT_LAST      5

static int g12a_ap_set_mux(struct pinctrl_dev *pctldev,
		unsigned int selector, unsigned int group)
{
	struct ap_data *ap = pinctrl_dev_get_drvdata(pctldev);
	struct aml_audio_controller *actrl = ap->actrl;
	unsigned int addr = 0, offset = 0, val = 0;

	if (selector <= FUNC_TDM_CLK_OUT_LAST) {
		addr = EE_AUDIO_MST_PAD_CTRL1(0);
		offset = ((group >= 3) ? (group + 1) : group) * 4;
		val = selector;
		aml_audiobus_update_bits(actrl, addr,
			0x7 << offset, val << offset);
	} else {
		dev_err(ap->dev, "%s() unsupport selector: %d, grp %d\n",
			__func__, selector, group);
	}
	pr_debug("%s(), selector %d, group %d, addr %#x, offset %d, val %d\n",
		__func__, selector, group, addr, offset, val);
	return 0;
}

static int g12a_ap_pmx_request(struct pinctrl_dev *pctldev, unsigned int offset)
{
	pr_debug("%s(), offset %d\n", __func__, offset);
	return 0;
}

static const struct pinmux_ops g12a_ap_pmxops = {
	.request = g12a_ap_pmx_request,
	.get_functions_count = g12a_ap_get_functions_count,
	.get_function_name = g12a_ap_get_fname,
	.get_function_groups = g12a_ap_get_groups,
	.set_mux = g12a_ap_set_mux,
	.strict = true,
};

static void audio_pin_dbg_show(struct pinctrl_dev *pcdev, struct seq_file *s,
			       unsigned int offset)
{
	seq_printf(s, " %s %d\n", __func__, offset);
}

static const struct pinctrl_ops g12a_audio_pctrl_ops = {
	.get_groups_count	= g12a_ap_get_groups_count,
	.get_group_name		= g12a_ap_get_group_name,
	.get_group_pins		= g12a_ap_get_group_pins,
	.dt_node_to_map		= pinconf_generic_dt_node_to_map_all,
	.dt_free_map		= pinconf_generic_dt_free_map,
	.pin_dbg_show		= audio_pin_dbg_show,
};

struct pinctrl_desc g12a_audio_pin_desc = {
	.name = DRV_NAME,
	.pctlops = &g12a_audio_pctrl_ops,
	.pmxops = &g12a_ap_pmxops,
	.pins = g12a_audio_pins,
	.npins = ARRAY_SIZE(g12a_audio_pins),
	.owner = THIS_MODULE,
};

static int g12a_audio_pinctrl_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	struct device_node *node_prt = NULL;
	struct platform_device *pdev_parent;
	struct pinctrl_dev *ap_dev;
	struct ap_data *ap;

	ap = devm_kzalloc(dev, sizeof(*ap), GFP_KERNEL);
	if (!ap)
		return -ENOMEM;

	ap->dev = dev;
	/* get audio controller */
	node_prt = of_get_parent(node);
	if (!node_prt) {
		dev_err(dev, "%s() line %d err\n", __func__, __LINE__);
		return -ENXIO;
	}

	pdev_parent = of_find_device_by_node(node_prt);
	of_node_put(node_prt);
	ap->actrl = (struct aml_audio_controller *)
				platform_get_drvdata(pdev_parent);
	if (!ap->actrl) {
		dev_err(dev, "%s() line %d err\n", __func__, __LINE__);
		return -ENXIO;
	}

	ap_dev = devm_pinctrl_register(&pdev->dev, &g12a_audio_pin_desc, ap);
	if (IS_ERR(ap_dev)) {
		dev_err(dev, "%s() line %d err\n", __func__, __LINE__);
		return PTR_ERR(ap_dev);
	}

	return 0;
}

static const struct of_device_id g12a_audio_pinctrl_of_match[] = {
	{
		.compatible = "amlogic, g12a-audio-pinctrl",
	},
	{}
};
MODULE_DEVICE_TABLE(of, g12a_audio_pinctrl_of_match);

static struct platform_driver g12a_audio_pinctrl_driver = {
	.driver = {
		.name           = DRV_NAME,
		.of_match_table = g12a_audio_pinctrl_of_match,
	},
	.probe  = g12a_audio_pinctrl_probe,
};

int __init g12a_audio_pinctrl_init(void)
{
	return platform_driver_register(&g12a_audio_pinctrl_driver);
}

void __exit g12a_audio_pinctrl_exit(void)
{
	platform_driver_unregister(&g12a_audio_pinctrl_driver);
}

#ifndef MODULE
module_init(g12a_audio_pinctrl_init);
module_exit(g12a_audio_pinctrl_exit);
MODULE_AUTHOR("Amlogic, Inc.");
MODULE_DESCRIPTION("Amlogic g12a audio pinctrl driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" DRV_NAME);
#endif

