// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/spinlock.h>
#include <linux/rtc.h>
#include <linux/timekeeping.h>
#include <linux/amlogic/media/vout/hdmi_tx_ext.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_common.h>
#include <hdmitx_boot_parameters.h>
#include "hdmitx_log.h"
#include "../../../efuse_unifykey/efuse.h"

int hdmitx_format_para_init(struct hdmi_format_para *para,
		enum hdmi_vic vic, u32 frac_rate_policy,
		enum hdmi_colorspace cs, enum hdmi_color_depth cd,
		enum hdmi_quantization_range cr);
const struct hdmi_timing *hdmitx_mode_match_timing_name(const char *name);

void hdmitx_get_init_state(struct hdmitx_common *tx_common,
			   struct hdmitx_common_state *state)
{
	struct hdmi_format_para *para = &tx_common->fmt_para;

	memcpy(&state->para, para, sizeof(*para));
	state->hdr_priority = tx_common->hdr_priority;
}
EXPORT_SYMBOL(hdmitx_get_init_state);

int hdmitx_common_init(struct hdmitx_common *tx_comm, struct hdmitx_hw_common *hw_comm)
{
	struct hdmitx_boot_param *boot_param = get_hdmitx_boot_params();

	/*load tx boot params*/
	tx_comm->hdr_priority = boot_param->hdr_mask;
	/* rxcap.hdmichecksum save the edid checksum of kernel */
	/* memcpy(tx_comm->rxcap.hdmichecksum, boot_param->edid_chksum, */
		/* sizeof(tx_comm->rxcap.hdmichecksum)); */
	memcpy(tx_comm->fmt_attr, boot_param->color_attr, sizeof(tx_comm->fmt_attr));

	tx_comm->frac_rate_policy = boot_param->fraction_refreshrate;
	tx_comm->config_csc_en = boot_param->config_csc;
	tx_comm->res_1080p = 0;
	tx_comm->max_refreshrate = 60;

	tx_comm->tx_hw = hw_comm;
	hw_comm->hdcp_repeater_en = 0;

	tx_comm->rxcap.physical_addr = 0xffff;
	tx_comm->debug_param.avmute_frame = 0;

	hdmitx_format_para_reset(&tx_comm->fmt_para);

	/*mutex init*/
	mutex_init(&tx_comm->hdmimode_mutex);
	return 0;
}

int hdmitx_common_attch_platform_data(struct hdmitx_common *tx_comm,
	enum HDMITX_PLATFORM_API_TYPE type, void *plt_data)
{
	switch (type) {
	case HDMITX_PLATFORM_TRACER:
		tx_comm->tx_tracer = (struct hdmitx_tracer *)plt_data;
		break;
	case HDMITX_PLATFORM_UEVENT:
		tx_comm->event_mgr = (struct hdmitx_event_mgr *)plt_data;
		break;
	default:
		HDMITX_ERROR("%s unknown platform api %d\n", __func__, type);
		break;
	};

	return 0;
}

int hdmitx_common_destroy(struct hdmitx_common *tx_comm)
{
	if (tx_comm->tx_tracer)
		hdmitx_tracer_destroy(tx_comm->tx_tracer);
	if (tx_comm->event_mgr)
		hdmitx_event_mgr_destroy(tx_comm->event_mgr);
	return 0;
}

int hdmitx_common_validate_vic(struct hdmitx_common *tx_comm, u32 vic)
{
	const struct hdmi_timing *timing = hdmitx_mode_vic_to_hdmi_timing(vic);

	if (!timing)
		return -EINVAL;

	/*soc level filter*/
	/*filter 1080p max size.*/
	if (tx_comm->res_1080p) {
		/* if the vic equals to HDMI_0_UNKNOWN or VESA,
		 * then create it as over limited
		 */
		if (vic == HDMI_0_UNKNOWN || vic >= HDMITX_VESA_OFFSET)
			return -ERANGE;

		/* check the resolution is over 1920x1080 or not */
		if (timing->h_active > 1920 || timing->v_active > 1080)
			return -ERANGE;

		/* check the fresh rate is over 60hz or not */
		if (timing->v_freq > 60000)
			return -ERANGE;

		/* test current vic is over 150MHz or not */
		if (timing->pixel_freq > 150000)
			return -ERANGE;
	}

	/* efuse ctrl all 4k mode */
	if (tx_comm->efuse_dis_output_4k)
		if (timing->v_active >= 2160)
			return -ERANGE;

	/* efuse ctrl 4k50, 4k60 */
	if (tx_comm->efuse_dis_hdmi_4k60)
		if (timing->v_active >= 2160 && timing->v_freq >= 5000)
			return -ERANGE;

	/*filter max refreshrate.*/
	if (timing->v_freq > (tx_comm->max_refreshrate * 1000)) {
		//HDMITX_INFO("validate refreshrate (%s)-(%d) fail\n",
		//timing->name, timing->v_freq);
		return -EACCES;
	}

	/*ip level filter*/
	if (hdmitx_hw_validate_mode(tx_comm->tx_hw, vic) != 0)
		return -EPERM;

	return 0;
}

