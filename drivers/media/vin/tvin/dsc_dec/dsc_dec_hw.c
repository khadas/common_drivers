// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/amlogic/media/vpu/vpu.h>
#include <linux/amlogic/media/vout/dsc.h>
#include "../tvin_global.h"
#include "dsc_dec_reg.h"
#include "dsc_dec_drv.h"
#include "dsc_dec_debug.h"

unsigned int R_DSC_DEC_CLKCTRL_REG(unsigned int reg)
{
	struct aml_dsc_dec_drv_s *dsc_dec_drv = NULL;

	dsc_dec_drv = dsc_dec_drv_get();
	if (dsc_dec_drv && dsc_dec_drv->dsc_dec_reg_base[0])
		return readl(dsc_dec_drv->dsc_dec_reg_base[0] + (reg << 2));
	return 0;
}

void W_DSC_DEC_CLKCTRL_REG(unsigned int reg, unsigned int val)
{
	struct aml_dsc_dec_drv_s *dsc_dec_drv = NULL;

	dsc_dec_drv = dsc_dec_drv_get();
	if (dsc_dec_drv && dsc_dec_drv->dsc_dec_reg_base[0])
		writel(val, (dsc_dec_drv->dsc_dec_reg_base[0] + (reg << 2)));
}

unsigned int R_DSC_DEC_CLKCTRL_BIT(u32 reg, const u32 start, const u32 len)
{
	return ((R_DSC_DEC_CLKCTRL_REG(reg) >> (start)) & ((1L << (len)) - 1));
}

void W_DSC_DEC_CLKCTRL_BIT(u32 reg, const u32 value, const u32 start, const u32 len)
{
	W_DSC_DEC_CLKCTRL_REG(reg, ((R_DSC_DEC_CLKCTRL_REG(reg) &
			     ~(((1L << (len)) - 1) << (start))) |
			    (((value) & ((1L << (len)) - 1)) << (start))));
}

unsigned int R_DSC_DEC_REG(unsigned int reg)
{
	struct aml_dsc_dec_drv_s *dsc_dec_drv = NULL;

	dsc_dec_drv = dsc_dec_drv_get();
	if (dsc_dec_drv && dsc_dec_drv->dsc_dec_reg_base[1])
		return readl(dsc_dec_drv->dsc_dec_reg_base[1] + (reg << 2));
	return 0;
}

void W_DSC_DEC_REG(unsigned int reg, unsigned int val)
{
	struct aml_dsc_dec_drv_s *dsc_dec_drv = NULL;

	dsc_dec_drv = dsc_dec_drv_get();
	if (dsc_dec_drv && dsc_dec_drv->dsc_dec_reg_base[1])
		writel(val, (dsc_dec_drv->dsc_dec_reg_base[1] + (reg << 2)));
}

unsigned int R_DSC_DEC_BIT(u32 reg, const u32 start, const u32 len)
{
	return ((R_DSC_DEC_REG(reg) >> (start)) & ((1L << (len)) - 1));
}

void W_DSC_DEC_BIT(u32 reg, const u32 value, const u32 start, const u32 len)
{
	W_DSC_DEC_REG(reg, ((R_DSC_DEC_REG(reg) &
			     ~(((1L << (len)) - 1) << (start))) |
			    (((value) & ((1L << (len)) - 1)) << (start))));
}

void set_dsc_dec_en(unsigned int enable)
{
	if (enable) {
		W_DSC_DEC_BIT(DSC_ASIC_CTRL0, 1, DSC_DEC_EN, DSC_DEC_EN_WID);
		W_DSC_DEC_BIT(DSC_ASIC_CTRL3, 1, TMG_EN, TMG_EN_WID);
		W_DSC_DEC_CLKCTRL_REG(CLKCTRL_DSC_CLK_CTRL, 0x1c0);
	} else {
		W_DSC_DEC_BIT(DSC_ASIC_CTRL0, 0, DSC_DEC_EN, DSC_DEC_EN_WID);
		W_DSC_DEC_BIT(DSC_ASIC_CTRL3, 0, TMG_EN, TMG_EN_WID);
		W_DSC_DEC_CLKCTRL_REG(CLKCTRL_DSC_CLK_CTRL, 0x2c0);
	}
}

