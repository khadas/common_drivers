// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2023 Amlogic, Inc. All rights reserved.
 */

#include "sensor_drv.h"
#include "i2c_api.h"
#include "mclk_api.h"
#include "aml_imx577.h"

#define AML_SENSOR_NAME "imx577-%u"

static int setting_index = 0;
module_param(setting_index, int, 0644);
MODULE_PARM_DESC(setting_index, "Set setting index: 0 vendor setting;");

static const s64 link_freq[] = {
	IMX577_LINK_FREQ
};

/* Supported sensor mode configurations */
static const struct imx577_mode supported_mode[] =
{
	{
		.width = 4048,
		.height = 3040,
		.code = MEDIA_BUS_FMT_SRGGB10_1X10,

		.hblank = 456,
		.vblank = 76,
		.vblank_min = 0,
		.vblank_max = 32420,

		.link_freq_index = 0,
		.pclk = 480000000,

		.data = mode_4048x3040_raw10_30fps_from_vendor,
		.data_size = ARRAY_SIZE(mode_4048x3040_raw10_30fps_from_vendor),
	}
};

/**
 * to_imx577() - imx577 V4L2 sub-device to imx577 device.
 * @subdev: pointer to imx577 V4L2 sub-device
 *
 * Return: pointer to imx577 device
 */
static inline struct imx577 *to_imx577(struct v4l2_subdev *subdev)
{
	return container_of(subdev, struct imx577, sd);
}

static inline int imx577_read_reg(struct imx577 *imx577, u16 addr, u8 *value)
{
	unsigned int regval;

	int i, ret;

	for ( i = 0; i < 3; ++i ) {
		ret = regmap_read(imx577->regmap, addr, &regval);
		if ( 0 == ret ) {
			break;
		}
	}

	if (ret)
		dev_err(imx577->dev, "I2C read with i2c transfer failed for addr: %x, ret %d\n", addr, ret);

	*value = regval & 0xff;
	return 0;
}

static int imx577_write_reg(struct imx577 *imx577, u16 addr, u8 value)
{
	int i, ret;

	for (i = 0; i < 3; i++) {
		ret = regmap_write(imx577->regmap, addr, value);
		if (0 == ret) {
			break;
		}
	}

	if (ret)
		dev_err(imx577->dev, "I2C write failed for addr: %x, ret %d\n", addr, ret);

	return ret;
}

