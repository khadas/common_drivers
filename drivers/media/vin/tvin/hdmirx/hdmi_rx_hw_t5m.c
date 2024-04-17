// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/mm.h>
#include <linux/major.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/arm-smccc.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/highmem.h>

/* Local Include */
#include "hdmi_rx_repeater.h"
#include "hdmi_rx_drv.h"
#include "hdmi_rx_hw.h"
#include "hdmi_rx_wrapper.h"
#include "hdmi_rx_hw_t5m.h"

/* FT trim flag:1-valid, 0-not valid */
bool rterm_trim_flag_t5m;
/* FT trim value 4 bits */
u32 rterm_trim_val_t5m;
/* for T5m */
static const u32 phy_misc_t5m[][2] = {
		/*  0x18	0x1c	*/
	{	 /* 24~35M */
		0xffe000c0, 0x11c73003,
	},
	{	 /* 37~75M */
		0xffe000c0, 0x11c73003,
	},
	{	 /* 75~115M */
		0xffe00080, 0x11c73002,
	},
	{	 /* 1155~150M */
		0xffe00080, 0x11c73002,
	},
	{	 /* 150~340M */
		0xffe00040, 0x11c73001,
	},
	{	 /* 340~525M */
		0xffe00000, 0x11c73000,
	},
	{	 /* 525~600M */
		0xffe00000, 0x11c73000,
	},
};

static const u32 phy_dcha_t5m[][2] = {
		 /* 0x08	 0x0c*/
		/* some bits default close,reopen when pll stable */
	{	 /* 24~45M */
		0x00f77ccc, 0x40100c59,
	},
	{	 /* 35~75M */
		0x00f77666, 0x40100c59,
	},
	{	 /* 75~115M */
		0x00f77666, 0x40100459,
	},
	{	 /* 115~150M */
		0x00f77666, 0x40100459,
	},
	{	 /* 150~340M */
		0x00f77666, 0x7ff00459,
	},
	{	 /* 340~525M */
		0x00f73666, 0x7ff00459,
	},
	{	 /* 525~600M */
		0x02821666, 0x7ff00459,
	},
};

static const u32 phy_dchd_t5m[][2] = {
		/*  0x10	 0x14 */
		/* some bits default close,reopen when pll stable */
		/* 0x10:12,13,14,15;0x14:12,13,16,17 */
	{	 /* 24~35M */
		0x04000586, 0x30880060,
	},
	{	 /* 35~75M */
		0x04000095, 0x30880060,
	},
	{	 /* 75~115M */
		0x04000095, 0x30880065,
	},
	{	 /* 115~150M */
		0x04000095, 0x30880069,
	},
	{	 /* 140~340M */
		0x04080093, 0x30880469,
	},
	{	 /* 340~525M */
		0x04080091, 0x30e00469,
	},
	{	 /* 525~600M */
		0x04080091, 0x30e0046f,
	},
};

u32 t5m_rlevel[] = {8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7};

/*Decimal to Gray code */
unsigned int decimaltogray_t5m(unsigned int x)
{
	return x ^ (x >> 1);
}

/* Gray code to Decimal */
unsigned int graytodecimal_t5m(unsigned int x)
{
	unsigned int y = x;

	while (x >>= 1)
		y ^= x;
	return y;
}

bool is_pll_lock_t5m(void)
{
	return ((hdmirx_rd_amlphy(T5M_HDMIRX20PLL_CTRL0) >> 31) & 0x1);
}

void t5m_480p_pll_cfg(void)
{
	u8 port = rx_info.main_port;
	/* the times of pll = 80 for debug */
//	hdmirx_wr_amlphy(T5M_RG_RX20PLL_0, 0x05305000);
//	usleep_range(10, 20);
//	hdmirx_wr_amlphy(T5M_RG_RX20PLL_1, 0x01481236);
//	usleep_range(10, 20);
//	hdmirx_wr_amlphy(T5M_RG_RX20PLL_0, 0x05305001);
//	usleep_range(10, 20);
//	hdmirx_wr_amlphy(T5M_RG_RX20PLL_0, 0x05305003);
//	usleep_range(10, 20);
//	hdmirx_wr_amlphy(T5M_RG_RX20PLL_1, 0x01401236);
//	usleep_range(10, 20);
//	hdmirx_wr_amlphy(T5M_RG_RX20PLL_0, 0x05305007);
//	usleep_range(10, 20);
//	hdmirx_wr_amlphy(T5M_RG_RX20PLL_0, 0x45305007);
//	usleep_range(10, 20);
	/*the times of pll = 160 */
	hdmirx_wr_amlphy(T5M_RG_RX20PLL_0, 0x0530a000);
	usleep_range(10, 20);
	hdmirx_wr_amlphy(T5M_RG_RX20PLL_1, 0x01481236);
	usleep_range(10, 20);
	hdmirx_wr_amlphy(T5M_RG_RX20PLL_0, 0x0530a001);
	usleep_range(10, 20);
	hdmirx_wr_amlphy(T5M_RG_RX20PLL_0, 0x0530a003);
	usleep_range(10, 20);
	hdmirx_wr_amlphy(T5M_RG_RX20PLL_1, 0x21401236);
	usleep_range(10, 20);
	hdmirx_wr_amlphy(T5M_RG_RX20PLL_0, 0x0530a007);
	usleep_range(10, 20);
	hdmirx_wr_amlphy(T5M_RG_RX20PLL_0, 0x4530a007);
	usleep_range(10, 20);
	rx[port].phy.aud_div = 3;
}

void t5m_720p_pll_cfg(void)
{
	u8 port = rx_info.main_port;

	hdmirx_wr_amlphy(T5M_RG_RX20PLL_0, 0x05305000);
	usleep_range(10, 20);
	hdmirx_wr_amlphy(T5M_RG_RX20PLL_1, 0x01481236);
	usleep_range(10, 20);
	hdmirx_wr_amlphy(T5M_RG_RX20PLL_0, 0x05305001);
	usleep_range(10, 20);
	hdmirx_wr_amlphy(T5M_RG_RX20PLL_0, 0x05305003);
	usleep_range(10, 20);
	hdmirx_wr_amlphy(T5M_RG_RX20PLL_1, 0x61401236);
	usleep_range(10, 20);
	hdmirx_wr_amlphy(T5M_RG_RX20PLL_0, 0x05305007);
	usleep_range(10, 20);
	hdmirx_wr_amlphy(T5M_RG_RX20PLL_0, 0x45305007);
	usleep_range(10, 20);
	rx[port].phy.aud_div = 0;
}

void t5m_1080p_pll_cfg(void)
{
	u8 port = rx_info.main_port;

	hdmirx_wr_amlphy(T5M_RG_RX20PLL_0, 0x05302800);
	usleep_range(10, 20);
	hdmirx_wr_amlphy(T5M_RG_RX20PLL_1, 0x01481236);
	usleep_range(10, 20);
	hdmirx_wr_amlphy(T5M_RG_RX20PLL_0, 0x05302801);
	usleep_range(10, 20);
	hdmirx_wr_amlphy(T5M_RG_RX20PLL_0, 0x05302803);
	usleep_range(10, 20);
	hdmirx_wr_amlphy(T5M_RG_RX20PLL_1, 0x41401236);
	usleep_range(10, 20);
	hdmirx_wr_amlphy(T5M_RG_RX20PLL_0, 0x05302807);
	usleep_range(10, 20);
	hdmirx_wr_amlphy(T5M_RG_RX20PLL_0, 0x45302807);
	usleep_range(10, 20);
	rx[port].phy.aud_div = 0;
}

void t5m_4k30_pll_cfg(void)
{
	u8 port = rx_info.main_port;

	hdmirx_wr_amlphy(T5M_RG_RX20PLL_0, 0x05302810);
	usleep_range(10, 20);
	hdmirx_wr_amlphy(T5M_RG_RX20PLL_1, 0x01481236);
	usleep_range(10, 20);
	hdmirx_wr_amlphy(T5M_RG_RX20PLL_0, 0x05302811);
	usleep_range(10, 20);
	hdmirx_wr_amlphy(T5M_RG_RX20PLL_0, 0x05302813);
	usleep_range(10, 20);
	hdmirx_wr_amlphy(T5M_RG_RX20PLL_1, 0x21401236);
	usleep_range(10, 20);
	hdmirx_wr_amlphy(T5M_RG_RX20PLL_0, 0x05302817);
	usleep_range(10, 20);
	hdmirx_wr_amlphy(T5M_RG_RX20PLL_0, 0x45302817);
	usleep_range(10, 20);
	rx[port].phy.aud_div = 0;
}

void t5m_4k60_pll_cfg(void)
{
	u8 port = rx_info.main_port;

	if (rx[port].clk.cable_clk > 300 &&
		rx[port].clk.cable_clk < 340) {
		hdmirx_wr_amlphy(T5M_RG_RX20PLL_0, 0x05302820);
		usleep_range(10, 20);
		hdmirx_wr_amlphy(T5M_RG_RX20PLL_1, 0x01481236);
		usleep_range(10, 20);
		hdmirx_wr_amlphy(T5M_RG_RX20PLL_0, 0x05302821);
		usleep_range(10, 20);
		hdmirx_wr_amlphy(T5M_RG_RX20PLL_0, 0x05302823);
		usleep_range(10, 20);
		hdmirx_wr_amlphy(T5M_RG_RX20PLL_1, 0x21401236);
		usleep_range(10, 20);
		hdmirx_wr_amlphy(T5M_RG_RX20PLL_0, 0x05302827);
		usleep_range(10, 20);
		hdmirx_wr_amlphy(T5M_RG_RX20PLL_0, 0x45302827);
		usleep_range(10, 20);
		rx[port].phy.aud_div = 0;
	} else {
		hdmirx_wr_amlphy(T5M_RG_RX20PLL_0, 0x05302800);
		usleep_range(10, 20);
		hdmirx_wr_amlphy(T5M_RG_RX20PLL_1, 0x01481236);
		usleep_range(10, 20);
		hdmirx_wr_amlphy(T5M_RG_RX20PLL_0, 0x05302801);
		usleep_range(10, 20);
		hdmirx_wr_amlphy(T5M_RG_RX20PLL_0, 0x45302803);
		usleep_range(10, 20);
		hdmirx_wr_amlphy(T5M_RG_RX20PLL_1, 0x01401236);
		usleep_range(10, 20);
		hdmirx_wr_amlphy(T5M_RG_RX20PLL_0, 0x05302807);
		usleep_range(10, 20);
		hdmirx_wr_amlphy(T5M_RG_RX20PLL_0, 0x45302807);
		rx[port].phy.aud_div = 0;
	}
	usleep_range(10, 20);
}

void aml_pll_bw_cfg_t5m(void)
{
	u8 port = rx_info.main_port;
	u32 idx = rx[port].phy.pll_bw;
	u32 cableclk = rx[port].clk.cable_clk / KHz;
	int pll_rst_cnt = 0;
	u32 clk_rate;

	clk_rate = rx_get_scdc_clkrate_sts(port);
	idx = aml_phy_pll_band(rx[port].clk.cable_clk, clk_rate);
	if (!is_clk_stable(port) || !cableclk)
		return;
	if (log_level & PHY_LOG)
		rx_pr("pll bw: %d\n", idx);
	if (rx_info.aml_phy.osc_mode && idx == PHY_BW_5) {
		/* sel osc as pll clock */
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHA_MISC2, T5M_PLL_CLK_SEL, 1);
		/* t5m: select tmds_clk from tclk or tmds_ch_clk */
		/* cdr = tmds_ch_ck,  vco =tclk */
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHA_MISC1, T5M_VCO_TMDS_EN, 0);
	}
	switch (idx) {
	case PLL_BW_0:
		t5m_480p_pll_cfg();
		break;
	case PLL_BW_1:
		t5m_720p_pll_cfg();
		break;
	case PLL_BW_2:
		t5m_1080p_pll_cfg();
		break;
	case PLL_BW_3:
		t5m_4k30_pll_cfg();
		break;
	case PLL_BW_4:
		t5m_4k60_pll_cfg();
		break;
	}
	/* do 5 times when clk not stable within a interrupt */
	do {
		if (idx == PLL_BW_0)
			t5m_480p_pll_cfg();
		if (idx == PLL_BW_1)
			t5m_720p_pll_cfg();
		if (idx == PLL_BW_2)
			t5m_1080p_pll_cfg();
		if (idx == PLL_BW_3)
			t5m_4k30_pll_cfg();
		if (idx == PLL_BW_4)
			t5m_4k60_pll_cfg();
		if (log_level & PHY_LOG)
			rx_pr("PLL0=0x%x\n", hdmirx_rd_amlphy(T5M_RG_RX20PLL_0));
		if (pll_rst_cnt++ > pll_rst_max) {
			if (log_level & VIDEO_LOG)
				rx_pr("pll rst error\n");
			break;
		}
		if (log_level & VIDEO_LOG) {
			rx_pr("sq=%d,pll_lock=%d",
			      hdmirx_rd_top(TOP_MISC_STAT0, port) & 0x1,
			      is_pll_lock_t5m());
		}
	} while (!is_tmds_clk_stable(port) && is_clk_stable(port) && !aml_phy_pll_lock(port));
	if (log_level & PHY_LOG)
		rx_pr("pll done\n");
	/* t5m debug */
	/* manual VGA mode for debug,hyper gain=1 */
	if (rx_info.aml_phy.vga_gain <= 0xfff) {
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHA_AFE, MSK(12, 0),
			(decimaltogray_t5m(rx_info.aml_phy.vga_gain & 0x7) |
			(decimaltogray_t5m(rx_info.aml_phy.vga_gain >> 4 &
			0x7) << 4) |
			(decimaltogray_t5m(rx_info.aml_phy.vga_gain >> 8 &
			0x7) << 8)));
	}
	/* manual EQ mode for debug */
	if (rx_info.aml_phy.eq_stg1 < 0x1f) {
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ,
				      T5M_BYP_EQ, rx_info.aml_phy.eq_stg1 & 0x1f);
		/* eq adaptive:0-adaptive 1-manual */
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_EN_BYP_EQ, 1);
	}
	/*tap2 byp*/
	if (rx_info.aml_phy.tap2_byp && rx[port].phy.phy_bw >= PHY_BW_3)
		/* dfe_tap_en [28:20]*/
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHA_DFE, _BIT(22), 0);
}

