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
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <linux/sched/clock.h>
#include <uapi/linux/sched/types.h>
#include <linux/amlogic/meson_uvm_core.h>
#include <linux/amlogic/media/ge2d/ge2d.h>
#include <linux/amlogic/media/canvas/canvas_mgr.h>

#include "../frame_provider/decoder/utils/aml_buf_helper.h"
#include "../common/chips/decoder_cpu_ver_info.h"
#include "aml_vcodec_ge2d.h"
#include "aml_vcodec_adapt.h"
#include "vdec_drv_if.h"
#include "utils/common.h"
#include "../common/media_utils/media_utils.h"

#define GE2D_BUF_GET_IDX(ge2d_buf) (ge2d_buf->aml_vb->vb.vb2_buf.index)

#define INPUT_PORT 0
#define OUTPUT_PORT 1

extern int dump_ge2d_input;
extern int ge2d_bypass_frames;
extern char dump_path[32];

enum GE2D_FLAG {
	GE2D_FLAG_P		= 0x1,
	GE2D_FLAG_I		= 0x2,
	GE2D_FLAG_EOS		= 0x4,
	GE2D_FLAG_BUF_BY_PASS	= 0x8,
	GE2D_FLAG_MAX		= 0x7FFFFFFF,
};

enum videocom_source_type {
	DECODER_8BIT_NORMAL = 0,
	DECODER_8BIT_BOTTOM,
	DECODER_8BIT_TOP,
	DECODER_10BIT_NORMAL,
	DECODER_10BIT_BOTTOM,
	DECODER_10BIT_TOP
};

#ifndef  CONFIG_AMLOGIC_MEDIA_GE2D
inline void stretchblt_noalpha(struct ge2d_context_s *wq,
		int src_x, int src_y, int src_w, int src_h,
		int dst_x, int dst_y, int dst_w, int dst_h) { return; }
inline int ge2d_context_config_ex(struct ge2d_context_s *context,
		struct config_para_ex_s *ge2d_config) { return -1; }
inline struct ge2d_context_s *create_ge2d_work_queue(void) { return NULL; }
inline int destroy_ge2d_work_queue(struct ge2d_context_s *ge2d_work_queue) { return -1; }
#endif

static int get_source_type(struct vframe_s *vf)
{
	enum videocom_source_type ret;
	int interlace_mode;

	interlace_mode = vf->type & VIDTYPE_TYPEMASK;

	if ((vf->bitdepth & BITDEPTH_Y10)  &&
		(!(vf->type & VIDTYPE_COMPRESS)) &&
		(get_cpu_type() >= MESON_CPU_MAJOR_ID_TXL)) {
		if (interlace_mode == VIDTYPE_INTERLACE_TOP)
			ret = DECODER_10BIT_TOP;
		else if (interlace_mode == VIDTYPE_INTERLACE_BOTTOM)
			ret = DECODER_10BIT_BOTTOM;
		else
			ret = DECODER_10BIT_NORMAL;
	} else {
		if (interlace_mode == VIDTYPE_INTERLACE_TOP)
			ret = DECODER_8BIT_TOP;
		else if (interlace_mode == VIDTYPE_INTERLACE_BOTTOM)
			ret = DECODER_8BIT_BOTTOM;
		else
			ret = DECODER_8BIT_NORMAL;
	}

	return ret;
}

