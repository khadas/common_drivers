// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2023 Amlogic, Inc. All rights reserved.
 */

#include "sensor_drv.h"
#include "i2c_api.h"
#include "mclk_api.h"
#include "aml_ov5640.h"

#define AML_SENSOR_NAME  "ov5640-%u"

/* supported link frequencies */
#define FREQ_INDEX_1080P	0
#define FREQ_INDEX_5MP		1

/* supported link frequencies */
static const s64 ov5640_link_freq_2lanes[] = {
	[FREQ_INDEX_1080P] = 672000000,
	[FREQ_INDEX_5MP] = 672000000,
};

static inline const s64 *ov5640_link_freqs_ptr(const struct ov5640 *ov5640)
{
	return ov5640_link_freq_2lanes;
}

static inline int ov5640_link_freqs_num(const struct ov5640 *ov5640)
{
	return ARRAY_SIZE(ov5640_link_freq_2lanes);
}

/* Mode configs */
static const struct ov5640_mode ov5640_modes_2lanes[] = {
	{
		.width = 1920,
		.height = 1080,
		.hmax  = 0x1130,
		.data = setting_1920_1080_2lane_672m_30fps,
		.data_size = ARRAY_SIZE(setting_1920_1080_2lane_672m_30fps),

		.link_freq_index = FREQ_INDEX_1080P,
	},
	{
		.width = 2592,
		.height = 1944,
		.hmax = 0x19c8,
		.data = setting_2592_1944_2lane_672m_30fps,
		.data_size = ARRAY_SIZE(setting_2592_1944_2lane_672m_30fps),

		.link_freq_index = FREQ_INDEX_5MP,
	},
};

static inline const struct ov5640_mode *ov5640_modes_ptr(const struct ov5640 *ov5640)
{
	return ov5640_modes_2lanes;
}

static inline int ov5640_modes_num(const struct ov5640 *ov5640)
{
	return ARRAY_SIZE(ov5640_modes_2lanes);
}

static inline struct ov5640 *to_ov5640(struct v4l2_subdev *_sd)
{
	return container_of(_sd, struct ov5640, sd);
}

static inline int ov5640_read_reg(struct ov5640 *ov5640, u16 addr, u8 *value)
{
	unsigned int regval;

	int i, ret;

	for (i = 0; i < 3; ++i) {
		ret = regmap_read(ov5640->regmap, addr, &regval);
		if (0 == ret ) {
			break;
		}
	}

	if (ret)
		dev_err(ov5640->dev, "I2C read with i2c transfer failed for addr: %x, ret %d\n", addr, ret);

	*value = regval & 0xff;
	return 0;
}

static int ov5640_write_reg(struct ov5640 *ov5640, u16 addr, u8 value)
{
	int i, ret;

	for (i = 0; i < 3; i++) {
		ret = regmap_write(ov5640->regmap, addr, value);
		if (0 == ret) {
			break;
		}
	}

	if (ret)
		dev_err(ov5640->dev, "I2C write failed for addr: %x, ret %d\n", addr, ret);

	return ret;
}

