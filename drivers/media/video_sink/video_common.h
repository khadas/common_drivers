/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * drivers/amlogic/media/video_sink/video_common.h
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

#ifndef VIDEO_COMMON_HH
#define VIDEO_COMMON_HH
#include "video_priv.h"

extern int debug_common_flag;
#define DEBUG_FLAG_COMMON_FRC      BIT(0)
#define DEBUG_FLAG_COMMON_AISR     BIT(1)
#define DEBUG_FLAG_COMMON_FG       BIT(2)
#define DEBUG_FLAG_COMMON_FG_MORE  BIT(3)
#define DEBUG_FLAG_COMMON_AMDV     BIT(4)

u32 is_crop_left_odd(struct vpp_frame_par_s *frame_par);
void get_pre_hscaler_para(u8 layer_id, int *ds_ratio, int *flt_num);
void get_pre_vscaler_para(u8 layer_id, int *ds_ratio, int *flt_num);
void get_pre_hscaler_coef(u8 layer_id, int *pre_hscaler_table);
void get_pre_vscaler_coef(u8 layer_id, int *pre_hscaler_table);
u32 viu_line_stride(u32 buffr_width);
u32 viu_line_stride_ex(u32 buffr_width, u8 plane_bits);
void init_layer_canvas(struct video_layer_s *layer,
			      u32 start_canvas);
void vframe_canvas_set(struct canvas_config_s *config,
	u32 planes,
	u32 *index);
bool is_layer_aisr_supported(struct video_layer_s *layer);
ssize_t reg_dump_store(struct class *cla,
				 struct class_attribute *attr,
				const char *buf, size_t count);
bool frc_n2m_worked(void);
bool frc_n2m_1st_frame_worked(struct video_layer_s *layer);
bool check_aisr_need_disable(struct video_layer_s *layer);
bool is_aisr_enable(struct video_layer_s *layer);
#ifndef CONFIG_AMLOGIC_VIDEO_COMPOSER
bool get_lowlatency_mode(void);
#endif
#endif
