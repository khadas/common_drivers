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
#define DEBUG
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/kfifo.h>
#include <linux/platform_device.h>
#include <linux/amlogic/media/utils/amstream.h>
#include <linux/amlogic/media/frame_sync/ptsserv.h>
#include <linux/amlogic/media/canvas/canvas.h>
#include <linux/amlogic/media/canvas/canvas_mgr.h>
#include <linux/amlogic/media/vfm/vframe.h>
#include <linux/amlogic/media/vfm/vframe_provider.h>
#include <linux/amlogic/media/vfm/vframe_receiver.h>
#include <linux/amlogic/media/codec_mm/codec_mm.h>
#include <linux/amlogic/media/codec_mm/configs.h>
#include <linux/amlogic/media/registers/register.h>

#include "../../../stream_input/amports/streambuf_reg.h"
#include "../../../stream_input/amports/amports_priv.h"
#include "../../../common/chips/decoder_cpu_ver_info.h"
#include "../../../amvdec_ports/vdec_drv_base.h"
#include "../../decoder/utils/amvdec.h"
#include "../../decoder/utils/decoder_mmu_box.h"
#include "../../decoder/utils/decoder_bmmu_box.h"
#include "../../decoder/utils/firmware.h"
#include "../../decoder/utils/vdec_feature.h"
#include "../../decoder/utils/config_parser.h"
#include "../../decoder/utils/vdec_v4l2_buffer_ops.h"
#include "../../decoder/utils/aml_buf_helper.h"
#include "../../decoder/utils/decoder_dma_alloc.h"

//#include <linux/amlogic/tee.h>
#include <uapi/linux/tee.h>
#include <linux/delay.h>

#define DRIVER_NAME "amvdec_vc1_v4l"
#define MODULE_NAME "amvdec_vc1_v4l"

#define DEBUG_PTS
#if 1	/* //MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6 */
#define NV21
#endif

#define VC1_MAX_SUPPORT_SIZE (1920*1088)

#define I_PICTURE   0
#define P_PICTURE   1
#define B_PICTURE   2

#define ORI_BUFFER_START_ADDR   0x01000000

#define INTERLACE_FLAG          0x80
#define BOTTOM_FIELD_FIRST_FLAG 0x40

/* protocol registers */
#define VC1_PIC_RATIO       AV_SCRATCH_0
#define VC1_ERROR_COUNT    AV_SCRATCH_6
#define VC1_SOS_COUNT     AV_SCRATCH_7
#define VC1_BUFFERIN       AV_SCRATCH_8
#define VC1_BUFFEROUT      AV_SCRATCH_9
#define VC1_REPEAT_COUNT    AV_SCRATCH_A
#define VC1_TIME_STAMP      AV_SCRATCH_B
#define VC1_OFFSET_REG      AV_SCRATCH_C
#define MEM_OFFSET_REG      AV_SCRATCH_F

#define CANVAS_BUF_REG      AV_SCRATCH_D
#define ANC0_CANVAS_REG     AV_SCRATCH_E
#define ANC1_CANVAS_REG     AV_SCRATCH_5
#define DECODE_STATUS       AV_SCRATCH_H
#define VC1_PIC_INFO        AV_SCRATCH_J
#define DEBUG_REG1          AV_SCRATCH_M
#define DEBUG_REG2          AV_SCRATCH_N

#define DECODE_STATUS_SEQ_HEADER_DONE 0x1
#define DECODE_STATUS_PIC_HEADER_DONE 0x2
#define DECODE_STATUS_PIC_SKIPPED     0x3
#define DECODE_STATUS_BUF_INVALID     0x4

#define NEW_DRV_VER         1

#define VF_POOL_SIZE		16
#define DECODE_BUFFER_NUM_MAX	4
#define WORKSPACE_SIZE		(2 * SZ_1M)
#define MAX_BMMU_BUFFER_NUM	(DECODE_BUFFER_NUM_MAX + 1)
#define VF_BUFFER_IDX(n)	(1 + n)
#define DCAC_BUFF_START_ADDR	0x01f00000

#define PUT_INTERVAL        (HZ/100)

#if 1	/* /MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6 */
/* TODO: move to register headers */
#define VPP_VD1_POSTBLEND       (1 << 10)
#define MEM_FIFO_CNT_BIT        16
#define MEM_LEVEL_CNT_BIT       18
#endif
static struct vdec_info *gvs;
static struct vdec_s *vdec = NULL;

static struct vframe_s *vvc1_vf_peek(void *);
static struct vframe_s *vvc1_vf_get(void *);
static void vvc1_vf_put(struct vframe_s *, void *);
static int vvc1_vf_states(struct vframe_states *states, void *);
static int vvc1_event_cb(int type, void *data, void *private_data);

static int vvc1_prot_init(void);
static void vvc1_local_init(bool is_reset);

static const char vvc1_dec_id[] = "vvc1-dev";

#define PROVIDER_NAME   "decoder.vc1"
static const struct vframe_operations_s vvc1_vf_provider = {
	.peek = vvc1_vf_peek,
	.get = vvc1_vf_get,
	.put = vvc1_vf_put,
	.event_cb = vvc1_event_cb,
	.vf_states = vvc1_vf_states,
};
static void *mm_blk_handle;
static struct vframe_provider_s vvc1_vf_prov;

static DECLARE_KFIFO(newframe_q, struct vframe_s *, VF_POOL_SIZE);
static DECLARE_KFIFO(display_q, struct vframe_s *, VF_POOL_SIZE);
static DECLARE_KFIFO(recycle_q, struct vframe_s *, VF_POOL_SIZE);

static struct vframe_s vfpool[VF_POOL_SIZE];
static struct vframe_s vfpool2[VF_POOL_SIZE];
static int cur_pool_idx;
static struct timer_list recycle_timer;
static u32 stat;
static u32 buf_offset;
static u32 avi_flag;
static u32 unstable_pts_debug;
static u32 unstable_pts;
static u32 vvc1_ratio;
static u32 vvc1_format;

static u32 intra_output;
static u32 frame_width, frame_height, frame_dur;
static u32 saved_resolution;
static u32 pts_by_offset = 1;
static u32 total_frame;
static u32 next_pts;
static u64 next_pts_us64;
static bool is_reset;
static struct work_struct set_clk_work;
static struct work_struct error_wd_work;
static struct canvas_config_s vc1_canvas_config[DECODE_BUFFER_NUM_MAX][3];
spinlock_t vc1_rp_lock;

#ifdef DEBUG_PTS
static u32 pts_hit, pts_missed, pts_i_hit, pts_i_missed;
#endif
static DEFINE_SPINLOCK(lock);

static struct dec_sysinfo vvc1_amstream_dec_info;

struct frm_s {
	int state;
	u32 start_pts;
	int num;
	u32 end_pts;
	u32 rate;
	u32 trymax;
};

static struct frm_s frm;

enum {
	RATE_MEASURE_START_PTS = 0,
	RATE_MEASURE_END_PTS,
	RATE_MEASURE_DONE
};
#define RATE_MEASURE_NUM 8
#define RATE_CORRECTION_THRESHOLD 5
#define RATE_24_FPS  3755	/* 23.97 */
#define RATE_30_FPS  3003	/* 29.97 */
#define DUR2PTS(x) ((x)*90/96)
#define PTS2DUR(x) ((x)*96/90)

#define I_PICTURE 0
#define P_PICTURE 1
#define B_PICTURE 2
#define BI_PICTURE 3

#define VC1_DEBUG_DETAIL                   0x01

#define INVALID_IDX -1  /* Invalid buffer index.*/

static u32 udebug_flag;
static int debug;
unsigned int debug_mask = 0xff;
static u32 wait_time = 5;

bool process_busy;

struct pic_info_t {
	u32 buffer_info;
	u32 index;
	u32 offset;
	u32 width;
	u32 height;
	u32 pts;
	u64 pts64;
	bool pts_valid;
	ulong v4l_ref_buf_addr;
	ulong cma_alloc_addr;
	u32 hw_decode_time;
	u32 frame_size; // For frame base mode
	u64 timestamp;
	u32 picture_type;
	unsigned short decode_pic_count;
	u32 repeat_cnt;
};

struct vdec_vc1_hw_s {
	spinlock_t lock;
	struct platform_device *platform_dev;
	s32 vfbuf_use[DECODE_BUFFER_NUM_MAX];
	unsigned char again_flag;
	unsigned char recover_flag;
	u32 frame_width;
	u32 frame_height;
	u32 frame_dur;
	u32 frame_prog;
	u32 saved_resolution;
	u32 avi_flag;
	u32 vavs_ratio;
	u32 pic_type;

	u32 vf_buf_num_used;
	u32 total_frame;
	u32 next_pts;
	unsigned char throw_pb_flag;

	/*debug*/
	u32 ucode_pause_pos;
	u32 decode_pic_count;
	u8 reset_decode_flag;
	u32 display_frame_count;
	u32 buf_status;
	u32 pre_parser_wr_ptr;
	u32 eos;
	s32 refs[2];
	atomic_t prepare_num;
	atomic_t put_num;
	atomic_t peek_num;
	atomic_t get_num;
	s32 ref_use[DECODE_BUFFER_NUM_MAX];
	s32 buf_use[DECODE_BUFFER_NUM_MAX];
	s32 vf_ref[DECODE_BUFFER_NUM_MAX];
	u32 decoding_index;
	struct pic_info_t pics[DECODE_BUFFER_NUM_MAX];
	u32 interlace_flag;
	u32 new_type;
	void *v4l2_ctx;
	struct aml_buf *aml_buf;
	u32 res_ch_flag;
	u32 last_width;
	u32 last_height;
	bool v4l_params_parsed;
	struct vframe_s vframe_dummy;
	u32 dynamic_buf_num_margin;
	u32 cur_duration;
	u32 canvas_mode;
	u8 is_decoder_working;
	u32 last_wp;
	u32 last_rp;
};

struct vdec_vc1_hw_s vc1_hw;
static struct task_ops_s task_dec_ops;
static u32 run_ready_min_buf_num = 1;
static u32 default_vc1_margin = 2;
static unsigned int start_decode_buf_level = 0x50;
static unsigned int timeout_times = 0;

#undef pr_info
#define pr_info pr_cont

static int prepare_display_buf(struct vdec_vc1_hw_s *hw, struct pic_info_t *pic);
static int find_free_buffer(struct vdec_vc1_hw_s *hw);
static void flush_output(struct vdec_vc1_hw_s * hw);
static int notify_v4l_eos(void);

int vc1_print(int index, int debug_flag, const char *fmt, ...)
{
	if ((debug_flag == 0) ||
		((debug & debug_flag) &&
		((1 << index) & debug_mask))) {
		unsigned char *buf = kzalloc(512, GFP_ATOMIC);
		int len = 0;
		va_list args;

		if (!buf)
			return 0;

		va_start(args, fmt);
		len = sprintf(buf, "%d: ", index);
		vsnprintf(buf + len, 512-len, fmt, args);
		pr_info("%s", buf);
		va_end(args);
		kfree(buf);
	}
	return 0;
}

static inline int pool_index(struct vframe_s *vf)
{
	if ((vf >= &vfpool[0]) && (vf <= &vfpool[VF_POOL_SIZE - 1]))
		return 0;
	else if ((vf >= &vfpool2[0]) && (vf <= &vfpool2[VF_POOL_SIZE - 1]))
		return 1;
	else
		return -1;
}

static inline bool close_to(int a, int b, int m)
{
	return abs(a - b) < m;
}