static int imx577_set_register_array(struct imx577 *imx577,
				     const struct imx577_regval *settings,
				     unsigned int num_settings)
{
	unsigned int i;
	int ret = 0;

	for (i = 0; i < num_settings; ++i, ++settings) {
		ret = imx577_write_reg(imx577, settings->reg, settings->val);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static inline int imx577_write_reg_le_first(struct imx577 *imx577, u16 address_low,
				     u8 nr_regs, u32 value)
{
	unsigned int i;
	int ret;
	if (1 == nr_regs) {
		return imx577_write_reg(imx577, address_low, (u8)(value & 0xff) );
	}

	// little endian to low addr
	for (i = 0; i < nr_regs; i++) {
		ret = imx577_write_reg(imx577, address_low + i,
				       (u8)(value >> (i * 8)));
		if (ret) {
			pr_err("Error writing buffered registers\n");
			return ret;
		}
	}

	return ret;
}

static int imx577_write_reg_be_first(struct imx577 *imx577, u16 address_low,
				     u8 nr_regs, u32 value)
{
	unsigned int i;
	int ret;
	if (1 == nr_regs) {
		return imx577_write_reg(imx577, address_low, (u8)(value & 0xff) );
	}

	// big endian to low addr.
	for (i = 0; i < nr_regs; i++) {
		u8 reg_val = ((value >> ((nr_regs - 1 - i) * 8)) & 0xff) ;
		ret = imx577_write_reg(imx577, address_low + i,
				       reg_val);
		if (ret) {
			pr_err("Error writing buffered registers\n");
			return ret;
		}
	}

	return ret;
}

/**
 * imx577_update_controls() - Update control ranges based on streaming mode
 * @imx577: pointer to imx577 device
 * @mode: pointer to imx577_mode sensor mode
 *
 * Return: 0 if successful, error code otherwise.
 */
static int imx577_update_controls(struct imx577 *imx577,
				  const struct imx577_mode *mode)
{
	int ret;

	ret = __v4l2_ctrl_s_ctrl(imx577->link_freq_ctrl, mode->link_freq_index);
	if (ret)
		return ret;

	ret = __v4l2_ctrl_s_ctrl(imx577->hblank_ctrl, mode->hblank);
	if (ret)
		return ret;

	return __v4l2_ctrl_modify_range(imx577->vblank_ctrl, mode->vblank_min,
					mode->vblank_max, 1, mode->vblank);
}

/**
 * imx577_update_exp_gain() - Set updated exposure and gain
 * @imx577: pointer to imx577 device
 * @exposure: updated exposure value
 * @gain: updated analog gain value
 *
 * Return: 0 if successful, error code otherwise.
 */
static int imx577_update_exp_gain(struct imx577 *imx577, u32 exposure, u32 gain)
{
	u32 lpfr;
	int ret;

	lpfr = imx577->vblank + imx577->current_mode->height;

	//pr_info( "Set exp (reg) 0x%x, again(reg) 0x%x , lpfr(reg) 0x%x",
	//	exposure, gain, lpfr);

	ret = imx577_write_reg_be_first(imx577, IMX577_REG_HOLD, 1, 1);
	if (ret) {
		pr_err("error leave");
		return ret;
	}

	ret = imx577_write_reg_be_first(imx577, IMX577_REG_LPFR, 2, lpfr);
	if (ret) {
		pr_err("error leave");
		goto error_release_group_hold;
	}

	ret = imx577_write_reg_be_first(imx577, IMX577_EXPOSURE, 2, exposure);
	if (ret) {
		pr_err("error leave");
		goto error_release_group_hold;
	}

	ret = imx577_write_reg_be_first(imx577, IMX577_AGAIN, 2, gain);

error_release_group_hold:
	imx577_write_reg_be_first(imx577, IMX577_REG_HOLD, 1, 0);

	return ret;
}


static int imx577_set_gain(struct imx577 *imx577, u32 value)
{
	int ret;
	//pr_info( "new analog gain 0x%x", value);

	ret = imx577_update_exp_gain(imx577, imx577->exp_ctrl->val, value);
	if (ret)
		dev_err(imx577->dev, "Unable to write gain\n");

	return ret;
}

static int imx577_set_exposure(struct imx577 *imx577, u32 value)
{
	int ret;
	//pr_info( "new exp 0x%x", value);

	ret = imx577_update_exp_gain(imx577, value, imx577->again_ctrl->val);
	if (ret)
		dev_err(imx577->dev, "Unable to write gain\n");

	return ret;
}

/**
 * imx577_set_ctrl() - Set subdevice control
 * @ctrl: pointer to v4l2_ctrl structure
 *
 * Supported controls:
 * - V4L2_CID_VBLANK
 *   - V4L2_CID_GAIN
 *   - V4L2_CID_EXPOSURE
 *
 * Return: 0 if successful, error code otherwise.
 */
static int imx577_set_ctrl(struct v4l2_ctrl *ctrl)
{
	struct imx577 *imx577 =
		container_of(ctrl->handler, struct imx577, ctrl_handler);

	int ret = 0;

	/* V4L2 controls values will be applied only when power is already up */
	if (!pm_runtime_get_if_in_use(imx577->dev))
		return 0;

	switch (ctrl->id) {
	case V4L2_CID_GAIN:
		ret = imx577_set_gain(imx577, ctrl->val);
		break;
	case V4L2_CID_EXPOSURE:
		ret = imx577_set_exposure(imx577, ctrl->val);
		break;
	case V4L2_CID_HBLANK:
		break;
	case V4L2_CID_VBLANK:
		imx577->vblank = imx577->vblank_ctrl->val;

		pr_info( "Received vblank %u, new lpfr %u",
			imx577->vblank,
			imx577->vblank + imx577->current_mode->height);

		ret = __v4l2_ctrl_modify_range(imx577->exp_ctrl,
					       IMX577_EXPOSURE_MIN,
					       imx577->vblank + imx577->current_mode->height - IMX577_EXPOSURE_OFFSET,
					       1, IMX577_EXPOSURE_DEFAULT);
		break;
	case V4L2_CID_AML_CSI_LANES:
		break;
	case V4L2_CID_AML_MODE:
		imx577->enWDRMode = ctrl->val;
		break;
	default:
		dev_err(imx577->dev, "Error ctrl->id %u, flag 0x%lx\n",
			ctrl->id, ctrl->flags);
		ret = -EINVAL;
		break;
	}

	pm_runtime_put(imx577->dev);

	return ret;

}

/* V4l2 subdevice control ops*/
static const struct v4l2_ctrl_ops imx577_ctrl_ops = {
	.s_ctrl = imx577_set_ctrl,
};

/**
 * imx577_enum_mbus_code() - Enumerate V4L2 sub-device mbus codes
 * @sd: pointer to imx577 V4L2 sub-device structure
 * @sd_state: V4L2 sub-device configuration
 * @code: V4L2 sub-device code enumeration need to be filled
 *
 * Return: 0 if successful, error code otherwise.
 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int imx577_enum_mbus_code(struct v4l2_subdev *sd,
				  struct v4l2_subdev_state *cfg,
				 struct v4l2_subdev_mbus_code_enum *code)
#else
static int imx577_enum_mbus_code(struct v4l2_subdev *sd,
				 struct v4l2_subdev_pad_config *cfg,
				 struct v4l2_subdev_mbus_code_enum *code)
#endif
{
	if (code->index > 0)
		return -EINVAL;

	code->code = supported_mode[setting_index].code;

	return 0;
}

/**
 * imx577_enum_frame_size() - Enumerate V4L2 sub-device frame sizes
 * @sd: pointer to imx577 V4L2 sub-device structure
 * @sd_state: V4L2 sub-device configuration
 * @fsize: V4L2 sub-device size enumeration need to be filled
 *
 * Return: 0 if successful, error code otherwise.
 */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int imx577_enum_frame_size(struct v4l2_subdev *sd,
			        struct v4l2_subdev_state *cfg,
			       struct v4l2_subdev_frame_size_enum *fsize)
#else
static int imx577_enum_frame_size(struct v4l2_subdev *sd,
			       struct v4l2_subdev_pad_config *cfg,
			       struct v4l2_subdev_frame_size_enum *fsize)
#endif
{
	if (fsize->index > 0)
		return -EINVAL;

	if (fsize->code != supported_mode[setting_index].code)
		return -EINVAL;

	fsize->min_width = supported_mode[setting_index].width;
	fsize->max_width = fsize->min_width;
	fsize->min_height = supported_mode[setting_index].height;
	fsize->max_height = fsize->min_height;

	return 0;
}

/**
 * imx577_fill_pad_format() - Fill subdevice pad format
 *                            from selected sensor mode
 * @imx577: pointer to imx577 device
 * @mode: pointer to imx577_mode sensor mode
 * @fmt: V4L2 sub-device format need to be filled
 */
static void imx577_fill_pad_format(struct imx577 *imx577,
				   const struct imx577_mode *mode,
				   struct v4l2_subdev_format *fmt)
{
	fmt->format.width = mode->width;
	fmt->format.height = mode->height;
	fmt->format.code = mode->code;
	fmt->format.field = V4L2_FIELD_NONE;
	fmt->format.colorspace = V4L2_COLORSPACE_RAW;
	fmt->format.ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
	fmt->format.quantization = V4L2_QUANTIZATION_DEFAULT;
	fmt->format.xfer_func = V4L2_XFER_FUNC_NONE;
}

/**
 * imx577_get_pad_format() - Get subdevice pad format
 * @sd: pointer to imx577 V4L2 sub-device structure
 * @sd_state: V4L2 sub-device configuration
 * @fmt: V4L2 sub-device format need to be set
 *
 * Return: 0 if successful, error code otherwise.
 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int imx577_get_pad_format(struct v4l2_subdev *sd,
				 struct v4l2_subdev_state *cfg,
				 struct v4l2_subdev_format *fmt)
#else
static int imx577_get_pad_format(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *fmt)

#endif

{
	struct imx577 *imx577 = to_imx577(sd);

	mutex_lock(&imx577->mutex);

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		struct v4l2_mbus_framefmt *framefmt;

		framefmt = v4l2_subdev_get_try_format(sd, cfg, fmt->pad);
		fmt->format = *framefmt;
	} else {
		imx577_fill_pad_format(imx577, imx577->current_mode, fmt);
	}

	mutex_unlock(&imx577->mutex);

	return 0;
}

/**
 * imx577_set_pad_format() - Set subdevice pad format
 * @sd: pointer to imx577 V4L2 sub-device structure
 * @sd_state: V4L2 sub-device configuration
 * @fmt: V4L2 sub-device format need to be set
 *
 * Return: 0 if successful, error code otherwise.
 */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int imx577_set_pad_format(struct v4l2_subdev *sd,
				 struct v4l2_subdev_state *cfg,
				 struct v4l2_subdev_format *fmt)
#else
static int imx577_set_pad_format(struct v4l2_subdev *sd,
			struct v4l2_subdev_pad_config *cfg,
			struct v4l2_subdev_format *fmt)
#endif

{
	struct imx577 *imx577 = to_imx577(sd);
	const struct imx577_mode *mode;
	int ret = 0;

	mutex_lock(&imx577->mutex);

	mode = &supported_mode[setting_index];
	imx577_fill_pad_format(imx577, mode, fmt);

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		struct v4l2_mbus_framefmt *framefmt;

		framefmt = v4l2_subdev_get_try_format(sd, cfg, fmt->pad);
		*framefmt = fmt->format;
	} else {
		imx577->current_mode = mode;
		ret = imx577_update_controls(imx577, mode);
	}

	/* Set init register settings */
	ret = imx577_set_register_array(imx577, imx577->current_mode->data,
		imx577->current_mode->data_size );
	if (ret < 0) {
		pr_err( "Could not set initial setting\n");
	} else {
		pr_info("mode changed. setting ok \n");
	}

	mutex_unlock(&imx577->mutex);

	return ret;
}

/**
 * imx577_init_pad_cfg() - Initialize sub-device pad configuration
 * @sd: pointer to imx577 V4L2 sub-device structure
 * @sd_state: V4L2 sub-device configuration
 *
 * Return: 0 if successful, error code otherwise.
 */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int imx577_init_pad_cfg(struct v4l2_subdev *subdev,
				  struct v4l2_subdev_state *cfg)
#else
static int imx577_init_pad_cfg(struct v4l2_subdev *subdev,
				  struct v4l2_subdev_pad_config *cfg)
#endif

{
	struct imx577 *imx577 = to_imx577(subdev);
	struct v4l2_subdev_format fmt = { 0 };

	fmt.which = cfg ? V4L2_SUBDEV_FORMAT_TRY : V4L2_SUBDEV_FORMAT_ACTIVE;
	imx577_fill_pad_format(imx577, &supported_mode[setting_index], &fmt);

	return imx577_set_pad_format(subdev, cfg, &fmt);
}

/**
 * imx577_start_streaming() - Start sensor stream
 * @imx577: pointer to imx577 device
 *
 * Return: 0 if successful, error code otherwise.
 */
static int imx577_start_streaming(struct imx577 *imx577)
{
	int ret;

	/* Setup handler will write actual exposure and gain */
	ret =  v4l2_ctrl_handler_setup(imx577->sd.ctrl_handler);
	if (ret) {
		dev_err(imx577->dev, "fail to setup handler");
		return ret;
	}

	/* Delay is required before streaming*/
	usleep_range(7400, 8000);

	/* Start streaming */
	ret = imx577_write_reg_be_first(imx577, IMX577_REG_MODE_SELECT,
			       1, IMX577_MODE_STREAMING);
	if (ret) {
		dev_err(imx577->dev, "fail to start streaming");
		return ret;
	}

	return 0;
}

/**
 * imx577_stop_streaming() - Stop sensor stream
 * @imx577: pointer to imx577 device
 *
 * Return: 0 if successful, error code otherwise.
 */
static int imx577_stop_streaming(struct imx577 *imx577)
{
	return imx577_write_reg_be_first(imx577, IMX577_REG_MODE_SELECT,
				1, IMX577_MODE_STANDBY);
}

/**
 * imx577_set_stream() - Enable sensor streaming
 * @sd: pointer to imx577 subdevice
 * @enable: set to enable sensor streaming
 *
 * Return: 0 if successful, error code otherwise.
 */
static int imx577_set_stream(struct v4l2_subdev *sd, int enable)
{
	struct imx577 *imx577 = to_imx577(sd);
	int ret;

	mutex_lock(&imx577->mutex);

	if (imx577->streaming == enable) {
		mutex_unlock(&imx577->mutex);
		return 0;
	}

	if (enable) {
		ret = imx577_start_streaming(imx577);
		if (ret)
			goto error_unlock;
	} else {
		imx577_stop_streaming(imx577);
	}

	imx577->streaming = enable;

	mutex_unlock(&imx577->mutex);

	return 0;

error_unlock:
	mutex_unlock(&imx577->mutex);

	return ret;
}

/**
 * imx577_power_on() - Sensor power on sequence
 * @dev: pointer to i2c device
 *
 * Return: 0 if successful, error code otherwise.
 */
int imx577_power_on(struct device *dev, struct sensor_gpio *gpio)
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

/**
 * imx577_power_off() - Sensor power off sequence
 * @dev: pointer to i2c device
 *
 * Return: 0 if successful, error code otherwise.
 */
int imx577_power_off(struct device *dev, struct sensor_gpio *gpio)
{
	mclk_disable(dev);

	gpiod_set_value_cansleep(gpio->rst_gpio, 0);
	if (!IS_ERR_OR_NULL(gpio->pwdn_gpio)) {
		gpiod_set_value_cansleep(gpio->pwdn_gpio, 0);
	}
	return 0;
}

int imx577_power_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct imx577 *imx577 = to_imx577(sd);

	gpiod_set_value_cansleep(imx577->gpio->rst_gpio, 0);

	return 0;
}

