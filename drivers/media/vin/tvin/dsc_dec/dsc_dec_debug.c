// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/delay.h>
#include "../tvin_global.h"
#include "dsc_dec_reg.h"
#include "dsc_dec_hw.h"
#include "dsc_dec_drv.h"
#include "dsc_dec_config.h"
#include "dsc_dec_debug.h"
//===========================================================================
// For DSC_DEC Debug
//===========================================================================
static const char *dsc_dec_debug_usage_str = {
"Usage:\n"
"    echo state > /sys/class/aml_dsc_dec/dsc_dec0/status; dump dsc_dec status\n"
"    echo dump_reg > /sys/class/aml_dsc_dec/dsc_dec0/debug; dump dsc_dec register\n"
"    echo read addr >/sys/class/aml_dsc_dec/dsc_dec0/debug\n"
"    echo write addr value >/sys/class/aml_dsc_dec/dsc_dec0/debug\n"
"\n"
};

static ssize_t dsc_dec_debug_help(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", dsc_dec_debug_usage_str);
}

static ssize_t dsc_dec_status_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	ssize_t len = 0;

	return len;
}

static void dsc_dec_parse_param(char *buf_orig, char **parm)
{
	char *ps, *token;
	char delim1[3] = " ";
	char delim2[2] = "\n";
	unsigned int n = 0;

	ps = buf_orig;
	strcat(delim1, delim2);
	while (1) {
		token = strsep(&ps, delim1);
		if (!token)
			break;
		if (*token == '\0')
			continue;
		parm[n++] = token;
	}
}

