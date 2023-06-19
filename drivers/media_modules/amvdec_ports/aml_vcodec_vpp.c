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
#include <uapi/linux/sched/types.h>
#include <linux/amlogic/meson_uvm_core.h>

#include "aml_vcodec_vpp.h"
#include "aml_vcodec_adapt.h"
#include "vdec_drv_if.h"
#include "../common/chips/decoder_cpu_ver_info.h"
#include "../frame_provider/decoder/utils/aml_buf_helper.h"
#include "utils/common.h"
#include "../common/media_utils/media_utils.h"

#define KERNEL_ATRACE_TAG KERNEL_ATRACE_TAG_V4L2
#include <trace/events/meson_atrace.h>

#define VPP_BUF_GET_IDX(vpp_buf) (vpp_buf->aml_vb->vb.vb2_buf.index)
#define INPUT_PORT 0
#define OUTPUT_PORT 1

extern int dump_vpp_input;
extern int vpp_bypass_frames;
extern char dump_path[32];

struct di_buffer_wrap {
	struct vframe_s vf_cp;
	struct di_buffer *buf;
};

static void di_release_keep_buf_wrap1(void *arg)
{
	struct di_buffer *buf = (struct di_buffer *)arg;

	v4l_dbg(0, V4L_DEBUG_VPP_BUFMGR,
		"%s release di local buffer %px, vf:%px, comm:%s, pid:%d\n",
		__func__ , buf, buf->vf,
		current->comm, current->pid);

	di_release_keep_buf(buf);

	ATRACE_COUNTER("VC_OUT_VPP_LC-1.lc_release", buf->mng.index);
}

static void di_release_keep_buf_wrap2(void *arg)
{
	struct di_buffer_wrap *buf_wrap = (struct di_buffer_wrap *)arg;
	struct di_buffer *buf = NULL;
	struct vframe_s *vf_cp = NULL;
	s32 cur_di_ins_id = -1, last_di_ins_id = -1;

	if (buf_wrap) {
		buf = buf_wrap->buf;
		vf_cp = &buf_wrap->vf_cp;
		if (buf && buf->vf)
			cur_di_ins_id = buf->vf->di_instance_id;
		last_di_ins_id = vf_cp->di_instance_id;
	}

	v4l_dbg(0, V4L_DEBUG_VPP_BUFMGR,
		"%s release di local buffer %px, vf:%px, cur_id: %d vf_cp:%px, vf-ext:%px, vf_cp-ext:%px, last_id: %d comm:%s, pid:%d,buf->flag=%d\n",
		__func__ , buf, buf->vf, cur_di_ins_id, vf_cp, buf->vf ? buf->vf->vf_ext : NULL, vf_cp->vf_ext, last_di_ins_id,
		current->comm, current->pid, buf->flag);

	/* just release di bufffer when same id or interlace video or post write mode */
	if ((cur_di_ins_id == last_di_ins_id && cur_di_ins_id > 0) ||
		(buf->flag & DI_FLAG_I) ||
	    !(vf_cp->di_flag & DI_FLAG_DI_PVPPLINK))
		di_release_keep_buf(buf);
	else
		v4l_dbg(0, V4L_DEBUG_VPP_BUFMGR,"%s release warning\n",__func__);

	if (buf_wrap)
		vfree(buf_wrap);

	ATRACE_COUNTER("VC_OUT_VPP_LC-2.lc_release", buf->mng.index);
}

static int attach_DI_buffer(struct aml_v4l2_vpp_buf *vpp_buf)
{
	struct aml_v4l2_vpp *vpp = vpp_buf->di_buf.caller_data;
	struct dma_buf *dma = NULL;
	struct aml_v4l2_buf *aml_vb = NULL;
	struct uvm_hook_mod_info u_info;
	struct di_buffer_wrap *buf_wrap = NULL;
	int ret;

	aml_vb = vpp_buf->aml_vb;
	if (!aml_vb)
		return -EINVAL;

	dma = aml_vb->vb.vb2_buf.planes[0].dbuf;
	if (!dma || !dmabuf_is_uvm(dma)) {
		v4l_dbg(vpp->ctx, V4L_DEBUG_CODEC_ERROR,
			"attach_DI_buffer err\n");
		return -EINVAL;
	}

	if (!vpp_buf->di_local_buf) {
		v4l_dbg(vpp->ctx, V4L_DEBUG_VPP_BUFMGR,
			"attach_DI_buffer nothing\n");
		return 0;
	}

	if (uvm_get_hook_mod(dma, VF_PROCESS_DI)) {
		uvm_put_hook_mod(dma, VF_PROCESS_DI);
		v4l_dbg(vpp->ctx, V4L_DEBUG_CODEC_ERROR,
			"attach_DI_buffer exist hook, dbuf:%px\n", dma);
		return -EINVAL;
	}
	buf_wrap = vzalloc(sizeof(struct di_buffer_wrap));
	if (buf_wrap) {
		buf_wrap->buf = vpp_buf->di_local_buf;
		u_info.type = VF_PROCESS_DI;
		u_info.arg = (void *)buf_wrap;
		u_info.free = di_release_keep_buf_wrap2;

		ret = uvm_attach_hook_mod(dma, &u_info);
		if (ret < 0) {
			v4l_dbg(vpp->ctx, V4L_DEBUG_CODEC_ERROR,
				"fail to set dmabuf DI hook\n");
			vfree(buf_wrap);
		} else if (vpp_buf->di_buf.vf->vf_ext) {
			v4l_dbg(vpp->ctx, V4L_DEBUG_VPP_BUFMGR,
				"%s swap di_buf %px.vf->vf_ext:%px to buf_wrap->vf_cp:%px\n",
				__func__, vpp_buf->di_buf.vf, vpp_buf->di_buf.vf->vf_ext, &buf_wrap->vf_cp);
			memcpy(&buf_wrap->vf_cp,
				vpp_buf->di_buf.vf->vf_ext,
				sizeof(struct vframe_s));
			vpp_buf->di_buf.vf->vf_ext = &buf_wrap->vf_cp;
		}
	}

	vdec_tracing(&vpp->ctx->vtr, VTRACE_VPP_PIC_12, vpp_buf->di_local_buf->mng.index);

	v4l_dbg(vpp->ctx, V4L_DEBUG_VPP_BUFMGR,
		"%s attach di local buffer %px, dbuf:%px\n",
		__func__ , vpp_buf->di_local_buf, dma);

	return ret;
}

static int detach_DI_buffer(struct aml_v4l2_vpp_buf *vpp_buf)
{
	struct aml_v4l2_vpp *vpp = vpp_buf->di_buf.caller_data;
	struct dma_buf *dma = NULL;
	struct aml_v4l2_buf *aml_vb = NULL;
	int ret;

	aml_vb = vpp_buf->aml_vb;
	if (!aml_vb)
		return -EINVAL;
	dma = aml_vb->vb.vb2_buf.planes[0].dbuf;
	if (IS_ERR_OR_NULL(dma) || !dmabuf_is_uvm(dma)) {
		v4l_dbg(vpp->ctx, V4L_DEBUG_CODEC_ERROR,
			"detach_DI_buffer err\n");
		return -EINVAL;
	}

	if (!vpp_buf->di_local_buf) {
		v4l_dbg(vpp->ctx, V4L_DEBUG_VPP_BUFMGR,
			"detach_DI_buffer nothing\n");
		return 0;
	}

	vdec_tracing(&vpp->ctx->vtr, VTRACE_VPP_PIC_13, vpp_buf->di_local_buf->mng.index);

	ret = uvm_detach_hook_mod(dma, VF_PROCESS_DI);
	if (ret < 0) {
		v4l_dbg(vpp->ctx, V4L_DEBUG_VPP_BUFMGR,
			"fail to remove dmabuf DI hook\n");
	}

	v4l_dbg(vpp->ctx, V4L_DEBUG_VPP_BUFMGR,
		"%s detach di local buffer %px, dbuf:%px\n",
		__func__ , vpp_buf->di_local_buf, dma);

	return ret;
}

