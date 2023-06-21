// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2023 Amlogic, Inc. All rights reserved.
 */

#include "sensor_drv.h"
#include "i2c_api.h"
#include "mclk_api.h"
#include "aml_lt6911c.h"

#define FREQ_INDEX_1080P    0
#define AML_SENSOR_NAME     "lt6911c-%u"

static const s64 lt6911c_link_freq_2lanes[] = {
	[FREQ_INDEX_1080P] = 672000000,
};

static inline const s64 *lt6911c_link_freqs_ptr(const struct lt6911c *lt6911c)
{
	return lt6911c_link_freq_2lanes;
}
static inline int lt6911c_link_freqs_num(const struct lt6911c *lt6911c)
{
	int lt6911c_link_freqs_num = 1;
	return lt6911c_link_freqs_num;
}

static const struct lt6911c_mode lt6911c_modes_2lanes[] = {
	{
		.width = 1920,
		.height = 1080,
		.hmax  = 0x1130,
		.data = NULL,
		.data_size = 0,
		.link_freq_index = FREQ_INDEX_1080P,
	},
};

int lt6911c_power_on(struct device *dev, struct sensor_gpio *gpio)
{
	return 0;
}

int lt6911c_power_off(struct device *dev, struct sensor_gpio *gpio)
{
	return 0;
}

static int lt6911c_stop_streaming(struct lt6911c *lt6911c)
{
	return 0;
}
static inline const struct lt6911c_mode *lt6911c_modes_ptr(const struct lt6911c *lt6911c)
{
	return lt6911c_modes_2lanes;
}
static inline int lt6911c_modes_num(const struct lt6911c *lt6911c)
{
	return ARRAY_SIZE(lt6911c_modes_2lanes);
}

static inline struct lt6911c *to_lt6911c(struct v4l2_subdev *_sd)
{
	return container_of(_sd, struct lt6911c, sd);
}

static int lt6911c_set_ctrl(struct v4l2_ctrl *ctrl) {
	switch (ctrl->id) {
	case V4L2_CID_AML_CSI_LANES:
		break;
	case V4L2_CID_AML_USER_FPS:
		break;
	}
	return 0;
}

static const struct v4l2_ctrl_ops lt6911c_ctrl_ops = {
	.s_ctrl = lt6911c_set_ctrl,
};

int lt6911c_sbdev_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh) {
	struct lt6911c *lt6911c = to_lt6911c(sd);
	lt6911c_power_on(lt6911c->dev, lt6911c->gpio);
	return 0;
}

int lt6911c_sbdev_close(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh) {
	struct lt6911c *lt6911c = to_lt6911c(sd);
	lt6911c_stop_streaming(lt6911c);
	lt6911c_power_off(lt6911c->dev, lt6911c->gpio);
	return 0;
}

int lt6911c_power_suspend(struct device *dev)
{
	return 0;
}

int lt6911c_power_resume(struct device *dev)
{
	return 0;
}

static const struct dev_pm_ops lt6911c_pm_ops = {
	SET_RUNTIME_PM_OPS(lt6911c_power_suspend, lt6911c_power_resume, NULL)
};

static int lt6911c_log_status(struct v4l2_subdev *sd)
{
	struct lt6911c *lt6911c = to_lt6911c(sd);
	dev_info(lt6911c->dev, "log status done\n");
	return 0;
}

const struct v4l2_subdev_core_ops lt6911c_core_ops = {
	.log_status = lt6911c_log_status,
};

static int lt6911c_start_streaming(struct lt6911c *lt6911c)
{
	return 0;
}

static int lt6911c_set_stream(struct v4l2_subdev *sd, int enable)
{
	struct lt6911c *lt6911c = to_lt6911c(sd);
	int ret = 0;

	if (lt6911c->status == enable)
		return ret;
	else
		lt6911c->status = enable;

	if (enable) {
		ret = lt6911c_start_streaming(lt6911c);
		if (ret) {
			dev_err(lt6911c->dev, "Start stream failed\n");
			goto unlock_and_return;
		}

		dev_info(lt6911c->dev, "stream on\n");
	} else {
		lt6911c_stop_streaming(lt6911c);

		dev_info(lt6911c->dev, "stream off\n");
	}

unlock_and_return:

	return ret;
}

