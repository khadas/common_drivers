// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2023 Amlogic, Inc. All rights reserved.
 */

#include "sensor_drv.h"
#include "i2c_api.h"
#include "mclk_api.h"
#include "aml_imx335.h"

#define AML_SENSOR_NAME "imx335-%u"
#define IMX335_FREQ_INDEX_1944P	0

static const s64 imx335_link_freq_4lanes[] = {
	[IMX335_FREQ_INDEX_1944P] = 891000000,
};

static inline const s64 *imx335_link_freqs_ptr(const struct imx335 *imx335)
{
	return imx335_link_freq_4lanes;
}

static inline int imx335_link_freqs_num(const struct imx335 *imx335)
{
	return ARRAY_SIZE(imx335_link_freq_4lanes);
}

static const struct imx335_mode imx335_modes_4lanes[] = {
	{
		.width = 2592,
		.height = 1944,
		.hmax = 0x0226,
		.link_freq_index = IMX335_FREQ_INDEX_1944P,
		.data = imx335_1944p_linear_settings,
		.data_size = ARRAY_SIZE(imx335_1944p_linear_settings),
	},
};

static const struct imx335_mode *imx335_modes_ptr(const struct imx335 *imx335)
{
	return imx335_modes_4lanes;
}

static int imx335_modes_num(const struct imx335 *imx335)
{
	return ARRAY_SIZE(imx335_modes_4lanes);
}

static struct imx335 *to_imx335(struct v4l2_subdev *_sd)
{
	return container_of(_sd, struct imx335, sd);
}

static inline int imx335_read_reg(struct imx335 *imx335, u16 addr, u8 *value)
{
	int i, ret;
	u32 regval;

	for (i = 0; i < 3; ++i) {
		ret = regmap_read(imx335->regmap, addr, &regval);
		if (ret == 0)
			break;
	}

	if (ret)
		dev_err(imx335->dev, "I2C read with i2c transfer failed for addr: %x, ret %d\n", addr, ret);

	*value = regval & 0xff;

	return 0;
}

static int imx335_write_reg(struct imx335 *imx335, u16 addr, u8 value)
{
	int i, ret;

	for (i = 0; i < 3; i++) {
		ret = regmap_write(imx335->regmap, addr, value);
		if (ret == 0)
			break;
	}

	if (ret)
		dev_err(imx335->dev, "I2C write failed for addr: %x, ret %d\n", addr, ret);

	return ret;
}

