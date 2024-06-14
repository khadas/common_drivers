// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 *
 * Copyright (C) 2019 Amlogic, Inc. All rights reserved.
 *
 */

#include <linux/init.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/of.h>
#include <linux/reset.h>
#include <linux/clk.h>
#ifdef CONFIG_AMLOGIC_VPU
#include <linux/amlogic/media/vpu/vpu.h>
#endif
#include <linux/amlogic/media/vout/vinfo.h>
#include <linux/amlogic/media/vout/vout_notify.h>
#include <linux/amlogic/media/vout/lcd/lcd_vout.h>
#include <linux/amlogic/media/vout/lcd/lcd_unifykey.h>
#include <linux/amlogic/media/vout/lcd/lcd_notify.h>
#include <linux/amlogic/media/vout/lcd/aml_bl.h>
#include "lcd_tablet.h"
#include "../lcd_reg.h"
#include "../lcd_common.h"
#include <linux/sched/clock.h>

// in detailed timing
//unsigned char fixed_type;
//unsigned int fixed_val_set[8];

static struct lcd_duration_s lcd_std_fr[] = {
	{144, 144,    1,    0},
	{120, 120,    1,    0},
	{119, 120000, 1001, 1},
	{100, 100,    1,    0},
	{96,  96,     1,    0},
	{95,  96000,  1001, 1},
	{60,  60,     1,    0},
	{59,  60000,  1001, 1},
	{50,  50,     1,    0},
	{48,  48,     1,    0},
	{47,  48000,  1001, 1},
	{0,   0,      0,    0}
};

// * append to pdrv->vmode_mgr.vmode_list_header
static int lcd_vmode_add_single(struct aml_lcd_drv_s *pdrv, struct lcd_vmode_info_s *vmode_info)
{
	struct lcd_vmode_list_s *temp_list;
	struct lcd_vmode_list_s *cur_list;

	if (!vmode_info)
		return -1;

	/* create list */
	cur_list = kzalloc(sizeof(*cur_list), GFP_KERNEL);
	if (!cur_list)
		return -1;
	cur_list->info = vmode_info;

	if (!pdrv->vmode_mgr.vmode_list_header) {
		pdrv->vmode_mgr.vmode_list_header = cur_list;
	} else {
		temp_list = pdrv->vmode_mgr.vmode_list_header;
		while (temp_list->next)
			temp_list = temp_list->next;
		temp_list->next = cur_list;
	}
	pdrv->vmode_mgr.vmode_cnt += vmode_info->duration_cnt + 1;

	LCDPR("%s: name:%s, base_fr:%dhz, vmode_cnt: %d\n",
		__func__, cur_list->info->name, cur_list->info->base_fr,
		pdrv->vmode_mgr.vmode_cnt);

	return 0;
}

static void lcd_vmode_add_duration(struct lcd_vmode_info_s *vmode, struct lcd_detail_timing_s *dtm)
{
	unsigned int n = 0, i;//, fr, ht, vt;
	unsigned short fr_min, fr_max;

	fr_min = div_around(div_around(dtm->pclk_min, dtm->v_period_max), dtm->h_period_max);
	fr_max = div_around(div_around(dtm->pclk_max, dtm->v_period_min), dtm->h_period_min);

	memset(vmode->duration, 0, sizeof(struct lcd_duration_s) * LCD_DURATION_MAX);

	//if (dtm->fixed_type != 0xff) {
	//	for (i = 0; i < 8; i++) {
	//		if (dtm->fixed_val_set[i] == 0)
	//			break;
	//		if (n >= LCD_DURATION_MAX)
	//			break;
	//		fr = (dtm->fixed_type == 0) ? dtm->fixed_val_set[i] : dtm->pixel_clk;
	//		ht = (dtm->fixed_type == 1) ? dtm->fixed_val_set[i] : dtm->h_period;
	//		vt = (dtm->fixed_type == 2) ? dtm->fixed_val_set[i] : dtm->v_period;
	//		vmode->duration[n].frame_rate = (fr / ht) / vt;
	//		vmode->duration[n].duration_num = fr;
	//		vmode->duration[n].duration_den = ht * vt;
	//		vmode->duration[n].frac = 1;
	//		n++;
	//	}
	//}

	for (i = 0; i < ARRAY_SIZE(lcd_std_fr); i++) {
		if (dtm->fr_adjust_type == 0xff)
			break;
		if (lcd_std_fr[i].frame_rate == 0)
			break;
		if (lcd_std_fr[i].frame_rate > vmode->dft_timing->frame_rate_max)
			continue;
		if (lcd_std_fr[i].frame_rate < vmode->dft_timing->frame_rate_min)
			break;
		if (vmode->base_fr * lcd_std_fr[i].duration_den == lcd_std_fr[i].duration_num)
			continue;

		vmode->duration[n].frame_rate = lcd_std_fr[i].frame_rate;
		vmode->duration[n].duration_num = lcd_std_fr[i].duration_num;
		vmode->duration[n].duration_den = lcd_std_fr[i].duration_den;
		vmode->duration[n].frac = lcd_std_fr[i].frac;
		n++;
		if (n >= LCD_DURATION_MAX)
			break;
	}
	vmode->duration_cnt = n;
}

//--general mode: (h)x(v)p(frame_rate)hz
static struct lcd_vmode_info_s *lcd_detail_timing_to_vmode(struct lcd_detail_timing_s *ptiming)
{
	struct lcd_vmode_info_s *vmode_find = NULL;

	vmode_find = kzalloc(sizeof(*vmode_find), GFP_KERNEL);
	if (!vmode_find)
		return NULL;
	vmode_find->width = ptiming->h_active;
	vmode_find->height = ptiming->v_active;
	vmode_find->base_fr = ptiming->frame_rate;