static struct v4l2_ctrl_config wdr_cfg = {
	.ops = &lt6911c_ctrl_ops,
	.id = V4L2_CID_AML_MODE,
	.name = "wdr mode",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.min = 0,
	.max = 2,
	.step = 1,
	.def = 0,
};

static struct v4l2_ctrl_config v4l2_ctrl_output_fps = {
	.ops = &lt6911c_ctrl_ops,
	.id = V4L2_CID_AML_USER_FPS,
	.name = "Sensor output fps",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.min = 0,
	.max = 120,
	.step = 1,
	.def = 60,
};

static struct v4l2_ctrl_config nlane_cfg = {
	.ops = &lt6911c_ctrl_ops,
	.id = V4L2_CID_AML_CSI_LANES,
	.name = "sensor lanes",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.flags = V4L2_CTRL_FLAG_VOLATILE,
	.min = 1,
	.max = 4,
	.step = 1,
	.def = 2,
};

static const struct v4l2_subdev_video_ops lt6911c_video_ops = {
	.s_stream = lt6911c_set_stream,
};

static const struct media_entity_operations lt6911c_subdev_entity_ops = {
	.link_validate = v4l2_subdev_link_validate,
};

static struct v4l2_subdev_internal_ops lt6911c_internal_ops = {
	.open = lt6911c_sbdev_open,
	.close = lt6911c_sbdev_close,
};

static inline u8 lt6911c_get_link_freq_index(struct lt6911c *lt6911c)
{
	return lt6911c->current_mode->link_freq_index;
}
static s64 lt6911c_get_link_freq(struct lt6911c *lt6911c)
{
	u8 index = lt6911c_get_link_freq_index(lt6911c);
	return *(lt6911c_link_freqs_ptr(lt6911c) + index);
}

static u64 lt6911c_calc_pixel_rate(struct lt6911c *lt6911c)
{
	s64 link_freq = lt6911c_get_link_freq(lt6911c);
	u8 nlanes = lt6911c->nlanes;
	u64 pixel_rate;
	pixel_rate = link_freq * 2 * nlanes;
	do_div(pixel_rate, lt6911c->bpp);
	return pixel_rate;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int lt6911c_get_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_state *cfg,
			  struct v4l2_subdev_format *fmt)
#else
static int lt6911c_get_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *fmt)
#endif
{
	struct lt6911c *lt6911c = to_lt6911c(sd);
	struct v4l2_mbus_framefmt *framefmt;

	mutex_lock(&lt6911c->lock);

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY)
		framefmt = v4l2_subdev_get_try_format(&lt6911c->sd, cfg,
							  fmt->pad);
	else
		framefmt = &lt6911c->current_format;

	fmt->format = *framefmt;

	mutex_unlock(&lt6911c->lock);

	return 0;
}


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
int lt6911c_get_selection(struct v4l2_subdev *sd,
				 struct v4l2_subdev_state *cfg,
				 struct v4l2_subdev_selection *sel)
#else
int lt6911c_get_selection(struct v4l2_subdev *sd,
				 struct v4l2_subdev_pad_config *cfg,
				 struct v4l2_subdev_selection *sel)
#endif
{
	int rtn = 0;
	struct lt6911c *lt6911c = to_lt6911c(sd);
	const struct lt6911c_mode *mode = lt6911c->current_mode;

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
			dev_err(lt6911c->dev, "Error support target: 0x%x\n", sel->target);
			break;
	}

	return rtn;
}


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int lt6911c_enum_frame_size(struct v4l2_subdev *sd,
					struct v4l2_subdev_state *cfg,
				   struct v4l2_subdev_frame_size_enum *fse)
#else
static int lt6911c_enum_frame_size(struct v4l2_subdev *sd,
				   struct v4l2_subdev_pad_config *cfg,
				   struct v4l2_subdev_frame_size_enum *fse)
