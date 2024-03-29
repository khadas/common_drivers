// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

//#define DEBUG
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/printk.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/cpufreq.h>
#include <linux/cpu_cooling.h>
#include "cpucore_cooling.h"
#include <linux/amlogic/gpucore_cooling.h>
#include <linux/amlogic/gpu_cooling.h>
#include <linux/amlogic/ddr_cooling.h>
#include <linux/amlogic/meson_cooldev.h>
#include <linux/amlogic/media_cooling.h>
#include <linux/cpu.h>

enum cluster_type {
	CLUSTER_BIG = 0,
	CLUSTER_LITTLE,
	NUM_CLUSTERS
};

char *cooldev_name[COOL_DEV_TYPE_MAX] = {"cpucore", "gpufreq", "gpucore", "ddr", "media"};

struct meson_cooldev {
	int cool_dev_num;
	struct mutex lock;/*cooling devices mutexlock*/
	struct cool_dev *cool_devs;
	struct thermal_zone_device    *tzd;
};

static struct meson_cooldev *meson_gcooldev;

int get_cool_dev_type(char *type)
{
	int i;

	for (i = 0; i < COOL_DEV_TYPE_MAX; i++) {
		if (!strcmp(type, cooldev_name[i]))
			return i;
	}

	return COOL_DEV_TYPE_MAX;
}

int meson_get_cooldev_type(struct thermal_cooling_device *cdev)
{
	int i;

	for (i = 0; i < COOL_DEV_TYPE_MAX; i++) {
		if (strstr(cdev->type, cooldev_name[i]))
			return get_cool_dev_type(cooldev_name[i]);
	}
	return COOL_DEV_TYPE_MAX;
}

int meson_gcooldev_min_update(struct thermal_cooling_device *cdev)
{
	return 0;
}
EXPORT_SYMBOL(meson_gcooldev_min_update);

static int register_cool_dev(struct platform_device *pdev,
			     int index, struct device_node *child)
{
	struct meson_cooldev *mcooldev = platform_get_drvdata(pdev);
	struct cool_dev *cool = &mcooldev->cool_devs[index];
	struct device_node *node;
	int pp, coeff, i;
	const char *node_name;
	char *ddrdata_name[2] = {"ddr_data", "gpu_data"};

	pr_debug("meson_cdev index: %d %s\n", index, cool->device_type);

	if (of_property_read_string(child, "node_name", &node_name)) {
		pr_err("thermal: read node_name failed\n");
		return -EINVAL;
	}

	switch (get_cool_dev_type(cool->device_type)) {
	case COOL_DEV_TYPE_CPU_CORE:
		node = of_find_node_by_name(NULL, node_name);
		if (!node) {
			pr_err("thermal: can't find node\n");
			return -EINVAL;
		}
		cool->np = node;

		cool->cooling_dev = cpucore_cooling_register(cool->np, child);
		break;
	case COOL_DEV_TYPE_DDR:
		node = of_find_node_by_name(NULL, node_name);
		if (!node) {
			pr_err("thermal: can't find node\n");
			return -EINVAL;
		}
		cool->np = node;
		cool->ddr_reg_cnt = of_property_count_u32_elems(child, "ddr_reg");
		if (cool->ddr_reg_cnt < 1)
			return -EINVAL;
		cool->ddr_reg = devm_kzalloc(&pdev->dev, sizeof(u32) * cool->ddr_reg_cnt,
			GFP_KERNEL);
		cool->ddr_bits = devm_kzalloc(&pdev->dev, sizeof(u32) * 2 * cool->ddr_reg_cnt,
			GFP_KERNEL);
		cool->ddr_data = devm_kzalloc(&pdev->dev, sizeof(u32) * 20 * cool->ddr_reg_cnt,
			GFP_KERNEL);

		if (of_property_read_u32_array(child, "ddr_reg", cool->ddr_reg, cool->ddr_reg_cnt))
			return -EINVAL;

		if (of_property_read_u32(child, "ddr_status", &cool->ddr_status)) {
			pr_err("thermal: read ddr reg_status failed\n");
			return -EINVAL;
		}

		if (of_property_read_u32_array(child, "ddr_bits",
					       cool->ddr_bits[0], 2 * cool->ddr_reg_cnt)) {
			pr_err("thermal: read ddr_bits failed\n");
			return -EINVAL;
		}

		for (i = 0; i < cool->ddr_reg_cnt; i++) {
			if (of_property_read_u32_array(child, ddrdata_name[i],
						       cool->ddr_data[i], cool->ddr_status)) {
				pr_err("thermal: read ddr_data failed\n");
				return -EINVAL;
			}
		}

		cool->cooling_dev = ddr_cooling_register(cool->np, cool);
		break;
	/* GPU is KO, just save these parameters */
	case COOL_DEV_TYPE_GPU_FREQ:
		node = of_find_node_by_name(NULL, node_name);
		if (!node) {
			pr_err("thermal: can't find node\n");
			return -EINVAL;
		}
		cool->np = node;
		if (of_property_read_u32(cool->np, "num_of_pp", &pp)) {
			pr_err("thermal: read num_of_pp failed\n");
			return -EINVAL;
		}

		if (of_property_read_u32(child, "dyn_coeff", &coeff)) {
			pr_err("thermal: read dyn_coeff failed\n");
			return -EINVAL;
		}
		save_gpu_cool_para(coeff, cool->np, pp);
		return 0;

	case COOL_DEV_TYPE_GPU_CORE:
		node = of_find_node_by_name(NULL, node_name);
		if (!node) {
			pr_err("thermal: can't find node\n");
			return -EINVAL;
		}
		cool->np = node;
		save_gpucore_thermal_para(cool->np);
		return 0;

	case COOL_DEV_TYPE_MEDIA:
		if (setup_media_para(node_name))
			return -EINVAL;
		break;

	default:
		pr_err("thermal: unknown type:%s\n", cool->device_type);
		return -EINVAL;
	}

	if (IS_ERR(cool->cooling_dev)) {
		pr_err("thermal: register %s failed\n", cool->device_type);
		cool->cooling_dev = NULL;
		return -EINVAL;
	}
	return 0;
}