	str_add_vmode(vmode_find->name, 0, vmode_find->width, vmode_find->height, 0);

	vmode_find->dft_timing = ptiming;
	lcd_vmode_add_duration(vmode_find, ptiming);

	return vmode_find;
}

static int lcd_vmode_remove_all(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_vmode_list_s *cur_list;
	struct lcd_vmode_list_s *next_list;

	cur_list = pdrv->vmode_mgr.vmode_list_header;
	while (cur_list) {
		next_list = cur_list->next;
		kfree(cur_list->info);
		kfree(cur_list);
		cur_list = next_list;
	}
	pdrv->vmode_mgr.vmode_list_header = NULL;
	pdrv->vmode_mgr.cur_vmode_info = NULL;
	pdrv->vmode_mgr.next_vmode_info = NULL;
	pdrv->vmode_mgr.vmode_cnt = 0;

	return 0;
}

//clear all vmode (lcd_vmode_remove_all)
//dft_timing to vmode (lcd_detail_timing_to_vmode(append duration)),
//	link vmode to bottom of vmode_mgr(lcd_vmode_add_single)
//cus_ctrl_timing to vmode (lcd_detail_timing_to_vmode(append duration))
static void lcd_tablet_add_all_vmode(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_vmode_info_s *vmode_find = NULL;
	struct lcd_detail_timing_s **timing_match;
	int i;

	if (!pdrv)
		return;

	lcd_vmode_remove_all(pdrv);

	//default ref timing
	vmode_find = lcd_detail_timing_to_vmode(&pdrv->config.timing.dft_timing);
	if (!vmode_find)
		return;
	lcd_vmode_add_single(pdrv, vmode_find);

	timing_match = lcd_cus_ctrl_timing_match_get(pdrv);
	if (timing_match) {
		for (i = 0; i < pdrv->config.cus_ctrl.timing_cnt; i++) {
			if (!timing_match[i])
				break;

			vmode_find = lcd_detail_timing_to_vmode(timing_match[i]);
			if (!vmode_find)
				continue;
			lcd_vmode_add_single(pdrv, vmode_find);
		}
		kfree(timing_match);
	}
}

// * act_timing to pdrv->vinfo (lcd_config_init)
static void lcd_act_timing_update_vinfo(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_detail_timing_s *ptiming = &pdrv->config.timing.act_timing;

	if (!pdrv)
		return;

	memset(pdrv->output_name, 0, sizeof(pdrv->output_name));

	str_add_vmode(pdrv->output_name, 0,
		ptiming->h_active, ptiming->v_active, ptiming->frame_rate);

	pdrv->vinfo.width = ptiming->h_active;
	pdrv->vinfo.height = ptiming->v_active;
	pdrv->vinfo.field_height = ptiming->v_active;
	pdrv->vinfo.aspect_ratio_num = pdrv->config.basic.screen_width;
	pdrv->vinfo.aspect_ratio_den =  pdrv->config.basic.screen_height;
	pdrv->vinfo.screen_real_width =  pdrv->config.basic.screen_width;
	pdrv->vinfo.screen_real_height =  pdrv->config.basic.screen_height;
	pdrv->vinfo.sync_duration_num = ptiming->sync_duration_num;
	pdrv->vinfo.sync_duration_den = ptiming->sync_duration_den;
	pdrv->vinfo.frac = ptiming->frac;
	pdrv->vinfo.std_duration = ptiming->frame_rate;
	pdrv->vinfo.vfreq_max = ptiming->frame_rate_max;
	pdrv->vinfo.vfreq_min = ptiming->frame_rate_min;
	pdrv->vinfo.video_clk =  pdrv->config.timing.enc_clk;
	pdrv->vinfo.htotal = ptiming->h_period;
	pdrv->vinfo.vtotal = ptiming->v_period;
	pdrv->vinfo.hsw = ptiming->hsync_width;
	pdrv->vinfo.hbp = ptiming->hsync_bp;
	pdrv->vinfo.hfp = ptiming->hsync_fp;
	pdrv->vinfo.vsw = ptiming->vsync_width;
	pdrv->vinfo.vbp = ptiming->vsync_bp;
	pdrv->vinfo.vfp = ptiming->vsync_fp;
	pdrv->vinfo.cur_enc_ppc =  pdrv->config.timing.ppc;
	switch (ptiming->fr_adjust_type) {
	case 0:
		pdrv->vinfo.fr_adj_type = VOUT_FR_ADJ_CLK;
		break;
	case 1:
		pdrv->vinfo.fr_adj_type = VOUT_FR_ADJ_HTOTAL;
		break;
	case 2:
		pdrv->vinfo.fr_adj_type = VOUT_FR_ADJ_VTOTAL;
		break;
	case 3:
		pdrv->vinfo.fr_adj_type = VOUT_FR_ADJ_COMBO;
		break;
	case 4:
		pdrv->vinfo.fr_adj_type = VOUT_FR_ADJ_HDMI;
		break;
	case 5:
		pdrv->vinfo.fr_adj_type = VOUT_FR_ADJ_FREERUN;
		break;
	default:
		pdrv->vinfo.fr_adj_type = VOUT_FR_ADJ_NONE;
		break;
	}

	lcd_optical_vinfo_update(pdrv);
}

