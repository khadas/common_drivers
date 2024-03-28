// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/printk.h>
#include <linux/delay.h>
#include <linux/arm-smccc.h>
#include "common.h"

#define MIN_HTXPLL_VCO 3000000 /* Min 3GHz */
#define MAX_HTXPLL_VCO 6000000 /* Max 6GHz */

#define WAIT_FOR_PLL_LOCKED(_reg) \
	do { \
		unsigned int st = 0; \
		int cnt = 10; \
		unsigned int reg = _reg; \
		while (cnt--) { \
			usleep_range(50, 60); \
			st = (((hd21_read_reg(reg) >> 31) & 0x1) == 1); \
			if (st) \
				break; \
			else { \
				/* reset hpll */ \
				hd21_set_reg_bits(reg, 1, 29, 1); \
				hd21_set_reg_bits(reg, 0, 29, 1); \
			} \
		} \
		if (cnt < 9) \
			HDMITX_INFO("pll[0x%x] reset %d times\n", reg, 9 - cnt);\
	} while (0)

static const char od_map[9] = {
	0, 0, 1, 0, 2, 0, 0, 0, 3,
};

void disable_hdmitx_s7_plls(struct hdmitx_dev *hdev)
{
	hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0);
	hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0);
}

/* htx pll VCO output: (3G, 6G), for tmds */
static void set_s7_htxpll_clk_other(const u32 clk, const bool frl_en)
{
	u32 quotient;
	u32 remainder;

	if (clk < 3000000 || clk >= 6000000) {
		pr_err("%s[%d] clock should be 4~6G\n", __func__, __LINE__);
		return;
	}

	quotient = clk / 24000;
	remainder = clk - quotient * 24000;
	/* remainder range: 0 ~ 23999, 0x5dbf, 15bits */
	remainder *= 1 << 17;
	remainder /= 24000;

	hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x00801000 | (quotient << 0));
	hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x2c6011c8);
	hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x86801000);
	hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x00000000 | remainder);
	hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 1, 28, 1);
	usleep_range(10, 20);
	hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL2, 1, 29, 1);
	usleep_range(10, 20);
	hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 1, 29, 1);
	hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL2, 0, 29, 1);
	usleep_range(80, 90);
	hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL1, 1, 2, 1);
	usleep_range(80, 90);
	WAIT_FOR_PLL_LOCKED(ANACTRL_HDMIPLL_CTRL0);
}

void set21_s7_htxpll_clk_out(const u32 clk, u32 div)
{
	u32 pll_od1 = 0;
	u32 pll_od10 = 0;
	u32 pll_od11 = 0;
	u32 pll_od2 = 0;
	u32 pll_od20 = 0;
	u32 pll_od21 = 0;
	u32 pll_od3 = 0;
	struct hdmitx_dev *hdev = get_hdmitx21_device();
	enum hdmi_colorspace cs = HDMI_COLORSPACE_YUV444;
	enum hdmi_color_depth cd = COLORDEPTH_24B;
	struct hdmi_format_para *para = &hdev->tx_comm.fmt_para;

	if (!hdev || !para)
		return;

	cs = para->cs;
	cd = para->cd;

	HDMITX_INFO("%s[%d] htxpll vco %d div %d\n", __func__, __LINE__, clk, div);

	if (clk <= 3000000 || clk > 6000000) {
		pr_err("%s[%d] %d out of htxpll range(3~6G]\n", __func__, __LINE__, clk);
		return;
	}
	set_s7_htxpll_clk_other(clk, hdev->frl_rate ? 1 : 0);

	//pll_od10
	if ((div % 8) == 0) {
		pll_od10 = 3; //div8
		div = div / 8;
	} else if ((div % 4) == 0) {
		pll_od10 = 2; //div4
		div = div / 4;
	} else if ((div % 2) == 0) {
		pll_od10 = 1; //div2
		div = div / 2;
	}

	//pll_od11
	if ((div % 8) == 0) {
		pll_od11 = 3;
		div = div / 8;
	} else if ((div % 4) == 0) {
		pll_od11 = 2;
		div = div / 4;
	} else if ((div % 2) == 0) {
		pll_od11 = 1;
		div = div / 2;
	}

	//pll_od1
	pll_od1 = (pll_od10 << 2) | pll_od11;

	//pll_od20 for clk to phy
	if ((div % 4) == 0) {
		pll_od20 = 2;
		div = div / 4;
	} else if ((div % 2) == 0) {
		pll_od20 = 1;
		div = div / 2;
	}

	//pll_od21 for clk_out2
	if (cs == HDMI_COLORSPACE_YUV420)
		pll_od21 = pll_od20;
	else
		pll_od21 = pll_od20 + 1;

	pll_od2 = (pll_od20 << 2) | pll_od21;

	//pll_od3
	if (cd == COLORDEPTH_24B)
		pll_od3 = 0;//pll_div3 = 5;
	else if (cd == COLORDEPTH_30B)
		pll_od3 = 1;//pll_div3 = 6.25;
	else if (cd == COLORDEPTH_36B)
		pll_od3 = 2;//pll_div3 = 7.5;

	hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 1, 19, 1);
	HDMITX_INFO("pll_od1 = %d, pll_od2 = %d, pll_od3 = %d\n",
		pll_od1, pll_od2, pll_od3);
	if (hdev->tx_hw.s7_clk_config)
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, pll_od3, 9, 2);
	hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL2, pll_od2, 15, 4);
	hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL2, pll_od1, 19, 4);

}

