/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2021 Amlogic, Inc. All rights reserved.
 */

#include <linux/amlogic/media/vicp/vicp.h>

#define FILM_GRAIN_LUT_DATA_SIZE (498 * 4)

/* *********************************************************************** */
/* ************************* enum definitions ****************************.*/
/* *********************************************************************** */
enum vicp_input_path_e {
	VICP_INPUT_PATH_RDMIF = 0,
	VICP_INPUT_PATH_AFBCD,
	VICP_INPUT_PATH_MAX,
};

enum vicp_output_path_e {
	VICP_OUTPUT_PATH_WRMIF = 1,
	VICP_OUTPUT_PATH_AFBCE,
	VICP_OUTPUT_PATH_ALL,
	VICP_OUTPUT_PATH_MAX,
};

enum rdmif_baseaddr_type_e {
	RDMIF_BASEADDR_TYPE_Y = 0,
	RDMIF_BASEADDR_TYPE_CB,
	RDMIF_BASEADDR_TYPE_CR,
	RDMIF_BASEADDR_TYPE_MAX,
};

enum rdmif_stride_type_e {
	RDMIF_STRIDE_TYPE_Y = 0,
	RDMIF_STRIDE_TYPE_CB,
	RDMIF_STRIDE_TYPE_CR,
	RDMIF_STRIDE_TYPE_MAX,
};

/* *********************************************************************** */
/* ************************* struct definitions **************************.*/
/* *********************************************************************** */
struct vicp_rdmif_general_reg_s {
	u32 enable_free_clk;
	u32 reset_on_go_field;
	u32 urgent_chroma;
	u32 urgent_luma;
	u32 chroma_end_at_last_line;
	u32 luma_end_at_last_line;
	u32 hold_lines;
	u32 last_line_mode;
	u32 ro_busy;
	u32 demux_mode;
	u32 bytes_per_pixel;
	u32 ddr_burst_size_cr;
	u32 ddr_burst_size_cb;
	u32 ddr_burst_size_y;
	u32 chroma_rpt_lastl;
	u32 little_endian;
	u32 chroma_hz_avg;
	u32 luma_hz_avg;
	u32 set_separate_en;
	u32 enable;
};

struct vicp_rdmif_general_reg2_s {
	u32 chroma_line_read_sel;
	u32 luma_line_read_sel;
	u32 shift_pat_cr;
	u32 shift_pat_cb;
	u32 shift_pat_y;
	u32 hold_lines;
	u32 y_rev1;
	u32 x_rev1;
	u32 y_rev0;
	u32 x_rev0;
	u32 color_map;
};

struct vicp_rdmif_general_reg3_s {
	u32 f0_stride32aligned2;
	u32 f0_stride32aligned1;
	u32 f0_stride32aligned0;
	u32 f0_cav_blk_mode2;
	u32 f0_cav_blk_mode1;
	u32 f0_cav_blk_mode0;
	u32 abort_mode;
	u32 dbg_mode;
	u32 bits_mode;
	u32 block_len;
	u32 burst_len;
	u32 bit_swap;
};

struct vicp_rdmif_rpt_loop_s {
	u32 rpt_loop1_chroma_start;
	u32 rpt_loop1_chroma_end;
	u32 rpt_loop1_luma_start;
	u32 rpt_loop1_luma_end;
	u32 rpt_loop0_chroma_start;
	u32 rpt_loop0_chroma_end;
	u32 rpt_loop0_luma_start;
	u32 rpt_loop0_luma_end;
};

struct vicp_rdmif_color_format_s {
	u32 cfmt_gclk_bit_dis;
	u32 cfmt_soft_rst_bit;
	u32 cfmt_h_rpt_pix;
	u32 cfmt_h_ini_phase;
	u32 cfmt_h_rpt_p0_en;
	u32 cfmt_h_yc_ratio;
	u32 cfmt_h_en;
	u32 cfmt_v_phase0_always_en;
	u32 cfmt_v_rpt_last_dis;
	u32 cfmt_v_phase0_nrpt_en;
	u32 cfmt_v_rpt_line0_en;
	u32 cfmt_v_skip_line_num;
	u32 cfmt_v_ini_phase;
	u32 cfmt_v_phase_step;
	u32 cfmt_v_en;
};

struct vicp_wrmif_control_s {
	u32 wrmif_en;
	u32 bit10_mode;
	u32 little_endian;
	u32 data_ext_ena;
	u32 word_limit;
	u32 urgent;
	u32 swap_cbcr;
	u32 v_conv;
	u32 h_conv;
	u32 rgb_mode;
	u32 gate_clock_en;
	u32 canvas_sync_en;
	u32 burst_limit;
	u32 swap_64bits_en;
};

