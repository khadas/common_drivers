// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2023 Amlogic, Inc. All rights reserved.
 */

#include "sensor_drv.h"
#include "i2c_api.h"
#include "mclk_api.h"
#include "aml_ov16a1q.h"

#define AML_SENSOR_NAME  "ov16a1q-%u"

/* supported link frequencies */
#define FREQ_INDEX_4M	0
#define FREQ_INDEX_4M_60FPS 1
#define FREQ_INDEX_4M_120FPS	2

/* supported link frequencies */
#if defined(OV16A1Q_SDR_60FPS_1572M) || defined(OV16A1Q_SDR_30FPS_756M)
static const s64 ov16a1q_link_freq_2lanes[] = {
	[FREQ_INDEX_4M] = 756000000,
	[FREQ_INDEX_4M_60FPS] = 1572000000,
	[FREQ_INDEX_4M_120FPS] = 1572000000,
};

static const s64 ov16a1q_link_freq_4lanes[] = {
	[FREQ_INDEX_4M] = 756000000,
	[FREQ_INDEX_4M_60FPS] = 1572000000,
	[FREQ_INDEX_4M_120FPS] = 1572000000,
};
#else
static const s64 ov16a1q_link_freq_2lanes[] = {
	[FREQ_INDEX_4M] = 756000000,
	[FREQ_INDEX_4M_60FPS] = 1572000000,
	[FREQ_INDEX_4M_120FPS] = 1572000000,
};

static const s64 ov16a1q_link_freq_4lanes[] = {
	[FREQ_INDEX_4M] = 756000000,
	[FREQ_INDEX_4M_60FPS] = 1572000000,
	[FREQ_INDEX_4M_120FPS] = 1572000000,
};
#endif

static inline const s64 *ov16a1q_link_freqs_ptr(const struct ov16a1q *ov16a1q)
{
	if (ov16a1q->nlanes == 2)
		return ov16a1q_link_freq_2lanes;
	else {
		return ov16a1q_link_freq_4lanes;
	}
}

static inline int ov16a1q_link_freqs_num(const struct ov16a1q *ov16a1q)
{
	if (ov16a1q->nlanes == 2)
		return ARRAY_SIZE(ov16a1q_link_freq_2lanes);
	else
		return ARRAY_SIZE(ov16a1q_link_freq_4lanes);
}

/* Mode configs */
static const struct ov16a1q_mode ov16a1q_modes_2lanes[] = {
	{
		.width = 2304,
		.height = 1748,
		.hmax  = 0x352,
		.data = ov16a1q_1080p_settings,
		.data_size = ARRAY_SIZE(ov16a1q_1080p_settings),

		.link_freq_index = FREQ_INDEX_4M,
	},
	{
		.width = 2304,
		.height = 1748,
		.hmax = 0x1a9,
		.data = ov16a1q_720p_settings,
		.data_size = ARRAY_SIZE(ov16a1q_720p_settings),

		.link_freq_index = FREQ_INDEX_4M,
	},
};

static const struct ov16a1q_mode ov16a1q_modes_4lanes[] = {
	{
		.width = 2304,
		.height = 1748,
		.hmax = 0x352,
		.link_freq_index = FREQ_INDEX_4M,
		.data = ov16a1q_1080p_settings,
		.data_size = ARRAY_SIZE(ov16a1q_1080p_settings),
	},
	{
		.width = 2304,
		.height = 1748,
		.hmax = 0x1a9,
		.link_freq_index = FREQ_INDEX_4M_60FPS,
		.data = ov16a1q_720p_settings,
		.data_size = ARRAY_SIZE(ov16a1q_720p_settings),
	},
};

static inline const struct ov16a1q_mode *ov16a1q_modes_ptr(const struct ov16a1q *ov16a1q)
{
	if (ov16a1q->nlanes == 2)
		return ov16a1q_modes_2lanes;
	else
		return ov16a1q_modes_4lanes;
}

static inline int ov16a1q_modes_num(const struct ov16a1q *ov16a1q)
{
	if (ov16a1q->nlanes == 2)
		return ARRAY_SIZE(ov16a1q_modes_2lanes);
	else
		return ARRAY_SIZE(ov16a1q_modes_4lanes);
}

static inline struct ov16a1q *to_ov16a1q(struct v4l2_subdev *_sd)
{
	return container_of(_sd, struct ov16a1q, sd);
}

