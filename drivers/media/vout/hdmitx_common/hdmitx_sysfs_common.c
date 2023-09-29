// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/amlogic/media/vout/vinfo.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_common.h>

#include "hdmitx_sysfs_common.h"

/*!!Only one instance supported.*/
static struct hdmitx_common *global_tx_common;
static struct hdmitx_hw_common *global_tx_hw;

const char *hdmitx_mode_get_timing_name(enum hdmi_vic vic);

/************************common sysfs*************************/
static ssize_t attr_show(struct device *dev,
			 struct device_attribute *attr, char *buf)
{
	int pos = 0;
	char fmt_attr[16];

	hdmitx_get_attr(global_tx_common, fmt_attr);
	pos = snprintf(buf, PAGE_SIZE, "%s\n\r", fmt_attr);

	return pos;
}

static ssize_t attr_store(struct device *dev,
		   struct device_attribute *attr,
		   const char *buf, size_t count)
{
	hdmitx_setup_attr(global_tx_common, buf);
	return count;
}

static DEVICE_ATTR_RW(attr);

static ssize_t hpd_state_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	int pos = 0;

	pos += snprintf(buf + pos, PAGE_SIZE, "%d",
		global_tx_common->hpd_state);
	return pos;
}

static DEVICE_ATTR_RO(hpd_state);

/* rawedid attr */
static ssize_t rawedid_show(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	int pos = 0;
	int i;
	int num;
	int ext_block_num = 0;

	/* prevent null prt */
	if (!global_tx_common->edid_ptr)
		global_tx_common->edid_ptr = global_tx_common->EDID_buf;

	ext_block_num = global_tx_common->edid_ptr[126];
	if (ext_block_num && global_tx_common->edid_ptr[128 + 4] == EXTENSION_EEODB_EXT_TAG &&
		global_tx_common->edid_ptr[128 + 5] == EXTENSION_EEODB_EXT_CODE)
		ext_block_num = global_tx_common->edid_ptr[128 + 6];	//EEODB
	if (ext_block_num < 8)
		num = (ext_block_num + 1) * 128;
	else
		num = 8 * 128;

	for (i = 0; i < num; i++)
		pos += snprintf(buf + pos, PAGE_SIZE, "%02x",
				global_tx_common->edid_ptr[i]);

	pos += snprintf(buf + pos, PAGE_SIZE, "\n");

	return pos;
}

static DEVICE_ATTR_RO(rawedid);

/*
 * edid_parsing attr
 * If RX edid data are all correct, HEAD(00 ff ff ff ff ff ff 00), checksum,
 * version, etc), then return "ok". Otherwise, "ng"
 * Actually, in some old televisions, EDID is stored in EEPROM.
 * some bits in EEPROM may reverse with time.
 * But it does not affect  edid_parsing.
 * Therefore, we consider the RX edid data are all correct, return "OK"
 */
static ssize_t edid_parsing_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	int pos = 0;

	if (check_dvi_hdmi_edid_valid(global_tx_common->edid_ptr))
		pos += snprintf(buf + pos, PAGE_SIZE, "ok\n");
	else
		pos += snprintf(buf + pos, PAGE_SIZE, "ng\n");

	return pos;
}

static DEVICE_ATTR_RO(edid_parsing);

static ssize_t edid_show(struct device *dev,
			 struct device_attribute *attr,
			 char *buf)
{
	return hdmitx_print_sink_cap(global_tx_common, buf, PAGE_SIZE);
}

static ssize_t edid_store(struct device *dev,
			  struct device_attribute *attr,
			  const char *buf, size_t count)
{
	u32 argn = 0;
	char *p = NULL, *para = NULL, *argv[8] = {NULL};
	u32 path_length = 0;

	p = kstrdup(buf, GFP_KERNEL);
	if (!p)
		return count;

	do {
		para = strsep(&p, " ");
		if (para) {
			argv[argn] = para;
			argn++;
			if (argn > 7)
				break;
		}
	} while (para);

	if (!strncmp(argv[0], "save", strlen("save"))) {
		u32 type = 0;

		if (argn != 3) {
			pr_info("[%s] cmd format: save bin/txt edid_file_path\n",
				__func__);
			goto PROCESS_END;
		}
		if (!strncmp(argv[1], "bin", strlen("bin")))
			type = 1;
		else if (!strncmp(argv[1], "txt", strlen("txt")))
			type = 2;

		if (type == 1 || type == 2) {
			/* clean '\n' from file path*/
			path_length = strlen(argv[2]);
			if (argv[2][path_length - 1] == '\n')
				argv[2][path_length - 1] = 0x0;

			hdmitx_save_edid_file(global_tx_common->EDID_buf, argv[2]);
		}
	} else if (!strncmp(argv[0], "load", strlen("load"))) {
		if (argn != 2) {
			pr_info("[%s] cmd format: load edid_file_path\n",
				__func__);
			goto PROCESS_END;
		}

		/* clean '\n' from file path*/
		path_length = strlen(argv[1]);
		if (argv[1][path_length - 1] == '\n')
			argv[1][path_length - 1] = 0x0;
		hdmitx_load_edid_file(argv[1]);
	}

PROCESS_END:
	kfree(p);
	return count;
}

static DEVICE_ATTR_RW(edid);

static ssize_t contenttype_cap_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	int pos = 0;
	struct rx_cap *prxcap = &global_tx_common->rxcap;

	if (prxcap->cnc0)
		pos += snprintf(buf + pos, PAGE_SIZE, "graphics\n\r");
	if (prxcap->cnc1)
		pos += snprintf(buf + pos, PAGE_SIZE, "photo\n\r");
	if (prxcap->cnc2)
		pos += snprintf(buf + pos, PAGE_SIZE, "cinema\n\r");
	if (prxcap->cnc3)
		pos += snprintf(buf + pos, PAGE_SIZE, "game\n\r");

	return pos;
}

