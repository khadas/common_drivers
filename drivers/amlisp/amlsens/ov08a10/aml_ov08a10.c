// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2023 Amlogic, Inc. All rights reserved.
 */

#include "sensor_drv.h"
#include "i2c_api.h"
#include "mclk_api.h"
#include "aml_ov08a10.h"

#define AML_SENSOR_NAME  "ov08a10-%u"

/* supported link frequencies */
#define FREQ_INDEX_1080P	0
#define FREQ_INDEX_720P		1

/* supported link frequencies */

#if defined(OV08A10_HDR_30FPS_1440M) || defined(OV08A10_SDR_60FPS_1440M)
static const s64 ov08a10_link_freq_2lanes[] = {
	[FREQ_INDEX_1080P] = 1440000000,
	[FREQ_INDEX_720P] = 1440000000,
};

static const s64 ov08a10_link_freq_4lanes[] = {
	[FREQ_INDEX_1080P] = 1440000000,
	[FREQ_INDEX_720P] = 1440000000,
};

#else
static const s64 ov08a10_link_freq_2lanes[] = {
	[FREQ_INDEX_1080P] = 848000000,
	[FREQ_INDEX_720P] = 848000000,
};

static const s64 ov08a10_link_freq_4lanes[] = {
	[FREQ_INDEX_1080P] = 848000000,
	[FREQ_INDEX_720P] = 848000000,
};
#endif

static inline const s64 *ov08a10_link_freqs_ptr(const struct ov08a10 *ov08a10)
{
	if (ov08a10->nlanes == 2)
		return ov08a10_link_freq_2lanes;
	else {
		return ov08a10_link_freq_4lanes;
	}
}

static inline int ov08a10_link_freqs_num(const struct ov08a10 *ov08a10)
{
	if (ov08a10->nlanes == 2)
		return ARRAY_SIZE(ov08a10_link_freq_2lanes);
	else
		return ARRAY_SIZE(ov08a10_link_freq_4lanes);
}

/* Mode configs */
static const struct ov08a10_mode ov08a10_modes_2lanes[] = {
	{
		.width = 1920,
		.height = 1080,
		.hmax  = 0x1130,
		.data = ov08a10_1080p_settings,
		.data_size = ARRAY_SIZE(ov08a10_1080p_settings),

		.link_freq_index = FREQ_INDEX_1080P,
	},
	{
		.width = 1280,
		.height = 720,
		.hmax = 0x19c8,
		.data = ov08a10_720p_settings,
		.data_size = ARRAY_SIZE(ov08a10_720p_settings),

		.link_freq_index = FREQ_INDEX_720P,
	},
};

static const struct ov08a10_mode ov08a10_modes_4lanes[] = {
	{
		.width = 3840,
		.height = 2160,
		.hmax = 0x0898,
		.link_freq_index = FREQ_INDEX_1080P,
		.data = ov08a10_1080p_settings,
		.data_size = ARRAY_SIZE(ov08a10_1080p_settings),
	},
	{
		.width = 3840,
		.height = 2160,
		.hmax = 0x0ce4,
		.link_freq_index = FREQ_INDEX_720P,
		.data = ov08a10_720p_settings,
		.data_size = ARRAY_SIZE(ov08a10_720p_settings),
	},
};

static inline const struct ov08a10_mode *ov08a10_modes_ptr(const struct ov08a10 *ov08a10)
{
	if (ov08a10->nlanes == 2)
		return ov08a10_modes_2lanes;
	else
		return ov08a10_modes_4lanes;
}

static inline int ov08a10_modes_num(const struct ov08a10 *ov08a10)
{
	if (ov08a10->nlanes == 2)
		return ARRAY_SIZE(ov08a10_modes_2lanes);
	else
		return ARRAY_SIZE(ov08a10_modes_4lanes);
}

static inline struct ov08a10 *to_ov08a10(struct v4l2_subdev *_sd)
{
	return container_of(_sd, struct ov08a10, sd);
}

