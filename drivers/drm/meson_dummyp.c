// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <drm/drm_modeset_helper.h>
#include <drm/drmP.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_atomic_helper.h>
#include <linux/component.h>
#include <linux/amlogic/gki_module.h>
#include <vout/vout_serve/vout_func.h>
#include "meson_crtc.h"
#include "meson_dummyp.h"

static struct drm_display_mode dummy_mode = {
	.name = "dummy_panel",
	.status = MODE_OK,
	.clock = 148500,
	.hdisplay = 1920,
	.hsync_start = 2008,
	.hsync_end = 2052,
	.htotal = 2200,
	.hskew = 0,
	.vdisplay = 1080,
	.vsync_start = 1090,
	.vsync_end = 1095,
	.vtotal = 1125,
	.vscan = 0,
};

int meson_dummyp_get_modes(struct drm_connector *connector)
{
	struct drm_display_mode *mode = NULL;
	struct vinfo_s *vinfo = NULL;
	u32 dummyp_timing_flip;
	int count = 0;
	int tmp = 0;

	vinfo = get_current_vinfo2();
	dummyp_timing_flip = get_dummyp_timing_flip();

	if (dummyp_timing_flip) {
		dummy_mode.clock  = vinfo->video_clk / 1000;

		dummy_mode.hdisplay  = vinfo->height;
		tmp = vinfo->vtotal - vinfo->height - vinfo->vbp;
		dummy_mode.hsync_start  = vinfo->height + tmp - vinfo->vsw;
		dummy_mode.hsync_end = vinfo->height + tmp;
		dummy_mode.htotal  = vinfo->vtotal;

		dummy_mode.vdisplay = vinfo->width;
		tmp = vinfo->htotal - vinfo->width - vinfo->hbp;
		dummy_mode.vsync_start  =  vinfo->width + tmp - vinfo->hsw;
		dummy_mode.vsync_end = vinfo->width + tmp;
		dummy_mode.vtotal = vinfo->htotal;
	} else {
		dummy_mode.clock = vinfo->video_clk / 1000;

		dummy_mode.hdisplay = vinfo->width;
		tmp = vinfo->htotal - vinfo->width - vinfo->hbp;
		dummy_mode.hsync_start = vinfo->width + tmp - vinfo->hsw;
		dummy_mode.hsync_end = vinfo->width + tmp;
		dummy_mode.htotal = vinfo->htotal;

		dummy_mode.vdisplay = vinfo->height;
		tmp = vinfo->vtotal - vinfo->height - vinfo->vbp;
		dummy_mode.vsync_start = vinfo->height + tmp - vinfo->vsw;
		dummy_mode.vsync_end = vinfo->height + tmp;
		dummy_mode.vtotal = vinfo->vtotal;
	}
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

enum drm_mode_status meson_dummyp_check_mode(struct drm_connector *connector,
	struct drm_display_mode *mode)
{
	return MODE_OK;
}

static struct drm_encoder *meson_dummyp_best_encoder
	(struct drm_connector *connector)
{
	struct meson_dummyp *am_dummyp = connector_to_meson_dummyp(connector);

	return &am_dummyp->encoder;
}

static const struct drm_connector_helper_funcs meson_dummyp_helper_funcs = {
	.get_modes = meson_dummyp_get_modes,
	.mode_valid = meson_dummyp_check_mode,
	.best_encoder = meson_dummyp_best_encoder,
};

static enum drm_connector_status
meson_dummyp_detect(struct drm_connector *connector, bool force)
{
	return connector_status_connected;
}

static void am_dummyp_connector_destroy(struct drm_connector *connector)
{
	DRM_DEBUG("[%s]-[%d] called\n", __func__, __LINE__);

	drm_connector_unregister(connector);
	drm_connector_cleanup(connector);
}

static void am_dummyp_connector_reset(struct drm_connector *connector)
{
	DRM_DEBUG("[%s]-[%d] called\n", __func__, __LINE__);

	drm_atomic_helper_connector_reset(connector);
}

static struct drm_connector_state *
am_dummyp_connector_duplicate_state(struct drm_connector *connector)
{
	struct drm_connector_state *state;

	DRM_DEBUG("[%s]-[%d] called\n", __func__, __LINE__);

	state = drm_atomic_helper_connector_duplicate_state(connector);
	return state;
}

static void am_dummyp_connector_destroy_state(struct drm_connector *connector,
					   struct drm_connector_state *state)
{
	DRM_DEBUG("[%s]-[%d] called\n", __func__, __LINE__);

	drm_atomic_helper_connector_destroy_state(connector, state);
}

int meson_dummyp_atomic_set_property(struct drm_connector *connector,
			   struct drm_connector_state *state,
			   struct drm_property *property,
			   uint64_t val)
{
	return -EINVAL;
}

int meson_dummyp_atomic_get_property(struct drm_connector *connector,
			   const struct drm_connector_state *state,
			   struct drm_property *property,
			   uint64_t *val)
{
	return -EINVAL;
}

static const struct drm_connector_funcs meson_dummyp_funcs = {
	.detect			= meson_dummyp_detect,
	.fill_modes		= drm_helper_probe_single_connector_modes,
	.destroy		= am_dummyp_connector_destroy,
	.reset			= am_dummyp_connector_reset,
	.atomic_duplicate_state	= am_dummyp_connector_duplicate_state,
	.atomic_destroy_state	= am_dummyp_connector_destroy_state,
	.atomic_set_property	= meson_dummyp_atomic_set_property,
	.atomic_get_property	= meson_dummyp_atomic_get_property,
};

static void meson_dummyp_encoder_atomic_enable(struct drm_encoder *encoder,
	struct drm_atomic_state *state)
{
	struct am_meson_crtc_state *meson_crtc_state =
				to_am_meson_crtc_state(encoder->crtc->state);
	struct am_meson_crtc *amcrtc = to_am_meson_crtc(encoder->crtc);
	struct drm_display_mode *mode = &encoder->crtc->mode;
	enum vmode_e vmode = meson_crtc_state->vmode;

	if (vmode != VMODE_DUMMY_ENCP) {
		DRM_DEBUG("%s:enable fail! vmode:%d\n", __func__, vmode);
		return;
	}

	if (meson_crtc_state->uboot_mode_init == 1)
		vmode |= VMODE_INIT_BIT_MASK;

	DRM_DEBUG("[%s]-[%d] called\n", __func__, __LINE__);

	meson_vout_notify_mode_change(amcrtc->vout_index,
		vmode, EVENT_MODE_SET_START);
	vout_func_set_vmode(amcrtc->vout_index, vmode);
	meson_vout_notify_mode_change(amcrtc->vout_index,
		vmode, EVENT_MODE_SET_FINISH);
	meson_vout_update_mode_name(amcrtc->vout_index, mode->name, "dummy_panel");

	DRM_DEBUG("[%s]-[%d] exit\n", __func__, __LINE__);
}

static void meson_dummyp_encoder_atomic_disable(struct drm_encoder *encoder,
	struct drm_atomic_state *state)
{
	DRM_DEBUG("[%s]-[%d] called\n", __func__, __LINE__);
}

static int meson_dummyp_encoder_atomic_check(struct drm_encoder *encoder,
				       struct drm_crtc_state *crtc_state,
				struct drm_connector_state *conn_state)
{
	DRM_DEBUG("[%s]-[%d] called\n", __func__, __LINE__);
	return 0;
}

static const struct drm_encoder_helper_funcs meson_dummyp_encoder_helper_funcs = {
	.atomic_enable = meson_dummyp_encoder_atomic_enable,
	.atomic_disable = meson_dummyp_encoder_atomic_disable,
	.atomic_check = meson_dummyp_encoder_atomic_check,
};

static const struct drm_encoder_funcs meson_dummyp_encoder_funcs = {
	.destroy        = drm_encoder_cleanup,
};

int meson_dummyp_dev_bind(struct drm_device *drm,
	int type, struct meson_connector_dev *intf)
{
	struct drm_connector *connector = NULL;
	struct drm_encoder *encoder = NULL;
	struct meson_dummyp *am_dummyp = NULL;
	char *connector_name = "MESON-DUMMYP";
	int ret = 0;

	DRM_INFO("[%s]-[%d] called\n", __func__, __LINE__);

	am_dummyp = kzalloc(sizeof(*am_dummyp), GFP_KERNEL);
	if (!am_dummyp) {
		DRM_ERROR("[%s]: alloc drm_dummyp failed\n", __func__);
		return -ENOMEM;
	}

	am_dummyp->drm = drm;
	encoder = &am_dummyp->encoder;
	/* Encoder */
	encoder->possible_crtcs = 0x3;
	drm_encoder_helper_add(encoder, &meson_dummyp_encoder_helper_funcs);
	ret = drm_encoder_init(drm, encoder, &meson_dummyp_encoder_funcs,
			       DRM_MODE_ENCODER_VIRTUAL, "am_dummyp_encoder");
	if (ret) {
		DRM_ERROR("error:%s-%d: Failed to init lcd encoder\n",
			__func__, __LINE__);
		goto free_resource;
	}

	/* Connector */
	connector = &am_dummyp->base.connector;
	drm_connector_helper_add(connector, &meson_dummyp_helper_funcs);
	ret = drm_connector_init(drm, connector, &meson_dummyp_funcs,
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
	kfree(am_dummyp);

	DRM_DEBUG("%s: %d Exit\n", __func__, ret);
	return ret;
}

int meson_dummyp_dev_unbind(struct drm_device *drm,
	int type, int connector_id)
{
	struct drm_connector *connector =
		drm_connector_lookup(drm, 0, connector_id);
	struct meson_dummyp *am_dummyp = 0;

	if (!connector)
		DRM_ERROR("%s got invalid connector id %d\n",
			__func__, connector_id);

	am_dummyp = connector_to_meson_dummyp(connector);
	if (!am_dummyp)
		return -EINVAL;

	kfree(am_dummyp);
	DRM_DEBUG("[%s]-[%d] called\n", __func__, __LINE__);
	return 0;
}

