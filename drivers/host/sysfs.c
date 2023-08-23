// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

//#define DEBUG
#include <linux/pm_runtime.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include "host.h"
#include "sysfs.h"

#define PWR_OFF	0
#define PWR_ON  1

struct dentry *debug_dir;

static int gethost_clkrate(struct seq_file *s, void *what)
{
	struct host_module *host = s->private;

	seq_printf(s, "%u\n", host->clk_rate);
	return 0;
}

static ssize_t host_clkrate_write(struct file *file, const char __user *userbuf,
			    size_t count, loff_t *ppos)
{
	struct seq_file *sf = file->private_data;
	struct host_module *host = sf->private;
	char buffer[15] = {0};

	count = min_t(size_t, count, sizeof(buffer) - 1);

	if (copy_from_user(buffer, userbuf, count))
		return -EFAULT;
	if (kstrtouint(buffer, 0, &host->clk_rate))
		return -EINVAL;

	return count;
}

static int host_clkrate_open(struct inode *inode, struct file *file)
{
	return single_open(file, gethost_clkrate, inode->i_private);
}

static const struct file_operations clkrate_fops = {
	.open		= host_clkrate_open,
	.read		= seq_read,
	.write		= host_clkrate_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int health_cnt_show(struct seq_file *s, void *data)
{
	struct host_module *host = s->private;

	if (host->health_reg)
		seq_printf(s, "[%u %u]\n", host->pre_cnt, host->cur_cnt);
	else
		seq_puts(s, "not support health monitor\n");

	return 0;
}

DEFINE_SHOW_ATTRIBUTE(health_cnt);

void host_create_debugfs_files(struct host_module *host)
{
	struct dentry *freq_file;
	struct host_data *host_data = host->host_data;

	debug_dir = debugfs_create_dir(host_data->name, NULL);
	if (!debug_dir) {
		pr_err("[%s]debugfs_create_dir %s failed..\n", __func__, host_data->name);
		return;
	}

	freq_file = debugfs_create_file("clk_rate", S_IFREG | 0440,
						debug_dir, host, &clkrate_fops);
	if (!freq_file)
		pr_err("[%s]debugfs_create_file failed..\n", __func__);
	debugfs_create_file("health_cnt", 0444, debug_dir, host,
				&health_cnt_fops);
}

void host_destroy_debugfs_files(struct host_module *host)
{
	debugfs_remove_recursive(debug_dir);
}

static ssize_t pwron_store(struct device *dev, struct device_attribute *attr,
			       const char *buf, size_t count)
{
	int ret = 0;
	unsigned int pwr_on = 0;

	ret = kstrtouint(buf, 0, &pwr_on);
	if (ret)
		return -EINVAL;
	switch (pwr_on) {
	case PWR_ON:
		pm_runtime_get_sync(dev);
		break;
	case PWR_OFF:
		pm_runtime_put_sync(dev);
		break;
	default:
		break;
	};
	return count;
}

static DEVICE_ATTR_WO(pwron);

void host_create_device_files(struct device *dev)
{
	WARN_ON(device_create_file(dev, &dev_attr_pwron));
}

void host_destroy_device_files(struct device *dev)
{
	device_remove_file(dev, &dev_attr_pwron);
}