int hdmitx_common_validate_format_para(struct hdmitx_common *tx_comm,
	struct hdmi_format_para *para)
{
	int ret = 0;
	unsigned int calc_tmds_clk = para->tmds_clk / 1000;

	if (para->vic == HDMI_0_UNKNOWN)
		return -EPERM;

	/* if current status already limited to 1080p, so here also needs to
	 * limit the rx_max_tmds_clk as 150 * 1.5 = 225 to make the valid mode
	 * checking works
	 */
	if (tx_comm->res_1080p) {
		if (calc_tmds_clk > 225)
			return -ERANGE;
	}

	ret = hdmitx_edid_validate_format_para(&tx_comm->tx_hw->txcap, &tx_comm->rxcap, para);

	return ret;
}
EXPORT_SYMBOL(hdmitx_common_validate_format_para);

int hdmitx_common_build_format_para(struct hdmitx_common *tx_comm,
		struct hdmi_format_para *para, enum hdmi_vic vic, u32 frac_rate_policy,
		enum hdmi_colorspace cs, enum hdmi_color_depth cd, enum hdmi_quantization_range cr)
{
	int ret = 0;

	ret = hdmitx_format_para_init(para, vic, frac_rate_policy, cs, cd, cr);
	if (ret == 0)
		ret = hdmitx_hw_calc_format_para(tx_comm->tx_hw, para);
	if (ret < 0)
		hdmitx_format_para_print(para, NULL);

	return ret;
}
EXPORT_SYMBOL(hdmitx_common_build_format_para);

int hdmitx_common_init_bootup_format_para(struct hdmitx_common *tx_comm,
		struct hdmi_format_para *para)
{
	int ret = 0;
	struct hdmitx_hw_common *tx_hw = tx_comm->tx_hw;
	enum hdmi_tf_type dv_type;

	if (hdmitx_hw_get_state(tx_hw, STAT_TX_OUTPUT, 0)) {
		para->vic = hdmitx_hw_get_state(tx_hw, STAT_VIDEO_VIC, 0);
		para->cs = hdmitx_hw_get_state(tx_hw, STAT_VIDEO_CS, 0);
		para->cd = hdmitx_hw_get_state(tx_hw, STAT_VIDEO_CD, 0);
		/* when STD DV has already output in uboot, the real cs is rgb
		 * but hdmi CSC actually uses the Y444. So here needs to reassign
		 * the para->cs as YUV444
		 */
		if (para->cs == HDMI_COLORSPACE_RGB) {
			dv_type = hdmitx_hw_get_state(tx_hw, STAT_TX_DV, 0);
			if (dv_type == HDMI_DV_VSIF_STD)
				para->cs = HDMI_COLORSPACE_YUV444;
		}

		ret = hdmitx_common_build_format_para(tx_comm, para, para->vic,
			tx_comm->frac_rate_policy, para->cs, para->cd,
			HDMI_QUANTIZATION_RANGE_FULL);
		if (ret == 0) {
			HDMITX_INFO("%s init ok\n", __func__);
			hdmitx_format_para_print(para, NULL);
		} else {
			HDMITX_INFO("%s: init uboot format para fail (%d,%d,%d)\n",
				__func__, para->vic, para->cs, para->cd);
		}

		return ret;
	} else {
		HDMITX_INFO("%s hdmi is not enabled\n", __func__);
		return hdmitx_format_para_reset(para);
	}
}

int hdmitx_fire_drm_hpd_cb_unlocked(struct hdmitx_common *tx_comm)
{
	if (tx_comm->drm_hpd_cb.callback)
		tx_comm->drm_hpd_cb.callback(tx_comm->drm_hpd_cb.data);

	return 0;
}

