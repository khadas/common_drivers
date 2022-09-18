// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_dev_common.h>
#include <drm/amlogic/meson_drm_bind.h>
#include <linux/component.h>
#include "hdmitx_drm_hook.h"

/*!!Only one instance supported.*/
static struct hdmitx_common *global_tx_base;
static struct hdmitx_hw_common *global_tx_hw;
static struct meson_hdmitx_dev hdmitx_drm_instance;
static int drm_hdmitx_id;

static int drm_hdmitx_get_hpd_state(void)
{
	return global_tx_base->hpd_state;
}

static int drm_hdmitx_register_hpd_cb(struct connector_hpd_cb *hpd_cb)
{
	return hdmitx_register_hpd_cb(global_tx_base, hpd_cb);
}

static unsigned char *drm_hdmitx_get_raw_edid(void)
{
	return hdmitx_get_raw_edid(global_tx_base);
}

static void drm_hdmitx_setup_attr(const char *buf)
{
	hdmitx_setup_attr(global_tx_base, buf);
}

static void drm_hdmitx_get_attr(char attr[16])
{
	hdmitx_get_attr(global_tx_base, attr);
}

static int drm_hdmitx_get_hdr_priority(void)
{
	return global_tx_base->hdr_priority;
}

static unsigned int drm_hdmitx_get_contenttypes(void)
{
	unsigned int types = 1 << DRM_MODE_CONTENT_TYPE_NO_DATA;/*NONE DATA*/
	struct rx_cap *prxcap = &global_tx_base->rxcap;

	if (prxcap->cnc0)
		types |= 1 << DRM_MODE_CONTENT_TYPE_GRAPHICS;
	if (prxcap->cnc1)
		types |= 1 << DRM_MODE_CONTENT_TYPE_PHOTO;
	if (prxcap->cnc2)
		types |= 1 << DRM_MODE_CONTENT_TYPE_CINEMA;
	if (prxcap->cnc3)
		types |= 1 << DRM_MODE_CONTENT_TYPE_GAME;

	return types;
}

static const struct dv_info *drm_hdmitx_get_dv_info(void)
{
	const struct dv_info *dv = &global_tx_base->rxcap.dv_info;

	return dv;
}

static const struct hdr_info *drm_hdmitx_get_hdr_info(void)
{
	static struct hdr_info hdrinfo;

	hdmitx_get_hdrinfo(global_tx_base, &hdrinfo);
	return &hdrinfo;
}

static void drm_hdmitx_avmute(unsigned char mute)
{
	int muteflag = OFF_AVMUTE;

	if (mute == 0)
		muteflag = CLR_AVMUTE;
	else
		muteflag = SET_AVMUTE;

	hdmitx_hw_avmute(global_tx_hw, muteflag);
}

static void drm_hdmitx_set_phy(unsigned char en)
{
	hdmitx_hw_set_phy(global_tx_hw, en);
}

static int drm_hdmitx_set_contenttype(int content_type)
{
	int ret = 0;

	/* for content type game function conflict with ALLM, so
	 * reset allm to enable contenttype.
	 * TODO: follow spec to skip contenttype when ALLM is on.
	 */
	hdmitx_dev_setup_vsif_packet(global_tx_base,
		global_tx_hw, VT_HDMI14_4K, 1, NULL);

	/*reset previous ct.*/
	global_tx_hw->cntlconfig(global_tx_hw, CONF_CT_MODE, SET_CT_OFF);
	global_tx_base->ct_mode = 0;

	switch (content_type) {
	case DRM_MODE_CONTENT_TYPE_GRAPHICS:
		content_type = SET_CT_GRAPHICS;
		break;
	case DRM_MODE_CONTENT_TYPE_PHOTO:
		content_type = SET_CT_PHOTO;
		break;
	case DRM_MODE_CONTENT_TYPE_CINEMA:
		content_type = SET_CT_CINEMA;
		break;
	case DRM_MODE_CONTENT_TYPE_GAME:
		content_type = SET_CT_GAME;
		break;
	default:
		pr_err("[%s]: [%d] unsupported type\n",
			__func__, content_type);
		content_type = 0;
		ret = -EINVAL;
		break;
	};

	global_tx_hw->cntlconfig(global_tx_hw,
		CONF_CT_MODE, content_type);
	global_tx_base->ct_mode = content_type;

	return ret;
}