static inline int ov16a1q_read_reg(struct ov16a1q *ov16a1q, u16 addr, u8 *value)
{
	int msg_count = 0;
	int i, ret;

	u8 buf[4];
	struct i2c_msg msgs[2];

	buf[0] = (addr >> 8) & 0xff;
	buf[1] = addr & 0xff;

	msgs[0].addr  = ov16a1q->i2c_addr;
	msgs[0].flags = 0;
	msgs[0].len = 2;
	msgs[0].buf = buf;

	msgs[1].addr  = ov16a1q->i2c_addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = 1;
	msgs[1].buf = buf;

	msg_count = sizeof(msgs) / sizeof(msgs[0]);
	for (i = 0; i < 3; i++) {
		ret = i2c_transfer(ov16a1q->client->adapter, msgs, msg_count);
		if (ret == msg_count)
			break;
	}

	if (ret != msg_count)
		dev_err(ov16a1q->dev, "I2C read with i2c transfer failed for addr: %x, ret %d\n", addr, ret);

	*value = buf[0] & 0xff;
	return 0;
}

static int ov16a1q_write_reg(struct ov16a1q *ov16a1q, u16 addr, u8 value)
{
	int msg_count = 0;
	int i, ret;

	u8 buf[3];
	struct i2c_msg msgs[1];

	buf[0] = (addr >> 8) & 0xff;
	buf[1] = addr & 0xff;
	buf[2] = value & 0xff;

	msgs[0].addr = ov16a1q->i2c_addr;
	msgs[0].flags = 0;
	msgs[0].len = 3;
	msgs[0].buf = buf;

	msg_count = sizeof(msgs) / sizeof(msgs[0]);
	for (i = 0; i < 3; i++) {
		ret = i2c_transfer(ov16a1q->client->adapter, msgs, msg_count);
		if (ret == msg_count) {
			break;
		}
	}

	if (ret != msg_count)
		dev_err(ov16a1q->dev, "I2C write failed for addr: %x, ret %d\n", addr, ret);

	return ((ret != msg_count) ? -1 : 0);
}

static int ov16a1q_set_register_array(struct ov16a1q *ov16a1q,
					 const struct ov16a1q_regval *settings,
					 unsigned int num_settings)
{
	unsigned int i;
	int ret = 0;

	for (i = 0; i < num_settings; ++i, ++settings) {
		ret = ov16a1q_write_reg(ov16a1q, settings->reg, settings->val);
		if (ret < 0)
			return ret;
	}

	return 0;
}

/*
static int ov16a1q_write_buffered_reg(struct ov16a1q *ov16a1q, u16 address_low,
					 u8 nr_regs, u32 value)
{
	unsigned int i;
	int ret;

	for (i = 0; i < nr_regs; i++) {
		ret = ov16a1q_write_reg(ov16a1q, address_low + i,
					   (u8)(value >> ((1-i) * 8)));
		if (ret) {
			dev_err(ov16a1q->dev, "Error writing buffered registers\n");
			return ret;
		}
	}

	return ret;
}
*/

static uint16_t again_table_1[16] =
{
	0x0,0x10,0x20,0x30,0x40,0x50,0x60,0x70,0x80,0x90,0xA0,0xB0,0xC0,0xD0,0xE0,0xF0
};
static uint16_t again_table_2[8] =
{
	0x0,0x20,0x40,0x60,0x80,0xA0,0xC0,0xE0
};
static uint16_t again_table_3[4] =
{
	0x0,0x40,0x80,0xC0
};

static int ov16a1q_set_gain(struct ov16a1q *ov16a1q, u32 value)
{
	int ret = 0;
	int i = 0;
	u8 value_H = 0;
	u8 value_L = 0;

	dev_err(ov16a1q->dev, "ov16a1q_set_gain = 0x%x \n", value);

	value_H = (value << 1) >> 8;
	value_L = (value << 1) & 0xFF;

	if ( value_H == 1) {
		for (i = 0; i < 15; i++) {
			if ( value_L < ((again_table_1[i] + again_table_1[i + 1])/2) ) {
				value_L = again_table_1[i];
				break;
			} else {
				if ((value_L < again_table_1[i + 1]) || (value_L == again_table_1[i + 1])) {
					value_L = again_table_1[i + 1];
					break;
				}
			}
		}
		if (value_L >  again_table_1[15]) {
			value_L = again_table_1[15];
		}
	} else if (value_H > 1 && value_H < 4) {
			for (i = 0; i < 7; i++) {
				if ( value_L < ((again_table_2[i] + again_table_2[i + 1])/2) ) {
					value_L = again_table_2[i];
					break;
				} else {
					if ((value_L < again_table_2[i + 1]) || (value_L == again_table_2[i + 1])) {
						value_L = again_table_2[i + 1];
						break;
					}
				}
			}
			if (value_L >  again_table_1[7]) {
				value_L = again_table_1[7];
			}
		} else if (((value_H == 4 || value_H > 4) && value_H < 8)) {
				for (i = 0; i < 3; i++) {
					if ( value_L < ((again_table_3[i] + again_table_3[i + 1])/2) ) {
						value_L = again_table_3[i];
						break;
					} else {
						if ((value_L < again_table_3[i + 1]) || (value_L == again_table_3[i + 1])) {
							value_L = again_table_3[i + 1];
							break;
						}
					}
				}
				if (value_L >  again_table_1[4]) {
					value_L = again_table_1[4];
				}
			} else {
				if ( value_L < (0x80/2)) {
						value_L = 0;
					} else {
						value_L = 0x80;
					}
				}

	ret = ov16a1q_write_reg(ov16a1q, OV16A1Q_GAIN, value_H);
	if (ret)
		dev_err(ov16a1q->dev, "Unable to write OV16A1Q_GAIN_H \n");
	ret = ov16a1q_write_reg(ov16a1q, OV16A1Q_GAIN_L, value_L);
	if (ret)
		dev_err(ov16a1q->dev, "Unable to write OV16A1Q_GAIN_L \n");

	ov16a1q_read_reg(ov16a1q, OV16A1Q_GAIN, &value_H);
	ov16a1q_read_reg(ov16a1q, OV16A1Q_GAIN_L, &value_L);
	dev_err(ov16a1q->dev, "ov16a1q read gain = 0x%x \n", (((value_H << 8) | value_L) >> 1));
	return ret;
}