static DEVICE_ATTR_RO(contenttype_cap);

static ssize_t _hdr_cap_show(struct device *dev,
			     struct device_attribute *attr,
			     char *buf,
			     const struct hdr_info *hdr)
{
	int pos = 0;
	unsigned int i, j;
	int hdr10plugsupported = 0;
	const struct cuva_info *cuva = &hdr->cuva_info;
	const struct hdr10_plus_info *hdr10p = &hdr->hdr10plus_info;
	const struct sbtm_info *sbtm = &hdr->sbtm_info;

	if (hdr10p->ieeeoui == HDR10_PLUS_IEEE_OUI &&
		hdr10p->application_version != 0xFF)
		hdr10plugsupported = 1;
	pos += snprintf(buf + pos, PAGE_SIZE, "HDR10Plus Supported: %d\n",
		hdr10plugsupported);
	pos += snprintf(buf + pos, PAGE_SIZE, "HDR Static Metadata:\n");
	pos += snprintf(buf + pos, PAGE_SIZE, "    Supported EOTF:\n");
	pos += snprintf(buf + pos, PAGE_SIZE, "        Traditional SDR: %d\n",
		!!(hdr->hdr_support & 0x1));
	pos += snprintf(buf + pos, PAGE_SIZE, "        Traditional HDR: %d\n",
		!!(hdr->hdr_support & 0x2));
	pos += snprintf(buf + pos, PAGE_SIZE, "        SMPTE ST 2084: %d\n",
		!!(hdr->hdr_support & 0x4));
	pos += snprintf(buf + pos, PAGE_SIZE, "        Hybrid Log-Gamma: %d\n",
		!!(hdr->hdr_support & 0x8));
	pos += snprintf(buf + pos, PAGE_SIZE, "    Supported SMD type1: %d\n",
		hdr->static_metadata_type1);
	pos += snprintf(buf + pos, PAGE_SIZE, "    Luminance Data\n");
	pos += snprintf(buf + pos, PAGE_SIZE, "        Max: %d\n",
		hdr->lumi_max);
	pos += snprintf(buf + pos, PAGE_SIZE, "        Avg: %d\n",
		hdr->lumi_avg);
	pos += snprintf(buf + pos, PAGE_SIZE, "        Min: %d\n\n",
		hdr->lumi_min);
	pos += snprintf(buf + pos, PAGE_SIZE, "HDR Dynamic Metadata:");

	for (i = 0; i < 4; i++) {
		if (hdr->dynamic_info[i].type == 0)
			continue;
		pos += snprintf(buf + pos, PAGE_SIZE,
			"\n    metadata_version: %x\n",
			hdr->dynamic_info[i].type);
		pos += snprintf(buf + pos, PAGE_SIZE,
			"        support_flags: %x\n",
			hdr->dynamic_info[i].support_flags);
		pos += snprintf(buf + pos, PAGE_SIZE,
			"        optional_fields:");
		for (j = 0; j <
			(hdr->dynamic_info[i].of_len - 3); j++)
			pos += snprintf(buf + pos, PAGE_SIZE, " %x",
				hdr->dynamic_info[i].optional_fields[j]);
	}

	pos += snprintf(buf + pos, PAGE_SIZE, "\n\ncolorimetry_data: %x\n",
		hdr->colorimetry_support);
	if (cuva->ieeeoui == CUVA_IEEEOUI) {
		pos += snprintf(buf + pos, PAGE_SIZE, "CUVA supported: 1\n");
		pos += snprintf(buf + pos, PAGE_SIZE,
			"  system_start_code: %u\n", cuva->system_start_code);
		pos += snprintf(buf + pos, PAGE_SIZE,
			"  version_code: %u\n", cuva->version_code);
		pos += snprintf(buf + pos, PAGE_SIZE,
			"  display_maximum_luminance: %u\n",
			cuva->display_max_lum);
		pos += snprintf(buf + pos, PAGE_SIZE,
			"  display_minimum_luminance: %u\n",
			cuva->display_min_lum);
		pos += snprintf(buf + pos, PAGE_SIZE,
			"  monitor_mode_support: %u\n", cuva->monitor_mode_sup);
		pos += snprintf(buf + pos, PAGE_SIZE,
			"  rx_mode_support: %u\n", cuva->rx_mode_sup);
		for (i = 0; i < (cuva->length + 1); i++)
			pos += snprintf(buf + pos, PAGE_SIZE, "%02x",
				cuva->rawdata[i]);
		pos += snprintf(buf + pos, PAGE_SIZE, "\n");
	}
	/* sbtm capability show */
	if (sbtm->sbtm_support) {
		pos += snprintf(buf + pos, PAGE_SIZE, "SBTM supported: 1\n");
		if (sbtm->max_sbtm_ver)
			pos += snprintf(buf + pos, PAGE_SIZE, "  Max_SBTM_Ver: 0x%x\n",
				sbtm->max_sbtm_ver);
		if (sbtm->grdm_support)
			pos += snprintf(buf + pos, PAGE_SIZE, "  grdm_support: 0x%x\n",
				sbtm->grdm_support);
		if (sbtm->drdm_ind)
			pos += snprintf(buf + pos, PAGE_SIZE, "  drdm_ind: 0x%x\n",
				sbtm->drdm_ind);
		if (sbtm->hgig_cat_drdm_sel)
			pos += snprintf(buf + pos, PAGE_SIZE, "  hgig_cat_drdm_sel: 0x%x\n",
				sbtm->hgig_cat_drdm_sel);
		if (sbtm->use_hgig_drdm)
			pos += snprintf(buf + pos, PAGE_SIZE, "  use_hgig_drdm: 0x%x\n",
				sbtm->use_hgig_drdm);
		if (sbtm->maxrgb)
			pos += snprintf(buf + pos, PAGE_SIZE, "  maxrgb: 0x%x\n",
				sbtm->maxrgb);
		if (sbtm->gamut)
			pos += snprintf(buf + pos, PAGE_SIZE, "  gamut: 0x%x\n",
				sbtm->gamut);
		if (sbtm->red_x)
			pos += snprintf(buf + pos, PAGE_SIZE, "  red_x: 0x%x\n",
				sbtm->red_x);
		if (sbtm->red_y)
			pos += snprintf(buf + pos, PAGE_SIZE, "  red_y: 0x%x\n",
				sbtm->red_y);
		if (sbtm->green_x)
			pos += snprintf(buf + pos, PAGE_SIZE, "  green_x: 0x%x\n",
				sbtm->green_x);
		if (sbtm->green_y)
			pos += snprintf(buf + pos, PAGE_SIZE, "  green_y: 0x%x\n",
				sbtm->green_y);
		if (sbtm->blue_x)
			pos += snprintf(buf + pos, PAGE_SIZE, "  blue_x: 0x%x\n",
				sbtm->blue_x);
		if (sbtm->blue_y)
			pos += snprintf(buf + pos, PAGE_SIZE, "  blue_y: 0x%x\n",
				sbtm->blue_y);
		if (sbtm->white_x)
			pos += snprintf(buf + pos, PAGE_SIZE, "  white_x: 0x%x\n",
				sbtm->white_x);
		if (sbtm->white_y)
			pos += snprintf(buf + pos, PAGE_SIZE, "  white_y: 0x%x\n",
				sbtm->white_y);
		if (sbtm->min_bright_10)
			pos += snprintf(buf + pos, PAGE_SIZE, "  min_bright_10: 0x%x\n",
				sbtm->min_bright_10);
		if (sbtm->peak_bright_100)
			pos += snprintf(buf + pos, PAGE_SIZE, "  peak_bright_100: 0x%x\n",
				sbtm->peak_bright_100);
		if (sbtm->p0_exp)
			pos += snprintf(buf + pos, PAGE_SIZE, "  p0_exp: 0x%x\n",
				sbtm->p0_exp);
		if (sbtm->p0_mant)
			pos += snprintf(buf + pos, PAGE_SIZE, "  p0_mant: 0x%x\n",
				sbtm->p0_mant);
		if (sbtm->peak_bright_p0)
			pos += snprintf(buf + pos, PAGE_SIZE, "  peak_bright_p0: 0x%x\n",
				sbtm->peak_bright_p0);
		if (sbtm->p1_exp)
			pos += snprintf(buf + pos, PAGE_SIZE, "  p1_exp: 0x%x\n",
				sbtm->p1_exp);
		if (sbtm->p1_mant)
			pos += snprintf(buf + pos, PAGE_SIZE, "  p1_mant: 0x%x\n",
				sbtm->p1_mant);
		if (sbtm->peak_bright_p1)
			pos += snprintf(buf + pos, PAGE_SIZE, "  peak_bright_p1: 0x%x\n",
				sbtm->peak_bright_p1);
		if (sbtm->p2_exp)
			pos += snprintf(buf + pos, PAGE_SIZE, "  p2_exp: 0x%x\n",
				sbtm->p2_exp);
		if (sbtm->p2_mant)
			pos += snprintf(buf + pos, PAGE_SIZE, "  p2_mant: 0x%x\n",
				sbtm->p2_mant);
		if (sbtm->peak_bright_p2)
			pos += snprintf(buf + pos, PAGE_SIZE, "  peak_bright_p2: 0x%x\n",
				sbtm->peak_bright_p2);
		if (sbtm->p3_exp)
			pos += snprintf(buf + pos, PAGE_SIZE, "  p3_exp: 0x%x\n",
				sbtm->p3_exp);
		if (sbtm->p3_mant)
			pos += snprintf(buf + pos, PAGE_SIZE, "  p3_mant: 0x%x\n",
				sbtm->p3_mant);
		if (sbtm->peak_bright_p3)
			pos += snprintf(buf + pos, PAGE_SIZE, "  peak_bright_p3: 0x%x\n",
				sbtm->peak_bright_p3);
	}
	return pos;
}

