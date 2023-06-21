// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2023 Amlogic Corporation

#include <linux/acpi.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>

#define DW9800W_ID              0xf2

#define DW9800W_MAX_FOCUS_POS	1023
#define DW9800W_FOCUS_STEPS	1

#define DW9800W_CTRL_STEPS	16
#define DW9800W_CTRL_DELAY_US	1000

#define DW9800W_ID_REG          0x00
#define DW9800W_VER_REG         0x01
#define DW9800W_CTL_REG	        0x02
#define DW9800W_MSB_REG		0x03
#define DW9800W_LSB_REG		0x04
#define DW9800W_STATUS_REG	0x05
#define DW9800W_MODE_REG	0x06
#define DW9800W_RESONANCE_REG	0x07

#define MAX_RETRY		10

struct dw9800w_device {
	struct v4l2_ctrl_handler ctrls_vcm;
	struct v4l2_subdev sd;
	u16 current_val;
};

static inline struct dw9800w_device *sd_to_dw9800w_vcm(
					struct v4l2_subdev *subdev)
{
	return container_of(subdev, struct dw9800w_device, sd);
}

static int dw9800w_read_reg(struct i2c_client *client, char addr)
{
	int ret = 0;
	char value = 0;

	ret = i2c_master_send(client, &addr, sizeof(addr));
	if (ret < 0) {
		dev_err(&client->dev, "Failed to send ret = %d\n", ret);
		return ret;
	}

	ret = i2c_master_recv(client, &value, sizeof(value));
	if (ret < 0) {
		dev_err(&client->dev, "Failed to recv ret = %d\n", ret);
		return ret;
	}

	return value;
}

static int dw9800w_write_reg(struct i2c_client *client, char addr, char val)
{
	int ret = 0;
	char buff[2] = {addr, val};

	ret = i2c_master_send(client, buff, sizeof(buff));
	if (ret < 0)
		dev_err(&client->dev, "Failed to write ret = %d\n", ret);

	return ret;
}

static int dw9800w_get_id(struct i2c_client *client)
{
	int ret = -EINVAL;

	ret = dw9800w_read_reg(client, DW9800W_ID_REG);
	if (ret != DW9800W_ID) {
		dev_err(&client->dev, "Failed to get dw9800w id\n");
		return ret;
	}

	dev_err(&client->dev, "Success to get dw9800w id\n");

	return 0;
}

static int dw9800w_i2c_check(struct i2c_client *client)
{
	return dw9800w_read_reg(client, DW9800W_STATUS_REG);
}

static int dw9800w_set_dac(struct i2c_client *client, u16 data)
{
	int val, ret;
	const char tx_data[3] = {
		DW9800W_MSB_REG, ((data >> 8) & 0x03), (data & 0xff)
	};

	ret = readx_poll_timeout(dw9800w_i2c_check, client, val, val <= 0,
			DW9800W_CTRL_DELAY_US, MAX_RETRY * DW9800W_CTRL_DELAY_US);

	if (ret || val < 0) {
		if (ret) {
			dev_warn(&client->dev,
				"Cannot do the write operation because VCM is busy\n");
		}

		return ret ? -EBUSY : val;
	}

	ret = i2c_master_send(client, tx_data, sizeof(tx_data));
	if (ret < 0) {
		dev_err(&client->dev, "I2C write MSB fail ret=%d\n", ret);

		return ret;
	}

	dev_dbg(&client->dev, "position: %u\n", data);

	return 0;
}

static int dw9800w_set_ctrl(struct v4l2_ctrl *ctrl)
{
	struct i2c_client *client = NULL;
	struct dw9800w_device *dev_vcm = NULL;

	dev_vcm = container_of(ctrl->handler, struct dw9800w_device, ctrls_vcm);

	if (ctrl->id == V4L2_CID_FOCUS_ABSOLUTE) {
		client = v4l2_get_subdevdata(&dev_vcm->sd);
		dev_vcm->current_val = ctrl->val;

		return dw9800w_set_dac(client, ctrl->val);
	}

	return -EINVAL;
}

static const struct v4l2_ctrl_ops dw9800w_vcm_ctrl_ops = {
	.s_ctrl = dw9800w_set_ctrl,
};

static int dw9800w_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	dw9800w_write_reg(client, DW9800W_CTL_REG, 0x02);
	dw9800w_write_reg(client, DW9800W_MODE_REG, 0x40);
	dw9800w_write_reg(client, DW9800W_RESONANCE_REG, 0x79);

	dw9800w_set_dac(client, 0);

	return 0;
}

static int dw9800w_close(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	return 0;
}

static const struct v4l2_subdev_internal_ops dw9800w_int_ops = {
	.open = dw9800w_open,
	.close = dw9800w_close,
};

static const struct v4l2_subdev_ops dw9800w_ops = { };

static void dw9800w_subdev_cleanup(struct dw9800w_device *dw9800w_dev)
{
	v4l2_async_unregister_subdev(&dw9800w_dev->sd);
	v4l2_ctrl_handler_free(&dw9800w_dev->ctrls_vcm);
	media_entity_cleanup(&dw9800w_dev->sd.entity);
}