static inline void dsc_dec_print_state(struct aml_dsc_dec_drv_s *dsc_dec_drv)
{
	int i;

	/* Picture Parameter Set Start*/
	DSC_DEC_PR("------Picture Parameter Set Start------\n");
	DSC_DEC_PR("dsc_dec_version_major:%d\n", dsc_dec_drv->pps_data.dsc_version_major);
	DSC_DEC_PR("dsc_dec_version_minor:%d\n", dsc_dec_drv->pps_data.dsc_version_minor);
	DSC_DEC_PR("pps_identifier:%d\n", dsc_dec_drv->pps_data.pps_identifier);
	DSC_DEC_PR("bits_per_component:%d\n", dsc_dec_drv->pps_data.bits_per_component);
	DSC_DEC_PR("line_buf_depth:%d\n", dsc_dec_drv->pps_data.line_buf_depth);
	DSC_DEC_PR("block_pred_enable:%d\n", dsc_dec_drv->pps_data.block_pred_enable);
	DSC_DEC_PR("convert_rgb:%d\n", dsc_dec_drv->pps_data.convert_rgb);
	DSC_DEC_PR("simple_422:%d\n", dsc_dec_drv->pps_data.simple_422);
	DSC_DEC_PR("vbr_enable:%d\n", dsc_dec_drv->pps_data.vbr_enable);
	DSC_DEC_PR("bits_per_pixel:%d\n", dsc_dec_drv->pps_data.bits_per_pixel);
	DSC_DEC_PR("pic_height:%d\n", dsc_dec_drv->pps_data.pic_height);
	DSC_DEC_PR("pic_width:%d\n", dsc_dec_drv->pps_data.pic_width);
	DSC_DEC_PR("slice_height:%d\n", dsc_dec_drv->pps_data.slice_height);
	DSC_DEC_PR("slice_width:%d\n", dsc_dec_drv->pps_data.slice_width);
	DSC_DEC_PR("chunk_size:%d\n", dsc_dec_drv->pps_data.chunk_size);
	DSC_DEC_PR("initial_xmit_delay:%d\n", dsc_dec_drv->pps_data.initial_xmit_delay);
	DSC_DEC_PR("initial_dec_delay:%d\n", dsc_dec_drv->pps_data.initial_dec_delay);
	DSC_DEC_PR("initial_scale_value:%d\n", dsc_dec_drv->pps_data.initial_scale_value);
	DSC_DEC_PR("scale_increment_interval:%d\n", dsc_dec_drv->pps_data.scale_increment_interval);
	DSC_DEC_PR("scale_decrement_interval:%d\n", dsc_dec_drv->pps_data.scale_decrement_interval);
	DSC_DEC_PR("first_line_bpg_offset:%d\n", dsc_dec_drv->pps_data.first_line_bpg_offset);
	DSC_DEC_PR("nfl_bpg_offset:%d\n", dsc_dec_drv->pps_data.nfl_bpg_offset);
	DSC_DEC_PR("slice_bpg_offset:%d\n", dsc_dec_drv->pps_data.slice_bpg_offset);
	DSC_DEC_PR("initial_offset:%d\n", dsc_dec_drv->pps_data.initial_offset);
	DSC_DEC_PR("final_offset:%d\n", dsc_dec_drv->pps_data.final_offset);
	DSC_DEC_PR("flatness_min_qp:%d\n", dsc_dec_drv->pps_data.flatness_min_qp);
	DSC_DEC_PR("flatness_max_qp:%d\n", dsc_dec_drv->pps_data.flatness_max_qp);
	DSC_DEC_PR("rc_model_size:%d\n", dsc_dec_drv->pps_data.rc_parameter_set.rc_model_size);
	DSC_DEC_PR("rc_edge_factor:%d\n", dsc_dec_drv->pps_data.rc_parameter_set.rc_edge_factor);
	DSC_DEC_PR("rc_quant_incr_limit0:%d\n",
		dsc_dec_drv->pps_data.rc_parameter_set.rc_quant_incr_limit0);
	DSC_DEC_PR("rc_quant_incr_limit1:%d\n",
		dsc_dec_drv->pps_data.rc_parameter_set.rc_quant_incr_limit1);
	DSC_DEC_PR("rc_tgt_offset_hi:%d\n",
		dsc_dec_drv->pps_data.rc_parameter_set.rc_tgt_offset_hi);
	DSC_DEC_PR("rc_tgt_offset_lo:%d\n",
		dsc_dec_drv->pps_data.rc_parameter_set.rc_tgt_offset_lo);
	for (i = 0; i < RC_BUF_THRESH_NUM; i++) {
		DSC_DEC_PR("rc_buf_thresh[%d]:%d(%d)\n",
			i, dsc_dec_drv->pps_data.rc_parameter_set.rc_buf_thresh[i],
			dsc_dec_drv->pps_data.rc_parameter_set.rc_buf_thresh[i] << 6);
	}
	for (i = 0; i < RC_RANGE_PARAMETERS_NUM; i++) {
		DSC_DEC_PR("rc_range_parameters[%d].range_min_qp:%d\n",
		i, dsc_dec_drv->pps_data.rc_parameter_set.rc_range_parameters[i].range_min_qp);
		DSC_DEC_PR("rc_range_parameters[%d].range_max_qp:%d\n",
		i, dsc_dec_drv->pps_data.rc_parameter_set.rc_range_parameters[i].range_max_qp);
		DSC_DEC_PR("rc_range_parameters[%d].range_bpg_offset:%d(%d)\n",
		i, dsc_dec_drv->pps_data.rc_parameter_set.rc_range_parameters[i].range_bpg_offset,
		dsc_dec_drv->pps_data.rc_parameter_set.rc_range_parameters[i].range_bpg_offset &
		0x3f);
	}
	DSC_DEC_PR("native_420:%d\n", dsc_dec_drv->pps_data.native_420);
	DSC_DEC_PR("native_422:%d\n", dsc_dec_drv->pps_data.native_422);
	DSC_DEC_PR("very_flat_qp:%d\n", dsc_dec_drv->very_flat_qp);
	DSC_DEC_PR("somewhat_flat_qp_thresh:%d\n", dsc_dec_drv->somewhat_flat_qp_thresh);
	DSC_DEC_PR("somewhat_flat_qp_delta:%d\n", dsc_dec_drv->somewhat_flat_qp_delta);
	DSC_DEC_PR("second_line_bpg_offset:%d\n", dsc_dec_drv->pps_data.second_line_bpg_offset);
	DSC_DEC_PR("nsl_bpg_offset:%d\n", dsc_dec_drv->pps_data.nsl_bpg_offset);
	DSC_DEC_PR("second_line_offset_adj:%d\n", dsc_dec_drv->pps_data.second_line_offset_adj);
	DSC_DEC_PR("rcb_bits:%d\n", dsc_dec_drv->rcb_bits);
	DSC_DEC_PR("flatness_det_thresh:%d\n", dsc_dec_drv->flatness_det_thresh);
	DSC_DEC_PR("mux_word_size:%d\n", dsc_dec_drv->mux_word_size);
	DSC_DEC_PR("full_ich_err_precision:%d\n", dsc_dec_drv->full_ich_err_precision);
	DSC_DEC_PR("hc_active_bytes:%d\n", dsc_dec_drv->pps_data.hc_active_bytes);
	DSC_DEC_PR("------Picture Parameter Set End------\n");
	DSC_DEC_PR("slice_num_m1:%d\n", dsc_dec_drv->slice_num_m1);
	DSC_DEC_PR("dsc_dec_enc_en:%d\n", dsc_dec_drv->dsc_dec_en);
	DSC_DEC_PR("dsc_dec_enc_frm_latch_en:%d\n", dsc_dec_drv->dsc_dec_frm_latch_en);
	DSC_DEC_PR("pix_per_clk:%d\n", dsc_dec_drv->pix_per_clk);
	DSC_DEC_PR("c3_clk_en:%x\n", dsc_dec_drv->c3_clk_en);
	DSC_DEC_PR("c2_clk_en:%x\n", dsc_dec_drv->c2_clk_en);
	DSC_DEC_PR("c1_clk_en:%x\n", dsc_dec_drv->c1_clk_en);
	DSC_DEC_PR("c0_clk_en:%x\n", dsc_dec_drv->c0_clk_en);
	DSC_DEC_PR("aff_clr:%x\n", dsc_dec_drv->aff_clr);
	DSC_DEC_PR("slices_in_core:%d\n", dsc_dec_drv->slices_in_core);
	DSC_DEC_PR("slice_group_number:%d\n", dsc_dec_drv->slice_group_number);
	DSC_DEC_PR("partial_group_pix_num:%d\n", dsc_dec_drv->partial_group_pix_num);
	DSC_DEC_PR("------timing start------\n");
	DSC_DEC_PR("tmg_havon_begin:%d\n", dsc_dec_drv->tmg_ctrl.tmg_havon_begin);
	DSC_DEC_PR("tmg_hso_begin:%d\n", dsc_dec_drv->tmg_ctrl.tmg_hso_begin);
	DSC_DEC_PR("tmg_hso_end:%d\n", dsc_dec_drv->tmg_ctrl.tmg_hso_end);
	DSC_DEC_PR("tmg_vso_begin:%d\n", dsc_dec_drv->tmg_ctrl.tmg_vso_begin);
	DSC_DEC_PR("tmg_vso_end:%d\n", dsc_dec_drv->tmg_ctrl.tmg_vso_end);
	DSC_DEC_PR("tmg_vso_bline:%d\n", dsc_dec_drv->tmg_ctrl.tmg_vso_bline);
	DSC_DEC_PR("tmg_vso_eline:%d\n", dsc_dec_drv->tmg_ctrl.tmg_vso_eline);
	DSC_DEC_PR("tmg_cb_von_bline:%d\n", dsc_dec_drv->tmg_cb_von_bline);
	DSC_DEC_PR("tmg_cb_von_eline:%d\n", dsc_dec_drv->tmg_cb_von_eline);
	DSC_DEC_PR("------timing end------\n");
	DSC_DEC_PR("recon_jump_depth:%d\n", dsc_dec_drv->recon_jump_depth);
	DSC_DEC_PR("in_swap:%#x\n", dsc_dec_drv->in_swap);
	DSC_DEC_PR("gclk_ctrl:%d\n", dsc_dec_drv->gclk_ctrl);
	DSC_DEC_PR("c0s1_cb_ovfl_th:%d\n", dsc_dec_drv->c0s1_cb_ovfl_th);
	DSC_DEC_PR("c0s0_cb_ovfl_th:%d\n", dsc_dec_drv->c0s0_cb_ovfl_th);
	DSC_DEC_PR("c1s1_cb_ovfl_th:%d\n", dsc_dec_drv->c1s1_cb_ovfl_th);
	DSC_DEC_PR("c1s0_cb_ovfl_th:%d\n", dsc_dec_drv->c1s0_cb_ovfl_th);
	DSC_DEC_PR("c2s1_cb_ovfl_th:%d\n", dsc_dec_drv->c2s1_cb_ovfl_th);
	DSC_DEC_PR("c2s0_cb_ovfl_th:%d\n", dsc_dec_drv->c2s0_cb_ovfl_th);
	DSC_DEC_PR("c3s1_cb_ovfl_th:%d\n", dsc_dec_drv->c3s1_cb_ovfl_th);
	DSC_DEC_PR("s0_de_dly:%d\n", dsc_dec_drv->s0_de_dly);
	DSC_DEC_PR("s1_de_dly:%d\n", dsc_dec_drv->s1_de_dly);
	DSC_DEC_PR("hc_htotal_offs_oddline:%d\n", dsc_dec_drv->hc_htotal_offs_oddline);
	DSC_DEC_PR("hc_htotal_offs_evenline:%d\n", dsc_dec_drv->hc_htotal_offs_evenline);
	DSC_DEC_PR("hc_htotal_m1:%d\n", dsc_dec_drv->hc_htotal_m1);
	DSC_DEC_PR("pix_out_swap0:%#x\n", dsc_dec_drv->pix_out_swap0);
	DSC_DEC_PR("intr_maskn:%d\n", dsc_dec_drv->intr_maskn);
	DSC_DEC_PR("pix_out_swap1:%#x\n", dsc_dec_drv->pix_out_swap1);
	DSC_DEC_PR("clr_bitstream_fetch:%d\n", dsc_dec_drv->clr_bitstream_fetch);
	DSC_DEC_PR("dbg_vcnt:%d\n", dsc_dec_drv->dbg_vcnt);
	DSC_DEC_PR("dbg_hcnt:%d\n", dsc_dec_drv->dbg_hcnt);
	DSC_DEC_PR("manual_set_select:0x%x\n", dsc_dec_drv->dsc_dec_debug.manual_set_select);
	DSC_DEC_PR("dsc_dec_print_en:0x%x\n", dsc_dec_drv->dsc_dec_print_en);
}

