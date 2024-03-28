/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __HDMITX_COMMON_H
#define __HDMITX_COMMON_H

#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/hdmi.h>

#include <drm/amlogic/meson_connector_dev.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_format_para.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_hw_common.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_edid.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_types.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_tracer.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_event_mgr.h>

struct hdmitx_common_state {
	struct hdmi_format_para para;
	enum vmode_e mode;
	u32 hdr_priority;
};

typedef void (*audio_en_callback)(bool enable);
typedef int (*audio_st_callback)(void);

struct hdmitx_ctrl_ops {
	int (*pre_enable_mode)(struct hdmitx_common *tx_comm, struct hdmi_format_para *para);
	int (*enable_mode)(struct hdmitx_common *tx_comm, struct hdmi_format_para *para);
	int (*post_enable_mode)(struct hdmitx_common *tx_comm, struct hdmi_format_para *para);
	int (*disable_mode)(struct hdmitx_common *tx_comm, struct hdmi_format_para *para);
	int (*init_uboot_mode)(enum vmode_e mode);
	void (*disable_hdcp)(struct hdmitx_common *tx_comm);
	void (*clear_pkt)(struct hdmitx_hw_common *tx_hw_base);
	void (*disable_21_work)(void);
};

struct st_debug_param {
	unsigned int avmute_frame;
};

/* 0: VESA DSC 1.2a is not supported
 * 1: up to 1 slice and up to (340 MHz/K SliceAdjust) pixel clock per slice
 * 2: up to 2 slices and up to (340 MHz/K SliceAdjust) pixel clock per slice
 * 3: up to 4 slices and up to (340 MHz/K SliceAdjust) pixel clock per slice
 * 4: up to 8 slices and up to (340 MHz/K SliceAdjust) pixel clock per slice
 * 5: up to 8 slices and up to (400 MHz/K SliceAdjust) pixel clock per slice
 * 6: up to 12 slices and up to (400 MHz/K SliceAdjust) pixel clock per slice
 * 7: up to 16 slices and up to (400 MHz/K SliceAdjust) pixel clock per slice
 * 8-15: Reserved
 */

static const u8 dsc_max_slices_num[] = {
	0,
	1,
	2,
	4,
	8,
	8,
	12,
	16
};

struct hdmitx_common {
	struct hdmitx_hw_common *tx_hw;
	struct hdmitx_ctrl_ops *ctrl_ops;
	struct connector_hpd_cb drm_hpd_cb;/*drm hpd notify*/

	char fmt_attr[16];
	/* for pxp test */
	char tst_fmt_attr[16];

	/*edid related*/
	/* edid hdr/dv cap lock, hdr/dv handle in irq, need spinlock*/
	spinlock_t edid_spinlock;
	u32 forced_edid; /* for external loading EDID */
	unsigned char EDID_buf[EDID_MAX_BLOCK * 128];
	struct rx_cap rxcap;

	/****** hdmitx state ******/
	/* Normally, after the HPD in or late resume, there will reading EDID, and
	 * notify application to select a hdmi mode output. But during the mode
	 * setting moment, there may be HPD out. It will clear the edid data, ..., etc.
	 * To avoid such case, here adds the hdmimode_mutex to let the HPD in, HPD out
	 * handler and mode setting sequentially.
	 */
	struct mutex hdmimode_mutex;

	/* save the last plug out/in work done state */
	enum hdmi_event_t last_hpd_handle_done_stat;
	/* 1, connect; 0, disconnect */
	unsigned char hpd_state;
	/* if HDMI plugin even once time, then set 1
	 * if never hdmi plugin, then keep as 0
	 * for android ott.
	 */
	u32 already_used;

	/* indicate hdmitx output ready, sw/hw mode setting done */
	bool ready;