int hdmitx_register_hpd_cb(struct hdmitx_common *tx_comm, struct connector_hpd_cb *hpd_cb)
{
	tx_comm->drm_hpd_cb.callback = hpd_cb->callback;
	tx_comm->drm_hpd_cb.data = hpd_cb->data;
	return 0;
}
EXPORT_SYMBOL(hdmitx_register_hpd_cb);

/* TODO: no mutex */
int hdmitx_get_hpd_state(struct hdmitx_common *tx_comm)
{
	return tx_comm->hpd_state;
}
EXPORT_SYMBOL(hdmitx_get_hpd_state);

/* TODO: no mutex */
unsigned char *hdmitx_get_raw_edid(struct hdmitx_common *tx_comm)
{
	return tx_comm->EDID_buf;
}
EXPORT_SYMBOL(hdmitx_get_raw_edid);

/* TODO: no mutex */
bool hdmitx_common_get_ready_state(struct hdmitx_common *tx_comm)
{
	return tx_comm->ready;
}
EXPORT_SYMBOL(hdmitx_common_get_ready_state);

int hdmitx_setup_attr(struct hdmitx_common *tx_comm, const char *buf)
{
	char attr[16] = {0};
	int len = strlen(buf);

	if (len <= 16)
		memcpy(attr, buf, len);
	memcpy(tx_comm->fmt_attr, attr, sizeof(tx_comm->fmt_attr));
	return 0;
}
EXPORT_SYMBOL(hdmitx_setup_attr);

int hdmitx_get_attr(struct hdmitx_common *tx_comm, char attr[16])
{
	memcpy(attr, tx_comm->fmt_attr, sizeof(tx_comm->fmt_attr));
	return 0;
}
EXPORT_SYMBOL(hdmitx_get_attr);

/* hdr_priority definition:
 *   strategy1: bit[3:0]
 *       0: original cap
 *       1: disable DV cap
 *       2: disable DV and hdr cap
 *   strategy2:
 *       bit4: 1: disable dv  0:enable dv
 *       bit5: 1: disable hdr10/hdr10+  0: enable hdr10/hdr10+
 *       bit6: 1: disable hlg  0: enable hlg
 *   bit28-bit31 choose strategy: bit[31:28]
 *       0: strategy1
 *       1: strategy2
 */

/* dv_info */
static void enable_dv_info(struct dv_info *des, const struct dv_info *src)
{
	if (!des || !src)
		return;

	memcpy(des, src, sizeof(*des));
}

static void disable_dv_info(struct dv_info *des)
{
	if (!des)
		return;

	memset(des, 0, sizeof(*des));
}

/* hdr10 */
static void enable_hdr10_info(struct hdr_info *des, const struct hdr_info *src)
{
	if (!des || !src)
		return;

	des->hdr_support |= (src->hdr_support) & BIT(2);
	des->static_metadata_type1 = src->static_metadata_type1;
	des->lumi_max = src->lumi_max;
	des->lumi_avg = src->lumi_avg;
	des->lumi_min = src->lumi_min;
	des->lumi_peak = src->lumi_peak;
	des->ldim_support = src->ldim_support;
}

static void disable_hdr10_info(struct hdr_info *des)
{
	if (!des)
		return;

	des->hdr_support &= ~BIT(2);
	des->static_metadata_type1 = 0;
	des->lumi_max = 0;
	des->lumi_avg = 0;
	des->lumi_min = 0;
	des->lumi_peak = 0;
	des->ldim_support = 0;
}

/* hdr10plus */
static void enable_hdr10p_info(struct hdr10_plus_info *des, const struct hdr10_plus_info *src)
{
	if (!des || !src)
		return;

	memcpy(des, src, sizeof(*des));
}

static void disable_hdr10p_info(struct hdr10_plus_info *des)
{
	if (!des)
		return;

	memset(des, 0, sizeof(*des));
}

/* hlg */
static void enable_hlg_info(struct hdr_info *des, const struct hdr_info *src)
{
	if (!des || !src)
		return;

	des->hdr_support |= (src->hdr_support) & BIT(3);
}

static void disable_hlg_info(struct hdr_info *des)
{
	if (!des)
		return;

	des->hdr_support &= ~BIT(3);
}

