// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
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
 */

#include <linux/types.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_fdt.h>
#include <linux/of_reserved_mem.h>
#include <linux/of_irq.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/errno.h>
#include <linux/uaccess.h>
#include <linux/ioport.h>
#include <linux/ctype.h>
#include <linux/vmalloc.h>
#include <linux/interrupt.h>
#include <linux/pm_runtime.h>
#include <linux/amlogic/media/frc/frc_reg.h>
#include <linux/amlogic/media/frc/frc_common.h>
#include <linux/amlogic/media/vout/vinfo.h>
#include <linux/amlogic/media/vout/vout_notify.h>
#include <linux/amlogic/power_domain.h>

#include "frc_drv.h"
#include "frc_dbg.h"
#include "frc_buf.h"
#include "frc_hw.h"
#include "frc_proc.h"
#include "frc_memc_dbg.h"

ssize_t frc_bbd_ctrl_param_show(struct class *class,
	struct class_attribute *attr,
	char *buf)
{
	struct frc_dev_s *devp = get_frc_devp();
	struct frc_fw_data_s *fw_data;
	ssize_t len = 0;

	fw_data = (struct frc_fw_data_s *)devp->fw_data;
	len =  fw_data->frc_alg_dbg_show(fw_data, MEMC_DBG_BBD_FINAL_LINE, buf);
	return len;
}

ssize_t frc_bbd_ctrl_param_store(struct class *class,
	struct class_attribute *attr,
	const char *buf,
	size_t count)
{
	char *buf_orig;
	struct frc_dev_s *devp = get_frc_devp();
	struct frc_fw_data_s *fw_data = (struct frc_fw_data_s *)devp->fw_data;

	buf_orig = kstrdup(buf, GFP_KERNEL);
	count = fw_data->frc_alg_dbg_stor(fw_data, MEMC_DBG_BBD_FINAL_LINE, buf_orig, count);
	kfree(buf_orig);
	return count;
}

ssize_t frc_vp_ctrl_param_show(struct class *class,
	struct class_attribute *attr,
	char *buf)
{
	struct frc_dev_s *devp = get_frc_devp();
	struct frc_fw_data_s *fw_data = (struct frc_fw_data_s *)devp->fw_data;
	ssize_t len = 0;

	len = fw_data->frc_alg_dbg_show(fw_data, MEMC_DBG_VP_CTRL, buf);
	return len;
}

ssize_t frc_vp_ctrl_param_store(struct class *class,
	struct class_attribute *attr,
	const char *buf,
	size_t count)
{
	char *buf_orig;
	struct frc_dev_s *devp = get_frc_devp();
	struct frc_fw_data_s *fw_data = (struct frc_fw_data_s *)devp->fw_data;

	buf_orig = kstrdup(buf, GFP_KERNEL);
	count = fw_data->frc_alg_dbg_stor(fw_data, MEMC_DBG_VP_CTRL, buf_orig, count);
	kfree(buf_orig);
	return count;
}

ssize_t frc_logo_ctrl_param_show(struct class *class,
	struct class_attribute *attr,
	char *buf)
{
	struct frc_dev_s *devp = get_frc_devp();
	struct frc_fw_data_s *fw_data = (struct frc_fw_data_s *)devp->fw_data;
	ssize_t len = 0;

	len = fw_data->frc_alg_dbg_show(fw_data, MEMC_DBG_LOGO_CTRL, buf);
	return len;
}

ssize_t frc_logo_ctrl_param_store(struct class *class,
	struct class_attribute *attr,
	const char *buf,
	size_t count)
{
	char *buf_orig;
	struct frc_dev_s *devp = get_frc_devp();
	struct frc_fw_data_s *fw_data = (struct frc_fw_data_s *)devp->fw_data;

	buf_orig = kstrdup(buf, GFP_KERNEL);
	count = fw_data->frc_alg_dbg_stor(fw_data, MEMC_DBG_LOGO_CTRL, buf_orig, count);
	kfree(buf_orig);
	return count;
}

