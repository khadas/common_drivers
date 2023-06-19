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
#include <media/v4l2-mem2mem.h>
#include <linux/amlogic/media/utils/amstream.h>
#include <linux/amlogic/media/utils/vformat.h>
#include <linux/amlogic/media/utils/aformat.h>
#include <linux/amlogic/media/frame_sync/tsync.h>
#include <linux/amlogic/media/frame_sync/ptsserv.h>
#include <linux/amlogic/media/frame_sync/timestamp.h>
#include <linux/amlogic/media/utils/amports_config.h>
#include <linux/amlogic/media/frame_sync/tsync_pcr.h>
#include <linux/amlogic/media/codec_mm/codec_mm.h>
#include <linux/amlogic/media/codec_mm/configs.h>
#include <linux/amlogic/media/utils/vformat.h>
#include <linux/amlogic/media/utils/aformat.h>
#include <linux/amlogic/media/registers/register.h>
#include "../stream_input/amports/adec.h"
#include "../stream_input/amports/streambuf.h"
#include "../stream_input/amports/streambuf_reg.h"
#include "../frame_provider/decoder/utils/vdec.h"
#include "../common/media_clock/switch/amports_gate.h"
#include "../stream_input/parser/stream_parser.h"
#include <linux/delay.h>
#include "aml_vcodec_adapt.h"
#include "aml_vcodec_ts.h"
#include <linux/crc32.h>
#include "../common/media_utils/media_utils.h"

#define DEFAULT_VIDEO_BUFFER_SIZE		(1024 * 1024 * 3)
#define DEFAULT_VIDEO_BUFFER_SIZE_4K		(1024 * 1024 * 6)
#define DEFAULT_VIDEO_BUFFER_SIZE_TVP		(1024 * 1024 * 10)
#define DEFAULT_VIDEO_BUFFER_SIZE_4K_TVP	(1024 * 1024 * 15)
#define DEFAULT_AUDIO_BUFFER_SIZE		(1024*768*2)
#define DEFAULT_SUBTITLE_BUFFER_SIZE		(1024*256)

#define PTS_OUTSIDE	(1)
#define SYNC_OUTSIDE	(2)

//#define DATA_DEBUG
extern int dump_es_output_frame;
extern int dump_output_frame;
extern char dump_path[32];
extern u32 dump_output_start_position;
extern void aml_recycle_dma_buffers(struct aml_vcodec_ctx *ctx, u32 handle);

static int slow_input = 0;

static struct stream_buf_s bufs[BUF_MAX_NUM] = {
	{
		.reg_base = VLD_MEM_VIFIFO_REG_BASE,
		.type = BUF_TYPE_VIDEO,
		.buf_start = 0,
		.buf_size = DEFAULT_VIDEO_BUFFER_SIZE,
		.default_buf_size = DEFAULT_VIDEO_BUFFER_SIZE,
		.first_tstamp = INVALID_PTS
	},
	{
		.reg_base = AIU_MEM_AIFIFO_REG_BASE,
		.type = BUF_TYPE_AUDIO,
		.buf_start = 0,
		.buf_size = DEFAULT_AUDIO_BUFFER_SIZE,
		.default_buf_size = DEFAULT_AUDIO_BUFFER_SIZE,
		.first_tstamp = INVALID_PTS
	},
	{
		.reg_base = 0,
		.type = BUF_TYPE_SUBTITLE,
		.buf_start = 0,
		.buf_size = DEFAULT_SUBTITLE_BUFFER_SIZE,
		.default_buf_size = DEFAULT_SUBTITLE_BUFFER_SIZE,
		.first_tstamp = INVALID_PTS
	},
	{
		.reg_base = 0,
		.type = BUF_TYPE_USERDATA,
		.buf_start = 0,
		.buf_size = 0,
		.first_tstamp = INVALID_PTS
	},
	{
		.reg_base = HEVC_STREAM_REG_BASE,
		.type = BUF_TYPE_HEVC,
		.buf_start = 0,
		.buf_size = DEFAULT_VIDEO_BUFFER_SIZE_4K,
		.default_buf_size = DEFAULT_VIDEO_BUFFER_SIZE_4K,
		.first_tstamp = INVALID_PTS
	},
};

