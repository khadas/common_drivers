// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2023 Amlogic, Inc. All rights reserved.
 */

#include "sensor_drv.h"
#include "i2c_api.h"
#include "mclk_api.h"
#include "aml_imx585.h"

#define AML_SENSOR_NAME  "imx585-%u"

/* supported link frequencies */
#define FREQ_INDEX_2160P    0
#define FREQ_INDEX_2160P_HDR 1

static const s64 imx585_link_freq_4lanes[] = {
	[FREQ_INDEX_2160P] = 1440000000,
	[FREQ_INDEX_2160P_HDR] = 1440000000,
};

static inline const s64 *imx585_link_freqs_ptr(const struct imx585 *imx585)
{
	return imx585_link_freq_4lanes;

}

static inline int imx585_link_freqs_num(const struct imx585 *imx585)
{
	return ARRAY_SIZE(imx585_link_freq_4lanes);
}

static const struct imx585_mode imx585_modes_4lanes[] = {
	{
		.width = 3840,
		.height = 2160,
		.hmax = 0x0226,
		.link_freq_index = FREQ_INDEX_2160P,
		.max_fps = {
			.numerator = 10000,
			.denominator = 600000,
		},
		.data = imx585_4lane_3840_2160_1440m_60fps,
		.data_size = ARRAY_SIZE(imx585_4lane_3840_2160_1440m_60fps),
	},
	{
		.width = 3840,
		.height = 2160,
		.hmax = 0x0226,
		.link_freq_index = FREQ_INDEX_2160P_HDR,
		.max_fps = {
			.numerator = 10000,
			.denominator = 600000,
		},
		.data = dol_hdr_4k_30fps_1440Mbps_4lane_10bits,
		.data_size = ARRAY_SIZE(dol_hdr_4k_30fps_1440Mbps_4lane_10bits),
	},
};

static inline const struct imx585_mode *imx585_modes_ptr(const struct imx585 *imx585)
{
	return imx585_modes_4lanes;
}

static inline int imx585_modes_num(const struct imx585 *imx585)
{
	return ARRAY_SIZE(imx585_modes_4lanes);
}

static inline struct imx585 *to_imx585(struct v4l2_subdev *_sd)
{
	return container_of(_sd, struct imx585, sd);
}

static inline int imx585_read_reg(struct imx585 *imx585, u16 addr, u8 *value)
{
	unsigned int regval;

	int i, ret;

	for (i = 0; i < 3; ++i) {
		ret = regmap_read(imx585->regmap, addr, &regval);
		if (0 == ret ) {
			break;
		}
	}

	if (ret)
		dev_err(imx585->dev, "I2C read with i2c transfer failed for addr: %x, ret %d\n", addr, ret);

	*value = regval & 0xff;
	return 0;
}

static int imx585_write_reg(struct imx585 *imx585, u16 addr, u8 value)
{
	int i, ret;

	for (i = 0; i < 3; i++) {
		ret = regmap_write(imx585->regmap, addr, value);
		if (0 == ret) {
			break;
		}
	}

	if (ret)
		dev_err(imx585->dev, "I2C write failed for addr: %x, ret %d\n", addr, ret);

	return ret;
}

