// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/init.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/notifier.h>
#include <linux/reset.h>
#include <linux/amlogic/media/vout/lcd/lcd_vout.h>
#include <linux/amlogic/media/vout/lcd/lcd_notify.h>
//#include "../lcd_reg_t3x.h"
#include "../lcd_reg.h"
#include "../lcd_common.h"
#include "lcd_venc.h"

static void lcd_venc_wait_vsync(struct aml_lcd_drv_s *pdrv)
{
	unsigned int line_cnt, line_cnt_previous;
	unsigned int reg, offset;
	int i = 0;

	if (!pdrv)
		return;

	offset = pdrv->index == 1 ? 0x600 : 0x00;
	reg = VPU_VENCP_STAT + (offset << 2);

	line_cnt = 0x1fff;
	line_cnt_previous = lcd_vcbus_getb(reg, 0, 13);
	while (i++ < LCD_WAIT_VSYNC_TIMEOUT) {
		line_cnt = lcd_vcbus_getb(reg, 0, 13);
		if (line_cnt < line_cnt_previous)
			break;
		line_cnt_previous = line_cnt;
		udelay(2);
	}
}

static void lcd_venc_gamma_check_en(struct aml_lcd_drv_s *pdrv)
{
	unsigned int reg, offset;

	offset = pdrv->data->offset_venc_data[pdrv->index];
	reg = LCD_GAMMA_CNTL_PORT0_T3X + offset;

	if (lcd_vcbus_getb(reg, 0, 1))
		pdrv->gamma_en_flag = 1;
	else
		pdrv->gamma_en_flag = 0;
	LCDPR("[%d]: %s: %d\n", pdrv->index, __func__, pdrv->gamma_en_flag);
}

static void lcd_venc_gamma_debug_test_en(struct aml_lcd_drv_s *pdrv, int flag)
{
	unsigned int reg, offset;

	offset = pdrv->data->offset_venc_data[pdrv->index];
	reg = LCD_GAMMA_CNTL_PORT0_T3X + offset;

	if (flag) {
		if (pdrv->gamma_en_flag) {
			if (lcd_vcbus_getb(reg, 0, 1) == 0) {
				lcd_vcbus_setb(reg, 1, 0, 1);
				LCDPR("[%d]: %s: %d\n", pdrv->index, __func__, flag);
			}
		}
	} else {
		if (pdrv->gamma_en_flag) {
			if (lcd_vcbus_getb(reg, 0, 1)) {
				lcd_vcbus_setb(reg, 0, 0, 1);
				LCDPR("[%d]: %s: %d\n", pdrv->index, __func__, flag);
			}
		}
	}
}

#define LCD_ENC_TST_NUM_MAX    10
struct lcd_enc_test_t {
	char *name;
	unsigned int mode;
	unsigned int y;
	unsigned int cb;
	unsigned int cr;
	unsigned int en;
	unsigned int vfifo_en;
	unsigned int rgb_in;
};

struct lcd_enc_test_t lcd_enc_tst[] = {
	/*       tst_mode,   Y/B, Cb/G,   Cr/R,  tst_en,  vfifo_en  rgbin*/
	{"0-none",      0, 0x200, 0x200,  0x200,   0,      1,        0},
	{"1-Color Bar", 1, 0x200, 0x200,  0x200,   1,      0,        0},  /* 1 */
	{"2-Thin Line", 2, 0x200, 0x200,  0x200,   1,      0,        0},  /* 2 */
	{"3-Dot Grid",  3, 0x200, 0x200,  0x200,   1,      0,        0},  /* 3 */
	{"4-X icon",    4, 0x200, 0x200,  0x200,   1,      0,        0},  /* 4 */
	{"5-Gray",      0, 0x1ff, 0x1ff,  0x1ff,   1,      0,        1},  /* 5 */
	{"6-Red",       0, 0x000, 0x000,  0x3ff,   1,      0,        1},  /* 6 */
	{"7-Green",     0, 0x000, 0x3ff,  0x000,   1,      0,        1},  /* 7 */
	{"8-Blue",      0, 0x3ff, 0x000,  0x000,   1,      0,        1},  /* 8 */
	{"9-Black",     0, 0x000, 0x000,  0x000,   1,      0,        1},  /* 9 */
};

