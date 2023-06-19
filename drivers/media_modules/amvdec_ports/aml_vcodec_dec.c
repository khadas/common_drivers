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
#include <media/v4l2-event.h>
#include <media/v4l2-mem2mem.h>
#include <media/videobuf2-dma-contig.h>
#include <media/videobuf2-dma-sg.h>

#include <linux/delay.h>
#include <linux/atomic.h>
#include <linux/crc32.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/amlogic/meson_uvm_core.h>
#include <linux/scatterlist.h>
#include <linux/sched/clock.h>
#include <linux/highmem.h>
#include <linux/version.h>
#include <uapi/linux/sched/types.h>
#include <linux/amlogic/media/canvas/canvas_mgr.h>
#include <linux/amlogic/media/codec_mm/dmabuf_manage.h>
#include <linux/dma-heap.h>
#include <uapi/linux/dma-heap.h>
#include <linux/amlogic/media/meson_uvm_allocator.h>

#include "aml_vcodec_drv.h"
#include "aml_vcodec_dec.h"
#include "aml_vcodec_util.h"
#include "vdec_drv_if.h"
#include "aml_vcodec_adapt.h"
#include "aml_vcodec_vpp.h"
#include "aml_vcodec_ge2d.h"

#include "../frame_provider/decoder/utils/decoder_bmmu_box.h"
#include "../frame_provider/decoder/utils/decoder_mmu_box.h"
#include "../common/chips/decoder_cpu_ver_info.h"
#include "utils/common.h"
#include "../media_sync/pts_server/pts_server_core.h"
#include "../frame_provider/decoder/utils/vdec_sync.h"
#include "../frame_provider/decoder/utils/aml_buf_helper.h"
#include "../common/media_utils/media_utils.h"
#include "../frame_provider/decoder/utils/vdec_v4l2_buffer_ops.h"
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
#include <linux/amlogic/media/amdolbyvision/dolby_vision.h>
#endif
#include <linux/amlogic/media/codec_mm/codec_mm.h>
#include <linux/amlogic/media/codec_mm/codec_mm_scatter.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
#include <linux/amlogic/media/dmabuf_heaps/amlogic_dmabuf_heap.h>
#include <linux/amlogic/ion.h>
#else
#include <linux/meson_ion.h>
#endif


#define OUT_FMT_IDX		(0) //default h264
#define CAP_FMT_IDX		(14) //capture nv21m
#define CAP_FMT_I420_IDX	(18) //use for mjpeg

#define AML_VDEC_MIN_W	64U
#define AML_VDEC_MIN_H	64U
#define DFT_CFG_WIDTH	AML_VDEC_MIN_W
#define DFT_CFG_HEIGHT	AML_VDEC_MIN_H

#define V4L2_CID_USER_AMLOGIC_BASE (V4L2_CID_USER_BASE + 0x1100)
#define AML_V4L2_SET_DRMMODE (V4L2_CID_USER_AMLOGIC_BASE + 0)
#define AML_V4L2_GET_INPUT_BUFFER_NUM (V4L2_CID_USER_AMLOGIC_BASE + 1)
#define AML_V4L2_SET_DURATION (V4L2_CID_USER_AMLOGIC_BASE + 2)
#define AML_V4L2_GET_FILMGRAIN_INFO (V4L2_CID_USER_AMLOGIC_BASE + 3)
#define AML_V4L2_SET_INPUT_BUFFER_NUM_CACHE (V4L2_CID_USER_AMLOGIC_BASE + 4)
#define AML_V4L2_GET_DECODER_INFO (V4L2_CID_USER_AMLOGIC_BASE + 5)

/*V4L2_CID_USER_AMLOGIC_BASE + 5 occupied*/
#define AML_V4L2_GET_BITDEPTH (V4L2_CID_USER_AMLOGIC_BASE + 6)
#define AML_V4L2_DEC_PARMS_CONFIG (V4L2_CID_USER_AMLOGIC_BASE + 7)
#define AML_V4L2_GET_INST_ID (V4L2_CID_USER_AMLOGIC_BASE + 8)
#define AML_V4L2_SET_STREAM_MODE (V4L2_CID_USER_AMLOGIC_BASE + 9)
#define AML_V4L2_SET_ES_DMABUF_TYPE (V4L2_CID_USER_AMLOGIC_BASE + 10)

#define V4L2_EVENT_PRIVATE_EXT_VSC_BASE (V4L2_EVENT_PRIVATE_START + 0x2000)
#define V4L2_EVENT_PRIVATE_EXT_VSC_EVENT (V4L2_EVENT_PRIVATE_EXT_VSC_BASE + 1)
#define V4L2_EVENT_PRIVATE_EXT_SEND_ERROR (V4L2_EVENT_PRIVATE_EXT_VSC_BASE + 2)
#define V4L2_EVENT_PRIVATE_EXT_REPORT_ERROR_FRAME (V4L2_EVENT_PRIVATE_EXT_VSC_BASE + 3)

#define WORK_ITEMS_MAX (32)
#define MAX_DI_INSTANCE (2)

#define PAGE_NUM_ONE_MB	(256)
//#define USEC_PER_SEC 1000000

#define INVALID_IDX -1

#define call_void_memop(vb, op, args...)				\
	do {								\
		if ((vb)->vb2_queue->mem_ops->op)			\
			(vb)->vb2_queue->mem_ops->op(args);		\
	} while (0)

static struct aml_video_fmt aml_video_formats[] = {
	{
		.name = "H.264",
		.fourcc = V4L2_PIX_FMT_H264,
		.type = AML_FMT_DEC,
		.num_planes = 1,
	},
	{
		.name = "H.265",
		.fourcc = V4L2_PIX_FMT_HEVC,
		.type = AML_FMT_DEC,
		.num_planes = 1,
	},
	{
		.name = "VP9",
		.fourcc = V4L2_PIX_FMT_VP9,
		.type = AML_FMT_DEC,
		.num_planes = 1,
	},
	{
		.name = "MPEG1",
		.fourcc = V4L2_PIX_FMT_MPEG1,
		.type = AML_FMT_DEC,
		.num_planes = 1,
	},
	{
		.name = "MPEG2",
		.fourcc = V4L2_PIX_FMT_MPEG2,
		.type = AML_FMT_DEC,
		.num_planes = 1,
	},
	{
		.name = "MPEG4",
		.fourcc = V4L2_PIX_FMT_MPEG4,
		.type = AML_FMT_DEC,
		.num_planes = 1,
	},
	{
		.name = "MJPEG",
		.fourcc = V4L2_PIX_FMT_MJPEG,
		.type = AML_FMT_DEC,
		.num_planes = 1,
	},
	{
		.name = "AVS",
		.fourcc = V4L2_PIX_FMT_AVS,
		.type = AML_FMT_DEC,
		.num_planes = 1,
	},
	{
		.name = "AV1",
		.fourcc = V4L2_PIX_FMT_AV1,
		.type = AML_FMT_DEC,
		.num_planes = 1,
	},
	{
		.name = "AVS2",
		.fourcc = V4L2_PIX_FMT_AVS2,
		.type = AML_FMT_DEC,
		.num_planes = 1,
	},
	{
		.name = "AVS3",
		.fourcc = V4L2_PIX_FMT_AVS3,
		.type = AML_FMT_DEC,
		.num_planes = 1,
	},
	{
		.name = "VC1L",
		.fourcc = V4L2_PIX_FMT_VC1_ANNEX_L,
		.type = AML_FMT_DEC,
		.num_planes = 1,
	},
	{
		.name = "VC1G",
		.fourcc = V4L2_PIX_FMT_VC1_ANNEX_G,
		.type = AML_FMT_DEC,
		.num_planes = 1,
	},
	{
		.name = "NV21",
		.fourcc = V4L2_PIX_FMT_NV21,
		.type = AML_FMT_FRAME,
		.num_planes = 1,
	},
	{
		.name = "NV21M",
		.fourcc = V4L2_PIX_FMT_NV21M,
		.type = AML_FMT_FRAME,
		.num_planes = 2,
	},
	{
		.name = "NV12",
		.fourcc = V4L2_PIX_FMT_NV12,
		.type = AML_FMT_FRAME,
		.num_planes = 1,
	},
	{
		.name = "NV12M",
		.fourcc = V4L2_PIX_FMT_NV12M,
		.type = AML_FMT_FRAME,
		.num_planes = 2,
	},
	{
		.name = "YUV420",
		.fourcc = V4L2_PIX_FMT_YUV420,
		.type = AML_FMT_FRAME,
		.num_planes = 1,
	},
	{
		.name = "YUV420M",
		.fourcc = V4L2_PIX_FMT_YUV420M,
		.type = AML_FMT_FRAME,
		.num_planes = 2,
	},
};

static const struct aml_codec_framesizes aml_vdec_framesizes[] = {
	{
		.fourcc	= V4L2_PIX_FMT_H264,
		.stepwise = {  AML_VDEC_MIN_W, AML_VDEC_MAX_W, 2,
				AML_VDEC_MIN_H, AML_VDEC_MAX_H, 2},
	},
	{
		.fourcc	= V4L2_PIX_FMT_HEVC,
		.stepwise = {  AML_VDEC_MIN_W, AML_VDEC_MAX_W, 2,
				AML_VDEC_MIN_H, AML_VDEC_MAX_H, 2},
	},
	{
		.fourcc = V4L2_PIX_FMT_VP9,
		.stepwise = {  AML_VDEC_MIN_W, AML_VDEC_MAX_W, 2,
				AML_VDEC_MIN_H, AML_VDEC_MAX_H, 2},
	},
	{
		.fourcc = V4L2_PIX_FMT_MPEG1,
		.stepwise = {  AML_VDEC_MIN_W, AML_VDEC_MAX_W, 2,
				AML_VDEC_MIN_H, AML_VDEC_MAX_H, 2},
	},
	{
		.fourcc = V4L2_PIX_FMT_MPEG2,
		.stepwise = {  AML_VDEC_MIN_W, AML_VDEC_MAX_W, 2,
				AML_VDEC_MIN_H, AML_VDEC_MAX_H, 2},
	},
	{
		.fourcc = V4L2_PIX_FMT_MPEG4,
		.stepwise = {  AML_VDEC_MIN_W, AML_VDEC_MAX_W, 2,
				AML_VDEC_MIN_H, AML_VDEC_MAX_H, 2},
	},
	{
		.fourcc = V4L2_PIX_FMT_MJPEG,
		.stepwise = {  AML_VDEC_MIN_W, AML_VDEC_MAX_W, 2,
				AML_VDEC_MIN_H, AML_VDEC_MAX_H, 2},
	},
	{
		.fourcc = V4L2_PIX_FMT_AVS,
		.stepwise = {  AML_VDEC_MIN_W, AML_VDEC_MAX_W, 2,
				AML_VDEC_MIN_H, AML_VDEC_MAX_H, 2},
	},
	{
		.fourcc = V4L2_PIX_FMT_AV1,
		.stepwise = {  AML_VDEC_MIN_W, AML_VDEC_MAX_W, 2,
				AML_VDEC_MIN_H, AML_VDEC_MAX_H, 2},
	},
	{
		.fourcc = V4L2_PIX_FMT_AVS2,
		.stepwise = {  AML_VDEC_MIN_W, AML_VDEC_MAX_W, 2,
				AML_VDEC_MIN_H, AML_VDEC_MAX_H, 2},
	},
	{
		.fourcc = V4L2_PIX_FMT_AVS3,
		.stepwise = {  AML_VDEC_MIN_W, AML_VDEC_MAX_W, 2,
				AML_VDEC_MIN_H, AML_VDEC_MAX_H, 2},
	},
	{
		.fourcc = V4L2_PIX_FMT_VC1_ANNEX_L,
		.stepwise = {  AML_VDEC_MIN_W, AML_VDEC_MAX_W, 2,
				AML_VDEC_MIN_H, AML_VDEC_MAX_H, 2},
	},
	{
		.fourcc = V4L2_PIX_FMT_VC1_ANNEX_G,
		.stepwise = {  AML_VDEC_MIN_W, AML_VDEC_MAX_W, 2,
				AML_VDEC_MIN_H, AML_VDEC_MAX_H, 2},
	},
	{
		.fourcc = V4L2_PIX_FMT_NV21,
		.stepwise = {  AML_VDEC_MIN_W, AML_VDEC_MAX_W, 2,
				AML_VDEC_MIN_H, AML_VDEC_MAX_H, 2},
	},
	{
		.fourcc = V4L2_PIX_FMT_NV21M,
		.stepwise = {  AML_VDEC_MIN_W, AML_VDEC_MAX_W, 2,
				AML_VDEC_MIN_H, AML_VDEC_MAX_H, 2},
	},
	{
		.fourcc = V4L2_PIX_FMT_NV12,
		.stepwise = {  AML_VDEC_MIN_W, AML_VDEC_MAX_W, 2,
				AML_VDEC_MIN_H, AML_VDEC_MAX_H, 2},
	},
	{
		.fourcc = V4L2_PIX_FMT_NV12M,
		.stepwise = {  AML_VDEC_MIN_W, AML_VDEC_MAX_W, 2,
				AML_VDEC_MIN_H, AML_VDEC_MAX_H, 2},
	},
	{
		.fourcc = V4L2_PIX_FMT_YUV420,
		.stepwise = {  AML_VDEC_MIN_W, AML_VDEC_MAX_W, 2,
				AML_VDEC_MIN_H, AML_VDEC_MAX_H, 2},
	},
	{
		.fourcc = V4L2_PIX_FMT_YUV420M,
		.stepwise = {  AML_VDEC_MIN_W, AML_VDEC_MAX_W, 2,
				AML_VDEC_MIN_H, AML_VDEC_MAX_H, 2},
	},
};

#define NUM_SUPPORTED_FRAMESIZE ARRAY_SIZE(aml_vdec_framesizes)
#define NUM_FORMATS ARRAY_SIZE(aml_video_formats)

extern bool multiplanar;
extern int dump_capture_frame;
extern char dump_path[32];
extern int bypass_vpp;
extern int bypass_ge2d;
extern bool support_format_I420;
extern bool support_mjpeg;
extern int bypass_progressive;
extern int force_enable_nr;
extern int force_enable_di_local_buffer;
extern int max_di_instance;
extern int bypass_nr_flag;
extern int es_node_expand;
extern int force_di_permission;
extern int enable_di_post;

extern int vdec_get_size_ratio(int dw_mode);
static void update_ctx_dimension(struct aml_vcodec_ctx *ctx, u32 type);
static void copy_v4l2_format_dimension(struct aml_vcodec_ctx *ctx,
				       struct v4l2_pix_format_mplane *pix_mp,
				       struct v4l2_pix_format *pix,
				       struct aml_q_data *q_data,
				       u32 type);
static void vidioc_vdec_s_parm_ext(struct v4l2_ctrl *, struct aml_vcodec_ctx *);
static void vidioc_vdec_g_parm_ext(struct v4l2_ctrl *, struct aml_vcodec_ctx *);
static int is_vdec_core_fmt(struct aml_vcodec_ctx *ctx);


static ulong aml_vcodec_ctx_lock(struct aml_vcodec_ctx *ctx)
{
	ulong flags;

	spin_lock_irqsave(&ctx->slock, flags);

	return flags;
}

static void aml_vcodec_ctx_unlock(struct aml_vcodec_ctx *ctx, ulong flags)
{
	spin_unlock_irqrestore(&ctx->slock, flags);
}

static ulong dmabuf_contiguous_size(struct sg_table *sgt)
{
	struct scatterlist *s;
	dma_addr_t expected = sg_dma_address(sgt->sgl);
	ulong size = 0;
	u32 i;

	for_each_sg(sgt->sgl, s, sgt->nents, i) {
		if (sg_dma_address(s) != expected)
			break;
		expected = sg_dma_address(s) + sg_dma_len(s);
		size += sg_dma_len(s);
	}

	return size;
}

static struct aml_video_fmt *aml_vdec_find_format(struct v4l2_format *f)
{
	struct aml_video_fmt *fmt;
	unsigned int k;

	for (k = 0; k < NUM_FORMATS; k++) {
		fmt = &aml_video_formats[k];
		if (fmt->fourcc == f->fmt.pix_mp.pixelformat)
			return fmt;
	}

	return NULL;
}

static struct aml_q_data *aml_vdec_get_q_data(struct aml_vcodec_ctx *ctx,
					      enum v4l2_buf_type type)
{
	if (V4L2_TYPE_IS_OUTPUT(type))
		return &ctx->q_data[AML_Q_DATA_SRC];

	return &ctx->q_data[AML_Q_DATA_DST];
}

void aml_vdec_dispatch_event(struct aml_vcodec_ctx *ctx, u32 changes)
{
	struct v4l2_event event = {0};

	switch (changes) {
	case V4L2_EVENT_SRC_CH_RESOLUTION:
	case V4L2_EVENT_SRC_CH_HDRINFO:
	case V4L2_EVENT_REQUEST_RESET:
	case V4L2_EVENT_REQUEST_EXIT:
		event.type = V4L2_EVENT_SOURCE_CHANGE;
		event.u.src_change.changes = changes;
		break;
	case V4L2_EVENT_SEND_EOS:
		event.type = V4L2_EVENT_EOS;
		break;
	case V4L2_EVENT_SEND_ERROR:
		event.type = V4L2_EVENT_PRIVATE_EXT_SEND_ERROR;
		break;
	case V4L2_EVENT_REPORT_ERROR_FRAME:
		event.type = V4L2_EVENT_PRIVATE_EXT_REPORT_ERROR_FRAME;
		memcpy(event.u.data, &ctx->current_timestamp, sizeof(u64));
		v4l_dbg(ctx, V4L_DEBUG_CODEC_EXINFO, "report error frame timestamp: %llu\n",
			ctx->current_timestamp);
		break;
	default:
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"unsupport dispatch event %x\n", changes);
		return;
	}

	v4l2_event_queue_fh(&ctx->fh, &event);
	if (changes != V4L2_EVENT_SRC_CH_HDRINFO)
		v4l_dbg(ctx, V4L_DEBUG_CODEC_PRINFO, "changes: %x\n", changes);
	else
		v4l_dbg(ctx, V4L_DEBUG_CODEC_EXINFO, "changes: %x\n", changes);
}

static void aml_vdec_flush_decoder(struct aml_vcodec_ctx *ctx)
{
	v4l_dbg(ctx, V4L_DEBUG_CODEC_EXINFO, "%s\n", __func__);

	aml_decoder_flush(ctx->ada_ctx);
}

static bool is_di_support(int width, int height)
{
	int di_max_width = 1920;
	int di_max_height = 1088;
	int di_min_width = 128;
	int di_min_height = 16;

	if (width > di_max_width || height > di_max_height) {
		return false;
	} else if (width < di_min_width || height < di_min_height) {
		return false;
	}

	return true;
}

/* Conditions:
 * Always connect VPP for mpeg2 and h264 when the stream size is under 2K.
 * Always connect VPP for hevc/av1/vp9 when color space is not SDR and
 *     stream size is under 2K.
 * For DV, need application to notify V4L2 driver to enforce the color space
 *     conversion. Plan to do it through a system node.
 * Do not connect VPP in other cases.
 */
static bool vpp_needed(struct aml_vcodec_ctx *ctx, u32* mode)
{
	int width = ctx->picinfo.coded_width;
	int height = ctx->picinfo.coded_height;

	if (bypass_vpp || ctx->enable_di_post)
		return false;

	if (ctx->vpp_cfg.bypass)
		return false;

	if (!ctx->vpp_cfg.enable_nr &&
		(ctx->picinfo.field == V4L2_FIELD_NONE) &&
		!((ctx->config.parm.dec.cfg.double_write_mode & VDEC_MODE_MMU_DW_MASK) &&
		(get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_S4 ||
		get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_S4D))) {
		return false;
	}

	if (!ctx->vpp_cfg.enable_nr &&
		(ctx->output_pix_fmt == V4L2_PIX_FMT_HEVC)) {
		if (!is_di_support(width, height)) {
			return false;
		}
	}

	if ((ctx->output_pix_fmt == V4L2_PIX_FMT_H264) &&
		(ctx->picinfo.field != V4L2_FIELD_NONE)) {
		if (!is_di_support(width, height)) {
			return false;
		}
	}

	if (ctx->vpp_cfg.enable_nr) {
		if (ctx->vpp_cfg.enable_local_buf)
			*mode = VPP_MODE_NOISE_REDUC_LOCAL;
		else
			*mode = VPP_MODE_NOISE_REDUC;
	} else {
		if (ctx->vpp_cfg.enable_local_buf)
			*mode = VPP_MODE_DI_LOCAL;
		else
			*mode = VPP_MODE_DI;
	}

	if (!disable_vpp_dw_mmu &&
		(ctx->config.parm.dec.cfg.double_write_mode & VDEC_MODE_MMU_DW_MASK) &&
		(get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_S4 ||
		get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_S4D)) {
		*mode = VPP_MODE_S4_DW_MMU;
	}
#if 0//enable later
	if (ctx->colorspace != V4L2_COLORSPACE_DEFAULT &&
		!is_over_size(width, height, size)) {
		if (ctx->vpp_cfg.enable_local_buf)
			*mode = VPP_MODE_COLOR_CONV_LOCAL;
		else
			*mode = VPP_MODE_COLOR_CONV;
	}
#endif

	return true;
}

static bool ge2d_needed(struct aml_vcodec_ctx *ctx, u32* mode)
{
	bool enable_fence = (ctx->config.parm.dec.cfg.low_latency_mode & 2) ? 1 : 0;

	if ((get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_T7) && enable_fence) {
		return false;
	}

	if (bypass_ge2d)
		return false;

	if (ctx->ge2d_cfg.bypass)
		return false;

	if (is_cpu_t7()) {
		if ((ctx->output_pix_fmt != V4L2_PIX_FMT_H264) &&
			(ctx->output_pix_fmt != V4L2_PIX_FMT_MPEG1) &&
			(ctx->output_pix_fmt != V4L2_PIX_FMT_MPEG2) &&
			(ctx->output_pix_fmt != V4L2_PIX_FMT_MPEG4) &&
			(ctx->output_pix_fmt != V4L2_PIX_FMT_MJPEG) &&
			(ctx->output_pix_fmt != V4L2_PIX_FMT_AVS)) {
			return false;
		}
	} else if (ctx->output_pix_fmt != V4L2_PIX_FMT_MJPEG) {
			return false;
	}

	if (ctx->picinfo.field != V4L2_FIELD_NONE) {
		return false;
	}

	*mode = GE2D_MODE_CONVERT_LE;

	return true;
}

static u32 v4l_buf_size_decision(struct aml_vcodec_ctx *ctx)
{
	u32 mode, total_size;
	struct vdec_pic_info *picinfo = &ctx->picinfo;
	struct aml_vpp_cfg_infos *vpp = &ctx->vpp_cfg;
	struct aml_ge2d_cfg_infos *ge2d = &ctx->ge2d_cfg;

	if (vpp_needed(ctx, &mode)) {
		vpp->mode        = mode;
		vpp->fmt         = ctx->cap_pix_fmt;
		vpp->is_drm      = ctx->is_drm_mode;
		vpp->buf_size = aml_v4l2_vpp_get_buf_num(vpp->mode)
			+ picinfo->vpp_margin;

		if (picinfo->field == V4L2_FIELD_NONE) {
			vpp->is_prog = true;
			vpp->buf_size = 0;
		} else {
			vpp->is_prog = false;
			/* for between with dec & vpp. */
			picinfo->dpb_margin = 2;
		}

		if (vpp->is_prog &&
			!vpp->enable_local_buf &&
			bypass_progressive) {
			vpp->is_bypass_p = true;
		}
		ctx->vpp_is_need = true;
	} else {
		vpp->buf_size = 0;
		ctx->vpp_is_need = false;
	}

	if (ge2d_needed(ctx, &mode)) {
		ge2d->mode = mode;
		ge2d->buf_size = 4 + picinfo->dpb_margin;
		ctx->ge2d_is_need = true;
		picinfo->dpb_margin = 2;
	} else {
		ge2d->buf_size = 0;
		ctx->ge2d_is_need = false;
	}

	ctx->dpb_size = picinfo->dpb_frames + picinfo->dpb_margin;
	ctx->vpp_size = vpp->buf_size;
	ctx->ge2d_size = ge2d->buf_size;

	if (ctx->enable_di_post &&
		ctx->picinfo.field != V4L2_FIELD_NONE &&
		is_vdec_core_fmt(ctx))
		ctx->dpb_size *= PAIR_DONE;

	if (ctx->enable_di_post && (is_vdec_core_fmt(ctx)) &&
		ctx->dpb_size > 2 * V4L_CAP_BUFF_MAX) {
		ctx->dpb_size = ctx->dpb_size / PAIR_DONE * 2;
	}

	total_size = ctx->dpb_size + ctx->vpp_size + ctx->ge2d_size;

	v4l_dbg(ctx, V4L_DEBUG_CODEC_EXINFO,
		"dpb_size: %d dpb_frames: %d dpb_margin: %d vpp_size: %d ge2d_size: %d\n",
			ctx->dpb_size, picinfo->dpb_frames, picinfo->dpb_margin,
			ctx->vpp_size, ctx->ge2d_size);

	if ((total_size > V4L_CAP_BUFF_MAX) &&
		!(ctx->enable_di_post && ctx->picinfo.field != V4L2_FIELD_NONE)) {
		if (ctx->ge2d_size) {
			ctx->dpb_size = V4L_CAP_BUFF_MAX - ctx->ge2d_size - ctx->vpp_size;
		} else if (ctx->vpp_size) {
			ctx->dpb_size = V4L_CAP_BUFF_MAX - ctx->vpp_size;
		} else {
			ctx->dpb_size = V4L_CAP_BUFF_MAX;
		}
		picinfo->dpb_margin = ctx->dpb_size - picinfo->dpb_frames;
		total_size = V4L_CAP_BUFF_MAX;
	}
	vdec_if_set_param(ctx, SET_PARAM_PIC_INFO, picinfo);

	return total_size;
}

static void aml_buf_configure_update(struct aml_vcodec_ctx *ctx)
{
	struct aml_buf_config config = {0};
	struct vb2_queue * que = v4l2_m2m_get_dst_vq(ctx->m2m_ctx);
	u32 dw = DM_YUV_ONLY;
	u32 tw = DM_INVALID;

	if (vdec_if_get_param(ctx, GET_PARAM_DW_MODE, &dw)) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR, "invalid dw_mode\n");
		return;

	}
	if (vdec_if_get_param(ctx, GET_PARAM_TW_MODE, &tw)) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR, "invalid tw_mode\n");
		return;

	}

	config.enable_extbuf	= true;
	config.enable_fbc	= ((dw != DM_YUV_ONLY) || tw) ? true : false;
	config.enable_secure	= ctx->is_drm_mode;
	config.memory_mode	= que->memory;
	config.planes		= V4L2_TYPE_IS_MULTIPLANAR(que->type) ? 2 : 1;
	config.luma_length	= ctx->picinfo.y_len_sz;
	config.chroma_length	= ctx->picinfo.c_len_sz;
	config.luma_length_tw	= ctx->picinfo.y_len_sz_tw;
	config.chroma_length_tw	= ctx->picinfo.c_len_sz_tw;
	config.dw_mode		= dw;
	config.tw_mode		= tw;

	aml_buf_configure(&ctx->bm, &config);
}

