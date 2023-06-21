// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2023 Amlogic, Inc. All rights reserved.
 */

#include "sensor_drv.h"
#include "i2c_api.h"
#include "mclk_api.h"
#include "aml_ov13b10.h"

#define AML_SENSOR_NAME  "ov13b10-%u"

/* supported link frequencies */
#define FREQ_INDEX_1080P	0
#define FREQ_INDEX_720P		1

/* supported link frequencies */
static const s64 ov13b10_link_freq_2lanes[] = {
	[FREQ_INDEX_1080P] = 1080000000,
	[FREQ_INDEX_720P] = 1080000000,
};

static const s64 ov13b10_link_freq_4lanes[] = {
	[FREQ_INDEX_1080P] = 1080000000,
	[FREQ_INDEX_720P] = 1080000000,
};

static inline const s64 *ov13b10_link_freqs_ptr(const struct ov13b10 *ov13b10)
{
	if (ov13b10->nlanes == 2)
		return ov13b10_link_freq_2lanes;
	else {
		return ov13b10_link_freq_4lanes;
	}
}

static inline int ov13b10_link_freqs_num(const struct ov13b10 *ov13b10)
{
	if (ov13b10->nlanes == 2)
		return ARRAY_SIZE(ov13b10_link_freq_2lanes);
	else
		return ARRAY_SIZE(ov13b10_link_freq_4lanes);
}

/* Mode configs */
static const struct ov13b10_mode ov13b10_modes_2lanes[] = {
	{
		.width = 1920,
		.height = 1080,
		.hmax  = 0x1130,
		.data = ov13b10_1080p_settings,
		.data_size = ARRAY_SIZE(ov13b10_1080p_settings),

		.link_freq_index = FREQ_INDEX_1080P,
	},
	{
		.width = 1280,
		.height = 720,
		.hmax = 0x19c8,
		.data = ov13b10_720p_settings,
		.data_size = ARRAY_SIZE(ov13b10_720p_settings),

		.link_freq_index = FREQ_INDEX_720P,
	},
};


static const struct ov13b10_mode ov13b10_modes_4lanes[] = {
	{
		.width = 4208,
		.height = 3120,
		.hmax = 0x0898,
		.link_freq_index = FREQ_INDEX_1080P,
		.data = ov13b10_1080p_settings,
		.data_size = ARRAY_SIZE(ov13b10_1080p_settings),
	},
	{
		.width = 4208,
		.height = 3120,
		.hmax = 0x0ce4,
		.link_freq_index = FREQ_INDEX_720P,
		.data = ov13b10_720p_settings,
		.data_size = ARRAY_SIZE(ov13b10_720p_settings),
	},
};

static inline const struct ov13b10_mode *ov13b10_modes_ptr(const struct ov13b10 *ov13b10)
{
	if (ov13b10->nlanes == 2)
		return ov13b10_modes_2lanes;
	else
		return ov13b10_modes_4lanes;
}

static inline int ov13b10_modes_num(const struct ov13b10 *ov13b10)
{
	if (ov13b10->nlanes == 2)
		return ARRAY_SIZE(ov13b10_modes_2lanes);
	else
		return ARRAY_SIZE(ov13b10_modes_4lanes);
}

static inline struct ov13b10 *to_ov13b10(struct v4l2_subdev *_sd)
{
	return container_of(_sd, struct ov13b10, sd);
}

static inline int ov13b10_read_reg(struct ov13b10 *ov13b10, u16 addr, u8 *value)
{
	unsigned int regval;

	int i, ret;

	for (i = 0; i < 3; ++i) {
		ret = regmap_read(ov13b10->regmap, addr, &regval);
		if (0 == ret ) {
			break;
		}
	}

	if (ret)
		dev_err(ov13b10->dev, "I2C read with i2c transfer failed for addr: %x, ret %d\n", addr, ret);

	*value = regval & 0xff;
	return 0;
}

static int ov13b10_write_reg(struct ov13b10 *ov13b10, u16 addr, u8 value)
{
	int i, ret;

	for (i = 0; i < 3; i++) {
		ret = regmap_write(ov13b10->regmap, addr, value);
		if (0 == ret) {
			break;
		}
	}

	if (ret)
		dev_err(ov13b10->dev, "I2C write failed for addr: %x, ret %d\n", addr, ret);

	return ret;
}