static void release_DI_buff(struct aml_v4l2_vpp* vpp)
{
	struct aml_v4l2_vpp_buf *vpp_buf = NULL;

	while (kfifo_get(&vpp->out_done_q, &vpp_buf)) {
		if (vpp_buf->di_buf.private_data) {
			di_release_keep_buf(vpp_buf->di_local_buf);
			v4l_dbg(vpp->ctx, V4L_DEBUG_VPP_BUFMGR,
				"%s release di local buffer %px\n",
				__func__ , vpp_buf->di_local_buf);
		}
	}
}

static void update_vpp_num_cache(struct aml_v4l2_vpp *vpp)
{
	if (!vpp->ctx->is_stream_off)
		atomic_set(&vpp->ctx->vpp_cache_num, VPP_FRAME_SIZE - kfifo_len(&vpp->input));
}

static int is_di_input_buff_full(struct aml_v4l2_vpp *vpp)
{
	return ((vpp->in_num[INPUT_PORT] - vpp->in_num[OUTPUT_PORT])
		> vpp->di_ibuf_num) ? true : false;
}

static int is_di_output_buff_full(struct aml_v4l2_vpp *vpp)
{
	return ((vpp->out_num[INPUT_PORT] - vpp->out_num[OUTPUT_PORT])
		> vpp->di_obuf_num) ? true : false;
}

static enum DI_ERRORTYPE
	v4l_vpp_fill_output_done_alloc_buffer(struct di_buffer *buf)
{
	struct aml_v4l2_vpp *vpp = buf->caller_data;
	struct aml_v4l2_vpp_buf *vpp_buf = NULL;
	struct aml_buf *aml_buf = NULL;
	bool bypass = false;
	bool eos = false;
	struct vframe_s *tmp;

	if (!vpp || !vpp->ctx) {
		pr_err("fatal %s %d vpp:%p\n",
			__func__, __LINE__, vpp);
		di_release_keep_buf_wrap1(buf);
		return DI_ERR_UNDEFINED;
	}

	if (vpp->ctx->is_stream_off) {
		v4l_dbg(vpp->ctx, V4L_DEBUG_CODEC_EXINFO,
			"vpp discard submit frame %s %d vpp:%p\n",
			__func__, __LINE__, vpp);
		di_release_keep_buf_wrap1(buf);
		return DI_ERR_UNDEFINED;
	}

	if (!kfifo_get(&vpp->processing, &vpp_buf)) {
		v4l_dbg(vpp->ctx, V4L_DEBUG_CODEC_EXINFO,
			"vpp doesn't get output %s %d vpp:%p\n",
			__func__, __LINE__, vpp);
		di_release_keep_buf_wrap1(buf);
		return DI_ERR_UNDEFINED;
	}

	aml_buf	= vpp_buf->aml_vb->aml_buf;
	eos	= (buf->flag & DI_FLAG_EOS);
	bypass	= (buf->flag & DI_FLAG_BUF_BY_PASS);

	vpp_buf->di_buf.vf->timestamp	= buf->vf->timestamp;
	vpp_buf->di_buf.private_data	= buf->private_data;
	vpp_buf->di_buf.vf->vf_ext	= buf->vf;
	vpp_buf->di_buf.flag		= buf->flag;
	vpp_buf->di_buf.vf->v4l_mem_handle = (ulong)aml_buf;

	if (!eos && !bypass) {
		vpp_buf->di_local_buf = buf;
		vpp_buf->di_buf.vf->vf_ext = buf->vf;
		vpp_buf->di_buf.vf->flag |= VFRAME_FLAG_CONTAIN_POST_FRAME;
		atomic_set(&vpp->local_buf_out, 1);
	}

	kfifo_put(&vpp->out_done_q, vpp_buf);

	if (vpp->is_prog) {
		kfifo_put(&vpp->input, vpp_buf->inbuf);
		update_vpp_num_cache(vpp);
		vdec_tracing(&vpp->ctx->vtr, VTRACE_VPP_PIC_15, atomic_read(&vpp->ctx->vpp_cache_num));
	}

	if (aml_buf->vpp_buf == NULL) {
		aml_buf->vpp_buf = vzalloc(sizeof(struct aml_v4l2_vpp_buf));
	}

	if (aml_buf->vpp_buf)
		memcpy((struct aml_v4l2_vpp_buf *)(aml_buf->vpp_buf), vpp_buf,
			sizeof(struct aml_v4l2_vpp_buf));

	tmp = (struct vframe_s *)(buf->vf ? buf->vf->vf_ext : NULL);
	v4l_dbg(vpp->ctx, V4L_DEBUG_VPP_BUFMGR,
		"vpp_output local done: idx:%d, vf:%px, afbc:0x%lx ext vf:%px, idx:%d, flag(vf:%x di:%x) (buf->vf:%px vf_ext:%px, afbc:0x%lx) %s %s, ts:%lld, "
		"in:%d, out:%d, vf:%d, in done:%d, out done:%d\n",
		aml_buf->index,
		vpp_buf->di_buf.vf,
		vpp_buf->di_buf.vf->compHeadAddr,
		vpp_buf->di_buf.vf->vf_ext,
		vpp_buf->di_buf.vf->index,
		vpp_buf->di_buf.vf->flag,
		buf->flag,
		buf->vf,
		buf->vf ? buf->vf->vf_ext : NULL,
		tmp ? tmp->compHeadAddr : 0,
		vpp->is_prog ? "P" : "I",
		eos ? "eos" : "",
		vpp_buf->di_buf.vf->timestamp,
		kfifo_len(&vpp->input),
		kfifo_len(&vpp->output),
		kfifo_len(&vpp->frame),
		kfifo_len(&vpp->in_done_q),
		kfifo_len(&vpp->out_done_q));

	vdec_tracing(&vpp->ctx->vtr, VTRACE_VPP_PIC_10, aml_buf->index);

	aml_buf_done(&vpp->ctx->bm, aml_buf, BUF_USER_VPP);

	vpp->out_num[OUTPUT_PORT]++;
	vpp->in_num[OUTPUT_PORT]++;

	return DI_ERR_NONE;
}

