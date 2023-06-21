// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2023 Amlogic, Inc. All rights reserved.
 */
#ifndef __MCLK_API__
#define __MCLK_API__
#include <linux/version.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <media/media-entity.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-subdev.h>
#include <linux/of_gpio.h>

#include "../../amlcam/cam_common/aml_misc.h"
#include "../../amlcam/cam_common/aml_common.h"

static inline int mclk_enable(struct device *dev, uint32_t rate)
{
	struct clk *clk = NULL, *clk_sel = NULL, *clk_pre = NULL, *clk_p = NULL, *clk_x = NULL;
	int clk_val;
	int ret;

	clk = devm_clk_get(dev, "mclk");
	if (IS_ERR_OR_NULL(clk)) {
		pr_err("cannot get %s clk\n", "mclk");
		clk = NULL;
		return -1;
	}

	clk_sel = devm_clk_get(dev, "mclk_sel");
	if (IS_ERR_OR_NULL(clk_sel)) {
		pr_err("cannot get %s clk_sel\n", "clk_sel");
		clk_sel = NULL;
		return -1;
	}

	if (rate == 24000000) {
		clk_x = devm_clk_get(dev, "mclk_x");
		if (!IS_ERR_OR_NULL(clk_x)) {
			ret = clk_set_parent(clk_sel, clk_x);
			if (ret < 0)
				dev_err(dev, "set parent to xtal, ret %d", ret);
		}
		clk_pre = devm_clk_get(dev, "mclk_pre");
	} else {
		clk_p = devm_clk_get(dev, "mclk_p");
		if (!IS_ERR_OR_NULL(clk_p)) {
			ret = clk_set_parent(clk_sel, clk_p);
			if (ret < 0)
				dev_err(dev, "set parent to clk_p, ret %d", ret);

			ret = clk_set_rate(clk_p, rate * 2);
			if (ret < 0)
				dev_err(dev, "set clk_p rate %d, ret %d", rate * 2, ret);
		}
	}

	ret = clk_set_rate(clk, rate);
	if (ret < 0)
		dev_err(dev, "clk_set_rate failed\n");
	udelay(30);

	ret = clk_prepare_enable(clk);
	if (ret < 0)
		dev_err(dev, " clk_prepare_enable failed\n");

	// disable mclk_pre after mclk has been enabled. in this way, we got mclk == xtal == 24M;
	if (!IS_ERR_OR_NULL(clk_pre) && __clk_is_enabled(clk_pre)) {
		clk_disable_unprepare(clk_pre);
	}

	clk_val = clk_get_rate(clk);

	dev_info(dev, "mclk is %d MHZ\n", clk_val/1000000);

	return 0;
}

static inline int mclk_disable(struct device *dev)
{
	struct clk *clk = NULL;
	struct clk *clk_pre = NULL;
	int clk_val;

	clk = devm_clk_get(dev, "mclk");
	if (IS_ERR_OR_NULL(clk)) {
		pr_err("cannot get %s clk\n", "mclk");
		clk = NULL;
		return -1;
	}

	clk_val = clk_get_rate(clk);
	if (__clk_is_enabled(clk)) {
		// ==== begin  avoiding warning callstack  ===================
		// mclk_pre has been disabled
		// mclk_pre has been unprepared
		if ((clk_val == 12000000) || (clk_val == 24000000)) {
			pr_info("enable mclk_pre to avoid callstack\n");
			clk_pre = devm_clk_get(dev, "mclk_pre");
			if ( !IS_ERR_OR_NULL(clk_pre) ) {
				clk_prepare_enable(clk_pre);
			}
		}

		// ========== end  avoiding warning callstack ================
		clk_disable_unprepare(clk);
	}

	devm_clk_put(dev, clk);

	dev_info(dev, "disable mclk: %d\n", clk_val);

	return 0;
}

static inline int reset_am_enable(struct device *dev, const char* propname, int val)
{
	int ret = -1;

	int reset = of_get_named_gpio(dev->of_node, propname, 0);
	ret = reset;

	if (ret >= 0) {
		devm_gpio_request(dev, reset, "RESET");
		if (gpio_is_valid(reset)) {
			gpio_direction_output(reset, val);
			pr_info("reset init\n");
		} else {
			pr_err("reset_enable: gpio %s is not valid\n", propname);
			return -1;
		}
	} else {
		pr_err("reset_enable: get_named_gpio %s fail\n", propname);
	}

	return ret;
}
static inline int pwdn_am_enable(struct device *dev, const char* propname, int val)
{
	int ret = -1;

	int reset = of_get_named_gpio(dev->of_node, propname, 0);
	ret = reset;

	if (ret >= 0) {
		devm_gpio_request(dev, reset, "PWDN");
		if (gpio_is_valid(reset)) {
			gpio_direction_output(reset, val);
			pr_info("reset init\n");
		} else {
			pr_err("reset_enable: gpio %s is not valid\n", propname);
			return -1;
		}
	} else {
		pr_err("reset_enable: get_named_gpio %s fail\n", propname);
	}

	return ret;
}
#endif