static void enable_all_hdr_info(struct rx_cap *prxcap)
{
	if (!prxcap)
		return;

	memcpy(&prxcap->hdr_info, &prxcap->hdr_info2, sizeof(prxcap->hdr_info));
	memcpy(&prxcap->dv_info, &prxcap->dv_info2, sizeof(prxcap->dv_info));
}

static void update_hdr_strategy1(struct hdmitx_common *tx_comm, u32 strategy)
{
	struct rx_cap *prxcap;

	if (!tx_comm)
		return;

	prxcap = &tx_comm->rxcap;
	switch (strategy) {
	case 0:
		enable_all_hdr_info(prxcap);
		break;
	case 1:
		disable_dv_info(&prxcap->dv_info);
		break;
	case 2:
		disable_dv_info(&prxcap->dv_info);
		disable_hdr10_info(&prxcap->hdr_info);
		disable_hdr10p_info(&prxcap->hdr_info.hdr10plus_info);
		break;
	default:
		break;
	}
}

static void update_hdr_strategy2(struct hdmitx_common *tx_comm, u32 strategy)
{
	struct rx_cap *prxcap;

	if (!tx_comm)
		return;

	prxcap = &tx_comm->rxcap;
	/* bit4: 1 disable dv  0 enable dv */
	if (strategy & BIT(4))
		disable_dv_info(&prxcap->dv_info);
	else
		enable_dv_info(&prxcap->dv_info, &prxcap->dv_info2);
	/* bit5: 1 disable hdr10/hdr10+   0 enable hdr10/hdr10+ */
	if (strategy & BIT(5)) {
		disable_hdr10_info(&prxcap->hdr_info);
		disable_hdr10p_info(&prxcap->hdr_info.hdr10plus_info);
	} else {
		enable_hdr10_info(&prxcap->hdr_info, &prxcap->hdr_info2);
		enable_hdr10p_info(&prxcap->hdr_info.hdr10plus_info,
			&prxcap->hdr_info2.hdr10plus_info);
	}
	/* bit6: 1 disable hlg   0 enable hlg */
	if (strategy & BIT(6))
		disable_hlg_info(&prxcap->hdr_info);
	else
		enable_hlg_info(&prxcap->hdr_info, &prxcap->hdr_info2);
}

int hdmitx_set_hdr_priority(struct hdmitx_common *tx_comm, u32 hdr_priority)
{
	u32 choose = 0;
	u32 strategy = 0;

	tx_comm->hdr_priority = hdr_priority;
	HDMITX_DEBUG("%s, set hdr_prio: %u\n", __func__, hdr_priority);
	/* choose strategy: bit[31:28] */
	choose = (tx_comm->hdr_priority >> 28) & 0xf;
	switch (choose) {
	case 0:
		strategy = tx_comm->hdr_priority & 0xf;
		update_hdr_strategy1(tx_comm, strategy);
		break;
	case 1:
		strategy = tx_comm->hdr_priority & 0xf0;
		update_hdr_strategy2(tx_comm, strategy);
		break;
	default:
		break;
	}
	return 0;
}
EXPORT_SYMBOL(hdmitx_set_hdr_priority);

int hdmitx_get_hdr_priority(struct hdmitx_common *tx_comm, u32 *hdr_priority)
{
	*hdr_priority = tx_comm->hdr_priority;
	return 0;
}
EXPORT_SYMBOL(hdmitx_get_hdr_priority);

int hdmitx_get_hdrinfo(struct hdmitx_common *tx_comm, struct hdr_info *hdrinfo)
{
	struct rx_cap *prxcap = &tx_comm->rxcap;

	memcpy(hdrinfo, &prxcap->hdr_info, sizeof(struct hdr_info));
	hdrinfo->colorimetry_support = prxcap->colorimetry_data;

	return 0;
}

static enum hdmi_vic get_alternate_ar_vic(enum hdmi_vic vic)
{
	int i = 0;
	struct {
		u32 mode_16x9_vic;
		u32 mode_4x3_vic;
	} vic_pairs[] = {
		{HDMI_7_720x480i60_16x9, HDMI_6_720x480i60_4x3},
		{HDMI_3_720x480p60_16x9, HDMI_2_720x480p60_4x3},
		{HDMI_22_720x576i50_16x9, HDMI_21_720x576i50_4x3},
		{HDMI_18_720x576p50_16x9, HDMI_17_720x576p50_4x3},
	};