static inline void dsc_dec_print_reg_value(struct aml_dsc_dec_drv_s *dsc_dec_drv)
{
	int i;

	/* Picture Parameter Set Start*/
	DSC_DEC_PR("------Picture Parameter Set Start------\n");
	DSC_DEC_PR("dsc_version_major:%d\n", dsc_dec_drv->pps_data.dsc_version_major);
	DSC_DEC_PR("dsc_version_minor:%d\n", R_DSC_DEC_BIT(DSC_COMP_CTRL, DSC_VERSION_MINOR,
		DSC_VERSION_MINOR_WID));
	DSC_DEC_PR("pps_identifier:%d\n", dsc_dec_drv->pps_data.pps_identifier);
	DSC_DEC_PR("bits_per_component:%d\n", R_DSC_DEC_BIT(DSC_COMP_BIT_DEPTH,
		BITS_PER_COMPONENT, BITS_PER_COMPONENT_WID));
	DSC_DEC_PR("line_buf_depth:%d\n", R_DSC_DEC_BIT(DSC_COMP_BIT_DEPTH,
		LINE_BUF_DEPTH, LINE_BUF_DEPTH_WID));
	DSC_DEC_PR("block_pred_enable:%d\n", R_DSC_DEC_BIT(DSC_COMP_CTRL,
		BLOCK_PRED_ENABLE, BLOCK_PRED_ENABLE_WID));
	DSC_DEC_PR("convert_rgb:%d\n", R_DSC_DEC_BIT(DSC_COMP_CTRL, CONVERT_RGB, CONVERT_RGB_WID));
	DSC_DEC_PR("simple_422:%d\n", dsc_dec_drv->pps_data.simple_422);
	DSC_DEC_PR("vbr_enable:%d\n", R_DSC_DEC_BIT(DSC_COMP_CTRL, VBR_ENABLE, VBR_ENABLE_WID));
	DSC_DEC_PR("bits_per_pixel:%d\n", R_DSC_DEC_BIT(DSC_COMP_BIT_DEPTH,
		BITS_PER_PIXEL, BITS_PER_PIXEL_WID));
	DSC_DEC_PR("pic_height:%d\n", R_DSC_DEC_BIT(DSC_COMP_PIC_SIZE,
		PCI_HEIGHT, PCI_HEIGHT_WID));
	DSC_DEC_PR("pic_width:%d\n", R_DSC_DEC_BIT(DSC_COMP_PIC_SIZE, PCI_WIDTH, PCI_WIDTH_WID));
	DSC_DEC_PR("slice_height:%d\n", R_DSC_DEC_BIT(DSC_COMP_SLICE_SIZE,
		SLICE_HEIGHT, SLICE_HEIGHT_WID));
	DSC_DEC_PR("slice_width:%d\n", R_DSC_DEC_BIT(DSC_COMP_SLICE_SIZE,
		SLICE_WIDTH, SLICE_WIDTH_WID));
	DSC_DEC_PR("chunk_size:%d\n", R_DSC_DEC_BIT(DSC_COMP_CHUNK_WORD_SIZE,
		CHUNK_SIZE, CHUNK_SIZE_WID));
	DSC_DEC_PR("initial_xmit_delay:%d\n", R_DSC_DEC_BIT(DSC_COMP_INITIAL_DELAY,
		INITIAL_XMIT_DELAY, INITIAL_XMIT_DELAY_WID));
	DSC_DEC_PR("initial_dec_delay:%d\n", R_DSC_DEC_BIT(DSC_COMP_INITIAL_DELAY,
		INITIAL_DEC_DELAY, INITIAL_DEC_DELAY_WID));
	DSC_DEC_PR("initial_scale_value:%d\n", R_DSC_DEC_BIT(DSC_COMP_INITIAL_OFFSET_SCALE,
		INITIAL_SCALE_VALUE, INITIAL_SCALE_VALUE_WID));
	DSC_DEC_PR("scale_increment_interval:%d\n", R_DSC_DEC_BIT(DSC_COMP_SCALE,
		SCALE_INCREMENT_INTERVAL, SCALE_INCREMENT_INTERVAL_WID));
	DSC_DEC_PR("scale_decrement_interval:%d\n", R_DSC_DEC_BIT(DSC_COMP_SCALE,
		SCALE_DECREMENT_INTERVAL, SCALE_DECREMENT_INTERVAL_WID));
	DSC_DEC_PR("first_line_bpg_offset:%d\n", R_DSC_DEC_BIT(DSC_COMP_FS_BPG_OFS,
		FIRST_LINE_BPG_OFS, FIRST_LINE_BPG_OFS_WID));
	DSC_DEC_PR("nfl_bpg_offset:%d\n", R_DSC_DEC_BIT(DSC_COMP_NFS_BPG_OFFSET,
		NFL_BPG_OFFSET, NFL_BPG_OFFSET_WID));
	DSC_DEC_PR("slice_bpg_offset:%d\n", R_DSC_DEC_BIT(DSC_COMP_SLICE_FINAL_OFFSET,
		SLICE_BPG_OFFSET, SLICE_BPG_OFFSET_WID));
	DSC_DEC_PR("initial_offset:%d\n", R_DSC_DEC_BIT(DSC_COMP_INITIAL_OFFSET_SCALE,
		INITIAL_OFFSET, INITIAL_OFFSET_WID));
	DSC_DEC_PR("final_offset:%d\n", R_DSC_DEC_BIT(DSC_COMP_SLICE_FINAL_OFFSET,
		FINAL_OFFSET, FINAL_OFFSET_WID));
	DSC_DEC_PR("flatness_min_qp:%d\n", R_DSC_DEC_BIT(DSC_COMP_FLATNESS,
		FLATNESS_MIN_QP, FLATNESS_MIN_QP_WID));
	DSC_DEC_PR("flatness_max_qp:%d\n", R_DSC_DEC_BIT(DSC_COMP_FLATNESS,
		FLATNESS_MAX_QP, FLATNESS_MAX_QP_WID));
	DSC_DEC_PR("rc_model_size:%d\n", R_DSC_DEC_BIT(DSC_COMP_RC_BITS_SIZE,
		RC_MODEL_SIZE, RC_MODEL_SIZE_WID));
	DSC_DEC_PR("rc_edge_factor:%d\n", R_DSC_DEC_BIT(DSC_COMP_RC_TGT_QUANT,
		RC_EDGE_FACTOR, RC_EDGE_FACTOR_WID));
	DSC_DEC_PR("rc_quant_incr_limit0:%d\n", R_DSC_DEC_BIT(DSC_COMP_RC_TGT_QUANT,
		RC_QUANT_INCR_LIMIT0, RC_QUANT_INCR_LIMIT0_WID));
	DSC_DEC_PR("rc_quant_incr_limit1:%d\n", R_DSC_DEC_BIT(DSC_COMP_RC_TGT_QUANT,
		RC_QUANT_INCR_LIMIT1, RC_QUANT_INCR_LIMIT1_WID));
	DSC_DEC_PR("rc_tgt_offset_hi:%d\n", R_DSC_DEC_BIT(DSC_COMP_RC_TGT_QUANT,
		RC_TGT_OFFSET_HI, RC_TGT_OFFSET_HI_WID));
	DSC_DEC_PR("rc_tgt_offset_lo:%d\n", R_DSC_DEC_BIT(DSC_COMP_RC_TGT_QUANT,
	RC_TGT_OFFSET_LO, RC_TGT_OFFSET_LO_WID));
	for (i = 0; i < RC_BUF_THRESH_NUM; i++) {
		DSC_DEC_PR("rc_buf_thresh[%d]:%d(%d)\n",
			i, R_DSC_DEC_BIT(DSC_COMP_RC_BUF_THRESH_0 + i,
			RC_BUF_THRESH_0, RC_BUF_THRESH_0_WID) >> 6,
			R_DSC_DEC_BIT(DSC_COMP_RC_BUF_THRESH_0 + i,
			RC_BUF_THRESH_0, RC_BUF_THRESH_0_WID));
	}
	for (i = 0; i < RC_RANGE_PARAMETERS_NUM; i++) {
		DSC_DEC_PR("rc_range_parameters[%d].range_min_qp:%d\n",
				i, R_DSC_DEC_BIT(DSC_COMP_RC_RANGE_0 + i,
					RANGE_MIN_QP_0, RANGE_MIN_QP_0_WID));
		DSC_DEC_PR("rc_range_parameters[%d].range_max_qp:%d\n",
				i, R_DSC_DEC_BIT(DSC_COMP_RC_RANGE_0 + i,
					RANGE_MAX_QP_0, RANGE_MAX_QP_0_WID));
		DSC_DEC_PR("rc_range_parameters[%d].range_bpg_offset:%d(%d-%d)\n",
		i, dsc_dec_drv->pps_data.rc_parameter_set.rc_range_parameters[i].range_bpg_offset,
		R_DSC_DEC_BIT(DSC_COMP_RC_RANGE_0 + i, RANGE_BPG_OFFSET_0, RANGE_BPG_OFFSET_0_WID),
		dsc_dec_drv->pps_data.rc_parameter_set.rc_range_parameters[i].range_bpg_offset &
		0x3f);
	}
	DSC_DEC_PR("native_420:%d\n", R_DSC_DEC_BIT(DSC_COMP_CTRL, NATIVE_420, NATIVE_420_WID));
	DSC_DEC_PR("native_422:%d\n", R_DSC_DEC_BIT(DSC_COMP_CTRL, NATIVE_422, NATIVE_422_WID));
	DSC_DEC_PR("very_flat_qp:%d\n", R_DSC_DEC_BIT(DSC_COMP_FLAT_QP,
		VERY_FLAT_QP, VERY_FLAT_QP_WID));
	DSC_DEC_PR("somewhat_flat_qp_thresh:%d\n", R_DSC_DEC_BIT(DSC_COMP_FLAT_QP,
		SOMEWHAT_FLAT_QP_THRESH, SOMEWHAT_FLAT_QP_THRESH_WID));
	DSC_DEC_PR("somewhat_flat_qp_delta:%d\n", R_DSC_DEC_BIT(DSC_COMP_FLAT_QP,
		SOMEWHAT_FLAT_QP_DELTA, SOMEWHAT_FLAT_QP_DELTA_WID));
	DSC_DEC_PR("second_line_bpg_offset:%d\n", R_DSC_DEC_BIT(DSC_COMP_FS_BPG_OFS,
		SECOND_LINE_BPG_OFS, SECOND_LINE_BPG_OFS_WID));
	DSC_DEC_PR("nsl_bpg_offset:%d\n", R_DSC_DEC_BIT(DSC_COMP_NFS_BPG_OFFSET, NSL_BPG_OFFSET,
		NSL_BPG_OFFSET_WID));
	DSC_DEC_PR("second_line_offset_adj:%d\n", R_DSC_DEC_BIT(DSC_COMP_SEC_OFS_ADJ,
		SECOND_LINE_OFS_ADJ, SECOND_LINE_OFS_ADJ_WID));
	DSC_DEC_PR("rcb_bits:%d\n", R_DSC_DEC_BIT(DSC_COMP_RC_BITS_SIZE, RCB_BITS, RCB_BITS_WID));
	DSC_DEC_PR("flatness_det_thresh:%d\n", R_DSC_DEC_BIT(DSC_COMP_FLATNESS,
		FLATNESS_DET_THRESH, FLATNESS_DET_THRESH_WID));
	DSC_DEC_PR("mux_word_size:%d\n", R_DSC_DEC_BIT(DSC_COMP_CHUNK_WORD_SIZE,
		MUX_WORD_SIZE, MUX_WORD_SIZE_WID));
	DSC_DEC_PR("full_ich_err_precision:%d\n", R_DSC_DEC_BIT(DSC_COMP_CTRL,
		FULL_ICH_ERR_PRECISION, FULL_ICH_ERR_PRECISION_WID));
	DSC_DEC_PR("------Picture Parameter Set End------\n");
	DSC_DEC_PR("slice_num_m1:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL0,
		SLICE_NUM_M1, SLICE_NUM_M1_WID));
	DSC_DEC_PR("dsc_enc_en:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL0, DSC_DEC_EN, DSC_DEC_EN_WID));
	DSC_DEC_PR("dsc_enc_frm_latch_en:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL0,
		DSC_DEC_FRM_LATCH_EN, DSC_DEC_FRM_LATCH_EN_WID));
	DSC_DEC_PR("pix_per_clk:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL0,
		PIX_PER_CLK, PIX_PER_CLK_WID));
	DSC_DEC_PR("c3_clk_en:%x\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL1, C3_CLK_EN, C3_CLK_EN_WID));
	DSC_DEC_PR("c2_clk_en:%x\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL1, C2_CLK_EN, C2_CLK_EN_WID));
	DSC_DEC_PR("c1_clk_en:%x\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL1, C1_CLK_EN, C1_CLK_EN_WID));
	DSC_DEC_PR("c0_clk_en:%x\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL1, C0_CLK_EN, C0_CLK_EN_WID));
	DSC_DEC_PR("aff_clr:%x\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL1, AFF_CLR, AFF_CLR_WID));
	DSC_DEC_PR("slices_in_core:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL1,
		SLICES_IN_CORE, SLICES_IN_CORE_WID));
	DSC_DEC_PR("slice_group_number:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL1,
		SLICES_GROUP_NUMBER, SLICES_GROUP_NUMBER_WID));
	DSC_DEC_PR("partial_group_pix_num:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL2,
		PARTIAL_GROUP_PIX_NUM, PARTIAL_GROUP_PIX_NUM_WID));
	DSC_DEC_PR("------timing start------\n");
	DSC_DEC_PR("tmg_en:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL3, TMG_EN, TMG_EN_WID));
	DSC_DEC_PR("tmg_havon_begin:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL3,
		TMG_HAVON_BEGIN, TMG_HAVON_BEGIN_WID));
	DSC_DEC_PR("tmg_hso_begin:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL4,
		TMG_HSO_BEGIN, TMG_HSO_BEGIN_WID));
	DSC_DEC_PR("tmg_hso_end:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL4, TMG_HSO_END,
		TMG_HSO_END_WID));
	DSC_DEC_PR("tmg_vso_begin:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL5,
		TMG_VSO_BEGIN, TMG_VSO_BEGIN_WID));
	DSC_DEC_PR("tmg_vso_end:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL5,
		TMG_VSO_END, TMG_VSO_END_WID));
	DSC_DEC_PR("tmg_vso_bline:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL6,
		TMG_VSO_BLINE, TMG_VSO_BLINE_WID));
	DSC_DEC_PR("tmg_vso_eline:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL6,
		TMG_VSO_ELINE, TMG_VSO_ELINE_WID));
	DSC_DEC_PR("tmg_cb_von_bline:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL10,
		TMG_CB_VON_BLINE, TMG_CB_VON_BLINE_WID));
	DSC_DEC_PR("tmg_cb_von_eline:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL10,
		TMG_CB_VON_ELINE, TMG_CB_VON_ELINE_WID));
	DSC_DEC_PR("------timing end------\n");
	DSC_DEC_PR("recon_jump_depth:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL7,
		RECON_JUMP_DEPTH, RECON_JUMP_DEPTH_WID));
	DSC_DEC_PR("in_swap:%#x\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL8, IN_SWAP, IN_SWAP_WID));
	DSC_DEC_PR("gclk_ctrl:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL9, GCLK_CTRL, GCLK_CTRL_WID));
	DSC_DEC_PR("clr_cb_sts:%#x\n", R_DSC_DEC_BIT(DSC_ASIC_CTRLA, C0S0_CLR_CB_STS, 8));
	DSC_DEC_PR("c0s1_cb_over_flow_th:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRLA,
		C0S1_CB_OVER_FLOW_TH, C0S1_CB_OVER_FLOW_TH_WID));
	DSC_DEC_PR("c0s0_cb_over_flow_th:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRLA,
		C0S0_CB_OVER_FLOW_TH, C0S0_CB_OVER_FLOW_TH_WID));
	DSC_DEC_PR("c1s1_cb_over_flow_th:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRLB,
		C1S1_CB_OVER_FLOW_TH, C1S1_CB_OVER_FLOW_TH_WID));
	DSC_DEC_PR("c1s0_cb_over_flow_th:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRLB,
		C1S0_CB_OVER_FLOW_TH, C1S0_CB_OVER_FLOW_TH_WID));
	DSC_DEC_PR("c2s1_cb_over_flow_th:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRLC,
		C2S1_CB_OVER_FLOW_TH, C2S1_CB_OVER_FLOW_TH_WID));
	DSC_DEC_PR("c2s0_cb_over_flow_th:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRLC,
		C2S0_CB_OVER_FLOW_TH, C2S0_CB_OVER_FLOW_TH_WID));
	DSC_DEC_PR("c3s1_cb_over_flow_th:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRLD,
		C3S1_CB_OVER_FLOW_TH, C3S1_CB_OVER_FLOW_TH_WID));
	DSC_DEC_PR("c3s0_cb_over_flow_th:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRLD,
		C3S0_CB_OVER_FLOW_TH, C3S0_CB_OVER_FLOW_TH_WID));
	DSC_DEC_PR("cb_under_flow:%#x\n", R_DSC_DEC_BIT(DSC_ASIC_CTRLE,
		C0S0_CB_UNDER_FLOW, 8));
	DSC_DEC_PR("cb_over_flow:%#x\n", R_DSC_DEC_BIT(DSC_ASIC_CTRLE, C0S0_CB_OVER_FLOW, 8));
	DSC_DEC_PR("s0_de_dly:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRLF,
		S0_DE_DLY, S0_DE_DLY_WID));
	DSC_DEC_PR("s1_de_dly:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRLF,
		S1_DE_DLY, S1_DE_DLY_WID));
	DSC_DEC_PR("hc_htotal_offs_oddline:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL11,
		HC_HTOTAL_OFFS_ODDLINE, HC_HTOTAL_OFFS_ODDLINE_WID));
	DSC_DEC_PR("hc_htotal_offs_evenline:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL11,
		HC_HTOTAL_OFFS_EVENLINE, HC_HTOTAL_OFFS_EVENLINE));
	DSC_DEC_PR("hc_htotal_m1:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL11,
		HC_HTOTAL_M1, HC_HTOTAL_M1_WID));
	DSC_DEC_PR("pix_out_swap0:%#x\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL12,
		PIX_OUT_SWAP0, PIX_OUT_SWAP0_WID));
	DSC_DEC_PR("intr_stat:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL13, INTR_STAT, INTR_STAT_WID));
	DSC_DEC_PR("intr_maskn:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL13, INTR_MASKN, INTR_MASKN_WID));
	DSC_DEC_PR("pix_out_swap1:%#x\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL13,
		PIX_OUT_SWAP1, PIX_OUT_SWAP1_WID));
	DSC_DEC_PR("clr_bitstream_fetch:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL14,
		CLR_BITSTREAM_FETCH, CLR_BITSTREAM_FETCH_WID));
	DSC_DEC_PR("dbg_vcnt:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL14, DBG_VCNT, DBG_VCNT_WID));
	DSC_DEC_PR("dbg_hcnt:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL14, DBG_HCNT, DBG_HCNT_WID));
	DSC_DEC_PR("input_hactive:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL15,
		INPUT_HACTIVE, INPUT_HACTIVE_WID));
	DSC_DEC_PR("dbg_de_begin_vcnt:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL15,
		DBG_DE_BEGIN_VCNT, DBG_DE_BEGIN_VCNT_WID));
	DSC_DEC_PR("input_htotal:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL16,
		INPUT_HTOTAL, INPUT_HTOTAL_WID));
	DSC_DEC_PR("bitstream_dat0:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL17,
		BITSTREAM_DAT0, BITSTREAM_DAT0_WID));
	DSC_DEC_PR("bitstream_dat1:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL18,
		BITSTREAM_DAT1, BITSTREAM_DAT1_WID));
}

