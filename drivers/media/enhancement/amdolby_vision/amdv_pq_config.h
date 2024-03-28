/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _AMDV_PQ_H_
#define _AMDV_PQ_H_

#include <linux/types.h>

#define BACKLIGHT_LUT_SIZE  4096
#define BLU_LUT_SIZE 5
#define AMBIENT_LUT_SIZE    8
#define TUNING_LUT_SIZE     14
#define DM4_TUNING_LUT_SIZE 7

#define MAX_DV_PICTUREMODES 40
#define AMBIENT_CFG_FRAMES 46
#define AMBIENT_CFG_FRAMES_2 120
#define VARIABLE_FPS_COUNT 4

# pragma pack(push, 1)
struct tgt_out_csc_cfg {
	s32  lms2rgb_mat[3][3];
	s32  lms2rgb_mat_scale;
	u8   white_point[3];
	u8   white_point_scale;
	s32  reserved[3];
};

#pragma pack(pop)

# pragma pack(push, 1)
struct tgt_gc_cfg {
	s32   gd_enable;
	u32  gd_wmin;
	u32  gd_wmax;
	u32  gd_wmm;
	u32  gd_wdyn_rng_sqrt;
	u32  gd_weight_mean;
	u32  gd_weight_std;
	u32  gd_delay_msec_hdmi;
	s32  gd_rgb2yuv_ext;
	s32  gd_m33_rgb2yuv[3][3];
	s32  gd_m33_rgb2yuv_scale2p;
	s32  gd_rgb2yuv_off_ext;
	s32  gd_rgb2yuv_off[3];
	u32  gd_up_bound;
	u32  gd_low_bound;
	u32  last_max_pq;
	u16  gd_wmin_pq;
	u16  gd_wmax_pq;
	u16  gd_wm_pq;
	u16  gd_trigger_period;
	u32  gd_trigger_lin_thresh;
	u32  gd_delay_msec_ott;
	s16  gd_rise_weight;
	s16  gd_fall_weight;
	u32  gd_delay_msec_ll;
	u32  gd_contrast;
	u32  reserved[3];
};

#pragma pack(pop)

# pragma pack(push, 1)
struct ambient_cfg {
	u8  reserved[3];
	u8  dark_detail;
	u32 dark_detail_complum;
	u32 ambient;
	u32 t_front_lux;
	u32 t_front_lux_scale;
	u32 t_rear_lum;
	u32 t_rear_lum_scale;
	u32 t_whitexy[2];
	u32 t_surround_reflection;
	u32 t_screen_reflection;
	u32 al_delay;
	u32 al_rise;
	u32 al_fall;
};

#pragma pack(pop)

# pragma pack(push, 1)
struct tgt_ab_cfg {
	s32  ab_enable;
	u32  ab_highest_tmax;
	u32  ab_lowest_tmax;
	s16  ab_rise_weight;
	s16  ab_fall_weight;
	u32  ab_delay_msec_hdmi;
	u32  ab_delay_msec_ott;
	u32  ab_delay_msec_ll;
	u32  reserved[1];
};

#pragma pack(pop)

# pragma pack(push, 1)
struct target_config {
	u16 gamma;
	u16 eotf;
	u16 range_spec;
	u16 max_pq;
	u16 min_pq;
	u16 max_pq_dm3;
	u32 min_lin;
	u32 max_lin;
	u32 max_lin_dm3;
	s32 t_primaries[8];
	u16 m_sweight;
	s16 trim_slope_bias;
	s16 trim_offset_bias;
	s16 trim_power_bias;
	s16 ms_weight_bias;
	s16 chroma_weight_bias;
	s16 saturation_gain_bias;
	u16 tuning_mode;
	s16 d_brightness;
	s16 d_contrast;
	s16 d_color_shift;
	s16 d_saturation;
	s16 d_backlight;
	s16 dbg_exec_params_print_period;
	s16 dbg_dm_md_print_period;
	s16 dbg_dm_cfg_print_period;
	struct tgt_gc_cfg gd_config;
	struct tgt_ab_cfg ab_config;
	struct ambient_cfg ambient_config;
	u8 vsvdb[7];
	u8 dm31_avail;
	u8 ref_mode_dark_id;
	u8 apply_l11_wp;
	u8 reserved1[1];
	s16 backlight_scaler;
	struct tgt_out_csc_cfg ocsc_config;
	s16 bright_preservation;
	u8 total_viewing_modes_num;
	u8 viewing_mode_valid;
	u32 ambient_frontlux[AMBIENT_LUT_SIZE];
	u32 ambient_complevel[AMBIENT_LUT_SIZE];
	s16 mid_pq_bias_lut[TUNING_LUT_SIZE];
	s16 slope_bias_lut[TUNING_LUT_SIZE];
	s16 backlight_bias_lut[TUNING_LUT_SIZE];
	s16 user_brightness_ui_lut[DM4_TUNING_LUT_SIZE];
	u16 padding2;
	s16 blu_pwm[5];
	s16 blu_light[5];
	s16 padding[36];
};