static int meson_hdmitx_bind(struct device *dev,
			      struct device *master, void *data)
{
	struct meson_drm_bound_data *bound_data = data;

	if (bound_data->connector_component_bind) {
		drm_hdmitx_id = bound_data->connector_component_bind
			(bound_data->drm,
			DRM_MODE_CONNECTOR_HDMIA,
			&hdmitx_drm_instance.base);
		pr_err("%s hdmi [%d]\n", __func__, drm_hdmitx_id);
	} else {
		pr_err("no bind func from drm.\n");
	}

	return 0;
}

static void meson_hdmitx_unbind(struct device *dev,
				 struct device *master, void *data)
{
	struct meson_drm_bound_data *bound_data = data;

	if (bound_data->connector_component_unbind) {
		bound_data->connector_component_unbind(bound_data->drm,
			DRM_MODE_CONNECTOR_HDMIA, drm_hdmitx_id);
		pr_err("%s hdmi [%d]\n", __func__, drm_hdmitx_id);
	} else {
		pr_err("no unbind func.\n");
	}

	drm_hdmitx_id = 0;
	global_tx_base = 0;
}

/*drm component bind ops*/
static const struct component_ops meson_hdmitx_bind_ops = {
	.bind	= meson_hdmitx_bind,
	.unbind	= meson_hdmitx_unbind,
};

int hdmitx_bind_meson_drm(struct device *device,
	struct hdmitx_common *tx_base,
	struct hdmitx_hw_common *tx_hw,
	struct meson_hdmitx_dev *diff)
{
	if (global_tx_base)
		pr_err("global_tx_base [%p] already hooked.\n", global_tx_base);

	global_tx_base = tx_base;
	global_tx_hw = tx_hw;

	hdmitx_drm_instance = *diff;
	hdmitx_drm_instance.base.ver = MESON_DRM_CONNECTOR_V10;

	hdmitx_drm_instance.detect = drm_hdmitx_get_hpd_state;
	hdmitx_drm_instance.register_hpd_cb = drm_hdmitx_register_hpd_cb;

	hdmitx_drm_instance.setup_attr = drm_hdmitx_setup_attr;
	hdmitx_drm_instance.get_attr = drm_hdmitx_get_attr;
	hdmitx_drm_instance.get_hdr_priority = drm_hdmitx_get_hdr_priority;

	/*edid related.*/
	hdmitx_drm_instance.get_raw_edid = drm_hdmitx_get_raw_edid;
	hdmitx_drm_instance.get_content_types = drm_hdmitx_get_contenttypes;
	hdmitx_drm_instance.get_dv_info = drm_hdmitx_get_dv_info;
	hdmitx_drm_instance.get_hdr_info = drm_hdmitx_get_hdr_info;

	/*hw related*/
	hdmitx_drm_instance.avmute = drm_hdmitx_avmute;
	hdmitx_drm_instance.set_phy = drm_hdmitx_set_phy;

	hdmitx_drm_instance.set_content_type = drm_hdmitx_set_contenttype;

	return component_add(device, &meson_hdmitx_bind_ops);
}

int hdmitx_unbind_meson_drm(struct device *device,
	struct hdmitx_common *tx_base,
	struct hdmitx_hw_common *tx_hw,
	struct meson_hdmitx_dev *diff)
{
	if (drm_hdmitx_id != 0)
		component_del(device, &meson_hdmitx_bind_ops);
	global_tx_base = 0;
	global_tx_hw = 0;
	return 0;
}
