// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/of.h>
#include <linux/iio/consumer.h>
#include <linux/mailbox_controller.h>
#include <linux/mailbox_client.h>
#include <linux/amlogic/aml_mbox.h>
#include <linux/amlogic/pm.h>
#include "adc_keypad.h"

#define POLL_INTERVAL_DEFAULT 25
#define KEY_JITTER_COUNT  1
#define TMP_BUF_MAX 128

static char adc_key_mode_name[MAX_NAME_LEN] = "abcdef";
static char kernelkey_en_name[MAX_NAME_LEN] = "abcdef";
static bool keypad_enable_flag = true;
struct mbox_chan *adc_mbox_chan;

static int meson_adc_kp_search_key(struct meson_adc_kp *kp)
{
	struct adc_key *key;
	int value, i;

	mutex_lock(&kp->kp_lock);
	for (i = 0; i < kp->chan_num; i++) {
		if (iio_read_channel_processed(kp->pchan[kp->chan[i]],
					       &value) >= 0) {
			if (value < 0)
				continue;
			list_for_each_entry(key, &kp->adckey_head, list) {
				if (key->chan == kp->chan[i] &&
				    (value >= key->value - key->tolerance) &&
				    (value <= key->value + key->tolerance)) {
					mutex_unlock(&kp->kp_lock);
					return key->code;
				}
			}
		}
	}
	mutex_unlock(&kp->kp_lock);
	return KEY_RESERVED;
}

static void meson_adc_kp_report_key(struct meson_adc_kp *kp, int code, int value)
{
	input_report_key(kp->input, code, value);
	input_sync(kp->input);

	if (kp->led_blink && value) {
		led_trigger_blink_oneshot(kp->led_blink, &kp->led_delay_on,
					  &kp->led_delay_off, 0);
	}
}

static void meson_adc_kp_poll(struct work_struct *pwork)
{
	struct meson_adc_kp *kp =
		container_of(pwork, struct meson_adc_kp, work.work);
	int code = meson_adc_kp_search_key(kp);

	if (kp->report_code && kp->report_code != code) {
		dev_info(&kp->input->dev,
			 "key %d up\n", kp->report_code);
		meson_adc_kp_report_key(kp, kp->report_code, 0);

		kp->report_code = 0;
	}

	if (code) {
		if (kp->count > 0 && code != kp->prev_code)
			kp->count = 0;

		if (kp->count < KEY_JITTER_COUNT) {
			kp->count++;
		} else {
			if (keypad_enable_flag && kp->report_code != code) {
				dev_info(&kp->input->dev,
					 "key %d down\n", code);
				meson_adc_kp_report_key(kp, code, 1);
				if (code == KEY_POWER)
					pm_wakeup_hard_event(kp->input->dev.parent);

				kp->report_code = code;
			}
			kp->count = 0;
		}
		kp->prev_code = code;
	}

	queue_delayed_work(system_wq, &kp->work,
			   msecs_to_jiffies(kp->poll_period));
}

static void send_data_to_bl301(struct mbox_chan *adc_mbox_chan)
{
	u32 val;
	struct __packed {
		u32 status;
		u32 val1;
	} buf;

	if (!strcmp(adc_key_mode_name, "POWER_WAKEUP_POWER")) {
		val = 0;  /*only power key resume*/
	} else if (!strcmp(adc_key_mode_name, "POWER_WAKEUP_ANY")) {
		val = 1; /*any key resume*/
	} else if (!strcmp(adc_key_mode_name, "POWER_WAKEUP_NONE")) {
		val = 2; /*no key can resume*/
	}
	aml_mbox_transfer_data(adc_mbox_chan, MBOX_CMD_SET_USR_DATA,
			       &val, sizeof(val), &buf, sizeof(buf), MBOX_SYNC);
}

static void kernel_keypad_enable_mode_enable(void)
{
	if (!strcmp(kernelkey_en_name, "KEYPAD_UNLOCK"))
		keypad_enable_flag = 1;  /*unlock, normal mode*/
	else if (!strcmp(kernelkey_en_name, "KEYPAD_LOCK"))
		keypad_enable_flag = 0;  /*lock, press key will be not useful*/
	else
		keypad_enable_flag = 1;
}

/*meson_adc_kp_get_valid_chan() - used to get valid adc channel
 *
 *@kp: to save number of channel in
 *
 */
