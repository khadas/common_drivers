// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/init.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/clk.h>
#include <linux/of_device.h>
#include <linux/compat.h>
#include <linux/workqueue.h>
#include <linux/mm.h>
#include <linux/of_reserved_mem.h>
#include <linux/cma.h>
#include <linux/dma-mapping.h>
#include <linux/sched/clock.h>
#include <linux/amlogic/pm.h>
#include "ambilight.h"
#include "../lcd_reg.h"

static void amblt_buffer_dump(struct amblt_drv_s *amblt_drv)
{
	unsigned int *vaddr;
	char *print_buf;
	int i, j, n, len;

	if (amblt_drv->dma_mem.flag == 0) {
		AMBLTERR("dma_mem is null\n");
		return;
	}

	vaddr = amblt_drv->dma_mem.vaddr;
	print_buf = kzalloc(20 * 3 + 10, GFP_KERNEL);
	if (!print_buf) {
		AMBLTERR("print_buf is null\n");
		return;
	}
	pr_info("ambilight buffer:\n");
	for (i = 0; i < amblt_drv->zone_size; i++) {
		len = 0;
		n = i * 4;
		for (j = 0; j < 4; j++)
			len += sprintf(print_buf + len, " 0x%08x", vaddr[n + j]);
		pr_info("[%d]:%s\n", i, print_buf);
	}
	pr_info("\n");
	kfree(print_buf);
}

static void amblt_data_sum_dump(struct amblt_drv_s *amblt_drv)
{
	struct amblt_data_s *data;
	char *print_buf;
	int x, y, n, len;

	if (!amblt_drv->buf) {
		AMBLTERR("data buf is null\n");
		return;
	}

	print_buf = kzalloc(5 * 30 + 10, GFP_KERNEL);
	if (!print_buf) {
		AMBLTERR("print_buf is null\n");
		return;
	}

	for (y = 0; y < amblt_drv->zone_v; y++) {
		for (x = 0; x < amblt_drv->zone_h; x++) {
			len = 0;
			n = y * amblt_drv->zone_h + x;
			data = amblt_drv->buf + n;
			len += sprintf(print_buf + len, " %8d %8d %8d",
				data->sum_r, data->sum_g, data->sum_b);
			if (data->err)
				len += sprintf(print_buf + len, " (x)");
			pr_info("[x:%02d, y:%02d]: %s\n", x, y, print_buf);
		}
	}

	kfree(print_buf);
}

static void amblt_data_avg_dump(struct amblt_drv_s *amblt_drv)
{
	struct amblt_data_s *data;
	char *print_buf;
	int x, y, n, len;

	if (!amblt_drv->buf) {
		AMBLTERR("data buf is null\n");
		return;
	}

	print_buf = kzalloc(5 * 20 + 10, GFP_KERNEL);
	if (!print_buf) {
		AMBLTERR("print_buf is null\n");
		return;
	}

	for (y = 0; y < amblt_drv->zone_v; y++) {
		for (x = 0; x < amblt_drv->zone_h; x++) {
			len = 0;
			n = y * amblt_drv->zone_h + x;
			data = amblt_drv->buf + n;
			len += sprintf(print_buf + len, " %3d %3d %3d",
				data->avg_r, data->avg_g, data->avg_b);
			if (data->err)
				len += sprintf(print_buf + len, " (x)");
			pr_info("[x:%02d, y:%02d]: %s\n", x, y, print_buf);
		}
	}

	kfree(print_buf);
}