static int ov5640_set_register_array(struct ov5640 *ov5640,
				     const struct ov5640_regval *settings,
				     unsigned int num_settings)
{
	unsigned int i;
	int ret = 0;

	for (i = 0; i < num_settings; ++i, ++settings) {
		ret = ov5640_write_reg(ov5640, settings->reg, settings->val);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int ov5640_write_buffered_reg(struct ov5640 *ov5640, u16 address_low,
				     u8 nr_regs, u32 value)
{
	unsigned int i;
	int ret;

	for (i = 0; i < nr_regs; i++) {
		ret = ov5640_write_reg(ov5640, address_low + i,
				       (u8)(value >> (i * 8)));
		if (ret) {
			dev_err(ov5640->dev, "Error writing buffered registers\n");
			return ret;
		}
	}

	return ret;
}

static int ov5640_set_gain(struct ov5640 *ov5640, u32 value)
{
	int ret;

	ret = ov5640_write_buffered_reg(ov5640, OV5640_GAIN, 1, value);
	if (ret)
		dev_err(ov5640->dev, "Unable to write gain\n");

	return ret;
}

static int ov5640_set_exposure(struct ov5640 *ov5640, u32 value)
{
	int ret;

	ret = ov5640_write_buffered_reg(ov5640, OV5640_EXPOSURE, 2, value);
	if (ret)
		dev_err(ov5640->dev, "Unable to write gain\n");

	return ret;
}


/* Stop streaming */
static int ov5640_stop_streaming(struct ov5640 *ov5640)
{
	ov5640->enWDRMode = WDR_MODE_NONE;
	return ov5640_write_reg(ov5640, 0x300e, 0x41);
}

static int ov5640_set_ctrl(struct v4l2_ctrl *ctrl)
{
	struct ov5640 *ov5640 = container_of(ctrl->handler,
					     struct ov5640, ctrls);
	int ret = 0;

	/* V4L2 controls values will be applied only when power is already up */
	if (!pm_runtime_get_if_in_use(ov5640->dev))
		return 0;

	switch (ctrl->id) {
	case V4L2_CID_GAIN:
		ret = ov5640_set_gain(ov5640, ctrl->val);
		break;
	case V4L2_CID_EXPOSURE:
		ret = ov5640_set_exposure(ov5640, ctrl->val);
		break;
	case V4L2_CID_HBLANK:
		break;
	case V4L2_CID_AML_CSI_LANES:
		break;
	case V4L2_CID_AML_MODE:
		ov5640->enWDRMode = ctrl->val;
		break;
	default:
		dev_err(ov5640->dev, "Error ctrl->id %u, flag 0x%lx\n",
			ctrl->id, ctrl->flags);
		ret = -EINVAL;
		break;
	}

	pm_runtime_put(ov5640->dev);

	return ret;
}

static const struct v4l2_ctrl_ops ov5640_ctrl_ops = {
	.s_ctrl = ov5640_set_ctrl,
};
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int ov5640_enum_mbus_code(struct v4l2_subdev *sd,
				  struct v4l2_subdev_state *cfg,
				 struct v4l2_subdev_mbus_code_enum *code)
#else
static int ov5640_enum_mbus_code(struct v4l2_subdev *sd,
				 struct v4l2_subdev_pad_config *cfg,
				 struct v4l2_subdev_mbus_code_enum *code)
#endif
{
	if (code->index >= ARRAY_SIZE(ov5640_formats))
		return -EINVAL;

	code->code = ov5640_formats[code->index].code;

	return 0;
}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int ov5640_enum_frame_size(struct v4l2_subdev *sd,
			        struct v4l2_subdev_state *cfg,
			       struct v4l2_subdev_frame_size_enum *fse)
#else
static int ov5640_enum_frame_size(struct v4l2_subdev *sd,
			       struct v4l2_subdev_pad_config *cfg,
			       struct v4l2_subdev_frame_size_enum *fse)
#endif
{
	if (fse->index >= ARRAY_SIZE(ov5640_formats))
		return -EINVAL;

	fse->min_width = ov5640_formats[fse->index].min_width;
	fse->min_height = ov5640_formats[fse->index].min_height;;
	fse->max_width = ov5640_formats[fse->index].max_width;
	fse->max_height = ov5640_formats[fse->index].max_height;

	return 0;
}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int ov5640_get_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_state *cfg,
			  struct v4l2_subdev_format *fmt)
#else
static int ov5640_get_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *fmt)
#endif
{
	struct ov5640 *ov5640 = to_ov5640(sd);
	struct v4l2_mbus_framefmt *framefmt;

	mutex_lock(&ov5640->lock);

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY)
		framefmt = v4l2_subdev_get_try_format(&ov5640->sd, cfg,
						      fmt->pad);
	else
		framefmt = &ov5640->current_format;

	fmt->format = *framefmt;

	mutex_unlock(&ov5640->lock);

	return 0;
}


