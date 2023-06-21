// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2023 Amlogic, Inc. All rights reserved.
 */

#include "sensor_drv.h"
#include "i2c_api.h"
#include "mclk_api.h"
#include "aml_imx415.h"

#define AML_SENSOR_NAME  "imx415-%u"

/* supported link frequencies */
#define FREQ_INDEX_1080P	0
#define FREQ_INDEX_720P		1

/* supported link frequencies */
static const s64 imx415_link_freq_2lanes[] = {
	[FREQ_INDEX_1080P] = 1440000000,
	[FREQ_INDEX_720P] = 1440000000,
};

static const s64 imx415_link_freq_4lanes[] = {
	[FREQ_INDEX_1080P] = 1440000000,
	[FREQ_INDEX_720P] = 1440000000,
};

static inline const s64 *imx415_link_freqs_ptr(const struct imx415 *imx415)
{
	if (imx415->nlanes == 2)
		return imx415_link_freq_2lanes;
	else {
		return imx415_link_freq_4lanes;
	}
}

static inline int imx415_link_freqs_num(const struct imx415 *imx415)
{
	if (imx415->nlanes == 2)
		return ARRAY_SIZE(imx415_link_freq_2lanes);
	else
		return ARRAY_SIZE(imx415_link_freq_4lanes);
}

/* Mode configs */
static const struct imx415_mode imx415_modes_4lanes[] = {
	{
		.width = 3840,
		.height = 2160,
		.hmax = 0x0898,
		.link_freq_index = FREQ_INDEX_1080P,
		.data = linear_4k_30fps_1440Mbps_4lane_10bits,
		.data_size = ARRAY_SIZE(linear_4k_30fps_1440Mbps_4lane_10bits),
	},
};

static inline const struct imx415_mode *imx415_modes_ptr(const struct imx415 *imx415)
{
	return imx415_modes_4lanes;
}

static inline int imx415_modes_num(const struct imx415 *imx415)
{
	return ARRAY_SIZE(imx415_modes_4lanes);
}

static inline struct imx415 *to_imx415(struct v4l2_subdev *_sd)
{
	return container_of(_sd, struct imx415, sd);
}

static inline int imx415_read_reg(struct imx415 *imx415, u16 addr, u8 *value)
{
	unsigned int regval;

	int i, ret;

	for ( i = 0; i < 3; ++i ) {
		ret = regmap_read(imx415->regmap, addr, &regval);
		if ( 0 == ret ) {
			break;
		}
	}

	if (ret)
		dev_err(imx415->dev, "I2C read with i2c transfer failed for addr: %x, ret %d\n", addr, ret);

	*value = regval & 0xff;
	return 0;
}

static int imx415_write_reg(struct imx415 *imx415, u16 addr, u8 value)
{
	int i, ret;

	for (i = 0; i < 3; i++) {
		ret = regmap_write(imx415->regmap, addr, value);
		if (0 == ret) {
			break;
		}
	}

	if (ret)
		dev_err(imx415->dev, "I2C write failed for addr: %x, ret %d\n", addr, ret);

	return ret;
}

