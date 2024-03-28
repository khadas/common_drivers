// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * drivers/amlogic/media/video_sink/video.c
 *
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
 */

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/amlogic/major.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/ctype.h>
#include <linux/compat.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/amlogic/media/vfm/vframe_provider.h>
#include <linux/amlogic/media/vfm/vframe_receiver.h>
#include <linux/amlogic/media/utils/amstream.h>
#include <linux/amlogic/media/utils/am_com.h>
#ifdef CONFIG_AMLOGIC_VOUT
#include <linux/amlogic/media/vout/vout_notify.h>
#endif
#include <linux/amlogic/media/video_sink/video_signal_notify.h>
#ifdef CONFIG_AMLOGIC_MEDIA_VIN
#include <linux/amlogic/media/frame_provider/tvin/tvin.h>
#endif
#include <linux/amlogic/media/vfm/vfm_ext.h>
#include <linux/sched.h>
#include <linux/sched/clock.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/clk.h>
#include <linux/debugfs.h>
#include <linux/amlogic/media/canvas/canvas.h>
#include <linux/amlogic/media/canvas/canvas_mgr.h>
#include <linux/dma-mapping.h>
#include <linux/dma-map-ops.h>
#include <linux/sched.h>
#include <linux/amlogic/media/video_sink/video_keeper.h>
#include "video_priv.h"
#include "video_reg.h"
#define KERNEL_ATRACE_TAG KERNEL_ATRACE_TAG_VIDEO
#include <trace/events/meson_atrace.h>

#ifdef CONFIG_AMLOGIC_MEDIA_FRAME_SYNC
#include <linux/amlogic/media/frame_sync/ptsserv.h>
#include <linux/amlogic/media/frame_sync/timestamp.h>
#include <linux/amlogic/media/frame_sync/tsync.h>
#endif
#if defined(CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_VECM)
#include "vpp_pq.h"
#include <linux/amlogic/media/amvecm/amvecm.h>
#endif
#include <linux/amlogic/media/utils/vdec_reg.h>

#ifdef CONFIG_PM
#include <linux/delay.h>
#include <linux/pm.h>
#endif

#include <linux/amlogic/media/registers/register.h>
#include <linux/uaccess.h>
#include <linux/amlogic/media/utils/amports_config.h>
#ifdef CONFIG_AMLOGIC_VPU
#include <linux/amlogic/media/vpu/vpu.h>
#endif
#include "videolog.h"
#include "video_reg.h"
#ifdef CONFIG_AMLOGIC_MEDIA_VIDEOCAPTURE
#include "amvideocap_priv.h"
#endif
#ifdef CONFIG_AM_VIDEO_LOG
#define AMLOG
#endif
#include <linux/amlogic/media/utils/amlog.h>
MODULE_AMLOG(LOG_LEVEL_ERROR, 0, LOG_DEFAULT_LEVEL_DESC, LOG_MASK_DESC);

#include <linux/amlogic/media/video_sink/vpp.h>
#ifdef CONFIG_AMLOGIC_MEDIA_TVIN
#include "linux/amlogic/media/frame_provider/tvin/tvin_v4l2.h"
#endif
#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
#include "../common/rdma/rdma.h"
#endif
#include <linux/amlogic/media/video_sink/video.h>
#include <linux/amlogic/media/codec_mm/configs.h>
#include <linux/amlogic/media/codec_mm/codec_mm.h>

#include "../common/vfm/vfm.h"
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
#include <linux/amlogic/media/amdolbyvision/dolby_vision.h>
#endif
#ifdef CONFIG_AMLOGIC_MEDIA_DEINTERLACE
#include <linux/amlogic/media/di/di.h>
#endif
#ifdef CONFIG_AMLOGIC_LEGACY_EARLY_SUSPEND
#include <linux/amlogic/pm.h>
#endif
#ifdef CONFIG_AMLOGIC_MEDIA_SECURITY
#include <linux/amlogic/media/vpu_secure/vpu_secure.h>
#endif
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_PRIME_SL
#include <linux/amlogic/media/amprime_sl/prime_sl.h>
#endif
#include <linux/amlogic/media/video_processor/video_pp_common.h>
#include <linux/amlogic/media/video_processor/di_proc_buf_mgr.h>

#include <linux/math64.h>
#include "video_receiver.h"
#include "video_multi_vsync.h"
#include "video_hw_s5.h"
#include "video_hw.h"
#include "vpp_post_s5.h"
#include "video_common.h"

#include <linux/amlogic/gki_module.h>
#ifdef CONFIG_AMLOGIC_MEDIA_MSYNC
#include <uapi/amlogic/msync.h>
#endif
#include "vpp_pq.h"
#include "video_low_latency.h"
#include "video_func.h"

#define DEBUG_TMP 0
#define DRIVER_NAME "amvideo"
#define MODULE_NAME "amvideo"
#define DEVICE_NAME "amvideo"

#define RECEIVER_NAME "amvideo"
#define RECEIVERPIP_NAME "videopip"
#define RECEIVERPIP2_NAME "videopip2"

#define PARSE_MD_IN_ADVANCE 1

#ifdef CONFIG_AML_VSYNC_FIQ_ENABLE
#define FIQ_VSYNC
#endif

#ifdef CONFIG_AMLOGIC_VOUT
#define CONFIG_AM_VOUT
#endif

#ifdef FIQ_VSYNC
#define BRIDGE_IRQ INT_TIMER_C
#define BRIDGE_IRQ_SET() WRITE_CBUS_REG(ISA_TIMERC, 1)
#endif

#ifdef FIQ_VSYNC
static bridge_item_t vsync_fiq_bridge;
#endif

/* local var */
static s32 amvideo_poll_major;
static struct device *amvideo_dev;
static struct device *amvideo_poll_dev;
static const char video_dev_id[] = "amvideo-dev";
static struct amvideo_device_data_s amvideo_meson_dev;
static struct dentry *video_debugfs_root;
static struct video_save_s video_save;

static int video_vsync = -ENXIO;
static int video_vsync_viu2 = -ENXIO;
static int video_vsync_viu3 = -ENXIO;
static int video_pre_vsync = -ENXIO;
/* mosaic mode */
int mosaic_frame_done = -ENXIO;

static DEFINE_MUTEX(video_layer_mutex);
static DEFINE_MUTEX(video_module_mutex);
static DEFINE_MUTEX(video_inuse_mutex);
static DEFINE_SPINLOCK(hdmi_avsync_lock);

static unsigned long hist_buffer_addr;
static u32 hist_print_count;
static u8 probe_id;
static u32 probe_output;
static int receive_frame_count;
static int tvin_delay_mode;
static int debugflags;
static int output_fps;
static u32 cur_width;
static u32 cur_height;
static u32 layer_cap;

/* omx related */
#define OMX_PTS_DV_DEFAULT_UPPER 2500
#define OMX_PTS_DV_DEFAULT_LOWER -1600
#define OMX_MAX_COUNT_RESET_SYSTEMTIME 2
#define OMX_MAX_COUNT_RESET_SYSTEMTIME_BEGIN 10
#define OMX_CONTINUOUS_DROP_LEVEL 5
#define DEBUG_FLAG_FFPLAY	BIT(0)
#define DEBUG_FLAG_CALC_PTS_INC	BIT(1)
static u32 cur_omx_index;
static u32 omx_pts;
static u32 omx_pts_set_index;
static bool omx_run;
static u32 omx_version = 3;
static u32 omx_continusdrop_cnt;
static int omx_pts_interval_upper = 5500;
static int omx_pts_interval_lower = -5500;
static int omx_pts_dv_upper = OMX_PTS_DV_DEFAULT_UPPER;
static int omx_pts_dv_lower = OMX_PTS_DV_DEFAULT_LOWER;
static int omx_pts_set_from_hwc_count;
static int omx_pts_set_from_hwc_count_begin;
static bool omx_check_previous_session;
static int drop_frame_count;
static int omx_need_drop_frame_num;
static bool omx_drop_done;
static long long time_setomxpts;
static long long time_setomxpts_last;
static int omx_continuous_drop_count;
static bool omx_continuous_drop_flag;
static u32 cur_disp_omx_index;
/*----omx_info  bit0: keep_last_frame, bit1~31: unused----*/
static u32 omx_info = 0x1;
static DEFINE_MUTEX(omx_mutex);
/* pts related */
#define DURATION_GCD 750
#define M_PTS_SMOOTH_MAX 45000
#define M_PTS_SMOOTH_MIN 2250
#define M_PTS_SMOOTH_ADJUST 900

static bool video_start_post;
static bool videopeek;
static bool nopostvideostart;
static int hold_property_changed;
#define PTS_LOGGING
#define PTS_THROTTLE
/* #define PTS_TRACE_DEBUG */
/* #define PTS_TRACE_START */
#define AVSYNC_COUNT
#ifdef AVSYNC_COUNT
static bool av_discontinue;
static u32 avsync_count;
#endif

#ifdef PTS_TRACE_DEBUG
static int pts_trace_his[16];
static u32 pts_his[16];
static u32 scr_his[16];
static int pts_trace_his_rd;
#endif

#if defined(PTS_LOGGING)
#define PTS_32_PATTERN_DETECT_RANGE 10
#define PTS_22_PATTERN_DETECT_RANGE 10
#define PTS_41_PATTERN_DETECT_RANGE 2
#define PTS_32_PATTERN_DURATION 3750
#define PTS_22_PATTERN_DURATION 3000

enum video_refresh_pattern {
	PTS_32_PATTERN = 0,
	PTS_22_PATTERN,
	PTS_41_PATTERN,
	PTS_MAX_NUM_PATTERNS
};

static int pts_pattern[3] = {0, 0, 0};
static int pts_pattern_enter_cnt[3] = {0, 0, 0};
static int pts_pattern_exit_cnt[3] = {0, 0, 0};
static int pts_log_enable[3] = {0, 0, 0};
static int pre_pts_trace;
static int pts_escape_vsync = -1;

#define PTS_41_PATTERN_SINK_MAX 4
static int pts_41_pattern_sink[PTS_41_PATTERN_SINK_MAX];
static int pts_41_pattern_sink_index;
static int pts_pattern_detected = -1;
static bool pts_enforce_pulldown = true;
#endif
static u32 vsync_pts_inc;
static u32 vsync_pts_inc_upint;
static u32 vsync_pts_inc_adj;
static u32 vsync_pts_125;
static u32 vsync_pts_112;
static u32 vsync_pts_101;
static u32 vsync_pts_100;
static u32 vsync_freerun;
/* extend this value to support both slow and fast playback
 * 0,1: normal playback
 * [2,1000]: speed/vsync_slow_factor
 * >1000: speed*(vsync_slow_factor/1000000)
 */
static u32 vsync_slow_factor = 1;

/* pts alignment */
static bool vsync_pts_aligned;
static s32 vsync_pts_align;

/* dv related */
static bool dovi_drop_flag;
static int dovi_drop_frame_num;
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
/*dv instance id, used for tunnel mode*/
static int dv_inst = -1;
int dv_inst_pip;
#endif

#ifdef INTERLACE_FIELD_MATCH_PROCESS
#define FIELD_MATCH_THRESHOLD  10
static int field_matching_count;
#endif

/* other avsync, frame drop etc */
static u32 underflow;
static u32 next_peek_underflow;
static u32 frame_skip_check_cnt;
/*frame_detect_flag: 1 enable, 0 disable */
/*frame_detect_time: */
/*	How often "frame_detect_receive_count" and */
/*		"frame_detect_drop_count" are updated, suggested set 1(s) */
/*frame_detect_fps: Set fps based on the video file, */
/*					If the FPS is 60, set it to 60000. */
/*frame_detect_receive_count: */
/*	The number of frame that should be obtained during the test time. */
/*frame_detect_drop_count: */
/*	The number of frame lost during test time. */
static u32 frame_detect_flag;
static u32 frame_detect_time = 1;
static u32 frame_detect_fps = 60000;
static u32 frame_detect_receive_count;
static u32 frame_detect_drop_count;
/* #define SLOW_SYNC_REPEAT */
/* #define INTERLACE_FIELD_MATCH_PROCESS */
static bool disable_slow_sync;
struct video_frame_detect_s {
	u32 interrupt_count;
	u32 start_receive_count;
};

static struct video_frame_detect_s video_frame_detect;

#define ENABLE_UPDATE_HDR_FROM_USER 0
#if ENABLE_UPDATE_HDR_FROM_USER
static struct vframe_master_display_colour_s vf_hdr;
static bool has_hdr_info;
static DEFINE_SPINLOCK(omx_hdr_lock);
#endif

/* show first frame*/
static bool show_first_frame_nosync;
static bool show_first_picture;
/* video frame repeat count */
static u32 frame_repeat_count;
/* test screen*/
static u32 test_screen;
/* rgb screen*/
static u32 rgb_screen;
static u32 hold_video;

/* frame rate calculate */
static u32 last_frame_count;
static u32 frame_count;
static u32 first_frame_toggled;
static u32 last_frame_time;
#ifdef CONFIG_AMLOGIC_MEDIA_FRAME_SYNC
static u64 last_frame_duration;
#endif

/* video freerun mode */
#define FREERUN_NONE    0	/* no freerun mode */
#define FREERUN_NODUR   1	/* freerun without duration */
#define FREERUN_DUR     2	/* freerun with duration */
static u32 freerun_mode;
static u32 slowsync_repeat_enable;

/* hdmin related */
static int hdmin_delay_start;
static int hdmin_delay_start_time;
static int hdmin_delay_duration;
static int hdmin_delay_min_ms;
static int hdmin_delay_max_ms = 128;
static int hdmin_dv_flag;
static int hdmin_delay_done = true;
static int hdmin_need_drop_count;

static int last_required_total_delay;
static int hdmi_vframe_count;
static bool hdmi_delay_first_check;
static u8 hdmi_delay_normal_check; /* 0xff: always check, n: check n times */
static u32 hdmin_delay_count_debug;
/* 0xff: always check, n: check n times after first check */
static u8 enable_hdmi_delay_normal_check = 1;

#define HDMI_DELAY_FIRST_CHECK_COUNT 60
#define HDMI_DELAY_NORMAL_CHECK_COUNT 300
#define HDMI_VIDEO_MIN_DELAY 3

/*bit0~1 for vd1, bit2~3 for vd2*/
static u32 force_skip_cnt;
/* wait queue for poll */
static wait_queue_head_t amvideo_trick_wait;
static u32 vpts_ref;
static u32 video_frame_repeat_count;
static u32 smooth_sync_enable;
static u32 hdmi_in_onvideo;

#ifdef VIDEO_PTS_CHASE
static int vpts_chase;
static int av_sync_flag;
static int vpts_chase_counter;
static int vpts_chase_pts_diff;
#endif

static int step_enable;
static int step_flag;
#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
static int vsync_rdma_line_max;
#endif
static u32 vpts_remainder;
static int enable_video_discontinue_report = 1;
#ifdef CONFIG_AMLOGIC_MEDIA_VIDEOCAPTURE
static struct amvideocap_req *capture_frame_req;
#endif

/*********************************************************/
/* #define DUR2PTS(x) ((x) - ((x) >> 4)) */
static inline int DUR2PTS(int x)
{
	int var = x;

	var -= var >> 4;
	return var;
}

#define DUR2PTS_RM(x) ((x) & 0xf)
#define PTS2DUR(x) (((x) << 4) / 15)

/*vdin output 29.97 59.94, vinfo is 60hz, but after vlock locked, actual vout is 59.94;
 *23.974: vlock not work, both vinfo and actual vout are 60;
 *119.88: vinfo is 120hz, but after vlock locked, actual vout is 119.88
 *this case, vinfo is not equal to actual vout frame rate, so duration need to be adjusted
 */
int vf_get_pts(struct vframe_s *vf)
{
	int duration = vf->duration;

	if (vf->source_type == VFRAME_SOURCE_TYPE_HDMI ||
		vf->source_type == VFRAME_SOURCE_TYPE_CVBS ||
		vf->source_type == VFRAME_SOURCE_TYPE_TUNER) {
		if (duration == 4004)
			duration = 4000;
		else if (duration == 3203)
			duration = 3200;
		else if (duration == 1601)
			duration = 1600;
		else if (duration == 801)
			duration = 800;
	}
	return DUR2PTS(duration);
}

int vf_get_pts_rm(struct vframe_s *vf)
{
	int duration = vf->duration;

	if (vf->source_type == VFRAME_SOURCE_TYPE_HDMI ||
		vf->source_type == VFRAME_SOURCE_TYPE_CVBS ||
		vf->source_type == VFRAME_SOURCE_TYPE_TUNER) {
		if (duration == 4004)
			duration = 4000;
		else if (duration == 3203)
			duration = 3200;
		else if (duration == 1601)
			duration = 1600;
		else if (duration == 801)
			duration = 800;
	}
	return DUR2PTS_RM(duration);
}

/* frame rate calculate */
u32 toggle_count;
u32 timer_count;
u32 vsync_count;

/* extern var for video_func.h */
/* default value 20 30 */
s32 black_threshold_width = 20;
s32 black_threshold_height = 48;
struct vframe_s hist_test_vf;
bool hist_test_flag;
/* wait queue for poll */
wait_queue_head_t amvideo_prop_change_wait;
char old_vmode[32];
char new_vmode[32];
int get_count_pip[MAX_VD_LAYER];
int get_di_count;
int put_di_count;
int di_release_count;
int display_frame_count;
u32 vpp_hold_setting_cnt;
bool to_notify_trick_wait;
int vsync_enter_line_max;
int vsync_exit_line_max;
u32 performance_debug;
bool over_field;
u32 over_field_case1_cnt;
u32 over_field_case2_cnt;
u32 video_notify_flag;
int tvin_source_type;
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
bool dvel_changed;
u32 dvel_size;
#endif
DEFINE_SPINLOCK(lock);
atomic_t cur_over_field_state = ATOMIC_INIT(OVER_FIELD_NORMAL);
u32 config_vsync_num;
ulong config_timeinfo;
bool go_exit;

/* ai_pq */
int ai_pq_disable;
int ai_pq_debug;
int ai_pq_value = -1;
int ai_pq_policy = 1;
struct ai_scenes_pq vpp_scenes[AI_SCENES_MAX];
struct nn_value_t nn_scenes_value[AI_PQ_TOP];
struct video_frame_aiface_s ai_face_value;

/* pts related */
u32 vsync_pts_inc_scale;
u32 vsync_pts_inc_scale_base = 1;
#if defined(PTS_LOGGING) || defined(PTS_TRACE_DEBUG)
int pts_trace;
#endif
int vframe_walk_delay;

/* display canvas */
#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
struct vframe_s *cur_rdma_buf[MAX_VD_LAYERS];
struct vframe_s *dispbuf_to_put[MAX_VD_LAYERS][DISPBUF_TO_PUT_MAX];
s8 dispbuf_to_put_num[MAX_VD_LAYERS];
#endif
/* 0: amvideo, 1: pip */
/* rdma buf + dispbuf + to_put_buf */
struct vframe_s *recycle_buf[MAX_VD_LAYERS][1 + DISPBUF_TO_PUT_MAX];
s32 recycle_cnt[MAX_VD_LAYERS];
/* config */
u32 blackout[MAX_VD_LAYERS];
u32 force_blackout;
u32 pip_frame_count[MAX_VD_LAYERS];
u32 pip_loop;
u32 pip2_loop;
struct vframe_s *cur_dispbuf[MAX_VD_LAYERS];
struct vframe_s *cur_dispbuf2;
//static struct vframe_s *cur_dispbuf3;

/* extern var for video_priv.h */
struct vpp_frame_par_s *cur_frame_par[MAX_VD_LAYERS];
struct vframe_s vf_local[MAX_VD_LAYERS], vf_local2, vf_local_ext[MAX_VD_LAYERS];
bool need_disable_vd[MAX_VD_LAYER];
struct video_recv_s *gvideo_recv[3] = {NULL, NULL, NULL};
struct video_recv_s *gvideo_recv_vpp[2] = {NULL, NULL};
/*seek values on.video_define.h*/
int debug_flag;

int vdec_out_size_threshold_8k = 7;
int vpp_in_size_threshold_8k = 7;
int vdec_out_size_threshold_4k = 9;
int vpp_in_size_threshold_4k = 9;

/* for vd1 vsync 2to1 */
bool vsync_count_start;
u32 new_frame_cnt;
MODULE_PARM_DESC(new_frame_cnt, "\n new_frame_cnt\n");
module_param(new_frame_cnt, uint, 0664);

MODULE_PARM_DESC(vsync_count_start, "\n vsync_count_start\n");
module_param(vsync_count_start, bool, 0664);

u32 new_frame_count;
u32 vd1_vd2_mux_dts;
u32 osd_vpp_misc;
u32 osd_vpp_misc_mask;
bool update_osd_vpp_misc;
bool update_osd_vpp1_bld_ctrl;
bool update_osd_vpp2_bld_ctrl;
u32 osd_vpp1_bld_ctrl;
u32 osd_vpp1_bld_ctrl_mask;
u32 osd_vpp2_bld_ctrl;
u32 osd_vpp2_bld_ctrl_mask;
u32 osd_vpp_bld_ctrl_update_mask;
u32 osd2_blend_path_sel;
u32 osd_preblend_en;
/* omx related */
u32 omx_cur_session = 0xffffffff;
bool omx_secret_mode;
bool bypass_pps = true;
/* vpp_crc */
u32 vpp_crc_en;
int vpp_crc_result;
/* viu2 vpp_crc */
static u32 vpp_crc_viu2_en;
/* source fmt string */
const char *src_fmt_str[11] = {
	"SDR", "HDR10", "HDR10+", "HDR Prime", "HLG",
	"Dolby Vison", "Dolby Vison Low latency", "MVC",
	"CUVA_HDR", "CUVA_HLG", "SDR_2020"
};

atomic_t primary_src_fmt =
	ATOMIC_INIT(VFRAME_SIGNAL_FMT_INVALID);
atomic_t cur_primary_src_fmt =
	ATOMIC_INIT(VFRAME_SIGNAL_FMT_INVALID);
atomic_t gafbc_request = ATOMIC_INIT(0);
u32 video_prop_status;
u32 video_info_change_status;
u32 force_switch_vf_mode;
/* video_inuse */
u32 video_inuse;
bool reverse;
u32  video_mirror;
bool vd1_vd2_mux;
bool aisr_en;
bool video_suspend;
u32 video_suspend_cycle;
int log_out;
u64 vsync_cnt[VPP_MAX] = {0, 0, 0};

#ifdef CONFIG_PM
struct video_pm_state_s {
	int event;
	u32 vpp_misc;
	int mem_pd_vd1;
	int mem_pd_vd2;
	int mem_pd_di_post;
};
#endif

/* vout */
#ifdef CONFIG_AMLOGIC_VOUT
const struct vinfo_s *vinfo;
#endif

/* is fffb or seeking*/
static u32 video_seek_flag;
/* trickmode i frame*/
u32 trickmode_i;
EXPORT_SYMBOL(trickmode_i);
/* trickmode ff/fb */
u32 trickmode_fffb;
atomic_t trickmode_framedone = ATOMIC_INIT(0);
atomic_t video_prop_change = ATOMIC_INIT(0);
atomic_t status_changed = ATOMIC_INIT(0);
atomic_t fmm_changed = ATOMIC_INIT(0);
atomic_t video_unreg_flag = ATOMIC_INIT(0);
atomic_t vt_unreg_flag = ATOMIC_INIT(0);
atomic_t vt_disable_video_done = ATOMIC_INIT(0);
atomic_t video_inirq_flag = ATOMIC_INIT(0);
atomic_t video_prevsync_inirq_flag = ATOMIC_INIT(0);
atomic_t video_pause_flag = ATOMIC_INIT(0);
atomic_t video_proc_lock = ATOMIC_INIT(0);
atomic_t video_recv_cnt = ATOMIC_INIT(0);
int trickmode_duration;
int trickmode_duration_count;
u32 trickmode_vpts;
/* last_playback_filename */
char file_name[512];

/* 3d related */
static unsigned int framepacking_width = 1920;
static unsigned int framepacking_height = 2205;
static int pause_one_3d_fl_frame;
u32 framepacking_support;
u32 g_framepacking_support;
unsigned int framepacking_blank = 45;
unsigned int process_3d_type;
#ifdef TV_3D_FUNCTION_OPEN
/* toggle_3d_fa_frame is for checking the vpts_expire  in 2 vsnyc */
int toggle_3d_fa_frame = 1;
/*the pause_one_3d_fl_frame is for close*/
/*the A/B register switch in every sync at pause mode. */

MODULE_PARM_DESC(pause_one_3d_fl_frame, "\n pause_one_3d_fl_frame\n");
module_param(pause_one_3d_fl_frame, int, 0664);

/*debug info control for skip & repeate vframe case*/
static unsigned int video_dbg_vf;
MODULE_PARM_DESC(video_dbg_vf, "\n video_dbg_vf\n");
module_param(video_dbg_vf, uint, 0664);

static int vd_cnt = MAX_VD_LAYER;
unsigned int video_get_vf_cnt[MAX_VD_LAYER];
module_param_array(video_get_vf_cnt, uint, &vd_cnt, 0664);
MODULE_PARM_DESC(video_get_vf_cnt, "\n video_get_vf_cnt\n");

unsigned int video_drop_vf_cnt[MAX_VD_LAYER];
module_param_array(video_drop_vf_cnt, uint, &vd_cnt, 0664);
MODULE_PARM_DESC(video_drop_vf_cnt, "\n video_drop_vf_cnt\n");

static unsigned int disable_dv_drop;
MODULE_PARM_DESC(disable_dv_drop, "\n disable_dv_drop\n");
module_param(disable_dv_drop, uint, 0664);

static u32 vdin_frame_skip_cnt;
MODULE_PARM_DESC(vdin_frame_skip_cnt, "\n vdin_frame_skip_cnt\n");
module_param(vdin_frame_skip_cnt, uint, 0664);

static u32 vdin_err_crc_cnt;
MODULE_PARM_DESC(vdin_err_crc_cnt, "\n vdin_err_crc_cnt\n");
module_param(vdin_err_crc_cnt, uint, 0664);
#define ERR_CRC_COUNT 6

#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
static bool dv_mute_vpp_flag;
#endif

static unsigned int video_3d_format;
#ifdef CONFIG_AMLOGIC_MEDIA_TVIN
static unsigned int mvc_flag;
#endif
unsigned int force_3d_scaler = 3;
int last_mode_3d;
#endif

static void update_process_hdmi_avsync_flag(bool flag);
static void hdmi_in_delay_maxmin_reset(void);
static int is_interlaced(struct vinfo_s *vinfo);

#if defined(CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_VECM)
static unsigned int det_stb_cnt = 30;
static unsigned int det_unstb_cnt = 20;
static unsigned int tolrnc_cnt = 6;
static unsigned int timer_filter_en;
static unsigned int aipq_set_policy;
static unsigned int color_th = 100;

u32 get_stb_cnt(void)
{
	return det_stb_cnt;
}

u32 get_unstb_cnt(void)
{
	return det_unstb_cnt;
}

u32 get_tolrnc_cnt(void)
{
	return tolrnc_cnt;
}

u32 get_timer_filter_en(void)
{
	return timer_filter_en;
}

u32 get_aipq_set_policy(void)
{
	if (video_is_meson_s5_cpu())
		aipq_set_policy = 2;
	if (video_is_meson_t5m_cpu())
		aipq_set_policy = 3;

	return aipq_set_policy;
}

u32 get_color_th(void)
{
	return color_th;
}
#endif

static u64 func_div(u64 number, u32 divid)
{
	u64 tmp = number;

	do_div(tmp, divid);
	return tmp;
}

int get_video_debug_flags(void)
{
	return debug_flag;
}

bool vf_source_from_vdin(struct vframe_s *vf)
{
	if (vf->source_type == VFRAME_SOURCE_TYPE_HDMI ||
		vf->source_type == VFRAME_SOURCE_TYPE_CVBS ||
		vf->source_type == VFRAME_SOURCE_TYPE_TUNER)
		return true;
	else
		return false;
}

/* amvideo vf related api */
void pre_process_for_3d(struct vframe_s *vf)
{
	int frame_width, frame_height;

	if (vf->type & VIDTYPE_COMPRESS) {
		frame_width = vf->compWidth;
		frame_height = vf->compHeight;
	} else {
		frame_width = vf->width;
		frame_height = vf->height;
	}

#if defined(TV_3D_FUNCTION_OPEN) && defined(CONFIG_AMLOGIC_MEDIA_TVIN)
	/*can be moved to h264mvc.c */
	if ((vf->type & VIDTYPE_MVC) &&
	    (process_3d_type & MODE_3D_ENABLE) && vf->trans_fmt) {
		vf->type = VIDTYPE_PROGRESSIVE | VIDTYPE_VIU_FIELD;
		process_3d_type |= MODE_3D_MVC;
		mvc_flag = 1;
	} else {
		process_3d_type &= (~MODE_3D_MVC);
		mvc_flag = 0;
	}
	if (((process_3d_type & MODE_FORCE_3D_TO_2D_LR) ||
	     (process_3d_type & MODE_FORCE_3D_LR) ||
	     (process_3d_type & MODE_FORCE_3D_FA_LR)) &&
	    (!(vf->type & VIDTYPE_MVC)) &&
	    vf->trans_fmt != TVIN_TFMT_3D_FP) {
		vf->trans_fmt = TVIN_TFMT_3D_DET_LR;
		vf->left_eye.start_x = 0;
		vf->left_eye.start_y = 0;
		vf->left_eye.width = frame_width / 2;
		vf->left_eye.height = frame_height;
		vf->right_eye.start_x = frame_width / 2;
		vf->right_eye.start_y = 0;
		vf->right_eye.width = frame_width / 2;
		vf->right_eye.height = frame_height;
	}
	if (((process_3d_type & MODE_FORCE_3D_TO_2D_TB) ||
	     (process_3d_type & MODE_FORCE_3D_TB) ||
	     (process_3d_type & MODE_FORCE_3D_FA_TB)) &&
	    (!(vf->type & VIDTYPE_MVC)) &&
	    vf->trans_fmt != TVIN_TFMT_3D_FP) {
		vf->trans_fmt = TVIN_TFMT_3D_TB;
		vf->left_eye.start_x = 0;
		vf->left_eye.start_y = 0;
		vf->left_eye.width = frame_width;
		vf->left_eye.height = frame_height / 2;
		vf->right_eye.start_x = 0;
		vf->right_eye.start_y = frame_height / 2;
		vf->right_eye.width = frame_width;
		vf->right_eye.height = frame_height / 2;
	}
#endif
}

inline struct vframe_s *amvideo_vf_peek(void)
{
	struct vframe_s *vf = vf_peek(RECEIVER_NAME);

	if (hist_test_flag) {
		if (cur_dispbuf[0] != &hist_test_vf)
			vf = &hist_test_vf;
		else
			vf = NULL;
	}

	if (vf && vf->disp_pts && vf->disp_pts_us64) {
		vf->pts = vf->disp_pts;
		vf->pts_us64 = vf->disp_pts_us64;
		vf->disp_pts = 0;
		vf->disp_pts_us64 = 0;
	}
	#ifdef used_fence
	if (vf && vf->fence) {
		int ret = 0;
		/*
		 * the ret of fence status.
		 * 0: has not been signaled.
		 * 1: signaled without err.
		 * other: fence err.
		 */
		ret = fence_get_status(vf->fence);
		if (ret < 0) {
			vf = vf_get(RECEIVER_NAME);
			if (vf)
				vf_put(vf, RECEIVER_NAME);
		} else if (ret == 0) {
			vf = NULL;
		}
	}
	#endif
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
	/*tunnel mode, add dv_inst to vf*/
	if (vf && dv_inst >= 0)
		vf->src_fmt.dv_id = dv_inst;
#endif
	return vf;
}

inline struct vframe_s *amvideo_vf_get(void)
{
	struct vframe_s *vf = NULL;

	if (hist_test_flag) {
		if (cur_dispbuf[0] != &hist_test_vf)
			vf = &hist_test_vf;
		return vf;
	}

	vf = vf_get(RECEIVER_NAME);
	if (vf) {
		get_count_pip[0]++;
		vpp_trace_vframe("amvideo_vf_peek",
			(void *)vf, vf->type, vf->flag, 0, vsync_cnt[VPP0]);
		if (vf->type & VIDTYPE_V4L_EOS) {
			vf_put(vf, RECEIVER_NAME);
			return NULL;
		}
		if (IS_DI_POSTWRTIE(vf->type))
			get_di_count++;
		if (vf->disp_pts && vf->disp_pts_us64) {
			vf->pts = vf->disp_pts;
			vf->pts_us64 = vf->disp_pts_us64;
			vf->disp_pts = 0;
			vf->disp_pts_us64 = 0;
		}
		video_notify_flag |= VIDEO_NOTIFY_PROVIDER_GET;
		atomic_set(&vf->use_cnt, 1);
		/*always to 1,for first get from vfm provider */
		if ((vf->type & VIDTYPE_MVC) && (framepacking_support) &&
		    (framepacking_width) && (framepacking_height)) {
			vf->width = framepacking_width;
			vf->height = framepacking_height;
		}
		pre_process_for_3d(vf);
		receive_frame_count++;
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
		/*tunnel mode, add dv_inst to vf*/
		if (dv_inst >= 0)
			vf->src_fmt.dv_id = dv_inst;
#endif
	}
	return vf;
}

static int amvideo_vf_get_states(struct vframe_states *states)
{
	int ret;
	unsigned long flags;

	spin_lock_irqsave(&lock, flags);
	ret = vf_get_states_by_name(RECEIVER_NAME, states);
	spin_unlock_irqrestore(&lock, flags);
	return ret;
}

inline int amvideo_vf_put(struct vframe_s *vf)
{
	struct vframe_provider_s *vfp = vf_get_provider(RECEIVER_NAME);

	if (vf == &hist_test_vf)
		return 0;

	if (!vfp || !vf) {
		if (!vf)
			return 0;
		return -EINVAL;
	}
	if (atomic_dec_and_test(&vf->use_cnt)) {
		vpp_trace_vframe("video_vf_put",
			(void *)vf, vf->type, vf->flag, 0, vsync_cnt[VPP0]);
		if (vf_put(vf, RECEIVER_NAME) < 0)
			return -EFAULT;
		if (IS_DI_POSTWRTIE(vf->type))
			put_di_count++;
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
		if (is_amdv_enable())
			amdv_vf_put(vf);
#endif
		video_notify_flag |= VIDEO_NOTIFY_PROVIDER_PUT;
	} else {
		return -EINVAL;
	}
	return 0;
}

static bool has_receive_dummy_vframe(void)
{
	struct vframe_s *vf;
#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
	int i;
#endif
	u8 layer_id = 0;

	vf = amvideo_vf_peek();

	if (vf && vf->flag & VFRAME_FLAG_EMPTY_FRAME_V4L) {
		/* get dummy vf. */
		vf = amvideo_vf_get();

#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA /* recycle vframe. */
		for (i = 0; i < DISPBUF_TO_PUT_MAX; i++) {
			if (dispbuf_to_put[layer_id][i]) {
				if (!amvideo_vf_put(dispbuf_to_put[layer_id][i])) {
					dispbuf_to_put[layer_id][i] = NULL;
					dispbuf_to_put_num[layer_id]--;
				}
			}
		}
#endif
		/* recycle the last vframe. */
		if (cur_dispbuf[0] && cur_dispbuf[0] != &vf_local[0]) {
			if (!amvideo_vf_put(cur_dispbuf[0]))
				cur_dispbuf[0] = NULL;
		}

		/*pr_info("put dummy vframe.\n");*/
		if (amvideo_vf_put(vf) < 0)
			check_dispbuf(layer_id, vf, true);
		return true;
	}

	return false;
}

static void video_vf_unreg_provider(void)
{
	ulong flags;
	bool layer1_used = false;
	bool layer2_used = false;
	bool layer3_used = false;

	struct vframe_s *el_vf = NULL;
	int keeped = 0, ret = 0;
#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
	int i;
#endif

	clear_vsync_2to1_info();
	new_frame_count = 0;
	first_frame_toggled = 0;
	videopeek = 0;
	nopostvideostart = false;
	hold_property_changed = 0;
	video_save.save_vf_en = false;
	video_save.save_vf = NULL;
	video_save.toggle_vf = NULL;

	atomic_inc(&video_unreg_flag);
	while (atomic_read(&video_inirq_flag) > 0)
		schedule();
	if (cur_dev->pre_vsync_enable)
		while (atomic_read(&video_prevsync_inirq_flag) > 0)
			schedule();
	memset(&video_frame_detect, 0,
	       sizeof(struct video_frame_detect_s));
	frame_detect_drop_count = 0;
	frame_detect_receive_count = 0;
	spin_lock_irqsave(&lock, flags);
	ret = update_video_recycle_buffer(0);
	if (ret == -EAGAIN) {
	/* The currently displayed vf is not added to the queue
	 * that needs to be released. You need to release the vf
	 * data in the release queue before adding the currently
	 * displayed vf to the release queue.
	 */
		release_di_buffer(0);
		update_video_recycle_buffer(0);
	}
#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
	dispbuf_to_put_num[0] = 0;
	for (i = 0; i < DISPBUF_TO_PUT_MAX; i++)
		dispbuf_to_put[0][i] = NULL;
	cur_rdma_buf[0] = NULL;
#endif
	if (cur_dispbuf[0]) {
		if (cur_dispbuf[0]->vf_ext &&
		    IS_DI_POSTWRTIE(cur_dispbuf[0]->type)) {
			struct vframe_s *tmp;

			if (cur_dispbuf[0]->uvm_vf)
				tmp = cur_dispbuf[0]->uvm_vf;
			else
				tmp = (struct vframe_s *)cur_dispbuf[0]->vf_ext;
			memcpy(&tmp->pic_mode, &cur_dispbuf[0]->pic_mode,
				sizeof(struct vframe_pic_mode_s));
			vf_local_ext[0] = *tmp;
			vf_local[0] = *cur_dispbuf[0];
			vf_local[0].vf_ext = (void *)&vf_local_ext[0];
			vf_local[0].uvm_vf = NULL;
			vf_local_ext[0].ratio_control = vf_local[0].ratio_control;
		} else if (cur_dispbuf[0]->vf_ext &&
			is_pre_link_source(cur_dispbuf[0])) {
			u32 tmp_rc;
			struct vframe_s *tmp;

			if (cur_dispbuf[0]->uvm_vf)
				tmp = cur_dispbuf[0]->uvm_vf;
			else
				tmp = (struct vframe_s *)cur_dispbuf[0]->vf_ext;
			if (debug_flag & DEBUG_FLAG_PRELINK)
				pr_info("video_unreg: prelink: cur_dispbuf:%px vf_ext:%px uvm_vf:%px flag:%x\n",
					cur_dispbuf[0], cur_dispbuf[0]->vf_ext,
					cur_dispbuf[0]->uvm_vf, cur_dispbuf[0]->flag);
			tmp_rc = cur_dispbuf[0]->ratio_control;
			memcpy(&tmp->pic_mode, &cur_dispbuf[0]->pic_mode,
				sizeof(struct vframe_pic_mode_s));
			vf_local[0] = *tmp;
			vf_local[0].ratio_control = tmp_rc;
			vf_local[0].vf_ext = NULL;
			vf_local[0].uvm_vf = NULL;
		} else if (IS_DI_POST(cur_dispbuf[0]->type) &&
			(cur_dispbuf[0]->vf_ext || cur_dispbuf[0]->uvm_vf)) {
			u32 tmp_rc;
			struct vframe_s *tmp;

			if (cur_dispbuf[0]->uvm_vf)
				tmp = cur_dispbuf[0]->uvm_vf;
			else
				tmp = (struct vframe_s *)cur_dispbuf[0]->vf_ext;
			if (debug_flag & DEBUG_FLAG_PRELINK)
				pr_info("video_unreg: pre/post link: cur_dispbuf:%px vf_ext:%px uvm_vf:%px flag:%x\n",
					cur_dispbuf[0], cur_dispbuf[0]->vf_ext,
					cur_dispbuf[0]->uvm_vf, cur_dispbuf[0]->flag);
			tmp_rc = cur_dispbuf[0]->ratio_control;
			memcpy(&tmp->pic_mode, &cur_dispbuf[0]->pic_mode,
				sizeof(struct vframe_pic_mode_s));
			vf_local[0] = *tmp;
			vf_local[0].ratio_control = tmp_rc;
			vf_local[0].vf_ext = NULL;
			vf_local[0].uvm_vf = NULL;
		} else {
			vf_local[0] = *cur_dispbuf[0];
			vf_local[0].vf_ext = NULL;
			vf_local[0].uvm_vf = NULL;
		}
		cur_dispbuf[0] = &vf_local[0];
		cur_dispbuf[0]->video_angle = 0;
	}

	if (cur_dispbuf2)
		need_disable_vd[1] = true;
	if (is_amdv_enable()) {
		if (cur_dispbuf2 == &vf_local2) {
			cur_dispbuf2 = NULL;
		} else if (cur_dispbuf2) {
			vf_local2 = *cur_dispbuf2;
			el_vf = &vf_local2;
		}
		cur_dispbuf2 = NULL;
	}
	if (trickmode_fffb) {
		atomic_set(&trickmode_framedone, 0);
		to_notify_trick_wait = false;
	}

	vsync_pts_100 = 0;
	vsync_pts_112 = 0;
	vsync_pts_125 = 0;
	vsync_freerun = 0;
	vsync_pts_align = 0;
	vsync_pts_aligned = false;

	if (pip_loop) {
#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
		dispbuf_to_put_num[1] = 0;
		for (i = 0; i < DISPBUF_TO_PUT_MAX; i++)
			dispbuf_to_put[1][i] = NULL;
		cur_rdma_buf[1] = NULL;
#endif
		if (cur_dispbuf[1]) {
			vf_local[1] = *cur_dispbuf[1];
			cur_dispbuf[1] = &vf_local[1];
			cur_dispbuf[1]->video_angle = 0;
		}
		pip_frame_count[1] = 0;
	}
	if (pip2_loop) {
#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
		for (i = 0; i < DISPBUF_TO_PUT_MAX; i++)
			dispbuf_to_put[2][i] = NULL;
		cur_rdma_buf[2] = NULL;
#endif
		if (cur_dispbuf[2]) {
			vf_local[2] = *cur_dispbuf[2];
			cur_dispbuf[2] = &vf_local[2];
			cur_dispbuf[2]->video_angle = 0;
		}
		pip_frame_count[2] = 0;
	}

	spin_unlock_irqrestore(&lock, flags);

	if (vd_layer[0].dispbuf_mapping
		== &cur_dispbuf[0])
		layer1_used = true;
	if (vd_layer[1].dispbuf_mapping
		== &cur_dispbuf[0])
		layer2_used = true;
	if (vd_layer[2].dispbuf_mapping
		== &cur_dispbuf[0])
		layer3_used = true;

	if (layer1_used || !vd_layer[0].dispbuf_mapping)
		atomic_set(&primary_src_fmt, VFRAME_SIGNAL_FMT_INVALID);

	if (pip_loop) {
		vd_layer[1].disable_video =
			VIDEO_DISABLE_FORNEXT;
		safe_switch_videolayer(1, false, false);
	}
	if (pip2_loop) {
		vd_layer[2].disable_video =
			VIDEO_DISABLE_FORNEXT;
		safe_switch_videolayer(2, false, false);
	}

	if (!layer1_used && !layer2_used && !layer3_used)
		cur_dispbuf[0] = NULL;

	if (blackout[0] | force_blackout) {
		if (layer1_used)
			safe_switch_videolayer
				(0, false, false);
		if (layer2_used)
			safe_switch_videolayer
				(1, false, false);
		if (layer3_used)
			safe_switch_videolayer
				(2, false, false);
		try_free_keep_vdx(0, 1);
	}
	if (cur_dispbuf[0])
		keeped = vf_keep_current
			(cur_dispbuf[0], el_vf);

	pr_info("%s: vd1 used: %s, vd2 used: %s, vd3 used: %s, keep_ret:%d, black_out:%d, cur_dispbuf:%p\n",
		__func__,
		layer1_used ? "true" : "false",
		layer2_used ? "true" : "false",
		layer3_used ? "true" : "false",
		keeped, blackout[0] | force_blackout,
		cur_dispbuf[0]);

	if (hdmi_in_onvideo == 0 && (video_start_post)) {
		tsync_avevent(VIDEO_STOP, 0);
		video_start_post = false;
	}

	if (keeped <= 0) {/*keep failed.*/
		if (keeped < 0)
			pr_info("keep frame failed, disable video now.\n");
		else
			pr_info("keep frame skip, disable video again.\n");
		if (layer1_used)
			safe_switch_videolayer
				(0, false, false);
		if (layer2_used)
			safe_switch_videolayer
				(1, false, false);
		if (layer3_used)
			safe_switch_videolayer
				(2, false, false);
		try_free_keep_vdx(0, 1);
	}

	atomic_dec(&video_unreg_flag);
	pr_info("VD1 AFBC 0x%x.\n", is_afbc_enabled(0));
	enable_video_discontinue_report = 1;
	show_first_picture = false;
	show_first_frame_nosync = false;

	time_setomxpts = 0;
	time_setomxpts_last = 0;
	vdin_err_crc_cnt = 0;

#ifdef PTS_LOGGING
	{
		int pattern;
		/* Print what we have right now*/
		if (pts_pattern_detected >= PTS_32_PATTERN &&
		    pts_pattern_detected < PTS_MAX_NUM_PATTERNS) {
			pr_info("pattern detected = %d, pts_enter_pattern_cnt =%d, pts_exit_pattern_cnt =%d",
				pts_pattern_detected,
				pts_pattern_enter_cnt[pts_pattern_detected],
				pts_pattern_exit_cnt[pts_pattern_detected]);
		}
		/* Reset all metrics now*/
		for (pattern = 0; pattern < PTS_MAX_NUM_PATTERNS; pattern++) {
			pts_pattern[pattern] = 0;
			pts_pattern_exit_cnt[pattern] = 0;
			pts_pattern_enter_cnt[pattern] = 0;
		}
		/* Reset 4:1 data*/
		memset(&pts_41_pattern_sink[0], 0, PTS_41_PATTERN_SINK_MAX);
		pts_pattern_detected = -1;
		pre_pts_trace = 0;
		pts_escape_vsync = 0;
	}
#endif
}

static void video_vf_light_unreg_provider(int need_keep_frame)
{
	ulong flags;
	int ret = 0;
#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
	int i;
#endif

	atomic_inc(&video_unreg_flag);
	while (atomic_read(&video_inirq_flag) > 0)
		schedule();
	if (cur_dev->pre_vsync_enable)
		while (atomic_read(&video_prevsync_inirq_flag) > 0)
			schedule();
	spin_lock_irqsave(&lock, flags);
	ret = update_video_recycle_buffer(0);
	if (ret == -EAGAIN) {
	/* The currently displayed vf is not added to the queue
	 * that needs to be released. You need to release the vf
	 * data in the release queue before adding the currently
	 * displayed vf to the release queue.
	 */
		release_di_buffer(0);
		update_video_recycle_buffer(0);
	}
#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
	dispbuf_to_put_num[0] = 0;
	for (i = 0; i < DISPBUF_TO_PUT_MAX; i++)
		dispbuf_to_put[0][i] = NULL;
	cur_rdma_buf[0] = NULL;
#endif

	if (cur_dispbuf[0]) {
		if (cur_dispbuf[0]->vf_ext &&
		    IS_DI_POSTWRTIE(cur_dispbuf[0]->type)) {
			struct vframe_s *tmp;

			if (cur_dispbuf[0]->uvm_vf)
				tmp = cur_dispbuf[0]->uvm_vf;
			else
				tmp = (struct vframe_s *)cur_dispbuf[0]->vf_ext;
			memcpy(&tmp->pic_mode, &cur_dispbuf[0]->pic_mode,
				sizeof(struct vframe_pic_mode_s));
			vf_local_ext[0] = *tmp;
			vf_local[0] = *cur_dispbuf[0];
			vf_local[0].vf_ext = (void *)&vf_local_ext[0];
			vf_local[0].uvm_vf = NULL;
			vf_local_ext[0].ratio_control = vf_local[0].ratio_control;
		} else if (cur_dispbuf[0]->vf_ext &&
			is_pre_link_source(cur_dispbuf[0])) {
			u32 tmp_rc;
			struct vframe_s *tmp;

			if (cur_dispbuf[0]->uvm_vf)
				tmp = cur_dispbuf[0]->uvm_vf;
			else
				tmp = (struct vframe_s *)cur_dispbuf[0]->vf_ext;
			if (debug_flag & DEBUG_FLAG_PRELINK)
				pr_info("%s: prelink: cur_dispbuf:%px vf_ext:%px uvm_vf:%px flag:%x\n",
					__func__,
					cur_dispbuf[0], cur_dispbuf[0]->vf_ext,
					cur_dispbuf[0]->uvm_vf, cur_dispbuf[0]->flag);
			tmp_rc = cur_dispbuf[0]->ratio_control;
			memcpy(&tmp->pic_mode, &cur_dispbuf[0]->pic_mode,
				sizeof(struct vframe_pic_mode_s));
			vf_local[0] = *tmp;
			vf_local[0].ratio_control = tmp_rc;
			vf_local[0].vf_ext = NULL;
			vf_local[0].uvm_vf = NULL;
		} else if (IS_DI_POST(cur_dispbuf[0]->type) &&
			(cur_dispbuf[0]->vf_ext || cur_dispbuf[0]->uvm_vf)) {
			u32 tmp_rc;
			struct vframe_s *tmp;

			if (cur_dispbuf[0]->uvm_vf)
				tmp = cur_dispbuf[0]->uvm_vf;
			else
				tmp = (struct vframe_s *)cur_dispbuf[0]->vf_ext;
			if (debug_flag & DEBUG_FLAG_PRELINK)
				pr_info("%s: pre/post link: cur_dispbuf:%px vf_ext:%px uvm_vf:%px flag:%x\n",
					__func__,
					cur_dispbuf[0], cur_dispbuf[0]->vf_ext,
					cur_dispbuf[0]->uvm_vf, cur_dispbuf[0]->flag);
			tmp_rc = cur_dispbuf[0]->ratio_control;
			memcpy(&tmp->pic_mode, &cur_dispbuf[0]->pic_mode,
				sizeof(struct vframe_pic_mode_s));
			vf_local[0] = *tmp;
			vf_local[0].ratio_control = tmp_rc;
			vf_local[0].vf_ext = NULL;
			vf_local[0].uvm_vf = NULL;
		} else {
			vf_local[0] = *cur_dispbuf[0];
			vf_local[0].vf_ext = NULL;
			vf_local[0].uvm_vf = NULL;
		}
		cur_dispbuf[0] = &vf_local[0];
	}
	spin_unlock_irqrestore(&lock, flags);

	if (need_keep_frame) {
		/* keep the last toggled frame*/
		if (cur_dispbuf[0]) {
			unsigned int result;

			result = vf_keep_current
				(cur_dispbuf[0], NULL);
			if (result == 0)
				pr_info("%s: keep cur_disbuf failed\n",
					__func__);
		}
	}
	atomic_dec(&video_unreg_flag);
}

static int video_receiver_event_fun(int type, void *data, void *private_data)
{
	if (type == VFRAME_EVENT_PROVIDER_UNREG) {
		video_vf_unreg_provider();
		drop_frame_count = 0;
		receive_frame_count = 0;
		display_frame_count = 0;
#ifdef CONFIG_AMLOGIC_MEDIA_FRAME_SYNC
#ifdef AVSYNC_COUNT
		avsync_count = 0;
		timestamp_avsync_counter_set(avsync_count);
#endif
#endif
		nn_scenes_value[0].maxprob = 0;
		//init_hdr_info();
		mutex_lock(&omx_mutex);
		omx_continuous_drop_count = 0;
		omx_continuous_drop_flag = false;
		cur_disp_omx_index = 0;
		dovi_drop_flag = false;
		dovi_drop_frame_num = 0;
		mutex_unlock(&omx_mutex);
		atomic_dec(&video_recv_cnt);
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
		if (is_amdv_enable()) {
			dv_vf_light_unreg_provider();
			if (dv_inst >= 0) {/*tunnel mode*/
				dv_inst_unmap(dv_inst);
				dv_inst = -1;
			}
		}
#endif
		update_process_hdmi_avsync_flag(false);
	} else if (type == VFRAME_EVENT_PROVIDER_RESET) {
		video_vf_light_unreg_provider(1);
		nn_scenes_value[0].maxprob = 0;
	} else if (type == VFRAME_EVENT_PROVIDER_LIGHT_UNREG) {
		video_vf_light_unreg_provider(0);
		nn_scenes_value[0].maxprob = 0;
	} else if (type == VFRAME_EVENT_PROVIDER_REG) {
		atomic_inc(&video_recv_cnt);
		video_drop_vf_cnt[0] = 0;
		enable_video_discontinue_report = 1;
		drop_frame_count = 0;
		receive_frame_count = 0;
		display_frame_count = 0;
#ifdef CONFIG_AMLOGIC_MEDIA_FRAME_SYNC
#ifdef AVSYNC_COUNT
		avsync_count = 0;
		timestamp_avsync_counter_set(avsync_count);
#endif
#endif
		mutex_lock(&omx_mutex);
		omx_run = false;
		omx_pts_set_from_hwc_count = 0;
		omx_pts_set_from_hwc_count_begin = 0;
		omx_check_previous_session = true;
		omx_need_drop_frame_num = 0;
		omx_drop_done = false;
		omx_pts_set_index = 0;
		omx_continusdrop_cnt = 0;
		omx_continuous_drop_count = 0;
		omx_continuous_drop_flag = false;
		cur_disp_omx_index = 0;
		dovi_drop_flag = false;
		dovi_drop_frame_num = 0;
		mutex_unlock(&omx_mutex);
		hdmi_in_delay_maxmin_reset();
		//init_hdr_info();
/*notify di 3d mode is frame*/
/*alternative mode,passing two buffer in one frame */
#ifdef CONFIG_AMLOGIC_MEDIA_TVIN
		if ((process_3d_type & MODE_3D_FA) &&
		    cur_dispbuf[0] &&
		    !cur_dispbuf[0]->trans_fmt)
			vf_notify_receiver_by_name
			("deinterlace",
			VFRAME_EVENT_PROVIDER_SET_3D_VFRAME_INTERLEAVE,
			(void *)1);
#endif
		video_vf_light_unreg_provider(0);
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
		if (is_amdv_enable()) {
			dv_vf_light_reg_provider();
			if (is_tunnel_mode(RECEIVER_NAME)) {
				dv_inst_map(&dv_inst);
				pr_info("video receiver map dv_inst %d\n", dv_inst);
			}
		}
#endif
		update_process_hdmi_avsync_flag(true);
	} else if (type == VFRAME_EVENT_PROVIDER_FORCE_BLACKOUT) {
		force_blackout = 1;
		if (debug_flag & DEBUG_FLAG_BASIC_INFO) {
			pr_info("%s VFRAME_EVENT_PROVIDER_FORCE_BLACKOUT\n",
				__func__);
		}
	} else if (type == VFRAME_EVENT_PROVIDER_FR_HINT) {
#ifdef CONFIG_AM_VOUT
		if (data && video_seek_flag == 0) {
			set_vframe_rate_hint((unsigned long)data);
			omx_pts_dv_upper = DUR2PTS((unsigned long)data) * 3 / 2;
			omx_pts_dv_lower = 0 - DUR2PTS((unsigned long)data);
		}
#endif
	} else if (type == VFRAME_EVENT_PROVIDER_FR_END_HINT) {
#ifdef CONFIG_AM_VOUT
		if (video_seek_flag == 0) {
			set_vframe_rate_hint(0);
			omx_pts_dv_upper = OMX_PTS_DV_DEFAULT_UPPER;
			omx_pts_dv_lower = OMX_PTS_DV_DEFAULT_LOWER;
		}
#endif
	} else if (type == VFRAME_EVENT_PROVIDER_QUREY_DISPLAY_INFO) {
		get_display_info(data);
	} else if (type == VFRAME_EVENT_PROVIDER_PROPERTY_CHANGED) {
		vd_layer[0].property_changed = true;
		vd_layer[1].property_changed = true;
	}
	return 0;
}

static const struct vframe_receiver_op_s video_vf_receiver = {
	.event_cb = video_receiver_event_fun
};

static const struct vframe_receiver_op_s videopip_vf_receiver = {
	.event_cb = pip_receiver_event_fun
};

static const struct vframe_receiver_op_s videopip2_vf_receiver = {
	.event_cb = pip2_receiver_event_fun
};

static struct vframe_receiver_s video_vf_recv;
static struct vframe_receiver_s videopip_vf_recv;
static struct vframe_receiver_s videopip2_vf_recv;

/* other amvideo related api */
static void dump_vframe_status(const char *name)
{
	int ret = -1;
	struct vframe_states states;
	struct vframe_provider_s *vfp;

	memset(&states, 0, sizeof(struct vframe_states));

	vfp = vf_get_provider_by_name(name);
	if (vfp && vfp->ops && vfp->ops->vf_states)
		ret = vfp->ops->vf_states(&states, vfp->op_arg);

	if (ret == 0) {
		ret += pr_info("%s_pool_size=%d\n",
			name, states.vf_pool_size);
		ret += pr_info("%s buf_free_num=%d\n",
			name, states.buf_free_num);
		ret += pr_info("%s buf_avail_num=%d\n",
			name, states.buf_avail_num);
	} else {
		ret += pr_info("%s vframe no states\n", name);
	}
}

static void dump_vdin_reg(void)
{
	unsigned int reg001 = 0, reg002 = 0;

	if (cur_dev->display_module != S5_DISPLAY_MODULE) {
		reg001 = READ_VCBUS_REG(0x1204);
		reg002 = READ_VCBUS_REG(0x1205);
	}
	pr_info("VDIN_LCNT_STATUS:0x%x,VDIN_COM_STATUS0:0x%x\n",
		reg001, reg002);
}

#ifdef CONFIG_AMLOGIC_MEDIA_VIDEOCAPTURE
static int ext_get_cur_video_frame(struct vframe_s **vf, int *canvas_index)
{
	if (!cur_dispbuf[0])
		return -1;
	atomic_inc(&cur_dispbuf[0]->use_cnt);
	if (cur_dev->display_module != S5_DISPLAY_MODULE)
		*canvas_index = READ_VCBUS_REG(vd_layer[0].vd_mif_reg.vd_if0_canvas0);
	*vf = cur_dispbuf;
	return 0;
}

int ext_put_video_frame(struct vframe_s *vf)
{
	u8 layer_id = 0;

	if (vf == &vf_local[0])
		return 0;
	if (amvideo_vf_put(vf) < 0)
		check_dispbuf(layer_id, vf, true);
	return 0;
}

int ext_register_end_frame_callback(struct amvideocap_req *req)
{
	mutex_lock(&video_module_mutex);
	capture_frame_req = req;
	mutex_unlock(&video_module_mutex);
	return 0;
}

int ext_frame_capture_poll(int endflags)
{
	mutex_lock(&video_module_mutex);
	if (capture_frame_req && capture_frame_req->callback) {
		struct vframe_s *vf;
		int index;
		int ret;
		struct amvideocap_req *req = capture_frame_req;

		ret = ext_get_cur_video_frame(&vf, &index);
		if (!ret) {
			req->callback(req->data, vf, index);
			capture_frame_req = NULL;
		}
	}
	mutex_unlock(&video_module_mutex);
	return 0;
}
#endif

#ifdef TV_3D_FUNCTION_OPEN
/* judge the out mode is 240:LBRBLRBR  or 120:LRLRLR */
static void judge_3d_fa_out_mode(void)
{
	if ((process_3d_type & MODE_3D_OUT_FA_MASK) &&
	    pause_one_3d_fl_frame == 2) {
		toggle_3d_fa_frame = OUT_FA_B_FRAME;
	} else if ((process_3d_type & MODE_3D_OUT_FA_MASK) &&
		 pause_one_3d_fl_frame == 1) {
		toggle_3d_fa_frame = OUT_FA_A_FRAME;
	} else if ((process_3d_type & MODE_3D_OUT_FA_MASK) &&
		 pause_one_3d_fl_frame == 0) {
		/* toggle_3d_fa_frame  determine*/
		/*the out frame is L or R or blank */
		if ((process_3d_type & MODE_3D_OUT_FA_L_FIRST)) {
			if ((vsync_count % 2) == 0)
				toggle_3d_fa_frame = OUT_FA_A_FRAME;
			else
				toggle_3d_fa_frame = OUT_FA_B_FRAME;
		} else if ((process_3d_type & MODE_3D_OUT_FA_R_FIRST)) {
			if ((vsync_count % 2) == 0)
				toggle_3d_fa_frame = OUT_FA_B_FRAME;
			else
				toggle_3d_fa_frame = OUT_FA_A_FRAME;
		} else if ((process_3d_type & MODE_3D_OUT_FA_LB_FIRST)) {
			if ((vsync_count % 4) == 0)
				toggle_3d_fa_frame = OUT_FA_A_FRAME;
			else if ((vsync_count % 4) == 2)
				toggle_3d_fa_frame = OUT_FA_B_FRAME;
			else
				toggle_3d_fa_frame = OUT_FA_BANK_FRAME;
		} else if ((process_3d_type & MODE_3D_OUT_FA_RB_FIRST)) {
			if ((vsync_count % 4) == 0)
				toggle_3d_fa_frame = OUT_FA_B_FRAME;
			else if ((vsync_count % 4) == 2)
				toggle_3d_fa_frame = OUT_FA_A_FRAME;
			else
				toggle_3d_fa_frame = OUT_FA_BANK_FRAME;
		}
	} else {
		toggle_3d_fa_frame = OUT_FA_A_FRAME;
	}
}
#endif

#ifdef PTS_LOGGING
static void log_vsync_video_pattern(int pattern)
{
	int factor1 = 0, factor2 = 0, pattern_range = 0;

	if (pattern >= PTS_MAX_NUM_PATTERNS)
		return;

	if (pattern == PTS_32_PATTERN) {
		factor1 = 3;
		factor2 = 2;
		pattern_range =  PTS_32_PATTERN_DETECT_RANGE;
	} else if (pattern == PTS_22_PATTERN) {
		factor1 = 2;
		factor2 = 2;
		pattern_range =  PTS_22_PATTERN_DETECT_RANGE;
	} else if (pattern == PTS_41_PATTERN) {
		/* update 2111 mode detection */
		if (pts_trace == 2) {
			if (pts_41_pattern_sink[1] == 1 &&
			    pts_41_pattern_sink[2] == 1 &&
			    pts_41_pattern_sink[3] == 1 &&
			    pts_pattern[PTS_41_PATTERN] <
			     PTS_41_PATTERN_DETECT_RANGE) {
				pts_pattern[PTS_41_PATTERN]++;
				if (pts_pattern[PTS_41_PATTERN] ==
					PTS_41_PATTERN_DETECT_RANGE) {
					pts_pattern_enter_cnt[PTS_41_PATTERN]++;
					pts_pattern_detected = pattern;
					if (pts_log_enable[PTS_41_PATTERN])
						pr_info("video 4:1 mode detected\n");
				}
			}
			pts_41_pattern_sink[0] = 2;
			pts_41_pattern_sink_index = 1;
		} else if (pts_trace == 1) {
			if (pts_41_pattern_sink_index <
				PTS_41_PATTERN_SINK_MAX &&
				pts_41_pattern_sink_index > 0) {
				pts_41_pattern_sink[pts_41_pattern_sink_index] =
				1;
				pts_41_pattern_sink_index++;
			} else if (pts_pattern[PTS_41_PATTERN] ==
				PTS_41_PATTERN_DETECT_RANGE) {
				pts_pattern[PTS_41_PATTERN] = 0;
				pts_41_pattern_sink_index = 0;
				pts_pattern_exit_cnt[PTS_41_PATTERN]++;
				memset(&pts_41_pattern_sink[0], 0,
				       PTS_41_PATTERN_SINK_MAX);
				if (pts_log_enable[PTS_41_PATTERN])
					pr_info("video 4:1 mode broken\n");
			} else {
				pts_pattern[PTS_41_PATTERN] = 0;
				pts_41_pattern_sink_index = 0;
				memset(&pts_41_pattern_sink[0], 0,
				       PTS_41_PATTERN_SINK_MAX);
			}
		} else if (pts_pattern[PTS_41_PATTERN] ==
			PTS_41_PATTERN_DETECT_RANGE) {
			pts_pattern[PTS_41_PATTERN] = 0;
			pts_41_pattern_sink_index = 0;
			memset(&pts_41_pattern_sink[0], 0,
			       PTS_41_PATTERN_SINK_MAX);
			pts_pattern_exit_cnt[PTS_41_PATTERN]++;
			if (pts_log_enable[PTS_41_PATTERN])
				pr_info("video 4:1 mode broken\n");
		} else {
			pts_pattern[PTS_41_PATTERN] = 0;
			pts_41_pattern_sink_index = 0;
			memset(&pts_41_pattern_sink[0], 0,
			       PTS_41_PATTERN_SINK_MAX);
		}
		return;
	}

	/* update 3:2 or 2:2 mode detection */
	if ((pre_pts_trace == factor1 && pts_trace == factor2) ||
	    (pre_pts_trace == factor2 && pts_trace == factor1)) {
		if (pts_pattern[pattern] < pattern_range) {
			pts_pattern[pattern]++;
			if (pts_pattern[pattern] == pattern_range) {
				pts_pattern_enter_cnt[pattern]++;
				pts_pattern_detected = pattern;
				if (pts_log_enable[pattern])
					pr_info("video %d:%d mode detected\n",
						factor1, factor2);
			}
		}
	} else if (pts_pattern[pattern] == pattern_range) {
		pts_pattern[pattern] = 0;
		pts_pattern_exit_cnt[pattern]++;
		if (pts_log_enable[pattern])
			pr_info("video %d:%d mode broken\n", factor1, factor2);
	} else {
		pts_pattern[pattern] = 0;
	}
}
#endif

#ifdef INTERLACE_FIELD_MATCH_PROCESS
static inline bool interlace_field_type_need_match(int vout_type,
						   struct vframe_s *vf)
{
	if (vf_get_pts(vf) != vsync_pts_inc)
		return false;

	if (vout_type == VOUT_TYPE_TOP_FIELD &&
	    ((vf->type & VIDTYPE_TYPEMASK) == VIDTYPE_INTERLACE_BOTTOM))
		return true;
	else if (vout_type == VOUT_TYPE_BOT_FIELD &&
		 ((vf->type & VIDTYPE_TYPEMASK) == VIDTYPE_INTERLACE_TOP))
		return true;

	return false;
}
#endif

/* add a new function to check if current display frame has been*/
/*displayed for its duration */
static inline bool duration_expire(struct vframe_s *cur_vf,
				   struct vframe_s *next_vf, u32 dur)
{
	u32 pts;
	s32 dur_disp;
	static s32 rpt_tab_idx;
	static const u32 rpt_tab[4] = { 0x100, 0x100, 0x300, 0x300 };

	/* do not switch to new frames in none-normal speed */
	if (vsync_slow_factor > 1000)
		return false;

	if (!cur_vf || cur_dispbuf[0] == &vf_local[0])
		return true;
	pts = next_vf->pts;
	if (pts == 0)
		dur_disp = vf_get_pts(cur_vf);
	else
		dur_disp = pts - timestamp_vpts_get();

	if ((dur << 8) >= (dur_disp * rpt_tab[rpt_tab_idx & 3])) {
		rpt_tab_idx = (rpt_tab_idx + 1) & 3;
		return true;
	} else {
		return false;
	}
}

#ifdef PTS_LOGGING
static inline void vpts_perform_pulldown(struct vframe_s *next_vf,
					 bool *expired)
{
	int pattern_range, expected_curr_interval;
	int expected_prev_interval;
	int next_vf_nextpts = 0;

	/* Dont do anything if we have invalid data */
	if (!next_vf || !next_vf->pts)
		return;
	if (next_vf->next_vf_pts_valid)
		next_vf_nextpts = next_vf->next_vf_pts;

	switch (pts_pattern_detected) {
	case PTS_32_PATTERN:
		pattern_range = PTS_32_PATTERN_DETECT_RANGE;
		switch (pre_pts_trace) {
		case 3:
			expected_prev_interval = 3;
			expected_curr_interval = 2;
			break;
		case 2:
			expected_prev_interval = 2;
			expected_curr_interval = 3;
			break;
		default:
			return;
		}
		if (!next_vf_nextpts)
			next_vf_nextpts = next_vf->pts +
				PTS_32_PATTERN_DURATION;
		break;
	case PTS_22_PATTERN:
		if (pre_pts_trace != 2)
			return;
		pattern_range =  PTS_22_PATTERN_DETECT_RANGE;
		expected_prev_interval = 2;
		expected_curr_interval = 2;
		if (!next_vf_nextpts)
			next_vf_nextpts = next_vf->pts +
				PTS_22_PATTERN_DURATION;
		break;
	case PTS_41_PATTERN:
		/* TODO */
	default:
		return;
	}

	/* We do nothing if  we dont have enough data*/
	if (pts_pattern[pts_pattern_detected] != pattern_range)
		return;

	if (*expired) {
		if (pts_trace < expected_curr_interval) {
			/* 2323232323..2233..2323, prev=2, curr=3,*/
			/* check if next frame will toggle after 3 vsyncs */
			/* 22222...22222 -> 222..2213(2)22...22 */
			/* check if next frame will toggle after 3 vsyncs */
			int nextpts = timestamp_pcrscr_get() + vsync_pts_align;

			if (/*((int)(nextpts + expected_prev_interval * */
			/*vsync_pts_inc - next_vf->next_vf_pts) < 0) && */
				((int)(nextpts + (expected_prev_interval + 1) *
				vsync_pts_inc - next_vf_nextpts) >= 0)) {
				*expired = false;
				if (pts_log_enable[PTS_32_PATTERN] ||
				    pts_log_enable[PTS_22_PATTERN])
					pr_info("hold frame for pattern: %d",
						pts_pattern_detected);
			}

			/* here need to escape a vsync */
			if (timestamp_pcrscr_get() >
				(next_vf->pts + vsync_pts_inc)) {
				*expired = true;
				pts_escape_vsync = 1;
				if (pts_log_enable[PTS_32_PATTERN] ||
				    pts_log_enable[PTS_22_PATTERN])
					pr_info("escape a vsync pattern: %d",
						pts_pattern_detected);
			}
		}
	} else {
		if (pts_trace == expected_curr_interval) {
			/* 23232323..233223...2323 curr=2, prev=3 */
			/* check if this frame will expire next vsyncs and */
			/* next frame will expire after 3 vsyncs */
			/* 22222...22222 -> 222..223122...22 */
			/* check if this frame will expire next vsyncs and */
			/* next frame will expire after 2 vsyncs */
			int nextpts = timestamp_pcrscr_get() + vsync_pts_align;

			if (((int)(nextpts + vsync_pts_inc - next_vf->pts)
				>= 0) &&
			    ((int)(nextpts +
			    vsync_pts_inc * (expected_prev_interval - 1)
			    - next_vf_nextpts) < 0) &&
			    ((int)(nextpts + expected_prev_interval *
				vsync_pts_inc - next_vf_nextpts) >= 0)) {
				*expired = true;
				if (pts_log_enable[PTS_32_PATTERN] ||
				    pts_log_enable[PTS_22_PATTERN])
					pr_info("pull frame for pattern: %d",
						pts_pattern_detected);
			}
		}
	}
}
#endif

static inline bool vpts_expire(struct vframe_s *cur_vf,
			       struct vframe_s *next_vf,
			       int toggled_cnt)
{
	u32 pts;
#ifdef CONFIG_AMLOGIC_MEDIA_FRAME_SYNC
	u32 pts_temp = 0;
#endif
#ifdef VIDEO_PTS_CHASE
	u32 vid_pts, scr_pts;
	int aud_start = 0;
#endif
	u32 systime;
	u32 adjust_pts, org_vpts;
	bool expired;

	if (!next_vf)
		return false;

	if (videopeek) {
		videopeek = false;
		pr_info("video peek toggle the first frame\n");
		return true;
	}

	if (debug_flag & DEBUG_FLAG_TOGGLE_FRAME_PER_VSYNC)
		return true;
	if (/*(cur_vf == NULL) || (cur_dispbuf[0] == &vf_local[0]) ||*/ debugflags &
	    DEBUG_FLAG_FFPLAY)
		return true;

	if (freerun_mode == FREERUN_NODUR || hdmi_in_onvideo)
		return true;
	/*freerun for game mode*/
	if (next_vf->flag & VFRAME_FLAG_GAME_MODE)
		return true;

	/* FIXME: remove it */
	if (next_vf->flag & VFRAME_FLAG_VIDEO_COMPOSER)
		return true;

	/*freerun for drm video*/
	if (next_vf->flag & VFRAME_FLAG_VIDEO_DRM)
		return true;

	if (cur_vf &&
		(next_vf->source_type == VFRAME_SOURCE_TYPE_HDMI ||
		next_vf->source_type == VFRAME_SOURCE_TYPE_CVBS ||
		next_vf->source_type == VFRAME_SOURCE_TYPE_TUNER) &&
		!(next_vf->flag & VFRAME_FLAG_GAME_MODE) &&
		cur_vf->flag & VFRAME_FLAG_GAME_MODE) {
		pr_info("game to non-game\n");
		/*do avsync normal check*/
		timestamp_pcrscr_set(timestamp_vpts_get());
		hdmi_vframe_count = HDMI_DELAY_NORMAL_CHECK_COUNT;
		return true;
	}

	if (step_enable) {
		if (step_flag)
			return false;
		if (!step_flag) {
			step_flag = 1;
			return true;
		}
	}

	if (trickmode_i == 1 || trickmode_fffb == 1) {
		if (((atomic_read(&trickmode_framedone) == 0) ||
		     trickmode_i == 1) &&
		    !to_notify_trick_wait &&
		    trickmode_duration_count <= 0) {
			return true;
		} else {
			return false;
		}
	}
	if (omx_secret_mode && (!omx_run || !omx_drop_done))
		return false;

	if (next_vf->duration == 0)

		return true;

	systime = timestamp_pcrscr_get();
	pts = next_vf->pts;

#ifdef AVSYNC_COUNT
	if (abs(timestamp_apts_get() - timestamp_vpts_get()) > 90 * 500)
		av_discontinue = true;
	if (abs(timestamp_apts_get() - timestamp_vpts_get()) <= 90 * 500 &&
	    av_discontinue) {
		avsync_count++;
		av_discontinue = false;
#ifdef CONFIG_AMLOGIC_MEDIA_FRAME_SYNC
		timestamp_avsync_counter_set(avsync_count);
#endif
	}
#endif
	if ((pts == 0 && ((cur_dispbuf[0] && cur_dispbuf[0] != &vf_local[0]) ||
			   hold_property_changed == 1)) ||
	    freerun_mode == FREERUN_DUR) {
		pts =
		    timestamp_vpts_get() +
		    (cur_vf ? vf_get_pts(cur_vf) : 0);
		if (hold_property_changed == 1)
			hold_property_changed = 0;
	}
	/* check video PTS discontinuity */
#ifdef CONFIG_AMLOGIC_MEDIA_FRAME_SYNC
	if ((enable_video_discontinue_report) &&
	    (first_frame_toggled) &&
	    (AM_ABSSUB(systime, pts) > tsync_vpts_discontinuity_margin()) &&
	    ((next_vf->flag & VFRAME_FLAG_NO_DISCONTINUE) == 0)) {
		/*
		 * if paused ignore discontinue
		 */
		if (!timestamp_pcrscr_enable_state() &&
		    tsync_get_mode() != TSYNC_MODE_PCRMASTER) {
			/*pr_info("video pts discontinue,
			 * but pcrscr is disabled,
			 * return false\n");
			 */
			return false;
		}
		pts_temp = cur_vf ? vf_get_pts(cur_vf) : 0;
		pts = timestamp_vpts_get() + pts_temp;
		if (debug_flag & DEBUG_FLAG_OMX_DEBUG_DROP_FRAME) {
			pr_info("system=0x%x ,dur:%d,pts:0x%x,align:%d\n",
				systime,
				pts_temp, pts,
				vsync_pts_align);
		}
		/* pr_info("system=0x%x vpts=0x%x\n", systime,*/
		/*timestamp_vpts_get()); */
		/* [SWPL-21116] If pts and systime diff is smaller than
		 * vsync_pts_align, will return true at end of this
		 * function and show this frame. In this bug, next_vf->pts
		 * is not 0 and have large diff with systime, need get into
		 * discontinue process and shouldn't send out this frame.
		 */
		if ((((int)(systime - pts) >= 0) ||
		    (next_vf->pts > 0 &&
		    ((int)(systime + vsync_pts_align - pts) >= 0))) &&
		    tsync_get_mode() != TSYNC_MODE_PCRMASTER) {
			if (next_vf->pts != 0)
				tsync_avevent_locked(VIDEO_TSTAMP_DISCONTINUITY,
						     next_vf->pts);
			else if (next_vf->pts == 0 &&
				 (tsync_get_mode() != TSYNC_MODE_PCRMASTER))
				tsync_avevent_locked(VIDEO_TSTAMP_DISCONTINUITY,
						     pts);

			/* pr_info("discontinue,
			 *   systime=0x%x vpts=0x%x next_vf->pts = 0x%x\n",
			 *	systime,
			 *	pts,
			 *	next_vf->pts);
			 */

			/* pts==0 is a keep frame maybe. */
			if (systime > next_vf->pts || next_vf->pts == 0 ||
			    (systime < pts &&
			    (pts > 0xFFFFFFFF - TIME_UNIT90K)))
				return true;
			if (omx_secret_mode &&
			    cur_omx_index >= next_vf->omx_index)
				return true;

			return false;
		} else if (omx_secret_mode &&
			cur_omx_index >= next_vf->omx_index) {
			return true;
		} else if (tsync_check_vpts_discontinuity(pts) &&
			tsync_get_mode() == TSYNC_MODE_PCRMASTER) {
			/* in pcrmaster mode and pcr clk was used by tync,
			 * when the stream was replayed, the pcr clk was
			 * changed to the head of the stream. in this case,
			 * we send the "VIDEO_TSTAMP_DISCONTINUITY" signal
			 *  to notify tsync and adjust the sysclock to
			 * make playback smooth.
			 */
			if (next_vf->pts != 0)
				tsync_avevent_locked(VIDEO_TSTAMP_DISCONTINUITY,
						     next_vf->pts);
			else if (next_vf->pts == 0) {
				tsync_avevent_locked(VIDEO_TSTAMP_DISCONTINUITY,
						     pts);
				return true;
			}
		} else {
			/* +[SE] [BUG][SWPL-21070][zihao.ling]
			 *when vdiscontinue, not displayed
			 */
			return false;
		}
	} else if (omx_run &&
		omx_secret_mode &&
		(omx_pts + omx_pts_interval_upper < next_vf->pts) &&
		(omx_pts_set_index >= next_vf->omx_index)) {
		pr_info("omx, omx_pts=%d omx_pts_set_index=%d pts=%d omx_index=%d\n",
			omx_pts,
			omx_pts_set_index,
			next_vf->pts,
			next_vf->omx_index);
		return true;
	}
#endif
	if (vsync_pts_inc_upint && !freerun_mode) {
		struct vframe_states frame_states;
		u32 delayed_ms, t1, t2;

		delayed_ms =
		    calculation_stream_delayed_ms(PTS_TYPE_VIDEO, &t1, &t2);
		memset(&frame_states, 0, sizeof(struct vframe_states));
		if (amvideo_vf_get_states(&frame_states) == 0) {
			u32 pcr = timestamp_pcrscr_get();
			u32 vpts = timestamp_vpts_get();
			u32 diff = pcr - vpts;

			if (delayed_ms > 200) {
				vsync_freerun++;
				if (pcr < next_vf->pts ||
				    pcr < vpts + next_vf->duration) {
					if (next_vf->pts > 0) {
						timestamp_pcrscr_set
						    (next_vf->pts);
					} else {
						timestamp_pcrscr_set(vpts +
							next_vf->duration);
					}
				}
				return true;
			} else if ((frame_states.buf_avail_num >= 3) &&
				diff < vsync_pts_inc << 2) {
				vsync_pts_inc_adj =
				    vsync_pts_inc + (vsync_pts_inc >> 2);
				vsync_pts_125++;
			} else if ((frame_states.buf_avail_num >= 2 &&
				diff < vsync_pts_inc << 1)) {
				vsync_pts_inc_adj =
				    vsync_pts_inc + (vsync_pts_inc >> 3);
				vsync_pts_112++;
			} else if (frame_states.buf_avail_num >= 1 &&
				diff < vsync_pts_inc - 20) {
				vsync_pts_inc_adj = vsync_pts_inc + 10;
				vsync_pts_101++;
			} else {
				vsync_pts_inc_adj = 0;
				vsync_pts_100++;
			}
		}
	}

#ifdef VIDEO_PTS_CHASE
	vid_pts = timestamp_vpts_get();
	scr_pts = timestamp_pcrscr_get();
	vid_pts += vsync_pts_inc;

	if (av_sync_flag) {
		if (vpts_chase) {
			if ((abs(vid_pts - scr_pts) < 6000) ||
			    (abs(vid_pts - scr_pts) > 90000)) {
				vpts_chase = 0;
				pr_info("leave vpts chase mode, diff:%d\n",
					vid_pts - scr_pts);
			}
		} else if ((abs(vid_pts - scr_pts) > 9000) &&
			(abs(vid_pts - scr_pts) < 90000)) {
			vpts_chase = 1;
			if (vid_pts < scr_pts)
				vpts_chase_pts_diff = 50;
			else
				vpts_chase_pts_diff = -50;
			vpts_chase_counter =
			    ((int)(scr_pts - vid_pts)) / vpts_chase_pts_diff;
			pr_info("enter vpts chase mode, diff:%d\n",
				vid_pts - scr_pts);
		} else if (abs(vid_pts - scr_pts) >= 90000) {
			pr_info("video pts discontinue, diff:%d\n",
				vid_pts - scr_pts);
		}
	} else {
		vpts_chase = 0;
	}
	if (vpts_chase) {
		u32 curr_pts =
		    scr_pts - vpts_chase_pts_diff * vpts_chase_counter;

	/* pr_info("vchase pts %d, %d, %d, %d, %d\n",*/
	/*curr_pts, scr_pts, curr_pts-scr_pts, vid_pts, vpts_chase_counter); */
		return ((int)(curr_pts - pts)) >= 0;
	}
	aud_start = (timestamp_apts_get() != -1);

	if (!av_sync_flag && aud_start && (abs(scr_pts - pts) < 9000) &&
	    ((int)(scr_pts - pts) < 0)) {
		av_sync_flag = 1;
		pr_info("av sync ok\n");
	}
	return ((int)(scr_pts - pts)) >= 0;
#else
	if (smooth_sync_enable) {
		org_vpts = timestamp_vpts_get();
		if ((abs(org_vpts + vsync_pts_inc - systime) <
			M_PTS_SMOOTH_MAX) &&
			(abs(org_vpts + vsync_pts_inc - systime) >
			M_PTS_SMOOTH_MIN)) {
			if (!video_frame_repeat_count) {
				vpts_ref = org_vpts;
				video_frame_repeat_count++;
			}

			if ((int)(org_vpts + vsync_pts_inc - systime) > 0) {
				adjust_pts =
				    vpts_ref + (vsync_pts_inc -
						M_PTS_SMOOTH_ADJUST) *
				    video_frame_repeat_count;
			} else {
				adjust_pts =
				    vpts_ref + (vsync_pts_inc +
						M_PTS_SMOOTH_ADJUST) *
				    video_frame_repeat_count;
			}

			return (int)(adjust_pts - pts) >= 0;
		}

		if (video_frame_repeat_count) {
			vpts_ref = 0;
			video_frame_repeat_count = 0;
		}
	}
	if (tsync_get_mode() == TSYNC_MODE_PCRMASTER)
		expired = (timestamp_pcrscr_get() + vsync_pts_align >= pts) ?
				true : false;
	else
		expired = (int)(timestamp_pcrscr_get() +
				vsync_pts_align - pts) >= 0;

	if (debug_flag & DEBUG_FLAG_HDMI_AVSYNC_DEBUG)
		pr_info("vf/next %p/%p, pcr %d, pts %d, duration %d, expired %d\n",
			cur_vf, next_vf, timestamp_pcrscr_get(), pts, next_vf->duration, expired);

#ifdef PTS_THROTTLE
	if (expired && next_vf && next_vf->next_vf_pts_valid &&
	    vsync_slow_factor == 1 &&
	    next_vf->next_vf_pts &&
	    toggled_cnt > 0 &&
	    ((int)(timestamp_pcrscr_get() + vsync_pts_inc +
	    vsync_pts_align - next_vf->next_vf_pts) < 0)) {
		expired = false;
	} else if (!expired && next_vf && next_vf->next_vf_pts_valid &&
		(vsync_slow_factor == 1) &&
		next_vf->next_vf_pts &&
		(toggled_cnt == 0) &&
		((int)(timestamp_pcrscr_get() + vsync_pts_inc +
		vsync_pts_align - next_vf->next_vf_pts) >= 0)) {
		expired = true;
	}
#endif

#ifdef PTS_LOGGING
	if (pts_enforce_pulldown) {
		/* Perform Pulldown if needed*/
		vpts_perform_pulldown(next_vf, &expired);
	}
#endif
	return expired;
#endif
}

#ifdef PTS_LOGGING
static void vsync_video_pattern(void)
{
	/* Check for 3:2*/
	log_vsync_video_pattern(PTS_32_PATTERN);
	/* Check for 2:2*/
	log_vsync_video_pattern(PTS_22_PATTERN);
	/* Check for 4:1*/
	log_vsync_video_pattern(PTS_41_PATTERN);
}
#endif

static void set_omx_pts(u32 *p)
{
	u32 tmp_pts = p[0];
	/*u32 vision = p[1];*/
	u32 set_from_hwc = p[2];
	u32 frame_num = p[3];
	u32 not_reset = p[4];
	u32 session = p[5];
	unsigned int try_cnt = 0x1000;
	bool updateomxpts;

	cur_omx_index = frame_num;
	mutex_lock(&omx_mutex);
	if (omx_pts_set_index < frame_num)
		omx_pts_set_index = frame_num;

	if (omx_check_previous_session) {
		if (session != omx_cur_session) {
			omx_cur_session = session;
			omx_check_previous_session = false;
		} else {
			mutex_unlock(&omx_mutex);
			pr_info("check session return: tmp_pts %d session=0x%x\n",
				tmp_pts, omx_cur_session);
			omx_pts_set_index = 0;
			return;
		}
	}
	if (debug_flag & DEBUG_FLAG_PTS_TRACE)
		pr_info("[%s]tmp_pts:%d, set_from_hwc:%d,frame_num=%d, not_reset=%d\n",
			__func__, tmp_pts, set_from_hwc, frame_num, not_reset);
	if (set_from_hwc == 1) {
		if (frame_num >= cur_disp_omx_index) {
			omx_continuous_drop_flag = false;
			omx_continuous_drop_count = 0;
		} else {
			if (omx_continuous_drop_flag &&
			    (debug_flag &
			     DEBUG_FLAG_OMX_DEBUG_DROP_FRAME))
				pr_info("ignore previous rendered frame %d\n",
					frame_num);
		}
	} else {
		omx_continuous_drop_count++;
		if (omx_continuous_drop_count >=
		     OMX_CONTINUOUS_DROP_LEVEL) {
			omx_continuous_drop_flag = true;
			if (debug_flag & DEBUG_FLAG_OMX_DEBUG_DROP_FRAME)
				pr_info("continuous drop %d\n",
					omx_continuous_drop_count);
		}
	}
	if (not_reset == 0) {
		updateomxpts = set_from_hwc;
		if (!set_from_hwc) {
			omx_continusdrop_cnt++;
			if (omx_continusdrop_cnt > 1) {
				/* continuous drop update omx_pts */
				updateomxpts = true;
			} else {
				struct vframe_s *vf = NULL;

				vf = vf_peek(RECEIVER_NAME);
				if (vf && vf->omx_index > 0 &&
				    omx_pts_set_index > vf->omx_index)
					omx_pts_set_index = vf->omx_index - 1;
			}
		} else {
			omx_continusdrop_cnt = 0;
		}
		if (updateomxpts) {
			time_setomxpts_last = time_setomxpts;
			time_setomxpts = sched_clock();
			omx_pts = tmp_pts;
			ATRACE_COUNTER("omxpts", omx_pts);
		}
	}
	/* kodi may render first frame, then drop dozens of frames */
	if (set_from_hwc == 0 && omx_run &&
	    frame_num <= 2 && not_reset == 0 &&
	    omx_pts_set_from_hwc_count > 0) {
		pr_info("reset omx_run to false.\n");
		omx_run = false;
	}
	if (set_from_hwc == 1) {
		if (!omx_run) {
			omx_need_drop_frame_num =
				frame_num > 0 ? frame_num - 1 : 0;
			if (omx_need_drop_frame_num == 0)
				omx_drop_done = true;
			pr_info("omx need drop %d\n",
				omx_need_drop_frame_num);
		}
		omx_run = true;
		if (omx_pts_set_from_hwc_count < OMX_MAX_COUNT_RESET_SYSTEMTIME)
			omx_pts_set_from_hwc_count++;
		if (omx_pts_set_from_hwc_count_begin <
			OMX_MAX_COUNT_RESET_SYSTEMTIME_BEGIN)
			omx_pts_set_from_hwc_count_begin++;

	} else if (set_from_hwc == 0 && !omx_run) {
		struct vframe_s *vf = NULL;

		while (try_cnt--) {
			vf = vf_peek(RECEIVER_NAME);
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
			if (is_amdv_enable() &&
			    vf && is_amdv_frame(vf)) {
				if (debug_flag &
					DEBUG_FLAG_OMX_DV_DROP_FRAME)
					pr_info("dovi will drop %d in vsync\n",
						frame_num);
				dovi_drop_flag = true;
				dovi_drop_frame_num = frame_num;

				if (disable_dv_drop) {
					omx_run = true;
					dovi_drop_flag = false;
					dovi_drop_frame_num = 0;
					omx_drop_done = true;
				}
				break;
			}
#endif
			if (vf) {
				pr_debug("drop frame_num=%d, vf->omx_index=%d\n",
					 frame_num, vf->omx_index);
				if (frame_num >= vf->omx_index) {
					vf = vf_get(RECEIVER_NAME);
					if (vf)
						vf_put(vf, RECEIVER_NAME);
				} else {
					break;
				}
			} else {
				break;
			}
		}
	}
	mutex_unlock(&omx_mutex);
}

static int alloc_layer(u32 layer_id)
{
	int ret = -EINVAL;

	if (layer_id == 0) {
		if (layer_cap & LAYER0_BUSY) {
			ret = -EBUSY;
		} else if (layer_cap & LAYER0_AVAIL) {
			ret = 0;
			layer_cap |= LAYER0_BUSY;
		}
	} else if (layer_id == 1) {
		if (layer_cap & LAYER1_BUSY) {
			ret = -EBUSY;
		} else if (layer_cap & LAYER1_AVAIL) {
			ret = 0;
			layer_cap |= LAYER1_BUSY;
		}
	} else if (layer_id == 2) {
		if (layer_cap & LAYER2_BUSY) {
			ret = -EBUSY;
		} else if (layer_cap & LAYER2_AVAIL) {
			ret = 0;
			layer_cap |= LAYER2_BUSY;
		}
	}
	return ret;
}

static int free_layer(u32 layer_id)
{
	int ret = -EINVAL;

	if (layer_id == 0) {
		if ((layer_cap & LAYER0_BUSY) &&
		    (layer_cap & LAYER0_AVAIL)) {
			ret = 0;
			layer_cap &= ~LAYER0_BUSY;
		}
	} else if (layer_id == 1) {
		if ((layer_cap & LAYER1_BUSY) &&
		    (layer_cap & LAYER1_AVAIL)) {
			ret = 0;
			layer_cap &= ~LAYER1_BUSY;
		}
	} else if (layer_id == 2) {
		if ((layer_cap & LAYER2_BUSY) &&
		    (layer_cap & LAYER2_AVAIL)) {
			ret = 0;
			layer_cap &= ~LAYER2_BUSY;
		}
	}
	return ret;
}

static void update_process_hdmi_avsync_flag(bool flag)
{
	char *provider_name = vf_get_provider_name(RECEIVER_NAME);
	unsigned long flags;

	/*enable hdmi delay process only when audio have required*/
	if (last_required_total_delay <= 0)
		return;

	while (provider_name) {
		if (!vf_get_provider_name(provider_name))
			break;
		provider_name =
			vf_get_provider_name(provider_name);
	}
	if (provider_name && (!strcmp(provider_name, "dv_vdin") ||
		!strcmp(provider_name, "vdin0"))) {
		spin_lock_irqsave(&hdmi_avsync_lock, flags);
		hdmi_delay_first_check = flag;
		hdmi_vframe_count = 0;
		hdmin_delay_count_debug = 0;
		if (enable_hdmi_delay_normal_check && flag)
			hdmi_delay_normal_check = enable_hdmi_delay_normal_check;
		else
			hdmi_delay_normal_check = 0;
		pr_info("update hdmi_delay_check %d\n", flag);
		spin_unlock_irqrestore(&hdmi_avsync_lock, flags);
	}
}

static int video_vdin_buf_info_get(void)
{
	char *provider_name = vf_get_provider_name(RECEIVER_NAME);
	int max_buf_cnt = -1;

	while (provider_name) {
		if (!vf_get_provider_name(provider_name))
			break;
		provider_name =
			vf_get_provider_name(provider_name);
	}
	if (provider_name && (!strcmp(provider_name, "dv_vdin") ||
		!strcmp(provider_name, "vdin0")))
		vf_notify_provider_by_name(provider_name,
			VFRAME_EVENT_RECEIVER_BUF_COUNT, (void *)&max_buf_cnt);
	return max_buf_cnt;
}

static inline bool is_valid_drop_count(int drop_count)
{
	int buf_cnt = video_vdin_buf_info_get();

	if (drop_count > 0 && drop_count < (buf_cnt - 2))
		return true;
	return false;
}

static void process_hdmi_video_sync(struct vframe_s *vf)
{
	char *provider_name = vf_get_provider_name(RECEIVER_NAME);
	int update_value = 0;
	unsigned long flags;
	int vsync_dur = 16;
	int need_drop = 0;
	int hdmin_delay_min_ms = vsync_dur * HDMI_VIDEO_MIN_DELAY;

	if ((!hdmi_delay_first_check && !hdmi_delay_normal_check &&
	     hdmin_delay_start == 0) || !vf || last_required_total_delay <= 0)
		return;

	if (vf->flag & VFRAME_FLAG_GAME_MODE) {
		if (debug_flag & DEBUG_FLAG_HDMI_AVSYNC_DEBUG)
			pr_info("game mode, not do avsync\n");
		return;
	}

	hdmin_delay_duration = 0;
	while (provider_name) {
		if (!vf_get_provider_name(provider_name))
			break;
		provider_name =
			vf_get_provider_name(provider_name);
	}
	if (provider_name && (!strcmp(provider_name, "dv_vdin") ||
		!strcmp(provider_name, "vdin0"))) {
		if (vf->duration > 0) {
			vsync_dur = (int)(vf->duration / 96);
			hdmin_delay_min_ms = vsync_dur *
				HDMI_VIDEO_MIN_DELAY;
		}
		spin_lock_irqsave(&hdmi_avsync_lock, flags);

		if (last_required_total_delay > vframe_walk_delay) { /*delay video*/
			vframe_walk_delay = (int)div_u64(((jiffies_64 -
			vf->ready_jiffies64) * 1000), HZ);
#ifdef CONFIG_AMLOGIC_MEDIA_FRC
			vframe_walk_delay += frc_get_video_latency();
#endif
			/*check hdmi max delay*/
			if (last_required_total_delay > hdmin_delay_max_ms) {
				if (hdmin_delay_max_ms > vframe_walk_delay)
					update_value = hdmin_delay_max_ms -
					vframe_walk_delay;
			} else {
				update_value = last_required_total_delay -
					vframe_walk_delay;
			}
			/*set only if delay bigger than half vsync*/
			if (update_value > vsync_dur / 2) {
				hdmin_delay_duration = update_value;
				hdmin_delay_start_time = -1;
				hdmin_delay_count_debug++;
				hdmin_delay_done = false;
				hdmin_need_drop_count = 0;
			}
		} else { /*drop video*/
			/*check hdmi min delay*/
			if (last_required_total_delay >= hdmin_delay_min_ms)
				update_value = vframe_walk_delay -
					last_required_total_delay;
			else
				update_value = vframe_walk_delay -
					hdmin_delay_min_ms;

			/*drop only if diff bigger than half vsync*/
			if (update_value > vsync_dur / 2) {
				need_drop = update_value / vsync_dur;
				/*check if drop need_drop + 1 is closer to*/
				/*required than need_drop*/
				if ((update_value - need_drop * vsync_dur) >
					vsync_dur / 2) {
					if ((vframe_walk_delay -
						(need_drop + 1) * vsync_dur) >=
						hdmin_delay_min_ms)
						need_drop = need_drop + 1;
				}
				hdmin_delay_duration = -update_value;
				hdmin_delay_done = true;
				if (is_valid_drop_count(need_drop))
					hdmin_need_drop_count = need_drop;
			}
		}

		//if (debug_flag & DEBUG_FLAG_HDMI_AVSYNC_DEBUG)
		pr_info("required delay:%d, current %d, extra %d, delay %s, drop cnt %d, normal_check:0x%02x\n",
			last_required_total_delay,
			vframe_walk_delay,
			hdmin_delay_duration,
			hdmin_delay_done ? "false" : "true",
			need_drop, hdmi_delay_normal_check);
		spin_unlock_irqrestore(&hdmi_avsync_lock, flags);
	}
}

/*ret = 0: no need delay*/
/*ret = 1: need to delay*/
static int hdmi_in_delay_check(struct vframe_s *vf)
{
	int expire;
	int vsync_duration = 0;
	u64 pts = 0;
	u64 us;
	unsigned long flags;
	int expire_align = 0;

	char *provider_name = vf_get_provider_name(RECEIVER_NAME);

	if (hdmin_delay_done)
		return 0;

	if (!vf || vf->duration == 0)
		return 0;

	while (provider_name) {
		if (!vf_get_provider_name(provider_name))
			break;
		provider_name =
			vf_get_provider_name(provider_name);
	}
	if (!provider_name || (strcmp(provider_name, "dv_vdin") &&
		strcmp(provider_name, "vdin0"))) {
		return 0;
	}

	spin_lock_irqsave(&hdmi_avsync_lock, flags);
	/* update duration */
	vsync_duration = (int)(vf->duration / 96);

	if (hdmin_delay_start_time == -1) {
		hdmin_delay_start_time = jiffies_to_msecs(jiffies);
		/*this function lead to one vsync delay */
		/*hdmin_delay_start_time -= vsync_duration;*/

		if (debug_flag & DEBUG_FLAG_HDMI_AVSYNC_DEBUG)
			pr_info("vsync_duration %d, hdmin_delay_start_time %d\n",
				vsync_duration, hdmin_delay_start_time);
		spin_unlock_irqrestore(&hdmi_avsync_lock, flags);
		return 1;
	}

	expire = jiffies_to_msecs(jiffies) -
		hdmin_delay_start_time;

	if (last_required_total_delay >= hdmin_delay_max_ms) {
		/*when required more than hdmin_delay_max_ms, */
		expire_align = -vsync_duration;
	} else {
		/*delay one more vsync? select the one that closer to required*/
		expire_align = -vsync_duration / 2;
	}
	if (debug_flag & DEBUG_FLAG_HDMI_AVSYNC_DEBUG)
		pr_info("expire %d, hdmin_delay_duration %d, expire_align %d\n",
			expire, hdmin_delay_duration, expire_align);

	if (expire - hdmin_delay_duration <= expire_align) {
		spin_unlock_irqrestore(&hdmi_avsync_lock, flags);
		return 1;
	}
	hdmin_delay_done = true;

	if (debug_flag & DEBUG_FLAG_HDMI_AVSYNC_DEBUG)
		pr_info("hdmi video delay done! expire %d\n", expire);

	/*reset vpts=pcr will lead vpts_expire delay 1 vsync - vsync_pts_align*/
	timestamp_vpts_set(timestamp_pcrscr_get() - (vf_get_pts(vf) - vsync_pts_align));
#ifdef CONFIG_AMLOGIC_MEDIA_FRAME_SYNC
	timestamp_vpts_set_u64((u64)(timestamp_pcrscr_get() -
		(vf_get_pts(vf) - vsync_pts_align)));
	pts = (u64)timestamp_pcrscr_get_u64();
#endif
	pts = pts - (vf_get_pts(vf) - vsync_pts_align);
	us = div64_u64(pts * 100, 9);
#ifdef CONFIG_AMLOGIC_MEDIA_FRAME_SYNC
	timestamp_vpts_set_u64(us);
#endif
	spin_unlock_irqrestore(&hdmi_avsync_lock, flags);

	return 0;
}

static void hdmi_in_drop_frame(void)
{
	struct vframe_s *vf;

	while (hdmin_need_drop_count > 0) {
		vf = amvideo_vf_get();
		if (!vf) { /*no video frame, drop done*/
			/*hdmi_need_drop_count = 0;*/
			break;
		}
		if (amvideo_vf_put(vf) < 0)
			check_dispbuf(0, vf, true);

		if (debug_flag & DEBUG_FLAG_PRINT_DROP_FRAME)
			pr_info("#line %d: drop %p\n", __LINE__, vf);
		video_drop_vf_cnt[0]++;
		--hdmin_need_drop_count;
	}
}

static inline bool video_vf_disp_mode_check(struct vframe_s *vf)
{
	struct provider_disp_mode_req_s req;
	char *provider_name = vf_get_provider_name(RECEIVER_NAME);

	req.vf = vf;
	req.disp_mode = VFRAME_DISP_MODE_NULL;
	req.req_mode = 1;

	while (provider_name) {
		if (!vf_get_provider_name(provider_name))
			break;
		provider_name =
			vf_get_provider_name(provider_name);
	}
	if (provider_name)
		vf_notify_provider_by_name(provider_name,
			VFRAME_EVENT_RECEIVER_DISP_MODE, (void *)&req);
	if (req.disp_mode == VFRAME_DISP_MODE_OK ||
		req.disp_mode == VFRAME_DISP_MODE_NULL)
		return false;

	/*set video vpts*/
	if (cur_dispbuf[0] != vf) {
		if (vf->pts != 0) {
			amlog_mask(LOG_MASK_TIMESTAMP,
				   "vpts to vf->pts:0x%x,scr:0x%x,abs_scr: 0x%x\n",
			vf->pts, timestamp_pcrscr_get(),
			READ_MPEG_REG(SCR_HIU));
			timestamp_vpts_set(vf->pts);
		} else if (cur_dispbuf[0]) {
			amlog_mask(LOG_MASK_TIMESTAMP,
				   "vpts inc:0x%x,scr: 0x%x, abs_scr: 0x%x\n",
			timestamp_vpts_get() +
			vf_get_pts(cur_dispbuf[0]),
			timestamp_pcrscr_get(),
			READ_MPEG_REG(SCR_HIU));
			timestamp_vpts_inc(vf_get_pts(cur_dispbuf[0]));

			vpts_remainder +=
				vf_get_pts_rm(cur_dispbuf[0]);
			if (vpts_remainder >= 0xf) {
				vpts_remainder -= 0xf;
				timestamp_vpts_inc(-1);
			}
		}
	}
	if (debug_flag & DEBUG_FLAG_HDMI_AVSYNC_DEBUG)
		pr_info("video %p disp_mode %d\n", vf, req.disp_mode);

	if (amvideo_vf_put(vf) < 0)
		check_dispbuf(0, vf, true);

	return true;
}

static enum vframe_disp_mode_e video_vf_disp_mode_get(struct vframe_s *vf)
{
	struct provider_disp_mode_req_s req;
	char *provider_name = vf_get_provider_name(RECEIVER_NAME);

	req.vf = vf;
	req.disp_mode = VFRAME_DISP_MODE_NULL;
	req.req_mode = 0;

	while (provider_name) {
		if (!vf_get_provider_name(provider_name))
			break;
		provider_name =
			vf_get_provider_name(provider_name);
	}
	if (provider_name)
		vf_notify_provider_by_name(provider_name,
			VFRAME_EVENT_RECEIVER_DISP_MODE, (void *)&req);
	return req.disp_mode;
}

static inline bool video_vf_dirty_put(struct vframe_s *vf)
{
	if (!vf->frame_dirty)
		return false;
	if (cur_dispbuf[0] != vf) {
#ifdef CONFIG_AMLOGIC_MEDIA_FRAME_SYNC
		if (vf->pts != 0) {
			amlog_mask(LOG_MASK_TIMESTAMP,
				   "vpts to vf->pts:0x%x,scr:0x%x,abs_scr: 0x%x\n",
			vf->pts, timestamp_pcrscr_get(),
			READ_MPEG_REG(SCR_HIU));
			timestamp_vpts_set(vf->pts);
			timestamp_vpts_set_u64(vf->pts_us64);
		} else if (cur_dispbuf[0]) {
			amlog_mask(LOG_MASK_TIMESTAMP,
				   "vpts inc:0x%x,scr: 0x%x, abs_scr: 0x%x\n",
			timestamp_vpts_get() +
			vf_get_pts(cur_dispbuf[0]),
			timestamp_pcrscr_get(),
			READ_MPEG_REG(SCR_HIU));
			timestamp_vpts_inc
				(vf_get_pts(cur_dispbuf[0]));
			timestamp_vpts_inc_u64
				(vf_get_pts(cur_dispbuf[0]));

			vpts_remainder +=
				vf_get_pts_rm(cur_dispbuf[0]);
			if (vpts_remainder >= 0xf) {
				vpts_remainder -= 0xf;
				timestamp_vpts_inc(-1);
				timestamp_vpts_inc_u64(-1);
			}
		}
#endif
	}
	if (amvideo_vf_put(vf) < 0)
		check_dispbuf(0, vf, true);
	return true;
}

#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
/*SDK test: check metadata crc*/
/*frame crc error ---> drop err frame and repeat last frame*/
/*err count >= 6 ---> mute*/
static inline bool dv_vf_crc_check(struct vframe_s *vf)
{
	bool crc_err = false;
	char *provider_name = vf_get_provider_name(RECEIVER_NAME);

	while (provider_name) {
		if (!vf_get_provider_name(provider_name))
			break;
		provider_name =
			vf_get_provider_name(provider_name);
	}
	if (provider_name && (!strcmp(provider_name, "dv_vdin") ||
			      !strcmp(provider_name, "vdin0"))) {
		if (!vf->dv_crc_sts) {
			/*drop err crc frame*/
			vdin_err_crc_cnt++;
			if (debug_flag & DEBUG_FLAG_HDMI_DV_CRC)
				pr_info("vdin_err_crc_cnt %d\n",
					vdin_err_crc_cnt);

			/*need set video vpts when drop frame*/
			if (cur_dispbuf[0] != vf) {
				if (vf->pts != 0) {
					amlog_mask(LOG_MASK_TIMESTAMP,
						   "vpts to vf->pts:0x%x,scr:0x%x,abs_scr: 0x%x\n",
					vf->pts, timestamp_pcrscr_get(),
					READ_MPEG_REG(SCR_HIU));
					timestamp_vpts_set(vf->pts);
				} else if (cur_dispbuf[0]) {
					amlog_mask(LOG_MASK_TIMESTAMP,
						   "vpts inc:0x%x,scr: 0x%x, abs_scr: 0x%x\n",
					timestamp_vpts_get() +
					vf_get_pts(cur_dispbuf[0]),
					timestamp_pcrscr_get(),
					READ_MPEG_REG(SCR_HIU));
					timestamp_vpts_inc(vf_get_pts(cur_dispbuf[0]));

					vpts_remainder +=
					vf_get_pts_rm(cur_dispbuf[0]);
					if (vpts_remainder >= 0xf) {
						vpts_remainder -= 0xf;
						timestamp_vpts_inc(-1);
					}
				}
			}
			if (amvideo_vf_put(vf) < 0)
				check_dispbuf(0, vf, true);
			crc_err = true;
		} else {
			vdin_err_crc_cnt = 0;
		}

	} else {
		vdin_err_crc_cnt = 0;
	}

	/*mute when err crc > = 6*/
	if (vdin_err_crc_cnt >= ERR_CRC_COUNT) {
		set_video_mute(VIDEO_MUTE_SET, true);
		dv_mute_vpp_flag = true;
	} else if (dv_mute_vpp_flag) {
		set_video_mute(VIDEO_MUTE_SET, false);
		dv_mute_vpp_flag = false;
	}
	return crc_err;
}

struct vframe_s *dv_toggle_frame(struct vframe_s *vf, enum vd_path_e vd_path, bool new_frame)
{
	struct vframe_s *toggle_vf = NULL;

	if (!is_amdv_enable()) {
		cur_dispbuf2 = NULL;
		dvel_size = 0;
		dvel_changed = false;
		return NULL;
	}

	if (new_frame) {
		int ret = amdv_update_metadata(vf, vd_path, false);

		if ((!is_amdv_el_disable() ||
		    for_amdv_certification()) &&
		    !is_multi_dv_mode())
			cur_dispbuf2 = amdv_vf_peek_el(vf);
		if (ret == 0) {
			/* setting generated for this frame */
			/* or DOVI in bypass mode */
			toggle_vf = vf;
			amdv_set_toggle_flag(1);
		} else if (ret == 1) {
			/* both dolby and hdr module bypass */
			toggle_vf = vf;
			amdv_set_toggle_flag(0);
		} else {
			/* fail generating setting for this frame */
			toggle_vf = NULL;
			amdv_set_toggle_flag(0);
		}
	} else {
		/* FIXME: if need the is on condition */
		/* if (is_amdv_on() && get_video_enabled(0)) */
		if (!amdv_parse_metadata(vf, vd_path, 2, false, false))
			amdv_set_toggle_flag(1);
	}
	return toggle_vf;
}

/* 1: drop fail; 0: drop success*/
static int dolby_vision_drop_frame(void)
{
	struct vframe_s *vf;

	if (dolby_vision_need_wait(0)) {
		if (debug_flag & DEBUG_FLAG_OMX_DV_DROP_FRAME)
			pr_info("drop frame need wait!\n");
		return 1;
	}
	vf = amvideo_vf_get();
	if (!vf)
		return 1;

	if (vf && (debug_flag & DEBUG_FLAG_OMX_DV_DROP_FRAME))
		pr_info("drop vf %p, index %d, pts %d\n",
			vf, vf->omx_index, vf->pts);

	amdv_update_metadata(vf, VD1_PATH, true);
	if (amvideo_vf_put(vf) < 0)
		check_dispbuf(0, vf, true);

	if (debug_flag & DEBUG_FLAG_OMX_DV_DROP_FRAME)
		pr_info("drop vf %p done\n", vf);

	return 0;
}
#endif

void do_frame_detect(void)
{
	static u32 interrupt_count;

	if (frame_detect_flag == 1 &&
	    receive_frame_count &&
		frame_detect_time &&
		atomic_read(&video_unreg_flag)) {
		struct vinfo_s *video_info;

		video_info = get_current_vinfo();
		if (video_frame_detect.interrupt_count == 0) {
			interrupt_count = 0;
#ifdef CONFIG_AMLOGIC_VOUT
			video_frame_detect.interrupt_count =
				frame_detect_time *
				video_info->sync_duration_num /
			    video_info->sync_duration_den;
			if (debug_flag & DEBUG_FLAG_FRAME_DETECT) {
				pr_info("sync_duration_num = %d\n",
					video_info->sync_duration_num);
				pr_info("sync_duration_den = %d\n",
					video_info->sync_duration_den);
			}
#endif
			video_frame_detect.start_receive_count =
				receive_frame_count;
		}

		interrupt_count++;

		if (interrupt_count == video_frame_detect.interrupt_count + 1) {
			u32 receive_count;
			u32 expect_frame_count = 0;

			receive_count = receive_frame_count -
				video_frame_detect.start_receive_count;
			expect_frame_count =
				video_frame_detect.interrupt_count *
				frame_detect_fps *
				video_info->sync_duration_den /
				video_info->sync_duration_num /
				1000;

			if (receive_count < expect_frame_count) {
				frame_detect_drop_count +=
					expect_frame_count -
					receive_count;
				if (debug_flag & DEBUG_FLAG_FRAME_DETECT) {
					pr_info("drop_count = %d\n",
						expect_frame_count -
						receive_count);
				}
				frame_detect_receive_count +=
					expect_frame_count;
			} else {
				frame_detect_receive_count += receive_count;
			}
			if (debug_flag & DEBUG_FLAG_FRAME_DETECT) {
				pr_info("expect count = %d\n",
					expect_frame_count);
				pr_info("receive_count = %d, time = %ds\n",
					receive_count,
					frame_detect_time);
				pr_info("interrupt_count = %d\n",
					video_frame_detect.interrupt_count);
				pr_info("frame_detect_drop_count = %d\n",
					frame_detect_drop_count);
				pr_info("frame_detect_receive_count = %d\n",
					frame_detect_receive_count);
			}
			interrupt_count = 0;
			memset(&video_frame_detect, 0,
			       sizeof(struct video_frame_detect_s));
		}
	}
}

void frame_drop_process(void)
{
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
	if (is_amdv_enable() && dovi_drop_flag) {
		struct vframe_s *vf = NULL;
		unsigned int cnt = 10;
		int max_drop_index;

		if (debug_flag & DEBUG_FLAG_OMX_DV_DROP_FRAME)
			pr_info("dovi_drop_frame_num %d, omx_run %d\n",
				dovi_drop_frame_num, omx_run);
		while (cnt--) {
			vf = amvideo_vf_peek();
			if (vf && is_amdv_frame(vf)) {
				max_drop_index = omx_run ?
				omx_need_drop_frame_num : dovi_drop_frame_num;

				if (max_drop_index >= vf->omx_index) {
					if (dolby_vision_drop_frame() == 1)
						break;
					continue;
				} else if (omx_run &&
					   (vf->omx_index >
					   omx_need_drop_frame_num)) {
					/* all drop done*/
					dovi_drop_flag = false;
					omx_drop_done = true;
					if (debug_flag &
					    DEBUG_FLAG_OMX_DV_DROP_FRAME)
						pr_info("dolby vision drop done\n");
					break;
				}
				break;
			}
			break;
		}
	}
#endif
	if (omx_need_drop_frame_num > 0 && !omx_drop_done && omx_secret_mode) {
		struct vframe_s *vf = NULL;

		while (1) {
			vf = amvideo_vf_peek();
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
			if (is_amdv_enable() &&
			    vf && is_amdv_frame(vf)) {
				break;
			}
#endif
			if (!vf)
				break;
			if (omx_need_drop_frame_num >= vf->omx_index) {
				vf = amvideo_vf_get();
				if (amvideo_vf_put(vf) < 0)
					check_dispbuf(0, vf, true);
				video_drop_vf_cnt[0]++;
				if (debug_flag & DEBUG_FLAG_PRINT_DROP_FRAME)
					pr_info("drop frame: drop count %d\n",
						video_drop_vf_cnt[0]);
			} else {
				omx_drop_done = true;
				break;
			}
		}
	}
}

void pts_process(void)
{
	if (vsync_pts_inc_upint) {
		if (vsync_pts_inc_adj) {
			/* pr_info("adj %d, org %d\n",*/
			/*vsync_pts_inc_adj, vsync_pts_inc); */
			timestamp_pcrscr_inc(vsync_pts_inc_adj);
			timestamp_apts_inc(vsync_pts_inc_adj);
#ifdef CONFIG_AMLOGIC_VIDEOSYNC
			videosync_pcrscr_inc(vsync_pts_inc_adj);
#endif
		} else {
			timestamp_pcrscr_inc(vsync_pts_inc + 1);
			timestamp_apts_inc(vsync_pts_inc + 1);
#ifdef CONFIG_AMLOGIC_VIDEOSYNC
			videosync_pcrscr_inc(vsync_pts_inc + 1);
#endif
		}
	} else {
		if (vsync_slow_factor == 0) {
			pr_info("invalid vsync_slow_factor, set to 1\n");
			vsync_slow_factor = 1;
		}

		if (vsync_slow_factor == 1) {
			timestamp_pcrscr_inc_scale(vsync_pts_inc_scale,
						   vsync_pts_inc_scale_base);
			timestamp_apts_inc(vsync_pts_inc / vsync_slow_factor);
#ifdef CONFIG_AMLOGIC_VIDEOSYNC
			videosync_pcrscr_update(vsync_pts_inc_scale,
						vsync_pts_inc_scale_base);
#endif

		} else if (vsync_slow_factor > 1000) {
			u32 inc = (vsync_slow_factor / 1000)
				* vsync_pts_inc / 1000;

			timestamp_pcrscr_inc(inc);
			timestamp_apts_inc(inc);
#ifdef CONFIG_AMLOGIC_VIDEOSYNC
			videosync_pcrscr_inc(inc);
#endif
		} else {
			timestamp_pcrscr_inc(vsync_pts_inc / vsync_slow_factor);
			timestamp_apts_inc(vsync_pts_inc / vsync_slow_factor);
#ifdef CONFIG_AMLOGIC_VIDEOSYNC
			videosync_pcrscr_inc(vsync_pts_inc / vsync_slow_factor);
#endif
		}
	}
	if (omx_secret_mode) {
		u32 system_time = timestamp_pcrscr_get();
		int diff = 0;
		unsigned long delta1 = 0;
		unsigned long time_setomxpts_delta = 0;

		diff = system_time - omx_pts;
		if (time_setomxpts > 0 && time_setomxpts_last > 0) {
			/* time_setomxpts record hwc setomxpts time, */
			/* when check  diff between pcr and  omx_pts, */
			/* add compensation will let omx_pts and pcr */
			/* is at the some time, more accurate. Also */
			/* remove the compensation when omx_pts */
			/* is not update for a while, in case when */
			/* paused, pcr is not paused */
			delta1 = func_div(sched_clock() - time_setomxpts, 1000);
			time_setomxpts_delta = func_div(time_setomxpts -
				time_setomxpts_last, 1000);
			if ((time_setomxpts_delta >
				((ulong)(4 * vsync_pts_inc) * 1000 / 90)) ||
				((diff - omx_pts_interval_upper * 3 / 2) > 0) ||
				((diff - omx_pts_interval_lower * 3 / 2)
				< 0)) {
				time_setomxpts = 0;
				time_setomxpts_last = 0;
				if (debug_flag & DEBUG_FLAG_PTS_TRACE)
					pr_info("omxpts is not update for a while,do not need compensate\n");
			} else {
				diff -=  delta1 * 90 / 1000;
			}
		}

		if (((diff - omx_pts_interval_upper) > 0 ||
		     (diff - omx_pts_interval_lower) < 0 ||
		    omx_pts_set_from_hwc_count <
		    OMX_MAX_COUNT_RESET_SYSTEMTIME) &&
		    video_start_post) {
			timestamp_pcrscr_enable(1);
			if (debug_flag & DEBUG_FLAG_PTS_TRACE)
				pr_info("system_time=%d, omx_pts=%d, diff=%d\n",
					system_time, omx_pts, diff);
			/*add  greatest common divisor of duration*/
			/*1500(60fps) 3000(30fps) 3750(24fps) for some video*/
			/*that pts is not evenly*/
			if (debug_flag & DEBUG_FLAG_OMX_DEBUG_DROP_FRAME) {
				pr_info("pcrscr_set sys_time=%d, omx_pts=%d, diff=%d",
					system_time, omx_pts, diff);
			}
			timestamp_pcrscr_set(omx_pts + DURATION_GCD);
		} else if (((diff - omx_pts_interval_upper / 2) > 0 ||
			(diff - omx_pts_interval_lower / 2) < 0) &&
			(omx_pts_set_from_hwc_count_begin <
			OMX_MAX_COUNT_RESET_SYSTEMTIME_BEGIN) &&
			video_start_post) {
			timestamp_pcrscr_enable(1);
			if (debug_flag & DEBUG_FLAG_PTS_TRACE)
				pr_info("begin-system_time=%d, omx_pts=%d, diff=%d\n",
					system_time, omx_pts, diff);
			timestamp_pcrscr_set(omx_pts + DURATION_GCD);
		} else if (is_amdv_enable() &&
			((diff - omx_pts_dv_upper) > 0 ||
			(diff - omx_pts_dv_lower) < 0) &&
			video_start_post) {
			timestamp_pcrscr_set(omx_pts + DURATION_GCD);
		}
	} else {
		omx_pts = 0;
	}
	if (trickmode_duration_count > 0)
		trickmode_duration_count -= vsync_pts_inc;
#ifdef VIDEO_PTS_CHASE
	if (vpts_chase)
		vpts_chase_counter--;
#endif

	if (slowsync_repeat_enable)
		frame_repeat_count++;

	if (smooth_sync_enable) {
		if (video_frame_repeat_count)
			video_frame_repeat_count++;
	}
}

#if ENABLE_UPDATE_HDR_FROM_USER
static void init_hdr_info(void)
{
	unsigned long flags;

	spin_lock_irqsave(&omx_hdr_lock, flags);

	has_hdr_info = false;
	memset(&vf_hdr, 0, sizeof(vf_hdr));

	spin_unlock_irqrestore(&omx_hdr_lock, flags);
}

static void set_hdr_to_frame(struct vframe_s *vf)
{
	unsigned long flags;

	spin_lock_irqsave(&omx_hdr_lock, flags);

	if (has_hdr_info) {
		vf->prop.master_display_colour = vf_hdr;

		//config static signal_type for vp9
		vf->signal_type = (1 << 29)
			| (5 << 26) /* unspecified */
			| (0 << 25) /* limit */
			| (1 << 24) /* color available */
			| (9 << 16) /* 2020 */
			| (16 << 8) /* 2084 */
			| (9 << 0); /* 2020 */

		//pr_info("set_hdr_to_frame %d, signal_type 0x%x",
		//vf->prop.master_display_colour.present_flag,vf->signal_type);
	}
	spin_unlock_irqrestore(&omx_hdr_lock, flags);
}

static void config_hdr_info(const struct vframe_master_display_colour_s p)
{
	struct vframe_master_display_colour_s tmp = {0};
	bool valid_hdr = false;
	unsigned long flags;

	tmp.present_flag = p.present_flag;
	if (tmp.present_flag == 1) {
		tmp = p;

		if (tmp.primaries[0][0] == 0 &&
		    tmp.primaries[0][1] == 0 &&
		    tmp.primaries[1][0] == 0 &&
		    tmp.primaries[1][1] == 0 &&
		    tmp.primaries[2][0] == 0 &&
		    tmp.primaries[2][1] == 0 &&
		    tmp.white_point[0] == 0 &&
		    tmp.white_point[1] == 0 &&
		    tmp.luminance[0] == 0 &&
		    tmp.luminance[1] == 0 &&
		    tmp.content_light_level.max_content == 0 &&
		    tmp.content_light_level.max_pic_average == 0) {
			valid_hdr = false;
		} else {
			valid_hdr = true;
		}
	}

	spin_lock_irqsave(&omx_hdr_lock, flags);
	vf_hdr = tmp;
	has_hdr_info = valid_hdr;
	spin_unlock_irqrestore(&omx_hdr_lock, flags);

	pr_debug("has_hdr_info %d\n", has_hdr_info);
}
#endif

static struct vframe_s *vsync_toggle_frame(struct vframe_s *vf, int line)
{
	static u32 last_pts;
	u32 diff_pts;
	u32 first_picture = 0;
	long long *clk_array;
	static long long clock_vdin_last;
	static long long clock_last;
	u8 layer_id = 0;

	ATRACE_COUNTER(__func__,  line);
	if (!vf)
		return NULL;
	ATRACE_COUNTER("vsync_toggle_frame_pts", vf->pts);

	diff_pts = vf->pts - last_pts;
	if (last_pts && diff_pts < 90000)
		ATRACE_COUNTER("vsync_toggle_frame_inc", diff_pts);
	else
		ATRACE_COUNTER("vsync_toggle_frame_inc", 0);  /* discontinue */

	last_pts = vf->pts;

	frame_count++;
	toggle_count++;

#ifdef PTS_TRACE_DEBUG
#ifdef PTS_TRACE_START
		if (pts_trace_his_rd < 16) {
#endif
			pts_trace_his[pts_trace_his_rd] = pts_trace;
			pts_his[pts_trace_his_rd] = vf->pts;
			scr_his[pts_trace_his_rd] = timestamp_pcrscr_get();
			pts_trace_his_rd++;
			if (pts_trace_his_rd >= 16)
				pts_trace_his_rd = 0;
#ifdef PTS_TRACE_START
		}
#endif
#endif

#ifdef PTS_LOGGING
	if (pts_escape_vsync == 1) {
		pts_trace++;
		pts_escape_vsync = 0;
	}
	vsync_video_pattern();
	pre_pts_trace = pts_trace;
#endif

#if defined(PTS_LOGGING) || defined(PTS_TRACE_DEBUG)
	pts_trace = 0;
#endif

	if (debug_flag & DEBUG_FLAG_PRINT_TOGGLE_FRAME) {
		u32 pcr = timestamp_pcrscr_get();
		u32 vpts = timestamp_vpts_get();
		u32 apts = timestamp_apts_get();

		pr_info("%s pts:%d.%06d pcr:%d.%06d vpts:%d.%06d apts:%d.%06d disp:%d\n",
			__func__, (vf->pts) / 90000,
			((vf->pts) % 90000) * 1000 / 90, (pcr) / 90000,
			((pcr) % 90000) * 1000 / 90, (vpts) / 90000,
			((vpts) % 90000) * 1000 / 90, (apts) / 90000,
			((apts) % 90000) * 1000 / 90, vf->index_disp);
	}

	if (trickmode_i || trickmode_fffb)
		trickmode_duration_count = trickmode_duration;

#ifdef OLD_DI
	if (vf->early_process_fun) {
		if (vf->early_process_fun(vf->private_data, vf) == 1)
			first_picture = 1;
	} else {
#ifdef CONFIG_AMLOGIC_MEDIA_DEINTERLACE
		if ((DI_POST_REG_RD(DI_IF1_GEN_REG) & 0x1) != 0) {
			/* check mif enable status, disable post di */
			cur_dev->rdma_func[vpp_index].rdma_wr(DI_POST_CTRL, 0x3 << 30);
			cur_dev->rdma_func[vpp_index].rdma_wr(DI_POST_SIZE,
					  (32 - 1) | ((128 - 1) << 16));
			cur_dev->rdma_func[vpp_index].rdma_wr(DI_IF1_GEN_REG,
					  READ_VCBUS_REG(DI_IF1_GEN_REG) &
					  0xfffffffe);
		}
#endif
	}
#endif

	timer_count = 0;
	if (vf->width == 0 && vf->height == 0) {
		amlog_level
			(LOG_LEVEL_ERROR,
			"Video: invalid frame dimension\n");
		ATRACE_COUNTER(__func__,  __LINE__);
		return vf;
	}

	if (hold_video) {
		if (cur_dispbuf[0] != vf) {
			u32 old_w, old_h;

			new_frame_count++;
#ifdef CONFIG_AMLOGIC_MEDIA_FRAME_SYNC
			if (vf->pts != 0) {
				amlog_mask
					(LOG_MASK_TIMESTAMP,
					"vpts to: 0x%x, scr: 0x%x, abs_scr: 0x%x\n",
					vf->pts, timestamp_pcrscr_get(),
					READ_MPEG_REG(SCR_HIU));

				timestamp_vpts_set(vf->pts);
				timestamp_vpts_set_u64(vf->pts_us64);
				last_frame_duration = vf->duration;
			} else if (last_frame_duration) {
				amlog_mask
					(LOG_MASK_TIMESTAMP,
					"vpts inc: 0x%x, scr: 0x%x, abs_scr: 0x%x\n",
					timestamp_vpts_get() +
					vf_get_pts(cur_dispbuf[0]),
					timestamp_pcrscr_get(),
					READ_MPEG_REG(SCR_HIU));

				timestamp_vpts_inc
					(DUR2PTS(last_frame_duration));
				timestamp_vpts_inc_u64
					(DUR2PTS(last_frame_duration));

				vpts_remainder +=
					DUR2PTS_RM(last_frame_duration);
				if (vpts_remainder >= 0xf) {
					vpts_remainder -= 0xf;
					timestamp_vpts_inc(-1);
					timestamp_vpts_inc_u64(-1);
				}
			}
#endif

			old_w = cur_width;
			old_h = cur_height;
			if (vf->type & VIDTYPE_COMPRESS) {
				cur_width = vf->compWidth;
				cur_height = vf->compHeight;
			} else {
				cur_width = vf->width;
				cur_height = vf->height;
			}
			if (old_w != cur_width ||
			    old_h != cur_height)
				video_prop_status |= VIDEO_PROP_CHANGE_SIZE;
			if (amvideo_vf_put(vf) < 0)
				check_dispbuf(layer_id, vf, true);
			ATRACE_COUNTER(__func__,  __LINE__);
			return NULL;
		}
	}

	if (cur_dispbuf[0] != vf) {
		new_frame_count++;
		if (new_frame_count == 1)
			first_picture = 1;
	}

	if (cur_dispbuf[0] &&
	    cur_dispbuf[0] != &vf_local[0] &&
	    cur_dispbuf[0] != vf) {
#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
		int i = 0, done = false;

		if (is_vsync_rdma_enable()) {
			if (cur_rdma_buf[0] == cur_dispbuf[0]) {
				done = check_dispbuf(layer_id, cur_dispbuf[0], false);
				if (!done) {
					if (amvideo_vf_put(cur_dispbuf[0]) < 0)
						pr_info("put err,line: %d\n",
							__LINE__);
				}
			} else {
				if (amvideo_vf_put(cur_dispbuf[0]) < 0) {
					done = check_dispbuf(layer_id, cur_dispbuf[0], true);
					if (!done)
						pr_info("put err,que full\n");
				}
			}
		} else {
			for (i = 0; i < DISPBUF_TO_PUT_MAX; i++) {
				if (dispbuf_to_put[layer_id][i]) {
					if (!amvideo_vf_put(dispbuf_to_put[layer_id][i])) {
						dispbuf_to_put[layer_id][i] = NULL;
						dispbuf_to_put_num[layer_id]--;
					}
				}
			}
			if (amvideo_vf_put(cur_dispbuf[0]) < 0)
				check_dispbuf(layer_id, cur_dispbuf[0], true);
		}
#else
		if (amvideo_vf_put(cur_dispbuf[0]) < 0)
			check_dispbuf(layer_id, cur_dispbuf[0], true);
#endif
		if (debug_flag & DEBUG_FLAG_LATENCY) {
			vf->ready_clock[3] = sched_clock();
			pr_info("video toggle latency %lld us, diff %lld, get latency %lld us, vdin put latency %lld us, first %lld us, diff %lld.\n",
				func_div(vf->ready_clock[3], 1000),
				func_div(vf->ready_clock[3] - clock_last, 1000),
				func_div(vf->ready_clock[2], 1000),
				func_div(vf->ready_clock[1], 1000),
				func_div(vf->ready_clock[0], 1000),
				func_div(vf->ready_clock[0] -
				clock_vdin_last, 1000));

			clock_vdin_last = vf->ready_clock[0];
			clock_last = vf->ready_clock[3];
			cur_dispbuf[0]->ready_clock[4] = sched_clock();
			clk_array = cur_dispbuf[0]->ready_clock;
			pr_info("video put latency %lld us, video toggle latency %lld us, video get latency %lld us, vdin put latency %lld us, first %lld us.\n",
				func_div(*(clk_array + 4), 1000),
				func_div(*(clk_array + 3), 1000),
				func_div(*(clk_array + 2), 1000),
				func_div(*(clk_array + 1), 1000),
				func_div(*clk_array, 1000));
		}
	} else {
		first_picture = 1;
	}

	if ((debug_flag & DEBUG_FLAG_BASIC_INFO) && first_picture)
		pr_info("[vpp-kpi] first toggle picture {%d,%d} pts:%x\n",
			vf->width, vf->height, vf->pts);
	vframe_walk_delay = (int)div_u64(((jiffies_64 -
		vf->ready_jiffies64) * 1000), HZ);

	if (debug_flag & DEBUG_FLAG_HDMI_AVSYNC_DEBUG)
		pr_info("toggle vf %p, ready_jiffies64 %d, walk_delay %d\n",
			vf, jiffies_to_msecs(vf->ready_jiffies64),
			vframe_walk_delay);

	/* set video PTS */
	if (cur_dispbuf[0] != vf) {
#ifdef CONFIG_AMLOGIC_MEDIA_FRAME_SYNC
		if (vf->pts != 0) {
			amlog_mask
				(LOG_MASK_TIMESTAMP,
				"vpts to: 0x%x, scr: 0x%x, abs_scr: 0x%x\n",
				vf->pts, timestamp_pcrscr_get(),
				READ_MPEG_REG(SCR_HIU));
				timestamp_vpts_set(vf->pts);
				timestamp_vpts_set_u64(vf->pts_us64);
		} else if (cur_dispbuf[0]) {
			amlog_mask
				(LOG_MASK_TIMESTAMP,
				"vpts inc: 0x%x, scr: 0x%x, abs_scr: 0x%x\n",
				timestamp_vpts_get() +
				vf_get_pts(cur_dispbuf[0]),
				timestamp_pcrscr_get(),
				READ_MPEG_REG(SCR_HIU));

			timestamp_vpts_inc
				(vf_get_pts(cur_dispbuf[0]));
			timestamp_vpts_inc_u64
				(vf_get_pts(cur_dispbuf[0]));

			vpts_remainder +=
				vf_get_pts_rm(cur_dispbuf[0]);
			if (vpts_remainder >= 0xf) {
				vpts_remainder -= 0xf;
				timestamp_vpts_inc(-1);
				timestamp_vpts_inc_u64(-1);
			}
		}
#endif
		vf->type_backup = vf->type;
	}

	cur_dispbuf[0] = vf;

	if (cur_dispbuf[0] && omx_secret_mode)
		cur_disp_omx_index = cur_dispbuf[0]->omx_index;

	if (first_picture) {
		first_frame_toggled = 1;
#ifdef VIDEO_PTS_CHASE
		av_sync_flag = 0;
#endif
	}

	if (vf != &vf_local[0] && vf && !vsync_pts_aligned) {
#ifdef PTS_TRACE_DEBUG
		pr_info("####timestamp_pcrscr_get() = 0x%x, vf->pts = 0x%x, vsync_pts_inc = %d\n",
			timestamp_pcrscr_get(), vf->pts, vsync_pts_inc);
#endif
#ifdef CONFIG_AMLOGIC_MEDIA_FRAME_SYNC
		if ((abs(timestamp_pcrscr_get() - vf->pts) <= vsync_pts_inc) &&
		    ((int)(timestamp_pcrscr_get() - vf->pts) >= 0)) {
			vsync_pts_align =  vsync_pts_inc / 4 -
				(timestamp_pcrscr_get() - vf->pts);
			vsync_pts_aligned = true;
#ifdef PTS_TRACE_DEBUG
			pts_trace_his_rd = 0;
			pr_info("####vsync_pts_align set to %d\n",
				vsync_pts_align);
#endif
		}
#endif
	}
	ATRACE_COUNTER(__func__,  0);
	return cur_dispbuf[0];
}

static struct vframe_s *save_toggle_frame(struct vframe_s *vf)
{
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
	if (get_top1_onoff()) {
		if (video_save.save_vf_en && video_save.save_vf) {
			/* need toggle */
			video_save.toggle_vf = video_save.save_vf;
			video_save.save_vf = vf;
			vf = video_save.toggle_vf;
		} else {
			/* save frame, not toggle */
			video_save.save_vf = vf;
			video_save.toggle_vf = NULL;
			vf = NULL;
			video_save.save_vf_en = true;
		}
		if (debug_flag & DEBUG_FLAG_PRINT_FRAME_DETAIL)
			pr_info("%s: save_vf_en=%d, vf=%p, save_vf=%p, toggle_vf=%p\n",
				__func__,
				video_save.save_vf_en, vf,
				video_save.save_vf ?
				video_save.save_vf : NULL,
				video_save.toggle_vf ?
				video_save.toggle_vf : NULL);
	} else {
		video_save.toggle_vf = vf;
		if (video_save.save_vf) {
			if (!amvideo_vf_put(video_save.save_vf))
				video_save.save_vf = NULL;
		}
		video_save.save_vf_en = false;
	}
	vf = video_save.toggle_vf;
#endif
	return vf;
}

struct vframe_s *amvideo_toggle_frame(s32 *vd_path_id)
{
	struct vframe_s *path0_new_frame = NULL;
	struct vframe_s *vf;
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
	struct vframe_s *dv_new_vf = NULL;
	struct vframe_s *vf_top1 = NULL;
	int ret;
	static bool force_top1_once;
#endif
	struct vframe_s *cur_dispbuf_back = cur_dispbuf[0];
	int toggle_cnt;
	bool show_nosync = false;
	struct cur_line_info_t *cur_line_info = get_cur_line_info(0);

	toggle_cnt = 0;

	if (!cur_dispbuf[0] || cur_dispbuf[0] == &vf_local[0]) {
		vf = amvideo_vf_peek();
		if (vf) {
			if (hdmi_in_onvideo == 0) {
				if (!nopostvideostart) {
					if (vf_source_from_vdin(vf)) {
						tsync_set_enable(0);
						tsync_set_mode(TSYNC_MODE_VMASTER);
					}
					tsync_avevent_locked
						(VIDEO_START,
						(vf->pts) ? vf->pts :
						timestamp_vpts_get());
				}
				video_start_post = true;
			}

			if (show_first_frame_nosync || show_first_picture)
				show_nosync = true;

			if (slowsync_repeat_enable)
				frame_repeat_count = 0;
		} else {
			goto SET_FILTER;
		}
	}

	vf = amvideo_vf_peek();
	/*process hdmi in video sync*/
	if (vf) {
		/*step 1: audio required*/
		if (hdmin_delay_start > 0) {
			process_hdmi_video_sync(vf);
			hdmin_delay_start = 0;
			hdmi_vframe_count = 0;
		}
		/*step 2: recheck video sync after hdmi-in start*/
		if (hdmi_delay_first_check) {
			hdmi_vframe_count++;
			if (hdmi_vframe_count > HDMI_DELAY_FIRST_CHECK_COUNT) {
				process_hdmi_video_sync(vf);
				hdmi_vframe_count = 0;
				hdmi_delay_first_check = false;
			}
		/*step 3: re-check video sync every 5s by n times */
		} else if (hdmi_delay_normal_check) {
			hdmi_vframe_count++;
			if (hdmi_vframe_count > HDMI_DELAY_NORMAL_CHECK_COUNT) {
				process_hdmi_video_sync(vf);
				hdmi_vframe_count = 0;
			}
		}

		/* HDMI-IN AV SYNC Control, delay video*/
		if (!hdmin_delay_done) {
			if (hdmi_in_delay_check(vf) > 0) {
				go_exit = 1;
				return NULL;
			}
		}
		/*HDMI-IN AV SYNC Control, drop video*/
		if (hdmin_need_drop_count > 0)
			hdmi_in_drop_frame();
	}

	/* buffer switch management */
	vf = amvideo_vf_peek();

	/*debug info for skip & repeate vframe case*/
	if (!vf) {
		underflow++;
		/* video master mode, reduce pcrscr */
		if (tsync_get_mode() == TSYNC_MODE_VMASTER) {
			s32 pts_inc = 0 - vsync_pts_inc;

			timestamp_pcrscr_inc(pts_inc);
		}
		ATRACE_COUNTER("underflow",  1);
		if (video_dbg_vf & (1 << 0))
			dump_vframe_status("vdin0");
		if (video_dbg_vf & (1 << 1))
			dump_vframe_status("deinterlace");
		if (video_dbg_vf & (1 << 2))
			dump_vframe_status("amlvideo2");
		if (video_dbg_vf & (1 << 3))
			dump_vframe_status("ppmgr");
		if (video_dbg_vf & (1 << 4))
			dump_vdin_reg();
	} else {
		ATRACE_COUNTER("underflow",  0);
	}

	video_get_vf_cnt[0] = 0;

	while (vf && !video_suspend) {
		if (debug_flag & DEBUG_FLAG_OMX_DEBUG_DROP_FRAME) {
			pr_info("next pts= %d,index %d,pcr = %d,vpts = %d\n",
				vf->pts, vf->omx_index,
				timestamp_pcrscr_get(), timestamp_vpts_get());
		}
		if ((omx_continuous_drop_flag && omx_run)) {
			if (is_amdv_enable() && vf &&
			    is_amdv_frame(vf)) {
				if (debug_flag & DEBUG_FLAG_OMX_DV_DROP_FRAME)
					pr_info("dovi ignore continuous drop\n");
				/* if (omx_run)
				 *	dolby_vision_drop_frame();
				 */
			} else {
				if (debug_flag &
					DEBUG_FLAG_OMX_DEBUG_DROP_FRAME) {
					pr_info("drop omx_index %d, pts %d\n",
						vf->omx_index, vf->pts);
				}
				vf = vf_get(RECEIVER_NAME);
				if (vf) {
					vf_put(vf, RECEIVER_NAME);
					video_drop_vf_cnt[0]++;
					if (debug_flag &
					    DEBUG_FLAG_PRINT_DROP_FRAME)
						pr_info("drop frame: drop count %d\n",
							video_drop_vf_cnt[0]);
				}
				vf = amvideo_vf_peek();
				continue;
			}
		}

		if (vpts_expire(cur_dispbuf[0], vf, toggle_cnt) || show_nosync) {
#if defined(CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_VECM)
			int iret1 = 0, iret2 = 0, iret3 = 0;
#endif

			ATRACE_COUNTER(MODULE_NAME,  __LINE__);
			if (debug_flag & DEBUG_FLAG_PTS_TRACE) {
				pr_info("vpts = 0x%x, c.dur=0x%x, n.pts=0x%x, scr = 0x%x, pcr-pts-diff=%d, ptstrace=%d\n",
					timestamp_vpts_get(),
					(cur_dispbuf[0]) ?
					cur_dispbuf[0]->duration : 0,
					vf->pts, timestamp_pcrscr_get(),
					timestamp_pcrscr_get() - vf->pts +
					vsync_pts_align,
					pts_trace);
				if (pts_trace > 4)
					pr_info("smooth trace:%d\n", pts_trace);
			}
			amlog_mask_if(toggle_cnt > 0, LOG_MASK_FRAMESKIP,
				      "skipped\n");
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
			ret = dolby_vision_need_wait(0);
			if (((vd_path_id[0] == VFM_PATH_AMVIDEO ||
			     vd_path_id[0] == VFM_PATH_DEF ||
			     vd_path_id[0] == VFM_PATH_AUTO) && ret) ||
			     force_top1_once) {
			     /*first frame, only proc top1*/
				if (ret == 4 || force_top1_once) {
					if (debug_flag & DEBUG_FLAG_OMX_DV_DROP_FRAME)
						pr_info("first frame for top1\n");
					amdv_parse_metadata_hw5_top1(vf);
					amdolby_vision_process_hw5(vf, NULL,
						vf->compWidth << 16 | vf->compHeight, 1, 0);
					force_top1_once = false;
				}
				break;/*wait metadata or step or top1*/
			}
#endif
#if ENABLE_UPDATE_HDR_FROM_USER
			set_hdr_to_frame(vf);
#endif
			/*
			 *two special case:
			 *case1:4k display case,input buffer not enough &
			 *	quickly for display
			 *case2:input buffer all not OK
			 */
			if (vf &&
			    (vf->source_type == VFRAME_SOURCE_TYPE_HDMI ||
			     vf->source_type == VFRAME_SOURCE_TYPE_CVBS) &&
			    (video_vf_disp_mode_get(vf) ==
			     VFRAME_DISP_MODE_UNKNOWN) &&
			    (frame_skip_check_cnt++ < 10))
				break;
			frame_skip_check_cnt = 0;
#if defined(CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_VECM)
			if (vd_path_id[0] == VFM_PATH_AMVIDEO ||
			    vd_path_id[0] == VFM_PATH_DEF)
				iret1 = amvecm_update(VD1_PATH, 0, vf);
			if (vd_path_id[1] == VFM_PATH_AMVIDEO)
				iret2 = amvecm_update(VD2_PATH, 0, vf);
			if (vd_path_id[2] == VFM_PATH_AMVIDEO)
				iret3 = amvecm_update(VD3_PATH, 0, vf);
			if (iret1 == 1 || iret2 == 1 || iret3 == 1)
				break;
#endif
			if (performance_debug & DEBUG_FLAG_VSYNC_PROCESS_TIME)
				do_gettimeofday(&cur_line_info->end2);

			vf = amvideo_vf_get();
			if (!vf) {
				ATRACE_COUNTER(MODULE_NAME,  __LINE__);
				break;
			}
			if (debug_flag & DEBUG_FLAG_LATENCY) {
				vf->ready_clock[2] = sched_clock();
				pr_info("video get latency %lld ms vdin put latency %lld ms. first %lld ms.\n",
					func_div(vf->ready_clock[2], 1000),
					func_div(vf->ready_clock[1], 1000),
					func_div(vf->ready_clock[0], 1000));
			}
			if (video_vf_dirty_put(vf)) {
				ATRACE_COUNTER(MODULE_NAME,  __LINE__);
				break;
			}
			if (vf &&
			    (vf->source_type == VFRAME_SOURCE_TYPE_HDMI ||
			     vf->source_type == VFRAME_SOURCE_TYPE_CVBS) &&
			    video_vf_disp_mode_check(vf)) {
				vdin_frame_skip_cnt++;
				break;
			}
			force_blackout = 0;
			if (vf) {
				if (last_mode_3d !=
					vf->mode_3d_enable) {
					last_mode_3d =
						vf->mode_3d_enable;
				}
#ifdef CONFIG_AMLOGIC_MEDIA_TVIN
				video_3d_format = vf->trans_fmt;
#endif
			}
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
			/*check metadata crc*/
			if (vf &&
			    (vf->source_type == VFRAME_SOURCE_TYPE_HDMI ||
			    vf->source_type == VFRAME_SOURCE_TYPE_CVBS) &&
				dv_vf_crc_check(vf)) {
				break; // not render err crc frame
			}
			/*top1 enable, need check one more frame*/
			if (is_amdv_enable() && get_top1_onoff()) {/*todo*/
				vf_top1 = amvideo_vf_peek();
				/*wait next new Fn+1 for top1, proc top2 Fn + top1 Fn+1*/
				/*if no new frame, proc top2 Fn + repeat Top1 Fn cur vsync*/
				/*then only do top1 Fn+1 next vsync*/
				if (!vf_top1) {
					if (debug_flag & DEBUG_FLAG_OMX_DV_DROP_FRAME)
						pr_info("wait new frame for top1\n");
					//force_top1_once = true;
					force_top1_once = false;
					vf_top1 = vf;/*temporarily use cur vf for top1, no wait*/
				} else {
					force_top1_once = false;
				}
			}
#endif
			path0_new_frame = vsync_toggle_frame(vf, __LINE__);
			/* The v4l2 capture needs a empty vframe to flush */
			if (has_receive_dummy_vframe())
				break;

			if (performance_debug & DEBUG_FLAG_VSYNC_PROCESS_TIME)
				do_gettimeofday(&cur_line_info->end3);

#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
			if (vd_path_id[0] == VFM_PATH_AMVIDEO ||
			    vd_path_id[0] == VFM_PATH_DEF ||
				vd_path_id[0] == VFM_PATH_AUTO) {
				if (!get_top1_onoff() || !vf_top1) {/*no top1*/
					dv_new_vf = dv_toggle_frame(vf, VD1_PATH, true);
				} else if (vf_top1) {/*top1 + top2*/
					amdv_parse_metadata_hw5_top1(vf_top1);
					dv_new_vf = dv_toggle_frame(vf, VD1_PATH, true);
				}
			}
			vd_layer[0].vf_top1 = vf_top1;

			if (hold_video)
				dv_new_vf = NULL;
#endif
			if (performance_debug & DEBUG_FLAG_VSYNC_PROCESS_TIME)
				do_gettimeofday(&cur_line_info->end4);

			if (trickmode_fffb == 1) {
				trickmode_vpts = vf->pts;
#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
				/* FIXME: replace is_di_on */
				if (is_di_post_mode(vf)) {
					atomic_set(&trickmode_framedone, 1);
					video_notify_flag |=
					    VIDEO_NOTIFY_TRICK_WAIT;
				} else {
					to_notify_trick_wait = true;
				}
#else
				atomic_set(&trickmode_framedone, 1);
				video_notify_flag |= VIDEO_NOTIFY_TRICK_WAIT;
#endif
				break;
			}
			if (slowsync_repeat_enable)
				frame_repeat_count = 0;
			vf = amvideo_vf_peek();
			if (vf) {
				if ((vf->flag & VFRAME_FLAG_VIDEO_COMPOSER) &&
				    (debug_flag &
				     DEBUG_FLAG_COMPOSER_NO_DROP_FRAME)) {
					pr_info("composer not drop\n");
					vf = NULL;
				}
			}

			if (!vf)
				next_peek_underflow++;
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
			if ((vd_path_id[0] == VFM_PATH_AMVIDEO ||
			     vd_path_id[0] == VFM_PATH_DEF ||
			     vd_path_id[0] == VFM_PATH_AUTO) &&
			    for_amdv_certification() &&
			    dv_new_vf)
				break;
#endif
			if (debug_flag & DEBUG_FLAG_TOGGLE_FRAME_PER_VSYNC)
				break;
			video_get_vf_cnt[0]++;
			if (video_get_vf_cnt[0] >= 2) {
				video_drop_vf_cnt[0]++;
				if (debug_flag & DEBUG_FLAG_PRINT_DROP_FRAME)
					pr_info("drop frame: drop count %d\n",
						video_drop_vf_cnt[0]);
			}
		} else {
			ATRACE_COUNTER(MODULE_NAME,  __LINE__);
			/* check if current frame's duration has expired,
			 *in this example
			 * it compares current frame display duration
			 * with 1/1/1/1.5 frame duration
			 * every 4 frames there will be one frame play
			 * longer than usual.
			 * you can adjust this array for any slow sync
			 * control as you want.
			 * The playback can be smoother than previous method.
			 */
			if (slowsync_repeat_enable) {
				ATRACE_COUNTER(MODULE_NAME,  __LINE__);
				if (duration_expire
					(cur_dispbuf[0], vf,
					frame_repeat_count
					* vsync_pts_inc) &&
					timestamp_pcrscr_enable_state()) {
#if defined(CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_VECM)
					int iret1 = 0, iret2 = 0, iret3 = 0;
#endif

					amlog_mask(LOG_MASK_SLOWSYNC,
						   "slow sync toggle,repeat_count = %d\n",
					frame_repeat_count);
					amlog_mask(LOG_MASK_SLOWSYNC,
						   "sys.time = 0x%x, video time = 0x%x\n",
					timestamp_pcrscr_get(),
					timestamp_vpts_get());
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
					if ((vd_path_id[0] == VFM_PATH_AMVIDEO ||
					     vd_path_id[0] == VFM_PATH_DEF ||
					     vd_path_id[0] == VFM_PATH_AUTO) &&
					    dolby_vision_need_wait(0))
						break;
#endif

#if defined(CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_VECM)
					if (vd_path_id[0] == VFM_PATH_AMVIDEO ||
						vd_path_id[0] == VFM_PATH_DEF)
						iret1 = amvecm_update(VD1_PATH, 0, vf);
					if (vd_path_id[1] == VFM_PATH_AMVIDEO)
						iret2 = amvecm_update(VD2_PATH, 0, vf);
					if (vd_path_id[2] == VFM_PATH_AMVIDEO)
						iret3 = amvecm_update(VD3_PATH, 0, vf);
					if (iret1 == 1 || iret2 == 1 || iret3 == 1)
						break;
#endif
					vf = amvideo_vf_get();
					if (!vf) {
						ATRACE_COUNTER(MODULE_NAME,
							       __LINE__);
						break;
					}
					vf = save_toggle_frame(vf);
					path0_new_frame = vsync_toggle_frame(vf, __LINE__);
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
					if (vd_path_id[0] == VFM_PATH_AMVIDEO ||
					    vd_path_id[0] == VFM_PATH_DEF ||
					    vd_path_id[0] == VFM_PATH_AUTO)
						dv_new_vf =
						dv_toggle_frame(vf, VD1_PATH, true);
					/*coverity[Unused value]*/
					if (hold_video)
						dv_new_vf = NULL;
#endif
					frame_repeat_count = 0;

					vf = amvideo_vf_peek();
				} else if ((cur_dispbuf[0]) &&
					(cur_dispbuf[0]->duration_pulldown >
					vsync_pts_inc)) {
					frame_count++;
					cur_dispbuf[0]->duration_pulldown -=
					    PTS2DUR(vsync_pts_inc);
				}
			} else {
				if (cur_dispbuf[0] &&
				    cur_dispbuf[0]->duration_pulldown >
				     vsync_pts_inc) {
					frame_count++;
					cur_dispbuf[0]->duration_pulldown -=
					    PTS2DUR(vsync_pts_inc);
				}
			}
			break;
		}
		toggle_cnt++;
	}
#ifdef INTERLACE_FIELD_MATCH_PROCESS
	if (interlace_field_type_need_match(vout_type, vf)) {
		if (field_matching_count++ == FIELD_MATCH_THRESHOLD) {
			field_matching_count = 0;
			/* adjust system time to get one more field toggle */
			/* at next vsync to match field */
			timestamp_pcrscr_inc(vsync_pts_inc);
		}
	} else {
		field_matching_count = 0;
	}
#endif
SET_FILTER:
	/* toggle_3d_fa_frame*/
	/* determine the out frame is L or R or blank */
	judge_3d_fa_out_mode();

	if (cur_dispbuf_back != cur_dispbuf[0]) {
		display_frame_count++;
		drop_frame_count = receive_frame_count - display_frame_count;
	}
	return path0_new_frame;
}

#define VDIN_KEEP_COUNT 1
#define DI_KEEP_COUNT_P 1
#define DI_KEEP_COUNT_I 2
#define DIS_PATH_DELAY_COUNT 2
#define VDIN_BUF_COUNT 11
#define DI_MAX_OUT_COUNT 9
#define VLOCL_DELAY 7  /*vdin vsync before vpp vsync about 7ms*/

static void hdmi_in_delay_maxmin_reset(void)
{
	hdmin_dv_flag = 0;
	hdmin_delay_min_ms = 0;
	hdmin_delay_max_ms = 0;
}

void hdmi_in_delay_maxmin_old(struct vframe_s *vf)
{
	u64 vdin_vsync = 0;
	u64 vpp_vsync = 0;
	u32 vdin_count = 0;
	int di_keep_count = 0;
	u64 hdmin_delay_min = 0;
	u64 hdmin_delay_max = 0;
	int buf_cnt;
	struct vinfo_s *video_info;
	u64 memc_delay = 0;
	int vdin_keep_count = VDIN_KEEP_COUNT;
	u32 sync_count_pre = 0;

	if (vf->source_type != VFRAME_SOURCE_TYPE_HDMI &&
		vf->source_type != VFRAME_SOURCE_TYPE_CVBS &&
		vf->source_type != VFRAME_SOURCE_TYPE_TUNER)
		return;

	if (vf->type & VIDTYPE_DI_PW || vf->di_flag & DI_FLAG_DI_PVPPLINK) {
		if (debug_flag & DEBUG_FLAG_HDMI_AVSYNC_DEBUG)
			pr_info("%s: start check DI count.\n", __func__);
		if (vf->type_original & VIDTYPE_INTERLACE)
			di_keep_count = DI_KEEP_COUNT_I;
		else
			di_keep_count = DI_KEEP_COUNT_P;
	}

#ifdef CONFIG_AMLOGIC_MEDIA_VDIN
	vdin_keep_count += get_vdin_add_delay_num();
#endif

	video_info = get_current_vinfo();
	if (!video_info || video_info->mode == VMODE_INVALID)
		return;
	if (video_info->sync_duration_num > 0) {
		vpp_vsync = video_info->sync_duration_den;
		vpp_vsync = vpp_vsync * 1000000;
		vpp_vsync = div64_u64(vpp_vsync,
			video_info->sync_duration_num);
	}

	vdin_vsync = vf->duration;
	vdin_vsync = vdin_vsync * 1000;
	vdin_vsync = div64_u64(vdin_vsync, 96);

#ifdef CONFIG_AMLOGIC_MEDIA_FRC
	memc_delay = frc_get_video_latency();
#endif

	/*pre: vdin keep 1, di keep 1/2(one process,one for I frame), total 2/3
	 *rdma one vpp vsync, one for next vsync to peek
	 *if do di: count = (1 + 1/2) * vdin_vsync + vpp_vsync * 2;
	 *if no di: count = (1 + 0) * vdin_vsync + vpp_vsync * 2;
	 */

	if (debug_flag & DEBUG_FLAG_HDMI_AVSYNC_DEBUG)
		pr_info("%s: vdin:count=%d vsync=%lld, di:count=%d vsync=%lld.\n",
			__func__, vdin_keep_count, vdin_vsync, di_keep_count, vpp_vsync);

	sync_count_pre = vdin_keep_count + di_keep_count;
	if (sync_count_pre < 2) {
		if (debug_flag & DEBUG_FLAG_HDMI_AVSYNC_DEBUG)
			pr_info("%s: pre delay need at least 2 vsync.\n", __func__);
		sync_count_pre = 2;
	}
	hdmin_delay_min = sync_count_pre * vdin_vsync + vpp_vsync * 2;
	hdmin_delay_min_ms = div64_u64(hdmin_delay_min, 1000);
	hdmin_delay_min_ms += memc_delay;

	/*vdin total 10 buf, one for vdin next write, one is on display, 8 left
	 */
	buf_cnt = video_vdin_buf_info_get();
	if (buf_cnt <= 2)
		return;
	vdin_count = buf_cnt - 1 - 1;

	hdmin_delay_max = vdin_count * vdin_vsync;
	hdmin_delay_max_ms = div64_u64(hdmin_delay_max, 1000);
	hdmin_delay_max_ms += memc_delay;

	if (debug_flag & DEBUG_FLAG_HDMI_AVSYNC_DEBUG)
		pr_info("%s: range(%d, %d).\n", __func__, hdmin_delay_min_ms, hdmin_delay_max_ms);
}

void hdmi_in_delay_maxmin_new(struct vframe_s *vf)
{
	u64 vdin_vsync = 0;
	u64 vpp_vsync = 0;
	u32 vdin_count = 0;
	u32 vpp_count = 0;
	int di_keep_count = 0;
	bool do_di = false;
	bool di_has_vdin_vf = false;
	u64 hdmin_delay_min = 0;
	u64 hdmin_delay_max = 0;
	u64 memc_delay = 0;
	int vdin_keep_count = VDIN_KEEP_COUNT;
	unsigned int ret_hz = 0;
	u64 ext_delay = 0;
	u32 vdin_buf_count = 0;
	u32 dv_flag = 0;
	bool di_backend_en = false;
	int display_path_count = DIS_PATH_DELAY_COUNT;
	u32 sync_count_pre = 0;

	if (!tvin_delay_mode)
		return;

	if (vf->source_type != VFRAME_SOURCE_TYPE_HDMI &&
		vf->source_type != VFRAME_SOURCE_TYPE_CVBS &&
		vf->source_type != VFRAME_SOURCE_TYPE_TUNER)
		return;

	if (vf->type & VIDTYPE_DI_PW || vf->di_flag & DI_FLAG_DI_PVPPLINK) {
		if (vf->type_original & VIDTYPE_INTERLACE)
			di_keep_count = DI_KEEP_COUNT_I;
		else
			di_keep_count = DI_KEEP_COUNT_P;
		do_di = true;
		if (vf->flag & VFRAME_FLAG_DOUBLE_FRAM)
			di_has_vdin_vf = true;
	}

	dv_flag = vf->dv_input ? 1 : 0;
	if (dv_flag != hdmin_dv_flag) {
		hdmin_dv_flag = dv_flag;
		pr_info("dv_flag changed, new flag is %d.\n", dv_flag);
	}

#ifdef CONFIG_AMLOGIC_MEDIA_VDIN
	vdin_keep_count += get_vdin_add_delay_num();
#endif
	di_backend_en = get_di_proc_enable();
	if (di_backend_en) {
		vdin_keep_count += 1;
		di_keep_count = 0;
		if (vf->type_original & VIDTYPE_INTERLACE)
			display_path_count += 1;
	}

	vdin_vsync = vf->duration;
	vdin_vsync = vdin_vsync * 1000;
	vdin_vsync = div64_u64(vdin_vsync, 96);

	vpp_vsync = vsync_pts_inc_scale;
	vpp_vsync = vpp_vsync * 1000000;
	vpp_vsync = div64_u64(vpp_vsync, vsync_pts_inc_scale_base);

#ifdef CONFIG_AMLOGIC_MEDIA_FRC
	memc_delay = frc_get_video_latency();
#endif

	if (vf->duration != 0)
		ret_hz = (96000 + vf->duration / 2) / vf->duration;

	if (ret_hz >= 25)
		ext_delay = VLOCL_DELAY * 1000;
	else
		ext_delay = vpp_vsync;

	/*pre: vdin keep 1, di keep 1/2(p:1; I:2, one process), total 2/3
	 *I frame di will keep two, P frame keep one,
	 *post:disp path 2 buf delay(vq->vpp 1 buf, rdma one buf),
	 *if do di: count = (1 + 1/2) * vdin_vsync + 2 * vpp_vsync;
	 *if no di: count = (1 + 0) * vdin_vsync + 2* vpp_vsync;
	 *vdin vsync before vpp vsync about 7ms
	 */
	sync_count_pre = vdin_keep_count + di_keep_count;
	if (sync_count_pre < 2) {
		if (debug_flag & DEBUG_FLAG_HDMI_AVSYNC_DEBUG)
			pr_info("%s: pre delay need at least 2 vsync.\n", __func__);
		sync_count_pre = 2;
	}
	hdmin_delay_min = sync_count_pre * vdin_vsync +
		display_path_count * vpp_vsync + ext_delay;
	hdmin_delay_min_ms = div64_u64(hdmin_delay_min, 1000);
	hdmin_delay_min_ms += memc_delay;

	/*case 1:vdin vf sent to vpp:
	 *vdin total 11 buf, one for vdin next write, one vdin write, 9 left
	 *one is on display, 8 left
	 *disp path 2 buf delay(vq->vpp 1 buf, rdma one buf),
	 *one for vq next vsync to get, 5 left
	 *count = VDIN_BUF_COUNT - 1 -1 -1 -DIS_PATH_DELAY_COUNT - 1 = 5;
	 *total delay = 5 * vdin_vsync + 3 * vpp_vsync;
	 *
	 * case 2:vdin vf not sent to vpp:
	 *2.1:di max out 9,one on display, one for vq next vsync to get, 7 left
	 *2.2:vdin total 11, one for vdin next write, one vdin write, 9 left
	 *di keep 1/2 buf, 8/7 left
	 *count = (7 + 8/7) * vdin_vsync+ 3 * vpp_vsync;
	 */

#ifdef CONFIG_AMLOGIC_MEDIA_VIN
	vdin_buf_count = get_vdin_buffer_num();
	if (vdin_buf_count <= 0) {
		pr_info("%s:Get count failed, use default value.\n", __func__);
		vdin_buf_count = VDIN_BUF_COUNT;
	}
#endif
	if (di_has_vdin_vf || !do_di || di_backend_en) {
		vdin_count = vdin_buf_count - 3 - display_path_count - 1;
		vpp_count = display_path_count + 1;
	} else {
		vdin_count = DI_MAX_OUT_COUNT - 2 + vdin_buf_count - 2 - di_keep_count;
		vpp_count = display_path_count + 1;
	}
	hdmin_delay_max = vdin_count * vdin_vsync + vpp_count * vpp_vsync;
	hdmin_delay_max_ms = div64_u64(hdmin_delay_max, 1000);
	hdmin_delay_max_ms += memc_delay;

	if (debug_flag & DEBUG_FLAG_HDMI_AVSYNC_DEBUG) {
		pr_info("%s: di_has_vdin_vf=%d, do_di =%d.\n", __func__, di_has_vdin_vf, do_di);
		pr_info("%s: range(%d, %d).\n", __func__, hdmin_delay_min_ms, hdmin_delay_max_ms);
		pr_info("%s: vdin_keep_count=%d, di_keep_count=%d, display_path_count=%d.\n",
			__func__, vdin_keep_count, di_keep_count, display_path_count);
		pr_info("%s: vdin_count=%d, vpp_count=%d.\n", __func__, vdin_count, vpp_count);
	}
}

#ifdef CONFIG_AMLOGIC_MEDIA_VIN
static void hdmi_in_delay_maxmin_new1(struct tvin_to_vpp_info_s *tvin_info)
{
	u64 vdin_vsync = 0;
	u64 vpp_vsync = 0;
	u32 vdin_count = 0;
	u32 vpp_count = 0;
	int di_keep_count = 0;
	bool do_di = false;
	bool di_has_vdin_vf = false;
	u64 hdmin_delay_min = 0;
	u64 hdmin_delay_max = 0;
	u64 memc_delay = 0;
	int vdin_keep_count = VDIN_KEEP_COUNT;
	u64 ext_delay = 0;
	u32 vdin_buf_count = 0;
	bool di_backend_en = false;
	int display_path_count = DIS_PATH_DELAY_COUNT;
	u32 sync_count_pre = 0;

	if (!tvin_info->is_dv && tvin_info->width <= 3840 &&
		tvin_info->cfmt == TVIN_YUV422) {
		do_di = true;
		if ((tvin_info->width > 1920 && tvin_info->width <= 3840) || dim_get_pre_link())
			di_has_vdin_vf = true;
		if (tvin_info->scan_mode == TVIN_SCAN_MODE_INTERLACED)
			di_keep_count = DI_KEEP_COUNT_I;
		else
			di_keep_count = DI_KEEP_COUNT_P;
	}

	hdmin_dv_flag = tvin_info->is_dv;

#ifdef CONFIG_AMLOGIC_MEDIA_VDIN
	vdin_keep_count += get_vdin_add_delay_num();
#endif
	di_backend_en = get_di_proc_enable();
	if (di_backend_en) {
		vdin_keep_count += 1;
		di_keep_count = 0;
		if (tvin_info->scan_mode == TVIN_SCAN_MODE_INTERLACED)
			display_path_count += 1;
	}

	vdin_vsync = div64_u64(1000 * 1000, tvin_info->fps);

	if (tvin_info->fps == 50 || tvin_info->fps == 25)
		vpp_vsync = div64_u64(1000 * 1000, 50);
	else
		vpp_vsync = div64_u64(1000 * 1000, 60);

#ifdef CONFIG_AMLOGIC_MEDIA_FRC
	memc_delay = frc_get_video_latency();
#endif

	if (tvin_info->fps >= 25)
		ext_delay = VLOCL_DELAY * 1000;
	else
		ext_delay = vpp_vsync;

	/*pre: vdin keep 1, di keep 1/2(p:1; I:2, one process), total 2/3
	 *I frame di will keep two, P frame keep one,
	 *post:disp path 2 buf delay(vq->vpp 1 buf, rdma one buf),
	 *if do di: count = (1 + 1/2) * vdin_vsync + 2 * vpp_vsync;
	 *if no di: count = (1 + 0) * vdin_vsync + 2* vpp_vsync;
	 *vdin vsync before vpp vsync about 7ms
	 */
	sync_count_pre = vdin_keep_count + di_keep_count;
	if (sync_count_pre < 2) {
		if (debug_flag & DEBUG_FLAG_HDMI_AVSYNC_DEBUG)
			pr_info("%s: pre delay need at least 2 vsync.\n", __func__);
		sync_count_pre = 2;
	}
	hdmin_delay_min = sync_count_pre * vdin_vsync +
		display_path_count * vpp_vsync + ext_delay;
	hdmin_delay_min_ms = div64_u64(hdmin_delay_min, 1000);
	hdmin_delay_min_ms += memc_delay;

	/*case 1:vdin vf sent to vpp:
	 *vdin total 11 buf, one for vdin next write, one vdin write, 9 left
	 *one is on display, 8 left
	 *disp path 2 buf delay(vq->vpp 1 buf, rdma one buf),
	 *one for vq next vsync to get, 5 left
	 *count = VDIN_BUF_COUNT - 1 -1 -1 -DIS_PATH_DELAY_COUNT - 1 = 5;
	 *total delay = 5 * vdin_vsync + 3 * vpp_vsync;
	 *
	 * case 2:vdin vf not sent to vpp:
	 *2.1:di max out 9,one on display, one for vq next vsync to get, 7 left
	 *2.2:vdin total 11, one for vdin next write, one vdin write, 9 left
	 *di keep 1/2 buf, 8/7 left
	 *count = (7 + 8/7) * vdin_vsync+ 3 * vpp_vsync;
	 */
	vdin_buf_count = get_vdin_buffer_num();
	if (vdin_buf_count <= 0) {
		pr_info("%s:Get count failed, use default value.\n", __func__);
		vdin_buf_count = VDIN_BUF_COUNT;
	}
	if (di_has_vdin_vf || !do_di || di_backend_en) {
		vdin_count = vdin_buf_count - 3 - display_path_count - 1;
		vpp_count = display_path_count + 1;
	} else {
		vdin_count = DI_MAX_OUT_COUNT - 2 + vdin_buf_count - 2 - di_keep_count;
		vpp_count = display_path_count + 1;
	}
	hdmin_delay_max = vdin_count * vdin_vsync + vpp_count * vpp_vsync;
	hdmin_delay_max_ms = div64_u64(hdmin_delay_max, 1000);
	hdmin_delay_max_ms += memc_delay;
	pr_info("cfmt=%d, scan=%d,%d*%d_%dhz, isdv=%d.\n",
		tvin_info->cfmt,
		tvin_info->scan_mode,
		tvin_info->width,
		tvin_info->height,
		tvin_info->fps,
		tvin_info->is_dv);
	if (debug_flag & DEBUG_FLAG_HDMI_AVSYNC_DEBUG) {
		pr_info("%s: di_has_vdin_vf=%d, do_di =%d.\n", __func__, di_has_vdin_vf, do_di);
		pr_info("%s: range(%d, %d).\n", __func__, hdmin_delay_min_ms, hdmin_delay_max_ms);
		pr_info("%s: vdin_keep_count=%d, di_keep_count=%d, display_path_count=%d.\n",
			__func__, vdin_keep_count, di_keep_count, display_path_count);
		pr_info("%s: vdin_count=%d, vpp_count=%d.\n", __func__, vdin_count, vpp_count);
	}
}

void vdin_start_notify_vpp(struct tvin_to_vpp_info_s *tvin_info)
{
	tvin_delay_mode = 1;
	if (vf_check_node("videoqueue.0"))
		hdmi_in_delay_maxmin_new1(tvin_info);
}
EXPORT_SYMBOL(vdin_start_notify_vpp);
#endif

void get_vdx_real_axis(u32 index, int *buf)
{
	struct vpp_frame_par_s *_cur_frame_par = cur_frame_par[index];
	struct vinfo_s *vinfo = get_current_vinfo();

	if (!_cur_frame_par)
		return;
	*(buf + 0) = _cur_frame_par->VPP_hsc_startp;
	*(buf + 1) = _cur_frame_par->VPP_vsc_startp;
	*(buf + 2) = _cur_frame_par->VPP_hsc_endp - 1;
	if (is_interlaced(vinfo))
		*(buf + 3) = (_cur_frame_par->VPP_vsc_endp << 1) + 1;
	else
		*(buf + 3) = _cur_frame_par->VPP_vsc_endp;
	if (debug_flag & DEBUG_FLAG_BASIC_INFO)
		pr_info("index=%d, axis= %d %d %d %d\n", index, *(buf + 0),
			*(buf + 1), *(buf + 2), *(buf + 3));
}
EXPORT_SYMBOL(get_vdx_real_axis);

void get_vdx_axis(u32 index, int *buf)
{
	struct disp_info_s *layer_info = NULL;

	switch (index) {
	case 0:
		layer_info = &glayer_info[0];
		break;
	case 1:
		layer_info = &glayer_info[1];
		break;
	case 2:
		layer_info = &glayer_info[2];
		break;
	}
	if (!layer_info)
		return;
	*(buf + 0) = layer_info->layer_left;
	*(buf + 1) = layer_info->layer_top;
	*(buf + 2) = layer_info->layer_left + layer_info->layer_width - 1;
	*(buf + 3) = layer_info->layer_top + layer_info->layer_height - 1;
	pr_info("index=%d, axis= %d %d %d %d\n", index, *(buf + 0),
		*(buf + 1), *(buf + 2), *(buf + 3));
}
EXPORT_SYMBOL(get_vdx_axis);

u32 get_tvin_delay_start(void)
{
	if (debug_flag & DEBUG_FLAG_HDMI_AVSYNC_DEBUG)
		pr_info("%s: start value = %d.\n",
			__func__, hdmin_delay_start);

	return hdmin_delay_start;
}
EXPORT_SYMBOL(get_tvin_delay_start);

void set_tvin_delay_start(u32 start)
{
	if (debug_flag & DEBUG_FLAG_HDMI_AVSYNC_DEBUG)
		pr_info("%s: start value = %d.\n", __func__, start);

	hdmin_delay_start = start;
}
EXPORT_SYMBOL(set_tvin_delay_start);

u32 get_tvin_delay_duration(void)
{
	if (debug_flag & DEBUG_FLAG_HDMI_AVSYNC_DEBUG)
		pr_info("%s: last required total_delay %d ms.\n",
			__func__, last_required_total_delay);

	return last_required_total_delay;
}
EXPORT_SYMBOL(get_tvin_delay_duration);

void set_tvin_delay_duration(u32 time)
{
	if (debug_flag & DEBUG_FLAG_HDMI_AVSYNC_DEBUG)
		pr_info("%s: audio want vpp delay %d ms.\n", __func__, time);

	last_required_total_delay = time;
}
EXPORT_SYMBOL(set_tvin_delay_duration);

u32 get_tvin_delay(void)
{
	if (debug_flag & DEBUG_FLAG_HDMI_AVSYNC_DEBUG)
		pr_info("%s: vframe walk delay%d ms.\n",
			__func__, vframe_walk_delay);

	return vframe_walk_delay;
}
EXPORT_SYMBOL(get_tvin_delay);

u32 get_tvin_delay_max_ms(void)
{
	if (debug_flag & DEBUG_FLAG_HDMI_AVSYNC_DEBUG)
		pr_info("%s: max delay %d ms.\n", __func__, hdmin_delay_max_ms);

	return hdmin_delay_max_ms;
}
EXPORT_SYMBOL(get_tvin_delay_max_ms);

u32 get_tvin_delay_min_ms(void)
{
	if (debug_flag & DEBUG_FLAG_HDMI_AVSYNC_DEBUG)
		pr_info("%s: min delay %d ms.\n", __func__, hdmin_delay_min_ms);

	return hdmin_delay_min_ms;
}
EXPORT_SYMBOL(get_tvin_delay_min_ms);

u32 get_tvin_dv_flag(void)
{
	if (debug_flag & DEBUG_FLAG_HDMI_AVSYNC_DEBUG)
		pr_info("%s: dv_flag is %d.\n", __func__, hdmin_dv_flag);

	return hdmin_dv_flag;
}
EXPORT_SYMBOL(get_tvin_dv_flag);

void clear_vsync_2to1_info(void)
{
	vd_layer[0].frc_n2m_1st_frame = false;
	if (cur_dev->vsync_2to1_enable) {
		vsync_count_start = false;
		new_frame_cnt = 0;
	}
}
EXPORT_SYMBOL(clear_vsync_2to1_info);

/*for video related files only.*/
void video_module_lock(void)
{
	mutex_lock(&video_module_mutex);
}

void video_module_unlock(void)
{
	mutex_unlock(&video_module_mutex);
}

#if defined(PTS_LOGGING)
static ssize_t pts_pattern_enter_cnt_read_file(struct file *file,
					       char __user *userbuf,
					       size_t count,
					       loff_t *ppos)
{
	char buf[20];
	ssize_t len;

	len = snprintf(buf, 20, "%d,%d,%d\n", pts_pattern_enter_cnt[0],
		       pts_pattern_enter_cnt[1], pts_pattern_enter_cnt[2]);
	return simple_read_from_buffer(userbuf, count, ppos, buf, len);
}

static ssize_t pts_pattern_exit_cnt_read_file(struct file *file,
					      char __user *userbuf,
					      size_t count,
					      loff_t *ppos)
{
	char buf[20];
	ssize_t len;

	len = snprintf(buf, 20, "%d,%d,%d\n", pts_pattern_exit_cnt[0],
		       pts_pattern_exit_cnt[1], pts_pattern_exit_cnt[2]);
	return simple_read_from_buffer(userbuf, count, ppos, buf, len);
}

static ssize_t pts_log_enable_read_file(struct file *file,
					char __user *userbuf,
					size_t count,
					loff_t *ppos)
{
	char buf[20];
	ssize_t len;

	len = snprintf(buf, 20, "%d,%d,%d\n", pts_log_enable[0],
		       pts_log_enable[1], pts_log_enable[2]);
	return simple_read_from_buffer(userbuf, count, ppos, buf, len);
}

static ssize_t pts_log_enable_write_file(struct file *file,
					 const char __user *userbuf,
					 size_t count,
					 loff_t *ppos)
{
	char buf[20];
	int ret;

	count = min_t(size_t, count, (sizeof(buf) - 1));
	if (copy_from_user(buf, userbuf, count))
		return -EFAULT;
	buf[count] = 0;
	/* pts_pattern_log_enable (3:2) (2:2) (4:1) */
	ret = sscanf(buf, "%d,%d,%d", &pts_log_enable[0], &pts_log_enable[1],
		     &pts_log_enable[2]);
	if (ret != 3) {
		pr_info("use echo 0/1,0/1,0/1 > /sys/kernel/debug/pts_log_enable\n");
	} else {
		pr_info("pts_log_enable: %d,%d,%d\n", pts_log_enable[0],
			pts_log_enable[1], pts_log_enable[2]);
	}
	return count;
}

static ssize_t pts_enforce_pulldown_read_file(struct file *file,
					      char __user *userbuf,
					      size_t count,
					      loff_t *ppos)
{
	char buf[16];
	ssize_t len;

	len = snprintf(buf, 16, "%d\n", pts_enforce_pulldown);
	return simple_read_from_buffer(userbuf, count, ppos, buf, len);
}

static ssize_t pts_enforce_pulldown_write_file(struct file *file,
					       const char __user *userbuf,
					       size_t count, loff_t *ppos)
{
	unsigned int write_val;
	char buf[16];
	int ret;

	count = min_t(size_t, count, (sizeof(buf) - 1));
	if (copy_from_user(buf, userbuf, count))
		return -EFAULT;
	buf[count] = 0;
	ret = kstrtouint(buf, 0, &write_val);
	if (ret != 0)
		return -EINVAL;
	pr_info("pts_enforce_pulldown: %d->%d\n",
		pts_enforce_pulldown, write_val);
	pts_enforce_pulldown = write_val;
	return count;
}

static ssize_t dump_reg_write(struct file *file, const char __user *userbuf,
			      size_t count, loff_t *ppos)
{
	return count;
}

static int dump_reg_show(struct seq_file *s, void *what)
{
	unsigned int  i = 0, count;
	u32 reg_addr, reg_val = 0;
	struct sr_info_s *sr;

	if (cur_dev->display_module == C3_DISPLAY_MODULE) {
		/* vd1 mif */
		seq_puts(s, "\nvd1 mif registers:\n");
		reg_addr = vd_layer[0].vd_mif_reg.vd_if0_gen_reg;
		count = 32;
		for (i = 0; i < count; i++) {
			reg_val = READ_VCBUS_REG(reg_addr);
			seq_printf(s, "[0x%x] = 0x%X\n",
				   reg_addr, reg_val);
			reg_addr += 1;
		}
		seq_puts(s, "\nvd1 csc registers:\n");
		reg_addr = VOUT_VD1_CSC_COEF00_01;
		count = 14;
		for (i = 0; i < count; i++) {
			reg_val = READ_VCBUS_REG(reg_addr);
			seq_printf(s, "[0x%x] = 0x%X\n",
				   reg_addr, reg_val);
			reg_addr += 1;
		}
		seq_puts(s, "\nvout blend registers:\n");
		reg_addr = VPU_VOUT_BLEND_CTRL;
		count = 3;
		for (i = 0; i < count; i++) {
			reg_val = READ_VCBUS_REG(reg_addr);
			seq_printf(s, "[0x%x] = 0x%X\n",
				   reg_addr, reg_val);
			reg_addr += 1;
		}
		reg_addr = VPU_VOUT_BLD_SRC0_HPOS;
		count = 2;
		for (i = 0; i < count; i++) {
			reg_val = READ_VCBUS_REG(reg_addr);
			seq_printf(s, "[0x%x] = 0x%X\n",
				   reg_addr, reg_val);
			reg_addr += 1;
		}
		reg_addr = VPU_VOUT_BLD_SRC1_HPOS;
		count = 2;
		for (i = 0; i < count; i++) {
			reg_val = READ_VCBUS_REG(reg_addr);
			seq_printf(s, "[0x%x] = 0x%X\n",
				   reg_addr, reg_val);
			reg_addr += 1;
		}
	} else {
		/* viu top regs */
		seq_puts(s, "\nviu top registers:\n");
		reg_addr = 0x1a04;
		count = 12;
		for (i = 0; i < count; i++) {
			reg_val = READ_VCBUS_REG(reg_addr);
			seq_printf(s, "[0x%x] = 0x%X\n",
				   reg_addr, reg_val);
			reg_addr += 1;
		}
		/* vpp registers begin from 0x1d00*/
		seq_puts(s, "vpp registers:\n");
		reg_addr = VPP_DUMMY_DATA + cur_dev->vpp_off;
		count = 256;
		for (i = 0; i < count; i++) {
			reg_val = READ_VCBUS_REG(reg_addr);
			seq_printf(s, "[0x%x] = 0x%X\n",
				   reg_addr, reg_val);
			reg_addr += 1;
		}
		/* vd1 afbc regs */
		seq_puts(s, "\nvd1 afbc registers:\n");
		reg_addr = vd_layer[0].vd_afbc_reg.afbc_enable;
		count = 32;
		for (i = 0; i < count; i++) {
			reg_val = READ_VCBUS_REG(reg_addr);
			seq_printf(s, "[0x%x] = 0x%X\n",
				   reg_addr, reg_val);
			reg_addr += 1;
		}
		/* vd2 afbc regs */
		seq_puts(s, "\nvd2 afbc registers:\n");
		reg_addr = vd_layer[1].vd_afbc_reg.afbc_enable;
		count = 32;
		for (i = 0; i < count; i++) {
			reg_val = READ_VCBUS_REG(reg_addr);
			seq_printf(s, "[0x%x] = 0x%X\n",
				   reg_addr, reg_val);
			reg_addr += 1;
		}
		if (cur_dev->max_vd_layers == 3) {
			/* vd3 afbc regs */
			seq_puts(s, "\nvd3 afbc registers:\n");
			reg_addr = vd_layer[2].vd_afbc_reg.afbc_enable;
			count = 32;
			for (i = 0; i < count; i++) {
				reg_val = READ_VCBUS_REG(reg_addr);
				seq_printf(s, "[0x%x] = 0x%X\n",
					   reg_addr, reg_val);
				reg_addr += 1;
			}
		}
		/* vd1 mif */
		seq_puts(s, "\nvd1 mif registers:\n");
		reg_addr = vd_layer[0].vd_mif_reg.vd_if0_gen_reg;
		count = 32;
		for (i = 0; i < count; i++) {
			reg_val = READ_VCBUS_REG(reg_addr);
			seq_printf(s, "[0x%x] = 0x%X\n",
				   reg_addr, reg_val);
			reg_addr += 1;
		}
		/* vd2 mif */
		seq_puts(s, "\nvd2 mif registers:\n");
		reg_addr = vd_layer[1].vd_mif_reg.vd_if0_gen_reg;
		count = 32;
		for (i = 0; i < count; i++) {
			reg_val = READ_VCBUS_REG(reg_addr);
			seq_printf(s, "[0x%x] = 0x%X\n",
				   reg_addr, reg_val);
			reg_addr += 1;
		}
		if (cur_dev->max_vd_layers == 3) {
			/* vd3 mif */
			seq_puts(s, "\nvd3 mif registers:\n");
			reg_addr = vd_layer[2].vd_mif_reg.vd_if0_gen_reg;
			count = 32;
			for (i = 0; i < count; i++) {
				reg_val = READ_VCBUS_REG(reg_addr);
				seq_printf(s, "[0x%x] = 0x%X\n",
					   reg_addr, reg_val);
				reg_addr += 1;
			}
		}
		/* vd1(0x3800) & vd2(0x3850) hdr */
		/* osd hdr (0x38a0) */
		seq_puts(s, "\nvd1(0x3800) & vd2(0x3850) hdr registers:\n");
		seq_puts(s, "osd hdr (0x38a0) registers:\n");
		reg_addr = 0x3800;
		count = 256;
		for (i = 0; i < count; i++) {
			reg_val = READ_VCBUS_REG(reg_addr);
			seq_printf(s, "[0x%x] = 0x%X\n",
				   reg_addr, reg_val);
			reg_addr += 1;
		}
		/* super scaler */
		sr = &sr_info;
		/* g12a ~ tm2: 0x3e00 */
		/* tm2 revb: 0x5000 */
		seq_puts(s, "\nsuper scaler 0 registers:\n");
		reg_addr = SRSHARP0_SHARP_HVSIZE + sr->sr_reg_offt;
		count = 128;
		for (i = 0; i < count; i++) {
			reg_val = READ_VCBUS_REG(reg_addr);
			seq_printf(s, "[0x%x] = 0x%X\n",
				   reg_addr, reg_val);
			reg_addr += 1;
		}
		/* tl1, tm2 : 0x3f00*/
		/* tm2 revb: 0x5200 */
		seq_puts(s, "\nsuper scaler 1 registers:\n");
		reg_addr = SRSHARP1_SHARP_HVSIZE + sr->sr_reg_offt2;
		count = 128;
		for (i = 0; i < count; i++) {
			reg_val = READ_VCBUS_REG(reg_addr);
			seq_printf(s, "[0x%x] = 0x%X\n",
				   reg_addr, reg_val);
			reg_addr += 1;
		}
	}
	return 0;
}

static int dump_reg_open(struct inode *inode, struct file *file)
{
	return single_open(file, dump_reg_show, inode->i_private);
}

static const struct file_operations pts_pattern_enter_cnt_file_ops = {
	.open		= simple_open,
	.read		= pts_pattern_enter_cnt_read_file,
};

static const struct file_operations pts_pattern_exit_cnt_file_ops = {
	.open		= simple_open,
	.read		= pts_pattern_exit_cnt_read_file,
};

static const struct file_operations pts_log_enable_file_ops = {
	.open		= simple_open,
	.read		= pts_log_enable_read_file,
	.write		= pts_log_enable_write_file,
};

static const struct file_operations pts_enforce_pulldown_file_ops = {
	.open		= simple_open,
	.read		= pts_enforce_pulldown_read_file,
	.write		= pts_enforce_pulldown_write_file,
};
#endif

static const struct file_operations reg_dump_file_ops = {
	.open		= dump_reg_open,
	.read		= seq_read,
	.write		= dump_reg_write,
	.release	= single_release,
};

struct video_debugfs_files_s {
	const char *name;
	const umode_t mode;
	const struct file_operations *fops;
};

static struct video_debugfs_files_s video_debugfs_files[] = {
#if defined(PTS_LOGGING)
	{"pts_pattern_enter_cnt", S_IFREG | 0444,
		&pts_pattern_enter_cnt_file_ops
	},
	{"pts_pattern_exit_cnt", S_IFREG | 0444,
		&pts_pattern_exit_cnt_file_ops
	},
	{"pts_log_enable", S_IFREG | 0644,
		&pts_log_enable_file_ops
	},
	{"pts_enforce_pulldown", S_IFREG | 0644,
		&pts_enforce_pulldown_file_ops
	},
#endif
	{"reg_dump", S_IFREG | 0644,
		&reg_dump_file_ops
	},
};

static void video_debugfs_init(void)
{
	struct dentry *ent;
	int i;

	if (video_debugfs_root)
		return;
	video_debugfs_root = debugfs_create_dir("video", NULL);
	if (!video_debugfs_root) {
		pr_err("can't create video debugfs dir\n");
		return;
	}

	for (i = 0; i < ARRAY_SIZE(video_debugfs_files); i++) {
		ent = debugfs_create_file(video_debugfs_files[i].name,
					  video_debugfs_files[i].mode,
					  video_debugfs_root, NULL,
					  video_debugfs_files[i].fops);
		if (!ent)
			pr_info("debugfs create file %s failed\n",
				video_debugfs_files[i].name);
	}
}

static void video_debugfs_exit(void)
{
	debugfs_remove(video_debugfs_root);
}

#ifndef CONFIG_AMLOGIC_VOUT
const struct vinfo_s invalid_vinfo = {
	.name			   = "invalid",
	.mode			   = VMODE_INVALID,
	.width			   = 1920,
	.height			   = 1080,
	.field_height	   = 1080,
	.aspect_ratio_num  = 16,
	.aspect_ratio_den  = 9,
	.sync_duration_num = 60,
	.sync_duration_den = 1,
	.video_clk		   = 148500000,
	.htotal			   = 2200,
	.vtotal			   = 1125,
	.viu_color_fmt	   = COLOR_FMT_RGB444,
	.viu_mux		   = ((3 << 4) | VIU_MUX_MAX),
};

const struct vinfo_s *vinfo = &invalid_vinfo;

struct vinfo_s *get_current_vinfo(void)
{
	struct vinfo_s *vinfo = (struct vinfo_s *)&invalid_vinfo;

	return vinfo;
}
#endif

#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
#define OVER_FIELD_NORMAL 0
#define OVER_FIELD_NEW_VF 1
#define OVER_FIELD_RDMA_READY 2
#define OVER_FIELD_STATE_MAX 3
static u32 wrong_state_change_cnt;

void update_over_field_states(u32 new_state, bool force)
{
	struct timeval timeinfo;
	u32 cur_state;
	bool expection = true;

	/* TODO: if need add rdma ready to new frame case */
	cur_state = atomic_read(&cur_over_field_state);
	switch (new_state) {
	case OVER_FIELD_NORMAL:
		if (cur_state == OVER_FIELD_RDMA_READY) {
			atomic_set(&cur_over_field_state, OVER_FIELD_NORMAL);
			expection = false;
		} else if (cur_state == OVER_FIELD_NORMAL) {
			expection = false;
		}
		break;
	case OVER_FIELD_NEW_VF:
		if (cur_state == OVER_FIELD_NORMAL) {
			atomic_set(&cur_over_field_state, OVER_FIELD_NEW_VF);
			expection = false;
		}
		break;
	case OVER_FIELD_RDMA_READY:
		if (cur_state == OVER_FIELD_NEW_VF) {
			atomic_set(&cur_over_field_state, OVER_FIELD_RDMA_READY);
			expection = false;
			config_vsync_num = get_cur_enc_num();
			do_gettimeofday(&timeinfo);
			config_timeinfo = timeinfo.tv_sec * 1000000 + timeinfo.tv_usec;
		} else if (cur_state == OVER_FIELD_NORMAL) {
			expection = false;
		}
		break;
	default:
		break;
	}
	if (force && new_state < OVER_FIELD_STATE_MAX) {
		atomic_set(&cur_over_field_state, new_state);
		expection = false;
	}

	if (expection) {
		if (new_state >= OVER_FIELD_STATE_MAX ||
		    cur_state >= OVER_FIELD_STATE_MAX)
			pr_info("invalid over field state: %d/%d\n",
				cur_state, new_state);
		else
			wrong_state_change_cnt++;
	}
	vpp_trace_field_state("UPDATE-STATE",
		cur_state, new_state, expection ? 1 : 0,
		wrong_state_change_cnt, 0);
}
#endif

void set_freerun_mode(int mode)
{
	freerun_mode = mode;
}
EXPORT_SYMBOL(set_freerun_mode);

void set_pts_realign(void)
{
	vsync_pts_aligned = false;
}
EXPORT_SYMBOL(set_pts_realign);

u32 get_first_pic_coming(void)
{
	return first_frame_toggled;
}
EXPORT_SYMBOL(get_first_pic_coming);

u32 get_toggle_frame_count(void)
{
	return new_frame_count;
}
EXPORT_SYMBOL(get_toggle_frame_count);

void set_video_peek(void)
{
	videopeek = true;
}
EXPORT_SYMBOL(set_video_peek);

u32 get_first_frame_toggled(void)
{
	return first_frame_toggled;
}
EXPORT_SYMBOL(get_first_frame_toggled);

u32 get_vsync_frame_count(void)
{
	return new_frame_count;
}
EXPORT_SYMBOL(get_vsync_frame_count);

static s32 is_afbc_for_vpp(u8 id)
{
	s32 ret = -1;
	u32 val;

	if (id >= MAX_VD_LAYERS || legacy_vpp)
		return ret;
	if (cur_dev->display_module != OLD_DISPLAY_MODULE)
		return 0;
	if (cur_dev->display_module == S5_DISPLAY_MODULE)
		return 0;
	if (id == 0)
		val = READ_VCBUS_REG(VD1_AFBCD0_MISC_CTRL);
	else
		val = READ_VCBUS_REG(VD2_AFBCD1_MISC_CTRL);

	if ((val & (1 << 10)) && (val & (1 << 12)) &&
	    !(val & (1 << 9)))
		ret = 1;
	else
		ret = 0;
	return ret;
}

s32 di_request_afbc_hw(u8 id, bool on)
{
	u32 cur_afbc_request;
	u32 next_request = 0;
	s32 ret = -1;

	if (id >= MAX_VD_LAYERS)
		return ret;

	if (!glayer_info[id].afbc_support || legacy_vpp)
		return ret;

	next_request = 1 << id;
	cur_afbc_request = atomic_read(&gafbc_request);
	if (on) {
		if (cur_afbc_request & next_request)
			return is_afbc_for_vpp(id);

		atomic_add(next_request, &gafbc_request);
		ret = 1;
	} else {
		if ((cur_afbc_request & next_request) == 0)
			return is_afbc_for_vpp(id);

		atomic_sub(next_request, &gafbc_request);
		ret = 1;
	}
	vd_layer[id].property_changed = true;
	if (id > 0 && vd_layer[id].vpp_index != VPP0)
		vd_layer_vpp[id - 1].property_changed = true;
	return ret;
}
EXPORT_SYMBOL(di_request_afbc_hw);

u32 get_video_hold_state(void)
{
	return hold_video;
}
EXPORT_SYMBOL(get_video_hold_state);
/*********************************************************/
void vsync_notify(void)
{
	if (video_notify_flag & VIDEO_NOTIFY_TRICK_WAIT) {
		wake_up_interruptible(&amvideo_trick_wait);
		video_notify_flag &= ~VIDEO_NOTIFY_TRICK_WAIT;
	}
	if (video_notify_flag & VIDEO_NOTIFY_FRAME_WAIT) {
		video_notify_flag &= ~VIDEO_NOTIFY_FRAME_WAIT;
		vf_notify_provider(RECEIVER_NAME,
				   VFRAME_EVENT_RECEIVER_FRAME_WAIT, NULL);
	}
	if (video_notify_flag &
	    (VIDEO_NOTIFY_PROVIDER_GET | VIDEO_NOTIFY_PROVIDER_PUT)) {
		int event = 0;

		if (video_notify_flag & VIDEO_NOTIFY_PROVIDER_GET)
			event |= VFRAME_EVENT_RECEIVER_GET;
		if (video_notify_flag & VIDEO_NOTIFY_PROVIDER_PUT)
			event |= VFRAME_EVENT_RECEIVER_PUT;

		vf_notify_provider(RECEIVER_NAME, event, NULL);

		video_notify_flag &=
		    ~(VIDEO_NOTIFY_PROVIDER_GET | VIDEO_NOTIFY_PROVIDER_PUT);
	}
	if (video_notify_flag & VIDEO_NOTIFY_NEED_NO_COMP) {
		char *provider_name = vf_get_provider_name(RECEIVER_NAME);

		while (provider_name) {
			if (!vf_get_provider_name(provider_name))
				break;
			provider_name =
				vf_get_provider_name(provider_name);
		}
		if (provider_name)
			vf_notify_provider_by_name(provider_name,
				VFRAME_EVENT_RECEIVER_NEED_NO_COMP,
				(void *)&vpp_hold_setting_cnt);
		video_notify_flag &= ~VIDEO_NOTIFY_NEED_NO_COMP;
	}
#ifdef CONFIG_CLK81_DFS
	check_and_set_clk81();
#endif

#ifdef CONFIG_GAMMA_PROC
	gamma_adjust();
#endif
}

#ifdef FIQ_VSYNC
static irqreturn_t vsync_bridge_isr(int irq, void *dev_id)
{
	vsync_notify();

	return IRQ_HANDLED;
}
#endif

int get_vsync_count(unsigned char reset)
{
	if (reset)
		vsync_count = 0;
	return vsync_count;
}
EXPORT_SYMBOL(get_vsync_count);

int get_vsync_pts_inc_mode(void)
{
	return vsync_pts_inc_upint;
}
EXPORT_SYMBOL(get_vsync_pts_inc_mode);

void set_vsync_pts_inc_mode(int inc)
{
	vsync_pts_inc_upint = inc;
}
EXPORT_SYMBOL(set_vsync_pts_inc_mode);

u32 get_force_skip_cnt(enum vd_path_e path)
{
	if (path == VD1_PATH)
		return (force_skip_cnt & 3);
	else if (path == VD2_PATH)
		return ((force_skip_cnt >> 2) & 3);
	else
		return 0;
}

#ifdef FIQ_VSYNC
void vsync_fisr_in(void)
#else
static irqreturn_t vsync_isr_in(int irq, void *dev_id)
#endif
{
	if (video_suspend && video_suspend_cycle >= 1) {
		if (log_out)
			if (debug_flag & DEBUG_FLAG_BASIC_INFO)
				pr_info("video suspend, vsync exit\n");
		log_out = 0;
		check_video_mute();
		return IRQ_HANDLED;
	}
	if (debug_flag & DEBUG_FLAG_VSYNC_DONONE)
		return IRQ_HANDLED;

	post_vsync_process();
	return IRQ_HANDLED;
}

#ifdef FIQ_VSYNC
void vsync_fisr(void)
{
	lowlatency_vsync_count++;
	atomic_set(&video_inirq_flag, 1);
	vsync_fisr_in();
	atomic_set(&video_inirq_flag, 0);
}
#else
static irqreturn_t vsync_isr(int irq, void *dev_id)
{
	irqreturn_t ret;

	if (get_lowlatency_mode())
		put_buffer_proc();
	lowlatency_vsync_count++;
	if (atomic_inc_return(&video_proc_lock) > 1) {
		vsync_proc_drop++;
		atomic_dec(&video_proc_lock);
		vsync_cnt[VPP0]++;
		return 0;
	}
	if (overrun_flag) {
		overrun_flag = false;
		vsync_proc_drop++;
		lowlatency_overrun_recovery_cnt++;
		atomic_dec(&video_proc_lock);
		vsync_cnt[VPP0]++;
		return 0;
	}
	atomic_set(&video_inirq_flag, 1);
	ret = vsync_isr_in(irq, dev_id);
	atomic_set(&video_inirq_flag, 0);
	atomic_dec(&video_proc_lock);
	return ret;
}
#endif

static irqreturn_t vsync_pre_vsync_isr(int irq, void *dev_id)
{
	if (video_suspend && video_suspend_cycle >= 1) {
		if (log_out)
			if (debug_flag & DEBUG_FLAG_BASIC_INFO)
				pr_info("video suspend, vsync exit\n");
		log_out = 0;
		return IRQ_HANDLED;
	}
	if (debug_flag & DEBUG_FLAG_VSYNC_DONONE)
		return IRQ_HANDLED;
	atomic_set(&video_prevsync_inirq_flag, 1);
	pre_vsync_process();
	atomic_set(&video_prevsync_inirq_flag, 0);
	return IRQ_HANDLED;
}

static int printk_frame_done;
MODULE_PARM_DESC(printk_frame_done, "\n printk_frame_done\n");
module_param(printk_frame_done, int, 0664);

static irqreturn_t mosaic_frame_done_isr(int irq, void *dev_id)
{

	if (printk_frame_done) {
		u32 val1, val2;

		set_frm_idx(VPP0, 1);
		val1 = READ_VCBUS_REG(0x321e);
		val2 = READ_VCBUS_REG(0x3218);
		pr_info("0x321e=0x%x, 0x3218=0x%x\n", val1, val2);
		dump_mosaic_pps();
		set_frm_idx(VPP0, 0);
		printk_frame_done = 0;
	}
	return IRQ_HANDLED;
}

/*********************************************************
 * FIQ Routines
 *********************************************************/
static void vsync_fiq_up(void)
{
#ifdef FIQ_VSYNC
	request_fiq(INT_VIU_VSYNC, &vsync_fisr);
#else
	int r;

	r = request_irq(video_vsync, &vsync_isr,
		IRQF_SHARED, "vsync", (void *)video_dev_id);
	if (amvideo_meson_dev.cpu_type == MESON_CPU_MAJOR_ID_S5_)
		r = request_irq(mosaic_frame_done, &mosaic_frame_done_isr,
			IRQF_SHARED, "frame_done", (void *)video_dev_id);
	if (amvideo_meson_dev.cpu_type == MESON_CPU_MAJOR_ID_SC2_ ||
	    amvideo_meson_dev.has_vpp1)
		r = request_irq(video_vsync_viu2, &vsync_isr_viu2,
				IRQF_SHARED, "vsync_viu2",
				(void *)video_dev_id);
	if (amvideo_meson_dev.has_vpp2)
		r = request_irq(video_vsync_viu3, &vsync_isr_viu3,
				IRQF_SHARED, "vsync_viu3",
				(void *)video_dev_id);
	if (amvideo_meson_dev.dev_property.prevsync_support)
		r = request_irq(video_pre_vsync, &vsync_pre_vsync_isr,
				IRQF_SHARED, "pre_vsync",
				(void *)video_dev_id);
	if (r < 0)
		pr_info("request irq fail, %d\n", r);

#ifdef CONFIG_MESON_TRUSTZONE
	if (num_online_cpus() > 1)
		irq_set_affinity(INT_VIU_VSYNC, cpumask_of(1));
#endif
#endif
}

static void vsync_fiq_down(void)
{
#ifdef FIQ_VSYNC
	free_fiq(INT_VIU_VSYNC, &vsync_fisr);
#else
	free_irq(video_vsync, (void *)video_dev_id);
	if (amvideo_meson_dev.cpu_type == MESON_CPU_MAJOR_ID_S5_)
		free_irq(mosaic_frame_done, (void *)video_dev_id);
	if (amvideo_meson_dev.cpu_type == MESON_CPU_MAJOR_ID_SC2_ ||
	   amvideo_meson_dev.has_vpp1)
		free_irq(video_vsync_viu2, (void *)video_dev_id);
	if (amvideo_meson_dev.has_vpp2)
		free_irq(video_vsync_viu3, (void *)video_dev_id);
	if (amvideo_meson_dev.dev_property.prevsync_support)
		free_irq(video_pre_vsync, (void *)video_dev_id);
#endif
}

/*********************************************************
 * Vframe src fmt API
 *********************************************************/
#define signal_color_primaries ((vf->signal_type >> 16) & 0xff)
#define signal_transfer_characteristic ((vf->signal_type >> 8) & 0xff)
#define signal_cuva ((vf->signal_type >> 31) & 1)
#define PREFIX_SEI_NUT 39
#define SUFFIX_SEI_NUT 40
#define SEI_ITU_T_T35 4
#define ATSC_T35_PROV_CODE    0x0031
#define PRIME_SL_T35_PROV_CODE     0x003A
#define DVB_T35_PROV_CODE     0x003B
#define AV1_HDR10P_T35_PROV_CODE   0x003C
#define AV1_HDR10P_T35_PROV_ORIENTED_CODE   0x0001
#define AV1_HDR10P_APPLICATION_IDENTIFIER   4
#define ATSC_USER_ID_CODE     0x47413934
#define DVB_USER_ID_CODE      0x00000000
#define DM_MD_USER_TYPE_CODE  0x09
#define FMT_TYPE_DV 0
#define FMT_TYPE_DV_AV1 1
#define FMT_TYPE_HDR10_PLUS 2
#define FMT_TYPE_PRIME 3
#define FMT_TYPE_HDR10_PLUS_AV1 4
#ifndef DV_SEI
#define DV_SEI 0x01000000
#endif
/* for both dv and hdr10plus */
#ifndef AV1_SEI
#define AV1_SEI 0x14000000
#endif
#ifndef HDR10P
#define HDR10P 0x02000000
#endif

bool check_av1_hdr10p(char *p)
{
	u32 country_code;
	u32 provider_code;
	u32 provider_oriented_code;
	u32 application_identifier;

	if (!p)
		return false;

	country_code = *(p);
	provider_code = (*(p + 1) << 8) |
			*(p + 2);
	provider_oriented_code = (*(p + 3) << 8) | *(p + 4);
	application_identifier = *(p + 5);
	if (country_code == 0xB5 &&
	    provider_code ==
	    AV1_HDR10P_T35_PROV_CODE &&
	    provider_oriented_code == AV1_HDR10P_T35_PROV_ORIENTED_CODE &&
	    application_identifier == AV1_HDR10P_APPLICATION_IDENTIFIER)
		return true;
	else
		return false;
}
EXPORT_SYMBOL(check_av1_hdr10p);

static char *check_media_sei(char *sei, u32 sei_size, u32 fmt_type, u32 *ret_size)
{
	char *ret = NULL;
	char *p, *cur_p;
	u32 type = 0, size;
	unsigned char nal_type;
	unsigned char sei_payload_type = 0;
	unsigned char sei_payload_size = 0;
	u32 len_2094_sei = 0;
	u32 country_code;
	u32 provider_code;
	u32 user_id;
	u32 user_type_code;
	u32 sei_type;

	if (ret_size)
		*ret_size = 0;
	if (fmt_type == FMT_TYPE_DV)
		sei_type = DV_SEI;
	else if (fmt_type == FMT_TYPE_DV_AV1)
		sei_type = AV1_SEI;
	else if (fmt_type == FMT_TYPE_HDR10_PLUS ||
		fmt_type == FMT_TYPE_PRIME)
		sei_type = HDR10P; /* same sei type */
	else if (fmt_type == FMT_TYPE_HDR10_PLUS_AV1)
		sei_type = AV1_SEI;
	else
		return ret;

	if (!sei || sei_size <= 8)
		return ret;

	p = sei;
	while (p < sei + sei_size - 8) {
		cur_p = p;
		size = *p++;
		size = (size << 8) | *p++;
		size = (size << 8) | *p++;
		size = (size << 8) | *p++;
		type = *p++;
		type = (type << 8) | *p++;
		type = (type << 8) | *p++;
		type = (type << 8) | *p++;
		if (ret_size)
			*ret_size = size + 8;
		if ((sei_type == DV_SEI && sei_type == type)) {/*h264/h265 dv*/
			ret = cur_p;
			break;
		} else if (fmt_type == FMT_TYPE_DV_AV1 &&
			   sei_type == (type & 0xffff0000) &&
			   size > 6) {
			/*av1 dv, double check nal type and payload type to distinguish hdr10p*/
			if (!check_av1_hdr10p(p))
				ret = cur_p;
			if (debug_flag & DEBUG_FLAG_OMX_DV_DROP_FRAME)
				pr_info("check FMT_TYPE_DV_AV1 %px\n", ret);
			break;
		} else if (fmt_type == FMT_TYPE_HDR10_PLUS && sei_type == type) {
			/* TODO: double check nal type and payload type */
			ret = cur_p;
			break;
		} else if (fmt_type == FMT_TYPE_HDR10_PLUS_AV1 &&
			   sei_type == (type & 0xffff0000) &&
			   size > 6) {
			/* av1 hdr10p, double check nal type and payload type */
			/*4 byte size + 4 byte type*/
			/*1 byte country_code B5*/
			/*2 byte provider_code 003C*/
			/*2 byte provider_oriented_code 0001, 2094-40*/
			/*1 byte app_identifier 4*/
			/*1 byte app_mode 1*/
			if (check_av1_hdr10p(p))
				ret = cur_p;
			if (debug_flag & DEBUG_FLAG_OMX_DV_DROP_FRAME)
				pr_info("check FMT_TYPE_HDR10_PLUS_AV1 %px\n", ret);
			break;
		} else if (sei_type == DV_SEI && type == HDR10P) {
			/* check DVB/ATSC as DV */
			if (p >= sei + sei_size - 12)
				break;
			nal_type = ((*p) & 0x7E) >> 1;
			if (nal_type == PREFIX_SEI_NUT) {
				sei_payload_type = *(p + 2);
				sei_payload_size = *(p + 3);
				if (sei_payload_type == SEI_ITU_T_T35 &&
				    sei_payload_size >= 8) {
					country_code = *(p + 4);
					provider_code = (*(p + 5) << 8) |
							*(p + 6);
					user_id = (*(p + 7) << 24) |
						(*(p + 8) << 16) |
						(*(p + 9) << 8) |
						(*(p + 10));
					user_type_code = *(p + 11);
					if (country_code == 0xB5 &&
					    ((provider_code ==
					    ATSC_T35_PROV_CODE &&
					    user_id == ATSC_USER_ID_CODE) ||
					    (provider_code ==
					    DVB_T35_PROV_CODE &&
					    user_id == DVB_USER_ID_CODE)) &&
					    user_type_code ==
					    DM_MD_USER_TYPE_CODE)
						len_2094_sei = sei_payload_size;
				}
				if (len_2094_sei > 0) {
					ret = cur_p;
					break;
				}
			}
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_PRIME_SL
		} else if (fmt_type == FMT_TYPE_PRIME &&
				sei_type == type) {
			if (p >= sei + sei_size - 7)
				break;
			nal_type = ((*p) & 0x7E) >> 1;
			if (nal_type == PREFIX_SEI_NUT ||
			    nal_type == SUFFIX_SEI_NUT) {
				sei_payload_type = *(p + 2);
				sei_payload_size = *(p + 3);
				if (sei_payload_type == SEI_ITU_T_T35 &&
				    sei_payload_size >= 3) {
					country_code = *(p + 4);
					provider_code = (*(p + 5) << 8) |
							*(p + 6);
					if (country_code == 0xB5 &&
					    provider_code ==
					    PRIME_SL_T35_PROV_CODE) {
						ret = cur_p;
						break;
					}
				}
			}
#endif
		}
		p += size;
	}
	if (!ret && ret_size)
		*ret_size = 0;
	return ret;
}

static s32 clear_vframe_dovi_md_info(struct vframe_s *vf)
{
	if (!vf)
		return -1;

	/* invalid src fmt case */
	if (vf->src_fmt.sei_magic_code != SEI_MAGIC_CODE)
		return -1;

	vf->src_fmt.md_size = 0;
	vf->src_fmt.comp_size = 0;
	vf->src_fmt.md_buf = NULL;
	vf->src_fmt.comp_buf = NULL;
	vf->src_fmt.parse_ret_flags = 0;

	return 0;
}

static enum vframe_signal_fmt_e get_sfmt_from_signal_type(struct vframe_s *vf)
{
	if (!vf)
		return VFRAME_SIGNAL_FMT_INVALID;

	/* invalid src fmt case */
	if (vf->src_fmt.sei_magic_code != SEI_MAGIC_CODE) {
		if ((signal_transfer_characteristic == 14 ||
			 signal_transfer_characteristic == 18) &&
			signal_color_primaries == 9) {
			if (signal_cuva)
				vf->src_fmt.fmt = VFRAME_SIGNAL_FMT_CUVA_HLG;
			else
				vf->src_fmt.fmt = VFRAME_SIGNAL_FMT_HLG;
		} else if ((signal_transfer_characteristic == 16) &&
				 ((signal_color_primaries == 9) ||
				  (signal_color_primaries == 2))) {
			if (signal_cuva)
				vf->src_fmt.fmt = VFRAME_SIGNAL_FMT_CUVA_HDR;
			else
				vf->src_fmt.fmt = VFRAME_SIGNAL_FMT_HDR10;
		} else {
			vf->src_fmt.fmt = VFRAME_SIGNAL_FMT_SDR;
		}
	}

	if (debug_flag & DEBUG_FLAG_OMX_DV_DROP_FRAME)
		pr_info("[%s]  %d\n", __func__, vf->src_fmt.fmt);

	return vf->src_fmt.fmt;
}

s32 update_vframe_src_fmt(struct vframe_s *vf,
				   void *sei, u32 size, bool dual_layer,
				   char *prov_name, char *recv_name)
{
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
#if PARSE_MD_IN_ADVANCE
	int src_fmt = -1;
	int ret = 0;
#endif
#endif
	int i;
	char *p;

	if (!vf)
		return -1;

	vf->src_fmt.sei_magic_code = SEI_MAGIC_CODE;
	vf->src_fmt.fmt = VFRAME_SIGNAL_FMT_INVALID;
	vf->src_fmt.sei_ptr = sei;
	vf->src_fmt.sei_size = size;
	vf->src_fmt.dual_layer = false;
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
	vf->src_fmt.pr_done = false;
	vf->src_fmt.py_level = PY_NO_LEVEL;
	vf->src_fmt.downsamplers = 2;
#endif
	if (debug_flag & DEBUG_FLAG_OMX_DV_DROP_FRAME) {
		pr_info("===update vf %p, sei %p, size %d, dual_layer %d play_id = %d ===\n",
			vf, sei, size, dual_layer,
			vf->src_fmt.play_id);
		if (sei && size > 15) {
			p = (char *)sei;
			for (i = 0; i < size; i += 16)
				pr_info("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
					p[i],
					p[i + 1],
					p[i + 2],
					p[i + 3],
					p[i + 4],
					p[i + 5],
					p[i + 6],
					p[i + 7],
					p[i + 8],
					p[i + 9],
					p[i + 10],
					p[i + 11],
					p[i + 12],
					p[i + 13],
					p[i + 14],
					p[i + 15]);
		}
	}

	if (vf->type & VIDTYPE_MVC) {
		vf->src_fmt.fmt = VFRAME_SIGNAL_FMT_MVC;
	} else if (sei && size) {
		if (vf->discard_dv_data) {
			if (debug_flag & DEBUG_FLAG_OMX_DV_DROP_FRAME)
				pr_info("ignore nonstandard dv\n");
		}
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
		else if (dual_layer || check_media_sei(sei, size, FMT_TYPE_DV, NULL) ||
			   check_media_sei(sei, size, FMT_TYPE_DV_AV1, NULL)) {
			vf->src_fmt.fmt = VFRAME_SIGNAL_FMT_DOVI;
			vf->src_fmt.dual_layer = dual_layer;
#if PARSE_MD_IN_ADVANCE
			if (vf->src_fmt.md_buf && vf->src_fmt.comp_buf) {
				if (debug_flag & DEBUG_FLAG_OMX_DV_DROP_FRAME)
					pr_info("parse vf %p, sei %p, size %d, md_buf %p\n",
						vf, sei, size, vf->src_fmt.md_buf);
				ret = parse_sei_and_meta_ext
					(vf, sei, size,
					 &vf->src_fmt.comp_size,
					 &vf->src_fmt.md_size,
					 &src_fmt,
					 &vf->src_fmt.parse_ret_flags,
					 vf->src_fmt.md_buf,
					 vf->src_fmt.comp_buf);
				if (debug_flag & DEBUG_FLAG_OMX_DV_DROP_FRAME)
					pr_info("parse ret %d, %d, %d %d, %d\n",
						ret,
						src_fmt,
						vf->src_fmt.md_size,
						vf->src_fmt.comp_size,
						vf->src_fmt.parse_ret_flags);

				if (ret) {
					/* mark size -1 as parser failed */
					vf->src_fmt.md_size = -1;
					vf->src_fmt.comp_size = -1;
				}
			}
#else
			clear_vframe_dovi_md_info(vf);
#endif
		}
#endif
	}

	if (vf->src_fmt.fmt == VFRAME_SIGNAL_FMT_INVALID) {
		if (signal_transfer_characteristic == 18 &&
		    signal_color_primaries == 9) {
			if (signal_cuva)
				vf->src_fmt.fmt = VFRAME_SIGNAL_FMT_CUVA_HLG;
			else
				vf->src_fmt.fmt = VFRAME_SIGNAL_FMT_HLG;
		} else if ((signal_transfer_characteristic == 0x30) &&
			     ((signal_color_primaries == 9) ||
			      (signal_color_primaries == 2))) {
			if (check_media_sei(sei, size, FMT_TYPE_HDR10_PLUS, NULL) ||
			    check_media_sei(sei, size, FMT_TYPE_HDR10_PLUS_AV1, NULL))
				vf->src_fmt.fmt = VFRAME_SIGNAL_FMT_HDR10PLUS;
			else /* TODO: if need switch to HDR10 */
				vf->src_fmt.fmt = VFRAME_SIGNAL_FMT_HDR10;
		} else if ((signal_transfer_characteristic == 16) &&
			     ((signal_color_primaries == 9) ||
			      (signal_color_primaries == 2))) {
			if (signal_cuva)
				vf->src_fmt.fmt = VFRAME_SIGNAL_FMT_CUVA_HDR;
			else
				vf->src_fmt.fmt = VFRAME_SIGNAL_FMT_HDR10;
		} else if (signal_transfer_characteristic == 14 ||
			signal_transfer_characteristic == 1) {
			if (signal_color_primaries == 9)
				vf->src_fmt.fmt = VFRAME_SIGNAL_FMT_SDR_2020;
			else
				vf->src_fmt.fmt = VFRAME_SIGNAL_FMT_SDR;
		} else {
			vf->src_fmt.fmt = VFRAME_SIGNAL_FMT_SDR;
		}
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_PRIME_SL
		if (is_prime_sl_enable() && sei && size &&
		    vf->src_fmt.fmt != VFRAME_SIGNAL_FMT_HDR10PLUS &&
		    !signal_cuva) {
			if (check_media_sei(sei, size, FMT_TYPE_PRIME, NULL))
				vf->src_fmt.fmt = VFRAME_SIGNAL_FMT_HDR10PRIME;
		}
#endif
	}

	if (vf->src_fmt.fmt != VFRAME_SIGNAL_FMT_DOVI)
		clear_vframe_dovi_md_info(vf);

	if (debug_flag & DEBUG_FLAG_OMX_DV_DROP_FRAME)
		pr_info("[%s]fmt: %d, vf %p, sei %p\n", __func__, vf->src_fmt.fmt,
				vf, vf->src_fmt.sei_ptr);

	return 0;
}
EXPORT_SYMBOL(update_vframe_src_fmt);

void *get_sei_from_src_fmt(struct vframe_s *vf, u32 *sei_size)
{
	if (!vf || !sei_size)
		return NULL;

	/* invalid src fmt case */
	if (vf->src_fmt.sei_magic_code != SEI_MAGIC_CODE)
		return NULL;

	*sei_size = vf->src_fmt.sei_size;
	return vf->src_fmt.sei_ptr;
}
EXPORT_SYMBOL(get_sei_from_src_fmt);

enum vframe_signal_fmt_e get_vframe_src_fmt(struct vframe_s *vf)
{
	if (!vf)
		return VFRAME_SIGNAL_FMT_INVALID;

	/* invalid src fmt case */
	if (vf->src_fmt.sei_magic_code != SEI_MAGIC_CODE)
		return VFRAME_SIGNAL_FMT_INVALID;

	if (debug_flag & DEBUG_FLAG_OMX_DV_DROP_FRAME)
		pr_info("[%s]  %d\n", __func__, vf->src_fmt.fmt);

	return vf->src_fmt.fmt;
}
EXPORT_SYMBOL(get_vframe_src_fmt);

/*return 0: no parse*/
/*return 1: dovi source and parse succeeded*/
/*return 2: dovi source and parse failed*/
int get_md_from_src_fmt(struct vframe_s *vf)
{
	int ret = 0;

	if (!vf)
		return 0;

	/* invalid src fmt case */
	if (vf->src_fmt.sei_magic_code != SEI_MAGIC_CODE ||
	    vf->src_fmt.fmt != VFRAME_SIGNAL_FMT_DOVI ||
	    !vf->src_fmt.md_buf ||
	    !vf->src_fmt.comp_buf) {
		ret = 0;
	} else {
		if (vf->src_fmt.md_size > 0)
			ret = 1;
		else if (vf->src_fmt.md_size == -1 &&
			 vf->src_fmt.comp_size == -1)
			ret = 2;
	}
	if (debug_flag & DEBUG_FLAG_OMX_DV_DROP_FRAME)
		pr_info("[%s] vf %p, ret %d\n", __func__, vf, ret);

	return ret;
}
EXPORT_SYMBOL(get_md_from_src_fmt);

s32 clear_vframe_src_fmt(struct vframe_s *vf)
{
	if (!vf)
		return -1;

	/* invalid src fmt case */
	if (vf->src_fmt.sei_magic_code != SEI_MAGIC_CODE)
		return -1;

	vf->src_fmt.sei_magic_code = 0;
	vf->src_fmt.fmt = VFRAME_SIGNAL_FMT_INVALID;
	vf->src_fmt.sei_ptr = NULL;
	vf->src_fmt.sei_size = 0;
	vf->src_fmt.dual_layer = false;
	vf->src_fmt.md_size = 0;
	vf->src_fmt.comp_size = 0;
	vf->src_fmt.md_buf = NULL;
	vf->src_fmt.comp_buf = NULL;
	vf->src_fmt.parse_ret_flags = 0;

	return 0;
}
EXPORT_SYMBOL(clear_vframe_src_fmt);

char *find_vframe_sei(struct vframe_s *vf,
		void *sei, u32 size, u32 *ret_size)
{
	u32 cur_sei_size = 0;
	char *ret_sei = NULL;
	bool dv_src = false;
	bool hdr10p = false;

	if (!vf || !ret_size)
		return NULL;

	*ret_size = 0;

	if (!sei || !size || (vf->type & VIDTYPE_MVC))
		return NULL;

	ret_sei = NULL;
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
	if (!vf->discard_dv_data) {
		ret_sei = check_media_sei(sei, size, FMT_TYPE_DV, &cur_sei_size);
		if (!ret_sei) {
			cur_sei_size = 0;
			ret_sei = check_media_sei(sei, size, FMT_TYPE_DV_AV1, &cur_sei_size);
		}
		if (ret_sei && cur_sei_size)
			dv_src = true;
	}
#endif
	if (!dv_src) {
		if (signal_transfer_characteristic == 18 &&
		    signal_color_primaries == 9) {
			/* HLG */
			/* TODO: need parser SEI for CUVA */
			if (signal_cuva) {
				ret_sei = sei;
				cur_sei_size = size;
			} else {
				ret_sei = NULL;
				cur_sei_size = 0;
			}
		} else if ((signal_transfer_characteristic == 0x30) &&
			     ((signal_color_primaries == 9) ||
			      (signal_color_primaries == 2))) {
			/* HDR10+ */
			ret_sei = check_media_sei(sei, size, FMT_TYPE_HDR10_PLUS, &cur_sei_size);
			if (!ret_sei) {
				cur_sei_size = 0;
				ret_sei = check_media_sei(sei, size,
					FMT_TYPE_HDR10_PLUS_AV1, &cur_sei_size);
			}
			if (ret_sei && cur_sei_size) {
				hdr10p = true;
			} else {
				/* Switch to HDR10 */
				ret_sei = NULL;
				cur_sei_size = 0;
			}
		} else if ((signal_transfer_characteristic == 16) &&
			     ((signal_color_primaries == 9) ||
			      (signal_color_primaries == 2))) {
			/* HDR10 */
			/* TODO: need parser SEI for CUVA */
			if (signal_cuva) {
				ret_sei = sei;
				cur_sei_size = size;
			} else {
				ret_sei = NULL;
				cur_sei_size = 0;
			}
		} else {
			/* SDR */
			ret_sei = NULL;
			cur_sei_size = 0;
		}
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_PRIME_SL
		/* PRIME */
		if (is_prime_sl_enable() && !hdr10p && !signal_cuva)
			ret_sei = check_media_sei(sei, size, FMT_TYPE_PRIME, &cur_sei_size);
#endif
	}
	if (ret_sei && cur_sei_size)
		*ret_size = cur_sei_size;
	else
		ret_sei = NULL;
	return ret_sei;
}
EXPORT_SYMBOL(find_vframe_sei);

/*********************************************************
 * Utilities
 *********************************************************/
int query_video_status(int type, int *value)
{
	if (!value)
		return -1;
	switch (type) {
	case 0:
		*value = trickmode_fffb;
		break;
	case 1:
		*value = trickmode_i;
		break;
	default:
		break;
	}
	return 0;
}
EXPORT_SYMBOL(query_video_status);

u32 video_get_layer_capability(void)
{
	return layer_cap;
}

int get_output_pcrscr_info(s32 *inc, u32 *base)
{
	if (IS_ERR_OR_NULL(inc) || IS_ERR_OR_NULL(base)) {
		pr_info("%s: param is NULL.\n", __func__);
		return -1;
	}

	if (frc_n2m_worked()) {
		*inc = vsync_pts_inc_scale;
		*base = vsync_pts_inc_scale_base / 2;
	} else {
		*inc = vsync_pts_inc_scale;
		*base = vsync_pts_inc_scale_base;
	}
	return 0;
}
EXPORT_SYMBOL(get_output_pcrscr_info);

int is_in_vsync_isr(void)
{
	if (atomic_read(&video_inirq_flag) > 0)
		return 1;
	else
		return 0;
}
EXPORT_SYMBOL(is_in_vsync_isr);

int is_in_pre_vsync_isr(void)
{
	if (atomic_read(&video_prevsync_inirq_flag) > 0)
		return 1;
	else
		return 0;
}
EXPORT_SYMBOL(is_in_pre_vsync_isr);

int is_video_process_in_thread(void)
{
	return 0;
}
EXPORT_SYMBOL(is_video_process_in_thread);

void set_video_angle(u32 s_value)
{
	struct disp_info_s *layer = &glayer_info[0];

	if (s_value <= 3 && layer->angle != s_value) {
		layer->angle = s_value;
		pr_info("video angle:%d\n", layer->angle);
	}
}
EXPORT_SYMBOL(set_video_angle);

bool get_vd1_vd2_mux(void)
{
	return vd1_vd2_mux;
}
EXPORT_SYMBOL(get_vd1_vd2_mux);

void set_vd1_vd2_mux(bool en)
{
	if (video_is_meson_t5d_revb_cpu()) {
		vd1_vd2_mux = en;
		vd_layer[0].vd1_vd2_mux = en;
		pr_info("%s:mux=%d\n", __func__, en);
	}
}
EXPORT_SYMBOL(set_vd1_vd2_mux);
/*********************************************************
 * /dev/amvideo APIs
 ********************************************************
 */
static int amvideo_open(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

static int amvideo_poll_open(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

static int amvideo_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

static int amvideo_poll_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

static long amvideo_ioctl(struct file *file, unsigned int cmd, ulong arg)
{
	long ret = 0;
	void __user *argp = (void __user *)arg;
	struct disp_info_s *layer = &glayer_info[0];
	u32 layer_id;

	switch (cmd) {
	case AMSTREAM_IOC_GLOBAL_SET_VIDEOPIP_OUTPUT:
	case AMSTREAM_IOC_GLOBAL_GET_VIDEOPIP_OUTPUT:
	case AMSTREAM_IOC_GET_VIDEOPIP_DISABLE:
	case AMSTREAM_IOC_SET_VIDEOPIP_DISABLE:
	case AMSTREAM_IOC_GET_VIDEOPIP_AXIS:
	case AMSTREAM_IOC_SET_VIDEOPIP_AXIS:
	case AMSTREAM_IOC_GET_VIDEOPIP_CROP:
	case AMSTREAM_IOC_SET_VIDEOPIP_CROP:
	case AMSTREAM_IOC_GET_PIP_SCREEN_MODE:
	case AMSTREAM_IOC_SET_PIP_SCREEN_MODE:
	case AMSTREAM_IOC_GET_PIP_ZORDER:
	case AMSTREAM_IOC_SET_PIP_ZORDER:
	case AMSTREAM_IOC_GET_PIP_DISPLAYPATH:
	case AMSTREAM_IOC_SET_PIP_DISPLAYPATH:
		layer = &glayer_info[1];
		break;
	case AMSTREAM_IOC_GLOBAL_SET_VIDEOPIP2_OUTPUT:
	case AMSTREAM_IOC_GLOBAL_GET_VIDEOPIP2_OUTPUT:
	case AMSTREAM_IOC_GET_VIDEOPIP2_DISABLE:
	case AMSTREAM_IOC_SET_VIDEOPIP2_DISABLE:
	case AMSTREAM_IOC_GET_VIDEOPIP2_AXIS:
	case AMSTREAM_IOC_SET_VIDEOPIP2_AXIS:
	case AMSTREAM_IOC_GET_VIDEOPIP2_CROP:
	case AMSTREAM_IOC_SET_VIDEOPIP2_CROP:
	case AMSTREAM_IOC_GET_PIP2_SCREEN_MODE:
	case AMSTREAM_IOC_SET_PIP2_SCREEN_MODE:
	case AMSTREAM_IOC_GET_PIP2_ZORDER:
	case AMSTREAM_IOC_SET_PIP2_ZORDER:
	case AMSTREAM_IOC_GET_PIP2_DISPLAYPATH:
	case AMSTREAM_IOC_SET_PIP2_DISPLAYPATH:
		layer = &glayer_info[2];
	default:
		break;
	}

	if (file->private_data)
		layer = (struct disp_info_s *)file->private_data;

	switch (cmd) {
	case AMSTREAM_IOC_SET_HDR_INFO:{
#if ENABLE_UPDATE_HDR_FROM_USER
			struct vframe_master_display_colour_s tmp;

			if (copy_from_user(&tmp, argp, sizeof(tmp)) == 0)
				config_hdr_info(tmp);
#endif
		}
		break;
	case AMSTREAM_IOC_SET_OMX_VPTS:{
			u32 pts[6];

			if (copy_from_user(pts, argp, sizeof(pts)) == 0)
				set_omx_pts(pts);
		}
		break;

	case AMSTREAM_IOC_GET_OMX_VPTS:
		put_user(omx_pts, (u32 __user *)argp);
		break;

	case AMSTREAM_IOC_GET_OMX_VERSION:
		put_user(omx_version, (u32 __user *)argp);
		break;

	case AMSTREAM_IOC_GET_OMX_INFO:
		put_user(omx_info, (u32 __user *)argp);
		break;

	case AMSTREAM_IOC_TRICKMODE:
		if (arg == TRICKMODE_I) {
			trickmode_i = 1;
		} else if (arg == TRICKMODE_FFFB) {
			trickmode_fffb = 1;
		} else {
			trickmode_i = 0;
			trickmode_fffb = 0;
		}
		to_notify_trick_wait = false;
		atomic_set(&trickmode_framedone, 0);
		tsync_trick_mode(trickmode_fffb);
		break;

	case AMSTREAM_IOC_TRICK_STAT:
		put_user(atomic_read(&trickmode_framedone),
			 (u32 __user *)argp);
		break;

	case AMSTREAM_IOC_GET_TRICK_VPTS:
		put_user(trickmode_vpts, (u32 __user *)argp);
		break;

	case AMSTREAM_IOC_VPAUSE:
		tsync_avevent(VIDEO_PAUSE, arg);
		break;

	case AMSTREAM_IOC_AVTHRESH:
		tsync_set_avthresh(arg);
		break;

	case AMSTREAM_IOC_SYNCTHRESH:
		tsync_set_syncthresh(arg);
		break;

	case AMSTREAM_IOC_SYNCENABLE:
		tsync_set_enable(arg);
		break;

	case AMSTREAM_IOC_SET_SYNC_ADISCON:
		tsync_set_sync_adiscont(arg);
		break;

	case AMSTREAM_IOC_SET_SYNC_VDISCON:
		tsync_set_sync_vdiscont(arg);
		break;

	case AMSTREAM_IOC_GET_SYNC_ADISCON:
		put_user(tsync_get_sync_adiscont(), (u32 __user *)argp);
		break;

	case AMSTREAM_IOC_GET_SYNC_VDISCON:
		put_user(tsync_get_sync_vdiscont(), (u32 __user *)argp);
		break;

	case AMSTREAM_IOC_GET_SYNC_ADISCON_DIFF:
		put_user(tsync_get_sync_adiscont_diff(), (u32 __user *)argp);
		break;

	case AMSTREAM_IOC_GET_SYNC_VDISCON_DIFF:
		put_user(tsync_get_sync_vdiscont_diff(), (u32 __user *)argp);
		break;

	case AMSTREAM_IOC_SET_SYNC_ADISCON_DIFF:
		tsync_set_sync_adiscont_diff(arg);
		break;

	case AMSTREAM_IOC_SET_SYNC_VDISCON_DIFF:
		tsync_set_sync_vdiscont_diff(arg);
		break;

	case AMSTREAM_IOC_VF_STATUS:{
			struct vframe_states states;

			ret = -EFAULT;
			memset(&states, 0, sizeof(struct vframe_states));
			if (amvideo_vf_get_states(&states) == 0)
				if (copy_to_user(argp,
				    &states, sizeof(states)) == 0)
					ret = 0;
		}
		break;
	case AMSTREAM_IOC_GET_VIDEOPIP2_DISABLE:
		put_user(vd_layer[2].disable_video, (u32 __user *)argp);
		break;
	case AMSTREAM_IOC_GET_VIDEOPIP_DISABLE:
		put_user(vd_layer[1].disable_video, (u32 __user *)argp);
		break;
	case AMSTREAM_IOC_GET_VIDEO_DISABLE:
		put_user(vd_layer[0].disable_video, (u32 __user *)argp);
		break;

	case AMSTREAM_IOC_SET_VIDEOPIP_DISABLE:
		{
			u32 val;

			if (copy_from_user(&val, argp, sizeof(u32)) == 0)
				ret = _videopip_set_disable(VD2_PATH, val);
			else
				ret = -EFAULT;
		}
		break;
	case AMSTREAM_IOC_SET_VIDEOPIP2_DISABLE:
		{
			u32 val;

			if (copy_from_user(&val, argp, sizeof(u32)) == 0)
				ret = _videopip_set_disable(VD3_PATH, val);
			else
				ret = -EFAULT;
		}
		break;

	case AMSTREAM_IOC_SET_VIDEO_DISABLE:
		{
			u32 val;

			if (copy_from_user(&val, argp, sizeof(u32)) == 0)
				ret = _video_set_disable(val);
			else
				ret = -EFAULT;
		}
		break;

	case AMSTREAM_IOC_GET_VIDEO_DISCONTINUE_REPORT:
		put_user(enable_video_discontinue_report, (u32 __user *)argp);
		break;

	case AMSTREAM_IOC_SET_VIDEO_DISCONTINUE_REPORT:
		enable_video_discontinue_report = (arg == 0) ? 0 : 1;
		break;

	case AMSTREAM_IOC_GET_VIDEOPIP_AXIS:
	case AMSTREAM_IOC_GET_VIDEOPIP2_AXIS:
	case AMSTREAM_IOC_GET_VIDEO_AXIS:
		{
			int axis[4];

			axis[0] = layer->layer_left;
			axis[1] = layer->layer_top;
			axis[2] = layer->layer_width;
			axis[3] = layer->layer_height;
			axis[2] = axis[0] + axis[2] - 1;
			axis[3] = axis[1] + axis[3] - 1;
			if (copy_to_user(argp, &axis[0], sizeof(axis)) != 0)
				ret = -EFAULT;
		}
		break;

	case AMSTREAM_IOC_SET_VIDEOPIP_AXIS:
	case AMSTREAM_IOC_SET_VIDEOPIP2_AXIS:
	case AMSTREAM_IOC_SET_VIDEO_AXIS:
		{
			int axis[4];

			if (!(debug_flag & DEBUG_FLAG_AXIS_NO_UPDATE)) {
				if (copy_from_user(axis, argp, sizeof(axis)) == 0)
					_set_video_window(layer, axis);
				else
					ret = -EFAULT;
			} else {
				ret = -EFAULT;
			}
		}
		break;

	case AMSTREAM_IOC_GET_VIDEOPIP_CROP:
	case AMSTREAM_IOC_GET_VIDEOPIP2_CROP:
	case AMSTREAM_IOC_GET_VIDEO_CROP:
		{
			int crop[4];
			{
				crop[0] = layer->crop_top;
				crop[1] = layer->crop_left;
				crop[2] = layer->crop_bottom;
				crop[3] = layer->crop_right;
			}

			if (copy_to_user(argp, &crop[0], sizeof(crop)) != 0)
				ret = -EFAULT;
		}
		break;

	case AMSTREAM_IOC_SET_VIDEOPIP_CROP:
	case AMSTREAM_IOC_SET_VIDEOPIP2_CROP:
	case AMSTREAM_IOC_SET_VIDEO_CROP:
		{
			int crop[4];

			if (copy_from_user(crop, argp, sizeof(crop)) == 0)
				_set_video_crop(layer, crop);
			else
				ret = -EFAULT;
		}
		break;

	case AMSTREAM_IOC_GET_PIP_SCREEN_MODE:
	case AMSTREAM_IOC_GET_PIP2_SCREEN_MODE:
	case AMSTREAM_IOC_GET_SCREEN_MODE:
		if (copy_to_user(argp, &layer->wide_mode, sizeof(u32)) != 0)
			ret = -EFAULT;
		break;

	case AMSTREAM_IOC_SET_PIP_SCREEN_MODE:
	case AMSTREAM_IOC_SET_PIP2_SCREEN_MODE:
	case AMSTREAM_IOC_SET_SCREEN_MODE:
		{
			u32 mode;

			if (copy_from_user(&mode, argp, sizeof(u32)) == 0) {
				if (mode >= VIDEO_WIDEOPTION_MAX) {
					ret = -EINVAL;
				} else if (mode != layer->wide_mode) {
					u8 id = layer->layer_id;

					if (debug_flag & DEBUG_FLAG_BASIC_INFO)
						pr_info("screen_mode ioctl: id:%d, mode %d->%d %s\n",
							id, mode,
							layer->wide_mode,
							current->comm);
					layer->wide_mode = mode;
					vd_layer[id].property_changed = true;
				}
			} else {
				ret = -EFAULT;
			}
		}
		break;

	case AMSTREAM_IOC_GET_BLACKOUT_POLICY:
		if (copy_to_user(argp, &blackout[0], sizeof(u32)) != 0)
			ret = -EFAULT;
		break;

	case AMSTREAM_IOC_SET_BLACKOUT_POLICY:{
			u32 mode;

			if (copy_from_user(&mode, argp, sizeof(u32)) == 0) {
				if (mode > 2)
					ret = -EINVAL;
				else
					blackout[0] = mode;
			} else {
				ret = -EFAULT;
			}
		}
		break;

	case AMSTREAM_IOC_GET_BLACKOUT_PIP_POLICY:
		if (copy_to_user(argp, &blackout[1], sizeof(u32)) != 0)
			ret = -EFAULT;
		break;
	case AMSTREAM_IOC_GET_BLACKOUT_PIP2_POLICY:
		if (copy_to_user(argp, &blackout[2], sizeof(u32)) != 0)
			ret = -EFAULT;
		break;

	case AMSTREAM_IOC_SET_BLACKOUT_PIP_POLICY:{
			u32 mode;

			if (copy_from_user(&mode, argp, sizeof(u32)) == 0) {
				if (mode > 2)
					ret = -EINVAL;
				else
					blackout[1] = mode;
			} else {
				ret = -EFAULT;
			}
		}
		break;
	case AMSTREAM_IOC_SET_BLACKOUT_PIP2_POLICY:{
			u32 mode;

			if (copy_from_user(&mode, argp, sizeof(u32)) == 0) {
				if (mode > 2)
					ret = -EINVAL;
				else
					blackout[2] = mode;
			} else {
				ret = -EFAULT;
			}
		}
		break;

	case AMSTREAM_IOC_CLEAR_VBUF:
		pr_info("Invalid cmd now, skip clear vbuf\n");
		break;

	case AMSTREAM_IOC_CLEAR_VIDEO:
		if (blackout[0])
			safe_switch_videolayer(0, false, false);
		break;

	case AMSTREAM_IOC_CLEAR_PIP_VBUF:
		pr_info("Invalid cmd now, skip clear pip vbuf\n");
		break;

	case AMSTREAM_IOC_CLEAR_VIDEOPIP:
		safe_switch_videolayer(1, false, false);
		break;

	case AMSTREAM_IOC_CLEAR_PIP2_VBUF:
		pr_info("Invalid cmd now, skip clear pip2 vbuf\n");
		break;

	case AMSTREAM_IOC_CLEAR_VIDEOPIP2:
		safe_switch_videolayer(2, false, false);
		break;

	case AMSTREAM_IOC_SET_FREERUN_MODE:
		if (arg > FREERUN_DUR)
			ret = -EFAULT;
		else
			freerun_mode = arg;
		break;

	case AMSTREAM_IOC_GET_FREERUN_MODE:
		put_user(freerun_mode, (u32 __user *)argp);
		break;

	case AMSTREAM_IOC_DISABLE_SLOW_SYNC:
		if (arg)
			disable_slow_sync = 1;
		else
			disable_slow_sync = 0;
		break;
	/*
	 ***************************************************************
	 *3d process ioctl
	 ****************************************************************
	 */
	case AMSTREAM_IOC_SET_3D_TYPE:
		{
#if defined(TV_3D_FUNCTION_OPEN) && defined(CONFIG_AMLOGIC_MEDIA_TVIN)
			unsigned int set_3d =
				VFRAME_EVENT_PROVIDER_SET_3D_VFRAME_INTERLEAVE;
			unsigned int type = (unsigned int)arg;
			struct vframe_s *dispbuf = NULL;

			if (type != process_3d_type) {
				process_3d_type = type;
				if (mvc_flag)
					process_3d_type |= MODE_3D_MVC;
				vd_layer[0].property_changed = true;
				dispbuf = get_dispbuf(0);
				if ((process_3d_type & MODE_3D_FA) &&
				    dispbuf &&
				    !dispbuf->trans_fmt)
					/*notify di 3d mode is frame*/
					  /*alternative mode,passing two*/
					  /*buffer in one frame */
					vf_notify_receiver_by_name
					("deinterlace", set_3d, (void *)1);
				else
					vf_notify_receiver_by_name
					("deinterlace", set_3d, (void *)0);
			}
#endif
			break;
		}
	case AMSTREAM_IOC_GET_3D_TYPE:
#ifdef TV_3D_FUNCTION_OPEN
		put_user(process_3d_type, (u32 __user *)argp);

#endif
		break;
	case AMSTREAM_IOC_GET_SOURCE_VIDEO_3D_TYPE:
#if defined(TV_3D_FUNCTION_OPEN) && defined(CONFIG_AMLOGIC_MEDIA_TVIN)
	{
		u32 source_video_3d_type = VPP_3D_MODE_NULL;

		if (!cur_frame_par[0])
			source_video_3d_type = VPP_3D_MODE_NULL;
		else
			get_vpp_3d_mode
				(process_3d_type,
				cur_frame_par[0]->trans_fmt,
				&source_video_3d_type);
		put_user(source_video_3d_type, (u32 __user *)argp);
	}
#endif
		break;
	case AMSTREAM_IOC_SET_VSYNC_UPINT:
		vsync_pts_inc_upint = arg;
		break;

	case AMSTREAM_IOC_GET_VSYNC_SLOW_FACTOR:
		put_user(vsync_slow_factor, (u32 __user *)argp);
		break;

	case AMSTREAM_IOC_SET_VSYNC_SLOW_FACTOR:
		vsync_slow_factor = arg;
		break;

	case AMSTREAM_IOC_GLOBAL_SET_VIDEOPIP_OUTPUT:
	case AMSTREAM_IOC_GLOBAL_SET_VIDEOPIP2_OUTPUT:
	case AMSTREAM_IOC_GLOBAL_SET_VIDEO_OUTPUT:
		video_set_global_output(layer->layer_id, arg ? 1 : 0);
		break;

	case AMSTREAM_IOC_GLOBAL_GET_VIDEOPIP_OUTPUT:
	case AMSTREAM_IOC_GLOBAL_GET_VIDEOPIP2_OUTPUT:
	case AMSTREAM_IOC_GLOBAL_GET_VIDEO_OUTPUT:
		if (layer->layer_id == 0)
			put_user(vd_layer[0].global_output, (u32 __user *)argp);
		else if (layer->layer_id == 1)
			put_user(vd_layer[1].global_output, (u32 __user *)argp);
		else if (layer->layer_id == 2)
			put_user(vd_layer[2].global_output, (u32 __user *)argp);
		break;

	case AMSTREAM_IOC_GET_VIDEO_LAYER1_ON: {
			u32 vsync_duration;
			u32 video_onoff_diff = 0;

			vsync_duration = vsync_pts_inc / 90;
			video_onoff_diff =
				jiffies_to_msecs(jiffies) -
				vd_layer[0].onoff_time;

			if (vd_layer[0].onoff_state ==
			    VIDEO_ENABLE_STATE_IDLE) {
				/* wait until 5ms after next vsync */
				msleep(video_onoff_diff < vsync_duration
					? vsync_duration - video_onoff_diff + 5
					: 0);
			}
			put_user(vd_layer[0].onoff_state, (u32 __user *)argp);
			break;
		}

	case AMSTREAM_IOC_SET_TUNNEL_MODE: {
		u32 tunnelmode = 0;

		if (copy_from_user(&tunnelmode, argp, sizeof(u32)) == 0)
			tsync_set_tunnel_mode(tunnelmode);
		else
			ret = -EFAULT;
		break;
	}

	case AMSTREAM_IOC_GET_FIRST_FRAME_TOGGLED:
		put_user(first_frame_toggled, (u32 __user *)argp);
		break;

	case AMSTREAM_IOC_SET_VIDEOPEEK:
		videopeek = true;
		nopostvideostart = true;
		break;

	case AMSTREAM_IOC_GET_PIP_ZORDER:
	case AMSTREAM_IOC_GET_PIP2_ZORDER:
	case AMSTREAM_IOC_GET_ZORDER:
		put_user(layer->zorder, (u32 __user *)argp);
		break;

	case AMSTREAM_IOC_SET_PIP_ZORDER:
	case AMSTREAM_IOC_SET_PIP2_ZORDER:
	case AMSTREAM_IOC_SET_ZORDER:{
			u32 zorder, new_prop = 0;

			if (copy_from_user(&zorder, argp, sizeof(u32)) == 0) {
				if (layer->zorder != zorder)
					new_prop = 1;
				layer->zorder = zorder;
				if (layer->layer_id == 0 && new_prop)
					vd_layer[0].property_changed = true;
				else if ((layer->layer_id == 1) && new_prop)
					vd_layer[1].property_changed = true;
			} else {
				ret = -EFAULT;
			}
		}
		break;

	case AMSTREAM_IOC_GET_DISPLAYPATH:
	case AMSTREAM_IOC_GET_PIP_DISPLAYPATH:
	case AMSTREAM_IOC_GET_PIP2_DISPLAYPATH:
		put_user(layer->display_path_id, (s32 __user *)argp);
		break;

	case AMSTREAM_IOC_SET_DISPLAYPATH:
	case AMSTREAM_IOC_SET_PIP_DISPLAYPATH:
	case AMSTREAM_IOC_SET_PIP2_DISPLAYPATH:{
			u32 path_id, new_prop = 0;

			if (copy_from_user(&path_id, argp, sizeof(s32)) == 0) {
				if (layer->display_path_id != path_id) {
					new_prop = 1;
					pr_info
					("VID: VD%d, path_id changed %d->%d\n",
					 layer->layer_id + 1,
					 layer->display_path_id,
					 path_id);
				}
				layer->display_path_id = path_id;
				if (layer->layer_id == 0 && new_prop)
					vd_layer[0].property_changed = true;
				else if ((layer->layer_id == 1) && new_prop)
					vd_layer[1].property_changed = true;
				else if ((layer->layer_id == 2) && new_prop)
					vd_layer[2].property_changed = true;
			} else {
				ret = -EFAULT;
			}
		}
		break;

	case AMSTREAM_IOC_QUERY_LAYER:
		mutex_lock(&video_layer_mutex);
		put_user(layer_cap, (u32 __user *)argp);
		mutex_unlock(&video_layer_mutex);
		ret = 0;
		break;

	case AMSTREAM_IOC_ALLOC_LAYER:
		if (copy_from_user(&layer_id, argp, sizeof(u32)) == 0) {
			if (layer_id >= MAX_VD_LAYERS) {
				ret = -EINVAL;
			} else {
				mutex_lock(&video_layer_mutex);
				if (file->private_data) {
					ret = -EBUSY;
				} else {
					ret = alloc_layer(layer_id);
					if (!ret)
						file->private_data =
						(void *)&glayer_info[layer_id];
				}
				mutex_unlock(&video_layer_mutex);
			}
		} else {
			ret = -EFAULT;
		}
		break;

	case AMSTREAM_IOC_FREE_LAYER:
		mutex_lock(&video_layer_mutex);
		if (!file->private_data) {
			ret = -EINVAL;
		} else {
			ret = free_layer(layer->layer_id);
			if (!ret)
				file->private_data = NULL;
		}
		mutex_unlock(&video_layer_mutex);
		break;

	default:
		return -EINVAL;
	}

	return ret;
}

#ifdef CONFIG_COMPAT
static long amvideo_compat_ioctl(struct file *file, unsigned int cmd, ulong arg)
{
	long ret = 0;

	switch (cmd) {
	case AMSTREAM_IOC_SET_HDR_INFO:
	case AMSTREAM_IOC_SET_OMX_VPTS:
	case AMSTREAM_IOC_GET_OMX_VPTS:
	case AMSTREAM_IOC_GET_OMX_VERSION:
	case AMSTREAM_IOC_GET_OMX_INFO:
	case AMSTREAM_IOC_TRICK_STAT:
	case AMSTREAM_IOC_GET_TRICK_VPTS:
	case AMSTREAM_IOC_GET_SYNC_ADISCON:
	case AMSTREAM_IOC_GET_SYNC_VDISCON:
	case AMSTREAM_IOC_GET_SYNC_ADISCON_DIFF:
	case AMSTREAM_IOC_GET_SYNC_VDISCON_DIFF:
	case AMSTREAM_IOC_VF_STATUS:
	case AMSTREAM_IOC_GET_VIDEO_DISABLE:
	case AMSTREAM_IOC_GET_VIDEO_DISCONTINUE_REPORT:
	case AMSTREAM_IOC_GET_VIDEO_AXIS:
	case AMSTREAM_IOC_SET_VIDEO_AXIS:
	case AMSTREAM_IOC_GET_VIDEO_CROP:
	case AMSTREAM_IOC_SET_VIDEO_CROP:
	case AMSTREAM_IOC_GET_SCREEN_MODE:
	case AMSTREAM_IOC_SET_SCREEN_MODE:
	case AMSTREAM_IOC_GET_BLACKOUT_POLICY:
	case AMSTREAM_IOC_SET_BLACKOUT_POLICY:
	case AMSTREAM_IOC_GET_FREERUN_MODE:
	case AMSTREAM_IOC_GET_3D_TYPE:
	case AMSTREAM_IOC_GET_SOURCE_VIDEO_3D_TYPE:
	case AMSTREAM_IOC_GET_VSYNC_SLOW_FACTOR:
	case AMSTREAM_IOC_GLOBAL_GET_VIDEO_OUTPUT:
	case AMSTREAM_IOC_GET_VIDEO_LAYER1_ON:
	case AMSTREAM_IOC_GLOBAL_SET_VIDEOPIP_OUTPUT:
	case AMSTREAM_IOC_GLOBAL_GET_VIDEOPIP_OUTPUT:
	case AMSTREAM_IOC_GET_VIDEOPIP_DISABLE:
	case AMSTREAM_IOC_SET_VIDEOPIP_DISABLE:
	case AMSTREAM_IOC_GET_VIDEOPIP_AXIS:
	case AMSTREAM_IOC_SET_VIDEOPIP_AXIS:
	case AMSTREAM_IOC_GET_VIDEOPIP_CROP:
	case AMSTREAM_IOC_SET_VIDEOPIP_CROP:
	case AMSTREAM_IOC_GET_PIP_SCREEN_MODE:
	case AMSTREAM_IOC_SET_PIP_SCREEN_MODE:
	case AMSTREAM_IOC_GET_PIP_ZORDER:
	case AMSTREAM_IOC_SET_PIP_ZORDER:
	case AMSTREAM_IOC_GET_ZORDER:
	case AMSTREAM_IOC_SET_ZORDER:
	case AMSTREAM_IOC_GET_DISPLAYPATH:
	case AMSTREAM_IOC_SET_DISPLAYPATH:
	case AMSTREAM_IOC_GET_PIP_DISPLAYPATH:
	case AMSTREAM_IOC_SET_PIP_DISPLAYPATH:
	case AMSTREAM_IOC_QUERY_LAYER:
	case AMSTREAM_IOC_ALLOC_LAYER:
	case AMSTREAM_IOC_FREE_LAYER:
	case AMSTREAM_IOC_GET_PIP2_DISPLAYPATH:
	case AMSTREAM_IOC_SET_PIP2_DISPLAYPATH:
	case AMSTREAM_IOC_GLOBAL_SET_VIDEOPIP2_OUTPUT:
	case AMSTREAM_IOC_GLOBAL_GET_VIDEOPIP2_OUTPUT:
	case AMSTREAM_IOC_GET_VIDEOPIP2_DISABLE:
	case AMSTREAM_IOC_SET_VIDEOPIP2_DISABLE:
	case AMSTREAM_IOC_GET_VIDEOPIP2_AXIS:
	case AMSTREAM_IOC_SET_VIDEOPIP2_AXIS:
	case AMSTREAM_IOC_GET_VIDEOPIP2_CROP:
	case AMSTREAM_IOC_SET_VIDEOPIP2_CROP:
	case AMSTREAM_IOC_GET_PIP2_SCREEN_MODE:
	case AMSTREAM_IOC_SET_PIP2_SCREEN_MODE:
	case AMSTREAM_IOC_GET_PIP2_ZORDER:
	case AMSTREAM_IOC_SET_PIP2_ZORDER:
		arg = (unsigned long)compat_ptr(arg);
		return amvideo_ioctl(file, cmd, arg);
	case AMSTREAM_IOC_TRICKMODE:
	case AMSTREAM_IOC_VPAUSE:
	case AMSTREAM_IOC_AVTHRESH:
	case AMSTREAM_IOC_SYNCTHRESH:
	case AMSTREAM_IOC_SYNCENABLE:
	case AMSTREAM_IOC_SET_SYNC_ADISCON:
	case AMSTREAM_IOC_SET_SYNC_VDISCON:
	case AMSTREAM_IOC_SET_SYNC_ADISCON_DIFF:
	case AMSTREAM_IOC_SET_SYNC_VDISCON_DIFF:
	case AMSTREAM_IOC_SET_VIDEO_DISABLE:
	case AMSTREAM_IOC_SET_VIDEO_DISCONTINUE_REPORT:
	case AMSTREAM_IOC_CLEAR_VBUF:
	case AMSTREAM_IOC_CLEAR_VIDEO:
	case AMSTREAM_IOC_CLEAR_PIP_VBUF:
	case AMSTREAM_IOC_CLEAR_VIDEOPIP:
	case AMSTREAM_IOC_SET_FREERUN_MODE:
	case AMSTREAM_IOC_DISABLE_SLOW_SYNC:
	case AMSTREAM_IOC_SET_3D_TYPE:
	case AMSTREAM_IOC_SET_VSYNC_UPINT:
	case AMSTREAM_IOC_SET_VSYNC_SLOW_FACTOR:
	case AMSTREAM_IOC_GLOBAL_SET_VIDEO_OUTPUT:
	case AMSTREAM_IOC_SET_TUNNEL_MODE:
	case AMSTREAM_IOC_GET_FIRST_FRAME_TOGGLED:
	case AMSTREAM_IOC_SET_VIDEOPEEK:
		return amvideo_ioctl(file, cmd, arg);
	default:
		return -EINVAL;
	}

	return ret;
}
#endif

static unsigned int amvideo_poll(struct file *file, poll_table *wait_table)
{
	poll_wait(file, &amvideo_trick_wait, wait_table);

	if (atomic_read(&trickmode_framedone)) {
		atomic_set(&trickmode_framedone, 0);
		return POLLOUT | POLLWRNORM;
	}

	return 0;
}

static unsigned int amvideo_poll_poll(struct file *file, poll_table *wait_table)
{
	u32 val = 0;

	poll_wait(file, &amvideo_prop_change_wait, wait_table);

	val = atomic_read(&video_prop_change);
	if (val) {
		atomic_set(&status_changed, val);
		atomic_set(&video_prop_change, 0);
		return POLLIN | POLLWRNORM;
	}

	return 0;
}

static const struct file_operations amvideo_fops = {
	.owner = THIS_MODULE,
	.open = amvideo_open,
	.release = amvideo_release,
	.unlocked_ioctl = amvideo_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = amvideo_compat_ioctl,
#endif
	.poll = amvideo_poll,
};

static const struct file_operations amvideo_poll_fops = {
	.owner = THIS_MODULE,
	.open = amvideo_poll_open,
	.release = amvideo_poll_release,
	.poll = amvideo_poll_poll,
};

/*********************************************************
 * SYSFS property functions
 *********************************************************/
#define MAX_NUMBER_PARA 10
#define AMVIDEO_CLASS_NAME "video"
#define AMVIDEO_POLL_CLASS_NAME "video_poll"

static int parse_para(const char *para, int para_num, int *result)
{
	char *token = NULL;
	char *params, *params_base;
	int *out = result;
	int len = 0, count = 0;
	int res = 0;
	int ret = 0;

	if (!para)
		return 0;

	params = kstrdup(para, GFP_KERNEL);
	params_base = params;
	token = params;
	if (token) {
		len = strlen(token);
		do {
			token = strsep(&params, " ");
			if (!token)
				break;
			while (token &&
			       (isspace(*token) ||
				!isgraph(*token)) && len) {
				token++;
				len--;
			}
			if (len == 0)
				break;
			ret = kstrtoint(token, 0, &res);
			if (ret < 0)
				break;
			len = strlen(token);
			*out++ = res;
			count++;
		} while ((count < para_num) && (len > 0));
	}

	kfree(params_base);
	return count;
}

static void set_video_crop(struct disp_info_s *layer,
			   const char *para)
{
	int parsed[4];

	if (likely(parse_para(para, 4, parsed) == 4))
		_set_video_crop(layer, parsed);
	amlog_mask
		(LOG_MASK_SYSFS,
		"video crop=>x0:%d,y0:%d,x1:%d,y1:%d\n ",
		parsed[0], parsed[1], parsed[2], parsed[3]);
}

static void set_video_speed_check(const char *para)
{
	int parsed[2];
	struct disp_info_s *layer = &glayer_info[0];

	if (likely(parse_para(para, 2, parsed) == 2)) {
		layer->speed_check_height = parsed[0];
		layer->speed_check_width = parsed[1];
	}
	amlog_mask
		(LOG_MASK_SYSFS,
		"video speed_check=>h:%d,w:%d\n ",
		parsed[0], parsed[1]);
}

static void set_video_window(struct disp_info_s *layer,
			     const char *para)
{
	int parsed[4];

	if (likely(parse_para(para, 4, parsed) == 4))
		_set_video_window(layer, parsed);
	amlog_mask
		(LOG_MASK_SYSFS,
		"video=>x0:%d,y0:%d,x1:%d,y1:%d\n ",
		parsed[0], parsed[1], parsed[2], parsed[3]);
}

static void parse_param_to_char(char *buf_orig, char **parm)
{
	char *ps, *token;
	unsigned int n = 0;
	char delim1[3] = " ";
	char delim2[2] = "\n";

	ps = buf_orig;
	strcat(delim1, delim2);
	while (1) {
		token = strsep(&ps, delim1);
		if (!token)
			break;
		if (*token == '\0')
			continue;
		parm[n++] = token;
	}
}

static void int_convert_str(int num, char cur_s[], int bit_chose)
{
	char buf[SCENES_CUR_USE_VALUE] = {0};
	int i, count, cur_s_len;

	if (bit_chose == 10)
		snprintf(buf, sizeof(buf), "%d", num);
	else if (bit_chose == 16)
		snprintf(buf, sizeof(buf), "%x", num);

	count = strlen(buf);
	cur_s_len = strlen(cur_s);

	buf[count] = ' ';

	for (i = 0; i < count + 1; i++)
		cur_s[i + cur_s_len] = buf[i];
}

static void str_sapr_conv(const char *s, unsigned int size, int *dest, int num)
{
	int i, j;
	char *s1;
	const char *end;
	unsigned int len;
	long value;

	if (size <= 0 || !s)
		return;

	s1 = kmalloc(size + 1, GFP_KERNEL);
	if (!s1)
		return;

	//len = sizeof(s);
	len = size * num;
	end = s;

	j = 0;
	while (len >= size) {
		for (i = 0; i < size; i++)
			s1[i] = end[i];
		s1[size] = '\0';
		if (kstrtoul(s1, 10, &value) < 0)
			break;
		*dest++ = value;
		end = end + size;
		len -= size;
		j++;
		if (j >= num)
			break;
	}
	kfree(s1);
}

static int is_interlaced(struct vinfo_s *vinfo)
{
	if (!vinfo)
		return 0;
	if (vinfo->mode == VMODE_CVBS)
		return 1;
	if (vinfo->height != vinfo->field_height)
		return 1;
	else
		return 0;
}

static ssize_t video_3d_scale_store(struct class *cla,
				    struct class_attribute *attr,
				    const char *buf, size_t count)
{
#ifdef TV_3D_FUNCTION_OPEN
	u32 enable;
	int r;
	struct disp_info_s *layer = &glayer_info[0];

	r = kstrtouint(buf, 0, &enable);
	if (r < 0)
		return -EINVAL;

	layer->vpp_3d_scale = enable ? true : false;
	vd_layer[0].property_changed = true;
	amlog_mask
		(LOG_MASK_SYSFS,
		"%s:%s 3d scale.\n", __func__,
		enable ? "enable" : "disable");
#endif
	return count;
}

static ssize_t video_sr_show(struct class *cla,
			     struct class_attribute *attr,
			     char *buf)
{
	return sprintf(buf, "super_scaler:%d\n", super_scaler);
}

static ssize_t video_sr_store(struct class *cla,
			      struct class_attribute *attr,
			      const char *buf, size_t count)
{
	int parsed[1];

	mutex_lock(&video_module_mutex);
	if (likely(parse_para(buf, 1, parsed) == 1)) {
		if (super_scaler != (parsed[0] & 0x1)) {
			super_scaler = parsed[0] & 0x1;
			vd_layer[0].property_changed = true;
		}
	}
	mutex_unlock(&video_module_mutex);

	return strnlen(buf, count);
}

static ssize_t video_crop_show(struct class *cla, struct class_attribute *attr,
			       char *buf)
{
	u32 t, l, b, r;
	struct disp_info_s *layer = &glayer_info[0];

	t = layer->crop_top;
	l = layer->crop_left;
	b = layer->crop_bottom;
	r = layer->crop_right;
	return snprintf(buf, 40, "%d %d %d %d\n", t, l, b, r);
}

static ssize_t video_crop_store(struct class *cla,
				struct class_attribute *attr,
				const char *buf, size_t count)
{
	struct disp_info_s *layer = &glayer_info[0];

	mutex_lock(&video_module_mutex);

	set_video_crop(layer, buf);

	mutex_unlock(&video_module_mutex);

	return strnlen(buf, count);
}

static ssize_t real_axis_show(struct class *cla, struct class_attribute *attr,
			       char *buf)
{
	int x_start, y_start, x_end, y_end;
	ssize_t len = 0;
	struct vpp_frame_par_s *_cur_frame_par = cur_frame_par[VD1_PATH];
	struct vinfo_s *vinfo = get_current_vinfo();

	if (!_cur_frame_par)
		return len;
	x_start = _cur_frame_par->VPP_hsc_startp;
	y_start = _cur_frame_par->VPP_vsc_startp;
	x_end = _cur_frame_par->VPP_hsc_endp;
	if (is_interlaced(vinfo))
		y_end = (_cur_frame_par->VPP_vsc_endp << 1) + 1;
	else
		y_end = _cur_frame_par->VPP_vsc_endp;
	return snprintf(buf, 40, "%d %d %d %d\n", x_start, y_start, x_end, y_end);
}

static ssize_t video_axis_show(struct class *cla,
			       struct class_attribute *attr,
			       char *buf)
{
	int x, y, w, h;
	struct disp_info_s *layer = &glayer_info[0];

	x = layer->layer_left;
	y = layer->layer_top;
	w = layer->layer_width;
	h = layer->layer_height;
	return snprintf(buf, 40, "%d %d %d %d\n", x, y, x + w - 1, y + h - 1);
}

static ssize_t video_axis_store(struct class *cla,
				struct class_attribute *attr,
				const char *buf, size_t count)
{
	struct disp_info_s *layer = &glayer_info[0];

	mutex_lock(&video_module_mutex);

	set_video_window(layer, buf);

	mutex_unlock(&video_module_mutex);

	return strnlen(buf, count);
}

static ssize_t video_global_offset_show(struct class *cla,
					struct class_attribute *attr,
					char *buf)
{
	int x, y;
	struct disp_info_s *layer = &glayer_info[0];

	x = layer->global_offset_x;
	y = layer->global_offset_y;

	return snprintf(buf, 40, "%d %d\n", x, y);
}

static ssize_t video_global_offset_store(struct class *cla,
					 struct class_attribute *attr,
					 const char *buf, size_t count)
{
	int parsed[2];
	struct disp_info_s *layer = &glayer_info[0];

	mutex_lock(&video_module_mutex);

	if (likely(parse_para(buf, 2, parsed) == 2)) {
		layer->global_offset_x = parsed[0];
		layer->global_offset_y = parsed[1];
		vd_layer[0].property_changed = true;

		amlog_mask(LOG_MASK_SYSFS,
			   "video_offset=>x0:%d,y0:%d\n ",
			   parsed[0], parsed[1]);
	}

	mutex_unlock(&video_module_mutex);

	return count;
}

static ssize_t video_zoom_show(struct class *cla,
			       struct class_attribute *attr,
			       char *buf)
{
	u32 r;
	struct disp_info_s *layer = &glayer_info[0];

	r = layer->zoom_ratio;

	return snprintf(buf, 40, "%d\n", r);
}

static ssize_t video_zoom_store(struct class *cla,
				struct class_attribute *attr,
				const char *buf, size_t count)
{
	unsigned long r;
	int ret = 0;
	struct disp_info_s *layer = &glayer_info[0];

	ret = kstrtoul(buf, 0, (unsigned long *)&r);
	if (ret < 0)
		return -EINVAL;

	if (r <= MAX_ZOOM_RATIO && r != layer->zoom_ratio) {
		layer->zoom_ratio = r;
		vd_layer[0].property_changed = true;
	}

	return count;
}

static ssize_t video_screen_mode_show(struct class *cla,
				      struct class_attribute *attr, char *buf)
{
	struct disp_info_s *layer = &glayer_info[0];
	static const char * const wide_str[] = {
		"normal", "full stretch", "4-3", "16-9", "non-linear-V",
		"normal-noscaleup",
		"4-3 ignore", "4-3 letter box", "4-3 pan scan", "4-3 combined",
		"16-9 ignore", "16-9 letter box", "16-9 pan scan",
		"16-9 combined", "Custom AR", "AFD", "non-linear-T", "21-9"
	};

	if (layer->wide_mode < ARRAY_SIZE(wide_str)) {
		return sprintf(buf, "%d:%s\n",
			layer->wide_mode,
			wide_str[layer->wide_mode]);
	} else {
		return 0;
	}
}

static ssize_t video_screen_mode_store(struct class *cla,
				       struct class_attribute *attr,
				       const char *buf, size_t count)
{
	unsigned long mode;
	int ret = 0;
	struct disp_info_s *layer = &glayer_info[0];

	ret = kstrtoul(buf, 0, (unsigned long *)&mode);
	if (ret < 0)
		return -EINVAL;

	if (mode < VIDEO_WIDEOPTION_MAX &&
	    mode != layer->wide_mode) {
		if (debug_flag & DEBUG_FLAG_BASIC_INFO)
			pr_info("video_screen_mode sysfs:%d->%ld %s\n",
				layer->wide_mode, mode, current->comm);
		layer->wide_mode = mode;
		vd_layer[0].property_changed = true;
	}

	return count;
}

static ssize_t video_blackout_policy_show(struct class *cla,
					  struct class_attribute *attr,
					  char *buf)
{
	return sprintf(buf, "%d\n", blackout[0]);
}

static ssize_t video_blackout_policy_store(struct class *cla,
					   struct class_attribute *attr,
					   const char *buf, size_t count)
{
	int r;

	r = kstrtouint(buf, 0, &blackout[0]);
	if (r < 0)
		return -EINVAL;

	if (debug_flag & DEBUG_FLAG_BASIC_INFO)
		pr_info("%s(%d)\n", __func__, blackout[0]);

	return count;
}

static ssize_t video_seek_flag_show(struct class *cla,
				    struct class_attribute *attr,
				    char *buf)
{
	return sprintf(buf, "%d\n", video_seek_flag);
}

static ssize_t video_seek_flag_store(struct class *cla,
				     struct class_attribute *attr,
				     const char *buf, size_t count)
{
	int r;

	r = kstrtouint(buf, 0, &video_seek_flag);
	if (r < 0)
		return -EINVAL;

	return count;
}

static ssize_t video_max_src_show(struct class *cla,
			       struct class_attribute *attr,
			       char *buf)
{
	int src_width, src_height;
	struct disp_info_s *layer = &glayer_info[0];

	src_width = layer->src_width_max;
	src_height = layer->src_height_max;
	return snprintf(buf, 40, "%d %d\n", src_width, src_height);
}

#ifdef PTS_TRACE_DEBUG
static ssize_t pts_trace_show(struct class *cla,
			      struct class_attribute *attr,
			      char *buf)
{
	return sprintf(buf, "%d %d %d %d %d %d %d %d\n"
				"%d %d %d %d %d %d %d %d\n"
				"%0x %0x %0x %0x %0x %0x %0x %0x\n"
				"%0x %0x %0x %0x %0x %0x %0x %0x\n"
				"%0x %0x %0x %0x %0x %0x %0x %0x\n"
				"%0x %0x %0x %0x %0x %0x %0x %0x\n",
		pts_trace_his[0], pts_trace_his[1], pts_trace_his[2],
		pts_trace_his[3], pts_trace_his[4], pts_trace_his[5],
		pts_trace_his[6], pts_trace_his[7], pts_trace_his[8],
		pts_trace_his[9], pts_trace_his[10], pts_trace_his[11],
		pts_trace_his[12], pts_trace_his[13], pts_trace_his[14],
		pts_trace_his[15],
		pts_his[0], pts_his[1], pts_his[2], pts_his[3],
		pts_his[4], pts_his[5], pts_his[6], pts_his[7],
		pts_his[8], pts_his[9], pts_his[10], pts_his[11],
		pts_his[12], pts_his[13], pts_his[14], pts_his[15],
		scr_his[0], scr_his[1], scr_his[2], scr_his[3],
		scr_his[4], scr_his[5], scr_his[6], scr_his[7],
		scr_his[8], scr_his[9], scr_his[10], scr_his[11],
		scr_his[12], scr_his[13], scr_his[14], scr_his[15]);
}
#endif

static ssize_t video_brightness_show(struct class *cla,
				     struct class_attribute *attr, char *buf)
{
	s32 val = 0;

	if (cur_dev->display_module != S5_DISPLAY_MODULE)
		val = (READ_VCBUS_REG(VPP_VADJ1_Y + cur_dev->vpp_off) >> 8) &
				0x1ff;

	val = (val << 23) >> 23;

	return sprintf(buf, "%d\n", val);
}

static ssize_t video_brightness_store(struct class *cla,
				      struct class_attribute *attr,
				      const char *buf, size_t count)
{
	int r;
	int val;

	r = kstrtoint(buf, 0, &val);
	if (r < 0 || val < -255 || val > 255)
		return -EINVAL;

	if (cur_dev->display_module != S5_DISPLAY_MODULE) {
#ifndef CONFIG_AMLOGIC_REMOVE_OLD
		if (get_cpu_type() <= MESON_CPU_MAJOR_ID_GXTVBB) {
			WRITE_VCBUS_REG_BITS(VPP_VADJ1_Y +
				cur_dev->vpp_off, val, 8, 9);
		} else {
#else
		{
#endif
			WRITE_VCBUS_REG_BITS(VPP_VADJ1_Y +
				cur_dev->vpp_off, val << 1, 8, 10);
		}
		WRITE_VCBUS_REG(VPP_VADJ_CTRL + cur_dev->vpp_off, VPP_VADJ1_EN);
	}
	return count;
}

static ssize_t video_contrast_show(struct class *cla,
				   struct class_attribute *attr, char *buf)
{
	int val = 0;

	if (cur_dev->display_module != S5_DISPLAY_MODULE)
		val = (int)(READ_VCBUS_REG(VPP_VADJ1_Y + cur_dev->vpp_off) &
			     0xff) - 0x80;
	return sprintf(buf, "%d\n", val);
}

static ssize_t video_contrast_store(struct class *cla,
				    struct class_attribute *attr,
				    const char *buf, size_t count)
{
	int r;
	int val;

	r = kstrtoint(buf, 0, &val);
	if (r < 0 || val < -127 || val > 127)
		return -EINVAL;

	val += 0x80;
	if (cur_dev->display_module != S5_DISPLAY_MODULE) {
		WRITE_VCBUS_REG_BITS(VPP_VADJ1_Y + cur_dev->vpp_off, val, 0, 8);
		WRITE_VCBUS_REG(VPP_VADJ_CTRL + cur_dev->vpp_off, VPP_VADJ1_EN);
	}
	return count;
}

static ssize_t vpp_brightness_show(struct class *cla,
				   struct class_attribute *attr, char *buf)
{
	s32 val = 0;

	if (cur_dev->display_module != S5_DISPLAY_MODULE)
		val = (READ_VCBUS_REG(VPP_VADJ2_Y +
			cur_dev->vpp_off) >> 8) & 0x1ff;

	val = (val << 23) >> 23;

	return sprintf(buf, "%d\n", val);
}

static ssize_t vpp_brightness_store(struct class *cla,
				    struct class_attribute *attr,
				    const char *buf, size_t count)
{
	int r;
	int val;

	r = kstrtoint(buf, 0, &val);
	if (r < 0 || val < -255 || val > 255)
		return -EINVAL;
	if (cur_dev->display_module != S5_DISPLAY_MODULE) {
#ifndef CONFIG_AMLOGIC_REMOVE_OLD
		if (get_cpu_type() <= MESON_CPU_MAJOR_ID_GXTVBB) {
			WRITE_VCBUS_REG_BITS(VPP_VADJ2_Y +
				cur_dev->vpp_off, val, 8, 9);
		} else {
#else
		{
#endif
			WRITE_VCBUS_REG_BITS(VPP_VADJ2_Y +
				cur_dev->vpp_off, val << 1, 8, 10);
		}
		WRITE_VCBUS_REG(VPP_VADJ_CTRL + cur_dev->vpp_off, VPP_VADJ2_EN);
	}
	return count;
}

static ssize_t vpp_contrast_show(struct class *cla,
				 struct class_attribute *attr,
				 char *buf)
{
	int val = 0;

	if (cur_dev->display_module != S5_DISPLAY_MODULE)
		val = (int)(READ_VCBUS_REG(VPP_VADJ2_Y + cur_dev->vpp_off) &
			     0xff) - 0x80;
	return sprintf(buf, "%d\n", val);
}

static ssize_t vpp_contrast_store(struct class *cla,
				  struct class_attribute *attr,
				  const char *buf,
				  size_t count)
{
	int r;
	int val;

	r = kstrtoint(buf, 0, &val);
	if (r < 0 || val < -127 || val > 127)
		return -EINVAL;

	val += 0x80;
	if (cur_dev->display_module != S5_DISPLAY_MODULE) {
		WRITE_VCBUS_REG_BITS(VPP_VADJ2_Y + cur_dev->vpp_off, val, 0, 8);
		WRITE_VCBUS_REG(VPP_VADJ_CTRL + cur_dev->vpp_off, VPP_VADJ2_EN);
	}

	return count;
}

static ssize_t video_saturation_show(struct class *cla,
				     struct class_attribute *attr, char *buf)
{
	int val = 0;

	if (cur_dev->display_module != S5_DISPLAY_MODULE)
		val = READ_VCBUS_REG(VPP_VADJ1_Y + cur_dev->vpp_off) & 0xff;

	return sprintf(buf, "%d\n", val);
}

static ssize_t video_saturation_store(struct class *cla,
				      struct class_attribute *attr,
				      const char *buf, size_t count)
{
	int r;
	int val;

	r = kstrtoint(buf, 0, &val);
	if (r < 0 || val < -127 || val > 127)
		return -EINVAL;
	if (cur_dev->display_module != S5_DISPLAY_MODULE) {
		WRITE_VCBUS_REG_BITS(VPP_VADJ1_Y + cur_dev->vpp_off, val, 0, 8);
		WRITE_VCBUS_REG(VPP_VADJ_CTRL + cur_dev->vpp_off, VPP_VADJ1_EN);
	}

	return count;
}

static ssize_t vpp_saturation_hue_show(struct class *cla,
				       struct class_attribute *attr, char *buf)
{
	int val = 0;

	if (cur_dev->display_module != S5_DISPLAY_MODULE)
		val = READ_VCBUS_REG(VPP_VADJ2_MA_MB);
	return sprintf(buf, "0x%x\n", val);
}

static ssize_t vpp_saturation_hue_store(struct class *cla,
					struct class_attribute *attr,
					const char *buf, size_t count)
{
	int r;
	s32 mab = 0;
	s16 mc = 0, md = 0;

	r = kstrtoint(buf, 0, &mab);
	if (r < 0 || (mab & 0xfc00fc00))
		return -EINVAL;
	if (cur_dev->display_module != S5_DISPLAY_MODULE) {
		WRITE_VCBUS_REG(VPP_VADJ2_MA_MB, mab);
		mc = (s16)((mab << 22) >> 22);	/* mc = -mb */
		mc = 0 - mc;
		if (mc > 511)
			mc = 511;
		if (mc < -512)
			mc = -512;
		md = (s16)((mab << 6) >> 22);	/* md =  ma; */
		mab = ((mc & 0x3ff) << 16) | (md & 0x3ff);
		WRITE_VCBUS_REG(VPP_VADJ2_MC_MD, mab);
		/* WRITE_MPEG_REG(VPP_VADJ_CTRL, 1); */
		WRITE_VCBUS_REG_BITS(VPP_VADJ_CTRL + cur_dev->vpp_off, 1, 2, 1);
	}
#ifdef PQ_DEBUG_EN
	pr_info("\n[amvideo..] set vpp_saturation OK!!!\n");
#endif
	return count;
}

/* [   24] 1/enable, 0/disable */
/* [23:16] Y */
/* [15: 8] Cb */
/* [ 7: 0] Cr */
static ssize_t video_test_screen_show(struct class *cla,
				      struct class_attribute *attr,
				      char *buf)
{
	return sprintf(buf, "0x%x\n", test_screen);
}

/* [24]    Flag: enable/disable auto background color */
/* [23:16] Y */
/* [15: 8] Cb */
/* [ 7: 0] Cr */
static ssize_t video_background_show(struct class *cla,
				      struct class_attribute *attr,
				      char *buf)
{
	return sprintf(buf, "channel_bg(0x%x) no_channel_bg(0x%x)\n",
		       vd_layer[0].video_en_bg_color,
		       vd_layer[0].video_dis_bg_color);
}

static ssize_t video_rgb_screen_show(struct class *cla,
				     struct class_attribute *attr,
				     char *buf)
{
	return sprintf(buf, "0x%x\n", rgb_screen);
}

static ssize_t enable_hdmi_delay_check_show(struct class *cla,
				      struct class_attribute *attr,
					  char *buf)
{
	return sprintf(buf, "%d\n", enable_hdmi_delay_normal_check);
}

static ssize_t enable_hdmi_delay_check_store(struct class *cla,
				      struct class_attribute *attr,
				      const char *buf,
					  size_t count)
{
	int r;
	int value;

	r = kstrtoint(buf, 0, &value);
	if (r < 0)
		return -EINVAL;

	enable_hdmi_delay_normal_check = value >= 0 ? (u8)value : 0;
	return count;
}

static ssize_t hdmi_delay_debug_show(struct class *cla,
				      struct class_attribute *attr,
					  char *buf)
{
	return sprintf(buf, "%d\n", hdmin_delay_count_debug);
}
#define SCALE 6

#ifndef CONFIG_AMLOGIC_REMOVE_OLD
static short R_Cr[] = { -11484, -11394, -11305, -11215, -11125,
-11036, -10946, -10856, -10766, -10677, -10587, -10497, -10407,
-10318, -10228, -10138, -10049, -9959, -9869, -9779, -9690, -9600,
-9510, -9420, -9331, -9241, -9151, -9062, -8972, -8882, -8792, -8703,
-8613, -8523, -8433, -8344, -8254, -8164, -8075, -7985, -7895, -7805,
-7716, -7626, -7536, -7446, -7357, -7267, -7177, -7088, -6998, -6908,
-6818, -6729, -6639, -6549, -6459, -6370, -6280, -6190, -6101, -6011,
-5921, -5831, -5742, -5652, -5562, -5472, -5383, -5293, -5203, -5113,
-5024, -4934, -4844, -4755, -4665, -4575, -4485, -4396, -4306, -4216,
-4126, -4037, -3947, -3857, -3768, -3678, -3588, -3498, -3409, -3319,
-3229, -3139, -3050, -2960, -2870, -2781, -2691, -2601, -2511, -2422,
-2332, -2242, -2152, -2063, -1973, -1883, -1794, -1704, -1614, -1524,
-1435, -1345, -1255, -1165, -1076, -986, -896, -807, -717, -627, -537,
-448, -358, -268, -178, -89, 0, 90, 179, 269, 359, 449, 538, 628, 718,
808, 897, 987, 1077, 1166, 1256, 1346, 1436, 1525, 1615, 1705, 1795,
1884, 1974, 2064, 2153, 2243, 2333, 2423, 2512, 2602, 2692, 2782,
2871, 2961, 3051, 3140, 3230, 3320, 3410, 3499, 3589, 3679, 3769,
3858, 3948, 4038, 4127, 4217, 4307, 4397, 4486, 4576, 4666, 4756,
4845, 4935, 5025, 5114, 5204, 5294, 5384, 5473, 5563, 5653, 5743,
5832, 5922, 6012, 6102, 6191, 6281, 6371, 6460, 6550, 6640, 6730,
6819, 6909, 6999, 7089, 7178, 7268, 7358, 7447, 7537, 7627, 7717,
7806, 7896, 7986, 8076, 8165, 8255, 8345, 8434, 8524, 8614, 8704,
8793, 8883, 8973, 9063, 9152, 9242, 9332, 9421, 9511, 9601, 9691,
9780, 9870, 9960, 10050, 10139, 10229, 10319, 10408, 10498, 10588,
10678, 10767, 10857, 10947, 11037, 11126, 11216, 11306, 11395 };

static short G_Cb[] = { 2819, 2797, 2775, 2753, 2731, 2709, 2687,
2665, 2643, 2621, 2599, 2577, 2555, 2533, 2511, 2489, 2467, 2445,
2423, 2401, 2379, 2357, 2335, 2313, 2291, 2269, 2247, 2225, 2202,
2180, 2158, 2136, 2114, 2092, 2070, 2048, 2026, 2004, 1982, 1960,
1938, 1916, 1894, 1872, 1850, 1828, 1806, 1784, 1762, 1740, 1718,
1696, 1674, 1652, 1630, 1608, 1586, 1564, 1542, 1520, 1498, 1476,
1454, 1432, 1410, 1388, 1366, 1344, 1321, 1299, 1277, 1255, 1233,
1211, 1189, 1167, 1145, 1123, 1101, 1079, 1057, 1035, 1013, 991, 969,
947, 925, 903, 881, 859, 837, 815, 793, 771, 749, 727, 705, 683, 661,
639, 617, 595, 573, 551, 529, 507, 485, 463, 440, 418, 396, 374, 352,
330, 308, 286, 264, 242, 220, 198, 176, 154, 132, 110, 88, 66, 44, 22,
0, -21, -43, -65, -87, -109, -131, -153, -175, -197, -219, -241, -263,
-285, -307, -329, -351, -373, -395, -417, -439, -462, -484, -506,
-528, -550, -572, -594, -616, -638, -660, -682, -704, -726, -748,
-770, -792, -814, -836, -858, -880, -902, -924, -946, -968, -990,
-1012, -1034, -1056, -1078, -1100, -1122, -1144, -1166, -1188, -1210,
-1232, -1254, -1276, -1298, -1320, -1343, -1365, -1387, -1409, -1431,
-1453, -1475, -1497, -1519, -1541, -1563, -1585, -1607, -1629, -1651,
-1673, -1695, -1717, -1739, -1761, -1783, -1805, -1827, -1849, -1871,
-1893, -1915, -1937, -1959, -1981, -2003, -2025, -2047, -2069, -2091,
-2113, -2135, -2157, -2179, -2201, -2224, -2246, -2268, -2290, -2312,
-2334, -2356, -2378, -2400, -2422, -2444, -2466, -2488, -2510, -2532,
-2554, -2576, -2598, -2620, -2642, -2664, -2686, -2708, -2730, -2752,
-2774, -2796 };

static short G_Cr[] = { 5850, 5805, 5759, 5713, 5667, 5622, 5576,
5530, 5485, 5439, 5393, 5347, 5302, 5256, 5210, 5165, 5119, 5073,
5028, 4982, 4936, 4890, 4845, 4799, 4753, 4708, 4662, 4616, 4570,
4525, 4479, 4433, 4388, 4342, 4296, 4251, 4205, 4159, 4113, 4068,
4022, 3976, 3931, 3885, 3839, 3794, 3748, 3702, 3656, 3611, 3565,
3519, 3474, 3428, 3382, 3336, 3291, 3245, 3199, 3154, 3108, 3062,
3017, 2971, 2925, 2879, 2834, 2788, 2742, 2697, 2651, 2605, 2559,
2514, 2468, 2422, 2377, 2331, 2285, 2240, 2194, 2148, 2102, 2057,
2011, 1965, 1920, 1874, 1828, 1782, 1737, 1691, 1645, 1600, 1554,
1508, 1463, 1417, 1371, 1325, 1280, 1234, 1188, 1143, 1097, 1051,
1006, 960, 914, 868, 823, 777, 731, 686, 640, 594, 548, 503, 457, 411,
366, 320, 274, 229, 183, 137, 91, 46, 0, -45, -90, -136, -182, -228,
-273, -319, -365, -410, -456, -502, -547, -593, -639, -685, -730,
-776, -822, -867, -913, -959, -1005, -1050, -1096, -1142, -1187,
-1233, -1279, -1324, -1370, -1416, -1462, -1507, -1553, -1599, -1644,
-1690, -1736, -1781, -1827, -1873, -1919, -1964, -2010, -2056, -2101,
-2147, -2193, -2239, -2284, -2330, -2376, -2421, -2467, -2513, -2558,
-2604, -2650, -2696, -2741, -2787, -2833, -2878, -2924, -2970, -3016,
-3061, -3107, -3153, -3198, -3244, -3290, -3335, -3381, -3427, -3473,
-3518, -3564, -3610, -3655, -3701, -3747, -3793, -3838, -3884, -3930,
-3975, -4021, -4067, -4112, -4158, -4204, -4250, -4295, -4341, -4387,
-4432, -4478, -4524, -4569, -4615, -4661, -4707, -4752, -4798, -4844,
-4889, -4935, -4981, -5027, -5072, -5118, -5164, -5209, -5255, -5301,
-5346, -5392, -5438, -5484, -5529, -5575, -5621, -5666, -5712, -5758,
-5804 };

static short B_Cb[] = { -14515, -14402, -14288, -14175, -14062,
-13948, -13835, -13721, -13608, -13495, -13381, -13268, -13154,
-13041, -12928, -12814, -12701, -12587, -12474, -12360, -12247,
-12134, -12020, -11907, -11793, -11680, -11567, -11453, -11340,
-11226, -11113, -11000, -10886, -10773, -10659, -10546, -10433,
-10319, -10206, -10092, -9979, -9865, -9752, -9639, -9525, -9412,
-9298, -9185, -9072, -8958, -8845, -8731, -8618, -8505, -8391, -8278,
-8164, -8051, -7938, -7824, -7711, -7597, -7484, -7371, -7257, -7144,
-7030, -6917, -6803, -6690, -6577, -6463, -6350, -6236, -6123, -6010,
-5896, -5783, -5669, -5556, -5443, -5329, -5216, -5102, -4989, -4876,
-4762, -4649, -4535, -4422, -4309, -4195, -4082, -3968, -3855, -3741,
-3628, -3515, -3401, -3288, -3174, -3061, -2948, -2834, -2721, -2607,
-2494, -2381, -2267, -2154, -2040, -1927, -1814, -1700, -1587, -1473,
-1360, -1246, -1133, -1020, -906, -793, -679, -566, -453, -339, -226,
-112, 0, 113, 227, 340, 454, 567, 680, 794, 907, 1021, 1134, 1247,
1361, 1474, 1588, 1701, 1815, 1928, 2041, 2155, 2268, 2382, 2495,
2608, 2722, 2835, 2949, 3062, 3175, 3289, 3402, 3516, 3629, 3742,
3856, 3969, 4083, 4196, 4310, 4423, 4536, 4650, 4763, 4877, 4990,
5103, 5217, 5330, 5444, 5557, 5670, 5784, 5897, 6011, 6124, 6237,
6351, 6464, 6578, 6691, 6804, 6918, 7031, 7145, 7258, 7372, 7485,
7598, 7712, 7825, 7939, 8052, 8165, 8279, 8392, 8506, 8619, 8732,
8846, 8959, 9073, 9186, 9299, 9413, 9526, 9640, 9753, 9866, 9980,
10093, 10207, 10320, 10434, 10547, 10660, 10774, 10887, 11001, 11114,
11227, 11341, 11454, 11568, 11681, 11794, 11908, 12021, 12135, 12248,
12361, 12475, 12588, 12702, 12815, 12929, 13042, 13155, 13269, 13382,
13496, 13609, 13722, 13836, 13949, 14063, 14176, 14289, 14403
};

static u32 yuv2rgb(u32 yuv)
{
	int y = (yuv >> 16) & 0xff;
	int cb = (yuv >> 8) & 0xff;
	int cr = yuv & 0xff;
	int r, g, b;

	r = y + ((R_Cr[cr]) >> SCALE);
	g = y + ((G_Cb[cb] + G_Cr[cr]) >> SCALE);
	b = y + ((B_Cb[cb]) >> SCALE);

	r = r - 16;
	if (r < 0)
		r = 0;
	r = r * 1164 / 1000;
	g = g - 16;
	if (g < 0)
		g = 0;
	g = g * 1164 / 1000;
	b = b - 16;
	if (b < 0)
		b = 0;
	b = b * 1164 / 1000;

	r = (r <= 0) ? 0 : (r >= 255) ? 255 : r;
	g = (g <= 0) ? 0 : (g >= 255) ? 255 : g;
	b = (b <= 0) ? 0 : (b >= 255) ? 255 : b;

	return  (r << 16) | (g << 8) | b;
}
#endif

/* 8bit convert to 10bit */
static u32 eight2ten(u32 yuv)
{
	int y = (yuv >> 16) & 0xff;
	int cb = (yuv >> 8) & 0xff;
	int cr = yuv & 0xff;
	u32 data32;

	/* txlx need check vd1 path bit width by s2u registers */
	if (get_cpu_type() == MESON_CPU_MAJOR_ID_TXLX) {
		data32 = READ_VCBUS_REG(0x1d94) & 0xffff;
		if (data32 == 0x2000 ||
		    data32 == 0x800)
			return  ((y << 20) << 2) |
			((cb << 10) << 2) | (cr << 2);
		else
			return  (y << 20) | (cb << 10) | cr;
	} else {
		return  (y << 20) | (cb << 10) | cr;
	}
}

static u32 rgb2yuv(u32 rgb)
{
	int r = (rgb >> 16) & 0xff;
	int g = (rgb >> 8) & 0xff;
	int b = rgb & 0xff;
	int y, u, v;

	y = ((47 * r + 157 * g + 16 * b + 128) >> 8) + 16;
	u = ((-26 * r - 87 * g + 113 * b + 128) >> 8) + 128;
	v = ((112 * r - 102 * g - 10 * b + 128) >> 8) + 128;

	return  (y << 16) | (u << 8) | v;
}

static ssize_t video_test_screen_store(struct class *cla,
				       struct class_attribute *attr,
				       const char *buf, size_t count)
{
	int r;
	unsigned int data = 0x0;

	r = kstrtouint(buf, 0, &test_screen);
	if (r < 0)
		return -EINVAL;

#if DEBUG_TMP
	if (test_screen & 0x04000000)
		data |= VPP_VD2_PREBLEND;
	else
		data &= (~VPP_VD2_PREBLEND);

	if (test_screen & 0x08000000)
		data |= VPP_VD2_POSTBLEND;
	else
		data &= (~VPP_VD2_POSTBLEND);
#endif

	/* show test screen  YUV blend*/
	/* force as black 0x008080 for dolbyvision stb ipt blend */
	if (!legacy_vpp) {
		if (cur_dev->display_module != S5_DISPLAY_MODULE) {
			if (is_amdv_enable() &&
			    is_amdv_stb_mode())
				WRITE_VCBUS_REG
				(VPP_POST_BLEND_BLEND_DUMMY_DATA,
				 0x00008080);
			else
				WRITE_VCBUS_REG
				(VPP_POST_BLEND_BLEND_DUMMY_DATA,
				 test_screen & 0x00ffffff);
		} else {
			struct vpp_post_blend_reg_s *vpp_reg = &vpp_post_reg.vpp_post_blend_reg;

			if (is_amdv_enable() &&
			    is_amdv_stb_mode())
				WRITE_VCBUS_REG
				(vpp_reg->vpp_post_blend_blend_dummy_data,
				 0x00008080);
			else
				WRITE_VCBUS_REG
				(vpp_reg->vpp_post_blend_blend_dummy_data,
				 test_screen & 0x00ffffff);
		}
	}
#ifndef CONFIG_AMLOGIC_REMOVE_OLD
	else if (is_meson_gxm_cpu() ||
		 (get_cpu_type() == MESON_CPU_MAJOR_ID_TXLX))
		/* bit width change to 10bit in gxm, 10/12 in txlx*/
		WRITE_VCBUS_REG
			(VPP_DUMMY_DATA1,
			eight2ten(test_screen & 0x00ffffff));
	else if (get_cpu_type() == MESON_CPU_MAJOR_ID_GXTVBB)
		WRITE_VCBUS_REG
			(VPP_DUMMY_DATA1,
			yuv2rgb(test_screen & 0x00ffffff));
	else if (get_cpu_type() < MESON_CPU_MAJOR_ID_GXTVBB)
		if (READ_VCBUS_REG(VIU_OSD1_BLK0_CFG_W0) & 0x80)
			WRITE_VCBUS_REG
				(VPP_DUMMY_DATA1,
				test_screen & 0x00ffffff);
		else /* RGB blend */
			WRITE_VCBUS_REG
				(VPP_DUMMY_DATA1,
				yuv2rgb(test_screen & 0x00ffffff));
#endif
	else
		WRITE_VCBUS_REG(VPP_DUMMY_DATA1,
				test_screen & 0x00ffffff);
	if (debug_flag & DEBUG_FLAG_BASIC_INFO) {
		pr_info("%s write(VPP_MISC,%x) write(VPP_DUMMY_DATA1, %x)\n",
			__func__, data, test_screen & 0x00ffffff);
	}

	return count;
}

/* [24]    Flag: enable/disable auto background color */
/* [23:16] Y */
/* [15: 8] Cb */
/* [ 7: 0] Cr */
static ssize_t video_background_store(struct class *cla,
				       struct class_attribute *attr,
				       const char *buf, size_t count)
{
	int parsed[2];

	if (likely(parse_para(buf, 2, parsed) == 2)) {
		pr_info("video bg color, channel(0x%x) no_channel(0x%x)\n",
			 parsed[0], parsed[1]);
		vd_layer[0].video_en_bg_color = parsed[0];
		vd_layer[0].video_dis_bg_color = parsed[1];
	} else {
		pr_err("video_background: wrong input params\n");
		return -EINVAL;
	}

	return count;
}

static ssize_t video_rgb_screen_store(struct class *cla,
				      struct class_attribute *attr,
				      const char *buf, size_t count)
{
	int r;
	u32 yuv_eight;

	/* unsigned data = 0x0; */
	r = kstrtouint(buf, 0, &rgb_screen);
	if (r < 0)
		return -EINVAL;

#if DEBUG_TMP
	/* vdin0 pre post blend enable or disabled */

	data = READ_VCBUS_REG(VPP_MISC);
	if (rgb_screen & 0x01000000)
		data |= VPP_VD1_PREBLEND;
	else
		data &= (~VPP_VD1_PREBLEND);

	if (rgb_screen & 0x02000000)
		data |= VPP_VD1_POSTBLEND;
	else
		data &= (~VPP_VD1_POSTBLEND);

	if (test_screen & 0x04000000)
		data |= VPP_VD2_PREBLEND;
	else
		data &= (~VPP_VD2_PREBLEND);

	if (test_screen & 0x08000000)
		data |= VPP_VD2_POSTBLEND;
	else
		data &= (~VPP_VD2_POSTBLEND);
#endif
	/* show test screen  YUV blend*/
	yuv_eight = rgb2yuv(rgb_screen & 0x00ffffff);
	if (!legacy_vpp) {
		if (cur_dev->display_module != S5_DISPLAY_MODULE) {
			WRITE_VCBUS_REG
				(VPP_POST_BLEND_BLEND_DUMMY_DATA,
				yuv_eight & 0x00ffffff);
			if (amvideo_meson_dev.has_vpp1) {
				WRITE_VCBUS_REG(VPP1_BLEND_BLEND_DUMMY_DATA,
					yuv_eight & 0x00ffffff);
			}
			if (amvideo_meson_dev.has_vpp2) {
				WRITE_VCBUS_REG(VPP2_BLEND_BLEND_DUMMY_DATA,
					yuv_eight & 0x00ffffff);
			}
		} else {
			struct vpp_post_blend_reg_s *vpp_reg = &vpp_post_reg.vpp_post_blend_reg;
			struct vpp1_post_blend_reg_s *vpp1_reg = &vpp_post_reg.vpp1_post_blend_reg;

			WRITE_VCBUS_REG
				(vpp_reg->vpp_post_blend_blend_dummy_data,
				yuv_eight & 0x00ffffff);
			if (amvideo_meson_dev.has_vpp1) {
				WRITE_VCBUS_REG(vpp1_reg->vpp_post_blend_blend_dummy_data,
					yuv_eight & 0x00ffffff);
			}
		}
	} else if (is_meson_gxtvbb_cpu()) {
#ifndef CONFIG_AMLOGIC_REMOVE_OLD
		WRITE_VCBUS_REG(VPP_DUMMY_DATA1,
				rgb_screen & 0x00ffffff);
#endif
	} else if (cpu_after_eq(MESON_CPU_MAJOR_ID_TXL)) {
		WRITE_VCBUS_REG(VPP_DUMMY_DATA1,
				eight2ten(yuv_eight & 0x00ffffff));
	}
	/* WRITE_VCBUS_REG(VPP_MISC, data); */

	if (debug_flag & DEBUG_FLAG_BASIC_INFO) {
		pr_info("%s write(VPP_DUMMY_DATA1, %x)\n",
			__func__, rgb_screen & 0x00ffffff);
	}
	return count;
}

static ssize_t video_nonlinear_factor_show(struct class *cla,
					   struct class_attribute *attr,
					   char *buf)
{
	u32 factor;
	struct disp_info_s *layer = &glayer_info[0];

	factor = vpp_get_nonlinear_factor(layer);

	return sprintf(buf, "%d\n", factor);
}

static ssize_t video_nonlinear_factor_store(struct class *cla,
					    struct class_attribute *attr,
					    const char *buf, size_t count)
{
	int r;
	u32 factor;
	struct disp_info_s *layer = &glayer_info[0];

	r = kstrtouint(buf, 0, &factor);
	if (r < 0)
		return -EINVAL;

	if (vpp_set_nonlinear_factor(layer, factor) == 0)
		vd_layer[0].property_changed = true;

	return count;
}

static ssize_t video_nonlinear_t_factor_show(struct class *cla,
					   struct class_attribute *attr,
					   char *buf)
{
	u32 factor;
	struct disp_info_s *layer = &glayer_info[0];

	factor = vpp_get_nonlinear_t_factor(layer);

	return sprintf(buf, "%d\n", factor);
}

static ssize_t video_nonlinear_t_factor_store(struct class *cla,
					    struct class_attribute *attr,
					    const char *buf, size_t count)
{
	int r;
	u32 factor;
	struct disp_info_s *layer = &glayer_info[0];

	r = kstrtouint(buf, 0, &factor);
	if (r < 0)
		return -EINVAL;

	if (vpp_set_nonlinear_t_factor(layer, factor) == 0)
		vd_layer[0].property_changed = true;

	return count;
}

static ssize_t video_mute_show(struct class *cla,
				  struct class_attribute *attr, char *buf)
{
	ssize_t ret = 0;

	get_video_mute_info();
	return ret;
}

static ssize_t video_mute_store(struct class *cla,
				   struct class_attribute *attr,
				   const char *buf, size_t count)
{
	int r, val, ret;

	r = kstrtoint(buf, 0, &val);
	if (r < 0)
		return -EINVAL;

	if (val)
		ret = set_video_mute_info(USER_MUTE_SET, true);
	else
		ret = set_video_mute_info(USER_MUTE_SET, false);
	if (ret == 0) {
		if (val == 0)
			pr_info("VIDEO UNMUTE by %s ret = %d\n", current->comm, ret);
		else if (val == 1)
			pr_info("VIDEO MUTE by %s ret = %d\n", current->comm, ret);
		else
			pr_info("set 1 mute video,set 0 unmute video\n");
	}
	if (ret < 0)
		return -EINVAL;

	return count;
}

static ssize_t video_disable_show(struct class *cla,
				  struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n",
		vd_layer[0].disable_video);
}

static ssize_t video_disable_store(struct class *cla,
				   struct class_attribute *attr,
				   const char *buf, size_t count)
{
	int r;
	int val;

	if (debug_flag & DEBUG_FLAG_BASIC_INFO)
		pr_info("%s(%s)\n", __func__, buf);

	r = kstrtoint(buf, 0, &val);
	if (r < 0)
		return -EINVAL;

	if (_video_set_disable(val) < 0)
		return -EINVAL;

	return count;
}

static ssize_t video_global_output_show(struct class *cla,
					struct class_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%d\n", vd_layer[0].global_output);
}

static ssize_t video_global_output_store(struct class *cla,
					 struct class_attribute *attr,
					 const char *buf,
					 size_t count)
{
	int r;

	r = kstrtouint(buf, 0, &vd_layer[0].global_output);
	if (r < 0)
		return -EINVAL;

	pr_info("%s(%d)\n", __func__, vd_layer[0].global_output);
	return count;
}

static ssize_t video_hold_show(struct class *cla,
			       struct class_attribute *attr,
			       char *buf)
{
	return sprintf(buf, "%d\n", hold_video);
}

static ssize_t video_hold_store(struct class *cla,
				struct class_attribute *attr,
				const char *buf, size_t count)
{
	int r;
	int value;

	cur_width = 0;
	cur_height = 0;
	if (debug_flag & DEBUG_FLAG_BASIC_INFO)
		pr_info("%s(%s)\n", __func__, buf);

	r = kstrtoint(buf, 0, &value);
	if (r < 0)
		return -EINVAL;

	while (atomic_read(&video_inirq_flag) > 0)
		schedule();
	if (cur_dev->pre_vsync_enable)
		while (atomic_read(&video_prevsync_inirq_flag) > 0)
			schedule();
	if (value == 0 && hold_video == 1)
		hold_property_changed = 1;

	hold_video = value;
	return count;
}

static ssize_t video_freerun_mode_show(struct class *cla,
				       struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", freerun_mode);
}

static ssize_t video_freerun_mode_store(struct class *cla,
					struct class_attribute *attr,
					const char *buf, size_t count)
{
	int r;

	r = kstrtouint(buf, 0, &freerun_mode);
	if (r < 0)
		return -EINVAL;

	if (debug_flag)
		pr_info("%s(%d)\n", __func__, freerun_mode);

	return count;
}

static ssize_t video_speed_check_show(struct class *cla,
				      struct class_attribute *attr, char *buf)
{
	u32 h, w;
	struct disp_info_s *layer = &glayer_info[0];

	h = layer->speed_check_height;
	w = layer->speed_check_width;

	return snprintf(buf, 40, "%d %d\n", h, w);
}

static ssize_t video_speed_check_store(struct class *cla,
				       struct class_attribute *attr,
				       const char *buf, size_t count)
{
	set_video_speed_check(buf);
	return strnlen(buf, count);
}

static ssize_t threedim_mode_store(struct class *cla,
				   struct class_attribute *attr,
				   const char *buf, size_t len)
{
#if defined(TV_3D_FUNCTION_OPEN) && defined(CONFIG_AMLOGIC_MEDIA_TVIN)
	u32 type;
	int r;
	struct vframe_s *dispbuf = NULL;

	r = kstrtouint(buf, 0, &type);
	if (r < 0)
		return -EINVAL;

	if (type != process_3d_type) {
		process_3d_type = type;
		if (mvc_flag)
			process_3d_type |= MODE_3D_MVC;
		vd_layer[0].property_changed = true;

		dispbuf = get_dispbuf(0);
		if ((process_3d_type & MODE_3D_FA) &&
		    dispbuf && !dispbuf->trans_fmt)
			/*notify di 3d mode is frame alternative mode,1*/
			/*passing two buffer in one frame */
			vf_notify_receiver_by_name
			("deinterlace",
			VFRAME_EVENT_PROVIDER_SET_3D_VFRAME_INTERLEAVE,
			(void *)1);
		else
			vf_notify_receiver_by_name
			("deinterlace",
			VFRAME_EVENT_PROVIDER_SET_3D_VFRAME_INTERLEAVE,
			(void *)0);
	}
#endif
	return len;
}

static ssize_t threedim_mode_show(struct class *cla,
				  struct class_attribute *attr, char *buf)
{
#ifdef TV_3D_FUNCTION_OPEN
	return sprintf(buf, "process type 0x%x,trans fmt %u.\n",
		       process_3d_type, video_3d_format);
#else
	return 0;
#endif
}

static ssize_t frame_addr_show(struct class *cla, struct class_attribute *attr,
			       char *buf)
{
	struct canvas_s canvas;
	u32 addr[3];
	struct vframe_s *dispbuf = NULL;
	unsigned int canvas0Addr;

	dispbuf = get_dispbuf(0);
	if (dispbuf) {
		canvas0Addr = get_layer_display_canvas(0);
		canvas_read(canvas0Addr & 0xff, &canvas);
		addr[0] = canvas.addr;
		canvas_read((canvas0Addr >> 8) & 0xff, &canvas);
		addr[1] = canvas.addr;
		canvas_read((canvas0Addr >> 16) & 0xff, &canvas);
		addr[2] = canvas.addr;

		return sprintf(buf, "0x%x-0x%x-0x%x\n", addr[0], addr[1],
			       addr[2]);
	}

	return sprintf(buf, "NA\n");
}

static ssize_t hdmin_delay_start_show(struct class *class,
				      struct class_attribute *attr,
				      char *buf)
{
	return sprintf(buf, "%d\n", hdmin_delay_start);
}

static ssize_t hdmin_delay_start_store(struct class *class,
				       struct class_attribute *attr,
				       const char *buf,
				       size_t count)
{
	int r;
	int value;
	unsigned long flags;

	r = kstrtoint(buf, 0, &value);
	if (r < 0)
		return -EINVAL;

	spin_lock_irqsave(&hdmi_avsync_lock, flags);
	hdmin_delay_start = value;
	hdmin_delay_start_time = -1;
	pr_info("[%s] hdmin_delay_start:%d\n", __func__, value);
	spin_unlock_irqrestore(&hdmi_avsync_lock, flags);

	return count;
}

static ssize_t hdmin_delay_min_ms_show(struct class *class,
				      struct class_attribute *attr,
				      char *buf)
{
	return sprintf(buf, "%d\n", hdmin_delay_min_ms);
}

static ssize_t hdmin_delay_min_ms_store(struct class *class,
				       struct class_attribute *attr,
				       const char *buf,
				       size_t count)
{
	int r;
	int value;

	r = kstrtoint(buf, 0, &value);
	if (r < 0)
		return -EINVAL;
	hdmin_delay_min_ms = value;
	pr_info("[%s] hdmin_delay_min_ms:%d\n", __func__, value);
	return count;
}

static ssize_t hdmin_delay_max_ms_show(struct class *class,
				      struct class_attribute *attr,
				      char *buf)
{
	return sprintf(buf, "%d\n", hdmin_delay_max_ms);
}

static ssize_t hdmin_delay_max_ms_store(struct class *class,
				       struct class_attribute *attr,
				       const char *buf,
				       size_t count)
{
	int r;
	int value;

	r = kstrtoint(buf, 0, &value);
	if (r < 0)
		return -EINVAL;
	hdmin_delay_max_ms = value;
	pr_info("[%s] hdmin_delay_max_ms:%d\n", __func__, value);
	return count;
}

static ssize_t hdmin_delay_duration_show(struct class *class,
					 struct class_attribute *attr,
					 char *buf)
{
	return sprintf(buf, "%d\n", last_required_total_delay);
}

/*set video total delay*/
static ssize_t hdmin_delay_duration_store(struct class *class,
					  struct class_attribute *attr,
					  const char *buf,
					  size_t count)
{
	int r;
	int value;
	unsigned long flags;

	r = kstrtoint(buf, 0, &value);
	if (r < 0)
		return -EINVAL;

	if (value < 0) { /*not support*/
		pr_info("[%s] invalid delay: %d\n", __func__, value);
		return -EINVAL;
	}

	spin_lock_irqsave(&hdmi_avsync_lock, flags);
	last_required_total_delay = value;
	spin_unlock_irqrestore(&hdmi_avsync_lock, flags);

	pr_info("[%s]current delay %d, total require %d\n",
		__func__, vframe_walk_delay, last_required_total_delay);
	return count;
}

static ssize_t vframe_walk_delay_show(struct class *class,
				      struct class_attribute *attr,
				      char *buf)
{
	return sprintf(buf, "%d\n", vframe_walk_delay);
}

static ssize_t last_required_total_delay_show(struct class *class,
				      struct class_attribute *attr,
				      char *buf)
{
	return sprintf(buf, "%d\n", last_required_total_delay);
}

static ssize_t frame_canvas_width_show(struct class *cla,
				       struct class_attribute *attr, char *buf)
{
	struct vframe_s *dispbuf = NULL;
	struct canvas_s canvas;
	u32 width[3];
	unsigned int canvas0Addr;

	dispbuf = get_dispbuf(0);
	if (dispbuf) {
		canvas0Addr = get_layer_display_canvas(0);
		canvas_read(canvas0Addr & 0xff, &canvas);
		width[0] = canvas.width;
		canvas_read((canvas0Addr >> 8) & 0xff, &canvas);
		width[1] = canvas.width;
		canvas_read((canvas0Addr >> 16) & 0xff, &canvas);
		width[2] = canvas.width;

		return sprintf(buf, "%d-%d-%d\n",
			width[0], width[1], width[2]);
	}

	return sprintf(buf, "NA\n");
}

static ssize_t frame_canvas_height_show(struct class *cla,
					struct class_attribute *attr,
					char *buf)
{
	struct vframe_s *dispbuf = NULL;
	struct canvas_s canvas;
	u32 height[3];
	unsigned int canvas0Addr;

	dispbuf = get_dispbuf(0);
	if (dispbuf) {
		canvas0Addr = get_layer_display_canvas(0);
		canvas_read(canvas0Addr & 0xff, &canvas);
		height[0] = canvas.height;
		canvas_read((canvas0Addr >> 8) & 0xff, &canvas);
		height[1] = canvas.height;
		canvas_read((canvas0Addr >> 16) & 0xff, &canvas);
		height[2] = canvas.height;

		return sprintf(buf, "%d-%d-%d\n", height[0], height[1],
			       height[2]);
	}

	return sprintf(buf, "NA\n");
}

static ssize_t frame_width_show(struct class *cla,
				struct class_attribute *attr,
				char *buf)
{
	struct vframe_s *dispbuf = NULL;

	dispbuf = get_dispbuf(0);

	if (hold_video == 1 &&
	    (glayer_info[0].display_path_id ==
	     VFM_PATH_AMVIDEO ||
	     glayer_info[0].display_path_id ==
	     VFM_PATH_DEF))
		return sprintf(buf, "%d\n", cur_width);
	if (dispbuf) {
		if (dispbuf->type & VIDTYPE_COMPRESS)
			return sprintf(buf, "%d\n", dispbuf->compWidth);
		else
			return sprintf(buf, "%d\n", dispbuf->width);
	}

	return sprintf(buf, "NA\n");
}

static ssize_t frame_height_show(struct class *cla,
				 struct class_attribute *attr, char *buf)
{
	struct vframe_s *dispbuf = NULL;

	if (hold_video == 1 &&
	    (glayer_info[0].display_path_id ==
	     VFM_PATH_AMVIDEO ||
	     glayer_info[0].display_path_id ==
	     VFM_PATH_DEF))
		return sprintf(buf, "%d\n", cur_height);

	dispbuf = get_dispbuf(0);
	if (dispbuf) {
		if (dispbuf->type & VIDTYPE_COMPRESS)
			return sprintf(buf, "%d\n", dispbuf->compHeight);
		else
			return sprintf(buf, "%d\n", dispbuf->height);
	}

	return sprintf(buf, "NA\n");
}

static ssize_t frame_format_show(struct class *cla,
				 struct class_attribute *attr, char *buf)
{
	struct vframe_s *dispbuf = NULL;
	ssize_t ret = 0;

	dispbuf = get_dispbuf(0);
	if (dispbuf) {
		if ((dispbuf->type & VIDTYPE_TYPEMASK) ==
		    VIDTYPE_INTERLACE_TOP)
			ret = sprintf(buf, "interlace-top\n");
		else if ((dispbuf->type & VIDTYPE_TYPEMASK) ==
			 VIDTYPE_INTERLACE_BOTTOM)
			ret = sprintf(buf, "interlace-bottom\n");
		else
			ret = sprintf(buf, "progressive\n");

		if (dispbuf->type & VIDTYPE_COMPRESS)
			ret += sprintf(buf + ret, "Compressed\n");

		return ret;
	}

	return sprintf(buf, "NA\n");
}

static ssize_t frame_original_format_show(struct class *cla,
				 struct class_attribute *attr, char *buf)
{
	struct vframe_s *dispbuf = NULL;
	ssize_t ret = 0;

	dispbuf = get_dispbuf(0);
	if (dispbuf) {
		if ((dispbuf->type_original & VIDTYPE_TYPEMASK) ==
		    VIDTYPE_INTERLACE_TOP)
			ret = sprintf(buf, "interlace-top\n");
		else if ((dispbuf->type_original & VIDTYPE_TYPEMASK) ==
			 VIDTYPE_INTERLACE_BOTTOM)
			ret = sprintf(buf, "interlace-bottom\n");
		else
			ret = sprintf(buf, "progressive\n");

		if (dispbuf->type_original & VIDTYPE_COMPRESS)
			ret += sprintf(buf + ret, "Compressed\n");

		return ret;
	}

	return sprintf(buf, "NA\n");
}

static ssize_t frame_aspect_ratio_show(struct class *cla,
				       struct class_attribute *attr,
				       char *buf)
{
	struct vframe_s *dispbuf = NULL;

	dispbuf = get_dispbuf(0);
	if (dispbuf) {
		u32 ar = (dispbuf->ratio_control &
			DISP_RATIO_ASPECT_RATIO_MASK) >>
			DISP_RATIO_ASPECT_RATIO_BIT;

		if (ar)
			return sprintf(buf, "0x%x\n", ar);
		else
			return sprintf(buf, "0x%x\n",
				       (dispbuf->width << 8) /
				       dispbuf->height);
	}

	return sprintf(buf, "NA\n");
}

static ssize_t frame_rate_show(struct class *cla, struct class_attribute *attr,
			       char *buf)
{
	u32 cnt = frame_count - last_frame_count;
	u32 time = jiffies;
	u32 tmp = time;
	u32 rate = 0;
	u32 vsync_rate;
	ssize_t ret = 0;

	time -= last_frame_time;
	last_frame_time = tmp;
	last_frame_count = frame_count;
	if (time == 0)
		return 0;
	rate = 100 * cnt * HZ / time;
	vsync_rate = 100 * vsync_count * HZ / time;
	if (vinfo->sync_duration_den > 0) {
		ret = sprintf
			(buf,
			 "VF.fps=%d.%02d panel fps %d, dur/is: %d,v/s=%d.%02d,inc=%d\n",
			 rate / 100, rate % 100,
			 vinfo->sync_duration_num /
			 vinfo->sync_duration_den,
			 time, vsync_rate / 100, vsync_rate % 100,
			 vsync_pts_inc);
	}
	if ((debugflags & DEBUG_FLAG_CALC_PTS_INC) &&
	    time > HZ * 10 && vsync_rate > 0) {
		if ((vsync_rate * vsync_pts_inc / 100) != 90000)
			vsync_pts_inc = 90000 * 100 / (vsync_rate);
	}
	vsync_count = 0;
	return ret;
}

static ssize_t vframe_states_show(struct class *cla,
				  struct class_attribute *attr, char *buf)
{
	int ret = 0;
	struct vframe_states states;
	unsigned long flags;
	struct vframe_s *vf;

	memset(&states, 0, sizeof(struct vframe_states));
	if (amvideo_vf_get_states(&states) == 0) {
		ret += sprintf(buf + ret, "amvideo vframe states\n");
		ret += sprintf(buf + ret, "vframe_pool_size=%d\n",
			states.vf_pool_size);
		ret += sprintf(buf + ret, "vframe buf_free_num=%d\n",
			states.buf_free_num);
		ret += sprintf(buf + ret, "vframe buf_recycle_num=%d\n",
			states.buf_recycle_num);
		ret += sprintf(buf + ret, "vframe buf_avail_num=%d\n",
			states.buf_avail_num);

		spin_lock_irqsave(&lock, flags);

		vf = amvideo_vf_peek();
		if (vf) {
			ret += sprintf(buf + ret,
				"vframe ready frame delayed =%dms\n",
				(int)(jiffies_64 -
				vf->ready_jiffies64) * 1000 /
				HZ);
			ret += sprintf(buf + ret,
				"vf index=%d\n", vf->index);
			ret += sprintf(buf + ret,
				"vf->pts=%d\n", vf->pts);
			ret += sprintf(buf + ret,
				"cur vpts=%d\n",
				timestamp_vpts_get());
			ret += sprintf(buf + ret,
				"vf type=%d\n",
				vf->type);
			ret += sprintf(buf + ret,
				"vf type_original=%d\n",
				vf->type_original);
			if (vf->type & VIDTYPE_COMPRESS) {
				ret += sprintf(buf + ret,
					"vf compHeadAddr=%lx\n",
						vf->compHeadAddr);
				ret += sprintf(buf + ret,
					"vf compBodyAddr =%lx\n",
						vf->compBodyAddr);
			} else {
				ret += sprintf(buf + ret,
					"vf canvas0Addr=%x\n",
						vf->canvas0Addr);
				ret += sprintf(buf + ret,
					"vf canvas1Addr=%x\n",
						vf->canvas1Addr);
				ret += sprintf(buf + ret,
					"vf canvas0Addr.y.addr=%lx(%ld)\n",
					canvas_get_addr
					(canvasY(vf->canvas0Addr)),
					canvas_get_addr
					(canvasY(vf->canvas0Addr)));
				ret += sprintf(buf + ret,
					"vf canvas0Adr.uv.adr=%lx(%ld)\n",
					canvas_get_addr
					(canvasUV(vf->canvas0Addr)),
					canvas_get_addr
					(canvasUV(vf->canvas0Addr)));
			}
			if (vf->vf_ext)
				ret += sprintf(buf + ret,
					"vf_ext=%p\n",
					vf->vf_ext);
			if (vf->uvm_vf)
				ret += sprintf(buf + ret,
					"uvm_vf=%p\n",
					vf->uvm_vf);
		}
		spin_unlock_irqrestore(&lock, flags);

	} else {
		spin_lock_irqsave(&lock, flags);

		vf = get_dispbuf(0);
		if (vf) {
			ret += sprintf(buf + ret,
				"vd_layer[0] vframe states\n");
			ret += sprintf(buf + ret,
				"vframe ready frame delayed =%dms\n",
				(int)(jiffies_64 -
				vf->ready_jiffies64) * 1000 /
				HZ);
			ret += sprintf(buf + ret,
				"vf index=%d\n", vf->index);
			ret += sprintf(buf + ret,
				"vf->pts=%d\n", vf->pts);
			ret += sprintf(buf + ret,
				"cur vpts=%d\n",
				timestamp_vpts_get());
			ret += sprintf(buf + ret,
				"vf type=%d\n",
				vf->type);
			ret += sprintf(buf + ret,
				"vf type_original=%d\n",
				vf->type_original);
			if (vf->type & VIDTYPE_COMPRESS) {
				ret += sprintf(buf + ret,
					"vf compHeadAddr=%lx\n",
						vf->compHeadAddr);
				ret += sprintf(buf + ret,
					"vf compBodyAddr =%lx\n",
						vf->compBodyAddr);
			} else {
				ret += sprintf(buf + ret,
					"vf canvas0Addr=%x\n",
						vf->canvas0Addr);
				ret += sprintf(buf + ret,
					"vf canvas1Addr=%x\n",
						vf->canvas1Addr);
				ret += sprintf(buf + ret,
					"vf canvas0Addr.y.addr=%lx(%ld)\n",
					canvas_get_addr
					(canvasY(vf->canvas0Addr)),
					canvas_get_addr
					(canvasY(vf->canvas0Addr)));
				ret += sprintf(buf + ret,
					"vf canvas0Adr.uv.adr=%lx(%ld)\n",
					canvas_get_addr
					(canvasUV(vf->canvas0Addr)),
					canvas_get_addr
					(canvasUV(vf->canvas0Addr)));
			}
			if (vf->vf_ext)
				ret += sprintf(buf + ret,
					"vf_ext=%p\n",
					vf->vf_ext);
			if (vf->uvm_vf)
				ret += sprintf(buf + ret,
					"uvm_vf=%p\n",
					vf->uvm_vf);
		} else {
			ret += sprintf(buf + ret, "vframe no states\n");
		}
		spin_unlock_irqrestore(&lock, flags);
	}
	return ret;
}

#ifdef CONFIG_AM_VOUT
static ssize_t device_resolution_show(struct class *cla,
				      struct class_attribute *attr, char *buf)
{
	const struct vinfo_s *info = get_current_vinfo();

	if (!info || info->mode == VMODE_INVALID)
		return sprintf(buf, "0x0\n");
	else
		return sprintf(buf, "%dx%d\n", info->width, info->height);
}
#endif

static ssize_t video_filename_show(struct class *cla,
				   struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", file_name);
}

static ssize_t video_filename_store(struct class *cla,
				    struct class_attribute *attr,
				    const char *buf, size_t count)
{
	size_t r;

	/* check input buf to mitigate buffer overflow issue */
	if (strlen(buf) >= sizeof(file_name)) {
		memcpy(file_name, buf, sizeof(file_name));
		file_name[sizeof(file_name) - 1] = '\0';
		r = 1;
	} else {
		r = sscanf(buf, "%s", file_name);
	}
	if (r != 1)
		return -EINVAL;

	return r;
}

static ssize_t video_debugflags_show(struct class *cla,
				     struct class_attribute *attr, char *buf)
{
	int len = 0;

	len += sprintf(buf + len, "value=%d\n", debugflags);
	len += sprintf(buf + len, "bit0:playing as fast!\n");
	len += sprintf(buf + len,
		"bit1:enable calc pts inc in frame rate show\n");
	return len;
}

static ssize_t video_debugflags_store(struct class *cla,
				      struct class_attribute *attr,
				      const char *buf, size_t count)
{
	int r;
	int value = -1;

/*
 *	r = sscanf(buf, "%d", &value);
 *	if (r == 1) {
 *		debugflags = value;
 *		seted = 1;
 *	} else {
 *		r = sscanf(buf, "0x%x", &value);
 *		if (r == 1) {
 *			debugflags = value;
 *			seted = 1;
 *		}
 *	}
 */

	r = kstrtoint(buf, 0, &value);
	if (r < 0)
		return -EINVAL;

	debugflags = value;

	pr_info("debugflags changed to %d(%x)\n", debugflags,
		debugflags);
	return count;
}

static ssize_t trickmode_duration_show(struct class *cla,
				       struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "trickmode frame duration %d\n",
		       trickmode_duration / 9000);
}

static ssize_t trickmode_duration_store(struct class *cla,
					struct class_attribute *attr,
					const char *buf, size_t count)
{
	int r;
	u32 s_value;

	r = kstrtouint(buf, 0, &s_value);
	if (r < 0)
		return -EINVAL;

	trickmode_duration = s_value * 9000;

	return count;
}

static ssize_t video_vsync_pts_inc_upint_show(struct class *cla,
					      struct class_attribute *attr,
					      char *buf)
{
	if (vsync_pts_inc_upint)
		return sprintf(buf,
		"%d,freerun %d,1.25xInc %d,1.12xInc %d,inc+10 %d,1xInc %d\n",
		vsync_pts_inc_upint, vsync_freerun,
		vsync_pts_125, vsync_pts_112, vsync_pts_101,
		vsync_pts_100);
	else
		return sprintf(buf, "%d\n", vsync_pts_inc_upint);
}

static ssize_t video_vsync_pts_inc_upint_store(struct class *cla,
					       struct class_attribute *attr,
					       const char *buf, size_t count)
{
	int r;

	r = kstrtouint(buf, 0, &vsync_pts_inc_upint);
	if (r < 0)
		return -EINVAL;

	if (debug_flag)
		pr_info("%s(%d)\n", __func__, vsync_pts_inc_upint);

	return count;
}

static ssize_t slowsync_repeat_enable_show(struct class *cla,
					   struct class_attribute *attr,
					   char *buf)
{
	return sprintf(buf, "slowsync repeate enable = %d\n",
		       slowsync_repeat_enable);
}

static ssize_t slowsync_repeat_enable_store(struct class *cla,
					    struct class_attribute *attr,
					    const char *buf, size_t count)
{
	int r;

	r = kstrtouint(buf, 0, &slowsync_repeat_enable);
	if (r < 0)
		return -EINVAL;

	if (debug_flag)
		pr_info("%s(%d)\n", __func__, slowsync_repeat_enable);

	return count;
}

static ssize_t video_vsync_slow_factor_show(struct class *cla,
					    struct class_attribute *attr,
					    char *buf)
{
	return sprintf(buf, "%d\n", vsync_slow_factor);
}

static ssize_t video_vsync_slow_factor_store(struct class *cla,
					     struct class_attribute *attr,
					     const char *buf, size_t count)
{
	int r;

	r = kstrtouint(buf, 0, &vsync_slow_factor);
	if (r < 0)
		return -EINVAL;

	if (debug_flag)
		pr_info("%s(%d)\n", __func__, vsync_slow_factor);

	return count;
}

static ssize_t vframe_ready_cnt_show(struct class *cla,
				     struct class_attribute *attr,
				     char *buf)
{
	int ret;
	struct vframe_states states;

	states.buf_avail_num = 0;
	ret = amvideo_vf_get_states(&states);

	return snprintf(buf, 10, "%d\n", (ret == 0) ?
		states.buf_avail_num : 0);
}

static ssize_t fps_info_show(struct class *cla,
			     struct class_attribute *attr,
			     char *buf)
{
	u32 cnt = frame_count - last_frame_count;
	u32 time = jiffies;
	u32 input_fps = 0;
	u32 tmp = time;

	time -= last_frame_time;
	last_frame_time = tmp;
	last_frame_count = frame_count;
	if (time != 0)
		output_fps = cnt * HZ / time;
	if (cur_dispbuf[0] && cur_dispbuf[0]->duration > 0) {
		input_fps = 96000 / cur_dispbuf[0]->duration;
		if (output_fps > input_fps)
			output_fps = input_fps;
	} else {
		input_fps = output_fps;
	}
	return sprintf(buf, "input_fps:0x%x output_fps:0x%x drop_fps:0x%x\n",
		       input_fps, output_fps, input_fps - output_fps);
}

static ssize_t video_layer1_state_show(struct class *cla,
				       struct class_attribute *attr,
				       char *buf)
{
	return sprintf(buf, "%d\n", vd_layer[0].enabled);
}

static ssize_t video_angle_show(struct class *cla,
				struct class_attribute *attr,
				char *buf)
{
	struct disp_info_s *layer = &glayer_info[0];

	return snprintf(buf, 40, "%d\n", layer->angle);
}

static ssize_t video_angle_store(struct class *cla,
				 struct class_attribute *attr, const char *buf,
				 size_t count)
{
	int r;
	u32 s_value;

	r = kstrtouint(buf, 0, &s_value);
	if (r < 0)
		return -EINVAL;

	set_video_angle(s_value);
	return strnlen(buf, count);
}

static ssize_t show_first_frame_nosync_show(struct class *cla,
					    struct class_attribute *attr,
					    char *buf)
{
	return sprintf(buf, "%d\n", show_first_frame_nosync ? 1 : 0);
}

static ssize_t show_first_frame_nosync_store(struct class *cla,
					     struct class_attribute *attr,
					     const char *buf, size_t count)
{
	int r;
	int value;

	r = kstrtoint(buf, 0, &value);
	if (r < 0)
		return -EINVAL;

	if (value == 0)
		show_first_frame_nosync = false;
	else
		show_first_frame_nosync = true;

	return count;
}

static ssize_t show_first_picture_store(struct class *cla,
					struct class_attribute *attr,
					const char *buf, size_t count)
{
	int r;
	int value;

	r = kstrtoint(buf, 0, &value);
	if (r < 0)
		return -EINVAL;

	if (value == 0)
		show_first_picture = false;
	else
		show_first_picture = true;

	return count;
}

static ssize_t video_free_keep_buffer_store(struct class *cla,
					    struct class_attribute *attr,
					    const char *buf, size_t count)
{
	int r;
	int val;

	if (debug_flag & DEBUG_FLAG_BASIC_INFO)
		pr_info("%s(%s)\n", __func__, buf);
	r = kstrtoint(buf, 0, &val);
	if (r < 0)
		return -EINVAL;
	if (val == 1)
		try_free_keep_vdx(vd_layer[0].keep_frame_id, 1);

	return count;
}

static ssize_t free_cma_buffer_store(struct class *cla,
				     struct class_attribute *attr,
				     const char *buf, size_t count)
{
	int r;
	int val;

	r = kstrtoint(buf, 0, &val);
	if (r < 0)
		return -EINVAL;
	if (val == 1) {
		pr_info("start to free cma buffer\n");
#if DEBUG_TMP
		vh265_free_cmabuf();
		vh264_4k_free_cmabuf();
		vdec_free_cmabuf();
#endif
	}
	return count;
}

static ssize_t pic_mode_info_show(struct class *cla,
				  struct class_attribute *attr,
				  char *buf)
{
	int ret = 0;
	struct vframe_s *dispbuf = NULL;

	dispbuf = get_dispbuf(0);
	if (dispbuf) {
		u32 adapted_mode = (dispbuf->ratio_control
			& DISP_RATIO_ADAPTED_PICMODE) ? 1 : 0;
		u32 info_frame = (dispbuf->ratio_control
			& DISP_RATIO_INFOFRAME_AVAIL) ? 1 : 0;

		ret += sprintf(buf + ret, "ratio_control=0x%x\n",
			dispbuf->ratio_control);
		ret += sprintf(buf + ret, "adapted_mode=%d\n",
			adapted_mode);
		ret += sprintf(buf + ret, "info_frame=%d\n",
			info_frame);
		ret += sprintf(buf + ret,
			"hs=%d, he=%d, vs=%d, ve=%d\n",
			dispbuf->pic_mode.hs,
			dispbuf->pic_mode.he,
			dispbuf->pic_mode.vs,
			dispbuf->pic_mode.ve);
		ret += sprintf(buf + ret, "screen_mode=%d\n",
			dispbuf->pic_mode.screen_mode);
		ret += sprintf(buf + ret, "custom_ar=%d\n",
			dispbuf->pic_mode.custom_ar);
		ret += sprintf(buf + ret, "AFD_enable=%d\n",
			dispbuf->pic_mode.AFD_enable);
		return ret;
	}
	return sprintf(buf, "NA\n");
}

static ssize_t src_fmt_show(struct class *cla,
			    struct class_attribute *attr,
			    char *buf)
{
	int ret = 0;
	struct vframe_s *dispbuf = NULL;
	enum vframe_signal_fmt_e fmt;
	void *sei_ptr;
	u32 sei_size = 0;

	dispbuf = get_dispbuf(0);
	ret += sprintf(buf + ret, "vd1 dispbuf: %p\n", dispbuf);
	if (dispbuf) {
		fmt = get_vframe_src_fmt(dispbuf);
		if (fmt != VFRAME_SIGNAL_FMT_INVALID &&
		    fmt < VFRAME_SIGNAL_FMT_MAX) {
			sei_ptr = get_sei_from_src_fmt(dispbuf, &sei_size);
			ret += sprintf(buf + ret, "fmt = %s\n",
				src_fmt_str[fmt]);
			ret += sprintf(buf + ret, "sei: %p, size: %d\n",
				sei_ptr, sei_size);
		} else {
			ret += sprintf(buf + ret, "src_fmt is invalid\n");
		}
	}
	dispbuf = get_dispbuf(1);
	ret += sprintf(buf + ret, "vd2 dispbuf: %p\n", dispbuf);
	if (dispbuf) {
		fmt = get_vframe_src_fmt(dispbuf);
		if (fmt != VFRAME_SIGNAL_FMT_INVALID &&
		    fmt < VFRAME_SIGNAL_FMT_MAX) {
			sei_size = 0;
			sei_ptr = get_sei_from_src_fmt(dispbuf, &sei_size);
			ret += sprintf(buf + ret, "fmt=0x%s\n",
				src_fmt_str[fmt]);
			ret += sprintf(buf + ret, "sei: %p, size: %d\n",
				sei_ptr, sei_size);
		} else {
			ret += sprintf(buf + ret, "src_fmt is invalid\n");
		}
	}
	if (!cur_dev || cur_dev->max_vd_layers <= 2)
		goto src_fmt_end;

	dispbuf = get_dispbuf(2);
	ret += sprintf(buf + ret, "vd3 dispbuf: %p\n", dispbuf);
	if (dispbuf) {
		fmt = get_vframe_src_fmt(dispbuf);
		if (fmt != VFRAME_SIGNAL_FMT_INVALID &&
		    fmt < VFRAME_SIGNAL_FMT_MAX) {
			sei_size = 0;
			sei_ptr = get_sei_from_src_fmt(dispbuf, &sei_size);
			ret += sprintf(buf + ret, "fmt=0x%s\n",
				src_fmt_str[fmt]);
			ret += sprintf(buf + ret, "sei: %p, size: %d\n",
				sei_ptr, sei_size);
		} else {
			ret += sprintf(buf + ret, "src_fmt is invalid\n");
		}
	}
src_fmt_end:
	return ret;
}

static ssize_t process_fmt_show
	(struct class *cla, struct class_attribute *attr, char *buf)
{
	int ret = 0;
	struct vframe_s *dispbuf = NULL;
	enum vframe_signal_fmt_e fmt;
	char process_name[MAX_VD_LAYER][32] = {{'\0'},};
	char output_fmt[32] = {'\0',};
	bool amdv_on = false;
	bool hdr_bypass = false;
	int l;
	int output_mode;

	amdv_on = is_amdv_on();
	dispbuf = get_dispbuf(0);
	if (dispbuf) {
		fmt = get_vframe_src_fmt(dispbuf);
		if (fmt == VFRAME_SIGNAL_FMT_INVALID)
			fmt = get_sfmt_from_signal_type(dispbuf);
		if (fmt != VFRAME_SIGNAL_FMT_INVALID &&
		    fmt < VFRAME_SIGNAL_FMT_MAX)
			ret += sprintf(buf + ret, "vd1: src_fmt = %s; ",
				src_fmt_str[fmt]);
		else
			ret += sprintf(buf + ret, "vd1: src_fmt = null; ");

#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_VECM
		get_hdr_process_name(0, process_name[0], output_fmt);
#endif

		l = strlen("HDR_BYPASS");
		if (!strncmp(process_name[0], "HDR_BYPASS", l) ||
		    !strncmp(process_name[0], "HLG_BYPASS", l) ||
		    !strncmp(process_name[0], "CUVA_BYPASS", l))
			hdr_bypass = true;

		if (amdv_on) {
			output_mode = get_amdv_mode();
			if (output_mode == 0 || output_mode == 1)
				ret += sprintf(buf + ret, "out_fmt = IPT\n");
			else if (output_mode == 2)
				ret += sprintf(buf + ret, "out_fmt = HDR10\n");
			else if (output_mode == 3)
				ret += sprintf(buf + ret, "out_fmt = SDR10\n");
			else if (output_mode == 4)
				ret += sprintf(buf + ret, "out_fmt = SDR8\n");
		} else if (hdr_bypass) {
			if (fmt != VFRAME_SIGNAL_FMT_INVALID &&
			    fmt < VFRAME_SIGNAL_FMT_MAX)
				if ((!strcmp(src_fmt_str[fmt], "HDR10") ||
				     !strcmp(src_fmt_str[fmt], "HDR10+") ||
				     !strcmp(src_fmt_str[fmt], "CUVA_HDR") ||
				     !strcmp(src_fmt_str[fmt], "CUVA_HLG")) &&
				    (!strcmp(output_fmt, "HDR") ||
				     !strcmp(output_fmt, "HDR+") ||
				     !strcmp(output_fmt, "CUVA_HDR") ||
				     !strcmp(output_fmt, "CUVA_HLG")))
					ret += sprintf(buf + ret,
						"out_fmt = %s_%s\n",
						src_fmt_str[fmt], output_fmt);
				else
					ret += sprintf(buf + ret,
						"out_fmt = %s\n", src_fmt_str[fmt]);
			else
				ret += sprintf(buf + ret, "out_fmt = src!\n");
		} else {
			ret += sprintf(buf + ret, "process = %s\n",
				process_name[0]);
		}
	}
	hdr_bypass = false;
	dispbuf = get_dispbuf(1);
	if (dispbuf) {
		fmt = get_vframe_src_fmt(dispbuf);
		if (fmt == VFRAME_SIGNAL_FMT_INVALID)
			fmt = get_sfmt_from_signal_type(dispbuf);
		if (fmt != VFRAME_SIGNAL_FMT_INVALID &&
		    fmt < VFRAME_SIGNAL_FMT_MAX)
			ret += sprintf(buf + ret, "vd2: src_fmt = %s; ",
				src_fmt_str[fmt]);
		else
			ret += sprintf(buf + ret, "vd2: src_fmt = null; ");

#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_VECM
		get_hdr_process_name(1, process_name[1], output_fmt);
#endif

		l = strlen("HDR_BYPASS");
		if (!strncmp(process_name[1], "HDR_BYPASS", l) ||
		    !strncmp(process_name[1], "HLG_BYPASS", l) ||
		    !strncmp(process_name[1], "CUVA_BYPASS", l))
			hdr_bypass = true;

		if (amdv_on && support_multi_core1()) {
			output_mode = get_amdv_mode();
			if (output_mode == 0 || output_mode == 1)
				ret += sprintf(buf + ret, "out_fmt = IPT\n");
			else if (output_mode == 2)
				ret += sprintf(buf + ret, "out_fmt = HDR10\n");
			else if (output_mode == 3)
				ret += sprintf(buf + ret, "out_fmt = SDR10\n");
			else if (output_mode == 4)
				ret += sprintf(buf + ret, "out_fmt = SDR8\n");
		} else if (hdr_bypass) {
			if (fmt != VFRAME_SIGNAL_FMT_INVALID &&
			    fmt < VFRAME_SIGNAL_FMT_MAX)
				if ((!strcmp(src_fmt_str[fmt], "HDR10") ||
				     !strcmp(src_fmt_str[fmt], "HDR10+") ||
				     !strcmp(src_fmt_str[fmt], "CUVA_HDR") ||
				     !strcmp(src_fmt_str[fmt], "CUVA_HLG")) &&
				    (!strcmp(output_fmt, "HDR") ||
				     !strcmp(output_fmt, "HDR+") ||
				     !strcmp(output_fmt, "CUVA_HDR") ||
				     !strcmp(output_fmt, "CUVA_HLG")))
					ret += sprintf(buf + ret,
						"out_fmt = %s_%s\n",
						src_fmt_str[fmt], output_fmt);
				else
					ret += sprintf(buf + ret,
						"out_fmt = %s\n", src_fmt_str[fmt]);
			else
				ret += sprintf(buf + ret, "out_fmt = src!\n");
		} else {
			ret += sprintf(buf + ret, "process = %s\n",
				process_name[1]);
		}
	}

	if (!cur_dev || cur_dev->max_vd_layers <= 2)
		goto show_end;

	hdr_bypass = false;
	dispbuf = get_dispbuf(2);
	if (dispbuf) {
		fmt = get_vframe_src_fmt(dispbuf);
		if (fmt == VFRAME_SIGNAL_FMT_INVALID)
			fmt = get_sfmt_from_signal_type(dispbuf);
		if (fmt != VFRAME_SIGNAL_FMT_INVALID &&
		    fmt < VFRAME_SIGNAL_FMT_MAX)
			ret += sprintf(buf + ret, "vd3: src_fmt = %s; ",
				src_fmt_str[fmt]);
		else
			ret += sprintf(buf + ret, "vd3: src_fmt = null; ");
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_VECM
		get_hdr_process_name(2, process_name[2], output_fmt);
#endif
		l = strlen("HDR_BYPASS");
		if (!strncmp(process_name[2], "HDR_BYPASS", l) ||
		    !strncmp(process_name[2], "HLG_BYPASS", l) ||
		    !strncmp(process_name[2], "CUVA_BYPASS", l))
			hdr_bypass = true;

		if (amdv_on) {
			ret += sprintf(buf + ret, "out_fmt = IPT\n");
		} else if (hdr_bypass) {
			if (fmt != VFRAME_SIGNAL_FMT_INVALID &&
			    fmt < VFRAME_SIGNAL_FMT_MAX)
				if ((!strcmp(src_fmt_str[fmt], "HDR10") ||
				     !strcmp(src_fmt_str[fmt], "HDR10+") ||
				     !strcmp(src_fmt_str[fmt], "CUVA_HDR") ||
				     !strcmp(src_fmt_str[fmt], "CUVA_HLG")) &&
				    (!strcmp(output_fmt, "HDR") ||
				     !strcmp(output_fmt, "HDR+") ||
				     !strcmp(output_fmt, "CUVA_HDR") ||
				     !strcmp(output_fmt, "CUVA_HLG")))
					ret += sprintf(buf + ret,
						"out_fmt = %s_%s\n",
						src_fmt_str[fmt], output_fmt);
				else
					ret += sprintf(buf + ret,
						"out_fmt = %s\n", src_fmt_str[fmt]);
			else
				ret += sprintf(buf + ret, "out_fmt = src!\n");
		} else {
			ret += sprintf(buf + ret, "process = %s\n",
				process_name[2]);
		}
	}
show_end:
	return ret;
}

static ssize_t cur_aipq_sp_show(struct class *cla,
			struct class_attribute *attr, char *buf)
{
	ssize_t count = 0;

	if (!cur_dispbuf[0])
		return 0;

	count += sprintf(buf + count, "top_one:%d\n", scene_prob[0]);
	count += sprintf(buf + count, "top_one_prob:%d\n", scene_prob[1]);

	count += sprintf(buf + count, "cur_skin_pct:%d\n",
		aipq_hist_data.cur_skin_pct);
	count += sprintf(buf + count, "cur_green_pct:%d\n",
		aipq_hist_data.cur_green_pct);
	count += sprintf(buf + count, "cur_blue_pct:%d\n",
		aipq_hist_data.cur_blue_pct);
	count += sprintf(buf + count, "pre_skin_pct:%d\n",
		aipq_hist_data.pre_skin_pct);
	count += sprintf(buf + count, "pre_green_pct:%d\n",
		aipq_hist_data.pre_green_pct);
	count += sprintf(buf + count, "pre_blue_pct:%d\n",
		aipq_hist_data.pre_blue_pct);

	count += sprintf(buf + count, "\n");

	return count;
}

static ssize_t video_inuse_show(struct class *class,
				struct class_attribute *attr,
				char *buf)
{
	size_t r;

	mutex_lock(&video_inuse_mutex);
	if (video_inuse == 0) {
		r = sprintf(buf, "%d\n", video_inuse);
		video_inuse = 1;
		pr_info("video_inuse return 0,set 1\n");
	} else {
		r = sprintf(buf, "%d\n", video_inuse);
		pr_info("video_inuse = %d\n", video_inuse);
	}
	mutex_unlock(&video_inuse_mutex);
	return r;
}

static ssize_t video_inuse_store(struct class *class,
				 struct class_attribute *attr,
				 const char *buf,
				 size_t count)
{
	size_t r;
	int val;

	mutex_lock(&video_inuse_mutex);
	r = kstrtoint(buf, 0, &val);
	pr_info("set video_inuse val:%d\n", val);
	video_inuse = val;
	mutex_unlock(&video_inuse_mutex);
	if (r != 1)
		return -EINVAL;

	return count;
}

static ssize_t video_zorder_show(struct class *cla,
				 struct class_attribute *attr,
				 char *buf)
{
	struct disp_info_s *layer = &glayer_info[0];

	return sprintf(buf, "%d\n", layer->zorder);
}

static ssize_t video_zorder_store(struct class *cla,
				  struct class_attribute *attr,
				  const char *buf,
				  size_t count)
{
	int zorder;
	int ret = 0;
	struct disp_info_s *layer = &glayer_info[0];

	ret = kstrtoint(buf, 0, &zorder);
	if (ret < 0)
		return -EINVAL;

	if (zorder != layer->zorder) {
		layer->zorder = zorder;
		vd_layer[0].property_changed = true;
	}
	return count;
}

static ssize_t black_threshold_show(struct class *cla,
				    struct class_attribute *attr,
				    char *buf)
{
	return sprintf(buf, "width: %d, height: %d\n",
		black_threshold_width,
		black_threshold_height);
}

static ssize_t black_threshold_store(struct class *cla,
				     struct class_attribute *attr,
				     const char *buf,
				     size_t count)
{
	int parsed[2];

	if (likely(parse_para(buf, 2, parsed) == 2)) {
		black_threshold_width = parsed[0];
		black_threshold_height = parsed[1];
	}
	return strnlen(buf, count);
}

static ssize_t get_di_count_show(struct class *cla,
				 struct class_attribute *attr,
				 char *buf)
{
	return sprintf(buf, "%d\n", get_di_count);
}

static ssize_t get_di_count_store(struct class *cla,
				  struct class_attribute *attr,
				  const char *buf, size_t count)
{
	int r;

	r = kstrtoint(buf, 0, &get_di_count);
	if (r < 0)
		return -EINVAL;

	return count;
}

static ssize_t put_di_count_show(struct class *cla,
				 struct class_attribute *attr,
				 char *buf)
{
	return sprintf(buf, "%d\n", put_di_count);
}

static ssize_t put_di_count_store(struct class *cla,
				  struct class_attribute *attr,
				  const char *buf, size_t count)
{
	int r;

	r = kstrtoint(buf, 0, &put_di_count);
	if (r < 0)
		return -EINVAL;

	return count;
}

static ssize_t di_release_count_show(struct class *cla,
				     struct class_attribute *attr,
				     char *buf)
{
	return sprintf(buf, "%d\n", di_release_count);
}

static ssize_t di_release_count_store(struct class *cla,
				      struct class_attribute *attr,
				      const char *buf, size_t count)
{
	int r;

	r = kstrtoint(buf, 0, &di_release_count);
	if (r < 0)
		return -EINVAL;

	return count;
}

static ssize_t limited_win_ratio_show(struct class *cla,
				     struct class_attribute *attr,
				     char *buf)
{
	int limited_win_ratio = 0;

	return sprintf(buf, "%d\n", limited_win_ratio);
}

#ifndef CONFIG_AMLOGIC_MEDIA_CODEC_MM
unsigned long codec_mm_alloc_for_dma(const char *owner, int page_cnt,
				     int align2n, int memflags)
{
	return 0;
}

int codec_mm_free_for_dma(const char *owner, unsigned long phy_addr)
{
	return 0;
}

int codec_mm_keeper_unmask_keeper(int keep_id, int delayms)
{
	return 0;
}

int codec_mm_keeper_mask_keep_mem(void *mem_handle, int type)
{
	return 0;
}

int configs_register_path_configs(const char *path,
				  struct mconfig *configs, int num)
{
	return 0;
}

#endif

static int free_alloced_hist_test_buffer(void)
{
	if (hist_buffer_addr) {
		codec_mm_free_for_dma("hist-test", hist_buffer_addr);
		hist_buffer_addr = 0;
	}
	return 0;
}

static int alloc_hist_test_buffer(u32 size)
{
	int ret = -ENOMEM;
	int flags = CODEC_MM_FLAGS_DMA |
		CODEC_MM_FLAGS_FOR_VDECODER;

	if (!hist_buffer_addr) {
		hist_buffer_addr = codec_mm_alloc_for_dma
			("hist-test",
			PAGE_ALIGN(size) / PAGE_SIZE, 0, flags);
	}
	if (hist_buffer_addr)
		ret = 0;
	return ret;
}

static ssize_t hist_test_show(struct class *cla,
			      struct class_attribute *attr,
			      char *buf)
{
#define VI_HIST_MAX_MIN (0x2e03)
#define VI_HIST_SPL_VAL (0x2e04)
#define VI_HIST_SPL_PIX_CNT (0x2e05)
#define VI_HIST_CHROMA_SUM (0x2e06)
	ssize_t len = 0;
	u32 hist_result[4] = {0};

	if (hist_test_flag) {
		if (cur_dev->display_module != S5_DISPLAY_MODULE) {
			hist_result[0] = READ_VCBUS_REG(VI_HIST_MAX_MIN);
			hist_result[1] = READ_VCBUS_REG(VI_HIST_SPL_VAL);
			hist_result[2] = READ_VCBUS_REG(VI_HIST_SPL_PIX_CNT);
			hist_result[3] = READ_VCBUS_REG(VI_HIST_CHROMA_SUM);
		}

		len += sprintf(buf + len, "\n======time %d =====\n",
			       hist_print_count + 1);
		len += sprintf(buf + len, "hist_max_min: 0x%08x\n",
			       hist_result[0]);
		len += sprintf(buf + len, "hist_spl_val: 0x%08x\n",
			       hist_result[1]);
		len += sprintf(buf + len, "hist_spl_pix_cnt: 0x%08x\n",
			       hist_result[2]);
		len += sprintf(buf + len, "hist_chroma_sum: 0x%08x\n",
			       hist_result[3]);
		msleep(50);
		hist_print_count++;
	} else {
		len +=
			sprintf(buf + len, "no hist data\n");
	}
	return len;
}

static ssize_t hist_test_store(struct class *cla,
			       struct class_attribute *attr,
			       const char *buf,
			       size_t count)
{
#define VI_HIST_CTRL (0x2e00)
#define VI_HIST_H_START_END (0x2e01)
#define VI_HIST_V_START_END (0x2e02)
#define VI_HIST_PIC_SIZE (0x2e28)
#define VIU_EOTF_CTL (0x31d0)
#define XVYCC_LUT_CTL (0x3165)
#define XVYCC_INV_LUT_CTL (0x3164)
#define XVYCC_VD1_RGB_CTRST (0x3170)
	int parsed[3];
	int frame_width = 0, frame_height = 0;
	int pat_val = 0, canvas_width;
	u32 hist_dst_w, hist_dst_h;
	const struct vinfo_s *ginfo = get_current_vinfo();
	struct disp_info_s *layer = &glayer_info[0];

	if (likely(parse_para(buf, 3, parsed) == 3)) {
		frame_width = parsed[0];
		frame_height = parsed[1];
		pat_val = parsed[2];
	}
	if (!ginfo || ginfo->mode == VMODE_INVALID)
		return 0;

	if (cur_dev->display_module == S5_DISPLAY_MODULE)
		return 0;

	if (cur_dispbuf[0] &&
	    cur_dispbuf[0] != &vf_local[0] &&
	    cur_dispbuf[0] != &hist_test_vf)
		pat_val = 0;
	if (!frame_width || !frame_height)
		pat_val = 0;

	if (legacy_vpp)
		pat_val = 0;

	if (pat_val > 0 && pat_val <= 0x3fffffff) {
		if (!hist_test_flag) {
			memset(&hist_test_vf, 0, sizeof(hist_test_vf));
			canvas_width = (frame_width + 31) & (~31);
			if (!alloc_hist_test_buffer
				(canvas_width * frame_height * 3)) {
				hist_test_vf.canvas0Addr =
					LAYER1_CANVAS_BASE_INDEX + 5;
				hist_test_vf.canvas1Addr =
					LAYER1_CANVAS_BASE_INDEX + 5;
				canvas_config
					(LAYER1_CANVAS_BASE_INDEX + 5,
					(unsigned int)hist_buffer_addr,
					canvas_width * 3,
					frame_height,
					CANVAS_ADDR_NOWRAP,
					CANVAS_BLKMODE_LINEAR);
				hist_test_vf.width = frame_width;
				hist_test_vf.height = frame_height;
				hist_test_vf.type = VIDTYPE_VIU_444 |
					VIDTYPE_VIU_SINGLE_PLANE |
					VIDTYPE_VIU_FIELD | VIDTYPE_PIC;
				/* indicate the vframe is a full range frame */
				hist_test_vf.signal_type =
					/* HD default 709 limit */
					  (1 << 29) /* video available */
					| (5 << 26) /* unspecified */
					| (1 << 25) /* full */
					| (1 << 24) /* color available */
					| (1 << 16) /* bt709 */
					| (1 << 8)  /* bt709 */
					| (1 << 0); /* bt709 */
				hist_test_vf.duration_pulldown = 0;
				hist_test_vf.index = 0;
				hist_test_vf.pts = 0;
				hist_test_vf.pts_us64 = 0;
				hist_test_vf.ratio_control = 0;
				hist_test_flag = true;
				WRITE_VCBUS_REG(VIU_EOTF_CTL, 0);
				WRITE_VCBUS_REG(XVYCC_LUT_CTL, 0);
				WRITE_VCBUS_REG(XVYCC_INV_LUT_CTL, 0);
				WRITE_VCBUS_REG(VPP_VADJ_CTRL, 0);
				WRITE_VCBUS_REG(VPP_GAINOFF_CTRL0, 0);
				WRITE_VCBUS_REG(VPP_VE_ENABLE_CTRL, 0);
				WRITE_VCBUS_REG(XVYCC_VD1_RGB_CTRST, 0);
				if (ginfo) {
					if (ginfo->width >
						(layer->layer_width
						+ layer->layer_left))
						hist_dst_w =
						layer->layer_width
						+ layer->layer_left;
					else
						hist_dst_w =
						ginfo->width;
					if (ginfo->field_height >
						(layer->layer_height
						+ layer->layer_top))
						hist_dst_h =
						layer->layer_height
						+ layer->layer_top;
					else
						hist_dst_h =
						ginfo->field_height;
					WRITE_VCBUS_REG
					(VI_HIST_H_START_END,
					hist_dst_w & 0xfff);
					WRITE_VCBUS_REG
					(VI_HIST_V_START_END,
					(hist_dst_h - 2) & 0xfff);
					WRITE_VCBUS_REG
					(VI_HIST_PIC_SIZE,
					(ginfo->width & 0xfff) |
					(ginfo->field_height << 16));
					WRITE_VCBUS_REG(VI_HIST_CTRL, 0x3803);
				} else {
					WRITE_VCBUS_REG
					(VI_HIST_H_START_END, 0);
					WRITE_VCBUS_REG
					(VI_HIST_V_START_END, 0);
					WRITE_VCBUS_REG
					(VI_HIST_PIC_SIZE,
					1080 | 1920);
					WRITE_VCBUS_REG(VI_HIST_CTRL, 0x3801);
				}
			}
		}
		WRITE_VCBUS_REG(VPP_VD1_CLIP_MISC0, pat_val);
		WRITE_VCBUS_REG(VPP_VD1_CLIP_MISC1, pat_val);
		WRITE_VCBUS_REG(AMDV_PATH_CTRL, 0x3f);
		msleep(50);
		hist_print_count = 0;
	} else if (hist_test_flag) {
		hist_test_flag = false;
		msleep(50);
		free_alloced_hist_test_buffer();
		WRITE_VCBUS_REG(VPP_VD1_CLIP_MISC0, 0x3fffffff);
		WRITE_VCBUS_REG(VPP_VD1_CLIP_MISC1, 0);
		WRITE_VCBUS_REG(AMDV_PATH_CTRL, 0xf);
		safe_switch_videolayer(0, false, false);
	}
	return strnlen(buf, count);
}

static ssize_t videopip_blackout_policy_show(struct class *cla,
					     struct class_attribute *attr,
					     char *buf)
{
	return sprintf(buf, "%d\n", blackout[1]);
}

static ssize_t videopip_blackout_policy_store(struct class *cla,
					      struct class_attribute *attr,
					      const char *buf, size_t count)
{
	int r;

	r = kstrtouint(buf, 0, &blackout[1]);
	if (r < 0)
		return -EINVAL;

	return count;
}

static ssize_t videopip2_blackout_policy_show(struct class *cla,
					     struct class_attribute *attr,
					     char *buf)
{
	return sprintf(buf, "%d\n", blackout[2]);
}

static ssize_t videopip2_blackout_policy_store(struct class *cla,
					      struct class_attribute *attr,
					      const char *buf, size_t count)
{
	int r;

	r = kstrtouint(buf, 0, &blackout[2]);
	if (r < 0)
		return -EINVAL;

	return count;
}

static ssize_t videopip_axis_show(struct class *cla,
				  struct class_attribute *attr,
				  char *buf)
{
	int x0, y0, x1, y1;
	struct disp_info_s *layer = &glayer_info[1];

	x0 = layer->layer_left;
	y0 = layer->layer_top;
	x1 = layer->layer_width + x0 - 1;
	y1 = layer->layer_height + y0 - 1;
	return snprintf(buf, 40, "%d %d %d %d\n", x0, y0, x1, y1);
}

static ssize_t videopip_axis_store(struct class *cla,
				   struct class_attribute *attr,
				   const char *buf,
				   size_t count)
{
	struct disp_info_s *layer = &glayer_info[1];

	mutex_lock(&video_module_mutex);

	set_video_window(layer, buf);

	mutex_unlock(&video_module_mutex);

	return strnlen(buf, count);
}

static ssize_t videopip2_axis_show(struct class *cla,
				  struct class_attribute *attr,
				  char *buf)
{
	int x0, y0, x1, y1;
	struct disp_info_s *layer = &glayer_info[2];

	x0 = layer->layer_left;
	y0 = layer->layer_top;
	x1 = layer->layer_width + x0 - 1;
	y1 = layer->layer_height + y0 - 1;
	return snprintf(buf, 40, "%d %d %d %d\n", x0, y0, x1, y1);
}

static ssize_t videopip2_axis_store(struct class *cla,
				   struct class_attribute *attr,
				   const char *buf,
				   size_t count)
{
	struct disp_info_s *layer = &glayer_info[2];

	mutex_lock(&video_module_mutex);

	set_video_window(layer, buf);

	mutex_unlock(&video_module_mutex);

	return strnlen(buf, count);
}

static ssize_t videopip_crop_show(struct class *cla,
				  struct class_attribute *attr,
				  char *buf)
{
	u32 t, l, b, r;
	struct disp_info_s *layer = &glayer_info[1];

	t = layer->crop_top;
	l = layer->crop_left;
	b = layer->crop_bottom;
	r = layer->crop_right;
	return snprintf(buf, 40, "%d %d %d %d\n", t, l, b, r);
}

static ssize_t videopip_crop_store(struct class *cla,
				   struct class_attribute *attr,
				   const char *buf,
				   size_t count)
{
	struct disp_info_s *layer = &glayer_info[1];

	mutex_lock(&video_module_mutex);

	set_video_crop(layer, buf);

	mutex_unlock(&video_module_mutex);

	return strnlen(buf, count);
}

static ssize_t videopip2_crop_show(struct class *cla,
				  struct class_attribute *attr,
				  char *buf)
{
	u32 t, l, b, r;
	struct disp_info_s *layer = &glayer_info[2];

	t = layer->crop_top;
	l = layer->crop_left;
	b = layer->crop_bottom;
	r = layer->crop_right;
	return snprintf(buf, 40, "%d %d %d %d\n", t, l, b, r);
}

static ssize_t videopip2_crop_store(struct class *cla,
				   struct class_attribute *attr,
				   const char *buf,
				   size_t count)
{
	struct disp_info_s *layer = &glayer_info[2];

	mutex_lock(&video_module_mutex);

	set_video_crop(layer, buf);

	mutex_unlock(&video_module_mutex);

	return strnlen(buf, count);
}

static ssize_t videopip_disable_show(struct class *cla,
				     struct class_attribute *attr,
				     char *buf)
{
	return sprintf(buf, "%d\n", vd_layer[1].disable_video);
}

static ssize_t videopip_disable_store(struct class *cla,
				      struct class_attribute *attr,
				      const char *buf, size_t count)
{
	int r;
	int val;

	if (debug_flag & DEBUG_FLAG_BASIC_INFO)
		pr_info("%s(%s)\n", __func__, buf);

	r = kstrtoint(buf, 0, &val);
	if (r < 0)
		return -EINVAL;

	if (_videopip_set_disable(VD2_PATH, val) < 0)
		return -EINVAL;

	return count;
}

static ssize_t videopip2_disable_show(struct class *cla,
				     struct class_attribute *attr,
				     char *buf)
{
	return sprintf(buf, "%d\n", vd_layer[2].disable_video);
}

static ssize_t videopip2_disable_store(struct class *cla,
				      struct class_attribute *attr,
				      const char *buf, size_t count)
{
	int r;
	int val;

	if (debug_flag & DEBUG_FLAG_BASIC_INFO)
		pr_info("%s(%s)\n", __func__, buf);

	r = kstrtoint(buf, 0, &val);
	if (r < 0)
		return -EINVAL;

	if (_videopip_set_disable(VD3_PATH, val) < 0)
		return -EINVAL;

	return count;
}

static ssize_t videopip_screen_mode_show(struct class *cla,
					 struct class_attribute *attr,
					 char *buf)
{
	struct disp_info_s *layer = &glayer_info[1];
	static const char * const wide_str[] = {
		"normal", "full stretch", "4-3", "16-9", "non-linear",
		"normal-noscaleup",
		"4-3 ignore", "4-3 letter box", "4-3 pan scan", "4-3 combined",
		"16-9 ignore", "16-9 letter box", "16-9 pan scan",
		"16-9 combined", "Custom AR", "AFD", "non-linear-T", "21-9"
	};

	if (layer->wide_mode < ARRAY_SIZE(wide_str))
		return sprintf(buf, "%d:%s\n",
			layer->wide_mode,
			wide_str[layer->wide_mode]);
	else
		return 0;
}

static ssize_t videopip_screen_mode_store(struct class *cla,
					  struct class_attribute *attr,
					  const char *buf, size_t count)
{
	unsigned long mode;
	int ret = 0;
	struct disp_info_s *layer = &glayer_info[1];

	ret = kstrtoul(buf, 0, (unsigned long *)&mode);
	if (ret < 0)
		return -EINVAL;

	if (mode < VIDEO_WIDEOPTION_MAX &&
	    mode != layer->wide_mode) {
		layer->wide_mode = mode;
		if (debug_flag & DEBUG_FLAG_BASIC_INFO)
			pr_info("video_screen_mode sysfs:%d->%ld %s\n",
				layer->wide_mode, mode, current->comm);
		if (layer->layer_id == 1) {
			if (vd_layer[1].vpp_index == VPP0)
				vd_layer[1].property_changed = true;
			else
				update_vppx_property(layer->layer_id);
		} else if (layer->layer_id == 2) {
			if (vd_layer[2].vpp_index == VPP0)
				vd_layer[2].property_changed = true;
			else
				update_vppx_property(layer->layer_id);
		}
	}
	return count;
}

static ssize_t videopip2_screen_mode_show(struct class *cla,
					 struct class_attribute *attr,
					 char *buf)
{
	struct disp_info_s *layer = &glayer_info[2];
	static const char * const wide_str[] = {
		"normal", "full stretch", "4-3", "16-9", "non-linear",
		"normal-noscaleup",
		"4-3 ignore", "4-3 letter box", "4-3 pan scan", "4-3 combined",
		"16-9 ignore", "16-9 letter box", "16-9 pan scan",
		"16-9 combined", "Custom AR", "AFD", "non-linear-T", "21-9"
	};

	if (layer->wide_mode < ARRAY_SIZE(wide_str))
		return sprintf(buf, "%d:%s\n",
			layer->wide_mode,
			wide_str[layer->wide_mode]);
	else
		return 0;
}

static ssize_t videopip2_screen_mode_store(struct class *cla,
					  struct class_attribute *attr,
					  const char *buf, size_t count)
{
	unsigned long mode;
	int ret = 0;
	struct disp_info_s *layer = &glayer_info[2];

	ret = kstrtoul(buf, 0, (unsigned long *)&mode);
	if (ret < 0)
		return -EINVAL;

	if (mode < VIDEO_WIDEOPTION_MAX &&
	    mode != layer->wide_mode) {
		layer->wide_mode = mode;
		if (debug_flag & DEBUG_FLAG_BASIC_INFO)
			pr_info("video_screen_mode sysfs:%d->%ld %s\n",
				layer->wide_mode, mode, current->comm);
		if (layer->layer_id == 1) {
			if (vd_layer[1].vpp_index == VPP0)
				vd_layer[1].property_changed = true;
			else
				update_vppx_property(layer->layer_id);
		} else if (layer->layer_id == 2) {
			if (vd_layer[2].vpp_index == VPP0)
				vd_layer[2].property_changed = true;
			else
				update_vppx_property(layer->layer_id);
		}
	}
	return count;
}

static ssize_t videopip_loop_show(struct class *cla,
				  struct class_attribute *attr,
				  char *buf)
{
	return sprintf(buf, "%d\n", pip_loop);
}

static ssize_t videopip_loop_store(struct class *cla,
				   struct class_attribute *attr,
				   const char *buf, size_t count)
{
	int r;
	u32 val;

	r = kstrtouint(buf, 0, &val);
	if (r < 0)
		return -EINVAL;

	pip_loop = val;
	return count;
}

static ssize_t videopip2_loop_show(struct class *cla,
				  struct class_attribute *attr,
				  char *buf)
{
	return sprintf(buf, "%d\n", pip2_loop);
}

static ssize_t videopip2_loop_store(struct class *cla,
				   struct class_attribute *attr,
				   const char *buf, size_t count)
{
	int r;
	u32 val;

	r = kstrtouint(buf, 0, &val);
	if (r < 0)
		return -EINVAL;

	pip2_loop = val;
	return count;
}

static ssize_t videopip_global_output_show(struct class *cla,
					   struct class_attribute *attr,
					   char *buf)
{
	return sprintf(buf, "%d\n",
		vd_layer[1].global_output);
}

static ssize_t videopip_global_output_store(struct class *cla,
					    struct class_attribute *attr,
					    const char *buf, size_t count)
{
	int r;

	r = kstrtouint(buf, 0, &vd_layer[1].global_output);
	if (r < 0)
		return -EINVAL;

	pr_info("%s(%d)\n", __func__, vd_layer[1].global_output);
	return count;
}

static ssize_t videopip2_global_output_show(struct class *cla,
					   struct class_attribute *attr,
					   char *buf)
{
	return sprintf(buf, "%d\n",
		vd_layer[2].global_output);
}

static ssize_t videopip2_global_output_store(struct class *cla,
					    struct class_attribute *attr,
					    const char *buf, size_t count)
{
	int r;

	r = kstrtouint(buf, 0, &vd_layer[2].global_output);
	if (r < 0)
		return -EINVAL;

	pr_info("%s(%d)\n", __func__, vd_layer[2].global_output);
	return count;
}

static ssize_t videopip_zorder_show(struct class *cla,
				    struct class_attribute *attr,
				    char *buf)
{
	struct disp_info_s *layer = &glayer_info[1];

	return sprintf(buf, "%d\n", layer->zorder);
}

static ssize_t videopip_zorder_store(struct class *cla,
				     struct class_attribute *attr,
				     const char *buf, size_t count)
{
	int zorder;
	int ret = 0;
	struct disp_info_s *layer = &glayer_info[1];

	ret = kstrtoint(buf, 0, &zorder);
	if (ret < 0)
		return -EINVAL;

	if (zorder != layer->zorder) {
		layer->zorder = zorder;
		vd_layer[1].property_changed = true;
	}
	return count;
}

static ssize_t videopip2_zorder_show(struct class *cla,
				    struct class_attribute *attr,
				    char *buf)
{
	struct disp_info_s *layer = &glayer_info[2];

	return sprintf(buf, "%d\n", layer->zorder);
}

static ssize_t videopip2_zorder_store(struct class *cla,
				     struct class_attribute *attr,
				     const char *buf, size_t count)
{
	int zorder;
	int ret = 0;
	struct disp_info_s *layer = &glayer_info[2];

	ret = kstrtoint(buf, 0, &zorder);
	if (ret < 0)
		return -EINVAL;

	if (zorder != layer->zorder) {
		layer->zorder = zorder;
		vd_layer[2].property_changed = true;
	}
	return count;
}

static ssize_t videopip_max_src_show(struct class *cla,
			       struct class_attribute *attr,
			       char *buf)
{
	int src_width, src_height;
	struct disp_info_s *layer = &glayer_info[1];

	src_width = layer->src_width_max;
	src_height = layer->src_height_max;
	return snprintf(buf, 40, "%d %d\n", src_width, src_height);
}

static ssize_t videopip2_max_src_show(struct class *cla,
			       struct class_attribute *attr,
			       char *buf)
{
	int src_width, src_height;
	struct disp_info_s *layer = &glayer_info[2];

	src_width = layer->src_width_max;
	src_height = layer->src_height_max;
	return snprintf(buf, 40, "%d %d\n", src_width, src_height);
}

static ssize_t aisr_state_show(char *buf)
{
	ssize_t len = 0;
	struct vpp_frame_par_s *_cur_frame_par = NULL;
	struct vppfilter_mode_s *vpp_filter = NULL;
	struct aisr_setting_s *aisr_mif_setting = &vd_layer[0].aisr_mif_setting;

	_cur_frame_par = &cur_dev->aisr_frame_parms;
	if (!_cur_frame_par)
		return len;
	if (!aisr_mif_setting->aisr_enable)
		return len;
	vpp_filter = &_cur_frame_par->vpp_filter;
	len += sprintf(buf + len,
		       "src_w:%u.src_h:%u.\n",
		       aisr_mif_setting->src_w, aisr_mif_setting->src_h);
	len += sprintf(buf + len,
		       "in_ratio:%u.\n",
		       aisr_mif_setting->in_ratio);
	len += sprintf(buf + len,
		       "phy_addr:%ld.\n",
		       aisr_mif_setting->phy_addr);
	len += sprintf(buf + len,
		       "reshape_output_w:%d.\n",
		       _cur_frame_par->reshape_output_w);
	len += sprintf(buf + len,
		       "reshape_output_h:%d.\n",
		       _cur_frame_par->reshape_output_h);
	len += sprintf(buf + len,
		       "nnhf_input_w:%d.\n",
		       _cur_frame_par->nnhf_input_w);
	len += sprintf(buf + len,
		       "nnhf_input_h:%d.\n",
		       _cur_frame_par->nnhf_input_h);
	len += sprintf(buf + len,
		       "crop_top:%d.\n",
		       _cur_frame_par->crop_top);
	len += sprintf(buf + len,
		       "crop_bottom:%d.\n",
		       _cur_frame_par->crop_bottom);
	len += sprintf(buf + len,
		       "vscale_skip_count:%d.\n",
		       _cur_frame_par->vscale_skip_count);
	len += sprintf(buf + len,
		       "hsc_rpt_p0_num0:%d.\n",
		       _cur_frame_par->hsc_rpt_p0_num0);
	len += sprintf(buf + len,
		       "vsc_top_rpt_l0_num:%d.\n",
		       _cur_frame_par->vsc_top_rpt_l0_num);
	len += sprintf(buf + len, "hscale phase step 0x%x.\n",
		       vpp_filter->vpp_hsc_start_phase_step);
	len += sprintf(buf + len, "vscale phase step 0x%x.\n",
		       vpp_filter->vpp_vsc_start_phase_step);
	len += sprintf(buf + len, "pps pre hsc enable %d.\n",
		       vpp_filter->vpp_pre_hsc_en);
	len += sprintf(buf + len, "pps pre vsc enable %d.\n",
		       vpp_filter->vpp_pre_vsc_en);
	len += sprintf(buf + len, "hscale filter coef %d.\n",
		       vpp_filter->vpp_horz_filter);
	len += sprintf(buf + len, "vscale filter coef %d.\n",
		       vpp_filter->vpp_vert_filter);
	len += sprintf(buf + len, "vpp_vert_chroma_filter_en %d.\n",
		       vpp_filter->vpp_vert_chroma_filter_en);
	len += sprintf(buf + len, "VPP_hsc_startp 0x%x.\n",
		       _cur_frame_par->VPP_hsc_startp);
	len += sprintf(buf + len, "VPP_hsc_endp 0x%x.\n",
		       _cur_frame_par->VPP_hsc_endp);
	len += sprintf(buf + len, "VPP_vsc_startp 0x%x.\n",
		       _cur_frame_par->VPP_vsc_startp);
	len += sprintf(buf + len, "VPP_vsc_endp 0x%x.\n",
		       _cur_frame_par->VPP_vsc_endp);
	return len;
}
static ssize_t vdx_state_show(u32 index, char *buf)
{
	ssize_t len = 0;
	struct vppfilter_mode_s *vpp_filter = NULL;
	struct vpp_frame_par_s *_cur_frame_par = NULL;
	struct video_layer_s *_vd_layer = NULL;
	struct disp_info_s *layer_info = NULL;

	if (index >= MAX_VD_LAYER)
		return 0;
	_cur_frame_par = cur_frame_par[index];
	_vd_layer = &vd_layer[index];
	layer_info = &glayer_info[index];

	if (!_cur_frame_par)
		return len;
	vpp_filter = &_cur_frame_par->vpp_filter;
	len += sprintf(buf + len,
		       "zoom_start_x_lines:%u.zoom_end_x_lines:%u.\n",
		       _vd_layer->start_x_lines, _vd_layer->end_x_lines);
	len += sprintf(buf + len,
		       "zoom_start_y_lines:%u.zoom_end_y_lines:%u.\n",
		       _vd_layer->start_y_lines, _vd_layer->end_x_lines);
	len += sprintf(buf + len, "frame parameters: pic_in_height %u.\n",
		       _cur_frame_par->VPP_pic_in_height_);
	len += sprintf(buf + len,
		       "frame parameters: VPP_line_in_length_ %u.\n",
		       _cur_frame_par->VPP_line_in_length_);
	len += sprintf(buf + len, "vscale_skip_count %u.\n",
		       _cur_frame_par->vscale_skip_count);
	len += sprintf(buf + len, "hscale_skip_count %u.\n",
		       _cur_frame_par->hscale_skip_count);
	len += sprintf(buf + len, "supscl_path %u.\n",
		       _cur_frame_par->supscl_path);
	len += sprintf(buf + len, "supsc0_enable %u.\n",
		       _cur_frame_par->supsc0_enable);
	len += sprintf(buf + len, "supsc1_enable %u.\n",
		       _cur_frame_par->supsc1_enable);
	len += sprintf(buf + len, "supsc0_hori_ratio %u.\n",
		       _cur_frame_par->supsc0_hori_ratio);
	len += sprintf(buf + len, "supsc1_hori_ratio %u.\n",
		       _cur_frame_par->supsc1_hori_ratio);
	len += sprintf(buf + len, "supsc0_vert_ratio %u.\n",
		       _cur_frame_par->supsc0_vert_ratio);
	len += sprintf(buf + len, "supsc1_vert_ratio %u.\n",
		       _cur_frame_par->supsc1_vert_ratio);
	len += sprintf(buf + len, "spsc0_h_in %u.\n",
		       _cur_frame_par->spsc0_h_in);
	len += sprintf(buf + len, "spsc1_h_in %u.\n",
		       _cur_frame_par->spsc1_h_in);
	len += sprintf(buf + len, "spsc0_w_in %u.\n",
		       _cur_frame_par->spsc0_w_in);
	len += sprintf(buf + len, "spsc1_w_in %u.\n",
		       _cur_frame_par->spsc1_w_in);
	len += sprintf(buf + len, "video_input_w %u.\n",
		       _cur_frame_par->video_input_w);
	len += sprintf(buf + len, "video_input_h %u.\n",
		       _cur_frame_par->video_input_h);
	len += sprintf(buf + len, "clk_in_pps %u.\n",
		       _cur_frame_par->clk_in_pps);
	if (index == 0) {
#ifdef TV_3D_FUNCTION_OPEN
		len += sprintf(buf + len, "vpp_2pic_mode %u.\n",
			_cur_frame_par->vpp_2pic_mode);
		len += sprintf(buf + len, "vpp_3d_scale %u.\n",
			_cur_frame_par->vpp_3d_scale);
		len += sprintf(buf + len, "vpp_3d_mode %u.\n",
			_cur_frame_par->vpp_3d_mode);
#endif
	}
	len += sprintf(buf + len, "hscale phase step 0x%x.\n",
		       vpp_filter->vpp_hsc_start_phase_step);
	len += sprintf(buf + len, "vscale phase step 0x%x.\n",
		       vpp_filter->vpp_vsc_start_phase_step);
	len += sprintf(buf + len, "pps pre hsc enable %d.\n",
		       vpp_filter->vpp_pre_hsc_en);
	len += sprintf(buf + len, "pps pre vsc enable %d.\n",
		       vpp_filter->vpp_pre_vsc_en);
	len += sprintf(buf + len, "hscale filter coef %d.\n",
		       vpp_filter->vpp_horz_filter);
	len += sprintf(buf + len, "vscale filter coef %d.\n",
		       vpp_filter->vpp_vert_filter);
	len += sprintf(buf + len, "vpp_vert_chroma_filter_en %d.\n",
		       vpp_filter->vpp_vert_chroma_filter_en);
	len += sprintf(buf + len, "post_blend_vd_h_start 0x%x.\n",
		       _cur_frame_par->VPP_post_blend_vd_h_start_);
	len += sprintf(buf + len, "post_blend_vd_h_end 0x%x.\n",
		       _cur_frame_par->VPP_post_blend_vd_h_end_);
	len += sprintf(buf + len, "post_blend_vd_v_start 0x%x.\n",
		       _cur_frame_par->VPP_post_blend_vd_v_start_);
	len += sprintf(buf + len, "post_blend_vd_v_end 0x%x.\n",
		       _cur_frame_par->VPP_post_blend_vd_v_end_);
	len += sprintf(buf + len, "VPP_hd_start_lines_ 0x%x.\n",
		       _cur_frame_par->VPP_hd_start_lines_);
	len += sprintf(buf + len, "VPP_hd_end_lines_ 0x%x.\n",
		       _cur_frame_par->VPP_hd_end_lines_);
	len += sprintf(buf + len, "VPP_vd_start_lines_ 0x%x.\n",
		       _cur_frame_par->VPP_vd_start_lines_);
	len += sprintf(buf + len, "VPP_vd_end_lines_ 0x%x.\n",
		       _cur_frame_par->VPP_vd_end_lines_);
	len += sprintf(buf + len, "VPP_hsc_startp 0x%x.\n",
		       _cur_frame_par->VPP_hsc_startp);
	len += sprintf(buf + len, "VPP_hsc_endp 0x%x.\n",
		       _cur_frame_par->VPP_hsc_endp);
	len += sprintf(buf + len, "VPP_vsc_startp 0x%x.\n",
		       _cur_frame_par->VPP_vsc_startp);
	len += sprintf(buf + len, "VPP_vsc_endp 0x%x.\n",
		       _cur_frame_par->VPP_vsc_endp);
	if (index == 0)
		len += aisr_state_show(buf + len);
	if (layer_info) {
		len += sprintf(buf + len, "mirror: %d\n",
			layer_info->mirror);
		len += sprintf(buf + len, "reverse: %s\n",
			layer_info->reverse ? "true" : "false");
		if (layer_info->afd_enable) {
			len += sprintf(buf + len, "afd: enable\n");
			len += sprintf(buf + len, "afd_pos: %d %d %d %d\n",
				layer_info->afd_pos.x_start,
				layer_info->afd_pos.y_start,
				layer_info->afd_pos.x_end,
				layer_info->afd_pos.y_end);
			len += sprintf(buf + len, "afd_crop: %d %d %d %d\n",
				layer_info->afd_crop.top,
				layer_info->afd_crop.left,
				layer_info->afd_crop.bottom,
				layer_info->afd_crop.right);
		} else {
			len += sprintf(buf + len, "afd: disable\n");
		}
	}
	return len;
}

static ssize_t performance_debug_show(struct class *cla,
	struct class_attribute *attr,
	char *buf)
{
	return sprintf(buf, "performance_debug: %d\n",
		performance_debug);
}

static ssize_t performance_debug_store(struct class *cla,
	struct class_attribute *attr,
	const char *buf, size_t count)
{
	int ret;
	u32 enable;

	ret = kstrtoint(buf, 0, &enable);
	if (ret < 0)
		return -EINVAL;
	performance_debug = enable;
	return count;
}

static ssize_t over_field_state_show(struct class *cla,
	struct class_attribute *attr,
	char *buf)
{
	u32 cur_state = OVER_FIELD_NORMAL;
	u32 cur_over_field_wrong_cnt = 0;

#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
	cur_state = atomic_read(&cur_over_field_state);
	cur_over_field_wrong_cnt = wrong_state_change_cnt;
#endif
	return sprintf(buf, "cur_over_filed_state: %d; wrong state cnt:%d; over cnt:%d %d\n",
		cur_state, cur_over_field_wrong_cnt,
		over_field_case1_cnt, over_field_case2_cnt);
}

static ssize_t over_field_state_store(struct class *cla,
	struct class_attribute *attr,
	const char *buf, size_t count)
{
	int ret;
	u32 val;

	ret = kstrtoint(buf, 0, &val);
	if (ret < 0)
		return -EINVAL;
	over_field_case1_cnt = 0;
	over_field_case2_cnt = 0;
#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
	update_over_field_states(OVER_FIELD_NORMAL, val ? true : false);
	wrong_state_change_cnt = 0;
	pr_info("%s to clear over field state.\n", val ? "force" : "no force");
#endif
	return count;
}

static ssize_t video_force_skip_cnt_show(struct class *cla, struct class_attribute *attr,
			       char *buf)
{
	return sprintf(buf, "force_skip_cnt:%d, bit0~1 for vd1, bit2~3 for vd2\n",
		       force_skip_cnt);
}

static ssize_t video_force_skip_cnt_store(struct class *cla,
		struct class_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long cnt;
	int ret = 0;

	ret = kstrtoul(buf, 0, (unsigned long *)&cnt);
	if (ret < 0)
		return -EINVAL;

	force_skip_cnt = cnt;

	return count;
}

static ssize_t video_state_show(struct class *cla,
				struct class_attribute *attr,
				char *buf)
{
	return vdx_state_show(VD1_PATH, buf);
}

static ssize_t videopip_state_show(struct class *cla,
				   struct class_attribute *attr,
				   char *buf)
{
	return vdx_state_show(VD2_PATH, buf);
}

static ssize_t videopip2_state_show(struct class *cla,
				   struct class_attribute *attr,
				   char *buf)
{
	return vdx_state_show(VD3_PATH, buf);
}

static ssize_t video_vd_proc_state_show(struct class *cla,
				struct class_attribute *attr,
				char *buf)
{
	return video_vd_proc_state_dump(buf);
}

static ssize_t path_select_show(struct class *cla,
				struct class_attribute *attr,
				char *buf)
{
	return snprintf(buf, 40, "vd1: %d vd2: %d, vd3: %d\n",
		glayer_info[0].display_path_id,
		glayer_info[1].display_path_id,
		glayer_info[2].display_path_id);
}

static ssize_t path_select_store(struct class *cla,
				 struct class_attribute *attr,
				 const char *buf, size_t count)
{
	int parsed[3];

	if (likely(parse_para(buf, 3, parsed) == 3)) {
		pr_info("VID: cur %d %d %d, set %d %d %d\n",
				glayer_info[0].display_path_id,
				glayer_info[1].display_path_id,
				glayer_info[2].display_path_id,
				parsed[0], parsed[1], parsed[2]);
		if (glayer_info[0].display_path_id != parsed[0]) {
			pr_info("VID: store VD1 path_id changed %d->%d\n",
				glayer_info[0].display_path_id, parsed[0]);
			glayer_info[0].display_path_id = parsed[0];
			vd_layer[0].property_changed = true;
		}
		if (glayer_info[1].display_path_id != parsed[1]) {
			pr_info("VID: store VD2 path_id changed %d->%d\n",
				glayer_info[1].display_path_id, parsed[1]);
			glayer_info[1].display_path_id = parsed[1];
			vd_layer[1].property_changed = true;
		}
		if (glayer_info[2].display_path_id != parsed[2]) {
			pr_info("VID: store VD3 path_id changed %d->%d\n",
				glayer_info[2].display_path_id, parsed[2]);
			glayer_info[2].display_path_id = parsed[2];
			vd_layer[2].property_changed = true;
		}
	} else {
		pr_err("need 3 input params\n");
	}
	return strnlen(buf, count);
}

static ssize_t vpp_crc_show
	(struct class *cla,
	struct class_attribute *attr,
	char *buf)
{
	return snprintf(buf, 64, "vpp_crc_en: %d vpp_crc_result: %x\n\n",
		vpp_crc_en,
		vpp_crc_result);
}

static ssize_t vpp_crc_store
	(struct class *cla,
	struct class_attribute *attr,
	const char *buf, size_t count)
{
	int ret;

	ret = kstrtouint(buf, 0, &vpp_crc_en);
	if (ret < 0)
		return -EINVAL;
	return count;
}

static ssize_t vpp_crc_viu2_show
	(struct class *cla,
	struct class_attribute *attr,
	char *buf)
{
	int vpp_crc_viu2_result = 0;

	if (amvideo_meson_dev.cpu_type >= MESON_CPU_MAJOR_ID_SC2_)
		vpp_crc_viu2_result = vpp_crc_viu2_check(vpp_crc_viu2_en);
	return snprintf(buf, 64, "crc_viu2_en: %d crc_vui2_result: %x\n\n",
		vpp_crc_viu2_en,
		vpp_crc_viu2_result);
}

static ssize_t vpp_crc_viu2_store
	(struct class *cla,
	struct class_attribute *attr,
	const char *buf, size_t count)
{
	int ret;

	ret = kstrtouint(buf, 0, &vpp_crc_viu2_en);
	if (ret < 0)
		return -EINVAL;
	if (amvideo_meson_dev.cpu_type >= MESON_CPU_MAJOR_ID_SC2_)
		enable_vpp_crc_viu2(vpp_crc_viu2_en);
	return count;
}

static ssize_t film_grain_show
	(struct class *cla,
	struct class_attribute *attr,
	char *buf)
{
	return snprintf(buf, 40, "fgrain_enable vd1: %d vd2: %d vd3: %d\n",
		glayer_info[0].fgrain_enable,
		glayer_info[1].fgrain_enable,
		glayer_info[2].fgrain_enable);
}

static ssize_t film_grain_store
	(struct class *cla,
	struct class_attribute *attr,
	const char *buf, size_t count)
{
	int parsed[3];

	if (likely(parse_para(buf, 3, parsed) == 3)) {
		glayer_info[0].fgrain_enable = parsed[0];
		glayer_info[1].fgrain_enable = parsed[1];
		glayer_info[2].fgrain_enable = parsed[2];
	} else {
		pr_err("need 3 input params\n");
	}
	return strnlen(buf, count);
}

static ssize_t film_grain_support_show
	(struct class *cla,
	struct class_attribute *attr,
	char *buf)
{
	return snprintf(buf, 40, "%d\n",
		((glayer_info[2].fgrain_support ? 1 : 0) << 2) |
		((glayer_info[1].fgrain_support ? 1 : 0) << 1) |
		((glayer_info[0].fgrain_support ? 1 : 0) << 0));
}

static ssize_t pip_alpha_store
	(struct class *cla,
	struct class_attribute *attr,
	const char *buf, size_t count)
{
	int i = 0;
	int param_num;
	int parsed[66];
	int layer_id, win_num, win_en = 0;
	struct pip_alpha_scpxn_s alpha_win;

	if (likely(parse_para(buf, 2, parsed) >= 2)) {
		layer_id = parsed[0];
		win_num = parsed[1];
		param_num = win_num * 4 + 2;
		if (likely(parse_para(buf, param_num, parsed) == param_num)) {
			for (i = 0; i < win_num; i++) {
				alpha_win.scpxn_bgn_h[i] = parsed[2 + i * 4];
				alpha_win.scpxn_end_h[i] = parsed[3 + i * 4];
				alpha_win.scpxn_bgn_v[i] = parsed[4 + i * 4];
				alpha_win.scpxn_end_v[i] = parsed[5 + i * 4];
				win_en |= 1 << i;
			}
			pr_info("layer_id=%d, win_num=%d, win_en=%d\n",
				layer_id, win_num, win_en);
			vd_layer[layer_id].alpha_win_en = win_en;
			memcpy(&vd_layer[layer_id].alpha_win, &alpha_win,
				sizeof(struct pip_alpha_scpxn_s));
			vd_layer[0].property_changed = true;
		}
	}

	return strnlen(buf, count);
}

/*
 * default setting scenes is 23
 */
static ssize_t pq_default_show(struct class *cla,
			       struct class_attribute *attr, char *buf)
{
	ssize_t count;
	int i = 0;
	char end[4] = "\n";
	char temp[20] = "default scene pq:";

	count = sprintf(buf, "%s", temp);
	while (i < SCENES_VALUE)
		count += sprintf(buf + count, " %d",
				 vpp_scenes[AI_SCENES_MAX - 1].pq_values[i++]);
	count += sprintf(buf + count, " %s", end);
	return count;
}

static ssize_t pq_default_store(struct class *cla,
				struct class_attribute *attr,
				const char *buf, size_t count)
{
	int i = 0;
	int parsed[SCENES_VALUE];

	if (likely(parse_para(buf, SCENES_VALUE, parsed) == SCENES_VALUE)) {
		vpp_scenes[AI_SCENES_MAX - 1].pq_scenes = DEFAUT_SETTING;
		while (i < SCENES_VALUE) {
			vpp_scenes[AI_SCENES_MAX - 1].pq_values[i] = parsed[i];
			i++;
		}
	}
	i = 0;
	pr_info("the default scene pq param: ");
	while (i < SCENES_VALUE)
		pr_info("%d ", vpp_scenes[AI_SCENES_MAX - 1].pq_values[i++]);
	pr_info("\n");

	return strnlen(buf, count);
}

static ssize_t pq_data_store(struct class *cla,
			     struct class_attribute *attr,
			     const char *buf, size_t count)

{
	int parsed[4] = {0, 0, 0, 0};
	ssize_t buflen;
	int ret;

	buflen = strlen(buf);
	if (buflen <= 0)
		return 0;

	ret = parse_para(buf, 4, parsed);
	if (ret == 4 && parsed[0]) {
		if (parsed[1] < AI_SCENES_MAX && parsed[1] >= 0 &&
		    parsed[2] < SCENES_VALUE && parsed[2] >= 0 &&
		    parsed[3] >= 0) {
			vpp_scenes[parsed[1]].pq_scenes =
				(enum ai_scenes)parsed[1];
			vpp_scenes[parsed[1]].pq_values[parsed[2]] = parsed[3];

		} else {
			pr_info("the 2nd param: 0~23,the 3rd param: 0~9,the 4th param: >=0\n");
		}
	} else if (ret == 3 && parsed[0] == 0) {
		if (parsed[1] < AI_SCENES_MAX && parsed[1] >= 0 &&
		    parsed[2] < SCENES_VALUE && parsed[2] >= 0)
			pr_info("pq value: %d\n",
				vpp_scenes[parsed[1]].pq_values[parsed[2]]);
		else
			pr_info("the 2nd param: 0~23,the 3rd param: 0~9\n");
	} else {
		if (parsed[0] == 0)
			pr_info("1st param 0 is to read pq value,need set 3 param\n");
		else
			pr_info("1st param 1 is to set pq value,need set 4 param\n");
	}
	return strnlen(buf, count);
}

static ssize_t aipq_dbg_data_show(struct class *cla,
					struct class_attribute *attr,
					char *buf)
{
	ssize_t len = 0;
	char *stemp = NULL;
	int i, j;
	int stemp_len = AI_SCENES_CUR_USE_MAX * SCENES_CUR_USE_VALUE * 3;

	stemp = kmalloc(stemp_len, GFP_KERNEL);
	if (!stemp)
		return 0;

	memset(stemp, 0, stemp_len);

	for (i = 0; i < AI_SCENES_CUR_USE_MAX; i++) {
		for (j = 0; j < SCENES_CUR_USE_VALUE; j++) {
			int_convert_str(vpp_pq_data[i][j],
				stemp, 10);
		}
	}

	len = sprintf(buf, "for_tool:%s\n", stemp);
	kfree(stemp);

	return len;
}

static ssize_t aipq_dbg_data_store(struct class *cla,
			     struct class_attribute *attr,
			     const char *buf, size_t count)
{
	int i, j;
	char *buf_orig, *parm[8] = {NULL};
	int aipq_data[AI_SCENES_CUR_USE_MAX * SCENES_CUR_USE_VALUE] = {0};

	if (!buf)
		return count;

	buf_orig = kstrdup(buf, GFP_KERNEL);
	if (!buf_orig)
		return -ENOMEM;

	parse_param_to_char(buf_orig, (char **)&parm);

	if (!strncmp(parm[0], "w", 1)) {
		if (strlen(parm[1]) !=
			AI_SCENES_CUR_USE_MAX * SCENES_CUR_USE_VALUE * 3) {
			goto free_buf;
		}

		str_sapr_conv(parm[1], 3,
		aipq_data,
		AI_SCENES_CUR_USE_MAX * SCENES_CUR_USE_VALUE);

		for (i = 0; i < AI_SCENES_CUR_USE_MAX; i++)
			for (j = 0; j < SCENES_CUR_USE_VALUE; j++)
				vpp_pq_data[i][j] =
					aipq_data[i * SCENES_CUR_USE_VALUE + j];
	} else if (!strncmp(parm[0], "r", 1)) {
		int i, j;

		for (i = 0; i < AI_SCENES_MAX; i++) {
			for (j = 0; j < SCENES_VALUE; j++)
				pr_info("%d   ", vpp_pq_data[i][j]);
		}
	} else {
		pr_info("aipq_dbg cmd invalid!!!\n");
	}

	kfree(buf_orig);
	return count;

free_buf:
	kfree(buf_orig);
	return -EINVAL;
}

static ssize_t hscaler_8tap_enable_show(struct class *cla,
					struct class_attribute *attr,
					char *buf)
{
	return snprintf(buf, 64, "hscaler_8tap_en: %d\n\n",
		hscaler_8tap_enable[0]);
}

static ssize_t hscaler_8tap_enable_store(struct class *cla,
					 struct class_attribute *attr,
					 const char *buf, size_t count)
{
	int ret;
	int hscaler_8tap_en;

	ret = kstrtoint(buf, 0, &hscaler_8tap_en);
	if (ret < 0)
		return -EINVAL;

	if (amvideo_meson_dev.has_hscaler_8tap[0] &&
	    hscaler_8tap_en != hscaler_8tap_enable[0]) {
		hscaler_8tap_enable[0] = hscaler_8tap_en;
		vd_layer[0].property_changed = true;
	}
	return count;
}

static ssize_t pip_hscaler_8tap_enable_show
	(struct class *cla,
	struct class_attribute *attr,
	char *buf)
{
	return snprintf(buf, 64, "pip hscaler_8tap_en: %d\n\n",
		hscaler_8tap_enable[1]);
}

static ssize_t pip_hscaler_8tap_enable_store
	(struct class *cla,
	struct class_attribute *attr,
	const char *buf, size_t count)
{
	int ret;
	int hscaler_8tap_en;

	ret = kstrtoint(buf, 0, &hscaler_8tap_en);
	if (ret < 0)
		return -EINVAL;

	if (amvideo_meson_dev.has_hscaler_8tap[1] &&
	    hscaler_8tap_en != hscaler_8tap_enable[1]) {
		hscaler_8tap_enable[1] = hscaler_8tap_en;
		if (vd_layer[1].vpp_index == VPP0) {
			vd_layer[1].property_changed = true;
		} else {
			vd_layer_vpp[0].property_changed = true;
			vd_layer_vpp[1].property_changed = true;
		}
	}
	return count;
}

static ssize_t pip2_hscaler_8tap_enable_show
	(struct class *cla,
	struct class_attribute *attr,
	char *buf)
{
	return snprintf(buf, 64, "pip2_hscaler_8tap_en: %d\n\n",
		hscaler_8tap_enable[2]);
}

static ssize_t pip2_hscaler_8tap_enable_store
	(struct class *cla,
	struct class_attribute *attr,
	const char *buf, size_t count)
{
	int ret;
	int hscaler_8tap_en;

	ret = kstrtoint(buf, 0, &hscaler_8tap_en);
	if (ret < 0)
		return -EINVAL;

	if (amvideo_meson_dev.has_hscaler_8tap[2] &&
	    hscaler_8tap_en != hscaler_8tap_enable[2]) {
		hscaler_8tap_enable[2] = hscaler_8tap_en;
		if (vd_layer[2].vpp_index == VPP0) {
			vd_layer[2].property_changed = true;
		} else {
			vd_layer_vpp[0].property_changed = true;
			vd_layer_vpp[1].property_changed = true;
		}
	}
	return count;
}

static ssize_t pre_hscaler_ntap_enable_show
	(struct class *cla,
	struct class_attribute *attr,
	char *buf)
{
	int i;

	for (i = 0; i < MAX_VD_LAYER; i++) {
		if (pre_scaler[i].pre_hscaler_ntap_set == 0xff)
			pr_info("pre_hscaler_ntap_en[%d](0xff):%d\n",
				i,
				pre_scaler[i].pre_hscaler_ntap_enable);
		else
			pr_info("pre_hscaler_ntap_en[%d]: %d\n",
				i,
				pre_scaler[i].pre_hscaler_ntap_set);
	}
	return 0;
}

static ssize_t pre_hscaler_ntap_enable_store
	(struct class *cla,
	struct class_attribute *attr,
	const char *buf, size_t count)
{
	int parsed[2];
	int layer_id = 0, pre_hscaler_ntap_en = 0;

	if (likely(parse_para(buf, 2, parsed) == 2)) {
		if (parsed[0] < MAX_VD_LAYER)
			layer_id = parsed[0];
		pre_hscaler_ntap_en = parsed[1];
		if (pre_hscaler_ntap_en !=
			pre_scaler[layer_id].pre_hscaler_ntap_set) {
			pre_scaler[layer_id].pre_hscaler_ntap_set =
				pre_hscaler_ntap_en;
			vd_layer[layer_id].property_changed = true;
			if (layer_id >= 1 && vd_layer[layer_id].vpp_index != VPP0) {
				vd_layer_vpp[0].property_changed = true;
				vd_layer_vpp[1].property_changed = true;
			}
		}
	}
	return strnlen(buf, count);
}

static ssize_t pre_hscaler_ntap_set_show
	(struct class *cla,
	struct class_attribute *attr,
	char *buf)
{
	return snprintf(buf, 256, "pre_hscaler_ntap: vd1:%d, vd2:%d, vd3:%d\n",
		pre_scaler[0].pre_hscaler_ntap,
		pre_scaler[1].pre_hscaler_ntap,
		pre_scaler[2].pre_hscaler_ntap);
}

static ssize_t cur_ai_scenes_show(struct class *cla,
				  struct class_attribute *attr, char *buf)
{
	ssize_t count;
	int i = 0;

	if (!vd_layer[0].dispbuf)
		return 0;
	if (!vd_layer[0].dispbuf->ai_pq_enable)
		return 0;

	if (nn_scenes_value[0].maxprob == 0)
		return 0;
	if (vd_layer[0].disable_video == 1 ||
	    vd_layer[0].global_output == 0)
		return 0;
	count = 0;
	while (i < AI_PQ_TOP - 1) {
		count += sprintf(buf + count, "%d:",
			nn_scenes_value[i].maxclass);
		count += sprintf(buf + count, "%d;",
			nn_scenes_value[i].maxprob);
		i++;
	}
	count += sprintf(buf + count, "\n");
	return count;
}

static ssize_t cur_ai_face_show(struct class *cla, struct class_attribute *attr, char *buf)
{
	ssize_t count = 0;
	int i = 0, j = 0;
	int x, y, w, h, score;
	bool need_show_vd0 = true, need_show_vd1 = true;

	if (!vd_layer[0].dispbuf) {
		need_show_vd0 = false;
	} else {
		if (vd_layer[0].disable_video == 1 ||
		    vd_layer[0].global_output == 0) {
			need_show_vd0 = false;
		}

		if (!vd_layer[0].dispbuf->vc_private) {
			need_show_vd0 = false;
		} else {
			if ((vd_layer[0].dispbuf->vc_private->flag & VC_FLAG_AI_FACE) == 0)
				need_show_vd0 = false;
		}
	}

	if (!vd_layer[1].dispbuf) {
		need_show_vd1 = false;
	} else {
		if (vd_layer[1].disable_video == 1 ||
		    vd_layer[1].global_output == 0)
			need_show_vd1 = false;

		if (!vd_layer[1].dispbuf->vc_private) {
			need_show_vd1 = false;
		} else {
			if ((vd_layer[1].dispbuf->vc_private->flag & VC_FLAG_AI_FACE) == 0)
				need_show_vd1 = false;
		}
	}

	if (!need_show_vd0 && !need_show_vd1)
		return 0;

	if (need_show_vd0) {
		while (i < ai_face_value.face_count_vd0) {
			x = ai_face_value.face_value[i].x;
			y = ai_face_value.face_value[i].y;
			w = ai_face_value.face_value[i].w;
			h = ai_face_value.face_value[i].h;
			score = ai_face_value.face_value[i].score;
			count += sprintf(buf + count,
				"omx_index=%d: i=%d: x=%d; y=%d; w=%d; h=%d; score=%d\n",
				vd_layer[0].dispbuf->omx_index, i, x, y, w, h, score);
			i++;
		}
	}

	if (need_show_vd1) {
		i = 0;
		while (i < ai_face_value.face_count_vd1) {
			j = ai_face_value.face_count_vd0 + i;
			x = ai_face_value.face_value[j].x;
			y = ai_face_value.face_value[j].y;
			w = ai_face_value.face_value[j].w;
			h = ai_face_value.face_value[j].h;
			score = ai_face_value.face_value[j].score;
			count += sprintf(buf + count,
				"omx_index=%d: i=%d: x=%d; y=%d; w=%d; h=%d; score=%d\n",
				vd_layer[1].dispbuf->omx_index, j, x, y, w, h, score);
			i++;
		}
	}

	count += sprintf(buf + count, "\n");
	return count;
}

static ssize_t pre_hscaler_ntap_set_store
	(struct class *cla,
	struct class_attribute *attr,
	const char *buf, size_t count)
{
	int parsed[2];
	int layer_id = 0;

	if (likely(parse_para(buf, 2, parsed) == 2)) {
		if (parsed[0] < MAX_VD_LAYER)
			layer_id = parsed[0];
		/* check valid */
		if (has_pre_hscaler_8tap(layer_id)) {
			/* only support 2,4,6,8 tap */
			if (parsed[1] > 8) {
				pr_info("err(%d):max support tap is 8\n", parsed[1]);
				parsed[1] = 8;
			}
			pre_scaler[layer_id].pre_hscaler_ntap = parsed[1];
		} else if (has_pre_hscaler_ntap(layer_id)) {
			/* only support 2,4 tap */
			if (parsed[1] > 4) {
				pr_info("err(%d):max support tap is 4\n", parsed[1]);
				parsed[1] = 4;
			}
			pre_scaler[layer_id].pre_hscaler_ntap = parsed[1];
		}
		vd_layer[layer_id].property_changed = true;
		if (layer_id >= 1 && vd_layer[layer_id].vpp_index != VPP0) {
			vd_layer_vpp[0].property_changed = true;
			vd_layer_vpp[1].property_changed = true;
		}
	}
	return strnlen(buf, count);
}

static ssize_t pre_vscaler_ntap_enable_show
	(struct class *cla,
	struct class_attribute *attr,
	char *buf)
{
	int i;

	for (i = 0; i < MAX_VD_LAYER; i++) {
		if (pre_scaler[i].pre_vscaler_ntap_set == 0xff)
			pr_info("pre_vscaler_ntap_en[%d](0xff):%d\n",
				i,
				pre_scaler[i].pre_vscaler_ntap_enable);
		else
			pr_info("pre_vscaler_ntap_en[%d]: %d\n",
				i,
				pre_scaler[i].pre_vscaler_ntap_set);
	}
	return 0;
}

static ssize_t pre_vscaler_ntap_enable_store
	(struct class *cla,
	struct class_attribute *attr,
	const char *buf, size_t count)
{
	int parsed[2];
	int layer_id = 0, pre_vscaler_ntap_en = 0;

	if (likely(parse_para(buf, 2, parsed) == 2)) {
		if (parsed[0] < MAX_VD_LAYER)
			layer_id = parsed[0];
		pre_vscaler_ntap_en = parsed[1];
		if (pre_vscaler_ntap_en !=
			pre_scaler[layer_id].pre_vscaler_ntap_set) {
			pre_scaler[layer_id].pre_vscaler_ntap_set =
				pre_vscaler_ntap_en;
			vd_layer[layer_id].property_changed = true;
			if (layer_id >= 1 && vd_layer[layer_id].vpp_index != VPP0) {
				vd_layer_vpp[0].property_changed = true;
				vd_layer_vpp[1].property_changed = true;
			}
		}
	}
	return strnlen(buf, count);
}

static ssize_t pre_vscaler_ntap_set_show
	(struct class *cla,
	struct class_attribute *attr,
	char *buf)
{
	return snprintf(buf, 256, "pre_vscaler_ntap: vd1:%d, vd2:%d, vd3:%d\n",
		pre_scaler[0].pre_vscaler_ntap,
		pre_scaler[1].pre_vscaler_ntap,
		pre_scaler[2].pre_vscaler_ntap);
}

static ssize_t pre_vscaler_ntap_set_store
	(struct class *cla,
	struct class_attribute *attr,
	const char *buf, size_t count)
{
	int parsed[2];
	int layer_id = 0;

	if (likely(parse_para(buf, 2, parsed) == 2)) {
		if (parsed[0] < MAX_VD_LAYER)
			layer_id = parsed[0];
		if (has_pre_vscaler_ntap(layer_id)) {
			/* only support 2,4 tap */
			if (parsed[1] > 4) {
				pr_info("err(%d):max support tap is 4\n", parsed[1]);
				parsed[1] = 4;
			}
			pre_scaler[layer_id].pre_vscaler_ntap = parsed[1];
			vd_layer[layer_id].property_changed = true;
			if (layer_id >= 1 && vd_layer[layer_id].vpp_index != VPP0) {
				vd_layer_vpp[0].property_changed = true;
				vd_layer_vpp[1].property_changed = true;
			}
		}
	}
	return strnlen(buf, count);
}

static ssize_t pre_hscaler_rate_show
	(struct class *cla,
	struct class_attribute *attr,
	char *buf)
{
	return snprintf(buf, 256, "pre_hscaler_rate: vd1:%d, vd2:%d, vd3:%d\n",
		pre_scaler[0].pre_hscaler_rate,
		pre_scaler[1].pre_hscaler_rate,
		pre_scaler[2].pre_hscaler_rate);
}

static ssize_t pre_hscaler_rate_store
	(struct class *cla,
	struct class_attribute *attr,
	const char *buf, size_t count)
{
	int parsed[2];
	int layer_id = 0;

	/* 0: no scaler;     1: 2 scaler down */
	/* 2: 4 scaler down; 3: 8 scaler down */
	if (likely(parse_para(buf, 2, parsed) == 2)) {
		if (parsed[0] < MAX_VD_LAYER)
			layer_id = parsed[0];
		if (parsed[1] > 3) {
			pr_info("err, max support max is 3(8 scaler down)\n");
			parsed[1] = 3;
		}
		pre_scaler[layer_id].pre_hscaler_rate = parsed[1];
		vd_layer[layer_id].property_changed = true;
		if (layer_id >= 1 && vd_layer[layer_id].vpp_index != VPP0) {
			vd_layer_vpp[0].property_changed = true;
			vd_layer_vpp[1].property_changed = true;
		}
	}
	return strnlen(buf, count);
}

static ssize_t pre_vscaler_rate_show
	(struct class *cla,
	struct class_attribute *attr,
	char *buf)
{
	return snprintf(buf, 256, "pre_vscaler_rate: vd1:%d, vd2:%d, vd3:%d\n",
		pre_scaler[0].pre_vscaler_rate,
		pre_scaler[1].pre_vscaler_rate,
		pre_scaler[2].pre_vscaler_rate);
}

static ssize_t pre_vscaler_rate_store
	(struct class *cla,
	struct class_attribute *attr,
	const char *buf, size_t count)
{
	int parsed[2];
	int layer_id = 0;

	if (likely(parse_para(buf, 2, parsed) == 2)) {
		if (parsed[0] < MAX_VD_LAYER)
			layer_id = parsed[0];
		if (parsed[1] > 3) {
			pr_info("err, max support max is 3(8 scaler down)\n");
			parsed[1] = 3;
		}
		pre_scaler[layer_id].pre_vscaler_rate = parsed[1];
		vd_layer[layer_id].property_changed = true;
		if (layer_id >= 1 && vd_layer[layer_id].vpp_index != VPP0) {
			vd_layer_vpp[0].property_changed = true;
			vd_layer_vpp[1].property_changed = true;
		}
	}
	return strnlen(buf, count);
}

static ssize_t pre_hscaler_coef_show
	(struct class *cla,
	struct class_attribute *attr,
	char *buf)
{
	return snprintf(buf, 256,
		"pre_hscaler_coef: vd1:%x,%x,%x,%x, vd2:%x,%x,%x,%x, vd3:%x,%x,%x,%x\n",
		pre_scaler[0].pre_hscaler_coef[0],
		pre_scaler[0].pre_hscaler_coef[1],
		pre_scaler[0].pre_hscaler_coef[2],
		pre_scaler[0].pre_hscaler_coef[3],
		pre_scaler[1].pre_hscaler_coef[0],
		pre_scaler[1].pre_hscaler_coef[1],
		pre_scaler[1].pre_hscaler_coef[2],
		pre_scaler[1].pre_hscaler_coef[3],
		pre_scaler[2].pre_hscaler_coef[0],
		pre_scaler[2].pre_hscaler_coef[1],
		pre_scaler[2].pre_hscaler_coef[2],
		pre_scaler[2].pre_hscaler_coef[3]);
}

static ssize_t pre_hscaler_coef_store
	(struct class *cla,
	struct class_attribute *attr,
	const char *buf, size_t count)
{
	int parsed[5];
	int layer_id = 0;
	int i;

	if (likely(parse_para(buf, 5, parsed) == 5)) {
		if (parsed[0] < MAX_VD_LAYER)
			layer_id = parsed[0];
		for (i = 0; i < 4; i++)
			pre_scaler[layer_id].pre_hscaler_coef[i] = parsed[i + 1];
		pre_scaler[layer_id].pre_hscaler_coef_set = 1;
		vd_layer[layer_id].property_changed = true;
		if (layer_id >= 1 && vd_layer[layer_id].vpp_index != VPP0) {
			vd_layer_vpp[0].property_changed = true;
			vd_layer_vpp[1].property_changed = true;
		}
	}
	return strnlen(buf, count);
}

static ssize_t pre_vscaler_coef_show
	(struct class *cla,
	struct class_attribute *attr,
	char *buf)
{
	return snprintf(buf, 256,
		"pre_vscaler_coef: vd1:%x,%x,%x,%x, vd2:%x,%x,%x,%x, vd3:%x,%x,%x,%x\n",
		pre_scaler[0].pre_vscaler_coef[0],
		pre_scaler[0].pre_vscaler_coef[1],
		pre_scaler[0].pre_vscaler_coef[2],
		pre_scaler[0].pre_vscaler_coef[3],
		pre_scaler[1].pre_vscaler_coef[0],
		pre_scaler[1].pre_vscaler_coef[1],
		pre_scaler[1].pre_vscaler_coef[2],
		pre_scaler[1].pre_vscaler_coef[3],
		pre_scaler[2].pre_vscaler_coef[0],
		pre_scaler[2].pre_vscaler_coef[1],
		pre_scaler[2].pre_vscaler_coef[2],
		pre_scaler[2].pre_vscaler_coef[3]);
}

static ssize_t pre_vscaler_coef_store
	(struct class *cla,
	struct class_attribute *attr,
	const char *buf, size_t count)
{
	int parsed[5];
	int layer_id = 0;
	int i;

	if (likely(parse_para(buf, 5, parsed) == 5)) {
		if (parsed[0] < MAX_VD_LAYER)
			layer_id = parsed[0];
		for (i = 0; i < 4; i++)
			pre_scaler[layer_id].pre_vscaler_coef[i] = parsed[i + 1];
		pre_scaler[layer_id].pre_vscaler_coef_set = 1;
		vd_layer[layer_id].property_changed = true;
		if (layer_id >= 1 && vd_layer[layer_id].vpp_index != VPP0) {
			vd_layer_vpp[0].property_changed = true;
			vd_layer_vpp[1].property_changed = true;
		}
	}
	return strnlen(buf, count);
}

static ssize_t force_pre_scaler_show
	(struct class *cla,
	struct class_attribute *attr,
	char *buf)
{
	return snprintf(buf, 256, "force_pre_scaler: vd1:%d, vd2:%d, vd3:%d\n",
		pre_scaler[0].force_pre_scaler,
		pre_scaler[1].force_pre_scaler,
		pre_scaler[2].force_pre_scaler);
}

static ssize_t force_pre_scaler_store
	(struct class *cla,
	struct class_attribute *attr,
	const char *buf, size_t count)
{
	int parsed[2];
	int layer_id = 0;

	if (likely(parse_para(buf, 2, parsed) == 2)) {
		if (parsed[0] < MAX_VD_LAYER)
			layer_id = parsed[0];
		pre_scaler[layer_id].force_pre_scaler = parsed[1];
		vd_layer[layer_id].property_changed = true;
		if (layer_id >= 1 && vd_layer[layer_id].vpp_index != VPP0) {
			vd_layer_vpp[0].property_changed = true;
			vd_layer_vpp[1].property_changed = true;
		}
	}
	return strnlen(buf, count);
}

static ssize_t force_switch_vf_show
	(struct class *cla,
	struct class_attribute *attr,
	char *buf)
{
	return sprintf(buf, "%d\n", force_switch_vf_mode);
}

static ssize_t force_switch_vf_store
	(struct class *cla,
	struct class_attribute *attr,
	const char *buf, size_t count)
{
	int ret;
	int force;

	ret = kstrtoint(buf, 0, &force);
	if (ret < 0)
		return -EINVAL;

	if (force >= 0 && force <= 2 &&
	    force_switch_vf_mode != force) {
		force_switch_vf_mode = force;
		vd_layer[0].property_changed = true;
		vd_layer[1].property_changed = true;
		vd_layer[2].property_changed = true;
		vd_layer_vpp[0].property_changed = true;
		vd_layer_vpp[1].property_changed = true;
	}
	return count;
}

static ssize_t force_property_change_store
	(struct class *cla,
	struct class_attribute *attr,
	const char *buf, size_t count)
{
	int ret;
	int force;

	ret = kstrtoint(buf, 0, &force);
	if (ret < 0)
		return -EINVAL;

	if (force) {
		vd_layer[0].property_changed = true;
		vd_layer[1].property_changed = true;
		vd_layer[2].property_changed = true;
		vd_layer_vpp[0].property_changed = true;
		vd_layer_vpp[1].property_changed = true;
	}
	return count;
}

static ssize_t probe_en_store
	(struct class *cla,
	struct class_attribute *attr,
	const char *buf, size_t count)
{
	int ret;
	int probe_en;

	ret = kstrtoint(buf, 0, &probe_en);
	if (ret < 0)
		return -EINVAL;

	vpp_probe_en_set(probe_en);
	return count;
}

static ssize_t mirror_axis_show(struct class *cla,
				struct class_attribute *attr,
				char *buf)
{
	return snprintf(buf, 40, "%d (1: H_MIRROR 2: V_MIRROR)\n", video_mirror);
}

static ssize_t mirror_axis_store(struct class *cla,
				 struct class_attribute *attr,
				 const char *buf, size_t count)
{
	int res = 0;
	int ret = 0;

	ret = kstrtoint(buf, 0, &res);
	if (ret) {
		pr_err("kstrtoint err\n");
		return -EINVAL;
	}
	pr_info("mirror: %d->%d (1: H_MIRROR 2: V_MIRROR)\n", video_mirror, res);
	video_mirror = res;

	return count;
}

static ssize_t pps_coefs_store(struct class *cla,
				    struct class_attribute *attr,
				    const char *buf, size_t count)
{
	int parsed[3];
	int layer_id = 0, bit9_mode = 0, coef_type = 0;

	if (likely(parse_para(buf, 3, parsed) == 3)) {
		layer_id = parsed[0];
		bit9_mode = parsed[1];
		coef_type = parsed[2];
	}

	dump_pps_coefs_info(layer_id, bit9_mode, coef_type);
	return strnlen(buf, count);
}

static ssize_t load_pps_coefs_store(struct class *cla,
				struct class_attribute *attr,
				 const char *buf, size_t count)
{
	int res = 0;
	int ret = 0;

	ret = kstrtoint(buf, 0, &res);
	if (ret) {
		pr_err("kstrtoint err\n");
		return -EINVAL;
	}
	load_pps_coef = res;
	return count;
}

static ssize_t primary_src_fmt_show(struct class *cla,
		struct class_attribute *attr, char *buf)
{
	int ret = 0;
	enum vframe_signal_fmt_e fmt;

	fmt = (enum vframe_signal_fmt_e)atomic_read(&cur_primary_src_fmt);
	if (fmt != VFRAME_SIGNAL_FMT_INVALID)
		ret += sprintf(buf + ret, "src_fmt = %s\n",
			src_fmt_str[fmt]);
	else
		ret += sprintf(buf + ret, "src_fmt = invalid\n");
	return ret;
}

static ssize_t status_changed_show(struct class *cla,
		struct class_attribute *attr, char *buf)
{
	u32 status = 0;

	status = atomic_read(&status_changed);
	return sprintf(buf, "0x%x\n", status);
}

static ssize_t vd_attach_vpp_show
	(struct class *cla,
	struct class_attribute *attr,
	char *buf)
{
	return snprintf(buf, 256, "vpp_index: vd1:%d, vd2:%d, vd3:%d\n",
		vd_layer[0].vpp_index,
		vd_layer[1].vpp_index,
		vd_layer[2].vpp_index);
}

static ssize_t vd_attach_vpp_store
	(struct class *cla,
	struct class_attribute *attr,
	const char *buf, size_t count)
{
	int parsed[2];
	int layer_id = 0;

	/* parsed[0]: layer_id */
	/* parsed[1]: vpp index */
	if (likely(parse_para(buf, 2, parsed) == 2)) {
		if (parsed[0] < MAX_VD_LAYER)
			layer_id = parsed[0];
		vd_layer[layer_id].vpp_index = parsed[1];
		if (layer_id >= 1) {
			if (parsed[1] == VPP1) {
				vd_layer_vpp[0].vpp_index = VPP1;
				vd_layer_vpp[0].layer_id = layer_id;
			} else if (parsed[1] == VPP2) {
				vd_layer_vpp[1].vpp_index = VPP2;
				vd_layer_vpp[1].layer_id = layer_id;
			}
		}
	}
	return strnlen(buf, count);
}

static ssize_t force_disable_show(struct class *cla,
		struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "force_disable: %d %d %d\n",
		vd_layer[0].force_disable ? 1 : 0,
		vd_layer[1].force_disable ? 1 : 0,
		vd_layer[2].force_disable ? 1 : 0);
}

static ssize_t vd1_vd2_mux_show(struct class *cla,
				struct class_attribute *attr,
				char *buf)
{
	return snprintf(buf, 40, "vd1_vd2_mux:%d(for t5d revb)\n", vd1_vd2_mux);
}

static ssize_t vd1_vd2_mux_store(struct class *cla,
				 struct class_attribute *attr,
				 const char *buf, size_t count)
{
	int res = 0;
	int ret = 0;

	ret = kstrtoint(buf, 0, &res);
	if (ret) {
		pr_err("kstrtoint err\n");
		return -EINVAL;
	}
	vd1_vd2_mux = res;
	vd_layer[0].vd1_vd2_mux = res;
	if (vd1_vd2_mux)
		di_used_vd1_afbc(true);
	else
		di_used_vd1_afbc(false);
	return count;
}

static ssize_t aisr_en_show(struct class *cla,
				struct class_attribute *attr,
				char *buf)
{
	return snprintf(buf, 40, "aisr en:%d\n", aisr_en);
}

static ssize_t aisr_en_store(struct class *cla,
				 struct class_attribute *attr,
				 const char *buf, size_t count)
{
	int res = 0;
	int ret = 0;

	ret = kstrtoint(buf, 0, &res);
	if (ret) {
		pr_err("kstrtoint err\n");
		return -EINVAL;
	}
	if (res != aisr_en) {
		aisr_en = res;
		aisr_sr1_nn_enable_sync(aisr_en);
	}
	return count;
}

static ssize_t reshape_output_store(struct class *cla,
				 struct class_attribute *attr,
				 const char *buf, size_t count)
{
	int res = 0;
	int ret = 0;

	ret = kstrtoint(buf, 0, &res);
	if (ret) {
		pr_err("kstrtoint err\n");
		return -EINVAL;
	}
	aisr_reshape_output(res);
	return count;
}

static ssize_t pps_auto_calc_show(struct class *cla,
				struct class_attribute *attr,
				char *buf)
{
	return snprintf(buf, 40, "aisr pps_auto_calc:%d\n", cur_dev->pps_auto_calc);
}

static ssize_t ai_pq_disable_show(struct class *cla,
				  struct class_attribute *attr, char *buf)
{
	return snprintf(buf, 80, "for_tool:%d\n", ai_pq_disable);
}

static ssize_t ai_pq_disable_store(struct class *cla,
				   struct class_attribute *attr,
				   const char *buf, size_t count)
{
	long tmp;

	int ret = kstrtol(buf, 0, &tmp);

	if (ret != 0) {
		pr_info("ERROR converting %s to long int!\n", buf);
		return ret;
	}
	ai_pq_disable = tmp;
	return count;
}

static ssize_t ai_pq_debug_show(struct class *cla,
				struct class_attribute *attr, char *buf)
{
	return snprintf(buf, 80, "for_tool:%d\n", ai_pq_debug);
}

static ssize_t ai_pq_debug_store(struct class *cla,
				 struct class_attribute *attr,
				 const char *buf, size_t count)
{
	long tmp;

	int ret = kstrtol(buf, 0, &tmp);

	if (ret != 0) {
		pr_info("ERROR converting %s to long int!\n", buf);
		return ret;
	}
	ai_pq_debug = tmp;
	return count;
}

static ssize_t ai_pq_value_show(struct class *cla,
				struct class_attribute *attr, char *buf)
{
	return snprintf(buf, 80, "for_tool:%d\n", ai_pq_value);
}

static ssize_t ai_pq_value_store(struct class *cla,
				 struct class_attribute *attr,
				 const char *buf, size_t count)
{
	long tmp;

	int ret = kstrtol(buf, 0, &tmp);

	if (ret != 0) {
		pr_info("ERROR converting %s to long int!\n", buf);
		return ret;
	}
	ai_pq_value = tmp;
	return count;
}

static ssize_t ai_pq_policy_show(struct class *cla,
				 struct class_attribute *attr, char *buf)
{
	return snprintf(buf, 80, "for_tool:%d\n", ai_pq_policy);
}

static ssize_t ai_pq_policy_store(struct class *cla,
				  struct class_attribute *attr,
				  const char *buf, size_t count)
{
	long tmp;

	int ret = kstrtol(buf, 0, &tmp);

	if (ret != 0) {
		pr_info("ERROR converting %s to long int!\n", buf);
		return ret;
	}
	ai_pq_policy = tmp;
	return count;
}

#if defined(CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_VECM)
static ssize_t det_stb_cnt_show(struct class *cla,
				struct class_attribute *attr, char *buf)
{
	return snprintf(buf, 80, "det_stb_cnt: %d\n", det_stb_cnt);
}

static ssize_t det_stb_cnt_store(struct class *cla,
				 struct class_attribute *attr,
				 const char *buf, size_t count)
{
	long tmp;

	int ret = kstrtol(buf, 0, &tmp);

	if (ret != 0) {
		pr_info("ERROR converting %s to long int!\n", buf);
		return ret;
	}
	det_stb_cnt = tmp;
	return count;
}

static ssize_t det_unstb_cnt_show(struct class *cla,
				  struct class_attribute *attr, char *buf)
{
	return snprintf(buf, 80, "det_unstb_cnt: %d\n", det_unstb_cnt);
}

static ssize_t det_unstb_cnt_store(struct class *cla,
				   struct class_attribute *attr,
				   const char *buf, size_t count)
{
	long tmp;

	int ret = kstrtol(buf, 0, &tmp);

	if (ret != 0) {
		pr_info("ERROR converting %s to long int!\n", buf);
		return ret;
	}
	det_unstb_cnt = tmp;
	return count;
}

static ssize_t tolrnc_cnt_show(struct class *cla,
			       struct class_attribute *attr, char *buf)
{
	return snprintf(buf, 80, "tolrnc_cnt: %d\n", tolrnc_cnt);
}

static ssize_t tolrnc_cnt_store(struct class *cla,
				struct class_attribute *attr,
				const char *buf, size_t count)
{
	long tmp;

	int ret = kstrtol(buf, 0, &tmp);

	if (ret != 0) {
		pr_info("ERROR converting %s to long int!\n", buf);
		return ret;
	}
	tolrnc_cnt = tmp;
	return count;
}

static ssize_t timer_filter_en_show(struct class *cla,
				    struct class_attribute *attr, char *buf)
{
	return snprintf(buf, 80, "timer_filter_en: %d\n", timer_filter_en);
}

static ssize_t timer_filter_en_store(struct class *cla,
				     struct class_attribute *attr,
				     const char *buf, size_t count)
{
	long tmp;

	int ret = kstrtol(buf, 0, &tmp);

	if (ret != 0) {
		pr_info("ERROR converting %s to long int!\n", buf);
		return ret;
	}
	timer_filter_en = tmp;
	return count;
}

static ssize_t aipq_set_policy_show(struct class *cla,
				    struct class_attribute *attr, char *buf)
{
	return snprintf(buf, 80, "aipq_set_policy: %d\n", aipq_set_policy);
}

static ssize_t aipq_set_policy_store(struct class *cla,
				     struct class_attribute *attr,
				     const char *buf, size_t count)
{
	long tmp;

	int ret = kstrtol(buf, 0, &tmp);

	if (ret != 0) {
		pr_info("ERROR converting %s to long int!\n", buf);
		return ret;
	}
	aipq_set_policy = tmp;
	return count;
}

static ssize_t color_th_show(struct class *cla,
			     struct class_attribute *attr, char *buf)
{
	return snprintf(buf, 80, "color_th: %d\n", color_th);
}

static ssize_t color_th_store(struct class *cla,
			      struct class_attribute *attr,
			      const char *buf, size_t count)
{
	long tmp;

	int ret = kstrtol(buf, 0, &tmp);

	if (ret != 0) {
		pr_info("ERROR converting %s to long int!\n", buf);
		return ret;
	}
	color_th = tmp;
	return count;
}

static ssize_t aisr_demo_en_show(struct class *cla,
			     struct class_attribute *attr, char *buf)
{
	return snprintf(buf, 80, "aisr_demo_en: %d\n", cur_dev->aisr_demo_en);
}

static ssize_t aisr_demo_en_store(struct class *cla,
			      struct class_attribute *attr,
			      const char *buf, size_t count)
{
	int ret;
	int res;

	ret = kstrtoint(buf, 0, &res);
	if (ret) {
		pr_err("kstrtoint err\n");
		return -EINVAL;
	}
	if (res != cur_dev->aisr_demo_en) {
		cur_dev->aisr_demo_en = res;
		vd_layer[0].property_changed = true;
	}
	return count;
}

static ssize_t aisr_demo_axis_show(struct class *cla,
			     struct class_attribute *attr, char *buf)
{
	return snprintf(buf, 80, "aisr_demo_axis: %d,%d,%d,%d\n",
		cur_dev->aisr_demo_xstart,
		cur_dev->aisr_demo_ystart,
		cur_dev->aisr_demo_xend,
		cur_dev->aisr_demo_yend);
}

static ssize_t aisr_demo_axis_store(struct class *cla,
			      struct class_attribute *attr,
			      const char *buf, size_t count)
{
	int parsed[4];

	if (likely(parse_para(buf, 4, parsed) == 4)) {
		if (parsed[0] != cur_dev->aisr_demo_xstart ||
			parsed[1] != cur_dev->aisr_demo_ystart ||
			parsed[2] != cur_dev->aisr_demo_xend ||
			parsed[3] != cur_dev->aisr_demo_yend) {
			cur_dev->aisr_demo_xstart = parsed[0];
			cur_dev->aisr_demo_ystart = parsed[1];
			cur_dev->aisr_demo_xend = parsed[2];
			cur_dev->aisr_demo_yend = parsed[3];
			vd_layer[0].property_changed = true;
		}
	}
	return count;
}

static ssize_t power_ctrl_show(struct class *cla,
			     struct class_attribute *attr, char *buf)
{
	return snprintf(buf, 80, "power_ctrl: %d\n", cur_dev->power_ctrl);
}

static ssize_t power_ctrl_store(struct class *cla,
			      struct class_attribute *attr,
			      const char *buf, size_t count)
{
	int ret;
	int res;

	ret = kstrtoint(buf, 0, &res);
	if (ret) {
		pr_err("kstrtoint err\n");
		return -EINVAL;
	}
	if (res) {
		pr_info("enable power_ctrl, disable vd scaler/SR clk\n");
		cur_dev->power_ctrl = res;
		vpu_module_clk_disable(VPP0, VD1_SCALER, 1);
		vpu_module_clk_disable(VPP0, VD2_SCALER, 1);
		vpu_module_clk_disable(VPP0, SR0, 1);
		vpu_module_clk_disable(VPP0, SR1, 1);
		vpu_module_clk_disable(VPP0, VD1_HDR_CORE, 1);
		vpu_module_clk_disable(VPP0, VD2_HDR_CORE, 1);
		vpu_module_clk_disable(VPP0, OSD1_HDR_CORE, 1);
		vpu_module_clk_disable(VPP0, OSD2_HDR_CORE, 1);
		vpu_module_clk_disable(VPP0, DV_TVCORE, 1);
	} else {
		pr_info("disable power_ctrl, enable vd scaler/SR clk\n");
		vpu_module_clk_enable(VPP0, VD1_SCALER, 1);
		vpu_module_clk_enable(VPP0, VD2_SCALER, 1);
		vpu_module_clk_enable(VPP0, SR0, 1);
		vpu_module_clk_enable(VPP0, SR1, 1);
		vpu_module_clk_enable(VPP0, VD1_HDR_CORE, 1);
		vpu_module_clk_enable(VPP0, VD2_HDR_CORE, 1);
		vpu_module_clk_enable(VPP0, OSD1_HDR_CORE, 1);
		vpu_module_clk_enable(VPP0, OSD2_HDR_CORE, 1);
		vpu_module_clk_enable(VPP0, DV_TVCORE, 1);
		cur_dev->power_ctrl = res;
	}
	return count;
}
#endif

#ifdef CONFIG_AMLOGIC_MEDIA_FRC
static ssize_t frc_delay_show(struct class *class,
				      struct class_attribute *attr,
				      char *buf)
{
	u32 frc_delay = 0;

	frc_delay += frc_get_video_latency();
	return sprintf(buf, "%d\n", frc_delay);
}
#endif

static ssize_t vpu_module_urgent_show(struct class *cla,
			     struct class_attribute *attr, char *buf)
{
#ifndef CONFIG_AMLOGIC_C3_REMOVE
	if (video_is_meson_t3_cpu()) {
		get_vpu_urgent_info_t3();
	} else if (video_is_meson_t7_cpu()) {
		get_vpu_urgent_info_t5m();
		pr_info("t7 prot mapping:\n");
		pr_info("vpp_arb0: osd1, vd1, osd3, dolby0, vd3\n");
		pr_info("vpp_arb1: osd2, vd2, osd4, mali-afbc\n");
	} else if (video_is_meson_s5_cpu()) {
		get_vpu_urgent_info_s5();
		pr_info("s5 port mapping:\n");
		pr_info("vpp_arb0:  osd1, osd2, osd3, osd4, mali-afbc\n");
		pr_info("vpp_arb1:  vd1 slice0-slice1, vd2\n");
		pr_info("vpp_arb2:  vd1 slice2-slice3 aisr\n");
	} else if (video_is_meson_t5m_cpu()) {
		get_vpu_urgent_info_t5m();
		pr_info("t5m prot mapping:\n");
		pr_info("vpp_arb0: vd1, vd2, dolby0\n");
		pr_info("vpp_arb1: osd1, osd2, osd3, mali-afbc\n");
	}
#endif
	return 0;
}

#ifndef CONFIG_AMLOGIC_C3_REMOVE
static const char vpu_module_urgent_help_t3[] = "Usage:\n"
"  echo module_id low_level(0-7) high_level(0-7)> /sys/class/video/urgent_set\n"
"  FRC0_R: 0\n"
"  FRC0_W: 1\n"
"  FRC1_R: 2\n"
"  FRC1_W: 3\n"
"  FRC2_R: 4\n"
"  VPU0_R: 5\n"
"  VPU0_W: 6\n"
"  VPU1_R: 7\n"
"  VPU1_W: 8\n"
"  VPU2_R: 9\n\n";

static const char vpu_module_urgent_help_t7[] = "Usage:\n"
"  echo module_id urgent_level(0-3) > /sys/class/video/urgent_set\n"
"  VPP_ARB0: 0\n"
"  VPP_ARB1: 1\n"
"  RDMA READ: 2\n"
"  LDIM: 7\n"
"  VDIN_AFBCE: 8\n"
"  VPU DMA: 9\n\n";

static const char vpu_module_urgent_help_s5[] = "Usage:\n"
"  echo module_id urgent_level(0-3) > /sys/class/video/urgent_set\n"
"  VPP_ARB0: 0\n"
"  VPP_ARB1: 1\n"
"  VPP_ARB2: 2\n"
"  VPU_SUB_READ: 3\n"
"  DCNTR_GRID: 4\n"
"  TCON_P1: 5\n"
"  TCON_P2: 6\n"
"  TCON_P3: 7\n\n";

static const char vpu_module_urgent_help_t5m[] = "Usage:\n"
"  echo module_id urgent_level(0-3) > /sys/class/video/urgent_set\n"
"  VPP_ARB0: 0\n"
"  VPP_ARB1: 1\n"
"  RDMA READ: 2\n"
"  VPU_SUB_READ: 3\n"
"  TCON_P1: 5\n"
"  DCNTR_GRID: 4\n"
"  TCON_P2: 6\n\n";
#endif

static ssize_t vpu_module_urgent_set(struct class *class,
			struct class_attribute *attr,
			const char *buf, size_t count)
{
#ifndef CONFIG_AMLOGIC_C3_REMOVE
	int parsed[3];
	int ret = -1;

	if (video_is_meson_t3_cpu()) {
		if (likely(parse_para(buf, 3, parsed) == 3))
			ret = set_vpu_super_urgent_t3(parsed[0], parsed[1], parsed[2]);
	} else if (video_is_meson_t7_cpu()) {
		if (likely(parse_para(buf, 2, parsed) == 2))
			ret = set_vpu_super_urgent_t7(parsed[0], parsed[1]);
	} else if (video_is_meson_s5_cpu()) {
		if (likely(parse_para(buf, 2, parsed) == 2))
			ret = set_vpu_super_urgent_s5(parsed[0], parsed[1]);
	} else if (video_is_meson_t5m_cpu()) {
		if (likely(parse_para(buf, 2, parsed) == 2))
			ret = set_vpu_super_urgent_t5m(parsed[0], parsed[1]);
	}
	if (ret < 0) {
		if (video_is_meson_t3_cpu())
			pr_info("%s", vpu_module_urgent_help_t3);
		else if (video_is_meson_t7_cpu())
			pr_info("%s", vpu_module_urgent_help_t7);
		else if (video_is_meson_s5_cpu())
			pr_info("%s", vpu_module_urgent_help_s5);
		else if (video_is_meson_t5m_cpu())
			pr_info("%s", vpu_module_urgent_help_t5m);
	}
#endif
	return count;
}

static ssize_t video_threshold_8k_show(struct class *cla,
			     struct class_attribute *attr, char *buf)
{
	return snprintf(buf, 80, "vdec_out_size_threshold_8k:%d,vpp_in_size_threshold_8k:%d\n",
		vdec_out_size_threshold_8k,
		vpp_in_size_threshold_8k);
}

static ssize_t video_threshold_8k_store(struct class *cla,
			      struct class_attribute *attr,
			      const char *buf, size_t count)
{
	int parsed[2];

	if (likely(parse_para(buf, 2, parsed) == 2)) {
		vdec_out_size_threshold_8k = parsed[0];
		vpp_in_size_threshold_8k = parsed[1];
	}
	return count;
}

static ssize_t video_threshold_4k_show(struct class *cla,
			     struct class_attribute *attr, char *buf)
{
	return snprintf(buf, 80, "vdec_out_size_threshold_4k:%d,vpp_in_size_threshold_4k:%d\n",
		vdec_out_size_threshold_4k,
		vpp_in_size_threshold_4k);
}

static ssize_t video_threshold_4k_store(struct class *cla,
			      struct class_attribute *attr,
			      const char *buf, size_t count)
{
	int parsed[2];

	if (likely(parse_para(buf, 2, parsed) == 2)) {
		vdec_out_size_threshold_4k = parsed[0];
		vpp_in_size_threshold_4k = parsed[1];
	}
	return count;
}

static ssize_t video_test_pattern_show(struct class *cla,
			     struct class_attribute *attr, char *buf)
{
	bool vdx_test_pattern_on[MAX_VD_LAYER];
	int vdx_color[MAX_VD_LAYER];

	get_vdx_test_pattern(0, &vdx_test_pattern_on[0], &vdx_color[0]);
	get_vdx_test_pattern(1, &vdx_test_pattern_on[1], &vdx_color[1]);
	get_vdx_test_pattern(2, &vdx_test_pattern_on[2], &vdx_color[2]);
	return snprintf(buf, 80, "vdx_test_pattern_on:%d,%d,%d,vd color:0x%x,0x%x,0x%x\n",
		vdx_test_pattern_on[0],
		vdx_test_pattern_on[1],
		vdx_test_pattern_on[2],
		vdx_color[0],
		vdx_color[1],
		vdx_color[2]);
}

static ssize_t video_test_pattern_store(struct class *cla,
			      struct class_attribute *attr,
			      const char *buf, size_t count)
{
	int parsed[3];
	u32 index;

	if (likely(parse_para(buf, 3, parsed) == 3)) {
		if (parsed[0] < MAX_VD_LAYER) {
			index = parsed[0];
			set_vdx_test_pattern(index, parsed[1], parsed[2]);
		}
	}
	return count;
}

static ssize_t postblend_test_pattern_show(struct class *cla,
			     struct class_attribute *attr, char *buf)
{
	bool postblend_test_pattern_on;
	u32 postblend_color;

	get_postblend_test_pattern(&postblend_test_pattern_on,
		&postblend_color);
	return snprintf(buf, 80, "postblend_test_pattern_on:%d, color:0x%x\n",
		postblend_test_pattern_on,
		postblend_color);
}

static ssize_t postblend_test_pattern_store(struct class *cla,
			      struct class_attribute *attr,
			      const char *buf, size_t count)
{
	int parsed[2];

	if (likely(parse_para(buf, 2, parsed) == 2))
		set_postblend_test_pattern(parsed[0], parsed[1]);
	return count;
}

static ssize_t tvin_source_type_show(struct class *cla,
			     struct class_attribute *attr, char *buf)
{
	return snprintf(buf, 80, "tvin_source_type:%d\n", tvin_source_type);
}

static ssize_t tvin_source_type_store(struct class *cla,
			      struct class_attribute *attr,
			      const char *buf, size_t count)
{
	long tmp;

	int ret = kstrtol(buf, 0, &tmp);

	if (ret != 0) {
		pr_info("ERROR converting %s to long int!\n", buf);
		return ret;
	}
	tvin_source_type = tmp;
	pr_info("store tvin_source_type=%d\n", tvin_source_type);
	return count;
}

static ssize_t bypass_module_s5_show(struct class *cla,
			     struct class_attribute *attr, char *buf)
{
	return snprintf(buf, 80, "bypass module:0x%x(bit0: dv, bit1: hdr, bit2: detunnel)\n",
		get_module_bypass_s5());
}

static ssize_t bypass_module_s5_store(struct class *cla,
			      struct class_attribute *attr,
			      const char *buf, size_t count)
{
	int ret, res;

	ret = kstrtoint(buf, 0, &res);
	if (ret) {
		pr_err("kstrtoint err\n");
		return -EINVAL;
	}

	set_module_bypass_s5(res);
	return count;
}

static ssize_t set_post_matrix_show(struct class *cla,
					   struct class_attribute *attr,
					   char *buf)
{
	if (cur_dev->display_module == S5_DISPLAY_MODULE) {
		pr_info("Usage:\n");
		pr_info("echo port > /sys/class/amvecm/matrix_set\n");
		pr_info("1 : VD1_PROBE\n");
		pr_info("2 : VD2_PROBE\n");
		pr_info("3 : VD3_PROBE\n");
		pr_info("4 : OSD1_PROBE\n");
		pr_info("5 : OSD2_PROBE\n");
		pr_info("6 : OSD3_PROBE\n");
		pr_info("7 : OSD4_PROBE\n");
		pr_info("8 : POST_VADJ_PROBE\n");
		pr_info("9 : POSTBLEND_PROBE\n");
		pr_info("BIT4: 0 INPUT\n");
		pr_info("BIT4: 1 OUTPUT\n");
		pr_info("current setting: %d\n", probe_id | probe_output);
	}

	return 0;
}

static ssize_t set_post_matrix_store(struct class *cla,
					    struct class_attribute *attr,
					    const char *buf, size_t count)
{
	int val = 0;
	u32 output = false;

	if (kstrtoint(buf, 0, &val) < 0)
		return -EINVAL;
	output = val & 0x10;
	val = val & 0xf;
	if (cur_dev->display_module == S5_DISPLAY_MODULE) {
		probe_id = val;
		probe_output = output;
		switch (val) {
		case OSD1_PROBE:
		case OSD2_PROBE:
		case OSD3_PROBE:
		case OSD4_PROBE:
			set_osdx_probe_ctrl_s5(val, output);
			break;
		case VD1_PROBE:
		case VD2_PROBE:
		case VD3_PROBE:
		case POST_VADJ_PROBE:
		case POSTBLEND_PROBE:
			break;
		default:
			pr_info("error! please cat /sys/class/video/matrix_set\n");
			return 0;
		}
		pr_info("VPP_MATRIX_CTRL is set\n");
	}
	return strnlen(buf, count);
}

static ssize_t post_matrix_pos_show(struct class *cla,
					   struct class_attribute *attr,
					   char *buf)
{
	int val = 0;

	pr_info("Usage:\n");
	pr_info("echo x y > /sys/class/video/matrix_pos\n");

	if (cur_dev->display_module == S5_DISPLAY_MODULE)
		val = get_probe_pos_s5(probe_id);
	pr_info("current position: %d %d\n",
		(val >> 16) & 0x1fff,
			(val >> 0) & 0x1fff);
	return 0;
}

static ssize_t post_matrix_pos_store(struct class *cla,
					    struct class_attribute *attr,
					    const char *buf, size_t count)
{
	int val_x, val_y;
	int parsed[2];

	if (likely(parse_para(buf, 2, parsed) == 2)) {
		val_x = parsed[0] & 0x1fff;
		val_y = parsed[1] & 0x1fff;
		if (cur_dev->display_module == S5_DISPLAY_MODULE)
			set_probe_pos_s5(val_x, val_y, probe_id, probe_output);
	}
	return strnlen(buf, count);
}

static ssize_t post_matrix_data_show(struct class *cla,
					    struct class_attribute *attr,
					    char *buf)
{
	int len = 0, val1 = 0, val2 = 0;
	u8 bit_depth = 10;

	if (cur_dev->display_module == S5_DISPLAY_MODULE) {
		if (video_is_meson_s5_cpu())
			bit_depth = 12;
		get_probe_data_s5(&val1, &val2, probe_id);
		if (bit_depth == 10)
			len += sprintf(buf + len,
			"VPP_MATRIX_PROBE_COLOR %d, %d, %d\n",
			(val1 >> 20) & 0x3ff,
			(val1 >> 10) & 0x3ff,
			(val1 >> 0) & 0x3ff);
		else
			len += sprintf(buf + len,
			"VPP_MATRIX_PROBE_COLOR %d, %d, %d\n",
			((val2 & 0xf) << 8) | ((val1 >> 24) & 0xff),
			(val1 >> 12) & 0xfff, val1 & 0xfff);
	}
	return len;
}

static ssize_t post_matrix_data_store(struct class *cla,
					     struct class_attribute *attr,
					     const char *buf, size_t count)
{
	return 0;
}

static ssize_t pre_vsync_enable_show(struct class *cla,
				struct class_attribute *attr,
				char *buf)
{
	return snprintf(buf, 40, "pre_vsync_enable:%d\n", cur_dev->pre_vsync_enable);
}

static ssize_t pre_vsync_enable_store(struct class *cla,
				 struct class_attribute *attr,
				 const char *buf, size_t count)
{
	int res = 0;
	int ret = 0;

	ret = kstrtoint(buf, 0, &res);
	if (ret) {
		pr_err("kstrtoint err\n");
		return -EINVAL;
	}
	set_pre_vsync_mode(res);
	return count;
}

static ssize_t mosaic_axis_pic_show(struct class *cla,
		struct class_attribute *attr,
		char *buf)
{
	if (cur_dev->display_module == S5_DISPLAY_MODULE)
		get_mosaic_axis();
	return 0;
}

static ssize_t mosaic_axis_pic_store(struct class *cla,
		struct class_attribute *attr,
		const char *buf, size_t count)
{
	if (cur_dev->display_module == S5_DISPLAY_MODULE) {
		int parsed[5];
		int pic_index  = 0;

		if (likely(parse_para(buf, 5, parsed) == 5)) {
			pic_index = parsed[0];
			set_mosaic_axis(pic_index, parsed[1], parsed[2],
				parsed[3], parsed[4]);
		} else {
			pr_info("%s: err\n", __func__);
		}
	}
	return strnlen(buf, count);
}

static ssize_t aisr_info_show(struct class *cla,
		struct class_attribute *attr, char *buf)
{
	ssize_t len = 0;
	struct vpp_frame_par_s *_cur_frame_par = NULL;
	struct aisr_setting_s *aisr_mif_setting = &vd_layer[0].aisr_mif_setting;

	_cur_frame_par = &cur_dev->aisr_frame_parms;
	if (!_cur_frame_par)
		return len;
	if (!aisr_mif_setting->aisr_enable || !vd_layer[0].dispbuf)
		return sprintf(buf, "aisr is not enable\n");
	else
		return sprintf(buf, "input %d*%d;output %d*%d;\n",
			aisr_mif_setting->src_w,
			aisr_mif_setting->src_h,
			_cur_frame_par->nnhf_input_w,
			_cur_frame_par->nnhf_input_h);
}

static ssize_t vsync_2to1_enable_show(struct class *cla,
				struct class_attribute *attr,
				char *buf)
{
	return snprintf(buf, 40, "vsync_2to1_enable:%d\n", cur_dev->vsync_2to1_enable);
}

static ssize_t vsync_2to1_enable_store(struct class *cla,
				 struct class_attribute *attr,
				 const char *buf, size_t count)
{
	int res = 0;
	int ret = 0;

	ret = kstrtoint(buf, 0, &res);
	if (ret) {
		pr_err("kstrtoint err\n");
		return -EINVAL;
	}
	if (cur_dev->prevsync_support)
		cur_dev->vsync_2to1_enable = res;
	return count;
}

static ssize_t vpp_in_padding_enable_show(struct class *cla,
				struct class_attribute *attr,
				char *buf)
{
	int padding_enable, h_padding, v_padding;

	get_vpp_in_padding_axis(&padding_enable, &h_padding, &v_padding);
	return snprintf(buf, 80, "vpp_in_padding_enable:%d, h/v padding: %d, %d\n",
		padding_enable,
		h_padding, v_padding);
}

static ssize_t vpp_in_padding_enable_store(struct class *cla,
				 struct class_attribute *attr,
				 const char *buf, size_t count)
{
	int parsed[3];

	if (likely(parse_para(buf, 3, parsed) == 3)) {
		if (cur_dev->display_module == S5_DISPLAY_MODULE &&
			cur_dev->vpp_in_padding_support)
			set_vpp_in_padding_axis(parsed[0], parsed[1], parsed[2]);
		else
			pr_err("%s: padding not support\n", __func__);
	} else {
		pr_err("%s: err\n", __func__);
	}
	return strnlen(buf, count);
}

static ssize_t vpu_venc_status_show(struct class *cla, struct class_attribute *attr, char *buf)
{
	u32 status;

	/* 0 is normal */
	status = get_vpu_venc_error_status();
	return snprintf(buf, 40, "vpu_venc_status:%x\n", status);
}

static ssize_t vpu_venc_status_store(struct class *cla,
				 struct class_attribute *attr,
				 const char *buf, size_t count)
{
	int res = 0;
	int ret = 0;

	ret = kstrtoint(buf, 0, &res);
	if (ret) {
		pr_err("kstrtoint err\n");
		return -EINVAL;
	}
	if (res)
		clear_vpu_venc_error();
	return count;
}

static struct class_attribute amvideo_class_attrs[] = {
	__ATTR(axis,
	       0664,
	       video_axis_show,
	       video_axis_store),
	__ATTR(real_axis,
	       0664,
	       real_axis_show,
	       NULL),
	__ATTR(crop,
	       0644,
	       video_crop_show,
	       video_crop_store),
	__ATTR(sr,
	       0644,
	       video_sr_show,
	       video_sr_store),
	__ATTR(global_offset,
	       0644,
	       video_global_offset_show,
	       video_global_offset_store),
	__ATTR(screen_mode,
	       0664,
	       video_screen_mode_show,
	       video_screen_mode_store),
	__ATTR(blackout_policy,
	       0664,
	       video_blackout_policy_show,
	       video_blackout_policy_store),
	__ATTR(blackout_pip_policy,
	       0664,
	       videopip_blackout_policy_show,
	       videopip_blackout_policy_store),
	__ATTR(blackout_pip2_policy,
	       0664,
	       videopip2_blackout_policy_show,
	       videopip2_blackout_policy_store),
	__ATTR(video_seek_flag,
	       0664,
	       video_seek_flag_show,
	       video_seek_flag_store),
	__ATTR(video_mute,
	       0664,
	       video_mute_show,
	       video_mute_store),
	__ATTR(disable_video,
	       0664,
	       video_disable_show,
	       video_disable_store),
	__ATTR(video_global_output,
	       0664,
	       video_global_output_show,
	       video_global_output_store),
	__ATTR(hold_video,
	       0664,
	       video_hold_show,
	       video_hold_store),
	__ATTR(zoom,
	       0664,
	       video_zoom_show,
	       video_zoom_store),
	__ATTR(brightness,
	       0644,
	       video_brightness_show,
	       video_brightness_store),
	__ATTR(contrast,
	       0644,
	       video_contrast_show,
	       video_contrast_store),
	__ATTR(vpp_brightness,
	       0644,
	       vpp_brightness_show,
	       vpp_brightness_store),
	__ATTR(vpp_contrast,
	       0644,
	       vpp_contrast_show,
	       vpp_contrast_store),
	__ATTR(saturation,
	       0644,
	       video_saturation_show,
	       video_saturation_store),
	__ATTR(vpp_saturation_hue,
	       0644,
	       vpp_saturation_hue_show,
	       vpp_saturation_hue_store),
	__ATTR(video_background,
	       0644,
	       video_background_show,
	       video_background_store),
	__ATTR(test_screen,
	       0644,
	       video_test_screen_show,
	       video_test_screen_store),
	__ATTR(rgb_screen,
	       0644,
	       video_rgb_screen_show,
	       video_rgb_screen_store),
	__ATTR(file_name,
	       0644,
	       video_filename_show,
	       video_filename_store),
	__ATTR(debugflags,
	       0644,
	       video_debugflags_show,
	       video_debugflags_store),
	__ATTR(trickmode_duration,
	       0644,
	       trickmode_duration_show,
	       trickmode_duration_store),
	__ATTR(nonlinear_factor,
	       0644,
	       video_nonlinear_factor_show,
	       video_nonlinear_factor_store),
	__ATTR(nonlinear_t_factor,
	       0644,
	       video_nonlinear_t_factor_show,
	       video_nonlinear_t_factor_store),
	__ATTR(freerun_mode,
	       0644,
	       video_freerun_mode_show,
	       video_freerun_mode_store),
	__ATTR(video_speed_check_h_w,
	       0644,
	       video_speed_check_show,
	       video_speed_check_store),
	__ATTR(threedim_mode,
	       0644,
	       threedim_mode_show,
	       threedim_mode_store),
	__ATTR(vsync_pts_inc_upint,
	       0644,
	       video_vsync_pts_inc_upint_show,
	       video_vsync_pts_inc_upint_store),
	__ATTR(vsync_slow_factor,
	       0644,
	       video_vsync_slow_factor_show,
	       video_vsync_slow_factor_store),
	__ATTR(angle,
	       0644,
	       video_angle_show,
	       video_angle_store),
	__ATTR(stereo_scaler,
	       0644, NULL,
	       video_3d_scale_store),
	__ATTR(show_first_frame_nosync,
	       0644,
	       show_first_frame_nosync_show,
	       show_first_frame_nosync_store),
	__ATTR(show_first_picture,
	       0664, NULL,
	       show_first_picture_store),
	__ATTR(slowsync_repeat_enable,
	       0644,
	       slowsync_repeat_enable_show,
	       slowsync_repeat_enable_store),
	__ATTR(free_keep_buffer,
	       0664, NULL,
	       video_free_keep_buffer_store),
	__ATTR(hdmin_delay_start,
	       0664,
	       hdmin_delay_start_show,
	       hdmin_delay_start_store),
	__ATTR(hdmin_delay_duration,
	       0664,
	       hdmin_delay_duration_show,
	       hdmin_delay_duration_store),
	__ATTR(hdmin_delay_min_ms,
	       0664,
	       hdmin_delay_min_ms_show,
	       hdmin_delay_min_ms_store),
	__ATTR(hdmin_delay_max_ms,
	       0664,
	       hdmin_delay_max_ms_show,
	       hdmin_delay_max_ms_store),
	__ATTR(vframe_walk_delay,
	       0664,
	       vframe_walk_delay_show, NULL),
	__ATTR(last_required_total_delay,
	       0664,
	       last_required_total_delay_show, NULL),
	__ATTR(free_cma_buffer,
	       0664, NULL,
	       free_cma_buffer_store),
#ifdef CONFIG_AM_VOUT
	__ATTR_RO(device_resolution),
#endif
#ifdef PTS_TRACE_DEBUG
	__ATTR_RO(pts_trace),
#endif
	__ATTR(video_inuse,
	       0664,
	       video_inuse_show,
	       video_inuse_store),
	__ATTR(video_zorder,
	       0664,
	       video_zorder_show,
	       video_zorder_store),
	__ATTR(black_threshold,
	       0664,
	       black_threshold_show,
	       black_threshold_store),
	 __ATTR(get_di_count,
		0664,
		get_di_count_show,
		get_di_count_store),
	 __ATTR(put_di_count,
		0664,
		put_di_count_show,
		put_di_count_store),
	 __ATTR(di_release_count,
		0664,
		di_release_count_show,
		di_release_count_store),
	 __ATTR(hist_test,
	       0664,
	       hist_test_show,
	       hist_test_store),
	__ATTR(limited_win_ratio,
		  0664,
		  limited_win_ratio_show,
		  NULL),
	__ATTR_RO(frame_addr),
	__ATTR_RO(frame_canvas_width),
	__ATTR_RO(frame_canvas_height),
	__ATTR_RO(frame_width),
	__ATTR_RO(frame_height),
	__ATTR_RO(frame_format),
	__ATTR_RO(frame_original_format),
	__ATTR_RO(frame_aspect_ratio),
	__ATTR_RO(frame_rate),
	__ATTR_RO(vframe_states),
	__ATTR_RO(video_state),
	__ATTR_RO(fps_info),
	__ATTR_RO(vframe_ready_cnt),
	__ATTR_RO(video_layer1_state),
	__ATTR_RO(pic_mode_info),
	__ATTR_RO(src_fmt),
	__ATTR_RO(cur_aipq_sp),
	__ATTR_RO(process_fmt),
	__ATTR(axis_pip,
	       0664,
	       videopip_axis_show,
	       videopip_axis_store),
	__ATTR(crop_pip,
	       0664,
	       videopip_crop_show,
	       videopip_crop_store),
	__ATTR(disable_videopip,
	       0664,
	       videopip_disable_show,
	       videopip_disable_store),
	__ATTR(screen_mode_pip,
	       0664,
	       videopip_screen_mode_show,
	       videopip_screen_mode_store),
	__ATTR(videopip_loop,
	       0664,
	       videopip_loop_show,
	       videopip_loop_store),
	__ATTR(pip_global_output,
	       0664,
	       videopip_global_output_show,
	       videopip_global_output_store),
	__ATTR(videopip_zorder,
	       0664,
	       videopip_zorder_show,
	       videopip_zorder_store),
	__ATTR_RO(videopip_state),
	__ATTR(axis_pip2,
	       0664,
	       videopip2_axis_show,
	       videopip2_axis_store),
	__ATTR(crop_pip2,
	       0664,
	       videopip2_crop_show,
	       videopip2_crop_store),
	__ATTR(disable_videopip2,
	       0664,
	       videopip2_disable_show,
	       videopip2_disable_store),
	__ATTR(screen_mode_pip2,
	       0664,
	       videopip2_screen_mode_show,
	       videopip2_screen_mode_store),
	__ATTR(videopip2_loop,
	       0664,
	       videopip2_loop_show,
	       videopip2_loop_store),
	__ATTR(pip2_global_output,
	       0664,
	       videopip2_global_output_show,
	       videopip2_global_output_store),
	__ATTR(videopip2_zorder,
	       0664,
	       videopip2_zorder_show,
	       videopip2_zorder_store),
	__ATTR_RO(videopip2_state),
	__ATTR(path_select,
	       0664,
	       path_select_show,
	       path_select_store),
	__ATTR(vpp_crc,
	       0664,
	       vpp_crc_show,
	       vpp_crc_store),
	__ATTR(vpp_crc_viu2,
	       0664,
	       vpp_crc_viu2_show,
	       vpp_crc_viu2_store),
	__ATTR(pip_alpha,
	       0220,
	       NULL,
	       pip_alpha_store),
	__ATTR(film_grain,
	       0664,
	       film_grain_show,
	       film_grain_store),
	__ATTR_RO(film_grain_support),
	__ATTR(pq_default,
	       0664,
	       pq_default_show,
	       pq_default_store),
	__ATTR(hscaler_8tap_en,
	       0664,
	       hscaler_8tap_enable_show,
	       hscaler_8tap_enable_store),
	__ATTR(pip_hscaler_8tap_en,
	       0664,
	       pip_hscaler_8tap_enable_show,
	       pip_hscaler_8tap_enable_store),
	__ATTR(pip2_hscaler_8tap_en,
	       0664,
	       pip2_hscaler_8tap_enable_show,
	       pip2_hscaler_8tap_enable_store),
	__ATTR(pre_hscaler_ntap_en,
	       0664,
	       pre_hscaler_ntap_enable_show,
	       pre_hscaler_ntap_enable_store),
	__ATTR(pre_hscaler_ntap,
	       0664,
	       pre_hscaler_ntap_set_show,
	       pre_hscaler_ntap_set_store),
	__ATTR(pre_vscaler_ntap_en,
	       0664,
	       pre_vscaler_ntap_enable_show,
	       pre_vscaler_ntap_enable_store),
	__ATTR_WO(pq_data),
	__ATTR_RO(cur_ai_scenes),
	__ATTR(pre_vscaler_ntap,
	       0664,
	       pre_vscaler_ntap_set_show,
	       pre_vscaler_ntap_set_store),
	__ATTR(pre_hscaler_rate,
	       0664,
	       pre_hscaler_rate_show,
	       pre_hscaler_rate_store),
	__ATTR(pre_vscaler_rate,
	       0664,
	       pre_vscaler_rate_show,
	       pre_vscaler_rate_store),
	__ATTR(pre_hscaler_coef,
	       0664,
	       pre_hscaler_coef_show,
	       pre_hscaler_coef_store),
	__ATTR(pre_vscaler_coef,
	       0664,
	       pre_vscaler_coef_show,
	       pre_vscaler_coef_store),
	__ATTR(force_pre_scaler,
	       0664,
	       force_pre_scaler_show,
	       force_pre_scaler_store),
	__ATTR(force_switch_vf,
	       0644,
	       force_switch_vf_show,
	       force_switch_vf_store),
	__ATTR(force_property_change,
	       0644, NULL,
	       force_property_change_store),
	__ATTR(probe_en,
	       0644,
	       NULL,
	       probe_en_store),
	__ATTR(mirror,
	       0664,
	       mirror_axis_show,
	       mirror_axis_store),
	__ATTR(performance_debug,
	       0664,
	       performance_debug_show,
	       performance_debug_store),
	__ATTR(over_field_state,
	       0664,
	       over_field_state_show,
	       over_field_state_store),
	__ATTR(pps_coefs,
	       0664,
	       NULL,
	       pps_coefs_store),
	__ATTR(load_pps_coefs,
	       0664,
	       NULL,
	       load_pps_coefs_store),
	__ATTR(reg_dump,
	       0220,
	       NULL,
	       reg_dump_store),
	__ATTR(vd_attch_vpp,
	       0664,
	       vd_attach_vpp_show,
	       vd_attach_vpp_store),
	__ATTR_RO(blend_conflict),
	__ATTR_RO(force_disable),
	__ATTR(enable_hdmi_delay_normal_check,
	       0664,
	       enable_hdmi_delay_check_show,
	       enable_hdmi_delay_check_store),
	__ATTR(hdmin_delay_count_debug,
	       0664,
	       hdmi_delay_debug_show,
	       NULL),
	__ATTR(vd1_vd2_mux,
	       0664,
	       vd1_vd2_mux_show,
	       vd1_vd2_mux_store),
	__ATTR(aisr_en,
	       0664,
	       aisr_en_show,
	       aisr_en_store),
	__ATTR(reshape_output,
	       0664,
	       NULL,
	       reshape_output_store),
	__ATTR(aisr_pps_auto_calc,
	       0664,
	       pps_auto_calc_show,
	       NULL),
	__ATTR(ai_pq_disable,
	       0664,
	       ai_pq_disable_show,
	       ai_pq_disable_store),
	__ATTR(ai_pq_debug,
	       0664,
	       ai_pq_debug_show,
	       ai_pq_debug_store),
	__ATTR(ai_pq_value,
	       0664,
	       ai_pq_value_show,
	       ai_pq_value_store),
	__ATTR(ai_pq_policy,
	       0664,
	       ai_pq_policy_show,
	       ai_pq_policy_store),
	__ATTR(aisr_info,
	       0664,
	       aisr_info_show,
	       NULL),
#if defined(CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_VECM)
	__ATTR(det_stb_cnt,
	       0664,
	       det_stb_cnt_show,
	       det_stb_cnt_store),
	__ATTR(det_unstb_cnt,
	       0664,
	       det_unstb_cnt_show,
	       det_unstb_cnt_store),
	__ATTR(tolrnc_cnt,
	       0664,
	       tolrnc_cnt_show,
	       tolrnc_cnt_store),
	__ATTR(timer_filter_en,
	       0664,
	       timer_filter_en_show,
	       timer_filter_en_store),
	__ATTR(aipq_set_policy,
	       0664,
	       aipq_set_policy_show,
	       aipq_set_policy_store),
	__ATTR(color_th,
	       0664,
	       color_th_show,
	       color_th_store),
	__ATTR(aisr_demo_en,
	       0664,
	       aisr_demo_en_show,
	       aisr_demo_en_store),
	__ATTR(aisr_demo_axis,
	       0664,
	       aisr_demo_axis_show,
	       aisr_demo_axis_store),
	__ATTR(power_ctrl,
	       0664,
	       power_ctrl_show,
	       power_ctrl_store),
#endif
#ifdef CONFIG_AMLOGIC_MEDIA_FRC
	__ATTR(frc_delay,
	       0664,
	       frc_delay_show,
	       NULL),
#endif
	__ATTR(urgent_set,
		0644,
		vpu_module_urgent_show,
		vpu_module_urgent_set),
	__ATTR(aipq_dbg,
		0644,
		aipq_dbg_data_show,
		aipq_dbg_data_store),
	__ATTR(video_threshold_8k,
		0664,
		video_threshold_8k_show,
		video_threshold_8k_store),
	__ATTR(video_threshold_4k,
		0664,
		video_threshold_4k_show,
		video_threshold_4k_store),
	__ATTR(lowlatency_states,
	    0664,
	    lowlatency_states_show,
	    lowlatency_states_store),
	__ATTR(video_test_pattern,
	    0664,
	    video_test_pattern_show,
	    video_test_pattern_store),
	__ATTR(postblend_test_pattern,
	    0664,
	    postblend_test_pattern_show,
	    postblend_test_pattern_store),
	__ATTR(force_skip_count,
		0644,
		video_force_skip_cnt_show,
		video_force_skip_cnt_store),
	__ATTR(tvin_source_type,
		0664,
		tvin_source_type_show,
		tvin_source_type_store),
	__ATTR_RO(video_vd_proc_state),
	__ATTR(bypass_module,
		0664,
		bypass_module_s5_show,
		bypass_module_s5_store),
	__ATTR(matrix_set, 0644,
		set_post_matrix_show,
		set_post_matrix_store),
	__ATTR(matrix_pos, 0644,
		post_matrix_pos_show,
		post_matrix_pos_store),
	__ATTR(matrix_data, 0644,
		post_matrix_data_show,
		post_matrix_data_store),
	__ATTR(pre_vsync_enable, 0664,
		pre_vsync_enable_show,
		pre_vsync_enable_store),
	__ATTR(mosaic_axis_pic, 0644,
		mosaic_axis_pic_show,
		mosaic_axis_pic_store),
	__ATTR_RO(cur_ai_face),
	__ATTR(vsync_2to1_enable, 0664,
		vsync_2to1_enable_show,
		vsync_2to1_enable_store),
	__ATTR(vpp_in_padding, 0664,
		vpp_in_padding_enable_show,
		vpp_in_padding_enable_store),
	__ATTR(vpu_venc_status, 0664,
		vpu_venc_status_show,
		vpu_venc_status_store),
	__ATTR(video_max_src, 0664,
		video_max_src_show,
		NULL),
	__ATTR(videopip_max_src, 0664,
		videopip_max_src_show,
		NULL),
	__ATTR(videopip2_max_src, 0664,
		videopip2_max_src_show,
		NULL),
};

static struct class_attribute amvideo_poll_class_attrs[] = {
	__ATTR_RO(frame_width),
	__ATTR_RO(frame_height),
	__ATTR_RO(vframe_states),
	__ATTR_RO(video_state),
	__ATTR_RO(primary_src_fmt),
	__ATTR_RO(status_changed),
#if defined(CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_VECM)
#ifdef CONFIG_AMLOGIC_MEDIA_FRC
	__ATTR_RO(frc_delay),
#endif
#endif
	__ATTR_NULL
};

static struct class *amvideo_class;
static struct class *amvideo_poll_class;

static int video_attr_create(void)
{
	int i;
	int ret = 0;

	/* create amvideo_class */
	amvideo_class = class_create(THIS_MODULE, AMVIDEO_CLASS_NAME);
	if (IS_ERR(amvideo_class)) {
		pr_err("create amvideo_class fail\n");
		return -1;
	}

	/* create amvideo class attr files */
	for (i = 0; i < ARRAY_SIZE(amvideo_class_attrs); i++) {
		if (class_create_file(amvideo_class,
				      &amvideo_class_attrs[i])) {
			pr_err("create amvideo attribute %s fail\n",
			       amvideo_class_attrs[i].attr.name);
		}
	}

	/* create amvideo_poll_class */
	amvideo_poll_class = class_create(THIS_MODULE, AMVIDEO_POLL_CLASS_NAME);
	if (IS_ERR(amvideo_poll_class)) {
		pr_err("create amvideo_poll_class fail\n");
		return -1;
	}

	/* create amvideo_poll class attr files */
	for (i = 0; i < ARRAY_SIZE(amvideo_poll_class_attrs); i++) {
		if (class_create_file(amvideo_poll_class,
				      &amvideo_poll_class_attrs[i])) {
			pr_err("create amvideo_poll attribute %s fail\n",
			       amvideo_poll_class_attrs[i].attr.name);
		}
	}

	pr_debug("create video attribute OK\n");

	return ret;
}

#ifdef TV_REVERSE
int screen_orientation(void)
{
	int ret = 0;

	if (reverse)
		ret = HV_MIRROR;
	else if (video_mirror == H_MIRROR)
		ret = H_MIRROR;
	else if (video_mirror == V_MIRROR)
		ret = V_MIRROR;
	else
		ret = NO_MIRROR;

	return ret;
}

static int vpp_axis_reverse(char *str)
{
	char *ptr = str;

	pr_info("%s: bootargs is %s\n", __func__, str);

	/* Bootargs are defined as follows:
	 *   "video_reverse=n"
	 *      n=0: No flip
	 *      n=1: X and Y flip
	 *      n=2: X flip
	 *      n=3: Y flip
	 * The corresponding global vars:
	 *   reverse -- 0:No flip  1.X and Y flip
	 *    mirror -- 0:No flip  1:X flip 2:Y flip
	 */
	if (strstr(ptr, "1")) {
		reverse = true;
		video_mirror = 0;
	} else if (strstr(ptr, "2")) {
		reverse = false;
		video_mirror = H_MIRROR;
	} else if (strstr(ptr, "3")) {
		reverse = false;
		video_mirror = V_MIRROR;
	} else {
		reverse = false;
		video_mirror = 0;
	}

	return 0;
}

__setup("video_reverse=", vpp_axis_reverse);
#endif

struct vframe_s *get_cur_dispbuf(void)
{
	return cur_dispbuf[0];
}

#ifdef CONFIG_AM_VOUT
int vout_notify_callback(struct notifier_block *block, unsigned long cmd,
			 void *para)
{
	const struct vinfo_s *info;
	ulong flags;

	switch (cmd) {
	case VOUT_EVENT_MODE_CHANGE:
		info = get_current_vinfo();
		if (!info || info->mode == VMODE_INVALID)
			return 0;
		spin_lock_irqsave(&lock, flags);
		vinfo = info;
		/* pre-calculate vsync_pts_inc in 90k unit */
		vsync_pts_inc = 90000 * vinfo->sync_duration_den /
				vinfo->sync_duration_num;
		vsync_pts_inc_scale = vinfo->sync_duration_den;
		vsync_pts_inc_scale_base = vinfo->sync_duration_num;
		spin_unlock_irqrestore(&lock, flags);
		if (vinfo->name)
			strncpy(new_vmode, vinfo->name, sizeof(new_vmode) - 1);
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
		pr_info("DOLBY: %s: VOUT_EVENT_MODE_CHANGE\n",
			__func__);
		/* force send hdmi pkt in dv code */
		/* to workaround pkt cleaned during hotplug */
		if (is_amdv_enable())
			amdv_set_toggle_flag(2);
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_VECM
		else
			set_cur_hdr_policy(0xff);
#endif
#endif
		break;
	case VOUT_EVENT_OSD_PREBLEND_ENABLE:
		break;
	case VOUT_EVENT_OSD_DISP_AXIS:
		break;
	}
	return 0;
}

static struct notifier_block vout_notifier = {
	.notifier_call = vout_notify_callback,
};

static void vout_hook(void)
{
	vout_register_client(&vout_notifier);

	vinfo = get_current_vinfo();

	if (!vinfo) {
#if DEBUG_TMP
		set_current_vmode(VMODE_720P);
#endif
		vinfo = get_current_vinfo();
	}
	if (!vinfo || vinfo->mode == VMODE_INVALID)
		return;

	if (vinfo) {
		vsync_pts_inc = 90000 * vinfo->sync_duration_den /
			vinfo->sync_duration_num;
		vsync_pts_inc_scale = vinfo->sync_duration_den;
		vsync_pts_inc_scale_base = vinfo->sync_duration_num;
		if (vinfo->name)
			strncpy(old_vmode, vinfo->name, sizeof(old_vmode) - 1);
		if (vinfo->name)
			strncpy(new_vmode, vinfo->name, sizeof(new_vmode) - 1);
	}
#ifdef CONFIG_AM_VIDEO_LOG
	if (vinfo) {
		amlog_mask(LOG_MASK_VINFO, "vinfo = %p\n", vinfo);
		amlog_mask(LOG_MASK_VINFO, "display platform %s:\n",
			   vinfo->name);
		amlog_mask(LOG_MASK_VINFO, "\tresolution %d x %d\n",
			   vinfo->width, vinfo->height);
		amlog_mask(LOG_MASK_VINFO, "\taspect ratio %d : %d\n",
			   vinfo->aspect_ratio_num, vinfo->aspect_ratio_den);
		amlog_mask(LOG_MASK_VINFO, "\tsync duration %d : %d\n",
			   vinfo->sync_duration_num, vinfo->sync_duration_den);
	}
#endif
}
#endif

static int amvideo_notify_callback(struct notifier_block *block,
				   unsigned long cmd,
	void *para)
{
	u32 *p, val, unreg_flag;
	static struct vd_signal_info_s vd_signal;

	switch (cmd) {
	case AMVIDEO_UPDATE_OSD_MODE:
		p = (u32 *)para;
		if (!update_osd_vpp_misc)
			osd_vpp_misc_mask = p[1];
		val = osd_vpp_misc
			& (~osd_vpp_misc_mask);
		val |= (p[0] & osd_vpp_misc_mask);
		osd_vpp_misc = val;

		osd_vpp1_bld_ctrl_mask = p[3];
		val = (p[2] & osd_vpp1_bld_ctrl_mask);

		osd_vpp1_bld_ctrl = val;

		osd_vpp2_bld_ctrl_mask = p[5];
		val = (p[4] & osd_vpp2_bld_ctrl_mask);
		osd_vpp2_bld_ctrl = val;

		osd_vpp_bld_ctrl_update_mask = p[6];
		val = (p[2] & osd_vpp_bld_ctrl_update_mask);
		update_osd_vpp1_bld_ctrl = val;

		val = (p[4] & osd_vpp_bld_ctrl_update_mask);
		update_osd_vpp2_bld_ctrl = val;

		if (!update_osd_vpp_misc)
			update_osd_vpp_misc = true;
		break;
	case AMVIDEO_UPDATE_PREBLEND_MODE:
		p = (u32 *)para;
		osd_preblend_en = p[0];
		break;
	case AMVIDEO_UPDATE_SIGNAL_MODE:
		memcpy(&vd_signal, para,
		       sizeof(struct vd_signal_info_s));
		vd_signal_notifier_call_chain
			(VIDEO_SIGNAL_TYPE_CHANGED,
			&vd_signal);
		break;
	case AMVIDEO_UPDATE_VT_REG:
		p = (u32 *)para;
		val = p[0]; /* video tunnel id */
		if (p[1] == 1) { /* path enable */
			unreg_flag = atomic_read(&vt_unreg_flag);
			if (unreg_flag > 0)
				atomic_dec(&vt_unreg_flag);
		} else { /* path disable */
			hdmi_in_delay_maxmin_reset();
			while (atomic_read(&video_inirq_flag) > 0)
				schedule();
			if (cur_dev->pre_vsync_enable)
				while (atomic_read(&video_prevsync_inirq_flag) > 0)
					schedule();
			atomic_set(&vt_disable_video_done, 0);
			atomic_inc(&vt_unreg_flag);
			while (atomic_read(&vt_disable_video_done) > 0)
				schedule();
		}
		pr_info("%s vt reg/unreg: id %d, state:%d\n",
			__func__, val, p[1]);
		break;
	case AMVIDEO_UPDATE_FRC_CHAR_FLASH:
		p = (u32 *)para;
		val = p[0];
		if (val)
			glayer_info[0].ver_coef_adjust = true;
		else
			glayer_info[0].ver_coef_adjust = false;
		vd_layer[0].property_changed = true;
		break;
	default:
		break;
	}
	return 0;
}

static struct notifier_block amvideo_notifier = {
	.notifier_call = amvideo_notify_callback,
};

static RAW_NOTIFIER_HEAD(amvideo_notifier_list);
int amvideo_register_client(struct notifier_block *nb)
{
	return raw_notifier_chain_register(&amvideo_notifier_list, nb);
}
EXPORT_SYMBOL(amvideo_register_client);

int amvideo_unregister_client(struct notifier_block *nb)
{
	return raw_notifier_chain_unregister(&amvideo_notifier_list, nb);
}
EXPORT_SYMBOL(amvideo_unregister_client);

int amvideo_notifier_call_chain(unsigned long val, void *v)
{
	return raw_notifier_call_chain(&amvideo_notifier_list, val, v);
}
EXPORT_SYMBOL_GPL(amvideo_notifier_call_chain);

/*********************************************************/
struct device *get_video_device(void)
{
	return amvideo_dev;
}

static struct mconfig video_configs[] = {
	MC_PI32("pause_one_3d_fl_frame", &pause_one_3d_fl_frame),
	MC_PI32("debug_flag", &debug_flag),
	MC_PU32("force_3d_scaler", &force_3d_scaler),
	MC_PU32("video_3d_format", &video_3d_format),
	MC_PI32("vsync_enter_line_max", &vsync_enter_line_max),
	MC_PI32("vsync_exit_line_max", &vsync_exit_line_max),
#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
	MC_PI32("vsync_rdma_line_max", &vsync_rdma_line_max),
#endif
	MC_PU32("underflow", &underflow),
	MC_PU32("next_peek_underflow", &next_peek_underflow),
	MC_PU32("smooth_sync_enable", &smooth_sync_enable),
	MC_PU32("hdmi_in_onvideo", &hdmi_in_onvideo),
	MC_PU32("smooth_sync_enable", &smooth_sync_enable),
	MC_PU32("hdmi_in_onvideo", &hdmi_in_onvideo),
	MC_PU32("new_frame_count", &new_frame_count),
	MC_PU32("omx_pts", &omx_pts),
	MC_PU32("omx_pts_set_index", &omx_pts_set_index),
	MC_PBOOL("omx_run", &omx_run),
	MC_PU32("omx_version", &omx_version),
	MC_PU32("omx_info", &omx_info),
	MC_PI32("omx_need_drop_frame_num", &omx_need_drop_frame_num),
	MC_PBOOL("omx_drop_done", &omx_drop_done),
	MC_PI32("omx_pts_interval_upper", &omx_pts_interval_upper),
	MC_PI32("omx_pts_interval_lower", &omx_pts_interval_lower),
	MC_PBOOL("bypass_pps", &bypass_pps),
	MC_PU32("process_3d_type", &process_3d_type),
	MC_PU32("omx_pts", &omx_pts),
	MC_PU32("framepacking_support", &framepacking_support),
	MC_PU32("framepacking_width", &framepacking_width),
	MC_PU32("framepacking_height", &framepacking_height),
	MC_PU32("framepacking_blank", &framepacking_blank),
	MC_PU32("video_seek_flag", &video_seek_flag),
	MC_PU32("slowsync_repeat_enable", &slowsync_repeat_enable),
	MC_PU32("toggle_count", &toggle_count),
	MC_PBOOL("show_first_frame_nosync", &show_first_frame_nosync),
#ifdef TV_REVERSE
	MC_PBOOL("reverse", &reverse),
#endif
};

#ifdef CONFIG_AMLOGIC_LEGACY_EARLY_SUSPEND
#include <linux/amlogic/media/di/di_interface.h>
static void video_early_suspend(struct early_suspend *h)
{
	safe_switch_videolayer(0, false, false);
	safe_switch_videolayer(1, false, false);
	safe_switch_videolayer(2, false, false);
	di_disable_prelink_notify(0);
	video_suspend = true;
	pr_info("%s ok\n", __func__);
}

static void video_late_resume(struct early_suspend *h)
{
	video_suspend_cycle = 0;
	video_suspend = false;
	log_out = 1;
	pr_info("%s ok\n", __func__);
};

static struct early_suspend video_early_suspend_handler = {
	.suspend = video_early_suspend,
	.resume = video_late_resume,
};
#endif

#ifndef CONFIG_AMLOGIC_C3_REMOVE
static struct amvideo_device_data_s amvideo = {
	.cpu_type = MESON_CPU_MAJOR_ID_COMPATIBLE,
	.sr_reg_offt = 0xff,
	.sr_reg_offt2 = 0xff,
	.layer_support[0] = 0xff,
	.layer_support[1] = 0xff,
	.layer_support[2] = 0,
	.afbc_support[0] = 0xff,
	.afbc_support[1] = 0xff,
	.afbc_support[2] = 0,
	.pps_support[0] = 0xff,
	.pps_support[1] = 0xff,
	.pps_support[2] = 0,
	.alpha_support[0] = 0xff,
	.alpha_support[1] = 0xff,
	.alpha_support[2] = 0,
	.dv_support = 1,
	.sr0_support = 0xff,
	.sr1_support = 0xff,
	.core_v_disable_width_max[0] = 0xff,
	.core_v_disable_width_max[1] = 0xff,
	.core_v_enable_width_max[0] = 0xff,
	.core_v_enable_width_max[1] = 0xff,
	.supscl_path = 0xff,
	.fgrain_support[0] = 0,
	.fgrain_support[1] = 0,
	.fgrain_support[2] = 0,
	.has_hscaler_8tap[0] = 0,
	.has_hscaler_8tap[1] = 0,
	.has_hscaler_8tap[2] = 0,
	.has_pre_hscaler_ntap[0] = 0,
	.has_pre_hscaler_ntap[1] = 0,
	.has_pre_hscaler_ntap[2] = 0,
	.has_pre_vscaler_ntap[0] = 0,
	.has_pre_vscaler_ntap[1] = 0,
	.has_pre_vscaler_ntap[2] = 0,
	.src_width_max[0] = 0xff,
	.src_width_max[1] = 0xff,
	.src_height_max[0] = 0xff,
	.src_height_max[1] = 0xff,
	.ofifo_size = 0xff,
	.afbc_conv_lbuf_len[0] = 0x100,
	.afbc_conv_lbuf_len[1] = 0x100,
	.mif_linear = 0,
	.display_module = 0,
	.max_vd_layers = 2,
	.has_vpp1 = 0,
	.has_vpp2 = 0,
	.is_tv_panel = 0,
};

static struct amvideo_device_data_s amvideo_tm2_revb = {
	.cpu_type = MESON_CPU_MAJOR_ID_TM2_REVB,
	.sr_reg_offt = 0x1e00,
	.sr_reg_offt2 = 0x1f80,
	.layer_support[0] = 1,
	.layer_support[1] = 1,
	.layer_support[2] = 0,
	.afbc_support[0] = 1,
	.afbc_support[1] = 1,
	.afbc_support[2] = 0,
	.pps_support[0] = 1,
	.pps_support[1] = 1,
	.pps_support[2] = 0,
	.alpha_support[0] = 0,
	.alpha_support[1] = 0,
	.alpha_support[2] = 0,
	.dv_support = 1,
	.sr0_support = 1,
	.sr1_support = 1,
	.core_v_disable_width_max[0] = 2048,
	.core_v_disable_width_max[1] = 4096,
	.core_v_enable_width_max[0] = 1024,
	.core_v_enable_width_max[1] = 2048,
	.supscl_path = CORE0_PPS_CORE1,
	.fgrain_support[0] = 1,
	.fgrain_support[1] = 1,
	.fgrain_support[2] = 0,
	.has_hscaler_8tap[0] = 0,
	.has_hscaler_8tap[1] = 0,
	.has_hscaler_8tap[2] = 0,
	.has_pre_hscaler_ntap[0] = 0,
	.has_pre_hscaler_ntap[1] = 0,
	.has_pre_hscaler_ntap[2] = 0,
	.has_pre_vscaler_ntap[0] = 0,
	.has_pre_vscaler_ntap[1] = 0,
	.has_pre_vscaler_ntap[2] = 0,
	.src_width_max[0] = 4096,
	.src_width_max[1] = 4096,
	.src_height_max[0] = 2160,
	.src_height_max[1] = 2160,
	.ofifo_size = 0x1000,
	.afbc_conv_lbuf_len[0] = 0x100,
	.afbc_conv_lbuf_len[1] = 0x100,
	.mif_linear = 0,
	.display_module = 0,
	.max_vd_layers = 2,
	.has_vpp1 = 0,
	.has_vpp2 = 0,
	.is_tv_panel = 1,
};

static struct amvideo_device_data_s amvideo_sc2 = {
	.cpu_type = MESON_CPU_MAJOR_ID_SC2_,
	.sr_reg_offt = 0x1e00,
	.sr_reg_offt2 = 0x1f80,
	.layer_support[0] = 1,
	.layer_support[1] = 1,
	.layer_support[2] = 0,
	.afbc_support[0] = 1,
	.afbc_support[1] = 1,
	.afbc_support[2] = 0,
	.pps_support[0] = 1,
	.pps_support[1] = 1,
	.pps_support[2] = 0,
	.alpha_support[0] = 1,
	.alpha_support[1] = 1,
	.alpha_support[2] = 0,
	.dv_support = 1,
	.sr0_support = 1,
	.sr1_support = 0,
	.core_v_disable_width_max[0] = 4096,
	.core_v_disable_width_max[1] = 4096,
	.core_v_enable_width_max[0] = 2048,
	.core_v_enable_width_max[1] = 2048,
	.supscl_path = CORE0_BEFORE_PPS,
	.fgrain_support[0] = 1,
	.fgrain_support[1] = 1,
	.fgrain_support[2] = 0,
	.has_hscaler_8tap[0] = 1,
	.has_hscaler_8tap[1] = 1,
	.has_hscaler_8tap[2] = 0,
	.has_pre_hscaler_ntap[0] = 1,
	.has_pre_hscaler_ntap[1] = 1,
	.has_pre_hscaler_ntap[2] = 0,
	.has_pre_vscaler_ntap[0] = 0,
	.has_pre_vscaler_ntap[1] = 0,
	.has_pre_vscaler_ntap[2] = 0,
	.src_width_max[0] = 4096,
	.src_width_max[1] = 4096,
	.src_height_max[0] = 2160,
	.src_height_max[1] = 2160,
	.ofifo_size = 0x1000,
	.afbc_conv_lbuf_len[0] = 0x100,
	.afbc_conv_lbuf_len[1] = 0x100,
	.mif_linear = 0,
	.display_module = 0,
	.max_vd_layers = 2,
	.has_vpp1 = 0,
	.has_vpp2 = 0,
	.is_tv_panel = 0,
};

static struct amvideo_device_data_s amvideo_t5 = {
	.cpu_type = MESON_CPU_MAJOR_ID_T5_,
	.sr_reg_offt = 0x1e00,
	.sr_reg_offt2 = 0x1f80,
	.layer_support[0] = 1,
	.layer_support[1] = 1,
	.layer_support[2] = 0,
	.afbc_support[0] = 1,
	.afbc_support[1] = 0,
	.afbc_support[2] = 0,
	.pps_support[0] = 1,
	.pps_support[1] = 0,
	.pps_support[2] = 0,
	.alpha_support[0] = 0,
	.alpha_support[1] = 0,
	.alpha_support[2] = 0,
	.dv_support = 0,
	.sr0_support = 1,
	.sr1_support = 1,
	.core_v_disable_width_max[0] = 2048,
	.core_v_disable_width_max[1] = 4096,
	.core_v_enable_width_max[0] = 1024,
	.core_v_enable_width_max[1] = 2048,
	.supscl_path = CORE0_PPS_CORE1,
	.fgrain_support[0] = 0,
	.fgrain_support[1] = 0,
	.fgrain_support[2] = 0,
	.has_hscaler_8tap[0] = 1,
	.has_hscaler_8tap[1] = 0,
	.has_hscaler_8tap[2] = 0,
	.has_pre_hscaler_ntap[0] = 1,
	.has_pre_hscaler_ntap[1] = 0,
	.has_pre_hscaler_ntap[2] = 0,
	.has_pre_vscaler_ntap[0] = 1,
	.has_pre_vscaler_ntap[1] = 0,
	.has_pre_vscaler_ntap[2] = 0,
	.src_width_max[0] = 4096,
	.src_width_max[1] = 4096,
	.src_height_max[0] = 2160,
	.src_height_max[1] = 2160,
	.ofifo_size = 0x1000,
	.afbc_conv_lbuf_len[0] = 0x100,
	.afbc_conv_lbuf_len[1] = 0x100,
	.mif_linear = 0,
	.display_module = 0,
	.max_vd_layers = 2,
	.has_vpp1 = 0,
	.has_vpp2 = 0,
	.is_tv_panel = 1,
};

static struct amvideo_device_data_s amvideo_t5d = {
	.cpu_type = MESON_CPU_MAJOR_ID_T5D_,
	.sr_reg_offt = 0x1e00,
	.sr_reg_offt2 = 0x1f80,
	.layer_support[0] = 1,
	.layer_support[1] = 1,
	.layer_support[2] = 0,
	.afbc_support[0] = 1,
	.afbc_support[1] = 0,
	.afbc_support[2] = 0,
	.pps_support[0] = 1,
	.pps_support[1] = 1,
	.pps_support[2] = 0,
	.alpha_support[0] = 0,
	.alpha_support[1] = 0,
	.alpha_support[2] = 0,
	.dv_support = 0,
	.sr0_support = 0,
	.sr1_support = 1,
	.core_v_disable_width_max[0] = 1024,
	.core_v_disable_width_max[1] = 2048,
	.core_v_enable_width_max[0] = 1024,
	.core_v_enable_width_max[1] = 1024,
	.supscl_path = PPS_CORE1_CM,
	.fgrain_support[0] = 0,
	.fgrain_support[1] = 0,
	.fgrain_support[2] = 0,
	.has_hscaler_8tap[0] = 1,
	.has_hscaler_8tap[1] = 0,
	.has_hscaler_8tap[2] = 0,
	.has_pre_hscaler_ntap[0] = 1,
	.has_pre_hscaler_ntap[1] = 0,
	.has_pre_hscaler_ntap[2] = 0,
	.has_pre_vscaler_ntap[0] = 1,
	.has_pre_vscaler_ntap[1] = 0,
	.has_pre_vscaler_ntap[2] = 0,
	.src_width_max[0] = 2048,
	.src_width_max[1] = 2048,
	.src_height_max[0] = 1088,
	.src_height_max[1] = 1088,
	.ofifo_size = 0x780,
	.afbc_conv_lbuf_len[0] = 0x80,
	.afbc_conv_lbuf_len[1] = 0x80,
	.mif_linear = 0,
	.display_module = 0,
	.max_vd_layers = 2,
	.has_vpp1 = 0,
	.has_vpp2 = 0,
	.is_tv_panel = 1,
};

static struct amvideo_device_data_s amvideo_t7 = {
	.cpu_type = MESON_CPU_MAJOR_ID_T7_,
	.sr_reg_offt = 0x1e00,
	.sr_reg_offt2 = 0x1f80,
	.layer_support[0] = 1,
	.layer_support[1] = 1,
	.layer_support[2] = 1,
	.afbc_support[0] = 1,
	.afbc_support[1] = 1,
	.afbc_support[2] = 1,
	.pps_support[0] = 1,
	.pps_support[1] = 1,
	.pps_support[2] = 1,
	.alpha_support[0] = 1,
	.alpha_support[1] = 1,
	.alpha_support[2] = 1,
	.dv_support = 1,
	.sr0_support = 1,
	.sr1_support = 0,
	.core_v_disable_width_max[0] = 4096,
	.core_v_disable_width_max[1] = 4096,
	.core_v_enable_width_max[0] = 2048,
	.core_v_enable_width_max[1] = 2048,
	.supscl_path = CORE0_AFTER_PPS,
	.fgrain_support[0] = 1,
	.fgrain_support[1] = 1,
	.fgrain_support[2] = 1,
	.has_hscaler_8tap[0] = 1,
	.has_hscaler_8tap[1] = 1,
	.has_hscaler_8tap[2] = 1,
	.has_pre_hscaler_ntap[0] = 2,
	.has_pre_hscaler_ntap[1] = 2,
	.has_pre_hscaler_ntap[2] = 2,
	.has_pre_vscaler_ntap[0] = 1,
	.has_pre_vscaler_ntap[1] = 1,
	.has_pre_vscaler_ntap[2] = 1,
	.src_width_max[0] = 4096,
	.src_width_max[1] = 4096,
	.src_width_max[2] = 4096,
	.src_height_max[0] = 2160,
	.src_height_max[1] = 2160,
	.src_height_max[2] = 2160,
	.ofifo_size = 0x1000,
	.afbc_conv_lbuf_len[0] = 0x100,
	.afbc_conv_lbuf_len[1] = 0x100,
	.afbc_conv_lbuf_len[2] = 0x100,
	.mif_linear = 1,
	.display_module = T7_DISPLAY_MODULE,
	.max_vd_layers = 3,
	.has_vpp1 = 1,
	.has_vpp2 = 1,
	.is_tv_panel = 1,
};

static struct amvideo_device_data_s amvideo_s4 = {
	.cpu_type = MESON_CPU_MAJOR_ID_S4_,
	.sr_reg_offt = 0x1e00,
	.sr_reg_offt2 = 0x1f80,
	.layer_support[0] = 1,
	.layer_support[1] = 1,
	.layer_support[2] = 0,
	.afbc_support[0] = 1,
	.afbc_support[1] = 1,
	.afbc_support[2] = 0,
	.pps_support[0] = 1,
	.pps_support[1] = 1,
	.pps_support[2] = 0,
	.alpha_support[0] = 1,
	.alpha_support[1] = 1,
	.alpha_support[2] = 0,
	.dv_support = 0,
	.sr0_support = 1,
	.sr1_support = 0,
	.core_v_disable_width_max[0] = 4096,
	.core_v_disable_width_max[1] = 4096,
	.core_v_enable_width_max[0] = 2048,
	.core_v_enable_width_max[1] = 2048,
	.supscl_path = CORE0_BEFORE_PPS,
	.fgrain_support[0] = 0,
	.fgrain_support[1] = 0,
	.fgrain_support[2] = 0,
	.has_hscaler_8tap[0] = 1,
	.has_hscaler_8tap[1] = 1,
	.has_hscaler_8tap[2] = 0,
	.has_pre_hscaler_ntap[0] = 1,
	.has_pre_hscaler_ntap[1] = 1,
	.has_pre_hscaler_ntap[2] = 0,
	.has_pre_vscaler_ntap[0] = 1,
	.has_pre_vscaler_ntap[1] = 1,
	.has_pre_vscaler_ntap[2] = 0,
	.src_width_max[0] = 4096,
	.src_width_max[1] = 4096,
	.src_width_max[2] = 4096,
	.src_height_max[0] = 2160,
	.src_height_max[1] = 2160,
	.src_height_max[2] = 2160,
	.ofifo_size = 0x1000,
	.afbc_conv_lbuf_len[0] = 0x100,
	.afbc_conv_lbuf_len[1] = 0x100,
	.mif_linear = 0,
	.display_module = 0,
	.max_vd_layers = 2,
	.is_tv_panel = 0,
};

static struct amvideo_device_data_s amvideo_t5d_revb = {
	.cpu_type = MESON_CPU_MAJOR_ID_T5D_REVB_,
	.sr_reg_offt = 0x1e00,
	.sr_reg_offt2 = 0x1f80,
	.layer_support[0] = 1,
	.layer_support[1] = 1,
	.layer_support[2] = 0,
	.afbc_support[0] = 1,
	.afbc_support[1] = 0,
	.afbc_support[2] = 0,
	.pps_support[0] = 1,
	.pps_support[1] = 1,
	.pps_support[2] = 0,
	.alpha_support[0] = 0,
	.alpha_support[1] = 0,
	.alpha_support[2] = 0,
	.dv_support = 0,
	.sr0_support = 0,
	.sr1_support = 1,
	.core_v_disable_width_max[0] = 1024,
	.core_v_disable_width_max[1] = 2048,
	.core_v_enable_width_max[0] = 1024,
	.core_v_enable_width_max[1] = 1024,
	.supscl_path = PPS_CORE1_CM,
	.fgrain_support[0] = 0,
	.fgrain_support[1] = 0,
	.fgrain_support[2] = 0,
	.has_hscaler_8tap[0] = 1,
	.has_hscaler_8tap[1] = 0,
	.has_hscaler_8tap[2] = 0,
	.has_pre_hscaler_ntap[0] = 1,
	.has_pre_hscaler_ntap[1] = 0,
	.has_pre_hscaler_ntap[2] = 0,
	.has_pre_vscaler_ntap[0] = 1,
	.has_pre_vscaler_ntap[1] = 0,
	.has_pre_vscaler_ntap[2] = 0,
	.src_width_max[0] = 2048,
	.src_width_max[1] = 2048,
	.src_height_max[0] = 1088,
	.src_height_max[1] = 1088,
	.ofifo_size = 0x780,
	.afbc_conv_lbuf_len[0] = 0x80,
	.afbc_conv_lbuf_len[1] = 0x80,
	.mif_linear = 0,
	.display_module = 0,
	.max_vd_layers = 2,
	.is_tv_panel = 1,
};

static struct amvideo_device_data_s amvideo_t3 = {
	.cpu_type = MESON_CPU_MAJOR_ID_T3_,
	.sr_reg_offt = 0x1e00,
	.sr_reg_offt2 = 0x1f80,
	.layer_support[0] = 1,
	.layer_support[1] = 1,
	.layer_support[2] = 0,
	.afbc_support[0] = 1,
	.afbc_support[1] = 1,
	.afbc_support[2] = 0,
	.pps_support[0] = 1,
	.pps_support[1] = 1,
	.pps_support[2] = 0,
	.alpha_support[0] = 1,
	.alpha_support[1] = 1,
	.alpha_support[2] = 0,
	.dv_support = 1,
	.sr0_support = 1,
	.sr1_support = 1,
	.core_v_disable_width_max[0] = 2048,
	.core_v_disable_width_max[1] = 4096,
	.core_v_enable_width_max[0] = 1024,
	.core_v_enable_width_max[1] = 2048,
	.supscl_path = CORE0_PPS_CORE1,
	.fgrain_support[0] = 1,
	.fgrain_support[1] = 1,
	.fgrain_support[2] = 0,
	.has_hscaler_8tap[0] = 1,
	.has_hscaler_8tap[1] = 1,
	.has_hscaler_8tap[2] = 0,
	.has_pre_hscaler_ntap[0] = 2,
	.has_pre_hscaler_ntap[1] = 2,
	.has_pre_hscaler_ntap[2] = 0,
	.has_pre_vscaler_ntap[0] = 1,
	.has_pre_vscaler_ntap[1] = 1,
	.has_pre_vscaler_ntap[2] = 0,
	.src_width_max[0] = 4096,
	.src_width_max[1] = 4096,
	.src_width_max[2] = 4096,
	.src_height_max[0] = 2160,
	.src_height_max[1] = 2160,
	.src_height_max[2] = 2160,
	.ofifo_size = 0x1000,
	.afbc_conv_lbuf_len[0] = 0x100,
	.afbc_conv_lbuf_len[1] = 0x100,
	.mif_linear = 1,
	.display_module = T7_DISPLAY_MODULE,
	.max_vd_layers = 2,
	.has_vpp1 = 1,
	.has_vpp2 = 0,
	.is_tv_panel = 1,
};

static struct amvideo_device_data_s amvideo_t5w = {
	.cpu_type = MESON_CPU_MAJOR_ID_T5W_,
	.sr_reg_offt = 0x1e00,
	.sr_reg_offt2 = 0x1f80,
	.layer_support[0] = 1,
	.layer_support[1] = 1,
	.layer_support[2] = 0,
	.afbc_support[0] = 1,
	.afbc_support[1] = 1,
	.afbc_support[2] = 0,
	.pps_support[0] = 1,
	.pps_support[1] = 1,
	.pps_support[2] = 0,
	.alpha_support[0] = 1,
	.alpha_support[1] = 1,
	.alpha_support[2] = 0,
	.dv_support = 1,
	.sr0_support = 1,
	.sr1_support = 1,
	.core_v_disable_width_max[0] = 2048,
	.core_v_disable_width_max[1] = 4096,
	.core_v_enable_width_max[0] = 1024,
	.core_v_enable_width_max[1] = 2048,
	.supscl_path = CORE0_PPS_CORE1,
	.fgrain_support[0] = 1,
	.fgrain_support[1] = 1,
	.fgrain_support[2] = 0,
	.has_hscaler_8tap[0] = 1,
	.has_hscaler_8tap[1] = 1,
	.has_hscaler_8tap[2] = 0,
	.has_pre_hscaler_ntap[0] = 2,
	.has_pre_hscaler_ntap[1] = 2,
	.has_pre_hscaler_ntap[2] = 0,
	.has_pre_vscaler_ntap[0] = 1,
	.has_pre_vscaler_ntap[1] = 1,
	.has_pre_vscaler_ntap[2] = 0,
	.src_width_max[0] = 4096,
	.src_width_max[1] = 2048,
	.src_width_max[2] = 4096,
	.src_height_max[0] = 2160,
	.src_height_max[1] = 1088,
	.src_height_max[2] = 2160,
	.ofifo_size = 0x1000,
	.afbc_conv_lbuf_len[0] = 0x100,
	.afbc_conv_lbuf_len[1] = 0x80,
	.mif_linear = 1,
	.display_module = T7_DISPLAY_MODULE,
	.max_vd_layers = 2,
	.has_vpp1 = 1,
	.has_vpp2 = 0,
	.is_tv_panel = 1,
};
#endif

static struct amvideo_device_data_s amvideo_c3 = {
	.cpu_type = MESON_CPU_MAJOR_ID_C3_,
	.sr_reg_offt = 0x1e00,
	.sr_reg_offt2 = 0x1f80,
	.layer_support[0] = 1,
	.layer_support[1] = 0,
	.layer_support[2] = 0,
	.afbc_support[0] = 0,
	.afbc_support[1] = 0,
	.afbc_support[2] = 0,
	.pps_support[0] = 0,
	.pps_support[1] = 0,
	.pps_support[2] = 0,
	.alpha_support[0] = 0,
	.alpha_support[1] = 0,
	.alpha_support[2] = 0,
	.dv_support = 0,
	.sr0_support = 0,
	.sr1_support = 0,
	.core_v_disable_width_max[0] = 2048,
	.core_v_disable_width_max[1] = 4096,
	.core_v_enable_width_max[0] = 1024,
	.core_v_enable_width_max[1] = 2048,
	.supscl_path = CORE0_PPS_CORE1,
	.fgrain_support[0] = 0,
	.fgrain_support[1] = 0,
	.fgrain_support[2] = 0,
	.has_hscaler_8tap[0] = 0,
	.has_hscaler_8tap[1] = 0,
	.has_hscaler_8tap[2] = 0,
	.has_pre_hscaler_ntap[0] = 0,
	.has_pre_hscaler_ntap[1] = 0,
	.has_pre_hscaler_ntap[2] = 0,
	.has_pre_vscaler_ntap[0] = 0,
	.has_pre_vscaler_ntap[1] = 0,
	.has_pre_vscaler_ntap[2] = 0,
	.src_width_max[0] = 4096,
	.src_width_max[1] = 2048,
	.src_width_max[2] = 4096,
	.src_height_max[0] = 2160,
	.src_height_max[1] = 1088,
	.src_height_max[2] = 2160,
	.ofifo_size = 0x1000,
	.afbc_conv_lbuf_len[0] = 0x100,
	.afbc_conv_lbuf_len[1] = 0x80,
	.mif_linear = 1,
	.display_module = C3_DISPLAY_MODULE,
	.max_vd_layers = 1,
	.has_vpp1 = 0,
	.has_vpp2 = 0,
	.is_tv_panel = 1,
};

#ifndef CONFIG_AMLOGIC_C3_REMOVE
static struct amvideo_device_data_s amvideo_s5 = {
	.cpu_type = MESON_CPU_MAJOR_ID_S5_,
	.sr_reg_offt = 0x1e00,
	.sr_reg_offt2 = 0x1f80,
	.layer_support[0] = 1,
	.layer_support[1] = 1,
	.layer_support[2] = 0,
	.afbc_support[0] = 1,
	.afbc_support[1] = 1,
	.afbc_support[2] = 0,
	.pps_support[0] = 1,
	.pps_support[1] = 1,
	.pps_support[2] = 0,
	.alpha_support[0] = 1,
	.alpha_support[1] = 1,
	.alpha_support[2] = 0,
	.dv_support = 1,
	.sr0_support = 1,
	.sr1_support = 1,
	.core_v_disable_width_max[0] = 2048,
	.core_v_disable_width_max[1] = 4096,
	.core_v_enable_width_max[0] = 1024,
	.core_v_enable_width_max[1] = 2048,
	.supscl_path = CORE0_PPS_CORE1,
	.fgrain_support[0] = 1,
	.fgrain_support[1] = 1,
	.fgrain_support[2] = 0,
	.has_hscaler_8tap[0] = 1,
	.has_hscaler_8tap[1] = 1,
	.has_hscaler_8tap[2] = 0,
	.has_pre_hscaler_ntap[0] = 2,
	.has_pre_hscaler_ntap[1] = 2,
	.has_pre_hscaler_ntap[2] = 0,
	.has_pre_vscaler_ntap[0] = 1,
	.has_pre_vscaler_ntap[1] = 1,
	.has_pre_vscaler_ntap[2] = 0,
	.src_width_max[0] = 8192,
	.src_width_max[1] = 8192,
	.src_width_max[2] = 4096,
	.src_height_max[0] = 4320,
	.src_height_max[1] = 4320,
	.src_height_max[2] = 2160,
	.ofifo_size = 0x800,
	.afbc_conv_lbuf_len[0] = 0x100,
	.afbc_conv_lbuf_len[1] = 0x100,
	.mif_linear = 1,
	.display_module = S5_DISPLAY_MODULE,
	.max_vd_layers = 2,
	.has_vpp1 = 1,
	.has_vpp2 = 0,
	.is_tv_panel = 0,
};

static struct amvideo_device_data_s amvideo_t5m = {
	.cpu_type = MESON_CPU_MAJOR_ID_T5M_,
	.sr_reg_offt = 0x1e00,
	.sr_reg_offt2 = 0x1f80,
	.layer_support[0] = 1,
	.layer_support[1] = 1,
	.layer_support[2] = 0,
	.afbc_support[0] = 1,
	.afbc_support[1] = 1,
	.afbc_support[2] = 0,
	.pps_support[0] = 1,
	.pps_support[1] = 1,
	.pps_support[2] = 0,
	.alpha_support[0] = 1,
	.alpha_support[1] = 1,
	.alpha_support[2] = 0,
	.dv_support = 1,
	.sr0_support = 1,
	.sr1_support = 1,
	.core_v_disable_width_max[0] = 2048,
	.core_v_disable_width_max[1] = 4096,
	.core_v_enable_width_max[0] = 1024,
	.core_v_enable_width_max[1] = 2048,
	.supscl_path = CORE0_PPS_CORE1,
	.fgrain_support[0] = 1,
	.fgrain_support[1] = 1,
	.fgrain_support[2] = 0,
	.has_hscaler_8tap[0] = 1,
	.has_hscaler_8tap[1] = 1,
	.has_hscaler_8tap[2] = 0,
	.has_pre_hscaler_ntap[0] = 2,
	.has_pre_hscaler_ntap[1] = 2,
	.has_pre_hscaler_ntap[2] = 0,
	.has_pre_vscaler_ntap[0] = 1,
	.has_pre_vscaler_ntap[1] = 1,
	.has_pre_vscaler_ntap[2] = 0,
	.src_width_max[0] = 4096,
	.src_width_max[1] = 2048,
	.src_width_max[2] = 4096,
	.src_height_max[0] = 2160,
	.src_height_max[1] = 1088,
	.src_height_max[2] = 2160,
	.ofifo_size = 0x1000,
	.afbc_conv_lbuf_len[0] = 0x100,
	.afbc_conv_lbuf_len[1] = 0x80,
	.mif_linear = 1,
	.display_module = T7_DISPLAY_MODULE,
	.max_vd_layers = 2,
	.has_vpp1 = 1,
	.has_vpp2 = 0,
	.is_tv_panel = 1,
};

static struct amvideo_device_data_s amvideo_t3x = {
	.cpu_type = MESON_CPU_MAJOR_ID_T3X_,
	.sr_reg_offt = 0x1e00,
	.sr_reg_offt2 = 0x1f80,
	.layer_support[0] = 1,
	.layer_support[1] = 1,
	.layer_support[2] = 0,
	.afbc_support[0] = 1,
	.afbc_support[1] = 1,
	.afbc_support[2] = 0,
	.pps_support[0] = 1,
	.pps_support[1] = 1,
	.pps_support[2] = 0,
	.alpha_support[0] = 1,
	.alpha_support[1] = 1,
	.alpha_support[2] = 0,
	.dv_support = 1,
	.sr0_support = 1,
	.sr1_support = 1,
	.core_v_disable_width_max[0] = 2048,
	.core_v_disable_width_max[1] = 4096,
	.core_v_enable_width_max[0] = 1024,
	.core_v_enable_width_max[1] = 2048,
	.supscl_path = CORE0_PPS_CORE1,
	.fgrain_support[0] = 1,
	.fgrain_support[1] = 1,
	.fgrain_support[2] = 0,
	.has_hscaler_8tap[0] = 1,
	.has_hscaler_8tap[1] = 1,
	.has_hscaler_8tap[2] = 0,
	.has_pre_hscaler_ntap[0] = 2,
	.has_pre_hscaler_ntap[1] = 2,
	.has_pre_hscaler_ntap[2] = 0,
	.has_pre_vscaler_ntap[0] = 1,
	.has_pre_vscaler_ntap[1] = 1,
	.has_pre_vscaler_ntap[2] = 0,
	.src_width_max[0] = 4096,
	.src_width_max[1] = 4096,
	.src_width_max[2] = 4096,
	.src_height_max[0] = 2160,
	.src_height_max[1] = 2160,
	.src_height_max[2] = 2160,
	.ofifo_size = 0x800,
	.afbc_conv_lbuf_len[0] = 0x100,
	.afbc_conv_lbuf_len[1] = 0x100,
	.mif_linear = 1,
	.display_module = S5_DISPLAY_MODULE,
	.max_vd_layers = 2,
	.has_vpp1 = 1,
	.has_vpp2 = 0,
	.is_tv_panel = 1,
};

static struct amvideo_device_data_s amvideo_txhd2 = {
	.cpu_type = MESON_CPU_MAJOR_ID_TXHD2_,
	.sr_reg_offt = 0x1e00,
	.sr_reg_offt2 = 0x1f80,
	.layer_support[0] = 1,
	.layer_support[1] = 0,
	.layer_support[2] = 0,
	.afbc_support[0] = 1,
	.afbc_support[1] = 0,
	.afbc_support[2] = 0,
	.pps_support[0] = 1,
	.pps_support[1] = 0,
	.pps_support[2] = 0,
	.alpha_support[0] = 0,
	.alpha_support[1] = 0,
	.alpha_support[2] = 0,
	.dv_support = 0,
	.sr0_support = 1,
	.sr1_support = 0,
	.core_v_disable_width_max[0] = 2048,
	.core_v_disable_width_max[1] = 2048,
	.core_v_enable_width_max[0] = 1024,
	.core_v_enable_width_max[1] = 1024,
	.supscl_path = CORE0_BEFORE_PPS,
	.fgrain_support[0] = 0,
	.fgrain_support[1] = 0,
	.fgrain_support[2] = 0,
	.has_hscaler_8tap[0] = 1,
	.has_hscaler_8tap[1] = 0,
	.has_hscaler_8tap[2] = 0,
	.has_pre_hscaler_ntap[0] = 1,
	.has_pre_hscaler_ntap[1] = 0,
	.has_pre_hscaler_ntap[2] = 0,
	.has_pre_vscaler_ntap[0] = 1,
	.has_pre_vscaler_ntap[1] = 0,
	.has_pre_vscaler_ntap[2] = 0,
	.src_width_max[0] = 2048,
	.src_width_max[1] = 2048,
	.src_height_max[0] = 1088,
	.src_height_max[1] = 1088,
	.ofifo_size = 0x780,
	.afbc_conv_lbuf_len[0] = 0x80,
	.afbc_conv_lbuf_len[1] = 0x80,
	.mif_linear = 0,
	.display_module = 0,
	.max_vd_layers = 1,
	.is_tv_panel = 1,
};

static struct amvideo_device_data_s amvideo_s1a = {
	.cpu_type = MESON_CPU_MAJOR_ID_S1A_,
	.sr_reg_offt = 0x1e00,
	.sr_reg_offt2 = 0x1f80,
	.layer_support[0] = 1,
	.layer_support[1] = 1,
	.layer_support[2] = 0,
	.afbc_support[0] = 0,
	.afbc_support[1] = 0,
	.afbc_support[2] = 0,
	.pps_support[0] = 1,
	.pps_support[1] = 0,
	.pps_support[2] = 0,
	.alpha_support[0] = 0,
	.alpha_support[1] = 0,
	.alpha_support[2] = 0,
	.dv_support = 0,
	.sr0_support = 0,
	.sr1_support = 0,
	.core_v_disable_width_max[0] = 4096,
	.core_v_disable_width_max[1] = 4096,
	.core_v_enable_width_max[0] = 2048,
	.core_v_enable_width_max[1] = 2048,
	.supscl_path = CORE0_PPS_CORE1,
	.fgrain_support[0] = 0,
	.fgrain_support[1] = 0,
	.fgrain_support[2] = 0,
	.has_hscaler_8tap[0] = 1,
	.has_hscaler_8tap[1] = 1,
	.has_hscaler_8tap[2] = 0,
	.has_pre_hscaler_ntap[0] = 1,
	.has_pre_hscaler_ntap[1] = 1,
	.has_pre_hscaler_ntap[2] = 0,
	.has_pre_vscaler_ntap[0] = 1,
	.has_pre_vscaler_ntap[1] = 1,
	.has_pre_vscaler_ntap[2] = 0,
	.src_width_max[0] = 2048,
	.src_width_max[1] = 2048,
	.src_width_max[2] = 2048,
	.src_height_max[0] = 1088,
	.src_height_max[1] = 1088,
	.src_height_max[2] = 1088,
	.ofifo_size = 0x800,
	.afbc_conv_lbuf_len[0] = 0x100,
	.afbc_conv_lbuf_len[1] = 0x100,
	.mif_linear = 0,
	.display_module = 0,
	.max_vd_layers = 2,
	.is_tv_panel = 0,
};

static struct amvideo_device_data_s amvideo_s7 = {
	.cpu_type = MESON_CPU_MAJOR_ID_S7_,
	.sr_reg_offt = 0x1e00,
	.sr_reg_offt2 = 0x1f80,
	.layer_support[0] = 1,
	.layer_support[1] = 1,
	.layer_support[2] = 0,
	.afbc_support[0] = 1,
	.afbc_support[1] = 1,
	.afbc_support[2] = 0,
	.pps_support[0] = 1,
	.pps_support[1] = 1,
	.pps_support[2] = 0,
	.alpha_support[0] = 1,
	.alpha_support[1] = 1,
	.alpha_support[2] = 0,
	.dv_support = 0,
	.sr0_support = 1,
	.sr1_support = 0,
	.core_v_disable_width_max[0] = 4096,
	.core_v_disable_width_max[1] = 4096,
	.core_v_enable_width_max[0] = 2048,
	.core_v_enable_width_max[1] = 2048,
	.supscl_path = CORE0_BEFORE_PPS,
	.fgrain_support[0] = 1,
	.fgrain_support[1] = 0,
	.fgrain_support[2] = 0,
	.has_hscaler_8tap[0] = 1,
	.has_hscaler_8tap[1] = 1,
	.has_hscaler_8tap[2] = 0,
	.has_pre_hscaler_ntap[0] = 1,
	.has_pre_hscaler_ntap[1] = 1,
	.has_pre_hscaler_ntap[2] = 0,
	.has_pre_vscaler_ntap[0] = 1,
	.has_pre_vscaler_ntap[1] = 1,
	.has_pre_vscaler_ntap[2] = 0,
	.src_width_max[0] = 4096,
	.src_width_max[1] = 2048,
	.src_width_max[2] = 2048,
	.src_height_max[0] = 2160,
	.src_height_max[1] = 1080,
	.src_height_max[2] = 1080,
	.ofifo_size = 0x1000,
	.afbc_conv_lbuf_len[0] = 0x100,
	.afbc_conv_lbuf_len[1] = 0x80,
	.mif_linear = 1,
	.display_module = 0,
	.max_vd_layers = 2,
	.is_tv_panel = 0,
};
#endif

static struct video_device_hw_s legcy_dev_property = {
	.vd2_independ_blend_ctrl = 0,
	.aisr_support = 0,
	.prevsync_support = 0,
	.sr_in_size = 0,
	.sr01_num = 1,
};

#ifndef CONFIG_AMLOGIC_C3_REMOVE
static struct video_device_hw_s t3_dev_property = {
	.vd2_independ_blend_ctrl = 1,
	.aisr_support = 1,
	.prevsync_support = 0,
	/* aisr reverse workaround for t3*/
	.di_hf_y_reverse = 1,
	.sr_in_size = 1,
	.sr01_num = 1,
};

static struct video_device_hw_s t5w_dev_property = {
	.vd2_independ_blend_ctrl = 1,
	.aisr_support = 0,
	.prevsync_support = 0,
	.di_hf_y_reverse = 0,
	.sr_in_size = 1,
	.sr01_num = 1,
};
#endif

static struct video_device_hw_s c3_dev_property = {
	.vd2_independ_blend_ctrl = 0,
	.aisr_support = 0,
	.prevsync_support = 0,
	.di_hf_y_reverse = 0,
	.sr_in_size = 0,
	.sr01_num = 1,
};

#ifndef CONFIG_AMLOGIC_C3_REMOVE
static struct video_device_hw_s s5_dev_property = {
	.vd2_independ_blend_ctrl = 1,
	.aisr_support = 1,
	.prevsync_support = 0,
	/* aisr reverse workaround for t3*/
	.di_hf_y_reverse = 1,
	.sr_in_size = 1,
	.mosaic_support = 1,
	.sr01_num = 1,
	.cr_loss = 0,
	.amdv_tvcore = 0,
};

static struct video_device_hw_s t5m_dev_property = {
	.vd2_independ_blend_ctrl = 1,
	.aisr_support = 0,
	.prevsync_support = 1,
	.di_hf_y_reverse = 0,
	.sr_in_size = 1,
};

static struct video_device_hw_s t3x_dev_property = {
	.vd2_independ_blend_ctrl = 1,
	.aisr_support = 1,
	.prevsync_support = 1,
	/* aisr reverse workaround for t3x*/
	.di_hf_y_reverse = 1,
	.sr_in_size = 1,
	.mosaic_support = 0,
	.sr01_num = 2,
	.cr_loss = 1,
	.amdv_tvcore = 1,
	.vpp_in_padding_support = 1,
};
#endif

static const struct of_device_id amlogic_amvideom_dt_match[] = {
#ifndef CONFIG_AMLOGIC_C3_REMOVE
	{
		.compatible = "amlogic, amvideom",
		.data = &amvideo,
	},
	{
		.compatible = "amlogic, amvideom-tm2-revb",
		.data = &amvideo_tm2_revb,
	},
	{
		.compatible = "amlogic, amvideom-sc2",
		.data = &amvideo_sc2,
	},
	{
		.compatible = "amlogic, amvideom-t5",
		.data = &amvideo_t5,
	},
	{
		.compatible = "amlogic, amvideom-t5d",
		.data = &amvideo_t5d,
	},
	{
		.compatible = "amlogic, amvideom-t7",
		.data = &amvideo_t7,
	},
	{
		.compatible = "amlogic, amvideom-s4",
		.data = &amvideo_s4,
	},
	{
		.compatible = "amlogic, amvideom-t5d-revb",
		.data = &amvideo_t5d_revb,
	},
	{
		.compatible = "amlogic, amvideom-t3",
		.data = &amvideo_t3,
	},
	{
		.compatible = "amlogic, amvideom-t5w",
		.data = &amvideo_t5w,
	},
#endif
	{
		.compatible = "amlogic, amvideom-c3",
		.data = &amvideo_c3,
	},
#ifndef CONFIG_AMLOGIC_C3_REMOVE
	{
		.compatible = "amlogic, amvideom-s5",
		.data = &amvideo_s5,
	},
	{
		.compatible = "amlogic, amvideom-t5m",
		.data = &amvideo_t5m,
	},
	{
		.compatible = "amlogic, amvideom-t3x",
		.data = &amvideo_t3x,
	},
	{
		.compatible = "amlogic, amvideom-txhd2",
		.data = &amvideo_txhd2,
	},
	{
		.compatible = "amlogic, amvideom-s1a",
		.data = &amvideo_s1a,
	},
	{
		.compatible = "amlogic, amvideom-s7",
		.data = &amvideo_s7,
	},
#endif
	{}
};

bool is_meson_tm2_revb(void)
{
	if (amvideo_meson_dev.cpu_type ==
		MESON_CPU_MAJOR_ID_TM2_REVB)
		return true;
	else
		return false;
}

bool video_is_meson_sc2_cpu(void)
{
	if (amvideo_meson_dev.cpu_type ==
		MESON_CPU_MAJOR_ID_SC2_)
		return true;
	else
		return false;
}

bool video_is_meson_t5d_cpu(void)
{
	if (amvideo_meson_dev.cpu_type ==
		MESON_CPU_MAJOR_ID_T5D_)
		return true;
	else
		return false;
}

bool video_is_meson_t7_cpu(void)
{
	if (amvideo_meson_dev.cpu_type ==
		MESON_CPU_MAJOR_ID_T7_)
		return true;
	else
		return false;
}

bool video_is_meson_s4_cpu(void)
{
	if (amvideo_meson_dev.cpu_type ==
		MESON_CPU_MAJOR_ID_S4_)
		return true;
	else
		return false;
}

bool video_is_meson_t5d_revb_cpu(void)
{
	if (amvideo_meson_dev.cpu_type ==
		MESON_CPU_MAJOR_ID_T5D_REVB_)
		return true;
	else
		return false;
}

bool video_is_meson_t3_cpu(void)
{
	if (amvideo_meson_dev.cpu_type ==
		MESON_CPU_MAJOR_ID_T3_)
		return true;
	else
		return false;
}

bool video_is_meson_c3_cpu(void)
{
	if (amvideo_meson_dev.cpu_type ==
		MESON_CPU_MAJOR_ID_C3_)
		return true;
	else
		return false;
}

bool video_is_meson_t5w_cpu(void)
{
	if (amvideo_meson_dev.cpu_type ==
		MESON_CPU_MAJOR_ID_T5W_)
		return true;
	else
		return false;
}

bool video_is_meson_s5_cpu(void)
{
	if (amvideo_meson_dev.cpu_type ==
		MESON_CPU_MAJOR_ID_S5_)
		return true;
	else
		return false;
}

bool video_is_meson_t5m_cpu(void)
{
	if (amvideo_meson_dev.cpu_type ==
		MESON_CPU_MAJOR_ID_T5M_)
		return true;
	else
		return false;
}

bool video_is_meson_t3x_cpu(void)
{
	if (amvideo_meson_dev.cpu_type ==
		MESON_CPU_MAJOR_ID_T3X_)
		return true;
	else
		return false;
}

bool video_is_meson_txhd2_cpu(void)
{
	if (amvideo_meson_dev.cpu_type ==
		MESON_CPU_MAJOR_ID_TXHD2_)
		return true;
	else
		return false;
}

bool video_is_meson_s1a_cpu(void)
{
	if (amvideo_meson_dev.cpu_type ==
		MESON_CPU_MAJOR_ID_S1A_)
		return true;
	else
		return false;
}

bool video_is_meson_s7_cpu(void)
{
	if (amvideo_meson_dev.cpu_type ==
		MESON_CPU_MAJOR_ID_S7_)
		return true;
	else
		return false;
}

bool has_hscaler_8tap(u8 layer_id)
{
	if (amvideo_meson_dev.has_hscaler_8tap[layer_id])
		return true;
	else
		return false;
}

bool has_pre_hscaler_ntap(u8 layer_id)
{
	if (amvideo_meson_dev.has_pre_hscaler_ntap[layer_id])
		return true;
	else
		return false;
}

bool has_pre_hscaler_8tap(u8 layer_id)
{
	if (amvideo_meson_dev.has_pre_hscaler_ntap[layer_id] == 2)
		return true;
	else
		return false;
}

bool has_pre_vscaler_ntap(u8 layer_id)
{
	if (amvideo_meson_dev.has_pre_vscaler_ntap[layer_id])
		return true;
	else
		return false;
}

int get_video_src_max_buffer(u8 layer_id,
	u32 *src_width, u32 *src_height)
{
	if (layer_id >= MAX_VD_LAYER)
		return -1;
	*src_width = amvideo_meson_dev.src_width_max[layer_id];
	*src_height = amvideo_meson_dev.src_height_max[layer_id];
	return 0;
}

int get_video_src_min_buffer(u8 layer_id,
	u32 *src_width, u32 *src_height)
{
	if (layer_id >= MAX_VD_LAYER)
		return -1;
	*src_width = 64;
	*src_height = 64;
	return 0;
}

static void video_cap_set(struct amvideo_device_data_s *p_amvideo)
{
	if (p_amvideo->cpu_type ==
		MESON_CPU_MAJOR_ID_COMPATIBLE) {
		if (legacy_vpp) {
			layer_cap =
				LAYER1_AFBC |
				LAYER1_AVAIL |
				LAYER0_AFBC |
				LAYER0_SCALER |
				LAYER0_AVAIL;
		} else if (is_meson_tl1_cpu()) {
			layer_cap =
				LAYER1_AVAIL |
				LAYER0_AFBC |
				LAYER0_SCALER |
				LAYER0_AVAIL;
		} else if (is_meson_tm2_cpu()) {
			layer_cap =
				LAYER1_SCALER |
				LAYER1_AVAIL |
				LAYER0_AFBC |
				LAYER0_SCALER |
				LAYER0_AVAIL;
		} else {
			/* g12a, g12b, sm1 */
			layer_cap =
				LAYER1_AFBC |
				LAYER1_SCALER |
				LAYER1_AVAIL |
				LAYER0_AFBC |
				LAYER0_SCALER |
				LAYER0_AVAIL;
		}
	} else {
		if (p_amvideo->layer_support[0])
			layer_cap |= LAYER0_AVAIL;
		if (p_amvideo->layer_support[1])
			layer_cap |= LAYER1_AVAIL;
		if (p_amvideo->layer_support[2])
			layer_cap |= LAYER2_AVAIL;
		if (p_amvideo->afbc_support[0])
			layer_cap |= LAYER0_AFBC;
		if (p_amvideo->afbc_support[1])
			layer_cap |= LAYER1_AFBC;
		if (p_amvideo->afbc_support[2])
			layer_cap |= LAYER2_AFBC;
		if (p_amvideo->pps_support[0])
			layer_cap |= LAYER0_SCALER;
		/* remove the vd2 support cap for upper layer */
		if (p_amvideo->pps_support[1] &&
		    (p_amvideo->cpu_type != MESON_CPU_MAJOR_ID_T5D_REVB_ ||
		     !vd1_vd2_mux))
			layer_cap |= LAYER1_SCALER;
		if (p_amvideo->pps_support[2])
			layer_cap |= LAYER2_SCALER;
		if (p_amvideo->alpha_support[0])
			layer_cap |= LAYER0_ALPHA;
		if (p_amvideo->alpha_support[1])
			layer_cap |= LAYER1_ALPHA;
		if (p_amvideo->alpha_support[2])
			layer_cap |= LAYER2_ALPHA;
		layer_cap |= ((u32)vd_layer[0].vpp_index << LAYER0_VPP |
			(u32)vd_layer[1].vpp_index << LAYER1_VPP |
			(u32)vd_layer[2].vpp_index << LAYER2_VPP);
		if (cur_dev->mosaic_support)
			layer_cap |= MOSAIC_MODE;
	}
	pr_debug("%s cap:%x, ptype:%d\n", __func__, layer_cap, p_amvideo->cpu_type);
}

static void set_rdma_func_handler(void)
{
	cur_dev->rdma_func[0].rdma_rd =
		VSYNC_RD_MPEG_REG;
	cur_dev->rdma_func[0].rdma_wr =
		VSYNC_WR_MPEG_REG;
	cur_dev->rdma_func[0].rdma_wr_bits =
		VSYNC_WR_MPEG_REG_BITS;

	cur_dev->rdma_func[1].rdma_rd =
		VSYNC_RD_MPEG_REG_VPP1;
	cur_dev->rdma_func[1].rdma_wr =
		VSYNC_WR_MPEG_REG_VPP1;
	cur_dev->rdma_func[1].rdma_wr_bits =
		VSYNC_WR_MPEG_REG_BITS_VPP1;

	cur_dev->rdma_func[2].rdma_rd =
		VSYNC_RD_MPEG_REG_VPP2;
	cur_dev->rdma_func[2].rdma_wr =
		VSYNC_WR_MPEG_REG_VPP2;
	cur_dev->rdma_func[2].rdma_wr_bits =
		VSYNC_WR_MPEG_REG_BITS_VPP2;

	cur_dev->rdma_func[3].rdma_rd =
		PRE_VSYNC_RD_MPEG_REG;
	cur_dev->rdma_func[3].rdma_wr =
		PRE_VSYNC_WR_MPEG_REG;
	cur_dev->rdma_func[3].rdma_wr_bits =
		PRE_VSYNC_WR_MPEG_REG_BITS;
}

static int amvideom_probe(struct platform_device *pdev)
{
	int ret = 0;
	int i, j;
	int vdtemp = -1;
	const void *prop;
	int display_device_cnt = 1;
	int ex_rdma = 0;
	char propname[16];

	if (pdev->dev.of_node) {
		const struct of_device_id *match;
		struct amvideo_device_data_s *amvideo_meson;
		struct device_node	*of_node = pdev->dev.of_node;

		match = of_match_node(amlogic_amvideom_dt_match, of_node);
		if (match) {
			amvideo_meson =
				(struct amvideo_device_data_s *)match->data;
			if (amvideo_meson) {
				memcpy(&amvideo_meson_dev, amvideo_meson,
				       sizeof(struct amvideo_device_data_s));
			} else {
				pr_err("%s data NOT match\n", __func__);
				return -ENODEV;
			}
		} else {
			pr_err("%s NOT match\n", __func__);
			return -ENODEV;
		}
	}
	if (amvideo_meson_dev.cpu_type == MESON_CPU_MAJOR_ID_T3_) {
#ifndef CONFIG_AMLOGIC_C3_REMOVE
		memcpy(&amvideo_meson_dev.dev_property, &t3_dev_property,
		       sizeof(struct video_device_hw_s));
		aisr_en = 1;
		cur_dev->power_ctrl = true;
		WRITE_VCBUS_REG(VPU_AXI_CACHE, 0x11111);
		set_vpu_super_urgent_t3(FRC0_R, 3, 3);
		set_vpu_super_urgent_t3(FRC0_W, 3, 3);
		set_vpu_super_urgent_t3(FRC1_R, 2, 2);
		set_vpu_super_urgent_t3(FRC1_W, 2, 2);
		set_vpu_super_urgent_t3(FRC2_R, 3, 7);
		set_vpu_super_urgent_t3(VPU0_R, 3, 5);
		set_vpu_super_urgent_t3(VPU0_W, 3, 5);
		set_vpu_super_urgent_t3(VPU1_R, 0, 0);
		set_vpu_super_urgent_t3(VPU1_W, 0, 0);
		set_vpu_super_urgent_t3(VPU2_R, 3, 5);
	} else if (amvideo_meson_dev.cpu_type == MESON_CPU_MAJOR_ID_T5W_) {
		memcpy(&amvideo_meson_dev.dev_property, &t5w_dev_property,
		       sizeof(struct video_device_hw_s));
		cur_dev->power_ctrl = true;
#endif
	} else if (amvideo_meson_dev.cpu_type == MESON_CPU_MAJOR_ID_C3_) {
		memcpy(&amvideo_meson_dev.dev_property, &c3_dev_property,
			   sizeof(struct video_device_hw_s));
#ifndef CONFIG_AMLOGIC_C3_REMOVE
	} else if (amvideo_meson_dev.cpu_type == MESON_CPU_MAJOR_ID_S5_) {
		memcpy(&amvideo_meson_dev.dev_property, &s5_dev_property,
		       sizeof(struct video_device_hw_s));
		aisr_en = 1;
		cur_dev->power_ctrl = true;
	}  else if (amvideo_meson_dev.cpu_type == MESON_CPU_MAJOR_ID_T5M_) {
		memcpy(&amvideo_meson_dev.dev_property, &t5m_dev_property,
		       sizeof(struct video_device_hw_s));
		cur_dev->power_ctrl = true;
	} else if (amvideo_meson_dev.cpu_type == MESON_CPU_MAJOR_ID_T3X_) {
		memcpy(&amvideo_meson_dev.dev_property, &t3x_dev_property,
		       sizeof(struct video_device_hw_s));
		aisr_en = 1;
		cur_dev->power_ctrl = true;
#endif
	} else {
		memcpy(&amvideo_meson_dev.dev_property, &legcy_dev_property,
		       sizeof(struct video_device_hw_s));
	}
	if (amvideo_meson_dev.max_vd_layers > MAX_VD_LAYERS)
		return -EINVAL;

	vdtemp = of_property_read_u32(pdev->dev.of_node, "vd1_vd2_mux",
				      &vd1_vd2_mux_dts);
	if (vdtemp < 0)
		vd1_vd2_mux_dts = 0;
	set_rdma_func_handler();
	if (amvideo_meson_dev.display_module == S5_DISPLAY_MODULE) {
		video_early_init_s5(&amvideo_meson_dev);
		video_hw_init_s5();
	} else {
		video_early_init(&amvideo_meson_dev);
		video_hw_init();
	}
	prop = of_get_property(pdev->dev.of_node, "display_device_cnt", NULL);
	if (prop)
		display_device_cnt = of_read_ulong(prop, 1);
	prop = of_get_property(pdev->dev.of_node, "vpp2_layer_count", NULL);
	if (prop && display_device_cnt >= 3) {
		int layer_count;

		layer_count = of_read_ulong(prop, 1);
		if (layer_count >= 2) {
			pr_err("vpp2 only support one video layer\n");
		} else if (layer_count == 1) {
			vd_layer[2].vpp_index = VPP2;
			vd_layer_vpp[1].vpp_index = VPP2;
			vd_layer_vpp[1].layer_id = 2;
			if (video_is_meson_t7_cpu())
				vppx_vdx_mux_set();
		}
	}
	prop = of_get_property(pdev->dev.of_node, "vpp1_layer_count", NULL);
	if (prop && display_device_cnt >= 2) {
		int layer_count;

		layer_count = of_read_ulong(prop, 1);
		if (layer_count >= 2) {
			pr_err("vpp1 only support one video layer\n");
		} else if (layer_count == 1) {
			vd_layer[1].vpp_index = VPP1;
			vd_layer_vpp[0].vpp_index = VPP1;
			vd_layer_vpp[0].layer_id = 1;
			if (video_is_meson_t3x_cpu())
				vd_3mux3_set(VPP1);
		}
	}

	prop = of_get_property(pdev->dev.of_node, "low_latency", NULL);
	if (prop) {
		ex_rdma = of_read_ulong(prop, 1);
		if (ex_rdma)
			ex_vsync_rdma_register();
		else
			pr_info("ex_vsync_rdma_register function can not be used\n");
	}

	for (i = 0; i < VPP_MAX; i++) {
		snprintf(propname, sizeof(propname), "vpp%d_hold_line", i);
		prop = of_get_property(pdev->dev.of_node, propname, NULL);
		if (prop)
			vpp_hold_line[i] = of_read_ulong(prop, 1);
	}

	video_cap_set(&amvideo_meson_dev);
	video_suspend = false;
	video_suspend_cycle = 0;
	log_out = 1;

	safe_switch_videolayer(0, false, false);
	safe_switch_videolayer(1, false, false);
	safe_switch_videolayer(2, false, false);

	/* get interrupt resource */
	video_vsync = platform_get_irq_byname(pdev, "vsync");
	if (video_vsync  == -ENXIO) {
		pr_info("cannot get amvideom irq resource\n");

		return video_vsync;
	}

	pr_info("amvideom vsync irq: %d\n", video_vsync);
	if (amvideo_meson_dev.cpu_type == MESON_CPU_MAJOR_ID_S5_) {
		mosaic_frame_done = platform_get_irq_byname(pdev, "frame_done");
		if (mosaic_frame_done  == -ENXIO) {
			pr_info("cannot get amvideom frame_done irq resource\n");
			//return mosaic_frame_done;
		}
		pr_info("amvideom frame_done irq: %d\n", mosaic_frame_done);
	}
	if (amvideo_meson_dev.cpu_type == MESON_CPU_MAJOR_ID_SC2_ ||
	    amvideo_meson_dev.has_vpp1) {
		/* get interrupt resource */
		video_vsync_viu2 = platform_get_irq_byname(pdev, "vsync_viu2");
		if (video_vsync_viu2  == -ENXIO)
			pr_info("cannot get amvideom viu2 irq resource\n");
		else
			pr_info("amvideom vsync viu2 irq: %d\n",
				video_vsync_viu2);
		/* vpp1 used then register rdma channel */
		if (display_device_cnt >= 2)
			vpp1_vsync_rdma_register();
	}
	if (amvideo_meson_dev.has_vpp2) {
		/* get interrupt resource */
		video_vsync_viu3 = platform_get_irq_byname(pdev, "vsync_viu3");
		if (video_vsync_viu3  == -ENXIO)
			pr_info("cannot get amvideom viu3 irq resource\n");
		else
			pr_info("amvideom vsync viu3 irq: %d\n",
				video_vsync_viu3);
		/* vpp2 used then register rdma channel */
		if (display_device_cnt == 3)
			vpp2_vsync_rdma_register();
	}
	if (amvideo_meson_dev.dev_property.prevsync_support) {
		video_pre_vsync = platform_get_irq_byname(pdev, "pre_vsync");
		if (video_pre_vsync  == -ENXIO)
			pr_err("cannot get amvideom video_pre_vsync irq resource\n");
		else
			pr_info("amvideom video pre vsync: %d\n",
				video_pre_vsync);
		pre_vsync_rdma_register();
	}
#ifdef CONFIG_AMLOGIC_LEGACY_EARLY_SUSPEND
	register_early_suspend(&video_early_suspend_handler);
#endif
	video_keeper_init();
	for (i = 0; i < AI_SCENES_MAX; i++) {
		vpp_scenes[i].pq_scenes = (enum ai_scenes)i;
		for (j = 0; j < SCENES_VALUE; j++)
			vpp_scenes[i].pq_values[j] = vpp_pq_data[i][j];
	}
	return ret;
}

void update_aipq_data(void)
{
	int i, j;

	for (i = 0; i < AI_SCENES_MAX; i++) {
		vpp_scenes[i].pq_scenes = (enum ai_scenes)i;
		for (j = 0; j < SCENES_VALUE; j++)
			vpp_scenes[i].pq_values[j] = vpp_pq_data[i][j];
	}
}
EXPORT_SYMBOL_GPL(update_aipq_data);

static int amvideom_remove(struct platform_device *pdev)
{
#ifdef CONFIG_AMLOGIC_LEGACY_EARLY_SUSPEND
	unregister_early_suspend(&video_early_suspend_handler);
#endif
	video_keeper_exit();
	return 0;
}

static struct platform_driver amvideom_driver = {
	.probe = amvideom_probe,
	.remove = amvideom_remove,
	.driver = {
		.name = "amvideom",
		.of_match_table = amlogic_amvideom_dt_match,
	},
};

int __init video_init(void)
{
	int r = 0, i = 0;
	/*
	 *#ifdef CONFIG_ARCH_MESON1
	 *ulong clk = clk_get_rate(clk_get_sys("clk_other_pll", NULL));
	 *#elif !defined(CONFIG_ARCH_MESON3) && !defined(CONFIG_ARCH_MESON6)
	 *ulong clk = clk_get_rate(clk_get_sys("clk_misc_pll", NULL));
	 *#endif
	 */
#ifdef CONFIG_ARCH_MESON1
	no to here ulong clk =
		clk_get_rate(clk_get_sys("clk_other_pll", NULL));
#elif defined(CONFIG_ARCH_MESON2)
	not to here ulong clk =
		clk_get_rate(clk_get_sys("clk_misc_pll", NULL));
#endif
	/* #if !defined(CONFIG_ARCH_MESON3) && !defined(CONFIG_ARCH_MESON6) */

	if (platform_driver_register(&amvideom_driver)) {
		pr_info("failed to amvideom driver!\n");
		return -ENODEV;
	}

	cur_dispbuf[0] = NULL;
	cur_dispbuf2 = NULL;
	amvideo_register_client(&amvideo_notifier);

#ifdef FIQ_VSYNC
	/* enable fiq bridge */
	vsync_fiq_bridge.handle = vsync_bridge_isr;
	vsync_fiq_bridge.key = (u32)vsync_bridge_isr;
	vsync_fiq_bridge.name = "vsync_bridge_isr";

	r = register_fiq_bridge_handle(&vsync_fiq_bridge);

	if (r) {
		amlog_level(LOG_LEVEL_ERROR,
			    "video fiq bridge register error.\n");
		r = -ENOENT;
		goto err0;
	}
#endif

	r = video_attr_create();
	if (r < 0) {
		amlog_level(LOG_LEVEL_ERROR, "create video_poll class fail.\n");
#ifdef FIQ_VSYNC
		free_irq(BRIDGE_IRQ, (void *)video_dev_id);
#else
		free_irq(INT_VIU_VSYNC, (void *)video_dev_id);
#endif
		goto err1;
	}

	/* create video device */
	r = register_chrdev(AMVIDEO_MAJOR, "amvideo", &amvideo_fops);
	if (r < 0) {
		amlog_level(LOG_LEVEL_ERROR,
			    "Can't register major for amvideo device\n");
		goto err2;
	}

	r = register_chrdev(0, "amvideo_poll", &amvideo_poll_fops);
	if (r < 0) {
		amlog_level(LOG_LEVEL_ERROR,
			    "Can't register major for amvideo_poll device\n");
		goto err3;
	}

	amvideo_poll_major = r;

	amvideo_dev = device_create(amvideo_class, NULL,
				    MKDEV(AMVIDEO_MAJOR, 0),
				    NULL, DEVICE_NAME);

	if (IS_ERR(amvideo_dev)) {
		amlog_level(LOG_LEVEL_ERROR, "Can't create amvideo device\n");
		goto err4;
	}

	amvideo_poll_dev = device_create(amvideo_poll_class, NULL,
					 MKDEV(amvideo_poll_major, 0),
					 NULL, "amvideo_poll");

	if (IS_ERR(amvideo_poll_dev)) {
		amlog_level(LOG_LEVEL_ERROR,
			    "Can't create amvideo_poll device\n");
		goto err5;
	}

	init_waitqueue_head(&amvideo_trick_wait);
	init_waitqueue_head(&amvideo_prop_change_wait);

#ifdef CONFIG_AM_VOUT
	vout_hook();
	viu2_hook();
	viu3_hook();
#endif

#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
	for (i = 0; i < MAX_VD_LAYER; i++)
		memset(dispbuf_to_put[i], 0, sizeof(dispbuf_to_put[i]));
#endif
	memset(recycle_buf, 0, sizeof(recycle_buf));
	memset(recycle_cnt, 0, sizeof(recycle_cnt));

	/* render receiver is fixed to vpp port */
	gvideo_recv[0] = create_video_receiver
		("video_render.0", VFM_PATH_VIDEO_RENDER0, VPP0);
	gvideo_recv[1] = create_video_receiver
		("video_render.1", VFM_PATH_VIDEO_RENDER1, VPP0);
	gvideo_recv[2] = create_video_receiver
		("video_render.2", VFM_PATH_VIDEO_RENDER2, VPP0);
	/* for multi vpp */
	gvideo_recv_vpp[0] = create_video_receiver
		("video_render.5", VFM_PATH_VIDEO_RENDER5, VPP1);
	gvideo_recv_vpp[1] = create_video_receiver
		("video_render.6", VFM_PATH_VIDEO_RENDER6, VPP2);

	vsync_fiq_up();

	vf_receiver_init
		(&video_vf_recv, RECEIVER_NAME,
		&video_vf_receiver, NULL);
	vf_reg_receiver(&video_vf_recv);

	vf_receiver_init
		(&videopip_vf_recv, RECEIVERPIP_NAME,
		&videopip_vf_receiver, NULL);
	vf_reg_receiver(&videopip_vf_recv);

	vf_receiver_init
		(&videopip2_vf_recv, RECEIVERPIP2_NAME,
		&videopip2_vf_receiver, NULL);
	vf_reg_receiver(&videopip2_vf_recv);

	REG_PATH_CONFIGS("media.video", video_configs);
	video_debugfs_init();
	return 0;
 err5:
	device_destroy(amvideo_class, MKDEV(AMVIDEO_MAJOR, 0));
 err4:
	unregister_chrdev(amvideo_poll_major, "amvideo_poll");
 err3:
	unregister_chrdev(AMVIDEO_MAJOR, DEVICE_NAME);

 err2:
#ifdef FIQ_VSYNC
	unregister_fiq_bridge_handle(&vsync_fiq_bridge);
#endif
	class_unregister(amvideo_class);
 err1:
	class_unregister(amvideo_poll_class);
#ifdef FIQ_VSYNC
 err0:
#endif
	amvideo_unregister_client(&amvideo_notifier);
	platform_driver_unregister(&amvideom_driver);

	return r;
}

void __exit video_exit(void)
{
	video_debugfs_exit();
	vf_unreg_receiver(&video_vf_recv);
	vf_unreg_receiver(&videopip_vf_recv);
	vf_unreg_receiver(&videopip2_vf_recv);
	safe_switch_videolayer(0, false, false);
	safe_switch_videolayer(1, false, false);
	safe_switch_videolayer(2, false, false);
#ifdef CONFIG_AMLOGIC_MEDIA_SECURITY
	secure_unregister(VIDEO_MODULE);
#endif
	vsync_fiq_down();

	if (gvideo_recv[0])
		destroy_video_receiver(gvideo_recv[0]);
	gvideo_recv[0] = NULL;
	if (gvideo_recv[1])
		destroy_video_receiver(gvideo_recv[1]);
	gvideo_recv[1] = NULL;
	if (gvideo_recv[2])
		destroy_video_receiver(gvideo_recv[2]);
	gvideo_recv[2] = NULL;
	/* for multi vpp */
	if (gvideo_recv_vpp[0])
		destroy_video_receiver(gvideo_recv_vpp[0]);
	gvideo_recv_vpp[0] = NULL;
	if (gvideo_recv_vpp[1])
		destroy_video_receiver(gvideo_recv_vpp[1]);
	gvideo_recv_vpp[1] = NULL;

	device_destroy(amvideo_class, MKDEV(AMVIDEO_MAJOR, 0));
	device_destroy(amvideo_poll_class, MKDEV(amvideo_poll_major, 0));

	unregister_chrdev(AMVIDEO_MAJOR, DEVICE_NAME);
	unregister_chrdev(amvideo_poll_major, "amvideo_poll");

#ifdef FIQ_VSYNC
	unregister_fiq_bridge_handle(&vsync_fiq_bridge);
#endif

	class_unregister(amvideo_class);
	class_unregister(amvideo_poll_class);
	amvideo_unregister_client(&amvideo_notifier);
}

MODULE_PARM_DESC(debug_flag, "\n debug_flag\n");
module_param(debug_flag, int, 0664);

#ifdef TV_3D_FUNCTION_OPEN
MODULE_PARM_DESC(force_3d_scaler, "\n force_3d_scaler\n");
module_param(force_3d_scaler, uint, 0664);

MODULE_PARM_DESC(video_3d_format, "\n video_3d_format\n");
module_param(video_3d_format, uint, 0664);
#endif

module_param_named(video_vsync_enter_line_max,
	vsync_enter_line_max, int, 0664);
module_param_named(video_vsync_exit_line_max,
	vsync_exit_line_max, int, 0664);

#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
MODULE_PARM_DESC(vsync_rdma_line_max, "\n vsync_rdma_line_max\n");
module_param(vsync_rdma_line_max, int, 0664);
#endif

module_param(underflow, uint, 0664);
MODULE_PARM_DESC(underflow, "\n Underflow count\n");

module_param(next_peek_underflow, uint, 0664);
MODULE_PARM_DESC(skip, "\n Underflow count\n");

module_param(step_enable, int, 0664);
MODULE_PARM_DESC(step_enable, "\n step_enable\n");

module_param(step_flag, int, 0664);
MODULE_PARM_DESC(step_flag, "\n step_flag\n");

/*arch_initcall(video_early_init);*/

MODULE_PARM_DESC(smooth_sync_enable, "\n smooth_sync_enable\n");
module_param(smooth_sync_enable, uint, 0664);

MODULE_PARM_DESC(hdmi_in_onvideo, "\n hdmi_in_onvideo\n");
module_param(hdmi_in_onvideo, uint, 0664);

MODULE_PARM_DESC(vsync_count, "\n vsync_count\n");
module_param(vsync_count, uint, 0664);

MODULE_PARM_DESC(new_frame_count, "\n new_frame_count\n");
module_param(new_frame_count, uint, 0664);

MODULE_PARM_DESC(first_frame_toggled, "\n first_frame_toggled\n");
module_param(first_frame_toggled, uint, 0664);

MODULE_PARM_DESC(omx_pts, "\n omx_pts\n");
module_param(omx_pts, uint, 0664);

MODULE_PARM_DESC(omx_run, "\n omx_run\n");
module_param(omx_run, bool, 0664);

MODULE_PARM_DESC(omx_pts_set_index, "\n omx_pts_set_index\n");
module_param(omx_pts_set_index, uint, 0664);

MODULE_PARM_DESC(omx_version, "\n omx_version\n");
module_param(omx_version, uint, 0664);

MODULE_PARM_DESC(omx_info, "\n omx_info\n");
module_param(omx_info, uint, 0664);

MODULE_PARM_DESC(omx_need_drop_frame_num, "\n omx_need_drop_frame_num\n");
module_param(omx_need_drop_frame_num, int, 0664);

MODULE_PARM_DESC(omx_drop_done, "\n omx_drop_done\n");
module_param(omx_drop_done, bool, 0664);

MODULE_PARM_DESC(omx_pts_interval_upper, "\n omx_pts_interval\n");
module_param(omx_pts_interval_upper, int, 0664);

MODULE_PARM_DESC(omx_pts_interval_lower, "\n omx_pts_interval\n");
module_param(omx_pts_interval_lower, int, 0664);

MODULE_PARM_DESC(drop_frame_count, "\n drop_frame_count\n");
module_param(drop_frame_count, int, 0664);

MODULE_PARM_DESC(receive_frame_count, "\n receive_frame_count\n");
module_param(receive_frame_count, int, 0664);

MODULE_PARM_DESC(display_frame_count, "\n display_frame_count\n");
module_param(display_frame_count, int, 0664);

module_param(frame_detect_time, uint, 0664);
MODULE_PARM_DESC(frame_detect_time, "\n frame_detect_time\n");

module_param(frame_detect_flag, uint, 0664);
MODULE_PARM_DESC(frame_detect_flag, "\n frame_detect_flag\n");

module_param(frame_detect_fps, uint, 0664);
MODULE_PARM_DESC(frame_detect_fps, "\n frame_detect_fps\n");

module_param(frame_detect_receive_count, uint, 0664);
MODULE_PARM_DESC(frame_detect_receive_count, "\n frame_detect_receive_count\n");

module_param(frame_detect_drop_count, uint, 0664);
MODULE_PARM_DESC(frame_detect_drop_count, "\n frame_detect_drop_count\n");

MODULE_PARM_DESC(bypass_pps, "\n pps_bypass\n");
module_param(bypass_pps, bool, 0664);

MODULE_PARM_DESC(process_3d_type, "\n process_3d_type\n");
module_param(process_3d_type, uint, 0664);

MODULE_PARM_DESC(g_framepacking_support, "\n g_framepacking_support\n");
module_param(g_framepacking_support, uint, 0664);

MODULE_PARM_DESC(framepacking_width, "\n framepacking_width\n");
module_param(framepacking_width, uint, 0664);

MODULE_PARM_DESC(framepacking_height, "\n framepacking_height\n");
module_param(framepacking_height, uint, 0664);

MODULE_PARM_DESC(framepacking_blank, "\n framepacking_blank\n");
module_param(framepacking_blank, uint, 0664);

module_param(reverse, bool, 0644);
MODULE_PARM_DESC(reverse, "reverse /disable reverse");

MODULE_PARM_DESC(toggle_count, "\n toggle count\n");
module_param(toggle_count, uint, 0664);

module_param(osd_vpp1_bld_ctrl, uint, 0444);
MODULE_PARM_DESC(osd_vpp1_bld_ctrl, "osd_vpp1_bld_ctrl");
module_param(osd_vpp2_bld_ctrl, uint, 0444);
MODULE_PARM_DESC(osd_vpp2_bld_ctrl, "osd_vpp2_bld_ctrl");

//MODULE_DESCRIPTION("AMLOGIC video output driver");
//MODULE_LICENSE("GPL");
//MODULE_AUTHOR("Tim Yao <timyao@amlogic.com>");