static void meson_adc_kp_get_valid_chan(struct meson_adc_kp *kp)
{
	unsigned char incr;
	struct adc_key *key;

	mutex_lock(&kp->kp_lock);
	kp->chan_num = 0; /*recalculate*/
	list_for_each_entry(key, &kp->adckey_head, list) {
		if (kp->chan_num == 0) {
			kp->chan[kp->chan_num++] = key->chan;
		} else {
			for (incr = 0; incr < kp->chan_num; incr++) {
				if (key->chan == kp->chan[incr])
					break;
				if (incr == (kp->chan_num - 1))
					kp->chan[kp->chan_num++] = key->chan;
			}
		}
	}
	mutex_unlock(&kp->kp_lock);
}

static int meson_adc_kp_get_devtree_pdata(struct platform_device *pdev,
					  struct meson_adc_kp *kp)
{
	int ret;
	int count;
	int value;
	int state = 0;
	unsigned char cnt;
	const char *uname;
	unsigned int key_num;
	struct adc_key *key;
	struct of_phandle_args chanspec;

	if (!pdev->dev.of_node) {
		dev_err(&pdev->dev, "failed to get device node\n");
		return -EINVAL;
	}

	count = of_property_count_strings(pdev->dev.of_node,
					  "io-channel-names");
	if (count < 0) {
		dev_err(&pdev->dev, "failed to get io-channel-names");
		return -ENODATA;
	}

	ret = of_property_read_u32(pdev->dev.of_node, "poll-interval", &value);
	if (ret)
		kp->poll_period = POLL_INTERVAL_DEFAULT;
	else
		kp->poll_period = value;

	for (cnt = 0; cnt < count; cnt++) {
		ret = of_parse_phandle_with_args(pdev->dev.of_node,
						 "io-channels",
						 "#io-channel-cells",
						 cnt, &chanspec);
		if (ret)
			return ret;

		if (!chanspec.args_count)
			return -EINVAL;

		if (chanspec.args[0] >= 8) {
			dev_err(&pdev->dev, "invalid channel index[%u]\n",
				chanspec.args[0]);
			return -EINVAL;
		}

		ret = of_property_read_string_index(pdev->dev.of_node,
						    "io-channel-names",
						    cnt, &uname);
		if (ret < 0) {
			dev_err(&pdev->dev, "invalid channel name index[%d]\n",
				cnt);
			return -EINVAL;
		}

		kp->pchan[chanspec.args[0]] = devm_iio_channel_get(&pdev->dev,
				uname);
		if (IS_ERR(kp->pchan[chanspec.args[0]]))
			return PTR_ERR(kp->pchan[chanspec.args[0]]);
	}

	ret = of_property_read_u32(pdev->dev.of_node, "key_num", &key_num);
	if (ret) {
		dev_err(&pdev->dev, "failed to get key_num!\n");
		return -EINVAL;
	}

	for (cnt = 0; cnt < key_num; cnt++) {
		key = kzalloc(sizeof(*key), GFP_KERNEL);
		if (!key)
			return -ENOMEM;

		ret = of_property_read_string_index(pdev->dev.of_node,
						    "key_name", cnt, &uname);
		if (ret < 0) {
			dev_err(&pdev->dev, "invalid key name index[%d]\n",
				cnt);
			state = -EINVAL;
			goto err;
		}
		snprintf(key->name, MAX_NAME_LEN, "%s", uname);

		ret = of_property_read_u32_index(pdev->dev.of_node,
						 "key_code", cnt, &key->code);
		if (ret < 0) {
			dev_err(&pdev->dev, "invalid key code index[%d]\n",
				cnt);
			state = -EINVAL;
			goto err;
		}

		ret = of_property_read_u32_index(pdev->dev.of_node,
						 "key_chan", cnt, &key->chan);
		if (ret < 0) {
			dev_err(&pdev->dev, "invalid key chan index[%d]\n",
				cnt);
			state = -EINVAL;
			goto err;
		}

		if (!kp->pchan[key->chan]) {
			dev_err(&pdev->dev, "invalid channel[%u], please enable it first by DTS\n",
				key->chan);
			state = -EINVAL;
			goto err;
		}

		ret = of_property_read_u32_index(pdev->dev.of_node,
						 "key_val", cnt, &key->value);
		if (ret < 0) {
			dev_err(&pdev->dev, "invalid key value index[%d]\n",
				cnt);
			state = -EINVAL;
			goto err;
		}

		ret = of_property_read_u32_index(pdev->dev.of_node,
						 "key_tolerance",
						 cnt, &key->tolerance);
		if (ret < 0) {
			dev_err(&pdev->dev, "invalid key tolerance index[%d]\n",
				cnt);
			state = -EINVAL;
			goto err;
		}
		list_add_tail(&key->list, &kp->adckey_head);
	}
	meson_adc_kp_get_valid_chan(kp);
	return 0;
err:
	kfree(key);
	return state;
}

