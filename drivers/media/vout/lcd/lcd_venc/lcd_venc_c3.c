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
#include "../lcd_reg.h"
#include "../lcd_common.h"
#include "lcd_venc.h"

static unsigned int lcd_dth_lut_c3[16] = {
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x82412814, 0x48122481, 0x18242184, 0x18242841,
	0x9653ca56, 0x6a3ca635, 0x6ca9c35a, 0xca935635,
	0x7dbedeb7, 0xde7dbed7, 0xeb7dbed7, 0xdbe77edb
};

static void lcd_set_venc_c3(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_config_s *pconf = &pdrv->config;
	unsigned int hsize, vsize;
	unsigned int reoder, timgen_mode, serial_rate, field_mode;
	unsigned int total_hsize, total_vsize;
	unsigned int hs_pix_bgn, hs_pix_end, vs_lne_bgn_e, vs_lne_end_e;
	unsigned int vs_pix_bgn_e, vs_pix_end_e, de_lne_bgn_e, de_lne_end_e;
	unsigned int de_px_bgn_e, de_px_end_e, bot_bgn_lne, top_bgn_lne;
	unsigned int vs_lne_bgn_o, vs_lne_end_o, vs_pix_bgn_o, vs_pix_end_o;
	unsigned int de_lne_bgn_o, de_lne_end_o;
	int i;

	hsize = pconf->basic.h_active;
	vsize = pconf->basic.v_active;
	reoder = 36;

	//timgen_mode:
	//1<<0:MIPI_TX, 1<<1:LCDs8,  1<<2:BT1120,  1<<3:BT656, 1<<4:lcds6,
	//1<<5:lcdp6, 1<<6:lcdp8, 1<<7:lcd565, 1<<8:lcds9(6+3,3+6), 1<<9:lcds8(5+3,3+5)

	//serial_rate:
	//0:pix/1cylce    1:pix/2cycle  2:pix/3cycle

	//field_mode:
	//0:progress 1:interlace

	switch (pconf->basic.lcd_type) {
	case LCD_MIPI:
		timgen_mode = (1 << 0);
		serial_rate = 0;
		field_mode = 0;
		break;
	case LCD_BT656:
		timgen_mode = (1 << 3);
		serial_rate = 0;
		field_mode = 1;
		break;
	case LCD_BT1120:
		timgen_mode = (1 << 2);
		serial_rate = 0;
		field_mode = 1;
		break;
	default:
		timgen_mode = (1 << 6);
		serial_rate = 0;
		field_mode = 0;
		break;
	}

	total_hsize  = pconf->basic.h_period;
	total_vsize  = pconf->basic.v_period;

	hs_pix_bgn   = pconf->timing.hs_hs_addr;
	hs_pix_end   = pconf->timing.hs_he_addr;
	vs_lne_bgn_e = pconf->timing.vs_vs_addr;
	vs_lne_end_e = pconf->timing.vs_ve_addr;
	vs_pix_bgn_e = pconf->timing.vs_hs_addr;
	vs_pix_end_e = pconf->timing.vs_he_addr;

	de_lne_bgn_e = pconf->timing.vstart;
	de_lne_end_e = pconf->timing.vend;
	de_px_bgn_e  = pconf->timing.hstart;
	de_px_end_e  = pconf->timing.hend;

	bot_bgn_lne  =    0;
	top_bgn_lne  =    0;
	vs_lne_bgn_o =    0;
	vs_lne_end_o =    0;
	vs_pix_bgn_o =    0;
	vs_pix_end_o =    0;
	de_lne_bgn_o =    0;
	de_lne_end_o =    0;

	lcd_vcbus_setb(VPU_VOUT_CORE_CTRL, 0, 0, 1); //disable venc_en
	lcd_vcbus_setb(VPU_VOUT_DETH_CTRL, 0, 5, 1); //10bit to 9bit
	lcd_vcbus_setb(VPU_VOUT_DETH_CTRL, hsize, 6, 13);
	lcd_vcbus_setb(VPU_VOUT_DETH_CTRL, vsize, 19, 13);
	lcd_vcbus_setb(VPU_VOUT_INT_CTRL, 1, 14, 1); //dth_en

	if (reoder != 36)
		lcd_vcbus_setb(VPU_VOUT_CORE_CTRL, reoder, 4, 6);
	lcd_vcbus_setb(VPU_VOUT_CORE_CTRL, timgen_mode, 16, 10);
	lcd_vcbus_setb(VPU_VOUT_CORE_CTRL, serial_rate, 2, 2);

	//if (bt1120 || bt656) {
	//	lcd_vcbus_setb(VPU_VOUT_BT_CTRL, yc_switch, 0, 1);
	//	//0:cb first   1:cr first
	//	lcd_vcbus_setb(VPU_VOUT_BT_CTRL, cr_fst, 1, 1);
	//	//0:left, 1:right, 2:average
	//	lcd_vcbus_setb(VPU_VOUT_BT_CTRL, mode_422, 2, 2);
	//}

	lcd_vcbus_write(VPU_VOUT_DTH_ADDR, 0);
	for (i = 0; i < 32; i++)
		lcd_vcbus_write(VPU_VOUT_DTH_DATA, lcd_dth_lut_c3[i % 16]);

	lcd_vcbus_setb(VPU_VOUT_CORE_CTRL,    field_mode,  1, 1);
	lcd_vcbus_setb(VPU_VOUT_MAX_SIZE,     total_hsize, 16, 13);
	lcd_vcbus_setb(VPU_VOUT_MAX_SIZE,     total_vsize,  0, 13);
	lcd_vcbus_setb(VPU_VOUT_FLD_BGN_LINE, bot_bgn_lne, 16, 13);
	lcd_vcbus_setb(VPU_VOUT_FLD_BGN_LINE, top_bgn_lne,  0, 13);

	lcd_vcbus_setb(VPU_VOUT_HS_POS,      hs_pix_bgn, 16, 13);
	lcd_vcbus_setb(VPU_VOUT_HS_POS,      hs_pix_end,  0, 13);
	lcd_vcbus_setb(VPU_VOUT_VSLN_E_POS,  vs_lne_bgn_e, 16, 13);
	lcd_vcbus_setb(VPU_VOUT_VSLN_E_POS,  vs_lne_end_e,  0, 13);
	lcd_vcbus_setb(VPU_VOUT_VSPX_E_POS,  vs_pix_bgn_e, 16, 13);
	lcd_vcbus_setb(VPU_VOUT_VSPX_E_POS,  vs_pix_end_e,  0, 13);
	lcd_vcbus_setb(VPU_VOUT_VSLN_O_POS,  vs_lne_bgn_o, 16, 13);
	lcd_vcbus_setb(VPU_VOUT_VSLN_O_POS,  vs_lne_end_o,  0, 13);
	lcd_vcbus_setb(VPU_VOUT_VSPX_O_POS,  vs_pix_bgn_o, 16, 13);
	lcd_vcbus_setb(VPU_VOUT_VSPX_O_POS,  vs_pix_end_o,  0, 13);

	lcd_vcbus_setb(VPU_VOUT_DELN_E_POS,  de_lne_bgn_e, 16, 13);
	lcd_vcbus_setb(VPU_VOUT_DELN_E_POS,  de_lne_end_e,  0, 13);
	lcd_vcbus_setb(VPU_VOUT_DELN_O_POS,  de_lne_bgn_o, 16, 13);
	lcd_vcbus_setb(VPU_VOUT_DELN_O_POS,  de_lne_end_o,  0, 13);
	lcd_vcbus_setb(VPU_VOUT_DE_PX_EN,    de_px_bgn_e, 16, 13);
	lcd_vcbus_setb(VPU_VOUT_DE_PX_EN,    de_px_end_e,  0, 13);

	lcd_vcbus_setb(VPU_VOUT_CORE_CTRL, 1, 0, 1); //venc_en
}