static int dw9800w_init_controls(struct dw9800w_device *dev_vcm)
{
	struct v4l2_ctrl_handler *hdl = &dev_vcm->ctrls_vcm;
	const struct v4l2_ctrl_ops *ops = &dw9800w_vcm_ctrl_ops;
	struct i2c_client *client = v4l2_get_subdevdata(&dev_vcm->sd);

	v4l2_ctrl_handler_init(hdl, 1);

	v4l2_ctrl_new_std(hdl, ops, V4L2_CID_FOCUS_ABSOLUTE,
			  0, DW9800W_MAX_FOCUS_POS, DW9800W_FOCUS_STEPS, 0);

	dev_vcm->sd.ctrl_handler = hdl;
	if (hdl->error) {
		dev_err(&client->dev, "%s fail error: 0x%x\n",
			__func__, hdl->error);
		return hdl->error;
	}

	return 0;
}

static int dw9800w_probe(struct i2c_client *client)
{
	int ret;
	struct dw9800w_device *dw9800w_dev;

	dw9800w_dev = devm_kzalloc(&client->dev, sizeof(*dw9800w_dev), GFP_KERNEL);
	if (dw9800w_dev == NULL) {
		dev_err(&client->dev, "Failed to alloc dw9800w_dev\n");
		return -ENOMEM;
	}

	ret = dw9800w_get_id(client);
	if (ret)
		return ret;

	v4l2_i2c_subdev_init(&dw9800w_dev->sd, client, &dw9800w_ops);
	dw9800w_dev->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	dw9800w_dev->sd.internal_ops = &dw9800w_int_ops;

	ret = dw9800w_init_controls(dw9800w_dev);
	if (ret)
		goto err_cleanup;

	ret = media_entity_pads_init(&dw9800w_dev->sd.entity, 0, NULL);
	if (ret < 0)
		goto err_cleanup;

	dw9800w_dev->sd.entity.function = MEDIA_ENT_F_LENS;

	ret = v4l2_async_register_subdev(&dw9800w_dev->sd);
	if (ret < 0)
		goto err_cleanup;

	dev_err(&client->dev, "LKK: success probe dw9800w\n");

	return 0;

err_cleanup:
	v4l2_ctrl_handler_free(&dw9800w_dev->ctrls_vcm);
	media_entity_cleanup(&dw9800w_dev->sd.entity);

	return ret;
}

static int dw9800w_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct dw9800w_device *dw9800w_dev = sd_to_dw9800w_vcm(sd);

	dw9800w_subdev_cleanup(dw9800w_dev);

	return 0;
}

static int __maybe_unused dw9800w_vcm_suspend(struct device *dev)
{
	int ret, val;
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct dw9800w_device *dw9800w_dev = sd_to_dw9800w_vcm(sd);
	const char tx_data[2] = {DW9800W_CTL_REG, 0x01};

	for (val = dw9800w_dev->current_val & ~(DW9800W_CTRL_STEPS - 1);
	     val >= 0; val -= DW9800W_CTRL_STEPS) {
		ret = dw9800w_set_dac(client, val);
		if (ret)
			dev_err_once(dev, "%s I2C failure: %d", __func__, ret);

		usleep_range(DW9800W_CTRL_DELAY_US, DW9800W_CTRL_DELAY_US + 10);
	}

	/* Power down */
	ret = i2c_master_send(client, tx_data, sizeof(tx_data));
	if (ret < 0) {
		dev_err(&client->dev, "I2C write CTL fail ret = %d\n", ret);
		return ret;
	}

	return 0;
}

static int  __maybe_unused dw9800w_vcm_resume(struct device *dev)
{
	int ret, val;
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct dw9800w_device *dw9800w_dev = sd_to_dw9800w_vcm(sd);
	const char tx_data[2] = {DW9800W_CTL_REG, 0x00};

	ret = i2c_master_send(client, tx_data, sizeof(tx_data));
	if (ret < 0) {
		dev_err(&client->dev, "I2C write CTL fail ret = %d\n", ret);
		return ret;
	}

	for (val = dw9800w_dev->current_val % DW9800W_CTRL_STEPS;
	     val < dw9800w_dev->current_val + DW9800W_CTRL_STEPS - 1;
	     val += DW9800W_CTRL_STEPS) {
		ret = dw9800w_set_dac(client, val);
		if (ret)
			dev_err_ratelimited(dev, "%s I2C failure: %d", __func__, ret);

		usleep_range(DW9800W_CTRL_DELAY_US, DW9800W_CTRL_DELAY_US + 10);
	}

	return 0;
}

static const struct of_device_id dw9800w_of_table[] = {
	{ .compatible = "dongwoon, dw9800w" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, dw9800w_of_table);

static const struct dev_pm_ops dw9800w_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(dw9800w_vcm_suspend, dw9800w_vcm_resume)
	SET_RUNTIME_PM_OPS(dw9800w_vcm_suspend, dw9800w_vcm_resume, NULL)
};

static struct i2c_driver dw9800w_i2c_driver = {
	.driver = {
		.name = "dw9800w",
		.pm = &dw9800w_pm_ops,
		.of_match_table = dw9800w_of_table,
	},
	.probe_new = dw9800w_probe,
	.remove = dw9800w_remove,
};

module_i2c_driver(dw9800w_i2c_driver);

MODULE_AUTHOR("Keke Li <keke.li@amlogic.com>");
MODULE_DESCRIPTION("DW9800W driver");
MODULE_LICENSE("GPL");