static int ov16a1q_set_exposure(struct ov16a1q *ov16a1q, u32 value)
{
	int ret;
	u8 value_H = 0;
	u8 value_L = 0;
	dev_err(ov16a1q->dev, "ov16a1q_set_exposure = 0x%x \n", value);
	ret = ov16a1q_write_reg(ov16a1q, OV16A1Q_EXPOSURE, (value >> 8) & 0xFF);
	if (ret)
		dev_err(ov16a1q->dev, "Unable to write gain_H\n");
	ret = ov16a1q_write_reg(ov16a1q, OV16A1Q_EXPOSURE_L, value & 0xFE);
	if (ret)
		dev_err(ov16a1q->dev, "Unable to write gain_L\n");

	ov16a1q_read_reg(ov16a1q, OV16A1Q_EXPOSURE, &value_H);
	ov16a1q_read_reg(ov16a1q, OV16A1Q_EXPOSURE_L, &value_L);
	dev_err(ov16a1q->dev, "ov16a1q read exposure = 0x%x \n", (value_H << 8) | (value_L & 0xFE));
	return ret;
}

static int ov16a1q_set_fps(struct ov16a1q *ov16a1q, u32 value)
{
	//u32 vts = 0;
	//u8 vts_h, vts_l;

	dev_err(ov16a1q->dev, "ov16a1q_set_fps = %d \n", value);

	if (value == 30) {
		ov16a1q_write_reg(ov16a1q, 0x0305, 0x7a); //PLL
		ov16a1q_write_reg(ov16a1q, 0x0307, 0x01);
		ov16a1q_write_reg(ov16a1q, 0x4837, 0x15);

		ov16a1q_write_reg(ov16a1q, 0x380c, 0x03); // HTS
		ov16a1q_write_reg(ov16a1q, 0x380d, 0x52);

		ov16a1q_write_reg(ov16a1q, 0x3501, 0x0f);
		ov16a1q_write_reg(ov16a1q, 0x3802, 0x48);
	} else if (value == 60) {
		ov16a1q_write_reg(ov16a1q, 0x0305, 0x89); //PLL
		ov16a1q_write_reg(ov16a1q, 0x0307, 0x00);
		ov16a1q_write_reg(ov16a1q, 0x4837, 0x0a);

		ov16a1q_write_reg(ov16a1q, 0x380c, 0x01); // HTS
		ov16a1q_write_reg(ov16a1q, 0x380d, 0xa9);

		ov16a1q_write_reg(ov16a1q, 0x3501, 0x07);
		ov16a1q_write_reg(ov16a1q, 0x3802, 0x3c);
	} else {
		dev_err(ov16a1q->dev, "don't support %d fps, keep same\n", value);
	}

	/**
	vts = 30 * 0x0cc0 / value;
	vts_h = (vts >> 8) & 0x7f;
	vts_l = vts & 0xff;

	ov16a1q_write_reg(ov16a1q, 0x380e, vts_h);
	ov16a1q_write_reg(ov16a1q, 0x380f, vts_l);
	*/
	return 0;
}

/* Stop streaming */
static int ov16a1q_stop_streaming(struct ov16a1q *ov16a1q)
{
	ov16a1q->enWDRMode = WDR_MODE_NONE;
	return ov16a1q_write_reg(ov16a1q, 0x0100, 0x00);
}