static inline void dsc_dec_dump_regs(struct aml_dsc_dec_drv_s *dsc_dec_drv)
{
	unsigned int reg;

	DSC_DEC_PR("dsc_dec regs start----\n");
	for (reg = DSC_COMP_CTRL; reg <= DSC_ASIC_CTRL18; reg++)
		DSC_DEC_PR("0x%04x = 0x%08x\n", reg, R_DSC_DEC_REG(reg));
	DSC_DEC_PR("dsc_dec regs end----\n\n");
	DSC_DEC_PR("VPU register config----\n\n");
	DSC_DEC_PR("0x%04x = 0x%08x\n", VPU_VDIN_HDMI0_CTRL0, rd(0, VPU_VDIN_HDMI0_CTRL0));
}

static inline void dsc_dec_clk_dump_regs(struct aml_dsc_dec_drv_s *dsc_dec_drv)
{
	unsigned int reg;

	DSC_DEC_PR("dsc_dec clk start----\n");
	for (reg = CLKCTRL_PIX_PLL_CTRL0; reg <= CLKCTRL_PIX_PLL_STS; reg++)
		DSC_DEC_PR("0x%04x = 0x%08x\n", reg, R_DSC_DEC_CLKCTRL_REG(reg));
	DSC_DEC_PR("dsc_dec clk regs end----\n\n");
}