void aml_vdec_pic_info_update(struct aml_vcodec_ctx *ctx)
{
	struct aml_buf_config config;
	struct vb2_queue * que = v4l2_m2m_get_dst_vq(ctx->m2m_ctx);
	u32 dw = DM_YUV_ONLY;
	u32 tw = DM_INVALID;

	if (vdec_if_get_param(ctx, GET_PARAM_PIC_INFO, &ctx->picinfo)) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"Cannot get param : GET_PARAM_PICTURE_INFO ERR\n");
		return;
	}

	if (vdec_if_get_param(ctx, GET_PARAM_DW_MODE, &dw)) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR, "invalid dw_mode\n");
		return;

	}

	if (vdec_if_get_param(ctx, GET_PARAM_TW_MODE, &tw)) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR, "invalid tw_mode\n");
		return;

	}

	if (ctx->picinfo.visible_width == 0 ||
		ctx->picinfo.visible_height == 0 ||
		ctx->picinfo.coded_width == 0 ||
		ctx->picinfo.coded_height == 0) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"Cannot get correct pic info\n");
		return;
	}

	/*if ((ctx->last_decoded_picinfo.visible_width == ctx->picinfo.visible_width) ||
	    (ctx->last_decoded_picinfo.visible_height == ctx->picinfo.visible_height))
		return;*/

	v4l_dbg(ctx, V4L_DEBUG_CODEC_EXINFO,
		"new(%d,%d), old(%d,%d), real(%d,%d)\n",
			ctx->picinfo.visible_width,
			ctx->picinfo.visible_height,
			ctx->last_decoded_picinfo.visible_width,
			ctx->last_decoded_picinfo.visible_height,
			ctx->picinfo.coded_width,
			ctx->picinfo.coded_width);

	if (ctx->vpp_is_need)
		ctx->vpp_cfg.is_vpp_reset = true;

	v4l_buf_size_decision(ctx);

	config.enable_extbuf	= true;
	config.enable_fbc	= ((dw != DM_YUV_ONLY) || tw) ? true : false;
	config.enable_secure	= ctx->is_drm_mode;
	config.memory_mode	= que->memory;
	config.planes		= V4L2_TYPE_IS_MULTIPLANAR(que->type) ? 2 : 1;
	config.luma_length	= ctx->picinfo.y_len_sz;
	config.chroma_length	= ctx->picinfo.c_len_sz;
	config.luma_length_tw	= ctx->picinfo.y_len_sz_tw;
	config.chroma_length_tw	= ctx->picinfo.c_len_sz_tw;
	config.dw_mode			= dw;
	config.tw_mode			= tw;

	aml_buf_configure(&ctx->bm, &config);

	aml_buf_put_dma(&ctx->bm);

	v4l_dbg(ctx, V4L_DEBUG_CODEC_PRINFO,
		"Update picture buffer count: dec:%u, vpp:%u, ge2d:%u, margin:%u, total:%u\n",
		ctx->picinfo.dpb_frames, ctx->vpp_size, ctx->ge2d_size,
		ctx->picinfo.dpb_margin,
		CTX_BUF_TOTAL(ctx));
}

void vdec_frame_buffer_release(void *data)
{
	struct file_private_data *priv_data =
		(struct file_private_data *) data;
	struct aml_vcodec_ctx *ctx = (struct aml_vcodec_ctx *)
		priv_data->v4l_dec_ctx;
	struct aml_v4l2_buf *vb = (struct aml_v4l2_buf *)
		priv_data->vb_handle;
	struct uvm_hook_mod_info *uvm = NULL;

	if (ctx && ctx->uvm_proxy) {
		uvm = &ctx->uvm_proxy[vb->internal_index];
		uvm->free(uvm->arg);
	}

	memset(data, 0, sizeof(struct file_private_data));
	kfree(data);
}

void aml_clean_proxy_uvm(struct aml_vcodec_ctx *ctx)
{
	struct uvm_hook_mod_info *uvm = NULL;
	int i;

	for (i = 0; i < V4L_CAP_BUFF_MAX; i++) {
		if (ctx && ctx->uvm_proxy) {
			uvm = &ctx->uvm_proxy[i];
			if (uvm->free)
				uvm->free(uvm->arg);
		}
	}
}

static void v4l2_buff_done(struct vb2_v4l2_buffer *buf, enum vb2_buffer_state state)
{
	struct aml_vcodec_ctx *ctx = vb2_get_drv_priv(buf->vb2_buf.vb2_queue);

	mutex_lock(&ctx->buff_done_lock);
	if (buf->vb2_buf.state != VB2_BUF_STATE_ACTIVE) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_PROT, "vb is not active state = %d!\n",
			buf->vb2_buf.state);
		mutex_unlock(&ctx->buff_done_lock);
		return;
	}
	v4l2_m2m_buf_done(buf, state);
	mutex_unlock(&ctx->buff_done_lock);
}

static void comp_buf_set_vframe(struct aml_vcodec_ctx *ctx,
			 struct vb2_buffer *vb,
			 struct vframe_s *vf)
{
	dmabuf_set_vframe(vb->planes[0].dbuf, vf, VF_SRC_DECODER);
}

void fbc_transcode_and_set_vf(struct aml_vcodec_ctx *ctx,
						  struct aml_buf *aml_buf,
						  struct vframe_s *vf)
{
	struct vb2_buffer *vb2_buf = aml_buf->vb;
	struct vb2_v4l2_buffer *vb = to_vb2_v4l2_buffer(vb2_buf);
	struct aml_v4l2_buf *dstbuf =
		container_of(vb, struct aml_v4l2_buf, vb);

	if (is_cpu_t7()) {
		if (vf->canvas0_config[0].block_mode == CANVAS_BLKMODE_LINEAR) {
			if ((ctx->output_pix_fmt != V4L2_PIX_FMT_H264) &&
				(ctx->output_pix_fmt != V4L2_PIX_FMT_MPEG1) &&
				(ctx->output_pix_fmt != V4L2_PIX_FMT_MPEG2) &&
				(ctx->output_pix_fmt != V4L2_PIX_FMT_MPEG4) &&
				(ctx->output_pix_fmt != V4L2_PIX_FMT_MJPEG)) {
				vf->flag |= VFRAME_FLAG_VIDEO_LINEAR;
			}
			else {
				if (aml_buf->state == FB_ST_GE2D)
					vf->flag |= VFRAME_FLAG_VIDEO_LINEAR;
			}
		}
	} else {
		if (vf->canvas0_config[0].block_mode == CANVAS_BLKMODE_LINEAR)
			vf->flag |= VFRAME_FLAG_VIDEO_LINEAR;
	}
	if (!ctx->enable_di_post) {
		vf->index_disp = ctx->index_disp;
		vf->omx_index = vf->index_disp;

		if (vb2_buf->memory == VB2_MEMORY_DMABUF) {
			struct dma_buf * dma;

			dma = dstbuf->vb.vb2_buf.planes[0].dbuf;
			if (dmabuf_is_uvm(dma)) {
				/* only Y will contain vframe */
				comp_buf_set_vframe(ctx, vb2_buf, vf);
				v4l_dbg(ctx, V4L_DEBUG_CODEC_EXINFO,
					"set vf(%px) into %dth buf\n",
					vf, vb2_buf->index);
			}
		}
	}
}

ssize_t dump_cma_and_sys_memsize(struct aml_vcodec_ctx *ctx, char *buf)
{
	struct cma_sys_size_info *info = &ctx->mem_size_info;
	int cma_size = codec_mm_alloc_cma_size();
	int sys_size = codec_mm_alloc_sys_size();
	int total_size = cma_size + sys_size;
	char *pbuf = buf;

	if (total_size > info->max_total_size) {
		info->max_total_size = total_size;
		info->cma_part = cma_size;
		info->sys_part = sys_size;
	}

	info->max_cma_size = cma_size > info->max_cma_size ?
	cma_size : info->max_cma_size;

	info->max_sys_size = sys_size > info->max_sys_size ?
	sys_size : info->max_sys_size;

	info->cur_cma_size = cma_size;
	info->cur_sys_size = sys_size;

	if (pbuf) {
		pbuf += sprintf(pbuf, "TOTAL[Max:%dM(CMA:%dM, SYS:%dM), CUR:%dM], "
			"CMA[%dM, Max:%dM], SYS[%dM, Max:%dM]\n",
			info->max_total_size, info->cma_part,
			info->sys_part, total_size,
			info->cur_cma_size, info->max_cma_size,
			info->cur_sys_size, info->max_sys_size);
	} else {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_COUNT,
			"TOTAL[Max:%dM(CMA:%dM, SYS:%dM), CUR:%dM], "
			"CMA[%dM, Max:%dM], SYS[%dM, Max:%dM]\n",
			info->max_total_size, info->cma_part,
			info->sys_part, total_size,
			info->cur_cma_size, info->max_cma_size,
			info->cur_sys_size, info->max_sys_size);
	}

	return pbuf - buf;
}

 static void post_frame_to_upper(struct aml_vcodec_ctx *ctx,
	struct aml_buf *aml_buf)
{
	struct vb2_buffer *vb2_buf = aml_buf->vb;
	struct vb2_v4l2_buffer *vb = to_vb2_v4l2_buffer(vb2_buf);
	struct aml_v4l2_buf *dstbuf =
		container_of(vb, struct aml_v4l2_buf, vb);
	struct vframe_s *vf = &aml_buf->vframe;
	struct aml_vdec_cfg_infos *cfg = &ctx->config.parm.dec.cfg;
	struct aml_buf_plane *planes = aml_buf->planes; // DW def.

	dump_cma_and_sys_memsize(ctx, NULL);

	if (!ctx->enable_di_post) {
		vf->index_disp = ctx->index_disp;
		if ((vf->type & VIDTYPE_V4L_EOS) == 0)
			ctx->index_disp++;
	}

	ctx->post_to_upper_done = false;

	if ((cfg->double_write_mode == DM_AVBC_ONLY) ||
		(ctx->force_tw_output && cfg->triple_write_mode)) {
		if (aml_buf->planes_tw[0].bytes_used)
			planes = aml_buf->planes_tw;
	}

	v4l_dbg(ctx, V4L_DEBUG_CODEC_OUTPUT,
		"OUT_BUFF (%s, st:%d, seq:%d, idx:%d) vb:(%d, %px), vf:(%d, %px), ts:%llu, flag: 0x%x "
		"Y:(%lx, %u) C/U:(%lx, %u) V:(%lx, %u)\n",
		ctx->ada_ctx->frm_name, aml_buf->state, ctx->out_buff_cnt, aml_buf->index,
		vb2_buf->index, vb2_buf,
		vf->index & 0xff, vf,
		vf->timestamp,
		vf->flag,
		planes[0].addr, planes[0].length,
		planes[1].addr, planes[1].length,
		planes[2].addr, planes[2].length);
	ctx->out_buff_cnt++;

	if (dstbuf->aml_buf->num_planes == 1) {
		vb2_set_plane_payload(vb2_buf, 0, planes[0].bytes_used);
	} else if (dstbuf->aml_buf->num_planes == 2) {
		vb2_set_plane_payload(vb2_buf, 0, planes[0].bytes_used);
		vb2_set_plane_payload(vb2_buf, 1, planes[1].bytes_used);
	}

	vb2_buf->timestamp = vf->timestamp;
	dstbuf->vb.flags |= vf->frame_type;

	if ((ctx->picinfo.field == V4L2_FIELD_INTERLACED) && (!ctx->vpp_is_need)) {
		vb->field = V4L2_FIELD_INTERLACED;
	}

	do {
		u32 dw_mode = DM_YUV_ONLY;
		u32 tw_mode = DM_INVALID;
		struct file *fp;
		char file_name[64] = {0};

		if (!dump_capture_frame || ctx->is_drm_mode)
			break;

		if (vdec_if_get_param(ctx, GET_PARAM_DW_MODE, &dw_mode))
			break;

		if (vdec_if_get_param(ctx, GET_PARAM_TW_MODE, &tw_mode))
			break;

		if ((dw_mode == DM_AVBC_ONLY) && (tw_mode == DM_INVALID))
			break;

		snprintf(file_name, 64, "%s/dec_dump_%ux%u.raw", dump_path, vf->width, vf->height);

		fp = media_open(file_name,
				O_CREAT | O_RDWR | O_LARGEFILE | O_APPEND, 0666);

		if (!IS_ERR(fp)) {
			struct vb2_buffer *vb = vb2_buf;
			// dump y data
			u8 *yuv_data_addr = aml_yuv_dump(fp, (u8 *)vb2_plane_vaddr(vb, 0),
				vf->width, vf->height, 64);
			// dump uv data
			if (vb->num_planes == 1) {
				aml_yuv_dump(fp, yuv_data_addr, vf->width,
					vf->height / 2, 64);
			} else {
				aml_yuv_dump(fp, (u8 *)vb2_plane_vaddr(vb, 1),
					vf->width, vf->height / 2, 64);
			}

			pr_info("dump idx: %d %dx%d\n", dump_capture_frame, vf->width, vf->height);
			dump_capture_frame--;
			media_close(fp, NULL);
		}
	} while(0);


	vdec_tracing(&ctx->vtr, VTRACE_V4L_PIC_6, vb2_buf->index);
	vdec_tracing(&ctx->vtr, VTRACE_V4L_ST_1, vdec_frame_number(ctx->ada_ctx));
	vdec_tracing(&ctx->vtr, VTRACE_V4L_PIC_10,
			CTX_BUF_TOTAL(ctx) + ctx->out_buff_cnt - ctx->in_buff_cnt);

	if (vf->flag & VFRAME_FLAG_EMPTY_FRAME_V4L) {
		dstbuf->vb.flags = V4L2_BUF_FLAG_LAST;
		if (dstbuf->aml_buf->num_planes == 1) {
			vb2_set_plane_payload(vb2_buf, 0, 0);
		} else if (dstbuf->aml_buf->num_planes == 2) {
			vb2_set_plane_payload(vb2_buf, 0, 0);
			vb2_set_plane_payload(vb2_buf, 1, 0);
		}
		ctx->has_receive_eos = true;
		v4l_dbg(ctx, V4L_DEBUG_CODEC_BUFMGR,
			"receive a empty frame. idx: %d, state: %d\n",
			vb2_buf->index, vb2_buf->state);
	}

	v4l_dbg(ctx, V4L_DEBUG_CODEC_EXINFO,
		"receive vbuf idx: %d, state: %d\n",
		vb2_buf->index, vb2_buf->state);

	if (vf->flag & VFRAME_FLAG_EMPTY_FRAME_V4L) {
		if (ctx->v4l_resolution_change) {
			/* make the run to stanby until new buffs to enqueue. */
			ctx->reset_flag = V4L_RESET_MODE_LIGHT;
			ctx->vpp_cfg.res_chg = true;
			ctx->last_decoded_picinfo = ctx->picinfo;
			/*
			 * After all buffers containing decoded frames from
			 * before the resolution change point ready to be
			 * dequeued on the CAPTURE queue, the driver sends a
			 * V4L2_EVENT_SOURCE_CHANGE event for source change
			 * type V4L2_EVENT_SRC_CH_RESOLUTION, also the upper
			 * layer will get new information from cts->picinfo.
			 */
			aml_vdec_dispatch_event(ctx, V4L2_EVENT_SRC_CH_RESOLUTION);
		} else
			aml_vdec_dispatch_event(ctx, V4L2_EVENT_SEND_EOS);
	}

	if (dstbuf->vb.vb2_buf.state == VB2_BUF_STATE_ACTIVE) {
		if (!ctx->no_fbc_output &&
			!(vf->flag & VFRAME_FLAG_EMPTY_FRAME_V4L)) {
			ctx->fbc_transcode_and_set_vf(ctx, aml_buf, vf);
		}

		dstbuf->privdata.vf = *vf;

		if (vf->frame_type & V4L2_BUF_FLAG_ERROR)
			v4l2_buff_done(&dstbuf->vb, VB2_BUF_STATE_ERROR);
		else
			v4l2_buff_done(&dstbuf->vb, VB2_BUF_STATE_DONE);

		aml_buf->state = FB_ST_DISPLAY;
	}

	mutex_lock(&ctx->state_lock);
	if (ctx->state == AML_STATE_FLUSHING &&
		ctx->has_receive_eos) {
		ctx->state = AML_STATE_FLUSHED;
		vdec_tracing(&ctx->vtr, VTRACE_V4L_ST_0, ctx->state);
		v4l_dbg(ctx, V4L_DEBUG_CODEC_STATE,
			"vcodec state (AML_STATE_FLUSHED)\n");
	}
	mutex_unlock(&ctx->state_lock);

	if (ctx->post_to_upper_done == false) {
		ctx->post_to_upper_done = true;
		wake_up_interruptible(&ctx->post_done_wq);
	}

	ctx->decoded_frame_cnt++;
}

static void fill_capture_done_cb(void *v4l_ctx, void *fb_ctx)
{
	struct aml_vcodec_ctx *ctx =
		(struct aml_vcodec_ctx *)v4l_ctx;
	struct aml_buf *aml_buf = (struct aml_buf *)fb_ctx;
	struct vb2_buffer *vb2_buf = aml_buf->vb;
	struct vb2_v4l2_buffer *vb = to_vb2_v4l2_buffer(vb2_buf);
	if (ctx->is_stream_off) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_INPUT,
			"ignore buff idx: %d streamoff\n", aml_buf->index);
		return;
	}

	vdec_tracing(&ctx->vtr, VTRACE_V4L_PIC_5, vb->vb2_buf.index);

	mutex_lock(&ctx->capture_buffer_lock);
	kfifo_put(&ctx->capture_buffer, vb);
	mutex_unlock(&ctx->capture_buffer_lock);
	aml_thread_post_task(ctx, AML_THREAD_CAPTURE);
	aml_buf_update_holder(&ctx->bm, aml_buf, BUF_USER_VSINK, BUF_GET);
}

static struct task_ops_s *get_v4l_sink_ops(void);

void aml_creat_pipeline(struct aml_vcodec_ctx *ctx,
		       struct aml_buf *aml_buf,
		       u32 requester)
{
	struct task_chain_s *task = aml_buf->task;
	/*
	 * line 1: dec <==> vpp <==> v4l-sink, for P / P + DI.NR.
	 * line 2: dec <==> vpp, vpp <==> v4l-sink, for I / I + DI.NR.
	 * line 3: dec <==> v4l-sink, only for P.
	 * line 4: dec <==> ge2d, ge2d <==> v4l-sink, used for fmt convert.
	 * line 5: dec <==> ge2d, ge2d <==>vpp, vpp <==> v4l-sink.
	 * line 6: dec <==> ge2d, ge2d <==> vpp <==> v4l-sink.
	 */

	switch (requester) {
	case AML_FB_REQ_DEC:
		if (ctx->ge2d) {
			/* dec <==> ge2d. */
			task->attach(task, get_ge2d_ops(), ctx->ge2d);
		} else if (ctx->vpp) {
			if (ctx->vpp->is_prog) {
				/* dec <==> vpp <==> v4l-sink. */
				task->attach(task, get_v4l_sink_ops(), ctx);
				task->attach(task, get_vpp_ops(), ctx->vpp);
			} else {
				/* dec <==> vpp. */
				task->attach(task, get_vpp_ops(), ctx->vpp);
			}
		} else {
			/* dec <==> v4l-sink. */
			task->attach(task, get_v4l_sink_ops(), ctx);
		}
		break;

	case AML_FB_REQ_GE2D:
		if (ctx->vpp) {
			if (ctx->vpp->is_prog) {
				/* ge2d <==> vpp <==> v4l-sink. */
				task->attach(task, get_v4l_sink_ops(), ctx);
				task->attach(task, get_vpp_ops(), ctx->vpp);
				task->attach(task, get_ge2d_ops(), ctx->ge2d);
			} else {
				/* ge2d <==> vpp. */
				task->attach(task, get_vpp_ops(), ctx->vpp);
				task->attach(task, get_ge2d_ops(), ctx->ge2d);
			}
		} else {
			/* ge2d <==> v4l-sink. */
			task->attach(task, get_v4l_sink_ops(), ctx);
			task->attach(task, get_ge2d_ops(), ctx->ge2d);
		}
		break;

	case AML_FB_REQ_VPP:
		/* vpp <==> v4l-sink. */
		task->attach(task, get_v4l_sink_ops(), ctx);
		task->attach(task, get_vpp_ops(), ctx->vpp);
		break;

	default:
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"unsupport requester %x\n", requester);
	}
}
void cal_compress_buff_info(ulong used_page_num, struct aml_vcodec_ctx *ctx)
{
	struct v4l_compressed_buffer_info *buf_info = &ctx->compressed_buf_info;
	u32 total_buffer_num = ctx->dpb_size;
	u32 cur_index = buf_info->recycle_num % total_buffer_num;
	u32 cur_avg_val_by_group;

	if (!(debug_mode & V4L_DEBUG_CODEC_COUNT))
		return;

	mutex_lock(&ctx->compressed_buf_info_lock);
	buf_info->used_page_sum += used_page_num;
	buf_info->used_page_distributed_array[(u32)used_page_num / PAGE_NUM_ONE_MB]++;

	buf_info->used_page_by_group = buf_info->used_page_by_group -
		buf_info->used_page_in_group[cur_index] + used_page_num;
	buf_info->used_page_in_group[cur_index] = used_page_num;
	cur_avg_val_by_group = buf_info->used_page_by_group / total_buffer_num;
	if (cur_avg_val_by_group > buf_info->max_avg_val_by_group)
		buf_info->max_avg_val_by_group = cur_avg_val_by_group;

	buf_info->recycle_num++;
	v4l_dbg(ctx, V4L_DEBUG_CODEC_BUFMGR,
		"4k_used_num %ld used_page_sum %llu used_page_by_group %u max_avg_val %u cur_avg_val %u buffer_num %d recycle_num %u\n",
		used_page_num, buf_info->used_page_sum, buf_info->used_page_by_group,
			buf_info->max_avg_val_by_group, cur_avg_val_by_group, total_buffer_num, buf_info->recycle_num);
	mutex_unlock(&ctx->compressed_buf_info_lock);
}

static struct task_ops_s v4l_sink_ops = {
	.type		= TASK_TYPE_V4L_SINK,
	.fill_buffer	= fill_capture_done_cb,
};

static struct task_ops_s *get_v4l_sink_ops(void)
{
	return &v4l_sink_ops;
}

ssize_t aml_vdec_basic_information(struct aml_vcodec_ctx *ctx, char *buf)
{
	struct aml_q_data *outq = NULL;
	struct aml_q_data *capq = NULL;
	struct vdec_pic_info pic;
	u32 dw_mode = -1;
	u32 tw_mode = -1;
	char *pbuf = buf;

	if (vdec_if_get_param(ctx, GET_PARAM_PIC_INFO, &pic)) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"get pic info err\n");
		return 0;
	}

	vdec_if_get_param(ctx, GET_PARAM_DW_MODE, &dw_mode);
	vdec_if_get_param(ctx, GET_PARAM_TW_MODE, &tw_mode);

	outq = aml_vdec_get_q_data(ctx, V4L2_BUF_TYPE_VIDEO_OUTPUT);
	capq = aml_vdec_get_q_data(ctx, V4L2_BUF_TYPE_VIDEO_CAPTURE);

	pbuf += sprintf(pbuf, "\n==== Show Basic Information ==== \n");
	pbuf += sprintf(pbuf, "Format     : %s\n", outq->fmt->name);
	pbuf += sprintf(pbuf, "Color space: %s\n", capq->fmt->name);
	pbuf += sprintf(pbuf, "Scan type  : %s\n",
		(pic.field == V4L2_FIELD_NONE) ? "Progressive" : "Interlaced");
	pbuf += sprintf(pbuf, "Resolution : visible(%dx%d), coded(%dx%d)\n",
		pic.visible_width, pic.visible_height,
		pic.coded_width, pic.coded_height);
	pbuf += sprintf(pbuf, "Buffer num : dec:%d, vpp:%d, ge2d:%d, margin:%d, total:%d\n",
		ctx->picinfo.dpb_frames, ctx->vpp_size, ctx->ge2d_size,
		ctx->picinfo.dpb_margin, CTX_BUF_TOTAL(ctx));

	pbuf += sprintf(pbuf, "Config     : DW/TW:(0x%x, 0x%x), drm:%d, byp:%d, lc:%d, nr:%d, ge2d:%x\n",
		dw_mode,
		tw_mode,
		ctx->is_drm_mode,
		ctx->vpp_cfg.is_bypass_p,
		ctx->vpp_cfg.enable_local_buf,
		ctx->vpp_cfg.enable_nr,
		ctx->ge2d_cfg.mode);
	pbuf += sprintf(pbuf, "write frames : %d, out_buff : %d in_buff : %d\n",
		ctx->write_frames, ctx->out_buff_cnt, ctx->in_buff_cnt);

	return pbuf - buf;
}

ssize_t aml_buffer_status(struct aml_vcodec_ctx *ctx, char *buf)
{
	struct vb2_v4l2_buffer *vb = NULL;
	struct aml_v4l2_buf *aml_buff = NULL;
	struct aml_buf *aml_buf = NULL;
	struct vb2_queue *q = NULL;
	ulong flags;
	int i;
	char *pbuf = buf;

	flags = aml_vcodec_ctx_lock(ctx);

	q = v4l2_m2m_get_vq(ctx->m2m_ctx, V4L2_BUF_TYPE_VIDEO_CAPTURE);
	if (!q->streaming) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"can't achieve buffers status before start streaming.\n");
	}
	if (!ctx->enable_di_post) {
		pbuf += sprintf(pbuf, "\n==== Show Buffer Status ======== \n");
		for (i = 0; i < q->num_buffers; ++i) {
			vb = to_vb2_v4l2_buffer(q->bufs[i]);
			aml_buff = container_of(vb, struct aml_v4l2_buf, vb);
			aml_buf = aml_buff->aml_buf;

			/* print out task chain status. */
			if (aml_buf)
				pbuf += task_chain_show(aml_buf->task, pbuf);
		}
	}

	aml_vcodec_ctx_unlock(ctx, flags);

	pbuf += sprintf(pbuf, "\n==== Show Buffer Status ======== \n");
	pbuf += buf_core_walk(&ctx->bm.bc, pbuf);

	return pbuf - buf;
}

void aml_compressed_info_show(struct aml_vcodec_ctx *ctx)
{
	struct aml_q_data *outq = NULL;
	struct vdec_pic_info pic;
	int i;
	u32 aerage_mem_size;
	u32 max_avg_val_by_proup;
	struct v4l_compressed_buffer_info *buffer = &ctx->compressed_buf_info;
	u64 used_page_sum = buffer->used_page_sum;

	if (!(debug_mode & V4L_DEBUG_CODEC_COUNT))
		return;

	if (vdec_if_get_param(ctx, GET_PARAM_PIC_INFO, &pic)) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"get pic info err\n");
		return;
	}

	outq = aml_vdec_get_q_data(ctx, V4L2_BUF_TYPE_VIDEO_OUTPUT);

	pr_info("==== Show mmu buffer info ======== \n");
	if (buffer->recycle_num == 0) {
		pr_info("No valid info \n");
		return;
	}
	mutex_lock(&ctx->compressed_buf_info_lock);
	v4l_dbg(ctx, V4L_DEBUG_CODEC_PRINFO,
		"Fmt:%s, DW/TW:(%x, %x), Res:%dx%d, DPB:%d\n",
		outq->fmt->name,
		ctx->config.parm.dec.cfg.double_write_mode,
		ctx->config.parm.dec.cfg.triple_write_mode,
		pic.visible_width,
		pic.visible_height,
		ctx->dpb_size);

	do_div(used_page_sum, buffer->recycle_num);
	aerage_mem_size = ((u32)used_page_sum * 100) / PAGE_NUM_ONE_MB;
	v4l_dbg(ctx, V4L_DEBUG_CODEC_PRINFO,
		"mmu mem recycle num: %u, average used mmu mem %u.%u%u(MB)\n",
		buffer->recycle_num, aerage_mem_size / 100, (aerage_mem_size % 100) / 10, aerage_mem_size % 10);

	max_avg_val_by_proup = buffer->max_avg_val_by_group * 100 / PAGE_NUM_ONE_MB;
	v4l_dbg(ctx, V4L_DEBUG_CODEC_PRINFO,
		"%d buffer in group, max avg used mem by group %u.%u%u(MB)\n", ctx->dpb_size,
		max_avg_val_by_proup / 100, (max_avg_val_by_proup % 100) / 10, max_avg_val_by_proup % 10);

	v4l_dbg(ctx, V4L_DEBUG_CODEC_PRINFO,"mmu mem used distribution ratio\n");

	for (i = 0; i < MAX_AVBC_BUFFER_SIZE; i++) {
		u32 count = buffer->used_page_distributed_array[i];
		//if (count)
			v4l_dbg(ctx, V4L_DEBUG_CODEC_PRINFO,
				"range %d [%dMB ~ %dMB] distribution num %d ratio %u%%\n",
				i, i, i+1, count, (count * 100) / buffer->recycle_num);
	}

	mutex_unlock(&ctx->compressed_buf_info_lock);
	pr_info("==== End Show mmu buffer info ========");
}