struct vicp_afbcd_mode_reg_s {
	u32 ddr_sz_mode;
	u32 blk_mem_mode;
	u32 rev_mode;
	u32 mif_urgent;
	u32 hold_line_num;
	u32 burst_len;
	u32 compbits_yuv;
	u32 vert_skip_y;
	u32 horz_skip_y;
	u32 vert_skip_uv;
	u32 horz_skip_uv;
};

struct vicp_afbcd_general_reg_s {
	u32 gclk_ctrl_core;
	u32 fmt_size_sw_mode;
	u32 addr_link_en;
	u32 fmt444_comb;
	u32 dos_uncomp_mode;
	u32 soft_rst;
	u32 ddr_blk_size;
	u32 cmd_blk_size;
	u32 dec_enable;
	u32 head_len_sel;
	u32 reserved;
};

struct vicp_afbcd_cfmt_control_reg_s {
	u32 gclk_bit_dis;
	u32 soft_rst_bit;
	u32 cfmt_h_rpt_pix;
	u32 cfmt_h_ini_phase;
	u32 cfmt_h_rpt_p0_en;
	u32 cfmt_h_yc_ratio;
	u32 cfmt_h_en;
	u32 cfmt_v_phase0_en;
	u32 cfmt_v_rpt_last_dis;
	u32 cfmt_v_phase0_nrpt_en;
	u32 cfmt_v_rpt_line0_en;
	u32 cfmt_v_skip_line_num;
	u32 cfmt_v_ini_phase;
	u32 cfmt_v_phase_step;
	u32 cfmt_v_en;
};

struct vicp_afbcd_quant_control_reg_s {
	u32 quant_expand_en_1;
	u32 quant_expand_en_0;
	u32 bcleav_offsst;
	u32 lossy_chrm_en;
	u32 lossy_luma_en;

};

struct vicp_afbcd_rotate_control_reg_s {
	u32 out_ds_mode_h;
	u32 out_ds_mode_v;
	u32 pip_mode;
	u32 uv_shrk_drop_mode_v;
	u32 uv_shrk_drop_mode_h;
	u32 uv_shrk_ratio_v;
	u32 uv_shrk_ratio_h;
	u32 y_shrk_drop_mode_v;
	u32 y_shrk_drop_mode_h;
	u32 y_shrk_ratio_v;
	u32 y_shrk_ratio_h;
	u32 uv422_drop_mode;
	u32 out_fmt_for_uv422;
	u32 enable;
};

struct vicp_afbcd_rotate_scope_reg_s {
	u32 debug_probe;
	u32 out_ds_mode;
	u32 out_fmt_down_mode;
	u32 in_fmt_force444;
	u32 out_fmt_mode;
	u32 out_compbits_y;
	u32 out_compbits_uv;
	u32 win_bgn_v;
	u32 win_bgn_h;
};

struct vicp_afbcd_lossy_ctrl_reg_s {
	u32 fix_cr_en;			//unsigned ,    RW  default = 0
	u32 quant_diff_root_leave;	//unsigned ,    RW  default = 2
};

struct vicp_afbcd_burst_ctrl_reg_s {
	u32 ofset_burst4_en;		//unsigned ,	RW  default = 0
	u32 burst_length_add_en;	//unsigned ,	RW  default = 0
	u32 burst_length_add_value;	//unsigned ,	RW  default = 2
};

struct vicp_afbce_mode_reg_s {
	u32 soft_rst;
	u32 rev_mode;
	u32 mif_urgent;
	u32 hold_line_num;
	u32 burst_mode;
	u32 fmt444_comb;
};

struct vicp_afbce_mif_size_reg_s {
	u32 ddr_blk_size;	//Bit 29:28    default = 1
	u32 cmd_blk_size;	//Bit 26:24    default = 3
	u32 uncmp_size;		//Bit 23:16    default = 20
	u32 mmu_page_size;	//Bit 15: 0    default = 4096
};

struct vicp_afbce_mmu_rmif_control1_reg_s {
	u32 sync_sel;
	u32 canvas_id;
	u32 cmd_intr_len;
	u32 cmd_req_size;
	u32 burst_len;
	u32 swap_64bit;
	u32 little_endian;
	u32 y_rev;
	u32 x_rev;
	u32 pack_mode;
};

struct vicp_afbce_mmu_rmif_control2_reg_s {
	u32 sw_rst;
	u32 int_clr;
	u32 gclk_ctrl;
	u32 urgent_ctrl;
};