static inline void dsc_dec_reg_status(void)
{
	DSC_DEC_PR("cb_under_flow:%#x\n", R_DSC_DEC_BIT(DSC_ASIC_CTRLE,
			C0S0_CB_UNDER_FLOW, 8));
	DSC_DEC_PR("cb_over_flow:%#x\n", R_DSC_DEC_BIT(DSC_ASIC_CTRLE,
			C0S0_CB_OVER_FLOW, 8));
	DSC_DEC_PR("dbg_vcnt:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL14, DBG_VCNT, DBG_VCNT_WID));
	DSC_DEC_PR("dbg_hcnt:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL14, DBG_HCNT, DBG_HCNT_WID));
	DSC_DEC_PR("input_hactive:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL15,
		INPUT_HACTIVE, INPUT_HACTIVE_WID));
	DSC_DEC_PR("dbg_de_begin_vcnt:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL15,
		DBG_DE_BEGIN_VCNT, DBG_DE_BEGIN_VCNT_WID));
	DSC_DEC_PR("input_htotal:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL16,
		INPUT_HTOTAL, INPUT_HTOTAL_WID));
	DSC_DEC_PR("bitstream_dat0:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL17,
		BITSTREAM_DAT0, BITSTREAM_DAT0_WID));
	DSC_DEC_PR("bitstream_dat1:%d\n", R_DSC_DEC_BIT(DSC_ASIC_CTRL18,
		BITSTREAM_DAT1, BITSTREAM_DAT1_WID));
}

