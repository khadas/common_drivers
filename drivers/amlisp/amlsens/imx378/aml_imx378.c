// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2023 Amlogic, Inc. All rights reserved.
 */

#include "sensor_drv.h"
#include "i2c_api.h"
#include "mclk_api.h"
#include "aml_imx378.h"

#define AML_SENSOR_NAME "imx378-%u"
#define IMX378_FREQ_INDEX_2160P	0

static const s64 imx378_link_freq_4lanes[] = {
	[IMX378_FREQ_INDEX_2160P] = 1200000000,
};

static inline const s64 *imx378_link_freqs_ptr(const struct imx378 *imx378)
{
	return imx378_link_freq_4lanes;
}

static inline int imx378_link_freqs_num(const struct imx378 *imx378)
{
	return ARRAY_SIZE(imx378_link_freq_4lanes);
}

static const struct imx378_mode imx378_modes_4lanes[] = {
	{
		.width = 3840,
		.height = 2160,
		.hmax = 0x0898,
		.link_freq_index = IMX378_FREQ_INDEX_2160P,
		.data = imx378_2160p_linear_settings,
		.data_size = ARRAY_SIZE(imx378_2160p_linear_settings),
	},
};

static const struct imx378_mode *imx378_modes_ptr(const struct imx378 *imx378)
{

	return imx378_modes_4lanes;
}

static int imx378_modes_num(const struct imx378 *imx378)
{
	return ARRAY_SIZE(imx378_modes_4lanes);
}

static struct imx378 *to_imx378(struct v4l2_subdev *_sd)
{
	return container_of(_sd, struct imx378, sd);
}

static inline int imx378_read_reg(struct imx378 *imx378, u16 addr, u8 *value)
{
	int i, ret;
	u32 regval;

	for (i = 0; i < 3; ++i) {
		ret = regmap_read(imx378->regmap, addr, &regval);
		if (ret == 0)
			break;
	}

	if (ret)
		dev_err(imx378->dev, "I2C read with i2c transfer failed for addr: %x, ret %d\n", addr, ret);

	*value = regval & 0xff;

	return 0;
}

static int imx378_write_reg(struct imx378 *imx378, u16 addr, u8 value)
{
	int i, ret;

	for (i = 0; i < 3; i++) {
		ret = regmap_write(imx378->regmap, addr, value);
		if (ret == 0)
			break;
	}

	if (ret)
		dev_err(imx378->dev, "I2C write failed for addr: %x, ret %d\n", addr, ret);

	return ret;
}