static inline int ov08a10_read_reg(struct ov08a10 *ov08a10, u16 addr, u8 *value)
{
	unsigned int regval;

	int i, ret;

	for (i = 0; i < 3; ++i) {
		ret = regmap_read(ov08a10->regmap, addr, &regval);
		if (0 == ret ) {
			break;
		}
	}

	if (ret)
		dev_err(ov08a10->dev, "I2C read with i2c transfer failed for addr: %x, ret %d\n", addr, ret);

	*value = regval & 0xff;
	return 0;
}

static int ov08a10_write_reg(struct ov08a10 *ov08a10, u16 addr, u8 value)
{
	int i, ret;

	for (i = 0; i < 3; i++) {
		ret = regmap_write(ov08a10->regmap, addr, value);
		if (0 == ret) {
			break;
		}
	}

	if (ret)
		dev_err(ov08a10->dev, "I2C write failed for addr: %x, ret %d\n", addr, ret);

	return ret;
}

static int ov08a10_set_register_array(struct ov08a10 *ov08a10,
				     const struct ov08a10_regval *settings,
				     unsigned int num_settings)
{
	unsigned int i;
	int ret = 0;

	for (i = 0; i < num_settings; ++i, ++settings) {
		ret = ov08a10_write_reg(ov08a10, settings->reg, settings->val);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int ov08a10_set_gain(struct ov08a10 *ov08a10, u32 value)
{
	int ret;
	ret = ov08a10_write_reg(ov08a10, OV08A10_GAIN, (value >> 8) & 0xFF);
	if (ret)
		dev_err(ov08a10->dev, "Unable to write gain_H\n");
	ret = ov08a10_write_reg(ov08a10, OV08A10_GAIN_L, value & 0xFF);
	if (ret)
		dev_err(ov08a10->dev, "Unable to write gain_L\n");

	return ret;
}

static int ov08a10_set_exposure(struct ov08a10 *ov08a10, u32 value)
{
	int ret;
	ret = ov08a10_write_reg(ov08a10, OV08A10_EXPOSURE, (value >> 8) & 0xFF);
	if (ret)
		dev_err(ov08a10->dev, "Unable to write gain_H\n");
	ret = ov08a10_write_reg(ov08a10, OV08A10_EXPOSURE_L, value & 0xFF);
	if (ret)
		dev_err(ov08a10->dev, "Unable to write gain_L\n");
	return ret;
}

static int ov08a10_set_fps(struct ov08a10 *ov08a10, u32 value)
{
	u32 vts = 0;
	u8 vts_h, vts_l;

	dev_info(ov08a10->dev, "ov08a10_set_fps = %d \n", value);

	vts = 30 * 2314 / value;
	vts_h = (vts >> 8) & 0x7f;
	vts_l = vts & 0xff;

	ov08a10_write_reg(ov08a10, 0x380e, vts_h);
	ov08a10_write_reg(ov08a10, 0x380f, vts_l);

	return 0;
}

/* Stop streaming */
static int ov08a10_stop_streaming(struct ov08a10 *ov08a10)
{
	ov08a10->enWDRMode = WDR_MODE_NONE;
	return ov08a10_write_reg(ov08a10, 0x0100, 0x00);
}

static int ov08a10_set_ctrl(struct v4l2_ctrl *ctrl)
{
	struct ov08a10 *ov08a10 = container_of(ctrl->handler,
					     struct ov08a10, ctrls);
	int ret = 0;

	/* V4L2 controls values will be applied only when power is already up */
	if (!pm_runtime_get_if_in_use(ov08a10->dev))
		return 0;

	switch (ctrl->id) {
	case V4L2_CID_GAIN:
		ret = ov08a10_set_gain(ov08a10, ctrl->val);
		break;
	case V4L2_CID_EXPOSURE:
		ret = ov08a10_set_exposure(ov08a10, ctrl->val);
		break;
	case V4L2_CID_HBLANK:
		break;
	case V4L2_CID_AML_CSI_LANES:
		break;
	case V4L2_CID_AML_MODE:
		ov08a10->enWDRMode = ctrl->val;
		break;
	case V4L2_CID_AML_ORIG_FPS:
		ret = ov08a10_set_fps(ov08a10, ctrl->val);
		break;
	default:
		dev_err(ov08a10->dev, "Error ctrl->id %u, flag 0x%lx\n",
			ctrl->id, ctrl->flags);
		ret = -EINVAL;
		break;
	}

	pm_runtime_put(ov08a10->dev);

	return ret;
}

static const struct v4l2_ctrl_ops ov08a10_ctrl_ops = {
	.s_ctrl = ov08a10_set_ctrl,
};
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int ov08a10_enum_mbus_code(struct v4l2_subdev *sd,
				  struct v4l2_subdev_state *cfg,
				 struct v4l2_subdev_mbus_code_enum *code)
#else
static int ov08a10_enum_mbus_code(struct v4l2_subdev *sd,
				 struct v4l2_subdev_pad_config *cfg,
				 struct v4l2_subdev_mbus_code_enum *code)
#endif
{
	if (code->index >= ARRAY_SIZE(ov08a10_formats))
		return -EINVAL;

	code->code = ov08a10_formats[code->index].code;

	return 0;
}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int ov08a10_enum_frame_size(struct v4l2_subdev *sd,
			        struct v4l2_subdev_state *cfg,
			       struct v4l2_subdev_frame_size_enum *fse)
#else
static int ov08a10_enum_frame_size(struct v4l2_subdev *sd,
			       struct v4l2_subdev_pad_config *cfg,
			       struct v4l2_subdev_frame_size_enum *fse)
#endif
{
	if (fse->index >= ARRAY_SIZE(ov08a10_formats))
		return -EINVAL;

	fse->min_width = ov08a10_formats[fse->index].min_width;
	fse->min_height = ov08a10_formats[fse->index].min_height;;
	fse->max_width = ov08a10_formats[fse->index].max_width;
	fse->max_height = ov08a10_formats[fse->index].max_height;

	return 0;
}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int ov08a10_get_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_state *cfg,
			  struct v4l2_subdev_format *fmt)
#else
static int ov08a10_get_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *fmt)
#endif
{
	struct ov08a10 *ov08a10 = to_ov08a10(sd);
	struct v4l2_mbus_framefmt *framefmt;

	mutex_lock(&ov08a10->lock);

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY)
		framefmt = v4l2_subdev_get_try_format(&ov08a10->sd, cfg,
						      fmt->pad);
	else
		framefmt = &ov08a10->current_format;

	fmt->format = *framefmt;

	mutex_unlock(&ov08a10->lock);

	return 0;
}


