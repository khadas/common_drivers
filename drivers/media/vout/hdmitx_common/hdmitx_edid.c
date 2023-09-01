// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_edid.h>

void phy_addr_clear(struct vsdb_phyaddr *vsdb_phy_addr)
{
	if (!vsdb_phy_addr)
		return;

	vsdb_phy_addr->a = 0;
	vsdb_phy_addr->b = 0;
	vsdb_phy_addr->c = 0;
	vsdb_phy_addr->d = 0;
	vsdb_phy_addr->valid = 0;
}

static bool hdmitx_edid_header_invalid(u8 *buf)
{
	bool base_blk_invalid = false;
	bool ext_blk_invalid = false;
	bool ret = false;
	int i = 0;

	if (buf[0] != 0 || buf[7] != 0) {
		base_blk_invalid = true;
	} else {
		for (i = 1; i < 7; i++) {
			if (buf[i] != 0xff) {
				base_blk_invalid = true;
				break;
			}
		}
	}
	/* judge header strictly, only if both header invalid */
	if (buf[0x7e] > 0) {
		if (buf[0x80] != 0x2 && buf[0x80] != 0xf0)
			ext_blk_invalid = true;
		ret = base_blk_invalid && ext_blk_invalid;
	} else {
		ret = base_blk_invalid;
	}

	return ret;
}

bool hdmitx_edid_is_all_zeros(u8 *rawedid)
{
	unsigned int i = 0, j = 0;
	unsigned int chksum = 0;

	for (j = 0; j < EDID_MAX_BLOCK; j++) {
		chksum = 0;
		for (i = 0; i < 128; i++)
			chksum += rawedid[i + j * 128];
		if (chksum != 0)
			return false;
	}
	return true;
}

/* check the first edid block */
int _check_base_structure(unsigned char *buf)
{
	unsigned int i = 0;

	/* check block 0 first 8 bytes */
	if (buf[0] != 0 && buf[7] != 0)
		return 0;

	for (i = 1; i < 7; i++) {
		if (buf[i] != 0xff)
			return 0;
	}

	if (_check_edid_blk_chksum(buf) == 0)
		return 0;

	return 1;
}

/* check the checksum for each sub block */
int _check_edid_blk_chksum(unsigned char *block)
{
	unsigned int chksum = 0;
	unsigned int i = 0;

	for (chksum = 0, i = 0; i < 0x80; i++)
		chksum += block[i];
	if ((chksum & 0xff) != 0)
		return 0;
	else
		return 1;
}

/*
 * check the EDID validity
 * base structure: header, checksum
 * extension: the first non-zero byte, checksum
 */
int check_dvi_hdmi_edid_valid(unsigned char *buf)
{
	int i;
	int blk_cnt = buf[0x7e] + 1;

	/* limit blk_cnt to EDID_MAX_BLOCK  */
	if (blk_cnt > EDID_MAX_BLOCK)
		blk_cnt = EDID_MAX_BLOCK;

	/* check block 0 */
	if (_check_base_structure(&buf[0]) == 0)
		return 0;

	if (blk_cnt == 1)
		return 1;

	/* check extension block 1 and more */
	for (i = 1; i < blk_cnt; i++) {
		if (buf[i * 0x80] == 0)
			return 0;
		if (_check_edid_blk_chksum(&buf[i * 0x80]) == 0)
			return 0;
	}

	return 1;
}

/* return 0 means valid */
int hdmitx_edid_validate(u8 *rawedid)
{
	unsigned int hdmi_ver = hdmitx_drv_ver();

	/* notify EDID NG to systemcontrol */
	/* todo: hdmi21 for tv_ts */
	if (!rawedid)
		return -EINVAL;
	if (hdmi_ver == 0) {
		return -EINVAL;
	} else if (hdmi_ver == 20) {
		if (check_dvi_hdmi_edid_valid(rawedid))
			return 0;
		else
			return -EINVAL;
	} else if (hdmi_ver == 21) {
		if (hdmitx_edid_is_all_zeros(rawedid))
			return -EINVAL;
		else if ((rawedid[0x7e] > 3) &&
			hdmitx_edid_header_invalid(rawedid))
			return -EINVAL;
		/* may extend NG case here */
	}
	return 0;
}