	for (i = 0; i < ARRAY_SIZE(vic_pairs); i++) {
		if (vic_pairs[i].mode_16x9_vic == vic)
			return vic_pairs[i].mode_4x3_vic;
		if (vic_pairs[i].mode_4x3_vic == vic)
			return vic_pairs[i].mode_16x9_vic;
	}

	return HDMI_0_UNKNOWN;
}

int hdmitx_common_check_valid_para_of_vic(struct hdmitx_common *tx_comm, enum hdmi_vic vic)
{
	struct hdmi_format_para tst_para;
	enum hdmi_color_depth cd; /* cd8, cd10 or cd12 */
	enum hdmi_colorspace cs; /* 0/1/2/3: rgb/422/444/420 */
	const enum hdmi_quantization_range cr = HDMI_QUANTIZATION_RANGE_FULL; /*default to full*/
	int i = 0;

	if (vic == HDMI_0_UNKNOWN)
		return -EINVAL;

	if (vic == HDMI_96_3840x2160p50_16x9 ||
		vic == HDMI_97_3840x2160p60_16x9 ||
		vic == HDMI_101_4096x2160p50_256x135 ||
		vic == HDMI_102_4096x2160p60_256x135) {
		cs = HDMI_COLORSPACE_YUV420;
		cd = COLORDEPTH_24B;
		if (hdmitx_common_build_format_para(tx_comm,
			&tst_para, vic, false, cs, cd, cr) == 0 &&
			hdmitx_common_validate_format_para(tx_comm, &tst_para) == 0) {
			return 0;
		}
	}

	struct {
		enum hdmi_colorspace cs;
		enum hdmi_color_depth cd;
	} test_color_attr[] = {
		{HDMI_COLORSPACE_RGB, COLORDEPTH_24B},
		{HDMI_COLORSPACE_YUV444, COLORDEPTH_24B},
		{HDMI_COLORSPACE_YUV422, COLORDEPTH_36B},
	};

	for (i = 0; i < ARRAY_SIZE(test_color_attr); i++) {
		cs = test_color_attr[i].cs;
		cd = test_color_attr[i].cd;
		hdmitx_format_para_reset(&tst_para);
		if (hdmitx_common_build_format_para(tx_comm,
			&tst_para, vic, false, cs, cd, cr) == 0 &&
			hdmitx_common_validate_format_para(tx_comm, &tst_para) == 0) {
			return 0;
		}
	}

	return -EPERM;
}
EXPORT_SYMBOL(hdmitx_common_check_valid_para_of_vic);

int hdmitx_common_parse_vic_in_edid(struct hdmitx_common *tx_comm, const char *mode)
{
	const struct hdmi_timing *timing;
	enum hdmi_vic vic = HDMI_0_UNKNOWN;
	enum hdmi_vic alternate_vic = HDMI_0_UNKNOWN;

	/*parse by name to find default mode*/
	timing = hdmitx_mode_match_timing_name(mode);
	if (!timing || timing->vic == HDMI_0_UNKNOWN)
		return HDMI_0_UNKNOWN;

	vic = timing->vic;
	/* for compatibility: 480p/576p
	 * 480p/576p use same short name in hdmitx_timing table, so when match name, will return
	 * 4x3 mode fist. But user prefer 16x9 first, so try 16x9 first;
	 */
	alternate_vic = get_alternate_ar_vic(vic);
	if (alternate_vic != HDMI_0_UNKNOWN) {
		//HDMITX_INFO("get alternate vic %d->%d\n", vic, alternate_vic);
		vic = alternate_vic;
	}

	/*check if vic supported by rx*/
	if (hdmitx_edid_validate_mode(&tx_comm->rxcap, vic) == true)
		return vic;

	/* for compatibility: 480p/576p, will get 0 if there is no alternate vic;*/
	alternate_vic = get_alternate_ar_vic(vic);
	if (alternate_vic != vic) {
		//HDMITX_INFO("get alternate vic %d->%d\n", vic, alternate_vic);
		vic = alternate_vic;
	}

	if (vic != HDMI_0_UNKNOWN && hdmitx_edid_validate_mode(&tx_comm->rxcap, vic) == false)
		vic = HDMI_0_UNKNOWN;

	/* Dont call hdmitx_common_check_valid_para_of_vic anymore.
	 * This function used to parse user passed mode name which should already
	 * checked by hdmitx_common_check_valid_para_of_vic().
	 */

	if (vic == HDMI_0_UNKNOWN)
		HDMITX_ERROR("%s: parse mode %s vic %d\n", __func__, mode, vic);

	return vic;
}
EXPORT_SYMBOL(hdmitx_common_parse_vic_in_edid);