int imx577_power_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct imx577 *imx577 = to_imx577(sd);

	gpiod_set_value_cansleep(imx577->gpio->rst_gpio, 1);

	return 0;
}

static int imx577_log_status(struct v4l2_subdev *sd)
{
	struct imx577 *imx577 = to_imx577(sd);

	dev_info(imx577->dev, "log status done\n");

	return 0;
}

int imx577_sbdev_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh) {
	struct imx577 *imx577 = to_imx577(sd);
	imx577_power_on(imx577->dev, imx577->gpio);
	return 0;
}

int imx577_sbdev_close(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh) {
	struct imx577 *imx577 = to_imx577(sd);

	imx577_stop_streaming(imx577);
	imx577_power_off(imx577->dev, imx577->gpio);
	return 0;
}

static const struct dev_pm_ops imx577_pm_ops = {
	SET_RUNTIME_PM_OPS(imx577_power_suspend, imx577_power_resume, NULL)
};

const struct v4l2_subdev_core_ops imx577_core_ops = {
	.log_status = imx577_log_status,
};

/* V4l2 subdevice ops */
static const struct v4l2_subdev_video_ops imx577_video_ops = {
	.s_stream = imx577_set_stream,
};

static const struct v4l2_subdev_pad_ops imx577_pad_ops = {
	.init_cfg = imx577_init_pad_cfg,
	.enum_mbus_code = imx577_enum_mbus_code,
	.enum_frame_size = imx577_enum_frame_size,
	.get_fmt = imx577_get_pad_format,
	.set_fmt = imx577_set_pad_format,
};