static int imx415_set_register_array(struct imx415 *imx415,
				     const struct imx415_regval *settings,
				     unsigned int num_settings)
{
	unsigned int i;
	int ret = 0;

	for (i = 0; i < num_settings; ++i, ++settings) {
		ret = imx415_write_reg(imx415, settings->reg, settings->val);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int imx415_write_buffered_reg(struct imx415 *imx415, u16 address_low,
				     u8 nr_regs, u32 value)
{
	unsigned int i;
	int ret;

	for (i = 0; i < nr_regs; i++) {
		ret = imx415_write_reg(imx415, address_low + i,
				       (u8)(value >> (i * 8)));
		if (ret) {
			dev_err(imx415->dev, "Error writing buffered registers\n");
			return ret;
		}
	}

	return ret;
}

static int imx415_set_gain(struct imx415 *imx415, u32 value)
{
	int ret;
	//dev_err(imx415->dev, "imx415_set_gain = 0x%x \n", value);

	ret = imx415_write_buffered_reg(imx415, IMX415_GAIN, 1, value);
	if (ret)
		dev_err(imx415->dev, "Unable to write gain\n");

	return ret;
}

static int imx415_set_exposure(struct imx415 *imx415, u32 value)
{
	int ret;
	//dev_err(imx415->dev, "imx415_set_exposure = 0x%x \n", value);

	ret = imx415_write_buffered_reg(imx415, IMX415_EXPOSURE, 2, value);
	if (ret)
		dev_err(imx415->dev, "Unable to write gain\n");

	return ret;
}

static int imx415_set_fps(struct imx415 *imx415, u32 value)
{
	u32 vts = 0;
	u8 vts_h, vts_l;

	//dev_err(imx415->dev, "-imx415-value = %d\n", value);

	vts = 30 * 4503 / value;
	vts_h = (vts >> 8) & 0x7f;
	vts_l = vts & 0xff;

	imx415_write_reg(imx415, 0x3025, vts_h);
	imx415_write_reg(imx415, 0x3024, vts_l);

	return 0;
}

/* Stop streaming */
static int imx415_stop_streaming(struct imx415 *imx415)
{
	imx415->enWDRMode = WDR_MODE_NONE;
	return imx415_write_reg(imx415, 0x3000, 0x01);
}

static int imx415_set_ctrl(struct v4l2_ctrl *ctrl)
{
	struct imx415 *imx415 = container_of(ctrl->handler,
					     struct imx415, ctrls);
	int ret = 0;

	/* V4L2 controls values will be applied only when power is already up */
	if (!pm_runtime_get_if_in_use(imx415->dev))
		return 0;

	switch (ctrl->id) {
	case V4L2_CID_GAIN:
		ret = imx415_set_gain(imx415, ctrl->val);
		break;
	case V4L2_CID_EXPOSURE:
		ret = imx415_set_exposure(imx415, ctrl->val);
		break;
	case V4L2_CID_HBLANK:
		break;
	case V4L2_CID_AML_CSI_LANES:
		break;
	case V4L2_CID_AML_MODE:
		imx415->enWDRMode = ctrl->val;
		break;
	case V4L2_CID_AML_ORIG_FPS:
		ret = imx415_set_fps(imx415, ctrl->val);
		break;
	default:
		dev_err(imx415->dev, "Error ctrl->id %u, flag 0x%lx\n",
			ctrl->id, ctrl->flags);
		ret = -EINVAL;
		break;
	}

	pm_runtime_put(imx415->dev);

	return ret;
}

static const struct v4l2_ctrl_ops imx415_ctrl_ops = {
	.s_ctrl = imx415_set_ctrl,
};
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int imx415_enum_mbus_code(struct v4l2_subdev *sd,
				  struct v4l2_subdev_state *cfg,
				 struct v4l2_subdev_mbus_code_enum *code)
#else
static int imx415_enum_mbus_code(struct v4l2_subdev *sd,
				 struct v4l2_subdev_pad_config *cfg,
				 struct v4l2_subdev_mbus_code_enum *code)
#endif
{
	if (code->index >= ARRAY_SIZE(imx415_formats))
		return -EINVAL;

	code->code = imx415_formats[code->index].code;

	return 0;
}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int imx415_enum_frame_size(struct v4l2_subdev *sd,
			        struct v4l2_subdev_state *cfg,
			       struct v4l2_subdev_frame_size_enum *fse)
#else
static int imx415_enum_frame_size(struct v4l2_subdev *sd,
			       struct v4l2_subdev_pad_config *cfg,
			       struct v4l2_subdev_frame_size_enum *fse)
#endif
{
	if (fse->index >= ARRAY_SIZE(imx415_formats))
		return -EINVAL;

	fse->min_width = imx415_formats[fse->index].min_width;
	fse->min_height = imx415_formats[fse->index].min_height;;
	fse->max_width = imx415_formats[fse->index].max_width;
	fse->max_height = imx415_formats[fse->index].max_height;

	return 0;
}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int imx415_get_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_state *cfg,
			  struct v4l2_subdev_format *fmt)
#else
static int imx415_get_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *fmt)
#endif
{
	struct imx415 *imx415 = to_imx415(sd);
	struct v4l2_mbus_framefmt *framefmt;

	mutex_lock(&imx415->lock);

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY)
		framefmt = v4l2_subdev_get_try_format(&imx415->sd, cfg,
						      fmt->pad);
	else
		framefmt = &imx415->current_format;

	fmt->format = *framefmt;

	mutex_unlock(&imx415->lock);

	return 0;
}


