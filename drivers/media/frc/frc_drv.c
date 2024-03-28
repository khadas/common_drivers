// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * drivers/amlogic/media/frc/frc_drv.c
 *
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

/* Standard Linux headers */
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
#include <linux/compat.h>
#include <linux/of_irq.h>
#include <linux/of_clk.h>
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
#include <linux/workqueue.h>
#include <linux/clk.h>
#include <linux/sched/clock.h>
#include <linux/pm_runtime.h>
#include <linux/pm_domain.h>
#include <linux/miscdevice.h>
#include <linux/amlogic/media/vout/vout_notify.h>
#include <linux/amlogic/media/codec_mm/codec_mm.h>
#include <linux/amlogic/media/frc/frc_reg.h>
#include <linux/amlogic/media/frc/frc_common.h>
#include <linux/amlogic/power_domain.h>
#if IS_ENABLED(CONFIG_AMLOGIC_DMC_DEV_ACCESS)
#include <linux/amlogic/dmc_dev_access.h>
#endif
#include <dt-bindings/power/t3-pd.h>
#include <dt-bindings/power/t5m-pd.h>
#include <dt-bindings/power/t3x-pd.h>
#include <linux/amlogic/media/video_sink/video_signal_notify.h>
#include <linux/io.h>
#include <linux/of_address.h>
#include <linux/dma-map-ops.h>
#include <linux/cma.h>
#include <linux/genalloc.h>
#include <linux/dma-mapping.h>
#include <linux/timer.h>
#include <linux/hrtimer.h>

#include "frc_drv.h"
#include "frc_proc.h"
#include "frc_dbg.h"
#include "frc_buf.h"
#include "frc_hw.h"
#include "frc_memc_dbg.h"
// #ifdef CONFIG_AMLOGIC_MEDIA_FRC_RDMA
#include "frc_rdma.h"
// #endif
const struct frm_dly_dat_s chip_frc_frame_dly[3][4] = {
	{ // chip_t3  fhd,4k2k,4k1k,other
		{130, 11},
		{222, 28},
		{222, 20},
		{140, 10},
	},
	{ // chip_t5m  fhd,4k2k,4k1k,other
		{110,  5},
		{222, 28},
		{222, 20},
		{140, 10},
	},
	{ // chip_t3x  fhd,4k2k,4k1k,4K2K-120Hz
		{130, 11},
		{120, 14},
		{232, 22},   //{266, 28},
		{220, 20},  // {240, 15},
	},
};

struct hrtimer frc_hi_timer;  // timer

// static struct frc_dev_s *frc_dev; // for SWPL-53056:KASAN: use-after-free
static struct frc_dev_s frc_dev;

int frc_dbg_en;
EXPORT_SYMBOL(frc_dbg_en);
module_param(frc_dbg_en, int, 0664);
MODULE_PARM_DESC(frc_dbg_en, "frc debug level");
struct platform_device *runtime_frc_dev;

struct frc_dev_s *get_frc_devp(void)
{
	// return frc_dev;
	return &frc_dev;
}

int  frc_kerdrv_ver = FRC_KERDRV_VER;
EXPORT_SYMBOL(frc_kerdrv_ver);

// static struct frc_fw_data_s *fw_data;
struct frc_fw_data_s fw_data;  // important 2021_0510
static const char frc_alg_def_ver[] = "alg_ver:default";

struct frc_fw_data_s *frc_get_fw_data(void)
{
	return &fw_data;
}
EXPORT_SYMBOL(frc_get_fw_data);

u32 sizeof_frc_fw_data_struct(void)
{
	return sizeof(struct frc_fw_data_s);
}
EXPORT_SYMBOL(sizeof_frc_fw_data_struct);

static struct class_attribute frc_class_attrs[] = {
	__ATTR(debug, 0644, frc_debug_show, frc_debug_store),
	__ATTR(reg, 0644, frc_reg_show, frc_reg_store),
	__ATTR(tool_debug, 0644, frc_tool_debug_show, frc_tool_debug_store),
	__ATTR(buf, 0644, frc_buf_show, frc_buf_store),
	__ATTR(rdma, 0644, frc_rdma_show, frc_rdma_store),
	__ATTR(param, 0644, frc_param_show, frc_param_store),
	__ATTR(other, 0644, frc_other_show, frc_other_store),
	__ATTR(bbd_ctrl_param, 0644, frc_bbd_ctrl_param_show, frc_bbd_ctrl_param_store),
	__ATTR(vp_ctrl_param, 0644, frc_vp_ctrl_param_show, frc_vp_ctrl_param_store),
	__ATTR(logo_ctrl_param, 0644, frc_logo_ctrl_param_show, frc_logo_ctrl_param_store),
	__ATTR(iplogo_ctrl_param, 0644, frc_iplogo_ctrl_param_show, frc_iplogo_ctrl_param_store),
	__ATTR(melogo_ctrl_param, 0644, frc_melogo_ctrl_param_show, frc_melogo_ctrl_param_store),
	__ATTR(scene_chg_detect_param, 0644, frc_scene_chg_detect_param_show,
		frc_scene_chg_detect_param_store),
	__ATTR(fb_ctrl_param, 0644, frc_fb_ctrl_param_show, frc_fb_ctrl_param_store),
	__ATTR(me_ctrl_param, 0644, frc_me_ctrl_param_show, frc_me_ctrl_param_store),
	__ATTR(search_rang_param, 0644, frc_search_rang_param_show, frc_search_rang_param_store),
	__ATTR(mc_ctrl_param, 0644, frc_mc_ctrl_param_show, frc_mc_ctrl_param_store),
	__ATTR(me_rule_param, 0644, frc_me_rule_param_show, frc_me_rule_param_store),
	__ATTR(film_ctrl_param, 0644, frc_film_ctrl_param_show, frc_film_ctrl_param_store),
	__ATTR(glb_ctrl_param, 0644, frc_glb_ctrl_param_show, frc_glb_ctrl_param_store),
	__ATTR(bad_edit_ctrl_param, 0644, frc_bad_edit_ctrl_param_show,
			frc_bad_edit_ctrl_param_store),
	__ATTR(region_fb_ctrl_param, 0644, frc_region_fb_ctrl_param_show,
			frc_region_fb_ctrl_param_store),
	__ATTR(trace_enable, 0664,
	       frc_rdma_trace_enable_show, frc_rdma_trace_enable_stroe),
	__ATTR(trace_reg, 0664,
	       frc_rdma_trace_reg_show, frc_rdma_trace_reg_stroe),
	__ATTR_NULL
};

static int frc_open(struct inode *inode, struct file *file)
{
	struct frc_dev_s *frc_devp;

	frc_devp = container_of(inode->i_cdev, struct frc_dev_s, cdev);
	file->private_data = frc_devp;

	return 0;
}

static int frc_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