static ssize_t hdr_cap_show(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	int pos = 0;
	const struct hdr_info *info = &global_tx_common->rxcap.hdr_info;

	if (global_tx_common->hdr_priority == 2) {
		pos += snprintf(buf + pos, PAGE_SIZE,
			"mask rx hdr capability\n");
		return pos;
	}

	return _hdr_cap_show(dev, attr, buf, info);
}

static DEVICE_ATTR_RO(hdr_cap);

static ssize_t hdr_cap2_show(struct device *dev,
			    struct device_attribute *attr,
			    char *buf)
{
	const struct hdr_info *info2 = &global_tx_common->rxcap.hdr_info2;

	return _hdr_cap_show(dev, attr, buf, info2);
}

static DEVICE_ATTR_RO(hdr_cap2);

static ssize_t _show_dv_cap(struct device *dev,
			    struct device_attribute *attr,
			    char *buf,
			    const struct dv_info *dv)
{
	int pos = 0;
	int i;

	if (dv->ieeeoui != DV_IEEE_OUI || dv->block_flag != CORRECT) {
		pos += snprintf(buf + pos, PAGE_SIZE,
			"The Rx don't support DolbyVision\n");
		return pos;
	}
	pos += snprintf(buf + pos, PAGE_SIZE,
		"DolbyVision RX support list:\n");

	if (dv->ver == 0) {
		pos += snprintf(buf + pos, PAGE_SIZE,
			"VSVDB Version: V%d\n", dv->ver);
		pos += snprintf(buf + pos, PAGE_SIZE,
			"2160p%shz: 1\n",
			dv->sup_2160p60hz ? "60" : "30");
		pos += snprintf(buf + pos, PAGE_SIZE,
			"Support mode:\n");
		pos += snprintf(buf + pos, PAGE_SIZE,
			"  DV_RGB_444_8BIT\n");
		if (dv->sup_yuv422_12bit)
			pos += snprintf(buf + pos, PAGE_SIZE,
				"  DV_YCbCr_422_12BIT\n");
	}
	if (dv->ver == 1) {
		pos += snprintf(buf + pos, PAGE_SIZE,
			"VSVDB Version: V%d(%d-byte)\n",
			dv->ver, dv->length + 1);
		if (dv->length == 0xB) {
			pos += snprintf(buf + pos, PAGE_SIZE,
				"2160p%shz: 1\n",
				dv->sup_2160p60hz ? "60" : "30");
		pos += snprintf(buf + pos, PAGE_SIZE,
			"Support mode:\n");
		pos += snprintf(buf + pos, PAGE_SIZE,
			"  DV_RGB_444_8BIT\n");
		if (dv->sup_yuv422_12bit)
			pos += snprintf(buf + pos, PAGE_SIZE,
			"  DV_YCbCr_422_12BIT\n");
		if (dv->low_latency == 0x01)
			pos += snprintf(buf + pos, PAGE_SIZE,
				"  LL_YCbCr_422_12BIT\n");
		}

		if (dv->length == 0xE) {
			pos += snprintf(buf + pos, PAGE_SIZE,
				"2160p%shz: 1\n",
				dv->sup_2160p60hz ? "60" : "30");
			pos += snprintf(buf + pos, PAGE_SIZE,
				"Support mode:\n");
			pos += snprintf(buf + pos, PAGE_SIZE,
				"  DV_RGB_444_8BIT\n");
			if (dv->sup_yuv422_12bit)
				pos += snprintf(buf + pos, PAGE_SIZE,
				"  DV_YCbCr_422_12BIT\n");
		}
	}
	if (dv->ver == 2) {
		pos += snprintf(buf + pos, PAGE_SIZE,
			"VSVDB Version: V%d\n", dv->ver);
		pos += snprintf(buf + pos, PAGE_SIZE,
			"2160p%shz: 1\n",
			dv->sup_2160p60hz ? "60" : "30");
		pos += snprintf(buf + pos, PAGE_SIZE,
			"Parity: %d\n", dv->parity);
		pos += snprintf(buf + pos, PAGE_SIZE,
			"Support mode:\n");
		if (dv->Interface != 0x00 && dv->Interface != 0x01) {
			pos += snprintf(buf + pos, PAGE_SIZE,
				"  DV_RGB_444_8BIT\n");
			if (dv->sup_yuv422_12bit)
				pos += snprintf(buf + pos, PAGE_SIZE,
					"  DV_YCbCr_422_12BIT\n");
		}
		pos += snprintf(buf + pos, PAGE_SIZE,
			"  LL_YCbCr_422_12BIT\n");
		if (dv->Interface == 0x01 || dv->Interface == 0x03) {
			if (dv->sup_10b_12b_444 == 0x1) {
				pos += snprintf(buf + pos, PAGE_SIZE,
					"  LL_RGB_444_10BIT\n");
			}
			if (dv->sup_10b_12b_444 == 0x2) {
				pos += snprintf(buf + pos, PAGE_SIZE,
					"  LL_RGB_444_12BIT\n");
			}
		}
	}
	pos += snprintf(buf + pos, PAGE_SIZE,
		"IEEEOUI: 0x%06x\n", dv->ieeeoui);
	pos += snprintf(buf + pos, PAGE_SIZE,
		"EMP: %d\n", dv->dv_emp_cap);
	pos += snprintf(buf + pos, PAGE_SIZE, "VSVDB: ");
	for (i = 0; i < (dv->length + 1); i++)
		pos += snprintf(buf + pos, PAGE_SIZE, "%02x",
		dv->rawdata[i]);
	pos += snprintf(buf + pos, PAGE_SIZE, "\n");
	return pos;
}

