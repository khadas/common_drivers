// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/arm-smccc.h>
#include <linux/io.h>
#include <linux/amlogic/secmon.h>

#ifdef CONFIG_MESON_TRUSTZONE
#include <mach/meson-secure.h>
#endif

#include "audio_data.h"
#include "efuse.h"

/*#define MYPRT pr_info*/
#define MYPRT(...)
/* major device number and minor device number */
static int major_audio_data;
/* device class and device var */
static struct class *class_audio_data;
static struct device  *dev_audio_data;
void __iomem *sharemem_in_base;
unsigned int efuse_query_licence_cmd;
unsigned int mem_in_base_cmd;


static int audio_data_open(struct inode *inode, struct file *filp);
static int audio_data_release(struct inode *inode, struct file *filp);
static ssize_t audio_data_read(struct file *filp, char __user *buf,
						size_t count, loff_t *f_pos);
static ssize_t audio_data_write(struct file *filp, const char __user *buf,
						size_t count, loff_t *f_pos);

/* device operation methods chart */
static struct file_operations const audio_data_fops = {
	.owner = THIS_MODULE,
	.open = audio_data_open,
	.release = audio_data_release,
	.read = audio_data_read,
	.write = audio_data_write,
	/* .unlocked_ioctl = audio_data_ioctl, */
};

