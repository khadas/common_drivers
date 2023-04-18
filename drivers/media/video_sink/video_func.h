/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef VIDEO_FUNC_HEADER_HH
#define VIDEO_FUNC_HEADER_HH

#if defined(CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_VECM)
#include "vpp_pq.h"
#include <linux/amlogic/media/amvecm/amvecm.h>
#endif

#define RECEIVER_NAME "amvideo"
#define RECEIVERPIP_NAME "videopip"
#define RECEIVERPIP2_NAME "videopip2"
#ifdef CONFIG_AML_VSYNC_FIQ_ENABLE
#define FIQ_VSYNC
#endif
#define ENABLE_UPDATE_HDR_FROM_USER 0
#define PTS_LOGGING

enum VPP_RECV_PATH_e {
	AMVIDEO,
	PIP1,
	PIP2,
	RENDER0,
	RENDER1,
	RENDER2
};

#ifdef FIQ_VSYNC
extern bridge_item_t vsync_fiq_bridge;
#endif

extern spinlock_t lock;

struct video_frame_aiface_s {
	u32 face_count_vd0;
	u32 face_count_vd1;
	struct face_value_t face_value[MAX_FACE_COUNT_PER_FRAME];
};

/* pts related */
#if defined(PTS_LOGGING) || defined(PTS_TRACE_DEBUG)
extern int pts_trace;
#endif
extern u32 vsync_pts_inc_scale;
extern u32 vsync_pts_inc_scale_base;

/* other avsync, frame drop etc */
extern int display_frame_count;
extern bool over_field;
extern int over_sync_count;
extern u32 toggle_count;
extern u32 timer_count;
extern u32 vsync_count;
extern bool to_notify_trick_wait;
extern atomic_t trickmode_framedone;
extern atomic_t video_prop_change;
extern unsigned int video_drop_vf_cnt[MAX_VD_LAYER];

extern int get_count_pip[MAX_VD_LAYER];
extern int get_di_count;
extern int put_di_count;
extern int di_release_count;
extern u32 vpp_hold_setting_cnt;
extern s32 black_threshold_width;
extern s32 black_threshold_height;
extern struct vframe_s hist_test_vf;
extern bool hist_test_flag;
extern int vsync_enter_line_max;
extern int vsync_exit_line_max;
extern u32 video_notify_flag;
extern struct ai_scenes_pq vpp_scenes[AI_SCENES_MAX];
extern struct nn_value_t nn_scenes_value[AI_PQ_TOP];
extern struct video_frame_aiface_s ai_face_value;
extern int vframe_walk_delay;
extern u32  video_mirror;
extern int tvin_source_type;

/* wait queue for poll */
extern wait_queue_head_t amvideo_prop_change_wait;
extern char old_vmode[32];
extern char new_vmode[32];
extern int last_mode_3d;

extern u32 performance_debug;
extern bool over_field;
extern u32 over_field_case1_cnt;
extern u32 over_field_case2_cnt;

/* display canvas */
#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
extern struct vframe_s *cur_rdma_buf[MAX_VD_LAYERS];
extern struct vframe_s *dispbuf_to_put[MAX_VD_LAYERS][DISPBUF_TO_PUT_MAX];
extern s8 dispbuf_to_put_num[MAX_VD_LAYERS];
#endif
extern struct vframe_s *recycle_buf[MAX_VD_LAYERS][1 + DISPBUF_TO_PUT_MAX];
extern s32 recycle_cnt[MAX_VD_LAYERS];
extern u32 blackout[MAX_VD_LAYERS];
extern unsigned int video_get_vf_cnt[MAX_VD_LAYER];
extern u32 pip_frame_count[MAX_VD_LAYERS];
extern u32 pip_loop;
extern u32 pip2_loop;
extern struct vframe_s *cur_dispbuf2;
extern struct vpp_frame_par_s *cur_frame_par[MAX_VD_LAYERS];

/* vpp_crc */
extern u32 vpp_crc_en;
extern int vpp_crc_result;
extern const struct vinfo_s *vinfo;
extern const char *src_fmt_str[];
extern bool go_exit;

/* ai_pq */
extern int ai_pq_disable;
extern int ai_pq_debug;
extern int ai_pq_value;
extern int ai_pq_policy;

#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
extern int dv_inst_pip;
extern bool dvel_changed;
extern u32 dvel_size;
#endif
extern atomic_t cur_over_field_state;
extern u32 config_vsync_num;
extern ulong config_timeinfo;
extern u32 new_frame_count;
extern atomic_t vt_disable_video_done;
extern atomic_t vt_unreg_flag;

struct cur_line_info_t {
	int enc_line_start;
	struct timeval start;
	struct timeval end1;
	struct timeval end2;
	struct timeval end3;
	struct timeval end4;
};

void vsync_notify(void);
int dolby_vision_need_wait(u8 path_index);
int pip_receiver_event_fun(int type, void *data, void *private_data);
int pip2_receiver_event_fun(int type, void *data, void *private_data);
inline struct vframe_s *amvideo_vf_peek(void);
inline struct vframe_s *amvideo_vf_get(void);
inline int amvideo_vf_put(struct vframe_s *vf);
int amvecm_update(u8 layer_id, u8 path_index, struct vframe_s *vf);
bool check_dispbuf(u8 layer_id, struct vframe_s *vf, bool is_put_err);
void do_frame_detect(void);
int update_video_recycle_buffer(u8 path_index);
void frame_drop_process(void);
void pts_process(void);
void release_di_buffer(int inst);
int  get_display_info(void *data);
struct vframe_s *amvideo_toggle_frame
	(s32 *vd_path_id);
struct vframe_s *get_dispbuf(u8 layer_id);
struct cur_line_info_t *get_cur_line_info(void);
inline bool is_tunnel_mode(const char *receiver_name);

void post_vsync_process(void);
void pre_vsync_process(void);

ssize_t blend_conflict_show(struct class *cla,
		struct class_attribute *attr, char *buf);

#endif
/*VIDEO_FUNC_HEADER_HH*/
