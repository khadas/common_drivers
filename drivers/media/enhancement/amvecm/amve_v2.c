// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/string.h>
#include <linux/spinlock.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
/* #include <mach/am_regs.h> */
#include <linux/amlogic/media/utils/amstream.h>
/* #include <linux/amlogic/aml_common.h> */
/* media module used media/registers/cpu_version.h since kernel 5.4 */
#include <linux/amlogic/media/registers/cpu_version.h>
#include <linux/amlogic/media/vfm/vframe.h>
#include <linux/amlogic/media/video_sink/video.h>
#include <linux/amlogic/media/amvecm/amvecm.h>
#include <linux/amlogic/media/vout/vinfo.h>
#include <linux/amlogic/media/vout/vout_notify.h>
#include <linux/amlogic/media/vout/lcd/lcd_unifykey.h>
#include <linux/amlogic/media/utils/amstream.h>
#include "arch/vpp_regs_v2.h"
#include "amve_v2.h"
#include <linux/io.h>
#include "reg_helper.h"
#include "local_contrast.h"
#include "am_dma_ctrl.h"
#include "set_hdr2_v0.h"

static int multi_slice_case;
module_param(multi_slice_case, int, 0644);
MODULE_PARM_DESC(multi_slice_case, "multi_slice_case after t3x");

static int vev2_dbg;
module_param(vev2_dbg, int, 0644);
MODULE_PARM_DESC(vev2_dbg, "ve dbg after s5");