static ssize_t dv_cap_show(struct device *dev,
			   struct device_attribute *attr,
			   char *buf)
{
	int pos = 0;
	const struct dv_info *dv = &global_tx_common->rxcap.dv_info;

	if (dv->ieeeoui != DV_IEEE_OUI || global_tx_common->hdr_priority) {
		pos += snprintf(buf + pos, PAGE_SIZE,
			"The Rx don't support DolbyVision\n");
		return pos;
	}
	return _show_dv_cap(dev, attr, buf, dv);
}

static DEVICE_ATTR_RO(dv_cap);

static ssize_t dv_cap2_show(struct device *dev,
			    struct device_attribute *attr,
			    char *buf)
{
	const struct dv_info *dv2 = &global_tx_common->rxcap.dv_info2;

	return _show_dv_cap(dev, attr, buf, dv2);
}

static DEVICE_ATTR_RO(dv_cap2);

static ssize_t frac_rate_policy_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf,
				size_t count)
{
	int val = 0;

	if (isdigit(buf[0])) {
		val = buf[0] - '0';
		pr_info("set frac_rate_policy as %d\n", val);
		if (val == 0 || val == 1)
			global_tx_common->frac_rate_policy = val;
		else
			pr_info("only accept as 0 or 1\n");
	}

	return count;
}

