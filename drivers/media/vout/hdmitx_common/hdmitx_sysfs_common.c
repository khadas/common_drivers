// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include "hdmitx_sysfs_common.h"

/*!!Only one instance supported.*/
static struct hdmitx_common *global_tx_common;

/************************common sysfs*************************/
static ssize_t attr_show(struct device *dev,
			 struct device_attribute *attr, char *buf)
{
	int pos = 0;
	char fmt_attr[16];

	hdmitx_get_attr(global_tx_common, fmt_attr);
	pos = snprintf(buf, PAGE_SIZE, "%s\n\r", fmt_attr);

	return pos;
}

static ssize_t attr_store(struct device *dev,
		   struct device_attribute *attr,
		   const char *buf, size_t count)
{
	hdmitx_setup_attr(global_tx_common, buf);
	return count;
}

static DEVICE_ATTR_RW(attr);

static ssize_t hpd_state_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	int pos = 0;

	pos += snprintf(buf + pos, PAGE_SIZE, "%d",
		global_tx_common->hpd_state);
	return pos;
}

static DEVICE_ATTR_RO(hpd_state);

/*************************tx20 sysfs*************************/

/*************************tx21 sysfs*************************/

int hdmitx_sysfs_common_create(struct device *dev,
			struct hdmitx_common *tx_comm)
{
	int ret = 0;

	global_tx_common = tx_comm;

	ret = device_create_file(dev, &dev_attr_attr);
	ret = device_create_file(dev, &dev_attr_hpd_state);

	return ret;
}

int hdmitx_sysfs_common_destroy(struct device *dev)
{
	device_remove_file(dev, &dev_attr_attr);
	device_remove_file(dev, &dev_attr_hpd_state);

	global_tx_common = 0;

	return 0;
}