#define pr_amve_v2(fmt, args...)\
	do {\
		if (vev2_dbg & 0x1)\
			pr_info("AMVE: " fmt, ## args);\
	} while (0)\

/*ve module slice1~slice3 offset*/
unsigned int ve_reg_ofst[3] = {
	0x0, 0x100, 0x200
};

unsigned int pst_reg_ofst[4] = {
	0x0, 0x100, 0x700, 0x1900
};

/*sr sharpness module slice0~slice1 offset*/
unsigned int sr_sharp_reg_ofst[2] = {
	0x0, 0x100
};

/*lc curve module slice0~slice1 offset*/
unsigned int lc_reg_ofst[2] = {
	0x0, 0x40
};

/*cm module slice0~slice1 offset*/
unsigned int cm_reg_ofst[2] = {
	0x0, 0xb00
};

int get_slice_max(void)
{
	if (chip_type_id == chip_s5)
		return SLICE_MAX;
	else if (chip_type_id == chip_t3x)
		return SLICE2;
	else
		return SLICE1;
}

static void ve_brightness_cfg(int val,
	enum wr_md_e mode, enum vadj_index_e vadj_idx,
	enum vpp_slice_e slice)
{
	int reg;
	int slice_max;

	slice_max = get_slice_max();
	if (slice >= slice_max)
		return;

	switch (vadj_idx) {
	case VE_VADJ1:
		if (slice == SLICE0)
			reg = VPP_VADJ1_Y;
		else
			reg = VPP_SLICE1_VADJ1_Y + ve_reg_ofst[slice - 1];
		break;
	case VE_VADJ2:
		reg = VPP_VADJ2_Y + pst_reg_ofst[slice];
		break;
	default:
		break;
	}

	if (mode == WR_VCB)
		WRITE_VPP_REG_BITS(reg, val, 8, 11);
	else if (mode == WR_DMA)
		VSYNC_WRITE_VPP_REG_BITS(reg, val, 8, 11);

	pr_amve_v2("brigtness: val = %d, slice = %d", val, slice);
}

#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
static void ve_contrast_cfg(int val,
	enum wr_md_e mode, enum vadj_index_e vadj_idx,
	enum vpp_slice_e slice)
{
	int reg;
	int slice_max;

	slice_max = get_slice_max();

	if (slice >= slice_max)
		return;

	switch (vadj_idx) {
	case VE_VADJ1:
		if (slice == SLICE0)
			reg = VPP_VADJ1_Y;
		else
			reg = VPP_SLICE1_VADJ1_Y + ve_reg_ofst[slice - 1];
		break;
	case VE_VADJ2:
		reg = VPP_VADJ2_Y + pst_reg_ofst[slice];
		break;
	default:
		break;
	}

	if (mode == WR_VCB)
		WRITE_VPP_REG_BITS(reg, val, 0, 8);
	else if (mode == WR_DMA)
		VSYNC_WRITE_VPP_REG_BITS(reg, val, 0, 8);

	pr_amve_v2("contrast: val = %d, slice = %d", val, slice);
}
#endif

static void ve_sat_hue_mab_cfg(int mab,
	enum wr_md_e mode, enum vadj_index_e vadj_idx,
	enum vpp_slice_e slice)
{
	int reg_mab;
	//int reg_mcd;
	int slice_max;

	slice_max = get_slice_max();

	if (slice >= slice_max)
		return;

	switch (vadj_idx) {
	case VE_VADJ1:
		if (slice == SLICE0) {
			reg_mab = VPP_VADJ1_MA_MB;
			//reg_mcd = VPP_VADJ1_MC_MD;
		} else {
			reg_mab = VPP_SLICE1_VADJ1_MA_MB + ve_reg_ofst[slice - 1];
			//reg_mcd = VPP_SLICE1_VADJ1_MC_MD + ve_reg_ofst[slice - 1];
		}
		break;
	case VE_VADJ2:
		reg_mab = VPP_VADJ2_MA_MB + pst_reg_ofst[slice];
		//reg_mcd = VPP_VADJ2_MC_MD + pst_reg_ofst[slice];
		break;
	default:
		break;
	}

	if (mode == WR_VCB)
		WRITE_VPP_REG(reg_mab, mab);
	else if (mode == WR_DMA)
		VSYNC_WRITE_VPP_REG(reg_mab, mab);

	pr_amve_v2("sat_hue: mab = %d, slice = %d", mab, slice);
}

static void ve_sat_hue_mcd_cfg(int mcd,
	enum wr_md_e mode, enum vadj_index_e vadj_idx,
	enum vpp_slice_e slice)
{
	//int reg_mab;
	int reg_mcd;
	int slice_max;

	slice_max = get_slice_max();

	if (slice >= slice_max)
		return;

	switch (vadj_idx) {
	case VE_VADJ1:
		if (slice == SLICE0) {
			//reg_mab = VPP_VADJ1_MA_MB;
			reg_mcd = VPP_VADJ1_MC_MD;
		} else {
			//reg_mab = VPP_SLICE1_VADJ1_MA_MB + ve_reg_ofst[slice - 1];
			reg_mcd = VPP_SLICE1_VADJ1_MC_MD + ve_reg_ofst[slice - 1];
		}
		break;
	case VE_VADJ2:
		//reg_mab = VPP_VADJ2_MA_MB + pst_reg_ofst[slice];
		reg_mcd = VPP_VADJ2_MC_MD + pst_reg_ofst[slice];
		break;
	default:
		break;
	}

	if (mode == WR_VCB)
		WRITE_VPP_REG(reg_mcd, mcd);
	else if (mode == WR_DMA)
		VSYNC_WRITE_VPP_REG(reg_mcd, mcd);

	pr_amve_v2("sat_hue: mcd = %d, slice = %d", mcd, slice);
}

void ve_brigtness_set(int val,
	enum vadj_index_e vadj_idx,
	enum wr_md_e mode)
{
	int i;
	int slice_max;

	slice_max = get_slice_max();

	for (i = SLICE0; i < slice_max; i++)
		ve_brightness_cfg(val, mode, vadj_idx, i);
}

#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
void ve_contrast_set(int val,
	enum vadj_index_e vadj_idx,
	enum wr_md_e mode)
{
	int i;
	int slice_max;

	slice_max = get_slice_max();

	for (i = SLICE0; i < slice_max; i++)
		ve_contrast_cfg(val, mode, vadj_idx, i);
}
#endif

void ve_color_mab_set(int mab,
	enum vadj_index_e vadj_idx,
	enum wr_md_e mode)
{
	int i;
	int slice_max;

	slice_max = get_slice_max();

	for (i = SLICE0; i < slice_max; i++)
		ve_sat_hue_mab_cfg(mab, mode, vadj_idx, i);
}

void ve_color_mcd_set(int mcd,
	enum vadj_index_e vadj_idx,
	enum wr_md_e mode)
{
	int i;
	int slice_max;

	slice_max = get_slice_max();

	for (i = SLICE0; i < slice_max; i++)

		ve_sat_hue_mcd_cfg(mcd, mode, vadj_idx, i);
}

int ve_brightness_contrast_get(enum vadj_index_e vadj_idx)
{
	int val = 0;

	if (vadj_idx == VE_VADJ1)
		val = READ_VPP_REG(VPP_VADJ1_Y);
	else if (vadj_idx == VE_VADJ2)
		val = READ_VPP_REG(VPP_VADJ2_Y);
	return val;
}

void vpp_mtx_config_v2(struct matrix_coef_s *coef,
	enum wr_md_e mode, enum vpp_slice_e slice,
	enum vpp_matrix_e mtx_sel)
{
	int reg_pre_offset0_1 = 0;
	int reg_pre_offset2 = 0;
	int reg_coef00_01 = 0;
	int reg_coef02_10;
	int reg_coef11_12;
	int reg_coef20_21;
	int reg_coef22;
	int reg_offset0_1 = 0;
	int reg_offset2 = 0;
	int reg_en_ctl = 0;
	int slice_max;

	slice_max = get_slice_max();

	if (slice >= slice_max)
		return;

	switch (slice) {
	case SLICE0:
		if (mtx_sel == VD1_MTX) {
			reg_pre_offset0_1 = VPP_VD1_MATRIX_COEF00_01;
			reg_pre_offset2 = VPP_VD1_MATRIX_PRE_OFFSET2;
			reg_coef00_01 = VPP_VD1_MATRIX_COEF00_01;
			reg_coef02_10 = VPP_VD1_MATRIX_COEF02_10;
			reg_coef11_12 = VPP_VD1_MATRIX_COEF11_12;
			reg_coef20_21 = VPP_VD1_MATRIX_COEF20_21;
			reg_coef22 = VPP_VD1_MATRIX_COEF22;
			reg_offset0_1 = VPP_VD1_MATRIX_OFFSET0_1;
			reg_offset2 = VPP_VD1_MATRIX_OFFSET2;
			reg_en_ctl = VPP_VD1_MATRIX_EN_CTRL;
		} else if (mtx_sel == POST2_MTX) {
			reg_pre_offset0_1 = VPP_POST2_MATRIX_COEF00_01;
			reg_pre_offset2 = VPP_POST2_MATRIX_PRE_OFFSET2;
			reg_coef00_01 = VPP_POST2_MATRIX_COEF00_01;
			reg_coef02_10 = VPP_POST2_MATRIX_COEF02_10;
			reg_coef11_12 = VPP_POST2_MATRIX_COEF11_12;
			reg_coef20_21 = VPP_POST2_MATRIX_COEF20_21;
			reg_coef22 = VPP_POST2_MATRIX_COEF22;
			reg_offset0_1 = VPP_POST2_MATRIX_OFFSET0_1;
			reg_offset2 = VPP_POST2_MATRIX_OFFSET2;
			reg_en_ctl = VPP_POST2_MATRIX_EN_CTRL;
		} else if (mtx_sel == POST_MTX) {
			reg_pre_offset0_1 = VPP_POST_MATRIX_COEF00_01;
			reg_pre_offset2 = VPP_POST_MATRIX_PRE_OFFSET2;
			reg_coef00_01 = VPP_POST_MATRIX_COEF00_01;
			reg_coef02_10 = VPP_POST_MATRIX_COEF02_10;
			reg_coef11_12 = VPP_POST_MATRIX_COEF11_12;
			reg_coef20_21 = VPP_POST_MATRIX_COEF20_21;
			reg_coef22 = VPP_POST_MATRIX_COEF22;
			reg_offset0_1 = VPP_POST_MATRIX_OFFSET0_1;
			reg_offset2 = VPP_POST_MATRIX_OFFSET2;
			reg_en_ctl = VPP_POST_MATRIX_EN_CTRL;
		}
		break;
	case SLICE1:
	case SLICE2:
	case SLICE3:
		if (mtx_sel == VD1_MTX) {
			reg_pre_offset0_1 = VPP_SLICE1_VD1_MATRIX_PRE_OFFSET0_1 +
				ve_reg_ofst[slice - 1];
			reg_pre_offset2 = VPP_SLICE1_VD1_MATRIX_PRE_OFFSET2 +
				ve_reg_ofst[slice - 1];
			reg_coef00_01 = VPP_SLICE1_VD1_MATRIX_COEF00_01 +
				ve_reg_ofst[slice - 1];
			reg_coef02_10 = VPP_SLICE1_VD1_MATRIX_COEF02_10 +
				ve_reg_ofst[slice - 1];
			reg_coef11_12 = VPP_SLICE1_VD1_MATRIX_COEF11_12 +
				ve_reg_ofst[slice - 1];
			reg_coef20_21 = VPP_SLICE1_VD1_MATRIX_COEF20_21 +
				ve_reg_ofst[slice - 1];
			reg_coef22 = VPP_SLICE1_VD1_MATRIX_COEF22 +
				ve_reg_ofst[slice - 1];
			reg_offset0_1 = VPP_SLICE1_VD1_MATRIX_OFFSET0_1 +
				ve_reg_ofst[slice - 1];
			reg_offset2 = VPP_SLICE1_VD1_MATRIX_OFFSET2 +
				ve_reg_ofst[slice - 1];
			reg_en_ctl = VPP_SLICE1_VD1_MATRIX_EN_CTRL +
				ve_reg_ofst[slice - 1];
		} else if (mtx_sel == POST2_MTX) {
			reg_pre_offset0_1 = VPP_POST2_MATRIX_COEF00_01 +
				pst_reg_ofst[slice];
			reg_pre_offset2 = VPP_POST2_MATRIX_PRE_OFFSET2 +
				pst_reg_ofst[slice];
			reg_coef00_01 = VPP_POST2_MATRIX_COEF00_01 +
				pst_reg_ofst[slice];
			reg_coef02_10 = VPP_POST2_MATRIX_COEF02_10 +
				pst_reg_ofst[slice];
			reg_coef11_12 = VPP_POST2_MATRIX_COEF11_12 +
				pst_reg_ofst[slice];
			reg_coef20_21 = VPP_POST2_MATRIX_COEF20_21 +
				pst_reg_ofst[slice];
			reg_coef22 = VPP_POST2_MATRIX_COEF22 +
				pst_reg_ofst[slice];
			reg_offset0_1 = VPP_POST2_MATRIX_OFFSET0_1 +
				pst_reg_ofst[slice];
			reg_offset2 = VPP_POST2_MATRIX_OFFSET2 +
				pst_reg_ofst[slice];
			reg_en_ctl = VPP_POST2_MATRIX_EN_CTRL +
				pst_reg_ofst[slice];
		} else if (mtx_sel == POST_MTX) {
			reg_pre_offset0_1 = VPP_POST_MATRIX_COEF00_01 +
				pst_reg_ofst[slice];
			reg_pre_offset2 = VPP_POST_MATRIX_PRE_OFFSET2 +
				pst_reg_ofst[slice];
			reg_coef00_01 = VPP_POST_MATRIX_COEF00_01 +
				pst_reg_ofst[slice];
			reg_coef02_10 = VPP_POST_MATRIX_COEF02_10 +
				pst_reg_ofst[slice];
			reg_coef11_12 = VPP_POST_MATRIX_COEF11_12 +
				pst_reg_ofst[slice];
			reg_coef20_21 = VPP_POST_MATRIX_COEF20_21 +
				pst_reg_ofst[slice];
			reg_coef22 = VPP_POST_MATRIX_COEF22 +
				pst_reg_ofst[slice];
			reg_offset0_1 = VPP_POST_MATRIX_OFFSET0_1 +
				pst_reg_ofst[slice];
			reg_offset2 = VPP_POST_MATRIX_OFFSET2 +
				pst_reg_ofst[slice];
			reg_en_ctl = VPP_POST_MATRIX_EN_CTRL +
				pst_reg_ofst[slice];
		}
		break;
	default:
		break;
	}

	switch (mode) {
	case WR_VCB:
		WRITE_VPP_REG(reg_pre_offset0_1,
			(coef->pre_offset[0] << 16) | coef->pre_offset[1]);
		WRITE_VPP_REG(reg_pre_offset2, coef->pre_offset[2]);
		WRITE_VPP_REG(reg_coef00_01,
			(coef->matrix_coef[0][0] << 16) | coef->matrix_coef[0][1]);
		WRITE_VPP_REG(reg_coef00_01,
			(coef->matrix_coef[0][2] << 16) | coef->matrix_coef[1][0]);
		WRITE_VPP_REG(reg_coef00_01,
			(coef->matrix_coef[1][1] << 16) | coef->matrix_coef[1][2]);
		WRITE_VPP_REG(reg_coef00_01,
			(coef->matrix_coef[2][0] << 16) | coef->matrix_coef[2][1]);
		WRITE_VPP_REG(reg_coef00_01, coef->matrix_coef[2][2]);
		WRITE_VPP_REG(reg_offset0_1,
			(coef->post_offset[0] << 16) | coef->post_offset[1]);
		WRITE_VPP_REG(reg_offset2, coef->post_offset[2]);
		WRITE_VPP_REG_BITS(reg_en_ctl, coef->en, 0, 1);
		break;
	case WR_DMA:
		VSYNC_WRITE_VPP_REG(reg_pre_offset0_1,
			(coef->pre_offset[0] << 16) | coef->pre_offset[1]);
		VSYNC_WRITE_VPP_REG(reg_pre_offset2, coef->pre_offset[2]);
		VSYNC_WRITE_VPP_REG(reg_coef00_01,
			(coef->matrix_coef[0][0] << 16) | coef->matrix_coef[0][1]);
		VSYNC_WRITE_VPP_REG(reg_coef00_01,
			(coef->matrix_coef[0][2] << 16) | coef->matrix_coef[1][0]);
		VSYNC_WRITE_VPP_REG(reg_coef00_01,
			(coef->matrix_coef[1][1] << 16) | coef->matrix_coef[1][2]);
		VSYNC_WRITE_VPP_REG(reg_coef00_01,
			(coef->matrix_coef[2][0] << 16) | coef->matrix_coef[2][1]);
		VSYNC_WRITE_VPP_REG(reg_coef00_01, coef->matrix_coef[2][2]);
		VSYNC_WRITE_VPP_REG(reg_offset0_1,
			(coef->post_offset[0] << 16) | coef->post_offset[1]);
		VSYNC_WRITE_VPP_REG(reg_offset2, coef->post_offset[2]);
		VSYNC_WRITE_VPP_REG_BITS(reg_en_ctl, coef->en, 0, 1);
		break;
	default:
		break;
	}
}

void cm_top_ctl(enum wr_md_e mode, int en)
{
	int i;
	int slice_max;

	slice_max = get_slice_max();

	switch (mode) {
	case WR_VCB:
		WRITE_VPP_REG_BITS(VPP_VE_ENABLE_CTRL, en, 4, 1);
		for (i = SLICE1; i < slice_max; i++)
			WRITE_VPP_REG_BITS(VPP_SLICE1_VE_ENABLE_CTRL + i - SLICE1,
				en, 4, 1);
		break;
	case WR_DMA:
		VSYNC_WRITE_VPP_REG_BITS(VPP_VE_ENABLE_CTRL, en, 4, 1);
		for (i = SLICE1; i < slice_max; i++)
			VSYNC_WRITE_VPP_REG_BITS(VPP_SLICE1_VE_ENABLE_CTRL + i - SLICE1,
				en, 4, 1);
		break;
	default:
		break;
	}
}

struct cm_port_s get_cm_port(void)
{
	struct cm_port_s port;

	port.cm_addr_port[0] = VPP_CHROMA_ADDR_PORT;
	port.cm_data_port[0] = VPP_CHROMA_DATA_PORT;
	port.cm_addr_port[1] = VPP_SLICE1_CHROMA_ADDR_PORT;
	port.cm_data_port[1] = VPP_SLICE1_CHROMA_DATA_PORT;
	port.cm_addr_port[2] = VPP_SLICE2_CHROMA_ADDR_PORT;
	port.cm_data_port[2] = VPP_SLICE2_CHROMA_DATA_PORT;
	port.cm_addr_port[3] = VPP_SLICE3_CHROMA_ADDR_PORT;
	port.cm_data_port[3] = VPP_SLICE3_CHROMA_DATA_PORT;

	return port;
}

/*modules after post matrix*/
void post_gainoff_cfg(struct tcon_rgb_ogo_s *p,
	enum wr_md_e mode, enum vpp_slice_e slice)
{
	unsigned int reg_ctl0;
	unsigned int reg_ctl1;
	unsigned int reg_ctl2;
	unsigned int reg_ctl3;
	unsigned int reg_ctl4;

	reg_ctl0 = VPP_GAINOFF_CTRL0 + pst_reg_ofst[slice];
	reg_ctl1 = VPP_GAINOFF_CTRL1 + pst_reg_ofst[slice];
	reg_ctl2 = VPP_GAINOFF_CTRL2 + pst_reg_ofst[slice];
	reg_ctl3 = VPP_GAINOFF_CTRL3 + pst_reg_ofst[slice];
	reg_ctl4 = VPP_GAINOFF_CTRL4 + pst_reg_ofst[slice];

	if (mode == WR_VCB) {
		WRITE_VPP_REG(reg_ctl0,
			((p->en << 31) & 0x80000000) |
			((p->r_gain << 16) & 0x07ff0000) |
			((p->g_gain <<  0) & 0x000007ff));
		WRITE_VPP_REG(reg_ctl1,
			((p->b_gain << 16) & 0x07ff0000) |
			((p->r_post_offset <<  0) & 0x00001fff));
		WRITE_VPP_REG(reg_ctl2,
			((p->g_post_offset << 16) & 0x1fff0000) |
			((p->b_post_offset <<  0) & 0x00001fff));
		WRITE_VPP_REG(reg_ctl3,
			((p->r_pre_offset  << 16) & 0x1fff0000) |
			((p->g_pre_offset  <<  0) & 0x00001fff));
		WRITE_VPP_REG(reg_ctl4,
			((p->b_pre_offset  <<  0) & 0x00001fff));
	} else if (mode == WR_DMA) {
		VSYNC_WRITE_VPP_REG(reg_ctl0,
			((p->en << 31) & 0x80000000) |
			((p->r_gain << 16) & 0x07ff0000) |
			((p->g_gain <<	0) & 0x000007ff));
		VSYNC_WRITE_VPP_REG(reg_ctl1,
			((p->b_gain << 16) & 0x07ff0000) |
			((p->r_post_offset <<  0) & 0x00001fff));
		VSYNC_WRITE_VPP_REG(reg_ctl2,
			((p->g_post_offset << 16) & 0x1fff0000) |
			((p->b_post_offset <<  0) & 0x00001fff));
		VSYNC_WRITE_VPP_REG(reg_ctl3,
			((p->r_pre_offset  << 16) & 0x1fff0000) |
			((p->g_pre_offset  <<  0) & 0x00001fff));
		VSYNC_WRITE_VPP_REG(reg_ctl4,
			((p->b_pre_offset  <<  0) & 0x00001fff));
	}

	pr_amve_v2("go_en: %d, slice = %d",
		p->en, slice);
	pr_amve_v2("go_gain: %d, %d, %d",
		p->r_gain, p->g_gain, p->b_gain);
	pr_amve_v2("go_pre_offset: %d, %d, %d",
		p->r_pre_offset, p->g_pre_offset, p->b_pre_offset);
	pr_amve_v2("go_post_offset: %d, %d, %d",
		p->r_post_offset, p->g_post_offset, p->b_post_offset);
}

void post_gainoff_set(struct tcon_rgb_ogo_s *p,
	enum wr_md_e mode)
{
	int i;
	int slice_max;

	slice_max = get_slice_max();

	for (i = SLICE0; i < slice_max; i++)
		post_gainoff_cfg(p, mode, i);
}

void post_pre_gamma_set(int *lut)
{
	int i, j;
	unsigned int ctl_port;
	unsigned int addr_port;
	unsigned int data_port;
	int slice_max;

	slice_max = get_slice_max();

	for (j = SLICE0; j < slice_max; j++) {
		ctl_port = VPP_GAMMA_CTRL + pst_reg_ofst[j];
		addr_port = VPP_GAMMA_BIN_ADDR + pst_reg_ofst[j];
		data_port = VPP_GAMMA_BIN_DATA + pst_reg_ofst[j];
		WRITE_VPP_REG(addr_port, 0);
		for (i = 0; i < 33; i = i + 1)
			WRITE_VPP_REG(data_port,
				      (((lut[i * 2 + 1] << 2) & 0xffff) << 16 |
				      ((lut[i * 2] << 2) & 0xffff)));
		for (i = 0; i < 33; i = i + 1)
			WRITE_VPP_REG(data_port,
				      (((lut[i * 2 + 1] << 2) & 0xffff) << 16 |
				      ((lut[i * 2] << 2) & 0xffff)));
		for (i = 0; i < 33; i = i + 1)
			WRITE_VPP_REG(data_port,
				      (((lut[i * 2 + 1] << 2) & 0xffff) << 16 |
				      ((lut[i * 2] << 2) & 0xffff)));
		WRITE_VPP_REG_BITS(ctl_port, 0x3, 0, 2);
	}
}

/*vpp module enable/disable control*/
void ve_vadj_ctl(enum wr_md_e mode, enum vadj_index_e vadj_idx, int en)
{
	int i;
	int slice_max;

	slice_max = get_slice_max();

	switch (mode) {
	case WR_VCB:
		if (vadj_idx == VE_VADJ1) {
			WRITE_VPP_REG_BITS(VPP_VADJ1_MISC, en, 0, 1);
			for (i = SLICE1; i < slice_max; i++)
				WRITE_VPP_REG_BITS(VPP_SLICE1_VADJ1_MISC + ve_reg_ofst[i - 1],
					en, 0, 1);
		} else if (vadj_idx == VE_VADJ2) {
			for (i = SLICE0; i < slice_max; i++)
				WRITE_VPP_REG_BITS(VPP_VADJ2_MISC + pst_reg_ofst[i],
					en, 0, 1);
		}
		break;
	case WR_DMA:
		if (vadj_idx == VE_VADJ1) {
			VSYNC_WRITE_VPP_REG_BITS(VPP_VADJ1_MISC, en, 0, 1);
			for (i = SLICE1; i < slice_max; i++)
				VSYNC_WRITE_VPP_REG_BITS(VPP_SLICE1_VADJ1_MISC +
					ve_reg_ofst[i - 1], en, 0, 1);
		} else if (vadj_idx == VE_VADJ2) {
			for (i = SLICE0; i < slice_max; i++)
				VSYNC_WRITE_VPP_REG_BITS(VPP_VADJ2_MISC +
					pst_reg_ofst[i], en, 0, 1);
		}
		break;
	default:
		break;
	}

	pr_amve_v2("vadj_ctl: en = %d, vadj_idx = %d", en, vadj_idx);
}

/*blue stretch can only use on slice0 on s5*/
void ve_bs_ctl(enum wr_md_e mode, int en)
{
	int i;
	int slice_max;
	unsigned int reg_bs_0 = VPP_BLUE_STRETCH_1;
	unsigned int reg_bs_0_slice1 = VPP_SLICE1_BLUE_STRETCH_1;

	if (chip_type_id == chip_t3x) {
		reg_bs_0 = 0x1d78;
		reg_bs_0_slice1 = 0x2878;
	}

	slice_max = get_slice_max();

	if (mode == WR_VCB) {
		WRITE_VPP_REG_BITS(reg_bs_0, en, 31, 1);
		if (chip_type_id == chip_t3x) {
			for (i = SLICE1; i < slice_max; i++)
				WRITE_VPP_REG_BITS(reg_bs_0_slice1 +
					ve_reg_ofst[i - 1], en, 31, 1);
		}
	} else if (mode == WR_DMA) {
		VSYNC_WRITE_VPP_REG_BITS(reg_bs_0, en, 31, 1);
		if (chip_type_id == chip_t3x) {
			for (i = SLICE1; i < slice_max; i++)
				VSYNC_WRITE_VPP_REG_BITS(reg_bs_0_slice1 +
					ve_reg_ofst[i - 1], en, 31, 1);
		}
	}
}

void ve_ble_ctl(enum wr_md_e mode, int en)
{
	int i;
	int slice_max;

	slice_max = get_slice_max();

	if (mode == WR_VCB) {
		WRITE_VPP_REG_BITS(VPP_VE_ENABLE_CTRL, en, 3, 1);
		for (i = SLICE1; i < slice_max; i++)
			WRITE_VPP_REG_BITS(VPP_SLICE1_VE_ENABLE_CTRL +
				ve_reg_ofst[i - 1], en, 3, 1);
	} else if (mode == WR_DMA) {
		VSYNC_WRITE_VPP_REG_BITS(VPP_VE_ENABLE_CTRL, en, 3, 1);
		for (i = SLICE1; i < slice_max; i++)
			VSYNC_WRITE_VPP_REG_BITS(VPP_SLICE1_VE_ENABLE_CTRL +
				ve_reg_ofst[i - 1], en, 3, 1);
	}
}

void ve_cc_ctl(enum wr_md_e mode, int en)
{
	int i;
	int slice_max;

	slice_max = get_slice_max();

	if (mode == WR_VCB) {
		WRITE_VPP_REG_BITS(VPP_VE_ENABLE_CTRL, en, 1, 1);
		for (i = SLICE1; i < slice_max; i++)
			WRITE_VPP_REG_BITS(VPP_SLICE1_VE_ENABLE_CTRL +
				ve_reg_ofst[i - 1], en, 1, 1);
	} else if (mode == WR_DMA) {
		VSYNC_WRITE_VPP_REG_BITS(VPP_VE_ENABLE_CTRL, en, 1, 1);
		for (i = SLICE1; i < slice_max; i++)
			VSYNC_WRITE_VPP_REG_BITS(VPP_SLICE1_VE_ENABLE_CTRL +
				ve_reg_ofst[i - 1], en, 1, 1);
	}
}

void post_wb_ctl(enum wr_md_e mode, int en)
{
	int i;
	int slice_max;

	slice_max = get_slice_max();

	if (mode == WR_VCB) {
		for (i = SLICE0; i < slice_max; i++)
			WRITE_VPP_REG_BITS(VPP_GAINOFF_CTRL0 +
				pst_reg_ofst[i], en, 31, 1);
	} else if (mode == WR_DMA) {
		for (i = SLICE0; i < slice_max; i++)
			VSYNC_WRITE_VPP_REG_BITS(VPP_GAINOFF_CTRL0 +
				pst_reg_ofst[i], en, 31, 1);
	}
}

void post_pre_gamma_ctl(enum wr_md_e mode, int en)
{
	int i;
	int slice_max;

	slice_max = get_slice_max();

	if (mode == WR_VCB) {
		for (i = SLICE0; i < slice_max; i++)
			WRITE_VPP_REG_BITS(VPP_GAMMA_CTRL +
				pst_reg_ofst[i], en, 0, 1);
	} else if (mode == WR_DMA) {
		for (i = SLICE0; i < slice_max; i++)
			VSYNC_WRITE_VPP_REG_BITS(VPP_GAMMA_CTRL +
				pst_reg_ofst[i], en, 0, 1);
	}
}

void vpp_luma_hist_init(void)
{
	WRITE_VPP_REG_BITS(VI_HIST_CTRL, 2, 5, 3);
	/*select slice0,  all selection: vpp post, slc0~slc3,vd2, osd1,osd2,vd1 post*/
	WRITE_VPP_REG_BITS(VI_HIST_CTRL, 1, 11, 3);
	WRITE_VPP_REG_BITS(VI_HIST_CTRL, 0, 2, 1);
	/*full picture*/
	WRITE_VPP_REG_BITS(VI_HIST_CTRL, 0, 1, 1);
	/*enable*/
	WRITE_VPP_REG_BITS(VI_HIST_CTRL, 1, 0, 1);
}

void get_luma_hist(struct vframe_s *vf)
{
	static int pre_w, pre_h;
	int width, height;

	width = vf->width;
	height = vf->height;

	if (pre_w != width || pre_h != height) {
		WRITE_VPP_REG(VI_HIST_PIC_SIZE, width | (height << 16));
		pre_w = width;
		pre_h = height;
	}

	vf->prop.hist.vpp_luma_sum = READ_VPP_REG(VI_HIST_SPL_VAL);
	vf->prop.hist.vpp_pixel_sum = READ_VPP_REG(VI_HIST_SPL_PIX_CNT);
	vf->prop.hist.vpp_chroma_sum = READ_VPP_REG(VI_HIST_CHROMA_SUM);
	vf->prop.hist.vpp_height     =
	READ_VPP_REG_BITS(VI_HIST_PIC_SIZE,
			  VI_HIST_PIC_HEIGHT_BIT, VI_HIST_PIC_HEIGHT_WID);
	vf->prop.hist.vpp_width      =
	READ_VPP_REG_BITS(VI_HIST_PIC_SIZE,
			  VI_HIST_PIC_WIDTH_BIT, VI_HIST_PIC_WIDTH_WID);
	vf->prop.hist.vpp_luma_max   =
	READ_VPP_REG_BITS(VI_HIST_MAX_MIN,
			  VI_HIST_MAX_BIT, VI_HIST_MAX_WID);
	vf->prop.hist.vpp_luma_min   =
	READ_VPP_REG_BITS(VI_HIST_MAX_MIN,
			  VI_HIST_MIN_BIT, VI_HIST_MIN_WID);
	vf->prop.hist.vpp_gamma[0]   =
	READ_VPP_REG_BITS(VI_DNLP_HIST00,
			  VI_HIST_ON_BIN_00_BIT, VI_HIST_ON_BIN_00_WID);
	vf->prop.hist.vpp_gamma[1]   =
	READ_VPP_REG_BITS(VI_DNLP_HIST00,
			  VI_HIST_ON_BIN_01_BIT, VI_HIST_ON_BIN_01_WID);
	vf->prop.hist.vpp_gamma[2]   =
	READ_VPP_REG_BITS(VI_DNLP_HIST01,
			  VI_HIST_ON_BIN_02_BIT, VI_HIST_ON_BIN_02_WID);
	vf->prop.hist.vpp_gamma[3]   =
	READ_VPP_REG_BITS(VI_DNLP_HIST01,
			  VI_HIST_ON_BIN_03_BIT, VI_HIST_ON_BIN_03_WID);
	vf->prop.hist.vpp_gamma[4]   =
	READ_VPP_REG_BITS(VI_DNLP_HIST02,
			  VI_HIST_ON_BIN_04_BIT, VI_HIST_ON_BIN_04_WID);
	vf->prop.hist.vpp_gamma[5]   =
	READ_VPP_REG_BITS(VI_DNLP_HIST02,
			  VI_HIST_ON_BIN_05_BIT, VI_HIST_ON_BIN_05_WID);
	vf->prop.hist.vpp_gamma[6]   =
	READ_VPP_REG_BITS(VI_DNLP_HIST03,
			  VI_HIST_ON_BIN_06_BIT, VI_HIST_ON_BIN_06_WID);
	vf->prop.hist.vpp_gamma[7]   =
	READ_VPP_REG_BITS(VI_DNLP_HIST03,
			  VI_HIST_ON_BIN_07_BIT, VI_HIST_ON_BIN_07_WID);
	vf->prop.hist.vpp_gamma[8]   =
	READ_VPP_REG_BITS(VI_DNLP_HIST04,
			  VI_HIST_ON_BIN_08_BIT, VI_HIST_ON_BIN_08_WID);
	vf->prop.hist.vpp_gamma[9]   =
	READ_VPP_REG_BITS(VI_DNLP_HIST04,
			  VI_HIST_ON_BIN_09_BIT, VI_HIST_ON_BIN_09_WID);
	vf->prop.hist.vpp_gamma[10]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST05,
			  VI_HIST_ON_BIN_10_BIT, VI_HIST_ON_BIN_10_WID);
	vf->prop.hist.vpp_gamma[11]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST05,
			  VI_HIST_ON_BIN_11_BIT, VI_HIST_ON_BIN_11_WID);
	vf->prop.hist.vpp_gamma[12]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST06,
			  VI_HIST_ON_BIN_12_BIT, VI_HIST_ON_BIN_12_WID);
	vf->prop.hist.vpp_gamma[13]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST06,
			  VI_HIST_ON_BIN_13_BIT, VI_HIST_ON_BIN_13_WID);
	vf->prop.hist.vpp_gamma[14]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST07,
			  VI_HIST_ON_BIN_14_BIT, VI_HIST_ON_BIN_14_WID);
	vf->prop.hist.vpp_gamma[15]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST07,
			  VI_HIST_ON_BIN_15_BIT, VI_HIST_ON_BIN_15_WID);
	vf->prop.hist.vpp_gamma[16]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST08,
			  VI_HIST_ON_BIN_16_BIT, VI_HIST_ON_BIN_16_WID);
	vf->prop.hist.vpp_gamma[17]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST08,
			  VI_HIST_ON_BIN_17_BIT, VI_HIST_ON_BIN_17_WID);
	vf->prop.hist.vpp_gamma[18]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST09,
			  VI_HIST_ON_BIN_18_BIT, VI_HIST_ON_BIN_18_WID);
	vf->prop.hist.vpp_gamma[19]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST09,
			  VI_HIST_ON_BIN_19_BIT, VI_HIST_ON_BIN_19_WID);
	vf->prop.hist.vpp_gamma[20]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST10,
			  VI_HIST_ON_BIN_20_BIT, VI_HIST_ON_BIN_20_WID);
	vf->prop.hist.vpp_gamma[21]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST10,
			  VI_HIST_ON_BIN_21_BIT, VI_HIST_ON_BIN_21_WID);
	vf->prop.hist.vpp_gamma[22]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST11,
			  VI_HIST_ON_BIN_22_BIT, VI_HIST_ON_BIN_22_WID);
	vf->prop.hist.vpp_gamma[23]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST11,
			  VI_HIST_ON_BIN_23_BIT, VI_HIST_ON_BIN_23_WID);
	vf->prop.hist.vpp_gamma[24]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST12,
			  VI_HIST_ON_BIN_24_BIT, VI_HIST_ON_BIN_24_WID);
	vf->prop.hist.vpp_gamma[25]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST12,
			  VI_HIST_ON_BIN_25_BIT, VI_HIST_ON_BIN_25_WID);
	vf->prop.hist.vpp_gamma[26]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST13,
			  VI_HIST_ON_BIN_26_BIT, VI_HIST_ON_BIN_26_WID);
	vf->prop.hist.vpp_gamma[27]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST13,
			  VI_HIST_ON_BIN_27_BIT, VI_HIST_ON_BIN_27_WID);
	vf->prop.hist.vpp_gamma[28]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST14,
			  VI_HIST_ON_BIN_28_BIT, VI_HIST_ON_BIN_28_WID);
	vf->prop.hist.vpp_gamma[29]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST14,
			  VI_HIST_ON_BIN_29_BIT, VI_HIST_ON_BIN_29_WID);
	vf->prop.hist.vpp_gamma[30]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST15,
			  VI_HIST_ON_BIN_30_BIT, VI_HIST_ON_BIN_30_WID);
	vf->prop.hist.vpp_gamma[31]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST15,
			  VI_HIST_ON_BIN_31_BIT, VI_HIST_ON_BIN_31_WID);
	vf->prop.hist.vpp_gamma[32]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST16,
			  VI_HIST_ON_BIN_32_BIT, VI_HIST_ON_BIN_32_WID);
	vf->prop.hist.vpp_gamma[33]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST16,
			  VI_HIST_ON_BIN_33_BIT, VI_HIST_ON_BIN_33_WID);
	vf->prop.hist.vpp_gamma[34]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST17,
			  VI_HIST_ON_BIN_34_BIT, VI_HIST_ON_BIN_34_WID);
	vf->prop.hist.vpp_gamma[35]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST17,
			  VI_HIST_ON_BIN_35_BIT, VI_HIST_ON_BIN_35_WID);
	vf->prop.hist.vpp_gamma[36]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST18,
			  VI_HIST_ON_BIN_36_BIT, VI_HIST_ON_BIN_36_WID);
	vf->prop.hist.vpp_gamma[37]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST18,
			  VI_HIST_ON_BIN_37_BIT, VI_HIST_ON_BIN_37_WID);
	vf->prop.hist.vpp_gamma[38]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST19,
			  VI_HIST_ON_BIN_38_BIT, VI_HIST_ON_BIN_38_WID);
	vf->prop.hist.vpp_gamma[39]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST19,
			  VI_HIST_ON_BIN_39_BIT, VI_HIST_ON_BIN_39_WID);
	vf->prop.hist.vpp_gamma[40]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST20,
			  VI_HIST_ON_BIN_40_BIT, VI_HIST_ON_BIN_40_WID);
	vf->prop.hist.vpp_gamma[41]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST20,
			  VI_HIST_ON_BIN_41_BIT, VI_HIST_ON_BIN_41_WID);
	vf->prop.hist.vpp_gamma[42]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST21,
			  VI_HIST_ON_BIN_42_BIT, VI_HIST_ON_BIN_42_WID);
	vf->prop.hist.vpp_gamma[43]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST21,
			  VI_HIST_ON_BIN_43_BIT, VI_HIST_ON_BIN_43_WID);
	vf->prop.hist.vpp_gamma[44]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST22,
			  VI_HIST_ON_BIN_44_BIT, VI_HIST_ON_BIN_44_WID);
	vf->prop.hist.vpp_gamma[45]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST22,
			  VI_HIST_ON_BIN_45_BIT, VI_HIST_ON_BIN_45_WID);
	vf->prop.hist.vpp_gamma[46]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST23,
			  VI_HIST_ON_BIN_46_BIT, VI_HIST_ON_BIN_46_WID);
	vf->prop.hist.vpp_gamma[47]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST23,
			  VI_HIST_ON_BIN_47_BIT, VI_HIST_ON_BIN_47_WID);
	vf->prop.hist.vpp_gamma[48]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST24,
			  VI_HIST_ON_BIN_48_BIT, VI_HIST_ON_BIN_48_WID);
	vf->prop.hist.vpp_gamma[49]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST24,
			  VI_HIST_ON_BIN_49_BIT, VI_HIST_ON_BIN_49_WID);
	vf->prop.hist.vpp_gamma[50]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST25,
			  VI_HIST_ON_BIN_50_BIT, VI_HIST_ON_BIN_50_WID);
	vf->prop.hist.vpp_gamma[51]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST25,
			  VI_HIST_ON_BIN_51_BIT, VI_HIST_ON_BIN_51_WID);
	vf->prop.hist.vpp_gamma[52]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST26,
			  VI_HIST_ON_BIN_52_BIT, VI_HIST_ON_BIN_52_WID);
	vf->prop.hist.vpp_gamma[53]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST26,
			  VI_HIST_ON_BIN_53_BIT, VI_HIST_ON_BIN_53_WID);
	vf->prop.hist.vpp_gamma[54]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST27,
			  VI_HIST_ON_BIN_54_BIT, VI_HIST_ON_BIN_54_WID);
	vf->prop.hist.vpp_gamma[55]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST27,
			  VI_HIST_ON_BIN_55_BIT, VI_HIST_ON_BIN_55_WID);
	vf->prop.hist.vpp_gamma[56]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST28,
			  VI_HIST_ON_BIN_56_BIT, VI_HIST_ON_BIN_56_WID);
	vf->prop.hist.vpp_gamma[57]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST28,
			  VI_HIST_ON_BIN_57_BIT, VI_HIST_ON_BIN_57_WID);
	vf->prop.hist.vpp_gamma[58]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST29,
			  VI_HIST_ON_BIN_58_BIT, VI_HIST_ON_BIN_58_WID);
	vf->prop.hist.vpp_gamma[59]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST29,
			  VI_HIST_ON_BIN_59_BIT, VI_HIST_ON_BIN_59_WID);
	vf->prop.hist.vpp_gamma[60]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST30,
			  VI_HIST_ON_BIN_60_BIT, VI_HIST_ON_BIN_60_WID);
	vf->prop.hist.vpp_gamma[61]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST30,
			  VI_HIST_ON_BIN_61_BIT, VI_HIST_ON_BIN_61_WID);
	vf->prop.hist.vpp_gamma[62]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST31,
			  VI_HIST_ON_BIN_62_BIT, VI_HIST_ON_BIN_62_WID);
	vf->prop.hist.vpp_gamma[63]  =
	READ_VPP_REG_BITS(VI_DNLP_HIST31,
			  VI_HIST_ON_BIN_63_BIT, VI_HIST_ON_BIN_63_WID);
}