static int imx335_set_register_array(struct imx335 *imx335,
				     const struct imx335_regval *settings,
				     unsigned int num_settings)
{
	unsigned int i;
	int ret = 0;

	for (i = 0; i < num_settings; ++i, ++settings) {
		ret = imx335_write_reg(imx335, settings->reg, settings->val);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int imx335_write_buffered_reg(struct imx335 *imx335, u16 address_low,
				     u8 nr_regs, u32 value)
{
	u8 val = 0;
	int ret = 0;
	unsigned int i;

	ret = imx335_write_reg(imx335, IMX335_REGHOLD, 0x01);
	if (ret) {
		dev_err(imx335->dev, "Error setting hold register\n");
		return ret;
	}

	for (i = 0; i < nr_regs; i++) {
		val = (u8)(value >> ((nr_regs - 1 - i) * 8));
		ret = imx335_write_reg(imx335, address_low + i, val);
		if (ret) {
			dev_err(imx335->dev, "Error writing buffered registers\n");
			return ret;
		}
	}

	ret = imx335_write_reg(imx335, IMX335_REGHOLD, 0x00);
	if (ret) {
		dev_err(imx335->dev, "Error setting hold register\n");
		return ret;
	}

	return ret;
}

static int imx335_set_gain(struct imx335 *imx335, u32 value)
{
	int ret;

	ret = imx335_write_buffered_reg(imx335, IMX335_GAIN, 2, value);
	if (ret)
		dev_err(imx335->dev, "Unable to write gain\n");

	return ret;
}

static int imx335_set_exposure(struct imx335 *imx335, u32 value)
{
	int ret;

	ret = imx335_write_buffered_reg(imx335, IMX335_EXPOSURE, 2, value & 0xFFFF);
	if (ret)
		dev_err(imx335->dev, "Unable to write gain\n");

	return ret;
}

static int imx335_set_fps(struct imx335 *imx335, u32 value)
{
	u32 vts = 0;
	u8 vts_h, vts_l;

	//dev_err(imx335->dev, "-imx335-value = %d\n", value);

	vts = 30 * 3488 / value;
	vts_h = (vts >> 8) & 0x7f;
	vts_l = vts & 0xff;

	imx335_write_reg(imx335, 0x3025, vts_h);
	imx335_write_reg(imx335, 0x3024, vts_l);

	return 0;
}

static int imx335_stop_streaming(struct imx335 *imx335)
{
	imx335->enWDRMode = WDR_MODE_NONE;

	return imx335_write_reg(imx335, IMX335_STANDBY, 0x01);
}

static int imx335_set_ctrl(struct v4l2_ctrl *ctrl)
{
	struct imx335 *imx335 = container_of(ctrl->handler, struct imx335, ctrls);
	int ret = 0;

	/* V4L2 controls values will be applied only when power is already up */
	if (!pm_runtime_get_if_in_use(imx335->dev))
		return 0;

	switch (ctrl->id)
	{
	case V4L2_CID_GAIN:
		ret = imx335_set_gain(imx335, ctrl->val);
		break;
	case V4L2_CID_EXPOSURE:
		ret = imx335_set_exposure(imx335, ctrl->val);
		break;
	case V4L2_CID_HBLANK:
		break;
	case V4L2_CID_AML_CSI_LANES:
		break;
	case V4L2_CID_AML_MODE:
		imx335->enWDRMode = ctrl->val;
		break;
	case V4L2_CID_AML_ORIG_FPS:
		ret = imx335_set_fps(imx335, ctrl->val);
		break;
	default:
		dev_err(imx335->dev, "Error ctrl->id %u, flag 0x%lx\n",
				ctrl->id, ctrl->flags);
		ret = -EINVAL;
		break;
	}

	pm_runtime_put(imx335->dev);

	return ret;
}

static const struct v4l2_ctrl_ops imx335_ctrl_ops = {
	.s_ctrl = imx335_set_ctrl,
};
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int imx335_enum_mbus_code(struct v4l2_subdev *sd,
				 struct v4l2_subdev_state *cfg,
				 struct v4l2_subdev_mbus_code_enum *code)
#else
static int imx335_enum_mbus_code(struct v4l2_subdev *sd,
				 struct v4l2_subdev_pad_config *cfg,
				 struct v4l2_subdev_mbus_code_enum *code)
#endif
{
	if (code->index >= ARRAY_SIZE(imx335_formats))
		return -EINVAL;

	code->code = imx335_formats[code->index].code;

	return 0;
}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int imx335_enum_frame_size(struct v4l2_subdev *sd,
				  struct v4l2_subdev_state *cfg,
				  struct v4l2_subdev_frame_size_enum *fse)
#else
static int imx335_enum_frame_size(struct v4l2_subdev *sd,
				  struct v4l2_subdev_pad_config *cfg,
				  struct v4l2_subdev_frame_size_enum *fse)
#endif
{
	if (fse->index >= ARRAY_SIZE(imx335_formats))
		return -EINVAL;

	fse->min_width = imx335_formats[fse->index].min_width;
	fse->min_height = imx335_formats[fse->index].min_height;
	;
	fse->max_width = imx335_formats[fse->index].max_width;
	fse->max_height = imx335_formats[fse->index].max_height;

	return 0;
}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int imx335_get_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_state *cfg,
			  struct v4l2_subdev_format *fmt)
#else
static int imx335_get_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *fmt)
#endif
{
	struct imx335 *imx335 = to_imx335(sd);
	struct v4l2_mbus_framefmt *framefmt;

	mutex_lock(&imx335->lock);

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY)
		framefmt = v4l2_subdev_get_try_format(&imx335->sd, cfg, fmt->pad);
	else
		framefmt = &imx335->current_format;

	fmt->format = *framefmt;

	mutex_unlock(&imx335->lock);

	return 0;
}

