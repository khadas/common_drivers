// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2023 Amlogic, Inc. All rights reserved.
 */

#include "sensor_drv.h"
#include "i2c_api.h"
#include "mclk_api.h"
#include "aml_ov13855.h"

#define AML_SENSOR_NAME  "ov13855-%u"

/* supported link frequencies */
static const s64 ov13855_link_freq_2lanes[] = {
	[FREQ_INDEX_1080P] = 1080000000,
	[FREQ_INDEX_720P] = 1080000000,
};

static const s64 ov13855_link_freq_4lanes[] = {
	[FREQ_INDEX_1080P] = 1080000000,
	[FREQ_INDEX_720P] = 1080000000,
};

static inline const s64 *ov13855_link_freqs_ptr(const struct ov13855 *ov13855)
{
	if (ov13855->nlanes == 2)
		return ov13855_link_freq_2lanes;
	else {
		return ov13855_link_freq_4lanes;
	}
}

static inline int ov13855_link_freqs_num(const struct ov13855 *ov13855)
{
	if (ov13855->nlanes == 2)
		return ARRAY_SIZE(ov13855_link_freq_2lanes);
	else
		return ARRAY_SIZE(ov13855_link_freq_4lanes);
}

static const struct ov13855_mode ov13855_modes_2lanes[] = {

};

static const struct ov13855_mode ov13855_modes_4lanes[] = {
	{
		.width = 4224,
		.height = 3136,
		.hmax = 0x0898,
		.link_freq_index = FREQ_INDEX_1080P,
		.data = setting_4224_3136_4lane_1080m_30fps,
		.data_size = ARRAY_SIZE(ov13855_1080p_settings),
	},
	/*
	{
		.width = 4224,
		.height = 3136,
		.hmax = 0x0ce4,
		.link_freq_index = FREQ_INDEX_720P,
		.data = setting_4224_3136_4lane_1080m_30fps,
		.data_size = ARRAY_SIZE(ov13855_720p_settings),
	},
	*/
};

static inline const struct ov13855_mode *ov13855_modes_ptr(const struct ov13855 *ov13855)
{
	if (ov13855->nlanes == 2)
		return ov13855_modes_2lanes;
	else
		return ov13855_modes_4lanes;
}

static inline int ov13855_modes_num(const struct ov13855 *ov13855)
{
	if (ov13855->nlanes == 2)
		return ARRAY_SIZE(ov13855_modes_2lanes);
	else
		return ARRAY_SIZE(ov13855_modes_4lanes);
}

static inline struct ov13855 *to_ov13855(struct v4l2_subdev *_sd)
{
	return container_of(_sd, struct ov13855, sd);
}

static inline int ov13855_read_reg(struct ov13855 *ov13855, u16 addr, u8 *value)
{
	unsigned int regval;

	int i, ret;

	for (i = 0; i < 3; ++i) {
		ret = regmap_read(ov13855->regmap, addr, &regval);
		if (0 == ret) {
			break;
		}
	}

	if (ret)
		dev_err(ov13855->dev, "I2C read with i2c transfer failed for addr: %x, ret %d\n", addr, ret);

	*value = regval & 0xff;
	return 0;
}

static int ov13855_write_reg(struct ov13855 *ov13855, u16 addr, u8 value)
{
	int i, ret;

	for (i = 0; i < 3; i++) {
		ret = regmap_write(ov13855->regmap, addr, value);
		if (0 == ret) {
			break;
		}
	}

	if (ret)
		dev_err(ov13855->dev, "I2C write failed for addr: %x, ret %d\n", addr, ret);

	return ret;
}

static int ov13855_set_register_array(struct ov13855 *ov13855,
				     const struct ov13855_regval *settings,
				     unsigned int num_settings)
{
	unsigned int i;
	int ret = 0;

	for (i = 0; i < num_settings; ++i, ++settings) {
		ret = ov13855_write_reg(ov13855, settings->reg, settings->val);
		if (ret < 0)
			return ret;
	}

	return 0;
}