#endif
{
	if (fse->index >= ARRAY_SIZE(lt6911c_formats))
		return -EINVAL;

	fse->min_width = lt6911c_formats[fse->index].min_width;
	fse->min_height = lt6911c_formats[fse->index].min_height;;
	fse->max_width = lt6911c_formats[fse->index].max_width;
	fse->max_height = lt6911c_formats[fse->index].max_height;
	return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int lt6911c_enum_mbus_code(struct v4l2_subdev *sd,
				  struct v4l2_subdev_state *cfg,
				 struct v4l2_subdev_mbus_code_enum *code)
#else
static int lt6911c_enum_mbus_code(struct v4l2_subdev *sd,
				 struct v4l2_subdev_pad_config *cfg,
				 struct v4l2_subdev_mbus_code_enum *code)
#endif
{
	if (code->index >= ARRAY_SIZE(lt6911c_formats))
		return -EINVAL;
	code->code = lt6911c_formats[code->index].code;
	return 0;
}

static inline int lt6911c_read_reg(struct lt6911c *lt6911c, u16 addr, u8 *value)
{
	unsigned int regval;
	int i, ret;

	for (i = 0; i < 3; ++i) {
		ret = regmap_read(lt6911c->regmap, addr, &regval);
		if (0 == ret ) {
			break;
		}
	}

	if (ret)
		dev_err(lt6911c->dev, "I2C read with i2c transfer failed for addr: %x, ret %d\n", addr, ret);

	*value = regval & 0xff;

	return 0;
}

static int lt6911c_write_reg(struct lt6911c *lt6911c, u16 addr, u8 value)
{
	int i, ret;

	for (i = 0; i < 3; i++) {
		ret = regmap_write(lt6911c->regmap, addr, value);
		if (0 == ret) {
			break;
		}
	}

	if (ret)
		dev_err(lt6911c->dev, "I2C write failed for addr: %x, ret %d\n", addr, ret);

	return ret;
}

static int lt6911c_set_register_array(struct lt6911c *lt6911c,
					 const struct lt6911c_regval *settings,
					 unsigned int num_settings)
{
	unsigned int i;
	int ret = 0;

	for (i = 0; i < num_settings; ++i, ++settings) {
		ret = lt6911c_write_reg(lt6911c, settings->reg, settings->val);
		if (ret < 0)
			return ret;
	}

	return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int lt6911c_set_fmt(struct v4l2_subdev *sd,
			struct v4l2_subdev_state *cfg,
			struct v4l2_subdev_format *fmt)
#else
static int lt6911c_set_fmt(struct v4l2_subdev *sd,
			struct v4l2_subdev_pad_config *cfg,
			struct v4l2_subdev_format *fmt)
#endif
{
	struct lt6911c *lt6911c = to_lt6911c(sd);
	const struct lt6911c_mode *mode;
	struct v4l2_mbus_framefmt *format;
	unsigned int i,ret;

	mutex_lock(&lt6911c->lock);

	mode = v4l2_find_nearest_size(lt6911c_modes_ptr(lt6911c),
				 lt6911c_modes_num(lt6911c),
				width, height,
				fmt->format.width, fmt->format.height);

	fmt->format.width = mode->width;
	fmt->format.height = mode->height;

	for (i = 0; i < ARRAY_SIZE(lt6911c_formats); i++) {
		if (lt6911c_formats[i].code == fmt->format.code) {
			dev_info(lt6911c->dev, "find proper format %d \n",i);
			break;
		}
	}

	if (i >= ARRAY_SIZE(lt6911c_formats)) {
		i = 0;
		dev_err(lt6911c->dev, "No format. reset i = 0 \n");
	}

	fmt->format.code = lt6911c_formats[i].code;
	fmt->format.field = V4L2_FIELD_NONE;

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		dev_info(lt6911c->dev, "try format \n");
		format = v4l2_subdev_get_try_format(sd, cfg, fmt->pad);
		mutex_unlock(&lt6911c->lock);
		return 0;
	} else {
		dev_err(lt6911c->dev, "set format, w %d, h %d, code 0x%x \n",
			fmt->format.width, fmt->format.height,
			fmt->format.code);
		format = &lt6911c->current_format;
		lt6911c->current_mode = mode;
		lt6911c->bpp = lt6911c_formats[i].bpp;
		lt6911c->nlanes = 2;

		if (lt6911c->link_freq)
			__v4l2_ctrl_s_ctrl(lt6911c->link_freq, lt6911c_get_link_freq_index(lt6911c));
		if (lt6911c->pixel_rate)
			__v4l2_ctrl_s_ctrl_int64(lt6911c->pixel_rate, lt6911c_calc_pixel_rate(lt6911c));
		if (lt6911c->data_lanes)
			__v4l2_ctrl_s_ctrl(lt6911c->data_lanes, lt6911c->nlanes);
	}

	*format = fmt->format;
	lt6911c->status = 0;
	mutex_unlock(&lt6911c->lock);

	/* Set init register settings */
	ret = lt6911c_set_register_array(lt6911c, NULL, 0);

	return ret;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int lt6911c_entity_init_cfg(struct v4l2_subdev *subdev,
				  struct v4l2_subdev_state *cfg)
#else
static int lt6911c_entity_init_cfg(struct v4l2_subdev *subdev,
				  struct v4l2_subdev_pad_config *cfg)
#endif
{
	struct v4l2_subdev_format fmt = { 0 };

	fmt.which = cfg ? V4L2_SUBDEV_FORMAT_TRY : V4L2_SUBDEV_FORMAT_ACTIVE;
	fmt.format.width = 1920;
	fmt.format.height = 1080;
	fmt.format.code = MEDIA_BUS_FMT_YVYU8_2X8;

	lt6911c_set_fmt(subdev, cfg, &fmt);

	return 0;
}

static const struct v4l2_subdev_pad_ops lt6911c_pad_ops = {
	.init_cfg = lt6911c_entity_init_cfg,
	.enum_mbus_code = lt6911c_enum_mbus_code,
	.enum_frame_size = lt6911c_enum_frame_size,
	.get_selection = lt6911c_get_selection,
	.get_fmt = lt6911c_get_fmt,
	.set_fmt = lt6911c_set_fmt,
};
static const struct v4l2_subdev_ops lt6911c_subdev_ops = {
	.core = &lt6911c_core_ops,
	.video = &lt6911c_video_ops,
	.pad = &lt6911c_pad_ops,
};

static int lt6911c_ctrls_init(struct lt6911c *lt6911c) {
	int rtn = 0;
	v4l2_ctrl_handler_init(&lt6911c->ctrls, 7);
	v4l2_ctrl_new_std(&lt6911c->ctrls, &lt6911c_ctrl_ops,
				V4L2_CID_GAIN, 0, 0xF0, 1, 0);
	v4l2_ctrl_new_std(&lt6911c->ctrls, &lt6911c_ctrl_ops,
				V4L2_CID_EXPOSURE, 0, 0xffff, 1, 0);
	lt6911c->link_freq = v4l2_ctrl_new_int_menu(&lt6911c->ctrls,
						   &lt6911c_ctrl_ops,
						   V4L2_CID_LINK_FREQ,
						   lt6911c_link_freqs_num(lt6911c) - 1,
						   0, lt6911c_link_freqs_ptr(lt6911c) );
	if (lt6911c->link_freq)
			lt6911c->link_freq->flags |= V4L2_CTRL_FLAG_READ_ONLY;
	lt6911c->pixel_rate = v4l2_ctrl_new_std(&lt6911c->ctrls,
						   &lt6911c_ctrl_ops,
						   V4L2_CID_PIXEL_RATE,
						   1, INT_MAX, 1,
						   lt6911c_calc_pixel_rate(lt6911c));

	lt6911c->data_lanes = v4l2_ctrl_new_custom(&lt6911c->ctrls, &nlane_cfg, NULL);
	if (lt6911c->data_lanes)
		lt6911c->data_lanes->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	lt6911c->wdr = v4l2_ctrl_new_custom(&lt6911c->ctrls, &wdr_cfg, NULL);
	lt6911c->fps = v4l2_ctrl_new_custom(&lt6911c->ctrls, &v4l2_ctrl_output_fps, NULL);

	lt6911c->sd.ctrl_handler = &lt6911c->ctrls;

	if (lt6911c->ctrls.error) {
		dev_err(lt6911c->dev, "Control initialization a error  %d\n",
			lt6911c->ctrls.error);
		rtn = lt6911c->ctrls.error;
	}
	rtn = 0;
	return rtn;
}

static int lt6911c_register_subdev(struct lt6911c *lt6911c)
{
	int rtn = 0;

	v4l2_i2c_subdev_init(&lt6911c->sd, lt6911c->client, &lt6911c_subdev_ops);

	lt6911c->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	lt6911c->sd.dev = &lt6911c->client->dev;
	lt6911c->sd.internal_ops = &lt6911c_internal_ops;
	lt6911c->sd.entity.ops = &lt6911c_subdev_entity_ops;
	lt6911c->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;
	snprintf(lt6911c->sd.name, sizeof(lt6911c->sd.name), AML_SENSOR_NAME, lt6911c->index);

	lt6911c->pad.flags = MEDIA_PAD_FL_SOURCE;
	rtn = media_entity_pads_init(&lt6911c->sd.entity, 1, &lt6911c->pad);
	if (rtn < 0) {
		dev_err(lt6911c->dev, "Could not register media entity\n");
		goto err_return;
	}
	rtn = v4l2_async_register_subdev(&lt6911c->sd);
	if (rtn < 0) {
		dev_err(lt6911c->dev, "Could not register v4l2 device\n");
		goto err_return;
	}

err_return:
	return rtn;
}

int lt6911c_init(struct i2c_client *client, void *sdrv)
{
	struct device *dev = &client->dev;
	struct lt6911c *lt6911c;
	struct amlsens *sensor = (struct amlsens *)sdrv;
	int ret = -EINVAL;

	lt6911c = devm_kzalloc(dev, sizeof(*lt6911c), GFP_KERNEL);
	if (!lt6911c)
		return -ENOMEM;

	lt6911c->dev = dev;
	lt6911c->client = client;
	lt6911c->client->addr = LT6911C_SLAVE_ID;
	lt6911c->gpio = &sensor->gpio;

	lt6911c->regmap = devm_regmap_init_i2c(client, &lt6911c_regmap_config);
	if (IS_ERR(lt6911c->regmap)) {
		dev_err(dev, "Unable to initialize I2C\n");
		return -ENODEV;
	}

	if (of_property_read_u32(dev->of_node, "index", &lt6911c->index)) {
		dev_err(dev, "Failed to read sensor index. default to 0\n");
		lt6911c->index = 0;
	}

	mutex_init(&lt6911c->lock);

	/* Power on the device to match runtime PM state below */
	ret = lt6911c_power_on(lt6911c->dev, lt6911c->gpio);
	if (ret < 0) {
		dev_err(dev, "Could not power on the device\n");
		return -ENODEV;
	}

	/*
	 * Initialize the frame format. In particular, lt6911c->current_mode
	 * and lt6911c->bpp are set to defaults: lt6911c_calc_pixel_rate() call
	 * below in lt6911c_ctrls_init relies on these fields.
	 */
	lt6911c_entity_init_cfg(&lt6911c->sd, NULL);

	ret = lt6911c_ctrls_init(lt6911c);
	if (ret) {
		dev_err(lt6911c->dev, "Error ctrls init\n");
		goto free_ctrl;
	}

	ret = lt6911c_register_subdev(lt6911c);
	if (ret) {
		dev_err(lt6911c->dev, "Error register subdev\n");
		goto free_entity;
	}

	dev_err(lt6911c->dev, "lt6911c probe done\n");

	return 0;

free_entity:
	media_entity_cleanup(&lt6911c->sd.entity);
free_ctrl:
	v4l2_ctrl_handler_free(&lt6911c->ctrls);
	mutex_destroy(&lt6911c->lock);

	return ret;
}

int lt6911c_deinit(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct lt6911c *lt6911c = to_lt6911c(sd);

	v4l2_async_unregister_subdev(sd);
	media_entity_cleanup(&sd->entity);
	v4l2_ctrl_handler_free(sd->ctrl_handler);

	mutex_destroy(&lt6911c->lock);

	lt6911c_power_off(lt6911c->dev, lt6911c->gpio);

	return 0;
}

int lt6911c_sensor_id(struct i2c_client *client)
{
	u32 id = 0;
	u8 val = 0;
	int rtn = -EINVAL;

	i2c_read_a16d8(client, LT6911C_SLAVE_ID, 0, &val);
	id |= (val << 8);

	i2c_read_a16d8(client, LT6911C_SLAVE_ID, 1, &val);
	id |= val;

	if (id != lt6911c_ID) {
		dev_info(&client->dev, "Failed to get lt6911c id: 0x%x\n", id);
		return rtn;
	} else {
		dev_err(&client->dev, "success get lt6911c id 0x%x", id);
	}

	return 0;
}