static ssize_t frac_rate_policy_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	int pos = 0;

	pos += snprintf(buf + pos, PAGE_SIZE, "%d\n",
		global_tx_common->frac_rate_policy);

	return pos;
}

static DEVICE_ATTR_RW(frac_rate_policy);

/*
 *  1: enable hdmitx phy
 *  0: disable hdmitx phy
 */
static ssize_t phy_store(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	int cmd = TMDS_PHY_ENABLE;

	pr_info("%s %s\n", __func__, buf);

	if (strncmp(buf, "0", 1) == 0)
		cmd = TMDS_PHY_DISABLE;
	else if (strncmp(buf, "1", 1) == 0)
		cmd = TMDS_PHY_ENABLE;
	else
		pr_info("set phy wrong: %s\n", buf);

	hdmitx_hw_cntl_misc(global_tx_hw, MISC_TMDS_PHY_OP, cmd);
	return count;
}

static ssize_t phy_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int pos = 0;
	int state = 0;

	state = hdmitx_hw_get_state(global_tx_hw, STAT_TX_PHY, 0);
	pos += snprintf(buf + pos, PAGE_SIZE, "%d\n", state);

	return pos;
}

static DEVICE_ATTR_RW(phy);

static ssize_t contenttype_mode_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int pos = 0;
	static char * const ct_names[] = {
		"off",
		"game",
		"graphics",
		"photo",
		"cinema",
	};

	if (global_tx_common->ct_mode < ARRAY_SIZE(ct_names))
		pos += snprintf(buf + pos, PAGE_SIZE, "%s\n\r",
					ct_names[global_tx_common->ct_mode]);

	return pos;
}

static inline int com_str(const char *buf, const char *str)
{
	return strncmp(buf, str, strlen(str)) == 0;
}

static ssize_t contenttype_mode_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	u32 ct_mode = SET_CT_OFF;

	pr_info("hdmitx: store contenttype_mode as %s\n", buf);

	if (global_tx_common->allm_mode == 1) {
		global_tx_common->allm_mode = 0;
		hdmitx_common_setup_vsif_packet(global_tx_common, VT_ALLM, 0, NULL);
	}
	hdmitx_common_setup_vsif_packet(global_tx_common, VT_HDMI14_4K, 1, NULL);

	if (com_str(buf, "0") || com_str(buf, "off"))
		ct_mode = SET_CT_OFF;
	else if (com_str(buf, "1") || com_str(buf, "game"))
		ct_mode = SET_CT_GAME;
	else if (com_str(buf, "2") || com_str(buf, "graphics"))
		ct_mode = SET_CT_GRAPHICS;
	else if (com_str(buf, "3") || com_str(buf, "photo"))
		ct_mode = SET_CT_PHOTO;
	else if (com_str(buf, "4") || com_str(buf, "cinema"))
		ct_mode = SET_CT_CINEMA;

	hdmitx_hw_cntl_config(global_tx_hw, CONF_CT_MODE, ct_mode);
	global_tx_common->ct_mode = ct_mode;

	return count;
}

static DEVICE_ATTR_RW(contenttype_mode);

static ssize_t disp_cap_show(struct device *dev,
			     struct device_attribute *attr,
			     char *buf)
{
	struct rx_cap *prxcap = &global_tx_common->rxcap;
	const struct hdmi_timing *timing = NULL;
	enum hdmi_vic vic;
	const char *mode_name;
	int i, pos = 0;
	int vic_len = prxcap->VIC_count + VESA_MAX_TIMING;
	int *edid_vics = vmalloc(vic_len * sizeof(int));

	memset(edid_vics, 0, vic_len * sizeof(int));

	/*copy edid vic list*/
	if (prxcap->VIC_count > 0)
		memcpy(edid_vics, prxcap->VIC, sizeof(int) * prxcap->VIC_count);
	for (i = 0; i < VESA_MAX_TIMING && prxcap->vesa_timing[i]; i++)
		edid_vics[prxcap->VIC_count + i] = prxcap->vesa_timing[i];

	for (i = 0; i < vic_len; i++) {
		vic = edid_vics[i];
		if (vic == HDMI_0_UNKNOWN)
			continue;

		if (vic == HDMI_2_720x480p60_4x3 ||
			vic == HDMI_6_720x480i60_4x3 ||
			vic == HDMI_17_720x576p50_4x3 ||
			vic == HDMI_21_720x576i50_4x3) {
			if (hdmitx_edid_validate_mode(prxcap, vic + 1) == 0) {
				/*pr_info("%s: check vic exist, handle [%d] later.\n",
				 *	__func__, vic + 1);
				 */
				continue;
			}
		}

		timing = hdmitx_mode_vic_to_hdmi_timing(vic);
		if (!timing) {
			// pr_err("%s: unsupport vic [%d]\n", __func__, vic);
			continue;
		}

		if (hdmitx_common_validate_vic(global_tx_common, vic) != 0) {
			// pr_err("%s: vic[%d] over range.\n", __func__, vic);
			continue;
		}

		mode_name = timing->sname ? timing->sname : timing->name;

		pos += snprintf(buf + pos, PAGE_SIZE, "%s", mode_name);
		if (vic == prxcap->native_vic)
			pos += snprintf(buf + pos, PAGE_SIZE, "*\n");
		else
			pos += snprintf(buf + pos, PAGE_SIZE, "\n");
	}

	vfree(edid_vics);
	return pos;
}