static inline u8 imx335_get_link_freq_index(struct imx335 *imx335)
{
	return imx335->current_mode->link_freq_index;
}

static s64 imx335_get_link_freq(struct imx335 *imx335)
{
	u8 index = imx335_get_link_freq_index(imx335);

	return *(imx335_link_freqs_ptr(imx335) + index);
}

static u64 imx335_calc_pixel_rate(struct imx335 *imx335)
{
	u64 pixel_rate;
	s64 link_freq = imx335_get_link_freq(imx335);
	u8 nlanes = imx335->nlanes;

	pixel_rate = link_freq * 2 * nlanes;
	do_div(pixel_rate, imx335->bpp);

	return pixel_rate;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int imx335_set_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_state *cfg,
			  struct v4l2_subdev_format *fmt)
#else
static int imx335_set_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *fmt)
#endif
{
	unsigned int i;
	struct imx335 *imx335 = to_imx335(sd);
	const struct imx335_mode *mode;
	struct v4l2_mbus_framefmt *format;

	mutex_lock(&imx335->lock);

	mode = v4l2_find_nearest_size(imx335_modes_ptr(imx335),
					imx335_modes_num(imx335),
					width, height,
					fmt->format.width, fmt->format.height);

	fmt->format.width = mode->width;
	fmt->format.height = mode->height;

	for (i = 0; i < ARRAY_SIZE(imx335_formats); i++) {
		if (imx335_formats[i].code == fmt->format.code) {
			dev_info(imx335->dev, "find proper format %d \n", i);
			break;
		}
	}

	if (i >= ARRAY_SIZE(imx335_formats)) {
		i = 0;
		dev_err(imx335->dev, "No format. reset i = 0 \n");
	}

	fmt->format.code = imx335_formats[i].code;
	fmt->format.field = V4L2_FIELD_NONE;

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		dev_info(imx335->dev, "try format \n");
		format = v4l2_subdev_get_try_format(sd, cfg, fmt->pad);
		mutex_unlock(&imx335->lock);

		return 0;
	} else {
		dev_err(imx335->dev, "set format, w %d, h %d, code 0x%x \n",
				fmt->format.width, fmt->format.height,
				fmt->format.code);
		format = &imx335->current_format;
		imx335->current_mode = mode;
		imx335->bpp = imx335_formats[i].bpp;
		imx335->nlanes = 4;

		if (imx335->link_freq)
			__v4l2_ctrl_s_ctrl(imx335->link_freq, imx335_get_link_freq_index(imx335));
		if (imx335->pixel_rate)
			__v4l2_ctrl_s_ctrl_int64(imx335->pixel_rate, imx335_calc_pixel_rate(imx335));
		if (imx335->data_lanes)
			__v4l2_ctrl_s_ctrl(imx335->data_lanes, imx335->nlanes);
	}

	*format = fmt->format;
	imx335->status = 0;

	imx335_set_register_array(imx335, mode->data, mode->data_size);

	mutex_unlock(&imx335->lock);

	return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
int imx335_get_selection(struct v4l2_subdev *sd,
			 struct v4l2_subdev_state *cfg,
			 struct v4l2_subdev_selection *sel)
#else
int imx335_get_selection(struct v4l2_subdev *sd,
			 struct v4l2_subdev_pad_config *cfg,
			 struct v4l2_subdev_selection *sel)