static inline u8 ov08a10_get_link_freq_index(struct ov08a10 *ov08a10)
{
	return ov08a10->current_mode->link_freq_index;
}

static s64 ov08a10_get_link_freq(struct ov08a10 *ov08a10)
{
	u8 index = ov08a10_get_link_freq_index(ov08a10);

	return *(ov08a10_link_freqs_ptr(ov08a10) + index);
}

static u64 ov08a10_calc_pixel_rate(struct ov08a10 *ov08a10)
{
	s64 link_freq = ov08a10_get_link_freq(ov08a10);
	u8 nlanes = ov08a10->nlanes;
	u64 pixel_rate;

	/* pixel rate = link_freq * 2 * nr_of_lanes / bits_per_sample */
	pixel_rate = link_freq * 2 * nlanes;
	do_div(pixel_rate, ov08a10->bpp);
	return pixel_rate;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int ov08a10_set_fmt(struct v4l2_subdev *sd,
			struct v4l2_subdev_state *cfg,
			struct v4l2_subdev_format *fmt)
#else
static int ov08a10_set_fmt(struct v4l2_subdev *sd,
			struct v4l2_subdev_pad_config *cfg,
			struct v4l2_subdev_format *fmt)
#endif
{
	struct ov08a10 *ov08a10 = to_ov08a10(sd);
	const struct ov08a10_mode *mode;
	struct v4l2_mbus_framefmt *format;
	unsigned int i,ret;

	mutex_lock(&ov08a10->lock);

	mode = v4l2_find_nearest_size(ov08a10_modes_ptr(ov08a10),
				 ov08a10_modes_num(ov08a10),
				width, height,
				fmt->format.width, fmt->format.height);

	fmt->format.width = mode->width;
	fmt->format.height = mode->height;

	for (i = 0; i < ARRAY_SIZE(ov08a10_formats); i++) {
		if (ov08a10_formats[i].code == fmt->format.code) {
			dev_info(ov08a10->dev, "find proper format %d \n",i);
			break;
		}
	}

	if (i >= ARRAY_SIZE(ov08a10_formats)) {
		i = 0;
		dev_err(ov08a10->dev, "No format. reset i = 0 \n");
	}

	fmt->format.code = ov08a10_formats[i].code;
	fmt->format.field = V4L2_FIELD_NONE;

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		dev_info(ov08a10->dev, "try format \n");
		format = v4l2_subdev_get_try_format(sd, cfg, fmt->pad);
		mutex_unlock(&ov08a10->lock);
		return 0;
	} else {
		dev_err(ov08a10->dev, "set format, w %d, h %d, code 0x%x \n",
			fmt->format.width, fmt->format.height,
			fmt->format.code);
		format = &ov08a10->current_format;
		ov08a10->current_mode = mode;
		ov08a10->bpp = ov08a10_formats[i].bpp;
		ov08a10->nlanes = 4;

		if (ov08a10->link_freq)
			__v4l2_ctrl_s_ctrl(ov08a10->link_freq, ov08a10_get_link_freq_index(ov08a10) );
		if (ov08a10->pixel_rate)
			__v4l2_ctrl_s_ctrl_int64(ov08a10->pixel_rate, ov08a10_calc_pixel_rate(ov08a10) );
		if (ov08a10->data_lanes)
			__v4l2_ctrl_s_ctrl(ov08a10->data_lanes, ov08a10->nlanes);
	}

	*format = fmt->format;
	ov08a10->status = 0;

	mutex_unlock(&ov08a10->lock);

	if (ov08a10->enWDRMode) {
		/* Set init register settings */
		#ifdef OV08A10_HDR_30FPS_1440M
		ret = ov08a10_set_register_array(ov08a10, setting_hdr_3840_2160_4lane_1440m_30fps,
			ARRAY_SIZE(setting_hdr_3840_2160_4lane_1440m_30fps));
		#else
		ret = ov08a10_set_register_array(ov08a10, setting_hdr_3840_2160_4lane_848m_15fps,
				ARRAY_SIZE(setting_hdr_3840_2160_4lane_848m_15fps));
		#endif
		if (ret < 0) {
			dev_err(ov08a10->dev, "Could not set init registers\n");
			return ret;
		} else
			dev_err(ov08a10->dev, "ov08a10 wdr mode init...\n");
	} else {
		/* Set init register settings */

		#ifdef OV08A10_SDR_60FPS_1440M
		ret = ov08a10_set_register_array(ov08a10, setting_3840_2160_4lane_1440m_60fps,
			ARRAY_SIZE(setting_3840_2160_4lane_1440m_60fps));
		#else
		ret = ov08a10_set_register_array(ov08a10, setting_3840_2160_4lane_800m_30fps,
			ARRAY_SIZE(setting_3840_2160_4lane_800m_30fps));
		#endif

		if (ret < 0) {
			dev_err(ov08a10->dev, "Could not set init registers\n");
			return ret;
		} else
			dev_err(ov08a10->dev, "ov08a10 linear mode init...\n");
	}

	return 0;
}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
int ov08a10_get_selection(struct v4l2_subdev *sd,
			     struct v4l2_subdev_state *cfg,
			     struct v4l2_subdev_selection *sel)