void ve_multi_slice_case_set(int enable)
{
	multi_slice_case = enable;
	pr_amve_v2("%s: multi_slice_case = %d", __func__, enable);
}

int ve_multi_slice_case_get(void)
{
	return multi_slice_case;
}

void ve_vadj_misc_set(int val,
	enum wr_md_e mode, enum vadj_index_e vadj_idx,
	enum vpp_slice_e slice, int start, int len)
{
	int reg;
	int slice_max;

	slice_max = get_slice_max();
	if (slice >= slice_max)
		return;

	switch (vadj_idx) {
	case VE_VADJ1:
		if (slice == SLICE0)
			reg = VPP_VADJ1_MISC;
		else
			reg = VPP_SLICE1_VADJ1_MISC + ve_reg_ofst[slice - 1];
		break;
	case VE_VADJ2:
		reg = VPP_VADJ2_MISC + pst_reg_ofst[slice];
		break;
	default:
		break;
	}

	if (mode == WR_VCB)
		WRITE_VPP_REG_BITS(reg, val, start, len);
	else if (mode == WR_DMA)
		VSYNC_WRITE_VPP_REG_BITS(reg, val, start, len);
}

int ve_vadj_misc_get(enum vadj_index_e vadj_idx,
	enum vpp_slice_e slice, int start, int len)
{
	int reg;
	int slice_max;