static inline u32 index2canvas(u32 index)
{
	const u32 canvas_tab[DECODE_BUFFER_NUM_MAX] = {
#if 1	/* ALWAYS.MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6 */
	0x010100, 0x030302, 0x050504, 0x070706/*,
	0x090908, 0x0b0b0a, 0x0d0d0c, 0x0f0f0e*/
#else
		0x020100, 0x050403, 0x080706, 0x0b0a09
#endif
	};

	return canvas_tab[index];
}

static void set_aspect_ratio(struct vframe_s *vf, unsigned int pixel_ratio)
{
	int ar = 0;

	if (vvc1_ratio == 0) {
		/* always stretch to 16:9 */
		vf->ratio_control |= (0x90 << DISP_RATIO_ASPECT_RATIO_BIT);
	} else if (pixel_ratio > 0x0f) {
		ar = (vvc1_amstream_dec_info.height * (pixel_ratio & 0xff) *
			  vvc1_ratio) / (vvc1_amstream_dec_info.width *
							 (pixel_ratio >> 8));
	} else {
		switch (pixel_ratio) {
		case 0:
			ar = (vvc1_amstream_dec_info.height * vvc1_ratio) /
				 vvc1_amstream_dec_info.width;
			break;
		case 1:
			vf->sar_width = 1;
			vf->sar_height = 1;
			ar = (vf->height * vvc1_ratio) / vf->width;
			break;
		case 2:
			vf->sar_width = 12;
			vf->sar_height = 11;
			ar = (vf->height * 11 * vvc1_ratio) / (vf->width * 12);
			break;
		case 3:
			vf->sar_width = 10;
			vf->sar_height = 11;
			ar = (vf->height * 11 * vvc1_ratio) / (vf->width * 10);
			break;
		case 4:
			vf->sar_width = 16;
			vf->sar_height = 11;
			ar = (vf->height * 11 * vvc1_ratio) / (vf->width * 16);
			break;
		case 5:
			vf->sar_width = 40;
			vf->sar_height = 33;
			ar = (vf->height * 33 * vvc1_ratio) / (vf->width * 40);
			break;
		case 6:
			vf->sar_width = 24;
			vf->sar_height = 11;
			ar = (vf->height * 11 * vvc1_ratio) / (vf->width * 24);
			break;
		case 7:
			vf->sar_width = 20;
			vf->sar_height = 11;
			ar = (vf->height * 11 * vvc1_ratio) / (vf->width * 20);
			break;
		case 8:
			vf->sar_width = 32;
			vf->sar_height = 11;
			ar = (vf->height * 11 * vvc1_ratio) / (vf->width * 32);
			break;
		case 9:
			vf->sar_width = 80;
			vf->sar_height = 33;
			ar = (vf->height * 33 * vvc1_ratio) / (vf->width * 80);
			break;
		case 10:
			vf->sar_width = 18;
			vf->sar_height = 11;
			ar = (vf->height * 11 * vvc1_ratio) / (vf->width * 18);
			break;
		case 11:
			vf->sar_width = 15;
			vf->sar_height = 11;
			ar = (vf->height * 11 * vvc1_ratio) / (vf->width * 15);
			break;
		case 12:
			vf->sar_width = 64;
			vf->sar_height = 33;
			ar = (vf->height * 33 * vvc1_ratio) / (vf->width * 64);
			break;
		case 13:
			vf->sar_width = 160;
			vf->sar_height = 99;
			ar = (vf->height * 99 * vvc1_ratio) /
				(vf->width * 160);
			break;
		default:
			vf->sar_width = 1;
			vf->sar_height = 1;
			ar = (vf->height * vvc1_ratio) / vf->width;
			break;
		}
	}

	ar = min(ar, DISP_RATIO_ASPECT_RATIO_MAX);

	vf->ratio_control = (ar << DISP_RATIO_ASPECT_RATIO_BIT);
	/*vf->ratio_control |= DISP_RATIO_FORCECONFIG | DISP_RATIO_KEEPRATIO;*/
}

static void vc1_set_rp(void) {
	unsigned long flags;

	if (!(stat & STAT_VDEC_RUN))
		return;

	spin_lock_irqsave(&vc1_rp_lock, flags);
	STBUF_WRITE(&vdec->vbuf, set_rp,
		READ_VREG(VLD_MEM_VIFIFO_RP));
	spin_unlock_irqrestore(&vc1_rp_lock, flags);
}

static void recycle_frames(struct vdec_vc1_hw_s *hw)
{
	while (!kfifo_is_empty(&recycle_q)/* && (READ_VREG(VC1_BUFFERIN) == 0)*/) {
		struct vframe_s *vf;

		if (kfifo_get(&recycle_q, &vf)) {
			if ((vf->index < hw->vf_buf_num_used) &&
			 (--hw->vfbuf_use[vf->index] == 0)) {
				hw->buf_use[vf->index]--;
				vc1_print(0, VC1_DEBUG_DETAIL,
					"%s WRITE_VREG(VC1_BUFFERIN, 0x%x) for vf index of %d,  buf_use %d\n",
					__func__, ~(1 << vf->index), vf->index, hw->buf_use[vf->index]);
				WRITE_VREG(VC1_BUFFERIN, ~(1 << vf->index));
				vf->index = hw->vf_buf_num_used;
			}
			if (pool_index(vf) == cur_pool_idx)
				kfifo_put(&newframe_q, (const struct vframe_s *)vf);
		}

	}
}

static int vc1_recycle_frame_buffer(struct vdec_vc1_hw_s *hw)
{
	struct aml_vcodec_ctx *ctx =
		(struct aml_vcodec_ctx *)(hw->v4l2_ctx);
	struct aml_buf *aml_buf;
	ulong flags;
	int i;

	for (i = 0; i < hw->vf_buf_num_used; ++i) {
		if ((hw->vf_ref[i]) &&
			!(hw->ref_use[i]) &&
			hw->pics[i].v4l_ref_buf_addr){
			aml_buf = (struct aml_buf *)hw->pics[i].v4l_ref_buf_addr;

			vc1_print(0, VC1_DEBUG_DETAIL,
				"%s buf idx: %d dma addr: 0x%lx fb idx: %d vf_ref %d\n",
				__func__, i, hw->pics[i].cma_alloc_addr,
				aml_buf->index,
				hw->vf_ref[i]);
			if ((ctx->vpp_is_need || ctx->enable_di_post) &&
				hw->interlace_flag &&
				hw->vf_ref[i] < 2)
				continue;
			aml_buf_put_ref(&ctx->bm, aml_buf);
			spin_lock_irqsave(&hw->lock, flags);

			hw->pics[i].v4l_ref_buf_addr = 0;
			hw->pics[i].cma_alloc_addr = 0;
			while (hw->vf_ref[i]) {
				atomic_add(1, &hw->put_num);
				hw->vf_ref[i]--;
			}
			spin_unlock_irqrestore(&hw->lock, flags);

			break;
		}
	}

	return 0;
}

static bool is_available_buffer(struct vdec_vc1_hw_s *hw)
{
	struct aml_vcodec_ctx *ctx =
		(struct aml_vcodec_ctx *)(hw->v4l2_ctx);
	struct aml_buf *sub_buf;
	int i, free_count = 0;
	int free_slot =0;

	/* Ignore the buffer available check until the head parse done. */
	if (!hw->v4l_params_parsed) {
		/*
		 * If a resolution change and eos are detected, decoding will
		 * wait until the first valid buffer queue in driver
		 * before scheduling continues.
		 */
		if (ctx->v4l_resolution_change) {
			if (hw->eos)
				return false;

			/* Wait for buffers ready. */
			if (!ctx->dst_queue_streaming)
				return false;
		} else {
			return true;
		}
	}

	vc1_recycle_frame_buffer(hw);

	/* Wait for the buffer number negotiation to complete. */
	for (i = 0; i < hw->vf_buf_num_used; ++i) {
		if ((hw->vf_ref[i] == 0) &&
			(hw->vfbuf_use[i] == 0) &&
			(hw->ref_use[i] == 0) &&
			!hw->pics[i].v4l_ref_buf_addr) {
			free_slot++;

			break;
		}
	}

	if (!free_slot) {
		vc1_print(0, 0,
			"%s not enough free_slot %d!\n",
		__func__, free_slot);
		for (i = 0; i < hw->vf_buf_num_used; ++i) {
			vc1_print(0, VC1_DEBUG_DETAIL,
				"%s idx %d ref_count %d vf_ref %d cma_alloc_addr = 0x%lx\n",
				__func__, i, hw->ref_use[i],
				hw->vfbuf_use[i],
				hw->pics[i].v4l_ref_buf_addr);
		}

		return false;
	}

	if (((hw->interlace_flag) &&
		atomic_read(&ctx->vpp_cache_num) > 1) ||
		atomic_read(&ctx->vpp_cache_num) >= MAX_VPP_BUFFER_CACHE_NUM ||
		atomic_read(&ctx->ge2d_cache_num) > 1) {
		vc1_print(0, 0,
			"%s vpp or ge2d cache: %d/%d full!\n",
		__func__, atomic_read(&ctx->vpp_cache_num), atomic_read(&ctx->ge2d_cache_num));

		return false;
	}

	if (!hw->aml_buf && !aml_buf_empty(&ctx->bm)) {
		hw->aml_buf = aml_buf_get(&ctx->bm, BUF_USER_DEC, false);
		if (!hw->aml_buf) {
			return false;
		}
		hw->aml_buf->task->attach(hw->aml_buf->task, &task_dec_ops, &vc1_hw);
		hw->aml_buf->state = FB_ST_DECODER;
		if (hw->aml_buf->sub_buf[0]) {
			sub_buf = (struct aml_buf *)hw->aml_buf->sub_buf[0];
			sub_buf->task->attach(sub_buf->task, &task_dec_ops, &vc1_hw);
			sub_buf->state = FB_ST_DECODER;
		}
		if (hw->aml_buf->sub_buf[1]) {
			sub_buf = (struct aml_buf *)hw->aml_buf->sub_buf[1];
			sub_buf->task->attach(sub_buf->task, &task_dec_ops, &vc1_hw);
			sub_buf->state = FB_ST_DECODER;
		}
	}

	if (hw->aml_buf) {
		free_count++;
		free_count += aml_buf_ready_num(&ctx->bm);
		vc1_print(0, VC1_DEBUG_DETAIL,
			"%s get fb: 0x%lx fb idx: %d\n",
			__func__, hw->aml_buf, hw->aml_buf->index);
	}

	return free_count >= run_ready_min_buf_num ? 1 : 0;
}

static void flush_output(struct vdec_vc1_hw_s * hw)
{
	struct pic_info_t *pic;

	if (hw->refs[1] >= 0 &&
		hw->refs[1] < DECODE_BUFFER_NUM_MAX &&
		hw->vfbuf_use[hw->refs[1]] > 0) {
		pic = &hw->pics[hw->refs[1]];
		prepare_display_buf(hw, pic);
	}
}

