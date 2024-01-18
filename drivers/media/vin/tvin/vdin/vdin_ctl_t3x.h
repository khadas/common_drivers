/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __TVIN_VDIN_CTL_T3X_H
#define __TVIN_VDIN_CTL_T3X_H
#include <linux/highmem.h>
#include <linux/page-flags.h>
#include <linux/vmalloc.h>
#include <linux/dma-mapping.h>
#include <linux/dma-map-ops.h>
#include "vdin_drv.h"

//#define CLKCTRL_DSC_CLK_CTRL	0xfe000100
#define DSC_CLK_CTRL_OFFSET	0x0100

/* smc cmd for setting vpu secure reg0 via bl31 */
#define VDIN_SECURE_CFG	0x8200008d

#define VDIN_LITE_CORE_MAX_W	4096
#define VDIN_LITE_CORE_MAX_H	2160
#define VDIN_LITE_CORE_MAX_PIXEL_CLOCK	(VDIN_LITE_CORE_MAX_W * VDIN_LITE_CORE_MAX_H * 60UL)

struct vdin_blkbar_s {
	unsigned int gclk_ctrl;
	unsigned int soft_rst;
	unsigned int conv_num;
	unsigned int hwidth;
	unsigned int vwidth;
	unsigned int row_top;
	unsigned int row_bot;
	unsigned int top_bot_hstart;
	unsigned int top_bot_hend;
	unsigned int top_vstart;
	unsigned int bot_vend;
	unsigned int left_hstart;
	unsigned int right_hend;
	unsigned int lef_rig_vstart;
	unsigned int lef_rig_vend;
	unsigned int black_level;
	unsigned int blk_col_th;
	unsigned int white_level;
	unsigned int top_pos;
	unsigned int bot_pos;
	unsigned int left_pos;
	unsigned int right_pos;
	unsigned int debug_top_cnt;
	unsigned int debug_bot_cnt;
	unsigned int debug_lef_cnt;
	unsigned int debug_rig_cnt;

	unsigned int pat_gclk_ctrl;
	unsigned int pat_bw_flag;
	unsigned int blk_wht_th;
	unsigned int black_th;
	unsigned int white_th;
	unsigned int win_en;
	unsigned int h_bgn;
	unsigned int h_end;
	unsigned int v_bgn;
	unsigned int v_end;
};

struct vdin_pre_hsc_s {
	u32 phsc_gclk_ctrl;
	u32 prehsc_mode;
	u32 prehsc_pattern;
	u32 prehsc_flt_num;
	u32 prehsc_rate;
	u32 prehsc_pat_star;
	u32 prehsc_pat_end;
	u32 preh_hb_num;
	u32 preh_vb_num;
	u32 prehsc_coef_0;
	u32 prehsc_coef_1;
	u32 prehsc_coef_2;
	u32 prehsc_coef_3;
};

extern int vsync_reset_mask;
extern int vdin_ctl_dbg;
extern unsigned int game_mode;
extern unsigned int vdin_force_game_mode;
extern int vdin_dbg_en;
extern unsigned int vdin_pc_mode;
extern int irq_max_count;

/* ************************************************************************ */
/* ******** GLOBAL FUNCTION CLAIM ******** */
/* ************************************************************************ */
u8 *vdin_vmap(ulong addr, u32 size);
void vdin_unmap_phyaddr(u8 *vaddr);
void vdin_dma_flush(struct vdin_dev_s *devp, void *vaddr,
		    int size, enum dma_data_direction dir);
void vdin_set_vframe_prop_info_t3x(struct vframe_s *vf,
			       struct vdin_dev_s *devp);
void vdin_get_crc_val_t3x(struct vframe_s *vf, struct vdin_dev_s *devp);
void vdin_get_format_convert(struct vdin_dev_s *devp);
enum vdin_format_convert_e
	vdin_get_format_convert_matrix0(struct vdin_dev_s *devp);
enum vdin_format_convert_e
	vdin_get_format_convert_matrix1(struct vdin_dev_s *devp);
void vdin_set_prob_xy(unsigned int offset, unsigned int x,
		      unsigned int y, struct vdin_dev_s *devp);
void vdin_prob_get_rgb_t3x(unsigned int offset, unsigned int *r,
		       unsigned int *g, unsigned int *b);
