// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2021 Amlogic, Inc. All rights reserved.
 */

#include <drm/drm_modeset_helper.h>
#include <drm/drmP.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_atomic_helper.h>
#include <linux/component.h>
#include <vout/vout_serve/vout_func.h>
#include "meson_crtc.h"
#include "meson_dummyl.h"
#include "meson_vpu_pipeline.h"

static struct drm_display_mode dummy_mode = {
	.name = "dummy_l",
	.type = DRM_MODE_TYPE_USERDEF,
	.status = MODE_OK,
	.clock = 25000,
	.hdisplay = 720,
	.hsync_start = 736,
	.hsync_end = 798,
	.htotal = 858,
	.hskew = 0,
	.vdisplay = 480,
	.vsync_start = 489,
	.vsync_end = 495,
	.vtotal = 525,
	.vscan = 0,
	.flags =  DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC,
};

int meson_dummyl_get_modes(struct drm_connector *connector)
{
	struct drm_display_mode *mode = NULL;
	int count = 0;

	mode = drm_mode_duplicate(connector->dev, &dummy_mode);
	if (!mode) {
		DRM_INFO("[%s:%d]dup dummy mode failed.\n", __func__,
			 __LINE__);
	} else {
		drm_mode_probed_add(connector, mode);
		count++;
	}

	return count;
}

enum drm_mode_status meson_dummyl_check_mode(struct drm_connector *connector,
	struct drm_display_mode *mode)
{
	return MODE_OK;
}

static struct drm_encoder *meson_dummyl_best_encoder
	(struct drm_connector *connector)
{
	struct meson_dummyl *am_dummyl = connector_to_meson_dummyl(connector);

	return &am_dummyl->encoder;
}

static const struct drm_connector_helper_funcs meson_dummyl_helper_funcs = {
	.get_modes = meson_dummyl_get_modes,
	.mode_valid = meson_dummyl_check_mode,
	.best_encoder = meson_dummyl_best_encoder,
};

static enum drm_connector_status
meson_dummyl_detect(struct drm_connector *connector, bool force)
{
	return connector_status_connected;
}

static void am_dummyl_connector_destroy(struct drm_connector *connector)
{
	DRM_DEBUG("[%s]-[%d] called\n", __func__, __LINE__);

	drm_connector_unregister(connector);
	drm_connector_cleanup(connector);
}

static void am_dummyl_connector_reset(struct drm_connector *connector)
{
	DRM_DEBUG("[%s]-[%d] called\n", __func__, __LINE__);

	drm_atomic_helper_connector_reset(connector);
}

static struct drm_connector_state *
am_dummyl_connector_duplicate_state(struct drm_connector *connector)
{
	struct drm_connector_state *state;

	DRM_DEBUG("[%s]-[%d] called\n", __func__, __LINE__);

	state = drm_atomic_helper_connector_duplicate_state(connector);
	return state;
}

static void am_dummyl_connector_destroy_state(struct drm_connector *connector,
					   struct drm_connector_state *state)
{
	DRM_DEBUG("[%s]-[%d] called\n", __func__, __LINE__);

	drm_atomic_helper_connector_destroy_state(connector, state);
}

int meson_dummyl_atomic_set_property(struct drm_connector *connector,
			   struct drm_connector_state *state,
			   struct drm_property *property,
			   uint64_t val)
{
	return -EINVAL;
}

int meson_dummyl_atomic_get_property(struct drm_connector *connector,
			   const struct drm_connector_state *state,
			   struct drm_property *property,
			   uint64_t *val)
{
	return -EINVAL;
}

static const struct drm_connector_funcs meson_dummyl_funcs = {
	.detect			= meson_dummyl_detect,
	.fill_modes		= drm_helper_probe_single_connector_modes,
	.destroy		= am_dummyl_connector_destroy,
	.reset			= am_dummyl_connector_reset,
	.atomic_duplicate_state	= am_dummyl_connector_duplicate_state,
	.atomic_destroy_state	= am_dummyl_connector_destroy_state,
	.atomic_set_property	= meson_dummyl_atomic_set_property,
	.atomic_get_property	= meson_dummyl_atomic_get_property,
};

static void meson_dummyl_encoder_atomic_enable(struct drm_encoder *encoder,
	struct drm_atomic_state *state)
{
	struct am_meson_crtc_state *meson_crtc_state =
				to_am_meson_crtc_state(encoder->crtc->state);
	enum vmode_e vmode = meson_crtc_state->vmode;