	/*if hdmitx is in early suspend.*/
	bool suspend_flag;
	/*current hdcp mode, 2.1 or 1.4*/
	u8 hdcp_mode;
	/* hdcp2.2 bcaps */
	int hdcp_bcaps_repeater;
	/* allm_mode: 1/on 0/off */
	u32 allm_mode;
	/* contenttype:0/off 1/game, 2/graphics, 3/photo, 4/cinema */
	u32 ct_mode;
	bool it_content;
	/* When hdr_priority is 1, then dv_info will be all 0;
	 * when hdr_priority is 2, then dv_info/hdr_info will be all 0
	 * App won't get real dv_cap/hdr_cap, but can get real dv_cap2/hdr_cap2
	 */
	u32 hdr_priority;
	u32 hdr_8bit_en;
	/*current format para.*/
	struct hdmi_format_para fmt_para;
	/* HDR format state */
	u32 hdmi_last_hdr_mode;
	u32 hdmi_current_hdr_mode;

	/* 0.1% clock shift, 1080p60hz->59.94hz */
	u32 frac_rate_policy;

	/* audio */
	/* if switching from 48k pcm to 48k DD, the ACR/N parameter is same,
	 * so there is no need to update ACR/N. but for mode change, different
	 * sample rate, need to update ACR/N.
	 */
	struct aud_para cur_audio_param;
	/*audio end*/

	/****** device config ******/
	/* 0: TV product, 1: stb/soundbar product;
	 * used to check if need to notify edid to
	 * upstream hdmirx.
	 */
	u32 hdmi_repeater;
	/*hdcp control type config*/
	u32 hdcp_ctl_lvl;
	/* enc index: for non-ott product*/
	u32 enc_idx;
	/*soc limitation config*/
	u32 res_1080p;
	/* efuse ctrl state
	 * 1 disable the function
	 * 0 dont disable the function
	 */
	bool efuse_dis_hdmi_4k60;	/* 4k50,60hz */
	bool efuse_dis_output_4k;	/* all 4k resolution*/
	bool efuse_dis_hdcp_tx22;	/* hdcptx22 */
	bool efuse_dis_hdmi_tx3d;	/* 3d */
	bool efuse_dis_hdcp_tx14;	/* s1a hdcptx14 */
	u32 max_refreshrate;
	/*for color space conversion*/
	bool config_csc_en;

	/***** ced/rxsense related *****/
	bool cedst_en;
	u32 cedst_policy;
	struct workqueue_struct *cedst_wq;
	struct delayed_work work_cedst;
	u32 rxsense_policy;
	struct workqueue_struct *rxsense_wq;
	struct delayed_work work_rxsense;

	/***** VOUT related: TO move out *****/
	struct vinfo_s hdmitx_vinfo;
	struct vout_device_s *vdev;

	/****** debug & log ******/
	struct hdmitx_tracer *tx_tracer;
	struct hdmitx_event_mgr *event_mgr;
	struct st_debug_param debug_param;
};

void hdmitx_get_init_state(struct hdmitx_common *tx_common,
			   struct hdmitx_common_state *state);

/*******************************hdmitx common api*******************************/
int hdmitx_common_init(struct hdmitx_common *tx_common, struct hdmitx_hw_common *hw_comm);
int hdmitx_common_destroy(struct hdmitx_common *tx_common);
/* modename policy: get vic from name and check if support by rx;
 * return the vic of mode, if failed return HDMI_0_UNKNOWN;
 */
int hdmitx_common_parse_vic_in_edid(struct hdmitx_common *tx_comm, const char *mode);
/* validate if vic can supported. return 0 if can support, return < 0 with error reason;
 * This function used by get_mode_list;
 */
int hdmitx_common_validate_vic(struct hdmitx_common *tx_comm, u32 vic);
/* for some non-std TV, it declare 4k while MAX_TMDS_CLK
 * not match 4K format, so filter out mode list by
 * check if basic color space/depth is supported
 * or not under this resolution;
 * return 0 when can found valid cs/cd configs, or return < 0;
 */
int hdmitx_common_check_valid_para_of_vic(struct hdmitx_common *tx_comm, enum hdmi_vic vic);
/* validate if hdmi_format_para can support, return 0 if can support or return < 0;
 * vic should already validate by hdmitx_common_validate_mode(), will not check if vic
 * support by rx. This function used to verify hdmi setting config from userspace;
 */
int hdmitx_common_validate_format_para(struct hdmitx_common *tx_comm,
	struct hdmi_format_para *para);

/* create hdmi_format_para from config and also calc setting from hw; */
int hdmitx_common_build_format_para(struct hdmitx_common *tx_comm,
		struct hdmi_format_para *para, enum hdmi_vic vic, u32 frac_rate_policy,
		enum hdmi_colorspace cs, enum hdmi_color_depth cd, enum hdmi_quantization_range cr);

