/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __MESON_DUMMYP_H
#define __MESON_DUMMYP_H

#include "meson_drv.h"
#include <drm/drm_encoder.h>
#include <drm/amlogic/meson_connector_dev.h>

struct meson_dummyp {
	struct meson_connector base;
	struct drm_encoder encoder;
	struct drm_device *drm;
};

#define connector_to_meson_dummyp(x) \
		container_of(connector_to_meson_connector(x), struct meson_dummyp, base)
#define encoder_to_meson_dummyp(x) container_of(x, struct meson_dummyp, encoder)

int meson_dummyp_dev_bind(struct drm_device *drm,
	int type, struct meson_connector_dev *intf);
int meson_dummyp_dev_unbind(struct drm_device *drm,
	int type, int connector_id);

#endif