void vdin_set_all_regs_t3x(struct vdin_dev_s *devp);
void vdin_set_double_write_regs_t3x(struct vdin_dev_s *devp);
void vdin_set_default_regmap_t3x(struct vdin_dev_s *devp);
void vdin_set_def_wr_canvas(struct vdin_dev_s *devp);
void vdin_hw_enable_t3x(struct vdin_dev_s *devp);
void vdin_hw_disable_t3x(struct vdin_dev_s *devp);
int vdin_vsync_reset_mif_t3x(int index);
bool vdin_check_vdi6_afifo_overflow_t3x(unsigned int offset);
void vdin_clear_vdi6_afifo_overflow_flg_t3x(unsigned int offset);
void vdin_set_cutwin_t3x(struct vdin_dev_s *devp);
void vdin_set_decimation_t3x(struct vdin_dev_s *devp);
void vdin_fix_nonstd_vsync_t3x(struct vdin_dev_s *devp);
unsigned int vdin_get_meas_h_cnt64_t3x(unsigned int offset);
unsigned int vdin_get_meas_v_stamp_t3x(struct vdin_dev_s *devp);
unsigned int vdin_get_active_h_t3x(unsigned int offset);
unsigned int vdin_get_active_v_t3x(unsigned int offset);
unsigned int vdin_get_total_v_t3x(unsigned int offset);
unsigned int vdin_get_canvas_id_t3x(unsigned int offset);
void vdin_set_canvas_id_t3x(struct vdin_dev_s *devp,
			unsigned int rdma_enable,
			struct vf_entry *vfe);
unsigned int vdin_get_chroma_canvas_id_t3x(unsigned int offset);
void vdin_set_chroma_canvas_id_t3x(struct vdin_dev_s *devp,
			     unsigned int rdma_enable,
			     struct vf_entry *vfe);
void vdin_set_crc_pulse_t3x(struct vdin_dev_s *devp);
void vdin_enable_module_t3x(struct vdin_dev_s *devp, bool enable);
void vdin_set_matrix_t3x(struct vdin_dev_s *devp);
void vdin_select_matrix_t3x(struct vdin_dev_s *devp, unsigned char no,
		      enum vdin_format_convert_e csc);
bool vdin_check_cycle(struct vdin_dev_s *devp);
bool vdin_write_done_check_t3x(unsigned int offset,
			   struct vdin_dev_s *devp);
void vdin_calculate_duration(struct vdin_dev_s *devp);
void vdin_wr_reverse_t3x(unsigned int offset, bool h_reverse,
		     bool v_reverse);
void vdin_set_hv_scale_t3x(struct vdin_dev_s *devp);
void vdin_set_bitdepth_t3x(struct vdin_dev_s *devp);
void vdin_set_cm2_t3x(unsigned int offset, unsigned int w,
		  unsigned int h, unsigned int *data);
void vdin_bypass_isp_t3x(unsigned int offset);
void vdin_set_mpegin_t3x(struct vdin_dev_s *devp);
void vdin_force_go_filed_t3x(struct vdin_dev_s *devp);
void vdin_adjust_tvafe_snow_brightness(void);
void vdin_set_config(struct vdin_dev_s *devp);
void vdin_set_wr_mif_t3x(struct vdin_dev_s *devp);
void vdin_dolby_config_t3x(struct vdin_dev_s *devp);
void vdin_dolby_buffer_update(struct vdin_dev_s *devp,
			      unsigned int index);
void vdin_dolby_addr_update_t3x(struct vdin_dev_s *devp, unsigned int index);
void vdin_dolby_addr_alloc(struct vdin_dev_s *devp, unsigned int size);
void vdin_dolby_addr_release(struct vdin_dev_s *devp, unsigned int size);
int vdin_event_cb(int type, void *data, void *op_arg);
void vdin_hdmiin_patch(struct vdin_dev_s *devp);
void vdin_set_top_t3x(struct vdin_dev_s *devp, enum tvin_port_e port,
		  enum tvin_color_fmt_e input_cfmt, enum bt_path_e bt_path);
void vdin_set_wr_ctrl_vsync_t3x(struct vdin_dev_s *devp,
			    unsigned int offset,
			    enum vdin_format_convert_e format_convert,
			    unsigned int full_pack,
			    unsigned int source_bitdepth,
			    unsigned int rdma_enable);
void vdin_urgent_patch_t3x(unsigned int offset, unsigned int v, unsigned int h);
void vdin_urgent_patch_resume_t3x(unsigned int offset);
int vdin_hdr_sei_error_check(struct vdin_dev_s *devp);
void vdin_set_drm_data(struct vdin_dev_s *devp,
		       struct vframe_s *vf);
void vdin_set_source_bitdepth(struct vdin_dev_s *devp,
			      struct vframe_s *vf);
