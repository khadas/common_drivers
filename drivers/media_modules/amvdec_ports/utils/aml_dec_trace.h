/*
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Description:
 */
#ifndef _AML_DEC_TRACE_H_
#define _AML_DEC_TRACE_H_

#include <trace/events/meson_atrace.h>

#define VTRACE_GROUP_V4L_DEC	(1)
#define VTRACE_GROUP_V4L_GE2D	(2)
#define VTRACE_GROUP_V4L_VPP	(4)
#define VTRACE_GROUP_DVB_DEC	(8)
#define VTRACE_GROUP_DVB_VTP	(0x10)
#define VTRACE_GROUP_DVB_VDO	(0x20)
#define VTRACE_GROUP_DVB_VOUT	(0x40)
#define VTRACE_GROUP_WRP	(0x80)
#define VTRACE_GROUP_WRP_GE2D	(0x100)
#define VTRACE_GROUP_WRP_VPP	(0x200)
#define VTRACE_GROUP_WRP_SINK	(0x200)
#define VTRACE_GROUP_DEC_CORE	(0x400)

enum vdec_trace_type_list {
	/* v4l wrapper. */
	VTRACE_V4L_ES_0,	/* que.                  */
	VTRACE_V4L_ES_1,	/* que_again.            */
	VTRACE_V4L_ES_2,	/* write.                */
	VTRACE_V4L_ES_3,	/* write_secure.         */
	VTRACE_V4L_ES_4,	/* receive.              */
	VTRACE_V4L_ES_5,	/* submit.               */
	VTRACE_V4L_ES_6,	/* deque.                */
	VTRACE_V4L_ES_7,	/* deque_again.          */
	VTRACE_V4L_ES_8,	/* write_end.            */
	VTRACE_V4L_ES_9,	/* write_secure_end.     */
	VTRACE_V4L_ES_10,	/* write_error.          */
	VTRACE_V4L_ES_11,	/* write_again.          */
	VTRACE_V4L_ES_12,	/* in PTS.	 	 */

	VTRACE_V4L_PIC_0,	/* que.                  */
	VTRACE_V4L_PIC_1,	/* que_again.            */
	VTRACE_V4L_PIC_2,	/* storage.              */
	VTRACE_V4L_PIC_3,	/* require.              */
	VTRACE_V4L_PIC_4,	/* recycle.              */
	VTRACE_V4L_PIC_5,	/* receive.              */
	VTRACE_V4L_PIC_6,	/* submit.               */
	VTRACE_V4L_PIC_7,	/* deque.                */
	VTRACE_V4L_PIC_8,	/* deque_again.          */
	VTRACE_V4L_PIC_9,	/* out PTS.		 */
	VTRACE_V4L_PIC_10,	/* vsink_cache_num.		 */

	VTRACE_V4L_ST_0,	/* state.                */
	VTRACE_V4L_ST_1,	/* input_buffering.      */

	/* ge2d wrapper. */
	VTRACE_GE2D_PIC_0,	/* vf_put.               */
	VTRACE_GE2D_PIC_1,	/* recycle.              */
	VTRACE_GE2D_PIC_2,	/* receive.              */
	VTRACE_GE2D_PIC_3,	/* handle_start.         */
	VTRACE_GE2D_PIC_4,	/* submit.               */
	VTRACE_GE2D_PIC_5,	/* vf_get.               */
	VTRACE_GE2D_PIC_6,	/* ge2d_cache_num.              */

	VTRACE_GE2D_ST_0,	/* reserver.             */

	/* vpp wrapper. */
	VTRACE_VPP_PIC_0,	/* vf_put.               */
	VTRACE_VPP_PIC_1,	/* recycle.              */
	VTRACE_VPP_PIC_2,	/* receive.              */
	VTRACE_VPP_PIC_3,	/* fill_output_start.    */
	VTRACE_VPP_PIC_4,	/* empty_input_start.    */
	VTRACE_VPP_PIC_5,	/* lc_handle_start.      */
	VTRACE_VPP_PIC_6,	/* direct_handle_start.  */
	VTRACE_VPP_PIC_7,	/* fill_output_start_dw. */
	VTRACE_VPP_PIC_8,	/* empty_input_start_dw. */
	VTRACE_VPP_PIC_9,	/* submit.               */
	VTRACE_VPP_PIC_10,	/* lc_submit.            */
	VTRACE_VPP_PIC_11,	/* vf_get.               */
	VTRACE_VPP_PIC_12,	/* lc_attach.            */
	VTRACE_VPP_PIC_13,	/* lc_detach.            */
	VTRACE_VPP_PIC_14,	/* lc_release.           */
	VTRACE_VPP_PIC_15,	/* vpp_cache_num.           */

	VTRACE_VPP_ST_0,	/* reserver.		 */

	/* dec core */
	VTRACE_DEC_PIC_0,	/* submit.		 */
	VTRACE_DEC_PIC_1,	/* reserver.		 */

	VTRACE_DEC_ST_0,	/* chunk_size.           */
	VTRACE_DEC_ST_1,	/* free_buffer_count.    */
	VTRACE_DEC_ST_2,	/* dec_state.		 */
	VTRACE_DEC_ST_3,	/* work_state.           */
	VTRACE_DEC_ST_4,	/* eos.		         */
	VTRACE_DEC_ST_5,	/* wait_more_buf.        */
	VTRACE_DEC_ST_6,	/* reserver.		 */

	VTRACE_MAX
};

struct vdec_trace_map {
	u32	group;
	int	type;
	u8	*gname;
	u8	*description;
};

struct vdec_trace_item {
	int	channel;
	bool	enable;
	u32	group;
	int	type;
	ulong	value;
	u8	name[64];
};

struct vdec_trace {
	struct vdec_trace_item item[VTRACE_MAX + 1];
};

void vdec_trace_init(struct vdec_trace *vtr, int ch, int vdec_id);

void vdec_trace_clean(struct vdec_trace *vtr);

static inline void vdec_tracing(struct vdec_trace *vtr, int type, ulong val)
{
	if (vtr->item[type].enable) {
		meson_atrace(KERNEL_ATRACE_TAG_V4L2,
			vtr->item[type].name,
			(1 << KERNEL_ATRACE_COUNTER), val);
		vtr->item[type].value = val;
	}
}

static inline void vdec_tracing_begin(struct vdec_trace *vtr, int type)
{
	if (vtr->item[type].enable) {
		meson_atrace(KERNEL_ATRACE_TAG_V4L2,
			vtr->item[type].name,
			(1 << KERNEL_ATRACE_BEGIN), 0);
	}
}

static inline void vdec_tracing_end(struct vdec_trace *vtr, int type)
{
	if (vtr->item[type].enable) {
		meson_atrace(KERNEL_ATRACE_TAG_V4L2,
			vtr->item[type].name,
			(1 << KERNEL_ATRACE_END), 1);
	}
}

#endif //_AML_DEC_TRACE_H_

