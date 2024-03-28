/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __MESON_CVBS_H_
#define __MESON_CVBS_H_

#include <drm/drm_connector.h>
#include <drm/drm_encoder.h>
#include <drm/drm_modes.h>

#include <linux/amlogic/media/vout/vinfo.h>
#include <drm/amlogic/meson_connector_dev.h>

#include "meson_drv.h"

struct am_drm_cvbs_s {
	struct meson_connector base;
	struct drm_encoder encoder;
	struct meson_hdmitx_dev *hdmitx_dev;
	struct drm_property *update_attr_prop;
};

struct am_cvbs_connector_state {
	struct drm_connector_state base;
	bool update : 1;
};

#define to_am_cvbs_connector_state(x)	container_of(x, struct am_cvbs_connector_state, base)

int meson_cvbs_dev_bind(struct drm_device *drm,
	int type, struct meson_connector_dev *intf);
int meson_cvbs_dev_unbind(struct drm_device *drm,
	int type, int connector_id);

#endif
