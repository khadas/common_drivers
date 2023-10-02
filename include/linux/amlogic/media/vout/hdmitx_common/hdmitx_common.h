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
};

struct hdmitx_ctrl_ops {
	int (*pre_enable_mode)(struct hdmitx_common *tx_comm, struct hdmi_format_para *para);
	int (*enable_mode)(struct hdmitx_common *tx_comm, struct hdmi_format_para *para);
	int (*post_enable_mode)(struct hdmitx_common *tx_comm, struct hdmi_format_para *para);
	int (*disable_mode)(struct hdmitx_common *tx_comm, struct hdmi_format_para *para);
	int (*init_uboot_mode)(enum vmode_e mode);
	void (*disable_hdcp)(void);
};

struct st_debug_param {
	unsigned int avmute_frame;
};

struct hdmitx_common {
	/* When hdr_priority is 1, then dv_info will be all 0;
	 * when hdr_priority is 2, then dv_info/hdr_info will be all 0
	 * App won't get real dv_cap/hdr_cap, but can get real dv_cap2/hdr_cap2
	 */
	u32 hdr_priority;

	char fmt_attr[16];

	/* 0.1% clock shift, 1080p60hz->59.94hz */
	u32 frac_rate_policy;

	/* enc index */
	u32 enc_idx;

	/*current format para.*/
	struct hdmi_format_para fmt_para;
	struct vinfo_s hdmitx_vinfo;

	/* allm_mode: 1/on 0/off */
	u32 allm_mode;
	/* contenttype:0/off 1/game, 2/graphics, 3/photo, 4/cinema */
	u32 ct_mode;

	/* 1, connect; 0, disconnect */
	unsigned char hpd_state;
	/* if HDMI plugin even once time, then set 1
	 * if never hdmi plugin, then keep as 0
	 * for android ott.
	 */
	u32 already_used;
	/* save the last plug out/in work done state */
	enum hdmi_event_t last_hpd_handle_done_stat;

	pf_callback earc_hdmitx_hpdst;
	/*edid related*/
	unsigned char EDID_buf[EDID_MAX_BLOCK * 128]; // TODO
	/* the checksum of current edid */
	/* indicate RX edid data integrated, HEAD valid and checksum pass */
	struct rx_cap rxcap;
	/* edid related end */
	spinlock_t edid_spinlock; /* edid hdr/dv cap lock */
	/* GPIO hpd/scl/sda members*/
	int hdmitx_gpios_hpd;
	int hdmitx_gpios_scl;
	int hdmitx_gpios_sda;

	/*DRM related*/
	struct connector_hpd_cb drm_hpd_cb;
//	struct connector_hdcp_cb drm_hdcp_cb;
	/*for color space conversion*/
	bool config_csc_en;
	bool suspend_flag;

	/*soc limitation*/
	u32 res_1080p;
	u32 max_refreshrate;

	struct hdmitx_hw_common *tx_hw;

	struct hdmitx_ctrl_ops *ctrl_ops;

	struct vout_device_s *vdev;

	/* Normally, after the HPD in or late resume, there will reading EDID, and
	 * notify application to select a hdmi mode output. But during the mode
	 * setting moment, there may be HPD out. It will clear the edid data, ..., etc.
	 * To avoid such case, here adds the hdmimode_mutex to let the HPD in, HPD out
	 * handler and mode setting sequentially.
	 */
	struct mutex hdmimode_mutex;

	u32 repeater_mode;
	/*hdcp control type config*/
	u32 hdcp_ctl_lvl;
	u8 hdcp_mode;

	/*debug & log*/
	struct hdmitx_tracer *tx_tracer;
	struct hdmitx_event_mgr *event_mgr;

	struct st_debug_param debug_param;

	/* ced/rxsense related */
	bool cedst_en;
	u32 cedst_policy;
	struct workqueue_struct *cedst_wq;
	struct delayed_work work_cedst;
	u32 rxsense_policy;
	struct workqueue_struct *rxsense_wq;
	struct delayed_work work_rxsense;
	/* 1: TV product, 0: stb/soundbar product */
	u32 tv_usage;

