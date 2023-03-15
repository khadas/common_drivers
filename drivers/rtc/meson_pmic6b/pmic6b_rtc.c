// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * drivers/amlogic/rtc/rtc-pmic6b.c
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

#include <linux/of.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/rtc.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/amlogic/pmic/meson_pmic6b.h>
#include <linux/delay.h>

/* PMIC6B rtc private data */
struct pmic6b_rtc {
	struct rtc_device *rtc;
	struct regmap_irq_chip_data *regmap_irq;
	struct regmap *regmap;
	bool allow_set_time;
	bool alarm_enabled;
	int rtc_alarm_irq;
	struct device *rtc_dev;
};

static const struct of_device_id pmic6b_rtc_match_table[] = {
	{ .compatible = "amlogic,pmic6b-rtc" },
	{ },
};
MODULE_DEVICE_TABLE(of, pmic6b_rtc_match_table);

static int pmic6b_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	int ret = 0;
	struct pmic6b_rtc *rtc_data = dev_get_drvdata(dev);

	u16 reg6_val = 0;
	u16 reg7_val = 0;
	u16 reg8_val = 0;
	u16 reg9_val = 0;

	reg6_val = (tm->tm_sec | (tm->tm_min << CALENDAR_MIN_MASK_POS)) & 0xffff;
	ret = regmap_write(rtc_data->regmap, PMIC6B_RG_RTC_REG6, reg6_val);
	if (ret < 0) {
		dev_err(dev, "RTC write PMIC6B_RG_RTC_REG6 register failed\n");
		return ret;
	}

	reg7_val = (tm->tm_hour | (tm->tm_mday << CALENDAR_DAY_MASK_POS)) & 0xffff;
	ret = regmap_write(rtc_data->regmap, PMIC6B_RG_RTC_REG7, reg7_val);
	if (ret < 0) {
		dev_err(dev, "RTC write PMIC6B_RG_RTC_REG7 register failed\n");
		return ret;
	}

	/* tm_mon starts at zero */
	reg8_val = tm->tm_wday | ((tm->tm_mon + 1) << CALENDAR_MON_MASK_POS);
	reg8_val &= 0xffff;
	ret = regmap_write(rtc_data->regmap, PMIC6B_RG_RTC_REG8, reg8_val);
	if (ret < 0) {
		dev_err(dev, "RTC write PMIC6B_RG_RTC_REG8 register failed\n");
		return ret;
	}

	reg9_val = (tm->tm_year & 0xfff) + 1900;
	ret = regmap_write(rtc_data->regmap, PMIC6B_RG_RTC_REG9, reg9_val);
	if (ret < 0) {
		dev_err(dev, "RTC write PMIC6B_RG_RTC_REG9 register failed\n");
		return ret;
	}

	/* Set RTC Calendar time */
	ret = regmap_update_bits(rtc_data->regmap, PMIC6B_RG_RTC_REG10,
				 0x1 << CALENDAR_TIME_SET_POS,
				 0x1 << CALENDAR_TIME_SET_POS);
	if (ret < 0) {
		dev_err(dev, "RTC write PMIC6B_RG_RTC_REG10 register failed\n");
		return ret;
	}

	dev_dbg(dev, "%s: %d-%d-%d(%d) %d:%d:%d\n", __func__, tm->tm_year,
		tm->tm_mon, tm->tm_mday, tm->tm_wday, tm->tm_hour,
		tm->tm_min, tm->tm_sec);

	return ret;
}