static void lcd_venc_enable_c3(struct aml_lcd_drv_s *pdrv, int flag)
{
	if (flag)
		lcd_vcbus_setb(VPU_VOUT_CORE_CTRL, 1, 0, 1);
	else
		lcd_vcbus_setb(VPU_VOUT_CORE_CTRL, 0, 0, 1);
}

static int lcd_get_venc_init_config_c3(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_config_s *pconf = &pdrv->config;
	unsigned int init_state;

	pconf->basic.h_active = lcd_vcbus_getb(VPU_VOUT_DE_PX_EN, 0, 13)
		- lcd_vcbus_getb(VPU_VOUT_DE_PX_EN, 16, 13) + 1;
	pconf->basic.v_active = lcd_vcbus_getb(VPU_VOUT_DELN_E_POS, 0, 13)
		- lcd_vcbus_getb(VPU_VOUT_DELN_E_POS, 16, 13) + 1;
	pconf->basic.h_period = lcd_vcbus_getb(VPU_VOUT_MAX_SIZE, 16, 13);
	pconf->basic.v_period = lcd_vcbus_getb(VPU_VOUT_MAX_SIZE, 0, 13);

	init_state = lcd_vcbus_getb(VPU_VOUT_CORE_CTRL, 0, 1);
	return init_state;
}

struct lcd_venc_op_s lcd_venc_op_c3 = {
	.wait_vsync = NULL,
	.gamma_test_en = NULL,
	.venc_set_timing = NULL,
	.venc_set = lcd_set_venc_c3,
	.venc_change = NULL,
	.venc_enable = lcd_venc_enable_c3,
	.get_venc_init_config = lcd_get_venc_init_config_c3,
};
