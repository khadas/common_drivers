// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
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
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/amlogic/media/vfm/vframe.h>
#include <linux/amlogic/media/vfm/vframe_provider.h>
#include <linux/amlogic/media/vfm/vframe_receiver.h>
#include <linux/amlogic/media/utils/amstream.h>
#include <linux/amlogic/media/vout/vout_notify.h>
#include <linux/amlogic/media/video_sink/video_signal_notify.h>
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
#include <asm/div64.h>

#include <linux/amlogic/media/video_sink/video_keeper.h>
#include <linux/amlogic/media/video_sink/video.h>
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
#include <linux/amlogic/media/amdolbyvision/dolby_vision.h>
#endif
#include <linux/amlogic/media/utils/amlog.h>
#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
#include "../common/rdma/rdma.h"
#endif
#include "../common/vfm/vfm.h"
#ifdef CONFIG_AMLOGIC_MEDIA_FRAME_SYNC
#include <linux/amlogic/media/frame_sync/ptsserv.h>
#include <linux/amlogic/media/frame_sync/timestamp.h>
#include <linux/amlogic/media/frame_sync/tsync.h>
#endif
#define KERNEL_ATRACE_TAG KERNEL_ATRACE_TAG_VIDEO
#include <trace/events/meson_atrace.h>
#ifdef CONFIG_AMLOGIC_MEDIA_DEINTERLACE
#include <linux/amlogic/media/di/di.h>
#include <linux/amlogic/media/di/di_interface.h>
#endif
#include "vpp_pq.h"
#if defined(CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_VECM)
#include <linux/amlogic/media/amvecm/amvecm.h>
#endif
#include <linux/amlogic/media/utils/vdec_reg.h>
#ifdef CONFIG_AMLOGIC_MEDIA_MSYNC
#include <uapi/amlogic/msync.h>
#endif
#ifdef CONFIG_AMLOGIC_MEDIA_FRC
#include <linux/amlogic/media/frc/frc_common.h>
#endif
#include <linux/amlogic/media/video_processor/video_pp_common.h>
#include "video_common.h"
#include "video_hw.h"
#include "video_hw_s5.h"
#include "vpp_post_s5.h"
#include "video_receiver.h"
#include "video_priv.h"
#include "video_reg.h"
#include "video_func.h"
#if IS_ENABLED(CONFIG_AMLOGIC_DEBUG_IOTRACE)
#include <linux/amlogic/aml_iotrace.h>
#endif

#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
#if defined(CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_VECM)
static unsigned int vpp_new_frame;
#endif
#endif
/* local var */
static u32 blend_conflict_cnt;
static u32 stop_update;
static u32 stop_force_dmc;

/* 3d related */
static unsigned int last_process_3d_type;
static bool dmc_adjust = true;
module_param_named(dmc_adjust, dmc_adjust, bool, 0644);
#ifndef CONFIG_AMLOGIC_REMOVE_OLD
static u32 dmc_config_state;
static u32 last_toggle_count;
static u32 toggle_same_count;
#endif

/* dovi related */
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
/*amvideo, videopip dv provider*/
static char dv_provider[2][32] = {"dvbldec", "dvbldec2"};
#endif

atomic_t axis_changed = ATOMIC_INIT(0);

/* display canvas */
#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
static int enable_rdma_log_count;
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
static bool pre_vsync_rdma_enable_pre;
#endif
#endif
/* 0: amvideo, 1: pip */
/* rdma buf + dispbuf + to_put_buf */
static u32 post_canvas;
static bool last_mvc_status;
static u32 pre_vsync_count;
#define MAX_VIDEO_FAKE 6
struct vd_func_s vd_fake_func[MAX_VIDEO_FAKE];
#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
static bool first_irq = true;
#endif
static struct cur_line_info_t g_cur_line_info[2];
static u8 new_frame_mask;
static bool need_force_black;
static u32 always_new_vf_cnt;

bool rdma_enable_pre;

u32 frc_mute_frames = 3;
u32 frc_muted_frames;
MODULE_PARM_DESC(frc_mute_frames, "\n frc_mute_frames\n");
module_param(frc_mute_frames, uint, 0664);

bool get_video_reverse(void)
{
	return reverse;
}
EXPORT_SYMBOL(get_video_reverse);

bool is_di_hf_y_reverse(void)
{
	if (glayer_info[0].reverse || glayer_info[0].mirror == 2)
		return cur_dev->di_hf_y_reverse;
	else
		return false;
}
EXPORT_SYMBOL(is_di_hf_y_reverse);

void get_video_axis_offset(s32 *x_offset, s32 *y_offset)
{
	s32 x_end, y_end;
	struct disp_info_s *layer = &glayer_info[0];
	const struct vinfo_s *info = get_current_vinfo();
	int orientation = 0;

	#ifdef TV_REVERSE
	orientation = screen_orientation();
	#endif

	if (!info) {
		*x_offset = 0;
		*y_offset = 0;
		return;
	}

	/* reverse and mirror case */
	if (orientation == HV_MIRROR) {
		/* reverse x/y start */
		x_end = layer->layer_left + layer->layer_width - 1;
		*x_offset = info->width - x_end - 1;
		y_end = layer->layer_top + layer->layer_height - 1;
		*y_offset = info->height - y_end - 1;
	} else if (orientation == H_MIRROR) {
		/* horizontal mirror */
		x_end = layer->layer_left + layer->layer_width - 1;
		*x_offset = info->width - x_end - 1;
		*y_offset = layer->layer_top;
	} else if (orientation == V_MIRROR) {
		/* vertical mirror */
		*x_offset = layer->layer_left;
		y_end = layer->layer_top + layer->layer_height - 1;
		*y_offset = info->height - y_end - 1;
	} else {
		*x_offset = layer->layer_left;
		*y_offset = layer->layer_top;
	}
}

/*********************************************************/

static inline struct vframe_s *pip2_vf_peek(void)
{
	struct vframe_s *cur_pipbuf2 = NULL;
	u8 path_index = 2;

	cur_pipbuf2 = cur_dispbuf[path_index];
	if (pip2_loop && cur_dispbuf[0] != cur_pipbuf2)
		return cur_dispbuf[0];
	return vf_peek(RECEIVERPIP2_NAME);
}

static inline struct vframe_s *pip2_vf_get(void)
{
	struct vframe_s *vf = NULL;
	struct vframe_s *cur_pipbuf2 = NULL;
	u8 path_index = 2;

	cur_pipbuf2 = cur_dispbuf[path_index];
	if (pip2_loop && cur_dispbuf[0] != cur_pipbuf2)
		return cur_dispbuf[0];

	vf = vf_get(RECEIVERPIP2_NAME);

	if (vf) {
		get_count_pip[path_index]++;
		if (vf->type & VIDTYPE_V4L_EOS) {
			vf_put(vf, RECEIVERPIP2_NAME);
			return NULL;
		}
		/* video_notify_flag |= VIDEO_NOTIFY_PROVIDER_GET; */
		atomic_set(&vf->use_cnt_pip, 1);
		/* atomic_set(&vf->use_cnt_pip2, 1); */
	}
	return vf;
}

static inline int pip2_vf_put(struct vframe_s *vf)
{
	struct vframe_provider_s *vfp = vf_get_provider(RECEIVERPIP2_NAME);

	if (pip2_loop)
		return 0;

	/* if (vfp && vf && atomic_dec_and_test(&vf->use_cnt_pip2)) { */
	if (vfp && vf && atomic_dec_and_test(&vf->use_cnt_pip)) {
		if (vf_put(vf, RECEIVERPIP2_NAME) < 0)
			return -EFAULT;
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
		if ((glayer_info[0].display_path_id
			== VFM_PATH_PIP2) &&
			is_amdv_enable())
			amdv_vf_put(vf);
#endif
		/* video_notify_flag |= VIDEO_NOTIFY_PROVIDER_PUT; */
	} else {
		if (vf)
			return -EINVAL;
	}
	return 0;
}

static inline struct vframe_s *pip_vf_peek(void)
{
	struct vframe_s *vf = NULL;
	struct vframe_s *cur_pipbuf = NULL;
	u8 path_index = 1;

	cur_pipbuf = cur_dispbuf[path_index];
	if (pip_loop && cur_dispbuf[0] != cur_pipbuf)
		return cur_dispbuf[0];
	vf = vf_peek(RECEIVERPIP_NAME);

#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
	/*tunnel mode, add dv_inst to vf*/
	if (vf && dv_inst_pip >= 0)
		vf->src_fmt.dv_id = dv_inst_pip;
#endif
	return vf;
}

static inline struct vframe_s *pip_vf_get(void)
{
	struct vframe_s *vf = NULL;
	struct vframe_s *cur_pipbuf = NULL;
	u8 path_index = 1;

	cur_pipbuf = cur_dispbuf[path_index];
	if (pip_loop && cur_dispbuf[0] != cur_pipbuf)
		return cur_dispbuf[0];

	vf = vf_get(RECEIVERPIP_NAME);

	if (vf) {
		get_count_pip[path_index]++;
		if (vf->type & VIDTYPE_V4L_EOS) {
			vf_put(vf, RECEIVERPIP_NAME);
			return NULL;
		}
		/* video_notify_flag |= VIDEO_NOTIFY_PROVIDER_GET; */
		atomic_set(&vf->use_cnt_pip, 1);
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
		/*tunnel mode, add dv_inst to vf*/
		if (dv_inst_pip >= 0)
			vf->src_fmt.dv_id = dv_inst_pip;
#endif
	}
	return vf;
}

static inline int pip_vf_put(struct vframe_s *vf)
{
	struct vframe_provider_s *vfp = vf_get_provider(RECEIVERPIP_NAME);

	if (pip_loop)
		return 0;

	if (vfp && vf && atomic_dec_and_test(&vf->use_cnt_pip)) {
		if (vf_put(vf, RECEIVERPIP_NAME) < 0)
			return -EFAULT;
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
		if ((glayer_info[0].display_path_id
			== VFM_PATH_PIP || is_multi_dv_mode()) &&
			is_amdv_enable())
			amdv_vf_put(vf);
#endif
		/* video_notify_flag |= VIDEO_NOTIFY_PROVIDER_PUT; */
	} else {
		if (vf)
			return -EINVAL;
	}
	return 0;
}

static inline struct vframe_s *video_vf_peek(u8 path_index)
{
	struct vframe_s *vf = NULL;

	switch (path_index) {
	case 0:
		vf = amvideo_vf_peek();
		break;
	case 1:
		vf = pip_vf_peek();
		break;
	case 2:
		vf = pip2_vf_peek();
		break;
	default:
		break;
	}
	return vf;
}

static inline struct vframe_s *video_vf_get(u8 path_index)
{
	struct vframe_s *vf = NULL;

	switch (path_index) {
	case 0:
		vf = amvideo_vf_get();
		break;
	case 1:
		vf = pip_vf_get();
		break;
	case 2:
		vf = pip2_vf_get();
		break;
	default:
		break;
	}
	return vf;
}

static inline int video_vf_put(u8 path_index, struct vframe_s *vf)
{
	int ret = -1;

	switch (path_index) {
	case 0:
		ret = amvideo_vf_put(vf);
		break;
	case 1:
		ret = pip_vf_put(vf);
		break;
	case 2:
		ret = pip2_vf_put(vf);
		break;
	default:
		break;
	}
	return ret;
}

bool check_dispbuf(u8 path_index, struct vframe_s *vf, bool is_put_err)
{
	int i;
	bool done = false;

	if (!vf)
		return true;
	for (i = 0; i < DISPBUF_TO_PUT_MAX; i++) {
		if (!is_put_err ||
		    (is_put_err && IS_DI_POSTWRTIE(vf->type))) {
			if (!dispbuf_to_put[path_index][i]) {
				dispbuf_to_put[path_index][i] = vf;
				dispbuf_to_put_num[path_index]++;
				done = true;
				break;
			}
		}
	}
	return done;
}

static struct vframe_s *pipx_toggle_frame(u8 path_index, struct vframe_s *vf)
{
	u32 first_picture = 0;
	u8 i = path_index;

	if (!vf || path_index >= MAX_VD_LAYER)
		return NULL;

	if (debug_flag & DEBUG_FLAG_PRINT_TOGGLE_FRAME)
		pr_info("%s pip%d (%p)\n", __func__, path_index - 1, vf);

	if (vf->width == 0 || vf->height == 0) {
		amlog_level(LOG_LEVEL_ERROR,
			    "Video: invalid frame dimension\n");
		return vf;
	}
	if (cur_dispbuf[i] && cur_dispbuf[i] !=
		&vf_local[i] && cur_dispbuf[i] != vf) {
#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
		int j = 0;

		if (is_vsync_rdma_enable()) {
			if (cur_rdma_buf[i] == cur_dispbuf[i]) {
				if (!check_dispbuf(i, cur_dispbuf[i], false)) {
					if (video_vf_put(i, cur_dispbuf[i]) < 0)
						pr_info("put err, line: %d\n",
							__LINE__);
				}
			} else {
				if (video_vf_put(i, cur_dispbuf[i]) < 0) {
					if (!check_dispbuf(i, cur_dispbuf[i], true))
						pr_info("put err,pip%d full\n", i - 1);
				}
			}
		} else {
			for (j = 0; j < dispbuf_to_put_num[i]; j++) {
				if (dispbuf_to_put[i][j]) {
					if (!video_vf_put(i, dispbuf_to_put[i][j])) {
						dispbuf_to_put[i][j] = NULL;
						dispbuf_to_put_num[i]--;
					}
				}
			}
			if (video_vf_put(i, cur_dispbuf[i]) < 0)
				check_dispbuf(i, cur_dispbuf[i], true);
		}
#else
		if (video_vf_put(i, cur_dispbuf[i]) < 0)
			check_dispbuf(i, cur_dispbuf[i], true);
#endif
		pip_frame_count[i]++;
		if (pip_frame_count[i] == 1)
			first_picture = 1;
	} else if (!cur_dispbuf[i] || (cur_dispbuf[i] == &vf_local[i]))
		first_picture = 1;

	if (cur_dispbuf[i] != vf)
		vf->type_backup = vf->type;

	if (first_picture && (debug_flag & DEBUG_FLAG_PRINT_TOGGLE_FRAME))
		pr_info("%s pip%d (%p)\n", __func__, i - 1, vf);
	cur_dispbuf[i] = vf;
	return cur_dispbuf[i];
}

bool tvin_vf_is_keeped(struct vframe_s *vf)
{
	struct vframe_s *src_vf;

	if (!vf)
		return false;

	if (vf->source_type != VFRAME_SOURCE_TYPE_HDMI &&
		vf->source_type != VFRAME_SOURCE_TYPE_CVBS &&
		vf->source_type != VFRAME_SOURCE_TYPE_TUNER)
		return false;

	if (!(vf->flag & VFRAME_FLAG_VIDEO_COMPOSER_BYPASS))
		return false;

	if (!vf->vc_private)
		return false;

	src_vf = vf->vc_private->src_vf;
	if (!src_vf)
		return false;

	if (src_vf->flag & VFRAME_FLAG_KEEPED)
		return true;

	return false;
}

/* for sdr/hdr/single dv switch with dual dv */
u32 last_el_status;
bool has_enhanced_layer(struct vframe_s *vf)
{
	struct provider_aux_req_s req;
	enum vframe_signal_fmt_e fmt;
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
	if (is_amdv_el_disable() &&
	    !for_amdv_certification())
		return 0;
#endif
	if (!vf)
		return 0;
	if (vf->source_type != VFRAME_SOURCE_TYPE_OTHERS)
		return 0;
	if (!is_amdv_on())
		return 0;

	fmt = get_vframe_src_fmt(vf);
	/* valid src_fmt = DOVI or invalid src_fmt will check dual layer */
	/* otherwise, it certainly is a non-dv vframe */
	if (fmt != VFRAME_SIGNAL_FMT_DOVI &&
	    fmt != VFRAME_SIGNAL_FMT_INVALID)
		return 0;

	req.vf = vf;
	req.bot_flag = 0;
	req.aux_buf = NULL;
	req.aux_size = 0;
	req.dv_enhance_exist = 0;
	vf_notify_provider_by_name
		("dvbldec",
		VFRAME_EVENT_RECEIVER_GET_AUX_DATA,
		(void *)&req);
	return req.dv_enhance_exist;
}

#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
bool dvel_status;
int dolby_vision_need_wait(u8 path_index)
{
	struct vframe_s *vf;
	int ret = 0;

	if (!is_amdv_enable())
		return 0;

	vf = video_vf_peek(path_index);
	if (!vf) {
		return 1;
	} else {
		ret = amdv_wait_metadata(vf, VD1_PATH);
		if (ret > 0)
			return ret;
	}
	return 0;
}

int dvel_swap_frame(struct vframe_s *vf)
{
	int ret = 0;
	struct video_layer_s *layer = NULL;
	struct disp_info_s *layer_info = NULL;

	/* use bl layer info */
	layer = &vd_layer[0];
	layer_info = &glayer_info[0];

	if (!is_amdv_enable()) {
		if (dvel_status) {
			//safe_switch_videolayer(1, false, true);
			dvel_status = false;
			need_disable_vd[1] = true;
		}
	} else if (vf) {
		/* enable the vd2 when the first el vframe coming */
		u32 new_dvel_w = (vf->type
			& VIDTYPE_COMPRESS) ?
			vf->compWidth :
			vf->width;

		/* check if el available first */
		if (!dvel_status) {
			dvel_changed = true;
			dvel_size = new_dvel_w;
			need_disable_vd[1] = false;
			safe_switch_videolayer(1, true, true);
		}
		/* check the el size */
		if (dvel_size != new_dvel_w)
			dvel_changed = true;
		ret = set_layer_display_canvas
			(layer, vf, layer->cur_frame_par,
			layer_info, __LINE__);
		dvel_status = true;
	} else if (dvel_status) {
		dvel_changed = true;
		dvel_status = false;
		dvel_size = 0;
		need_disable_vd[1] = true;
	}
	return ret;
}

static void amdolby_vision_proc
	(struct video_layer_s *layer_1,
	 struct vpp_frame_par_s *cur_frame_par_1,
	 struct video_layer_s *layer_2,
	 struct vpp_frame_par_s *cur_frame_par_2)
{
	static struct vframe_s *cur_dv_vf_1;/*vd1*/
	static u32 cur_frame_size_1;/*vd1*/
	struct vframe_s *disp_vf_1 = NULL;/*vd1*/
	static struct vframe_s *cur_dv_vf_2;/*vd2*/
	static u32 cur_frame_size_2;/*vd2*/
	struct vframe_s *disp_vf_2 = NULL;/*vd2*/
	u8 toggle_mode_1 = 0;
	u8 toggle_mode_2 = 0;
	u8 toggle_mode = 0;

	if (is_amdv_enable()) {
		u32 frame_size_1 = 0, h_size, v_size;
		u32 frame_size_2 = 0;
		u8 pps_state = 0; /* pps no change */

/* TODO: check if need */
#ifdef OLD_DV_FLOW
		/* force toggle when keeping frame after playing */
		if (is_local_vf(layer_1->dispbuf) &&
		    !layer_1->new_frame &&
		    is_amdv_video_on() &&
		    get_video_enabled(0)) {
			if (!amdv_parse_metadata
				(layer_1->dispbuf, VD1_PATH, 2, false, false))
				amdv_set_toggle_flag(1);
		}
		/* force toggle when keeping frame after playing */
		if (layer_2) {
			if (is_local_vf(layer_2->dispbuf) &&
			    !layer_2->new_frame &&
			    is_amdv_video_on() &&
			    get_video_enabled(0)) {
				if (!amdv_parse_metadata
					(layer_2->dispbuf, VD2_PATH, 2, false, false))
					amdv_set_toggle_flag(1);
			}
		}
#endif
		if (layer_1->new_frame)
			toggle_mode_1 = 1; /* new frame */
		else if (!layer_1->dispbuf ||
			is_local_vf(layer_1->dispbuf))
			toggle_mode_1 = 2; /* keep frame */
		else
			toggle_mode_1 = 0; /* pasue frame */

		if (layer_2 && layer_2->new_frame)
			toggle_mode_2 = 1; /* new frame */
		else if (layer_2 && (!layer_2->dispbuf ||
			is_local_vf(layer_2->dispbuf)))
			toggle_mode_2 = 2; /* keep frame */
		else
			toggle_mode_2 = 0; /* pasue frame */

		/* vd1 toggle_mode priority is high than vd2*/
		toggle_mode = toggle_mode_1 ? toggle_mode_1 : toggle_mode_2;

		/*************get vd1 disp_vf and frame_size************/

		if (layer_1->switch_vf && layer_1->vf_ext)
			disp_vf_1 = layer_1->vf_ext;
		else
			disp_vf_1 = layer_1->dispbuf;

		if (cur_frame_par_1) {
			if (layer_1->new_vpp_setting) {
				struct vppfilter_mode_s *vpp_filter =
					&cur_frame_par_1->vpp_filter;
				if (vpp_filter->vpp_hsc_start_phase_step ==
				    0x1000000 &&
				    vpp_filter->vpp_vsc_start_phase_step ==
				    0x1000000 &&
				    vpp_filter->vpp_hsc_start_phase_step ==
				    vpp_filter->vpp_hf_start_phase_step &&
				    !vpp_filter->vpp_pre_vsc_en &&
				    !vpp_filter->vpp_pre_hsc_en &&
				    !cur_frame_par_1->supsc0_enable &&
					!cur_frame_par_1->supsc1_enable &&
					layer_1->bypass_pps)
					pps_state = 2; /* pps disable */
				else
					pps_state = 1; /* pps enable */
			}
			if (cur_frame_par_1->VPP_hd_start_lines_
				>=  cur_frame_par_1->VPP_hd_end_lines_)
				h_size = 0;
			else
				h_size = cur_frame_par_1->VPP_hd_end_lines_
				- cur_frame_par_1->VPP_hd_start_lines_ + 1;
			h_size /= (cur_frame_par_1->hscale_skip_count + 1);
			if (cur_frame_par_1->VPP_vd_start_lines_
				>=  cur_frame_par_1->VPP_vd_end_lines_)
				v_size = 0;
			else
				v_size = cur_frame_par_1->VPP_vd_end_lines_
				- cur_frame_par_1->VPP_vd_start_lines_ + 1;
			v_size /=
				(cur_frame_par_1->vscale_skip_count + 1);
			frame_size_1 = (h_size << 16) | v_size;
		} else if (disp_vf_1) {
			h_size = (disp_vf_1->type & VIDTYPE_COMPRESS) ?
				disp_vf_1->compWidth : disp_vf_1->width;
			v_size = (disp_vf_1->type & VIDTYPE_COMPRESS) ?
				disp_vf_1->compHeight : disp_vf_1->height;
			frame_size_1 = (h_size << 16) | v_size;
		}

		/* trigger dv process once when stop playing */
		/* because disp_vf is not sync with video off */
		if (cur_dv_vf_1 && !disp_vf_1)
			amdv_set_toggle_flag(1);

		cur_dv_vf_1 = disp_vf_1;

		if (cur_frame_size_1 != frame_size_1) {
			cur_frame_size_1 = frame_size_1;
			amdv_set_toggle_flag(1);
		}
		/**************************************************/

		/*************get vd2 disp_vf and frame_size************/
		if (layer_2 && cur_frame_par_2) {
			if (layer_2->switch_vf && layer_2->vf_ext)
				disp_vf_2 = layer_2->vf_ext;
			else
				disp_vf_2 = layer_2->dispbuf;

			if (cur_frame_par_2) {
				if (layer_2->new_vpp_setting) {
					struct vppfilter_mode_s *vpp_filter =
						&cur_frame_par_2->vpp_filter;
					if (vpp_filter->vpp_hsc_start_phase_step ==
					    0x1000000 && vpp_filter->vpp_vsc_start_phase_step ==
					    0x1000000 && vpp_filter->vpp_hsc_start_phase_step ==
					    vpp_filter->vpp_hf_start_phase_step &&
					    !vpp_filter->vpp_pre_vsc_en &&
					    !vpp_filter->vpp_pre_hsc_en &&
					    !cur_frame_par_2->supsc0_enable &&
					    !cur_frame_par_2->supsc1_enable &&
					    layer_2->bypass_pps)
						pps_state = 2; /* pps disable */
					else
						pps_state = 1; /* pps enable */
				}
				if (cur_frame_par_2->VPP_hd_start_lines_
					>=  cur_frame_par_2->VPP_hd_end_lines_)
					h_size = 0;
				else
					h_size = cur_frame_par_2->VPP_hd_end_lines_
					- cur_frame_par_2->VPP_hd_start_lines_ + 1;
				h_size /= (cur_frame_par_2->hscale_skip_count + 1);
				if (cur_frame_par_2->VPP_vd_start_lines_
					>=  cur_frame_par_2->VPP_vd_end_lines_)
					v_size = 0;
				else
					v_size = cur_frame_par_2->VPP_vd_end_lines_
					- cur_frame_par_2->VPP_vd_start_lines_ + 1;
				v_size /=
					(cur_frame_par_2->vscale_skip_count + 1);
				frame_size_2 = (h_size << 16) | v_size;
			}

			/* trigger dv process once when stop playing */
			/* because disp_vf is not sync with video off */
			if (cur_dv_vf_2 && !disp_vf_2)
				amdv_set_toggle_flag(1);

			cur_dv_vf_2 = disp_vf_2;

			if (cur_frame_size_2 != frame_size_2) {
				cur_frame_size_2 = frame_size_2;
				amdv_set_toggle_flag(1);
			}
		}
		/*************************************************/
		if (!disp_vf_1 || is_local_vf(disp_vf_1))
			layer_1->vf_top1 = NULL;/*reset vf_top1 when video disable or keep*/
		if (is_aml_hw5())
			amdolby_vision_process_hw5
				(layer_1->vf_top1,
				 disp_vf_1, frame_size_1,
				 toggle_mode_1, pps_state);
		else
			amdolby_vision_process
				(disp_vf_1, frame_size_1,
				 disp_vf_2, frame_size_2,
				 toggle_mode_1, toggle_mode_2, pps_state);

		/*update setting according to vd1*/
		if (!is_aml_hw5())
			amdv_update_setting(disp_vf_1);
	}
}
#endif

#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
void vsync_rdma_process(void)
{
	int ret;

	ret = vsync_rdma_config();
	if (ret == 1) {
		/* rdma reset case */
		vd_layer[0].property_changed = true;
		vd_layer[1].property_changed = true;
		vd_layer[2].property_changed = true;
	}
}
#endif

#ifndef CONFIG_AMLOGIC_REMOVE_OLD
/* patch for 4k2k bandwidth issue, skiw mali and vpu mif */
static void dmc_adjust_for_mali_vpu(unsigned int width,
				    unsigned int height,
				    bool force_adjust)
{
	if (toggle_count == last_toggle_count) {
		toggle_same_count++;
	} else {
		last_toggle_count = toggle_count;
		toggle_same_count = 0;
	}
	/*avoid 3840x2160 crop*/
	if (width >= 2000 && height >= 1400 &&
	    ((dmc_config_state != 1 && toggle_same_count < 30) ||
		force_adjust)) {
		if (0) {/* if (is_dolby_vision_enable()) { */
			/* vpu dmc */
			WRITE_DMCREG
				(DMC_AM0_CHAN_CTRL,
				0x85f403f4);
			WRITE_DMCREG
				(DMC_AM1_CHAN_CTRL,
				0x85f403f4);
			WRITE_DMCREG
				(DMC_AM2_CHAN_CTRL,
				0x85f403f4);

			/* mali dmc */
			WRITE_DMCREG
				(DMC_AXI1_CHAN_CTRL,
				0xff10ff4);
			WRITE_DMCREG
				(DMC_AXI2_CHAN_CTRL,
				0xff10ff4);
			WRITE_DMCREG
				(DMC_AXI1_HOLD_CTRL,
				0x08040804);
			WRITE_DMCREG
				(DMC_AXI2_HOLD_CTRL,
				0x08040804);
		} else {
			/* vpu dmc */
			WRITE_DMCREG
				(DMC_AM1_CHAN_CTRL,
				0x43028);
			WRITE_DMCREG
				(DMC_AM1_HOLD_CTRL,
				0x18101818);
			WRITE_DMCREG
				(DMC_AM3_CHAN_CTRL,
				0x85f403f4);
			WRITE_DMCREG
				(DMC_AM4_CHAN_CTRL,
				0x85f403f4);
			/* mali dmc */
			WRITE_DMCREG
				(DMC_AXI1_HOLD_CTRL,
				0x10080804);
			WRITE_DMCREG
				(DMC_AXI2_HOLD_CTRL,
				0x10080804);
		}
		dmc_config_state = 1;
	} else if (((toggle_same_count >= 30) ||
		((width < 2000) && (height < 1400))) &&
		(dmc_config_state != 2)) {
		/* vpu dmc */
		WRITE_DMCREG
			(DMC_AM0_CHAN_CTRL,
			0x8FF003C4);
		WRITE_DMCREG
			(DMC_AM1_CHAN_CTRL,
			0x3028);
		WRITE_DMCREG
			(DMC_AM1_HOLD_CTRL,
			0x18101810);
		WRITE_DMCREG
			(DMC_AM2_CHAN_CTRL,
			0x8FF003C4);
		WRITE_DMCREG
			(DMC_AM2_HOLD_CTRL,
			0x3028);
		WRITE_DMCREG
			(DMC_AM3_CHAN_CTRL,
			0x85f003f4);
		WRITE_DMCREG
			(DMC_AM4_CHAN_CTRL,
			0x85f003f4);

		/* mali dmc */
		WRITE_DMCREG
			(DMC_AXI1_CHAN_CTRL,
			0x8FF00FF4);
		WRITE_DMCREG
			(DMC_AXI2_CHAN_CTRL,
			0x8FF00FF4);
		WRITE_DMCREG
			(DMC_AXI1_HOLD_CTRL,
			0x18101810);
		WRITE_DMCREG
			(DMC_AXI2_HOLD_CTRL,
			0x18101810);
		toggle_same_count = 30;
		dmc_config_state = 2;
	}
}
#endif

bool black_threshold_check(u8 id)
{
	struct video_layer_s *layer = NULL;
	struct disp_info_s *layer_info = NULL;
	struct vpp_frame_par_s *frame_par = NULL;
	bool ret = false;

	if (id >= MAX_VD_LAYERS)
		return ret;

	if (black_threshold_width <= 0 ||
	    black_threshold_height <= 0)
		return ret;

	layer = &vd_layer[id];
	layer_info = &glayer_info[id];
	if (layer_info->layer_top == 0 &&
	    layer_info->layer_left == 0 &&
	    layer_info->layer_width <= 1 &&
	    layer_info->layer_height <= 1)
		/* special case to do full screen display */
		return ret;

	frame_par = layer->cur_frame_par;
	if (frame_par &&
	    (frame_par->VPP_pic_in_height_ <= 10 ||
	     frame_par->VPP_line_in_length_ <= 10))
		return true;
	if (layer_info->layer_width <= black_threshold_width ||
	    layer_info->layer_height <= black_threshold_height) {
		if (frame_par &&
		    frame_par->vscale_skip_count == 8 &&
		    frame_par->hscale_skip_count == 1)
			ret = true;
	}
	return ret;
}