static void dsc_dec_config_rc_parameter_register(struct dsc_rc_parameter_set *rc_parameter_set)
{
	int i;
	unsigned char range_bpg_offset;

	for (i = 0; i < RC_BUF_THRESH_NUM; i++) {
		W_DSC_DEC_BIT(DSC_COMP_RC_BUF_THRESH_0 + i, rc_parameter_set->rc_buf_thresh[i] << 6,
			RC_BUF_THRESH_0, RC_BUF_THRESH_0_WID);
	}

	for (i = 0; i < RC_RANGE_PARAMETERS_NUM; i++) {
		range_bpg_offset =
			rc_parameter_set->rc_range_parameters[i].range_bpg_offset & 0x3f;
		W_DSC_DEC_BIT(DSC_COMP_RC_RANGE_0 + i, range_bpg_offset,
			RANGE_BPG_OFFSET_0, RANGE_BPG_OFFSET_0_WID);
		W_DSC_DEC_BIT(DSC_COMP_RC_RANGE_0 + i,
			rc_parameter_set->rc_range_parameters[i].range_max_qp,
			RANGE_MAX_QP_0, RANGE_MAX_QP_0_WID);
		W_DSC_DEC_BIT(DSC_COMP_RC_RANGE_0 + i,
			rc_parameter_set->rc_range_parameters[i].range_min_qp,
			RANGE_MIN_QP_0, RANGE_MIN_QP_0_WID);
	}
}

static void dsc_dec_config_timing_register(struct aml_dsc_dec_drv_s *dsc_dec_drv)
{
	struct dsc_timing_gen_ctrl tmg_ctrl = dsc_dec_drv->tmg_ctrl;

	W_DSC_DEC_BIT(DSC_ASIC_CTRL3, tmg_ctrl.tmg_havon_begin,
			TMG_HAVON_BEGIN, TMG_HAVON_BEGIN_WID);
	W_DSC_DEC_BIT(DSC_ASIC_CTRL4, tmg_ctrl.tmg_hso_begin, TMG_HSO_BEGIN, TMG_HSO_BEGIN_WID);
	W_DSC_DEC_BIT(DSC_ASIC_CTRL4, tmg_ctrl.tmg_hso_end, TMG_HSO_END, TMG_HSO_END_WID);

	W_DSC_DEC_BIT(DSC_ASIC_CTRL5, tmg_ctrl.tmg_vso_begin, TMG_VSO_BEGIN, TMG_VSO_BEGIN_WID);
	W_DSC_DEC_BIT(DSC_ASIC_CTRL5, tmg_ctrl.tmg_vso_end, TMG_VSO_END, TMG_VSO_END_WID);

	W_DSC_DEC_BIT(DSC_ASIC_CTRL6, tmg_ctrl.tmg_vso_bline, TMG_VSO_BLINE, TMG_VSO_BLINE_WID);
	W_DSC_DEC_BIT(DSC_ASIC_CTRL6, tmg_ctrl.tmg_vso_eline, TMG_VSO_ELINE, TMG_VSO_ELINE_WID);

	W_DSC_DEC_BIT(DSC_ASIC_CTRL10, dsc_dec_drv->tmg_cb_von_bline,
		TMG_CB_VON_BLINE, TMG_CB_VON_BLINE_WID);
	W_DSC_DEC_BIT(DSC_ASIC_CTRL10, dsc_dec_drv->tmg_cb_von_eline,
		TMG_CB_VON_ELINE, TMG_CB_VON_ELINE_WID);
}