static void reconfig_vpp_status(struct aml_vcodec_ctx *ctx)
{
	if (bypass_nr_flag &&
		!ctx->vpp_cfg.is_prog &&
		((ctx->vpp_cfg.mode == VPP_MODE_NOISE_REDUC_LOCAL) ||
		(ctx->vpp_cfg.mode == VPP_MODE_NOISE_REDUC))) {
		ctx->vpp_cfg.enable_nr = 0;
		ctx->vpp_cfg.enable_local_buf = 0;

		ctx->vpp_cfg.mode = VPP_MODE_DI;
	}
}

static int is_vdec_ready(struct aml_vcodec_ctx *ctx)
{
	if (!is_input_ready(ctx->ada_ctx)) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"the decoder input has not ready.\n");
		return 0;
	}

	if (ctx->state == AML_STATE_PROBE) {
		mutex_lock(&ctx->state_lock);
		if (ctx->state == AML_STATE_PROBE) {
			ctx->state = AML_STATE_READY;
			vdec_tracing(&ctx->vtr, VTRACE_V4L_ST_0, ctx->state);
			v4l_dbg(ctx, V4L_DEBUG_CODEC_STATE,
				"vcodec state (AML_STATE_READY)\n");
		}
		mutex_unlock(&ctx->state_lock);
	}

	mutex_lock(&ctx->state_lock);
	if (ctx->state == AML_STATE_READY) {
		if (ctx->m2m_ctx->out_q_ctx.q.streaming &&
			ctx->m2m_ctx->cap_q_ctx.q.streaming) {
			ctx->state = AML_STATE_ACTIVE;
			vdec_tracing(&ctx->vtr, VTRACE_V4L_ST_0, ctx->state);
			v4l_dbg(ctx, V4L_DEBUG_CODEC_STATE,
				"vcodec state (AML_STATE_ACTIVE)\n");
		}
	}
	mutex_unlock(&ctx->state_lock);

	return 1;
}

void dmabuff_recycle_worker(struct work_struct *work)
{
	struct aml_vcodec_ctx *ctx =
		container_of(work, struct aml_vcodec_ctx, es_wkr_out);
	struct vb2_v4l2_buffer *vb = NULL;
	struct aml_v4l2_buf *buf = NULL;

	if (ctx->es_wkr_stop ||
		!kfifo_get(&ctx->dmabuff_recycle, &vb))
		return;

	buf = container_of(vb, struct aml_v4l2_buf, vb);

	v4l_dbg(ctx, V4L_DEBUG_CODEC_INPUT,
		"recycle buff idx: %d, vbuf: %lx\n", vb->vb2_buf.index,
		buf->addr ? buf->addr: (ulong)sg_dma_address(buf->out_sgt->sgl));

	vdec_tracing(&ctx->vtr, VTRACE_V4L_ES_9, vb->vb2_buf.index);

	if (vb->vb2_buf.state != VB2_BUF_STATE_ERROR)
		v4l2_buff_done(vb, buf->error ? VB2_BUF_STATE_ERROR :
			VB2_BUF_STATE_DONE);
}

void aml_recycle_dma_buffers(struct aml_vcodec_ctx *ctx, u32 handle)
{
	struct aml_vcodec_dev *dev = ctx->dev;
	struct vb2_v4l2_buffer *vb = NULL;
	struct vb2_queue *q = NULL;
	int index = handle & 0xf;
	ulong flags;

retry:
	spin_lock_irqsave(&ctx->es_wkr_slock, flags);

	if (ctx->es_wkr_stop) {
		spin_unlock_irqrestore(&ctx->es_wkr_slock, flags);

		v4l_dbg(ctx, V4L_DEBUG_CODEC_INPUT,
			"ignore buff idx: %d streamoff\n", index);
		return;
	}

	if (work_pending(&ctx->es_wkr_out)) {
		spin_unlock_irqrestore(&ctx->es_wkr_slock, flags);

		flush_work(&ctx->es_wkr_out);

		goto retry;
	}

	q = v4l2_m2m_get_vq(ctx->m2m_ctx,
		V4L2_BUF_TYPE_VIDEO_OUTPUT);

	vb = to_vb2_v4l2_buffer(q->bufs[index]);

	kfifo_put(&ctx->dmabuff_recycle, vb);

	queue_work(dev->decode_workqueue, &ctx->es_wkr_out);

	spin_unlock_irqrestore(&ctx->es_wkr_slock, flags);
}

static void aml_vdec_worker(struct work_struct *work)
{
	struct aml_vcodec_ctx *ctx =
		container_of(work, struct aml_vcodec_ctx, es_wkr_in);
	struct aml_vcodec_dev *dev = ctx->dev;
	struct aml_v4l2_buf *aml_vb;
	struct vb2_v4l2_buffer *vb2_v4l2;
	struct vb2_buffer *vb;
	struct aml_vcodec_mem buf;
	bool res_chg = false;
	int ret;

	if (!is_vdec_ready(ctx)) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"the decoder has not ready.\n");
		goto out;
	}

	vb2_v4l2 = v4l2_m2m_next_src_buf(ctx->m2m_ctx);
	if (vb2_v4l2 == NULL) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"src_buf empty.\n");
		goto out;
	}

	vb = (struct vb2_buffer *)vb2_v4l2;

	aml_vb = container_of(vb2_v4l2, struct aml_v4l2_buf, vb);
	if (aml_vb->lastframe) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_BUFMGR,
			"Got empty flush input buffer.\n");

		mutex_lock(&ctx->state_lock);
		if (ctx->state == AML_STATE_ACTIVE) {
			ctx->state = AML_STATE_FLUSHING;// prepare flushing
			vdec_tracing(&ctx->vtr, VTRACE_V4L_ST_0, ctx->state);
			v4l_dbg(ctx, V4L_DEBUG_CODEC_STATE,
				"vcodec state (AML_STATE_FLUSHING-LASTFRM)\n");
		}
		mutex_unlock(&ctx->state_lock);

		v4l2_m2m_src_buf_remove(ctx->m2m_ctx);

		/* sets eos data for vdec input. */
		aml_vdec_flush_decoder(ctx);

		goto out;
	}

	if (ctx->stream_mode &&
		!(ctx->output_pix_fmt == V4L2_PIX_FMT_VC1_ANNEX_G ||
		ctx->output_pix_fmt == V4L2_PIX_FMT_VC1_ANNEX_L)) {
		struct dmabuf_dmx_sec_es_data *es_data = (struct dmabuf_dmx_sec_es_data *)aml_vb->dma_buf;
		int offset = vb->planes[0].data_offset;
		buf.addr = es_data->data_start + offset;
		buf.size = vb->planes[0].bytesused - offset;
		buf.dbuf = vb->planes[0].dbuf;
		v4l_dbg(ctx, V4L_DEBUG_CODEC_INPUT, "stream update wp 0x%lx + sz 0x%x offset 0x%x ori start 0x%x ts %llu\n",
			buf.addr, buf.size, offset, es_data->data_start, vb->timestamp);
	} else {
		buf.addr	= aml_vb->addr ? aml_vb->addr : sg_dma_address(aml_vb->out_sgt->sgl);
		buf.size	= vb->planes[0].bytesused;
	}
	buf.index	= vb->index;
	if (!ctx->is_drm_mode)
		buf.vaddr = vb2_plane_vaddr(vb, 0);

	buf.model	= vb->memory;
	buf.timestamp = vb->timestamp;
	buf.meta_ptr	= (ulong)aml_vb->meta_data;

	if (!buf.vaddr && !buf.addr) {
		v4l2_m2m_src_buf_remove(ctx->m2m_ctx);
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"id=%d src_addr is NULL.\n", vb->index);
		goto out;
	}

	/* v4l_dbg(ctx, V4L_DEBUG_CODEC_EXINFO,
		"size: 0x%zx, crc: 0x%x\n",
		buf.size, crc32(0, buf.va, buf.size));*/

	/* pts = (time / 10e6) * (90k / fps) */
	/*v4l_dbg(ctx, V4L_DEBUG_CODEC_EXINFO,
		"timestamp: 0x%llx\n", src_buf->timestamp);*/

	if ((!ctx->stream_mode) && ctx->output_dma_mode) {
		vdec_tracing(&ctx->vtr, VTRACE_V4L_ES_3, buf.size);
	} else {
		vdec_tracing(&ctx->vtr, VTRACE_V4L_ES_2, buf.size);
	}

	vdec_tracing(&ctx->vtr, VTRACE_V4L_ST_1, vdec_frame_number(ctx->ada_ctx));

	ret = vdec_if_decode(ctx, &buf, &res_chg);
	if (ret > 0) {
		v4l2_m2m_src_buf_remove(ctx->m2m_ctx);

		/* frame mode and non-dbuf mode. */
		if (ctx->stream_mode || !ctx->output_dma_mode) {
			vdec_tracing(&ctx->vtr, VTRACE_V4L_ES_8, buf.size);
			v4l2_buff_done(&aml_vb->vb,
				VB2_BUF_STATE_DONE);
		}
	} else if (ret && ret != -EAGAIN) {
		v4l2_m2m_src_buf_remove(ctx->m2m_ctx);

		/* frame mode and non-dbuf mode. */
		if (ctx->stream_mode || !ctx->output_dma_mode) {
			vdec_tracing(&ctx->vtr, VTRACE_V4L_ES_10, buf.size);
			v4l2_buff_done(&aml_vb->vb,
				VB2_BUF_STATE_ERROR);
		}

		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"error processing src data. %d.\n", ret);
	} else {
		/* ES frame write again. */
		vdec_tracing(&ctx->vtr, VTRACE_V4L_ES_11, buf.size);
	}
out:
	v4l2_m2m_job_finish(dev->m2m_dev_dec, ctx->m2m_ctx);
}

static void aml_vdec_reset(struct aml_vcodec_ctx *ctx)
{
	if (ctx->state == AML_STATE_ABORT) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"the decoder will be exited.\n");
		goto out;
	}

	aml_codec_disconnect(ctx->ada_ctx);

	if (aml_codec_reset(ctx->ada_ctx, &ctx->reset_flag)) {
		ctx->state = AML_STATE_ABORT;
		vdec_tracing(&ctx->vtr, VTRACE_V4L_ST_0, ctx->state);
		v4l_dbg(ctx, V4L_DEBUG_CODEC_STATE,
			"vcodec state (AML_STATE_ABORT).\n");
	}
out:
	complete(&ctx->comp);
}

void stop_pipeline(struct aml_vcodec_ctx *ctx)
{
	if (ctx->ge2d) {
		aml_v4l2_ge2d_thread_stop(ctx->ge2d);
		v4l_dbg(ctx, V4L_DEBUG_CODEC_PRINFO, "ge2d stop.\n");
	}

	if (ctx->vpp) {
		aml_v4l2_vpp_thread_stop(ctx->vpp);
		v4l_dbg(ctx, V4L_DEBUG_CODEC_PRINFO, "vpp stop\n");
	}
}

void wait_vcodec_ending(struct aml_vcodec_ctx *ctx)
{
	/* disable queue output item to worker. */
	ctx->output_thread_ready = false;
	ctx->is_stream_off = true;

	/* flush output buffer worker. */
	cancel_work_sync(&ctx->es_wkr_in);
	cancel_work_sync(&ctx->es_wkr_out);

	/* clean output cache and decoder status . */
	if (ctx->state > AML_STATE_INIT)
		aml_vdec_reset(ctx);

	stop_pipeline(ctx);
}

void aml_thread_capture_worker(struct aml_vcodec_ctx *ctx)
{
	struct vb2_v4l2_buffer *vb = NULL;
	struct aml_v4l2_buf *aml_buff = NULL;
	struct aml_buf *aml_buf = NULL;

	for (;;) {
		mutex_lock(&ctx->capture_buffer_lock);
		if (!kfifo_get(&ctx->capture_buffer, &vb)) {
			mutex_unlock(&ctx->capture_buffer_lock);
			break;
		}
		mutex_unlock(&ctx->capture_buffer_lock);

		aml_buff = container_of(vb, struct aml_v4l2_buf, vb);
		aml_buf = aml_buff->aml_buf;

		if (ctx->is_stream_off)
			continue;

		post_frame_to_upper(ctx, aml_buf);
	}
}
EXPORT_SYMBOL_GPL(aml_thread_capture_worker);

static int vdec_capture_thread(void *data)
{
	struct aml_vdec_thread *thread =
		(struct aml_vdec_thread *) data;
	struct aml_vcodec_ctx *ctx =
		(struct aml_vcodec_ctx *) thread->priv;

	for (;;) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_EXINFO,
			"%s, state: %d\n", __func__, ctx->state);

		if (down_interruptible(&thread->sem))
			break;

		if (thread->stop)
			break;

		/* handle event. */
		thread->func(ctx);
	}

	while (!kthread_should_stop()) {
		usleep_range(1000, 2000);
	}

	return 0;
}

void aml_thread_post_task(struct aml_vcodec_ctx *ctx,
	enum aml_thread_type type)
{
	struct aml_vdec_thread *thread = NULL;
	ulong flags;

	spin_lock_irqsave(&ctx->tsplock, flags);
	list_for_each_entry(thread, &ctx->vdec_thread_list, node) {
		if (thread->task == NULL)
			continue;

		if (thread->type == type)
			up(&thread->sem);
	}
	spin_unlock_irqrestore(&ctx->tsplock, flags);
}
EXPORT_SYMBOL_GPL(aml_thread_post_task);

int aml_thread_start(struct aml_vcodec_ctx *ctx, aml_thread_func func,
	enum aml_thread_type type, const char *thread_name)
{
	struct aml_vdec_thread *thread;
	struct sched_param param = { .sched_priority = MAX_RT_PRIO - 1 };
	int ret = 0;

	thread = kzalloc(sizeof(*thread), GFP_KERNEL);
	if (thread == NULL)
		return -ENOMEM;

	thread->type = type;
	thread->func = func;
	thread->priv = ctx;
	sema_init(&thread->sem, 0);

	thread->task = kthread_run(vdec_capture_thread, thread, "aml-%s-%d", thread_name, ctx->id);
	if (IS_ERR(thread->task)) {
		ret = PTR_ERR(thread->task);
		thread->task = NULL;
		goto err;
	}
	sched_setscheduler_nocheck(thread->task, SCHED_FIFO, &param);

	v4l_dbg(ctx, V4L_DEBUG_CODEC_EXINFO,
			"%s, policy is:%d priority is:%d\n",
			__func__, thread->task->policy, thread->task->rt_priority);

	list_add(&thread->node, &ctx->vdec_thread_list);

	return 0;

err:
	kfree(thread);

	return ret;
}
EXPORT_SYMBOL_GPL(aml_thread_start);

void aml_thread_stop(struct aml_vcodec_ctx *ctx)
{
	struct aml_vdec_thread *thread = NULL;
	ulong flags;

	while (!list_empty(&ctx->vdec_thread_list)) {
		thread = list_entry(ctx->vdec_thread_list.next,
			struct aml_vdec_thread, node);
		spin_lock_irqsave(&ctx->tsplock, flags);
		list_del(&thread->node);
		spin_unlock_irqrestore(&ctx->tsplock, flags);

		thread->stop = true;
		up(&thread->sem);
		kthread_stop(thread->task);
		thread->task = NULL;
		kfree(thread);
	}
}
EXPORT_SYMBOL_GPL(aml_thread_stop);

static int vidioc_try_decoder_cmd(struct file *file, void *priv,
				struct v4l2_decoder_cmd *cmd)
{
	struct aml_vcodec_ctx *ctx = fh_to_ctx(priv);

	v4l_dbg(ctx, V4L_DEBUG_CODEC_PROT,
		"%s, cmd: %u\n", __func__, cmd->cmd);

	switch (cmd->cmd) {
	case V4L2_DEC_CMD_STOP:
	case V4L2_DEC_CMD_START:
		if (cmd->cmd == V4L2_DEC_CMD_START) {
			if (cmd->start.speed == ~0)
				cmd->start.speed = 0;
			if (cmd->start.format == ~0)
				cmd->start.format = 0;
		}

		if (cmd->flags == ~0)
			cmd->flags = 0;

		if ((cmd->flags != 0) && (cmd->flags != ~0)) {
			v4l_dbg(0, V4L_DEBUG_CODEC_ERROR,
				"cmd->flags=%u\n", cmd->flags);
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int vidioc_decoder_cmd(struct file *file, void *priv,
				struct v4l2_decoder_cmd *cmd)
{
	struct aml_vcodec_ctx *ctx = fh_to_ctx(priv);
	struct vb2_queue *src_vq, *dst_vq;
	int ret;

	v4l_dbg(ctx, V4L_DEBUG_CODEC_PROT,
		"%s, cmd: %u\n", __func__, cmd->cmd);

	ret = vidioc_try_decoder_cmd(file, priv, cmd);
	if (ret)
		return ret;

	switch (cmd->cmd) {
	case V4L2_DEC_CMD_STOP:
		src_vq = v4l2_m2m_get_vq(ctx->m2m_ctx,
				V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);
		if (!vb2_is_streaming(src_vq)) {
			v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
				"Output stream is off. No need to flush.\n");
			return 0;
		}

		if ((vdec_frame_number(ctx->ada_ctx) <= 0) && (v4l2_m2m_num_dst_bufs_ready(ctx->m2m_ctx) == 0)) {
			dst_vq = v4l2_m2m_get_vq(ctx->m2m_ctx,
				multiplanar ? V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE :
				V4L2_BUF_TYPE_VIDEO_CAPTURE);
			if (!vb2_is_streaming(dst_vq)) {
				v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
					"Capture stream is off. No need to flush.\n");
				return 0;
			}
		}

		/* flush pipeline */
		v4l2_m2m_buf_queue(ctx->m2m_ctx, &ctx->empty_flush_buf->vb);
		v4l2_m2m_try_schedule(ctx->m2m_ctx);//pay attention
		ctx->receive_cmd_stop = true;

		v4l_dbg(ctx, V4L_DEBUG_CODEC_BUFMGR,
			"%s, receive cmd stop and prepare flush pipeline.\n", __func__);
		break;

	case V4L2_DEC_CMD_START:
		dst_vq = v4l2_m2m_get_vq(ctx->m2m_ctx,
			multiplanar ? V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE :
			V4L2_BUF_TYPE_VIDEO_CAPTURE);
		vb2_clear_last_buffer_dequeued(dst_vq);//pay attention

		v4l_dbg(ctx, V4L_DEBUG_CODEC_BUFMGR,
			"%s, receive cmd start.\n", __func__);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

ulong get_addr(struct vb2_buffer *vb, int i)
{
	struct vb2_v4l2_buffer *vb2_v4l2 = container_of(vb,
		struct vb2_v4l2_buffer, vb2_buf);
	struct aml_v4l2_buf *buf = container_of(vb2_v4l2,
		struct aml_v4l2_buf, vb);
	struct aml_buf *am_buf = buf->aml_buf;

	return am_buf->is_delay_allocated ?
		sg_dma_address(am_buf->cap_sgt->sgl) :
		vb2_dma_contig_plane_dma_addr(vb, i);
}

static void aml_uvm_buf_delay_free(struct uvm_buf_obj *obj)
{
	struct mua_buffer *mbuf;
	struct aml_vcodec_ctx *ctx = obj->arg;

	mbuf = container_of(obj, struct mua_buffer, base);
	if (!mbuf)
		return;

	v4l_dbg(ctx, V4L_DEBUG_CODEC_BUFMGR,
		"%s ion buffer:%px/%px, dbuf:%px/%px\n",
		__func__, mbuf->ibuffer[0], mbuf->ibuffer[1],
		mbuf->idmabuf[0], mbuf->idmabuf[1]);

	if (mbuf->idmabuf[0]) {
		dma_buf_put(mbuf->idmabuf[0]);
	}
	if (mbuf->idmabuf[1]) {
		dma_buf_put(mbuf->idmabuf[1]);
	}

	kref_put(&ctx->ctx_ref, aml_v4l_ctx_release);

	kfree(mbuf);
}

static void aml_uvm_copy_sgt(struct sg_table *dst_table,
			     struct sg_table *src_table)
{
	struct scatterlist *dst_sgl = NULL;
	struct scatterlist *src_sgl = NULL;
	int i;

	dst_table->nents = src_table->nents;
	dst_table->orig_nents = src_table->orig_nents;

	dst_sgl = dst_table->sgl;
	src_sgl = src_table->sgl;

	for (i = 0; i < src_table->nents; i++) {
		sg_set_page(dst_sgl, sg_page(src_sgl), src_sgl->length, 0);
		sg_dma_address(dst_sgl) = sg_phys(src_sgl);
		sg_dma_len(dst_sgl) = sg_dma_len(src_sgl);
		dst_sgl = sg_next(dst_sgl);
		src_sgl = sg_next(src_sgl);
	}
}

static int aml_uvm_buf_delay_alloc(struct aml_vcodec_ctx *ctx,
				  struct vb2_v4l2_buffer *vb)
{
	struct aml_v4l2_buf *buf =
		container_of(vb, struct aml_v4l2_buf, vb);
	struct aml_buf *am_buf = buf->aml_buf;
	struct dma_buf *dbuf = vb->vb2_buf.planes[0].dbuf;
	struct device *dev = vb->vb2_buf.vb2_queue->alloc_devs[0];
	struct uvm_handle *handle;
	struct mua_buffer *mbuf = NULL;
	struct dma_buf *idbuf = NULL;
	struct uvm_alloc *ua = NULL;
	struct uvm_buf_obj *obj = NULL;
	/* ion heap*/
	struct ion_buffer *ibuf = NULL;
	struct page *page = NULL;

	u64 time = local_clock();
	struct sg_table *cap_sgt;
	struct aml_uvm_buff_ref *ubuf = (struct aml_uvm_buff_ref *)am_buf->uvm_buf;
	struct sg_table *sgt;
	struct dma_buf_attachment *dba = NULL;

	if (!ctx->enable_di_post || ctx->picinfo.field == V4L2_FIELD_NONE ||
		!is_vdec_core_fmt(ctx))
		return 0;

	if ((vb->vb2_buf.memory != VB2_MEMORY_DMABUF) ||
		!dbuf ||
		!dmabuf_is_uvm(dbuf) ||
		am_buf->is_delay_allocated)
		return 0;

	obj = dmabuf_get_uvm_buf_obj(dbuf);
	mbuf = container_of(obj, struct mua_buffer, base);

	/* free fake dma buffer. */
	if (mbuf->idmabuf[0])
		dma_buf_put(mbuf->idmabuf[0]);
	if (mbuf->idmabuf[1])
		dma_buf_put(mbuf->idmabuf[1]);

	v4l_dbg(ctx, V4L_DEBUG_CODEC_BUFMGR,
				"dma buffer size(fake: %zu, real: %d)\n",
				mbuf->size,
				ctx->picinfo.y_len_sz + ctx->picinfo.c_len_sz);

	mbuf->size =  ctx->picinfo.y_len_sz + ctx->picinfo.c_len_sz;

	handle = dbuf->priv;
	if (handle->ua->obj->arg)
		kref_put(&ctx->ctx_ref, aml_v4l_ctx_release);

	if (ctx->master_buf) {
		struct aml_buf *master_buf = ctx->master_buf;

		if (master_buf->pair_state == MASTER_DONE) {
			master_buf->sub_buf[0] = (void *)am_buf;
			am_buf->master_buf = (void *)master_buf;
			am_buf->pair = BUF_SUB0;
			aml_buf_put(&ctx->bm, am_buf);
		}

		if (master_buf->pair_state == SUB0_DONE) {
			master_buf->sub_buf[1] = (void *)am_buf;
			am_buf->master_buf = (void *)master_buf;
			am_buf->pair = BUF_SUB1;
			aml_buf_put(&ctx->bm, am_buf);
		}

		am_buf->is_delay_allocated = true;
		master_buf->pair_state++;
		if (master_buf->pair_state == PAIR_DONE)
			ctx->master_buf = NULL;
		else
			aml_buf_get_ref(&ctx->bm, master_buf);
		idbuf = master_buf->idmabuf[0];
		sgt = master_buf->cap_sgt;
		get_dma_buf(idbuf);
		v4l_dbg(ctx, V4L_DEBUG_CODEC_BUFMGR,
				"[Paired]index(%d, %d), pair(%d), uvm dbuf: %px\n",
				master_buf->index, am_buf->index, am_buf->pair,dbuf);
	} else {
		if (ctx->alloc_type) { /* ion */
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
			u32 flags = 0;

			/* alloc real ion buffer relace older one. */
			if (mbuf->ion_flags & MUA_USAGE_PROTECTED)
				flags |= ION_FLAG_PROTECTED;

			flags |= ION_FLAG_EXTEND_MESON_HEAP;

			idbuf = ion_alloc(mbuf->size, (1 << ION_HEAP_TYPE_CUSTOM), flags);
			if (IS_ERR(idbuf) || !idbuf->priv) {
				v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
					"%s: ion alloc fail.\n", __func__);
				return -ENOMEM;
			}
#endif
		} else {  /* dma heap */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
			char *name = CODECMM_HEAP_NAME;
			struct dma_heap *heap;

			/* alloc real dma buffer relace older one. */
			if (mbuf->ion_flags & MUA_USAGE_PROTECTED)
				name = CODECMM_SECURE_HEAP_NAME;
			else if (mbuf->ion_flags & MUA_BUFFER_CACHED)
				name = CODECMM_CACHED_HEAP_NAME;

			heap = dma_heap_find(name);
			if (!heap) {
				v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
					"%s: dma_heap_find fail. heap name is %s\n", __func__, name);
				return -ENOMEM;
			}
			idbuf = dma_heap_buffer_alloc(heap, mbuf->size, O_RDWR,
					DMA_HEAP_VALID_HEAP_FLAGS);

			/* create attachment for the dmabuf with the user device */
			dba = dma_buf_attach(idbuf, dev);
			if (IS_ERR(dba)) {
				v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR, "failed to attach dmabuf\n");
				return 0;
			}

			/* get the associated scatterlist for this buffer */
			sgt = dma_buf_map_attachment(dba, DMA_BIDIRECTIONAL);
			if (IS_ERR(sgt)) {
				v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR, "Error getting dmabuf scatterlist\n");
				return 0;
			}
#endif
		}

