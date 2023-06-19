 /*
 * drivers/amlogic/amports/avs3.c
 *
 * Copyright (C) 2015 Amlogic, Inc. All rights reserved.
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
*/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/semaphore.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/kfifo.h>
#include <linux/kthread.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <linux/amlogic/media/vfm/vframe.h>
#include <linux/amlogic/media/utils/amstream.h>
#include <linux/amlogic/media/utils/vformat.h>
#include <linux/amlogic/media/frame_sync/ptsserv.h>
#include <linux/amlogic/media/frame_sync/tsync.h>
#include <linux/amlogic/media/canvas/canvas.h>
#include <linux/amlogic/media/vfm/vframe_provider.h>
#include <linux/amlogic/media/vfm/vframe_receiver.h>
#include <linux/dma-mapping.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
#include <linux/dma-map-ops.h>
#else
#include <linux/dma-contiguous.h>
#endif
#include <linux/slab.h>
//#include <linux/amlogic/tee.h>
#include <uapi/linux/tee.h>
#include <linux/sched/clock.h>
#include "../../../stream_input/amports/amports_priv.h"
#include <linux/amlogic/media/codec_mm/codec_mm.h>
#include "../utils/decoder_mmu_box.h"
#include "../utils/decoder_bmmu_box.h"
#include "avs3_global.h"
#include "../../../common/media_utils/media_utils.h"
//#define TMP_DEBUG

//#define USE_FRONT_ISR_HANDLE_FOR_BACK

#define FOR_S5
#define PXP_DEBUG
#define USE_SIM_BUFSPEC

//#define DEBUG_CMD

#define MEM_NAME "codec_avs3"
#include "../utils/vdec.h"
#include "../utils/amvdec.h"

#include <linux/amlogic/media/video_sink/video.h>
#include <linux/amlogic/media/codec_mm/configs.h>
#include "../utils/config_parser.h"
#include "../utils/firmware.h"
#include "../../../common/chips/decoder_cpu_ver_info.h"
#include "../utils/vdec_feature.h"
#include "../utils/decoder_dma_alloc.h"
#include "../utils/vdec_profile.h"
#include "../../../media_sync/pts_server/pts_server_core.h"

#define DYN_CACHE
#define I_ONLY_SUPPORT
#define MIX_STREAM_SUPPORT
//#define CONSTRAIN_MAX_BUF_NUM

#define CO_MV_COMPRESS

#include "vavs3.h"
#define HEVC_SHIFT_LENGTH_PROTECT                  0x313a
#define HEVC_MPRED_CTRL4                           0x324c
#define HEVC_DBLK_CFGD                             0x350d
#define HEVC_CM_HEADER_START_ADDR                  0x3628
#define HEVC_DBLK_CFGB                             0x350b
#define HEVCD_MPP_ANC2AXI_TBL_DATA                 0x3464
#define HEVC_SAO_MMU_VH1_ADDR                      0x363b
#define HEVC_SAO_MMU_VH0_ADDR                      0x363a

#define HEVC_CM_BODY_LENGTH2                       0x3663
#define HEVC_CM_HEADER_OFFSET2                     0x3664
#define HEVC_CM_HEADER_LENGTH2                     0x3665

#define HEVC_ASSIST_MMU_MAP_ADDR                   0x3009

#define HEVC_CM_HEADER_START_ADDR                  0x3628
#define HEVC_CM_HEADER_START_ADDR2                 0x364a
#define HEVC_SAO_MMU_VH1_ADDR                      0x363b
#define HEVC_SAO_MMU_VH0_ADDR                      0x363a
#define HEVC_SAO_MMU_VH0_ADDR2                     0x364d
#define HEVC_SAO_MMU_VH1_ADDR2                     0x364e

#define HEVC_SAO_MMU_DMA_CTRL2                     0x364c
#define HEVC_SAO_MMU_STATUS2                       0x3650
#define HEVC_DW_VH0_ADDDR                          0x365e
#define HEVC_DW_VH1_ADDDR                          0x365f

#define HEVC_SAO_CTRL9                             0x362d

/*
 * AVS3_DEC_STATUS define
*/
/*internal*/
#define AVS3_DEC_IDLE                           0
#define AVS3_SEQUENCE                           1
#define AVS3_I_PICTURE                          2
#define AVS3_PB_PICTURE                         3
#define AVS3_DISCARD_STARTCODE                  4
#define AVS3_DISCARD_NAL                        4

#define AVS3_SLICE_DECODING                     6

#define SWAP_IN_CMD                          0x10
#define SWAP_OUT_CMD                         0x11
#define SWAP_OUTIN_CMD                       0x12
#define SWAP_DONE                            0x13
#define SWAP_POST_INIT                       0x14

/*head*/
#define AVS3_HEAD_SEQ_READY                  0x21
#define AVS3_HEAD_PIC_I_READY                0x22
#define AVS3_HEAD_PIC_PB_READY               0x23
#define AVS3_HEAD_SEQ_END_READY              0x24
#define AVS3_STARTCODE_SEARCH_DONE           0x25

/*pic done*/
#define HEVC_DECPIC_DATA_DONE       0x30
#define HEVC_DECPIC_DATA_ERROR      0x31
#define HEVC_NAL_DECODE_DONE        0x32
#define AVS3_DECODE_BUFEMPTY        0x33
#define AVS3_DECODE_TIMEOUT         0x34
#define AVS3_DECODE_OVER_SIZE       0x35
#define AVS3_EOS                    0x36

/*cmd*/
#define AVS3_10B_DISCARD_NAL                 0xf0
#define AVS3_SEARCH_NEW_PIC                  0xf1
#define AVS3_ACTION_ERROR                    0xfe
#define HEVC_ACTION_ERROR                    0xfe
#define AVS3_ACTION_DONE                     0xff
/*AVS3_DEC_STATUS end*/

//???
#define HEVC_BE_DECODE_DATA        0xa0
#define HEVC_BE_DECODE_DATA_DONE   0xb0
#define HEVC_BE_DECODE_TIMEOUT     0xc0
/*AVS3_BE_DEC_STATUS end*/

#define VF_POOL_SIZE        32

#undef pr_info
#define pr_info printk

#define DECODE_MODE_SINGLE				(0 | (0x80 << 24))
#define DECODE_MODE_MULTI_STREAMBASE	(1 | (0x80 << 24))
#define DECODE_MODE_MULTI_FRAMEBASE		(2 | (0x80 << 24))

#define  VP9_TRIGGER_FRAME_DONE		0x100
#define  VP9_TRIGGER_FRAME_ENABLE	0x200

/*#define MV_MEM_UNIT 0x240*/
#define MV_MEM_UNIT 0x200
/*---------------------------------------------------
 Include "parser_cmd.h"
---------------------------------------------------*/
#define PARSER_CMD_SKIP_CFG_0 0x0000090b

#define PARSER_CMD_SKIP_CFG_1 0x1b14140f

#define PARSER_CMD_SKIP_CFG_2 0x001b1910

#define PARSER_CMD_NUMBER 37

static unsigned short parser_cmd[PARSER_CMD_NUMBER] = {
0x0401,
0x8401,
0x0800,
0x0402,
0x9002,
0x1423,
0x8CC3,
0x1423,
0x8804,
0x9825,
0x0800,
0x04FE,
0x8406,
0x8411,
0x1800,
0x8408,
0x8409,
0x8C2A,
0x9C2B,
0x1C00,
0x840F,
0x8407,
0x8000,
0x8408,
0x2000,
0xA800,
0x8410,
0x04DE,
0x840C,
0x840D,
0xAC00,
0xA000,
0x08C0,
0x08E0,
0xA40E,
0xFC00,
0x7C00
};

static int32_t g_WqMDefault4x4[16] = {
	64,     64,     64,     68,
	64,     64,     68,     72,
	64,     68,     76,     80,
	72,     76,     84,     96
};

static int32_t g_WqMDefault8x8[64] = {
	64,     64,     64,     64,     68,     68,     72,     76,
	64,     64,     64,     68,     72,     76,     84,     92,
	64,     64,     68,     72,     76,     80,     88,     100,
	64,     68,     72,     80,     84,     92,     100,    112,
	68,     72,     80,     84,     92,     104,    112,    128,
	76,     80,     84,     92,     104,    116,    132,    152,
	96,     100,    104,    116,    124,    140,    164,    188,
	104,    108,    116,    128,    152,    172,    192,    216
};
/*#define HEVC_PIC_STRUCT_SUPPORT*/
/* to remove, fix build error */

/*#define CODEC_MM_FLAGS_FOR_VDECODER  0*/

#define MULTI_INSTANCE_SUPPORT
/* #define ERROR_HANDLE_DEBUG */

#ifndef STAT_KTHREAD
#define STAT_KTHREAD 0x40
#endif

#ifdef MULTI_INSTANCE_SUPPORT
#define MAX_DECODE_INSTANCE_NUM     12
#define MULTI_DRIVER_NAME "ammvdec_avs3"

#define lock_buffer(dec, flags) \
		spin_lock_irqsave(&dec->buffer_lock, flags)

#define unlock_buffer(dec, flags) \
		spin_unlock_irqrestore(&dec->buffer_lock, flags)

#define lock_front_back(dec, flags) \
		spin_lock_irqsave(&dec->front_back_lock, flags)

#define unlock_front_back(dec, flags) \
		spin_unlock_irqrestore(&dec->front_back_lock, flags)

u32 debug_mask = 0xffffffff;
#define get_dbg_flag(dec) ((debug_mask & (1 << dec->index)) ? debug : 0)

static unsigned int max_decode_instance_num
				= MAX_DECODE_INSTANCE_NUM;
static unsigned int decode_frame_count[MAX_DECODE_INSTANCE_NUM];
static unsigned int display_frame_count[MAX_DECODE_INSTANCE_NUM];
static unsigned int max_process_time[MAX_DECODE_INSTANCE_NUM];
static unsigned int run_count[MAX_DECODE_INSTANCE_NUM];
#ifdef NEW_FB_CODE
static unsigned int max_process_time_back[MAX_DECODE_INSTANCE_NUM];
static unsigned int run_count_back[MAX_DECODE_INSTANCE_NUM];
#endif
static unsigned int input_empty[MAX_DECODE_INSTANCE_NUM];
static unsigned int not_run_ready[MAX_DECODE_INSTANCE_NUM];

static u32 decode_timeout_val = 200;

static u32 over_decoder_shiftbytes = 0x80;

#ifdef NEW_FB_CODE
static unsigned int decode_timeout_val_back = 200;
static unsigned int fast_timer_check_count = 3;

static unsigned int efficiency_mode = 1;

static void avs3_work_back(struct work_struct *work);
static void avs3_timeout_work_back(struct work_struct *work);
#endif
#endif

static int start_decode_buf_level = 0x8000;

static u32 work_buf_size;

static u32 mv_buf_margin;
static int pre_decode_buf_level = 0x1000;
static u32 again_threshold;

/* DOUBLE_WRITE_MODE is enabled only when NV21 8 bit output is needed */
/* double_write_mode:
 * 0, no double write;
 * 1, 1:1 ratio;
 * 2, (1/4):(1/4) ratio;
 * 3, (1/4):(1/4) ratio, with both compressed frame included
 * 4, (1/2):(1/2) ratio;
 * 8, (1/8):(1/8) ratio;
 * 0x10, double write only
 * 0x100, if > 1080p,use mode 4,else use mode 1;
 * 0x200, if > 1080p,use mode 2,else use mode 1;
 * 0x300, if > 720p, use mode 4, else use mode 1;
 * 0x10000, double write p010 enable
 */
static u32 double_write_mode;

/* triple_write_mode:
 * 0, no triple write;
 * 1, 1:1 ratio;
 * 2, (1/4):(1/4) ratio;
 * 3, (1/4):(1/4) ratio, with both compressed frame included
 * 4, (1/2):(1/2) ratio;
 * 5, (1/2):(1/2) ratio, with both compressed frame included
 * 8, (1/8):(1/8) ratio
 * 0x10000, triple write p010 enable
 */
static u32 triple_write_mode;

static u32 without_display_mode;

static u32 trace_debug_mode = 7;
static u32 paral_alloc_buffer_mode = 1;

static u32 mv_buf_dynamic_alloc;

#define DRIVER_NAME "amvdec_avs3"
#define DRIVER_HEADER_NAME "amvdec_avs3_header"

#define PUT_INTERVAL        (HZ/100)
#define ERROR_SYSTEM_RESET_COUNT   200

#define PTS_NORMAL                0
#define PTS_NONE_REF_USE_DURATION 1

#define PTS_MODE_SWITCHING_THRESHOLD           3
#define PTS_MODE_SWITCHING_RECOVERY_THRESHOLD 3

#define DUR2PTS(x) ((x)*90/96)

struct AVS3Decoder_s;
static int vavs3_vf_states(struct vframe_states *states, void *);
static struct vframe_s *vavs3_vf_peek(void *);
static struct vframe_s *vavs3_vf_get(void *);
static void vavs3_vf_put(struct vframe_s *, void *);
static int vavs3_event_cb(int type, void *data, void *private_data);
static void set_vframe(struct AVS3Decoder_s *dec,
	struct vframe_s *vf, struct avs3_frame_s *pic, u8 dummy);

static int vavs3_stop(struct AVS3Decoder_s *dec);
static s32 vavs3_init(struct vdec_s *vdec);
static void vavs3_prot_init(struct AVS3Decoder_s *dec);
static int vavs3_local_init(struct AVS3Decoder_s *dec);
static void vavs3_put_timer_func(struct timer_list *timer);
static void dump_data(struct AVS3Decoder_s *dec, int size);
static unsigned char get_data_check_sum
	(struct AVS3Decoder_s *dec, int size);
static struct avs3_frame_s *get_pic_by_index(
	struct AVS3Decoder_s *dec, int index);
void pic_backend_ref_operation(struct AVS3Decoder_s *dec, avs3_frame_t *pic, bool add_flag);
static int avs3_hw_ctx_restore(struct AVS3Decoder_s *dec);
static void dump_or_fill_phy_buffer(struct AVS3Decoder_s *dec, u32 phy_adr, u32 size, char *file, u8 flag, char *mark);
static void d_dump(struct AVS3Decoder_s *dec, unsigned int phy_adr, int size,
	struct file *fp, loff_t *wr_off, u32 * total_check_sum, u8 print_flag);

static const char vavs3_dec_id[] = "vavs3-dev";

#define PROVIDER_NAME   "decoder.avs3"
#define MULTI_INSTANCE_PROVIDER_NAME    "vdec.avs3"

static const struct vframe_operations_s vavs3_vf_provider = {
	.peek = vavs3_vf_peek,
	.get = vavs3_vf_get,
	.put = vavs3_vf_put,
	.event_cb = vavs3_event_cb,
	.vf_states = vavs3_vf_states,
};

static struct vframe_provider_s vavs3_vf_prov;

static u32 bit_depth_luma;
static u32 bit_depth_chroma;
static u32 frame_width;
static u32 frame_height;
static u32 video_signal_type;
static u32 pts_unstable;
static u32 on_no_keyframe_skiped;

static u32 force_video_signal_type;
static u32 enable_force_video_signal_type;
#define VIDEO_SIGNAL_TYPE_AVAILABLE_MASK	0x20000000
#define HDR_CUVA_MASK                           0x40000000

static const char * const video_format_names[] = {
	"component", "PAL", "NTSC", "SECAM",
	"MAC", "unspecified", "Reserved", "Reserved"
};

static inline int div_r32(int64_t m, int n)
{
/*
return (int)(m/n)
*/
#ifndef CONFIG_ARM64
	int64_t qu = 0;
	qu = div_s64(m, n);
	return (int)qu;
#else
	return (int)(m/n);
#endif
}

enum vpx_bit_depth_t {
	AVS3_BITS_8  =  8,  /**<  8 bits */
	AVS3_BITS_10 = 10,  /**< 10 bits */
	AVS3_BITS_12 = 12,  /**< 12 bits */
};

enum trace_debug_t {
	TRACE_BASIC  =  1,
	TRACE_BUFFER = 2,
	TRACE_PERFORMANCE_DETAIL = 4,
};

enum alloc_buffer_status_t {
	BUFFER_INIT  =  0,
	BUFFER_ALLOCATING = 1,
	BUFFER_ALLOCATE_DONE = 2,
};

/*USE_BUF_BLOCK*/
struct BUF_s {
	int index;
	unsigned int alloc_flag;
	/*buffer */
	unsigned int cma_page_count;
	unsigned long alloc_addr;
	unsigned long start_adr;
	unsigned int size;

	unsigned int free_start_adr;
} /*BUF_t */;

struct MVBUF_s {
	unsigned long start_adr;
	unsigned int size;
	int used_flag;
} /*MVBUF_t */;

	/* #undef BUFMGR_ONLY to enable hardware configuration */

/*#define TEST_WR_PTR_INC*/
#define WR_PTR_INC_NUM 128

#define SIMULATION
#define DOS_PROJECT
#undef MEMORY_MAP_IN_REAL_CHIP

/*#undef DOS_PROJECT*/
/*#define MEMORY_MAP_IN_REAL_CHIP*/

/*#define BUFFER_MGR_ONLY*/
/*#define CONFIG_HEVC_CLK_FORCED_ON*/
/*#define ENABLE_SWAP_TEST*/

#ifdef AVS3_10B_NV21
#define MEM_MAP_MODE 2  /* 0:linear 1:32x32 2:64x32*/
#else
#define MEM_MAP_MODE 0  /* 0:linear 1:32x32 2:64x32*/
#endif

#ifdef AVS3_10B_NV21
#else
#define LOSLESS_COMPRESS_MODE
#endif

#define DOUBLE_WRITE_YSTART_TEMP 0x02000000
#define DOUBLE_WRITE_CSTART_TEMP 0x02900000

#if (defined DEBUG_UCODE_LOG) || (defined DEBUG_CMD)
//#define UCODE_LOG_BUF_SIZE   (16 * 1024)
#define UCODE_LOG_BUF_SIZE   (1024 * 1024)
#endif

#ifdef DEBUG_CMD
static u32 debug_cmd_wait_count;
static u32 debug_cmd_wait_type;
#endif

/* bit 0, disable backend
	bit 1, start frontend only after last backend done (not pipeline front-back)
	bit 2, do not loop buffer (use with bit[1] of 1)
	bit 3, fill loop buffer 0 before starting front decoding
	##
	bit[31:28],1: dump loop buffer at frontend data done for specific frame to file
			bit[27:16], frame num
	bit[31:28],2: dump frame count of frames to file
			bit[27:16], frame count
	bit[31:28],3: dump every frame and print checksum
	bit[31:28] 8: dump loop buffer to file in dump_state()
*/
static u32 fbdebug_flag;

static u32 debug;

static u32 debug_again;

u32 avs3_get_debug_flag(void)
{
	return debug;
}

bool is_avs3_print_param(void)
{
	bool ret = false;
	if (debug & AVS3_DBG_PRINT_PARAM)
		ret = true;
	return ret;
}

inline bool is_avs3_print_bufmgr_detail(void)
{
	bool ret = false;
	if (debug & AVS3_DBG_BUFMGR_DETAIL)
		ret = true;
	return ret;
}
static bool is_reset;
/*for debug*/
static u32 force_bufspec;
/*
	udebug_flag:
	bit 0, enable ucode print
	bit 1, enable ucode detail print
	bit [31:16] not 0, pos to dump lmem
		bit 2, pop bits to lmem
		bit [11:8], pre-pop bits for alignment (when bit 2 is 1)
*/
static u32 udebug_flag;
/*
	when udebug_flag[1:0] is not 0
	udebug_pause_pos not 0,
		pause position
*/
static u32 udebug_pause_pos;
/*
	when udebug_flag[1:0] is not 0
	and udebug_pause_pos is not 0,
		pause only when DEBUG_REG2 is equal to this val
*/
static u32 udebug_pause_val;

static u32 udebug_pause_decode_idx;

static u32 force_disp_pic_index;

static u32 dump_phy_adr;
static u32 dump_phy_size;

#define AUX_BUF_ALIGN(adr) ((adr + 0xf) & (~0xf))
static u32 cuva_buf_size = 512;

#define DEBUG_REG
#ifdef DEBUG_REG
static void WRITE_VREG_DBG2(unsigned adr, unsigned val)
{
	if (debug & AVS3_DBG_REG) {
		unsigned adr_conv = dos_reg_compat_convert(adr);
		pr_info("%s(%x, %x)\n", __func__, adr_conv, val);
	}
	if (adr != 0)
		WRITE_VREG(adr, val);

	if (debug & AVS3_DBG_BUFMGR_DETAIL) {
		if (adr == HEVC_ASSIST_SCRATCH_0) // && val == AVS3_ACTION_DONE)
			pr_info("!!write HEVC_ASSIST_SCRATCH_0 %x\n", val);
	}
}

#undef WRITE_VREG
#define WRITE_VREG WRITE_VREG_DBG2
#endif

#define MMU_COMPRESS_HEADER_SIZE_1080P  0x10000
#define MMU_COMPRESS_HEADER_SIZE_4K  0x48000
#define MMU_COMPRESS_HEADER_SIZE_8K  0x120000

#define INVALID_IDX -1  /* Invalid buffer index.*/

#define FRAME_BUFFERS (MAX_PB_SIZE)
#define HEADER_FRAME_BUFFERS (FRAME_BUFFERS)

#define FRAME_CONTEXTS_LOG2 2
#define FRAME_CONTEXTS (1 << FRAME_CONTEXTS_LOG2)
/*buffer + header buffer + workspace*/

#ifdef NEW_FB_CODE
#define	BMMU_IFBUF_SCALELUT_ID		(0)
#define	BMMU_IFBUF_VCPU_IMEM_ID 	(BMMU_IFBUF_SCALELUT_ID + 1)
#define	BMMU_IFBUF_SYS_IMEM_ID		(BMMU_IFBUF_VCPU_IMEM_ID + 1)
#define	BMMU_IFBUF_LMEM0_ID			(BMMU_IFBUF_SYS_IMEM_ID + 1)
#define	BMMU_IFBUF_LMEM1_ID			(BMMU_IFBUF_LMEM0_ID + 1)
#define	BMMU_IFBUF_PARSER_SAO0_ID	(BMMU_IFBUF_LMEM1_ID + 1)
#define	BMMU_IFBUF_PARSER_SAO1_ID	(BMMU_IFBUF_PARSER_SAO0_ID + 1)
#define	BMMU_IFBUFF_MPRED_IMP0_ID	(BMMU_IFBUF_PARSER_SAO1_ID + 1)
#define	BMMU_IFBUFF_MPRED_IMP1_ID	(BMMU_IFBUFF_MPRED_IMP0_ID + 1)
#define FB_LOOP_BUF_COUNT	(BMMU_IFBUFF_MPRED_IMP1_ID + 1)
#else
#define FB_LOOP_BUF_COUNT	0
#endif

#define MV_USE_FIXED_BUF
#ifdef MV_USE_FIXED_BUF
#define MAX_BMMU_BUFFER_NUM (FB_LOOP_BUF_COUNT + (FRAME_BUFFERS + HEADER_FRAME_BUFFERS + 1)+1)
#define VF_BUFFER_IDX(n) (FB_LOOP_BUF_COUNT + n)
#define HEADER_BUFFER_IDX(n) (FB_LOOP_BUF_COUNT + FRAME_BUFFERS + n+1)
#define WORK_SPACE_BUF_ID (FB_LOOP_BUF_COUNT + FRAME_BUFFERS + HEADER_FRAME_BUFFERS+1)
#else
#define MAX_BMMU_BUFFER_NUM (FB_LOOP_BUF_COUNT + ((FRAME_BUFFERS*2)+HEADER_FRAME_BUFFERS+1)+1)
#define VF_BUFFER_IDX(n) (FB_LOOP_BUF_COUNT + n)
#define HEADER_BUFFER_IDX(n) (FB_LOOP_BUF_COUNT + FRAME_BUFFERS + n+1)
#define MV_BUFFER_IDX(n) (FB_LOOP_BUF_COUNT + (FRAME_BUFFERS * 2) + n+1)
#define WORK_SPACE_BUF_ID (FB_LOOP_BUF_COUNT + (FRAME_BUFFERS * 2) + HEADER_FRAME_BUFFERS+1)
//#define DW_HEADER_BUFFER_IDX(n) ((FRAME_BUFFERS * 3) + n+1)
#endif

#define CO_MV_BUF_SIZE_1080P  0x3fc00
#define CO_MV_BUF_SIZE_4K     0x120000
#define CO_MV_BUF_SIZE_8K     0x480000
/*
static void set_canvas(struct AVS3Decoder_s *dec,
	struct avs3_frame_s *pic);
int avs3_prepare_display_buf(struct AVS3Decoder_s *dec,
					int pos);
*/

#ifdef NEW_FB_CODE
static u32 fb_ifbuf_num = 3;
/*
	0: single core mode
	1: front_back_mode
	2: front_back_test_mode
*/
static u32 front_back_mode = 1;
#endif

struct BuffInfo_s {
	u32 max_width;
	u32 max_height;
	u32 start_adr;
	u32 end_adr;
	struct buff_s ipp;
#ifdef NEW_FRONT_BACK_CODE
	struct buff_s ipp1;
#endif
	struct buff_s sao_abv;
	struct buff_s sao_vb;
	struct buff_s short_term_rps;
	struct buff_s rcs;
	struct buff_s sps;
	struct buff_s pps;
	struct buff_s sbac_top;
	struct buff_s sao_up;
	struct buff_s swap_buf;
	struct buff_s swap_buf2;
	struct buff_s scalelut;
	struct buff_s dblk_para;
	struct buff_s dblk_data;
	struct buff_s dblk_data2;
#ifdef AVS3_10B_MMU
	struct buff_s mmu_vbh;
	struct buff_s cm_header;
#endif
#ifdef AVS3_10B_MMU_DW
	struct buff_s mmu_vbh_dw;
	struct buff_s cm_header_dw;
#endif
	struct buff_s mpred_above;
#ifdef MV_USE_FIXED_BUF
	struct buff_s mpred_mv;
#endif
	struct buff_s rpm;
	struct buff_s lmem;
};

#define DEC_RESULT_NONE             0
#define DEC_RESULT_DONE             1
#define DEC_RESULT_AGAIN            2
#define DEC_RESULT_CONFIG_PARAM     3
#define DEC_RESULT_ERROR            4
#define DEC_INIT_PICLIST			5
#define DEC_UNINIT_PICLIST			6
#define DEC_RESULT_GET_DATA         7
#define DEC_RESULT_GET_DATA_RETRY   8
#define DEC_RESULT_EOS              9
#define DEC_RESULT_FORCE_EXIT       10
#define DEC_RESULT_WAIT_BUFFER      11
#define DEC_RESULT_TIMEOUT          12

#ifdef NEW_FB_CODE
#define DEC_BACK_RESULT_NONE             0
#define DEC_BACK_RESULT_DONE             1
#define DEC_BACK_RESULT_TIMEOUT          2
#define DEC_BACK_RESULT_FORCE_EXIT       10
#endif

static void avs3_work(struct work_struct *work);
struct loop_filter_info_n;
struct loopfilter;
struct segmentation;

struct AVS3Decoder_s {
	int pic_list_init_flag;
	int pic_list_wait_alloc_done_flag;
	unsigned char index;
	spinlock_t front_back_lock;
	spinlock_t buffer_lock;
	struct device *cma_dev;
	struct platform_device *platform_dev;
	void (*vdec_cb)(struct vdec_s *, void *, int);
	void *vdec_cb_arg;
	struct vframe_chunk_s *chunk;
	int dec_result;
	struct work_struct work;
	u32 start_shift_bytes;

	struct BuffInfo_s work_space_buf_store;
	unsigned long buf_start;
	u32 buf_size;
	u32 cma_alloc_count;
	unsigned long cma_alloc_addr;
	uint8_t eos;
	unsigned long int start_process_time;
	unsigned last_lcu_idx;
	int decode_timeout_count;
	unsigned timeout_num;

	int double_write_mode;

	unsigned char m_ins_flag;
	char *provider_name;
	int frame_count;
	u32 stat;
	struct timer_list timer;
	u32 frame_dur;
	u32 frame_ar;
	u32 vavs3_ratio;
	int fatal_error;
	uint8_t init_flag;
	uint8_t first_sc_checked;
	uint8_t process_busy;
#define PROC_STATE_INIT			0
#define PROC_STATE_HEAD_DONE	1
#define PROC_STATE_DECODING		2
#define PROC_STATE_HEAD_AGAIN	3
#define PROC_STATE_DECODE_AGAIN	4
#define PROC_STATE_TEST1		5
	uint8_t process_state;
	u32 ucode_pause_pos;

	int show_frame_num;
#ifndef AVS3_10B_MMU
	struct buff_s mc_buf_spec;
#endif
	struct dec_sysinfo vavs3_amstream_dec_info;
	void *rpm_addr;
	void *lmem_addr;
	dma_addr_t rpm_phy_addr;
	dma_addr_t lmem_phy_addr;
	unsigned short *lmem_ptr;
	unsigned short *debug_ptr;

#if (defined DEBUG_UCODE_LOG) || (defined DEBUG_CMD)
	void *ucode_log_addr;
	dma_addr_t ucode_log_phy_addr;
#endif

#ifdef AVS3_10B_MMU
	bool mmu_enable;

	u32 cuva_size;
	void *cuva_addr;
	dma_addr_t cuva_phy_addr;

	void *frame_mmu_map_addr;
	dma_addr_t frame_mmu_map_phy_addr;
#endif
#ifdef AVS3_10B_MMU_DW
	bool dw_mmu_enable;
	void *dw_mmu_box;
	void *dw_frame_mmu_map_addr;
	dma_addr_t dw_frame_mmu_map_phy_addr;
#endif
	unsigned int use_cma_flag;

	DECLARE_KFIFO(newframe_q, struct vframe_s *, VF_POOL_SIZE);
	DECLARE_KFIFO(display_q, struct vframe_s *, VF_POOL_SIZE);
	DECLARE_KFIFO(pending_q, struct vframe_s *, VF_POOL_SIZE);
	struct vframe_s vfpool[VF_POOL_SIZE];
	u32 vf_pre_count;
	u32 vf_get_count;
	u32 vf_put_count;
	int buf_num;
	unsigned int losless_comp_body_size;

	u32 video_signal_type;
	u32 video_ori_signal_type;

	int pts_mode;
	int last_lookup_pts;
	int last_pts;
	u64 last_lookup_pts_us64;
	u64 last_pts_us64;
	u64 shift_byte_count;
	u32 shift_byte_count_lo;
	u32 shift_byte_count_hi;
	int pts_mode_switching_count;
	int pts_mode_recovery_count;

	bool get_frame_dur;
	u32 saved_resolution;

	/**/
	int refresh_frame_flags;
	uint8_t hold_ref_buf;
	struct BuffInfo_s *work_space_buf;
#ifndef AVS3_10B_MMU
	struct buff_s *mc_buf;
#endif
	unsigned int frame_width;
	unsigned int frame_height;

	unsigned short *rpm_ptr;
	int     init_pic_w;
	int     init_pic_h;

	int     slice_type;

	int decode_idx;
	int slice_idx;
	uint8_t wait_buf;
	uint8_t error_flag;
	unsigned int bufmgr_error_count;

	/* bit 0, for decoding; bit 1, for displaying */
	uint8_t ignore_bufmgr_error;
	uint8_t skip_PB_before_I;
	int PB_skip_mode;
	int PB_skip_count_after_decoding;
	/*hw*/

	/**/
	struct vdec_info *gvs;

	unsigned int dec_status;
	u32 last_put_idx;
	int new_frame_displayed;
	void *mmu_box;
	void *bmmu_box;
	struct vframe_master_display_colour_s vf_dp;
	struct firmware_s *fw;
#ifdef AVS3_10B_MMU
	int cur_fb_idx_mmu;
#endif
	struct avs3_decoder avs3_dec;
	int32_t m_filterCoeffSym[16][9];
	int32_t m_varIndTab[NO_VAR_BINS];

	struct vframe_s vframe_dummy;
		/* start_decoding_flag,
			bit 0, SEQ ready
			bit 1, I ready
		*/
	unsigned char start_decoding_flag;
	uint32_t mpred_abv_start_addr;
	uint32_t mpred_abv_start_addr_bak;
	u8 next_again_flag;
	u32 pre_parser_wr_ptr;
	int need_cache_size;
	u64 sc_start_time;
#ifdef I_ONLY_SUPPORT
	u32 i_only;
#endif
	int frameinfo_enable;
	u32 dynamic_buf_margin;
	int sidebind_type;
	int sidebind_channel_id;
	u32 endian;
	dma_addr_t rdma_phy_adr;
	unsigned *rdma_adr;
	int hdr_flag;

	ulong rpm_mem_handle;
	ulong lmem_phy_handle;
	ulong frame_mmu_map_handle;
	ulong frame_mmu_map_handle_1;
	ulong frame_dw_mmu_map_handle;
	ulong frame_dw_mmu_map_handle_1;
	ulong ucode_log_handle;
	ulong rdma_mem_handle;
	ulong cuva_handle;
#ifdef NEW_FB_CODE
	u32 front_back_mode;
	int fb_ifbuf_num;
	//int pic_wr_count;
	//int pic_rd_count;
	//struct PIC_s *decoded_PIC[MAX_REF_PIC_NUM];
	//u32 flush_count;

	/*init_fb_bufstate() for linux APIs*/
	struct work_struct work_back;
	struct work_struct timeout_work_back;

	void *frame_mmu_map_addr_1;
	dma_addr_t frame_mmu_map_phy_addr_1;
	void *mmu_box_1;

	void *dw_mmu_box_1;
	void *dw_frame_mmu_map_addr_1;
	dma_addr_t dw_frame_mmu_map_phy_addr_1;

	void *mmu_box_fb;
	void *fb_buf_mmu0_addr;
	void *fb_buf_mmu1_addr;
	/**/
	void (*vdec_back_cb)(struct vdec_s *, void *, int);
	void *vdec_back_cb_arg;
	struct firmware_s *fw_back;
	struct timer_list timer_back;
	unsigned long int start_process_time_back;
	unsigned int decode_timeout_count_back;
	unsigned int timeout_num_back;

	int dec_back_result;
	u32 dec_status_back;
	struct mutex fb_mutex;
	u32 mmu_fb_4k_number;
#endif
	uint32_t ASSIST_MBOX0_IRQ_REG;
	uint32_t ASSIST_MBOX0_CLR_REG;
	uint32_t ASSIST_MBOX0_MASK;
	uint32_t backend_ASSIST_MBOX0_IRQ_REG;
	uint32_t backend_ASSIST_MBOX0_CLR_REG;
	uint32_t backend_ASSIST_MBOX0_MASK;

	unsigned char print_buf[1024*16+16];
	int print_buf_len;
	struct trace_decoder_name trace;
	int has_i_frame;
	struct mutex slice_header_lock;
#ifdef OW_TRIPLE_WRITE
	int triple_write_mode;
#endif
	u32 last_monitor_data;
};

static int  compute_losless_comp_body_size(
		struct AVS3Decoder_s *dec, int width, int height,
		uint8_t is_bit_depth_10);

static int avs3_mmu_page_num(struct AVS3Decoder_s *dec,
	int pic_width, int pic_height, int is_bit_depth_10);

static void avs3_work_implement(struct AVS3Decoder_s *dec);


#undef pr_info
#define pr_info printk
int avs3_debug(struct AVS3Decoder_s *dec,
	int flag, const char *fmt, ...)
{
#define HEVC_PRINT_BUF		512
	unsigned char buf[HEVC_PRINT_BUF];
	int len = 0;
	if (dec && dec->print_buf_len>0) {
		dec->print_buf_len = 0;
		pr_info("%s", dec->print_buf);
	}
	if (dec == NULL ||
		(flag == 0) ||
		((debug_mask &
		(1 << dec->index))
		&& (debug & flag))) {
		va_list args;

		va_start(args, fmt);
		if (dec)
			len = sprintf(buf, "[%d]", dec->index);

#ifdef DEBUG_AMRISC
		if (dec && dec->front_back_mode == 1)
			len += sprintf(buf+len, "<%x,%x,%x>", READ_VREG(HEVC_ASSIST_SCRATCH_0), READ_VREG(HEVC_MPC_E), READ_VREG(HEVC_MPC_E_DBE));
		else
			len += sprintf(buf+len, "<%x,%x>", READ_VREG(HEVC_ASSIST_SCRATCH_0), READ_VREG(HEVC_MPC_E));
#endif
		vsnprintf(buf + len, HEVC_PRINT_BUF - len, fmt, args);
		//pr_debug("%s", buf);
		pr_info("%s", buf);
		va_end(args);
	}
	return 0;
}

static void avs3_print_flush(struct AVS3Decoder_s *dec)
{
	if (dec->print_buf_len>0) {
		dec->print_buf_len = 0;
		pr_info("%s", dec->print_buf);
	}
}

static int avs3_print_cont(struct AVS3Decoder_s *dec,
	int flag, const char *fmt, ...)
{
	//unsigned char buf[HEVC_PRINT_BUF];
	//int len = 0;
	if (dec == NULL ||
		(flag == 0) ||
		((debug_mask &
		(1 << dec->index))
		&& (debug & flag))) {
		va_list args;

		va_start(args, fmt);
#if 0
		vsnprintf(buf + len, HEVC_PRINT_BUF - len, fmt, args);
		pr_info("%s", buf);
#else
		if (dec->print_buf_len<1024*16)
			dec->print_buf_len += vsnprintf(dec->print_buf+dec->print_buf_len,
				1024*16-dec->print_buf_len, fmt, args);
		else
			pr_info("print_buf is full\n");
#endif
		va_end(args);
	}
	return 0;
}

#define PROB_SIZE    (496 * 2 * 4)
#define PROB_BUF_SIZE    (0x5000)
#define COUNT_BUF_SIZE   (0x300 * 4 * 4)
/*compute_losless_comp_body_size(4096, 2304, 1) = 18874368(0x1200000)*/
#define MAX_FRAME_4K_NUM 0x1200
#define MAX_FRAME_8K_NUM 0x4800
#define MAX_SIZE_4K (4096 * 2304)
#define IS_8K_SIZE(w, h)	(((w) * (h)) > MAX_SIZE_4K)
#define IS_4K_SIZE(w, h)  (((w) * (h)) > (1920*1088))

/*
error handling
*/
/*error_handle_policy:
bit 0: search seq again if buffer mgr error occur
	(buffer mgr error count need big than
	re_search_seq_threshold)
bit 1:  1: display from I picture; 0: display from any correct pic;
bit 2:  1: not output error frame; 0: output error frame;
bit 3:  1: same poc insert dpb;    0: same poc don't insert dpb;

*/

static u32 error_handle_policy = 5;

int avs3_get_error_policy(void)
{
	return error_handle_policy;
}

static int get_frame_mmu_map_size(void)
{
	if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_SM1)
		return (MAX_FRAME_8K_NUM * 4);

	return (MAX_FRAME_4K_NUM * 4);
}

static int get_compress_header_size(struct AVS3Decoder_s *dec)
{
	int32_t pic_width = dec->avs3_dec.img.width;
	int32_t pic_height = dec->avs3_dec.img.height;

	if ((get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_SM1) &&
		(IS_8K_SIZE(pic_width, pic_height)))
		return MMU_COMPRESS_HEADER_SIZE_8K;
	else if (IS_4K_SIZE(pic_width, pic_height))
		return MMU_COMPRESS_HEADER_SIZE_4K;
	return MMU_COMPRESS_HEADER_SIZE_1080P;
}

static void reset_process_time(struct AVS3Decoder_s *dec)
{
	if (dec->start_process_time) {
		//unsigned process_time =
		//	1000 * (jiffies - dec->start_process_time) / HZ;
		dec->start_process_time = 0;
		//if (process_time > max_process_time[dec->index])
		//	max_process_time[dec->index] = process_time;
	}
}

static void start_process_time(struct AVS3Decoder_s *dec)
{
	dec->start_process_time = jiffies;
	if ((dec->start_process_time != 0) &&
		!(error_handle_policy & 0x4) &&
		(decode_timeout_val > 0)) {
		dec->decode_timeout_count = fast_timer_check_count;
	} else
		dec->decode_timeout_count = 0;
	dec->last_lcu_idx = 0;
}

static void update_decoded_pic(struct AVS3Decoder_s *dec);

static void timeout_process(struct AVS3Decoder_s *dec)
{
	struct avs3_decoder *avs3_dec = &dec->avs3_dec;
	struct avs3_frame_s *cur_pic = avs3_dec->cur_pic;
	dec->timeout_num++;
#ifdef NEW_FB_CODE
	if (dec->front_back_mode == 1) {
		amhevc_stop_f();
	} else
#endif
	amhevc_stop();
	avs3_print(dec,
		0, "%s decoder timeout\n", __func__);
	if (cur_pic)
		cur_pic->error_mark = 1;
	dec->dec_result = DEC_RESULT_DONE;
	update_decoded_pic(dec);
	reset_process_time(dec);
	vdec_schedule_work(&dec->work);
}

#ifdef NEW_FB_CODE
static void reset_process_time_back(struct AVS3Decoder_s *dec)
{
	if (dec->start_process_time_back) {
		unsigned int process_time =
			1000 * (jiffies - dec->start_process_time_back) / HZ;
		dec->start_process_time_back = 0;
		if (process_time > max_process_time_back[dec->index])
			max_process_time_back[dec->index] = process_time;
	}
}

static void start_process_time_back(struct AVS3Decoder_s *dec)
{
	dec->start_process_time_back = jiffies;
	dec->decode_timeout_count_back = fast_timer_check_count;
}

/*
static void restart_process_time_back(struct AVS3Decoder_s *dec)
{
	dec->start_process_time_back = jiffies;
	dec->decode_timeout_count_back = 2;
}
*/
static void timeout_process_back(struct AVS3Decoder_s *dec)
{
	/*
	 * In this very timeout point,the vh265_work arrives,
	 * or in some cases the system become slow,  then come
	 * this second timeout. In both cases we return.
	 */
	if (work_pending(&dec->work_back) ||
		work_busy(&dec->work_back) ||
		work_busy(&dec->timeout_work_back) ||
		work_pending(&dec->timeout_work_back)) {
		pr_err("%s avs3[%d] work back pending, do nothing.\n",__func__, dec->index);
		return;
	}

	/* disable irq before handle timeout */
	WRITE_VREG(dec->backend_ASSIST_MBOX0_MASK, 0);

	avs3_print(dec,
		0, "%s decoder timeout\n", __func__);
	dec->timeout_num_back++;
	reset_process_time_back(dec);

	if (work_pending(&dec->work_back))
		return;
	dec->dec_back_result = DEC_BACK_RESULT_TIMEOUT;
	vdec_schedule_work(&dec->timeout_work_back);
}
#endif

static inline void decoder_trace(char *name, int value, int flag)
{
	if (trace_debug_mode & flag)
		ATRACE_COUNTER(name, value);
}

static u32 get_valid_double_write_mode(struct AVS3Decoder_s *dec)
{
	u32 dw_mode;

	dw_mode = (dec->m_ins_flag &&
		((double_write_mode & 0x80000000) == 0)) ?
		dec->double_write_mode :
		(double_write_mode & 0x7fffffff);
	if (dw_mode & 0x20) {
		if ((get_cpu_major_id() < AM_MESON_CPU_MAJOR_ID_T3)
			&& ((dw_mode & 0xf) == 2 || (dw_mode & 0xf) == 3))
			dw_mode = 0;
	}

	return dw_mode;
}

static int get_double_write_mode(struct AVS3Decoder_s *dec)
{
	u32 valid_dw_mode = get_valid_double_write_mode(dec) & 0xffff;
	int w = dec->avs3_dec.img.width;
	int h = dec->avs3_dec.img.height;
	u32 dw = 0x1; /*1:1*/
	switch (valid_dw_mode) {
	case 0x100:
		if (w > 1920 && h > 1088)
			dw = 0x4; /*1:2*/
		break;
	case 0x200:
		if (w > 1920 && h > 1088)
			dw = 0x2; /*1:4*/
		break;
	case 0x300:
		if (w > 1280 && h > 720)
			dw = 0x4; /*1:2*/
		break;
	default:
		dw = valid_dw_mode;
		break;
	}
	return dw;
}

#ifdef OW_TRIPLE_WRITE
static int get_triple_write_mode(struct AVS3Decoder_s *dec)
{
	int tw = ((triple_write_mode & 0x80000000) == 0) ?
		dec->triple_write_mode : (triple_write_mode & 0x7fffffff);

	return (tw & 0xffff);
}
#endif

#ifdef P010_ENABLE
static __inline__ bool is_dw_p010(struct AVS3Decoder_s *dec)
{
	int a = (double_write_mode & 0x80000000) ? (double_write_mode & 0x10000) : 0;

	return ((dec->double_write_mode & 0x10000) || a);
}

static __inline__ bool is_tw_p010(struct AVS3Decoder_s *dec)
{
	int a = (triple_write_mode & 0x80000000) ? (triple_write_mode & 0x10000) : 0;

	return ((dec->triple_write_mode & 0x10000) || a);
}

#else
static __inline__ bool is_dw_p010(struct AVS3Decoder_s *dec)
{
	return 0;
}

static __inline__ bool is_tw_p010(struct AVS2Decoder_s *dec)
{
	return 0;
}
#endif

/* for double write buf alloc */
static int get_double_write_mode_init(struct AVS3Decoder_s *dec)
{
	u32 valid_dw_mode = get_valid_double_write_mode(dec);
	u32 dw;
	int w = dec->init_pic_w;
	int h = dec->init_pic_h;

	dw = 0x1; /*1:1*/
	switch (valid_dw_mode) {
	case 0x100:
		if (w > 1920 && h > 1088)
			dw = 0x4; /*1:2*/
		break;
	case 0x200:
		if (w > 1920 && h > 1088)
			dw = 0x2; /*1:4*/
		break;
	case 0x300:
		if (w > 1280 && h > 720)
			dw = 0x4; /*1:2*/
		break;
	default:
		dw = valid_dw_mode;
		break;
	}
	return dw;
}

//#define	MAX_4K_NUM		0x1200
#ifdef AVS3_10B_MMU
int avs3_alloc_mmu(
	struct AVS3Decoder_s *dec,
	int cur_buf_idx,
	int pic_width,
	int pic_height,
	unsigned short bit_depth,
	unsigned int *mmu_index_adr)
{
	int cur_mmu_4k_number;
	int bit_depth_10 = (bit_depth == AVS3_BITS_10);
	int ret;

	cur_mmu_4k_number = avs3_mmu_page_num(dec, pic_width, pic_height, bit_depth_10);
	if (cur_mmu_4k_number < 0) {
		pr_err("%s: cur_mmu_4k_number 0x%x is error width %d height %d\n",
			__func__, cur_mmu_4k_number, pic_width, pic_height);
		return -1;
	}

	avs3_print(dec, AVS3_DBG_BUFMGR_MORE,
		"%s decoder_mmu_box_alloc_idx index=%d mmu_4k_number %d\n",
		__func__, cur_buf_idx, cur_mmu_4k_number);

	decoder_trace(dec->trace.decode_header_memory_time_name, TRACE_HEADER_MEMORY_START, TRACE_BASIC);
	ret = decoder_mmu_box_alloc_idx(
		dec->mmu_box,
		cur_buf_idx,
		cur_mmu_4k_number,
		mmu_index_adr);
	decoder_trace(dec->trace.decode_header_memory_time_name, TRACE_HEADER_MEMORY_END, TRACE_BASIC);

	return ret;
}
#endif

#ifdef AVS3_10B_MMU_DW
int avs3_alloc_dw_mmu(
	struct AVS3Decoder_s *dec,
	int cur_buf_idx,
	int pic_width,
	int pic_height,
	unsigned short bit_depth,
	unsigned int *mmu_index_adr)
{
	int cur_mmu_4k_number;
	int bit_depth_10 = (bit_depth == AVS3_BITS_10);
	int ret;

	cur_mmu_4k_number = avs3_mmu_page_num(dec, pic_width, pic_height, bit_depth_10);
	if (cur_mmu_4k_number < 0) {
		return -1;
	}

	avs3_print(dec, AVS3_DBG_BUFMGR_MORE,
		"%s decoder_mmu_box_alloc_idx index=%d mmu_4k_number %d\n",
		__func__, cur_buf_idx, cur_mmu_4k_number);

	decoder_trace(dec->trace.decode_header_memory_time_name, TRACE_HEADER_MEMORY_START, TRACE_BASIC);
	ret = decoder_mmu_box_alloc_idx(
		dec->dw_mmu_box,
		cur_buf_idx,
		cur_mmu_4k_number,
		mmu_index_adr);
	decoder_trace(dec->trace.decode_header_memory_time_name, TRACE_HEADER_MEMORY_END, TRACE_BASIC);

	return ret;
}
#endif

static int get_free_buf_count(struct AVS3Decoder_s *dec)
{
	struct avs3_decoder *avs3_dec = &dec->avs3_dec;
	int ii;
	int count = 0;
	for (ii = 0; ii < avs3_dec->max_pb_size; ii++) {
		if (avs3_dec->pic_pool[ii].buf_cfg.used == 0)
			count++;
	}
	return count;
}

#ifdef CONSTRAIN_MAX_BUF_NUM
static int get_vf_ref_only_buf_count(struct AVS3Decoder_s *dec)
{
	struct avs3_decoder *avs3_dec = &dec->avs3_dec;
	int ii;
	int count = 0;
	for (ii = 0; ii < avs3_dec->max_pb_size; ii++) {
		if (avs3_dec->pic_pool[ii].buf_cfg.used > 0 &&
			avs3_dec->pic_pool[ii].buf_cfg.vf_ref > 0 &&
#ifdef NEW_FRONT_BACK_CODE
			avs3_dec->pic_pool[ii].buf_cfg.backend_ref == 0 && /*?? why need it in case0763,
				                               used is supposed to be 1 if backend_ref is not 0*/
#endif
			avs3_dec->pic_pool[ii].is_ref == 0 &&
			avs3_dec->pic_pool[ii].need_for_out == 0
			) {
			count++;
		}
	}
	return count;
}

static int get_used_buf_count(struct AVS3Decoder_s *dec)
{
	struct avs3_decoder *avs3_dec = &dec->avs3_dec;
	int ii;
	int count = 0;
	for (ii = 0; ii < avs3_dec->max_pb_size; ii++) {
		if (avs3_dec->pic_pool[ii].buf_cfg.used > 0)
			count++;
	}
	return count;
}
#endif
int avs3_dec_init(struct AVS3Decoder_s *dec, struct BuffInfo_s *buf_spec_i,
		struct buff_s *mc_buf_i) {

	dec->frame_count = 0;
#ifdef AVS3_10B_MMU
	dec->cur_fb_idx_mmu = INVALID_IDX;
#endif

	/* private init */
	dec->work_space_buf = buf_spec_i;
#ifndef AVS3_10B_MMU
	dec->mc_buf = mc_buf_i;
#endif
	dec->rpm_addr = NULL;
	dec->lmem_addr = NULL;

	dec->use_cma_flag = 0;
	dec->decode_idx = 0;
	dec->slice_idx = 0;
	/*int m_uiMaxCUWidth = 1<<7;*/
	/*int m_uiMaxCUHeight = 1<<7;*/
	dec->wait_buf = 0;
	dec->error_flag = 0;
	dec->skip_PB_before_I = 0;

	dec->pts_mode = PTS_NORMAL;
	dec->last_pts = 0;
	dec->last_lookup_pts = 0;
	dec->last_pts_us64 = 0;
	dec->last_lookup_pts_us64 = 0;
	dec->shift_byte_count = 0;
	dec->shift_byte_count_lo = 0;
	dec->shift_byte_count_hi = 0;
	dec->pts_mode_switching_count = 0;
	dec->pts_mode_recovery_count = 0;

	dec->buf_num = 0;
	dec->has_i_frame = 0;
	dec->bufmgr_error_count = 0;
#ifdef NEW_FB_CODE
	dec->start_process_time_back = 0;
	dec->decode_timeout_count_back = 0;
	dec->timeout_num_back = 0;
	dec->decode_timeout_count_back = 0;
#endif
	return 0;
}

#define HEVC_CM_BODY_START_ADDR                    0x3626
#define HEVC_CM_BODY_LENGTH                        0x3627
#define HEVC_CM_HEADER_LENGTH                      0x3629
#define HEVC_CM_HEADER_OFFSET                      0x362b

#define LOSLESS_COMPRESS_MODE

/*#define DECOMP_HEADR_SURGENT*/

static u32 mem_map_mode; /* 0:linear 1:32x32 2:64x32 ; m8baby test1902 */
static u32 enable_mem_saving = 1;
static u32 force_w_h;

static u32 force_fps;

const u32 avs3_version = 201602101;
static u32 radr;
static u32 rval;
static u32 pop_shorts;
	/*dbg_cmd[3:0]:
		2, dump phy address of (dbg_cmd[31:4]<<4)
	*/
static u32 dbg_cmd;
static u32 dbg_skip_decode_index;
/*
 * bit 0~3, for HEVCD_IPP_AXIIF_CONFIG endian config
 * bit 8~23, for HEVC_SAO_CTRL1 endian config
 */
static u32 endian;
#define HEVC_CONFIG_BIG_ENDIAN     ((0x880 << 8) | 0x8)
#define HEVC_CONFIG_LITTLE_ENDIAN  ((0xff0 << 8) | 0xf)
#define HEVC_CONFIG_P010_LE        (0x77007)

#ifdef ERROR_HANDLE_DEBUG
static u32 dbg_nal_skip_flag;
		/* bit[0], skip vps; bit[1], skip sps; bit[2], skip pps */
static u32 dbg_nal_skip_count;
#endif
/*for debug*/
static u32 decode_pic_begin;
static uint slice_parse_begin;
static u32 step;
#ifdef MIX_STREAM_SUPPORT
static u32 buf_alloc_width = 4096;
static u32 buf_alloc_height = 2304;

static u32 dynamic_buf_num_margin;
#else
static u32 buf_alloc_width;
static u32 buf_alloc_height;
static u32 dynamic_buf_num_margin = 7;
#endif
#ifdef CONSTRAIN_MAX_BUF_NUM
static u32 run_ready_max_vf_only_num;
static u32 run_ready_display_q_num;
	/*0: not check
		0xff: avs3_dec.ref_maxbuffer
		*/
static u32 run_ready_max_buf_num = 0xff;
#endif
static u32 buf_alloc_depth = 10;
static u32 buf_alloc_size;

/*
bit[0]: 0,
	bit[1]: 0, always release cma buffer when stop
	bit[1]: 1, never release cma buffer when stop
bit[0]: 1, when stop, release cma buffer if blackout is 1;
do not release cma buffer is blackout is not 1

bit[2]: 0, when start decoding, check current displayed buffer
		(only for buffer decoded by vp9) if blackout is 0
		1, do not check current displayed buffer

bit[3]: 1, if blackout is not 1, do not release current
			displayed cma buffer always.
*/
/* set to 1 for fast play;
	set to 8 for other case of "keep last frame"
*/
static u32 buffer_mode = 1;
/* buffer_mode_dbg: debug only*/
static u32 buffer_mode_dbg = 0xffff0000;
/**/

/*
bit 0, 1: only display I picture;
bit 1, 1: only decode I picture;
*/
static u32 i_only_flag;

static u32 max_decoding_time;

/*
re_search_seq_threshold:
	bit 7~0: buffer mgr error research seq count
	bit 15~8: frame count threshold
*/
static u32 re_search_seq_threshold = 0x800; /*0x8;*/
/*static u32 parser_sei_enable = 1;*/

static u32 max_buf_num = (MAX_NUM_REF_PICS);

static u32 run_ready_min_buf_num = 1;

static DEFINE_MUTEX(vavs3_mutex);

#define HEVC_DEC_STATUS_REG       HEVC_ASSIST_SCRATCH_0
#define HEVC_RPM_BUFFER           HEVC_ASSIST_SCRATCH_1
#define AVS3_ALF_SWAP_BUFFER      HEVC_ASSIST_SCRATCH_2
#define HEVC_RCS_BUFFER           HEVC_ASSIST_SCRATCH_3
//#define HEVC_SPS_BUFFER           HEVC_ASSIST_SCRATCH_4
//#define HEVC_PPS_BUFFER           HEVC_ASSIST_SCRATCH_5
#define HEVC_SAO_UP               HEVC_ASSIST_SCRATCH_6
#ifdef AVS3_10B_MMU
#define AVS3_MMU_MAP_BUFFER       HEVC_ASSIST_SCRATCH_7
#else
#define HEVC_STREAM_SWAP_BUFFER   HEVC_ASSIST_SCRATCH_7
#endif
#define HEVC_STREAM_SWAP_BUFFER2  HEVC_ASSIST_SCRATCH_8
/*
#define VP9_PROB_SWAP_BUFFER      HEVC_ASSIST_SCRATCH_9
#define VP9_COUNT_SWAP_BUFFER     HEVC_ASSIST_SCRATCH_A
#define VP9_SEG_MAP_BUFFER        HEVC_ASSIST_SCRATCH_B
*/
//#define AVS3_CUVA_ADR             HEVC_ASSIST_SCRATCH_A
//#define AVS3_CUVA_DATA_SIZE       HEVC_ASSIST_SCRATCH_B
/*same as simulation*/
#define HEVC_DECODE_COUNT       HEVC_ASSIST_SCRATCH_A
#define HEVC_DECODE_SIZE		HEVC_ASSIST_SCRATCH_B
/**/

#define AVS3_SBAC_TOP_BUFFER      HEVC_ASSIST_SCRATCH_C
#define HEVC_SCALELUT             HEVC_ASSIST_SCRATCH_D
#define HEVC_WAIT_FLAG            HEVC_ASSIST_SCRATCH_E
#define RPM_CMD_REG               HEVC_ASSIST_SCRATCH_F

#define LMEM_DUMP_ADR             HEVC_ASSIST_SCRATCH_9
#define HEVC_STREAM_SWAP_TEST     HEVC_ASSIST_SCRATCH_L
#define HEVC_EFFICIENCY_MODE      HEVC_ASSIST_SCRATCH_L
/*!!!*/
#define AVS3_CUVA_ADR       HEVC_ASSIST_SCRATCH_M
#define AVS3_CUVA_DATA_SIZE		HEVC_ASSIST_SCRATCH_N
#define HEVC_DECODE_PIC_BEGIN     HEVC_ASSIST_SCRATCH_M
#define DEBUG_REG1              HEVC_ASSIST_SCRATCH_G
#define DEBUG_REG2              HEVC_ASSIST_SCRATCH_H

#if (defined DEBUG_UCODE_LOG) || (defined DEBUG_CMD)
#define HEVC_DBG_LOG_ADR       HEVC_ASSIST_SCRATCH_5
#ifdef DEBUG_CMD
#define HEVC_D_ADR               HEVC_ASSIST_SCRATCH_4
#endif
#endif

#ifdef NEW_FRONT_BACK_CODE
#define HEVC_DEC_STATUS_DBE             HEVC_ASSIST_SCRATCH_W
#define PIC_DECODE_COUNT_DBE            HEVC_ASSIST_SCRATCH_X
#define DEBUG_REG1_DBE                  HEVC_ASSIST_SCRATCH_Y
#define DEBUG_REG2_DBE                  HEVC_ASSIST_SCRATCH_Z
#endif
/*
ucode parser/search control
bit 0:  0, header auto parse; 1, header manual parse
bit 1:  0, auto skip for noneseamless stream; 1, no skip
bit [3:2]: valid when bit1 == 0;
0, auto skip nal before first vps/sps/pps/idr;
1, auto skip nal before first vps/sps/pps
2, auto skip nal before first  vps/sps/pps,
	and not decode until the first I slice (with slice address of 0)

3, auto skip before first I slice (nal_type >=16 && nal_type <= 21)
bit [15:4] nal skip count (valid when bit0 == 1 (manual mode) )
bit [16]: for NAL_UNIT_EOS when bit0 is 0:
	0, send SEARCH_DONE to arm ;  1, do not send SEARCH_DONE to arm
bit [17]: for NAL_SEI when bit0 is 0:
	0, do not parse SEI in ucode; 1, parse SEI in ucode
bit [31:20]: used by ucode for debug purpose
*/
#define NAL_SEARCH_CTL            HEVC_ASSIST_SCRATCH_I
	/*DECODE_MODE: set before start decoder
		bit 7~0: decode mode
		bit 23~16: start_decoding_flag
			bit [0]   - SEQ_ready
			bit [2:1] - I Picture Count
		bit 31~24: chip feature
	*/
#define DECODE_MODE              HEVC_ASSIST_SCRATCH_J
#define DECODE_STOP_POS         HEVC_ASSIST_SCRATCH_K
	/*read only*/
#define CUR_NAL_UNIT_TYPE       HEVC_ASSIST_SCRATCH_J

#ifdef NEW_FRONT_BACK_CODE
#define HEVC_DEC_STATUS_DBE             HEVC_ASSIST_SCRATCH_W
#define PIC_DECODE_COUNT_DBE            HEVC_ASSIST_SCRATCH_X
#define DEBUG_REG1_DBE                  HEVC_ASSIST_SCRATCH_Y
#define DEBUG_REG2_DBE                  HEVC_ASSIST_SCRATCH_Z

#define EE_ASSIST_MBOX0_IRQ_REG    0x3f70
#define EE_ASSIST_MBOX0_CLR_REG    0x3f71
#define EE_ASSIST_MBOX0_MASK       0x3f72
#endif

#define RPM_BUF_SIZE (0x600 * 2)
#define LMEM_BUF_SIZE (0x600 * 2)

	/*mmu_vbh buf is used by HEVC_SAO_MMU_VH0_ADDR, HEVC_SAO_MMU_VH1_ADDR*/
#define VBH_BUF_SIZE_1080P 0x3000
#define VBH_BUF_SIZE_4K 0x5000
#define VBH_BUF_SIZE_8K 0xa000
#define VBH_BUF_SIZE(bufspec) (bufspec->mmu_vbh.buf_size / 2)
	/*mmu_vbh_dw buf is used by HEVC_SAO_MMU_VH0_ADDR2,HEVC_SAO_MMU_VH1_ADDR2,
		HEVC_DW_VH0_ADDDR, HEVC_DW_VH1_ADDDR*/
#define DW_VBH_BUF_SIZE_1080P (VBH_BUF_SIZE_1080P * 2)
#define DW_VBH_BUF_SIZE_4K (VBH_BUF_SIZE_4K * 2)
#define DW_VBH_BUF_SIZE_8K (VBH_BUF_SIZE_8K * 2)
#define DW_VBH_BUF_SIZE(bufspec) (bufspec->mmu_vbh_dw.buf_size / 4)

/* necessary 4K page size align for t7/t3 decoder and after */
#define WORKBUF_ALIGN(addr) (ALIGN(addr, PAGE_SIZE))

#define WORK_BUF_SPEC_NUM 6
static struct BuffInfo_s amvavs3_workbuff_spec[WORK_BUF_SPEC_NUM] = {
	{
		/* 8M bytes */
		.max_width = 1920,
		.max_height = 1088,
		.ipp = {
			/* IPP work space calculation :
				4096 * (Y+CbCr+Flags) = 12k, round to 16k */
			.buf_size = 0x1e00,
		},
#ifdef NEW_FRONT_BACK_CODE
		.ipp1 = {
		// IPP work space calculation : 4096 * (Y+CbCr+Flags) = 12k, round to 16k
		.buf_size = 0x1e00,
		},
#endif
		.sao_abv = {
			.buf_size = 0,
		},
		.sao_vb = {
			.buf_size = 0,
		},
		.short_term_rps = {
			/* SHORT_TERM_RPS - Max 64 set, 16 entry every set,
				total 64x16x2 = 2048 bytes (0x800) */
			.buf_size = 0x800,
		},
		.rcs = {
			/* RCS STORE AREA - Max 32 RCS, each has 32 bytes,
				total 0x0400 bytes */
			.buf_size = 0x400,
		},
		.sps = {
			/* SPS STORE AREA - Max 16 SPS, each has 0x80 bytes,
			total 0x0800 bytes*/
			.buf_size = 0x800,
		},
		.pps = {
			/*PPS STORE AREA - Max 64 PPS, each has 0x80 bytes,
			total 0x2000 bytes*/
			.buf_size = 0x2000,
		},
		.sbac_top = {
		// DAALA TOP STORE AREA - 224 Bytes (use 256 Bytes for LPDDR4) per 128. Total 4096/128*256 = 0x2000
		.buf_size = 0x2000,
		},
		.sao_up = {
			/* SAO UP STORE AREA - Max 640(10240/16) LCU,
				each has 16 bytes total 0x2800 bytes */
			.buf_size = 0,
		},
		.swap_buf = {
			/* 256cyclex64bit = 2K bytes 0x800
				(only 144 cycles valid) */
			.buf_size = 0x800,
		},
		.swap_buf2 = {
			.buf_size = 0x800,
		},
		.scalelut = {
			/* support up to 32 SCALELUT 1024x32 =
				32Kbytes (0x8000) */
			.buf_size = 0,
		},
		.dblk_para = {
			/* DBLK -> Max 256(4096/16) LCU,
			each para 1024bytes(total:0x40000),
			data 1024bytes(total:0x40000)*/
			.buf_size = 0x3d00, //0x3c80,
		},
		.dblk_data = {
			.buf_size = 0x62800,
		},
		.dblk_data2 = {
			.buf_size = 0x62800,
		},
#ifdef AVS3_10B_MMU
		.mmu_vbh = {
			.buf_size = VBH_BUF_SIZE_1080P, /*2*16*(more than 2304)/4, 4K*/
		},
#if 0
		.cm_header = {
			/*add one for keeper.*/
			.buf_size = MMU_COMPRESS_HEADER_SIZE *
						(FRAME_BUFFERS + 1),
			/* 0x44000 = ((1088*2*1024*4)/32/4)*(32/8) */
		},
#endif
#endif
#ifdef AVS3_10B_MMU_DW
		.mmu_vbh_dw = {
			.buf_size = DW_VBH_BUF_SIZE_1080P, //2*16*2304/4, 4K
		},
#if 0
		.cm_header_dw = {
			.buf_size = MMU_COMPRESS_HEADER_SIZE_DW*17, // 0x44000 = ((1088*2*1024*4)/32/4)*(32/8)
		},
#endif
#endif
		.mpred_above = {
			.buf_size = 0x1e00, /* 2 * size of hevc*/
		},
#ifdef MV_USE_FIXED_BUF
		.mpred_mv = {/* 1080p, 0x40000 per buffer */
			.buf_size = CO_MV_BUF_SIZE_1080P * FRAME_BUFFERS,
		},
#endif
		.rpm = {
			.buf_size = RPM_BUF_SIZE,
		},
		.lmem = {
			.buf_size = 0x600 * 2,
		}
	},
	{
		.max_width = 4096,
		.max_height = 2304,
		.ipp = {
			/* IPP work space calculation :
				4096 * (Y+CbCr+Flags) = 12k, round to 16k */
			.buf_size = 0x4000,
		},
#ifdef NEW_FRONT_BACK_CODE
		.ipp1 = {
		// IPP work space calculation : 4096 * (Y+CbCr+Flags) = 12k, round to 16k
		.buf_size = 0x4000,
		},
#endif
		.sao_abv = {
			.buf_size = 0,
		},
		.sao_vb = {
			.buf_size = 0,
		},
		.short_term_rps = {
			/* SHORT_TERM_RPS - Max 64 set, 16 entry every set,
				total 64x16x2 = 2048 bytes (0x800) */
			.buf_size = 0x800,
		},
		.rcs = {
			/* RCS STORE AREA - Max 16 RCS, each has 32 bytes,
			total 0x0400 bytes */
		// RLP STORE AREA - Max 64 L0 + 64 L1 RLP, each has 32 bytes, total 0x1000 bytes
		.buf_size = 0x1000,
		},
		.sps = {
			/* SPS STORE AREA - Max 16 SPS, each has 0x80 bytes,
				total 0x0800 bytes */
			.buf_size = 0x800,
		},
		.pps = {
			/* PPS STORE AREA - Max 64 PPS, each has 0x80 bytes,
				total 0x2000 bytes */
			.buf_size = 0x2000,
		},
		.sbac_top = {
		// DAALA TOP STORE AREA - 224 Bytes (use 256 Bytes for LPDDR4) per 128. Total 4096/128*256 = 0x2000
		.buf_size = 0x2000,
		},
		.sao_up = {
			/* SAO UP STORE AREA - Max 640(10240/16) LCU,
				each has 16 bytes total 0x2800 bytes */
			.buf_size = 0,
		},
		.swap_buf = {
			/* 256cyclex64bit = 2K bytes 0x800
				(only 144 cycles valid) */
			.buf_size = 0x800,
		},
		.swap_buf2 = {
			.buf_size = 0x800,
		},
		.scalelut = {
			/* support up to 32 SCALELUT 1024x32 = 32Kbytes
				(0x8000) */
			.buf_size = 0,
		},
		.dblk_para  = { .buf_size = 0x40000, },
		.dblk_data  = { .buf_size = 0x80000, },
		.dblk_data2 = { .buf_size = 0x80000, },
#ifdef AVS3_10B_MMU
		.mmu_vbh = {
			.buf_size = VBH_BUF_SIZE_4K,/*2*16*(more than 2304)/4, 4K*/
		},
#if 0
		.cm_header = {
			/*add one for keeper.*/
			.buf_size = MMU_COMPRESS_HEADER_SIZE *
						(FRAME_BUFFERS + 1),
			/* 0x44000 = ((1088*2*1024*4)/32/4)*(32/8) */
		},
#endif
#endif
#ifdef AVS3_10B_MMU_DW
		.mmu_vbh_dw = {
			.buf_size = DW_VBH_BUF_SIZE_4K, //2*16*2304/4, 4K
		},
#if 0
		.cm_header_dw = {
			.buf_size = MMU_COMPRESS_HEADER_SIZE_DW*17, // 0x44000 = ((1088*2*1024*4)/32/4)*(32/8)
		},
#endif
#endif
		.mpred_above = {
			.buf_size = 0x8000, /* 2 * size of hevc*/
		},
#ifdef MV_USE_FIXED_BUF
		.mpred_mv = {
			/* .buf_size = 0x100000*16,
			//4k2k , 0x100000 per buffer */
			/* 4096x2304 , 0x120000 per buffer */
			.buf_size = CO_MV_BUF_SIZE_4K * FRAME_BUFFERS,
		},
#endif
		.rpm = {
			.buf_size = RPM_BUF_SIZE,
		},
		.lmem = {
			.buf_size = 0x600 * 2,
		}
	},
	{
		.max_width = 4096 * 2,
		.max_height = 2304 * 2,
		.ipp = {
		/*IPP work space calculation : 4096 * (Y+CbCr+Flags) = 12k,
		round to 16k*/
			.buf_size = 0x4000 * 2,
		},
#ifdef NEW_FRONT_BACK_CODE
		.ipp1 = {
		// IPP work space calculation : 4096 * (Y+CbCr+Flags) = 12k, round to 16k
		.buf_size = 0x4000 * 2,
		},
#endif
#ifdef USE_SIM_BUFSPEC
		.sao_abv = {
		.buf_size = 0x30000*2,
		},
		.sao_vb = {
		.buf_size = 0x30000*2,
		},

#else
		.sao_abv = {
			.buf_size = 0,
		},
		.sao_vb = {
			.buf_size = 0,
		},
#endif
		.short_term_rps = {
		/*SHORT_TERM_RPS - Max 64 set, 16 entry every set,
			total 64x16x2 = 2048 bytes (0x800)*/
			.buf_size = 0x800,
		},
		.rcs = {
		/*RCS STORE AREA - Max 16 RCS, each has 32 bytes,
		total 0x0400 bytes*/
		// RLP STORE AREA - Max 64 L0 + 64 L1 RLP, each has 32 bytes, total 0x1000 bytes
		.buf_size = 0x1000,
		},
		.sps = {
		/*SPS STORE AREA - Max 16 SPS, each has 0x80 bytes,
			total 0x0800 bytes*/
			.buf_size = 0x800,
		},
		.pps = {
		/*PPS STORE AREA - Max 64 PPS, each has 0x80 bytes, total
			0x2000 bytes*/
			.buf_size = 0x2000,
		},
		.sbac_top = {
		// DAALA TOP STORE AREA - 224 Bytes (use 256 Bytes for LPDDR4) per 128. Total 4096/128*256 = 0x2000
		.buf_size = 0x2000 * 2,
		},
#ifdef USE_SIM_BUFSPEC
		.sao_up = {
		// SAO UP STORE AREA - Max 640(10240/16) LCU, each has 16 bytes total 0x2800 bytes
		.buf_size = 0x2800*2,
		},
#else
		.sao_up = {
		/*SAO UP STORE AREA - Max 640(10240/16) LCU, each has 16 bytes i
				total 0x2800 bytes*/
			.buf_size = 0,
		},
#endif
		.swap_buf = {
		/*256cyclex64bit = 2K bytes 0x800 (only 144 cycles valid)*/
			.buf_size = 0x800,
		},
		.swap_buf2 = {
			.buf_size = 0x800,
		},
#ifdef USE_SIM_BUFSPEC
		.scalelut = {
		// support up to 32 SCALELUT 1024x32 = 32Kbytes (0x8000)
		.buf_size = 0x8000*2,
		},
#else
		.scalelut = {
		/*support up to 32 SCALELUT 1024x32 = 32Kbytes (0x8000)*/
			.buf_size = 0,
		},
#endif
		.dblk_para  = { .buf_size = 0x40000*2, },
		.dblk_data  = { .buf_size = 0x80000*2, },
		.dblk_data2 = { .buf_size = 0x80000*2, },
#ifdef AVS3_10B_MMU
		.mmu_vbh = {
			.buf_size = VBH_BUF_SIZE_8K, /*2*16*2304/4, 4K*/
		},
#if 0
		.cm_header = {
			/*0x44000 = ((1088*2*1024*4)/32/4)*(32/8)*/
			.buf_size = MMU_COMPRESS_8K_HEADER_SIZE * 17,
		},
#endif
#endif
#ifdef AVS3_10B_MMU_DW
		.mmu_vbh_dw = {
			.buf_size = DW_VBH_BUF_SIZE_8K, //2*16*2304/4, 4K
		},
#if 0
		.cm_header_dw = {
			.buf_size = MMU_COMPRESS_HEADER_SIZE_DW*17, // 0x44000 = ((1088*2*1024*4)/32/4)*(32/8)
		},
#endif
#endif
		.mpred_above = {
			.buf_size = 0x8000*2,
		},
#ifdef MV_USE_FIXED_BUF
		.mpred_mv = {
			/*4k2k , 0x100000 per buffer*/
			.buf_size = CO_MV_BUF_SIZE_8K * FRAME_BUFFERS,
		},
#endif
		.rpm = {
			.buf_size = RPM_BUF_SIZE,
		},
		.lmem = {
			.buf_size = 0x600 * 2,
		}
	},

	//large buf size for debugging only
	{
		.max_width = 4096 * 2,
		.max_height = 2304 * 2,
		.ipp = {
		/*IPP work space calculation : 4096 * (Y+CbCr+Flags) = 12k,
		round to 16k*/
			.buf_size = 0x4000 * 4,
		},
#ifdef NEW_FRONT_BACK_CODE
		.ipp1 = {
		// IPP work space calculation : 4096 * (Y+CbCr+Flags) = 12k, round to 16k
			.buf_size = 0x4000 * 4,
		},
#endif
#ifdef USE_SIM_BUFSPEC
		.sao_abv = {
			.buf_size = 0x30000*4,
		},
		.sao_vb = {
			.buf_size = 0x30000*4,
		},
#else
		.sao_abv = {
			.buf_size = 0,
		},
		.sao_vb = {
			.buf_size = 0,
		},
#endif
		.short_term_rps = {
		/*SHORT_TERM_RPS - Max 64 set, 16 entry every set,
			total 64x16x2 = 2048 bytes (0x800)*/
			.buf_size = 0x800*4
		},
		.rcs = {
		/*RCS STORE AREA - Max 16 RCS, each has 32 bytes,
		total 0x0400 bytes*/
		// RLP STORE AREA - Max 64 L0 + 64 L1 RLP, each has 32 bytes, total 0x1000 bytes
			.buf_size = 0x1000*4,
		},
		.sps = {
		/*SPS STORE AREA - Max 16 SPS, each has 0x80 bytes,
			total 0x0800 bytes*/
			.buf_size = 0x800*4,
		},
		.pps = {
		/*PPS STORE AREA - Max 64 PPS, each has 0x80 bytes, total
			0x2000 bytes*/
			.buf_size = 0x2000*2,
		},
		.sbac_top = {
			// DAALA TOP STORE AREA - 224 Bytes (use 256 Bytes for LPDDR4) per 128. Total 4096/128*256 = 0x2000
			.buf_size = 0x2000 * 4,
		},
#ifdef USE_SIM_BUFSPEC
		.sao_up = {
			// SAO UP STORE AREA - Max 640(10240/16) LCU, each has 16 bytes total 0x2800 bytes
			.buf_size = 0x2800*4,
		},
#else
		.sao_up = {
		/*SAO UP STORE AREA - Max 640(10240/16) LCU, each has 16 bytes i
				total 0x2800 bytes*/
			.buf_size = 0,
		},
#endif
		.swap_buf = {
		/*256cyclex64bit = 2K bytes 0x800 (only 144 cycles valid)*/
			.buf_size = 0x800*4,
		},
		.swap_buf2 = {
			.buf_size = 0x800*4,
		},
#ifdef USE_SIM_BUFSPEC
		.scalelut = {
		// support up to 32 SCALELUT 1024x32 = 32Kbytes (0x8000)
			.buf_size = 0x8000*2,
		},
#else
		.scalelut = {
		/*support up to 32 SCALELUT 1024x32 = 32Kbytes (0x8000)*/
			.buf_size = 0,
		},
#endif
		.dblk_para  = { .buf_size = 0x40000*4, },
		.dblk_data  = { .buf_size = 0x80000*4, },
		.dblk_data2 = { .buf_size = 0x80000*4, },
#ifdef AVS3_10B_MMU
		.mmu_vbh = {
			.buf_size = VBH_BUF_SIZE_8K*2, /*2*16*2304/4, 4K*/
		},
#if 0
		.cm_header = {
			/*0x44000 = ((1088*2*1024*4)/32/4)*(32/8)*/
			.buf_size = MMU_COMPRESS_8K_HEADER_SIZE * 17,
		},
#endif
#endif
#ifdef AVS3_10B_MMU_DW
		.mmu_vbh_dw = {
			.buf_size = DW_VBH_BUF_SIZE_8K*2, //2*16*2304/4, 4K
		},
#if 0
		.cm_header_dw = {
			.buf_size = MMU_COMPRESS_HEADER_SIZE_DW*17, // 0x44000 = ((1088*2*1024*4)/32/4)*(32/8)
		},
#endif
#endif
		.mpred_above = {
			.buf_size = 0x8000*4,
		},
#ifdef MV_USE_FIXED_BUF
		.mpred_mv = {
			/*4k2k , 0x100000 per buffer*/
			.buf_size = CO_MV_BUF_SIZE_8K * FRAME_BUFFERS,
		},
#endif
		.rpm = {
			.buf_size = RPM_BUF_SIZE,
		},
		.lmem = {
			.buf_size = 0x600 * 2,
		}
	}

};

#define IS_8K_SIZE(w, h)  (((w) * (h)) > MAX_SIZE_4K)
#define IS_4K_SIZE(w, h)  (((w) * (h)) > (1920*1088))

static u32 get_mv_mem_unit(int lcu_size_log2)
{
	u32 mv_mem_unit;
	//lcu_size_log2 == 6 ? 0x200 : lcu_size_log2 == 5 ? 0x80 : 0x20; //avs2
	//mv_mem_unit = lcu_size_log2 == 7 ? 256 : lcu_size_log2 == 6 ? 64 : 16;
	mv_mem_unit = lcu_size_log2 == 7 ? 512 : lcu_size_log2 == 6 ? 128 : 32;
	return mv_mem_unit;
}

#ifndef MV_USE_FIXED_BUF

static uint32_t get_mv_buf_size(struct AVS3Decoder_s *dec, int width, int height) {
	struct avs3_decoder *avs3_dec = &dec->avs3_dec;
	uint32_t size;
	if (mv_buf_dynamic_alloc == 1) {
		int mv_mem_unit =
			get_mv_mem_unit(avs3_dec->lcu_size_log2);

			int32_t pic_width = avs3_dec->img.width;
			int32_t pic_height = avs3_dec->img.height;
		int32_t lcu_size = 1<<avs3_dec->lcu_size_log2;
		int32_t pic_width_lcu  = ( pic_width %lcu_size  ) ? pic_width /lcu_size  + 1 : pic_width /lcu_size;
		int32_t pic_height_lcu = ( pic_height %lcu_size ) ? pic_height/lcu_size + 1 : pic_height/lcu_size;
		int32_t lcu_total       =pic_width_lcu*pic_height_lcu;
		int new_size =  lcu_total * mv_mem_unit;
		size = (new_size + 0xffff) & (~0xffff);

	} else {
		if (IS_8K_SIZE(width, height))
			size = CO_MV_BUF_SIZE_8K;
		else if (IS_4K_SIZE(width, height))
			size = CO_MV_BUF_SIZE_4K;
		else
			size = CO_MV_BUF_SIZE_1080P;
	}
	return size;
}
#endif

static void config_hevc_irq_num(struct AVS3Decoder_s *dec)
{
#ifdef NEW_FB_CODE
	if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_S5) {
		dec->ASSIST_MBOX0_IRQ_REG = EE_ASSIST_MBOX0_IRQ_REG;
		dec->ASSIST_MBOX0_CLR_REG = EE_ASSIST_MBOX0_CLR_REG;
		dec->ASSIST_MBOX0_MASK    = EE_ASSIST_MBOX0_MASK;
		dec->backend_ASSIST_MBOX0_IRQ_REG = HEVC_ASSIST_MBOX0_IRQ_REG;
		dec->backend_ASSIST_MBOX0_CLR_REG = HEVC_ASSIST_MBOX0_CLR_REG;
		dec->backend_ASSIST_MBOX0_MASK    = HEVC_ASSIST_MBOX0_MASK;
	} else {
		dec->ASSIST_MBOX0_IRQ_REG = HEVC_ASSIST_MBOX0_IRQ_REG;
		dec->ASSIST_MBOX0_CLR_REG = HEVC_ASSIST_MBOX0_CLR_REG;
		dec->ASSIST_MBOX0_MASK    = HEVC_ASSIST_MBOX0_MASK;
		dec->backend_ASSIST_MBOX0_IRQ_REG = EE_ASSIST_MBOX0_IRQ_REG;
		dec->backend_ASSIST_MBOX0_CLR_REG = EE_ASSIST_MBOX0_CLR_REG;
		dec->backend_ASSIST_MBOX0_MASK    = EE_ASSIST_MBOX0_MASK;
	}
#else
	dec->ASSIST_MBOX0_IRQ_REG = HEVC_ASSIST_MBOX0_IRQ_REG;
	dec->ASSIST_MBOX0_CLR_REG = HEVC_ASSIST_MBOX0_CLR_REG;
	dec->ASSIST_MBOX0_MASK    = HEVC_ASSIST_MBOX0_MASK;
#endif
}

#ifdef NEW_FB_CODE
#include "avs3_fb_hw.c"

void pic_backend_ref_operation(struct AVS3Decoder_s *dec, avs3_frame_t *pic, bool add_flag)
{
	struct avs3_decoder *avs3_dec = &dec->avs3_dec;
	avs3_frame_t *ref_pic = NULL;
	int i = 0;

	mutex_lock(&dec->fb_mutex);
	if (add_flag) {
		pic->backend_ref = 1;
		pic->back_done_mark = 0;
	} else {
		pic->backend_ref--;
		if (pic->backend_ref < 0) {
			pic->backend_ref = 0;
			avs3_print(dec, PRINT_FLAG_VDEC_DETAIL, "%s:pic(%px) backend_ref error\n",
				__func__, pic);
		}
		pic->back_done_mark = 1;
	}

	for (i = 0; i < pic->list0_num_refp; i++) {
		ref_pic = &avs3_dec->pic_pool[pic->list0_index[i]].buf_cfg;
		if (add_flag) {
			ref_pic->backend_ref++;
		} else {
			ref_pic->backend_ref--;
			if (ref_pic->backend_ref < 0) {
				ref_pic->backend_ref = 0;
				avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL, "%s:L0 ref_pic %d backend_ref error\n",
					__func__, pic->list0_index[i]);
			}
		}
	}

	for (i = 0; i < pic->list1_num_refp; i++) {
		ref_pic = &avs3_dec->pic_pool[pic->list1_index[i]].buf_cfg;
		if (add_flag) {
			ref_pic->backend_ref++;
		} else {
			ref_pic->backend_ref--;
			if (ref_pic->backend_ref < 0) {
				ref_pic->backend_ref = 0;
				avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL, "%s:L1 ref_pic %d backend_ref error\n",
					__func__, pic->list1_index[i]);
			}
		}
	}

	mutex_unlock(&dec->fb_mutex);
}

/*simulation code: if (dec_status == HEVC_DECPIC_DATA_DONE) {*/
static int front_decpic_done_update(struct AVS3Decoder_s *dec, uint8_t reset_flag)
{
	struct avs3_decoder *avs3_dec = &dec->avs3_dec;
	avs3_frame_t *cur_pic = avs3_dec->cur_pic;
	int j;
	if (debug & AVS3_DBG_PRINT_PIC_LIST) {
		print_pic_pool(avs3_dec, "before inc backend_ref");

		avs3_print_cont(dec, 0, "pic_cfg index %d, list0 index ", cur_pic->index);
		for (j = 0; j < cur_pic->list0_num_refp; j++)
			avs3_print_cont(dec, 0, "%d ", cur_pic->list0_index[j]);
		avs3_print_cont(dec, 0, "list1 index ");
		for (j = 0; j < cur_pic->list1_num_refp; j++)
			avs3_print_cont(dec, 0, "%d ", cur_pic->list1_index[j]);
		avs3_print_cont(dec, 0, "\n");
	}

	pic_backend_ref_operation(dec, cur_pic, 1);

	if (debug & AVS3_DBG_PRINT_PIC_LIST)
		print_pic_pool(avs3_dec, "after inc backend_ref");

	if (dec->front_back_mode == 1) {
		if (fbdebug_flag & 0x70000000) {
			u8 cmd = (fbdebug_flag >> 28) & 0x7;
			u32 dump_c = (fbdebug_flag >> 16) & 0xfff;
			if ((cmd == 3) ||
				(cmd == 1 && dump_c == avs3_dec->frontend_decoded_count) ||
				(cmd == 2 && avs3_dec->frontend_decoded_count < dump_c))
				dump_loop_buffer(dec, avs3_dec->frontend_decoded_count, cmd ==1 || cmd == 2);
		}

		if (fbdebug_flag & 0x4) {
			copy_loopbufs_ptr(&avs3_dec->fr, &avs3_dec->init_fr);
			print_loopbufs_ptr(dec, "fr", &avs3_dec->fr);
		} else {
			copy_loopbufs_ptr(&avs3_dec->p_fr, &avs3_dec->fr);
			read_bufstate_front(avs3_dec);
			avs3_print(dec, PRINT_FLAG_VDEC_DETAIL,
				"update fr from loop buf register\n");

			print_loopbufs_ptr2(dec, "fr", &avs3_dec->p_fr, &avs3_dec->fr);
		}

		WRITE_VREG(HEVC_ASSIST_FB_PIC_CLR, 1);

		//WRITE_VREG(HEVC_DEC_STATUS_REG, AVS3_DEC_IDLE);
	} else {
		if (reset_flag) /*not multi pictures in one packe*/
			amhevc_stop();
	}

	avs3_dec->frontend_decoded_count++;
	avs3_dec->next_be_decode_pic[avs3_dec->fb_wr_pos] = avs3_dec->cur_pic;

	mutex_lock(&dec->fb_mutex);
	avs3_dec->fb_wr_pos++;
	if (avs3_dec->fb_wr_pos >= dec->fb_ifbuf_num)
		avs3_dec->fb_wr_pos = 0;

	if (avs3_dec->fb_wr_pos == avs3_dec->fb_rd_pos)
		avs3_dec->wait_working_buf = 1;

	avs3_print(dec, PRINT_FLAG_VDEC_DETAIL,
		"fb_wr_pos %d, fb_rd_pos %d, wait_working_buf %d\n",
		avs3_dec->fb_wr_pos, avs3_dec->fb_rd_pos, avs3_dec->wait_working_buf);

	mutex_unlock(&dec->fb_mutex);

	return 0;
}

#endif

/*Losless compression body buffer size 4K per 64x32 (jt)*/
static int  compute_losless_comp_body_size(struct AVS3Decoder_s *dec,
	int width, int height,
	uint8_t is_bit_depth_10)
{
	int     width_x64;
	int     height_x32;
	int     bsize;
	width_x64 = width + 63;
	width_x64 >>= 6;
	height_x32 = height + 31;
	height_x32 >>= 5;
		bsize = (is_bit_depth_10 ? 4096 : 3200)
		* width_x64 * height_x32;

	avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
		"%s(%d,%d,%d)=>%d\n",
		__func__, width, height,
		is_bit_depth_10, bsize);

	return  bsize;
}

static int avs3_mmu_page_num(struct AVS3Decoder_s *dec,
	int pic_width, int pic_height, int is_bit_depth_10)
{
	int picture_size;
	int cur_mmu_4k_number, max_frame_num;

	picture_size =
		compute_losless_comp_body_size(dec, pic_width, pic_height, is_bit_depth_10);
	cur_mmu_4k_number = ((picture_size + (1 << 12) - 1) >> 12);

	if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_SM1)
		max_frame_num = MAX_FRAME_8K_NUM;
	else
		max_frame_num = MAX_FRAME_4K_NUM;

	if (cur_mmu_4k_number > max_frame_num) {
		avs3_print(dec, 0, "over max !! 0x%x width %d height %d\n",
			max_frame_num, pic_width, pic_height);
		return -1;
	}
	return cur_mmu_4k_number;
}

/* Losless compression header buffer size 32bytes per 128x64 (jt)*/
static  int  compute_losless_comp_header_size(struct AVS3Decoder_s *dec,
	int width, int height)
{
	int     width_x128;
	int     height_x64;
	int     hsize;
	width_x128 = width + 127;
	width_x128 >>= 7;
	height_x64 = height + 63;
	height_x64 >>= 6;

	hsize = 32 * width_x128 * height_x64;
	avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
		"%s(%d,%d)=>%d\n",
		__func__, width, height,
		hsize);

	return  hsize;
}

static void init_buff_spec(struct AVS3Decoder_s *dec,
	struct BuffInfo_s *buf_spec)
{
	void *mem_start_virt;
	buf_spec->ipp.buf_start =
		WORKBUF_ALIGN(buf_spec->start_adr);
#ifdef NEW_FRONT_BACK_CODE
	buf_spec->ipp1.buf_start =
		WORKBUF_ALIGN(buf_spec->ipp.buf_start + buf_spec->ipp.buf_size);
	buf_spec->sao_abv.buf_start =
		WORKBUF_ALIGN(buf_spec->ipp1.buf_start + buf_spec->ipp1.buf_size);
#else
	buf_spec->sao_abv.buf_start =
		WORKBUF_ALIGN(buf_spec->ipp.buf_start + buf_spec->ipp.buf_size);
#endif
	buf_spec->sao_vb.buf_start =
		WORKBUF_ALIGN(buf_spec->sao_abv.buf_start + buf_spec->sao_abv.buf_size);
	buf_spec->short_term_rps.buf_start =
		WORKBUF_ALIGN(buf_spec->sao_vb.buf_start + buf_spec->sao_vb.buf_size);
	buf_spec->rcs.buf_start =
		WORKBUF_ALIGN(buf_spec->short_term_rps.buf_start + buf_spec->short_term_rps.buf_size);
	buf_spec->sps.buf_start =
		WORKBUF_ALIGN(buf_spec->rcs.buf_start + buf_spec->rcs.buf_size);
	buf_spec->pps.buf_start =
		WORKBUF_ALIGN(buf_spec->sps.buf_start + buf_spec->sps.buf_size);
	buf_spec->sbac_top.buf_start =
		WORKBUF_ALIGN(buf_spec->pps.buf_start + buf_spec->pps.buf_size);
	buf_spec->sao_up.buf_start =
		WORKBUF_ALIGN(buf_spec->sbac_top.buf_start + buf_spec->sbac_top.buf_size);
	buf_spec->swap_buf.buf_start =
		WORKBUF_ALIGN(buf_spec->sao_up.buf_start + buf_spec->sao_up.buf_size);
	buf_spec->swap_buf2.buf_start =
		WORKBUF_ALIGN(buf_spec->swap_buf.buf_start + buf_spec->swap_buf.buf_size);
	buf_spec->scalelut.buf_start =
		WORKBUF_ALIGN(buf_spec->swap_buf2.buf_start + buf_spec->swap_buf2.buf_size);
	buf_spec->dblk_para.buf_start =
		WORKBUF_ALIGN(buf_spec->scalelut.buf_start + buf_spec->scalelut.buf_size);
	buf_spec->dblk_data.buf_start =
		WORKBUF_ALIGN(buf_spec->dblk_para.buf_start + buf_spec->dblk_para.buf_size);
	buf_spec->dblk_data2.buf_start =
		WORKBUF_ALIGN(buf_spec->dblk_data.buf_start + buf_spec->dblk_data.buf_size);
#ifdef AVS3_10B_MMU
	buf_spec->mmu_vbh.buf_start  =
		WORKBUF_ALIGN(buf_spec->dblk_data2.buf_start + buf_spec->dblk_data2.buf_size);
	#ifdef AVS3_10B_MMU_DW
	buf_spec->mmu_vbh_dw.buf_start =
		WORKBUF_ALIGN(buf_spec->mmu_vbh.buf_start + buf_spec->mmu_vbh.buf_size);
	buf_spec->mpred_above.buf_start =
		WORKBUF_ALIGN(buf_spec->mmu_vbh_dw.buf_start + buf_spec->mmu_vbh_dw.buf_size);
	#else
	buf_spec->mpred_above.buf_start =
		WORKBUF_ALIGN(buf_spec->mmu_vbh.buf_start + buf_spec->mmu_vbh.buf_size);
	#endif
#else /* AVS3_10B_MMU */
	#ifdef AVS3_10B_MMU_DW
	buf_spec->mmu_vbh_dw.buf_start =
		WORKBUF_ALIGN(buf_spec->dblk_data2.buf_start + buf_spec->dblk_data2.buf_size);
	buf_spec->mpred_above.buf_start =
		WORKBUF_ALIGN(buf_spec->mmu_vbh_dw.buf_start + buf_spec->mmu_vbh_dw.buf_size);
	#else
	buf_spec->mpred_above.buf_start =
		WORKBUF_ALIGN(buf_spec->dblk_data2.buf_start + buf_spec->dblk_data2.buf_size);
	#endif
#endif /* AVS3_10B_MMU */
#ifdef MV_USE_FIXED_BUF
	buf_spec->mpred_mv.buf_start =
		WORKBUF_ALIGN(buf_spec->mpred_above.buf_start + buf_spec->mpred_above.buf_size);
	buf_spec->rpm.buf_start =
		WORKBUF_ALIGN(buf_spec->mpred_mv.buf_start + buf_spec->mpred_mv.buf_size);
#else
	buf_spec->rpm.buf_start =
		WORKBUF_ALIGN(buf_spec->mpred_above.buf_start + buf_spec->mpred_above.buf_size);
#endif
	buf_spec->lmem.buf_start =
		WORKBUF_ALIGN(buf_spec->rpm.buf_start + buf_spec->rpm.buf_size);
	buf_spec->end_adr =
		WORKBUF_ALIGN(buf_spec->lmem.buf_start + buf_spec->lmem.buf_size);

	if (dec) {
		mem_start_virt =
			codec_mm_phys_to_virt(buf_spec->dblk_para.buf_start);
		if (mem_start_virt) {
			memset(mem_start_virt, 0, buf_spec->dblk_para.buf_size);
			codec_mm_dma_flush(mem_start_virt,
				buf_spec->dblk_para.buf_size,
				DMA_TO_DEVICE);
		} else {
			/*not virt for tvp playing,
			may need clear on ucode.*/
			pr_err("mem_start_virt failed\n");
		}
		if (debug) {
			pr_info("%s workspace (%x %x) size = %x\n", __func__,
				   buf_spec->start_adr, buf_spec->end_adr,
				   buf_spec->end_adr - buf_spec->start_adr);
		}
		if (debug) {
			pr_info("ipp.buf_start             :%x\n",
				   buf_spec->ipp.buf_start);
#ifdef NEW_FRONT_BACK_CODE
			pr_info("ipp1.buf_start             :%x\n",
				   buf_spec->ipp1.buf_start);
#endif
			pr_info("sao_abv.buf_start          :%x\n",
				   buf_spec->sao_abv.buf_start);
			pr_info("sao_vb.buf_start          :%x\n",
				   buf_spec->sao_vb.buf_start);
			pr_info("short_term_rps.buf_start  :%x\n",
				   buf_spec->short_term_rps.buf_start);
			pr_info("rcs.buf_start             :%x\n",
				   buf_spec->rcs.buf_start);
			pr_info("sps.buf_start             :%x\n",
				   buf_spec->sps.buf_start);
			pr_info("pps.buf_start             :%x\n",
				   buf_spec->pps.buf_start);
			pr_info("sao_up.buf_start          :%x\n",
				   buf_spec->sao_up.buf_start);
			pr_info("swap_buf.buf_start        :%x\n",
				   buf_spec->swap_buf.buf_start);
			pr_info("swap_buf2.buf_start       :%x\n",
				   buf_spec->swap_buf2.buf_start);
			pr_info("scalelut.buf_start        :%x\n",
				   buf_spec->scalelut.buf_start);
			pr_info("dblk_para.buf_start       :%x\n",
				   buf_spec->dblk_para.buf_start);
			pr_info("dblk_data.buf_start       :%x\n",
				   buf_spec->dblk_data.buf_start);
			pr_info("dblk_data2.buf_start       :%x\n",
				   buf_spec->dblk_data2.buf_start);
	#ifdef AVS3_10B_MMU
			pr_info("mmu_vbh.buf_start     :%x\n",
				buf_spec->mmu_vbh.buf_start);
	#endif
	#ifdef AVS3_10B_MMU_DW
			pr_info("mmu_vbh_dw.buf_start     :%x\n",
				buf_spec->mmu_vbh_dw.buf_start);
	#endif
			pr_info("mpred_above.buf_start     :%x\n",
				   buf_spec->mpred_above.buf_start);
#ifdef MV_USE_FIXED_BUF
			pr_info("mpred_mv.buf_start        :%x\n",
				   buf_spec->mpred_mv.buf_start);
#endif
			if ((debug & AVS3_DBG_SEND_PARAM_WITH_REG) == 0) {
				pr_info("rpm.buf_start             :%x\n",
					   buf_spec->rpm.buf_start);
			}
		}
	}

}

static void uninit_mmu_buffers(struct AVS3Decoder_s *dec)
{
#ifdef AVS3_10B_MMU_DW
	if (dec->dw_mmu_enable && dec->dw_mmu_box) {
		decoder_mmu_box_free(dec->dw_mmu_box);
		dec->dw_mmu_box = NULL;
	}
#endif
	decoder_mmu_box_free(dec->mmu_box);
	dec->mmu_box = NULL;
#ifdef NEW_FB_CODE
	if (dec->front_back_mode) {
		if (dec->mmu_box_1)
			decoder_mmu_box_free(dec->mmu_box_1);
		dec->mmu_box_1 = NULL;
		if (dec->dw_mmu_enable && dec->dw_mmu_box_1) {
			decoder_mmu_box_free(dec->dw_mmu_box_1);
			dec->dw_mmu_box_1 = NULL;
		}
	}
	if (dec->front_back_mode) {
		uninit_fb_bufstate(dec);
	}
#endif
	if (dec->bmmu_box)
		decoder_bmmu_box_free(dec->bmmu_box);
	dec->bmmu_box = NULL;
}

static u32 calc_buffer_u_v_h_size(u32 w, u32 h, u32 ratio, u32 lcu_size)
{
	int pic_width_dw = w / ratio;
	int pic_height_dw = h / ratio;

	int pic_width_lcu_dw = (pic_width_dw % lcu_size) ?
		(pic_width_dw / lcu_size + 1)  : (pic_width_dw / lcu_size);
	int pic_height_lcu_dw = (pic_height_dw % lcu_size) ?
		(pic_height_dw / lcu_size + 1) : (pic_height_dw / lcu_size);

	int lcu_total_dw = pic_width_lcu_dw * pic_height_lcu_dw;
	int mc_buffer_size_u_v = (lcu_total_dw * lcu_size * lcu_size) >> 1; // div 2

	return ((mc_buffer_size_u_v + 0xffff) >> 16);
}

static int config_pic(struct AVS3Decoder_s *dec,
				struct avs3_frame_s *pic, int32_t lcu_size_log2)
{
	struct vdec_s *vdec = hw_to_vdec(dec);
	int ret = -1;
	int i;
	/* to do: init_pic_w, init_pic_h*/
#if 0
	int pic_width = dec->init_pic_w;
	int pic_height = dec->init_pic_h;
#else
	//simulation
	int32_t pic_width = dec->avs3_dec.img.width;
	int32_t pic_height = dec->avs3_dec.img.height;
#endif
	/*struct avs3_decoder *avs3_dec = &dec->avs3_dec;
	int32_t lcu_size_log2 = avs3_dec->lcu_size_log2;*/
	int32_t lcu_size = 1 << dec->avs3_dec.lcu_size_log2;
	int32_t pic_width_lcu  = ( pic_width %lcu_size  ) ? pic_width /lcu_size  + 1 : pic_width /lcu_size;
	int32_t pic_height_lcu = ( pic_height %lcu_size ) ? pic_height/lcu_size + 1 : pic_height/lcu_size;
	int32_t lcu_total       =pic_width_lcu*pic_height_lcu;

	u32 y_adr = 0;
	int buf_size = 0;
	int losless_comp_body_size = compute_losless_comp_body_size(
			dec, pic_width,
			pic_height, buf_alloc_depth == 10);

	//int mc_buffer_size_u_v = 0;
	int mc_buffer_size_u_v_h = 0;
	int dw_uv_size;
	int dw_mode = get_double_write_mode_init(dec);
#ifdef OW_TRIPLE_WRITE
	int tw_mode = get_triple_write_mode(dec);
#endif

	if (dw_mode && ((dw_mode & 0x20) == 0)) {
		mc_buffer_size_u_v_h = calc_buffer_u_v_h_size(pic_width,
			pic_height, get_double_write_ratio(dw_mode & 0xf), lcu_size);

		/*64k alignment*/
		buf_size = ((mc_buffer_size_u_v_h << 16) * 3);
		buf_size = ((buf_size + 0xffff) >> 16) << 16;
#ifdef P010_ENABLE
		if (is_dw_p010(dec)) {	//double size mem for p010 mode
			buf_size += ((mc_buffer_size_u_v_h << 16) * 3);
		}
#endif
	}
#ifdef OW_TRIPLE_WRITE
	if (tw_mode) {
		mc_buffer_size_u_v_h = calc_buffer_u_v_h_size(pic_width,
			pic_height, get_double_write_ratio(tw_mode), lcu_size);

		buf_size += ((mc_buffer_size_u_v_h << 16) * 3);
		if (is_tw_p010(dec)) {
			buf_size += ((mc_buffer_size_u_v_h << 16) * 3);
		}
	}
#endif

	if (dec->mmu_enable) {
		pic->header_adr = decoder_bmmu_box_get_phy_addr(
				dec->bmmu_box, HEADER_BUFFER_IDX(pic->index));

#ifdef AVS3_10B_MMU_DW
		if (dec->dw_mmu_enable) {
			pic->dw_header_adr = pic->header_adr
				+ get_compress_header_size(dec);
		}
#endif
	}

	i = pic->index;

	/*if ((dec->mc_buf->buf_start + (i + 1) * buf_size) <
		dec->mc_buf->buf_end)
		y_adr = dec->mc_buf->buf_start + i * buf_size;
	else {*/
	if (buf_size > 0 && pic->cma_alloc_addr == 0) {
		ret = decoder_bmmu_box_alloc_buf_phy(dec->bmmu_box,
				VF_BUFFER_IDX(i),
				buf_size, DRIVER_NAME,
				&pic->cma_alloc_addr);
		if (ret < 0) {
			avs3_print(dec, 0,
				"decoder_bmmu_box_alloc_buf_phy idx %d size %d fail\n",
				VF_BUFFER_IDX(i),
				buf_size
				);
			return ret;
		}

		if (pic->cma_alloc_addr) {
			y_adr = pic->cma_alloc_addr;
			if (!vdec_secure(vdec))
				codec_mm_memset(y_adr, 0, buf_size);

			if (vdec->vdata == NULL ||
				atomic_read(&vdec->vdata->use_flag) == 0) {
				vdec->vdata = vdec_data_get();
			}

			if (vdec->vdata != NULL) {
				int index = 0;
				struct vdec_data_buf_s data_buf;
				data_buf.alloc_policy = ALLOC_AUX_BUF;
				data_buf.aux_buf_size = dec->cuva_size;

				index = vdec_data_get_index((ulong)vdec->vdata, &data_buf);
				if (index >= 0) {
					pic->cuva_data_buf = vdec->vdata->data[index].aux_data_buf;
					vdec_data_buffer_count_increase((ulong)vdec->vdata, index, i);
					INIT_LIST_HEAD(&vdec->vdata->release_callback[i].node);
					decoder_bmmu_box_add_callback_func(dec->bmmu_box, VF_BUFFER_IDX(i), (void *)&vdec->vdata->release_callback[i]);
				} else {
					avs3_print(dec, 0, "vdec data is full\n");
				}
			}
		} else {
			avs3_print(dec, 0,
				"decoder_bmmu_box_alloc_buf_phy idx %d size %d return null\n",
				VF_BUFFER_IDX(i),
				buf_size
				);
			return -1;
		}
	}

	/*ensure get_pic_by_POC()
	not get the buffer not decoded*/
	dw_uv_size = mc_buffer_size_u_v_h << (16 + is_dw_p010(dec));

	pic->BUF_index = i;
	pic->lcu_total = lcu_total;

	pic->comp_body_size = losless_comp_body_size;
	pic->buf_size = buf_size;
	pic->mc_canvas_y = pic->index;
	pic->mc_canvas_u_v = pic->index;
	if (dw_mode) {
		pic->dw_y_adr = y_adr;
		pic->dw_u_v_adr = pic->dw_y_adr + (dw_uv_size << 1);

		pic->mc_y_adr = pic->dw_y_adr;
		pic->mc_u_v_adr = pic->dw_u_v_adr;
	}
	if (tw_mode) {
		if (dw_mode) {
			pic->tw_y_adr = pic->dw_u_v_adr + dw_uv_size;  //base dw buf addr
		} else {
			pic->tw_y_adr = y_adr;	//base no dw buf addr
		}
		pic->tw_u_v_adr = pic->tw_y_adr + (mc_buffer_size_u_v_h << (16 + is_tw_p010(dec) + 1));
	}

#ifdef MV_USE_FIXED_BUF
	pic->mpred_mv_wr_start_addr =
		dec->work_space_buf->mpred_mv.buf_start +
		pic->index * (dec->work_space_buf->mpred_mv.buf_size / FRAME_BUFFERS);
	if (pic->mpred_mv_wr_start_addr >
		(dec->work_space_buf->mpred_mv.buf_start
		+ dec->work_space_buf->mpred_mv.buf_size)) {
		avs3_print(dec, 0, "err: fixed mv buf out of size, 0x0%x\n",
			pic->mpred_mv_wr_start_addr);
		pic->mpred_mv_wr_start_addr =
			dec->work_space_buf->mpred_mv.buf_start;
	}
#endif
	if (debug & AVS3_DBG_BUFMGR) {
		avs3_print(dec, AVS3_DBG_BUFMGR,
			"%s index %d, head_size 0x%08x, MMU header_adr 0x%08x, dw_header_adr 0x%08x; ",
			__func__, pic->index, get_compress_header_size(dec),
			pic->header_adr, pic->dw_header_adr);

		avs3_print_cont(dec, AVS3_DBG_BUFMGR,
			"dw_y_adr 0x%08x, dw_u_v_adr 0x%08x, ",
			pic->dw_y_adr,
			pic->dw_u_v_adr);

		avs3_print_cont(dec, AVS3_DBG_BUFMGR,
			"comp_body_size %x comp_buf_size %x ;",
			pic->comp_body_size,
			pic->buf_size);
		avs3_print_cont(dec, AVS3_DBG_BUFMGR,
			"mpred_mv_wr_start_adr %d ",
			pic->mpred_mv_wr_start_addr);

		avs3_print_cont(dec, AVS3_DBG_BUFMGR,
			"tw y_addr %x, uv_addr %x\n", pic->tw_y_adr, pic->tw_u_v_adr);
		avs3_print_flush(dec);
	}

	ret = 0;
	return ret;
}

static void init_pic_list(struct AVS3Decoder_s *dec,
	int32_t lcu_size_log2)
{
	int i;
	struct avs3_decoder *avs3_dec = &dec->avs3_dec;
	struct avs3_frame_s *pic;
#ifdef AVS3_10B_MMU
	if (dec->mmu_enable) {
		for (i = 0; i < avs3_dec->max_pb_size; i++) {
			unsigned long buf_addr;
			u32 header_size = get_compress_header_size(dec);
#ifdef AVS3_10B_MMU_DW
			if (dec->dw_mmu_enable)
				header_size <<= 1;
#endif
			if (decoder_bmmu_box_alloc_buf_phy
					(dec->bmmu_box,
					HEADER_BUFFER_IDX(i), header_size,
					DRIVER_HEADER_NAME,
					&buf_addr) < 0) {
				avs3_print(dec, 0,
					"%s malloc compress header failed %d\n",
					DRIVER_HEADER_NAME, i);
				dec->fatal_error |= DECODER_FATAL_ERROR_NO_MEM;
				return;
			}
			if (!vdec_secure(hw_to_vdec(dec)))
				codec_mm_memset(buf_addr, 0, header_size);
		}
	}
#endif
	dec->frame_height = avs3_dec->img.height;
	dec->frame_width = avs3_dec->img.width;

	for (i = 0; i < avs3_dec->max_pb_size; i++) {
		pic = &avs3_dec->pic_pool[i].buf_cfg;
		pic->index = i;
		pic->BUF_index = -1;
		//pic->mv_buf_index = -1;
		if (config_pic(dec, pic, lcu_size_log2) < 0) {
			//if (debug)
				avs3_print(dec, 0,
					"Config_pic %d fail\n",
					pic->index);
			pic->index = -1;
			break;
		}
		pic->width = avs3_dec->img.width;
		pic->height = avs3_dec->img.height;
	}
	for (; i < avs3_dec->max_pb_size; i++) {
		pic = &avs3_dec->pic_pool[i].buf_cfg;
		pic->index = -1;
		pic->BUF_index = -1;
		//pic->mv_buf_index = -1;
	}
	avs3_print(dec, AVS3_DBG_BUFMGR,
		"%s ok, max_pb_size = %d\n",
		__func__, avs3_dec->max_pb_size);
	dec->pic_list_wait_alloc_done_flag = BUFFER_ALLOCATE_DONE;
}

static void init_pic_list_hw(struct AVS3Decoder_s *dec)
{
	int i;
	struct avs3_decoder *avs3_dec = &dec->avs3_dec;
	struct avs3_frame_s *pic;
	/*WRITE_VREG(HEVCD_MPP_ANC2AXI_TBL_CONF_ADDR, 0x0);*/
#if 0
	WRITE_VREG(HEVCD_MPP_ANC2AXI_TBL_CONF_ADDR,
		(0x1 << 1) | (0x1 << 2));

#ifdef DUAL_CORE_64
	WRITE_VREG(HEVC2_HEVCD_MPP_ANC2AXI_TBL_CONF_ADDR,
		(0x1 << 1) | (0x1 << 2));
#endif
#endif
	for (i = 0; i < avs3_dec->max_pb_size; i++) {
		pic = &avs3_dec->pic_pool[i].buf_cfg;
		if (pic->index < 0)
			break;
#ifdef AVS3_10B_MMU
	/*WRITE_VREG(HEVCD_MPP_ANC2AXI_TBL_CMD_ADDR,
		pic->header_adr
		| (pic->mc_canvas_y << 8)|0x1);*/
		WRITE_VREG(HEVCD_MPP_ANC2AXI_TBL_CONF_ADDR,
			(0x1 << 1) | (pic->index << 8));

#ifdef DUAL_CORE_64
	if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_GXLX2)
		WRITE_VREG(HEVC2_MPP_ANC2AXI_TBL_CONF_ADDR,
			(0x1 << 1) | (pic->index << 8));
	else
		WRITE_VREG(HEVC2_HEVCD_MPP_ANC2AXI_TBL_CONF_ADDR,
			(0x1 << 1) | (pic->index << 8));
#endif
	WRITE_VREG(HEVCD_MPP_ANC2AXI_TBL_DATA, pic->header_adr >> 5);
#else
	/*WRITE_VREG(HEVCD_MPP_ANC2AXI_TBL_CMD_ADDR,
		pic->mc_y_adr
		| (pic->mc_canvas_y << 8) | 0x1);*/
	WRITE_VREG(HEVCD_MPP_ANC2AXI_TBL_DATA, pic->mc_y_adr >> 5);
#endif
#ifndef LOSLESS_COMPRESS_MODE
	/*WRITE_VREG(HEVCD_MPP_ANC2AXI_TBL_CMD_ADDR,
		pic->mc_u_v_adr
		| (pic->mc_canvas_u_v << 8)| 0x1);*/
	WRITE_VREG(HEVCD_MPP_ANC2AXI_TBL_DATA, pic->mc_u_v_adr >> 5);
#endif
#ifdef DUAL_CORE_64
#ifdef AVS3_10B_MMU
	WRITE_VREG(HEVC2_HEVCD_MPP_ANC2AXI_TBL_DATA,
		pic->header_adr >> 5);
#else
	WRITE_VREG(HEVC2_HEVCD_MPP_ANC2AXI_TBL_DATA,
		pic->mc_y_adr >> 5);
#endif
#ifndef LOSLESS_COMPRESS_MODE
	WRITE_VREG(HEVC2_HEVCD_MPP_ANC2AXI_TBL_DATA,
		pic->mc_u_v_adr >> 5);
#endif
/*DUAL_CORE_64*/
#endif
	}
	WRITE_VREG(HEVCD_MPP_ANC2AXI_TBL_CONF_ADDR, 0x1);
#ifdef DUAL_CORE_64
	WRITE_VREG(HEVC2_HEVCD_MPP_ANC2AXI_TBL_CONF_ADDR,
		0x1);
#endif
	/*Zero out canvas registers in IPP -- avoid simulation X*/
	WRITE_VREG(HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR,
			(0 << 8) | (0 << 1) | 1);
	for (i = 0; i < 32; i++) {
		WRITE_VREG(HEVCD_MPP_ANC_CANVAS_DATA_ADDR, 0);
#ifdef DUAL_CORE_64
		WRITE_VREG(HEVC2_HEVCD_MPP_ANC_CANVAS_DATA_ADDR, 0);
#endif
	}
}

#ifdef DEBUG_CMD
#if 0
static void d_fill_zero(struct AVS3Decoder_s *hw, unsigned int phyadr, int size)
{
	WRITE_VREG(HEVC_DBG_LOG_ADR, phyadr);
	WRITE_VREG(DEBUG_REG1,
		0x20000000 | size);
	debug_cmd_wait_count = 0;
	debug_cmd_wait_type = 1;
	while ((READ_VREG(DEBUG_REG1) & 0x1) == 0
		&& debug_cmd_wait_count < 0x7fffffff) {
		debug_cmd_wait_count++;
	}

	WRITE_VREG(DEBUG_REG1, 0);
	debug_cmd_wait_type = 0;
}
#endif
#if 0
static void d_dump(struct AVS3Decoder_s *hw, unsigned int phyadr, int size,
	struct file *fp, loff_t *wr_off)
{

	int jj;
	unsigned char *data = (unsigned char *)
		(hw->ucode_log_addr);
	WRITE_VREG(HEVC_DBG_LOG_ADR, hw->ucode_log_phy_addr);

	WRITE_VREG(HEVC_D_ADR, phyadr);
	WRITE_VREG(DEBUG_REG1,
		0x10000000 | size);

	debug_cmd_wait_count = 0;
	debug_cmd_wait_type = 3;
	while ((READ_VREG(DEBUG_REG1) & 0x1) == 0
		&& debug_cmd_wait_count < 0x7fffffff) {
		debug_cmd_wait_count++;
	}

	if (fp) {
		vfs_write(fp, data,
			size, wr_off);

	} else {
		for (jj = 0; jj < size; jj++) {
			if ((jj & 0xf) == 0)
				avs3_print(hw, 0,
					"%06x:", jj);
			avs3_print_cont(hw, 0,
				"%02x ", data[jj]);
			if (((jj + 1) & 0xf) == 0)
				avs3_print_cont(hw, 0,
					"\n");
		}
		avs3_print(hw, 0, "\n");
	}

	WRITE_VREG(DEBUG_REG1, 0);
	debug_cmd_wait_type = 0;

}

static void mv_buffer_fill_zero(struct AVS3Decoder_s *hw, struct PIC_BUFFER_CONFIG_s *pic_config)
{
	pr_info("fill dummy data pic index %d colocate addresses %x size %x\n",
		pic_config->index, pic_config->mpred_mv_wr_start_addr,
		hw->m_mv_BUF[pic_config->mv_buf_index].size);
	d_fill_zero(hw, pic_config->mpred_mv_wr_start_addr,
		hw->m_mv_BUF[pic_config->mv_buf_index].size);
}

static void dump_mv_buffer(struct AVS3Decoder_s *hw, struct PIC_BUFFER_CONFIG_s *pic_config)
{

	unsigned int adr, size;
	unsigned int adr_end = pic_config->mpred_mv_wr_start_addr +
		hw->m_mv_BUF[pic_config->mv_buf_index].size;
	mm_segment_t old_fs;
	loff_t off = 0;
	int mode = O_CREAT | O_WRONLY | O_TRUNC;
	char file[64];
	struct file *fp;
	sprintf(&file[0], "/data/tmp/colocate%d", hw->frame_count-1);
	fp = filp_open(file, mode, 0666);
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	for (adr = pic_config->mpred_mv_wr_start_addr;
		adr < adr_end;
		adr += UCODE_LOG_BUF_SIZE) {
		size = UCODE_LOG_BUF_SIZE;
		if (size > (adr_end - adr))
			size = adr_end - adr;
		pr_info("dump pic index %d colocate addresses %x size %x\n",
			pic_config->index, adr, size);
		d_dump(hw, adr, size, fp, &off);
	}
	set_fs(old_fs);
	vfs_fsync(fp, 0);

	filp_close(fp, current->files);
}
#endif
#endif

static int config_mc_buffer(struct AVS3Decoder_s *dec)
{
	int32_t i;
	struct avs3_decoder *avs3_dec = &dec->avs3_dec;
	//avs3_frame_t *cur_pic = avs3_dec->cur_pic;
	avs3_frame_t *pic;
	//uint32_t rdata32;
	//uint32_t rdata32_2;
	//if (avs3_dec->img.type == I_IMG)
	//    return 0;
	avs3_print(dec, AVS3_DBG_BUFMGR_MORE,
		"Entered config_mc_buffer....\n");
	if (avs3_dec->f_bg != NULL) {
		avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
			"config_mc_buffer for background (canvas_y %d, canvas_u_v %d)\n",
		avs3_dec->f_bg->mc_canvas_y, avs3_dec->f_bg->mc_canvas_u_v);
		//WRITE_VREG(HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, (7 << 8) | (0<<1) | 1);   // L0:BG
		WRITE_VREG(HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, (15 << 8) | (0<<1) | 1);   // L0:BG
		WRITE_VREG(HEVCD_MPP_ANC_CANVAS_DATA_ADDR,
		(avs3_dec->f_bg->mc_canvas_u_v<<16)|(avs3_dec->f_bg->mc_canvas_u_v<<8)|avs3_dec->f_bg->mc_canvas_y);
		//WRITE_VREG(HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, (23 << 8) | (0<<1) | 1);  // L1:BG
		WRITE_VREG(HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, (31 << 8) | (0<<1) | 1);  // L1:BG
		WRITE_VREG(HEVCD_MPP_ANC_CANVAS_DATA_ADDR,
		(avs3_dec->f_bg->mc_canvas_u_v<<16)|(avs3_dec->f_bg->mc_canvas_u_v<<8)|avs3_dec->f_bg->mc_canvas_y);
	}
	if (avs3_dec->slice_type == SLICE_I)
		return 0;
	if (avs3_dec->slice_type == SLICE_P || avs3_dec->slice_type == SLICE_B) {
		avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
			"config_mc_buffer for REF_0, img type %d\n", avs3_dec->slice_type);
		WRITE_VREG(HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, (0 << 8) | (0<<1) | 1);
		for (i = 0; i < avs3_dec->ctx.dpm.num_refp[REFP_0]; i++) {
		pic = &avs3_dec->ctx.refp[i][REFP_0].pic->buf_cfg;
		WRITE_VREG(HEVCD_MPP_ANC_CANVAS_DATA_ADDR,
				(pic->mc_canvas_u_v<<16)|(pic->mc_canvas_u_v<<8)|pic->mc_canvas_y);
			if (pic->error_mark) {
				avs3_dec->ctx.pic->buf_cfg.error_mark = 1;
				avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL, "%s:L0 refid %d pic error\n", __func__, i);
			}
			avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
				"L0 refid %x mc_canvas_u_v %x mc_canvas_y %x\n", i, pic->mc_canvas_u_v, pic->mc_canvas_y);
		}
	}
	if (avs3_dec->slice_type == SLICE_B) {
		avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
			"config_mc_buffer for REF_1\n");

		WRITE_VREG(HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, (16 << 8) | (0<<1) | 1);
		for (i = 0; i < avs3_dec->ctx.dpm.num_refp[REFP_1]; i++) {
			pic = &avs3_dec->ctx.refp[i][REFP_1].pic->buf_cfg;
			WRITE_VREG(HEVCD_MPP_ANC_CANVAS_DATA_ADDR,
				(pic->mc_canvas_u_v<<16)|(pic->mc_canvas_u_v<<8)|pic->mc_canvas_y);
			if (pic->error_mark) {
				avs3_dec->ctx.pic->buf_cfg.error_mark = 1;
				avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL, "%s:L1 refid %d pic error\n", __func__, i);
			}
			avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
				"L1 refid %x mc_canvas_u_v %x mc_canvas_y %x\n", i, pic->mc_canvas_u_v, pic->mc_canvas_y);
		}
	}
	return 0;
}
#if 0
static void mcrcc_get_hitrate(void)
{
	u32 tmp;
	u32 raw_mcr_cnt;
	u32 hit_mcr_cnt;
	u32 byp_mcr_cnt_nchoutwin;
	u32 byp_mcr_cnt_nchcanv;
	int hitrate;

	if (debug & AVS3_DBG_CACHE)
		pr_info("[cache_util.c] Entered mcrcc_get_hitrate...\n");
	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL, (unsigned int)(0x0<<1));
	raw_mcr_cnt = READ_VREG(HEVCD_MCRCC_PERFMON_DATA);
	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL, (unsigned int)(0x1<<1));
	hit_mcr_cnt = READ_VREG(HEVCD_MCRCC_PERFMON_DATA);
	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL, (unsigned int)(0x2<<1));
	byp_mcr_cnt_nchoutwin = READ_VREG(HEVCD_MCRCC_PERFMON_DATA);
	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL, (unsigned int)(0x3<<1));
	byp_mcr_cnt_nchcanv = READ_VREG(HEVCD_MCRCC_PERFMON_DATA);

	if (debug & AVS3_DBG_CACHE) {
		pr_info("raw_mcr_cnt_total: %d\n",raw_mcr_cnt);
		pr_info("hit_mcr_cnt_total: %d\n",hit_mcr_cnt);
		pr_info("byp_mcr_cnt_nchoutwin_total: %d\n",byp_mcr_cnt_nchoutwin);
		pr_info("byp_mcr_cnt_nchcanv_total: %d\n",byp_mcr_cnt_nchcanv);
	}
	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL, (unsigned int)(0x4<<1));
	tmp = READ_VREG(HEVCD_MCRCC_PERFMON_DATA);
	if (debug & AVS3_DBG_CACHE)
		pr_info("miss_mcr_0_cnt_total: %d\n", tmp);

	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL, (unsigned int)(0x5<<1));
	tmp = READ_VREG(HEVCD_MCRCC_PERFMON_DATA);
	if (debug & AVS3_DBG_CACHE)
		pr_info("miss_mcr_1_cnt_total: %d\n", tmp);

	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL, (unsigned int)(0x6<<1));
	tmp = READ_VREG(HEVCD_MCRCC_PERFMON_DATA);
	if (debug & AVS3_DBG_CACHE)
		pr_info("hit_mcr_0_cnt_total: %d\n",tmp);

	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL, (unsigned int)(0x7<<1));
	tmp= READ_VREG(HEVCD_MCRCC_PERFMON_DATA);
	if (debug & AVS3_DBG_CACHE)
		pr_info("hit_mcr_1_cnt_total: %d\n",tmp);

	if (raw_mcr_cnt != 0) {
		hitrate = (hit_mcr_cnt / raw_mcr_cnt) * 100;
		if (debug & AVS3_DBG_CACHE)
			pr_info("MCRCC_HIT_RATE : %d\n", hitrate);
		hitrate = ((byp_mcr_cnt_nchoutwin + byp_mcr_cnt_nchcanv)
			/raw_mcr_cnt) * 100;
		if (debug & AVS3_DBG_CACHE)
			pr_info("MCRCC_BYP_RATE : %d\n", hitrate);
	} else if (debug & AVS3_DBG_CACHE) {
			pr_info("MCRCC_HIT_RATE : na\n");
			pr_info("MCRCC_BYP_RATE : na\n");
	}
	return;
}

static void  decomp_get_hitrate(void)
{
	u32 raw_mcr_cnt;
	u32 hit_mcr_cnt;
	int hitrate;

	if (debug & AVS3_DBG_CACHE)
		pr_info("[cache_util.c] Entered decomp_get_hitrate...\n");
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL, (unsigned int)(0x0<<1));
	raw_mcr_cnt = READ_VREG(HEVCD_MPP_DECOMP_PERFMON_DATA);
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL, (unsigned int)(0x1<<1));
	hit_mcr_cnt = READ_VREG(HEVCD_MPP_DECOMP_PERFMON_DATA);

	if (debug & AVS3_DBG_CACHE) {
		pr_info("hcache_raw_cnt_total: %d\n",raw_mcr_cnt);
		pr_info("hcache_hit_cnt_total: %d\n",hit_mcr_cnt);
	}
	if (raw_mcr_cnt != 0) {
		hitrate = (hit_mcr_cnt / raw_mcr_cnt) * 100;
		if (debug & AVS3_DBG_CACHE)
			pr_info("DECOMP_HCACHE_HIT_RATE : %d\n", hitrate);
	} else {
		if (debug & AVS3_DBG_CACHE)
			pr_info("DECOMP_HCACHE_HIT_RATE : na\n");
	}
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL, (unsigned int)(0x2<<1));
	raw_mcr_cnt = READ_VREG(HEVCD_MPP_DECOMP_PERFMON_DATA);
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL, (unsigned int)(0x3<<1));
	hit_mcr_cnt = READ_VREG(HEVCD_MPP_DECOMP_PERFMON_DATA);

	if (debug & AVS3_DBG_CACHE) {
		pr_info("dcache_raw_cnt_total: %d\n", raw_mcr_cnt);
		pr_info("dcache_hit_cnt_total: %d\n", hit_mcr_cnt);
	}
	if (raw_mcr_cnt != 0) {
		hitrate = (hit_mcr_cnt / raw_mcr_cnt) * 100;
		if (debug & AVS3_DBG_CACHE)
			pr_info("DECOMP_DCACHE_HIT_RATE : %d\n", hitrate);
	} else if (debug & AVS3_DBG_CACHE) {
		pr_info("DECOMP_DCACHE_HIT_RATE : na\n");
	}
return;
}

static void decomp_get_comprate(void)
{
	u32 raw_ucomp_cnt;
	u32 fast_comp_cnt;
	u32 slow_comp_cnt;
	int comprate;

	if (debug & AVS3_DBG_CACHE)
		pr_info("[cache_util.c] Entered decomp_get_comprate...\n");
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL, (unsigned int)(0x4<<1));
	fast_comp_cnt = READ_VREG(HEVCD_MPP_DECOMP_PERFMON_DATA);
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL, (unsigned int)(0x5<<1));
	slow_comp_cnt = READ_VREG(HEVCD_MPP_DECOMP_PERFMON_DATA);
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL, (unsigned int)(0x6<<1));
	raw_ucomp_cnt = READ_VREG(HEVCD_MPP_DECOMP_PERFMON_DATA);
	if (debug & AVS3_DBG_CACHE) {
		pr_info("decomp_fast_comp_total: %d\n", fast_comp_cnt);
		pr_info("decomp_slow_comp_total: %d\n", slow_comp_cnt);
		pr_info("decomp_raw_uncomp_total: %d\n", raw_ucomp_cnt);
	}

	if (raw_ucomp_cnt != 0) {
		comprate = ((fast_comp_cnt + slow_comp_cnt)
			/ raw_ucomp_cnt) * 100;
		if (debug & AVS3_DBG_CACHE)
			pr_info("DECOMP_COMP_RATIO : %d\n", comprate);
	} else if (debug & AVS3_DBG_CACHE) {
			pr_info("DECOMP_COMP_RATIO : na\n");
	}
	return;
}
#endif

static void config_mcrcc_axi_hw(struct AVS3Decoder_s *dec)
{
	struct avs3_decoder *avs3_dec = &dec->avs3_dec;
	uint32_t rdata32;
	uint32_t rdata32_2;

	WRITE_VREG(HEVCD_MCRCC_CTL1, 0x2); // reset mcrcc
	if (avs3_dec->slice_type == SLICE_I) {
		WRITE_VREG(HEVCD_MCRCC_CTL1, 0x0); // remove reset -- disables clock
		return;
	}

#if 0
	mcrcc_get_hitrate();
	decomp_get_hitrate();
	decomp_get_comprate();
#endif
	if ((avs3_dec->slice_type == SLICE_B) || (avs3_dec->slice_type == SLICE_P)) {
		// Programme canvas0
		WRITE_VREG(HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, (0 << 8) | (0<<1) | 0);
		rdata32 = READ_VREG(HEVCD_MPP_ANC_CANVAS_DATA_ADDR);
		rdata32 = rdata32 & 0xffff;
		rdata32 = rdata32 | ( rdata32 << 16);
		WRITE_VREG(HEVCD_MCRCC_CTL2, rdata32);

		// Programme canvas1
		WRITE_VREG(HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, (16 << 8) | (1<<1) | 0);
		rdata32_2 = READ_VREG(HEVCD_MPP_ANC_CANVAS_DATA_ADDR);
		rdata32_2 = rdata32_2 & 0xffff;
		rdata32_2 = rdata32_2 | ( rdata32_2 << 16);
		if ( rdata32 == rdata32_2 ) {
		rdata32_2 = READ_VREG(HEVCD_MPP_ANC_CANVAS_DATA_ADDR);
		rdata32_2 = rdata32_2 & 0xffff;
		rdata32_2 = rdata32_2 | ( rdata32_2 << 16);
		}
		WRITE_VREG(HEVCD_MCRCC_CTL3, rdata32_2);
	} else { // P-PIC
		WRITE_VREG(HEVCD_MPP_ANC_CANVAS_ACCCONFIG_ADDR, (0 << 8) | (1<<1) | 0);
		rdata32 = READ_VREG(HEVCD_MPP_ANC_CANVAS_DATA_ADDR);
		rdata32 = rdata32 & 0xffff;
		rdata32 = rdata32 | ( rdata32 << 16);
		WRITE_VREG(HEVCD_MCRCC_CTL2, rdata32);

		// Programme canvas1
		rdata32 = READ_VREG(HEVCD_MPP_ANC_CANVAS_DATA_ADDR);
		rdata32 = rdata32 & 0xffff;
		rdata32 = rdata32 | ( rdata32 << 16);
		WRITE_VREG(HEVCD_MCRCC_CTL3, rdata32);
	}

	WRITE_VREG(HEVCD_MCRCC_CTL1, 0xff0); // enable mcrcc progressive-mode
	return;
}

static void config_mpred_hw(struct AVS3Decoder_s *dec)
{
	struct avs3_decoder *avs3_dec = &dec->avs3_dec;
	int32_t i;
	uint32_t data32;
	avs3_frame_t *cur_pic = avs3_dec->cur_pic;
	avs3_frame_t *col_pic;
	//COM_PIC *colPic;

	//int32_t     AMVP_MAX_NUM_CANDS_MEM=3;
	//int32_t     AMVP_MAX_NUM_CANDS=2;
	//int32_t     NUM_CHROMA_MODE=5;
	//int32_t     DM_CHROMA_IDX=36;
	int32_t     above_ptr_ctrl =0;
	//int32_t     buffer_linear =1;
	//int32_t     cu_size_log2 =3;
	int32_t     mpred_mv_rd_start_addr ;
	//int32_t     mpred_curr_lcu_x;
	//int32_t     mpred_curr_lcu_y;
	//int32_t     mpred_above_buf_start ;
	//int32_t     mpred_mv_rd_ptr ;
	//int32_t     mpred_mv_rd_ptr_p1 ;
	int32_t     mpred_mv_rd_end_addr;
	int32_t     MV_MEM_UNIT_l;
	//int32_t     mpred_mv_wr_ptr ;

	int32_t     above_en;
	int32_t     mv_wr_en;
	int32_t     mv_rd_en;
	int32_t     col_isIntra;
	int32_t     col_ptr;

	if (avs3_dec->slice_type == SLICE_P) {
		col_pic = &avs3_dec->ctx.refp[0][REFP_0].pic->buf_cfg;
	}
	else if (avs3_dec->slice_type == SLICE_B) {
		col_pic = &avs3_dec->ctx.refp[0][REFP_1].pic->buf_cfg;
	}
	else {
		col_pic = cur_pic;
	}

	//if (dec->slice_type!=2)
	if (avs3_dec->slice_type != SLICE_I)
	{
		above_en=1;
		mv_wr_en=1;
		if (col_pic->slice_type != SLICE_I)
		mv_rd_en=1;
		else
		mv_rd_en=0;
		col_isIntra=0;
	}
	else
	{
		above_en=1;
		mv_wr_en=1;
		mv_rd_en=0;
		col_isIntra=0;
	}

	mpred_mv_rd_start_addr=col_pic->mpred_mv_wr_start_addr;
	/*data32 = READ_VREG(HEVC_MPRED_CURR_LCU);
	mpred_curr_lcu_x   =data32 & 0xffff;
	mpred_curr_lcu_y   =(data32>>16) & 0xffff;*/

	MV_MEM_UNIT_l=get_mv_mem_unit(avs3_dec->lcu_size_log2);

	mpred_mv_rd_end_addr=mpred_mv_rd_start_addr + ((avs3_dec->lcu_x_num*avs3_dec->lcu_y_num)*MV_MEM_UNIT_l);

	//mpred_above_buf_start = buf_spec->mpred_above.buf_start;

	avs3_print(dec, AVS3_DBG_BUFMGR_MORE,
		"cur pic index %d  slicetype %d col pic index %d slicetype %d\n",
		cur_pic->index, cur_pic->slice_type,
		col_pic->index, col_pic->slice_type);

	WRITE_VREG(HEVC_MPRED_MV_WR_START_ADDR, cur_pic->mpred_mv_wr_start_addr);
	WRITE_VREG(HEVC_MPRED_MV_RD_START_ADDR, col_pic->mpred_mv_wr_start_addr);
	avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
		"[MPRED CO_MV] write 0x%x  read 0x%x\n", cur_pic->mpred_mv_wr_start_addr, col_pic->mpred_mv_wr_start_addr);
	/*
	data32 =
	((avs3_dec->bk_img_is_top_field) << 13) |
	((avs3_dec->hd.background_picture_enable & 1) << 12) |
		((avs3_dec->hd.curr_RPS.num_of_ref & 7) << 8) |
		((avs3_dec->hd.b_pmvr_enabled & 1) << 6) |
		((avs3_dec->img.is_top_field & 1) << 5)  |
		((avs3_dec->img.is_field_sequence & 1) << 4) |
		((avs3_dec->img.typeb & 7) << 1) |
		(avs3_dec->hd.background_reference_enable & 0x1);
	printk("HEVC_MPRED_CTRL9 <= 0x%x(num of ref %d)\n", data32, avs3_dec->hd.curr_RPS.num_of_ref);
	WRITE_VREG(HEVC_MPRED_CTRL9, data32);
	*/
#if 1
	/*
	data32 = ((dec->lcu_x_num - dec->tile_width_lcu)*MV_MEM_UNIT);
	WRITE_VREG(HEVC_MPRED_MV_WR_ROW_JUMP,data32);
	WRITE_VREG(HEVC_MPRED_MV_RD_ROW_JUMP,data32);
	*/
	data32 = READ_VREG(HEVC_MPRED_CTRL0);
	data32 &= (~(0x3 | (0xf << 8) | (0xf << 16)));
	data32  =   (
		avs3_dec->slice_type |
		/*dec->new_pic<<2 |
		dec->new_tile<<3|
		dec->isNextSliceSegment<<4|
		dec->TMVPFlag<<5|
		dec->LDCFlag<<6|
		dec->ColFromL0Flag<<7|
	 */
		above_ptr_ctrl<<8 |
		above_en<<9|
		mv_wr_en<<10|
		mv_rd_en<<11|
		avs3_dec->lcu_size_log2<<16
		/*|cu_size_log2<<20*/
		);
	WRITE_VREG(HEVC_MPRED_CTRL0,data32);
	avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
		"P_HEVC_MPRED_CTRL0=0x%x\n",
		READ_VREG(HEVC_MPRED_CTRL0));

	data32 = READ_VREG(HEVC_MPRED_CTRL1);
	data32 &= (~0xf);
	data32 |= avs3_dec->ctx.info.sqh.num_of_hmvp_cand;
	avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
		"write P_HEVC_MPRED_CTRL1=0x%x, avs3_dec->ctx.info.sqh.num_of_hmvp_cand=%d\n",
		data32, avs3_dec->ctx.info.sqh.num_of_hmvp_cand);
	WRITE_VREG(HEVC_MPRED_CTRL1,data32);
	avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
		"P_HEVC_MPRED_CTRL1=0x%x\n",
		READ_VREG(HEVC_MPRED_CTRL1));

	data32  =   (
		avs3_dec->img.width|
		avs3_dec->img.height<<16
		);
	WRITE_VREG(HEVC_MPRED_PIC_SIZE,data32);

	data32  =   (
		(avs3_dec->lcu_x_num-1)   |
		(avs3_dec->lcu_y_num-1)<<16
		);
	WRITE_VREG(HEVC_MPRED_PIC_SIZE_LCU,data32);

	/*data32  =   (
		dec->tile_start_lcu_x   |
		dec->tile_start_lcu_y<<16
		);
	WRITE_VREG(HEVC_MPRED_TILE_START,data32);
	data32  =   (
		dec->tile_width_lcu   |
		dec->tile_height_lcu<<16
		);
	WRITE_VREG(HEVC_MPRED_TILE_SIZE_LCU,data32);
	*/
	data32  =   (
		avs3_dec->ctx.dpm.num_refp[REFP_0]   |
		avs3_dec->ctx.dpm.num_refp[REFP_1]<<8|
		0
		//col_RefNum_L0<<16|
		//col_RefNum_L1<<24
		);
	WRITE_VREG(HEVC_MPRED_REF_NUM,data32);

	data32=0;
	for (i=0;i<avs3_dec->ctx.dpm.num_refp[REFP_0];i++)data32=data32|(1<<i);
	WRITE_VREG(HEVC_MPRED_REF_EN_L0,data32);

	data32=0;
	for (i=0;i<avs3_dec->ctx.dpm.num_refp[REFP_1];i++)data32=data32|(1<<i);
	WRITE_VREG(HEVC_MPRED_REF_EN_L1,data32);
#endif

	WRITE_VREG(HEVC_MPRED_CUR_POC, avs3_dec->ctx.ptr & 0xffff);
	if (avs3_dec->slice_type == SLICE_P) {
		col_ptr = avs3_dec->ctx.refp[0][REFP_0].ptr;
		//colPic = avs3_dec->ctx.refp[0][REFP_0].pic;
	}
	else if (avs3_dec->slice_type == SLICE_B) {
		col_ptr = avs3_dec->ctx.refp[0][REFP_1].ptr;
		//colPic = avs3_dec->ctx.refp[0][REFP_0].pic;
	}
	else {
		col_ptr = avs3_dec->ctx.pic->ptr;
		//colPic = avs3_dec->ctx.pic;
	}
	WRITE_VREG(HEVC_MPRED_COL_POC, col_ptr & 0xffff);
	//below MPRED Ref_POC_xx_Lx registers must follow Ref_POC_xx_L0 -> Ref_POC_xx_L1 in pair write order!!!
	avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
		"P_HEVC_MPRED_CUR_POC=0x%x, P_HEVC_MPRED_COL_POC=0x%x\n",
		READ_VREG(HEVC_MPRED_CUR_POC), READ_VREG(HEVC_MPRED_COL_POC));

	for (i = 0; i < MAX_NUM_REF_PICS; i++) {
		data32 = 0;
		if (i < cur_pic->list0_num_refp || i < col_pic->list0_num_refp) {
		if (i < cur_pic->list0_num_refp) {
			data32 |= cur_pic->list0_ptr[i] & 0xffff;
		}
		if (i < col_pic->list0_num_refp) {
			data32 |= ((col_pic->list0_ptr[i] & 0xffff) << 16);
		}
		}
		WRITE_VREG(HEVC_MPRED_L0_REF_POC_ADR[i], data32);
	}
	if (debug & AVS3_DBG_BUFMGR_DETAIL) {
		for (i = 0; i < MAX_NUM_REF_PICS; i++) {
			if (i < cur_pic->list0_num_refp || i < col_pic->list0_num_refp) {
				if (i < cur_pic->list0_num_refp) {
					avs3_print_cont(dec, AVS3_DBG_BUFMGR_DETAIL,
						"[%d] ", cur_pic->list0_ptr[i]);
				}
				if (i < col_pic->list0_num_refp) {
					avs3_print_cont(dec, AVS3_DBG_BUFMGR_DETAIL,
						"<%d> ", col_pic->list0_ptr[i]);
				}
			}
			if (i < cur_pic->list0_num_refp || i < col_pic->list0_num_refp) {
				avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
					"P_HEVC_MPRED_L0_REF=0x%x (readback 0x%x)\n", i,
					data32, READ_VREG(HEVC_MPRED_L0_REF_POC_ADR[i]));
			}
		}
	}
	for (i = 0; i < avs3_dec->ctx.dpm.num_refp[REFP_1]; i++) {
		WRITE_VREG(HEVC_MPRED_L1_REF_POC_ADR[i], avs3_dec->ctx.refp[i][REFP_1].ptr & 0xffff);
	}
	if (debug & AVS3_DBG_BUFMGR_DETAIL) {
		for (i = 0; i < avs3_dec->ctx.dpm.num_refp[REFP_1]; i++) {
			avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
				"P_HEVC_MPRED_L1_REF%02d_POC=0x%x (readback 0x%x)\n", i, avs3_dec->ctx.refp[i][REFP_1].ptr & 0xffff, READ_VREG(HEVC_MPRED_L1_REF_POC_ADR[i]));
		}
	}

#if 0
	if (dec->new_pic)
	{
		WRITE_VREG(HEVC_MPRED_ABV_START_ADDR,mpred_above_buf_start);
		WRITE_VREG(HEVC_MPRED_MV_WPTR,mpred_mv_wr_ptr);
		//WRITE_VREG(HEVC_MPRED_MV_RPTR,mpred_mv_rd_ptr);
		WRITE_VREG(HEVC_MPRED_MV_RPTR,mpred_mv_rd_start_addr);
	}
	else if (!dec->isNextSliceSegment)
	{
		//WRITE_VREG(HEVC_MPRED_MV_RPTR,mpred_mv_rd_ptr_p1);
		WRITE_VREG(HEVC_MPRED_MV_RPTR,mpred_mv_rd_ptr);
	}
#endif
	WRITE_VREG(HEVC_MPRED_MV_RD_END_ADDR,mpred_mv_rd_end_addr);
}

static void config_dblk_hw(struct AVS3Decoder_s *dec)
{
	/*
	* Picture level de-block parameter configuration here
	*/
	struct avs3_decoder *avs3_dec = &dec->avs3_dec;
	union param_u *rpm_param = &avs3_dec->param;
	uint32_t data32;
	DEC_CTX * ctx = &avs3_dec->ctx;
	int32_t alpha_c_offset = rpm_param->p.pic_header_alpha_c_offset;
	int32_t beta_offset = rpm_param->p.pic_header_beta_offset;

	alpha_c_offset = (alpha_c_offset >=16) ? 15 : ((alpha_c_offset < -16) ? -16 : alpha_c_offset);
	beta_offset = (beta_offset >=16) ? 15 : ((beta_offset < -16) ? -16 : beta_offset);

	data32 = READ_VREG(HEVC_DBLK_CFG1);
	data32 = (((data32>>20)&0xfff)<<20) |
		(((ctx->info.bit_depth_internal == 10) ? 0xa:0x0)<<16) |             // [16 +: 4]: {luma_bd[1:0],chroma_bd[1:0]}
		(((data32>>2)&0x3fff)<<2) |
		(((ctx->info.log2_max_cuwh == 6) ? 0:(ctx->info.log2_max_cuwh == 5) ? 1:(ctx->info.log2_max_cuwh == 4) ? 2:3)<<0);// [ 0 +: 2]: lcu_size
	WRITE_VREG(HEVC_DBLK_CFG1, data32);

	data32 = (avs3_dec->img.height<<16) | avs3_dec->img.width;
	WRITE_VREG(HEVC_DBLK_CFG2, data32);

	data32 = ((rpm_param->p.sqh_cross_patch_loop_filter & 0x1)<<27) |// [27 +: 1]: cross_slice_loopfilter_enable_flag
		((rpm_param->p.pic_header_loop_filter_disable_flag & 0x1)<<26) |               // [26 +: 1]: loop_filter_disable
		((alpha_c_offset&0x1f) <<17) |                    // [17 +: 5]: alpha_c_offset (-8~8)
		((beta_offset&0x1f) <<12) |                       // [12 +: 5]: beta_offset (-8~8)
		((rpm_param->p.pic_header_chroma_quant_param_delta_cb&0x3f)<<6) |         // [ 6 +: 6]: chroma_quant_param_delta_u (-16~16)
		((rpm_param->p.pic_header_chroma_quant_param_delta_cr&0x3f)<<0);          // [ 0 +: 6]: chroma_quant_param_delta_v (-16~16)
	WRITE_VREG(HEVC_DBLK_CFG9, data32);
	avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
		"[c] cfgDBLK: crossslice(%d),lfdisable(%d),bitDepth(%d,%d),log2_lcuSize(%d)\n",
			rpm_param->p.sqh_cross_patch_loop_filter,rpm_param->p.pic_header_loop_filter_disable_flag,
			ctx->info.bit_depth_input,ctx->info.bit_depth_internal,ctx->info.log2_max_cuwh);
	avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
		"[c] cfgDBLK: alphaCOffset(%d crop to %d),betaOffset(%d crop to %d),quantDeltaCb(%d),quantDeltaCr(%d)\n",
			rpm_param->p.pic_header_alpha_c_offset, alpha_c_offset,rpm_param->p.pic_header_beta_offset, beta_offset,
			rpm_param->p.pic_header_chroma_quant_param_delta_cb,rpm_param->p.pic_header_chroma_quant_param_delta_cr);
	avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
		"[c] cfgDBLK: .done.\n");

}

static void config_dw(struct AVS3Decoder_s *dec, struct avs3_frame_s *pic,
		u32 mc_buffer_size_u_v_h)
{

	int dw_mode = get_double_write_mode(dec);
#ifdef OW_TRIPLE_WRITE
	int tw_mode = get_triple_write_mode(dec);
#endif
	uint32_t data32;

	if ((dw_mode & 0x10) == 0) {
		WRITE_VREG(HEVC_SAO_CTRL26, 0);

		data32 = READ_VREG(HEVC_SAO_CTRL5);
		data32 &= (~(0xff << 16));
		if (((dw_mode & 0xf) == 8) ||
			((dw_mode & 0xf) == 9)) {
			data32 |= (0xff << 16);
			WRITE_VREG(HEVC_SAO_CTRL5, data32);
			WRITE_VREG(HEVC_SAO_CTRL26, 0xf);
		} else {
			if ((dw_mode & 0xf) == 2 ||
				(dw_mode & 0xf) == 3)
				data32 |= (0xff<<16);
			else if ((dw_mode & 0xf) == 4 ||
				(dw_mode & 0xf) == 5)
				data32 |= (0x33<<16);

			/*if (dec->mem_saving_mode == 1)
				data32 |= (1 << 9);
			else*/
			data32 &= ~(1 << 9);
			/*if (workaround_enable & 1)
				data32 |= (1 << 7);*/
			WRITE_VREG(HEVC_SAO_CTRL5, data32);
		}
	}

	/* m8baby test1902 */
	data32 = READ_VREG(HEVC_SAO_CTRL1);
	data32 &= (~0x3000);
	/* [13:12] axi_aformat, 0-Linear, 1-32x32, 2-64x32 */
	data32 |= (MEM_MAP_MODE << 12);
	data32 &= (~0xff0);
#ifdef AVS3_10B_MMU_DW
	if (dec->dw_mmu_enable == 0)
		data32 |= ((dec->endian >> 8) & 0xfff); /* endian: ((0x880 << 8) | 0x8) or ((0xff0 << 8) | 0xf) */
#else
	data32 |= ((dec->endian >> 8) & 0xfff); /* data32 |= 0x670; Big-Endian per 64-bit */
#endif

	data32 &= (~0x3); /*[1]:dw_disable [0]:cm_disable*/
	if (dw_mode == 0)
		data32 |= 0x2; /*disable double write*/
	else if (dw_mode & 0x10)
		data32 |= 0x1; /*disable cm*/

	data32 &= (~(3 << 14));
	data32 |= (2 << 14);
	/*
	 *  [31:24] ar_fifo1_axi_thred
	 *  [23:16] ar_fifo0_axi_thred
	 *  [15:14] axi_linealign, 0-16bytes, 1-32bytes, 2-64bytes
	 *  [13:12] axi_aformat, 0-Linear, 1-32x32, 2-64x32
	 *  [11:08] axi_lendian_C
	 *  [07:04] axi_lendian_Y
	 *  [3]     reserved
	 *  [2]     clk_forceon
	 *  [1]     dw_disable:disable double write output
	 *  [0]     cm_disable:disable compress output
	 */
	WRITE_VREG(HEVC_SAO_CTRL1, data32);

	data32 = READ_VREG(HEVC_DBLK_CFGB);
	data32 &= (~0x300); /*[8]:first write enable (compress)
			[9]:double write enable (uncompress)*/
	if (dw_mode == 0)
		data32 |= (0x1 << 8); /*enable first write*/
	else if (dw_mode == 0x10)
		data32 |= (0x1 << 9); /*double write only*/
	else
		data32 |= ((0x1 << 8) | (0x1 << 9));
	WRITE_VREG(HEVC_DBLK_CFGB, data32);

	if (dw_mode & 0x10) {
		/* [23:22] dw_v1_ctrl
		*[21:20] dw_v0_ctrl
		*[19:18] dw_h1_ctrl
		*[17:16] dw_h0_ctrl
		*/
		data32 = READ_VREG(HEVC_SAO_CTRL5);
		/*set them all 0 for H265_NV21 (no down-scale)*/
		data32 &= ~(0xff << 16);
		WRITE_VREG(HEVC_SAO_CTRL5, data32);
	}

#ifdef P010_ENABLE
	if (get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_T3X) {
		data32 = READ_VREG(HEVC_SAO_CTRL3);
		if (is_dw_p010(dec)) {
			data32 |= (1 << 1);  /* enable double write p010 */
		} else {
			data32 &= ~(1 << 1);
		}
		WRITE_VREG(HEVC_SAO_CTRL3, data32);
	}
#endif
#ifdef OW_TRIPLE_WRITE
	if (tw_mode) {
		data32 = READ_VREG(HEVC_SAO_CTRL31);
		data32 &= ~0xfff;
		if ((tw_mode == 2) || (tw_mode == 3)) {
			data32 |= ((0xf << 6) | 0xf);	//1:4
		} else if ((tw_mode == 4) || (tw_mode == 5)) {
			data32 |= ((0x3 << 6) | 0x3);	//1:2
		} else if ((tw_mode == 8) || (tw_mode == 9)) {
			data32 |= ((0x3f << 6) | 0x3f);
		}
		WRITE_VREG(HEVC_SAO_CTRL31, data32);

		data32 = READ_VREG(HEVC_SAO_CTRL32);
		data32 &= (~0xfff); /* clr endian, blkmod and align */
		data32 |= ((dec->endian >> 12) & 0xff);
		data32 |= ((mem_map_mode & 0x3) << 8);
		/* Linear_LineAlignment 00:16byte 01:32byte 10:64byte */
		data32 |= (2 << 10);
		WRITE_VREG(HEVC_SAO_CTRL32, data32);

		data32 = READ_VREG(HEVC_SAO_CTRL3);
		data32 |= (1 << 2);  /* enable triple write */
#ifdef P010_ENABLE
		if (is_tw_p010(dec)) {
			data32 |= (1 << 3);  /* enable triple write p010 */
		} else {
			data32 &= ~(1 << 3);
		}
#endif
		WRITE_VREG(HEVC_SAO_CTRL3, data32);

		if (debug & AVS3_DBG_REG) {
			avs3_print(dec, 0, "%s, HEVC_SAO_CTRL3 %x, HEVC_SAO_CTRL31 %x, HEVC_SAO_CTRL32 %x\n",
				__func__, READ_VREG(HEVC_SAO_CTRL3), READ_VREG(HEVC_SAO_CTRL31), READ_VREG(HEVC_SAO_CTRL32));
		}
	}
#endif

#ifdef LOSLESS_COMPRESS_MODE
/*SUPPORT_10BIT*/

	data32 = pic->mc_y_adr;
	if (dw_mode && ((dw_mode & 0x20) == 0)) {
		WRITE_VREG(HEVC_SAO_Y_START_ADDR, pic->dw_y_adr);
		WRITE_VREG(HEVC_SAO_C_START_ADDR, pic->dw_u_v_adr);
		WRITE_VREG(HEVC_SAO_Y_WPTR, pic->dw_y_adr);
		WRITE_VREG(HEVC_SAO_C_WPTR, pic->dw_u_v_adr);
	} else {
		WRITE_VREG(HEVC_SAO_Y_START_ADDR, 0xffffffff);
		WRITE_VREG(HEVC_SAO_C_START_ADDR, 0xffffffff);
	}
	if ((dw_mode & 0x10) == 0)
		WRITE_VREG(HEVC_CM_BODY_START_ADDR, data32);
#ifdef OW_TRIPLE_WRITE
	if (get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_T3X) {
		if (tw_mode) {
			WRITE_VREG(HEVC_SAO_Y_START_ADDR3, pic->tw_y_adr);
			WRITE_VREG(HEVC_SAO_C_START_ADDR3, pic->tw_u_v_adr);
		} else {
			WRITE_VREG(HEVC_SAO_Y_START_ADDR3, 0xffffffff);
			WRITE_VREG(HEVC_SAO_C_START_ADDR3, 0xffffffff);
		}
	}
#endif

	if (dec->mmu_enable)
		WRITE_VREG(HEVC_CM_HEADER_START_ADDR, pic->header_adr);
#ifdef AVS3_10B_MMU_DW
	if (dec->dw_mmu_enable) {
		WRITE_VREG(HEVC_SAO_Y_START_ADDR, 0);
		WRITE_VREG(HEVC_SAO_C_START_ADDR, 0);
		WRITE_VREG(HEVC_CM_HEADER_START_ADDR2, pic->dw_header_adr);
	}
#endif
#else
	WRITE_VREG(HEVC_SAO_Y_START_ADDR, pic->mc_y_adr);
	WRITE_VREG(HEVC_SAO_C_START_ADDR, pic->mc_u_v_adr);
	WRITE_VREG(HEVC_SAO_Y_WPTR, pic->mc_y_adr);
	WRITE_VREG(HEVC_SAO_C_WPTR, pic->mc_u_v_adr);
#endif
	data32 = mc_buffer_size_u_v_h << (16 + is_dw_p010(dec) + 1);
	WRITE_VREG(HEVC_SAO_Y_LENGTH, data32);

	data32 = mc_buffer_size_u_v_h << (16 + is_dw_p010(dec));
	WRITE_VREG(HEVC_SAO_C_LENGTH, data32);

	if (tw_mode) {
		data32 = mc_buffer_size_u_v_h << (16 + is_tw_p010(dec) + 1);
		WRITE_VREG(HEVC_SAO_Y_LENGTH3, data32);
		data32 = mc_buffer_size_u_v_h << (16 + is_tw_p010(dec));
		WRITE_VREG(HEVC_SAO_C_LENGTH3, data32);

		if (debug & AVS3_DBG_REG) {
			avs3_print(dec, 0, "triple write SAO_Y_ADDR3 %x, SAO_Y_LENGTH3 %x, SAO_C_ADDR3 %x, SAO_C_LENGTH3 %x\n",
				READ_VREG(HEVC_SAO_Y_START_ADDR3), READ_VREG(HEVC_SAO_Y_LENGTH3),
				READ_VREG(HEVC_SAO_C_START_ADDR3), READ_VREG(HEVC_SAO_C_LENGTH3));
		}
	}
}

static void config_sao_hw(struct AVS3Decoder_s *dec)
{
	struct avs3_decoder *avs3_dec = &dec->avs3_dec;
	avs3_frame_t *pic = avs3_dec->cur_pic;
	//union param_u* params = &avs3_dec->param;
	uint32_t data32;
	int32_t pic_width = avs3_dec->img.width;
	int32_t pic_height = avs3_dec->img.height;
	int32_t lcu_size_log2 = avs3_dec->lcu_size_log2;
	int32_t lcu_size = 1<<lcu_size_log2;
	int32_t pic_width_lcu  = ( pic_width % lcu_size  ) ? pic_width /lcu_size + 1 : pic_width /lcu_size;
	int32_t pic_height_lcu = ( pic_height % lcu_size ) ? pic_height/lcu_size + 1 : pic_height/lcu_size;
	int32_t lcu_total = pic_width_lcu*pic_height_lcu;
	int32_t mc_buffer_size_u_v = lcu_total*lcu_size*lcu_size/2;
	int32_t mc_buffer_size_u_v_h = (mc_buffer_size_u_v + 0xffff)>>16; //64k alignment

	config_dw(dec, pic, mc_buffer_size_u_v_h);
	data32 = READ_VREG(HEVC_SAO_CTRL0);
	data32 &= (~0xf);
	data32 |= avs3_dec->lcu_size_log2;
	WRITE_VREG(HEVC_SAO_CTRL0, data32);
#if 0
#ifdef LOSLESS_COMPRESS_MODE
	if ((get_double_write_mode(dec) & 0x10) == 0)
		WRITE_VREG(HEVC_CM_BODY_START_ADDR, pic->mc_y_adr);

	if ((get_double_write_mode(dec) & 0x10) == 0)
		WRITE_VREG(HEVC_CM_BODY_START_ADDR, pic->mc_y_adr);
	if ((get_double_write_mode(dec) & 0x20) == 0) {
		WRITE_VREG(HEVC_SAO_Y_START_ADDR, pic->dw_y_adr);
		WRITE_VREG(HEVC_SAO_C_START_ADDR, pic->dw_u_v_adr);
		WRITE_VREG(HEVC_SAO_Y_WPTR, pic->dw_y_adr);
		WRITE_VREG(HEVC_SAO_C_WPTR, pic->dw_u_v_adr);
	} else {
		WRITE_VREG(HEVC_SAO_Y_START_ADDR, 0xffffffff);
		WRITE_VREG(HEVC_SAO_C_START_ADDR, 0xffffffff);
	}
	WRITE_VREG(HEVC_CM_HEADER_START_ADDR, pic->header_adr);
	if (dec->dw_mmu_enable) {
		WRITE_VREG(HEVC_CM_HEADER_START_ADDR2, pic->dw_header_adr);
		WRITE_VREG(HEVC_SAO_Y_START_ADDR, 0);
		WRITE_VREG(HEVC_SAO_C_START_ADDR, 0);
	}

	//WRITE_VREG(HEVC_SAO_Y_START_ADDR, DOUBLE_WRITE_YSTART_TEMP);
	//WRITE_VREG(HEVC_CM_BODY_START_ADDR, pic->mc_y_adr);
#ifdef AVS3_10B_MMU
	//WRITE_VREG(HEVC_CM_HEADER_START_ADDR, avs3_dec->cm_header_start + (pic->index * get_compress_header_size(dec)));
#endif
#ifdef AVS3_10B_MMU_DW
	//if (dec->dw_mmu_enable) {
	//    WRITE_VREG(HEVC_CM_HEADER_START_ADDR2, pic->header_dw_adr);
	//}
#endif

#else
	WRITE_VREG(HEVC_SAO_Y_START_ADDR, pic->mc_y_adr);
#endif

	data32 = (mc_buffer_size_u_v_h<<16)<<1;
	WRITE_VREG(HEVC_SAO_Y_LENGTH ,data32);

	//WRITE_VREG(HEVC_SAO_C_START_ADDR,DOUBLE_WRITE_CSTART_TEMP);

	data32 = (mc_buffer_size_u_v_h<<16);
	WRITE_VREG(HEVC_SAO_C_LENGTH  ,data32);
	//WRITE_VREG(HEVC_SAO_Y_WPTR ,DOUBLE_WRITE_YSTART_TEMP);
	//WRITE_VREG(HEVC_SAO_C_WPTR ,DOUBLE_WRITE_CSTART_TEMP);
#endif

#ifdef AVS3_10B_NV21
	data32 = READ_VREG(HEVC_SAO_CTRL1);
	data32 &= (~0x3000);
	data32 |= (MEM_MAP_MODE << 12); // [13:12] axi_aformat, 0-Linear, 1-32x32, 2-64x32
	data32 &= (~0x3);
	data32 |= 0x1; // [1]:dw_disable [0]:cm_disable
	WRITE_VREG(HEVC_SAO_CTRL1, data32);

	data32 = READ_VREG(HEVC_SAO_CTRL5); // [23:22] dw_v1_ctrl [21:20] dw_v0_ctrl [19:18] dw_h1_ctrl [17:16] dw_h0_ctrl
	data32 &= ~(0xff << 16);               // set them all 0 for H265_NV21 (no down-scale)
	WRITE_VREG(HEVC_SAO_CTRL5, data32);

	data32 = READ_VREG(HEVCD_IPP_AXIIF_CONFIG);
	data32 &= (~0x30);
	data32 |= (MEM_MAP_MODE << 4); // [5:4]    -- address_format 00:linear 01:32x32 10:64x32
	WRITE_VREG(HEVCD_IPP_AXIIF_CONFIG, data32);
#endif

#ifndef AVS3_10B_NV21
#ifdef AVS3_10B_MMU_DW
	if (dec->dw_mmu_enable) {
		WRITE_VREG(HEVC_DW_VH0_ADDDR, DOUBLE_WRITE_VH0_TEMP);
		WRITE_VREG(HEVC_DW_VH1_ADDDR, DOUBLE_WRITE_VH1_TEMP);
	}
#endif
#endif

	avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
		"[c] cfgSAO .done.\n");
}

static void config_alf_hw(struct AVS3Decoder_s *dec)
{
	/*
	* Picture level ALF parameter configuration here
	*/
	struct avs3_decoder *avs3_dec = &dec->avs3_dec;
	uint32_t data32;
	int32_t i,j;
	int32_t m_filters_per_group;
#ifdef USE_FORCED_ALF_PARAM
	ALFParam forced_alf_cr;
	forced_alf_cr.alf_flag = 1;
	forced_alf_cr.num_coeff = 9;
	forced_alf_cr.filters_per_group = 1;
	forced_alf_cr.component_id = 2;
	forced_alf_cr.coeff_multi[0][0] = -3;
	forced_alf_cr.coeff_multi[0][1] = -3;
	forced_alf_cr.coeff_multi[0][2] = 4;
	forced_alf_cr.coeff_multi[0][3] = 7;
	forced_alf_cr.coeff_multi[0][4] = 6;
	forced_alf_cr.coeff_multi[0][5] = -1;
	forced_alf_cr.coeff_multi[0][6] = 3;
	forced_alf_cr.coeff_multi[0][7] = 6;
	forced_alf_cr.coeff_multi[0][8] = 0;
#endif

	ALFParam *m_alfPictureParam_y = &avs3_dec->m_alfPictureParam[0];
	ALFParam *m_alfPictureParam_cb = &avs3_dec->m_alfPictureParam[1];
#ifdef USE_FORCED_ALF_PARAM
	ALFParam *m_alfPictureParam_cr = &forced_alf_cr; // &avs3_dec->m_alfPictureParam[2];
#else
	ALFParam *m_alfPictureParam_cr = &avs3_dec->m_alfPictureParam[2];
#endif

	printf("[t]alfy,cidx(%d),flag(%d),filters_per_group(%d),filter_pattern[0]=0x%x,[15]=0x%x\n",
			m_alfPictureParam_y->component_id,
			m_alfPictureParam_y->alf_flag,
			m_alfPictureParam_y->filters_per_group,
			m_alfPictureParam_y->filter_pattern[0],m_alfPictureParam_y->filter_pattern[15]);
	printf("[t]alfy,num_coeff(%d),coeff_multi[0][0]=0x%x,[0][1]=0x%x,[1][0]=0x%x,[1][1]=0x%x\n",
			m_alfPictureParam_y->num_coeff,
			m_alfPictureParam_y->coeff_multi[0][0],
			m_alfPictureParam_y->coeff_multi[0][1],
			m_alfPictureParam_y->coeff_multi[1][0],
			m_alfPictureParam_y->coeff_multi[1][1]);

	// Cr
	for (i=0;i<16;i++) dec->m_varIndTab[i] = 0;
	for (j=0;j<16;j++) for (i=0;i<9;i++) dec->m_filterCoeffSym[j][i] = 0;
	reconstructCoefInfo(dec, 2, m_alfPictureParam_cr);
	data32 = ((dec->m_filterCoeffSym[0][4] & 0xf ) << 28) |
		((dec->m_filterCoeffSym[0][3] & 0x7f) << 21) |
		((dec->m_filterCoeffSym[0][2] & 0x7f) << 14) |
		((dec->m_filterCoeffSym[0][1] & 0x7f) <<  7) |
		((dec->m_filterCoeffSym[0][0] & 0x7f) <<  0);
	WRITE_VREG(HEVC_DBLK_CFGD, data32);
	data32 = ((dec->m_filterCoeffSym[0][8] & 0x7f) << 24) |
		((dec->m_filterCoeffSym[0][7] & 0x7f) << 17) |
		((dec->m_filterCoeffSym[0][6] & 0x7f) << 10) |
		((dec->m_filterCoeffSym[0][5] & 0x7f) <<  3) |
		(((dec->m_filterCoeffSym[0][4]>>4) & 0x7 ) <<  0);
	WRITE_VREG(HEVC_DBLK_CFGD, data32);
	avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
		"[c] pic_alf_on_cr(%d), alf_cr_coef(%d %d %d %d %d %d %d %d %d)\n", m_alfPictureParam_cr->alf_flag,
				                                dec->m_filterCoeffSym[0][0],dec->m_filterCoeffSym[0][1],dec->m_filterCoeffSym[0][2],
				                                dec->m_filterCoeffSym[0][3],dec->m_filterCoeffSym[0][4],dec->m_filterCoeffSym[0][5],
				                                dec->m_filterCoeffSym[0][6],dec->m_filterCoeffSym[0][7],dec->m_filterCoeffSym[0][8]);

	// Cb
	for (j=0;j<16;j++) for (i=0;i<9;i++) dec->m_filterCoeffSym[j][i] = 0;
	reconstructCoefInfo(dec, 1, m_alfPictureParam_cb);
	data32 = ((dec->m_filterCoeffSym[0][4] & 0xf ) << 28) |
		((dec->m_filterCoeffSym[0][3] & 0x7f) << 21) |
		((dec->m_filterCoeffSym[0][2] & 0x7f) << 14) |
		((dec->m_filterCoeffSym[0][1] & 0x7f) <<  7) |
		((dec->m_filterCoeffSym[0][0] & 0x7f) <<  0);
	WRITE_VREG(HEVC_DBLK_CFGD, data32);
	data32 = ((dec->m_filterCoeffSym[0][8] & 0x7f) << 24) |
		((dec->m_filterCoeffSym[0][7] & 0x7f) << 17) |
		((dec->m_filterCoeffSym[0][6] & 0x7f) << 10) |
		((dec->m_filterCoeffSym[0][5] & 0x7f) <<  3) |
		(((dec->m_filterCoeffSym[0][4]>>4) & 0x7 ) <<  0);
	WRITE_VREG(HEVC_DBLK_CFGD, data32);
	avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
		"[c] pic_alf_on_cb(%d), alf_cb_coef(%d %d %d %d %d %d %d %d %d)\n", m_alfPictureParam_cb->alf_flag,
				                                dec->m_filterCoeffSym[0][0],dec->m_filterCoeffSym[0][1],dec->m_filterCoeffSym[0][2],
				                                dec->m_filterCoeffSym[0][3],dec->m_filterCoeffSym[0][4],dec->m_filterCoeffSym[0][5],
				                                dec->m_filterCoeffSym[0][6],dec->m_filterCoeffSym[0][7],dec->m_filterCoeffSym[0][8]);

	// Y
	for (j=0;j<16;j++) for (i=0;i<9;i++) dec->m_filterCoeffSym[j][i] = 0;
	reconstructCoefInfo(dec, 0, m_alfPictureParam_y);
	data32 = ((dec->m_varIndTab[7] & 0xf) << 28) | ((dec->m_varIndTab[6] & 0xf) << 24) |
		((dec->m_varIndTab[5] & 0xf) << 20) | ((dec->m_varIndTab[4] & 0xf) << 16) |
		((dec->m_varIndTab[3] & 0xf) << 12) | ((dec->m_varIndTab[2] & 0xf) <<  8) |
		((dec->m_varIndTab[1] & 0xf) <<  4) | ((dec->m_varIndTab[0] & 0xf) <<  0);
	WRITE_VREG(HEVC_DBLK_CFGD, data32);
	data32 = ((dec->m_varIndTab[15] & 0xf) << 28) | ((dec->m_varIndTab[14] & 0xf) << 24) |
		((dec->m_varIndTab[13] & 0xf) << 20) | ((dec->m_varIndTab[12] & 0xf) << 16) |
		((dec->m_varIndTab[11] & 0xf) << 12) | ((dec->m_varIndTab[10] & 0xf) <<  8) |
		((dec->m_varIndTab[ 9] & 0xf) <<  4) | ((dec->m_varIndTab[ 8] & 0xf) <<  0);
	WRITE_VREG(HEVC_DBLK_CFGD, data32);
	avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
		"[c] pic_alf_on_y(%d), alf_y_tab(%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d)\n", m_alfPictureParam_y->alf_flag,
				                                dec->m_varIndTab[ 0],dec->m_varIndTab[ 1],dec->m_varIndTab[ 2],dec->m_varIndTab[ 3],
				                                dec->m_varIndTab[ 4],dec->m_varIndTab[ 5],dec->m_varIndTab[ 6],dec->m_varIndTab[ 7],
				                                dec->m_varIndTab[ 8],dec->m_varIndTab[ 9],dec->m_varIndTab[10],dec->m_varIndTab[11],
				                                dec->m_varIndTab[12],dec->m_varIndTab[13],dec->m_varIndTab[14],dec->m_varIndTab[15]);

	m_filters_per_group = (m_alfPictureParam_y->alf_flag == 0) ? 1 : m_alfPictureParam_y->filters_per_group;
	for (i=0;i<m_filters_per_group;i++) {
		data32 = ((dec->m_filterCoeffSym[i][4] & 0xf ) << 28) |
			((dec->m_filterCoeffSym[i][3] & 0x7f) << 21) |
			((dec->m_filterCoeffSym[i][2] & 0x7f) << 14) |
			((dec->m_filterCoeffSym[i][1] & 0x7f) <<  7) |
			((dec->m_filterCoeffSym[i][0] & 0x7f) <<  0);
		WRITE_VREG(HEVC_DBLK_CFGD, data32);
		data32 = ((i == m_filters_per_group-1) << 31) | // [31] last indication
			((dec->m_filterCoeffSym[i][8] & 0x7f) << 24) |
			((dec->m_filterCoeffSym[i][7] & 0x7f) << 17) |
			((dec->m_filterCoeffSym[i][6] & 0x7f) << 10) |
			((dec->m_filterCoeffSym[i][5] & 0x7f) <<  3) |
			(((dec->m_filterCoeffSym[i][4]>>4) & 0x7 ) <<  0);
		WRITE_VREG(HEVC_DBLK_CFGD, data32);
		avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
			"[c] alf_y_coef[%d](%d %d %d %d %d %d %d %d %d)\n",i,dec->m_filterCoeffSym[i][0],dec->m_filterCoeffSym[i][1],dec->m_filterCoeffSym[i][2],
				                                        dec->m_filterCoeffSym[i][3],dec->m_filterCoeffSym[i][4],dec->m_filterCoeffSym[i][5],
				                                        dec->m_filterCoeffSym[i][6],dec->m_filterCoeffSym[i][7],dec->m_filterCoeffSym[i][8]);
	}
	avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
		"[c] cfgALF .done.\n");
}

static u32 init_cuva_size;

static int cuva_data_is_available(struct AVS3Decoder_s *dec, u32 reg_val)
{

	avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
		"%s:reg_val: %u \n",
		__func__, reg_val);
	if (reg_val != 0 && reg_val != init_cuva_size)
		return 1;
	else
		return 0;
}

void config_cuva_buf(struct AVS3Decoder_s *dec)
{
	WRITE_VREG(AVS3_CUVA_ADR, dec->cuva_phy_addr);
	init_cuva_size = (dec->cuva_size >> 4) << 16;
	WRITE_VREG(AVS3_CUVA_DATA_SIZE, init_cuva_size);
}

static void set_cuva_data(struct AVS3Decoder_s *dec)
{
	int i;
	unsigned short *cuva_adr;
	unsigned int size_reg_val =
		READ_VREG(AVS3_CUVA_DATA_SIZE);
	unsigned int cuva_count = 0;
	int cuva_size = 0;
	struct avs3_frame_s *pic = dec->avs3_dec.cur_pic;
	if (pic == NULL || 0 == cuva_data_is_available(dec, size_reg_val)) {
		avs3_print(dec, AVS3_DBG_HDR_INFO,
		"%s:pic 0x%p or data not available\n",
		__func__, pic);
		return;
	}

	cuva_adr = (unsigned short *)dec->cuva_addr;
	cuva_count = ((size_reg_val >> 16) << 4) >> 1;
	cuva_size = dec->cuva_size;

	avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
			"%s:pic 0x%p cuva_count(%d) cuva_size(%d) hdr_flag 0x%x\n",
			__func__, pic, cuva_count, cuva_size, dec->hdr_flag);
	if (cuva_size > 0 && cuva_count > 0) {
		if (pic->cuva_data_buf) {
			unsigned char *p = pic->cuva_data_buf;
			int len = 0;

			for (i = 0; i < cuva_count; i += 4) {
				int j;

				for (j = 0; j < 4; j++) {
					unsigned short aa = cuva_adr[i + 3 - j];
					*p = aa & 0xff;
					p++;
					len++;
				}
			}
			if (len > 0) {
				pic->cuva_data_size = len;
			}

			if (pic->cuva_data_buf[0] == 0x26
				&& pic->cuva_data_buf[1] == 0x00
				&& pic->cuva_data_buf[2] == 0x04
				&& pic->cuva_data_buf[3] == 0x00
				&& pic->cuva_data_buf[4] == 0x05) {
				dec->hdr_flag |= HDR_CUVA_MASK;
				avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
					"cuva stream: (size %d)\n", pic->cuva_data_size);
			} else {
				avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
					" other hdr stream (size %d)\n", pic->cuva_data_size);
			}

			if (get_dbg_flag(dec) & AVS3_DBG_HDR_INFO) {
				for (i = 0; i < pic->cuva_data_size; i++) {
					pr_info("%02x ", pic->cuva_data_buf[i]);
					if (((i + 1) & 0xf) == 0)
						pr_info("\n");
				}
				pr_info("\n");
			}

		} else {
			avs3_print(dec, 0, "cuva_data_buf NULL\n");
		}
	}
}

static void release_cuva_data(struct avs3_frame_s *pic)
{
	pic->cuva_data_size = 0;
}

static void avs3_config_work_space_hw(struct AVS3Decoder_s *dec)
{
	DEC_CTX * ctx = &dec->avs3_dec.ctx;
	struct BuffInfo_s *buf_spec = dec->work_space_buf;
	u32 width = dec->avs3_dec.img.width ? dec->avs3_dec.img.width : dec->init_pic_w;
	u32 height = dec->avs3_dec.img.height ? dec->avs3_dec.img.height : dec->init_pic_h;
	u8 is_bit_depth_10 = (ctx->info.bit_depth_internal == 8) ? 0 : 1;

#ifdef LOSLESS_COMPRESS_MODE
	int losless_comp_header_size = compute_losless_comp_header_size(
		dec, width, height);
	int losless_comp_body_size = compute_losless_comp_body_size(
		dec, width, height, is_bit_depth_10);
#endif
#ifdef AVS3_10B_MMU
	unsigned int data32;
#endif
	if (debug && dec->init_flag == 0)
		avs3_print(dec, 0,
			"%s %x %x %x %x %x %x %x %x %x %x %x %x %x\n",
			__func__,
			buf_spec->ipp.buf_start,
			buf_spec->start_adr,
			buf_spec->short_term_rps.buf_start,
			buf_spec->rcs.buf_start,
			buf_spec->sps.buf_start,
			buf_spec->pps.buf_start,
			buf_spec->sao_up.buf_start,
			buf_spec->swap_buf.buf_start,
			buf_spec->swap_buf2.buf_start,
			buf_spec->scalelut.buf_start,
			buf_spec->dblk_para.buf_start,
			buf_spec->dblk_data.buf_start,
			buf_spec->dblk_data2.buf_start);
	WRITE_VREG(HEVCD_IPP_LINEBUFF_BASE, buf_spec->ipp.buf_start);
	if ((debug & AVS3_DBG_SEND_PARAM_WITH_REG) == 0)
		WRITE_VREG(HEVC_RPM_BUFFER, (u32)dec->rpm_phy_addr);
	WRITE_VREG(AVS3_ALF_SWAP_BUFFER, buf_spec->short_term_rps.buf_start);
	WRITE_VREG(HEVC_RCS_BUFFER, buf_spec->rcs.buf_start);
	//WRITE_VREG(HEVC_SPS_BUFFER, buf_spec->sps.buf_start);
	//WRITE_VREG(HEVC_PPS_BUFFER, buf_spec->pps.buf_start);
	WRITE_VREG(AVS3_SBAC_TOP_BUFFER, buf_spec->sbac_top.buf_start);
	//WRITE_VREG(HEVC_SAO_UP, buf_spec->sao_up.buf_start);
#ifdef AVS3_10B_MMU
	WRITE_VREG(AVS3_MMU_MAP_BUFFER, dec->frame_mmu_map_phy_addr);
#else
	WRITE_VREG(HEVC_STREAM_SWAP_BUFFER, buf_spec->swap_buf.buf_start);
#endif
#ifdef AVS3_10B_MMU_DW
	if (dec->dw_mmu_enable) {
		//WRITE_VREG(HEVC_ASSIST_MMU_MAP_ADDR2, FRAME_MMU_MAP_ADDR_DW);
		WRITE_VREG(HEVC_SAO_MMU_DMA_CTRL2, dec->dw_frame_mmu_map_phy_addr);
	}
#endif
	WRITE_VREG(HEVC_STREAM_SWAP_BUFFER2, buf_spec->swap_buf2.buf_start);
	WRITE_VREG(HEVC_SCALELUT, buf_spec->scalelut.buf_start);
#ifndef FOR_S5
	if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_SM1) {
		if (buf_spec->max_width <= 4096 && buf_spec->max_height <= 2304)
			WRITE_VREG(HEVC_DBLK_CFG3, 0x404010); //default value
		else
			WRITE_VREG(HEVC_DBLK_CFG3, 0x808020); // make left storage 2 x 4k]
		avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
			"HEVC_DBLK_CFG3 = %x\n", READ_VREG(HEVC_DBLK_CFG3));
	}
#endif
	/* cfg_p_addr */
	WRITE_VREG(HEVC_DBLK_CFG4, buf_spec->dblk_para.buf_start);
	/* cfg_d_addr */
	WRITE_VREG(HEVC_DBLK_CFG5, buf_spec->dblk_data.buf_start);

	WRITE_VREG(HEVC_DBLK_CFGE, buf_spec->dblk_data2.buf_start);

#ifdef LOSLESS_COMPRESS_MODE
	data32 = READ_VREG(HEVC_SAO_CTRL5);
#if 1
	data32 &= ~(1<<9);
#else
	if (params->p.bit_depth != 0x00)
		data32 &= ~(1<<9);
	else
		data32 |= (1<<9);
#endif
	WRITE_VREG(HEVC_SAO_CTRL5, data32);
#ifdef AVS3_10B_MMU
	/*bit[4] : paged_mem_mode*/
	WRITE_VREG(HEVCD_MPP_DECOMP_CTL1, (0x1 << 4));
	WRITE_VREG(HEVCD_MPP_DECOMP_CTL2, 0);
#else
	/* bit[3] smem mode*/
	WRITE_VREG(HEVCD_MPP_DECOMP_CTL1, (0<<3));

	WRITE_VREG(HEVCD_MPP_DECOMP_CTL2, (losless_comp_body_size >> 5));
#endif
	/*WRITE_VREG(HEVCD_MPP_DECOMP_CTL2,(losless_comp_body_size >> 5));*/
	/*WRITE_VREG(HEVCD_MPP_DECOMP_CTL3,(0xff<<20) | (0xff<<10) | 0xff);*/
/*8-bit mode */
	WRITE_VREG(HEVC_CM_BODY_LENGTH, losless_comp_body_size);
	WRITE_VREG(HEVC_CM_HEADER_OFFSET, losless_comp_body_size);
	WRITE_VREG(HEVC_CM_HEADER_LENGTH, losless_comp_header_size);
#else
#ifdef P010_ENABLE
	if (get_double_write_mode(dec) & 0x10) {
		if (is_dw_p010(dec)) {
			/* Enable P010 reference read mode for MC */
			WRITE_VREG(HEVCD_MPP_DECOMP_CTL1,
				(0x1 << 31) | (1 << 24) | (((dec->endian >> 12) & 0xff) << 16));
		} else {
			/* Enable NV21 reference read mode for MC */
			WRITE_VREG(HEVCD_MPP_DECOMP_CTL1, 0x1 << 31);
		}
	}
#endif
#endif

#ifdef AVS3_10B_MMU
	WRITE_VREG(HEVC_SAO_MMU_VH0_ADDR, buf_spec->mmu_vbh.buf_start);
	WRITE_VREG(HEVC_SAO_MMU_VH1_ADDR, buf_spec->mmu_vbh.buf_start
			+ VBH_BUF_SIZE(buf_spec));
	/*data32 = READ_VREG(HEVC_SAO_CTRL9);*/
	/*data32 |= 0x1;*/
	/*WRITE_VREG(HEVC_SAO_CTRL9, data32);*/

	/* use HEVC_CM_HEADER_START_ADDR */
	data32 = READ_VREG(HEVC_SAO_CTRL5);
	data32 |= (1<<10);
#if 0
	if (debug & AVS3_DBG_FORCE_UNCOMPRESS)
		data32 |= 0x80;
#endif
	WRITE_VREG(HEVC_SAO_CTRL5, data32);

#endif

#ifdef AVS3_10B_MMU_DW
	if (dec->dw_mmu_enable) {
		u32 data_tmp;
		data_tmp = READ_VREG(HEVC_SAO_CTRL9);
		data_tmp |= (1<<10);
		WRITE_VREG(HEVC_SAO_CTRL9, data_tmp);

		WRITE_VREG(HEVC_CM_BODY_LENGTH2,losless_comp_body_size);
		WRITE_VREG(HEVC_CM_HEADER_OFFSET2,losless_comp_body_size);
		WRITE_VREG(HEVC_CM_HEADER_LENGTH2,losless_comp_header_size);

		WRITE_VREG(HEVC_SAO_MMU_VH0_ADDR2, buf_spec->mmu_vbh_dw.buf_start);
		WRITE_VREG(HEVC_SAO_MMU_VH1_ADDR2, buf_spec->mmu_vbh_dw.buf_start + DW_VBH_BUF_SIZE(buf_spec));

		/* use HEVC_CM_HEADER_START_ADDR */
		data32 = READ_VREG(HEVC_SAO_CTRL5);
		data32 |= (1<<15);
		WRITE_VREG(HEVC_SAO_CTRL5, data32);
	}
#endif

	WRITE_VREG(LMEM_DUMP_ADR, (u32)dec->lmem_phy_addr);

	WRITE_VREG(HEVC_MPRED_ABV_START_ADDR, buf_spec->mpred_above.buf_start);

#ifdef CO_MV_COMPRESS
	//if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_T7) {
		data32 = READ_VREG(HEVC_MPRED_CTRL4);
		data32 |=  (1 << 1);
		WRITE_VREG(HEVC_MPRED_CTRL4, data32);
	//}
#endif
	data32 = READ_VREG(HEVC_MPRED_CTRL4);
	data32 |=  (1<<26); //enable AVS3 mode
	WRITE_VREG(HEVC_MPRED_CTRL4, data32);
}

static void decomp_perfcount_reset(void)
{
	if (debug & AVS3_DBG_CACHE)
		pr_info("[cache_util.c] Entered decomp_perfcount_reset...\n");
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL, (unsigned int)0x1);
	WRITE_VREG(HEVCD_MPP_DECOMP_PERFMON_CTL, (unsigned int)0x0);
	return;
}

static void mcrcc_perfcount_reset(void)
{
	if (debug & AVS3_DBG_CACHE)
		pr_info("[cache_util.c] Entered mcrcc_perfcount_reset...\n");
	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL, (unsigned int)0x1);
	WRITE_VREG(HEVCD_MCRCC_PERFMON_CTL, (unsigned int)0x0);
	return;
}

#ifdef TMP_DEBUG

static void avs2_init_decoder_hw(struct AVS3Decoder_s *dec)
{
	unsigned int data32;
	unsigned int decode_mode;
	int i;

	data32 = READ_VREG(HEVC_PARSER_INT_CONTROL);

	/* set bit 31~29 to 3 if HEVC_STREAM_FIFO_CTL[29] is 1 */
	data32 &= ~(7 << 29);
	data32 |= (3 << 29);

	data32 = data32 |
		(1 << 24) |/*stream_buffer_empty_int_amrisc_enable*/
		(1 << 22) |/*stream_fifo_empty_int_amrisc_enable*/
		(1 << 7) |/*dec_done_int_cpu_enable*/
		(1 << 4) |/*startcode_found_int_cpu_enable*/
		(0 << 3) |/*startcode_found_int_amrisc_enable*/
		(1 << 0);    /*parser_int_enable*/

	WRITE_VREG(HEVC_PARSER_INT_CONTROL, data32);

	data32 = READ_VREG(HEVC_SHIFT_STATUS);
	data32 = data32 |
		(0 << 1) |/*emulation_check_off VP9
		do not have emulation*/
		(1 << 0);/*startcode_check_on*/

	WRITE_VREG(HEVC_SHIFT_STATUS, data32);
	WRITE_VREG(HEVC_SHIFT_CONTROL,
		(6 << 20) | /* emu_push_bits  (6-bits for AVS2)*/
		(0 << 19) | /* emu_3_enable, maybe turned on in microcode*/
		(0 << 18) | /* emu_2_enable, maybe turned on in microcode*/
		(0 << 17) | /* emu_1_enable, maybe turned on in microcode*/
		(0 << 16) | /* emu_0_enable, maybe turned on in microcode*/
		(0 << 14) | /*disable_start_code_protect*/
		(3 << 6) | /* sft_valid_wr_position*/
		(2 << 4) | /* emulate_code_length_sub_1*/
		(2 << 1) | /* start_code_length_sub_1*/
		(1 << 0));   /* stream_shift_enable*/

	WRITE_VREG(HEVC_SHIFT_LENGTH_PROTECT,
		(0 << 30) |   /*data_protect_fill_00_enable*/
		(1 << 29));     /*data_protect_fill_ff_enable*/

	WRITE_VREG(HEVC_CABAC_CONTROL, (1 << 0));/*cabac_enable*/

	WRITE_VREG(HEVC_PARSER_CORE_CONTROL, (1 << 0));/* hevc_parser_core_clk_en*/

	WRITE_VREG(HEVC_DEC_STATUS_REG, 0);

	/*Initial IQIT_SCALELUT memory -- just to avoid X in simulation*/
	if (is_rdma_enable())
		rdma_back_end_work(dec->rdma_phy_adr, RDMA_SIZE);
	else {
		WRITE_VREG(HEVC_IQIT_SCALELUT_WR_ADDR, 0);/*cfg_p_addr*/
		for (i = 0; i < 1024; i++)
			WRITE_VREG(HEVC_IQIT_SCALELUT_DATA, 0);
	}

#ifdef ENABLE_SWAP_TEST
	WRITE_VREG(HEVC_STREAM_SWAP_TEST, 100);
#else
	WRITE_VREG(HEVC_STREAM_SWAP_TEST, 0);
#endif
	if (!dec->m_ins_flag)
		decode_mode = DECODE_MODE_SINGLE;
	else if (vdec_frame_based(hw_to_vdec(dec)))
		decode_mode = DECODE_MODE_MULTI_FRAMEBASE;
	else
		decode_mode = DECODE_MODE_MULTI_STREAMBASE;
	dec->start_decoding_flag = 3;
	decode_mode |= (dec->start_decoding_flag << 16);

	WRITE_VREG(DECODE_MODE, decode_mode);
	WRITE_VREG(HEVC_DECODE_SIZE, 0);
	WRITE_VREG(HEVC_DECODE_COUNT, 0);
#ifdef DYN_CACHE
	if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_S5) {
			avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL, "HEVC DYN MCRCC\n");
		WRITE_VREG(HEVCD_IPP_DYN_CACHE,0x2b);//enable new mcrcc
	}
#endif

	/*Send parser_cmd*/
	WRITE_VREG(HEVC_PARSER_CMD_WRITE, (1 << 16) | (0 << 0));
	for (i = 0; i < PARSER_CMD_NUMBER; i++)
		WRITE_VREG(HEVC_PARSER_CMD_WRITE, parser_cmd[i]);
	WRITE_VREG(HEVC_PARSER_CMD_SKIP_0, PARSER_CMD_SKIP_CFG_0);
	WRITE_VREG(HEVC_PARSER_CMD_SKIP_1, PARSER_CMD_SKIP_CFG_1);
	WRITE_VREG(HEVC_PARSER_CMD_SKIP_2, PARSER_CMD_SKIP_CFG_2);

	WRITE_VREG(HEVC_PARSER_IF_CONTROL,
		(1 << 9) | /* parser_alf_if_en*/
		/*  (1 << 8) |*/ /*sao_sw_pred_enable*/
		(1 << 5) | /*parser_sao_if_en*/
		(1 << 2) | /*parser_mpred_if_en*/
		(1 << 0) /*parser_scaler_if_en*/
	);

#ifdef MULTI_INSTANCE_SUPPORT
	WRITE_VREG(HEVC_MPRED_INT_STATUS, (1<<31));

	WRITE_VREG(HEVC_PARSER_RESULT_3, 0xffffffff);

	for (i = 0; i < 8; i++)
		data32 = READ_VREG(HEVC_MPRED_ABV_START_ADDR);

	WRITE_VREG(DOS_SW_RESET3, (1<<18)); /* reset mpred */
	WRITE_VREG(DOS_SW_RESET3, 0);
	WRITE_VREG(HEVC_MPRED_ABV_START_ADDR, data32);
	WRITE_VREG(HEVC_MPRED_ABV_START_ADDR, data32);
	WRITE_VREG(HEVC_MPRED_ABV_START_ADDR, data32);
#endif

	return;
}

#else
static
#endif
void avs3_init_decoder_hw(struct AVS3Decoder_s *dec)
{
	unsigned int data32;
	unsigned int decode_mode;
	int i;
	/*if (debug & AVS3_DBG_BUFMGR_MORE)
		pr_info("%s\n", __func__);*/
		data32 = READ_VREG(HEVC_PARSER_INT_CONTROL);
#if 1
		/* set bit 31~29 to 3 if HEVC_STREAM_FIFO_CTL[29] is 1 */
		data32 &= ~(7 << 29);
		data32 |= (3 << 29);
#endif
		data32 = data32 |
		(1 << 24) |/*stream_buffer_empty_int_amrisc_enable*/
		(1 << 22) |/*stream_fifo_empty_int_amrisc_enable*/
		(1 << 7) |/*dec_done_int_cpu_enable*/
		(1 << 4) |/*startcode_found_int_cpu_enable*/
		(0 << 3) |/*startcode_found_int_amrisc_enable*/
		(1 << 0)    /*parser_int_enable*/
		;
	WRITE_VREG(HEVC_PARSER_INT_CONTROL, data32);

	data32 = READ_VREG(HEVC_SHIFT_STATUS);
	data32 = data32 |
	(0 << 1) |/*emulation_check_off VP9
		do not have emulation*/
	(1 << 0)/*startcode_check_on*/
	;
	WRITE_VREG(HEVC_SHIFT_STATUS, data32);
	WRITE_VREG(HEVC_SHIFT_CONTROL,
		(6 << 20) | /* emu_push_bits  (6-bits for AVS3)*/
		(0 << 19) | /* emu_3_enable, maybe turned on in microcode*/
		(0 << 18) | /* emu_2_enable, maybe turned on in microcode*/
		(0 << 17) | /* emu_1_enable, maybe turned on in microcode*/
		(0 << 16) | /* emu_0_enable, maybe turned on in microcode*/
		(0 << 14) | /*disable_start_code_protect*/
		(3 << 6) | /* sft_valid_wr_position*/
		(2 << 4) | /* emulate_code_length_sub_1*/
		(2 << 1) | /* start_code_length_sub_1*/
		(1 << 0)   /* stream_shift_enable*/
		);

	WRITE_VREG(HEVC_SHIFT_LENGTH_PROTECT,
		(0 << 30) |   /*data_protect_fill_00_enable*/
		(1 << 29)     /*data_protect_fill_ff_enable*/
		);
	WRITE_VREG(HEVC_CABAC_CONTROL,
		(1 << 0)/*cabac_enable*/
	);

	WRITE_VREG(HEVC_PARSER_CORE_CONTROL,
		(1 << 0)/* hevc_parser_core_clk_en*/
	);

	WRITE_VREG(HEVC_DEC_STATUS_REG, 0);

	/*Initial IQIT_SCALELUT memory -- just to avoid X in simulation*/
	if (is_rdma_enable())
		rdma_back_end_work(dec->rdma_phy_adr, RDMA_SIZE);
	else {
		if ((debug & AVS3_DBG_DISABLE_IQIT_SCALELUT_INIT) == 0) {
			WRITE_VREG(HEVC_IQIT_SCALELUT_WR_ADDR, 0);/*cfg_p_addr*/
			for (i = 0; i < 1024; i++)
				WRITE_VREG(HEVC_IQIT_SCALELUT_DATA, 0);
		}
	}

#ifdef ENABLE_SWAP_TEST
	WRITE_VREG(HEVC_STREAM_SWAP_TEST, 100);
#else
	WRITE_VREG(HEVC_STREAM_SWAP_TEST, 0);
#endif
	if (!dec->m_ins_flag)
		decode_mode = DECODE_MODE_SINGLE;
	else if (vdec_frame_based(hw_to_vdec(dec)))
		decode_mode = DECODE_MODE_MULTI_FRAMEBASE;
	else
		decode_mode = DECODE_MODE_MULTI_STREAMBASE;
	if (dec->avs3_dec.bufmgr_error_flag &&
		(error_handle_policy & 0x1)) {
		dec->bufmgr_error_count++;
		dec->avs3_dec.bufmgr_error_flag = 0;
		if (dec->bufmgr_error_count >
			(re_search_seq_threshold & 0xff)
			&& dec->frame_count >
			((re_search_seq_threshold >> 8) & 0xff)) {
			//struct avs3_decoder *avs3_dec = &dec->avs3_dec;
			dec->start_decoding_flag = 0;
			//avs3_dec->hd.vec_flag = 1;
			dec->skip_PB_before_I = 1;
			avs3_print(dec, 0,
				"!!Bufmgr error, search seq again (0x%x %d %d)\n",
				error_handle_policy,
				dec->frame_count,
				dec->bufmgr_error_count);
			dec->bufmgr_error_count = 0;
		}
	}
#ifdef FOR_S5
	/*to do..*/
	dec->start_decoding_flag = 3;
#endif
	decode_mode |= (dec->start_decoding_flag << 16);
	WRITE_VREG(DECODE_MODE, decode_mode);
	WRITE_VREG(HEVC_DECODE_SIZE, 0);
	WRITE_VREG(HEVC_DECODE_COUNT, 0);
#ifdef DYN_CACHE
	if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_S5) {
			avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL, "HEVC DYN MCRCC\n");
		WRITE_VREG(HEVCD_IPP_DYN_CACHE,0x2b);//enable new mcrcc}
	}
#endif
	/*Send parser_cmd*/
	WRITE_VREG(HEVC_PARSER_CMD_WRITE, (1 << 16) | (0 << 0));
	for (i = 0; i < PARSER_CMD_NUMBER; i++)
		WRITE_VREG(HEVC_PARSER_CMD_WRITE, parser_cmd[i]);
	WRITE_VREG(HEVC_PARSER_CMD_SKIP_0, PARSER_CMD_SKIP_CFG_0);
	WRITE_VREG(HEVC_PARSER_CMD_SKIP_1, PARSER_CMD_SKIP_CFG_1);
	WRITE_VREG(HEVC_PARSER_CMD_SKIP_2, PARSER_CMD_SKIP_CFG_2);

	WRITE_VREG(HEVC_PARSER_IF_CONTROL,
		(1 << 9) | /* parser_alf_if_en*/
		/*  (1 << 8) |*/ /*sao_sw_pred_enable*/
		(1 << 5) | /*parser_sao_if_en*/
		(1 << 2) | /*parser_mpred_if_en*/
		(1 << 0) /*parser_scaler_if_en*/
	);

#if 0 //def MULTI_INSTANCE_SUPPORT
	WRITE_VREG(HEVC_MPRED_INT_STATUS, (1<<31));

	WRITE_VREG(HEVC_PARSER_RESULT_3, 0xffffffff);

	for (i = 0; i < 8; i++)
		data32 = READ_VREG(HEVC_MPRED_ABV_START_ADDR);

	WRITE_VREG(DOS_SW_RESET3, (1<<18)); /* reset mpred */
	WRITE_VREG(DOS_SW_RESET3, 0);
	WRITE_VREG(HEVC_MPRED_ABV_START_ADDR, data32);
	WRITE_VREG(HEVC_MPRED_ABV_START_ADDR, data32);
	WRITE_VREG(HEVC_MPRED_ABV_START_ADDR, data32);
#endif
	/*End of Multi-instance*/
	/*Changed to Start MPRED in microcode*/
	/*
	pr_info("[test.c] Start MPRED\n");
	WRITE_VREG(HEVC_MPRED_INT_STATUS,
	(1<<31)
	);
	*/

	/*AVS3 default seq_wq_matrix config*/

	avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
		"Config AVS3 default seq_wq_matrix ...\n");
	/*4x4*/
		/* default seq_wq_matrix_4x4 begin address*/
	WRITE_VREG(HEVC_IQIT_SCALELUT_WR_ADDR, 64);
	for (i = 0; i < 16; i++)
		WRITE_VREG(HEVC_IQIT_SCALELUT_DATA, g_WqMDefault4x4[i]);

	/*8x8*/
	/*default seq_wq_matrix_8x8 begin address*/
	WRITE_VREG(HEVC_IQIT_SCALELUT_WR_ADDR, 0);
	for (i = 0; i < 64; i++)
		WRITE_VREG(HEVC_IQIT_SCALELUT_DATA, g_WqMDefault8x8[i]);

	WRITE_VREG(HEVCD_IPP_TOP_CNTL,
		(0 << 1) | /*enable ipp*/
		(1 << 0)   /*software reset ipp and mpp*/
	);
	WRITE_VREG(HEVCD_IPP_TOP_CNTL,
		(1 << 1) | /*enable ipp*/
		(0 << 0)   /*software reset ipp and mpp*/
	);
#if 0
/*AVS3_10B_NV21*/
	/*Enable NV21 reference read mode for MC*/
	WRITE_VREG(HEVCD_MPP_DECOMP_CTL1, 0x1 << 31);
#endif
	/* Init dblk*/
	data32 = READ_VREG(HEVC_DBLK_CFGB);
	data32 |= (5 << 0);
	/* [3:0] cfg_video_type -> AVS3*/
#ifndef FOR_S5
	data32 &= (~0x300); /*[8]:first write enable (compress)
					[9]:double write enable (uncompress)*/
	if (get_double_write_mode(dec) == 0)
		data32 |= (0x1 << 8); /*enable first write*/
	else if (get_double_write_mode(dec) == 0x10)
		data32 |= (0x1 << 9); /*double write only*/
	else
		data32 |= ((0x1 << 8) | (0x1 << 9));
#endif
	WRITE_VREG(HEVC_DBLK_CFGB, data32);

	WRITE_VREG(HEVC_DBLK_CFG0, (1<<18) | (1 << 0)); /* [0] rst_sync*/
	avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
		"Bitstream level Init for DBLK .Done.\n");

	if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_SM1) {
		mcrcc_perfcount_reset();
		decomp_perfcount_reset();
	}

	return;
}

#ifdef CONFIG_HEVC_CLK_FORCED_ON
static void config_avs3_clk_forced_on(void)
{
	unsigned int rdata32;
	/*IQIT*/
	rdata32 = READ_VREG(HEVC_IQIT_CLK_RST_CTRL);
	WRITE_VREG(HEVC_IQIT_CLK_RST_CTRL, rdata32 | (0x1 << 2));

	/* DBLK*/
	rdata32 = READ_VREG(HEVC_DBLK_CFG0);
	WRITE_VREG(HEVC_DBLK_CFG0, rdata32 | (0x1 << 2));

	/* SAO*/
	rdata32 = READ_VREG(HEVC_SAO_CTRL1);
	WRITE_VREG(HEVC_SAO_CTRL1, rdata32 | (0x1 << 2));

	/*MPRED*/
	rdata32 = READ_VREG(HEVC_MPRED_CTRL1);
	WRITE_VREG(HEVC_MPRED_CTRL1, rdata32 | (0x1 << 24));

	/* PARSER*/
	rdata32 = READ_VREG(HEVC_STREAM_CONTROL);
	WRITE_VREG(HEVC_STREAM_CONTROL, rdata32 | (0x1 << 15));
	rdata32 = READ_VREG(HEVC_SHIFT_CONTROL);
	WRITE_VREG(HEVC_SHIFT_CONTROL, rdata32 | (0x1 << 15));
	rdata32 = READ_VREG(HEVC_CABAC_CONTROL);
	WRITE_VREG(HEVC_CABAC_CONTROL, rdata32 | (0x1 << 13));
	rdata32 = READ_VREG(HEVC_PARSER_CORE_CONTROL);
	WRITE_VREG(HEVC_PARSER_CORE_CONTROL, rdata32 | (0x1 << 15));
	rdata32 = READ_VREG(HEVC_PARSER_INT_CONTROL);
	WRITE_VREG(HEVC_PARSER_INT_CONTROL, rdata32 | (0x1 << 15));
	rdata32 = READ_VREG(HEVC_PARSER_IF_CONTROL);
	WRITE_VREG(HEVC_PARSER_IF_CONTROL,
			rdata32 | (0x1 << 6) | (0x1 << 3) | (0x1 << 1));

	/*IPP*/
	rdata32 = READ_VREG(HEVCD_IPP_DYNCLKGATE_CONFIG);
	WRITE_VREG(HEVCD_IPP_DYNCLKGATE_CONFIG, rdata32 | 0xffffffff);

	/* MCRCC*/
	rdata32 = READ_VREG(HEVCD_MCRCC_CTL1);
	WRITE_VREG(HEVCD_MCRCC_CTL1, rdata32 | (0x1 << 3));
}
#endif

static struct AVS3Decoder_s gAVS3Decoder;

static void avs3_local_uninit(struct AVS3Decoder_s *dec)
{
	dec->rpm_ptr = NULL;
	dec->lmem_ptr = NULL;
	if (dec->rpm_addr) {
		decoder_dma_free_coherent(dec->rpm_mem_handle,
						RPM_BUF_SIZE, dec->rpm_addr,
						dec->rpm_phy_addr);
		dec->rpm_addr = NULL;
	}

	if (dec->cuva_addr) {
		decoder_dma_free_coherent(dec->cuva_handle,
				dec->cuva_size, dec->cuva_addr,
					dec->cuva_phy_addr);
		dec->cuva_addr = NULL;
	}
#if (defined DEBUG_UCODE_LOG) || (defined DEBUG_CMD)
	if (dec->ucode_log_addr) {
		decoder_dma_free_coherent(dec->ucode_log_handle,
				UCODE_LOG_BUF_SIZE, dec->ucode_log_addr,
					dec->ucode_log_phy_addr);
		dec->ucode_log_addr = NULL;
	}
#endif
	if (dec->lmem_addr) {
			if (dec->lmem_phy_addr)
				decoder_dma_free_coherent(dec->lmem_phy_handle,
						LMEM_BUF_SIZE, dec->lmem_addr,
						dec->lmem_phy_addr);
		dec->lmem_addr = NULL;
	}

#ifdef AVS3_10B_MMU
	if (dec->frame_mmu_map_addr) {
		if (dec->frame_mmu_map_phy_addr)
			decoder_dma_free_coherent(dec->frame_mmu_map_handle,
				get_frame_mmu_map_size(), dec->frame_mmu_map_addr,
					dec->frame_mmu_map_phy_addr);
		dec->frame_mmu_map_addr = NULL;
	}
#ifdef NEW_FB_CODE
	if (dec->front_back_mode) {
		if (dec->frame_mmu_map_phy_addr_1)
			decoder_dma_free_coherent(dec->frame_mmu_map_handle_1,
				get_frame_mmu_map_size(), dec->frame_mmu_map_addr_1,
					dec->frame_mmu_map_phy_addr_1);

		dec->frame_mmu_map_addr_1 = NULL;
	}
#endif
#endif

#ifdef AVS3_10B_MMU_DW
	if (dec->dw_frame_mmu_map_addr) {
		if (dec->dw_frame_mmu_map_phy_addr)
			decoder_dma_free_coherent(dec->frame_dw_mmu_map_handle,
				get_frame_mmu_map_size(), dec->dw_frame_mmu_map_addr,
					dec->dw_frame_mmu_map_phy_addr);
		dec->dw_frame_mmu_map_addr = NULL;
	}
#ifdef NEW_FB_CODE
		if (dec->front_back_mode && dec->dw_frame_mmu_map_addr_1) {
			if (dec->dw_frame_mmu_map_phy_addr_1)
				decoder_dma_free_coherent(dec->frame_dw_mmu_map_handle_1,
					get_frame_mmu_map_size(), dec->dw_frame_mmu_map_addr_1,
						dec->dw_frame_mmu_map_phy_addr_1);

			dec->dw_frame_mmu_map_addr_1 = NULL;
		}
#endif
#endif

	if (dec->gvs)
		vfree(dec->gvs);
	dec->gvs = NULL;
}

static int avs3_local_init(struct AVS3Decoder_s *dec)
{
	int ret = -1;
	/*int losless_comp_header_size, losless_comp_body_size;*/

	struct avs3_decoder *avs3_dec = &dec->avs3_dec;
	struct BuffInfo_s *cur_buf_info = NULL;
	unsigned bufspec_index = 0;
	cur_buf_info = &dec->work_space_buf_store;
	if (force_bufspec) {
		bufspec_index = force_bufspec & 0xf;
		pr_info("force buffer spec %d\n", force_bufspec & 0xf);
	} else {
		if (vdec_is_support_4k()) {
			bufspec_index = 2;	/* 8k */
		} else
			bufspec_index = 0;/* 1080p */
	}
	pr_info("buffer spec %d\n", bufspec_index);
	memcpy(cur_buf_info, &amvavs3_workbuff_spec[bufspec_index],
		sizeof(struct BuffInfo_s));

	cur_buf_info->start_adr = dec->buf_start;
#ifndef AVS3_10B_MMU
	dec->mc_buf_spec.buf_end = dec->buf_start + dec->buf_size;
#endif

	init_buff_spec(dec, cur_buf_info);

	avs3_bufmgr_init(&dec->avs3_dec);
	//init_avs3_decoder(&dec->avs3_dec);

#ifdef AVS3_10B_MMU
	avs3_dec_init(dec, cur_buf_info, NULL);
#else
	dec->mc_buf_spec.buf_start = (cur_buf_info->end_adr + 0xffff)
		& (~0xffff);
	dec->mc_buf_spec.buf_size = (dec->mc_buf_spec.buf_end
		- dec->mc_buf_spec.buf_start);
	if (debug) {
		pr_err("dec->mc_buf_spec.buf_start %x-%x\n",
			dec->mc_buf_spec.buf_start,
			dec->mc_buf_spec.buf_start +
			dec->mc_buf_spec.buf_size);
	}
	avs3_dec_init(dec, cur_buf_info, &dec->mc_buf_spec);
#endif
	if ((buf_alloc_width & buf_alloc_height) == 0) {
		if (!vdec_is_support_4k()
			&& (buf_alloc_width > 1920 &&  buf_alloc_height > 1088)) {
			buf_alloc_width = 1920;
			buf_alloc_height = 1088;
		} else if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_SM1) {
			buf_alloc_width = 3840;
			buf_alloc_height = 2160;
		}
	}

	dec->init_pic_w = buf_alloc_width ? buf_alloc_width :
		(dec->vavs3_amstream_dec_info.width ?
		dec->vavs3_amstream_dec_info.width :
		dec->work_space_buf->max_width);
	dec->init_pic_h = buf_alloc_height ? buf_alloc_height :
		(dec->vavs3_amstream_dec_info.height ?
		dec->vavs3_amstream_dec_info.height :
		dec->work_space_buf->max_height);

	pr_info(
		"init_pic_w %d init_pic_h %d\n", dec->init_pic_w, dec->init_pic_h);

#ifndef AVS3_10B_MMU
	init_buf_list(dec);
#else
	dec->avs3_dec.max_pb_size = max_buf_num + dec->dynamic_buf_margin;
	if (dec->avs3_dec.max_pb_size > MAX_PB_SIZE)
		dec->avs3_dec.max_pb_size = MAX_PB_SIZE;
#endif
	//dec->avs3_dec.ref_maxbuffer = MAX_PB_SIZE; //dec->used_buf_num - 1;
	/*init_pic_list(dec);*/

	pts_unstable = ((unsigned long)(dec->vavs3_amstream_dec_info.param)
			& 0x40) >> 6;

	if ((debug & AVS3_DBG_SEND_PARAM_WITH_REG) == 0) {
		dec->rpm_addr = decoder_dma_alloc_coherent(&dec->rpm_mem_handle,
			RPM_BUF_SIZE,
			&dec->rpm_phy_addr, "AVS3_RPM_BUF");
		if (dec->rpm_addr == NULL) {
			pr_err("%s: failed to alloc rpm buffer\n", __func__);
			return -1;
		}
		avs3_print(dec, AVS3_DBG_BUFMGR,
			"rpm_phy_addr %x\n", (u32) dec->rpm_phy_addr);
		dec->rpm_ptr = dec->rpm_addr;
	}

	if (cuva_buf_size > 0) {
		dec->cuva_size = AUX_BUF_ALIGN(cuva_buf_size);

		dec->cuva_addr = decoder_dma_alloc_coherent(&dec->cuva_handle,
				dec->cuva_size, &dec->cuva_phy_addr, "AVS3_CUVA_BUF");
			avs3_print(dec, AVS3_DBG_BUFMGR,
			"%s, cuva_size = %d cuva_phy_addr %x dec->cuva_addr = %px\n",
			__func__, dec->cuva_size, (u32)dec->cuva_phy_addr, dec->cuva_addr);
		if (dec->cuva_addr == NULL) {
			pr_err("%s: failed to alloc cuva buffer\n", __func__);
			return -1;
		}
	}
#if (defined DEBUG_UCODE_LOG) || (defined DEBUG_CMD)
	//if (udebug_flag & 0x8) {
		dec->ucode_log_addr = decoder_dma_alloc_coherent(&dec->ucode_log_handle,
				UCODE_LOG_BUF_SIZE, &dec->ucode_log_phy_addr, "AVS3_UCODE_LOG_BUF");
		if (dec->ucode_log_addr == NULL) {
			dec->ucode_log_phy_addr = 0;
		}
		pr_info("%s: alloc ucode log buffer %p\n",
			__func__, dec->ucode_log_addr);
	//}
#endif
	dec->lmem_addr = decoder_dma_alloc_coherent(&dec->lmem_phy_handle,
			LMEM_BUF_SIZE,
			&dec->lmem_phy_addr, "AVS3_LMEM_BUF");
	if (dec->lmem_addr == NULL) {
		pr_err("%s: failed to alloc lmem buffer\n", __func__);
		return -1;
	} else
		avs3_print(dec, AVS3_DBG_BUFMGR,
			"%s, lmem_phy_addr %x\n",
			__func__, (u32)dec->lmem_phy_addr);
	dec->lmem_ptr = dec->lmem_addr;

#ifdef AVS3_10B_MMU
	if (dec->mmu_enable) {
		dec->frame_mmu_map_addr = decoder_dma_alloc_coherent(&dec->frame_mmu_map_handle,
					get_frame_mmu_map_size(),
					&dec->frame_mmu_map_phy_addr, "AVS3_MMU_BUF");
		if (dec->frame_mmu_map_addr == NULL) {
			pr_err("%s: failed to alloc count_buffer\n", __func__);
			return -1;
		}
		memset(dec->frame_mmu_map_addr, 0, get_frame_mmu_map_size());
#ifdef NEW_FB_CODE
		if (dec->front_back_mode && dec->frame_mmu_map_addr_1 == NULL) {
			dec->frame_mmu_map_addr_1 =
				decoder_dma_alloc_coherent(&dec->frame_mmu_map_handle_1,
				get_frame_mmu_map_size(),
				&dec->frame_mmu_map_phy_addr_1, "AVS3_MMU_1_BUF");
			if (dec->frame_mmu_map_addr_1 == NULL) {
				pr_err("%s: failed to alloc count_buffer\n", __func__);
				return -1;
			}
			memset(dec->frame_mmu_map_addr_1, 0, get_frame_mmu_map_size());
		}
#endif
	}
#endif

#ifdef AVS3_10B_MMU_DW
	if (dec->dw_mmu_enable) {
		dec->dw_frame_mmu_map_addr = decoder_dma_alloc_coherent(&dec->frame_dw_mmu_map_handle,
					get_frame_mmu_map_size(),
					&dec->dw_frame_mmu_map_phy_addr, "AVS3_DWMMU_BUF");
		if (dec->dw_frame_mmu_map_addr == NULL) {
			pr_err("%s: failed to alloc count_buffer\n", __func__);
			return -1;
		}
		memset(dec->dw_frame_mmu_map_addr, 0, get_frame_mmu_map_size());
#ifdef NEW_FB_CODE
		if (dec->front_back_mode) {
			dec->dw_frame_mmu_map_addr_1 =
					decoder_dma_alloc_coherent(&dec->frame_dw_mmu_map_handle_1,
					get_frame_mmu_map_size(),
					&dec->dw_frame_mmu_map_phy_addr_1, "AVS3_DWMMU_1_BUF");
			if (dec->dw_frame_mmu_map_addr_1 == NULL) {
				pr_err("%s: failed to alloc count_buffer\n", __func__);
				return -1;
			}
			memset(dec->dw_frame_mmu_map_addr_1, 0, get_frame_mmu_map_size());
		}
#endif
	}
#endif
#ifdef NEW_FB_CODE
	avs3_dec->wait_working_buf = 0;
	avs3_dec->front_pause_flag = 0; /*multi pictures in one packe*/
	if (dec->front_back_mode) {
		avs3_dec->frontend_decoded_count = 0;
		avs3_dec->backend_decoded_count = 0;
		avs3_dec->fb_wr_pos = 0;
		avs3_dec->fb_rd_pos = 0;
		init_fb_bufstate(dec);
		if (fbdebug_flag & 0x4) {
			copy_loopbufs_ptr(&avs3_dec->init_fr, &avs3_dec->fr);
		}
		avs3_print(dec, PRINT_FLAG_VDEC_DETAIL,
			"copy loopbuf fr to next_bk[fb_wr_pos=%d]\n",avs3_dec->fb_wr_pos);
		copy_loopbufs_ptr(&avs3_dec->next_bk[avs3_dec->fb_wr_pos], &avs3_dec->fr);
	}
#endif
	ret = 0;
	return ret;
}

/********************************************
 *  Mailbox command
 ********************************************/
#define CMD_FINISHED               0
#define CMD_ALLOC_VIEW             1
#define CMD_FRAME_DISPLAY          3
#define CMD_DEBUG                  10

#define DECODE_BUFFER_NUM_MAX    32
#define DISPLAY_BUFFER_NUM       6

#define video_domain_addr(adr) (adr&0x7fffffff)
#define DECODER_WORK_SPACE_SIZE 0x800000

#define spec2canvas(x)  \
	(((x)->uv_canvas_index << 16) | \
		((x)->uv_canvas_index << 8)  | \
		((x)->y_canvas_index << 0))

static void set_canvas(struct AVS3Decoder_s *dec,
	struct avs3_frame_s *pic)
{
	int canvas_w = ALIGN(pic->width, 64)/4;
	int canvas_h = ALIGN(pic->height, 32)/4;
	int blkmode = mem_map_mode;
	struct vdec_s *vdec = hw_to_vdec(dec);
	/*CANVAS_BLKMODE_64X32*/
	if (pic->double_write_mode) {
		canvas_w = pic->width /
			get_double_write_ratio(pic->double_write_mode);
		canvas_h = pic->height /
			get_double_write_ratio(pic->double_write_mode);
		/*sao_crtl1 aligned with 64*/
		canvas_w = ALIGN(canvas_w, 64);
		canvas_h = ALIGN(canvas_h, 32);

		if (vdec->parallel_dec == 1) {
			if (pic->y_canvas_index == -1)
				pic->y_canvas_index = vdec->get_canvas_ex(CORE_MASK_HEVC, vdec->id);
			if (pic->uv_canvas_index == -1)
				pic->uv_canvas_index = vdec->get_canvas_ex(CORE_MASK_HEVC, vdec->id);
		} else {
			pic->y_canvas_index = 128 + pic->index * 2;
			pic->uv_canvas_index = 128 + pic->index * 2 + 1;
		}

		config_cav_lut_ex(pic->y_canvas_index,
			pic->dw_y_adr, canvas_w, canvas_h,
			CANVAS_ADDR_NOWRAP, blkmode, 0x7, VDEC_HEVC);
		config_cav_lut_ex(pic->uv_canvas_index,
			pic->dw_u_v_adr, canvas_w, canvas_h,
			CANVAS_ADDR_NOWRAP, blkmode, 0x7, VDEC_HEVC);
#ifdef MULTI_INSTANCE_SUPPORT
		pic->canvas_config[0].phy_addr = pic->dw_y_adr;
		pic->canvas_config[0].width = canvas_w;
		pic->canvas_config[0].height = canvas_h;
		pic->canvas_config[0].block_mode = blkmode;
		pic->canvas_config[0].endian = 7;
		pic->canvas_config[0].bit_depth = is_dw_p010(dec);

		pic->canvas_config[1].phy_addr = pic->dw_u_v_adr;
		pic->canvas_config[1].width = canvas_w;
		pic->canvas_config[1].height = canvas_h;
		pic->canvas_config[1].block_mode = blkmode;
		pic->canvas_config[1].endian = 7;
		pic->canvas_config[1].bit_depth = is_dw_p010(dec);

		decoder_trace(dec->trace.set_canvas0_addr, pic->canvas_config[0].phy_addr, TRACE_BUFFER);
#endif
	} else {
	#ifndef AVS3_10B_MMU
		if (vdec->parallel_dec == 1) {
			if (pic->y_canvas_index == -1)
				pic->y_canvas_index = vdec->get_canvas_ex(CORE_MASK_HEVC, vdec->id);
			if (pic->uv_canvas_index == -1)
				pic->uv_canvas_index = vdec->get_canvas_ex(CORE_MASK_HEVC, vdec->id);
		} else {
			pic->y_canvas_index = 128 + pic->index;
			pic->uv_canvas_index = 128 + pic->index;
		}

		config_cav_lut_ex(pic->y_canvas_index,
			pic->mc_y_adr, canvas_w, canvas_h,
			CANVAS_ADDR_NOWRAP, blkmode, 0x7, VDEC_HEVC);
		config_cav_lut_ex(pic->uv_canvas_index,
			pic->mc_u_v_adr, canvas_w, canvas_h,
			CANVAS_ADDR_NOWRAP, blkmode, 0x7, VDEC_HEVC);

		decoder_trace(dec->trace.set_canvas0_addr, spec2canvas(pic), TRACE_BUFFER);
	#endif
	}

#ifdef OW_TRIPLE_WRITE
	if (pic->triple_write_mode) {
		canvas_w = pic->width /
			get_double_write_ratio(pic->triple_write_mode & 0xf);	//same ratio with double write
		canvas_h = pic->height /
			get_double_write_ratio(pic->triple_write_mode & 0xf);

		canvas_w = ALIGN(canvas_w, 64);
		canvas_h = ALIGN(canvas_h, 32);

#if 0
		if (vdec->parallel_dec == 1) {
			if (pic->tw_y_canvas_index == -1)
				pic->tw_y_canvas_index = vdec->get_canvas_ex(CORE_MASK_HEVC, vdec->id);
			if (pic->tw_uv_canvas_index == -1)
				pic->tw_uv_canvas_index = vdec->get_canvas_ex(CORE_MASK_HEVC, vdec->id);
		} else {
			pic->tw_y_canvas_index = 128 + pic->index * 2;
			pic->tw_uv_canvas_index = 128 + pic->index * 2 + 1;
		}

		config_cav_lut_ex(pic->y_canvas_index,
			pic->dw_y_adr, canvas_w, canvas_h,
			CANVAS_ADDR_NOWRAP, blkmode, 7, VDEC_HEVC);
		config_cav_lut_ex(pic->uv_canvas_index, pic->dw_u_v_adr,
			canvas_w, canvas_h,
			CANVAS_ADDR_NOWRAP, blkmode, 7, VDEC_HEVC);
#endif
		pic->tw_canvas_config[0].phy_addr = pic->tw_y_adr;
		pic->tw_canvas_config[0].width = canvas_w;
		pic->tw_canvas_config[0].height = canvas_h;
		pic->tw_canvas_config[0].block_mode = blkmode;
		pic->tw_canvas_config[0].endian = 7;
		pic->tw_canvas_config[0].bit_depth = is_tw_p010(dec);

		pic->tw_canvas_config[1].phy_addr = pic->tw_u_v_adr;
		pic->tw_canvas_config[1].width = canvas_w;
		pic->tw_canvas_config[1].height = canvas_h;
		pic->tw_canvas_config[1].block_mode = blkmode;
		pic->tw_canvas_config[1].endian = 7;
		pic->tw_canvas_config[1].bit_depth = is_tw_p010(dec);
	}
#endif
}

static void set_frame_info(struct AVS3Decoder_s *dec, struct vframe_s *vf)
{
	unsigned int ar = 0;
	unsigned int pixel_ratio = 0;;

	vf->duration = dec->frame_dur;
	vf->duration_pulldown = 0;
	vf->flag = 0;
	vf->prop.master_display_colour = dec->vf_dp;
	if (dec->hdr_flag & HDR_CUVA_MASK)
		dec->video_signal_type |= 1 << 31;
	vf->signal_type = dec->video_signal_type;

	avs3_print(dec, AVS3_DBG_HDR_INFO,
			"signal_type 0x%x \n",
			vf->signal_type);

	pixel_ratio = dec->vavs3_amstream_dec_info.ratio;

	if (dec->vavs3_ratio == 0) {
			/* always stretch to 16:9 */
			vf->ratio_control |= (0x90 <<
					DISP_RATIO_ASPECT_RATIO_BIT);
			vf->sar_width = 1;
			vf->sar_height = 1;
		} else {
			switch (pixel_ratio) {
			case 1:
				vf->sar_width = 1;
				vf->sar_height = 1;
				ar = (vf->height * dec->vavs3_ratio) / vf->width;
				break;
			case 2:
				vf->sar_width = 4;
				vf->sar_height = 3;
				ar = (vf->height * 3 * dec->vavs3_ratio) / (vf->width * 4);
				break;
			case 3:
				vf->sar_width = 16;
				vf->sar_height = 9;
				ar = (vf->height * 9 * dec->vavs3_ratio) / (vf->width * 16);
				break;
			case 4:
				vf->sar_width = 221;
				vf->sar_height = 100;
				ar = (vf->height * 100 * dec->vavs3_ratio) / (vf->width *
						221);
				break;
			default:
				vf->sar_width = 1;
				vf->sar_height = 1;
				ar = (vf->height * dec->vavs3_ratio) / vf->width;
				break;
			}
		}

	ar = min_t(u32, ar, DISP_RATIO_ASPECT_RATIO_MAX);
	vf->ratio_control = (ar << DISP_RATIO_ASPECT_RATIO_BIT);

	vf->sidebind_type = dec->sidebind_type;
	vf->sidebind_channel_id = dec->sidebind_channel_id;
	vf->codec_vfmt = VFORMAT_AVS3;

	return;
}

static int vavs3_vf_states(struct vframe_states *states, void *op_arg)
{
	struct AVS3Decoder_s *dec = (struct AVS3Decoder_s *)op_arg;

	states->vf_pool_size = VF_POOL_SIZE;
	states->buf_free_num = kfifo_len(&dec->newframe_q);
	states->buf_avail_num = kfifo_len(&dec->display_q);

	if (step == 2)
		states->buf_avail_num = 0;
	return 0;
}

static struct vframe_s *vavs3_vf_peek(void *op_arg)
{
	struct vframe_s *vf;
	struct AVS3Decoder_s *dec = (struct AVS3Decoder_s *)op_arg;
	if (step == 2)
		return NULL;

	if (force_disp_pic_index & 0x100) {
		if (force_disp_pic_index & 0x200)
			return NULL;
		return &dec->vframe_dummy;
	}

	if (kfifo_len(&dec->display_q) > VF_POOL_SIZE) {
		avs3_print(dec, AVS3_DBG_BUFMGR,
			"kfifo len:%d invalid, peek error\n",
			kfifo_len(&dec->display_q));
		return NULL;
	}
#ifdef NEW_FB_CODE
	if (dec->front_back_mode) {
		struct vframe_s *vf_tmp;
		if (kfifo_peek(&dec->display_q, &vf_tmp) && vf_tmp) {
				uint8_t index = vf_tmp->index & 0xff;
				struct avs3_frame_s *pic = get_pic_by_index(dec, index);

				if ((pic == NULL) || (!pic->back_done_mark)) {
					return NULL;
				}
		} else
			return NULL;
	}
#endif

	if (kfifo_peek(&dec->display_q, &vf)) {
		return vf;
	}

	return NULL;
}

static struct avs3_frame_s *get_pic_by_index(
	struct AVS3Decoder_s *dec, int index)
{
	struct avs3_frame_s *pic = NULL;

	if ((index >= 0) && (index < dec->avs3_dec.max_pb_size))
		pic = &dec->avs3_dec.pic_pool[index].buf_cfg;
	return pic;
}

static void update_vf_memhandle(struct AVS3Decoder_s *dec,
	struct vframe_s *vf, struct avs3_frame_s *pic)
{
	vf->mem_handle = NULL;
	vf->mem_handle_1 = NULL;
	vf->mem_head_handle = NULL;
	vf->mem_dw_handle = NULL;

#ifdef AVS3_10B_MMU
	if (vf->type & VIDTYPE_SCATTER) {
#ifdef AVS3_10B_MMU_DW
		if (pic->double_write_mode & 0x20) {
			vf->mem_handle =
				decoder_mmu_box_get_mem_handle(
					dec->dw_mmu_box, pic->index);
			if (dec->front_back_mode)
				vf->mem_handle_1 = decoder_mmu_box_get_mem_handle(dec->dw_mmu_box_1, pic->index);
			vf->mem_head_handle =
				decoder_bmmu_box_get_mem_handle(
					dec->bmmu_box,
					HEADER_BUFFER_IDX(pic->BUF_index));
			vf->mem_dw_handle = NULL;
		} else
#endif
		{
			vf->mem_handle = decoder_mmu_box_get_mem_handle(
				dec->mmu_box,
				pic->index);
			if (dec->front_back_mode)
				vf->mem_handle_1 = decoder_mmu_box_get_mem_handle(dec->mmu_box_1, pic->index);
			vf->mem_head_handle = decoder_bmmu_box_get_mem_handle(
				dec->bmmu_box,
				HEADER_BUFFER_IDX(pic->index));
			if (dec->double_write_mode == 3)
				vf->mem_dw_handle =
					decoder_bmmu_box_get_mem_handle(
						dec->bmmu_box,
						VF_BUFFER_IDX(pic->BUF_index));
			else
				vf->mem_dw_handle = NULL;
		}
	} else {
		vf->mem_handle = decoder_bmmu_box_get_mem_handle(
			dec->bmmu_box,
			VF_BUFFER_IDX(pic->index));
		vf->mem_head_handle = decoder_bmmu_box_get_mem_handle(
			dec->bmmu_box,
			HEADER_BUFFER_IDX(pic->index));
	}
#else
	vf->mem_handle = decoder_bmmu_box_get_mem_handle(
		dec->bmmu_box,
		VF_BUFFER_IDX(pic->index));
#endif
}

static void fill_frame_info(struct AVS3Decoder_s *dec,
	struct avs3_frame_s *pic, unsigned int framesize, unsigned int pts);

static struct vframe_s *vavs3_vf_get(void *op_arg)
{
	struct vframe_s *vf;
	struct AVS3Decoder_s *dec = (struct AVS3Decoder_s *)op_arg;
	unsigned long flags = 0;

	if (step == 2)
		return NULL;
	else if (step == 1)
			step = 2;

	if (force_disp_pic_index & 0x100) {
		int idx = force_disp_pic_index & 0xff;
		struct avs3_frame_s *pic = get_pic_by_index(dec, idx);

		if (pic == NULL)
			return NULL;
		if (force_disp_pic_index & 0x200)
			return NULL;

		vf = &dec->vframe_dummy;

		set_vframe(dec, vf, pic, 1);

		force_disp_pic_index |= 0x200;
		return vf;
	}

#ifdef NEW_FB_CODE
	if (dec->front_back_mode) {
		if (kfifo_peek(&dec->display_q, &vf) && vf) {
				uint8_t index = vf->index & 0xff;
				struct avs3_frame_s *pic = get_pic_by_index(dec, index);

				if ((pic == NULL) || (!pic->back_done_mark)) {
					return NULL;
				}
		} else
			return NULL;
	}
#endif

	lock_buffer(dec, flags);
	if (kfifo_get(&dec->display_q, &vf)) {
		uint8_t index = vf->index & 0xff;
		struct vdec_s *vdec = hw_to_vdec(dec);

		decoder_trace(dec->trace.vf_get_name, (long)vf, TRACE_BUFFER);
		decoder_trace(dec->trace.disp_q_name, kfifo_len(&dec->display_q), TRACE_BUFFER);
#ifdef MULTI_INSTANCE_SUPPORT
		decoder_trace(dec->trace.set_canvas0_addr, vf->canvas0_config[0].phy_addr, TRACE_BUFFER);
#else
		decoder_trace(dec->trace.get_canvas0_addr, vf->canvas0Addr, TRACE_BUFFER);
#endif

		if (index < dec->avs3_dec.max_pb_size) {
			struct avs3_frame_s *pic = get_pic_by_index(dec, index);

			if (pic == NULL &&
				(debug & AVS3_DBG_PIC_LEAK)) {
				int i;
				avs3_print(dec, 0,
				"%s error index 0x%x pic not exist\n",
				__func__, index);
				print_pic_pool(&dec->avs3_dec, "");
				for (i = 0; i < 10; i++) {
					pic = get_pic_by_index(dec, index);
					pr_info("pic = %p\n", pic);
				}

				if (debug & AVS3_DBG_PIC_LEAK)
					debug |= AVS3_DBG_PIC_LEAK_WAIT;
				unlock_buffer(dec, flags);
				return NULL;
			}

			vf->vf_ud_param.magic_code = UD_MAGIC_CODE;
			vf->vf_ud_param.ud_param.buf_len = 0;
			vf->vf_ud_param.ud_param.pbuf_addr = NULL;
			vf->vf_ud_param.ud_param.instance_id = vdec->afd_video_id;

			vf->vf_ud_param.ud_param.meta_info.duration = vf->duration;
			vf->vf_ud_param.ud_param.meta_info.flags = (VFORMAT_AVS3 << 3);
			vf->vf_ud_param.ud_param.meta_info.vpts = vf->pts;
			if (vf->pts)
				vf->vf_ud_param.ud_param.meta_info.vpts_valid = 1;

			vf->omx_index = dec->vf_get_count;
			dec->vf_get_count++;
			if (pic) {
				if (dec->front_back_mode == 1) {
					update_vf_memhandle(dec, vf, pic);
					decoder_do_frame_check(hw_to_vdec(dec), vf);
				}
				avs3_print(dec, AVS3_DBG_BUFMGR,
				"%s vf %p pic %p index 0x%x getcount %d type 0x%x w/h/depth %d/%d/0x%x, compHeadAddr 0x%08x, pts %d, %lld\n",
				__func__, vf, pic, index,
				//pic->imgtr_fwRefDistance_bak,
				dec->vf_get_count,
				vf->type,
				vf->width, vf->height,
				vf->bitdepth,
				vf->compHeadAddr,
				vf->pts,
				vf->pts_us64);
			}

			if (pic && (!(pic->error_mark) || !(error_handle_policy & 0x4))) {
#ifdef NEW_FB_CODE
				if (dec->front_back_mode == 1) {
					fill_frame_info(dec, pic, 0, vf->pts);
					vdec_fill_vdec_frame(vdec, &pic->vqos, dec->gvs, vf, pic->hw_decode_time);
				}
#endif
				unlock_buffer(dec, flags);
				return vf;
			}

			dec->vf_get_count--;
			dec->vf_pre_count--;
			avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
				"%s:get error pic from fifo\n", __func__);
			unlock_buffer(dec, flags);
			vavs3_vf_put(vf, dec);
			return NULL;
		}
	}
	unlock_buffer(dec, flags);
	return NULL;
}

static void vavs3_vf_put(struct vframe_s *vf, void *op_arg)
{
	struct AVS3Decoder_s *dec = (struct AVS3Decoder_s *)op_arg;
#ifdef MULTI_INSTANCE_SUPPORT
	struct vdec_s *vdec = hw_to_vdec(dec);
#endif
	uint8_t index;
	unsigned long flags = 0;

	if (vf == (&dec->vframe_dummy))
		return;

	if (!vf)
		return;

	decoder_trace(dec->trace.vf_put_name, (long)vf, TRACE_BUFFER);
#ifdef MULTI_INSTANCE_SUPPORT
	decoder_trace(dec->trace.put_canvas0_addr, vf->canvas0_config[0].phy_addr, TRACE_BUFFER);
#else
	decoder_trace(dec->trace.put_canvas0_addr, vf->canvas0Addr, TRACE_BUFFER);
#endif

	lock_buffer(dec, flags);
	index = vf->index & 0xff;
	kfifo_put(&dec->newframe_q, (const struct vframe_s *)vf);
	decoder_trace(dec->trace.new_q_name, kfifo_len(&dec->newframe_q), TRACE_BUFFER);

	if (index < dec->avs3_dec.max_pb_size) {
		struct avs3_frame_s *pic = get_pic_by_index(dec, index);

		if (pic && pic->vf_ref > 0) {
			pic->vf_ref--;
		} else {
			if (pic)
				avs3_print(dec, 0,
					"%s, error pic (index %d) vf_ref is %d\n",
					__func__, index, pic->vf_ref);
			else
				avs3_print(dec, 0,
					"%s, error pic (index %d) is NULL\n",
					__func__, index);
		}

		if (pic && (!(pic->error_mark) || !(error_handle_policy & 0x4)))
			dec->vf_put_count++;

		avs3_print(dec, AVS3_DBG_BUFMGR,
			"%s vf %p pic %p index 0x%x putcount %d\n",
			__func__, vf, pic, vf->index & 0xff, dec->vf_put_count);

		if (dec->wait_buf)
			WRITE_VREG(dec->ASSIST_MBOX0_IRQ_REG, 0x1);
		dec->last_put_idx = index;
		dec->new_frame_displayed++;
	}
	unlock_buffer(dec, flags);

#ifdef MULTI_INSTANCE_SUPPORT
	vdec_up(vdec);
#endif
}

static int vavs3_event_cb(int type, void *data, void *private_data)
{
	struct AVS3Decoder_s *dec = (struct AVS3Decoder_s *)private_data;

	if (type & VFRAME_EVENT_RECEIVER_REQ_STATE) {
		struct provider_state_req_s *req =
			(struct provider_state_req_s *)data;
		if (req->req_type == REQ_STATE_SECURE)
			req->req_result[0] = vdec_secure(hw_to_vdec(dec));
		else
			req->req_result[0] = 0xffffffff;
	} else if (type & VFRAME_EVENT_RECEIVER_GET_AUX_DATA) {
		struct provider_aux_req_s *req =
			(struct provider_aux_req_s *)data;
		unsigned char index;
		unsigned long flags;
		struct avs3_frame_s *pic;

		if (!req->vf) {
			req->aux_size = dec->vf_put_count;
			return 0;
		}
		lock_buffer(dec, flags);
		index = req->vf->index & 0xff;
		req->aux_buf = NULL;
		req->aux_size = 0;
		req->format = VFORMAT_AVS3;
		if (index < dec->avs3_dec.max_pb_size) {
			pic = get_pic_by_index(dec, index);
			req->aux_buf = pic->cuva_data_buf;
			req->aux_size = pic->cuva_data_size;
		}
		unlock_buffer(dec, flags);

		avs3_print(dec, PRINT_FLAG_VDEC_STATUS,
		"%s pic 0x%p index %d =>size %d\n",
		__func__, pic, index, req->aux_size);
	}

	return 0;
}

/*
static struct avs3_frame_s *get_disp_pic(struct AVS3Decoder_s *dec)
{
	struct avs3_decoder *avs3_dec = &dec->avs3_dec;
	struct avs3_frame_s *pic = NULL;
	int32_t j;
	int32_t pre_disp_count_min = 0x7fffffff;
	for (j = 0; j < avs3_dec->ref_maxbuffer; j++) {
		if (avs3_dec->fref[j]->to_prepare_disp &&
			avs3_dec->fref[j]->to_prepare_disp <
			pre_disp_count_min) {
			pre_disp_count_min =
				avs3_dec->fref[j]->to_prepare_disp;
			pic = avs3_dec->fref[j];
		}
	}
	if (pic)
		pic->to_prepare_disp = 0;

	return pic;

}
*/

static void fill_frame_info(struct AVS3Decoder_s *dec,
	struct avs3_frame_s *pic, unsigned int framesize, unsigned int pts)
{
	struct vdec_s *vdec = hw_to_vdec(dec);

	if (pic->slice_type == SLICE_I)
		pic->vqos.type = 1;
	else if (pic->slice_type == SLICE_P)
		pic->vqos.type = 2;
	else if (pic->slice_type == SLICE_B)
		pic->vqos.type = 3;

	if (input_frame_based(hw_to_vdec(dec)))
		pic->vqos.size = pic->frame_size;
	else {
		if (!((vdec->vbuf.no_parser == 0) || (vdec->vbuf.use_ptsserv == SINGLE_PTS_SERVER_DECODER_LOOKUP))
			&& vdec_stream_based(vdec))
			pic->vqos.size = pic->stream_size;
		else if (framesize == 0)
			pic->vqos.size = pic->stream_size;
		else
			pic->vqos.size = framesize;
	}

	if (pic->error_mark)
		pic->vqos.decode_buffer = 3;
	else
		pic->vqos.decode_buffer = 0;

	pic->vqos.num++;
	pic->vqos.pts = pts;

	if (get_dbg_flag(dec) & AVS3_DBG_QOS_INFO) {
		avs3_print(dec, 0, "slice:%d, poc:%d, size %d, decode_buffer %d, time %dus\n",
			pic->vqos.type, pic->poc, pic->vqos.size, pic->vqos.decode_buffer, pic->hw_decode_time);
		avs3_print(dec, 0, "mv: max:%d, avg:%d, min:%d\n",
			pic->vqos.max_mv,
			pic->vqos.avg_mv,
			pic->vqos.min_mv);

		avs3_print(dec, 0, "qp: max:%d, avg:%d, min:%d\n",
			pic->vqos.max_qp,
			pic->vqos.avg_qp,
			pic->vqos.min_qp);

		avs3_print(dec, 0, "skip: max:%d, avg:%d, min:%d\n",
			pic->vqos.max_skip,
			pic->vqos.avg_skip,
			pic->vqos.min_skip);
	}

	return ;
}

static void set_vframe(struct AVS3Decoder_s *dec,
	struct vframe_s *vf, struct avs3_frame_s *pic, u8 dummy)
{
	u32 stream_offset = pic->stream_offset;
	unsigned int frame_size = 0;
	int pts_discontinue;
	struct vdec_s *vdec = hw_to_vdec(dec);
	ulong flags = 0;

	avs3_print(dec, AVS3_DBG_BUFMGR,
		"%s index = %d\r\n",
		__func__, pic->index);

#ifdef OW_TRIPLE_WRITE
	if ((pic->triple_write_mode) ||
		(pic->double_write_mode && (pic->double_write_mode & 0x20) == 0))
#else
	if (pic->double_write_mode && (pic->double_write_mode & 0x20) == 0)
#endif
		set_canvas(dec, pic);

	display_frame_count[dec->index]++;

	if (!dummy) {
#ifdef MULTI_INSTANCE_SUPPORT
		if (vdec_frame_based(vdec)) {
			vf->pts = pic->pts;
			vf->pts_us64 = pic->pts64;
		} else {
#endif
			if (vdec->vbuf.use_ptsserv == SINGLE_PTS_SERVER_DECODER_LOOKUP) {
			/* if (pts_lookup_offset(PTS_TYPE_VIDEO,
				stream_offset, &vf->pts, 0) != 0) { */
				if (pts_lookup_offset_us64
					(PTS_TYPE_VIDEO, stream_offset,
					&vf->pts, &frame_size, 0,
					 &vf->pts_us64) != 0) {
#ifdef DEBUG_PTS
					dec->pts_missed++;
#endif
					vf->pts = 0;
					vf->pts_us64 = 0;
				}
			}
		}
#ifdef DEBUG_PTS
		else
			dec->pts_hit++;
#endif
		if (pts_unstable)
			dec->pts_mode = PTS_NONE_REF_USE_DURATION;

#ifdef NEW_FB_CODE
		if (dec->front_back_mode != 1)
#endif
		fill_frame_info(dec, pic, frame_size, vf->pts);

		if ((dec->pts_mode == PTS_NORMAL) && (vf->pts != 0)
			&& dec->get_frame_dur) {
			int pts_diff = (int)vf->pts - dec->last_lookup_pts;

			if (pts_diff < 0) {
				dec->pts_mode_switching_count++;
				dec->pts_mode_recovery_count = 0;

				if (dec->pts_mode_switching_count >=
					PTS_MODE_SWITCHING_THRESHOLD) {
					dec->pts_mode =
						PTS_NONE_REF_USE_DURATION;
					pr_info
					("HEVC: switch to n_d mode.\n");
				}

			} else {
				int p = PTS_MODE_SWITCHING_RECOVERY_THRESHOLD;
				dec->pts_mode_recovery_count++;
				if (dec->pts_mode_recovery_count > p) {
					dec->pts_mode_switching_count = 0;
					dec->pts_mode_recovery_count = 0;
				}
			}
		}

		pts_discontinue =
			(abs(dec->last_pts  - vf->pts) >=
				tsync_vpts_discontinuity_margin());

		if (vf->pts != 0)
			dec->last_lookup_pts = vf->pts;

		if ((dec->pts_mode == PTS_NONE_REF_USE_DURATION)
			&& ((pic->slice_type != I_IMG) || (!pts_discontinue &&
			!first_pts_checkin_complete(PTS_TYPE_AUDIO))))
			vf->pts = dec->last_pts + DUR2PTS(dec->frame_dur);
		dec->last_pts = vf->pts;

		if (vf->pts_us64 != 0)
			dec->last_lookup_pts_us64 = vf->pts_us64;

		if ((dec->pts_mode == PTS_NONE_REF_USE_DURATION)
			&& ((pic->slice_type != I_IMG) || (!pts_discontinue &&
			!first_pts_checkin_complete(PTS_TYPE_AUDIO)))) {
			vf->pts_us64 =
				dec->last_pts_us64 +
				(DUR2PTS(dec->frame_dur) * 100 / 9);
		}

		dec->last_pts_us64 = vf->pts_us64;
	}

	vf->index = 0xff00 | pic->index;

	if (pic->double_write_mode & 0x10) {
		/* double write only */
		vf->compBodyAddr = 0;
		vf->compHeadAddr = 0;
	} else {
#ifdef AVS3_10B_MMU
		vf->compBodyAddr = 0;
		vf->compHeadAddr = pic->header_adr;
#ifdef AVS3_10B_MMU_DW
		vf->dwBodyAddr = 0;
		vf->dwHeadAddr = 0;
		if (pic->double_write_mode & 0x20) {
			u32 mode = pic->double_write_mode & 0xf;
			if (mode == 5 || mode == 3)
				vf->dwHeadAddr = pic->dw_header_adr;
			else if ((mode == 1 || mode == 2 || mode == 4)
				&& ((debug & AVS3_DBG_OUT_PTS) == 0)) {
				vf->compHeadAddr = pic->dw_header_adr;
				pr_info("Use dw mmu for display\n");
			}
		}
#endif

#else
		vf->compBodyAddr = pic->mc_y_adr; /*body adr*/
		vf->compHeadAddr = pic->mc_y_adr + pic->comp_body_size;
#endif
	}
	if (pic->double_write_mode &&
		((pic->double_write_mode & 0x20) == 0)) {
		vf->type = VIDTYPE_PROGRESSIVE |
			VIDTYPE_VIU_FIELD;
		vf->type |= VIDTYPE_VIU_NV21;
		if (pic->double_write_mode == 3) {
			vf->type |= VIDTYPE_COMPRESS;
#ifdef AVS3_10B_MMU
			vf->type |= VIDTYPE_SCATTER;
#endif
		}
#ifdef MULTI_INSTANCE_SUPPORT
		if (dec->m_ins_flag) {
				vf->canvas0Addr = vf->canvas1Addr = -1;
				vf->plane_num = 2;
				vf->canvas0_config[0] =
					pic->canvas_config[0];
				vf->canvas0_config[1] =
					pic->canvas_config[1];

				vf->canvas1_config[0] =
					pic->canvas_config[0];
				vf->canvas1_config[1] =
					pic->canvas_config[1];

		} else
#endif
			vf->canvas0Addr = vf->canvas1Addr =
				spec2canvas(pic);
	} else {
		vf->canvas0Addr = vf->canvas1Addr = 0;
		vf->type = VIDTYPE_COMPRESS | VIDTYPE_VIU_FIELD;
#ifdef AVS3_10B_MMU
		vf->type |= VIDTYPE_SCATTER;
#endif
	}

	switch (pic->depth) {
	case AVS3_BITS_8:
		vf->bitdepth = BITDEPTH_Y8 |
			BITDEPTH_U8 | BITDEPTH_V8;
		break;
	case AVS3_BITS_10:
	case AVS3_BITS_12:
		vf->bitdepth = BITDEPTH_Y10 |
			BITDEPTH_U10 | BITDEPTH_V10;
		break;
	default:
		vf->bitdepth = BITDEPTH_Y10 |
			BITDEPTH_U10 | BITDEPTH_V10;
		break;
	}
	if ((vf->type & VIDTYPE_COMPRESS) == 0)
		vf->bitdepth =
			BITDEPTH_Y8 | BITDEPTH_U8 | BITDEPTH_V8;
	if (pic->depth == AVS3_BITS_8)
		vf->bitdepth |= BITDEPTH_SAVING_MODE;

	set_frame_info(dec, vf);
	/* if ((vf->width!=pic->width)|
		(vf->height!=pic->height)) */
	/* pr_info("aaa: %d/%d, %d/%d\n",
		vf->width,vf->height, pic->width,
		pic->height); */
	vf->width = pic->width /
		get_double_write_ratio(pic->double_write_mode);
	vf->height = pic->height /
		get_double_write_ratio(pic->double_write_mode);
	if (force_w_h != 0) {
		vf->width = (force_w_h >> 16) & 0xffff;
		vf->height = force_w_h & 0xffff;
	}
	if ((pic->double_write_mode & 0x20) &&
		((pic->double_write_mode & 0xf) == 2 ||
		(pic->double_write_mode & 0xf) == 4)) {
		vf->compWidth = pic->width /
			get_double_write_ratio(
				pic->double_write_mode & 0xf);
		vf->compHeight = pic->height /
			get_double_write_ratio(
				pic->double_write_mode & 0xf);
	} else {
		vf->compWidth = pic->width;
		vf->compHeight = pic->height;
	}

#ifdef OW_TRIPLE_WRITE
	if (!pic->double_write_mode && pic->triple_write_mode) {
		vf->type |= VIDTYPE_VIU_NV21;		//nv12 flag
		vf->canvas0_config[0] = pic->tw_canvas_config[0];
		vf->canvas0_config[1] = pic->tw_canvas_config[1];
		vf->canvas1_config[0] = pic->tw_canvas_config[0];
		vf->canvas1_config[1] = pic->tw_canvas_config[1];
		vf->width = pic->width /
			get_double_write_ratio(pic->triple_write_mode & 0xf);	//tw same ratio defined with dw
		vf->height = pic->height /
			get_double_write_ratio(pic->triple_write_mode & 0xf);
		avs3_print(dec, 0, "output triple write w %d, h %d, bitdepth %s\n",
				vf->width, vf->height, vf->canvas0_config[0].bit_depth?"10":"8");
	}
#endif

	if (force_fps & 0x100) {
		u32 rate = force_fps & 0xff;
		if (rate)
			vf->duration = 96000/rate;
		else
			vf->duration = 0;
	}

	if (dec->front_back_mode != 1)
		update_vf_memhandle(dec, vf, pic);

	if ((vdec->vbuf.use_ptsserv == MULTI_PTS_SERVER_UPPER_LOOKUP) && vdec_stream_based(vdec)) {
		/* offset for tsplayer pts lookup */
		u64 frame_type = 0;
		if (pic->slice_type == SLICE_I)
			frame_type = KEYFRAME_FLAG;
		else if (pic->slice_type == SLICE_P)
			frame_type = PFRAME_FLAG;
		else if (pic->slice_type == SLICE_B)
			frame_type = BFRAME_FLAG;
		vf->pts_us64 = (((u64)vf->duration << 32 | (frame_type << 62)) & 0xffffffff00000000)
			| pic->stream_offset;
		vf->pts = 0;
	} else if (vdec->vbuf.use_ptsserv == MULTI_PTS_SERVER_DECODER_LOOKUP) {
		/* lookup by decoder */
		checkout_pts_offset pts_info;
		u64 frame_type = 0;
		if (pic->slice_type == SLICE_I)
			frame_type = KEYFRAME_FLAG;
		else if (pic->slice_type == SLICE_P)
			frame_type = PFRAME_FLAG;
		else if (pic->slice_type == SLICE_B)
			frame_type = BFRAME_FLAG;
		pts_info.offset = (((u64)vf->duration << 32 | (frame_type << 62)) & 0xffffffff00000000)
			| pic->stream_offset;
		if (!ptsserver_checkout_pts_offset((vdec->pts_server_id & 0xff), &pts_info)) {
			vf->pts = pts_info.pts;
			vf->pts_us64 = pts_info.pts_64;
		} else {
			vf->pts = 0;
			vf->pts_us64 = 0;
		}
	}
	avs3_print(dec, AVS3_DBG_OUT_PTS,
		"avs3 dec out pts: vf->pts=%d, vf->pts_us64 = %lld slice_type %d, duration %d\n",
		vf->pts, vf->pts_us64, pic->slice_type, vf->duration);

	lock_buffer(dec, flags);
	if (!dummy) {
		pic->vf_ref = 1;
	}
	dec->vf_pre_count++;
	unlock_buffer(dec, flags);
}

static inline void dec_update_gvs(struct AVS3Decoder_s *dec)
{
	if (dec->gvs->frame_height != dec->frame_height) {
		dec->gvs->frame_width = dec->frame_width;
		dec->gvs->frame_height = dec->frame_height;
	}
	/*if (dec->gvs->frame_dur != dec->frame_dur) {
		dec->gvs->frame_dur = dec->frame_dur;
		if (dec->frame_dur != 0)
			dec->gvs->frame_rate = ((96000 * 10 / dec->frame_dur) % 10) < 5 ?
					96000 / dec->frame_dur : (96000 / dec->frame_dur +1);
		else
			dec->gvs->frame_rate = -1;
	}*/
	dec->gvs->status = dec->stat | dec->fatal_error;
}

static int avs3_prepare_display_buf(struct AVS3Decoder_s *dec)
{
#ifndef NO_DISPLAY
	struct vframe_s *vf = NULL;
	/*unsigned short slice_type;*/
	struct avs3_frame_s *pic;
	struct vdec_s *pvdec = hw_to_vdec(dec);

	while (1) {
		COM_PIC *com_pic = dec_pull_frm(&dec->avs3_dec.ctx, 0);

		if (com_pic == NULL)
			break;
		pic = &com_pic->buf_cfg;
		if (force_disp_pic_index & 0x100) {
			/*recycle directly*/
			continue;
		}

		if (pic->error_mark && (error_handle_policy & 0x4)) {
			avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
				"%s: error pic, skip\n", __func__);
			continue;
		}

		if (dec->start_decoding_flag != 0) {
			if (dec->skip_PB_before_I &&
				pic->slice_type != I_IMG) {
				avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
					"!!!slice type %d (not I) skip\n",
					0, pic->slice_type);
				continue;
			}
			dec->skip_PB_before_I = 0;
		}

		if (kfifo_get(&dec->newframe_q, &vf) == 0) {
			pr_info("fatal error, no available buffer slot.");
			return -1;
		}

		if (vf) {
			set_vframe(dec, vf, pic, 0);
			if (dec->front_back_mode != 1)
				decoder_do_frame_check(pvdec, vf);
			vdec_vframe_ready(pvdec, vf);
			avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
				"%s: pic %p stream_offset 0x%x\n",
				__func__, com_pic, pic->stream_offset);

			kfifo_put(&dec->display_q, (const struct vframe_s *)vf);
			decoder_trace(dec->trace.pts_name, vf->pts, TRACE_BUFFER);
			decoder_trace(dec->trace.new_q_name, kfifo_len(&dec->newframe_q), TRACE_BUFFER);
			decoder_trace(dec->trace.disp_q_name, kfifo_len(&dec->display_q), TRACE_BUFFER);

#ifdef NEW_FB_CODE
			if (dec->front_back_mode == 0) {
#endif
			dec_update_gvs(dec);
			/*count info*/
			vdec_count_info(dec->gvs, 2, pic->stream_offset);

			dec->gvs->bit_depth_luma = pic->depth;
			dec->gvs->bit_depth_chroma = pic->depth;
			dec->gvs->double_write_mode = pic->double_write_mode;
			vdec_fill_vdec_frame(pvdec, &pic->vqos, dec->gvs, vf, pic->hw_decode_time);
#ifdef NEW_FB_CODE
			}
#endif

			pvdec->vdec_fps_detec(pvdec->id);
			if (without_display_mode == 0) {
				if (dec->front_back_mode != 1)
					vf_notify_receiver(dec->provider_name,
						VFRAME_EVENT_PROVIDER_VFRAME_READY, NULL);
			} else
				vavs3_vf_put(vavs3_vf_get(dec), dec);
		}
	}
/*!NO_DISPLAY*/
#endif
	return 0;
}

static void get_rpm_param(union param_u *params)
{
	int i;
	unsigned int data32;
	if (debug & AVS3_DBG_BUFMGR)
		pr_info("enter %s\r\n", __func__);
	for (i = 0; i < (RPM_END - RPM_BEGIN); i++) {
		do {
			data32 = READ_VREG(RPM_CMD_REG);
			/*pr_info("%x\n", data32);*/
		} while ((data32 & 0x10000) == 0);
		params->l.data[i] = data32&0xffff;
		/*pr_info("%x\n", data32);*/
		WRITE_VREG(RPM_CMD_REG, 0);
	}
	if (debug & AVS3_DBG_BUFMGR)
		pr_info("leave %s\r\n", __func__);
}
static void debug_buffer_mgr_more(struct AVS3Decoder_s *dec)
{
	int i;
	if (!(debug & AVS3_DBG_BUFMGR_MORE))
		return;

	avs3_print(dec, 0, "avs3_param: (%d)\n", dec->avs3_dec.img.number);
	for (i = 0; i < (RPM_END-RPM_BEGIN); i++) {
		avs3_print_cont(dec, 0, "%04x ", dec->avs3_dec.param.l.data[i]);
		if (((i + 1) & 0xf) == 0) {
			avs3_print_cont(dec, 0, "\n");
			avs3_print_flush(dec);
		}
	}
	avs3_print(dec, 0, "\n");
}

#ifdef AVS3_10B_MMU
static void avs3_recycle_mmu_buf_tail(struct AVS3Decoder_s *dec)
{
	if (dec->cur_fb_idx_mmu != INVALID_IDX) {
		u32 used_4k_num =
		(READ_VREG(HEVC_SAO_MMU_STATUS) >> 16);
		if (dec->m_ins_flag)
			hevc_mmu_dma_check(hw_to_vdec(dec));
		decoder_mmu_box_free_idx_tail(dec->mmu_box,
			dec->cur_fb_idx_mmu, used_4k_num);

		avs3_print(dec, AVS3_DBG_BUFMGR_MORE,
		"%s decoder_mmu_box_free_idx_tail index=%d used_4k_num %d\n",
		__func__, dec->cur_fb_idx_mmu, used_4k_num);

		if (dec->dw_mmu_enable) {
			used_4k_num = READ_VREG(HEVC_SAO_MMU_STATUS2) >> 16;
			avs3_print(dec, AVS3_DBG_BUFMGR_MORE,
			"%s DW decoder_mmu_box_free_idx_tail index=%d used_4k_num %d\n",
			__func__, dec->cur_fb_idx_mmu, used_4k_num);

			decoder_mmu_box_free_idx_tail(
					dec->dw_mmu_box,
					dec->cur_fb_idx_mmu,
					used_4k_num);
		}

		dec->cur_fb_idx_mmu = INVALID_IDX;
	}
}

static void avs3_recycle_mmu_buf(struct AVS3Decoder_s *dec)
{
	if (dec->cur_fb_idx_mmu != INVALID_IDX) {
		decoder_mmu_box_free_idx(dec->mmu_box,
			dec->cur_fb_idx_mmu);

		if (dec->front_back_mode)
			decoder_mmu_box_free_idx(dec->mmu_box_1,
			dec->cur_fb_idx_mmu);

		avs3_print(dec, AVS3_DBG_BUFMGR_MORE,
		"%s decoder_mmu_box_free_idx index=%d\n",
		__func__, dec->cur_fb_idx_mmu);

		if (dec->dw_mmu_enable) {
			decoder_mmu_box_free_idx(dec->dw_mmu_box,
				dec->cur_fb_idx_mmu);

			if (dec->front_back_mode)
				decoder_mmu_box_free_idx(dec->dw_mmu_box_1,
				dec->cur_fb_idx_mmu);

			avs3_print(dec, AVS3_DBG_BUFMGR_MORE,
			"%s DW decoder_mmu_box_free_idx index=%d\n",
			__func__, dec->cur_fb_idx_mmu);
		}
		dec->cur_fb_idx_mmu = INVALID_IDX;
	}
}
#endif

static void dec_again_process(struct AVS3Decoder_s *dec)
{
#ifdef NEW_FB_CODE
	if (dec->front_back_mode == 1)
		amhevc_stop_f();
	else
#endif
	amhevc_stop();
	dec->dec_result = DEC_RESULT_AGAIN;
	if (dec->process_state ==
		PROC_STATE_DECODING) {
		dec->process_state =
		PROC_STATE_DECODE_AGAIN;
	} else if (dec->process_state ==
		PROC_STATE_HEAD_DONE) {
		dec->process_state =
		PROC_STATE_HEAD_AGAIN;
	}
	dec->next_again_flag = 1;

	vdec_schedule_work(&dec->work);
}
/*
static uint32_t log2i(uint32_t val)
{
	uint32_t ret = -1;
	while (val != 0) {
		val >>= 1;
		ret++;
	}
	return ret;
}
*/
static void check_pic_error(struct AVS3Decoder_s *dec,
	struct avs3_frame_s *pic)
{
#ifndef PXP_DEBUG
	if (dec->front_back_mode == 3 ||
		(debug & AVS3_DBG_DIS_LOC_ERROR_PROC))
		return;

	if (pic->decoded_lcu != dec->avs3_dec.lcu_total) {
		avs3_print(dec, AVS3_DBG_BUFMGR,
			"%s error pic(index %d) decoded lcu %d (total %d)\n",
			__func__, pic->index,
			pic->decoded_lcu, dec->avs3_dec.lcu_total);
		pic->error_mark = 1;
	} else {
		avs3_print(dec, AVS3_DBG_BUFMGR_MORE,
			"%s pic(index %d) decoded lcu %d (total %d)\n",
			__func__, pic->index,
			pic->decoded_lcu, dec->avs3_dec.lcu_total);

	}
#endif
}
static void update_decoded_pic(struct AVS3Decoder_s *dec)
{
	struct avs3_frame_s *pic = dec->avs3_dec.cur_pic;
	if (pic) {
		dec->avs3_dec.cur_pic->decoded_lcu =
			(READ_VREG(HEVC_PARSER_LCU_START)
					& 0xffffff) + 1;
		avs3_print(dec, AVS3_DBG_BUFMGR_MORE,
			"%s pic(index %d) decoded lcu %d (total %d)\n",
			__func__, pic->index,
			pic->decoded_lcu, dec->avs3_dec.lcu_total);
	}
}
/* +[SE] [BUG][BUG-171463][chuanqi.wang]: get frame rate by video sequeue*/
static int get_frame_rate(union param_u *params, struct AVS3Decoder_s *dec)
{
	int tmp = 0;

	switch (params->p.sqh_frame_rate_code) {
	case 1:
	case 2:
		tmp = 24;
		break;
	case 3:
		tmp =  25;
		break;
	case 4:
	case 5:
		tmp =  30;
		break;
	case 6:
		tmp =  50;
		break;
	case 7:
	case 8:
		tmp =  60;
		break;
	case 9:
		tmp =  100;
		break;
	case 10:
		tmp = 120;
		break;
	default:
		tmp =  25;
		break;
	}

	if (!params->p.sqh_progressive_sequence)
		tmp = tmp / 2;
	dec->frame_dur = div_u64(96000ULL, tmp);
	dec->get_frame_dur = true;
	avs3_print(dec, PRINT_FLAG_VDEC_STATUS, "avs3 frame_dur:%d,progressive:%d\n",
		dec->frame_dur, params->p.sqh_progressive_sequence);
	return 0;
}

#define HEVC_MV_INFO   0x310d
#define HEVC_QP_INFO   0x3137
#define HEVC_SKIP_INFO 0x3136

static void get_qos_info(struct AVS3Decoder_s *dec, struct vframe_qos_s* vqos, int pic_number, int core_num)
{
	uint32_t blk88_y_count;
	uint32_t blk88_c_count;
	uint32_t blk22_mv_count;
	uint32_t rdata32;
	int32_t mv_hi;
	int32_t mv_lo;
	uint32_t rdata32_l;
	uint32_t mvx_L0_hi;
	uint32_t mvy_L0_hi;
	uint32_t mvx_L1_hi;
	uint32_t mvy_L1_hi;
	int64_t value;
	uint64_t temp_value;
	uint64_t ctrl_reg = HEVC_PIC_QUALITY_CTRL;
	uint64_t data_reg = HEVC_PIC_QUALITY_DATA;

	if (core_num == 1) {
		ctrl_reg = HEVC_PIC_QUALITY_CTRL_DBE1;
		data_reg = HEVC_PIC_QUALITY_DATA_DBE1;
	}

	/* set rd_idx to 0 */
	WRITE_VREG(ctrl_reg, 0);

	blk88_y_count = READ_VREG(data_reg);
	if (blk88_y_count == 0) {
		avs3_print(dec, AVS3_DBG_QOS_INFO,
			"[Picture %d Quality] NO Data yet.\n",
			pic_number);
		/* reset all counts */
		WRITE_VREG(ctrl_reg, (1 << 8));
		return;
	}

	/* qp_y_sum */
	rdata32 = READ_VREG(data_reg);
	avs3_print(dec, AVS3_DBG_QOS_INFO,
		"[Picture %d Quality] Y QP AVG:%d (%d/%d)\n",
		pic_number, rdata32/blk88_y_count, rdata32, blk88_y_count);
	vqos->avg_qp = rdata32 * 10 / blk88_y_count;

	/* intra_y_count */
	rdata32 = READ_VREG(data_reg);
	avs3_print(dec, AVS3_DBG_QOS_INFO,
		"[Picture %d Quality] Y intra rate:%d%c (%d)\n",
		pic_number, rdata32 * 100 / blk88_y_count, '%', rdata32);

	/* skipped_y_count */
	rdata32 = READ_VREG(data_reg);
	avs3_print(dec, AVS3_DBG_QOS_INFO,
		"[Picture %d Quality] Y skipped rate:%d%c (%d)\n",
		pic_number, rdata32 * 100 / blk88_y_count, '%', rdata32);
	vqos->avg_skip = rdata32 * 1000 / blk88_y_count;

	/* coeff_non_zero_y_count */
	rdata32 = READ_VREG(data_reg);
	avs3_print(dec, AVS3_DBG_QOS_INFO,
		"[Picture %d Quality] Y ZERO_Coeff rate:%d%c (%d)\n",
		pic_number, (100 - rdata32 * 100 / blk88_y_count), '%', rdata32);

	/* blk66_c_count */
	blk88_c_count = READ_VREG(data_reg);
	if (blk88_c_count == 0) {
		avs3_print(dec, AVS3_DBG_QOS_INFO,
			"[Picture %d Quality] NO Data yet.\n",
			pic_number);

		/* reset all counts */
		WRITE_VREG(ctrl_reg, (1 << 8));
		return;
	}

	/* qp_c_sum */
	rdata32 = READ_VREG(data_reg);
	avs3_print(dec, AVS3_DBG_QOS_INFO,
		"[Picture %d Quality] C QP AVG:%d (%d/%d)\n",
		pic_number, rdata32 / blk88_c_count, rdata32, blk88_c_count);

	/* intra_c_count */
	rdata32 = READ_VREG(data_reg);
	avs3_print(dec, AVS3_DBG_QOS_INFO,
		"[Picture %d Quality] C intra rate:%d%c (%d)\n",
		pic_number, rdata32 * 100 / blk88_c_count, '%', rdata32);

	/* skipped_cu_c_count */
	rdata32 = READ_VREG(data_reg);
	avs3_print(dec, AVS3_DBG_QOS_INFO,
		"[Picture %d Quality] C skipped rate:%d%c (%d)\n",
		pic_number, rdata32 * 100 / blk88_c_count, '%', rdata32);

	/* coeff_non_zero_c_count */
	rdata32 = READ_VREG(data_reg);
	avs3_print(dec, AVS3_DBG_QOS_INFO,
		"[Picture %d Quality] C ZERO_Coeff rate:%d%c (%d)\n",
		pic_number, (100 - rdata32 * 100 / blk88_c_count), '%', rdata32);

	/* 1'h0, qp_c_max[6:0], 1'h0, qp_c_min[6:0],
	1'h0, qp_y_max[6:0], 1'h0, qp_y_min[6:0] */
	rdata32 = READ_VREG(data_reg);
	avs3_print(dec, AVS3_DBG_QOS_INFO,
		"[Picture %d Quality] Y QP min:%d max:%d\n",
		pic_number, (rdata32 >> 0) & 0xff, (rdata32 >> 8) & 0xff);

	vqos->min_qp = (rdata32 >> 0) & 0xff;
	vqos->max_qp = (rdata32 >> 8) & 0xff;

	avs3_print(dec, AVS3_DBG_QOS_INFO,
		"[Picture %d Quality] C QP min:%d max:%d\n",
		pic_number, (rdata32 >> 16) & 0xff, (rdata32 >> 24) & 0xff);

	/* blk22_mv_count */
	blk22_mv_count = READ_VREG(data_reg);
	if (blk22_mv_count == 0) {
		avs3_print(dec, AVS3_DBG_QOS_INFO,
			"[Picture %d Quality] NO MV Data yet.\n",
			pic_number);

		/* reset all counts */
		WRITE_VREG(ctrl_reg, (1 << 8));
		return;
	}

	/* mvy_L1_count[39:32], mvx_L1_count[39:32],
		mvy_L0_count[39:32], mvx_L0_count[39:32] */
	rdata32 = READ_VREG(data_reg);

	/* should all be 0x00 or 0xff */
	avs3_print(dec, AVS3_DBG_QOS_INFO,
		"[Picture %d Quality] MV AVG High Bits:0x%X\n",
		pic_number, rdata32);

	mvx_L0_hi = ((rdata32 >> 0) & 0xff);
	mvy_L0_hi = ((rdata32 >> 8) & 0xff);
	mvx_L1_hi = ((rdata32 >> 16) & 0xff);
	mvy_L1_hi = ((rdata32 >> 24) & 0xff);

	/* mvx_L0_count[31:0] */
	rdata32_l = READ_VREG(data_reg);
	temp_value = mvx_L0_hi;
	temp_value = (temp_value << 32) | rdata32_l;

	if (mvx_L0_hi & 0x80)
		value = 0xFFFFFFF000000000 | temp_value;
	else
		value = temp_value;
	value = div_s64(value * 10, blk22_mv_count);
	avs3_print(dec, AVS3_DBG_QOS_INFO,
		"[Picture %d Quality] MVX_L0 AVG:%d (%lld/%d)\n",
		pic_number, div_s64(value, 10), div_s64(value, 10), blk22_mv_count);

	vqos->avg_mv = value;

	/* mvy_L0_count[31:0] */
	rdata32_l = READ_VREG(data_reg);
	temp_value = mvy_L0_hi;
	temp_value = (temp_value << 32) | rdata32_l;

	if (mvy_L0_hi & 0x80)
		value = 0xFFFFFFF000000000 | temp_value;
	else
		value = temp_value;

	avs3_print(dec, AVS3_DBG_QOS_INFO,
		"[Picture %d Quality] MVY_L0 AVG:%d (%lld/%d)\n",
		pic_number, rdata32_l / blk22_mv_count, value, blk22_mv_count);

	/* mvx_L1_count[31:0] */
	rdata32_l = READ_VREG(data_reg);
	temp_value = mvx_L1_hi;
	temp_value = (temp_value << 32) | rdata32_l;
	if (mvx_L1_hi & 0x80)
		value = 0xFFFFFFF000000000 | temp_value;
	else
		value = temp_value;

	avs3_print(dec, AVS3_DBG_QOS_INFO,
		"[Picture %d Quality] MVX_L1 AVG:%d (%lld/%d)\n",
		pic_number, rdata32_l / blk22_mv_count, value, blk22_mv_count);

	/* mvy_L1_count[31:0] */
	rdata32_l = READ_VREG(data_reg);
	temp_value = mvy_L1_hi;
	temp_value = (temp_value << 32) | rdata32_l;
	if (mvy_L1_hi & 0x80)
		value = 0xFFFFFFF000000000 | temp_value;
	else
		value = temp_value;

	avs3_print(dec, AVS3_DBG_QOS_INFO,
		"[Picture %d Quality] MVY_L1 AVG:%d (%lld/%d)\n",
		pic_number, rdata32_l/blk22_mv_count, value, blk22_mv_count);

	/* {mvx_L0_max, mvx_L0_min} // format : {sign, abs[14:0]}  */
	rdata32 = READ_VREG(data_reg);
	mv_hi = (rdata32 >> 16) & 0xffff;
	if (mv_hi & 0x8000)
		mv_hi = 0x8000 - mv_hi;
	vqos->max_mv = mv_hi;

	mv_lo = (rdata32 >> 0) & 0xffff;
	if (mv_lo & 0x8000)
		mv_lo = 0x8000 - mv_lo;
	vqos->min_mv = mv_lo;

	avs3_print(dec, AVS3_DBG_QOS_INFO,
		"[Picture %d Quality] MVX_L0 MAX:%d MIN:%d\n",
		pic_number, mv_hi, mv_lo);

	/* {mvy_L0_max, mvy_L0_min} */
	rdata32 = READ_VREG(data_reg);
	mv_hi = (rdata32 >> 16) & 0xffff;
	if (mv_hi & 0x8000)
		mv_hi = 0x8000 - mv_hi;

	mv_lo = (rdata32 >> 0) & 0xffff;
	if (mv_lo & 0x8000)
		mv_lo = 0x8000 - mv_lo;

	avs3_print(dec, AVS3_DBG_QOS_INFO,
		"[Picture %d Quality] MVY_L0 MAX:%d MIN:%d\n",
		pic_number, mv_hi, mv_lo);

	/* {mvx_L1_max, mvx_L1_min} */
	rdata32 = READ_VREG(data_reg);
	mv_hi = (rdata32 >> 16) & 0xffff;
	if (mv_hi & 0x8000)
		mv_hi = 0x8000 - mv_hi;

	mv_lo = (rdata32 >> 0) & 0xffff;
	if (mv_lo & 0x8000)
		mv_lo = 0x8000 - mv_lo;

	avs3_print(dec, AVS3_DBG_QOS_INFO,
		"[Picture %d Quality] MVX_L1 MAX:%d MIN:%d\n",
		pic_number, mv_hi, mv_lo);

	/* {mvy_L1_max, mvy_L1_min} */
	rdata32 = READ_VREG(data_reg);
	mv_hi = (rdata32 >> 16) & 0xffff;
	if (mv_hi & 0x8000)
		mv_hi = 0x8000 - mv_hi;

	mv_lo = (rdata32 >> 0) & 0xffff;
	if (mv_lo & 0x8000)
		mv_lo = 0x8000 - mv_lo;

	avs3_print(dec, AVS3_DBG_QOS_INFO,
		"[Picture %d Quality] MVY_L1 MAX:%d MIN:%d\n",
		pic_number, mv_hi, mv_lo);

	rdata32 = READ_VREG(ctrl_reg);
	avs3_print(dec, AVS3_DBG_QOS_INFO,
		"[Picture %d Quality] After Read:VDEC_PIC_QUALITY_CTRL: 0x%x\n",
		pic_number, rdata32);

	/* reset all counts */
	WRITE_VREG(ctrl_reg, (1 << 8));

}

/* only when we decoded one field or one frame,
we can call this function to get qos info*/
static void get_picture_qos_info(struct AVS3Decoder_s *dec, bool back_flag)
{
	struct avs3_frame_s *picture = NULL;
	struct vdec_s *vdec = hw_to_vdec(dec);
	struct avs3_decoder *avs3_dec = &dec->avs3_dec;
	struct vframe_qos_s vqos_0, vqos_1;

	if (back_flag) {
#ifdef NEW_FB_CODE
		picture = avs3_dec->next_be_decode_pic[avs3_dec->fb_rd_pos];
#endif
	} else {
		picture = dec->avs3_dec.cur_pic;
	}

	if (!picture) {
		avs3_print(dec, AVS3_DBG_BUFMGR_MORE,
			"%s decode picture is none exist\n");
		return;
	}

	if (vdec->mvfrm) {
#ifdef NEW_FB_CODE
		if (dec->front_back_mode != 0) {
			if (back_flag) {
				picture->hw_decode_time =
					local_clock() - vdec->hw_back_decode_start;
				if (picture->hw_decode_time < picture->hw_front_decode_time)
					picture->hw_decode_time = picture->hw_front_decode_time;

			} else {
				picture->hw_front_decode_time =
					local_clock() - vdec->hw_front_decode_start;
				return ;
			}
		} else
#endif
			picture->hw_decode_time =
				local_clock() - vdec->mvfrm->hw_decode_start;
	}

	avs3_print(dec, AVS3_DBG_QOS_INFO, "slice_type:%d, poc:%d hw_decode_time:%d\n",
		picture->slice_type, picture->poc, picture->hw_decode_time);

	if (get_cpu_major_id() < AM_MESON_CPU_MAJOR_ID_G12A) {
		unsigned char a[3];
		unsigned char i, j, t;
		unsigned long  data;

		data = READ_VREG(HEVC_MV_INFO);
		if (picture->slice_type == SLICE_I)
			data = 0;
		a[0] = data & 0xff;
		a[1] = (data >> 8) & 0xff;
		a[2] = (data >> 16) & 0xff;

		for (i = 0; i < 3; i++) {
			for (j = i+1; j < 3; j++) {
				if (a[j] < a[i]) {
					t = a[j];
					a[j] = a[i];
					a[i] = t;
				} else if (a[j] == a[i]) {
					a[i]++;
					t = a[j];
					a[j] = a[i];
					a[i] = t;
				}
			}
		}
		picture->vqos.max_mv = a[2];
		picture->vqos.avg_mv = a[1];
		picture->vqos.min_mv = a[0];
		avs3_print(dec, AVS3_DBG_QOS_INFO, "mv data %x  a[0]= %x a[1]= %x a[2]= %x\n",
			data, a[0], a[1], a[2]);

		data = READ_VREG(HEVC_QP_INFO);
		a[0] = data & 0x1f;
		a[1] = (data >> 8) & 0x3f;
		a[2] = (data >> 16) & 0x7f;

		for (i = 0; i < 3; i++)
			for (j = i+1; j < 3; j++) {
				if (a[j] < a[i]) {
					t = a[j];
					a[j] = a[i];
					a[i] = t;
				} else if (a[j] == a[i]) {
					a[i]++;
					t = a[j];
					a[j] = a[i];
					a[i] = t;
				}
			}
		picture->vqos.max_qp = a[2];
		picture->vqos.avg_qp = a[1];
		picture->vqos.min_qp = a[0];

		avs3_print(dec, AVS3_DBG_QOS_INFO, "qp data %x  a[0]= %x a[1]= %x a[2]= %x\n",
			data, a[0], a[1], a[2]);

		data = READ_VREG(HEVC_SKIP_INFO);
		a[0] = data & 0x1f;
		a[1] = (data >> 8) & 0x3f;
		a[2] = (data >> 16) & 0x7f;

		for (i = 0; i < 3; i++)
			for (j = i+1; j < 3; j++) {
				if (a[j] < a[i]) {
					t = a[j];
					a[j] = a[i];
					a[i] = t;
				} else if (a[j] == a[i]) {
					a[i]++;
					t = a[j];
					a[j] = a[i];
					a[i] = t;
				}
			}
		picture->vqos.max_skip = a[2];
		picture->vqos.avg_skip = a[1];
		picture->vqos.min_skip = a[0];

		avs3_print(dec, AVS3_DBG_QOS_INFO,
			"skip data %x  a[0]= %x a[1]= %x a[2]= %x\n",
			data, a[0], a[1], a[2]);
	} else {
		picture->vqos.max_mv = 0;
		picture->vqos.avg_mv = 0;
		picture->vqos.min_mv = 0;

		picture->vqos.max_skip = 0;
		picture->vqos.avg_skip = 0;
		picture->vqos.min_skip = 0;

		picture->vqos.max_qp = 0;
		picture->vqos.avg_qp = 0;
		picture->vqos.min_qp = 0;

		get_qos_info(dec, &vqos_0, picture->poc, 0);

		if (back_flag) {
#ifdef NEW_FB_CODE
			get_qos_info(dec, &vqos_1, picture->poc, 1);

			vqos_0.max_mv = max(vqos_0.max_mv, vqos_1.max_mv);
			vqos_0.min_mv = min(vqos_0.min_mv, vqos_1.min_mv);
			vqos_0.max_qp = max(vqos_0.max_qp, vqos_1.max_qp);
			vqos_0.min_qp = min(vqos_0.min_qp, vqos_1.min_qp);

			vqos_0.avg_qp = (vqos_0.avg_qp + vqos_1.avg_qp) / 20;
			vqos_0.avg_skip = (vqos_0.avg_skip + vqos_1.avg_skip) / 20;
			vqos_0.avg_mv = (vqos_0.avg_mv + vqos_1.avg_mv) / 20;
#endif
		} else {
			vqos_0.avg_qp = vqos_0.avg_qp / 10;
			vqos_0.avg_skip = vqos_0.avg_skip / 10;
			vqos_0.avg_mv = vqos_0.avg_mv / 10;
		}

		picture->vqos = vqos_0;
	}
}

#ifdef NEW_FB_CODE
irqreturn_t avs3_back_irq_cb(struct vdec_s *vdec, int irq)
{
	struct AVS3Decoder_s *dec =
		(struct AVS3Decoder_s *)vdec->private;

	ATRACE_COUNTER(dec->trace.decode_back_time_name, DECODER_ISR_PIC_DONE);
	dec->dec_status_back = READ_VREG(HEVC_DEC_STATUS_DBE);
	if (dec->dec_status_back == HEVC_BE_DECODE_DATA_DONE) {
		vdec_profile(hw_to_vdec(dec), VDEC_PROFILE_DECODER_END, CORE_MASK_HEVC_BACK);
	}

	/*BackEnd_Handle()*/
	if (dec->front_back_mode != 1) {
		avs3_print(dec, AVS3_DBG_IRQ_EVENT,
			"[BE] %s\n", __func__);
		if (dec->front_back_mode == 3)
			dec->dec_status_back = HEVC_BE_DECODE_DATA_DONE;
		return IRQ_WAKE_THREAD;
	}

#if 0
	if (debug & AVS3_DBG_IRQ_EVENT)
		avs3_print(dec, 0,
			"[BE] avs3 back isr dec_status_back  = 0x%x\n",
			dec->dec_status_back
		);
	if (READ_VREG(DEBUG_REG1_DBE)) {
		pr_info("[BE] dbg%x: %x, HEVC_SAO_CRC %x HEVC_SAO_CRC_DBE1 %x\n", READ_VREG(DEBUG_REG1_DBE),
		READ_VREG(DEBUG_REG2_DBE),
		READ_VREG(HEVC_SAO_CRC),
		READ_VREG(HEVC_SAO_CRC_DBE1)
		);
		WRITE_VREG(DEBUG_REG1_DBE, 0);
	}
#else
	if (READ_VREG(DEBUG_REG1_DBE)) {
#ifdef USE_FRONT_ISR_HANDLE_FOR_BACK
		WRITE_VREG(dec->ASSIST_MBOX0_IRQ_REG, 0x1);
		ATRACE_COUNTER(dec->trace.decode_back_time_name, DECODER_ISR_END);
		return IRQ_HANDLED;
#else
		ATRACE_COUNTER(dec->trace.decode_back_time_name, DECODER_ISR_END);
		return IRQ_WAKE_THREAD;
#endif
	}
#endif
	if (dec->dec_status_back == AVS3_DEC_IDLE) {
		ATRACE_COUNTER(dec->trace.decode_back_time_name, DECODER_ISR_END);
		return IRQ_HANDLED;
	}
	/**/
#ifdef USE_FRONT_ISR_HANDLE_FOR_BACK
	WRITE_VREG(dec->ASSIST_MBOX0_IRQ_REG, 0x1);
	ATRACE_COUNTER(dec->trace.decode_back_time_name, DECODER_ISR_END);
	return IRQ_HANDLED;
#else
	ATRACE_COUNTER(dec->trace.decode_back_time_name, DECODER_ISR_END);
	reset_process_time_back(dec);
	return IRQ_WAKE_THREAD;
#endif
}

irqreturn_t vavs3_back_isr_thread_fn(struct AVS3Decoder_s *dec)
{
	unsigned int dec_status = dec->dec_status_back;
	struct avs3_decoder *avs3_dec = &dec->avs3_dec;
	struct vdec_s *vdec = hw_to_vdec(dec);
	int j;
	//unsigned long flags;
	ATRACE_COUNTER(dec->trace.decode_back_time_name, DECODER_ISR_THREAD_PIC_DONE_START);

	if (debug & AVS3_DBG_IRQ_EVENT)
		avs3_print(dec, 0,
			"[BE] avs3 back isr dec_status_back  = 0x%x\n",
			dec->dec_status_back);
	if (READ_VREG(DEBUG_REG1_DBE)) {
		avs3_print(dec, 0, "[BE] dbg%x: %x\n",
			READ_VREG(DEBUG_REG1_DBE), READ_VREG(DEBUG_REG2_DBE));
		WRITE_VREG(DEBUG_REG1_DBE, 0);
	}

	/*simulation code: if (READ_VREG(HEVC_DEC_STATUS_DBE)==HEVC_BE_DECODE_DATA_DONE)*/
	if (dec_status == HEVC_BE_DECODE_DATA_DONE || dec->front_back_mode == 2) {
		struct avs3_frame_s *pic = avs3_dec->next_be_decode_pic[avs3_dec->fb_rd_pos];

		get_picture_qos_info(dec, true);

		dec_update_gvs(dec);
		/*count info*/
		vdec_count_info(dec->gvs, 2, pic->stream_offset);
		dec->gvs->bit_depth_luma = pic->depth;
		dec->gvs->bit_depth_chroma = pic->depth;
		dec->gvs->double_write_mode = pic->double_write_mode;

		reset_process_time_back(dec);
		vdec->back_pic_done = true;
		avs3_print(dec, PRINT_FLAG_VDEC_STATUS,
			"BackEnd data done %d, fb_rd_pos %d, pic index %d, poc %d HEVC_SAO_CRC %x HEVC_SAO_CRC_DBE1 %x\n",
			avs3_dec->backend_decoded_count, avs3_dec->fb_rd_pos, pic->index, pic->poc,
			READ_VREG(HEVC_SAO_CRC), READ_VREG(HEVC_SAO_CRC_DBE1));

		if (debug & AVS3_DBG_BUFMGR_DETAIL) {
			avs3_print(dec, 0,
				"CM_BODY_START_ADDR 0x%x CM_HEADER_START_ADDR 0x%x SAO_Y_START_ADDR 0x%x SAO_Y_LENGTH 0x%x\n",
				READ_VREG(HEVC_CM_BODY_START_ADDR), READ_VREG(HEVC_CM_HEADER_START_ADDR),
				READ_VREG(HEVC_SAO_Y_START_ADDR), READ_VREG(HEVC_SAO_Y_LENGTH));

			avs3_print(dec, 0,
				"SAO_C_START_ADDR 0x%x SAO_C_LENGTH 0x%x SAO_Y_WPTR 0x%x SAO_C_WPTR 0x%x\n",
				READ_VREG(HEVC_SAO_C_START_ADDR), READ_VREG(HEVC_SAO_C_LENGTH),
				READ_VREG(HEVC_SAO_Y_WPTR), READ_VREG(HEVC_SAO_C_WPTR));

			avs3_print(dec, 0,
				"SAO_CTRL0 0x%x SAO_CTRL1 0x%x SAO_CTRL2 0x%x SAO_CTRL3 0x%x SAO_CTRL4 0x%x SAO_CTRL5 0x%x\n",
				READ_VREG(HEVC_SAO_CTRL0), READ_VREG(HEVC_SAO_CTRL1), READ_VREG(HEVC_SAO_CTRL2),
				READ_VREG(HEVC_SAO_CTRL3), READ_VREG(HEVC_SAO_CTRL4), READ_VREG(HEVC_SAO_CTRL5));

			avs3_print(dec, 0,
				"CM_BODY_START_ADDR_DBE1 0x%x CM_HEADER_START_ADDR_DBE1 0x%x SAO_Y_START_ADDR_DBE1 0x%x SAO_Y_LENGTH_DBE1 0x%x\n",
				READ_VREG(HEVC_CM_BODY_START_ADDR_DBE1), READ_VREG(HEVC_CM_HEADER_START_ADDR_DBE1),
				READ_VREG(HEVC_SAO_Y_START_ADDR_DBE1), READ_VREG(HEVC_SAO_Y_LENGTH_DBE1));

			avs3_print(dec, 0,
				"SAO_C_START_ADDR_DBE1 0x%x SAO_C_LENGTH_DBE1 0x%x SAO_Y_WPTR_DBE1 0x%x SAO_C_WPTR_DBE1 0x%x\n",
				READ_VREG(HEVC_SAO_C_START_ADDR_DBE1), READ_VREG(HEVC_SAO_C_LENGTH_DBE1),
				READ_VREG(HEVC_SAO_Y_WPTR_DBE1), READ_VREG(HEVC_SAO_C_WPTR_DBE1));

			avs3_print(dec, 0,
				"SAO_CTRL0_DBE1 0x%x SAO_CTRL1_DBE1 0x%x SAO_CTRL2_DBE1 0x%x SAO_CTRL3_DBE1 0x%x SAO_CTRL4_DBE1 0x%x SAO_CTRL5_DBE1 0x%x\n",
				READ_VREG(HEVC_SAO_CTRL0_DBE1), READ_VREG(HEVC_SAO_CTRL1_DBE1), READ_VREG(HEVC_SAO_CTRL2_DBE1),
				READ_VREG(HEVC_SAO_CTRL3_DBE1), READ_VREG(HEVC_SAO_CTRL4_DBE1), READ_VREG(HEVC_SAO_CTRL5_DBE1));
		}

		if (fbdebug_flag & 0x70000000) {
			u8 cmd = (fbdebug_flag >> 28) & 0x7;
			u32 dump_c = (fbdebug_flag >> 16) & 0xfff;
			if ((cmd == 3) ||
				(cmd == 1 && dump_c == avs3_dec->backend_decoded_count) ||
				(cmd == 2 && avs3_dec->backend_decoded_count < dump_c))
				//10000, make log different from dump_loop_buffer after FrontEnd data done.
				dump_loop_buffer(dec, 10000 + avs3_dec->backend_decoded_count, cmd ==1 || cmd == 2);
		}

		if (dec->front_back_mode == 1) {
			avs3_print(dec, PRINT_FLAG_VDEC_DETAIL,
				"MMU0 b cur addr : 0x%x\n", READ_VREG(HEVC_ASSIST_FBD_MMU_MAP_ADDR0));
			avs3_print(dec, PRINT_FLAG_VDEC_DETAIL,
				"MMU1 b cur addr : 0x%x\n", READ_VREG(HEVC_ASSIST_FBD_MMU_MAP_ADDR1));
			WRITE_VREG(HEVC_DEC_STATUS_DBE, AVS3_DEC_IDLE);
			WRITE_VREG(HEVC_ASSIST_FB_PIC_CLR, 2);
		}
		if (debug & AVS3_DBG_PRINT_PIC_LIST) {
			print_pic_pool(avs3_dec, "before dec backend_ref");
			avs3_print_cont(dec, 0, "pic_cfg index %d, list0 index: ", pic->index);
			for (j = 0; j < pic->list0_num_refp; j++)
				avs3_print_cont(dec, 0, "%d ", pic->list0_index[j]);
			avs3_print_cont(dec, 0, "list1 index: ");
			for (j = 0; j < pic->list1_num_refp; j++)
				avs3_print_cont(dec, 0, "%d ", pic->list1_index[j]);
			avs3_print_cont(dec, 0, "\n");
		}

#if 0
#ifdef AVS3_10B_MMU
		release_unused_4k(&avs3_mmumgr_0, pic->index);
		release_unused_4k(&avs3_mmumgr_1, pic->index);
#endif
#ifdef AVS3_10B_MMU_DW
		release_unused_4k(&avs3_mmumgr_dw0, pic->index); // new dual
		release_unused_4k(&avs3_mmumgr_dw1, pic->index); // new dual
#endif
#else
		if ((dec->front_back_mode == 1 ||
			dec->front_back_mode == 3
			)  && (debug & AVS3_DBG_NOT_RECYCLE_MMU_TAIL) == 0) {
			/*if (dec->is_used_v4l) {
				// to do
			} else {*/
				unsigned used_4k_num0;
				unsigned used_4k_num1;
				used_4k_num0 = READ_VREG(HEVC_SAO_MMU_STATUS) >> 16;
				if (dec->front_back_mode == 3)
					used_4k_num1 = used_4k_num0;
				else
					used_4k_num1 = READ_VREG(HEVC_SAO_MMU_STATUS_DBE1) >> 16;
				avs3_print(dec, AVS3_DBG_BUFMGR_MORE,
					"%s decoder_mmu_box_free_idx_tail index=%d, core0 %d core1 %d\n",
					__func__, pic->index,
					used_4k_num0, used_4k_num1);
				decoder_mmu_box_free_idx_tail(
						dec->mmu_box,
						pic->index,
						used_4k_num0);
				decoder_mmu_box_free_idx_tail(
						dec->mmu_box_1,
						pic->index,
						used_4k_num1);
				if (dec->dw_mmu_enable) {
					used_4k_num0 = READ_VREG(HEVC_SAO_MMU_STATUS2) >> 16;
					if (dec->front_back_mode == 3)
						used_4k_num1 = used_4k_num0;
					else
						used_4k_num1 = READ_VREG(HEVC_SAO_MMU_STATUS2_DBE1) >> 16;
					avs3_print(dec, AVS3_DBG_BUFMGR_MORE,
						"%s DW decoder_mmu_box_free_idx_tail index=%d, core0 %d core1 %d\n",
						__func__, pic->index,
						used_4k_num0, used_4k_num1);
					decoder_mmu_box_free_idx_tail(
							dec->dw_mmu_box,
							pic->index,
							used_4k_num0);
					decoder_mmu_box_free_idx_tail(
							dec->dw_mmu_box_1,
							pic->index,
							used_4k_num1);
				}
		}
#endif

		if (without_display_mode == 0) {
			struct vframe_s *vf = NULL;
			if (kfifo_peek(&dec->display_q, &vf) && vf) {
				uint8_t index = vf->index & 0xff;
				struct avs3_frame_s *peek_pic = get_pic_by_index(dec, index);

				if (peek_pic == pic)
					vf_notify_receiver(dec->provider_name,
						VFRAME_EVENT_PROVIDER_VFRAME_READY, NULL);
			}
		} else
			vavs3_vf_put(vavs3_vf_get(dec), dec);

		pic_backend_ref_operation(dec, pic, 0);

		if (debug & AVS3_DBG_PRINT_PIC_LIST)
			print_pic_pool(avs3_dec, "after dec backend_ref");

		if (dec->front_back_mode == 1 ||
			dec->front_back_mode == 3)
		release_free_mmu_buffers(dec);

		dec->dec_back_result = DEC_BACK_RESULT_DONE;
		ATRACE_COUNTER(dec->trace.decode_back_time_name, DECODER_ISR_THREAD_EDN);
		vdec_schedule_work(&dec->work_back);
		if (avs3_dec->front_pause_flag) {
			/*multi pictures in one packe*/
			WRITE_VREG(dec->ASSIST_MBOX0_IRQ_REG,
						0x1);
		}
	}
	//unlock_front_back(dec, flags);

	return IRQ_HANDLED;
}

irqreturn_t avs3_back_threaded_irq_cb(struct vdec_s *vdec, int irq)
{
	struct AVS3Decoder_s *dec =
		(struct AVS3Decoder_s *)vdec->private;
	irqreturn_t ret;
	//unsigned long flags;
	//lock_front_back(dec, flags);
	ret = vavs3_back_isr_thread_fn(dec);
	//unlock_front_back(dec, flags);
	return ret;
}

#endif

static void handle_ucode_dbg(struct AVS3Decoder_s *dec, uint debug_tag)
{

	int i;
	if (debug_tag & 0x10000) {
		dma_sync_single_for_cpu(
			amports_get_dma_device(),
			dec->lmem_phy_addr,
			LMEM_BUF_SIZE,
			DMA_FROM_DEVICE);

		pr_info("LMEM<tag %x>:\n", READ_HREG(DEBUG_REG1));
		for (i = 0; i < 0x400; i += 4) {
			int ii;
			if ((i & 0xf) == 0)
				pr_info("%03x: ", i);
			for (ii = 0; ii < 4; ii++) {
				pr_info("%04x ",
					   dec->lmem_ptr[i + 3 - ii]);
			}
			if (((i + ii) & 0xf) == 0)
				pr_info("\n");
		}

		if (((udebug_pause_pos & 0xffff)
			== (debug_tag & 0xffff)) &&
			(udebug_pause_decode_idx == 0 ||
			udebug_pause_decode_idx == dec->decode_idx) &&
			(udebug_pause_val == 0 ||
			udebug_pause_val == READ_HREG(DEBUG_REG2))) {
			udebug_pause_pos &= 0xffff;
			dec->ucode_pause_pos = udebug_pause_pos;
		} else if (debug_tag & 0x20000)
			dec->ucode_pause_pos = 0xffffffff;
		if (dec->ucode_pause_pos)
			reset_process_time(dec);
		else
			WRITE_VREG(DEBUG_REG1, 0);
	} else if (debug_tag != 0) {
		pr_info(
			"dbg%x: %x lcu %x stream crc %x, shiftbytes 0x%x decbytes 0x%x\n", READ_HREG(DEBUG_REG1),
				READ_HREG(DEBUG_REG2),
				READ_VREG(HEVC_PARSER_LCU_START),
			   READ_VREG(HEVC_STREAM_CRC), READ_VREG(HEVC_SHIFT_BYTE_COUNT),
				READ_VREG(HEVC_SHIFT_BYTE_COUNT) -
				dec->start_shift_bytes);
		if ((debug & AVS3_DBG_SAO_CRC) &&
			(get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_S5)) {
			unsigned old_crc = READ_VREG(HEVC_SAO_CRC);
			int count = 0;
			while (1) {
				unsigned crc;
				crc = READ_VREG(HEVC_SAO_CRC);
				if (crc == old_crc) {
					pr_info("stable HEVC_SAO_CRC = %x, count=%d\n", crc, count);
					break;
				}
				count++;
				old_crc = crc;
				if (count > 100) {
					pr_info("read HEVC_SAO_CRC = %x\n", crc);
					count = 0;
				}
			}
		}

		if (((udebug_pause_pos & 0xffff)
			== (debug_tag & 0xffff)) &&
			(udebug_pause_decode_idx == 0 ||
			udebug_pause_decode_idx == dec->decode_idx) &&
			(udebug_pause_val == 0 ||
			udebug_pause_val == READ_HREG(DEBUG_REG2))) {
			udebug_pause_pos &= 0xffff;
			dec->ucode_pause_pos = udebug_pause_pos;
		}
		if (dec->ucode_pause_pos)
			reset_process_time(dec);
		else
			WRITE_VREG(DEBUG_REG1, 0);
		/*if (dec->dec_status) {
			return IRQ_WAKE_THREAD;
		} else*/ {
			//dec->process_busy = 0;
		}
	}
}

static irqreturn_t vavs3_isr_thread_fn(int irq, void *data)
{
	struct AVS3Decoder_s *dec = (struct AVS3Decoder_s *)data;
	DEC_CTX *ctx = &dec->avs3_dec.ctx;
	unsigned int dec_status = dec->dec_status;
	int i, ret;
	int32_t start_code = 0;
#ifdef NEW_FB_CODE
	struct avs3_decoder *avs3_dec = &dec->avs3_dec;
#endif
	unsigned char avs3_bi_mid_ptr;
	struct vdec_s *vdec = hw_to_vdec(dec);

#if 0
	avs3_print(dec, AVS3_DBG_BUFMGR_MORE,
		"%s decode_status 0x%x process_state %d lcu 0x%x\n",
		__func__, dec_status, dec->process_state,
		READ_VREG(HEVC_PARSER_LCU_START));
#else
	if (debug & AVS3_DBG_IRQ_EVENT) {
		if (dec->front_back_mode != 1)
			avs3_print(dec, 0,
				"avs3 isr dec status  = 0x%x, debug_tag 0x%x, lcu 0x%x shiftbyte 0x%x (%x %x lev %x, wr %x, rd %x)\n",
				dec_status, READ_HREG(DEBUG_REG1), READ_VREG(HEVC_PARSER_LCU_START),
				READ_VREG(HEVC_SHIFT_BYTE_COUNT),
				READ_VREG(HEVC_STREAM_START_ADDR),
				READ_VREG(HEVC_STREAM_END_ADDR),
				READ_VREG(HEVC_STREAM_LEVEL),
				READ_VREG(HEVC_STREAM_WR_PTR),
				READ_VREG(HEVC_STREAM_RD_PTR)
			);
		else
			avs3_print(dec, 0,
				"avs3 isr dec status  = 0x%x, debug_tag 0x%x, lcu 0x%x shiftbyte 0x%x (%x %x lev %x, wr %x, rd %x) [BE] pc %x psr %x DEC_STATUS %x\n",
				dec_status, READ_HREG(DEBUG_REG1), READ_VREG(HEVC_PARSER_LCU_START),
				READ_VREG(HEVC_SHIFT_BYTE_COUNT),
				READ_VREG(HEVC_STREAM_START_ADDR),
				READ_VREG(HEVC_STREAM_END_ADDR),
				READ_VREG(HEVC_STREAM_LEVEL),
				READ_VREG(HEVC_STREAM_WR_PTR),
				READ_VREG(HEVC_STREAM_RD_PTR),
				READ_VREG(HEVC_MPC_E_DBE),
			READ_VREG(HEVC_MPSR_DBE),
			READ_VREG(HEVC_DEC_STATUS_DBE)
			);
	}

	if (udebug_flag) {
		uint debug_tag = READ_HREG(DEBUG_REG1);

		if (debug_tag != 0) {
			handle_ucode_dbg(dec, debug_tag);
			//dec->process_busy = 0;
			//return IRQ_HANDLED;
		}
	}

#endif

	if ((dec_status == AVS3_HEAD_PIC_I_READY) ||
		(dec_status == AVS3_HEAD_PIC_PB_READY)) {
		decoder_trace(dec->trace.decode_time_name, DECODER_ISR_THREAD_HEAD_START, TRACE_BASIC);
	} else if (dec_status == HEVC_DECPIC_DATA_DONE) {
		decoder_trace(dec->trace.decode_time_name, DECODER_ISR_THREAD_PIC_DONE_START, TRACE_BASIC);
	}

#ifndef G12A_BRINGUP_DEBUG
	if (dec->eos) {
		goto irq_handled_exit;
	}
#endif
	dec->wait_buf = 0;
	if (dec_status == AVS3_DECODE_BUFEMPTY) {
		if (dec->m_ins_flag) {
			if (!vdec_frame_based(hw_to_vdec(dec)))
				dec_again_process(dec);
			else {
				dec->dec_result = DEC_RESULT_DONE;
#ifdef NEW_FB_CODE
				if (dec->front_back_mode == 1) {
					amhevc_stop_f();
				} else
#endif
				amhevc_stop();
				vdec_schedule_work(&dec->work);
			}
		}
		goto irq_handled_exit;
	} else if ((dec_status == HEVC_DECPIC_DATA_DONE) ||
		(dec_status == HEVC_DECPIC_DATA_ERROR)) {
		struct avs3_frame_s *pic = dec->avs3_dec.cur_pic;
		u32 shiftbytes = 0;

		if (pic != NULL)
			pic->poc = ctx->info.pic_header.dtr;
		mutex_lock(&dec->slice_header_lock);
		mutex_unlock(&dec->slice_header_lock);
		if (dec_status == HEVC_DECPIC_DATA_DONE)
			vdec->front_pic_done = true;

		if (pic && (vdec_stream_based(hw_to_vdec(dec)))
			&& (dec_status == HEVC_DECPIC_DATA_DONE))
			pic->stream_size = vdec_get_stream_size(hw_to_vdec(dec));

		WRITE_VREG(HEVC_PARSER_DEBUG_IDX, 0xb);
		shiftbytes = READ_VREG(HEVC_PARSER_DEBUG_DAT) & 0xffff;
		if ((shiftbytes > over_decoder_shiftbytes) && (pic != NULL)) {
			pic->error_mark = 1;
			avs3_print(dec, PRINT_FLAG_VDEC_STATUS, "%s: over decoder(0x%x),set pic error\n",
				__func__, shiftbytes);
		}

		if (dec->front_back_mode == 0) {
			avs3_print(dec, PRINT_FLAG_VDEC_STATUS,
				"HEVC_DECPIC_DATA_DONE: decode_idx %d stream crc %x shiftcnt=0x%x, HEVC_SAO_CRC %x HEVC_SAO_MMU_STATUS %x\n",
				dec->frame_count,
				READ_VREG(HEVC_STREAM_CRC),
				READ_VREG(HEVC_SHIFT_BYTE_COUNT),
				READ_VREG(HEVC_SAO_CRC),
				READ_VREG(HEVC_SAO_MMU_STATUS));

			if (debug & AVS3_DBG_BUFMGR_DETAIL) {
				pr_info("HEVC_CM_BODY_START_ADDR = %x\n", READ_VREG(HEVC_CM_BODY_START_ADDR));
				pr_info("HEVC_CM_HEADER_START_ADDR = %x\n", READ_VREG(HEVC_CM_HEADER_START_ADDR));
				pr_info("HEVC_SAO_Y_START_ADDR = %x\n", READ_VREG(HEVC_SAO_Y_START_ADDR));
				pr_info("HEVC_SAO_Y_LENGTH = %x\n", READ_VREG(HEVC_SAO_Y_LENGTH));
				pr_info("HEVC_SAO_C_START_ADDR = %x\n", READ_VREG(HEVC_SAO_C_START_ADDR));
				pr_info("HEVC_SAO_C_LENGTH = %x\n", READ_VREG(HEVC_SAO_C_LENGTH));
				pr_info("HEVC_SAO_Y_WPTR = %x\n", READ_VREG(HEVC_SAO_Y_WPTR));
				pr_info("HEVC_SAO_C_WPTR = %x\n", READ_VREG(HEVC_SAO_C_WPTR));

				pr_info("HEVC_SAO_CTRL0 = %x\n", READ_VREG(HEVC_SAO_CTRL0));
				pr_info("HEVC_SAO_CTRL1 = %x\n", READ_VREG(HEVC_SAO_CTRL1));
				pr_info("HEVC_SAO_CTRL2 = %x\n", READ_VREG(HEVC_SAO_CTRL2));
				pr_info("HEVC_SAO_CTRL3 = %x\n", READ_VREG(HEVC_SAO_CTRL3));
				pr_info("HEVC_SAO_CTRL4 = %x\n", READ_VREG(HEVC_SAO_CTRL4));
				pr_info("HEVC_SAO_CTRL5 = %x\n", READ_VREG(HEVC_SAO_CTRL5));

			}
			if ((debug & AVS3_DBG_NOT_RECYCLE_MMU_TAIL) == 0)
				avs3_recycle_mmu_buf_tail(dec);
		} else {
			if (pic != NULL)
				avs3_print(dec, PRINT_FLAG_VDEC_STATUS,
					"FrontEnd data done %d, fb_rd_pos %d, pic index %d, poc %d stream crc %x shiftbyte %x, shiftstatus 0x%x\n",
					avs3_dec->frontend_decoded_count, avs3_dec->fb_rd_pos, pic->index, pic->poc,
					READ_VREG(HEVC_STREAM_CRC), READ_VREG(HEVC_SHIFT_BYTE_COUNT), READ_VREG(HEVC_SHIFT_STATUS));
			else
				avs3_print(dec, PRINT_FLAG_VDEC_STATUS,
					"FrontEnd data done %d, fb_rd_pos %d, stream crc %x shiftbyte %x\n",
					avs3_dec->frontend_decoded_count, avs3_dec->fb_rd_pos,
					READ_VREG(HEVC_STREAM_CRC), READ_VREG(HEVC_SHIFT_BYTE_COUNT));
		}

		dec->start_decoding_flag |= 0x3;
		if (dec->m_ins_flag) {
			if (dec_status == HEVC_DECPIC_DATA_ERROR &&
				(pic != NULL)) {
				pic->error_mark = 1;
			}

			set_cuva_data(dec);
			update_decoded_pic(dec);
			check_pic_error(dec, pic);
			get_picture_qos_info(dec, false);
			dec->dec_result = DEC_RESULT_DONE;
#ifdef NEW_FB_CODE
			if (dec->front_back_mode) {
				/*simulation code: if (dec_status == HEVC_DECPIC_DATA_DONE) {*/
				if (pic != NULL)
					front_decpic_done_update(dec, 1); /*not multi pictures in one packe*/
			} else
#endif
			amhevc_stop();
			decoder_trace(dec->trace.decode_time_name, DECODER_ISR_THREAD_EDN, TRACE_BASIC);
			avs3_work_implement(dec);
		}

		goto irq_handled_exit;
	}
#if 0
	if (dec_status == AVS3_EOS) {
		if (dec->m_ins_flag)
			reset_process_time(dec);

		avs3_print(dec, AVS3_DBG_BUFMGR,
			"AVS3_EOS, flush buffer\r\n");

		avs3_post_process(&dec->avs3_dec);
		avs3_prepare_display_buf(dec);

		avs3_print(dec, AVS3_DBG_BUFMGR,
			"send AVS3_10B_DISCARD_NAL\r\n");
		WRITE_VREG(HEVC_DEC_STATUS_REG, AVS3_10B_DISCARD_NAL);
		if (dec->m_ins_flag) {
			update_decoded_pic(dec);
			dec->dec_result = DEC_RESULT_DONE;
			amhevc_stop();
			vdec_schedule_work(&dec->work);
		}
		goto irq_handled_exit;
	} else
#endif
	if (dec_status == AVS3_DECODE_OVER_SIZE) {
		avs3_print(dec, 0,
			"avs3  decode oversize !!\n");
		debug |= (AVS3_DBG_DIS_LOC_ERROR_PROC |
			AVS3_DBG_DIS_SYS_ERROR_PROC);
		dec->fatal_error |= DECODER_FATAL_ERROR_SIZE_OVERFLOW;
		goto irq_handled_exit;
	}

	if (dec_status == AVS3_HEAD_SEQ_READY)
		start_code = SEQUENCE_HEADER_CODE;
	else if (dec_status == AVS3_HEAD_PIC_I_READY)
		start_code = I_PICTURE_START_CODE;
	else if (dec_status == AVS3_HEAD_PIC_PB_READY)
		start_code = PB_PICTURE_START_CODE;
	else if (dec_status == AVS3_HEAD_SEQ_END_READY)
		start_code = SEQUENCE_END_CODE;
	else if (dec_status == AVS3_STARTCODE_SEARCH_DONE)
		/*VIDEO_EDIT_CODE*/
		start_code = READ_VREG(CUR_NAL_UNIT_TYPE);
	else
		goto irq_handled_exit_and_start_timer;

	if (dec->process_state ==
			PROC_STATE_HEAD_AGAIN
			) {
		if ((start_code == I_PICTURE_START_CODE)
		|| (start_code == PB_PICTURE_START_CODE)) {
			avs3_print(dec, 0,
				"PROC_STATE_HEAD_AGAIN error, start_code 0x%x!!!\r\n",
				start_code);
			goto irq_handled_exit_and_start_timer;
		} else {
			avs3_print(dec, AVS3_DBG_BUFMGR,
				"PROC_STATE_HEAD_AGAIN, start_code 0x%x\r\n",
				start_code);
			dec->process_state = PROC_STATE_HEAD_DONE;
			WRITE_VREG(HEVC_DEC_STATUS_REG, AVS3_ACTION_DONE);
			goto irq_handled_exit_and_start_timer;
		}
	} else if (dec->process_state ==
			PROC_STATE_DECODE_AGAIN) {
		if ((start_code == I_PICTURE_START_CODE)
		|| (start_code == PB_PICTURE_START_CODE)) {
			avs3_print(dec, AVS3_DBG_BUFMGR,
				"PROC_STATE_DECODE_AGAIN=> decode_slice, start_code 0x%x\r\n",
				start_code);
			if ((dec->front_back_mode == 1) && (dec->pic_list_init_flag == 0) && (dec->pic_list_wait_alloc_done_flag == BUFFER_ALLOCATE_DONE)) {
				dec->pic_list_init_flag = 1;
				goto alloc_buffer_done;
			}
			else
				goto decode_slice;
		} else {
			avs3_print(dec, 0,
				"PROC_STATE_DECODE_AGAIN, start_code 0x%x!!!\r\n",
				start_code);
			WRITE_VREG(HEVC_DEC_STATUS_REG, AVS3_ACTION_DONE);
			goto irq_handled_exit_and_start_timer;
		}
	}

	if ((start_code == I_PICTURE_START_CODE)
		|| (start_code == PB_PICTURE_START_CODE)
		|| (start_code == SEQUENCE_END_CODE)
		|| (start_code == VIDEO_EDIT_CODE)) {

		if (dec->avs3_dec.cur_pic != NULL) {
#ifndef NEW_FB_CODE
			int32_t ii;
#endif
			if (debug & AVS3_DBG_PRINT_PIC_LIST)
				print_pic_pool(avs3_dec, "before post process");

			ret = avs3_bufmgr_post_process(avs3_dec);

			if (ret == 2) {
				avs3_print(dec, AVS3_DBG_BUFMGR, "avs3 same poc %d\n",
					avs3_dec->ctx.ptr);
				pic_backend_ref_operation(dec, dec->avs3_dec.cur_pic, 0);
			}

			if (debug & AVS3_DBG_PRINT_PIC_LIST)
				print_pic_pool(&dec->avs3_dec, "after post_process");

			if ((dec->front_back_mode != 1) || !efficiency_mode)
				avs3_prepare_display_buf(dec);
			dec->avs3_dec.cur_pic = NULL;
#ifdef NEW_FB_CODE
			//release_free_mmu_buffers(dec);
#else
#endif
		}
	}

	if ((dec_status == AVS3_HEAD_PIC_I_READY)
		|| (dec_status == AVS3_HEAD_PIC_PB_READY)) {

		if (debug & AVS3_DBG_SEND_PARAM_WITH_REG) {
			get_rpm_param(
				&dec->avs3_dec.param);
		} else {
			decoder_trace(dec->trace.decode_header_memory_time_name, TRACE_HEADER_RPM_START, TRACE_BASIC);
			for (i = 0; i < (RPM_VALID_END - RPM_BEGIN); i += 4) {
				int ii;
				for (ii = 0; ii < 4; ii++)
					dec->avs3_dec.param.l.data[i + ii] =
						dec->rpm_ptr[i + 3 - ii];
			}
			decoder_trace(dec->trace.decode_header_memory_time_name, TRACE_HEADER_RPM_END, TRACE_BASIC);
		}
#if 0 //def SANITY_CHECK
		if (dec->avs3_dec.param.p.num_of_ref_cur >
			dec->avs3_dec.ref_maxbuffer) {
			pr_info("Warning: Wrong num_of_ref_cur %d, force to %d\n",
				dec->avs3_dec.param.p.num_of_ref_cur,
				dec->avs3_dec.ref_maxbuffer);
			dec->avs3_dec.param.p.num_of_ref_cur =
				dec->avs3_dec.ref_maxbuffer;
		}
#endif

		debug_buffer_mgr_more(dec);
		get_frame_rate(&dec->avs3_dec.param, dec);

		if (dec->avs3_dec.param.p.video_signal_type & (1<<14)) {
			union param_u *pPara;

			avs3_print(dec, AVS3_DBG_HDR_INFO, "avs3 HDR meta data present\n");
			pPara = &dec->avs3_dec.param;

			/*clean this flag*/
			pPara->p.video_signal_type &= ~(1<<14);

			dec->vf_dp.present_flag = 1;

			dec->vf_dp.white_point[0]
				= pPara->p.white_point_x;
			avs3_print(dec, AVS3_DBG_HDR_INFO,
				"white_point[0]:0x%x\n",
				dec->vf_dp.white_point[0]);

			dec->vf_dp.white_point[1]
				= pPara->p.white_point_y;
			avs3_print(dec, AVS3_DBG_HDR_INFO,
				"white_point[1]:0x%x\n",
				dec->vf_dp.white_point[1]);

			for (i = 0; i < 3; i++) {
				dec->vf_dp.primaries[i][0]
					= pPara->p.display_primaries_x[i];
				avs3_print(dec, AVS3_DBG_HDR_INFO,
					"primaries[%d][0]:0x%x\n",
					i,
					dec->vf_dp.primaries[i][0]);
			}

			for (i = 0; i < 3; i++) {
				dec->vf_dp.primaries[i][1]
					= pPara->p.display_primaries_y[i];
				avs3_print(dec, AVS3_DBG_HDR_INFO,
					"primaries[%d][1]:0x%x\n",
					i,
					dec->vf_dp.primaries[i][1]);
			}

			dec->vf_dp.luminance[0]
				= pPara->p.max_display_mastering_luminance;
			avs3_print(dec, AVS3_DBG_HDR_INFO,
				"luminance[0]:0x%x\n",
				dec->vf_dp.luminance[0]);

			dec->vf_dp.luminance[1]
				= pPara->p.min_display_mastering_luminance;
			avs3_print(dec, AVS3_DBG_HDR_INFO,
				"luminance[1]:0x%x\n",
				dec->vf_dp.luminance[1]);

			dec->vf_dp.content_light_level.present_flag
				= 1;
			dec->vf_dp.content_light_level.max_content
				= pPara->p.max_content_light_level;
			avs3_print(dec, AVS3_DBG_HDR_INFO,
				"max_content:0x%x\n",
				dec->vf_dp.content_light_level.max_content);

			dec->vf_dp.content_light_level.max_pic_average
				= pPara->p.max_picture_average_light_level;

			avs3_print(dec, AVS3_DBG_HDR_INFO,
				"max_pic_average:0x%x\n",
				dec->vf_dp.content_light_level.max_pic_average);
		}
		if (dec->video_ori_signal_type !=
			((dec->avs3_dec.param.p.video_signal_type << 16)
			| dec->avs3_dec.param.p.color_description)) {
			u32 v = dec->avs3_dec.param.p.video_signal_type;
			u32 c = dec->avs3_dec.param.p.color_description;
			u32 convert_c = c;

			if (v & 0x2000) {
				avs3_print(dec, AVS3_DBG_HDR_INFO,
					"video_signal_type present:\n");
				avs3_print(dec, AVS3_DBG_HDR_INFO,
					" %s %s\n",
					video_format_names[(v >> 10) & 7],
					((v >> 9) & 1) ?
						"full_range" : "limited");
				if (v & 0x100) {
					u32 transfer;
					u32 maxtrix;

					avs3_print(dec, AVS3_DBG_HDR_INFO,
						"color_description present:\n");
					avs3_print(dec, AVS3_DBG_HDR_INFO,
						"color_primarie = %d\n",
						v & 0xff);
					avs3_print(dec, AVS3_DBG_HDR_INFO,
						"transfer_characteristic = %d\n",
						(c >> 8) & 0xff);
					avs3_print(dec, AVS3_DBG_HDR_INFO,
						"  matrix_coefficient = %d\n",
						c & 0xff);

					transfer = (c >> 8) & 0xFF;
					if (transfer >= 15)
						avs3_print(dec, AVS3_DBG_HDR_INFO,
							"unsupport transfer_characteristic\n");
					else if (transfer  == 14)
						transfer = 18; /* HLG */
					else if (transfer == 13)
						transfer = 32;
					else if (transfer == 12)
						transfer = 16;
					else if (transfer == 11)
						transfer = 15;

					maxtrix = c & 0xFF;
					if (maxtrix >= 10)
						avs3_print(dec, AVS3_DBG_HDR_INFO,
							"unsupport matrix_coefficient\n");
					else if (maxtrix == 9)
						maxtrix = 10;
					else if (maxtrix == 8)
						maxtrix = 9;

					convert_c = (transfer << 8) | (maxtrix);

					avs3_print(dec, AVS3_DBG_HDR_INFO,
						" convered c:0x%x\n",
						convert_c);
				}
			}

			if (enable_force_video_signal_type)
				dec->video_signal_type
					= force_video_signal_type;
			else {
				dec->video_signal_type
					= (v << 16) | convert_c;

				dec->video_ori_signal_type
					= (v << 16) | c;
			}

			video_signal_type = dec->video_signal_type;
		}
	}

#if 0
	if ((debug_again & 0x4) &&
		dec->process_state ==
		PROC_STATE_INIT) {
		if (start_code == PB_PICTURE_START_CODE) {
			dec->process_state = PROC_STATE_TEST1;
			dec_again_process(dec);
			goto irq_handled_exit;
		}
	}
#endif

	if (start_code == SEQUENCE_HEADER_CODE) {
		avs3_print(dec, AVS3_DBG_BUFMGR_MORE,
			" ## SEQUENCE_HEADER_CODE ##\n");
		dec->avs3_dec.seq_change_flag = 1;
		dec->avs3_dec.init_hw_flag = 0;
	}

	if (dec->has_i_frame == 0 &&
		start_code == PB_PICTURE_START_CODE) {
		dec->dec_result = DEC_RESULT_DONE;
#ifdef NEW_FB_CODE
		if (dec->front_back_mode == 1)
			amhevc_stop_f();
		else
#endif
		amhevc_stop();
		avs3_print(dec, 0, "no i frame!!\n");
		vdec_schedule_work(&dec->work);
		goto irq_handled_exit;
	}

	mutex_lock(&dec->slice_header_lock);
	if (start_code == SEQUENCE_END_CODE) {
		avs3_bufmgr_process(&dec->avs3_dec, SEQUENCE_END_CODE);
		WRITE_VREG(HEVC_DEC_STATUS_REG, AVS3_ACTION_DONE);
	} else if (start_code == SEQUENCE_HEADER_CODE ||
		start_code == VIDEO_EDIT_CODE) {
		/*if (dec->m_ins_flag &&
			vdec_frame_based(hw_to_vdec(dec)))
			dec->start_decoding_flag |= 0x1;*/
		dec->process_state = PROC_STATE_HEAD_DONE;
		WRITE_VREG(HEVC_DEC_STATUS_REG, AVS3_ACTION_DONE);
	} else if (start_code == I_PICTURE_START_CODE ||
		start_code == PB_PICTURE_START_CODE) {
		avs3_frame_t *cur_pic;
		ret = 0;
#ifdef NEW_FB_CODE
		if (dec->m_ins_flag) {
			u32 width = avs3_dec->param.p.sqh_horizontal_size;
			u32 height = avs3_dec->param.p.sqh_vertical_size;
			u8 bit_depth = (u8)avs3_dec->param.p.sqh_encoding_precision;
			int cur_mmu_fb_4k_number = 0;

			width = ((width + MINI_SIZE - 1) >> MINI_SIZE_LOG2) << MINI_SIZE_LOG2;
			height = ((height   + MINI_SIZE - 1) >> MINI_SIZE_LOG2) << MINI_SIZE_LOG2;
			bit_depth = (bit_depth == 2) ? 10 : 8;
			cur_mmu_fb_4k_number = dec->fb_ifbuf_num * avs3_mmu_page_num(dec,
				width, height, (bit_depth == 10));

			if ((dec->front_back_mode == 1) &&
				(start_code == I_PICTURE_START_CODE) &&
				(dec->mmu_fb_4k_number < cur_mmu_fb_4k_number) &&
				(cur_mmu_fb_4k_number > 0)) {
				amhevc_stop_f();
				avs3_print(dec, AVS3_DBG_BUFMGR, "need realloc mmu fb\n");
				uninit_mmu_fb_bufstate(dec);
				init_mmu_fb_bufstate(dec, cur_mmu_fb_4k_number);
				dec_again_process(dec);
				mutex_unlock(&dec->slice_header_lock);
				goto irq_handled_exit;
			}
		}
#endif

		avs3_print(dec, AVS3_DBG_BUFMGR, "========== Picture %d\n", avs3_dec->decode_id++);
		//avs3_dec->ins_offset = 0; //move to init_pic_list_hw_fb
		if (debug & AVS3_DBG_PRINT_PIC_LIST)
			print_pic_pool(avs3_dec, "before bufmgr process");
		if (is_avs3_print_param()) {
			print_param(&avs3_dec->param);
			print_alf_param(&avs3_dec->param);
		}

		decoder_trace(dec->trace.decode_header_memory_time_name, 9, TRACE_BASIC);

		if (avs3_dec->seq_change_flag) {
		avs3_bufmgr_process(avs3_dec, SEQUENCE_HEADER_CODE);
		avs3_dec->seq_change_flag = 0;
		}
		ret = avs3_bufmgr_process(avs3_dec, start_code);
		if (debug & AVS3_DBG_PRINT_PIC_LIST)
			print_pic_pool(avs3_dec, "after bufmgr process");


		if ((ret == 0) && (avs3_dec->cur_pic != NULL)) {
			cur_pic = avs3_dec->cur_pic;
#ifdef NEW_FRONT_BACK_CODE
			cur_pic->width = avs3_dec->img.width;
			cur_pic->height = avs3_dec->img.height;
			cur_pic->depth = avs3_dec->input.sample_bit_depth;
#endif

			if (!dec->m_ins_flag)
				dec->slice_idx++;
			else
				release_cuva_data(cur_pic);
		}

#if 0
				if (avs3_dec->init_hw_flag == 0) {
					//from simulation
				init_pic_list_hw(avs3_dec, buf_spec, mc_buf_spec);
				avs3_dec->init_hw_flag = 1;
				}
#else
		if ((dec->pic_list_init_flag == 0) && (ctx->init_flag != 0)) {
			int32_t lcu_size_log2 = avs3_dec->lcu_size_log2;

			if ((dec->front_back_mode == 1) && (paral_alloc_buffer_mode & 1)) {
				if ((dec->pic_list_wait_alloc_done_flag == BUFFER_INIT)) {
					dec->dec_result = DEC_RESULT_WAIT_BUFFER;
					avs3_print(dec, AVS3_DBG_BUFMGR, "alloc buffer\n");
					vdec_schedule_work(&dec->work);
					mutex_unlock(&dec->slice_header_lock);
					goto irq_handled_exit;
				}
			} else
				init_pic_list(dec, lcu_size_log2);

#ifdef NEW_FB_CODE
			if ((dec->front_back_mode == 1) ||
				(dec->front_back_mode == 3))
				init_pic_list_hw_fb(dec);
			else
#endif
				init_pic_list_hw(dec);
#endif
			dec->pic_list_init_flag = 1;
		}

		decoder_trace(dec->trace.decode_header_memory_time_name, 10, TRACE_BASIC);
alloc_buffer_done:

#ifdef I_ONLY_SUPPORT
		if ((start_code == PB_PICTURE_START_CODE) &&
			(dec->i_only & 0x2))
			ret = -2;
#endif

		if (ret == 0) {
#ifdef AVS3_10B_MMU
			if (dec->mmu_enable
#ifdef NEW_FB_CODE
				&& (dec->front_back_mode != 1)
#endif
				) {
#if 0
//DDD
				if (cur_pic->index == -1) {
					pr_info("dec->avs3_dec.ctx.pic %p\n", dec->avs3_dec.ctx.pic);
					print_pic_pool(avs3_dec, "error");
				}
#endif
				ret = avs3_alloc_mmu(dec,
					cur_pic->index,
					cur_pic->width,
					cur_pic->height,
					cur_pic->depth,
					dec->frame_mmu_map_addr);
				if (ret >= 0) {
					dec->cur_fb_idx_mmu =
						cur_pic->index;
					cur_pic->mmu_alloc_flag = 1;
				} else
					pr_err("can't alloc need mmu1,idx %d ret =%d\n",
						cur_pic->index,
						ret);
			}
#endif
#ifdef AVS3_10B_MMU_DW
			if (dec->dw_mmu_enable
#ifdef NEW_FB_CODE
				&& (dec->front_back_mode != 1)
#endif
				) {
				ret = avs3_alloc_dw_mmu(dec,
					cur_pic->index,
					cur_pic->width,
					cur_pic->height,
					cur_pic->depth,
					dec->dw_frame_mmu_map_addr);
				if (ret >= 0) {
					dec->cur_fb_idx_mmu =
						cur_pic->index;
					cur_pic->mmu_alloc_flag = 1;
				} else
					pr_err("can't alloc need dw mmu1,idx %d ret =%d\n",
						dec->avs3_dec.cur_pic->index,
						ret);
			}
#endif
		}

#ifndef MV_USE_FIXED_BUF
		decoder_trace(dec->trace.decode_header_memory_time_name, TRACE_HEADER_MEMORY_START, TRACE_BASIC);

		if (ret == 0 && cur_pic->
			mpred_mv_wr_start_addr == 0
#if 0 //def NEW_FB_CODE
			&& (dec->front_back_mode != 1)
#endif
			) {
			unsigned long buf_addr;
			unsigned mv_buf_size = get_mv_buf_size(
				dec,
				cur_pic->width,
				cur_pic->height);
			int i = cur_pic->index;
			/*if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_SM1)
				mv_buf_size = 0x120000 * 4;*/
			if (decoder_bmmu_box_alloc_buf_phy
			(dec->bmmu_box,
			MV_BUFFER_IDX(i),
			mv_buf_size,
			DRIVER_NAME,
			&buf_addr) < 0) {
				ret = -1;
			} else {
				cur_pic->mpred_mv_wr_start_addr = buf_addr;
				if (!vdec_secure(hw_to_vdec(dec)))
					codec_mm_memset(cur_pic->mpred_mv_wr_start_addr,
						0, mv_buf_size);
			}
		}

		decoder_trace(dec->trace.decode_header_memory_time_name, TRACE_HEADER_MEMORY_END, TRACE_BASIC);
#endif
		if (ret) {
			avs3_print(dec, AVS3_DBG_BUFMGR,
				"avs3_bufmgr_process=> %d, AVS3_10B_DISCARD_NAL\r\n",
				ret);
			WRITE_VREG(HEVC_DEC_STATUS_REG, AVS3_10B_DISCARD_NAL);
	#ifdef AVS3_10B_MMU
			if (dec->mmu_enable
#ifdef NEW_FB_CODE
				&& (dec->front_back_mode != 1)
#endif
				)
				avs3_recycle_mmu_buf(dec);
	#endif
			if (dec->m_ins_flag) {
				int slice_type = 0;

#ifdef NEW_FB_CODE
				mutex_lock(&dec->fb_mutex);
#endif
				if (avs3_dec->cur_pic != NULL) {
					slice_type = avs3_dec->cur_pic->slice_type;
				} else {
					slice_type = dec->avs3_dec.param.p.pic_header_slice_type;
				}

				dec->gvs->frame_count++;
				dec->gvs->drop_frame_count++;
				dec->gvs->error_frame_count++;

				if (slice_type == SLICE_I) {
					dec->gvs->i_lost_frames++;
					dec->gvs->i_concealed_frames++;
					dec->gvs->i_decoded_frames++;
				} else if (slice_type == SLICE_P) {
					dec->gvs->p_lost_frames++;
					dec->gvs->p_concealed_frames++;
					dec->gvs->p_decoded_frames++;
				} else if (slice_type == SLICE_B) {
					dec->gvs->b_lost_frames++;
					dec->gvs->b_concealed_frames++;
					dec->gvs->b_decoded_frames++;
				}

#ifdef NEW_FB_CODE
				mutex_unlock(&dec->fb_mutex);
#endif

				dec->dec_result = DEC_RESULT_ERROR;

				vdec_schedule_work(&dec->work);
			}
			mutex_unlock(&dec->slice_header_lock);
			goto irq_handled_exit;
		} else {
			if (avs3_dec->cur_pic != NULL) {
				dec->gvs->frame_count++;
				if (avs3_dec->cur_pic->slice_type == SLICE_I) {
					dec->gvs->i_decoded_frames++;
				} else if (avs3_dec->cur_pic->slice_type == SLICE_P) {
					dec->gvs->p_decoded_frames++;
				} else if (avs3_dec->cur_pic->slice_type == SLICE_B) {
					dec->gvs->b_decoded_frames++;
				}
			}

			dec->avs3_dec.cur_pic->stream_offset =
			READ_VREG(HEVC_SHIFT_BYTE_COUNT);
			/*
			struct PIC_BUFFER_CONFIG_s *cur_pic
				= &cm->cur_frame->buf;
			cur_pic->decode_idx = dec->frame_count;
			*/
			if (!dec->m_ins_flag) {
				dec->frame_count++;
				decode_frame_count[dec->index]
					= dec->frame_count;
			}
			/*MULTI_INSTANCE_SUPPORT*/
			if (dec->chunk) {
				dec->avs3_dec.cur_pic->pts =
				dec->chunk->pts;
				dec->avs3_dec.cur_pic->pts64 =
				dec->chunk->pts64;
			}
			if (vdec->mvfrm)
				dec->avs3_dec.cur_pic->frame_size = vdec->mvfrm->frame_size;
decode_slice:
			dec->avs3_dec.cur_pic->double_write_mode
				= get_double_write_mode(dec);
#ifdef OW_TRIPLE_WRITE
			dec->avs3_dec.cur_pic->triple_write_mode
				= get_triple_write_mode(dec);
#endif

			decoder_trace(dec->trace.decode_header_memory_time_name, TRACE_HEADER_REGISTER_START, TRACE_BASIC);
#ifdef NEW_FB_CODE
			if (dec->front_back_mode == 1 || dec->front_back_mode == 3) {
				if (!efficiency_mode) {
					config_mc_buffer_fb(dec);
					config_mcrcc_axi_hw_fb(dec);
				}

				config_mpred_hw_fb(dec);

				if (!efficiency_mode) {
					config_dblk_hw_fb(dec);
					config_sao_hw_fb(dec);
					config_alf_hw_fb(dec);
				}
			} else {
#endif
				config_mc_buffer(dec);
#ifdef NEW_FB_CODE
				if (dec->front_back_mode == 0)
#endif
					config_mcrcc_axi_hw(dec);
				config_mpred_hw(dec);
				config_dblk_hw(dec);
				config_sao_hw(dec);
				config_alf_hw(dec);
#ifdef NEW_FB_CODE
			}
#endif
			decoder_trace(dec->trace.decode_header_memory_time_name, TRACE_HEADER_REGISTER_END, TRACE_BASIC);

			// HEVC_PARSER_HEADER_INFO :
			// bit[30]    --  avs3_bi_mid_ptr  // (ctx->ptr - ctx->refp[0][REFP_0].ptr == ctx->refp[0][REFP_1].ptr - ctx->ptr)
			// bit[21:16] --  ctx->dpm.num_refp[REFP_1]
			// bit[15:10] --  ctx->dpm.num_refp[REFP_0]
			avs3_bi_mid_ptr = (ctx->ptr - ctx->refp[0][REFP_0].ptr == ctx->refp[0][REFP_1].ptr - ctx->ptr);
			WRITE_VREG(HEVC_PARSER_HEADER_INFO, (ctx->dpm.num_refp[REFP_0]<<10) | (ctx->dpm.num_refp[REFP_1]<<16) | (avs3_bi_mid_ptr<<30));

#ifdef BUFMGR_ONLY
			WRITE_VREG(NAL_SEARCH_CTL, 0x1);
#else
			WRITE_VREG(NAL_SEARCH_CTL, 0);
#endif

#ifdef NEW_FRONT_BACK_CODE
			if ((!efficiency_mode) && (dec->front_back_mode == 1 ||
				dec->front_back_mode == 3)) {

				WRITE_BACK_RET(avs3_dec);
				avs3_print(dec, AVS3_DBG_BUFMGR_MORE,
					"write system instruction, ins_offset = %d, addr = 0x%x\n",
					avs3_dec->ins_offset, avs3_dec->fr.sys_imem_ptr);

				avs3_dec->sys_imem_ptr = avs3_dec->fr.sys_imem_ptr;
				avs3_dec->sys_imem_ptr_v = avs3_dec->fr.sys_imem_ptr_v;

#ifdef LARGE_INSTRUCTION_SPACE_SUPORT
				if (avs3_dec->ins_offset > 512) {
					avs3_print(dec, 0,
						"!!!!!Error!!!!!!!!, ins_offset %d is too big (>512)\n", avs3_dec->ins_offset);
					avs3_dec->ins_offset = 512;
				} else if (avs3_dec->ins_offset < 256) {
					avs3_dec->ins_offset = 256;
					WRITE_BACK_RET(avs3_dec);
				}

				memcpy(avs3_dec->sys_imem_ptr_v, (void*)(&avs3_dec->instruction[0]), avs3_dec->ins_offset*4);
				//copyToDDR_32bits(dec->fr.sys_imem_ptr, instruction, ins_offset*4, 0);
				avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
					"cur_imem_ptr_v 0x%x: %02x %02x %02x %02x\n",
					avs3_dec->sys_imem_ptr_v,
					((u8 *)avs3_dec->sys_imem_ptr_v)[0], ((u8 *)avs3_dec->sys_imem_ptr_v)[1],
					((u8 *)avs3_dec->sys_imem_ptr_v)[2], ((u8 *)avs3_dec->sys_imem_ptr_v)[3]);
				avs3_dec->sys_imem_ptr += 2 * FB_IFBUF_SYS_IMEM_BLOCK_SIZE;
				avs3_dec->sys_imem_ptr_v += 2 * FB_IFBUF_SYS_IMEM_BLOCK_SIZE;
#else
				if (avs3_dec->ins_offset > 256) {
					avs3_print(dec, 0,
						"!!!!!Error!!!!!!!!, ins_offset %d is too big (>256)\n", avs3_dec->ins_offset);
					avs3_dec->ins_offset = 256;
				}

				memcpy(avs3_dec->sys_imem_ptr_v, (void*)(&avs3_dec->instruction[0]), avs3_dec->ins_offset*4);
				//copyToDDR_32bits(dec->fr.sys_imem_ptr, instruction, ins_offset*4, 0);
				avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
					"cur_imem_ptr_v 0x%x: %02x %02x %02x %02x\n",
					avs3_dec->sys_imem_ptr_v,
					((u8 *)avs3_dec->sys_imem_ptr_v)[0], ((u8 *)avs3_dec->sys_imem_ptr_v)[1],
					((u8 *)avs3_dec->sys_imem_ptr_v)[2], ((u8 *)avs3_dec->sys_imem_ptr_v)[3]);
				avs3_dec->sys_imem_ptr += FB_IFBUF_SYS_IMEM_BLOCK_SIZE;
				avs3_dec->sys_imem_ptr_v +=FB_IFBUF_SYS_IMEM_BLOCK_SIZE;
#endif
				if (avs3_dec->sys_imem_ptr >= avs3_dec->fb_buf_sys_imem.buf_end) {
					avs3_print(dec, AVS3_DBG_BUFMGR,
						"sys_imem_ptr is 0x%x, wrap around\n", avs3_dec->sys_imem_ptr);
					avs3_dec->sys_imem_ptr = avs3_dec->fb_buf_sys_imem.buf_start;
					avs3_dec->sys_imem_ptr_v = avs3_dec->fb_buf_sys_imem_addr;
				}
				if (dec->front_back_mode == 1) {
					//WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 8);
					//WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, sys_imem_ptr);
					//imem_count++;
					WRITE_VREG(DOS_HEVC_STALL_START, 0); // disable stall
				}

				WRITE_VREG(HEVC_DEC_STATUS_REG, AVS3_ACTION_DONE);
			} else {
				WRITE_VREG(HEVC_DEC_STATUS_REG, AVS3_ACTION_DONE);
			}
#endif

			//WRITE_VREG(HEVC_DEC_STATUS_REG, AVS3_ACTION_DONE);
			//pr_info("!!write AVS3_ACTION_DONE %d\n", __LINE__);

			if ((debug_again & 0x2) &&
				dec->process_state ==
				PROC_STATE_INIT) {
				dec->process_state = PROC_STATE_DECODING;
				dec_again_process(dec);
				mutex_unlock(&dec->slice_header_lock);
				goto irq_handled_exit;
			}

			dec->process_state = PROC_STATE_DECODING;

		}
		if (start_code == I_PICTURE_START_CODE)
			dec->has_i_frame = 1;
		if (dec->m_ins_flag)
			start_process_time(dec);
		vdec_profile(hw_to_vdec(dec), VDEC_PROFILE_DECODER_START, CORE_MASK_HEVC);
	}

	if ((dec_status == AVS3_HEAD_PIC_I_READY) ||
		(dec_status == AVS3_HEAD_PIC_PB_READY)) {
		decoder_trace(dec->trace.decode_time_name, DECODER_ISR_THREAD_HEAD_END, TRACE_PERFORMANCE_DETAIL);
	}
	if (efficiency_mode && (dec->front_back_mode == 1)) {
		if (dec->pic_list_init_flag) {
#ifdef NEW_FB_CODE
			if ((dec->front_back_mode == 1) ||
				(dec->front_back_mode == 3))
				init_pic_list_hw_fb(dec);
			else
#endif
				init_pic_list_hw(dec);
		}

		if ((dec->front_back_mode == 1 || dec->front_back_mode == 3) &&
			(start_code == I_PICTURE_START_CODE ||
			start_code == PB_PICTURE_START_CODE)) {
			config_mc_buffer_fb(dec);
			config_mcrcc_axi_hw_fb(dec);
			config_dblk_hw_fb(dec);
			config_sao_hw_fb(dec);
			config_alf_hw_fb(dec);

			WRITE_BACK_RET(avs3_dec);
			avs3_print(dec, AVS3_DBG_BUFMGR_MORE,
				"write system instruction, ins_offset = %d, addr = 0x%x\n",
				avs3_dec->ins_offset, avs3_dec->fr.sys_imem_ptr);

			avs3_dec->sys_imem_ptr = avs3_dec->fr.sys_imem_ptr;
			avs3_dec->sys_imem_ptr_v = avs3_dec->fr.sys_imem_ptr_v;
#ifdef LARGE_INSTRUCTION_SPACE_SUPORT
			if (avs3_dec->ins_offset > 512) {
				avs3_print(dec, 0,
					"!!!!!Error!!!!!!!!, ins_offset %d is too big (>512)\n", avs3_dec->ins_offset);
				avs3_dec->ins_offset = 512;
			} else if (avs3_dec->ins_offset < 256) {
				avs3_dec->ins_offset = 256;
				WRITE_BACK_RET(avs3_dec);
			}
			memcpy(avs3_dec->sys_imem_ptr_v, (void*)(&avs3_dec->instruction[0]), avs3_dec->ins_offset*4);
			//copyToDDR_32bits(dec->fr.sys_imem_ptr, instruction, ins_offset*4, 0);
			avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
				"cur_imem_ptr_v 0x%x: %02x %02x %02x %02x\n",
				avs3_dec->sys_imem_ptr_v,
				((u8 *)avs3_dec->sys_imem_ptr_v)[0], ((u8 *)avs3_dec->sys_imem_ptr_v)[1],
				((u8 *)avs3_dec->sys_imem_ptr_v)[2], ((u8 *)avs3_dec->sys_imem_ptr_v)[3]);

			avs3_dec->sys_imem_ptr += 2*FB_IFBUF_SYS_IMEM_BLOCK_SIZE;
			avs3_dec->sys_imem_ptr_v += 2*FB_IFBUF_SYS_IMEM_BLOCK_SIZE;
#else
			if (avs3_dec->ins_offset > 256) {
				avs3_print(dec, 0,
					"!!!!!Error!!!!!!!!, ins_offset %d is too big (>256)\n", avs3_dec->ins_offset);
				avs3_dec->ins_offset = 256;
			}
			memcpy(avs3_dec->sys_imem_ptr_v, (void*)(&avs3_dec->instruction[0]), avs3_dec->ins_offset*4);
			//copyToDDR_32bits(dec->fr.sys_imem_ptr, instruction, ins_offset*4, 0);
			avs3_print(dec, AVS3_DBG_BUFMGR_DETAIL,
				"cur_imem_ptr_v 0x%x: %02x %02x %02x %02x\n",
				avs3_dec->sys_imem_ptr_v,
				((u8 *)avs3_dec->sys_imem_ptr_v)[0], ((u8 *)avs3_dec->sys_imem_ptr_v)[1],
				((u8 *)avs3_dec->sys_imem_ptr_v)[2], ((u8 *)avs3_dec->sys_imem_ptr_v)[3]);
			avs3_dec->sys_imem_ptr += FB_IFBUF_SYS_IMEM_BLOCK_SIZE;
			avs3_dec->sys_imem_ptr_v += FB_IFBUF_SYS_IMEM_BLOCK_SIZE;
#endif
			if (avs3_dec->sys_imem_ptr >= avs3_dec->fb_buf_sys_imem.buf_end) {
				avs3_print(dec, AVS3_DBG_BUFMGR,
					"sys_imem_ptr is 0x%x, wrap around\n", avs3_dec->sys_imem_ptr);
				avs3_dec->sys_imem_ptr = avs3_dec->fb_buf_sys_imem.buf_start;
				avs3_dec->sys_imem_ptr_v = avs3_dec->fb_buf_sys_imem_addr;
			}
			if (dec->front_back_mode == 1) {
				//WRITE_VREG(HEVC_ASSIST_RING_F_INDEX, 8);
				//WRITE_VREG(HEVC_ASSIST_RING_F_WPTR, sys_imem_ptr);
				//imem_count++;
				WRITE_VREG(DOS_HEVC_STALL_START, 0); // disable stall
			}
		}

		if ((start_code == I_PICTURE_START_CODE)
			|| (start_code == PB_PICTURE_START_CODE)
			|| (start_code == SEQUENCE_END_CODE)
			|| (start_code == VIDEO_EDIT_CODE)) {
			avs3_prepare_display_buf(dec);
		}
	}
	mutex_unlock(&dec->slice_header_lock);

irq_handled_exit_and_start_timer:
	if (dec->m_ins_flag)
		start_process_time(dec);

irq_handled_exit:

	dec->process_busy = 0;
	return IRQ_HANDLED;
}

static irqreturn_t vavs3_isr(int irq, void *data)
{
	//int i;
	unsigned int dec_status;
	struct AVS3Decoder_s *dec = (struct AVS3Decoder_s *)data;
	uint debug_tag = 0;
#if 0
	if ((debug & AVS3_DBG_BE_SIMULATE_IRQ)
		&&(READ_VREG(DEBUG_REG1_DBE) ||
			READ_VREG(HEVC_DEC_STATUS_DBE)== HEVC_BE_DECODE_DATA_DONE)) {
		pr_info("Simulate BE irq\n");
		WRITE_VREG(dec->backend_ASSIST_MBOX0_IRQ_REG, 1);
	}
#endif

	dec_status = READ_VREG(HEVC_DEC_STATUS_REG);
	if (dec_status == HEVC_DECPIC_DATA_DONE) {
		vdec_profile(hw_to_vdec(dec), VDEC_PROFILE_DECODER_END, CORE_MASK_HEVC);
	}

	WRITE_VREG(dec->ASSIST_MBOX0_CLR_REG, 1);

	if ((dec_status == AVS3_HEAD_PIC_I_READY) ||
		(dec_status == AVS3_HEAD_PIC_PB_READY)) {
		decoder_trace(dec->trace.decode_time_name, DECODER_ISR_HEAD_DONE, TRACE_PERFORMANCE_DETAIL);
	} else if (dec_status == HEVC_DECPIC_DATA_DONE) {
		decoder_trace(dec->trace.decode_time_name, DECODER_ISR_PIC_DONE, TRACE_PERFORMANCE_DETAIL);
	}


	if (!dec)
		return IRQ_HANDLED;
	if (dec->init_flag == 0)
		return IRQ_HANDLED;
	if (dec->process_busy)/*on process.*/
		return IRQ_HANDLED;
	dec->dec_status = dec_status;
	dec->process_busy = 1;
#if 0
	if (debug & AVS3_DBG_IRQ_EVENT) {
		if (dec->front_back_mode != 1)
			avs3_print(dec, 0,
				"avs3 isr dec status  = 0x%x, debug_tag 0x%x, lcu 0x%x shiftbyte 0x%x (%x %x lev %x, wr %x, rd %x) HEVC_SAO_CRC %x\n",
				dec_status, READ_HREG(DEBUG_REG1), READ_VREG(HEVC_PARSER_LCU_START),
				READ_VREG(HEVC_SHIFT_BYTE_COUNT),
				READ_VREG(HEVC_STREAM_START_ADDR),
				READ_VREG(HEVC_STREAM_END_ADDR),
				READ_VREG(HEVC_STREAM_LEVEL),
				READ_VREG(HEVC_STREAM_WR_PTR),
				READ_VREG(HEVC_STREAM_RD_PTR),
				(get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_S5) ?
					READ_VREG(HEVC_SAO_CRC) : 0
			);
		else {
			avs3_print(dec, 0,
				"avs3 isr dec status  = 0x%x, debug_tag 0x%x, lcu 0x%x shiftbyte 0x%x (%x %x lev %x, wr %x, rd %x) [BE] pc %x psr %x DEC_STATUS %x shiftstatus 0x%x\n",
				dec_status, READ_HREG(DEBUG_REG1), READ_VREG(HEVC_PARSER_LCU_START),
				READ_VREG(HEVC_SHIFT_BYTE_COUNT),
				READ_VREG(HEVC_STREAM_START_ADDR),
				READ_VREG(HEVC_STREAM_END_ADDR),
				READ_VREG(HEVC_STREAM_LEVEL),
				READ_VREG(HEVC_STREAM_WR_PTR),
				READ_VREG(HEVC_STREAM_RD_PTR),
				READ_VREG(HEVC_MPC_E_DBE),
				READ_VREG(HEVC_MPSR_DBE),
				READ_VREG(HEVC_DEC_STATUS_DBE),
				READ_VREG(HEVC_SHIFT_STATUS)
			);
		}
	}
#endif

	if (udebug_flag)
		debug_tag = READ_HREG(DEBUG_REG1);
	if (debug_tag != 0) {
#if 0
		handle_ucode_dbg(dec, debug_tag);
		dec->process_busy = 0;
		return IRQ_HANDLED;
#else
		decoder_trace(dec->trace.decode_time_name, DECODER_ISR_END, TRACE_BASIC);
		return IRQ_WAKE_THREAD;
#endif
	}

	if (!dec->m_ins_flag) {
		if (dec->error_flag == 1) {
			dec->error_flag = 2;
			dec->process_busy = 0;
			return IRQ_HANDLED;
		} else if (dec->error_flag == 3) {
			dec->process_busy = 0;
			return IRQ_HANDLED;
		}

		if ((dec->pic_list_init_flag) &&
			get_free_buf_count(dec) <= 0) {
			/*
			if (dec->wait_buf == 0)
				pr_info("set wait_buf to 1\r\n");
			*/
			dec->wait_buf = 1;
			dec->process_busy = 0;
			if (debug & AVS3_DBG_IRQ_EVENT)
				avs3_print(dec, 0, "wait_buf\n");
			return IRQ_HANDLED;
		} else if (force_disp_pic_index) {
			dec->process_busy = 0;
			return IRQ_HANDLED;
		}
	}

	if (dec->m_ins_flag)
		reset_process_time(dec);

	decoder_trace(dec->trace.decode_time_name, DECODER_ISR_END, TRACE_BASIC);
	return IRQ_WAKE_THREAD;
}

#ifdef NEW_FB_CODE
static void avs3_check_timer_back_func(struct timer_list *timer)
{
	struct AVS3Decoder_s *dec = container_of(timer,
			struct AVS3Decoder_s, timer_back);

	if (dec->init_flag == 0) {
		if (dec->stat & STAT_TIMER_BACK_ARM) {
			mod_timer(&dec->timer_back, jiffies + PUT_INTERVAL);
		}
		return;
	}

	if (
		(decode_timeout_val_back > 0) &&
		(dec->start_process_time_back > 0) &&
		((1000 * (jiffies - dec->start_process_time_back) / HZ)
			> decode_timeout_val_back)
	) {
		if (dec->decode_timeout_count_back > 0)
			dec->decode_timeout_count_back--;
		if (dec->decode_timeout_count_back == 0)
			timeout_process_back(dec);
	}

#if 0
	if (debug & AVS3_DBG_BE_SIMULATE_IRQ) {
		//struct vdec_s *vdec = hw_to_vdec(dec);
		pr_info("Simulate BE irq\n");
		WRITE_VREG(dec->backend_ASSIST_MBOX0_IRQ_REG, 1);
		//if (avs3_back_irq_cb(vdec, 0) == IRQ_WAKE_THREAD)
		//	avs3_back_threaded_irq_cb(vdec, 0);
	}
#endif
	if ((dec->start_process_time_back > 0) &&
		(dec->decode_timeout_count_back != 0) &&
		!(error_handle_policy & 0x4) &&
		(decode_timeout_val_back > 0)) {
		int current_monitor_data = 0;

		WRITE_VREG(HEVC_PATH_MONITOR_CTRL, (READ_VREG(HEVC_PATH_MONITOR_CTRL) & 0xfffffe0f) | 0x91);
		current_monitor_data = READ_VREG(HEVC_PATH_MONITOR_DATA);
		if (dec->last_monitor_data == current_monitor_data) {
			if (dec->decode_timeout_count_back > 0)
				dec->decode_timeout_count_back--;
			if (dec->decode_timeout_count_back == 0) {
				timeout_process_back(dec);
			}
		} else {
			dec->decode_timeout_count_back = fast_timer_check_count;
		}
		dec->last_monitor_data = current_monitor_data;
		mod_timer(timer, jiffies + 1);
	} else
		mod_timer(timer, jiffies + PUT_INTERVAL);
}
#endif

static void vavs3_put_timer_func(struct timer_list *timer)
{
	struct AVS3Decoder_s *dec = container_of(timer,
		struct AVS3Decoder_s, timer);
	//uint8_t empty_flag;
	//unsigned int buf_level;

	//enum receiver_start_e state = RECEIVER_INACTIVE;
	if (dec->init_flag == 0) {
		if (dec->stat & STAT_TIMER_ARM) {
			timer->expires = jiffies + PUT_INTERVAL;
			add_timer(&dec->timer);
		}
		return;
	}
	if (dec->m_ins_flag == 0) {
#ifndef PXP_DEBUG
		if (vf_get_receiver(dec->provider_name)) {
			state =
				vf_notify_receiver(dec->provider_name,
					VFRAME_EVENT_PROVIDER_QUREY_STATE,
					NULL);
			if ((state == RECEIVER_STATE_NULL)
				|| (state == RECEIVER_STATE_NONE))
				state = RECEIVER_INACTIVE;
		} else
			state = RECEIVER_INACTIVE;

		empty_flag = (READ_VREG(HEVC_PARSER_INT_STATUS) >> 6) & 0x1;
		/* error watchdog */
		if (empty_flag == 0) {
			/* decoder has input */
			if ((debug & AVS3_DBG_DIS_LOC_ERROR_PROC) == 0) {

				buf_level = READ_VREG(HEVC_STREAM_LEVEL);
				/* receiver has no buffer to recycle */
				if ((state == RECEIVER_INACTIVE) &&
					(kfifo_is_empty(&dec->display_q) &&
					 buf_level > 0x200)
					) {
						WRITE_VREG
						(dec->ASSIST_MBOX0_IRQ_REG,
						 0x1);
				}
			}

			if ((debug & AVS3_DBG_DIS_SYS_ERROR_PROC) == 0) {
				/* receiver has no buffer to recycle */
				/*if ((state == RECEIVER_INACTIVE) &&
					(kfifo_is_empty(&dec->display_q))) {
				pr_info("avs3 something error,need reset\n");
				}*/
			}
		}
#endif
	} else {
		if (
			(decode_timeout_val > 0) &&
			(dec->start_process_time > 0) &&
			((1000 * (jiffies - dec->start_process_time) / HZ)
				> decode_timeout_val)
		) {
			int current_lcu_idx =
				READ_VREG(HEVC_PARSER_LCU_START)
				& 0xffffff;
			if (dec->last_lcu_idx == current_lcu_idx) {
				if (dec->decode_timeout_count > 0)
					dec->decode_timeout_count--;
				if (dec->decode_timeout_count == 0) {
					if (input_frame_based(
						hw_to_vdec(dec)) ||
					(READ_VREG(HEVC_STREAM_LEVEL) > 0x200))
						timeout_process(dec);
					else {
						avs3_print(dec, 0,
							"timeout & empty, again\n");
						dec_again_process(dec);
					}
				}
			} else {
				start_process_time(dec);
				dec->last_lcu_idx = current_lcu_idx;
			}
		}
	}
	if ((dec->ucode_pause_pos != 0) &&
		(dec->ucode_pause_pos != 0xffffffff) &&
		udebug_pause_pos != dec->ucode_pause_pos) {
		dec->ucode_pause_pos = 0;
		WRITE_VREG(DEBUG_REG1, 0);
	}
	if (debug & AVS3_DBG_DUMP_DATA) {
		debug &= ~AVS3_DBG_DUMP_DATA;
		avs3_print(dec, 0,
			"%s: chunk size 0x%x off 0x%x sum 0x%x\n",
			__func__,
			dec->chunk->size,
			dec->chunk->offset,
			get_data_check_sum(dec, dec->chunk->size)
			);
		dump_data(dec, dec->chunk->size);
	}
	if (debug & AVS3_DBG_DUMP_PIC_LIST) {
		print_pic_pool(&dec->avs3_dec, "");
		debug &= ~AVS3_DBG_DUMP_PIC_LIST;
	}
	if (debug & AVS3_DBG_TRIG_SLICE_SEGMENT_PROC) {
		WRITE_VREG(dec->ASSIST_MBOX0_IRQ_REG, 0x1);
		debug &= ~AVS3_DBG_TRIG_SLICE_SEGMENT_PROC;
	}
	if (debug & AVS3_DBG_DUMP_RPM_BUF) {
		int i;

		pr_info("RPM:\n");
		for (i = 0; i < RPM_BUF_SIZE; i += 4) {
			int ii;
			if ((i & 0xf) == 0)
				pr_info("%03x: ", i);
			for (ii = 0; ii < 4; ii++) {
				pr_info("%04x ",
					   dec->lmem_ptr[i + 3 - ii]);
			}
			if (((i + ii) & 0xf) == 0)
				pr_info("\n");
		}
		debug &= ~AVS3_DBG_DUMP_RPM_BUF;
	}
	if (debug & AVS3_DBG_DUMP_LMEM_BUF) {
		int i;

		pr_info("LMEM:\n");
		for (i = 0; i < LMEM_BUF_SIZE; i += 4) {
			int ii;
			if ((i & 0xf) == 0)
				pr_info("%03x: ", i);
			for (ii = 0; ii < 4; ii++) {
				pr_info("%04x ",
					   dec->lmem_ptr[i + 3 - ii]);
			}
			if (((i + ii) & 0xf) == 0)
				pr_info("\n");
		}
		debug &= ~AVS3_DBG_DUMP_LMEM_BUF;
	}
	/*if (debug & AVS3_DBG_HW_RESET) {
	}*/

	if (radr != 0) {
		if (rval != 0) {
			WRITE_VREG(radr, rval);
			pr_info("WRITE_VREG(%x,%x)\n", radr, rval);
		} else
			pr_info("READ_VREG(%x)=%x\n", radr, READ_VREG(radr));
		rval = 0;
		radr = 0;
	}
	if (pop_shorts != 0) {
		int i;
		u32 sum = 0;
		pr_info("pop stream 0x%x shorts\r\n", pop_shorts);
		for (i = 0; i < pop_shorts; i++) {
			u32 data =
			(READ_HREG(HEVC_SHIFTED_DATA) >> 16);
			WRITE_VREG(HEVC_SHIFT_COMMAND,
			(1<<7)|16);
			if ((i & 0xf) == 0)
				pr_info("%04x:", i);
			pr_info("%04x ", data);
			if (((i + 1) & 0xf) == 0)
				pr_info("\r\n");
			sum += data;
		}
		pr_info("\r\nsum = %x\r\n", sum);
		pop_shorts = 0;
	}
	if (dbg_cmd != 0) {
		if (dbg_cmd == 1) {
			u32 disp_laddr;
			if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_GXBB &&
				get_double_write_mode(dec) == 0) {
				disp_laddr =
					READ_VCBUS_REG(AFBC_BODY_BADDR) << 4;
			} else {
				struct canvas_s cur_canvas;
				canvas_read((READ_VCBUS_REG(VD1_IF0_CANVAS0)
					& 0xff), &cur_canvas);
				disp_laddr = cur_canvas.addr;
			}
			pr_info("current displayed buffer address %x\r\n",
				disp_laddr);
		}
#ifdef DEBUG_CMD
		else if ((dbg_cmd & 0xf) == 2) {
			d_dump(dec, dbg_cmd & (~0xf), 128, NULL, NULL);
		}
#endif
		dbg_cmd = 0;
	}
	/*don't changed at start.*/
	if (dec->get_frame_dur && dec->show_frame_num > 60 &&
		dec->frame_dur > 0 && dec->saved_resolution !=
		frame_width * frame_height *
			(96000 / dec->frame_dur)) {
		int fps = 96000 / dec->frame_dur;
		if (hevc_source_changed(VFORMAT_AVS3,
			frame_width, frame_height, fps) > 0)
			dec->saved_resolution = frame_width *
			frame_height * fps;
	}

	if ((dec->start_process_time != 0) &&
		!(error_handle_policy & 0x4) &&
		(decode_timeout_val > 0)) {
		int current_lcu_idx =
			READ_VREG(HEVC_PARSER_LCU_START) & 0xffffff;

		if (dec->last_lcu_idx == current_lcu_idx) {
			if (dec->decode_timeout_count > 0)
				dec->decode_timeout_count--;
			if (dec->decode_timeout_count == 0) {
				timeout_process(dec);
			}
		} else {
			dec->decode_timeout_count = fast_timer_check_count;
			dec->last_lcu_idx = current_lcu_idx;
		}

		mod_timer(timer, jiffies + 1);
	} else
		mod_timer(timer, jiffies + PUT_INTERVAL);
}

int vavs3_dec_status(struct vdec_s *vdec, struct vdec_info *vstatus)
{
	struct AVS3Decoder_s *dec =
		(struct AVS3Decoder_s *)vdec->private;

	if (!dec)
		return -1;

	vstatus->frame_width = dec->frame_width;
	vstatus->frame_height = dec->frame_height;

	if (dec->frame_dur != 0)
		vstatus->frame_rate = ((96000 * 10 / dec->frame_dur) % 10) < 5 ?
				      96000 / dec->frame_dur : (96000 / dec->frame_dur +1);
	else
		vstatus->frame_rate = -1;
	vstatus->error_count = 0;
	vstatus->status = dec->stat | dec->fatal_error;
	vstatus->frame_dur = dec->frame_dur;
	vstatus->bit_rate = dec->gvs->bit_rate;
	vstatus->frame_data = dec->gvs->frame_data;
	vstatus->total_data = dec->gvs->total_data;
	vstatus->frame_count = dec->gvs->frame_count;
	vstatus->error_frame_count = dec->gvs->error_frame_count;
	vstatus->drop_frame_count = dec->gvs->drop_frame_count;
	vstatus->i_decoded_frames = dec->gvs->i_decoded_frames;
	vstatus->i_lost_frames =  dec->gvs->i_lost_frames;
	vstatus->i_concealed_frames =  dec->gvs->i_concealed_frames;
	vstatus->p_decoded_frames =  dec->gvs->p_decoded_frames;
	vstatus->p_lost_frames =  dec->gvs->p_lost_frames;
	vstatus->p_concealed_frames =  dec->gvs->p_concealed_frames;
	vstatus->b_decoded_frames =  dec->gvs->b_decoded_frames;
	vstatus->b_lost_frames =  dec->gvs->b_lost_frames;
	vstatus->b_concealed_frames =  dec->gvs->b_concealed_frames;
	vstatus->total_data = dec->gvs->total_data;
	vstatus->samp_cnt = dec->gvs->samp_cnt;
	vstatus->offset = dec->gvs->offset;
	snprintf(vstatus->vdec_name, sizeof(vstatus->vdec_name),
		"%s", DRIVER_NAME);
	return 0;
}

int vavs3_set_isreset(struct vdec_s *vdec, int isreset)
{
#ifndef PXP_DEBUG
	is_reset = isreset;
#endif
	return 0;
}

static void vavs3_prot_init(struct AVS3Decoder_s *dec)
{
	unsigned int data32;

	avs3_config_work_space_hw(dec);
	if (dec->pic_list_init_flag)
		init_pic_list_hw(dec);
#ifdef TMP_DEBUG
	avs2_init_decoder_hw(dec);
#else
	avs3_init_decoder_hw(dec);
#endif
#if 1
	avs3_print(dec, AVS3_DBG_BUFMGR_MORE,
		"%s\n", __func__);
#if 0
	data32 = READ_VREG(HEVC_STREAM_CONTROL);
	data32 = data32 |
		(1 << 0)/*stream_fetch_enable*/
		;
	WRITE_VREG(HEVC_STREAM_CONTROL, data32);
#endif
#if 0
	data32 = READ_VREG(HEVC_SHIFT_STARTCODE);
	if (data32 != 0x00000100) {
		pr_info("avs3 prot init error %d\n", __LINE__);
		return;
	}
	data32 = READ_VREG(HEVC_SHIFT_EMULATECODE);
	if (data32 != 0x00000300) {
		pr_info("avs3 prot init error %d\n", __LINE__);
		return;
	}
	WRITE_VREG(HEVC_SHIFT_STARTCODE, 0x12345678);
	WRITE_VREG(HEVC_SHIFT_EMULATECODE, 0x9abcdef0);
	data32 = READ_VREG(HEVC_SHIFT_STARTCODE);
	if (data32 != 0x12345678) {
		pr_info("avs3 prot init error %d\n", __LINE__);
		return;
	}
	data32 = READ_VREG(HEVC_SHIFT_EMULATECODE);
	if (data32 != 0x9abcdef0) {
		pr_info("avs3 prot init error %d\n", __LINE__);
		return;
	}
#endif
	WRITE_VREG(HEVC_SHIFT_STARTCODE, 0x00000100);
	WRITE_VREG(HEVC_SHIFT_EMULATECODE, 0x00000000);

// Set MCR fetch priorities
	data32 = 0x1 | (0x1 << 2) | (0x1 <<3) | (24 << 4) | (32 << 11) | (24 << 18) | (32 << 25);
	WRITE_VREG(HEVCD_MPP_DECOMP_AXIURG_CTL, data32);

#endif

	WRITE_VREG(HEVC_WAIT_FLAG, 1);

	/* WRITE_VREG(HEVC_MPSR, 1); */

	/* clear mailbox interrupt */
	WRITE_VREG(dec->ASSIST_MBOX0_CLR_REG, 1);

	/* enable mailbox interrupt */
	WRITE_VREG(dec->ASSIST_MBOX0_MASK, 1);
	/* disable PSCALE for hardware sharing */
#ifndef FOR_S5
	WRITE_VREG(HEVC_PSCALE_CTRL, 0);
#endif

	WRITE_VREG(DEBUG_REG1, 0x0);
	/*check vps/sps/pps/i-slice in ucode*/
	WRITE_VREG(NAL_SEARCH_CTL, 0x8);

	WRITE_VREG(DECODE_STOP_POS, udebug_flag);
#if (defined DEBUG_UCODE_LOG) || (defined DEBUG_CMD)
	WRITE_VREG(HEVC_DBG_LOG_ADR, dec->ucode_log_phy_addr);
#endif
	config_cuva_buf(dec);
	if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_S5) {
		WRITE_VREG(HEVC_SAO_CRC, 0);
		if (dec->front_back_mode == 1)
			WRITE_VREG(HEVC_SAO_CRC_DBE1, 0);
	}

}

#ifdef I_ONLY_SUPPORT
static int vavs3_set_trickmode(struct vdec_s *vdec, unsigned long trickmode)
{
	struct AVS3Decoder_s *dec =
		(struct AVS3Decoder_s *)vdec->private;
	if (i_only_flag & 0x100)
		return 0;
	if (trickmode == TRICKMODE_I || trickmode == TRICKMODE_I_HEVC)
		dec->i_only = 0x3;
	else if (trickmode == TRICKMODE_NONE)
		dec->i_only = 0x0;
	return 0;
}
#endif

static int vavs3_local_init(struct AVS3Decoder_s *dec)
{
	int i;
	int ret;
	int width, height;

	dec->vavs3_ratio = dec->vavs3_amstream_dec_info.ratio;

	dec->gvs = vzalloc(sizeof(struct vdec_info));
	if (NULL == dec->gvs) {
		avs3_print(dec, 0,
			"the struct of vdec status malloc failed.\n");
		return -1;
	}
#ifdef DEBUG_PTS
	dec->pts_missed = 0;
	dec->pts_hit = 0;
#endif
	dec->new_frame_displayed = 0;
	dec->last_put_idx = -1;
	dec->saved_resolution = 0;
	dec->get_frame_dur = false;
	on_no_keyframe_skiped = 0;
	width = dec->vavs3_amstream_dec_info.width;
	height = dec->vavs3_amstream_dec_info.height;
	dec->frame_dur =
		(dec->vavs3_amstream_dec_info.rate ==
			0) ? 3600 : dec->vavs3_amstream_dec_info.rate;
	if (width && height)
		dec->frame_ar = height * 0x100 / width;
/*
TODO:FOR VERSION
*/
	avs3_print(dec, AVS3_DBG_BUFMGR,
		"avs3: ver (%d,%d) decinfo: %dx%d rate=%d\n", avs3_version,
			0, width, height, dec->frame_dur);

	if (dec->frame_dur == 0)
		dec->frame_dur = 96000 / 24;
#ifdef I_ONLY_SUPPORT
	if (i_only_flag & 0x100)
		dec->i_only = i_only_flag & 0xff;
	else if ((unsigned long) dec->vavs3_amstream_dec_info.param
		& 0x08)
		dec->i_only = 0x7;
	else
		dec->i_only = 0x0;
#endif
	INIT_KFIFO(dec->display_q);
	INIT_KFIFO(dec->newframe_q);

	for (i = 0; i < VF_POOL_SIZE; i++) {
		const struct vframe_s *vf = &dec->vfpool[i];
		dec->vfpool[i].index = -1;
		kfifo_put(&dec->newframe_q, vf);
	}

	ret = avs3_local_init(dec);

	return ret;
}

static s32 vavs3_init(struct vdec_s *vdec)
{
	int ret = -1, size = -1;
	int fw_size = 0x1000 * 16;
	struct firmware_s *fw = NULL;
	struct AVS3Decoder_s *dec = (struct AVS3Decoder_s *)vdec->private;
	struct avs3_decoder *avs3_dec = &dec->avs3_dec;
#ifdef NEW_FB_CODE
	struct firmware_s *fw_back = NULL;
#endif

	avs3_dec->start_time = div64_u64(local_clock(), 1000);
	timer_setup(&dec->timer, vavs3_put_timer_func, 0);
#ifdef NEW_FB_CODE
	timer_setup(&dec->timer_back, avs3_check_timer_back_func, 0);
	dec->stat |= STAT_TIMER_BACK_INIT;
#endif

	dec->stat |= STAT_TIMER_INIT;
	if (vavs3_local_init(dec) < 0)
		return -EBUSY;

	vdec_set_vframe_comm(vdec, DRIVER_NAME);

	fw = vmalloc(sizeof(struct firmware_s) + fw_size);
	if (IS_ERR_OR_NULL(fw))
		return -ENOMEM;
#ifdef NEW_FB_CODE
	if (dec->front_back_mode == 1 || dec->front_back_mode == 3) {
		fw_back = vzalloc(sizeof(struct firmware_s) + fw_size);
		if (IS_ERR_OR_NULL(fw_back))
			return -ENOMEM;

		size = get_firmware_data(VIDEO_DEC_AVS3_FRONT, fw->data);

		fw_back->len = get_firmware_data(VIDEO_DEC_AVS3_BACK, fw_back->data);
		if (fw_back->len < 0) {
			pr_err("get back firmware fail.\n");
			vfree(fw_back);
			return -1;
		}
	} else
#endif
	size = get_firmware_data(VIDEO_DEC_AVS3, fw->data);
	if (size < 0) {
		pr_err("get firmware fail.\n");
		vfree(fw);
		return -1;
	}

	fw->len = fw_size;

	if (dec->m_ins_flag) {
		dec->timer.expires = jiffies + PUT_INTERVAL;
#ifdef NEW_FB_CODE
		if (dec->front_back_mode) {
			dec->timer_back.expires = jiffies + PUT_INTERVAL;
			dec->fw_back = fw_back;
		}
#endif
		/*add_timer(&dec->timer);

		dec->stat |= STAT_TIMER_ARM;
		dec->stat |= STAT_ISR_REG;*/

		INIT_WORK(&dec->work, avs3_work);
#ifdef NEW_FB_CODE
		if (dec->front_back_mode) {
			INIT_WORK(&dec->work_back, avs3_work_back);
			INIT_WORK(&dec->timeout_work_back, avs3_timeout_work_back);
			mutex_init(&dec->fb_mutex);
		}
#endif
		dec->fw = fw;

		return 0;
	}

	amhevc_enable();

	ret = amhevc_loadmc_ex(VFORMAT_AVS3, NULL, fw->data);
	if (ret < 0) {
		amhevc_disable();
		vfree(fw);
		pr_err("AVS3: the %s fw loading failed, err: %x\n",
			fw_tee_enabled() ? "TEE" : "local", ret);
		return -EBUSY;
	}

	vfree(fw);

	dec->stat |= STAT_MC_LOAD;

	/* enable AMRISC side protocol */
	vavs3_prot_init(dec);

	if (vdec_request_threaded_irq(VDEC_IRQ_0,
				vavs3_isr,
				vavs3_isr_thread_fn,
				IRQF_ONESHOT,/*run thread on this irq disabled*/
				"vavs3-irq", (void *)dec)) {
		pr_info("vavs3 irq register error.\n");
		amhevc_disable();
		return -ENOENT;
	}
	dec->stat |= STAT_ISR_REG;

	dec->provider_name = PROVIDER_NAME;
	vf_provider_init(&vavs3_vf_prov, PROVIDER_NAME,
				&vavs3_vf_provider, dec);
	vf_reg_provider(&vavs3_vf_prov);
	vf_notify_receiver(PROVIDER_NAME, VFRAME_EVENT_PROVIDER_START, NULL);
	if (dec->frame_dur != 0) {
		if (!is_reset)
			vf_notify_receiver(dec->provider_name,
					VFRAME_EVENT_PROVIDER_FR_HINT,
					(void *)
					((unsigned long)dec->frame_dur));
	}
	dec->stat |= STAT_VF_HOOK;

	dec->timer.expires = jiffies + PUT_INTERVAL;
	add_timer(&dec->timer);
	dec->stat |= STAT_TIMER_ARM;

	/* dec->stat |= STAT_KTHREAD; */
	dec->process_busy = 0;
	avs3_print(dec, AVS3_DBG_BUFMGR_MORE,
		"%d, vavs3_init, RP=0x%x\n",
		__LINE__, READ_VREG(HEVC_STREAM_RD_PTR));
	return 0;
}

static int vmavs3_stop(struct AVS3Decoder_s *dec)
{
	dec->init_flag = 0;
	dec->first_sc_checked = 0;
	if (dec->stat & STAT_TIMER_ARM) {
		del_timer_sync(&dec->timer);
		dec->stat &= ~STAT_TIMER_ARM;
	}
#ifdef NEW_FB_CODE
	if (dec->front_back_mode && (dec->stat & STAT_TIMER_BACK_ARM)) {
		del_timer_sync(&dec->timer_back);
		dec->stat &= ~STAT_TIMER_BACK_ARM;
	}
#endif
	if (dec->stat & STAT_VF_HOOK) {
		if (!is_reset)
			vf_notify_receiver(dec->provider_name,
					VFRAME_EVENT_PROVIDER_FR_END_HINT,
					NULL);

		vf_unreg_provider(&vavs3_vf_prov);
		dec->stat &= ~STAT_VF_HOOK;
	}
	avs3_local_uninit(dec);
	reset_process_time(dec);
	cancel_work_sync(&dec->work);
#ifdef NEW_FB_CODE
	if (dec->front_back_mode) {
		cancel_work_sync(&dec->work_back);
		vfree(dec->fw_back);
		dec->fw_back = NULL;
	}
#endif
	uninit_mmu_buffers(dec);
	if (dec->fw) {
		vfree(dec->fw);
		dec->fw = NULL;
	}

	return 0;
}

static int vavs3_stop(struct AVS3Decoder_s *dec)
{

	dec->init_flag = 0;
	dec->first_sc_checked = 0;
	if (dec->stat & STAT_VDEC_RUN) {
		amhevc_stop();
		dec->stat &= ~STAT_VDEC_RUN;
	}

	if (dec->stat & STAT_ISR_REG) {
		if (!dec->m_ins_flag)
			WRITE_VREG(dec->ASSIST_MBOX0_MASK, 0);
		vdec_free_irq(VDEC_IRQ_0, (void *)dec);
		dec->stat &= ~STAT_ISR_REG;
	}

	if (dec->stat & STAT_TIMER_ARM) {
		del_timer_sync(&dec->timer);
		dec->stat &= ~STAT_TIMER_ARM;
	}
#ifdef NEW_FB_CODE
	if (dec->front_back_mode && (dec->stat & STAT_TIMER_BACK_ARM)) {
		del_timer_sync(&dec->timer_back);
		dec->stat &= ~STAT_TIMER_BACK_ARM;
	}
#endif

	if (dec->stat & STAT_VF_HOOK) {
		if (!is_reset)
			vf_notify_receiver(dec->provider_name,
					VFRAME_EVENT_PROVIDER_FR_END_HINT,
					NULL);

		vf_unreg_provider(&vavs3_vf_prov);
		dec->stat &= ~STAT_VF_HOOK;
	}
	avs3_local_uninit(dec);

	if (dec->m_ins_flag) {
		cancel_work_sync(&dec->work);
#ifdef NEW_FB_CODE
		if (dec->front_back_mode) {
			cancel_work_sync(&dec->work_back);
			vfree(dec->fw_back);
			dec->fw_back = NULL;
		}
#endif
	} else
		amhevc_disable();
	uninit_mmu_buffers(dec);

	return 0;
}

static int amvdec_avs3_mmu_init(struct AVS3Decoder_s *dec)
{
	int tvp_flag = vdec_secure(hw_to_vdec(dec)) ?
		CODEC_MM_FLAGS_TVP : 0;
	int buf_size = 48;

	dec->need_cache_size = buf_size * SZ_1M;
	dec->sc_start_time = get_jiffies_64();
#ifdef AVS3_10B_MMU
	if (dec->mmu_enable) {
		dec->mmu_box = decoder_mmu_box_alloc_box(DRIVER_NAME,
			dec->index, FRAME_BUFFERS,
			dec->need_cache_size,
			tvp_flag
			);
		if (!dec->mmu_box) {
			pr_err("avs3 alloc mmu box failed!!\n");
			return -1;
		}
#ifdef NEW_FB_CODE
		if (dec->front_back_mode) {
			dec->mmu_box_1 = decoder_mmu_box_alloc_box(DRIVER_NAME,
				dec->index, FRAME_BUFFERS,
				dec->need_cache_size,
				tvp_flag
				);
			if (!dec->mmu_box_1) {
				pr_err("avs3 alloc mmu box1 failed!!\n");
				return -1;
			}
		}
	}
#endif
#ifdef AVS3_10B_MMU_DW
	if (dec->dw_mmu_enable) {
		dec->dw_mmu_box = decoder_mmu_box_alloc_box(DRIVER_NAME,
			dec->index, FRAME_BUFFERS,
			dec->need_cache_size,
			tvp_flag
			);
		if (!dec->dw_mmu_box) {
			pr_err("avs3 alloc dw mmu box failed!!\n");
			dec->dw_mmu_enable = 0;
		}
#ifdef NEW_FB_CODE
		if (dec->front_back_mode) {
			dec->dw_mmu_box_1 = decoder_mmu_box_alloc_box(DRIVER_NAME,
				dec->index, FRAME_BUFFERS,
				dec->need_cache_size,
				tvp_flag
				);
			if (!dec->dw_mmu_box_1) {
				pr_err("avs3 alloc dw mmu box1 failed!!\n");
				dec->dw_mmu_enable = 0;
			}
		}
#endif
	}
#endif
	dec->bmmu_box = decoder_bmmu_box_alloc_box(
			DRIVER_NAME,
			dec->index,
			MAX_BMMU_BUFFER_NUM,
			4 + PAGE_SHIFT,
			CODEC_MM_FLAGS_CMA_CLEAR |
			CODEC_MM_FLAGS_FOR_VDECODER |
			tvp_flag,
			BMMU_ALLOC_FLAGS_WAITCLEAR);
	if (!dec->bmmu_box) {
		pr_err("avs3 alloc bmmu box failed!!\n");
		return -1;
	}
	return 0;
}

static int amvdec_avs3_probe(struct platform_device *pdev)
{
	struct vdec_s *pdata = *(struct vdec_s **)pdev->dev.platform_data;
	/*struct BUF_s BUF[MAX_BUF_NUM];*/
	struct AVS3Decoder_s *dec = &gAVS3Decoder;
	int ret;
	pr_info("%s\n", __func__);

	dec = vzalloc(sizeof(struct AVS3Decoder_s));
	if (!dec)
		return -ENOMEM;

	pdata->private = dec;
	platform_set_drvdata(pdev, pdata);

	mutex_lock(&vavs3_mutex);

	dec->init_flag = 0;
	dec->first_sc_checked = 0;
	dec->eos = 0;
	dec->start_process_time = 0;
	dec->timeout_num = 0;
	dec->fatal_error = 0;
	dec->show_frame_num = 0;
	if (pdata == NULL) {
		avs3_print(dec, 0,
			"\namvdec_avs3 memory resource undefined.\n");
		mutex_unlock(&vavs3_mutex);
		return -EFAULT;
	}
	dec->m_ins_flag = 0;
	dec->platform_dev = pdev;
	platform_set_drvdata(pdev, pdata);

#ifdef NEW_FB_CODE
	dec->front_back_mode = 0;
#endif
	config_hevc_irq_num(dec);

#ifdef AVS3_10B_MMU_DW
	if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_T5D) {
		dec->dw_mmu_enable =
			(get_double_write_mode(dec) & 0x20) ? 1 : 0;
	} else {
		dec->dw_mmu_enable = 0;
	}
#endif
	if (amvdec_avs3_mmu_init(dec) < 0) {
		mutex_unlock(&vavs3_mutex);
		pr_err("avs3 alloc bmmu box failed!!\n");
		return -1;
	}

	ret = decoder_bmmu_box_alloc_buf_phy(dec->bmmu_box, WORK_SPACE_BUF_ID,
			work_buf_size, DRIVER_NAME, &pdata->mem_start);
	if (ret < 0) {
		uninit_mmu_buffers(dec);
		mutex_unlock(&vavs3_mutex);
		return ret;
	}
	dec->buf_size = work_buf_size;

	dec->buf_start = pdata->mem_start;

	if (debug) {
		avs3_print(dec, 0,
			"===AVS3 decoder mem resource 0x%lx size 0x%x\n",
				pdata->mem_start, dec->buf_size);
	}

	if (pdata->sys_info) {
		dec->vavs3_amstream_dec_info = *pdata->sys_info;
		dec->frame_width = dec->vavs3_amstream_dec_info.width;
		dec->frame_height = dec->vavs3_amstream_dec_info.height;
	} else {
		dec->vavs3_amstream_dec_info.width = 0;
		dec->vavs3_amstream_dec_info.height = 0;
		dec->vavs3_amstream_dec_info.rate = 30;
	}
	dec->cma_dev = pdata->cma_dev;

	dec->endian = HEVC_CONFIG_LITTLE_ENDIAN;
	if (is_support_vdec_canvas())
		dec->endian = HEVC_CONFIG_BIG_ENDIAN;
	if (endian)
		dec->endian = endian;

	pdata->private = dec;
	pdata->dec_status = vavs3_dec_status;
	/*pdata->set_isreset = vavs3_set_isreset;*/
	is_reset = 0;
	if (vavs3_init(pdata) < 0) {
		pr_info("\namvdec_avs3 init failed.\n");
		avs3_local_uninit(dec);
		uninit_mmu_buffers(dec);
		pdata->dec_status = NULL;
		mutex_unlock(&vavs3_mutex);
		return -ENODEV;
	}
	/*set the max clk for smooth playing...*/
	hevc_source_changed(VFORMAT_AVS3,
			4096, 2048, 60);
	mutex_unlock(&vavs3_mutex);

	return 0;
}

static int amvdec_avs3_remove(struct platform_device *pdev)
{
	struct AVS3Decoder_s *dec = &gAVS3Decoder;
	if (debug)
		pr_info("amvdec_avs3_remove\n");

	mutex_lock(&vavs3_mutex);

	vavs3_stop(dec);

	hevc_source_changed(VFORMAT_AVS3, 0, 0, 0);

#ifdef DEBUG_PTS
	pr_info("pts missed %ld, pts hit %ld, duration %d\n",
			dec->pts_missed, dec->pts_hit, dec->frame_dur);
#endif

	mutex_unlock(&vavs3_mutex);

	return 0;
}

/****************************************/

static struct platform_driver amvdec_avs3_driver = {
	.probe = amvdec_avs3_probe,
	.remove = amvdec_avs3_remove,
#ifdef CONFIG_PM
	.suspend = amhevc_suspend,
	.resume = amhevc_resume,
#endif
	.driver = {
		.name = DRIVER_NAME,
	}
};
static struct codec_profile_t amvdec_avs3_profile = {
	.name = "avs3",
	.profile = ""
};

static struct codec_profile_t amvdec_avs3_profile_mult;
static unsigned char get_data_check_sum
	(struct AVS3Decoder_s *dec, int size)
{
	int jj;
	int sum = 0;
	u8 *data = NULL;

	if (!dec->chunk->block->is_mapped)
		data = codec_mm_vmap(dec->chunk->block->start +
			dec->chunk->offset, size);
	else
		data = ((u8 *)dec->chunk->block->start_virt) +
			dec->chunk->offset;

	for (jj = 0; jj < size; jj++)
		sum += data[jj];

	if (!dec->chunk->block->is_mapped)
		codec_mm_unmap_phyaddr(data);
	return sum;
}

static void dump_data(struct AVS3Decoder_s *dec, int size)
{
	int jj;
	u8 *data = NULL;
	int padding_size = dec->chunk->offset &
		(VDEC_FIFO_ALIGN - 1);

	if (!dec->chunk->block->is_mapped)
		data = codec_mm_vmap(dec->chunk->block->start +
			dec->chunk->offset, size);
	else
		data = ((u8 *)dec->chunk->block->start_virt) +
			dec->chunk->offset;

	avs3_print(dec, 0, "padding: ");
	for (jj = padding_size; jj > 0; jj--)
		avs3_print_cont(dec,
			0,
			"%02x ", *(data - jj));
	avs3_print_cont(dec, 0, "data adr %p\n",
		data);

	for (jj = 0; jj < size; jj++) {
		if ((jj & 0xf) == 0)
			avs3_print(dec,
				0,
				"%06x:", jj);
		avs3_print_cont(dec,
			0,
			"%02x ", data[jj]);
		if (((jj + 1) & 0xf) == 0)
			avs3_print(dec,
				0,
				"\n");
	}
	avs3_print(dec,
		0,
		"\n");

	if (!dec->chunk->block->is_mapped)
		codec_mm_unmap_phyaddr(data);
}

void wait_shift_byte_search_done(struct AVS3Decoder_s *dec)
{
	u32 shift_byte = READ_VREG(HEVC_SHIFT_BYTE_COUNT);
	int count = 0;
	do {
		usleep_range(1000, 1010);
		if (shift_byte == READ_VREG(HEVC_SHIFT_BYTE_COUNT))
			break;
		if (count > 200) {
			avs3_print(dec,
			0, "%s timeout count %d vld_rp 0x%x VLD_MEM_VIFIFO_RP 0x%x\n",
			 __func__, count, shift_byte, READ_VREG(HEVC_SHIFT_BYTE_COUNT));
			break;
		} else
			shift_byte = READ_VREG(HEVC_SHIFT_BYTE_COUNT);
		count++;
	} while (1);
		pr_err("count %d, shift_byte 0x%x\n", count, shift_byte);
}
void wait_hevc_search_done(struct AVS3Decoder_s *dec)
{
	int count = 0;
	WRITE_VREG(HEVC_SHIFT_STATUS, 0);
	while (READ_VREG(HEVC_STREAM_CONTROL) & 0x2) {
		usleep_range(100, 101);
		count++;
		if (count > 100) {
			avs3_print(dec, 0, "%s timeout\n", __func__);
			break;
		}
	}
}

static int avs3_wait_alloc_buf(void *args)
{
	struct AVS3Decoder_s *dec = (struct AVS3Decoder_s *)args;
	struct vdec_s *vdec = hw_to_vdec(dec);

	init_pic_list(dec, dec->avs3_dec.lcu_size_log2);

	vdec_up(vdec);

	return 0;
}

static void avs3_work_implement(struct AVS3Decoder_s *dec)
{
	struct vdec_s *vdec = hw_to_vdec(dec);
	struct avs3_decoder *avs3_dec = &dec->avs3_dec;

	/*if (dec->dec_result == DEC_RESULT_DONE)
		decoder_trace(dec->trace.decode_time_name, DECODER_WORKER_START, TRACE_BASIC);
	else if (dec->dec_result == DEC_RESULT_AGAIN)
		decoder_trace(dec->trace.decode_time_name, DECODER_WORKER_AGAIN, TRACE_BASIC);*/

	/* finished decoding one frame or error,
	 * notify vdec core to switch context
	 */
	avs3_print(dec, PRINT_FLAG_VDEC_DETAIL,
		"%s dec_result %d %x %x %x\n",
		__func__,
		dec->dec_result,
		READ_VREG(HEVC_STREAM_LEVEL),
		READ_VREG(HEVC_STREAM_WR_PTR),
		READ_VREG(HEVC_STREAM_RD_PTR));

	if (((dec->dec_result == DEC_RESULT_GET_DATA) ||
		(dec->dec_result == DEC_RESULT_GET_DATA_RETRY))
		&& (hw_to_vdec(dec)->next_status !=
		VDEC_STATUS_DISCONNECTED)) {
		if (!vdec_has_more_input(vdec)) {
			dec->dec_result = DEC_RESULT_EOS;
			vdec_schedule_work(&dec->work);
			return;
		}

		if (dec->dec_result == DEC_RESULT_GET_DATA) {
			avs3_print(dec, PRINT_FLAG_VDEC_STATUS,
				"%s DEC_RESULT_GET_DATA %x %x %x\n",
				__func__,
				READ_VREG(HEVC_STREAM_LEVEL),
				READ_VREG(HEVC_STREAM_WR_PTR),
				READ_VREG(HEVC_STREAM_RD_PTR));
			vdec_vframe_dirty(vdec, dec->chunk);
			vdec_clean_input(vdec);
		}

		if (get_free_buf_count(dec) >=
			run_ready_min_buf_num) {
			int r;
			int decode_size;
			r = vdec_prepare_input(vdec, &dec->chunk);
			if (r < 0) {
				dec->dec_result = DEC_RESULT_GET_DATA_RETRY;

				avs3_print(dec,
					PRINT_FLAG_VDEC_DETAIL,
					"amvdec_avs3: Insufficient data\n");

				vdec_schedule_work(&dec->work);
				return;
			}
			dec->dec_result = DEC_RESULT_NONE;
			avs3_print(dec, PRINT_FLAG_VDEC_STATUS,
				"%s: chunk size 0x%x sum 0x%x\n",
				__func__, r,
				(debug & PRINT_FLAG_VDEC_STATUS) ?
				get_data_check_sum(dec, r) : 0
				);
			if (debug & PRINT_FLAG_VDEC_DATA)
				dump_data(dec, dec->chunk->size);

			decode_size = dec->chunk->size +
				(dec->chunk->offset & (VDEC_FIFO_ALIGN - 1));

			WRITE_VREG(HEVC_DECODE_SIZE,
				READ_VREG(HEVC_DECODE_SIZE) + decode_size);

			vdec_enable_input(vdec);

			WRITE_VREG(HEVC_DEC_STATUS_REG, AVS3_ACTION_DONE);

			start_process_time(dec);

		} else{
			dec->dec_result = DEC_RESULT_GET_DATA_RETRY;

			avs3_print(dec, PRINT_FLAG_VDEC_DETAIL,
				"amvdec_avs3: Insufficient data\n");

			vdec_schedule_work(&dec->work);
		}
		return;
	} else if ((dec->dec_result == DEC_RESULT_DONE) ||
		(dec->dec_result == DEC_RESULT_ERROR)) {
		struct avs3_frame_s *pic = dec->avs3_dec.cur_pic;
		/* if (!dec->ctx_valid)
			dec->ctx_valid = 1; */
		dec->slice_idx++;
		dec->frame_count++;
		dec->process_state = PROC_STATE_INIT;
		decode_frame_count[dec->index] = dec->frame_count;

		if ((pic != NULL) && (pic->error_mark) &&
			(dec->dec_result == DEC_RESULT_DONE)) {
#ifdef NEW_FB_CODE
			mutex_lock(&dec->fb_mutex);
#endif
			dec->gvs->error_frame_count++;
			if (pic->slice_type == SLICE_I) {
				dec->gvs->i_concealed_frames++;
			} else if (pic->slice_type == SLICE_P) {
				dec->gvs->p_concealed_frames++;
			} else if (pic->slice_type == SLICE_B) {
				dec->gvs->b_concealed_frames++;
			}

			if (pic->backend_ref == 0) {
				dec->gvs->drop_frame_count++;
				if (pic->slice_type == SLICE_I) {
					dec->gvs->i_lost_frames++;
				} else if (pic->slice_type == SLICE_P) {
					dec->gvs->p_lost_frames++;
				} else if (pic->slice_type == SLICE_B) {
					dec->gvs->b_lost_frames++;
				}
			}
#ifdef NEW_FB_CODE
			mutex_unlock(&dec->fb_mutex);
#endif
		}

		avs3_print(dec, PRINT_FLAG_VDEC_STATUS,
			"%s (===> %d) dec_result %d %x %x %x stream crc %x shiftbytes 0x%x decbytes 0x%x\n",
			__func__,
			dec->frame_count,
			dec->dec_result,
			READ_VREG(HEVC_STREAM_LEVEL),
			READ_VREG(HEVC_STREAM_WR_PTR),
			READ_VREG(HEVC_STREAM_RD_PTR),
			READ_VREG(HEVC_STREAM_CRC),
			READ_VREG(HEVC_SHIFT_BYTE_COUNT),
			READ_VREG(HEVC_SHIFT_BYTE_COUNT) -
			dec->start_shift_bytes
			);
		vdec_vframe_dirty(hw_to_vdec(dec), dec->chunk);
	} else if (dec->dec_result == DEC_RESULT_AGAIN) {
		/*
			stream base: stream buf empty or timeout
			frame base: vdec_prepare_input fail
		*/
		if (!vdec_has_more_input(vdec)) {
			dec->dec_result = DEC_RESULT_EOS;
			vdec_schedule_work(&dec->work);
			return;
		}
	} else if (dec->dec_result == DEC_RESULT_EOS) {
		DEC_CTX *avs3_ctx = &avs3_dec->ctx;
		avs3_print(dec, 0,
			"%s: end of stream\n",
			__func__);
		dec->eos = 1;
		if (dec->avs3_dec.cur_pic != NULL) {
			avs3_bufmgr_post_process(&dec->avs3_dec);
		}

		//output all pic;
		avs3_ctx->info.pic_header.decode_order_index =
			avs3_ctx->info.pic_header.decode_order_index + DOI_CYCLE_LENGTH;
		avs3_prepare_display_buf(dec);
		vdec_vframe_dirty(hw_to_vdec(dec), dec->chunk);
	} else if (dec->dec_result == DEC_RESULT_FORCE_EXIT) {
		avs3_print(dec, PRINT_FLAG_VDEC_STATUS,
			"%s: force exit\n",
			__func__);
		if (dec->stat & STAT_VDEC_RUN) {
			amhevc_stop();
			dec->stat &= ~STAT_VDEC_RUN;
		}

		if (dec->stat & STAT_ISR_REG) {
			if (!dec->m_ins_flag)
				WRITE_VREG(dec->ASSIST_MBOX0_MASK, 0);
			vdec_free_irq(VDEC_IRQ_0, (void *)dec);
			dec->stat &= ~STAT_ISR_REG;
		}
	} else if (dec->dec_result == DEC_RESULT_WAIT_BUFFER) {
		pr_err("DEC_RESULT_WAIT_BUFFER in\n");
		vdec_post_task(avs3_wait_alloc_buf, dec);
		dec->pic_list_wait_alloc_done_flag = BUFFER_ALLOCATING;
		dec->process_state =
			PROC_STATE_DECODE_AGAIN;
	}

#ifdef NEW_FRONT_BACK_CODE
	if (!vdec->front_pic_done && (dec->front_back_mode == 1)) {
		fb_hw_status_clear(true);
		avs3_print(dec, PRINT_FLAG_VDEC_STATUS,
			"%s, clear front, status 0x%x, status_back 0x%x\n",
			__func__, dec->dec_status, dec->dec_status_back);
	}
#endif

	if (dec->front_back_mode == 1)
		amhevc_stop_f();

	if (dec->stat & STAT_TIMER_ARM) {
		del_timer_sync(&dec->timer);
		dec->stat &= ~STAT_TIMER_ARM;
	}

	wait_hevc_search_done(dec);

	if (dec->dec_result == DEC_RESULT_DONE)
		decoder_trace(dec->trace.decode_time_name, DECODER_WORKER_END, TRACE_PERFORMANCE_DETAIL);

	if (get_dbg_flag(dec) & AVS3_DBG_QOS_INFO) {
		avs3_print(dec, 0, "%s:frame_count %d, drop_frame_count %d, error_frame_count %d\n",
			__func__, dec->gvs->frame_count, dec->gvs->drop_frame_count, dec->gvs->error_frame_count);
		avs3_print(dec, 0, "i decoded_frames %d, lost_frames %d, concealed_frames %d\n",
			dec->gvs->i_decoded_frames, dec->gvs->i_lost_frames, dec->gvs->i_concealed_frames);
		avs3_print(dec, 0, "p decoded_frames %d, lost_frames %d, concealed_frames %d\n",
			dec->gvs->p_decoded_frames, dec->gvs->p_lost_frames, dec->gvs->p_concealed_frames);
		avs3_print(dec, 0, "b decoded_frames %d, lost_frames %d, concealed_frames %d\n",
			dec->gvs->b_decoded_frames, dec->gvs->b_lost_frames, dec->gvs->b_concealed_frames);
	}

	/* mark itself has all HW resource released and input released */
	if (vdec->parallel_dec ==1)
		vdec_core_finish_run(vdec, CORE_MASK_HEVC);
	else
		vdec_core_finish_run(vdec, CORE_MASK_VDEC_1 | CORE_MASK_HEVC);

	if (dec->vdec_cb)
		dec->vdec_cb(hw_to_vdec(dec), dec->vdec_cb_arg, CORE_MASK_HEVC);
}

static void avs3_work(struct work_struct *work)
{
	struct AVS3Decoder_s *dec = container_of(work,
		struct AVS3Decoder_s, work);

	avs3_work_implement(dec);
}

#ifdef NEW_FB_CODE
static void avs3_work_back_implement(struct AVS3Decoder_s *dec,
	struct vdec_s *vdec, int from)
{
	struct avs3_decoder *avs3_dec = &dec->avs3_dec;
	avs3_print(dec, PRINT_FLAG_VDEC_DETAIL,
		"[BE] %s result %d\n", __func__, dec->dec_back_result);
	ATRACE_COUNTER(dec->trace.decode_back_time_name, DECODER_WORKER_START);

	if (dec->dec_back_result == DEC_BACK_RESULT_TIMEOUT) {
		avs3_frame_t* pic = avs3_dec->next_be_decode_pic[avs3_dec->fb_rd_pos];

		mutex_lock(&dec->fb_mutex);
		if (pic->error_mark == 0) {
			dec->gvs->error_frame_count++;
			if (pic->slice_type == SLICE_I) {
				dec->gvs->i_concealed_frames++;
			} else if (pic->slice_type == SLICE_P) {
				dec->gvs->p_concealed_frames++;
			} else if (pic->slice_type == SLICE_B) {
				dec->gvs->b_concealed_frames++;
			}
		}
		mutex_lock(&dec->fb_mutex);
		pic->error_mark = 1;  /* set error mark for timeout pic */
		pic_backend_ref_operation(dec, pic, 0);
		if (debug & AVS3_DBG_PRINT_PIC_LIST)
			print_pic_pool(avs3_dec, "after dec backend_ref");

		if (without_display_mode == 0) {
			struct vframe_s *vf = NULL;
			if (kfifo_peek(&dec->display_q, &vf) && vf) {
				uint8_t index = vf->index & 0xff;
				struct avs3_frame_s *peek_pic = get_pic_by_index(dec, index);
				if (peek_pic == pic)
					vf_notify_receiver(dec->provider_name,
						VFRAME_EVENT_PROVIDER_VFRAME_READY, NULL);
			}
		} else
			vavs3_vf_put(vavs3_vf_get(dec), dec);

		if ((dec->front_back_mode == 1 ||
			dec->front_back_mode == 3) && (error_handle_policy & 0x4))

			release_free_mmu_buffers(dec);
	}

	avs3_dec->backend_decoded_count++;

	mutex_lock(&dec->fb_mutex);
	avs3_dec->fb_rd_pos++;
	if (avs3_dec->fb_rd_pos >= dec->fb_ifbuf_num)
		avs3_dec->fb_rd_pos = 0;

	avs3_dec->wait_working_buf = 0;
	mutex_unlock(&dec->fb_mutex);

	WRITE_VREG(HEVC_DEC_STATUS_DBE, AVS3_DEC_IDLE);
	if (dec->front_back_mode == 1)
		amhevc_stop_b();

	avs3_print(dec, PRINT_FLAG_VDEC_DETAIL,
		"fb_wr_pos %d, set next fb_rd_pos %d, set wait_working_buf %d\n",
		avs3_dec->fb_wr_pos, avs3_dec->fb_rd_pos, avs3_dec->wait_working_buf);

	if (!vdec->back_pic_done && (dec->front_back_mode == 1)) {
		fb_hw_status_clear(false);
		avs3_print(dec, PRINT_FLAG_VDEC_STATUS,
			"%s, clear back, status 0x%x, status_back 0x%x\n",
			__func__, dec->dec_status, dec->dec_status_back);
	}

	if (dec->stat & STAT_TIMER_BACK_ARM) {
		del_timer_sync(&dec->timer_back);
		dec->stat &= ~STAT_TIMER_BACK_ARM;
	}
	ATRACE_COUNTER(dec->trace.decode_back_time_name, DECODER_WORKER_END);

	vdec_core_finish_run(vdec, CORE_MASK_HEVC_BACK);

	if (dec->vdec_back_cb)
		dec->vdec_back_cb(hw_to_vdec(dec), dec->vdec_back_cb_arg, CORE_MASK_HEVC_BACK);

}

static void avs3_work_back(struct work_struct *work)
{
	struct AVS3Decoder_s *dec = container_of(work,
			struct AVS3Decoder_s, work_back);
	struct vdec_s *vdec = hw_to_vdec(dec);

	avs3_work_back_implement(dec, vdec, 0);

}

static void avs3_timeout_work_back(struct work_struct *work)
{
	struct AVS3Decoder_s *dec = container_of(work,
		struct AVS3Decoder_s, timeout_work_back);
	struct vdec_s *vdec = hw_to_vdec(dec);

	if (work_pending(&dec->work_back))
		return;
	avs3_work_back_implement(dec, vdec, 1);
}
#endif

static int avs3_hw_ctx_restore(struct AVS3Decoder_s *dec)
{
	/* new to do ... */
	vavs3_prot_init(dec);
	return 0;
}

#ifdef NEW_FB_CODE
	/*run_ready_back*/
static unsigned long check_input_data(struct vdec_s *vdec, unsigned long mask)
{
	struct AVS3Decoder_s *dec =
		(struct AVS3Decoder_s *)vdec->private;
	struct avs3_decoder *avs3_dec = &dec->avs3_dec;

	if (fbdebug_flag & 0x1)
		return 0;

	if (((avs3_dec->fb_wr_pos != avs3_dec->fb_rd_pos) || avs3_dec->wait_working_buf) &&
		(dec->front_back_mode))
		return mask;
	else
		return 0;
}
#endif

static unsigned long run_ready(struct vdec_s *vdec, unsigned long mask)
{
	struct AVS3Decoder_s *dec =
		(struct AVS3Decoder_s *)vdec->private;
	struct avs3_decoder *avs3_dec = &dec->avs3_dec;
	int tvp = vdec_secure(hw_to_vdec(dec)) ?
		CODEC_MM_FLAGS_TVP : 0;
	unsigned long ret = 0;
	unsigned int run_ready_case = 0;
#if 0
	if ((debug & AVS3_DBG_BE_SIMULATE_IRQ)
		&&(READ_VREG(DEBUG_REG1_DBE) ||
			READ_VREG(HEVC_DEC_STATUS_DBE)== HEVC_BE_DECODE_DATA_DONE)) {
		pr_info("Simulate BE irq\n");
		WRITE_VREG(dec->backend_ASSIST_MBOX0_IRQ_REG, 1);
	}
#endif
	if (dec->pic_list_wait_alloc_done_flag == BUFFER_ALLOCATING)
		return 0;

	if ((fbdebug_flag & 0x2) &&
		dec->front_back_mode &&
		(dec->avs3_dec.cur_pic != NULL) &&
		(dec->avs3_dec.cur_pic->back_done_mark == 0) &&
		dec->next_again_flag == 0) {
		run_ready_case = 1;
		avs3_print(dec,
			PRINT_FLAG_VDEC_DETAIL, "%s case%d\r\n", __func__, run_ready_case);
		return 0;
	}

	if (debug & AVS3_DBG_PIC_LEAK_WAIT) {
		run_ready_case = 2;
		avs3_print(dec,
			PRINT_FLAG_VDEC_DETAIL, "%s case%d\r\n", __func__, run_ready_case);
		return ret;
	}
#ifdef NEW_FB_CODE
	if (dec->front_back_mode && avs3_dec->wait_working_buf) {
		run_ready_case = 3;
		avs3_print(dec,
			PRINT_FLAG_VDEC_DETAIL, "%s case%d\r\n", __func__, run_ready_case);
		return 0xffffffff;
	}
#endif

	if (dec->eos) {
		run_ready_case = 4;
		avs3_print(dec,
			PRINT_FLAG_VDEC_DETAIL, "%s case%d\r\n", __func__, run_ready_case);
		return ret;
	}
	if (!dec->first_sc_checked) {
		int size = decoder_mmu_box_sc_check(dec->mmu_box, tvp);
#ifdef NEW_FB_CODE
/* to do:
		for dec->mmu_box_1
*/
#endif
		dec->first_sc_checked = 1;
		avs3_print(dec, 0, "vavs3 cached=%d  need_size=%d speed= %d ms\n",
			size, (dec->need_cache_size >> PAGE_SHIFT),
					(int)(get_jiffies_64() - dec->sc_start_time) * 1000/HZ);
	}

	if (dec->next_again_flag &&
		(!vdec_frame_based(vdec))) {
		u32 parser_wr_ptr =
			STBUF_READ(&vdec->vbuf, get_wp);
		if (parser_wr_ptr >= dec->pre_parser_wr_ptr &&
			(parser_wr_ptr - dec->pre_parser_wr_ptr) <
			again_threshold) {
			int r = vdec_sync_input(vdec);
			run_ready_case = 5;
			avs3_print(dec,
			PRINT_FLAG_VDEC_DETAIL, "%s case%d buf lelvel:%x\n", __func__, run_ready_case, r);
			return 0;
		}
	}
/*
	if (vdec_stream_based(vdec) && (dec->pic_list_init_flag == 0)
		&& pre_decode_buf_level != 0) {
		u32 rp, wp, level;

		rp = STBUF_READ(&vdec->vbuf, get_rp);
		wp = STBUF_READ(&vdec->vbuf, get_wp);
		if (wp < rp)
			level = vdec->input.size + wp - rp;
		else
			level = wp - rp;

		if (level < pre_decode_buf_level)
			return 0;
	}
*/

	if ((dec->pic_list_init_flag == 0) ||
		get_free_buf_count(dec) >=
		run_ready_min_buf_num)
		ret = 1;
	else {
		avs3_cleanup_useless_pic_buffer_in_pm(avs3_dec);
		if (get_free_buf_count(dec) >= run_ready_min_buf_num)
			ret = 1;
		else
			run_ready_case = 0;
	}

#ifdef CONSTRAIN_MAX_BUF_NUM
	if (dec->pic_list_init_flag) {
		if (run_ready_max_vf_only_num > 0 &&
			get_vf_ref_only_buf_count(dec) >=
			run_ready_max_vf_only_num
			) {
			run_ready_case = 10;
			ret = 0;
		}
		if (run_ready_display_q_num > 0 &&
			kfifo_len(&dec->display_q) >=
			run_ready_display_q_num) {
			run_ready_case = 11;
			ret = 0;
		}

		/*if (run_ready_max_buf_num == 0xff &&
			get_used_buf_count(dec) >=
			dec->avs3_dec.ref_maxbuffer) {
			run_ready_case = 12;
			ret = 0;
		} else*/ if (run_ready_max_buf_num &&
			get_used_buf_count(dec) >=
			run_ready_max_buf_num) {
			run_ready_case = 13;
			ret = 0;
		}
	}
#endif
	if (ret)
		not_run_ready[dec->index] = 0;
	else {
		not_run_ready[dec->index]++;
		avs3_print(dec,
			PRINT_FLAG_VDEC_DETAIL, "%s case%d\r\n", __func__, run_ready_case);
	}

	if (vdec->parallel_dec == 1)
#ifdef NEW_FB_CODE
		return ret ? mask : mask & ~(CORE_MASK_HEVC);
#else
		return ret ? CORE_MASK_HEVC : 0;
#endif
	else
		return ret ? (CORE_MASK_VDEC_1 | CORE_MASK_HEVC) : 0;
}

#ifdef NEW_FB_CODE
static void run_back(struct vdec_s *vdec, void (*callback)(struct vdec_s *, void *, int), void *arg)
{
	struct AVS3Decoder_s *dec =
		(struct AVS3Decoder_s *)vdec->private;
	int loadr = 0;
	int ret = 0;

	ATRACE_COUNTER(dec->trace.decode_back_run_time_name, TRACE_RUN_LOADING_FW_START);

	if (vdec->mc_back_loaded || dec->front_back_mode != 1) {
		/*firmware have load before,
			and not changes to another.
			ignore reload.
		*/
	} else {
		loadr = amhevc_vdec_loadmc_ex(VFORMAT_AVS3, vdec,
				"avs3_back", dec->fw_back->data);

		if (loadr < 0) {
			amhevc_disable();
			avs3_print(dec, 0, "AVS3: the %s back fw loading failed, err: %x\n",
				fw_tee_enabled() ? "TEE" : "local", loadr);
			dec->dec_back_result = DEC_BACK_RESULT_FORCE_EXIT;
			vdec_schedule_work(&dec->work_back);
			return;
		}

		//vdec->mc_back_loaded = 1;
		vdec->mc_back_type = VFORMAT_AVS3;
	}
	ATRACE_COUNTER(dec->trace.decode_back_run_time_name, TRACE_RUN_LOADING_FW_END);

	ATRACE_COUNTER(dec->trace.decode_back_run_time_name, TRACE_RUN_BACK_ALLOC_MMU_START);

	run_count_back[dec->index]++;
	dec->vdec_back_cb_arg = arg;
	dec->vdec_back_cb = callback;
	vdec->back_pic_done = false;
	//pr_err("run h265_HEVC_back_test\n");
	//vdec_post_task(h265_HEVC_back_test, hevc);

	ret = BackEnd_StartDecoding(dec);

	if (ret == 1) {
		dec->dec_back_result = DEC_BACK_RESULT_DONE;
		vdec_schedule_work(&dec->timeout_work_back);
	} else {
		mod_timer(&dec->timer_back, jiffies);
		dec->stat |= STAT_TIMER_BACK_ARM;
		start_process_time_back(dec);
	}
}

#if 0
static void start_front_end_multi_pic_decoding(struct AVS3Decoder_s *dec)
{ /*multi pictures in one packe*/
	struct avs3_decoder *avs3_dec = &dec->avs3_dec;
	printk("Start FrontEnd Decoding %d\n", avs3_dec->frontend_decoded_count);
	printk("copy loopbuf to next_bk[%d]\n",avs3_dec->fb_wr_pos);
	copy_loopbufs_ptr(&avs3_dec->next_bk[avs3_dec->fb_wr_pos], &avs3_dec->fr);

	if (dec->front_back_mode == 1)
		config_bufstate_front_hw(avs3_dec);
	WRITE_VREG(HEVC_DEC_STATUS_REG, AVS3_ACTION_DONE);
}
#endif
#endif

static void run(struct vdec_s *vdec, unsigned long mask,
	void (*callback)(struct vdec_s *, void *, int), void *arg)
{
	struct AVS3Decoder_s *dec =
		(struct AVS3Decoder_s *)vdec->private;
	int r;

#ifdef NEW_FB_CODE
	struct avs3_decoder *avs3_dec = &dec->avs3_dec;
	/*simulation code: if (dec_status == AVS3_DEC_IDLE)*/
	avs3_print(dec, PRINT_FLAG_VDEC_DETAIL, "%s(mask = 0x%lx)\n", __func__, mask);
	if (dec->front_back_mode == 0 || (mask & CORE_MASK_HEVC)) {
#endif
	if (debug & PRINT_FLAG_VDEC_STATUS)
		WRITE_VREG(HEVC_STREAM_CRC, 0);
	run_count[dec->index]++;
	dec->vdec_cb_arg = arg;
	dec->vdec_cb = callback;
	vdec->front_pic_done = false;

	decoder_trace(dec->trace.decode_time_name, DECODER_RUN_START, TRACE_PERFORMANCE_DETAIL);
	/* dec->chunk = vdec_prepare_input(vdec); */
#ifdef NEW_FRONT_BACK_CODE
	/*simulation code: if (dec_status == HEVC_DECPIC_DATA_DONE) {*/
	if (dec->front_back_mode) {
		avs3_print(dec, PRINT_FLAG_VDEC_STATUS,
			"Start FrontEnd Decoding %d\n", avs3_dec->frontend_decoded_count);
		avs3_print(dec, PRINT_FLAG_VDEC_DETAIL,
			"copy loopbuf fr to next_bk[fb_wr_pos=%d]\n",avs3_dec->fb_wr_pos);
		copy_loopbufs_ptr(&avs3_dec->next_bk[avs3_dec->fb_wr_pos], &avs3_dec->fr);

		if (dec->front_back_mode == 1)
			amhevc_reset_f();
		else
			hevc_reset_core(vdec);
	} else
#endif
	{
	hevc_reset_core(vdec);
	}
	if (vdec_stream_based(vdec)) {
		dec->pre_parser_wr_ptr =
			STBUF_READ(&vdec->vbuf, get_wp);
		dec->next_again_flag = 0;
	}

	r = vdec_prepare_input(vdec, &dec->chunk);

	if (r < 0) {
		input_empty[dec->index]++;

		dec->dec_result = DEC_RESULT_AGAIN;

		avs3_print(dec, PRINT_FLAG_VDEC_DETAIL,
			"ammvdec_avs3: Insufficient data\n");

		vdec_schedule_work(&dec->work);
		return;
	}

	input_empty[dec->index] = 0;
	dec->dec_result = DEC_RESULT_NONE;

	if (debug)
		dec->start_shift_bytes = READ_VREG(HEVC_SHIFT_BYTE_COUNT);

	if (debug & PRINT_FLAG_VDEC_STATUS) {
		int ii;
		avs3_print(dec, 0,
			"%s (%d): size 0x%x (0x%x 0x%x) sum 0x%x (%x %x %x %x %x) bytes 0x%x",
			__func__,
			dec->frame_count, r,
			dec->chunk ? dec->chunk->size : 0,
			dec->chunk ? dec->chunk->offset : 0,
			dec->chunk ? ((vdec_frame_based(vdec) &&
			(debug & PRINT_FLAG_VDEC_STATUS)) ?
			get_data_check_sum(dec, r) : 0) : 0,
		READ_VREG(HEVC_STREAM_START_ADDR),
		READ_VREG(HEVC_STREAM_END_ADDR),
		READ_VREG(HEVC_STREAM_LEVEL),
		READ_VREG(HEVC_STREAM_WR_PTR),
		READ_VREG(HEVC_STREAM_RD_PTR),
		dec->start_shift_bytes);
		if (vdec_frame_based(vdec) && dec->chunk) {
			u8 *data = NULL;
			if (!dec->chunk->block->is_mapped)
				data = codec_mm_vmap(dec->chunk->block->start +
					dec->chunk->offset, 8);
			else
				data = ((u8 *)dec->chunk->block->start_virt) +
					dec->chunk->offset;

			avs3_print_cont(dec, 0, "data adr %p:",
				data);
			for (ii = 0; ii < 8; ii++)
				avs3_print_cont(dec, 0, "%02x ",
					data[ii]);
			if (!dec->chunk->block->is_mapped)
				codec_mm_unmap_phyaddr(data);
		}
		avs3_print_cont(dec, 0, "\r\n");
	}

	decoder_trace(dec->trace.decode_run_time_name, TRACE_RUN_LOADING_FW_START, TRACE_BASIC);
	if (vdec->mc_loaded) {
		/*firmware have load before,
			and not changes to another.
			ignore reload.
		*/
	} else {
#ifdef NEW_FB_CODE
		int loadr = 0;
		if (dec->front_back_mode == 1 || dec->front_back_mode == 3)
			loadr = amhevc_vdec_loadmc_ex(VFORMAT_AVS3, vdec,
					"avs3_front", dec->fw->data);
		else
#endif
		if (amhevc_loadmc_ex(VFORMAT_AVS3, "avs3_mmu", dec->fw->data) < 0) {
			vdec->mc_loaded = 0;
			amhevc_disable();
			avs3_print(dec, 0,
				"%s: Error amvdec_loadmc avs3_mmu fail \n", __func__);
			dec->dec_result = DEC_RESULT_FORCE_EXIT;
			vdec_schedule_work(&dec->work);
			return;
		}
		//vdec->mc_loaded = 1;
		vdec->mc_type = VFORMAT_AVS3;
	}
	decoder_trace(dec->trace.decode_run_time_name, TRACE_RUN_LOADING_FW_END, TRACE_BASIC);

	decoder_trace(dec->trace.decode_run_time_name, TRACE_RUN_LOADING_RESTORE_START, TRACE_BASIC);
#ifdef NEW_FB_CODE
	if (dec->front_back_mode) {
		avs3_hw_init(dec, 1, 0);
			//config_decode_mode(dec);
		if (dec->front_back_mode == 1) {
			config_bufstate_front_hw(avs3_dec);
			if ((fbdebug_flag & 0x8) && (avs3_dec->frontend_decoded_count == 0))
				loop_buffer_fill_zero(dec);
		}
		WRITE_VREG(HEVC_DEC_STATUS_REG, AVS3_ACTION_DONE);
	} else
#endif
	if (avs3_hw_ctx_restore(dec) < 0) {
		vdec_schedule_work(&dec->work);
		return;
	}
	decoder_trace(dec->trace.decode_run_time_name, TRACE_RUN_LOADING_RESTORE_END, TRACE_BASIC);

	if (vdec_frame_based(vdec))
		WRITE_VREG(HEVC_SHIFT_BYTE_COUNT, 0);

	vdec_enable_input(vdec);

	//WRITE_VREG(HEVC_DEC_STATUS_REG, AVS3_SEARCH_NEW_PIC);

	if (vdec_frame_based(vdec) && dec->chunk) {
		if (debug & PRINT_FLAG_VDEC_DATA)
			dump_data(dec, dec->chunk->size);

		r = dec->chunk->size +
			(dec->chunk->offset & (VDEC_FIFO_ALIGN - 1));
		if (vdec->mvfrm)
			vdec->mvfrm->frame_size = dec->chunk->size;
	}

	WRITE_VREG(HEVC_DECODE_SIZE, r);
	WRITE_VREG(HEVC_DECODE_COUNT, dec->slice_idx);
	dec->init_flag = 1;

	/*
	avs3_print(dec, PRINT_FLAG_VDEC_DETAIL,
		"%s: start hevc (%x %x %x)\n",
		__func__,
		READ_VREG(HEVC_DEC_STATUS_REG),
		READ_VREG(HEVC_MPC_E),
		READ_VREG(HEVC_MPSR));
	*/
	start_process_time(dec);
	mod_timer(&dec->timer, jiffies);
	dec->stat |= STAT_TIMER_ARM;
	dec->stat |= STAT_ISR_REG;
	if (vdec->mvfrm)
		vdec->mvfrm->hw_decode_start = local_clock();
#ifdef NEW_FB_CODE
	if (dec->front_back_mode == 1) {
		vdec->hw_front_decode_start = local_clock();
		amhevc_start_f();
	} else
#endif
	amhevc_start();
	dec->stat |= STAT_VDEC_RUN;
	decoder_trace(dec->trace.decode_time_name, DECODER_RUN_END, TRACE_PERFORMANCE_DETAIL);
#ifdef NEW_FB_CODE
	}
	if (dec->front_back_mode &&
		(mask & CORE_MASK_HEVC_BACK)) {
		ATRACE_COUNTER(dec->trace.decode_back_time_name, DECODER_RUN_START);
		run_back(vdec, callback, arg);
		ATRACE_COUNTER(dec->trace.decode_back_time_name, DECODER_RUN_END);
	}
#endif
}

static void reset(struct vdec_s *vdec)
{

	struct AVS3Decoder_s *dec =
		(struct AVS3Decoder_s *)vdec->private;

	avs3_print(dec,
		PRINT_FLAG_VDEC_DETAIL, "%s\r\n", __func__);

}

static irqreturn_t avs3_irq_cb(struct vdec_s *vdec, int irq)
{
	struct AVS3Decoder_s *dec =
		(struct AVS3Decoder_s *)vdec->private;
	return vavs3_isr(0, dec);
}

static irqreturn_t avs3_threaded_irq_cb(struct vdec_s *vdec, int irq)
{
	struct AVS3Decoder_s *dec =
		(struct AVS3Decoder_s *)vdec->private;
	irqreturn_t ret;
	//unsigned long flags;
	//lock_front_back(dec, flags);
	ret = vavs3_isr_thread_fn(0, dec);
	//unlock_front_back(dec, flags);
#ifdef USE_FRONT_ISR_HANDLE_FOR_BACK
	if ((dec->dec_status_back != AVS3_DEC_IDLE) ||
		READ_VREG(DEBUG_REG1_DBE)) {
		ret = vavs3_back_isr_thread_fn(dec);
	}
#endif
	return ret;
}

static void avs3_dump_state(struct vdec_s *vdec)
{
	struct AVS3Decoder_s *dec =
		(struct AVS3Decoder_s *)vdec->private;

	if (radr != 0) {
		if (rval != 0) {
			WRITE_VREG(radr, rval);
			pr_info("WRITE_VREG(%x,%x)\n", radr, rval);
		} else
			pr_info("READ_VREG(%x)=%x\n", radr, READ_VREG(radr));
		rval = 0;
		radr = 0;
		return;
	}

	avs3_print(dec, 0, "====== %s\n", __func__);
	avs3_print(dec, 0,
		"width/height (%d/%d), max_pb_size %d\n",
		dec->avs3_dec.img.width,
		dec->avs3_dec.img.height,
		dec->avs3_dec.max_pb_size
		);

	avs3_print(dec, 0,
		"front_back_mode (%d), is_framebase(%d), eos %d, dec_result 0x%x dec_frm %d disp_frm %d run %d not_run_ready %d input_empty %d\n",
		dec->front_back_mode,
		input_frame_based(vdec),
		dec->eos,
		dec->dec_result,
		decode_frame_count[dec->index],
		display_frame_count[dec->index],
		run_count[dec->index],
		not_run_ready[dec->index],
		input_empty[dec->index]
		);

	if (vf_get_receiver(vdec->vf_provider_name)) {
		enum receiver_start_e state =
		vf_notify_receiver(vdec->vf_provider_name,
			VFRAME_EVENT_PROVIDER_QUREY_STATE,
			NULL);
		avs3_print(dec, 0,
			"\nreceiver(%s) state %d\n",
			vdec->vf_provider_name,
			state);
	}

	avs3_print(dec, 0,
	"%s, newq(%d/%d), dispq(%d/%d), vf prepare/get/put (%d/%d/%d), free_buf_count %d (min %d for run_ready)\n",
	__func__,
	kfifo_len(&dec->newframe_q),
	VF_POOL_SIZE,
	kfifo_len(&dec->display_q),
	VF_POOL_SIZE,
	dec->vf_pre_count,
	dec->vf_get_count,
	dec->vf_put_count,
	get_free_buf_count(dec),
	run_ready_min_buf_num
	);

	print_pic_pool(&dec->avs3_dec, "");

	avs3_print(dec, 0,
		"HEVC_DEC_STATUS_REG=0x%x\n",
		READ_VREG(HEVC_DEC_STATUS_REG));
	avs3_print(dec, 0,
		"HEVC_MPC_E=0x%x\n",
		READ_VREG(HEVC_MPC_E));
	avs3_print(dec, 0,
		"DECODE_MODE=0x%x\n",
		READ_VREG(DECODE_MODE));
	avs3_print(dec, 0,
		"NAL_SEARCH_CTL=0x%x\n",
		READ_VREG(NAL_SEARCH_CTL));
	avs3_print(dec, 0,
		"HEVC_PARSER_LCU_START=0x%x\n",
		READ_VREG(HEVC_PARSER_LCU_START));
	avs3_print(dec, 0,
		"HEVC_DECODE_SIZE=0x%x\n",
		READ_VREG(HEVC_DECODE_SIZE));
	avs3_print(dec, 0,
		"HEVC_SHIFT_BYTE_COUNT=0x%x\n",
		READ_VREG(HEVC_SHIFT_BYTE_COUNT));
	avs3_print(dec, 0,
		"HEVC_STREAM_START_ADDR=0x%x\n",
		READ_VREG(HEVC_STREAM_START_ADDR));
	avs3_print(dec, 0,
		"HEVC_STREAM_END_ADDR=0x%x\n",
		READ_VREG(HEVC_STREAM_END_ADDR));
	avs3_print(dec, 0,
		"HEVC_STREAM_LEVEL=0x%x\n",
		READ_VREG(HEVC_STREAM_LEVEL));
	avs3_print(dec, 0,
		"HEVC_STREAM_WR_PTR=0x%x\n",
		READ_VREG(HEVC_STREAM_WR_PTR));
	avs3_print(dec, 0,
		"HEVC_STREAM_RD_PTR=0x%x\n",
		READ_VREG(HEVC_STREAM_RD_PTR));
	avs3_print(dec, 0,
		"PARSER_VIDEO_RP=0x%x\n",
		STBUF_READ(&vdec->vbuf, get_rp));
	avs3_print(dec, 0,
		"PARSER_VIDEO_WP=0x%x\n",
		STBUF_READ(&vdec->vbuf, get_wp));

	if (input_frame_based(vdec) &&
		(debug & PRINT_FLAG_VDEC_DATA)
		) {
		int jj;
		if (dec->chunk && dec->chunk->block &&
			dec->chunk->size > 0) {
			u8 *data = NULL;
			if (!dec->chunk->block->is_mapped)
				data = codec_mm_vmap(dec->chunk->block->start +
					dec->chunk->offset, dec->chunk->size);
			else
				data = ((u8 *)dec->chunk->block->start_virt) +
					dec->chunk->offset;
			avs3_print(dec, 0,
				"frame data size 0x%x\n",
				dec->chunk->size);
			for (jj = 0; jj < dec->chunk->size; jj++) {
				if ((jj & 0xf) == 0)
					avs3_print(dec, 0,
						"%06x:", jj);
				avs3_print_cont(dec, 0,
					"%02x ", data[jj]);
				if (((jj + 1) & 0xf) == 0)
					avs3_print_cont(dec, 0,
						"\n");
			}

			if (!dec->chunk->block->is_mapped)
				codec_mm_unmap_phyaddr(data);
		}
	}

	if (dec->front_back_mode == 1) {
		struct avs3_decoder *avs3_dec = &dec->avs3_dec;
		avs3_print(dec, 0,
			"[BE] dec_back_result 0x%x, frontend_decoded_count %d, backend_decoded_count %d, fb_wr_pos %d, fb_rd_pos %d, wait_working_buf %d\n",
			dec->dec_back_result,
			avs3_dec->frontend_decoded_count,
			avs3_dec->backend_decoded_count,
			avs3_dec->fb_wr_pos,
			avs3_dec->fb_rd_pos,
			avs3_dec->wait_working_buf
		);

		avs3_print(dec, 0,
			"[BE] HEVC_DEC_STATUS_DBE=0x%x\n",
			READ_VREG(HEVC_DEC_STATUS_DBE));
		avs3_print(dec, 0,
			"[BE] HEVC_MPC_E_DBE=0x%x\n",
			READ_VREG(HEVC_MPC_E_DBE));
		avs3_print(dec, 0,
			"[BE] HEVC_MPSR_DBE=0x%x\n",
			READ_VREG(HEVC_MPSR_DBE));
		avs3_print(dec, 0,
			"[BE] DEBUG_REG1_DBE=0x%x\n",
			READ_VREG(DEBUG_REG1_DBE));
		avs3_print(dec, 0,
			"[BE] DEBUG_REG2_DBE=0x%x\n",
			READ_VREG(DEBUG_REG2_DBE));
		print_loopbufs_adr_size(dec);
		print_loopbufs_ptr(dec, "fr", &avs3_dec->fr);
		print_loopbufs_ptr(dec, "bk", &avs3_dec->bk);
	}

	if (dump_phy_adr != 0 && dump_phy_size != 0) {
		dump_or_fill_phy_buffer(dec, dump_phy_adr, dump_phy_size, NULL, 2, NULL);
	}
	if (fbdebug_flag & 0x80000000) {
		dump_loop_buffer(dec, -1, 1);
		fbdebug_flag &= ~0x80000000;
	}
}

static void d_dump(struct AVS3Decoder_s *dec, unsigned int phy_adr, int size,
	struct file *fp, loff_t *wr_off, u32 * total_check_sum, u8 print_flag)
{
	u8 *vaddr;
	u32 check_sum = 0;
	vaddr = codec_mm_vmap(phy_adr, size);
	if (vaddr) {
		int i;
		if (total_check_sum == NULL) {
			for (i = 0; i < size; i++)
				vaddr[i] = 0;
			codec_mm_dma_flush(vaddr, size, DMA_TO_DEVICE);
		} else {
			codec_mm_dma_flush(vaddr, size, DMA_FROM_DEVICE);
			if (fp) {
				media_write(fp, vaddr, size, wr_off);
				for (i = 0; i < size; i++)
					check_sum += vaddr[i];
				*total_check_sum += check_sum;
			} else {
				for (i = 0; i < size; i++) {
					if (print_flag) {
						if ((i & 0xf) == 0)
							avs3_print_cont(dec, 0, "%08x: ", phy_adr+i);
						avs3_print_cont(dec, 0, "%02x ", vaddr[i]);
					}
					check_sum += vaddr[i];
					if (print_flag) {
						if (((i + 1) & 0xf) == 0)
							avs3_print_flush(dec); //avs3_print(dec, 0, "\n");
					}
				}
				*total_check_sum += check_sum;
				if (print_flag)
					avs3_print(dec, 0, "check_sum %08x, %08x\n", *total_check_sum, check_sum);
			}
		}
		codec_mm_unmap_phyaddr(vaddr);
	} else {
		pr_info("%s codec_mm_vmap fail\n", __func__);
	}
}

static void dump_or_fill_phy_buffer(struct AVS3Decoder_s *dec, u32 dump_phy_adr, u32 dump_phy_size, char *file, u8 flag, char *mark)
{
	/*
		flag: 1, fill zero
		flag: 2, print data
	*/
	loff_t off = 0;
	int mode = O_CREAT | O_WRONLY | O_TRUNC;
	struct file *fp = NULL;

	int dump_size = 1024;
	int remain_size = dump_phy_size;
	u32 phy_adr = dump_phy_adr;
	u32 total_check_sum = 0;

	if (file) {
		fp = media_open(file, mode, 0666);
	}
	while (remain_size > 0) {
		if (flag == 1)
			d_dump(dec, phy_adr, dump_size, NULL, NULL, NULL, 0);
		else
			d_dump(dec, phy_adr, dump_size, fp, &off, &total_check_sum, flag == 2);

		remain_size -= dump_size;
		phy_adr += dump_size;
		if (remain_size < dump_size)
			dump_size = remain_size;
		//msleep(20);
	}
	if (fp) {
		media_close(fp, current->files);
	}
	if (flag == 1)
		avs3_print(dec, 0, "fill phy mem %s %x (size %x):\n", mark?mark:"", dump_phy_adr, dump_phy_size);
	else
		avs3_print(dec, 0, "dump phy mem %s %x (size %x) check_sum %x %s\n", mark?mark:"", dump_phy_adr, dump_phy_size, total_check_sum, file? file:"");
}

static int ammvdec_avs3_probe(struct platform_device *pdev)
{
	struct vdec_s *pdata = *(struct vdec_s **)pdev->dev.platform_data;
	int ret;
	int config_val;
	int i;
	struct vframe_content_light_level_s content_light_level;
	struct vframe_master_display_colour_s vf_dp;
	/*struct BUF_s BUF[MAX_BUF_NUM];*/
	struct AVS3Decoder_s *dec = NULL;
	static struct vframe_operations_s vf_tmp_ops;

	pr_info("%s\n", __func__);

	if (get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_T5D) {
		pr_info("%s, chip id %d is not support avs3\n",
			__func__, get_cpu_major_id());
		return -1;
	}
	if (pdata == NULL) {
		pr_info("\nammvdec_avs3 memory resource undefined.\n");
		return -EFAULT;
	}
	/*dec = (struct AVS3Decoder_s *)devm_kzalloc(&pdev->dev,
		sizeof(struct AVS3Decoder_s), GFP_KERNEL);*/
	memset(&vf_dp, 0, sizeof(struct vframe_master_display_colour_s));
	dec = vzalloc(sizeof(struct AVS3Decoder_s));
	if (dec == NULL) {
		pr_info("\nammvdec_avs3 device data allocation failed\n");
		return -ENOMEM;
	}
	/*
	//move to other place after pic_pool is initialized
	if (pdata->parallel_dec == 1) {
		int i;
		for (i = 0; i < MAX_PB_SIZE; i++) {
			dec->avs3_dec.pic_pool[i].buf_cfg.y_canvas_index = -1;
			dec->avs3_dec.pic_pool[i].buf_cfg.uv_canvas_index = -1;
		}
	}
	*/
	pdata->private = dec;
	pdata->dec_status = vavs3_dec_status;
#ifdef I_ONLY_SUPPORT
	pdata->set_trickmode = vavs3_set_trickmode;
#endif
	pdata->run_ready = run_ready;
	pdata->run = run;
	dec->avs3_dec.max_pb_size = MAX_PB_SIZE; //will reconfig later
#ifdef NEW_FB_CODE
	if (get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_S5) {
		dec->front_back_mode = front_back_mode;
	} else {
		dec->front_back_mode = 0;
	}
	dec->fb_ifbuf_num = fb_ifbuf_num;
	if (dec->fb_ifbuf_num > MAX_FB_IFBUF_NUM)
		dec->fb_ifbuf_num = MAX_FB_IFBUF_NUM;
	pdata->check_input_data = NULL;
	if (dec->front_back_mode) {
		pdata->check_input_data = check_input_data;
		pdata->reset = NULL;
		pdata->back_irq_handler = avs3_back_irq_cb;
		pdata->back_threaded_irq_handler = avs3_back_threaded_irq_cb;
	} else
#endif
	pdata->reset = reset;
	pdata->irq_handler = avs3_irq_cb;
	pdata->threaded_irq_handler = avs3_threaded_irq_cb;
	pdata->dump_state = avs3_dump_state;

	/*
	 * memcpy(&BUF[0], &dec->m_BUF[0], sizeof(struct BUF_s) * MAX_BUF_NUM);
	 * memset(dec, 0, sizeof(struct AVS3Decoder_s));
	 * memcpy(&dec->m_BUF[0], &BUF[0], sizeof(struct BUF_s) * MAX_BUF_NUM);
	 */

	dec->index = pdev->id;
	dec->m_ins_flag = 1;

	config_hevc_irq_num(dec);

	if (is_rdma_enable()) {
		dec->rdma_adr = decoder_dma_alloc_coherent(&dec->rdma_mem_handle, RDMA_SIZE, &dec->rdma_phy_adr, "AVS3_RDMA_BUF");
		for (i = 0; i < SCALELUT_DATA_WRITE_NUM; i++) {
			dec->rdma_adr[i * 4] = HEVC_IQIT_SCALELUT_WR_ADDR & 0xfff;
			dec->rdma_adr[i * 4 + 1] = i;
			dec->rdma_adr[i * 4 + 2] = HEVC_IQIT_SCALELUT_DATA & 0xfff;
			dec->rdma_adr[i * 4 + 3] = 0;
			if (i == SCALELUT_DATA_WRITE_NUM - 1) {
				dec->rdma_adr[i * 4 + 2] = (HEVC_IQIT_SCALELUT_DATA & 0xfff) | 0x20000;
			}
		}
	}

	snprintf(dec->trace.vdec_name, sizeof(dec->trace.vdec_name),
		"avs3-%d", dec->index);
	snprintf(dec->trace.pts_name, sizeof(dec->trace.pts_name),
		"%s-pts", dec->trace.vdec_name);
	snprintf(dec->trace.vf_get_name, sizeof(dec->trace.vf_get_name),
		"%s-vf_get", dec->trace.vdec_name);
	snprintf(dec->trace.vf_put_name, sizeof(dec->trace.vf_put_name),
		"%s-vf_put", dec->trace.vdec_name);
	snprintf(dec->trace.set_canvas0_addr, sizeof(dec->trace.set_canvas0_addr),
		"%s-set_canvas0_addr", dec->trace.vdec_name);
	snprintf(dec->trace.get_canvas0_addr, sizeof(dec->trace.get_canvas0_addr),
		"%s-get_canvas0_addr", dec->trace.vdec_name);
	snprintf(dec->trace.put_canvas0_addr, sizeof(dec->trace.put_canvas0_addr),
		"%s-put_canvas0_addr", dec->trace.vdec_name);
	snprintf(dec->trace.new_q_name, sizeof(dec->trace.new_q_name),
		"%s-newframe_q", dec->trace.vdec_name);
	snprintf(dec->trace.disp_q_name, sizeof(dec->trace.disp_q_name),
		"%s-dispframe_q", dec->trace.vdec_name);
	snprintf(dec->trace.decode_time_name, sizeof(dec->trace.decode_time_name),
		"decoder_time%d", pdev->id);
	snprintf(dec->trace.decode_run_time_name, sizeof(dec->trace.decode_run_time_name),
		"decoder_run_time%d", pdev->id);
	snprintf(dec->trace.decode_header_memory_time_name, sizeof(dec->trace.decode_header_memory_time_name),
		"decoder_header_time%d", pdev->id);
	snprintf(dec->trace.decode_work_time_name, sizeof(dec->trace.decode_work_time_name),
		"decoder_work_time%d", pdev->id);
	snprintf(dec->trace.decode_back_time_name, sizeof(dec->trace.decode_back_time_name),
		"decoder_back_time%d", pdev->id);
	snprintf(dec->trace.decode_back_run_time_name, sizeof(dec->trace.decode_back_run_time_name),
		"decoder_back_run_time%d", pdev->id);
	snprintf(dec->trace.decode_back_work_time_name, sizeof(dec->trace.decode_back_work_time_name),
		"decoder_back_work_time%d", pdev->id);

	if (pdata->use_vfm_path) {
		snprintf(pdata->vf_provider_name, VDEC_PROVIDER_NAME_SIZE,
			VFM_DEC_PROVIDER_NAME);
		dec->frameinfo_enable = 1;
	} else
		snprintf(pdata->vf_provider_name, VDEC_PROVIDER_NAME_SIZE,
			MULTI_INSTANCE_PROVIDER_NAME ".%02x", pdev->id & 0xff);

	memcpy(&vf_tmp_ops, &vavs3_vf_provider, sizeof(struct vframe_operations_s));
	if (without_display_mode == 1) {
		vf_tmp_ops.get = NULL;
	}
	vf_provider_init(&pdata->vframe_provider, pdata->vf_provider_name,
		&vf_tmp_ops, dec);

	dec->provider_name = pdata->vf_provider_name;
	platform_set_drvdata(pdev, pdata);

	dec->platform_dev = pdev;
	dec->video_signal_type = 0;
	dec->video_ori_signal_type = 0;
	if (get_cpu_major_id() < AM_MESON_CPU_MAJOR_ID_TXLX)
		dec->stat |= VP9_TRIGGER_FRAME_ENABLE;

	if ((debug & IGNORE_PARAM_FROM_CONFIG) == 0 && pdata->config_len) {
		/*use ptr config for double_write_mode, etc*/
		avs3_print(dec, 0, "pdata->config=%s\n", pdata->config);
		if (get_config_int(pdata->config, "avs3_double_write_mode",
				&config_val) == 0)
			dec->double_write_mode = config_val;
		else
			dec->double_write_mode = double_write_mode;

#ifdef OW_TRIPLE_WRITE
		if (get_config_int(pdata->config, "avs3_triple_write_mode",
			&config_val) == 0)
			dec->triple_write_mode = config_val;
		else
			dec->triple_write_mode = triple_write_mode;
#endif

		if (get_config_int(pdata->config, "parm_v4l_buffer_margin",
			&config_val) == 0)
			dec->dynamic_buf_margin = config_val;
		else
			dec->dynamic_buf_margin = 0;

		if (get_config_int(pdata->config, "sidebind_type",
				&config_val) == 0)
			dec->sidebind_type = config_val;

		if (get_config_int(pdata->config, "sidebind_channel_id",
				&config_val) == 0)
			dec->sidebind_channel_id = config_val;

		if (get_config_int(pdata->config, "HDRStaticInfo",
				&vf_dp.present_flag) == 0
				&& vf_dp.present_flag == 1) {
			get_config_int(pdata->config, "mG.x",
					&vf_dp.primaries[0][0]);
			get_config_int(pdata->config, "mG.y",
					&vf_dp.primaries[0][1]);
			get_config_int(pdata->config, "mB.x",
					&vf_dp.primaries[1][0]);
			get_config_int(pdata->config, "mB.y",
					&vf_dp.primaries[1][1]);
			get_config_int(pdata->config, "mR.x",
					&vf_dp.primaries[2][0]);
			get_config_int(pdata->config, "mR.y",
					&vf_dp.primaries[2][1]);
			get_config_int(pdata->config, "mW.x",
					&vf_dp.white_point[0]);
			get_config_int(pdata->config, "mW.y",
					&vf_dp.white_point[1]);
			get_config_int(pdata->config, "mMaxDL",
					&vf_dp.luminance[0]);
			get_config_int(pdata->config, "mMinDL",
					&vf_dp.luminance[1]);
			vf_dp.content_light_level.present_flag = 1;
			get_config_int(pdata->config, "mMaxCLL",
					&content_light_level.max_content);
			get_config_int(pdata->config, "mMaxFALL",
					&content_light_level.max_pic_average);
			vf_dp.content_light_level = content_light_level;
			dec->video_signal_type = (1 << 29)
					| (5 << 26)	/* unspecified */
					| (0 << 25)	/* limit */
					| (1 << 24)	/* color available */
					| (9 << 16)	/* 2020 */
					| (16 << 8)	/* 2084 */
					| (9 << 0);	/* 2020 */
		}
		dec->vf_dp = vf_dp;
	} else {
		/*dec->vavs3_amstream_dec_info.width = 0;
		dec->vavs3_amstream_dec_info.height = 0;
		dec->vavs3_amstream_dec_info.rate = 30;*/
		dec->double_write_mode = double_write_mode;
		dec->dynamic_buf_margin = dynamic_buf_num_margin;
	}
	video_signal_type = dec->video_signal_type;

	if (double_write_mode) {
		dec->double_write_mode = get_double_write_mode(dec);
	}

	if ((dec->double_write_mode & 0x10) == 0)
		dec->mmu_enable = 1;

#ifdef AVS3_10B_MMU_DW
	if (get_cpu_major_id() >= AM_MESON_CPU_MAJOR_ID_T5D) {
		dec->dw_mmu_enable =
			(get_double_write_mode(dec) & 0x20) ? 1 : 0;
	} else {
		dec->dw_mmu_enable = 0;
	}
#endif
#ifdef OW_TRIPLE_WRITE
	if (get_cpu_major_id() < AM_MESON_CPU_MAJOR_ID_T3X) {
		if ((dec->triple_write_mode) || (triple_write_mode) ||
			(dec->double_write_mode & 0x10000) || (double_write_mode & 0x10000)) {
			double_write_mode &= ~(1 <<16);
			dec->double_write_mode &= ~(1 <<16);
			triple_write_mode = 0;
			dec->triple_write_mode = 0;
			pr_err("%s warn: unsupport triple write or p010 mode, force disabled\n", __func__);
		}
	}
#endif

	if (amvdec_avs3_mmu_init(dec) < 0) {
		pr_err("avs3 alloc bmmu box failed!!\n");
		/* devm_kfree(&pdev->dev, (void *)dec); */
		vfree((void *)dec);
		return -1;
	}
	dec->cma_alloc_count = PAGE_ALIGN(work_buf_size) / PAGE_SIZE;
	ret = decoder_bmmu_box_alloc_buf_phy(dec->bmmu_box, WORK_SPACE_BUF_ID,
			dec->cma_alloc_count * PAGE_SIZE, DRIVER_NAME,
			&dec->cma_alloc_addr);
	if (ret < 0) {
		uninit_mmu_buffers(dec);
		/* devm_kfree(&pdev->dev, (void *)dec); */
		vfree((void *)dec);
		return ret;
	}
	if (!vdec_secure(pdata))
		codec_mm_memset(dec->cma_alloc_addr, 0, dec->cma_alloc_count * PAGE_SIZE);

	dec->buf_start = dec->cma_alloc_addr;
	dec->buf_size = work_buf_size;

	dec->init_flag = 0;
	dec->first_sc_checked = 0;
	dec->fatal_error = 0;
	dec->show_frame_num = 0;

	if (debug) {
		pr_info("===AVS3 decoder mem resource 0x%lx size 0x%x\n",
				dec->buf_start,
				dec->buf_size);
	}

	if (pdata->sys_info) {
		dec->vavs3_amstream_dec_info = *pdata->sys_info;
		dec->frame_width = dec->vavs3_amstream_dec_info.width;
		dec->frame_height = dec->vavs3_amstream_dec_info.height;
	} else {
		dec->vavs3_amstream_dec_info.width = 0;
		dec->vavs3_amstream_dec_info.height = 0;
		dec->vavs3_amstream_dec_info.rate = 30;
	}

	dec->endian = HEVC_CONFIG_LITTLE_ENDIAN;
	if (is_support_vdec_canvas())
		dec->endian = HEVC_CONFIG_BIG_ENDIAN;
	if (is_dw_p010(dec) || is_tw_p010(dec))
		dec->endian = HEVC_CONFIG_P010_LE;
	if (endian)
		dec->endian = endian;

	dec->cma_dev = pdata->cma_dev;
	if (vavs3_init(pdata) < 0) {
		pr_info("\namvdec_avs3 init failed.\n");
		avs3_local_uninit(dec);
		uninit_mmu_buffers(dec);
		/* devm_kfree(&pdev->dev, (void *)dec); */
		vfree((void *)dec);
		pdata->dec_status = NULL;
		return -ENODEV;
	}
	if (pdata->parallel_dec == 1) {
		//run only after pic_pool initialzied in vavs3_init()
		int i;
		for (i = 0; i < MAX_PB_SIZE; i++) {
			dec->avs3_dec.pic_pool[i].buf_cfg.y_canvas_index = -1;
			dec->avs3_dec.pic_pool[i].buf_cfg.uv_canvas_index = -1;
		}
	}
	spin_lock_init(&dec->buffer_lock);
	spin_lock_init(&dec->front_back_lock);
	mutex_init(&dec->slice_header_lock);
	vdec_set_prepare_level(pdata, start_decode_buf_level);
	hevc_source_changed(VFORMAT_AVS3,
			4096, 2048, 60);
	if (pdata->parallel_dec == 1) {
#ifdef NEW_FB_CODE
		if (dec->front_back_mode)
			vdec_core_request(pdata, CORE_MASK_HEVC | CORE_MASK_HEVC_BACK);
		else
#endif
		vdec_core_request(pdata, CORE_MASK_HEVC);
	} else {
		vdec_core_request(pdata, CORE_MASK_VDEC_1 | CORE_MASK_HEVC
				| CORE_MASK_COMBINE);
	}

	return 0;
}

static int ammvdec_avs3_remove(struct platform_device *pdev)
{
	struct AVS3Decoder_s *dec = (struct AVS3Decoder_s *)
		(((struct vdec_s *)(platform_get_drvdata(pdev)))->private);
	struct vdec_s *pdata = *(struct vdec_s **)pdev->dev.platform_data;
	struct avs3_decoder *avs3_dec = &dec->avs3_dec;
	struct avs3_frame_s *pic;
	int i;

	if (debug)
		pr_info("amvdec_avs3_remove\n");

	vmavs3_stop(dec);

	if (pdata->parallel_dec == 1) {
#ifdef NEW_FB_CODE
		if (dec->front_back_mode)
			vdec_core_release(hw_to_vdec(dec), CORE_MASK_HEVC | CORE_MASK_HEVC_BACK);
		else
#endif
		vdec_core_release(hw_to_vdec(dec), CORE_MASK_HEVC);
	} else
		vdec_core_release(hw_to_vdec(dec), CORE_MASK_HEVC);

	vdec_set_status(hw_to_vdec(dec), VDEC_STATUS_DISCONNECTED);
	if (pdata->parallel_dec == 1) {
		for (i = 0; i < MAX_PB_SIZE; i++) {
			pdata->free_canvas_ex(dec->avs3_dec.pic_pool[i].buf_cfg.y_canvas_index, pdata->id);
			pdata->free_canvas_ex(dec->avs3_dec.pic_pool[i].buf_cfg.uv_canvas_index, pdata->id);
		}
	}

	for (i = 0; i < avs3_dec->max_pb_size; i++) {
		pic = &avs3_dec->pic_pool[i].buf_cfg;
		if (pic->used < 0)
			continue;

		release_cuva_data(pic);
	}

#ifdef DEBUG_PTS
	pr_info("pts missed %ld, pts hit %ld, duration %d\n",
			dec->pts_missed, dec->pts_hit, dec->frame_dur);
#endif
	if (is_rdma_enable())
		decoder_dma_free_coherent(dec->rdma_mem_handle, RDMA_SIZE, dec->rdma_adr, dec->rdma_phy_adr);
	/* devm_kfree(&pdev->dev, (void *)dec); */
	vfree((void *)dec);
	return 0;
}

static struct platform_driver ammvdec_avs3_driver = {
	.probe = ammvdec_avs3_probe,
	.remove = ammvdec_avs3_remove,
#ifdef CONFIG_PM
	.suspend = amvdec_suspend,
	.resume = amvdec_resume,
#endif
	.driver = {
		.name = MULTI_DRIVER_NAME,
	}
};
#endif
//#ifndef FOR_S5
static struct mconfig avs3_configs[] = {
	MC_PU32("bit_depth_luma", &bit_depth_luma),
	MC_PU32("bit_depth_chroma", &bit_depth_chroma),
	MC_PU32("frame_width", &frame_width),
	MC_PU32("frame_height", &frame_height),
	MC_PU32("debug", &debug),
	MC_PU32("radr", &radr),
	MC_PU32("rval", &rval),
	MC_PU32("pop_shorts", &pop_shorts),
	MC_PU32("dbg_cmd", &dbg_cmd),
	MC_PU32("dbg_skip_decode_index", &dbg_skip_decode_index),
	MC_PU32("endian", &endian),
	MC_PU32("step", &step),
	MC_PU32("udebug_flag", &udebug_flag),
	MC_PU32("decode_pic_begin", &decode_pic_begin),
	MC_PU32("slice_parse_begin", &slice_parse_begin),
	MC_PU32("i_only_flag", &i_only_flag),
	MC_PU32("error_handle_policy", &error_handle_policy),
	MC_PU32("buf_alloc_width", &buf_alloc_width),
	MC_PU32("buf_alloc_height", &buf_alloc_height),
	MC_PU32("buf_alloc_depth", &buf_alloc_depth),
	MC_PU32("buf_alloc_size", &buf_alloc_size),
	MC_PU32("buffer_mode", &buffer_mode),
	MC_PU32("buffer_mode_dbg", &buffer_mode_dbg),
	MC_PU32("max_buf_num", &max_buf_num),
	MC_PU32("dynamic_buf_num_margin", &dynamic_buf_num_margin),
	MC_PU32("mem_map_mode", &mem_map_mode),
	MC_PU32("double_write_mode", &double_write_mode),
	MC_PU32("enable_mem_saving", &enable_mem_saving),
	MC_PU32("force_w_h", &force_w_h),
	MC_PU32("force_fps", &force_fps),
	MC_PU32("max_decoding_time", &max_decoding_time),
	MC_PU32("on_no_keyframe_skiped", &on_no_keyframe_skiped),
	MC_PU32("start_decode_buf_level", &start_decode_buf_level),
	MC_PU32("decode_timeout_val", &decode_timeout_val),
};
static struct mconfig_node avs3_node;
//#endif
static int __init amvdec_avs3_driver_init_module(void)
{

#ifdef AVS3_10B_MMU
	struct BuffInfo_s *p_buf_info;
	unsigned int bufspec_index = 0;
	if (force_bufspec) {
		bufspec_index = force_bufspec & 0xf;
		pr_info("force buffer spec %d\n", force_bufspec & 0xf);
	} else {
		if (vdec_is_support_4k()) {
			bufspec_index = 2;	/* 8k */
		} else
			bufspec_index = 0;/* 1080p */
	}
	p_buf_info = &amvavs3_workbuff_spec[bufspec_index];
	pr_info("%s bufspec_index = %d\n", __func__, bufspec_index);
	init_buff_spec(NULL, p_buf_info);
	work_buf_size =
		(p_buf_info->end_adr - p_buf_info->start_adr
			+ 0xffff) & (~0xffff);

#endif
	pr_debug("amvdec_avs3 module init\n");

#ifdef ERROR_HANDLE_DEBUG
	dbg_nal_skip_flag = 0;
	dbg_nal_skip_count = 0;
#endif
	udebug_flag = 0;
	decode_pic_begin = 0;
	slice_parse_begin = 0;
	step = 0;
	buf_alloc_size = 0;
	if (platform_driver_register(&ammvdec_avs3_driver))
		pr_err("failed to register ammvdec_avs3 driver\n");

	if (platform_driver_register(&amvdec_avs3_driver)) {
		pr_err("failed to register amvdec_avs3 driver\n");
		return -ENODEV;
	}
	if ((get_cpu_major_id() < AM_MESON_CPU_MAJOR_ID_G12A) ||
		(get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_T5D)) {
		amvdec_avs3_profile.name = "avs3_unsupport";
	} else if (get_cpu_major_id() < AM_MESON_CPU_MAJOR_ID_SM1) {
		if (vdec_is_support_4k())
			amvdec_avs3_profile.profile =
				"4k, 10bit, dwrite, compressed";
		else
			amvdec_avs3_profile.profile =
				"10bit, dwrite, compressed";
	} else {
		/* cpu id larger than sm1 support 8k */
		amvdec_avs3_profile.profile =
				"8k, 10bit, dwrite, compressed";
	}

	vcodec_profile_register(&amvdec_avs3_profile);
	amvdec_avs3_profile_mult = amvdec_avs3_profile;
	amvdec_avs3_profile_mult.name = "mavs3";
	vcodec_profile_register(&amvdec_avs3_profile_mult);

	INIT_REG_NODE_CONFIGS("media.decoder", &avs3_node,
		"avs3", avs3_configs, CONFIG_FOR_RW);
	vcodec_feature_register(VFORMAT_AVS3, 0);
	return 0;
}

static void __exit amvdec_avs3_driver_remove_module(void)
{
	pr_debug("amvdec_avs3 module remove.\n");
	platform_driver_unregister(&ammvdec_avs3_driver);
	platform_driver_unregister(&amvdec_avs3_driver);
}

/****************************************/

module_param(bit_depth_luma, uint, 0664);
MODULE_PARM_DESC(bit_depth_luma, "\n amvdec_avs3 bit_depth_luma\n");

module_param(bit_depth_chroma, uint, 0664);
MODULE_PARM_DESC(bit_depth_chroma, "\n amvdec_avs3 bit_depth_chroma\n");

module_param(frame_width, uint, 0664);
MODULE_PARM_DESC(frame_width, "\n amvdec_avs3 frame_width\n");

module_param(frame_height, uint, 0664);
MODULE_PARM_DESC(frame_height, "\n amvdec_avs3 frame_height\n");

module_param(debug, uint, 0664);
MODULE_PARM_DESC(debug, "\n amvdec_avs3 debug\n");

module_param(debug_again, uint, 0664);
MODULE_PARM_DESC(debug_again, "\n amvdec_avs3 debug_again\n");

module_param(radr, uint, 0664);
MODULE_PARM_DESC(radr, "\nradr\n");

module_param(rval, uint, 0664);
MODULE_PARM_DESC(rval, "\nrval\n");

module_param(pop_shorts, uint, 0664);
MODULE_PARM_DESC(pop_shorts, "\nrval\n");

module_param(dbg_cmd, uint, 0664);
MODULE_PARM_DESC(dbg_cmd, "\ndbg_cmd\n");

module_param(dbg_skip_decode_index, uint, 0664);
MODULE_PARM_DESC(dbg_skip_decode_index, "\ndbg_skip_decode_index\n");

module_param(endian, uint, 0664);
MODULE_PARM_DESC(endian, "\nrval\n");

module_param(step, uint, 0664);
MODULE_PARM_DESC(step, "\n amvdec_avs3 step\n");

module_param(decode_pic_begin, uint, 0664);
MODULE_PARM_DESC(decode_pic_begin, "\n amvdec_avs3 decode_pic_begin\n");

module_param(slice_parse_begin, uint, 0664);
MODULE_PARM_DESC(slice_parse_begin, "\n amvdec_avs3 slice_parse_begin\n");

module_param(i_only_flag, uint, 0664);
MODULE_PARM_DESC(i_only_flag, "\n amvdec_avs3 i_only_flag\n");

module_param(error_handle_policy, uint, 0664);
MODULE_PARM_DESC(error_handle_policy, "\n amvdec_avs3 error_handle_policy\n");

module_param(re_search_seq_threshold, uint, 0664);
MODULE_PARM_DESC(re_search_seq_threshold, "\n amvdec_avs3 re_search_seq_threshold\n");

module_param(buf_alloc_width, uint, 0664);
MODULE_PARM_DESC(buf_alloc_width, "\n buf_alloc_width\n");

module_param(buf_alloc_height, uint, 0664);
MODULE_PARM_DESC(buf_alloc_height, "\n buf_alloc_height\n");

module_param(buf_alloc_depth, uint, 0664);
MODULE_PARM_DESC(buf_alloc_depth, "\n buf_alloc_depth\n");

module_param(buf_alloc_size, uint, 0664);
MODULE_PARM_DESC(buf_alloc_size, "\n buf_alloc_size\n");

module_param(over_decoder_shiftbytes, uint, 0664);
MODULE_PARM_DESC(over_decoder_shiftbytes, "\n over_decoder_shiftbytes\n");

module_param(buffer_mode, uint, 0664);
MODULE_PARM_DESC(buffer_mode, "\n buffer_mode\n");

module_param(buffer_mode_dbg, uint, 0664);
MODULE_PARM_DESC(buffer_mode_dbg, "\n buffer_mode_dbg\n");
/*USE_BUF_BLOCK*/
module_param(max_buf_num, uint, 0664);
MODULE_PARM_DESC(max_buf_num, "\n max_buf_num\n");

module_param(dynamic_buf_num_margin, uint, 0664);
MODULE_PARM_DESC(dynamic_buf_num_margin, "\n dynamic_buf_num_margin\n");

#ifdef CONSTRAIN_MAX_BUF_NUM
module_param(run_ready_max_vf_only_num, uint, 0664);
MODULE_PARM_DESC(run_ready_max_vf_only_num, "\n run_ready_max_vf_only_num\n");

module_param(run_ready_display_q_num, uint, 0664);
MODULE_PARM_DESC(run_ready_display_q_num, "\n run_ready_display_q_num\n");

module_param(run_ready_max_buf_num, uint, 0664);
MODULE_PARM_DESC(run_ready_max_buf_num, "\n run_ready_max_buf_num\n");
#endif

module_param(mv_buf_margin, uint, 0664);
MODULE_PARM_DESC(mv_buf_margin, "\n mv_buf_margin\n");

module_param(run_ready_min_buf_num, uint, 0664);
MODULE_PARM_DESC(run_ready_min_buf_num, "\n run_ready_min_buf_num\n");

/**/

module_param(mem_map_mode, uint, 0664);
MODULE_PARM_DESC(mem_map_mode, "\n mem_map_mode\n");

module_param(double_write_mode, uint, 0664);
MODULE_PARM_DESC(double_write_mode, "\n double_write_mode\n");

#ifdef OW_TRIPLE_WRITE
module_param(triple_write_mode, uint, 0664);
MODULE_PARM_DESC(triple_write_mode, "\n triple_write_mode\n");
#endif

module_param(enable_mem_saving, uint, 0664);
MODULE_PARM_DESC(enable_mem_saving, "\n enable_mem_saving\n");

module_param(force_w_h, uint, 0664);
MODULE_PARM_DESC(force_w_h, "\n force_w_h\n");

module_param(force_fps, uint, 0664);
MODULE_PARM_DESC(force_fps, "\n force_fps\n");

module_param(max_decoding_time, uint, 0664);
MODULE_PARM_DESC(max_decoding_time, "\n max_decoding_time\n");

module_param(on_no_keyframe_skiped, uint, 0664);
MODULE_PARM_DESC(on_no_keyframe_skiped, "\n on_no_keyframe_skiped\n");

module_param(start_decode_buf_level, int, 0664);
MODULE_PARM_DESC(start_decode_buf_level,
		"\n avs3 start_decode_buf_level\n");

module_param(decode_timeout_val, uint, 0664);
MODULE_PARM_DESC(decode_timeout_val,
	"\n avs3 decode_timeout_val\n");

module_param_array(decode_frame_count, uint,
	&max_decode_instance_num, 0664);

module_param_array(display_frame_count, uint,
	&max_decode_instance_num, 0664);

module_param_array(max_process_time, uint,
	&max_decode_instance_num, 0664);

module_param_array(run_count, uint,
	&max_decode_instance_num, 0664);

module_param_array(input_empty, uint,
	&max_decode_instance_num, 0664);

module_param_array(not_run_ready, uint,
	&max_decode_instance_num, 0664);

module_param(video_signal_type, uint, 0664);
MODULE_PARM_DESC(video_signal_type, "\n amvdec_avs3 video_signal_type\n");

module_param(force_video_signal_type, uint, 0664);
MODULE_PARM_DESC(force_video_signal_type, "\n amvdec_avs3 force_video_signal_type\n");

module_param(enable_force_video_signal_type, uint, 0664);
MODULE_PARM_DESC(enable_force_video_signal_type, "\n amvdec_avs3 enable_force_video_signal_type\n");

module_param(force_bufspec, uint, 0664);
MODULE_PARM_DESC(force_bufspec, "\n amvdec_avs3 force_bufspec\n");

module_param(fbdebug_flag, uint, 0664);
MODULE_PARM_DESC(fbdebug_flag, "\n amvdec_avs3 fbdebug_flag\n");

module_param(udebug_flag, uint, 0664);
MODULE_PARM_DESC(udebug_flag, "\n amvdec_avs3 udebug_flag\n");

module_param(udebug_pause_pos, uint, 0664);
MODULE_PARM_DESC(udebug_pause_pos, "\n udebug_pause_pos\n");

module_param(udebug_pause_val, uint, 0664);
MODULE_PARM_DESC(udebug_pause_val, "\n udebug_pause_val\n");

module_param(udebug_pause_decode_idx, uint, 0664);
MODULE_PARM_DESC(udebug_pause_decode_idx, "\n udebug_pause_decode_idx\n");

module_param(dump_phy_adr, uint, 0664);
MODULE_PARM_DESC(dump_phy_adr, "\n dump_phy_adr\n");

module_param(dump_phy_size, uint, 0664);
MODULE_PARM_DESC(dump_phy_size, "\n dump_phy_size\n");

#ifdef DEBUG_CMD
module_param(debug_cmd_wait_type, uint, 0664);
MODULE_PARM_DESC(debug_cmd_wait_type, "\n debug_cmd_wait_type\n");

module_param(debug_cmd_wait_count, uint, 0664);
MODULE_PARM_DESC(debug_cmd_wait_count, "\n debug_cmd_wait_count\n");
#endif

module_param(pre_decode_buf_level, int, 0664);
MODULE_PARM_DESC(pre_decode_buf_level,
		"\n amvdec_avs3 pre_decode_buf_level\n");

module_param(again_threshold, uint, 0664);
MODULE_PARM_DESC(again_threshold, "\n again_threshold\n");

module_param(force_disp_pic_index, int, 0664);
MODULE_PARM_DESC(force_disp_pic_index,
	"\n amvdec_avs3 force_disp_pic_index\n");

module_param(without_display_mode, uint, 0664);
MODULE_PARM_DESC(without_display_mode, "\n without_display_mode\n");

module_param(mv_buf_dynamic_alloc, uint, 0664);
MODULE_PARM_DESC(mv_buf_dynamic_alloc, "\n mv_buf_dynamic_alloc\n");

#ifdef NEW_FB_CODE
module_param(front_back_mode, uint, 0664);
MODULE_PARM_DESC(front_back_mode, "\n amvdec_avs3 front_back_mode\n");

module_param(fb_ifbuf_num, uint, 0664);
MODULE_PARM_DESC(fb_ifbuf_num, "\n amvdec_avs3 fb_ifbuf_num\n");

module_param(decode_timeout_val_back, uint, 0664);
MODULE_PARM_DESC(decode_timeout_val_back,
	"\n amvdec_avs3 decode_timeout_val_back\n");

module_param(fast_timer_check_count, uint, 0664);
MODULE_PARM_DESC(fast_timer_check_count,
	"\n amvdec_avs3 fast_timer_check_count\n");

module_param_array(max_process_time_back, uint,
	&max_decode_instance_num, 0664);
#endif

module_param(trace_debug_mode, uint, 0664);
MODULE_PARM_DESC(trace_debug_mode, "\n  trace_debug_mode\n");

module_param(paral_alloc_buffer_mode, uint, 0664);
MODULE_PARM_DESC(paral_alloc_buffer_mode, "\n  paral_alloc_buffer_mode\n");

#ifdef NEW_FB_CODE

module_param(efficiency_mode, uint, 0664);
MODULE_PARM_DESC(efficiency_mode, "\n  efficiency_mode\n");

#endif

module_init(amvdec_avs3_driver_init_module);
module_exit(amvdec_avs3_driver_remove_module);

MODULE_DESCRIPTION("AMLOGIC avs3 Video Decoder Driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tim Yao <tim.yao@amlogic.com>");
