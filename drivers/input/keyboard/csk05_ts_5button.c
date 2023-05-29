// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

/**************************************************************************
 *  csk05_ts_5button.c
 *
 *  Create Date : 2019/07/12
 *
 *  Modify Date : 2021/05/12
 *
 *  Create by   : jian.cai@amlogic.com
 *
 *  Modify by   : ziheng.li@amlogic.com
 *
 *  Version     : 1.0.1 , 2021/05/12
 *
 **************************************************************************/
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/firmware.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/dma-mapping.h>
#include <linux/gameport.h>
#include <linux/moduleparam.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/atomic.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include "csk05_reg.h"
/****************************************************************
 *
 * Marco
 *
 ***************************************************************/
#define CSK05_TS_NAME		"hynitron, csk05_ts"
#define CSK05_DEVICE		"csk05"

//#define AML_CSK05_CHECK

#ifdef AML_CSK05_CHECK
#define CSK05_CHECK_ENABLE 1
#endif

#define KEY_TYPE_RELEASE    0
#define KEY_TYPE_PRESS      1

struct touchkey_st {
	char keyname[32];
	int keycode;
};

/****************************************************************
 *
 * Touch Key Driver
 *
 ***************************************************************/
struct csk05_ts_data {
	struct input_dev	*input_dev;
	struct device_node *node;
	int irq;

	struct i2c_client *client;

	unsigned int reset_gpio;
	unsigned int interrupt_gpio;

	unsigned char last_key;
	struct touchkey_st *user_keys;
	unsigned int user_keys_count;

	unsigned int g_reg_cfg_freq;
	unsigned int g_reg_cfg_sensitivity;
	unsigned int g_reg_cfg_idac;
	unsigned int g_reg_cfg_noise;

#ifdef AML_CSK05_CHECK
	struct task_struct *csk05_check_task;
	struct mutex csk05_i2c_mutex;	/*i2c read and write mutex*/
	wait_queue_head_t csk05_check_task_wq;
	atomic_t csk05_check_task_wakeup;
#endif

};

static int csk05_gpio_init(struct csk05_ts_data *csk05_ts)
{
	if (csk05_ts->node) {
		csk05_ts->reset_gpio = of_get_named_gpio_flags(csk05_ts->node,
			"reset_pin", 0, NULL);
		if (gpio_request(csk05_ts->reset_gpio, "csk05-reset")) {
			pr_err("%s %d gpio %d request failed!\n", __func__,
				__LINE__, csk05_ts->reset_gpio);
			return -EINVAL;
		}
		gpio_direction_output(csk05_ts->reset_gpio, 1);

		csk05_ts->interrupt_gpio = of_get_named_gpio_flags(csk05_ts->node,
			"interrupt_pin", 0, NULL);
		if (gpio_request(csk05_ts->interrupt_gpio, "csk05-irq-gpio")) {
			pr_err("%s %d gpio %d request failed!\n", __func__,
				__LINE__, csk05_ts->interrupt_gpio);
			return -EINVAL;
		}
		gpio_direction_input(csk05_ts->interrupt_gpio);
	}
	return 0;
}

static void csk05_gpio_deinit(struct csk05_ts_data *csk05_ts)
{
	gpio_free(csk05_ts->reset_gpio);
	gpio_free(csk05_ts->interrupt_gpio);
}

static void csk05_gpio_reset(struct csk05_ts_data *csk05_ts)
{
	gpio_set_value(csk05_ts->reset_gpio, 0);
	msleep(30);
	gpio_set_value(csk05_ts->reset_gpio, 1);
	msleep(30);
}

static unsigned int i2c_write_reg(struct csk05_ts_data *csk05_ts,
	unsigned char reg, unsigned int reg_data)
{
	int ret;
	unsigned char buffer[2];
	struct i2c_msg msg[1];

	buffer[0] = reg;
	buffer[1] = reg_data;

	msg[0].addr = csk05_ts->client->addr;
	msg[0].flags = I2C_M_TEN;
	msg[0].buf = buffer;
	msg[0].len = sizeof(buffer);

	if (!csk05_ts->client) {
		pr_err("%s %d client is NULL\n", __func__, __LINE__);
		return -EINVAL;
	}

#ifdef AML_CSK05_CHECK
	mutex_lock(&csk05_ts->csk05_i2c_mutex);
#endif
	ret = i2c_transfer(csk05_ts->client->adapter, msg, 1);
#ifdef AML_CSK05_CHECK
	mutex_unlock(&csk05_ts->csk05_i2c_mutex);
#endif

	if (ret < 0)
		pr_err("%s %d i2c read error: %d\n", __func__, __LINE__, ret);

	return ret;
}