static void lcd_venc_pattern(struct aml_lcd_drv_s *pdrv, unsigned int num)
{
	unsigned int start, width, offset, height;

	if (num >= LCD_ENC_TST_NUM_MAX)
		return;

	offset = pdrv->data->offset_venc[pdrv->index];

	start = pdrv->config.timing.hstart;
	height = pdrv->config.basic.v_active;

	if (num == 4) {
		width = pdrv->config.basic.h_active;
		lcd_vcbus_setb(ENCL_TST_DATA + offset, (width << 8) / height, 10, 10);//px step
		lcd_vcbus_setb(ENCL_TST_DATA + offset, 0, 0, 10);//expand lr
	} else {
		width = pdrv->config.basic.h_active / 8;
		lcd_vcbus_setb(ENCL_TST_DATA + offset, lcd_enc_tst[num].y, 20, 10);//color Y/B
		lcd_vcbus_setb(ENCL_TST_DATA + offset, lcd_enc_tst[num].cb, 10, 10);//color cb/G
		lcd_vcbus_setb(ENCL_TST_DATA + offset, lcd_enc_tst[num].cr, 0, 10);//color cb/G
	}
	lcd_vcbus_setb(ENCL_TST_CLRBAR_CNTL + offset, start, 0, 13);//color bar x start
	lcd_vcbus_setb(ENCL_TST_CLRBAR_CNTL + offset, width, 16, 13);//color bar  width
	lcd_vcbus_setb(ENCL_TST_CRSBAR_CNTL + offset, 0, 0, 13);//color bar y start
	lcd_vcbus_setb(ENCL_TST_CRSBAR_CNTL + offset, height, 16, 13);//color bar  height

	lcd_vcbus_setb(ENCL_TST_EN_T3X + offset, lcd_enc_tst[num].mode, 0, 8);//encl_test_model_sel
	lcd_vcbus_setb(ENCL_TST_EN_T3X + offset, 0, 12, 2);//vdcnt_start renctange width
	lcd_vcbus_setb(ENCL_TST_EN_T3X + offset, lcd_enc_tst[num].en, 8, 1);//enable

	lcd_vcbus_setb(ENCL_VIDEO_VSRC_CTRL + offset, lcd_enc_tst[num].vfifo_en, 4, 1);
	/* 1: rgb to rgb   0: yuv to rgb) */
	lcd_vcbus_setb(ENCL_VIDEO_MODE_T3X + offset, lcd_enc_tst[num].rgb_in, 16,  1);

	if (num > 0) {
		LCDPR("[%d]: show test pattern: %s\n",
		      pdrv->index, lcd_enc_tst[num].name);
	}
}

static int lcd_venc_debug_test(struct aml_lcd_drv_s *pdrv, unsigned int num)
{
	if (num > 0)
		lcd_venc_gamma_debug_test_en(pdrv, 0);
	else
		lcd_venc_gamma_debug_test_en(pdrv, 1);

	lcd_venc_pattern(pdrv, num);

	return 0;
}

static void lcd_venc_gamma_init(struct aml_lcd_drv_s *pdrv)
{
	int index = pdrv->index;

	if (pdrv->lcd_pxp)
		return;

	aml_lcd_notifier_call_chain(LCD_EVENT_GAMMA_UPDATE, &index);
	lcd_venc_gamma_check_en(pdrv);
}

static void lcd_venc_set_tcon(struct aml_lcd_drv_s *pdrv)
{
	//TODO
}