static int get_input_format(struct vframe_s *vf)
{
	int format = GE2D_FORMAT_M24_YUV420;
	enum videocom_source_type source_type;

	source_type = get_source_type(vf);

	switch (source_type) {
	case DECODER_8BIT_NORMAL:
		if (vf->type & VIDTYPE_VIU_422)
			format = GE2D_FORMAT_S16_YUV422;
		else if (vf->type & VIDTYPE_VIU_NV21)
			format = GE2D_FORMAT_M24_NV21;
		else if (vf->type & VIDTYPE_VIU_NV12)
			format = GE2D_FORMAT_M24_NV12;
		else if (vf->type & VIDTYPE_VIU_444)
			format = GE2D_FORMAT_S24_YUV444;
		else
			format = GE2D_FORMAT_M24_YUV420;
		break;
	case DECODER_8BIT_BOTTOM:
		if (vf->type & VIDTYPE_VIU_422)
			format = GE2D_FORMAT_S16_YUV422
				| (GE2D_FORMAT_S16_YUV422B & (3 << 3));
		else if (vf->type & VIDTYPE_VIU_NV21)
			format = GE2D_FORMAT_M24_NV21
				| (GE2D_FORMAT_M24_NV21B & (3 << 3));
		else if (vf->type & VIDTYPE_VIU_NV12)
			format = GE2D_FORMAT_M24_NV12
				| (GE2D_FORMAT_M24_NV12B & (3 << 3));
		else if (vf->type & VIDTYPE_VIU_444)
			format = GE2D_FORMAT_S24_YUV444
				| (GE2D_FORMAT_S24_YUV444B & (3 << 3));
		else
			format = GE2D_FORMAT_M24_YUV420
				| (GE2D_FMT_M24_YUV420B & (3 << 3));
		break;
	case DECODER_8BIT_TOP:
		if (vf->type & VIDTYPE_VIU_422)
			format = GE2D_FORMAT_S16_YUV422
				| (GE2D_FORMAT_S16_YUV422T & (3 << 3));
		else if (vf->type & VIDTYPE_VIU_NV21)
			format = GE2D_FORMAT_M24_NV21
				| (GE2D_FORMAT_M24_NV21T & (3 << 3));
		else if (vf->type & VIDTYPE_VIU_NV12)
			format = GE2D_FORMAT_M24_NV12
				| (GE2D_FORMAT_M24_NV12T & (3 << 3));
		else if (vf->type & VIDTYPE_VIU_444)
			format = GE2D_FORMAT_S24_YUV444
				| (GE2D_FORMAT_S24_YUV444T & (3 << 3));
		else
			format = GE2D_FORMAT_M24_YUV420
				| (GE2D_FMT_M24_YUV420T & (3 << 3));
		break;
	case DECODER_10BIT_NORMAL:
		if (vf->type & VIDTYPE_VIU_422) {
			if (vf->bitdepth & FULL_PACK_422_MODE)
				format = GE2D_FORMAT_S16_10BIT_YUV422;
			else
				format = GE2D_FORMAT_S16_12BIT_YUV422;
		}
		break;
	case DECODER_10BIT_BOTTOM:
		if (vf->type & VIDTYPE_VIU_422) {
			if (vf->bitdepth & FULL_PACK_422_MODE)
				format = GE2D_FORMAT_S16_10BIT_YUV422
					| (GE2D_FORMAT_S16_10BIT_YUV422B
					& (3 << 3));
			else
				format = GE2D_FORMAT_S16_12BIT_YUV422
					| (GE2D_FORMAT_S16_12BIT_YUV422B
					& (3 << 3));
		}
		break;
	case DECODER_10BIT_TOP:
		if (vf->type & VIDTYPE_VIU_422) {
			if (vf->bitdepth & FULL_PACK_422_MODE)
				format = GE2D_FORMAT_S16_10BIT_YUV422
					| (GE2D_FORMAT_S16_10BIT_YUV422T
					& (3 << 3));
			else
				format = GE2D_FORMAT_S16_12BIT_YUV422
					| (GE2D_FORMAT_S16_12BIT_YUV422T
					& (3 << 3));
		}
		break;
	default:
		format = GE2D_FORMAT_M24_YUV420;
	}
	return format;
}

static void update_ge2d_num_cache(struct aml_v4l2_ge2d *ge2d)
{
	atomic_set(&ge2d->ctx->ge2d_cache_num,
		GE2D_FRAME_SIZE - kfifo_len(&ge2d->input));
}

static int v4l_ge2d_empty_input_done(struct aml_v4l2_ge2d_buf *buf)
{
	struct aml_v4l2_ge2d *ge2d = buf->caller_data;
	struct aml_buf *aml_buf = NULL;
	bool eos = false;

	if (!ge2d || !ge2d->ctx) {
		v4l_dbg(0, V4L_DEBUG_CODEC_ERROR,
			"fatal %s %d ge2d:%px\n",
			__func__, __LINE__, ge2d);
		return -1;
	}

	if (ge2d->ctx->is_stream_off) {
		v4l_dbg(ge2d->ctx, V4L_DEBUG_CODEC_EXINFO,
			"ge2d discard recycle frame %s %d ge2d:%p\n",
			__func__, __LINE__, ge2d);
		return -1;
	}

	aml_buf 	= buf->aml_vb->aml_buf;
	eos	= (buf->flag & GE2D_FLAG_EOS);

	v4l_dbg(ge2d->ctx, V4L_DEBUG_GE2D_BUFMGR,
		"ge2d_input done: vf:%px, idx: %d, flag(vf:%x ge2d:%x) %s, ts:%lld, "
		"in:%d, out:%d, vf:%d, in done:%d, out done:%d, ge2d_cache_num:%d\n",
		buf->vf,
		buf->vf->index,
		buf->vf->flag,
		buf->flag,
		eos ? "eos" : "",
		buf->vf->timestamp,
		kfifo_len(&ge2d->input),
		kfifo_len(&ge2d->output),
		kfifo_len(&ge2d->frame),
		kfifo_len(&ge2d->in_done_q),
		kfifo_len(&ge2d->out_done_q),
		atomic_read(&ge2d->ctx->ge2d_cache_num));

	aml_buf_fill(&ge2d->ctx->bm, aml_buf, BUF_USER_GE2D);

	kfifo_put(&ge2d->input, buf);
	update_ge2d_num_cache(ge2d);

	vdec_tracing(&ge2d->ctx->vtr, VTRACE_GE2D_PIC_6, atomic_read(&ge2d->ctx->ge2d_cache_num));
	vdec_tracing(&ge2d->ctx->vtr, VTRACE_GE2D_PIC_1, aml_buf->index);

	return 0;
}