static inline u8 ov5640_get_link_freq_index(struct ov5640 *ov5640)
{
	return ov5640->current_mode->link_freq_index;
}

static s64 ov5640_get_link_freq(struct ov5640 *ov5640)
{
	u8 index = ov5640_get_link_freq_index(ov5640);

	return *(ov5640_link_freqs_ptr(ov5640) + index);
}

static u64 ov5640_calc_pixel_rate(struct ov5640 *ov5640)
{
	s64 link_freq = ov5640_get_link_freq(ov5640);
	u8 nlanes = ov5640->nlanes;
	u64 pixel_rate;

	/* pixel rate = link_freq * 2 * nr_of_lanes / bits_per_sample */
	pixel_rate = link_freq * 2 * nlanes;
	do_div(pixel_rate, ov5640->bpp);
	return pixel_rate;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int ov5640_set_fmt(struct v4l2_subdev *sd,
			struct v4l2_subdev_state *cfg,
			struct v4l2_subdev_format *fmt)
#else
static int ov5640_set_fmt(struct v4l2_subdev *sd,
			struct v4l2_subdev_pad_config *cfg,
			struct v4l2_subdev_format *fmt)
#endif
{
	struct ov5640 *ov5640 = to_ov5640(sd);
	const struct ov5640_mode *mode;
	struct v4l2_mbus_framefmt *format;
	unsigned int i,ret;

	mutex_lock(&ov5640->lock);

	mode = v4l2_find_nearest_size(ov5640_modes_ptr(ov5640),
				 ov5640_modes_num(ov5640),
				width, height,
				fmt->format.width, fmt->format.height);

	fmt->format.width = mode->width;
	fmt->format.height = mode->height;

	for (i = 0; i < ARRAY_SIZE(ov5640_formats); i++) {
		if (ov5640_formats[i].code == fmt->format.code) {
			dev_info(ov5640->dev, "find proper format %d \n",i);
			break;
		}
	}

	if (i >= ARRAY_SIZE(ov5640_formats)) {
		i = 0;
		dev_err(ov5640->dev, "No format. reset i = 0 \n");
	}

	fmt->format.code = ov5640_formats[i].code;
	fmt->format.field = V4L2_FIELD_NONE;

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		dev_info(ov5640->dev, "try format \n");
		format = v4l2_subdev_get_try_format(sd, cfg, fmt->pad);
		mutex_unlock(&ov5640->lock);
		return 0;
	} else {
		dev_err(ov5640->dev, "set format, w %d, h %d, code 0x%x \n",
            fmt->format.width, fmt->format.height,
            fmt->format.code);
		format = &ov5640->current_format;
		ov5640->current_mode = mode;
		ov5640->bpp = ov5640_formats[i].bpp;
		ov5640->nlanes = 2;

		if (ov5640->link_freq)
			__v4l2_ctrl_s_ctrl(ov5640->link_freq, ov5640_get_link_freq_index(ov5640) );
		if (ov5640->pixel_rate)
			__v4l2_ctrl_s_ctrl_int64(ov5640->pixel_rate, ov5640_calc_pixel_rate(ov5640) );
		if (ov5640->data_lanes)
			__v4l2_ctrl_s_ctrl(ov5640->data_lanes, ov5640->nlanes);
	}

	*format = fmt->format;
	ov5640->status = 0;

	mutex_unlock(&ov5640->lock);

	/* Set init register settings */
	ret = ov5640_set_register_array(ov5640, setting_2592_1944_2lane_672m_30fps,
		ARRAY_SIZE(setting_2592_1944_2lane_672m_30fps));
	if (ret < 0) {
		dev_err(ov5640->dev, "Could not set init registers\n");
		return ret;
	} else
		dev_err(ov5640->dev, "ov5640 linear mode init\n");

	return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
int ov5640_get_selection(struct v4l2_subdev *sd,
			     struct v4l2_subdev_state *cfg,
			     struct v4l2_subdev_selection *sel)
#else
int ov5640_get_selection(struct v4l2_subdev *sd,
			     struct v4l2_subdev_pad_config *cfg,
			     struct v4l2_subdev_selection *sel)
#endif
{
	int rtn = 0;
	struct ov5640 *ov5640 = to_ov5640(sd);
	const struct ov5640_mode *mode = ov5640->current_mode;

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
		dev_err(ov5640->dev, "Error support target: 0x%x\n", sel->target);
	break;
	}

	return rtn;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int ov5640_entity_init_cfg(struct v4l2_subdev *subdev,
				  struct v4l2_subdev_state *cfg)
#else
static int ov5640_entity_init_cfg(struct v4l2_subdev *subdev,
				  struct v4l2_subdev_pad_config *cfg)
#endif
{
	struct v4l2_subdev_format fmt = { 0 };

	fmt.which = cfg ? V4L2_SUBDEV_FORMAT_TRY : V4L2_SUBDEV_FORMAT_ACTIVE;
	fmt.format.width = 2592;
	fmt.format.height = 1944;
	fmt.format.code = MEDIA_BUS_FMT_YUYV8_2X8;

	ov5640_set_fmt(subdev, cfg, &fmt);

	return 0;
}

/* Start streaming */
static int ov5640_start_streaming(struct ov5640 *ov5640)
{
	/* Start streaming */
	return ov5640_write_reg(ov5640, 0x300e, 0x45);
}

static int ov5640_set_stream(struct v4l2_subdev *sd, int enable)
{
	struct ov5640 *ov5640 = to_ov5640(sd);
	int ret = 0;

	if (ov5640->status == enable)
		return ret;
	else
		ov5640->status = enable;

	if (enable) {
		ret = ov5640_start_streaming(ov5640);
		if (ret) {
			dev_err(ov5640->dev, "Start stream failed\n");
			goto unlock_and_return;
		}

		dev_info(ov5640->dev, "stream on\n");
	} else {
		ov5640_stop_streaming(ov5640);

		dev_info(ov5640->dev, "stream off\n");
	}

unlock_and_return:

	return ret;
}

int ov5640_power_on(struct device *dev, struct sensor_gpio *gpio)
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

int ov5640_power_off(struct device *dev, struct sensor_gpio *gpio)
{
	mclk_disable(dev);

	gpiod_set_value_cansleep(gpio->rst_gpio, 0);
	if (!IS_ERR_OR_NULL(gpio->pwdn_gpio)) {
		gpiod_set_value_cansleep(gpio->pwdn_gpio, 0);
	}
	return 0;
}

int ov5640_power_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ov5640 *ov5640 = to_ov5640(sd);

	gpiod_set_value_cansleep(ov5640->gpio->rst_gpio, 0);

	return 0;
}