/* For bootup init: init hdmi_format_para from hw configs.*/
int hdmitx_common_init_bootup_format_para(struct hdmitx_common *tx_comm,
		struct hdmi_format_para *para);

/*edid valid api*/
int hdmitx_edid_validate_format_para(struct tx_cap *hdmi_tx_cap,
		struct rx_cap *prxcap, struct hdmi_format_para *para);
bool hdmitx_edid_check_y420_support(struct rx_cap *prxcap,
	enum hdmi_vic vic);

bool hdmitx_edid_validate_mode(struct rx_cap *rxcap, u32 vic);
bool hdmitx_edid_only_support_sd(struct rx_cap *prxcap);

/* Attach platform related functions to hdmitx_common;
 * Currently hdmitx_tracer, hdmitx_uevent_mgr is platform related;
 */
enum HDMITX_PLATFORM_API_TYPE {
	HDMITX_PLATFORM_TRACER = 0,
	HDMITX_PLATFORM_UEVENT,
};

int hdmitx_common_attch_platform_data(struct hdmitx_common *tx_comm,
	enum HDMITX_PLATFORM_API_TYPE type, void *plt_data);

/*Notify hpd event to all outer modules: vpp by vout, drm, userspace
 *bool force_uevent: force send uevent even the hpd state NOT change.
 */
int hdmitx_common_notify_hpd_status(struct hdmitx_common *tx_comm, bool force_uevent);

/*packet api*/
/* mode = 0 , disable allm; mode 1: set allm; mode -1: */
int hdmitx_common_set_allm_mode(struct hdmitx_common *tx_comm, int mode);

/* avmute function with lock:
 * do set mute when mute cmd from any path;
 * do clear when all path have cleared avmute;
 */
#define AVMUTE_PATH_1 0x80 //mute by avmute sysfs node
#define AVMUTE_PATH_2 0x40 //mute by upstream side request re-auth
#define AVMUTE_PATH_DRM 0x20 //called by drm;
#define AVMUTE_PATH_HDMITX 0x10 //internal use

int hdmitx_common_avmute_locked(struct hdmitx_common *tx_comm,
		int mute_flag, int mute_path_hint);

/*edid tracer post-processing*/
int hdmitx_common_edid_tracer_post_proc(struct hdmitx_common *tx_comm, struct rx_cap *prxcap);
/*read edid raw data and parse edid to rxcap*/
int hdmitx_common_get_edid(struct hdmitx_common *tx_comm);

/*modesetting function*/
int hdmitx_common_do_mode_setting(struct hdmitx_common *tx_comm,
				  struct hdmitx_common_state *new,
				  struct hdmitx_common_state *old);
int hdmitx_common_validate_mode_locked(struct hdmitx_common *tx_comm,
				       struct hdmitx_common_state *new_state,
				       char *mode, char *attr, bool do_validate);
int hdmitx_common_disable_mode(struct hdmitx_common *tx_comm,
			       struct hdmitx_common_state *new_state);
int set_disp_mode(struct hdmitx_common *tx_comm, const char *mode);

/*packet api*/
int hdmitx_common_setup_vsif_packet(struct hdmitx_common *tx_comm,
	enum vsif_type type, int on, void *param);

unsigned int hdmitx_get_frame_duration(void);
/*******************************hdmitx common api end*******************************/

int hdmitx_register_hpd_cb(struct hdmitx_common *tx_comm, struct connector_hpd_cb *hpd_cb);
int hdmitx_fire_drm_hpd_cb_unlocked(struct hdmitx_common *tx_comm);
int hdmitx_audio_register_ctrl_callback(struct hdmitx_tracer *tracer,
						audio_en_callback cb1, audio_st_callback cb2);

int hdmitx_get_hpd_state(struct hdmitx_common *tx_comm);
unsigned char *hdmitx_get_raw_edid(struct hdmitx_common *tx_comm);
bool hdmitx_common_get_ready_state(struct hdmitx_common *tx_comm);
int hdmitx_setup_attr(struct hdmitx_common *tx_comm, const char *buf);
int hdmitx_get_attr(struct hdmitx_common *tx_comm, char attr[16]);