static int notify_v4l_eos(void)
{
	struct vdec_vc1_hw_s *hw = &vc1_hw;
	struct aml_vcodec_ctx *ctx = (struct aml_vcodec_ctx *)(hw->v4l2_ctx);
	struct vframe_s *vf = &hw->vframe_dummy;
	struct aml_buf *aml_buf = NULL;
	int index = INVALID_IDX;
	ulong expires;

	expires = jiffies + msecs_to_jiffies(2000);

	while (!is_available_buffer(hw)) {
		if (time_after(jiffies, expires)) {
			pr_info("[%d] VC1 isn't enough buff for notify eos.\n", ctx->id);
			return 0;
		}
	}

	index = find_free_buffer(hw);
	if (INVALID_IDX == index) {
		pr_info("[%d] VC1 EOS get free buff fail.\n", ctx->id);
		return 0;
	}

	aml_buf = (struct aml_buf *)
		hw->pics[index].v4l_ref_buf_addr;

	vf->type		|= VIDTYPE_V4L_EOS;
	vf->timestamp		= ULONG_MAX;
	vf->flag		= VFRAME_FLAG_EMPTY_FRAME_V4L;
	vf->v4l_mem_handle	= (ulong)aml_buf;

	//vdec_vframe_ready(vdec, vf);
	kfifo_put(&display_q, (const struct vframe_s *)vf);

	aml_buf_done(&ctx->bm, aml_buf, BUF_USER_DEC);

	hw->eos = true;

	vc1_print(0, 0, "[%d] VC1 EOS notify.\n", ctx->id);

	return 0;
}

static int vvc1_get_ps_info(struct vdec_vc1_hw_s *hw, struct aml_vdec_ps_infos *ps)
{
	ps->visible_width 	= hw->frame_width;
	ps->visible_height 	= hw->frame_height;
	ps->coded_width 	= ALIGN(hw->frame_width, 64);
	ps->coded_height 	= ALIGN(hw->frame_height, 64);
	ps->dpb_size 		= hw->vf_buf_num_used;
	ps->dpb_margin		= hw->dynamic_buf_num_margin;
	ps->dpb_frames		= DECODE_BUFFER_NUM_MAX;

	ps->field		= hw->interlace_flag ?
		V4L2_FIELD_INTERLACED : V4L2_FIELD_NONE;

	return 0;
}

static int v4l_res_change(struct vdec_vc1_hw_s *hw)
{
	struct aml_vcodec_ctx *ctx =
		(struct aml_vcodec_ctx *)(hw->v4l2_ctx);
	int ret = 0;

	if (ctx->param_sets_from_ucode &&
		hw->res_ch_flag == 0) {
		struct aml_vdec_ps_infos ps;

		if ((hw->last_width != 0 &&
			hw->last_height != 0) &&
			(hw->frame_width != hw->last_width ||
			hw->frame_height != hw->last_height)) {

			vc1_print(0, 0, "%s (%d,%d)=>(%d,%d)\r\n", __func__, hw->last_width,
				hw->last_height, hw->frame_width, hw->frame_height);

			vvc1_get_ps_info(hw, &ps);
			vdec_v4l_set_ps_infos(ctx, &ps);
			vdec_v4l_res_ch_event(ctx);
			ctx->decoder_status_info.frame_height = ps.visible_height;
			ctx->decoder_status_info.frame_width = ps.visible_width;

			hw->v4l_params_parsed = false;
			hw->res_ch_flag = 1;
			ctx->v4l_resolution_change = 1;
			flush_output(hw);
			notify_v4l_eos();
			ret = 1;
		}
	}

	return ret;
}

static int vvc1_config_ref_buf(struct vdec_vc1_hw_s *hw)
{
	vc1_print(0, VC1_DEBUG_DETAIL,"%s: new_type %d\n", __func__, hw->new_type);

	if (hw->new_type != B_PICTURE) {
		if (hw->refs[1] == -1) {
			WRITE_VREG(ANC0_CANVAS_REG, 0xffffffff);
		} else {
			WRITE_VREG(ANC0_CANVAS_REG, index2canvas(hw->refs[1]));
		}

		if (hw->refs[0] == -1) {
			WRITE_VREG(ANC1_CANVAS_REG, 0xffffffff);
		} else {
			WRITE_VREG(ANC1_CANVAS_REG, index2canvas(hw->refs[0]));
		}
	} else {
		if (hw->refs[0] == -1) {
			WRITE_VREG(ANC0_CANVAS_REG, 0xffffffff);
		} else {
			WRITE_VREG(ANC0_CANVAS_REG, index2canvas(hw->refs[0]));
		}

		if (hw->refs[1] == -1) {
			WRITE_VREG(ANC1_CANVAS_REG, 0xffffffff);
		} else {
			WRITE_VREG(ANC1_CANVAS_REG, index2canvas(hw->refs[1]));
		}
	}

	return 0;
}

static int update_reference(struct vdec_vc1_hw_s *hw,	int index)
{
	hw->ref_use[index]++;
	if (hw->refs[1] == -1) {
		hw->refs[1] = index;
		/*
		* first pic need output to show
		* usecnt do not decrease.
		*/
	} else if (hw->refs[0] == -1) {
		hw->refs[0] = hw->refs[1];
		hw->refs[1] = index;
		/* second pic do not output */
		index = hw->vf_buf_num_used;
	} else {
		hw->ref_use[hw->refs[0]]--; 	//old ref0 unused
		hw->refs[0] = hw->refs[1];
		hw->refs[1] = index;
		index = hw->refs[0];
	}
	vc1_print(0, VC1_DEBUG_DETAIL,"%s: hw->refs[0] = %d, hw->refs[1] = %d\n", __func__, hw->refs[0], hw->refs[1]);
	return index;
}

static void vc1_put_video_frame(void *vdec_ctx, struct vframe_s *vf)
{
	vvc1_vf_put(vf, vdec_ctx);
}

static void vc1_get_video_frame(void *vdec_ctx, struct vframe_s *vf)
{
	memcpy(vf, vvc1_vf_get(vdec_ctx), sizeof(struct vframe_s));
}

static struct task_ops_s task_dec_ops = {
	.type		= TASK_TYPE_DEC,
	.get_vframe	= vc1_get_video_frame,
	.put_vframe	= vc1_put_video_frame,
};

static int v4l_alloc_buff_config_canvas(struct vdec_vc1_hw_s *hw, int i)
{
	ulong decbuf_start = 0, decbuf_uv_start = 0;
	int decbuf_y_size = 0, decbuf_uv_size = 0;
	u32 canvas_width = 0, canvas_height = 0;
	struct aml_buf *aml_buf = hw->aml_buf;
	struct aml_vcodec_ctx *ctx =
		(struct aml_vcodec_ctx *)(hw->v4l2_ctx);

	if (!aml_buf) {
		vc1_print(0, 0, "%s not get aml_buf \n", __func__);
		return -1;
	}

	if (!hw->frame_width || !hw->frame_height) {
		struct vdec_pic_info pic;
		vdec_v4l_get_pic_info(ctx, &pic);
		hw->frame_width = pic.visible_width;
		hw->frame_height = pic.visible_height;
		vc1_print(0, 0, "[%d] set %d x %d from IF layer\n", ctx->id,
			hw->frame_width, hw->frame_height);
	}

	hw->pics[i].v4l_ref_buf_addr = (ulong)aml_buf;
	hw->pics[i].cma_alloc_addr = aml_buf->planes[0].addr;
	if (aml_buf->num_planes == 1) {
		decbuf_start	= aml_buf->planes[0].addr;
		decbuf_y_size	= aml_buf->planes[0].offset;
		decbuf_uv_start	= decbuf_start + decbuf_y_size;
		decbuf_uv_size	= decbuf_y_size / 2;
		canvas_width	= ALIGN(hw->frame_width, 64);
		canvas_height	= ALIGN(hw->frame_height, 64);
		aml_buf->planes[0].bytes_used = aml_buf->planes[0].length;
	} else if (aml_buf->num_planes == 2) {
		decbuf_start	= aml_buf->planes[0].addr;
		decbuf_y_size	= aml_buf->planes[0].length;
		decbuf_uv_start	= aml_buf->planes[1].addr;
		decbuf_uv_size	= aml_buf->planes[1].length;
		canvas_width	= ALIGN(hw->frame_width, 64);
		canvas_height	= ALIGN(hw->frame_height, 64);
		aml_buf->planes[0].bytes_used = decbuf_y_size;
		aml_buf->planes[1].bytes_used = decbuf_uv_size;
	}

	/* setting canvas */
	vc1_canvas_config[i][0].width 		= canvas_width;
	vc1_canvas_config[i][0].height		= canvas_height;
	vc1_canvas_config[i][0].phy_addr 	= decbuf_start;
	vc1_canvas_config[i][0].block_mode 	= CANVAS_BLKMODE_32X32;
	vc1_canvas_config[i][0].endian 		= 0;
	config_cav_lut_ex(i * 2 + 0,
		vc1_canvas_config[i][0].phy_addr,
		vc1_canvas_config[i][0].width,
		vc1_canvas_config[i][0].height,
		CANVAS_ADDR_NOWRAP,
		vc1_canvas_config[i][0].block_mode,
		vc1_canvas_config[i][0].endian,
		VDEC_1);

	vc1_canvas_config[i][1].width 		= canvas_width;
	vc1_canvas_config[i][1].height 		= canvas_height >> 1;
	vc1_canvas_config[i][1].phy_addr 	= decbuf_uv_start;
	vc1_canvas_config[i][1].block_mode 	= CANVAS_BLKMODE_32X32;
	vc1_canvas_config[i][1].endian 		= 0;
	config_cav_lut_ex(i * 2 + 1,
		vc1_canvas_config[i][1].phy_addr,
		vc1_canvas_config[i][1].width,
		vc1_canvas_config[i][1].height,
		CANVAS_ADDR_NOWRAP,
		vc1_canvas_config[i][1].block_mode,
		vc1_canvas_config[i][1].endian,
		VDEC_1);

	vc1_print(0, VC1_DEBUG_DETAIL, "[%d] %s y: %x uv: %x w: %d h: %d canvas_mode 0x%x endian %d \n",
		ctx->id, __func__,
		decbuf_start, decbuf_uv_start,
		canvas_width, canvas_height,
		vc1_canvas_config[i][0].block_mode,
		vc1_canvas_config[i][0].endian);

	aml_buf_get_ref(&ctx->bm, aml_buf);
	if ((ctx->vpp_is_need || ctx->enable_di_post) &&
		hw->interlace_flag) {
		aml_buf_get_ref(&ctx->bm, aml_buf);
	}

	hw->aml_buf = NULL;

	return 0;
}

static void reset(struct vdec_s *vdec)
{
	struct vdec_vc1_hw_s *hw = &vc1_hw;
	struct aml_vcodec_ctx *ctx =
		(struct aml_vcodec_ctx *)(hw->v4l2_ctx);
	int i;
	ulong timeout;

	if (stat & STAT_VDEC_RUN) {
		amvdec_stop();
		stat &= ~STAT_VDEC_RUN;
	}

	timeout = jiffies + HZ / 10;
	while (hw->is_decoder_working) {
		if (time_after(jiffies, timeout)) {
			vc1_print(0, 0,
			"reset...wait timeout!\n");
			break;
		}

		usleep_range(500, 550);
	}

	vdec_v4l_inst_reset(ctx);

	INIT_KFIFO(display_q);
	INIT_KFIFO(recycle_q);
	INIT_KFIFO(newframe_q);

	for (i = 0; i < VF_POOL_SIZE; i++) {
		const struct vframe_s *vf = NULL;

		if (cur_pool_idx == 0) {
			vf = &vfpool[i];
			vfpool[i].index = DECODE_BUFFER_NUM_MAX;
		} else {
			vf = &vfpool2[i];
			vfpool2[i].index = DECODE_BUFFER_NUM_MAX;
		}

		memset((void *)vf, 0, sizeof(*vf));
		kfifo_put(&newframe_q, vf);
	}

	for (i = 0; i < hw->vf_buf_num_used; i++) {
		hw->pics[i].v4l_ref_buf_addr = 0;
		hw->pics[i].cma_alloc_addr = 0;
		hw->vfbuf_use[i] = 0;
		hw->ref_use[i] = 0;
		hw->buf_use[i] = 0;
		hw->vf_ref[i] = 0;
	}

	hw->refs[0]		= -1;
	hw->refs[1]		= -1;
	hw->eos			= 0;
	hw->aml_buf		= NULL;

	atomic_set(&hw->get_num, 0);
	atomic_set(&hw->put_num, 0);
	WRITE_VREG(VC1_BUFFEROUT, NEW_DRV_VER); //reuse the register VC1_BUFFEROUT to support new ucode version

	pr_info("vc1: reset.\n");
}

