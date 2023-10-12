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

int hdmitx_hw_cntl_ddc(struct hdmitx_hw_common *tx_hw,
	unsigned int cmd, unsigned long arg)
{
	return tx_hw->cntlddc(tx_hw, cmd, arg);
}

int hdmitx_hw_cntl(struct hdmitx_hw_common *tx_hw,
	unsigned int cmd, unsigned long arg)
{
	return tx_hw->cntl(tx_hw, cmd, arg);
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
	return tx_hw->calc_format_para(tx_hw, para);
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

int hdmitx_hw_set_phy(struct hdmitx_hw_common *tx_hw, int flag)
{
	int cmd = TMDS_PHY_ENABLE;

	if (flag == 0)
		cmd = TMDS_PHY_DISABLE;
	else
		cmd = TMDS_PHY_ENABLE;
	return hdmitx_hw_cntl_misc(tx_hw, MISC_TMDS_PHY_OP, cmd);
}
EXPORT_SYMBOL(hdmitx_hw_set_phy);

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

/* for legacy HDMI2.0 or earlier modes, still select TMDS */
/* TODO DSC modes */
enum frl_rate_enum hdmitx_select_frl_rate(bool dsc_en, enum hdmi_vic vic,
	enum hdmi_colorspace cs, enum hdmi_color_depth cd)
{
	const struct hdmi_timing *timing;
	enum frl_rate_enum rate = FRL_NONE;
	u32 tx_frl_bandwidth = 0;
	u32 tx_tmds_bandwidth = 0;

	pr_debug("dsc_en %d  vic %d  cs %d  cd %d\n", dsc_en, vic, cs, cd);
	timing = hdmitx_mode_vic_to_hdmi_timing(vic);
	if (!timing)
		return FRL_NONE;

	tx_tmds_bandwidth = hdmitx_calc_tmds_clk(timing->pixel_freq / 1000, cs, cd);
	pr_debug("Hactive=%d Vactive=%d Vfreq=%d TMDS_BandWidth=%d\n",
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
	for (rate = FRL_3G3L; rate < FRL_12G4L + 1; rate++) {
		if (tx_frl_bandwidth <= hdmitx_get_frl_bandwidth(rate)) {
			pr_debug("select frl_rate as %d\n", rate);
			return rate;
		}
	}

	return FRL_NONE;
}