ssize_t amblt_status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct amblt_drv_s *amblt_drv = dev_get_drvdata(dev);
	ssize_t len = 0;

	if (!amblt_drv)
		return sprintf(buf, "amblt_drv is NULL\n");

	len = sprintf(buf, "ambilight status:\n"
		"en:          %d\n"
		"state:       0x%x\n"
		"zone_h:      %d\n"
		"zone_v:      %d\n"
		"zone_size:   %d\n"
		"zone_size:   %d\n"
		"dbg_level:   0x%x\n\n",
		amblt_drv->en,
		amblt_drv->state,
		amblt_drv->zone_h,
		amblt_drv->zone_v,
		amblt_drv->zone_size,
		amblt_drv->zone_pixel,
		amblt_drv->dbg_level);

	len += sprintf(buf + len, "ambilight lut_dma:\n"
		"dma_wr_id:    %d\n"
		"wr_sel:       %d\n"
		"offset:       %d\n"
		"stride:       %d\n"
		"rpt_num:      %d\n"
		"addr_mode:    %d\n"
		"baddr0:       0x%08x\n"
		"baddr1:       0x%08x\n\n",
		amblt_drv->lut_dma.dma_wr_id,
		amblt_drv->lut_dma.wr_sel,
		amblt_drv->lut_dma.offset,
		amblt_drv->lut_dma.stride,
		amblt_drv->lut_dma.rpt_num,
		amblt_drv->lut_dma.addr_mode,
		amblt_drv->lut_dma.baddr0,
		amblt_drv->lut_dma.baddr1);

	lcd_vcbus_write(VPU_DMA_WRMIF_SEL, (0xff0 + amblt_drv->lut_dma.wr_sel));
	len += sprintf(buf + len, "ambilight regs:\n");
	len += sprintf(buf + len, "LDC_REG_INPUT_STAT_NUM 0x%04x = 0x%08x\n",
			LDC_REG_INPUT_STAT_NUM, lcd_vcbus_read(LDC_REG_INPUT_STAT_NUM));
	len += sprintf(buf + len, "LCD_OLED_SIZE 0x%04x = 0x%08x\n",
			LCD_OLED_SIZE, lcd_vcbus_read(LCD_OLED_SIZE));
	len += sprintf(buf + len, "VPU_DMA_WRMIF_SEL 0x%04x = 0x%08x\n",
			VPU_DMA_WRMIF_SEL, lcd_vcbus_read(VPU_DMA_WRMIF_SEL));
	len += sprintf(buf + len, "VPU_DMA_WRMIF_CTRL 0x%04x = 0x%08x\n",
			VPU_DMA_WRMIF_CTRL, lcd_vcbus_read(VPU_DMA_WRMIF_CTRL));
	len += sprintf(buf + len, "VPU_DMA_WRMIF1_CTRL 0x%04x = 0x%08x\n",
			VPU_DMA_WRMIF1_CTRL, lcd_vcbus_read(VPU_DMA_WRMIF1_CTRL));
	len += sprintf(buf + len, "VPU_DMA_WRMIF1_BADR0 0x%04x = 0x%08x\n",
			VPU_DMA_WRMIF1_BADR0, lcd_vcbus_read(VPU_DMA_WRMIF1_BADR0));
	len += sprintf(buf + len, "VPU_DMA_WRMIF1_BADR1 0x%04x = 0x%08x\n",
			VPU_DMA_WRMIF1_BADR1, lcd_vcbus_read(VPU_DMA_WRMIF1_BADR1));

	return len;
}

static const char *amblt_debug_usage_str = {
"Usage:\n"
"    cat /sys/class/amblt/amblt/status\n"
"    echo 0 >/sys/class/amblt/amblt/enable\n"
"    echo 1 >/sys/class/amblt/amblt/enable\n"
"    echo reg >/sys/class/amblt/amblt/debug\n"
"    echo data >/sys/class/amblt/amblt/debug\n"
"    echo buffer >/sys/class/amblt/amblt/debug\n"
"    echo 1 >/sys/class/amblt/amblt/print\n"
};

static ssize_t amblt_debug_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", amblt_debug_usage_str);
}

static ssize_t amblt_debug_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct amblt_drv_s *amblt_drv = dev_get_drvdata(dev);
	unsigned int data[5];
	int ret;

	if (!buf)
		return count;
	if (!amblt_drv)
		return count;

	switch (buf[0]) {
	case 'z': //zone
		ret = sscanf(buf, "zone %d %d", &data[0], &data[1]);
		if (ret == 2) {
			amblt_drv->zone_h = data[0];
			amblt_drv->zone_v = data[1];
			amblt_drv->zone_size = data[0] * data[1];
		}
		pr_info("zone_h=%d, zone_v=%d\n",
			amblt_drv->zone_h, amblt_drv->zone_v);
		break;
	case 'r': //reg
		pr_info("LDC_REG_INPUT_STAT_NUM 0x%04x = 0x%08x\n",
			LDC_REG_INPUT_STAT_NUM, lcd_vcbus_read(LDC_REG_INPUT_STAT_NUM));
		pr_info("LCD_OLED_SIZE          0x%04x = 0x%08x\n",
			LCD_OLED_SIZE, lcd_vcbus_read(LCD_OLED_SIZE));
		pr_info("VPU_DMA_WRMIF_CTRL     0x%04x = 0x%08x\n",
			VPU_DMA_WRMIF_CTRL, lcd_vcbus_read(VPU_DMA_WRMIF_CTRL));
		pr_info("VPU_DMA_WRMIF_SEL      0x%04x = 0x%08x\n",
			VPU_DMA_WRMIF_SEL, lcd_vcbus_read(VPU_DMA_WRMIF_SEL));
		pr_info("VPU_DMA_WRMIF1_CTRL    0x%04x = 0x%08x\n",
			VPU_DMA_WRMIF1_CTRL, lcd_vcbus_read(VPU_DMA_WRMIF1_CTRL));
		pr_info("VPU_DMA_WRMIF1_BADR0   0x%04x = 0x%08x\n",
			VPU_DMA_WRMIF1_BADR0, lcd_vcbus_read(VPU_DMA_WRMIF1_BADR0));
		pr_info("VPU_DMA_WRMIF1_BADR1   0x%04x = 0x%08x\n",
			VPU_DMA_WRMIF1_BADR1, lcd_vcbus_read(VPU_DMA_WRMIF1_BADR1));
		break;
	case 'd': //dma
		ret = sscanf(buf, "dma %d %d %d", &data[0], &data[1], &data[2]);
		if (ret == 3) {
			amblt_drv->lut_dma.stride = data[0];
			amblt_drv->lut_dma.rpt_num = data[1];
			amblt_drv->lut_dma.addr_mode = data[2];
		}
		pr_info("lut_dma: stride=%d, rpt_num=%d, addr_mode=%d\n",
			amblt_drv->lut_dma.stride,
			amblt_drv->lut_dma.rpt_num,
			amblt_drv->lut_dma.addr_mode);
		break;
	case 'b': //buffer
		amblt_buffer_dump(amblt_drv);
		break;
	case 's': //sum
		amblt_data_sum_dump(amblt_drv);
		break;
	case 'a': //avg
		amblt_data_avg_dump(amblt_drv);
		break;
	default:
		pr_info("invalid cmd\n");
		break;
	}
	return count;
}