int get_tap2_t5m(int val)
{
	if ((val >> 4) == 0)
		return val;
	else
		return (0 - (val & 0xf));
}

bool is_dfe_sts_ok_t5m(void)
{
	u32 data32;
	u32 dfe0_tap2, dfe1_tap2, dfe2_tap2;
	u32 dfe0_tap3, dfe1_tap3, dfe2_tap3;
	u32 dfe0_tap4, dfe1_tap4, dfe2_tap4;
	u32 dfe0_tap5, dfe1_tap5, dfe2_tap5;
	u32 dfe0_tap6, dfe1_tap6, dfe2_tap6;
	u32 dfe0_tap7, dfe1_tap7, dfe2_tap7;
	u32 dfe0_tap8, dfe1_tap8, dfe2_tap8;
	bool ret = true;

	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_EHM_DBG_SEL, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_STATUS_MUX_SEL, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_DFE_OFST_DBG_SEL, 0x2);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHD_STAT);
	dfe0_tap2 = data32 & 0xf;
	dfe1_tap2 = (data32 >> 8) & 0xf;
	dfe2_tap2 = (data32 >> 16) & 0xf;
	if (dfe0_tap2 >= 8 ||
	    dfe1_tap2 >= 8 ||
	    dfe2_tap2 >= 8)
		ret = false;

	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_EHM_DBG_SEL, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_STATUS_MUX_SEL, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_DFE_OFST_DBG_SEL, 0x3);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHD_STAT);
	dfe0_tap3 = data32 & 0x7;
	dfe1_tap3 = (data32 >> 8) & 0x7;
	dfe2_tap3 = (data32 >> 16) & 0x7;
	if (dfe0_tap3 >= 6 ||
	    dfe1_tap3 >= 6 ||
	    dfe2_tap3 >= 6)
		ret = false;

	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_EHM_DBG_SEL, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_STATUS_MUX_SEL, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_DFE_OFST_DBG_SEL, 0x4);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHD_STAT);
	dfe0_tap4 = data32 & 0x7;
	dfe1_tap4 = (data32 >> 8) & 0x7;
	dfe2_tap4 = (data32 >> 16) & 0x7;
	if (dfe0_tap4 >= 6 ||
	    dfe1_tap4 >= 6 ||
	    dfe2_tap4 >= 6)
		ret = false;

	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_EHM_DBG_SEL, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_STATUS_MUX_SEL, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_DFE_OFST_DBG_SEL, 0x5);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHD_STAT);
	dfe0_tap5 = data32 & 0x7;
	dfe1_tap5 = (data32 >> 8) & 0x7;
	dfe2_tap5 = (data32 >> 16) & 0x7;
	if (dfe0_tap5 >= 6 ||
	    dfe1_tap5 >= 6 ||
	    dfe2_tap5 >= 6)
		ret = false;

	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_EHM_DBG_SEL, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_STATUS_MUX_SEL, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_DFE_OFST_DBG_SEL, 0x6);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHD_STAT);
	dfe0_tap6 = data32 & 0x7;
	dfe1_tap6 = (data32 >> 8) & 0x7;
	dfe2_tap6 = (data32 >> 16) & 0x7;
	if (dfe0_tap6 >= 6 ||
	    dfe1_tap6 >= 6 ||
	    dfe2_tap6 >= 6)
		ret = false;

	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_EHM_DBG_SEL, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_STATUS_MUX_SEL, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_DFE_OFST_DBG_SEL, 0x7);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHD_STAT);
	dfe0_tap7 = data32 & 0x7;
	dfe0_tap8 = (data32 >> 4) & 0x7;
	dfe1_tap7 = (data32 >> 8) & 0x7;
	dfe1_tap8 = (data32 >> 12) & 0x7;
	dfe2_tap7 = (data32 >> 16) & 0x7;
	dfe2_tap8 = (data32 >> 20) & 0x7;
	if (dfe0_tap7 >= 6 ||
	    dfe1_tap7 >= 6 ||
	    dfe2_tap7 >= 6 ||
	    dfe0_tap8 >= 6 ||
	    dfe1_tap8 >= 6 ||
	    dfe2_tap8 >= 6)
		ret = false;

	return ret;
}

/* long cable detection for <3G need to be change */
void aml_phy_long_cable_det_t5m(void)
{
	int tap2_0, tap2_1, tap2_2;
	int tap2_max = 0;
	u32 data32 = 0;
	u8 port = rx_info.main_port;

	if (rx[port].phy.phy_bw > PHY_BW_3)
		return;
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_EHM_DBG_SEL, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_STATUS_MUX_SEL, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_DFE_OFST_DBG_SEL, 0x2);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHD_STAT);
	tap2_0 = get_tap2_t5m(data32 & 0x1f);
	tap2_1 = get_tap2_t5m(((data32 >> 8) & 0x1f));
	tap2_2 = get_tap2_t5m(((data32 >> 16) & 0x1f));
	if (rx[port].phy.phy_bw == PHY_BW_2) {
		/*disable DFE*/
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_DFE_RSTB, 0);
		tap2_max = 6;
	} else if (rx[port].phy.phy_bw == PHY_BW_3) {
		tap2_max = 10;
	}
	if ((tap2_0 + tap2_1 + tap2_2) >= tap2_max) {
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_BYP_EQ, 0x12);
		usleep_range(10, 20);
		rx_pr("long cable\n");
	}
}

/* aml_hyper_gain_tuning */
void aml_hyper_gain_tuning_t5m(void)
{
	u32 data32;
	u32 tap0, tap1, tap2;
	u32 hyper_gain_0 = 0;
	u32 hyper_gain_1 = 0;
	u32 hyper_gain_2 = 0;
	int eq_boost0, eq_boost1, eq_boost2;

	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_EHM_DBG_SEL, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_STATUS_MUX_SEL, 0x3);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHD_STAT);
	eq_boost0 = data32 & 0x1f;
	eq_boost1 = (data32 >> 8)  & 0x1f;
	eq_boost2 = (data32 >> 16)	& 0x1f;

	/* use HYPER_GAIN calibration instead of vga */
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_EHM_DBG_SEL, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_STATUS_MUX_SEL, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_DFE_OFST_DBG_SEL, 0x0);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHD_STAT);

	tap0 = data32 & 0xff;
	tap1 = (data32 >> 8) & 0xff;
	tap2 = (data32 >> 16) & 0xff;

	if ((rx_info.aml_phy.eq_en && eq_boost0 < 3) || tap0 < 0x12) {
		hyper_gain_0 = 1;
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHA_AFE,
					  T5M_LEQ_HYPER_GAIN_CH0,
					  hyper_gain_0);
		if (log_level & PHY_LOG)
			rx_pr("ch0 hyper gain triger\n");
	}
	if ((rx_info.aml_phy.eq_en && eq_boost1 < 3) || tap1 < 0x12) {
		hyper_gain_1 = 1;
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHA_AFE,
					  T5M_LEQ_HYPER_GAIN_CH1,
					  hyper_gain_1);
		if (log_level & PHY_LOG)
			rx_pr("ch1 hyper gain triger\n");
	}
	if ((rx_info.aml_phy.eq_en && eq_boost2 < 3) || tap2 < 0x12) {
		hyper_gain_2 = 1;
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHA_AFE,
					  T5M_LEQ_HYPER_GAIN_CH2,
					  hyper_gain_2);
		if (log_level & PHY_LOG)
			rx_pr("ch2 hyper gain triger\n");
	}
}

int max_offset(int a, int b, int c)
{
	if (a >= b && a >= c)
		return 0;
	if (b >= a && b >= c)
		return 1;
	if (c >= a && c >= b)
		return 2;
	return -1;
}

void aml_eq_retry_t5m(void)
{
	int data32 = 0;
	int eq_boost0, eq_boost1, eq_boost2;
	u8 port = rx_info.main_port;

	if (rx[port].phy.phy_bw >= PHY_BW_3) {
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_EHM_DBG_SEL, 0x0);
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_STATUS_MUX_SEL, 0x3);
		usleep_range(100, 110);
		data32 = hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHD_STAT);
		eq_boost0 = data32 & 0x1f;
		eq_boost1 = (data32 >> 8)  & 0x1f;
		eq_boost2 = (data32 >> 16)	& 0x1f;
		if (eq_boost0 == 0 || eq_boost0 == 31 ||
		    eq_boost1 == 0 || eq_boost1 == 31 ||
		    eq_boost2 == 0 || eq_boost2 == 31 ||
		    abs(eq_boost0 - eq_boost1) > 10 ||
		    abs(eq_boost0 - eq_boost2) > 10 ||
		    abs(eq_boost1 - eq_boost2) > 10) {
			rx_pr("eq_retry:%d-%d-%d\n", eq_boost0, eq_boost1, eq_boost2);
			hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_EQ_EN, 1);
			hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_EQ_RSTB, 0x0);
			hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_CDR_RSTB, 0x1);
			usleep_range(100, 110);
			hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_EQ_RSTB, 0x1);
			hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_CDR_RSTB, 0x1);
			usleep_range(10000, 10100);
			/* read eq value after eq retry */
			//hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_EHM_DBG_SEL, 0x0);
			//hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_STATUS_MUX_SEL, 0x3);
			//usleep_range(100, 110);
			//data32 = hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHD_STAT);
			//eq_boost0 = data32 & 0x1f;
			//eq_boost1 = (data32 >> 8)  & 0x1f;
			//eq_boost2 = (data32 >> 16)	& 0x1f;
			//rx_pr("after eq_retry:%d-%d-%d\n", eq_boost0, eq_boost1, eq_boost2);
			if (rx_info.aml_phy.eq_hold)
				hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_EQ_EN, 0);
		}
	}
}

void aml_dfe_en_t5m(void)
{
	if (rx_info.aml_phy.dfe_en) {
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_DFE_EN, 1);
		//if (rx_info.aml_phy.eq_hold)
			//hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_EQ_MODE, 1);
		if (rx_info.aml_phy.eq_retry)
			aml_eq_retry_t5m();
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_DFE_RSTB, 0);
		usleep_range(10, 20);
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ,
				      T5M_DFE_RSTB, 1);
		usleep_range(200, 220);
		if (rx_info.aml_phy.dfe_hold)
			hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ,
					      T5M_DFE_HOLD_EN, 1);
		if (log_level & PHY_LOG)
			rx_pr("dfe\n");
	}
}

/* phy offset calibration based on different chip and board */
void aml_phy_offset_cal_t5m(void)
{
	u32 data32;

	/* PHY */
	hdmirx_wr_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, 0x70080050);
	usleep_range(10, 20);
	hdmirx_wr_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, 0x04008013);
	usleep_range(10, 20);
	hdmirx_wr_amlphy(T5M_HDMIRX20PHY_DCHA_DFE, 0x40102459);
	usleep_range(10, 20);
	hdmirx_wr_amlphy(T5M_HDMIRX20PHY_DCHA_AFE, 0x02821666);
	usleep_range(10, 20);
	hdmirx_wr_amlphy(T5M_HDMIRX20PHY_DCHA_MISC2, 0x11c73220);
	usleep_range(10, 20);
	data32 = 0xffe00100;
	if (rterm_trim_flag_t5m) {
		data32 = ((data32 & (~((0xf << 12) | 0x1))) |
			(rterm_trim_val_t5m << 12) | rterm_trim_flag_t5m << 4);
	}
	hdmirx_wr_amlphy(T5M_HDMIRX20PHY_DCHA_MISC1, data32);
	usleep_range(10, 20);

	/* PLL */
	hdmirx_wr_amlphy(T5M_RG_RX20PLL_0, 0x0500f800);
	usleep_range(10, 20);
	hdmirx_wr_amlphy(T5M_RG_RX20PLL_1, 0x01481236);
	usleep_range(10, 20);
	hdmirx_wr_amlphy(T5M_RG_RX20PLL_0, 0x0500f801);
	usleep_range(10, 20);
	hdmirx_wr_amlphy(T5M_RG_RX20PLL_0, 0x0500f803);
	usleep_range(10, 20);
	hdmirx_wr_amlphy(T5M_RG_RX20PLL_1, 0x01401236);
	usleep_range(10, 20);
	hdmirx_wr_amlphy(T5M_RG_RX20PLL_0, 0x0500f807);
	usleep_range(10, 20);
	hdmirx_wr_amlphy(T5M_RG_RX20PLL_0, 0x4500f807);
	usleep_range(100, 200);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, _BIT(26), 1);
	usleep_range(10, 20);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, MSK(2, 12), 0X3);
	usleep_range(10, 20);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, _BIT(27), 1);
	usleep_range(200, 210);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, _BIT(27), 0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHA_DFE, _BIT(13), 0);
	if (log_level & PHY_LOG)
		rx_pr("ofst cal\n");
}

