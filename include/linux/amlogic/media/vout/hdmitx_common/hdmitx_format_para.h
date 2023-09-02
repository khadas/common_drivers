/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __HDMI_FORMAT_PARA_H__
#define __HDMI_FORMAT_PARA_H__

#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_types.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_mode.h>

struct hdmi_format_para {
	enum hdmi_vic vic;
	unsigned char *name;
	unsigned char *sname;

	enum hdmi_color_depth cd; /* cd8, cd10 or cd12 */
	enum hdmi_colorspace cs; /* 0/1/2/3: rgb/422/444/420 */
	enum hdmi_quantization_range cr; /* limit, full */
	u32 frac_mode;

	struct hdmi_timing timing;

	u32 scrambler_en:1;
	u32 tmds_clk_div40:1;
	u32 tmds_clk; /* Unit: 1000 */

	u32 frl_clk;
	u32 dsc_en;
};

int hdmitx_format_para_reset(struct hdmi_format_para *para);
int hdmitx_format_para_print(struct hdmi_format_para *para);

void hdmitx_parse_color_attr(char const *attr_str,
	enum hdmi_colorspace *cs, enum hdmi_color_depth *cd,
	enum hdmi_quantization_range *cr);

u32 hdmitx_calc_tmds_clk(u32 pixel_freq,
	enum hdmi_colorspace cs, enum hdmi_color_depth cd);
u32 hdmitx_calc_frl_clk(u32 pixel_freq,
	enum hdmi_colorspace cs, enum hdmi_color_depth cd);
u32 hdmitx_get_frl_bandwidth(const enum frl_rate_enum rate);

#endif