		ctx->master_buf = am_buf;
		am_buf->pair_state++;
		am_buf->pair = BUF_MASTER;
		aml_buf_get_ref(&ctx->bm, am_buf);
		v4l_dbg(ctx, V4L_DEBUG_CODEC_BUFMGR,
				"[Pairing]index(%d), pair(%d), uvm dbuf: %px\n",
				am_buf->index, am_buf->pair, dbuf);
	}

	/* replace free cb. */
	ua		= handle->ua;
	ua->obj->arg	= ctx;
	ua->obj->dev	= dev;
	ua->free	= aml_uvm_buf_delay_free;

	if (ctx->alloc_type) {
		ibuf		= idbuf->priv;
		page		= sg_page(ibuf->sg_table->sgl);
		mbuf->paddr	= PFN_PHYS(page_to_pfn(page));
		mbuf->sg_table	= ibuf->sg_table;
		mbuf->ibuffer[0] = ibuf;
		mbuf->idmabuf[0] = idbuf;
		sgt		= ibuf->sg_table;
	} else {
		mbuf->paddr	= sg_dma_address(sgt->sgl);
		mbuf->sg_table	= ua->sgt;
		mbuf->ibuffer[0] = (void *)idbuf->priv;
		mbuf->idmabuf[0] = idbuf;
	}
	am_buf->idmabuf[0] = idbuf;
	/* update sg table. */
	aml_uvm_copy_sgt(ua->sgt, sgt);

	/* fill aml buffer information. */
	am_buf->cap_sgt	= ua->sgt;
	cap_sgt         = vb2_dma_sg_plane_desc(&vb->vb2_buf, 0);
	memcpy(cap_sgt, am_buf->cap_sgt, sizeof(struct sg_table));
	am_buf->is_delay_allocated = true;
	ubuf->addr	= get_addr(&vb->vb2_buf, 0);

	if (am_buf->pair == BUF_MASTER) {
		/* no need clear cache. If dev is ion, the cache has been flushed in ion_alloc;
		 * if dev is dma heap, dma_buf_map_attachment includes flush cache operation */
		#if 0
		/* clear cache. */
		if (dma_map_sg(dev, mbuf->sg_table->sgl,
			mbuf->sg_table->nents, DMA_FROM_DEVICE) <= 0) {
			v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
				"dma map sg %u bytes error\n", (u32)mbuf->size);
			return -ENOMEM;
		}
		dma_sync_sg_for_device(dev, mbuf->sg_table->sgl,
			mbuf->sg_table->nents, DMA_FROM_DEVICE);

		dma_unmap_sg(ua->obj->dev, mbuf->sg_table->sgl,
			mbuf->sg_table->nents, DMA_FROM_DEVICE);
		#endif
		/* unmap attachment and detach dbuf */
		if (dba) {
			dma_buf_unmap_attachment(dba, sgt, DMA_BIDIRECTIONAL);
			dma_buf_detach(mbuf->idmabuf[0], dba);
		}
	}

	kref_get(&ctx->ctx_ref);

	aml_buf_update(&ctx->bm, get_addr(&vb->vb2_buf, 0), am_buf);

	v4l_dbg(ctx, V4L_DEBUG_CODEC_BUFMGR,
		"%s, Res:%ux%u, stride:%d, addr:%lx, size:%u, flags:%x, cost:%llu ms\n",
		__func__, mbuf->width, mbuf->height, mbuf->byte_stride,
		(ulong)mbuf->paddr, (u32)mbuf->size, mbuf->ion_flags,
		div64_u64(local_clock() - time, 1000000));

	return 0;
}

static void aml_wait_resource(struct aml_vcodec_ctx *ctx)
{
	ulong expires = jiffies + msecs_to_jiffies(1000);

	while (atomic_read(&ctx->dev->vpp_count) >= max_di_instance) {
		if (time_after(jiffies, expires)) {
			v4l_dbg(ctx, V4L_DEBUG_CODEC_PRINFO,
				"wait resource timeout.\n");
			break;
		}
		usleep_range(2000, 4000);
	}
}

static int vidioc_decoder_streamon(struct file *file, void *priv,
	enum v4l2_buf_type i)
{
	struct v4l2_fh *fh = file->private_data;
	struct aml_vcodec_ctx *ctx = fh_to_ctx(fh);
	struct vb2_queue *q;

	q = v4l2_m2m_get_vq(fh->m2m_ctx, i);
	if (!V4L2_TYPE_IS_OUTPUT(q->type) &&
		ctx->is_stream_off) {
		if (ctx->ge2d_is_need) {
			int ret;

			if (ctx->ge2d) {
				aml_v4l2_ge2d_destroy(ctx->ge2d);
				ctx->ge2d = NULL;
			}

			ret = aml_v4l2_ge2d_init(ctx, &ctx->ge2d_cfg, &ctx->ge2d);
			if (ret) {
				v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
					"ge2d_wrapper init err:%d\n", ret);
				return ret;
			}
		}

		if (ctx->vpp_is_need) {
			int ret;

			if (ctx->vpp_cfg.fmt == 0)
				ctx->vpp_cfg.fmt = ctx->cap_pix_fmt;

			if (ctx->vpp == NULL &&
				vdec_get_instance_num() <= max_di_instance)
				aml_wait_resource(ctx);

			if ((atomic_read(&ctx->dev->vpp_count) < max_di_instance) ||
				(ctx->vpp != NULL)) {
				if (ctx->vpp && ctx->vpp_cfg.is_vpp_reset &&
					(ctx->vpp->is_prog == ctx->vpp_cfg.is_prog) &&
					(ctx->vpp->is_bypass_p == ctx->vpp_cfg.is_bypass_p) &&
					(ctx->vpp->work_mode == ctx->vpp_cfg.mode)) {
					aml_v4l2_vpp_reset(ctx->vpp);
				} else {
					if (ctx->vpp) {
						aml_v4l2_vpp_destroy(ctx->vpp);
						atomic_dec(&ctx->dev->vpp_count);
						ctx->vpp = NULL;
					}
					atomic_inc(&ctx->dev->vpp_count);
					ret = aml_v4l2_vpp_init(ctx, &ctx->vpp_cfg, &ctx->vpp);
					if (ret) {
						atomic_dec(&ctx->dev->vpp_count);
						v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
							"vpp_wrapper init err:%d vpp_cfg.fmt: %d\n",
							ret, ctx->vpp_cfg.fmt);
						return ret;
					}

					v4l_dbg(ctx, V4L_DEBUG_CODEC_PRINFO,
						"vpp_wrapper instance count: %d\n",
						atomic_read(&ctx->dev->vpp_count));
				}
			} else {
				ctx->vpp_cfg.enable_local_buf = 0;
				ctx->vpp_cfg.enable_nr = 0;
				ctx->picinfo.dpb_margin += ctx->vpp_size;
				ctx->dpb_size = ctx->picinfo.dpb_margin + ctx->picinfo.dpb_frames;
				ctx->vpp_size = 0;
				vdec_if_set_param(ctx, SET_PARAM_PIC_INFO, &ctx->picinfo);
				ctx->vpp_is_need = false;
			}
			ctx->vpp_cfg.is_vpp_reset = false;
		} else {
			if (ctx->vpp) {
				aml_v4l2_vpp_destroy(ctx->vpp);
				atomic_dec(&ctx->dev->vpp_count);
				ctx->vpp = NULL;
			}
		}

		ctx->is_stream_off = false;
	} else {
		ctx->is_out_stream_off = false;
		ctx->es_wkr_stop = false;
		aml_codec_connect(ctx->ada_ctx); /* for seek */

#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
		if (ctx->dv_id < 0) {
			dv_inst_map(&ctx->dv_id);
			v4l_dbg(ctx, V4L_DEBUG_CODEC_BUFMGR,
				"%s: dv_inst_map ctx %p, dv_id %d\n",__func__, ctx, ctx->dv_id);
		}
#endif
	}
	if (V4L2_TYPE_IS_OUTPUT(q->type)) {
		memset(&ctx->decoder_status_info, 0,
			sizeof(ctx->decoder_status_info));
	}
	v4l_dbg(ctx, V4L_DEBUG_CODEC_PROT,
		"%s, type: %d\n", __func__, q->type);

	return v4l2_m2m_ioctl_streamon(file, priv, i);
}

static int vidioc_decoder_streamoff(struct file *file, void *priv,
	enum v4l2_buf_type i)
{
	struct v4l2_fh *fh = file->private_data;
	struct aml_vcodec_ctx *ctx = fh_to_ctx(fh);
	struct vb2_queue *q;
	ulong flags;

	q = v4l2_m2m_get_vq(fh->m2m_ctx, i);

	flags = aml_vcodec_ctx_lock(ctx);

	if (V4L2_TYPE_IS_OUTPUT(q->type))
		ctx->is_out_stream_off = true;
	else
		ctx->is_stream_off = true;

	aml_vcodec_ctx_unlock(ctx, flags);

	if (!V4L2_TYPE_IS_OUTPUT(q->type)) {
		if (ctx->vpp) {
			reconfig_vpp_status(ctx);
		}
	} else {
		ctx->index_disp = 0;
	}

	v4l_dbg(ctx, V4L_DEBUG_CODEC_PROT,
		"%s, type: %d\n", __func__, q->type);

	return v4l2_m2m_ioctl_streamoff(file, priv, i);
}

static int vidioc_decoder_reqbufs(struct file *file, void *priv,
	struct v4l2_requestbuffers *rb)
{
	struct aml_vcodec_ctx *ctx = fh_to_ctx(priv);
	struct v4l2_fh *fh = file->private_data;
	struct vb2_queue *q;

	q = v4l2_m2m_get_vq(fh->m2m_ctx, rb->type);

	if (!rb->count) {
		if (!V4L2_TYPE_IS_OUTPUT(rb->type)) {
			if (wait_event_interruptible_timeout
				(ctx->post_done_wq, ctx->post_to_upper_done == true,
				 msecs_to_jiffies(200)) == 0) {
				v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
					"wait post frame to upper finish timeout.\n");
			}
		}
		vb2_queue_release(q);
	}

	v4l_dbg(ctx, V4L_DEBUG_CODEC_PROT,
		"%s, type: %d, count: %d\n",
		__func__, q->type, rb->count);

	if (!V4L2_TYPE_IS_OUTPUT(rb->type)) {
		/* driver needs match v4l buffer number with total size*/
		if (rb->count > CTX_BUF_TOTAL(ctx)) {
			v4l_dbg(ctx, V4L_DEBUG_CODEC_PROT,
					"reqbufs (st:%d) %d -> %d\n",
					ctx->state, rb->count, CTX_BUF_TOTAL(ctx));
			ctx->picinfo.dpb_margin += (rb->count - CTX_BUF_TOTAL(ctx));
			ctx->dpb_size = ctx->picinfo.dpb_frames + ctx->picinfo.dpb_margin;
			vdec_if_set_param(ctx, SET_PARAM_PIC_INFO, &ctx->picinfo);
			v4l_dbg(ctx, V4L_DEBUG_CODEC_PROT,
					"%s buf updated, dec: %d (%d + %d), vpp %d\n",
					__func__,
					ctx->dpb_size,
					ctx->picinfo.dpb_frames,
					ctx->picinfo.dpb_margin,
					ctx->vpp_size);
			//rb->count = ctx->dpb_size;
		}
		ctx->v4l_reqbuff_flag = true;
		ctx->capture_memory_mode = rb->memory;
		v4l_dbg(ctx, V4L_DEBUG_CODEC_OUTPUT,
			"capture buffer memory mode is %d\n", rb->memory);
	} else {
		ctx->output_dma_mode =
			(rb->memory == VB2_MEMORY_DMABUF) ? 1 : 0;
		vdec_set_dmabuf_type(ctx->ada_ctx, ctx->output_dma_mode);

		v4l_dbg(ctx, V4L_DEBUG_CODEC_INPUT,
			"output buffer memory mode is %d\n", rb->memory);
	}

	return v4l2_m2m_ioctl_reqbufs(file, priv, rb);
}

static int vidioc_vdec_querybuf(struct file *file, void *priv,
	struct v4l2_buffer *buf)
{
	struct aml_vcodec_ctx *ctx = fh_to_ctx(priv);

	v4l_dbg(ctx, V4L_DEBUG_CODEC_PROT,
		"%s, type: %d\n", __func__, buf->type);

	return v4l2_m2m_ioctl_querybuf(file, priv, buf);
}

static int vidioc_vdec_expbuf(struct file *file, void *priv,
	struct v4l2_exportbuffer *eb)
{
	struct aml_vcodec_ctx *ctx = fh_to_ctx(priv);

	v4l_dbg(ctx, V4L_DEBUG_CODEC_PROT,
		"%s, type: %d\n", __func__, eb->type);

	return v4l2_m2m_ioctl_expbuf(file, priv, eb);
}

static bool is_turn_around(struct aml_es_ref_elem *elem)
{
	return (elem->au_addr + elem->au_size) >=
		(elem->buf_start + elem->buf_size);
}

static ulong get_remain_pos(struct aml_es_ref_elem *elem)
{
	return is_turn_around(elem) ?
		elem->buf_start + (elem->au_size -
		(elem->buf_start + elem->buf_size - elem->au_addr)) :
		elem->au_addr + elem->au_size;
}

static int aml_es_ref_que(struct aml_es_mgr *stmgr, struct dma_buf *dbuf,
			    ulong addr, u32 size, u64 timestamp)
{
	struct aml_es_ref_elem *elem;

	/* 1.Get stbuf from free pool. */
	if (!kfifo_get(&stmgr->free_q, &elem)) {
		v4l_dbg(stmgr->ctx, 0, "Get elem from free queue fail.\n");
		return -EBADSLT;
	}

	/* 2.Fill stbuf info from dbuf. */
	elem->dbuf		= dbuf;
	elem->buf_start		= stmgr->buf_start;
	elem->buf_size		= stmgr->buf_size;
	elem->au_addr		= addr;
	elem->au_size		= size;
	elem->timestamp		= timestamp;

	if (stmgr->cursor_Wp == -1)
		stmgr->cursor_Wp = elem->au_addr;

	/* 3.Get dmabuf ref in driver. */
	get_dma_buf(elem->dbuf);

	atomic_inc(&stmgr->buf_cache);

	/* 4.Add stbuf into the management queue. */
	kfifo_put(&stmgr->ref_q, elem);
	v4l_dbg(stmgr->ctx, V4L_DEBUG_CODEC_INPUT,
		"Get-ref, CPB(%lx, %d), AU(%lx, %d, %d), "
		"cursor_Wp:%lx, ts:%llu, buf_cache:%d\n",
		elem->buf_start,
		elem->buf_size,
		elem->au_addr,
		elem->au_size,
		is_turn_around(elem),
		stmgr->cursor_Wp,
		elem->timestamp,
		atomic_read(&stmgr->buf_cache));

	return 0;
}

static void aml_es_put_ref(struct aml_es_mgr *stmgr,
			     struct aml_es_ref_elem *elem)
{
	/* Update cursor Wp. */
	stmgr->cursor_Wp = get_remain_pos(elem);
	atomic_dec(&stmgr->buf_cache);
	v4l_dbg(stmgr->ctx, V4L_DEBUG_CODEC_INPUT,
		"Put-ref, CPB(%lx, %d), AU(%lx, %d, %d), "
		"cursor_Wp:%lx, ts:%llu, buf_cache:%d\n",
		elem->buf_start,
		elem->buf_size,
		elem->au_addr,
		elem->au_size,
		is_turn_around(elem),
		stmgr->cursor_Wp,
		elem->timestamp,
		atomic_read(&stmgr->buf_cache));

	dma_buf_put(elem->dbuf);

	kfifo_put(&stmgr->free_q, elem);
}

static int aml_es_ref_dque(struct aml_es_mgr *stmgr, ulong Rp)
{
	struct aml_es_ref_elem *elem;
	ulong cursor_Wp = stmgr->cursor_Wp;

	v4l_dbg(stmgr->ctx, V4L_DEBUG_CODEC_INPUT,
		"Put-ref-start, cursor_Wp:%lx, Rp:%lx\n", cursor_Wp, Rp);

	/* Get stbuf from management queue. */
	if (kfifo_is_empty(&stmgr->ref_q)) {
		v4l_dbg(stmgr->ctx, 0, "There is no ref elem to process.\n");
		return 0;
	}
retry:
	if (!kfifo_peek(&stmgr->ref_q, &elem)) {
		v4l_dbg(stmgr->ctx, 0, "Peek elem from ref queue fail.\n");
		return -EINVAL;
	}

	/* Checks the stbuf whether was consumed. */
	if (Rp >= cursor_Wp) {
		/*
		 * If Rp is to the right of the cursor_Wp, except the AU_end
		 * with around, the others just should be checked Rp >= AU_end,
		 * means that the AU-data has been consumed.
		 */
		if (!is_turn_around(elem) &&
			(Rp >= get_remain_pos(elem))) {
			if (!kfifo_get(&stmgr->ref_q, &elem)) {
				v4l_dbg(stmgr->ctx, 0, "Get elem from ref queue fail.\n");
				return -EINVAL;
			}

			aml_es_put_ref(stmgr, elem);

			if (!kfifo_is_empty(&stmgr->ref_q))
				goto retry;
		}
	} else {
		/*
		 * Otherwise, there are 2 cases:
		 * 1. As long as the AU-data is on the right side of the cursor_Wp
		 *    and the AU_end is not around, then the AU-data is consumed
		 * 2. If Rp > AU_end then indicates that the AU-data is consumed.
		 */
		if ((!is_turn_around(elem) &&
			(elem->au_addr >= cursor_Wp)) ||
			(Rp >= get_remain_pos(elem))) {
			if (!kfifo_get(&stmgr->ref_q, &elem)) {
				v4l_dbg(stmgr->ctx, 0, "Get elem from ref queue fail.\n");
				return -EINVAL;
			}

			aml_es_put_ref(stmgr, elem);

			if (!kfifo_is_empty(&stmgr->ref_q))
				goto retry;
		}
	}

	return 0;
}

int aml_es_write(struct aml_vcodec_ctx *ctx, struct dma_buf *dbuf,
			ulong addr, u32 size, u64 timestamp)
{
	int ret = -1;

	/* 1.Increase dmabuf reference. */
	ret = aml_es_ref_que(&ctx->es_mgr, dbuf, addr, size, timestamp);
	if (ret)
		return ret;

	/* 2.PTS checkin. */
	ctx->pts_serves_ops->checkin(ctx->ptsserver_id, size, timestamp);

	return ret;
}

void aml_es_input_free(struct aml_vcodec_ctx *ctx, ulong addr)
{
	aml_es_ref_dque(&ctx->es_mgr, addr);
}

int aml_es_mgr_init(struct aml_vcodec_ctx *ctx)
{
	struct aml_es_ref_elem *elems = NULL;
	struct aml_es_mgr *stmgr = &ctx->es_mgr;
	int ret = -1, i;

	INIT_KFIFO(stmgr->free_q);
	INIT_KFIFO(stmgr->ref_q);

	ret = kfifo_alloc(&stmgr->free_q, AML_ES_REF_MAX, GFP_KERNEL);
	if (ret) {
		v4l_dbg(stmgr->ctx, 0, "Alloc free_q fifo fail.\n");
		return -ENOMEM;
	}

	ret = kfifo_alloc(&stmgr->ref_q, AML_ES_REF_MAX, GFP_KERNEL);
	if (ret) {
		kfifo_free(&stmgr->free_q);
		v4l_dbg(stmgr->ctx, 0, "Alloc ref_q fifo fail.\n");
		return -ENOMEM;
	}

	elems = vzalloc(sizeof(*elems) * AML_ES_REF_MAX);
	if (!elems) {
		kfifo_free(&stmgr->free_q);
		kfifo_free(&stmgr->ref_q);
		return -ENOMEM;
	}

	for (i = 0; i < AML_ES_REF_MAX; i++) {
		kfifo_put(&stmgr->free_q, &elems[i]);
	}

	atomic_set(&stmgr->buf_cache, 0);

	stmgr->cursor_Wp	= -1;
	stmgr->elems		= elems;
	stmgr->ctx		= ctx;

	return 0;
}

void aml_es_mgr_release(struct aml_vcodec_ctx *ctx)
{
	struct aml_es_ref_elem *elem;
	struct aml_es_mgr *stmgr = &ctx->es_mgr;

	if (!stmgr->elems)
		return;

retry:
	if (!kfifo_is_empty(&stmgr->ref_q)) {
		if (!kfifo_get(&stmgr->ref_q, &elem)) {
			v4l_dbg(stmgr->ctx, 0, "Get elem from ref queue fail.\n");
			goto out;
		}

		aml_es_put_ref(stmgr, elem);

		goto retry;
	}
out:
	kfifo_free(&stmgr->free_q);

	kfifo_free(&stmgr->ref_q);

	vfree(stmgr->elems);
}

void aml_vcodec_dec_release(struct aml_vcodec_ctx *ctx)
{
	ulong flags;
	if (ctx->capture_memory_mode == VB2_MEMORY_MMAP) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_BUFMGR,"clean proxy uvm\n");
		aml_clean_proxy_uvm(ctx);
	} else
		aml_buf_put_dma(&ctx->bm);

	flags = aml_vcodec_ctx_lock(ctx);
	ctx->state = AML_STATE_ABORT;
	vdec_tracing(&ctx->vtr, VTRACE_V4L_ST_0, ctx->state);
	v4l_dbg(ctx, V4L_DEBUG_CODEC_STATE,
		"vcodec state (AML_STATE_ABORT)\n");
	aml_vcodec_ctx_unlock(ctx, flags);

	vdec_if_deinit(ctx);
}

void aml_vcodec_dec_set_default_params(struct aml_vcodec_ctx *ctx)
{
	struct aml_q_data *q_data;

	ctx->m2m_ctx->q_lock = &ctx->v4l_intf_lock;
	ctx->fh.m2m_ctx = ctx->m2m_ctx;
	ctx->fh.ctrl_handler = &ctx->ctrl_hdl;
	INIT_WORK(&ctx->es_wkr_in, aml_vdec_worker);
	ctx->colorspace = V4L2_COLORSPACE_REC709;
	ctx->ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
	ctx->quantization = V4L2_QUANTIZATION_DEFAULT;
	ctx->xfer_func = V4L2_XFER_FUNC_DEFAULT;
	ctx->dev->dec_capability = 0;//VCODEC_CAPABILITY_4K_DISABLED;//disable 4k
	if (get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_T5D) {
		ctx->dev->dec_capability = VCODEC_CAPABILITY_4K_DISABLED;
	}

	if (t3x_tw_output && (get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_T3X))
		ctx->force_tw_output = 1;

	q_data = &ctx->q_data[AML_Q_DATA_SRC];
	memset(q_data, 0, sizeof(struct aml_q_data));
	q_data->visible_width = DFT_CFG_WIDTH;
	q_data->visible_height = DFT_CFG_HEIGHT;
	q_data->coded_width = DFT_CFG_WIDTH;
	q_data->coded_height = DFT_CFG_HEIGHT;
	q_data->fmt = &aml_video_formats[OUT_FMT_IDX];
	q_data->field = V4L2_FIELD_NONE;

	q_data->sizeimage[0] = (1024 * 1024);//DFT_CFG_WIDTH * DFT_CFG_HEIGHT; //1m
	q_data->bytesperline[0] = 0;

	q_data = &ctx->q_data[AML_Q_DATA_DST];
	memset(q_data, 0, sizeof(struct aml_q_data));
	q_data->visible_width = DFT_CFG_WIDTH;
	q_data->visible_height = DFT_CFG_HEIGHT;
	q_data->coded_width = DFT_CFG_WIDTH;
	q_data->coded_height = DFT_CFG_HEIGHT;
	q_data->fmt = &aml_video_formats[CAP_FMT_IDX];
	if (support_format_I420)
		q_data->fmt = &aml_video_formats[CAP_FMT_I420_IDX];

	q_data->field = V4L2_FIELD_NONE;

	v4l_bound_align_image(&q_data->coded_width,
				AML_VDEC_MIN_W,
				AML_VDEC_MAX_W, 4,
				&q_data->coded_height,
				AML_VDEC_MIN_H,
				AML_VDEC_MAX_H, 5, 6);

	q_data->sizeimage[0] = q_data->coded_width * q_data->coded_height;
	q_data->bytesperline[0] = q_data->coded_width;
	q_data->sizeimage[1] = q_data->sizeimage[0] / 2;
	q_data->bytesperline[1] = q_data->coded_width;

	q_data->sizeimage_tw[0] = q_data->sizeimage[0];
	q_data->bytesperline_tw[0] = q_data->bytesperline[0];
	q_data->sizeimage_tw[1] = q_data->sizeimage[1];
	q_data->bytesperline_tw[1] = q_data->bytesperline[1];

	ctx->reset_flag = V4L_RESET_MODE_NORMAL;

	vdec_trace_init(&ctx->vtr, ctx->id, -1);

	ctx->state = AML_STATE_IDLE;
	vdec_tracing(&ctx->vtr, VTRACE_V4L_ST_0, ctx->state);
	v4l_dbg(ctx, V4L_DEBUG_CODEC_STATE,
		"vcodec state (AML_STATE_IDLE)\n");
}

static int vidioc_vdec_qbuf(struct file *file, void *priv,
	struct v4l2_buffer *buf)
{
	struct aml_vcodec_ctx *ctx = fh_to_ctx(priv);
	int ret;

	v4l_dbg(ctx, V4L_DEBUG_CODEC_PROT,
		"%s, type: %d\n", __func__, buf->type);

	if (ctx->state == AML_STATE_ABORT) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"Call on QBUF after unrecoverable error, type = %s\n",
			V4L2_TYPE_IS_OUTPUT(buf->type) ? "OUT" : "IN");
		return -EIO;
	}

	ret = v4l2_m2m_qbuf(file, ctx->m2m_ctx, buf);

	if (V4L2_TYPE_IS_OUTPUT(buf->type)) {
		if (V4L2_TYPE_IS_MULTIPLANAR(buf->type)) {
			if (ret == -EAGAIN)
				vdec_tracing(&ctx->vtr, VTRACE_V4L_ES_1, buf->m.planes[0].bytesused);
			else
				vdec_tracing(&ctx->vtr, VTRACE_V4L_ES_0, buf->m.planes[0].bytesused);
		} else {
			if (ret == -EAGAIN)
				vdec_tracing(&ctx->vtr, VTRACE_V4L_ES_1, buf->length);
			else
				vdec_tracing(&ctx->vtr, VTRACE_V4L_ES_0, buf->length);
		}

		vdec_tracing(&ctx->vtr, VTRACE_V4L_ES_12, timeval_to_ns(&buf->timestamp));
	} else {
		if (ret == -EAGAIN)
			vdec_tracing(&ctx->vtr, VTRACE_V4L_PIC_1, buf->index);
		else
			vdec_tracing(&ctx->vtr, VTRACE_V4L_PIC_0, buf->index);
	}

	return ret;
}

static int vidioc_vdec_dqbuf(struct file *file, void *priv,
	struct v4l2_buffer *buf)
{
	struct aml_vcodec_ctx *ctx = fh_to_ctx(priv);
	int ret;

	v4l_dbg(ctx, V4L_DEBUG_CODEC_PROT,
		"%s, type: %d\n", __func__, buf->type);

	if (ctx->state == AML_STATE_ABORT) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"Call on DQBUF after unrecoverable error, type = %s\n",
			V4L2_TYPE_IS_OUTPUT(buf->type) ? "OUT" : "IN");
		if (!V4L2_TYPE_IS_OUTPUT(buf->type))
			return -EIO;
	}

	ret = v4l2_m2m_dqbuf(file, ctx->m2m_ctx, buf);

	if (V4L2_TYPE_IS_OUTPUT(buf->type)) {
		if (V4L2_TYPE_IS_MULTIPLANAR(buf->type)) {
			if (ret == -EAGAIN)
				vdec_tracing(&ctx->vtr, VTRACE_V4L_ES_7, buf->m.planes[0].bytesused);
			else
				vdec_tracing(&ctx->vtr, VTRACE_V4L_ES_6, buf->m.planes[0].bytesused);
		} else {
			if (ret == -EAGAIN)
				vdec_tracing(&ctx->vtr, VTRACE_V4L_ES_7, buf->length);
			else
				vdec_tracing(&ctx->vtr, VTRACE_V4L_ES_6, buf->length);
		}
	} else {
		if (ret == -EAGAIN)
			vdec_tracing(&ctx->vtr, VTRACE_V4L_PIC_8, buf->index);
		else
			vdec_tracing(&ctx->vtr, VTRACE_V4L_PIC_7, buf->index);

		vdec_tracing(&ctx->vtr, VTRACE_V4L_PIC_9, timeval_to_ns(&buf->timestamp));
	}

	return ret;
}