static int pmic6b_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	int ret;
	int i;
	u32 reg15_val = 0;
	u32 reg16_val = 0;
	u32 reg17_val = 0;
	u32 reg18_val = 0;
	struct pmic6b_rtc *rtc_data = dev_get_drvdata(dev);

	/* Read year */
	for (i = 0; i < 4; i++)
		ret = regmap_read(rtc_data->regmap, PMIC6B_RG_RTC_REG18, &reg18_val);
	if (ret < 0) {
		dev_err(dev, "RTC read PMIC6B_RG_RTC_REG18 register failed\n");
		return ret;
	}

	/* Read week & month */
	for (i = 0; i < 4; i++)
		ret = regmap_read(rtc_data->regmap, PMIC6B_RG_RTC_REG17, &reg17_val);
	if (ret < 0) {
		dev_err(dev, "RTC read PMIC6B_RG_RTC_REG17 register failed\n");
		return ret;
	}

	/* Read hour & day */
	for (i = 0; i < 4; i++)
		ret = regmap_read(rtc_data->regmap, PMIC6B_RG_RTC_REG16, &reg16_val);
	if (ret < 0) {
		dev_err(dev, "RTC read PMIC6B_RG_RTC_REG16 register failed\n");
		return ret;
	}

	/* Read second & minute */
	for (i = 0; i < 4; i++)
		ret = regmap_read(rtc_data->regmap, PMIC6B_RG_RTC_REG15, &reg15_val);
	if (ret < 0) {
		dev_err(dev, "RTC read PMIC6B_RG_RTC_REG15 register failed\n");
		return ret;
	}

	tm->tm_year = (reg18_val & CALENDAR_YEAR_MASK_BITS) - 1900;
	/* tm_mon start at zero */
	tm->tm_mon = ((reg17_val >> 0x8) & 0xf) - 1;
	tm->tm_wday = (reg17_val >> CALENDAR_WEEK_MASK_POS) & CALENDAR_WEEK_MASK_BITS;
	tm->tm_mday = (reg16_val >> CALENDAR_DAY_MASK_POS) & CALENDAR_DAY_MASK_BITS;
	tm->tm_hour = (reg16_val >> CALENDAR_HOUR_MASK_POS) & CALENDAR_HOUR_MASK_BITS;
	tm->tm_min = (reg15_val >> CALENDAR_MIN_MASK_POS) & CALENDAR_MIN_MASK_BITS;
	tm->tm_sec = (reg15_val >> CALENDAR_SEC_MASK_POS) & CALENDAR_SEC_MASK_BITS;

	dev_dbg(dev, "%s: %d-%d-%d(%d) %d:%d:%d\n", __func__, tm->tm_year,
		tm->tm_mon, tm->tm_mday, tm->tm_wday, tm->tm_hour,
		tm->tm_min, tm->tm_sec);

	return 0;
}

static int pmic6b_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *alarm)
{
	int ret = 0;
	struct pmic6b_rtc *rtc_data = dev_get_drvdata(dev);
	struct rtc_time *tm = &alarm->time;

	u16 reg11_val = 0;
	u16 reg12_val = 0;
	u16 reg13_val = 0;
	u16 reg14_val = 0;

	reg11_val = (tm->tm_sec | (tm->tm_min << CALENDAR_MIN_MASK_POS)) & 0xffff;
	ret = regmap_write(rtc_data->regmap, PMIC6B_RG_RTC_REG11, reg11_val);
	if (ret < 0) {
		dev_err(dev, "RTC write PMIC6B_RG_RTC_REG11 register failed\n");
		return ret;
	}

	reg12_val = (tm->tm_hour | (tm->tm_mday << CALENDAR_DAY_MASK_POS)) & 0xffff;
	ret = regmap_write(rtc_data->regmap, PMIC6B_RG_RTC_REG12, reg12_val);
	if (ret < 0) {
		dev_err(dev, "RTC write PMIC6B_RG_RTC_REG12 register failed\n");
		return ret;
	}

	reg13_val = tm->tm_wday | ((tm->tm_mon + 1) << CALENDAR_MON_MASK_POS);
	reg13_val &= 0xffff;
	ret = regmap_write(rtc_data->regmap, PMIC6B_RG_RTC_REG13, reg13_val);
	if (ret < 0) {
		dev_err(dev, "RTC write PMIC6B_RG_RTC_REG13 register failed\n");
		return ret;
	}

	reg14_val = (tm->tm_year & 0xfff) + 1900;
	ret = regmap_write(rtc_data->regmap, PMIC6B_RG_RTC_REG14, reg14_val);
	if (ret < 0) {
		dev_err(dev, "RTC write PMIC6B_RG_RTC_REG14 register failed\n");
		return ret;
	}

	/* Unmask RTC wake up IRQ */
	regmap_update_bits(rtc_data->regmap,
			PMIC6B_IRQ_MASK1, 0x1 << IRQ_MASK_BITS_POS_29,
			0x1 << IRQ_MASK_BITS_POS_29);

	/* Set RTC Calendar alarm */
	ret = regmap_update_bits(rtc_data->regmap, PMIC6B_RG_RTC_REG10,
				 0x1 << CALENDAR_ALARM_SET_POS,
				 0x1 << CALENDAR_ALARM_SET_POS);
	if (ret < 0) {
		dev_err(dev, "RTC write PMIC6B_RG_RTC_REG10 register failed\n");
		return ret;
	}

	rtc_data->alarm_enabled = alarm->enabled;
	dev_dbg(dev, "%s: alarm_enabled = %d\n", __func__,
		rtc_data->alarm_enabled);

	dev_dbg(dev, "%s: %d-%d-%d(%d) %d:%d:%d\n", __func__, tm->tm_year,
		tm->tm_mon, tm->tm_mday, tm->tm_wday, tm->tm_hour,
		tm->tm_min, tm->tm_sec);

	return ret;
}