static int ov13b10_set_register_array(struct ov13b10 *ov13b10,
				     const struct ov13b10_regval *settings,
				     unsigned int num_settings)
{
	unsigned int i;
	int ret = 0;

	for (i = 0; i < num_settings; ++i, ++settings) {
		ret = ov13b10_write_reg(ov13b10, settings->reg, settings->val);
		if (ret < 0)
			return ret;
	}

	return 0;
}

/*
static int ov13b10_write_buffered_reg(struct ov13b10 *ov13b10, u16 address_low,
				     u8 nr_regs, u32 value)
{
	unsigned int i;
	int ret;

	for (i = 0; i < nr_regs; i++) {
		ret = ov13b10_write_reg(ov13b10, address_low + i,
				       (u8)(value >> ((1-i) * 8)));
		if (ret) {
			dev_err(ov13b10->dev, "Error writing buffered registers\n");
			return ret;
		}
	}

	return ret;
}
*/

static uint16_t again_table_1[16] =
{
	0x0,0x10,0x20,0x30,0x40,0x50,0x60,0x70,0x80,0x90,0xA0,0xB0,0xC0,0xD0,0xE0,0xF0
};
static uint16_t again_table_2[8] =
{
	0x0,0x20,0x40,0x60,0x80,0xA0,0xC0,0xE0
};
static uint16_t again_table_3[4] =
{
	0x0,0x40,0x80,0xC0
};

static int ov13b10_set_gain(struct ov13b10 *ov13b10, u32 value)
{
	int ret = 0;
	int i = 0;
	u8 value_H = 0;
	u8 value_L = 0;

	//dev_info(ov13b10->dev, "ov13b10_set_gain = 0x%x \n", value);

	value_H = value >> 8;
	value_L = value & 0xFF;

	if ( value_H == 1) {
		for ( i = 0; i < 15; i++ ) {
			if ( value_L < ((again_table_1[i] + again_table_1[i + 1])/2) ) {
				value_L = again_table_1[i];
				break;
			} else {
				if ((value_L < again_table_1[i + 1]) || (value_L == again_table_1[i + 1])) {
					value_L = again_table_1[i + 1];
					break;
				}
			}
		}
		if (value_L >  again_table_1[15]) {
			value_L = again_table_1[15];
		}
	} else if (value_H > 1 && value_H < 4) {
			for ( i = 0; i < 7; i++ ) {
				if ( value_L < ((again_table_2[i] + again_table_2[i + 1])/2) ) {
					value_L = again_table_2[i];
					break;
				} else {
					if ((value_L < again_table_2[i + 1]) || (value_L == again_table_2[i + 1])) {
						value_L = again_table_2[i + 1];
						break;
					}
				}
			}
			if (value_L >  again_table_2[7]) {
				value_L = again_table_2[7];
			}
		} else if (value_H >= 4 && value_H < 8) {
				for ( i = 0; i < 3; i++ ) {
					if ( value_L < ((again_table_3[i] + again_table_3[i + 1])/2) ) {
						value_L = again_table_3[i];
						break;
					} else {
						if ((value_L < again_table_3[i + 1]) || (value_L == again_table_3[i + 1])) {
							value_L = again_table_3[i + 1];
							break;
						}
					}
				}
				if (value_L >  again_table_3[3]) {
					value_L = again_table_3[3];
				}
			} else if (value_H >= 8 && value_H <= 0x0F) {
				if ( value_L < (0x80/2)) {
						value_L = 0;
					} else {
						value_L = 0x80;
			        }
	        } else {
				dev_err(ov13b10->dev, "Wrong gain value \n");
			}
	ret = ov13b10_write_reg(ov13b10, OV13B10_GAIN, value_H);
	if (ret)
		dev_err(ov13b10->dev, "Unable to write OV13B10_GAIN_H \n");
	ret = ov13b10_write_reg(ov13b10, OV13B10_GAIN_L, value_L);
	if (ret)
		dev_err(ov13b10->dev, "Unable to write OV13B10_GAIN_L \n");

	//ov13b10_read_reg(ov13b10, OV13B10_GAIN, &value_H);
	//ov13b10_read_reg(ov13b10, OV13B10_GAIN_L, &value_L);
	//dev_info(ov13b10->dev, "ov13b10 read gain = 0x%x \n", (((value_H << 8) | value_L) >> 1));

	return ret;
}