// * dft_timing 2 pdrv->vinfo (lcd_tablet_vout_server_init)
static void lcd_dft_timing_update_vinfo(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_detail_timing_s *ptiming = &pdrv->config.timing.dft_timing;

	memset(pdrv->output_name, 0, sizeof(pdrv->output_name));

	str_add_vmode(pdrv->output_name, 0,
		ptiming->h_active, ptiming->v_active, ptiming->frame_rate);

	pdrv->vinfo.name = pdrv->output_name;
	pdrv->vinfo.mode = VMODE_LCD;
	pdrv->vinfo.width =  ptiming->h_active;
	pdrv->vinfo.height = ptiming->v_active;
	pdrv->vinfo.field_height = ptiming->v_active;
	pdrv->vinfo.aspect_ratio_num =  ptiming->h_active;
	pdrv->vinfo.aspect_ratio_den = ptiming->v_active;
	pdrv->vinfo.screen_real_width =  ptiming->h_active;
	pdrv->vinfo.screen_real_height = ptiming->v_active;
	pdrv->vinfo.sync_duration_num = ptiming->frame_rate;
	pdrv->vinfo.sync_duration_den = 1;
	pdrv->vinfo.frac = 0;
	pdrv->vinfo.std_duration = ptiming->frame_rate;
	pdrv->vinfo.vfreq_max = ptiming->frame_rate;
	pdrv->vinfo.vfreq_min = ptiming->frame_rate;
	pdrv->vinfo.video_clk = 0;
	pdrv->vinfo.htotal = ptiming->h_period;
	pdrv->vinfo.vtotal = ptiming->v_period;
	pdrv->vinfo.cur_enc_ppc = pdrv->config.timing.ppc;
	pdrv->vinfo.fr_adj_type = VOUT_FR_ADJ_NONE;
}

static int lcd_tablet_outputmode_check(struct aml_lcd_drv_s *pdrv, char *mode)
{
	struct lcd_vmode_list_s *temp_list = pdrv->vmode_mgr.vmode_list_header;
	char mode_name[48];
	char lagecy_name[8] = "panel\0\0";
	int i;

	if (!mode || !temp_list)
		return -1;

	if (pdrv->index)
		lagecy_name[5] = '0' + pdrv->index;
	if (strcmp(mode, lagecy_name) == 0) {
		temp_list->info->duration_index = 0;
		if (pdrv->vmode_mgr.cur_vmode_info != temp_list->info)
			pdrv->vmode_mgr.next_vmode_info = temp_list->info;
		LCDPR("[%d]: %s: legacy mode %s\n", pdrv->index, __func__, mode);
		return 0;
	}

	while (temp_list) {
		memset(mode_name, 0, 48);
		str_add_vmode(mode_name, 0,
			temp_list->info->width, temp_list->info->height, temp_list->info->base_fr);
		if (strcmp(mode, mode_name) == 0) {
			temp_list->info->duration_index = 0xff;
			if (pdrv->vmode_mgr.cur_vmode_info != temp_list->info)
				pdrv->vmode_mgr.next_vmode_info = temp_list->info;

			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
				LCDPR("[%d]: %s: match %s, %dx%d@%dhz\n",
					pdrv->index, __func__, temp_list->info->name,
					temp_list->info->width, temp_list->info->height,
					temp_list->info->base_fr);
			}
			return 0;
		}

		for (i = 0; i < LCD_DURATION_MAX; i++) {
			if (temp_list->info->duration[i].frame_rate == 0)
				break;
			memset(mode_name, 0, 48);
			str_add_vmode(mode_name, 0, temp_list->info->width,
				temp_list->info->height, temp_list->info->duration[i].frame_rate);

			if (strcmp(mode, mode_name))
				continue;

			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
				LCDPR("[%d]: %s: match %s, %dx%d@%dhz\n",
					pdrv->index, __func__, temp_list->info->name,
					temp_list->info->width, temp_list->info->height,
					temp_list->info->duration[i].frame_rate);
			}
			temp_list->info->duration_index = i;
			if (pdrv->vmode_mgr.cur_vmode_info != temp_list->info)
				pdrv->vmode_mgr.next_vmode_info = temp_list->info;
			return 0;
		}
		temp_list = temp_list->next;
	}

	LCDERR("[%d]: %s: invalid mode: %s\n", pdrv->index, __func__, mode);
	return -1;
}

// * update pdrv->output_name, *cur_vmode_info, act_timing==>vinfo
static void lcd_vmode_vinfo_update(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_vmode_info_s *info;
	unsigned short frame_rate;

	if (!pdrv)
		return;

	info = pdrv->vmode_mgr.cur_vmode_info;
	if (!info)
		return;

	memset(pdrv->output_name, 0, sizeof(pdrv->output_name));

	frame_rate = info->duration_index == 0xff ?
		info->base_fr : info->duration[info->duration_index].frame_rate;

	str_add_vmode(pdrv->output_name, 0, info->width, info->height, frame_rate);

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s: outputmode = %s\n", pdrv->index, __func__, pdrv->output_name);

	lcd_act_timing_update_vinfo(pdrv);
	lcd_optical_vinfo_update(pdrv);
}

/* **************************************************
 * vout server api
 * **************************************************
 */
static struct vinfo_s *lcd_get_current_info(void *data)
{
	struct aml_lcd_drv_s *pdrv = (struct aml_lcd_drv_s *)data;

	if (!pdrv)
		return NULL;
	return &pdrv->vinfo;
}

static int lcd_check_same_vmodeattr(char *mode, void *data)
{
	return 1;
}

static int lcd_vmode_is_supported(enum vmode_e mode, void *data)
{
	mode &= VMODE_MODE_BIT_MASK;
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("%s vmode = %d\n", __func__, mode);

	if (mode == VMODE_LCD)
		return true;
	return false;
}

