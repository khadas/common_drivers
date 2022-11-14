// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/export.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/mailbox_controller.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/notifier.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/mailbox_client.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/list.h>
#include <linux/amlogic/aml_mbox.h>
#include <dt-bindings/mailbox/amlogic,mbox.h>
#include "meson_mbox_devfs.h"

#define DRIVER_NAME		"mbox-devfs"

struct aml_mbox_dev {
	struct list_head list;
	dev_t dev_t;
	struct cdev cdev;
	struct device *dev;
	struct device *p_dev;
	struct mbox_chan *mbox_chan;
	const char *name;
	u32 dest;
};

struct aml_mbox_priv_data {
	u32 cmd;
	char data[MBOX_USER_SIZE];
} __packed;

struct aml_mbox_priv {
	struct aml_mbox_dev *aml_dev;
	struct aml_mbox_data *aml_data;
};

static ssize_t mbox_message_write(struct file *filp,
				  const char __user *userbuf,
				  size_t count, loff_t *ppos)
{
	struct aml_mbox_priv *aml_priv = filp->private_data;
	struct aml_mbox_dev *aml_dev = aml_priv->aml_dev;
	struct mbox_chan *mbox_chan = aml_dev->mbox_chan;
	struct aml_mbox_chan *aml_chan = mbox_chan->con_priv;
	struct aml_mbox_data *aml_data = aml_priv->aml_data;
	struct aml_mbox_priv_data aml_priv_data;
	struct device *dev = aml_dev->p_dev;
	int ret = 0;

	if (count > MBOX_USER_SIZE) {
		dev_err(dev, "Msg len %zd over range dest %d\n", count, aml_dev->dest);
		return -EINVAL;
	}
	ret = copy_from_user(&aml_priv_data, userbuf, count);
	if (ret)
		return ret;

	aml_data->txbuf = aml_priv_data.data;
	aml_data->cmd = aml_priv_data.cmd;
	aml_data->txsize = count - sizeof(u32);

	switch (aml_dev->dest) {
	case MAILBOX_AOCPU:
		aml_data->rxsize = count - sizeof(u32);
		aml_data->sync = MBOX_SYNC;
		break;
	case MAILBOX_DSP:
		aml_data->rxsize = 0;
		aml_data->sync = MBOX_TSYNC;
		break;
	case MAILBOX_SECPU:
		aml_data->txbuf = &aml_priv_data;
		aml_data->rxsize = 0;
		aml_data->txsize = count;
		aml_data->sync = MBOX_SYNC;
		break;
	default:
		break;
	};

	mutex_lock(&aml_chan->mutex);
	ret = mbox_send_message(mbox_chan, aml_data);
	if (ret < 0) {
		dev_err(dev, "Msg send fail %d dest %d\n", ret, aml_dev->dest);
		complete(&aml_data->complete);
		mutex_unlock(&aml_chan->mutex);
		return ret;
	}

	if (aml_data->sync == MBOX_SYNC)
		complete(&aml_data->complete);
	mutex_unlock(&aml_chan->mutex);
	return count;
}

static ssize_t mbox_message_read(struct file *filp, char __user *userbuf,
				 size_t count, loff_t *ppos)
{
	struct aml_mbox_priv *aml_priv = filp->private_data;
	struct aml_mbox_dev *aml_dev = aml_priv->aml_dev;
	struct aml_mbox_data *aml_data = aml_priv->aml_data;
	struct device *dev = aml_dev->p_dev;
	int ret = 0;
	int rxsize = count;

	ret = wait_for_completion_killable(&aml_data->complete);
	if (ret < 0) {
		dev_err(dev, "Read msg wait killed %d\n", ret);
		return -ENXIO;
	}
	barrier();
	*ppos = 0;
	if (rxsize > aml_data->rxsize)
		rxsize = aml_data->rxsize;
	ret = simple_read_from_buffer(userbuf, rxsize, ppos,
				      aml_data->rxbuf, MBOX_DATA_SIZE);
	return ret;
}

static long mbox_message_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return 0;
}

static int mbox_message_open(struct inode *inode, struct file *filp)
{
	struct cdev *cdev = inode->i_cdev;
	struct aml_mbox_dev *aml_dev = container_of(cdev, struct aml_mbox_dev, cdev);
	struct aml_mbox_priv *aml_priv;
	struct aml_mbox_data *aml_data;

	aml_priv = devm_kzalloc(aml_dev->p_dev, sizeof(*aml_priv), GFP_KERNEL);
	if (IS_ERR(aml_priv))
		return -ENOMEM;
	aml_data = devm_kzalloc(aml_dev->p_dev, sizeof(*aml_data), GFP_KERNEL);
	if (IS_ERR(aml_data)) {
		devm_kfree(aml_dev->p_dev, aml_priv);
		return -ENOMEM;
	}

	switch (aml_dev->dest) {
	case MAILBOX_AOCPU:
	case MAILBOX_DSP:
	case MAILBOX_SECPU:
		aml_data->rxbuf = devm_kzalloc(aml_dev->p_dev, MBOX_DATA_SIZE, GFP_KERNEL);
		if (IS_ERR(aml_data->rxbuf)) {
			devm_kfree(aml_dev->p_dev, aml_priv);
			devm_kfree(aml_dev->p_dev, aml_data);
			return -ENOMEM;
		}
		break;
	default:
		break;
	};

	init_completion(&aml_data->complete);
	aml_priv->aml_dev = aml_dev;
	aml_priv->aml_data = aml_data;

	filp->private_data = aml_priv;
	return 0;
}

