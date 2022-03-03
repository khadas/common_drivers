// SPDX-License-Identifier: GPL-2.0
/*
 * Audio clock driver
 *
 * Copyright (C) 2019 Amlogic,inc
 *
 */

#undef pr_fmt
#define pr_fmt(fmt) "audio_clocks: " fmt

#include <linux/of_device.h>

#include "audio_clks.h"

#define DRV_NAME "audio-clocks"

static const struct of_device_id audio_clocks_of_match[] = {
#ifdef __KERNEL_419_AUDIO__
	{
		.compatible = "amlogic, axg-audio-clocks",
		.data       = &axg_audio_clks_init,
	},
	{
		.compatible = "amlogic, g12a-audio-clocks",
		.data       = &g12a_audio_clks_init,
	},
	{
		.compatible = "amlogic, tl1-audio-clocks",
		.data       = &tl1_audio_clks_init,
	},
	{
		.compatible = "amlogic, sm1-audio-clocks",
		.data		= &sm1_audio_clks_init,
	},
	{
		.compatible = "amlogic, tm2-audio-clocks",
		.data		= &tm2_audio_clks_init,
	},
#else
	{
		.compatible = "amlogic, g12a-audio-clocks",
		.data		= &g12a_audio_clks_init,
	},
	{
		.compatible = "amlogic, sm1-audio-clocks",
		.data		= &sm1_audio_clks_init,
	},
	{
		.compatible = "amlogic, a1-audio-clocks",
		.data		= &a1_audio_clks_init,
	},
#endif
	{},
};
MODULE_DEVICE_TABLE(of, audio_clocks_of_match);

static int audio_clocks_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct clk **clks;
	struct clk_onecell_data *clk_data = NULL;
	void __iomem *clk_base = NULL, *clk_base2 = NULL;
	struct audio_clk_init *p_audioclk_init = NULL;
	int ret;

	p_audioclk_init = (struct audio_clk_init *)
		of_device_get_match_data(dev);
	if (!p_audioclk_init) {
		dev_warn_once(dev,
			      "check whether to update audio clock chipinfo\n");
		return -EINVAL;
	}

	clk_base = of_iomap(np, 0);
	if (!clk_base) {
		dev_err(dev,
			"Unable to map clk base\n");
		return -ENXIO;
	}

	if (p_audioclk_init->clk2_gates && p_audioclk_init->clks2) {
		clk_base2 = of_iomap(np, 1);
		if (!clk_base2) {
			dev_err(dev,
				"Unable to map clk base2\n");
			return -ENXIO;
		}
		dev_dbg(dev, "map clk base2\n");
	}

	clk_data = devm_kmalloc(dev, sizeof(*clk_data), GFP_KERNEL);
	if (!clk_data)
		return -ENOMEM;

	clks = devm_kmalloc(dev,
			    p_audioclk_init->clk_num * sizeof(*clks),
			    GFP_KERNEL);
	if (!clks)
		return -ENOMEM;

	if (p_audioclk_init->clk_gates)
		p_audioclk_init->clk_gates(clks, clk_base);
	if (p_audioclk_init->clks)
		p_audioclk_init->clks(clks, clk_base);
	if (p_audioclk_init->clk2_gates && clk_base2)
		p_audioclk_init->clk2_gates(clks, clk_base2);
	if (p_audioclk_init->clks2 && clk_base2)
		p_audioclk_init->clks2(clks, clk_base2);

	clk_data->clks = clks;
	clk_data->clk_num = p_audioclk_init->clk_num;

	ret = of_clk_add_provider(np, of_clk_src_onecell_get,
				  clk_data);
	if (ret) {
		dev_err(dev, "%s fail ret: %d\n", __func__, ret);

		return ret;
	}

	return 0;
}

static struct platform_driver audio_clocks_driver = {
	.driver = {
		.name           = DRV_NAME,
		.of_match_table = audio_clocks_of_match,
	},
	.probe  = audio_clocks_probe,
};

#ifdef MODULE
int __init audio_clocks_init(void)
{
	return platform_driver_register(&audio_clocks_driver);
}

void __exit audio_clocks_exit(void)
{
	platform_driver_unregister(&audio_clocks_driver);
}
#else
module_platform_driver(audio_clocks_driver);

MODULE_AUTHOR("Amlogic, Inc.");
MODULE_DESCRIPTION("Amlogic audio clocks ASoc driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" DRV_NAME);
#endif