static struct v4l2_subdev_internal_ops imx577_internal_ops = {
	.open = imx577_sbdev_open,
	.close = imx577_sbdev_close,
};

static const struct v4l2_subdev_ops imx577_subdev_ops = {
	.core = &imx577_core_ops,
	.video = &imx577_video_ops,
	.pad = &imx577_pad_ops,
};

static const struct media_entity_operations imx577_subdev_entity_ops = {
	.link_validate = v4l2_subdev_link_validate,
};

static struct v4l2_ctrl_config wdr_cfg = {
	.ops = &imx577_ctrl_ops,
	.id = V4L2_CID_AML_MODE,
	.name = "wdr mode",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	.min = 0,
	.max = 2,
	.step = 1,
	.def = 0,
};

static struct v4l2_ctrl_config nlane_cfg = {
	.ops = &imx577_ctrl_ops,
	.id = V4L2_CID_AML_CSI_LANES,
	.name = "sensor lanes",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.flags = V4L2_CTRL_FLAG_VOLATILE,
	.min = 1,
	.max = 4,
	.step = 1,
	.def = 4,
};

/**
 * imx577_ctrls_init() - Initialize sensor subdevice controls
 * @imx577: pointer to imx577 device
 *
 * Return: 0 if successful, error code otherwise.
 */