void dsc_dec_config_register(struct aml_dsc_dec_drv_s *dsc_dec_drv)
{
	struct dsc_pps_data_s *pps_data = &dsc_dec_drv->pps_data;

	/* config pps register begin */
	W_DSC_DEC_BIT(DSC_COMP_CTRL, pps_data->native_422, NATIVE_422, NATIVE_422_WID);
	W_DSC_DEC_BIT(DSC_COMP_CTRL, pps_data->native_420, NATIVE_420, NATIVE_420_WID);
	W_DSC_DEC_BIT(DSC_COMP_CTRL, pps_data->convert_rgb, CONVERT_RGB, CONVERT_RGB_WID);
	W_DSC_DEC_BIT(DSC_COMP_CTRL, pps_data->block_pred_enable,
			BLOCK_PRED_ENABLE, BLOCK_PRED_ENABLE_WID);
	W_DSC_DEC_BIT(DSC_COMP_CTRL, pps_data->vbr_enable, VBR_ENABLE, VBR_ENABLE_WID);
	W_DSC_DEC_BIT(DSC_COMP_CTRL, dsc_dec_drv->full_ich_err_precision,
			FULL_ICH_ERR_PRECISION, FULL_ICH_ERR_PRECISION_WID);
	W_DSC_DEC_BIT(DSC_COMP_CTRL, pps_data->dsc_version_minor,
			DSC_VERSION_MINOR, DSC_VERSION_MINOR_WID);

	W_DSC_DEC_BIT(DSC_COMP_PIC_SIZE, pps_data->pic_width, PCI_WIDTH, PCI_WIDTH_WID);
	W_DSC_DEC_BIT(DSC_COMP_PIC_SIZE, pps_data->pic_height, PCI_HEIGHT, PCI_HEIGHT_WID);
	W_DSC_DEC_BIT(DSC_COMP_SLICE_SIZE, pps_data->slice_width, SLICE_WIDTH, SLICE_WIDTH_WID);
	W_DSC_DEC_BIT(DSC_COMP_SLICE_SIZE, pps_data->slice_height, SLICE_HEIGHT, SLICE_HEIGHT_WID);

	W_DSC_DEC_BIT(DSC_COMP_BIT_DEPTH, pps_data->line_buf_depth,
			LINE_BUF_DEPTH, LINE_BUF_DEPTH_WID);
	W_DSC_DEC_BIT(DSC_COMP_BIT_DEPTH, pps_data->bits_per_component,
			BITS_PER_COMPONENT, BITS_PER_COMPONENT_WID);
	W_DSC_DEC_BIT(DSC_COMP_BIT_DEPTH, pps_data->bits_per_pixel,
			BITS_PER_PIXEL, BITS_PER_PIXEL_WID);

	W_DSC_DEC_BIT(DSC_COMP_RC_BITS_SIZE, dsc_dec_drv->rcb_bits, RCB_BITS, RCB_BITS_WID);
	W_DSC_DEC_BIT(DSC_COMP_RC_BITS_SIZE, pps_data->rc_parameter_set.rc_model_size,
			RC_MODEL_SIZE, RC_MODEL_SIZE_WID);
	W_DSC_DEC_BIT(DSC_COMP_RC_TGT_QUANT, pps_data->rc_parameter_set.rc_tgt_offset_hi,
			RC_TGT_OFFSET_HI, RC_TGT_OFFSET_HI_WID);
	W_DSC_DEC_BIT(DSC_COMP_RC_TGT_QUANT, pps_data->rc_parameter_set.rc_tgt_offset_lo,
			RC_TGT_OFFSET_LO, RC_TGT_OFFSET_LO_WID);
	W_DSC_DEC_BIT(DSC_COMP_RC_TGT_QUANT, pps_data->rc_parameter_set.rc_edge_factor,
			RC_EDGE_FACTOR, RC_EDGE_FACTOR_WID);
	W_DSC_DEC_BIT(DSC_COMP_RC_TGT_QUANT, pps_data->rc_parameter_set.rc_quant_incr_limit1,
			RC_QUANT_INCR_LIMIT1, RC_QUANT_INCR_LIMIT1_WID);
	W_DSC_DEC_BIT(DSC_COMP_RC_TGT_QUANT, pps_data->rc_parameter_set.rc_quant_incr_limit0,
			RC_QUANT_INCR_LIMIT0, RC_QUANT_INCR_LIMIT0_WID);

	W_DSC_DEC_BIT(DSC_COMP_INITIAL_DELAY, pps_data->initial_xmit_delay,
			INITIAL_XMIT_DELAY, INITIAL_XMIT_DELAY_WID);
	W_DSC_DEC_BIT(DSC_COMP_INITIAL_DELAY, pps_data->initial_dec_delay,
			INITIAL_DEC_DELAY, INITIAL_DEC_DELAY_WID);
	W_DSC_DEC_BIT(DSC_COMP_INITIAL_OFFSET_SCALE, pps_data->initial_scale_value,
			INITIAL_SCALE_VALUE, INITIAL_SCALE_VALUE_WID);
	W_DSC_DEC_BIT(DSC_COMP_INITIAL_OFFSET_SCALE, pps_data->initial_offset,
			INITIAL_OFFSET, INITIAL_OFFSET_WID);
	W_DSC_DEC_BIT(DSC_COMP_SEC_OFS_ADJ, pps_data->second_line_offset_adj,
			SECOND_LINE_OFS_ADJ, SECOND_LINE_OFS_ADJ_WID);

	W_DSC_DEC_BIT(DSC_COMP_FS_BPG_OFS, pps_data->first_line_bpg_offset,
			FIRST_LINE_BPG_OFS, FIRST_LINE_BPG_OFS_WID);
	W_DSC_DEC_BIT(DSC_COMP_FS_BPG_OFS, pps_data->second_line_bpg_offset,
			SECOND_LINE_BPG_OFS, SECOND_LINE_BPG_OFS_WID);
	W_DSC_DEC_BIT(DSC_COMP_NFS_BPG_OFFSET, pps_data->nfl_bpg_offset,
			NFL_BPG_OFFSET, NFL_BPG_OFFSET_WID);
	W_DSC_DEC_BIT(DSC_COMP_NFS_BPG_OFFSET, pps_data->nsl_bpg_offset,
			NSL_BPG_OFFSET, NSL_BPG_OFFSET_WID);

	dsc_dec_config_rc_parameter_register(&pps_data->rc_parameter_set);

	W_DSC_DEC_BIT(DSC_COMP_FLATNESS, pps_data->flatness_min_qp,
			FLATNESS_MIN_QP, FLATNESS_MIN_QP_WID);
	W_DSC_DEC_BIT(DSC_COMP_FLATNESS, pps_data->flatness_max_qp,
			FLATNESS_MAX_QP, FLATNESS_MAX_QP_WID);
	W_DSC_DEC_BIT(DSC_COMP_SCALE, pps_data->scale_decrement_interval,
			SCALE_DECREMENT_INTERVAL, SCALE_DECREMENT_INTERVAL_WID);
	W_DSC_DEC_BIT(DSC_COMP_SCALE, pps_data->scale_increment_interval,
			SCALE_INCREMENT_INTERVAL, SCALE_INCREMENT_INTERVAL_WID);
	W_DSC_DEC_BIT(DSC_COMP_SLICE_FINAL_OFFSET, pps_data->slice_bpg_offset,
			SLICE_BPG_OFFSET, SLICE_BPG_OFFSET_WID);
	W_DSC_DEC_BIT(DSC_COMP_SLICE_FINAL_OFFSET, pps_data->final_offset,
			FINAL_OFFSET, FINAL_OFFSET_WID);

	W_DSC_DEC_BIT(DSC_COMP_CHUNK_WORD_SIZE, dsc_dec_drv->mux_word_size,
			MUX_WORD_SIZE, MUX_WORD_SIZE_WID);
	W_DSC_DEC_BIT(DSC_COMP_CHUNK_WORD_SIZE, pps_data->chunk_size,
			CHUNK_SIZE, CHUNK_SIZE_WID);
	W_DSC_DEC_BIT(DSC_COMP_FLAT_QP, dsc_dec_drv->very_flat_qp,
			VERY_FLAT_QP, VERY_FLAT_QP_WID);
	W_DSC_DEC_BIT(DSC_COMP_FLAT_QP, dsc_dec_drv->somewhat_flat_qp_delta,
			SOMEWHAT_FLAT_QP_DELTA, SOMEWHAT_FLAT_QP_DELTA_WID);
	W_DSC_DEC_BIT(DSC_COMP_FLAT_QP, dsc_dec_drv->somewhat_flat_qp_thresh,
			SOMEWHAT_FLAT_QP_THRESH, SOMEWHAT_FLAT_QP_THRESH_WID);
	/* config pps register end */

	W_DSC_DEC_BIT(DSC_COMP_FLATNESS, dsc_dec_drv->flatness_det_thresh,
			FLATNESS_DET_THRESH, FLATNESS_DET_THRESH_WID);
	W_DSC_DEC_BIT(DSC_ASIC_CTRL0, dsc_dec_drv->slice_num_m1,
			SLICE_NUM_M1, SLICE_NUM_M1_WID);
	W_DSC_DEC_BIT(DSC_ASIC_CTRL0, dsc_dec_drv->dsc_dec_frm_latch_en,
			DSC_DEC_FRM_LATCH_EN, DSC_DEC_FRM_LATCH_EN_WID);
	W_DSC_DEC_BIT(DSC_ASIC_CTRL0, dsc_dec_drv->pix_per_clk,
			PIX_PER_CLK, PIX_PER_CLK_WID);

	W_DSC_DEC_BIT(DSC_ASIC_CTRL1, dsc_dec_drv->c3_clk_en, C3_CLK_EN, C3_CLK_EN_WID);
	W_DSC_DEC_BIT(DSC_ASIC_CTRL1, dsc_dec_drv->c2_clk_en, C2_CLK_EN, C2_CLK_EN_WID);
	W_DSC_DEC_BIT(DSC_ASIC_CTRL1, dsc_dec_drv->c1_clk_en, C1_CLK_EN, C1_CLK_EN_WID);
	W_DSC_DEC_BIT(DSC_ASIC_CTRL1, dsc_dec_drv->c0_clk_en, C0_CLK_EN, C0_CLK_EN_WID);
	W_DSC_DEC_BIT(DSC_ASIC_CTRL1, dsc_dec_drv->aff_clr, AFF_CLR, AFF_CLR_WID);
	W_DSC_DEC_BIT(DSC_ASIC_CTRL1, dsc_dec_drv->slices_in_core,
			SLICES_IN_CORE, SLICES_IN_CORE_WID);
	W_DSC_DEC_BIT(DSC_ASIC_CTRL1, dsc_dec_drv->slice_group_number,
			SLICES_GROUP_NUMBER, SLICES_GROUP_NUMBER_WID);

	W_DSC_DEC_BIT(DSC_ASIC_CTRL2, dsc_dec_drv->partial_group_pix_num,
			PARTIAL_GROUP_PIX_NUM, PARTIAL_GROUP_PIX_NUM_WID);

	/* config timing register */
	dsc_dec_config_timing_register(dsc_dec_drv);

	W_DSC_DEC_BIT(DSC_ASIC_CTRL7, dsc_dec_drv->recon_jump_depth,
			RECON_JUMP_DEPTH, RECON_JUMP_DEPTH_WID);
	W_DSC_DEC_BIT(DSC_ASIC_CTRL8, dsc_dec_drv->in_swap, IN_SWAP, IN_SWAP_WID);
	W_DSC_DEC_BIT(DSC_ASIC_CTRL9, dsc_dec_drv->gclk_ctrl, GCLK_CTRL, GCLK_CTRL_WID);

	W_DSC_DEC_BIT(DSC_ASIC_CTRLC, dsc_dec_drv->gclk_ctrl,
			GCLK_CTRL, GCLK_CTRL_WID);

	/* config slice overflow threshold value */
	W_DSC_DEC_BIT(DSC_ASIC_CTRLA, dsc_dec_drv->c0s1_cb_ovfl_th,
			C0S1_CB_OVER_FLOW_TH, C0S1_CB_OVER_FLOW_TH_WID);
	W_DSC_DEC_BIT(DSC_ASIC_CTRLA, dsc_dec_drv->c0s0_cb_ovfl_th,
			C0S0_CB_OVER_FLOW_TH, C0S0_CB_OVER_FLOW_TH_WID);
	W_DSC_DEC_BIT(DSC_ASIC_CTRLB, dsc_dec_drv->c1s1_cb_ovfl_th,
			C1S1_CB_OVER_FLOW_TH, C1S1_CB_OVER_FLOW_TH_WID);
	W_DSC_DEC_BIT(DSC_ASIC_CTRLB, dsc_dec_drv->c1s0_cb_ovfl_th,
			C1S0_CB_OVER_FLOW_TH, C1S0_CB_OVER_FLOW_TH_WID);
	W_DSC_DEC_BIT(DSC_ASIC_CTRLC, dsc_dec_drv->c2s1_cb_ovfl_th,
			C2S1_CB_OVER_FLOW_TH, C2S1_CB_OVER_FLOW_TH_WID);
	W_DSC_DEC_BIT(DSC_ASIC_CTRLC, dsc_dec_drv->c2s0_cb_ovfl_th,
			C2S0_CB_OVER_FLOW_TH, C2S0_CB_OVER_FLOW_TH_WID);
	W_DSC_DEC_BIT(DSC_ASIC_CTRLD, dsc_dec_drv->c3s1_cb_ovfl_th,
			C3S1_CB_OVER_FLOW_TH, C3S1_CB_OVER_FLOW_TH_WID);
	W_DSC_DEC_BIT(DSC_ASIC_CTRLD, dsc_dec_drv->c3s0_cb_ovfl_th,
			C3S0_CB_OVER_FLOW_TH, C3S0_CB_OVER_FLOW_TH_WID);

	W_DSC_DEC_BIT(DSC_ASIC_CTRLF, dsc_dec_drv->s0_de_dly,
			S0_DE_DLY, S0_DE_DLY_WID);
	W_DSC_DEC_BIT(DSC_ASIC_CTRLF, dsc_dec_drv->s1_de_dly,
			S1_DE_DLY, S1_DE_DLY_WID);

	W_DSC_DEC_BIT(DSC_ASIC_CTRL11, dsc_dec_drv->hc_htotal_offs_oddline,
			HC_HTOTAL_OFFS_ODDLINE, HC_HTOTAL_OFFS_ODDLINE_WID);
	W_DSC_DEC_BIT(DSC_ASIC_CTRL11, dsc_dec_drv->hc_htotal_offs_evenline,
			HC_HTOTAL_OFFS_EVENLINE, HC_HTOTAL_OFFS_EVENLINE_WID);
	W_DSC_DEC_BIT(DSC_ASIC_CTRL11, dsc_dec_drv->hc_htotal_m1,
			HC_HTOTAL_M1, HC_HTOTAL_M1_WID);

	W_DSC_DEC_BIT(DSC_ASIC_CTRL12, dsc_dec_drv->pix_out_swap0,
			PIX_OUT_SWAP0, PIX_OUT_SWAP0_WID);

	W_DSC_DEC_BIT(DSC_ASIC_CTRL13, dsc_dec_drv->intr_maskn,
			INTR_MASKN, INTR_MASKN_WID);
	W_DSC_DEC_BIT(DSC_ASIC_CTRL13, dsc_dec_drv->pix_out_swap1,
			PIX_OUT_SWAP1, PIX_OUT_SWAP1_WID);

	W_DSC_DEC_BIT(DSC_ASIC_CTRL14, dsc_dec_drv->clr_bitstream_fetch,
			CLR_BITSTREAM_FETCH, CLR_BITSTREAM_FETCH_WID);
	W_DSC_DEC_BIT(DSC_ASIC_CTRL14, dsc_dec_drv->dbg_vcnt, DBG_VCNT, DBG_VCNT_WID);
	W_DSC_DEC_BIT(DSC_ASIC_CTRL14, dsc_dec_drv->dbg_hcnt, DBG_HCNT, DBG_HCNT_WID);
}

