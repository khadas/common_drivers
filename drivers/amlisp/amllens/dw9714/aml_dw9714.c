#include <linux/acpi.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>

#define V4L2_CID_AML_BASE            (V4L2_CID_BASE + 0x1000)
#define V4L2_CID_AML_LENS_MOVING     (V4L2_CID_AML_BASE + 0x006)


#define DW9714_MAX_FOCUS_POS	1023
#define DW9714_FOCUS_STEPS	1
#define DW9714_MIN_STEP 		  64;

#define STEP_PERIOD_81_US	0x0
#define STEP_PERIOD_162_US	0x1
#define STEP_PERIOD_324_US	0x2
#define STEP_PERIOD_648_US	0x3


#define PER_STEP_CODES_0	(0x0<<2)
#define PER_STEP_NO_SRC 	PER_STEP_CODES_0
#define PER_STEP_CODES_1	(0x1<<2)
#define PER_STEP_CODES_2	(0x2<<2)
#define PER_STEP_CODES_3	(0x3<<2)

#define DW9714_PACKET_BYTES  2
#define MAX_RETRY		10

struct dw9714_device {
	struct v4l2_ctrl_handler ctrls_vcm;
	struct v4l2_subdev sd;
	u16 current_val;
};


static inline struct dw9714_device *sd_to_dw9714_vcm(struct v4l2_subdev *subdev)
{
	return container_of(subdev, struct dw9714_device, sd);
}

static int dw9714_read_reg(struct i2c_client *client, uint8_t *data, int bytes)
{
	int msg_count = 0;
	int i = 0;
	int rc = -1;
	uint16_t saddr = client->addr;

	struct i2c_msg msgs[] = {
		{
			.addr  = saddr,
			.flags = 0,
			.len   = 0,
			.buf   = 0,
		},
		{
			.addr  = saddr,
			.flags = I2C_M_RD,
			.len   = bytes,
			.buf   = data,
		},
	};

	msg_count = sizeof(msgs) / sizeof(msgs[0]);

	for (i = 0; i < 5; i++) {
		rc = i2c_transfer(client->adapter, msgs, msg_count);
		if (rc == msg_count) {
			break;
		}
	}

	if (rc < 0) {
		pr_err("%s:failed to read reg data: rc %d, saddr 0x%x\n", __func__,
					rc, saddr);
		return rc;
	}

	return 0;
}

static int dw9714_write_reg(struct i2c_client *client, uint8_t *data, int bytes)
{
	int msg_count = 0;
	int i = 0;
	int rc = -1;
	uint16_t saddr = client->addr;

	struct i2c_msg msgs[] = {
		{
			.addr  = saddr,
			.flags = 0, // write
			.len   = bytes,
			.buf   = data,
		}
	};

	msg_count = sizeof(msgs) / sizeof(msgs[0]);

	for (i = 0; i < 5; i++) {
		rc = i2c_transfer(client->adapter, msgs, msg_count);
		if (rc == msg_count) {
			break;
		}
	}

	if (rc < 0) {
		pr_err("%s:failed to write reg data: rc %d, saddr 0x%x\n", __func__,
					rc, saddr);
		return rc;
	}

	return 0;
}

static int dw9714_set_dac(struct i2c_client *client, u16 position)
{

	uint8_t data[DW9714_PACKET_BYTES];
	uint16_t pos = position;
	pr_err("dw9714_set_dac new position=%d, pos= %d",position, pos);
	if (0 != dw9714_read_reg(client, data, DW9714_PACKET_BYTES))
	{
		pr_err("Fail to read dw9714_dev");
		return -1;
	}

	data[0] = (pos >> 4) & 0b00111111;// PD & FLAG bits must be low. D9 - D4
	data[1] = ( (pos & 0x0f) << 4 ) + (data[1] & 0x0f); // KEEP s[3:0]

	if (0 != dw9714_write_reg(client, data, DW9714_PACKET_BYTES))
	{
		pr_err("Fail to write dw9714_dev");
		return -1;
	}
	return 0;
}

static int dw9714_get_moving_status (struct i2c_client *client, int *status)
{
	uint8_t data[DW9714_PACKET_BYTES];
	uint8_t ret = 0;
	if (0 == dw9714_read_reg(client, data, DW9714_PACKET_BYTES)) {
		ret = (data[0] >> 6 ) & 0x01; // FLAG bit
		pr_err("moing is %d", ret);
		*status = ret;
		return 0;
	}
	pr_err("Fail to read reg");
	return -1;
}

static int dw9714_get_ctrl(struct v4l2_ctrl *ctrl)
{
	struct i2c_client *client = NULL;
	struct dw9714_device *dev_vcm = NULL;
	pr_err("dw9714_get_ctrl");
	dev_vcm = container_of(ctrl->handler, struct dw9714_device, ctrls_vcm);
	if (ctrl->id == V4L2_CID_AML_LENS_MOVING) {
		client = v4l2_get_subdevdata(&dev_vcm->sd);
		return dw9714_get_moving_status(client, &ctrl->val);
	}
	pr_err("id is err");
	return -EINVAL;
}

static int dw9714_set_ctrl(struct v4l2_ctrl *ctrl)
{
	struct i2c_client *client = NULL;
	struct dw9714_device *dev_vcm = NULL;

	dev_vcm = container_of(ctrl->handler, struct dw9714_device, ctrls_vcm);

	if (ctrl->id == V4L2_CID_FOCUS_ABSOLUTE) {
		client = v4l2_get_subdevdata(&dev_vcm->sd);
		dev_vcm->current_val = ctrl->val;

		return dw9714_set_dac(client, ctrl->val);
	}

	return -EINVAL;
}