static void lcd_venc_set_timing(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_config_s *pconf = &pdrv->config;
	unsigned int hstart, hend, vstart, vend, h_period, v_period;
	unsigned int offset;//, ppc;
	unsigned int pre_de_vs, pre_de_ve, pre_de_hs, pre_de_he;
	unsigned int hs_hs_addr, hs_he_addr, vs_vs_addr, vs_ve_addr, vs_hs_addr, vs_he_addr;

	offset = pdrv->data->offset_venc[pdrv->index];
	hstart = pconf->timing.hstart;
	hend = pconf->timing.hend;
	vstart = pconf->timing.vstart;
	vend = pconf->timing.vend;
	h_period = pconf->basic.h_period;
	v_period = pconf->basic.v_period;
	//ppc = pconf->timing.ppc;
	hs_hs_addr = pconf->timing.hs_hs_addr;
	hs_he_addr = pconf->timing.hs_he_addr;
	vs_hs_addr = pconf->timing.vs_hs_addr;
	vs_he_addr = pconf->timing.vs_he_addr;
	vs_vs_addr = pconf->timing.vs_vs_addr;
	vs_ve_addr = pconf->timing.vs_ve_addr;

	lcd_vcbus_write(ENCL_VIDEO_MODE_T3X + offset, 0x0040);
	lcd_vcbus_setb(ENCL_VIDEO_VSRC_CTRL + offset, 1, 4, 1);//reg_vfifo_en
	/*vfifo_upmode, 0:1ppc,1:2ppc,2, 4ppc */
	lcd_vcbus_setb(ENCL_VIDEO_VSRC_CTRL + offset, 0, 0, 3);

	lcd_vcbus_setb(ENCL_VIDEO_MAX_CNT + offset, h_period - 1, 16, 16);
	lcd_vcbus_setb(ENCL_VIDEO_MAX_CNT + offset, v_period - 1, 0, 16);

	lcd_vcbus_setb(ENCL_VIDEO_HAVON_PX_RNG + offset, hstart, 16, 16);//HAVON_BEGIN
	lcd_vcbus_setb(ENCL_VIDEO_HAVON_PX_RNG + offset, hend, 0, 16);//HAVON_END

	lcd_vcbus_setb(ENCL_VIDEO_VAVON_LN_RNG + offset, vstart, 16, 16);//BLINE
	lcd_vcbus_setb(ENCL_VIDEO_VAVON_LN_RNG + offset, vend, 0, 16);//ELINE

	lcd_vcbus_setb(ENCL_VIDEO_HSO_PX_RNG + offset, hs_hs_addr, 16, 16);//HSO_BEGIN
	lcd_vcbus_setb(ENCL_VIDEO_HSO_PX_RNG + offset, hs_he_addr, 0, 16);//HSO_END

	lcd_vcbus_setb(ENCL_VIDEO_VSO_PX_RNG + offset, vs_hs_addr, 16, 16);//HSO_BEGIN
	lcd_vcbus_setb(ENCL_VIDEO_VSO_PX_RNG + offset, vs_he_addr, 0, 16);//HSO_END

	lcd_vcbus_setb(ENCL_VIDEO_VSO_LN_RNG + offset, vs_vs_addr, 16, 16);//VSO BLINE
	lcd_vcbus_setb(ENCL_VIDEO_VSO_LN_RNG + offset, vs_ve_addr, 0, 16);//VSO ELINE

	if (pconf->basic.lcd_type == LCD_P2P ||
	    pconf->basic.lcd_type == LCD_MLVDS) {
		pre_de_vs = vstart - 8;
		pre_de_ve = pconf->basic.v_active + pre_de_vs;
		pre_de_hs = hstart;
		pre_de_he = pconf->basic.h_active - 1 + pre_de_hs;
		lcd_vcbus_setb(ENCL_VIDEO_V_PRE_DE_LN_RNG + offset, pre_de_vs, 16, 16);
		lcd_vcbus_setb(ENCL_VIDEO_V_PRE_DE_ELINE + offset, pre_de_ve, 0, 16);
		lcd_vcbus_setb(ENCL_VIDEO_H_PRE_DE_PX_RNG + offset, pre_de_hs + 6, 16, 16);
		lcd_vcbus_setb(ENCL_VIDEO_H_PRE_DE_PX_RNG + offset,   pre_de_he + 6, 0, 16);
	}

	switch (pdrv->data->chip_type) {
	case LCD_CHIP_T3X:
		lcd_vcbus_write(ENCL_INBUF_CNTL1 + offset,
				(4 << 13) | (pconf->basic.h_active - 1));
		lcd_vcbus_write(ENCL_INBUF_CNTL0_T3X + offset, 0x200);
		break;
	default:
		break;
	}
	lcd_venc_set_tcon(pdrv);
	aml_lcd_notifier_call_chain(LCD_EVENT_BACKLIGHT_UPDATE, (void *)pdrv);
}