static int ov13b10_set_exposure(struct ov13b10 *ov13b10, u32 value)
{
	int ret;

	//dev_info(ov13b10->dev, "ov13b10_set_exposure = 0x%x \n", value);

	ret = ov13b10_write_reg(ov13b10, OV13B10_EXPOSURE, (value >> 8) & 0xFF);
	if (ret)
		dev_err(ov13b10->dev, "Unable to write gain_H\n");
	ret = ov13b10_write_reg(ov13b10, OV13B10_EXPOSURE_L, value & 0xFF);
	if (ret)
		dev_err(ov13b10->dev, "Unable to write gain_L\n");

	return ret;
}

static int ov13b10_set_fps(struct ov13b10 *ov13b10, u32 value)
{
	u32 vts = 0;
	u8 vts_h, vts_l;

	dev_info(ov13b10->dev, "ov13b10_set_fps = %d \n", value);

	vts = 30 * 0x0cc0 / value;
	vts_h = (vts >> 8) & 0x7f;
	vts_l = vts & 0xff;

	ov13b10_write_reg(ov13b10, 0x380e, vts_h);
	ov13b10_write_reg(ov13b10, 0x380f, vts_l);

	return 0;
}

/* Stop streaming */
static int ov13b10_stop_streaming(struct ov13b10 *ov13b10)
{
	ov13b10->enWDRMode = WDR_MODE_NONE;
	return ov13b10_write_reg(ov13b10, 0x0100, 0x00);
}

static int ov13b10_set_ctrl(struct v4l2_ctrl *ctrl)
{
	struct ov13b10 *ov13b10 = container_of(ctrl->handler,
					     struct ov13b10, ctrls);
	int ret = 0;

	/* V4L2 controls values will be applied only when power is already up */
	if (!pm_runtime_get_if_in_use(ov13b10->dev))
		return 0;

	switch (ctrl->id) {
	case V4L2_CID_GAIN:
		ret = ov13b10_set_gain(ov13b10, ctrl->val);
		break;
	case V4L2_CID_EXPOSURE:
		ret = ov13b10_set_exposure(ov13b10, ctrl->val);
		break;
	case V4L2_CID_HBLANK:
		break;
	case V4L2_CID_AML_CSI_LANES:
		break;
	case V4L2_CID_AML_MODE:
		ov13b10->enWDRMode = ctrl->val;
		break;
	case V4L2_CID_AML_ORIG_FPS:
		ret = ov13b10_set_fps(ov13b10, ctrl->val);
		break;
	default:
		dev_err(ov13b10->dev, "Error ctrl->id %u, flag 0x%lx\n",
			ctrl->id, ctrl->flags);
		ret = -EINVAL;
		break;
	}

	pm_runtime_put(ov13b10->dev);

	return ret;
}