static int imx378_set_register_array(struct imx378 *imx378,
				     const struct imx378_regval *settings,
				     unsigned int num_settings)
{
	unsigned int i;
	int ret = 0;

	for (i = 0; i < num_settings; ++i, ++settings) {
		ret = imx378_write_reg(imx378, settings->reg, settings->val);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int imx378_write_buffered_reg(struct imx378 *imx378, u16 address_low,
				     u8 nr_regs, u32 value)
{
	u8 val = 0;
	int ret = 0;
	unsigned int i;

	ret = imx378_write_reg(imx378, IMX378_REGHOLD, 0x01);
	if (ret) {
		dev_err(imx378->dev, "Error setting hold register\n");
		return ret;
	}

	for (i = 0; i < nr_regs; i++) {
		val = (u8)(value >> ((nr_regs - 1 - i) * 8));
		ret = imx378_write_reg(imx378, address_low + i, val);
		if (ret) {
			dev_err(imx378->dev, "Error writing buffered registers\n");
			return ret;
		}
	}

	ret = imx378_write_reg(imx378, IMX378_REGHOLD, 0x00);
	if (ret) {
		dev_err(imx378->dev, "Error setting hold register\n");
		return ret;
	}

	return ret;
}

static int imx378_set_gain(struct imx378 *imx378, u32 value)
{
	int ret;

	ret = imx378_write_buffered_reg(imx378, IMX378_GAIN, 2, value);
	if (ret)
		dev_err(imx378->dev, "Unable to write gain\n");

	return ret;
}

static int imx378_set_exposure(struct imx378 *imx378, u32 value)
{
	int ret;

	ret = imx378_write_buffered_reg(imx378, IMX378_EXPOSURE, 2, value & 0xFFFF);
	if (ret)
		dev_err(imx378->dev, "Unable to write gain\n");

	return ret;
}

static int imx378_set_fps(struct imx378 *imx378, u32 value)
{
	u32 vts = 0;
	u8 vts_h, vts_l;

	//dev_err(imx378->dev, "-imx378-value = %d\n", value);

	vts = 30 * 3488 / value;
	vts_h = (vts >> 8) & 0x7f;
	vts_l = vts & 0xff;

	imx378_write_reg(imx378, 0x3025, vts_h);
	imx378_write_reg(imx378, 0x3024, vts_l);

	return 0;
}

static int imx378_stop_streaming(struct imx378 *imx378)
{
	imx378->enWDRMode = WDR_MODE_NONE;

	return imx378_write_reg(imx378, IMX378_STANDBY, 0x00);
}

static int imx378_set_ctrl(struct v4l2_ctrl *ctrl)
{
	struct imx378 *imx378 = container_of(ctrl->handler, struct imx378, ctrls);
	int ret = 0;

	/* V4L2 controls values will be applied only when power is already up */
	if (!pm_runtime_get_if_in_use(imx378->dev))
		return 0;

	switch (ctrl->id)
	{
	case V4L2_CID_GAIN:
		ret = imx378_set_gain(imx378, ctrl->val);
		break;
	case V4L2_CID_EXPOSURE:
		ret = imx378_set_exposure(imx378, ctrl->val);
		break;
	case V4L2_CID_HBLANK:
		break;
	case V4L2_CID_AML_CSI_LANES:
		break;
	case V4L2_CID_AML_MODE:
		imx378->enWDRMode = ctrl->val;
		break;
	case V4L2_CID_AML_ORIG_FPS:
		ret = imx378_set_fps(imx378, ctrl->val);
		break;
	default:
		dev_err(imx378->dev, "Error ctrl->id %u, flag 0x%lx\n",
				ctrl->id, ctrl->flags);
		ret = -EINVAL;
		break;
	}

	pm_runtime_put(imx378->dev);

	return ret;
}

static const struct v4l2_ctrl_ops imx378_ctrl_ops = {
	.s_ctrl = imx378_set_ctrl,
};
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int imx378_enum_mbus_code(struct v4l2_subdev *sd,
				 struct v4l2_subdev_state *cfg,
				 struct v4l2_subdev_mbus_code_enum *code)
#else
static int imx378_enum_mbus_code(struct v4l2_subdev *sd,
				 struct v4l2_subdev_pad_config *cfg,
				 struct v4l2_subdev_mbus_code_enum *code)
#endif
{
	if (code->index >= ARRAY_SIZE(imx378_formats))
		return -EINVAL;

	code->code = imx378_formats[code->index].code;

	return 0;
}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int imx378_enum_frame_size(struct v4l2_subdev *sd,
				  struct v4l2_subdev_state *cfg,
				  struct v4l2_subdev_frame_size_enum *fse)
#else
static int imx378_enum_frame_size(struct v4l2_subdev *sd,
				  struct v4l2_subdev_pad_config *cfg,
				  struct v4l2_subdev_frame_size_enum *fse)
#endif
{
	if (fse->index >= ARRAY_SIZE(imx378_formats))
		return -EINVAL;

	fse->min_width = imx378_formats[fse->index].min_width;
	fse->min_height = imx378_formats[fse->index].min_height;
	;
	fse->max_width = imx378_formats[fse->index].max_width;
	fse->max_height = imx378_formats[fse->index].max_height;

	return 0;
}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int imx378_get_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_state *cfg,
			  struct v4l2_subdev_format *fmt)
#else
static int imx378_get_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *fmt)
#endif
{
	struct imx378 *imx378 = to_imx378(sd);
	struct v4l2_mbus_framefmt *framefmt;

	mutex_lock(&imx378->lock);

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY)
		framefmt = v4l2_subdev_get_try_format(&imx378->sd, cfg, fmt->pad);
	else
		framefmt = &imx378->current_format;

	fmt->format = *framefmt;

	mutex_unlock(&imx378->lock);

	return 0;
}

static inline u8 imx378_get_link_freq_index(struct imx378 *imx378)
{
	return imx378->current_mode->link_freq_index;
}

static s64 imx378_get_link_freq(struct imx378 *imx378)
{
	u8 index = imx378_get_link_freq_index(imx378);

	return *(imx378_link_freqs_ptr(imx378) + index);
}

static u64 imx378_calc_pixel_rate(struct imx378 *imx378)
{
	u64 pixel_rate;
	s64 link_freq = imx378_get_link_freq(imx378);
	u8 nlanes = imx378->nlanes;

	pixel_rate = link_freq * 2 * nlanes;
	do_div(pixel_rate, imx378->bpp);

	return pixel_rate;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int imx378_set_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_state *cfg,
			  struct v4l2_subdev_format *fmt)