	DRM_INFO("%s: vmode:%d skip atomic enable!\n", __func__, vmode);
}

static void meson_dummyl_encoder_atomic_disable(struct drm_encoder *encoder,
	struct drm_atomic_state *state)
{
	DRM_DEBUG("[%s]-[%d] called\n", __func__, __LINE__);
}

static int meson_dummyl_encoder_atomic_check(struct drm_encoder *encoder,
				       struct drm_crtc_state *crtc_state,
				struct drm_connector_state *conn_state)
{
	DRM_DEBUG("[%s]-[%d] called\n", __func__, __LINE__);
	return 0;
}

static const struct drm_encoder_helper_funcs meson_dummyl_encoder_helper_funcs = {
	.atomic_enable = meson_dummyl_encoder_atomic_enable,
	.atomic_disable = meson_dummyl_encoder_atomic_disable,
	.atomic_check = meson_dummyl_encoder_atomic_check,
};

static const struct drm_encoder_funcs meson_dummyl_encoder_funcs = {
	.destroy        = drm_encoder_cleanup,
};

int meson_dummyl_dev_bind(struct drm_device *drm,
	int type, struct meson_connector_dev *intf)
{
	struct meson_drm *priv = drm->dev_private;
	struct drm_connector *connector = NULL;
	struct drm_encoder *encoder = NULL;
	struct meson_dummyl *am_dummyl = NULL;
	char *connector_name = "MESON-DUMMYL";
	u32 i;
	int ret = 0;

	if (priv->dummyl_from_hdmitx) {
		DRM_INFO("[%s]-[%d] dummy from hdmitx, skip register dummyl connector.\n",
			__func__, __LINE__);
		return 0;
	}

	DRM_INFO("[%s]-[%d] called\n", __func__, __LINE__);
	am_dummyl = kzalloc(sizeof(*am_dummyl), GFP_KERNEL);
	if (!am_dummyl) {
		DRM_ERROR("[%s]: alloc drm_dummyl failed\n", __func__);
		return -ENOMEM;
	}

	am_dummyl->drm = drm;
	am_dummyl->base.drm_priv = priv;
	encoder = &am_dummyl->encoder;
	/* Encoder */
	for (i = 0; i < priv->pipeline->num_postblend; i++)
		encoder->possible_crtcs |= BIT(i);

	drm_encoder_helper_add(encoder, &meson_dummyl_encoder_helper_funcs);
	ret = drm_encoder_init(drm, encoder, &meson_dummyl_encoder_funcs,
			       DRM_MODE_ENCODER_VIRTUAL, "am_dummyl_encoder");
	if (ret) {
		DRM_ERROR("error:%s-%d: Failed to init lcd encoder\n",
			__func__, __LINE__);
		goto free_resource;
	}

	/* Connector */
	connector = &am_dummyl->base.connector;
	drm_connector_helper_add(connector, &meson_dummyl_helper_funcs);
	ret = drm_connector_init(drm, connector, &meson_dummyl_funcs,
				 DRM_MODE_CONNECTOR_VIRTUAL);
	if (ret) {
		DRM_ERROR("%s-%d: Failed to init lcd connector\n",
			__func__, __LINE__);
		goto free_resource;
	}

	/*update name to amlogic name*/
	if (connector_name) {
		kfree(connector->name);
		connector->name = kasprintf(GFP_KERNEL, "%s", connector_name);
		if (!connector->name)
			DRM_ERROR("[%s]: alloc name failed\n", __func__);
	}

	ret = drm_connector_attach_encoder(connector, encoder);
	if (ret != 0) {
		DRM_ERROR("%s-%d: attach failed.\n",
			__func__, __LINE__);
		goto free_resource;
	}

	return 0;

free_resource:
	kfree(am_dummyl);

	DRM_DEBUG("%s: %d Exit\n", __func__, ret);
	return ret;
}

int meson_dummyl_dev_unbind(struct drm_device *drm,
	int type, int connector_id)
{
	struct drm_connector *connector =
		drm_connector_lookup(drm, 0, connector_id);
	struct meson_dummyl *am_dummyl = 0;

	if (!connector)
		DRM_ERROR("%s got invalid connector id %d\n",
			__func__, connector_id);

	am_dummyl = connector_to_meson_dummyl(connector);
	if (!am_dummyl)
		return -EINVAL;

	kfree(am_dummyl);
	DRM_DEBUG("[%s]-[%d] called\n", __func__, __LINE__);
	return 0;
}