static void lcd_venc_set(struct aml_lcd_drv_s *pdrv)
{
	unsigned int reg_disp_viu_ctrl, offset, offset1;
	unsigned int frm_hsize, reg_data, vpp0_ppc_num;
	unsigned int ppc = 1;

	if (!pdrv || !pdrv->data)
		return;

	offset = pdrv->data->offset_venc[pdrv->index];
	ppc = pdrv->config.timing.ppc;

	lcd_vcbus_write(ENCL_VIDEO_EN_T3X + offset, 0);

	lcd_set_venc_timing(pdrv);

	//restore test pattern
	lcd_venc_pattern(pdrv, pdrv->test_state);

	if (pdrv->index == 0) {
		reg_data  = lcd_vcbus_read(ENCL_VIDEO_HAVON_PX_RNG);
		frm_hsize = (reg_data & 0xFFFF) - ((reg_data >> 16) & 0xFFFF) + 1;
		vpp0_ppc_num  = lcd_vcbus_read(VPP_4S4P_CTRL) & 3;	//2: 1ppc, 1: 2ppc, 0: 4ppc
		//s2p enable, only for venc0
		lcd_vcbus_setb(VPU_VENC_DITH, vpp0_ppc_num == 2 && ppc == 2, 31, 1);
		lcd_vcbus_setb(VPU_VENC_DITH, frm_hsize, 0,  13); //hsize
	}

	offset1 = pdrv->index == 1 ? 0x600 : 0x0;
	lcd_vcbus_setb(VPU_VENC_CTRL + offset1, 2, 0, 2);// enable timming
	lcd_vcbus_setb(ENCL_VIDEO_EN_T3X + offset, 1, 0, 1);//enable

	lcd_vcbus_setb(LCD_LCD_IF_CTRL + offset1, (0x400 << 12), 0, 23);
	lcd_vcbus_write(LCD_DITH_CTRL + offset1,   0);

	switch (pdrv->index) {
	case 0:
		reg_disp_viu_ctrl = VPU_DISP_VIU0_CTRL;
		break;
	case 1:
		reg_disp_viu_ctrl = VPU_DISP_VIU1_CTRL;
		break;
	case 2:
		reg_disp_viu_ctrl = VPU_DISP_VIU2_CTRL;
		break;
	default:
		LCDERR("[%d]: %s: invalid drv_index\n",
		pdrv->index, __func__);
		return;
	}

	/*
	 * bit31: lvds enable
	 * bit30: vx1 enable
	 * bit29: hdmitx enable
	 * bit28: dsi_edp enable
	 */
	switch (pdrv->config.basic.lcd_type) {
	case LCD_LVDS:
		lcd_vcbus_write(reg_disp_viu_ctrl, (1 << 31) |
						(0 << 30) |
						(0 << 29) |
						(0 << 28));
		break;
	case LCD_VBYONE:
		lcd_vcbus_write(reg_disp_viu_ctrl, (0 << 31) |
						(1 << 30) |
						(0 << 29) |
						(0 << 28));
		break;
	case LCD_MIPI:
	case LCD_EDP:
		lcd_vcbus_write(reg_disp_viu_ctrl, (0 << 31) |
						(0 << 30) |
						(0 << 29) |
						(1 << 28));
		break;
	default:
		break;
	}

	lcd_venc_gamma_init(pdrv);
}

static void lcd_venc_change_timing(struct aml_lcd_drv_s *pdrv)
{
	unsigned int htotal, vtotal, offset, ppc;

	offset = pdrv->data->offset_venc[pdrv->index];
	ppc = pdrv->config.timing.ppc;

	if (pdrv->vmode_update) {
		lcd_timing_init_config(pdrv);
		lcd_set_venc_timing(pdrv);
	} else {
		htotal = ((lcd_vcbus_read(ENCL_VIDEO_MAX_CNT + offset) >> 16) & 0xffff) + 1;
		vtotal = (lcd_vcbus_read(ENCL_VIDEO_MAX_CNT + offset) & 0xffff) + 1;

		if (pdrv->config.basic.h_period != htotal) {
			lcd_vcbus_setb(ENCL_VIDEO_MAX_CNT + offset,
					pdrv->config.basic.h_period / ppc - 1, 16, 16);
		}
		if (pdrv->config.basic.v_period != vtotal) {
			lcd_vcbus_setb(ENCL_VIDEO_MAX_CNT + offset,
					pdrv->config.basic.v_period - 1, 16, 16);
		}
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			LCDPR("[%d]: venc changed: %d,%d\n",
			      pdrv->index,
			      pdrv->config.basic.h_period,
			      pdrv->config.basic.v_period);
		}
	}

	aml_lcd_notifier_call_chain(LCD_EVENT_BACKLIGHT_UPDATE, (void *)pdrv);
}