static int imx585_set_register_array(struct imx585 *imx585,
				const struct imx585_regval *settings,
				unsigned int num_settings)
{
	unsigned int i;
	int ret = 0;

	for (i = 0; i < num_settings; ++i, ++settings) {
		ret = imx585_write_reg(imx585, settings->reg, settings->val);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int imx585_write_buffered_reg(struct imx585 *imx585, u16 address_low,
				u8 nr_regs, u32 value)
{
	u8 val = 0;
	int ret = 0;
	unsigned int i;

	ret = imx585_write_reg(imx585, IMX585_REGHOLD, 0x01);
	if (ret) {
		dev_err(imx585->dev, "Error setting hold register\n");
		return ret;
	}

	for (i = 0; i < nr_regs; i++) {
		val = (u8)(value >> (i * 8)); //low register write low value
		//pr_err("%s addr: 0x%x, value 0x%x", __func__, (address_low + i), val);
		ret = imx585_write_reg(imx585, address_low + i, val);
		if (ret) {
			dev_err(imx585->dev, "Error writing buffered registers\n");
			return ret;
		}
	}

	ret = imx585_write_reg(imx585, IMX585_REGHOLD, 0x00);
	if (ret) {
		dev_err(imx585->dev, "Error setting hold register\n");
		return ret;
	}

	return ret;
}

static int imx585_set_gain(struct imx585 *imx585, u32 value)
{
	int ret = 0;
	u8 val;
	ret = imx585_write_buffered_reg(imx585, IMX585_GAIN, 2, value);
	if (ret)
	imx585_read_reg(imx585, IMX585_GAIN, &val);
	imx585_read_reg(imx585, IMX585_GAIN+1, &val);
	return ret;
}

static int imx585_set_exposure(struct imx585 *imx585, u32 value)
{
	int ret = 0;
	int shr0_reg = value & 0xFFFF;
	int shr1_reg = (value >> 16) & 0xFFFF;
	u8 val;
	//pr_err("%s exporL value 0x%x, exporS value 0x%x", __func__, shr0_reg, shr1_reg);
	ret = imx585_write_buffered_reg(imx585, IMX585_EXPOSURE, 2, shr0_reg);
	if (ret)
		dev_err(imx585->dev, "Unable to write expo\n");
	if (imx585->enWDRMode) {
		ret = imx585_write_buffered_reg(imx585, IMX585_EXPOSURE_SHR1, 2, shr1_reg);
		//dev_info(imx585->dev,"expo 0x%x reg value: SHR0-F1-big 0x%x SHR1-f0-small 0x%x", value, shr0_reg , shr1_reg);
		if (ret)
			dev_err(imx585->dev, "Unable to write exposure SHR1 reg\n");
	}



	imx585_read_reg(imx585, IMX585_EXPOSURE, &val);
	//pr_err("%s read exp value 0x%x", __func__, val);
	val = 0;
	imx585_read_reg(imx585, IMX585_EXPOSURE+1, &val);
	//pr_err("%s read exp value 0x%x", __func__, val);
	return ret;
}

static int imx585_set_fps(struct imx585 *imx585, u32 value)
{
	return 0;
}

static int imx585_stop_streaming(struct imx585 *imx585)
{
	int ret;
	imx585->enWDRMode = WDR_MODE_NONE;

	ret = imx585_write_reg(imx585, IMX585_STANDBY, 0x01);
	ret = imx585_write_reg(imx585, 0x3002, 0x01);
	if (ret != 0)
		pr_err("%s write register fail\n", __func__);
	return ret;
}

static int imx585_set_ctrl(struct v4l2_ctrl *ctrl)
{
	struct imx585 *imx585 = container_of(ctrl->handler,
					struct imx585, ctrls);
	int ret = 0;

	/* V4L2 controls values will be applied only when power is already up */
	if (!pm_runtime_get_if_in_use(imx585->dev))
		return 0;

	switch (ctrl->id) {
	case V4L2_CID_GAIN:
		ret = imx585_set_gain(imx585, ctrl->val);
		break;
	case V4L2_CID_EXPOSURE:
		ret = imx585_set_exposure(imx585, ctrl->val);
		break;
	case V4L2_CID_HBLANK:
		break;
	case V4L2_CID_AML_CSI_LANES:
		break;
	case V4L2_CID_AML_MODE:
		imx585->enWDRMode = ctrl->val;
		break;
	case V4L2_CID_AML_ORIG_FPS:
		imx585->fps = ctrl->val;
		if (imx585->fps != 60) {
			ret = imx585_set_fps(imx585, imx585->fps);
		}
		break;
	default:
		dev_err(imx585->dev, "Error ctrl->id %u, flag 0x%lx\n",
			ctrl->id, ctrl->flags);
		ret = -EINVAL;
		break;
	}

	pm_runtime_put(imx585->dev);

	return ret;
}

static const struct v4l2_ctrl_ops imx585_ctrl_ops = {
	.s_ctrl = imx585_set_ctrl,
};

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int imx585_enum_mbus_code(struct v4l2_subdev *sd,
				  struct v4l2_subdev_state *cfg,
				 struct v4l2_subdev_mbus_code_enum *code)
#else
static int imx585_enum_mbus_code(struct v4l2_subdev *sd,
				 struct v4l2_subdev_pad_config *cfg,
				 struct v4l2_subdev_mbus_code_enum *code)
#endif
{
	if (code->index >= ARRAY_SIZE(imx585_formats))
		return -EINVAL;

	code->code = imx585_formats[code->index].code;

	return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int imx585_enum_frame_size(struct v4l2_subdev *sd,
			        struct v4l2_subdev_state *cfg,
			       struct v4l2_subdev_frame_size_enum *fse)
#else
static int imx585_enum_frame_size(struct v4l2_subdev *sd,
			       struct v4l2_subdev_pad_config *cfg,
			       struct v4l2_subdev_frame_size_enum *fse)
#endif
{
	int cfg_num = 1; //report one max size
	if (fse->index >= cfg_num)
		return -EINVAL;

	fse->min_width = imx585_formats[0].max_width;
	fse->min_height = imx585_formats[0].max_height;;
	fse->max_width = imx585_formats[0].max_width;
	fse->max_height = imx585_formats[0].max_height;

	return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int imx585_enum_frame_interval(struct v4l2_subdev *sd,
				struct v4l2_subdev_state *sd_state,
				struct v4l2_subdev_frame_interval_enum *fie)
#else
static int imx585_enum_frame_interval(struct v4l2_subdev *sd,
				struct v4l2_subdev_pad_config *cfg,
				struct v4l2_subdev_frame_interval_enum *fie)
#endif
{
	struct imx585 *imx585 = to_imx585(sd);
	int cfg_num = imx585_modes_num(imx585);
	const struct imx585_mode* supported_modes = imx585_modes_ptr(imx585);
	if (fie->index >= cfg_num)
		return -EINVAL;

	fie->code = imx585_formats[0].code;
	fie->width = imx585_formats[0].max_width;
	fie->height = imx585_formats[0].max_height;
	fie->interval = supported_modes[fie->index].max_fps;
	return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int imx585_get_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_state *cfg,
			  struct v4l2_subdev_format *fmt)
#else
static int imx585_get_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *fmt)
#endif
{
	struct imx585 *imx585 = to_imx585(sd);
	struct v4l2_mbus_framefmt *framefmt;

	mutex_lock(&imx585->lock);

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY)
		framefmt = v4l2_subdev_get_try_format(&imx585->sd, cfg,
						      fmt->pad);
	else
		framefmt = &imx585->current_format;

	fmt->format = *framefmt;

	mutex_unlock(&imx585->lock);

	return 0;
}

static inline u8 imx585_get_link_freq_index(struct imx585 *imx585)
{
	return imx585->current_mode->link_freq_index;
}

static s64 imx585_get_link_freq(struct imx585 *imx585)
{
	u8 index = imx585_get_link_freq_index(imx585);

	return *(imx585_link_freqs_ptr(imx585) + index);
}

static u64 imx585_calc_pixel_rate(struct imx585 *imx585)
{
	s64 link_freq = imx585_get_link_freq(imx585);
	u8 nlanes = imx585->nlanes;
	u64 pixel_rate;

	/* pixel rate = link_freq * 2 * nr_of_lanes / bits_per_sample */
	pixel_rate = link_freq * 2 * nlanes;
	do_div(pixel_rate, imx585->bpp);
	return pixel_rate;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int imx585_set_fmt(struct v4l2_subdev *sd,
			struct v4l2_subdev_state *cfg,
			struct v4l2_subdev_format *fmt)
#else
static int imx585_set_fmt(struct v4l2_subdev *sd,
			struct v4l2_subdev_pad_config *cfg,
			struct v4l2_subdev_format *fmt)
#endif
{
	struct imx585 *imx585 = to_imx585(sd);
	const struct imx585_mode *mode;
	struct v4l2_mbus_framefmt *format;
	unsigned int i,ret;

	mutex_lock(&imx585->lock);
	mode = v4l2_find_nearest_size(imx585_modes_ptr(imx585),
				 imx585_modes_num(imx585),
				width, height,
				fmt->format.width, fmt->format.height);
	fmt->format.width = mode->width;
	fmt->format.height = mode->height;

	for (i = 0; i < ARRAY_SIZE(imx585_formats); i++) {
		if (imx585_formats[i].code == fmt->format.code) {
			dev_info(imx585->dev, "find proper format %d \n",i);
			break;
		}
	}

	if (i >= ARRAY_SIZE(imx585_formats)) {
		i = 0;
		dev_err(imx585->dev, "No format. reset i = 0 \n");
	}

	fmt->format.code = imx585_formats[i].code;
	fmt->format.field = V4L2_FIELD_NONE;

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		dev_info(imx585->dev, "try format \n");
		format = v4l2_subdev_get_try_format(sd, cfg, fmt->pad);
		mutex_unlock(&imx585->lock);
		return 0;
	} else {
		dev_err(imx585->dev, "set format, w %d, h %d, code 0x%x \n",
			fmt->format.width, fmt->format.height,
			fmt->format.code);
		format = &imx585->current_format;
		imx585->current_mode = mode;
		imx585->bpp = imx585_formats[i].bpp;
		imx585->nlanes = 4;

		if (imx585->link_freq)
			__v4l2_ctrl_s_ctrl(imx585->link_freq, imx585_get_link_freq_index(imx585) );
		if (imx585->pixel_rate)
			__v4l2_ctrl_s_ctrl_int64(imx585->pixel_rate, imx585_calc_pixel_rate(imx585) );
		if (imx585->data_lanes)
			__v4l2_ctrl_s_ctrl(imx585->data_lanes, imx585->nlanes);
	}

	*format = fmt->format;
	imx585->status = 0;

	mutex_unlock(&imx585->lock);

	if (imx585->enWDRMode) {
		ret = imx585_set_register_array(imx585, dol_hdr_4k_30fps_1440Mbps_4lane_10bits,
			ARRAY_SIZE(dol_hdr_4k_30fps_1440Mbps_4lane_10bits));
		if (ret < 0) {
			dev_err(imx585->dev, "Could not set init wdr registers\n");
			return ret;
		} else
			dev_err(imx585->dev, "imx585 wdr mode init...\n");
	} else {/* Set init register settings */
		ret = imx585_set_register_array(imx585, imx585_4lane_3840_2160_1440m_60fps,
			ARRAY_SIZE(imx585_4lane_3840_2160_1440m_60fps));
		if (ret < 0) {
			dev_err(imx585->dev, "Could not set init registers\n");
			return ret;
		} else
			dev_err(imx585->dev, "imx585 linear mode init...\n");
	}
	return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
int imx585_get_selection(struct v4l2_subdev *sd,
			     struct v4l2_subdev_state *cfg,
			     struct v4l2_subdev_selection *sel)
#else
int imx585_get_selection(struct v4l2_subdev *sd,
			     struct v4l2_subdev_pad_config *cfg,
			     struct v4l2_subdev_selection *sel)
#endif
{
	int rtn = 0;
	struct imx585 *imx585 = to_imx585(sd);
	const struct imx585_mode *mode = imx585->current_mode;

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
		dev_err(imx585->dev, "Error support target: 0x%x\n", sel->target);
	break;
	}

	return rtn;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int imx585_entity_init_cfg(struct v4l2_subdev *subdev,
				  struct v4l2_subdev_state *cfg)
#else
static int imx585_entity_init_cfg(struct v4l2_subdev *subdev,
				  struct v4l2_subdev_pad_config *cfg)
#endif
{
	struct v4l2_subdev_format fmt = { 0 };

	fmt.which = cfg ? V4L2_SUBDEV_FORMAT_TRY : V4L2_SUBDEV_FORMAT_ACTIVE;
	fmt.format.width = 3840;
	fmt.format.height = 2160;
	fmt.format.code = MEDIA_BUS_FMT_SRGGB10_1X10;
	imx585_set_fmt(subdev, cfg, &fmt);
	return 0;
}

static int imx585_start_streaming(struct imx585 *imx585)
{
	int ret = 0;
	ret = imx585_write_reg(imx585, IMX585_STANDBY, 0x00);
	if (ret) {
		pr_err("%s: start streaming fail\n", __func__);
		return ret;
	}

	usleep_range(30000, 31000);
	ret = imx585_write_reg(imx585, 0x3002, 0x00);
	if (ret) {
		pr_err("%s: start streaming fail\n", __func__);
		return ret;
	}

	return ret;
}

static int imx585_set_stream(struct v4l2_subdev *sd, int enable)
{
	struct imx585 *imx585 = to_imx585(sd);
	int ret = 0;

	if (imx585->status == enable)
		return ret;
	else
		imx585->status = enable;

	if (enable) {
		ret = imx585_start_streaming(imx585);
		if (ret) {
			dev_err(imx585->dev, "Start stream failed\n");
			goto unlock_and_return;
		}

		dev_info(imx585->dev, "stream on\n");
	} else {
		imx585_stop_streaming(imx585);

		dev_info(imx585->dev, "stream off\n");
	}

unlock_and_return:

	return ret;
}

int imx585_power_on(struct device *dev, struct sensor_gpio *gpio)
{
	int ret;

	gpiod_set_value_cansleep(gpio->rst_gpio, 1);

	ret = mclk_enable(dev, 37125000);
	if (ret < 0 )
		dev_err(dev, "set mclk fail\n");

	// 30ms
	usleep_range(30000, 31000);

	return 0;
}

int imx585_power_off(struct device *dev, struct sensor_gpio *gpio)
{
	mclk_disable(dev);
	gpiod_set_value_cansleep(gpio->rst_gpio, 0);
	return 0;
}

int imx585_power_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct imx585 *imx585 = to_imx585(sd);

	dev_err(dev, "%s\n", __func__);
	imx585_power_off(imx585->dev, imx585->gpio);

	return 0;
}

int imx585_power_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct imx585 *imx585 = to_imx585(sd);

	dev_err(dev, "%s\n", __func__);
	imx585_power_on(imx585->dev, imx585->gpio);

	return 0;
}

static int imx585_log_status(struct v4l2_subdev *sd)
{
	struct imx585 *imx585 = to_imx585(sd);

	dev_info(imx585->dev, "log status done\n");

	return 0;
}

int imx585_sbdev_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh) {
	struct imx585 *imx585 = to_imx585(sd);

	if (atomic_inc_return(&imx585->open_count) == 1)
		imx585_power_on(imx585->dev, imx585->gpio);
	return 0;
}