u32 min_ch(u32 a, u32 b, u32 c)
{
	if (a <= b && a <= c)
		return 0;
	if (b <= a && b <= c)
		return 1;
	if (c <= a && c <= b)
		return 2;
	return 3;
}

/* hardware eye monitor */
u32 aml_eq_eye_monitor_t5m(void)
{
	u32 data32;
	u32 positive_eye_height0, positive_eye_height1, positive_eye_height2;

	usleep_range(50, 100);
	/* hold dfe tap1~tap8 */
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ,
			      T5M_DFE_HOLD_EN, 1);
	usleep_range(10, 20);
	/* disable hw scan mode */
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ,
			      T5M_EHM_HW_SCAN_EN, 0);
	usleep_range(10, 20);
	/* enable hw scan mode */
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ,
			      T5M_EHM_HW_SCAN_EN, 1);
	/* wait for scan done */
	usleep_range(rx_info.aml_phy.eye_delay,
	rx_info.aml_phy.eye_delay + 100);
	/* positive eye height  */
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR,
			      T5M_EHM_DBG_SEL, 1);
	data32 = hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHD_STAT);
	positive_eye_height0 = data32 & 0xff;
	positive_eye_height1 = (data32 >> 8) & 0xff;
	positive_eye_height2 = (data32 >> 16) & 0xff;
	/* exit eye monitor scan mode */
	/* disable hw scan mode */
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ,
			      T5M_EHM_HW_SCAN_EN, 0);
	/* disable eye monitor */
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR,
			     T5M_EHM_DBG_SEL, 0);
	//hdmirx_wr_bits_amlphy(T7_HHI_RX_PHY_DCHA_CNTL2,
			      //T7_EYE_MONITOR_EN1, 0);
	usleep_range(10, 20);
	/* release dfe */
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ,
			      T5M_DFE_HOLD_EN, 0);
	positive_eye_height0 = positive_eye_height0 & 0x3f;
	positive_eye_height1 = positive_eye_height1 & 0x3f;
	positive_eye_height2 = positive_eye_height2 & 0x3f;
	rx_pr("eye height:[%d, %d, %d]\n",
		positive_eye_height0, positive_eye_height1, positive_eye_height2);
	return min_ch(positive_eye_height0, positive_eye_height1, positive_eye_height2);
}

void get_eq_val_t5m(void)
{
	u32 data32 = 0;
	u32 eq_boost0, eq_boost1, eq_boost2;

	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_EHM_DBG_SEL, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_STATUS_MUX_SEL, 0x3);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHD_STAT);
	eq_boost0 = data32 & 0x1f;
	eq_boost1 = (data32 >> 8)  & 0x1f;
	eq_boost2 = (data32 >> 16)      & 0x1f;
	rx_pr("eq:%d-%d-%d\n", eq_boost0, eq_boost1, eq_boost2);
}

/* check eq_boost1 & tap0 status */
bool is_eq1_tap0_err_t5m(void)
{
	u32 data32 = 0;
	u32 eq0, eq1, eq2;
	u32 tap0, tap1, tap2;
	u32 eq_avr, tap0_avr;
	bool ret = false;
	u8 port = rx_info.main_port;

	if (rx[port].phy.phy_bw < PHY_BW_5)
		return ret;
	/* get eq_boost1 val */
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_EHM_DBG_SEL, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_STATUS_MUX_SEL, 0x3);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHD_STAT);
	eq0 = data32 & 0x1f;
	eq1 = (data32 >> 8)  & 0x1f;
	eq2 = (data32 >> 16)      & 0x1f;
	eq_avr =  (eq0 + eq1 + eq2) / 3;
	if (log_level & EQ_LOG)
		rx_pr("eq0=0x%x, eq1=0x%x, eq2=0x%x avr=0x%x\n",
			eq0, eq1, eq2, eq_avr);

	/* get tap0 val */
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_EHM_DBG_SEL, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_STATUS_MUX_SEL, 0x3);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_DFE_OFST_DBG_SEL, 0x0);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHD_STAT);
	tap0 = data32 & 0xff;
	tap1 = (data32 >> 8) & 0xff;
	tap2 = (data32 >> 16) & 0xff;
	tap0_avr = (tap0 + tap1 + tap2) / 3;
	if (log_level & EQ_LOG)
		rx_pr("tap0=0x%x, tap1=0x%x, tap2=0x%x avr=0x%x\n",
			tap0, tap1, tap2, tap0_avr);
	if (eq_avr >= 21 && tap0_avr >= 40)
		ret = true;

	return ret;
}

void aml_agc_flow_t5m(void)
{
	int i;
	u32 data32 = 0;
	u32 tap0, tap1, tap2;
	int flags = 0x7;

	for (i = 7; i > 0; i--) {
		if (flags & 0x1)
			hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHA_AFE, MSK(3, 0),
									decimaltogray_t5m(i));
		if (flags & 0x2)
			hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHA_AFE, MSK(3, 4),
									decimaltogray_t5m(i));
		if (flags & 0x4)
			hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHA_AFE, MSK(3, 8),
									decimaltogray_t5m(i));
		usleep_range(50, 60);
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_EHM_DBG_SEL, 0x0);
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_STATUS_MUX_SEL, 0x3);
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_DFE_OFST_DBG_SEL, 0x0);
		data32 = hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHD_STAT);
		tap0 = data32 & 0xff;
		tap1 = (data32 >> 8) & 0xff;
		tap2 = (data32 >> 16) & 0xff;
		if (tap0 <= rx_info.aml_phy.tapx_value)
			flags &= 0x6;
		if (tap1 <= rx_info.aml_phy.tapx_value)
			flags &= 0x5;
		if (tap2 <= rx_info.aml_phy.tapx_value)
			flags &= 0x3;
		if (!flags)
			break;
	}
	rx_pr("agc done\n");
}

u32 eq_eye_height(u32 wst_ch)
{
	u32 data32;
	u32 positive_eye_height;

	/* hold dfe tap1~tap8 */
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ,
					  T5M_DFE_HOLD_EN, 1);
	usleep_range(10, 20);
	/* disable hw scan mode */
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ,
					  T5M_EHM_HW_SCAN_EN, 0);
	usleep_range(10, 20);
	/* enable hw scan mode */
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ,
			  T5M_EHM_HW_SCAN_EN, 1);

	/* wait for scan done */
	usleep_range(rx_info.aml_phy.eye_delay,
	rx_info.aml_phy.eye_delay + 100);
	/* positive eye height	*/
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR,
			  T5M_EHM_DBG_SEL, 1);
	data32 = hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHD_STAT);
	positive_eye_height = data32 >> (8 * wst_ch) & 0xff;
	/* exit eye monitor scan mode */
	/* disable hw scan mode */
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ,
			  T5M_EHM_HW_SCAN_EN, 0);
	/* disable eye monitor */
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR,
			 T5M_EHM_DBG_SEL, 0);
	//hdmirx_wr_bits_amlphy(T7_HHI_RX_PHY_DCHA_CNTL2,
			  //T7_EYE_MONITOR_EN1, 0);
	usleep_range(10, 20);
	/* release dfe */
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ,
					  T5M_DFE_HOLD_EN, 0);
	positive_eye_height = positive_eye_height & 0x3f;
	return positive_eye_height;
}

void dump_cdr_info(void)
{
	u32 cdr0_lock, cdr1_lock, cdr2_lock;
	u32 cdr0_int, cdr1_int, cdr2_int;
	u32 cdr0_code, cdr1_code, cdr2_code;
	u32 data32;

	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_EHM_DBG_SEL, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_STATUS_MUX_SEL, 0x22);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_MUX_CDR_DBG_SEL, 0x0);
	usleep_range(10, 20);
	data32 = hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHD_STAT);
	cdr0_code = data32 & 0x7f;
	cdr0_lock = (data32 >> 7) & 0x1;
	cdr1_code = (data32 >> 8) & 0x7f;
	cdr1_lock = (data32 >> 15) & 0x1;
	cdr2_code = (data32 >> 16) & 0x7f;
	cdr2_lock = (data32 >> 23) & 0x1;
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_MUX_CDR_DBG_SEL, 0x1);
	usleep_range(10, 20);
	data32 = hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHD_STAT);
	cdr0_int = data32 & 0x7f;
	cdr1_int = (data32 >> 8) & 0x7f;
	cdr2_int = (data32 >> 16) & 0x7f;
	rx_pr("cdr_code=[%d,%d,%d]\n", cdr0_code, cdr1_code, cdr2_code);
	rx_pr("cdr_lock=[%d,%d,%d]\n", cdr0_lock, cdr1_lock, cdr2_lock);
	comb_val_t5m(get_val_t5m, "cdr_int", cdr0_int, cdr1_int, cdr2_int, 7);
}

void cdr_retry(void)
{
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, _BIT(6), 0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_EQ_RSTB, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_DFE_RSTB, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_CDR_RSTB, 0x0);
	usleep_range(10, 20);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_CDR_RSTB, 0x1);
	usleep_range(100, 200);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_EQ_RSTB, 0x1);
	usleep_range(100, 200);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_DFE_RSTB, 0x1);
	usleep_range(500, 600);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, _BIT(6), 1);
	usleep_range(100, 200);
	if (log_level & PHY_LOG)
		dump_cdr_info();
}

void dfe_tap0_pol_polling(u32 pos_min_eh, u32 pos_avg_eh, u32 wst_ch)
{
	u32 int_eye_height_sum = 0;
	u32 int_eye_height[20];
	u32 int_avg_eye_height;
	u32 int_min_eye_height = 63;
	u32 neg_eye_height_sum = 0;
	u32 neg_eye_height[20];
	u32 neg_avg_eye_height;
	u32 neg_min_eye_height = 63;
	int i, j, k;

	/*select inter leave*/
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, MSK(2, 20), 0x0);
	usleep_range(100, 200);
	for (i = 0; i < 20; i++)
		int_eye_height[i] = eq_eye_height(wst_ch);
	quick_sort2(int_eye_height, 0, 19);
	int_min_eye_height = int_eye_height[1];
	for (j = 1; j < 6; j++)
		int_eye_height_sum += int_eye_height[j];
	int_avg_eye_height = int_eye_height_sum;
	if (log_level & PHY_LOG) {
		rx_pr("int_min_eye_height = %d\n", int_min_eye_height);
		rx_pr("int_avg_eye_height = %d / 5\n", int_avg_eye_height);
	}
	if (int_min_eye_height > pos_min_eh ||
		(int_min_eye_height == pos_min_eh && int_avg_eye_height > pos_avg_eh)) {
		rx_pr("select int eq\n");
		return;
	}
	/*select negative*/
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, MSK(2, 20), 0x3);
	usleep_range(100, 200);
	for (k = 0; k < 20; k++)
		neg_eye_height[k] = eq_eye_height(wst_ch);
	quick_sort2(neg_eye_height, 0, 19);
	neg_min_eye_height = neg_eye_height[1];
	for (j = 1; j < 6; j++)
		neg_eye_height_sum += neg_eye_height[j];
	neg_avg_eye_height = neg_eye_height_sum;
	if (log_level & PHY_LOG) {
		rx_pr("neg_min_eye_height = %d\n", neg_min_eye_height);
		rx_pr("neg_avg_eye_height = %d / 5\n", neg_avg_eye_height);
	}
	if (neg_min_eye_height > pos_min_eh ||
		(neg_min_eye_height == pos_min_eh && neg_avg_eye_height > pos_avg_eh)) {
		rx_pr("select neg eq\n");
		return;
	}
	/*select positive*/
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, MSK(2, 20), 0x2);
	usleep_range(100, 200);
	rx_pr("select pos eq\n");
}

//dfe tap polarity was set positive in initial setting matrix
//dfe_tap_pol_polling is for polling tap polarity by the order positvie
//->interleave->negative->positive->positive
//positive = (dchd_eq[20:19] = 2'b10), interleave = (dchd_eq[20:19] = 2'b00)
//negative = (dchd_eq[20:19] = 2'b11)

void swap_num(int *a, int *b)
{
	int temp = *a;
	*a = *b;
	*b = temp;
}

int *num_divide_three(int arr[], int l, int r)
{
	int less = l - 1;
	int more = r;
	int *p = kmalloc(sizeof(int) * 2, GFP_KERNEL);

	while (l < more) {
		if (arr[l] < arr[r])
			swap_num(&arr[++less], &arr[l++]);
		else if (arr[l] > arr[r])
			swap_num(&arr[l], &arr[--more]);
		else
			l++;
	}
	swap_num(&arr[more], &arr[r]);
	p[0] = less + 1;
	p[1] = more;
	return p;
}

void quick_sort2(int arr[], int l, int r)
{
	int len = r + 1;
	int i, j;

	for (i = 0; i < len - 1; i++) {
		for (j = 0; j < len - 1 - i; j++) {
			if (arr[j] > arr[j + 1])
				swap_num(&arr[j], &arr[j + 1]);
		}
	}
}