static int find_free_buffer(struct vdec_vc1_hw_s *hw)
{
	int i;

	for (i = 0; i < hw->vf_buf_num_used; i++) {
		vc1_print(0, VC1_DEBUG_DETAIL,"%s: i %d, vfbuf_use %d, ref_use %d, buf_use %d\n", __func__,
			i, hw->vfbuf_use[i], hw->ref_use[i], hw->buf_use[i]);
		if ((hw->vf_ref[i] == 0) &&
			(hw->vfbuf_use[i] == 0) &&
			(hw->ref_use[i] == 0) &&
			(hw->buf_use[i] == 0) &&
			!hw->pics[i].v4l_ref_buf_addr) {
			break;
		}
	}

	if ((i == hw->vf_buf_num_used) &&
		(hw->vf_buf_num_used != 0)) {
		vc1_print(0, VC1_DEBUG_DETAIL, "%s: buf INVALID_IDX\n", __func__);
		return INVALID_IDX;
	}

	if (v4l_alloc_buff_config_canvas(hw, i))
		return INVALID_IDX;

	return i;
}

static int vvc1_config_buf(struct vdec_vc1_hw_s *hw)
{
	u32 index = -1;
	u32 canvas1_info = 0;
	index = find_free_buffer(hw);
	if (index == INVALID_IDX) {
		WRITE_VREG(CANVAS_BUF_REG, 0xff);
		return -1;
	}
	hw->decoding_index = index;
	canvas1_info = (index2canvas(index) << 8) | index;
	vc1_print(0, VC1_DEBUG_DETAIL,"%s: i %d, buf_use %d, canvas1_info 0x%x\n",
		__func__, index, hw->buf_use[hw->decoding_index], canvas1_info);
	WRITE_VREG(CANVAS_BUF_REG, canvas1_info);

	vvc1_config_ref_buf(hw);
	return 0;
}

static int prepare_display_buf(struct vdec_vc1_hw_s *hw,	struct pic_info_t *pic)
{
	struct vframe_s *vf = NULL;
	u32 reg = pic->buffer_info;
	u32 picture_type = pic->picture_type;
	u32 repeat_count = pic->repeat_cnt;
	unsigned int pts = pic->pts;
	unsigned int pts_valid = pic->pts_valid;
	//unsigned int offset = pic->offset;
	u64 pts_us64 = pic->pts64;
	u32 buffer_index = pic->index;
	struct aml_vcodec_ctx *ctx =
		(struct aml_vcodec_ctx *)(hw->v4l2_ctx);
	struct aml_buf *aml_buf = NULL;

	if (!hw->pics[buffer_index].v4l_ref_buf_addr) {
		vc1_print(0, 0, "%s do not get aml_buf! \n", __func__);
		return -1;
	}

	vc1_print(0, VC1_DEBUG_DETAIL, "%s: buffer_info 0x%x, index %d, picture_type %d\n",
					__func__, reg, buffer_index, picture_type);

	if (hw->interlace_flag &&
		(ctx->vpp_is_need || ctx->enable_di_post)) { /* interlace */
		vc1_print(0, VC1_DEBUG_DETAIL, "%s: interlace reg 0x%x\n", __func__, reg);
		hw->throw_pb_flag = 0;
		if (kfifo_get(&newframe_q, &vf) == 0) {
			pr_info
			("fatal error, no available buffer slot.");
			return IRQ_HANDLED;
		}
		vf->signal_type = 0;
		vf->index = buffer_index;
		vf->width = vvc1_amstream_dec_info.width;
		vf->height = vvc1_amstream_dec_info.height;
		vf->bufWidth = 1920;
		vf->flag = 0;

		if (pts_valid) {
			vf->pts = pts;
			vf->pts_us64 = pts_us64;
			if ((repeat_count > 1) && avi_flag) {
				vf->duration =
					vvc1_amstream_dec_info.rate *
					repeat_count >> 1;
				next_pts = pts +
					(vvc1_amstream_dec_info.rate *
					 repeat_count >> 1) * 15 / 16;
				next_pts_us64 = pts_us64 +
					((vvc1_amstream_dec_info.rate *
					repeat_count >> 1) * 15 / 16) *
					100 / 9;
			} else {
				vf->duration =
				vvc1_amstream_dec_info.rate >> 1;
				next_pts = 0;
				next_pts_us64 = 0;
				if (picture_type != I_PICTURE &&
					unstable_pts) {
					vf->pts = 0;
					vf->pts_us64 = 0;
				}
			}
		} else {
			vf->pts = next_pts;
			vf->pts_us64 = next_pts_us64;
			if ((repeat_count > 1) && avi_flag) {
				vf->duration =
					vvc1_amstream_dec_info.rate *
					repeat_count >> 1;
				if (next_pts != 0) {
					next_pts += ((vf->duration) -
					((vf->duration) >> 4));
				}
				if (next_pts_us64 != 0) {
					next_pts_us64 +=
					div_u64((u64)((vf->duration) -
					((vf->duration) >> 4)) *
					100, 9);
				}
			} else {
				vf->duration =
				vvc1_amstream_dec_info.rate >> 1;
				next_pts = 0;
				next_pts_us64 = 0;
				if (picture_type != I_PICTURE &&
					unstable_pts) {
					vf->pts = 0;
					vf->pts_us64 = 0;
				}
			}
		}

		vf->duration_pulldown = 0;
		vf->type = (reg & BOTTOM_FIELD_FIRST_FLAG) ?
		VIDTYPE_INTERLACE_BOTTOM : VIDTYPE_INTERLACE_TOP;
#ifdef NV21
		vf->type |= VIDTYPE_VIU_NV21;
#endif
		vf->canvas0Addr = vf->canvas1Addr =
					index2canvas(buffer_index);
		vf->orientation = 0;
		vf->type_original = vf->type;
		set_aspect_ratio(vf, READ_VREG(VC1_PIC_RATIO));

		hw->vfbuf_use[buffer_index]++;
		hw->vf_ref[buffer_index]++;
		vf->v4l_mem_handle = hw->pics[buffer_index].v4l_ref_buf_addr;
		aml_buf = (struct aml_buf *)vf->v4l_mem_handle;
		vf->pts_us64 = pts_us64;
		vf->timestamp = pts_us64;

		vf->canvas0Addr = vf->canvas1Addr = -1;
		vf->canvas0_config[0] = vc1_canvas_config[buffer_index][0];
		vf->canvas0_config[1] = vc1_canvas_config[buffer_index][1];
#ifdef NV21
		vf->plane_num = 2;
#else
		vf->canvas0_config[2] = vc1_canvas_config[buffer_index][2];
		vf->plane_num = 3;
#endif

		if (get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_T5D && vdec->use_vfm_path &&
			vdec_stream_based(vdec)) {
			vf->type |= VIDTYPE_FORCE_SIGN_IP_JOINT;
		}
		kfifo_put(&display_q, (const struct vframe_s *)vf);
		ATRACE_COUNTER(MODULE_NAME, vf->pts);

		if (ctx->is_stream_off) {
			vvc1_vf_put(vvc1_vf_get(vdec), vdec);
		} else {
			if (ctx->enable_di_post)
				ctx->fbc_transcode_and_set_vf(ctx, aml_buf, vf);
			aml_buf_set_vframe(aml_buf, vf);
			aml_buf_done(&ctx->bm, aml_buf, BUF_USER_DEC);
		}

		if (kfifo_get(&newframe_q, &vf) == 0) {
			pr_info
			("fatal error, no available buffer slot.");
			return IRQ_HANDLED;
		}
		vf->signal_type = 0;
		vf->index = buffer_index;
		vf->width = vvc1_amstream_dec_info.width;
		vf->height = vvc1_amstream_dec_info.height;
		vf->bufWidth = 1920;
		vf->flag = 0;

		vf->pts = next_pts;
		vf->pts_us64 = next_pts_us64;
		if ((repeat_count > 1) && avi_flag) {
			vf->duration =
				vvc1_amstream_dec_info.rate *
				repeat_count >> 1;
			if (next_pts != 0) {
				next_pts +=
					((vf->duration) -
					 ((vf->duration) >> 4));
			}
			if (next_pts_us64 != 0) {
				next_pts_us64 += div_u64((u64)((vf->duration) -
				((vf->duration) >> 4)) * 100, 9);
			}
		} else {
			vf->duration =
				vvc1_amstream_dec_info.rate >> 1;
			next_pts = 0;
			next_pts_us64 = 0;
			if (picture_type != I_PICTURE &&
				unstable_pts) {
				vf->pts = 0;
				vf->pts_us64 = 0;
			}
		}

		vf->duration_pulldown = 0;
		vf->type = (reg & BOTTOM_FIELD_FIRST_FLAG) ?
		VIDTYPE_INTERLACE_TOP : VIDTYPE_INTERLACE_BOTTOM;
#ifdef NV21
		vf->type |= VIDTYPE_VIU_NV21;
#endif
		vf->canvas0Addr = vf->canvas1Addr =
				index2canvas(buffer_index);
		vf->orientation = 0;
		vf->type_original = vf->type;
		set_aspect_ratio(vf, READ_VREG(VC1_PIC_RATIO));

		hw->vfbuf_use[buffer_index]++;
		hw->vf_ref[buffer_index]++;
		vf->v4l_mem_handle = hw->pics[buffer_index].v4l_ref_buf_addr;
		aml_buf = (struct aml_buf *)vf->v4l_mem_handle;
		vf->pts_us64 = pts_us64;
		vf->timestamp = pts_us64;

		vf->canvas0Addr = vf->canvas1Addr = -1;
		vf->canvas0_config[0] = vc1_canvas_config[buffer_index][0];
		vf->canvas0_config[1] = vc1_canvas_config[buffer_index][1];
#ifdef NV21
		vf->plane_num = 2;
#else
		vf->canvas0_config[2] = vc1_canvas_config[buffer_index][2];
		vf->plane_num = 3;
#endif

		if (get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_T5D && vdec->use_vfm_path &&
			vdec_stream_based(vdec)) {
			vf->type |= VIDTYPE_FORCE_SIGN_IP_JOINT;
		}

		kfifo_put(&display_q, (const struct vframe_s *)vf);
		ATRACE_COUNTER(MODULE_NAME, vf->pts);
		if (ctx->is_stream_off) {
			vvc1_vf_put(vvc1_vf_get(vdec), vdec);
		} else {
			if (aml_buf->sub_buf[0])
				aml_buf = aml_buf->sub_buf[0];
			if (ctx->enable_di_post)
				ctx->fbc_transcode_and_set_vf(ctx, aml_buf, vf);
			aml_buf_set_vframe(aml_buf, vf);
			aml_buf_done(&ctx->bm, aml_buf, BUF_USER_DEC);
		}
	} else {	/* progressive */
		hw->throw_pb_flag = 0;
		if (kfifo_get(&newframe_q, &vf) == 0) {
			pr_info
			("fatal error, no available buffer slot.");
			return IRQ_HANDLED;
		}
		vf->signal_type = 0;
		vf->index = buffer_index;
		vf->width = vvc1_amstream_dec_info.width;
		vf->height = vvc1_amstream_dec_info.height;
		vf->bufWidth = 1920;
		vf->flag = 0;

		if (pts_valid) {
			vf->pts = pts;
			vf->pts_us64 = pts_us64;
			if ((repeat_count > 1) && avi_flag) {
				vf->duration =
					vvc1_amstream_dec_info.rate *
					repeat_count;
				next_pts =
					pts +
					(vvc1_amstream_dec_info.rate *
					 repeat_count) * 15 / 16;
				next_pts_us64 = pts_us64 +
					((vvc1_amstream_dec_info.rate *
					repeat_count) * 15 / 16) *
					100 / 9;
			} else {
				vf->duration =
					vvc1_amstream_dec_info.rate;
				next_pts = 0;
				next_pts_us64 = 0;
				if (picture_type != I_PICTURE &&
					unstable_pts) {
					vf->pts = 0;
					vf->pts_us64 = 0;
				}
			}
		} else {
			vf->pts = next_pts;
			vf->pts_us64 = next_pts_us64;
			if ((repeat_count > 1) && avi_flag) {
				vf->duration =
					vvc1_amstream_dec_info.rate *
					repeat_count;
				if (next_pts != 0) {
					next_pts += ((vf->duration) -
						((vf->duration) >> 4));
				}
				if (next_pts_us64 != 0) {
					next_pts_us64 +=
					div_u64((u64)((vf->duration) -
					((vf->duration) >> 4)) *
					100, 9);
				}
			} else {
				vf->duration =
					vvc1_amstream_dec_info.rate;
				next_pts = 0;
				next_pts_us64 = 0;
				if (picture_type != I_PICTURE &&
					unstable_pts) {
					vf->pts = 0;
					vf->pts_us64 = 0;
				}
			}
		}

		vf->duration_pulldown = 0;
#ifdef NV21
		vf->type =
			VIDTYPE_PROGRESSIVE | VIDTYPE_VIU_FIELD |
			VIDTYPE_VIU_NV21;
#else
		vf->type = VIDTYPE_PROGRESSIVE | VIDTYPE_VIU_FIELD;
#endif
		vf->canvas0Addr = vf->canvas1Addr =
					index2canvas(buffer_index);
		vf->orientation = 0;
		vf->type_original = vf->type;
		set_aspect_ratio(vf, READ_VREG(VC1_PIC_RATIO));

		hw->vfbuf_use[buffer_index]++;
		hw->vf_ref[buffer_index]++;
		vc1_print(0, VC1_DEBUG_DETAIL, "%s:  progressive vfbuf_use[%d] %d\n",
			__func__, buffer_index, hw->vfbuf_use[buffer_index]);

		vf->v4l_mem_handle = hw->pics[buffer_index].v4l_ref_buf_addr;
		aml_buf = (struct aml_buf *)vf->v4l_mem_handle;
		vf->pts_us64 = pts_us64;
		vf->timestamp = pts_us64;

		vc1_print(0, VC1_DEBUG_DETAIL,
			"[%d] %s(), v4l mem handle: 0x%lx\n",
			((struct aml_vcodec_ctx *)(hw->v4l2_ctx))->id,
			__func__, vf->v4l_mem_handle);

		vf->canvas0Addr = vf->canvas1Addr = -1;
		vf->canvas0_config[0] = vc1_canvas_config[buffer_index][0];
		vf->canvas0_config[1] = vc1_canvas_config[buffer_index][1];
#ifdef NV21
		vf->plane_num = 2;
#else
		vf->canvas0_config[2] = vc1_canvas_config[buffer_index][2];
		vf->plane_num = 3;
#endif

		if (get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_T5D && vdec->use_vfm_path &&
			vdec_stream_based(vdec)) {
			vf->type |= VIDTYPE_FORCE_SIGN_IP_JOINT;
		}
		vc1_print(0, VC1_DEBUG_DETAIL, "%s: display_q index %d, pts 0x%x/0x%x\n", __func__, vf->index, vf->pts, vf->pts_us64);
		if (ctx->enable_di_post)
			ctx->fbc_transcode_and_set_vf(ctx, aml_buf, vf);
		kfifo_put(&display_q, (const struct vframe_s *)vf);
		ATRACE_COUNTER(MODULE_NAME, vf->pts);

		if (ctx->is_stream_off) {
			vvc1_vf_put(vvc1_vf_get(vdec), vdec);
		} else {
			aml_buf_done(&ctx->bm, aml_buf, BUF_USER_DEC);
		}
	}

	hw->is_decoder_working = false;

	return 0;
}