#else
int ov08a10_get_selection(struct v4l2_subdev *sd,
			     struct v4l2_subdev_pad_config *cfg,
			     struct v4l2_subdev_selection *sel)
#endif
{
	int rtn = 0;
	struct ov08a10 *ov08a10 = to_ov08a10(sd);
	const struct ov08a10_mode *mode = ov08a10->current_mode;

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
		dev_err(ov08a10->dev, "Error support target: 0x%x\n", sel->target);
	break;
	}

	return rtn;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int ov08a10_entity_init_cfg(struct v4l2_subdev *subdev,
				  struct v4l2_subdev_state *cfg)
#else
static int ov08a10_entity_init_cfg(struct v4l2_subdev *subdev,
				  struct v4l2_subdev_pad_config *cfg)
#endif
{
	struct v4l2_subdev_format fmt = { 0 };

	fmt.which = cfg ? V4L2_SUBDEV_FORMAT_TRY : V4L2_SUBDEV_FORMAT_ACTIVE;
	fmt.format.width = 1920;
	fmt.format.height = 1080;
	fmt.format.code = MEDIA_BUS_FMT_SBGGR10_1X10;

	ov08a10_set_fmt(subdev, cfg, &fmt);

	return 0;
}

/* Start streaming */
static int ov08a10_start_streaming(struct ov08a10 *ov08a10)
{

	/* Start streaming */
	return ov08a10_write_reg(ov08a10, 0x0100, 0x01);
}