/*
static int ov13855_write_buffered_reg(struct ov13855 *ov13855, u16 address_low,
				     u8 nr_regs, u32 value)
{
	unsigned int i;
	int ret;

	for (i = 0; i < nr_regs; i++) {
		ret = ov13855_write_reg(ov13855, address_low + i,
				       (u8)(value >> ((1-i) * 8)));
		if (ret) {
			dev_err(ov13855->dev, "Error writing buffered registers\n");
			return ret;
		}
	}

	return ret;
}
*/

static int ov13855_set_gain(struct ov13855 *ov13855, u32 value)
{
	int ret = 0;
	u8 value_H = 0;
	u8 value_L = 0;

	dev_dbg(ov13855->dev, "ov13855_set_gain = 0x%x \n", value);
	value_H = value >> 1;
	ret = ov13855_write_reg(ov13855, OV13855_GAIN, (value >> 8) & 0xFF);
	if (ret)
		dev_err(ov13855->dev, "Unable to write ov13855_GAIN_H \n");
	ret = ov13855_write_reg(ov13855, OV13855_GAIN_L, value & 0xFF);
	if (ret)
		dev_err(ov13855->dev, "Unable to write ov13855_GAIN_L \n");

	ov13855_read_reg(ov13855, OV13855_GAIN, &value_H);
	ov13855_read_reg(ov13855, OV13855_GAIN_L, &value_L);
	//dev_err(ov13855->dev, "ov13855 read gain = 0x%x \n", (value_H << 8) | value_L);
	return ret;
}

static int ov13855_set_exposure(struct ov13855 *ov13855, u32 value)
{
	int ret = 0;
	u8 value_H = 0;
	u8 value_L = 0;
	dev_dbg(ov13855->dev, "ov13855_set_exposure = 0x%x \n", value);
	value_H = (value >> 4) & 0xFF;
	value_L = (value << 4) & 0xFF;
	ret = ov13855_write_reg(ov13855, OV13855_EXPOSURE, value_H);
	if (ret)
		dev_err(ov13855->dev, "Unable to write EXPOSURE_H\n");
	ret = ov13855_write_reg(ov13855, OV13855_EXPOSURE_L, value_L);
	if (ret)
		dev_err(ov13855->dev, "Unable to write EXPOSURE_L\n");

	ov13855_read_reg(ov13855, OV13855_EXPOSURE, &value_H);
	ov13855_read_reg(ov13855, OV13855_EXPOSURE_L, &value_L);
	dev_err(ov13855->dev, "ov13855 read exposure = 0x%x \n", (value_H << 4) | (value_L >> 4));
	return ret;
}

static int ov13855_set_fps(struct ov13855 *ov13855, u32 value)
{
	u32 vts = 0;
	u8 vts_h, vts_l;

	dev_info(ov13855->dev, "ov13855_set_fps = %d \n", value);

	vts = 30 * 0xCA0 / value;
	vts_h = (vts >> 8) & 0x7f;
	vts_l = vts & 0xff;

	ov13855_write_reg(ov13855, 0x380e, vts_h);
	ov13855_write_reg(ov13855, 0x380f, vts_l);

	return 0;
}

/* Stop streaming */
static int ov13855_stop_streaming(struct ov13855 *ov13855)
{
	ov13855->enWDRMode = WDR_MODE_NONE;
	return ov13855_write_reg(ov13855, 0x0100, 0x00);
}