static int ov13b10_get_ctrl(struct v4l2_ctrl *ctrl)
{
	struct ov13b10 *ov13b10 = container_of(ctrl->handler,
					     struct ov13b10, ctrls);
	int ret = 0;

	switch (ctrl->id) {
	case V4L2_CID_AML_ADDRESS:
		dev_err(ov13b10->dev, "i2c_addr 0x%x\n", ov13b10->i2c_addr);
		ctrl->val = ov13b10->i2c_addr;
		break;
	default:
		dev_err(ov13b10->dev, "Error ctrl->id %u, flag 0x%lx\n",
			ctrl->id, ctrl->flags);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static const struct v4l2_ctrl_ops ov13b10_ctrl_ops = {
	.s_ctrl = ov13b10_set_ctrl,
	.g_volatile_ctrl = ov13b10_get_ctrl,
};
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int ov13b10_enum_mbus_code(struct v4l2_subdev *sd,
				  struct v4l2_subdev_state *cfg,
				 struct v4l2_subdev_mbus_code_enum *code)
#else
static int ov13b10_enum_mbus_code(struct v4l2_subdev *sd,
				 struct v4l2_subdev_pad_config *cfg,
				 struct v4l2_subdev_mbus_code_enum *code)
#endif
{
	if (code->index >= ARRAY_SIZE(ov13b10_formats))
		return -EINVAL;

	code->code = ov13b10_formats[code->index].code;

	return 0;
}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int ov13b10_enum_frame_size(struct v4l2_subdev *sd,
			        struct v4l2_subdev_state *cfg,
			       struct v4l2_subdev_frame_size_enum *fse)
#else
static int ov13b10_enum_frame_size(struct v4l2_subdev *sd,
			       struct v4l2_subdev_pad_config *cfg,
			       struct v4l2_subdev_frame_size_enum *fse)
#endif
{
	if (fse->index >= ARRAY_SIZE(ov13b10_formats))
		return -EINVAL;

	fse->min_width = ov13b10_formats[fse->index].min_width;
	fse->min_height = ov13b10_formats[fse->index].min_height;;
	fse->max_width = ov13b10_formats[fse->index].max_width;
	fse->max_height = ov13b10_formats[fse->index].max_height;

	return 0;
}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int ov13b10_get_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_state *cfg,
			  struct v4l2_subdev_format *fmt)
#else
static int ov13b10_get_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *fmt)
#endif
{
	struct ov13b10 *ov13b10 = to_ov13b10(sd);
	struct v4l2_mbus_framefmt *framefmt;

	mutex_lock(&ov13b10->lock);

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY)
		framefmt = v4l2_subdev_get_try_format(&ov13b10->sd, cfg,
						      fmt->pad);
	else
		framefmt = &ov13b10->current_format;

	fmt->format = *framefmt;

	mutex_unlock(&ov13b10->lock);

	return 0;
}


static inline u8 ov13b10_get_link_freq_index(struct ov13b10 *ov13b10)
{
	return ov13b10->current_mode->link_freq_index;
}

static s64 ov13b10_get_link_freq(struct ov13b10 *ov13b10)
{
	u8 index = ov13b10_get_link_freq_index(ov13b10);

	return *(ov13b10_link_freqs_ptr(ov13b10) + index);
}