static inline u8 imx415_get_link_freq_index(struct imx415 *imx415)
{
	return imx415->current_mode->link_freq_index;
}

static s64 imx415_get_link_freq(struct imx415 *imx415)
{
	u8 index = imx415_get_link_freq_index(imx415);

	return *(imx415_link_freqs_ptr(imx415) + index);
}

static u64 imx415_calc_pixel_rate(struct imx415 *imx415)
{
	s64 link_freq = imx415_get_link_freq(imx415);
	u8 nlanes = imx415->nlanes;
	u64 pixel_rate;

	/* pixel rate = link_freq * 2 * nr_of_lanes / bits_per_sample */
	pixel_rate = link_freq * 2 * nlanes;
	do_div(pixel_rate, imx415->bpp);
	return pixel_rate;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int imx415_set_fmt(struct v4l2_subdev *sd,
			struct v4l2_subdev_state *cfg,
			struct v4l2_subdev_format *fmt)
#else
static int imx415_set_fmt(struct v4l2_subdev *sd,
			struct v4l2_subdev_pad_config *cfg,
			struct v4l2_subdev_format *fmt)
#endif
{
	struct imx415 *imx415 = to_imx415(sd);
	const struct imx415_mode *mode;
	struct v4l2_mbus_framefmt *format;
	unsigned int i,ret;

	mutex_lock(&imx415->lock);

	mode = v4l2_find_nearest_size(imx415_modes_ptr(imx415),
				 imx415_modes_num(imx415),
				width, height,
				fmt->format.width, fmt->format.height);

	fmt->format.width = mode->width;
	fmt->format.height = mode->height;

	for (i = 0; i < ARRAY_SIZE(imx415_formats); i++) {
		if (imx415_formats[i].code == fmt->format.code) {
			dev_info(imx415->dev, "find proper format %d \n",i);
			break;
		}
	}

	if (i >= ARRAY_SIZE(imx415_formats)) {
		i = 0;
		dev_err(imx415->dev, "No format. reset i = 0 \n");
	}

	fmt->format.code = imx415_formats[i].code;
	fmt->format.field = V4L2_FIELD_NONE;

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		dev_info(imx415->dev, "try format \n");
		format = v4l2_subdev_get_try_format(sd, cfg, fmt->pad);
		mutex_unlock(&imx415->lock);
		return 0;
	} else {
		dev_err(imx415->dev, "set format, w %d, h %d, code 0x%x \n",
            fmt->format.width, fmt->format.height,
            fmt->format.code);
		format = &imx415->current_format;
		imx415->current_mode = mode;
		imx415->bpp = imx415_formats[i].bpp;
		imx415->nlanes = 4;

		if (imx415->link_freq)
			__v4l2_ctrl_s_ctrl(imx415->link_freq, imx415_get_link_freq_index(imx415) );
		if (imx415->pixel_rate)
			__v4l2_ctrl_s_ctrl_int64(imx415->pixel_rate, imx415_calc_pixel_rate(imx415) );
		if (imx415->data_lanes)
			__v4l2_ctrl_s_ctrl(imx415->data_lanes, imx415->nlanes);
	}

	*format = fmt->format;
	imx415->status = 0;

	mutex_unlock(&imx415->lock);

	if (imx415->enWDRMode) {
		/* Set init register settings */
		ret = imx415_set_register_array(imx415, dol_4k_30fps_1440Mbps_4lane_10bits,
			ARRAY_SIZE(dol_4k_30fps_1440Mbps_4lane_10bits));
		if (ret < 0) {
			dev_err(imx415->dev, "Could not set init registers\n");
			return ret;
		} else
			dev_err(imx415->dev, "imx415 wdr mode init...\n");
	} else {
		/* Set init register settings */
		ret = imx415_set_register_array(imx415, linear_4k_30fps_1440Mbps_4lane_10bits,
			ARRAY_SIZE(linear_4k_30fps_1440Mbps_4lane_10bits));
		if (ret < 0) {
			dev_err(imx415->dev, "Could not set init registers\n");
			return ret;
		} else
			dev_err(imx415->dev, "imx415 linear mode init...\n");
	}

	return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
int imx415_get_selection(struct v4l2_subdev *sd,
			     struct v4l2_subdev_state *cfg,
			     struct v4l2_subdev_selection *sel)
#else
int imx415_get_selection(struct v4l2_subdev *sd,
			     struct v4l2_subdev_pad_config *cfg,
			     struct v4l2_subdev_selection *sel)
#endif
{
	int rtn = 0;
	struct imx415 *imx415 = to_imx415(sd);
	const struct imx415_mode *mode = imx415->current_mode;

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
		dev_err(imx415->dev, "Error support target: 0x%x\n", sel->target);
	break;
	}

	return rtn;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int imx415_entity_init_cfg(struct v4l2_subdev *subdev,
				  struct v4l2_subdev_state *cfg)
#else
static int imx415_entity_init_cfg(struct v4l2_subdev *subdev,
				  struct v4l2_subdev_pad_config *cfg)
#endif
{
	struct v4l2_subdev_format fmt = { 0 };

	fmt.which = cfg ? V4L2_SUBDEV_FORMAT_TRY : V4L2_SUBDEV_FORMAT_ACTIVE;
	fmt.format.width = 3840;
	fmt.format.height = 2160;
	fmt.format.code = MEDIA_BUS_FMT_SGBRG10_1X10;

	imx415_set_fmt(subdev, cfg, &fmt);

	return 0;
}

/* Start streaming */
static int imx415_start_streaming(struct imx415 *imx415)
{

	/* Start streaming */
	return imx415_write_reg(imx415, 0x3000, 0x00);
}

static int imx415_set_stream(struct v4l2_subdev *sd, int enable)
{
	struct imx415 *imx415 = to_imx415(sd);
	int ret = 0;

	if (imx415->status == enable)
		return ret;
	else
		imx415->status = enable;

	if (enable) {
		ret = imx415_start_streaming(imx415);
		if (ret) {
			dev_err(imx415->dev, "Start stream failed\n");
			goto unlock_and_return;
		}

		dev_info(imx415->dev, "stream on\n");
	} else {
		imx415_stop_streaming(imx415);

		dev_info(imx415->dev, "stream off\n");
	}

unlock_and_return:

	return ret;
}

int imx415_power_on(struct device *dev, struct sensor_gpio *gpio)
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

int imx415_power_off(struct device *dev, struct sensor_gpio *gpio)
{
	mclk_disable(dev);

	gpiod_set_value_cansleep(gpio->rst_gpio, 0);
	if (!IS_ERR_OR_NULL(gpio->pwdn_gpio)) {
		gpiod_set_value_cansleep(gpio->pwdn_gpio, 0);
	}
	return 0;
}

int imx415_power_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct imx415 *imx415 = to_imx415(sd);

	gpiod_set_value_cansleep(imx415->gpio->rst_gpio, 0);

	return 0;
}