static inline void set_dsc_tmg_ctrl(struct aml_dsc_dec_drv_s *dsc_dec_drv, char **parm)
{
	int temp = 0;

	if (parm[1] && (kstrtoint(parm[1], 10, &temp) == 0))
		dsc_dec_drv->tmg_ctrl.tmg_havon_begin = temp;

	if (parm[2] && (kstrtoint(parm[2], 10, &temp) == 0))
		dsc_dec_drv->tmg_ctrl.tmg_hso_begin = temp;

	if (parm[3] && (kstrtoint(parm[3], 10, &temp) == 0))
		dsc_dec_drv->tmg_ctrl.tmg_hso_end = temp;

	if (parm[4] && (kstrtoint(parm[4], 10, &temp) == 0))
		dsc_dec_drv->tmg_ctrl.tmg_vso_begin = temp;

	if (parm[5] && (kstrtoint(parm[5], 10, &temp) == 0))
		dsc_dec_drv->tmg_ctrl.tmg_vso_end = temp;

	if (parm[6] && (kstrtoint(parm[6], 10, &temp) == 0))
		dsc_dec_drv->tmg_ctrl.tmg_vso_bline = temp;

	if (parm[7] && (kstrtoint(parm[7], 10, &temp) == 0))
		dsc_dec_drv->tmg_ctrl.tmg_vso_eline = temp;

	if (parm[8] && (kstrtoint(parm[8], 10, &temp) == 0))
		dsc_dec_drv->tmg_cb_von_bline = temp;

	if (parm[9] && (kstrtoint(parm[9], 10, &temp) == 0))
		dsc_dec_drv->tmg_cb_von_eline = temp;

	W_DSC_DEC_BIT(DSC_ASIC_CTRL3, dsc_dec_drv->tmg_ctrl.tmg_havon_begin,
			TMG_HAVON_BEGIN, TMG_HAVON_BEGIN_WID);
	W_DSC_DEC_BIT(DSC_ASIC_CTRL4, dsc_dec_drv->tmg_ctrl.tmg_hso_begin,
			TMG_HSO_BEGIN, TMG_HSO_BEGIN_WID);
	W_DSC_DEC_BIT(DSC_ASIC_CTRL4, dsc_dec_drv->tmg_ctrl.tmg_hso_end,
			TMG_HSO_END, TMG_HSO_END_WID);
	W_DSC_DEC_BIT(DSC_ASIC_CTRL5, dsc_dec_drv->tmg_ctrl.tmg_vso_begin,
			TMG_VSO_BEGIN, TMG_VSO_BEGIN_WID);
	W_DSC_DEC_BIT(DSC_ASIC_CTRL5, dsc_dec_drv->tmg_ctrl.tmg_vso_end,
			TMG_VSO_END, TMG_VSO_END_WID);
	W_DSC_DEC_BIT(DSC_ASIC_CTRL6, dsc_dec_drv->tmg_ctrl.tmg_vso_bline,
			TMG_VSO_BLINE, TMG_VSO_BLINE_WID);
	W_DSC_DEC_BIT(DSC_ASIC_CTRL6, dsc_dec_drv->tmg_ctrl.tmg_vso_eline,
			TMG_VSO_ELINE, TMG_VSO_ELINE_WID);
	W_DSC_DEC_BIT(DSC_ASIC_CTRL10, dsc_dec_drv->tmg_cb_von_bline,
			TMG_CB_VON_BLINE, TMG_CB_VON_BLINE_WID);
	W_DSC_DEC_BIT(DSC_ASIC_CTRL10, dsc_dec_drv->tmg_cb_von_eline,
			TMG_CB_VON_ELINE, TMG_CB_VON_ELINE_WID);

	DSC_DEC_PR("tmg_en:%d\n",
		R_DSC_DEC_BIT(DSC_ASIC_CTRL3, TMG_EN, TMG_EN_WID));
	DSC_DEC_PR("tmg_havon_begin:%d\n",
		R_DSC_DEC_BIT(DSC_ASIC_CTRL3, TMG_HAVON_BEGIN, TMG_HAVON_BEGIN_WID));
	DSC_DEC_PR("tmg_vavon_bline:%d\n",
		R_DSC_DEC_BIT(DSC_ASIC_CTRL4, TMG_HSO_BEGIN, TMG_HSO_BEGIN_WID));
	DSC_DEC_PR("tmg_vavon_eline:%d\n",
		R_DSC_DEC_BIT(DSC_ASIC_CTRL4, TMG_HSO_END, TMG_HSO_END_WID));
	DSC_DEC_PR("tmg_hso_begin:%d\n",
		R_DSC_DEC_BIT(DSC_ASIC_CTRL5, TMG_VSO_BEGIN, TMG_VSO_BEGIN_WID));
	DSC_DEC_PR("tmg_hso_end:%d\n",
		R_DSC_DEC_BIT(DSC_ASIC_CTRL5, TMG_VSO_END, TMG_VSO_END_WID));
	DSC_DEC_PR("tmg_vso_begin:%d\n",
		R_DSC_DEC_BIT(DSC_ASIC_CTRL6, TMG_VSO_BLINE, TMG_VSO_BLINE_WID));
	DSC_DEC_PR("tmg_vso_end:%d\n",
		R_DSC_DEC_BIT(DSC_ASIC_CTRL6, TMG_VSO_ELINE, TMG_VSO_ELINE_WID));
	DSC_DEC_PR("tmg_vso_bline:%d\n",
		R_DSC_DEC_BIT(DSC_ASIC_CTRL10, TMG_CB_VON_BLINE, TMG_CB_VON_BLINE_WID));
	DSC_DEC_PR("tmg_vso_eline:%d\n",
		R_DSC_DEC_BIT(DSC_ASIC_CTRL10, TMG_CB_VON_ELINE, TMG_CB_VON_ELINE_WID));
}

