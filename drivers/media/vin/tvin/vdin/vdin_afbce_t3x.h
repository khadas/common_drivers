/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __VDIN_AFBCE_T3X_H__
#define __VDIN_AFBCE_T3X_H__
#include "vdin_drv.h"
#include "vdin_ctl.h"

#define VDIN_AFBCE_HOLD_LINE_NUM    4

void vdin_afbce_update_t3x(struct vdin_dev_s *devp);
void vdin_afbce_config_t3x(struct vdin_dev_s *devp);
//void vdin_afbce_maptable_init_t3x(struct vdin_dev_s *devp);
void vdin_afbce_set_next_frame_t3x(struct vdin_dev_s *devp,
			       unsigned int rdma_enable, struct vf_entry *vfe);
void vdin_afbce_clear_write_down_flag_t3x(struct vdin_dev_s *devp);
int vdin_afbce_read_write_down_flag_t3x(struct vdin_dev_s *devp);
void vdin_afbce_soft_reset_t3x(struct vdin_dev_s *devp);
void vdin_afbce_mode_update_t3x(struct vdin_dev_s *devp);
//bool vdin_chk_is_comb_mode_t3x(struct vdin_dev_s *devp);
void vdin_pause_afbce_write_t3x(struct vdin_dev_s *devp, unsigned int rdma_enable);
#endif