static u64 ov13b10_calc_pixel_rate(struct ov13b10 *ov13b10)
{
	s64 link_freq = ov13b10_get_link_freq(ov13b10);
	u8 nlanes = ov13b10->nlanes;
	u64 pixel_rate;

	/* pixel rate = link_freq * 2 * nr_of_lanes / bits_per_sample */
	pixel_rate = link_freq * 2 * nlanes;
	do_div(pixel_rate, ov13b10->bpp);
	return pixel_rate;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int ov13b10_set_fmt(struct v4l2_subdev *sd,
			struct v4l2_subdev_state *cfg,
			struct v4l2_subdev_format *fmt)
#else
static int ov13b10_set_fmt(struct v4l2_subdev *sd,
			struct v4l2_subdev_pad_config *cfg,
			struct v4l2_subdev_format *fmt)
#endif
{
	struct ov13b10 *ov13b10 = to_ov13b10(sd);
	const struct ov13b10_mode *mode;
	struct v4l2_mbus_framefmt *format;
	unsigned int i;
	int ret;

	mutex_lock(&ov13b10->lock);

	mode = v4l2_find_nearest_size(ov13b10_modes_ptr(ov13b10),
				 ov13b10_modes_num(ov13b10),
				width, height,
				fmt->format.width, fmt->format.height);

	fmt->format.width = mode->width;
	fmt->format.height = mode->height;

	for (i = 0; i < ARRAY_SIZE(ov13b10_formats); i++) {
		if (ov13b10_formats[i].code == fmt->format.code) {
			dev_info(ov13b10->dev, "find proper format %d \n",i);
			break;
		}
	}

	if (i >= ARRAY_SIZE(ov13b10_formats)) {
		i = 0;
		dev_err(ov13b10->dev, "No format. reset i = 0 \n");
	}

	fmt->format.code = ov13b10_formats[i].code;
	fmt->format.field = V4L2_FIELD_NONE;

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		dev_info(ov13b10->dev, "try format \n");
		format = v4l2_subdev_get_try_format(sd, cfg, fmt->pad);
		mutex_unlock(&ov13b10->lock);
		return 0;
	} else {
		dev_err(ov13b10->dev, "set format, w %d, h %d, code 0x%x \n",
            fmt->format.width, fmt->format.height,
            fmt->format.code);
		format = &ov13b10->current_format;
		ov13b10->current_mode = mode;
		ov13b10->bpp = ov13b10_formats[i].bpp;
		ov13b10->nlanes = 4;

		if (ov13b10->link_freq)
			__v4l2_ctrl_s_ctrl(ov13b10->link_freq, ov13b10_get_link_freq_index(ov13b10));
		if (ov13b10->pixel_rate)
			__v4l2_ctrl_s_ctrl_int64(ov13b10->pixel_rate, ov13b10_calc_pixel_rate(ov13b10));
		if (ov13b10->data_lanes)
			__v4l2_ctrl_s_ctrl(ov13b10->data_lanes, ov13b10->nlanes);
	}

	*format = fmt->format;
	ov13b10->status = 0;

	mutex_unlock(&ov13b10->lock);
	if (ov13b10->enWDRMode) {
		/* Set init register settings */
		ret = ov13b10_set_register_array(ov13b10, setting_4208_3120_4lane_1080m_30fps,
			ARRAY_SIZE(setting_4208_3120_4lane_1080m_30fps));
		if (ret < 0) {
			dev_err(ov13b10->dev, "Could not set init registers\n");
			return ret;
		} else
			dev_err(ov13b10->dev, "ov13b10 wdr mode init...\n");
	} else {
		/* Set init register settings */
		ret = ov13b10_set_register_array(ov13b10, setting_4208_3120_4lane_1080m_30fps,
			ARRAY_SIZE(setting_4208_3120_4lane_1080m_30fps));
		if (ret < 0) {
			dev_err(ov13b10->dev, "Could not set init registers\n");
			return ret;
		} else
			dev_err(ov13b10->dev, "ov13b10 linear mode init...\n");
	}

	return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
int ov13b10_get_selection(struct v4l2_subdev *sd,
			     struct v4l2_subdev_state *cfg,
			     struct v4l2_subdev_selection *sel)
#else
int ov13b10_get_selection(struct v4l2_subdev *sd,
			     struct v4l2_subdev_pad_config *cfg,
			     struct v4l2_subdev_selection *sel)
#endif
{
	int rtn = 0;
	struct ov13b10 *ov13b10 = to_ov13b10(sd);
	const struct ov13b10_mode *mode = ov13b10->current_mode;

	switch (sel->target) {
	case V4L2_SEL_TGT_CROP_DEFAULT:
		sel->r.left = 0;
		sel->r.top = 0;
		sel->r.width = mode->width;
		sel->r.height = mode->height;
	break;
	case V4L2_SEL_TGT_CROP:
		sel->r.left = 0;
		sel->r.top = 0;
		sel->r.width = mode->width;
		sel->r.height = mode->height;
	break;
	default:
		rtn = -EINVAL;
		dev_err(ov13b10->dev, "Error support target: 0x%x\n", sel->target);
	break;
	}

	return rtn;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int ov13b10_entity_init_cfg(struct v4l2_subdev *subdev,
				  struct v4l2_subdev_state *cfg)
#else
static int ov13b10_entity_init_cfg(struct v4l2_subdev *subdev,
				  struct v4l2_subdev_pad_config *cfg)
#endif
{
	struct v4l2_subdev_format fmt = { 0 };

	fmt.which = cfg ? V4L2_SUBDEV_FORMAT_TRY : V4L2_SUBDEV_FORMAT_ACTIVE;
	fmt.format.width = 4208;
	fmt.format.height = 3120;
	fmt.format.code = MEDIA_BUS_FMT_SBGGR10_1X10;

	ov13b10_set_fmt(subdev, cfg, &fmt);

	return 0;
}

/* Start streaming */
static int ov13b10_start_streaming(struct ov13b10 *ov13b10)
{

	/* Start streaming */
	return ov13b10_write_reg(ov13b10, 0x0100, 0x01);
}

static int ov13b10_set_stream(struct v4l2_subdev *sd, int enable)
{
	struct ov13b10 *ov13b10 = to_ov13b10(sd);
	int ret = 0;

	if (ov13b10->status == enable)
		return ret;
	else
		ov13b10->status = enable;

	if (enable) {
		ret = ov13b10_start_streaming(ov13b10);
		if (ret) {
			dev_err(ov13b10->dev, "Start stream failed\n");
			goto unlock_and_return;
		}

		dev_info(ov13b10->dev, "stream on\n");
	} else {
		ov13b10_stop_streaming(ov13b10);

		dev_info(ov13b10->dev, "stream off\n");
	}

unlock_and_return:

	return ret;
}

/*
int reset_am_enable(struct device *dev, const char* propname, int val)
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
*/

int ov13b10_power_on(struct device *dev, struct sensor_gpio *gpio)
{
	int ret;

	gpiod_set_value_cansleep(gpio->rst_gpio, 1);
	if (!IS_ERR_OR_NULL(gpio->pwdn_gpio)) {
		gpiod_set_value_cansleep(gpio->pwdn_gpio, 1);
	}
	ret = mclk_enable(dev, 24000000);
	if (ret < 0 )
		dev_err(dev, "set mclk fail\n");

	// 30ms
	usleep_range(30000, 31000);

	return 0;
}

int ov13b10_power_off(struct device *dev, struct sensor_gpio *gpio)
{
	mclk_disable(dev);

	gpiod_set_value_cansleep(gpio->rst_gpio, 0);
	if (!IS_ERR_OR_NULL(gpio->pwdn_gpio)) {
		gpiod_set_value_cansleep(gpio->pwdn_gpio, 0);
	}
	return 0;
}

int ov13b10_power_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ov13b10 *ov13b10 = to_ov13b10(sd);

	gpiod_set_value_cansleep(ov13b10->gpio->rst_gpio, 0);

	return 0;
}

int ov13b10_power_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ov13b10 *ov13b10 = to_ov13b10(sd);

	gpiod_set_value_cansleep(ov13b10->gpio->rst_gpio, 1);

	return 0;
}