static DEVICE_ATTR_RO(disp_cap);

/* cea_cap, a clone of disp_cap */
static ssize_t cea_cap_show(struct device *dev,
			    struct device_attribute *attr,
			    char *buf)
{
	return disp_cap_show(dev, attr, buf);
}

static DEVICE_ATTR_RO(cea_cap);

static ssize_t vesa_cap_show(struct device *dev,
			     struct device_attribute *attr,
			     char *buf)
{
	int i;
	enum hdmi_vic *vesa_t = &global_tx_common->rxcap.vesa_timing[0];
	int pos = 0;

	for (i = 0; vesa_t[i] && i < VESA_MAX_TIMING; i++) {
		const struct hdmi_timing *timing = hdmitx_mode_vic_to_hdmi_timing(vesa_t[i]);

		if (timing && timing->vic >= HDMITX_VESA_OFFSET)
			pos += snprintf(buf + pos, PAGE_SIZE, "%s\n",
					timing->name);
	}
	return pos;
}

static DEVICE_ATTR_RO(vesa_cap);

static ssize_t dc_cap_show(struct device *dev,
			   struct device_attribute *attr, char *buf)
{
	int pos = 0;
	struct rx_cap *prxcap = &global_tx_common->rxcap;
	const struct dv_info *dv =  &prxcap->dv_info;
	const struct dv_info *dv2 = &prxcap->dv_info2;
	int i;

	if (prxcap->dc_36bit_420)
		pos += snprintf(buf + pos, PAGE_SIZE, "420,12bit\n");
	if (prxcap->dc_30bit_420)
		pos += snprintf(buf + pos, PAGE_SIZE, "420,10bit\n");

	for (i = 0; i < Y420_VIC_MAX_NUM; i++) {
		if (prxcap->y420_vic[i]) {
			pos += snprintf(buf + pos, PAGE_SIZE,
				"420,8bit\n");
			break;
		}
	}

	if (prxcap->native_Mode & (1 << 5)) {
		if (prxcap->dc_y444) {
			if (prxcap->dc_36bit || dv->sup_10b_12b_444 == 0x2 ||
			    dv2->sup_10b_12b_444 == 0x2)
				pos += snprintf(buf + pos, PAGE_SIZE, "444,12bit\n");
			if (prxcap->dc_30bit || dv->sup_10b_12b_444 == 0x1 ||
			    dv2->sup_10b_12b_444 == 0x1) {
				pos += snprintf(buf + pos, PAGE_SIZE, "444,10bit\n");
			}
		}
		pos += snprintf(buf + pos, PAGE_SIZE, "444,8bit\n");
	}
	/* y422, not check dc */
	if (prxcap->native_Mode & (1 << 4))
		pos += snprintf(buf + pos, PAGE_SIZE, "422,12bit\n");

	if (prxcap->dc_36bit || dv->sup_10b_12b_444 == 0x2 ||
	    dv2->sup_10b_12b_444 == 0x2)
		pos += snprintf(buf + pos, PAGE_SIZE, "rgb,12bit\n");
	if (prxcap->dc_30bit || dv->sup_10b_12b_444 == 0x1 ||
	    dv2->sup_10b_12b_444 == 0x1)
		pos += snprintf(buf + pos, PAGE_SIZE, "rgb,10bit\n");
	pos += snprintf(buf + pos, PAGE_SIZE, "rgb,8bit\n");
	return pos;
}

static DEVICE_ATTR_RO(dc_cap);

static ssize_t preferred_mode_show(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	int pos = 0;
	struct rx_cap *prxcap = &global_tx_common->rxcap;
	const char *modename =
		hdmitx_mode_get_timing_name(prxcap->preferred_mode);

	pos += snprintf(buf + pos, PAGE_SIZE, "%s\n", modename);

	return pos;
}

static DEVICE_ATTR_RO(preferred_mode);

static bool pre_process_str(const char *name, char *mode, char *attr)
{
	int i;
	char *color_format[4] = {"444", "422", "420", "rgb"};
	char *search_pos = 0;

	if (!mode || !attr)
		return false;

	for (i = 0 ; i < 4 ; i++) {
		search_pos = strstr(name, color_format[i]);
		if (search_pos)
			break;
	}
	/*no cs parsed, return error.*/
	if (!search_pos)
		return false;

	/*search remaining color_formats, if have more than one cs string, return error.*/
	i++;
	for (; i < 4 ; i++) {
		if (strstr(search_pos, color_format[i]))
			return false;
	}

	/*copy mode name;*/
	memcpy(mode, name, search_pos - name);
	/*copy attr str;*/
	memcpy(attr, search_pos, strlen(search_pos));

	//pr_info("%s parse (%s,%s) from (%s)\n", __func__, mode, attr, name);

	return true;
}