	slice_max = get_slice_max();
	if (slice >= slice_max)
		return 0;

	switch (vadj_idx) {
	case VE_VADJ1:
		if (slice == SLICE0)
			reg = VPP_VADJ1_MISC;
		else
			reg = VPP_SLICE1_VADJ1_MISC + ve_reg_ofst[slice - 1];
		break;
	case VE_VADJ2:
		reg = VPP_VADJ2_MISC + pst_reg_ofst[slice];
		break;
	default:
		break;
	}

	return READ_VPP_REG_BITS(reg, start, len);
}

void ve_mtrx_setting(enum vpp_matrix_e mtx_sel,
	int mtx_csc, int mtx_on, enum vpp_slice_e slice)
{
	unsigned int matrix_coef00_01 = 0;
	unsigned int matrix_coef02_10 = 0;
	unsigned int matrix_coef11_12 = 0;
	unsigned int matrix_coef20_21 = 0;
	unsigned int matrix_coef22 = 0;
	unsigned int matrix_coef13_14 = 0;
	unsigned int matrix_coef23_24 = 0;
	unsigned int matrix_coef15_25 = 0;
	unsigned int matrix_clip = 0;
	unsigned int matrix_offset0_1 = 0;
	unsigned int matrix_offset2 = 0;
	unsigned int matrix_pre_offset0_1 = 0;
	unsigned int matrix_pre_offset2 = 0;
	unsigned int matrix_en_ctrl = 0;
	int vpp_sel = VPP_TOP0;
	int offset = 0x100;

	if (mtx_sel == VD1_MTX) {
		matrix_coef00_01 = VPP_VD1_MATRIX_COEF00_01;
		matrix_coef02_10 = VPP_VD1_MATRIX_COEF02_10;
		matrix_coef11_12 = VPP_VD1_MATRIX_COEF11_12;
		matrix_coef20_21 = VPP_VD1_MATRIX_COEF20_21;
		matrix_coef22 = VPP_VD1_MATRIX_COEF22;
		matrix_coef13_14 = VPP_VD1_MATRIX_COEF13_14;
		matrix_coef23_24 = VPP_VD1_MATRIX_COEF23_24;
		matrix_coef15_25 = VPP_VD1_MATRIX_COEF15_25;
		matrix_clip = VPP_VD1_MATRIX_CLIP;
		matrix_offset0_1 = VPP_VD1_MATRIX_OFFSET0_1;
		matrix_offset2 = VPP_VD1_MATRIX_OFFSET2;
		matrix_pre_offset0_1 = VPP_VD1_MATRIX_PRE_OFFSET0_1;
		matrix_pre_offset2 = VPP_VD1_MATRIX_PRE_OFFSET2;
		matrix_en_ctrl = VPP_VD1_MATRIX_EN_CTRL;
	} else if (mtx_sel == POST2_MTX) {
		matrix_coef00_01 = VPP_POST2_MATRIX_COEF00_01;
		matrix_coef02_10 = VPP_POST2_MATRIX_COEF02_10;
		matrix_coef11_12 = VPP_POST2_MATRIX_COEF11_12;
		matrix_coef20_21 = VPP_POST2_MATRIX_COEF20_21;
		matrix_coef22 = VPP_POST2_MATRIX_COEF22;
		matrix_coef13_14 = VPP_POST2_MATRIX_COEF13_14;
		matrix_coef23_24 = VPP_POST2_MATRIX_COEF23_24;
		matrix_coef15_25 = VPP_POST2_MATRIX_COEF15_25;
		matrix_clip = VPP_POST2_MATRIX_CLIP;
		matrix_offset0_1 = VPP_POST2_MATRIX_OFFSET0_1;
		matrix_offset2 = VPP_POST2_MATRIX_OFFSET2;
		matrix_pre_offset0_1 = VPP_POST2_MATRIX_PRE_OFFSET0_1;
		matrix_pre_offset2 = VPP_POST2_MATRIX_PRE_OFFSET2;
		matrix_en_ctrl = VPP_POST2_MATRIX_EN_CTRL;
	} else if (mtx_sel == POST_MTX) {
		matrix_coef00_01 = VPP_POST_MATRIX_COEF00_01;
		matrix_coef02_10 = VPP_POST_MATRIX_COEF02_10;
		matrix_coef11_12 = VPP_POST_MATRIX_COEF11_12;
		matrix_coef20_21 = VPP_POST_MATRIX_COEF20_21;
		matrix_coef22 = VPP_POST_MATRIX_COEF22;
		matrix_coef13_14 = VPP_POST_MATRIX_COEF13_14;
		matrix_coef23_24 = VPP_POST_MATRIX_COEF23_24;
		matrix_coef15_25 = VPP_POST_MATRIX_COEF15_25;
		matrix_clip = VPP_POST_MATRIX_CLIP;
		matrix_offset0_1 = VPP_POST_MATRIX_OFFSET0_1;
		matrix_offset2 = VPP_POST_MATRIX_OFFSET2;
		matrix_pre_offset0_1 = VPP_POST_MATRIX_PRE_OFFSET0_1;
		matrix_pre_offset2 = VPP_POST_MATRIX_PRE_OFFSET2;
		matrix_en_ctrl = VPP_POST_MATRIX_EN_CTRL;
	} else {
		return;
	}

	if (slice > SLICE0) {
		if (mtx_sel == VD1_MTX) {
			matrix_coef00_01 = VPP_SLICE1_VD1_MATRIX_COEF00_01;
			matrix_coef02_10 = VPP_SLICE1_VD1_MATRIX_COEF02_10;
			matrix_coef11_12 = VPP_SLICE1_VD1_MATRIX_COEF11_12;
			matrix_coef20_21 = VPP_SLICE1_VD1_MATRIX_COEF20_21;
			matrix_coef22 = VPP_SLICE1_VD1_MATRIX_COEF22;
			matrix_coef13_14 = VPP_SLICE1_VD1_MATRIX_COEF13_14;
			matrix_coef23_24 = VPP_SLICE1_VD1_MATRIX_COEF23_24;
			matrix_coef15_25 = VPP_SLICE1_VD1_MATRIX_COEF15_25;
			matrix_clip = VPP_SLICE1_VD1_MATRIX_CLIP;
			matrix_offset0_1 = VPP_SLICE1_VD1_MATRIX_OFFSET0_1;
			matrix_offset2 = VPP_SLICE1_VD1_MATRIX_OFFSET2;
			matrix_pre_offset0_1 = VPP_SLICE1_VD1_MATRIX_PRE_OFFSET0_1;
			matrix_pre_offset2 = VPP_SLICE1_VD1_MATRIX_PRE_OFFSET2;
			matrix_en_ctrl = VPP_SLICE1_VD1_MATRIX_EN_CTRL;
		} else {
			matrix_coef00_01 += offset;
			matrix_coef02_10 += offset;
			matrix_coef11_12 += offset;
			matrix_coef20_21 += offset;
			matrix_coef22 += offset;
			matrix_coef13_14 += offset;
			matrix_coef23_24 += offset;
			matrix_coef15_25 += offset;
			matrix_clip += offset;
			matrix_offset0_1 += offset;
			matrix_offset2 += offset;
			matrix_pre_offset0_1 += offset;
			matrix_pre_offset2 += offset;
			matrix_en_ctrl += offset;
		}
	}

	VSYNC_WRITE_VPP_REG_BITS_VPP_SEL(matrix_en_ctrl, mtx_on, 0, 1, vpp_sel);

	if (!mtx_on)
		return;

	switch (mtx_csc) {
	case MATRIX_RGB_YUV709:
		VSYNC_WRITE_VPP_REG_VPP_SEL(matrix_coef00_01, 0x00bb0275, vpp_sel);
		VSYNC_WRITE_VPP_REG_VPP_SEL(matrix_coef02_10, 0x003f1f99, vpp_sel);
		VSYNC_WRITE_VPP_REG_VPP_SEL(matrix_coef11_12, 0x1ea601c2, vpp_sel);
		VSYNC_WRITE_VPP_REG_VPP_SEL(matrix_coef20_21, 0x01c21e67, vpp_sel);
		VSYNC_WRITE_VPP_REG_VPP_SEL(matrix_coef22, 0x00001fd7, vpp_sel);
		VSYNC_WRITE_VPP_REG_VPP_SEL(matrix_offset0_1, 0x00400200, vpp_sel);
		VSYNC_WRITE_VPP_REG_VPP_SEL(matrix_offset2, 0x00000200, vpp_sel);
		VSYNC_WRITE_VPP_REG_VPP_SEL(matrix_pre_offset0_1, 0x0, vpp_sel);
		VSYNC_WRITE_VPP_REG_VPP_SEL(matrix_pre_offset2, 0x0, vpp_sel);
		break;
	case MATRIX_YUV709_RGB:
		VSYNC_WRITE_VPP_REG_VPP_SEL(matrix_coef00_01, 0x04ac0000, vpp_sel);
		VSYNC_WRITE_VPP_REG_VPP_SEL(matrix_coef02_10, 0x073104ac, vpp_sel);
		VSYNC_WRITE_VPP_REG_VPP_SEL(matrix_coef11_12, 0x1f251ddd, vpp_sel);
		VSYNC_WRITE_VPP_REG_VPP_SEL(matrix_coef20_21, 0x04ac0879, vpp_sel);
		VSYNC_WRITE_VPP_REG_VPP_SEL(matrix_coef22, 0x0, vpp_sel);
		VSYNC_WRITE_VPP_REG_VPP_SEL(matrix_offset0_1, 0x0, vpp_sel);
		VSYNC_WRITE_VPP_REG_VPP_SEL(matrix_offset2, 0x0, vpp_sel);
		VSYNC_WRITE_VPP_REG_VPP_SEL(matrix_pre_offset0_1, 0x7c00600, vpp_sel);
		VSYNC_WRITE_VPP_REG_VPP_SEL(matrix_pre_offset2, 0x00000600, vpp_sel);
		break;
	case MATRIX_YUV709F_RGB:/*full to full*/
		VSYNC_WRITE_VPP_REG_VPP_SEL(matrix_coef00_01, 0x04000000, vpp_sel);
		VSYNC_WRITE_VPP_REG_VPP_SEL(matrix_coef02_10, 0x064D0400, vpp_sel);
		VSYNC_WRITE_VPP_REG_VPP_SEL(matrix_coef11_12, 0x1F411E21, vpp_sel);
		VSYNC_WRITE_VPP_REG_VPP_SEL(matrix_coef20_21, 0x0400076D, vpp_sel);
		VSYNC_WRITE_VPP_REG_VPP_SEL(matrix_coef22, 0x0, vpp_sel);
		VSYNC_WRITE_VPP_REG_VPP_SEL(matrix_offset0_1, 0x0, vpp_sel);
		VSYNC_WRITE_VPP_REG_VPP_SEL(matrix_offset2, 0x0, vpp_sel);
		VSYNC_WRITE_VPP_REG_VPP_SEL(matrix_pre_offset0_1, 0x0000600, vpp_sel);
		VSYNC_WRITE_VPP_REG_VPP_SEL(matrix_pre_offset2, 0x00000600, vpp_sel);
		break;
	default:
		break;
	}

	/*pr_info("mtx_sel:%d, mtx_csc:0x%x\n", mtx_sel, mtx_csc);*/
}