void dsc_dec_config_fix_pll_clk(unsigned int value)
{
	if (value == 297) {
		W_DSC_DEC_CLKCTRL_REG(CLKCTRL_PIX_PLL_CTRL0, 0x20020cc6);
		W_DSC_DEC_CLKCTRL_REG(CLKCTRL_PIX_PLL_CTRL0, 0x30020cc6);
		//udelay(20);
		W_DSC_DEC_CLKCTRL_REG(CLKCTRL_PIX_PLL_CTRL1, 0x03a00000);
		W_DSC_DEC_CLKCTRL_REG(CLKCTRL_PIX_PLL_CTRL2, 0x00040000);
		W_DSC_DEC_CLKCTRL_REG(CLKCTRL_PIX_PLL_CTRL3, 0x090da000);
		//udelay(20);
		W_DSC_DEC_CLKCTRL_REG(CLKCTRL_PIX_PLL_CTRL0, 0x10020cc6);
		//udelay(20);
		W_DSC_DEC_CLKCTRL_REG(CLKCTRL_PIX_PLL_CTRL3, 0x090da200);
	} else if (value == 594) {
		W_DSC_DEC_CLKCTRL_REG(CLKCTRL_PIX_PLL_CTRL0, 0x20010cc6);
		W_DSC_DEC_CLKCTRL_REG(CLKCTRL_PIX_PLL_CTRL0, 0x30010cc6);
		//udelay(20);
		W_DSC_DEC_CLKCTRL_REG(CLKCTRL_PIX_PLL_CTRL1, 0x03a00000);
		W_DSC_DEC_CLKCTRL_REG(CLKCTRL_PIX_PLL_CTRL2, 0x00040000);
		W_DSC_DEC_CLKCTRL_REG(CLKCTRL_PIX_PLL_CTRL3, 0x090da000);
		//udelay(20);
		W_DSC_DEC_CLKCTRL_REG(CLKCTRL_PIX_PLL_CTRL0, 0x10010cc6);
		//udelay(20);
		W_DSC_DEC_CLKCTRL_REG(CLKCTRL_PIX_PLL_CTRL3, 0x090da200);
	}
}