static int ov13855_set_ctrl(struct v4l2_ctrl *ctrl)
{
	struct ov13855 *ov13855 = container_of(ctrl->handler,
					     struct ov13855, ctrls);
	int ret = 0;

	/* V4L2 controls values will be applied only when power is already up */
	if (!pm_runtime_get_if_in_use(ov13855->dev))
		return 0;

	switch (ctrl->id) {
	case V4L2_CID_GAIN:
		ret = ov13855_set_gain(ov13855, ctrl->val);
		break;
	case V4L2_CID_EXPOSURE:
		ret = ov13855_set_exposure(ov13855, ctrl->val);
		break;
	case V4L2_CID_HBLANK:
		break;
	case V4L2_CID_AML_CSI_LANES:
		break;
	case V4L2_CID_AML_MODE:
		ov13855->enWDRMode = ctrl->val;
		break;
	case V4L2_CID_AML_ORIG_FPS:
		ret = ov13855_set_fps(ov13855, ctrl->val);
		break;
	default:
		dev_err(ov13855->dev, "Error ctrl->id %u, flag 0x%lx\n",
			ctrl->id, ctrl->flags);
		ret = -EINVAL;
		break;
	}

	pm_runtime_put(ov13855->dev);

	return ret;
}

static const struct v4l2_ctrl_ops ov13855_ctrl_ops = {
	.s_ctrl = ov13855_set_ctrl,
};
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int ov13855_enum_mbus_code(struct v4l2_subdev *sd,
				  struct v4l2_subdev_state *cfg,
				 struct v4l2_subdev_mbus_code_enum *code)
#else
static int ov13855_enum_mbus_code(struct v4l2_subdev *sd,
				 struct v4l2_subdev_pad_config *cfg,
				 struct v4l2_subdev_mbus_code_enum *code)
#endif
{
	if (code->index >= ARRAY_SIZE(ov13855_formats))
		return -EINVAL;

	code->code = ov13855_formats[code->index].code;

	return 0;
}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int ov13855_enum_frame_size(struct v4l2_subdev *sd,
			        struct v4l2_subdev_state *cfg,
			       struct v4l2_subdev_frame_size_enum *fse)
#else
static int ov13855_enum_frame_size(struct v4l2_subdev *sd,
			       struct v4l2_subdev_pad_config *cfg,
			       struct v4l2_subdev_frame_size_enum *fse)
#endif
{
	if (fse->index >= ARRAY_SIZE(ov13855_formats))
		return -EINVAL;

	fse->min_width = ov13855_formats[fse->index].min_width;
	fse->min_height = ov13855_formats[fse->index].min_height;;
	fse->max_width = ov13855_formats[fse->index].max_width;
	fse->max_height = ov13855_formats[fse->index].max_height;

	return 0;
}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int ov13855_get_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_state *cfg,
			  struct v4l2_subdev_format *fmt)
#else
static int ov13855_get_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *fmt)
#endif
{
	struct ov13855 *ov13855 = to_ov13855(sd);
	struct v4l2_mbus_framefmt *framefmt;

	mutex_lock(&ov13855->lock);

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY)
		framefmt = v4l2_subdev_get_try_format(&ov13855->sd, cfg,
						      fmt->pad);
	else
		framefmt = &ov13855->current_format;

	fmt->format = *framefmt;

	mutex_unlock(&ov13855->lock);

	return 0;
}


static inline u8 ov13855_get_link_freq_index(struct ov13855 *ov13855)
{
	return ov13855->current_mode->link_freq_index;
}

static s64 ov13855_get_link_freq(struct ov13855 *ov13855)
{
	u8 index = ov13855_get_link_freq_index(ov13855);

	return *(ov13855_link_freqs_ptr(ov13855) + index);
}