static int vidioc_vdec_querycap(struct file *file, void *priv,
	struct v4l2_capability *cap)
{
	struct aml_vcodec_ctx *ctx = fh_to_ctx(priv);
	struct video_device *vfd_dec = video_devdata(file);

	strlcpy(cap->driver, AML_VCODEC_DEC_NAME, sizeof(cap->driver));
	strlcpy(cap->bus_info, AML_PLATFORM_STR, sizeof(cap->bus_info));
	strlcpy(cap->card, AML_PLATFORM_STR, sizeof(cap->card));
	cap->device_caps = vfd_dec->device_caps;

	v4l_dbg(ctx, V4L_DEBUG_CODEC_PROT, "%s, %s\n", __func__, cap->card);

	return 0;
}

static int vidioc_vdec_subscribe_evt(struct v4l2_fh *fh,
	const struct v4l2_event_subscription *sub)
{
	struct aml_vcodec_ctx *ctx = fh_to_ctx(fh);

	v4l_dbg(ctx, V4L_DEBUG_CODEC_PROT,
		"%s, type: %d\n", __func__, sub->type);

	switch (sub->type) {
	case V4L2_EVENT_EOS:
		return v4l2_event_subscribe(fh, sub, 2, NULL);
	case V4L2_EVENT_SOURCE_CHANGE:
		return v4l2_src_change_event_subscribe(fh, sub);
	case V4L2_EVENT_PRIVATE_EXT_REPORT_ERROR_FRAME:
		return v4l2_event_subscribe(fh, sub, 10, NULL);
	case V4L2_EVENT_PRIVATE_EXT_SEND_ERROR:
		return v4l2_event_subscribe(fh, sub, 5, NULL);
	default:
		return v4l2_ctrl_subscribe_event(fh, sub);
	}
}

static int vidioc_vdec_event_unsubscribe(struct v4l2_fh *fh,
	const struct v4l2_event_subscription *sub)
{
	struct aml_vcodec_ctx *ctx = fh_to_ctx(fh);

	v4l_dbg(ctx, V4L_DEBUG_CODEC_PROT, "%s, type: %d\n",
		__func__, sub->type);

	return v4l2_event_unsubscribe(fh, sub);
}

static int vidioc_try_fmt(struct v4l2_format *f, struct aml_video_fmt *fmt)
{
	int i;
	struct v4l2_pix_format_mplane *pix_mp = &f->fmt.pix_mp;
	struct v4l2_pix_format *pix = &f->fmt.pix;

	if (V4L2_TYPE_IS_MULTIPLANAR(f->type)) {
		if (V4L2_TYPE_IS_OUTPUT(f->type)) {
			pix_mp->num_planes = 1;
			pix_mp->plane_fmt[0].bytesperline = 0;

			if ((pix_mp->pixelformat != V4L2_PIX_FMT_MPEG2) &&
			    (pix_mp->pixelformat != V4L2_PIX_FMT_H264) &&
			    (pix_mp->pixelformat != V4L2_PIX_FMT_MPEG1)) {
				pix_mp->field = V4L2_FIELD_NONE;
			} else if (pix_mp->field != V4L2_FIELD_NONE) {
				if (pix_mp->field == V4L2_FIELD_ANY)
					pix_mp->field = V4L2_FIELD_NONE;

				pr_info("%s, field: %u, fmt: %x\n",
					__func__, pix_mp->field,
					pix_mp->pixelformat);
			}
		} else {
			if (pix_mp->field != V4L2_FIELD_INTERLACED)
				pix_mp->field = V4L2_FIELD_NONE;
			pix_mp->height = clamp(pix_mp->height,
						AML_VDEC_MIN_H,
						AML_VDEC_MAX_H);
			pix_mp->width = clamp(pix_mp->width,
						AML_VDEC_MIN_W,
						AML_VDEC_MAX_W);

			pix_mp->num_planes = fmt->num_planes;

			pix_mp->plane_fmt[0].bytesperline = pix_mp->width;
			pix_mp->plane_fmt[0].sizeimage =
				pix_mp->width * pix_mp->height;

			pix_mp->plane_fmt[1].bytesperline = pix_mp->width;
			pix_mp->plane_fmt[1].sizeimage =
				pix_mp->width * pix_mp->height / 2;
		}

		for (i = 0; i < pix_mp->num_planes; i++) {
			memset(&(pix_mp->plane_fmt[i].reserved[0]), 0x0,
				   sizeof(pix_mp->plane_fmt[0].reserved));
		}
		memset(&pix_mp->reserved, 0x0, sizeof(pix_mp->reserved));

		pix_mp->flags = 0;
	} else {
		if (V4L2_TYPE_IS_OUTPUT(f->type)) {
			pix->bytesperline = 0;
			if ((pix->pixelformat != V4L2_PIX_FMT_MPEG2) &&
			    (pix->pixelformat != V4L2_PIX_FMT_H264) &&
			    (pix->pixelformat != V4L2_PIX_FMT_MPEG1)) {
				pix->field = V4L2_FIELD_NONE;
			} else if (pix->field != V4L2_FIELD_NONE) {
				if (pix->field == V4L2_FIELD_ANY)
					pix->field = V4L2_FIELD_NONE;

				pr_info("%s, field: %u, fmt: %x\n",
					__func__, pix->field,
					pix->pixelformat);
			}
		} else {
			if (pix->field != V4L2_FIELD_INTERLACED)
				pix->field = V4L2_FIELD_NONE;

			pix->height = clamp(pix->height,
						AML_VDEC_MIN_H,
						AML_VDEC_MAX_H);
			pix->width = clamp(pix->width,
						AML_VDEC_MIN_W,
						AML_VDEC_MAX_W);

			pix->bytesperline = pix->width;
			pix->sizeimage = pix->width * pix->height;
		}
		pix->flags = 0;
	}

	return 0;
}

static int vidioc_try_fmt_vid_cap_out(struct file *file, void *priv,
				struct v4l2_format *f)
{
	struct v4l2_pix_format_mplane *pix_mp = &f->fmt.pix_mp;
	struct v4l2_pix_format *pix = &f->fmt.pix;
	struct aml_q_data *q_data = NULL;
	struct aml_video_fmt *fmt = NULL;
	struct aml_vcodec_ctx *ctx = fh_to_ctx(priv);
	struct vb2_queue *dst_vq;

	v4l_dbg(ctx, V4L_DEBUG_CODEC_PROT,
		"%s, type: %u, planes: %u, fmt: %x\n",
		__func__, f->type,
		V4L2_TYPE_IS_MULTIPLANAR(f->type) ?
		f->fmt.pix_mp.num_planes : 1,
		f->fmt.pix_mp.pixelformat);

	dst_vq = v4l2_m2m_get_vq(ctx->m2m_ctx, V4L2_BUF_TYPE_VIDEO_CAPTURE);
	if (!dst_vq) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"no vb2 queue for type=%d\n", V4L2_BUF_TYPE_VIDEO_CAPTURE);
		return -EINVAL;
	}

	if (!V4L2_TYPE_IS_MULTIPLANAR(f->type) && dst_vq->is_multiplanar)
		return -EINVAL;

	fmt = aml_vdec_find_format(f);
	if (!fmt) {
		if (V4L2_TYPE_IS_OUTPUT(f->type))
			f->fmt.pix.pixelformat = aml_video_formats[OUT_FMT_IDX].fourcc;
		else
			f->fmt.pix.pixelformat = aml_video_formats[CAP_FMT_IDX].fourcc;
		fmt = aml_vdec_find_format(f);
	}

	vidioc_try_fmt(f, fmt);

	q_data = aml_vdec_get_q_data(ctx, f->type);
	if (!q_data)
		return -EINVAL;

	if (ctx->state >= AML_STATE_PROBE)
		update_ctx_dimension(ctx, f->type);
	copy_v4l2_format_dimension(ctx, pix_mp, pix, q_data, f->type);

	if (!V4L2_TYPE_IS_OUTPUT(f->type))
		return 0;

	if (V4L2_TYPE_IS_MULTIPLANAR(f->type)) {
		if (pix_mp->plane_fmt[0].sizeimage == 0) {
			v4l_dbg(0, V4L_DEBUG_CODEC_ERROR,
				"sizeimage of output format must be given\n");
			return -EINVAL;
		}
	} else {
		if (pix->sizeimage == 0) {
			v4l_dbg(0, V4L_DEBUG_CODEC_ERROR,
				"sizeimage of output format must be given\n");
			return -EINVAL;
		}
	}

	return 0;
}

static int vidioc_vdec_g_selection(struct file *file, void *priv,
	struct v4l2_selection *s)
{
	struct aml_vcodec_ctx *ctx = fh_to_ctx(priv);
	struct aml_q_data *q_data;
	int ratio = 1;

	if ((s->type != V4L2_BUF_TYPE_VIDEO_CAPTURE) &&
		(s->type != V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE))
		return -EINVAL;

	if (ctx->internal_dw_scale) {
		if (ctx->state >= AML_STATE_PROBE) {
			u32 dw_mode = DM_YUV_ONLY;
			u32 tw_mode = DM_INVALID;

			if (vdec_if_get_param(ctx, GET_PARAM_DW_MODE, &dw_mode))
				return -EBUSY;

			if (vdec_if_get_param(ctx, GET_PARAM_TW_MODE, &tw_mode))
				return -EBUSY;

			if ((dw_mode == DM_AVBC_ONLY) && tw_mode)
				ratio = vdec_get_size_ratio(tw_mode);
			else
				ratio = vdec_get_size_ratio(dw_mode);
		}
	}

	q_data = &ctx->q_data[AML_Q_DATA_DST];

	switch (s->target) {
	case V4L2_SEL_TGT_COMPOSE_DEFAULT:
	case V4L2_SEL_TGT_COMPOSE:
		s->r.left = 0;
		s->r.top = 0;
		s->r.width = ctx->picinfo.visible_width / ratio;
		s->r.height = ctx->picinfo.visible_height / ratio;
		break;
	case V4L2_SEL_TGT_COMPOSE_BOUNDS:
		s->r.left = 0;
		s->r.top = 0;
		s->r.width = ctx->picinfo.coded_width / ratio;
		s->r.height = ctx->picinfo.coded_height / ratio;
		break;
	case V4L2_SEL_TGT_CROP_DEFAULT:
		s->r.left = 0;
		s->r.top = 0;
		s->r.width = ctx->picinfo.visible_width;
		s->r.height = ctx->picinfo.visible_height;
		break;
	case V4L2_SEL_TGT_CROP_BOUNDS:
		s->r.left = 0;
		s->r.top = 0;
		s->r.width = ctx->picinfo.coded_width;
		s->r.height = ctx->picinfo.coded_height;
		break;
	default:
		return -EINVAL;
	}

	if (ctx->state < AML_STATE_PROBE) {
		/* set to default value if header info not ready yet*/
		s->r.left = 0;
		s->r.top = 0;
		s->r.width = q_data->visible_width;
		s->r.height = q_data->visible_height;
	}

	v4l_dbg(ctx, V4L_DEBUG_CODEC_PROT, "%s, type: %d\n",
		__func__, s->type);

	return 0;
}

static int vidioc_vdec_s_selection(struct file *file, void *priv,
	struct v4l2_selection *s)
{
	struct aml_vcodec_ctx *ctx = fh_to_ctx(priv);
	int ratio = 1;

	v4l_dbg(ctx, V4L_DEBUG_CODEC_PROT, "%s, type: %d\n",
		__func__, s->type);

	if (s->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	if (ctx->internal_dw_scale) {
		if (ctx->state >= AML_STATE_PROBE) {
			u32 dw_mode = DM_YUV_ONLY;
			u32 tw_mode = DM_INVALID;

			if (vdec_if_get_param(ctx, GET_PARAM_DW_MODE, &dw_mode))
				return -EBUSY;

			if (vdec_if_get_param(ctx, GET_PARAM_TW_MODE, &tw_mode))
				return -EBUSY;

			if ((dw_mode == DM_AVBC_ONLY) && tw_mode)
				ratio = vdec_get_size_ratio(tw_mode);
			else
				ratio = vdec_get_size_ratio(dw_mode);
		}
	}

	switch (s->target) {
	case V4L2_SEL_TGT_COMPOSE:
		s->r.left = 0;
		s->r.top = 0;
		s->r.width = ctx->picinfo.visible_width / ratio;
		s->r.height = ctx->picinfo.visible_height / ratio;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int is_vdec_core_fmt(struct aml_vcodec_ctx *ctx)
{
	if (ctx->output_pix_fmt == V4L2_PIX_FMT_H264 ||
		ctx->output_pix_fmt == V4L2_PIX_FMT_MPEG ||
		ctx->output_pix_fmt == V4L2_PIX_FMT_MPEG1 ||
		ctx->output_pix_fmt == V4L2_PIX_FMT_MPEG2 ||
		ctx->output_pix_fmt == V4L2_PIX_FMT_MPEG4 ||
		ctx->output_pix_fmt == V4L2_PIX_FMT_AVS ||
		ctx->output_pix_fmt == V4L2_PIX_FMT_MJPEG ||
		ctx->output_pix_fmt == V4L2_PIX_FMT_VC1_ANNEX_G ||
		ctx->output_pix_fmt == V4L2_PIX_FMT_VC1_ANNEX_L)
		return true;

	return false;
}

/* called when it is beyong AML_STATE_PROBE */
static void update_ctx_dimension(struct aml_vcodec_ctx *ctx, u32 type)
{
	struct aml_q_data *q_data;
	unsigned int dw_mode = DM_YUV_ONLY;
	unsigned int tw_mode = DM_INVALID;
	int ratio = 1;

	q_data = aml_vdec_get_q_data(ctx, type);

	if (vdec_if_get_param(ctx, GET_PARAM_DW_MODE, &dw_mode))
		return;

	if (ctx->internal_dw_scale)
		ratio = vdec_get_size_ratio(dw_mode);

	if (V4L2_TYPE_IS_MULTIPLANAR(type)) {
		q_data->sizeimage[0] = ctx->picinfo.y_len_sz;
		q_data->sizeimage[1] = ctx->picinfo.c_len_sz;

		q_data->coded_width = ALIGN(ctx->picinfo.coded_width / ratio, 64);
		q_data->coded_height = ALIGN(ctx->picinfo.coded_height / ratio, 64);

		q_data->bytesperline[0] = ALIGN(ctx->picinfo.coded_width / ratio, 64);
		q_data->bytesperline[1] = ALIGN(ctx->picinfo.coded_width / ratio, 64);

		q_data->bytesperline[0] = q_data->bytesperline[0] << is_output_p010(dw_mode);
		q_data->bytesperline[1] = q_data->bytesperline[1] << is_output_p010(dw_mode);
	} else {
		q_data->coded_width = ALIGN(ctx->picinfo.coded_width / ratio, 64);
		q_data->coded_height = ALIGN(ctx->picinfo.coded_height / ratio, 64);
		q_data->sizeimage[0] = ctx->picinfo.y_len_sz;
		q_data->sizeimage[0] += ctx->picinfo.c_len_sz;
		q_data->bytesperline[0] = ALIGN(ctx->picinfo.coded_width / ratio, 64);
		q_data->bytesperline[0] = q_data->bytesperline[0] << is_output_p010(dw_mode);
	}

	/* For triple write. */
	if (vdec_if_get_param(ctx, GET_PARAM_TW_MODE, &tw_mode))
		return;

	if (tw_mode == DM_INVALID)
		return;

	if (ctx->internal_dw_scale)
		ratio = vdec_get_size_ratio(tw_mode);

	if (V4L2_TYPE_IS_MULTIPLANAR(type)) {
		q_data->sizeimage_tw[0] = ctx->picinfo.y_len_sz_tw;
		q_data->sizeimage_tw[1] = ctx->picinfo.c_len_sz_tw;

		q_data->coded_width = ALIGN(ctx->picinfo.coded_width / ratio, 64);
		q_data->coded_height = ALIGN(ctx->picinfo.coded_height / ratio, 64);

		q_data->bytesperline_tw[0] = ALIGN(ctx->picinfo.coded_width / ratio, 64);
		q_data->bytesperline_tw[1] = ALIGN(ctx->picinfo.coded_width / ratio, 64);

		q_data->bytesperline_tw[0] = q_data->bytesperline_tw[0] << is_output_p010(tw_mode);
		q_data->bytesperline_tw[1] = q_data->bytesperline_tw[1] << is_output_p010(tw_mode);
	} else {
		q_data->coded_width = ALIGN(ctx->picinfo.coded_width / ratio, 64);
		q_data->coded_height = ALIGN(ctx->picinfo.coded_height / ratio, 64);
		q_data->sizeimage_tw[0] = ctx->picinfo.y_len_sz_tw;
		q_data->sizeimage_tw[0] += ctx->picinfo.c_len_sz_tw;
		q_data->bytesperline_tw[0] = ALIGN(ctx->picinfo.coded_width / ratio, 64);
		q_data->bytesperline_tw[0] = q_data->bytesperline_tw[0] << is_output_p010(tw_mode);
	}
}

static void copy_v4l2_format_dimension(struct aml_vcodec_ctx *ctx,
				       struct v4l2_pix_format_mplane *pix_mp,
				       struct v4l2_pix_format *pix,
				       struct aml_q_data *q_data,
				       u32 type)
{
	u32 dw_mode = DM_YUV_ONLY;
	u32 tw_mode = DM_INVALID;
	int i;

	if (!pix || !pix_mp || !q_data)
		return;

	if (vdec_if_get_param(ctx, GET_PARAM_DW_MODE, &dw_mode))
		return;

	if (vdec_if_get_param(ctx, GET_PARAM_TW_MODE, &tw_mode))
		return;

	if (V4L2_TYPE_IS_MULTIPLANAR(type)) {
		pix_mp->width		= q_data->coded_width;
		pix_mp->height		= q_data->coded_height;
		pix_mp->num_planes	= q_data->fmt->num_planes;
		pix_mp->pixelformat	= q_data->fmt->fourcc;

		for (i = 0; i < q_data->fmt->num_planes; i++) {
			pix_mp->plane_fmt[i].bytesperline = q_data->bytesperline[i];
			pix_mp->plane_fmt[i].sizeimage = q_data->sizeimage[i];

			if ((dw_mode == DM_AVBC_ONLY) && tw_mode) {
				pix_mp->plane_fmt[i].bytesperline = q_data->bytesperline_tw[i];
				pix_mp->plane_fmt[i].sizeimage = q_data->sizeimage_tw[i];
			}
		}
	} else {
		pix->width		= q_data->coded_width;
		pix->height		= q_data->coded_height;
		pix->pixelformat	= q_data->fmt->fourcc;
		pix->bytesperline	= q_data->bytesperline[0];
		pix->sizeimage		= q_data->sizeimage[0];

		if ((dw_mode == DM_AVBC_ONLY) && tw_mode) {
			pix->bytesperline	= q_data->bytesperline_tw[0];
			pix->sizeimage		= q_data->sizeimage_tw[0];
		}
	}
}

static int vidioc_vdec_s_fmt(struct file *file, void *priv,
	struct v4l2_format *f)
{
	int ret = 0;
	struct aml_vcodec_ctx *ctx = fh_to_ctx(priv);
	struct v4l2_pix_format_mplane *pix_mp = &f->fmt.pix_mp;
	struct v4l2_pix_format *pix = &f->fmt.pix;
	struct aml_q_data *q_data = NULL;
	struct aml_video_fmt *fmt;
	struct vb2_queue *dst_vq;

	v4l_dbg(ctx, V4L_DEBUG_CODEC_PROT,
		"%s, type: %u, planes: %u, fmt: %x\n",
		__func__, f->type,
		V4L2_TYPE_IS_MULTIPLANAR(f->type) ?
		f->fmt.pix_mp.num_planes : 1,
		f->fmt.pix_mp.pixelformat);

	dst_vq = v4l2_m2m_get_vq(ctx->m2m_ctx, V4L2_BUF_TYPE_VIDEO_CAPTURE);
	if (!dst_vq) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"no vb2 queue for type=%d\n", V4L2_BUF_TYPE_VIDEO_CAPTURE);
		return -EINVAL;
	}

	if (!V4L2_TYPE_IS_MULTIPLANAR(f->type) && dst_vq->is_multiplanar)
		return -EINVAL;

	q_data = aml_vdec_get_q_data(ctx, f->type);
	if (!q_data)
		return -EINVAL;

	if ((f->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) &&
	    vb2_is_busy(&ctx->m2m_ctx->out_q_ctx.q)) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"out_q_ctx buffers already requested\n");
		return -EBUSY;
	}

	if ((!V4L2_TYPE_IS_OUTPUT(f->type)) &&
	    vb2_is_busy(&ctx->m2m_ctx->cap_q_ctx.q)) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"cap_q_ctx buffers already requested\n");
		return -EBUSY;
	}

	fmt = aml_vdec_find_format(f);
	if (fmt == NULL) {
		if (V4L2_TYPE_IS_OUTPUT(f->type))
			fmt = &aml_video_formats[OUT_FMT_IDX];
		else
			fmt = &aml_video_formats[CAP_FMT_IDX];
		f->fmt.pix.pixelformat = fmt->fourcc;
	}

	q_data->fmt = fmt;
	vidioc_try_fmt(f, q_data->fmt);

	if (V4L2_TYPE_IS_OUTPUT(f->type) && ctx->drv_handle && ctx->receive_cmd_stop) {
		ctx->state = AML_STATE_IDLE;
		vdec_tracing(&ctx->vtr, VTRACE_V4L_ST_0, ctx->state);
		v4l_dbg(ctx, V4L_DEBUG_CODEC_STATE,
			"vcodec state (AML_STATE_IDLE)\n");
		vdec_if_deinit(ctx);
		ctx->receive_cmd_stop = 0;
	}

	if (f->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		q_data->sizeimage[0] = pix_mp->plane_fmt[0].sizeimage;
		q_data->coded_width = pix_mp->width;
		q_data->coded_height = pix_mp->height;

		v4l_dbg(ctx, V4L_DEBUG_CODEC_EXINFO,
			"w: %d, h: %d, size: %d\n",
			pix_mp->width, pix_mp->height,
			pix_mp->plane_fmt[0].sizeimage);

		ctx->output_pix_fmt = pix_mp->pixelformat;
		ctx->colorspace = f->fmt.pix_mp.colorspace;
		ctx->ycbcr_enc = f->fmt.pix_mp.ycbcr_enc;
		ctx->quantization = f->fmt.pix_mp.quantization;
		ctx->xfer_func = f->fmt.pix_mp.xfer_func;

		mutex_lock(&ctx->state_lock);
		if (ctx->state == AML_STATE_IDLE) {
			ret = vdec_if_init(ctx, q_data->fmt->fourcc);
			if (ret) {
				v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
					"vdec_if_init() fail ret=%d\n", ret);
				mutex_unlock(&ctx->state_lock);
				return -EINVAL;
			}

			vdec_trace_init(&ctx->vtr, ctx->id, vdec_get_vdec_id(ctx->ada_ctx));

			ctx->state = AML_STATE_INIT;
			vdec_tracing(&ctx->vtr, VTRACE_V4L_ST_0, ctx->state);
			v4l_dbg(ctx, V4L_DEBUG_CODEC_STATE,
				"vcodec state (AML_STATE_INIT)\n");
		}
		mutex_unlock(&ctx->state_lock);
	} else if (f->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		q_data->sizeimage[0] = pix->sizeimage;
		q_data->coded_width = pix->width;
		q_data->coded_height = pix->height;

		if (!vdec_check_is_available(pix->pixelformat)) {
			v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
				"VC-1 only support single mode. \n");
			return -EINVAL;
		}

		v4l_dbg(ctx, V4L_DEBUG_CODEC_EXINFO,
			"w: %d, h: %d, size: %d\n",
			pix->width, pix->height,
			pix->sizeimage);

		ctx->output_pix_fmt = pix->pixelformat;
		ctx->colorspace = f->fmt.pix.colorspace;
		ctx->ycbcr_enc = f->fmt.pix.ycbcr_enc;
		ctx->quantization = f->fmt.pix.quantization;
		ctx->xfer_func = f->fmt.pix.xfer_func;

		mutex_lock(&ctx->state_lock);
		if (ctx->state == AML_STATE_IDLE) {
			ret = vdec_if_init(ctx, q_data->fmt->fourcc);
			if (ret) {
				v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
					"vdec_if_init() fail ret=%d\n", ret);
				mutex_unlock(&ctx->state_lock);
				return -EINVAL;
			}

			vdec_trace_init(&ctx->vtr, ctx->id, vdec_get_vdec_id(ctx->ada_ctx));

			ctx->state = AML_STATE_INIT;
			vdec_tracing(&ctx->vtr, VTRACE_V4L_ST_0, ctx->state);
			v4l_dbg(ctx, V4L_DEBUG_CODEC_STATE,
				"vcodec state (AML_STATE_INIT)\n");
		}
		mutex_unlock(&ctx->state_lock);
	}

	if (!V4L2_TYPE_IS_OUTPUT(f->type)) {
		ctx->cap_pix_fmt = V4L2_TYPE_IS_MULTIPLANAR(f->type) ?
			pix_mp->pixelformat : pix->pixelformat;
		if (ctx->state >= AML_STATE_PROBE) {
			update_ctx_dimension(ctx, f->type);
			copy_v4l2_format_dimension(ctx, pix_mp, pix, q_data, f->type);
		}
	}

	return 0;
}

static int vidioc_enum_framesizes(struct file *file, void *priv,
				struct v4l2_frmsizeenum *fsize)
{
	int i = 0;
	struct aml_vcodec_ctx *ctx = fh_to_ctx(priv);

	v4l_dbg(ctx, V4L_DEBUG_CODEC_PROT, "%s, idx: %d, pix fmt: %x\n",
		__func__, fsize->index, fsize->pixel_format);

	if (fsize->index != 0)
		return -EINVAL;

	for (i = 0; i < NUM_SUPPORTED_FRAMESIZE; ++i) {
		if (fsize->pixel_format != aml_vdec_framesizes[i].fourcc)
			continue;

		fsize->type = V4L2_FRMSIZE_TYPE_STEPWISE;
		fsize->stepwise = aml_vdec_framesizes[i].stepwise;
		if (!(ctx->dev->dec_capability &
				VCODEC_CAPABILITY_4K_DISABLED)) {
			v4l_dbg(ctx, V4L_DEBUG_CODEC_EXINFO, "4K is enabled\n");
			fsize->stepwise.max_width =
					VCODEC_DEC_4K_CODED_WIDTH;
			fsize->stepwise.max_height =
					VCODEC_DEC_4K_CODED_HEIGHT;
		}
		v4l_dbg(ctx, V4L_DEBUG_CODEC_EXINFO,
			"%x, %d %d %d %d %d %d\n",
			ctx->dev->dec_capability,
			fsize->stepwise.min_width,
			fsize->stepwise.max_width,
			fsize->stepwise.step_width,
			fsize->stepwise.min_height,
			fsize->stepwise.max_height,
			fsize->stepwise.step_height);
		return 0;
	}

	return -EINVAL;
}