static long frc_ioctl(struct file *file,
	unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct frc_dev_s *devp;
	void __user *argp = (void __user *)arg;
	u32 data;
	u8  tmpver[32];
	enum frc_fpp_state_e fpp_state;
	struct v4l2_ext_memc_motion_comp_info comp_info;

	devp = file->private_data;
	if (!devp)
		return -EFAULT;

	if (!devp->probe_ok)
		return -EFAULT;

	if (frc_dbg_ctrl) {
		pr_frc(0, "return frc ioc\n");
		return 0;
	}

	switch (cmd) {
	case FRC_IOC_GET_FRC_EN:
		data = devp->frc_en;
		if (copy_to_user(argp, &data, sizeof(u32)))
			ret = -EFAULT;
		break;

	case FRC_IOC_GET_FRC_STS:
		data = (u32)devp->frc_sts.state;
		if (copy_to_user(argp, &data, sizeof(u32)))
			ret = -EFAULT;
		break;

	case FRC_IOC_SET_FRC_CANDENCE:
		if (copy_from_user(&data, argp, sizeof(u32))) {
			ret = -EFAULT;
			break;
		}
		pr_frc(1, "SET_FRC_CANDENCE:%d\n", data);
		break;

	case FRC_IOC_GET_VIDEO_LATENCY:
		data = (u32)frc_get_video_latency();
		if (copy_to_user(argp, &data, sizeof(u32)))
			ret = -EFAULT;
		break;

	case FRC_IOC_GET_IS_ON:
		data = (u32)frc_is_on();
		if (copy_to_user(argp, &data, sizeof(u32)))
			ret = -EFAULT;
		break;

	case FRC_IOC_SET_DEBLUR_LEVEL:
		if (copy_from_user(&data, argp, sizeof(u32))) {
			ret = -EFAULT;
			break;
		}
		frc_memc_set_deblur(data);
		break;

	case FRC_IOC_SET_MEMC_ON_OFF:
		if (copy_from_user(&data, argp, sizeof(u32))) {
			ret = -EFAULT;
			break;
		}

		pr_frc(1, "set memc_autoctrl:%d boot_timestamp_en%d boot_check%d\n",
		data, devp->in_sts.boot_timestamp_en, devp->in_sts.boot_check_finished);
		if (data) {
//			if (devp->in_sts.boot_timestamp_en &&
//				!devp->in_sts.boot_check_finished) {
//				devp->in_sts.auto_ctrl_reserved = 1;
//				devp->frc_sts.auto_ctrl = false;
//				frc_change_to_state(FRC_STATE_DISABLE);
//				pr_frc(1, "set memc_autoctrl-1:%d\n", data);
//			} else if (!devp->frc_sts.auto_ctrl) {
			if (!devp->frc_sts.auto_ctrl) {
				devp->frc_sts.auto_ctrl = true;
//				devp->frc_sts.re_config = true;
				frc_change_to_state(FRC_STATE_ENABLE);
				pr_frc(1, "set memc_autoctrl-2:%d\n", data);
			}
		} else {
			devp->frc_sts.auto_ctrl = false;
			//if (devp->frc_sts.state != FRC_STATE_BYPASS)
				frc_change_to_state(FRC_STATE_DISABLE);
		}
		break;

	case FRC_IOC_SET_MEMC_LEVEL:
		if (copy_from_user(&data, argp, sizeof(u32))) {
			ret = -EFAULT;
			break;
		}
		frc_memc_set_level(data);
		// pr_frc(1, "SET_MEMC_LEVEL:%d\n", data);
		break;

	case FRC_IOC_SET_MEMC_DMEO_MODE:
		if (copy_from_user(&data, argp, sizeof(u32))) {
			ret = -EFAULT;
			break;
		}
		frc_memc_set_demo(data);
		// pr_frc(1, "SET_MEMC_DEMO:%d\n", data);
		break;

	case FRC_IOC_SET_FPP_MEMC_LEVEL:
		if (copy_from_user(&fpp_state, argp,
			sizeof(enum frc_fpp_state_e))) {
			pr_frc(1, "fpp copy from user error!/n");
			ret = -EFAULT;
			break;
		}
		if (fpp_state == FPP_MEMC_OFF)
			frc_fpp_memc_set_level(0, 0);
		else if (fpp_state == FPP_MEMC_LOW)
			frc_fpp_memc_set_level(9, 0);
		else if (fpp_state == FPP_MEMC_MID)
			frc_fpp_memc_set_level(10, 0);
		else if (fpp_state == FPP_MEMC_HIGH)
			frc_fpp_memc_set_level(10, 1);
		else if (fpp_state == FPP_MEMC_24PFILM)
			frc_fpp_memc_set_level(10, 2);
		else
			frc_fpp_memc_set_level((u8)fpp_state, 0);

		pr_frc(1, "SET_FPP_MEMC_LEVEL:%d\n", fpp_state);
		break;
	case FRC_IOC_SET_MEMC_VENDOR:
		if (copy_from_user(&data, argp, sizeof(u32))) {
			ret = -EFAULT;
			break;
		}
		frc_tell_alg_vendor(data);
		break;
	case FRC_IOC_SET_MEMC_FB:
		if (copy_from_user(&data, argp, sizeof(u32))) {
			ret = -EFAULT;
			break;
		}
		frc_set_memc_fallback(data);
		break;
	case FRC_IOC_SET_MEMC_FILM:
		if (copy_from_user(&data, argp, sizeof(u32))) {
			ret = -EFAULT;
			break;
		}
		frc_set_film_support(data);
		break;
	case FRC_IOC_SET_MEMC_N2M:
		if (copy_from_user(&data, argp, sizeof(u32))) {
			ret = -EFAULT;
			break;
		}
		if (devp->auto_n2m == 0)
			frc_set_n2m(data);
		break;
	case FRC_IOC_GET_MEMC_VERSION:
		strncpy(&tmpver[0], &fw_data.frc_alg_ver[0], sizeof(u8) * 32);
		if (copy_to_user(argp, tmpver, sizeof(u8) * 32))
			ret = -EFAULT;
		break;
	case PQ_MEMC_IOC_SET_LGE_MEMC_LEVEL:
		if (copy_from_user(&comp_info, argp,
			sizeof(struct v4l2_ext_memc_motion_comp_info))) {
			pr_frc(1, "lge copy from user error!/n");
			ret = -EFAULT;
			break;
		}
		// parm1 control memc level, parm2 control fullback reserved
		frc_lge_memc_set_level(comp_info);

		pr_frc(1, "SET_LGE_MEMC_LEVEL\n");
		break;
	case PQ_MEMC_IOC_GET_LGE_MEMC_LEVEL:
		frc_lge_memc_get_level(&comp_info);
		if (copy_to_user(argp, &comp_info,
			sizeof(struct v4l2_ext_memc_motion_comp_info))) {
			pr_frc(0, "lge copy from user error!/n");
			ret = -EFAULT;
			break;
		}
		pr_frc(1, "GET_LGE_MEMC_LEVEL:%d\n", comp_info.memc_type);
		break;
	case PQ_MEMC_IOC_LGE_SET_MEMC_INIT:
		if (copy_from_user(&data, argp, sizeof(u32))) {
			ret = -EFAULT;
			break;
		}
		pr_frc(1, "parm:%d\n", data);
		frc_lge_memc_init();
		break;
	}

	return ret;
}

static const struct dts_match_data dts_match_t3 = {
	.chip = ID_T3,
};

static const struct dts_match_data dts_match_t5m = {
	.chip = ID_T5M,
};

static const struct dts_match_data dts_match_t3x = {
	.chip = ID_T3X,
};

static const struct of_device_id frc_dts_match[] = {
	{
		.compatible = "amlogic, t3_frc",
		.data = &dts_match_t3,
	},
	{
		.compatible = "amlogic, t5m_frc",
		.data = &dts_match_t5m,
	},
	{
		.compatible = "amlogic, t3x_frc",
		.data = &dts_match_t3x,
	},
	{},
};

int frc_attach_pd(struct frc_dev_s *devp)
{
	enum chip_id chip;
	struct device_link *link;
	int i;
	u32 pd_cnt;
	char *pd_name[3] = {"frc-top", "frc-me", "frc-mc"};
	struct platform_device *pdev = devp->pdev;

	chip = get_chip_type();

	if (chip == ID_T5M || chip == ID_T3X)
		return 0;

	if (pdev->dev.pm_domain) {
		pr_frc(0, "%s err pm domain\n", __func__);
		return -1;
	}
	pd_cnt = 3;
	for (i = 0; i < pd_cnt; i++) {
		devp->pd_dev = dev_pm_domain_attach_by_name(&pdev->dev, pd_name[i]);
		if (IS_ERR(devp->pd_dev))
			return PTR_ERR(devp->pd_dev);
		if (!devp->pd_dev)
			return -1;

		link = device_link_add(&pdev->dev, devp->pd_dev,
				       DL_FLAG_STATELESS | DL_FLAG_PM_RUNTIME | DL_FLAG_RPM_ACTIVE);
		if (!link) {
			pr_frc(0, "%s fail to add device_link idx (%d) pd\n", __func__, i);
			return -EINVAL;
		}
		pr_frc(1, "pw domain %s attach\n", pd_name[i]);
	}
	return 0;
}