int imx585_sbdev_close(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh) {
	struct imx585 *imx585 = to_imx585(sd);
	imx585_set_stream(sd, 0);

	if (atomic_dec_and_test(&imx585->open_count))
		imx585_power_off(imx585->dev, imx585->gpio);
	return 0;
}

static const struct dev_pm_ops imx585_pm_ops = {
	SET_RUNTIME_PM_OPS(imx585_power_suspend, imx585_power_resume, NULL)
};

const struct v4l2_subdev_core_ops imx585_core_ops = {
	.log_status = imx585_log_status,
};

static const struct v4l2_subdev_video_ops imx585_video_ops = {
	.s_stream = imx585_set_stream,
};

static const struct v4l2_subdev_pad_ops imx585_pad_ops = {
	.init_cfg = imx585_entity_init_cfg,
	.enum_mbus_code = imx585_enum_mbus_code,
	.enum_frame_size = imx585_enum_frame_size,
	.enum_frame_interval = imx585_enum_frame_interval,
	.get_selection = imx585_get_selection,
	.get_fmt = imx585_get_fmt,
	.set_fmt = imx585_set_fmt,
};
static struct v4l2_subdev_internal_ops imx585_internal_ops = {
	.open = imx585_sbdev_open,
	.close = imx585_sbdev_close,
};