struct vicp_afbce_mmu_rmif_control3_reg_s {
	u32 vstep;
	u32 acc_mode;
	u32 stride;
};

struct vicp_afbce_pip_control_reg_s {
	u32 enc_align_en;
	u32 pip_ini_ctrl;
	u32 pip_mode;
};

struct vicp_afbce_enable_reg_s {
	u32 gclk_ctrl;
	u32 afbce_sync_sel;
	u32 enc_rst_mode;
	u32 enc_en_mode;
	u32 enc_enable;
	u32 pls_enc_frm_start;
};

struct vicp_pre_scaler_ctrl_reg_s {
	u32 preh_hb_num;
	u32 preh_vb_num;
	u32 sc_coef_s11_mode;
	u32 vsc_nor_rs_bits;
	u32 hsc_nor_rs_bits;
	u32 prehsc_flt_num;
	u32 prevsc_flt_num;
	u32 prehsc_rate;
	u32 prevsc_rate;
};

struct vicp_vsc_phase_ctrl_reg_s {
	u32 double_line_mode;
	u32 prog_interlace;
	u32 bot_l0_out_en;
	u32 bot_rpt_l0_num;
	u32 bot_ini_rcv_num;
	u32 top_l0_out_en;
	u32 top_rpt_l0_num;
	u32 top_ini_rcv_num;
};

struct vicp_hsc_phase_ctrl_reg_s {
	u32 ini_rcv_num0_exp;
	u32 rpt_p0_num0;
	u32 ini_rcv_num0;
	u32 ini_phase0;
};

struct vicp_scaler_misc_reg_s {
	u32 sc_din_mode;
	u32 reg_l0_out_fix;
	u32 hf_sep_coef_4srnet_en;
	u32 repeat_last_line_en;
	u32 old_prehsc_en;
	u32 hsc_len_div2_en;
	u32 prevsc_lbuf_mode;
	u32 prehsc_en;
	u32 prevsc_en;
	u32 vsc_en;
	u32 hsc_en;
	u32 sc_top_en;
	u32 sc_vd_en;
	u32 hsc_nonlinear_4region_en;
	u32 hsc_bank_length;
	u32 vsc_phase_field_mode;
	u32 vsc_nonlinear_4region_en;
	u32 vsc_bank_length;
};

struct vicp_fgrain_ctrl_reg_s {
	u32 sync_ctrl;
	u32 dma_st_clr;//clear DMA error status
	u32 hold4dma_scale;
	u32 hold4dma_tbl;
	u32 cin_uv_swap; //default = 0 1 to swap U/V input
	u32 cin_rev; // default = 0 1 to reverse the U/V input order
	u32 yin_rev;//default = 0 1 to reverse the Y input order
	u32 fgrain_ext_imode;//default = 0 1 to indicate the input data is *4 in 8bit mode
	u32 use_par_apply_fgrain;//default = 0 1 to use apply_fgrain from DMA table
	u32 fgrain_last_ln_mode;//keep fgrain noise generator though the input is finished for rdmif
	u32 fgrain_use_sat4bp;//default = 0 1 to use fgain_max/min for sat not
	u32 apply_c_mode; //default = 1 0 to following C
	u32 fgrain_tbl_sign_mode; //default=1 0 to indicate signed bit is not extended in 8bit mode
	u32 fgrain_tbl_ext_mode; //default = 1 0 to indicate the grain table is *4 in 8bit mode
	u32 fmt_mode; //default = 2 0:444; 1:422; 2:420; 3:reserved
	u32 comp_bits; //default = 1 0:8bits; 1:10bits, else 12 bits
	u32 rev_mode; //default = 0 0:h_rev; 1:v_rev;
	u32 block_mode; //default = 1
	u32 fgrain_loc_en; //default = 0 frame-based  fgrain enable
	u32 fgrain_glb_en; //default = 0 global-based fgrain enable
};

struct vicp_afbce_loss_ctrl_reg_s {
	u32 fix_cr_en;             //Bit 31    default = 1  enable of fix CR
	u32 rc_en;                 //Bit 30    default = 0  enable of rc
	u32 write_qlevel_mode;     //Bit 29    default = 1  qlevel write mode
	u32 debug_qlevel_en;       //Bit 28    default = 0  debug en
	u32 cal_bit_early_mode;    //Bit 17:16 default = 2  early mode, RTL fix 2
	u32 quant_diff_root_leave; //Bit 15:12 default = 2  diff value of bctroot and bcleave
	u32 debug_qlevel_value;    //Bit 11: 8 default = 0  debug qlevel value
	u32 debug_qlevel_max_0;    //Bit  7: 4 default = 10  debug qlevel value
	u32 debug_qlevel_max_1;    //Bit  3: 0 default = 10  debug qlevel value
};