static int ov16a1q_set_ctrl(struct v4l2_ctrl *ctrl)
{
	struct ov16a1q *ov16a1q = container_of(ctrl->handler,
						struct ov16a1q, ctrls);
	int ret = 0;

	/* V4L2 controls values will be applied only when power is already up */
	if (!pm_runtime_get_if_in_use(ov16a1q->dev))
		return 0;

	switch (ctrl->id) {
	case V4L2_CID_GAIN:
		ret = ov16a1q_set_gain(ov16a1q, ctrl->val);
		break;
	case V4L2_CID_EXPOSURE:
		ret = ov16a1q_set_exposure(ov16a1q, ctrl->val);
		break;
	case V4L2_CID_HBLANK:
		break;
	case V4L2_CID_AML_CSI_LANES:
		break;
	case V4L2_CID_AML_MODE:
		ov16a1q->enWDRMode = ctrl->val;
		break;
	case V4L2_CID_AML_ORIG_FPS:
		ret = ov16a1q_set_fps(ov16a1q, ctrl->val);
		break;
	default:
		dev_err(ov16a1q->dev, "Error ctrl->id %u, flag 0x%lx\n",
			ctrl->id, ctrl->flags);
		ret = -EINVAL;
		break;
	}

	pm_runtime_put(ov16a1q->dev);

	return ret;
}

static int ov16a1q_get_ctrl(struct v4l2_ctrl *ctrl)
{
	struct ov16a1q *ov16a1q = container_of(ctrl->handler,
						 struct ov16a1q, ctrls);
	int ret = 0;

	switch (ctrl->id) {
	case V4L2_CID_AML_ADDRESS:
		dev_err(ov16a1q->dev, "i2c_addr 0x%x\n", ov16a1q->i2c_addr);
		ctrl->val = ov16a1q->i2c_addr;
		break;
	default:
		dev_err(ov16a1q->dev, "Error ctrl->id %u, flag 0x%lx\n",
			ctrl->id, ctrl->flags);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static const struct v4l2_ctrl_ops ov16a1q_ctrl_ops = {
	.s_ctrl = ov16a1q_set_ctrl,
	.g_volatile_ctrl = ov16a1q_get_ctrl,
};
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int ov16a1q_enum_mbus_code(struct v4l2_subdev *sd,
				  struct v4l2_subdev_state *cfg,
				 struct v4l2_subdev_mbus_code_enum *code)
#else
static int ov16a1q_enum_mbus_code(struct v4l2_subdev *sd,
				 struct v4l2_subdev_pad_config *cfg,
				 struct v4l2_subdev_mbus_code_enum *code)
#endif
{
	if (code->index >= ARRAY_SIZE(ov16a1q_formats))
		return -EINVAL;

	code->code = ov16a1q_formats[code->index].code;

	return 0;
}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int ov16a1q_enum_frame_size(struct v4l2_subdev *sd,
					struct v4l2_subdev_state *cfg,
				   struct v4l2_subdev_frame_size_enum *fse)
#else
static int ov16a1q_enum_frame_size(struct v4l2_subdev *sd,
				   struct v4l2_subdev_pad_config *cfg,
				   struct v4l2_subdev_frame_size_enum *fse)
#endif
{
	if (fse->index >= ARRAY_SIZE(ov16a1q_formats))
		return -EINVAL;

	fse->min_width = ov16a1q_formats[fse->index].min_width;
	fse->min_height = ov16a1q_formats[fse->index].min_height;
	fse->max_width = ov16a1q_formats[fse->index].max_width;
	fse->max_height = ov16a1q_formats[fse->index].max_height;

	return 0;
}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int ov16a1q_get_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_state *cfg,
			  struct v4l2_subdev_format *fmt)
#else
static int ov16a1q_get_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *fmt)
#endif
{
	struct ov16a1q *ov16a1q = to_ov16a1q(sd);
	struct v4l2_mbus_framefmt *framefmt;

	mutex_lock(&ov16a1q->lock);

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY)
		framefmt = v4l2_subdev_get_try_format(&ov16a1q->sd, cfg,
							  fmt->pad);
	else
		framefmt = &ov16a1q->current_format;

	fmt->format = *framefmt;

	mutex_unlock(&ov16a1q->lock);

	return 0;
}


static inline u8 ov16a1q_get_link_freq_index(struct ov16a1q *ov16a1q)
{
	return ov16a1q->current_mode->link_freq_index;
}

static s64 ov16a1q_get_link_freq(struct ov16a1q *ov16a1q)
{
	u8 index = ov16a1q_get_link_freq_index(ov16a1q);

	return *(ov16a1q_link_freqs_ptr(ov16a1q) + index);
}