static enum DI_ERRORTYPE
	v4l_vpp_empty_input_done(struct di_buffer *buf)
{
	struct aml_v4l2_vpp *vpp = buf->caller_data;
	struct aml_v4l2_vpp_buf *vpp_buf;
	struct aml_buf *aml_buf = NULL;
	bool eos = false;

	if (!vpp || !vpp->ctx) {
		v4l_dbg(0, V4L_DEBUG_CODEC_ERROR,
			"fatal %s %d vpp:%px\n",
			__func__, __LINE__, vpp);
		return DI_ERR_UNDEFINED;
	}

	if (vpp->ctx->is_stream_off) {
		v4l_dbg(vpp->ctx, V4L_DEBUG_CODEC_EXINFO,
			"vpp discard recycle frame %s %d vpp:%p\n",
			__func__, __LINE__, vpp);
		return DI_ERR_UNDEFINED;
	}

	vpp_buf	= container_of(buf, struct aml_v4l2_vpp_buf, di_buf);
	aml_buf 	= vpp_buf->aml_vb->aml_buf;
	eos	= (buf->flag & DI_FLAG_EOS);

	v4l_dbg(vpp->ctx, V4L_DEBUG_VPP_BUFMGR,
		"vpp_input done: idx:%d, vf:%px, idx: %d, flag(vf:%x di:%x) %s %s, ts:%lld, "
		"in:%d, out:%d, vf:%d, in done:%d, out done:%d\n",
		aml_buf->index,
		buf->vf,
		buf->vf->index,
		buf->vf->flag,
		buf->flag,
		vpp->is_prog ? "P" : "I",
		eos ? "eos" : "",
		buf->vf->timestamp,
		kfifo_len(&vpp->input),
		kfifo_len(&vpp->output),
		kfifo_len(&vpp->frame),
		kfifo_len(&vpp->in_done_q),
		kfifo_len(&vpp->out_done_q));

	if (!vpp->is_prog) {
		/* recycle vf only in non-bypass mode */
		aml_buf_fill(&vpp->ctx->bm, aml_buf, BUF_USER_VPP);

		kfifo_put(&vpp->input, vpp_buf);
		update_vpp_num_cache(vpp);
		vdec_tracing(&vpp->ctx->vtr, VTRACE_VPP_PIC_15, atomic_read(&vpp->ctx->vpp_cache_num));
	}

	if (vpp->work_mode == VPP_MODE_S4_DW_MMU) {
		kfifo_put(&vpp->input, vpp_buf);
		update_vpp_num_cache(vpp);
		vdec_tracing(&vpp->ctx->vtr, VTRACE_VPP_PIC_15, atomic_read(&vpp->ctx->vpp_cache_num));
	}

	if (vpp->buffer_mode != BUFFER_MODE_ALLOC_BUF)
		vpp->in_num[OUTPUT_PORT]++;

	vdec_tracing(&vpp->ctx->vtr, VTRACE_VPP_PIC_1, aml_buf->index);

	return DI_ERR_NONE;
}

static enum DI_ERRORTYPE
	v4l_vpp_fill_output_done(struct di_buffer *buf)
{
	struct aml_v4l2_vpp *vpp = buf->caller_data;
	struct aml_v4l2_vpp_buf *vpp_buf = NULL;
	struct aml_buf *aml_buf = NULL;
	bool bypass = false;
	bool eos = false;

	if (!vpp || !vpp->ctx) {
		v4l_dbg(0, V4L_DEBUG_CODEC_ERROR,
			"fatal %s %d vpp:%px\n",
			__func__, __LINE__, vpp);
		return DI_ERR_UNDEFINED;
	}

	if (vpp->ctx->is_stream_off) {
		v4l_dbg(vpp->ctx, V4L_DEBUG_CODEC_EXINFO,
			"vpp discard submit frame %s %d vpp:%p\n",
			__func__, __LINE__, vpp);
		return DI_ERR_UNDEFINED;
	}

	vpp_buf	= container_of(buf, struct aml_v4l2_vpp_buf, di_buf);
	aml_buf	= vpp_buf->aml_vb->aml_buf;
	eos	= (buf->flag & DI_FLAG_EOS);
	bypass	= (buf->flag & DI_FLAG_BUF_BY_PASS);

	/* recovery aml_buf handle. */
	buf->vf->v4l_mem_handle = (ulong)aml_buf;

	kfifo_put(&vpp->out_done_q, vpp_buf);

	if (aml_buf->vpp_buf == NULL) {
		aml_buf->vpp_buf = vzalloc(sizeof(struct aml_v4l2_vpp_buf));
	}

	if (aml_buf->vpp_buf)
		memcpy((struct aml_v4l2_vpp_buf *)(aml_buf->vpp_buf), vpp_buf,
			sizeof(struct aml_v4l2_vpp_buf));

	v4l_dbg(vpp->ctx, V4L_DEBUG_VPP_BUFMGR,
		"vpp_output done: idx:%d, vf:%px, idx:%d, flag(vf:%x di:%x) %s %s, ts:%lld, "
		"in:%d, out:%d, vf:%d, in done:%d, out done:%d\n",
		aml_buf->index,
		buf->vf,
		buf->vf->index,
		buf->vf->flag,
		buf->flag,
		vpp->is_prog ? "P" : "I",
		eos ? "eos" : "",
		buf->vf->timestamp,
		kfifo_len(&vpp->input),
		kfifo_len(&vpp->output),
		kfifo_len(&vpp->frame),
		kfifo_len(&vpp->in_done_q),
		kfifo_len(&vpp->out_done_q));

	vdec_tracing(&vpp->ctx->vtr, VTRACE_VPP_PIC_9, aml_buf->index);

	aml_buf_done(&vpp->ctx->bm, aml_buf, BUF_USER_VPP);

	vpp->out_num[OUTPUT_PORT]++;

	/* count for bypass nr */
	if (vpp->buffer_mode == BUFFER_MODE_ALLOC_BUF)
		vpp->in_num[OUTPUT_PORT]++;

	return DI_ERR_NONE;
}