void frc_power_domain_ctrl(struct frc_dev_s *devp, u32 onoff)
{
	struct frc_data_s *frc_data;
	struct frc_fw_data_s *pfw_data;
	enum chip_id chip;

	frc_data = (struct frc_data_s *)devp->data;
	pfw_data = (struct frc_fw_data_s *)devp->fw_data;
	chip = frc_data->match_data->chip;

#define K_MEMC_CLK_DIS

	if (devp->power_on_flag == onoff) {
		// pr_frc(0, "warning: same pw state\n");
		return;
	}
	if (!onoff) {
		// devp->power_on_flag = false;
		frc_change_to_state(FRC_STATE_BYPASS);
		set_frc_enable(false);
		set_frc_bypass(true);
		frc_state_change_finish(devp);
		set_frc_clk_disable(devp, 1);
		devp->power_on_flag = false;
	}

	if (chip == ID_T3) {
		if (onoff) {
#ifdef K_MEMC_CLK_DIS
			pwr_ctrl_psci_smc(PDID_T3_FRCTOP, PWR_ON);
			pwr_ctrl_psci_smc(PDID_T3_FRCME, PWR_ON);
			pwr_ctrl_psci_smc(PDID_T3_FRCMC, PWR_ON);
#endif
			devp->power_on_flag = true;
			if (devp->clk_me || devp->clk_frc) {
				set_frc_clk_disable(devp, 0);
			} else {
				devp->clk_frc = clk_get(&devp->pdev->dev, "clk_frc");
				devp->clk_me = clk_get(&devp->pdev->dev, "clk_me");
				frc_clk_init(devp);
			}
			// alloc frc buf according to status of alloced
			if (!devp->buf.cma_mem_alloced) {
				frc_buf_alloc(devp);
			}
			frc_init_config(devp);
			frc_buf_config(devp);
			frc_internal_initial(devp);
			frc_hw_initial(devp);
			if (pfw_data->frc_fw_reinit)
				pfw_data->frc_fw_reinit();
		} else {
#ifdef K_MEMC_CLK_DIS
			pwr_ctrl_psci_smc(PDID_T3_FRCTOP, PWR_OFF);
			pwr_ctrl_psci_smc(PDID_T3_FRCME, PWR_OFF);
			pwr_ctrl_psci_smc(PDID_T3_FRCMC, PWR_OFF);
#endif
		}
		pr_frc(2, "t3 power domain power %d\n", onoff);
	} else if (chip == ID_T5M) {
		if (onoff) {
#ifdef K_MEMC_CLK_DIS
			pwr_ctrl_psci_smc(PDID_T5M_FRC_TOP, PWR_ON);
#endif
			devp->power_on_flag = true;
			pr_frc(0, "%s clk set\n", __func__);
			if (devp->clk_me || devp->clk_frc) {
				set_frc_clk_disable(devp, 0);
			} else {
				devp->clk_frc = clk_get(&devp->pdev->dev, "clk_frc");
				devp->clk_me = clk_get(&devp->pdev->dev, "clk_me");
				frc_clk_init(devp);
			}
			if (!devp->buf.cma_mem_alloced)
				frc_buf_alloc(devp);
			frc_init_config(devp);
			frc_buf_config(devp);
			frc_internal_initial(devp);
			frc_hw_initial(devp);
			if (pfw_data->frc_fw_reinit)
				pfw_data->frc_fw_reinit();
		} else {
#ifdef K_MEMC_CLK_DIS
			pwr_ctrl_psci_smc(PDID_T5M_FRC_TOP, PWR_OFF);

#endif
		}
		pr_frc(2, "t5m power domain power. %d\n", onoff);

	} else if (chip == ID_T3X) {
		if (onoff) {
#ifdef K_MEMC_CLK_DIS
			pwr_ctrl_psci_smc(PDID_T3X_FRC_TOP, PWR_ON);
#endif
			devp->power_on_flag = true;
			if (devp->clk_me || devp->clk_frc) {
				set_frc_clk_disable(devp, 0);
			} else {
				devp->clk_frc = clk_get(&devp->pdev->dev, "clk_frc");
				devp->clk_me = clk_get(&devp->pdev->dev, "clk_me");
				frc_clk_init(devp);
			}
			if (!devp->buf.cma_mem_alloced)
				frc_buf_alloc(devp);
			set_frc_clk_disable(devp, 0);
			frc_init_config(devp);
			frc_buf_config(devp);
			frc_internal_initial(devp);
			frc_hw_initial(devp);
			if (pfw_data->frc_fw_reinit)
				pfw_data->frc_fw_reinit();
		} else {
			set_frc_clk_disable(devp, 1);
#ifdef K_MEMC_CLK_DIS
			pwr_ctrl_psci_smc(PDID_T3X_FRC_TOP, PWR_OFF);
#endif
		}
		pr_frc(2, "t3x power domain power %d\n", onoff);

	}
	// if (onoff)
	//	devp->power_on_flag = true;
}