static u64 ov13855_calc_pixel_rate(struct ov13855 *ov13855)
{
	s64 link_freq = ov13855_get_link_freq(ov13855);
	u8 nlanes = ov13855->nlanes;
	u64 pixel_rate;

	/* pixel rate = link_freq * 2 * nr_of_lanes / bits_per_sample */
	pixel_rate = link_freq * 2 * nlanes;
	do_div(pixel_rate, ov13855->bpp);
	return pixel_rate;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int ov13855_set_fmt(struct v4l2_subdev *sd,
			struct v4l2_subdev_state *cfg,
			struct v4l2_subdev_format *fmt)
#else
static int ov13855_set_fmt(struct v4l2_subdev *sd,
			struct v4l2_subdev_pad_config *cfg,
			struct v4l2_subdev_format *fmt)
#endif
{
	struct ov13855 *ov13855 = to_ov13855(sd);
	const struct ov13855_mode *mode;
	struct v4l2_mbus_framefmt *format;
	unsigned int i,ret;

	mutex_lock(&ov13855->lock);

	mode = v4l2_find_nearest_size(ov13855_modes_ptr(ov13855),
				 ov13855_modes_num(ov13855),
				width, height,
				fmt->format.width, fmt->format.height);

	fmt->format.width = mode->width;
	fmt->format.height = mode->height;

	for (i = 0; i < ARRAY_SIZE(ov13855_formats); i++) {
		if (ov13855_formats[i].code == fmt->format.code) {
			dev_info(ov13855->dev, "find proper format %d \n",i);
			break;
		}
	}

	if (i >= ARRAY_SIZE(ov13855_formats)) {
		i = 0;
		dev_err(ov13855->dev, "No format. reset i = 0 \n");
	}

	fmt->format.code = ov13855_formats[i].code;
	fmt->format.field = V4L2_FIELD_NONE;

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		dev_info(ov13855->dev, "try format \n");
		format = v4l2_subdev_get_try_format(sd, cfg, fmt->pad);
		mutex_unlock(&ov13855->lock);
		return 0;
	} else {
		dev_err(ov13855->dev, "set format, w %d, h %d, code 0x%x \n",
			fmt->format.width, fmt->format.height,
			fmt->format.code);
		format = &ov13855->current_format;
		ov13855->current_mode = mode;
		ov13855->bpp = ov13855_formats[i].bpp;
		ov13855->nlanes = 4;

		if (ov13855->link_freq)
			__v4l2_ctrl_s_ctrl(ov13855->link_freq, ov13855_get_link_freq_index(ov13855) );
		if (ov13855->pixel_rate)
			__v4l2_ctrl_s_ctrl_int64(ov13855->pixel_rate, ov13855_calc_pixel_rate(ov13855) );
		if (ov13855->data_lanes)
			__v4l2_ctrl_s_ctrl(ov13855->data_lanes, ov13855->nlanes);
	}

	*format = fmt->format;
	ov13855->status = 0;

	mutex_unlock(&ov13855->lock);

	if (ov13855->enWDRMode) {
		/* Set init register settings */
		ret = ov13855_set_register_array(ov13855, setting_4224_3136_4lane_1080m_30fps,
			ARRAY_SIZE(setting_4224_3136_4lane_1080m_30fps));
		if (ret < 0) {
			dev_err(ov13855->dev, "Could not set init registers\n");
			return ret;
		} else
			dev_err(ov13855->dev, "ov13855 wdr mode init...\n");
	} else {
		ret = ov13855_set_register_array(ov13855, setting_4224_3136_4lane_1080m_30fps,
			ARRAY_SIZE(setting_4224_3136_4lane_1080m_30fps));
		if (ret < 0) {
			dev_err(ov13855->dev, "Could not set init registers\n");
			return ret;
		} else
			dev_err(ov13855->dev, "ov13855 linear mode init...\n");
	}

	return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
int ov13855_get_selection(struct v4l2_subdev *sd,
			     struct v4l2_subdev_state *cfg,
			     struct v4l2_subdev_selection *sel)
#else
int ov13855_get_selection(struct v4l2_subdev *sd,
			     struct v4l2_subdev_pad_config *cfg,
			     struct v4l2_subdev_selection *sel)
#endif
{
	int rtn = 0;
	struct ov13855 *ov13855 = to_ov13855(sd);
	const struct ov13855_mode *mode = ov13855->current_mode;

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
			dev_err(ov13855->dev, "Error support target: 0x%x\n", sel->target);
			break;
	}

	return rtn;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int ov13855_entity_init_cfg(struct v4l2_subdev *subdev,
				  struct v4l2_subdev_state *cfg)
#else
static int ov13855_entity_init_cfg(struct v4l2_subdev *subdev,
				  struct v4l2_subdev_pad_config *cfg)
#endif
{
	struct v4l2_subdev_format fmt = { 0 };

	fmt.which = cfg ? V4L2_SUBDEV_FORMAT_TRY : V4L2_SUBDEV_FORMAT_ACTIVE;
	fmt.format.width = 4224;
	fmt.format.height = 3136;
	fmt.format.code = MEDIA_BUS_FMT_SBGGR10_1X10;

	ov13855_set_fmt(subdev, cfg, &fmt);

	return 0;
}

/* Start streaming */
static int ov13855_start_streaming(struct ov13855 *ov13855)
{

	/* Start streaming */
	return ov13855_write_reg(ov13855, 0x0100, 0x01);
}

static int ov13855_set_stream(struct v4l2_subdev *sd, int enable)
{
	struct ov13855 *ov13855 = to_ov13855(sd);
	int ret = 0;

	if (ov13855->status == enable)
		return ret;
	else
		ov13855->status = enable;

	if (enable) {
		ret = ov13855_start_streaming(ov13855);
		if (ret) {
			dev_err(ov13855->dev, "Start stream failed\n");
			goto unlock_and_return;
		}

		dev_info(ov13855->dev, "stream on\n");
	} else {
		ov13855_stop_streaming(ov13855);

		dev_info(ov13855->dev, "stream off\n");
	}

unlock_and_return:

	return ret;
}

int ov13855_power_on(struct device *dev, struct sensor_gpio *gpio)
{
	int ret;

	gpiod_set_value_cansleep(gpio->rst_gpio, 1);
	if (!IS_ERR_OR_NULL(gpio->pwdn_gpio)) {
		gpiod_set_value_cansleep(gpio->pwdn_gpio, 1);
	}
	ret = mclk_enable(dev,24000000);
	if (ret < 0 )
		dev_err(dev, "set mclk fail\n");

	udelay(30);
	usleep_range(30000, 31000);

	return 0;
}

int ov13855_power_off(struct device *dev, struct sensor_gpio *gpio)
{
	mclk_disable(dev);

	gpiod_set_value_cansleep(gpio->rst_gpio, 0);
	if (!IS_ERR_OR_NULL(gpio->pwdn_gpio)) {
		gpiod_set_value_cansleep(gpio->pwdn_gpio, 0);
	}
	return 0;
}

int ov13855_power_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ov13855 *ov13855 = to_ov13855(sd);

	gpiod_set_value_cansleep(ov13855->gpio->rst_gpio, 0);

	return 0;
}