static int ov08a10_set_stream(struct v4l2_subdev *sd, int enable)
{
	struct ov08a10 *ov08a10 = to_ov08a10(sd);
	int ret = 0;

	if (ov08a10->status == enable)
		return ret;
	else
		ov08a10->status = enable;

	if (enable) {
		ret = ov08a10_start_streaming(ov08a10);
		if (ret) {
			dev_err(ov08a10->dev, "Start stream failed\n");
			goto unlock_and_return;
		}

		dev_info(ov08a10->dev, "stream on\n");
	} else {
		ov08a10_stop_streaming(ov08a10);

		dev_info(ov08a10->dev, "stream off\n");
	}

unlock_and_return:

	return ret;
}

int ov08a10_power_on(struct device *dev, struct sensor_gpio *gpio)
{
	int ret;

	gpiod_set_value_cansleep(gpio->rst_gpio, 1);
	if (!IS_ERR_OR_NULL(gpio->pwdn_gpio)) {
		gpiod_set_value_cansleep(gpio->pwdn_gpio, 1);
	}
	ret = mclk_enable(dev,24000000);
	if (ret < 0 )
		dev_err(dev, "set mclk fail\n");

	// 30ms
	usleep_range(30000, 31000);

	return 0;
}

int ov08a10_power_off(struct device *dev, struct sensor_gpio *gpio)
{
	mclk_disable(dev);

	gpiod_set_value_cansleep(gpio->rst_gpio, 0);
	if (!IS_ERR_OR_NULL(gpio->pwdn_gpio)) {
		gpiod_set_value_cansleep(gpio->pwdn_gpio, 0);
	}
	return 0;
}

int ov08a10_power_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ov08a10 *ov08a10 = to_ov08a10(sd);

	gpiod_set_value_cansleep(ov08a10->gpio->rst_gpio, 0);

	return 0;
}

int ov08a10_power_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ov08a10 *ov08a10 = to_ov08a10(sd);

	gpiod_set_value_cansleep(ov08a10->gpio->rst_gpio, 1);

	return 0;
}

static int ov08a10_log_status(struct v4l2_subdev *sd)
{
	struct ov08a10 *ov08a10 = to_ov08a10(sd);

	dev_info(ov08a10->dev, "log status done\n");

	return 0;
}

int ov08a10_sbdev_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh) {
	struct ov08a10 *ov08a10 = to_ov08a10(sd);
	ov08a10_power_on(ov08a10->dev, ov08a10->gpio);
	return 0;
}

int ov08a10_sbdev_close(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh) {
	struct ov08a10 *ov08a10 = to_ov08a10(sd);
	ov08a10_set_stream(sd, 0);
	ov08a10_power_off(ov08a10->dev, ov08a10->gpio);
	return 0;
}


static const struct dev_pm_ops ov08a10_pm_ops = {
	SET_RUNTIME_PM_OPS(ov08a10_power_suspend, ov08a10_power_resume, NULL)
};

const struct v4l2_subdev_core_ops ov08a10_core_ops = {
	.log_status = ov08a10_log_status,
};

static const struct v4l2_subdev_video_ops ov08a10_video_ops = {
	.s_stream = ov08a10_set_stream,
};