static u64 ov16a1q_calc_pixel_rate(struct ov16a1q *ov16a1q)
{
	s64 link_freq = ov16a1q_get_link_freq(ov16a1q);
	u8 nlanes = ov16a1q->nlanes;
	u64 pixel_rate;

	/* pixel rate = link_freq * 2 * nr_of_lanes / bits_per_sample */
	pixel_rate = link_freq * 2 * nlanes;
	do_div(pixel_rate, ov16a1q->bpp);
	return pixel_rate;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int ov16a1q_set_fmt(struct v4l2_subdev *sd,
			struct v4l2_subdev_state *cfg,
			struct v4l2_subdev_format *fmt)
#else
static int ov16a1q_set_fmt(struct v4l2_subdev *sd,
			struct v4l2_subdev_pad_config *cfg,
			struct v4l2_subdev_format *fmt)
#endif
{
	struct ov16a1q *ov16a1q = to_ov16a1q(sd);
	const struct ov16a1q_mode *mode;
	struct v4l2_mbus_framefmt *format;
	unsigned int i,ret;

	mutex_lock(&ov16a1q->lock);

	//ov16a1q_modes_ptr Returns a set of data
	//ov16a1q_modes_num How many groups to return
	mode = v4l2_find_nearest_size(ov16a1q_modes_ptr(ov16a1q),
				 ov16a1q_modes_num(ov16a1q),
				width, height,
				fmt->format.width, fmt->format.height);

	fmt->format.width = mode->width;
	fmt->format.height = mode->height;

	for (i = 0; i < ARRAY_SIZE(ov16a1q_formats); i++) {
		if (ov16a1q_formats[i].code == fmt->format.code) {
			dev_info(ov16a1q->dev, "find proper format %d \n",i);
			break;
		}
	}

	if (i >= ARRAY_SIZE(ov16a1q_formats)) {
		i = 0;
		dev_err(ov16a1q->dev, "No format. reset i = 0 \n");
	}

	/*Here it is equivalent to assigning ov16a1q_formats[i]*/
	fmt->format.code = ov16a1q_formats[i].code;
	fmt->format.field = V4L2_FIELD_NONE;

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		dev_info(ov16a1q->dev, "try format \n");
		format = v4l2_subdev_get_try_format(sd, cfg, fmt->pad);
		mutex_unlock(&ov16a1q->lock);
		return 0;
	} else {
		dev_err(ov16a1q->dev, "set format, w %d, h %d, code 0x%x \n",
			fmt->format.width, fmt->format.height,
			fmt->format.code);
		format = &ov16a1q->current_format;
		ov16a1q->current_mode = mode;
		ov16a1q->bpp = ov16a1q_formats[i].bpp;
		ov16a1q->nlanes = 4;

		if (ov16a1q->link_freq)
			__v4l2_ctrl_s_ctrl(ov16a1q->link_freq, ov16a1q_get_link_freq_index(ov16a1q) );
		if (ov16a1q->pixel_rate)
			__v4l2_ctrl_s_ctrl_int64(ov16a1q->pixel_rate, ov16a1q_calc_pixel_rate(ov16a1q) );
		if (ov16a1q->data_lanes)
			__v4l2_ctrl_s_ctrl(ov16a1q->data_lanes, ov16a1q->nlanes);
	}

	*format = fmt->format;
	ov16a1q->status = 0;

	mutex_unlock(&ov16a1q->lock);

	if (ov16a1q->enWDRMode) {
		/* Set init register settings */
		ret = ov16a1q_set_register_array(ov16a1q, setting_2328_1748_4lane_756m_30fps,
			ARRAY_SIZE(setting_2328_1748_4lane_756m_30fps));
		if (ret < 0) {
			dev_err(ov16a1q->dev, "Could not set init registers\n");
			return ret;
		} else
			dev_err(ov16a1q->dev, "ov16a1q wdr mode init...\n");
	} else {
		/* Set init register settings */
		ret = ov16a1q_set_register_array(ov16a1q, setting_2328_1748_4lane_756m_30fps,
			ARRAY_SIZE(setting_2328_1748_4lane_756m_30fps));
		if (ret < 0) {
			dev_err(ov16a1q->dev, "Could not set init registers\n");
			return ret;
		} else
			dev_err(ov16a1q->dev, "ov16a1q linear mode init...\n");
	}

	return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
int ov16a1q_get_selection(struct v4l2_subdev *sd,
				 struct v4l2_subdev_state *cfg,
				 struct v4l2_subdev_selection *sel)
#else
int ov16a1q_get_selection(struct v4l2_subdev *sd,
				 struct v4l2_subdev_pad_config *cfg,
				 struct v4l2_subdev_selection *sel)
#endif
{
	int rtn = 0;
	struct ov16a1q *ov16a1q = to_ov16a1q(sd);
	const struct ov16a1q_mode *mode = ov16a1q->current_mode;

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
		dev_err(ov16a1q->dev, "Error support target: 0x%x\n", sel->target);
	break;
	}

	return rtn;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static int ov16a1q_entity_init_cfg(struct v4l2_subdev *subdev,
				  struct v4l2_subdev_state *cfg)
#else
static int ov16a1q_entity_init_cfg(struct v4l2_subdev *subdev,
				  struct v4l2_subdev_pad_config *cfg)
#endif
{
	struct v4l2_subdev_format fmt = { 0 };

	fmt.which = cfg ? V4L2_SUBDEV_FORMAT_TRY : V4L2_SUBDEV_FORMAT_ACTIVE;
	fmt.format.width = 2304;
	fmt.format.height = 1748;
	fmt.format.code = MEDIA_BUS_FMT_SBGGR10_1X10;

	ov16a1q_set_fmt(subdev, cfg, &fmt);

	return 0;
}

/* Start streaming */
static int ov16a1q_start_streaming(struct ov16a1q *ov16a1q)
{
	u8 val = 0;
	u32 exp = 0;
	u32 gain = 0;
	ov16a1q_read_reg(ov16a1q, 0x3501, &val);
	exp = val << 8;
	ov16a1q_read_reg(ov16a1q, 0x3502, &val);
	exp = exp | val;
	ov16a1q_read_reg(ov16a1q, 0x3508, &val);
	gain = val << 8;
	ov16a1q_read_reg(ov16a1q, 0x3509, &val);
	gain = gain | val;

	dev_info(ov16a1q->dev, "gain: %d, exp: %d\n", gain, exp);

	/* Start streaming */
	return ov16a1q_write_reg(ov16a1q, 0x0100, 0x01);
}

static int ov16a1q_set_stream(struct v4l2_subdev *sd, int enable)
{
	struct ov16a1q *ov16a1q = to_ov16a1q(sd);
	int ret = 0;

	if (ov16a1q->status == enable)
		return ret;
	else
		ov16a1q->status = enable;

	if (enable) {
		ret = ov16a1q_start_streaming(ov16a1q);
		if (ret) {
			dev_err(ov16a1q->dev, "Start stream failed\n");
			goto unlock_and_return;
		}

		dev_info(ov16a1q->dev, "stream on\n");
	} else {
		ov16a1q_stop_streaming(ov16a1q);

		dev_info(ov16a1q->dev, "stream off\n");
	}

unlock_and_return:

	return ret;
}

/*
int reset_am_enable(struct device *dev, const char* propname, int val)
{
	int ret = -1;

	int reset = of_get_named_gpio(dev->of_node, propname, 0);
	ret = reset;

	if (ret >= 0) {
		devm_gpio_request(dev, reset, "RESET");
		if (gpio_is_valid(reset)) {
			gpio_direction_output(reset, val);
			pr_info("reset init\n");
		} else {
			pr_err("reset_enable: gpio %s is not valid\n", propname);
			return -1;
		}
	} else {
		pr_err("reset_enable: get_named_gpio %s fail\n", propname);
	}

	return ret;
}
*/

int ov16a1q_power_on(struct device *dev, struct sensor_gpio *gpio)
{
	int ret;

	if (!IS_ERR_OR_NULL(gpio->power_gpio))
		gpiod_set_value_cansleep(gpio->power_gpio, 1);

	// 30ms
	usleep_range(100000, 101000);

	gpiod_set_value_cansleep(gpio->rst_gpio, 1);

	if (!IS_ERR_OR_NULL(gpio->pwdn_gpio)) {
		gpiod_set_value_cansleep(gpio->pwdn_gpio, 1);
	}

	ret = mclk_enable(dev, 24000000);
	if (ret < 0 )
		dev_err(dev, "set mclk fail\n");

	// 30ms
	usleep_range(30000, 31000);

	return 0;
}

int ov16a1q_power_off(struct device *dev, struct sensor_gpio *gpio)
{
	mclk_disable(dev);

	gpiod_set_value_cansleep(gpio->rst_gpio,0);

	if (!IS_ERR_OR_NULL(gpio->pwdn_gpio)) {
		gpiod_set_value_cansleep(gpio->pwdn_gpio, 0);
	}

	if (!IS_ERR_OR_NULL(gpio->power_gpio))
		gpiod_set_value_cansleep(gpio->power_gpio, 0);

	return 0;
}

int ov16a1q_power_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ov16a1q *ov16a1q = to_ov16a1q(sd);

	gpiod_set_value_cansleep(ov16a1q->gpio->rst_gpio, 0);

	if (!IS_ERR_OR_NULL(ov16a1q->gpio->pwdn_gpio))
		gpiod_set_value_cansleep(ov16a1q->gpio->pwdn_gpio, 0);

	if (!IS_ERR_OR_NULL(ov16a1q->gpio->power_gpio))
		gpiod_set_value_cansleep(ov16a1q->gpio->power_gpio, 0);

	return 0;
}