int ov13855_power_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ov13855 *ov13855 = to_ov13855(sd);

	gpiod_set_value_cansleep(ov13855->gpio->rst_gpio, 1);

	return 0;
}

static int ov13855_log_status(struct v4l2_subdev *sd)
{
	struct ov13855 *ov13855 = to_ov13855(sd);

	dev_info(ov13855->dev, "log status done\n");

	return 0;
}

static const struct dev_pm_ops ov13855_pm_ops = {
	SET_RUNTIME_PM_OPS(ov13855_power_suspend, ov13855_power_resume, NULL)
};

const struct v4l2_subdev_core_ops ov13855_core_ops = {
	.log_status = ov13855_log_status,
};

static const struct v4l2_subdev_video_ops ov13855_video_ops = {
	.s_stream = ov13855_set_stream,
};

static const struct v4l2_subdev_pad_ops ov13855_pad_ops = {
	.init_cfg = ov13855_entity_init_cfg,
	.enum_mbus_code = ov13855_enum_mbus_code,
	.enum_frame_size = ov13855_enum_frame_size,
	.get_selection = ov13855_get_selection,
	.get_fmt = ov13855_get_fmt,
	.set_fmt = ov13855_set_fmt,
};

static const struct v4l2_subdev_ops ov13855_subdev_ops = {
	.core = &ov13855_core_ops,
	.video = &ov13855_video_ops,
	.pad = &ov13855_pad_ops,
};

