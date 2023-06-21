// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2023 Amlogic, Inc. All rights reserved.
 */

#include "sensor_drv.h"
#include "i2c_api.h"
#include "mclk_api.h"
#include "aml_imx290.h"

#define AML_SENSOR_NAME "imx290-%u"

/* supported link frequencies */
#define FREQ_INDEX_1080P 0
#define FREQ_INDEX_720P 1
#define FREQ_INDEX_1080P_60HZ 2

/* supported link frequencies */
static const s64 imx290_link_freq_2lanes[] = {
	[FREQ_INDEX_1080P] = 445500000,
	[FREQ_INDEX_720P] = 297000000,
};

static const s64 imx290_link_freq_4lanes[] = {
	[FREQ_INDEX_1080P] = 222750000,
	[FREQ_INDEX_720P] = 148500000,
	[FREQ_INDEX_1080P_60HZ] = 445500000,
};

static inline const s64 *imx290_link_freqs_ptr(const struct imx290 *imx290)
{
	if (imx290->nlanes == 2)
		return imx290_link_freq_2lanes;
	else
	{
		return imx290_link_freq_4lanes;
	}
}

static inline int imx290_link_freqs_num(const struct imx290 *imx290)
{
	if (imx290->nlanes == 2)
		return ARRAY_SIZE(imx290_link_freq_2lanes);
	else
		return ARRAY_SIZE(imx290_link_freq_4lanes);
}

/* Mode configs */
static const struct imx290_mode imx290_modes_2lanes[] = {
	{
		.width = 1920,
		.height = 1080,
		.hmax = 0x1130,
		.data = imx290_1080p_settings,
		.data_size = ARRAY_SIZE(imx290_1080p_settings),

		.link_freq_index = FREQ_INDEX_1080P,
	},
	{
		.width = 1280,
		.height = 720,
		.hmax = 0x19c8,
		.data = imx290_720p_settings,
		.data_size = ARRAY_SIZE(imx290_720p_settings),

		.link_freq_index = FREQ_INDEX_720P,
	},
};

static const struct imx290_mode imx290_modes_4lanes[] = {
	{
		.width = 1920,
		.height = 1080,
		.hmax = 0x0898,
		.link_freq_index = FREQ_INDEX_1080P,
		.data = imx290_1080p_settings,
		.data_size = ARRAY_SIZE(imx290_1080p_settings),
	},
	{
		.width = 1280,
		.height = 720,
		.hmax = 0x0ce4,
		.link_freq_index = FREQ_INDEX_720P,
		.data = imx290_720p_settings,
		.data_size = ARRAY_SIZE(imx290_720p_settings),
	},
	{
		.width = 1920,
		.height = 1080,
		.hmax = 0x1130,
		.link_freq_index = FREQ_INDEX_1080P_60HZ,
		.data = imx290_1080p_settings,
		.data_size = ARRAY_SIZE(imx290_1080p_settings),
	},
};

static inline const struct imx290_mode *imx290_modes_ptr(const struct imx290 *imx290)
{
	if (imx290->nlanes == 2)
		return imx290_modes_2lanes;
	else
		return imx290_modes_4lanes;
}

static inline int imx290_modes_num(const struct imx290 *imx290)
{
	if (imx290->nlanes == 2)
		return ARRAY_SIZE(imx290_modes_2lanes);
	else
		return ARRAY_SIZE(imx290_modes_4lanes);
}

static inline struct imx290 *to_imx290(struct v4l2_subdev *_sd)
{
	return container_of(_sd, struct imx290, sd);
}

static inline int imx290_read_reg(struct imx290 *imx290, u16 addr, u8 *value)
{
	unsigned int regval;

	int i, ret;

	for (i = 0; i < 3; ++i) {
		ret = regmap_read(imx290->regmap, addr, &regval);
		if (0 == ret) {
			break;
		}
	}

	if (ret)
		dev_err(imx290->dev, "I2C read with i2c transfer failed for addr: %x, ret %d\n", addr, ret);

	*value = regval & 0xff;
	return 0;
}