static enum DI_ERRORTYPE
	v4l_vpp_fill_output_done_dw_mmu(struct di_buffer *buf)
{
	struct aml_v4l2_vpp *vpp = buf->caller_data;
	struct aml_v4l2_vpp_buf *vpp_buf = NULL;
	struct aml_buf *fb = NULL;
	bool bypass = false;
	bool eos = false;

	if (!vpp || !vpp->ctx) {
		v4l_dbg(0, V4L_DEBUG_CODEC_ERROR,
			"fatal %s %d vpp:%px\n",
			__func__, __LINE__, vpp);
		return DI_ERR_UNDEFINED;
	}

	if (vpp->ctx->is_stream_off) {
		v4l_dbg(vpp->ctx, V4L_DEBUG_CODEC_EXINFO,
			"vpp discard submit frame %s %d vpp:%p\n",
			__func__, __LINE__, vpp);
		return DI_ERR_UNDEFINED;
	}

	if (!kfifo_get(&vpp->processing, &vpp_buf)) {
		v4l_dbg(vpp->ctx, V4L_DEBUG_CODEC_EXINFO,
			"vpp doesn't get output %s %d vpp:%p\n",
			__func__, __LINE__, vpp);
		di_release_keep_buf_wrap1(buf);
		return DI_ERR_UNDEFINED;
	}

	fb	= vpp_buf->aml_vb->aml_buf;
	eos	= (buf->flag & DI_FLAG_EOS);
	bypass	= (buf->flag & DI_FLAG_BUF_BY_PASS);

	vpp_buf->di_buf.flag		|= buf->flag;
	vpp_buf->di_buf.vf->v4l_mem_handle = (ulong)fb;

	v4l_dbg(vpp->ctx, V4L_DEBUG_VPP_BUFMGR,
		"%s dec_vf_type:0x%x di_out_vf_type:0x%x\n",
		__func__, vpp_buf->di_buf.vf->type, buf->vf->type);

	/*di pw*/
	if (buf->vf->type & 0x40000000) {
		vpp_buf->di_buf.vf->type	|= (buf->vf->type & 0xfffffff);

		v4l_dbg(vpp->ctx, V4L_DEBUG_VPP_DETAIL,
			"di pw enable, vf_type:0x%x\n", vpp_buf->di_buf.vf->type);
	}

	v4l_dbg(vpp->ctx, V4L_DEBUG_VPP_BUFMGR,
		"%s dec_vf(w x h):(%d x %d) di_out_vf(w x h):(%d x %d)\n",
		__func__, vpp_buf->di_buf.vf->width, vpp_buf->di_buf.vf->height,
		buf->vf->width, buf->vf->height);

	if (buf->vf->width != 0 &&
		buf->vf->height != 0) {
		vpp_buf->di_buf.vf->width	= buf->vf->width;
		vpp_buf->di_buf.vf->height	= buf->vf->height;
	}

	v4l_dbg(vpp->ctx, V4L_DEBUG_VPP_BUFMGR,
		"vpp out buff canvas:phy:%lx/%lx %dx%d",
		buf->vf->canvas0_config[0].phy_addr,
		buf->vf->canvas0_config[1].phy_addr,
		buf->vf->canvas0_config[0].width,
		buf->vf->canvas0_config[0].height);

	kfifo_put(&vpp->out_done_q, vpp_buf);

	v4l_dbg(vpp->ctx, V4L_DEBUG_VPP_BUFMGR,
		"vpp_output_dw done: idx:%d, vf:%px, idx:%d, flag(vf:%x di:%x) %s %s, ts:%lld, "
		"in:%d, out:%d, vf:%d, in done:%d, out done:%d\n",
		fb->index,
		vpp_buf->di_buf.vf,
		vpp_buf->di_buf.vf->index,
		vpp_buf->di_buf.vf->flag,
		vpp_buf->di_buf.flag,
		vpp->is_prog ? "P" : "I",
		eos ? "eos" : "",
		vpp_buf->di_buf.vf->timestamp,
		kfifo_len(&vpp->input),
		kfifo_len(&vpp->output),
		kfifo_len(&vpp->frame),
		kfifo_len(&vpp->in_done_q),
		kfifo_len(&vpp->out_done_q));

	vdec_tracing(&vpp->ctx->vtr, VTRACE_VPP_PIC_9, fb->index);

	fb->task->submit(fb->task, TASK_TYPE_VPP);

	vpp->out_num[OUTPUT_PORT]++;

	vpp->in_num[OUTPUT_PORT]++;

	return DI_ERR_NONE;
}

static void vpp_vf_get(void *caller, struct vframe_s *vf_out)
{
	struct aml_v4l2_vpp *vpp = (struct aml_v4l2_vpp *)caller;
	struct aml_v4l2_vpp_buf *vpp_buf = NULL;
	struct aml_buf *aml_buf = NULL;
	struct di_buffer *buf = NULL;
	struct vframe_s *vf = NULL;
	bool bypass = false;
	bool eos = false;

	if (!vpp || !vpp->ctx) {
		v4l_dbg(0, V4L_DEBUG_CODEC_ERROR,
			"fatal %s %d vpp:%px\n",
			__func__, __LINE__, vpp);
		return;
	}

	if (kfifo_get(&vpp->out_done_q, &vpp_buf)) {
		aml_buf	= vpp_buf->aml_vb->aml_buf;
		buf	= &vpp_buf->di_buf;
		eos	= (buf->flag & DI_FLAG_EOS);
		bypass	= (buf->flag & DI_FLAG_BUF_BY_PASS);
		vf	= buf->vf;

		if (eos) {
			v4l_dbg(vpp->ctx, V4L_DEBUG_VPP_DETAIL,
				"%s %d got eos\n",
				__func__, __LINE__);
			vf->type |= VIDTYPE_V4L_EOS;
			vf->flag = VFRAME_FLAG_EMPTY_FRAME_V4L;
		}

		if (!eos && !bypass) {
			if (vpp->buffer_mode == BUFFER_MODE_ALLOC_BUF) {
				attach_DI_buffer(vpp_buf);
			}
		}

		memcpy(vf_out, vf, sizeof(struct vframe_s));

		mutex_lock(&vpp->output_lock);
		kfifo_put(&vpp->frame, vf);
		kfifo_put(&vpp->output, vpp_buf);
		mutex_unlock(&vpp->output_lock);

		up(&vpp->sem_out);

		vdec_tracing(&vpp->ctx->vtr, VTRACE_VPP_PIC_11, aml_buf->index);

		v4l_dbg(vpp->ctx, V4L_DEBUG_VPP_BUFMGR,
			"%s: vf:%px, index:%d, flag(vf:%x di:%x), ts:%lld\n",
			__func__, vf,
			vf->index,
			vf->flag,
			buf->flag,
			vf->timestamp);
	}
}

static void vpp_vf_put(void *caller, struct vframe_s *vf)
{
	#if 0
	struct aml_v4l2_vpp *vpp = (struct aml_v4l2_vpp *)caller;
	struct aml_buf *aml_buf = NULL;
	struct aml_v4l2_buf *aml_vb = NULL;
	struct aml_v4l2_vpp_buf *vpp_buf = NULL;
	struct di_buffer *buf = NULL;
	bool bypass = false;
	bool eos = false;

	aml_buf	= (struct aml_buf *) vf->v4l_mem_handle;
	aml_vb	= container_of(to_vb2_v4l2_buffer(aml_buf->vb), struct aml_v4l2_buf, vb);

	vpp_buf	= (struct aml_v4l2_vpp_buf *) aml_vb->vpp_buf_handle;
	buf	= &vpp_buf->di_buf;
	eos	= (buf->flag & DI_FLAG_EOS);
	bypass	= (buf->flag & DI_FLAG_BUF_BY_PASS);

	v4l_dbg(vpp->ctx, V4L_DEBUG_VPP_BUFMGR,
		"%s: vf:%px, index:%d, flag(vf:%x di:%x), ts:%lld\n",
		__func__, vf,
		vf->index,
		vf->flag,
		buf->flag,
		vf->timestamp);

	vdec_tracing(&vpp->ctx->vtr, VTRACE_VPP_PIC_0, aml_buf->index);

	if (!eos && !bypass) {
		if (vpp->buffer_mode == BUFFER_MODE_ALLOC_BUF) {
			detach_DI_buffer(vpp_buf);
		}
	}

	kfifo_put(&vpp->frame, vf);
	kfifo_put(&vpp->pre_output, vpp_buf);

	queue_work(vpp->ctx->dev->decode_workqueue, &vpp->worker);
	#endif
}

void vpp_wrapper_worker(struct work_struct *work)
{
	struct aml_v4l2_vpp *vpp =
		container_of(work, struct aml_v4l2_vpp, worker);
	struct aml_v4l2_vpp_buf *vpp_buf = NULL;

	while (vpp->running &&
		!kfifo_is_empty(&vpp->pre_output)) {
		if (!kfifo_get(&vpp->pre_output, &vpp_buf)) {
			v4l_dbg(vpp->ctx, 0, "vpp can not get pre output\n");
			return;
		}

		if (vpp->is_prog) {
			ATRACE_COUNTER("VC_IN_VPP-1.recycle", vpp_buf->aml_vb->aml_buf->index);
			aml_buf_fill(&vpp->ctx->bm, vpp_buf->aml_vb->aml_buf, BUF_USER_VPP);
		}

		mutex_lock(&vpp->output_lock);
		kfifo_put(&vpp->output, vpp_buf);
		mutex_unlock(&vpp->output_lock);

		up(&vpp->sem_out);
	}
}