bool black_threshold_check_s5(u8 id)
{
	struct video_layer_s *layer = NULL;
	struct disp_info_s *layer_info = NULL;
	struct vpp_frame_par_s *frame_par = NULL;
	u32 black_threshold_width_s5 = 0;
	u32 black_threshold_height_s5 = 0;
	bool ret = false;
	const struct vinfo_s *vinfo = get_current_vinfo();

	if (id >= MAX_VD_LAYERS)
		return ret;

	black_threshold_width_s5 = black_threshold_width;
	black_threshold_height_s5 = black_threshold_width;

	/* check output */
	if (vinfo) {
		/* output: (4k-8k] */
		if (vinfo->width >= 3840 && vinfo->height >= 2160) {
			black_threshold_width_s5 = black_threshold_width * 4;
			black_threshold_height_s5 = black_threshold_height * 4 + 20;
		} else if ((vinfo->width >= 1920 && vinfo->height >= 1080 &&
			(vinfo->sync_duration_num /
			vinfo->sync_duration_den > 60))) {
			/* 1080p 120hz - 4k 120hz */
			black_threshold_width_s5 = black_threshold_width * 2;
			black_threshold_height_s5 = black_threshold_height * 2;
		} else {
			black_threshold_width_s5 = black_threshold_width;
			black_threshold_height_s5 = black_threshold_height;
		}
	}
	if (black_threshold_width_s5 <= 0 ||
	    black_threshold_height_s5 <= 0)
		return ret;

	layer = &vd_layer[id];
	layer_info = &glayer_info[id];
	if (layer_info->layer_top == 0 &&
	    layer_info->layer_left == 0 &&
	    layer_info->layer_width <= 1 &&
	    layer_info->layer_height <= 1)
		/* special case to do full screen display */
		return ret;

	frame_par = layer->cur_frame_par;
	if (frame_par &&
	    (frame_par->VPP_pic_in_height_ <= 10 ||
	     frame_par->VPP_line_in_length_ <= 10))
		return true;
	if (layer_info->layer_width <= black_threshold_width_s5 ||
	    layer_info->layer_height <= black_threshold_height_s5) {
		if (frame_par &&
		    frame_par->vscale_skip_count == 8 &&
		    frame_par->hscale_skip_count == 1)
			ret = true;
	}
	return ret;
}

void update_vppx_property(u8 layer_id)
{
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
	/* vpp1 case */
	if (vd_layer_vpp[0].layer_id == layer_id &&
		vd_layer_vpp[0].vpp_index == VPP1)
		vd_layer_vpp[0].property_changed = true;
		/* vpp2 case */
	else if (vd_layer_vpp[1].layer_id == layer_id &&
		vd_layer_vpp[1].vpp_index == VPP2)
		vd_layer_vpp[1].property_changed = true;
#endif
}