/*index is hdmi 1.4 vic in vsif, value is hdmi2.0 vic*/
static const u32 hdmi14_4k_vics[] = {
/* 0 - dummy*/
	0,
/* 1 - 3840x2160@30Hz */
	95,
/* 2 - 3840x2160@25Hz */
	94,
/* 3 - 3840x2160@24Hz */
	93,
/* 4 - 4096x2160@24Hz (SMPTE) */
	98,
};

u32 hdmitx_edid_get_hdmi14_4k_vic(u32 vic)
{
	bool ret = 0;
	int i;

	for (i = 0; i < ARRAY_SIZE(hdmi14_4k_vics); i++) {
		if (vic == hdmi14_4k_vics[i]) {
			ret = i;
			break;
		}
	}

	return ret;
}

bool hdmitx_edid_check_y420_support(struct rx_cap *prxcap, enum hdmi_vic vic)
{
	unsigned int i = 0;
	bool ret = false;
	const struct hdmi_timing *timing = hdmitx_mode_vic_to_hdmi_timing(vic);

	if (!timing)
		return ret;

	/* In Spec2.1 Table 7-34, greater than 2160p30hz will support y420 */
	if ((timing->v_active >= 2160 && timing->v_freq > 30000) ||
		timing->v_active >= 4320) {
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

int hdmitx_edid_validate_mode(struct rx_cap *rxcap, u32 vic)
{
	int i = 0;
	bool edid_matched = false;

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

	return edid_matched ? 0 : -EINVAL;
}

/* For some TV's EDID, there maybe exist some information ambiguous.
 * Such as EDID declare support 2160p60hz(Y444 8bit), but no valid
 * Max_TMDS_Clock2 to indicate that it can support 5.94G signal.
 */
int hdmitx_edid_validate_format_para(struct rx_cap *prxcap,
		struct hdmi_format_para *para)
{
	const struct dv_info *dv = &prxcap->dv_info;
	unsigned int calc_tmds_clk = 0;
	unsigned int rx_max_tmds_clk = 0;
	int ret = 0;

	switch (para->timing.vic) {
	case HDMI_96_3840x2160p50_16x9:
	case HDMI_97_3840x2160p60_16x9:
	case HDMI_101_4096x2160p50_256x135:
	case HDMI_102_4096x2160p60_256x135:
	case HDMI_106_3840x2160p50_64x27:
	case HDMI_107_3840x2160p60_64x27:
		if (para->cs == HDMI_COLORSPACE_RGB ||
		    para->cs == HDMI_COLORSPACE_YUV444)
			if (para->cd != COLORDEPTH_24B && !para->frl_clk)
				return -EPERM;
		break;
	case HDMI_6_720x480i60_4x3:
	case HDMI_7_720x480i60_16x9:
	case HDMI_21_720x576i50_4x3:
	case HDMI_22_720x576i50_16x9:
		if (para->cs == HDMI_COLORSPACE_YUV422)
			return -EPERM;
	default:
		break;
	}

	/* DVI case, only 8bit */
	if (prxcap->ieeeoui != HDMI_IEEE_OUI) {
		if (para->cd != COLORDEPTH_24B)
			return -EPERM;
	}

	/*FOR hdmi 2.0 TMDS: check clk limitation.*/
	/* Get RX Max_TMDS_Clock, and compare format clks*/
	if (prxcap->Max_TMDS_Clock2) {
		rx_max_tmds_clk = prxcap->Max_TMDS_Clock2 * 5;
	} else {
		/* Default min is 74.25 / 5 */
		if (prxcap->Max_TMDS_Clock1 < 0xf)
			prxcap->Max_TMDS_Clock1 = 0x1e;
		rx_max_tmds_clk = prxcap->Max_TMDS_Clock1 * 5;
	}
	calc_tmds_clk = para->tmds_clk / 1000;
	if (calc_tmds_clk > rx_max_tmds_clk)
		ret = -ERANGE;
	/* If tmds check failed, try frl*
	 * FOR hdmi 2.1 : check frl limitation.
	 */
	if (ret != 0) {
		u32 rx_frl_bandwidth = hdmitx_get_frl_bandwidth(prxcap->max_frl_rate);
		u32 tx_frl_bandwidth = para->frl_clk / 1000;

		if (tx_frl_bandwidth > 0 && rx_frl_bandwidth > 0 &&
			tx_frl_bandwidth <= rx_frl_bandwidth)
			ret = 0;
	}
	/*TDMS/FRL all failed, return;*/
	if (ret != 0)
		return -ERANGE;

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