static ssize_t dsc_dec_debug_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t count)
{
	char *buf_orig, *parm[47] = {NULL};
	struct aml_dsc_dec_drv_s *dsc_dec_drv;
	unsigned int temp = 0;
	unsigned int val = 0;

	if (!buf)
		return count;
	buf_orig = kstrdup(buf, GFP_KERNEL);
	dsc_dec_drv = dev_get_drvdata(dev);
	dsc_dec_parse_param(buf_orig, (char **)&parm);

	if (!strcmp(parm[0], "read")) {
		if (parm[1] && (kstrtouint(parm[1], 16, &temp) == 0))
			pr_info("reg:0x%x val:0x%x\n", temp, R_DSC_DEC_REG(temp));
	} else if (!strcmp(parm[0], "write")) {
		if (parm[1] && (kstrtouint(parm[1], 16, &temp) == 0)) {
			if (parm[1] && (kstrtouint(parm[2], 16, &val) == 0))
				W_DSC_DEC_REG(temp, val);
		}
		DSC_DEC_PR("reg:0x%x val:0x%x\n", temp, R_DSC_DEC_REG(temp));
	} else if (!strcmp(parm[0], "state")) {
		dsc_dec_print_state(dsc_dec_drv);
	} else if (!strcmp(parm[0], "dump_reg")) {
		if (parm[1] && (kstrtouint(parm[1], 10, &temp) == 0)) {
			if (temp == 0) { //all register
				dsc_dec_dump_regs(dsc_dec_drv);
				dsc_dec_clk_dump_regs(dsc_dec_drv);
			} else if (temp == 1) { //dsc dec register
				dsc_dec_dump_regs(dsc_dec_drv);
			} else if (temp == 2) { //clk register
				dsc_dec_clk_dump_regs(dsc_dec_drv);
			} else { //dsc dec register
				dsc_dec_dump_regs(dsc_dec_drv);
			}
		} else { //dsc dec register
			dsc_dec_dump_regs(dsc_dec_drv);
			dsc_dec_clk_dump_regs(dsc_dec_drv);
		}
	} else if (!strcmp(parm[0], "get_reg_config")) {
		dsc_dec_print_reg_value(dsc_dec_drv);
	} else if (!strcmp(parm[0], "get_pps_para")) {
		//TODO
	} else if (!strcmp(parm[0], "config_m41h_value_4k120hz")) {
		init_pps_data_4k_120hz(dsc_dec_drv);
	} else if (!strcmp(parm[0], "config_m41h_value_4k60hz")) {
		init_pps_data_4k_60hz(dsc_dec_drv);
	} else if (!strcmp(parm[0], "config_m41h_value_8k30hz")) {
		init_pps_data_8k_30hz(dsc_dec_drv);
	} else if (!strcmp(parm[0], "config_m41h_value_8k60hz_8bpc")) {
		init_pps_data_8k_60hz_8bpc(dsc_dec_drv);
	} else if (!strcmp(parm[0], "config_m41h_value_8k60hz_10bpc")) {
		init_pps_data_8k_60hz_10bpc(dsc_dec_drv);
	} else if (!strcmp(parm[0], "is_enable_dsc_dec")) {
		if (parm[1] && (kstrtouint(parm[1], 10, &temp) == 0))
			set_dsc_dec_en(temp);
		DSC_DEC_PR("is_enable_dsc value:%d\n", temp);
	} else if (!strcmp(parm[0], "manual_dsc_tmg")) {
		set_dsc_tmg_ctrl(dsc_dec_drv, (char **)&parm);
	} else if (!strcmp(parm[0], "clr_dsc_dec_status")) {
		W_DSC_DEC_BIT(DSC_ASIC_CTRLA, 0xff, C0S0_CLR_CB_STS, 8);
		DSC_DEC_PR("clr_dsc_dec_statusing\n");
		W_DSC_DEC_BIT(DSC_ASIC_CTRLA, 0x0, C0S0_CLR_CB_STS, 8);
	} else if (!strcmp(parm[0], "read_dsc_dec_status")) {
		dsc_dec_reg_status();
	} else if (!strcmp(parm[0], "calculate_dsc_dec_clk")) {
		if (parm[1] && (kstrtouint(parm[1], 10, &temp) == 0) &&
		    parm[2] && (kstrtouint(parm[2], 10, &val) == 0)) {
			dsc_dec_clk_calculate(temp, val);
		} else {
			pr_info("calculate_dsc_dec_clk wrong format\n");
		}
	} else {
		pr_info("unknown command\n");
	}

	kfree(buf_orig);
	return count;
}

static struct device_attribute dsc_dec_debug_attrs[] = {
	__ATTR(help, 0444, dsc_dec_debug_help, NULL),
	__ATTR(status, 0444, dsc_dec_status_show, NULL),
	__ATTR(debug, 0644, dsc_dec_debug_help, dsc_dec_debug_store),
};

int dsc_dec_debug_file_create(struct aml_dsc_dec_drv_s *dsc_dec_drv)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(dsc_dec_debug_attrs); i++) {
		if (device_create_file(dsc_dec_drv->dev, &dsc_dec_debug_attrs[i])) {
			DSC_DEC_ERR("create lcd debug attribute %s fail\n",
				dsc_dec_debug_attrs[i].attr.name);
		}
	}

	return 0;
}

int dsc_dec_debug_file_remove(struct aml_dsc_dec_drv_s *dsc_dec_drv)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(dsc_dec_debug_attrs); i++)
		device_remove_file(dsc_dec_drv->dev, &dsc_dec_debug_attrs[i]);
	return 0;
}