static unsigned int i2c_read_reg(struct csk05_ts_data *csk05_ts,
	unsigned char reg, unsigned char *buf, unsigned char num)
{
	int ret;

	struct i2c_msg msgs[] = {
		{
			.addr	= csk05_ts->client->addr,
			.flags	= 0,
			.len	= 1,
			.buf	= &reg,
		},
		{
			.addr	= csk05_ts->client->addr,
			.flags	= I2C_M_RD,
			.len	= num,
			.buf	= buf,
		},
	};

	if (!csk05_ts->client) {
		pr_err("%s %d client is NULL\n", __func__, __LINE__);
		return -EINVAL;
	}

#ifdef AML_CSK05_CHECK
	mutex_lock(&csk05_ts->csk05_i2c_mutex);
#endif

	ret = i2c_transfer(csk05_ts->client->adapter, msgs, 2);

#ifdef AML_CSK05_CHECK
	mutex_unlock(&csk05_ts->csk05_i2c_mutex);
#endif

	if (ret < 0)
		pr_err("%s %d i2c read error: %d\n", __func__, __LINE__,  ret);

	return ret;
}

static void csk05_on_key_event(struct input_dev	*input_dev,
	const char *keyname, int keycode, int event)
{
	pr_debug("%s %d  keycode:%d keyname:%s event:%s\n", __func__, __LINE__,
		keycode, keyname, (event == KEY_TYPE_PRESS) ? "press" : "release");

	input_report_key(input_dev, keycode, event);
	input_sync(input_dev);
}

/****************************************************************
 *
 * csk05 initial register @ mobile active
 *
 * The smaller the GFREQ, the higher the sensitivity and the need to match the resistance.
 * The bigger the GSENSITIVITY,the higher the sensitivity.
 * The smaller the GIDAC, the higher the sensitivity.
 ***************************************************************/

static int csk05_init_reg(struct csk05_ts_data *csk05_ts)
{
	unsigned char buff[2] = {0};
	unsigned int i;

	for (i = 0; i < 3; i++) {
		i2c_read_reg(csk05_ts, SCANCR, buff, 1);
		if (buff[0] == 0)
			break;

		pr_err("%s %d  Reg[0x%02x]=0x%02x, %d time\n",
				__func__, __LINE__, SCANCR, buff[0], i);
		msleep(30);
	}
	if (i >= 3)
		return -EINVAL;

	buff[0] = 0x1f;
	i2c_write_reg(csk05_ts, CHANNELEN0, buff[0]);

	buff[0] = (csk05_ts->g_reg_cfg_freq == 0 ?
			6 : csk05_ts->g_reg_cfg_freq & 0xFF);

	i2c_write_reg(csk05_ts, GFREQ, buff[0]);

	buff[0] = 200;
	buff[1] = 0;
	i2c_write_reg(csk05_ts, GFINGERTH_L, buff[0]);
	i2c_write_reg(csk05_ts, GFINGERTH_H, buff[1]);

	buff[0] = (csk05_ts->g_reg_cfg_sensitivity == 0 ?
			60 : csk05_ts->g_reg_cfg_sensitivity & 0xFF);

	i2c_write_reg(csk05_ts, GSENSITIVITY, buff[0]);

	buff[0] = (csk05_ts->g_reg_cfg_idac == 0 ?
			40 : csk05_ts->g_reg_cfg_idac & 0xFF);

	i2c_write_reg(csk05_ts, GIDAC, buff[0]);

	buff[0] = (csk05_ts->g_reg_cfg_noise == 0 ?
			80 : csk05_ts->g_reg_cfg_noise & 0xFF);

	i2c_write_reg(csk05_ts, GNOISETH, buff[0]);

	buff[0] = 0x20;
	i2c_write_reg(csk05_ts, IRQCR, buff[0]);

	buff[0] = 0x05;
	i2c_write_reg(csk05_ts, MOTIONSR, buff[0]);

	buff[0] = 0x01;
	i2c_write_reg(csk05_ts, SCANCR, buff[0]);

	return 0;
}

