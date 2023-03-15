// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * drivers/amlogic/regulator/pmic6b_regulator.c
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

#include <linux/amlogic/pmic/meson_pmic6b.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>

static const struct linear_range pmic6b_buck1_voltage_ranges[] = {
	REGULATOR_LINEAR_RANGE(500000, 0x00, 0x38, 12500),
};

static const struct linear_range pmic6b_buck2_voltage_ranges[] = {
	REGULATOR_LINEAR_RANGE(800000, 0x00, 0x58, 12500),
};

static const struct linear_range pmic6b_buck3_voltage_ranges[] = {
	REGULATOR_LINEAR_RANGE(2900000, 0x0, 0x4, 200000),
};

/*
 *static const unsigned int pmic6b_buck3_voltage_table[] = {
 *	2900000, 3100000, 3300000, 3500000, 3700000
 *};
 */

static const struct linear_range pmic6b_ldo1_voltage_ranges[] = {
	REGULATOR_LINEAR_RANGE(1660000, 0x00, 0x0f, 36000),
};

static const struct linear_range pmic6b_ldo2_voltage_ranges[] = {
	REGULATOR_LINEAR_RANGE(700000, 0x01, 0x3f, 50000),
};

static const struct linear_range pmic6b_ldo3_voltage_ranges[] = {
	REGULATOR_LINEAR_RANGE(800000, 0x02, 0x3e, 50000),
};

int pmic6b_regulator_enable_regmap(struct regulator_dev *rdev)
{
	unsigned int val;

	if (rdev->desc->enable_is_inverted) {
		val = rdev->desc->disable_val;
	} else {
		val = rdev->desc->enable_val;
		if (!val)
			val = rdev->desc->enable_mask;
	}

	return regmap_update_bits(rdev->regmap, rdev->desc->enable_reg,
				rdev->desc->enable_mask, val);
}

int pmic6b_regulator_disable_regmap(struct regulator_dev *rdev)
{
	unsigned int val;

/*
 *	if (rdev->desc->enable_is_inverted) {
 *		val = rdev->desc->enable_val;
 *		if (!val)
 *			val = rdev->desc->enable_mask;
 *	} else {
 *		val = rdev->desc->disable_val;
 *	}
 */

	if (rdev->desc->enable_is_inverted) {
		val = rdev->desc->disable_val;
	} else {
		val = rdev->desc->enable_val;
		if (!val)
			val = rdev->desc->enable_mask;
	}
	/* disable reg is 0x42(0x41 +1),so use enable_reg + 1 */
	return regmap_update_bits(rdev->regmap, rdev->desc->enable_reg + 1,
			rdev->desc->enable_mask, val);
}

int pmic6b_regulator_list_voltage_regmap(struct regulator_dev *rdev,
	unsigned int selector)
{
	if (selector < rdev->desc->linear_min_sel)
		return 0;

	if (rdev->desc->id <= 1)
		selector -= rdev->desc->linear_min_sel;

	if (rdev->desc->id >= 3)
		selector = ((rdev->desc->linear_ranges)[0]).max_sel - selector;

	return rdev->desc->min_uV + (rdev->desc->uV_step * selector);
}

int pmic6b_regulator_get_voltage_sel_regmap(struct regulator_dev *rdev)
{
	unsigned int val;
	int ret;

	ret = regmap_read(rdev->regmap, rdev->desc->vsel_reg, &val);
	if (ret != 0)
		return ret;

	val &= rdev->desc->vsel_mask;
	val >>= ffs(rdev->desc->vsel_mask) - 1;

	if (rdev->desc->id == 2)
		val -= 0xb;
	return val;
}

int pmic6b_regulator_set_voltage_sel_regmap(struct regulator_dev *rdev,
	unsigned int sel)
{
	int ret = 0;

	if (rdev->desc->id == 2)
		sel += 0xb;
	sel <<= ffs(rdev->desc->vsel_mask) - 1;
	ret = regmap_update_bits(rdev->regmap, rdev->desc->vsel_reg,
		rdev->desc->vsel_mask, sel);
	if (ret)
		return ret;

	return ret;
}

static int pmic6b_dcdc3_get_voltage_sel(struct regulator_dev *rdev)
{
	unsigned int val;
	int ret;

	ret = regmap_read(rdev->regmap, rdev->desc->vsel_reg, &val);
	if (ret != 0)
		return ret;

	val &= rdev->desc->vsel_mask;
	val >>= ffs(rdev->desc->vsel_mask) - 1;

	val = val - 0xb;

	return val;
}

