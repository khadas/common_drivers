// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * drivers/amlogic/gpio/pmic6b_gpio.c
 *
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/regmap.h>
#include <linux/of.h>

#include <linux/amlogic/pmic/meson_pmic6b.h>

enum {
	PMIC6B_GPIO1,
	PMIC6B_GPIO2,
	PMIC6B_GPIO3,
	PMIC6B_GPIO4,

	PMIC6B_MAX_GPIO,
};

struct pmic6b_gpio {
	struct gpio_chip gpio_chip;
	struct regmap *regmap;
	struct device *dev;
};

static int pmic6b_gpio_get(struct gpio_chip *gc, unsigned int offset)
{
	struct pmic6b_gpio *pmic6b_gpio = gpiochip_get_data(gc);
	unsigned int val = 0, temp = 0;
	int ret;
	int i_offset = 0;

	ret = regmap_read(pmic6b_gpio->regmap, PMIC6B_PIN_MUX_REG8, &temp);
	if (ret < 0) {
		dev_err(pmic6b_gpio->dev, "Failed in: %d\n", __LINE__);
		return ret;
	}

	switch (offset) {
	case 0:
		i_offset = offset + 0;
		break;
	case 1:
		i_offset = offset + 1;
		break;
	case 2:
		i_offset = offset + 2;
		break;
	case 3:
		i_offset = offset + 3;
		break;
	default:
		dev_err(pmic6b_gpio->dev, "offset fault in: %d\n", __LINE__);
		break;
	}

	if (temp & BIT(i_offset)) {
		ret = regmap_read(pmic6b_gpio->regmap,
					PMIC6B_GPIO_Input_LEVEL, &val);
		if (ret < 0) {
			dev_err(pmic6b_gpio->dev, "Failed in: %d\n", __LINE__);
			return ret;
		}

	} else {
		ret = regmap_read(pmic6b_gpio->regmap,
					PMIC6B_GPIO_OUTPUT_CONTROL, &val);

		if (ret < 0) {
			dev_err(pmic6b_gpio->dev, "Failed in: %d\n", __LINE__);
			return ret;
		}
	}

	return !!(val & BIT(offset));
}

static void pmic6b_gpio_set(struct gpio_chip *gc, unsigned int offset, int val)
{
	struct pmic6b_gpio *pmic6b_gpio = gpiochip_get_data(gc);
	int ret;

	if (val) {
		ret = regmap_update_bits(pmic6b_gpio->regmap,
			PMIC6B_GPIO_OUTPUT_CONTROL, BIT(offset), BIT(offset));
		if (ret)
			dev_err(pmic6b_gpio->dev, "Failed in: %d\n", __LINE__);
	} else {
		ret = regmap_update_bits(pmic6b_gpio->regmap,
			PMIC6B_GPIO_OUTPUT_CONTROL, BIT(offset), 0);
		if (ret)
			dev_err(pmic6b_gpio->dev, "Failed in: %d\n", __LINE__);
	}
}

static int pmic6b_gpio_dir_input(struct gpio_chip *gc, unsigned int offset)
{
	struct pmic6b_gpio *pmic6b_gpio = gpiochip_get_data(gc);
	int ret;
	int i_offset = 0, o_offset = 0;

	switch (offset) {
	case 0:
		o_offset = offset + 1;
		i_offset = offset + 0;
		break;
	case 1:
		o_offset = offset + 2;
		i_offset = offset + 1;
		break;
	case 2:
		o_offset = offset + 3;
		i_offset = offset + 2;
		break;
	case 3:
		o_offset = offset + 4;
		i_offset = offset + 3;
		break;
	default:
		dev_err(pmic6b_gpio->dev, "offset fault in: %d\n", __LINE__);
		break;
	}
	/*disable output bit*/
	ret = regmap_update_bits(pmic6b_gpio->regmap,
				PMIC6B_PIN_MUX_REG8, BIT(o_offset), 0);
	if (ret < 0) {
		dev_err(pmic6b_gpio->dev,
			"Failed in: %d\n", __LINE__);
		return ret;
	}

	usleep_range(10, 15);

	ret = regmap_update_bits(pmic6b_gpio->regmap,
			PMIC6B_PIN_MUX_REG8, BIT(i_offset), BIT(i_offset));
	if (ret < 0) {
		dev_err(pmic6b_gpio->dev, "Failed in: %d\n", __LINE__);
		return ret;
	}

	return 0;
}

