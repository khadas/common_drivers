// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * drivers/amlogic/mfd/meson_pmic6b.c
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/regmap.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/mfd/core.h>
#include <linux/of.h>
#include <linux/regulator/of_regulator.h>
#include <linux/gpio/consumer.h>

#include <linux/amlogic/pmic/meson_pmic6b.h>

#define	PMIC6B_IRQ_MASK0_OFFSET		0
#define	PMIC6B_IRQ_MASK1_OFFSET		1

static const struct regmap_range meson_pmic6b_readable_ranges[] = {
	regmap_reg_range(PMIC6B_POWER_UP_SEL0, PMIC6B_REG_MAX),
};

static const struct regmap_range meson_pmic6b_writeable_ranges[] = {
	regmap_reg_range(PMIC6B_POWER_UP_SEL0, PMIC6B_REG_MAX),
};

static const struct regmap_range meson_pmic6b_volatile_ranges[] = {
	regmap_reg_range(PMIC6B_POWER_UP_SEL0, PMIC6B_REG_MAX),
};

static const struct regmap_access_table meson_pmic6b_readable_table = {
	.yes_ranges = meson_pmic6b_readable_ranges,
	.n_yes_ranges = ARRAY_SIZE(meson_pmic6b_readable_ranges),
};

static const struct regmap_access_table meson_pmic6b_writeable_table = {
	.yes_ranges = meson_pmic6b_writeable_ranges,
	.n_yes_ranges = ARRAY_SIZE(meson_pmic6b_writeable_ranges),
};

static const struct regmap_access_table meson_pmic6b_volatile_table = {
	.yes_ranges = meson_pmic6b_volatile_ranges,
	.n_yes_ranges = ARRAY_SIZE(meson_pmic6b_volatile_ranges),
};

static bool meson_pmic6b_volatile_reg(struct device *dev, unsigned int reg)
{
	return false;
}

static struct regmap_config meson_pmic6b_regmap_config = {
	.reg_bits = 16,
	.val_bits = 16,
	.volatile_reg = meson_pmic6b_volatile_reg,
	.max_register = PMIC6B_REG_MAX,
	.reg_format_endian = REGMAP_ENDIAN_LITTLE,
	.val_format_endian = REGMAP_ENDIAN_LITTLE,
	.cache_type = REGCACHE_NONE,
	.name = "pmic6b-regmap",
	.use_single_read = true,
	.use_single_write = true,
};