static int vidioc_enum_fmt(struct v4l2_fmtdesc *f, bool output_queue)
{
	struct aml_video_fmt *fmt;
	int i = 0, j = 0;

	/* I420 only used for mjpeg. */
	if (!output_queue && support_mjpeg && support_format_I420) {
		for (i = 0; i < NUM_FORMATS; i++) {
			fmt = &aml_video_formats[i];
			if ((fmt->fourcc == V4L2_PIX_FMT_YUV420) ||
				(fmt->fourcc == V4L2_PIX_FMT_YUV420M)) {
				break;
			}
		}
	}

	for (; i < NUM_FORMATS; i++) {
		fmt = &aml_video_formats[i];
		if (output_queue && (fmt->type != AML_FMT_DEC))
			continue;
		if (!output_queue && (fmt->type != AML_FMT_FRAME))
			continue;
		if (support_mjpeg && !support_format_I420 &&
			((fmt->fourcc == V4L2_PIX_FMT_YUV420) ||
			(fmt->fourcc == V4L2_PIX_FMT_YUV420M)))
			continue;

		if (j == f->index) {
			f->pixelformat = fmt->fourcc;
			strncpy(f->description, fmt->name, sizeof(f->description));
			return 0;
		}
		++j;
	}

	return -EINVAL;
}

static int vidioc_vdec_enum_fmt_vid_cap_mplane(struct file *file,
	void *priv, struct v4l2_fmtdesc *f)
{
	struct aml_vcodec_ctx *ctx = fh_to_ctx(priv);

	v4l_dbg(ctx, V4L_DEBUG_CODEC_PROT, "%s\n", __func__);

	return vidioc_enum_fmt(f, false);
}

static int vidioc_vdec_enum_fmt_vid_out_mplane(struct file *file,
	void *priv, struct v4l2_fmtdesc *f)
{
	struct aml_vcodec_ctx *ctx = fh_to_ctx(priv);

	v4l_dbg(ctx, V4L_DEBUG_CODEC_PROT, "%s\n", __func__);

	return vidioc_enum_fmt(f, true);
}

static int vidioc_vdec_g_fmt(struct file *file, void *priv,
	struct v4l2_format *f)
{
	struct aml_vcodec_ctx *ctx = fh_to_ctx(priv);
	struct v4l2_pix_format_mplane *pix_mp = &f->fmt.pix_mp;
	struct v4l2_pix_format *pix = &f->fmt.pix;
	struct vb2_queue *vq;
	struct vb2_queue *dst_vq;
	struct aml_q_data *q_data;
	int ret = 0;

	vq = v4l2_m2m_get_vq(ctx->m2m_ctx, f->type);
	if (!vq) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"no vb2 queue for type=%d\n", f->type);
		return -EINVAL;
	}

	dst_vq = v4l2_m2m_get_vq(ctx->m2m_ctx, V4L2_BUF_TYPE_VIDEO_CAPTURE);
	if (!dst_vq) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"no vb2 queue for type=%d\n", V4L2_BUF_TYPE_VIDEO_CAPTURE);
		return -EINVAL;
	}

	if (!V4L2_TYPE_IS_MULTIPLANAR(f->type) && dst_vq->is_multiplanar)
		return -EINVAL;

	q_data = aml_vdec_get_q_data(ctx, f->type);

	ret = vdec_if_get_param(ctx, GET_PARAM_PIC_INFO, &ctx->picinfo);
	if (ret) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"GET_PARAM_PICTURE_INFO err\n");
	} else {
		if ((ctx->picinfo.visible_height < 16 && ctx->picinfo.visible_height > 0) ||
			(ctx->picinfo.visible_width < 16 && ctx->picinfo.visible_width > 0)) {
			v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
				"The width or height of the stream is less than 16\n");
			return -EPERM;
		}
	}

	if (V4L2_TYPE_IS_MULTIPLANAR(f->type)) {
		pix_mp->field = ctx->force_report_interlace ? V4L2_FIELD_INTERLACED :
			(ret ? V4L2_FIELD_NONE : ctx->picinfo.field);
		pix_mp->colorspace = ctx->colorspace;
		pix_mp->ycbcr_enc = ctx->ycbcr_enc;
		pix_mp->quantization = ctx->quantization;
		pix_mp->xfer_func = ctx->xfer_func;
	} else {
		pix->field = ctx->force_report_interlace ? V4L2_FIELD_INTERLACED :
			(ret ? V4L2_FIELD_NONE : ctx->picinfo.field);
		pix->colorspace = ctx->colorspace;
		pix->ycbcr_enc = ctx->ycbcr_enc;
		pix->quantization = ctx->quantization;
		pix->xfer_func = ctx->xfer_func;
	}

	if ((!V4L2_TYPE_IS_OUTPUT(f->type)) &&
	    (ctx->state >= AML_STATE_PROBE)) {
		update_ctx_dimension(ctx, f->type);
		copy_v4l2_format_dimension(ctx, pix_mp, pix, q_data, f->type);
	} else if (f->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		/*
		 * This is run on OUTPUT
		 * The buffer contains compressed image
		 * so width and height have no meaning.
		 * Assign value here to pass v4l2-compliance test
		 */
		copy_v4l2_format_dimension(ctx, pix_mp, pix, q_data, f->type);
	} else {
		copy_v4l2_format_dimension(ctx, pix_mp, pix, q_data, f->type);

		v4l_dbg(ctx, V4L_DEBUG_CODEC_EXINFO,
			"type=%d state=%d Format information could not be read, not ready yet!\n",
			f->type, ctx->state);
	}

	v4l_dbg(ctx, V4L_DEBUG_CODEC_PROT,
		"%s, type: %u, planes: %u, fmt: %x\n",
		__func__, f->type,
		V4L2_TYPE_IS_MULTIPLANAR(f->type) ?
		q_data->fmt->num_planes : 1,
		q_data->fmt->fourcc);

	return 0;
}

static int vidioc_vdec_create_bufs(struct file *file, void *priv,
	struct v4l2_create_buffers *create)
{
	struct aml_vcodec_ctx *ctx = fh_to_ctx(priv);

	v4l_dbg(ctx, V4L_DEBUG_CODEC_PROT,
		"%s, type: %u, count: %u\n",
		__func__, create->format.type, create->count);

	return v4l2_m2m_ioctl_create_bufs(file, priv, create);
}

/*int vidioc_vdec_g_ctrl(struct file *file, void *fh,
	struct v4l2_control *a)
{
	struct aml_vcodec_ctx *ctx = fh_to_ctx(fh);

	v4l_dbg(ctx, V4L_DEBUG_CODEC_PROT,
		"%s, id: %d\n", __func__, a->id);

	if (a->id == V4L2_CID_MIN_BUFFERS_FOR_CAPTURE)
		a->value = 4;
	else if (a->id == V4L2_CID_MIN_BUFFERS_FOR_OUTPUT)
		a->value = 8;

	return 0;
}*/

static int vb2ops_vdec_queue_setup(struct vb2_queue *vq,
				unsigned int *nbuffers,
				unsigned int *nplanes,
				unsigned int sizes[], struct device *alloc_devs[])
{
	struct aml_vcodec_ctx *ctx = vb2_get_drv_priv(vq);
	struct aml_q_data *q_data;
	unsigned int i;

	v4l_dbg(ctx, V4L_DEBUG_CODEC_PROT, "%s, type: %d\n",
		__func__, vq->type);

	q_data = aml_vdec_get_q_data(ctx, vq->type);
	if (q_data == NULL) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"vq->type=%d err\n", vq->type);
		return -EINVAL;
	}

	if (*nplanes) {
		for (i = 0; i < *nplanes; i++) {
			if (sizes[i] < q_data->sizeimage[i])
				return -EINVAL;
			alloc_devs[i] = &ctx->dev->plat_dev->dev;

			if (!V4L2_TYPE_IS_OUTPUT(vq->type) &&
				(vq->memory == VB2_MEMORY_MMAP))
				alloc_devs[i] = v4l_get_dev_from_codec_mm();
			else if (vq->memory == VB2_MEMORY_MMAP)
				dma_coerce_mask_and_coherent(alloc_devs[i], DMA_BIT_MASK(64));
		}
	} else {
		int dw_mode = DM_YUV_ONLY;
		int tw_mode = DM_INVALID;

		if (vq->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
			*nplanes = 2;
		else
			*nplanes = 1;

		if (vdec_if_get_param(ctx, GET_PARAM_DW_MODE, &dw_mode))
			return -EINVAL;

		if (vdec_if_get_param(ctx, GET_PARAM_TW_MODE, &tw_mode))
			return -EINVAL;

		if ((dw_mode == DM_AVBC_ONLY) && (tw_mode == DM_INVALID))
			*nplanes = 1;

		for (i = 0; i < *nplanes; i++) {
			alloc_devs[i] = &ctx->dev->plat_dev->dev;
			sizes[i] = (dw_mode != DM_AVBC_ONLY) ? q_data->sizeimage[i] :
				(tw_mode != DM_INVALID) ? q_data->sizeimage_tw[i] :
				PAGE_SIZE;

			if (ctx->enable_di_post && is_vdec_core_fmt(ctx) &&
				ctx->picinfo.field != V4L2_FIELD_NONE)
				sizes[i] = PAGE_SIZE;

			if (V4L2_TYPE_IS_OUTPUT(vq->type)) {
				if (ctx->output_dma_mode)
					sizes[i] = 1;
				else
					sizes[i] = q_data->sizeimage[i];

				if (vq->memory == VB2_MEMORY_MMAP)
					dma_coerce_mask_and_coherent(alloc_devs[i], DMA_BIT_MASK(64));
			} else
				alloc_devs[i] = v4l_get_dev_from_codec_mm();
		}
	}

	v4l_dbg(ctx, V4L_DEBUG_CODEC_BUFMGR,
		"type: %d, plane: %d, buf cnt: %d, size: [Y: %u, C: %u]\n",
		vq->type, *nplanes, *nbuffers, sizes[0], sizes[1]);

	return 0;
}

static int vb2ops_vdec_buf_prepare(struct vb2_buffer *vb)
{
	struct aml_vcodec_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct aml_q_data *q_data;
	struct vb2_v4l2_buffer *vb2_v4l2 = NULL;
	struct aml_v4l2_buf *buf = NULL;
	int i;
	v4l_dbg(ctx, V4L_DEBUG_CODEC_PROT,
		"%s, type: %d, idx: %d\n",
		__func__, vb->vb2_queue->type, vb->index);

	vb2_v4l2 = to_vb2_v4l2_buffer(vb);
	buf = container_of(vb2_v4l2, struct aml_v4l2_buf, vb);

	if (vb->memory == VB2_MEMORY_DMABUF && V4L2_TYPE_IS_OUTPUT(vb->vb2_queue->type)) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
		if (!ctx->is_drm_mode) {
			struct dmabuf_videodec_es_data *videodec_es_data;
			if (dmabuf_manage_get_type(vb->planes[0].dbuf) != DMA_BUF_TYPE_VIDEODEC_ES) {
				return 0;
			}
			videodec_es_data = (struct dmabuf_videodec_es_data *)dmabuf_manage_get_info(vb->planes[0].dbuf,
					DMA_BUF_TYPE_VIDEODEC_ES);

			if (videodec_es_data && videodec_es_data->data_type == DMA_BUF_VIDEODEC_HDR10PLUS) {
				v4l_dbg(ctx, V4L_DEBUG_CODEC_PROT, "buf_prepare hdr10+ info type %d, len: %d\n",
					videodec_es_data->data_type, videodec_es_data->data_len);
				memcpy(buf->meta_data, (void *)videodec_es_data->data, videodec_es_data->data_len);
			}
		}
#endif
		return 0;
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
	if (vb2_v4l2->meta_ptr && (copy_from_user(buf->meta_data,
		(void *)vb2_v4l2->meta_ptr, VDEC_META_DATA_SIZE + 4))) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"%s:copy meta data error. ptr: %lx\n", __func__, vb2_v4l2->meta_ptr);
	}
#endif

	q_data = aml_vdec_get_q_data(ctx, vb->vb2_queue->type);

	for (i = 0; i < q_data->fmt->num_planes; i++) {
		if (vb2_plane_size(vb, i) < q_data->sizeimage[i] &&
			vb2_plane_size(vb, i) != PAGE_SIZE) {
			v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
				"data will not fit into plane %d (%lu < %d)\n",
				i, vb2_plane_size(vb, i),
				q_data->sizeimage[i]);
		}
	}

	return 0;
}

void aml_alloc_buffer(struct aml_vcodec_ctx *ctx, int flag)
{
	int i = 0;

	if (flag & DV_TYPE) {
		for (i = 0; i < V4L_CAP_BUFF_MAX; i++) {
			ctx->aux_infos.bufs[i].md_buf = vzalloc(MD_BUF_SIZE);
			if (ctx->aux_infos.bufs[i].md_buf == NULL) {
				v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
					"v4l2 alloc %dth dv md buffer fail\n", i);
			}

			ctx->aux_infos.bufs[i].comp_buf = vzalloc(COMP_BUF_SIZE);
			if (ctx->aux_infos.bufs[i].comp_buf == NULL) {
				v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
					"v4l2 alloc %dth dv comp buffer fail\n", i);
			}
		}
	}

	if (flag & SEI_TYPE) {
		for (i = 0; i < V4L_CAP_BUFF_MAX; i++) {
			ctx->aux_infos.bufs[i].sei_buf = vzalloc(SEI_BUF_SIZE);
			if (ctx->aux_infos.bufs[i].sei_buf) {
				ctx->aux_infos.bufs[i].sei_size  = 0;
				ctx->aux_infos.bufs[i].sei_state = 1;
				ctx->aux_infos.sei_need_free = false;
				v4l_dbg(ctx, V4L_DEBUG_CODEC_EXINFO,
					"v4l2 alloc %dth aux buffer:%px\n",
					i, ctx->aux_infos.bufs[i].sei_buf);
			} else {
				ctx->aux_infos.bufs[i].sei_buf = NULL;
				ctx->aux_infos.bufs[i].sei_state = 0;
				ctx->aux_infos.bufs[i].sei_size  = 0;
				v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
					"v4l2 alloc %dth aux buffer fail\n", i);
			}
		}
	}

	if (flag & HDR10P_TYPE) {
		for (i = 0; i < V4L_CAP_BUFF_MAX; i++) {
			ctx->aux_infos.bufs[i].hdr10p_buf = vzalloc(HDR10P_BUF_SIZE);
			if (ctx->aux_infos.bufs[i].hdr10p_buf) {
				v4l_dbg(ctx, V4L_DEBUG_CODEC_EXINFO,
					"v4l2 alloc %dth hdr10p buffer:%px\n",
					i, ctx->aux_infos.bufs[i].hdr10p_buf);
			} else {
				ctx->aux_infos.bufs[i].hdr10p_buf = NULL;
				v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
					"v4l2 alloc %dth hdr10p buffer fail\n", i);
			}
		}
	}
}

void aml_free_buffer(struct aml_vcodec_ctx *ctx, int flag)
{
	int i = 0;

	if (flag & DV_TYPE) {
		for (i = 0; i < V4L_CAP_BUFF_MAX; i++) {
			if (ctx->aux_infos.bufs[i].md_buf != NULL) {
				vfree(ctx->aux_infos.bufs[i].md_buf);
				ctx->aux_infos.bufs[i].md_buf = NULL;
			}

			if (ctx->aux_infos.bufs[i].comp_buf != NULL) {
				vfree(ctx->aux_infos.bufs[i].comp_buf);
				ctx->aux_infos.bufs[i].comp_buf = NULL;
			}
		}
	}

	if (flag & SEI_TYPE) {
		for (i = 0; i < V4L_CAP_BUFF_MAX; i++) {
			if (ctx->aux_infos.bufs[i].sei_buf != NULL) {
				v4l_dbg(ctx, V4L_DEBUG_CODEC_EXINFO,
					"v4l2 free %dth aux buffer:%px\n",
					i, ctx->aux_infos.bufs[i].sei_buf);
				vfree(ctx->aux_infos.bufs[i].sei_buf);
				ctx->aux_infos.bufs[i].sei_state = 0;
				ctx->aux_infos.bufs[i].sei_size = 0;
				ctx->aux_infos.bufs[i].sei_buf = NULL;
			}
		}
	}

	if (flag & HDR10P_TYPE) {
		for (i = 0; i < V4L_CAP_BUFF_MAX; i++) {
			if (ctx->aux_infos.bufs[i].hdr10p_buf != NULL) {
				v4l_dbg(ctx, V4L_DEBUG_CODEC_EXINFO,
					"v4l2 free %dth hdr10p buffer:%px\n",
					i, ctx->aux_infos.bufs[i].hdr10p_buf);
				vfree(ctx->aux_infos.bufs[i].hdr10p_buf);
				ctx->aux_infos.bufs[i].hdr10p_buf = NULL;
			}
		}
	}
}

void aml_free_one_sei_buffer(struct aml_vcodec_ctx *ctx, char **addr, int *size, int idx)
{
	if (ctx->aux_infos.bufs[idx].sei_buf != NULL) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_EXINFO,
			"v4l2 free %dth aux buffer:%px\n",
			idx, ctx->aux_infos.bufs[idx].sei_buf);

		vfree(ctx->aux_infos.bufs[idx].sei_buf);
		ctx->aux_infos.bufs[idx].sei_state = 0;
		ctx->aux_infos.bufs[idx].sei_size = 0;
		ctx->aux_infos.bufs[idx].sei_buf = NULL;
		*addr = NULL;
		*size = 0;
		ctx->aux_infos.sei_need_free = true;
	}
}

void aml_bind_sei_buffer(struct aml_vcodec_ctx *ctx, char **addr, int *size, int *idx)
{
	int index = ctx->aux_infos.sei_index;
	int count = 0;

	if (ctx->aux_infos.sei_need_free) {
		for (count = 0; count < V4L_CAP_BUFF_MAX; count++) {
			if ((ctx->aux_infos.bufs[index].sei_buf != NULL) &&
				(ctx->aux_infos.bufs[index].sei_state == 1)) {
				break;
			}
			index = (index + 1) % V4L_CAP_BUFF_MAX;
		}
	} else {
		for (count = 0; count < V4L_CAP_BUFF_MAX; count++) {
			if ((ctx->aux_infos.bufs[index].sei_buf != NULL) &&
				((ctx->aux_infos.bufs[index].sei_state == 1) ||
				(ctx->aux_infos.bufs[index].sei_state == 2))) {
				memset(ctx->aux_infos.bufs[index].sei_buf, 0, SEI_BUF_SIZE);
				ctx->aux_infos.bufs[index].sei_size = 0;
				break;
			}
			index = (index + 1) % V4L_CAP_BUFF_MAX;
		}
	}

	if (count == V4L_CAP_BUFF_MAX) {
		*addr = NULL;
		*size = 0;
	} else {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_EXINFO,
			"v4l2 bind %dth aux buffer:%px, count = %d\n",
			index, ctx->aux_infos.bufs[index].sei_buf, count);
		*addr = ctx->aux_infos.bufs[index].sei_buf;
		*size = ctx->aux_infos.bufs[index].sei_size;
		*idx  = index;
		ctx->aux_infos.bufs[index].sei_state = 2;
		ctx->aux_infos.sei_index = (index + 1) % V4L_CAP_BUFF_MAX;
	}
}

void aml_bind_dv_buffer(struct aml_vcodec_ctx *ctx, char **comp_buf, char **md_buf)
{
	int index = ctx->aux_infos.dv_index;

	if ((ctx->aux_infos.bufs[index].comp_buf != NULL) &&
		(ctx->aux_infos.bufs[index].md_buf != NULL)) {
		*comp_buf = ctx->aux_infos.bufs[index].comp_buf;
		*md_buf = ctx->aux_infos.bufs[index].md_buf;
		ctx->aux_infos.dv_index = (index + 1) % V4L_CAP_BUFF_MAX;
	}
}

void aml_bind_hdr10p_buffer(struct aml_vcodec_ctx *ctx, char **addr)
{
	int index = ctx->aux_infos.hdr10p_index;

	if (ctx->aux_infos.bufs[index].hdr10p_buf != NULL) {
		*addr = ctx->aux_infos.bufs[index].hdr10p_buf;
		ctx->aux_infos.hdr10p_index = (index + 1) % V4L_CAP_BUFF_MAX;
	}
}

static void aml_canvas_cache_free(struct canvas_cache *cache)
{
	int i = -1;

	for (i = 0; i < ARRAY_SIZE(cache->res); i++) {
		if (cache->res[i].cid > 0) {
			v4l_dbg(0, V4L_DEBUG_CODEC_BUFMGR,
				"canvas-free, name:%s, canvas id:%d\n",
				cache->res[i].name,
				cache->res[i].cid);

			canvas_pool_map_free_canvas(cache->res[i].cid);

			cache->res[i].cid = 0;
		}
	}
}

void aml_canvas_cache_put(struct aml_vcodec_dev *dev)
{
	struct canvas_cache *cache = &dev->cache;

	mutex_lock(&cache->lock);

	v4l_dbg(0, V4L_DEBUG_CODEC_BUFMGR,
		"canvas-put, ref:%d\n", cache->ref);

	cache->ref--;

	if (cache->ref == 0) {
		aml_canvas_cache_free(cache);
	}

	mutex_unlock(&cache->lock);
}

int aml_canvas_cache_get(struct aml_vcodec_dev *dev, char *usr)
{
	struct canvas_cache *cache = &dev->cache;
	int i;

	mutex_lock(&cache->lock);

	cache->ref++;

	for (i = 0; i < ARRAY_SIZE(cache->res); i++) {
		if (cache->res[i].cid <= 0) {
			snprintf(cache->res[i].name, 32, "%s-%d", usr, i);
			cache->res[i].cid =
				canvas_pool_map_alloc_canvas(cache->res[i].name);
		}

		v4l_dbg(0, V4L_DEBUG_CODEC_BUFMGR,
			"canvas-alloc, name:%s, canvas id:%d\n",
			cache->res[i].name,
			cache->res[i].cid);

		if (cache->res[i].cid <= 0) {
			v4l_dbg(0, V4L_DEBUG_CODEC_ERROR,
				"canvas-fail, name:%s, canvas id:%d.\n",
				cache->res[i].name,
				cache->res[i].cid);

			mutex_unlock(&cache->lock);
			goto err;
		}
	}

	v4l_dbg(0, V4L_DEBUG_CODEC_BUFMGR,
		"canvas-get, ref:%d\n", cache->ref);

	mutex_unlock(&cache->lock);
	return 0;
err:
	aml_canvas_cache_put(dev);
	return -1;
}

int aml_canvas_cache_init(struct aml_vcodec_dev *dev)
{
	dev->cache.ref = 0;
	mutex_init(&dev->cache.lock);

	v4l_dbg(0, V4L_DEBUG_CODEC_BUFMGR,
		"canvas-init, ref:%d\n", dev->cache.ref);

	return 0;
}

void aml_v4l_vpp_release_early(struct aml_vcodec_ctx * ctx)
{
	struct aml_vpp_cfg_infos *vpp_cfg = &ctx->vpp_cfg;

	if (ctx->vpp == NULL)
		return;

	if (vpp_cfg->early_release_flag ||
		!(vpp_cfg->enable_nr && atomic_read(&ctx->vpp->local_buf_out))) {
		aml_v4l2_vpp_destroy(ctx->vpp);
		atomic_dec(&ctx->dev->vpp_count);
		ctx->vpp = NULL;

		v4l_dbg(ctx, V4L_DEBUG_CODEC_PRINFO,
			"vpp destroy inst count:%d.\n",
			atomic_read(&ctx->dev->vpp_count));
	}
}

void aml_v4l_ctx_release(struct kref *kref)
{
	struct aml_vcodec_ctx * ctx;

	ctx = container_of(kref, struct aml_vcodec_ctx, ctx_ref);

	if (ctx->vpp) {
		aml_v4l2_vpp_destroy(ctx->vpp);
		atomic_dec(&ctx->dev->vpp_count);
		ctx->vpp = NULL;

		v4l_dbg(ctx, V4L_DEBUG_CODEC_PRINFO,
			"vpp destroy inst count:%d.\n",
			atomic_read(&ctx->dev->vpp_count));
	}

	if (ctx->ge2d) {
		aml_v4l2_ge2d_destroy(ctx->ge2d);

		v4l_dbg(ctx, V4L_DEBUG_CODEC_PRINFO,
			"ge2d destroy.\n");
	}

	v4l2_m2m_ctx_release(ctx->m2m_ctx);

	vfree(ctx->meta_infos.meta_bufs);
	ctx->aux_infos.free_buffer(ctx, SEI_TYPE | DV_TYPE | HDR10P_TYPE);

	v4l_dbg(ctx, V4L_DEBUG_CODEC_BUFMGR,
		"v4ldec has been destroyed.\n");

	if (ctx->sync) {
		vdec_clean_all_fence(ctx->sync);
	}

	vdec_trace_clean(&ctx->vtr);

#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
	if (ctx->dv_id >= 0) {
		dv_inst_unmap(ctx->dv_id);
		v4l_dbg(ctx, V4L_DEBUG_CODEC_BUFMGR,
				"%s: dv_inst_unmap ctx %p, dv_id %d\n", __func__, ctx, ctx->dv_id);
	}
#endif

	if (ctx->stream_mode) {
		ctx->set_ext_buf_flg = false;
		ptsserver_ins_release(ctx->ptsserver_id);
	}

	aml_es_mgr_release(ctx);

	kfree(ctx);
}

static void aml_uvm_buf_free(void *arg)
{
	struct aml_uvm_buff_ref * ubuf =
		(struct aml_uvm_buff_ref*)arg;
	struct aml_vcodec_ctx * ctx
		= container_of(ubuf->ref, struct aml_vcodec_ctx, ctx_ref);

	v4l_dbg(ctx, V4L_DEBUG_CODEC_BUFMGR,
		"%s, vb:%d, dbuf:%px, ino:%lu\n",
		__func__, ubuf->index, ubuf->dbuf,
		file_inode(ubuf->dbuf->file)->i_ino);

	aml_buf_detach(&ctx->bm, (ulong)ubuf->dbuf);

	kref_put(ubuf->ref, aml_v4l_ctx_release);
	vfree(ubuf);
}

int aml_uvm_buff_attach(struct vb2_buffer * vb)
{
	int ret = 0;
	struct dma_buf *dbuf = vb->planes[0].dbuf;
	struct uvm_hook_mod_info u_info;
	struct aml_vcodec_ctx *ctx =
		vb2_get_drv_priv(vb->vb2_queue);
	struct aml_uvm_buff_ref *ubuf = NULL;
	struct vb2_v4l2_buffer *vb2_v4l2 = to_vb2_v4l2_buffer(vb);
	struct aml_v4l2_buf *buf =
		container_of(vb2_v4l2, struct aml_v4l2_buf, vb);
	struct aml_buf *am_buf = buf->aml_buf;

	if (vb->memory != VB2_MEMORY_DMABUF || !dmabuf_is_uvm(dbuf))
		return 0;

	ubuf = vzalloc(sizeof(struct aml_uvm_buff_ref));
	if (ubuf == NULL)
		return -ENOMEM;

	ubuf->index	= vb->index;
	ubuf->addr	= get_addr(vb, 0);
	ubuf->dbuf	= dbuf;
	ubuf->ref	= &ctx->ctx_ref;
	am_buf->uvm_buf = (void *)ubuf;

	u_info.type	= VF_PROCESS_DECODER;
	u_info.arg	= (void *)ubuf;
	u_info.free	= aml_uvm_buf_free;
	ret = uvm_attach_hook_mod(dbuf, &u_info);
	if (ret < 0) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"aml uvm buffer %d attach fail.\n",
			ubuf->index);
		return ret;
	}

	kref_get(ubuf->ref);

	v4l_dbg(ctx, V4L_DEBUG_CODEC_BUFMGR,
		"%s, vb:%d, dbuf:%px, ino:%lu\n",
		__func__, ubuf->index, ubuf->dbuf,
		file_inode(ubuf->dbuf->file)->i_ino);

	return ret;
}