extern int aml_set_vfm_path, aml_set_vdec_type;
extern bool aml_set_vfm_enable, aml_set_vdec_type_enable;

static void set_default_params(struct aml_vdec_adapt *vdec)
{
	ulong sync_mode = (PTS_OUTSIDE | SYNC_OUTSIDE);

	vdec->dec_prop.param = (void *)sync_mode;
	vdec->dec_prop.format = vdec->format;
	vdec->dec_prop.width = 1920;
	vdec->dec_prop.height = 1088;
	vdec->dec_prop.rate = 3200;
}

static int enable_hardware(struct stream_port_s *port)
{
	if (get_cpu_type() < MESON_CPU_MAJOR_ID_M6)
		return -1;

	amports_switch_gate("demux", 1);
	if (get_cpu_type() >= MESON_CPU_MAJOR_ID_M8)
		amports_switch_gate("parser_top", 1);

	if (port->type & PORT_TYPE_VIDEO) {
		amports_switch_gate("vdec", 1);

		if (has_hevc_vdec()) {
			if (port->type & PORT_TYPE_HEVC)
				vdec_poweron(VDEC_HEVC);
			else
				vdec_poweron(VDEC_1);
		} else {
			if (get_cpu_type() >= MESON_CPU_MAJOR_ID_M8)
				vdec_poweron(VDEC_1);
		}
	}

	return 0;
}

static int disable_hardware(struct stream_port_s *port)
{
	if (get_cpu_type() < MESON_CPU_MAJOR_ID_M6)
		return -1;

	if (port->type & PORT_TYPE_VIDEO) {
		if (has_hevc_vdec()) {
			if (port->type & PORT_TYPE_HEVC)
				vdec_poweroff(VDEC_HEVC);
			else
				vdec_poweroff(VDEC_1);
		}

		amports_switch_gate("vdec", 0);
	}

	if (get_cpu_type() >= MESON_CPU_MAJOR_ID_M8)
		amports_switch_gate("parser_top", 0);

	amports_switch_gate("demux", 0);

	return 0;
}

static void user_buffer_init(void)
{
	struct stream_buf_s *pubuf = &bufs[BUF_TYPE_USERDATA];

	pubuf->buf_size = 0;
	pubuf->buf_start = 0;
	pubuf->buf_wp = 0;
	pubuf->buf_rp = 0;
}

static void video_component_release(struct stream_port_s *port)
{
	struct aml_vdec_adapt *ada_ctx
		= container_of(port, struct aml_vdec_adapt, port);
	struct vdec_s *vdec = ada_ctx->vdec;

	vdec_release(vdec);

}

static int video_component_init(struct stream_port_s *port,
			  struct stream_buf_s *pbuf)
{
	int ret = -1;
	struct aml_vdec_adapt *ada_ctx
		= container_of(port, struct aml_vdec_adapt, port);
	struct vdec_s *vdec = ada_ctx->vdec;

	if ((vdec->port_flag & PORT_FLAG_VFORMAT) == 0) {
		v4l_dbg(ada_ctx->ctx, V4L_DEBUG_CODEC_ERROR, "vformat not set\n");
		return -EPERM;
	}

	port->is_4k = false;

	if (port->type & PORT_TYPE_FRAME ||
		(port->type & PORT_TYPE_ES)) {
		ret = vdec_init(vdec, port->is_4k, true);
		if (ret < 0) {
			v4l_dbg(ada_ctx->ctx, V4L_DEBUG_CODEC_ERROR, "failed\n");
			video_component_release(port);
			return ret;
		}
	}

	return 0;
}

static int vdec_ports_release(struct stream_port_s *port)
{
	struct stream_buf_s *pvbuf = &bufs[BUF_TYPE_VIDEO];

	if (has_hevc_vdec()) {
		if (port->vformat == VFORMAT_HEVC ||
			port->vformat == VFORMAT_VP9)
			pvbuf = &bufs[BUF_TYPE_HEVC];
	}

	if (port->type & PORT_TYPE_MPTS) {
		tsync_pcr_stop();
		tsdemux_release();
	}

	if (port->type & PORT_TYPE_VIDEO)
		video_component_release(port);

	port->pcr_inited = 0;
	port->flag = 0;

	return 0;
}