void dsc_dec_config_pll_clk(unsigned int od, unsigned int dpll_m,
				unsigned int dpll_n, unsigned int div_frac)
{
	unsigned int config_value  = 0;

	config_value = (dpll_n << 16) | (od << 10) | dpll_m;
	W_DSC_DEC_CLKCTRL_REG(CLKCTRL_PIX_PLL_CTRL0, (2 << 28) | config_value);
	W_DSC_DEC_CLKCTRL_REG(CLKCTRL_PIX_PLL_CTRL0, (3 << 28) | config_value);
	W_DSC_DEC_CLKCTRL_REG(CLKCTRL_PIX_PLL_CTRL1, 0x03a00000 | div_frac);
	W_DSC_DEC_CLKCTRL_REG(CLKCTRL_PIX_PLL_CTRL2, 0x00040000);
	W_DSC_DEC_CLKCTRL_REG(CLKCTRL_PIX_PLL_CTRL3, 0x090da000);
	usleep_range(20, 30);
	W_DSC_DEC_CLKCTRL_REG(CLKCTRL_PIX_PLL_CTRL0, (1 << 28) | config_value);
	usleep_range(20, 30);
	W_DSC_DEC_CLKCTRL_REG(CLKCTRL_PIX_PLL_CTRL3, 0x090da200);
	usleep_range(20, 30);
}