static int frc_dts_parse(struct frc_dev_s *frc_devp)
{
	struct device_node *of_node;
	unsigned int val;
	int ret = 0;
	const struct of_device_id *of_id;
	struct platform_device *pdev = frc_devp->pdev;
	struct resource *res;
	resource_size_t *base;
	struct frc_data_s *frc_data;
	struct device_node *np = pdev->dev.of_node;
	struct frc_fw_data_s *pfw_data;

	of_node = pdev->dev.of_node;
	of_id = of_match_device(frc_dts_match, &pdev->dev);
	if (of_id) {
		// PR_FRC("%s\n", of_id->compatible);
		frc_data = frc_devp->data;
		pfw_data = (struct frc_fw_data_s *)frc_devp->fw_data;
		frc_data->match_data = of_id->data;
		PR_FRC("%s\tchip id:%d\n", of_id->compatible, frc_data->match_data->chip);
		pfw_data->frc_top_type.chip = (u8)frc_data->match_data->chip;
	}

	if (of_node) {
		ret = of_property_read_u32(of_node, "frc_en", &val);
		if (ret) {
			PR_FRC("Can't find frc_en.\n");
			frc_devp->frc_en = 0;
		} else {
			frc_devp->frc_en = val;
		}
		ret = of_property_read_u32(of_node, "frc_hw_pos", &val);
		if (ret)
			PR_FRC("Can't find frc_hw_pos.\n");
		else
			frc_devp->frc_hw_pos = val;
	}
	pr_frc(0, "frc_en:%d, frc_hw_pos:%d\n", frc_devp->frc_en, frc_devp->frc_hw_pos);
	/*get irq number from dts*/
	frc_devp->in_irq = of_irq_get_byname(of_node, "irq_frc_in");
	snprintf(frc_devp->in_irq_name, sizeof(frc_devp->in_irq_name), "frc_input_irq");
//	PR_FRC("%s=%d\n", frc_devp->in_irq_name, frc_devp->in_irq);
	if (frc_devp->in_irq > 0) {
		ret = request_irq(frc_devp->in_irq, frc_input_isr, IRQF_SHARED,
				  frc_devp->in_irq_name, (void *)frc_devp);
		if (ret)
			PR_ERR("request in irq fail\n");
		else
			disable_irq(frc_devp->in_irq);
	}

	frc_devp->out_irq = of_irq_get_byname(of_node, "irq_frc_out");
	snprintf(frc_devp->out_irq_name, sizeof(frc_devp->out_irq_name), "frc_out_irq");
	//	PR_FRC("%s=%d\n", frc_devp->out_irq_name, frc_devp->out_irq);
	if (frc_devp->out_irq > 0) {
		ret = request_irq(frc_devp->out_irq, frc_output_isr, IRQF_SHARED,
				  frc_devp->out_irq_name, (void *)frc_devp);
		if (ret)
			PR_ERR("request out irq fail\n");
		else
			disable_irq(frc_devp->out_irq);
	}

	frc_devp->axi_crash_irq = of_irq_get_byname(of_node, "irq_axi_crash");
	snprintf(frc_devp->axi_crash_irq_name,
			sizeof(frc_devp->axi_crash_irq_name), "axi_crash_irq");
	//	PR_FRC("%s=%d\n", frc_devp->axi_crash_irq_name, frc_devp->axi_crash_irq);
	if (frc_devp->axi_crash_irq > 0) {
		ret = request_irq(frc_devp->axi_crash_irq, frc_axi_crash_isr, IRQF_SHARED,
				  frc_devp->axi_crash_irq_name, (void *)frc_devp);
		if (ret)
			PR_ERR("request axi_crash irq fail\n");
		else
			disable_irq(frc_devp->axi_crash_irq);
	} else {
		PR_ERR("axi_crash irq is not enabled\n");
	}
	frc_devp->rdma_irq = of_irq_get_byname(of_node, "irq_frc_rdma");
	snprintf(frc_devp->rdma_irq_name, sizeof(frc_devp->rdma_irq_name), "frc_rdma_irq");
	PR_FRC("%s=%d\t%s=%d\t%s=%d\t%s=%d\n", frc_devp->in_irq_name, frc_devp->in_irq,
			frc_devp->out_irq_name, frc_devp->out_irq,
			frc_devp->axi_crash_irq_name, frc_devp->axi_crash_irq,
			frc_devp->rdma_irq_name, frc_devp->rdma_irq);
	// #ifdef CONFIG_AMLOGIC_MEDIA_FRC_RDMA
	if (frc_devp->rdma_irq > 0) {
		ret = request_irq(frc_devp->rdma_irq, frc_rdma_isr, IRQF_SHARED,
				frc_devp->rdma_irq_name, (void *)frc_devp);
		if (ret)
			PR_ERR("request rdma irq fail\n");
		else
			disable_irq(frc_devp->rdma_irq);
	}
	// #endif
	/*register map*/
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "frc_reg");
	if (res) {
		base = devm_ioremap(&pdev->dev, res->start, res->end - res->start);
		if (!base) {
			PR_ERR("Unable to map reg base\n");
			frc_devp->reg = NULL;
			frc_base = NULL;
		} else {
			frc_devp->reg = (void *)base;
			frc_base = frc_devp->reg;
		}
	} else {
		frc_devp->reg = NULL;
		frc_base = NULL;
	}
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "frc_clk_reg");
	if (res) {
		base = devm_ioremap(&pdev->dev, res->start, res->end - res->start);
		if (!base) {
			PR_ERR("Unable to map frc clk reg base\n");
			frc_devp->clk_reg = NULL;
			frc_clk_base = NULL;
		} else {
			frc_devp->clk_reg = (void *)base;
			frc_clk_base = frc_devp->clk_reg;
		}
	} else {
		frc_devp->clk_reg = NULL;
		frc_clk_base = NULL;
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "vpu_reg");
	if (res) {
		base = devm_ioremap(&pdev->dev, res->start, res->end - res->start);
		if (!base) {
			PR_ERR("Unable to map vpu reg base\n");
			frc_devp->vpu_reg = NULL;
			vpu_base = NULL;
		} else {
			frc_devp->vpu_reg = (void *)base;
			vpu_base = frc_devp->vpu_reg;
		}
	} else {
		frc_devp->vpu_reg = NULL;
		vpu_base = NULL;
	}

	// frc buf reserved
	ret = of_reserved_mem_device_init_by_idx(&pdev->dev, np, 0);
	if (ret) {
		pr_frc(0, "cma resource undefined !\n");
		frc_devp->buf.cma_mem_size = 0;
	}
	frc_devp->buf.cma_mem_size = dma_get_cma_size_int_byte(&pdev->dev);

	frc_devp->clk_frc = clk_get(&pdev->dev, "clk_frc");
	frc_devp->clk_me = clk_get(&pdev->dev, "clk_me");
	if (IS_ERR(frc_devp->clk_me) || IS_ERR(frc_devp->clk_frc)) {
		pr_frc(0, "can't get frc clk !\n");
		frc_devp->clk_frc = NULL;
		frc_devp->clk_me = NULL;
	}
	frc_attach_pd(frc_devp);
	return ret;
}

#ifdef CONFIG_COMPAT
static long frc_campat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	arg = (unsigned long)compat_ptr(arg);
	return frc_ioctl(file, cmd, arg);
}
#endif

int frc_notify_callback(struct notifier_block *block, unsigned long cmd, void *para)
{
	struct frc_dev_s *devp = get_frc_devp();

	if (!devp)
		return -1;
	if (devp->clk_state == FRC_CLOCK_OFF)
		return 0;

	pr_frc(1, "%s cmd: 0x%lx\n", __func__, cmd);
	switch (cmd) {
	case VOUT_EVENT_MODE_CHANGE_PRE:
		/*if frc on, need disable frc, and enable frc*/
		// devp->frc_sts.out_put_mode_changed = FRC_EVENT_VOUT_CHG;
		//frc_change_to_state(FRC_STATE_DISABLE);
		break;

	case VOUT_EVENT_MODE_CHANGE:
		devp->frc_sts.out_put_mode_changed = FRC_EVENT_VOUT_CHG;
		//frc_change_to_state(FRC_STATE_DISABLE);
		break;

	default:
		break;
	}
	return 0;
}

int frc_vd_notify_callback(struct notifier_block *block, unsigned long cmd, void *para)
{
	struct frc_dev_s *devp = get_frc_devp();
	struct vd_info_s *info;
	enum chip_id chip = get_chip_type();
	u32 flags;

	if (!devp)
		return -1;
	if (devp->clk_state == FRC_CLOCK_OFF)
		return -1;
	if (chip == ID_T3X)
		return 0;

	info = (struct vd_info_s *)para;
	flags = info->flags;

	pr_frc(3, "%s cmd: 0x%lx flags:0x%x\n", __func__, cmd, flags);
	switch (cmd) {
	case VIDEO_INFO_CHANGED:
		/*if frc on, need disable frc, and enable frc*/
		if (((flags & VIDEO_SIZE_CHANGE_EVENT)
			== VIDEO_SIZE_CHANGE_EVENT) &&
			devp->probe_ok && (!devp->in_sts.frc_seamless_en ||
			(devp->in_sts.frc_seamless_en && devp->in_sts.frc_is_tvin))) {
				pr_frc(0, "%s start disable frc", __func__);
				set_frc_enable(false);
				set_frc_bypass(true);
				// frc_change_to_state(FRC_STATE_DISABLE);
				frc_change_to_state(FRC_STATE_BYPASS);
				frc_state_change_finish(devp);
			if (devp->frc_sts.frame_cnt != 0) {
				devp->frc_sts.frame_cnt = 0;
				pr_frc(1, "%s reset frm_cnt\n", __func__);
			}
			pr_frc(1, "%s VIDEO_SIZE_CHANGE_EVENT\n",  __func__);
			devp->frc_sts.out_put_mode_changed = FRC_EVENT_VF_CHG_IN_SIZE;
		}
		break;

	default:
		break;
	}

	return 0;
}

static struct notifier_block frc_notifier_nb = {
	.notifier_call	= frc_notify_callback,
};

static struct notifier_block frc_notifier_vb = {
	.notifier_call	= frc_vd_notify_callback,
};

static const struct file_operations frc_fops = {
	.owner = THIS_MODULE,
	.open = frc_open,
	.release = frc_release,
	.unlocked_ioctl = frc_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = frc_campat_ioctl,
#endif
};

static int runtimepm_frc_open(struct inode *inode, struct file *file)
{
	pm_runtime_get_sync(&runtime_frc_dev->dev);
	return 0;
}

