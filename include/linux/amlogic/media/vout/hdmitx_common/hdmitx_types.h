/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __HDMITX_TYPES_H
#define __HDMITX_TYPES_H

#include <linux/types.h>
#include <linux/hdmi.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_mode.h>
#include <linux/amlogic/media/vout/vinfo.h>

struct hdmi_format_para {
	enum hdmi_vic vic;
	unsigned char *name;
	unsigned char *sname;
	char ext_name[32];

	/*valid info*/
	enum hdmi_color_depth cd; /* cd8, cd10 or cd12 */
	enum hdmi_colorspace cs; /* 0/1/2/3: rgb/422/444/420 */
	enum hdmi_quantization_range cr; /* limit, full */
	/*valid info end*/

	unsigned int pixel_repetition_factor;
	unsigned int progress_mode:1;

	/*valid info*/
	u32 scrambler_en:1;
	u32 tmds_clk_div40:1;
	u32 tmds_clk; /* Unit: 1000 */
	/*valid info end*/

	struct hdmi_timing timing;
	struct vinfo_s hdmitx_vinfo;

	/*valid info*/
	u32 frac_mode;
	/*valid info end*/
};

#endif