static const struct regmap_irq meson_pmic6b_irqs[] = {
	/* PMIC6B IRQ mask 0 register */
	REGMAP_IRQ_REG(PMIC6B_RTC_IRQ, PMIC6B_IRQ_MASK0_OFFSET, BIT(0)),
	REGMAP_IRQ_REG(PMIC6B_IRQ_CHG_ERROR, PMIC6B_IRQ_MASK0_OFFSET, BIT(1)),
	REGMAP_IRQ_REG(PMIC6B_IRQ_CHG_BAT_OCOV,
		PMIC6B_IRQ_MASK0_OFFSET, BIT(2)),
	REGMAP_IRQ_REG(PMIC6B_IRQ_CHG_EOC, PMIC6B_IRQ_MASK0_OFFSET, BIT(3)),
	REGMAP_IRQ_REG(PMIC6B_IRQ_CHG_NOBAT, PMIC6B_IRQ_MASK0_OFFSET, BIT(4)),
	REGMAP_IRQ_REG(PMIC6B_IRQ_CHG_CHGING, PMIC6B_IRQ_MASK0_OFFSET, BIT(5)),
	REGMAP_IRQ_REG(PMIC6B_IRQ_CHG_BAT_TEMP,
		PMIC6B_IRQ_MASK0_OFFSET, BIT(6)),
	REGMAP_IRQ_REG(PMIC6B_IRQ_CHG_TIMEOUT, PMIC6B_IRQ_MASK0_OFFSET, BIT(7)),
	REGMAP_IRQ_REG(PMIC6B_IRQ_CHG_STATE_DIFF,
		PMIC6B_IRQ_MASK0_OFFSET, BIT(8)),
	REGMAP_IRQ_REG(PMIC6B_IRQ_CHG_PLUGOUT, PMIC6B_IRQ_MASK0_OFFSET, BIT(9)),
	REGMAP_IRQ_REG(PMIC6B_IRQ_CHG_PLUGIN, PMIC6B_IRQ_MASK0_OFFSET, BIT(10)),
	REGMAP_IRQ_REG(PMIC6B_DC1_OC_SP,
		PMIC6B_IRQ_MASK0_OFFSET, BIT(11)),
	REGMAP_IRQ_REG(PMIC6B_DC2_OC_SP,
		PMIC6B_IRQ_MASK0_OFFSET, BIT(12)),
	REGMAP_IRQ_REG(PMIC6B_DC3_OC_SP, PMIC6B_IRQ_MASK0_OFFSET, BIT(13)),
	REGMAP_IRQ_REG(PMIC6B_LDO1_OV_SP, PMIC6B_IRQ_MASK0_OFFSET, BIT(14)),
	REGMAP_IRQ_REG(PMIC6B_LDO1_UV_SP, PMIC6B_IRQ_MASK0_OFFSET, BIT(15)),

	/* PMIC6B IRQ mask 1 register */
	REGMAP_IRQ_REG(PMIC6B_LDO1_OC_SP, PMIC6B_IRQ_MASK1_OFFSET, BIT(0)),
	REGMAP_IRQ_REG(PMIC6B_LDO2_UV_SP, PMIC6B_IRQ_MASK1_OFFSET, BIT(1)),
	REGMAP_IRQ_REG(PMIC6B_LDO2_OC_SP, PMIC6B_IRQ_MASK1_OFFSET, BIT(2)),
	REGMAP_IRQ_REG(PMIC6B_LDO3_OV_SP, PMIC6B_IRQ_MASK1_OFFSET, BIT(3)),
	REGMAP_IRQ_REG(PMIC6B_LDO3_UV_SP, PMIC6B_IRQ_MASK1_OFFSET, BIT(4)),
	REGMAP_IRQ_REG(PMIC6B_LDO3_OC_SP, PMIC6B_IRQ_MASK1_OFFSET, BIT(5)),
	REGMAP_IRQ_REG(PMIC6B_DCIN_OVP_DB, PMIC6B_IRQ_MASK1_OFFSET, BIT(6)),
	REGMAP_IRQ_REG(PMIC6B_DCIN_UVLO_DB, PMIC6B_IRQ_MASK1_OFFSET, BIT(7)),
	REGMAP_IRQ_REG(PMIC6B_SYS_UVLO_DB, PMIC6B_IRQ_MASK1_OFFSET, BIT(8)),
	REGMAP_IRQ_REG(PMIC6B_SYS_OVP_DB, PMIC6B_IRQ_MASK1_OFFSET, BIT(9)),
	REGMAP_IRQ_REG(PMIC6B_OTP_SHUT, PMIC6B_IRQ_MASK1_OFFSET, BIT(10)),
	REGMAP_IRQ_REG(PMIC6B_RTC_TRING_DEEP_SLEEP,
		PMIC6B_IRQ_MASK1_OFFSET, BIT(11)),
	REGMAP_IRQ_REG(PMIC6B_RTC_TRING_SLEEP,
		PMIC6B_IRQ_MASK1_OFFSET, BIT(12)),
	REGMAP_IRQ_REG(PMIC6B_RTC_WAKE_UP, PMIC6B_IRQ_MASK1_OFFSET, BIT(13)),
	REGMAP_IRQ_REG(PMIC6B_PWR_KEY_PRESS, PMIC6B_IRQ_MASK1_OFFSET, BIT(14)),
	REGMAP_IRQ_REG(PMIC6B_SAR_IRQ, PMIC6B_IRQ_MASK1_OFFSET, BIT(15)),
};

static const struct regmap_irq_chip meson_pmic6b_irq_chip = {
	.name = "meson_pmic6b-irq",
	.irqs = meson_pmic6b_irqs,
	.num_irqs = ARRAY_SIZE(meson_pmic6b_irqs),
	.num_regs = 2,
	.status_base = PMIC6B_IRQ_STATUS_CLR0,
	.mask_base = PMIC6B_IRQ_MASK0,
	.ack_base = PMIC6B_IRQ_STATUS_CLR0,
	.init_ack_masked = true,
	.mask_invert = true,
	.ack_invert = true,
};

static struct resource meson_pmic6b_regulators_resources[] = {
	{
		.name	= "LDO1_OV",
		.start	= PMIC6B_LDO1_OV_SP,
		.end	= PMIC6B_LDO1_OV_SP,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.name	= "LDO1_UV",
		.start	= PMIC6B_LDO1_UV_SP,
		.end	= PMIC6B_LDO1_UV_SP,
		.flags	= IORESOURCE_IRQ,
	}
};