static const struct v4l2_subdev_ops imx585_subdev_ops = {
	.core = &imx585_core_ops,
	.video = &imx585_video_ops,
	.pad = &imx585_pad_ops,
};

static const struct media_entity_operations imx585_subdev_entity_ops = {
	.link_validate = v4l2_subdev_link_validate,
};

static struct v4l2_ctrl_config wdr_cfg = {
	.ops = &imx585_ctrl_ops,
	.id = V4L2_CID_AML_MODE,
	.name = "wdr mode",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.min = 0,
	.max = 2,
	.step = 1,
	.def = 0,
};

static struct v4l2_ctrl_config fps_cfg = {
	.ops = &imx585_ctrl_ops,
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
	.ops = &imx585_ctrl_ops,
	.id = V4L2_CID_AML_CSI_LANES,
	.name = "sensor lanes",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.flags = V4L2_CTRL_FLAG_VOLATILE,
	.min = 1,
	.max = 4,
	.step = 1,
	.def = 4,
};

static int imx585_ctrls_init(struct imx585 *imx585)
{
	int rtn = 0;

	v4l2_ctrl_handler_init(&imx585->ctrls, 7);

	v4l2_ctrl_new_std(&imx585->ctrls, &imx585_ctrl_ops,
				V4L2_CID_GAIN, 0, 0xffff, 1, 0);

	v4l2_ctrl_new_std(&imx585->ctrls, &imx585_ctrl_ops,
				V4L2_CID_EXPOSURE, 0, 0x7fffffff, 1, 0);

	imx585->link_freq = v4l2_ctrl_new_int_menu(&imx585->ctrls,
					       &imx585_ctrl_ops,
					       V4L2_CID_LINK_FREQ,
					       imx585_link_freqs_num(imx585) - 1,
					       0, imx585_link_freqs_ptr(imx585) );

	if (imx585->link_freq)
		imx585->link_freq->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	imx585->pixel_rate = v4l2_ctrl_new_std(&imx585->ctrls,
					       &imx585_ctrl_ops,
					       V4L2_CID_PIXEL_RATE,
					       1, INT_MAX, 1,
					       imx585_calc_pixel_rate(imx585));

	imx585->data_lanes = v4l2_ctrl_new_custom(&imx585->ctrls, &nlane_cfg, NULL);
	if (imx585->data_lanes)
		imx585->data_lanes->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	imx585->wdr = v4l2_ctrl_new_custom(&imx585->ctrls, &wdr_cfg, NULL);

	v4l2_ctrl_new_custom(&imx585->ctrls, &fps_cfg, NULL);

	imx585->sd.ctrl_handler = &imx585->ctrls;

	if (imx585->ctrls.error) {
		dev_err(imx585->dev, "Control initialization a error  %d\n",
			imx585->ctrls.error);
		rtn = imx585->ctrls.error;
	}

	return rtn;
}