ssize_t frc_iplogo_ctrl_param_show(struct class *class,
	struct class_attribute *attr,
	char *buf)
{
	struct frc_dev_s *devp = get_frc_devp();
	struct frc_fw_data_s *fw_data = (struct frc_fw_data_s *)devp->fw_data;
	ssize_t len = 0;

	len = fw_data->frc_alg_dbg_show(fw_data, MEMC_DBG_IPLOGO_CTRL, buf);
	return len;
}

ssize_t frc_iplogo_ctrl_param_store(struct class *class,
	struct class_attribute *attr,
	const char *buf,
	size_t count)
{
	char *buf_orig;
	struct frc_dev_s *devp = get_frc_devp();
	struct frc_fw_data_s *fw_data = (struct frc_fw_data_s *)devp->fw_data;

	buf_orig = kstrdup(buf, GFP_KERNEL);
	count = fw_data->frc_alg_dbg_stor(fw_data, MEMC_DBG_IPLOGO_CTRL, buf_orig, count);
	kfree(buf_orig);
	return count;
}

ssize_t frc_melogo_ctrl_param_show(struct class *class,
	struct class_attribute *attr,
	char *buf)
{
	struct frc_dev_s *devp = get_frc_devp();
	struct frc_fw_data_s *fw_data = (struct frc_fw_data_s *)devp->fw_data;
	ssize_t len = 0;

	len = fw_data->frc_alg_dbg_show(fw_data, MEMC_DBG_MELOGO_CTRL, buf);
	return len;
}

ssize_t frc_melogo_ctrl_param_store(struct class *class,
	struct class_attribute *attr,
	const char *buf,
	size_t count)
{
	char *buf_orig;
	struct frc_dev_s *devp = get_frc_devp();
	struct frc_fw_data_s *fw_data = (struct frc_fw_data_s *)devp->fw_data;

	buf_orig = kstrdup(buf, GFP_KERNEL);
	count = fw_data->frc_alg_dbg_stor(fw_data, MEMC_DBG_MELOGO_CTRL, buf_orig, count);
	kfree(buf_orig);
	return count;
}

ssize_t frc_scene_chg_detect_param_show(struct class *class,
	struct class_attribute *attr,
	char *buf)
{
	struct frc_dev_s *devp = get_frc_devp();
	struct frc_fw_data_s *fw_data = (struct frc_fw_data_s *)devp->fw_data;
	ssize_t len = 0;

	len = fw_data->frc_alg_dbg_show(fw_data, MEMC_DBG_SCENE_CHG_DETECT, buf);
	return len;
}

ssize_t frc_scene_chg_detect_param_store(struct class *class,
	struct class_attribute *attr,
	const char *buf,
	size_t count)
{
	char *buf_orig;
	struct frc_dev_s *devp = get_frc_devp();
	struct frc_fw_data_s *fw_data = (struct frc_fw_data_s *)devp->fw_data;

	buf_orig = kstrdup(buf, GFP_KERNEL);
	count = fw_data->frc_alg_dbg_stor(fw_data, MEMC_DBG_SCENE_CHG_DETECT, buf_orig, count);
	kfree(buf_orig);
	return count;
}

ssize_t frc_fb_ctrl_param_show(struct class *class,
	struct class_attribute *attr,
	char *buf)
{
	struct frc_dev_s *devp = get_frc_devp();
	struct frc_fw_data_s *fw_data = (struct frc_fw_data_s *)devp->fw_data;
	ssize_t len = 0;

	len = fw_data->frc_alg_dbg_show(fw_data, MEMC_DBG_FB_CTRL, buf);
	return len;
}

ssize_t frc_fb_ctrl_param_store(struct class *class,
	struct class_attribute *attr,
	const char *buf,
	size_t count)
{
	char *buf_orig;
	struct frc_dev_s *devp = get_frc_devp();
	struct frc_fw_data_s *fw_data = (struct frc_fw_data_s *)devp->fw_data;

	buf_orig = kstrdup(buf, GFP_KERNEL);
	count = fw_data->frc_alg_dbg_stor(fw_data, MEMC_DBG_FB_CTRL, buf_orig, count);
	kfree(buf_orig);
	return count;
}

