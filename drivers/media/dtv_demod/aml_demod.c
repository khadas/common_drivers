// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2021 Amlogic, Inc. All rights reserved.
 */

#include <linux/init.h>
#include <linux/types.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/major.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/ctype.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/i2c.h>
#include <linux/delay.h>
/* #include <mach/am_regs.h> */

/* #include <asm/fiq.h> */
#include <linux/uaccess.h>
#include "aml_demod.h"
#include "demod_func.h"
#include "demod_dbg.h"

#include <linux/slab.h>
#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif

/*#include "sdio/sdio_init.h"*/
#define DRIVER_NAME "aml_demod"
#define MODULE_NAME "aml_demod"
#define DEVICE_NAME "aml_demod"
#define DEVICE_UI_NAME "aml_demod_ui"

#define pr_dbg(a ...) \
	do { \
		if (1) { \
			printk(a); \
		} \
	} while (0)


const char aml_demod_dev_id[] = "aml_demod";
#define CONFIG_AM_DEMOD_DVBAPI	/*ary temp*/

/*******************************
 *#ifndef CONFIG_AM_DEMOD_DVBAPI
 * static struct aml_demod_i2c demod_i2c;
 * static struct aml_demod_sta demod_sta;
 * #else
 * extern struct aml_demod_i2c demod_i2c;
 * extern struct aml_demod_sta demod_sta;
 * #endif
 *******************************/
unsigned int demod_id;

static DECLARE_WAIT_QUEUE_HEAD(lock_wq);
static struct mutex demod_lock;