static int mbox_message_release(struct inode *inode, struct file *filp)
{
	struct aml_mbox_priv *aml_priv = filp->private_data;
	struct aml_mbox_dev *aml_dev = aml_priv->aml_dev;

	switch (aml_dev->dest) {
	case MAILBOX_AOCPU:
	case MAILBOX_DSP:
		devm_kfree(aml_dev->p_dev, aml_priv->aml_data->rxbuf);
		break;
	default:
		break;
	};
	devm_kfree(aml_dev->p_dev, aml_priv->aml_data);
	devm_kfree(aml_dev->p_dev, aml_priv);
	return 0;
}

static const struct file_operations mbox_message_ops = {
	.write		= mbox_message_write,
	.read		= mbox_message_read,
	.open		= mbox_message_open,
	.release	= mbox_message_release,
	.unlocked_ioctl = mbox_message_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= mbox_message_ioctl,
#endif
};

static int mbox_cdev_init(struct device *dev)
{
	static struct class *mbox_class;
	struct aml_mbox_dev *mbox_dev;
	dev_t dev_t;
	u32 idx;
	int err = 0;
	int mbox_nums = 0;

	dev_dbg(dev, "mbox devfs init start\n");
	err = of_property_read_u32(dev->of_node,
			     "mbox-nums", &mbox_nums);
	if (err < 0) {
		dev_err(dev, "No mbox-nums\n");
		return -1;
	}

	mbox_class = class_create(THIS_MODULE, "mbox_devfs");
	if (IS_ERR(mbox_class))
		goto err;

	err = alloc_chrdev_region(&dev_t, 0, mbox_nums, DRIVER_NAME);
	if (err < 0) {
		dev_err(dev, "alloc dev_t failed\n");
		err = -1;
		goto class_err;
	}

	for (idx = 0; idx < mbox_nums; idx++) {
		mbox_dev = devm_kzalloc(dev, sizeof(*mbox_dev), GFP_KERNEL);
		if (IS_ERR(mbox_dev)) {
			dev_err(dev, "Failed to alloc mbox_dev\n");
			goto out_err;
		}

		mbox_dev->p_dev = dev;
		mbox_dev->dev_t = MKDEV(MAJOR(dev_t), idx);
		cdev_init(&mbox_dev->cdev, &mbox_message_ops);
		err = cdev_add(&mbox_dev->cdev, mbox_dev->dev_t, 1);
		if (err) {
			dev_err(dev, "mbox fail to add cdev\n");
			goto out_err;
		}

		if (of_property_read_string_index(dev->of_node,
						  "mbox-names", idx, &mbox_dev->name)) {
			dev_err(dev, "%s get mbox[%d] name fail\n",
				__func__, idx);
			goto out_err;
		}

		if (of_property_read_u32_index(dev->of_node, "mbox-dests",
					       idx, &mbox_dev->dest)) {
			dev_err(dev, "%s get mbox[%d] dest fail\n",
				__func__, idx);
			goto out_err;
		}

		mbox_dev->dev = device_create(mbox_class, NULL, mbox_dev->dev_t,
					mbox_dev, "%s", mbox_dev->name);
		if (IS_ERR(mbox_dev->dev)) {
			dev_err(dev, "mbox fail to create device\n");
			goto out_err;
		}
		mbox_dev->mbox_chan = aml_mbox_request_channel_byidx(dev, idx);
		if (IS_ERR(mbox_dev->mbox_chan)) {
			dev_err(dev, "Failed to request mbox chan\n");
			goto out_err;
		}
	}
	dev_dbg(dev, "mbox devfs init done\n");
	return 0;
out_err:
	unregister_chrdev_region(dev_t, mbox_nums);
class_err:
	class_destroy(mbox_class);
err:
	return err;
}

static int mbox_devfs_probe(struct platform_device *pdev)
{
	return mbox_cdev_init(&pdev->dev);
}

static int mbox_devfs_remove(struct platform_device *pdev)
{
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static const struct of_device_id mbox_of_match[] = {
	{ .compatible = "amlogic, mbox-devfs" },
	{},
};

static struct platform_driver mbox_devfs_drvier = {
	.probe = mbox_devfs_probe,
	.remove = mbox_devfs_remove,
	.driver = {
		.owner		= THIS_MODULE,
		.name		= DRIVER_NAME,
		.of_match_table = mbox_of_match,
	},
};

int __init mbox_devfs_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&mbox_devfs_drvier);
	return ret;
}

void __exit mbox_devfs_exit(void)
{
	platform_driver_unregister(&mbox_devfs_drvier);
}