int hdmitx_common_notify_hpd_status(struct hdmitx_common *tx_comm, bool force_uevent)
{
	if (!tx_comm->suspend_flag) {
		/*notify to userspace by uevent*/
		hdmitx_event_mgr_send_uevent(tx_comm->event_mgr,
					HDMITX_HPD_EVENT, tx_comm->hpd_state, force_uevent);
		hdmitx_event_mgr_send_uevent(tx_comm->event_mgr,
					HDMITX_AUDIO_EVENT, tx_comm->hpd_state, force_uevent);
	} else {
		/* under early suspend, only update uevent state, not
		 * post to system, in case 1.old android system will
		 * set hdmi mode, 2.audio server and audio_hal will
		 * start run, increase power consumption
		 */
		hdmitx_event_mgr_set_uevent_state(tx_comm->event_mgr,
			HDMITX_HPD_EVENT, tx_comm->hpd_state);
		hdmitx_event_mgr_set_uevent_state(tx_comm->event_mgr,
			HDMITX_AUDIO_EVENT, tx_comm->hpd_state);
	}

	/* notify to other driver module:cec/rx
	 * TODO: need lock for EDID_buf
	 * note should not be used under TV product
	 */
	if (tx_comm->hdmi_repeater == 1) {
		if (tx_comm->hpd_state)
			hdmitx_event_mgr_notify(tx_comm->event_mgr, HDMITX_PLUG,
				tx_comm->rxcap.edid_parsing ? tx_comm->EDID_buf : NULL);
		else
			hdmitx_event_mgr_notify(tx_comm->event_mgr, HDMITX_UNPLUG, NULL);
	}
	return 0;
}

int hdmitx_common_set_allm_mode(struct hdmitx_common *tx_comm, int mode)
{
	struct hdmitx_hw_common *tx_hw_base = tx_comm->tx_hw;

	if (mode == 0) {
		tx_comm->allm_mode = 0;
		hdmitx_common_setup_vsif_packet(tx_comm, VT_ALLM, 0, NULL);
		hdmitx_common_setup_vsif_packet(tx_comm, VT_HDMI14_4K, 1, NULL);
	}
	if (mode == 1) {
		tx_comm->allm_mode = 1;
		hdmitx_common_setup_vsif_packet(tx_comm, VT_ALLM, 1, NULL);
		hdmitx_hw_cntl_config(tx_hw_base, CONF_CT_MODE, SET_CT_OFF);
	}

	if (mode == -1) {
		if (tx_comm->allm_mode == 1) {
			tx_comm->allm_mode = 0;
			hdmitx_hw_disable_packet(tx_hw_base, HDMI_PACKET_VEND);
		}
	}
	return 0;
}
EXPORT_SYMBOL(hdmitx_common_set_allm_mode);

static unsigned int get_frame_duration(struct vinfo_s *vinfo)
{
	unsigned int frame_duration;

	if (!vinfo || !vinfo->sync_duration_num)
		return 0;

	frame_duration =
		1000000 * vinfo->sync_duration_den / vinfo->sync_duration_num;
	return frame_duration;
}

int hdmitx_common_avmute_locked(struct hdmitx_common *tx_comm,
	int mute_flag, int mute_path_hint)
{
	static DEFINE_MUTEX(avmute_mutex);
	static unsigned int global_avmute_mask;
	unsigned int mute_us =
		tx_comm->debug_param.avmute_frame * get_frame_duration(&tx_comm->hdmitx_vinfo);

	mutex_lock(&avmute_mutex);

	if (mute_flag == SET_AVMUTE) {
		global_avmute_mask |= mute_path_hint;
		HDMITX_DEBUG("%s: AVMUTE path=0x%x\n", __func__, mute_path_hint);
		hdmitx_hw_cntl_misc(tx_comm->tx_hw, MISC_AVMUTE_OP, SET_AVMUTE);
	} else if (mute_flag == CLR_AVMUTE) {
		global_avmute_mask &= ~mute_path_hint;
		/* unmute only if none of the paths are muted */
		if (global_avmute_mask == 0) {
			HDMITX_DEBUG("%s: AV UNMUTE path=0x%x\n", __func__, mute_path_hint);
			hdmitx_hw_cntl_misc(tx_comm->tx_hw, MISC_AVMUTE_OP, CLR_AVMUTE);
		}
	} else if (mute_flag == OFF_AVMUTE) {
		hdmitx_hw_cntl_misc(tx_comm->tx_hw, MISC_AVMUTE_OP, OFF_AVMUTE);
	}
	if (mute_flag == SET_AVMUTE) {
		if (tx_comm->debug_param.avmute_frame > 0)
			msleep(mute_us / 1000);
		else if (mute_path_hint == AVMUTE_PATH_HDMITX)
			msleep(100);
	}

	mutex_unlock(&avmute_mutex);

	return 0;
}
EXPORT_SYMBOL(hdmitx_common_avmute_locked);

