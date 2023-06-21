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


#define lt6911c_ID          0x0500
#define LT6911C_SLAVE_ID    0x001A

struct lt6911c_regval {
	u16 reg;
	u8 val;
};
struct lt6911c_mode {
	u32 width;
	u32 height;
	u32 hmax;
	u32 link_freq_index;

	const struct lt6911c_regval *data;
	u32 data_size;
};

struct lt6911c {
	int index;
	struct device *dev;
	struct clk *xclk;
	struct regmap *regmap;
	u8 nlanes;
	u8 bpp;
	u32 enWDRMode;

	struct i2c_client *client;
	struct v4l2_subdev sd;
	struct v4l2_fwnode_endpoint ep;
	struct media_pad pad;
	struct v4l2_mbus_framefmt current_format;
	const struct lt6911c_mode *current_mode;

	struct sensor_gpio *gpio;

	struct v4l2_ctrl_handler ctrls;
	struct v4l2_ctrl *link_freq;
	struct v4l2_ctrl *pixel_rate;
	struct v4l2_ctrl *wdr;
	struct v4l2_ctrl *fps;
	struct v4l2_ctrl *data_lanes;

	int status;
	struct mutex lock;
};

struct lt6911c_pixfmt {
	u32 code;
	u32 min_width;
	u32 max_width;
	u32 min_height;
	u32 max_height;
	u8 bpp;
};

static const struct lt6911c_pixfmt lt6911c_formats[] = {
	{ MEDIA_BUS_FMT_YVYU8_2X8, 1920, 1920, 1080, 1080, 8 },
};

static const struct regmap_config lt6911c_regmap_config = {
	.reg_bits = 16,
	.val_bits = 8,
	.cache_type = REGCACHE_RBTREE,
};

extern int lt6911c_init(struct i2c_client *client, void *sdrv);
extern int lt6911c_deinit(struct i2c_client *client);
extern int lt6911c_sensor_id(struct i2c_client *client);
extern int lt6911c_power_on(struct device *dev, struct sensor_gpio *gpio);
extern int lt6911c_power_off(struct device *dev, struct sensor_gpio *gpio);
extern int lt6911c_power_suspend(struct device *dev);
extern int lt6911c_power_resume(struct device *dev);