static void lcd_vmode_update(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_detail_timing_s *ptiming;
	unsigned int pre_pclk;
	int dur_index;

	/* clear clk_change flag */
	pdrv->config.timing.clk_change &= ~(LCD_CLK_PLL_RESET);
	if (pdrv->vmode_mgr.next_vmode_info) {
		pre_pclk = pdrv->config.timing.base_timing.pixel_clk;
		pdrv->vmode_mgr.cur_vmode_info = pdrv->vmode_mgr.next_vmode_info;
		pdrv->vmode_mgr.next_vmode_info = NULL;

		pdrv->std_duration = pdrv->vmode_mgr.cur_vmode_info->duration;
		ptiming = pdrv->vmode_mgr.cur_vmode_info->dft_timing;
		memcpy(&pdrv->config.timing.base_timing, ptiming,
			sizeof(struct lcd_detail_timing_s));

		//update base_timing to act_timing
		lcd_enc_timing_init_config(pdrv);

		lcd_cus_ctrl_config_update(pdrv, (void *)ptiming, LCD_CUS_CTRL_SEL_TIMMING);
		if (pdrv->config.timing.base_timing.pixel_clk != pre_pclk) {
			pdrv->config.timing.clk_change |= LCD_CLK_PLL_RESET;
			lcd_clk_generate_parameter(pdrv);
		}

		pdrv->vmode_switch = 1; // more than (vfp, hfp, clk) changed
	}

	if (!pdrv->vmode_mgr.cur_vmode_info || !pdrv->std_duration) {
		LCDERR("[%d]: %s: cur_vmode_info or std_duration is null\n", pdrv->index, __func__);
		return;
	}

	dur_index = pdrv->vmode_mgr.cur_vmode_info->duration_index;

	if (dur_index != 0xff) {
		pdrv->config.timing.act_timing.sync_duration_num =
			pdrv->std_duration[dur_index].duration_num;
		pdrv->config.timing.act_timing.sync_duration_den =
			pdrv->std_duration[dur_index].duration_den;
		pdrv->config.timing.act_timing.frac =
			pdrv->std_duration[dur_index].frac;
		pdrv->config.timing.act_timing.frame_rate =
			pdrv->std_duration[dur_index].frame_rate;
	} else {
		pdrv->config.timing.act_timing.sync_duration_num =
			pdrv->vmode_mgr.cur_vmode_info->base_fr;
		pdrv->config.timing.act_timing.sync_duration_den = 1;
		pdrv->config.timing.act_timing.frac = 0;
		pdrv->config.timing.act_timing.frame_rate =
			pdrv->vmode_mgr.cur_vmode_info->base_fr;
	}
	lcd_frame_rate_change(pdrv);

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("[%d]: %s: %dx%d, duration=%d:%d, dur_index=%d, clk_change=0x%x\n",
			pdrv->index, __func__, pdrv->config.timing.act_timing.h_active,
			pdrv->config.timing.act_timing.v_active,
			pdrv->config.timing.act_timing.sync_duration_num,
			pdrv->config.timing.act_timing.sync_duration_den,
			dur_index, pdrv->config.timing.clk_change);
	}
}

static inline void lcd_vmode_switch(struct aml_lcd_drv_s *pdrv)
{
	unsigned long flags = 0;
	unsigned long long local_time[3];
	int ret;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("%s: timing_switch_flag: %d, lcd_status: 0x%x\n",
			__func__, pdrv->config.cus_ctrl.timing_switch_flag,
			pdrv->status);
	}
	local_time[0] = sched_clock();

	if (pdrv->config.cus_ctrl.timing_switch_flag == LCD_VMODE_SWITCH_MIN) {
		//step 1: switch off
		if (pdrv->status & LCD_STATUS_POWER) {
			//mute
			lcd_screen_black(pdrv);
			reinit_completion(&pdrv->vsync_done);
			spin_lock_irqsave(&pdrv->isr_lock, flags);
			if (pdrv->mute_count_test)
				pdrv->mute_count = pdrv->mute_count_test;
			else
				pdrv->mute_count = 3;
			pdrv->mute_flag = 1;
			spin_unlock_irqrestore(&pdrv->isr_lock, flags);
			//wait for mute apply
			ret = wait_for_completion_timeout(&pdrv->vsync_done, msecs_to_jiffies(500));
			if (!ret)
				LCDPR("wait_for_completion_timeout\n");
			if (pdrv->config.basic.lcd_type == LCD_P2P)
				lcd_tcon_reload_pre(pdrv);
		}
		local_time[1] = sched_clock();
		pdrv->config.cus_ctrl.mute_time = local_time[1] - local_time[0];

		//step 2: driver change
		if (pdrv->status & LCD_STATUS_PREPARE) {
			mutex_lock(&lcd_vout_mutex);
			lcd_tablet_driver_change(pdrv);
			mutex_unlock(&lcd_vout_mutex);
		}

		//step 3: switch on
		if (pdrv->status & LCD_STATUS_POWER) {
			aml_lcd_notifier_call_chain(LCD_EVENT_DLG_SWITCH_MODE, (void *)pdrv);
			lcd_queue_work(&pdrv->screen_restore_work);
		}

		local_time[1] = sched_clock();
		pdrv->config.cus_ctrl.switch_time = local_time[1] - local_time[0];
		return;
	}

	//step 1: switch off
	if (pdrv->status & LCD_STATUS_POWER) {
		/* include lcd_vout_mutex */
		aml_lcd_notifier_call_chain(LCD_EVENT_DLG_IF_POWER_OFF, (void *)pdrv);
	}
	local_time[1] = sched_clock();
	pdrv->config.cus_ctrl.power_off_time = local_time[1] - local_time[0];

	//step 2: driver change
	if (pdrv->status & LCD_STATUS_PREPARE) {
		mutex_lock(&lcd_vout_mutex);
		lcd_tablet_driver_change(pdrv);
		mutex_unlock(&lcd_vout_mutex);
	}

	//step 3: switch on
	if (pdrv->status & LCD_STATUS_POWER) {
		/* include lcd_vout_mutex */
		aml_lcd_notifier_call_chain(LCD_EVENT_DLG_IF_POWER_ON, (void *)pdrv);
		lcd_if_enable_retry(pdrv);
	}
	local_time[1] = sched_clock();
	pdrv->config.cus_ctrl.switch_time = local_time[1] - local_time[0];
}

