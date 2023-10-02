// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_common.h>
#include <drm/amlogic/meson_drm_bind.h>
#include <linux/component.h>
#include "hdmitx_drm_hook.h"

/*!!Only one instance supported.*/
const struct hdmi_timing *hdmitx_mode_match_timing_name(const char *name);
static struct hdmitx_common *global_tx_base;
static struct hdmitx_hw_common *global_tx_hw;
static struct meson_hdmitx_dev hdmitx_drm_instance;
static int drm_hdmitx_id;

unsigned int hdmitx_common_get_contenttypes(void)
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
EXPORT_SYMBOL(hdmitx_common_get_contenttypes);

const struct dv_info *hdmitx_common_get_dv_info(void)
{
	const struct dv_info *dv = &global_tx_base->rxcap.dv_info;

	return dv;
}
EXPORT_SYMBOL(hdmitx_common_get_dv_info);

const struct hdr_info *hdmitx_common_get_hdr_info(void)
{
	static struct hdr_info hdrinfo;

	hdmitx_get_hdrinfo(global_tx_base, &hdrinfo);
	return &hdrinfo;
}
EXPORT_SYMBOL(hdmitx_common_get_hdr_info);

int hdmitx_common_set_contenttype(int content_type)
{
	int ret = 0;

	/* for content type game function conflict with ALLM, so
	 * reset allm to enable contenttype.
	 * TODO: follow spec to skip contenttype when ALLM is on.
	 */
	hdmitx_common_setup_vsif_packet(global_tx_base, VT_HDMI14_4K, 1, NULL);

	/*reset previous ct.*/
	hdmitx_hw_cntl_config(global_tx_hw, CONF_CT_MODE, SET_CT_OFF);
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

	hdmitx_hw_cntl_config(global_tx_hw,
		CONF_CT_MODE, content_type);
	global_tx_base->ct_mode = content_type;

	return ret;
}
EXPORT_SYMBOL(hdmitx_common_set_contenttype);

int hdmitx_common_get_vic_list(int **vics)
{
	struct rx_cap *prxcap = &global_tx_base->rxcap;
	enum hdmi_vic vic;
	int len = prxcap->VIC_count + VESA_MAX_TIMING;
	int i;
	int count = 0;
	int *viclist = 0;
	int *edid_vics = 0;

	viclist = kcalloc(len, sizeof(int),  GFP_KERNEL);
	edid_vics = vmalloc(len * sizeof(int));
	memset(edid_vics, 0, len * sizeof(int));

	/*copy edid vic list*/
	if (prxcap->VIC_count > 0)
		memcpy(edid_vics, prxcap->VIC, sizeof(int) * prxcap->VIC_count);
	for (i = 0; i < VESA_MAX_TIMING && prxcap->vesa_timing[i]; i++)
		edid_vics[prxcap->VIC_count + i] = prxcap->vesa_timing[i];

	for (i = 0; i < len; i++) {
		vic = edid_vics[i];
		if (vic == HDMI_0_UNKNOWN)
			continue;

		if (vic == HDMI_2_720x480p60_4x3 ||
			vic == HDMI_6_720x480i60_4x3 ||
			vic == HDMI_17_720x576p50_4x3 ||
			vic == HDMI_21_720x576i50_4x3) {
			if (hdmitx_edid_validate_mode(prxcap, vic + 1) == 0) {
				//pr_info("%s: check vic exist, handle [%d] later.\n",
				//	__func__, vic + 1);
				continue;
			}
		}

		if (hdmitx_common_validate_vic(global_tx_base, vic) != 0) {
			//pr_err("%s: vic[%d] over range.\n", __func__, vic);
			continue;
		}

		if (hdmitx_common_check_valid_para_of_vic(global_tx_base, vic) != 0) {
			//pr_err("%s: vic[%d] check fmt attr failed.\n", __func__, vic);
			continue;
		}

		viclist[count] = vic;
		count++;
	}

	vfree(edid_vics);

	if (count == 0)
		kfree(viclist);
	else
		*vics = viclist;

	return count;
}
EXPORT_SYMBOL(hdmitx_common_get_vic_list);

bool hdmitx_common_chk_mode_attr_sup(char *mode, char *attr)
{
	struct hdmi_format_para tst_para;
	enum hdmi_vic vic = HDMI_0_UNKNOWN;
	int ret = 0;

	if (!mode || !attr)
		return false;

	vic = hdmitx_common_parse_vic_in_edid(global_tx_base, mode);
	if (vic == HDMI_0_UNKNOWN) {
		pr_err("%s: get vic from (%s) fail\n", __func__, mode);
		return false;
	}

	ret = hdmitx_common_validate_vic(global_tx_base, vic);
	if (ret != 0) {
		pr_err("validate vic [%s,%s]-%d return error %d\n", mode, attr, vic, ret);
		return false;
	}

	hdmitx_parse_color_attr(attr, &tst_para.cs, &tst_para.cd, &tst_para.cr);
	ret = hdmitx_common_build_format_para(global_tx_base,
		&tst_para, vic, global_tx_base->frac_rate_policy,
		tst_para.cs, tst_para.cd, tst_para.cr);
	if (ret != 0) {
		hdmitx_format_para_reset(&tst_para);
		pr_err("build formatpara [%s,%s] return error %d\n", mode, attr, ret);
		return false;
	}

	if (true) {
		pr_info("sname = %s\n", tst_para.sname);
		pr_info("char_clk = %d\n", tst_para.tmds_clk);
		pr_info("cd = %d\n", tst_para.cd);
		pr_info("cs = %d\n", tst_para.cs);
	}

	ret = hdmitx_common_validate_format_para(global_tx_base, &tst_para);
	if (ret != 0) {
		pr_err("vlidate formatpara [%s,%s] return error %d\n", mode, attr, ret);
		return false;
	}

	return true;
}
EXPORT_SYMBOL(hdmitx_common_chk_mode_attr_sup);

int hdmitx_common_get_timing_para(int vic, struct drm_hdmitx_timing_para *para)
{
	const struct hdmi_timing *timing;

	timing = hdmitx_mode_vic_to_hdmi_timing(vic);
	if (!timing)
		return -1;

	if (timing->sname) {
		memcpy(para->name, timing->sname, DRM_DISPLAY_MODE_LEN);
	} else if (timing->name) {
		memcpy(para->name, timing->name, DRM_DISPLAY_MODE_LEN);
	} else {
		pr_err(" func %s get vic %d without name\n", __func__, vic);
		return -1;
	}

	para->pi_mode = timing->pi_mode;
	para->pix_repeat_factor = timing->pixel_repetition_factor;
	para->h_pol = timing->h_pol;
	para->v_pol = timing->v_pol;
	para->pixel_freq = timing->pixel_freq;

	para->h_active = timing->h_active;
	para->h_front = timing->h_front;
	para->h_sync = timing->h_sync;
	para->h_total = timing->h_total;
	para->v_active = timing->v_active;
	para->v_front = timing->v_front;
	para->v_sync = timing->v_sync;
	para->v_total = timing->v_total;

	return 0;
}
EXPORT_SYMBOL(hdmitx_common_get_timing_para);

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
	hdmitx_drm_instance.hdmitx_common = tx_base;
	hdmitx_drm_instance.hw_common = tx_hw;

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