static int v4l_ge2d_fill_output_done(struct aml_v4l2_ge2d_buf *buf)
{
	struct aml_v4l2_ge2d *ge2d = buf->caller_data;
	struct aml_buf *aml_buf = NULL;
	bool bypass = false;
	bool eos = false;

	if (!ge2d || !ge2d->ctx) {
		v4l_dbg(0, V4L_DEBUG_CODEC_ERROR,
			"fatal %s %d ge2d:%px\n",
			__func__, __LINE__, ge2d);
		return -1;
	}

	if (ge2d->ctx->is_stream_off) {
		v4l_dbg(ge2d->ctx, V4L_DEBUG_CODEC_EXINFO,
			"ge2d discard submit frame %s %d ge2d:%p\n",
			__func__, __LINE__, ge2d);
		return -1;
	}

	aml_buf	= buf->aml_vb->aml_buf;
	eos	= (buf->flag & GE2D_FLAG_EOS);
	bypass	= (buf->flag & GE2D_FLAG_BUF_BY_PASS);

	/* recovery aml_buf handle. */
	buf->vf->v4l_mem_handle = (ulong)aml_buf;

	kfifo_put(&ge2d->out_done_q, buf);

	if (aml_buf->ge2d_buf == NULL) {
		aml_buf->ge2d_buf = vzalloc(sizeof(struct aml_v4l2_ge2d_buf));
	}

	if (aml_buf->ge2d_buf)
		memcpy((struct aml_v4l2_ge2d_buf *)(aml_buf->ge2d_buf), buf,
			sizeof(struct aml_v4l2_ge2d_buf));

	v4l_dbg(ge2d->ctx, V4L_DEBUG_GE2D_BUFMGR,
		"ge2d_output done: vf:%px, idx:%d, flag(vf:%x ge2d:%x) %s, ts:%lld, "
		"in:%d, out:%d, vf:%d, in done:%d, out done:%d, wxh:%ux%u\n",
		buf->vf,
		buf->vf->index,
		buf->vf->flag,
		buf->flag,
		eos ? "eos" : "",
		buf->vf->timestamp,
		kfifo_len(&ge2d->input),
		kfifo_len(&ge2d->output),
		kfifo_len(&ge2d->frame),
		kfifo_len(&ge2d->in_done_q),
		kfifo_len(&ge2d->out_done_q),
		buf->vf->width, buf->vf->height);

	vdec_tracing(&ge2d->ctx->vtr, VTRACE_GE2D_PIC_4, aml_buf->index);

	aml_buf_done(&ge2d->ctx->bm, aml_buf, BUF_USER_GE2D);

	ge2d->out_num[OUTPUT_PORT]++;

	return 0;
}

