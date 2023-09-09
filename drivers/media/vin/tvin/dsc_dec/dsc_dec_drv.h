/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __DSC_DEC_DRV_H__
#define __DSC_DEC_DRV_H__

#include <linux/amlogic/media/vout/dsc.h>
#include <linux/cdev.h>

#define DSC_DEC_NORMAL_DEBUG		BIT(0)
#define DSC_DEC_MAP_MAX			2

struct dsc_dec_cdev_s {
	dev_t           devno;
	struct class    *class;
};

enum dsc_dec_chip_e {
	DSC_DEC_CHIP_T3X = 0,
	DSC_DEC_CHIP_MAX,
};

struct dsc_dec_data_s {
	enum dsc_dec_chip_e chip_type;
	const char *chip_name;
};

struct dsc_timing_gen_ctrl {
	unsigned int tmg_havon_begin;//avon=active
	unsigned int tmg_hso_begin;//hso=hsync
	unsigned int tmg_hso_end; //havon_begin - hback/2
	unsigned int tmg_vso_begin;
	unsigned int tmg_vso_end;
	unsigned int tmg_vso_bline;
	unsigned int tmg_vso_eline;
};

/*******for debug **********/
struct dsc_dec_debug_s {
	unsigned int manual_set_select;
};

struct aml_dsc_dec_drv_s {
	struct cdev cdev;
	struct dsc_dec_data_s *data;
	struct device *dev;
	void __iomem *dsc_dec_reg_base[DSC_DEC_MAP_MAX];//[0]:clk base [1]:dsc enc module base

	struct dsc_dec_debug_s dsc_dec_debug;
	//PPS parameter start
	struct dsc_pps_data_s pps_data;
	unsigned int flatness_det_thresh;
	bool full_ich_err_precision;
	unsigned int rcb_bits;
	unsigned int mux_word_size;
	unsigned int very_flat_qp;
	unsigned int somewhat_flat_qp_delta;
	unsigned int somewhat_flat_qp_thresh;
	//PPS parameter end
	unsigned int slice_num_m1;//pic_width/slice_width;
	unsigned int dsc_dec_en;
	unsigned int dsc_dec_frm_latch_en;// need to check ucode
	unsigned int pix_per_clk;//input 0:1pix 1:2pix 2:4pix
	bool c3_clk_en;
	bool c2_clk_en;
	bool c1_clk_en;
	bool c0_clk_en;
	bool aff_clr;
	bool slices_in_core;//if slice_num_m1==8 enable
	unsigned int slice_group_number;//444:slice_width/3*slice_height
					//422/420:slice_width/2/3*slice_height
	unsigned int partial_group_pix_num;//register only [14:15] const value check ucode
	struct dsc_timing_gen_ctrl tmg_ctrl;
	unsigned int recon_jump_depth;
	unsigned int in_swap;
	unsigned int gclk_ctrl;//0x0a0a0a0a is enable clk
	unsigned int c0s1_cb_ovfl_th;//(8/h_slice_num)*350
	unsigned int c0s0_cb_ovfl_th;
	unsigned int c1s1_cb_ovfl_th;
	unsigned int c1s0_cb_ovfl_th;
	unsigned int c2s1_cb_ovfl_th;
	unsigned int c2s0_cb_ovfl_th;
	unsigned int c3s1_cb_ovfl_th;
	unsigned int c3s0_cb_ovfl_th;
	unsigned int s0_de_dly;
	unsigned int s1_de_dly; //use constant 33
	unsigned int tmg_cb_von_bline; //vsync + vbackporch + a little delay
	unsigned int tmg_cb_von_eline; //cb_von_bline + vactive_number
	unsigned int hc_htotal_offs_oddline;
	unsigned int hc_htotal_offs_evenline;
	unsigned int hc_htotal_m1; //compress htotal from HDMI RX (hc_active+hc_blank)/2 - 1
	unsigned int pix_out_swap0;
	unsigned int intr_maskn;
	unsigned int pix_out_swap1;
	unsigned int clr_bitstream_fetch;
	unsigned int dbg_vcnt;
	unsigned int dbg_hcnt;
	unsigned int dsc_dec_print_en;
};

//===========================================================================
// For ENCL DSC_DEC
//===========================================================================
#define DSC_DEC_ENC_INDEX                              0

struct aml_dsc_dec_drv_s *dsc_dec_drv_get(void);

#endif
