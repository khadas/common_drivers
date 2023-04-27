/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2021 Amlogic, Inc. All rights reserved.
 */

#ifndef __MESON_DUMMYL_H
#define __MESON_DUMMYL_H

#include "meson_drv.h"
#include <drm/drm_encoder.h>
#include <drm/amlogic/meson_connector_dev.h>

struct meson_dummyl {
	struct meson_connector base;
	struct drm_encoder encoder;
	struct drm_device *drm;
};

#define connector_to_meson_dummyl(x) \
		container_of(connector_to_meson_connector(x), struct meson_dummyl, base)
#define encoder_to_meson_dummyl(x) container_of(x, struct meson_dummyl, encoder)

int meson_dummyl_dev_bind(struct drm_device *drm,
	int type, struct meson_connector_dev *intf);
int meson_dummyl_dev_unbind(struct drm_device *drm,
	int type, int connector_id);

#endif