static void meson_adc_kp_list_free(struct meson_adc_kp *kp)
{
	struct adc_key *key;
	struct adc_key *key_tmp;

	mutex_lock(&kp->kp_lock);
	list_for_each_entry_safe(key, key_tmp, &kp->adckey_head, list) {
		list_del(&key->list);
		kfree(key);
	}
	mutex_unlock(&kp->kp_lock);
}

static ssize_t table_show(struct class *cls, struct class_attribute *attr,
			  char *buf)
{
	struct meson_adc_kp *kp = container_of(cls,
					struct meson_adc_kp, kp_class);
	struct adc_key *key;
	unsigned char key_num = 1;
	int len = 0;

	mutex_lock(&kp->kp_lock);
	list_for_each_entry(key, &kp->adckey_head, list) {
		len += sprintf(buf + len,
			"[%d]: name=%-21s code=%-5d channel=%-3d value=%-5d tolerance=%-5d\n",
			key_num,
			key->name,
			key->code,
			key->chan,
			key->value,
			key->tolerance);
		key_num++;
	}
	mutex_unlock(&kp->kp_lock);

	return len;
}

static ssize_t table_store(struct class *cls, struct class_attribute *attr,
			   const char *buf, size_t count)
{
	struct meson_adc_kp *kp = container_of(cls,
					struct meson_adc_kp, kp_class);
	struct device *dev = kp->input->dev.parent;
	struct adc_key *dkey;
	struct adc_key *key;
	struct adc_key *key_tmp;
	char nbuf[TMP_BUF_MAX];
	char *pbuf = nbuf;
	unsigned char colon_num = 0;
	int nsize = 0;
	int state = 0;
	char *pval;

	/*count inclued '\0'*/
	if (count > TMP_BUF_MAX) {
		dev_err(dev, "write data is too long[max:%d]: %zu\n",
			TMP_BUF_MAX, count);
		return -EINVAL;
	}

	/*trim all invisible characters include '\0', tab, space etc*/
	while (*buf) {
		if (*buf > ' ')
			nbuf[nsize++] = *buf;
		if (*buf == ':')
			colon_num++;
		buf++;
	}
	nbuf[nsize] = '\0';

	/*write "null" or "NULL" to clean up all key table*/
	if (strcasecmp("null", nbuf) == 0) {
		meson_adc_kp_list_free(kp);
		return count;
	}

	/*to judge write data format whether valid or not*/
	if (colon_num != 4) {
		dev_err(dev, "write data invalid: %s\n", nbuf);
		dev_err(dev, "=> [name]:[code]:[channel]:[value]:[tolerance]\n");
		return -EINVAL;
	}

	dkey = kzalloc(sizeof(*dkey), GFP_KERNEL);
	if (!dkey)
		return -ENOMEM;

	/*save the key data in order*/
	pval = strsep(&pbuf, ":"); /*name*/
	if (pval)
		snprintf(dkey->name, MAX_NAME_LEN, "%s", pval);

	pval = strsep(&pbuf, ":"); /*code*/
	if (pval)
		if (kstrtoint(pval, 0, &dkey->code) < 0) {
			state = -EINVAL;
			goto err;
		}

	pval = strsep(&pbuf, ":"); /*channel*/
	if (pval)
		if (kstrtoint(pval, 0, &dkey->chan) < 0) {
			state = -EINVAL;
			goto err;
		}
	if (!kp->pchan[dkey->chan]) {
		dev_err(dev, "invalid channel[%u], please enable it first by DTS\n",
			dkey->chan);
		state = -EINVAL;
		goto err;
	}
	pval = strsep(&pbuf, ":"); /*value*/
	if (pval)
		if (kstrtoint(pval, 0, &dkey->value) < 0) {
			state = -EINVAL;
			goto err;
		}
	pval = strsep(&pbuf, ":"); /*tolerance*/
	if (pval)
		if (kstrtoint(pval, 0, &dkey->tolerance) < 0) {
			state = -EINVAL;
			goto err;
		}

	/*check channel data whether valid or not*/
	if (dkey->chan >= 8) {
		dev_err(dev, "invalid channel[%d-%d]: %d\n", 0,
			8 - 1, dkey->chan);
		state = -EINVAL;
		goto err;
	}

	/*check sample data whether valid or not*/
	if (dkey->value > SAM_MAX) {
		dev_err(dev, "invalid sample value[%d-%d]: %d\n",
			SAM_MIN, SAM_MAX, dkey->value);
		state = -EINVAL;
		goto err;
	}

	/*check tolerance data whether valid or not*/
	if (dkey->tolerance > TOL_MAX) {
		dev_err(dev, "invalid tolerance[%d-%d]: %d\n",
			TOL_MIN, TOL_MAX, dkey->tolerance);
		state = -EINVAL;
		goto err;
	}

	mutex_lock(&kp->kp_lock);
	list_for_each_entry_safe(key, key_tmp, &kp->adckey_head, list) {
		if (key->code == dkey->code ||
		    (key->chan == dkey->chan &&
		     key->value == dkey->value)) {
			dev_info(dev, "del older key => %s:%d:%d:%d:%d\n",
				key->name, key->code, key->chan,
				key->value, key->tolerance);
			clear_bit(key->code,  kp->input->keybit);
			list_del(&key->list);
			kfree(key);
		}
	}
	set_bit(dkey->code,  kp->input->keybit);
	list_add_tail(&dkey->list, &kp->adckey_head);
	dev_info(dev, "add newer key => %s:%d:%d:%d:%d\n", dkey->name,
		dkey->code, dkey->chan, dkey->value, dkey->tolerance);

	mutex_unlock(&kp->kp_lock);

	meson_adc_kp_get_valid_chan(kp);

	return count;
err:
	kfree(dkey);
	return state;
}