static int imx585_register_subdev(struct imx585 *imx585)
{
	int rtn = 0;

	v4l2_i2c_subdev_init(&imx585->sd, imx585->client, &imx585_subdev_ops);

	imx585->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	imx585->sd.dev = &imx585->client->dev;
	imx585->sd.internal_ops = &imx585_internal_ops;
	imx585->sd.entity.ops = &imx585_subdev_entity_ops;
	imx585->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;
	snprintf(imx585->sd.name, sizeof(imx585->sd.name), AML_SENSOR_NAME, imx585->index);

	imx585->pad.flags = MEDIA_PAD_FL_SOURCE;
	rtn = media_entity_pads_init(&imx585->sd.entity, 1, &imx585->pad);
	if (rtn < 0) {
		dev_err(imx585->dev, "Could not register media entity\n");
		goto err_return;
	}

	rtn = v4l2_async_register_subdev(&imx585->sd);
	if (rtn < 0) {
		dev_err(imx585->dev, "Could not register v4l2 device\n");
		goto err_return;
	}

err_return:
	return rtn;
}

int imx585_init(struct i2c_client *client, void *sdrv)
{
	struct device *dev = &client->dev;
	struct imx585 *imx585;
	struct amlsens *sensor = (struct amlsens *)sdrv;
	int ret = -EINVAL;

	imx585 = devm_kzalloc(dev, sizeof(*imx585), GFP_KERNEL);
	if (!imx585)
		return -ENOMEM;

	imx585->dev = dev;
	imx585->client = client;
	imx585->client->addr = IMX585_SLAVE_ID;
	imx585->gpio = &sensor->gpio;
	imx585->fps = 60;
	imx585->nlanes = 4;

	imx585->regmap = devm_regmap_init_i2c(client, &imx585_regmap_config);
	if (IS_ERR(imx585->regmap)) {
		dev_err(dev, "Unable to initialize I2C\n");
		return -ENODEV;
	}

	if (of_property_read_u32(dev->of_node, "index", &imx585->index)) {
		dev_err(dev, "Failed to read sensor index. default to 0\n");
		imx585->index = 0;
	}

	mutex_init(&imx585->lock);

	/* Power on the device to match runtime PM state below */
	ret = imx585_power_on(imx585->dev, imx585->gpio);
	if (ret < 0) {
		dev_err(dev, "Could not power on the device\n");
		return -ENODEV;
	}

	/*
	 * Initialize the frame format. In particular, imx585->current_mode
	 * and imx585->bpp are set to defaults: imx585_calc_pixel_rate() call
	 * below in imx585_ctrls_init relies on these fields.
	 */
	imx585_entity_init_cfg(&imx585->sd, NULL);

	ret = imx585_ctrls_init(imx585);
	if (ret) {
		dev_err(imx585->dev, "Error ctrls init\n");
		goto free_ctrl;
	}

	ret = imx585_register_subdev(imx585);
	if (ret) {
		dev_err(imx585->dev, "Error register subdev\n");
		goto free_entity;
	}

	dev_info(imx585->dev, "probe done \n");

	return 0;

free_entity:
	media_entity_cleanup(&imx585->sd.entity);
free_ctrl:
	v4l2_ctrl_handler_free(&imx585->ctrls);
	mutex_destroy(&imx585->lock);

	return ret;
}