static void ge2d_vf_get(void *caller, struct vframe_s *vf_out)
{
	struct aml_v4l2_ge2d *ge2d = (struct aml_v4l2_ge2d *)caller;
	struct aml_v4l2_ge2d_buf *buf = NULL;
	struct aml_buf *aml_buf = NULL;
	struct vframe_s *vf = NULL;
	bool bypass = false;
	bool eos = false;

	if (!ge2d || !ge2d->ctx) {
		v4l_dbg(0, V4L_DEBUG_CODEC_ERROR,
			"fatal %s %d ge2d:%px\n",
			__func__, __LINE__, ge2d);
		return;
	}

	if (kfifo_get(&ge2d->out_done_q, &buf)) {
		aml_buf	= buf->aml_vb->aml_buf;
		eos	= (buf->flag & GE2D_FLAG_EOS);
		bypass	= (buf->flag & GE2D_FLAG_BUF_BY_PASS);
		vf	= buf->vf;

		if (eos) {
			v4l_dbg(ge2d->ctx, V4L_DEBUG_GE2D_DETAIL,
				"%s %d got eos\n",
				__func__, __LINE__);
			vf->type |= VIDTYPE_V4L_EOS;
			vf->flag = VFRAME_FLAG_EMPTY_FRAME_V4L;
		}

		if (ge2d->ctx->enable_di_post) {
			if (is_cpu_t7()) {
				if (vf->canvas0_config[0].block_mode == CANVAS_BLKMODE_LINEAR) {
					if ((ge2d->ctx->output_pix_fmt != V4L2_PIX_FMT_H264) &&
						(ge2d->ctx->output_pix_fmt != V4L2_PIX_FMT_MPEG1) &&
						(ge2d->ctx->output_pix_fmt != V4L2_PIX_FMT_MPEG2) &&
						(ge2d->ctx->output_pix_fmt != V4L2_PIX_FMT_MPEG4) &&
						(ge2d->ctx->output_pix_fmt != V4L2_PIX_FMT_MJPEG)) {
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
		}

		memcpy(vf_out, vf, sizeof(struct vframe_s));

		mutex_lock(&ge2d->output_lock);
		kfifo_put(&ge2d->frame, vf);
		kfifo_put(&ge2d->output, buf);
		mutex_unlock(&ge2d->output_lock);
		up(&ge2d->sem_out);

		vdec_tracing(&ge2d->ctx->vtr, VTRACE_GE2D_PIC_5, aml_buf->index);

		v4l_dbg(ge2d->ctx, V4L_DEBUG_GE2D_BUFMGR,
			"%s: vf:%px, index:%d, flag(vf:%x ge2d:%x), ts:%lld, type:%x, wxh:%ux%u\n",
			__func__, vf,
			vf->index,
			vf->flag,
			buf->flag,
			vf->timestamp, vf->type, vf->width, vf->height);
	}
}

static void ge2d_vf_put(void *caller, struct vframe_s *vf)
{

}

static int aml_v4l2_ge2d_thread(void* param)
{
	struct aml_v4l2_ge2d* ge2d = param;
	struct aml_vcodec_ctx *ctx = ge2d->ctx;
	struct config_para_ex_s ge2d_config;
	u32 src_fmt = 0, dst_fmt = 0;
	struct canvas_s cd;
	ulong start_time;

	v4l_dbg(ctx, V4L_DEBUG_GE2D_DETAIL, "enter ge2d thread\n");
	while (ge2d->running) {
		struct aml_v4l2_ge2d_buf *in_buf;
		struct aml_v4l2_ge2d_buf *out_buf = NULL;
		struct vframe_s *vf_out = NULL;
		struct aml_buf *aml_buf;

		if (down_interruptible(&ge2d->sem_in))
			goto exit;
retry:
		if (!ge2d->running)
			break;

		if (kfifo_is_empty(&ge2d->output)) {
			if (down_interruptible(&ge2d->sem_out))
				goto exit;
			goto retry;
		}

		mutex_lock(&ge2d->output_lock);
		if (!kfifo_get(&ge2d->output, &out_buf)) {
			mutex_unlock(&ge2d->output_lock);
			v4l_dbg(ctx, 0, "ge2d can not get output\n");
			goto exit;
		}
		mutex_unlock(&ge2d->output_lock);

		aml_buf = aml_buf_get(&ctx->bm, BUF_USER_GE2D, false);
		if (!aml_buf) {
			usleep_range(5000, 5500);
			mutex_lock(&ge2d->output_lock);
			kfifo_put(&ge2d->output, out_buf);
			mutex_unlock(&ge2d->output_lock);
			goto retry;
		}

		out_buf->aml_vb =
			container_of(to_vb2_v4l2_buffer(aml_buf->vb), struct aml_v4l2_buf, vb);
		#if 0
		memcpy(&out_buf->aml_vb->ge2d_buf, out_buf,
					sizeof(struct aml_v4l2_ge2d_buf));
		#endif
		v4l_dbg(ctx, V4L_DEBUG_GE2D_BUFMGR,
			"ge2d bind buf:%d to ge2d_buf:%px\n",
			GE2D_BUF_GET_IDX(out_buf), out_buf);

		aml_buf->planes[0].bytes_used = aml_buf->planes[0].length;
		aml_buf->planes[1].bytes_used = aml_buf->planes[1].length;

		/* safe to pop in_buf */
		if (!kfifo_get(&ge2d->in_done_q, &in_buf)) {
			v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
				"ge2d can not get input\n");
			goto exit;
		}

		mutex_lock(&ge2d->output_lock);
		if (!kfifo_get(&ge2d->frame, &vf_out)) {
			mutex_unlock(&ge2d->output_lock);
			v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
				"ge2d can not get frame\n");
			goto exit;
		}
		mutex_unlock(&ge2d->output_lock);

		aml_buf->state = FB_ST_GE2D;


		/* fill output vframe information. */
		memcpy(vf_out, in_buf->vf, sizeof(*vf_out));
		memcpy(vf_out->canvas0_config,
			in_buf->vf->canvas0_config,
			2 * sizeof(struct canvas_config_s));

		vf_out->canvas0_config[0].phy_addr = aml_buf->planes[0].addr;
		if (aml_buf->num_planes == 1) {
			vf_out->canvas0_config[1].phy_addr =
				aml_buf->planes[0].addr + aml_buf->planes[0].offset;
			vf_out->canvas0_config[2].phy_addr =
				aml_buf->planes[0].addr + aml_buf->planes[0].offset
				+ (aml_buf->planes[0].offset >> 2);
		} else {
			vf_out->canvas0_config[1].phy_addr =
				aml_buf->planes[1].addr;
			vf_out->canvas0_config[2].phy_addr =
				aml_buf->planes[2].addr;
		}

		/* fill outbuf parms. */
		out_buf->vf		= vf_out;
		out_buf->flag		= 0;
		out_buf->caller_data	= ge2d;

		/* fill inbuf parms. */
		in_buf->caller_data	= ge2d;

		memset(&ge2d_config, 0, sizeof(ge2d_config));

		src_fmt = get_input_format(in_buf->vf);

		if (in_buf->vf->canvas0_config[0].endian == 7)
			src_fmt |= (is_cpu_t7c() ||
						(get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_S5) ||
						(get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_T5M) ||
						(get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_T3X)) ?
						GE2D_LITTLE_ENDIAN : GE2D_BIG_ENDIAN;
		else
			src_fmt |= (is_cpu_t7c() ||
						(get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_S5) ||
						(get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_T5M) ||
						(get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_T3X)) ?
						GE2D_BIG_ENDIAN : GE2D_LITTLE_ENDIAN;

		/* negotiate format of destination */
		dst_fmt = get_input_format(in_buf->vf);
		if (ge2d->work_mode & GE2D_MODE_CONVERT_NV12)
			dst_fmt |= GE2D_FORMAT_M24_NV12;
		else if (ge2d->work_mode & GE2D_MODE_CONVERT_NV21)
			dst_fmt |= GE2D_FORMAT_M24_NV21;

		if (ge2d->work_mode & GE2D_MODE_CONVERT_LE)
			dst_fmt |= GE2D_LITTLE_ENDIAN;
		else
			dst_fmt |= GE2D_BIG_ENDIAN;

		if ((dst_fmt & GE2D_COLOR_MAP_MASK) == GE2D_COLOR_MAP_NV12) {
			vf_out->type |= VIDTYPE_VIU_NV12;
			vf_out->type &= ~VIDTYPE_VIU_NV21;
		} else if ((dst_fmt & GE2D_COLOR_MAP_MASK) == GE2D_COLOR_MAP_NV21) {
			vf_out->type |= VIDTYPE_VIU_NV21;
			vf_out->type &= ~VIDTYPE_VIU_NV12;
		}
		if ((dst_fmt & GE2D_ENDIAN_MASK) == GE2D_LITTLE_ENDIAN) {
			vf_out->canvas0_config[0].endian = 0;
			vf_out->canvas0_config[1].endian = 0;
			vf_out->canvas0_config[2].endian = 0;
		} else if ((dst_fmt & GE2D_ENDIAN_MASK) == GE2D_BIG_ENDIAN){
			vf_out->canvas0_config[0].endian = 7;
			vf_out->canvas0_config[1].endian = 7;
			vf_out->canvas0_config[2].endian = 7;
		}

		vf_out->mem_sec = ctx->is_drm_mode ? 1 : 0;
		start_time = local_clock();

		mutex_lock(&ctx->dev->cache.lock);
		/* src canvas configure. */
		if ((in_buf->vf->canvas0Addr == 0) ||
			(in_buf->vf->canvas0Addr == (u32)-1)) {
			canvas_config_config(ctx->dev->cache.res[0].cid, &in_buf->vf->canvas0_config[0]);
			canvas_config_config(ctx->dev->cache.res[1].cid, &in_buf->vf->canvas0_config[1]);
			canvas_config_config(ctx->dev->cache.res[2].cid, &in_buf->vf->canvas0_config[2]);
			ge2d_config.src_para.canvas_index =
				ctx->dev->cache.res[0].cid |
				ctx->dev->cache.res[1].cid << 8 |
				ctx->dev->cache.res[2].cid << 16;

			ge2d_config.src_planes[0].addr =
				in_buf->vf->canvas0_config[0].phy_addr;
			ge2d_config.src_planes[0].w =
				in_buf->vf->canvas0_config[0].width;
			ge2d_config.src_planes[0].h =
				in_buf->vf->canvas0_config[0].height;
			ge2d_config.src_planes[1].addr =
				in_buf->vf->canvas0_config[1].phy_addr;
			ge2d_config.src_planes[1].w =
				in_buf->vf->canvas0_config[1].width;
			ge2d_config.src_planes[1].h =
				in_buf->vf->canvas0_config[1].height;
			ge2d_config.src_planes[2].addr =
				in_buf->vf->canvas0_config[2].phy_addr;
			ge2d_config.src_planes[2].w =
				in_buf->vf->canvas0_config[2].width;
			ge2d_config.src_planes[2].h =
				in_buf->vf->canvas0_config[2].height;
		} else {
			ge2d_config.src_para.canvas_index = in_buf->vf->canvas0Addr;
		}
		ge2d_config.src_para.mem_type	= CANVAS_TYPE_INVALID;
		ge2d_config.src_para.format	= src_fmt;
		ge2d_config.src_para.fill_color_en = 0;
		ge2d_config.src_para.fill_mode	= 0;
		ge2d_config.src_para.x_rev	= 0;
		ge2d_config.src_para.y_rev	= 0;
		ge2d_config.src_para.color	= 0xffffffff;
		ge2d_config.src_para.top	= 0;
		ge2d_config.src_para.left	= 0;
		ge2d_config.src_para.width	= in_buf->vf->width;
		if (in_buf->vf->type & VIDTYPE_INTERLACE)
			ge2d_config.src_para.height = in_buf->vf->height >> 1;
		else
			ge2d_config.src_para.height = in_buf->vf->height;

		/* dst canvas configure. */
		canvas_config_config(ctx->dev->cache.res[3].cid, &vf_out->canvas0_config[0]);
		if ((ge2d_config.src_para.format & 0xfffff) == GE2D_FORMAT_M24_YUV420) {
			vf_out->canvas0_config[1].width <<= 1;
		}
		canvas_config_config(ctx->dev->cache.res[4].cid, &vf_out->canvas0_config[1]);
		canvas_config_config(ctx->dev->cache.res[5].cid, &vf_out->canvas0_config[2]);
		ge2d_config.dst_para.canvas_index =
			ctx->dev->cache.res[3].cid |
			ctx->dev->cache.res[4].cid << 8;
		canvas_read(ctx->dev->cache.res[3].cid, &cd);
		ge2d_config.dst_planes[0].addr	= cd.addr;
		ge2d_config.dst_planes[0].w	= cd.width;
		ge2d_config.dst_planes[0].h	= cd.height;
		canvas_read(ctx->dev->cache.res[4].cid, &cd);
		ge2d_config.dst_planes[1].addr	= cd.addr;
		ge2d_config.dst_planes[1].w	= cd.width;
		ge2d_config.dst_planes[1].h	= cd.height;

		ge2d_config.dst_para.format	=  dst_fmt;
		ge2d_config.dst_para.width	= in_buf->vf->width;
		ge2d_config.dst_para.height	= in_buf->vf->height;
		ge2d_config.dst_para.mem_type	= CANVAS_TYPE_INVALID;
		ge2d_config.dst_para.fill_color_en = 0;
		ge2d_config.dst_para.fill_mode	= 0;
		ge2d_config.dst_para.x_rev	= 0;
		ge2d_config.dst_para.y_rev	= 0;
		ge2d_config.dst_para.color	= 0;
		ge2d_config.dst_para.top	= 0;
		ge2d_config.dst_para.left	= 0;

		/* other ge2d parameters configure. */
		ge2d_config.src_key.key_enable	= 0;
		ge2d_config.src_key.key_mask	= 0;
		ge2d_config.src_key.key_mode	= 0;
		ge2d_config.alu_const_color	= 0;
		ge2d_config.bitmask_en		= 0;
		ge2d_config.src1_gb_alpha	= 0;
		ge2d_config.dst_xy_swap		= 0;
		ge2d_config.src2_para.mem_type	= CANVAS_TYPE_INVALID;
		ge2d_config.mem_sec	= ctx->is_drm_mode ? 1 : 0;

		vdec_tracing(&ge2d->ctx->vtr, VTRACE_GE2D_PIC_3,
			in_buf->aml_vb->aml_buf->index);

		v4l_dbg(ctx, V4L_DEBUG_GE2D_BUFMGR,
			"ge2d_handle start: dec vf:%px/%d, ge2d vf:%px/%d, iphy:%lx/%lx %dx%d ophy:%lx/%lx %dx%d, vf:%ux%u, fmt(src:%x, dst:%x), "
			"in:%d, out:%d, vf:%d, in done:%d, out done:%d\n",
			in_buf->vf, in_buf->vf->index,
			out_buf->vf, GE2D_BUF_GET_IDX(out_buf),
			in_buf->vf->canvas0_config[0].phy_addr,
			in_buf->vf->canvas0_config[1].phy_addr,
			in_buf->vf->canvas0_config[0].width,
			in_buf->vf->canvas0_config[0].height,
			vf_out->canvas0_config[0].phy_addr,
			vf_out->canvas0_config[1].phy_addr,
			vf_out->canvas0_config[0].width,
			vf_out->canvas0_config[0].height,
			vf_out->width, vf_out->height,
			src_fmt, dst_fmt,
			kfifo_len(&ge2d->input),
			kfifo_len(&ge2d->output),
			kfifo_len(&ge2d->frame),
			kfifo_len(&ge2d->in_done_q),
			kfifo_len(&ge2d->out_done_q));

		if (ge2d_context_config_ex(ge2d->ge2d_context, &ge2d_config) < 0) {
			v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
				"ge2d_context_config_ex error.\n");
			mutex_unlock(&ctx->dev->cache.lock);
			goto exit;
		}

		if (!(in_buf->flag & GE2D_FLAG_EOS)) {
			if (in_buf->vf->type & VIDTYPE_INTERLACE) {
				stretchblt_noalpha(ge2d->ge2d_context,
					0, 0, in_buf->vf->width, in_buf->vf->height / 2,
					0, 0, in_buf->vf->width, in_buf->vf->height);
			} else {
				stretchblt_noalpha(ge2d->ge2d_context,
					0, 0, in_buf->vf->width, in_buf->vf->height,
					0, 0, in_buf->vf->width, in_buf->vf->height);
			}
		}
		mutex_unlock(&ctx->dev->cache.lock);

		//pr_info("consume time %d us\n", div64_u64(local_clock() - start_time, 1000));

		v4l_ge2d_fill_output_done(out_buf);
		v4l_ge2d_empty_input_done(in_buf);

		ge2d->in_num[INPUT_PORT]++;
		ge2d->out_num[INPUT_PORT]++;
	}
exit:
	while (!kthread_should_stop()) {
		usleep_range(1000, 2000);
	}

	v4l_dbg(ctx, V4L_DEBUG_GE2D_DETAIL, "exit ge2d thread\n");

	return 0;
}