static int runtimepm_frc_release(struct inode *inode, struct file *file)
{
	pm_runtime_put_sync(&runtime_frc_dev->dev);
	return 0;
}

static const struct file_operations runtimepm_frc_fops = {
	.owner = THIS_MODULE,
	.open = runtimepm_frc_open,
	.release = runtimepm_frc_release,
};

static struct miscdevice frc_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "runtime_frc",
	.fops = &runtimepm_frc_fops,
};

/****************************************************************************/
#if IS_ENABLED(CONFIG_AMLOGIC_DMC_DEV_ACCESS)

static int frc0_dmc_dev_access_notify(struct notifier_block *nb, unsigned long id, void *data)
{
	struct dmc_dev_access_data *dmc = (struct dmc_dev_access_data *)data;
	struct frc_dev_s *devp = get_frc_devp();

	if (devp->dmc_cfg[0].id == id) {
		devp->dmc_cfg[0].ddr_addr = dmc->addr;
		devp->dmc_cfg[0].ddr_size = dmc->size;
		pr_frc(0, "%s:id(%d)-trust handle addr:0x%lX,size:0x%lX\n",
				__func__, (int)id, dmc->addr, dmc->size);
	}
	return 0;
}

static int frc1_dmc_dev_access_notify(struct notifier_block *nb, unsigned long id, void *data)
{
	struct dmc_dev_access_data *dmc = (struct dmc_dev_access_data *)data;
	struct frc_dev_s *devp = get_frc_devp();

	if (devp->dmc_cfg[1].id == id) {
		devp->dmc_cfg[1].ddr_addr = dmc->addr;
		devp->dmc_cfg[1].ddr_size = dmc->size;
		pr_frc(0, "%s:id(%d)--trust handle addr:0x%lX,size:0x%lX\n",
				__func__, (int)id, dmc->addr, dmc->size);
	}
	return 0;
}

static int frc2_dmc_dev_access_notify(struct notifier_block *nb, unsigned long id, void *data)
{
	struct dmc_dev_access_data *dmc = (struct dmc_dev_access_data *)data;
	struct frc_dev_s *devp = get_frc_devp();

	if (devp->dmc_cfg[2].id == id) {
		devp->dmc_cfg[2].ddr_addr = dmc->addr;
		devp->dmc_cfg[2].ddr_size = dmc->size;
		pr_frc(0, "%s:id(%d)-trust handle addr:0x%lX,size:0x%lX\n",
				__func__, (int)id, dmc->addr, dmc->size);
	}
	return 0;
}

static struct notifier_block frc0_dmc_dev_access_nb = {
	.notifier_call = frc0_dmc_dev_access_notify,
};

static struct notifier_block frc1_dmc_dev_access_nb = {
	.notifier_call = frc1_dmc_dev_access_notify,
};

static struct notifier_block frc2_dmc_dev_access_nb = {
	.notifier_call = frc2_dmc_dev_access_notify,
};

void frc_dmc_notifier(void)
{
	struct frc_dev_s *devp = get_frc_devp();

	if (unlikely(!devp)) {
		PR_ERR("%s:devp is NULL\n", __func__);
		return;
	}
	devp->dmc_cfg[0].id = register_dmc_dev_access_notifier("FRC0",
				&frc0_dmc_dev_access_nb);
	devp->dmc_cfg[1].id = register_dmc_dev_access_notifier("FRC1",
				&frc1_dmc_dev_access_nb);
	devp->dmc_cfg[2].id = register_dmc_dev_access_notifier("FRC2",
				&frc2_dmc_dev_access_nb);
}

//unregister notifier
void frc_dmc_un_notifier(void)
{
	struct frc_dev_s *devp = get_frc_devp();

	if (unlikely(!devp)) {
		PR_ERR("%s:devp is NULL\n", __func__);
		return;
	}

	devp->dmc_cfg[0].id = unregister_dmc_dev_access_notifier("FRC0",
			&frc0_dmc_dev_access_nb);
	devp->dmc_cfg[0].id = unregister_dmc_dev_access_notifier("FRC1",
			&frc1_dmc_dev_access_nb);
	devp->dmc_cfg[0].id = unregister_dmc_dev_access_notifier("FRC2",
			&frc2_dmc_dev_access_nb);
}
#endif
/****************************************************************************/
static void frc_clock_workaround(struct work_struct *work)
{
	struct frc_dev_s *devp = container_of(work,
		struct frc_dev_s, frc_clk_work);
	int default_me_clk, default_mc_clk;
	int default_min_me_clk, default_min_mc_clk;

	if (unlikely(!devp)) {
		PR_ERR("%s err, devp is NULL\n", __func__);
		return;
	}
	if (!devp->probe_ok)
		return;
	if (!devp->power_on_flag)
		return;

	if (get_chip_type() == ID_T3X) {
		default_min_me_clk = FRC_CLOCK_RATE_333;
		default_min_mc_clk = FRC_CLOCK_RATE_333;
		default_me_clk = FRC_CLOCK_RATE_667;
		default_mc_clk = FRC_CLOCK_RATE_667;
	} else {
		default_min_me_clk = FRC_CLOCK_RATE_333;
		default_min_mc_clk = FRC_CLOCK_RATE_333;
		default_me_clk = FRC_CLOCK_RATE_333;
		default_mc_clk = FRC_CLOCK_RATE_667;
	}

	if (devp->clk_chg == FRC_CLOCK_FIXED) {
		clk_set_rate(devp->clk_me, default_me_clk);
		clk_set_rate(devp->clk_frc, default_mc_clk);
		if (devp->clk_state != FRC_CLOCK_OFF)
			set_frc_clk_disable(devp, 1);
		if (devp->clk_state != FRC_CLOCK_NOR)
			set_frc_clk_disable(devp, 0);
	} else if (devp->clk_chg == FRC_CLOCK_DYNAMIC_0) {
		if (devp->clk_state == FRC_CLOCK_XXX2NOR)
			devp->clk_state = FRC_CLOCK_OFF2NOR;
		else if (devp->clk_state == FRC_CLOCK_NOR2XXX)
			devp->clk_state = FRC_CLOCK_NOR2OFF;
	} else if (devp->clk_chg == FRC_CLOCK_DYNAMIC_1) {
		if (devp->clk_state == FRC_CLOCK_XXX2NOR)
			devp->clk_state = FRC_CLOCK_MIN2NOR;
		else if (devp->clk_state == FRC_CLOCK_NOR2XXX)
			devp->clk_state = FRC_CLOCK_NOR2MIN;
	}

	if (devp->clk_state == FRC_CLOCK_NOR2OFF) {
		set_frc_clk_disable(devp, 1);
	} else if (devp->clk_state == FRC_CLOCK_OFF2NOR) {
		set_frc_clk_disable(devp, 0);
	} else if (devp->clk_state == FRC_CLOCK_NOR2MIN) {
		clk_set_rate(devp->clk_me, default_min_me_clk);
		clk_set_rate(devp->clk_frc, default_min_mc_clk);
		devp->clk_state = FRC_CLOCK_MIN;
	} else if (devp->clk_state == FRC_CLOCK_MIN2NOR) {
		clk_set_rate(devp->clk_me, default_me_clk);
		clk_set_rate(devp->clk_frc, default_mc_clk);
		devp->clk_state = FRC_CLOCK_NOR;
	} else if (devp->clk_state == FRC_CLOCK_MIN2OFF) {
		set_frc_clk_disable(devp, 1);
		devp->clk_state = FRC_CLOCK_OFF;
	} else if (devp->clk_state == FRC_CLOCK_OFF2MIN) {
		clk_set_rate(devp->clk_me, default_min_me_clk);
		clk_set_rate(devp->clk_frc, default_min_mc_clk);
		set_frc_clk_disable(devp, 0);
		devp->clk_state = FRC_CLOCK_MIN;
	}
	pr_frc(1, "%s, clk_new state:%d\n", __func__, devp->clk_state);
}