static void set_vdec_property(struct vdec_s *vdec,
	struct aml_vdec_adapt *ada_ctx)
{
	vdec->sys_info	= &ada_ctx->dec_prop;
	vdec->port	= &ada_ctx->port;
	vdec->format	= ada_ctx->video_type;
	vdec->sys_info_store = ada_ctx->dec_prop;

	/* binding v4l2 ctx to vdec. */
	vdec->private = ada_ctx->ctx;

	/* set video format, sys info and vfm map.*/
	vdec->port->vformat = vdec->format;
	vdec->port->type |= PORT_TYPE_VIDEO;
	vdec->port_flag |= (vdec->port->flag | PORT_FLAG_VFORMAT);
	if (vdec->slave) {
		vdec->slave->format = ada_ctx->dec_prop.format;
		vdec->slave->port_flag |= PORT_FLAG_VFORMAT;
	}

	vdec->type = VDEC_TYPE_FRAME_BLOCK;
	vdec->port->type |= PORT_TYPE_FRAME;
	vdec->frame_base_video_path = FRAME_BASE_PATH_V4L_OSD;

	if (ada_ctx->ctx->stream_mode) {
		vdec->type = VDEC_TYPE_STREAM_PARSER;
		vdec->port->type &= ~PORT_TYPE_FRAME;
		vdec->port->type |= PORT_TYPE_ES;
	} else {
		vdec->type = VDEC_TYPE_FRAME_BLOCK;
		vdec->port->type &= ~PORT_TYPE_ES;
		vdec->port->type |= PORT_TYPE_FRAME;
	}

	if (vdec->format == VFORMAT_VC1)
		vdec->type = VDEC_TYPE_SINGLE;

	if (aml_set_vfm_enable)
		vdec->frame_base_video_path = aml_set_vfm_path;

	vdec->port->flag = vdec->port_flag;

	vdec->config_len = ada_ctx->config.length >
		PAGE_SIZE ? PAGE_SIZE : ada_ctx->config.length;
	memcpy(vdec->config, ada_ctx->config.buf, vdec->config_len);

	ada_ctx->vdec = vdec;
}

static int vdec_ports_init(struct aml_vdec_adapt *ada_ctx)
{
	int ret = -1;
	struct stream_buf_s *pvbuf = &bufs[BUF_TYPE_VIDEO];
	struct vdec_s *vdec = NULL;

	/* create the vdec instance.*/
	vdec = vdec_create(&ada_ctx->port, NULL);
	if (IS_ERR_OR_NULL(vdec))
		return -1;

	vdec->disable_vfm = true;
	set_vdec_property(vdec, ada_ctx);

	/* init hw and gate*/
	ret = enable_hardware(vdec->port);
	if (ret < 0) {
		v4l_dbg(ada_ctx->ctx, V4L_DEBUG_CODEC_ERROR, "enable hw fail.\n");
		return ret;
	}

	stbuf_fetch_init();
	user_buffer_init();

	if ((vdec->port->type & PORT_TYPE_VIDEO)
		&& (vdec->port_flag & PORT_FLAG_VFORMAT)) {
		vdec->port->is_4k = false;
		if (has_hevc_vdec()) {
			if (vdec->port->vformat == VFORMAT_HEVC ||
				vdec->port->vformat == VFORMAT_VP9)
				pvbuf = &bufs[BUF_TYPE_HEVC];
		}
		if (vdec_stream_based(vdec)) {
			struct parser_args pars;
			struct stream_buf_ops *ops = get_stbuf_ops();

			ret = stream_buffer_base_init(&vdec->vbuf, ops, &pars);
			if (ret) {
				v4l_dbg(0, V4L_DEBUG_CODEC_ERROR,
					"stream buffer base init failed\n");
				return ret;
			}
		}
		ret = video_component_init(vdec->port, pvbuf);
		if (ret < 0) {
			v4l_dbg(ada_ctx->ctx, V4L_DEBUG_CODEC_ERROR, "video_component_init  failed\n");
			return ret;
		}

		/* connect vdec at the end after all HW initialization */
		aml_codec_connect(ada_ctx);
	}

	return 0;
}

