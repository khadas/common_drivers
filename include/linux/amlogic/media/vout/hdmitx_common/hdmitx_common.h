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
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_edid.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_types.h>

#define HDMI_INFOFRAME_TYPE_VENDOR2 (0x81 | 0x100)

struct hdmitx_common {
	/* When hdr_priority is 1, then dv_info will be all 0;
	 * when hdr_priority is 2, then dv_info/hdr_info will be all 0
	 * App won't get real dv_cap/hdr_cap, but can get real dv_cap2/hdr_cap2
	 */
	u32 hdr_priority;

	char hdmichecksum[11];

	char fmt_attr[16];
	char backup_fmt_attr[16];

	/* 0.1% clock shift, 1080p60hz->59.94hz */
	u32 frac_rate_policy;
	u32 backup_frac_rate_policy;

	/*current mode vic.*/
	u32 cur_VIC;
	/*current format para.*/
	struct hdmi_format_para fmt_para;
	struct vinfo_s hdmitx_vinfo;

	/* allm_mode: 1/on 0/off */
	u32 allm_mode;
	/* contenttype:0/off 1/game, 2/graphics, 3/photo, 4/cinema */
	u32 ct_mode;

	/*protect hotplug flow and related struct.*/
	struct mutex setclk_mutex;
	/* 1, connect; 0, disconnect */
	unsigned char hpd_state;

	/*edid related*/
	unsigned char *edid_ptr;
	unsigned char EDID_buf[EDID_MAX_BLOCK * 128];
	unsigned char EDID_buf1[EDID_MAX_BLOCK * 128]; /* for second read */
	unsigned char tmp_edid_buf[128 * EDID_MAX_BLOCK];
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

	struct hdmitx_base_state *states[HDMITX_MAX_MODULE];
	struct hdmitx_base_state *old_states[HDMITX_MAX_MODULE];
};

struct hdmitx_base_state *hdmitx_get_mod_state(struct hdmitx_common *tx_common,
					       enum HDMITX_MODULE type);
struct hdmitx_base_state *hdmitx_get_old_mod_state(struct hdmitx_common *tx_common,
			enum HDMITX_MODULE type);
void hdmitx_get_init_state(struct hdmitx_common *tx_common,
					struct hdmitx_binding_state *state);

int hdmitx_common_init(struct hdmitx_common *tx_common);
int hdmitx_common_destroy(struct hdmitx_common *tx_common);

int hdmitx_hpd_notify_unlocked(struct hdmitx_common *tx_comm);
int hdmitx_register_hpd_cb(struct hdmitx_common *tx_comm, struct connector_hpd_cb *hpd_cb);

unsigned char *hdmitx_get_raw_edid(struct hdmitx_common *tx_comm);
int hdmitx_setup_attr(struct hdmitx_common *tx_comm, const char *buf);
int hdmitx_get_attr(struct hdmitx_common *tx_comm, char attr[16]);

int hdmitx_get_hdrinfo(struct hdmitx_common *tx_comm, struct hdr_info *hdrinfo);

/*edid related function.*/
int hdmitx_update_edid_chksum(u8 *buf, u32 block_cnt, struct rx_cap *rxcap);

/*debug functions*/
int hdmitx_load_edid_file(char *path);
int hdmitx_save_edid_file(unsigned char *rawedid, char *path);
int hdmitx_print_sink_cap(struct hdmitx_common *tx_comm, char *buffer, int buffer_len);

#endif
