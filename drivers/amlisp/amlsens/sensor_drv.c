// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2023 Amlogic, Inc. All rights reserved.
 */
#include <linux/version.h>
#include <linux/clk.h>
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
#include <linux/of_platform.h>
#include <linux/of_graph.h>

#include "sensor_drv.h"

#include "imx290/aml_imx290.h"
#include "imx415/aml_imx415.h"
#include "imx378/aml_imx378.h"
#include "lt6911c/aml_lt6911c.h"
#include "imx335/aml_imx335.h"
#include "ov08a10/aml_ov08a10.h"
#include "ov5640/aml_ov5640.h"
#include "ov13855/aml_ov13855.h"
#include "ov13b10/aml_ov13b10.h"
#include "imx577/aml_imx577.h"
#include "ov16a1q/aml_ov16a1q.h"

struct sensor_subdev sd_imx290 = {
	.sensor_init = imx290_init,
	.sensor_deinit = imx290_deinit,
	.sensor_get_id = imx290_sensor_id,
	.sensor_power_on = imx290_power_on,
	.sensor_power_off = imx290_power_off,
	.sensor_power_suspend = imx290_power_suspend,
	.sensor_power_resume = imx290_power_resume,
};

struct sensor_subdev sd_imx335 = {
	.sensor_init = imx335_init,
	.sensor_deinit = imx335_deinit,
	.sensor_get_id = imx335_sensor_id,
	.sensor_power_on = imx335_power_on,
	.sensor_power_off = imx335_power_off,
	.sensor_power_suspend = imx335_power_suspend,
	.sensor_power_resume = imx335_power_resume,
};

struct sensor_subdev sd_imx378 = {
	.sensor_init = imx378_init,
	.sensor_deinit = imx378_deinit,
	.sensor_get_id = imx378_sensor_id,
	.sensor_power_on = imx378_power_on,
	.sensor_power_off = imx378_power_off,
	.sensor_power_suspend = imx378_power_suspend,
	.sensor_power_resume = imx378_power_resume,
};

struct sensor_subdev sd_imx415 = {
	.sensor_init = imx415_init,
	.sensor_deinit = imx415_deinit,
	.sensor_get_id = imx415_sensor_id,
	.sensor_power_on = imx415_power_on,
	.sensor_power_off = imx415_power_off,
	.sensor_power_suspend = imx415_power_suspend,
	.sensor_power_resume = imx415_power_resume,
};

struct sensor_subdev sd_ov08a10 = {
	.sensor_init = ov08a10_init,
	.sensor_deinit = ov08a10_deinit,
	.sensor_get_id = ov08a10_sensor_id,
	.sensor_power_on = ov08a10_power_on,
	.sensor_power_off = ov08a10_power_off,
	.sensor_power_suspend = ov08a10_power_suspend,
	.sensor_power_resume = ov08a10_power_resume,
};

struct sensor_subdev sd_lt6911c = {
	.sensor_init = lt6911c_init,
	.sensor_deinit = lt6911c_deinit,
	.sensor_get_id = lt6911c_sensor_id,
	.sensor_power_on = lt6911c_power_on,
	.sensor_power_off = lt6911c_power_off,
	.sensor_power_suspend = lt6911c_power_suspend,
	.sensor_power_resume = lt6911c_power_resume,
};

struct sensor_subdev sd_ov5640 = {
	.sensor_init = ov5640_init,
	.sensor_deinit = ov5640_deinit,
	.sensor_get_id = ov5640_sensor_id,
	.sensor_power_on = ov5640_power_on,
	.sensor_power_off = ov5640_power_off,
	.sensor_power_suspend = ov5640_power_suspend,
	.sensor_power_resume = ov5640_power_resume,
};

struct sensor_subdev sd_ov13855 = {
	.sensor_init = ov13855_init,
	.sensor_deinit = ov13855_deinit,
	.sensor_get_id = ov13855_sensor_id,
	.sensor_power_on = ov13855_power_on,
	.sensor_power_off = ov13855_power_off,
	.sensor_power_suspend = ov13855_power_suspend,
	.sensor_power_resume = ov13855_power_resume,
};

struct sensor_subdev sd_ov13b10 = {
	.sensor_init = ov13b10_init,
	.sensor_deinit = ov13b10_deinit,
	.sensor_get_id = ov13b10_sensor_id,
	.sensor_power_on = ov13b10_power_on,
	.sensor_power_off = ov13b10_power_off,
	.sensor_power_suspend = ov13b10_power_suspend,
	.sensor_power_resume = ov13b10_power_resume,
};

struct sensor_subdev sd_imx577 = {
	.sensor_init = imx577_init,
	.sensor_deinit = imx577_deinit,
	.sensor_get_id = imx577_sensor_id,
	.sensor_power_on = imx577_power_on,
	.sensor_power_off = imx577_power_off,
	.sensor_power_suspend = imx577_power_suspend,
	.sensor_power_resume = imx577_power_resume,
};

struct sensor_subdev sd_ov16a1q = {
	.sensor_init = ov16a1q_init,
	.sensor_deinit = ov16a1q_deinit,
	.sensor_get_id = ov16a1q_sensor_id,
	.sensor_power_on = ov16a1q_power_on,
	.sensor_power_off = ov16a1q_power_off,
	.sensor_power_suspend = ov16a1q_power_suspend,
	.sensor_power_resume = ov16a1q_power_resume,
};

struct sensor_subdev *aml_sensors[] = {
	&sd_imx290,
	&sd_imx415,
	&sd_ov13b10,
	&sd_imx577,
	&sd_imx378,
	&sd_ov08a10,
	&sd_ov5640,
	&sd_ov13855,
	&sd_lt6911c,
	&sd_ov16a1q,
	&sd_imx335,
};