int aml_v4l2_ge2d_get_buf_num(u32 mode)
{
	return 4;
}

void static inline aml_v4l2_ge2d_set_workmode(struct aml_vcodec_ctx *ctx,
		struct aml_ge2d_cfg_infos *cfg)
{
	if ((ctx->cap_pix_fmt == V4L2_PIX_FMT_NV12) ||
		(ctx->cap_pix_fmt == V4L2_PIX_FMT_NV12M))
		cfg->mode |= GE2D_MODE_CONVERT_NV12;
	else if ((ctx->cap_pix_fmt == V4L2_PIX_FMT_NV21) ||
		(ctx->cap_pix_fmt == V4L2_PIX_FMT_NV21M))
		cfg->mode |= GE2D_MODE_CONVERT_NV21;
	else
		cfg->mode |= GE2D_MODE_CONVERT_NV21;
}

int aml_v4l2_ge2d_init(
		struct aml_vcodec_ctx *ctx,
		struct aml_ge2d_cfg_infos *cfg,
		struct aml_v4l2_ge2d** ge2d_handle)
{
	struct sched_param param = { .sched_priority = MAX_RT_PRIO - 1 };
	struct aml_v4l2_ge2d *ge2d;
	u32 buf_size;
	int i, ret;

	if (!cfg || !ge2d_handle)
		return -EINVAL;