static int pmic6b_gpio_dir_output(struct gpio_chip *gc, unsigned int offset,
				 int value)
{
	struct pmic6b_gpio *pmic6b_gpio = gpiochip_get_data(gc);
	int ret;
	int i_offset = 0, o_offset = 0;

	switch (offset) {
	case 0:
		o_offset = offset + 1;
		i_offset = offset + 0;  //input = bit0 / output = bit1
		break;
	case 1:
		o_offset = offset + 2;
		i_offset = offset + 1;  //input = bit2 / output = 3
		break;
	case 2:
		o_offset = offset + 3;
		i_offset = offset + 2; //input = bit4 / output = bit5
		break;
	case 3:
		o_offset = offset + 4;
		i_offset = offset + 3; //input = bit6 /output = bit7
		break;
	default:
		dev_err(pmic6b_gpio->dev,
			"offset fault in: %d\n", __LINE__);
		break;
	}

	pmic6b_gpio_set(gc, offset, value);
	/*disable gpio input bit*/
	ret = regmap_update_bits(pmic6b_gpio->regmap,
				PMIC6B_PIN_MUX_REG8, BIT(i_offset), 0);
	if (ret < 0) {
		dev_err(pmic6b_gpio->dev, "Failed in: %d\n", __LINE__);
		return ret;
	}

	usleep_range(10, 15);

	ret = regmap_update_bits(pmic6b_gpio->regmap,
			PMIC6B_PIN_MUX_REG8, BIT(o_offset), BIT(o_offset));
	if (ret < 0) {
		dev_err(pmic6b_gpio->dev, "Failed in: %d\n", __LINE__);
		return ret;
	}

	return 0;
}

static int pmic6b_gpio_probe(struct platform_device *pdev)
{
	struct meson_pmic6b *pmic6 = dev_get_drvdata(pdev->dev.parent);
	struct pmic6b_gpio *pmic6b_gpio;

	pmic6b_gpio = devm_kzalloc(&pdev->dev,
				sizeof(*pmic6b_gpio), GFP_KERNEL);
	if (!pmic6b_gpio)
		return -ENOMEM;

	pmic6b_gpio->dev = &pdev->dev;
	pmic6b_gpio->regmap = pmic6->regmap;
	pmic6b_gpio->gpio_chip.label = "pmic6b-gpio",
	pmic6b_gpio->gpio_chip.owner = THIS_MODULE,
	pmic6b_gpio->gpio_chip.direction_input = pmic6b_gpio_dir_input,
	pmic6b_gpio->gpio_chip.direction_output = pmic6b_gpio_dir_output,
	pmic6b_gpio->gpio_chip.set = pmic6b_gpio_set,
	pmic6b_gpio->gpio_chip.get = pmic6b_gpio_get,
	pmic6b_gpio->gpio_chip.ngpio = PMIC6B_MAX_GPIO,
	pmic6b_gpio->gpio_chip.can_sleep = true,
	pmic6b_gpio->gpio_chip.parent = &pdev->dev;
	pmic6b_gpio->gpio_chip.base = -1;
	platform_set_drvdata(pdev, pmic6b_gpio);

	return devm_gpiochip_add_data(&pdev->dev, &pmic6b_gpio->gpio_chip,
				      pmic6b_gpio);
}

static const struct of_device_id pmic6b_gpio_match_table[] = {
	{ .compatible = "amlogic,pmic6b-gpio"},
	{ },
};
MODULE_DEVICE_TABLE(of, pmic6b_gpio_match_table);

static struct platform_driver pmic6b_gpio_driver = {
	.driver = {
		.name    = "pmic6b-gpio",
		.of_match_table = pmic6b_gpio_match_table,
	},
	.probe		= pmic6b_gpio_probe,
};

int __init meson_pmic6b_gpio_init(void)
{
	return platform_driver_register(&pmic6b_gpio_driver);
}

void __exit meson_pmic6b_gpio_exit(void)
{
	platform_driver_unregister(&pmic6b_gpio_driver);
}

MODULE_AUTHOR("Amlogic");
MODULE_DESCRIPTION("PMIC6B gpio driver");
MODULE_LICENSE("GPL v2");