static ssize_t aml_demod_info_store(struct class *class,
		struct class_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t aml_demod_info_show(struct class *class,
		struct class_attribute *attr, char *buf)
{
	return 0;
}

static CLASS_ATTR_RW(aml_demod_info);

static struct attribute *aml_demod_info_class_attrs[] = {
	&class_attr_aml_demod_info.attr,
	NULL,
};

ATTRIBUTE_GROUPS(aml_demod_info_class);

static struct class aml_demod_class = {
	.name		= "aml_demod",
	.class_groups	= aml_demod_info_class_groups,
};


static int aml_demod_open(struct inode *inode, struct file *file)
{
	int minor = iminor(inode);
	int major = imajor(inode);
	struct demod_device_t *pdev;

	mutex_lock(&demod_lock);

	pdev = container_of(inode->i_cdev, struct demod_device_t, dev);
	if (!pdev) {
		PR_ERR("demod_device_t == NULL\n");
		return -1;
	}
	if (!pdev->priv) {
		PR_ERR("pdev->priv == NULL\n");
		return -1;
	}

	file->private_data = pdev->priv;

	PR_INFO("%s: %s %s,major=%d,minor=%d\n", __func__, pdev->priv->name,
			pdev->priv->dev->kobj.name, major, minor);
	mutex_unlock(&demod_lock);

	return 0;
}

static int aml_demod_release(struct inode *inode, struct file *file)
{
	return 0;
}

void mem_read(struct aml_demod_mem *arg)
{
	int data;
	int addr;

	addr = arg->addr;
	data = arg->dat;
/*      memcpy(mem_buf[addr],data,1);*/
	pr_dbg("[addr %x] data is %x\n", addr, data);
}
static long aml_demod_ioctl(struct file *file,
			    unsigned int cmd, unsigned long arg)
{
	int strength = 0;
	struct aml_tuner_sys tuner_para = {0};
	struct aml_demod_reg arg_t;
	unsigned int val = 0;
	struct amldtvdemod_device_s *devp = dtvdemod_get_dev();
	struct aml_dtvdemod *demod = NULL, *tmp = NULL;
	int ret = 0;
	unsigned int dump_param = 0;
	struct demod_priv *priv = file->private_data;
	void __user *argp;

	mutex_lock(&demod_lock);

	if (!devp) {
		pr_err("%s devp is NULL\n", __func__);
		return -EFAULT;
	}

	list_for_each_entry(tmp, &devp->demod_list, list) {
		if (tmp->id == demod_id) {
			demod = tmp;
			break;
		}
	}

	if (!demod) {
		pr_err("%s get demod [id %d] is NULL.\n", __func__, demod_id);
		return -EFAULT;
	}

	if (!priv) {
		PR_ERR("priv is NULL!\n");
		return -EINVAL;
	}
	if (!devp->flg_cma_allc || !devp->cma_mem_size) {
		PR_ERR("%s: not enter_mode or invalid cma_mem_size!!\n", __func__);
		return -EINVAL;
	}

	argp = (void __user *)arg;

	switch (cmd) {
	case AML_DEMOD_GET_LOCK_STS:
		if (copy_to_user((void __user *)arg, &val, sizeof(unsigned int)))
			pr_dbg("copy_to_user error AML_DEMOD_GET_PLL_INIT\n");
		break;

	case AML_DEMOD_GET_PER:
		if (copy_to_user((void __user *)arg, &val, sizeof(unsigned int)))
			pr_dbg("copy_to_user error AML_DEMOD_GET_PLL_INIT\n");
		break;

	case AML_DEMOD_GET_CPU_ID:
		val = get_cpu_type();

		if (copy_to_user((void __user *)arg, &val, sizeof(unsigned int)))
			pr_dbg("copy_to_user error AML_DEMOD_GET_PLL_INIT\n");
		break;

	case AML_DEMOD_GET_RSSI:
		strength = tuner_get_ch_power(&demod->frontend);

		if (strength < 0)
			strength = 0 - strength;

		tuner_para.rssi = strength;
		if (copy_to_user((void __user *)arg, &tuner_para,
			sizeof(struct aml_tuner_sys))) {
			pr_err("copy_to_user error AML_DEMOD_GET_RSSI\n");
		}
		break;

	case AML_DEMOD_SET_TUNER:
		if (copy_from_user(&tuner_para, (void __user *)arg,
			sizeof(struct aml_tuner_sys))) {
			PR_ERR("copy error AML_DEMOD_SET_TUNER\n");
		} else {
			if (tuner_para.mode <= FE_ISDBT) {
				PR_INFO("set tuner md = %d\n",
					tuner_para.mode);
				demod->frontend.ops.info.type = tuner_para.mode;
			} else {
				PR_ERR("wrong md: %d\n", tuner_para.mode);
			}

			demod->frontend.dtv_property_cache.frequency =
				tuner_para.ch_freq;
			demod->frontend.ops.tuner_ops.set_config(&demod->frontend, NULL);
			tuner_set_params(&demod->frontend);
		}
		break;

	case AML_DEMOD_SET_SYS:
		pr_dbg("Ioctl Demod Set System\n");
		demod_set_sys(demod, (struct aml_demod_sys *)arg);
		break;

	case AML_DEMOD_GET_SYS:
		pr_dbg("Ioctl Demod Get System\n");

		/*demod_get_sys(&demod_i2c, (struct aml_demod_sys *)arg); */
		break;

	case AML_DEMOD_DVBC_SET_CH:
		pr_dbg("Ioctl DVB-C Set Channel.\n");
		dvbc_set_ch(demod, (struct aml_demod_dvbc *)arg,
			&demod->frontend);
		break;

	case AML_DEMOD_DVBC_GET_CH:
		dvbc_status(demod, (struct aml_demod_sts *)arg, NULL);
		break;

	case AML_DEMOD_DVBT_SET_CH:
		pr_dbg("Ioctl DVB-T Set Channel\n");
		dvbt_isdbt_set_ch(demod, (struct aml_demod_dvbt *)arg);
		break;

	case AML_DEMOD_DVBT_GET_CH:
		pr_dbg("Ioctl DVB-T Get Channel\n");
		/*dvbt_status(&demod_sta, &demod_i2c,*/
		/* (struct aml_demod_sts *)arg); */
		break;

	case AML_DEMOD_DTMB_SET_CH:
		dtmb_set_ch(demod, (struct aml_demod_dtmb *)arg);
		break;

	case AML_DEMOD_ATSC_SET_CH:
		atsc_set_ch(demod, (struct aml_demod_atsc *)arg);
		break;

	case AML_DEMOD_ATSC_GET_CH:
		check_atsc_fsm_status();
		break;

	case AML_DEMOD_SET_REG:
		if (copy_from_user(&arg_t, (void __user *)arg,
			sizeof(struct aml_demod_reg))) {
			pr_dbg("copy error AML_DEMOD_SET_REG\n");
		} else
			demod_set_reg(demod, &arg_t);

		break;

	case AML_DEMOD_GET_REG:
		if (copy_from_user(&arg_t, (void __user *)arg,
			sizeof(struct aml_demod_reg)))
			pr_dbg("copy error AML_DEMOD_GET_REG\n");
		else
			demod_get_reg(demod, &arg_t);

		if (copy_to_user((void __user *)arg, &arg_t,
			sizeof(struct aml_demod_reg))) {
			pr_dbg("copy_to_user copy error AML_DEMOD_GET_REG\n");
		}
		break;

	case AML_DEMOD_SET_MEM:
		/*step=(struct aml_demod_mem)arg;*/
		/* for(i=step;i<1024-1;i++){ */
		/* pr_dbg("0x%x,",mem_buf[i]); */
		/* } */
		mem_read((struct aml_demod_mem *)arg);
		break;

	case AML_DEMOD_ATSC_IRQ:
		atsc_read_iqr_reg();
		break;
	case AML_DEMOD_GET_PLL_INIT:
		val = get_dtvpll_init_flag();

		if (copy_to_user((void __user *)arg, &val,
			sizeof(unsigned int))) {
			pr_dbg("copy_to_user error AML_DEMOD_GET_PLL_INIT\n");
		}
		break;

	case AML_DEMOD_GET_CAPTURE_ADDR:
		val = devp->mem_start;

		if (copy_to_user((void __user *)arg, &val,
				 sizeof(unsigned int)))
			pr_dbg("copy_to_user error AML_DEMOD_GET_CAPTURE_ADDR\n");
		break;

	case AML_DEMOD_SET_ID:
		if (copy_from_user(&demod_id, (void __user *)arg,
			sizeof(demod_id)))
			pr_dbg("copy error AML_DEMOD_SET_ID\n");
		else
			pr_dbg("set demod_id %d.\n", demod_id);
		break;

	case DEMOD_IOC_START_DUMP_ADC:
		ret = copy_from_user(&dump_param, argp, sizeof(unsigned int));
		if (ret < 0) {
			PR_ERR("Error user param\n");
			return -EINVAL;
		}
		PR_INFO("%s dump_param %d\n", __func__, dump_param);
		capture_adc_data_once(NULL, dump_param, 0, priv);
		PR_INFO("%s %#x %d\n", __func__, priv->data, priv->size);

		ret = copy_to_user(argp, &priv->size, sizeof(unsigned int));
		if (ret < 0) {
			PR_ERR("Error user param\n");
			return -EINVAL;
		}
		break;
	case DEMOD_IOC_STOP_DUMP_ADC:
		break;
	case DEMOD_IOC_START_DUMP_TS:
		ret = copy_from_user(&dump_param, argp, sizeof(unsigned int));
		if (ret < 0) {
			PR_ERR("Error user param\n");
			return -EINVAL;
		}
		PR_INFO("%s dump_param %d\n", __func__, dump_param);
		break;
	case DEMOD_IOC_STOP_DUMP_TS:
		break;

	default:
		pr_dbg("enter Default! 0x%X\n", cmd);
		return -EINVAL;
	}

	mutex_unlock(&demod_lock);

	return 0;
}

#ifdef CONFIG_COMPAT

static long aml_demod_compat_ioctl(struct file *file, unsigned int cmd,
				   ulong arg)
{
	return aml_demod_ioctl(file, cmd, (ulong)compat_ptr(arg));
}

#endif

static int aml_demod_mmap(struct file *fp, struct vm_area_struct *vma)
{
	int ret = 0;
	unsigned long phyaddr = 0;
	unsigned long size = 0;
	struct demod_priv *priv = fp->private_data;
	struct amldtvdemod_device_s *devp = dtvdemod_get_dev();

	mutex_lock(&demod_lock);

	if (!vma) {
		PR_ERR("input error: vma is NULL\n");
		return -EINVAL;
	}
	if (!priv) {
		PR_ERR("priv is NULL!\n");
		return -EINVAL;
	}

	if (unlikely(!devp)) {
		PR_ERR("%s:devp is NULL\n", __func__);
		return -1;
	}

	if (!devp->flg_cma_allc || !devp->cma_mem_size) {
		PR_ERR("%s: not enter_mode or invalid cma_mem_size!!\n", __func__);
		return -1;
	}

	if (!priv->data) {
		PR_ERR("%s: priv->data is NULL\n", __func__);
		return -1;
	}
	PR_INFO("%s: %s\n", __func__,
			PageHighMem(phys_to_page(priv->data)) ? "high_mem" : "low_mem");

	phyaddr = (unsigned long)priv->data;
	size = vma->vm_end - vma->vm_start;
	PR_INFO("vma=0x%pK, size=%ld, vm_start=%#lx, end=%#lx.\n", vma,
			vma->vm_end - vma->vm_start, vma->vm_start, vma->vm_end);

	vma->vm_page_prot = PAGE_SHARED;
	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);

	if (size > priv->size)
		size = priv->size;

	ret = remap_pfn_range(vma, vma->vm_start, phyaddr >> PAGE_SHIFT, size,
			vma->vm_page_prot);
	if (ret != 0) {
		PR_ERR("remap_pfn_range failed, ret=%d\n", ret);
		return -1;
	}

	mutex_unlock(&demod_lock);

	return ret;
}