void vc1_buf_ref_process_for_exception(struct vdec_vc1_hw_s *hw)
{
	struct aml_vcodec_ctx *ctx = (struct aml_vcodec_ctx *)(hw->v4l2_ctx);
	struct aml_buf *aml_buf;
	s32 index = hw->decoding_index;

	if (index < 0) {
		vc1_print(0, 0,
			"[ERR]cur_idx is invalid!\n");
		return;
	}

	aml_buf = (struct aml_buf *)hw->pics[index].v4l_ref_buf_addr;
	if (aml_buf == NULL) {
		vc1_print(0, 0,
			"[ERR]fb is NULL!\n");
		return;
	}

	vc1_print(0, 0,
		"process_for_exception: dma addr(0x%lx) buf_ref %d vfbuf_use %d ref_use %d buf_use %d\n",
		hw->pics[index].cma_alloc_addr,
		atomic_read(&aml_buf->entry.ref),
		hw->vfbuf_use[index],
		hw->ref_use[index],
		hw->buf_use[index]);

	aml_buf_put_ref(&ctx->bm, aml_buf);
	aml_buf_put_ref(&ctx->bm, aml_buf);
	if ((ctx->vpp_is_need || ctx->enable_di_post) && hw->interlace_flag) {
		aml_buf_put_ref(&ctx->bm, aml_buf);
	}

	hw->vfbuf_use[index] = 0;
	hw->ref_use[index] = 0;
	hw->pics[index].v4l_ref_buf_addr = 0;
	hw->pics[index].cma_alloc_addr = 0;
}

