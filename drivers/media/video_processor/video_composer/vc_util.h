/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef VC_UTIL_H
#define VC_UTIL_H

#include <linux/amlogic/media/vfm/vframe.h>

enum videocom_source_type {
	DECODER_8BIT_NORMAL = 0,
	DECODER_8BIT_BOTTOM,
	DECODER_8BIT_TOP,
	DECODER_10BIT_NORMAL,
	DECODER_10BIT_BOTTOM,
	DECODER_10BIT_TOP,
	VDIN_8BIT_NORMAL,
	VDIN_10BIT_NORMAL,
};

enum com_buffer_used {
	USED_UNINITIAL = 0,
	USED_BY_GE2D,
	USED_BY_DEWARP,
};

struct dst_buf_t {
	int index;
	struct vframe_s frame;
	struct composer_info_t componser_info;
	enum com_buffer_used buf_used;
	bool dirty;
	u32 phy_addr;
	u32 buf_w;
	u32 buf_h;
	u32 buf_size;
	bool is_tvp;
	u32 dw_size;
	ulong afbc_head_addr;
	u32 afbc_head_size;
	ulong afbc_body_addr;
	u32 afbc_body_size;
	ulong afbc_table_addr;
	ulong afbc_table_handle;
	u32 afbc_table_size;
};

struct composer_vf_para {
	int src_vf_format;
	int src_vf_width;
	int src_vf_height;
	int src_vf_plane_count;
	int src_vf_angle;
	u32 src_buf_addr0;
	int src_buf_stride0;
	u32 src_buf_addr1;
	int src_buf_stride1;
	int src_endian;
	int dst_vf_format;
	int dst_vf_width;
	int dst_vf_height;
	int dst_vf_plane_count;
	u32 dst_buf_addr;
	int dst_buf_stride;
	int dst_endian;
	bool is_tvp;
};

struct pic_struct_t {
	int format;
	u32 width;
	u32 height;
	int addr[2];
	u32 align_w;
	u32 align_h;
	int plane_count;
	bool is_tvp;
};

struct composer_input_para {
	int call_index;
	struct vframe_s *vframe;
	struct pic_struct_t pic_info;
	int transform;
};

struct composer_output_para {
	struct pic_struct_t pic_info;
};

struct composer_common_para {
	struct composer_input_para input_para;
	struct composer_output_para output_para;
};

#endif
