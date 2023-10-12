/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __HDMI_FORMAT_PARA_H__
#define __HDMI_FORMAT_PARA_H__

#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_types.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_mode.h>
#include <linux/amlogic/media/vout/vinfo.h>

struct hdmi_format_para {
	enum hdmi_vic vic;
	unsigned char *name;
	unsigned char *sname;

	struct hdmi_timing timing;

	enum hdmi_color_depth cd; /* cd8, cd10 or cd12 */
	enum hdmi_colorspace cs; /* 0/1/2/3: rgb/422/444/420 */
	enum hdmi_quantization_range cr; /* limit, full */
	u32 frac_mode;

	/*hw related information, set in calc_format_para() func*/
	u32 scrambler_en:1;
	u32 tmds_clk_div40:1;
	u32 tmds_clk; /* Unit: 1000 */
	u32 dsc_en;
	enum frl_rate_enum frl_rate;
	/*hw related information end*/
};

int hdmitx_format_para_reset(struct hdmi_format_para *para);

/* log_buf = null, will print with printk. or will write to log_buf.
 * return is the log len.
 */
int hdmitx_format_para_print(struct hdmi_format_para *para, char *log_buf);

/* fmt_attr is still used by userspace, need rebuild this string from formatpara.
 * TODO: remove it when we delete fmt_attr sysfs.
 */
int hdmitx_format_para_rebuild_fmtattr_str(struct hdmi_format_para *para, char *attr_str, int len);

void hdmitx_parse_color_attr(char const *attr_str,
	enum hdmi_colorspace *cs, enum hdmi_color_depth *cd,
	enum hdmi_quantization_range *cr);

u32 hdmitx_calc_tmds_clk(u32 pixel_freq,
	enum hdmi_colorspace cs, enum hdmi_color_depth cd);
u32 hdmitx_get_frl_bandwidth(const enum frl_rate_enum rate);
enum frl_rate_enum hdmitx_select_frl_rate(bool dsc_en, enum hdmi_vic vic,
	enum hdmi_colorspace cs, enum hdmi_color_depth cd);

#endif