#pragma pack(pop)

struct pq_config {
	unsigned char backlight_lut[BACKLIGHT_LUT_SIZE];
	struct target_config tdc;
};

# pragma pack(push, 1)
struct gd_cfg_dvp {
	u8   reserved1[1];
	u8   global_dimming;
	u16  gd_delay_msec_hdmi;
	u16  gd_delay_msec_ott;
	u16  gd_delay_msec_ll;
	u32  gd_lowest_tmax;
	u16  gd_rise_weight;
	u16  gd_fall_weight;
	u32  reserved2[3];
};

#pragma pack(pop)

# pragma pack(push, 1)
struct ambient_cfg_dvp {
	u8   ambient;
	u8   dark_detail;
	u8   reserved1[2];
	u32  dark_detail_complum;
	u16  t_screen_reflection;
	u16  t_surround_reflection;
	u32  t_front_lux;
	u32  t_front_lux_scale;
	u32  t_rear_lum;
	u32  t_rear_lum_scale;
	u32  ambient_front_lux[AMBIENT_LUT_SIZE];
	u32  ambient_comp_level[AMBIENT_LUT_SIZE];
	u16  al_delay;
	u16  al_rise;
	u16  al_fall;
	u16  ac_delay;
	u16  ac_rise;
	u16  ac_fall;
	u16  t_whitexy[2];
	u32  reserved2[3];
};

#pragma pack(pop)

# pragma pack(push, 1)
struct pr_cfg_dvp {
	u8   reserved1[3];
	u8   supports_precision_rendering;
	u16  precision_rendering_strength;
	u16  pyramid_pad_value;
	u16  pyramid_weights[7];
	u16  pyramid_alpha[7];
	u16  local_mapping29_scalar;
	u16  reserved2;
	u32  reserved3[2];
};

#pragma pack(pop)

# pragma pack(push, 1)
struct ana_cfg_dvp {
	s32   analyzer_delay;
	s32   l1_mid_sensitivity;
	s32   l1_mid_slope;
	s32   l1_min_max_slope;
	s32   l1_mid_bot_roll;
	s32   l1_mid_top_roll;
	s32   l1_mid_bot_beta;
	s32   l1_mid_top_beta;
	s32   l4_base_alpha;
	u8    enalbe_l1l4_gen;
	u8    reserved1[3];
	s32   reserved2[3];
};

#pragma pack(pop)

# pragma pack(push, 1)
struct target_config_dvp {
	u32 gamma;
	u32 max;
	u32 min;
	u32 t_primaries[8];
	s32 rgb_to_ycc[3][3];
	s32 rgb_to_ycc_off[3];
	u8  rgb_to_ycc_scale;
	u8  range_spec;
	u8  eotf;
	u8  total_viewing_modes_num;
	u8  mode_valid;
	u8  vsvdb[7];
	s16 user_brightness_ui_lut[DM4_TUNING_LUT_SIZE];
	u16 tuning_mode;
	u16 d_brightness;
	s32 d_contrast;
	s32 d_color_shift;
	s32 d_saturation;
	u16 d_backlight;
	u16 d_local_contrast;
	s16 dbg_exec_params_print_period;
	s16 dbg_dm_md_print_period;
	s16 dbg_dm_cfg_print_period;
	u16 d_brightness_pr_on;
	struct gd_cfg_dvp gd_config;
	struct ambient_cfg_dvp ambient_config;
	struct pr_cfg_dvp pr_config;
	s16 blu_pwm[BLU_LUT_SIZE];
	s16 blu_light[BLU_LUT_SIZE];
	u32 l11_wp_response_rise_fall;
	u8  apply_l11_wp;
	u8  ref_mode_dark_id;
	struct ana_cfg_dvp ana_config;
	u16  reserved2[110];
};

#pragma pack(pop)

struct pq_config_dvp {
	unsigned char backlight_lut[BACKLIGHT_LUT_SIZE];
	struct target_config_dvp tdc;
};