static void frc_secure_workaround(struct work_struct *work)
{
	struct frc_dev_s *devp = container_of(work,
		struct frc_dev_s, frc_secure_work);

	if (unlikely(!devp)) {
		PR_ERR("%s err, devp is NULL\n", __func__);
		return;
	}

	frc_mm_secure_set(devp);
}

static void frc_drv_initial(struct frc_dev_s *devp)
{
	struct vinfo_s *vinfo = get_current_vinfo();
	struct frc_fw_data_s *fw_data;
	u32 i;

	if (!devp)
		return;

	devp->frc_sts.state = FRC_STATE_BYPASS;
	devp->frc_sts.new_state = FRC_STATE_BYPASS;

	/*0:before postblend; 1:after postblend*/
	//devp->frc_hw_pos = FRC_POS_AFTER_POSTBLEND;/*for test*/
	devp->frc_sts.auto_ctrl = 0;
	devp->frc_fw_pause = false;
	// devp->frc_fw_pause = true;
	devp->frc_sts.frame_cnt = 0;
	devp->frc_sts.changed_flag = 0;
	devp->frc_sts.state_transing = false;
	devp->frc_sts.re_cfg_cnt = 0;
	devp->frc_sts.out_put_mode_changed = false;
	devp->frc_sts.re_config = false;
	devp->dbg_force_en = 0;
	devp->auto_n2m = 1;
	devp->other1_flag = 0;
	devp->other2_flag = 0;  // 25, 16;
	devp->vlock_flag = 1;
	devp->dbg_mvrd_mode = 8;
	devp->dbg_mute_disable = 1;
	devp->test2 = 1;
	/*input sts initial*/
	devp->in_sts.have_vf_cnt = 0;
	devp->in_sts.no_vf_cnt = 0;
	devp->in_sts.vf_sts = 0;/*initial to no*/

	devp->dbg_in_out_ratio = FRC_RATIO_1_1;
	// devp->dbg_in_out_ratio = FRC_RATIO_2_5;
	// devp->dbg_in_out_ratio = FRC_RATIO_1_2;
	devp->dbg_input_hsize = vinfo->width;
	devp->dbg_input_vsize = vinfo->height;
	devp->dbg_reg_monitor_i = 0;
	devp->dbg_reg_monitor_o = 0;
	devp->dbg_vf_monitor = 0;
	for (i = 0; i < MONITOR_REG_MAX; i++) {
		devp->dbg_in_reg[i] = 0;
		devp->dbg_out_reg[i] = 0;
	}
	devp->dbg_buf_len = 0;
	devp->prot_mode = true;
	devp->use_pre_vsync = PRE_VSYNC_120HZ;

	devp->in_out_ratio = FRC_RATIO_1_1;
	// devp->in_out_ratio = FRC_RATIO_2_5;
	// devp->in_out_ratio = FRC_RATIO_1_2;
	// devp->film_mode = EN_DRV_FILM32;
	devp->film_mode = EN_DRV_VIDEO;
	devp->film_mode_det = 0;

	devp->pat_dbg.pat_en = 1;

	// ctrl high-priority tasklet
	devp->in_sts.hi_en = 0;
	devp->out_sts.hi_en = 0;

	fw_data = (struct frc_fw_data_s *)devp->fw_data;
	fw_data->holdline_parm.me_hold_line = 4;
	fw_data->holdline_parm.mc_hold_line = 1;
	fw_data->holdline_parm.inp_hold_line = 4;
	fw_data->holdline_parm.reg_post_dly_vofst = 0;/*fixed*/
	fw_data->holdline_parm.reg_mc_dly_vofst0 = 1;/*fixed*/

	if (fw_data->frc_top_type.chip != 0)
		memcpy(&devp->frm_dly_set[0],
			&chip_frc_frame_dly[fw_data->frc_top_type.chip - 1][0],
			sizeof(struct frm_dly_dat_s) * 4);
	else
		memcpy(&devp->frm_dly_set[0],
			&chip_frc_frame_dly[0][0],
			sizeof(struct frm_dly_dat_s) * 4);
	pr_frc(0, "frc_get_dly:%d,%d, %d,%d, %d,%d, %d,%d\n",
					devp->frm_dly_set[0].mevp_frm_dly,
					devp->frm_dly_set[0].mc_frm_dly,
				devp->frm_dly_set[1].mevp_frm_dly,
				devp->frm_dly_set[1].mc_frm_dly,
				devp->frm_dly_set[2].mevp_frm_dly,
				devp->frm_dly_set[2].mc_frm_dly,
				devp->frm_dly_set[3].mevp_frm_dly,
				devp->frm_dly_set[3].mc_frm_dly);

	memset(&devp->frc_crc_data, 0, sizeof(struct frc_crc_data_s));
	memset(&devp->ud_dbg, 0, sizeof(struct frc_ud_s));
	/*used for force in/out size for frc process*/
	memset(&devp->force_size, 0, sizeof(struct frc_force_size_s));
	devp->ud_dbg.res2_dbg_en = 3;  // t3x_revB test
	devp->ud_dbg.align_dbg_en = 0;  // t3x_revB test
	if (get_chip_type() == ID_T3X) {
		devp->in_sts.boot_timestamp_en = 1;
		devp->vpu_byp_frc_reg_addr = VIU_FRC_MISC;
	} else if (get_chip_type() == ID_T5M) {
		devp->vpu_byp_frc_reg_addr = VPU_FRC_TOP_CTRL;
	} else if (get_chip_type() == ID_T3) {
		devp->vpu_byp_frc_reg_addr = VPU_FRC_TOP_CTRL;
	} else {
		devp->vpu_byp_frc_reg_addr = VPU_FRC_TOP_CTRL;
	}
}

void get_vout_info(struct frc_dev_s *frc_devp)
{
	struct vinfo_s *vinfo = get_current_vinfo();
	struct frc_fw_data_s *pfw_data;
	u16  tmpframterate = 0;

	if (!frc_devp) {
		PR_ERR("%s: frc_devp is null\n", __func__);
		return;
	}

	pfw_data = (struct frc_fw_data_s *)frc_devp->fw_data;
	if (frc_devp->out_sts.vout_height != vinfo->height)
		frc_devp->out_sts.vout_height = vinfo->height;
	if (frc_devp->out_sts.vout_width != vinfo->width)
		frc_devp->out_sts.vout_width = vinfo->width;
	tmpframterate =
	(vinfo->sync_duration_num * 100 / vinfo->sync_duration_den) / 100;
	if (frc_devp->out_sts.out_framerate != tmpframterate) {
		frc_devp->out_sts.out_framerate = tmpframterate;
		pfw_data->frc_top_type.frc_out_frm_rate =
			frc_devp->out_sts.out_framerate;
		pfw_data->frc_top_type.frc_other_reserved =
			frc_devp->out_sts.out_framerate;
		if (frc_devp->auto_n2m == 1) {
			if (frc_devp->out_sts.out_framerate > 90) {
				frc_set_n2m(FRC_RATIO_1_2);
				if ((frc_devp->use_pre_vsync & PRE_VSYNC_120HZ) ==
					PRE_VSYNC_120HZ) {
					set_vsync_2to1_mode(0);
					set_pre_vsync_mode(1);
				} else {
					set_vsync_2to1_mode(1);
					set_pre_vsync_mode(0);
				}
			} else if (frc_devp->out_sts.out_framerate < 70) {
				if ((frc_devp->use_pre_vsync & PRE_VSYNC_060HZ) ==
					PRE_VSYNC_060HZ) {
					frc_set_n2m(FRC_RATIO_1_2);
					set_vsync_2to1_mode(0);
					set_pre_vsync_mode(1);
				} else {
					frc_set_n2m(FRC_RATIO_1_1);
					set_vsync_2to1_mode(0);
					set_pre_vsync_mode(0);
				}

			}
		}
		pr_frc(1, "vout:w-%d,h-%d,rate-%d\n",
				frc_devp->out_sts.vout_width,
				frc_devp->out_sts.vout_height,
				frc_devp->out_sts.out_framerate);

	}
}