static ssize_t amblt_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct amblt_drv_s *amblt_drv = dev_get_drvdata(dev);
	unsigned int temp;

	temp = (amblt_drv->state & AMBLT_STATE_EN) ? 1 : 0;

	return sprintf(buf, "%d\n", temp);
}

static ssize_t amblt_enable_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct amblt_drv_s *amblt_drv = dev_get_drvdata(dev);
	unsigned int temp, zone_h, zone_v;
	int ret;

	if (!buf)
		return count;

	ret = sscanf(buf, "%d %d %d", &temp, &zone_h, &zone_v);
	if (ret == 3) {
		if (temp) {
			amblt_drv->zone_h = zone_h;
			amblt_drv->zone_v = zone_v;
			amblt_drv->zone_size = zone_h * zone_v;
			amblt_zone_pixel_init(amblt_drv);
			amblt_function_enable(amblt_drv);
		} else {
			amblt_function_disable(amblt_drv);
		}
	} else if (ret == 1) {
		if (temp)
			amblt_function_enable(amblt_drv);
		else
			amblt_function_disable(amblt_drv);
	} else {
		pr_info("invalid data\n");
		return -EINVAL;
	}

	return count;
}

static ssize_t amblt_print_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", amblt_debug_print);
}

static ssize_t amblt_print_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	unsigned int temp;
	int ret;

	if (!buf)
		return count;

	ret = kstrtouint(buf, 16, &temp);
	if (ret) {
		pr_info("invalid data\n");
		return -EINVAL;
	}
	amblt_debug_print = (unsigned int)temp;
	pr_info("set debug print flag: 0x%x\n", amblt_debug_print);

	return count;
}

static struct device_attribute amblt_debug_attrs[] = {
	__ATTR(status,    0444, amblt_status_show, NULL),
	__ATTR(debug,     0644, amblt_debug_show, amblt_debug_store),
	__ATTR(enable,    0644, amblt_enable_show, amblt_enable_store),
	__ATTR(print,     0644, amblt_print_show, amblt_print_store)
};

/* ************************************************************* */
/* vout ioctl                                                    */
/* ************************************************************* */
static int amblt_io_open(struct inode *inode, struct file *file)
{
	struct amblt_drv_s *amblt_drv;

	if (amblt_debug_print)
		AMBLTPR("amblt io_open\n");
	amblt_drv = container_of(inode->i_cdev, struct amblt_drv_s, cdev);
	file->private_data = amblt_drv;
	return 0;
}

static int amblt_io_release(struct inode *inode, struct file *file)
{
	if (amblt_debug_print)
		AMBLTPR("amblt io_release\n");
	file->private_data = NULL;
	return 0;
}