static struct resource meson_pmic6b_onkey_resources[] = {
	{
		.name	= "PWRKEY",
		.start	= PMIC6B_PWR_KEY_PRESS,
		.end	= PMIC6B_PWR_KEY_PRESS,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct resource meson_pmic6b_rtc_resources[] = {
	{
		.name	= "RTC",
		.start	= PMIC6B_RTC_WAKE_UP,
		.end	= PMIC6B_RTC_WAKE_UP,
		.flags	= IORESOURCE_IRQ,
	},
};

static const struct mfd_cell meson_pmic6b_common_devs[] = {
	{
		.name		= PMIC6B_DRVNAME_REGULATORS,
		.num_resources	= ARRAY_SIZE(meson_pmic6b_regulators_resources),
		.resources	= meson_pmic6b_regulators_resources,
	},

	{
		.name		= PMIC6B_DRVNAME_GPIO,
		.of_compatible	= "amlogic,pmic6b-gpio",
	},

	{
		.name		= PMIC6B_DRVNAME_ONKEY,
		.num_resources	= ARRAY_SIZE(meson_pmic6b_onkey_resources),
		.resources	= meson_pmic6b_onkey_resources,
		.of_compatible = "amlogic,pmic6b-pwrkey",
	},
	{
		.name		= PMIC6B_DRVNAME_BATTERY,
		.of_compatible	= "amlogic,pmic6b-battery",
	},
	{
		.name		= PMIC6B_DRVNAME_RTC,
		.of_compatible	= "amlogic,pmic6b-rtc",
		.num_resources	= ARRAY_SIZE(meson_pmic6b_rtc_resources),
		.resources	= meson_pmic6b_rtc_resources,
	},
};

static ssize_t read_reg_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct i2c_client *client = container_of(dev, struct i2c_client,
			dev);
	int ret;
	int reg = 0;
	u32 val;
	struct meson_pmic6b *pmic = i2c_get_clientdata(client);

	ret = kstrtoint(buf, 16, &reg);
	if (ret) {
		pr_err("%s: %d\n", __func__, __LINE__);
		return ret;
	}

	ret = regmap_read(pmic->regmap, reg, &val);
	if (ret)
		pr_err("%s: %d\n", __func__, __LINE__);

	pr_info("[0x%x] = 0x%x\n", reg, val);

	return strlen(buf);
}

static ssize_t write_reg_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t count)
{
	struct i2c_client *client = container_of(dev, struct i2c_client,
			dev);
	int ret, i = 0, j = 0;
	struct meson_pmic6b *pmic = i2c_get_clientdata(client);
	int reg = 0, val = 0;
	char tone[20];

	while (buf[i] != '\0') {
		if (buf[i] == 's') {
			tone[j] = '\0';
			ret = kstrtoint(tone, 16, &reg);
			if (ret) {
				pr_err("%s: %d\n", __func__, __LINE__);
				return ret;
			}
		break;
		}
		tone[j] = buf[i];
		i++;
		j++;
	}
	ret = kstrtoint(&buf[i + 1], 16, &val);
	if (ret) {
		pr_err("%s: %d\n", __func__, __LINE__);
		return ret;
	}

	ret = regmap_write(pmic->regmap, reg, val);
	if (ret)
		pr_err("%s: %d\n", __func__, __LINE__);

	pr_info("[0x%x] = 0x%x\n", reg, val);

	return strlen(buf);
}

static DEVICE_ATTR_WO(read_reg);
static DEVICE_ATTR_WO(write_reg);

static struct attribute *pmic6b_attrs[] = {
		&dev_attr_read_reg.attr,
		&dev_attr_write_reg.attr,
		NULL,
};

static struct attribute_group pmic6b_attr_group = {
		.attrs = pmic6b_attrs,
};

int meson_pmic6b_irq_init(struct meson_pmic6b *meson_pmic)
{
	const struct regmap_irq_chip *irq_chip;
	int ret;

	if (!meson_pmic->chip_irq) {
		dev_err(meson_pmic->dev, "No IRQ configured\n");
		return -EINVAL;
	}

	irq_chip = &meson_pmic6b_irq_chip;

	ret = devm_regmap_add_irq_chip(meson_pmic->dev, meson_pmic->regmap,
				       meson_pmic->chip_irq, IRQF_TRIGGER_HIGH |
				       IRQF_ONESHOT | IRQF_SHARED,
				       meson_pmic->irq_base, irq_chip,
				       &meson_pmic->regmap_irq);
	if (ret) {
		dev_err(meson_pmic->dev, "Failed to reguest IRQ %d: %d\n",
			meson_pmic->chip_irq, ret);
		return ret;
	}

	return 0;
}

