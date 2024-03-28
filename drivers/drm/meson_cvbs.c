// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <drm/drm_modeset_helper.h>
#include <drm/drmP.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_connector.h>

#include <linux/component.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/amlogic/media/vout/vout_notify.h>
#include <linux/amlogic/media/vout/vinfo.h>
#include <vout/vout_serve/vout_func.h>
#include <vout/cvbs/cvbs_out.h>

#include "meson_crtc.h"
#include "meson_cvbs.h"

static struct drm_display_mode cvbs_mode[] = {
	{ /* MODE_480CVBS */
		.name = "480cvbs",
		.status = 0,
		.clock = 13500,
		.hdisplay = 720,
		.hsync_start = 739,
		.hsync_end = 801,
		.htotal = 858,
		.hskew = 0,
		.vdisplay = 480,
		.vsync_start = 488,
		.vsync_end = 494,
		.vtotal = 525,
		.vscan = 0,
		.flags = DRM_MODE_FLAG_INTERLACE,
	},
	{ /* MODE_576CVBS*/
		.name = "576cvbs",
		.status = 0,
		.clock = 13500,
		.hdisplay = 720,
		.hsync_start = 732,
		.hsync_end = 795,
		.htotal = 864,
		.hskew = 0,
		.vdisplay = 576,
		.vsync_start = 580,
		.vsync_end = 586,
		.vtotal = 625,
		.vscan = 0,
		.flags = DRM_MODE_FLAG_INTERLACE,
	},
	{ /* MODE_PAL_M */
		.name = "pal_m",
		.status = 0,
		.clock = 13500,
		.hdisplay = 720,
		.hsync_start = 732,
		.hsync_end = 801,
		.htotal = 858,
		.hskew = 0,
		.vdisplay = 480,
		.vsync_start = 488,
		.vsync_end = 494,
		.vtotal = 525,
		.vscan = 0,
		.flags = DRM_MODE_FLAG_INTERLACE,
	},
	{ /* MODE_PAL_N*/
		.name = "pal_n",
		.status = 0,
		.clock = 13500,
		.hdisplay = 720,
		.hsync_start = 732,
		.hsync_end = 796,
		.htotal = 864,
		.hskew = 0,
		.vdisplay = 576,
		.vsync_start = 580,
		.vsync_end = 586,
		.vtotal = 625,
		.vscan = 0,
		.flags = DRM_MODE_FLAG_INTERLACE,
	},
	{ /* MODE_NTSC_M */
		.name = "ntsc_m",
		.status = 0,
		.clock = 13500,
		.hdisplay = 720,
		.hsync_start = 736,
		.hsync_end = 800,
		.htotal = 858,
		.hskew = 0,
		.vdisplay = 480,
		.vsync_start = 488,
		.vsync_end = 494,
		.vtotal = 525,
		.vscan = 0,
		.flags = DRM_MODE_FLAG_INTERLACE,
	},
};

static struct am_drm_cvbs_s *am_drm_cvbs;

static inline struct am_drm_cvbs_s *con_to_cvbs(struct drm_connector *con)
{
	return container_of(connector_to_meson_connector(con), struct am_drm_cvbs_s, base);
}

static inline struct am_drm_cvbs_s *encoder_to_cvbs(struct drm_encoder *encoder)
{
	return container_of(encoder, struct am_drm_cvbs_s, encoder);
}

int am_cvbs_tx_get_modes(struct drm_connector *connector)
{
	int i, count;
	struct drm_display_mode *mode;

	count = 0;
	for (i = 0; i < ARRAY_SIZE(cvbs_mode); i++) {
		mode = drm_mode_duplicate(connector->dev, &cvbs_mode[i]);
		if (!mode) {
			DRM_INFO("[%s:%d]duplicate failed\n", __func__,
				 __LINE__);
			continue;
		}

		drm_mode_probed_add(connector, mode);
		count++;
	}

	return count;
}

enum drm_mode_status am_cvbs_tx_check_mode(struct drm_connector *connector,
					   struct drm_display_mode *mode)
{
	int i;
	int checked_vrefresh = drm_mode_vrefresh(mode);

	for (i = 0; i < ARRAY_SIZE(cvbs_mode); i++) {
		if (cvbs_mode[i].hdisplay == mode->hdisplay &&
		    cvbs_mode[i].vdisplay == mode->vdisplay &&
		    (drm_mode_vrefresh(&cvbs_mode[i]) == checked_vrefresh))
			return MODE_OK;
	}