static ulong prepare_get_addr(struct dma_buf *dbuf, struct device	 *dev)
{
	struct dma_buf_attachment *dba;
	struct sg_table *sgt;
	ulong addr;

	/* create attachment for the dmabuf with the user device */
	dba = dma_buf_attach(dbuf, dev);
	if (IS_ERR(dba)) {
		pr_err("failed to attach dmabuf\n");
		return 0;
	}

	/* get the associated scatterlist for this buffer */
	sgt = dma_buf_map_attachment(dba, DMA_BIDIRECTIONAL);
	if (IS_ERR(sgt)) {
		pr_err("Error getting dmabuf scatterlist\n");
		return 0;
	}

	addr = sg_dma_address(sgt->sgl);

	/* unmap attachment and detach dbuf */
	dma_buf_unmap_attachment(dba, sgt, DMA_BIDIRECTIONAL);
	dma_buf_detach(dbuf, dba);

	return addr;
}

static void vb2ops_vdec_buf_queue(struct vb2_buffer *vb)
{
	struct aml_vcodec_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct vb2_v4l2_buffer *vb2_v4l2 = to_vb2_v4l2_buffer(vb);
	struct aml_v4l2_buf *buf =
		container_of(vb2_v4l2, struct aml_v4l2_buf, vb);
	struct aml_vcodec_mem src_mem;
	struct aml_buf_config config;
	u32 dw = DM_YUV_ONLY;
	u32 tw = DM_INVALID;

	v4l_dbg(ctx, V4L_DEBUG_CODEC_PROT,
		"%s, type: %d, vb: %lx, idx: %d, state: %d, used: %d, ts: %llu, uvm dbuf: %px\n",
		__func__, vb->vb2_queue->type, (ulong) vb,
		vb->index, vb->state, buf->used, vb->timestamp, vb->planes[0].dbuf);

	/*
	 * check if this buffer is ready to be used after decode
	 */
	if (!V4L2_TYPE_IS_OUTPUT(vb->vb2_queue->type)) {
		struct aml_buf *aml_buf = buf->aml_buf;
		struct vframe_s *vf;

		if (aml_buf->vb != vb) {
			v4l_dbg(ctx, V4L_DEBUG_CODEC_BUFMGR,
				"vb2 (old idx: %d new idx: %d) update\n",
				aml_buf->vb->index, vb->index);
			aml_buf_replace(&ctx->bm, aml_buf, vb);

		}

		if (aml_buf->pair == BUF_MASTER)
			aml_buf->queued_mask = 1 << BUF_MASTER;

		if (aml_buf->pair == BUF_SUB0) {
			aml_buf = aml_buf->master_buf;
			aml_buf->queued_mask = 1 << BUF_SUB0;
		}
		if (aml_buf->pair == BUF_SUB1) {
			aml_buf = aml_buf->master_buf;
			aml_buf->queued_mask = 1 << BUF_SUB1;
		}

		vf = &aml_buf->vframe;

		/* DI hook must be detached if the dmabuff be reused. */
		if (ctx->vpp_cfg.enable_local_buf) {
			if (!buf->attached) {
				struct dma_buf *dma = vb->planes[0].dbuf;

				if (uvm_detach_hook_mod(dma, VF_PROCESS_DI) < 0) {
					v4l_dbg(ctx, V4L_DEBUG_CODEC_EXINFO,
						"dmabuf without attach DI hook.\n");
				}
				buf->attached = true;
			} else
				aml_v4l2_vpp_recycle(ctx->vpp, buf);
		}

		v4l_dbg(ctx, V4L_DEBUG_CODEC_OUTPUT,
			"IN__BUFF (%s, st:%d, seq:%d) vb:(%d, %px), vf:(%d, %px), ts:%lld, "
			"dma addr: %llx Y:(%lx, %u) C/U:(%lx, %u) V:(%lx, %u)\n",
			ctx->ada_ctx->frm_name,
			aml_buf->state,
			ctx->in_buff_cnt,
			vb->index, vb,
			vf ? vf->index & 0xff : -1, vf,
			vf ? vf->timestamp : 0,
			(u64)get_addr(vb, 0),
			aml_buf->planes[0].addr, aml_buf->planes[0].length,
			aml_buf->planes[1].addr, aml_buf->planes[1].length,
			aml_buf->planes[2].addr, aml_buf->planes[2].length);

		ctx->in_buff_cnt++;

		vdec_tracing(&ctx->vtr, VTRACE_V4L_PIC_10,
			CTX_BUF_TOTAL(ctx) + ctx->out_buff_cnt - ctx->in_buff_cnt);
		vdec_tracing(&ctx->vtr, VTRACE_V4L_PIC_4, vb->index);

		aml_buf_fill(&ctx->bm, aml_buf, BUF_USER_VSINK);

		vdec_thread_wakeup(ctx->ada_ctx);

		wake_up_interruptible(&ctx->cap_wq);
		return;
	} else if (ctx->output_dma_mode) {
		struct dma_buf *dbuf = vb->planes[0].dbuf;
		struct device *dev = vb->vb2_queue->alloc_devs[0] ? :
							vb->vb2_queue->dev;

		if (dbuf && dev) {
			buf->addr = prepare_get_addr(dbuf, dev);
			if (buf->addr &&
				buf->addr != sg_dma_address(buf->out_sgt->sgl)) {
				v4l_dbg(ctx, V4L_DEBUG_CODEC_BUFMGR,
					"vb2 dma addr update(0x%llx --> 0x%lx)\n",
					(u64)sg_dma_address(buf->out_sgt->sgl), buf->addr);
			}
		} else
			v4l_dbg(ctx, 0,
					"dbuf %px dev %px\n", dbuf, dev);
	}

	if (ctx->stream_mode &&
		!(ctx->output_pix_fmt == V4L2_PIX_FMT_VC1_ANNEX_G ||
		ctx->output_pix_fmt == V4L2_PIX_FMT_VC1_ANNEX_L)) {
		struct dmabuf_dmx_sec_es_data *es_data;

		if (dmabuf_manage_get_type(vb->planes[0].dbuf) != DMA_BUF_TYPE_DMX_ES) {
			pr_err("not DMA_BUF_TYPE_DMX_ES\n");
			return;
		}
		es_data = (struct dmabuf_dmx_sec_es_data *)dmabuf_manage_get_info(vb->planes[0].dbuf, DMA_BUF_TYPE_DMX_ES);
		buf->dma_buf = (void *)es_data;
	}
	v4l2_m2m_buf_queue(ctx->m2m_ctx, to_vb2_v4l2_buffer(vb));

	if (ctx->state != AML_STATE_INIT) {
		return;
	}

	vb2_v4l2 = to_vb2_v4l2_buffer(vb);
	buf = container_of(vb2_v4l2, struct aml_v4l2_buf, vb);
	if (buf->lastframe) {
		/* This shouldn't happen. Just in case. */
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"Invalid flush buffer.\n");
		v4l2_m2m_src_buf_remove(ctx->m2m_ctx);

		return;
	}

	if (ctx->stream_mode &&
		!(ctx->output_pix_fmt == V4L2_PIX_FMT_VC1_ANNEX_G ||
		ctx->output_pix_fmt == V4L2_PIX_FMT_VC1_ANNEX_L)) {
		struct dmabuf_dmx_sec_es_data *es_data = (struct dmabuf_dmx_sec_es_data *)buf->dma_buf;
		int offset = vb->planes[0].data_offset;
		if (ctx->set_ext_buf_flg == false) {
			v4l2_set_ext_buf_addr(ctx->ada_ctx, es_data, offset);
			ctx->es_free = aml_es_input_free;
			ctx->set_ext_buf_flg = true;
		}

		src_mem.addr = es_data->data_start + offset;
		src_mem.size = vb->planes[0].bytesused - offset;
		src_mem.dbuf = vb->planes[0].dbuf;

		v4l_dbg(ctx, V4L_DEBUG_CODEC_INPUT, "update wp 0x%lx + sz 0x%x offset 0x%x ori start 0x%x pts %llu\n",
			src_mem.addr, src_mem.size, offset, es_data->data_start, vb->timestamp);

	} else {
		src_mem.addr	= buf->addr ? buf->addr :
				sg_dma_address(buf->out_sgt->sgl);
		src_mem.size	= vb->planes[0].bytesused;
	}
	src_mem.index	= vb->index;
	if (!ctx->is_drm_mode)
		src_mem.vaddr = vb2_plane_vaddr(vb, 0);

	src_mem.model	= vb->memory;
	src_mem.timestamp = vb->timestamp;
	src_mem.meta_ptr = (ulong)buf->meta_data;

	if (vdec_if_probe(ctx, &src_mem, NULL)) {
		v4l2_m2m_src_buf_remove(ctx->m2m_ctx);

		/* frame mode and non-dbuf mode. */
		if (ctx->stream_mode || !ctx->output_dma_mode) {
			v4l2_buff_done(to_vb2_v4l2_buffer(vb),
				VB2_BUF_STATE_DONE);
		}

		return;
	}

	/*
	 * If on model dmabuf must remove the buffer
	 * because this data has been consumed by hw.
	 */
	v4l2_m2m_src_buf_remove(ctx->m2m_ctx);

	/* frame mode and non-dbuf mode. */
	if (ctx->stream_mode || !ctx->output_dma_mode) {
		v4l2_buff_done(to_vb2_v4l2_buffer(vb),
			VB2_BUF_STATE_DONE);
	}

	if (vdec_if_get_param(ctx, GET_PARAM_PIC_INFO, &ctx->picinfo)) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"GET_PARAM_PICTURE_INFO err\n");
		return;
	}

	if (vdec_if_get_param(ctx, GET_PARAM_DW_MODE, &dw)) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR, "invalid dw_mode\n");
		return;

	}

	if (vdec_if_get_param(ctx, GET_PARAM_TW_MODE, &tw)) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR, "invalid tw_mode\n");
		return;

	}

	if (!ctx->picinfo.dpb_frames)
		return;

	v4l_buf_size_decision(ctx);
	ctx->last_decoded_picinfo = ctx->picinfo;

	config.enable_extbuf	= true;
	config.enable_fbc	= ((dw != DM_YUV_ONLY) || tw) ? true : false;
	config.enable_secure	= ctx->is_drm_mode;
	config.memory_mode	= vb->vb2_queue->memory;
	config.planes		= V4L2_TYPE_IS_MULTIPLANAR(vb->vb2_queue->type) ? 2 : 1;
	config.luma_length	= ctx->picinfo.y_len_sz;
	config.chroma_length	= ctx->picinfo.c_len_sz;
	config.luma_length_tw	= ctx->picinfo.y_len_sz_tw;
	config.chroma_length_tw	= ctx->picinfo.c_len_sz_tw;
	config.dw_mode		= dw;
	config.tw_mode		= tw;

	if (ctx->enable_di_post)
		ctx->bm.vpp_work_mode	= VPP_WORK_MODE_DI_POST;
	aml_buf_configure(&ctx->bm, &config);

	v4l_dbg(ctx, V4L_DEBUG_CODEC_PRINFO,
		"Picture buffer count: dec:%u, vpp:%u, ge2d:%u, margin:%u, total:%u\n",
		ctx->picinfo.dpb_frames, ctx->vpp_size, ctx->ge2d_size,
		ctx->picinfo.dpb_margin,
		CTX_BUF_TOTAL(ctx));

	aml_vdec_dispatch_event(ctx, V4L2_EVENT_SRC_CH_RESOLUTION);

	mutex_lock(&ctx->state_lock);
	if (ctx->state == AML_STATE_INIT) {
		ctx->state = AML_STATE_PROBE;
		vdec_tracing(&ctx->vtr, VTRACE_V4L_ST_0, ctx->state);
		v4l_dbg(ctx, V4L_DEBUG_CODEC_STATE,
			"vcodec state (AML_STATE_PROBE)\n");
	}
	mutex_unlock(&ctx->state_lock);
}

static void vb2ops_vdec_buf_finish(struct vb2_buffer *vb)
{
	struct aml_vcodec_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct vb2_v4l2_buffer *vb2_v4l2 = NULL;
	struct aml_v4l2_buf *buf = NULL;

	v4l_dbg(ctx, V4L_DEBUG_CODEC_PROT,
		"%s, type: %d, idx: %d\n",
		__func__, vb->vb2_queue->type, vb->index);

	vb2_v4l2 = to_vb2_v4l2_buffer(vb);
	buf = container_of(vb2_v4l2, struct aml_v4l2_buf, vb);

	if (buf->error) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"Unrecoverable error on buffer.\n");
		ctx->state = AML_STATE_ABORT;
		vdec_tracing(&ctx->vtr, VTRACE_V4L_ST_0, ctx->state);
		v4l_dbg(ctx, V4L_DEBUG_CODEC_STATE,
			"vcodec state (AML_STATE_ABORT)\n");
	}
}

static int vb2ops_vdec_buf_init(struct vb2_buffer *vb)
{
	struct aml_vcodec_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct vb2_v4l2_buffer *vb2_v4l2 = container_of(vb,
					struct vb2_v4l2_buffer, vb2_buf);
	struct aml_v4l2_buf *buf = container_of(vb2_v4l2,
					struct aml_v4l2_buf, vb);
	u32 size, phy_addr = 0;
	int i;
	int ret;

	v4l_dbg(ctx, V4L_DEBUG_CODEC_PROT, "%s, type: %d, idx: %d\n",
		__func__, vb->vb2_queue->type, vb->index);

	if (V4L2_TYPE_IS_OUTPUT(vb->vb2_queue->type)) {
		buf->lastframe = false;
	} else {
		buf->attached = false;
	}

	/* codec_mm buffers count */
	if (!V4L2_TYPE_IS_OUTPUT(vb->type)) {
		if (vb->memory == VB2_MEMORY_MMAP) {
			char *owner = __getname();

			snprintf(owner, PATH_MAX, "%s-%d", "v4l-output", ctx->id);
			strncpy(buf->mem_owner, owner, sizeof(buf->mem_owner));
			buf->mem_owner[sizeof(buf->mem_owner) - 1] = '\0';
			__putname(owner);

			for (i = 0; i < vb->num_planes; i++) {
				size = vb->planes[i].length;
				phy_addr = vb2_dma_contig_plane_dma_addr(vb, i);
				buf->mem[i] = v4l_reqbufs_from_codec_mm(buf->mem_owner,
						phy_addr, size, vb->index);
				v4l_dbg(ctx, V4L_DEBUG_CODEC_BUFMGR,
						"OUT %c alloc, addr: %x, size: %u, idx: %u\n",
						(i == 0? 'Y':'C'), phy_addr, size, vb->index);
			}
		} else if (vb->memory == VB2_MEMORY_DMABUF) {
			unsigned int dw_mode = DM_YUV_ONLY;

			for (i = 0; i < vb->num_planes; i++) {
				struct dma_buf * dma;

				if (vdec_if_get_param(ctx, GET_PARAM_DW_MODE, &dw_mode)) {
					v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR, "invalid dw_mode\n");
					return -EINVAL;
				}
				/* None-DW mode means single layer */
				if (dw_mode == DM_AVBC_ONLY && i > 0) {
					v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
							"only support single plane in dw mode 0\n");
					return -EINVAL;
				}
				size = vb->planes[i].length;
				dma = vb->planes[i].dbuf;

				if (!dmabuf_is_uvm(dma))
					v4l_dbg(ctx, V4L_DEBUG_CODEC_PRINFO, "non-uvm dmabuf\n");
			}
		}
	}

	if (!V4L2_TYPE_IS_OUTPUT(vb->type)) {
		ulong key;
		if (vb->memory == VB2_MEMORY_DMABUF)
			key = (ulong)vb->planes[0].dbuf;
		if (vb->memory == VB2_MEMORY_MMAP)
			key = vb2_dma_contig_plane_dma_addr(vb, 0);

		ret = aml_buf_attach(&ctx->bm, key,
			vb2_dma_contig_plane_dma_addr(vb, 0), vb);
		if (ret) {
			v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
				"aml_buf_attach fail!\n");
			return ret;
		}
		aml_uvm_buf_delay_alloc(ctx, vb2_v4l2);
	}

	if (V4L2_TYPE_IS_OUTPUT(vb->type)) {
		ulong contig_size;

		buf->out_sgt = vb2_dma_sg_plane_desc(vb, 0);

		contig_size = dmabuf_contiguous_size(buf->out_sgt);
		if (contig_size < vb->planes[0].bytesused) {
			v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
				"contiguous mapping is too small %lu/%u\n",
				contig_size, size);
			return -EFAULT;
		}
	}

	return 0;
}

static void vb2ops_vdec_buf_cleanup(struct vb2_buffer *vb)
{
	struct aml_vcodec_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct vb2_v4l2_buffer *vb2_v4l2 = container_of(vb,
					struct vb2_v4l2_buffer, vb2_buf);
	struct aml_v4l2_buf *buf = container_of(vb2_v4l2,
					struct aml_v4l2_buf, vb);

	v4l_dbg(ctx, V4L_DEBUG_CODEC_PROT, "%s, type: %d, idx: %d\n",
		__func__, vb->vb2_queue->type, vb->index);

	if (!V4L2_TYPE_IS_OUTPUT(vb->type)) {
		if (vb->memory == VB2_MEMORY_MMAP) {
			int i;

			for (i = 0; i < vb->num_planes ; i++) {
				v4l_dbg(ctx, V4L_DEBUG_CODEC_BUFMGR,
					"OUT %c clean, addr: %lx, size: %u, idx: %u\n",
					(i == 0)? 'Y':'C',
					buf->mem[i]->phy_addr, buf->mem[i]->buffer_size, vb->index);
				v4l_freebufs_back_to_codec_mm(buf->mem_owner, buf->mem[i]);
				buf->mem[i] = NULL;
			}
			aml_buf_detach(&ctx->bm, vb2_dma_contig_plane_dma_addr(vb, 0));
		}
	}
}

static int vb2ops_vdec_start_streaming(struct vb2_queue *q, unsigned int count)
{
	struct aml_vcodec_ctx *ctx = vb2_get_drv_priv(q);

	ctx->has_receive_eos = false;
	ctx->v4l_resolution_change = false;

	/* vdec has ready to decode subsequence data of new resolution. */
	//v4l2_m2m_job_resume(ctx->dev->m2m_dev_dec, ctx->m2m_ctx);

	v4l2_m2m_set_dst_buffered(ctx->fh.m2m_ctx, true);

	ctx->dst_queue_streaming = true;

	vdec_thread_wakeup(ctx->ada_ctx);

	v4l_dbg(ctx, V4L_DEBUG_CODEC_PROT,
		"%s, type: %d\n", __func__, q->type);

	return 0;
}

static void vb2ops_vdec_stop_streaming(struct vb2_queue *q)
{
	struct aml_v4l2_buf *buf = NULL;
	struct vb2_v4l2_buffer *vb2_v4l2 = NULL;
	struct aml_vcodec_ctx *ctx = vb2_get_drv_priv(q);
	int i;

	v4l_dbg(ctx, V4L_DEBUG_CODEC_PROT,
		"%s, type: %d, state: %x, frame_cnt: %d\n",
		__func__, q->type, ctx->state, ctx->decoded_frame_cnt);

	if (V4L2_TYPE_IS_OUTPUT(q->type)) {
		ctx->is_out_stream_off = true;
	} else {
		ctx->is_stream_off = true;
		ctx->dst_queue_streaming = false;
		ctx->out_buff_cnt = 0;
		ctx->in_buff_cnt = 0;
		ctx->write_frames = 0;
	}

	if (V4L2_TYPE_IS_OUTPUT(q->type)) {
		struct vb2_queue * que = v4l2_m2m_get_dst_vq(ctx->m2m_ctx);
		ulong flags;

		spin_lock_irqsave(&ctx->es_wkr_slock, flags);
		ctx->es_wkr_stop = true;
		spin_unlock_irqrestore(&ctx->es_wkr_slock, flags);

		cancel_work_sync(&ctx->es_wkr_in);
		cancel_work_sync(&ctx->es_wkr_out);
		INIT_KFIFO(ctx->dmabuff_recycle);

		while ((vb2_v4l2 = v4l2_m2m_src_buf_remove(ctx->m2m_ctx)))
			v4l2_buff_done(vb2_v4l2, VB2_BUF_STATE_ERROR);

		for (i = 0; i < q->num_buffers; ++i) {
			vb2_v4l2 = to_vb2_v4l2_buffer(q->bufs[i]);
			if (vb2_v4l2->vb2_buf.state == VB2_BUF_STATE_ACTIVE)
				v4l2_buff_done(vb2_v4l2, VB2_BUF_STATE_ERROR);
		}

		/*
		 * drop es frame was stored in the vdec_input
		 * if the capture queue have not start streaming.
		 */
		if (!que->streaming &&
			(vdec_frame_number(ctx->ada_ctx) > 0) &&
			(ctx->state < AML_STATE_ACTIVE)) {
			vdec_tracing(&ctx->vtr, VTRACE_V4L_ST_0, ctx->state);
			ctx->v4l_resolution_change = false;
			ctx->reset_flag = V4L_RESET_MODE_NORMAL;
			v4l_dbg(ctx, V4L_DEBUG_CODEC_PRINFO,
				"force reset to drop es frames.\n");
			wake_up_interruptible(&ctx->cap_wq);
			aml_vdec_reset(ctx);
		}
	} else {
		/* clean output cache and decoder status . */
		if (ctx->state > AML_STATE_INIT) {
			wake_up_interruptible(&ctx->cap_wq);
			aml_vdec_reset(ctx);
		}

		mutex_lock(&ctx->capture_buffer_lock);
		INIT_KFIFO(ctx->capture_buffer);
		mutex_unlock(&ctx->capture_buffer_lock);

		for (i = 0; i < q->num_buffers; ++i) {
			vb2_v4l2 = to_vb2_v4l2_buffer(q->bufs[i]);
			buf = container_of(vb2_v4l2, struct aml_v4l2_buf, vb);
			if (buf->aml_buf) {
				buf->aml_buf->state	= FB_ST_FREE;
				memset(&buf->aml_buf->vframe,
					0, sizeof(struct vframe_s));
			}
			buf->attached		= false;
			buf->used		= false;
			buf->vb.flags		= 0;

			if (vb2_v4l2->vb2_buf.state == VB2_BUF_STATE_ACTIVE)
				v4l2_buff_done(vb2_v4l2, VB2_BUF_STATE_ERROR);

			/*v4l_dbg(ctx, V4L_DEBUG_CODEC_EXINFO, "idx: %d, state: %d\n",
				q->bufs[i]->index, q->bufs[i]->state);*/
		}

		aml_buf_reset(&ctx->bm);
		aml_codec_connect(ctx->ada_ctx); /* for resolution change */
		aml_compressed_info_show(ctx);
		memset(&ctx->compressed_buf_info, 0, sizeof(ctx->compressed_buf_info));
		ctx->buf_used_count = 0;
	}
	if (ctx->is_out_stream_off && ctx->is_stream_off) {
		ctx->v4l_resolution_change = false;
		ctx->reset_flag = V4L_RESET_MODE_NORMAL;
		v4l_dbg(ctx, V4L_DEBUG_CODEC_PRINFO,
			"seek force reset to drop es frames.\n");
		aml_vdec_reset(ctx);
	}

	if (!V4L2_TYPE_IS_OUTPUT(q->type)) {
		atomic_set(&ctx->vpp_cache_num, 0);
		atomic_set(&ctx->ge2d_cache_num, 0);
	}
}

static void m2mops_vdec_device_run(void *priv)
{
	struct aml_vcodec_ctx *ctx = priv;
	struct aml_vcodec_dev *dev = ctx->dev;
	ulong flags;

	spin_lock_irqsave(&ctx->es_wkr_slock, flags);
	if (ctx->es_wkr_stop) {
		spin_unlock_irqrestore(&ctx->es_wkr_slock, flags);
		return;
	}

	queue_work(dev->decode_workqueue, &ctx->es_wkr_in);

	spin_unlock_irqrestore(&ctx->es_wkr_slock, flags);
}

static int m2mops_vdec_job_ready(void *m2m_priv)
{
	struct aml_vcodec_ctx *ctx = m2m_priv;

	return (ctx->es_wkr_stop ||
		!is_input_ready(ctx->ada_ctx) ||
		vdec_input_full(ctx->ada_ctx)) ? 0 : 1;
}

static void m2mops_vdec_job_abort(void *priv)
{
	struct aml_vcodec_ctx *ctx = priv;

	v4l2_m2m_job_finish(ctx->dev->m2m_dev_dec, ctx->m2m_ctx);
	v4l_dbg(ctx, V4L_DEBUG_CODEC_EXINFO, "%s\n", __func__);
}

static int aml_vdec_g_v_ctrl(struct v4l2_ctrl *ctrl)
{
	struct aml_vcodec_ctx *ctx = ctrl_to_ctx(ctrl);
	int ret = 0;

	v4l_dbg(ctx, V4L_DEBUG_CODEC_PROT,
		"%s, id: %d\n", __func__, ctrl->id);

	switch (ctrl->id) {
	case V4L2_CID_MIN_BUFFERS_FOR_CAPTURE:
		if (ctx->state >= AML_STATE_PROBE) {
			ctrl->val = CTX_BUF_TOTAL(ctx);
		} else {
			v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
				"Seqinfo not ready.\n");
			ctrl->val = 0;
		}
		break;
	case V4L2_CID_MIN_BUFFERS_FOR_OUTPUT:
		ctrl->val = 4;
		break;
	case AML_V4L2_GET_INPUT_BUFFER_NUM:
		if (ctx->ada_ctx != NULL)
			ctrl->val = vdec_frame_number(ctx->ada_ctx);
		break;
	case AML_V4L2_GET_FILMGRAIN_INFO:
		ctrl->val = ctx->film_grain_present;
		break;
	case AML_V4L2_GET_DECODER_INFO:
		memcpy(ctrl->p_new.p, &ctx->decoder_status_info,
			sizeof(struct aml_decoder_status_info));
		ctx->decoder_status_info.error_type = 0;
		break;
	case AML_V4L2_GET_BITDEPTH:
		ctrl->val = ctx->picinfo.bitdepth;
		v4l_dbg(ctx, V4L_DEBUG_CODEC_BUFMGR,
			"bitdepth: %d\n", ctrl->val);
		break;
	case AML_V4L2_DEC_PARMS_CONFIG:
		vidioc_vdec_g_parm_ext(ctrl, ctx);
		break;
	case AML_V4L2_GET_INST_ID:
		ctrl->val = ctx->id;
		break;
	default:
		ret = -EINVAL;
	}
	return ret;
}