static const struct file_operations aml_demod_fops = {
	.owner		= THIS_MODULE,
	.open		= aml_demod_open,
	.release	= aml_demod_release,
	.unlocked_ioctl = aml_demod_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= aml_demod_compat_ioctl,
#endif
	.mmap			= aml_demod_mmap,
};

/*
static int aml_demod_ui_open(struct inode *inode, struct file *file)
{
	pr_dbg("Amlogic aml_demod_ui_open Open\n");
	return 0;
}

static int aml_demod_ui_release(struct inode *inode, struct file *file)
{
	pr_dbg("Amlogic aml_demod_ui_open Release\n");
	return 0;
}
char buf_all[100];
static ssize_t aml_demod_ui_read(struct file *file, char __user *buf,
				 size_t count, loff_t *ppos)
{
	char *capture_buf = buf_all;
	int res = 0;

#if 0
	if (count >= 4 * 1024 * 1024)
		count = 4 * 1024 * 1024;
	else if (count == 0)
		return 0;
#endif
	if (count == 0)
		return 0;

	count = min_t(size_t, count, (sizeof(buf_all)-1));
	res = copy_to_user((void *)buf, (char *)capture_buf, count);
	if (res < 0) {
		pr_dbg("[aml_demod_ui_read]res is %d", res);
		return res;
	}

	return count;
}

static ssize_t aml_demod_ui_write(struct file *file, const char *buf,
				  size_t count, loff_t *ppos)
{
	return 0;
}

static struct device *aml_demod_ui_dev;
static dev_t aml_demod_devno_ui;
static struct cdev *aml_demod_cdevp_ui;
static const struct file_operations aml_demod_ui_fops = {
	.owner		= THIS_MODULE,
	.open		= aml_demod_ui_open,
	.release	= aml_demod_ui_release,
	.read		= aml_demod_ui_read,
	.write		= aml_demod_ui_write,
};

#if 0
static ssize_t aml_demod_ui_info(struct class *cla,
				 struct class_attribute *attr, char *buf)
{
	return 0;
}

static struct class_attribute aml_demod_ui_class_attrs[] = {
	__ATTR(info,
	       0644,
	       aml_demod_ui_info,
	       NULL),
	__ATTR_NULL
};
#endif

static struct class aml_demod_ui_class = {
	.name	= "aml_demod_ui",
//    .class_attrs = aml_demod_ui_class_attrs,
};

int aml_demod_ui_init(void)
{
	int r = 0;

	r = class_register(&aml_demod_ui_class);
	if (r) {
		pr_dbg("create aml_demod class fail\r\n");
		class_unregister(&aml_demod_ui_class);
		return r;
	}

	r = alloc_chrdev_region(&aml_demod_devno_ui, 0, 1, DEVICE_UI_NAME);
	if (r < 0) {
		PR_ERR("aml_demod_ui: failed to alloc major number\n");
		r = -ENODEV;
		unregister_chrdev_region(aml_demod_devno_ui, 1);
		class_unregister(&aml_demod_ui_class);
		return r;
	}

	aml_demod_cdevp_ui = kmalloc(sizeof(struct cdev), GFP_KERNEL);
	if (!aml_demod_cdevp_ui) {
		PR_ERR("aml_demod_ui: failed to allocate memory\n");
		r = -ENOMEM;
		unregister_chrdev_region(aml_demod_devno_ui, 1);
		kfree(aml_demod_cdevp_ui);
		class_unregister(&aml_demod_ui_class);
		return r;
	}
	// connect the file operation with cdev
	cdev_init(aml_demod_cdevp_ui, &aml_demod_ui_fops);
	aml_demod_cdevp_ui->owner = THIS_MODULE;
	// connect the major/minor number to cdev
	r = cdev_add(aml_demod_cdevp_ui, aml_demod_devno_ui, 1);
	if (r) {
		PR_ERR("aml_demod_ui:failed to add cdev\n");
		unregister_chrdev_region(aml_demod_devno_ui, 1);
		cdev_del(aml_demod_cdevp_ui);
		kfree(aml_demod_cdevp_ui);
		class_unregister(&aml_demod_ui_class);
		return r;
	}

	aml_demod_ui_dev = device_create(&aml_demod_ui_class, NULL,
					 MKDEV(MAJOR(aml_demod_devno_ui), 0),
					 NULL, DEVICE_UI_NAME);

	if (IS_ERR(aml_demod_ui_dev)) {
		pr_dbg("Can't create aml_demod device\n");
		unregister_chrdev_region(aml_demod_devno_ui, 1);
		cdev_del(aml_demod_cdevp_ui);
		kfree(aml_demod_cdevp_ui);
		class_unregister(&aml_demod_ui_class);
		return r;
	}

	return r;
}

void aml_demod_exit_ui(void)
{
	unregister_chrdev_region(aml_demod_devno_ui, 1);
	cdev_del(aml_demod_cdevp_ui);
	kfree(aml_demod_cdevp_ui);
	class_unregister(&aml_demod_ui_class);
}
*/

