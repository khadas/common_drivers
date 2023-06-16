/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * drivers/amlogic/media/video_sink/vpp_post_s5.h
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

#ifndef VPP_POST_S5_HH
#define VPP_POST_S5_HH
#include "video_reg_s5.h"

extern u32 g_vpp1_bypass_slice1;
/* VPP POST input src: 3VD, 2 OSD */
/* S5 only VD1, VD2 */
#define VPP_POST_VD_NUM   3
#define VPP_POST_OSD_NUM  2
#define VPP_POST_NUM (VPP_POST_VD_NUM + VPP_POST_OSD_NUM)

#define POST_SLICE_NUM    4
struct vpp_post_blend_s {
	u32 bld_out_en;
	u32 bld_out_w;
	u32 bld_out_h;
	u32 bld_out_premult;

	u32 bld_src1_sel;	  //1:din0	2:din1 3:din2 4:din3 5:din4 else :close
	u32 bld_src2_sel;	  //1:din0	2:din1 3:din2 4:din3 5:din4 else :close
	u32 bld_src3_sel;	  //1:din0	2:din1 3:din2 4:din3 5:din4 else :close
	u32 bld_src4_sel;	  //1:din0	2:din1 3:din2 4:din3 5:din4 else :close
	u32 bld_src5_sel;	  //1:din0	2:din1 3:din2 4:din3 5:din4 else :close
	u32 bld_dummy_data;

	//usually the bottom layer set 1, for example postbld_src1_sel = 1,set 0x1
	u32 bld_din0_premult_en;
	u32 bld_din1_premult_en;
	u32 bld_din2_premult_en;
	u32 bld_din3_premult_en;
	u32 bld_din4_premult_en;

	//u32 vd1_index;//VPP_VD1/VPP_VD2/VPP_VD3
	u32 bld_din0_h_start;
	u32 bld_din0_h_end;
	u32 bld_din0_v_start;
	u32 bld_din0_v_end;
	u32 bld_din0_alpha;

	//u32 vd2_index;//VPP_VD1/VPP_VD2/VPP_VD3
	u32 bld_din1_h_start;
	u32 bld_din1_h_end;
	u32 bld_din1_v_start;
	u32 bld_din1_v_end;
	u32 bld_din1_alpha;

	//u32 vd3_index;//VPP_VD1/VPP_VD2/VPP_VD3
	u32 bld_din2_h_start;
	u32 bld_din2_h_end;
	u32 bld_din2_v_start;
	u32 bld_din2_v_end;
	u32 bld_din2_alpha;

	//u32 osd1_index;//VPP_OSD1/VPP_OSD2/VPP_OSD3/VPP_OSD4
	u32 bld_din3_h_start;
	u32 bld_din3_h_end;
	u32 bld_din3_v_start;
	u32 bld_din3_v_end;

	//u32 osd2_index;//VPP_OSD1/VPP_OSD2/VPP_OSD3/VPP_OSD4
	u32 bld_din4_h_start;
	u32 bld_din4_h_end;
	u32 bld_din4_v_start;
	u32 bld_din4_v_end;
};

struct vpp1_post_blend_s {
	u32 bld_out_en;
	u32 bld_out_w;
	u32 bld_out_h;
	//0: vpp1 walk slice1 to venc1; 1: vpp1 bypass slice1 to venc1
	u32 vpp1_dpath_sel;
	//0:select postblend 1:select vpp1 blend
	u32 vd3_dpath_sel;
	u32 bld_out_premult;
	u32 bld_dummy_data;

	//usually the bottom layer set 1, for example postbld_src1_sel = 1,set 0x1
	u32 bld_din0_premult_en;
	u32 bld_din1_premult_en;

	//VD3
	u32 bld_din0_h_start;
	u32 bld_din0_h_end;
	u32 bld_din0_v_start;
	u32 bld_din0_v_end;
	u32 bld_din0_alpha;

	//OSD3
	u32 bld_din1_h_start;
	u32 bld_din1_h_end;
	u32 bld_din1_v_start;
	u32 bld_din1_v_end;
	u32 bld_din1_alpha;

	//1:din0(vd3)  2:din1(osd3) 3:din2 4:din3 5:din4 else :close
	u32 bld_src1_sel;
	u32 bld_src2_sel;
};

struct vd1_hwin_s {
	u32 vd1_hwin_en;
	u32 vd1_hwin_in_hsize;
	/* hwin cut out before to blend */
	u32 vd1_hwin_out_hsize;
	u32 vd1_win_vsize;
	u32 slice_num;
};

struct vpp_post_pad_s {
	u32 vpp_post_pad_en;
	u32 vpp_post_pad_hsize;
	u32 vpp_post_pad_dummy;
	/* 1: padding with last colum */
	/* 0: padding with vpp_post_pad_dummy val */
	u32 vpp_post_pad_rpt_lcol;
};

struct vpp_post_hwin_s {
	u32 vpp_post_hwin_en;
	u32 vpp_post_dout_hsize;
	u32 vpp_post_dout_vsize;
};