int ov16a1q_power_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ov16a1q *ov16a1q = to_ov16a1q(sd);

	gpiod_set_value_cansleep(ov16a1q->gpio->rst_gpio, 1);

	if (!IS_ERR_OR_NULL(ov16a1q->gpio->pwdn_gpio))
		gpiod_set_value_cansleep(ov16a1q->gpio->pwdn_gpio, 1);

	if (!IS_ERR_OR_NULL(ov16a1q->gpio->power_gpio))
		gpiod_set_value_cansleep(ov16a1q->gpio->power_gpio, 1);

	return 0;
}

static int ov16a1q_log_status(struct v4l2_subdev *sd)
{
	struct ov16a1q *ov16a1q = to_ov16a1q(sd);

	dev_info(ov16a1q->dev, "log status done\n");

	return 0;
}

static const struct dev_pm_ops ov16a1q_pm_ops = {
	SET_RUNTIME_PM_OPS(ov16a1q_power_suspend, ov16a1q_power_resume, NULL)
};

const struct v4l2_subdev_core_ops ov16a1q_core_ops = {
	.log_status = ov16a1q_log_status,
};

static const struct v4l2_subdev_video_ops ov16a1q_video_ops = {
	.s_stream = ov16a1q_set_stream,
};

static const struct v4l2_subdev_pad_ops ov16a1q_pad_ops = {
	.init_cfg = ov16a1q_entity_init_cfg,
	.enum_mbus_code = ov16a1q_enum_mbus_code,
	.enum_frame_size = ov16a1q_enum_frame_size,
	.get_selection = ov16a1q_get_selection,
	.get_fmt = ov16a1q_get_fmt,
	.set_fmt = ov16a1q_set_fmt,
};