static CLASS_ATTR_RW(table);

static struct attribute *meson_adckey_attrs[] = {
	&class_attr_table.attr,
	NULL
};

ATTRIBUTE_GROUPS(meson_adckey);

static void meson_adc_kp_init_keybit(struct meson_adc_kp *kp)

{
	struct adc_key *key;

	list_for_each_entry(key, &kp->adckey_head, list)
		set_bit(key->code,
			kp->input->keybit); /*set event code*/
}

static void meson_adc_kp_led_blink_register(struct platform_device *pdev)
{
	struct meson_adc_kp *kp = platform_get_drvdata(pdev);
	struct device_node *blink_node;
	u32 value;
	const char *str;
	int ret;

	kp->led_delay_on = 100;
	kp->led_delay_off = 100;
	kp->led_trigger_name = DRIVE_NAME;

	blink_node = of_find_node_by_name(pdev->dev.of_node, "led_blink");
	if (blink_node) {
		ret = of_property_read_u32(blink_node, "delay_on", &value);
		if (!ret)
			kp->led_delay_on = value;

		ret = of_property_read_u32(blink_node, "delay_off", &value);
		if (!ret)
			kp->led_delay_off = value;

		ret = of_property_read_string(blink_node, "trigger_name", &str);
		if (!ret)
			kp->led_trigger_name = str;
	}

	led_trigger_register_simple(kp->led_trigger_name, &kp->led_blink);
}

static void meson_adc_kp_led_blink_unregister(struct platform_device *pdev)
{
	struct meson_adc_kp *kp = platform_get_drvdata(pdev);

	if (kp->led_blink)
		led_trigger_unregister_simple(kp->led_blink);
	kp->led_blink = NULL;
}

static int meson_adc_kp_probe(struct platform_device *pdev)
{
	struct meson_adc_kp *kp;
	int ret = 0;

	adc_mbox_chan = aml_mbox_request_channel_byidx(&pdev->dev, 0);
	send_data_to_bl301(adc_mbox_chan);
	kernel_keypad_enable_mode_enable();

	kp = kzalloc(sizeof(*kp), GFP_KERNEL);
	if (!kp) {
		if (!IS_ERR_OR_NULL(adc_mbox_chan))
			mbox_free_channel(adc_mbox_chan);
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, kp);
	mutex_init(&kp->kp_lock);
	INIT_LIST_HEAD(&kp->adckey_head);
	kp->report_code = 0;
	kp->prev_code = 0;
	kp->count = 0;
	ret = meson_adc_kp_get_devtree_pdata(pdev, kp);
	if (ret)
		goto err;

	/*alloc input device*/
	kp->input = input_allocate_device();
	if (!kp->input) {
		dev_err(&pdev->dev, "alloc input device failed!\n");
		ret = -ENOMEM;
		goto err;
	}

	set_bit(EV_KEY, kp->input->evbit);
	meson_adc_kp_init_keybit(kp);

	kp->input->name = "adc_keypad";
	kp->input->phys = "adc_keypad/input0";
	kp->input->dev.parent = &pdev->dev;

	kp->input->id.bustype = BUS_ISA;
	kp->input->id.vendor = 0x0001;
	kp->input->id.product = 0x0001;
	kp->input->id.version = 0x0100;

	kp->input->rep[REP_DELAY] = 0xffffffff;
	kp->input->rep[REP_PERIOD] = 0xffffffff;

	kp->input->keycodesize = sizeof(unsigned short);
	kp->input->keycodemax = 0x1ff;

	/*init class*/
	kp->kp_class.name = DRIVE_NAME;
	kp->kp_class.owner = THIS_MODULE;
	kp->kp_class.class_groups = meson_adckey_groups;
	ret = class_register(&kp->kp_class);
	if (ret) {
		dev_err(&pdev->dev, "fail to create adc keypad class.\n");
		goto err1;
	}

	INIT_DELAYED_WORK(&kp->work, meson_adc_kp_poll);

	/*register input device*/
	ret = input_register_device(kp->input);
	if (ret) {
		dev_err(&pdev->dev,
			 "unable to register keypad input device.\n");
		goto err2;
	}

	device_init_wakeup(&pdev->dev, true);

	mod_delayed_work(system_wq, &kp->work,
			 msecs_to_jiffies(kp->poll_period));

	meson_adc_kp_led_blink_register(pdev);

	return ret;

err2:
	class_unregister(&kp->kp_class);
err1:
	input_free_device(kp->input);
err:
	if (!IS_ERR_OR_NULL(adc_mbox_chan))
		mbox_free_channel(adc_mbox_chan);
	meson_adc_kp_list_free(kp);
	kfree(kp);

	return ret;
}