static int ov13b10_log_status(struct v4l2_subdev *sd)
{
	struct ov13b10 *ov13b10 = to_ov13b10(sd);

	dev_info(ov13b10->dev, "log status done\n");

	return 0;
}

static const struct dev_pm_ops ov13b10_pm_ops = {
	SET_RUNTIME_PM_OPS(ov13b10_power_suspend, ov13b10_power_resume, NULL)
};

const struct v4l2_subdev_core_ops ov13b10_core_ops = {
	.log_status = ov13b10_log_status,
};

static const struct v4l2_subdev_video_ops ov13b10_video_ops = {
	.s_stream = ov13b10_set_stream,
};

static const struct v4l2_subdev_pad_ops ov13b10_pad_ops = {
	.init_cfg = ov13b10_entity_init_cfg,
	.enum_mbus_code = ov13b10_enum_mbus_code,
	.enum_frame_size = ov13b10_enum_frame_size,
	.get_selection = ov13b10_get_selection,
	.get_fmt = ov13b10_get_fmt,
	.set_fmt = ov13b10_set_fmt,
};

static const struct v4l2_subdev_ops ov13b10_subdev_ops = {
	.core = &ov13b10_core_ops,
	.video = &ov13b10_video_ops,
	.pad = &ov13b10_pad_ops,
};

static const struct media_entity_operations ov13b10_subdev_entity_ops = {
	.link_validate = v4l2_subdev_link_validate,
};

static struct v4l2_ctrl_config wdr_cfg = {
	.ops = &ov13b10_ctrl_ops,
	.id = V4L2_CID_AML_MODE,
	.name = "wdr mode",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.min = 0,
	.max = 2,
	.step = 1,
	.def = 0,
};

static struct v4l2_ctrl_config addr_cfg = {
	.ops = &ov13b10_ctrl_ops,
	.id = V4L2_CID_AML_ADDRESS,
	.name = "sensor address",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	.min = 0x00,
	.max = 0xFF,
	.step = 1,
	.def = 0x00,
};