void hdmitx_s7_phy_pre_init(struct hdmitx_dev *hdev)
{
}

void hdmitx21_phy_bandgap_en_s7(void)
{
}

void set21_phy_by_mode_s7(u32 mode)
{
	switch (mode) {
	case HDMI_PHYPARA_6G: /* 5.94/4.5/3.7Gbps */
	case HDMI_PHYPARA_4p5G:
	case HDMI_PHYPARA_3p7G:
		hd21_write_reg(ANACTRL_HDMIPHY_CTRL0, 0xa003a0ea);
		hd21_write_reg(ANACTRL_HDMIPHY_CTRL5, 0x555);
		hd21_write_reg(ANACTRL_HDMIPHY_CTRL3, 0x004ef001);
		break;
	case HDMI_PHYPARA_3G: /* 2.97Gbps */
		hd21_write_reg(ANACTRL_HDMIPHY_CTRL0, 0xa00380a4);
		hd21_write_reg(ANACTRL_HDMIPHY_CTRL5, 0x555);
		hd21_write_reg(ANACTRL_HDMIPHY_CTRL3, 0x004ef001);
		break;
	case HDMI_PHYPARA_270M: /* 1.485Gbps, and below */
	case HDMI_PHYPARA_DEF:
	default:
		hd21_write_reg(ANACTRL_HDMIPHY_CTRL0, 0xa2238080);
		hd21_write_reg(ANACTRL_HDMIPHY_CTRL5, 0x555);
		hd21_write_reg(ANACTRL_HDMIPHY_CTRL3, 0x004ef001);
		break;
	}
	/* The bit with resetn is configured later than other bits. */
	usleep_range(100, 110);
	hd21_set_reg_bits(ANACTRL_HDMIPHY_CTRL3, 3, 10, 2);
	hd21_set_reg_bits(ANACTRL_HDMIPHY_CTRL3, 3, 3, 2);
	hd21_set_reg_bits(ANACTRL_HDMIPHY_CTRL3, 1, 1, 1);
	/* finally config bit[29:28] */
	usleep_range(1000, 1010);
	hd21_set_reg_bits(ANACTRL_HDMIPHY_CTRL3, 3, 28, 2);
}