#else
static int imx378_set_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *fmt)
#endif
{
	unsigned int i;
	struct imx378 *imx378 = to_imx378(sd);
	const struct imx378_mode *mode;
	struct v4l2_mbus_framefmt *format;

	mutex_lock(&imx378->lock);

	mode = v4l2_find_nearest_size(imx378_modes_ptr(imx378),
					imx378_modes_num(imx378),
					width, height,
					fmt->format.width, fmt->format.height);

	fmt->format.width = mode->width;
	fmt->format.height = mode->height;

	for (i = 0; i < ARRAY_SIZE(imx378_formats); i++) {
		if (imx378_formats[i].code == fmt->format.code) {
			dev_info(imx378->dev, "find proper format %d \n", i);
			break;
		}
	}

	if (i >= ARRAY_SIZE(imx378_formats)) {
		i = 0;
		dev_err(imx378->dev, "No format. reset i = 0 \n");
	}

	fmt->format.code = imx378_formats[i].code;
	fmt->format.field = V4L2_FIELD_NONE;

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		dev_info(imx378->dev, "try format \n");
		format = v4l2_subdev_get_try_format(sd, cfg, fmt->pad);
		mutex_unlock(&imx378->lock);

		return 0;
	} else {
		dev_info(imx378->dev, "set format, w %d, h %d, code 0x%x \n",
				fmt->format.width, fmt->format.height,
				fmt->format.code);
		format = &imx378->current_format;
		imx378->current_mode = mode;
		imx378->bpp = imx378_formats[i].bpp;
		imx378->nlanes = 4;

		if (imx378->link_freq)
			__v4l2_ctrl_s_ctrl(imx378->link_freq, imx378_get_link_freq_index(imx378));
		if (imx378->pixel_rate)
			__v4l2_ctrl_s_ctrl_int64(imx378->pixel_rate, imx378_calc_pixel_rate(imx378));
		if (imx378->data_lanes)
			__v4l2_ctrl_s_ctrl(imx378->data_lanes, imx378->nlanes);
	}

	*format = fmt->format;
	imx378->status = 0;

	imx378_set_register_array(imx378, mode->data, mode->data_size);

	mutex_unlock(&imx378->lock);

	return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
int imx378_get_selection(struct v4l2_subdev *sd,
			 struct v4l2_subdev_state *cfg,
			 struct v4l2_subdev_selection *sel)
#else
int imx378_get_selection(struct v4l2_subdev *sd,
			 struct v4l2_subdev_pad_config *cfg,
			 struct v4l2_subdev_selection *sel)
#endif
{
	int rtn = 0;
	struct imx378 *imx378 = to_imx378(sd);
	const struct imx378_mode *mode = imx378->current_mode;

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
		dev_err(imx378->dev, "Error support target: 0x%x\n", sel->target);
		break;
	}

	return rtn;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int imx378_entity_init_cfg(struct v4l2_subdev *subdev,
				  struct v4l2_subdev_state *cfg)
#else
static int imx378_entity_init_cfg(struct v4l2_subdev *subdev,
				  struct v4l2_subdev_pad_config *cfg)
#endif
{
	struct v4l2_subdev_format fmt = {0};

	fmt.which = cfg ? V4L2_SUBDEV_FORMAT_TRY : V4L2_SUBDEV_FORMAT_ACTIVE;
	fmt.format.width = 3840;
	fmt.format.height = 2160;
	fmt.format.code = MEDIA_BUS_FMT_SRGGB10_1X10;

	imx378_set_fmt(subdev, cfg, &fmt);

	return 0;
}

static int imx378_start_streaming(struct imx378 *imx378)
{
	return imx378_write_reg(imx378, IMX378_STANDBY, 0x01);
}

static int imx378_set_stream(struct v4l2_subdev *sd, int enable)
{
	int ret = 0;
	struct imx378 *imx378 = to_imx378(sd);

	if (imx378->status == enable)
		return ret;
	else
		imx378->status = enable;

	if (enable) {
		ret = imx378_start_streaming(imx378);
		if (ret) {
			dev_err(imx378->dev, "Start stream failed\n");
			goto unlock_and_return;
		}

		dev_info(imx378->dev, "stream on\n");
	} else {
		imx378_stop_streaming(imx378);

		dev_info(imx378->dev, "stream off\n");
	}

unlock_and_return:

	return ret;
}

int imx378_power_on(struct device *dev, struct sensor_gpio *gpio)
{
	int ret = 0;

	gpiod_set_value_cansleep(gpio->rst_gpio, 1);
	if (!IS_ERR_OR_NULL(gpio->pwdn_gpio)) {
		gpiod_set_value_cansleep(gpio->pwdn_gpio, 1);
	}
	ret = mclk_enable(dev, 24000000);
	if (ret < 0)
		dev_err(dev, "set mclk fail\n");

	usleep_range(300, 310);

	return 0;
}