static int lcd_set_current_vmode(enum vmode_e mode, void *data)
{
	struct aml_lcd_drv_s *pdrv = (struct aml_lcd_drv_s *)data;
	int ret = 0;

	if (!pdrv)
		return -1;

	if (lcd_vout_serve_bypass) {
		LCDPR("[%d]: vout_serve bypass\n", pdrv->index);
		return 0;
	}

	if ((mode & VMODE_MODE_BIT_MASK) != VMODE_LCD) {
		LCDPR("[%d]: unsupport mode 0x%x\n", pdrv->index, mode & VMODE_MODE_BIT_MASK);
		return -1;
	}

	mutex_lock(&lcd_power_mutex);

	/* clear fr*/
	pdrv->fr_duration = 0;
	pdrv->fr_mode = 0;

	mutex_lock(&lcd_vout_mutex);
	lcd_vmode_update(pdrv);
	lcd_vmode_vinfo_update(pdrv);
	mutex_unlock(&lcd_vout_mutex);

	lcd_vrr_dev_register(pdrv);
	if (mode & VMODE_INIT_BIT_MASK) { // first boot: set clktree
		lcd_clk_gate_switch(pdrv, 1);
	} else if (pdrv->status & LCD_STATUS_ENCL_ON) { // mode change by vout
		if (pdrv->vmode_switch) {
			lcd_vmode_switch(pdrv);
		} else {
			mutex_lock(&lcd_vout_mutex);
			lcd_tablet_driver_change(pdrv);
			mutex_unlock(&lcd_vout_mutex);
		}
	} else if (!(pdrv->status & LCD_STATUS_ENCL_ON)) {
		// mode change by drm (disable + mode_change)
		aml_lcd_notifier_call_chain(LCD_EVENT_ENABLE, (void *)pdrv);
		pdrv->status |= LCD_EVENT_ENABLE;
	}

	/* must update vrr dev after driver change for panel parameters update */
	lcd_vrr_dev_update(pdrv);

	pdrv->vmode_switch = 0;
	pdrv->status |= LCD_STATUS_VMODE_ACTIVE;

	mutex_unlock(&lcd_power_mutex);
	return ret;
}

static int lcd_vout_disable(enum vmode_e cur_vmod, void *data)
{
	struct aml_lcd_drv_s *pdrv = (struct aml_lcd_drv_s *)data;

	if (!pdrv)
		return -1;

	mutex_lock(&lcd_power_mutex);
	lcd_vrr_dev_unregister(pdrv);

	pdrv->status &= ~(LCD_STATUS_VMODE_ACTIVE | LCD_STATUS_PREPARE | LCD_STATUS_POWER);
	aml_lcd_notifier_call_chain(LCD_EVENT_DISABLE, (void *)pdrv);
	LCDPR("%s finished\n", __func__);
	mutex_unlock(&lcd_power_mutex);

	return 0;
}

static int lcd_vout_set_state(int index, void *data)
{
	struct aml_lcd_drv_s *pdrv = (struct aml_lcd_drv_s *)data;

	if (!pdrv)
		return -1;

	pdrv->vout_state |= (1 << index);
	pdrv->viu_sel = index;

	return 0;
}

static int lcd_vout_clr_state(int index, void *data)
{
	struct aml_lcd_drv_s *pdrv = (struct aml_lcd_drv_s *)data;

	if (!pdrv)
		return -1;

	pdrv->vout_state &= ~(1 << index);
	if (pdrv->viu_sel == index)
		pdrv->viu_sel = LCD_VIU_SEL_NONE;

	return 0;
}

static int lcd_vout_get_state(void *data)
{
	struct aml_lcd_drv_s *pdrv = (struct aml_lcd_drv_s *)data;
	return pdrv->vout_state;
}

static int lcd_vout_get_disp_cap(char *buf, void *data)
{
	struct aml_lcd_drv_s *pdrv = (struct aml_lcd_drv_s *)data;
	struct lcd_vmode_list_s *temp_list;
	unsigned int frame_rate;
	int i, ret = 0;

	if (!pdrv)
		return 0;

	temp_list = pdrv->vmode_mgr.vmode_list_header;
	while (temp_list) {
		if (!temp_list->info)
			continue;
		ret += str_add_vmode(buf + ret, 1, temp_list->info->width,
			temp_list->info->height, temp_list->info->base_fr);

		for (i = 0; i < LCD_DURATION_MAX; i++) {
			frame_rate = temp_list->info->duration[i].frame_rate;
			if (frame_rate == 0)
				break;
			ret += str_add_vmode(buf + ret, 1, temp_list->info->width,
				temp_list->info->height, frame_rate);
		}
		temp_list = temp_list->next;
	}

	return ret;
}

static enum vmode_e lcd_validate_vmode(char *mode, unsigned int frac, void *data)
{
	struct aml_lcd_drv_s *pdrv = (struct aml_lcd_drv_s *)data;

	if (!pdrv || !mode || frac)
		return VMODE_MAX;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("[%d]: %s: display_mode=%s, drv_mode=%s\n",
		      pdrv->index, __func__, mode, pdrv->vinfo.name);
	}

	if (lcd_tablet_outputmode_check(pdrv, mode))
		return VMODE_MAX;

	return VMODE_LCD;
}