void aml_v4l2_vpp_recycle(struct aml_v4l2_vpp *vpp, struct aml_v4l2_buf *aml_vb)
{
	struct aml_v4l2_vpp_buf *vpp_buf;
	struct di_buffer *buf = NULL;
	struct vframe_s *vf = NULL;
	bool bypass = false;
	bool eos = false;

	if (aml_vb->aml_buf->vpp_buf == NULL)
		return;

	vpp_buf = (struct aml_v4l2_vpp_buf *)aml_vb->aml_buf->vpp_buf;
	buf	= &vpp_buf->di_buf;
	eos	= (buf->flag & DI_FLAG_EOS);
	bypass	= (buf->flag & DI_FLAG_BUF_BY_PASS);
	vf      = vpp_buf->di_buf.vf;

	ATRACE_COUNTER("VC_IN_VPP-0.vf_put", aml_vb->aml_buf->index);

	if (!eos && !bypass) {
		if (vpp->buffer_mode == BUFFER_MODE_ALLOC_BUF) {
			/*vpp_buf->di_buf.caller_data might point to the vpp that had been destroyed*/
			if (vpp != vpp_buf->di_buf.caller_data)
				vpp_buf->di_buf.caller_data = vpp;
			detach_DI_buffer(vpp_buf);
		}
	}
}

static int aml_v4l2_vpp_thread(void* param)
{
	struct aml_v4l2_vpp* vpp = param;
	struct aml_vcodec_ctx *ctx = vpp->ctx;
	bool dynamic_bypass_vpp_flag = ctx->vpp_cfg.dynamic_bypass_vpp;

	v4l_dbg(ctx, V4L_DEBUG_VPP_DETAIL, "enter vpp thread\n");

	if (dynamic_bypass_vpp_flag) {
		v4l_dbg(ctx, V4L_DEBUG_VPP_DETAIL, "dynamic bypass vpp\n");
	}
	while (vpp->running) {
		struct aml_v4l2_vpp_buf *in_buf;
		struct aml_v4l2_vpp_buf *out_buf = NULL;
		struct vframe_s *vf_out = NULL;
		struct aml_buf *aml_buf;

		if (down_interruptible(&vpp->sem_in))
			goto exit;
retry:
		if (!vpp->running)
			break;

		if (dynamic_bypass_vpp_flag != ctx->vpp_cfg.dynamic_bypass_vpp) {
			dynamic_bypass_vpp_flag = ctx->vpp_cfg.dynamic_bypass_vpp;
			di_s_bypass_ch(vpp->di_handle, dynamic_bypass_vpp_flag);
			v4l_dbg(ctx, V4L_DEBUG_VPP_DETAIL, "dynamic bypass vpp:%s\n",
				dynamic_bypass_vpp_flag ? "enable" : "disable");
		}
		if (kfifo_is_empty(&vpp->output)) {
			if (down_interruptible(&vpp->sem_out))
				goto exit;
			goto retry;
		}

		if ((vpp->buffer_mode == BUFFER_MODE_ALLOC_BUF) &&
			(is_di_input_buff_full(vpp) || is_di_output_buff_full(vpp))) {
			v4l_dbg(ctx, 0, "di input/output full!\n");
			usleep_range(500, 550);
			goto retry;
		}

		mutex_lock(&vpp->output_lock);
		if (!kfifo_get(&vpp->output, &out_buf)) {
			mutex_unlock(&vpp->output_lock);
			v4l_dbg(ctx, 0, "vpp can not get output\n");
			goto exit;
		}
		mutex_unlock(&vpp->output_lock);

		/* bind v4l2 buffers */
		if (!vpp->is_prog) {
			struct aml_buf *out;

			out = aml_buf_get(&ctx->bm, BUF_USER_VPP, false);
			if (!out) {
				usleep_range(5000, 5500);
				mutex_lock(&vpp->output_lock);
				kfifo_put(&vpp->output, out_buf);
				mutex_unlock(&vpp->output_lock);
				v4l_dbg(ctx, V4L_DEBUG_VPP_DETAIL, "vpp no frame buffers!\n");
				goto retry;
			}

			out_buf->aml_vb =
				container_of(to_vb2_v4l2_buffer(out->vb), struct aml_v4l2_buf, vb);

			v4l_dbg(ctx, V4L_DEBUG_VPP_BUFMGR,
				"vpp bind buf:%d to vpp_buf:%px\n",
				VPP_BUF_GET_IDX(out_buf), out_buf);

			out->planes[0].bytes_used = out->planes[0].length;
			out->planes[1].bytes_used = out->planes[1].length;
		}

		/* safe to pop in_buf */
		if (!kfifo_get(&vpp->in_done_q, &in_buf)) {
			v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
				"vpp can not get input\n");
			goto exit;
		}

		mutex_lock(&vpp->output_lock);
		if (!kfifo_get(&vpp->frame, &vf_out)) {
			mutex_unlock(&vpp->output_lock);
			v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
				"vpp can not get frame\n");
			goto exit;
		}
		mutex_unlock(&vpp->output_lock);

		if (!vpp->is_prog) {
			/* submit I to DI. */
			aml_buf = out_buf->aml_vb->aml_buf;
			aml_buf->state = FB_ST_VPP;

			memcpy(vf_out, in_buf->di_buf.vf, sizeof(*vf_out));
			memcpy(vf_out->canvas0_config,
				in_buf->di_buf.vf->canvas0_config,
				2 * sizeof(struct canvas_config_s));

			vf_out->canvas0_config[0].phy_addr = aml_buf->planes[0].addr;
			if (aml_buf->num_planes == 1)
				vf_out->canvas0_config[1].phy_addr =
					aml_buf->planes[0].addr + aml_buf->planes[0].offset;
			else
				vf_out->canvas0_config[1].phy_addr =
					aml_buf->planes[1].addr;

			vf_out->meta_data_size = in_buf->di_buf.vf->meta_data_size;
			vf_out->meta_data_buf = in_buf->di_buf.vf->meta_data_buf;
		} else {
			/* submit P to DI. */
			out_buf->aml_vb = in_buf->aml_vb;

			memcpy(vf_out, in_buf->di_buf.vf, sizeof(*vf_out));
		}

		vf_out->mem_sec = ctx->is_drm_mode ? 1 : 0;
		/* fill outbuf parms. */
		out_buf->di_buf.vf	= vf_out;
		out_buf->di_buf.flag	= 0;
		out_buf->di_local_buf	= NULL;
		out_buf->di_buf.caller_data = vpp;

		/* fill inbuf parms. */
		in_buf->di_buf.caller_data = vpp;

		/*
		 * HWC or SF should hold di buffres refcnt after resolution changed
		 * that might cause stuck, thus sumbit 10 frames from dec to display directly.
		 * then frames will be pushed out from these buffer queuen and
		 * recycle local buffers to DI module.
		 */
		if (/*(ctx->vpp_cfg.res_chg) && */(vpp->is_prog) &&
			(vpp->buffer_mode == BUFFER_MODE_ALLOC_BUF)) {
			if (vpp->in_num[INPUT_PORT] < vpp_bypass_frames) {
				vpp->is_bypass_p = true;
			} else {
				vpp->is_bypass_p = false;
				ctx->vpp_cfg.res_chg = false;
			}
		}

		v4l_dbg(ctx, V4L_DEBUG_VPP_BUFMGR,
			"vpp_handle start: idx:(%d, %d), dec vf:%px/%d afbc:0x%lx, vpp vf:%px/%d, iphy:%lx/%lx %dx%d ophy:%lx/%lx %dx%d, %s %s "
			"in:%d, out:%d, vf:%d, in done:%d, out done:%d, fgs_valid:%d",
			in_buf->aml_vb->aml_buf->index,
			out_buf->aml_vb->aml_buf->index,
			in_buf->di_buf.vf,
			in_buf->di_buf.vf->index,
			in_buf->di_buf.vf->compHeadAddr,
			out_buf->di_buf.vf, VPP_BUF_GET_IDX(out_buf),
			in_buf->di_buf.vf->canvas0_config[0].phy_addr,
			in_buf->di_buf.vf->canvas0_config[1].phy_addr,
			in_buf->di_buf.vf->canvas0_config[0].width,
			in_buf->di_buf.vf->canvas0_config[0].height,
			vf_out->canvas0_config[0].phy_addr,
			vf_out->canvas0_config[1].phy_addr,
			vf_out->canvas0_config[0].width,
			vf_out->canvas0_config[0].height,
			vpp->is_prog ? "P" : "",
			vpp->is_bypass_p ? "bypass-prog" : "",
			kfifo_len(&vpp->input),
			kfifo_len(&vpp->output),
			kfifo_len(&vpp->frame),
			kfifo_len(&vpp->in_done_q),
			kfifo_len(&vpp->out_done_q),
			in_buf->di_buf.vf->fgs_valid);

		if (vpp->work_mode == VPP_MODE_S4_DW_MMU) {
			vdec_tracing(&vpp->ctx->vtr, VTRACE_VPP_PIC_7,
				out_buf->aml_vb->aml_buf->index);

			kfifo_put(&vpp->processing, in_buf);

			di_fill_output_buffer(vpp->di_handle, &out_buf->di_buf);
			vdec_tracing(&vpp->ctx->vtr, VTRACE_VPP_PIC_8,
				in_buf->aml_vb->aml_buf->index);
			di_empty_input_buffer(vpp->di_handle, &in_buf->di_buf);
		} else {
			if (vpp->is_bypass_p) {
				vdec_tracing(&vpp->ctx->vtr, VTRACE_VPP_PIC_6,
					in_buf->aml_vb->aml_buf->index);
				out_buf->di_buf.flag = in_buf->di_buf.flag;
				out_buf->di_buf.vf->vf_ext = in_buf->di_buf.vf;

				v4l_vpp_fill_output_done(&out_buf->di_buf);
				v4l_vpp_empty_input_done(&in_buf->di_buf);
			} else {
				if (vpp->buffer_mode == BUFFER_MODE_ALLOC_BUF) {
					/*
					 * the flow of DI local buffer:
					 * empty input -> output done cb -> fetch processing fifo.
					 */
					vdec_tracing(&vpp->ctx->vtr, VTRACE_VPP_PIC_5,
						in_buf->aml_vb->aml_buf->index);

					out_buf->inbuf = in_buf;
					kfifo_put(&vpp->processing, out_buf);

					di_empty_input_buffer(vpp->di_handle, &in_buf->di_buf);
				} else {
					vdec_tracing(&vpp->ctx->vtr, VTRACE_VPP_PIC_3,
						out_buf->aml_vb->aml_buf->index);
					di_fill_output_buffer(vpp->di_handle, &out_buf->di_buf);

					vdec_tracing(&vpp->ctx->vtr, VTRACE_VPP_PIC_4,
						in_buf->aml_vb->aml_buf->index);
					di_empty_input_buffer(vpp->di_handle, &in_buf->di_buf);
				}
			}
		}
		vpp->in_num[INPUT_PORT]++;
		vpp->out_num[INPUT_PORT]++;
	}
