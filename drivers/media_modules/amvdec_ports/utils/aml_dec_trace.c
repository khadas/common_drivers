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

#include "aml_dec_trace.h"

extern u32 trace_config;

static const struct vdec_trace_map trace_map[] = {
	/* trace mapping for v4ldec context. */
	{ VTRACE_GROUP_V4L_DEC,  VTRACE_V4L_ES_0,   "V4L", "ES-que" },
	{ VTRACE_GROUP_V4L_DEC,  VTRACE_V4L_ES_1,   "V4L", "ES-que_again" },
	{ VTRACE_GROUP_V4L_DEC,  VTRACE_V4L_ES_2,   "V4L", "ES-write" },
	{ VTRACE_GROUP_V4L_DEC,  VTRACE_V4L_ES_3,   "V4L", "ES-write_secure" },
	{ VTRACE_GROUP_V4L_DEC,  VTRACE_V4L_ES_4,   "V4L", "ES-receive" },
	{ VTRACE_GROUP_V4L_DEC,  VTRACE_V4L_ES_5,   "V4L", "ES-submit" },
	{ VTRACE_GROUP_V4L_DEC,  VTRACE_V4L_ES_6,   "V4L", "ES-deque" },
	{ VTRACE_GROUP_V4L_DEC,  VTRACE_V4L_ES_7,   "V4L", "ES-deque_again" },
	{ VTRACE_GROUP_V4L_DEC,  VTRACE_V4L_ES_8,   "V4L", "ES-write_end" },
	{ VTRACE_GROUP_V4L_DEC,  VTRACE_V4L_ES_9,   "V4L", "ES-write_secure_end" },
	{ VTRACE_GROUP_V4L_DEC,  VTRACE_V4L_ES_10,  "V4L", "ES-write_error" },
	{ VTRACE_GROUP_V4L_DEC,  VTRACE_V4L_ES_11,  "V4L", "ES-write_again" },
	{ VTRACE_GROUP_V4L_DEC,  VTRACE_V4L_ES_12,  "V4L", "ES-in_pts" },

	{ VTRACE_GROUP_V4L_DEC,  VTRACE_V4L_PIC_0,  "V4L", "PIC-que" },
	{ VTRACE_GROUP_V4L_DEC,  VTRACE_V4L_PIC_1,  "V4L", "PIC-que_again" },
	{ VTRACE_GROUP_V4L_DEC,  VTRACE_V4L_PIC_2,  "V4L", "PIC-storage" },
	{ VTRACE_GROUP_V4L_DEC,  VTRACE_V4L_PIC_3,  "V4L", "PIC-require" },
	{ VTRACE_GROUP_V4L_DEC,  VTRACE_V4L_PIC_4,  "V4L", "PIC-recycle" },
	{ VTRACE_GROUP_V4L_DEC,  VTRACE_V4L_PIC_5,  "V4L", "PIC-receive" },
	{ VTRACE_GROUP_V4L_DEC,  VTRACE_V4L_PIC_6,  "V4L", "PIC-submit" },
	{ VTRACE_GROUP_V4L_DEC,  VTRACE_V4L_PIC_7,  "V4L", "PIC-deque" },
	{ VTRACE_GROUP_V4L_DEC,  VTRACE_V4L_PIC_8,  "V4L", "PIC-deque_again" },
	{ VTRACE_GROUP_V4L_DEC,  VTRACE_V4L_PIC_9,  "V4L", "PIC-out_pts" },
	{ VTRACE_GROUP_V4L_DEC,  VTRACE_V4L_PIC_10,  "V4L", "PIC-cache" },

	{ VTRACE_GROUP_V4L_DEC,  VTRACE_V4L_ST_0,   "V4L", "ST-state" },
	{ VTRACE_GROUP_V4L_DEC,  VTRACE_V4L_ST_1,   "V4L", "ST-input_buffering" },

	/* trace mapping for ge2d wrapper. */
	{ VTRACE_GROUP_V4L_GE2D, VTRACE_GE2D_PIC_0, "GE2D", "PIC-vf_put" },
	{ VTRACE_GROUP_V4L_GE2D, VTRACE_GE2D_PIC_1, "GE2D", "PIC-recycle" },
	{ VTRACE_GROUP_V4L_GE2D, VTRACE_GE2D_PIC_2, "GE2D", "PIC-receive" },
	{ VTRACE_GROUP_V4L_GE2D, VTRACE_GE2D_PIC_3, "GE2D", "PIC-handle_start" },
	{ VTRACE_GROUP_V4L_GE2D, VTRACE_GE2D_PIC_4, "GE2D", "PIC-submit" },
	{ VTRACE_GROUP_V4L_GE2D, VTRACE_GE2D_PIC_5, "GE2D", "PIC-vf_get" },
	{ VTRACE_GROUP_V4L_GE2D, VTRACE_GE2D_PIC_6, "GE2D", "PIC-cache" },

	{ VTRACE_GROUP_V4L_GE2D, VTRACE_GE2D_ST_0,  "GE2D", "reserver" },

	/* trace mapping for vpp wrapper. */
	{ VTRACE_GROUP_V4L_VPP,  VTRACE_VPP_PIC_0,  "VPP", "PIC-vf_put" },
	{ VTRACE_GROUP_V4L_VPP,  VTRACE_VPP_PIC_1,  "VPP", "PIC-recycle" },
	{ VTRACE_GROUP_V4L_VPP,  VTRACE_VPP_PIC_2,  "VPP", "PIC-receive" },
	{ VTRACE_GROUP_V4L_VPP,  VTRACE_VPP_PIC_3,  "VPP", "PIC-fill_output_start" },
	{ VTRACE_GROUP_V4L_VPP,  VTRACE_VPP_PIC_4,  "VPP", "PIC-empty_input_start" },
	{ VTRACE_GROUP_V4L_VPP,  VTRACE_VPP_PIC_5,  "VPP", "PIC-lc_handle_start" },
	{ VTRACE_GROUP_V4L_VPP,  VTRACE_VPP_PIC_6,  "VPP", "PIC-direct_handle_start" },
	{ VTRACE_GROUP_V4L_VPP,  VTRACE_VPP_PIC_7,  "VPP", "PIC-fill_output_start_dw" },
	{ VTRACE_GROUP_V4L_VPP,  VTRACE_VPP_PIC_8,  "VPP", "PIC-empty_input_start_dw" },
	{ VTRACE_GROUP_V4L_VPP,  VTRACE_VPP_PIC_9,  "VPP", "PIC-submit" },
	{ VTRACE_GROUP_V4L_VPP,  VTRACE_VPP_PIC_10, "VPP", "PIC-lc_submit" },
	{ VTRACE_GROUP_V4L_VPP,  VTRACE_VPP_PIC_11, "VPP", "PIC-vf_get" },
	{ VTRACE_GROUP_V4L_VPP,  VTRACE_VPP_PIC_12, "VPP", "PIC-lc_attach" },
	{ VTRACE_GROUP_V4L_VPP,  VTRACE_VPP_PIC_13, "VPP", "PIC-lc_detach" },
	{ VTRACE_GROUP_V4L_VPP,  VTRACE_VPP_PIC_14, "VPP", "PIC-lc_release" },
	{ VTRACE_GROUP_V4L_VPP,  VTRACE_VPP_PIC_15, "VPP", "PIC-cache" },

	{ VTRACE_GROUP_V4L_VPP,  VTRACE_VPP_ST_0,   "VPP", "reserver" },

	/* trace mapping for decode core. */
	{ VTRACE_GROUP_DEC_CORE,  VTRACE_DEC_PIC_0, "DEC", "PIC-submit" },
	{ VTRACE_GROUP_DEC_CORE,  VTRACE_DEC_PIC_1, "DEC", "PIC-reserver" },

	{ VTRACE_GROUP_DEC_CORE,  VTRACE_DEC_ST_0,  "DEC", "ST-chunk_size" },
	{ VTRACE_GROUP_DEC_CORE,  VTRACE_DEC_ST_1,  "DEC", "ST-free_buffer_count" },
	{ VTRACE_GROUP_DEC_CORE,  VTRACE_DEC_ST_2,  "DEC", "ST-dec_state" },
	{ VTRACE_GROUP_DEC_CORE,  VTRACE_DEC_ST_3,  "DEC", "ST-work_state" },
	{ VTRACE_GROUP_DEC_CORE,  VTRACE_DEC_ST_4,  "DEC", "ST-eos" },
	{ VTRACE_GROUP_DEC_CORE,  VTRACE_DEC_ST_5,  "DEC", "ST-wait_more_buf" },

	{ VTRACE_GROUP_DEC_CORE,  VTRACE_DEC_ST_6,  "DEC", "ST-reserver" },

	{ 0, VTRACE_MAX, "unknown", "unknown" },
};

void vdec_trace_init(struct vdec_trace *vtr, int ch, int vdec_id)
{
	int i, size = ARRAY_SIZE(trace_map);

	for (i = 0; i < size; i++) {
		if (!(trace_config & trace_map[i].group)) {
			vtr->item[i].enable = false;
			continue;
		}

		vtr->item[i].channel	= ch;
		vtr->item[i].enable	= true;
		vtr->item[i].group	= trace_map[i].group;
		vtr->item[i].type	= trace_map[i].type;

		snprintf(vtr->item[i].name, 64,
			"[%d,%d] %s_%s",
			ch, vdec_id,
			trace_map[i].gname,
			trace_map[i].description);
	}
}

void vdec_trace_clean(struct vdec_trace *vtr)
{
	int i, size = ARRAY_SIZE(trace_map);

	for (i = 0; i < size; i++) {
		if (vtr->item[i].value)
			vdec_tracing(vtr, vtr->item[i].type, 0);
	}
}