static int pmic6b_rtc_read_alarm(struct device *dev, struct rtc_wkalrm *alarm)
{
	int ret = 0;
	struct pmic6b_rtc *rtc_data = dev_get_drvdata(dev);
	struct rtc_time *tm = &alarm->time;

	u32 reg11_val = 0;
	u32 reg12_val = 0;
	u32 reg13_val = 0;
	u32 reg14_val = 0;

	/* Read second & minute */
	ret = regmap_read(rtc_data->regmap, PMIC6B_RG_RTC_REG11, &reg11_val);
	if (ret < 0) {
		dev_err(dev, "RTC read PMIC6B_RG_RTC_REG11 register failed\n");
		return ret;
	}

	/* Read hour & day */
	ret = regmap_read(rtc_data->regmap, PMIC6B_RG_RTC_REG12, &reg12_val);
	if (ret < 0) {
		dev_err(dev, "RTC read PMIC6B_RG_RTC_REG12 register failed\n");
		return ret;
	}

	/* Read week & month */
	ret = regmap_read(rtc_data->regmap, PMIC6B_RG_RTC_REG13, &reg13_val);
	if (ret < 0) {
		dev_err(dev, "RTC read PMIC6B_RG_RTC_REG13 register failed\n");
		return ret;
	}

	/* Read year */
	ret = regmap_read(rtc_data->regmap, PMIC6B_RG_RTC_REG14, &reg14_val);
	if (ret < 0) {
		dev_err(dev, "RTC read PMIC6B_RG_RTC_REG14 register failed\n");
		return ret;
	}

	tm->tm_sec = (reg11_val >> CALENDAR_SEC_MASK_POS) & CALENDAR_SEC_MASK_BITS;
	tm->tm_min = (reg11_val >> CALENDAR_MIN_MASK_POS) & CALENDAR_MIN_MASK_BITS;
	tm->tm_hour = (reg12_val >> CALENDAR_HOUR_MASK_POS) & CALENDAR_HOUR_MASK_BITS;
	tm->tm_mday = (reg12_val >> CALENDAR_DAY_MASK_POS) & CALENDAR_DAY_MASK_BITS;
	tm->tm_wday = (reg13_val >> CALENDAR_WEEK_MASK_POS) & CALENDAR_WEEK_MASK_BITS;
	tm->tm_mon = (reg13_val >> CALENDAR_MON_MASK_POS) & CALENDAR_MON_MASK_BITS;
	tm->tm_year = (reg14_val >> CALENDAR_YEAR_MASK_POS) & CALENDAR_YEAR_MASK_BITS;

	dev_dbg(dev, "%s: %d-%d-%d(%d) %d:%d:%d\n", __func__, tm->tm_year,
		tm->tm_mon, tm->tm_mday, tm->tm_wday, tm->tm_hour,
		tm->tm_min, tm->tm_sec);

	return ret;
}

static int pmic6b_rtc_alarm_enable(struct device *dev, unsigned int enabled)
{
	struct pmic6b_rtc *rtc_data = dev_get_drvdata(dev);

	dev_dbg(dev, "%s: enabled = %d\n", __func__,
		enabled);
	rtc_data->alarm_enabled = enabled;

	return 0;
}

static const struct rtc_class_ops pmic6b_rtc_ops = {
	.read_time	= pmic6b_rtc_read_time,
	.set_time	= pmic6b_rtc_set_time,
	.read_alarm	= pmic6b_rtc_read_alarm,
	.set_alarm	= pmic6b_rtc_set_alarm,
	.alarm_irq_enable = pmic6b_rtc_alarm_enable,
};

