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

#define HDMI_PACKET_TYPE_GCP 0x3
#define HDMI_INFOFRAME_TYPE_VENDOR2 (0x81 | 0x100)

struct hdmitx_ctrl_ops {
	int (*pre_enable_mode)(struct hdmitx_common *tx_comm, struct hdmi_format_para *para);
	int (*enable_mode)(struct hdmitx_common *tx_comm, struct hdmi_format_para *para);
	int (*post_enable_mode)(struct hdmitx_common *tx_comm, struct hdmi_format_para *para);
	int (*disable_mode)(struct hdmitx_common *tx_comm, struct hdmi_format_para *para);
};

struct hdmitx_common {
	/* When hdr_priority is 1, then dv_info will be all 0;
	 * when hdr_priority is 2, then dv_info/hdr_info will be all 0
	 * App won't get real dv_cap/hdr_cap, but can get real dv_cap2/hdr_cap2
	 */
	u32 hdr_priority;

	char hdmichecksum[11];

	char fmt_attr[16];

	/* 0.1% clock shift, 1080p60hz->59.94hz */
	u32 frac_rate_policy;

	/*current mode vic.*/
	u32 cur_VIC;
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

	/*edid related*/
	unsigned char *edid_ptr;
	unsigned char EDID_buf[EDID_MAX_BLOCK * 128]; // TODO
	unsigned char EDID_hash[20];
	/* indicate RX edid data integrated, HEAD valid and checksum pass */
	unsigned int edid_parsing;
	struct rx_cap rxcap;
	/*edid related end*/

	/*DRM related*/
	struct connector_hpd_cb drm_hpd_cb;
//	struct connector_hdcp_cb drm_hdcp_cb;
	/*for color space conversion*/
	bool config_csc_en;
	bool suspend_flag;

	struct hdmitx_base_state *states[HDMITX_MAX_MODULE];
	struct hdmitx_base_state *old_states[HDMITX_MAX_MODULE];

	/*soc limitation*/
	u32 res_1080p;
	u32 max_refreshrate;

	struct hdmitx_hw_common *tx_hw;

	struct hdmitx_ctrl_ops *ctrl_ops;

	/*protect set mode flow*/
	/*protect hotplug flow and related struct.*/
	struct mutex setclk_mutex;
	/*
	 * Normally, after the HPD in or late resume, there will reading EDID, and
	 * notify application to select a hdmi mode output. But during the mode
	 * setting moment, there may be HPD out. It will clear the edid data, ..., etc.
	 * To avoid such case, here adds the hdmimode_mutex to let the HPD in, HPD out
	 * handler and mode setting sequentially.
	 */
	struct mutex hdmimode_mutex;

	u32 repeater_mode;

	/*debug & log*/
	struct hdmitx_tracer *tx_tracer;
	struct hdmitx_event_mgr *event_mgr;
};

struct hdmitx_base_state *hdmitx_get_mod_state(struct hdmitx_common *tx_common,
					       enum HDMITX_MODULE type);
struct hdmitx_base_state *hdmitx_get_old_mod_state(struct hdmitx_common *tx_common,
			enum HDMITX_MODULE type);
void hdmitx_get_init_state(struct hdmitx_common *tx_common,
					struct hdmitx_binding_state *state);

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
/*******************************hdmitx common api end*******************************/

int hdmitx_hpd_notify_unlocked(struct hdmitx_common *tx_comm);
int hdmitx_register_hpd_cb(struct hdmitx_common *tx_comm, struct connector_hpd_cb *hpd_cb);

unsigned char *hdmitx_get_raw_edid(struct hdmitx_common *tx_comm);
int hdmitx_setup_attr(struct hdmitx_common *tx_comm, const char *buf);
int hdmitx_get_attr(struct hdmitx_common *tx_comm, char attr[16]);

int hdmitx_get_hdrinfo(struct hdmitx_common *tx_comm, struct hdr_info *hdrinfo);

/*edid related function.*/
int hdmitx_update_edid_chksum(u8 *buf, u32 block_cnt, struct rx_cap *rxcap);
int hdmitx_edid_parse(struct hdmitx_common *tx_comm); // TODO
void hdmitx_edid_print(struct hdmitx_common *tx_comm); // TODO, remove compare
void hdmitx_edid_buffer_clear(struct hdmitx_common *tx_comm);
void hdmitx_edid_rxcap_clear(struct hdmitx_common *tx_comm);
bool hdmitx_edid_only_support_sd(struct rx_cap *prxcap);
bool hdmitx_validate_y420_vic(enum hdmi_vic vic);

/*debug functions*/
int hdmitx_load_edid_file(char *path);
int hdmitx_save_edid_file(unsigned char *rawedid, char *path);
int hdmitx_print_sink_cap(struct hdmitx_common *tx_comm, char *buffer, int buffer_len);

/*modesetting function*/
int hdmitx_common_do_mode_setting(struct hdmitx_common *tx_comm, struct hdmitx_binding_state *new);

/*packet api*/
int hdmitx_common_setup_vsif_packet(struct hdmitx_common *tx_comm,
	enum vsif_type type, int on, void *param);

#endif
