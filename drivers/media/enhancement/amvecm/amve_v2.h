/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
#ifndef AMVE_V2_H
#define AMVE_V2_H

#include "set_hdr2_v0.h"

extern int multi_picture_case;
extern int multi_slice_case;
extern int hist_dma_case;
extern unsigned int lc_overlap;
extern int lc_slice_num_changed;

struct cm_port_s {
	int cm_addr_port[4];
	int cm_data_port[4];
};

int get_slice_max(void);

struct cm_port_s get_cm_port(void);
void cm_hist_get(struct vpp_hist_param_s *vp, unsigned int hue_bin0, unsigned int sat_bin0);
void cm_hist_by_type_get(enum cm_hist_e hist_sel,
	unsigned int *data, unsigned int length,
	unsigned int addr_bin0);

void post_gainoff_set(struct tcon_rgb_ogo_s *p,
	enum wr_md_e mode, int vpp_index);
void ve_brigtness_set(int val,
	enum vadj_index_e vadj_idx,
	enum wr_md_e mode,
	int vpp_index);
void ve_contrast_set(int val,
	enum vadj_index_e vadj_idx,
	enum wr_md_e mode,
	int vpp_index);
void ve_color_mab_set(int mab,
	enum vadj_index_e vadj_idx,
	enum wr_md_e mode,
	int vpp_index);
void ve_color_mcd_set(int mcd,
	enum vadj_index_e vadj_idx,
	enum wr_md_e mode,
	int vpp_index);
int ve_brightness_contrast_get(enum vadj_index_e vadj_idx);

void vpp_mtx_config_v2(struct matrix_coef_s *coef,
	enum wr_md_e mode, enum vpp_slice_e slice,
	enum vpp_matrix_e mtx_sel, int vpp_index);

/*vpp module control*/
void ve_vadj_ctl(enum wr_md_e mode, enum vadj_index_e vadj_idx, int en,
	int vpp_index);
void ve_bs_ctl(enum wr_md_e mode, int en, int vpp_index);
void ve_ble_ctl(enum wr_md_e mode, int en, int vpp_index);
void ve_cc_ctl(enum wr_md_e mode, int en, int vpp_index);
void post_wb_ctl(enum wr_md_e mode, int en, int vpp_index);
void post_pre_gamma_ctl(enum wr_md_e mode, int en, int vpp_index);
void post_pre_gamma_set(int *lut);
void vpp_luma_hist_init(void);
void get_luma_hist(struct vframe_s *vf, struct vpp_hist_param_s *vp);
void cm_top_ctl(enum wr_md_e mode, int en, int vpp_index);

void ve_multi_picture_case_set(int enable);
int ve_multi_picture_case_get(void);

int ve_multi_slice_case_get(void);

void ve_vadj_misc_set(int val,
	enum wr_md_e mode, enum vadj_index_e vadj_idx,
	enum vpp_slice_e slice, int start, int len,
	int vpp_index);
int ve_vadj_misc_get(enum vadj_index_e vadj_idx,
	enum vpp_slice_e slice, int start, int len);
void ve_mtrx_setting(enum vpp_matrix_e mtx_sel,
	int mtx_csc, int mtx_on, enum vpp_slice_e slice);

void ve_sharpness_gain_set(int sr0_gain, int sr1_gain,
	enum wr_md_e mode, int vpp_index);
void ve_sharpness_ctl(enum wr_md_e mode, int sr0_en,
	int sr1_en, int vpp_index);
void ve_dnlp_set(ulong *data);
void ve_dnlp_ctl(int enable);
void ve_dnlp_sat_set(unsigned int value, int vpp_index);

void ve_lc_stts_blk_cfg(unsigned int height,
	unsigned int width, int h_num, int v_num, int rdma_mode,
	int vpp_index);
void ve_lc_stts_en(int enable, int h_num,
	unsigned int height, unsigned int width,
	int pix_drop_mode, int eol_en, int hist_mode,
	int lpf_en, int din_sel, int bitdepth,
	int flag, int flag_full, int thd_black,
	int rdma_mode, int vpp_index);
void ve_lc_blk_num_get(int *h_num, int *v_num,
	int slice);
void ve_lc_disable(int rdma_mode, int vpp_index);
void ve_lc_curve_ctrl_cfg(int enable,
	unsigned int height, unsigned int width,
	int h_num, int v_num, int rdma_mode, int vpp_index);
void ve_lc_top_cfg(int enable, int h_num, int v_num,
	unsigned int height, unsigned int width, int bitdepth,
	int flag, int flag_full, int rdma_mode, int vpp_index);
void ve_lc_sat_lut_set(int *data);
void ve_lc_ymin_lmt_set(int *data);
void ve_lc_ymax_lmt_set(int *data);
void ve_lc_ypkbv_lmt_set(int *data);
void ve_lc_ypkbv_rat_set(int *data);
void ve_lc_ypkbv_slp_lmt_set(int *data);
void ve_lc_contrast_lmt_set(int *data);
void ve_lc_curve_set(int init_flag,
	int demo_mode, int *data, int slice, int vpp_index);
void ve_lc_base_init(void);
void ve_lc_region_read(int blk_vnum, int blk_hnum,
	int slice, int *black_count,
	int *curve_data, int *hist_data);
void ve_lc_total_en_ctrl(int enable, int rdma_mode, int vpp_index);
void dump_lc_mapping_reg(void);
void dump_lc_reg(void);
void monitor_lc_stts_overflow(void);
void clean_lc_stts_overflow(void);
void dump_dnlp_reg(void);

void post_lut3d_ctl(enum wr_md_e mode, int en, int vpp_index);
void post_lut3d_update(unsigned int *lut3d_data, int vpp_index);
void post_lut3d_set(unsigned int *lut3d_data);
void post_lut3d_section_write(int index, int section_len,
	unsigned int *lut3d_data_in);
void post_lut3d_section_read(int index, int section_len,
	unsigned int *lut3d_data_out);
void mtx_setting_v2(enum vpp_matrix_e mtx_sel,
	enum wr_md_e mode,
	enum mtx_csc_e mtx_csc,
	int mtx_on,
	enum vpp_slice_e slice, int vpp_index);

#endif
#endif