int imx415_power_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct imx415 *imx415 = to_imx415(sd);

	gpiod_set_value_cansleep(imx415->gpio->rst_gpio, 1);

	return 0;
}

static int imx415_log_status(struct v4l2_subdev *sd)
{
	struct imx415 *imx415 = to_imx415(sd);

	dev_info(imx415->dev, "log status done\n");

	return 0;
}

int imx415_sbdev_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh) {
	struct imx415 *imx415 = to_imx415(sd);

	imx415_power_on(imx415->dev, imx415->gpio);

	return 0;
}

int imx415_sbdev_close(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh) {
	struct imx415 *imx415 = to_imx415(sd);

	imx415_stop_streaming(imx415);
	imx415_power_off(imx415->dev, imx415->gpio);

	return 0;
}

static const struct dev_pm_ops imx415_pm_ops = {
	SET_RUNTIME_PM_OPS(imx415_power_suspend, imx415_power_resume, NULL)
};

const struct v4l2_subdev_core_ops imx415_core_ops = {
	.log_status = imx415_log_status,
};

static const struct v4l2_subdev_video_ops imx415_video_ops = {
	.s_stream = imx415_set_stream,
};

static const struct v4l2_subdev_pad_ops imx415_pad_ops = {
	.init_cfg = imx415_entity_init_cfg,
	.enum_mbus_code = imx415_enum_mbus_code,
	.enum_frame_size = imx415_enum_frame_size,
	.get_selection = imx415_get_selection,
	.get_fmt = imx415_get_fmt,
	.set_fmt = imx415_set_fmt,
};