void ve_dnlp_set(ulong *data)
{
	int i;
	int j;
	int slice_max;
	int dnlp_reg = VPP_SRSHARP1_DNLP_00;

	if (multi_slice_case)
		slice_max = get_slice_max();
	else
		slice_max = 1;

	for (i = SLICE0; i < slice_max; i++)
		for (j = 0; j < 32; j++)
			WRITE_VPP_REG_S5(dnlp_reg + sr_sharp_reg_ofst[i],
				data[j]);
}

void ve_dnlp_ctl(int enable)
{
	int i;
	int slice_max;
	int dnlp_reg = VPP_SRSHARP1_DNLP_EN;

	if (multi_slice_case)
		slice_max = get_slice_max();
	else
		slice_max = 1;

	for (i = SLICE0; i < slice_max; i++)
		WRITE_VPP_REG_BITS_S5(dnlp_reg + sr_sharp_reg_ofst[i],
			enable, 0, 1);
}

void ve_dnlp_sat_set(unsigned int value)
{
	int i;
	int slice_max;
	int addr_reg = VPP_CHROMA_ADDR_PORT;
	int data_reg = VPP_CHROMA_DATA_PORT;
	unsigned int reg_value;

	if (multi_slice_case)
		slice_max = get_slice_max();
	else
		slice_max = 1;

	for (i = SLICE0; i < slice_max; i++) {
		addr_reg += cm_reg_ofst[i];
		data_reg += cm_reg_ofst[i];

		VSYNC_WRITE_VPP_REG(addr_reg, 0x207);
		reg_value = VSYNC_READ_VPP_REG(data_reg);
		reg_value = (reg_value & 0xf000ffff) | (value << 16);

		VSYNC_WRITE_VPP_REG(addr_reg, 0x207);
		VSYNC_WRITE_VPP_REG(data_reg, reg_value);
	}

	pr_amve_v2("%s: val = %d", __func__, value);
}

