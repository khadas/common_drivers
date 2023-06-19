/*
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Description:
 */

#include <linux/slab.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>

#include "vdec.h"
#include "decoder_report.h"

#define BUFF_SIZE 1024 * 8
#define USER_BUFF_SIZE 1024 * 4


struct aml_dec_report_dev {
	struct aml_vcodec_dev *v4l_dev;
	dump_v4ldec_state_func dump_v4ldec_state_notify;
	dump_amstream_bufs_func dump_amstream_bufs_notify;
};

static struct aml_dec_report_dev *report_dev;

static ssize_t dump_v4ldec_state(char *buf)
{
	char *pbuf = buf;

	if (report_dev->dump_v4ldec_state_notify == NULL)
		return 0;
	if (report_dev->v4l_dev == NULL)
		return 0;

	pbuf += sprintf(pbuf, "\n============ cat /sys/class/v4ldec/status:\n");
	pbuf += report_dev->dump_v4ldec_state_notify(report_dev->v4l_dev, pbuf);

	return pbuf - buf;
}

void register_dump_v4ldec_state_func(struct aml_vcodec_dev *dev, dump_v4ldec_state_func func)
{
	report_dev->v4l_dev = dev;
	report_dev->dump_v4ldec_state_notify = func;
}
EXPORT_SYMBOL(register_dump_v4ldec_state_func);

static ssize_t dump_amstream_bufs(char *buf)
{
	char *pbuf = buf;
	char *tmpbuf = (char *)vzalloc(BUFF_SIZE);
	char *ptmpbuf = tmpbuf;
	if (report_dev->dump_amstream_bufs_notify == NULL)
		return 0;

	ptmpbuf += report_dev->dump_amstream_bufs_notify(tmpbuf);

	if (ptmpbuf - tmpbuf) {
		pbuf += sprintf(pbuf, "\n============ cat /sys/class/amstream/bufs:\n");
		pbuf += sprintf(pbuf, "%s", tmpbuf);
	}

	vfree(tmpbuf);
	return pbuf - buf;
}

void register_dump_amstream_bufs_func(dump_amstream_bufs_func func)
{
	report_dev->dump_amstream_bufs_notify = func;
}
EXPORT_SYMBOL(register_dump_amstream_bufs_func);

static void buff_show(ssize_t size, char *buf, int buff_size) {
	if (size) {
		pr_info("%s\n", buf);
		memset(buf, 0, buff_size);
	}
}

static ssize_t status_show(struct class *cls,
	struct class_attribute *attr, char *buf)
{
	char *pbuf = buf;
	char *tmpbuf = (char *)vzalloc(BUFF_SIZE);
	char *ptmpbuf = tmpbuf;
	ssize_t size = 0;

	if (!report_dev)
		return 0;

	pr_info("\n============ cat /sys/class/vdec/dump_decoder_state:\n");
	buff_show(dump_decoder_state(tmpbuf), tmpbuf, BUFF_SIZE);

	ptmpbuf += dump_v4ldec_state(ptmpbuf);

	ptmpbuf+= sprintf(ptmpbuf, "\n============ cat /sys/class/vdec/debug:\n");
	ptmpbuf += dump_vdec_debug(ptmpbuf);

	ptmpbuf += sprintf(ptmpbuf, "\n============ cat /sys/class/vdec/dump_vdec_chunks:\n");
	ptmpbuf += dump_vdec_chunks(ptmpbuf);

	ptmpbuf += dump_amstream_bufs(ptmpbuf);

	ptmpbuf += sprintf(ptmpbuf, "\n============ cat /sys/class/vdec/core:\n");
	ptmpbuf += dump_vdec_core(ptmpbuf);

	size = ptmpbuf - tmpbuf;
	if (size > USER_BUFF_SIZE) {
		buff_show(size, tmpbuf, BUFF_SIZE);
	} else {
		pbuf+= sprintf(pbuf, "%s\n", tmpbuf);
	}

	vfree(tmpbuf);
	return pbuf - buf;
}

static CLASS_ATTR_RO(status);

static struct attribute *report_class_attrs[] = {
	&class_attr_status.attr,
	NULL
};

ATTRIBUTE_GROUPS(report_class);

static struct class report_class = {
	.name = "dec_report",
	.class_groups = report_class_groups,
};


static struct platform_driver report_driver = {
	.driver = {
		.name = "dec_report",
	}
};

int report_module_init(void)
{
	int ret = -1;
	report_dev = (struct aml_dec_report_dev *)vzalloc(sizeof(struct aml_dec_report_dev));

	if (platform_driver_register(&report_driver)) {
		pr_info("failed to register decoder report module\n");
		goto err;
	}

	ret = class_register(&report_class);
	if (ret < 0) {
		pr_info("Failed in creating class.\n");
		goto unregister;
	}

	return 0;
unregister:
	platform_driver_unregister(&report_driver);
err:
	vfree(report_dev);
	return ret;
}
EXPORT_SYMBOL(report_module_init);

void report_module_exit(void)
{
	vfree(report_dev);
	platform_driver_unregister(&report_driver);
	class_unregister(&report_class);
}
EXPORT_SYMBOL(report_module_exit);

MODULE_DESCRIPTION("AMLOGIC bug report driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kuan Hu <kuan.hu@amlogic.com>");