int ov5640_power_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ov5640 *ov5640 = to_ov5640(sd);

	gpiod_set_value_cansleep(ov5640->gpio->rst_gpio, 1);

	return 0;
}

static int ov5640_log_status(struct v4l2_subdev *sd)
{
	struct ov5640 *ov5640 = to_ov5640(sd);

	dev_info(ov5640->dev, "log status done\n");

	return 0;
}

int ov5640_sbdev_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh) {
	struct ov5640 *ov5640 = to_ov5640(sd);
	ov5640_power_on(ov5640->dev, ov5640->gpio);
	return 0;
}

int ov5640_sbdev_close(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh) {
	struct ov5640 *ov5640 = to_ov5640(sd);
	ov5640_stop_streaming(ov5640);
	ov5640_power_off(ov5640->dev, ov5640->gpio);
	return 0;
}


static const struct dev_pm_ops ov5640_pm_ops = {
	SET_RUNTIME_PM_OPS(ov5640_power_suspend, ov5640_power_resume, NULL)
};

const struct v4l2_subdev_core_ops ov5640_core_ops = {
	.log_status = ov5640_log_status,
};

static const struct v4l2_subdev_video_ops ov5640_video_ops = {
	.s_stream = ov5640_set_stream,
};

static const struct v4l2_subdev_pad_ops ov5640_pad_ops = {
	.init_cfg = ov5640_entity_init_cfg,
	.enum_mbus_code = ov5640_enum_mbus_code,
	.enum_frame_size = ov5640_enum_frame_size,
	.get_selection = ov5640_get_selection,
	.get_fmt = ov5640_get_fmt,
	.set_fmt = ov5640_set_fmt,
};