static void _lc_mtrx_set(enum lc_mtx_sel_e mtrx_sel,
	enum lc_mtx_csc_e mtrx_csc, int mtrx_en,
	int bitdepth, int slice_idx)
{
	unsigned int mtrx_coef00_01 = 0;
	unsigned int mtrx_coef02_10 = 0;
	unsigned int mtrx_coef11_12 = 0;
	unsigned int mtrx_coef20_21 = 0;
	unsigned int mtrx_coef22 = 0;
	unsigned int mtrx_clip = 0;
	unsigned int mtrx_offset0_1 = 0;
	unsigned int mtrx_offset2 = 0;
	unsigned int mtrx_pre_offset0_1 = 0;
	unsigned int mtrx_pre_offset2 = 0;
	unsigned int mtrx_en_ctrl = 0;
	int slice_max;

	slice_max = get_slice_max();
	if (slice_idx > slice_max - 1)
		slice_idx = slice_max - 1;

	switch (mtrx_sel) {
	case INP_MTX:
		mtrx_coef00_01 = VPP_SRSHARP1_LC_YUV2RGB_MAT_0_1 +
			sr_sharp_reg_ofst[slice_idx];
		mtrx_coef02_10 = mtrx_coef00_01 + 1;
		mtrx_coef11_12 = mtrx_coef02_10 + 1;
		mtrx_coef20_21 = mtrx_coef11_12 + 1;
		mtrx_coef22 = mtrx_coef20_21 + 1;
		mtrx_pre_offset0_1 = VPP_SRSHARP1_LC_YUV2RGB_OFST +
			sr_sharp_reg_ofst[slice_idx];
		mtrx_clip = VPP_SRSHARP1_LC_YUV2RGB_CLIP +
			sr_sharp_reg_ofst[slice_idx];
		break;
	case OUTP_MTX:
		mtrx_coef00_01 = VPP_SRSHARP1_LC_RGB2YUV_MAT_0_1 +
			sr_sharp_reg_ofst[slice_idx];
		mtrx_coef02_10 = mtrx_coef00_01 + 1;
		mtrx_coef11_12 = mtrx_coef02_10 + 1;
		mtrx_coef20_21 = mtrx_coef11_12 + 1;
		mtrx_coef22 = mtrx_coef20_21 + 1;
		mtrx_offset0_1 = VPP_SRSHARP1_LC_RGB2YUV_OFST +
			sr_sharp_reg_ofst[slice_idx];
		mtrx_clip = VPP_SRSHARP1_LC_RGB2YUV_CLIP +
			sr_sharp_reg_ofst[slice_idx];
		break;
	case STAT_MTX:
		mtrx_coef00_01 = VPP_LC_STTS_MATRIX_COEF00_01;
		mtrx_coef02_10 = mtrx_coef00_01 + 1;
		mtrx_coef11_12 = mtrx_coef02_10 + 1;
		mtrx_coef20_21 = mtrx_coef11_12 + 1;
		mtrx_coef22 = VPP_LC_STTS_MATRIX_COEF22;
		mtrx_offset0_1 = VPP_LC_STTS_MATRIX_OFFSET0_1;
		mtrx_offset2 = VPP_LC_STTS_MATRIX_OFFSET2;
		mtrx_pre_offset0_1 = VPP_LC_STTS_MATRIX_PRE_OFFSET0_1;
		mtrx_pre_offset2 = VPP_LC_STTS_MATRIX_PRE_OFFSET2;
		if (slice_idx == 0)
			mtrx_en_ctrl = VPP_LC_STTS_CTRL0;
		else
			mtrx_en_ctrl = VPP_LC_STTS_CTRL1;
		break;
	default:
		return;
	}

	if (mtrx_sel & STAT_MTX)
		WRITE_VPP_REG_BITS_S5(mtrx_en_ctrl, mtrx_en, 2, 1);

	if (!mtrx_en)
		return;

	switch (mtrx_csc) {
	case LC_MTX_RGB_YUV601L:
		if (mtrx_sel & (INP_MTX | OUTP_MTX)) {
			WRITE_VPP_REG_S5(mtrx_coef00_01,
				0x01070204);
			WRITE_VPP_REG_S5(mtrx_coef02_10,
				0x00640f68);
			WRITE_VPP_REG_S5(mtrx_coef11_12,
				0x0ed601c2);
			WRITE_VPP_REG_S5(mtrx_coef20_21,
				0x01c20e87);
			WRITE_VPP_REG_S5(mtrx_coef22,
				0x00000fb7);
			if (bitdepth == 10) {
				WRITE_VPP_REG_S5(mtrx_offset0_1,
					0x00400200);
				WRITE_VPP_REG_S5(mtrx_clip,
					0x000003ff);
			} else {
				WRITE_VPP_REG_S5(mtrx_offset0_1,
					0x01000800);
				WRITE_VPP_REG_S5(mtrx_clip,
					0x00000fff);
			}
		} else if (mtrx_sel & STAT_MTX) {
			WRITE_VPP_REG_S5(mtrx_coef00_01,
				0x00bb0275);
			WRITE_VPP_REG_S5(mtrx_coef02_10,
				0x003f1f99);
			WRITE_VPP_REG_S5(mtrx_coef11_12,
				0x1ea601c2);
			WRITE_VPP_REG_S5(mtrx_coef20_21,
				0x01c21e67);
			WRITE_VPP_REG_S5(mtrx_coef22,
				0x00001fd7);
			WRITE_VPP_REG_S5(mtrx_offset0_1,
				0x00400200);
			WRITE_VPP_REG_S5(mtrx_offset2,
				0x00000200);
			WRITE_VPP_REG_S5(mtrx_pre_offset0_1,
				0x00000000);
			WRITE_VPP_REG_S5(mtrx_pre_offset2,
				0x00000000);
		}
		break;
	case LC_MTX_YUV601L_RGB:
		if (mtrx_sel & (INP_MTX | OUTP_MTX)) {
			if (bitdepth == 10) {
				WRITE_VPP_REG_S5(mtrx_coef00_01,
					0x012a0000);
				WRITE_VPP_REG_S5(mtrx_coef02_10,
					0x0198012a);
				WRITE_VPP_REG_S5(mtrx_coef11_12,
					0x0f9c0f30);
				WRITE_VPP_REG_S5(mtrx_coef20_21,
					0x012a0204);
				WRITE_VPP_REG_S5(mtrx_coef22,
					0x00000000);
				WRITE_VPP_REG_S5(mtrx_pre_offset0_1,
					0x00400200);
				WRITE_VPP_REG_S5(mtrx_clip,
					0x000003ff);
			} else {
				WRITE_VPP_REG_S5(mtrx_coef00_01,
					0x04A80000);
				WRITE_VPP_REG_S5(mtrx_coef02_10,
					0x066204a8);
				WRITE_VPP_REG_S5(mtrx_coef11_12,
					0x1e701cbf);
				WRITE_VPP_REG_S5(mtrx_coef20_21,
					0x04a80812);
				WRITE_VPP_REG_S5(mtrx_coef22,
					0x00000000);
				WRITE_VPP_REG_S5(mtrx_pre_offset0_1,
					0x01000800);
				WRITE_VPP_REG_S5(mtrx_clip,
					0x00000fff);
			}
		} else if (mtrx_sel & STAT_MTX) {
			WRITE_VPP_REG_S5(mtrx_coef00_01,
				0x04a80000);
			WRITE_VPP_REG_S5(mtrx_coef02_10,
				0x072c04a8);
			WRITE_VPP_REG_S5(mtrx_coef11_12,
				0x1f261ddd);
			WRITE_VPP_REG_S5(mtrx_coef20_21,
				0x04a80876);
			WRITE_VPP_REG_S5(mtrx_coef22,
				0x00000000);
			WRITE_VPP_REG_S5(mtrx_offset0_1,
				0x00000000);
			WRITE_VPP_REG_S5(mtrx_offset2,
				0x00000000);
			WRITE_VPP_REG_S5(mtrx_pre_offset0_1,
				0x07c00600);
			WRITE_VPP_REG_S5(mtrx_pre_offset2,
				0x00000600);
		}
		break;
	case LC_MTX_RGB_YUV709L:
		if (mtrx_sel & (INP_MTX | OUTP_MTX)) {
			WRITE_VPP_REG_S5(mtrx_coef00_01,
				0x00bb0275);
			WRITE_VPP_REG_S5(mtrx_coef02_10,
				0x003f1f99);
			WRITE_VPP_REG_S5(mtrx_coef11_12,
				0x1ea601c2);
			WRITE_VPP_REG_S5(mtrx_coef20_21,
				0x01c21e67);
			WRITE_VPP_REG_S5(mtrx_coef22,
				0x00001fd7);
			if (bitdepth == 10) {
				WRITE_VPP_REG_S5(mtrx_offset0_1,
					0x00400200);
				WRITE_VPP_REG_S5(mtrx_clip,
					0x000003ff);
			} else {
				WRITE_VPP_REG_S5(mtrx_offset0_1,
					0x01000800);
				WRITE_VPP_REG_S5(mtrx_clip,
					0x00000fff);
			}
		} else if (mtrx_sel & STAT_MTX) {
			WRITE_VPP_REG_S5(mtrx_coef00_01,
				0x00bb0275);
			WRITE_VPP_REG_S5(mtrx_coef02_10,
				0x003f1f99);
			WRITE_VPP_REG_S5(mtrx_coef11_12,
				0x1ea601c2);
			WRITE_VPP_REG_S5(mtrx_coef20_21,
				0x01c21e67);
			WRITE_VPP_REG_S5(mtrx_coef22,
				0x00001fd7);
			WRITE_VPP_REG_S5(mtrx_offset0_1,
				0x00400200);
			WRITE_VPP_REG_S5(mtrx_offset2,
				0x00000200);
			WRITE_VPP_REG_S5(mtrx_pre_offset0_1,
				0x00000000);
			WRITE_VPP_REG_S5(mtrx_pre_offset2,
				0x00000000);
		}
		break;
	case LC_MTX_YUV709L_RGB:
		if (mtrx_sel & (INP_MTX | OUTP_MTX)) {
			if (bitdepth == 10) {
				WRITE_VPP_REG_S5(mtrx_coef00_01,
					0x012a0000);
				WRITE_VPP_REG_S5(mtrx_coef02_10,
					0x01cb012a);
				WRITE_VPP_REG_S5(mtrx_coef11_12,
					0x1fc90f77);
				WRITE_VPP_REG_S5(mtrx_coef20_21,
					0x012a021d);
				WRITE_VPP_REG_S5(mtrx_coef22,
					0x00000000);
				WRITE_VPP_REG_S5(mtrx_pre_offset0_1,
					0x00400200);
				WRITE_VPP_REG_S5(mtrx_clip,
					0x000003ff);
			} else {
				WRITE_VPP_REG_S5(mtrx_coef00_01,
					0x04a80000);
				WRITE_VPP_REG_S5(mtrx_coef02_10,
					0x072c04a8);
				WRITE_VPP_REG_S5(mtrx_coef11_12,
					0x1f261ddd);
				WRITE_VPP_REG_S5(mtrx_coef20_21,
					0x04a80876);
				WRITE_VPP_REG_S5(mtrx_coef22,
					0x00000000);
				WRITE_VPP_REG_S5(mtrx_pre_offset0_1,
					0x01000800);
				WRITE_VPP_REG_S5(mtrx_clip,
					0x00000fff);
			}
		} else if (mtrx_sel & STAT_MTX) {
			WRITE_VPP_REG_S5(mtrx_coef00_01,
				0x04a80000);
			WRITE_VPP_REG_S5(mtrx_coef02_10,
				0x072c04a8);
			WRITE_VPP_REG_S5(mtrx_coef11_12,
				0x1f261ddd);
			WRITE_VPP_REG_S5(mtrx_coef20_21,
				0x04a80876);
			WRITE_VPP_REG_S5(mtrx_coef22,
				0x00000000);
			WRITE_VPP_REG_S5(mtrx_offset0_1,
				0x00000000);
			WRITE_VPP_REG_S5(mtrx_offset2,
				0x00000000);
			WRITE_VPP_REG_S5(mtrx_pre_offset0_1,
				0x07c00600);
			WRITE_VPP_REG_S5(mtrx_pre_offset2,
				0x00000600);
		}
		break;
	case LC_MTX_RGB_YUV709:
		if (mtrx_sel & (INP_MTX | OUTP_MTX)) {
			WRITE_VPP_REG_S5(mtrx_coef00_01,
				0x00da02dc);
			WRITE_VPP_REG_S5(mtrx_coef02_10,
				0x004a1f8a);
			WRITE_VPP_REG_S5(mtrx_coef11_12,
				0x1e760200);
			WRITE_VPP_REG_S5(mtrx_coef20_21,
				0x02001e2f);
			WRITE_VPP_REG_S5(mtrx_coef22,
				0x00001fd1);
			if (bitdepth == 10) {
				WRITE_VPP_REG_S5(mtrx_offset0_1,
					0x00000200);
				WRITE_VPP_REG_S5(mtrx_clip,
					0x000003ff);
			} else {
				WRITE_VPP_REG_S5(mtrx_offset0_1,
					0x00000800);
				WRITE_VPP_REG_S5(mtrx_clip,
					0x00000fff);
			}
		} else if (mtrx_sel & STAT_MTX) {
			WRITE_VPP_REG_S5(mtrx_coef00_01,
				0x00bb0275);
			WRITE_VPP_REG_S5(mtrx_coef02_10,
				0x003f1f99);
			WRITE_VPP_REG_S5(mtrx_coef11_12,
				0x1ea601c2);
			WRITE_VPP_REG_S5(mtrx_coef20_21,
				0x01c21e67);
			WRITE_VPP_REG_S5(mtrx_coef22,
				0x00001fd7);
			WRITE_VPP_REG_S5(mtrx_offset0_1,
				0x00400200);
			WRITE_VPP_REG_S5(mtrx_offset2,
				0x00000200);
			WRITE_VPP_REG_S5(mtrx_pre_offset0_1,
				0x00000000);
			WRITE_VPP_REG_S5(mtrx_pre_offset2,
				0x00000000);
		}
		break;
	case LC_MTX_YUV709_RGB:
		if (mtrx_sel & (INP_MTX | OUTP_MTX)) {
			if (bitdepth == 10) {
				WRITE_VPP_REG_S5(mtrx_coef00_01,
					0x01000000);
				WRITE_VPP_REG_S5(mtrx_coef02_10,
					0x01930100);
				WRITE_VPP_REG_S5(mtrx_coef11_12,
					0x1fd01f88);
				WRITE_VPP_REG_S5(mtrx_coef20_21,
					0x010001db);
				WRITE_VPP_REG_S5(mtrx_coef22,
					0x00000000);
				WRITE_VPP_REG_S5(mtrx_pre_offset0_1,
					0x00000200);
				WRITE_VPP_REG_S5(mtrx_clip,
					0x000003ff);
			} else {
				WRITE_VPP_REG_S5(mtrx_coef00_01,
					0x04000000);
				WRITE_VPP_REG_S5(mtrx_coef02_10,
					0x064d0400);
				WRITE_VPP_REG_S5(mtrx_coef11_12,
					0x1f411e21);
				WRITE_VPP_REG_S5(mtrx_coef20_21,
					0x0400076d);
				WRITE_VPP_REG_S5(mtrx_coef22,
					0x00000000);
				WRITE_VPP_REG_S5(mtrx_pre_offset0_1,
					0x00000800);
				WRITE_VPP_REG_S5(mtrx_clip,
					0x00000fff);
			}
		} else if (mtrx_sel & STAT_MTX) {
			WRITE_VPP_REG_S5(mtrx_coef00_01,
				0x04000000);
			WRITE_VPP_REG_S5(mtrx_coef02_10,
				0x064d0400);
			WRITE_VPP_REG_S5(mtrx_coef11_12,
				0x1f411e21);
			WRITE_VPP_REG_S5(mtrx_coef20_21,
				0x0400076d);
			WRITE_VPP_REG_S5(mtrx_coef22,
				0x00000000);
			WRITE_VPP_REG_S5(mtrx_offset0_1,
				0x00000000);
			WRITE_VPP_REG_S5(mtrx_offset2,
				0x00000000);
			WRITE_VPP_REG_S5(mtrx_pre_offset0_1,
				0x00000600);
			WRITE_VPP_REG_S5(mtrx_pre_offset2,
				0x00000600);
		}
		break;
	case LC_MTX_NULL:
		if (mtrx_sel & (INP_MTX | OUTP_MTX)) {
			WRITE_VPP_REG_S5(mtrx_coef00_01,
				0x04000000);
			WRITE_VPP_REG_S5(mtrx_coef02_10,
				0x00000000);
			WRITE_VPP_REG_S5(mtrx_coef11_12,
				0x04000000);
			WRITE_VPP_REG_S5(mtrx_coef20_21,
				0x00000000);
			WRITE_VPP_REG_S5(mtrx_coef22,
				0x00000400);
			WRITE_VPP_REG_S5(mtrx_offset0_1,
				0x00000000);
		} else if (mtrx_sel & STAT_MTX) {
			WRITE_VPP_REG_S5(mtrx_coef00_01,
				0x04000000);
			WRITE_VPP_REG_S5(mtrx_coef02_10,
				0x00000000);
			WRITE_VPP_REG_S5(mtrx_coef11_12,
				0x04000000);
			WRITE_VPP_REG_S5(mtrx_coef20_21,
				0x00000000);
			WRITE_VPP_REG_S5(mtrx_coef22,
				0x00000400);
			WRITE_VPP_REG_S5(mtrx_offset0_1,
				0x00000000);
			WRITE_VPP_REG_S5(mtrx_offset2,
				0x00000000);
			WRITE_VPP_REG_S5(mtrx_pre_offset0_1,
				0x00000000);
			WRITE_VPP_REG_S5(mtrx_pre_offset2,
				0x00000000);
		}
		break;
	default:
		break;
	}
}

static void _lc_blk_bdry_cfg(unsigned int height, unsigned int width)
{
	int i;
	int slice_max;
	int lc_reg;
	unsigned int value;

	width /= 12;
	height /= 8;

	slice_max = get_slice_max();

	/*lc curve mapping block IDX default 4k panel*/
	for (i = SLICE0; i < slice_max; i++) {
		lc_reg = VPP_SRSHARP1_LC_CURVE_BLK_HIDX_0_1 +
			sr_sharp_reg_ofst[i];
		value = width & GET_BITS(0, 14);
		value |= (0 << 16) & GET_BITS(16, 14);
		WRITE_VPP_REG_S5(lc_reg, value);

		lc_reg += 1;
		value = (width * 3) & GET_BITS(0, 14);
		value |= ((width * 2) << 16) & GET_BITS(16, 14);
		WRITE_VPP_REG_S5(lc_reg, value);

		lc_reg += 1;
		value = (width * 5) & GET_BITS(0, 14);
		value |= ((width * 4) << 16) & GET_BITS(16, 14);
		WRITE_VPP_REG_S5(lc_reg, value);

		lc_reg += 1;
		value = (width * 7) & GET_BITS(0, 14);
		value |= ((width * 6) << 16) & GET_BITS(16, 14);
		WRITE_VPP_REG_S5(lc_reg, value);

		lc_reg += 1;
		value = (width * 9) & GET_BITS(0, 14);
		value |= ((width * 8) << 16) & GET_BITS(16, 14);
		WRITE_VPP_REG_S5(lc_reg, value);

		lc_reg += 1;
		value = (width * 11) & GET_BITS(0, 14);
		value |= ((width * 10) << 16) & GET_BITS(16, 14);
		WRITE_VPP_REG_S5(lc_reg, value);

		lc_reg += 1;
		value = width & GET_BITS(0, 14);
		WRITE_VPP_REG_S5(lc_reg, value);

		lc_reg = VPP_SRSHARP1_LC_CURVE_BLK_VIDX_0_1 +
			sr_sharp_reg_ofst[i];
		value = height & GET_BITS(0, 14);
		value |= (0 << 16) & GET_BITS(16, 14);
		WRITE_VPP_REG_S5(lc_reg, value);

		lc_reg += 1;
		value = (height * 3) & GET_BITS(0, 14);
		value |= ((height * 2) << 16) & GET_BITS(16, 14);
		WRITE_VPP_REG_S5(lc_reg, value);

		lc_reg += 1;
		value = (height * 5) & GET_BITS(0, 14);
		value |= ((height * 4) << 16) & GET_BITS(16, 14);
		WRITE_VPP_REG_S5(lc_reg, value);

		lc_reg += 1;
		value = (height * 7) & GET_BITS(0, 14);
		value |= ((height * 6) << 16) & GET_BITS(16, 14);
		WRITE_VPP_REG_S5(lc_reg, value);

		lc_reg += 1;
		value = height & GET_BITS(0, 14);
		WRITE_VPP_REG_S5(lc_reg, value);
	}
}