void aml_enhance_dfe_old(void)
{
	u32 wst_ch;
	int i, j;
	u32 pos_eye_height_sum = 0;
	u32 pos_min_eye_height = 63;
	u32 pos_eye_height[20];
	u32 pos_avg_eye_height;

	wst_ch = aml_eq_eye_monitor_t5m();
	for (i = 0; i < 20; i++)
		pos_eye_height[i] = eq_eye_height(wst_ch);
	quick_sort2(pos_eye_height, 0, 19);
	pos_min_eye_height = pos_eye_height[1];
	for (j = 1; j < 6; j++)
		pos_eye_height_sum += pos_eye_height[j];
	pos_avg_eye_height = pos_eye_height_sum;
	if (log_level & PHY_LOG) {
		rx_pr("pos_min_eye_height = %d\n", pos_min_eye_height);
		rx_pr("pos_avg_eye_height = %d / 5\n", pos_avg_eye_height);
	}
	if (pos_avg_eye_height < rx_info.aml_phy.eye_height * 5)
		dfe_tap0_pol_polling(pos_min_eye_height, pos_avg_eye_height, wst_ch);
}

void aml_enhance_dfe_new(void)
{
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, MSK(2, 20), 0x0);
}

void eq_max_offset(int eq_boost0, int eq_boost1, int eq_boost2)
{
	int offset_eq0, offset_eq1, offset_eq2;
	int ch = -1;
	int eq_initial = 0;

	offset_eq0 = abs(2 * eq_boost0 - eq_boost1 - eq_boost2);
	offset_eq1 = abs(2 * eq_boost1 - eq_boost0 - eq_boost2);
	offset_eq2 = abs(2 * eq_boost2 - eq_boost0 - eq_boost1);
	ch = max_offset(offset_eq0, offset_eq1, offset_eq2);
	if (ch == 0)
		eq_initial = (eq_boost1 + eq_boost2) / 2;
	if (ch == 1)
		eq_initial = (eq_boost0 + eq_boost2) / 2;
	if (ch == 2)
		eq_initial = (eq_boost0 + eq_boost1) / 2;
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_EQ_RSTB, 0x0);
	if (rx_info.aml_phy.eq_level & 0x2) {
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, MSK(5, 0), eq_initial);
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, _BIT(5), 0x1);
	}
	if (rx_info.aml_phy.eq_level & 0x4) {
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, MSK(5, 0), eq_initial);
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, _BIT(5), 0x0);
	}
	if (rx_info.aml_phy.eq_level & 0x8) {
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, MSK(5, 0), 15);
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, _BIT(5), 0x1);
	}
	if (rx_info.aml_phy.eq_level & 0x10) {
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, MSK(5, 0), 15);
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, _BIT(5), 0x0);
	}
	if (rx_info.aml_phy.eq_level & 0x20) {
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, MSK(5, 0), 15);
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, _BIT(13), 0);
		usleep_range(10, 20);
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, _BIT(13), 1);
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, _BIT(13), 0);
		usleep_range(100, 110);
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, _BIT(13), 1);
	}
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_EQ_RSTB, 0x1);
	usleep_range(100, 110);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, _BIT(5), 0x0);
	usleep_range(100, 110);
}

void aml_enhance_eq_t5m(void)
{
	int eq_boost0, eq_boost1, eq_boost2;
	int data32;
	int offset_eq0, offset_eq1, offset_eq2;

	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_EHM_DBG_SEL, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_STATUS_MUX_SEL, 0x3);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHD_STAT);
	eq_boost0 = data32 & 0x1f;
	eq_boost1 = (data32 >> 8)  & 0x1f;
	eq_boost2 = (data32 >> 16)	& 0x1f;
	eq_boost0 = (eq_boost0 >= 23) ? 23 : eq_boost0;
	eq_boost1 = (eq_boost1 >= 23) ? 23 : eq_boost1;
	eq_boost2 = (eq_boost2 >= 23) ? 23 : eq_boost2;
	offset_eq0 = abs(2 * eq_boost0 - eq_boost1 - eq_boost2);
	offset_eq1 = abs(2 * eq_boost1 - eq_boost0 - eq_boost2);
	offset_eq2 = abs(2 * eq_boost2 - eq_boost0 - eq_boost1);
	if (offset_eq0 > 15 || offset_eq1 > 15 || offset_eq2 > 15) {
		eq_max_offset(eq_boost0, eq_boost1, eq_boost2);
	/* read eq value after eq retry */
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_EHM_DBG_SEL, 0x0);
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_STATUS_MUX_SEL, 0x3);
		usleep_range(100, 110);
		data32 = hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHD_STAT);
		eq_boost0 = data32 & 0x1f;
		eq_boost1 = (data32 >> 8)  & 0x1f;
		eq_boost2 = (data32 >> 16)	& 0x1f;
		rx_pr("after enhance eq:%d-%d-%d\n", eq_boost0, eq_boost1, eq_boost2);
	} else {
		rx_pr("no enhance eq\n");
	}
}

void aml_eq_cfg_t5m(void)
{
	u8 port = rx_info.main_port;
	u32 idx = rx[port].phy.phy_bw;
	u32 cdr0_int, cdr1_int, cdr2_int;
	u32 data32;
	int i = 0;

	/* dont need to run eq if no sqo_clk or pll not lock */
	if (!is_clk_stable(port))
		return;
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_CDR_RSTB, 1);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_CDR_EN, 1);
	usleep_range(200, 210);
	if (idx >= PHY_BW_3)
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_EN_BYP_EQ, 0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_EQ_EN, 1);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_EQ_RSTB, 1);
	usleep_range(200, 210);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_DFE_EN, 1);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_DFE_RSTB, 1);
	if (rx_info.aml_phy.cdr_fr_en) {
		udelay(rx_info.aml_phy.cdr_fr_en);
		/*cdr fr en*/
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, _BIT(6), 1);
	}
	usleep_range(10000, 10100);
	get_eq_val_t5m();
	/*if (rx_info.aml_phy.eq_retry)*/
		/*aml_eq_retry_t3();*/
	if (rx[port].phy.phy_bw >= PHY_BW_4) {
		/* step12 */
		/* aml_dfe_en(); */
		/* udelay(100); */
	} else if (rx[port].phy.phy_bw == PHY_BW_3) {//3G
		/* aml_dfe_en(); */
		/*udelay(100);*/
		/*t3 removed, tap1 min value*/
		/* if (rx_info.aml_phy.tap1_byp) { */
			/* aml_phy_tap1_byp_t3(); */
			/* hdmirx_wr_bits_amlphy( */
				/* HHI_RX_PHY_DCHD_CNTL2, */
				/* DFE_EN, 0); */
		/* } */
		/*udelay(100);*/
		/* hdmirx_wr_bits_amlphy(HHI_RX_PHY_DCHD_CNTL0, */
			/* _BIT(28), 1); */
		if (rx_info.aml_phy.long_cable)
			;/* aml_phy_long_cable_det_t3(); */
		if (rx_info.aml_phy.vga_dbg)
			;/* aml_vga_tuning_t3();*/
	} else if (rx[port].phy.phy_bw == PHY_BW_2) {
		if (rx_info.aml_phy.long_cable) {
			/*1.5G should enable DFE first*/
			/* aml_dfe_en(); */
			/* long cable detection*/
			/* aml_phy_long_cable_det_t3();*/
			/* 1.5G should disable DFE at the end*/
			/* udelay(100); */
			/* aml_dfe_en(); */
		}
	}
	/* enable dfe for all frequency */
	if (rx[port].phy.phy_bw >= PHY_BW_3)
		aml_dfe_en_t5m();
	if (is_eq1_tap0_err_t5m()) {
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHA_AFE, T5M_LEQ_BUF_GAIN, 0x0);
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHA_AFE, T5M_LEQ_POLE, 0x0);
		if (log_level & EQ_LOG)
			rx_pr("eq1 & tap0 err, tune eq setting\n");
	}
	/*enable HYPER_GAIN calibration for 6G to fix 2.0 cts HF2-1 issue*/
	if (rx[port].phy.phy_bw >= PHY_BW_2 &&
		rx_info.aml_phy.agc_enable)
		aml_agc_flow_t5m();
	//eq <= 3 will trigger the new hyper gain function
	// if an inappropriate eq value,it happened to
	//trigger hyper gain when eq <= 3,there is no chance to convergence to
	// a proper eq value any more.auto eq failed.
	//the only way to exit this wrong state is back to phy_init entrance.
	//enhance_eq insert before hyper gain function is try to get a proper/
	//right value.
	//this enhance_eq should auto convergence,never to be forced in this
	//stage,or hyper gain fail.
	//can do enhance_eq one more time after enhance_dfe finished.
	if (rx[port].phy.phy_bw >= PHY_BW_2 &&
		rx_info.aml_phy.enhance_eq)
		aml_enhance_eq_t5m();
	if (rx[port].phy.phy_bw == PHY_BW_2 || rx[port].phy.phy_bw == PHY_BW_1 ||
		rx_info.aml_phy.hyper_gain_en)
		aml_hyper_gain_tuning_t5m();
	usleep_range(200, 210);
	/*tmds valid det*/
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_CDR_LKDET_EN, 1);
	if (log_level & PHY_LOG)
		dump_cdr_info();
	for (i = 0; i < rx_info.aml_phy.cdr_retry_max &&
		rx_info.aml_phy.cdr_retry_en; i++) {
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_EHM_DBG_SEL, 0x0);
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_STATUS_MUX_SEL, 0x22);
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_MUX_CDR_DBG_SEL, 0x0);
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_MUX_CDR_DBG_SEL, 0x1);
		usleep_range(10, 20);
		data32 = hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHD_STAT);
		cdr0_int = data32 & 0x7f;
		cdr1_int = (data32 >> 8) & 0x7f;
		cdr2_int = (data32 >> 16) & 0x7f;
		if (cdr0_int || cdr1_int || cdr2_int)
			cdr_retry();
		else
			break;
	}
	if (log_level & PHY_LOG)
		rx_pr("cdr retry times:%d!!!\n", i);
	if (i == rx_info.aml_phy.cdr_retry_max &&
		rx_info.aml_phy.cdr_fr_en_auto) {
		if ((cdr0_int == 0 && cdr1_int == 0) ||
			(cdr0_int == 0 && cdr2_int == 0) ||
			(cdr1_int == 0 && cdr2_int == 0)) {
			hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, _BIT(6), 0);
			if (log_level & PHY_LOG)
				rx_pr("cdr_fr_en force 0!!!\n");
		}
	}
	if (rx[port].phy.phy_bw >= PHY_BW_5 &&
		rx_info.aml_phy.enhance_dfe_en_old)
		aml_enhance_dfe_old();
	if (rx[port].phy.phy_bw >= PHY_BW_5 &&
		rx_info.aml_phy.enhance_dfe_en_new)
		aml_enhance_dfe_new();
	if (rx[port].phy.phy_bw >= PHY_BW_2 &&
		rx_info.aml_phy.enhance_eq)
		aml_enhance_eq_t5m();
	rx_pr("%s,%s,%s\n",
		rx_info.aml_phy.enhance_dfe_en_new ? "new dfe" : "old dfe",
	rx_info.aml_phy.enhance_eq ? "eq en" : "no eq",
	rx_info.aml_phy.eq_en ? "eq triger" : "eq no triger");
	if (log_level & PHY_LOG)
		rx_pr("phy end\n");
}

void aml_phy_get_trim_val_t5m(void)
{
	u32 data32;

	dts_debug_flag = (phy_term_lel >> 4) & 0x1;
	if (dts_debug_flag == 0) {
		data32 = def_trim_value;
		rterm_trim_val_t5m = (data32 >> 12) & 0xf;
		rterm_trim_flag_t5m = data32 & 0x1;
	} else {
		rlevel = phy_term_lel & 0xf;
		if (rlevel > 15)
			rlevel = 15;
		rterm_trim_flag_t5m = dts_debug_flag;
	}
	if (rterm_trim_flag_t5m) {
		if (log_level & PHY_LOG)
			rx_pr("rterm trim=0x%x\n", rterm_trim_val_t5m);
	}
}