static int pmic6b_dcdc3_set_voltage_sel(struct regulator_dev *rdev,
	unsigned int sel)
{
	int ret = 0;

	sel = sel + 0xb;
	sel <<= ffs(rdev->desc->vsel_mask) - 1;
	ret = regmap_update_bits(rdev->regmap,
		rdev->desc->vsel_reg, rdev->desc->vsel_mask, sel);
	if (ret)
		return ret;

	return ret;
}

static const struct regulator_ops pmic6b_regulator_ops = {
	.enable			= pmic6b_regulator_enable_regmap,
	.disable		= pmic6b_regulator_disable_regmap,
	.is_enabled		= regulator_is_enabled_regmap,
	.set_voltage_sel = pmic6b_regulator_set_voltage_sel_regmap,
	.get_voltage_sel = pmic6b_regulator_get_voltage_sel_regmap,
	.list_voltage = pmic6b_regulator_list_voltage_regmap,
};

static const struct regulator_ops pmic6b_regulator_dcdc3_ops = {
	.enable			= pmic6b_regulator_enable_regmap,
	.disable		= pmic6b_regulator_disable_regmap,
	.is_enabled		= regulator_is_enabled_regmap,
	.set_voltage_sel = pmic6b_dcdc3_set_voltage_sel,
	.get_voltage = pmic6b_dcdc3_get_voltage_sel,
	.list_voltage = regulator_list_voltage_table,

};

static struct regulator_desc pmic6b_regulators[] = {
	{
		.name = "dcdc1",
		.of_match = of_match_ptr("DCDC1"),
		.regulators_node = of_match_ptr("regulators"),
		.id = PMIC6B_DCDC1,
		.ops = &pmic6b_regulator_ops,
		.type = REGULATOR_VOLTAGE,
		.n_voltages = PMIC6B_DCDC1_VOLTAGE_NUM,
		.linear_ranges = pmic6b_buck1_voltage_ranges,
		.n_linear_ranges = ARRAY_SIZE(pmic6b_buck1_voltage_ranges),
		.linear_min_sel = 0x0,
		.min_uV = 500000,
		.uV_step = 12500,
		.vsel_reg = PMIC6B_OTP_REG_0x0F,
		.vsel_mask = 0xff,
		.enable_reg = PMIC6B_BUCK_EN,
		.enable_mask = BIT(0),
		.owner = THIS_MODULE,
	},
	{
		.name = "dcdc2",
		.of_match = of_match_ptr("DCDC2"),
		.ops = &pmic6b_regulator_ops,
		.regulators_node = of_match_ptr("regulators"),
		.id = PMIC6B_DCDC2,
		.type = REGULATOR_VOLTAGE,
		.n_voltages = PMIC6B_DCDC2_VOLTAGE_NUM,
		.linear_ranges = pmic6b_buck2_voltage_ranges,
		.n_linear_ranges = ARRAY_SIZE(pmic6b_buck2_voltage_ranges),
		.linear_min_sel = 0x0,
		.min_uV = 800000,
		.uV_step = 12500,
		.vsel_reg = PMIC6B_OTP_REG_0x15,
		.vsel_mask = 0xff00,
		.enable_reg = PMIC6B_BUCK_EN,
		.enable_mask = BIT(1),
		.owner = THIS_MODULE,
	},
	{
		.name = "dcdc3",
		.of_match = of_match_ptr("DCDC3"),
		.regulators_node = of_match_ptr("regulators"),
		.id = PMIC6B_DCDC3,
//		.ops = &pmic6b_regulator_dcdc3_ops,
		.ops = &pmic6b_regulator_ops,
		.type = REGULATOR_VOLTAGE,
		.n_voltages = PMIC6B_DCDC3_VOLTAGE_NUM,
		.linear_ranges = pmic6b_buck3_voltage_ranges,
		.n_linear_ranges = ARRAY_SIZE(pmic6b_buck3_voltage_ranges),
//		.n_voltages	= ARRAY_SIZE(pmic6b_buck3_voltage_table),
//		.volt_table	 = pmic6b_buck3_voltage_table,
		.linear_min_sel = 0x0,
		.min_uV = 2900000,
		.uV_step = 200000,
		.vsel_reg = PMIC6B_OTP_REG_0x1C,
		.vsel_mask = 0x3c0,
		.enable_reg = PMIC6B_BUCK_EN,
		.enable_mask = BIT(2),
		.owner = THIS_MODULE,
	},
	{
		.name = "ldo1",
		.of_match = of_match_ptr("LDO1"),
		.regulators_node = of_match_ptr("regulators"),
		.id = PMIC6B_LDO1,
		.ops = &pmic6b_regulator_ops,
		.type = REGULATOR_VOLTAGE,
		.n_voltages = PMIC6B_LDO1_VOLTAGE_NUM,
		.linear_ranges = pmic6b_ldo1_voltage_ranges,
		.n_linear_ranges = ARRAY_SIZE(pmic6b_ldo1_voltage_ranges),
		.linear_min_sel = 0x00,
		.min_uV = 1660000,
		.uV_step = 36000,
		.vsel_reg = PMIC6B_OTP_REG_0x1E,
		.vsel_mask = 0xf000,
		.enable_reg = PMIC6B_BUCK_EN,
		.enable_mask = BIT(3),
		.owner = THIS_MODULE,
	},
	{
		.name = "ldo2",
		.of_match = of_match_ptr("LDO2"),
		.regulators_node = of_match_ptr("regulators"),
		.id = PMIC6B_LDO2,
		.ops = &pmic6b_regulator_ops,
		.type = REGULATOR_VOLTAGE,
		.n_voltages = PMIC6B_LDO2_VOLTAGE_NUM,
		.linear_ranges = pmic6b_ldo2_voltage_ranges,
		.n_linear_ranges = ARRAY_SIZE(pmic6b_ldo2_voltage_ranges),
		.linear_min_sel = 0x01,
		.min_uV = 700000,
		.uV_step = 50000,
		.vsel_reg = PMIC6B_OTP_REG_0x20,
		.vsel_mask = 0x3f,
		.enable_reg = PMIC6B_BUCK_EN,
		.enable_mask = BIT(4),
		.owner = THIS_MODULE,
	},
	{
		.name = "ldo3",
		.of_match = of_match_ptr("LDO3"),
		.regulators_node = of_match_ptr("regulators"),
		.id = PMIC6B_LDO3,
		.ops = &pmic6b_regulator_ops,
		.type = REGULATOR_VOLTAGE,
		.n_voltages = PMIC6B_LDO3_VOLTAGE_NUM,
		.linear_ranges = pmic6b_ldo3_voltage_ranges,
		.n_linear_ranges = ARRAY_SIZE(pmic6b_ldo3_voltage_ranges),
		.linear_min_sel = 0x02,
		.min_uV = 800000,
		.uV_step = 50000,
		.vsel_reg = PMIC6B_OTP_REG_0x21,
		.vsel_mask = 0x1f80,
		.enable_reg = PMIC6B_BUCK_EN,
		.enable_mask = BIT(5),
		.owner = THIS_MODULE,
	},
};