exit:
	while (!kthread_should_stop()) {
		usleep_range(1000, 2000);
	}

	v4l_dbg(ctx, V4L_DEBUG_VPP_DETAIL, "exit vpp thread\n");

	return 0;
}

int aml_v4l2_vpp_get_buf_num(u32 mode)
{
	if ((mode == VPP_MODE_DI) ||
		(mode == VPP_MODE_COLOR_CONV) ||
		(mode == VPP_MODE_NOISE_REDUC)) {
		return 4;
	}
	//TODO: support more modes
	return 2;
}

int aml_v4l2_vpp_reset(struct aml_v4l2_vpp *vpp)
{
	int i;
	struct sched_param param =
		{ .sched_priority = MAX_RT_PRIO - 1 };

	if (vpp->running) {
		vpp->running = false;
		cancel_work_sync(&vpp->worker);
		up(&vpp->sem_in);
		up(&vpp->sem_out);
		kthread_stop(vpp->task);
	}

	kfifo_reset(&vpp->input);
	kfifo_reset(&vpp->output);
	kfifo_reset(&vpp->pre_output);
	kfifo_reset(&vpp->frame);
	kfifo_reset(&vpp->out_done_q);
	kfifo_reset(&vpp->in_done_q);
	kfifo_reset(&vpp->processing);

	for (i = 0 ; i < VPP_FRAME_SIZE ; i++) {
		memset(&vpp->ivbpool[i], 0, sizeof(struct aml_v4l2_vpp_buf));

		kfifo_put(&vpp->input, &vpp->ivbpool[i]);
	}

	for (i = 0 ; i < vpp->buf_size ; i++) {
		memset(&vpp->ovbpool[i], 0, sizeof(struct aml_v4l2_vpp_buf));
		memset(&vpp->vfpool[i], 0, sizeof(struct vframe_s));

		kfifo_put(&vpp->output, &vpp->ovbpool[i]);
		kfifo_put(&vpp->frame, &vpp->vfpool[i]);
	}

	vpp->in_num[0]	= 0;
	vpp->in_num[1]	= 0;
	vpp->out_num[0]	= 0;
	vpp->out_num[1]	= 0;
	vpp->fb_token	= 0;
	sema_init(&vpp->sem_in, 0);
	sema_init(&vpp->sem_out, 0);

	vpp->running = true;
	vpp->task = kthread_run(aml_v4l2_vpp_thread, vpp,
		"%s", "aml-v4l2-vpp");
	if (IS_ERR(vpp->task)) {
		return PTR_ERR(vpp->task);
	}

	sched_setscheduler_nocheck(vpp->task, SCHED_FIFO, &param);

	v4l_dbg(vpp->ctx, V4L_DEBUG_CODEC_PRINFO, "vpp wrapper reset.\n");

	return 0;

}
EXPORT_SYMBOL(aml_v4l2_vpp_reset);

int aml_v4l2_vpp_init(
		struct aml_vcodec_ctx *ctx,
		struct aml_vpp_cfg_infos *cfg,
		struct aml_v4l2_vpp** vpp_handle)
{
	struct di_init_parm init;
	u32 buf_size;
	int i, ret;
	struct sched_param param = { .sched_priority = MAX_RT_PRIO - 1 };
	struct aml_v4l2_vpp *vpp;
	u32 work_mode = cfg->mode;

	if (!cfg || work_mode > VPP_MODE_MAX || !ctx || !vpp_handle)
		return -EINVAL;
	if (cfg->fmt != V4L2_PIX_FMT_NV12 && cfg->fmt != V4L2_PIX_FMT_NV12M &&
		cfg->fmt != V4L2_PIX_FMT_NV21 && cfg->fmt != V4L2_PIX_FMT_NV21M)
		return -EINVAL;