/********************************Debug function***********************************/

int hdmitx_common_get_edid(struct hdmitx_common *tx_comm)
{
	struct hdmitx_hw_common *tx_hw_base = tx_comm->tx_hw;
	unsigned long flags = 0;

	if (tx_comm->forced_edid) {
		HDMITX_INFO("using fixed edid\n");
		return 0;
	}
	hdmitx_edid_buffer_clear(tx_comm->EDID_buf, sizeof(tx_comm->EDID_buf));

	hdmitx_hw_cntl_misc(tx_hw_base, MISC_I2C_RESET, 0); /* reset i2c before edid read */
	hdmitx_hw_cntl_ddc(tx_hw_base, DDC_RESET_EDID, 0);
	hdmitx_hw_cntl_ddc(tx_hw_base, DDC_PIN_MUX_OP, PIN_MUX);
	/* start reading edid first time */
	hdmitx_hw_cntl_ddc(tx_hw_base, DDC_EDID_READ_DATA, 0);
	if (hdmitx_edid_is_all_zeros(tx_comm->EDID_buf)) {
		hdmitx_hw_cntl_ddc(tx_hw_base, DDC_GLITCH_FILTER_RESET, 0);
		hdmitx_hw_cntl_ddc(tx_hw_base, DDC_EDID_READ_DATA, 0);
	}
	/* If EDID is not correct at first time, then retry */
	if (hdmitx_edid_check_data_valid(tx_comm->EDID_buf) == false) {
		struct timespec64 kts;
		struct rtc_time tm;

		msleep(20);
		ktime_get_real_ts64(&kts);
		rtc_time64_to_tm(kts.tv_sec, &tm);
		if (tx_hw_base->hdmitx_gpios_scl != -EPROBE_DEFER)
			HDMITX_INFO("UTC+0 %ptRd %ptRt DDC SCL %s\n", &tm, &tm,
			gpio_get_value(tx_hw_base->hdmitx_gpios_scl) ? "HIGH" : "LOW");
		if (tx_hw_base->hdmitx_gpios_sda != -EPROBE_DEFER)
			HDMITX_INFO("UTC+0 %ptRd %ptRt DDC SDA %s\n", &tm, &tm,
			gpio_get_value(tx_hw_base->hdmitx_gpios_sda) ? "HIGH" : "LOW");
		msleep(80);

		/* start reading edid second time */
		hdmitx_hw_cntl_ddc(tx_hw_base, DDC_EDID_READ_DATA, 0);
		if (hdmitx_edid_is_all_zeros(tx_comm->EDID_buf)) {
			hdmitx_hw_cntl_ddc(tx_hw_base, DDC_GLITCH_FILTER_RESET, 0);
			hdmitx_hw_cntl_ddc(tx_hw_base, DDC_EDID_READ_DATA, 0);
		}
	}

	spin_lock_irqsave(&tx_comm->edid_spinlock, flags);
	hdmitx_edid_rxcap_clear(&tx_comm->rxcap);
	hdmitx_edid_parse(&tx_comm->rxcap, tx_comm->EDID_buf);

	/* update the hdr/hdr10+/dv capabilities in the end of parse */
	hdmitx_set_hdr_priority(tx_comm, tx_comm->hdr_priority);

	spin_unlock_irqrestore(&tx_comm->edid_spinlock, flags);

	/* notify phy addr to rx/cec:
	 * rx/cec currently do not use the phy addr of below
	 * two interfaces, just keep for safety
	 */
	if (tx_comm->hdmi_repeater == 1) {
		hdmitx_event_mgr_notify(tx_comm->event_mgr,
			HDMITX_PHY_ADDR_VALID, &tx_comm->rxcap.physical_addr);
		rx_edid_physical_addr(tx_comm->rxcap.vsdb_phy_addr.a,
			tx_comm->rxcap.vsdb_phy_addr.b,
			tx_comm->rxcap.vsdb_phy_addr.c,
			tx_comm->rxcap.vsdb_phy_addr.d);
	}
	hdmitx_edid_print(tx_comm->EDID_buf);

	return 0;
}

