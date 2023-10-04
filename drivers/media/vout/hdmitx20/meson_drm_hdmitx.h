/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2021 Amlogic, Inc. All rights reserved.
 */

#ifndef _MESON_DRM_HDMITX_H
#define _MESON_DRM_HDMITX_H
#include "hdmi_tx_module.h"

void drm_hdmitx_hdcp22_init(void);

unsigned int meson_hdcp_get_tx_cap(void);
unsigned int meson_hdcp_get_rx_cap(void);

void drm_hdmitx_enable_hdcp_mode(unsigned int content_type);
void drm_hdmitx_disable_hdcp_mode(unsigned int content_type);

#endif