static const struct v4l2_subdev_pad_ops ov08a10_pad_ops = {
	.init_cfg = ov08a10_entity_init_cfg,
	.enum_mbus_code = ov08a10_enum_mbus_code,
	.enum_frame_size = ov08a10_enum_frame_size,
	.get_selection = ov08a10_get_selection,
	.get_fmt = ov08a10_get_fmt,
	.set_fmt = ov08a10_set_fmt,
};
static struct v4l2_subdev_internal_ops ov08a10_internal_ops = {
	.open = ov08a10_sbdev_open,
	.close = ov08a10_sbdev_close,
};

static const struct v4l2_subdev_ops ov08a10_subdev_ops = {
	.core = &ov08a10_core_ops,
	.video = &ov08a10_video_ops,
	.pad = &ov08a10_pad_ops,
};

static const struct media_entity_operations ov08a10_subdev_entity_ops = {
	.link_validate = v4l2_subdev_link_validate,
};

static struct v4l2_ctrl_config wdr_cfg = {
	.ops = &ov08a10_ctrl_ops,
	.id = V4L2_CID_AML_MODE,
	.name = "wdr mode",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.min = 0,
	.max = 2,
	.step = 1,
	.def = 0,
};

static struct v4l2_ctrl_config fps_cfg = {
	.ops = &ov08a10_ctrl_ops,
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
	.ops = &ov08a10_ctrl_ops,
	.id = V4L2_CID_AML_CSI_LANES,
	.name = "sensor lanes",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.flags = V4L2_CTRL_FLAG_VOLATILE,
	.min = 1,
	.max = 4,
	.step = 1,
	.def = 4,
};

static int ov08a10_ctrls_init(struct ov08a10 *ov08a10)
{
	int rtn = 0;

	v4l2_ctrl_handler_init(&ov08a10->ctrls, 7);

	v4l2_ctrl_new_std(&ov08a10->ctrls, &ov08a10_ctrl_ops,
				V4L2_CID_GAIN, 0, 0xF0, 1, 0);

	v4l2_ctrl_new_std(&ov08a10->ctrls, &ov08a10_ctrl_ops,
				V4L2_CID_EXPOSURE, 0, 0xffff, 1, 0);

	ov08a10->link_freq = v4l2_ctrl_new_int_menu(&ov08a10->ctrls,
					       &ov08a10_ctrl_ops,
					       V4L2_CID_LINK_FREQ,
					       ov08a10_link_freqs_num(ov08a10) - 1,
					       0, ov08a10_link_freqs_ptr(ov08a10) );

	if (ov08a10->link_freq)
		ov08a10->link_freq->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	ov08a10->pixel_rate = v4l2_ctrl_new_std(&ov08a10->ctrls,
					       &ov08a10_ctrl_ops,
					       V4L2_CID_PIXEL_RATE,
					       1, INT_MAX, 1,
					       ov08a10_calc_pixel_rate(ov08a10));

	ov08a10->data_lanes = v4l2_ctrl_new_custom(&ov08a10->ctrls, &nlane_cfg, NULL);
	if (ov08a10->data_lanes)
		ov08a10->data_lanes->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	ov08a10->wdr = v4l2_ctrl_new_custom(&ov08a10->ctrls, &wdr_cfg, NULL);

	v4l2_ctrl_new_custom(&ov08a10->ctrls, &fps_cfg, NULL);

	ov08a10->sd.ctrl_handler = &ov08a10->ctrls;

	if (ov08a10->ctrls.error) {
		dev_err(ov08a10->dev, "Control initialization a error  %d\n",
			ov08a10->ctrls.error);
		rtn = ov08a10->ctrls.error;
	}

	return rtn;
}

static int ov08a10_register_subdev(struct ov08a10 *ov08a10)
{
	int rtn = 0;

	v4l2_i2c_subdev_init(&ov08a10->sd, ov08a10->client, &ov08a10_subdev_ops);

	ov08a10->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	ov08a10->sd.dev = &ov08a10->client->dev;
	ov08a10->sd.internal_ops = &ov08a10_internal_ops;
	ov08a10->sd.entity.ops = &ov08a10_subdev_entity_ops;
	ov08a10->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;
	snprintf(ov08a10->sd.name, sizeof(ov08a10->sd.name), AML_SENSOR_NAME, ov08a10->index);

	ov08a10->pad.flags = MEDIA_PAD_FL_SOURCE;
	rtn = media_entity_pads_init(&ov08a10->sd.entity, 1, &ov08a10->pad);
	if (rtn < 0) {
		dev_err(ov08a10->dev, "Could not register media entity\n");
		goto err_return;
	}

	rtn = v4l2_async_register_subdev(&ov08a10->sd);
	if (rtn < 0) {
		dev_err(ov08a10->dev, "Could not register v4l2 device\n");
		goto err_return;
	}

err_return:
	return rtn;
}