int video_decoder_init(struct aml_vdec_adapt *vdec)
{
	int ret = -1;

	/* sets configure data */
	set_default_params(vdec);

	/* init the buffer work space and connect vdec.*/
	ret = vdec_ports_init(vdec);
	if (ret < 0) {
		v4l_dbg(vdec->ctx, V4L_DEBUG_CODEC_ERROR, "vdec ports init fail.\n");
		goto out;
	}
out:
	return ret;
}

int video_decoder_release(struct aml_vdec_adapt *vdec)
{
	int ret = -1;
	struct stream_port_s *port = &vdec->port;

	ret = vdec_ports_release(port);
	if (ret < 0) {
		v4l_dbg(vdec->ctx, V4L_DEBUG_CODEC_ERROR, "vdec ports release fail.\n");
		goto out;
	}

	/* disable gates */
	ret = disable_hardware(port);
	if (ret < 0) {
		v4l_dbg(vdec->ctx, V4L_DEBUG_CODEC_ERROR, "disable hw fail.\n");
		goto out;
	}
out:
	return ret;
}

void dump(const char* path, const char *data, unsigned int size)
{
	struct file *fp;

	fp = media_open(path,
			O_CREAT | O_RDWR | O_LARGEFILE | O_APPEND, 0666);
	if (!IS_ERR(fp)) {
		media_write(fp, data, size, 0);
		media_close(fp, NULL);
	} else {
		pr_info("Dump ES fail, should check RW permission, size:%x\n", size);
	}
}

int vdec_vbuf_write(struct aml_vdec_adapt *ada_ctx,
	const char *buf, unsigned int count)
{
	int ret = -1;
	int try_cnt = 100;
	struct stream_port_s *port = &ada_ctx->port;
	struct vdec_s *vdec = ada_ctx->vdec;
	struct stream_buf_s *pbuf = NULL;

	if (has_hevc_vdec()) {
		pbuf = (port->type & PORT_TYPE_HEVC) ? &bufs[BUF_TYPE_HEVC] :
			&bufs[BUF_TYPE_VIDEO];
	} else
		pbuf = &bufs[BUF_TYPE_VIDEO];

	/*if (!(port_get_inited(priv))) {
		r = video_decoder_init(priv);
		if (r < 0)
			return r;
	}*/

	do {
		if (vdec->port_flag & PORT_FLAG_DRM)
			ret = drm_write(ada_ctx->filp, pbuf, buf, count);
		else
			ret = esparser_write(ada_ctx->filp, pbuf, buf, count);

		if (ret == -EAGAIN)
			msleep(30);
	} while (ret == -EAGAIN && try_cnt--);

	if (slow_input) {
		v4l_dbg(ada_ctx->ctx, V4L_DEBUG_CODEC_PRINFO,
			"slow_input: es codec write size %x\n", ret);
		msleep(10);
	}

#ifdef DATA_DEBUG
	/* dump to file */
	//dump_write(vbuf, size);
	//v4l_dbg(ada_ctx->ctx, V4L_DEBUG_CODEC_PRINFO, "vbuf: %p, size: %u, ret: %d\n", vbuf, size, ret);
#endif

	return ret;
}

bool vdec_input_full(struct aml_vdec_adapt *ada_ctx)
{
	struct vdec_s *vdec = ada_ctx->vdec;
	struct aml_vcodec_ctx *ctx = ada_ctx->ctx;

	/* The driver ignores the stream ctrl if in stream mode. */
	if (vdec_stream_based(vdec))
		return false;

	return (vdec->input.have_frame_num > ctx->cache_input_buffer_num) ? true : false;
}