int frc_buf_set(struct frc_dev_s *frc_devp)
{
	if (!frc_devp) {
		PR_ERR("%s: frc_devp is null\n", __func__);
		return -1;
	}
	if (frc_buf_calculate(frc_devp) != 0)
		return -1;
	if (frc_buf_alloc(frc_devp) != 0)
		return -1;
	if (frc_buf_distribute(frc_devp) != 0)
		return -1;
	if (frc_buf_config(frc_devp) != 0)
		return -1;
	else
		return 0;
}

static int frc_probe(struct platform_device *pdev)
{
	int ret = 0, i;
	struct frc_data_s *frc_data;
	struct frc_dev_s *frc_devp = &frc_dev;
	// frc_dev = kzalloc(sizeof(*frc_dev), GFP_KERNEL);
	// if (!frc_dev) {
	//	PR_ERR("%s: frc_dev kzalloc memory failed\n", __func__);
	//	goto fail_alloc_dev;
	// }
	// pr_frc(0, "%s, frc probe start\n", __func__);
	memset(frc_devp, 0, (sizeof(struct frc_dev_s)));

	frc_devp->data = NULL;
	frc_devp->data = kzalloc(sizeof(*frc_devp->data), GFP_KERNEL);
	if (!frc_devp->data) {
		PR_ERR("%s: frc_dev->data fail\n", __func__);
		goto fail_alloc_data_fail;
	}

	// frc_devp->fw_data = NULL;
	// frc_devp->fw_data = kzalloc(sizeof(struct frc_fw_data_s), GFP_KERNEL);
	frc_devp->fw_data = &fw_data;
	memset(frc_devp->fw_data, 0, (sizeof(struct frc_fw_data_s)));
	strncpy(&fw_data.frc_alg_ver[0], &frc_alg_def_ver[0],
			strlen(frc_alg_def_ver));
	if (!frc_devp->fw_data) {
		PR_ERR("%s: frc_dev->fw_data fail\n", __func__);
		goto fail_alloc_fw_data_fail;
	}
	// PR_FRC("%s fw_data st size:%d", __func__, sizeof_frc_fw_data_struct());

	ret = alloc_chrdev_region(&frc_devp->devno, 0, FRC_DEVNO, FRC_NAME);
	if (ret < 0) {
		PR_ERR("%s: alloc region fail\n", __func__);
		goto fail_alloc_region;
	}
	frc_devp->clsp = class_create(THIS_MODULE, FRC_CLASS_NAME);
	if (IS_ERR(frc_devp->clsp)) {
		ret = PTR_ERR(frc_devp->clsp);
		PR_ERR("%s: create class fail\n", __func__);
		goto fail_create_cls;
	}

	for (i = 0; frc_class_attrs[i].attr.name; i++) {
		if (class_create_file(frc_devp->clsp, &frc_class_attrs[i]) < 0)
			goto fail_class_create_file;
	}
	// get_vout_info(frc_devp);

	cdev_init(&frc_devp->cdev, &frc_fops);
	frc_devp->cdev.owner = THIS_MODULE;
	ret = cdev_add(&frc_devp->cdev, frc_devp->devno, FRC_DEVNO);
	if (ret)
		goto fail_add_cdev;

	frc_devp->dev = device_create(frc_devp->clsp, NULL,
		frc_devp->devno, frc_devp, FRC_NAME);
	if (IS_ERR(frc_devp->dev)) {
		PR_ERR("%s: device create fail\n", __func__);
		goto fail_dev_create;
	}

	dev_set_drvdata(frc_devp->dev, frc_devp);
	platform_set_drvdata(pdev, frc_devp);
	frc_devp->pdev = pdev;

	frc_data = (struct frc_data_s *)frc_devp->data;
	// fw_data = (struct frc_fw_data_s *)frc_devp->fw_data;
	frc_dts_parse(frc_devp);
	// if (ret < 0)  // fixed CID 139501
	//	goto fail_dev_create;
	tasklet_init(&frc_devp->input_tasklet, frc_input_tasklet_pro, (unsigned long)frc_devp);
	tasklet_init(&frc_devp->output_tasklet, frc_output_tasklet_pro, (unsigned long)frc_devp);
	/*register a notify*/
	vout_register_client(&frc_notifier_nb);
	vd_signal_register_client(&frc_notifier_vb);

	/*driver internal data initial*/
	frc_drv_initial(frc_devp);
	frc_clk_init(frc_devp);
	get_vout_info(frc_devp);
	frc_devp->power_on_flag = true;
	pm_runtime_enable(&pdev->dev);
	ret = pm_runtime_get_sync(&pdev->dev);
	if (ret < 0) {
		PR_ERR("pm_runtime_get_sync error\n");
		goto fail_dev_create;
	}
	frc_init_config(frc_devp);

	/*buffer config*/
	//frc_buf_calculate(frc_devp);
	//frc_buf_alloc(frc_devp);
	//frc_buf_distribute(frc_devp);
	//frc_buf_config(frc_devp);
	if (frc_buf_set(frc_devp) != 0)
		goto fail_dev_create;

	frc_internal_initial(frc_devp);
	frc_devp->out_line = frc_init_out_line();
	frc_hw_initial(frc_devp);
	/*enable irq*/
	if (frc_devp->in_irq > 0)
		enable_irq(frc_devp->in_irq);
	if (frc_devp->out_irq > 0)
		enable_irq(frc_devp->out_irq);
	// if (frc_devp->axi_crash_irq > 0)
	//	enable_irq(frc_devp->axi_crash_irq);

// #ifdef CONFIG_AMLOGIC_MEDIA_FRC_RDMA
	if (frc_devp->rdma_irq > 0)
		enable_irq(frc_devp->rdma_irq);
	if (!frc_rdma_init())
		PR_FRC("%s frc rdma init failed\n", __func__);
// #endif
	INIT_WORK(&frc_devp->frc_clk_work, frc_clock_workaround);
	INIT_WORK(&frc_devp->frc_print_work, frc_debug_table_print);
	INIT_WORK(&frc_devp->frc_secure_work, frc_secure_workaround);
	frc_devp->clk_chg = FRC_CLOCK_DYNAMIC_0;
	frc_set_enter_forcefilm(frc_devp, 0);

	frc_devp->probe_ok = true;
	frc_devp->power_off_flag = false;
	ret = misc_register(&frc_misc);
	runtime_frc_dev = pdev;
	pm_runtime_forbid(&pdev->dev);

#if IS_ENABLED(CONFIG_AMLOGIC_DMC_DEV_ACCESS)
	frc_dmc_notifier();
#endif
//	PR_FRC("%s probe st:%d", __func__, frc_devp->probe_ok);
	return ret;
fail_dev_create:
	cdev_del(&frc_devp->cdev);
fail_add_cdev:
	PR_ERR("%s: cdev add fail\n", __func__);
fail_class_create_file:
	for (i = 0; frc_class_attrs[i].attr.name; i++)
		class_remove_file(frc_devp->clsp, &frc_class_attrs[i]);
	class_destroy(frc_devp->clsp);
fail_create_cls:
	unregister_chrdev_region(frc_devp->devno, FRC_DEVNO);
fail_alloc_region:
	// kfree(frc_dev->fw_data);
	frc_devp->fw_data = NULL;
fail_alloc_fw_data_fail:
	kfree(frc_devp->data);
	frc_devp->data = NULL;
fail_alloc_data_fail:
	// kfree(frc_dev);
	// frc_dev = NULL;
// fail_alloc_dev:
	return ret;
}