	/* indicate hdmitx output ready, sw/hw mode setting done */
	bool ready;
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

/* create hdmi_format_para from config and also calc setting from hw;*/
int hdmitx_common_build_format_para(struct hdmitx_common *tx_comm,
		struct hdmi_format_para *para, enum hdmi_vic vic, u32 frac_rate_policy,
		enum hdmi_colorspace cs, enum hdmi_color_depth cd, enum hdmi_quantization_range cr);

/* For bootup init: init hdmi_format_para from hw configs.*/
int hdmitx_common_init_bootup_format_para(struct hdmitx_common *tx_comm,
		struct hdmi_format_para *para);

/* Attach platform related functions to hdmitx_common;
 * Currently hdmitx_tracer, hdmitx_uevent_mgr is platform related;
 */
enum HDMITX_PLATFORM_API_TYPE {
	HDMITX_PLATFORM_TRACER = 0,
	HDMITX_PLATFORM_UEVENT,
};

int hdmitx_common_attch_platform_data(struct hdmitx_common *tx_comm,
	enum HDMITX_PLATFORM_API_TYPE type, void *plt_data);

int hdmitx_common_trace_event(struct hdmitx_common *tx_comm,
	enum hdmitx_event_log_bits event);

/*Notify hpd event to all outer modules: vpp by vout, drm, userspace*/
int hdmitx_common_notify_hpd_status(struct hdmitx_common *tx_comm);

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
/* For different SOC, different output limited capabilities */
bool soc_resolution_limited(const struct hdmi_timing *timing, u32 res_v);
bool soc_freshrate_limited(const struct hdmi_timing *timing, u32 vsync);

/*******************************hdmitx common api end*******************************/

int hdmitx_hpd_notify_unlocked(struct hdmitx_common *tx_comm);
int hdmitx_register_hpd_cb(struct hdmitx_common *tx_comm, struct connector_hpd_cb *hpd_cb);

int hdmitx_get_hpd_state(struct hdmitx_common *tx_comm);
unsigned char *hdmitx_get_raw_edid(struct hdmitx_common *tx_comm);
int hdmitx_setup_attr(struct hdmitx_common *tx_comm, const char *buf);
int hdmitx_get_attr(struct hdmitx_common *tx_comm, char attr[16]);

int hdmitx_get_hdrinfo(struct hdmitx_common *tx_comm, struct hdr_info *hdrinfo);

/*edid related function.*/
void hdmitx_get_edid(struct hdmitx_common *tx_comm, struct hdmitx_hw_common *tx_hw_base);
bool is_tv_changed(char *cur_edid_chksum, char *boot_param_edid_chksum);

/*debug functions*/
int hdmitx_load_edid_file(char *path);
int hdmitx_save_edid_file(unsigned char *rawedid, char *path);
int hdmitx_print_sink_cap(struct hdmitx_common *tx_comm, char *buffer, int buffer_len);

/*modesetting function*/
int hdmitx_common_do_mode_setting(struct hdmitx_common *tx_comm,
				  struct hdmitx_common_state *new,
				  struct hdmitx_common_state *old);
int hdmitx_common_disable_mode(struct hdmitx_common *tx_comm,
			       struct hdmitx_common_state *new_state);
void hdmitx_vout_init(struct hdmitx_common *tx_comm, struct hdmitx_hw_common *tx_hw);
void hdmitx_vout_uninit(void);
enum vmode_e hdmitx_validate_vmode(char *mode, unsigned int frac, void *data);
struct vinfo_s *hdmitx_get_current_vinfo(void *data);
void update_vinfo_from_formatpara(struct hdmitx_common *tx_comm);
void edidinfo_detach_to_vinfo(struct hdmitx_common *tx_comm);
void edidinfo_attach_to_vinfo(struct hdmitx_common *tx_comm);
void hdrinfo_to_vinfo(struct hdr_info *hdrinfo, struct hdmitx_common *tx_comm);
void set_dummy_dv_info(struct vout_device_s *vdev);
void hdmitx_build_fmt_attr_str(struct hdmitx_common *tx_comm);

/*packet api*/
int hdmitx_common_setup_vsif_packet(struct hdmitx_common *tx_comm,
	enum vsif_type type, int on, void *param);

/* common work for plugin/resume, witch is done in lock */
void hdmitx_plugin_common_work(struct hdmitx_common *tx_comm,
	struct hdmitx_hw_common *tx_hw_base);
/* common work for plugout/suspend, witch is done in lock */
void hdmitx_plugout_common_work(struct hdmitx_common *tx_comm);

void hdmitx_common_pre_early_suspend(struct hdmitx_common *tx_comm,
	struct hdmitx_hw_common *tx_hw_base);
void hdmitx_common_late_resume(struct hdmitx_common *tx_comm,
	struct hdmitx_hw_common *tx_hw_base);

/*******************************drm hdmitx api*******************************/

unsigned int hdmitx_common_get_contenttypes(void);
int hdmitx_common_set_contenttype(int content_type);
const struct dv_info *hdmitx_common_get_dv_info(void);
const struct hdr_info *hdmitx_common_get_hdr_info(void);
int hdmitx_common_get_vic_list(int **vics);
bool hdmitx_common_chk_mode_attr_sup(char *mode, char *attr);
int hdmitx_common_get_timing_para(int vic, struct drm_hdmitx_timing_para *para);

#endif