struct dv_cfg_info_s {
	int id;
	char pic_mode_name[32];
	s32  brightness;        /*Brightness */
	s32  contrast;          /*Contrast */
	s32  colorshift;        /*ColorShift or Tint*/
	s32  saturation;        /*Saturation or color */
	u8  vsvdb[7];
	int dark_detail;        /*dark detail, on or off*/
	int light_sense;        /*light sense, on or off*/
	int t_front_lux;
};

struct dv_pq_center_value_s {
	s16  brightness;        /*Brightness */
	s16  contrast;          /*Contrast */
	s16  colorshift;        /*ColorShift or Tint*/
	s16  saturation;        /*Saturation or color */
};

struct dv_pq_range_s {
	s32  left;
	s32  right;
};

struct dv_user_cfg_info {
	int id;
	u32 tmax;
	u32 tmin;            /* 1/1000 */
	u16 tgamma;          /* 1/10 */
	s32 tprimaries[8];   /* 1/10000 */
};

struct dv_user_target_config {
	u16 gamma;
	u16 max_pq;
	u16 min_pq;
	u16 max_pq_dm3;
	u32 min_lin;
	u32 max_lin;
	u32 max_lin_dm3;
	s32 t_primaries[8];
};

extern struct pq_config *bin_to_cfg;
extern struct pq_config_dvp *bin_to_cfg_dvp;
extern struct dv_cfg_info_s cfg_info[MAX_DV_PICTUREMODES];
extern const char *pq_item_str[];
extern struct target_config def_tgt_display_cfg_bestpq;
extern struct target_config def_tgt_display_cfg_ll;
extern int cur_pic_mode;/*current picture mode id*/
extern struct ambient_cfg_s lightsense_test_cfg[2];
extern struct ambient_cfg_s ambient_test_cfg[AMBIENT_CFG_FRAMES];
extern struct ambient_cfg_s ambient_test_cfg_2[AMBIENT_CFG_FRAMES];
extern struct ambient_cfg_s ambient_test_cfg_3[AMBIENT_CFG_FRAMES];
extern struct dynamic_cfg_s dynamic_test_cfg[AMBIENT_CFG_FRAMES];
extern struct dynamic_cfg_s dynamic_test_cfg_2[AMBIENT_CFG_FRAMES];
extern struct dynamic_cfg_s dynamic_test_cfg_3[AMBIENT_CFG_FRAMES];
extern struct dynamic_cfg_s dynamic_test_cfg_4[AMBIENT_CFG_FRAMES_2];
extern struct target_config_dvp def_tgt_dvp_cfg;
extern bool pic_mode_changed;
extern u32 variable_fps[VARIABLE_FPS_COUNT];
extern bool dv_user_cfg_flag;

void restore_dv_pq_setting(enum pq_reset_e pq_reset);
bool load_dv_pq_config_data(char *bin_path, char *txt_path);
bool cp_dv_pq_config_data(void);
bool user_cfg_to_bin(char *user_cfg_data, int user_cfg_size);
void set_pic_mode(int mode);
int get_pic_mode_num(void);
int get_pic_mode(void);
char *get_cur_pic_mode_name(void);
int load_user_cfg_by_name(char *fw_name);

char *get_pic_mode_name(int mode);
s16 get_single_pq_value(int mode, enum pq_item_e item);
struct dv_full_pq_info_s get_full_pq_value(int mode);
void set_single_pq_value(int mode, enum pq_item_e item, s16 value);
void set_full_pq_value(struct dv_full_pq_info_s full_pq_info);

void get_dv_bin_config(void);
int get_dv_pq_info(char *buf);
int set_dv_pq_info(const char *buf, size_t count);
int set_dv_debug_tprimary(const char *buf, size_t count);
int get_dv_debug_tprimary(char *buf);
bool get_load_config_status(void);
void set_load_config_status(bool flag);
int get_inter_pq_flag(void);
void set_inter_pq_flag(int flag);
void set_cfg_id(uint id);
void update_cp_cfg(void);
void update_cp_cfg_hw5(bool update_pyramid, bool is_top1, bool enable);
void update_user_cfg_to_bin(void);
void update_ambient_lightsense(struct ambient_cfg_s *p_ambient);
void calculate_user_pq_config(void);
void get_dv_bin_config_hw5(void);
u32 check_cfg_enabled_top1(void);
u32 check_dynamic_cfg_enabled_top1(void);

#endif