/*only for first time plugout */
bool is_tv_changed(char *cur_edid_chksum, char *boot_param_edid_chksum)
{
	char invalidchecksum[11] = {
		'i', 'n', 'v', 'a', 'l', 'i', 'd', 'c', 'r', 'c', '\0'
	};
	char emptychecksum[11] = {0};
	bool ret = false;

	if (!cur_edid_chksum || !boot_param_edid_chksum)
		return ret;

	if (memcmp(boot_param_edid_chksum, cur_edid_chksum, 10) &&
		memcmp(emptychecksum, cur_edid_chksum, 10) &&
		memcmp(invalidchecksum, boot_param_edid_chksum, 10)) {
		ret = true;
		HDMITX_INFO("hdmi crc is diff between uboot and kernel\n");
	}

	return ret;
}

/* common work for plugout/suspend, which is done in lock */
void hdmitx_common_edid_clear(struct hdmitx_common *tx_comm)
{
	/* clear edid */
	hdmitx_edid_buffer_clear(tx_comm->EDID_buf, sizeof(tx_comm->EDID_buf));
	hdmitx_edid_rxcap_clear(&tx_comm->rxcap);
	if (tx_comm->hdmi_repeater == 1)
		rx_edid_physical_addr(0, 0, 0, 0);
}

unsigned int hdmitx_get_frame_duration(void)
{
	unsigned int frame_duration;
	struct vinfo_s *vinfo = hdmitx_get_current_vinfo(NULL);

	if (!vinfo || !vinfo->sync_duration_num)
		return 0;

	frame_duration =
		1000000 * vinfo->sync_duration_den / vinfo->sync_duration_num;
	return frame_duration;
}

static char *efuse_name_table[] = {"FEAT_DISABLE_HDMI_60HZ",
				   "FEAT_DISABLE_OUTPUT_4K",
				   "FEAT_DISABLE_HDCP_TX_22",
				   "FEAT_DISABLE_HDMI_TX_3D",
				   "FEAT_DISABLE_HDMITX",
				    NULL};
void get_hdmi_efuse(struct hdmitx_common *tx_comm)
{
	struct efuse_obj_field_t efuse_field;
	u8 buff[32];
	u32 bufflen = sizeof(buff);
	char *efuse_field_name;
	u32 rc = 0;
	int i = 0;

	memset(&buff[0], 0, sizeof(buff));
	memset(&efuse_field, 0, sizeof(efuse_field));

	for (; efuse_name_table[i]; i++) {
		efuse_field_name = efuse_name_table[i];
		rc = efuse_obj_read(EFUSE_OBJ_EFUSE_DATA, efuse_field_name, buff, &bufflen);
		if (rc == EFUSE_OBJ_SUCCESS) {
			strncpy(efuse_field.name, efuse_field_name, sizeof(efuse_field.name) - 1);
			memcpy(efuse_field.data, buff, bufflen);
			efuse_field.size = bufflen;

			if (*efuse_field.data == 1) {
				switch (i) {
				case 0:
					tx_comm->efuse_dis_hdmi_4k60 = 1;
					pr_info("get efuse FEAT_DISABLE_HDMI_60HZ = 1\n");
					break;
				case 1:
					tx_comm->efuse_dis_output_4k = 1;
					pr_info("get efuse FEAT_DISABLE_OUTPUT_4K = 1\n");
					break;
				case 2:
					tx_comm->efuse_dis_hdcp_tx22 = 1;
					pr_info("get efuse FEAT_DISABLE_HDCP_TX_22 = 1\n");
					break;
				case 3:
					tx_comm->efuse_dis_hdmi_tx3d = 1;
					pr_info("get efuse FEAT_DISABLE_HDMI_TX_3D = 1\n");
					break;
				case 4:
					tx_comm->efuse_dis_hdcp_tx14 = 1;
					pr_info("get efuse FEAT_DISABLE_HDMI = 1\n");
					break;
				default:
					break;
				}
			}
		} else {
			pr_err("Error getting %s: %d\n", efuse_field_name, rc);
		}
	}
}