static int parse_cool_device(struct platform_device *pdev)
{
	struct meson_cooldev *mcooldev = platform_get_drvdata(pdev);
	struct device_node *np = pdev->dev.of_node;
	int i, ret = 0;
	struct cool_dev *cool;
	struct device_node *child;
	const char *str;

	child = of_get_child_by_name(np, "cooling_devices");
	if (!child) {
		pr_err("meson cooldev: can't found cooling_devices\n");
		return -EINVAL;
	}
	mcooldev->cool_dev_num = of_get_child_count(child);
	i = sizeof(struct cool_dev) * mcooldev->cool_dev_num;
	mcooldev->cool_devs = kzalloc(i, GFP_KERNEL);
	if (!mcooldev->cool_devs) {
		pr_err("meson cooldev: alloc mem failed\n");
		return -ENOMEM;
	}

	child = of_get_next_child(child, NULL);
	for (i = 0; i < mcooldev->cool_dev_num; i++) {
		cool = &mcooldev->cool_devs[i];
		if (!child)
			break;
		if (of_property_read_string(child, "device_type", &str))
			pr_err("thermal: read device_type failed\n");
		else
			cool->device_type = (char *)str;

		ret += register_cool_dev(pdev, i, child);
		child = of_get_next_child(np, child);
	}
	return ret;
}

static int meson_cooldev_probe(struct platform_device *pdev)
{
	struct meson_cooldev *mcooldev;

	pr_debug("meson_cdev probe\n");
	mcooldev = devm_kzalloc(&pdev->dev,
				sizeof(struct meson_cooldev),
				GFP_KERNEL);
	if (!mcooldev)
		return -ENOMEM;
	platform_set_drvdata(pdev, mcooldev);
	mutex_init(&mcooldev->lock);

	if (parse_cool_device(pdev))
		pr_info("meson_cdev one or more cooldev register fail\n");
	/*save pdev for mali ko api*/
	meson_gcooldev = platform_get_drvdata(pdev);

	pr_debug("meson_cdev probe done\n");
	return 0;
}

static int meson_cooldev_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id meson_cooldev_of_match[] = {
	{ .compatible = "amlogic, meson-cooldev" },
	{},
};

struct platform_driver meson_cooldev_platdrv = {
	.driver = {
		.name		= "meson-cooldev",
		.owner		= THIS_MODULE,
		.of_match_table = meson_cooldev_of_match,
	},
	.probe	= meson_cooldev_probe,
	.remove	= meson_cooldev_remove,
};