static const struct v4l2_subdev_ops ov16a1q_subdev_ops = {
	.core = &ov16a1q_core_ops,
	.video = &ov16a1q_video_ops,
	.pad = &ov16a1q_pad_ops,
};

static const struct media_entity_operations ov16a1q_subdev_entity_ops = {
	.link_validate = v4l2_subdev_link_validate,
};

static struct v4l2_ctrl_config wdr_cfg = {
	.ops = &ov16a1q_ctrl_ops,
	.id = V4L2_CID_AML_MODE,
	.name = "wdr mode",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.min = 0,
	.max = 2,
	.step = 1,
	.def = 0,
};

static struct v4l2_ctrl_config addr_cfg = {
	.ops = &ov16a1q_ctrl_ops,
	.id = V4L2_CID_AML_ADDRESS,
	.name = "sensor address",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	.min = 0x00,
	.max = 0xFF,
	.step = 1,
	.def = 0x00,
};

static struct v4l2_ctrl_config fps_cfg = {
	.ops = &ov16a1q_ctrl_ops,
	.id = V4L2_CID_AML_ORIG_FPS,
	.name = "sensor fps",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	.min = 1,
	.max = 120,
	.step = 1,
	.def = 30,
};

static struct v4l2_ctrl_config nlane_cfg = {
	.ops = &ov16a1q_ctrl_ops,
	.id = V4L2_CID_AML_CSI_LANES,
	.name = "sensor lanes",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.flags = V4L2_CTRL_FLAG_VOLATILE,
	.min = 1,
	.max = 4,
	.step = 1,
	.def = 4,
};

static int ov16a1q_ctrls_init(struct ov16a1q *ov16a1q)
{
	int rtn = 0;

	v4l2_ctrl_handler_init(&ov16a1q->ctrls, 4);

	v4l2_ctrl_new_std(&ov16a1q->ctrls, &ov16a1q_ctrl_ops,
				V4L2_CID_GAIN, 0, 0xFFFF, 1, 0);

	v4l2_ctrl_new_std(&ov16a1q->ctrls, &ov16a1q_ctrl_ops,
				V4L2_CID_EXPOSURE, 0, 0xffff, 1, 0);

	ov16a1q->link_freq = v4l2_ctrl_new_int_menu(&ov16a1q->ctrls,
						   &ov16a1q_ctrl_ops,
						   V4L2_CID_LINK_FREQ,
						   ov16a1q_link_freqs_num(ov16a1q) - 1,
						   0, ov16a1q_link_freqs_ptr(ov16a1q) );

	if (ov16a1q->link_freq)
		ov16a1q->link_freq->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	ov16a1q->pixel_rate = v4l2_ctrl_new_std(&ov16a1q->ctrls,
						   &ov16a1q_ctrl_ops,
						   V4L2_CID_PIXEL_RATE,
						   1, INT_MAX, 1,
						   ov16a1q_calc_pixel_rate(ov16a1q));

	ov16a1q->data_lanes = v4l2_ctrl_new_custom(&ov16a1q->ctrls, &nlane_cfg, NULL);
	if (ov16a1q->data_lanes)
		ov16a1q->data_lanes->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	ov16a1q->wdr = v4l2_ctrl_new_custom(&ov16a1q->ctrls, &wdr_cfg, NULL);
	ov16a1q->address = v4l2_ctrl_new_custom(&ov16a1q->ctrls, &addr_cfg, NULL);

	v4l2_ctrl_new_custom(&ov16a1q->ctrls, &fps_cfg, NULL);

	ov16a1q->sd.ctrl_handler = &ov16a1q->ctrls;

	if (ov16a1q->ctrls.error) {
		dev_err(ov16a1q->dev, "Control initialization a error  %d\n",
			ov16a1q->ctrls.error);
		rtn = ov16a1q->ctrls.error;
	}

	return rtn;
}

