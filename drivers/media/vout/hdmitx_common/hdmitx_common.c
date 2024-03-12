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
#include <linux/amlogic/media/vout/vout_notify.h>
#include <linux/amlogic/media/vout/dsc.h>

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

/* init hdmitx_common struct which is done only when driver probe */
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
	tx_comm->rxcap.edid_check = boot_param->edid_check;

	tx_comm->tx_hw = hw_comm;
	if (tx_comm->tx_hw)
		tx_comm->tx_hw->hdmi_tx_cap.dsc_policy = boot_param->dsc_policy;
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

/* check if vic is supported by SOC hdmitx */
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

/* check fmt para is supported or not by hdmitx/edid.
 * note that vic is not checked if supported by hdmitx/edid
 */
int hdmitx_common_validate_format_para(struct hdmitx_common *tx_comm,
	struct hdmi_format_para *para)
{
	int ret = 0;

	if (para->vic == HDMI_0_UNKNOWN)
		return -EPERM;

	/* check if the format param is within capability of both TX/RX */
	ret = hdmitx_edid_validate_format_para(&tx_comm->tx_hw->hdmi_tx_cap,
		&tx_comm->rxcap, para);

	return ret;
}
EXPORT_SYMBOL(hdmitx_common_validate_format_para);

/* build format para of current mode + cs/cd + frac */
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

/* similar as valid_mode_store(), but with additional lock */
/* validation step:
 * step1, check if mode related VIC is supported in EDID
 * step2, check if VIC is supported by SOC hdmitx
 * step3, build format with mode/attr and check if it's
 * supported by EDID/hdmitx_cap
 */
int hdmitx_common_validate_mode_locked(struct hdmitx_common *tx_comm,
				       struct hdmitx_common_state *new_state,
				       char *mode, char *attr, bool brr_valid, bool do_validate)
{
	int ret = 0;
	struct hdmi_format_para *new_para;
	struct hdmi_format_para tst_para;
	enum hdmi_vic vic = HDMI_0_UNKNOWN;

	new_para = &new_state->para;

	if (new_state->state_sequence_id != tx_comm->tx_hw->hw_sequence_id) {
		HDMITX_ERROR("%s: state_sequence_id failed: %lld\n",
						__func__, new_state->state_sequence_id);
		return -1;
	}

	mutex_lock(&tx_comm->hdmimode_mutex);

	if (!mode || !attr) {
		ret = -EINVAL;
		goto out;
	}

	vic = hdmitx_common_parse_vic_in_edid(tx_comm, mode);
	if (vic == HDMI_0_UNKNOWN) {
		HDMITX_ERROR("%s: get vic from (%s) fail\n", __func__, mode);
		ret = -EINVAL;
		goto out;
	}

	ret = hdmitx_common_validate_vic(tx_comm, vic);
	if (ret != 0) {
		HDMITX_ERROR("validate vic [%s,%s]-%d return error %d\n", mode, attr, vic, ret);
		goto out;
	}

	hdmitx_parse_color_attr(attr, &tst_para.cs, &tst_para.cd, &tst_para.cr);
	ret = hdmitx_common_build_format_para(tx_comm,
		new_para, vic, brr_valid ? 0 : tx_comm->frac_rate_policy,
		tst_para.cs, tst_para.cd, tst_para.cr);
	if (ret != 0) {
		hdmitx_format_para_reset(new_para);
		HDMITX_ERROR("build formatpara [%s,%s] return error %d\n", mode, attr, ret);
		goto out;
	}

	if (do_validate) {
		ret = hdmitx_common_validate_format_para(tx_comm, new_para);
		if (ret)
			HDMITX_ERROR("validate formatpara [%s,%s] return error %d\n",
				     mode, attr, ret);
	}

out:
	mutex_unlock(&tx_comm->hdmimode_mutex);
	return ret;
}
EXPORT_SYMBOL(hdmitx_common_validate_mode_locked);

/* init format para only when probe */
int hdmitx_common_init_bootup_format_para(struct hdmitx_common *tx_comm,
		struct hdmi_format_para *para)
{
	int ret = 0;
	struct hdmitx_hw_common *tx_hw = tx_comm->tx_hw;
	enum hdmi_tf_type dv_type;
	bool dsc_en = false;

