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

#define IMX335_STANDBY	0x3000
#define IMX335_REGHOLD	0x3001
#define IMX335_GAIN		0x30e8
#define IMX335_EXPOSURE 0x3058
#define IMX335_ID		0x3305
#define IMX335_SLAVE_ID	0x001A

struct imx335_regval {
	u16 reg;
	u8 val;
};

struct imx335_mode {
	u32 width;
	u32 height;
	u32 hmax;
	u32 link_freq_index;

	const struct imx335_regval *data;
	u32 data_size;
};

struct imx335 {
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
	const struct imx335_mode *current_mode;

	struct sensor_gpio *gpio;

	struct v4l2_ctrl_handler ctrls;
	struct v4l2_ctrl *link_freq;
	struct v4l2_ctrl *pixel_rate;
	struct v4l2_ctrl *wdr;
	struct v4l2_ctrl *data_lanes;

	int status;
	struct mutex lock;

	int flag_60hz;
};

struct imx335_pixfmt {
	u32 code;
	u32 min_width;
	u32 max_width;
	u32 min_height;
	u32 max_height;
	u8 bpp;
};

static const struct imx335_pixfmt imx335_formats[] = {
	{MEDIA_BUS_FMT_SRGGB12_1X12, 1280, 2592, 720, 1944, 12},
};

static const struct regmap_config imx335_regmap_config = {
	.reg_bits = 16,
	.val_bits = 8,
	.cache_type = REGCACHE_RBTREE,
};

static const struct imx335_regval imx335_1944p_linear_settings[] = {
	{0x3000, 0x01},
	{0x3002, 0x00},
	{0x300C, 0x5B},
	{0x300D, 0x40},
	{0x3030, 0x94},
	{0x3031, 0x11},
	{0x3034, 0x26},
	{0x3035, 0x02},
	{0x314C, 0xC0},
	{0x315A, 0x06},
	{0x316A, 0x7E},
	{0x319E, 0x02},
	{0x31A1, 0x00},
	{0x3288, 0x21},
	{0x328A, 0x02},
	{0x3414, 0x05},
	{0x3416, 0x18},
	{0x3648, 0x01},
	{0x364A, 0x04},
	{0x364C, 0x04},
	{0x3678, 0x01},
	{0x367C, 0x31},
	{0x367E, 0x31},
	{0x3706, 0x10},
	{0x3708, 0x03},
	{0x3714, 0x02},
	{0x3715, 0x02},
	{0x3716, 0x01},
	{0x3717, 0x03},
	{0x371C, 0x3D},
	{0x371D, 0x3F},
	{0x372C, 0x00},
	{0x372D, 0x00},
	{0x372E, 0x46},
	{0x372F, 0x00},
	{0x3730, 0x89},
	{0x3731, 0x00},
	{0x3732, 0x08},
	{0x3733, 0x01},
	{0x3734, 0xFE},
	{0x3735, 0x05},
	{0x3740, 0x02},
	{0x375D, 0x00},
	{0x375E, 0x00},
	{0x375F, 0x11},
	{0x3760, 0x01},
	{0x3768, 0x1B},
	{0x3769, 0x1B},
	{0x376A, 0x1B},
	{0x376B, 0x1B},
	{0x376C, 0x1A},
	{0x376D, 0x17},
	{0x376E, 0x0F},
	{0x3776, 0x00},
	{0x3777, 0x00},
	{0x3778, 0x46},
	{0x3779, 0x00},
	{0x377A, 0x89},
	{0x377B, 0x00},
	{0x377C, 0x08},
	{0x377D, 0x01},
	{0x377E, 0x23},
	{0x377F, 0x02},
	{0x3780, 0xD9},
	{0x3781, 0x03},
	{0x3782, 0xF5},
	{0x3783, 0x06},
	{0x3784, 0xA5},
	{0x3788, 0x0F},
	{0x378A, 0xD9},
	{0x378B, 0x03},
	{0x378C, 0xEB},
	{0x378D, 0x05},
	{0x378E, 0x87},
	{0x378F, 0x06},
	{0x3790, 0xF5},
	{0x3792, 0x43},
	{0x3794, 0x7A},
	{0x3796, 0xA1},
	{0x3A18, 0x7F},
	{0x3A1A, 0x37},
	{0x3A1C, 0x37},
	{0x3A1E, 0xF7},
	{0x3A1F, 0x00},
	{0x3A20, 0x3F},
	{0x3A22, 0x6F},
	{0x3A24, 0x3F},
	{0x3A26, 0x5F},
	{0x3A28, 0x2F},
	{0x3002, 0x00},
};

extern int imx335_init(struct i2c_client *client, void *sdrv);
extern int imx335_deinit(struct i2c_client *client);
extern int imx335_sensor_id(struct i2c_client *client);
extern int imx335_power_on(struct device *dev, struct sensor_gpio *gpio);
extern int imx335_power_off(struct device *dev, struct sensor_gpio *gpio);
extern int imx335_power_suspend(struct device *dev);
extern int imx335_power_resume(struct device *dev);