static irqreturn_t csk05_ts_eint_work(int irq, void *param)
{
	unsigned char buff[3] = {0};
	char new_key = -1;
	struct csk05_ts_data *csk05_ts = (struct csk05_ts_data *)param;

	i2c_read_reg(csk05_ts, MOTIONSR, buff, 3);

	new_key = buff[2];

	if (csk05_ts->user_keys) {
		int i = 0;

		for (i = 0; i < csk05_ts->user_keys_count; i++) {
			if (new_key & (1 << i)) {
				csk05_on_key_event(csk05_ts->input_dev,
					csk05_ts->user_keys[i].keyname,
					csk05_ts->user_keys[i].keycode,
					KEY_TYPE_PRESS);
				csk05_ts->last_key = buff[2];
			}
		}

		if (buff[2] == 0)
			for (i = 0; i < csk05_ts->user_keys_count; i++)
				if (csk05_ts->last_key & (1 << i))
					csk05_on_key_event(csk05_ts->input_dev,
						csk05_ts->user_keys[i].keyname,
						csk05_ts->user_keys[i].keycode, KEY_TYPE_RELEASE);
	}

	return IRQ_HANDLED;
}

/****************************************************************
 *
 * csk05 eint set
 *
 ***************************************************************/
static int csk05_ts_setup_eint(struct csk05_ts_data *csk05_ts)
{
	if (csk05_ts->node) {
		csk05_ts->irq = gpio_to_irq(csk05_ts->interrupt_gpio);

		if (!csk05_ts->irq) {
			pr_err("%s %d irq_of_parse_and_map fail!!\n", __func__, __LINE__);
			return -EINVAL;
		}

		if (request_threaded_irq(csk05_ts->irq, NULL,
		     csk05_ts_eint_work,
		     IRQ_TYPE_EDGE_FALLING | IRQF_ONESHOT, CSK05_DEVICE, (void *)csk05_ts)) {
			pr_err("%s  %d IRQ LINE NOT AVAILABLE!!\n", __func__, __LINE__);
			return -EINVAL;
		}
	} else {
		pr_err("%s %d null irq node!!\n", __func__, __LINE__);
		return -EINVAL;
	}
	return 0;
}

#ifdef AML_CSK05_CHECK
static int read_csk05_reg(struct csk05_ts_data *csk05_ts)
{
	int ret = 0;
	unsigned char buf1[1] = {0x00};

	ret = i2c_read_reg(csk05_ts, SCANCR, buf1, 1);
	if (ret < 0) {
		pr_err("%s %d i2c read  fail! ret=%d\n", __func__, __LINE__, ret);
		return 1;
	}

	ret = (buf1[0] == 0x00) ? 1 : ret;

	return ret;
}

static int csk05_check(struct csk05_ts_data *csk05_ts)
{
	if (read_csk05_reg(csk05_ts) == 1)
		return 1;

	return 0;
}

static int csk05_recovery(struct csk05_ts_data *csk05_ts)
{
	csk05_gpio_reset(csk05_ts);
	msleep(30);
	csk05_init_reg(csk05_ts);
	return 0;
}

static unsigned int need_do_csk05_check(void)
{
	int ret = 0;

	if (CSK05_CHECK_ENABLE)
		ret = 1;

	return ret;
}

static void csk05_is_check_enable(struct csk05_ts_data *csk05_ts, int enable)
{
	if (need_do_csk05_check()) {
		if (enable) {
			atomic_set(&csk05_ts->csk05_check_task_wakeup, 1);
			wake_up_interruptible(&csk05_ts->csk05_check_task_wq);
		} else {
			atomic_set(&csk05_ts->csk05_check_task_wakeup, 0);
		}
	}
}

static int csk05_check_thread(void *data)
{
	int ret;
	int try_cnt = 10;
	struct csk05_ts_data *csk05_ts = (struct csk05_ts_data *)data;

	pr_debug("%s %d start!\n", __func__, __LINE__);

	while (1) {
		msleep(1000);
		ret = wait_event_interruptible(csk05_ts->csk05_check_task_wq,
			atomic_read(&csk05_ts->csk05_check_task_wakeup));
		if (ret < 0) {
			pr_err("%s %d csk05 check thread wakeup fail!\n", __func__, __LINE__);
			continue;
		}

		ret = csk05_check(csk05_ts);
		if (ret == 1) {
			pr_info("%s %d csk05 check fail!,start csk05 recovery!\n",
				__func__, __LINE__);

			while (try_cnt--) {
				pr_debug("%s %d recovery try: %d\n", __func__, __LINE__, try_cnt);

				msleep(200);

				csk05_recovery(csk05_ts);

				ret = csk05_check(csk05_ts);
				if (ret == 0) {
					pr_debug("%s %d csk05 recovery success!\n",
						__func__, __LINE__);
					try_cnt = 10;
					break;
				}

				if (try_cnt == 0) {
					pr_debug("%s %d exit csk05 check!\n", __func__, __LINE__);
					csk05_is_check_enable(csk05_ts, 0);
					csk05_recovery(csk05_ts);
				}
			}
		}

		if (kthread_should_stop())
			break;
	}

	return 0;
}