void aml_phy_cfg_t5m(void)
{
	u8 port = rx_info.main_port;
	u32 idx = rx[port].phy.phy_bw;
	u32 data32;
	u32 clk_rate;

	if (log_level & PHY_LOG)
		rx_pr("phy start\n");
	if (rx_info.aml_phy.pre_int) {
		clk_rate = rx_get_scdc_clkrate_sts(port);
		idx = aml_cable_clk_band(rx[port].clk.cable_clk, clk_rate);
		if (log_level & PHY_LOG)
			rx_pr("\nphy reg bw: %d\n", idx);
		if (rx_info.aml_phy.ofst_en)
			aml_phy_offset_cal_t5m();
		data32 = phy_dcha_t5m[idx][0];
		if (rx_info.aml_phy.phy_debug_en &&
			rx_info.aml_phy.afe_value)
			data32 = rx_info.aml_phy.afe_value;
		hdmirx_wr_amlphy(T5M_HDMIRX20PHY_DCHA_AFE, data32);
		usleep_range(5, 10);
		data32 = phy_dcha_t5m[idx][1];
		if (rx_info.aml_phy.phy_debug_en &&
			rx_info.aml_phy.dfe_value)
			data32 = rx_info.aml_phy.dfe_value;
		hdmirx_wr_amlphy(T5M_HDMIRX20PHY_DCHA_DFE, data32);
		usleep_range(5, 10);
		data32 = phy_dchd_t5m[idx][0];
		if (rx_info.aml_phy.phy_debug_en &&
			rx_info.aml_phy.cdr_value)
			data32 = rx_info.aml_phy.cdr_value;
		hdmirx_wr_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, data32);
		usleep_range(5, 10);
		data32 = phy_dchd_t5m[idx][1];
		if (rx_info.aml_phy.phy_debug_en &&
			rx_info.aml_phy.eq_value)
			data32 = rx_info.aml_phy.eq_value;
		hdmirx_wr_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, data32);
		usleep_range(5, 10);
		data32 = phy_misc_t5m[idx][0];
		aml_phy_get_trim_val_t5m();
		if (rx_info.aml_phy.phy_debug_en &&
			rx_info.aml_phy.misc1_value)
			data32 = rx_info.aml_phy.misc1_value;
		if (rterm_trim_flag_t5m) {
			if (dts_debug_flag)
				rterm_trim_val_t5m = t5m_rlevel[rlevel];
			data32 = ((data32 & (~((0xf << 12) | 0x1))) |
				(rterm_trim_val_t5m << 12) | rterm_trim_flag_t5m << 4);
		}
		hdmirx_wr_amlphy(T5M_HDMIRX20PHY_DCHA_MISC1, data32);
		usleep_range(5, 10);
		data32 = phy_misc_t5m[idx][1];
		if (rx_info.aml_phy.phy_debug_en &&
			rx_info.aml_phy.misc2_value)
			data32 = rx_info.aml_phy.misc2_value;
		/* port switch */
		data32 &= (~(0xf << 28));
		data32 |= (0xf << 28);
		data32 &= (~(0xf << 24));
		data32 |= ((1 << port) << 24);
		hdmirx_wr_amlphy(T5M_HDMIRX20PHY_DCHA_MISC2, data32);
		usleep_range(5, 10);
		if (!rx_info.aml_phy.pre_int_en)
			rx_info.aml_phy.pre_int = 0;
	}
	if (rx_info.aml_phy.sqrst_en) {
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHA_MISC1, T5M_SQ_RSTN, 0);
		usleep_range(5, 10);
		/*sq_rst*/
		hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHA_MISC1, T5M_SQ_RSTN, 1);
	}
}

 /* For t5m */
void aml_phy_init_t5m(void)
{
	u8 port = rx_info.main_port;

	if (rx[port].state == FSM_WAIT_CLK_STABLE &&
		!rx[port].cableclk_stb_flg) {
		aml_phy_cfg_t5m();
		return;
	}
	aml_phy_cfg_t5m();
	usleep_range(10, 20);
	aml_pll_bw_cfg_t5m();
	usleep_range(10, 20);
	aml_eq_cfg_t5m();
}

void dump_reg_phy_t5m(void)
{
	rx_pr("PHY Register:\n");
	rx_pr("dchd_eq-0x14=0x%x\n", hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHD_EQ));
	rx_pr("dchd_cdr-0x10=0x%x\n", hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHD_CDR));
	rx_pr("dcha_dfe-0xc=0x%x\n", hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHA_DFE));
	rx_pr("dcha_afe-0x8=0x%x\n", hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHA_AFE));
	rx_pr("misc2-0x1c=0x%x\n", hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHA_MISC2));
	rx_pr("misc1-0x18=0x%x\n", hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHA_MISC1));
}

/*
 * rx phy v2 debug
 */
int count_one_bits_t5m(u32 value)
{
	int count = 0;

	for (; value != 0; value >>= 1) {
		if (value & 1)
			count++;
	}
	return count;
}

void get_val_t5m(char *temp, unsigned int val, int len)
{
	if ((val >> (len - 1)) == 0)
		sprintf(temp, "+%d", val & (~(1 << (len - 1))));
	else
		sprintf(temp, "-%d", val & (~(1 << (len - 1))));
}

void get_flag_val_t5m(char *temp, unsigned int val, int len)
{
	if ((val >> (len - 1)) == 0)
		sprintf(temp, "-%d", val & (~(1 << (len - 1))));
	else
		sprintf(temp, "+%d", val & (~(1 << (len - 1))));
}

void comb_val_t5m(void (*p)(char *, unsigned int, int),
					char *type, unsigned int val_0, unsigned int val_1,
					unsigned int val_2, int len)
{
	char out[32], v0_buf[16], v1_buf[16], v2_buf[16];
	int pos = 0;

	p(v0_buf, val_0, len);
	p(v1_buf, val_1, len);
	p(v2_buf, val_2, len);
	pos += snprintf(out + pos, 32 - pos, "%s[", type);
	pos += snprintf(out + pos, 32 - pos, " %s,", v0_buf);
	pos += snprintf(out + pos, 32 - pos, " %s,", v1_buf);
	pos += snprintf(out + pos, 32 - pos, " %s]", v2_buf);
	rx_pr("%s\n", out);
}

void dump_aml_phy_sts_t5m(void)
{
	u8 port = rx_info.main_port;
	u32 data32;
	u32 terminal;
	u32 ch0_eq_boost1, ch1_eq_boost1, ch2_eq_boost1;
	u32 ch0_eq_err, ch1_eq_err, ch2_eq_err;
	u32 dfe0_tap0, dfe1_tap0, dfe2_tap0, dfe3_tap0;
	u32 dfe0_tap1, dfe1_tap1, dfe2_tap1, dfe3_tap1;
	u32 dfe0_tap2, dfe1_tap2, dfe2_tap2, dfe3_tap2;
	u32 dfe0_tap3, dfe1_tap3, dfe2_tap3, dfe3_tap3;
	u32 dfe0_tap4, dfe1_tap4, dfe2_tap4, dfe3_tap4;
	u32 dfe0_tap5, dfe1_tap5, dfe2_tap5, dfe3_tap5;
	u32 dfe0_tap6, dfe1_tap6, dfe2_tap6, dfe3_tap6;
	u32 dfe0_tap7, dfe1_tap7, dfe2_tap7, dfe3_tap7;
	u32 dfe0_tap8, dfe1_tap8, dfe2_tap8, dfe3_tap8;

	u32 cdr0_lock, cdr1_lock, cdr2_lock;
	u32 cdr0_int, cdr1_int, cdr2_int;
	u32 cdr0_code, cdr1_code, cdr2_code;

	bool pll_lock;
	bool squelch;

	u32 sli0_ofst0, sli1_ofst0, sli2_ofst0;
	u32 sli0_ofst1, sli1_ofst1, sli2_ofst1;
	u32 sli0_ofst2, sli1_ofst2, sli2_ofst2;

	/* rterm */
	terminal = (hdmirx_rd_bits_amlphy(T5M_HDMIRX20PHY_DCHA_MISC1, T5M_RTERM_CNTL));

	/* eq_boost1 status */
	/* mux_eye_en */
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_EHM_DBG_SEL, 0x0);
	/* mux_block_sel */
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_STATUS_MUX_SEL, 0x3);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHD_STAT);
	ch0_eq_boost1 = data32 & 0x1f;
	ch0_eq_err = (data32 >> 5) & 0x3;
	ch1_eq_boost1 = (data32 >> 8) & 0x1f;
	ch1_eq_err = (data32 >> 13) & 0x3;
	ch2_eq_boost1 = (data32 >> 16) & 0x1f;
	ch2_eq_err = (data32 >> 21) & 0x3;

	/* dfe tap0 sts */
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_EHM_DBG_SEL, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_STATUS_MUX_SEL, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_DFE_OFST_DBG_SEL, 0x0);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHD_STAT);
	dfe0_tap0 = data32 & 0x7f;
	dfe1_tap0 = (data32 >> 8) & 0x7f;
	dfe2_tap0 = (data32 >> 16) & 0x7f;
	dfe3_tap0 = (data32 >> 24) & 0x7f;
	/* dfe tap1 sts */
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_EHM_DBG_SEL, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_STATUS_MUX_SEL, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_DFE_OFST_DBG_SEL, 0x1);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHD_STAT);
	dfe0_tap1 = data32 & 0x3f;
	dfe1_tap1 = (data32 >> 8) & 0x3f;
	dfe2_tap1 = (data32 >> 16) & 0x3f;
	dfe3_tap1 = (data32 >> 24) & 0x3f;
	/* dfe tap2 sts */
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_EHM_DBG_SEL, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_STATUS_MUX_SEL, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_DFE_OFST_DBG_SEL, 0x2);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHD_STAT);
	dfe0_tap2 = data32 & 0x1f;
	dfe1_tap2 = (data32 >> 8) & 0x1f;
	dfe2_tap2 = (data32 >> 16) & 0x1f;
	dfe3_tap2 = (data32 >> 24) & 0x1f;
	/* dfe tap3 sts */
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_EHM_DBG_SEL, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_STATUS_MUX_SEL, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_DFE_OFST_DBG_SEL, 0x3);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHD_STAT);
	dfe0_tap3 = data32 & 0xf;
	dfe1_tap3 = (data32 >> 8) & 0xf;
	dfe2_tap3 = (data32 >> 16) & 0xf;
	dfe3_tap3 = (data32 >> 24) & 0xf;
	/* dfe tap4 sts */
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_EHM_DBG_SEL, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_STATUS_MUX_SEL, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_DFE_OFST_DBG_SEL, 0x4);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHD_STAT);
	dfe0_tap4 = data32 & 0xf;
	dfe1_tap4 = (data32 >> 8) & 0xf;
	dfe2_tap4 = (data32 >> 16) & 0xf;
	dfe3_tap4 = (data32 >> 24) & 0xf;
	/* dfe tap5 sts */
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_EHM_DBG_SEL, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_STATUS_MUX_SEL, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_DFE_OFST_DBG_SEL, 0x5);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHD_STAT);
	dfe0_tap5 = data32 & 0xf;
	dfe1_tap5 = (data32 >> 8) & 0xf;
	dfe2_tap5 = (data32 >> 16) & 0xf;
	dfe3_tap5 = (data32 >> 24) & 0xf;
	/* dfe tap6 sts */
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_EHM_DBG_SEL, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_STATUS_MUX_SEL, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_DFE_OFST_DBG_SEL, 0x6);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHD_STAT);
	dfe0_tap6 = data32 & 0xf;
	dfe1_tap6 = (data32 >> 8) & 0xf;
	dfe2_tap6 = (data32 >> 16) & 0xf;
	dfe3_tap6 = (data32 >> 24) & 0xf;
	/* dfe tap7/8 sts */
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_EHM_DBG_SEL, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_STATUS_MUX_SEL, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_DFE_OFST_DBG_SEL, 0x7);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHD_STAT);
	dfe0_tap7 = data32 & 0xf;
	dfe1_tap7 = (data32 >> 8) & 0xf;
	dfe2_tap7 = (data32 >> 16) & 0xf;
	dfe3_tap7 = (data32 >> 24) & 0xf;
	dfe0_tap8 = (data32 >> 4) & 0xf;
	dfe1_tap8 = (data32 >> 12) & 0xf;
	dfe2_tap8 = (data32 >> 20) & 0xf;
	dfe3_tap8 = (data32 >> 24) & 0xf;

	/* CDR status */
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_EHM_DBG_SEL, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_STATUS_MUX_SEL, 0x22);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_MUX_CDR_DBG_SEL, 0x0);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHD_STAT);
	cdr0_code = data32 & 0x7f;
	cdr0_lock = (data32 >> 7) & 0x1;
	cdr1_code = (data32 >> 8) & 0x7f;
	cdr1_lock = (data32 >> 15) & 0x1;
	cdr2_code = (data32 >> 16) & 0x7f;
	cdr2_lock = (data32 >> 23) & 0x1;
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_MUX_CDR_DBG_SEL, 0x1);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHD_STAT);
	cdr0_int = data32 & 0x7f;
	cdr1_int = (data32 >> 8) & 0x7f;
	cdr2_int = (data32 >> 16) & 0x7f;

	/* pll lock */
	pll_lock = hdmirx_rd_amlphy(T5M_RG_RX20PLL_0) >> 31;

	/* squelch */
	squelch = hdmirx_rd_top(TOP_MISC_STAT0, port) & 0x1;

	/* slicer offset status */
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_EHM_DBG_SEL, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_STATUS_MUX_SEL, 0x1);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_DFE_OFST_DBG_SEL, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_MUX_CDR_DBG_SEL, 0x1);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHD_STAT);
	sli0_ofst0 = data32 & 0x1f;
	sli1_ofst0 = (data32 >> 8) & 0x1f;
	sli2_ofst0 = (data32 >> 16) & 0x1f;

	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_EHM_DBG_SEL, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_STATUS_MUX_SEL, 0x1);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_DFE_OFST_DBG_SEL, 0x1);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_MUX_CDR_DBG_SEL, 0x1);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHD_STAT);
	sli0_ofst1 = data32 & 0x1f;
	sli1_ofst1 = (data32 >> 8) & 0x1f;
	sli2_ofst1 = (data32 >> 16) & 0x1f;

	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_EHM_DBG_SEL, 0x0);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, T5M_STATUS_MUX_SEL, 0x1);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_DFE_OFST_DBG_SEL, 0x2);
	hdmirx_wr_bits_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, T5M_MUX_CDR_DBG_SEL, 0x1);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHD_STAT);
	sli0_ofst2 = data32 & 0x1f;
	sli1_ofst2 = (data32 >> 8) & 0x1f;
	sli2_ofst2 = (data32 >> 16) & 0x1f;

	rx_pr("\nhdmirx phy status:\n");
	rx_pr("pll_lock=%d, squelch=%d, terminal=%d\n", pll_lock, squelch, terminal);
	rx_pr("eq_boost1=[%d,%d,%d]\n",
	      ch0_eq_boost1, ch1_eq_boost1, ch2_eq_boost1);
	rx_pr("eq_err=[%d,%d,%d]\n",
	      ch0_eq_err, ch1_eq_err, ch2_eq_err);

	comb_val_t5m(get_val_t5m, "	 dfe_tap0", dfe0_tap0, dfe1_tap0, dfe2_tap0, 7);
	comb_val_t5m(get_val_t5m, "	 dfe_tap1", dfe0_tap1, dfe1_tap1, dfe2_tap1, 6);
	comb_val_t5m(get_val_t5m, "	 dfe_tap2", dfe0_tap2, dfe1_tap2, dfe2_tap2, 5);
	comb_val_t5m(get_val_t5m, "	 dfe_tap3", dfe0_tap3, dfe1_tap3, dfe2_tap3, 4);
	comb_val_t5m(get_val_t5m, "	 dfe_tap4", dfe0_tap4, dfe1_tap4, dfe2_tap4, 4);
	comb_val_t5m(get_val_t5m, "	 dfe_tap5", dfe0_tap5, dfe1_tap5, dfe2_tap5, 4);
	comb_val_t5m(get_val_t5m, "	 dfe_tap6", dfe0_tap6, dfe1_tap6, dfe2_tap6, 4);
	comb_val_t5m(get_val_t5m, "	 dfe_tap7", dfe0_tap7, dfe1_tap7, dfe2_tap7, 4);
	comb_val_t5m(get_val_t5m, "	 dfe_tap8", dfe0_tap8, dfe1_tap8, dfe2_tap8, 4);

	comb_val_t5m(get_val_t5m, "slicer_ofst0", sli0_ofst0, sli1_ofst0, sli2_ofst0, 5);
	comb_val_t5m(get_val_t5m, "slicer_ofst1", sli0_ofst1, sli1_ofst1, sli2_ofst1, 5);
	comb_val_t5m(get_val_t5m, "slicer_ofst2", sli0_ofst2, sli1_ofst2, sli2_ofst2, 5);

	rx_pr("cdr_code=[%d,%d,%d]\n", cdr0_code, cdr1_code, cdr2_code);
	rx_pr("cdr_lock=[%d,%d,%d]\n", cdr0_lock, cdr1_lock, cdr2_lock);
	comb_val_t5m(get_val_t5m, "cdr_int", cdr0_int, cdr1_int, cdr2_int, 7);
}