static int lcd_framerate_support_check(struct aml_lcd_drv_s *pdrv, int duration)
{
	//unsigned int fr, ht, vt;
	//unsigned char i;
	struct lcd_detail_timing_s *dtm = &pdrv->config.timing.act_timing;

	//if (dtm->fixed_type != 0xff) {
	//	for (i = 0; i < 8; i++) {
	//		if (dtm->fixed_val_set[i] == 0)
	//			break;
	//		if (n >= LCD_DURATION_MAX)
	//			break;
	//		fr = (dtm->fixed_type == 0) ? dtm->fixed_val_set[i] : dtm->pixel_clk;
	//		ht = (dtm->fixed_type == 1) ? dtm->fixed_val_set[i] : dtm->h_period;
	//		vt = (dtm->fixed_type == 2) ? dtm->fixed_val_set[i] : dtm->v_period;
	//		fr = ((fr / ht) * 100) / vt;
	//		if (fr == duration)
	//			return 1;
	//	}
	//}

	if ((duration <= dtm->frame_rate_max * 100) && (duration >= dtm->frame_rate_min * 100))
		return 1;

	return 0;
}

static int lcd_framerate_automation_set_mode(struct aml_lcd_drv_s *pdrv)
{
	if (!pdrv)
		return -1;

	LCDPR("%s\n", __func__);
	lcd_vout_notify_mode_change_pre(pdrv);

	/* update clk & timing config */
	lcd_frame_rate_change(pdrv);
	/* update interface timing if needed, current no need */
#ifdef CONFIG_AMLOGIC_VPU
	vpu_dev_clk_request(pdrv->lcd_vpu_dev, pdrv->config.timing.enc_clk);
#endif

	/* change clk parameter */
	lcd_clk_change(pdrv);
	lcd_venc_change(pdrv);

	lcd_vout_notify_mode_change(pdrv);

	return 0;
}

static int lcd_set_vframe_rate_hint(int duration, void *data)
{
	struct aml_lcd_drv_s *pdrv = (struct aml_lcd_drv_s *)data;
	struct vinfo_s *info;
	unsigned int i, duration_num = 60, duration_den = 1, duration_frac = 0;

	if (!pdrv)
		return -1;
	if (pdrv->probe_done == 0)
		return -1;

	if ((pdrv->status & LCD_STATUS_ENCL_ON) == 0) {
		LCDPR("[%d]: %s: lcd is disabled, exit\n", pdrv->index, __func__);
		return -1;
	}

	if (lcd_fr_is_fixed(pdrv)) {
		LCDPR("[%d]: %s: fixed timing, exit\n", pdrv->index, __func__);
		return -1;
	}

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: fr_auto_flag = 0x%x\n", pdrv->index, pdrv->config.fr_auto_flag);

	info = &pdrv->vinfo;

	if (!pdrv->config.fr_auto_flag) {
		LCDPR("[%d]: %s: fr_auto_flag = 0x%x, disabled\n",
		      pdrv->index, __func__, pdrv->config.fr_auto_flag);
		return 0;
	}

	if (duration == 0) { /* end hint */
		LCDPR("[%d]: %s: return mode = %s, fr_auto_flag = 0x%x\n",
			pdrv->index, __func__, info->name, pdrv->config.fr_auto_flag);

		pdrv->fr_duration = 0;
		if (pdrv->fr_mode == 0) {
			LCDPR("[%d]: %s: fr_mode is invalid, exit\n", pdrv->index, __func__);
			return 0;
		}

		/* update frame rate */
		pdrv->config.timing.act_timing.frame_rate =
			pdrv->config.timing.base_timing.frame_rate;
		pdrv->config.timing.act_timing.sync_duration_num =
			pdrv->config.timing.base_timing.sync_duration_num;
		pdrv->config.timing.act_timing.sync_duration_den =
			pdrv->config.timing.base_timing.sync_duration_den;
		pdrv->config.timing.act_timing.frac =
			pdrv->config.timing.base_timing.frac;
		pdrv->fr_mode = 0;
	} else {
		if (!lcd_framerate_support_check(pdrv, duration)) {
			LCDERR("[%d]: %s: can't support duration %d\n, exit\n",
			       pdrv->index, __func__, duration);
			return -1;
		}
		LCDPR("[%d]: %s: fr_auto_flag = 0x%x, duration = %d, frame_rate = %d\n",
		      pdrv->index, __func__, pdrv->config.fr_auto_flag, duration, duration / 100);

		for (i = 0; i < ARRAY_SIZE(lcd_std_fr); i++) {
			if (duration ==
				((lcd_std_fr[i].duration_num * 100) / lcd_std_fr[i].duration_den))
				break;
		}
		if (i < ARRAY_SIZE(lcd_std_fr)) {
			duration_num  = lcd_std_fr[i].duration_num;
			duration_den  = lcd_std_fr[i].duration_den;
			duration_frac = lcd_std_fr[i].frac;
		} else {
			duration_num  = duration;
			duration_den  = 100;
			duration_frac = (bool)(duration % 100);
		}

		pdrv->fr_duration = duration;
		/* if the sync_duration is same as current */
		if (duration_num == pdrv->config.timing.act_timing.sync_duration_num &&
		    duration_den == pdrv->config.timing.act_timing.sync_duration_den) {
			LCDPR("[%d]: %s: sync_duration is same, exit\n", pdrv->index, __func__);
			return 0;
		}

		/* update frame rate */
		pdrv->config.timing.act_timing.frame_rate = duration_num / duration_den;
		pdrv->config.timing.act_timing.sync_duration_num = duration_num;
		pdrv->config.timing.act_timing.sync_duration_den = duration_den;
		pdrv->config.timing.act_timing.frac = duration_frac;
		pdrv->fr_mode = duration;
	}

	lcd_framerate_automation_set_mode(pdrv);

	return 0;
}