int imx378_power_off(struct device *dev, struct sensor_gpio *gpio)
{
	mclk_disable(dev);

	gpiod_set_value_cansleep(gpio->rst_gpio, 0);
	if (!IS_ERR_OR_NULL(gpio->pwdn_gpio)) {
		gpiod_set_value_cansleep(gpio->pwdn_gpio, 0);
	}
	return 0;
}

int imx378_power_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct imx378 *imx378 = to_imx378(sd);

	gpiod_set_value_cansleep(imx378->gpio->rst_gpio, 0);

	return 0;
}

int imx378_power_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct imx378 *imx378 = to_imx378(sd);

	gpiod_set_value_cansleep(imx378->gpio->rst_gpio, 1);

	return 0;
}

static int imx378_log_status(struct v4l2_subdev *sd)
{
	struct imx378 *imx378 = to_imx378(sd);

	dev_info(imx378->dev, "log status done\n");

	return 0;
}

static int imx378_sbdev_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct imx378 *imx378 = to_imx378(sd);

	imx378_power_on(imx378->dev, imx378->gpio);

	return 0;
}
static int imx378_sbdev_close(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct imx378 *imx378 = to_imx378(sd);

	imx378_set_stream(sd, 0);
	imx378_power_off(imx378->dev, imx378->gpio);

	return 0;
}

static const struct dev_pm_ops imx378_pm_ops = {
	SET_RUNTIME_PM_OPS(imx378_power_suspend, imx378_power_resume, NULL)};

const struct v4l2_subdev_core_ops imx378_core_ops = {
	.log_status = imx378_log_status,
};

static const struct v4l2_subdev_video_ops imx378_video_ops = {
	.s_stream = imx378_set_stream,
};

static const struct v4l2_subdev_pad_ops imx378_pad_ops = {
	.init_cfg = imx378_entity_init_cfg,
	.enum_mbus_code = imx378_enum_mbus_code,
	.enum_frame_size = imx378_enum_frame_size,
	.get_selection = imx378_get_selection,
	.get_fmt = imx378_get_fmt,
	.set_fmt = imx378_set_fmt,
};

static struct v4l2_subdev_internal_ops imx378_internal_ops = {
	.open = imx378_sbdev_open,
	.close = imx378_sbdev_close,
};

static const struct v4l2_subdev_ops imx378_subdev_ops = {
	.core = &imx378_core_ops,
	.video = &imx378_video_ops,
	.pad = &imx378_pad_ops,
};

static const struct media_entity_operations imx378_subdev_entity_ops = {
	.link_validate = v4l2_subdev_link_validate,
};

static struct v4l2_ctrl_config wdr_cfg = {
	.ops = &imx378_ctrl_ops,
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
	.ops = &imx378_ctrl_ops,
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
	.ops = &imx378_ctrl_ops,
	.id = V4L2_CID_AML_CSI_LANES,
	.name = "sensor lanes",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.flags = V4L2_CTRL_FLAG_VOLATILE,
	.min = 1,
	.max = 4,
	.step = 1,
	.def = 4,
};

static int imx378_ctrls_init(struct imx378 *imx378)
{
	int rtn = 0;

	v4l2_ctrl_handler_init(&imx378->ctrls, 7);

	v4l2_ctrl_new_std(&imx378->ctrls, &imx378_ctrl_ops,
				V4L2_CID_GAIN, 0, 0xF0, 1, 0);

	v4l2_ctrl_new_std(&imx378->ctrls, &imx378_ctrl_ops,
				V4L2_CID_EXPOSURE, 0, 0x7fffffff, 1, 0);

	imx378->link_freq = v4l2_ctrl_new_int_menu(&imx378->ctrls,
						&imx378_ctrl_ops,
						V4L2_CID_LINK_FREQ,
						imx378_link_freqs_num(imx378) - 1,
						0, imx378_link_freqs_ptr(imx378));

	if (imx378->link_freq)
		imx378->link_freq->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	imx378->pixel_rate = v4l2_ctrl_new_std(&imx378->ctrls,
						&imx378_ctrl_ops,
						V4L2_CID_PIXEL_RATE,
						1, INT_MAX, 1,
						imx378_calc_pixel_rate(imx378));

	imx378->data_lanes = v4l2_ctrl_new_custom(&imx378->ctrls, &nlane_cfg, NULL);
	if (imx378->data_lanes)
		imx378->data_lanes->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	imx378->wdr = v4l2_ctrl_new_custom(&imx378->ctrls, &wdr_cfg, NULL);
	v4l2_ctrl_new_custom(&imx378->ctrls, &fps_cfg, NULL);

	imx378->sd.ctrl_handler = &imx378->ctrls;

	if (imx378->ctrls.error) {
		dev_err(imx378->dev, "Control initialization a error  %d\n", imx378->ctrls.error);
		rtn = imx378->ctrls.error;
	}

	return rtn;
}