static int imx577_ctrls_init(struct imx577 *imx577)
{
	struct v4l2_ctrl_handler *ctrl_hdlr = &imx577->ctrl_handler;
	const struct imx577_mode *mode = imx577->current_mode;
	u32 lpfr;
	int ret;

	ret = v4l2_ctrl_handler_init(ctrl_hdlr, 9);
	if (ret)
		return ret;

	/* Serialize controls with sensor device */
	ctrl_hdlr->lock = &imx577->mutex;

	/* Initialize exposure and gain */
	imx577->again_ctrl = v4l2_ctrl_new_std(ctrl_hdlr,
					       &imx577_ctrl_ops,
					       V4L2_CID_GAIN,
					       IMX577_AGAIN_MIN,
					       IMX577_AGAIN_MAX,
					       IMX577_AGAIN_STEP,
					       IMX577_AGAIN_DEFAULT);

	lpfr = mode->vblank + mode->height;
	imx577->exp_ctrl = v4l2_ctrl_new_std(ctrl_hdlr,
					     &imx577_ctrl_ops,
					     V4L2_CID_EXPOSURE,
					     IMX577_EXPOSURE_MIN,
					     lpfr - IMX577_EXPOSURE_OFFSET,
					     IMX577_EXPOSURE_STEP,
					     IMX577_EXPOSURE_DEFAULT);

	imx577->link_freq_ctrl = v4l2_ctrl_new_int_menu(ctrl_hdlr,
							&imx577_ctrl_ops,
							V4L2_CID_LINK_FREQ,
							ARRAY_SIZE(link_freq) -
							1,
							mode->link_freq_index,
							link_freq);
	if (imx577->link_freq_ctrl)
		imx577->link_freq_ctrl->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	/* Read only controls */
	imx577->pclk_ctrl = v4l2_ctrl_new_std(ctrl_hdlr,
					      &imx577_ctrl_ops,
					      V4L2_CID_PIXEL_RATE,
					      mode->pclk, mode->pclk,
					      1, mode->pclk);

	imx577->data_lanes = v4l2_ctrl_new_custom(ctrl_hdlr, &nlane_cfg, NULL);
	if (imx577->data_lanes)
		imx577->data_lanes->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	imx577->wdr = v4l2_ctrl_new_custom(ctrl_hdlr, &wdr_cfg, NULL);

	imx577->vblank_ctrl = v4l2_ctrl_new_std(ctrl_hdlr,
						&imx577_ctrl_ops,
						V4L2_CID_VBLANK,
						mode->vblank_min,
						mode->vblank_max,
						1, mode->vblank);

	imx577->hblank_ctrl = v4l2_ctrl_new_std(ctrl_hdlr,
						&imx577_ctrl_ops,
						V4L2_CID_HBLANK,
						IMX577_REG_MIN,
						IMX577_REG_MAX,
						1, mode->hblank);
	if (imx577->hblank_ctrl)
		imx577->hblank_ctrl->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	if (ctrl_hdlr->error) {
		dev_err(imx577->dev, "control init failed: %d",
			ctrl_hdlr->error);
		v4l2_ctrl_handler_free(ctrl_hdlr);
		return ctrl_hdlr->error;
	}

	imx577->sd.ctrl_handler = ctrl_hdlr;

	return 0;
}