static int lcd_get_vframe_rate_hint(void *data)
{
	struct aml_lcd_drv_s *pdrv = (struct aml_lcd_drv_s *)data;

	if (!pdrv)
		return 0;

	return pdrv->fr_duration;
}

static void lcd_vout_debug_test(unsigned int num, void *data)
{
	struct aml_lcd_drv_s *pdrv = (struct aml_lcd_drv_s *)data;

	if (!pdrv)
		return;

	lcd_debug_test(pdrv, num);
}

static void lcd_vout_set_bl_brightness(unsigned int brightness, void *data)
{
	struct aml_lcd_drv_s *pdrv = (struct aml_lcd_drv_s *)data;
	struct aml_bl_drv_s *bdrv;

	if (!pdrv)
		return;

	bdrv = aml_bl_get_driver(pdrv->index);
	aml_bl_set_level_brightness(bdrv, brightness);
}

static unsigned int lcd_vout_get_bl_brightness(void *data)
{
	struct aml_lcd_drv_s *pdrv = (struct aml_lcd_drv_s *)data;
	struct aml_bl_drv_s *bdrv;

	if (!pdrv)
		return 0;

	bdrv = aml_bl_get_driver(pdrv->index);
	return aml_bl_get_level_brightness(bdrv);
}

static int lcd_suspend(void *data)
{
	struct aml_lcd_drv_s *pdrv = (struct aml_lcd_drv_s *)data;

	if (!pdrv)
		return -1;

	mutex_lock(&lcd_power_mutex);
	pdrv->status &= ~LCD_STATUS_POWER;
	aml_lcd_notifier_call_chain(LCD_EVENT_POWER_OFF, (void *)pdrv);
	LCDPR("[%d]: early_suspend finished\n", pdrv->index);
	mutex_unlock(&lcd_power_mutex);
	return 0;
}

static int lcd_resume(void *data)
{
	struct aml_lcd_drv_s *pdrv = (struct aml_lcd_drv_s *)data;

	if (!pdrv)
		return -1;

	if ((pdrv->status & LCD_STATUS_VMODE_ACTIVE) == 0)
		return 0;

	if (pdrv->status & LCD_STATUS_POWER)
		return 0;

	if (pdrv->resume_type) {
		lcd_queue_work(&pdrv->late_resume_work);
	} else {
		mutex_lock(&lcd_power_mutex);
		LCDPR("[%d]: directly lcd late_resume\n", pdrv->index);
		aml_lcd_notifier_call_chain(LCD_EVENT_POWER_ON, (void *)pdrv);
		lcd_if_enable_retry(pdrv);
		pdrv->status |= LCD_STATUS_POWER;
		LCDPR("[%d]: late_resume finished\n", pdrv->index);
		mutex_unlock(&lcd_power_mutex);
	}

	return 0;
}

// * bind 3 vout op
void lcd_tablet_vout_server_init(struct aml_lcd_drv_s *pdrv)
{
	unsigned int cnt_idx, lcd_type = pdrv->config.basic.lcd_type;
	char *connector_name_list[4] = {"LVDS", "VBYONE", "MIPI", "EDP"};
	char *curr_vout_connector;

	if (lcd_type == LCD_LVDS || lcd_type == LCD_MLVDS)
		cnt_idx = 0;
	else if (lcd_type == LCD_VBYONE || lcd_type == LCD_P2P)
		cnt_idx = 1;
	else if (lcd_type == LCD_MIPI)
		cnt_idx = 2;
	else if (lcd_type == LCD_EDP)
		cnt_idx = 3;
	else
		return;

	pdrv->vout_server = kzalloc(sizeof(*pdrv->vout_server), GFP_KERNEL);
	if (!pdrv->vout_server)
		return;
	pdrv->vout_server->name = kzalloc(32, GFP_KERNEL);
	if (!pdrv->vout_server->name) {
		kfree(pdrv->vout_server);
		return;
	}
	sprintf(pdrv->vout_server->name, "%s-%c", connector_name_list[cnt_idx], 'A' + pdrv->index);

	lcd_dft_timing_update_vinfo(pdrv);

	pdrv->vout_server->op.get_vinfo = lcd_get_current_info;
	pdrv->vout_server->op.set_vmode = lcd_set_current_vmode;
	pdrv->vout_server->op.validate_vmode = lcd_validate_vmode;
	pdrv->vout_server->op.check_same_vmodeattr = lcd_check_same_vmodeattr;
	pdrv->vout_server->op.vmode_is_supported = lcd_vmode_is_supported;
	pdrv->vout_server->op.disable = lcd_vout_disable;
	pdrv->vout_server->op.set_state = lcd_vout_set_state;
	pdrv->vout_server->op.clr_state = lcd_vout_clr_state;
	pdrv->vout_server->op.get_state = lcd_vout_get_state;
	pdrv->vout_server->op.get_disp_cap = lcd_vout_get_disp_cap;
	pdrv->vout_server->op.set_vframe_rate_hint = lcd_set_vframe_rate_hint;
	pdrv->vout_server->op.get_vframe_rate_hint = lcd_get_vframe_rate_hint;
	pdrv->vout_server->op.set_bist = lcd_vout_debug_test;
	pdrv->vout_server->op.set_bl_brightness = lcd_vout_set_bl_brightness;
	pdrv->vout_server->op.get_bl_brightness = lcd_vout_get_bl_brightness;
	pdrv->vout_server->op.vout_suspend = lcd_suspend;
	pdrv->vout_server->op.vout_resume = lcd_resume;
	pdrv->vout_server->data = (void *)pdrv;

#ifdef CONFIG_AMLOGIC_VOUT_SERVE
	curr_vout_connector = get_uboot_connector0_type();
	if (!strcmp(pdrv->vout_server->name, curr_vout_connector)) {
		pdrv->viu_sel = 1;
		sprintf(pdrv->vout_server->name, "lcd%d_vout_server", pdrv->index);
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			LCDPR("[%u]: regist: %s\n", pdrv->index, pdrv->vout_server->name);
		vout_register_server(pdrv->vout_server);
		return;
	}
#endif
#ifdef CONFIG_AMLOGIC_VOUT2_SERVE
	curr_vout_connector = get_uboot_connector1_type();
	if (!strcmp(pdrv->vout_server->name, curr_vout_connector)) {
		pdrv->viu_sel = 2;
		sprintf(pdrv->vout_server->name, "lcd%d_vout2_server", pdrv->index);
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			LCDPR("[%u]: regist: %s\n", pdrv->index, pdrv->vout_server->name);
		vout2_register_server(pdrv->vout_server);
		return;
	}
#endif
#ifdef CONFIG_AMLOGIC_VOUT3_SERVE
	curr_vout_connector = get_uboot_connector2_type();
	if (!strcmp(pdrv->vout_server->name, curr_vout_connector)) {
		pdrv->viu_sel = 3;
		sprintf(pdrv->vout_server->name, "lcd%d_vout3_server", pdrv->index);
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			LCDPR("[%u]: regist: %s\n", pdrv->index, pdrv->vout_server->name);
		vout3_register_server(pdrv->vout_server);
		return;
	}
#endif
}