static struct v4l2_subdev_internal_ops imx415_internal_ops = {
	.open = imx415_sbdev_open,
	.close = imx415_sbdev_close,
};

static const struct v4l2_subdev_ops imx415_subdev_ops = {
	.core = &imx415_core_ops,
	.video = &imx415_video_ops,
	.pad = &imx415_pad_ops,
};

static const struct media_entity_operations imx415_subdev_entity_ops = {
	.link_validate = v4l2_subdev_link_validate,
};

static struct v4l2_ctrl_config wdr_cfg = {
	.ops = &imx415_ctrl_ops,
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
	.ops = &imx415_ctrl_ops,
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
	.ops = &imx415_ctrl_ops,
	.id = V4L2_CID_AML_CSI_LANES,
	.name = "sensor lanes",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.flags = V4L2_CTRL_FLAG_VOLATILE,
	.min = 1,
	.max = 4,
	.step = 1,
	.def = 4,
};

static int imx415_ctrls_init(struct imx415 *imx415)
{
	int rtn = 0;

	v4l2_ctrl_handler_init(&imx415->ctrls, 7);

	v4l2_ctrl_new_std(&imx415->ctrls, &imx415_ctrl_ops,
				V4L2_CID_GAIN, 0, 0xF0, 1, 0);

	v4l2_ctrl_new_std(&imx415->ctrls, &imx415_ctrl_ops,
				V4L2_CID_EXPOSURE, 0, 0xffff, 1, 0);

	imx415->link_freq = v4l2_ctrl_new_int_menu(&imx415->ctrls,
					       &imx415_ctrl_ops,
					       V4L2_CID_LINK_FREQ,
					       imx415_link_freqs_num(imx415) - 1,
					       0, imx415_link_freqs_ptr(imx415) );

	if (imx415->link_freq)
		imx415->link_freq->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	imx415->pixel_rate = v4l2_ctrl_new_std(&imx415->ctrls,
					       &imx415_ctrl_ops,
					       V4L2_CID_PIXEL_RATE,
					       1, INT_MAX, 1,
					       imx415_calc_pixel_rate(imx415));

	imx415->data_lanes = v4l2_ctrl_new_custom(&imx415->ctrls, &nlane_cfg, NULL);
	if (imx415->data_lanes)
		imx415->data_lanes->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	imx415->wdr = v4l2_ctrl_new_custom(&imx415->ctrls, &wdr_cfg, NULL);

	v4l2_ctrl_new_custom(&imx415->ctrls, &fps_cfg, NULL);

	imx415->sd.ctrl_handler = &imx415->ctrls;

	if (imx415->ctrls.error) {
		dev_err(imx415->dev, "Control initialization a error  %d\n",
			imx415->ctrls.error);
		rtn = imx415->ctrls.error;
	}

	return rtn;
}