static irqreturn_t pmic6b_rtc_trig_wake_up_handle(int irq, void *dev_id)
{
	struct pmic6b_rtc *rtc_data = (struct pmic6b_rtc *)dev_id;
	u32 reg_val = 0;
	int ret = 0;
	struct device *dev = rtc_data->rtc_dev;

	dev_dbg(dev, "%s: rtc handler invoked\n", __func__);
	ret = regmap_read(rtc_data->regmap, PMIC6B_IRQ_STATUS_CLR1, &reg_val);
	if (ret)
		dev_err(dev, "%s: read PMIC6B_OTP_REG_0x3F failed\n", __func__);
	else
		dev_dbg(dev, "%s: PMIC6B_IRQ_STATUS_CLR1: 0x%x\n", __func__,
			reg_val);

	/* Mask RTC wake up IRQ */
	regmap_update_bits(rtc_data->regmap,
			PMIC6B_IRQ_MASK1, 0x1 << IRQ_MASK_BITS_POS_29, 0);

	/* Clean RTC alarm */
	regmap_update_bits(rtc_data->regmap,
			PMIC6B_RG_RTC_REG10,
			(0x1 << ALARM_CLEAN_MASK_BITS_POS),
			(0x1 << ALARM_CLEAN_MASK_BITS_POS));

	/*Clear rtc_wake_up irq*/
	regmap_update_bits(rtc_data->regmap,
			PMIC6B_IRQ_STATUS_CLR1,
			(0x1 << IRQ_MASK_BITS_POS_29),
			(0x1 << IRQ_MASK_BITS_POS_29));

	rtc_update_irq(rtc_data->rtc, 1, RTC_AF | RTC_IRQF);

	return IRQ_HANDLED;
}

static int pmic6b_rtc_init_wakeup(struct device *dev)
{
	struct pmic6b_rtc *rtc_data = dev_get_drvdata(dev);

	/* Select 32K OSC as the RTC clock */
	regmap_update_bits(rtc_data->regmap, PMIC6B_RG_RTC_REG3,
			   1 << 11, 1 << 11);
	/* Disable RTC calendar */
	regmap_update_bits(rtc_data->regmap, PMIC6B_OTP_REG_0x3F,
			   1 << CALENDAR_EN_MASK_BITS_POS,
			   0 << CALENDAR_EN_MASK_BITS_POS);
	/* Delay 1 millisecond */
	mdelay(1);
	/* Enable RTC calendar */
	regmap_update_bits(rtc_data->regmap, PMIC6B_OTP_REG_0x3F,
			   1 << CALENDAR_EN_MASK_BITS_POS,
			   1 << CALENDAR_EN_MASK_BITS_POS);

	/* Set RTC calendar to 2022-01-01 00:00:00 by default */
	regmap_write(rtc_data->regmap, PMIC6B_RG_RTC_REG6, 0x0);
	regmap_write(rtc_data->regmap, PMIC6B_RG_RTC_REG7, 0x100);
	regmap_write(rtc_data->regmap, PMIC6B_RG_RTC_REG8, 0x106);
	regmap_write(rtc_data->regmap, PMIC6B_RG_RTC_REG9, 0x7e6);
	regmap_update_bits(rtc_data->regmap, PMIC6B_RG_RTC_REG10,
			   0x1 << CALENDAR_TIME_SET_POS,
			    0x1 << CALENDAR_TIME_SET_POS);

	/* Enable RTC wake up top dig into active state */
	regmap_update_bits(rtc_data->regmap,
			   PMIC6B_OTP_REG_0x3F, BIT(10), BIT(10));

	/* Unmask RTC wake up IRQ */
	regmap_update_bits(rtc_data->regmap,
			PMIC6B_IRQ_MASK1, 0x1 << IRQ_MASK_BITS_POS_29,
			0x1 << IRQ_MASK_BITS_POS_29);

	return 0;
}

static int pmic6b_rtc_init(struct device *dev)
{
	int ret = 0;

	ret = pmic6b_rtc_init_wakeup(dev);
	return ret;
}

static int pmic6b_rtc_suspend(struct device *dev)
{
	struct pmic6b_rtc *rtc_data = dev_get_drvdata(dev);

	dev_dbg(dev, "%s\n", __func__);
	if (device_may_wakeup(dev)) {
		enable_irq_wake(rtc_data->rtc_alarm_irq);
		/* Unmask wake up alarm irq */
		regmap_update_bits(rtc_data->regmap,
				PMIC6B_IRQ_MASK1,
				(0x1 << IRQ_MASK_BITS_POS_29),
				(0x1 << IRQ_MASK_BITS_POS_29));
	}

	return 0;
}

static int pmic6b_rtc_resume(struct device *dev)
{
	struct pmic6b_rtc *rtc_data = dev_get_drvdata(dev);

	dev_dbg(dev, "%s\n", __func__);
	if (device_may_wakeup(dev))
		disable_irq_wake(rtc_data->rtc_alarm_irq);

	return 0;
}