#endif
{
	int rtn = 0;
	struct imx335 *imx335 = to_imx335(sd);
	const struct imx335_mode *mode = imx335->current_mode;

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
		dev_err(imx335->dev, "Error support target: 0x%x\n", sel->target);
		break;
	}

	return rtn;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int imx335_entity_init_cfg(struct v4l2_subdev *subdev,
				  struct v4l2_subdev_state *cfg)
#else
static int imx335_entity_init_cfg(struct v4l2_subdev *subdev,
				  struct v4l2_subdev_pad_config *cfg)
#endif
{
	struct v4l2_subdev_format fmt = {0};

	fmt.which = cfg ? V4L2_SUBDEV_FORMAT_TRY : V4L2_SUBDEV_FORMAT_ACTIVE;
	fmt.format.width = 2592;
	fmt.format.height = 1944;
	fmt.format.code = MEDIA_BUS_FMT_SRGGB12_1X12;

	imx335_set_fmt(subdev, cfg, &fmt);

	return 0;
}

static int imx335_start_streaming(struct imx335 *imx335)
{
	return imx335_write_reg(imx335, IMX335_STANDBY, 0x00);
}

static int imx335_set_stream(struct v4l2_subdev *sd, int enable)
{
	int ret = 0;
	struct imx335 *imx335 = to_imx335(sd);

	if (imx335->status == enable)
		return ret;
	else
		imx335->status = enable;

	if (enable) {
		ret = imx335_start_streaming(imx335);
		if (ret) {
			dev_err(imx335->dev, "Start stream failed\n");
			goto unlock_and_return;
		}

		dev_info(imx335->dev, "stream on\n");
	} else {
		imx335_stop_streaming(imx335);

		dev_info(imx335->dev, "stream off\n");
	}

unlock_and_return:

	return ret;
}

int imx335_power_on(struct device *dev, struct sensor_gpio *gpio)
{
	int ret;

	gpiod_set_value_cansleep(gpio->rst_gpio, 1);
	if (!IS_ERR_OR_NULL(gpio->pwdn_gpio)) {
		gpiod_set_value_cansleep(gpio->pwdn_gpio, 1);
	}
	ret = mclk_enable(dev, 37125000);
	if (ret < 0)
		dev_err(dev, "set mclk fail\n");

	usleep_range(300, 310);

	return 0;
}

int imx335_power_off(struct device *dev, struct sensor_gpio *gpio)
{
	mclk_disable(dev);

	gpiod_set_value_cansleep(gpio->rst_gpio, 0);
	if (!IS_ERR_OR_NULL(gpio->pwdn_gpio)) {
		gpiod_set_value_cansleep(gpio->pwdn_gpio, 0);
	}
	return 0;
}

int imx335_power_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct imx335 *imx335 = to_imx335(sd);

	gpiod_set_value_cansleep(imx335->gpio->rst_gpio, 0);

	return 0;
}

int imx335_power_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct imx335 *imx335 = to_imx335(sd);

	gpiod_set_value_cansleep(imx335->gpio->rst_gpio, 1);

	return 0;
}

static int imx335_log_status(struct v4l2_subdev *sd)
{
	struct imx335 *imx335 = to_imx335(sd);

	dev_info(imx335->dev, "log status done\n");

	return 0;
}

static int imx335_sbdev_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct imx335 *imx335 = to_imx335(sd);

	imx335_power_on(imx335->dev, imx335->gpio);

	return 0;
}
static int imx335_sbdev_close(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct imx335 *imx335 = to_imx335(sd);

	imx335_set_stream(sd, 0);
	imx335_power_off(imx335->dev, imx335->gpio);

	return 0;
}

static const struct dev_pm_ops imx335_pm_ops = {
	SET_RUNTIME_PM_OPS(imx335_power_suspend, imx335_power_resume, NULL)};

const struct v4l2_subdev_core_ops imx335_core_ops = {
	.log_status = imx335_log_status,
};