static int ov16a1q_register_subdev(struct ov16a1q *ov16a1q)
{
	int rtn = 0;

	v4l2_i2c_subdev_init(&ov16a1q->sd, ov16a1q->client, &ov16a1q_subdev_ops);

	ov16a1q->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	ov16a1q->sd.dev = &ov16a1q->client->dev;
	ov16a1q->sd.entity.ops = &ov16a1q_subdev_entity_ops;
	ov16a1q->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;
	snprintf(ov16a1q->sd.name, sizeof(ov16a1q->sd.name), AML_SENSOR_NAME, ov16a1q->index);

	ov16a1q->pad.flags = MEDIA_PAD_FL_SOURCE;
	rtn = media_entity_pads_init(&ov16a1q->sd.entity, 1, &ov16a1q->pad);
	if (rtn < 0) {
		dev_err(ov16a1q->dev, "Could not register media entity\n");
		goto err_return;
	}

	rtn = v4l2_async_register_subdev(&ov16a1q->sd);
	if (rtn < 0) {
		dev_err(ov16a1q->dev, "Could not register v4l2 device\n");
		goto err_return;
	}

err_return:
	return rtn;
}

int ov16a1q_init(struct i2c_client *client, void *sdrv)
{
	struct device *dev = &client->dev;
	struct ov16a1q *ov16a1q;
	struct amlsens *sensor = (struct amlsens *)sdrv;
	int ret = -EINVAL;

	ov16a1q = devm_kzalloc(dev, sizeof(*ov16a1q), GFP_KERNEL);
	if (!ov16a1q)
		return -ENOMEM;
	dev_err(dev, "i2c dev addr 0x%x, name %s \n", client->addr, client->name);

	ov16a1q->dev = dev;
	ov16a1q->client = client;
	ov16a1q->client->addr = OV16A1Q_SLAVE_ID;
	ov16a1q->gpio = &sensor->gpio;

	/*
	I2C registration, register address 16 bits, data 8 bits
	*/
	ov16a1q->regmap = devm_regmap_init_i2c(client, &ov16a1q_regmap_config);
	if (IS_ERR(ov16a1q->regmap)) {
		dev_err(dev, "Unable to initialize I2C\n");
		return -ENODEV;
	}

	/*index = 1*/
	if (of_property_read_u32(dev->of_node, "index", &ov16a1q->index)) {
		dev_err(dev, "Failed to read sensor index. default to 0\n");
		ov16a1q->index = 0;
	}

	mutex_init(&ov16a1q->lock);

	/* Power on the device to match runtime PM state below */
	ret = ov16a1q_power_on(ov16a1q->dev, ov16a1q->gpio);
	if (ret < 0) {
		dev_err(dev, "Could not power on the device\n");
		return -ENODEV;
	}

	/*
	 * Initialize the frame format. In particular, ov16a1q->current_mode
	 * and ov16a1q->bpp are set to defaults: ov16a1q_calc_pixel_rate() call
	 * below in ov16a1q_ctrls_init relies on these fields.
	 */
	ov16a1q_entity_init_cfg(&ov16a1q->sd, NULL);

	ret = ov16a1q_ctrls_init(ov16a1q);
	if (ret) {
		dev_err(ov16a1q->dev, "Error ctrls init\n");
		goto free_ctrl;
	}

	ret = ov16a1q_register_subdev(ov16a1q);
	if (ret) {
		dev_err(ov16a1q->dev, "Error register subdev\n");
		goto free_entity;
	}

	dev_info(ov16a1q->dev, "probe done \n");

	return 0;

free_entity:
	media_entity_cleanup(&ov16a1q->sd.entity);
free_ctrl:
	v4l2_ctrl_handler_free(&ov16a1q->ctrls);
	mutex_destroy(&ov16a1q->lock);

	return ret;
}

int ov16a1q_deinit(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ov16a1q *ov16a1q = to_ov16a1q(sd);

	v4l2_async_unregister_subdev(sd);
	media_entity_cleanup(&sd->entity);
	v4l2_ctrl_handler_free(sd->ctrl_handler);

	mutex_destroy(&ov16a1q->lock);

	ov16a1q_power_off(ov16a1q->dev, ov16a1q->gpio);

	return 0;
}

int ov16a1q_sensor_id(struct i2c_client *client)
{
	u32 id = 0;
	u8 val = 0;
	int rtn = -EINVAL;

	i2c_read_a16d8(client, OV16A1Q_SLAVE_ID, 0x3035, &val);
	id |= (val << 16);
	i2c_read_a16d8(client, OV16A1Q_SLAVE_ID, 0x3036, &val);
	id |= (val << 8);
	i2c_read_a16d8(client, OV16A1Q_SLAVE_ID, 0x3037, &val);
	id |= val;

	if (id != OV16A1Q_ID) {
		dev_info(&client->dev, "Failed to get ov16a1q id: 0x%x\n", id);
		return rtn;
	} else {
		dev_err(&client->dev, "success get ov16a1q id 0x%x", id);
	}

	return 0;
}