static irqreturn_t vvc1_isr_thread_handler(int irq, void *dev_id)
{
	struct vdec_vc1_hw_s *hw = &vc1_hw;
	struct aml_vcodec_ctx *ctx =
			(struct aml_vcodec_ctx *)(hw->v4l2_ctx);
	u32 reg;
	u32 repeat_count;
	u32 picture_type;
	u32 buffer_index;
	unsigned int pts, pts_valid = 0, offset = 0;
	u32 v_width, v_height;
	u64 pts_us64 = 0;
	u32 frame_size;
	u32 debug_tag;
	u32 status_reg;
	u32 ret = -1;
	ulong timeout;

	if (hw->eos) {
		WRITE_VREG(DECODE_STATUS, 0);
		return IRQ_HANDLED;
	}

	debug_tag = READ_VREG(DEBUG_REG1);
	if (debug_tag != 0) {
		vc1_print(0, 0, "%s: dbg%x: %x\n", __func__, debug_tag, READ_VREG(DEBUG_REG2));
		WRITE_VREG(DEBUG_REG1, 0);
		return IRQ_HANDLED;
	}

	status_reg = READ_VREG(DECODE_STATUS);

	if (status_reg == DECODE_STATUS_SEQ_HEADER_DONE) {//seq heard done
		reg = READ_VREG(VC1_PIC_INFO);
		hw->frame_width = READ_VREG(VC1_PIC_INFO) & 0x3fff;
		hw->frame_height = (READ_VREG(VC1_PIC_INFO) >> 14) & 0x3fff;
		hw->interlace_flag = (READ_VREG(VC1_PIC_INFO) >> 28) & 0x1;
		vc1_print(0, VC1_DEBUG_DETAIL, "%s: SEQ_HEADER_DONE frame_width %d/%d, interlace_flag %d\n", __func__,
			hw->frame_width, hw->frame_height, hw->interlace_flag);
		if (!v4l_res_change(hw)) {
			if (ctx->param_sets_from_ucode && !hw->v4l_params_parsed) {
				struct aml_vdec_ps_infos ps;
				pr_info("set ucode parse\n");
				vvc1_get_ps_info(hw, &ps);
				vdec_v4l_set_ps_infos(ctx, &ps);
				hw->last_width = hw->frame_width;
				hw->last_height = hw->frame_height;
				hw->v4l_params_parsed = true;
				ctx->decoder_status_info.frame_height = ps.visible_height;
				ctx->decoder_status_info.frame_width = ps.visible_width;
			}
		}

		WRITE_VREG(DECODE_STATUS, 0);

		return IRQ_HANDLED;
	} else if (status_reg == DECODE_STATUS_PIC_SKIPPED) {//PIC Skipped
		vc1_print(0, VC1_DEBUG_DETAIL, "%s: PIC_SKIPPED picture_type invalid\n", __func__);
		vdec_v4l_get_pts_info(ctx, &ctx->current_timestamp);
		vdec_v4l_post_error_frame_event(ctx);
		vc1_print(0, VC1_DEBUG_DETAIL,"%s: DECODE_STATUS_PIC_SKIPPED timestamp %lld\n",
			__func__, ctx->current_timestamp);
		WRITE_VREG(DECODE_STATUS, 0);
		return IRQ_HANDLED;
	} else if (status_reg == DECODE_STATUS_BUF_INVALID) {
		vc1_print(0, VC1_DEBUG_DETAIL, "%s: BUF_INVALID \n", __func__);
		WRITE_VREG(DECODE_STATUS, 0);
		return IRQ_HANDLED;
	} else if (status_reg == DECODE_STATUS_PIC_HEADER_DONE) {//PIC header done
		hw->new_type = READ_VREG(AV_SCRATCH_K);
		vc1_print(0, VC1_DEBUG_DETAIL, "%s: PIC_HEADER_DONE picture_type %d(%s)\n", __func__, hw->new_type,
			((hw->new_type == I_PICTURE) ? "I" : ((hw->new_type == P_PICTURE) ? "P" : "B")));
	}

	reg = READ_VREG(VC1_BUFFEROUT);
	if (reg) {
		v_width = hw->frame_width;//READ_VREG(AV_SCRATCH_J);
		v_height = hw->frame_height;//READ_VREG(AV_SCRATCH_K);
		hw->is_decoder_working = true;
		vc1_print(0, VC1_DEBUG_DETAIL, "%s: v_width %d, v_height %d\n",
					__func__, v_width, v_height);

		vc1_set_rp();

		if (v_width && v_width <= 4096
			&& (v_width != vvc1_amstream_dec_info.width)) {
			pr_info("frame width changed %d to %d\n",
				   vvc1_amstream_dec_info.width, v_width);
			vvc1_amstream_dec_info.width = v_width;
			frame_width = v_width;
		}
		if (v_height && v_height <= 4096
			&& (v_height != vvc1_amstream_dec_info.height)) {
			pr_info("frame height changed %d to %d\n",
				   vvc1_amstream_dec_info.height, v_height);
			vvc1_amstream_dec_info.height = v_height;
			frame_height = v_height;
		}

		if (pts_by_offset) {
			offset = READ_VREG(VC1_OFFSET_REG);
			if (pts_lookup_offset_us64(
					PTS_TYPE_VIDEO,
					offset, &pts, &frame_size,
					0, &pts_us64) == 0) {
				hw->pics[hw->decoding_index].pts_valid = 1;
				hw->pics[hw->decoding_index].pts = pts;
				hw->pics[hw->decoding_index].pts64 = pts_us64;

#ifdef DEBUG_PTS
				pts_hit++;
#endif
			} else {
				hw->pics[hw->decoding_index].pts_valid = 0;
				hw->pics[hw->decoding_index].pts = 0;
				hw->pics[hw->decoding_index].pts64 = 0;

#ifdef DEBUG_PTS
				pts_missed++;
#endif
			}
		}

		repeat_count = READ_VREG(VC1_REPEAT_COUNT);
		buffer_index = ((reg & 0x7) - 1) & 3;
		picture_type = (reg >> 3) & 7;//I:0,P:1,B:2
		vc1_print(0, VC1_DEBUG_DETAIL, "%s: get buffer_index %d, decoding_index %d\n",
					__func__, buffer_index, hw->decoding_index);

		vdec_v4l_get_pts_info(ctx, &hw->pics[hw->decoding_index].pts64);
		hw->interlace_flag = (reg & INTERLACE_FLAG) ? 1 : 0;
		hw->pics[hw->decoding_index].offset = READ_VREG(VC1_OFFSET_REG);
		hw->pics[hw->decoding_index].repeat_cnt = READ_VREG(VC1_REPEAT_COUNT);
		hw->pics[hw->decoding_index].buffer_info = reg;
		hw->pics[hw->decoding_index].index = hw->decoding_index;
		//hw->pics[hw->decoding_index].decode_pic_count = decode_pic_count;
		hw->pics[hw->decoding_index].picture_type = (reg >> 3) & 7;
		hw->buf_use[hw->decoding_index]++;
		vc1_print(0, VC1_DEBUG_DETAIL, "%s: get buffer_index %d, decoding_index %d, buffer_info 0x%x, index %d, picture_type %d offset %d\n",
					__func__, buffer_index, hw->decoding_index,
					hw->pics[hw->decoding_index].buffer_info,
					hw->pics[hw->decoding_index].index,
					hw->pics[hw->decoding_index].picture_type,
					hw->pics[hw->decoding_index].offset);

#ifdef DEBUG_PTS
		if (picture_type == I_PICTURE) {
			/* pr_info("I offset 0x%x,
			 *pts_valid %d\n", offset, pts_valid);
			 */
			if (!pts_valid)
				pts_i_missed++;
			else
				pts_i_hit++;
		}
#endif

		if ((pts_valid) && (frm.state != RATE_MEASURE_DONE)) {
			if (frm.state == RATE_MEASURE_START_PTS) {
				frm.start_pts = pts;
				frm.state = RATE_MEASURE_END_PTS;
				frm.trymax = RATE_MEASURE_NUM;
			} else if (frm.state == RATE_MEASURE_END_PTS) {
				if (frm.num >= frm.trymax) {
					frm.end_pts = pts;
					frm.rate = (frm.end_pts -
						frm.start_pts) / frm.num;
					pr_info("frate before=%d,%d,num=%d\n",
					frm.rate,
					DUR2PTS(vvc1_amstream_dec_info.rate),
					frm.num);
					/* check if measured rate is same as
					 * settings from upper layer
					 * and correct it if necessary
					 */
					if ((close_to(frm.rate, RATE_30_FPS,
						RATE_CORRECTION_THRESHOLD) &&
						close_to(
						DUR2PTS(
						vvc1_amstream_dec_info.rate),
						RATE_24_FPS,
						RATE_CORRECTION_THRESHOLD))
						||
						(close_to(
						frm.rate, RATE_24_FPS,
						RATE_CORRECTION_THRESHOLD)
						&&
						close_to(DUR2PTS(
						vvc1_amstream_dec_info.rate),
						RATE_30_FPS,
						RATE_CORRECTION_THRESHOLD))) {
						pr_info(
						"vvc1: frate from %d to %d\n",
						vvc1_amstream_dec_info.rate,
						PTS2DUR(frm.rate));

						vvc1_amstream_dec_info.rate =
							PTS2DUR(frm.rate);
						frm.state = RATE_MEASURE_DONE;
					} else if (close_to(frm.rate,
						DUR2PTS(
						vvc1_amstream_dec_info.rate),
						RATE_CORRECTION_THRESHOLD))
						frm.state = RATE_MEASURE_DONE;
					else {

/* maybe still have problem,
 *						try next double frames....
 */
						frm.state = RATE_MEASURE_DONE;
						frm.start_pts = pts;
						frm.state =
						RATE_MEASURE_END_PTS;
						/*60 fps*60 S */
						frm.num = 0;
					}
				}
			}
		}

		if (frm.state != RATE_MEASURE_DONE)
			frm.num += (repeat_count > 1) ? repeat_count : 1;
		if (vvc1_amstream_dec_info.rate == 0)
			vvc1_amstream_dec_info.rate = PTS2DUR(frm.rate);

		if (hw->throw_pb_flag && picture_type != I_PICTURE) {
			vc1_print(0, VC1_DEBUG_DETAIL, "%s: WRITE_VREG(VC1_BUFFERIN, 0x%x) decoding_index %d, for throwing picture with type of %d\n",
				__func__, ~(1 << hw->decoding_index), hw->decoding_index, picture_type);

			WRITE_VREG(VC1_BUFFERIN, ~(1 << hw->decoding_index));
			hw->buf_use[hw->decoding_index]--;
			ctx->current_timestamp = hw->pics[hw->decoding_index].pts64;
			vc1_buf_ref_process_for_exception(hw);
			vdec_v4l_post_error_frame_event(ctx);
			vc1_print(0, VC1_DEBUG_DETAIL,"%s: index %d, buf_use %d, buffer_index %d\n",
				__func__, hw->decoding_index, hw->buf_use[hw->decoding_index], buffer_index);
		} else {
			if ((picture_type == I_PICTURE) ||
				(picture_type == P_PICTURE)) {
				buffer_index = update_reference(hw, hw->decoding_index);
			} else {
				/* drop b frame before reference pic ready */
				if (hw->refs[0] == -1) {
					buffer_index = hw->vf_buf_num_used;
					WRITE_VREG(VC1_BUFFERIN, ~(1 << hw->decoding_index));
					hw->buf_use[hw->decoding_index]--;
					ctx->current_timestamp = hw->pics[hw->decoding_index].pts64;
					vc1_buf_ref_process_for_exception(hw);
					vdec_v4l_post_error_frame_event(ctx);
					vc1_print(0, VC1_DEBUG_DETAIL,"%s: index %d, buf_use %d\n",
						__func__, hw->decoding_index, hw->buf_use[hw->decoding_index]);
				}
			}

			if (buffer_index < hw->vf_buf_num_used) {
				vc1_print(0, VC1_DEBUG_DETAIL, "%s: show index %d\n",	__func__, buffer_index);
				prepare_display_buf(hw, &hw->pics[buffer_index]);
			} else {
				vc1_print(0, VC1_DEBUG_DETAIL, "%s: drop pic index %d\n",	__func__, buffer_index);
			}
		}

		frame_dur = vvc1_amstream_dec_info.rate;
		total_frame++;

		/*count info*/
		gvs->frame_dur = frame_dur;
		vdec_count_info(gvs, 0, offset);
	}

	recycle_frames(hw);
	if (is_available_buffer(hw))
		ret = vvc1_config_buf(hw);

	timeout = jiffies + HZ;
	while ((ret == -1) && (stat & STAT_VDEC_RUN)) {
		if (is_available_buffer(hw))
			ret = vvc1_config_buf(hw);

		if (ret == 0)
			break;

		if (time_after(jiffies, timeout)) {
			vc1_print(0, 0,
				"get capture buffer...wait timeout!\n");
			break;
		}

		msleep(wait_time);
	}

	WRITE_VREG(DECODE_STATUS, 0);

	return IRQ_HANDLED;
}

static irqreturn_t vvc1_isr_thread_fn(int irq, void *dev_id)
{
	irqreturn_t ret;

	ret = vvc1_isr_thread_handler(irq, dev_id);

	process_busy = false;

	return ret;
}

static irqreturn_t vvc1_isr(int irq, void *dev_id)
{

	if (process_busy)
		return IRQ_HANDLED;

	process_busy = true;

	WRITE_VREG(ASSIST_MBOX1_CLR_REG, 1);

	return IRQ_WAKE_THREAD;
}

static struct vframe_s *vvc1_vf_peek(void *op_arg)
{
	struct vframe_s *vf;

	if (kfifo_peek(&display_q, &vf))
		return vf;

	return NULL;
}

static struct vframe_s *vvc1_vf_get(void *op_arg)
{
	struct vframe_s *vf;
	struct vdec_vc1_hw_s *hw = &vc1_hw;

	if (kfifo_get(&display_q, &vf)) {
		if (vf) {
			vf->index_disp = atomic_read(&hw->get_num);
			atomic_add(1, &hw->get_num);

			vc1_print(0, VC1_DEBUG_DETAIL,
				"%s, index = %d, w %d h %d, type 0x%x \n",
				__func__,
				vf->index,
				vf->width,
				vf->height,
				vf->type);
		}
		kfifo_put(&recycle_q, (const struct vframe_s *)vf);

		return vf;
	}

	return NULL;
}

static void vvc1_vf_put(struct vframe_s *vf, void *op_arg)
{
	struct vdec_vc1_hw_s *hw = &vc1_hw;
	struct aml_vcodec_ctx *ctx =
		(struct aml_vcodec_ctx *)(hw->v4l2_ctx);
	struct aml_buf *aml_buf = (struct aml_buf *)vf->v4l_mem_handle;

	if (!aml_buf) {
		vc1_print(0, 0,
			"[ERR]invalid fb, vf: %lx\n", (ulong)vf);
		return;
	}

	aml_buf_put_ref(&ctx->bm, aml_buf);
}

static int vvc1_vf_states(struct vframe_states *states, void *op_arg)
{
	unsigned long flags;

	spin_lock_irqsave(&lock, flags);

	states->vf_pool_size = VF_POOL_SIZE;
	states->buf_free_num = kfifo_len(&newframe_q);
	states->buf_avail_num = kfifo_len(&display_q);
	states->buf_recycle_num = kfifo_len(&recycle_q);

	spin_unlock_irqrestore(&lock, flags);

	return 0;
}

static int vvc1_event_cb(int type, void *data, void *private_data)
{
	if (type & VFRAME_EVENT_RECEIVER_RESET) {
		unsigned long flags;

		amvdec_stop();
#ifndef CONFIG_AMLOGIC_POST_PROCESS_MANAGER
		vf_light_unreg_provider(&vvc1_vf_prov);
#endif
		spin_lock_irqsave(&lock, flags);
		vvc1_local_init(true);
		vvc1_prot_init();
		spin_unlock_irqrestore(&lock, flags);
#ifndef CONFIG_AMLOGIC_POST_PROCESS_MANAGER
		vf_reg_provider(&vvc1_vf_prov);
#endif
		amvdec_start();
	}

	if (type & VFRAME_EVENT_RECEIVER_REQ_STATE) {
		struct provider_state_req_s *req =
			(struct provider_state_req_s *)data;
		if (req->req_type == REQ_STATE_SECURE && vdec)
			req->req_result[0] = vdec_secure(vdec);
		else
			req->req_result[0] = 0xffffffff;
	}
	return 0;
}