bool aml_get_tmds_valid_t5m(void)
{
	u32 tmdsclk_valid;
	u32 sqofclk;
	u32 tmds_align;
	u32 ret;
	u8 port = rx_info.main_port;

	/* digital tmds valid depends on PLL lock from analog phy. */
	/* it is not necessary and T7 has not it */
	/* tmds_valid = hdmirx_rd_dwc(DWC_HDMI_PLL_LCK_STS) & 0x01; */
	sqofclk = hdmirx_rd_top(TOP_MISC_STAT0, port) & 0x1;
	tmdsclk_valid = is_tmds_clk_stable(port);
	/* modified in T7, 0x2b bit'0 tmds_align status */
	tmds_align = hdmirx_rd_top(TOP_TMDS_ALIGN_STAT, port) & 0x01;
	if (sqofclk && tmdsclk_valid && tmds_align) {
		ret = 1;
	} else {
		if (log_level & VIDEO_LOG) {
			rx_pr("sqo:%x,tmdsclk_valid:%x,align:%x\n",
			      sqofclk, tmdsclk_valid, tmds_align);
			rx_pr("cable clk0:%d\n", rx[port].clk.cable_clk);
			//rx_pr("cable clk1:%d\n", rx_get_clock(TOP_HDMI_CABLECLK, port));
		}
		ret = 0;
	}
	return ret;
}

void aml_phy_short_bist_t5m(void)
{
	int data32;
	int bist_mode = 3;
	int port;
	int ch0_lock = 0;
	int ch1_lock = 0;
	int ch2_lock = 0;
	int lock_sts = 0;

	for (port = 0; port < 3; port++) {
		hdmirx_wr_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, 0x30000050);
		usleep_range(5, 10);
		hdmirx_wr_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, 0x04007053);
		usleep_range(5, 10);
		hdmirx_wr_amlphy(T5M_HDMIRX20PHY_DCHA_DFE, 0x7ff00459);
		usleep_range(5, 10);
		hdmirx_wr_amlphy(T5M_HDMIRX20PHY_DCHA_MISC2, 0x11c73228);
		usleep_range(5, 10);
		hdmirx_wr_amlphy(T5M_HDMIRX20PHY_DCHA_MISC2, 0xfff00100);
		usleep_range(5, 10);
		hdmirx_wr_amlphy(T5M_HDMIRX20PLL_CTRL0, 0x0500f800);
		usleep_range(10, 20);
		hdmirx_wr_amlphy(T5M_HDMIRX20PLL_CTRL1, 0x01481236);
		usleep_range(10, 20);
		hdmirx_wr_amlphy(T5M_HDMIRX20PLL_CTRL0, 0x0500f801);
		usleep_range(10, 20);
		hdmirx_wr_amlphy(T5M_HDMIRX20PLL_CTRL0, 0x0500f803);
		usleep_range(10, 20);
		hdmirx_wr_amlphy(T5M_HDMIRX20PLL_CTRL1, 0x01401236);
		usleep_range(10, 20);
		hdmirx_wr_amlphy(T5M_HDMIRX20PLL_CTRL0, 0x0500f807);
		usleep_range(10, 20);
		hdmirx_wr_amlphy(T5M_HDMIRX20PLL_CTRL0, 0x4500f807);
		usleep_range(10, 20);
		usleep_range(1000, 1100);
		/* Reset */
		data32	= 0x0;
		data32	|=	1 << 8;
		data32	|=	1 << 7;
		/* Configure BIST analyzer before BIST path out of reset */
		hdmirx_wr_top(TOP_SW_RESET, data32, port);
		usleep_range(5, 10);
		// Configure BIST analyzer before BIST path out of reset
		data32 = 0;
		// [23:22] prbs_ana_ch2_prbs_mode:
		// 0=prbs11; 1=prbs15; 2=prbs7; 3=prbs31.
		data32	|=	bist_mode << 22;
		// [21:20] prbs_ana_ch2_width:3=10-bit pattern
		data32	|=	3 << 20;
		// [   19] prbs_ana_ch2_clr_ber_meter	//0
		data32	|=	1 << 19;
		// [   18] prbs_ana_ch2_freez_ber
		data32	|=	0 << 18;
		// [	17] prbs_ana_ch2_bit_reverse
		data32	|=	1 << 17;
		// [15:14] prbs_ana_ch1_prbs_mode:
		// 0=prbs11; 1=prbs15; 2=prbs7; 3=prbs31.
		data32	|=	bist_mode << 14;
		// [13:12] prbs_ana_ch1_width:3=10-bit pattern
		data32	|=	3 << 12;
		// [	 11] prbs_ana_ch1_clr_ber_meter //0
		data32	|=	1 << 11;
		// [   10] prbs_ana_ch1_freez_ber
		data32	|=	0 << 10;
		// [	9] prbs_ana_ch1_bit_reverse
		data32	|=	1 << 9;
		// [ 7: 6] prbs_ana_ch0_prbs_mode:
		// 0=prbs11; 1=prbs15; 2=prbs7; 3=prbs31.
		data32	|=	bist_mode << 6;
		// [ 5: 4] prbs_ana_ch0_width:3=10-bit pattern
		data32	|=	3 << 4;
		// [	 3] prbs_ana_ch0_clr_ber_meter	//0
		data32	|=	1 << 3;
		// [	  2] prbs_ana_ch0_freez_ber
		data32	|=	0 << 2;
		// [	1] prbs_ana_ch0_bit_reverse
		data32	|=	1 << 1;
		hdmirx_wr_top(TOP_PRBS_ANA_0,  data32, port);
		usleep_range(5, 10);
		data32			= 0;
		// [19: 8] prbs_ana_time_window
		data32	|=	255 << 8;
		// [ 7: 0] prbs_ana_err_thr
		data32	|=	0;
		hdmirx_wr_top(TOP_PRBS_ANA_1,  data32, port);
		usleep_range(5, 10);
		// Configure channel switch
		data32			= 0;
		data32	|=	2 << 28;// [29:28] source_2
		data32	|=	1 << 26;// [27:26] source_1
		data32	|=	0 << 24;// [25:24] source_0
		data32	|=	0 << 22;// [22:20] skew_2
		data32	|=	0 << 16;// [18:16] skew_1
		data32	|=	0 << 12;// [14:12] skew_0
		data32	|=	0 << 10;// [   10] bitswap_2
		data32	|=	0 << 9;// [    9] bitswap_1
		data32	|=	0 << 8;// [    8] bitswap_0
		data32	|=	0 << 6;// [    6] polarity_2
		data32	|=	0 << 5;// [    5] polarity_1
		data32	|=	0 << 4;// [    4] polarity_0
		data32	|=	0;// [	  0] enable
		hdmirx_wr_top(TOP_CHAN_SWITCH_0, data32, port);
		usleep_range(5, 10);
		// Configure BIST generator
		data32		   = 0;
		data32	|=	0 << 8;// [    8] bist_loopback
		data32	|=	3 << 5;// [ 7: 5] decoup_thresh
		// [ 4: 3] prbs_gen_mode:0=prbs11; 1=prbs15; 2=prbs7; 3=prbs31.
		data32	|=	bist_mode << 3;
		data32	|=	3 << 1;// [ 2: 1] prbs_gen_width:3=10-bit.
		data32	|=	0;// [	 0] prbs_gen_enable
		hdmirx_wr_top(TOP_PRBS_GEN, data32, port);
		usleep_range(1000, 1100);
		/* Reset */
		data32	= 0x0;
		data32	&=	~(1 << 8);
		data32	&=	~(1 << 7);
		/* Configure BIST analyzer before BIST path out of reset */
		hdmirx_wr_top(TOP_SW_RESET, data32, port);
		usleep_range(100, 110);
		// Configure channel switch
		data32 = 0;
		data32	|=	2 << 28;// [29:28] source_2
		data32	|=	1 << 26;// [27:26] source_1
		data32	|=	0 << 24;// [25:24] source_0
		data32	|=	0 << 22;// [22:20] skew_2
		data32	|=	0 << 16;// [18:16] skew_1
		data32	|=	0 << 12;// [14:12] skew_0
		data32	|=	0 << 10;// [   10] bitswap_2
		data32	|=	0 << 9;// [    9] bitswap_1
		data32	|=	0 << 8;// [    8] bitswap_0
		data32	|=	0 << 6;// [    6] polarity_2
		data32	|=	0 << 5;// [    5] polarity_1
		data32	|=	0 << 4;// [    4] polarity_0
		data32	|=	1;// [	  0] enable
		hdmirx_wr_top(TOP_CHAN_SWITCH_0, data32, port);

		/* Configure BIST generator */
		data32			= 0;
		/* [	8] bist_loopback */
		data32	|=	0 << 8;
		/* [ 7: 5] decoup_thresh */
		data32	|=	3 << 5;
		// [ 4: 3] prbs_gen_mode:
		// 0=prbs11; 1=prbs15; 2=prbs7; 3=prbs31.
		data32	|=	bist_mode << 3;
		/* [ 2: 1] prbs_gen_width:3=10-bit. */
		data32	|=	3 << 1;
		/* [	0] prbs_gen_enable */
		data32	|=	1;
		hdmirx_wr_top(TOP_PRBS_GEN, data32, port);

		/* PRBS analyzer control */
		hdmirx_wr_top(TOP_PRBS_ANA_0, 0xf6f6f6, port);
		usleep_range(100, 110);
		hdmirx_wr_top(TOP_PRBS_ANA_0, 0xf2f2f2, port);

		//if ((hdmirx_rd_top(TOP_PRBS_GEN) & data32) != 0)
			//return;
		usleep_range(5000, 5050);

		/* Check BIST analyzer BER counters */
		if (port == 0)
			rx_pr("BER_CH0 = %x\n",
			      hdmirx_rd_top(TOP_PRBS_ANA_BER_CH0, port));
		else if (port == 1)
			rx_pr("BER_CH1 = %x\n",
			      hdmirx_rd_top(TOP_PRBS_ANA_BER_CH1, port));
		else if (port == 2)
			rx_pr("BER_CH2 = %x\n",
			      hdmirx_rd_top(TOP_PRBS_ANA_BER_CH2, port));

		/* check BIST analyzer result */
		lock_sts = hdmirx_rd_top(TOP_PRBS_ANA_STAT, port) & 0x3f;
		rx_pr("ch%dsts=0x%x\n", port, lock_sts);
		if (port == 0) {
			ch0_lock = lock_sts & 3;
			if (ch0_lock == 1)
				rx_pr("ch0 PASS\n");
			else
				rx_pr("ch0 NG\n");
		}
		if (port == 1) {
			ch1_lock = (lock_sts >> 2) & 3;
			if (ch1_lock == 1)
				rx_pr("ch1 PASS\n");
			else
				rx_pr("ch1 NG\n");
		}
		if (port == 2) {
			ch2_lock = (lock_sts >> 4) & 3;
			if (ch2_lock == 1)
				rx_pr("ch2 PASS\n");
			else
				rx_pr("ch2 NG\n");
		}
		usleep_range(1000, 1100);
	}
	lock_sts = ch0_lock | (ch1_lock << 2) | (ch2_lock << 4);
	if (lock_sts == 0x15)/* lock_sts == b'010101' is PASS*/
		rx_pr("bist_test PASS\n");
	else
		rx_pr("bist_test FAIL\n");
	if (rx_info.aml_phy.long_bist_en)
		rx_pr("long bist done\n");
	else
		rx_pr("short bist done\n");
	if (rx_info.main_port_open)
		rx_info.aml_phy.pre_int = 1;
}