void hdmitx21_sys_reset_s7(void)
{
	/* Refer to system-Registers.docx */
	hd21_write_reg(RESETCTRL_RESET0, 1 << 29); /* hdmi_tx */
	hd21_write_reg(RESETCTRL_RESET0, 1 << 22); /* hdmitxphy */
	hd21_write_reg(RESETCTRL_RESET0, 1 << 19); /* vid_pll_div */
	hd21_write_reg(RESETCTRL_RESET0, 1 << 16); /* hdmitx_apb */
}

void set21_hpll_sspll_s7(enum hdmi_vic vic)
{
	switch (vic) {
	case HDMI_16_1920x1080p60_16x9:
	case HDMI_31_1920x1080p50_16x9:
	case HDMI_4_1280x720p60_16x9:
	case HDMI_19_1280x720p50_16x9:
	case HDMI_5_1920x1080i60_16x9:
	case HDMI_20_1920x1080i50_16x9:
		/* enable ssc, need update electric to 0x0100 */
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL1, 4, 25, 4);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL2, 2, 3, 4);//set ssc 1000 ppm
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL2, 1, 2, 1);//enable ssc
		break;
	default:
		break;
	}
}

/* CLKCTRL_HTX_CLK_CTRL0 bit8 gate for cts_hdmitx_prif_clk
 * it's necessary for register access of controller
 * CLKCTRL_HTX_CLK_CTRL0 bit24 gate for cts_hdmitx_200m_clk
 * it's necessary for i2c clk
 * CLKCTRL_HDMI_CLK_CTRL bit8 gate for cts_hdmitx_sys_clk
 * it's necessary for register access of hdmitx top
 */
static int gate_bit_mask = 0x01c7f;
module_param(gate_bit_mask, int, 0644);
MODULE_PARM_DESC(gate_bit_mask, "for gate_bit_mask");

void hdmitx_s7_clock_gate_ctrl(struct hdmitx_dev *hdev, bool en)
{
	HDMITX_INFO("hdmitx_s7_clock_gate %d\n", en);
	if (gate_bit_mask & BIT(1))
		hd21_set_reg_bits(CLKCTRL_VID_PLL_CLK0_DIV, en, 19, 1);
	if (gate_bit_mask & BIT(2))
		hd21_set_reg_bits(CLKCTRL_ENC_HDMI_CLK_CTRL, en, 4, 1);
	if (gate_bit_mask & BIT(3))
		hd21_set_reg_bits(CLKCTRL_ENC_HDMI_CLK_CTRL, en, 20, 1);
	if (gate_bit_mask & BIT(4))
		hd21_set_reg_bits(CLKCTRL_ENC_HDMI_CLK_CTRL, en, 12, 1);
	if (gate_bit_mask & BIT(5))
		hd21_set_reg_bits(CLKCTRL_VID_CLK0_CTRL2, en, 3, 1);
	if (gate_bit_mask & BIT(6))
		hd21_set_reg_bits(CLKCTRL_HTX_CLK_CTRL1, en, 8, 1);
	if (gate_bit_mask & BIT(7))
		hd21_set_reg_bits(CLKCTRL_HTX_CLK_CTRL0, en, 24, 1);
	if (gate_bit_mask & BIT(8))
		hd21_set_reg_bits(CLKCTRL_HTX_CLK_CTRL0, en, 8, 1);
	if (gate_bit_mask & BIT(9))
		hd21_set_reg_bits(CLKCTRL_HDMI_CLK_CTRL, en, 8, 1);

	if (gate_bit_mask & BIT(10)) {/* this will enable during the mode setting */
		hd21_write_reg(ANACTRL_HDMIPHY_CTRL0, 0x0);
		hd21_write_reg(ANACTRL_HDMIPHY_CTRL5, 0x0);
	}
	if (gate_bit_mask & BIT(11)) {// power off need
		hd21_write_reg(ANACTRL_HDMIPHY_CTRL3, 0x304efc1b);
		hd21_write_reg(ANACTRL_HDMIPHY_CTRL3, 0xc1b);
	}
	usleep_range(1, 10);
	if (gate_bit_mask & BIT(12)) /* this will enable during the pll setting */
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x0);
}