static int imx378_register_subdev(struct imx378 *imx378)
{
	int rtn = 0;

	v4l2_i2c_subdev_init(&imx378->sd, imx378->client, &imx378_subdev_ops);

	imx378->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	imx378->sd.dev = &imx378->client->dev;
	imx378->sd.internal_ops = &imx378_internal_ops;
	imx378->sd.entity.ops = &imx378_subdev_entity_ops;
	imx378->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;
	snprintf(imx378->sd.name, sizeof(imx378->sd.name), AML_SENSOR_NAME, imx378->index);

	imx378->pad.flags = MEDIA_PAD_FL_SOURCE;
	rtn = media_entity_pads_init(&imx378->sd.entity, 1, &imx378->pad);
	if (rtn < 0) {
		dev_err(imx378->dev, "Could not register media entity\n");
		goto err_return;
	}

	rtn = v4l2_async_register_subdev(&imx378->sd);
	if (rtn < 0) {
		dev_err(imx378->dev, "Could not register v4l2 device\n");
		goto err_return;
	}

err_return:
	return rtn;
}

int imx378_init(struct i2c_client *client, void *sdrv)
{
	struct device *dev = &client->dev;
	struct imx378 *imx378;
	struct amlsens *sensor = (struct amlsens *)sdrv;
	int ret = -EINVAL;

	imx378 = devm_kzalloc(dev, sizeof(*imx378), GFP_KERNEL);
	if (!imx378)
		return -ENOMEM;

	imx378->dev = dev;
	imx378->client = client;
	imx378->client->addr = IMX378_SLAVE_ID;
	imx378->gpio = &sensor->gpio;

	imx378->regmap = devm_regmap_init_i2c(client, &imx378_regmap_config);
	if (IS_ERR(imx378->regmap)) {
		dev_err(dev, "Unable to initialize I2C\n");
		return -ENODEV;
	}

	if (of_property_read_u32(dev->of_node, "index", &imx378->index)) {
		dev_err(dev, "Failed to read sensor index. default to 0\n");
		imx378->index = 0;
	}

	mutex_init(&imx378->lock);

	/* Power on the device to match runtime PM state below */
	ret = imx378_power_on(imx378->dev, imx378->gpio);
	if (ret < 0) {
		dev_err(dev, "Could not power on the device\n");
		return -ENODEV;
	}

	/*
	 * Initialize the frame format. In particular, imx378->current_mode
	 * and imx378->bpp are set to defaults: imx378_calc_pixel_rate() call
	 * below in imx378_ctrls_init relies on these fields.
	 */
	imx378_entity_init_cfg(&imx378->sd, NULL);

	ret = imx378_ctrls_init(imx378);
	if (ret) {
		dev_err(imx378->dev, "Error ctrls init\n");
		goto free_ctrl;
	}

	ret = imx378_register_subdev(imx378);
	if (ret) {
		dev_err(imx378->dev, "Error register subdev\n");
		goto free_entity;
	}

	dev_err(imx378->dev, "imx378 probe done\n");

	return 0;

free_entity:
	media_entity_cleanup(&imx378->sd.entity);
free_ctrl:
	v4l2_ctrl_handler_free(&imx378->ctrls);
	mutex_destroy(&imx378->lock);

	return ret;
}

int imx378_deinit(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct imx378 *imx378 = to_imx378(sd);

	v4l2_async_unregister_subdev(sd);
	media_entity_cleanup(&sd->entity);
	v4l2_ctrl_handler_free(sd->ctrl_handler);

	mutex_destroy(&imx378->lock);

	imx378_power_off(imx378->dev, imx378->gpio);

	return 0;
}

int imx378_sensor_id(struct i2c_client *client)
{
	u32 id = 0;
	u8 val = 0;
	int rtn = -EINVAL;

	i2c_read_a16d8(client, IMX378_SLAVE_ID, 0x0016, &val);
	id |= (val << 8);

	i2c_read_a16d8(client, IMX378_SLAVE_ID, 0x0017, &val);
	id |= val;

	if (id != IMX378_ID) {
		dev_info(&client->dev, "Failed to get imx378 id: 0x%x\n", id);
		return rtn;
	} else {
		dev_err(&client->dev, "success get imx378 id 0x%x", id);
	}

	return 0;
}