int meson_efuse_fn_smc_query_audioinfo(struct efuse_hal_api_arg *arg)
{
	int ret;
	unsigned int cmd, offset, size;
	unsigned long *retcnt;
	struct arm_smccc_res res;

	if (!arg)
		return -1;

	retcnt = (unsigned long *)(arg->retcnt);
	cmd = arg->cmd;
	offset = arg->offset;
	size = arg->size;

	meson_sm_mutex_lock();

	/*write data*/
	memcpy((void *)sharemem_in_base, (const void *)arg->buffer, size);

	asm __volatile__("" : : : "memory");

	arm_smccc_smc(cmd, offset, size, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
	*retcnt = res.a0;

	MYPRT("[%s %d]ret/%d\n", __func__, __LINE__, ret);

	if (ret == 0) {
		memset((void *)arg->buffer, 0, arg->size);
		memcpy((void *)arg->buffer,
				(const void *)sharemem_in_base, arg->size);
	}

	meson_sm_mutex_unlock();

	return ret;
}

int meson_trustzone_audio_info_get(struct efuse_hal_api_arg *arg)
{
	int ret;
	struct cpumask org_cpumask;

	if (!arg)
		return -1;
	cpumask_copy(&org_cpumask, current->cpus_ptr);
	set_cpus_allowed_ptr(current, cpumask_of(0));
	ret = meson_efuse_fn_smc_query_audioinfo(arg);
	set_cpus_allowed_ptr(current, &org_cpumask);
	return ret;
}

unsigned long audio_info_get(char *buf, unsigned long count,
							unsigned long pos)
{
	struct efuse_hal_api_arg arg;
	unsigned long retcnt;
	int ret;

	arg.cmd =  efuse_query_licence_cmd;
	arg.offset = pos;
	arg.size = count;
	arg.buffer = (unsigned long)buf;
	arg.retcnt = (unsigned long)&retcnt;

	ret = meson_trustzone_audio_info_get(&arg);

	if (ret == 0)
		MYPRT("[%s %d]: get licence!!!\n", __func__, __LINE__);
	else
		MYPRT("[%s:%d]: read error!!!\n", __func__, __LINE__);
	return ret;
}

#define EFUSE_BUF_SIZE 1024
/* open device methods */
static int audio_data_open(struct inode *inode, struct file *filp)
{
	MYPRT("[%s]\n", __func__);
	return 0;
}

/* device file release */
static int audio_data_release(struct inode *inode, struct file *filp)
{
	MYPRT("[%s]\n", __func__);
	return 0;
}

/* read device reg val */
static ssize_t audio_data_read(struct file *filp, char __user *buf,
						size_t count, loff_t *f_pos)
{
	int err = 0;
	loff_t pos = 0;
	char *buftmp;

	buftmp = kzalloc(EFUSE_BUF_SIZE, GFP_KERNEL);
	if (!buftmp) {
		MYPRT("kzalloc fail.\n");
		return -ENOMEM;
	}

	MYPRT("[%s]\n", __func__);
	if (count > EFUSE_BUF_SIZE) {
		MYPRT("[%s %d]buffer is too small\n", __func__, __LINE__);
		err =  -1;
	} else {
		err = copy_from_user(buftmp, buf, count);
		if (!err) {
			err = audio_info_get(buftmp, count, pos);
			if (!err) {
				MYPRT("[%s %d]copy data to user (count/%d)\n",
					__func__, __LINE__, (int)count);
				err = copy_to_user(buf, buftmp, count);
			} else {
				err =  -1;
			}
		}
	}

	kfree(buftmp);
	if (!err)
		return count;
	else
		return -1;
}

/* write device reg val */
static ssize_t audio_data_write(struct file *filp, const char __user *buf,
						size_t count, loff_t *f_pos)
{
	return 0;
}

static int audio_data_probe(struct platform_device *pdev)
{
	void *ptr_err;

	major_audio_data = register_chrdev(0,
						AUDIO_DATA_DEVICE_NODE_NAME,
						&audio_data_fops);
	if (major_audio_data < 0) {
		MYPRT("Registering audio_data char device %s failed with %d\n",
				AUDIO_DATA_DEVICE_NODE_NAME, major_audio_data);
		return major_audio_data;
	}
	class_audio_data = class_create(THIS_MODULE,
						AUDIO_DATA_DEVICE_NODE_NAME);
	ptr_err = class_audio_data;
	if (IS_ERR(ptr_err))
		goto err0;

	dev_audio_data = device_create(class_audio_data, NULL,
					MKDEV(major_audio_data, 0),
					NULL, AUDIO_DATA_DEVICE_NODE_NAME);
	ptr_err = dev_audio_data;
	if (IS_ERR(ptr_err))
		goto err1;

	sharemem_in_base = get_meson_sm_input_base();
	if (!sharemem_in_base) {
		MYPRT("%s:get share memory fail\n", __func__);
		goto err1;
	}

	if (pdev->dev.of_node) {
		int ret;
		struct device_node *np = pdev->dev.of_node;
		of_node_get(np);

		ret = of_property_read_u32(np, "query_licence_cmd",
				&efuse_query_licence_cmd);
		if (ret) {
			MYPRT("%s:please config query_cmd item\n", __func__);
			return -1;
		}
	}
	return 0;
err1:
	class_destroy(class_audio_data);
err0:
	unregister_chrdev(major_audio_data, AUDIO_DATA_DEVICE_NODE_NAME);
	return PTR_ERR(ptr_err);
}

static int audio_data_remove(struct platform_device *pdev)
{
	unregister_chrdev_region(major_audio_data, 1);
	device_destroy(class_audio_data, MKDEV(major_audio_data, 0));
	class_destroy(class_audio_data);
	return 0;
}

static const struct of_device_id amlogic_audio_data_dt_match[] = {
	{	.compatible = "amlogic, audio_data",
	},
	{},
};

static struct platform_driver audio_data_driver = {
	.probe = audio_data_probe,
	.remove = audio_data_remove,
	.driver = {
		.name = AUDIO_DATA_DEVICE_NODE_NAME,
		.of_match_table = amlogic_audio_data_dt_match,
	.owner = THIS_MODULE,
	},
};

int __init audio_data_init(void)
{
	int ret = -1;

	ret = platform_driver_register(&audio_data_driver);
	if (ret != 0) {
		MYPRT("failed to register audio data driver, error %d\n", ret);
		return -ENODEV;
	}
	return ret;
}

/* module unload */
void __exit audio_data_exit(void)
{
	platform_driver_unregister(&audio_data_driver);
}

#ifndef MODULE
module_init(audio_data_init);
module_exit(audio_data_exit);
MODULE_AUTHOR("Amlogic, Inc.");
MODULE_DESCRIPTION("Amlogic audio data driver");
MODULE_LICENSE("GPL");
MODULE_DEVICE_TABLE(of, amlogic_audio_data_dt_match);
#endif