void lcd_tablet_vout_server_remove(struct aml_lcd_drv_s *pdrv)
{
#ifdef CONFIG_AMLOGIC_VOUT_SERVE
	if (pdrv->viu_sel == 1) {
		vout_unregister_server(pdrv->vout_server);
		return;
	}
#endif
#ifdef CONFIG_AMLOGIC_VOUT2_SERVE
	if (pdrv->viu_sel == 2) {
		vout2_unregister_server(pdrv->vout_server);
		return;
	}
#endif
#ifdef CONFIG_AMLOGIC_VOUT3_SERVE
	if (pdrv->viu_sel == 3) {
		vout3_unregister_server(pdrv->vout_server);
		return;
	}
#endif
}

static void lcd_vmode_init(struct aml_lcd_drv_s *pdrv)
{
	char *mode, *init_mode;
	enum vmode_e vmode;

	init_mode = get_vout_mode_uboot();
	mode = kstrdup(init_mode, GFP_KERNEL);
	if (!mode)
		return;

	lcd_tablet_add_all_vmode(pdrv);
	LCDPR("[%d]: %s: mode: %s\n", pdrv->index, __func__, mode);
	vmode = lcd_validate_vmode(mode, 0, (void *)pdrv);
	if (vmode == VMODE_LCD)
		lcd_vmode_update(pdrv);

	lcd_vmode_vinfo_update(pdrv);
	kfree(mode);
}

static void lcd_config_init(struct aml_lcd_drv_s *pdrv)
{
	lcd_enc_timing_init_config(pdrv);

	lcd_clk_config_parameter_init(pdrv);
	lcd_clk_generate_parameter(pdrv);
	pdrv->config.timing.clk_change = 0; /* clear clk_change flag */

	lcd_tablet_config_post_update(pdrv);
	lcd_vmode_init(pdrv);
	lcd_dft_timing_update_vinfo(pdrv);
}

/* **************************************************
 * lcd notify
 * **************************************************
 */
/* sync_duration is real_value * 100 */
static void lcd_frame_rate_adjust(struct aml_lcd_drv_s *pdrv, int duration)
{
	LCDPR("%s: sync_duration=%d\n", __func__, duration);

	lcd_vout_notify_mode_change_pre(pdrv);

	/* update frame rate */
	pdrv->config.timing.act_timing.frame_rate = duration / 100;
	pdrv->config.timing.act_timing.sync_duration_num = duration;
	pdrv->config.timing.act_timing.sync_duration_den = 100;

	/* update interface timing */
	lcd_frame_rate_change(pdrv);
#ifdef CONFIG_AMLOGIC_VPU
	vpu_dev_clk_request(pdrv->lcd_vpu_dev, pdrv->config.timing.enc_clk);
#endif

	/* change clk parameter */
	lcd_clk_change(pdrv);
	lcd_venc_change(pdrv);

	lcd_vout_notify_mode_change(pdrv);
}

/* **************************************************
 * lcd tablet
 * **************************************************
 */
int lcd_mode_tablet_init(struct aml_lcd_drv_s *pdrv)
{
	if (!pdrv)
		return -1;

	lcd_config_init(pdrv);

	pdrv->driver_init_pre = lcd_tablet_driver_init_pre;
	pdrv->driver_disable_post = lcd_tablet_driver_disable_post;
	pdrv->driver_init = lcd_tablet_driver_init;
	pdrv->driver_disable = lcd_tablet_driver_disable;
	pdrv->driver_change = lcd_tablet_driver_change;
	pdrv->fr_adjust = lcd_frame_rate_adjust;

	if (pdrv->status & LCD_STATUS_VMODE_ACTIVE)
		lcd_vrr_dev_register(pdrv);

	return 0;
}

int lcd_mode_tablet_remove(struct aml_lcd_drv_s *pdrv)
{
	lcd_vrr_dev_unregister(pdrv);
	return 0;
}
