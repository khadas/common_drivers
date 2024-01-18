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
#include <linux/reboot.h>
#include <linux/clk.h>
#include <linux/of_device.h>
#include <linux/amlogic/iomap.h>
#include "dsc_dec_drv.h"
#include "dsc_dec_reg.h"
#include "dsc_dec_hw.h"
#include "dsc_dec_config.h"
#include "dsc_dec_debug.h"
#include "../tvin_global.h"

#include <linux/amlogic/gki_module.h>

#define DSC_CDEV_NAME  "aml_dsc_dec"

static struct dsc_dec_cdev_s *dsc_dec_cdev;
static struct aml_dsc_dec_drv_s *dsc_dec_drv_local;

struct aml_dsc_dec_drv_s *dsc_dec_drv_get(void)
{
	return dsc_dec_drv_local;
}

void dsc_dec_en(bool on_off, struct dsc_pps_data_s *pps_data)
{
	if (!dsc_dec_drv_local) {
		DSC_DEC_PR("dsc_dec_drv_local is NULL\n");
		return;
	}

	if (on_off) {
		dsc_dec_drv_local->pps_data = *pps_data;
		dsc_dec_config_init(dsc_dec_drv_local);
		dsc_dec_drv_local->dsc_dec_en = 1;
	} else {
		dsc_dec_drv_local->dsc_dec_en = 0;
		set_dsc_dec_en(0);
	}
}

static inline int dsc_dec_ioremap(struct platform_device *pdev,
				struct aml_dsc_dec_drv_s *dsc_dec_drv)
{
	int i;
	struct resource *res;
	int size_io_reg;

	for (i = 0; i < DSC_DEC_MAP_MAX; i++) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (res) {
			size_io_reg = resource_size(res);
			DSC_DEC_PR("%s: dsc_dec reg base=0x%p,size=0x%x\n",
				__func__, (void *)res->start, size_io_reg);
			dsc_dec_drv->dsc_dec_reg_base[i] =
				devm_ioremap(&pdev->dev, res->start, size_io_reg);
			if (!dsc_dec_drv->dsc_dec_reg_base[i]) {
				dev_err(&pdev->dev, "dsc_dec ioremap failed\n");
				return -ENOMEM;
			}
			DSC_DEC_PR("%s: dsc_dec maped reg_base =0x%p, size=0x%x\n",
					__func__, dsc_dec_drv->dsc_dec_reg_base[i], size_io_reg);
		} else {
			dev_err(&pdev->dev, "missing dsc_dec_reg_base memory resource\n");
			dsc_dec_drv->dsc_dec_reg_base[i] = NULL;
			return -1;
		}
	}
	return 0;
}

/* ************************************************************* */
static int dsc_dec_open(struct inode *inode, struct file *file)
{
	struct aml_dsc_dec_drv_s *dsc_dec_drv;

	dsc_dec_drv = container_of(inode->i_cdev, struct aml_dsc_dec_drv_s, cdev);
	file->private_data = dsc_dec_drv;

	return 0;
}

static int dsc_dec_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

static const struct file_operations dsc_dec_fops = {
	.owner          = THIS_MODULE,
	.open           = dsc_dec_open,
	.release        = dsc_dec_release,
};

static int dsc_dec_cdev_add(struct aml_dsc_dec_drv_s *dsc_dec_drv, struct device *parent)
{
	dev_t devno;
	int ret = 0;

	if (!dsc_dec_cdev || !dsc_dec_drv) {
		ret = 1;
		DSC_DEC_ERR("%s: dsc_dec_drv is null\n", __func__);
		return -1;
	}

	devno = MKDEV(MAJOR(dsc_dec_cdev->devno), 0);

	cdev_init(&dsc_dec_drv->cdev, &dsc_dec_fops);
	dsc_dec_drv->cdev.owner = THIS_MODULE;
	ret = cdev_add(&dsc_dec_drv->cdev, devno, 1);
	if (ret) {
		ret = 2;
		goto dsc_dec_cdev_add_failed;
	}

	dsc_dec_drv->dev = device_create(dsc_dec_cdev->class, parent,
				  devno, NULL, "dsc_dec%d", 0);
	if (IS_ERR_OR_NULL(dsc_dec_drv->dev)) {
		ret = 3;
		goto dsc_dec_cdev_add_failed1;
	}

	dev_set_drvdata(dsc_dec_drv->dev, dsc_dec_drv);
	dsc_dec_drv->dev->of_node = parent->of_node;

	DSC_DEC_PR("%s OK\n", __func__);
	return 0;

dsc_dec_cdev_add_failed1:
	cdev_del(&dsc_dec_drv->cdev);
dsc_dec_cdev_add_failed:
	DSC_DEC_ERR("%s: failed: %d\n", __func__, ret);
	return -1;
}

static void dsc_dec_cdev_remove(struct aml_dsc_dec_drv_s *dsc_dec_drv)
{
	dev_t devno;

	if (!dsc_dec_cdev || !dsc_dec_drv)
		return;

	devno = MKDEV(MAJOR(dsc_dec_cdev->devno), 0);
	device_destroy(dsc_dec_cdev->class, devno);
	cdev_del(&dsc_dec_drv->cdev);
}