const struct dev_pm_ops pmic6b_rtc_pm_ops = {
	.suspend = pmic6b_rtc_suspend,
	.resume = pmic6b_rtc_resume,
	.freeze = pmic6b_rtc_suspend,
	.thaw = pmic6b_rtc_resume,
	.poweroff = pmic6b_rtc_suspend,
	.restore = pmic6b_rtc_resume,
};

static int pmic6b_rtc_probe(struct platform_device *pdev)
{
	struct pmic6b_rtc *rtc_dd;
	struct meson_pmic6b *pmic6b = dev_get_drvdata(pdev->dev.parent);
	int ret = 0;

	if (!pmic6b) {
		dev_err(&pdev->dev, "Failed to get drvdata.\n");
		return -ENXIO;
	}

	rtc_dd = devm_kzalloc(&pdev->dev, sizeof(*rtc_dd), GFP_KERNEL);
	if (!rtc_dd)
		return -ENOMEM;

	rtc_dd->regmap = pmic6b->regmap;
	if (!rtc_dd->regmap) {
		dev_err(&pdev->dev, "Parent regmap unavailable.\n");
		return -ENXIO;
	}

	rtc_dd->regmap_irq = pmic6b->regmap_irq;
	if (!rtc_dd->regmap_irq) {
		dev_err(&pdev->dev, "Parent regmap_irq unavailable.\n");
		return -ENXIO;
	}

//	rtc_dd->rtc_alarm_irq = regmap_irq_get_virq(rtc_dd->regmap_irq,
//		PMIC6B_RTC_WAKE_UP);
	rtc_dd->rtc_alarm_irq = platform_get_irq_byname(pdev, "RTC");
	if (rtc_dd->rtc_alarm_irq < 0) {
		ret = rtc_dd->rtc_alarm_irq;
		dev_err(&pdev->dev, "%s: Failed to get platform IRQ: %d\n",
			__func__, ret);
		return ret;
	}
	dev_dbg(&pdev->dev, "%s: rtc_alarm_irq = %d\n", __func__,
			rtc_dd->rtc_alarm_irq);

	rtc_dd->rtc_dev = &pdev->dev;
	device_init_wakeup(&pdev->dev, 1);
	platform_set_drvdata(pdev, rtc_dd);

	/* Init RTC */
	pmic6b_rtc_init(&pdev->dev);

	ret = devm_request_threaded_irq(&pdev->dev, rtc_dd->rtc_alarm_irq, NULL,
		pmic6b_rtc_trig_wake_up_handle,
		IRQF_TRIGGER_HIGH | IRQF_ONESHOT,
		"pmic_rtc_alarm", rtc_dd);

	if (ret) {
		dev_err(&pdev->dev,
			"IRQ %d error %d\n", rtc_dd->rtc_alarm_irq, ret);
		return ret;
	}

	/* Register the RTC device */
	rtc_dd->rtc = devm_rtc_device_register(&pdev->dev,
				"pmic6b_rtc", &pmic6b_rtc_ops, THIS_MODULE);
	if (IS_ERR(rtc_dd->rtc)) {
		dev_err(&pdev->dev, "%s: RTC registration failed (%ld)\n",
				__func__, PTR_ERR(rtc_dd->rtc));
		return PTR_ERR(rtc_dd->rtc);
	}

	dev_dbg(&pdev->dev, "%s: probe done.\n", __func__);

	return 0;
}

static int pmic6b_rtc_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver pmic6b_rtc_driver = {
	.probe = pmic6b_rtc_probe,
	.remove = pmic6b_rtc_remove,
	.driver	= {
		.name		= PMIC6B_DRVNAME_RTC,
		.of_match_table	= pmic6b_rtc_match_table,
		.pm = &pmic6b_rtc_pm_ops,
	},
};

int __init meson_pmic6b_rtc_init(void)
{
	int ret;

	ret = platform_driver_register(&pmic6b_rtc_driver);
	return ret;
}

void __exit meson_pmic6b_rtc_exit(void)
{
	platform_driver_unregister(&pmic6b_rtc_driver);
}

MODULE_AUTHOR("Amlogic");
MODULE_ALIAS("platform:rtc-pmic6b");
MODULE_DESCRIPTION("PMIC6B RTC driver");
MODULE_LICENSE("GPL v2");
