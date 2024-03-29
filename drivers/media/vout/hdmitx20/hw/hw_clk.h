/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __HW_ENC_CLK_CONFIG_H__
#define __HW_ENC_CLK_CONFIG_H__

#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_types.h>
#include <linux/amlogic/media/vout/vinfo.h>
#include "hdmi_tx_hw.h"
#include "common.h"

#define VID_PLL_DIV_1      0
#define VID_PLL_DIV_2      1
#define VID_PLL_DIV_3      2
#define VID_PLL_DIV_3p5    3
#define VID_PLL_DIV_3p75   4
#define VID_PLL_DIV_4      5
#define VID_PLL_DIV_5      6
#define VID_PLL_DIV_6      7
#define VID_PLL_DIV_6p25   8
#define VID_PLL_DIV_7      9
#define VID_PLL_DIV_7p5    10
#define VID_PLL_DIV_12     11
#define VID_PLL_DIV_14     12
#define VID_PLL_DIV_15     13
#define VID_PLL_DIV_2p5    14
#define VID_PLL_DIV_3p25   15

#define GROUP_MAX	8
struct hw_enc_clk_val_group {
	enum hdmi_vic group[GROUP_MAX];
	unsigned int hpll_clk_out; /* Unit: kHz */
	unsigned int od1;
	unsigned int od2; /* HDMI_CLK_TODIG */
	unsigned int od3;
	unsigned int vid_pll_div;
	unsigned int vid_clk_div;
	unsigned int hdmi_tx_pixel_div;
	unsigned int encp_div;
	unsigned int enci_div;
};

void hdmitx_set_clk(struct hdmitx_dev *hdev);
void hdmitx_set_cts_sys_clk(struct hdmitx20_hw *tx_hw);
void hdmitx_set_top_pclk(struct hdmitx20_hw *tx_hw);
void hdmitx_set_hdcp_pclk(struct hdmitx_dev *hdev);
void hdmitx_set_cts_hdcp22_clk(struct hdmitx_dev *hdev);
void hdmitx_set_hdmi_axi_clk(struct hdmitx_dev *hdev);
void hdmitx_set_sys_clk(struct hdmitx20_hw *tx_hw, unsigned char flag);
void hdmitx_disable_clk(struct hdmitx_dev *hdev);

#endif

