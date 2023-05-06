// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>

struct meson_timestamp_dev {
	void __iomem *base;
	struct miscdevice timestamp;
};

static u64 meson_timestamp_hw_get(void __iomem *vaddr)
{
	u64 counter = 0;

	counter = readl_relaxed(vaddr);
	counter |= ((u64)readl_relaxed(vaddr + 4)) << 32;

	return counter;
}

static int meson_timestamp_open(struct inode *inp, struct file *file)
{
	if ((file->f_mode & FMODE_READ) == 0)
		return -EINVAL;

	if (file->f_mode & FMODE_WRITE)
		return -EINVAL;

	return 0;
}

static ssize_t meson_timestamp_read(struct file *file, char __user *buf,
				    size_t size, loff_t *offp)
{
	struct miscdevice *c;
	struct meson_timestamp_dev *tdev;
	u64 counter;
	int ret;

	c = (struct miscdevice *)file->private_data;
	tdev = container_of(c, struct meson_timestamp_dev, timestamp);
	counter = meson_timestamp_hw_get(tdev->base);

	ret = put_user(counter, (uint64_t __user *)buf);
	if (ret)
		return -EFAULT;

	return sizeof(uint64_t);
}

static int meson_timestamp_release(struct inode *inp, struct file *file)
{
	return 0;
}

static const struct file_operations meson_timestamp_fops = {
	.owner		= THIS_MODULE,
	.open		= meson_timestamp_open,
	.read		= meson_timestamp_read,
	.release	= meson_timestamp_release,
};

static ssize_t time_show(struct device *dev,
			 struct device_attribute *attr, char *buf)
{
	struct miscdevice *c = dev_get_drvdata(dev);
	struct meson_timestamp_dev *tdev;

	tdev = container_of(c, struct meson_timestamp_dev, timestamp);

	return sprintf(buf, "%llu\n", meson_timestamp_hw_get(tdev->base));
}
static DEVICE_ATTR_RO(time);

static struct attribute *meson_timestamp_attrs[] = {
	&dev_attr_time.attr,
	NULL
};

static const struct attribute_group meson_timestamp_group = {
	.attrs = meson_timestamp_attrs,
};

static const struct attribute_group *meson_timestamp_groups[] = {
	&meson_timestamp_group,
	NULL
};

static int meson_timestamp_dev_register(struct meson_timestamp_dev *tdev)
{
	tdev->timestamp.minor = MISC_DYNAMIC_MINOR;
	tdev->timestamp.name  = "timestamp";
	tdev->timestamp.fops  = &meson_timestamp_fops;
	tdev->timestamp.groups = meson_timestamp_groups;

	return misc_register(&tdev->timestamp);
}

static int meson_timestamp_probe(struct platform_device *pdev)
{
	struct meson_timestamp_dev *tdev;
	struct resource *res;

	tdev = devm_kzalloc(&pdev->dev, sizeof(*tdev), GFP_KERNEL);
	if (!tdev)
		return -ENOMEM;

	platform_set_drvdata(pdev, tdev);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "no mem resource.\n");
		return -EINVAL;
	}

	tdev->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(tdev->base))
		return PTR_ERR(tdev->base);

	return meson_timestamp_dev_register(tdev);
}

static int meson_timestamp_remove(struct platform_device *pdev)
{
	struct meson_timestamp_dev *tdev = platform_get_drvdata(pdev);

	misc_deregister(&tdev->timestamp);

	return 0;
}

static const struct of_device_id meson_timestamp_of_match[] = {
	{
		.compatible = "amlogic, meson-soc-timestamp",
	},
	{ /* sentinel */ }
};

static struct platform_driver meson_timestamp_driver = {
	.probe = meson_timestamp_probe,
	.remove = meson_timestamp_remove,
	.driver = {
		.name = "meson_soc_timestamp",
		.of_match_table = meson_timestamp_of_match,
	},
};

module_platform_driver(meson_timestamp_driver);

MODULE_DESCRIPTION("Amlogic SoC Timestamp driver");
MODULE_LICENSE("GPL");