static int imx577_register_subdev(struct imx577 *imx577)
{
	int rtn = 0;

	v4l2_i2c_subdev_init(&imx577->sd, imx577->client, &imx577_subdev_ops);

	imx577->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	imx577->sd.dev = &imx577->client->dev;
	imx577->sd.internal_ops = &imx577_internal_ops;
	imx577->sd.entity.ops = &imx577_subdev_entity_ops;
	imx577->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;
	snprintf(imx577->sd.name, sizeof(imx577->sd.name), AML_SENSOR_NAME, imx577->index);

	imx577->pad.flags = MEDIA_PAD_FL_SOURCE;
	rtn = media_entity_pads_init(&imx577->sd.entity, 1, &imx577->pad);
	if (rtn < 0) {
		dev_err(imx577->dev, "Could not register media entity\n");
		goto err_return;
	}

	rtn = v4l2_async_register_subdev(&imx577->sd);
	if (rtn < 0) {
		dev_err(imx577->dev, "Could not register v4l2 device\n");
		goto err_return;
	}

err_return:
	return rtn;
}

int imx577_init(struct i2c_client *client, void *sdrv)
{
	struct device *dev = &client->dev;
	struct imx577 *imx577;
	struct amlsens *sensor = (struct amlsens *)sdrv;
	int ret = -EINVAL;

	imx577 = devm_kzalloc(dev, sizeof(*imx577), GFP_KERNEL);
	if (!imx577)
		return -ENOMEM;

	imx577->dev = dev;
	imx577->client = client;
	imx577->client->addr = IMX577_SLAVE_ID;
	imx577->gpio = &sensor->gpio;

	imx577->regmap = devm_regmap_init_i2c(client, &imx577_regmap_config);
	if (IS_ERR(imx577->regmap)) {
		dev_err(dev, "Unable to initialize I2C\n");
		return -ENODEV;
	}

	if (of_property_read_u32(dev->of_node, "index", &imx577->index)) {
		dev_err(dev, "Failed to read sensor index. default to 0\n");
		imx577->index = 0;
	}

	mutex_init(&imx577->mutex);

	/* Power on the device to match runtime PM state below */
	ret = imx577_power_on(imx577->dev, imx577->gpio);
	if (ret < 0) {
		dev_err(dev, "Could not power on the device\n");
		return -ENODEV;
	}

	/*
	 * Initialize the frame format. In particular, imx577->current_mode
	 * and imx577->bpp are set to defaults: imx577_calc_pixel_rate() call
	 * below in imx577_ctrls_init relies on these fields.
	 */
	imx577_init_pad_cfg(&imx577->sd, NULL);

	ret = imx577_ctrls_init(imx577);
	if (ret) {
		dev_err(imx577->dev, "Error ctrls init\n");
		goto free_ctrl;
	}

	ret = imx577_register_subdev(imx577);
	if (ret) {
		dev_err(imx577->dev, "Error register subdev\n");
		goto free_entity;
	}

	dev_err(imx577->dev, "imx577 probe done\n");

	return 0;

free_entity:
	media_entity_cleanup(&imx577->sd.entity);
free_ctrl:
	v4l2_ctrl_handler_free(&imx577->ctrl_handler);
	mutex_destroy(&imx577->mutex);

	return ret;
}

/**
 * imx577_deinit() - I2C client device unbinding
 * @client: pointer to I2C client device
 *
 * Return: 0 if successful, error code otherwise.
 */
int imx577_deinit(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct imx577 *imx577 = to_imx577(sd);

	v4l2_async_unregister_subdev(sd);
	media_entity_cleanup(&sd->entity);
	v4l2_ctrl_handler_free(sd->ctrl_handler);

	imx577_power_off(imx577->dev, imx577->gpio);

	mutex_destroy(&imx577->mutex);

	return 0;
}

int imx577_sensor_id(struct i2c_client *client)
{
	u32 id = 0;
	u8 val = 0;
	int rtn = -EINVAL;

	i2c_read_a16d8(client, IMX577_SLAVE_ID, IMX577_REG_ID, &val);
	id |= (val << 8);

	i2c_read_a16d8(client, IMX577_SLAVE_ID, IMX577_REG_ID + 1, &val);
	id |= val;

	if (id != IMX577_ID) {
		dev_info(&client->dev, "Failed to get imx577 id: 0x%x\n", id);
		return rtn;
	} else {
		dev_err(&client->dev, "success get imx577 id 0x%x", id);
	}

	return 0;
}