int aml_phy_get_iq_skew_val_t5m(u32 val_0, u32 val_1)
{
	int val = val_0 - val_1;

	rx_pr("val=%d\n", val);
	if (val)
		return (val - 32);
	else
		return (val + 128 - 32);
}

/* IQ skew monitor */
void aml_phy_iq_skew_monitor_t5m(void)
{
}

void aml_phy_power_off_t5m(void)
{
	hdmirx_wr_amlphy(T5M_HDMIRX20PLL_CTRL0, 0x0);
	hdmirx_wr_amlphy(T5M_HDMIRX20PLL_CTRL1, 0x0);
	hdmirx_wr_amlphy(T5M_HDMIRX20PHY_DCHA_AFE, 0x0);
	hdmirx_wr_amlphy(T5M_HDMIRX20PHY_DCHA_DFE, 0x0);
	hdmirx_wr_amlphy(T5M_HDMIRX20PHY_DCHD_CDR, 0x0);
	hdmirx_wr_amlphy(T5M_HDMIRX20PHY_DCHD_EQ, 0x0);
	hdmirx_wr_amlphy(T5M_HDMIRX20PHY_DCHA_MISC1, 0x0);
	hdmirx_wr_amlphy(T5M_HDMIRX20PHY_DCHA_MISC2, 0x0);
	hdmirx_wr_amlphy(T5M_HDMIRX20PHY_DCHD_STAT, 0x0);
	hdmirx_wr_amlphy(T5M_HDMIRX_ARC_CNTL, 0x0);
	hdmirx_wr_amlphy(T5M_HDMIRX_PHY_PROD_TEST0, 0x0);
	hdmirx_wr_amlphy(T5M_HDMIRX_PHY_PROD_TEST1, 0x0);
}

void aml_phy_switch_port_t5m(void)
{
	u32 data32;
	u8 port = rx_info.main_port;

	/* reset and select data port */
	data32 = hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHA_MISC2);
	data32 &= (~(0xf << 24));
	data32 |= ((1 << port) << 24);
	hdmirx_wr_amlphy(T5M_HDMIRX20PHY_DCHA_MISC2, data32);
	hdmirx_wr_bits_top(TOP_PORT_SEL, MSK(4, 0), (1 << port), port);
}

void dump_vsi_reg_t5m(u8 port)
{
	u8 data8, i;

	rx_pr("vsi data:\n");
	for (i = 0; i <= 30; i++) {
		data8 = hdmirx_rd_cor(VSIRX_TYPE_DP3_IVCRX + i, port);
		rx_pr("%d-[%x]\n", i, data8);
	}
	rx_pr("hf-vsi data:\n");
	for (i = 0; i <= 30; i++) {
		data8 = hdmirx_rd_cor(HF_VSIRX_TYPE_DP3_IVCRX + i, port);
		rx_pr("%d-[%x]\n", i, data8);
	}
	rx_pr("aif-vsi data:\n");
	for (i = 0; i <= 30; i++) {
		data8 = hdmirx_rd_cor(AUDRX_TYPE_DP2_IVCRX + i, port);
		rx_pr("%d-[%x]\n", i, data8);
	}
	rx_pr("unrec data:\n");
	for (i = 0; i <= 30; i++) {
		data8 = hdmirx_rd_cor(RX_UNREC_BYTE1_DP2_IVCRX + i, port);
		rx_pr("%d-[%x]\n", i, data8);
	}
}

unsigned int rx_sec_hdcp_cfg_t5m(void)
{
	struct arm_smccc_res res;

	arm_smccc_smc(HDMI_RX_HDCP_CFG, 0, 0, 0, 0, 0, 0, 0, &res);

	return (unsigned int)((res.a0) & 0xffffffff);
}

void rx_set_irq_t5m(bool en, u8 port)
{
	u8 data8;

	if (en) {
		data8 = 0;
		data8 |= 1 << 4; /* intr_new_unrec en */
		data8 |= 1 << 2; /* intr_new_aud */
		data8 |= 1 << 1; /* intr_spd */
		hdmirx_wr_cor(RX_DEPACK_INTR2_MASK_DP2_IVCRX, data8, port);

		data8 = 0;
		data8 |= 1 << 4; /* intr_cea_repeat_hf_vsi en */
		data8 |= 1 << 3; /* intr_cea_new_hf_vsi en */
		data8 |= 1 << 2; /* intr_cea_new_vsi */
		hdmirx_wr_cor(RX_DEPACK_INTR3_MASK_DP2_IVCRX, data8, port);

		hdmirx_wr_cor(RX_GRP_INTR1_MASK_PWD_IVCRX, 0x25, port);
		hdmirx_wr_cor(RX_INTR1_MASK_PWD_IVCRX, 0x03, port);//register_address: 0x1050
		hdmirx_wr_cor(RX_INTR2_MASK_PWD_IVCRX, 0x00, port);//register_address: 0x1051
		hdmirx_wr_cor(RX_INTR3_MASK_PWD_IVCRX, 0x00, port);//register_address: 0x1052
		//must set 0.
		hdmirx_wr_cor(RX_INTR4_MASK_PWD_IVCRX, 0, port);//0x03);//register_address: 0x1053
		hdmirx_wr_cor(RX_INTR5_MASK_PWD_IVCRX, 0x00, port);//register_address: 0x1054
		hdmirx_wr_cor(RX_INTR6_MASK_PWD_IVCRX, 0x00, port);//register_address: 0x1055
		hdmirx_wr_cor(RX_INTR7_MASK_PWD_IVCRX, 0x00, port);//register_address: 0x1056
		hdmirx_wr_cor(RX_INTR8_MASK_PWD_IVCRX, 0x00, port);//register_address: 0x1057
		hdmirx_wr_cor(RX_INTR9_MASK_PWD_IVCRX, 0x00, port);//register_address: 0x1058

		data8 = 0;
		data8 |= 0 << 4; /* end of VSIF EMP data received */
		data8 |= 0 << 3;
		data8 |= 0 << 2;
		hdmirx_wr_cor(RX_DEPACK2_INTR2_MASK_DP0B_IVCRX, data8, port);

		//===for depack interrupt ====
		//hdmirx_wr_cor(CP2PAX_INTR0_MASK_HDCP2X_IVCRX, 0x3, port);
		hdmirx_wr_cor(RX_INTR13_MASK_PWD_IVCRX, 0x02, port);// int
		//hdmirx_wr_cor(RX_PWD_INT_CTRL, 0x00, port);//[1] reg_intr_polarity, default = 1
		//hdmirx_wr_cor(RX_DEPACK_INTR4_MASK_DP2_IVCRX, 0x00, port);//interrupt mask
		//hdmirx_wr_cor(RX_DEPACK2_INTR0_MASK_DP0B_IVCRX, 0x0c, port);//interrupt mask
		//hdmirx_wr_cor(RX_DEPACK_INTR3_MASK_DP2_IVCRX, 0x20, port);//interrupt mask [5] acr
		data8 = 0;
		data8 |= 0 << 2; /* gcp did not arrived 4 frames */
		hdmirx_wr_cor(RX_DEPACK_INTR6_MASK_DP2_IVCRX, data8, port);

		//HDCP irq
		// encrypted sts changed
		//hdmirx_wr_cor(RX_HDCP1X_INTR0_MASK_HDCP1X_IVCRX, 1, port);
		// AKE init received
		//hdmirx_wr_cor(CP2PAX_INTR1_MASK_HDCP2X_IVCRX, 4, port);
		// HDCP 2X_RX_ECC
		hdmirx_wr_cor(HDCP2X_RX_ECC_INTR_MASK, 1, port);
	} else {
		/* clear enable */
		hdmirx_wr_cor(RX_DEPACK_INTR2_MASK_DP2_IVCRX, 0, port);
		/* clear status */
		hdmirx_wr_cor(RX_DEPACK_INTR2_DP2_IVCRX, 0xff, port);
		/* clear enable */
		hdmirx_wr_cor(RX_DEPACK_INTR3_MASK_DP2_IVCRX, 0, port);
		/* clear status */
		hdmirx_wr_cor(RX_DEPACK_INTR3_DP2_IVCRX, 0xff, port);
		/* clear en */
		hdmirx_wr_cor(RX_GRP_INTR1_MASK_PWD_IVCRX, 0, port);
		/* clear status */
		hdmirx_wr_cor(RX_GRP_INTR1_STAT_PWD_IVCRX, 0xff, port);
		/* clear enable */
		hdmirx_wr_cor(RX_INTR1_MASK_PWD_IVCRX, 0, port);//register_address: 0x1050
		/* clear status */
		hdmirx_wr_cor(RX_INTR1_PWD_IVCRX, 0xff, port);
		/* clear enable */
		hdmirx_wr_cor(RX_INTR2_MASK_PWD_IVCRX, 0, port);//register_address: 0x1051
		/* clear status */
		hdmirx_wr_cor(RX_INTR2_PWD_IVCRX, 0xff, port);
		/* clear enable */
		hdmirx_wr_cor(RX_INTR3_MASK_PWD_IVCRX, 0, port);//register_address: 0x1052
		/* clear status */
		hdmirx_wr_cor(RX_INTR3_PWD_IVCRX, 0xff, port);
		/* clear enable */
		hdmirx_wr_cor(RX_INTR4_MASK_PWD_IVCRX, 0, port);//register_address: 0x1053
		/* clear status */
		hdmirx_wr_cor(RX_INTR4_PWD_IVCRX, 0xff, port);
		/* clear enable */
		hdmirx_wr_cor(RX_INTR5_MASK_PWD_IVCRX, 0, port);//register_address: 0x1054
		/* clear status */
		hdmirx_wr_cor(RX_INTR5_PWD_IVCRX, 0xff, port);
		/* clear enable */
		hdmirx_wr_cor(RX_INTR6_MASK_PWD_IVCRX, 0, port);//register_address: 0x1055
		/* clear status */
		hdmirx_wr_cor(RX_INTR6_PWD_IVCRX, 0xff, port);
		/* clear enable */
		hdmirx_wr_cor(RX_INTR7_MASK_PWD_IVCRX, 0, port);//register_address: 0x1056
		/* clear status */
		hdmirx_wr_cor(RX_INTR7_PWD_IVCRX, 0xff, port);
		/* clear enable */
		hdmirx_wr_cor(RX_INTR8_MASK_PWD_IVCRX, 0, port);//register_address: 0x1057
		/* clear status */
		hdmirx_wr_cor(RX_INTR8_PWD_IVCRX, 0xff, port);
		/* clear enable */
		hdmirx_wr_cor(RX_INTR9_MASK_PWD_IVCRX, 0, port);//register_address: 0x1058
		/* clear status */
		hdmirx_wr_cor(RX_INTR9_PWD_IVCRX, 0xff, port);
		/* clear enable */
		hdmirx_wr_cor(RX_DEPACK2_INTR2_MASK_DP0B_IVCRX, 0, port);
		/* clear status */
		hdmirx_wr_cor(RX_DEPACK2_INTR2_DP0B_IVCRX, 0xff, port);
		//===for depack interrupt ====
		//hdmirx_wr_cor(CP2PAX_INTR0_MASK_HDCP2X_IVCRX, 0x3, port);
		//hdmirx_wr_cor(RX_INTR13_MASK_PWD_IVCRX, 0x02, port);// int
		//hdmirx_wr_cor(RX_PWD_INT_CTRL, 0x00, port);//[1] reg_intr_polarity, default = 1
		/* clear status */
		hdmirx_wr_cor(RX_DEPACK_INTR2_DP2_IVCRX, 0xff, port);
		//hdmirx_wr_cor(RX_DEPACK_INTR4_MASK_DP2_IVCRX, 0x00, port);//interrupt mask
		//hdmirx_wr_cor(RX_DEPACK2_INTR0_MASK_DP0B_IVCRX, 0x0c, port);//interrupt mask
		//hdmirx_wr_cor(RX_DEPACK_INTR3_MASK_DP2_IVCRX, 0x20, port);
		//gcp
		hdmirx_wr_cor(RX_DEPACK_INTR6_DP2_IVCRX, 0xff, port);
		hdmirx_wr_cor(RX_DEPACK_INTR6_MASK_DP2_IVCRX, 0, port);
		//interrupt mask [5] acr

		//HDCP irq
		// encrypted sts changed
		//hdmirx_wr_cor(RX_HDCP1X_INTR0_MASK_HDCP1X_IVCRX, 0, port);
		// AKE init received
		//hdmirx_wr_cor(CP2PAX_INTR1_MASK_HDCP2X_IVCRX, 0, port);
		// HDCP 2X_RX_ECC
		hdmirx_wr_cor(HDCP2X_RX_ECC_INTR_MASK, 0, port);
	}
}

/*
 * 0 SPDIF
 * 1  I2S
 * 2  TDM
 */
