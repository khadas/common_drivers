/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __HDMITX_DEV_COMMON_H
#define __HDMITX_DEV_COMMON_H

#include <linux/types.h>
#include <linux/mutex.h>
#include <drm/amlogic/meson_connector_dev.h>

struct hdmitx_common {
	u32 hdr_priority;
	char hdmichecksum[11];

	char fmt_attr[16];
	char backup_fmt_attr[16];

	/* 0.1% clock shift, 1080p60hz->59.94hz */
	u32 frac_rate_policy;
	u32 backup_frac_rate_policy;

	/*protect hotplug flow and related struct.*/
	struct mutex setclk_mutex;
	unsigned char hpd_state; /* 1, connect; 0, disconnect */

	/*DRM related*/
	struct connector_hpd_cb drm_hpd_cb;
//	struct connector_hdcp_cb drm_hdcp_cb;
};

int hdmitx_common_init(struct hdmitx_common *tx_common);
int hdmitx_common_destroy(struct hdmitx_common *tx_common);

int hdmitx_hpd_notify_unlocked(struct hdmitx_common *tx_comm);
int hdmitx_register_hpd_cb(struct hdmitx_common *tx_comm, struct connector_hpd_cb *hpd_cb);

int hdmitx_setup_attr(struct hdmitx_common *tx_comm, const char *buf);
int hdmitx_get_attr(struct hdmitx_common *tx_comm, char attr[16]);

#endif