static int imx415_register_subdev(struct imx415 *imx415)
{
	int rtn = 0;

	v4l2_i2c_subdev_init(&imx415->sd, imx415->client, &imx415_subdev_ops);

	imx415->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	imx415->sd.dev = &imx415->client->dev;
	imx415->sd.internal_ops = &imx415_internal_ops;
	imx415->sd.entity.ops = &imx415_subdev_entity_ops;
	imx415->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;
	snprintf(imx415->sd.name, sizeof(imx415->sd.name), AML_SENSOR_NAME, imx415->index);

	imx415->pad.flags = MEDIA_PAD_FL_SOURCE;
	rtn = media_entity_pads_init(&imx415->sd.entity, 1, &imx415->pad);
	if (rtn < 0) {
		dev_err(imx415->dev, "Could not register media entity\n");
		goto err_return;
	}

	rtn = v4l2_async_register_subdev(&imx415->sd);
	if (rtn < 0) {
		dev_err(imx415->dev, "Could not register v4l2 device\n");
		goto err_return;
	}

err_return:
	return rtn;
}

int imx415_init(struct i2c_client *client, void *sdrv)
{
	struct device *dev = &client->dev;
	struct imx415 *imx415;
	struct amlsens *sensor = (struct amlsens *)sdrv;
	int ret = -EINVAL;

	imx415 = devm_kzalloc(dev, sizeof(*imx415), GFP_KERNEL);
	if (!imx415)
		return -ENOMEM;

	imx415->dev = dev;
	imx415->client = client;
	imx415->client->addr = IMX415_SLAVE_ID;
	imx415->gpio = &sensor->gpio;

	imx415->regmap = devm_regmap_init_i2c(client, &imx415_regmap_config);
	if (IS_ERR(imx415->regmap)) {
		dev_err(dev, "Unable to initialize I2C\n");
		return -ENODEV;
	}

	if (of_property_read_u32(dev->of_node, "index", &imx415->index)) {
		dev_err(dev, "Failed to read sensor index. default to 0\n");
		imx415->index = 0;
	}

	mutex_init(&imx415->lock);

	/* Power on the device to match runtime PM state below */
	ret = imx415_power_on(imx415->dev, imx415->gpio);
	if (ret < 0) {
		dev_err(dev, "Could not power on the device\n");
		return -ENODEV;
	}

	/*
	 * Initialize the frame format. In particular, imx415->current_mode
	 * and imx415->bpp are set to defaults: imx415_calc_pixel_rate() call
	 * below in imx415_ctrls_init relies on these fields.
	 */
	imx415_entity_init_cfg(&imx415->sd, NULL);

	ret = imx415_ctrls_init(imx415);
	if (ret) {
		dev_err(imx415->dev, "Error ctrls init\n");
		goto free_ctrl;
	}

	ret = imx415_register_subdev(imx415);
	if (ret) {
		dev_err(imx415->dev, "Error register subdev\n");
		goto free_entity;
	}

	dev_err(imx415->dev, "imx415 probe done\n");

	return 0;

free_entity:
	media_entity_cleanup(&imx415->sd.entity);
free_ctrl:
	v4l2_ctrl_handler_free(&imx415->ctrls);
	mutex_destroy(&imx415->lock);

	return ret;
}

int imx415_deinit(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct imx415 *imx415 = to_imx415(sd);

	v4l2_async_unregister_subdev(sd);
	media_entity_cleanup(&sd->entity);
	v4l2_ctrl_handler_free(sd->ctrl_handler);

	mutex_destroy(&imx415->lock);

	imx415_power_off(imx415->dev, imx415->gpio);

	return 0;
}

int imx415_sensor_id(struct i2c_client *client)
{
	u32 id = 0;
	u8 val = 0;
	int rtn = -EINVAL;

	i2c_read_a16d8(client, IMX415_SLAVE_ID, 0x30d9, &val);
	id |= (val << 8);

	i2c_read_a16d8(client, IMX415_SLAVE_ID, 0x30da, &val);
	id |= val;

	if (id != IMX415_ID) {
		dev_info(&client->dev, "Failed to get imx415 id: 0x%x\n", id);
		return rtn;
	} else {
		dev_err(&client->dev, "success get imx415 id 0x%x", id);
	}

	return 0;
}