static int imx290_write_reg(struct imx290 *imx290, u16 addr, u8 value)
{
	int i, ret;

	for (i = 0; i < 3; i++) {
		ret = regmap_write(imx290->regmap, addr, value);
		if (0 == ret)
		{
			break;
		}
	}

	if (ret)
		dev_err(imx290->dev, "I2C write failed for addr: %x, ret %d\n", addr, ret);

	return ret;
}

static int imx290_set_register_array(struct imx290 *imx290,
									 const struct imx290_regval *settings,
									 unsigned int num_settings)
{
	unsigned int i;
	int ret = 0;

	for (i = 0; i < num_settings; ++i, ++settings) {
		ret = imx290_write_reg(imx290, settings->reg, settings->val);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int imx290_write_buffered_reg(struct imx290 *imx290, u16 address_low,
									 u8 nr_regs, u32 value)
{
	unsigned int i;
	int ret;

	ret = imx290_write_reg(imx290, IMX290_REGHOLD, 0x01);
	if (ret) {
		dev_err(imx290->dev, "Error setting hold register\n");
		return ret;
	}

	for (i = 0; i < nr_regs; i++) {
		ret = imx290_write_reg(imx290, address_low + i,
							   (u8)(value >> (i * 8)));
		if (ret)
		{
			dev_err(imx290->dev, "Error writing buffered registers\n");
			return ret;
		}
	}

	ret = imx290_write_reg(imx290, IMX290_REGHOLD, 0x00);
	if (ret) {
		dev_err(imx290->dev, "Error setting hold register\n");
		return ret;
	}

	return ret;
}

static int imx290_set_gain(struct imx290 *imx290, u32 value)
{
	int ret;

	ret = imx290_write_buffered_reg(imx290, IMX290_GAIN, 1, value);
	if (ret)
		dev_err(imx290->dev, "Unable to write gain\n");

	return ret;
}

static int imx290_set_exposure(struct imx290 *imx290, u32 value)
{
	int ret;

	ret = imx290_write_buffered_reg(imx290, IMX290_EXPOSURE, 2, value & 0xFFFF);
	if (ret)
		dev_err(imx290->dev, "Unable to write gain\n");

	if (imx290->enWDRMode)
		ret = imx290_write_buffered_reg(imx290, 0x3024, 2, (value >> 16) & 0xFFFF);

	return ret;
}

static int imx290_set_fps(struct imx290 *imx290, u32 value)
{
	u32 vts = 0;
	u8 vts_h, vts_l;

	//dev_err(imx290->dev, "-imx290-value = %d\n", value);

	vts = 30 * 1125 / value;
	vts_h = (vts >> 8) & 0x7f;
	vts_l = vts & 0xff;

	imx290_write_reg(imx290, 0x3019, vts_h);
	imx290_write_reg(imx290, 0x3018, vts_l);

	return 0;
}

/* Stop streaming */
static int imx290_stop_streaming(struct imx290 *imx290)
{
	int ret;
	imx290->enWDRMode = WDR_MODE_NONE;

	ret = imx290_write_reg(imx290, IMX290_STANDBY, 0x01);
	if (ret < 0)
		return ret;

	msleep(30);

	return imx290_write_reg(imx290, IMX290_XMSTA, 0x01);
}

static int imx290_set_ctrl(struct v4l2_ctrl *ctrl)
{
	struct imx290 *imx290 = container_of(ctrl->handler,
										 struct imx290, ctrls);
	int ret = 0;

	/* V4L2 controls values will be applied only when power is already up */
	if (!pm_runtime_get_if_in_use(imx290->dev))
		return 0;

	switch (ctrl->id) {
	case V4L2_CID_GAIN:
		ret = imx290_set_gain(imx290, ctrl->val);
		break;
	case V4L2_CID_EXPOSURE:
		ret = imx290_set_exposure(imx290, ctrl->val);
		break;
	case V4L2_CID_HBLANK:
		break;
	case V4L2_CID_PIXEL_RATE:
		break;
	case V4L2_CID_AML_CSI_LANES:
		break;
	case V4L2_CID_AML_MODE:
		imx290->enWDRMode = ctrl->val;
		break;
	case V4L2_CID_AML_ORIG_FPS:
		ret = imx290_set_fps(imx290, ctrl->val);
		if (ctrl->val == 60) {
			imx290->flag_60hz = 1;
		} else {
			imx290->flag_60hz = 0;
		}
		break;
	default:
		dev_err(imx290->dev, "Error ctrl->id %u, flag 0x%lx\n",
				ctrl->id, ctrl->flags);
		ret = -EINVAL;
		break;
	}

	pm_runtime_put(imx290->dev);

	return ret;
}

static const struct v4l2_ctrl_ops imx290_ctrl_ops = {
	.s_ctrl = imx290_set_ctrl,
};

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int imx290_enum_mbus_code(struct v4l2_subdev *sd,
								 struct v4l2_subdev_state *cfg,
								 struct v4l2_subdev_mbus_code_enum *code)
#else
static int imx290_enum_mbus_code(struct v4l2_subdev *sd,
								 struct v4l2_subdev_pad_config *cfg,
								 struct v4l2_subdev_mbus_code_enum *code)
#endif
{
	if (code->index >= ARRAY_SIZE(imx290_formats))
		return -EINVAL;

	code->code = imx290_formats[code->index].code;

	return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int imx290_enum_frame_size(struct v4l2_subdev *sd,
								  struct v4l2_subdev_state *cfg,
								  struct v4l2_subdev_frame_size_enum *fse)
#else
static int imx290_enum_frame_size(struct v4l2_subdev *sd,
								  struct v4l2_subdev_pad_config *cfg,
								  struct v4l2_subdev_frame_size_enum *fse)
#endif
{
	if (fse->index >= ARRAY_SIZE(imx290_formats))
		return -EINVAL;

	fse->min_width = imx290_formats[fse->index].min_width;
	fse->min_height = imx290_formats[fse->index].min_height;

	fse->max_width = imx290_formats[fse->index].max_width;
	fse->max_height = imx290_formats[fse->index].max_height;

	return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int imx290_get_fmt(struct v4l2_subdev *sd,
						  struct v4l2_subdev_state *cfg,
						  struct v4l2_subdev_format *fmt)
#else
static int imx290_get_fmt(struct v4l2_subdev *sd,
						  struct v4l2_subdev_pad_config *cfg,
						  struct v4l2_subdev_format *fmt)
#endif
{
	struct imx290 *imx290 = to_imx290(sd);
	struct v4l2_mbus_framefmt *framefmt;

	mutex_lock(&imx290->lock);

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY)
		framefmt = v4l2_subdev_get_try_format(&imx290->sd, cfg,
											  fmt->pad);
	else
		framefmt = &imx290->current_format;

	fmt->format = *framefmt;

	mutex_unlock(&imx290->lock);

	return 0;
}

static inline u8 imx290_get_link_freq_index(struct imx290 *imx290)
{
	return imx290->current_mode->link_freq_index;
}

static s64 imx290_get_link_freq(struct imx290 *imx290)
{
	u8 index = imx290_get_link_freq_index(imx290);

	return *(imx290_link_freqs_ptr(imx290) + index);
}

static u64 imx290_calc_pixel_rate(struct imx290 *imx290)
{
	s64 link_freq = imx290_get_link_freq(imx290);
	u8 nlanes = imx290->nlanes;
	u64 pixel_rate;

	/* pixel rate = link_freq * 2 * nr_of_lanes / bits_per_sample */
	pixel_rate = link_freq * 2 * nlanes;
	do_div(pixel_rate, imx290->bpp);
	return pixel_rate;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int imx290_set_fmt(struct v4l2_subdev *sd,
						  struct v4l2_subdev_state *cfg,
						  struct v4l2_subdev_format *fmt)
#else
static int imx290_set_fmt(struct v4l2_subdev *sd,
						  struct v4l2_subdev_pad_config *cfg,
						  struct v4l2_subdev_format *fmt)
#endif
{
	struct imx290 *imx290 = to_imx290(sd);
	const struct imx290_mode *mode;
	struct v4l2_mbus_framefmt *format;
	unsigned int i, ret;

	mutex_lock(&imx290->lock);
	if (imx290->flag_60hz == 1) {
		mode = &imx290_modes_4lanes[2];
	} else {
		mode = v4l2_find_nearest_size(imx290_modes_ptr(imx290),
									  imx290_modes_num(imx290),
									  width, height,
									  fmt->format.width, fmt->format.height);
	}

	fmt->format.width = mode->width;
	fmt->format.height = mode->height;

	for (i = 0; i < ARRAY_SIZE(imx290_formats); i++) {
		if (imx290_formats[i].code == fmt->format.code) {
			dev_info(imx290->dev, "find proper format %d \n", i);
			break;
		}
	}

	if (i >= ARRAY_SIZE(imx290_formats)) {
		i = 0;
		dev_err(imx290->dev, "No format. reset i = 0 \n");
	}

	fmt->format.code = imx290_formats[i].code;
	fmt->format.field = V4L2_FIELD_NONE;

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		dev_info(imx290->dev, "try format \n");
		format = v4l2_subdev_get_try_format(sd, cfg, fmt->pad);
		mutex_unlock(&imx290->lock);
		return 0;
	} else {
		dev_err(imx290->dev, "set format, w %d, h %d, code 0x%x \n",
				fmt->format.width, fmt->format.height,
				fmt->format.code);
		format = &imx290->current_format;
		imx290->current_mode = mode;
		imx290->bpp = imx290_formats[i].bpp;
		imx290->nlanes = 4;

		if (imx290->link_freq)
			__v4l2_ctrl_s_ctrl(imx290->link_freq, imx290_get_link_freq_index(imx290));
		if (imx290->pixel_rate)
			__v4l2_ctrl_s_ctrl_int64(imx290->pixel_rate, imx290_calc_pixel_rate(imx290));
		if (imx290->data_lanes)
			__v4l2_ctrl_s_ctrl(imx290->data_lanes, imx290->nlanes);
	}

	*format = fmt->format;
	imx290->status = 0;

	mutex_unlock(&imx290->lock);
	if (imx290->enWDRMode) {
		/* Set init register settings */
		ret = imx290_set_register_array(imx290, dol_1080p_30fps_4lane_10bits,
										ARRAY_SIZE(dol_1080p_30fps_4lane_10bits));
		if (ret < 0) {
			dev_err(imx290->dev, "Could not set init registers\n");
			return ret;
		} else
			dev_err(imx290->dev, "imx290 wdr mode init...\n");
	} else {
		/* Set init register settings */
		if (imx290->flag_60hz) {
			ret = imx290_set_register_array(imx290, imx290_global_init_settings_60hz,
											ARRAY_SIZE(imx290_global_init_settings_60hz));
		} else {
			ret = imx290_set_register_array(imx290, imx290_global_init_settings,
											ARRAY_SIZE(imx290_global_init_settings));
		}

		if (ret < 0) {
			dev_err(imx290->dev, "Could not set init registers\n");
			return ret;
		} else
			dev_err(imx290->dev, "imx290 linear mode init...\n");
	}

	return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
int imx290_get_selection(struct v4l2_subdev *sd,
						 struct v4l2_subdev_state *cfg,
						 struct v4l2_subdev_selection *sel)
#else
int imx290_get_selection(struct v4l2_subdev *sd,
						 struct v4l2_subdev_pad_config *cfg,
						 struct v4l2_subdev_selection *sel)
#endif
{
	int rtn = 0;
	struct imx290 *imx290 = to_imx290(sd);
	const struct imx290_mode *mode = imx290->current_mode;

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
		dev_err(imx290->dev, "Error support target: 0x%x\n", sel->target);
		break;
	}

	return rtn;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int imx290_entity_init_cfg(struct v4l2_subdev *subdev,
								  struct v4l2_subdev_state *cfg)
#else
static int imx290_entity_init_cfg(struct v4l2_subdev *subdev,
								  struct v4l2_subdev_pad_config *cfg)
#endif
{
	struct v4l2_subdev_format fmt = {0};

	fmt.which = cfg ? V4L2_SUBDEV_FORMAT_TRY : V4L2_SUBDEV_FORMAT_ACTIVE;
	fmt.format.width = 1920;
	fmt.format.height = 1080;
	fmt.format.code = MEDIA_BUS_FMT_SRGGB12_1X12;

	imx290_set_fmt(subdev, cfg, &fmt);

	return 0;
}

/* Start streaming */
static int imx290_start_streaming(struct imx290 *imx290)
{
	int ret = 0;

	ret = imx290_write_reg(imx290, IMX290_STANDBY, 0x00);
	if (ret < 0)
		return ret;

	msleep(30);

	/* Start streaming */
	return imx290_write_reg(imx290, IMX290_XMSTA, 0x00);
}

static int imx290_set_stream(struct v4l2_subdev *sd, int enable)
{
	struct imx290 *imx290 = to_imx290(sd);
	int ret = 0;

	if (imx290->status == enable)
		return ret;
	else
		imx290->status = enable;

	if (enable) {
		ret = imx290_start_streaming(imx290);
		if (ret) {
			dev_err(imx290->dev, "Start stream failed\n");
			goto unlock_and_return;
		}

		dev_info(imx290->dev, "stream on\n");
	} else {
		imx290_stop_streaming(imx290);

		dev_info(imx290->dev, "stream off\n");
	}

unlock_and_return:

	return ret;
}

int imx290_power_on(struct device *dev, struct sensor_gpio *gpio)
{
	int ret;

	gpiod_set_value_cansleep(gpio->rst_gpio, 1);
	if (!IS_ERR_OR_NULL(gpio->pwdn_gpio)) {
		gpiod_set_value_cansleep(gpio->pwdn_gpio, 1);
	}

	ret = mclk_enable(dev, 37125000);
	if (ret < 0)
		dev_err(dev, "set mclk fail\n");

	// 30ms
	usleep_range(30000, 31000);

	return 0;
}

int imx290_power_off(struct device *dev, struct sensor_gpio *gpio)
{
	mclk_disable(dev);

	gpiod_set_value_cansleep(gpio->rst_gpio, 0);
	if (!IS_ERR_OR_NULL(gpio->pwdn_gpio)) {
		gpiod_set_value_cansleep(gpio->pwdn_gpio, 0);
	}
	return 0;
}

int imx290_power_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct imx290 *imx290 = to_imx290(sd);

	gpiod_set_value_cansleep(imx290->gpio->rst_gpio, 0);

	return 0;
}

int imx290_power_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct imx290 *imx290 = to_imx290(sd);

	gpiod_set_value_cansleep(imx290->gpio->rst_gpio, 1);

	return 0;
}

static int imx290_log_status(struct v4l2_subdev *sd)
{
	struct imx290 *imx290 = to_imx290(sd);

	dev_info(imx290->dev, "log status done\n");

	return 0;
}

int imx290_sbdev_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct imx290 *imx290 = to_imx290(sd);
	imx290_power_on(imx290->dev, imx290->gpio);
	return 0;
}
int imx290_sbdev_close(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct imx290 *imx290 = to_imx290(sd);
	imx290_set_stream(sd, 0);
	imx290_power_off(imx290->dev, imx290->gpio);
	return 0;
}

static const struct dev_pm_ops imx290_pm_ops = {
	SET_RUNTIME_PM_OPS(imx290_power_suspend, imx290_power_resume, NULL)};

const struct v4l2_subdev_core_ops imx290_core_ops = {
	.log_status = imx290_log_status,
};

static const struct v4l2_subdev_video_ops imx290_video_ops = {
	.s_stream = imx290_set_stream,
};

static const struct v4l2_subdev_pad_ops imx290_pad_ops = {
	.init_cfg = imx290_entity_init_cfg,
	.enum_mbus_code = imx290_enum_mbus_code,
	.enum_frame_size = imx290_enum_frame_size,
	.get_selection = imx290_get_selection,
	.get_fmt = imx290_get_fmt,
	.set_fmt = imx290_set_fmt,
};

static struct v4l2_subdev_internal_ops imx290_internal_ops = {
	.open = imx290_sbdev_open,
	.close = imx290_sbdev_close,
};

static const struct v4l2_subdev_ops imx290_subdev_ops = {
	.core = &imx290_core_ops,
	.video = &imx290_video_ops,
	.pad = &imx290_pad_ops,
};

static const struct media_entity_operations imx290_subdev_entity_ops = {
	.link_validate = v4l2_subdev_link_validate,
};

static struct v4l2_ctrl_config wdr_cfg = {
	.ops = &imx290_ctrl_ops,
	.id = V4L2_CID_AML_MODE,
	.name = "wdr mode",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	.min = 0,
	.max = 2,
	.step = 1,
	.def = 0,
};

static struct v4l2_ctrl_config fps_cfg = {
	.ops = &imx290_ctrl_ops,
	.id = V4L2_CID_AML_ORIG_FPS,
	.name = "sensor fps",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	.min = 1,
	.max = 60,
	.step = 1,
	.def = 30,
};

static struct v4l2_ctrl_config nlane_cfg = {
	.ops = &imx290_ctrl_ops,
	.id = V4L2_CID_AML_CSI_LANES,
	.name = "sensor lanes",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.flags = V4L2_CTRL_FLAG_VOLATILE,
	.min = 1,
	.max = 4,
	.step = 1,
	.def = 4,
};

int imx290_ctrls_init(struct imx290 *imx290)
{
	int rtn = 0;

	v4l2_ctrl_handler_init(&imx290->ctrls, 7);

	v4l2_ctrl_new_std(&imx290->ctrls, &imx290_ctrl_ops,
					  V4L2_CID_GAIN, 0, 0xF0, 1, 0);

	v4l2_ctrl_new_std(&imx290->ctrls, &imx290_ctrl_ops,
					  V4L2_CID_EXPOSURE, 0, 0x7fffffff, 1, 0);

	imx290->link_freq = v4l2_ctrl_new_int_menu(&imx290->ctrls,
											   &imx290_ctrl_ops,
											   V4L2_CID_LINK_FREQ,
											   imx290_link_freqs_num(imx290) - 1,
											   0, imx290_link_freqs_ptr(imx290));

	if (imx290->link_freq)
		imx290->link_freq->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	imx290->pixel_rate = v4l2_ctrl_new_std(&imx290->ctrls,
										   &imx290_ctrl_ops,
										   V4L2_CID_PIXEL_RATE,
										   1, INT_MAX, 1,
										   imx290_calc_pixel_rate(imx290));

	imx290->data_lanes = v4l2_ctrl_new_custom(&imx290->ctrls, &nlane_cfg, NULL);
	if (imx290->data_lanes)
		imx290->data_lanes->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	imx290->wdr = v4l2_ctrl_new_custom(&imx290->ctrls, &wdr_cfg, NULL);
	v4l2_ctrl_new_custom(&imx290->ctrls, &fps_cfg, NULL);

	imx290->sd.ctrl_handler = &imx290->ctrls;

	if (imx290->ctrls.error) {
		dev_err(imx290->dev, "Control initialization a error  %d\n",
				imx290->ctrls.error);
		rtn = imx290->ctrls.error;
	}

	return rtn;
}

int imx290_register_subdev(void *sensor)
{
	int rtn = 0;
	struct imx290 *imx290 = (struct imx290 *)sensor;

	v4l2_i2c_subdev_init(&imx290->sd, imx290->client, &imx290_subdev_ops);

	imx290->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	imx290->sd.dev = &imx290->client->dev;
	imx290->sd.internal_ops = &imx290_internal_ops;
	imx290->sd.entity.ops = &imx290_subdev_entity_ops;
	imx290->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;
	snprintf(imx290->sd.name, sizeof(imx290->sd.name), AML_SENSOR_NAME, imx290->index);

	imx290->pad.flags = MEDIA_PAD_FL_SOURCE;
	rtn = media_entity_pads_init(&imx290->sd.entity, 1, &imx290->pad);
	if (rtn < 0) {
		dev_err(imx290->dev, "Could not register media entity\n");
		goto err_return;
	}

	rtn = v4l2_async_register_subdev(&imx290->sd);
	if (rtn < 0) {
		dev_err(imx290->dev, "Could not register v4l2 device\n");
		goto err_return;
	}

err_return:
	return rtn;
}

int imx290_init(struct i2c_client *client, void *sdrv)
{
	struct device *dev = &client->dev;
	struct imx290 *imx290;
	struct amlsens *sensor = (struct amlsens *)sdrv;
	int ret = -EINVAL;

	imx290 = devm_kzalloc(dev, sizeof(*imx290), GFP_KERNEL);
	if (!imx290)
		return -ENOMEM;

	imx290->dev = dev;
	imx290->client = client;
	imx290->client->addr = IMX290_SLAVE_ID;
	imx290->gpio = &sensor->gpio;

	imx290->regmap = devm_regmap_init_i2c(client, &imx290_regmap_config);
	if (IS_ERR(imx290->regmap)) {
		dev_err(dev, "Unable to initialize I2C\n");
		return -ENODEV;
	}

	if (of_property_read_u32(dev->of_node, "index", &imx290->index)) {
		dev_err(dev, "Failed to read sensor index. default to 0\n");
		imx290->index = 0;
	}

	mutex_init(&imx290->lock);

	/* Power on the device to match runtime PM state below */
	ret = imx290_power_on(dev, imx290->gpio);
	if (ret < 0) {
		dev_err(dev, "Could not power on the device\n");
		return -ENODEV;
	}

	/*
	 * Initialize the frame format. In particular, imx290->current_mode
	 * and imx290->bpp are set to defaults: imx290_calc_pixel_rate() call
	 * below in imx290_ctrls_init relies on these fields.
	 */
	imx290_entity_init_cfg(&imx290->sd, NULL);

	ret = imx290_ctrls_init(imx290);
	if (ret) {
		dev_err(imx290->dev, "Error ctrls init\n");
		goto free_ctrl;
	}

	ret = imx290_register_subdev(imx290);
	if (ret) {
		dev_err(imx290->dev, "Error register subdev\n");
		goto free_entity;
	}

	dev_err(imx290->dev, "probe imx290 done\n");

	return 0;

free_entity:
	media_entity_cleanup(&imx290->sd.entity);
free_ctrl:
	v4l2_ctrl_handler_free(&imx290->ctrls);
	mutex_destroy(&imx290->lock);

	return ret;
}

int imx290_deinit(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct imx290 *imx290 = to_imx290(sd);

	v4l2_async_unregister_subdev(sd);
	media_entity_cleanup(&sd->entity);
	v4l2_ctrl_handler_free(sd->ctrl_handler);

	mutex_destroy(&imx290->lock);

	imx290_power_off(imx290->dev, imx290->gpio);

	return 0;
}

int imx290_sensor_id(struct i2c_client *client)
{
	int rtn = -EINVAL;
	u32 id = 0;
	u8 val = 0;

	i2c_read_a16d8(client, IMX290_SLAVE_ID, 0x348F, &val);
	id |= (val << 8);

	i2c_read_a16d8(client, IMX290_SLAVE_ID, 0x348E, &val);
	id |= val;
	if (id == IMX290_ID) {
		dev_err(&client->dev, "success get imx290 id 0x%x", id);
		return 0;
	}

	id = 0;
	val = 0;
	i2c_read_a16d8(client, IMX290_SLAVE_ID, 0x301e, &val);
	id |= (val << 8);

	i2c_read_a16d8(client, IMX290_SLAVE_ID, 0x301f, &val);
	id |= val;

	if (id != IMX290_SUB_ID) {
		dev_info(&client->dev, "Failed to get imx307 id: 0x%x\n", id);
		return rtn;
	} else {
		dev_err(&client->dev, "success get imx307 id 0x%x", id);
	}

	return 0;
}