int ov08a10_init(struct i2c_client *client, void *sdrv)
{
	struct device *dev = &client->dev;
	struct ov08a10 *ov08a10;
	struct amlsens *sensor = (struct amlsens *)sdrv;
	int ret = -EINVAL;

	ov08a10 = devm_kzalloc(dev, sizeof(*ov08a10), GFP_KERNEL);
	if (!ov08a10)
		return -ENOMEM;

	ov08a10->dev = dev;
	ov08a10->client = client;
	ov08a10->client->addr = OV08A10_SLAVE_ID;
	ov08a10->gpio = &sensor->gpio;

	ov08a10->regmap = devm_regmap_init_i2c(client, &ov08a10_regmap_config);
	if (IS_ERR(ov08a10->regmap)) {
		dev_err(dev, "Unable to initialize I2C\n");
		return -ENODEV;
	}

	if (of_property_read_u32(dev->of_node, "index", &ov08a10->index)) {
		dev_err(dev, "Failed to read sensor index. default to 0\n");
		ov08a10->index = 0;
	}

	mutex_init(&ov08a10->lock);

	/* Power on the device to match runtime PM state below */
	ret = ov08a10_power_on(ov08a10->dev, ov08a10->gpio);
	if (ret < 0) {
		dev_err(dev, "Could not power on the device\n");
		return -ENODEV;
	}

	/*
	 * Initialize the frame format. In particular, ov08a10->current_mode
	 * and ov08a10->bpp are set to defaults: ov08a10_calc_pixel_rate() call
	 * below in ov08a10_ctrls_init relies on these fields.
	 */
	ov08a10_entity_init_cfg(&ov08a10->sd, NULL);

	ret = ov08a10_ctrls_init(ov08a10);
	if (ret) {
		dev_err(ov08a10->dev, "Error ctrls init\n");
		goto free_ctrl;
	}

	ret = ov08a10_register_subdev(ov08a10);
	if (ret) {
		dev_err(ov08a10->dev, "Error register subdev\n");
		goto free_entity;
	}

	dev_info(ov08a10->dev, "probe done \n");

	return 0;

free_entity:
	media_entity_cleanup(&ov08a10->sd.entity);
free_ctrl:
	v4l2_ctrl_handler_free(&ov08a10->ctrls);
	mutex_destroy(&ov08a10->lock);

	return ret;
}

int ov08a10_deinit(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ov08a10 *ov08a10 = to_ov08a10(sd);

	v4l2_async_unregister_subdev(sd);
	media_entity_cleanup(&sd->entity);
	v4l2_ctrl_handler_free(sd->ctrl_handler);

	mutex_destroy(&ov08a10->lock);

	ov08a10_power_off(ov08a10->dev, ov08a10->gpio);

	return 0;
}

int ov08a10_sensor_id(struct i2c_client *client)
{
	u32 id = 0;
	u8 val = 0;
	int rtn = -EINVAL;

	i2c_read_a16d8(client, OV08A10_SLAVE_ID, 0x300a, &val);
	id |= (val << 16);
	i2c_read_a16d8(client, OV08A10_SLAVE_ID, 0x300b, &val);
	id |= (val << 8);
	i2c_read_a16d8(client, OV08A10_SLAVE_ID, 0x300c, &val);
	id |= val;

	if (id != OV08A10_ID) {
		dev_info(&client->dev, "Failed to get ov08a10 id: 0x%x\n", id);
		return rtn;
	} else {
		dev_err(&client->dev, "success get ov08a10 id 0x%x", id);
	}

	return 0;
}