static struct v4l2_subdev_internal_ops ov5640_internal_ops = {
	.open = ov5640_sbdev_open,
	.close = ov5640_sbdev_close,
};

static const struct v4l2_subdev_ops ov5640_subdev_ops = {
	.core = &ov5640_core_ops,
	.video = &ov5640_video_ops,
	.pad = &ov5640_pad_ops,
};

static const struct media_entity_operations ov5640_subdev_entity_ops = {
	.link_validate = v4l2_subdev_link_validate,
};

static struct v4l2_ctrl_config wdr_cfg = {
	.ops = &ov5640_ctrl_ops,
	.id = V4L2_CID_AML_MODE,
	.name = "wdr mode",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.min = 0,
	.max = 2,
	.step = 1,
	.def = 0,
};

static struct v4l2_ctrl_config nlane_cfg = {
	.ops = &ov5640_ctrl_ops,
	.id = V4L2_CID_AML_CSI_LANES,
	.name = "sensor lanes",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.flags = V4L2_CTRL_FLAG_VOLATILE,
	.min = 1,
	.max = 4,
	.step = 1,
	.def = 2,
};

static int ov5640_ctrls_init(struct ov5640 *ov5640)
{
	int rtn = 0;

	v4l2_ctrl_handler_init(&ov5640->ctrls, 4);

	v4l2_ctrl_new_std(&ov5640->ctrls, &ov5640_ctrl_ops,
				V4L2_CID_GAIN, 0, 0xF0, 1, 0);

	v4l2_ctrl_new_std(&ov5640->ctrls, &ov5640_ctrl_ops,
				V4L2_CID_EXPOSURE, 0, 0xffff, 1, 0);

	ov5640->link_freq = v4l2_ctrl_new_int_menu(&ov5640->ctrls,
					       &ov5640_ctrl_ops,
					       V4L2_CID_LINK_FREQ,
					       ov5640_link_freqs_num(ov5640) - 1,
					       0, ov5640_link_freqs_ptr(ov5640) );

	if (ov5640->link_freq)
		ov5640->link_freq->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	ov5640->pixel_rate = v4l2_ctrl_new_std(&ov5640->ctrls,
					       &ov5640_ctrl_ops,
					       V4L2_CID_PIXEL_RATE,
					       1, INT_MAX, 1,
					       ov5640_calc_pixel_rate(ov5640));

	ov5640->data_lanes = v4l2_ctrl_new_custom(&ov5640->ctrls, &nlane_cfg, NULL);
	if (ov5640->data_lanes)
		ov5640->data_lanes->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	ov5640->wdr = v4l2_ctrl_new_custom(&ov5640->ctrls, &wdr_cfg, NULL);

	ov5640->sd.ctrl_handler = &ov5640->ctrls;

	if (ov5640->ctrls.error) {
		dev_err(ov5640->dev, "Control initialization a error  %d\n",
			ov5640->ctrls.error);
		rtn = ov5640->ctrls.error;
	}

	return rtn;
}