ssize_t frc_me_ctrl_param_show(struct class *class,
	struct class_attribute *attr,
	char *buf)
{
	struct frc_dev_s *devp = get_frc_devp();
	struct frc_fw_data_s *fw_data = (struct frc_fw_data_s *)devp->fw_data;
	ssize_t len = 0;

	len = fw_data->frc_alg_dbg_show(fw_data, MEMC_DBG_ME_CTRL, buf);
	return len;
}

ssize_t frc_me_ctrl_param_store(struct class *class,
	struct class_attribute *attr,
	const char *buf,
	size_t count)
{
	char *buf_orig;
	struct frc_dev_s *devp = get_frc_devp();
	struct frc_fw_data_s *fw_data = (struct frc_fw_data_s *)devp->fw_data;

	buf_orig = kstrdup(buf, GFP_KERNEL);
	count = fw_data->frc_alg_dbg_stor(fw_data, MEMC_DBG_ME_CTRL, buf_orig, count);
	kfree(buf_orig);
	return count;
}

ssize_t frc_search_rang_param_show(struct class *class,
	struct class_attribute *attr,
	char *buf)
{
	struct frc_dev_s *devp = get_frc_devp();
	struct frc_fw_data_s *fw_data = (struct frc_fw_data_s *)devp->fw_data;
	ssize_t len = 0;

	len = fw_data->frc_alg_dbg_show(fw_data, MEMC_DBG_SEARCH_RANG, buf);
	return len;
}

ssize_t frc_search_rang_param_store(struct class *class,
	struct class_attribute *attr,
	const char *buf,
	size_t count)
{
	char *buf_orig;
	struct frc_dev_s *devp = get_frc_devp();
	struct frc_fw_data_s *fw_data = (struct frc_fw_data_s *)devp->fw_data;

	buf_orig = kstrdup(buf, GFP_KERNEL);
	count = fw_data->frc_alg_dbg_stor(fw_data, MEMC_DBG_SEARCH_RANG, buf_orig, count);
	kfree(buf_orig);
	return count;
}

ssize_t frc_mc_ctrl_param_show(struct class *class,
	struct class_attribute *attr,
	char *buf)
{
	struct frc_dev_s *devp = get_frc_devp();
	struct frc_fw_data_s *fw_data = (struct frc_fw_data_s *)devp->fw_data;
	ssize_t len = 0;

	len = fw_data->frc_alg_dbg_show(fw_data, MEMC_DBG_PIXEL_LPF, buf);
	return len;
}

ssize_t frc_mc_ctrl_param_store(struct class *class,
	struct class_attribute *attr,
	const char *buf,
	size_t count)
{
	char *buf_orig;
	struct frc_dev_s *devp = get_frc_devp();
	struct frc_fw_data_s *fw_data = (struct frc_fw_data_s *)devp->fw_data;

	buf_orig = kstrdup(buf, GFP_KERNEL);
	count = fw_data->frc_alg_dbg_stor(fw_data, MEMC_DBG_PIXEL_LPF, buf_orig, count);
	kfree(buf_orig);
	return count;
}

ssize_t frc_me_rule_param_show(struct class *class,
	struct class_attribute *attr,
	char *buf)
{
	struct frc_dev_s *devp = get_frc_devp();
	struct frc_fw_data_s *fw_data = (struct frc_fw_data_s *)devp->fw_data;
	ssize_t len = 0;

	len = fw_data->frc_alg_dbg_show(fw_data, MEMC_DBG_ME_RULE, buf);
	return len;
}

ssize_t frc_me_rule_param_store(struct class *class,
	struct class_attribute *attr,
	const char *buf,
	size_t count)
{
	char *buf_orig;
	struct frc_dev_s *devp = get_frc_devp();
	struct frc_fw_data_s *fw_data = (struct frc_fw_data_s *)devp->fw_data;

	buf_orig = kstrdup(buf, GFP_KERNEL);
	count = fw_data->frc_alg_dbg_stor(fw_data, MEMC_DBG_ME_RULE, buf_orig, count);
	kfree(buf_orig);
	return count;
}