int imx585_deinit(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct imx585 *imx585 = to_imx585(sd);

	v4l2_async_unregister_subdev(sd);
	media_entity_cleanup(&sd->entity);
	v4l2_ctrl_handler_free(sd->ctrl_handler);

	mutex_destroy(&imx585->lock);

	imx585_power_off(imx585->dev, imx585->gpio);

	return 0;
}

int imx585_sensor_id(struct i2c_client *client)
{
	u32 id = 0;
	u8 val = 0;
	int rtn = -EINVAL;

	i2c_write_a16d8(client, IMX585_SLAVE_ID, IMX585_STANDBY, 0x00);
	usleep_range(30000, 31000);

	i2c_read_a16d8(client, IMX585_SLAVE_ID, 0x5A1E, &val);
	id |= (val << 8);
	i2c_read_a16d8(client, IMX585_SLAVE_ID, 0x5A1D, &val);
	id |= val;

	i2c_write_a16d8(client, IMX585_SLAVE_ID, IMX585_STANDBY, 0x01);

	if (id != IMX585_ID) {
		dev_err(&client->dev, "Failed to get imx585 id: 0x%x\n", id);
		return rtn;
	} else {
		dev_err(&client->dev, "success get imx585 id 0x%x", id);
	}

	return 0;
}

