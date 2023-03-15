// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * drivers/amlogic/input/misc/pmic6b_pwrkey.c
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
#include <linux/errno.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/regmap.h>
#include <linux/of.h>
#include <linux/timer.h>
#include <linux/amlogic/pmic/meson_pmic6b.h>

/*short and long  pwrkey time, uint is ms*/
#define SHORT_PRESS_TIME 200
#define LONG_PRESS_TIME 3000
#define STEP_TIME 50
#define SHORT_COUNT (SHORT_PRESS_TIME / STEP_TIME)
#define LONG_COUNT (LONG_PRESS_TIME / STEP_TIME)

struct pmic6b_pwrkey {
	struct delayed_work work;
	struct input_dev *input;
	struct device *dev;
	struct regmap *regmap;
	struct timer_list timer;
	unsigned int count;
};

static const struct of_device_id pmic6b_pwrkey_id_table[] = {
	{ .compatible = "amlogic,pmic6b-pwrkey",},
	{ },
};
MODULE_DEVICE_TABLE(of, pmic6b_pwrkey_id_table);

static void pmic6b_irq_work(struct work_struct *work)
{
	struct pmic6b_pwrkey *pwrkey = container_of(work,
				struct pmic6b_pwrkey, work.work);
	unsigned int val;
	int ret;

	/* read pwrkey press status*/
	ret = regmap_read(pwrkey->regmap, PMIC6B_GEN_STATUS0, &val);
	if (ret)
		dev_err(pwrkey->dev, "Failed to read PWRKEY status: %d\n", ret);

	if ((val & 0x80)) {
		pwrkey->count++;
		if (pwrkey->count <= LONG_COUNT)
			goto poll;

		if (pwrkey->count > LONG_COUNT) {
			pwrkey->count = 0;
			input_report_key(pwrkey->input, KEY_POWER, 0);
			input_sync(pwrkey->input);

			ret = regmap_update_bits(pwrkey->regmap,
					PMIC6B_IRQ_MASK1, BIT(14), BIT(14));
			if (ret)
				dev_err(pwrkey->dev, "%d\n", __LINE__);
			return;
		}

	} else {
		if (pwrkey->count > SHORT_COUNT) {
			pwrkey->count = 0;
			input_report_key(pwrkey->input, KEY_POWER, 1);
			input_sync(pwrkey->input);
		}
		/*enable pwrkey irq*/
		ret = regmap_update_bits(pwrkey->regmap,
					PMIC6B_IRQ_MASK1, BIT(14), BIT(14));

		if (ret)
			dev_err(pwrkey->dev, "%d\n", __LINE__);

		return;
	}

poll:
	schedule_delayed_work(&pwrkey->work, msecs_to_jiffies(STEP_TIME));
}

static int pmic6b_dev_of_pwrkey_init(struct pmic6b_pwrkey *pwrkey)
{
	int ret;
	unsigned int temp;

	/*short press time : 1s*/
	/*long press time : 4s*/

	ret = regmap_read(pwrkey->regmap, PMIC6B_OTP_REG_0x3F, &temp);
	if (ret) {
		dev_err(pwrkey->dev,
			"Failed to read PWRKEY status: %d\n", __LINE__);
		return ret;
	}

	temp &= (~(0xf << 5));
	temp |= (0x1 << 5);
	/*long press time : 4s*/
	ret = regmap_write(pwrkey->regmap, PMIC6B_OTP_REG_0x3F, temp);

	if (ret) {
		dev_err(pwrkey->dev,
			"Failed to read PWRKEY status: %d\n", __LINE__);
		return ret;
	}
	/*enable pwrkey irq*/
	ret = regmap_update_bits(pwrkey->regmap,
		PMIC6B_IRQ_MASK1, BIT(14), BIT(14));
	if (ret) {
		dev_err(pwrkey->dev,
			"Failed to read PWRKEY status: %d\n", __LINE__);
		return ret;
	}

	return ret;
}

static irqreturn_t pmic6b_pwrkey_irq_handler(int irq, void *data)
{
	struct pmic6b_pwrkey *pwrkey = data;
	int ret;

	/*clear pwrkey irq*/
	ret = regmap_update_bits(pwrkey->regmap,
		PMIC6B_IRQ_MASK1, BIT(14), 0);
	if (ret) {
		dev_err(pwrkey->dev,
		"Failed to read PWRKEY status: %d\n", __LINE__);
		return IRQ_HANDLED;
	}

	schedule_delayed_work(&pwrkey->work, 0);

	return IRQ_HANDLED;
}

static int pmic6b_pwrkey_probe(struct platform_device *pdev)
{
	struct meson_pmic6b *pmic6 = dev_get_drvdata(pdev->dev.parent);
	struct pmic6b_pwrkey *pwrkey;
	int irq;
	int ret;

	pwrkey = devm_kzalloc(&pdev->dev, sizeof(struct pmic6b_pwrkey),
			      GFP_KERNEL);
	if (!pwrkey)
		return -ENOMEM;

	pwrkey->dev = &pdev->dev;
	pwrkey->regmap = pmic6->regmap;
	pwrkey->input = devm_input_allocate_device(&pdev->dev);
	if (!pwrkey->input) {
		dev_err(&pdev->dev, "Failed to allocated input device.\n");
		return -ENOMEM;
	}

	pwrkey->input->name = "pmic6b_pwrkey";
	pwrkey->input->phys = "pmic6b_pwrkey/input0";
	pwrkey->input->dev.parent = &pdev->dev;
	pwrkey->input->id.bustype = BUS_HOST;
	input_set_capability(pwrkey->input, EV_KEY, KEY_POWER);
	INIT_DELAYED_WORK(&pwrkey->work, pmic6b_irq_work);

	irq = platform_get_irq_byname(pdev, "PWRKEY");
	if (irq < 0) {
		ret = irq;
		dev_err(&pdev->dev, "Failed to get platform IRQ: %d\n", ret);
		return ret;
	}

	ret = pmic6b_dev_of_pwrkey_init(pwrkey);
	if (ret) {
		dev_err(&pdev->dev, "Failed to config pwrkey %d\n", ret);
		return ret;
	}

	ret = devm_request_threaded_irq(&pdev->dev, irq,
					NULL, pmic6b_pwrkey_irq_handler,
					IRQF_TRIGGER_HIGH | IRQF_ONESHOT,
					"PWRKEY", pwrkey);
	if (ret) {
		dev_err(&pdev->dev, "Failed to request IRQ %d: %d\n", irq, ret);
		return ret;
	}

	ret = input_register_device(pwrkey->input);
	if (ret) {
		dev_err(&pdev->dev,
			"Failed to register input device: %d\n", ret);
		return ret;
	}

	platform_set_drvdata(pdev, pwrkey);
	return 0;
}

static struct platform_driver pmic6b_pwrkey_driver = {
	.probe	= pmic6b_pwrkey_probe,
	.driver	= {
		.name	= PMIC6B_DRVNAME_ONKEY,
		.of_match_table = pmic6b_pwrkey_id_table,
	},
};

int __init meson_pmic6b_pwrkey_init(void)
{
	return platform_driver_register(&pmic6b_pwrkey_driver);
}

void __exit meson_pmic6b_pwrkey_exit(void)
{
	platform_driver_unregister(&pmic6b_pwrkey_driver);
}

MODULE_AUTHOR("Amlogic");
MODULE_DESCRIPTION("PMIC6B input driver");
MODULE_LICENSE("GPL v2");
