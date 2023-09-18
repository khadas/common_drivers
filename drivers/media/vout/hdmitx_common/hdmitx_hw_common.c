// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_hw_common.h>

int hdmitx_hw_cntl_config(struct hdmitx_hw_common *tx_hw,
	u32 cmd, u32 arg)
{
	return tx_hw->cntlconfig(tx_hw, cmd, arg);
}

int hdmitx_hw_cntl_misc(struct hdmitx_hw_common *tx_hw,
	u32 cmd, u32 arg)
{
	return tx_hw->cntlmisc(tx_hw, cmd, arg);
}

int hdmitx_hw_get_state(struct hdmitx_hw_common *tx_hw,
	u32 cmd, u32 arg)
{
	return tx_hw->getstate(tx_hw, cmd, arg);
}

int hdmitx_hw_validate_mode(struct hdmitx_hw_common *tx_hw, u32 vic)
{
	return tx_hw->validatemode(tx_hw, vic);
}

int hdmitx_hw_calc_format_para(struct hdmitx_hw_common *tx_hw,
	struct hdmi_format_para *para)
{
	return tx_hw->calcformatpara(tx_hw, para);
}

int hdmitx_hw_set_packet(struct hdmitx_hw_common *tx_hw,
	int type, unsigned char *DB, unsigned char *HB)
{
	tx_hw->setpacket(type, DB, HB);
	return 0;
}

int hdmitx_hw_disable_packet(struct hdmitx_hw_common *tx_hw,
	int type)
{
	tx_hw->disablepacket(type);
	return 0;
}

int hdmitx_hw_avmute(struct hdmitx_hw_common *tx_hw, int muteflag)
{
	return hdmitx_hw_cntl_misc(tx_hw, MISC_AVMUTE_OP, muteflag);
}

int hdmitx_hw_set_phy(struct hdmitx_hw_common *tx_hw, int flag)
{
	int cmd = TMDS_PHY_ENABLE;

	if (flag == 0)
		cmd = TMDS_PHY_DISABLE;
	else
		cmd = TMDS_PHY_ENABLE;
	return hdmitx_hw_cntl_misc(tx_hw, MISC_TMDS_PHY_OP, cmd);
}

enum hdmi_tf_type hdmitx_hw_get_hdr_st(struct hdmitx_hw_common *tx_hw)
{
	return hdmitx_hw_get_state(tx_hw, STAT_TX_HDR, 0);
}

enum hdmi_tf_type hdmitx_hw_get_dv_st(struct hdmitx_hw_common *tx_hw)
{
	return hdmitx_hw_get_state(tx_hw, STAT_TX_DV, 0);
}

enum hdmi_tf_type hdmitx_hw_get_hdr10p_st(struct hdmitx_hw_common *tx_hw)
{
	return hdmitx_hw_get_state(tx_hw, STAT_TX_HDR10P, 0);
}

u32 hdmitx_calc_frl_clk(u32 pixel_freq,
	enum hdmi_colorspace cs, enum hdmi_color_depth cd)
{
	u32 bandwidth;

	bandwidth = hdmitx_calc_tmds_clk(pixel_freq, cs, cd);

	/* bandwidth = tmds_bandwidth * 24 * 1.122 */
	bandwidth = bandwidth * 24;
	bandwidth = bandwidth * 561 / 500;

	return bandwidth;
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