static const struct v4l2_subdev_video_ops imx335_video_ops = {
	.s_stream = imx335_set_stream,
};

static const struct v4l2_subdev_pad_ops imx335_pad_ops = {
	.init_cfg = imx335_entity_init_cfg,
	.enum_mbus_code = imx335_enum_mbus_code,
	.enum_frame_size = imx335_enum_frame_size,
	.get_selection = imx335_get_selection,
	.get_fmt = imx335_get_fmt,
	.set_fmt = imx335_set_fmt,
};

static struct v4l2_subdev_internal_ops imx335_internal_ops = {
	.open = imx335_sbdev_open,
	.close = imx335_sbdev_close,
};

static const struct v4l2_subdev_ops imx335_subdev_ops = {
	.core = &imx335_core_ops,
	.video = &imx335_video_ops,
	.pad = &imx335_pad_ops,
};

static const struct media_entity_operations imx335_subdev_entity_ops = {
	.link_validate = v4l2_subdev_link_validate,
};

static struct v4l2_ctrl_config wdr_cfg = {
	.ops = &imx335_ctrl_ops,
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
	.ops = &imx335_ctrl_ops,
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
	.ops = &imx335_ctrl_ops,
	.id = V4L2_CID_AML_CSI_LANES,
	.name = "sensor lanes",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.flags = V4L2_CTRL_FLAG_VOLATILE,
	.min = 1,
	.max = 4,
	.step = 1,
	.def = 4,
};

static int imx335_ctrls_init(struct imx335 *imx335)
{
	int rtn = 0;

	v4l2_ctrl_handler_init(&imx335->ctrls, 7);

	v4l2_ctrl_new_std(&imx335->ctrls, &imx335_ctrl_ops,
				V4L2_CID_GAIN, 0, 0x7FF, 1, 0);

	v4l2_ctrl_new_std(&imx335->ctrls, &imx335_ctrl_ops,
				V4L2_CID_EXPOSURE, 0, 0x7fffffff, 1, 0);

	imx335->link_freq = v4l2_ctrl_new_int_menu(&imx335->ctrls,
						&imx335_ctrl_ops,
						V4L2_CID_LINK_FREQ,
						imx335_link_freqs_num(imx335) - 1,
						0, imx335_link_freqs_ptr(imx335));

	if (imx335->link_freq)
		imx335->link_freq->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	imx335->pixel_rate = v4l2_ctrl_new_std(&imx335->ctrls,
						&imx335_ctrl_ops,
						V4L2_CID_PIXEL_RATE,
						1, INT_MAX, 1,
						imx335_calc_pixel_rate(imx335));

	imx335->data_lanes = v4l2_ctrl_new_custom(&imx335->ctrls, &nlane_cfg, NULL);
	if (imx335->data_lanes)
		imx335->data_lanes->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	imx335->wdr = v4l2_ctrl_new_custom(&imx335->ctrls, &wdr_cfg, NULL);
	v4l2_ctrl_new_custom(&imx335->ctrls, &fps_cfg, NULL);

	imx335->sd.ctrl_handler = &imx335->ctrls;

	if (imx335->ctrls.error) {
		dev_err(imx335->dev, "Control initialization a error  %d\n", imx335->ctrls.error);
		rtn = imx335->ctrls.error;
	}

	return rtn;
}

static int imx335_register_subdev(struct imx335 *imx335)
{
	int rtn = 0;

	v4l2_i2c_subdev_init(&imx335->sd, imx335->client, &imx335_subdev_ops);

	imx335->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	imx335->sd.dev = &imx335->client->dev;
	imx335->sd.internal_ops = &imx335_internal_ops;
	imx335->sd.entity.ops = &imx335_subdev_entity_ops;
	imx335->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;
	snprintf(imx335->sd.name, sizeof(imx335->sd.name), AML_SENSOR_NAME, imx335->index);

	imx335->pad.flags = MEDIA_PAD_FL_SOURCE;
	rtn = media_entity_pads_init(&imx335->sd.entity, 1, &imx335->pad);
	if (rtn < 0) {
		dev_err(imx335->dev, "Could not register media entity\n");
		goto err_return;
	}

	rtn = v4l2_async_register_subdev(&imx335->sd);
	if (rtn < 0) {
		dev_err(imx335->dev, "Could not register v4l2 device\n");
		goto err_return;
	}

err_return:
	return rtn;
}