static int meson_pmic6b_regulator_probe(struct platform_device *pdev)
{
	struct meson_pmic6b *meson_pmic = dev_get_drvdata(pdev->dev.parent);
	struct regulator_config config = { };
	struct regulator_dev *rdev;
	struct regulator_desc *regulators;
	int i;
	int num_regulators = 0;

	regulators = pmic6b_regulators;
	num_regulators = ARRAY_SIZE(pmic6b_regulators);

	config.dev = pdev->dev.parent;
	config.regmap = meson_pmic->regmap;

	for (i = 0; i < num_regulators; i++) {
		rdev = devm_regulator_register(&pdev->dev,
					       &pmic6b_regulators[i],
					       &config);
		if (IS_ERR(rdev)) {
			dev_err(&pdev->dev, "failed to register %s regulator\n",
				regulators[i].name);
			continue;
		}
	}

	return 0;
}

static struct platform_driver pmic6b_regulator_driver = {
	.probe = meson_pmic6b_regulator_probe,
	.driver = {
		.name	= PMIC6B_DRVNAME_REGULATORS,
	},
};

int __init meson_pmic6b_regulator_init(void)
{
	int ret;

	ret = platform_driver_register(&pmic6b_regulator_driver);
	return ret;
}

void __exit meson_pmic6b_regulator_exit(void)
{
	platform_driver_unregister(&pmic6b_regulator_driver);
}

MODULE_AUTHOR("Amlogic");
MODULE_DESCRIPTION("PMIC6B regulator driver");
MODULE_LICENSE("GPL v2");