	ge2d = kzalloc(sizeof(*ge2d), GFP_KERNEL);
	if (!ge2d)
		return -ENOMEM;

	aml_v4l2_ge2d_set_workmode(ctx, cfg);
	ge2d->work_mode = cfg->mode;

	/* default convert little endian. */
	if (!ge2d->work_mode) {
		ge2d->work_mode = GE2D_MODE_CONVERT_LE;
	}

	ge2d->ge2d_context = create_ge2d_work_queue();
	if (!ge2d->ge2d_context) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"ge2d_create_instance fail\n");
		ret = -EINVAL;
		goto error;
	}

	INIT_KFIFO(ge2d->input);
	INIT_KFIFO(ge2d->output);
	INIT_KFIFO(ge2d->frame);
	INIT_KFIFO(ge2d->out_done_q);
	INIT_KFIFO(ge2d->in_done_q);

	ge2d->ctx = ctx;
	buf_size = cfg->buf_size;
	ge2d->buf_size = buf_size;

	/* setup output fifo */
	ret = kfifo_alloc(&ge2d->output, buf_size, GFP_KERNEL);
	if (ret) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"alloc output fifo fail.\n");
		ret = -ENOMEM;
		goto error2;
	}

	ge2d->ovbpool = vzalloc(buf_size * sizeof(*ge2d->ovbpool));
	if (!ge2d->ovbpool) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"alloc output vb pool fail.\n");
		ret = -ENOMEM;
		goto error3;
	}

	/* setup vframe fifo */
	ret = kfifo_alloc(&ge2d->frame, buf_size, GFP_KERNEL);
	if (ret) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"alloc ge2d vframe fifo fail.\n");
		ret = -ENOMEM;
		goto error4;
	}

	ge2d->vfpool = vzalloc(buf_size * sizeof(*ge2d->vfpool));
	if (!ge2d->vfpool) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"alloc vf pool fail.\n");
		ret = -ENOMEM;
		goto error5;
	}

	ret = kfifo_alloc(&ge2d->input, GE2D_FRAME_SIZE, GFP_KERNEL);
	if (ret) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"alloc input fifo fail.\n");
		ret = -ENOMEM;
		goto error6;
	}

	ge2d->ivbpool = vzalloc(GE2D_FRAME_SIZE * sizeof(*ge2d->ivbpool));
	if (!ge2d->ivbpool) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"alloc input vb pool fail.\n");
		ret = -ENOMEM;
		goto error7;
	}

	for (i = 0 ; i < GE2D_FRAME_SIZE ; i++) {
		kfifo_put(&ge2d->input, &ge2d->ivbpool[i]);
	}

	for (i = 0 ; i < buf_size ; i++) {
		kfifo_put(&ge2d->output, &ge2d->ovbpool[i]);
		kfifo_put(&ge2d->frame, &ge2d->vfpool[i]);
	}

	if (aml_canvas_cache_get(ctx->dev, "v4ldec-ge2d") < 0) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"canvas pool alloc fail. src(%d, %d, %d) dst(%d, %d, %d).\n",
			ctx->dev->cache.res[0].cid,
			ctx->dev->cache.res[1].cid,
			ctx->dev->cache.res[2].cid,
			ctx->dev->cache.res[3].cid,
			ctx->dev->cache.res[4].cid,
			ctx->dev->cache.res[5].cid);
		goto error8;
	}

	mutex_init(&ge2d->output_lock);
	sema_init(&ge2d->sem_in, 0);
	sema_init(&ge2d->sem_out, 0);

	ge2d->running = true;
	ge2d->task = kthread_run(aml_v4l2_ge2d_thread, ge2d,
		"%s", "aml-v4l2-ge2d");
	if (IS_ERR(ge2d->task)) {
		ret = PTR_ERR(ge2d->task);
		goto error9;
	}
	sched_setscheduler_nocheck(ge2d->task, SCHED_FIFO, &param);

	*ge2d_handle = ge2d;

	v4l_dbg(ctx, V4L_DEBUG_CODEC_PRINFO,
		"GE2D_CFG bsize:%d, wkm:%x, bm:%x, drm:%d\n",
		ge2d->buf_size,
		ge2d->work_mode,
		ge2d->buffer_mode,
		cfg->is_drm);

	return 0;