#endif

static int csk05_init_key_defines(struct csk05_ts_data *csk05_ts)
{
	if (of_property_read_u32(csk05_ts->node, "key_num", &csk05_ts->user_keys_count)) {
		pr_err("%s %d read key_num failed\n", __func__, __LINE__);
		return -EINVAL;
	}

	csk05_ts->user_keys =
		kzalloc(csk05_ts->user_keys_count * sizeof(struct touchkey_st), GFP_KERNEL);
	if (csk05_ts->user_keys) {
		int i = 0;

		for (i = 0; i < csk05_ts->user_keys_count; i++) {
			const char *string = NULL;

			of_property_read_u32_index(csk05_ts->node, "key_code",
				i, &csk05_ts->user_keys[i].keycode);
			of_property_read_string_index(csk05_ts->node, "key_name",
				i, &string);
			if (string)
				strncpy(csk05_ts->user_keys[i].keyname, string,
					sizeof(csk05_ts->user_keys[i].keyname));

			pr_debug("%s %d i:%d key_code:%d key_name:%s\n", __func__, __LINE__,
				i, csk05_ts->user_keys[i].keycode, csk05_ts->user_keys[i].keyname);
		}
	} else {
		pr_err("%s %d kmalloc failed for touchkey\n", __func__, __LINE__);
		return -EINVAL;
	}
	return 0;
}

static void csk05_init_reg_config(struct csk05_ts_data *csk05_ts)
{
	if (of_property_read_u32(csk05_ts->node, "freq",
			&csk05_ts->g_reg_cfg_freq) != 0) {
		csk05_ts->g_reg_cfg_freq = 6;
		pr_err("%s %d get <freq> from dts failed! default:6\n", __func__, __LINE__);
	}

	if (of_property_read_u32(csk05_ts->node, "sensitivity",
			&csk05_ts->g_reg_cfg_sensitivity) != 0) {
		csk05_ts->g_reg_cfg_sensitivity = 60;
		pr_err("%s %d get <sensitivity> from dts failed! default:60\n", __func__, __LINE__);
	}

	if (of_property_read_u32(csk05_ts->node, "idac",
			&csk05_ts->g_reg_cfg_idac) != 0) {
		csk05_ts->g_reg_cfg_idac = 40;
		pr_err("%s %d get <idac> from dts failed! default:40\n", __func__, __LINE__);
	}

	if (of_property_read_u32(csk05_ts->node, "noise",
			&csk05_ts->g_reg_cfg_noise) != 0) {
		csk05_ts->g_reg_cfg_noise = 80;
		pr_err("%s %d get <noise> from dts failed! default:80\n", __func__, __LINE__);
	}
}

/****************************************************************
 *
 * csk05 i2c driver
 *
 ***************************************************************/
static int csk05_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct csk05_ts_data *csk05_ts;
	int ret = 0;
	int i = 0;

	pr_debug("%s %d\n", __func__, __LINE__);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		ret = -ENODEV;
		goto exit_check_functionality_failed;
	}

	csk05_ts = kzalloc(sizeof(*csk05_ts), GFP_KERNEL);
	if (!csk05_ts)	{
		ret = -ENOMEM;
		pr_err("%s %d : kzalloc failed\n", __func__, __LINE__);
		goto exit_alloc_data_failed;
	}

	csk05_ts->node = client->dev.of_node;
	csk05_ts->client = client;

	ret = csk05_gpio_init(csk05_ts);
	if (ret) {
		pr_err("%s %d failed to init csk05 pinctrl.\n", __func__, __LINE__);
		goto exit_gpio_init_failed;
	}
	csk05_gpio_reset(csk05_ts);

#ifdef AML_CSK05_CHECK
	mutex_init(&csk05_ts->csk05_i2c_mutex);
	atomic_set(&csk05_ts->csk05_check_task_wakeup, 0);