int imx335_init(struct i2c_client *client, void *sdrv)
{
	struct device *dev = &client->dev;
	struct imx335 *imx335;
	struct amlsens *sensor = (struct amlsens *)sdrv;
	int ret = -EINVAL;

	imx335 = devm_kzalloc(dev, sizeof(*imx335), GFP_KERNEL);
	if (!imx335)
		return -ENOMEM;

	imx335->dev = dev;
	imx335->client = client;
	imx335->client->addr = IMX335_SLAVE_ID;
	imx335->gpio = &sensor->gpio;

	imx335->regmap = devm_regmap_init_i2c(client, &imx335_regmap_config);
	if (IS_ERR(imx335->regmap)) {
		dev_err(dev, "Unable to initialize I2C\n");
		return -ENODEV;
	}

	if (of_property_read_u32(dev->of_node, "index", &imx335->index)) {
		dev_err(dev, "Failed to read sensor index. default to 0\n");
		imx335->index = 0;
	}

	mutex_init(&imx335->lock);

	/* Power on the device to match runtime PM state below */
	ret = imx335_power_on(imx335->dev, imx335->gpio);
	if (ret < 0) {
		dev_err(dev, "Could not power on the device\n");
		return -ENODEV;
	}

	/*
	 * Initialize the frame format. In particular, imx335->current_mode
	 * and imx335->bpp are set to defaults: imx335_calc_pixel_rate() call
	 * below in imx335_ctrls_init relies on these fields.
	 */
	imx335_entity_init_cfg(&imx335->sd, NULL);

	ret = imx335_ctrls_init(imx335);
	if (ret) {
		dev_err(imx335->dev, "Error ctrls init\n");
		goto free_ctrl;
	}

	ret = imx335_register_subdev(imx335);
	if (ret) {
		dev_err(imx335->dev, "Error register subdev\n");
		goto free_entity;
	}

	dev_err(imx335->dev, "imx335 probe done\n");

	return 0;

free_entity:
	media_entity_cleanup(&imx335->sd.entity);
free_ctrl:
	v4l2_ctrl_handler_free(&imx335->ctrls);
	mutex_destroy(&imx335->lock);

	return ret;
}

int imx335_deinit(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct imx335 *imx335 = to_imx335(sd);

	v4l2_async_unregister_subdev(sd);
	media_entity_cleanup(&sd->entity);
	v4l2_ctrl_handler_free(sd->ctrl_handler);

	mutex_destroy(&imx335->lock);

	imx335_power_off(imx335->dev, imx335->gpio);

	return 0;
}

int imx335_sensor_id(struct i2c_client *client)
{
	u32 id = 0;
	u8 val = 0;
	int rtn = -EINVAL;

	i2c_write_a16d8(client, IMX335_SLAVE_ID, 0x3000, 0);
	usleep_range(30000, 31000);

	i2c_read_a16d8(client, IMX335_SLAVE_ID, 0x3912, &val);
	id |= (val << 8);

	i2c_read_a16d8(client, IMX335_SLAVE_ID, 0x3913, &val);
	id |= val;

	i2c_write_a16d8(client, IMX335_SLAVE_ID, 0x3000, 1);

	if (id != IMX335_ID) {
		dev_info(&client->dev, "Failed to get imx335 id: 0x%x\n", id);
		return rtn;
	} else {
		dev_err(&client->dev, "success get imx335 id 0x%x", id);
	}

	return 0;
}