static struct v4l2_ctrl_config fps_cfg = {
	.ops = &ov13b10_ctrl_ops,
	.id = V4L2_CID_AML_ORIG_FPS,
	.name = "sensor fps",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	.min = 1,
	.max = 30,
	.step = 1,
	.def = 30,
};

static struct v4l2_ctrl_config nlane_cfg = {
	.ops = &ov13b10_ctrl_ops,
	.id = V4L2_CID_AML_CSI_LANES,
	.name = "sensor lanes",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.flags = V4L2_CTRL_FLAG_VOLATILE,
	.min = 1,
	.max = 4,
	.step = 1,
	.def = 4,
};

static int ov13b10_ctrls_init(struct ov13b10 *ov13b10)
{
	int rtn = 0;

	v4l2_ctrl_handler_init(&ov13b10->ctrls, 7);

	v4l2_ctrl_new_std(&ov13b10->ctrls, &ov13b10_ctrl_ops,
				V4L2_CID_GAIN, 0, 0xFFFF, 1, 0);

	v4l2_ctrl_new_std(&ov13b10->ctrls, &ov13b10_ctrl_ops,
				V4L2_CID_EXPOSURE, 0, 0xffff, 1, 0);

	ov13b10->link_freq = v4l2_ctrl_new_int_menu(&ov13b10->ctrls,
					       &ov13b10_ctrl_ops,
					       V4L2_CID_LINK_FREQ,
					       ov13b10_link_freqs_num(ov13b10) - 1,
					       0, ov13b10_link_freqs_ptr(ov13b10) );

	if (ov13b10->link_freq)
		ov13b10->link_freq->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	ov13b10->pixel_rate = v4l2_ctrl_new_std(&ov13b10->ctrls,
					       &ov13b10_ctrl_ops,
					       V4L2_CID_PIXEL_RATE,
					       1, INT_MAX, 1,
					       ov13b10_calc_pixel_rate(ov13b10));

	ov13b10->data_lanes = v4l2_ctrl_new_custom(&ov13b10->ctrls, &nlane_cfg, NULL);
	if (ov13b10->data_lanes)
		ov13b10->data_lanes->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	ov13b10->wdr = v4l2_ctrl_new_custom(&ov13b10->ctrls, &wdr_cfg, NULL);
	ov13b10->address = v4l2_ctrl_new_custom(&ov13b10->ctrls, &addr_cfg, NULL);

	v4l2_ctrl_new_custom(&ov13b10->ctrls, &fps_cfg, NULL);

	ov13b10->sd.ctrl_handler = &ov13b10->ctrls;

	if (ov13b10->ctrls.error) {
		dev_err(ov13b10->dev, "Control initialization a error  %d\n",
			ov13b10->ctrls.error);
		rtn = ov13b10->ctrls.error;
	}

	return rtn;
}

static int ov13b10_register_subdev(struct ov13b10 *ov13b10)
{
	int rtn = 0;

	v4l2_i2c_subdev_init(&ov13b10->sd, ov13b10->client, &ov13b10_subdev_ops);

	ov13b10->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	ov13b10->sd.dev = &ov13b10->client->dev;
	ov13b10->sd.entity.ops = &ov13b10_subdev_entity_ops;
	ov13b10->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;
	snprintf(ov13b10->sd.name, sizeof(ov13b10->sd.name), AML_SENSOR_NAME, ov13b10->index);

	ov13b10->pad.flags = MEDIA_PAD_FL_SOURCE;
	rtn = media_entity_pads_init(&ov13b10->sd.entity, 1, &ov13b10->pad);
	if (rtn < 0) {
		dev_err(ov13b10->dev, "Could not register media entity\n");
		goto err_return;
	}

	rtn = v4l2_async_register_subdev(&ov13b10->sd);
	if (rtn < 0) {
		dev_err(ov13b10->dev, "Could not register v4l2 device\n");
		goto err_return;
	}

err_return:
	return rtn;
}

