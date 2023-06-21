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

struct sensor_regval {
	u16 reg;
	u8 val;
};

struct sensor_mode {
	u32 width;
	u32 height;
	u32 hmax;
	u32 link_freq_index;

	const struct sensor_regval *data;
	u32 data_size;
};

struct sensor_pixfmt {
	u32 code;
	u32 min_width;
	u32 max_width;
	u32 min_height;
	u32 max_height;
	u8 bpp;
};

struct sensor_gpio {
	struct gpio_desc *rst_gpio;
	struct gpio_desc *pwdn_gpio;
	struct gpio_desc *power_gpio;
};

struct amlsens {
	struct device *dev;
	u8 nlanes;

	struct i2c_client *client;
	struct v4l2_fwnode_endpoint ep;

	struct sensor_gpio gpio;

	struct sensor_subdev *sd_sdrv;
};

struct sensor_subdev {
	int (*sensor_init) (struct i2c_client *client, void *sdrv);
	int (*sensor_deinit) (struct i2c_client *client);
	int (*sensor_get_id) (struct i2c_client *client);
	int (*sensor_power_on) (struct device *dev, struct sensor_gpio *gpio);
	int (*sensor_power_off) (struct device *dev, struct sensor_gpio *gpio);
	int (*sensor_power_suspend) (struct device *dev);
	int (*sensor_power_resume) (struct device *dev);
};

extern struct sensor_subdev *aml_sensors[32];