int vdec_vframe_write(struct aml_vdec_adapt *ada_ctx, const char *buf,
	unsigned int count, u64 timestamp, ulong meta_ptr, chunk_free free)
{
	int ret = -1;
	struct vdec_s *vdec = ada_ctx->vdec;

	/* set timestamp */
	vdec_set_timestamp(vdec, timestamp);

	/* set metadata */
	vdec_set_metadata(vdec, meta_ptr);

	ret = vdec_write_vframe(vdec, buf, count, free, ada_ctx->ctx);

	if (slow_input) {
		v4l_dbg(ada_ctx->ctx, V4L_DEBUG_CODEC_PRINFO,
			"slow_input: frame codec write size %d\n", ret);
		msleep(30);
	}

	if (dump_output_frame > 0 &&
		(!dump_output_start_position ||
		(dump_output_start_position == crc32_le(0, buf, count)))) {
		char file_name[64] = {0};

		snprintf(file_name, 64, "%s/es.data", dump_path);
		dump(file_name, buf, count);
		dump_output_frame--;
		dump_output_start_position = 0;
	}

	v4l_dbg(ada_ctx->ctx, V4L_DEBUG_CODEC_INPUT,
		"write frames[%d], vbuf: %p, size: %u, ret: %d, crc: %x, ts: %llu\n",
		ada_ctx->ctx->write_frames, buf, count, ret,
		crc32_le(0, buf, count), timestamp);

	ada_ctx->ctx->write_frames++;

	return ret;
}

void vdec_vframe_input_free(void *priv, u32 handle)
{
	struct aml_vcodec_ctx *ctx = priv;

	if (ctx->output_dma_mode)
		aml_recycle_dma_buffers(ctx, handle);

	if (!vdec_input_full(ctx->ada_ctx))
		v4l2_m2m_try_schedule(ctx->m2m_ctx);
}

int vdec_vframe_write_with_dma(struct aml_vdec_adapt *ada_ctx,
	ulong addr, u32 count, u64 timestamp, u32 handle,
	chunk_free free, void* priv)
{
	int ret = -1;
	struct vdec_s *vdec = ada_ctx->vdec;
	/* set timestamp */
	vdec_set_timestamp(vdec, timestamp);

	ret = vdec_write_vframe_with_dma(vdec, addr, count,
		handle, free, priv);

	if (slow_input) {
		v4l_dbg(ada_ctx->ctx, V4L_DEBUG_CODEC_PRINFO,
			"slow_input: frame codec write size %d\n", ret);
		msleep(30);
	}

	v4l_dbg(ada_ctx->ctx, V4L_DEBUG_CODEC_INPUT,
		"write frames[%d], vbuf: %lx, size: %u, ret: %d, ts: %llu\n",
		ada_ctx->ctx->write_frames, addr, count, ret, timestamp);

	ada_ctx->ctx->write_frames++;

	return ret;
}

void aml_decoder_flush(struct aml_vdec_adapt *ada_ctx)
{
	struct vdec_s *vdec = ada_ctx->vdec;

	if (vdec)
		vdec_set_eos(vdec, true);
}

int aml_codec_reset(struct aml_vdec_adapt *ada_ctx, int *mode)
{
	struct vdec_s *vdec = ada_ctx->vdec;
	int ret = 0;

	if (vdec) {
		if (ada_ctx->ctx->v4l_resolution_change)
			*mode = V4L_RESET_MODE_LIGHT;
		else
			vdec_set_eos(vdec, false);

		v4l_dbg(ada_ctx->ctx, V4L_DEBUG_CODEC_PRINFO,
			"reset mode: %d, es frames buffering: %d\n",
			*mode, vdec_frame_number(ada_ctx));

		ret = vdec_v4l2_reset(vdec, *mode);
		*mode = V4L_RESET_MODE_NORMAL;
	}

	return ret;
}

void aml_codec_disconnect(struct aml_vdec_adapt *ada_ctx)
{
	struct vdec_s *vdec = ada_ctx->vdec;

	if (vdec) {
		vdec_disconnect(vdec);
		v4l_dbg(ada_ctx->ctx, V4L_DEBUG_CODEC_INPUT, "set vdec disconnect\n");
	}
}

void aml_codec_connect(struct aml_vdec_adapt *ada_ctx)
{
	struct vdec_s *vdec = ada_ctx->vdec;

	if (vdec) {
		vdec_connect(vdec);
		v4l_dbg(ada_ctx->ctx, V4L_DEBUG_CODEC_INPUT, "set vdec connect\n");
	}
}