static const struct media_entity_operations ov13855_subdev_entity_ops = {
	.link_validate = v4l2_subdev_link_validate,
};

static struct v4l2_ctrl_config wdr_cfg = {
	.ops = &ov13855_ctrl_ops,
	.id = V4L2_CID_AML_MODE,
	.name = "wdr mode",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.min = 0,
	.max = 2,
	.step = 1,
	.def = 0,
};

static struct v4l2_ctrl_config fps_cfg = {
	.ops = &ov13855_ctrl_ops,
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
	.ops = &ov13855_ctrl_ops,
	.id = V4L2_CID_AML_CSI_LANES,
	.name = "sensor lanes",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.flags = V4L2_CTRL_FLAG_VOLATILE,
	.min = 1,
	.max = 4,
	.step = 1,
	.def = 4,
};

static int ov13855_ctrls_init(struct ov13855 *ov13855)
{
	int rtn = 0;

	v4l2_ctrl_handler_init(&ov13855->ctrls, 7);

	v4l2_ctrl_new_std(&ov13855->ctrls, &ov13855_ctrl_ops,
				V4L2_CID_GAIN, 0, 0xFFFF, 1, 0);

	v4l2_ctrl_new_std(&ov13855->ctrls, &ov13855_ctrl_ops,
				V4L2_CID_EXPOSURE, 0, 0xffff, 1, 0);

	ov13855->link_freq = v4l2_ctrl_new_int_menu(&ov13855->ctrls,
					       &ov13855_ctrl_ops,
					       V4L2_CID_LINK_FREQ,
					       ov13855_link_freqs_num(ov13855) - 1,
					       0, ov13855_link_freqs_ptr(ov13855) );

	if (ov13855->link_freq)
		ov13855->link_freq->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	ov13855->pixel_rate = v4l2_ctrl_new_std(&ov13855->ctrls,
					       &ov13855_ctrl_ops,
					       V4L2_CID_PIXEL_RATE,
					       1, INT_MAX, 1,
					       ov13855_calc_pixel_rate(ov13855));

	ov13855->data_lanes = v4l2_ctrl_new_custom(&ov13855->ctrls, &nlane_cfg, NULL);
	if (ov13855->data_lanes)
		ov13855->data_lanes->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	ov13855->wdr = v4l2_ctrl_new_custom(&ov13855->ctrls, &wdr_cfg, NULL);

	v4l2_ctrl_new_custom(&ov13855->ctrls, &fps_cfg, NULL);

	ov13855->sd.ctrl_handler = &ov13855->ctrls;

	if (ov13855->ctrls.error) {
		dev_err(ov13855->dev, "Control initialization a error  %d\n",
			ov13855->ctrls.error);
		rtn = ov13855->ctrls.error;
	}

	return rtn;
}

static int ov13855_register_subdev(struct ov13855 *ov13855)
{
	int rtn = 0;

	v4l2_i2c_subdev_init(&ov13855->sd, ov13855->client, &ov13855_subdev_ops);

	ov13855->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	ov13855->sd.dev = &ov13855->client->dev;
	ov13855->sd.entity.ops = &ov13855_subdev_entity_ops;
	ov13855->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;
	snprintf(ov13855->sd.name, sizeof(ov13855->sd.name), AML_SENSOR_NAME, ov13855->index);

	ov13855->pad.flags = MEDIA_PAD_FL_SOURCE;
	rtn = media_entity_pads_init(&ov13855->sd.entity, 1, &ov13855->pad);
	if (rtn < 0) {
		dev_err(ov13855->dev, "Could not register media entity\n");
		goto err_return;
	}

	rtn = v4l2_async_register_subdev(&ov13855->sd);
	if (rtn < 0) {
		dev_err(ov13855->dev, "Could not register v4l2 device\n");
		goto err_return;
	}

err_return:
	return rtn;
}