struct vpp_post_in_pad_s {
	u32 pad_en;
	/* padding out size */
	u32 pad_hsize;
	u32 pad_vsize;
	u32 pad_h_bgn;
	u32 pad_h_end;
	u32 pad_v_bgn;
	u32 pad_v_end;
	u32 pad_dummy;
	/* 1: padding with last colum */
	/* 0: padding with vpp_post_pad_dummy val */
	u32 pad_rpt_lcol;
};

struct vpp_post_in_wincut_s {
	u32 win_en;
	u32 win_in_hsize;
	u32 win_in_vsize;
	u32 win_out_hsize;
	u32 win_out_vsize;
};

struct vpp_post_proc_slice_s {
	u32 hsize[POST_SLICE_NUM];
	u32 vsize[POST_SLICE_NUM];
};

struct vpp_post_proc_hwin_s {
	u32 hwin_en[POST_SLICE_NUM];
	u32 hwin_bgn[POST_SLICE_NUM];
	u32 hwin_end[POST_SLICE_NUM];
};

struct vpp_post_proc_s {
	struct vpp_post_proc_slice_s vpp_post_proc_slice;
	struct vpp_post_proc_hwin_s vpp_post_proc_hwin;
	u32 align_fifo_size[POST_SLICE_NUM];
	u32 gamma_bypass;
	u32 ccm_bypass;
	u32 vadj2_bypass;
	u32 lut3d_bypass;
	u32 gain_off_bypass;
	u32 vwm_bypass;
};

struct vpp0_post_s {
	u32 slice_num;
	u32 overlap_hsize;
	/* pad before to vpp post blend */
	struct vpp_post_in_pad_s vd_pad[VPP_POST_VD_NUM];
	struct vpp_post_in_pad_s osd_pad[VPP_POST_OSD_NUM];
	struct vpp_post_in_wincut_s vd_cut[VPP_POST_VD_NUM];
	struct vpp_post_in_wincut_s osd_cut[VPP_POST_OSD_NUM];
	struct vd1_hwin_s vd1_hwin;
	struct vpp_post_blend_s vpp_post_blend;
	struct vpp_post_pad_s vpp_post_pad;
	struct vpp_post_hwin_s vpp_post_hwin;
	struct vpp_post_proc_s vpp_post_proc;
	//struct vpp1_post_blend_s vpp1_post_blend;
};

struct vpp1_post_s {
	bool vpp1_en;
	bool vpp1_bypass_slice1;
	u32 slice_num;
	u32 overlap_hsize;
	struct vpp1_post_blend_s vpp1_post_blend;
};

struct vpp_post_s {
	struct vpp0_post_s vpp0_post;
	struct vpp1_post_s vpp1_post;
};

struct vpp_post_input_s {
	u32 slice_num;
	u32 overlap_hsize;
	u32 din_hsize[VPP_POST_NUM];
	u32 din_vsize[VPP_POST_NUM];
	u32 din_x_start[VPP_POST_NUM];
	u32 din_y_start[VPP_POST_NUM];
	u32 bld_out_hsize;
	u32 bld_out_vsize;
	u32 vpp_post_in_pad_en;
	/* > 0 right padding, < 0 left padding */
	int vpp_post_in_pad_hsize;
	int vpp_post_in_pad_vsize;
	/* means vd1 4s4p padding */
	u32 vd1_padding_en;
	u32 vd1_size_before_padding;
	u32 vd1_size_after_padding;
	u32 vd1_proc_slice;
};

struct vpp_post_in_padding_s {
	u32 vpp_post_in_pad_en;
	/* > 0 right padding, < 0 left padding */
	int vpp_post_in_pad_hsize;
	int vpp_post_in_pad_vsize;
};

struct vpp_post_reg_s {
	struct vpp_post_in_pad_reg_s vpp_post_in_pad_reg[5];
	struct vpp_post_blend_reg_s vpp_post_blend_reg;
	struct vpp_post_misc_reg_s vpp_post_misc_reg;
	struct vpp1_post_blend_reg_s vpp1_post_blend_reg;
};

int get_vpp_slice_num(const struct vinfo_s *info);
int vpp_post_param_set(struct vpp_post_input_s *vpp_input,
	struct vpp0_post_s *vpp_post);
int vpp1_post_param_set(struct vpp_post_input_s *vpp_input,
	struct vpp1_post_s *vpp_post);
void vpp_post_set(u32 vpp_index, struct vpp_post_s *vpp_post);
int update_vpp_input_info(const struct vinfo_s *info);
int update_vpp1_input_info(const struct vinfo_s *info);
struct vpp_post_input_s *get_vpp_input_info(void);
struct vpp_post_input_s *get_vpp1_input_info(void);
void dump_vpp_post_reg(void);
void vpp_clip_setting_s5(u8 vpp_index, struct clip_setting_s *setting);
void get_vpp_in_padding_axis(u32 *enable, int *h_padding, int *v_padding);
void set_vpp_in_padding_axis(u32 enable, int h_padding, int v_padding);
#endif
