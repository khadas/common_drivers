/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __HDMITX_DEV_COMMON_H
#define __HDMITX_DEV_COMMON_H

#include <linux/types.h>

struct hdmitx_dev_common {
	u32 hdr_priority;
	char hdmichecksum[11];

	char fmt_attr[16];
	char backup_fmt_attr[16];

	/* 0.1% clock shift, 1080p60hz->59.94hz */
	u32 frac_rate_policy;
	u32 backup_frac_rate_policy;

//	unsigned char hpd_state; /* 1, connect; 0, disconnect */
	/*DRM related*/
//	int drm_hdmitx_id;
//	struct connector_hpd_cb drm_hpd_cb;
//	struct connector_hdcp_cb drm_hdcp_cb;
};

int hdmitx_common_init(struct hdmitx_dev_common *tx_common);

#endif