static const struct v4l2_ctrl_ops dw9714_vcm_ctrl_ops = {
	.s_ctrl = dw9714_set_ctrl,
	.g_volatile_ctrl = dw9714_get_ctrl,
};

static const struct v4l2_ctrl_config isp_v4l2_ctrl_lens_moving = {
	.ops = &dw9714_vcm_ctrl_ops,
	.id = V4L2_CID_AML_LENS_MOVING,
	.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	.name = "lens is moving",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.min = 0,
	.max = 1,
	.step = 1,
	.def = 1,
};

static int dw9714_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	uint8_t data[DW9714_PACKET_BYTES];
	if (0 == dw9714_read_reg(client, data, DW9714_PACKET_BYTES)) {
		pr_err("%s success, byte 0 & 1 0x%x  0x%x ", __func__, data[0], data[1]);
	} else {
		pr_err("Fail to read");
		return -1;
	}

	// initial value.
	data[0] = 0X00;
	data[1] = PER_STEP_CODES_2 | STEP_PERIOD_81_US;
	if (0 != dw9714_write_reg(client, data, DW9714_PACKET_BYTES) ) {
		pr_err("Fail to write dw9714_dev");
	}

	if (0 == dw9714_read_reg(client, data, DW9714_PACKET_BYTES)) {
		pr_err("%s after initial value, read back byte 0 & 1 0x%x  0x%x ", __func__, data[0], data[1]);
	}
	return 0;
}

static int dw9714_close(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	return 0;
}

static const struct v4l2_subdev_internal_ops dw9714_int_ops = {
	.open = dw9714_open,
	.close = dw9714_close,
};

static const struct v4l2_subdev_ops dw9714_ops = { };

static void dw9714_subdev_cleanup(struct dw9714_device *dw9714_dev)
{
	v4l2_async_unregister_subdev(&dw9714_dev->sd);
	v4l2_ctrl_handler_free(&dw9714_dev->ctrls_vcm);
	media_entity_cleanup(&dw9714_dev->sd.entity);
}

static int dw9714_init_controls(struct dw9714_device *dev_vcm)
{
	struct v4l2_ctrl_handler *hdl = &dev_vcm->ctrls_vcm;
	const struct v4l2_ctrl_ops *ops = &dw9714_vcm_ctrl_ops;
	struct i2c_client *client = v4l2_get_subdevdata(&dev_vcm->sd);


	v4l2_ctrl_handler_init(hdl, 2);

	v4l2_ctrl_new_std(hdl, ops, V4L2_CID_FOCUS_ABSOLUTE, 0, DW9714_MAX_FOCUS_POS, DW9714_FOCUS_STEPS, 0);
	v4l2_ctrl_new_custom(hdl, &isp_v4l2_ctrl_lens_moving, NULL);
	dev_vcm->sd.ctrl_handler = hdl;
	if (hdl->error) {
		dev_err(&client->dev, "%s fail error: 0x%x\n",
			__func__, hdl->error);
		return hdl->error;
	}

	return 0;
}

static int dw9714_probe(struct i2c_client *client)
{
	int ret = 0;
	struct dw9714_device *dw9714_dev;

	dw9714_dev = devm_kzalloc(&client->dev, sizeof(*dw9714_dev), GFP_KERNEL);
	if (dw9714_dev == NULL) {
		dev_err(&client->dev, "Fail to alloc dw9714_dev");
		return -ENOMEM;
	}

	v4l2_i2c_subdev_init(&dw9714_dev->sd, client, &dw9714_ops);
	dw9714_dev->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	dw9714_dev->sd.internal_ops = &dw9714_int_ops;

	ret = dw9714_init_controls(dw9714_dev);
	if (ret)
		goto err_cleanup;
	ret = media_entity_pads_init(&dw9714_dev->sd.entity, 0, NULL);
	if (ret < 0)
		goto err_cleanup;
	dw9714_dev->sd.entity.function = MEDIA_ENT_F_LENS;

	ret = v4l2_async_register_subdev(&dw9714_dev->sd);
	if (ret < 0)
		goto err_cleanup;

	dev_err(&client->dev, "CY: success probe dw9714\n");

	return 0;

err_cleanup:
	v4l2_ctrl_handler_free(&dw9714_dev->ctrls_vcm);
	media_entity_cleanup(&dw9714_dev->sd.entity);

	return ret;
}
static int dw9714_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct dw9714_device *dw9714_dev = sd_to_dw9714_vcm(sd);

	dw9714_subdev_cleanup(dw9714_dev);

	return 0;

}

static int __maybe_unused dw9714_vcm_suspend(struct device *dev)
{

	return 0;
}

static int	__maybe_unused dw9714_vcm_resume(struct device *dev)
{
	return 0;
}

static const struct of_device_id dw9714_of_table[] = {
	{ .compatible = "dw, dw9714" },
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, dw9714_of_table);

static const struct dev_pm_ops dw9714_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(dw9714_vcm_suspend, dw9714_vcm_resume)
	SET_RUNTIME_PM_OPS(dw9714_vcm_suspend, dw9714_vcm_resume, NULL)
};

static struct i2c_driver dw9714_i2c_driver = {
	.driver = {
		.name = "dw9714",
		.pm = &dw9714_pm_ops,
		.of_match_table = dw9714_of_table,
	},
	.probe_new = dw9714_probe,
	.remove = dw9714_remove,
};

module_i2c_driver(dw9714_i2c_driver);

MODULE_AUTHOR("Yang Chen <yang.chen@amlogic.com>");
MODULE_DESCRIPTION("DW9714 driver");
MODULE_LICENSE("GPL");