void ve_lc_stts_blk_cfg(unsigned int height,
	unsigned int width)
{
	int lc_reg;
	int row_start = 0;
	int col_start = 0;
	int h_num = 12;
	int v_num = 8;
	int blk_height, blk_width;
	int data32;
	int hend0, hend1, hend2, hend3, hend4, hend5, hend6;
	int hend7, hend8, hend9, hend10, hend11;
	int vend0, vend1, vend2, vend3, vend4, vend5, vend6, vend7;

	blk_height = height / v_num;
	blk_width = width / h_num;

	hend0 = col_start + blk_width - 1;
	hend1 = hend0 + blk_width;
	hend2 = hend1 + blk_width;
	hend3 = hend2 + blk_width;
	hend4 = hend3 + blk_width;
	hend5 = hend4 + blk_width;
	hend6 = hend5 + blk_width;
	hend7 = hend6 + blk_width;
	hend8 = hend7 + blk_width;
	hend9 = hend8 + blk_width;
	hend10 = hend9 + blk_width;
	hend11 = width - 1;

	vend0 = row_start + blk_height - 1;
	vend1 = vend0 + blk_height;
	vend2 = vend1 + blk_height;
	vend3 = vend2 + blk_height;
	vend4 = vend3 + blk_height;
	vend5 = vend4 + blk_height;
	vend6 = vend5 + blk_height;
	vend7 = height - 1;

	lc_reg = VPP_LC_STTS_HIST_REGION_IDX;
	data32 = READ_VPP_REG_S5(lc_reg);
	WRITE_VPP_REG_S5(lc_reg, 0xffe0ffff & data32);

	lc_reg = VPP_LC_STTS_HIST_SET_REGION;
	data32 = (((row_start & 0x1fff) << 16) & 0xffff0000) |
		(col_start & 0x1fff);
	WRITE_VPP_REG_S5(lc_reg, data32);
	data32 = ((hend1 & 0x1fff) << 16) | (hend0 & 0x1fff);
	WRITE_VPP_REG_S5(lc_reg, data32);
	data32 = ((vend1 & 0x1fff) << 16) | (vend0 & 0x1fff);
	WRITE_VPP_REG_S5(lc_reg, data32);
	data32 = ((hend3 & 0x1fff) << 16) | (hend2 & 0x1fff);
	WRITE_VPP_REG_S5(lc_reg, data32);
	data32 = ((vend3 & 0x1fff) << 16) | (vend2 & 0x1fff);
	WRITE_VPP_REG_S5(lc_reg, data32);
	data32 = ((hend5 & 0x1fff) << 16) | (hend4 & 0x1fff);
	WRITE_VPP_REG_S5(lc_reg, data32);
	data32 = ((vend5 & 0x1fff) << 16) | (vend4 & 0x1fff);
	WRITE_VPP_REG_S5(lc_reg, data32);

	data32 = ((hend7 & 0x1fff) << 16) | (hend6 & 0x1fff);
	WRITE_VPP_REG_S5(lc_reg, data32);
	data32 = ((vend7 & 0x1fff) << 16) | (vend6 & 0x1fff);
	WRITE_VPP_REG_S5(lc_reg, data32);
	data32 = ((hend9 & 0x1fff) << 16) | (hend8 & 0x1fff);
	WRITE_VPP_REG_S5(lc_reg, data32);
	data32 = ((hend11 & 0x1fff) << 16) | (hend10 & 0x1fff);
	WRITE_VPP_REG_S5(lc_reg, data32);
	data32 = h_num;
	WRITE_VPP_REG_S5(lc_reg, data32);
}

void ve_lc_stts_en(int enable,
	unsigned int height, unsigned int width,
	int pix_drop_mode, int eol_en, int hist_mode,
	int lpf_en, int din_sel, int bitdepth,
	int flag, int flag_full, int thd_black)
{
	int i;
	int slice_max;
	int data32;

	slice_max = get_slice_max();

	WRITE_VPP_REG_S5(VPP_LC_STTS_GCLK_CTRL0, 0x0);

	data32 = ((width - 1) << 16) | (height - 1);
	WRITE_VPP_REG_S5(VPP_LC_STTS1_WIDTHM1_HEIGHTM1,
		data32);
	WRITE_VPP_REG_S5(VPP_LC_STTS2_WIDTHM1_HEIGHTM1,
		data32);

	data32 = 0x80000000 | ((pix_drop_mode & 0x3) << 29);
	data32 = data32 | ((eol_en & 0x1) << 28);
	data32 = data32 | ((hist_mode & 0x3) << 22);
	data32 = data32 | ((lpf_en & 0x1) << 21);
	WRITE_VPP_REG_S5(VPP_LC_STTS_HIST_REGION_IDX, data32);

	for (i = SLICE0; i < slice_max; i++) {
		if (flag == 0x3) {
			_lc_mtrx_set(STAT_MTX, LC_MTX_YUV601L_RGB,
				enable, bitdepth, i);
		} else {
			if (flag_full == 1)
				_lc_mtrx_set(STAT_MTX, LC_MTX_YUV709_RGB,
					enable, bitdepth, i);
			else
				_lc_mtrx_set(STAT_MTX, LC_MTX_YUV709L_RGB,
					enable, bitdepth, i);
		}
	}

	WRITE_VPP_REG_BITS_S5(VPP_LC_STTS_CTRL0, din_sel, 3, 3);
	WRITE_VPP_REG_BITS_S5(VPP_LC_STTS_CTRL1, din_sel, 3, 3);
	/*lc input probe enable*/
	WRITE_VPP_REG_BITS_S5(VPP_LC_STTS_CTRL0, 1, 10, 1);
	WRITE_VPP_REG_BITS_S5(VPP_LC_STTS_CTRL1, 1, 10, 1);
	/*lc hist stts enable*/
	WRITE_VPP_REG_BITS_S5(VPP_LC_STTS_HIST_REGION_IDX,
		enable, 31, 1);
	WRITE_VPP_REG_BITS_S5(VPP_LC_STTS_BLACK_INFO1,
		thd_black, 0, 8);
	WRITE_VPP_REG_BITS_S5(VPP_LC_STTS_BLACK_INFO2,
		thd_black, 0, 8);
}

void ve_lc_blk_num_get(int *h_num, int *v_num,
	int slice)
{
	int slice_max;
	int lc_reg;
	int dwtemp;

	if (!h_num || !v_num)
		return;

	slice_max = get_slice_max();

	if (slice >= slice_max || slice < 0)
		slice = 0;

	lc_reg = VPP_LC1_CURVE_HV_NUM +
		lc_reg_ofst[slice];
	dwtemp = READ_VPP_REG_S5(lc_reg);
	*h_num = (dwtemp >> 8) & 0x1f;
	*v_num = dwtemp & 0x1f;
}

void ve_lc_disable(void)
{
	int i;
	int slice_max;
	int lc_reg;

	slice_max = get_slice_max();

	for (i = SLICE0; i < slice_max; i++) {
		lc_reg = VPP_SRSHARP1_LC_TOP_CTRL +
			sr_sharp_reg_ofst[i];
		WRITE_VPP_REG_BITS_S5(lc_reg, 0, 4, 1);

		lc_reg = VPP_LC1_CURVE_CTRL +
			lc_reg_ofst[i];
		WRITE_VPP_REG_BITS_S5(lc_reg, 0, 0, 1);

		lc_reg = VPP_LC1_CURVE_RAM_CTRL +
			lc_reg_ofst[i];
		WRITE_VPP_REG_BITS_S5(lc_reg, 0, 0, 1);
	}

	WRITE_VPP_REG_BITS_S5(VPP_LC_STTS_HIST_REGION_IDX,
		0, 31, 1);
}

void ve_lc_curve_ctrl_cfg(int enable,
	unsigned int height, unsigned int width)
{
	unsigned int histvld_thrd;
	unsigned int blackbar_mute_thrd;
	unsigned int lmtrat_minmax;
	int h_num = 12;
	int v_num = 8;
	int i;
	int slice_max;
	int lc_reg;
	int tmp;

	slice_max = get_slice_max();

	for (i = SLICE0; i < slice_max; i++) {
		lc_reg = VPP_LC1_CURVE_LMT_RAT +
			lc_reg_ofst[i];
		tmp = READ_VPP_REG_S5(lc_reg);
		lmtrat_minmax = (tmp >> 8) & 0xff;
		tmp = (height * width) / (h_num * v_num);
		histvld_thrd = (tmp * lmtrat_minmax) >> 10;
		blackbar_mute_thrd = tmp >> 3;

		if (!enable) {
			lc_reg = VPP_LC1_CURVE_CTRL +
				lc_reg_ofst[i];
			WRITE_VPP_REG_BITS_S5(lc_reg, enable, 0, 1);
		} else {
			lc_reg = VPP_LC1_CURVE_HV_NUM +
				lc_reg_ofst[i];
			WRITE_VPP_REG_S5(lc_reg, (h_num << 8) | v_num);

			lc_reg = VPP_LC1_CURVE_HISTVLD_THRD +
				lc_reg_ofst[i];
			WRITE_VPP_REG_S5(lc_reg, histvld_thrd);

			lc_reg = VPP_LC1_CURVE_BB_MUTE_THRD +
				lc_reg_ofst[i];
			WRITE_VPP_REG_S5(lc_reg, blackbar_mute_thrd);

			lc_reg = VPP_LC1_CURVE_CTRL +
				lc_reg_ofst[i];
			WRITE_VPP_REG_BITS_S5(lc_reg, enable, 0, 1);
			WRITE_VPP_REG_BITS_S5(lc_reg, enable, 31, 1);
		}
	}
}

void ve_lc_top_cfg(int enable, int h_num, int v_num,
	unsigned int height, unsigned int width, int bitdepth,
	int flag, int flag_full)
{
	int i;
	int slice_max;
	int lc_reg;

	slice_max = get_slice_max();

	/*lc curve mapping block IDX default 4k panel*/
	for (i = SLICE0; i < slice_max; i++) {
		lc_reg = VPP_SRSHARP1_LC_HV_NUM +
			sr_sharp_reg_ofst[i];
		/*lc ram write h num*/
		WRITE_VPP_REG_BITS_S5(lc_reg, h_num, 8, 5);
		/*lc ram write v num*/
		WRITE_VPP_REG_BITS_S5(lc_reg, v_num, 0, 5);
	}

	/*lc curve mapping config*/
	_lc_blk_bdry_cfg(height, width);

	for (i = SLICE0; i < slice_max; i++) {
		lc_reg = VPP_SRSHARP1_LC_TOP_CTRL +
			sr_sharp_reg_ofst[i];
		/*lc enable need set at last*/
		WRITE_VPP_REG_BITS_S5(lc_reg, enable, 4, 1);

		if (flag == 0x3) {
			/* bt601 use 601 matrix */
			_lc_mtrx_set(INP_MTX, LC_MTX_YUV601L_RGB,
				1, bitdepth, i);
			_lc_mtrx_set(OUTP_MTX, LC_MTX_RGB_YUV601L,
				1, bitdepth, i);
		} else {
			/*all other cases use 709 by default*/
			/*to do, should we handle bg2020 separately?*/
			/*for special signal, keep full range to avoid clipping*/
			if (flag_full == 1) {
				_lc_mtrx_set(INP_MTX, LC_MTX_YUV709_RGB,
					1, bitdepth, i);
				_lc_mtrx_set(OUTP_MTX, LC_MTX_RGB_YUV709,
					1, bitdepth, i);
			} else {
				_lc_mtrx_set(INP_MTX, LC_MTX_YUV709L_RGB,
					1, bitdepth, i);
				_lc_mtrx_set(OUTP_MTX, LC_MTX_RGB_YUV709L,
					1, bitdepth, i);
			}
		}
	}
}

void ve_lc_sat_lut_set(int *data)
{
	int i;
	int j;
	int slice_max;
	int lc_reg;
	int tmp;

	if (!data)
		return;

	slice_max = get_slice_max();

	for (i = SLICE0; i < slice_max; i++) {
		lc_reg = VPP_SRSHARP1_LC_SAT_LUT_0_1 +
			sr_sharp_reg_ofst[i];

		for (j = 0; j < 31 ; j++) {
			tmp = ((data[2 * j] & 0xfff) << 16) |
				(data[2 * j + 1] & 0xfff);
			WRITE_VPP_REG_S5(lc_reg + j, tmp);
		}

		lc_reg = VPP_SRSHARP1_LC_SAT_LUT_62 +
			sr_sharp_reg_ofst[i];
		tmp = data[62] & 0xfff;
		WRITE_VPP_REG_S5(lc_reg, tmp);
	}
}

void ve_lc_ymin_lmt_set(int *data)
{
	int i;
	int j;
	int slice_max;
	int lc_reg;
	int tmp;

	if (!data)
		return;

	slice_max = get_slice_max();

	for (i = SLICE0; i < slice_max; i++) {
		lc_reg = VPP_LC1_CURVE_YMINVAL_LMT_0_1 +
			lc_reg_ofst[i];

		for (j = 0; j < 6 ; j++) {
			tmp = ((data[2 * j] & 0x3ff) << 16) |
				(data[2 * j + 1] & 0x3ff);
			WRITE_VPP_REG_S5(lc_reg + j, tmp);
		}

		lc_reg = VPP_LC1_CURVE_YMINVAL_LMT_12_13 +
			lc_reg_ofst[i];
		tmp = ((data[2 * j] & 0x3ff) << 16) |
			(data[2 * j + 1] & 0x3ff);
		WRITE_VPP_REG_S5(lc_reg, tmp);

		j++;
		lc_reg = VPP_LC1_CURVE_YMINVAL_LMT_14_15 +
			lc_reg_ofst[i];
		tmp = ((data[2 * j] & 0x3ff) << 16) |
			(data[2 * j + 1] & 0x3ff);
		WRITE_VPP_REG_S5(lc_reg, tmp);
	}
}

void ve_lc_ymax_lmt_set(int *data)
{
	int i;
	int j;
	int slice_max;
	int lc_reg;
	int tmp;

	if (!data)
		return;

	slice_max = get_slice_max();

	for (i = SLICE0; i < slice_max; i++) {
		lc_reg = VPP_LC1_CURVE_YMAXVAL_LMT_0_1 +
			lc_reg_ofst[i];

		for (j = 0; j < 6 ; j++) {
			tmp = ((data[2 * j] & 0x3ff) << 16) |
				(data[2 * j + 1] & 0x3ff);
			WRITE_VPP_REG_S5(lc_reg + j, tmp);
		}

		lc_reg = VPP_LC1_CURVE_YMAXVAL_LMT_12_13 +
			lc_reg_ofst[i];
		tmp = ((data[2 * j] & 0x3ff) << 16) |
			(data[2 * j + 1] & 0x3ff);
		WRITE_VPP_REG_S5(lc_reg, tmp);

		j++;
		lc_reg = VPP_LC1_CURVE_YMAXVAL_LMT_14_15 +
			lc_reg_ofst[i];
		tmp = ((data[2 * j] & 0x3ff) << 16) |
			(data[2 * j + 1] & 0x3ff);
		WRITE_VPP_REG_S5(lc_reg, tmp);
	}
}

