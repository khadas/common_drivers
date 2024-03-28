// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_format_para.h>
#include "hdmitx_log.h"

static struct parse_cd parse_cd_[] = {
	{COLORDEPTH_24B, "8bit",},
	{COLORDEPTH_30B, "10bit"},
	{COLORDEPTH_36B, "12bit"},
	{COLORDEPTH_48B, "16bit"},
};

static struct parse_cs parse_cs_[] = {
	{HDMI_COLORSPACE_RGB, "rgb",},
	{HDMI_COLORSPACE_YUV422, "422",},
	{HDMI_COLORSPACE_YUV444, "444",},
	{HDMI_COLORSPACE_YUV420, "420",},
};

static struct parse_cr parse_cr_[] = {
	{HDMI_QUANTIZATION_RANGE_LIMITED, "limit",},
	{HDMI_QUANTIZATION_RANGE_FULL, "full",},
};

/* parse the string from "hdmitx output FORMAT" */
void hdmitx_parse_color_attr(char const *attr_str,
	enum hdmi_colorspace *cs, enum hdmi_color_depth *cd,
	enum hdmi_quantization_range *cr)
{
	int i;

	/* parse color depth */
	for (i = 0; i < sizeof(parse_cd_) / sizeof(struct parse_cd); i++) {
		if (strstr(attr_str, parse_cd_[i].name)) {
			*cd = parse_cd_[i].cd;
			break;
		}
	}
	/* set default value */
	if (i == sizeof(parse_cd_) / sizeof(struct parse_cd))
		*cd = COLORDEPTH_24B;

	/* parse color space */
	for (i = 0; i < sizeof(parse_cs_) / sizeof(struct parse_cs); i++) {
		if (strstr(attr_str, parse_cs_[i].name)) {
			*cs = parse_cs_[i].cs;
			break;
		}
	}
	/* set default value */
	if (i == sizeof(parse_cs_) / sizeof(struct parse_cs))
		*cs = HDMI_COLORSPACE_YUV444;

	/* parse color range */
	for (i = 0; i < sizeof(parse_cr_) / sizeof(struct parse_cr); i++) {
		if (strstr(attr_str, parse_cr_[i].name)) {
			*cr = parse_cr_[i].cr;
			break;
		}
	}
	/* set default value */
	if (i == sizeof(parse_cr_) / sizeof(struct parse_cr))
		*cr = HDMI_QUANTIZATION_RANGE_FULL;
}

int hdmitx_format_para_init(struct hdmi_format_para *para,
		enum hdmi_vic vic, u32 frac_rate_policy,
		enum hdmi_colorspace cs, enum hdmi_color_depth cd,
		enum hdmi_quantization_range cr)
{
	int ret = 0;
	const struct hdmi_timing *timing =
		hdmitx_mode_vic_to_hdmi_timing(vic);

	if (!timing) {
		HDMITX_ERROR("%s got unknown vic %d\n", __func__, vic);
		return -EINVAL;
	}

	/*reset to default value*/
	hdmitx_format_para_reset(para);

	para->timing = *timing;
	para->vic = timing->vic;
	para->name = timing->name;
	para->sname = timing->sname;
	para->tmds_clk = timing->pixel_freq;
	para->cs = cs;
	para->cd = cd;
	para->cr = cr;

	/*check fraction mode, and update pixel_freq*/
	ret = hdmitx_mode_update_timing(&para->timing, frac_rate_policy);
	if (ret < 0)
		para->frac_mode = 0;
	else
		para->frac_mode = frac_rate_policy;

	return 0;
}

int hdmitx_format_para_reset(struct hdmi_format_para *para)
{
	memset(para, 0, sizeof(struct hdmi_format_para));
	para->vic = HDMI_0_UNKNOWN;
	para->name = "invalid";
	para->sname = "invalid";
	para->cs = HDMI_COLORSPACE_RESERVED4;
	para->cd = COLORDEPTH_RESERVED;
	para->cr = HDMI_QUANTIZATION_RANGE_RESERVED;
	para->frl_rate = FRL_NONE;
	para->dsc_en = false;

	return 0;
}

int hdmitx_format_para_print(struct hdmi_format_para *para, char *log_buf)
{
	char buf[256];
	const char *conf;
	ssize_t pos = 0;
	int len = sizeof(buf);
	int i = 0;

	if (para->vic == HDMI_0_UNKNOWN) {
		pos += snprintf(buf + pos, len - pos, "format_para: [INVALID] %px vic [0]", para);
	} else {
		pos += snprintf(buf + pos, len - pos, "format_para: %px vic [%d]\n",
					para, para->vic);
		pos += snprintf(buf + pos, len - pos, "format_para: name %s frac %d\n",
			para->sname ? para->sname : para->name, para->frac_mode);

		conf = NULL;
		for (i = 0; i < sizeof(parse_cs_) / sizeof(struct parse_cs); i++) {
			if (para->cs == parse_cs_[i].cs) {
				conf = parse_cs_[i].name;
				break;
			}
		}
		if (!conf)
			conf = "reserved";
		pos += snprintf(buf + pos, len - pos, "format_para: colorspace: %s, ", conf);

		conf = NULL;
		for (i = 0; i < sizeof(parse_cd_) / sizeof(struct parse_cd); i++) {
			if (para->cd == parse_cd_[i].cd) {
				conf = parse_cd_[i].name;
				break;
			}
		}
		if (!conf)
			conf = "reserved";
		pos += snprintf(buf + pos, len - pos, "colordepth: %s\n", conf);

		pos += snprintf(buf + pos, len - pos, "format_para: TMDS %d DIV40 %d,%d\n",
			para->tmds_clk, para->tmds_clk_div40, para->scrambler_en);

		pos += snprintf(buf + pos, len - pos, "format_para: frl_rate %d, dsc_en: %d\n",
			para->frl_rate, para->dsc_en);
	}

	if (log_buf)
		sprintf(log_buf, "%s", buf);
	else
		HDMITX_DEBUG("%s", buf);

	return pos;
}

int hdmitx_format_para_rebuild_fmtattr_str(struct hdmi_format_para *para, char *attr_str, int len)
{
	int i = 0;
	const char *conf;
	ssize_t pos = 0;

	attr_str[0] = 0;
	conf = NULL;
	for (i = 0; i < sizeof(parse_cs_) / sizeof(struct parse_cs); i++) {
		if (para->cs == parse_cs_[i].cs) {
			conf = parse_cs_[i].name;
			break;
		}
	}

	if (!conf) {
		HDMITX_ERROR("UNKNOWN cs %d\n", para->cs);
		attr_str[0] = 0;
		return -EINVAL;
	}

	pos += snprintf(attr_str + pos, len - pos, "%s,", conf);

	conf = NULL;
	for (i = 0; i < sizeof(parse_cd_) / sizeof(struct parse_cd); i++) {
		if (para->cd == parse_cd_[i].cd) {
			conf = parse_cd_[i].name;
			break;
		}
	}

	if (!conf) {
		HDMITX_ERROR("UNKNOWN cd %d\n", para->cd);
		attr_str[0] = 0;
		return -EINVAL;
	}

	pos += snprintf(attr_str + pos, len - pos, "%s", conf);

	HDMITX_DEBUG("rebuild fmt_string %s from (%d,%d)\n", attr_str, para->cs, para->cd);
	return 0;
}