struct vicp_afbce_format_reg_s {
	u32 burst_length_add_value;  //default = 2
	u32 ofset_burst4_en;         //default = 0
	u32 burst_length_add_en;     //default = 0
	u32 format_mode;             //default = 2  data format 0:YUV444 1:YUV422 2:YUV420 3:RGB
	u32 compbits_c;              //default = 10  chroma bitwidth
	u32 compbits_y;
};
/* ***********************************************************************.*/
/* ************************* function definitions ************************.*/
/* ***********************************************************************.*/
void init_vicp_module_reg(enum meson_cpuid_type_e cpuid);
void set_module_enable(u32 is_enable);
void set_module_start(u32 is_start);
void set_module_reset(void);
void set_rdmif_enable(u32 is_enable);
void set_afbcd_enable(u32 is_enable);
void set_input_path(enum vicp_input_path_e path);
void set_input_size(u32 size_v, u32 size_h);
void set_output_size(u32 size_v, u32 size_h);
void set_afbcd_4k_enable(u32 is_enable);
void set_afbcd_input_size(u32 size_h, u32 size_v);
void set_afbcd_default_color(u32 def_color_y, u32 def_color_u, u32 def_color_v);
void set_afbcd_mode(struct vicp_afbcd_mode_reg_s afbcd_mode);
void set_afbcd_conv_control(enum vicp_color_format_e fmt_mode, u32 lbuf_len);
void set_afbcd_lbuf_depth(u32 dec_lbuf_depth, u32 mif_lbuf_depth);
void set_afbcd_headaddr(u64 addr);
void set_afbcd_bodyaddr(u64 addr);
void set_afbcd_mif_scope(u32 v_h_flag, u32 scope_begin, u32 scope_end);
void set_afbcd_pixel_scope(u32 v_h_flag, u32 scope_begin, u32 scope_end);
void set_afbcd_general(struct vicp_afbcd_general_reg_s reg);
void set_afbcd_colorformat_control(struct vicp_afbcd_cfmt_control_reg_s cfmt_control);
void set_afbcd_colorformat_w(u32 hz_fmt_w, u32 vt_fmt_w);
void set_afbcd_colorformat_h(u32 fmt_size_h);
void set_afbcd_quant_control(struct vicp_afbcd_quant_control_reg_s quant_control);
void set_afbcd_rotate_control(struct vicp_afbcd_rotate_control_reg_s rotate_control);
void set_afbcd_rotate_scope(struct vicp_afbcd_rotate_scope_reg_s rotate_scope);
void set_afbcd_loss_control(struct vicp_afbcd_lossy_ctrl_reg_s lossy_ctrl_reg);
void set_afbcd_burst_control(struct vicp_afbcd_burst_ctrl_reg_s burst_ctrl_reg);
void set_afbce_path_enable(u32 is_enable);
void set_afbce_lossy_luma_enable(u32 enable);
void set_afbce_lossy_chrm_enable(u32 enable);
void set_afbce_lossy_control(struct vicp_afbce_loss_ctrl_reg_s loss_ctrl_reg);
void set_afbce_lossy_brust_num(u32 num0, u32 num1, u32 num2, u32 num3);
void set_afbce_format(struct vicp_afbce_format_reg_s format_reg);
void set_afbce_ofset_burst4_en(u32 enable);
void set_afbce_input_size(u32 size_h, u32 size_v);
void set_afbce_blk_size(u32 size_h, u32 size_v);
void set_afbce_head_addr(u64 head_addr);
void set_afbce_mif_size(struct vicp_afbce_mif_size_reg_s mif_size_reg);
void set_afbce_mix_scope(u32 h_or_v, u32 begin, u32 end);
void set_afbce_conv_control(u32 fmt_ybuf_depth, u32 lbuf_depth);
void set_afbce_pixel_in_scope(u32 h_or_v, u32 begin, u32 end);
void set_afbce_mode(struct vicp_afbce_mode_reg_s afbce_mode_reg);
void set_afbce_default_color1(u32 def_color_a, u32 def_color_y);
void set_afbce_default_color2(u32 def_color_u, u32 def_color_v);
void set_afbce_mmu_rmif_scope(u32 x_or_y, u32 start, u32 end);
void set_afbce_mmu_rmif_control1(struct vicp_afbce_mmu_rmif_control1_reg_s rmif_control1);
void set_afbce_mmu_rmif_control2(struct vicp_afbce_mmu_rmif_control2_reg_s rmif_control2);
void set_afbce_mmu_rmif_control3(struct vicp_afbce_mmu_rmif_control3_reg_s rmif_control3);
void set_afbce_mmu_rmif_control4(u64 baddr);
void set_afbce_pip_control(struct vicp_afbce_pip_control_reg_s pip_control);
void set_afbce_rotation_control(u32 rotation_en, u32 step_v);
void set_afbce_enable(struct vicp_afbce_enable_reg_s enable_reg);
void set_afbce_start(u32 is_start);
void set_rdmif_luma_fifo_size(u32 size);
void set_rdmif_general_reg(struct vicp_rdmif_general_reg_s reg);
void set_rdmif_general_reg2(struct vicp_rdmif_general_reg2_s reg);
void set_rdmif_general_reg3(struct vicp_rdmif_general_reg3_s reg);
void set_rdmif_base_addr(enum rdmif_baseaddr_type_e addr_sype, u64 base_addr);
void set_rdmif_stride(enum rdmif_stride_type_e stride_sype, u32 stride_value);
void set_rdmif_rpt_loop(struct vicp_rdmif_rpt_loop_s rpt_loop);
void set_rdmif_luma_rpt_pat(u32 luma_index, u32 value);
void set_rdmif_chroma_rpt_pat(u32 chroma_index, u32 value);
void set_rdmif_dummy_pixel(u32 value);
void set_rdmif_color_format_control(struct vicp_rdmif_color_format_s color_format);
void set_rdmif_color_format_width(u32 cfmt_width_h, u32 cfmt_width_v);
void set_rdmif_luma_position(u32 index, u32 is_x, u32 start, u32 end);
void set_rdmif_chroma_position(u32 index, u32 is_x, u32 start, u32 end);
void set_wrmif_path_enable(u32 is_enable);
void set_wrmif_shrk_enable(u32 is_enable);
void set_wrmif_shrk_size(u32 size);
void set_wrmif_shrk_mode_h(u32 mode);
void set_wrmif_shrk_mode_v(u32 mode);
void set_wrmif_base_addr(u32 index, u64 addr);
void set_wrmif_stride(u32 is_y, u32 value);
void set_wrmif_range(u32 is_x, u32 rev, u32 start, u32 end);
void set_wrmif_control(struct vicp_wrmif_control_s wrmif_control);
void set_crop_enable(u32 is_enable);
void set_crop_holdline(u32 hold_line);
void set_crop_dimm(u32 dimm_layer_en, u32 dimm_data);
void set_crop_size_in(u32 size_h, u32 size_v);
void set_crop_scope_h(u32 begain, u32 end);
void set_crop_scope_v(u32 begain, u32 end);
void set_top_holdline(void);
void set_pre_scaler_control(struct vicp_pre_scaler_ctrl_reg_s pre_scaler_ctrl_reg);
void set_scaler_coef_param(u32 coef_index);
void set_vsc_region12_start(u32 region1_start, u32 region2_start);
void set_vsc_region34_start(u32 region3_start, u32 region4_start);
void set_vsc_region4_end(u32 region4_end);
void set_vsc_start_phase_step(u32 step);
void set_vsc_region_phase_slope(u32 region_num, u32 slop_val);
void set_vsc_phase_control(struct vicp_vsc_phase_ctrl_reg_s vsc_phase_ctrl_reg);
void set_vsc_ini_phase(u32 bot_ini_phase, u32 top_ini_phase);
void set_hsc_region12_start(u32 region1_start, u32 region2_start);
void set_hsc_region34_start(u32 region3_start, u32 region4_start);
void set_hsc_region4_end(u32 region4_end);
void set_hsc_start_phase_step(u32 step);
void set_hsc_region_phase_slope(u32 region_num, u32 slop_val);
void set_hsc_phase_control(struct vicp_hsc_phase_ctrl_reg_s hsc_phase_ctrl_reg);
void set_scaler_misc(struct vicp_scaler_misc_reg_s scaler_misc_reg);
void set_rdma_start(u32 input_count);
void set_rdma_flag(u32 is_enable);
void set_security_enable(u32 dma_en, u32 mmu_en, u32 input_en);
void set_fgrain_control(struct vicp_fgrain_ctrl_reg_s fgrain_ctrl_reg);
void set_fgrain_window_h(u32 begain, u32 end);
void set_fgrain_window_v(u32 begain, u32 end);
void set_fgrain_slice_window_h(u32 begain, u32 end);
void set_fgrain_lut_data(u32 val);
int read_vicp_reg(u32 reg);
void write_vicp_reg(u32 reg, u32 val);
void write_vicp_reg_bits(u32 reg, const u32 value, const u32 start, const u32 len);