	vpp = kzalloc(sizeof(*vpp), GFP_KERNEL);
	if (!vpp)
		return -ENOMEM;

	vpp->work_mode = work_mode;
	if (vpp->work_mode >= VPP_MODE_DI_LOCAL &&
		vpp->work_mode <= VPP_MODE_NOISE_REDUC_LOCAL)
		vpp->buffer_mode = BUFFER_MODE_ALLOC_BUF;
	else
		vpp->buffer_mode = BUFFER_MODE_USE_BUF;

	if (vpp->work_mode == VPP_MODE_S4_DW_MMU)
		init.work_mode			= WORK_MODE_S4_DCOPY;
	else
		init.work_mode			= WORK_MODE_PRE_POST;
	init.buffer_mode		= vpp->buffer_mode;
	init.ops.fill_output_done	= v4l_vpp_fill_output_done;
	init.ops.empty_input_done	= v4l_vpp_empty_input_done;
	init.caller_data		= (void *)vpp;

	v4l_dbg(ctx, V4L_DEBUG_VPP_DETAIL,
		"%s work_mode:0x%x buffer_mode:%d\n",__func__, vpp->work_mode, vpp->buffer_mode);

	if (vpp->buffer_mode == BUFFER_MODE_ALLOC_BUF) {
		init.ops.fill_output_done =
			v4l_vpp_fill_output_done_alloc_buffer;
	}

	if (vpp->work_mode == VPP_MODE_S4_DW_MMU) {
		init.ops.fill_output_done =
			v4l_vpp_fill_output_done_dw_mmu;
	}

	if (vpp->buffer_mode == BUFFER_MODE_ALLOC_BUF)
		init.output_format = DI_OUTPUT_BY_DI_DEFINE;
	else if ((vpp->buffer_mode == BUFFER_MODE_USE_BUF) &&
		((cfg->fmt == V4L2_PIX_FMT_NV21M) || (cfg->fmt == V4L2_PIX_FMT_NV21)))
		init.output_format = DI_OUTPUT_NV21 | DI_OUTPUT_LINEAR;
	else if ((vpp->buffer_mode == BUFFER_MODE_USE_BUF) &&
		((cfg->fmt == V4L2_PIX_FMT_NV12M) || (cfg->fmt == V4L2_PIX_FMT_NV12)))
		init.output_format = DI_OUTPUT_NV12 | DI_OUTPUT_LINEAR;
	else /* AFBC decoder case, NV12 as default */
		init.output_format = DI_OUTPUT_NV12 | DI_OUTPUT_LINEAR;

	if (cfg->is_drm)
		init.output_format |= DI_OUTPUT_TVP;

	vpp->di_handle = di_create_instance(init);
	if (vpp->di_handle < 0) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"di_create_instance fail\n");
		ret = -EINVAL;
		goto error;
	}

	INIT_KFIFO(vpp->input);
	INIT_KFIFO(vpp->output);
	INIT_KFIFO(vpp->pre_output);
	INIT_KFIFO(vpp->frame);
	INIT_KFIFO(vpp->out_done_q);
	INIT_KFIFO(vpp->in_done_q);
	INIT_KFIFO(vpp->processing);

	vpp->ctx = ctx;
	vpp->is_prog = cfg->is_prog;
	vpp->is_bypass_p = cfg->is_bypass_p;
	INIT_WORK(&vpp->worker, vpp_wrapper_worker);

	buf_size = vpp->is_prog ? 16 : cfg->buf_size;
	vpp->buf_size = buf_size;

	/* setup output fifo */
	ret = kfifo_alloc(&vpp->pre_output, buf_size, GFP_KERNEL);
	if (ret) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"alloc pre_output fifo fail.\n");
		ret = -ENOMEM;
		goto error1;
	}

	ret = kfifo_alloc(&vpp->output, buf_size, GFP_KERNEL);
	if (ret) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"alloc output fifo fail.\n");
		ret = -ENOMEM;
		goto error2;
	}

	vpp->ovbpool = vzalloc(buf_size * sizeof(*vpp->ovbpool));
	if (!vpp->ovbpool) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"alloc output vb pool fail.\n");
		ret = -ENOMEM;
		goto error3;
	}

	/* setup vframe fifo */
	ret = kfifo_alloc(&vpp->frame, buf_size, GFP_KERNEL);
	if (ret) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"alloc vpp vframe fifo fail.\n");
		ret = -ENOMEM;
		goto error4;
	}

	vpp->vfpool = vzalloc(buf_size * sizeof(*vpp->vfpool));
	if (!vpp->vfpool) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"alloc vf pool fail.\n");
		ret = -ENOMEM;
		goto error5;
	}

	/* setup processing fifo */
	ret = kfifo_alloc(&vpp->processing, buf_size, GFP_KERNEL);
	if (ret) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"alloc processing fifo fail.\n");
		ret = -ENOMEM;
		goto error6;
	}

	ret = kfifo_alloc(&vpp->input, VPP_FRAME_SIZE, GFP_KERNEL);
	if (ret) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"alloc input fifo fail.\n");
		ret = -ENOMEM;
		goto error7;
	}

	vpp->ivbpool = vzalloc(VPP_FRAME_SIZE * sizeof(*vpp->ivbpool));
	if (!vpp->ivbpool) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"alloc input vb pool fail.\n");
		ret = -ENOMEM;
		goto error8;
	}

	for (i = 0 ; i < VPP_FRAME_SIZE ; i++) {
		kfifo_put(&vpp->input, &vpp->ivbpool[i]);
	}

	for (i = 0 ; i < buf_size ; i++) {
		kfifo_put(&vpp->output, &vpp->ovbpool[i]);
		kfifo_put(&vpp->frame, &vpp->vfpool[i]);
	}

	mutex_init(&vpp->output_lock);
	sema_init(&vpp->sem_in, 0);
	sema_init(&vpp->sem_out, 0);
	atomic_set(&vpp->local_buf_out, 0);

	vpp->running = true;
	vpp->task = kthread_run(aml_v4l2_vpp_thread, vpp,
		"aml-%s", "aml-v4l2-vpp");
	if (IS_ERR(vpp->task)) {
		ret = PTR_ERR(vpp->task);
		goto error9;
	}
	sched_setscheduler_nocheck(vpp->task, SCHED_FIFO, &param);

	vpp->di_ibuf_num = di_get_input_buffer_num(vpp->di_handle);
	vpp->di_obuf_num = di_get_output_buffer_num(vpp->di_handle);

	*vpp_handle = vpp;

	v4l_dbg(ctx, V4L_DEBUG_CODEC_PRINFO,
		"vpp_wrapper init bsize:%d, di(i:%d, o:%d), wkm:%x, bm:%x, fmt:%x, drm:%d, prog:%d, byp:%d, local:%d, NR:%d\n",
		vpp->buf_size,
		vpp->di_ibuf_num,
		vpp->di_obuf_num,
		vpp->work_mode,
		vpp->buffer_mode,
		init.output_format,
		cfg->is_drm,
		cfg->is_prog,
		cfg->is_bypass_p,
		cfg->enable_local_buf,
		cfg->enable_nr);

	return 0;

error9:
	vfree(vpp->ivbpool);
error8:
	kfifo_free(&vpp->input);
error7:
	kfifo_free(&vpp->processing);