static int meson_adc_kp_remove(struct platform_device *pdev)
{
	struct meson_adc_kp *kp = platform_get_drvdata(pdev);

	meson_adc_kp_led_blink_unregister(pdev);
	class_unregister(&kp->kp_class);
	cancel_delayed_work_sync(&kp->work);
	input_unregister_device(kp->input);
	input_free_device(kp->input);
	meson_adc_kp_list_free(kp);
	kfree(kp);

	return 0;
}

static int __maybe_unused meson_adc_kp_suspend(struct device *dev)
{
	return 0;
}

static int __maybe_unused meson_adc_kp_resume(struct device *dev)
{
	struct adc_key *key;
	struct meson_adc_kp *kp = dev_get_drvdata(dev);
	int val = 0;

	if (get_resume_method() == POWER_KEY_WAKEUP) {
		list_for_each_entry(key, &kp->adckey_head, list) {
			if (key->code == KEY_POWER) {
				dev_info(dev, "adc keypad wakeup\n");

				meson_adc_kp_report_key(kp, KEY_POWER, 1);
				meson_adc_kp_report_key(kp, KEY_POWER, 0);

				aml_mbox_transfer_data(adc_mbox_chan, MBOX_CMD_WAKEUP_REASON_CLR,
						       NULL, 0, &val, sizeof(val), MBOX_SYNC);
				if (val)
					pr_debug("clr adc wakeup reason fail.\n");
			}
		}
	}

	return 0;
}

static const struct dev_pm_ops meson_adc_kp_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(meson_adc_kp_suspend, meson_adc_kp_resume)
};

static void meson_adc_kp_shutdown(struct platform_device *pdev)
{
	struct meson_adc_kp *kp = platform_get_drvdata(pdev);

	cancel_delayed_work(&kp->work);
}

static const struct of_device_id key_dt_match[] = {
	{.compatible = "amlogic, adc_keypad",},
	{},
};

static struct platform_driver kp_driver = {
	.probe      = meson_adc_kp_probe,
	.remove     = meson_adc_kp_remove,
	.shutdown   = meson_adc_kp_shutdown,
	.driver     = {
		.name = DRIVE_NAME,
		.pm = &meson_adc_kp_pm_ops,
		.of_match_table = key_dt_match,
	},
};

int __init meson_adc_kp_init(void)
{
	return platform_driver_register(&kp_driver);
}

void __exit meson_adc_kp_exit(void)
{
	platform_driver_unregister(&kp_driver);
}

/*
 * Note: If the driver is compiled as a module, the __setup() do nothing
 *       __setup() is defined in include/linux/init.h
 */
#ifndef MODULE

static int __init adc_key_mode_para_setup(char *s)
{
	if (s)
		sprintf(adc_key_mode_name, "%s", s);

	return 0;
}
__setup("adckeyswitch=", adc_key_mode_para_setup);

static int __init kernel_keypad_enable_setup(char *s)
{
	if (s)
		sprintf(kernelkey_en_name, "%s", s);

	return 0;
}
__setup("kernelkey_enable=", kernel_keypad_enable_setup);

#endif
