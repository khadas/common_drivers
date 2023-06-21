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

#define IMX378_STANDBY	0x0100
#define IMX378_REGHOLD	0x0104
#define IMX378_GAIN		0x0204
#define IMX378_EXPOSURE	0x0202
#define IMX378_ID		0x0378
#define IMX378_SLAVE_ID	0x0010

struct imx378_regval {
	u16 reg;
	u8 val;
};

struct imx378_mode {
	u32 width;
	u32 height;
	u32 hmax;
	u32 link_freq_index;

	const struct imx378_regval *data;
	u32 data_size;
};

struct imx378 {
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
	const struct imx378_mode *current_mode;

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

struct imx378_pixfmt {
	u32 code;
	u32 min_width;
	u32 max_width;
	u32 min_height;
	u32 max_height;
	u8 bpp;
};

static const struct imx378_pixfmt imx378_formats[] = {
	{MEDIA_BUS_FMT_SRGGB10_1X10, 3840, 3840, 2160, 2160, 10},
};

static const struct regmap_config imx378_regmap_config = {
	.reg_bits = 16,
	.val_bits = 8,
	.cache_type = REGCACHE_RBTREE,
};

static const struct imx378_regval imx378_2160p_linear_settings[] = {
	{0x0136, 0x18},
	{0x0137, 0x00},

	{0x0112, 0x0A},    //MIPI setting
	{0x0113, 0x0A},
	{0x0114, 0x03},
	{0x0342, 0x11},    //Frame Horizontal Clock count
	{0x0343, 0xA0},
	{0x0340, 0x0D},    //Frame Vertical Clock count
	{0x0341, 0xA0},
	{0x0344, 0x00},    //Visible Size
	{0x0345, 0x00},
	{0x0346, 0x00},
	{0x0347, 0x00},
	{0x0348, 0x0F},
	{0x0349, 0xD7},
	{0x034A, 0x0B},
	{0x034B, 0xDF},
	{0x0220, 0x00},    //Mode setting
	{0x0221, 0x11},
	{0x3140, 0x00},
	{0x0381, 0x01},
	{0x0383, 0x01},
	{0x0385, 0x01},
	{0x0387, 0x01},
	{0x0900, 0x00},    //binning disable
	{0x0901, 0x11},    //no binning
	{0x0902, 0x02},
	{0x3C01, 0x03},
	{0x3C02, 0xA2},
	{0x3F0D, 0x00},
	{0x5748, 0x07},
	{0x5749, 0xFF},
	{0x574A, 0x00},
	{0x574B, 0x00},
	{0x7B53, 0x01},
	{0x9369, 0x5A},
	{0x936B, 0x55},
	{0x936D, 0x28},
	{0x9304, 0x03},
	{0x9305, 0x00},
	{0x9E9A, 0x2F},
	{0x9E9B, 0x2F},
	{0x9E9C, 0x2F},
	{0x9E9D, 0x00},
	{0x9E9E, 0x00},
	{0x9E9F, 0x00},
	{0xA2A9, 0x60},
	{0xA2B7, 0x00},

	{0x0401, 0x00},    //Digital Crop & Scaling
	{0x0404, 0x00},
	{0x0405, 0x10},
	{0x0408, 0x00},
	{0x0409, 0x68},
	{0x040A, 0x01},
	{0x040B, 0xB4},
	{0x040C, 0x0F},
	{0x040D, 0x08},
	{0x040E, 0x08},
	{0x040F, 0x78},

	{0x034C, 0x0F},    //Output Crop
	{0x034D, 0x08},
	{0x034E, 0x08},
	{0x034F, 0x78},

	{0x0301, 0x05},    //Clock setting
	{0x0303, 0x02},
	{0x0305, 0x03},
	{0x0306, 0x00},
	{0x0307, 0x96},
	{0x0309, 0x0A},
	{0x030B, 0x01},
	{0x030D, 0x02},
	{0x030E, 0x01},
	{0x030F, 0x5E},
	{0x0310, 0x00},
	{0x0820, 0x12},
	{0x0821, 0xC0},
	{0x0822, 0x00},
	{0x0823, 0x00},

	{0x3E20, 0x01},    //Output Data Select Setting
	{0x3E37, 0x01},
	{0x3F50, 0x00},    //PowerSave Setting
	{0x3F56, 0x01},
	{0x3F57, 0xE2},
	{0x3031, 0x01},    //Streaming setting
	{0x3033, 0x01},
};

extern int imx378_init(struct i2c_client *client, void *sdrv);
extern int imx378_deinit(struct i2c_client *client);
extern int imx378_sensor_id(struct i2c_client *client);
extern int imx378_power_on(struct device *dev, struct sensor_gpio *gpio);
extern int imx378_power_off(struct device *dev, struct sensor_gpio *gpio);
extern int imx378_power_suspend(struct device *dev);
extern int imx378_power_resume(struct device *dev);