error6:
	vfree(vpp->vfpool);
error5:
	kfifo_free(&vpp->frame);
error4:
	vfree(vpp->ovbpool);
error3:
	kfifo_free(&vpp->output);
error2:
	kfifo_free(&vpp->pre_output);
error1:
	di_destroy_instance(vpp->di_handle);
error:
	kfree(vpp);
	return ret;
}
EXPORT_SYMBOL(aml_v4l2_vpp_init);

int aml_v4l2_vpp_destroy(struct aml_v4l2_vpp* vpp)
{
	v4l_dbg(vpp->ctx, V4L_DEBUG_VPP_DETAIL,
		"vpp destroy begin\n");
	atomic_set(&vpp->local_buf_out, 0);
	if (vpp->running) {
		vpp->running = false;
		cancel_work_sync(&vpp->worker);
		up(&vpp->sem_in);
		up(&vpp->sem_out);
		kthread_stop(vpp->task);
	}
	di_destroy_instance(vpp->di_handle);
	/* no more vpp callback below this line */

	if (vpp->buffer_mode == BUFFER_MODE_ALLOC_BUF)
		release_DI_buff(vpp);

	kfifo_free(&vpp->processing);
	kfifo_free(&vpp->frame);
	vfree(vpp->vfpool);
	kfifo_free(&vpp->pre_output);
	kfifo_free(&vpp->output);
	vfree(vpp->ovbpool);
	kfifo_free(&vpp->input);
	vfree(vpp->ivbpool);
	mutex_destroy(&vpp->output_lock);
	v4l_dbg(vpp->ctx, V4L_DEBUG_VPP_DETAIL,
		"vpp_wrapper destroy done\n");
	kfree(vpp);

	return 0;
}
EXPORT_SYMBOL(aml_v4l2_vpp_destroy);

int aml_v4l2_vpp_thread_stop(struct aml_v4l2_vpp* vpp)
{
	v4l_dbg(vpp->ctx, V4L_DEBUG_VPP_DETAIL,
		"vpp thread stop begin\n");

	if (vpp->running) {
		vpp->running = false;
		up(&vpp->sem_in);
		up(&vpp->sem_out);
		kthread_stop(vpp->task);
	}

	v4l_dbg(vpp->ctx, V4L_DEBUG_VPP_DETAIL,
		"vpp thread stop done\n");

	return 0;
}
EXPORT_SYMBOL(aml_v4l2_vpp_thread_stop);

static int aml_v4l2_vpp_push_vframe(struct aml_v4l2_vpp* vpp, struct vframe_s *vf)
{
	struct aml_v4l2_vpp_buf *in_buf;
	struct aml_buf *aml_buf = NULL;

	if (!vpp)
		return -EINVAL;

	if (!kfifo_get(&vpp->input, &in_buf)) {
		v4l_dbg(vpp->ctx, V4L_DEBUG_CODEC_ERROR,
			"can not get free input buffer.\n");
		return -1;
	}

#if 0 //to debug di by frame
	if (vpp->in_num[INPUT_PORT] > 2)
		return 0;
	if (vpp->in_num[INPUT_PORT] == 2)
		vf->type |= VIDTYPE_V4L_EOS;
#endif
	in_buf->di_buf.vf = &in_buf->vf;
	in_buf->di_buf.flag = 0;
	if (vf->type & VIDTYPE_V4L_EOS) {
		u32 dw_mode = DM_YUV_ONLY;

		in_buf->di_buf.flag |= DI_FLAG_EOS;

		if (vdec_if_get_param(vpp->ctx, GET_PARAM_DW_MODE, &dw_mode))
			return -1;

		vf->type |= vpp->ctx->last_decoded_picinfo.field == V4L2_FIELD_INTERLACED ?
						VIDTYPE_INTERLACE :
						VIDTYPE_PROGRESSIVE;

		if (dw_mode != DM_YUV_ONLY)
			vf->type |= VIDTYPE_COMPRESS;
	}

	aml_buf = (struct aml_buf *)vf->v4l_mem_handle;
	in_buf->aml_vb = container_of(to_vb2_v4l2_buffer(aml_buf->vb), struct aml_v4l2_buf, vb);

	if (is_cpu_t7()) {
		if (vf->canvas0_config[0].block_mode == CANVAS_BLKMODE_LINEAR) {
			if ((vpp->ctx->output_pix_fmt != V4L2_PIX_FMT_H264) &&
				(vpp->ctx->output_pix_fmt != V4L2_PIX_FMT_MPEG1) &&
				(vpp->ctx->output_pix_fmt != V4L2_PIX_FMT_MPEG2) &&
				(vpp->ctx->output_pix_fmt != V4L2_PIX_FMT_MPEG4) &&
				(vpp->ctx->output_pix_fmt != V4L2_PIX_FMT_MJPEG)) {
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
	memcpy(&in_buf->vf, vf, sizeof(struct vframe_s));

	v4l_dbg(vpp->ctx, V4L_DEBUG_VPP_BUFMGR,
		"vpp_push_vframe: idx:%d, vf:%px, idx:%d, type:%x, ts:%lld flag:%x\n",
		aml_buf->index, vf, vf->index, vf->type, vf->timestamp, vf->flag);

	do {
		unsigned int dw_mode = DM_YUV_ONLY;
		struct file *fp;
		char file_name[64] = {0};
		if (!dump_vpp_input || vpp->ctx->is_drm_mode)
			break;
		if (vdec_if_get_param(vpp->ctx, GET_PARAM_DW_MODE, &dw_mode))
			break;
		if (dw_mode == DM_AVBC_ONLY)
			break;

		snprintf(file_name, 64, "%s/dec_dump_vpp_input_%ux%u.raw", dump_path, vf->width, vf->height);
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
				dump_vpp_input,
				vf->width,
				vf->height,
				vb->num_planes);

			dump_vpp_input--;
			media_close(fp, NULL);
		}
	} while(0);

	vdec_tracing(&vpp->ctx->vtr, VTRACE_VPP_PIC_2, aml_buf->index);

	kfifo_put(&vpp->in_done_q, in_buf);
	up(&vpp->sem_in);

	update_vpp_num_cache(vpp);
	vdec_tracing(&vpp->ctx->vtr, VTRACE_VPP_PIC_15, atomic_read(&vpp->ctx->vpp_cache_num));

	aml_buf_update_holder(&vpp->ctx->bm, aml_buf, BUF_USER_VPP, BUF_GET);

	return 0;
}

static void fill_vpp_buf_cb(void *v4l_ctx, void *fb_ctx)
{
	struct aml_vcodec_ctx *ctx =
		(struct aml_vcodec_ctx *)v4l_ctx;
	struct aml_buf *aml_buf =
		(struct aml_buf *)fb_ctx;
	int ret = -1;
	ret = aml_v4l2_vpp_push_vframe(ctx->vpp, &aml_buf->vframe);
	if (ret < 0) {
		v4l_dbg(ctx, V4L_DEBUG_CODEC_ERROR,
			"vpp push vframe err, ret: %d\n", ret);
	}
}

static struct task_ops_s vpp_ops = {
	.type		= TASK_TYPE_VPP,
	.get_vframe	= vpp_vf_get,
	.put_vframe	= vpp_vf_put,
	.fill_buffer	= fill_vpp_buf_cb,
};

struct task_ops_s *get_vpp_ops(void)
{
	return &vpp_ops;
}
EXPORT_SYMBOL(get_vpp_ops);