static int __exit frc_remove(struct platform_device *pdev)
{
	struct frc_dev_s *frc_devp = &frc_dev;

	if (!frc_devp || !frc_devp->probe_ok)
		return -1;

	PR_FRC("%s:module remove\n", __func__);
	// frc_devp = platform_get_drvdata(pdev);
	cancel_work_sync(&frc_devp->frc_clk_work);
	cancel_work_sync(&frc_devp->frc_print_work);
	cancel_work_sync(&frc_devp->frc_secure_work);
	tasklet_kill(&frc_devp->input_tasklet);
	tasklet_kill(&frc_devp->output_tasklet);
	tasklet_disable(&frc_devp->input_tasklet);
	tasklet_disable(&frc_devp->output_tasklet);
	if (frc_devp->in_irq > 0)
		free_irq(frc_devp->in_irq, (void *)frc_devp);
	if (frc_devp->out_irq > 0)
		free_irq(frc_devp->out_irq, (void *)frc_devp);
	if (frc_devp->axi_crash_irq > 0)
		free_irq(frc_devp->axi_crash_irq, (void *)frc_devp);
	if (frc_devp->rdma_irq > 0)
		free_irq(frc_devp->rdma_irq, (void *)frc_devp);

	device_destroy(frc_devp->clsp, frc_devp->devno);
	cdev_del(&frc_devp->cdev);
	class_destroy(frc_devp->clsp);
	unregister_chrdev_region(frc_devp->devno, FRC_DEVNO);
	set_frc_clk_disable(frc_devp, 1);
	frc_buf_release(frc_devp);
	kfree(frc_devp->data);
	frc_devp->data = NULL;
#if IS_ENABLED(CONFIG_AMLOGIC_DMC_DEV_ACCESS)
	frc_dmc_un_notifier();
#endif
	// kfree(frc_dev);
	// frc_dev = NULL;
	PR_FRC("%s:module remove done\n", __func__);
	return 0;
}

static void frc_shutdown(struct platform_device *pdev)
{
	struct frc_dev_s *frc_devp = &frc_dev;
	enum chip_id chip;

	if (!frc_devp || !frc_devp->probe_ok)
		return;
	PR_FRC("%s:module shutdown\n", __func__);
	chip = get_chip_type();
	frc_devp->power_on_flag = false;
	tasklet_kill(&frc_devp->input_tasklet);
	tasklet_kill(&frc_devp->output_tasklet);
	tasklet_disable(&frc_devp->input_tasklet);
	tasklet_disable(&frc_devp->output_tasklet);
	if (frc_devp->in_irq > 0)
		free_irq(frc_devp->in_irq, (void *)frc_devp);
	if (frc_devp->out_irq > 0)
		free_irq(frc_devp->out_irq, (void *)frc_devp);
	if (frc_devp->axi_crash_irq > 0)
		free_irq(frc_devp->axi_crash_irq, (void *)frc_devp);
	if (frc_devp->rdma_irq > 0)
		free_irq(frc_devp->rdma_irq, (void *)frc_devp);
	device_destroy(frc_devp->clsp, frc_devp->devno);
	cdev_del(&frc_devp->cdev);
	class_destroy(frc_devp->clsp);
	unregister_chrdev_region(frc_devp->devno, FRC_DEVNO);
	set_frc_clk_disable(frc_devp, 1);
	frc_buf_release(frc_devp);
	kfree(frc_devp->data);
	frc_devp->data = NULL;
	// kfree(frc_dev);
	// frc_dev = NULL;
#if IS_ENABLED(CONFIG_AMLOGIC_DMC_DEV_ACCESS)
	frc_dmc_un_notifier();
#endif
	if (chip == ID_T3) {
		pwr_ctrl_psci_smc(PDID_T3_FRCTOP, PWR_OFF);
		pwr_ctrl_psci_smc(PDID_T3_FRCME, PWR_OFF);
		pwr_ctrl_psci_smc(PDID_T3_FRCMC, PWR_OFF);
	} else if (chip == ID_T5M) {
		pwr_ctrl_psci_smc(PDID_T5M_FRC_TOP, PWR_OFF);
	} else if (chip == ID_T3X) {
		pwr_ctrl_psci_smc(PDID_T3X_FRC_TOP, PWR_OFF);
	}
	PR_FRC("%s:module shutdown done with powerdomain\n", __func__);

}

#if CONFIG_PM
static int frc_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct frc_dev_s *devp = NULL;

	devp = get_frc_devp();
	if (!devp || !devp->probe_ok)
		return -1;
	PR_FRC("%s ...\n", __func__);
	frc_power_domain_ctrl(devp, 0);
	if (devp->power_on_flag)
		devp->power_on_flag = false;

	return 0;
}

static int frc_resume(struct platform_device *pdev)
{
	struct frc_dev_s *devp = NULL;

	devp = get_frc_devp();
	if (!devp || !devp->probe_ok)
		return -1;
	PR_FRC("%s ...\n", __func__);
	frc_power_domain_ctrl(devp, 1);
	if (!devp->power_on_flag)
		devp->power_on_flag = true;

	return 0;
}

static int frc_pm_suspend(struct device *dev)
{
	struct frc_dev_s *devp = NULL;

	devp = get_frc_devp();
	if (!devp)
		return -1;
	PR_FRC("call %s ...\n", __func__);
	frc_power_domain_ctrl(devp, 0);
	if (devp->power_on_flag)
		devp->power_on_flag = false;

	return 0;
}

static int frc_pm_resume(struct device *dev)
{
	struct frc_dev_s *devp = NULL;

	devp = get_frc_devp();
	if (!devp)
		return -1;
	PR_FRC("call %s ...\n", __func__);
	frc_power_domain_ctrl(devp, 1);
	if (!devp->power_on_flag)
		devp->power_on_flag = true;
	set_frc_bypass(ON);
	devp->frc_sts.auto_ctrl = true;
	devp->frc_sts.re_config = true;

	return 0;
}

static int frc_runtime_suspend(struct device *dev)
{
	struct frc_dev_s *devp = NULL;

	devp = get_frc_devp();
	if (!devp)
		return -1;
	devp->frc_sts.auto_ctrl = false;
	PR_FRC("call %s\n", __func__);
	frc_power_domain_ctrl(devp, 0);
	if (devp->power_on_flag)
		devp->power_on_flag = false;

	return 0;
}

static int frc_runtime_resume(struct device *dev)
{
	struct frc_dev_s *devp = NULL;

	devp = get_frc_devp();
	if (!devp)
		return -1;
	// PR_FRC("call %s\n", __func__);
	frc_power_domain_ctrl(devp, 1);
	if (!devp->power_on_flag)
		devp->power_on_flag = true;
	set_frc_bypass(ON);
	devp->frc_sts.auto_ctrl = true;
	devp->frc_sts.re_config = true;

	return 0;
}
#endif

static const struct dev_pm_ops frc_dev_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(frc_pm_suspend, frc_pm_resume)
	SET_RUNTIME_PM_OPS(frc_runtime_suspend, frc_runtime_resume,
			NULL)
};

static struct platform_driver frc_driver = {
	.probe = frc_probe,
	.remove = frc_remove,
	.shutdown = frc_shutdown,
#ifdef CONFIG_PM
	.suspend = frc_suspend,
	.resume = frc_resume,
#endif
	.driver = {
		.name = "aml_frc",
		.owner = THIS_MODULE,
		.of_match_table = frc_dts_match,
		#ifdef CONFIG_PM
		.pm = &frc_dev_pm_ops,
		#endif
	},
};

int __init frc_init(void)
{
	PR_FRC("%s:module init\n", __func__);
	if (platform_driver_register(&frc_driver)) {
		PR_ERR("failed to register frc driver module\n");
		return -ENODEV;
	}
	hrtimer_init(&frc_hi_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	return 0;
}

void __exit frc_exit(void)
{
	platform_driver_unregister(&frc_driver);
	hrtimer_cancel(&frc_hi_timer);
	PR_FRC("%s:module exit\n", __func__);
}