static int dsc_dec_global_init_once(void)
{
	int ret;

	dsc_dec_cdev = kzalloc(sizeof(*dsc_dec_cdev), GFP_KERNEL);
	if (!dsc_dec_cdev)
		return -1;

	ret = alloc_chrdev_region(&dsc_dec_cdev->devno, 0,
				  1, DSC_CDEV_NAME);
	if (ret) {
		ret = 1;
		goto dsc_dec_global_init_once_err;
	}

	dsc_dec_cdev->class = class_create(THIS_MODULE, "aml_dsc_dec");
	if (IS_ERR_OR_NULL(dsc_dec_cdev->class)) {
		ret = 2;
		goto dsc_dec_global_init_once_err_1;
	}

	return 0;

dsc_dec_global_init_once_err_1:
	unregister_chrdev_region(dsc_dec_cdev->devno, 1);
dsc_dec_global_init_once_err:
	kfree(dsc_dec_cdev);
	dsc_dec_cdev = NULL;
	DSC_DEC_ERR("%s: failed: %d\n", __func__, ret);
	return -1;
}

static void dsc_dec_global_remove_once(void)
{
	if (!dsc_dec_cdev)
		return;

	class_destroy(dsc_dec_cdev->class);
	unregister_chrdev_region(dsc_dec_cdev->devno, 0);
	kfree(dsc_dec_cdev);
	dsc_dec_cdev = NULL;
}

#ifdef CONFIG_OF

static struct dsc_dec_data_s dsc_dec_data_t3x = {
	.chip_type = DSC_DEC_CHIP_T3X,
	.chip_name = "t3x",
};

static const struct of_device_id dsc_dec_dt_match_table[] = {
	{
		.compatible = "amlogic, dsc_dec-t3x",
		.data = &dsc_dec_data_t3x,
	},
	{}
};
#endif

static int dsc_dec_probe(struct platform_device *pdev)
{
	struct aml_dsc_dec_drv_s *dsc_dec_drv = NULL;
	const struct of_device_id *match = NULL;
	struct dsc_dec_data_s *vdata = NULL;

	match = of_match_device(dsc_dec_dt_match_table, &pdev->dev);
	if (!match) {
		DSC_DEC_ERR("%s: no match table\n", __func__);
		return -1;
	}
	vdata = (struct dsc_dec_data_s *)match->data;
	if (!vdata) {
		DSC_DEC_PR("driver version: %s(%d-%s)\n",
			DSC_DEC_DRV_VERSION, vdata->chip_type,
			vdata->chip_name);
		return -1;
	}

	dsc_dec_global_init_once();

	dsc_dec_drv = kzalloc(sizeof(*dsc_dec_drv), GFP_KERNEL);
	if (!dsc_dec_drv)
		return -ENOMEM;
	dsc_dec_drv_local = dsc_dec_drv;
	dsc_dec_drv->data = vdata;
	DSC_DEC_PR("%s: driver version: %s(%d-%s)\n",
	      __func__, DSC_DEC_DRV_VERSION,
	      vdata->chip_type, vdata->chip_name);

	/* set drvdata */
	platform_set_drvdata(pdev, dsc_dec_drv);
	dsc_dec_cdev_add(dsc_dec_drv, &pdev->dev);

	dsc_dec_debug_file_create(dsc_dec_drv);

	dsc_dec_ioremap(pdev, dsc_dec_drv);
	set_dsc_dec_en(0);
	DSC_DEC_PR("%s, driver initialized ok\n", __func__);

	return 0;
}

static int dsc_dec_remove(struct platform_device *pdev)
{
	int i;
	struct aml_dsc_dec_drv_s *dsc_dec_drv = platform_get_drvdata(pdev);

	if (!dsc_dec_drv)
		return 0;

	for (i = 0; i < DSC_DEC_MAP_MAX; i++) {
		if (dsc_dec_drv->dsc_dec_reg_base[i]) {
			devm_iounmap(&pdev->dev, dsc_dec_drv->dsc_dec_reg_base[i]);
			dsc_dec_drv->dsc_dec_reg_base[i] = NULL;
		}
	}
	dsc_dec_debug_file_remove(dsc_dec_drv);

	/* free drvdata */
	platform_set_drvdata(pdev, NULL);
	dsc_dec_cdev_remove(dsc_dec_drv);

	kfree(dsc_dec_drv);
	dsc_dec_drv = NULL;

	dsc_dec_global_remove_once();

	return 0;
}

static int dsc_dec_resume(struct platform_device *pdev)
{
	return 0;
}

static int dsc_dec_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static void dsc_dec_shutdown(struct platform_device *pdev)
{
}

static struct platform_driver dsc_dec_platform_driver = {
	.probe = dsc_dec_probe,
	.remove = dsc_dec_remove,
	.suspend = dsc_dec_suspend,
	.resume = dsc_dec_resume,
	.shutdown = dsc_dec_shutdown,
	.driver = {
		.name = "meson_dsc_dec",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(dsc_dec_dt_match_table),
#endif
	},
};

int __init dsc_dec_init(void)
{
	if (platform_driver_register(&dsc_dec_platform_driver)) {
		DSC_DEC_ERR("failed to register dsc_dec driver module\n");
		return -ENODEV;
	}

	return 0;
}

void __exit dsc_dec_exit(void)
{
	platform_driver_unregister(&dsc_dec_platform_driver);
}