bool is_input_ready(struct aml_vdec_adapt *ada_ctx)
{
	struct vdec_s *vdec = ada_ctx->vdec;
	int state = VDEC_STATUS_UNINITIALIZED;

	if (vdec) {
		state = vdec_get_status(vdec);

		if (state == VDEC_STATUS_CONNECTED
			|| state == VDEC_STATUS_ACTIVE)
			return true;
	}

	return false;
}

int vdec_frame_number(struct aml_vdec_adapt *ada_ctx)
{
	struct vdec_s *vdec = ada_ctx->vdec;

	if (vdec)
		return vdec_get_frame_num(vdec);
	else
		return -1;
}

void vdec_thread_wakeup(struct aml_vdec_adapt *ada_ctx)
{
	struct vdec_s *vdec = ada_ctx->vdec;

	if (vdec) {
		vdec_up(vdec);
	}
}

void vdec_set_dmabuf_type(struct aml_vdec_adapt *ada_ctx, bool dmabuf_type)
{
	struct vdec_s *vdec = ada_ctx->vdec;

	if (vdec) {
		if (dmabuf_type)
			vdec->port_flag |= PORT_FLAG_DMABUF;
		else
			vdec->port_flag &= ~PORT_FLAG_DMABUF;
	}
}

int vdec_get_instance_num(void)
{
	return vdec_get_core_nr();
}

bool vdec_check_is_available(u32 fmt)
{
	if ((fmt == V4L2_PIX_FMT_VC1_ANNEX_G) || (fmt == V4L2_PIX_FMT_VC1_ANNEX_L)) {
		if (vdec_get_instance_num() != 0)
			return false;
	} else {
		if (vdec_has_single_mode())
			return false;
	}

	return true;
}

int vdec_get_vdec_id(struct aml_vdec_adapt *ada_ctx)
{
	struct vdec_s *vdec = ada_ctx->vdec;

	return vdec ? vdec->id : -1;
}

void v4l2_config_vdec_parm(struct aml_vdec_adapt *ada_ctx, u8 *data, u32 len)
{
	struct vdec_s *vdec = ada_ctx->vdec;

	vdec->config_len = len > PAGE_SIZE ? PAGE_SIZE : len;
	memcpy(vdec->config, data, vdec->config_len);
}

void vdec_set_duration(s32 duration)
{
	vdec_frame_rate_uevent(duration);
}

void aml_vdec_recycle_dec_resource(struct aml_vcodec_ctx * ctx,
					struct aml_buf *aml_buf)
{
	if (ctx->vdec_recycle_dec_resource)
		ctx->vdec_recycle_dec_resource(ctx->ada_ctx->vdec->private, aml_buf);
}

void vdec_dump_strea_data(struct aml_vdec_adapt *ada_ctx, u32 addr, u32 size)
{
	char file_name[64] = {0};
	ulong buf_start = ada_ctx->vdec->vbuf.buf_start;
	ulong buf_end = ada_ctx->vdec->vbuf.buf_start + ada_ctx->vdec->vbuf.buf_size;
	u32 first_size = size;
	u32 second_size = 0;
	void *stbuf_vaddr;

	if ((addr +size) > buf_end) {
		first_size = buf_end - addr;
		second_size = size - first_size;
	}

	stbuf_vaddr = codec_mm_vmap(addr, first_size);
	if (stbuf_vaddr) {
		codec_mm_dma_flush(stbuf_vaddr, first_size, DMA_FROM_DEVICE);
		snprintf(file_name, 64, "%s/es.data", dump_path);
		dump(file_name, stbuf_vaddr, first_size);

		codec_mm_unmap_phyaddr(stbuf_vaddr);
		pr_info("dump es buffer (%x, %u)\n", addr, first_size);
	} else {
		pr_err("es buffer (%x, %u) vmap fail\n", addr, first_size);
	}

	if (second_size) {
		stbuf_vaddr = codec_mm_vmap(buf_start, second_size);
		if (stbuf_vaddr) {
			codec_mm_dma_flush(stbuf_vaddr, second_size, DMA_FROM_DEVICE);
			snprintf(file_name, 64, "%s/es.data", dump_path);
			dump(file_name, stbuf_vaddr, second_size);

			codec_mm_unmap_phyaddr(stbuf_vaddr);
			pr_info("dump next part es buffer (%lx, %u)\n", buf_start, second_size);
		} else {
			pr_err("next part es buffer (%lx, %u) vmap fail\n", buf_start, second_size);
		}
	}
}