static ssize_t valid_mode_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int ret;
	bool valid_mode = false;
	char cvalid_mode[32];
	char modename[32], attrstr[32];
	struct hdmi_format_para tst_para;
	enum hdmi_vic vic = HDMI_0_UNKNOWN;

	memset(modename, 0, sizeof(modename));
	memset(attrstr, 0, sizeof(attrstr));
	memset(cvalid_mode, 0, sizeof(cvalid_mode));

	strncpy(cvalid_mode, buf, sizeof(cvalid_mode));
	cvalid_mode[31] = '\0';
	if (cvalid_mode[0])
		valid_mode = pre_process_str(cvalid_mode, modename, attrstr);

	if (valid_mode) {
		vic = hdmitx_common_parse_vic_in_edid(global_tx_common, modename);
		if (vic == HDMI_0_UNKNOWN) {
			//pr_err("parse vic fail %s\n", modename);
			valid_mode = false;
		} else {
			ret = hdmitx_common_validate_vic(global_tx_common, vic);
			if (ret != 0) {
				//pr_err("validate vic %d failed ret %d\n", vic, ret);
				valid_mode = false;
			}
		}
	}

	if (valid_mode) {
		hdmitx_parse_color_attr(attrstr, &tst_para.cs, &tst_para.cd, &tst_para.cr);
		//pr_err("parse cs %d cd %d\n", tst_para.cs, tst_para.cd);
		ret = hdmitx_common_build_format_para(global_tx_common,
			&tst_para, vic, global_tx_common->frac_rate_policy,
			tst_para.cs, tst_para.cd, tst_para.cr);
		if (ret != 0) {
			//pr_err("build format para failed %d\n", ret);
			hdmitx_format_para_reset(&tst_para);
			valid_mode = false;
		}
	}

	if (valid_mode) {
		ret = hdmitx_common_validate_format_para(global_tx_common, &tst_para);
		if (ret != 0) {
			//pr_err("validate format para failed %d\n", ret);
			valid_mode = false;
		}
	}

	if (valid_mode) {
		ret = count;
	} else {
		pr_err("hdmitx: valid_mode input:%s\n", cvalid_mode);
		ret = -1;
	}

	return ret;
}

static DEVICE_ATTR_WO(valid_mode);

static ssize_t hdmitx_cur_status_show(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	int pos = 0;

	pos = hdmitx_tracer_read_event(global_tx_common->tx_tracer,
		buf, PAGE_SIZE);
	return pos;
}

static DEVICE_ATTR_RO(hdmitx_cur_status);

static ssize_t debug_store(struct device *dev,
			   struct device_attribute *attr,
			   const char *buf, size_t count)
{
	global_tx_hw->debugfun(global_tx_hw, buf);
	return count;
}

static DEVICE_ATTR_WO(debug);

/* Indicate whether a rptx under repeater */
static ssize_t hdmi_repeater_tx_show(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	int pos = 0;

	pos += snprintf(buf + pos, PAGE_SIZE, "%d\n",
		!!global_tx_common->repeater_mode);

	return pos;
}

static DEVICE_ATTR_RO(hdmi_repeater_tx);

static ssize_t hdmi_used_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int pos = 0;

	pos += snprintf(buf + pos, PAGE_SIZE, "%d",
		global_tx_common->already_used);
	return pos;
}

static DEVICE_ATTR_RO(hdmi_used);

static ssize_t fake_plug_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d", global_tx_common->hpd_state);
}

static ssize_t fake_plug_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t count)
{
	pr_info("hdmitx: fake plug %s\n", buf);

	if (strncmp(buf, "1", 1) == 0)
		global_tx_common->hpd_state = 1;

	if (strncmp(buf, "0", 1) == 0)
		global_tx_common->hpd_state = 0;

	/*notify to drm hdmi*/
	hdmitx_common_notify_hpd_status(global_tx_common);

	return count;
}

static DEVICE_ATTR_RW(fake_plug);

static ssize_t allm_cap_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	int pos = 0;
	struct rx_cap *prxcap = &global_tx_common->rxcap;

	pos += snprintf(buf + pos, PAGE_SIZE, "%d\n\r", prxcap->allm);
	return pos;
}

static DEVICE_ATTR_RO(allm_cap);

/*
 * sink_type attr
 * sink, or repeater
 */
static ssize_t sink_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int pos = 0;

	if (!global_tx_common->hpd_state) {
		pos += snprintf(buf + pos, PAGE_SIZE, "none\n");
		return pos;
	}

	if (global_tx_common->rxcap.vsdb_phy_addr.b)
		pos += snprintf(buf + pos, PAGE_SIZE, "repeater\n");
	else
		pos += snprintf(buf + pos, PAGE_SIZE, "sink\n");

	return pos;
}

static DEVICE_ATTR_RO(sink_type);

static ssize_t support_3d_show(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	int pos = 0;

	pos += snprintf(buf + pos, PAGE_SIZE, "%d\n",
			global_tx_common->rxcap.threeD_present);
	return pos;
}

static DEVICE_ATTR_RO(support_3d);

static ssize_t allm_mode_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	int pos = 0;

	pos += snprintf(buf + pos, PAGE_SIZE, "%d\n\r",
		global_tx_common->allm_mode);

	return pos;
}

static ssize_t allm_mode_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf,
			       size_t count)
{
	int mode = 0;

	pr_info("hdmitx: store allm_mode as %s\n", buf);

	if (com_str(buf, "0"))
		mode = 0;
	else if (com_str(buf, "1"))
		mode = 1;
	else if (com_str(buf, "1"))
		mode = -1;

	hdmitx_common_set_allm_mode(global_tx_common, mode);

	return count;
}

static DEVICE_ATTR_RW(allm_mode);

static ssize_t avmute_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ret = 0;
	int pos = 0;

	ret = hdmitx_hw_cntl_misc(global_tx_hw, MISC_READ_AVMUTE_OP, 0);
	pos += snprintf(buf + pos, PAGE_SIZE, "%d", ret);

	return pos;
}

/*
 *  1: set avmute
 * -1: clear avmute
 *  0: off avmute
 */