	if (hdmitx_hw_get_state(tx_hw, STAT_TX_OUTPUT, 0)) {
		/* TODO: it has not consider VESA mode witch HW VIC = 0 */
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

		/* when already output in uboot, use uboot fmt_attr to update cs cd
		 * if dsc is enabled, as cd from HW register is 8bit under dsc
		 */
		if (hdmitx_hw_get_state(tx_hw, STAT_TX_DSC_EN, 0)) {
			hdmitx_parse_color_attr(tx_comm->fmt_attr, &para->cs, &para->cd, &para->cr);
			dsc_en = true;
		}

		ret = hdmitx_common_build_format_para(tx_comm, para, para->vic,
			tx_comm->frac_rate_policy, para->cs, para->cd,
			HDMI_QUANTIZATION_RANGE_FULL);
		if (ret == 0) {
			HDMITX_INFO("%s init ok\n", __func__);
			/* for bootup, override build format with HW state */
			para->dsc_en = dsc_en;
			para->frl_rate = hdmitx_hw_cntl_misc(tx_hw, MISC_GET_FRL_MODE, 0);
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

/* get hdp plugin sequence id */
u64 hdmitx_get_hpd_hw_sequence_id(struct hdmitx_common *tx_comm)
{
	u64 tmp_sequence_id;

	mutex_lock(&tx_comm->hdmimode_mutex);
	tmp_sequence_id = tx_comm->tx_hw->hw_sequence_id;
	mutex_unlock(&tx_comm->hdmimode_mutex);

	return tmp_sequence_id;
}
EXPORT_SYMBOL(hdmitx_get_hpd_hw_sequence_id);

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

enum hdmi_vic hdmitx_get_prefer_vic(struct hdmitx_common *tx_comm, enum hdmi_vic vic)
{
	int i = 0;
	const struct {
		u32 mode_prefer_vic;
		u32 mode_alternate_vic;
	} vic_pairs[] = {
		{HDMI_7_720x480i60_16x9, HDMI_6_720x480i60_4x3},
		{HDMI_3_720x480p60_16x9, HDMI_2_720x480p60_4x3},
		{HDMI_22_720x576i50_16x9, HDMI_21_720x576i50_4x3},
		{HDMI_18_720x576p50_16x9, HDMI_17_720x576p50_4x3},
	};

	for (i = 0; i < ARRAY_SIZE(vic_pairs); i++) {
		if (vic_pairs[i].mode_alternate_vic == vic || vic_pairs[i].mode_prefer_vic == vic) {
			/* check if mode_best_vic supported by RX */
			if (hdmitx_edid_validate_mode(&tx_comm->rxcap,
						vic_pairs[i].mode_prefer_vic))
				return vic_pairs[i].mode_prefer_vic;
			if (hdmitx_edid_validate_mode(&tx_comm->rxcap,
						vic_pairs[i].mode_alternate_vic))
				return vic_pairs[i].mode_alternate_vic;
			return HDMI_0_UNKNOWN;
		}
	}

	return vic;
}

/* check VIC supported or not with basic cs/cd
 * compare with hdmitx_common_validate_mode_locked()
 * or valid_mode_store(), it doesn't check if vic
 * is supported by hdmitx/rx or not
 */
int hdmitx_common_check_valid_para_of_vic(struct hdmitx_common *tx_comm, enum hdmi_vic vic)
{
	struct hdmi_format_para tst_para;
	enum hdmi_color_depth cd; /* cd8, cd10 or cd12 */
	enum hdmi_colorspace cs; /* 0/1/2/3: rgb/422/444/420 */
	const enum hdmi_quantization_range cr = HDMI_QUANTIZATION_RANGE_FULL; /*default to full*/
	int i = 0;

	if (vic == HDMI_0_UNKNOWN)
		return -EINVAL;

	if (hdmitx_mode_validate_y420_vic(vic)) {
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

/* get corresponding vic of mode, and will check if it's supported in EDID */
int hdmitx_common_parse_vic_in_edid(struct hdmitx_common *tx_comm, const char *mode)
{
	const struct hdmi_timing *timing;
	enum hdmi_vic prefer_vic = HDMI_0_UNKNOWN;

	/*parse by name to find default mode*/
	timing = hdmitx_mode_match_timing_name(mode);
	if (!timing || timing->vic == HDMI_0_UNKNOWN)
		return HDMI_0_UNKNOWN;

	prefer_vic = hdmitx_get_prefer_vic(tx_comm, timing->vic);

	/*check if vic supported by rx, except:480i 480p 576i 576p*/
	if (prefer_vic != HDMI_0_UNKNOWN &&
		hdmitx_edid_validate_mode(&tx_comm->rxcap, prefer_vic) == false)
		prefer_vic = HDMI_0_UNKNOWN;

	/* Dont call hdmitx_common_check_valid_para_of_vic anymore.
	 * This function used to parse user passed mode name which should already
	 * checked by hdmitx_common_check_valid_para_of_vic().
	 */

	if (prefer_vic == HDMI_0_UNKNOWN)
		HDMITX_ERROR("%s: parse mode %s vic %d\n", __func__, mode, prefer_vic);

	return prefer_vic;
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

bool hdmitx_hdr_en(struct hdmitx_hw_common *tx_hw)
{
	if (!tx_hw)
		return false;

	return (hdmitx_hw_get_state(tx_hw, STAT_TX_HDR, 0) & HDMI_HDR_TYPE) == HDMI_HDR_TYPE;
}

bool hdmitx_dv_en(struct hdmitx_hw_common *tx_hw)
{
	if (!tx_hw)
		return false;

	return (hdmitx_hw_get_state(tx_hw, STAT_TX_DV, 0) & HDMI_DV_TYPE) == HDMI_DV_TYPE;
}

bool hdmitx_hdr10p_en(struct hdmitx_hw_common *tx_hw)
{
	if (!tx_hw)
		return false;

	return (hdmitx_hw_get_state(tx_hw, STAT_TX_HDR10P, 0) & HDMI_HDR10P_TYPE) ==
		HDMI_HDR10P_TYPE;
}

int hdmitx_common_set_allm_mode(struct hdmitx_common *tx_comm, int mode)
{
	struct hdmitx_hw_common *tx_hw_base = tx_comm->tx_hw;

	/* disable ALLM HF-VSIF, recover HDMI1.4 4k VSIF if it's legacy 4k24/25/30hz */
	if (mode == 0) {
		tx_comm->allm_mode = 0;
		hdmitx_common_setup_vsif_packet(tx_comm, VT_ALLM, 0, NULL);
		if (hdmitx_edid_get_hdmi14_4k_vic(tx_comm->fmt_para.vic) > 0 &&
			!hdmitx_dv_en(tx_hw_base) &&
			!hdmitx_hdr10p_en(tx_hw_base))
			hdmitx_common_setup_vsif_packet(tx_comm, VT_HDMI14_4K, 1, NULL);
	}
	/* enable ALLM HF-VSIF, will config vic in AVI if it's legacy 4k24/25/30hz */
	if (mode == 1) {
		tx_comm->allm_mode = 1;
		hdmitx_common_setup_vsif_packet(tx_comm, VT_ALLM, 1, NULL);
		tx_comm->ct_mode = 0;
		hdmitx_hw_cntl_config(tx_hw_base, CONF_CT_MODE, SET_CT_OFF);
	}
	/* disable ALLM HF-VSIF, not recover HDMI1.4 legacy 4k VSIF */
	if (mode == -1) {
		if (tx_comm->allm_mode == 1) {
			tx_comm->allm_mode = 0;
			/* just for hdmitx20, TODO: remove later */
			hdmitx_hw_disable_packet(tx_hw_base, HDMI_PACKET_VEND);
			/* common for hdmitx20/21 to disable HF-VSIF */
			hdmitx_common_setup_vsif_packet(tx_comm, VT_ALLM, 0, NULL);
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

int hdmitx_common_edid_tracer_post_proc(struct hdmitx_common *tx_comm, struct rx_cap *prxcap)
{
	struct dv_info *dv;
	struct hdr_info *hdr;

	if (!prxcap)
		return -1;

	if (prxcap->head_err)
		hdmitx_tracer_write_event(tx_comm->tx_tracer, HDMITX_EDID_HEAD_ERROR);
	if (prxcap->chksum_err)
		hdmitx_tracer_write_event(tx_comm->tx_tracer, HDMITX_EDID_CHECKSUM_ERROR);

	dv = &prxcap->dv_info2;
	if (dv->ieeeoui == DV_IEEE_OUI && dv->block_flag == CORRECT)
		hdmitx_tracer_write_event(tx_comm->tx_tracer, HDMITX_EDID_DV_SUPPORT);

	hdr = &prxcap->hdr_info2;
	if (hdr->hdr_support > 0)
		hdmitx_tracer_write_event(tx_comm->tx_tracer, HDMITX_EDID_HDR_SUPPORT);

	if (prxcap->ieeeoui == HDMI_IEEE_OUI)
		hdmitx_tracer_write_event(tx_comm->tx_tracer, HDMITX_EDID_HDMI_DEVICE);
	else
		hdmitx_tracer_write_event(tx_comm->tx_tracer, HDMITX_EDID_DVI_DEVICE);

	return 0;
}

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
	if (hdmitx_edid_check_data_valid(tx_comm->rxcap.edid_check, tx_comm->EDID_buf) == false) {
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
	hdmitx_common_edid_tracer_post_proc(tx_comm, &tx_comm->rxcap);

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

void hdmitx_hdr_state_init(struct hdmitx_common *tx_comm)
{
	enum hdmi_tf_type hdr_type = HDMI_NONE;
	unsigned int colorimetry = 0;
	unsigned int hdr_mode = 0;
	struct hdmitx_hw_common *tx_hw_base = tx_comm->tx_hw;

	hdr_type = hdmitx_hw_get_hdr_st(tx_hw_base);
	colorimetry = hdmitx_hw_cntl_config(tx_hw_base, CONF_GET_AVI_BT2020, 0);
	/* 1:standard HDR, 2:non-standard, 3:HLG, 0:other */
	if (hdr_type == HDMI_HDR_SMPTE_2084) {
		if (colorimetry == HDMI_EXTENDED_COLORIMETRY_BT2020)
			hdr_mode = 1;
		else
			hdr_mode = 2;
	} else if (hdr_type == HDMI_HDR_HLG) {
		if (colorimetry == HDMI_EXTENDED_COLORIMETRY_BT2020)
			hdr_mode = 3;
	} else {
		hdr_mode = 0;
	}

	tx_comm->hdmi_last_hdr_mode = hdr_mode;
	tx_comm->hdmi_current_hdr_mode = hdr_mode;
}

u32 hdmitx_calc_tmds_clk(u32 pixel_freq,
	enum hdmi_colorspace cs, enum hdmi_color_depth cd)
{
	u32 tmds_clk = pixel_freq;

	if (cs == HDMI_COLORSPACE_YUV420)
		tmds_clk = tmds_clk / 2;
	if (cs != HDMI_COLORSPACE_YUV422) {
		switch (cd) {
		case COLORDEPTH_48B:
			tmds_clk *= 2;
			break;
		case COLORDEPTH_36B:
			tmds_clk = tmds_clk * 3 / 2;
			break;
		case COLORDEPTH_30B:
			tmds_clk = tmds_clk * 5 / 4;
			break;
		case COLORDEPTH_24B:
		default:
			break;
		}
	}

	return tmds_clk;
}

/* get the corresponding bandwidth of current FRL_RATE, Unit: MHz */
u32 hdmitx_get_frl_bandwidth(const enum frl_rate_enum rate)
{
	const u32 frl_bandwidth[] = {
		[FRL_NONE] = 0,
		[FRL_3G3L] = 9000,
		[FRL_6G3L] = 18000,
		[FRL_6G4L] = 24000,
		[FRL_8G4L] = 32000,
		[FRL_10G4L] = 40000,
		[FRL_12G4L] = 48000,
	};

	if (rate > FRL_12G4L)
		return 0;
	return frl_bandwidth[rate];
}

u32 hdmitx_calc_frl_bandwidth(u32 pixel_freq, enum hdmi_colorspace cs,
	enum hdmi_color_depth cd)
{
	u32 bandwidth;

	bandwidth = hdmitx_calc_tmds_clk(pixel_freq, cs, cd);

	/* bandwidth = tmds_bandwidth * 24 * 1.122 */
	bandwidth = bandwidth * 24;
	bandwidth = bandwidth * 561 / 500;

	return bandwidth;
}

/* for legacy HDMI2.0 or earlier modes, still select TMDS */
enum frl_rate_enum hdmitx_select_frl_rate(u8 *dsc_en, u8 dsc_policy, enum hdmi_vic vic,
	enum hdmi_colorspace cs, enum hdmi_color_depth cd)
{
	const struct hdmi_timing *timing;
	enum frl_rate_enum frl_rate = FRL_NONE;
	u32 tx_frl_bandwidth = 0;
	u32 tx_tmds_bandwidth = 0;

	if (!dsc_en)
		return frl_rate;
	HDMITX_DEBUG("dsc_policy %d  vic %d  cs %d  cd %d\n", dsc_policy, vic, cs, cd);
	*dsc_en = 0;
	timing = hdmitx_mode_vic_to_hdmi_timing(vic);
	if (!timing)
		return FRL_NONE;

	tx_tmds_bandwidth = hdmitx_calc_tmds_clk(timing->pixel_freq / 1000, cs, cd);
	HDMITX_DEBUG("Hactive=%d Vactive=%d Vfreq=%d TMDS_BandWidth=%d\n",
		timing->h_active, timing->v_active,
		timing->v_freq, tx_tmds_bandwidth);
	/* If the tmds bandwidth is less than 594MHz, then select the tmds mode */
	/* the HxVp48hz is new introduced in HDMI 2.1 / CEA-861-H */
	if (timing->h_active <= 4096 && timing->v_active <= 2160 &&
		timing->v_freq != 48000 && tx_tmds_bandwidth <= 594 &&
		timing->pixel_freq / 1000 < 600)
		return FRL_NONE;
	/* tx_frl_bandwidth = tmds_bandwidth * 24 * 1.122 */
	tx_frl_bandwidth = tx_tmds_bandwidth * 24;
	tx_frl_bandwidth = tx_frl_bandwidth * 561 / 500;
	for (frl_rate = FRL_3G3L; frl_rate < FRL_12G4L + 1; frl_rate++) {
		if (tx_frl_bandwidth <= hdmitx_get_frl_bandwidth(frl_rate)) {
			HDMITX_DEBUG("select frl_rate as %d\n", frl_rate);
			break;
		}
	}

#ifdef CONFIG_AMLOGIC_DSC
	/* check tx_cap outside */
	//if (!tx_hw->base.hdmi_tx_cap.dsc_capable) {
	//	HDMITX_DEBUG("%s hdmitx not support DSC\n", __func__);
	//	return frl_rate;
	//}
	/* DSC specific, automatically enable dsc if necessary */
	if (vic == HDMI_199_7680x4320p60_16x9 ||
		vic == HDMI_207_7680x4320p60_64x27) {
		if (cs == HDMI_COLORSPACE_YUV444 ||
			cs == HDMI_COLORSPACE_RGB) {
			*dsc_en = 1;
			/* note: previously spec FRL_6G4L can't work */
			frl_rate = FRL_6G4L;
			HDMITX_DEBUG_DSC("%s automatically dsc enable\n", __func__);
		} else if ((cs == HDMI_COLORSPACE_YUV420 &&
			cd == COLORDEPTH_36B) ||
			(cs == HDMI_COLORSPACE_YUV422)) {
			*dsc_en = 1;
			/* note: previously spec FRL_6G3L can't work */
			frl_rate = FRL_6G3L;
			HDMITX_DEBUG_DSC("%s automatically dsc enable\n", __func__);
		} else if (dsc_policy == 1) {
			/* for 420,8/10bit */
			/* force mode for dsc test, may need to also set manual_frl_rate */
			*dsc_en = 1;
			/* note: previously spec FRL_6G3L can't work */
			frl_rate = FRL_6G3L;
		} else {
			*dsc_en = 0;
		}
		if (*dsc_en == 1)
			HDMITX_DEBUG_DSC("forced DSC rate %d\n", frl_rate);
	} else if (vic == HDMI_198_7680x4320p50_16x9 ||
		vic == HDMI_206_7680x4320p50_64x27) {
		if (cs == HDMI_COLORSPACE_YUV444 ||
			cs == HDMI_COLORSPACE_RGB) {
			*dsc_en = 1;
			frl_rate = FRL_6G4L;
			HDMITX_DEBUG_DSC("%s automatically dsc enable\n", __func__);
		} else if ((cs == HDMI_COLORSPACE_YUV420 &&
			cd == COLORDEPTH_36B) ||
			(cs == HDMI_COLORSPACE_YUV422)) {
			*dsc_en = 1;
			frl_rate = FRL_6G3L;
			HDMITX_DEBUG_DSC("%s automatically dsc enable\n", __func__);
		} else if (dsc_policy == 1) {
			/* for 420,8/10bit */
			/* force mode for dsc test, may need to also set manual_frl_rate */
			*dsc_en = 1;
			frl_rate = FRL_6G3L;
		} else {
			*dsc_en = 0;
		}
		if (*dsc_en)
			HDMITX_DEBUG_DSC("spec recommended DSC frl rate: %d\n", frl_rate);
	} else if (vic == HDMI_195_7680x4320p25_16x9 ||
		vic == HDMI_203_7680x4320p25_64x27 ||
		vic == HDMI_194_7680x4320p24_16x9 ||
		vic == HDMI_202_7680x4320p24_64x27) {
		if ((cs == HDMI_COLORSPACE_YUV444 ||
			cs == HDMI_COLORSPACE_RGB) &&
			cd == COLORDEPTH_36B) {
			*dsc_en = 1;
			frl_rate = FRL_6G3L;
			HDMITX_DEBUG_DSC("%s automatically dsc enable\n", __func__);
		} else if (dsc_policy == 1) {
			/* force mode for test, may need to also set manual_frl_rate */
			*dsc_en = 1;
			/* for y444/rgb,8/10bit */
			if (cs == HDMI_COLORSPACE_YUV444 ||
				cs == HDMI_COLORSPACE_RGB)
				frl_rate = FRL_6G3L;
			else
				frl_rate = FRL_3G3L; //for 422/420
		} else {
			*dsc_en = 0;
		}
		if (*dsc_en)
			HDMITX_DEBUG_DSC("spec recommended DSC frl rate: %d\n", frl_rate);
	} else if (vic == HDMI_196_7680x4320p30_16x9 ||
		vic == HDMI_204_7680x4320p30_64x27) {
		if ((cs == HDMI_COLORSPACE_YUV444 ||
			cs == HDMI_COLORSPACE_RGB) &&
			cd == COLORDEPTH_36B) {
			*dsc_en = 1;
			frl_rate = FRL_6G3L;
			HDMITX_DEBUG_DSC("%s automatically dsc enable\n", __func__);
		} else if (dsc_policy == 1) {
			/* force mode for test, may need to also set manual_frl_rate */
			*dsc_en = 1;
			/* for 444/rgb,8/10bit */
			if (cs == HDMI_COLORSPACE_YUV444 ||
				cs == HDMI_COLORSPACE_RGB)
				frl_rate = FRL_6G3L;
			else /* for 422/420, note: previously spec FRL_3G3L can't work */
				frl_rate = FRL_3G3L;
		} else {
			*dsc_en = 0;
		}
		if (*dsc_en)
			HDMITX_DEBUG_DSC("forced DSC frl rate: %d\n", frl_rate);
	} else if (vic == HDMI_97_3840x2160p60_16x9 ||
		vic == HDMI_107_3840x2160p60_64x27 ||
		vic == HDMI_96_3840x2160p50_16x9 ||
		vic == HDMI_106_3840x2160p50_64x27) {
		if (dsc_policy == 1) {
			/* force mode for test, may need to also set manual_frl_rate */
			*dsc_en = 1;
			frl_rate = FRL_3G3L;
		} else {
			*dsc_en = 0;
		}
		if (*dsc_en)
			HDMITX_DEBUG_DSC("spec recommended DSC frl rate: %d\n", frl_rate);
	} else if (vic == HDMI_117_3840x2160p100_16x9 ||
		vic == HDMI_119_3840x2160p100_64x27 ||
		vic == HDMI_118_3840x2160p120_16x9 ||
		vic == HDMI_120_3840x2160p120_64x27) {
		/* need 12G4L under uncompressed format */
		if ((cs == HDMI_COLORSPACE_YUV444 ||
			cs == HDMI_COLORSPACE_RGB) &&
			cd == COLORDEPTH_36B) {
			*dsc_en = 1;
			frl_rate = FRL_6G3L;
			HDMITX_DEBUG_DSC("%s automatically dsc enable\n", __func__);
		} else if (dsc_policy == 1) {
			/* force mode for test, may need to also set manual_frl_rate */
			*dsc_en = 1;
			/* for 444/rgb,8/10bit */
			if (cs == HDMI_COLORSPACE_YUV444 ||
				cs == HDMI_COLORSPACE_RGB)
				frl_rate = FRL_6G3L;
			else /* for 422/420, note: previously spec FRL_3G3L can't work */
				frl_rate = FRL_3G3L;
		} else {
			*dsc_en = 0;
		}
		if (*dsc_en)
			HDMITX_DEBUG_DSC("spec recommended DSC frl rate: %d\n", frl_rate);
	} else {
		/* when switch mode to lower resolution, need to back to non-dsc mode */
		if (dsc_policy != 2)
			*dsc_en = 0;
	}
#endif

	return frl_rate;
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
					HDMITX_INFO("get efuse FEAT_DISABLE_HDMI_60HZ = 1\n");
					break;
				case 1:
					tx_comm->efuse_dis_output_4k = 1;
					HDMITX_INFO("get efuse FEAT_DISABLE_OUTPUT_4K = 1\n");
					break;
				case 2:
					tx_comm->efuse_dis_hdcp_tx22 = 1;
					HDMITX_INFO("get efuse FEAT_DISABLE_HDCP_TX_22 = 1\n");
					break;
				case 3:
					tx_comm->efuse_dis_hdmi_tx3d = 1;
					HDMITX_INFO("get efuse FEAT_DISABLE_HDMI_TX_3D = 1\n");
					break;
				case 4:
					tx_comm->efuse_dis_hdcp_tx14 = 1;
					HDMITX_INFO("get efuse FEAT_DISABLE_HDMI = 1\n");
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

enum hdmi_color_depth get_hdmi_colordepth(const struct vinfo_s *vinfo)
{
	enum hdmi_color_depth cd = COLORDEPTH_24B;

	if (!vinfo)
		return cd;

	if (vinfo->cd)
		cd = vinfo->cd;

	return cd;
}

bool hdmitx_edid_check_y420_support(struct rx_cap *prxcap, enum hdmi_vic vic)
{
	unsigned int i = 0;
	bool ret = false;
	const struct hdmi_timing *timing = hdmitx_mode_vic_to_hdmi_timing(vic);

	if (!timing || !prxcap)
		return false;

	if (hdmitx_mode_validate_y420_vic(vic)) {
		for (i = 0; i < Y420_VIC_MAX_NUM; i++) {
			if (prxcap->y420_vic[i]) {
				if (prxcap->y420_vic[i] == vic) {
					ret = true;
					break;
				}
			} else {
				ret = false;
				break;
			}
		}
	}

	return ret;
}

/* only check if vic is in edid */
bool hdmitx_edid_validate_mode(struct rx_cap *rxcap, u32 vic)
{
	int i = 0;
	bool edid_matched = false;

	if (!rxcap)
		return false;

	if (vic < HDMITX_VESA_OFFSET) {
		/*check cea cap*/
		for (i = 0 ; i < rxcap->VIC_count; i++) {
			if (rxcap->VIC[i] == vic) {
				edid_matched = true;
				break;
			}
		}
	} else {
		enum hdmi_vic *vesa_t = &rxcap->vesa_timing[0];
		/*check vesa mode.*/
		for (i = 0; i < VESA_MAX_TIMING && vesa_t[i]; i++) {
			if (vic == vesa_t[i]) {
				edid_matched = true;
				break;
			}
		}
	}

	return edid_matched;
}

bool hdmitx_edid_only_support_sd(struct rx_cap *prxcap)
{
	enum hdmi_vic vic;
	u32 i, j;
	bool only_support_sd = true;
	/* EDID of SL8800 equipment only support below formats */
	static enum hdmi_vic sd_fmt[] = {
		1, 3, 4, 17, 18
	};

	if (!prxcap)
		return false;

	for (i = 0; i < prxcap->VIC_count; i++) {
		vic = prxcap->VIC[i];
		for (j = 0; j < ARRAY_SIZE(sd_fmt); j++) {
			if (vic == sd_fmt[j])
				break;
		}
		if (j == ARRAY_SIZE(sd_fmt)) {
			only_support_sd = false;
			break;
		}
	}

	return only_support_sd;
}

/* When connect both hdmi and panel, here will use different parameters
 * for HDR packets send out. Before hdmitx send HDR packets, check
 * current mode is HDMI or not. specially for T7
 */
bool is_cur_hdmi_mode(void)
{
	struct vinfo_s *vinfo = NULL;

	vinfo = get_current_vinfo();
	if (vinfo && vinfo->mode == VMODE_HDMI)
		return 1;
	vinfo = get_current_vinfo2();
	if (vinfo && vinfo->mode == VMODE_HDMI)
		return 1;
	return 0;
}

#ifdef CONFIG_AMLOGIC_DSC
/* get the needed frl rate, refer to 2.1 spec table 7-37/38,
 * actually it may also need to check bpp
 */
static enum frl_rate_enum get_dsc_frl_rate(enum dsc_encode_mode dsc_mode)
{
	enum frl_rate_enum frl_rate = FRL_RATE_MAX;

	switch (dsc_mode) {
	case DSC_RGB_3840X2160_60HZ:
	case DSC_YUV444_3840X2160_60HZ:
	case DSC_YUV422_3840X2160_60HZ:
	case DSC_YUV420_3840X2160_60HZ:
	case DSC_RGB_3840X2160_50HZ:
	case DSC_YUV444_3840X2160_50HZ:
	case DSC_YUV422_3840X2160_50HZ:
	case DSC_YUV420_3840X2160_50HZ:
		frl_rate = FRL_3G3L;
		break;
	case DSC_RGB_3840X2160_120HZ:
	case DSC_YUV444_3840X2160_120HZ:
	case DSC_RGB_3840X2160_100HZ:
	case DSC_YUV444_3840X2160_100HZ:
		frl_rate = FRL_6G3L;
		break;
	case DSC_YUV422_3840X2160_120HZ:
	case DSC_YUV420_3840X2160_120HZ:
	case DSC_YUV422_3840X2160_100HZ:
	case DSC_YUV420_3840X2160_100HZ:
		frl_rate = FRL_3G3L;
		break;

	case DSC_RGB_7680X4320_60HZ:
	case DSC_YUV444_7680X4320_60HZ:
		/* 6G4L is spec recommended, but actually it can't
		 * work on board, need to work under 8G4L
		 */
		frl_rate = FRL_6G4L;
		break;
	case DSC_YUV422_7680X4320_60HZ:
	case DSC_YUV420_7680X4320_60HZ:
		/* 6G3L is spec recommended, but actually it can't
		 * work on board, need to work under 6G4L
		 */
		frl_rate = FRL_6G3L;
		break;

	case DSC_RGB_7680X4320_50HZ:
	case DSC_YUV444_7680X4320_50HZ:
		frl_rate = FRL_6G4L;
		break;
	case DSC_YUV422_7680X4320_50HZ:
	case DSC_YUV420_7680X4320_50HZ:
		frl_rate = FRL_6G3L;
		break;

	case DSC_YUV444_7680X4320_30HZ: /* bpp = 12 */
	case DSC_RGB_7680X4320_30HZ: /* bpp = 12 */
		frl_rate = FRL_6G3L;
		break;
	case DSC_YUV422_7680X4320_30HZ: /* bpp = 7.375 */
	case DSC_YUV420_7680X4320_30HZ: /* bpp = 7.375 */
		/* 3G3L is spec recommended, but actually it can't
		 * work on board, need to work under 6G3L
		 */
		frl_rate = FRL_3G3L;
		break;

	case DSC_YUV444_7680X4320_25HZ: /* bpp = 12 */
	case DSC_RGB_7680X4320_25HZ: /* bpp = 12 */
	case DSC_YUV444_7680X4320_24HZ: /* bpp = 12 */
	case DSC_RGB_7680X4320_24HZ: /* bpp = 12 */
		frl_rate = FRL_6G3L;
		break;
	case DSC_YUV422_7680X4320_25HZ: /* bpp = 7.6875 */
	case DSC_YUV420_7680X4320_25HZ: /* bpp = 7.6875 */
	case DSC_YUV422_7680X4320_24HZ: /* bpp = 7.6875 */
	case DSC_YUV420_7680X4320_24HZ: /* bpp = 7.6875 */
		frl_rate = FRL_3G3L;
		break;
	case DSC_ENCODE_MAX:
	default:
		frl_rate = FRL_RATE_MAX;
		break;
	}
	return frl_rate;
}

static bool hdmitx_check_dsc_support(struct tx_cap *hdmi_tx_cap,
		struct rx_cap *rxcap, struct hdmi_format_para *para)
{
	enum dsc_encode_mode dsc_mode = DSC_ENCODE_MAX;
	u8 dsc_slice_num = 0;
	enum frl_rate_enum dsc_frl_rate = FRL_NONE;
	u32 bytes_target = 0;
	u8 dsc_policy;

	if (!hdmi_tx_cap || !rxcap || !para)
		return false;

	/* step1: check if DSC mode is supported by SOC driver & policy */
	if (!hdmi_tx_cap->dsc_capable) {
		HDMITX_DEBUG_EDID("tx not capable of dsc\n");
		return false;
	}
	dsc_policy = hdmi_tx_cap->dsc_policy;

	dsc_mode = dsc_enc_confirm_mode(para->timing.h_active,
		para->timing.v_active, para->timing.v_freq, para->cs);

	if (dsc_mode == DSC_ENCODE_MAX) {
		HDMITX_DEBUG_EDID("dsc mode not supported!\n");
		return false;
	}
	if (dsc_policy == 0) {
		/* force not support below 12bit format temporarily */
		switch (dsc_mode) {
		/* 4k120hz */
		case DSC_RGB_3840X2160_120HZ:
		case DSC_YUV444_3840X2160_120HZ:
		/* 4k100hz */
		case DSC_RGB_3840X2160_100HZ:
		case DSC_YUV444_3840X2160_100HZ:
		/* 8k60hz */
		case DSC_RGB_7680X4320_60HZ:
		case DSC_YUV444_7680X4320_60HZ:
		/* 8k50hz */
		case DSC_RGB_7680X4320_50HZ:
		case DSC_YUV444_7680X4320_50HZ:
		/* 8k24hz */
		case DSC_RGB_7680X4320_24HZ:
		case DSC_YUV444_7680X4320_24HZ:
		/* 8k25hz */
		case DSC_RGB_7680X4320_25HZ:
		case DSC_YUV444_7680X4320_25HZ:
		/* 8k30hz */
		case DSC_RGB_7680X4320_30HZ:
		case DSC_YUV444_7680X4320_30HZ:
			if (para->cd == COLORDEPTH_36B)
				return false;
			break;
		default:
			break;
		}
	}

	/* step2: check if DSC mode is supported by RX */
	if (rxcap->dsc_1p2 == 0) {
		HDMITX_DEBUG_EDID("RX not support DSC\n");
		return false;
	}
	/* check dsc color depth cap */
	if (para->cd == COLORDEPTH_30B &&
		!rxcap->dsc_10bpc) {
		HDMITX_DEBUG_EDID("RX not support 10bpc DSC\n");
		return false;
	} else if (para->cd == COLORDEPTH_36B &&
		!rxcap->dsc_12bpc) {
		HDMITX_DEBUG_EDID("RX not support 12bpc DSC\n");
		return false;
	}
	/* check dsc color space cap */
	if (para->cs == HDMI_COLORSPACE_YUV420 &&
		!rxcap->dsc_native_420) {
		HDMITX_DEBUG_EDID("RX not support Y420 DSC\n");
		return false;
	}
	dsc_slice_num = dsc_get_slice_num(dsc_mode);
	/* slice num exceed rx cap */
	if (dsc_slice_num == 0 ||
		dsc_slice_num > dsc_max_slices_num[rxcap->dsc_max_slices]) {
		HDMITX_DEBUG_EDID("current slice num %d exceed rx cap %d\n",
			dsc_slice_num, dsc_max_slices_num[rxcap->dsc_max_slices]);
		return false;
	}
	/* note: pixel clock per slice not checked, assume
	 * it's always within rx cap
	 */
	/* check dsc frl rate within rx cap */
	dsc_frl_rate = get_dsc_frl_rate(dsc_mode);
	if (dsc_frl_rate == FRL_RATE_MAX ||
		dsc_frl_rate > rxcap->dsc_max_frl_rate ||
		dsc_frl_rate > rxcap->max_frl_rate) {
		HDMITX_DEBUG_EDID("current dsc frl rate %d exceed rx cap %d-%d\n",
			dsc_frl_rate, rxcap->dsc_max_frl_rate, rxcap->max_frl_rate);
		return false;
	}
	/* 2.1 spec table 6-56, if Bytes Target is greater than
	 * the value indicated by DSC_TotalChunkKBytes (see Sections
	 * 7.7.1 and 7.7.4.2), then the configuration is not
	 * supported with Compressed Video Transport.
	 */
	bytes_target = dsc_get_bytes_target_by_mode(dsc_mode);
	if (bytes_target > (rxcap->dsc_total_chunk_bytes + 1) * 1024) {
		HDMITX_DEBUG_EDID("bytes_target %d exceed DSC_TotalChunkKBytes %d\n",
			bytes_target, (rxcap->dsc_total_chunk_bytes + 1) * 1024);
		return false;
	}
	return true;
}
#endif

/* For some TV's EDID, there maybe exist some information ambiguous.
 * Such as EDID declare support 2160p60hz(Y444 8bit), but no valid
 * Max_TMDS_Clock2 to indicate that it can support 5.94G signal.
 */
int hdmitx_edid_validate_format_para(struct tx_cap *hdmi_tx_cap,
		struct rx_cap *prxcap, struct hdmi_format_para *para)
{
	const struct dv_info *dv;
	/* needed tmds clk bandwidth for current format */
	unsigned int calc_tmds_clk = 0;
	/* max tmds clk supported by RX */
	unsigned int rx_max_tmds_clk = 0;
	/* bandwidth needed for current FRL mode */
	u32 tx_frl_bandwidth = 0;
	/* maximum supported frl bandwidth of RX */
	u32 rx_frl_bandwidth_cap = 0;
	/* maximum supported frl bandwidth of soc */
	u32 tx_frl_bandwidth_cap = 0;
	bool must_frl_flag = 0;
	int ret = 0;
	u8 dsc_policy;

	if (!hdmi_tx_cap || !prxcap || !para)
		return -EPERM;

	dsc_policy = hdmi_tx_cap->dsc_policy;
	dv = &prxcap->dv_info;
	/* step1: check if mode + cs/cd is supported by TX */
	switch (para->timing.vic) {
	/* Note: below check for formats which should use FRL
	 * is also checked in step3, so remove
	 */
	/* case HDMI_96_3840x2160p50_16x9: */
	/* case HDMI_97_3840x2160p60_16x9: */
	/* case HDMI_101_4096x2160p50_256x135: */
	/* case HDMI_102_4096x2160p60_256x135: */
	/* case HDMI_106_3840x2160p50_64x27: */
	/* case HDMI_107_3840x2160p60_64x27: */
		/* if (para->cs == HDMI_COLORSPACE_RGB || */
		    /* para->cs == HDMI_COLORSPACE_YUV444) */
			/* if (para->cd != COLORDEPTH_24B && */
				/* (prxcap->max_frl_rate == FRL_NONE || */
				/* hdmi_tx_cap->tx_max_frl_rate == FRL_NONE)) */
				/* return -EPERM; */
		/* break; */
	case HDMI_6_720x480i60_4x3:
	case HDMI_7_720x480i60_16x9:
	case HDMI_21_720x576i50_4x3:
	case HDMI_22_720x576i50_16x9:
		if (para->cs == HDMI_COLORSPACE_YUV422)
			return -EPERM;
		break;
	/* don't support 640x480p60 */
	case HDMI_1_640x480p60_4x3:
		return -EPERM;
	default:
		break;
	}

	/* step2: DVI case, only rgb,8bit */
	if (prxcap->ieeeoui != HDMI_IEEE_OUI) {
		if (para->cd != COLORDEPTH_24B || para->cs != HDMI_COLORSPACE_RGB) {
			HDMITX_DEBUG_EDID("cs:%d, cd:%d not support by DVI sink\n",
				para->cs, para->cd);
			return -EPERM;
		}
	}

	/* step3: check TMDS/FRL bandwidth is within TX/RX cap */
	if (prxcap->Max_TMDS_Clock2) {
		rx_max_tmds_clk = prxcap->Max_TMDS_Clock2 * 5;
	} else {
		/* Default min is 74.25 / 5 */
		if (prxcap->Max_TMDS_Clock1 < 0xf)
			prxcap->Max_TMDS_Clock1 = DEFAULT_MAX_TMDS_CLK;
		rx_max_tmds_clk = prxcap->Max_TMDS_Clock1 * 5;
	}
	calc_tmds_clk = para->tmds_clk / 1000;

	/* more > 4k60 must use frl mode */
	if (para->timing.h_active > 4096 || para->timing.v_active > 2160 ||
		para->timing.v_freq == 48000 || calc_tmds_clk > 594 ||
		para->timing.pixel_freq / 1000 > 600)
		must_frl_flag = true;

	if (hdmi_tx_cap->tx_max_frl_rate == FRL_NONE) {
		/* output format need FRL while SOC not support FRL */
		if (must_frl_flag) {
			HDMITX_DEBUG_EDID("output format need FRL, while tx not support\n");
			return -EPERM;
		}
		/* tmds clk of the output format exceed TX/RX cap */
		if (calc_tmds_clk > hdmi_tx_cap->tx_max_tmds_clk) {
			HDMITX_DEBUG_EDID("output tmds clk:%d exceed tx cap: %d\n",
				calc_tmds_clk, hdmi_tx_cap->tx_max_tmds_clk);
			return -EPERM;
		}
		if (calc_tmds_clk > rx_max_tmds_clk) {
			HDMITX_DEBUG_EDID("output tmds clk:%d exceed rx cap: %d\n",
				calc_tmds_clk, rx_max_tmds_clk);
			return -EPERM;
		}
	} else {
#ifdef CONFIG_AMLOGIC_DSC
		if (dsc_policy == 1) {
			/* force enable policy */
			if (hdmitx_check_dsc_support(hdmi_tx_cap, prxcap, para))
				return 0;
		} else if (dsc_policy == 2) {
			/* for debug test */
			return 0;
		}
#endif
		if (!must_frl_flag) {
			if (calc_tmds_clk > hdmi_tx_cap->tx_max_tmds_clk) {
				HDMITX_DEBUG_EDID("output tmds clk:%d exceed tx cap: %d\n",
					calc_tmds_clk, hdmi_tx_cap->tx_max_tmds_clk);
				return -EPERM;
			}
			if (calc_tmds_clk > rx_max_tmds_clk) {
				HDMITX_DEBUG_EDID("output tmds clk:%d exceed rx cap: %d\n",
					calc_tmds_clk, rx_max_tmds_clk);
				return -EPERM;
			}
		} else {
			/* try to check if able to run under FRL mode */

			/* output format need FRL while RX not support FRL
			 * no need below check, it will be checked with rx_frl_bandwidth_cap
			 */
			if (prxcap->max_frl_rate == FRL_NONE) {
				HDMITX_DEBUG_EDID("output format need FRL, while rx not support\n");
				return -EPERM;
			}
			/* tx_frl_bandwidth = timing->pixel_freq / 1000 * 24 * 1.122 */
			tx_frl_bandwidth = hdmitx_calc_frl_bandwidth(para->timing.pixel_freq / 1000,
				para->cs, para->cd);
			tx_frl_bandwidth_cap =
				hdmitx_get_frl_bandwidth(hdmi_tx_cap->tx_max_frl_rate);
			rx_frl_bandwidth_cap = hdmitx_get_frl_bandwidth(prxcap->max_frl_rate);

			if (prxcap->dsc_1p2 == 0) {
				/* RX not support DSC */
				if (tx_frl_bandwidth > tx_frl_bandwidth_cap) {
					HDMITX_DEBUG_EDID("frl bandwitch:%d exceed tx_cap:%d\n",
						tx_frl_bandwidth, tx_frl_bandwidth_cap);
					return -EPERM;
				}
				if (tx_frl_bandwidth > rx_frl_bandwidth_cap) {
					HDMITX_DEBUG_EDID("frl bandwitch:%d exceed rx_cap:%d\n",
						tx_frl_bandwidth, rx_frl_bandwidth_cap);
					return -EPERM;
				}
			} else {
				if (tx_frl_bandwidth <= tx_frl_bandwidth_cap &&
					tx_frl_bandwidth <= rx_frl_bandwidth_cap)
					; // non-dsc bandwidth is within cap, continue check
#ifdef CONFIG_AMLOGIC_DSC
				else if (dsc_policy == 3) //forcely filter out dsc mode output
					return -EPERM;
				else if (!hdmitx_check_dsc_support(hdmi_tx_cap, prxcap, para))
					return -EPERM;
#else
				else
					return -EPERM;
#endif
			}
		}
	}

	/* step4: check color space/depth is within RX cap */
	if (para->cs == HDMI_COLORSPACE_YUV444) {
		enum hdmi_color_depth rx_y444_max_dc = COLORDEPTH_24B;
		/* Rx may not support Y444 */
		if (!(prxcap->native_Mode & (1 << 5)))
			return -EACCES;
		if (prxcap->dc_y444 && (prxcap->dc_30bit ||
					dv->sup_10b_12b_444 == 0x1))
			rx_y444_max_dc = COLORDEPTH_30B;
		if (prxcap->dc_y444 && (prxcap->dc_36bit ||
					dv->sup_10b_12b_444 == 0x2))
			rx_y444_max_dc = COLORDEPTH_36B;

		if (para->cd <= rx_y444_max_dc)
			ret = 0;
		else
			ret = -EACCES;

		return ret;
	}

	if (para->cs == HDMI_COLORSPACE_YUV422) {
		/* Rx may not support Y422 */
		if (prxcap->native_Mode & (1 << 4))
			ret = 0;
		else
			ret = -EACCES;

		return ret;
	}

	if (para->cs == HDMI_COLORSPACE_RGB) {
		enum hdmi_color_depth rx_rgb_max_dc = COLORDEPTH_24B;
		/* Always assume RX supports RGB444 */
		if (prxcap->dc_30bit || dv->sup_10b_12b_444 == 0x1)
			rx_rgb_max_dc = COLORDEPTH_30B;
		if (prxcap->dc_36bit || dv->sup_10b_12b_444 == 0x2)
			rx_rgb_max_dc = COLORDEPTH_36B;

		if (para->cd <= rx_rgb_max_dc)
			ret = 0;
		else
			ret = -EACCES;

		return ret;
	}

	if (para->cs == HDMI_COLORSPACE_YUV420) {
		ret = 0;
		if (!hdmitx_edid_check_y420_support(prxcap, para->vic))
			ret = -EACCES;
		else if (!prxcap->dc_30bit_420 && para->cd == COLORDEPTH_30B)
			ret = -EACCES;
		else if (!prxcap->dc_36bit_420 && para->cd == COLORDEPTH_36B)
			ret = -EACCES;

		return ret;
	}

	return -EACCES;
}