void vdec_write_stream_data(struct aml_vdec_adapt *ada_ctx, u32 addr, u32 size)
{
	struct stream_buffer_metainfo stbuf_data = { 0 };
	stbuf_data.stbuf_pktaddr = addr;
	stbuf_data.stbuf_pktsize = size;
	stream_buffer_meta_write(&ada_ctx->vdec->vbuf, &stbuf_data);

	if (dump_es_output_frame) {
		vdec_dump_strea_data(ada_ctx, addr, size);
	}
}

void vdec_write_stream_data_inner(struct aml_vdec_adapt *ada_ctx, char *addr,
	u32 size, u64 timestamp)
{
	bool stbuf_around = false;
	u32 around_size;

	// calculate whether the remaining space is enougth
	if ((ada_ctx->vdec->vbuf.buf_wp + size) >
		(ada_ctx->vdec->vbuf.buf_start + ada_ctx->vdec->vbuf.buf_size)) {
		stbuf_around = true;
		around_size = (ada_ctx->vdec->vbuf.buf_wp + size) -
			(ada_ctx->vdec->vbuf.buf_start + ada_ctx->vdec->vbuf.buf_size);
	}

	// if not enougth, write two times
	if (!stbuf_around)
		stream_buffer_write_vc1(ada_ctx->filp, &ada_ctx->vdec->vbuf,
			addr, size);
	else {
		stream_buffer_write_vc1(ada_ctx->filp, &ada_ctx->vdec->vbuf,
			addr, size - around_size);

		stream_buffer_write_vc1(ada_ctx->filp, &ada_ctx->vdec->vbuf,
			addr + size - around_size, around_size);
	}

	v4l_dbg(ada_ctx->ctx, V4L_DEBUG_CODEC_INPUT,
		"VC1 input: es(add data size %d) -> stbuf(addr 0x%lx wp 0x%x) timestamp: %llu\n",
		size, ada_ctx->vdec->vbuf.buf_start, ada_ctx->vdec->vbuf.buf_wp, timestamp);
}

#if 0
void v4l2_set_rp_addr(struct aml_vdec_adapt *ada_ctx, struct dma_buf *dbuf)
{
	struct vdec_s *vdec = ada_ctx->vdec;
	u32 rp_addr;
	struct dmabuf_dmx_sec_es_data *es_data;

	/* get rp addr from vdec */
	rp_addr = STBUF_READ(&vdec->vbuf, get_rp);
	v4l_dbg(ada_ctx->ctx, V4L_DEBUG_CODEC_OUTPUT, "stream rp_addr is %x\n",rp_addr);

	if (dmabuf_manage_get_type(dbuf) != DMA_BUF_TYPE_DMX_ES) {
		pr_err("current dmabuf type is not DMA_BUF_TYPE_DMX_ES\n");
		return;
	}

	es_data = (struct dmabuf_dmx_sec_es_data *)dmabuf_manage_get_info(dbuf, DMA_BUF_TYPE_DMX_ES);

	es_data->buf_rp = rp_addr;
}
#endif
void v4l2_set_ext_buf_addr(struct aml_vdec_adapt *ada_ctx, struct dmabuf_dmx_sec_es_data *es_data, int offset)
{
	struct vdec_s *vdec = ada_ctx->vdec;
	u32 buf_size = 0;

	buf_size = es_data->buf_end - es_data->buf_start;
	ada_ctx->ctx->es_mgr.buf_start = es_data->buf_start;
	ada_ctx->ctx->es_mgr.buf_size = buf_size;

	stream_buffer_set_ext_buf(&vdec->vbuf, es_data->buf_start, buf_size, ada_ctx->ctx->is_drm_mode);
	vdec_init_stbuf_info(vdec);
	ada_ctx->ctx->pts_serves_ops->first_checkin(ada_ctx->ctx->output_pix_fmt, ada_ctx->ctx->ptsserver_id,
		es_data->data_start + offset, es_data->buf_start);

	return;
}