void vdin_source_bitdepth_reinit(struct vdin_dev_s *devp);
void set_invert_top_bot(bool invert_flag);
void vdin_clk_on_off_t3x(struct vdin_dev_s *devp, bool on_off);

extern enum tvin_force_color_range_e color_range_force;

//void vdin_vlock_input_sel(unsigned int type,
//			  enum vframe_source_type_e source_type);
void vdin_set_dv_tunnel_t3x(struct vdin_dev_s *devp);
void vdin_dolby_mdata_write_en_t3x(unsigned int offset, unsigned int en);
void vdin_prob_set_xy_t3x(unsigned int offset,
		      unsigned int x, unsigned int y, struct vdin_dev_s *devp);
void vdin_prob_set_before_or_after_mat(unsigned int offset,
				       unsigned int x,
				       struct vdin_dev_s *devp);
void vdin_prob_get_yuv_t3x(unsigned int offset,
		       unsigned int *rgb_yuv0,	unsigned int *rgb_yuv1,
		       unsigned int *rgb_yuv2);
void vdin_prob_matrix_sel_t3x(unsigned int offset,
			  unsigned int sel, struct vdin_dev_s *devp);
void vdin_change_matrix(unsigned int offset,
			unsigned int matrix_csc);
void vdin_dolby_desc_sc_enable(struct vdin_dev_s *devp,
			       unsigned int  on_off);
bool vdin_is_dolby_tunnel_444_input(struct vdin_dev_s *devp);
bool vdin_is_dolby_signal_in(struct vdin_dev_s *devp);
void vdin_dolby_de_tunnel_to_12bit(struct vdin_dev_s *devp,
				   unsigned int on_off);
void vdin_wr_frame_en(unsigned int ch, unsigned int on_off);
void vdin_set_mif_on_off_t3x(struct vdin_dev_s *devp, unsigned int rdma_enable);
enum tvin_color_fmt_range_e
	tvin_get_force_fmt_range(enum tvin_color_fmt_e color_fmt);
bool vdin_is_convert_to_444(u32 format_convert);
bool vdin_is_convert_to_422(u32 format_convert);
bool vdin_is_convert_to_nv21(u32 format_convert);
bool vdin_is_4k(struct vdin_dev_s *devp);
void vdin_set_matrix_color_t3x(struct vdin_dev_s *devp);
void vdin_set_bist_pattern_t3x(struct vdin_dev_s *devp, unsigned int on_off, unsigned int pat);

bool is_amdv_enable(void);
bool is_amdv_on(void);
bool is_amdv_stb_mode(void);
bool for_amdv_certification(void);

void vdin_change_matrix0_t3x(u32 offset, u32 matrix_csc);
void vdin_change_matrix1_t3x(u32 offset, u32 matrix_csc);
void vdin_change_matrix_hdr_t3x(u32 offset, u32 matrix_csc);

void vdin_set_frame_mif_write_addr_t3x(struct vdin_dev_s *devp,
			unsigned int rdma_enable, struct vf_entry *vfe);
void vdin_dv_pr_meta_data(void *addr, unsigned int size, unsigned int index);
bool vdin_is_dv_meta_data_case(struct vdin_dev_s *devp);
void vdin_dv_tunnel_set_t3x(struct vdin_dev_s *devp);
void vdin_descramble_setting_t3x(struct vdin_dev_s *devp, unsigned int on_off);
void vdin_scramble_setting_t3x(struct vdin_dev_s *devp, unsigned int on_off);
void vdin_get_duration_by_fps(struct vdin_dev_s *devp);
void vdin_set_to_vpp_parm(struct vdin_dev_s *devp);
void vdin_dmc_ctrl(struct vdin_dev_s *devp, bool on_off);
void vdin_pause_mif_write_t3x(struct vdin_dev_s *devp, unsigned int rdma_enable);
bool vdin_check_is_spd_data(struct vdin_dev_s *devp);
void vdin_sw_reset_t3x(struct vdin_dev_s *devp);
void vdin_bist_t3x(struct vdin_dev_s *devp, unsigned int mode);
bool vdin_is_wrmif_done_t3x(struct vdin_dev_s *devp);
void vdin_clr_write_done_t3x(struct vdin_dev_s *devp);
unsigned int vdin_get_div_t3x(struct vdin_dev_s *devp);
void vdin_set_scl_mode_t3x(struct vdin_dev_s *devp, bool on_off);
void vdin_set_dsc_config_t3x(struct vdin_dev_s *devp, bool on_off);

#endif