static dev_t aml_demod_devno;
static struct demod_device_t *demod_dev;

#ifdef CONFIG_AM_DEMOD_DVBAPI
int aml_demod_init(void)
#else
static int __init aml_demod_init(void)
#endif
{
	int r = 0;
	struct demod_priv *priv;
	struct device *aml_demod_dev;

	pr_dbg("Amlogic Demod DVB-T/C DebugIF Init\n");

	init_waitqueue_head(&lock_wq);

	/* hook demod isr */
	/* r = request_irq(INT_DEMOD, &aml_demod_isr, */
	/*              IRQF_SHARED, "aml_demod", */
	/*              (void *)aml_demod_dev_id); */
	/* if (r) { */
	/*      pr_dbg("aml_demod irq register error.\n"); */
	/*      r = -ENOENT; */
	/*      goto err0; */
	/* } */

	/* sysfs node creation */
	r = class_register(&aml_demod_class);
	if (r) {
		pr_dbg("create aml_demod class fail\r\n");
		goto err1;
	}

	r = alloc_chrdev_region(&aml_demod_devno, 0, 1, DEVICE_NAME);
	if (r < 0) {
		PR_ERR("aml_demod: failed to alloc major number\n");
		r = -ENODEV;
		goto err2;
	}

	//init demod_device_t
	demod_dev = kzalloc(sizeof(*demod_dev), GFP_KERNEL);
	if (!demod_dev) {
		PR_ERR("kzalloc for demod_device_t error\n");
		r = -ENOMEM;
		goto err3;
	}

	//init demod_priv
	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		PR_ERR("kzalloc for demod_priv error\n");
		r = -ENOMEM;
		goto err4;
	}

	/* connect the file operation with cdev */
	cdev_init(&demod_dev->dev, &aml_demod_fops);
	demod_dev->dev.owner = THIS_MODULE;
	/* connect the major/minor number to cdev */
	r = cdev_add(&demod_dev->dev, aml_demod_devno, 1);
	if (r) {
		PR_ERR("aml_demod:failed to add cdev\n");
		goto err5;
	}

	aml_demod_dev = device_create(&aml_demod_class, NULL,
				      MKDEV(MAJOR(aml_demod_devno), 0), NULL,
				      DEVICE_NAME);

	if (IS_ERR(aml_demod_dev)) {
		pr_dbg("Can't create aml_demod device\n");
		goto err6;
	}

	priv->dev = aml_demod_dev;
	strncpy(priv->name, DEVICE_NAME, sizeof(priv->name));
	priv->name[sizeof(priv->name) - 1] = '\0';
	priv->class = &aml_demod_class;
	demod_dev->priv = priv;
	mutex_init(&demod_lock);

	pr_dbg("Amlogic Demod DVB-T/C DebugIF Init ok----------------\n");