void dsc_dec_config_vpu_mux(struct aml_dsc_dec_drv_s *dsc_dec_drv)
{
	wr_bits(0, VPU_VDIN_HDMI0_CTRL0, 2, HDMI_OR_DSC_EN_BIT, HDMI_OR_DSC_EN_WID);
	wr_bits(0, VPU_VDIN_HDMI0_CTRL0, dsc_dec_drv->pix_per_clk + 1, IN_PPC_BIT, IN_PPC_WID);
	wr_bits(0, VPU_VDIN_HDMI0_CTRL0, 1, OUT_PPC_BIT, OUT_PPC_BIT);

	if (dsc_dec_drv->pps_data.convert_rgb)
		wr_bits(0, VPU_VDIN_HDMI0_CTRL0, 0, DSC_PPC_BIT, DSC_PPC_BIT);
	else if (dsc_dec_drv->pps_data.native_422)
		wr_bits(0, VPU_VDIN_HDMI0_CTRL0, 1, DSC_PPC_BIT, DSC_PPC_BIT);
	else if (dsc_dec_drv->pps_data.native_420)
		wr_bits(0, VPU_VDIN_HDMI0_CTRL0, 3, DSC_PPC_BIT, DSC_PPC_BIT);
	else
		wr_bits(0, VPU_VDIN_HDMI0_CTRL0, 2, DSC_PPC_BIT, DSC_PPC_BIT);
}