static void lcd_venc_enable_ctrl(struct aml_lcd_drv_s *pdrv, int flag)
{
	unsigned int offset;

	offset = pdrv->data->offset_venc[pdrv->index];
	if (flag)
		lcd_vcbus_write(ENCL_VIDEO_EN_T3X + offset, 1);
	else
		lcd_vcbus_write(ENCL_VIDEO_EN_T3X + offset, 0);
}

static void lcd_venc_mute_set(struct aml_lcd_drv_s *pdrv, unsigned char flag)
{
	if (flag)
		lcd_venc_debug_test(pdrv, 9);
	else
		lcd_venc_debug_test(pdrv, 0);
}

static int lcd_venc_get_init_config(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_config_s *pconf = &pdrv->config;
	unsigned int offset, init_state;

	offset = pdrv->data->offset_venc[pdrv->index];

	pconf->basic.h_active = lcd_vcbus_getb(ENCL_VIDEO_HAVON_PX_RNG + offset, 0, 16)
		- lcd_vcbus_getb(ENCL_VIDEO_HAVON_PX_RNG + offset, 16, 16) + 1;
	pconf->basic.v_active = lcd_vcbus_getb(ENCL_VIDEO_VAVON_LN_RNG + offset, 0, 16)
		- lcd_vcbus_getb(ENCL_VIDEO_VAVON_LN_RNG + offset, 16, 16) + 1;
	pconf->basic.h_period = lcd_vcbus_getb(ENCL_VIDEO_MAX_CNT + offset, 16, 16) + 1;
	pconf->basic.v_period = lcd_vcbus_getb(ENCL_VIDEO_MAX_CNT + offset, 0, 16) + 1;

	lcd_venc_gamma_check_en(pdrv);

	init_state = lcd_vcbus_read(ENCL_VIDEO_EN_T3X + offset);
	return init_state;
}

static void lcd_venc_set_vrr_recovery(struct aml_lcd_drv_s *pdrv)
{
	unsigned int vtotal, offset;

	offset = pdrv->data->offset_venc[pdrv->index];

	vtotal = pdrv->config.basic.v_period;

	lcd_vcbus_setb(ENCL_VIDEO_MAX_CNT + offset, vtotal, 0, 16);
}

static unsigned int lcd_venc_get_encl_line_cnt(struct aml_lcd_drv_s *pdrv)
{
	unsigned int reg, offset, line_cnt;

	if (!pdrv)
		return 0;

	offset = pdrv->index == 1 ? 0x600 : 0x0;
	reg = VPU_VENCP_STAT + offset;

	line_cnt = lcd_vcbus_getb(reg, 16, 13);
	return line_cnt;
}

int lcd_venc_op_init_t3x(struct aml_lcd_drv_s *pdrv, struct lcd_venc_op_s *venc_op)
{
	if (!venc_op)
		return -1;

	LCDPR("%s %d\n", __func__, __LINE__);
	venc_op->wait_vsync = lcd_venc_wait_vsync;
	venc_op->gamma_test_en = lcd_venc_gamma_debug_test_en;
	venc_op->venc_debug_test = lcd_venc_debug_test;
	venc_op->venc_set_timing = lcd_venc_set_timing;
	venc_op->venc_set = lcd_venc_set;
	venc_op->venc_change = lcd_venc_change_timing;
	venc_op->venc_enable = lcd_venc_enable_ctrl;
	venc_op->mute_set = lcd_venc_mute_set;
	venc_op->get_venc_init_config = lcd_venc_get_init_config;
	venc_op->venc_vrr_recovery = lcd_venc_set_vrr_recovery;
	venc_op->get_encl_line_cnt = lcd_venc_get_encl_line_cnt;

	return 0;
};