ssize_t frc_film_ctrl_param_show(struct class *class,
	struct class_attribute *attr,
	char *buf)
{
	struct frc_dev_s *devp = get_frc_devp();
	struct frc_fw_data_s *fw_data = (struct frc_fw_data_s *)devp->fw_data;
	ssize_t len = 0;

	len = fw_data->frc_alg_dbg_show(fw_data, MEMC_DBG_FILM_CTRL, buf);
	return len;
}

ssize_t frc_film_ctrl_param_store(struct class *class,
	struct class_attribute *attr,
	const char *buf,
	size_t count)
{
	char *buf_orig;
	struct frc_dev_s *devp = get_frc_devp();
	struct frc_fw_data_s *fw_data = (struct frc_fw_data_s *)devp->fw_data;

	buf_orig = kstrdup(buf, GFP_KERNEL);
	count = fw_data->frc_alg_dbg_stor(fw_data, MEMC_DBG_FILM_CTRL, buf_orig, count);
	kfree(buf_orig);
	return count;
}

ssize_t frc_glb_ctrl_param_show(struct class *class,
	struct class_attribute *attr,
	char *buf)
{
	struct frc_dev_s *devp = get_frc_devp();
	struct frc_fw_data_s *fw_data = (struct frc_fw_data_s *)devp->fw_data;
	ssize_t len = 0;

	len = fw_data->frc_alg_dbg_show(fw_data, MEMC_DBG_GLB_CTRL, buf);
	return len;
}

ssize_t frc_glb_ctrl_param_store(struct class *class,
	struct class_attribute *attr,
	const char *buf,
	size_t count)
{
	char *buf_orig;
	struct frc_dev_s *devp = get_frc_devp();
	struct frc_fw_data_s *fw_data = (struct frc_fw_data_s *)devp->fw_data;

	buf_orig = kstrdup(buf, GFP_KERNEL);
	count = fw_data->frc_alg_dbg_stor(fw_data, MEMC_DBG_GLB_CTRL, buf_orig, count);
	kfree(buf_orig);
	return count;
}

ssize_t frc_bad_edit_ctrl_param_show(struct class *class,
	struct class_attribute *attr,
	char *buf)
{
	struct frc_dev_s *devp = get_frc_devp();
	struct frc_fw_data_s *fw_data = (struct frc_fw_data_s *)devp->fw_data;
	ssize_t len = 0;

	len = fw_data->frc_alg_dbg_show(fw_data, MEMC_DBG_BAD_EDIT_CTRL, buf);
	return len;
}

ssize_t frc_bad_edit_ctrl_param_store(struct class *class,
	struct class_attribute *attr,
	const char *buf,
	size_t count)
{
	char *buf_orig;
	struct frc_dev_s *devp = get_frc_devp();
	struct frc_fw_data_s *fw_data = (struct frc_fw_data_s *)devp->fw_data;

	buf_orig = kstrdup(buf, GFP_KERNEL);
	count = fw_data->frc_alg_dbg_stor(fw_data, MEMC_DBG_BAD_EDIT_CTRL, buf_orig, count);
	kfree(buf_orig);
	return count;
}

ssize_t frc_region_fb_ctrl_param_show(struct class *class,
	struct class_attribute *attr,
	char *buf)
{
	struct frc_dev_s *devp = get_frc_devp();
	struct frc_fw_data_s *fw_data = (struct frc_fw_data_s *)devp->fw_data;
	ssize_t len = 0;

	len = fw_data->frc_alg_dbg_show(fw_data, MEMC_DBG_REGION_FB_CTRL, buf);
	return len;
}

ssize_t frc_region_fb_ctrl_param_store(struct class *class,
	struct class_attribute *attr,
	const char *buf,
	size_t count)
{
	char *buf_orig;
	struct frc_dev_s *devp = get_frc_devp();
	struct frc_fw_data_s *fw_data = (struct frc_fw_data_s *)devp->fw_data;

	buf_orig = kstrdup(buf, GFP_KERNEL);
	count = fw_data->frc_alg_dbg_stor(fw_data, MEMC_DBG_REGION_FB_CTRL, buf_orig, count);
	kfree(buf_orig);
	return count;
}