	return MODE_NOMODE;
}

static struct drm_encoder *am_cvbs_connector_best_encoder
	(struct drm_connector *connector)
{
	struct am_drm_cvbs_s *am_drm_cvbs = con_to_cvbs(connector);

	return &am_drm_cvbs->encoder;
}

int am_cvbs_tx_atomic_check(struct drm_connector *connector,
	struct drm_atomic_state *state)
{
	struct drm_crtc_state *new_crtc_state = NULL;
	struct am_cvbs_connector_state *cvbs_state = NULL;

	cvbs_state = to_am_cvbs_connector_state
		(drm_atomic_get_new_connector_state(state, connector));

	if (cvbs_state->base.crtc)
		new_crtc_state = drm_atomic_get_new_crtc_state(state,
			cvbs_state->base.crtc);
	else
		return 0;

	/*force set mode.*/
	if (cvbs_state->update)
		new_crtc_state->connectors_changed = true;

	return 0;
}

static const
struct drm_connector_helper_funcs am_cvbs_connector_helper_funcs = {
	.get_modes = am_cvbs_tx_get_modes,
	.mode_valid = am_cvbs_tx_check_mode,
	.best_encoder = am_cvbs_connector_best_encoder,
	.atomic_check	= am_cvbs_tx_atomic_check,
};

static enum drm_connector_status am_cvbs_connector_detect
	(struct drm_connector *connector, bool force)
{
	return connector_status_connected;
}

static void am_cvbs_connector_destroy(struct drm_connector *connector)
{
	drm_connector_unregister(connector);
	drm_connector_cleanup(connector);
}

static int am_cvbs_connector_atomic_set_property
	(struct drm_connector *connector,
	struct drm_connector_state *state,
	struct drm_property *property, uint64_t val)
{
	struct am_cvbs_connector_state *cvbs_state =
		to_am_cvbs_connector_state(state);

	if (property == am_drm_cvbs->update_attr_prop) {
		cvbs_state->update = true;
		return 0;
	}

	return -EINVAL;
}

static int am_cvbs_connector_atomic_get_property
	(struct drm_connector *connector,
	const struct drm_connector_state *state,
	struct drm_property *property, uint64_t *val)
{
	if (property == am_drm_cvbs->update_attr_prop) {
		*val = 0;
		return 0;
	}

	return -EINVAL;
}

struct drm_connector_state *am_cvbs_atomic_duplicate_state
	(struct drm_connector *connector)
{
	struct am_cvbs_connector_state *new_state;

	new_state = kzalloc(sizeof(*new_state), GFP_KERNEL);
	if (!new_state)
		return NULL;

	__drm_atomic_helper_connector_duplicate_state(connector,
		&new_state->base);

	new_state->update = false;
	return &new_state->base;
}

void am_cvbs_atomic_destroy_state(struct drm_connector *connector,
	 struct drm_connector_state *state)
{
	struct am_cvbs_connector_state *cvbs_state;

	cvbs_state = to_am_cvbs_connector_state(state);
	__drm_atomic_helper_connector_destroy_state(&cvbs_state->base);
	kfree(cvbs_state);
}

static const struct drm_connector_funcs am_cvbs_connector_funcs = {
	.detect			= am_cvbs_connector_detect,
	.fill_modes		= drm_helper_probe_single_connector_modes,
	.destroy		= am_cvbs_connector_destroy,
	.reset			= drm_atomic_helper_connector_reset,
	.atomic_duplicate_state	= am_cvbs_atomic_duplicate_state,
	.atomic_destroy_state	= am_cvbs_atomic_destroy_state,
	.atomic_set_property	= am_cvbs_connector_atomic_set_property,
	.atomic_get_property	= am_cvbs_connector_atomic_get_property,
};

void am_cvbs_encoder_mode_set(struct drm_encoder *encoder,
	struct drm_crtc_state *crtc_state,
	struct drm_connector_state *conn_state)
{
}

