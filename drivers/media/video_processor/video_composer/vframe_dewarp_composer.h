/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef VFRAME_DEWARP_COMPOSER_H
#define VFRAME_DEWARP_COMPOSER_H
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/amlogic/media/vout/vout_notify.h>
#include <linux/amlogic/media/vfm/vframe.h>
#include <linux/amlogic/media/gdc/gdc.h>
#include "vc_util.h"

struct dewarp_composer_para {
	int vc_index;
	struct gdc_context_s *context;
	struct firmware_load_s fw_load;
	struct composer_vf_para *vf_para;
};

extern u32 dewarp_load_flag;
int get_dewarp_format(int vc_index, struct vframe_s *vf);
int load_dewarp_firmware(struct dewarp_composer_para *param);
int unload_dewarp_firmware(struct dewarp_composer_para *param);
bool is_dewarp_supported(int vc_index, struct composer_vf_para *vf_param);
int init_dewarp_composer(struct dewarp_composer_para *param);
int uninit_dewarp_composer(struct dewarp_composer_para *param);
int config_dewarp_vframe(int vc_index, struct vframe_s *src_vf, int transform,
		struct dst_buf_t *dst_buf, struct composer_vf_para *vframe_para,
		unsigned long addr, u32 buf_w, u32 buf_h, u32 data_w, int buf_format);
int dewarp_data_composer(struct dewarp_composer_para *param);

#endif