void rx_set_aud_output_t5m(u32 param)
{
	u8 port = rx_info.main_port;

	if (param == 2) {
		hdmirx_wr_cor(RX_TDM_CTRL1_AUD_IVCRX, 0x0f, port);
		hdmirx_wr_cor(RX_TDM_CTRL2_AUD_IVCRX, 0xff, port);
		hdmirx_wr_cor(AAC_MCLK_SEL_AUD_IVCRX, 0x90, port); //TDM
	} else if (param == 1) {
		hdmirx_wr_cor(RX_TDM_CTRL1_AUD_IVCRX, 0x00, port);
		hdmirx_wr_cor(RX_TDM_CTRL2_AUD_IVCRX, 0x10, port);
		hdmirx_wr_cor(AAC_MCLK_SEL_AUD_IVCRX, 0x80, port); //I2S
		//hdmirx_wr_bits_top(TOP_CLK_CNTL, _BIT(15), 0);
		hdmirx_wr_bits_top(TOP_CLK_CNTL, _BIT(4), 1, port);
	} else {
		hdmirx_wr_cor(RX_TDM_CTRL1_AUD_IVCRX, 0x00, port);
		hdmirx_wr_cor(RX_TDM_CTRL2_AUD_IVCRX, 0x10, port);
		hdmirx_wr_cor(AAC_MCLK_SEL_AUD_IVCRX, 0x80, port); //SPDIF
		//hdmirx_wr_bits_top(TOP_CLK_CNTL, _BIT(15), 1);
		hdmirx_wr_bits_top(TOP_CLK_CNTL, _BIT(4), 0, port);
	}
}

void rx_sw_reset_t5m(int level)
{
	u8 port = rx_info.main_port;

	/* deep color fifo */
	hdmirx_wr_bits_cor(RX_PWD_SRST_PWD_IVCRX, _BIT(4), 1, port);
	udelay(1);
	hdmirx_wr_bits_cor(RX_PWD_SRST_PWD_IVCRX, _BIT(4), 0, port);
	//TODO..
}

void hdcp_init_t5m(void)
{
	u8 port = rx_info.main_port;
	u8 data8;
	//key config and crc check
	//rx_sec_hdcp_cfg_t5m();
	//hdcp config

	//======================================
	// HDCP 2.X Config ---- RX
	//======================================
	hdmirx_wr_cor(RX_HPD_C_CTRL_AON_IVCRX, 0x1, port);//HPD
	hdmirx_wr_cor(SCDCS_100MS_IN_1MS_CNT_SCDC_IVCRX, 0x1, port);
	//todo: enable hdcp22 according hdcp burning
	hdmirx_wr_cor(RX_HDCP2x_CTRL_PWD_IVCRX, 0x01, port);//ri_hdcp2x_en
	//hdmirx_wr_cor(RX_INTR13_MASK_PWD_IVCRX, 0x02, port);// irq
	hdmirx_wr_cor(PWD_SW_CLMP_AUE_OIF_PWD_IVCRX, 0x0, port);

	data8 = 0;
	data8 |= (hdmirx_repeat_support() && rx[port].hdcp.repeat) << 1;
	hdmirx_wr_cor(CP2PAX_CTRL_0_HDCP2X_IVCRX, data8, port);
	//depth
	hdmirx_wr_cor(CP2PAX_RPT_DEPTH_HDCP2X_IVCRX, 0, port);
	//dev cnt
	hdmirx_wr_cor(CP2PAX_RPT_DEVCNT_HDCP2X_IVCRX, 0, port);
	//
	data8 = 0;
	data8 |= 0 << 0; //hdcp1dev
	data8 |= 0 << 1; //hdcp1dev
	data8 |= 0 << 2; //max_casc
	data8 |= 0 << 3; //max_devs
	hdmirx_wr_cor(CP2PAX_RPT_DETAIL_HDCP2X_IVCRX, data8, port);

	hdmirx_wr_cor(CP2PAX_RX_CTRL_0_HDCP2X_IVCRX, 0x83, port);
	hdmirx_wr_cor(CP2PAX_RX_CTRL_0_HDCP2X_IVCRX, 0x80, port);

	//======================================
	// HDCP 1.X Config ---- RX
	//======================================
	hdmirx_wr_cor(RX_SYS_SWTCHC_AON_IVCRX, 0x86, port);//SYS_SWTCHC,Enable HDCP DDC,SCDC DDC

	//----clear ksv fifo rdy --------
	data8  =  0;
	data8 |= (1 << 3);//bit[  3] reg_hdmi_clr_en
	data8 |= (7 << 0);//bit[2:0] reg_fifordy_clr_en
	hdmirx_wr_cor(RX_RPT_RDY_CTRL_PWD_IVCRX, data8, port);//register address: 0x1010 (0x0f)

	//----BCAPS config-----
	data8 = 0;
	data8 |= (0 << 4);//bit[4] reg_fast I2C transfers speed.
	data8 |= (0 << 5);//bit[5] reg_fifo_rdy
	data8 |= ((hdmirx_repeat_support() &&
		rx[port].hdcp.repeat) << 6);//bit[6] reg_repeater
	data8 |= (1 << 7);//bit[7] reg_hdmi_capable  HDMI capable
	hdmirx_wr_cor(RX_BCAPS_SET_HDCP1X_IVCRX, data8, port);//register address: 0x169e (0x80)

	//for (data8 = 0; data8 < 10; data8++) //ksv list number
		//hdmirx_wr_cor(RX_KSV_FIFO_HDCP1X_IVCRX, ksvlist[data8], port);

	//----Bstatus1 config-----
	data8 =  0;
	// data8 |= (2 << 0); //bit[6:0] reg_dev_cnt
	data8 |= (0 << 7);//bit[  7] reg_dve_exceed
	hdmirx_wr_cor(RX_SHD_BSTATUS1_HDCP1X_IVCRX, data8, port);//register address: 0x169f (0x00)

		//----Bstatus2 config-----
	data8 =  0;
	// data8 |= (2 << 0);//bit[2:0] reg_depth
	data8 |= (0 << 3);//bit[  3] reg_casc_exceed
	hdmirx_wr_cor(RX_SHD_BSTATUS2_HDCP1X_IVCRX, data8, port);//register address: 0x169f (0x00)

	//----Rx Sha length in bytes----
	hdmirx_wr_cor(RX_SHA_length1_HDCP1X_IVCRX, 0x0a, port);//[7:0] 10=2ksv*5byte
	hdmirx_wr_cor(RX_SHA_length2_HDCP1X_IVCRX, 0x00, port);//[9:8]

	//----Rx Sha repeater KSV fifo start addr----
	hdmirx_wr_cor(RX_KSV_SHA_start1_HDCP1X_IVCRX, 0x00, port);//[7:0]
	hdmirx_wr_cor(RX_KSV_SHA_start2_HDCP1X_IVCRX, 0x00, port);//[9:8]
	//hdmirx_wr_cor(CP2PAX_INTR0_MASK_HDCP2X_IVCRX, 0x3, port); irq
	//hdmirx_wr_cor(RX_HDCP2x_CTRL_PWD_IVCRX, 0x1, port); //same as L3309
	//hdmirx_wr_cor(CP2PA_TP1_HDCP2X_IVCRX, 0x9e, port);
	//hdmirx_wr_cor(CP2PA_TP3_HDCP2X_IVCRX, 0x32, port);
	//hdmirx_wr_cor(CP2PA_TP5_HDCP2X_IVCRX, 0x32, port);
	//hdmirx_wr_cor(CP2PAX_GP_IN1_HDCP2X_IVCRX, 0x2, port);
	//hdmirx_wr_cor(CP2PAX_GP_CTL_HDCP2X_IVCRX, 0xdb, port);
	hdmirx_wr_cor(RX_PWD_SRST2_PWD_IVCRX, 0x8, port);
	hdmirx_wr_cor(RX_PWD_SRST2_PWD_IVCRX, 0x2, port);
}

void clk_init_cor_t5m(void)
{
	u32 data32;
	u8 port = rx_info.main_port;

	/* Turn on clk_hdmirx_pclk, also = sysclk */
	wr_reg_clk_ctl(CLKCTRL_SYS_CLK_EN0_REG2,
		       rd_reg_clk_ctl(CLKCTRL_SYS_CLK_EN0_REG2) | (1 << 9));

	data32	= 0;
	data32 |= (0 << 25);// [26:25] clk_sel for cts_hdmirx_2m_clk: 0=cts_oscin_clk
	data32 |= (0 << 24);// [   24] clk_en for cts_hdmirx_2m_clk
	data32 |= (11 << 16);// [22:16] clk_div for cts_hdmirx_2m_clk: 24/12=2M
	data32 |= (3 << 9);// [10: 9] clk_sel for cts_hdmirx_5m_clk: 3=fclk_div5
	data32 |= (0 << 8);// [    8] clk_en for cts_hdmirx_5m_clk
	data32 |= (79 << 0);// [ 6: 0] clk_div for cts_hdmirx_5m_clk: fclk_dvi5/80=400/80=5M
	wr_reg_clk_ctl(RX_CLK_CTRL, data32);
	data32 |= (1 << 24);// [   24] clk_en for cts_hdmirx_2m_clk
	data32 |= (1 << 8);// [    8] clk_en for cts_hdmirx_5m_clk
	wr_reg_clk_ctl(RX_CLK_CTRL, data32);

	data32  = 0;
	data32 |= (3 << 25);// [26:25] clk_sel for cts_hdmirx_hdcp2x_eclk: 3=fclk_div5
	data32 |= (0 << 24);// [   24] clk_en for cts_hdmirx_hdcp2x_eclk
	data32 |= (15 << 16);// [22:16] clk_div for cts_hdmirx_hdcp2x_eclk:
	//fclk_dvi5/16=400/16=25M
	data32 |= (3 << 9);// [10: 9] clk_sel for cts_hdmirx_cfg_clk: 3=fclk_div5
	data32 |= (0 << 8);// [    8] clk_en for cts_hdmirx_cfg_clk
	data32 |= (7 << 0);// [ 6: 0] clk_div for cts_hdmirx_cfg_clk: fclk_dvi5/8=400/8=50M
	wr_reg_clk_ctl(RX_CLK_CTRL1, data32);
	data32 |= (1 << 24);// [   24] clk_en for cts_hdmirx_hdcp2x_eclk
	data32 |= (1 << 8);// [    8] clk_en for cts_hdmirx_cfg_clk
	wr_reg_clk_ctl(RX_CLK_CTRL1, data32);

	data32  = 0;
	data32 |= (1 << 25);// [26:25] clk_sel for cts_hdmirx_acr_ref_clk: 1=fclk_div4
	data32 |= (0 << 24);// [   24] clk_en for cts_hdmirx_acr_ref_clk
	data32 |= (0 << 16);// [22:16] clk_div for cts_hdmirx_acr_ref_clk://fclk_div4/1=500M
	data32 |= (0 << 9);// [10: 9] clk_sel for cts_hdmirx_aud_pll_clk
	data32 |= (0 << 8);// [    8] clk_en for cts_hdmirx_aud_pll_clk
	data32 |= (0 << 0);// [ 6: 0] clk_div for cts_hdmirx_aud_pll_clk
	wr_reg_clk_ctl(RX_CLK_CTRL2, data32);
	data32 |= (1 << 24);// [   24] clk_en for cts_hdmirx_acr_ref_clk
	data32 |= (1 << 8);// [    8] clk_en for cts_hdmirx_aud_pll_clk
	wr_reg_clk_ctl(RX_CLK_CTRL2, data32);

	data32  = 0;
	data32 |= (0 << 9);// [10: 9] clk_sel for cts_hdmirx_meter_clk: 0=cts_oscin_clk
	data32 |= (0 << 8);// [    8] clk_en for cts_hdmirx_meter_clk
	data32 |= (0 << 0);// [ 6: 0] clk_div for cts_hdmirx_meter_clk: 24M
	wr_reg_clk_ctl(RX_CLK_CTRL3, data32);
	data32 |= (1 << 8);// [    8] clk_en for cts_hdmirx_meter_clk
	wr_reg_clk_ctl(RX_CLK_CTRL3, data32);

	data32  = 0;
	data32 |= (0 << 31);// [31]	  free_clk_en
	data32 |= (0 << 15);// [15]	  hbr_spdif_en
	data32 |= (0 << 8);// [8]	  tmds_ch2_clk_inv
	data32 |= (0 << 7);// [7]	  tmds_ch1_clk_inv
	data32 |= (0 << 6);// [6]	  tmds_ch0_clk_inv
	data32 |= (0 << 5);// [5]	  pll4x_cfg
	data32 |= (0 << 4);// [4]	  force_pll4x
	data32 |= (0 << 3);// [3]	  phy_clk_inv
	hdmirx_wr_top(TOP_CLK_CNTL, data32, port);
}

void rx_dig_clk_en_t5m(bool en)
{
	hdmirx_wr_bits_clk_ctl(RX_CLK_CTRL, CLK_2M_EN, en);
	hdmirx_wr_bits_clk_ctl(RX_CLK_CTRL, CLK_5M_EN, en);
	hdmirx_wr_bits_clk_ctl(RX_CLK_CTRL1, HDCP2X_ECLK_EN, en);
	hdmirx_wr_bits_clk_ctl(RX_CLK_CTRL1, CFG_CLK_EN, en);
	hdmirx_wr_bits_clk_ctl(RX_CLK_CTRL3, METER_CLK_EN, en);
}