int ov13855_init(struct i2c_client *client, void *sdrv)
{
	struct device *dev = &client->dev;
	struct ov13855 *ov13855;
	struct amlsens *sensor = (struct amlsens *)sdrv;
	int ret = -EINVAL;

	ov13855 = devm_kzalloc(dev, sizeof(*ov13855), GFP_KERNEL);
	if (!ov13855)
		return -ENOMEM;

	ov13855->dev = dev;
	ov13855->client = client;
	ov13855->client->addr = OV13855_SLAVE_ID;
	ov13855->gpio = &sensor->gpio;

	ov13855->regmap = devm_regmap_init_i2c(client, &ov13855_regmap_config);
	if (IS_ERR(ov13855->regmap)) {
		dev_err(dev, "Unable to initialize I2C\n");
		return -ENODEV;
	}

	/*index = 1*/
	if (of_property_read_u32(dev->of_node, "index", &ov13855->index)) {
		dev_err(dev, "Failed to read sensor index. default to 0\n");
		ov13855->index = 0;
	}

	mutex_init(&ov13855->lock);

	/* Power on the device to match runtime PM state below */
	ret = ov13855_power_on(ov13855->dev, ov13855->gpio);
	if (ret < 0) {
		dev_err(dev, "Could not power on the device\n");
		return -ENODEV;
	}

	/*
	 * Initialize the frame format. In particular, ov13855->current_mode
	 * and ov13855->bpp are set to defaults: ov13855_calc_pixel_rate() call
	 * below in ov13855_ctrls_init relies on these fields.
	 */
	ov13855_entity_init_cfg(&ov13855->sd, NULL);

	ret = ov13855_ctrls_init(ov13855);
	if (ret) {
		dev_err(ov13855->dev, "Error ctrls init\n");
		goto free_ctrl;
	}

	ret = ov13855_register_subdev(ov13855);
	if (ret) {
		dev_err(ov13855->dev, "Error register subdev\n");
		goto free_entity;
	}

	dev_err(ov13855->dev, "ov13855 probe done \n");

	return ret;

free_entity:
	media_entity_cleanup(&ov13855->sd.entity);
free_ctrl:
	v4l2_ctrl_handler_free(&ov13855->ctrls);
	mutex_destroy(&ov13855->lock);

	return ret;
}

int ov13855_deinit(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ov13855 *ov13855 = to_ov13855(sd);

	v4l2_async_unregister_subdev(sd);
	media_entity_cleanup(&sd->entity);
	v4l2_ctrl_handler_free(sd->ctrl_handler);

	mutex_destroy(&ov13855->lock);

	ov13855_power_off(ov13855->dev, ov13855->gpio);

	return 0;
}

int ov13855_sensor_id(struct i2c_client *client)
{
	u32 id = 0;
	u8 val = 0;
	int rtn = -EINVAL;

	i2c_read_a16d8(client, OV13855_SLAVE_ID, 0x300a, &val);
	id |= (val << 16);
	i2c_read_a16d8(client, OV13855_SLAVE_ID, 0x300b, &val);
	id |= (val << 8);
	i2c_read_a16d8(client, OV13855_SLAVE_ID, 0x300c, &val);
	id |= val;

	if (id != OV13855_ID) {
		dev_info(&client->dev, "Failed to get ov13855 id: 0x%x\n", id);
		return rtn;
	} else {
		dev_err(&client->dev, "success get ov13855 id 0x%x", id);
	}

	return 0;
}