error9:
	aml_canvas_cache_put(ctx->dev);
error8:
	vfree(ge2d->ivbpool);
error7:
	kfifo_free(&ge2d->input);
error6:
	vfree(ge2d->vfpool);
error5:
	kfifo_free(&ge2d->frame);
error4:
	vfree(ge2d->ovbpool);
error3:
	kfifo_free(&ge2d->output);
error2:
	destroy_ge2d_work_queue(ge2d->ge2d_context);
error:
	kfree(ge2d);

	return ret;
}
EXPORT_SYMBOL(aml_v4l2_ge2d_init);

int aml_v4l2_ge2d_destroy(struct aml_v4l2_ge2d* ge2d)
{
	v4l_dbg(ge2d->ctx, V4L_DEBUG_GE2D_DETAIL,
		"ge2d destroy begin\n");

	if (ge2d->running) {
		ge2d->running = false;
		up(&ge2d->sem_in);
		up(&ge2d->sem_out);
		kthread_stop(ge2d->task);
	}
	destroy_ge2d_work_queue(ge2d->ge2d_context);
	/* no more ge2d callback below this line */

	kfifo_free(&ge2d->frame);
	vfree(ge2d->vfpool);
	kfifo_free(&ge2d->output);
	vfree(ge2d->ovbpool);
	kfifo_free(&ge2d->input);
	vfree(ge2d->ivbpool);
	mutex_destroy(&ge2d->output_lock);

	aml_canvas_cache_put(ge2d->ctx->dev);

	v4l_dbg(ge2d->ctx, V4L_DEBUG_GE2D_DETAIL,
		"ge2d destroy done\n");

	kfree(ge2d);

	return 0;
}
EXPORT_SYMBOL(aml_v4l2_ge2d_destroy);