void am_cvbs_encoder_enable(struct drm_encoder *encoder,
	struct drm_atomic_state *state)
{
	struct am_meson_crtc_state *meson_crtc_state = to_am_meson_crtc_state(encoder->crtc->state);
	struct am_meson_crtc *amcrtc = to_am_meson_crtc(encoder->crtc);
	struct drm_display_mode *mode = &encoder->crtc->state->adjusted_mode;
	enum vmode_e vmode = meson_crtc_state->vmode;

	if ((vmode & VMODE_MODE_BIT_MASK) != VMODE_CVBS)
		return;

	if (meson_crtc_state->uboot_mode_init == 1)
		vmode |= VMODE_INIT_BIT_MASK;

	meson_vout_notify_mode_change(amcrtc->vout_index,
		vmode, EVENT_MODE_SET_START);
	cvbs_set_current_vmode(vmode, NULL);
	meson_vout_notify_mode_change(amcrtc->vout_index,
		vmode, EVENT_MODE_SET_FINISH);
	meson_vout_update_mode_name(amcrtc->vout_index, mode->name, "cvbs");
}

void am_cvbs_encoder_disable(struct drm_encoder *encoder,
	struct drm_atomic_state *state)
{
}

static const struct drm_encoder_helper_funcs am_cvbs_encoder_helper_funcs = {
	.atomic_mode_set = am_cvbs_encoder_mode_set,
	.atomic_enable = am_cvbs_encoder_enable,
	.atomic_disable = am_cvbs_encoder_disable,
};

static const struct drm_encoder_funcs am_cvbs_encoder_funcs = {
	.destroy        = drm_encoder_cleanup,
};

static void meson_cvbs_init_update_property(struct drm_device *drm_dev,
						  struct drm_connector *connector)
{
	struct drm_property *prop;

	prop = drm_property_create_bool(drm_dev, 0, "UPDATE");
	if (prop) {
		am_drm_cvbs->update_attr_prop = prop;
		drm_object_attach_property(&connector->base, prop, 0);
	} else {
		DRM_ERROR("Failed to UPDATE property\n");
	}
}

int meson_cvbs_dev_bind(struct drm_device *drm,
	int type, struct meson_connector_dev *intf)
{
	struct meson_drm *priv = drm->dev_private;
	struct drm_encoder *encoder;
	struct drm_connector *connector;
	int ret = 0;

	DRM_DEBUG("%s in[%d]\n", __func__, __LINE__);

	am_drm_cvbs = kzalloc(sizeof(*am_drm_cvbs), GFP_KERNEL);
	if (!am_drm_cvbs) {
		DRM_ERROR("[%s,%d] kzalloc failed\n", __func__, __LINE__);
		return -ENOMEM;
	}

	encoder = &am_drm_cvbs->encoder;
	connector = &am_drm_cvbs->base.connector;

	/* Encoder */
	encoder->possible_crtcs = priv->of_conf.crtc_masks[ENCODER_CVBS];
	drm_encoder_helper_add(encoder, &am_cvbs_encoder_helper_funcs);

	ret = drm_encoder_init(drm, encoder, &am_cvbs_encoder_funcs,
			       DRM_MODE_ENCODER_TVDAC, "am_cvbs_encoder");
	if (ret) {
		DRM_ERROR("Failed to init cvbs encoder\n");
		goto cvbs_err;
	}

	/* Connector */
	drm_connector_helper_add(connector,
				 &am_cvbs_connector_helper_funcs);

	ret = drm_connector_init(drm, connector, &am_cvbs_connector_funcs,
				 DRM_MODE_CONNECTOR_TV);
	if (ret) {
		DRM_ERROR("Failed to init cvbs OUT connector\n");
		goto cvbs_err;
	}

	connector->interlace_allowed = 1;

	ret = drm_connector_attach_encoder(connector, encoder);
	if (ret) {
		DRM_ERROR("Failed to init cvbs attach\n");
		goto cvbs_err;
	}

	meson_cvbs_init_update_property(drm, connector);

	DRM_DEBUG("%s out[%d]\n", __func__, __LINE__);
	return ret;

cvbs_err:
	kfree(am_drm_cvbs);

	return ret;
}

int meson_cvbs_dev_unbind(struct drm_device *drm,
	int type, int connector_id)
{
	am_drm_cvbs->base.connector.funcs->destroy(&am_drm_cvbs->base.connector);
	am_drm_cvbs->encoder.funcs->destroy(&am_drm_cvbs->encoder);
	kfree(am_drm_cvbs);
	return 0;
}