int vvc1_dec_status(struct vdec_s *vdec, struct vdec_info *vstatus)
{
	if (!(stat & STAT_VDEC_RUN))
		return -1;

	vstatus->frame_width = vvc1_amstream_dec_info.width;
	vstatus->frame_height = vvc1_amstream_dec_info.height;
	if (vvc1_amstream_dec_info.rate != 0)
		vstatus->frame_rate = 96000 / vvc1_amstream_dec_info.rate;
	else
		vstatus->frame_rate = -1;
	vstatus->error_count = READ_VREG(AV_SCRATCH_C);
	vstatus->status = stat;
	vstatus->bit_rate = gvs->bit_rate;
	vstatus->frame_dur = vvc1_amstream_dec_info.rate;
	vstatus->frame_data = gvs->frame_data;
	vstatus->total_data = gvs->total_data;
	vstatus->frame_count = gvs->frame_count;
	vstatus->error_frame_count = gvs->error_frame_count;
	vstatus->drop_frame_count = gvs->drop_frame_count;
	vstatus->total_data = gvs->total_data;
	vstatus->samp_cnt = gvs->samp_cnt;
	vstatus->offset = gvs->offset;
	snprintf(vstatus->vdec_name, sizeof(vstatus->vdec_name),
		"%s", DRIVER_NAME);

	return 0;
}

int vvc1_set_isreset(struct vdec_s *vdec, int isreset)
{
	vc1_print(0, VC1_DEBUG_DETAIL,"%s: isreset 0x%x\n", __func__, isreset);

	is_reset = isreset;
	return 0;
}

static int vvc1_vdec_info_init(void)
{
	gvs = kzalloc(sizeof(struct vdec_info), GFP_KERNEL);
	if (NULL == gvs) {
		pr_info("the struct of vdec status malloc failed.\n");
		return -ENOMEM;
	}
	return 0;
}

/****************************************/
static int vvc1_workspace_init(void)
{
	int ret;
	u32 alloc_size;
	unsigned long buf_start;

	/* workspace mem */
	alloc_size = WORKSPACE_SIZE;
	vc1_print(0, VC1_DEBUG_DETAIL, "%s: alloc_size %d\n", __func__, alloc_size);

	ret = decoder_bmmu_box_alloc_buf_phy(mm_blk_handle, 0,
			alloc_size, DRIVER_NAME, &buf_start);
	if (ret < 0)
		return ret;

	/* calculate workspace offset */
	buf_offset = buf_start - DCAC_BUFF_START_ADDR;

	return 0;
}

static int vvc1_prot_init(void)
{
	int r;
#if 1	/* /MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6 */
	WRITE_VREG(DOS_SW_RESET0, (1 << 7) | (1 << 6) | (1 << 4));
	WRITE_VREG(DOS_SW_RESET0, 0);

	READ_VREG(DOS_SW_RESET0);

	WRITE_VREG(DOS_SW_RESET0, (1 << 7) | (1 << 6) | (1 << 4));
	WRITE_VREG(DOS_SW_RESET0, 0);

	WRITE_VREG(DOS_SW_RESET0, (1 << 9) | (1 << 8));
	WRITE_VREG(DOS_SW_RESET0, 0);

#else
	WRITE_RESET_REG(RESET0_REGISTER,
				   RESET_IQIDCT | RESET_MC | RESET_VLD_PART);
	READ_RESET_REG(RESET0_REGISTER);
	WRITE_RESET_REG(RESET0_REGISTER,
				   RESET_IQIDCT | RESET_MC | RESET_VLD_PART);

	WRITE_RESET_REG(RESET2_REGISTER, RESET_PIC_DC | RESET_DBLK);
#endif

	WRITE_VREG(POWER_CTL_VLD, 0x10);
	WRITE_VREG_BITS(VLD_MEM_VIFIFO_CONTROL, 2, MEM_FIFO_CNT_BIT, 2);
	WRITE_VREG_BITS(VLD_MEM_VIFIFO_CONTROL, 8, MEM_LEVEL_CNT_BIT, 6);

	r = vvc1_workspace_init();
	/* index v << 16 | u << 8 | y */
#ifdef NV21
	WRITE_VREG(AV_SCRATCH_0, 0x010100);
	WRITE_VREG(AV_SCRATCH_1, 0x030302);
	WRITE_VREG(AV_SCRATCH_2, 0x050504);
	WRITE_VREG(AV_SCRATCH_3, 0x070706);
/*	WRITE_VREG(AV_SCRATCH_G, 0x090908);
	WRITE_VREG(AV_SCRATCH_H, 0x0b0b0a);
	WRITE_VREG(AV_SCRATCH_I, 0x0d0d0c);
	WRITE_VREG(AV_SCRATCH_J, 0x0f0f0e);*/
#else
	WRITE_VREG(AV_SCRATCH_0, 0x020100);
	WRITE_VREG(AV_SCRATCH_1, 0x050403);
	WRITE_VREG(AV_SCRATCH_2, 0x080706);
	WRITE_VREG(AV_SCRATCH_3, 0x0b0a09);
	WRITE_VREG(AV_SCRATCH_G, 0x090908);
	WRITE_VREG(AV_SCRATCH_H, 0x0b0b0a);
	WRITE_VREG(AV_SCRATCH_I, 0x0d0d0c);
	WRITE_VREG(AV_SCRATCH_J, 0x0f0f0e);
#endif

	/* notify ucode the buffer offset */
	WRITE_VREG(AV_SCRATCH_F, buf_offset);

	/* disable PSCALE for hardware sharing */
	WRITE_VREG(PSCALE_CTRL, 0);

	WRITE_VREG(VC1_SOS_COUNT, 0);
	WRITE_VREG(VC1_BUFFERIN, 0);
	WRITE_VREG(VC1_BUFFEROUT, NEW_DRV_VER); //reuse the register VC1_BUFFEROUT to support new ucode version
	vc1_print(0, VC1_DEBUG_DETAIL,"%s VC1_BUFFEROUT %d\n", __func__, NEW_DRV_VER);

	/* clear mailbox interrupt */
	WRITE_VREG(ASSIST_MBOX1_CLR_REG, 1);

	/* enable mailbox interrupt */
	WRITE_VREG(ASSIST_MBOX1_MASK, 1);

#ifdef NV21
	SET_VREG_MASK(MDEC_PIC_DC_CTRL, 1 << 17);
#endif
	CLEAR_VREG_MASK(MDEC_PIC_DC_CTRL, 1 << 16);

	WRITE_VREG(CANVAS_BUF_REG, 0);
	WRITE_VREG(ANC0_CANVAS_REG, 0);
	WRITE_VREG(ANC1_CANVAS_REG, 0);
	WRITE_VREG(DECODE_STATUS, 0);
	WRITE_VREG(VC1_PIC_INFO, 0);
	WRITE_VREG(DEBUG_REG1, 0);
	WRITE_VREG(DEBUG_REG2, 0);

	WRITE_VREG(AV_SCRATCH_L, udebug_flag);
	return r;
}

static void vvc1_local_init(bool is_reset)
{
	struct vdec_vc1_hw_s *hw = &vc1_hw;
	int i;
	vc1_print(0, VC1_DEBUG_DETAIL,"%s \n", __func__);

	/* vvc1_ratio = 0x100; */
	vvc1_ratio = vvc1_amstream_dec_info.ratio;

	avi_flag = (unsigned long) vvc1_amstream_dec_info.param & 0x01;

	unstable_pts = (((unsigned long) vvc1_amstream_dec_info.param & 0x40) >> 6);
	if (unstable_pts_debug == 1) {
		unstable_pts = 1;
		pr_info("vc1 init , unstable_pts_debug = %u\n",unstable_pts_debug);
	}
	total_frame = 0;

	next_pts = 0;

	next_pts_us64 = 0;
	saved_resolution = 0;
	frame_width = frame_height = frame_dur = 0;
	process_busy = false;
#ifdef DEBUG_PTS
	pts_hit = pts_missed = pts_i_hit = pts_i_missed = 0;
#endif

	memset(&frm, 0, sizeof(frm));

	if (!is_reset) {
		hw->refs[0] = -1;
		hw->refs[1] = -1;
		hw->throw_pb_flag = 1;
		hw->vf_buf_num_used = DECODE_BUFFER_NUM_MAX;
		if (hw->vf_buf_num_used > DECODE_BUFFER_NUM_MAX)
			hw->vf_buf_num_used = DECODE_BUFFER_NUM_MAX;

		for (i = 0; i < hw->vf_buf_num_used; i++) {
			hw->vfbuf_use[i] = 0;
			hw->buf_use[i] = 0;
			hw->ref_use[i] = 0;
			hw->vf_ref[i] = 0;
		}

		INIT_KFIFO(display_q);
		INIT_KFIFO(recycle_q);
		INIT_KFIFO(newframe_q);
		cur_pool_idx ^= 1;
		for (i = 0; i < VF_POOL_SIZE; i++) {
			const struct vframe_s *vf;

			if (cur_pool_idx == 0) {
				vf = &vfpool[i];
				vfpool[i].index = DECODE_BUFFER_NUM_MAX;
			} else {
				vf = &vfpool2[i];
				vfpool2[i].index = DECODE_BUFFER_NUM_MAX;
			}
			kfifo_put(&newframe_q, (const struct vframe_s *)vf);
		}
	}

	if (mm_blk_handle) {
		decoder_bmmu_box_free(mm_blk_handle);
		mm_blk_handle = NULL;
	}

		mm_blk_handle = decoder_bmmu_box_alloc_box(
			DRIVER_NAME,
			0,
			MAX_BMMU_BUFFER_NUM,
			4 + PAGE_SHIFT,
			CODEC_MM_FLAGS_CMA_CLEAR |
			CODEC_MM_FLAGS_FOR_VDECODER,
			BMMU_ALLOC_FLAGS_WAITCLEAR);
}

#ifdef CONFIG_AMLOGIC_POST_PROCESS_MANAGER
static void vvc1_ppmgr_reset(void)
{
	vc1_print(0, VC1_DEBUG_DETAIL,"%s: ", __func__);

	vf_notify_receiver(PROVIDER_NAME, VFRAME_EVENT_PROVIDER_RESET, NULL);

	vvc1_local_init(true);

	/* vf_notify_receiver(PROVIDER_NAME,
	 * VFRAME_EVENT_PROVIDER_START,NULL);
	 */

	pr_info("vvc1dec: vf_ppmgr_reset\n");
}
#endif

static void vvc1_set_clk(struct work_struct *work)
{
		int fps = 96000 / frame_dur;

		saved_resolution = frame_width * frame_height * fps;
		vdec_source_changed(VFORMAT_VC1,
			frame_width, frame_height, fps);

}

static void error_do_work(struct work_struct *work)
{
		vc1_print(0, VC1_DEBUG_DETAIL,"%s \n", __func__);

		amvdec_stop();
		msleep(20);
#ifdef CONFIG_AMLOGIC_POST_PROCESS_MANAGER
		vvc1_ppmgr_reset();
#else
		vf_light_unreg_provider(&vvc1_vf_prov);
		vvc1_local_init(true);
		vf_reg_provider(&vvc1_vf_prov);
#endif
		vvc1_prot_init();
		amvdec_start();
}