static int aml_vdec_try_s_v_ctrl(struct v4l2_ctrl *ctrl)
{
	struct aml_vcodec_ctx *ctx = ctrl_to_ctx(ctrl);

	v4l_dbg(ctx, V4L_DEBUG_CODEC_PROT, "%s id %d val %d\n", __func__, ctrl->id, ctrl->val);

	if (ctrl->id == AML_V4L2_SET_DRMMODE) {
		ctx->is_drm_mode = ctrl->val;
		ctx->param_sets_from_ucode = true;
		v4l_dbg(ctx, V4L_DEBUG_CODEC_PRINFO,
			"set stream mode: %x\n", ctrl->val);
	} else if (ctrl->id == AML_V4L2_SET_DURATION) {
		vdec_set_duration(ctrl->val);
		v4l_dbg(ctx, V4L_DEBUG_CODEC_PRINFO,
			"set duration: %x\n", ctrl->val);
	} else if (ctrl->id == AML_V4L2_SET_INPUT_BUFFER_NUM_CACHE) {
		ctx->cache_input_buffer_num = ctrl->val;
		v4l_dbg(ctx, V4L_DEBUG_CODEC_BUFMGR,
			"cache_input_buffer_num: %d\n", ctrl->val);
	} else if (ctrl->id == AML_V4L2_DEC_PARMS_CONFIG) {
		vidioc_vdec_s_parm_ext(ctrl, ctx);
	} else if (ctrl->id == AML_V4L2_SET_STREAM_MODE) {
		u32 ret;
		ctx->stream_mode = ctrl->val;
		v4l_dbg(ctx, V4L_DEBUG_CODEC_PRINFO, "set streambase: %x\n", ctrl->val);

		if (ctx->stream_mode == true) {
			ptsserver_ins *pIns = NULL;
			ret = ptsserver_ins_alloc(&ctx->ptsserver_id, &pIns, NULL);
			if (ret < 0) {
				v4l_dbg(ctx, 0, "%s Alloc pts server fail!\n", __func__);
			}
			ptsserver_set_mode(ctx->ptsserver_id, true);
			ctx->pts_serves_ops = get_pts_server_ops();
			if (ctx->pts_serves_ops == NULL) {
				v4l_dbg(ctx, 0, "%s pts_serves_ops is NULL!\n", __func__);
			}
		}
	} else if (ctrl->id == AML_V4L2_SET_ES_DMABUF_TYPE) {
		ctx->output_dma_mode = ctrl->val;
		v4l_dbg(ctx, V4L_DEBUG_CODEC_PRINFO,
			"set dma buf mode: %x\n", ctrl->val);
	}
	return 0;
}

static const struct v4l2_ctrl_ops aml_vcodec_dec_ctrl_ops = {
	.g_volatile_ctrl = aml_vdec_g_v_ctrl,
	.try_ctrl = aml_vdec_try_s_v_ctrl,
};

static const struct v4l2_ctrl_config ctrl_st_mode = {
	.name	= "drm mode",
	.id	= AML_V4L2_SET_DRMMODE,
	.ops	= &aml_vcodec_dec_ctrl_ops,
	.type	= V4L2_CTRL_TYPE_BOOLEAN,
	.flags	= V4L2_CTRL_FLAG_WRITE_ONLY,
	.min	= 0,
	.max	= 1,
	.step	= 1,
	.def	= 0,
};

static const struct v4l2_ctrl_config ctrl_set_dma_buf_mode = {
	.name	= "dma buf mode",
	.id	= AML_V4L2_SET_ES_DMABUF_TYPE,
	.ops	= &aml_vcodec_dec_ctrl_ops,
	.type	= V4L2_CTRL_TYPE_BOOLEAN,
	.flags	= V4L2_CTRL_FLAG_WRITE_ONLY,
	.min	= 0,
	.max	= 1,
	.step	= 1,
	.def	= 0,
};

static const struct v4l2_ctrl_config ctrl_gt_input_buffer_number = {
	.name	= "input buffer number",
	.id	= AML_V4L2_GET_INPUT_BUFFER_NUM,
	.ops	= &aml_vcodec_dec_ctrl_ops,
	.type	= V4L2_CTRL_TYPE_INTEGER,
	.flags	= V4L2_CTRL_FLAG_VOLATILE,
	.min	= 0,
	.max	= 128,
	.step	= 1,
	.def	= 0,
};

static const struct v4l2_ctrl_config ctrl_set_input_buffer_number_cache = {
	.name	= "input buffer number cache",
	.id	= AML_V4L2_SET_INPUT_BUFFER_NUM_CACHE,
	.ops	= &aml_vcodec_dec_ctrl_ops,
	.type	= V4L2_CTRL_TYPE_INTEGER,
	.min	= 0,
	.max	= 128,
	.step	= 1,
	.def	= 0,
};

static const struct v4l2_ctrl_config ctrl_st_duration = {
	.name	= "duration",
	.id	= AML_V4L2_SET_DURATION,
	.ops	= &aml_vcodec_dec_ctrl_ops,
	.type	= V4L2_CTRL_TYPE_INTEGER,
	.flags	= V4L2_CTRL_FLAG_WRITE_ONLY,
	.min	= 0,
	.max	= 96000,
	.step	= 1,
	.def	= 0,
};

static const struct v4l2_ctrl_config ctrl_stream_mode = {
	.name	= "stream mode",
	.id	= AML_V4L2_SET_STREAM_MODE,
	.ops	= &aml_vcodec_dec_ctrl_ops,
	.type	= V4L2_CTRL_TYPE_INTEGER,
	.flags	= V4L2_CTRL_FLAG_WRITE_ONLY,
	.min	= 0,
	.max	= 1,
	.step	= 1,
	.def	= 0,
};

static const struct v4l2_ctrl_config ctrl_gt_filmgrain_info = {
	.name	= "filmgrain info",
	.id	= AML_V4L2_GET_FILMGRAIN_INFO,
	.ops	= &aml_vcodec_dec_ctrl_ops,
	.type	= V4L2_CTRL_TYPE_INTEGER,
	.flags	= V4L2_CTRL_FLAG_VOLATILE,
	.min	= 0,
	.max	= 1,
	.step	= 1,
	.def	= 0,
};

static const struct v4l2_ctrl_config ctrl_gt_bit_depth = {
	.name	= "bitdepth",
	.id	= AML_V4L2_GET_BITDEPTH,
	.ops	= &aml_vcodec_dec_ctrl_ops,
	.type	= V4L2_CTRL_TYPE_INTEGER,
	.flags	= V4L2_CTRL_FLAG_VOLATILE,
	.min	= 0,
	.max	= 128,
	.step	= 1,
	.def	= 0,
};

static const struct v4l2_ctrl_config ctrl_sgt_streamparm = {
	.name	= "streamparm",
	.id	= AML_V4L2_DEC_PARMS_CONFIG,
	.ops	= &aml_vcodec_dec_ctrl_ops,
	.type	= V4L2_CTRL_COMPOUND_TYPES,
	.flags	= V4L2_CTRL_FLAG_VOLATILE,
	.dims = { sizeof(struct aml_dec_params) },
	.min	= 0,
	.max	= 0xff,
	.step	= 1,
	.def	= 0,
};

static const struct v4l2_ctrl_config ctrl_get_inst_id = {
	.name	= "Get instance id",
	.id	= AML_V4L2_GET_INST_ID,
	.ops	= &aml_vcodec_dec_ctrl_ops,
	.type	= V4L2_CTRL_TYPE_INTEGER,
	.flags	= V4L2_CTRL_FLAG_READ_ONLY | V4L2_CTRL_FLAG_VOLATILE,
	.min	= 0,
	.max	= 0x7fffffff,
	.step	= 1,
	.def	= 0,
};

static const struct v4l2_ctrl_config ctrl_gt_decoder_info = {
	.name		= "decoder information",
	.id		= AML_V4L2_GET_DECODER_INFO,
	.ops		= &aml_vcodec_dec_ctrl_ops,
	.type		= V4L2_CTRL_COMPOUND_TYPES,
	.flags		= V4L2_CTRL_FLAG_VOLATILE,
	.min		= 0,
	.max		= sizeof(struct aml_decoder_status_info),
	.step		= 1,
	.elem_size	= 1,
	.dims		= { sizeof(struct aml_decoder_status_info) },
};

int aml_vcodec_dec_ctrls_setup(struct aml_vcodec_ctx *ctx)
{
	int ret;
	struct v4l2_ctrl *ctrl;

	v4l2_ctrl_handler_init(&ctx->ctrl_hdl, 3);
	ctrl = v4l2_ctrl_new_std(&ctx->ctrl_hdl,
				&aml_vcodec_dec_ctrl_ops,
				V4L2_CID_MIN_BUFFERS_FOR_CAPTURE,
				0, 32, 1, 2);
	if ((ctrl == NULL) || (ctx->ctrl_hdl.error)) {
		ret = ctx->ctrl_hdl.error;
		goto err;
	}
	ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;

	ctrl = v4l2_ctrl_new_std(&ctx->ctrl_hdl,
				&aml_vcodec_dec_ctrl_ops,
				V4L2_CID_MIN_BUFFERS_FOR_OUTPUT,
				0, 32, 1, 8);
	if ((ctrl == NULL) || (ctx->ctrl_hdl.error)) {
		ret = ctx->ctrl_hdl.error;
		goto err;
	}
	ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;

	ctrl = v4l2_ctrl_new_custom(&ctx->ctrl_hdl, &ctrl_st_mode, NULL);
	if ((ctrl == NULL) || (ctx->ctrl_hdl.error)) {
		ret = ctx->ctrl_hdl.error;
		goto err;
	}

	ctrl = v4l2_ctrl_new_custom(&ctx->ctrl_hdl, &ctrl_set_dma_buf_mode, NULL);
	if ((ctrl == NULL) || (ctx->ctrl_hdl.error)) {
		ret = ctx->ctrl_hdl.error;
		goto err;
	}

	ctrl = v4l2_ctrl_new_custom(&ctx->ctrl_hdl, &ctrl_gt_input_buffer_number, NULL);
	if ((ctrl == NULL) || (ctx->ctrl_hdl.error)) {
		ret = ctx->ctrl_hdl.error;
		goto err;
	}

	ctrl = v4l2_ctrl_new_custom(&ctx->ctrl_hdl, &ctrl_set_input_buffer_number_cache, NULL);
	if ((ctrl == NULL) || (ctx->ctrl_hdl.error)) {
		ret = ctx->ctrl_hdl.error;
		goto err;
	}

	ctrl = v4l2_ctrl_new_custom(&ctx->ctrl_hdl, &ctrl_st_duration, NULL);
	if ((ctrl == NULL) || (ctx->ctrl_hdl.error)) {
		ret = ctx->ctrl_hdl.error;
		goto err;
	}

	ctrl = v4l2_ctrl_new_custom(&ctx->ctrl_hdl, &ctrl_stream_mode, NULL);
	if ((ctrl == NULL) || (ctx->ctrl_hdl.error)) {
		ret = ctx->ctrl_hdl.error;
		goto err;
	}

	ctrl = v4l2_ctrl_new_custom(&ctx->ctrl_hdl, &ctrl_gt_filmgrain_info, NULL);
	if ((ctrl == NULL) || (ctx->ctrl_hdl.error)) {
		ret = ctx->ctrl_hdl.error;
		goto err;
	}

	ctrl = v4l2_ctrl_new_custom(&ctx->ctrl_hdl, &ctrl_gt_decoder_info, NULL);
	if ((ctrl == NULL) || (ctx->ctrl_hdl.error)) {
		ret = ctx->ctrl_hdl.error;
		goto err;
	}

	ctrl = v4l2_ctrl_new_custom(&ctx->ctrl_hdl, &ctrl_gt_bit_depth, NULL);
	if ((ctrl == NULL) || (ctx->ctrl_hdl.error)) {
		ret = ctx->ctrl_hdl.error;
		goto err;
	}

	ctrl = v4l2_ctrl_new_custom(&ctx->ctrl_hdl, &ctrl_sgt_streamparm, NULL);
	if ((ctrl == NULL) || (ctx->ctrl_hdl.error)) {
		ret = ctx->ctrl_hdl.error;
		goto err;
	}

	ctrl = v4l2_ctrl_new_custom(&ctx->ctrl_hdl, &ctrl_get_inst_id, NULL);
	if ((ctrl == NULL) || (ctx->ctrl_hdl.error)) {
		ret = ctx->ctrl_hdl.error;
		goto err;
	}

	v4l2_ctrl_handler_setup(&ctx->ctrl_hdl);

	return 0;
err:
	v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
		"Adding control failed %d\n",
		ctx->ctrl_hdl.error);
	v4l2_ctrl_handler_free(&ctx->ctrl_hdl);
	return ret;
}

static int vidioc_vdec_g_parm(struct file *file, void *fh,
	struct v4l2_streamparm *a)
{
	struct aml_vcodec_ctx *ctx = fh_to_ctx(fh);
	struct vb2_queue *dst_vq;

	dst_vq = v4l2_m2m_get_vq(ctx->m2m_ctx, V4L2_BUF_TYPE_VIDEO_CAPTURE);
	if (!dst_vq) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"no vb2 queue for type=%d\n", V4L2_BUF_TYPE_VIDEO_CAPTURE);
		return -EINVAL;
	}

	if (!V4L2_TYPE_IS_MULTIPLANAR(a->type) && dst_vq->is_multiplanar)
		return -EINVAL;

	if ((a->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) || (a->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)) {
		if (vdec_if_get_param(ctx, GET_PARAM_CONFIG_INFO, &ctx->config.parm.dec))
			v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
				"GET_PARAM_CONFIG_INFO err\n");
		else
			memcpy(a->parm.raw_data, ctx->config.parm.data,
				sizeof(a->parm.raw_data));
	}

	v4l_dbg(ctx, V4L_DEBUG_CODEC_PROT, "%s\n", __func__);

	return 0;
}

static void vidioc_vdec_g_parm_ext(struct v4l2_ctrl *ctrl,
	struct aml_vcodec_ctx *ctx)
{
	struct v4l2_streamparm parm = {0};

	parm.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	memcpy(&parm.parm.raw_data, ctrl->p_new.p, sizeof(parm.parm.raw_data));
	vidioc_vdec_g_parm(NULL, &ctx->fh, &parm);
	memcpy(ctrl->p_new.p, &parm.parm.raw_data, sizeof(parm.parm.raw_data));

}

static int vidioc_vdec_g_pixelaspect(struct file *file, void *fh,
	int buf_type, struct v4l2_fract *aspect)
{
	struct aml_vcodec_ctx *ctx = fh_to_ctx(fh);
	u32 height_aspect_ratio, width_aspect_ratio;

	if ((aspect == NULL) || (ctx == NULL)) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"GET_PARAM_PIXEL_ASPECT_INFO err\n");
		return -EFAULT;
	}

	height_aspect_ratio = ctx->height_aspect_ratio;
	width_aspect_ratio = ctx->width_aspect_ratio;

	if ((height_aspect_ratio != 0) && (width_aspect_ratio != 0)) {
		aspect->numerator = height_aspect_ratio;
		aspect->denominator = width_aspect_ratio;
	}

	v4l_dbg(ctx, V4L_DEBUG_CODEC_PROT, "%s: numerator is %d, denominator is %d\n",
		__func__, aspect->numerator, aspect->denominator);

	return 0;
}

static int check_dec_cfginfo(struct aml_vdec_cfg_infos *cfg)
{
	if (cfg->double_write_mode != DM_AVBC_ONLY &&
		cfg->double_write_mode != DM_YUV_1_1_AVBC &&
		cfg->double_write_mode != DM_YUV_1_4_AVBC_A &&
		cfg->double_write_mode != DM_YUV_1_4_AVBC_B &&
		cfg->double_write_mode != DM_YUV_1_2_AVBC &&
		cfg->double_write_mode != DM_YUV_ONLY &&
		cfg->double_write_mode != DM_AVBC_1_1 &&
		cfg->double_write_mode != DM_YUV_AUTO_1_2_AVBC &&
		cfg->double_write_mode != DM_YUV_AUTO_1_4_AVBC &&
		cfg->double_write_mode != DM_YUV_1_1_10BIT_AVBC &&
		cfg->double_write_mode != DM_YUV_1_4_10BIT_AVBC &&
		cfg->double_write_mode != DM_YUV_1_2_10BIT_AVBC &&
		cfg->double_write_mode != DM_YUV_1_8_10BIT_AVBC) {
		pr_err("Invalid DW:0x%x\n", cfg->double_write_mode);
		return -1;
	}

	if (cfg->triple_write_mode != DM_INVALID &&
		cfg->triple_write_mode != DM_YUV_1_1_AVBC &&
		cfg->triple_write_mode != DM_YUV_1_4_AVBC_A &&
		cfg->triple_write_mode != DM_YUV_1_4_AVBC_B &&
		cfg->triple_write_mode != DM_YUV_1_2_AVBC &&
		cfg->triple_write_mode != DM_YUV_1_1_10BIT_AVBC &&
		cfg->triple_write_mode != DM_YUV_1_4_10BIT_AVBC &&
		cfg->triple_write_mode != DM_YUV_1_2_10BIT_AVBC &&
		cfg->triple_write_mode != DM_YUV_1_8_10BIT_AVBC) {
		pr_err("Invalid TW:0x%x\n", cfg->triple_write_mode);
		return -1;
	}

	if (cfg->ref_buf_margin > 20) {
		pr_err("Invalid margin %d\n", cfg->ref_buf_margin);
		return -1;
	}

	pr_info("DW:%x, TW:%x, Margin:%d\n",
		cfg->double_write_mode,
		cfg->triple_write_mode,
		cfg->ref_buf_margin);

	return 0;
}

static int vidioc_vdec_s_parm(struct file *file, void *fh,
	struct v4l2_streamparm *a)
{
	struct aml_vcodec_ctx *ctx = fh_to_ctx(fh);
	struct vb2_queue *dst_vq;

	v4l_dbg(ctx, V4L_DEBUG_CODEC_PROT, "%s\n", __func__);

	dst_vq = v4l2_m2m_get_vq(ctx->m2m_ctx, V4L2_BUF_TYPE_VIDEO_CAPTURE);
	if (!dst_vq) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"no vb2 queue for type=%d\n", V4L2_BUF_TYPE_VIDEO_CAPTURE);
		return -EINVAL;
	}

	if (!V4L2_TYPE_IS_MULTIPLANAR(a->type) && dst_vq->is_multiplanar)
		return -EINVAL;

	if (a->type == V4L2_BUF_TYPE_VIDEO_OUTPUT ||
		a->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		struct aml_dec_params *in =
			(struct aml_dec_params *) a->parm.raw_data;
		struct aml_dec_params *dec = &ctx->config.parm.dec;

		ctx->config.type = V4L2_CONFIG_PARM_DECODE;

		if (in->parms_status & V4L2_CONFIG_PARM_DECODE_CFGINFO) {
			if (check_dec_cfginfo(&in->cfg))
				return -EINVAL;
			dec->cfg = in->cfg;
		}

		if (!vdec_if_set_param(ctx, SET_PARAM_CFG_INFO, &dec->cfg) &&
			!vdec_if_get_param(ctx, GET_PARAM_PIC_INFO, &ctx->picinfo)) {
			update_ctx_dimension(ctx, dst_vq->type);
			aml_buf_configure_update(ctx);
		}

		if (in->parms_status & V4L2_CONFIG_PARM_DECODE_PSINFO)
			dec->ps = in->ps;
		if (in->parms_status & V4L2_CONFIG_PARM_DECODE_HDRINFO)
			dec->hdr = in->hdr;
		if (in->parms_status & V4L2_CONFIG_PARM_DECODE_CNTINFO)
			dec->cnt = in->cnt;

		dec->parms_status |= in->parms_status;

		/* aml v4l driver parms config. */
		ctx->vpp_cfg.enable_nr =
			(dec->cfg.metadata_config_flag & (1 << 15));
		if (force_enable_nr) {
			if (force_enable_nr & 0x1)
				ctx->vpp_cfg.enable_nr = true;
			if (force_enable_nr & 0x2)
				ctx->vpp_cfg.enable_nr = false;
		}

		ctx->vpp_cfg.enable_local_buf =
			(dec->cfg.metadata_config_flag & (1 << 14));
		if (force_enable_di_local_buffer) {
			if (force_enable_di_local_buffer & 0x1)
				ctx->vpp_cfg.enable_local_buf = true;
			if (force_enable_di_local_buffer & 0x2)
				ctx->vpp_cfg.enable_local_buf = false;
		}
		ctx->vpp_cfg.dynamic_bypass_vpp =
			dec->cfg.metadata_config_flag & (1 << 10);

		ctx->vpp_cfg.early_release_flag = dec->cfg.metadata_config_flag & (1 << 18);

		ctx->ge2d_cfg.bypass =
			(dec->cfg.metadata_config_flag & (1 << 9));
		ctx->vpp_cfg.bypass =
			(dec->cfg.metadata_config_flag & (1 << 8));

		if (ctx->ge2d_cfg.bypass || ctx->vpp_cfg.bypass)
			v4l_buf_size_decision(ctx);

		ctx->internal_dw_scale = dec->cfg.metadata_config_flag & (1 << 13);
		ctx->second_field_pts_mode = dec->cfg.metadata_config_flag & (1 << 12);
		ctx->force_di_permission = dec->cfg.metadata_config_flag & (1 << 17);
		ctx->no_fbc_output = dec->cfg.metadata_config_flag & (1 << 19);
		if (force_di_permission)
			ctx->force_di_permission = true;

		ctx->enable_di_post = dec->cfg.metadata_config_flag & (1 << 20);
		if (enable_di_post) {
			ctx->enable_di_post = true;
		}
		ctx->alloc_type = dec->cfg.metadata_config_flag & (1 << 21);

		v4l_dbg(ctx, V4L_DEBUG_CODEC_PROT, "%s parms:%x metadata_config_flag: 0x%x\n",
				__func__, in->parms_status, dec->cfg.metadata_config_flag);

		memset(a->parm.output.reserved, 0, sizeof(a->parm.output.reserved));
	} else {
		memset(a->parm.capture.reserved, 0, sizeof(a->parm.capture.reserved));
	}

	return 0;
}

static void vidioc_vdec_s_parm_ext(struct v4l2_ctrl *ctrl,
	struct aml_vcodec_ctx *ctx)
{
	struct v4l2_streamparm parm = {0};
	struct vb2_queue *dst_vq;
	dst_vq = v4l2_m2m_get_vq(ctx->m2m_ctx, V4L2_BUF_TYPE_VIDEO_CAPTURE);

	if (dst_vq->is_multiplanar == 1)
		parm.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	else
		parm.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

	memcpy(&parm.parm.raw_data, ctrl->p_new.p, sizeof(parm.parm.raw_data));
	vidioc_vdec_s_parm(NULL, &ctx->fh, &parm);
}

const struct v4l2_m2m_ops aml_vdec_m2m_ops = {
	.device_run	= m2mops_vdec_device_run,
	.job_ready	= m2mops_vdec_job_ready,
	.job_abort	= m2mops_vdec_job_abort,
};

static const struct vb2_ops aml_vdec_vb2_ops = {
	.queue_setup	= vb2ops_vdec_queue_setup,
	.buf_prepare	= vb2ops_vdec_buf_prepare,
	.buf_queue	= vb2ops_vdec_buf_queue,
	.wait_prepare	= vb2_ops_wait_prepare,
	.wait_finish	= vb2_ops_wait_finish,
	.buf_init	= vb2ops_vdec_buf_init,
	.buf_cleanup	= vb2ops_vdec_buf_cleanup,
	.buf_finish	= vb2ops_vdec_buf_finish,
	.start_streaming = vb2ops_vdec_start_streaming,
	.stop_streaming	= vb2ops_vdec_stop_streaming,
};

const struct v4l2_ioctl_ops aml_vdec_ioctl_ops = {
	.vidioc_streamon		= vidioc_decoder_streamon,
	.vidioc_streamoff		= vidioc_decoder_streamoff,
	.vidioc_reqbufs			= vidioc_decoder_reqbufs,
	.vidioc_querybuf		= vidioc_vdec_querybuf,
	.vidioc_expbuf			= vidioc_vdec_expbuf,
	//.vidioc_g_ctrl		= vidioc_vdec_g_ctrl,

	.vidioc_qbuf			= vidioc_vdec_qbuf,
	.vidioc_dqbuf			= vidioc_vdec_dqbuf,

	.vidioc_try_fmt_vid_cap_mplane	= vidioc_try_fmt_vid_cap_out,
	.vidioc_try_fmt_vid_cap		= vidioc_try_fmt_vid_cap_out,
	.vidioc_try_fmt_vid_out_mplane	= vidioc_try_fmt_vid_cap_out,
	.vidioc_try_fmt_vid_out		= vidioc_try_fmt_vid_cap_out,

	.vidioc_s_fmt_vid_cap_mplane	= vidioc_vdec_s_fmt,
	.vidioc_s_fmt_vid_cap		= vidioc_vdec_s_fmt,
	.vidioc_s_fmt_vid_out_mplane	= vidioc_vdec_s_fmt,
	.vidioc_s_fmt_vid_out		= vidioc_vdec_s_fmt,
	.vidioc_g_fmt_vid_cap_mplane	= vidioc_vdec_g_fmt,
	.vidioc_g_fmt_vid_cap		= vidioc_vdec_g_fmt,
	.vidioc_g_fmt_vid_out_mplane	= vidioc_vdec_g_fmt,
	.vidioc_g_fmt_vid_out		= vidioc_vdec_g_fmt,

	.vidioc_create_bufs		= vidioc_vdec_create_bufs,

	//fixme
	//.vidioc_enum_fmt_vid_cap_mplane	= vidioc_vdec_enum_fmt_vid_cap_mplane,
	//.vidioc_enum_fmt_vid_out_mplane = vidioc_vdec_enum_fmt_vid_out_mplane,
	.vidioc_enum_fmt_vid_cap	= vidioc_vdec_enum_fmt_vid_cap_mplane,
	.vidioc_enum_fmt_vid_out	= vidioc_vdec_enum_fmt_vid_out_mplane,
	.vidioc_enum_framesizes		= vidioc_enum_framesizes,

	.vidioc_querycap		= vidioc_vdec_querycap,
	.vidioc_subscribe_event		= vidioc_vdec_subscribe_evt,
	.vidioc_unsubscribe_event	= vidioc_vdec_event_unsubscribe,
	.vidioc_g_selection		= vidioc_vdec_g_selection,
	.vidioc_s_selection		= vidioc_vdec_s_selection,

	.vidioc_decoder_cmd		= vidioc_decoder_cmd,
	.vidioc_try_decoder_cmd		= vidioc_try_decoder_cmd,

	.vidioc_g_parm			= vidioc_vdec_g_parm,
	.vidioc_s_parm			= vidioc_vdec_s_parm,

	.vidioc_g_pixelaspect		= vidioc_vdec_g_pixelaspect,
};

int aml_vcodec_dec_queue_init(void *priv, struct vb2_queue *src_vq,
			   struct vb2_queue *dst_vq)
{
	struct aml_vcodec_ctx *ctx = priv;
	int ret = 0;

	v4l_dbg(ctx, V4L_DEBUG_CODEC_EXINFO, "%s\n", __func__);

	src_vq->type		= V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	src_vq->io_modes	= VB2_DMABUF | VB2_MMAP | VB2_USERPTR;
	src_vq->drv_priv	= ctx;
	src_vq->buf_struct_size = sizeof(struct aml_v4l2_buf);
	src_vq->ops		= &aml_vdec_vb2_ops;
	src_vq->mem_ops		= &vb2_dma_sg_memops;
	src_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	src_vq->lock		= &ctx->v4l_intf_lock;
	ret = vb2_queue_init(src_vq);
	if (ret) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"Failed to initialize videobuf2 queue(output)\n");
		return ret;
	}

	dst_vq->type		= multiplanar ? V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE :
					V4L2_BUF_TYPE_VIDEO_CAPTURE;
	dst_vq->io_modes	= VB2_DMABUF | VB2_MMAP | VB2_USERPTR;
	dst_vq->drv_priv	= ctx;
	dst_vq->buf_struct_size = sizeof(struct aml_v4l2_buf);
	dst_vq->ops		= &aml_vdec_vb2_ops;
	dst_vq->mem_ops		= &vb2_dma_contig_memops;
	dst_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	dst_vq->lock		= &ctx->v4l_intf_lock;
	dst_vq->min_buffers_needed = 1;
	ret = vb2_queue_init(dst_vq);
	if (ret) {
		vb2_queue_release(src_vq);
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"Failed to initialize videobuf2 queue(capture)\n");
	}

	return ret;
}