void _set_video_crop(struct disp_info_s *layer, int *p)
{
	int last_l, last_r, last_t, last_b;
	int new_l, new_r, new_t, new_b;

	if (!layer)
		return;

	last_t = layer->crop_top;
	last_l = layer->crop_left;
	last_b = layer->crop_bottom;
	last_r = layer->crop_right;

	layer->crop_top = p[0];
	layer->crop_left = p[1];
	layer->crop_bottom = p[2];
	layer->crop_right = p[3];

	new_t = layer->crop_top;
	new_l = layer->crop_left;
	new_b = layer->crop_bottom;
	new_r = layer->crop_right;
	if (new_t != last_t || new_l != last_l ||
	    new_b != last_b || new_r != last_r) {
		if (layer->layer_id == 0) {
			vd_layer[0].property_changed = true;
		} else if (layer->layer_id == 1) {
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
}

void _set_video_window(struct disp_info_s *layer, int *p)
{
	int w, h;
	int *parsed = p;
	int last_x, last_y, last_w, last_h;
	int new_x, new_y, new_w, new_h;
	const struct vinfo_s *info = get_current_vinfo();

	if (!layer)
		return;
	if (!info || info->mode == VMODE_INVALID)
		return;

	/* move the invert logic to vpp.c */
#ifdef TMP_DISABLE /* TV_REVERSE */
	if (reverse || video_mirror) {
		int temp, temp1;

		temp = parsed[0];
		temp1 = parsed[1];
		if (get_osd_reverse() & 1) {
			parsed[0] = info->width - parsed[2] - 1;
			parsed[2] = info->width - temp - 1;
		}
		if (get_osd_reverse() & 2) {
			parsed[1] = info->height - parsed[3] - 1;
			parsed[3] = info->height - temp1 - 1;
		}
	}
#endif

	last_x = layer->layer_left;
	last_y = layer->layer_top;
	last_w = layer->layer_width;
	last_h = layer->layer_height;

	if (parsed[0] < 0 && parsed[2] < 2) {
		parsed[2] = 2;
		parsed[0] = 0;
	}
	if (parsed[1] < 0 && parsed[3] < 2) {
		parsed[3] = 2;
		parsed[1] = 0;
	}
	w = parsed[2] - parsed[0] + 1;
	h = parsed[3] - parsed[1] + 1;

	if (w > 0 && h > 0) {
		if (w == 1 && h == 1) {
			w = 0;
			h = 0;
		}
		layer->layer_left = parsed[0];
		layer->layer_top = parsed[1];
		layer->layer_width = w;
		layer->layer_height = h;
	}

	new_x = layer->layer_left;
	new_y = layer->layer_top;
	new_w = layer->layer_width;
	new_h = layer->layer_height;
	vpp_trace_axis(layer->layer_left, layer->layer_top,
		layer->layer_left + layer->layer_width - 1,
		layer->layer_top + layer->layer_height - 1);

	if (last_x != new_x || last_y != new_y ||
	    last_w != new_w || last_h != new_h) {
		if (layer->layer_id == 0) {
			atomic_set(&axis_changed, 1);
			vd_layer[0].property_changed = true;
		} else if (layer->layer_id == 1) {
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
		if (debug_flag & DEBUG_FLAG_TRACE_EVENT)
			pr_info("VD%d axis changed: %d %d (%d %d)->%d %d (%d %d)\n",
				layer->layer_id + 1,
				last_x, last_y, last_w, last_h,
				new_x, new_y, new_w, new_h);
	}
}

void _set_video_mirror(struct disp_info_s *layer, int mirror)
{
	int last_mirror, new_mirror;
	int last_reverse, new_reverse;
	bool revser_temp = false;

	if (!layer)
		return;

	/* 'reverse' and 'video_mirror' are not enabled at the same time */
#ifdef TV_REVERSE
	if (reverse) {
		revser_temp = true;
		if (mirror == H_MIRROR) {
			mirror = V_MIRROR;
			revser_temp = false;
		} else if (mirror == V_MIRROR) {
			mirror = H_MIRROR;
			revser_temp = false;
		} else if (mirror == HV_MIRROR) {
			mirror = NO_MIRROR;
			revser_temp = false;
		}
	}
#endif
	if (video_mirror == NO_MIRROR) {
		switch (mirror) {
		case NO_MIRROR:
		case H_MIRROR:
		case V_MIRROR:
			break;
		case HV_MIRROR:
			mirror = NO_MIRROR;
			revser_temp = true;
			break;
		}
	} else if (video_mirror == H_MIRROR) {
		switch (mirror) {
		case NO_MIRROR:
			mirror = H_MIRROR;
			break;
		case H_MIRROR:
			mirror = NO_MIRROR;
			break;
		case V_MIRROR:
			mirror = NO_MIRROR;
			revser_temp = true;
			break;
		case HV_MIRROR:
			mirror = V_MIRROR;
			break;
		}
	} else if (video_mirror == V_MIRROR) {
		switch (mirror) {
		case NO_MIRROR:
			mirror = V_MIRROR;
			break;
		case H_MIRROR:
			mirror = NO_MIRROR;
			revser_temp = true;
			break;
		case V_MIRROR:
			mirror = NO_MIRROR;
			break;
		case HV_MIRROR:
			mirror = H_MIRROR;
			break;
		}
	}

	last_mirror = layer->mirror;
	new_mirror = mirror;
	last_reverse = layer->reverse;
	new_reverse = revser_temp;
	layer->mirror = mirror;
	layer->reverse = revser_temp;

	if (last_mirror != new_mirror ||
		last_reverse != new_reverse) {
		if (layer->layer_id == 0) {
			vd_layer[0].property_changed = true;
			if (debug_flag & DEBUG_FLAG_TRACE_EVENT)
				pr_info("VD1 mirror changed: %d ->%d\n",
					last_mirror, new_mirror);
		} else if (layer->layer_id == 1) {
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
}

void set_video_crop_ext(int layer_index, int *p)
{
	struct disp_info_s *layer = &glayer_info[layer_index];

	_set_video_crop(layer, p);
}

void set_video_window_ext(int layer_index, int *p)
{
	struct disp_info_s *layer = &glayer_info[layer_index];

	if (!(debug_flag & DEBUG_FLAG_AXIS_NO_UPDATE))
		_set_video_window(layer, p);
}

void set_video_zorder_ext(int layer_index, int zorder)
{
	struct disp_info_s *layer = &glayer_info[layer_index];

	if (layer->zorder != zorder) {
		layer->zorder = zorder;
		if (layer->layer_id == 0) {
			vd_layer[0].property_changed = true;
		} else if (layer->layer_id == 1) {
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
}

void  get_video_input_info(struct video_input_info *input_info)
{
	if (!cur_frame_par[0])
		return;
	input_info->height = cur_frame_par[0]->video_input_h;
	input_info->width = cur_frame_par[0]->video_input_w;
	input_info->crop_top = cur_frame_par[0]->crop_top;
	input_info->crop_bottom = cur_frame_par[0]->crop_bottom;
	input_info->crop_left = cur_frame_par[0]->crop_left;
	input_info->crop_right = cur_frame_par[0]->crop_right;
}

static void vdx_force_black(u8 layer_id)
{
	if (layer_id == 0) {
		if (vd_layer[0].dispbuf &&
			(vd_layer[0].dispbuf->flag & VFRAME_FLAG_FAKE_FRAME ||
			need_force_black)) {
			if ((vd_layer[0].force_black &&
				!(debug_flag & DEBUG_FLAG_NO_CLIP_SETTING)) ||
				!vd_layer[0].force_black) {
				/*default YUV */
				vd_layer[0].clip_setting.clip_max =
					(0x0 << 20) | (0x200 << 10) | 0x200;
				vd_layer[0].clip_setting.clip_min =
					vd_layer[0].clip_setting.clip_max;
				if (!cpu_after_eq(MESON_CPU_MAJOR_ID_T7) ||
					cur_dev->display_module == OLD_DISPLAY_MODULE) {
					if (vd_layer[0].dispbuf->type & VIDTYPE_RGB_444) {
						/* chip before t7 vd1 hdr core after vd1 clip */
						/* RGB */
						vd_layer[0].clip_setting.clip_max =
							(0x0 << 20) | (0x0 << 10) | 0;
						vd_layer[0].clip_setting.clip_min =
							vd_layer[0].clip_setting.clip_max;
					}
				}
				vd_layer[0].clip_setting.clip_done = false;
			}
			if (!vd_layer[0].force_black) {
				pr_debug("vsync: vd1 force black\n");
				vd_layer[0].force_black = true;
			}
		} else if (vd_layer[0].force_black) {
			pr_debug("vsync: vd1 black to normal\n");
			vd_layer[0].clip_setting.clip_max =
				(0x3ff << 20) | (0x3ff << 10) | 0x3ff;
			vd_layer[0].clip_setting.clip_min = 0;
			vd_layer[0].clip_setting.clip_done = false;
			vd_layer[0].force_black = false;
		}
	} else {
		if (vd_layer[layer_id].dispbuf &&
		   (vd_layer[layer_id].dispbuf->flag & VFRAME_FLAG_FAKE_FRAME))
			safe_switch_videolayer(layer_id, false, true);
	}
}

void pipx_swap_frame(struct video_layer_s *layer, struct vframe_s *vf,
			  const struct vinfo_s *vinfo)
{
	struct disp_info_s *layer_info;
	int axis[4];
	int crop[4];
	u8 layer_id = layer->layer_id;

	if (!vf || layer_id >= MAX_VD_LAYER)
		return;

	layer_info = &glayer_info[layer_id];
	if ((vf->flag & (VFRAME_FLAG_VIDEO_COMPOSER |
	    VFRAME_FLAG_VIDEO_DRM)) &&
	    !(debug_flag & DEBUG_FLAG_AXIS_NO_UPDATE)) {
		int mirror = 0;

		axis[0] = vf->axis[0];
		axis[1] = vf->axis[1];
		axis[2] = vf->axis[2];
		axis[3] = vf->axis[3];
		crop[0] = vf->crop[0];
		crop[1] = vf->crop[1];
		crop[2] = vf->crop[2];
		crop[3] = vf->crop[3];
		_set_video_window(layer_info, axis);
		_set_video_crop(layer_info, crop);
		if (vf->flag & VFRAME_FLAG_MIRROR_H)
			mirror = H_MIRROR;
		if (vf->flag & VFRAME_FLAG_MIRROR_V)
			mirror |= V_MIRROR;
		_set_video_mirror(layer_info, mirror);
		set_alpha_scpxn(layer, vf->composer_info);
		layer_info->zorder = vf->zorder;
	} else {
		_set_video_mirror(layer_info, 0);
	}

	layer_swap_frame(vf, layer, false, vinfo, 0);

	/* FIXME: free correct keep frame */
	if (!is_local_vf(layer->dispbuf))
		video_keeper_new_frame_notify(layer->keep_frame_id);
	fgrain_update_table(layer, vf);
	if (stop_update)
		layer->new_vpp_setting = false;
}

void primary_swap_frame(struct video_layer_s *layer,
				       struct vframe_s *vf1,
				       int line)
{
	bool vf_with_el = false;
	bool force_toggle = false;
	int ret = 0;
	struct disp_info_s *layer_info = NULL;
	int axis[4];
	int crop[4];
	struct vframe_s *vf;
#ifdef CONFIG_AMLOGIC_MEDIA_DEINTERLACE
	u32 vpp_index = layer->vpp_index;
#endif

	ATRACE_COUNTER(__func__,  line);

	if (!vf1)
		return;

	vf = vf1;
	layer_info = &glayer_info[0];
#ifdef CONFIG_AMLOGIC_MEDIA_DEINTERLACE
	if (layer->need_switch_vf && IS_DI_POST(vf->type) &&
	    dil_get_diff_ver_flag() == DI_DRV_DEINTERLACE) {
		if ((vf1->flag & VFRAME_FLAG_DOUBLE_FRAM) &&
		    is_di_post_mode(vf1)) {
#ifdef CONFIG_AMLOGIC_MEDIA_DEINTERLACE
			if (di_api_get_instance_id() == vf1->di_instance_id) {
				layer->need_switch_vf = false;
				pr_info("set need_switch_vf false\n");
			} else {
				vf = vf1->vf_ext;
				layer->do_switch = true;
				di_api_post_disable();
			}
#endif
		} else {
			layer->need_switch_vf = false;
		}
	}
#endif
	if ((vf->flag & (VFRAME_FLAG_VIDEO_COMPOSER |
	    VFRAME_FLAG_VIDEO_DRM)) &&
	    !(vf->flag & VFRAME_FLAG_FAKE_FRAME) &&
	    !(debug_flag & DEBUG_FLAG_AXIS_NO_UPDATE)) {
		int mirror = 0;

		axis[0] = vf->axis[0];
		axis[1] = vf->axis[1];
		axis[2] = vf->axis[2];
		axis[3] = vf->axis[3];
		crop[0] = vf->crop[0];
		crop[1] = vf->crop[1];
		crop[2] = vf->crop[2];
		crop[3] = vf->crop[3];
		_set_video_window(&glayer_info[0], axis);
		if (vf->source_type != VFRAME_SOURCE_TYPE_HDMI &&
			vf->source_type != VFRAME_SOURCE_TYPE_CVBS &&
			vf->source_type != VFRAME_SOURCE_TYPE_TUNER &&
			vf->source_type != VFRAME_SOURCE_TYPE_HWC)
			_set_video_crop(&glayer_info[0], crop);
		if (vf->flag & VFRAME_FLAG_MIRROR_H)
			mirror = H_MIRROR;
		if (vf->flag & VFRAME_FLAG_MIRROR_V)
			mirror |= V_MIRROR;
		_set_video_mirror(&glayer_info[0], mirror);
		set_alpha_scpxn(layer, vf->composer_info);
		glayer_info[0].zorder = vf->zorder;
	} else {
		_set_video_mirror(&glayer_info[0], 0);
	}

	if (layer->layer_id == 0 &&
	    !(vf->type & VIDTYPE_COMPRESS) &&
	    layer_info->need_no_compress) {
		atomic_sub(1, &gafbc_request);
		layer_info->need_no_compress = false;
		force_toggle = true;
	}

	if (is_amdv_enable())
		vf_with_el = has_enhanced_layer(vf);

	/* FIXME: need check the pre seq */
	if (vf->early_process_fun) {
		if (vf->early_process_fun(vf->private_data, vf) == 1)
			force_toggle = true;
	} else {
#ifdef CONFIG_AMLOGIC_MEDIA_DEINTERLACE
		/* FIXME: is_di_on */
		if (is_di_post_mode(vf)) {
			/* check mif enable status, disable post di */
			cur_dev->rdma_func[vpp_index].rdma_wr
				(DI_POST_CTRL, 0x3 << 30);
			cur_dev->rdma_func[vpp_index].rdma_wr
				(DI_POST_SIZE,
				(32 - 1) | ((128 - 1) << 16));
			cur_dev->rdma_func[vpp_index].rdma_wr
				(DI_IF1_GEN_REG,
				READ_VCBUS_REG(DI_IF1_GEN_REG) &
				0xfffffffe);
		}
#endif
	}

	if (last_process_3d_type != process_3d_type ||
	    last_el_status != vf_with_el)
		force_toggle = true;

	if (!layer->dispbuf ||
	    (vf->type & VIDTYPE_COMPRESS &&
	     (layer->dispbuf->compWidth != vf->compWidth ||
	      layer->dispbuf->compHeight != vf->compHeight)) ||
	    (layer->dispbuf->width != vf->width ||
	     layer->dispbuf->height != vf->height)) {
		video_prop_status |= VIDEO_PROP_CHANGE_SIZE;
		if (debug_flag & DEBUG_FLAG_TRACE_EVENT)
			pr_info("VD1 src size changed: %dx%x->%dx%d. cur:%p, new:%p\n",
				layer->dispbuf ? layer->dispbuf->width : 0,
				layer->dispbuf ? layer->dispbuf->height : 0,
				vf->width, vf->height,
				layer->dispbuf, vf);
	}

	/* switch buffer */
	post_canvas = vf->canvas0Addr;
	ret = layer_swap_frame
		(vf, layer, force_toggle, vinfo,
		cur_dispbuf2 ? OP_HAS_DV_EL : 0);
	if (ret >= vppfilter_success) {
		amlog_mask
			(LOG_MASK_FRAMEINFO,
			"%s %dx%d  ar=0x%x\n",
			((vf->type & VIDTYPE_TYPEMASK) ==
			VIDTYPE_INTERLACE_TOP) ? "interlace-top"
			: ((vf->type & VIDTYPE_TYPEMASK)
			== VIDTYPE_INTERLACE_BOTTOM)
			? "interlace-bottom" : "progressive", vf->width,
			vf->height, vf->ratio_control);
#if defined(TV_3D_FUNCTION_OPEN) && defined(CONFIG_AMLOGIC_MEDIA_TVIN)
		amlog_mask
			(LOG_MASK_FRAMEINFO,
			"%s trans_fmt=%u\n", __func__, vf->trans_fmt);

#endif
	}
	if (ret == vppfilter_changed_but_hold) {
		video_notify_flag |=
			VIDEO_NOTIFY_NEED_NO_COMP;
		vpp_hold_setting_cnt++;
		if (layer->global_debug & DEBUG_FLAG_BASIC_INFO)
			pr_info("toggle_frame vpp hold setting cnt: %d\n",
				vpp_hold_setting_cnt);
	} else {/* apply new vpp settings */
		if (layer->next_frame_par->vscale_skip_count <= 1 &&
		    vf->type_original & VIDTYPE_SUPPORT_COMPRESS &&
		    !(vf->type_original & VIDTYPE_COMPRESS)) {
			video_notify_flag |=
				VIDEO_NOTIFY_NEED_NO_COMP;
			if (layer->global_debug & DEBUG_FLAG_BASIC_INFO)
				pr_info("disable no compress mode\n");
		}
		vpp_hold_setting_cnt = 0;
	}
	last_process_3d_type = process_3d_type;

	/* if el is unnecessary, afbc2 need to be closed */
	if (last_el_status == 1 && vf_with_el == 0)
		need_disable_vd[1] = true;
	last_el_status = vf_with_el;

	if (((vf->type & VIDTYPE_MVC) == 0) && last_mvc_status)
		need_disable_vd[1] = true;

	if (vf->type & VIDTYPE_MVC)
		last_mvc_status = true;
	else
		last_mvc_status = false;

	/* FIXME: free correct keep frame */
	if (!is_local_vf(layer->dispbuf) && !layer->do_switch)
		video_keeper_new_frame_notify(layer->keep_frame_id);
	fgrain_update_table(layer, vf);
	if (stop_update)
		layer->new_vpp_setting = false;
	ATRACE_COUNTER(__func__,  0);
}

s32 primary_render_frame(struct video_layer_s *layer,
					const struct vinfo_s *vinfo)
{
	struct vpp_frame_par_s *frame_par;
	bool force_setting = false;
	struct scaler_setting_s local_vd2_pps = {0};
	struct blend_setting_s local_vd2_blend = {0};
	struct mif_pos_s local_vd2_mif = {0};
	bool update_vd2 = false;
	struct vframe_s *dispbuf = NULL;
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
	int pq_process_debug[4];
	int count = 0, k = 0, l = 0;
#endif
	int ret = 0;
	u8 vpp_index;

	vpp_index = layer->vpp_index;
#ifdef CONFIG_AMLOGIC_MEDIA_FRC
	if (cur_dev->vsync_2to1_enable && vsync_count_start) {
		/* not prevsync not handle */
		if (!frc_drv_get_1st_frm())
			return 0;
	}
#endif
	local_vd2_mif.p_vd_mif_reg = &vd_layer[1].vd_mif_reg;
	local_vd2_mif.p_vd_afbc_reg = &vd_layer[1].vd_afbc_reg;
	/* filter setting management */
	if (layer->new_vpp_setting) {
		layer->cur_frame_par = layer->next_frame_par;
		cur_frame_par[0] = layer->cur_frame_par;
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
		if (layer->mosaic_mode) {
			int i;
			struct mosaic_frame_s *mosaic_frame;
			struct video_layer_s *virtual_layer;

			for (i = 0; i < SLICE_NUM; i++) {
				mosaic_frame = get_mosaic_vframe_info(i);
				virtual_layer = &mosaic_frame->virtual_layer;
				if (!virtual_layer)
					return -1;
				virtual_layer->cur_frame_par =
					virtual_layer->next_frame_par;
			}
		}
#endif
	}
	if (glayer_info[layer->layer_id].fgrain_force_update) {
		force_setting = true;
		glayer_info[layer->layer_id].fgrain_force_update = false;
	}
	frame_par = layer->cur_frame_par;
	if (layer->switch_vf && layer->vf_ext)
		dispbuf = layer->vf_ext;
	else
		dispbuf = layer->dispbuf;

#ifdef ENABLE_PRE_LINK
	if (is_pre_link_on(layer) &&
	    dispbuf && !is_local_vf(dispbuf)) {
		int iret;
		struct pvpp_dis_para_in_s di_in_p;

		if (layer->prelink_skip_cnt == 0) {
			memset(&di_in_p, 0, sizeof(struct pvpp_dis_para_in_s));
			di_in_p.win.x_st = layer->cur_frame_par->VPP_hd_start_lines_;
			di_in_p.win.x_end = layer->cur_frame_par->VPP_hd_end_lines_;
			di_in_p.win.y_st = layer->cur_frame_par->VPP_vd_start_lines_;
			di_in_p.win.y_end = layer->cur_frame_par->VPP_vd_end_lines_;
			di_in_p.vinfo.x_d_st = layer->cur_frame_par->VPP_hsc_startp;
			di_in_p.vinfo.x_d_end = layer->cur_frame_par->VPP_hsc_endp;
			di_in_p.vinfo.y_d_st = layer->cur_frame_par->VPP_vsc_startp;
			di_in_p.vinfo.y_d_end = layer->cur_frame_par->VPP_vsc_endp;
			di_in_p.vinfo.x_d_size = di_in_p.vinfo.x_d_end - di_in_p.vinfo.x_d_st + 1;
			di_in_p.vinfo.y_d_size = di_in_p.vinfo.y_d_end - di_in_p.vinfo.y_d_st + 1;
			di_in_p.win.x_size = di_in_p.win.x_end - di_in_p.win.x_st + 1;
			di_in_p.win.y_size = di_in_p.win.y_end - di_in_p.win.y_st + 1;
			di_in_p.vinfo.htotal = vinfo->htotal;
			di_in_p.vinfo.vtotal = vinfo->vtotal;
			di_in_p.vinfo.height = vinfo->height;
			di_in_p.vinfo.width = vinfo->width;
			di_in_p.vinfo.frequency = vinfo->std_duration;
			di_in_p.plink_reverse = glayer_info[0].reverse;
			di_in_p.plink_hv_mirror = glayer_info[0].mirror;
			di_in_p.dmode = EPVPP_DISPLAY_MODE_NR;
			di_in_p.unreg_bypass = 0;
			di_in_p.follow_hold_line = vpp_hold_line[vpp_index];
			iret = pvpp_display(dispbuf, &di_in_p, NULL);
			if (iret <= 0) {
				vd_layer[0].property_changed = true;
				vd_layer[0].prelink_bypass_check = true;
				if (layer->global_debug & DEBUG_FLAG_PRELINK)
					pr_info("VIDEO: pvpp_display fail: %d\n", iret);
			} else if (layer->global_debug & DEBUG_FLAG_PRELINK_MORE) {
				pr_info("do di callback iret:%d\n", iret);
			}
		} else {
			layer->prelink_skip_cnt--;
		}
	} else if (is_pre_link_on(layer) &&
			layer->need_disable_prelink && !dispbuf) {
		int iret;
		struct pvpp_dis_para_in_s di_in_p;

		/* no keep buffer after unreg case */
		memset(&di_in_p, 0, sizeof(struct pvpp_dis_para_in_s));
		di_in_p.dmode = EPVPP_DISPLAY_MODE_BYPASS;
		di_in_p.unreg_bypass = 1;
		di_in_p.follow_hold_line = vpp_hold_line[vpp_index];
		iret = pvpp_display(NULL, &di_in_p, NULL);
		if (layer->global_debug & DEBUG_FLAG_PRELINK)
			pr_info("%s: unreg_bypass pre-link mode ret %d\n", __func__, iret);
		layer->pre_link_en = false;
		layer->prelink_bypass_check = false;
		layer->prelink_skip_cnt = 0;
		iret = pvpp_sw(false);
		if (layer->global_debug & DEBUG_FLAG_PRELINK)
			pr_info("%s: Disable pre-link mode ret %d\n", __func__, iret);
		atomic_set(&vd_layer[0].disable_prelink_done, 1);
	}
#endif

	/* process cur frame for each vsync */
	if (dispbuf) {
		int need_afbc =
			(dispbuf->type & VIDTYPE_COMPRESS);
		int afbc_need_reset =
			(layer->enabled && need_afbc &&
			 !is_afbc_enabled(layer->layer_id));

		/*video on && afbc is off && is compress frame.*/
		if (layer->new_vpp_setting || afbc_need_reset)
			vd_set_dcu
				(layer->layer_id, layer,
				frame_par, dispbuf);
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
		if (cur_dispbuf2 &&
		    (layer->new_vpp_setting ||
		     afbc_need_reset ||
		     dvel_changed))
			vd_set_dcu
				(1, &vd_layer[1],
				frame_par, cur_dispbuf2);
		if (dvel_changed)
			force_setting = true;
		dvel_changed = false;
#endif

#ifdef TV_3D_FUNCTION_OPEN
		if (last_mode_3d &&
		    (layer->new_vpp_setting ||
		     afbc_need_reset))
			vd_set_dcu
				(1, &vd_layer[1],
				frame_par, dispbuf);
#endif
		/* for vout change or interlace frame */
		proc_vd_vsc_phase_per_vsync
			(layer,
			frame_par, dispbuf);

		/* because 3d and dv process, vd2 need no scale. */
		/* so don't call the vd2 proc_vd_vsc_phase_per_vsync */

		/* Do 3D process if enabled */
		switch_3d_view_per_vsync(layer);
	}

	/* no frame parameter change */
	if ((!layer->new_vpp_setting && !force_setting) || !frame_par) {
		/* when new frame is toggled but video layer is not on */
		/* need always flush pps register before pwr on */
		/* to avoid pps coeff lost */
		if (frame_par && dispbuf && !is_local_vf(dispbuf) &&
		    (!get_video_enabled(0) ||
		     get_video_onoff_state(0) ==
		     VIDEO_ENABLE_STATE_ON_PENDING))
			vd_scaler_setting(layer, &layer->sc_setting);

		/* dolby vision process for each vsync */
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
		if (!support_multi_core1())
			amdolby_vision_proc(layer, frame_par, NULL, NULL);
#endif
		ret = 0;
		goto render_exit;
	}
	/* VPP one time settings */
	if (dispbuf) {
		config_vd_param(layer, dispbuf);
		config_vd_position
			(layer, &layer->mif_setting);
		config_aisr_position(layer, &layer->aisr_mif_setting);
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
		if (is_amdv_on() && cur_dispbuf2) {
			config_dvel_position
				(layer,
				&local_vd2_mif,
				cur_dispbuf2);
			update_vd2 = true;
		}
#endif
#ifdef TV_3D_FUNCTION_OPEN
		if ((dispbuf->type & VIDTYPE_MVC) ||
		    last_mode_3d) {
			config_3d_vd2_position
				(layer, &local_vd2_mif);
			update_vd2 = true;
		}
#endif
		if (layer->vd1_vd2_mux) {
			layer->mif_setting.p_vd_mif_reg =
				&vd_layer[1].vd_mif_reg;
			vd_mif_setting(&vd_layer[1], &layer->mif_setting);
		} else {
			vd_mif_setting(layer, &layer->mif_setting);
		}
		if (update_vd2)
			vd_mif_setting(&vd_layer[1], &local_vd2_mif);
		fgrain_config(layer,
			      layer->cur_frame_par,
			      &layer->mif_setting,
			      &layer->fgrain_setting,
			      dispbuf);
		/* aisr mif setting */
		aisr_reshape_cfg(layer, &layer->aisr_mif_setting);
	}

#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
	/* work around to cut the last green line */
	/* when two layer dv display and do vskip */
	if (is_amdv_on() &&
	    frame_par->vscale_skip_count > 0 &&
	    cur_dispbuf2 &&
	    frame_par->VPP_pic_in_height_ > 0)
		frame_par->VPP_pic_in_height_--;
#endif
	/* aisr pps config */
	config_aisr_pps(layer, &layer->aisr_sc_setting);

	config_vd_pps
		(layer, &layer->sc_setting, vinfo);
	config_vd_blend
		(layer, &layer->bld_setting);

	update_vd2 = false;
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
	if (for_amdv_certification() &&
	    !get_idk_need_pps())
	    /*some idk2.6 cert cases need pps scaler, idk5.1 5364 need pps*/
		layer->sc_setting.sc_top_enable = false;

	if (is_amdv_on() && cur_dispbuf2) {
		config_dvel_pps
			(layer, &local_vd2_pps, vinfo);
		config_dvel_blend
			(layer, &local_vd2_blend, cur_dispbuf2);
		update_vd2 = true;
	}
#endif
#ifdef TV_3D_FUNCTION_OPEN
	/*turn off vertical scaler when 3d display */
	if ((dispbuf &&
	     (dispbuf->type & VIDTYPE_MVC)) ||
	    last_mode_3d) {
		layer->sc_setting.sc_v_enable = false;
		config_3d_vd2_pps
			(layer, &local_vd2_pps, vinfo);
		config_3d_vd2_blend
			(layer, &local_vd2_blend);
		update_vd2 = true;
	}
#endif
	vd_s5_hw_set(layer, dispbuf, frame_par);
	vd_scaler_setting(layer, &layer->sc_setting);
	aisr_scaler_setting(layer, &layer->aisr_sc_setting);
	aisr_demo_axis_set(layer);
	/* update alpha win */
	if (!cur_dev->pre_vsync_enable)
		alpha_win_set(layer);
	vd_blend_setting(layer, &layer->bld_setting);
	if (update_vd2) {
		vd_scaler_setting(&vd_layer[1], &local_vd2_pps);
		vd_blend_setting(&vd_layer[1], &local_vd2_blend);
	}
	/* dolby vision process for each vsync */
	/* need set after correct_vd1_mif_size_for_DV */
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
	if (!support_multi_core1())
		amdolby_vision_proc(layer, frame_par, NULL, NULL);
#endif
	fgrain_setting(layer, &layer->fgrain_setting, dispbuf);
	layer->new_vpp_setting = false;
	ret = 1;
render_exit:
	/* check if need blank */
	vdx_force_black(0);
	video_secure_set(layer->vpp_index);
	vd_clip_setting(layer->vpp_index, 0, &vd_layer[0].clip_setting);
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
	memset(&ai_face_value, 0, sizeof(ai_face_value));
	if (vd_layer[0].dispbuf &&
		cur_dev->display_module != C3_DISPLAY_MODULE) {
		pq_process_debug[0] = ai_pq_value;
		pq_process_debug[1] = ai_pq_disable;
		pq_process_debug[2] = ai_pq_debug;
		pq_process_debug[3] = ai_pq_policy;
#ifdef CONFIG_AMLOGIC_VDETECT
		vdetect_get_frame_nn_info(vd_layer[0].dispbuf);
#endif
#if defined(CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_VECM)
		vf_pq_process(vd_layer[0].dispbuf, vpp_scenes,
			      pq_process_debug, vpp_new_frame);
#endif
		if (ai_pq_debug > 0x10) {
			ai_pq_debug--;
			if (ai_pq_debug == 0x10)
				ai_pq_debug = 0;
		}
		memcpy(nn_scenes_value, vd_layer[0].dispbuf->nn_value,
		       sizeof(nn_scenes_value));
		if (vd_layer[0].dispbuf->vc_private &&
			vd_layer[0].dispbuf->vc_private->flag & VC_FLAG_AI_FACE) {
			count = vd_layer[0].dispbuf->vc_private->aiface_info->aiface_value_count;
			for (k = 0; k < count; k++) {
				ai_face_value.face_value[k] =
					vd_layer[0].dispbuf->vc_private->aiface_info->face_value[k];
				if (debug_flag & DEBUG_FLAG_AI_FACE) {
					pr_info("vd0:omx_index=%d: i=%d: x=%d; y=%d; w=%d; h=%d; score=%d.\n",
						vd_layer[0].dispbuf->omx_index,
						k,
						ai_face_value.face_value[k].x,
						ai_face_value.face_value[k].y,
						ai_face_value.face_value[k].w,
						ai_face_value.face_value[k].h,
						ai_face_value.face_value[k].score);
				}
			}
			ai_face_value.face_count_vd0 = count;
		}
	}

	if (vd_layer[1].dispbuf &&
		cur_dev->display_module != C3_DISPLAY_MODULE) {
		if (vd_layer[1].dispbuf->vc_private &&
			vd_layer[1].dispbuf->vc_private->flag & VC_FLAG_AI_FACE) {
			count = vd_layer[1].dispbuf->vc_private->aiface_info->aiface_value_count;
			for (k = 0; k < count; k++) {
				l = ai_face_value.face_count_vd0 + k;
				ai_face_value.face_value[l] =
					vd_layer[1].dispbuf->vc_private->aiface_info->face_value[k];
				if (debug_flag & DEBUG_FLAG_AI_FACE) {
					pr_info("vd1:omx_index=%d: i=%d: x=%d; y=%d; w=%d; h=%d; score=%d.\n",
						vd_layer[1].dispbuf->omx_index,
						k,
						ai_face_value.face_value[l].x,
						ai_face_value.face_value[l].y,
						ai_face_value.face_value[l].w,
						ai_face_value.face_value[l].h,
						ai_face_value.face_value[l].score);
				}
			}
			ai_face_value.face_count_vd1 = count;
		}
	}
#endif

	if (vd_layer[0].dispbuf &&
	    (vd_layer[0].dispbuf->type & VIDTYPE_MVC))
		vd_layer[0].enable_3d_mode = mode_3d_mvc_enable;
	else if (process_3d_type)
		vd_layer[0].enable_3d_mode = mode_3d_enable;
	else
		vd_layer[0].enable_3d_mode = mode_3d_disable;
	/* all frames has been renderred, so reset new frame flag */
	vd_layer[0].new_frame = false;
	return ret;
}

s32 vdx_render_frame(struct video_layer_s *layer, const struct vinfo_s *vinfo)
{
	struct vpp_frame_par_s *frame_par;
	u32 zoom_start_y, zoom_end_y;
	struct vframe_s *dispbuf = NULL;
	bool force_setting = false;
	u8 layer_id = 0;
	int ret = 0;

	layer_id = layer->layer_id;
	if (layer_id == 0)
		return primary_render_frame(layer, vinfo);
	if (layer->new_vpp_setting) {
		layer->cur_frame_par = layer->next_frame_par;
		/* just keep the order variable */
		cur_frame_par[layer_id] = layer->cur_frame_par;
	}
	if (glayer_info[layer->layer_id].fgrain_force_update) {
		force_setting = true;
		glayer_info[layer->layer_id].fgrain_force_update = false;
	}

	frame_par = layer->cur_frame_par;

	if (layer->switch_vf && layer->vf_ext)
		dispbuf = layer->vf_ext;
	else
		dispbuf = layer->dispbuf;

	/* process cur frame for each vsync */
	if (dispbuf) {
		int need_afbc =
			(dispbuf->type & VIDTYPE_COMPRESS);
		int afbc_need_reset =
			(layer->enabled && need_afbc &&
			 !is_afbc_enabled(layer->layer_id));

		/*video on && afbc is off && is compress frame.*/
		if (layer->new_vpp_setting || afbc_need_reset)
			vd_set_dcu
				(layer->layer_id, layer,
				frame_par, dispbuf);

		/* for vout change or interlace frame */
		proc_vd_vsc_phase_per_vsync
			(layer,
			frame_par, dispbuf);
	}

	if (!layer->new_vpp_setting && !force_setting) {
		/* when new frame is toggled but video layer is not on */
		/* need always flush pps register before pwr on */
		/* to avoid pps coeff lost */
		if (frame_par && dispbuf && !is_local_vf(dispbuf) &&
		    (!get_video_enabled(layer_id) ||
		     get_video_onoff_state(layer_id) ==
		     VIDEO_ENABLE_STATE_ON_PENDING))
			vd_scaler_setting(layer, &layer->sc_setting);
		ret = 0;
		goto render_exit;
	}

	if (dispbuf) {
		/* progressive or decode interlace case height 1:1 */
		zoom_start_y = frame_par->VPP_vd_start_lines_;
		zoom_end_y = frame_par->VPP_vd_end_lines_;
		if (dispbuf &&
		    (dispbuf->type & VIDTYPE_INTERLACE) &&
		    (dispbuf->type & VIDTYPE_VIU_FIELD)) {
			/* vdin interlace frame case height/2 */
			zoom_start_y /= 2;
			zoom_end_y = ((zoom_end_y + 1) >> 1) - 1;
		}
		layer->start_x_lines = frame_par->VPP_hd_start_lines_;
		layer->end_x_lines = frame_par->VPP_hd_end_lines_;
		layer->start_y_lines = zoom_start_y;
		layer->end_y_lines = zoom_end_y;
		config_vd_position
			(layer, &layer->mif_setting);
		vd_mif_setting
			(layer, &layer->mif_setting);
		fgrain_config(layer,
			      layer->cur_frame_par,
			      &layer->mif_setting,
			      &layer->fgrain_setting,
			      dispbuf);
	}

	config_vd_pps
		(layer, &layer->sc_setting, vinfo);
	vd_s5_hw_set(layer, dispbuf, frame_par);
	vd_scaler_setting
		(layer, &layer->sc_setting);
	/* update alpha win */
	alpha_win_set(layer);
	config_vd_blend
		(layer, &layer->bld_setting);
	vd_blend_setting
		(layer, &layer->bld_setting);
	if (layer->vpp_index != VPP0)
		vppx_vd_blend_setting
		(layer, &layer->bld_setting);
	fgrain_setting(layer,
		       &layer->fgrain_setting,
		       dispbuf);
	layer->new_vpp_setting = false;
	ret = 1;
render_exit:
	/* check if need blank */
	vdx_force_black(layer_id);
	video_secure_set(layer->vpp_index);

	vd_clip_setting(layer->vpp_index,
		layer_id, &vd_layer[layer_id].clip_setting);
	/* all frames has been renderred, so reset new frame flag */
	vd_layer[layer_id].new_frame = false;
	return ret;
}

int update_video_recycle_buffer(u8 path_index)
{
	struct vframe_s *rdma_buf = NULL;
	struct vframe_s *to_put_buf[DISPBUF_TO_PUT_MAX];
	int i, j;
	bool mismatch  = true, need_force_recycle = true;
	int put_cnt = 0;

	j = path_index;
	memset(to_put_buf, 0, sizeof(to_put_buf));
#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
	/* after one vsync processing, should be same */
	if (cur_rdma_buf[j] != cur_dispbuf[j] &&
	    cur_dispbuf[j] != &vf_local[j]) {
		pr_err("expection: video%d rdma_buf:%p, dispbuf:%p!\n",
			j,
			cur_rdma_buf[j], cur_dispbuf[j]);
		return -1;
	}
	for (i = 0; i < DISPBUF_TO_PUT_MAX; i++)
		if (dispbuf_to_put[j][i] &&
		    (dispbuf_to_put[j][i]->flag & VFRAME_FLAG_DI_PW_VFM) &&
		    IS_DI_POSTWRTIE(dispbuf_to_put[j][i]->type))
			to_put_buf[put_cnt++] = dispbuf_to_put[j][i];
#endif

	if (cur_dispbuf[j] != &vf_local[j])
		rdma_buf = cur_dispbuf[j];

	if (rdma_buf &&
	    (!(rdma_buf->flag & VFRAME_FLAG_DI_PW_VFM) ||
	     !IS_DI_POSTWRTIE(rdma_buf->type)))
		rdma_buf = NULL;

	if (recycle_cnt[j] &&
	    (put_cnt > 0 || rdma_buf)) {
		for (i = 0; i < recycle_cnt[j]; i++) {
			if (recycle_buf[j][i] == rdma_buf)
				need_force_recycle = false;
		}
		if (need_force_recycle)
			return -EAGAIN;

		pr_err("expection: video%d recycle_cnt:%d, put_cnt:%d, recycle_buf:%p-%p, to_put:%p, rdma_buf:%p!\n",
			j,
			recycle_cnt[j], put_cnt,
			recycle_buf[j][0], recycle_buf[j][1],
			to_put_buf[0], rdma_buf);
		return -1;
	}
	if (!recycle_cnt[j]) {
		for (i = 0; i < DISPBUF_TO_PUT_MAX + 1; i++)
			recycle_buf[j][i] = NULL;

		for (i = 0; i < put_cnt; i++) {
			if (to_put_buf[i])
				recycle_buf[j][recycle_cnt[j]++] =
					to_put_buf[i];
			if (rdma_buf && rdma_buf == to_put_buf[i])
				mismatch = false;
		}
		if (rdma_buf && mismatch)
			recycle_buf[j][recycle_cnt[j]++] = rdma_buf;
		pr_debug("video%d need recycle %d new buffers: %p, %p, %p, %p, put_cnt:%d, rdma_buf:%p\n",
			j,
			recycle_cnt[j],
			recycle_buf[j][0], recycle_buf[j][1],
			recycle_buf[j][2], recycle_buf[j][3],
			put_cnt, rdma_buf);
	} else {
		pr_debug("video%d need recycle %d remained buffers: %p, %p, %p, %p\n",
			j,
			recycle_cnt[j],
			recycle_buf[j][0], recycle_buf[j][1],
			recycle_buf[j][2], recycle_buf[j][3]);
	}
	return 0;
}

void set_alpha_scpxn(struct video_layer_s *layer,
			   struct composer_info_t *composer_info)
{
	struct pip_alpha_scpxn_s alpha_win;
	static struct pip_alpha_scpxn_s last_alpha_win;
	int win_num = 0;
	int win_en = 0;
	static int last_alpha_win_en;
	int i, update = 0;

	memset(&alpha_win, 0, sizeof(struct pip_alpha_scpxn_s));

	if (composer_info) {
		win_num = composer_info->count;
		if (win_num >= MAX_COMPOSER_COUNT || win_num >= MAX_PIP_WINDOW) {
			pr_info("%s, win_num%d is wrong\n", __func__, win_num);
			return;
		}
		for (i = 0; i < win_num; i++) {
			alpha_win.scpxn_bgn_h[i] = composer_info->axis[i][0];
			alpha_win.scpxn_end_h[i] = composer_info->axis[i][2];
			alpha_win.scpxn_bgn_v[i] = composer_info->axis[i][1];
			alpha_win.scpxn_end_v[i] = composer_info->axis[i][3];
			win_en |= 1 << i;
		}
	}
	/* check win num first, if changed update */
	if (last_alpha_win_en != win_en) {
		update = 1;
	} else {
		/* check win set, if changed update */
		for (i = 0; i < MAX_PIP_WINDOW; i++) {
			if (alpha_win.scpxn_bgn_h[i] != last_alpha_win.scpxn_bgn_h[i] ||
				alpha_win.scpxn_end_h[i] != last_alpha_win.scpxn_end_h[i] ||
				alpha_win.scpxn_bgn_v[i] != last_alpha_win.scpxn_bgn_v[i] ||
				alpha_win.scpxn_end_v[i] != last_alpha_win.scpxn_end_v[i]) {
				update = 1;
				break;
			}
		}
	}
	layer->alpha_win_en = win_en;
	last_alpha_win_en = win_en;
	memcpy(&layer->alpha_win, &alpha_win,
		sizeof(struct pip_alpha_scpxn_s));
	memcpy(&last_alpha_win, &alpha_win,
		sizeof(struct pip_alpha_scpxn_s));
	if (update) {
		if (layer->layer_id == 0) {
			vd_layer[0].property_changed = true;
		} else if (layer->layer_id == 1) {
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
}

static void blend_reg_conflict_detect(void)
{
	u32 r1, r2, r3;
	u32 blend_en = 0;

	if (cur_dev->display_module == C3_DISPLAY_MODULE)
		return;
	if (glayer_info[0].afbc_support) {
		if (!legacy_vpp) {
			r1 = READ_VCBUS_REG(VD1_BLEND_SRC_CTRL);
			if (r1 & 0xfff)
				blend_en = 1;
		} else {
			r1 = READ_VCBUS_REG(VPP_MISC);
			if (r1 & VPP_VD1_POSTBLEND)
				blend_en = 1;
		}
		r2 = READ_VCBUS_REG(vd_layer[0].vd_afbc_reg.afbc_enable);
		r3 = READ_VCBUS_REG(vd_layer[0].vd_mif_reg.vd_if0_gen_reg);
		if (r2 == 0 && r3 == 0 && blend_en)
			blend_conflict_cnt++;
	} else {
		return;
	}

	if (glayer_info[1].afbc_support) {
		blend_en = 0;
		if (!legacy_vpp) {
			r1 = READ_VCBUS_REG(VD2_BLEND_SRC_CTRL);
			if (r1 & 0xfff)
				blend_en = 1;
		} else {
			r1 = READ_VCBUS_REG(VPP_MISC);
			if (r1 & VPP_VD2_POSTBLEND)
				blend_en = 1;
		}
		r2 = READ_VCBUS_REG(vd_layer[1].vd_afbc_reg.afbc_enable);
		r3 = READ_VCBUS_REG(vd_layer[1].vd_mif_reg.vd_if0_gen_reg);
		if (r2 == 0 && r3 == 0 && blend_en)
			blend_conflict_cnt++;
	} else {
		return;
	}

	if (glayer_info[2].afbc_support) {
		blend_en = 0;
		if (cur_dev->max_vd_layers == 3) {
			r1 = READ_VCBUS_REG(VD3_BLEND_SRC_CTRL);
			if (r1 & 0xfff)
				blend_en = 1;
			r2 = READ_VCBUS_REG(vd_layer[2].vd_afbc_reg.afbc_enable);
			r3 = READ_VCBUS_REG(vd_layer[2].vd_mif_reg.vd_if0_gen_reg);
			if (r2 == 0 && r3 == 0 && blend_en)
				blend_conflict_cnt++;
		}
	} else {
		return;
	}
}

#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_VECM
void amvecm_process(struct path_id_s *path_id,
			    struct video_recv_s *p_gvideo_recv,
			    struct vframe_s *new_frame)
{
	u32 vpp_index = VPP0;

	if (path_id->vd1_path_id == p_gvideo_recv->path_id)
		amvecm_on_vs
			((p_gvideo_recv->cur_buf !=
			 &p_gvideo_recv->local_buf)
			? p_gvideo_recv->cur_buf : NULL,
			new_frame,
			CSC_FLAG_CHECK_OUTPUT,
			0,
			0,
			0,
			0,
			0,
			0,
			VD1_PATH,
			vd_layer[0].vpp_index);
	else if (path_id->vd2_path_id == p_gvideo_recv->path_id) {
		if (p_gvideo_recv->vpp_id != VPP0)
			vpp_index = vd_layer_vpp[p_gvideo_recv->vpp_id - VPP1].vpp_index;
		else
			vpp_index = vd_layer[1].vpp_index;
		amvecm_on_vs
			((p_gvideo_recv->cur_buf !=
			 &p_gvideo_recv->local_buf)
			? p_gvideo_recv->cur_buf : NULL,
			new_frame,
			CSC_FLAG_CHECK_OUTPUT,
			0,
			0,
			0,
			0,
			0,
			0,
			VD2_PATH,
			vpp_index);
	} else if (path_id->vd3_path_id == p_gvideo_recv->path_id) {
		if (p_gvideo_recv->vpp_id != VPP0)
			vpp_index = vd_layer_vpp[p_gvideo_recv->vpp_id - VPP1].vpp_index;
		else
			vpp_index = vd_layer[2].vpp_index;
		amvecm_on_vs
			((p_gvideo_recv->cur_buf !=
			 &p_gvideo_recv->local_buf)
			? p_gvideo_recv->cur_buf : NULL,
			new_frame,
			CSC_FLAG_CHECK_OUTPUT,
			0,
			0,
			0,
			0,
			0,
			0,
			VD3_PATH,
			vpp_index);
	}
}
#endif

/*tunnel mode, map dv instance when creating amvideo/videopip path*/
inline bool is_tunnel_mode(const char *receiver_name)
{
	char *provider_name;

	provider_name = vf_get_provider_name(receiver_name);
	while (provider_name) {
		if (!vf_get_provider_name(provider_name))
			break;
		provider_name =
			vf_get_provider_name(provider_name);
	}
	if (provider_name &&
	    (strstr(provider_name, "dv_vdin") ||
	    strstr(provider_name, "vdin0") ||
	    strstr(provider_name, "decoder") ||
	    strstr(provider_name, "vdec.h265") ||
	    strstr(provider_name, "vdec.h264") ||
	    strstr(provider_name, "dvbldec") ||
	    strstr(provider_name, "dvbldec2") ||
	    strstr(provider_name, "vdec.av1") ||
	    strstr(provider_name, "vdec.vp9"))) {
		return true;
	}
	return false;
}

#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
static void set_dv_provide_name(void)
{
	s32 vd1_path_id = glayer_info[0].display_path_id;
	s32 vd2_path_id = glayer_info[1].display_path_id;

	if (is_amdv_enable()) {
		if (is_tunnel_mode(RECEIVER_NAME) || is_tunnel_mode(RECEIVERPIP_NAME)) {
			char *provider_name = NULL;

			if (vd1_path_id == VFM_PATH_PIP || vd2_path_id == VFM_PATH_PIP) {
				provider_name = vf_get_provider_name(RECEIVERPIP_NAME);
				while (provider_name) {
					if (!vf_get_provider_name(provider_name))
						break;
					provider_name =
						vf_get_provider_name(provider_name);
				}
				if (provider_name)
					amdv_set_provider(provider_name, VD2_PATH);
				else
					amdv_set_provider(dv_provider[1], VD2_PATH);
			}
			if (vd1_path_id == VFM_PATH_AMVIDEO || vd2_path_id == VFM_PATH_AMVIDEO) {
				provider_name = vf_get_provider_name(RECEIVER_NAME);
				while (provider_name) {
					if (!vf_get_provider_name(provider_name))
						break;
					provider_name =
						vf_get_provider_name(provider_name);
				}
				if (provider_name)
					amdv_set_provider(provider_name, VD1_PATH);
				else
					amdv_set_provider(dv_provider[0], VD1_PATH);
			}
		} else {
			amdv_set_provider(dv_provider[0], VD1_PATH);
			amdv_set_provider(dv_provider[1], VD2_PATH);
		}
	}
}
#endif

static void dmc_adjust_process(struct vframe_s *vf)
{
#ifndef CONFIG_AMLOGIC_REMOVE_OLD
	if (is_meson_txlx_cpu() && dmc_adjust) {
		bool force_adjust = false;
		u32 vf_width = 0, vf_height = 0;
		struct vframe_s *chk_vf;

		chk_vf = vf ? vf : cur_dispbuf[0];
		if (chk_vf)
			force_adjust =
				((chk_vf->type & VIDTYPE_VIU_444) ||
				(chk_vf->type & VIDTYPE_RGB_444))
				? true : false;
		if (chk_vf) {
			if (cur_frame_par[0] &&
			    cur_frame_par[0]->nocomp) {
				vf_width = chk_vf->width;
				vf_height = chk_vf->height;
			} else if ((chk_vf->type & VIDTYPE_COMPRESS) &&
				cur_frame_par[0] &&
				cur_frame_par[0]->vscale_skip_count) {
				vf_width = chk_vf->compWidth;
				vf_height = chk_vf->compHeight;
			} else {
				vf_width = chk_vf->width;
				vf_height = chk_vf->height;
			}
			dmc_adjust_for_mali_vpu
				(vf_width,
				vf_height,
				force_adjust);
		} else {
			dmc_adjust_for_mali_vpu
				(0, 0, force_adjust);
		}
	}
#endif
}

static void check_src_fmt_change(void)
{
	enum vframe_signal_fmt_e fmt;

	fmt = (enum vframe_signal_fmt_e)atomic_read(&primary_src_fmt);
	if (fmt != atomic_read(&cur_primary_src_fmt)) {
		if (debug_flag & DEBUG_FLAG_TRACE_EVENT) {
			char *old_str = NULL, *new_str = NULL;
			enum vframe_signal_fmt_e old_fmt;

			old_fmt = (enum vframe_signal_fmt_e)
				atomic_read(&cur_primary_src_fmt);
			if (old_fmt != VFRAME_SIGNAL_FMT_INVALID)
				old_str = (char *)src_fmt_str[old_fmt];
			if (fmt != VFRAME_SIGNAL_FMT_INVALID)
				new_str = (char *)src_fmt_str[fmt];
			pr_info("VD1 src fmt changed: %s->%s.\n",
				old_str ? old_str : "invalid",
				new_str ? new_str : "invalid");
		}
		atomic_set(&cur_primary_src_fmt, fmt);
		video_prop_status |= VIDEO_PROP_CHANGE_FMT;
		update_primary_fmt_event();
	}
}

static void set_cur_line_info(u8 index)
{
	struct cur_line_info_t *cur_line_info = &g_cur_line_info[index];
	struct timeval start;

	do_gettimeofday(&start);
	cur_line_info->enc_line_start = get_cur_enc_line();
	cur_line_info->enc_num_start = get_cur_enc_num();
	cur_line_info->start = start;
	cur_line_info->end1 = start;
	cur_line_info->end2 = start;
	cur_line_info->end3 = start;
	cur_line_info->end4 = start;
}

struct cur_line_info_t *get_cur_line_info(u8 index)
{
	return &g_cur_line_info[index];
}

u32 get_enc_num_start(u8 index)
{
	return g_cur_line_info[index].enc_num_start;
}

ulong get_enc_time_start(u8 index)
{
	struct timeval *t = &g_cur_line_info[index].start;

	return (t->tv_sec * 1000000 + t->tv_usec);
}

static inline void trace_performance(struct cur_line_info_t *cur_line_info,
	int cur_enc_line)
{
	u32 sync_duration;
	struct vinfo_s *video_info;
	unsigned long time_use1 = 0;
	unsigned long time_use2 = 0;
	unsigned long time_use3 = 0;
	unsigned long time_use4 = 0;
	unsigned long time_use5 = 0;
	int enc_line_start;
	struct timeval end;
	struct timeval *start;
	struct timeval *end1;
	struct timeval *end2;
	struct timeval *end3;
	struct timeval *end4;

	enc_line_start = cur_line_info->enc_line_start;
	start = &cur_line_info->start;
	end1 = &cur_line_info->end1;
	end2 = &cur_line_info->end2;
	end3 = &cur_line_info->end3;
	end4 = &cur_line_info->end4;

	do_gettimeofday(&end);
	time_use1 = (end1->tv_sec - start->tv_sec) * 1000000 +
				(end1->tv_usec - start->tv_usec);
	time_use2 = (end2->tv_sec - start->tv_sec) * 1000000 +
				(end2->tv_usec - start->tv_usec);
	time_use3 = (end3->tv_sec - start->tv_sec) * 1000000 +
				(end3->tv_usec - start->tv_usec);
	time_use4 = (end4->tv_sec - start->tv_sec) * 1000000 +
				(end4->tv_usec - start->tv_usec);
	time_use5 = (end.tv_sec - start->tv_sec) * 1000000 +
				(end.tv_usec - start->tv_usec);

	video_info = get_current_vinfo();
	if (video_info && video_info->sync_duration_num)
		sync_duration = video_info->sync_duration_den * 1000 /
				video_info->sync_duration_num;
	else
		sync_duration = 16;

	if (cur_enc_line < enc_line_start || time_use5 > sync_duration * 1000) {
		over_field = true;
		++over_field_case1_cnt;
		if (performance_debug & DEBUG_FLAG_OVER_VSYNC)
			pr_info("long vsync enc line: %4d/4%d, time %ld us\n",
				enc_line_start, cur_enc_line, time_use5);
	}

	vpp_trace_timeinfo(time_use1, time_use2, time_use3,
		time_use4, time_use5, sync_duration * 1000);

	if (performance_debug & DEBUG_FLAG_VSYNC_PROCESS_TIME) {
		pr_info("vsync time: %ld %ld %ld %ld %ld us\n",
			time_use1, time_use2, time_use3, time_use4, time_use5);
		pr_info("vsync enc line: %4d/4%d, over_field %d, count %d %d\n",
			enc_line_start, cur_enc_line,
			over_field, over_field_case1_cnt, over_field_case2_cnt);
	}
}

static void over_field_info_record(void)
{
	bool valid_mode = false;
	u32 timeinfo_th = 0;
	u32 enc_num = get_cur_enc_num();
	struct cur_line_info_t *cur_line_info = get_cur_line_info(0);
	struct timeval *start;

	start = &cur_line_info->start;
	if (vinfo &&
		(vinfo->mode == VMODE_HDMI ||
		 vinfo->mode == VMODE_LCD ||
		 vinfo->mode == VMODE_CVBS)) {
		valid_mode = true;
		if (vinfo->sync_duration_num) {
			timeinfo_th = vinfo->sync_duration_den * 1000000 /
				vinfo->sync_duration_num;
			/* set as 1.5 duration */
			timeinfo_th = (timeinfo_th * 3) >> 1;
		}
	}
	/* TODO: need check each layer by new frame flag */
	if (!over_field && valid_mode && timeinfo_th) {
		u32 state = atomic_read(&cur_over_field_state);

		if (state == OVER_FIELD_RDMA_READY ||
			state == OVER_FIELD_NORMAL) {
			ulong cur_timeinfo = start->tv_sec * 1000000 +
				start->tv_usec;

			if (enc_num == config_vsync_num &&
				cur_timeinfo < config_timeinfo + timeinfo_th)
				over_field = true;
		} else if (state == OVER_FIELD_NEW_VF) {
			over_field = true;
		}
		if (over_field)
			over_field_case2_cnt++;
	}
	update_over_field_states(OVER_FIELD_NORMAL, !valid_mode ? true : false);
	if (atomic_read(&cur_over_field_state) == OVER_FIELD_NEW_VF) {
		always_new_vf_cnt++;
		if (always_new_vf_cnt > 5) {
			update_over_field_states(OVER_FIELD_NORMAL, true);
			pr_info("Over field state is always as new_vf. Need reset it.\n");
			always_new_vf_cnt = 0;
		}
	} else {
		always_new_vf_cnt = 0;
	}
	vpp_trace_field_state("VSYNC-START",
		atomic_read(&cur_over_field_state),
		atomic_read(&cur_over_field_state),
		over_field ? 1 : 0,
		config_vsync_num, enc_num);
}

static bool check_sideband_type(struct vframe_s *vf, bool *need_force_black)
{
	if (!vf)
		return false;

	pr_info("vpp: sideband vf:%d, surface:%d; type: vf:%d, tvinput:%d\n",
		vf->sidebind_type,
		glayer_info[0].sideband_type,
		vf->source_type,
		tvin_source_type);

	if (vf->sidebind_type != 0) {
		if (vf->sidebind_type == glayer_info[0].sideband_type) {
			/*dtv -> ATV, sidebind is ATV , but vfm is dtv, can not disp*/
			if (glayer_info[0].sideband_type == 1 &&
				tvin_source_type == TVIN_SOURCE_TYPE_VDIN &&
				vf->source_type == VFRAME_SOURCE_TYPE_OTHERS) {
				pr_info("can not disp now\n");
				*need_force_black = true;
				return false;
			}
			return true;
		} else {
			return false;
		}
	} else {
		return true;
	}
}

static void vd_dispbuf_to_put(int layer_id)
{
#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
	int i;

	/* if mode is video_process_in_thread,
	 * toggle/swap/render in thread and put_buf in vsync isr, skip this func.
	 */
	if (is_video_process_in_thread())
		return;

	if (over_field)
		return;

	for (i = 0; i < DISPBUF_TO_PUT_MAX; i++) {
		if (dispbuf_to_put[layer_id][i]) {
			dispbuf_to_put[layer_id][i]->rendered = true;
			if (!video_vf_put(layer_id, dispbuf_to_put[layer_id][i])) {
				dispbuf_to_put[layer_id][i] = NULL;
				dispbuf_to_put_num[layer_id]--;
			}
		}
	}
#endif
}

static void vd_dispbuf_init(u8 layer_id)
{
	int i;
	u8 path_index = 0;

	if (layer_id >= MAX_VD_LAYER)
		return;
	i  = layer_id;
#ifdef CONFIG_AMLOGIC_MEDIA_DEINTERLACE
	struct vframe_s *_cur_dispbuf;
	struct vframe_s *_local_vf;
	int j;

	_cur_dispbuf = cur_dispbuf[i];
	_local_vf = &vf_local[i];
	if (recycle_cnt[i] > 0 && _cur_dispbuf != _local_vf) {
		for (j = 0; j < recycle_cnt[i]; j++) {
			if ((recycle_buf[i][j] &&
			    _cur_dispbuf != recycle_buf[i][j]) &&
			    (recycle_buf[i][j]->flag & VFRAME_FLAG_DI_PW_VFM)) {
				if (i == 0)
					di_release_count++;
				dim_post_keep_cmd_release2(recycle_buf[i][j]);
			} else if (recycle_buf[i][j] &&
				   _cur_dispbuf == recycle_buf[i][j]) {
				pr_err("recycle %d di buffer conflict, recycle vf:%p\n",
				       i, recycle_buf[i][j]);
			}
			recycle_buf[i][j] = NULL;
		}
		recycle_cnt[i] = 0;
	}
#endif
	for (path_index = 0; path_index < MAX_VD_LAYER; path_index++) {
		if (vd_layer[i].dispbuf_mapping == &cur_dispbuf[path_index] &&
		    (cur_dispbuf[path_index] == &vf_local[path_index] ||
		     !cur_dispbuf[path_index]) &&
		    vd_layer[i].dispbuf != cur_dispbuf[path_index])
			vd_layer[i].dispbuf = cur_dispbuf[path_index];
	}
	for (path_index = 0; path_index < MAX_VD_LAYER; path_index++) {
		if (gvideo_recv[path_index] &&
			vd_layer[i].dispbuf_mapping == &gvideo_recv[path_index]->cur_buf &&
			(gvideo_recv[path_index]->cur_buf == &gvideo_recv[path_index]->local_buf ||
			 !gvideo_recv[path_index]->cur_buf) &&
			vd_layer[i].dispbuf != gvideo_recv[path_index]->cur_buf)
			vd_layer[i].dispbuf = gvideo_recv[path_index]->cur_buf;
	}
}

void put_buffer_proc(void)
{
	int i;

	for (i = 0; i < 3; i++) {
		if (gvideo_recv[i])
			gvideo_recv[i]->func->early_proc(gvideo_recv[i], 0);
	}
}

static inline int recvx_early_proc(u8 path_index)
{
	if (atomic_read(&video_unreg_flag))
		return -1;

	check_src_fmt_change();
	if (!get_lowlatency_mode()) {
		if (gvideo_recv[path_index]) {
			/* normal mode: true; lowlatency mode: false */
			gvideo_recv[path_index]->irq_mode = true;
			gvideo_recv[path_index]->func->early_proc(gvideo_recv[path_index], //0);
							    over_field ? 1 : 0);
		}
	}
	return 0;
}

static int amvideo_early_proc(u8 layer_id)
{
	int enc_line;
	int hold_line;
	struct vframe_s *vf;
	struct vframe_s *vf_tmp;
	s32 vd1_path_id = glayer_info[0].display_path_id;
	struct cur_line_info_t *cur_line_info = get_cur_line_info(0);
#if defined(CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_VECM)
	u16 line = glayer_info[0].layer_top;
#endif

	vd_dispbuf_to_put(layer_id);
	get_count_pip[0] = 0;

	if (!hist_test_flag && cur_dispbuf[0] == &hist_test_vf)
		cur_dispbuf[0] = NULL;

	hold_line = calc_hold_line();
	do_frame_detect();

	frame_drop_process();

	vf = video_vf_peek(0);

	if (!vf)
		vf_tmp = cur_dispbuf[0];
	else
		vf_tmp = vf;

	if (vf_tmp) {
		if (glayer_info[0].display_path_id == VFM_PATH_AUTO) {
			if (check_sideband_type(vf_tmp, &need_force_black)) {
				pr_info("VID: path_id %d -> %d\n",
					glayer_info[0].display_path_id,
					VFM_PATH_AMVIDEO);
				glayer_info[0].display_path_id =
					VFM_PATH_AMVIDEO;
				vd1_path_id = glayer_info[0].display_path_id;
			} else if (glayer_info[0].sideband_type != -1)
				pr_info("vf->sideband_type =%d,layertype=%d\n",
					vf_tmp->sidebind_type,
					glayer_info[0].sideband_type);
		}
	}

	/* vout mode detection under old tunnel mode */
	if ((vf) && ((vf->type & VIDTYPE_NO_VIDEO_ENABLE) == 0)) {
		if (strcmp(old_vmode, new_vmode)) {
			vd_layer[0].property_changed = true;
			vd_layer[1].property_changed = true;
			vd_layer[2].property_changed = true;
			pr_debug("detect vout mode change!!!!!!!!!!!!\n");
			strcpy(old_vmode, new_vmode);
		}
	}

	/*the first/second vf di maybe bypass, So we need to calculate the
	 *first three frames
	 */
	if (display_frame_count < 3 && vf &&
	    (vf->source_type == VFRAME_SOURCE_TYPE_HDMI ||
	    vf->source_type == VFRAME_SOURCE_TYPE_CVBS ||
	    vf->source_type == VFRAME_SOURCE_TYPE_TUNER))
		hdmi_in_delay_maxmin_old(vf);

#if defined(CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_VECM)
	if (cur_frame_par[0] &&
		(vd1_path_id == VFM_PATH_AMVIDEO ||
		vd1_path_id == VFM_PATH_DEF)) {
		/*need call every vsync*/
		if (vf_tmp)
			frame_lock_process(vf_tmp, cur_frame_par[0], line);
		else
			frame_lock_process(NULL, cur_frame_par[0], line);
	}
#endif

	if (performance_debug & DEBUG_FLAG_VSYNC_PROCESS_TIME)
		do_gettimeofday(&cur_line_info->end1);
	enc_line = get_cur_enc_line();
	if (enc_line > vsync_enter_line_max)
		vsync_enter_line_max = enc_line;

	dmc_adjust_process(vf);

	if (to_notify_trick_wait) {
		atomic_set(&trickmode_framedone, 1);
		video_notify_flag |= VIDEO_NOTIFY_TRICK_WAIT;
		to_notify_trick_wait = false;
		return -1;
	}

#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
	/* check video frame before VECM process */
	if (is_amdv_enable() && vf &&
		(vd1_path_id == VFM_PATH_AMVIDEO ||
		 vd1_path_id == VFM_PATH_DEF ||
		 vd1_path_id == VFM_PATH_AUTO)) {
		amdv_check_mvc(vf);
		amdv_check_hdr10(vf);
		amdv_check_hdr10plus(vf);
		amdv_check_hlg(vf);
		amdv_check_primesl(vf);
		amdv_check_cuva(vf);
	}
#endif

	pts_process();
	if (atomic_read(&video_unreg_flag))
		return -1;

	if (atomic_read(&video_pause_flag) &&
	    !(vd_layer[0].global_output == 1 &&
	      vd_layer[0].enabled != vd_layer[0].enabled_status_saved))
		return -1;

	check_src_fmt_change();
	return 0;
}

static int pipx_early_proc(u8 path_index)
{
	vd_dispbuf_to_put(path_index);
	get_count_pip[path_index] = 0;
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
	/* for pip */
	if (path_index == 1)
		set_dv_provide_name();
#endif
	if (atomic_read(&video_unreg_flag))
		return -1;
	//check_src_fmt_change();
	return 0;
}

static int video_early_proc(u8 layer_id, u8 fake_layer_id)
{
	u8 func_id = 0, path_index = 0;

#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
		if (is_amdv_on() && layer_id == 0)
			print_dv_ro();
#endif

#ifdef CONFIG_AMLOGIC_MEDIA_FRC
	if (cur_dev->vsync_2to1_enable &&
		layer_id == 0 &&
		vsync_count_start) {
		/* not prevsync not handle */
		if (!frc_drv_get_1st_frm())
			return 0;
	}
#endif
	if (layer_id == 0xff)
		func_id = vd_fake_func[fake_layer_id].fake_func_id;
	else if (layer_id < MAX_VD_LAYER)
		func_id = vd_layer[layer_id].func_path_id;
	switch (func_id) {
	case AMVIDEO:
		return amvideo_early_proc(AMVIDEO);
	case PIP1:
	case PIP2:
		path_index = func_id - AMVIDEO;
		return pipx_early_proc(path_index);
	case RENDER0:
	case RENDER1:
	case RENDER2:
		path_index = func_id - RENDER0;
		return recvx_early_proc(path_index);
	default:
		break;
	}
	return 0;
}

static int pipx_late_proc(u8 path_index)
{
#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
	cur_rdma_buf[path_index] = cur_dispbuf[path_index];
#endif
	if (debug_flag & DEBUG_FLAG_GET_COUNT)
		pr_info("pip%d=%d\n",
			path_index,
			get_count_pip[path_index]);
	return 0;
}

static int video_late_proc(u8 layer_id, u8 fake_layer_id)
{
	u8 func_id = 0;
	u8 path_index = 0;

#ifdef CONFIG_AMLOGIC_MEDIA_FRC
	if (cur_dev->vsync_2to1_enable &&
		layer_id == 0 &&
		vsync_count_start) {
		/* not prevsync not handle */
		if (!frc_drv_get_1st_frm())
			return 0;
	}
#endif
	if (layer_id == 0xff)
		func_id = vd_fake_func[fake_layer_id].fake_func_id;
	else if (layer_id < MAX_VD_LAYER)
		func_id = vd_layer[layer_id].func_path_id;
	switch (func_id) {
	case AMVIDEO:
	case PIP1:
	case PIP2:
		pipx_late_proc(func_id - AMVIDEO);
		break;
	case RENDER0:
	case RENDER1:
	case RENDER2:
		path_index = func_id - RENDER0;
		if (gvideo_recv[path_index] && path_index < MAX_VD_LAYER)
			gvideo_recv[path_index]->func->late_proc(gvideo_recv[path_index]);
		break;
	default:
		break;
	}
	return 0;
}

static int vdx_misc_early_proc(u8 layer_id,
				      bool rdma_enable,
				      bool rdma_enable_pre)
{
	bool pre_vsync_notify = false;
	bool post_vsync_notify = false;

#ifdef CONFIG_AMLOGIC_MEDIA_FRC
	if (cur_dev->vsync_2to1_enable &&
		layer_id == 0 &&
		vsync_count_start) {
		/* not prevsync not handle */
		if (!frc_drv_get_1st_frm())
			return 0;
	}
#endif
	/* prevsync + postvsync case */
	if (cur_dev->pre_vsync_enable) {
		if (layer_id == 0) {
			u32 pts_inc_scale_base = 0;

			if (frc_n2m_worked())
				pts_inc_scale_base = vsync_pts_inc_scale_base / 2;
			else
				pts_inc_scale_base = vsync_pts_inc_scale_base;
#ifdef CONFIG_AMLOGIC_VIDEO_COMPOSER
			vsync_notify_video_composer(layer_id,
				vsync_pts_inc_scale,
				pts_inc_scale_base);
#endif
#ifdef CONFIG_AMLOGIC_VIDEOQUEUE
			vsync_notify_videoqueue(layer_id,
				vsync_pts_inc_scale,
				pts_inc_scale_base);
#endif
			pre_vsync_notify = true;
		}

		if (layer_id != 0 && !post_vsync_notify) {
#ifdef CONFIG_AMLOGIC_VIDEO_COMPOSER
			vsync_notify_video_composer(layer_id,
				vsync_pts_inc_scale,
				vsync_pts_inc_scale_base);
#endif
#ifdef CONFIG_AMLOGIC_VIDEOQUEUE
			vsync_notify_videoqueue(layer_id,
				vsync_pts_inc_scale,
				vsync_pts_inc_scale_base);
#endif
			post_vsync_notify = true;
		}
	} else {
		if (cur_dev->vsync_2to1_enable) {
			/* postvsync case for n2m(only for vd1) */
			if (layer_id == 0 && frc_n2m_worked()) {
#ifdef CONFIG_AMLOGIC_VIDEO_COMPOSER
				vsync_notify_video_composer(layer_id,
					vsync_pts_inc_scale,
					vsync_pts_inc_scale_base / 2);
#endif
#ifdef CONFIG_AMLOGIC_VIDEOQUEUE
				vsync_notify_videoqueue(layer_id,
					vsync_pts_inc_scale,
					vsync_pts_inc_scale_base / 2);
#endif

			} else {
#ifdef CONFIG_AMLOGIC_VIDEO_COMPOSER
				vsync_notify_video_composer(layer_id,
					vsync_pts_inc_scale,
					vsync_pts_inc_scale_base);
#endif
#ifdef CONFIG_AMLOGIC_VIDEOQUEUE
				vsync_notify_videoqueue(layer_id,
					vsync_pts_inc_scale,
					vsync_pts_inc_scale_base);
#endif
			}
		} else {
		/* always postvsync case for no n2m */
#ifdef CONFIG_AMLOGIC_VIDEO_COMPOSER
			vsync_notify_video_composer(layer_id,
				vsync_pts_inc_scale,
				vsync_pts_inc_scale_base);
#endif
#ifdef CONFIG_AMLOGIC_VIDEOQUEUE
			vsync_notify_videoqueue(layer_id,
				vsync_pts_inc_scale,
				vsync_pts_inc_scale_base);
#endif
			post_vsync_notify = true;
		}
	}

#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
	if (rdma_enable) {
		vd_layer[layer_id].cur_canvas_id =
			vd_layer[layer_id].next_canvas_id;
	} else {
		if (rdma_enable_pre) {
			/* force set state as normal without check */
			update_over_field_states(OVER_FIELD_NORMAL, true);
			return -1;
		}
		vd_layer[layer_id].cur_canvas_id = 0;
		vd_layer[layer_id].next_canvas_id = 1;
	}
#endif
	return 0;
}

static void vdx_misc_late_proc(u8 layer_id)
{
	if (layer_id == 0) {
		/* prevsync + postvsync case */
		if (cur_dev->pre_vsync_enable) {
			u32 pts_inc_scale_base = 0;

			if (frc_n2m_worked())
				pts_inc_scale_base = vsync_pts_inc_scale_base / 2;
			else
				pts_inc_scale_base = vsync_pts_inc_scale_base;
#ifdef CONFIG_AMLOGIC_VIDEOQUEUE
			videoqueue_pcrscr_update(vsync_pts_inc_scale,
				pts_inc_scale_base);
#endif
		} else {
#ifdef CONFIG_AMLOGIC_VIDEOQUEUE
			videoqueue_pcrscr_update(vsync_pts_inc_scale,
				vsync_pts_inc_scale_base);
#endif
		}
	}

#ifdef CONFIG_AMLOGIC_MEDIA_FRC
	if (cur_dev->vsync_2to1_enable &&
		layer_id == 0 &&
		vsync_count_start) {
		/* not prevsync not handle */
		if (!frc_drv_get_1st_frm())
			return;
	}
#endif
	if  (layer_id == 0) {
		if (vd_layer[0].dispbuf &&
		    (vd_layer[0].dispbuf->type & VIDTYPE_MVC))
			vd_layer[0].enable_3d_mode = mode_3d_mvc_enable;
		else if (process_3d_type)
			vd_layer[0].enable_3d_mode = mode_3d_enable;
		else
			vd_layer[0].enable_3d_mode = mode_3d_disable;
	}
	/* all frames has been renderred, so reset new frame flag */
	vd_layer[layer_id].new_frame = false;
}

int amvecm_update(u8 layer_id, u8 path_index, struct vframe_s *vf)
{
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_VECM
	return amvecm_on_vs
		((cur_dispbuf[path_index] != &vf_local[path_index])
		? cur_dispbuf[path_index] : NULL,
		vf, CSC_FLAG_CHECK_OUTPUT,
		0,
		0,
		0,
		0,
		0,
		0,
		layer_id,
		vd_layer[path_index].vpp_index);
#else
	return 0;
#endif
}

static struct vframe_s *do_pipx_toggle_frame
	(u8 path_index,
	struct path_id_s *path_id)
{
	struct vframe_s *vf;
	struct vframe_s *path_new_frame = NULL;
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
	struct vframe_s *dv_new_vf = NULL;
#endif
	s32 vd1_path_id, vd2_path_id, vd3_path_id;

	vf = video_vf_peek(path_index);
	video_get_vf_cnt[path_index] = 0;
	vd1_path_id = path_id->vd1_path_id;
	vd2_path_id = path_id->vd2_path_id;
	vd3_path_id = path_id->vd3_path_id;
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
	/* check video frame before VECM process */
	if (((vd1_path_id == VFM_PATH_PIP || is_multi_dv_mode()) ||
		vd1_path_id == VFM_PATH_PIP2) &&
		vf &&
	    is_amdv_enable()) {
		amdv_check_hdr10(vf);
		amdv_check_hdr10plus(vf);
		amdv_check_hlg(vf);
		amdv_check_primesl(vf);
		amdv_check_cuva(vf);
	}
#endif
	while (vf && !video_suspend) {
		if (!vf->frame_dirty) {
#if defined(CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_VECM)
			int iret1 = 0, iret2 = 0, iret3 = 0;
#endif

#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
			if ((vd1_path_id == VFM_PATH_PIP || is_multi_dv_mode() ||
				vd1_path_id == VFM_PATH_PIP2) &&
			    dolby_vision_need_wait(path_index))
				break;
#endif
#if defined(CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_VECM)
			if (vd1_path_id == VFM_PATH_PIP ||
				vd1_path_id == VFM_PATH_PIP2)
				iret1 = amvecm_update(VD1_PATH, path_index, vf);
			if (path_index == 1) {
				/* pip */
				if (vd2_path_id == VFM_PATH_DEF ||
				    vd2_path_id == VFM_PATH_PIP)
					iret2 = amvecm_update(VD2_PATH, path_index, vf);
			} else if (path_index == 2) {
				/* pip2 */
				if (vd2_path_id == VFM_PATH_PIP2)
					iret2 = amvecm_update(VD2_PATH, path_index, vf);
			}
			if (path_index == 1) {
				/* pip */
				if (vd3_path_id == VFM_PATH_PIP)
					iret3 = amvecm_update(VD3_PATH, path_index, vf);
			} else if (path_index == 2) {
				/* pip2 */
				if (vd3_path_id == VFM_PATH_DEF ||
					vd3_path_id == VFM_PATH_PIP2)
					iret3 = amvecm_update(VD3_PATH, path_index, vf);
			}
			if (iret1 == 1 || iret2 == 1 || iret3 == 1)
				break;
#endif
			vf = video_vf_get(path_index);
			if (vf) {
				video_get_vf_cnt[path_index]++;
				path_new_frame = pipx_toggle_frame(path_index, vf);
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
				if (vd1_path_id == VFM_PATH_PIP || is_multi_dv_mode() ||
					vd1_path_id == VFM_PATH_PIP2)
					dv_new_vf = dv_toggle_frame(vf, path_index, true);
#endif
			}
		} else {
			vf = video_vf_get(path_index);
			if (vf) {
				video_get_vf_cnt[path_index]++;
				if (video_vf_put(path_index, vf) < 0)
					check_dispbuf(path_index, vf, true);
			}
		}
		vf = video_vf_peek(path_index);
	}

	/* vsync_notify_videosync(); */
#ifdef CONFIG_AMLOGIC_VIDEOSYNC
	vsync_notify_videosync();
#endif

	if (video_get_vf_cnt[path_index] >= 2) {
		video_drop_vf_cnt[path_index] += (video_get_vf_cnt[path_index] - 1);
		if (debug_flag & DEBUG_FLAG_PRINT_DROP_FRAME)
			pr_info("videopip%d drop frame: drop count %d\n",
				path_index - 1,
				video_drop_vf_cnt[path_index]);
	}
	return path_new_frame;
}

static struct vframe_s *do_renderx_toggle_frame
	(u8 path_index,
	s32 *vd_path_id,
	struct path_id_s *path_id)
{
	struct vframe_s *path_new_frame = NULL;
#if defined(CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_VECM)
	u16 line = glayer_info[0].layer_top;
#endif

	/* video_render.x toggle frame */
	if (gvideo_recv[path_index]) {
		path_new_frame =
			gvideo_recv[path_index]->func->dequeue_frame
				(gvideo_recv[path_index], path_id);
		if (path_index == 0) {
			if (path_new_frame &&
				tvin_vf_is_keeped(path_new_frame)) {
				new_frame_count = 0;
			} else if (path_new_frame) {
				new_frame_count = gvideo_recv[0]->frame_count;
				hdmi_in_delay_maxmin_new(path_new_frame);
			} else if (gvideo_recv[0]->cur_buf) {
				if (tvin_vf_is_keeped(gvideo_recv[0]->cur_buf))
					new_frame_count = 0;
			}
#if defined(CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_VECM)
			if (vd_path_id[0] == VFM_PATH_VIDEO_RENDER0 &&
				cur_frame_par[0]) {
				/*need call every vsync*/
				if (path_new_frame)
					frame_lock_process(path_new_frame,
						cur_frame_par[0], line);
				else if (vd_layer[0].dispbuf)
					frame_lock_process(vd_layer[0].dispbuf,
						cur_frame_par[0], line);
				else
					frame_lock_process(NULL, cur_frame_par[0], line);
			}
#endif
		}
	}
	return path_new_frame;
}

static struct vframe_s *video_toggle_frame
	(u8 layer_id,
	u8 fake_layer_id,
	s32 *vd_path_id,
	struct path_id_s *path_id)
{
	u8 func_id = 0;
	u8 path_index = 0;
	struct vframe_s *path_new_frame = NULL;

	over_field = false;
#ifdef CONFIG_AMLOGIC_MEDIA_FRC
	if (cur_dev->vsync_2to1_enable &&
		layer_id == 0 &&
		vsync_count_start) {
		/* not prevsync not handle */
		if (!frc_drv_get_1st_frm())
			return NULL;
	}
#endif
	if (layer_id == 0xff)
		func_id = vd_fake_func[fake_layer_id].fake_func_id;
	else if (layer_id < MAX_VD_LAYER)
		func_id = vd_layer[layer_id].func_path_id;
	switch (func_id) {
	case AMVIDEO:
		path_new_frame = amvideo_toggle_frame(vd_path_id);
		break;
	case PIP1:
	case PIP2:
		path_index = func_id - AMVIDEO;
		if (path_index < MAX_VD_LAYER)
			path_new_frame = do_pipx_toggle_frame(path_index, path_id);
		break;
	case RENDER0:
	case RENDER1:
	case RENDER2:
		path_index = func_id - RENDER0;
		if (path_index < MAX_VD_LAYER)
			path_new_frame = do_renderx_toggle_frame(path_index, vd_path_id, path_id);
		break;
	default:
		break;
	}
	if (cur_dev->vsync_2to1_enable &&
		layer_id == 0 &&
		path_new_frame)
		new_frame_cnt++;
	return path_new_frame;
}

static struct vframe_s *vdx_swap_frame(u8 layer_id,
				s32 vdx_path_id,
				s32 cur_vdx_path_id,
				struct vframe_s **path_new_frame)
{
	struct vframe_s *new_frame = NULL;
	u32 cur_blackout;
	int source_type = 0;
	int axis[4];
	int crop[4];
	u8 i = 0;

	vd_layer[layer_id].force_switch_mode = force_switch_vf_mode;

	/* i is path_index */
	for (i = 0; i < MAX_VD_LAYER; i++) {
		if (vd_layer[layer_id].dispbuf_mapping == &cur_dispbuf[i] &&
		    (cur_dispbuf[i] == &vf_local[i] ||
		     !cur_dispbuf[i]) &&
		    vd_layer[layer_id].dispbuf != cur_dispbuf[i])
			vd_layer[layer_id].dispbuf = cur_dispbuf[i];
	}

	for (i = 0; i < MAX_VD_LAYER; i++) {
		if (gvideo_recv[i] &&
		    vd_layer[layer_id].dispbuf_mapping == &gvideo_recv[i]->cur_buf &&
		    (gvideo_recv[i]->cur_buf == &gvideo_recv[i]->local_buf ||
		     !gvideo_recv[i]->cur_buf) &&
		    vd_layer[layer_id].dispbuf != gvideo_recv[i]->cur_buf)
			vd_layer[layer_id].dispbuf = gvideo_recv[i]->cur_buf;
	}

	if (vd_layer[layer_id].switch_vf &&
		vd_layer[layer_id].dispbuf &&
		(vd_layer[layer_id].dispbuf->vf_ext ||
		 vd_layer[layer_id].dispbuf->uvm_vf)) {
		/* select uvm_vf first */
		if (vd_layer[layer_id].dispbuf->uvm_vf)
			vd_layer[layer_id].vf_ext =
				vd_layer[layer_id].dispbuf->uvm_vf;
		else
			vd_layer[layer_id].vf_ext =
				(struct vframe_s *)vd_layer[layer_id].dispbuf->vf_ext;
	} else {
		vd_layer[layer_id].vf_ext = NULL;
	}

	/* vdx config */
	if (gvideo_recv[0] &&
	    gvideo_recv[0]->path_id == vdx_path_id) {
		/* video_render.0 display on VDx */
		new_frame = path_new_frame[3];
		if (!new_frame) {
			if (!gvideo_recv[0]->cur_buf) {
				/* video_render.0 no frame in display */
				if (cur_vdx_path_id != vdx_path_id)
					safe_switch_videolayer
						(layer_id, false, true);
				vd_layer[layer_id].dispbuf = NULL;
			} else if (gvideo_recv[0]->cur_buf ==
				&gvideo_recv[0]->local_buf) {
				/* video_render.0 keep frame */
				vd_layer[layer_id].dispbuf =
					gvideo_recv[0]->cur_buf;
			} else if (vd_layer[layer_id].dispbuf
				!= gvideo_recv[0]->cur_buf) {
				/* video_render.0 has frame in display */
				new_frame = gvideo_recv[0]->cur_buf;
			}
		}
		if (new_frame || gvideo_recv[0]->cur_buf)
			vd_layer[layer_id].dispbuf_mapping =
				&gvideo_recv[0]->cur_buf;
		cur_blackout = 1;
	} else if (gvideo_recv[1] &&
	    (gvideo_recv[1]->path_id == vdx_path_id)) {
		/* video_render.1 display on VDx */
		new_frame = path_new_frame[4];
		if (!new_frame) {
			if (!gvideo_recv[1]->cur_buf) {
				/* video_render.1 no frame in display */
				if (cur_vdx_path_id != vdx_path_id)
					safe_switch_videolayer
						(layer_id, false, true);
				vd_layer[layer_id].dispbuf = NULL;
			} else if (gvideo_recv[1]->cur_buf ==
				&gvideo_recv[1]->local_buf) {
				/* video_render.1 keep frame */
				vd_layer[layer_id].dispbuf =
					gvideo_recv[1]->cur_buf;
			} else if (vd_layer[layer_id].dispbuf
				!= gvideo_recv[1]->cur_buf) {
				/* video_render.1 has frame in display */
				new_frame = gvideo_recv[1]->cur_buf;
			}
		}
		if (new_frame || gvideo_recv[1]->cur_buf)
			vd_layer[layer_id].dispbuf_mapping =
				&gvideo_recv[1]->cur_buf;
		cur_blackout = 1;
	} else if (gvideo_recv[2] &&
	    (gvideo_recv[2]->path_id == vdx_path_id)) {
		/* video_render.2 display on VDx */
		new_frame = path_new_frame[5];
		if (!new_frame) {
			if (!gvideo_recv[2]->cur_buf) {
				/* video_render.2 no frame in display */
				if (cur_vdx_path_id != vdx_path_id)
					safe_switch_videolayer
						(layer_id, false, true);
				vd_layer[layer_id].dispbuf = NULL;
			} else if (gvideo_recv[2]->cur_buf ==
				&gvideo_recv[2]->local_buf) {
				/* video_render.2 keep frame */
				vd_layer[layer_id].dispbuf =
					gvideo_recv[2]->cur_buf;
			} else if (vd_layer[layer_id].dispbuf
				!= gvideo_recv[2]->cur_buf) {
				/* video_render.2 has frame in display */
				new_frame = gvideo_recv[2]->cur_buf;
			}
		}
		if (new_frame || gvideo_recv[2]->cur_buf)
			vd_layer[layer_id].dispbuf_mapping =
				&gvideo_recv[2]->cur_buf;
		cur_blackout = 1;
	} else if (vdx_path_id == VFM_PATH_AMVIDEO) {
		/* primary display in VDx */
		new_frame = path_new_frame[0];
		if (!new_frame) {
			if (!cur_dispbuf[0]) {
				/* primary no frame in display */
				if (cur_vdx_path_id != vdx_path_id)
					safe_switch_videolayer
						(layer_id, false, true);
				vd_layer[layer_id].dispbuf = NULL;
			} else if (cur_dispbuf[0] == &vf_local[0]) {
				/* primary keep frame */
				vd_layer[layer_id].dispbuf = cur_dispbuf[0];
			} else if (vd_layer[layer_id].dispbuf
				!= cur_dispbuf[0]) {
				/* primary has frame in display */
				new_frame = cur_dispbuf[0];
			}
		}
		if (new_frame || cur_dispbuf[0])
			vd_layer[layer_id].dispbuf_mapping = &cur_dispbuf[0];
		cur_blackout = blackout[0] | force_blackout;
	} else if (vdx_path_id == VFM_PATH_PIP) {
		/* pip display in VDx */
		new_frame = path_new_frame[1];
		if (!new_frame) {
			if (!cur_dispbuf[1]) {
				/* pip no display frame */
				if (cur_vdx_path_id != vdx_path_id)
					safe_switch_videolayer
						(layer_id, false, true);
				vd_layer[layer_id].dispbuf = NULL;
			} else if (cur_dispbuf[1] == &vf_local[1]) {
				/* pip keep frame */
				vd_layer[layer_id].dispbuf = cur_dispbuf[1];
			} else if (vd_layer[layer_id].dispbuf
				!= cur_dispbuf[1]) {
				/* pip has frame in display */
				new_frame = cur_dispbuf[1];
			}
		}
		if (new_frame || cur_dispbuf[1])
			vd_layer[layer_id].dispbuf_mapping = &cur_dispbuf[1];
		cur_blackout = blackout[1] | force_blackout;
	} else if (vdx_path_id == VFM_PATH_PIP2) {
		/* pip2 display in VDx */
		new_frame = path_new_frame[2];
		if (!new_frame) {
			if (!cur_dispbuf[2]) {
				/* pip2 no display frame */
				if (cur_vdx_path_id != vdx_path_id)
					safe_switch_videolayer
						(layer_id, false, true);
				vd_layer[layer_id].dispbuf = NULL;
			} else if (cur_dispbuf[2] == &vf_local[1]) {
				/* pip2 keep frame */
				vd_layer[layer_id].dispbuf = cur_dispbuf[2];
			} else if (vd_layer[layer_id].dispbuf
				!= cur_dispbuf[2]) {
				/* pip2 has frame in display */
				new_frame = cur_dispbuf[2];
			}
		}
		if (new_frame || cur_dispbuf[2])
			vd_layer[layer_id].dispbuf_mapping = &cur_dispbuf[2];
		cur_blackout = blackout[2] | force_blackout;
	} else if (vdx_path_id != VFM_PATH_INVALID) {
		switch (layer_id) {
		case 0:
		/* primary display on VDx */
		new_frame = path_new_frame[0];
		if (vdx_path_id == VFM_PATH_AUTO) {
			if (path_new_frame[3] &&
				(path_new_frame[3]->flag &
					VFRAME_FLAG_FAKE_FRAME)) {
				new_frame = path_new_frame[3];
				/* path3 is render0 */
				pr_info("vsync: auto path3 get a fake\n");
			}
			if (!new_frame) {
				if (cur_dispbuf[0] == &vf_local[0])
					vd_layer[layer_id].dispbuf =
						cur_dispbuf[0];
			}
			if (gvideo_recv[0]->cur_buf &&
				gvideo_recv[0]->cur_buf->flag &
					VFRAME_FLAG_FAKE_FRAME)
				vd_layer[layer_id].dispbuf =
					gvideo_recv[0]->cur_buf;
			//new add:
			if (new_frame || cur_dispbuf[0]) {
				if (new_frame && path_new_frame[3] == new_frame)
					vd_layer[0].dispbuf_mapping =
						&gvideo_recv[0]->cur_buf;
				else
					vd_layer[0].dispbuf_mapping = &cur_dispbuf[0];
			}
		} else {
			if (!new_frame) {
				if (!cur_dispbuf[0]) {
					/* primary no frame in display */
					if (cur_vdx_path_id != vdx_path_id)
						safe_switch_videolayer
							(layer_id, false, true);
					vd_layer[layer_id].dispbuf = NULL;
				} else if (cur_dispbuf[0] == &vf_local[0]) {
					/* primary keep frame */
					vd_layer[layer_id].dispbuf =
						cur_dispbuf[0];
				} else if (vd_layer[layer_id].dispbuf
					!= cur_dispbuf[0]) {
					/* primary has frame in display */
					new_frame = cur_dispbuf[0];
				}
			}
			if (new_frame || cur_dispbuf[0])
				vd_layer[layer_id].dispbuf_mapping = &cur_dispbuf[0];
		}

		cur_blackout = blackout[0] | force_blackout;
		break;
		case 1:
		case 2:
		/* pip1/2 display in VDx */
		i = layer_id;
		new_frame = path_new_frame[i];
		if (!new_frame) {
			if (!cur_dispbuf[i]) {
				/* pip no display frame */
				if (cur_vdx_path_id != vdx_path_id)
					safe_switch_videolayer
						(layer_id, false, true);
				vd_layer[layer_id].dispbuf = NULL;
			} else if (cur_dispbuf[i] == &vf_local[i]) {
				/* pip keep frame */
				vd_layer[layer_id].dispbuf = cur_dispbuf[i];
			} else if (vd_layer[layer_id].dispbuf
				!= cur_dispbuf[i]) {
				new_frame = cur_dispbuf[i];
			}
		}
		if (new_frame || cur_dispbuf[i])
			vd_layer[layer_id].dispbuf_mapping = &cur_dispbuf[i];
		cur_blackout = blackout[i] | force_blackout;
		break;
		}
	} else {
		cur_blackout = 1;
	}

	/* vout mode detection under new non-tunnel mode */
	if (vd_layer[layer_id].dispbuf) {
		if (strcmp(old_vmode, new_vmode)) {
			vd_layer[0].property_changed = true;
			vd_layer[1].property_changed = true;
			vd_layer[2].property_changed = true;
			pr_debug("detect vout mode change!!!!!!!!!!!!\n");
			strcpy(old_vmode, new_vmode);
		}
	}
	/* vd1 special case */
	if (layer_id == 0) {
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
		if (is_amdv_enable() &&
			vd_layer[layer_id].global_output) {
			/* no new frame but path switched case, */
			if (new_frame && !is_local_vf(new_frame) &&
			    (!path_new_frame[0] || new_frame !=
				path_new_frame[0]) &&
			    (!path_new_frame[1] || new_frame !=
				path_new_frame[1]) &&
			    (!path_new_frame[2] || new_frame !=
				path_new_frame[2]) &&
			    (!path_new_frame[3] || new_frame !=
				path_new_frame[3]) &&
			    (!path_new_frame[4] || new_frame !=
				path_new_frame[4]) &&
			    (!path_new_frame[5] || new_frame !=
				path_new_frame[5]))
				amdv_update_src_format(new_frame, 1, VD1_PATH);
			else if (!new_frame &&
				 vd_layer[layer_id].dispbuf &&
				 !is_local_vf(vd_layer[layer_id].dispbuf))
				amdv_update_src_format(vd_layer[0].dispbuf, 0, VD1_PATH);
			/* pause and video off->on case */
		}
#endif
	}

	if (!new_frame && vd_layer[layer_id].dispbuf &&
	    is_local_vf(vd_layer[layer_id].dispbuf)) {
		if (cur_blackout) {
			vd_layer[layer_id].property_changed = false;
		} else if (vd_layer[layer_id].dispbuf) {
			if ((layer_id == 0 &&
				!is_di_post_mode(vd_layer[layer_id].dispbuf) &&
				!is_pre_link_on(&vd_layer[layer_id])) ||
				layer_id != 0) {
				if (vd_layer[layer_id].switch_vf &&
					vd_layer[layer_id].vf_ext)
					vd_layer[layer_id].vf_ext->canvas0Addr =
						get_layer_display_canvas(layer_id);
				else
					vd_layer[layer_id].dispbuf->canvas0Addr =
						get_layer_display_canvas(layer_id);
			}
		}
	}

	if (vd_layer[layer_id].dispbuf &&
		(vd_layer[layer_id].dispbuf->flag & (VFRAME_FLAG_VIDEO_COMPOSER |
		VFRAME_FLAG_VIDEO_DRM)) &&
		!(vd_layer[layer_id].dispbuf->flag & VFRAME_FLAG_FAKE_FRAME) &&
		!(debug_flag & DEBUG_FLAG_AXIS_NO_UPDATE)) {
		int mirror = 0;

		axis[0] = vd_layer[layer_id].dispbuf->axis[0];
		axis[1] = vd_layer[layer_id].dispbuf->axis[1];
		axis[2] = vd_layer[layer_id].dispbuf->axis[2];
		axis[3] = vd_layer[layer_id].dispbuf->axis[3];
		crop[0] = vd_layer[layer_id].dispbuf->crop[0];
		crop[1] = vd_layer[layer_id].dispbuf->crop[1];
		crop[2] = vd_layer[layer_id].dispbuf->crop[2];
		crop[3] = vd_layer[layer_id].dispbuf->crop[3];
		_set_video_window(&glayer_info[layer_id], axis);
		source_type = vd_layer[layer_id].dispbuf->source_type;
		if (source_type != VFRAME_SOURCE_TYPE_HDMI &&
			source_type != VFRAME_SOURCE_TYPE_CVBS &&
			source_type != VFRAME_SOURCE_TYPE_TUNER &&
			source_type != VFRAME_SOURCE_TYPE_HWC)
			_set_video_crop(&glayer_info[layer_id], crop);
		if (vd_layer[layer_id].dispbuf->flag & VFRAME_FLAG_MIRROR_H)
			mirror = H_MIRROR;
		if (vd_layer[layer_id].dispbuf->flag & VFRAME_FLAG_MIRROR_V)
			mirror |= V_MIRROR;
		_set_video_mirror(&glayer_info[layer_id], mirror);
		set_alpha_scpxn(&vd_layer[layer_id], vd_layer[layer_id].dispbuf->composer_info);
		glayer_info[layer_id].zorder = vd_layer[layer_id].dispbuf->zorder;
	} else {
		_set_video_mirror(&glayer_info[layer_id], 0);
	}

	return new_frame;
}

#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
#ifdef CONFIG_AMLOGIC_MEDIA_FRC
static void force_switch_slice(void)
{
	const struct vinfo_s *vinfo = get_current_vinfo();
	u32 slice_num;
	u32 switch_flag = 0;

	if (!video_is_meson_t3x_cpu())
		return;

	switch_flag = frc_ready_to_switch();
	if (switch_flag != 1 && switch_flag != 2)
		return;

	if (vinfo && (vinfo->width > 1920 && vinfo->height > 1080 &&
		(vinfo->sync_duration_num /
		vinfo->sync_duration_den > 60))) {
		/* for t3x */
		if (switch_flag == 1) {
			/* frc is ready on */
			/* 4k120hz and frc_n2m_worked 1 slice */
			slice_num = 1;
			if (slice_num != vd_layer[0].slice_num) {
				video_prop_status |= VIDEO_PROP_CHANGE_SLICE_NUM;
				if (debug_flag)
					pr_info("%s slice_num=%d-> %d, video_prop_status=%d\n",
						__func__,
						vd_layer[0].slice_num, slice_num,
						video_prop_status);
			}
			vd_layer[0].slice_num = slice_num;
			vd_layer[0].property_changed = true;
			if (debug_common_flag & DEBUG_FLAG_COMMON_FRC)
				pr_info("%s:slice_num=%d\n",
					__func__, slice_num);
		} else if (switch_flag == 2) {
			/* frc is ready off */
			/* 4k120hz and frc_n2m_not_worked 2 slice */
			slice_num = 2;
			if (slice_num != vd_layer[0].slice_num) {
				video_prop_status |= VIDEO_PROP_CHANGE_SLICE_NUM;
				if (debug_flag)
					pr_info("%s slice_num=%d-> %d, video_prop_status=%d\n",
						__func__,
						vd_layer[0].slice_num, slice_num,
						video_prop_status);
			}
			if (is_aisr_enable(&vd_layer[0]))
				vd_layer[0].aisr_mif_setting.aisr_enable = 0;
			vd_layer[0].slice_num = slice_num;
			vd_layer[0].property_changed = true;
			if (debug_common_flag & DEBUG_FLAG_COMMON_FRC)
				pr_info("%s:slice_num=%d\n",
					__func__, slice_num);
		}
	}
}

#endif

bool force_switch_to_2slice(void)
{
	const struct vinfo_s *vinfo = get_current_vinfo();
	u32 slice_num;

	if (!video_is_meson_t3x_cpu())
		return false;

	if (vinfo && (vinfo->width > 1920 && vinfo->height > 1080 &&
		(vinfo->sync_duration_num /
		vinfo->sync_duration_den > 60))) {
		/* for t3x */
		/* frc is ready off */
		/* 4k120hz and frc_n2m_not_worked 2 slice */
		slice_num = 2;
		if (debug_common_flag & DEBUG_FLAG_COMMON_FRC)
			pr_info("%s:slice_num = %d gslice_num = %d\n",
				__func__, slice_num, vd_layer[0].slice_num);
		if (slice_num != vd_layer[0].slice_num) {
			video_prop_status |= VIDEO_PROP_CHANGE_SLICE_NUM;
			if (debug_flag)
				pr_info("%s slice_num=%d-> %d, video_prop_status=%d\n",
					__func__,
					vd_layer[0].slice_num, slice_num,
					video_prop_status);
			if (is_aisr_enable(&vd_layer[0]))
				vd_layer[0].aisr_mif_setting.aisr_enable = 0;
			vd_layer[0].slice_num = slice_num;
			vd_layer[0].property_changed = true;
			return true;
		}
	}
	return false;
}
#endif

static void do_vd1_swap_frame(u8 layer_id,
				s32 vd1_path_id,
				s32 cur_vd1_path_id,
				struct vframe_s **path_new_frame)
{
	struct vframe_s *new_frame = NULL;
	enum vframe_signal_fmt_e fmt;
	int source_type = 0;

#ifdef CONFIG_AMLOGIC_MEDIA_FRC
	if (cur_dev->vsync_2to1_enable &&
		layer_id == 0 &&
		vsync_count_start) {
		/* not prevsync not handle */
		if (!frc_drv_get_1st_frm())
			return;
	}
#endif
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
#ifdef CONFIG_AMLOGIC_MEDIA_FRC
	force_switch_slice();
#endif
#endif
	new_frame = vdx_swap_frame(0, vd1_path_id,
				  cur_vd1_path_id,
				  path_new_frame);
	/* setting video display property in underflow mode */
	if (!new_frame &&
		vd_layer[0].dispbuf &&
		(vd_layer[0].property_changed ||
		is_picmode_changed(0, vd_layer[0].dispbuf))) {
		primary_swap_frame(&vd_layer[0], vd_layer[0].dispbuf, __LINE__);
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
		dvel_swap_frame(cur_dispbuf2);
#endif
	} else if (new_frame) {
		new_frame_mask |= 1;
		vframe_walk_delay = (int)div_u64(((jiffies_64 -
			new_frame->ready_jiffies64) * 1000), HZ);
		vframe_walk_delay += 1000 *
			vsync_pts_inc_scale / vsync_pts_inc_scale_base;
		vframe_walk_delay -= new_frame->duration / 96;
#ifdef CONFIG_AMLOGIC_MEDIA_FRC
		vframe_walk_delay += frc_get_video_latency();
#endif
		primary_swap_frame(&vd_layer[0], new_frame, __LINE__);
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
		dvel_swap_frame(cur_dispbuf2);
#endif
	}

	/* TODO: need check more vd layer, now only vd1 */
	if (vd_layer[0].dispbuf &&
		(atomic_read(&vt_unreg_flag) ||
		tvin_vf_is_keeped(vd_layer[0].dispbuf))) {
		source_type = vd_layer[0].dispbuf->source_type;
		/* TODO: change new flag to detect video tunnel path */
		if (source_type == VFRAME_SOURCE_TYPE_HDMI ||
		    source_type == VFRAME_SOURCE_TYPE_CVBS) {
			if (!vd_layer[0].force_disable) {
				safe_switch_videolayer(0, false, true);
				atomic_set(&vt_disable_video_done, 0);
			}
			vd_layer[0].force_disable = true;
		} else {
			if (vd_layer[0].force_disable &&
			    vd_layer[0].global_output &&
			    !vd_layer[0].disable_video)
				safe_switch_videolayer(0, true, true);
			vd_layer[0].force_disable = false;
		}
	} else {
		if (vd_layer[0].force_disable &&
		    vd_layer[0].global_output &&
		    !vd_layer[0].disable_video)
			safe_switch_videolayer(0, true, true);
		vd_layer[0].force_disable = false;
	}
	if (cur_frame_par[0]) {
		if (cur_dev->aisr_enable &&
		   cur_dev->aisr_frame_parms.aisr_enable)
			cur_frame_par[0]->aisr_enable = 1;
		else
			cur_frame_par[0]->aisr_enable = 0;
	}

#ifdef CONFIG_AMLOGIC_MEDIA_FRC
	update_frc_in_size(&vd_layer[0]);
	frc_input_handle(vd_layer[0].dispbuf, vd_layer[0].next_frame_par);
	vpu_set_frc_bypass(&vd_layer[0]);
#endif
	if (atomic_read(&axis_changed)) {
		video_prop_status |= VIDEO_PROP_CHANGE_AXIS;
		atomic_set(&axis_changed, 0);
	}

	if (vd1_path_id == VFM_PATH_AMVIDEO ||
	    vd1_path_id == VFM_PATH_DEF)
		vd_layer[0].keep_frame_id = 0;
	else if (vd1_path_id == VFM_PATH_PIP)
		vd_layer[0].keep_frame_id = 1;
	else if (vd1_path_id == VFM_PATH_PIP2)
		vd_layer[0].keep_frame_id = 2;
	else
		vd_layer[0].keep_frame_id = 0xff;

#if defined(CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_VECM)
	struct vpp_frame_par_s *frame_par = NULL;

	if (vd_layer[0].next_frame_par)
		frame_par = vd_layer[0].next_frame_par;
	else
		frame_par = vd_layer[0].cur_frame_par;
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
#if defined(CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_VECM)
	if (new_frame)
		vpp_new_frame = 1;
	else
		vpp_new_frame = 0;
#endif
#endif

	refresh_on_vs(new_frame, vd_layer[0].dispbuf, vd_layer[0].vpp_index);

	amvecm_on_vs
		(!is_local_vf(vd_layer[0].dispbuf)
		? vd_layer[0].dispbuf : NULL,
		new_frame,
		new_frame ? CSC_FLAG_TOGGLE_FRAME : 0,
		frame_par ?
		frame_par->supsc1_hori_ratio :
		0,
		frame_par ?
		frame_par->supsc1_vert_ratio :
		0,
		frame_par ?
		frame_par->spsc1_w_in :
		0,
		frame_par ?
		frame_par->spsc1_h_in :
		0,
		frame_par ?
		frame_par->cm_input_w :
		0,
		frame_par ?
		frame_par->cm_input_h :
		0,
		VD1_PATH,
		vd_layer[0].vpp_index);
#endif
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_PRIME_SL
	prime_sl_process(vd_layer[0].dispbuf);
#endif

	if ((new_frame || vd_layer[0].dispbuf) &&
		get_video_enabled(0)) {
		if (new_frame) {
			if (new_frame->ext_signal_type & 0x1) {
				if (!atomic_read(&fmm_changed)) {
					video_prop_status |= VIDEO_PROP_CHANGE_FMM;
					atomic_set(&fmm_changed, 1);
					if (debug_flag & DEBUG_FLAG_TRACE_EVENT)
						pr_info("VD1 FMM changed: %d->. cur:%p, new:%p\n",
						new_frame ?
						(new_frame->ext_signal_type & 0x1) :
						(vd_layer[0].dispbuf->ext_signal_type & 0x1),
						vd_layer[0].dispbuf, new_frame);
				}
			} else {
				if (atomic_read(&fmm_changed)) {
					video_prop_status |= VIDEO_PROP_CHANGE_FMM_DISABLE;
					atomic_set(&fmm_changed, 0);
					if (debug_flag & DEBUG_FLAG_TRACE_EVENT)
						pr_info("VD1 FMM changed: %d->. cur:%p, new:%p\n",
						new_frame ?
						(new_frame->ext_signal_type & 0x1) :
						(vd_layer[0].dispbuf->ext_signal_type & 0x1),
						vd_layer[0].dispbuf, new_frame);
				}
			}
		}
	} else {
		if (atomic_read(&fmm_changed)) {
			video_prop_status |= VIDEO_PROP_CHANGE_FMM_DISABLE;
			atomic_set(&fmm_changed, 0);
		}
	}

	/* work around which dec/vdin don't call update src_fmt function */
	if (vd_layer[0].dispbuf && !is_local_vf(vd_layer[0].dispbuf)) {
		int new_src_fmt = -1;
		u32 src_map[] = {
			VFRAME_SIGNAL_FMT_INVALID,
			VFRAME_SIGNAL_FMT_HDR10,
			VFRAME_SIGNAL_FMT_HDR10PLUS,
			VFRAME_SIGNAL_FMT_DOVI,
			VFRAME_SIGNAL_FMT_HDR10PRIME,
			VFRAME_SIGNAL_FMT_HLG,
			VFRAME_SIGNAL_FMT_SDR,
			VFRAME_SIGNAL_FMT_MVC,
			VFRAME_SIGNAL_FMT_CUVA_HDR,
			VFRAME_SIGNAL_FMT_CUVA_HLG,
			VFRAME_SIGNAL_FMT_SDR_2020
		};

#if defined(CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_VECM)
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
		if (is_amdv_on())
			new_src_fmt = get_amdv_src_format(VD1_PATH);
		else
#endif
			new_src_fmt =
				(int)get_cur_source_type(VD1_PATH, VPP_TOP0);
#endif
		/*coverity[dead_error_line] error report*/
		if (new_src_fmt > 0 && new_src_fmt < MAX_SOURCE)
			fmt = (enum vframe_signal_fmt_e)src_map[new_src_fmt];
		else
			fmt = VFRAME_SIGNAL_FMT_INVALID;
		if (fmt != atomic_read(&cur_primary_src_fmt)) {
			/* atomic_set(&primary_src_fmt, fmt); */
			if (debug_flag & DEBUG_FLAG_TRACE_EVENT) {
				char *old_str = NULL, *new_str = NULL;
				enum vframe_signal_fmt_e old_fmt;

				old_fmt = (enum vframe_signal_fmt_e)
					atomic_read(&cur_primary_src_fmt);
				if (old_fmt != VFRAME_SIGNAL_FMT_INVALID)
					old_str = (char *)src_fmt_str[old_fmt];
				if (fmt != VFRAME_SIGNAL_FMT_INVALID) {
					/*coverity[dead_error_line] error report*/
					new_str = (char *)src_fmt_str[fmt];
				}
				pr_info("VD1 src fmt changed: %s->%s. vf: %p, signal_type:0x%x\n",
					old_str ? old_str : "invalid",
					new_str ? new_str : "invalid",
					vd_layer[0].dispbuf,
					vd_layer[0].dispbuf->signal_type);
			}
			atomic_set(&cur_primary_src_fmt, fmt);
			atomic_set(&primary_src_fmt, fmt);
			video_prop_status |= VIDEO_PROP_CHANGE_FMT;
			update_primary_fmt_event();
		}
	}
}

static void do_vdx_swap_frame(u8 layer_id,
				s32 vd_path_id,
				s32 cur_vd_path_id,
				struct vframe_s **path_new_frame)
{
	struct vframe_s *new_frame = NULL;

	new_frame = vdx_swap_frame(layer_id, vd_path_id,
				   cur_vd_path_id,
				   path_new_frame);

	/* setting video display property in underflow mode */
	if (!new_frame &&
		vd_layer[layer_id].dispbuf &&
		(vd_layer[layer_id].property_changed ||
		is_picmode_changed(layer_id, vd_layer[layer_id].dispbuf))) {
		pipx_swap_frame(&vd_layer[layer_id],
			vd_layer[layer_id].dispbuf, vinfo);
		need_disable_vd[layer_id] = false;
	} else if (new_frame) {
		new_frame_mask |= 2;
		pipx_swap_frame(&vd_layer[layer_id], new_frame, vinfo);
		need_disable_vd[layer_id] = false;
	}

	if (layer_id == 1) {
		if (vd_path_id == VFM_PATH_PIP ||
		    vd_path_id == VFM_PATH_DEF)
			vd_layer[layer_id].keep_frame_id = 1;
		else if (vd_path_id == VFM_PATH_PIP2)
			vd_layer[layer_id].keep_frame_id = 2;
		else if (vd_path_id == VFM_PATH_AMVIDEO)
			vd_layer[layer_id].keep_frame_id = 0;
		else
			vd_layer[layer_id].keep_frame_id = 0xff;
	} else if (layer_id == 2) {
		if (vd_path_id == VFM_PATH_PIP2 ||
			vd_path_id == VFM_PATH_DEF)
			vd_layer[layer_id].keep_frame_id = 2;
		else if (vd_path_id == VFM_PATH_PIP)
			vd_layer[layer_id].keep_frame_id = 1;
		else if (vd_path_id == VFM_PATH_AMVIDEO)
			vd_layer[layer_id].keep_frame_id = 0;
		else
			vd_layer[layer_id].keep_frame_id = 0xff;
	}

#if defined(CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_VECM)
	struct vpp_frame_par_s *frame_par = NULL;

	if (vd_layer[layer_id].next_frame_par)
		frame_par = vd_layer[layer_id].next_frame_par;
	else
		frame_par = vd_layer[layer_id].cur_frame_par;

	amvecm_on_vs
		(!is_local_vf(vd_layer[layer_id].dispbuf)
		? vd_layer[layer_id].dispbuf : NULL,
		new_frame,
		new_frame ? CSC_FLAG_TOGGLE_FRAME : 0,
		frame_par ?
		frame_par->supsc1_hori_ratio :
		0,
		frame_par ?
		frame_par->supsc1_vert_ratio :
		0,
		frame_par ?
		frame_par->spsc1_w_in :
		0,
		frame_par ?
		frame_par->spsc1_h_in :
		0,
		frame_par ?
		frame_par->cm_input_w :
		0,
		frame_par ?
		frame_par->cm_input_h :
		0,
		layer_id,
		vd_layer[layer_id].vpp_index);
#endif

	if (need_disable_vd[layer_id]) {
		safe_switch_videolayer(layer_id, false, true);
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
		/* reset dvel statue when disable vd2 */
		if (layer_id == 1)
			dvel_status = false;
#endif
	if (vd_layer[layer_id].dispbuf &&
	    (vd_layer[layer_id].dispbuf->flag & VFRAME_FLAG_FAKE_FRAME))
		safe_switch_videolayer(layer_id, false, true);
	}
}

static int misc_early_proc(void)
{
	u32 next_afbc_request = atomic_read(&gafbc_request);
	s32 vout_type;
	bool rdma_enable, _rdma_enable_pre;
	int i, ret = 0;

#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
	set_vsync_rdma_id(VSYNC_RDMA);

	if (is_vsync_rdma_enable())
		over_field_info_record();
#endif

#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
	if (is_amdv_on())
		amdv_update_backlight();
#endif

	vout_type = detect_vout_type(vinfo);

	for (i = 0; i < cur_dev->max_vd_layers; i++) {
		glayer_info[i].need_no_compress =
			(next_afbc_request & (i + 1)) ? true : false;
		vd_layer[i].bypass_pps = bypass_pps;
		vd_layer[i].global_debug = debug_flag;
		vd_layer[i].vout_type = vout_type;
		vd_layer[i].display_cnt++;
	}

#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
	set_dv_provide_name();
#endif

	vsync_count++;
	timer_count++;

#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
	vsync_rdma_config_pre();

	if (debug_flag & DEBUG_FLAG_PRINT_RDMA) {
		if (vd_layer[0].property_changed) {
			enable_rdma_log_count = 5;
			enable_rdma_log(1);
		}
	if (enable_rdma_log_count > 0)
		enable_rdma_log_count--;
	}
#endif

	rdma_enable = is_vsync_rdma_enable();
	_rdma_enable_pre = rdma_enable_pre;
	for (i = 0; i < cur_dev->max_vd_layers; i++)
		if (vd_layer[i].vd_func.vd_misc_early_proc)
			ret = vd_layer[i].vd_func.vd_misc_early_proc
				(i, rdma_enable, _rdma_enable_pre);
	return ret;
}

static void misc_late_proc(void)
{
	struct cur_line_info_t *cur_line_info = NULL;
	int enc_line;
	int i;

	if (first_irq) {
		first_irq = false;
		goto RUN_FIRST_RDMA;
	}
	if (new_frame_mask && is_vsync_rdma_enable())
		update_over_field_states(OVER_FIELD_NEW_VF, false);

	for (i = 0; i < cur_dev->max_vd_layers; i++)
		if (vd_layer[i].vd_func.vd_misc_late_proc)
			vd_layer[i].vd_func.vd_misc_late_proc(i);

#if defined(PTS_LOGGING) || defined(PTS_TRACE_DEBUG)
	pts_trace++;
#endif

#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
	/* vsync_rdma_config(); */
RUN_FIRST_RDMA:
	vsync_rdma_process();
	set_vd_pi_input_size();
	enc_line = get_cur_enc_line();
	cur_line_info = get_cur_line_info(0);
	vpp_trace_encline("AFTER-RDMA", cur_line_info->enc_line_start, enc_line);

	trace_performance(cur_line_info, enc_line);

	rdma_enable_pre = is_vsync_rdma_enable();
	if (debug_flag & DEBUG_FLAG_PRINT_RDMA) {
		if (enable_rdma_log_count == 0)
			enable_rdma_log(0);
	}
#endif

	if (cur_dev->display_module != C3_DISPLAY_MODULE) {
		if (timer_count > 50) {
			timer_count = 0;
			video_notify_flag |= VIDEO_NOTIFY_FRAME_WAIT;
		}

		enc_line = get_cur_enc_line();
		if (enc_line > vsync_exit_line_max)
			vsync_exit_line_max = enc_line;
		if (video_suspend)
			video_suspend_cycle++;
#ifdef FIQ_VSYNC
		if (video_notify_flag)
			fiq_bridge_pulse_trigger(&vsync_fiq_bridge);
#else
		if (video_notify_flag)
			vsync_notify();

		/* if prop_change not zero, event will be delayed to next vsync */
		if (video_prop_status &&
		    !atomic_read(&video_prop_change)) {
			if (debug_flag & DEBUG_FLAG_TRACE_EVENT)
				pr_info("VD1 send event, changed status: 0x%x\n",
					video_prop_status);
			atomic_set(&video_prop_change, video_prop_status);
			video_prop_status = VIDEO_PROP_CHANGE_NONE;
			wake_up_interruptible(&amvideo_prop_change_wait);
		}
		if (video_info_change_status) {
			struct vd_info_s vd_info;

			if (debug_flag & DEBUG_FLAG_TRACE_EVENT)
				pr_info("VD1 send event to frc, changed status: 0x%x\n",
					video_info_change_status);
			vd_info.flags = video_info_change_status;
			vd_signal_notifier_call_chain(VIDEO_INFO_CHANGED,
						      &vd_info);
			video_info_change_status = VIDEO_INFO_CHANGE_NONE;
		}
#ifdef CONFIG_AMLOGIC_VPU
		vpu_work_process();
#endif
		vpp_crc_result = vpp_crc_check(vpp_crc_en, VPP0);
	}
#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
	vpp_trace_field_state("VSYNC-END",
		atomic_read(&cur_over_field_state),
		atomic_read(&cur_over_field_state),
		over_field ? 1 : 0,
		over_field_case1_cnt, over_field_case2_cnt);
#endif
	if (debug_flag & DEBUG_FLAG_GET_COUNT)
		pr_info("count=%d pip=%d, pip2=%d\n",
			get_count_pip[0], get_count_pip[1], get_count_pip[2]);

	/* clear the delay video flag of HDMI-IN AV SYNC Control */
	go_exit = 0;
}

static int do_vd1_path_select(void)
{
	int index = 0;
	int path_switched = 0;
	s32 vd1_path_id = glayer_info[index].display_path_id;

	/* vd1 path select */
	/* if pre_vsync_enable, vd1 need select to pre_vsync */
	if (cur_dev->pre_vsync_enable) {
		if (vd_layer[index].vd_path_id != vd1_path_id) {
			path_switched = 1;
			vd_layer[index].vd_path_id = vd1_path_id;
		}
		if (path_switched &&
		    vd_layer[index].next_pre_func ==
			vd_layer[index].cur_pre_func) {
			vd_layer[index].next_pre_func =
				(&vd_layer[index].pre_vd_func[0] ==
				vd_layer[index].next_pre_func) ?
				&vd_layer[index].pre_vd_func[1] :
				&vd_layer[index].pre_vd_func[0];
			if (debug_flag & DEBUG_FLAG_PRINT_PATH_SWITCH)
				pr_info("%s: vd1_path_id=%d, next_pre_func=%p, cur_pre_func=%p\n",
				__func__, vd1_path_id,
				vd_layer[index].next_pre_func,
				vd_layer[index].cur_pre_func);
		}
		if (!vd_layer[index].next_pre_func)
			vd_layer[index].next_pre_func =
				&vd_layer[index].pre_vd_func[0];

		vd_layer[index].next_pre_func->vd_toggle_frame =
			video_toggle_frame;
		vd_layer[index].next_pre_func->vd_swap_frame =
			do_vd1_swap_frame;
		vd_layer[index].next_pre_func->vd_render_frame =
			vdx_render_frame;
		vd_layer[index].next_pre_func->vd_early_process =
			video_early_proc;
		vd_layer[index].next_pre_func->vd_late_process =
			video_late_proc;
		vd_layer[index].next_pre_func->vd_misc_early_proc =
			vdx_misc_early_proc;
		vd_layer[index].next_pre_func->vd_misc_late_proc =
			vdx_misc_late_proc;

		if (gvideo_recv[0] &&
		    gvideo_recv[0]->path_id == vd1_path_id) {
			/* video_render.0 display on VD1 */
			vd_layer[index].next_pre_func->path_frame_index = 3;
			vd_layer[index].keep_frame_id = 0xff;
			vd_fake_func[3].video_process_flag = 1;
			vd_layer[index].func_path_id = RENDER0;
		} else if (gvideo_recv[1] &&
		    (gvideo_recv[1]->path_id == vd1_path_id)) {
			/* video_render.1 display on VD1 */
			vd_layer[index].next_pre_func->path_frame_index = 4;
			vd_layer[index].keep_frame_id = 0xff;
			vd_fake_func[4].video_process_flag = 1;
			vd_layer[index].func_path_id = RENDER1;
		} else if (gvideo_recv[2] &&
		    (gvideo_recv[2]->path_id == vd1_path_id)) {
			/* video_render.2 display on VD1 */
			vd_layer[index].next_pre_func->path_frame_index = 5;
			vd_layer[index].keep_frame_id = 0xff;
			vd_fake_func[5].video_process_flag = 1;
			vd_layer[index].func_path_id = RENDER2;
		} else if (vd1_path_id == VFM_PATH_PIP2) {
			/* pip2 display on VD1 */
			vd_layer[index].next_pre_func->path_frame_index = 2;
			vd_layer[index].keep_frame_id = 2;
			vd_fake_func[2].video_process_flag = 1;
			vd_layer[index].func_path_id = PIP2;
		} else if (vd1_path_id == VFM_PATH_PIP) {
			/* pip display on VD1 */
			vd_layer[index].next_pre_func->path_frame_index = 1;
			vd_layer[index].keep_frame_id = 1;
			vd_fake_func[1].video_process_flag = 1;
			vd_layer[index].func_path_id = PIP1;
		} else if ((vd1_path_id == VFM_PATH_AMVIDEO) ||
			  (vd1_path_id == VFM_PATH_DEF) ||
			  (vd1_path_id == VFM_PATH_AUTO)) {
			/* primary display on VD1 */
			vd_layer[index].next_pre_func->path_frame_index = 0;
			vd_layer[index].keep_frame_id = 0;
			vd_fake_func[0].video_process_flag = 1;
			vd_layer[index].func_path_id = AMVIDEO;
		} else {
			vd_layer[index].next_pre_func->vd_toggle_frame = NULL;
			vd_layer[index].next_pre_func->vd_swap_frame = NULL;
			vd_layer[index].next_pre_func->vd_render_frame = NULL;
			vd_layer[index].next_pre_func->vd_early_process = NULL;
			vd_layer[index].next_pre_func->vd_late_process = NULL;
			vd_layer[index].next_pre_func->vd_misc_early_proc =
				NULL;
			vd_layer[index].next_pre_func->vd_misc_late_proc = NULL;
		}
		/* set post vsync vd_func to NULL */
		vd_layer[index].vd_func.vd_toggle_frame = NULL;
		vd_layer[index].vd_func.vd_swap_frame = NULL;
		vd_layer[index].vd_func.vd_render_frame = NULL;
		vd_layer[index].vd_func.vd_early_process = NULL;
		vd_layer[index].vd_func.vd_late_process = NULL;
		vd_layer[index].vd_func.vd_misc_early_proc = NULL;
		vd_layer[index].vd_func.vd_misc_late_proc = NULL;

		vd_layer[0].vpp_index = PRE_VSYNC;
	} else {
		vd_layer[index].vd_func.vd_toggle_frame =
			video_toggle_frame;
		vd_layer[index].vd_func.vd_swap_frame =
			do_vd1_swap_frame;
		vd_layer[index].vd_func.vd_render_frame =
			vdx_render_frame;
		vd_layer[index].vd_func.vd_early_process =
			video_early_proc;
		vd_layer[index].vd_func.vd_late_process =
			video_late_proc;
		vd_layer[index].vd_func.vd_misc_early_proc =
			vdx_misc_early_proc;
		vd_layer[index].vd_func.vd_misc_late_proc =
			vdx_misc_late_proc;
		if (gvideo_recv[0] &&
		    gvideo_recv[0]->path_id == vd1_path_id) {
			/* video_render.0 display on VD1 */
			vd_layer[index].vd_func.path_frame_index = 3;
			vd_layer[index].keep_frame_id = 0xff;
			vd_fake_func[3].video_process_flag = 1;
			vd_layer[index].func_path_id = RENDER0;
		} else if (gvideo_recv[1] &&
		    (gvideo_recv[1]->path_id == vd1_path_id)) {
			/* video_render.1 display on VD1 */
			vd_layer[index].vd_func.path_frame_index = 4;
			vd_layer[index].keep_frame_id = 0xff;
			vd_fake_func[4].video_process_flag = 1;
			vd_layer[index].func_path_id = RENDER1;
		} else if (gvideo_recv[2] &&
		    (gvideo_recv[2]->path_id == vd1_path_id)) {
			/* video_render.2 display on VD1 */
			vd_layer[index].vd_func.path_frame_index = 5;
			vd_layer[index].keep_frame_id = 0xff;
			vd_fake_func[5].video_process_flag = 1;
			vd_layer[index].func_path_id = RENDER2;
		} else if (vd1_path_id == VFM_PATH_PIP2) {
			/* pip2 display on VD1 */
			vd_layer[index].vd_func.path_frame_index = 2;
			vd_layer[index].keep_frame_id = 2;
			vd_fake_func[2].video_process_flag = 1;
			vd_layer[index].func_path_id = PIP2;
		} else if (vd1_path_id == VFM_PATH_PIP) {
			/* pip display on VD1 */
			vd_layer[index].vd_func.path_frame_index = 1;
			vd_layer[index].keep_frame_id = 1;
			vd_fake_func[1].video_process_flag = 1;
			vd_layer[index].func_path_id = PIP1;
		} else if ((vd1_path_id == VFM_PATH_AMVIDEO) ||
			  (vd1_path_id == VFM_PATH_DEF) ||
			  (vd1_path_id == VFM_PATH_AUTO)) {
			/* primary display on VD1 */
			vd_layer[index].vd_func.path_frame_index = 0;
			vd_layer[index].keep_frame_id = 0;
			vd_fake_func[0].video_process_flag = 1;
			vd_layer[index].func_path_id = AMVIDEO;
		} else {
			vd_layer[index].vd_func.vd_toggle_frame = NULL;
			vd_layer[index].vd_func.vd_swap_frame = NULL;
			vd_layer[index].vd_func.vd_render_frame = NULL;
			vd_layer[index].vd_func.vd_early_process = NULL;
			vd_layer[index].vd_func.vd_late_process = NULL;
			vd_layer[index].vd_func.vd_misc_early_proc = NULL;
			vd_layer[index].vd_func.vd_misc_late_proc = NULL;
		}
		/* set pre vsync vd_func to NULL */
		vd_layer[index].pre_vd_func[0].vd_toggle_frame = NULL;
		vd_layer[index].pre_vd_func[0].vd_swap_frame = NULL;
		vd_layer[index].pre_vd_func[0].vd_render_frame = NULL;
		vd_layer[index].pre_vd_func[0].vd_early_process = NULL;
		vd_layer[index].pre_vd_func[0].vd_late_process = NULL;
		vd_layer[index].pre_vd_func[0].vd_misc_early_proc = NULL;
		vd_layer[index].pre_vd_func[0].vd_misc_late_proc = NULL;

		vd_layer[index].pre_vd_func[1].vd_toggle_frame = NULL;
		vd_layer[index].pre_vd_func[1].vd_swap_frame = NULL;
		vd_layer[index].pre_vd_func[1].vd_render_frame = NULL;
		vd_layer[index].pre_vd_func[1].vd_early_process = NULL;
		vd_layer[index].pre_vd_func[1].vd_late_process = NULL;
		vd_layer[index].pre_vd_func[1].vd_misc_early_proc = NULL;
		vd_layer[index].pre_vd_func[1].vd_misc_late_proc = NULL;

		vd_layer[0].vpp_index = VPP0;
		vd_layer[index].vd_path_id = -1;
	}
	return 0;
}

static int do_vd2_path_select(void)
{
	int index = 1;
	int path_used = 1;
	s32 vd1_path_id = glayer_info[0].display_path_id;
	s32 vd2_path_id = glayer_info[index].display_path_id;

	vd_layer[index].vd_func.vd_swap_frame = do_vdx_swap_frame;
	vd_layer[index].vd_func.vd_render_frame = vdx_render_frame;
	vd_layer[index].vd_func.vd_early_process = video_early_proc;
	vd_layer[index].vd_func.vd_late_process = video_late_proc;
	vd_layer[index].vd_func.vd_misc_early_proc =
		vdx_misc_early_proc;
	vd_layer[index].vd_func.vd_misc_late_proc = vdx_misc_late_proc;
	/* vd2 path select */
	if (gvideo_recv[0] &&
	    gvideo_recv[0]->path_id == vd2_path_id) {
		/* video_render.0 display on VD2 */
		if (vd2_path_id == vd1_path_id)
			vd_layer[index].vd_func.vd_toggle_frame = NULL;
		else
			vd_layer[index].vd_func.vd_toggle_frame =
				video_toggle_frame;
		vd_layer[index].vd_func.path_frame_index = 3;
		vd_layer[index].keep_frame_id = 0xff;
		vd_fake_func[3].video_process_flag = 1;
		vd_layer[index].func_path_id = RENDER0;
	} else if (gvideo_recv[1] &&
	    (gvideo_recv[1]->path_id == vd2_path_id)) {
		/* video_render.1 display on VD2 */
		if (vd2_path_id == vd1_path_id)
			vd_layer[index].vd_func.vd_toggle_frame = NULL;
		else
			vd_layer[index].vd_func.vd_toggle_frame =
				video_toggle_frame;
		vd_layer[index].vd_func.path_frame_index = 4;
		vd_layer[index].keep_frame_id = 0xff;
		vd_fake_func[4].video_process_flag = 1;
		vd_layer[index].func_path_id = RENDER1;
	} else if (gvideo_recv[2] &&
	    (gvideo_recv[2]->path_id == vd2_path_id)) {
		/* video_render.2 display on VD2 */
		if (vd2_path_id == vd1_path_id)
			vd_layer[index].vd_func.vd_toggle_frame = NULL;
		else
			vd_layer[index].vd_func.vd_toggle_frame =
				video_toggle_frame;
		vd_layer[index].vd_func.path_frame_index = 5;
		vd_layer[index].keep_frame_id = 0xff;
		vd_fake_func[5].video_process_flag = 1;
		vd_layer[index].func_path_id = RENDER2;
	} else if (vd2_path_id == VFM_PATH_PIP2) {
		/* pip2 display on VD2 */
		if (vd2_path_id == vd1_path_id)
			vd_layer[index].vd_func.vd_toggle_frame = NULL;
		else
			vd_layer[index].vd_func.vd_toggle_frame =
				video_toggle_frame;
		vd_layer[index].vd_func.path_frame_index = 2;
		vd_layer[index].keep_frame_id = 2;
		vd_fake_func[2].video_process_flag = 1;
		vd_layer[index].func_path_id = PIP2;
	} else if ((vd2_path_id == VFM_PATH_PIP) ||
		  (vd2_path_id == VFM_PATH_DEF)) {
		/* pip display on VD2 */
		if (vd2_path_id == VFM_PATH_PIP && vd2_path_id == vd1_path_id)
			vd_layer[index].vd_func.vd_toggle_frame = NULL;
		else
			vd_layer[index].vd_func.vd_toggle_frame =
				video_toggle_frame;
		vd_layer[index].vd_func.path_frame_index = 1;
		vd_layer[index].keep_frame_id = 1;
		vd_fake_func[1].video_process_flag = 1;
		vd_layer[index].func_path_id = PIP1;
	} else if (vd2_path_id == VFM_PATH_AMVIDEO) {
		/* primary display on VD2 */
		if (vd2_path_id == vd1_path_id)
			vd_layer[index].vd_func.vd_toggle_frame = NULL;
		else
			vd_layer[index].vd_func.vd_toggle_frame =
				video_toggle_frame;
		vd_layer[index].vd_func.path_frame_index = 0;
		vd_layer[index].keep_frame_id = 0;
		vd_fake_func[0].video_process_flag = 1;
		vd_layer[index].func_path_id = AMVIDEO;
	} else {
		vd_layer[index].vd_func.vd_toggle_frame = NULL;
		vd_layer[index].vd_func.vd_swap_frame = NULL;
		vd_layer[index].vd_func.vd_render_frame = NULL;
		vd_layer[index].vd_func.vd_early_process = NULL;
		vd_layer[index].vd_func.vd_late_process = NULL;
		vd_layer[index].vd_func.vd_misc_early_proc = NULL;
		vd_layer[index].vd_func.vd_misc_late_proc = NULL;
		path_used = 0;
	}
	return path_used;
}

#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
static int do_vd3_path_select(void)
{
	int index = 2;
	int path_used = 1;
	s32 vd1_path_id = glayer_info[0].display_path_id;
	s32 vd2_path_id = glayer_info[1].display_path_id;
	s32 vd3_path_id = glayer_info[index].display_path_id;

	vd_layer[index].vd_func.vd_swap_frame = do_vdx_swap_frame;
	vd_layer[index].vd_func.vd_render_frame = vdx_render_frame;
	vd_layer[index].vd_func.vd_early_process = video_early_proc;
	vd_layer[index].vd_func.vd_late_process = video_late_proc;
	vd_layer[index].vd_func.vd_misc_early_proc =
		vdx_misc_early_proc;
	vd_layer[index].vd_func.vd_misc_late_proc = vdx_misc_late_proc;
	/* vd3 path select */
	if (gvideo_recv[0] &&
	    gvideo_recv[0]->path_id == vd3_path_id) {
		/* video_render.0 display on VD3 */
		if (vd3_path_id == vd1_path_id ||
		   vd3_path_id == vd2_path_id)
			vd_layer[index].vd_func.vd_toggle_frame = NULL;
		else
			vd_layer[index].vd_func.vd_toggle_frame =
				video_toggle_frame;
		vd_layer[index].vd_func.path_frame_index = 3;
		vd_layer[index].keep_frame_id = 0xff;
		vd_fake_func[3].video_process_flag = 1;
		vd_layer[index].func_path_id = RENDER0;
	} else if (gvideo_recv[1] &&
	    (gvideo_recv[1]->path_id == vd3_path_id)) {
		/* video_render.1 display on VD3 */
		if (vd3_path_id == vd1_path_id ||
		   vd3_path_id == vd2_path_id)
			vd_layer[index].vd_func.vd_toggle_frame = NULL;
		else
			vd_layer[index].vd_func.vd_toggle_frame =
				video_toggle_frame;
		vd_layer[index].vd_func.path_frame_index = 4;
		vd_layer[index].keep_frame_id = 0xff;
		vd_fake_func[4].video_process_flag = 1;
		vd_layer[index].func_path_id = RENDER1;
	} else if (gvideo_recv[2] &&
	    (gvideo_recv[2]->path_id == vd3_path_id)) {
		/* video_render.2 display on VD3 */
		if (vd3_path_id == vd1_path_id ||
		   vd3_path_id == vd2_path_id)
			vd_layer[index].vd_func.vd_toggle_frame = NULL;
		else
			vd_layer[index].vd_func.vd_toggle_frame =
				video_toggle_frame;
		vd_layer[index].vd_func.path_frame_index = 5;
		vd_layer[index].keep_frame_id = 0xff;
		vd_fake_func[5].video_process_flag = 1;
		vd_layer[index].func_path_id = RENDER2;
	} else if (vd3_path_id == VFM_PATH_PIP2) {
		/* pip2 display on VD3 */
		if (vd3_path_id == vd1_path_id ||
		   vd3_path_id == vd2_path_id)
			vd_layer[index].vd_func.vd_toggle_frame = NULL;
		else
			vd_layer[index].vd_func.vd_toggle_frame =
				video_toggle_frame;
		vd_layer[index].vd_func.path_frame_index = 2;
		vd_layer[index].keep_frame_id = 2;
		vd_fake_func[2].video_process_flag = 1;
		vd_layer[index].func_path_id = PIP2;
	} else if (vd3_path_id == VFM_PATH_PIP) {
		/* pip display on VD3 */
		if (vd3_path_id == vd1_path_id ||
		   vd3_path_id == vd2_path_id)
			vd_layer[index].vd_func.vd_toggle_frame = NULL;
		else
			vd_layer[index].vd_func.vd_toggle_frame =
				video_toggle_frame;
		vd_layer[index].vd_func.path_frame_index = 1;
		vd_layer[index].keep_frame_id = 1;
		vd_fake_func[1].video_process_flag = 1;
		vd_layer[index].func_path_id = PIP1;
	} else if (vd3_path_id == VFM_PATH_AMVIDEO) {
		/* primary display on VD3 */
		if (vd3_path_id == vd1_path_id ||
		   vd3_path_id == vd2_path_id)
			vd_layer[index].vd_func.vd_toggle_frame = NULL;
		else
			vd_layer[index].vd_func.vd_toggle_frame =
			video_toggle_frame;
		vd_layer[index].vd_func.path_frame_index = 0;
		vd_layer[index].keep_frame_id = 0;
		vd_fake_func[0].video_process_flag = 1;
		vd_layer[index].func_path_id = AMVIDEO;
	} else {
		vd_layer[index].vd_func.vd_toggle_frame = NULL;
		vd_layer[index].vd_func.vd_swap_frame = NULL;
		vd_layer[index].vd_func.vd_render_frame = NULL;
		vd_layer[index].vd_func.vd_early_process = NULL;
		vd_layer[index].vd_func.vd_late_process = NULL;
		vd_layer[index].vd_func.vd_misc_early_proc = NULL;
		vd_layer[index].vd_func.vd_misc_late_proc = NULL;
		path_used = 0;
	}
	return path_used;
}
#endif

static void do_video_path_select(void)
{
	int i;

	do_vd1_path_select();
	do_vd2_path_select();
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
	if (cur_dev->max_vd_layers == 3)
		do_vd3_path_select();
#endif
	/* vdx init */
	for (i = 0; i < cur_dev->max_vd_layers; i++)
		vd_dispbuf_init(i);
}

static void do_fake_video_select(void)
{
	/* for amvideo */
	vd_fake_func[0].vd_toggle_frame = video_toggle_frame;
	vd_fake_func[0].vd_swap_frame = NULL;
	vd_fake_func[0].vd_render_frame = NULL;
	vd_fake_func[0].vd_early_process = video_early_proc;
	vd_fake_func[0].vd_late_process = video_late_proc;
	vd_fake_func[0].vd_misc_early_proc = NULL;
	vd_fake_func[0].vd_misc_late_proc = NULL;
	vd_fake_func[0].fake_func_id = AMVIDEO;

	/* for pip */
	vd_fake_func[1].vd_toggle_frame = video_toggle_frame;
	vd_fake_func[1].vd_swap_frame = NULL;
	vd_fake_func[1].vd_render_frame = NULL;
	vd_fake_func[1].vd_early_process = video_early_proc;
	vd_fake_func[1].vd_late_process = video_late_proc;
	vd_fake_func[1].vd_misc_early_proc = NULL;
	vd_fake_func[1].vd_misc_late_proc = NULL;
	vd_fake_func[1].fake_func_id = PIP1;

	/* for pip2 */
	vd_fake_func[2].vd_toggle_frame = video_toggle_frame;
	vd_fake_func[2].vd_swap_frame = NULL;
	vd_fake_func[2].vd_render_frame = NULL;
	vd_fake_func[2].vd_early_process = video_early_proc;
	vd_fake_func[2].vd_late_process = video_late_proc;
	vd_fake_func[2].vd_misc_early_proc = NULL;
	vd_fake_func[2].vd_misc_late_proc = NULL;
	vd_fake_func[2].fake_func_id = PIP2;

	/* for render0 */
	vd_fake_func[3].vd_toggle_frame = video_toggle_frame;
	vd_fake_func[3].vd_swap_frame = NULL;
	vd_fake_func[3].vd_render_frame = NULL;
	vd_fake_func[3].vd_early_process = video_early_proc;
	vd_fake_func[3].vd_late_process = video_late_proc;
	vd_fake_func[3].vd_misc_early_proc = NULL;
	vd_fake_func[3].vd_misc_late_proc = NULL;
	vd_fake_func[3].fake_func_id = RENDER0;

	/* for render1 */
	vd_fake_func[4].vd_toggle_frame = video_toggle_frame;
	vd_fake_func[4].vd_swap_frame = NULL;
	vd_fake_func[4].vd_render_frame = NULL;
	vd_fake_func[4].vd_early_process = video_early_proc;
	vd_fake_func[4].vd_late_process = video_late_proc;
	vd_fake_func[4].vd_misc_early_proc = NULL;
	vd_fake_func[4].vd_misc_late_proc = NULL;
	vd_fake_func[4].fake_func_id = RENDER1;

	/* for render2 */
	vd_fake_func[5].vd_toggle_frame = video_toggle_frame;
	vd_fake_func[5].vd_swap_frame = NULL;
	vd_fake_func[5].vd_render_frame = NULL;
	vd_fake_func[5].vd_early_process = video_early_proc;
	vd_fake_func[5].vd_late_process = video_late_proc;
	vd_fake_func[5].vd_misc_early_proc = NULL;
	vd_fake_func[5].vd_misc_late_proc = NULL;
	vd_fake_func[5].fake_func_id = RENDER2;
}

#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
void pre_vsync_process(void)
{
	int ret = 0, i;
	bool rdma_enable = false, _rdma_enable_pre = false;
	u32 path_frame_index;
	struct vframe_s *path_new_frame[6] = {NULL};
	static s32 cur_vd1_path_id = VFM_PATH_INVALID;
	bool path_switch = false;
	struct vd_func_s *cur_pre_func = NULL;
	s32 vd_path_id[MAX_VD_LAYER] = {0};
	struct path_id_s path_id;
	struct cur_line_info_t *cur_line_info = NULL;
	int enc_line;

	if (cur_dev->vsync_2to1_enable && frc_n2m_worked()) {
#ifdef CONFIG_AMLOGIC_VIDEO_COMPOSER
		vsync_notify_video_composer(0,
			vsync_pts_inc_scale,
			vsync_pts_inc_scale_base / 2);
#endif
#ifdef CONFIG_AMLOGIC_VIDEOQUEUE
		vsync_notify_videoqueue(0,
			vsync_pts_inc_scale,
			vsync_pts_inc_scale_base / 2);
#endif
	}

	set_cur_line_info(1);
	enc_line = get_cur_enc_line();
	cur_line_info = get_cur_line_info(1);
	vpp_trace_encline("ENTER-PRE-VSYNC", cur_line_info->enc_line_start, enc_line);

	for (i = 0; i < MAX_VD_LAYER; i++)
		vd_path_id[i] = glayer_info[i].display_path_id;
	path_id.vd1_path_id = vd_path_id[0];
	path_id.vd2_path_id = vd_path_id[1];
	path_id.vd3_path_id = vd_path_id[2];

	vd_path_id[1] = VFM_PATH_INVALID;
	vd_path_id[2] = VFM_PATH_INVALID;
	vd_layer[0].cur_pre_func = vd_layer[0].next_pre_func;
	cur_pre_func = vd_layer[0].cur_pre_func;
	if (!cur_pre_func)
		goto pre_exit_1;

#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
	pre_vsync_rdma_config_pre();
	rdma_enable = is_pre_vsync_rdma_enable();
	_rdma_enable_pre = pre_vsync_rdma_enable_pre;
#endif
	/* misc early process */
	if (cur_pre_func->vd_misc_early_proc) {
		ret = cur_pre_func->vd_misc_early_proc
			(0, rdma_enable, _rdma_enable_pre);
		if (ret < 0)
			goto pre_exit_2;
	}
	/* early process */
	if (cur_pre_func->vd_early_process) {
		ret = cur_pre_func->vd_early_process(0, 0);
		if (ret < 0)
			goto pre_exit_2;
	}

	if (cur_pre_func->vd_toggle_frame) {
		path_frame_index = cur_pre_func->path_frame_index;
		path_new_frame[path_frame_index] =
			cur_pre_func->vd_toggle_frame(0, 0, vd_path_id, &path_id);
		pre_vsync_count++;
	}
	if (!vd_layer[0].global_output) {
		cur_vd1_path_id = VFM_PATH_INVALID;
		vd_path_id[0] = VFM_PATH_INVALID;
	}

	if (cur_vd1_path_id != vd_path_id[0])
		path_switch = true;
	/* do vd swap */
	if (cur_pre_func->vd_swap_frame)
		cur_pre_func->vd_swap_frame(0,
						vd_path_id[0],
						cur_vd1_path_id,
						&path_new_frame[0]);

	/* filter setting management */
	if (cur_pre_func->vd_render_frame)
		cur_pre_func->vd_render_frame(&vd_layer[0], vinfo);
	/* do blend set */
	if (cur_dev->pre_vsync_enable)
		vpp_blend_update(vinfo, PRE_VSYNC);
pre_exit_2:
	if (cur_pre_func->vd_late_process)
		cur_pre_func->vd_late_process(0, 0);

	if (cur_pre_func->vd_misc_late_proc)
		cur_pre_func->vd_misc_late_proc(0);

#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
	pre_vsync_rdma_config();
	pre_vsync_rdma_enable_pre = is_pre_vsync_rdma_enable();
#endif
	cur_vd1_path_id = vd_path_id[0];
pre_exit_1:
	//trace_for_pre_vsync();
	enc_line = get_cur_enc_line();
	vpp_trace_encline("AFTER-PRE-VSYNC-RDMA", cur_line_info->enc_line_start, enc_line);
}
#endif

static void dump_current_display_regs_info(void)
{
	int i, layer, afbc, dw;
	struct vpp_frame_par_s *cur_frame_par;
	struct hw_vd_linear_reg_s *mif_linear_reg;
	struct hw_vd_reg_s *mif_reg = NULL;
	struct hw_afbc_reg_s *afbc_reg = NULL;
	u32 vd_if_baddr_y, vd_if_baddr_cb, vd_if_baddr_cr;
	u32 canvas_id;
	struct canvas_s cs0, cs1, cs2;
	u64 baddr_y = 0, baddr_cb = 0, baddr_cr = 0;
	u64 head_addr = 0, body_addr = 0;
	struct vframe_s *dispbuf = NULL;
#ifdef CONFIG_AMLOGIC_MEDIA_DEINTERLACE
	struct afbcd_info di_vfm;
#endif

	memset(&cs0, 0, sizeof(struct canvas_s));
	memset(&cs1, 0, sizeof(struct canvas_s));
	memset(&cs2, 0, sizeof(struct canvas_s));
	for (i = 0; i < cur_dev->max_vd_layers; i++) {
		dispbuf = get_dispbuf(i);
		if (!dispbuf)
			continue;
#ifdef CONFIG_AMLOGIC_MEDIA_DEINTERLACE
		if (IS_DI_PRELINK(dispbuf->di_flag)) {
			if (di_vfm_info(&di_vfm))
				pr_info("VID(%s): VD%d prelink afbc:0x%llx-0x%llx hsize:%d vsize:%d\n",
					__func__, i + 1, (u64)di_vfm.head_addr,
					(u64)di_vfm.body_addr,
					di_vfm.hsize_out, di_vfm.vsize_out);
			continue;
		}
#endif
		if (cur_dev->display_module == S5_DISPLAY_MODULE) {
			if (i != 0)
				layer = i + SLICE_NUM - 1;
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
			mif_reg = (struct hw_vd_reg_s *)&vd_proc_reg.vd_mif_reg[layer];
			mif_linear_reg = (struct hw_vd_linear_reg_s *)
				&vd_proc_reg.vd_mif_linear_reg[layer];
			afbc_reg = (struct hw_afbc_reg_s *)&vd_proc_reg.vd_afbc_reg[layer];
#endif
		} else {
			mif_reg = &vd_layer[i].vd_mif_reg;
			mif_linear_reg = &vd_layer[i].vd_mif_linear_reg;
			afbc_reg = &vd_layer[i].vd_afbc_reg;
			if (i == 0 && vd_layer[i].vd1_vd2_mux) {
				/* vd2 replaced vd1 mif reg */
				mif_reg = &vd_layer[1].vd_mif_reg;
			}
		}

		cur_frame_par = vd_layer[i].cur_frame_par;
		if (!mif_reg || !afbc_reg || !cur_frame_par)
			return;
		afbc = dispbuf->type & VIDTYPE_COMPRESS ? 1 : 0;
		dw = afbc && cur_frame_par->nocomp;

		canvas_id = READ_VCBUS_REG(mif_reg->vd_if0_canvas0);
		if (afbc && !dw) {
			head_addr = (u64)READ_VCBUS_REG(afbc_reg->afbc_head_baddr) << 4;
			body_addr = (u64)READ_VCBUS_REG(afbc_reg->afbc_body_baddr) << 4;
		} else {
			if (cur_dev->mif_linear) {
				vd_if_baddr_y = mif_linear_reg->vd_if0_baddr_y;
				vd_if_baddr_cb = mif_linear_reg->vd_if0_baddr_cb;
				vd_if_baddr_cr = mif_linear_reg->vd_if0_baddr_cr;
				baddr_y = (u64)READ_VCBUS_REG(vd_if_baddr_y) << 4;
				baddr_cb = (u64)READ_VCBUS_REG(vd_if_baddr_cb) << 4;
				baddr_cr = (u64)READ_VCBUS_REG(vd_if_baddr_cr) << 4;
			} else {
				canvas_read(canvas_id & 0xff, &cs0);
				canvas_read((canvas_id >> 8) & 0xff, &cs1);
				canvas_read((canvas_id >> 16) & 0xff, &cs2);
				baddr_y = (u64)cs0.addr;
				baddr_cb = (u64)cs1.addr;
				baddr_cr = (u64)cs2.addr;
			}
		}
		pr_info("VID(%s): VD%d afbc:%d dw:%d, canvas:%x canvas adr:0x%llx-0x%llx-0x%llx, afbc:0x%llx-0x%llx\n",
			__func__, i + 1, afbc, dw, canvas_id,
			baddr_y, baddr_cb, baddr_cr,
			head_addr, body_addr);
	}
}

void post_vsync_process(void)
{
	unsigned char frame_par_di_set = 0;
	struct vframe_s *path_new_frame[6] = {NULL};
	struct vframe_s *di_post_vf = NULL;
	bool di_post_process_done = false;
	static s32 cur_vd_path_id[MAX_VD_LAYER] = {VFM_PATH_INVALID};
	s32 vd_path_id[MAX_VD_LAYER] = {0};
	s32 vd_path_id_temp[MAX_VD_LAYER] = {0};
	int i, ret = 0;
	bool path_switch = false;
	u32 path_frame_index;
	struct path_id_s path_id;
	struct cur_line_info_t *cur_line_info = NULL;
	int enc_line;

#if IS_ENABLED(CONFIG_AMLOGIC_DEBUG_IOTRACE)
	iotrace_misc_record_write(RECORD_TYPE_VSYNC_IN, 0, 0, 0);
#endif

	set_cur_line_info(0);
	enc_line = get_cur_enc_line();
	cur_line_info = get_cur_line_info(0);
	vpp_trace_encline("ENTER-VSYNC", cur_line_info->enc_line_start, enc_line);

	if (cur_dev->display_module != S5_DISPLAY_MODULE)
		blend_reg_conflict_detect();
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
	else
		check_afbc_status();
#endif
	if (vd_layer[0].force_disable)
		atomic_set(&vt_disable_video_done, 1);

#ifdef CONFIG_AMLOGIC_MEDIA_MSYNC
	msync_vsync_update();
#endif
	for (i = 0; i < MAX_VD_LAYER; i++) {
		vd_path_id[i] = glayer_info[i].display_path_id;
		if (cur_vd_path_id[i] == 0xff)
			cur_vd_path_id[i] = vd_path_id[i];
	}
	path_id.vd1_path_id = vd_path_id[0];
	path_id.vd2_path_id = vd_path_id[1];
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
	path_id.vd3_path_id = vd_path_id[2];
#endif

#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
	/* Just a workaround to enable RDMA without any register config.
	 * because rdma is enabled after first rdma config.
	 * Previously, it will write register directly and
	 * maybe out of blanking in first irq.
	 */
	if (first_irq)
		goto LATE_PROC;
#endif

	for (i = 0; i < MAX_VIDEO_FAKE; i++)
		vd_fake_func[i].video_process_flag = 0;

	do_video_path_select();
	do_fake_video_select();
	/* misc early process */
	ret = misc_early_proc();
	if (ret < 0)
		goto exit;
	/* early process */
	for (i = 0; i < cur_dev->max_vd_layers; i++)
		if (vd_layer[i].vd_func.vd_early_process) {
			ret = vd_layer[i].vd_func.vd_early_process(i, 0);
			if (ret < 0)
				goto exit;
		}
	for (i = 0; i < MAX_VIDEO_FAKE; i++) {
		if (!vd_fake_func[i].video_process_flag)
			if (vd_fake_func[i].vd_early_process)
				vd_fake_func[i].vd_early_process(0xff, i);
	}
	/* do toggle frame */
	for (i = 0; i < cur_dev->max_vd_layers; i++) {
		if (vd_layer[i].vd_func.vd_toggle_frame) {
			path_frame_index = vd_layer[i].vd_func.path_frame_index;
			path_new_frame[path_frame_index] =
				vd_layer[i].vd_func.vd_toggle_frame(i, 0, vd_path_id, &path_id);
			if (go_exit)
				goto exit;
			if (!vd_layer[i].global_output) {
				switch (i) {
				#ifdef CHECK_LATER
				case 0:
				/* CHECK_LATER FIXME: if need enable for vd1 */
				cur_vd_path_id[0] = VFM_PATH_INVALID;
				vd_path_id[0] = VFM_PATH_INVALID;
				break;
				#endif
				case 1:
				case 2:
				cur_vd_path_id[i] = VFM_PATH_INVALID;
				vd_path_id[i] = VFM_PATH_INVALID;
				break;
				}
			}
		}
	}

	if (debug_flag & DEBUG_FLAG_PRINT_DISBUF_PER_VSYNC)
		dump_current_display_regs_info();

#ifdef CONFIG_AMLOGIC_ZAPPER_CUT
	if (cur_vd_path_id[0] != vd_path_id[0] ||
	   cur_vd_path_id[1] != vd_path_id[1])
		path_switch = true;

	if (path_switch &&
	    (debug_flag & DEBUG_FLAG_PRINT_PATH_SWITCH)) {
		pr_info("VID: === before path switch ===\n");
		pr_info("VID: \tcur_path_id: %d, %d;\nVID: \tnew_path_id: %d, %d;\nVID: \ttoggle:%p, %p, %p %p, %p, %p\nVID: \tcur:%p, %p, %p, %p;\n",
			cur_vd_path_id[0], cur_vd_path_id[1],
			vd_path_id[0], vd_path_id[1],
			path_new_frame[0], path_new_frame[1],
			path_new_frame[2], path_new_frame[3],
			path_new_frame[4], path_new_frame[5],
			cur_dispbuf[0], cur_dispbuf[1],
			gvideo_recv[0] ? gvideo_recv[0]->cur_buf : NULL,
			gvideo_recv[1] ? gvideo_recv[1]->cur_buf : NULL);
		pr_info("VID: \tdispbuf:%p, %p; \tvf_ext:%p, %p;\nVID: \tlocal:%p, %p, %p, %p\n",
			vd_layer[0].dispbuf, vd_layer[1].dispbuf,
			vd_layer[0].vf_ext, vd_layer[1].vf_ext,
			&vf_local[0], &vf_local[1],
			gvideo_recv[0] ? &gvideo_recv[0]->local_buf : NULL,
			gvideo_recv[1] ? &gvideo_recv[1]->local_buf : NULL);
		pr_info("VID: \tblackout:%d %d; force:%d;\n",
			blackout[0], blackout[1], force_blackout);
	}

	if (debug_flag & DEBUG_FLAG_PRINT_DISBUF_PER_VSYNC) {
		u32 omx_index[6] = {0}, i = 0;
		u64 timestamp[6] = {0};
		struct vframe_s *new_frame = NULL;

		pr_info("VID(%s): path id: %d, %d; new_frame:%p, %p, %p, %p, %p, %p cur:%p, %p, %p, %p; vd dispbuf:%p, %p; vf_ext:%p, %p; local:%p, %p, %p, %p\n",
			__func__,
			vd_path_id[0], vd_path_id[1],
			path_new_frame[0], path_new_frame[1],
			path_new_frame[2], path_new_frame[3],
			path_new_frame[4], path_new_frame[5],
			cur_dispbuf[0], cur_dispbuf[1],
			gvideo_recv[0] ? gvideo_recv[0]->cur_buf : NULL,
			gvideo_recv[1] ? gvideo_recv[1]->cur_buf : NULL,
			vd_layer[0].dispbuf, vd_layer[1].dispbuf,
			vd_layer[0].vf_ext, vd_layer[1].vf_ext,
			&vf_local, &vf_local[1],
			gvideo_recv[0] ? &gvideo_recv[0]->local_buf : NULL,
			gvideo_recv[1] ? &gvideo_recv[1]->local_buf : NULL);
		for (i = 0; i < 6; i++) {
			new_frame = path_new_frame[i];
			if (new_frame) {
				omx_index[i] = new_frame->omx_index;
				timestamp[i] = div_u64(new_frame->timestamp,
						       1000000000);
			}
		}
		pr_info("VID(%s): new_frame omx_index:%d, %d, %d, %d, %d, %d; new_frame timestamp:%lld %lld %lld %lld %lld %lld\n",
			 __func__,
			 omx_index[0], omx_index[1], omx_index[2], omx_index[3],
			 omx_index[4], omx_index[5], timestamp[0], timestamp[1],
			 timestamp[2], timestamp[3], timestamp[4], timestamp[5]
			);
	}
	vd_path_id_temp[0] = glayer_info[0].display_path_id;
	vd_path_id_temp[1] = glayer_info[1].display_path_id;
	for (i = 0; i < MAX_VIDEO_FAKE; i++) {
		if (!vd_fake_func[i].video_process_flag)
			if (vd_fake_func[i].vd_toggle_frame)
				vd_fake_func[i].vd_toggle_frame
					(0xff, i,
					vd_path_id_temp, &path_id);
	}

	/* do vd swap */
	for (i = 0; i < cur_dev->max_vd_layers; i++)
		if (vd_layer[i].vd_func.vd_swap_frame)
			vd_layer[i].vd_func.vd_swap_frame(i, vd_path_id[i],
							cur_vd_path_id[i],
							&path_new_frame[0]);
	if (debug_flag & DEBUG_FLAG_PRINT_DISBUF_PER_VSYNC)
		pr_info("VID(%s): layer enable status: VD1:e:%d,e_save:%d,g:%d,d:%d,f:%s; VD2:e:%d,e_save:%d,g:%d,d:%d,f:%s;",
			__func__,
			vd_layer[0].enabled, vd_layer[0].enabled_status_saved,
			vd_layer[0].global_output, vd_layer[0].disable_video,
			vd_layer[0].force_disable ? "true" : "false",
			vd_layer[1].enabled, vd_layer[1].enabled_status_saved,
			vd_layer[1].global_output, vd_layer[1].disable_video,
			vd_layer[1].force_disable ? "true" : "false");

#else
	if (cur_vd_path_id[0] != vd_path_id[0] ||
	   cur_vd_path_id[1] != vd_path_id[1] ||
	   cur_vd_path_id[2] != vd_path_id[2])
		path_switch = true;

	if (path_switch &&
	    (debug_flag & DEBUG_FLAG_PRINT_PATH_SWITCH)) {
		pr_info("VID: === before path switch ===\n");
		pr_info("VID: \tcur_path_id: %d, %d, %d;\nVID: \tnew_path_id: %d, %d, %d;\nVID: \ttoggle:%p, %p, %p %p, %p, %p\nVID: \tcur:%p, %p, %p, %p, %p, %p;\n",
			cur_vd_path_id[0], cur_vd_path_id[1], cur_vd_path_id[2],
			vd_path_id[0], vd_path_id[1], vd_path_id[2],
			path_new_frame[0], path_new_frame[1],
			path_new_frame[2], path_new_frame[3],
			path_new_frame[4], path_new_frame[5],
			cur_dispbuf[0], cur_dispbuf[1], cur_dispbuf[2],
			gvideo_recv[0] ? gvideo_recv[0]->cur_buf : NULL,
			gvideo_recv[1] ? gvideo_recv[1]->cur_buf : NULL,
			gvideo_recv[2] ? gvideo_recv[2]->cur_buf : NULL);

		pr_info("VID: \tdispbuf:%p, %p, %p; \tvf_ext:%p, %p, %p;\nVID: \tlocal:%p, %p, %p, %p, %p, %p\n",
			vd_layer[0].dispbuf, vd_layer[1].dispbuf, vd_layer[2].dispbuf,
			vd_layer[0].vf_ext, vd_layer[1].vf_ext, vd_layer[2].vf_ext,
			&vf_local[0], &vf_local[1], &vf_local[2],
			gvideo_recv[0] ? &gvideo_recv[0]->local_buf : NULL,
			gvideo_recv[1] ? &gvideo_recv[1]->local_buf : NULL,
			gvideo_recv[2] ? &gvideo_recv[2]->local_buf : NULL);
		pr_info("VID: \tblackout:%d %d, %d force:%d;\n",
			blackout[0], blackout[1], blackout[2], force_blackout);
	}

	if (debug_flag & DEBUG_FLAG_PRINT_DISBUF_PER_VSYNC) {
		u32 omx_index[6] = {0}, i = 0;
		u64 timestamp[6] = {0};
		struct vframe_s *new_frame = NULL;

		pr_info("VID(%s): path id: %d, %d, %d; new_frame:%p, %p, %p, %p, %p, %p cur:%p, %p, %p, %p, %p, %p; vd dispbuf:%p, %p, %p; vf_ext:%p, %p, %p; local:%p, %p, %p, %p, %p, %p\n",
			__func__,
			vd_path_id[0], vd_path_id[1], vd_path_id[2],
			path_new_frame[0], path_new_frame[1],
			path_new_frame[2], path_new_frame[3],
			path_new_frame[4], path_new_frame[5],
			cur_dispbuf[0], cur_dispbuf[1], cur_dispbuf[2],
			gvideo_recv[0] ? gvideo_recv[0]->cur_buf : NULL,
			gvideo_recv[1] ? gvideo_recv[1]->cur_buf : NULL,
			gvideo_recv[2] ? gvideo_recv[2]->cur_buf : NULL,
			vd_layer[0].dispbuf, vd_layer[1].dispbuf, vd_layer[2].dispbuf,
			vd_layer[0].vf_ext, vd_layer[1].vf_ext, vd_layer[2].vf_ext,
			&vf_local, &vf_local[1], &vf_local[2],
			gvideo_recv[0] ? &gvideo_recv[0]->local_buf : NULL,
			gvideo_recv[1] ? &gvideo_recv[1]->local_buf : NULL,
			gvideo_recv[2] ? &gvideo_recv[2]->local_buf : NULL);
		for (i = 0; i < 6; i++) {
			new_frame = path_new_frame[i];
			if (new_frame) {
				omx_index[i] = new_frame->omx_index;
				timestamp[i] = div_u64(new_frame->timestamp,
						       1000000000);
			}
		}
		pr_info("VID(%s): new_frame omx_index:%d, %d, %d, %d, %d, %d; new_frame timestamp:%lld %lld %lld %lld %lld %lld\n",
			 __func__,
			 omx_index[0], omx_index[1], omx_index[2], omx_index[3],
			 omx_index[4], omx_index[5], timestamp[0], timestamp[1],
			 timestamp[2], timestamp[3], timestamp[4], timestamp[5]
		);
	}
	vd_path_id_temp[0] = glayer_info[0].display_path_id;
	vd_path_id_temp[1] = glayer_info[1].display_path_id;
	vd_path_id_temp[2] = glayer_info[2].display_path_id;

	for (i = 0; i < MAX_VIDEO_FAKE; i++) {
		if (!vd_fake_func[i].video_process_flag)
			if (vd_fake_func[i].vd_toggle_frame)
				vd_fake_func[i].vd_toggle_frame
					(0xff, i,
					vd_path_id_temp, &path_id);
	}

	/* do vd swap */
	for (i = 0; i < cur_dev->max_vd_layers; i++)
		if (vd_layer[i].vd_func.vd_swap_frame)
			vd_layer[i].vd_func.vd_swap_frame(i, vd_path_id[i],
							cur_vd_path_id[i],
							&path_new_frame[0]);
	if (debug_flag & DEBUG_FLAG_PRINT_DISBUF_PER_VSYNC)
		pr_info("(%s)VID: layer enable status: VD1:e:%d,e_save:%d,g:%d,d:%d,f:%s; VD2:e:%d,e_save:%d,g:%d,d:%d,f:%s; VD3:e:%d,e_save:%d,g:%d,d:%d,f:%s",
			__func__,
			vd_layer[0].enabled, vd_layer[0].enabled_status_saved,
			vd_layer[0].global_output, vd_layer[0].disable_video,
			vd_layer[0].force_disable ? "true" : "false",
			vd_layer[1].enabled, vd_layer[1].enabled_status_saved,
			vd_layer[1].global_output, vd_layer[1].disable_video,
			vd_layer[1].force_disable ? "true" : "false",
			vd_layer[2].enabled, vd_layer[2].enabled_status_saved,
			vd_layer[2].global_output, vd_layer[2].disable_video,
			vd_layer[2].force_disable ? "true" : "false");
#endif

	/* filter setting management */
	for (i = 0; i < cur_dev->max_vd_layers; i++) {
		if (vd_layer[i].vd_func.vd_render_frame)
			ret = vd_layer[i].vd_func.vd_render_frame
				(&vd_layer[i], vinfo);
		if (i == 0)
			frame_par_di_set = ret;
	}

#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
	if (support_multi_core1())
		amdolby_vision_proc(&vd_layer[0], vd_layer[0].cur_frame_par,
			&vd_layer[1], vd_layer[1].cur_frame_par);
	set_amdv_delay_work_flag();
#endif

	if (path_switch &&
	    (debug_flag & DEBUG_FLAG_PRINT_PATH_SWITCH)) {
		pr_info("VID: === After path switch ===\n");
		pr_info("VID: \tpath_id: %d, %d, %d;\nVID: \ttoggle:%p, %p, %p %p, %p, %p\nVID: \tcur:%p, %p, %p, %p, %p, %p;\n",
			vd_path_id[0], vd_path_id[1], vd_path_id[2],
			path_new_frame[0], path_new_frame[1],
			path_new_frame[2], path_new_frame[3],
			path_new_frame[4], path_new_frame[5],
			cur_dispbuf[0], cur_dispbuf[1], cur_dispbuf[2],
			gvideo_recv[0] ? gvideo_recv[0]->cur_buf : NULL,
			gvideo_recv[1] ? gvideo_recv[1]->cur_buf : NULL,
			gvideo_recv[2] ? gvideo_recv[2]->cur_buf : NULL);
		pr_info("VID: \tdispbuf:%p, %p, %p; \tvf_ext:%p, %p, %p;\nVID: \tlocal:%p, %p, %p, %p, %p, %p\n",
			vd_layer[0].dispbuf, vd_layer[1].dispbuf,
			vd_layer[2].dispbuf,
			vd_layer[0].vf_ext, vd_layer[1].vf_ext,
			vd_layer[2].vf_ext,
			&vf_local[0], &vf_local[1], &vf_local[2],
			gvideo_recv[0] ? &gvideo_recv[0]->local_buf : NULL,
			gvideo_recv[1] ? &gvideo_recv[1]->local_buf : NULL,
			gvideo_recv[2] ? &gvideo_recv[2]->local_buf : NULL);
		pr_info("VID: \tblackout:%d %d, %d force:%d;\n",
			blackout[0], blackout[1], blackout[2], force_blackout);
	}

	/* di post process */
	if (cur_dispbuf[0] && cur_dispbuf[0]->process_fun &&
	    (vd_path_id[0] == VFM_PATH_AMVIDEO ||
	     vd_path_id[0] == VFM_PATH_DEF))
		di_post_vf = cur_dispbuf[0];
	else if (vd_layer[0].dispbuf &&
		 vd_layer[0].dispbuf->process_fun &&
		 is_di_post_mode(vd_layer[0].dispbuf))
		di_post_vf = vd_layer[0].dispbuf;
	if (vd_layer[0].do_switch)
		di_post_vf = NULL;
	if (di_post_vf) {
		/* for new deinterlace driver */
#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
		if (debug_flag & DEBUG_FLAG_PRINT_RDMA) {
			if (enable_rdma_log_count > 0)
				pr_info("call process_fun\n");
		}
#endif
		if (cur_frame_par[0])
			di_post_vf->process_fun
				(di_post_vf->private_data,
				vd_layer[0].start_x_lines |
				(cur_frame_par[0]->vscale_skip_count <<
				24) | (frame_par_di_set << 16),
				vd_layer[0].end_x_lines,
				vd_layer[0].start_y_lines,
				vd_layer[0].end_y_lines,
				di_post_vf);
		di_post_process_done = true;
	}
exit:
#ifdef CONFIG_AMLOGIC_MEDIA_DEINTERLACE
	if (legacy_vpp &&
	    !di_post_process_done &&
	    is_di_post_on())
		DI_POST_UPDATE_MC();
#endif
#ifdef ENABLE_PRE_LINK
	if (!vd_layer[0].need_disable_prelink || !is_pre_link_on(&vd_layer[0]))
		atomic_set(&vd_layer[0].disable_prelink_done, 1);
#endif

	/* update alpha win */
	if (cur_dev->pre_vsync_enable)
		alpha_win_set(&vd_layer[0]);

	/* do blend,judge really update in update_vpp_input_info for vpp_index */
	vpp_blend_update(vinfo, VPP0);

	/* late process */
	for (i = 0; i < cur_dev->max_vd_layers; i++)
		if (vd_layer[i].vd_func.vd_late_process)
			vd_layer[i].vd_func.vd_late_process(i, 0);
	for (i = 0; i < MAX_VIDEO_FAKE; i++) {
		if (!vd_fake_func[i].video_process_flag)
			if (vd_fake_func[i].vd_late_process)
				vd_fake_func[i].vd_late_process(0xff, i);
	}
LATE_PROC:
	misc_late_proc();
	for (i = 0; i < MAX_VD_LAYER; i++)
		cur_vd_path_id[i] = vd_path_id[i];
	if (new_frame_cnt == 1 && !vsync_count_start) {
		vsync_count_start = true;
		pr_info("%s, vsync_count_started\n", __func__);
	}

#endif
#if IS_ENABLED(CONFIG_AMLOGIC_DEBUG_IOTRACE)
	iotrace_misc_record_write(RECORD_TYPE_VSYNC_OUT, 0, 0, 0);
#endif
}

int get_current_frame_para(int *top, int *left, int *bottom, int *right)
{
	if (!cur_frame_par[0])
		return -1;
	*top = cur_frame_par[0]->VPP_vd_start_lines_;
	*left = cur_frame_par[0]->VPP_hd_start_lines_;
	*bottom = cur_frame_par[0]->VPP_vd_end_lines_;
	*right = cur_frame_par[0]->VPP_hd_end_lines_;
	return 0;
}

int get_current_vscale_skip_count(struct vframe_s *vf)
{
	int ret = 0;
	static struct vpp_frame_par_s frame_par;

	ret = vpp_set_filters
			(&glayer_info[0],
			vf, &frame_par, vinfo,
			(is_amdv_on() &&
			is_amdv_stb_mode() &&
			for_amdv_certification()),
			OP_FORCE_NOT_SWITCH_VF);
	if (ret < 0) {
		pr_info("%s vpp_set_filter fail\n", __func__);
		return ret;
	}
	ret = frame_par.vscale_skip_count;
	if (cur_frame_par[0] && (process_3d_type & MODE_3D_ENABLE))
		ret |= (cur_frame_par[0]->vpp_3d_mode << 8);
	return ret;
}

void release_di_buffer(int inst)
{
	int i;

	for (i = 0; i < recycle_cnt[inst]; i++) {
		if (recycle_buf[inst][i] &&
		    recycle_buf[inst][i]->flag & VFRAME_FLAG_DI_PW_VFM &&
		    IS_DI_POSTWRTIE(recycle_buf[inst][i]->type)) {
			di_release_count++;
#ifdef CONFIG_AMLOGIC_MEDIA_DEINTERLACE
			dim_post_keep_cmd_release2(recycle_buf[inst][i]);
#endif
		}
		recycle_buf[inst][i] = NULL;
	}
	recycle_cnt[inst] = 0;
}

int  get_display_info(void *data)
{
	s32 w, h, x, y;
	s32 x_end, y_end;
	struct vdisplay_info_s  *info_para = (struct vdisplay_info_s *)data;
	const struct vinfo_s *info = get_current_vinfo();
	struct disp_info_s *layer = &glayer_info[0];
	struct vpp_frame_par_s *frame_par = cur_frame_par[0];

	if (!frame_par || !info)
		return -1;
	if (info->mode == VMODE_INVALID)
		return -1;

	x = layer->layer_left;
	y = layer->layer_top;
	w = layer->layer_width;
	h = layer->layer_height;

	/* reverse and mirror case */
	if (layer->reverse) {
		/* reverse x/y start */
		x_end = x + w - 1;
		x = info->width - x_end - 1;
		y_end = y + h - 1;
		y = info->height - y_end - 1;
	} else if (layer->mirror == H_MIRROR) {
		/* horizontal mirror */
		x_end = x + w - 1;
		x = info->width - x_end - 1;
	} else if (layer->mirror == V_MIRROR) {
		/* vertical mirror */
		y_end = y + h - 1;
		y = info->height - y_end - 1;
	}

	if (w == 0 || w  > info->width)
		w =  info->width;
	if (h == 0 || h  > info->height)
		h =  info->height;

	info_para->frame_hd_start_lines_ = frame_par->VPP_hd_start_lines_;
	info_para->frame_hd_end_lines_ = frame_par->VPP_hd_end_lines_;
	info_para->frame_vd_start_lines_ = frame_par->VPP_vd_start_lines_;
	info_para->frame_vd_end_lines_ = frame_par->VPP_vd_end_lines_;
	info_para->display_hsc_startp = frame_par->VPP_hsc_startp;
	info_para->display_hsc_endp = frame_par->VPP_hsc_endp;
	info_para->display_vsc_startp = frame_par->VPP_vsc_startp;
	info_para->display_vsc_endp = frame_par->VPP_vsc_endp;
	info_para->screen_vd_h_start_ =
	frame_par->VPP_post_blend_vd_h_start_;
	info_para->screen_vd_h_end_ =
	frame_par->VPP_post_blend_vd_h_end_;
	info_para->screen_vd_v_start_ =
	frame_par->VPP_post_blend_vd_v_start_;
	info_para->screen_vd_v_end_ = frame_par->VPP_post_blend_vd_v_end_;

	return 0;
}

struct vframe_s *get_dispbuf(u8 layer_id)
{
	struct vframe_s *dispbuf = NULL;
	struct video_layer_s *layer = NULL;
	int i = 0;

	if (layer_id >= MAX_VD_LAYERS)
		return NULL;
	switch (glayer_info[layer_id].display_path_id) {
	case VFM_PATH_DEF:
		for (i = 0; i < MAX_VD_LAYER; i++)
			if (layer_id == i && cur_dispbuf[i])
				dispbuf = cur_dispbuf[i];
		break;
	case VFM_PATH_AMVIDEO:
		if (cur_dispbuf[0])
			dispbuf = cur_dispbuf[0];
		break;
	case VFM_PATH_PIP:
		if (cur_dispbuf[1])
			dispbuf = cur_dispbuf[1];
		break;
	case VFM_PATH_PIP2:
		if (cur_dispbuf[2])
			dispbuf = cur_dispbuf[2];
		break;
	case VFM_PATH_VIDEO_RENDER0:
		if (gvideo_recv[0] &&
		    gvideo_recv[0]->cur_buf)
			dispbuf = gvideo_recv[0]->cur_buf;
		break;
	case VFM_PATH_VIDEO_RENDER1:
		if (gvideo_recv[1] &&
		    gvideo_recv[1]->cur_buf)
			dispbuf = gvideo_recv[1]->cur_buf;
		break;
	case VFM_PATH_VIDEO_RENDER2:
		if (gvideo_recv[2] &&
		    gvideo_recv[2]->cur_buf)
			dispbuf = gvideo_recv[2]->cur_buf;
		break;
	case VFM_PATH_VIDEO_RENDER5:
		if (gvideo_recv_vpp[0] &&
		    gvideo_recv_vpp[0]->cur_buf)
			dispbuf = gvideo_recv_vpp[0]->cur_buf;
		break;
	case VFM_PATH_VIDEO_RENDER6:
		if (gvideo_recv_vpp[1] &&
		    gvideo_recv_vpp[1]->cur_buf)
			dispbuf = gvideo_recv_vpp[1]->cur_buf;
		break;
	default:
		break;
	}

	layer = get_layer_by_layer_id(layer_id);
	if (layer && dispbuf &&
	    !is_local_vf(dispbuf) &&
	    layer->switch_vf && layer->vf_ext)
		dispbuf = layer->vf_ext;
	return dispbuf;
}

static void pipx_vf_unreg_provider(u8 path_index)
{
	ulong flags;
	int keeped = 0, ret = 0;
	bool layer1_used = false;
	bool layer2_used = false;
	bool layer3_used = false;
	u32 enabled = 0;
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

	ret = update_video_recycle_buffer(path_index);
	if (ret == -EAGAIN) {
	/* The currently displayed vf is not added to the queue
	 * that needs to be released. You need to release the vf
	 * data in the release queue before adding the currently
	 * displayed vf to the release queue.
	 */
		release_di_buffer(path_index);
		update_video_recycle_buffer(path_index);
	}
#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
	dispbuf_to_put_num[path_index] = 0;
	for (i = 0; i < DISPBUF_TO_PUT_MAX; i++)
		dispbuf_to_put[path_index][i] = NULL;

	cur_rdma_buf[path_index] = NULL;
#endif
	i = path_index;
	if (cur_dispbuf[path_index]) {
		if (cur_dispbuf[path_index]->vf_ext &&
		    IS_DI_POSTWRTIE(cur_dispbuf[path_index]->type)) {
			struct vframe_s *tmp;

			if (cur_dispbuf[path_index]->uvm_vf)
				tmp = cur_dispbuf[path_index]->uvm_vf;
			else
				tmp = (struct vframe_s *)cur_dispbuf[path_index]->vf_ext;
			memcpy(&tmp->pic_mode, &cur_dispbuf[path_index]->pic_mode,
				sizeof(struct vframe_pic_mode_s));
			vf_local_ext[path_index] = *tmp;
			vf_local[path_index] = *cur_dispbuf[path_index];
			vf_local[path_index].vf_ext = (void *)&vf_local_ext[path_index];
			vf_local[path_index].uvm_vf = NULL;
			vf_local_ext[path_index].ratio_control = vf_local[path_index].ratio_control;
		} else if (cur_dispbuf[path_index]->vf_ext &&
			is_pre_link_source(cur_dispbuf[path_index])) {
			u32 tmp_rc;
			struct vframe_s *tmp;

			if (cur_dispbuf[path_index]->uvm_vf)
				tmp = cur_dispbuf[path_index]->uvm_vf;
			else
				tmp = (struct vframe_s *)cur_dispbuf[path_index]->vf_ext;
			if (debug_flag & DEBUG_FLAG_PRELINK)
				pr_info("%s %d: prelink: cur_dispbuf:%px vf_ext:%px uvm_vf:%px flag:%x\n",
					__func__, path_index,
					cur_dispbuf[path_index],
					cur_dispbuf[path_index]->vf_ext,
					cur_dispbuf[path_index]->uvm_vf,
					cur_dispbuf[path_index]->flag);
			tmp_rc = cur_dispbuf[path_index]->ratio_control;
			memcpy(&tmp->pic_mode, &cur_dispbuf[path_index]->pic_mode,
				sizeof(struct vframe_pic_mode_s));
			vf_local[path_index] = *tmp;
			vf_local[path_index].ratio_control = tmp_rc;
			vf_local[path_index].vf_ext = NULL;
			vf_local[path_index].uvm_vf = NULL;
		} else if (IS_DI_POST(cur_dispbuf[path_index]->type) &&
			(cur_dispbuf[path_index]->vf_ext || cur_dispbuf[path_index]->uvm_vf)) {
			u32 tmp_rc;
			struct vframe_s *tmp;

			if (cur_dispbuf[path_index]->uvm_vf)
				tmp = cur_dispbuf[path_index]->uvm_vf;
			else
				tmp = (struct vframe_s *)cur_dispbuf[path_index]->vf_ext;
			if (debug_flag & DEBUG_FLAG_PRELINK)
				pr_info("%s %d: pre/post link: cur_dispbuf:%px vf_ext:%px uvm_vf:%px flag:%x\n",
					__func__, path_index,
					cur_dispbuf[path_index],
					cur_dispbuf[path_index]->vf_ext,
					cur_dispbuf[path_index]->uvm_vf,
					cur_dispbuf[path_index]->flag);
			tmp_rc = cur_dispbuf[path_index]->ratio_control;
			memcpy(&tmp->pic_mode, &cur_dispbuf[path_index]->pic_mode,
				sizeof(struct vframe_pic_mode_s));
			vf_local[path_index] = *tmp;
			vf_local[path_index].ratio_control = tmp_rc;
			vf_local[path_index].vf_ext = NULL;
			vf_local[path_index].uvm_vf = NULL;

		} else {
			vf_local[path_index] = *cur_dispbuf[path_index];
			vf_local[path_index].vf_ext = NULL;
			vf_local[path_index].uvm_vf = NULL;

		}
		cur_dispbuf[path_index] = &vf_local[path_index];
		cur_dispbuf[path_index]->video_angle = 0;
	}
	pip_frame_count[path_index] = 0;
	spin_unlock_irqrestore(&lock, flags);

	if (vd_layer[0].dispbuf_mapping
		== &cur_dispbuf[path_index]) {
		layer1_used = true;
		enabled |= get_video_enabled(0);
	}
	if (vd_layer[1].dispbuf_mapping
		== &cur_dispbuf[path_index]) {
		layer2_used = true;
		enabled |= get_video_enabled(1);
	}
	if (vd_layer[2].dispbuf_mapping
		== &cur_dispbuf[path_index]) {
		layer3_used = true;
		enabled |= get_video_enabled(2);
	}

	if (!layer1_used && !layer2_used && !layer3_used)
		cur_dispbuf[path_index] = NULL;

	if (blackout[2] | force_blackout) {
		if (layer1_used)
			safe_switch_videolayer
				(0, false, false);
		if (layer2_used)
			safe_switch_videolayer
				(1, false, false);
		if (layer3_used)
			safe_switch_videolayer
				(2, false, false);
		try_free_keep_vdx(path_index, 1);
	}

	if (cur_dispbuf[path_index] && enabled)
		keeped = vf_keep_current_locked(path_index, cur_dispbuf[path_index], NULL);
	else if (cur_dispbuf[path_index])
		keeped = 0;

	pr_info("%s: vd1 used: %s, vd2 used: %s, vd3 used: %s, keep_ret:%d, black_out:%d, cur_dispbuf%d:%p\n",
		__func__,
		layer1_used ? "true" : "false",
		layer2_used ? "true" : "false",
		layer3_used ? "true" : "false",
		keeped, blackout[path_index] | force_blackout,
		path_index,
		cur_dispbuf[path_index]);

	if (keeped <= 0) {/*keep failed.*/
		if (keeped < 0)
			pr_info("keep frame failed, disable videopip%d now.\n",
				path_index - 1);
		else
			pr_info("keep frame skip, disable videopip%d again.\n",
				path_index - 1);
		if (layer1_used)
			safe_switch_videolayer
				(0, false, false);
		if (layer2_used)
			safe_switch_videolayer
				(1, false, false);
		if (layer3_used)
			safe_switch_videolayer
				(2, false, false);
		try_free_keep_vdx(path_index, 1);
	}

	/*disable_videopip = VIDEO_DISABLE_FORNEXT;*/
	/*DisableVideoLayer2();*/
	atomic_dec(&video_unreg_flag);
}

static void pipx_vf_light_unreg_provider(u8 path_index, int need_keep_frame)
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
	ret = update_video_recycle_buffer(path_index);
	if (ret == -EAGAIN) {
	/* The currently displayed vf is not added to the queue
	 * that needs to be released. You need to release the vf
	 * data in the release queue before adding the currently
	 * displayed vf to the release queue.
	 */
		release_di_buffer(path_index);
		update_video_recycle_buffer(path_index);
	}
#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
	dispbuf_to_put_num[path_index] = 0;
	for (i = 0; i < DISPBUF_TO_PUT_MAX; i++)
		dispbuf_to_put[path_index][i] = NULL;
	cur_rdma_buf[path_index] = NULL;
#endif

	if (cur_dispbuf[path_index]) {
		if (cur_dispbuf[path_index]->vf_ext &&
		    IS_DI_POSTWRTIE(cur_dispbuf[path_index]->type)) {
			struct vframe_s *tmp;

			if (cur_dispbuf[path_index]->uvm_vf)
				tmp = cur_dispbuf[path_index]->uvm_vf;
			else
				tmp = (struct vframe_s *)cur_dispbuf[path_index]->vf_ext;
			memcpy(&tmp->pic_mode, &cur_dispbuf[path_index]->pic_mode,
				sizeof(struct vframe_pic_mode_s));
			vf_local_ext[path_index] = *tmp;
			vf_local[path_index] = *cur_dispbuf[path_index];
			vf_local[path_index].vf_ext = (void *)&vf_local_ext[path_index];
			vf_local[path_index].uvm_vf = NULL;
			vf_local_ext[path_index].ratio_control = vf_local[path_index].ratio_control;
		} else if (cur_dispbuf[path_index]->vf_ext &&
			is_pre_link_source(cur_dispbuf[path_index])) {
			u32 tmp_rc;
			struct vframe_s *tmp;

			if (cur_dispbuf[path_index]->uvm_vf)
				tmp = cur_dispbuf[path_index]->uvm_vf;
			else
				tmp = (struct vframe_s *)cur_dispbuf[path_index]->vf_ext;
			if (debug_flag & DEBUG_FLAG_PRELINK)
				pr_info("%s %d: prelink: cur_dispbuf:%px vf_ext:%px uvm_vf:%px flag:%x\n",
					__func__, path_index,
					cur_dispbuf[path_index],
					cur_dispbuf[path_index]->vf_ext,
					cur_dispbuf[path_index]->uvm_vf,
					cur_dispbuf[path_index]->flag);
			tmp_rc = cur_dispbuf[path_index]->ratio_control;
			memcpy(&tmp->pic_mode, &cur_dispbuf[path_index]->pic_mode,
				sizeof(struct vframe_pic_mode_s));
			vf_local[path_index] = *tmp;
			vf_local[path_index].ratio_control = tmp_rc;
			vf_local[path_index].vf_ext = NULL;
			vf_local[path_index].uvm_vf = NULL;
		} else if (IS_DI_POST(cur_dispbuf[path_index]->type) &&
			(cur_dispbuf[path_index]->vf_ext || cur_dispbuf[path_index]->uvm_vf)) {
			u32 tmp_rc;
			struct vframe_s *tmp;

			if (cur_dispbuf[path_index]->uvm_vf)
				tmp = cur_dispbuf[path_index]->uvm_vf;
			else
				tmp = (struct vframe_s *)cur_dispbuf[path_index]->vf_ext;
			if (debug_flag & DEBUG_FLAG_PRELINK)
				pr_info("%s %d: pre/post link: cur_dispbuf:%px vf_ext:%px uvm_vf:%px flag:%x\n",
					__func__, path_index,
					cur_dispbuf[path_index],
					cur_dispbuf[path_index]->vf_ext,
					cur_dispbuf[path_index]->uvm_vf,
					cur_dispbuf[path_index]->flag);
			tmp_rc = cur_dispbuf[path_index]->ratio_control;
			memcpy(&tmp->pic_mode, &cur_dispbuf[path_index]->pic_mode,
				sizeof(struct vframe_pic_mode_s));
			vf_local[path_index] = *tmp;
			vf_local[path_index].ratio_control = tmp_rc;
			vf_local[path_index].vf_ext = NULL;
			vf_local[path_index].uvm_vf = NULL;

		} else {
			vf_local[path_index] = *cur_dispbuf[path_index];
			vf_local[path_index].vf_ext = NULL;
			vf_local[path_index].uvm_vf = NULL;

		}
		cur_dispbuf[path_index] = &vf_local[path_index];
	}

	spin_unlock_irqrestore(&lock, flags);

	if (need_keep_frame && cur_dispbuf[path_index])
		vf_keep_current_locked(path_index, cur_dispbuf[path_index], NULL);

	atomic_dec(&video_unreg_flag);
}

static int pipx_receiver_event_fun(u8 path_index,
		int type,
		void *data,
		void *private_data)
{
	if (type == VFRAME_EVENT_PROVIDER_UNREG) {
		pipx_vf_unreg_provider(path_index);

		/* for pip */
		if (path_index == 1) {
			atomic_dec(&video_recv_cnt);

#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
			if (is_amdv_enable()) {
				if (dv_inst_pip >= 0) {/*tunnel modes*/
					dv_inst_unmap(dv_inst_pip);
					dv_inst_pip = -1;
				}
			}
#endif
		}
	} else if (type == VFRAME_EVENT_PROVIDER_RESET) {
		pipx_vf_light_unreg_provider(path_index, 1);
	} else if (type == VFRAME_EVENT_PROVIDER_LIGHT_UNREG) {
		pipx_vf_light_unreg_provider(path_index, 0);
	} else if (type == VFRAME_EVENT_PROVIDER_REG) {
		atomic_inc(&video_recv_cnt);
		pipx_vf_light_unreg_provider(path_index, 0);
		video_drop_vf_cnt[path_index] = 0;
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
		if (path_index == 1) {
			if (is_amdv_enable()) {
				if (is_tunnel_mode(RECEIVERPIP_NAME)) {
					dv_inst_map(&dv_inst_pip);
					pr_info("pip receiver dv_inst_pip %d\n", dv_inst_pip);
				}
			}
		}
#endif
	}
	return 0;
}

int pip_receiver_event_fun(int type,
				  void *data,
				  void *private_data)
{
	pipx_receiver_event_fun(1, type, data, private_data);
	return 0;
}

int pip2_receiver_event_fun(int type,
				  void *data,
				  void *private_data)
{
	pipx_receiver_event_fun(2, type, data, private_data);
	return 0;
}

///////////////////////////////////////////////////////
ssize_t blend_conflict_show(struct class *cla,
		struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "blend_conflict_cnt: %d\n", blend_conflict_cnt);
}

unsigned int get_post_canvas(void)
{
	return post_canvas;
}
EXPORT_SYMBOL(get_post_canvas);

u32 get_blackout_policy(void)
{
	/* only for vd1 */
	return blackout[0] | force_blackout;
}
EXPORT_SYMBOL(get_blackout_policy);

u32 get_vdx_blackout_policy(u8 layer_id)
{
	if (layer_id >= MAX_VD_LAYER)
		return 1;
	else
		return blackout[layer_id] | force_blackout;
}
EXPORT_SYMBOL(get_vdx_blackout_policy);

u32 set_vdx_blackout_policy(u8 layer_id, int policy)
{
	if (layer_id >= MAX_VD_LAYER)
		return 1;
	blackout[layer_id] = policy;
	return 0;
}
EXPORT_SYMBOL(set_vdx_blackout_policy);

void pause_video(unsigned char pause_flag)
{
	atomic_set(&video_pause_flag, pause_flag ? 1 : 0);
}
EXPORT_SYMBOL(pause_video);

void di_unreg_notify(void)
{
	u32 sleep_time = 40;

	while (atomic_read(&video_inirq_flag) > 0)
		schedule();
	if (cur_dev->pre_vsync_enable)
		while (atomic_read(&video_prevsync_inirq_flag) > 0)
			schedule();

	if (gvideo_recv[0] && gvideo_recv[0]->active)
		switch_vf(gvideo_recv[0], true);

	vd_layer[0].need_switch_vf = true;
	vd_layer[0].property_changed = true;

	if (vinfo) {
		sleep_time = vinfo->sync_duration_den * 1000;
		if (vinfo->sync_duration_num) {
			sleep_time /= vinfo->sync_duration_num;
			/* need two vsync */
			sleep_time = (sleep_time + 1) * 2;
		} else {
			sleep_time = 40;
		}
	}
	msleep(sleep_time);
}
EXPORT_SYMBOL(di_unreg_notify);

void di_disable_prelink_notify(bool async)
{
	u32 sleep_time = 20;
	u32 vsync_time;
	u32 sleep_cycle = 0;
	u32 skip_cycle = 0;

	while (atomic_read(&video_inirq_flag) > 0)
		schedule();
	if (cur_dev->pre_vsync_enable)
		while (atomic_read(&video_prevsync_inirq_flag) > 0)
			schedule();

	vd_layer[0].need_disable_prelink = true;
	atomic_set(&vd_layer[0].disable_prelink_done, 0);
	vd_layer[0].property_changed = true;

	if (vinfo && !async) {
		vsync_time = vinfo->sync_duration_den * 1000;
		if (vinfo->sync_duration_num) {
			vsync_time /= vinfo->sync_duration_num;
			/* need two vsync */
			vsync_time = vsync_time + 1;
			sleep_time = vsync_time >> 1;
		}
		while (vd_layer[0].need_disable_prelink &&
		    !atomic_read(&vd_layer[0].disable_prelink_done) &&
		    skip_cycle < 20) {
			skip_cycle++;
			msleep(sleep_time);
		}
		/* need one more vsync for rdma config */
		msleep(vsync_time);
		sleep_cycle = 2;
	} else {
		sleep_cycle = 2;
		msleep(sleep_time * 2);
	}
	if (debug_flag & DEBUG_FLAG_PRELINK)
		pr_info("%s: wait %dx(%d+%d) ms\n", __func__, sleep_time, sleep_cycle, skip_cycle);
}
EXPORT_SYMBOL(di_disable_prelink_notify);

void di_prelink_state_changed_notify(void)
{
	vd_layer[0].property_changed = true;
}
EXPORT_SYMBOL(di_prelink_state_changed_notify);

void di_prelink_force_dmc_priority(bool urgent, bool wait)
{
#define DI_READ_DMC_AM1_CHAN_CTRL 0x0064
#define DI_WRTIE_DMC_AM4_CHAN_CTRL 0x0070
#define DI_RDARB_UGT_L1C1 0x205b
#define DI_WRARB_UGT_L1C1 0x205d

	bool valid = false;

	if (stop_force_dmc)
		return;

	/* check priority adjustment function valid or not */
	if (!legacy_vpp) {
		if (cur_dev->display_module == OLD_DISPLAY_MODULE ||
			video_is_meson_t5w_cpu() || video_is_meson_t5m_cpu())
			valid = true;
	}
	if (!legacy_vpp) {
		u32 sleep_time = 40;

		if (!valid)
			wait = false;

		while (atomic_read(&video_inirq_flag) > 0 && wait)
			schedule();

		if (vinfo && wait) {
			sleep_time = vinfo->sync_duration_den * 1000;
			if (vinfo->sync_duration_num) {
				sleep_time /= vinfo->sync_duration_num;
				/* need two vsync */
				sleep_time = (sleep_time + 1) * 2;
			} else {
				sleep_time = 40;
			}
		}
		if (wait)
			msleep(sleep_time);
		else
			sleep_time = 0;

		if (cur_dev->display_module == OLD_DISPLAY_MODULE ||
			video_is_meson_t5w_cpu()) {
			WRITE_DMCREG
				(DI_READ_DMC_AM1_CHAN_CTRL,
				urgent ? 0xCFF403C4 : 0xCFF203C4);
			WRITE_DMCREG
				(DI_READ_DMC_AM1_CHAN_CTRL,
				urgent ? 0xCFF403C4 : 0xCFF203C4);
			if (debug_flag & DEBUG_FLAG_PRELINK)
				pr_info("%s: port:0x%x 0x%x to 0x%x (%s) wait:%s time %dms\n",
					__func__,
					DI_READ_DMC_AM1_CHAN_CTRL,
					DI_WRTIE_DMC_AM4_CHAN_CTRL,
					urgent ? 0xCFF403C4 : 0xCFF203C4,
					urgent ? "super urgent" : "not urgent",
					wait ? "true" : "false",
					sleep_time);
		}
		if (video_is_meson_t5m_cpu()) {
			WRITE_VCBUS_REG
				(DI_RDARB_UGT_L1C1,
				 urgent ? 0x3ffff : 0x15555);
			WRITE_VCBUS_REG
				(DI_WRARB_UGT_L1C1,
				 urgent ? 0xfff : 0x555);
			if (debug_flag & DEBUG_FLAG_PRELINK)
				pr_info("%s: port:0x%x 0x%x to 0x%x 0x%x (%s) wait:%s time %dms\n",
					__func__,
					DI_RDARB_UGT_L1C1,
					DI_WRARB_UGT_L1C1,
					urgent ? 0x3ffff : 0x15555,
					urgent ? 0xfff : 0x555,
					urgent ? "super urgent" : "not urgent",
					wait ? "true" : "false",
					sleep_time);
		}
	}
}
EXPORT_SYMBOL(di_prelink_force_dmc_priority);

u32 get_playback_delay_duration(void)
{
	u32 memc_delay = 0;

#ifdef CONFIG_AMLOGIC_MEDIA_FRC
	memc_delay = frc_get_video_latency();
#endif
	return memc_delay;
}
EXPORT_SYMBOL(get_playback_delay_duration);

u32 get_video_angle(void)
{
	return glayer_info[0].angle;
}
EXPORT_SYMBOL(get_video_angle);

void set_video_zorder(u32 zorder, u32 index)
{
	if (index < 2)
		glayer_info[index].zorder = zorder;
}
EXPORT_SYMBOL(set_video_zorder);

int video_property_notify(int flag)
{
	vd_layer[0].property_changed = flag ? true : false;
	return 0;
}
EXPORT_SYMBOL(video_property_notify);

void set_vsync_2to1_mode(u8 enable)
{
	if (cur_dev->prevsync_support)
		cur_dev->vsync_2to1_enable = enable;
}
EXPORT_SYMBOL(set_vsync_2to1_mode);

void set_pre_vsync_mode(u8 enable)
{
	if (cur_dev->prevsync_support) {
		cur_dev->pre_vsync_enable = enable;
		vd1_set_go_field();
	}
}
EXPORT_SYMBOL(set_pre_vsync_mode);

/*********************************************************
 * Utilities
 *********************************************************/
int _video_set_disable(u32 val)
{
	struct video_layer_s *layer = &vd_layer[0];

	if (val > VIDEO_DISABLE_FORNEXT)
		return -EINVAL;

	layer->disable_video = val;
	if (layer->disable_video ==
	     VIDEO_DISABLE_FORNEXT &&
	    layer->dispbuf &&
	    !is_local_vf(layer->dispbuf))
		layer->disable_video = VIDEO_DISABLE_NONE;

	if (layer->disable_video != VIDEO_DISABLE_NONE) {
		pr_info("VID: VD1 off\n");
		safe_switch_videolayer
			(layer->layer_id, false, true);

		if (layer->disable_video ==
		     VIDEO_DISABLE_FORNEXT &&
		    layer->dispbuf &&
		    !is_local_vf(layer->dispbuf))
			layer->property_changed = true;
		/* FIXME */
		try_free_keep_vdx(layer->keep_frame_id, 0);
	} else {
		if (layer->dispbuf &&
		    !is_local_vf(layer->dispbuf)) {
			safe_switch_videolayer
				(layer->layer_id, true, true);
			pr_info("VID: VD1 on\n");
			layer->property_changed = true;
		}
	}
	if (get_video_debug_flags() & DEBUG_FLAG_HDMI_DV_CRC)
		dump_stack();

	return 0;
}

void video_set_global_output(u32 index, u32 val)
{
	if (index == 0) {
		if (val != 0)
			vd_layer[0].global_output = 1;
		else
			vd_layer[0].global_output = 0;
	} else if (index == 1) {
		if (vd_layer[1].vpp_index == VPP0) {
			if (val != 0)
				vd_layer[1].global_output = 1;
			else
				vd_layer[1].global_output = 0;
		}
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
		else if (vd_layer_vpp[0].layer_id == index &&
			vd_layer_vpp[0].vpp_index == VPP1) {
			if (val != 0)
				vd_layer_vpp[0].global_output = 1;
			else
				vd_layer_vpp[0].global_output = 0;
		} else if (vd_layer_vpp[1].layer_id == index &&
			vd_layer_vpp[1].vpp_index == VPP2) {
			if (val != 0)
				vd_layer_vpp[1].global_output = 1;
			else
				vd_layer_vpp[1].global_output = 0;
		}
#endif
	} else if (index == 2) {
		if (vd_layer[2].vpp_index == VPP0) {
			if (val != 0)
				vd_layer[2].global_output = 1;
			else
				vd_layer[2].global_output = 0;
		}
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
		else if (vd_layer_vpp[0].layer_id == index &&
			vd_layer_vpp[0].vpp_index == VPP1) {
			if (val != 0)
				vd_layer_vpp[0].global_output = 1;
			else
				vd_layer_vpp[0].global_output = 0;
		} else if (vd_layer_vpp[1].layer_id == index &&
			vd_layer_vpp[1].vpp_index == VPP2) {
			if (val != 0)
				vd_layer_vpp[1].global_output = 1;
			else
				vd_layer_vpp[1].global_output = 0;
		}
#endif
	}
	pr_info("VID: VD%d set global output as %d\n",
		index + 1, (val != 0) ? 1 : 0);
}

struct video_layer_s *get_layer_by_layer_id(u8 layer_id)
{
	struct video_layer_s *layer = NULL;

	if (layer_id == 0) {
		layer = &vd_layer[0];
	} else if (layer_id == 1) {
		if (vd_layer[layer_id].vpp_index == VPP0)
			layer = &vd_layer[1];
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
		/* vpp1 case */
		else if (vd_layer_vpp[0].layer_id == layer_id &&
			vd_layer_vpp[0].vpp_index == VPP1)
			layer = &vd_layer_vpp[0];
		/* vpp2 case */
		else if (vd_layer_vpp[1].layer_id == layer_id &&
			vd_layer_vpp[1].vpp_index == VPP2)
			layer = &vd_layer_vpp[1];
#endif
	} else if (layer_id == 2) {
		if (vd_layer[layer_id].vpp_index == VPP0)
			layer = &vd_layer[2];
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
		/* vpp1 case */
		else if (vd_layer_vpp[0].layer_id == layer_id &&
			vd_layer_vpp[0].vpp_index == VPP1)
			layer = &vd_layer_vpp[0];
		/* vpp2 case */
		else if (vd_layer_vpp[1].layer_id == layer_id &&
			vd_layer_vpp[1].vpp_index == VPP2)
			layer = &vd_layer_vpp[1];
#endif
	}
	return layer;
}

int _videopip_set_disable(u32 index, u32 val)
{
	struct video_layer_s *layer = NULL;

	if (val > VIDEO_DISABLE_FORNEXT)
		return -EINVAL;

	layer = get_layer_by_layer_id(index);

	if (!layer)
		return -EINVAL;

	layer->disable_video = val;

	if (layer->disable_video ==
	     VIDEO_DISABLE_FORNEXT &&
	    layer->dispbuf &&
	    !is_local_vf(layer->dispbuf))
		layer->disable_video = VIDEO_DISABLE_NONE;

	if (layer->disable_video != VIDEO_DISABLE_NONE) {
		pr_info("VID: VD%d off\n", index + 1);
		safe_switch_videolayer
			(layer->layer_id, false, true);

		if (layer->disable_video ==
		     VIDEO_DISABLE_FORNEXT &&
		    layer->dispbuf &&
		    !is_local_vf(layer->dispbuf))
			layer->property_changed = true;
		/* FIXME */
		try_free_keep_vdx(layer->keep_frame_id, 0);
	} else {
		if (layer->dispbuf &&
		    !is_local_vf(layer->dispbuf)) {
			safe_switch_videolayer
				(layer->layer_id, true, true);
			pr_info("VID: VD%d on\n", index + 1);
			layer->property_changed = true;
		}
	}
	return 0;
}

s32 set_video_path_select(const char *recv_name, u8 layer_id)
{
	u32 new_path_id;
	struct disp_info_s *layer_info;
	struct video_layer_s *layer;

	if (!recv_name ||
	    layer_id >= MAX_VD_LAYERS)
		return -1;

	layer_info = &glayer_info[layer_id];
	layer = get_layer_by_layer_id(layer_id);
	new_path_id = layer_info->display_path_id;
	if (!strcmp(recv_name, "default"))
		new_path_id = VFM_PATH_DEF;
	else if (!strcmp(recv_name, RECEIVER_NAME))
		new_path_id = VFM_PATH_AMVIDEO;
	else if (!strcmp(recv_name, RECEIVERPIP_NAME))
		new_path_id = VFM_PATH_PIP;
	else if (!strcmp(recv_name, RECEIVERPIP2_NAME))
		new_path_id = VFM_PATH_PIP2;
	else if (!strcmp(recv_name, "video_render.0"))
		new_path_id = VFM_PATH_VIDEO_RENDER0;
	else if (!strcmp(recv_name, "video_render.1"))
		new_path_id = VFM_PATH_VIDEO_RENDER1;
	else if (!strcmp(recv_name, "video_render.2"))
		new_path_id = VFM_PATH_VIDEO_RENDER2;
	else if (!strcmp(recv_name, "video_render.5"))
		new_path_id = VFM_PATH_VIDEO_RENDER5;
	else if (!strcmp(recv_name, "video_render.6"))
		new_path_id = VFM_PATH_VIDEO_RENDER6;
	else if (!strcmp(recv_name, "auto"))
		new_path_id = VFM_PATH_AUTO;
	else if (!strcmp(recv_name, "invalid"))
		new_path_id = VFM_PATH_INVALID;
	if (layer_info->display_path_id != new_path_id && layer) {
		pr_info("VID: store VD%d path_id changed %d->%d\n",
			layer_id, layer_info->display_path_id, new_path_id);
		layer_info->display_path_id = new_path_id;
		layer->property_changed = true;
		if (new_path_id == VFM_PATH_AUTO)
			layer_info->sideband_type = -1;
	}
	return 0;
}
EXPORT_SYMBOL(set_video_path_select);

s32 set_sideband_type(s32 type, u8 layer_id)
{
	struct disp_info_s *layer_info;

	if (layer_id >= MAX_VD_LAYERS)
		return -1;

	layer_info = &glayer_info[layer_id];
	pr_info("VID: sideband_type %d changed to %d\n",
		layer_info->sideband_type, type);
	layer_info->sideband_type = type;

	return 0;
}
EXPORT_SYMBOL(set_sideband_type);

/* dummy_data is ycbcr */
void set_post_blend_dummy_data(u32 vpp_index,
	u32 dummy_data, u32 dummy_alpha)
{
	if (vpp_index == 0) {
		vd_layer[0].video_en_bg_color = dummy_data;
		vd_layer[0].video_dis_bg_color = dummy_data;
		vd_layer[0].dummy_alpha = dummy_alpha;
	}
}
EXPORT_SYMBOL(set_post_blend_dummy_data);

/*return index: 0 post vsync, 3 pre vsync*/
u32 get_vpp_vsync_index(u32 layerid)
{
	if (layerid < MAX_VD_LAYER)
		return vd_layer[layerid].vpp_index;

	pr_info("error layerid %d\n", layerid);
	return 0;
}
EXPORT_SYMBOL(get_vpp_vsync_index);

MODULE_PARM_DESC(stop_update, "\n stop_update\n");
module_param(stop_update, uint, 0664);

MODULE_PARM_DESC(pre_vsync_count, "\n pre_vsync_count\n");
module_param(pre_vsync_count, uint, 0664);

MODULE_PARM_DESC(stop_force_dmc, "\n stop_force_dmc\n");
module_param(stop_force_dmc, uint, 0664);