int ov13b10_init(struct i2c_client *client, void *sdrv)
{
	struct device *dev = &client->dev;
	struct ov13b10 *ov13b10;
	struct amlsens *sensor = (struct amlsens *)sdrv;
	int ret = -EINVAL;

	ov13b10 = devm_kzalloc(dev, sizeof(*ov13b10), GFP_KERNEL);
	if (!ov13b10)
		return -ENOMEM;

	ov13b10->dev = dev;
	ov13b10->client = client;
	ov13b10->i2c_addr = client->addr;
	ov13b10->gpio = &sensor->gpio;

	ov13b10->regmap = devm_regmap_init_i2c(client, &ov13b10_regmap_config);
	if (IS_ERR(ov13b10->regmap)) {
		dev_err(dev, "Unable to initialize I2C\n");
		return -ENODEV;
	}

	if (of_property_read_u32(dev->of_node, "index", &ov13b10->index)) {
		dev_err(dev, "Failed to read sensor index. default to 0\n");
		ov13b10->index = 0;
	}

	mutex_init(&ov13b10->lock);

	/* Power on the device to match runtime PM state below */
	ret = ov13b10_power_on(ov13b10->dev, ov13b10->gpio);
	if (ret < 0) {
		dev_err(dev, "Could not power on the device\n");
		return -ENODEV;
	}

	/*
	 * Initialize the frame format. In particular, ov13b10->current_mode
	 * and ov13b10->bpp are set to defaults: ov13b10_calc_pixel_rate() call
	 * below in ov13b10_ctrls_init relies on these fields.
	 */
	ov13b10_entity_init_cfg(&ov13b10->sd, NULL);

	ret = ov13b10_ctrls_init(ov13b10);
	if (ret) {
		dev_err(ov13b10->dev, "Error ctrls init\n");
		goto free_ctrl;
	}

	ret = ov13b10_register_subdev(ov13b10);
	if (ret) {
		dev_err(ov13b10->dev, "Error register subdev\n");
		goto free_entity;
	}

	dev_err(ov13b10->dev, "ov13b10 probe done \n");

	return 0;

free_entity:
	media_entity_cleanup(&ov13b10->sd.entity);
free_ctrl:
	v4l2_ctrl_handler_free(&ov13b10->ctrls);
	mutex_destroy(&ov13b10->lock);

	return ret;
}

int ov13b10_deinit(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ov13b10 *ov13b10 = to_ov13b10(sd);

	v4l2_async_unregister_subdev(sd);
	media_entity_cleanup(&sd->entity);
	v4l2_ctrl_handler_free(sd->ctrl_handler);

	mutex_destroy(&ov13b10->lock);

	ov13b10_power_off(ov13b10->dev, ov13b10->gpio);

	return 0;
}

int ov13b10_sensor_id(struct i2c_client *client)
{
	u32 id = 0;
	u8 val = 0;
	int rtn = -EINVAL;

	i2c_read_a16d8(client, OV13B10_SLAVE_ID1, 0x300a, &val);
	id |= (val << 16);
	i2c_read_a16d8(client, OV13B10_SLAVE_ID1, 0x300b, &val);
	id |= (val << 8);
	i2c_read_a16d8(client, OV13B10_SLAVE_ID1, 0x300c, &val);
	id |= val;

	if (id != OV13B10_ID && id != OV13B10_ID_2) {
		dev_info(&client->dev, "Failed to get ov13b10 id1: 0x%x\n", id);
		goto try_id;
	} else {
		dev_err(&client->dev, "success get ov13b10 id1 0x%x", id);
		client->addr = OV13B10_SLAVE_ID1;
		return 0;
	}

try_id:
	i2c_read_a16d8(client, OV13B10_SLAVE_ID, 0x300a, &val);
	id |= (val << 16);
	i2c_read_a16d8(client, OV13B10_SLAVE_ID, 0x300b, &val);
	id |= (val << 8);
	i2c_read_a16d8(client, OV13B10_SLAVE_ID, 0x300c, &val);
	id |= val;

	if (id != OV13B10_ID && id != OV13B10_ID_2) {
		dev_info(&client->dev, "Failed to get ov13b10 id: 0x%x\n", id);
		return rtn;
	} else {
		dev_err(&client->dev, "success get ov13b10 id 0x%x", id);
		client->addr = OV13B10_SLAVE_ID;
	}

	return 0;
}