int hdmitx_get_hdrinfo(struct hdmitx_common *tx_comm, struct hdr_info *hdrinfo);

int hdmitx_set_hdr_priority(struct hdmitx_common *tx_comm, u32 hdr_priority);
int hdmitx_get_hdr_priority(struct hdmitx_common *tx_comm, u32 *hdr_priority);
void hdmitx_hdr_state_init(struct hdmitx_common *tx_comm);
bool hdmitx_hdr_en(struct hdmitx_hw_common *tx_hw);
bool hdmitx_dv_en(struct hdmitx_hw_common *tx_hw);
bool hdmitx_hdr10p_en(struct hdmitx_hw_common *tx_hw);

u32 hdmitx_get_frl_bandwidth(const enum frl_rate_enum rate);
u32 hdmitx_calc_frl_bandwidth(u32 pixel_freq, enum hdmi_colorspace cs,
	enum hdmi_color_depth cd);
u32 hdmitx_calc_tmds_clk(u32 pixel_freq,
	enum hdmi_colorspace cs, enum hdmi_color_depth cd);
enum frl_rate_enum hdmitx_select_frl_rate(u8 *dsc_en, u8 dsc_policy, enum hdmi_vic vic,
	enum hdmi_colorspace cs, enum hdmi_color_depth cd);

/*edid related function.*/
bool is_tv_changed(char *cur_edid_chksum, char *boot_param_edid_chksum);

/*debug functions*/
int hdmitx_load_edid_file(u32 type, char *path);
int hdmitx_save_edid_file(unsigned char *rawedid, char *path);

void hdmitx_vout_init(struct hdmitx_common *tx_comm, struct hdmitx_hw_common *tx_hw);
void hdmitx_vout_uninit(void);
struct vinfo_s *hdmitx_get_current_vinfo(void *data);
void update_vinfo_from_formatpara(struct hdmitx_common *tx_comm);
void edidinfo_detach_to_vinfo(struct hdmitx_common *tx_comm);
void edidinfo_attach_to_vinfo(struct hdmitx_common *tx_comm);
void hdrinfo_to_vinfo(struct hdr_info *hdrinfo, struct hdmitx_common *tx_comm);
void set_dummy_dv_info(struct vout_device_s *vdev);
void hdmitx_build_fmt_attr_str(struct hdmitx_common *tx_comm);

/* common work for plugin/resume, which is done in lock */
void hdmitx_plugin_common_work(struct hdmitx_common *tx_comm);
/* common work for plugout */
void hdmitx_plugout_common_work(struct hdmitx_common *tx_comm);
/* common edid clear, which is done in lock */
void hdmitx_common_edid_clear(struct hdmitx_common *tx_comm);
/* common work for late resume, which is done in lock */
void hdmitx_common_late_resume(struct hdmitx_common *tx_comm);
/* common disable hdmitx output api */
void hdmitx_common_output_disable(struct hdmitx_common *tx_comm,
	bool phy_dis, bool hdcp_reset, bool pkt_clear, bool edid_clear);
unsigned int hdmitx_get_frame_duration(void);

/*******************************drm hdmitx api*******************************/

unsigned int hdmitx_common_get_contenttypes(void);
int hdmitx_common_set_contenttype(int content_type);
const struct dv_info *hdmitx_common_get_dv_info(void);
const struct hdr_info *hdmitx_common_get_hdr_info(void);
int hdmitx_common_get_vic_list(int **vics);
bool hdmitx_common_chk_mode_attr_sup(char *mode, char *attr);
int hdmitx_common_get_timing_para(int vic, struct drm_hdmitx_timing_para *para);
void hdmitx_audio_notify_callback(struct hdmitx_common *tx_comm,
	struct hdmitx_hw_common *tx_hw_base,
	struct notifier_block *block,
	unsigned long cmd, void *para);
void get_hdmi_efuse(struct hdmitx_common *tx_comm);
enum hdmi_color_depth get_hdmi_colordepth(const struct vinfo_s *vinfo);
bool is_cur_hdmi_mode(void);
enum hdmi_vic hdmitx_get_prefer_vic(struct hdmitx_common *tx_comm, enum hdmi_vic vic);

#endif