#endif

	ret = csk05_init_key_defines(csk05_ts);
	if (ret) {
		pr_err("%s %d failed to init csk05 pinctrl.\n", __func__, __LINE__);
		goto exit_gpio_init_key;
	}

	i2c_set_clientdata(client, csk05_ts);

	csk05_init_reg_config(csk05_ts);
	ret = csk05_init_reg(csk05_ts);
	if (ret) {
		dev_err(&client->dev,
			"%s: failed to init csk05 reg!\n", __func__);
		goto exit_init_csk05_reg_failed;
	}

	csk05_ts->input_dev = input_allocate_device();
	if (!csk05_ts->input_dev) {
		ret = -ENOMEM;
		dev_err(&client->dev, "failed to allocate input device\n");
		goto exit_input_dev_alloc_failed;
	}

	__set_bit(EV_KEY, csk05_ts->input_dev->evbit);
	__set_bit(EV_SYN, csk05_ts->input_dev->evbit);

	for (i = 0; i < csk05_ts->user_keys_count; i++)
		__set_bit(csk05_ts->user_keys[i].keycode, csk05_ts->input_dev->keybit);

	csk05_ts->input_dev->name = CSK05_DEVICE;		//dev_name(&client->dev)
	ret = input_register_device(csk05_ts->input_dev);
	if (ret) {
		dev_err(&client->dev,
			"%s: failed to register input device: %s\n",
			__func__, dev_name(&client->dev));
		goto exit_input_register_device_failed;
	}

#ifdef AML_CSK05_CHECK
	csk05_ts->csk05_check_task = kthread_create(csk05_check_thread,
		(void *)csk05_ts, CSK05_DEVICE);
	if (IS_ERR(csk05_ts->csk05_check_task)) {
		pr_err("%s %d Unable to start kernel thread!\n", __func__, __LINE__);
		ret = PTR_ERR(csk05_ts->csk05_check_task);
		csk05_ts->csk05_check_task = NULL;
		goto exit_creat_thread_failed;
	}

	init_waitqueue_head(&csk05_ts->csk05_check_task_wq);
	if (need_do_csk05_check())
		wake_up_process(csk05_ts->csk05_check_task);

	if (need_do_csk05_check())
		csk05_is_check_enable(csk05_ts, 1);
#endif

	ret = csk05_ts_setup_eint(csk05_ts);
	if (ret) {
		dev_err(&client->dev,
			"%s: failed to create irq: %s\n",
			__func__, dev_name(&client->dev));
		goto exit_create_irq_failed;
	}

	return 0;

exit_create_irq_failed:
#ifdef AML_CSK05_CHECK
	kthread_stop(csk05_ts->csk05_check_task);
exit_creat_thread_failed:
#endif
	input_unregister_device(csk05_ts->input_dev);
exit_input_register_device_failed:
	input_free_device(csk05_ts->input_dev);
exit_input_dev_alloc_failed:
exit_init_csk05_reg_failed:
	i2c_set_clientdata(client, NULL);
exit_gpio_init_key:
	kfree(csk05_ts->user_keys);
	csk05_ts->user_keys = NULL;
exit_gpio_init_failed:
	csk05_gpio_deinit(csk05_ts);
	kfree(csk05_ts);
exit_alloc_data_failed:
exit_check_functionality_failed:
	return ret;
}

static int csk05_i2c_remove(struct i2c_client *client)
{
	struct csk05_ts_data *csk05_ts = i2c_get_clientdata(client);

	pr_debug("%s %d\n", __func__, __LINE__);
#ifdef AML_CSK05_CHECK
	kthread_stop(csk05_ts->csk05_check_task);
#endif
	input_unregister_device(csk05_ts->input_dev);
	input_free_device(csk05_ts->input_dev);

	kfree(csk05_ts->user_keys);
	csk05_ts->user_keys = NULL;

	csk05_gpio_deinit(csk05_ts);
	kfree(csk05_ts);
	i2c_set_clientdata(client, NULL);

	return 0;
}

static const struct i2c_device_id csk05_i2c_id[] = {
	{ CSK05_TS_NAME, 0 }, { }
};
MODULE_DEVICE_TABLE(i2c, csk05_i2c_id);

#ifdef CONFIG_OF
static const struct of_device_id csk05_of_match[] = {
	{.compatible = CSK05_TS_NAME},
	{},
};
#endif

static struct i2c_driver csk05_i2c_driver = {
	.probe		= csk05_i2c_probe,
	.remove		= csk05_i2c_remove,
	.id_table	= csk05_i2c_id,
	.driver	= {
		.name	= CSK05_TS_NAME,
		.owner	= THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = csk05_of_match,
#endif
	},
};

int __init csk05_ts_init(void)
{
	if (i2c_add_driver(&csk05_i2c_driver) < 0) {
		pr_err("%s %d failed to register csk05 i2c driver.\n", __func__, __LINE__);
		return -1;
	}
	return 0;
}

void __exit csk05_ts_exit(void)
{
	i2c_del_driver(&csk05_i2c_driver);
}

MODULE_AUTHOR("jian.cai@amlogic.com");
MODULE_AUTHOR("ziheng.li@amlogic.com");
MODULE_DESCRIPTION("hynitron csk05 Touch driver");
MODULE_LICENSE("GPL v2");