#if defined(CONFIG_AM_AMDEMOD_FPGA_VER) && !defined(CONFIG_AM_DEMOD_DVBAPI)
	pr_dbg("sdio_init\n");
	sdio_init();
#endif
	//aml_demod_ui_init();
	aml_demod_dbg_init();

	return 0;

err6:
	cdev_del(&demod_dev->dev);
err5:
	kfree(priv);
err4:
	kfree(demod_dev);
err3:
	unregister_chrdev_region(aml_demod_devno, 1);

err2:
/*    free_irq(INT_DEMOD, (void *)aml_demod_dev_id);*/

err1:
	class_unregister(&aml_demod_class);

/*  err0:*/
	return r;
}

#ifdef CONFIG_AM_DEMOD_DVBAPI
void aml_demod_exit(void)
#else
static void __exit aml_demod_exit(void)
#endif
{
	struct demod_priv *priv = demod_dev->priv;
	pr_dbg("Amlogic Demod DVB-T/C DebugIF Exit\n");

	unregister_chrdev_region(aml_demod_devno, 1);
	device_destroy(priv->class, priv->dev->devt);
	cdev_del(&demod_dev->dev);

	/*   free_irq(INT_DEMOD, (void *)aml_demod_dev_id); */

	class_unregister(priv->class);

	kfree(priv);
	kfree(demod_dev);
	demod_dev = NULL;

	//aml_demod_exit_ui();
	aml_demod_dbg_exit();
}

#ifndef CONFIG_AM_DEMOD_DVBAPI
module_init(aml_demod_init);
module_exit(aml_demod_exit);

MODULE_LICENSE("GPL");
/*MODULE_AUTHOR(DRV_AUTHOR);*/
/*MODULE_DESCRIPTION(DRV_DESC);*/
#endif