static ssize_t avmute_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int cmd = OFF_AVMUTE;
	static int mask0;
	static int mask1;

	pr_info("%s %s\n", __func__, buf);
	if (strncmp(buf, "-1", 2) == 0) {
		cmd = CLR_AVMUTE;
		mask0 = -1;
	} else if (strncmp(buf, "0", 1) == 0) {
		cmd = OFF_AVMUTE;
		mask0 = 0;
	} else if (strncmp(buf, "1", 1) == 0) {
		cmd = SET_AVMUTE;
		mask0 = 1;
	}
	if (strncmp(buf, "r-1", 3) == 0) {
		cmd = CLR_AVMUTE;
		mask1 = -1;
	} else if (strncmp(buf, "r0", 2) == 0) {
		cmd = OFF_AVMUTE;
		mask1 = 0;
	} else if (strncmp(buf, "r1", 2) == 0) {
		cmd = SET_AVMUTE;
		mask1 = 1;
	}
	if (mask0 == 1 || mask1 == 1)
		cmd = SET_AVMUTE;
	else if ((mask0 == -1) && (mask1 == -1))
		cmd = CLR_AVMUTE;

	hdmitx_common_avmute_locked(global_tx_common, cmd, AVMUTE_PATH_1);

	return count;
}

static DEVICE_ATTR_RW(avmute);

/*********************************************************/
int hdmitx_sysfs_common_create(struct device *dev,
		struct hdmitx_common *tx_comm,
		struct hdmitx_hw_common *tx_hw)
{
	int ret = 0;

	global_tx_common = tx_comm;
	global_tx_hw = tx_hw;

	ret = device_create_file(dev, &dev_attr_attr);
	ret = device_create_file(dev, &dev_attr_hpd_state);
	ret = device_create_file(dev, &dev_attr_frac_rate_policy);

	ret = device_create_file(dev, &dev_attr_rawedid);
	ret = device_create_file(dev, &dev_attr_edid_parsing);
	ret = device_create_file(dev, &dev_attr_edid);
	ret = device_create_file(dev, &dev_attr_disp_cap);
	ret = device_create_file(dev, &dev_attr_preferred_mode);
	ret = device_create_file(dev, &dev_attr_cea_cap);
	ret = device_create_file(dev, &dev_attr_vesa_cap);
	ret = device_create_file(dev, &dev_attr_dc_cap);
	ret = device_create_file(dev, &dev_attr_valid_mode);

	ret = device_create_file(dev, &dev_attr_support_3d);
	ret = device_create_file(dev, &dev_attr_allm_cap);
	ret = device_create_file(dev, &dev_attr_contenttype_cap);
	ret = device_create_file(dev, &dev_attr_hdr_cap);
	ret = device_create_file(dev, &dev_attr_hdr_cap2);
	ret = device_create_file(dev, &dev_attr_dv_cap);
	ret = device_create_file(dev, &dev_attr_dv_cap2);
	ret = device_create_file(dev, &dev_attr_sink_type);

	ret = device_create_file(dev, &dev_attr_phy);
	ret = device_create_file(dev, &dev_attr_avmute);

	ret = device_create_file(dev, &dev_attr_contenttype_mode);
	ret = device_create_file(dev, &dev_attr_allm_mode);

	ret = device_create_file(dev, &dev_attr_hdmitx_cur_status);
	ret = device_create_file(dev, &dev_attr_hdmi_repeater_tx);
	ret = device_create_file(dev, &dev_attr_hdmi_used);

	ret = device_create_file(dev, &dev_attr_debug);
	ret = device_create_file(dev, &dev_attr_fake_plug);

	return ret;
}

int hdmitx_sysfs_common_destroy(struct device *dev)
{
	device_remove_file(dev, &dev_attr_attr);
	device_remove_file(dev, &dev_attr_hpd_state);
	device_remove_file(dev, &dev_attr_frac_rate_policy);

	device_remove_file(dev, &dev_attr_rawedid);
	device_remove_file(dev, &dev_attr_edid_parsing);
	device_remove_file(dev, &dev_attr_edid);
	device_remove_file(dev, &dev_attr_disp_cap);
	device_remove_file(dev, &dev_attr_preferred_mode);
	device_remove_file(dev, &dev_attr_cea_cap);
	device_remove_file(dev, &dev_attr_vesa_cap);
	device_remove_file(dev, &dev_attr_dc_cap);
	device_remove_file(dev, &dev_attr_valid_mode);

	device_remove_file(dev, &dev_attr_support_3d);
	device_remove_file(dev, &dev_attr_allm_cap);
	device_remove_file(dev, &dev_attr_contenttype_cap);
	device_remove_file(dev, &dev_attr_hdr_cap);
	device_remove_file(dev, &dev_attr_hdr_cap2);
	device_remove_file(dev, &dev_attr_dv_cap);
	device_remove_file(dev, &dev_attr_dv_cap2);
	device_remove_file(dev, &dev_attr_sink_type);

	device_remove_file(dev, &dev_attr_phy);
	device_remove_file(dev, &dev_attr_avmute);

	device_remove_file(dev, &dev_attr_contenttype_mode);
	device_remove_file(dev, &dev_attr_allm_mode);

	device_remove_file(dev, &dev_attr_hdmitx_cur_status);
	device_remove_file(dev, &dev_attr_hdmi_repeater_tx);
	device_remove_file(dev, &dev_attr_hdmi_used);

	device_remove_file(dev, &dev_attr_debug);
	device_remove_file(dev, &dev_attr_fake_plug);

	global_tx_common = 0;
	global_tx_hw = 0;

	return 0;
}