int aml_v4l2_ge2d_thread_stop(struct aml_v4l2_ge2d* ge2d)
{
	v4l_dbg(ge2d->ctx, V4L_DEBUG_GE2D_DETAIL,
		"ge2d thread stop begin\n");

	if (ge2d->running) {
		ge2d->running = false;
		up(&ge2d->sem_in);
		up(&ge2d->sem_out);
		kthread_stop(ge2d->task);
	}
	v4l_dbg(ge2d->ctx, V4L_DEBUG_GE2D_DETAIL,
		"ge2d thread stop done\n");

	return 0;
}
EXPORT_SYMBOL(aml_v4l2_ge2d_thread_stop);

static int aml_v4l2_ge2d_push_vframe(struct aml_v4l2_ge2d* ge2d, struct vframe_s *vf)
{
	struct aml_v4l2_ge2d_buf* in_buf;
	struct aml_buf *aml_buf = NULL;

	if (!ge2d)
		return -EINVAL;

	if (!kfifo_get(&ge2d->input, &in_buf)) {
		v4l_dbg(ge2d->ctx, V4L_DEBUG_CODEC_ERROR,
			"cat not get free input buffer.\n");
		return -1;
	}

	if (vf->type & VIDTYPE_V4L_EOS)
		in_buf->flag |= GE2D_FLAG_EOS;

	v4l_dbg(ge2d->ctx, V4L_DEBUG_GE2D_BUFMGR,
		"ge2d_push_vframe: vf:%px, idx:%d, type:%x, ts:%lld\n",
		vf, vf->index, vf->type, vf->timestamp);

	aml_buf = (struct aml_buf *)vf->v4l_mem_handle;
	in_buf->aml_vb = container_of(to_vb2_v4l2_buffer(aml_buf->vb), struct aml_v4l2_buf, vb);
	in_buf->vf = vf;

	do {
		unsigned int dw_mode = DM_YUV_ONLY;
		struct file *fp;
		char file_name[64] = {0};

		if (!dump_ge2d_input || ge2d->ctx->is_drm_mode)
			break;

		if (vdec_if_get_param(ge2d->ctx, GET_PARAM_DW_MODE, &dw_mode))
			break;

		if (dw_mode == DM_AVBC_ONLY)
			break;

		snprintf(file_name, 64, "%s/dec_dump_ge2d_input_%ux%u.raw", dump_path, vf->width, vf->height);
		fp = media_open(file_name, O_CREAT | O_RDWR | O_LARGEFILE | O_APPEND, 0600);
		if (!IS_ERR(fp)) {
			struct vb2_buffer *vb = &in_buf->aml_vb->vb.vb2_buf;

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

			pr_info("dump idx: %d %dx%d num_planes %d\n",
				dump_ge2d_input,
				vf->width,
				vf->height,
				vb->num_planes);

			dump_ge2d_input--;
			media_close(fp, NULL);
		}
	} while(0);

	vdec_tracing(&ge2d->ctx->vtr, VTRACE_GE2D_PIC_2, aml_buf->index);

	kfifo_put(&ge2d->in_done_q, in_buf);
	update_ge2d_num_cache(ge2d);
	vdec_tracing(&ge2d->ctx->vtr, VTRACE_GE2D_PIC_6, atomic_read(&ge2d->ctx->ge2d_cache_num));
	aml_buf_update_holder(&ge2d->ctx->bm, aml_buf, BUF_USER_GE2D, BUF_GET);
	up(&ge2d->sem_in);

	return 0;
}

static void fill_ge2d_buf_cb(void *v4l_ctx, void *fb_ctx)
{
	struct aml_vcodec_ctx *ctx =
		(struct aml_vcodec_ctx *)v4l_ctx;
	struct aml_buf *aml_buf =
		(struct aml_buf *)fb_ctx;
	int ret = -1;

	ret = aml_v4l2_ge2d_push_vframe(ctx->ge2d, &aml_buf->vframe);
	if (ret < 0) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"ge2d push vframe err, ret: %d\n", ret);
	}
}

static struct task_ops_s ge2d_ops = {
	.type		= TASK_TYPE_GE2D,
	.get_vframe	= ge2d_vf_get,
	.put_vframe	= ge2d_vf_put,
	.fill_buffer	= fill_ge2d_buf_cb,
};

struct task_ops_s *get_ge2d_ops(void)
{
	return &ge2d_ops;
}
EXPORT_SYMBOL(get_ge2d_ops);