static void vvc1_put_timer_func(struct timer_list *timer)
{
	struct vdec_vc1_hw_s *hw = &vc1_hw;
	u32 wp, rp;

	if (READ_VREG(VC1_SOS_COUNT) > 10)
		schedule_work(&error_wd_work);

	vc1_set_rp();
	wp = READ_VREG(VLD_MEM_VIFIFO_WP);
	rp = READ_VREG(VLD_MEM_VIFIFO_RP);

	/* notify decoder */
	if (!(stat & STAT_VDEC_RUN)) {
		if (wp - rp > start_decode_buf_level) {
			amvdec_start();
			stat |= STAT_VDEC_RUN;
		}
	}

	/* notify eos after setting EOS */
	if (vdec->input.eos) {
		if ((!hw->last_rp && !hw->last_wp) ||
			((hw->last_rp != rp) || (hw->last_wp != wp))){
			hw->last_wp = wp;
			hw->last_rp = rp;
			timeout_times = 0;
		} else {
			timeout_times += 1;
		}
	}

	if (wp >= rp) {
		if (((wp - rp < start_decode_buf_level) ||
			(timeout_times >= 20)) &&
			!vdec_has_more_input(vdec)) {
			vc1_print(0, VC1_DEBUG_DETAIL,
				"%s wp 0x%x rp 0x%x level %d eos %d\n",
				__func__, wp, rp,
				wp - rp, vdec->input.eos);
			flush_output(hw);
			notify_v4l_eos();
		}
	} else {
		if (((wp + vdec->vbuf.buf_size - rp < start_decode_buf_level) ||
			(timeout_times >= 20)) &&
			!vdec_has_more_input(vdec)) {
			vc1_print(0, VC1_DEBUG_DETAIL,
				"%s wp 0x%x rp 0x%x level %d eos %d\n",
				__func__, wp, rp,
				wp + vdec->vbuf.buf_size - rp, vdec->input.eos);
			flush_output(hw);
			notify_v4l_eos();
		}
	}

	vc1_print(0, VC1_DEBUG_DETAIL,
		"%s wp 0x%x rp 0x%x level %d \n",
		__func__, wp, rp,
		(wp >= rp) ? (wp - rp) : (wp + vdec->vbuf.buf_size - rp));

	if (frame_dur > 0 && saved_resolution !=
		frame_width * frame_height * (96000 / frame_dur))
		schedule_work(&set_clk_work);
	timer->expires = jiffies + PUT_INTERVAL;

	add_timer(timer);
}

static s32 vvc1_init(void)
{
	int ret = -1;
	char *buf = vmalloc(0x1000 * 16);
	int fw_type = VIDEO_DEC_VC1;

	if (IS_ERR_OR_NULL(buf))
		return -ENOMEM;

	pr_info("vvc1_init, format %d\n", vvc1_amstream_dec_info.format);
	timer_setup(&recycle_timer, vvc1_put_timer_func, 0);

	stat |= STAT_TIMER_INIT;

	intra_output = 0;
	amvdec_enable();

	vvc1_local_init(false);

	if (vvc1_amstream_dec_info.format == VIDEO_DEC_FORMAT_WMV3) {
		pr_info("WMV3 dec format\n");
		vvc1_format = VIDEO_DEC_FORMAT_WMV3;
		WRITE_VREG(AV_SCRATCH_4, 0);
	} else if (vvc1_amstream_dec_info.format == VIDEO_DEC_FORMAT_WVC1) {
		pr_info("WVC1 dec format\n");
		vvc1_format = VIDEO_DEC_FORMAT_WVC1;
		WRITE_VREG(AV_SCRATCH_4, 1);
	} else
		pr_info("not supported VC1 format\n");

	if (get_firmware_data(fw_type, buf) < 0) {
		amvdec_disable();
		pr_err("get firmware fail.");
		vfree(buf);
		return -1;
	}

	ret = amvdec_loadmc_ex(VFORMAT_VC1, NULL, buf);
	if (ret < 0) {
		amvdec_disable();
		vfree(buf);
		pr_err("VC1: the %s fw loading failed, err: %x\n",
			fw_tee_enabled() ? "TEE" : "local", ret);
		return -EBUSY;
	}

	vfree(buf);

	stat |= STAT_MC_LOAD;

	/* enable AMRISC side protocol */
	ret = vvc1_prot_init();
	if (ret < 0)
		return ret;

	if (vdec_request_threaded_irq(VDEC_IRQ_1, vvc1_isr,
			vvc1_isr_thread_fn, IRQF_SHARED,
			"vvc1-irq", (void *)vvc1_dec_id)) {
		amvdec_disable();

		pr_info("vvc1 irq register error.\n");
		return -ENOENT;
	}

	stat |= STAT_ISR_REG;
#ifdef CONFIG_AMLOGIC_POST_PROCESS_MANAGER
	vf_provider_init(&vvc1_vf_prov,
		PROVIDER_NAME, &vvc1_vf_provider, NULL);
	vf_reg_provider(&vvc1_vf_prov);
	vf_notify_receiver(PROVIDER_NAME,
		VFRAME_EVENT_PROVIDER_START, NULL);
#else
	vf_provider_init(&vvc1_vf_prov,
		PROVIDER_NAME, &vvc1_vf_provider, NULL);
	vf_reg_provider(&vvc1_vf_prov);
#endif

	if (!is_reset)
		vf_notify_receiver(PROVIDER_NAME,
				VFRAME_EVENT_PROVIDER_FR_HINT,
				(void *)
				((unsigned long)vvc1_amstream_dec_info.rate));

	stat |= STAT_VF_HOOK;

	recycle_timer.expires = jiffies + PUT_INTERVAL;
	add_timer(&recycle_timer);

	stat |= STAT_TIMER_ARM;

	amvdec_start();

	stat |= STAT_VDEC_RUN;

	return 0;
}

static int amvdec_vc1_probe(struct platform_device *pdev)
{
	struct vdec_s *pdata = *(struct vdec_s **)pdev->dev.platform_data;
	struct vdec_vc1_hw_s *hw = NULL;
	int config_val = 0;

	vc1_print(0, VC1_DEBUG_DETAIL,"%s \n", __func__);

	if (pdata == NULL) {
		pr_info("amvdec_vc1_v4l memory resource undefined.\n");
		return -EFAULT;
	}

	if (pdata->sys_info) {
		vvc1_amstream_dec_info = *pdata->sys_info;

		if ((vvc1_amstream_dec_info.height != 0) &&
			(vvc1_amstream_dec_info.width >
			(VC1_MAX_SUPPORT_SIZE/vvc1_amstream_dec_info.height))) {
			pr_info("amvdec_vc1_v4l: over size, unsupport: %d * %d\n",
				vvc1_amstream_dec_info.width,
				vvc1_amstream_dec_info.height);
			return -EFAULT;
		}
	}
	pdata->dec_status = vvc1_dec_status;
	pdata->set_isreset = vvc1_set_isreset;
	pdata->reset = reset;
	is_reset = 0;
	vdec = pdata;

	/* the ctx from v4l2 driver. */
	hw = (struct vdec_vc1_hw_s *)vzalloc(sizeof(struct vdec_vc1_hw_s));
	hw->v4l2_ctx = pdata->private;
	hw->canvas_mode = pdata->canvas_mode;
	hw->is_decoder_working = false;
	if (pdata->config_len) {
		if (get_config_int(pdata->config, "parm_v4l_buffer_margin",
			&config_val) == 0)
			hw->dynamic_buf_num_margin = config_val;
		else
			hw->dynamic_buf_num_margin = default_vc1_margin;

		if (get_config_int(pdata->config,
			"parm_v4l_canvas_mem_mode",
			&config_val) == 0)
			hw->canvas_mode = config_val;

		if (get_config_int(pdata->config, "parm_v4l_duration",
			&config_val) == 0)
			hw->cur_duration = config_val;
	} else
		hw->dynamic_buf_num_margin = default_vc1_margin;

	vc1_hw = *hw;

	vvc1_vdec_info_init();

	INIT_WORK(&error_wd_work, error_do_work);
	INIT_WORK(&set_clk_work, vvc1_set_clk);
	spin_lock_init(&vc1_rp_lock);
	if (vvc1_init() < 0) {
		pr_info("amvdec_vc1_v4l init failed.\n");
		kfree(gvs);
		gvs = NULL;
		pdata->dec_status = NULL;
		return -ENODEV;
	}

	return 0;
}

static int amvdec_vc1_remove(struct platform_device *pdev)
{
	cancel_work_sync(&error_wd_work);
	vc1_print(0, VC1_DEBUG_DETAIL,"%s  %d\n", __func__, stat);
	if (stat & STAT_VDEC_RUN) {
		amvdec_stop();
		stat &= ~STAT_VDEC_RUN;
	}

	if (stat & STAT_ISR_REG) {
		vdec_free_irq(VDEC_IRQ_1, (void *)vvc1_dec_id);
		stat &= ~STAT_ISR_REG;
	}

	if (stat & STAT_TIMER_ARM) {
		del_timer_sync(&recycle_timer);
		stat &= ~STAT_TIMER_ARM;
	}

	cancel_work_sync(&set_clk_work);
	if (stat & STAT_VF_HOOK) {
		if (!is_reset)
			vf_notify_receiver(PROVIDER_NAME,
					VFRAME_EVENT_PROVIDER_FR_END_HINT,
					NULL);

		vf_unreg_provider(&vvc1_vf_prov);
		stat &= ~STAT_VF_HOOK;
	}

	amvdec_disable();
	if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_TM2)
		vdec_reset_core(NULL);

	if (mm_blk_handle) {
		decoder_bmmu_box_free(mm_blk_handle);
		mm_blk_handle = NULL;
	}

#ifdef DEBUG_PTS
	pr_debug("pts hit %d, pts missed %d, i hit %d, missed %d\n", pts_hit,
		pts_missed, pts_i_hit, pts_i_missed);
	pr_debug("total frame %d, avi_flag %d, rate %d\n",
		total_frame, avi_flag,
		vvc1_amstream_dec_info.rate);
#endif
	kfree(gvs);
	gvs = NULL;
	vdec = NULL;

	return 0;
}

/****************************************/

static struct platform_driver amvdec_vc1_driver = {
	.probe = amvdec_vc1_probe,
	.remove = amvdec_vc1_remove,
#ifdef CONFIG_PM
	.suspend = amvdec_suspend,
	.resume = amvdec_resume,
#endif
	.driver = {
		.name = DRIVER_NAME,
	}
};

#if defined(CONFIG_ARCH_MESON)	/*meson1 only support progressive */
static struct codec_profile_t amvdec_vc1_profile = {
	.name = "VC1-V4L",
	.profile = "progressive, wmv3"
};
#else
static struct codec_profile_t amvdec_vc1_profile = {
	.name = "VC1-V4L",
	.profile = "progressive, interlace, wmv3"
};
#endif

static int __init amvdec_vc1_driver_init_module(void)
{
	vc1_print(0, 0, "amvdec_vc1 module init\n");

	if (platform_driver_register(&amvdec_vc1_driver)) {
		pr_err("failed to register amvdec_vc1 driver\n");
		return -ENODEV;
	}
	vcodec_profile_register(&amvdec_vc1_profile);
	vcodec_feature_register(VFORMAT_VC1, 0);
	return 0;
}

static void __exit amvdec_vc1_driver_remove_module(void)
{
	pr_debug("amvdec_vc1_v4l module remove.\n");

	platform_driver_unregister(&amvdec_vc1_driver);
}
module_param(unstable_pts_debug, uint, 0664);
MODULE_PARM_DESC(unstable_pts_debug, "\n amvdec_vc1_v4l unstable_pts\n");

module_param(udebug_flag, uint, 0664);
MODULE_PARM_DESC(udebug_flag, "\n amvdec_vc1_v4l udebug_flag\n");

module_param(debug, uint, 0664);
MODULE_PARM_DESC(debug, "\n amvdec_vc1_v4l debug\n");

module_param(debug_mask, uint, 0664);
MODULE_PARM_DESC(debug_mask, "\n amvdec_vc1_v4l debug_mask\n");

module_param(wait_time, uint, 0664);
MODULE_PARM_DESC(wait_time, "\n amvdec_vc1_v4l wait_time\n");

/****************************************/
module_init(amvdec_vc1_driver_init_module);
module_exit(amvdec_vc1_driver_remove_module);

MODULE_DESCRIPTION("AMLOGIC VC1 Video Decoder Driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Qi Wang <qi.wang@amlogic.com>");