static long amblt_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct amblt_drv_s *amblt_drv;
	void __user *argp;
	int mcd_nr;
	unsigned int temp;
	int ret = 0;

	amblt_drv = (struct amblt_drv_s *)file->private_data;
	if (!amblt_drv)
		return -1;

	mcd_nr = _IOC_NR(cmd);
	if (amblt_debug_print) {
		AMBLTPR("%s: cmd: 0x%x, cmd_dir = 0x%x, cmd_nr = 0x%x\n",
			__func__, cmd, _IOC_DIR(cmd), mcd_nr);
	}

	argp = (void __user *)arg;
	switch (mcd_nr) {
	case AMBLT_IOC_EN_CTRL:
		if (copy_from_user(&temp, argp, sizeof(unsigned int)))
			ret = -EFAULT;
		if (amblt_debug_print)
			AMBLTPR("%s: AMBLT_IOC_EN_CTRL: %d\n", __func__, temp);
		if (temp)
			amblt_function_enable(amblt_drv);
		else
			amblt_function_disable(amblt_drv);
		break;
	case AMBLT_IOC_GET_ZONE_SIZE:
		if (amblt_debug_print) {
			AMBLTPR("%s: AMBLT_IOC_GET_ZONE_SIZE: %d\n",
				__func__, amblt_drv->zone_size);
		}

		temp = amblt_drv->zone_size;
		if (copy_to_user(argp, &temp, sizeof(unsigned int)))
			ret = -EFAULT;
		break;
	default:
		AMBLTERR("%s: invalid mcd_nr 0x%x\n", __func__, mcd_nr);
		ret = -EINVAL;
		break;
	}

	return ret;
}

#ifdef CONFIG_COMPAT
static long amblt_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	unsigned long ret;

	arg = (unsigned long)compat_ptr(arg);
	ret = amblt_ioctl(file, cmd, arg);
	return ret;
}
#endif

static const struct file_operations amblt_fops = {
	.owner          = THIS_MODULE,
	.open           = amblt_io_open,
	.release        = amblt_io_release,
	.unlocked_ioctl = amblt_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl   = amblt_compat_ioctl,
#endif
};

int amblt_debug_file_add(struct amblt_drv_s *amblt_drv)
{
	int i, ret;

	amblt_drv->clsp = class_create(THIS_MODULE, AMBLT_CLASS_NAME);
	if (IS_ERR(amblt_drv->clsp)) {
		AMBLTERR("failed to create class\n");
		return -1;
	}

	ret = alloc_chrdev_region(&amblt_drv->devno, 0, 1, AMBLT_DEVICE_NAME);
	if (ret < 0) {
		AMBLTERR("failed to alloc major number\n");
		goto err1;
	}

	amblt_drv->dev = device_create(amblt_drv->clsp, NULL,
		amblt_drv->devno, (void *)amblt_drv, AMBLT_DEVICE_NAME);
	if (IS_ERR(amblt_drv->dev)) {
		AMBLTERR("failed to create device\n");
		goto err2;
	}

	for (i = 0; i < ARRAY_SIZE(amblt_debug_attrs); i++) {
		if (device_create_file(amblt_drv->dev, &amblt_debug_attrs[i])) {
			AMBLTERR("tcon: create tcon debug attribute %s fail\n",
			       amblt_debug_attrs[i].attr.name);
			goto err3;
		}
	}

	/* connect the file operations with cdev */
	cdev_init(&amblt_drv->cdev, &amblt_fops);
	amblt_drv->cdev.owner = THIS_MODULE;

	/* connect the major/minor number to the cdev */
	ret = cdev_add(&amblt_drv->cdev, amblt_drv->devno, 1);
	if (ret) {
		AMBLTERR("failed to add device\n");
		goto err4;
	}

	AMBLTPR("%s: OK\n", __func__);
	return 0;

err4:
	for (i = 0; i < ARRAY_SIZE(amblt_debug_attrs); i++)
		device_remove_file(amblt_drv->dev, &amblt_debug_attrs[i]);
err3:
	device_destroy(amblt_drv->clsp, amblt_drv->devno);
	amblt_drv->dev = NULL;
err2:
	unregister_chrdev_region(amblt_drv->devno, 1);
err1:
	class_destroy(amblt_drv->clsp);
	amblt_drv->clsp = NULL;
	AMBLTERR("%s error\n", __func__);
	return -1;
}

int amblt_debug_file_remove(struct amblt_drv_s *amblt_drv)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(amblt_debug_attrs); i++)
		device_remove_file(amblt_drv->dev, &amblt_debug_attrs[i]);

	cdev_del(&amblt_drv->cdev);
	device_destroy(amblt_drv->clsp, amblt_drv->devno);
	amblt_drv->dev = NULL;
	class_destroy(amblt_drv->clsp);
	amblt_drv->clsp = NULL;
	unregister_chrdev_region(amblt_drv->devno, 1);

	return 0;
}