int meson_pmic6b_device_init(struct meson_pmic6b *meson_pmic, unsigned int irq)
{
	struct pmic6b_pdata *pdata = meson_pmic->dev->platform_data;
	int ret;

	if (pdata) {
		meson_pmic->flags = pdata->flags;
		meson_pmic->irq_base = pdata->irq_base;
	} else {
		meson_pmic->flags = 0;
		meson_pmic->irq_base = -1;
	}
	meson_pmic->chip_irq = irq;

	if (pdata && pdata->init) {
		ret = pdata->init(meson_pmic);
		if (ret != 0) {
			dev_err(meson_pmic->dev,
				"Platform initialization failed.\n");
			return ret;
		}
	}

	ret = meson_pmic6b_irq_init(meson_pmic);

	if (ret) {
		dev_err(meson_pmic->dev, "Cannot initialize interrupts.\n");
		return ret;
	}

	ret = devm_mfd_add_devices(meson_pmic->dev, PLATFORM_DEVID_NONE,
				   meson_pmic6b_common_devs,
				   ARRAY_SIZE(meson_pmic6b_common_devs),
				   NULL, 0, regmap_irq_get_domain(meson_pmic->regmap_irq));

	if (ret) {
		dev_err(meson_pmic->dev, "Failed to add child devices\n");
		return ret;
	}

	return ret;
}

static int meson_pmic6b_probe(struct i2c_client *i2c,
			    const struct i2c_device_id *id)
{
	struct meson_pmic6b *meson_pmic;
	struct gpio_desc *pmic_irq;
	int ret;

	meson_pmic = devm_kzalloc(&i2c->dev,
				  sizeof(struct meson_pmic6b),
				  GFP_KERNEL);
	if (!meson_pmic)
		return -ENOMEM;

	i2c_set_clientdata(i2c, meson_pmic);
	meson_pmic->dev = &i2c->dev;

	pmic_irq = gpiod_get(&i2c->dev, "pmic_irq", GPIOD_IN);
	if (IS_ERR(pmic_irq)) {
		dev_err(meson_pmic->dev, "Failed to claim gpio\n");
		return PTR_ERR(pmic_irq);
	}
	meson_pmic->chip_irq = gpiod_to_irq(pmic_irq);

	meson_pmic6b_regmap_config.rd_table = &meson_pmic6b_readable_table;
	meson_pmic6b_regmap_config.wr_table = &meson_pmic6b_writeable_table;
	meson_pmic6b_regmap_config.volatile_table =
		&meson_pmic6b_volatile_table;

	meson_pmic->regmap = devm_regmap_init_i2c(i2c,
						  &meson_pmic6b_regmap_config);
	if (IS_ERR(meson_pmic->regmap)) {
		ret = PTR_ERR(meson_pmic->regmap);
		dev_err(meson_pmic->dev, "Failed to allocate register map: %d\n",
			ret);
		return ret;
	}

	ret = sysfs_create_group(&meson_pmic->dev->kobj, &pmic6b_attr_group);
	if (ret) {
		dev_err(meson_pmic->dev,
			"pmic sysfs group creation failed: %d\n", ret);
	}

	return meson_pmic6b_device_init(meson_pmic, meson_pmic->chip_irq);
}

static const struct i2c_device_id meson_pmic_i2c_id[] = {
	{ "meson_pmic6b", },
	{},
};

static const struct of_device_id meson_pmic_dt_ids[] = {
	{ .compatible = "amlogic,pmic6b", },
	{ }
};
MODULE_DEVICE_TABLE(of, meson_pmic_dt_ids);

static struct i2c_driver meson_pmic6b_driver = {
	.driver = {
		.name = "meson_pmic6b",
		.of_match_table = of_match_ptr(meson_pmic_dt_ids),
	},
	.probe    = meson_pmic6b_probe,
	.id_table = meson_pmic_i2c_id,
};

int __init meson_pmic6b_i2c_init(void)
{
	return i2c_add_driver(&meson_pmic6b_driver);
}

void __exit meson_pmic6b_i2c_exit(void)
{
	return i2c_del_driver(&meson_pmic6b_driver);
}

MODULE_DESCRIPTION("PMIC6B mfd driver for Amlogic");
MODULE_AUTHOR("Amlogic");
MODULE_LICENSE("GPL v2");