static int ov5640_register_subdev(struct ov5640 *ov5640)
{
	int rtn = 0;

	v4l2_i2c_subdev_init(&ov5640->sd, ov5640->client, &ov5640_subdev_ops);

	ov5640->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	ov5640->sd.dev = &ov5640->client->dev;
	ov5640->sd.internal_ops = &ov5640_internal_ops;
	ov5640->sd.entity.ops = &ov5640_subdev_entity_ops;
	ov5640->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;
	snprintf(ov5640->sd.name, sizeof(ov5640->sd.name), AML_SENSOR_NAME, ov5640->index);

	ov5640->pad.flags = MEDIA_PAD_FL_SOURCE;
	rtn = media_entity_pads_init(&ov5640->sd.entity, 1, &ov5640->pad);
	if (rtn < 0) {
		dev_err(ov5640->dev, "Could not register media entity\n");
		goto err_return;
	}

	rtn = v4l2_async_register_subdev(&ov5640->sd);
	if (rtn < 0) {
		dev_err(ov5640->dev, "Could not register v4l2 device\n");
		goto err_return;
	}

err_return:
	return rtn;
}

int ov5640_init(struct i2c_client *client, void *sdrv)
{
	struct device *dev = &client->dev;
	struct ov5640 *ov5640;
	struct amlsens *sensor = (struct amlsens *)sdrv;
	int ret = -EINVAL;

	ov5640 = devm_kzalloc(dev, sizeof(*ov5640), GFP_KERNEL);
	if (!ov5640)
		return -ENOMEM;

	ov5640->dev = dev;
	ov5640->client = client;
	ov5640->client->addr = OV5640_SLAVE_ID;
	ov5640->gpio = &sensor->gpio;

	ov5640->regmap = devm_regmap_init_i2c(client, &ov5640_regmap_config);
	if (IS_ERR(ov5640->regmap)) {
		dev_err(dev, "Unable to initialize I2C\n");
		return -ENODEV;
	}

	if (of_property_read_u32(dev->of_node, "index", &ov5640->index)) {
		dev_err(dev, "Failed to read sensor index. default to 0\n");
		ov5640->index = 0;
	}

	mutex_init(&ov5640->lock);

	/* Power on the device to match runtime PM state below */
	ret = ov5640_power_on(ov5640->dev, ov5640->gpio);
	if (ret < 0) {
		dev_err(dev, "Could not power on the device\n");
		return -ENODEV;
	}

	/*
	 * Initialize the frame format. In particular, ov5640->current_mode
	 * and ov5640->bpp are set to defaults: ov5640_calc_pixel_rate() call
	 * below in ov5640_ctrls_init relies on these fields.
	 */
	ov5640_entity_init_cfg(&ov5640->sd, NULL);

	ret = ov5640_ctrls_init(ov5640);
	if (ret) {
		dev_err(ov5640->dev, "Error ctrls init\n");
		goto free_ctrl;
	}

	ret = ov5640_register_subdev(ov5640);
	if (ret) {
		dev_err(ov5640->dev, "Error register subdev\n");
		goto free_entity;
	}

	dev_err(ov5640->dev, "probe ov5640 done\n");

	return 0;

free_entity:
	media_entity_cleanup(&ov5640->sd.entity);
free_ctrl:
	v4l2_ctrl_handler_free(&ov5640->ctrls);
	mutex_destroy(&ov5640->lock);

	return ret;
}

int ov5640_deinit(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ov5640 *ov5640 = to_ov5640(sd);

	v4l2_async_unregister_subdev(sd);
	media_entity_cleanup(&sd->entity);
	v4l2_ctrl_handler_free(sd->ctrl_handler);

	mutex_destroy(&ov5640->lock);

	ov5640_power_off(ov5640->dev, ov5640->gpio);

	return 0;
}

int ov5640_sensor_id(struct i2c_client *client)
{
	u32 id = 0;
	u8 val = 0;
	int rtn = -EINVAL;

	i2c_read_a16d8(client, OV5640_SLAVE_ID, 0x300a, &val);
	id |= (val << 8);

	i2c_read_a16d8(client, OV5640_SLAVE_ID, 0x300b, &val);
	id |= val;

	if (id != OV5640_ID) {
		dev_info(&client->dev, "Failed to get ov5640 id: 0x%x\n", id);
		return rtn;
	} else {
		dev_err(&client->dev, "success get ov5640 id 0x%x", id);
	}

	return 0;
}