void ve_lc_ypkbv_lmt_set(int *data)
{
	int i;
	int j;
	int slice_max;
	int lc_reg;
	int tmp;

	if (!data)
		return;

	slice_max = get_slice_max();

	for (i = SLICE0; i < slice_max; i++) {
		lc_reg = VPP_LC1_CURVE_YPKBV_LMT_0_1 +
			lc_reg_ofst[i];

		for (j = 0; j < 8 ; j++) {
			tmp = ((data[2 * j] & 0x3ff) << 16) |
				(data[2 * j + 1] & 0x3ff);
			WRITE_VPP_REG_S5(lc_reg + j, tmp);
		}
	}
}

void ve_lc_ypkbv_rat_set(int *data)
{
	int i;
	int slice_max;
	int lc_reg;
	int tmp;

	if (!data)
		return;

	slice_max = get_slice_max();

	for (i = SLICE0; i < slice_max; i++) {
		lc_reg = VPP_LC1_CURVE_YPKBV_RAT +
			lc_reg_ofst[i];
		tmp = ((data[0] & 0xff) << 24) |
			((data[1] & 0xff) << 16) |
			((data[2] & 0xff) << 8) |
			(data[3] & 0xff);
		WRITE_VPP_REG_S5(lc_reg, tmp);
	}
}

void ve_lc_ypkbv_slp_lmt_set(int *data)
{
	int i;
	int slice_max;
	int lc_reg;
	int tmp;

	if (!data)
		return;

	slice_max = get_slice_max();

	for (i = SLICE0; i < slice_max; i++) {
		lc_reg = VPP_LC1_CURVE_YPKBV_SLP_LMT +
			lc_reg_ofst[i];
		tmp = ((data[0] & 0xff) << 8) |
			(data[1] & 0xff);
		WRITE_VPP_REG_S5(lc_reg, tmp);
	}
}

void ve_lc_contrast_lmt_set(int *data)
{
	int i;
	int slice_max;
	int lc_reg;
	int tmp;

	if (!data)
		return;

	slice_max = get_slice_max();

	for (i = SLICE0; i < slice_max; i++) {
		lc_reg = VPP_LC1_CURVE_CONTRAST_LMT_LH +
			lc_reg_ofst[i];
		tmp = ((data[0] & 0xff) << 24) |
			((data[1] & 0xff) << 16) |
			((data[2] & 0xff) << 8) |
			(data[3] & 0xff);
		WRITE_VPP_REG_S5(lc_reg, tmp);
	}
}

void ve_lc_curve_set(int init_flag,
	int demo_mode, int *data, int slice)
{
	int i, j, k;
	int slice_max;
	int lc_reg;
	int ctrl_reg;
	int addr_reg;
	int data_reg;
	int h_num, v_num;
	unsigned int tmp, tmp1;
	unsigned int lnr_data, lnr_data1;

	if (!init_flag && !data)
		return;

	/*initial linear data*/
	lnr_data = 0 | (0 << 10) | (512 << 20);
	lnr_data1 = 1023 | (1023 << 10) | (512 << 20);

	slice_max = get_slice_max();

	if (slice >= slice_max || slice < 0)
		slice = 0;

	lc_reg = VPP_SRSHARP1_LC_HV_NUM +
		sr_sharp_reg_ofst[slice];
	ctrl_reg = VPP_SRSHARP1_LC_MAP_RAM_CTRL +
		sr_sharp_reg_ofst[slice];
	addr_reg = VPP_SRSHARP1_LC_MAP_RAM_ADDR +
		sr_sharp_reg_ofst[slice];
	data_reg = VPP_SRSHARP1_LC_MAP_RAM_DATA +
		sr_sharp_reg_ofst[slice];

	tmp = READ_VPP_REG_S5(lc_reg);
	h_num = (tmp >> 8) & 0x1f;
	v_num = tmp & 0x1f;

	/*data sequence: ymin/minBv/pkBv/maxBv/ymaxv/ypkBv*/
	if (init_flag) {
		WRITE_VPP_REG_S5(ctrl_reg, 1);
		WRITE_VPP_REG_S5(addr_reg, 0);

		for (i = 0; i < h_num * v_num; i++) {
			WRITE_VPP_REG_S5(data_reg, lnr_data);
			WRITE_VPP_REG_S5(data_reg, lnr_data1);
		}

		WRITE_VPP_REG_S5(ctrl_reg, 0);
		return;
	}

	VSYNC_WRITE_VPP_REG(ctrl_reg, 1);
	VSYNC_WRITE_VPP_REG(addr_reg, 0);

	for (i = 0; i < v_num; i++) {
		for (j = 0; j < h_num; j++) {
			switch (demo_mode) {
			case 0:/*off*/
			default:
				k = 6 * (i * h_num + j);
				tmp = data[k + 0] |
					(data[k + 1] << 10) |
					(data[k + 2] << 20);
				tmp1 = data[k + 3] |
					(data[k + 4] << 10) |
					(data[k + 5] << 20);
				break;
			case 1:/*left_side*/
				if (j < h_num / 2) {
					k = 6 * (i * h_num + j);
					tmp = data[k + 0] |
						(data[k + 1] << 10) |
						(data[k + 2] << 20);
					tmp1 = data[k + 3] |
						(data[k + 4] << 10) |
						(data[k + 5] << 20);
				} else {
					tmp = lnr_data;
					tmp1 = lnr_data1;
				}
				break;
			case 2:/*right_side*/
				if (j < h_num / 2) {
					tmp = lnr_data;
					tmp1 = lnr_data1;
				} else {
					k = 6 * (i * h_num + j);
					tmp = data[k + 0] |
						(data[k + 1] << 10) |
						(data[k + 2] << 20);
					tmp1 = data[k + 3] |
						(data[k + 4] << 10) |
						(data[k + 5] << 20);
				}
				break;
			}

			VSYNC_WRITE_VPP_REG(data_reg, tmp);
			VSYNC_WRITE_VPP_REG(data_reg, tmp1);
		}
	}

	VSYNC_WRITE_VPP_REG(ctrl_reg, 0);
}

void ve_lc_base_init(void)
{
	int i;
	int slice_max;
	int lc_reg;

	slice_max = get_slice_max();

	for (i = SLICE0; i < slice_max; i++) {
		lc_reg = VPP_SRSHARP1_LC_INPUT_MUX +
			sr_sharp_reg_ofst[i];
		/*lc input_ysel*/
		WRITE_VPP_REG_BITS_S5(lc_reg, 5, 4, 3);
		/*lc input_csel*/
		WRITE_VPP_REG_BITS_S5(lc_reg, 5, 0, 3);

		lc_reg = VPP_SRSHARP1_LC_TOP_CTRL +
			sr_sharp_reg_ofst[i];
		WRITE_VPP_REG_BITS_S5(lc_reg, 8, 8, 8);
		/*lc blend mode, default 1*/
		WRITE_VPP_REG_BITS_S5(lc_reg, 1, 0, 1);
		/*lc sync ctrl*/
		WRITE_VPP_REG_BITS_S5(lc_reg, 0, 16, 1);

		lc_reg = VPP_LC1_CURVE_RAM_CTRL +
			lc_reg_ofst[i];
		WRITE_VPP_REG_BITS_S5(lc_reg, 0, 0, 1);

		/*default lc low parameters*/
		lc_reg = VPP_LC1_CURVE_CONTRAST_LH +
			lc_reg_ofst[i];
		WRITE_VPP_REG_S5(lc_reg, 0x000b000b);

		lc_reg = VPP_LC1_CURVE_CONTRAST_SCL_LH +
			lc_reg_ofst[i];
		WRITE_VPP_REG_S5(lc_reg, 0x00000b0b);

		lc_reg = VPP_LC1_CURVE_MISC0 +
			lc_reg_ofst[i];
		WRITE_VPP_REG_S5(lc_reg, 0x00023028);

		lc_reg = VPP_LC1_CURVE_YPKBV_RAT +
			lc_reg_ofst[i];
		WRITE_VPP_REG_S5(lc_reg, 0x8cc0c060);

		lc_reg = VPP_LC1_CURVE_YPKBV_SLP_LMT +
			lc_reg_ofst[i];
		WRITE_VPP_REG_S5(lc_reg, 0x00000b3a);

		lc_reg = VPP_LC1_CURVE_YMINVAL_LMT_0_1 +
			lc_reg_ofst[i];
		WRITE_VPP_REG_S5(lc_reg, 0x0030005d);
		lc_reg += 1;
		WRITE_VPP_REG_S5(lc_reg, 0x00830091);
		lc_reg += 1;
		WRITE_VPP_REG_S5(lc_reg, 0x00a000c4);
		lc_reg += 1;
		WRITE_VPP_REG_S5(lc_reg, 0x00e00100);
		lc_reg += 1;
		WRITE_VPP_REG_S5(lc_reg, 0x01200140);
		lc_reg += 1;
		WRITE_VPP_REG_S5(lc_reg, 0x01600190);

		lc_reg = VPP_LC1_CURVE_YMINVAL_LMT_12_13 +
			lc_reg_ofst[i];
		WRITE_VPP_REG_S5(lc_reg, 0x01b001d0);
		lc_reg += 1;
		WRITE_VPP_REG_S5(lc_reg, 0x01f00210);

		lc_reg = VPP_LC1_CURVE_YMAXVAL_LMT_0_1 +
			lc_reg_ofst[i];
		WRITE_VPP_REG_S5(lc_reg, 0x004400b4);
		lc_reg += 1;
		WRITE_VPP_REG_S5(lc_reg, 0x00fb0123);
		lc_reg += 1;
		WRITE_VPP_REG_S5(lc_reg, 0x015901a2);
		lc_reg += 1;
		WRITE_VPP_REG_S5(lc_reg, 0x01d90208);
		lc_reg += 1;
		WRITE_VPP_REG_S5(lc_reg, 0x02400280);
		lc_reg += 1;
		WRITE_VPP_REG_S5(lc_reg, 0x02d70310);

		lc_reg = VPP_LC1_CURVE_YMAXVAL_LMT_12_13 +
			lc_reg_ofst[i];
		WRITE_VPP_REG_S5(lc_reg, 0x03400380);
		lc_reg += 1;
		WRITE_VPP_REG_S5(lc_reg, 0x03c003ff);

		lc_reg = VPP_LC1_CURVE_YPKBV_LMT_0_1 +
			lc_reg_ofst[i];
		WRITE_VPP_REG_S5(lc_reg, 0x004400b4);
		lc_reg += 1;
		WRITE_VPP_REG_S5(lc_reg, 0x00fb0123);
		lc_reg += 1;
		WRITE_VPP_REG_S5(lc_reg, 0x015901a2);
		lc_reg += 1;
		WRITE_VPP_REG_S5(lc_reg, 0x01d90208);
		lc_reg += 1;
		WRITE_VPP_REG_S5(lc_reg, 0x02400280);
		lc_reg += 1;
		WRITE_VPP_REG_S5(lc_reg, 0x02d70310);
		lc_reg += 1;
		WRITE_VPP_REG_S5(lc_reg, 0x03400380);
		lc_reg += 1;
		WRITE_VPP_REG_S5(lc_reg, 0x03c003ff);
	}
}

void ve_lc_region_read(int blk_vnum, int blk_hnum,
	int slice, int *black_count,
	int *curve_data, int *hist_data)
{
	int slice_max;
	int lc_reg;
	int ctrl_reg;
	int addr_reg;
	int data_reg;
	int i, j;
	unsigned int tmp, tmp1;
	unsigned int cur_block;
	unsigned int length = 1632; /*12*8*17*/

	slice_max = get_slice_max();

	if (slice >= slice_max || slice < 0)
		slice = 0;

	if (slice == 0)
		lc_reg = VPP_LC_STTS_BLACK_INFO1;
	else
		lc_reg = VPP_LC_STTS_BLACK_INFO2;

	tmp = READ_VPP_REG_S5(lc_reg);
	*black_count = ((tmp >> 8) & 0xffffff) / 96;

	ctrl_reg = VPP_LC1_CURVE_RAM_CTRL +
		lc_reg_ofst[slice];
	addr_reg = VPP_LC1_CURVE_RAM_ADDR +
		lc_reg_ofst[slice];
	data_reg = VPP_LC1_CURVE_RAM_DATA +
		lc_reg_ofst[slice];

	/*part1: get lc curve node*/
	WRITE_VPP_REG_S5(ctrl_reg, 1);
	WRITE_VPP_REG_S5(addr_reg, 0);
	for (i = 0; i < blk_vnum; i++) {
		for (j = 0; j < blk_hnum; j++) {
			cur_block = i * blk_hnum + j;
			tmp = READ_VPP_REG_S5(data_reg);
			tmp1 = READ_VPP_REG_S5(data_reg);
			curve_data[cur_block * 6 + 0] =
				tmp & 0x3ff; /*bit0:9*/
			curve_data[cur_block * 6 + 1] =
				(tmp >> 10) & 0x3ff; /*bit10:19*/
			curve_data[cur_block * 6 + 2] =
				(tmp >> 20) & 0x3ff; /*bit20:29*/
			curve_data[cur_block * 6 + 3] =
				tmp1 & 0x3ff; /*bit0:9*/
			curve_data[cur_block * 6 + 4] =
				(tmp1 >> 10) & 0x3ff; /*bit10:19*/
			curve_data[cur_block * 6 + 5] =
				(tmp1 >> 20) & 0x3ff; /*bit20:29*/
		}
	}
	WRITE_VPP_REG_S5(ctrl_reg, 0);

	/*part2: get lc hist*/
	am_dma_get_mif_data_lc_stts(slice, hist_data, length);
}

