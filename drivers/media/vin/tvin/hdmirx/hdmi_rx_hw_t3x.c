// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2021 Amlogic, Inc. All rights reserved.
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
#include <linux/amlogic/clk_measure.h>

/* Local Include */
#include "hdmi_rx_repeater.h"
#include "hdmi_rx_drv.h"
#include "hdmi_rx_hw.h"
#include "hdmi_rx_wrapper.h"
#include "hdmi_rx_hw_t3x.h"

/* FT trim flag:1-valid, 0-not valid */
bool rterm_trim_flag_t3x;
/* FT trim value 4 bits */
u32 rterm_trim_val_t3x;
int frl_scrambler_en = 1;
/* 0=disable;1=3G3Lanes;2=6G3Lanes;3=6G4Lanes; */
/* 4=8G4Lanes;5=10G4Lanes;6=12G4Lanes */
//int frl_rate;
int phy_rate;
//for t3x debug,todo
u32 frl_sync_cnt = 300;
u32 odn_reg_n_mul = 6;
int vpcore_debug = 3;
u32 ext_cnt = 2000;
int tr_delay0 = 10;
int tr_delay1 = 10;
int frate_cnt = 100;
int tuning_cnt = 20;
int fpll_sel = 1;
/* bit'0 clk_ready, bit'1 overlap */
int fpll_chk_lvl = 0x1;
int valid_m_wait_max = 800;
int vga_tuning_min = 0x21;
int vga_tuning_max = 0x26;
int cal_phy_time;
enum frl_train_sts_e frl_train_sts = E_FRL_TRAIN_START;
enum frl_train_sts_e frl_train_sts1 = E_FRL_TRAIN_START;

/* i2c monitor */
#define I2C_BUFF_SIZE 0x1000
/* for T3X 2.0 */
static const u32 phy_misc_t3x_20[][2] = {
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
	{	 /* 115~150M */
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

static const u32 phy_dcha_t3x_20[][2] = {
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
		0x00f77666, 0x40100459,
	},
	{	 /* 340~525M */
		0x00f73666, 0x7ff00459,
	},
	{	 /* 525~600M */
		0x02821666, 0x7ff00459,
	},
};

static const u32 phy_dchd_t3x_20[][2] = {
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
		0x04080095, 0x30e80469,
	},
	{	 /* 340~525M */
		0x04080093, 0x30e00469,
	},
	{	 /* 525~600M */
		0x04080093, 0x30e00469,
	},
};

u32 t3x_rlevel[] = {8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7};

/*Decimal to Gray code */
unsigned int decimaltogray_t3x(unsigned int x)
{
	return x ^ (x >> 1);
}

/* Gray code to Decimal */
unsigned int graytodecimal_t3x(unsigned int x)
{
	unsigned int y = x;

	while (x >>= 1)
		y ^= x;
	return y;
}

bool is_pll_lock_t3x(u8 port)
{
	return ((hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PLL_CTRL0, port) >> 31) & 0x1);
}

void t3x_480p_pll_cfg_20(u8 port)
{
	/* the times of pll = 80 for debug */
//	hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_0, 0x05305000);
//	usleep_range(10, 20);
//	hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_1, 0x01481236);
//	usleep_range(10, 20);
//	hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_0, 0x05305001);
//	usleep_range(10, 20);
//	hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_0, 0x05305003);
//	usleep_range(10, 20);
//	hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_1, 0x01401236);
//	usleep_range(10, 20);
//	hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_0, 0x05305007);
//	usleep_range(10, 20);
//	hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_0, 0x45305007);
//	usleep_range(10, 20);
	/*the times of pll = 160 */
	hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_0, 0x0530a000, port);
	usleep_range(10, 20);
	hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_1, 0x01481236, port);
	usleep_range(10, 20);
	hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_0, 0x0530a001, port);
	usleep_range(10, 20);
	hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_0, 0x0530a003, port);
	usleep_range(10, 20);
	hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_1, 0x41401236, port);
	usleep_range(10, 20);
	hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_0, 0x0530a007, port);
	usleep_range(10, 20);
	hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_0, 0x4530a007, port);
	usleep_range(10, 20);
	rx[port].phy.aud_div = 0;
	rx[port].phy.aud_div_1 = 0x408;
}

void t3x_720p_pll_cfg_20(u8 port)
{
	hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_0, 0x05305000, port);
	usleep_range(10, 20);
	hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_1, 0x01481236, port);
	usleep_range(10, 20);
	hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_0, 0x05305001, port);
	usleep_range(10, 20);
	hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_0, 0x05305003, port);
	usleep_range(10, 20);
	hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_1, 0x61401236, port);
	usleep_range(10, 20);
	hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_0, 0x05305007, port);
	usleep_range(10, 20);
	hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_0, 0x45305007, port);
	usleep_range(10, 20);
	rx[port].phy.aud_div = 0;
	rx[port].phy.aud_div_1 = 0x8;
}

void t3x_1080p_pll_cfg_20(u8 port)
{
	hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_0, 0x05302800, port);
	usleep_range(10, 20);
	hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_1, 0x01481236, port);
	usleep_range(10, 20);
	hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_0, 0x05302801, port);
	usleep_range(10, 20);
	hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_0, 0x05302803, port);
	usleep_range(10, 20);
	hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_1, 0x41401236, port);
	usleep_range(10, 20);
	hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_0, 0x05302807, port);
	usleep_range(10, 20);
	hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_0, 0x45302807, port);
	usleep_range(10, 20);
	rx[port].phy.aud_div = 0;
	rx[port].phy.aud_div_1 = 0x8;
}

void t3x_4k30_pll_cfg_20(u8 port)
{
	hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_0, 0x05302810, port);
	usleep_range(10, 20);
	hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_1, 0x01481236, port);
	usleep_range(10, 20);
	hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_0, 0x05302811, port);
	usleep_range(10, 20);
	hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_0, 0x05302813, port);
	usleep_range(10, 20);
	hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_1, 0x21401236, port);
	usleep_range(10, 20);
	hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_0, 0x05302817, port);
	usleep_range(10, 20);
	hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_0, 0x45302817, port);
	usleep_range(10, 20);
	rx[port].phy.aud_div = 0;
	rx[port].phy.aud_div_1 = 0x8;
}

void t3x_4k60_pll_cfg_20(u8 port)
{
	if (rx[port].clk.cable_clk > 300 && rx[port].clk.cable_clk < 340) {
		hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_0, 0x05302820, port);
		usleep_range(10, 20);
		hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_1, 0x01481236, port);
		usleep_range(10, 20);
		hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_0, 0x05302821, port);
		usleep_range(10, 20);
		hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_0, 0x05302823, port);
		usleep_range(10, 20);
		hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_1, 0x21401236, port);
		usleep_range(10, 20);
		hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_0, 0x05302827, port);
		usleep_range(10, 20);
		hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_0, 0x45302827, port);
		usleep_range(10, 20);
	} else {
		hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_0, 0x05302800, port);
		usleep_range(10, 20);
		hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_1, 0x01481236, port);
		usleep_range(10, 20);
		hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_0, 0x05302801, port);
		usleep_range(10, 20);
		hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_0, 0x45302803, port);
		usleep_range(10, 20);
		hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_1, 0x01401236, port);
		usleep_range(10, 20);
		hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_0, 0x05302807, port);
		usleep_range(10, 20);
		hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_0, 0x45302807, port);
	}
	usleep_range(10, 20);
	rx[port].phy.aud_div = 0;
	rx[port].phy.aud_div_1 = 0x8;
}

void aml_pll_bw_cfg_t3x_20(u8 port)
{
	u32 idx = rx[port].phy.phy_bw;
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
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_MISC2, T3X_20_PLL_CLK_SEL, 1, port);
		/* t3x: select tmds_clk from tclk or tmds_ch_clk */
		/* cdr = tmds_ch_ck,  vco =tclk */
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_MISC1, T3X_20_VCO_TMDS_EN, 0, port);
	}
	switch (idx) {
	case PLL_BW_0:
		t3x_480p_pll_cfg_20(port);
		break;
	case PLL_BW_1:
		t3x_720p_pll_cfg_20(port);
		break;
	case PLL_BW_2:
		t3x_1080p_pll_cfg_20(port);
		break;
	case PLL_BW_3:
		t3x_4k30_pll_cfg_20(port);
		break;
	case PLL_BW_4:
		t3x_4k60_pll_cfg_20(port);
		break;
	}
	/* do 5 times when clk not stable within a interrupt */
	do {
		if (idx == PLL_BW_0)
			t3x_480p_pll_cfg_20(port);
		if (idx == PLL_BW_1)
			t3x_720p_pll_cfg_20(port);
		if (idx == PLL_BW_2)
			t3x_1080p_pll_cfg_20(port);
		if (idx == PLL_BW_3)
			t3x_4k30_pll_cfg_20(port);
		if (idx == PLL_BW_4)
			t3x_4k60_pll_cfg_20(port);
		if (log_level & PHY_LOG)
			rx_pr("PLL0=0x%x\n", hdmirx_rd_amlphy_t3x(T3X_RG_RX20PLL_0, port));
		if (pll_rst_cnt++ > pll_rst_max) {
			if (log_level & VIDEO_LOG)
				rx_pr("pll rst error\n");
			break;
		}
		if (log_level & VIDEO_LOG) {
			rx_pr("sq=%d,pll_lock=%d",
			      hdmirx_rd_top(TOP_MISC_STAT0_T3X, port) & 0x1,
			      is_pll_lock_t3x(port));
		}
	} while (!is_tmds_clk_stable(port) && is_clk_stable(port) && !aml_phy_pll_lock(0));
	if (log_level & PHY_LOG)
		rx_pr("pll done\n");
	/* t3x debug */
	/* manual VGA mode for debug,hyper gain=1 */
	if (rx_info.aml_phy.vga_gain <= 0xfff) {
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_AFE, MSK(12, 0),
				      (decimaltogray_t3x(rx_info.aml_phy.vga_gain & 0x7) |
				      (decimaltogray_t3x(rx_info.aml_phy.vga_gain
				      >> 4 & 0x7) << 4) |
				      (decimaltogray_t3x(rx_info.aml_phy.vga_gain
				      >> 8 & 0x7) << 8)),
				      port);
	}
	/* manual EQ mode for debug */
	if (rx_info.aml_phy.eq_stg1 < 0x1f) {
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ,
				      T3X_20_BYP_EQ, rx_info.aml_phy.eq_stg1 & 0x1f, port);
		/* eq adaptive:0-adaptive 1-manual */
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_EN_BYP_EQ, 1, port);
	}
	/*tap2 byp*/
	if (rx_info.aml_phy.tap2_byp && rx[port].phy.phy_bw >= PHY_BW_3)
		/* dfe_tap_en [28:20]*/
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_DFE, _BIT(22), 0, port);
}

int get_tap2_t3x_20(int val)
{
	if ((val >> 4) == 0)
		return val;
	else
		return (0 - (val & 0xf));
}

//for further using,not used now,keep it
bool is_dfe_sts_ok_t3x(u8 port)
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

	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_EHM_DBG_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_STATUS_MUX_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_DFE_OFST_DBG_SEL, 0x2, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_STAT, port);
	dfe0_tap2 = data32 & 0xf;
	dfe1_tap2 = (data32 >> 8) & 0xf;
	dfe2_tap2 = (data32 >> 16) & 0xf;
	if (dfe0_tap2 >= 8 ||
	    dfe1_tap2 >= 8 ||
	    dfe2_tap2 >= 8)
		ret = false;

	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_EHM_DBG_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_STATUS_MUX_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_DFE_OFST_DBG_SEL, 0x3, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_STAT, port);
	dfe0_tap3 = data32 & 0x7;
	dfe1_tap3 = (data32 >> 8) & 0x7;
	dfe2_tap3 = (data32 >> 16) & 0x7;
	if (dfe0_tap3 >= 6 ||
	    dfe1_tap3 >= 6 ||
	    dfe2_tap3 >= 6)
		ret = false;

	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_EHM_DBG_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_STATUS_MUX_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_DFE_OFST_DBG_SEL, 0x4, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_STAT, port);
	dfe0_tap4 = data32 & 0x7;
	dfe1_tap4 = (data32 >> 8) & 0x7;
	dfe2_tap4 = (data32 >> 16) & 0x7;
	if (dfe0_tap4 >= 6 ||
	    dfe1_tap4 >= 6 ||
	    dfe2_tap4 >= 6)
		ret = false;

	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_EHM_DBG_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_STATUS_MUX_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_DFE_OFST_DBG_SEL, 0x5, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_STAT, port);
	dfe0_tap5 = data32 & 0x7;
	dfe1_tap5 = (data32 >> 8) & 0x7;
	dfe2_tap5 = (data32 >> 16) & 0x7;
	if (dfe0_tap5 >= 6 ||
	    dfe1_tap5 >= 6 ||
	    dfe2_tap5 >= 6)
		ret = false;

	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_EHM_DBG_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_STATUS_MUX_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_DFE_OFST_DBG_SEL, 0x6, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_STAT, port);
	dfe0_tap6 = data32 & 0x7;
	dfe1_tap6 = (data32 >> 8) & 0x7;
	dfe2_tap6 = (data32 >> 16) & 0x7;
	if (dfe0_tap6 >= 6 ||
	    dfe1_tap6 >= 6 ||
	    dfe2_tap6 >= 6)
		ret = false;

	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_EHM_DBG_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_STATUS_MUX_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_DFE_OFST_DBG_SEL, 0x7, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_STAT, port);
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

//for further using,not used now,keep it
/* long cable detection for <3G need to be change */
void aml_phy_long_cable_det_t3x(u8 port)
{
	int tap2_0, tap2_1, tap2_2;
	int tap2_max = 0;
	u32 data32 = 0;

	if (rx[port].phy.phy_bw > PHY_BW_3)
		return;
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_EHM_DBG_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_STATUS_MUX_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_DFE_OFST_DBG_SEL, 0x2, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_STAT, port);
	tap2_0 = get_tap2_t3x_20(data32 & 0x1f);
	tap2_1 = get_tap2_t3x_20(((data32 >> 8) & 0x1f));
	tap2_2 = get_tap2_t3x_20(((data32 >> 16) & 0x1f));
	if (rx[port].phy.phy_bw == PHY_BW_2) {
		/*disable DFE*/
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_DFE_RSTB, 0, port);
		tap2_max = 6;
	} else if (rx[port].phy.phy_bw == PHY_BW_3) {
		tap2_max = 10;
	}
	if ((tap2_0 + tap2_1 + tap2_2) >= tap2_max) {
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_BYP_EQ, 0x12, port);
		usleep_range(10, 20);
		rx_pr("long cable\n");
	}
}

/* aml_hyper_gain_tuning */
void aml_hyper_gain_tuning_t3x_20(u8 port)
{
	u32 data32;
	u32 tap0, tap1, tap2;
	u32 hyper_gain_0, hyper_gain_1, hyper_gain_2;
	int eq_boost0, eq_boost1, eq_boost2;

	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_EHM_DBG_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_STATUS_MUX_SEL, 0x3, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_STAT, port);
	eq_boost0 = data32 & 0x1f;
	eq_boost1 = (data32 >> 8)  & 0x1f;
	eq_boost2 = (data32 >> 16)	& 0x1f;
	/* use HYPER_GAIN calibration instead of vga */
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_EHM_DBG_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_STATUS_MUX_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_DFE_OFST_DBG_SEL, 0x0, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_STAT, port);

	tap0 = data32 & 0xff;
	tap1 = (data32 >> 8) & 0xff;
	tap2 = (data32 >> 16) & 0xff;
	if ((rx_info.aml_phy.eq_en && eq_boost0 < 3) || tap0 < 0x12) {
		hyper_gain_0 = 1;
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_AFE,
					  T3X_20_LEQ_HYPER_GAIN_CH0,
					  hyper_gain_0,
					  port);
		if (log_level & PHY_LOG)
			rx_pr("ch0 hyper gain triger\n");
	}
	if ((rx_info.aml_phy.eq_en && eq_boost1 < 3) || tap1 < 0x12) {
		hyper_gain_1 = 1;
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_AFE,
					  T3X_20_LEQ_HYPER_GAIN_CH1,
					  hyper_gain_1,
					  port);
		if (log_level & PHY_LOG)
			rx_pr("ch1 hyper gain triger\n");
	}
	if ((rx_info.aml_phy.eq_en && eq_boost2 < 3) || tap2 < 0x12) {
		hyper_gain_2 = 1;
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_AFE,
					  T3X_20_LEQ_HYPER_GAIN_CH2,
					  hyper_gain_2,
					  port);
		if (log_level & PHY_LOG)
			rx_pr("ch2 hyper gain triger\n");
	}
}

int max_offset_t3x_20(int a, int b, int c)
{
	if (a >= b && a >= c)
		return 0;
	if (b >= a && b >= c)
		return 1;
	if (c >= a && c >= b)
		return 2;
	return -1;
}

void aml_eq_retry_t3x_20(u8 port)
{
	u32 data32 = 0;
	u32 eq_boost0, eq_boost1, eq_boost2;

	if (rx[port].phy.phy_bw >= PHY_BW_3) {
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR,
			T3X_20_EHM_DBG_SEL, 0x0, port);
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ,
		T3X_20_STATUS_MUX_SEL, 0x3, port);
		usleep_range(100, 110);
		data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_STAT, port);
		eq_boost0 = data32 & 0x1f;
		eq_boost1 = (data32 >> 8)  & 0x1f;
		eq_boost2 = (data32 >> 16)	& 0x1f;
		if (eq_boost0 == 0 || eq_boost0 == 31 ||
		    eq_boost1 == 0 || eq_boost1 == 31 ||
		    eq_boost2 == 0 || eq_boost2 == 31) {
			rx_pr("eq_retry:%d-%d-%d\n", eq_boost0, eq_boost1, eq_boost2);
			hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ,
				T3X_20_EQ_EN, 1, port);
			hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ,
				T3X_20_EQ_RSTB, 0x0, port);
			hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR,
				T3X_20_CDR_RSTB, 0x1, port);
			usleep_range(100, 110);
			hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ,
				T3X_20_EQ_RSTB, 0x1, port);
			hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR,
				T3X_20_CDR_RSTB, 0x1, port);
			usleep_range(10000, 10100);
			if (rx_info.aml_phy.eq_hold)
				hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_EQ_EN,
				0, port);
		}
	}
}

void aml_dfe_en_t3x_20(u8 port)
{
	if (rx_info.aml_phy.dfe_en) {
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_DFE_EN, 1, port);
		//if (rx_info.aml_phy.eq_hold)
			//hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_EQ_MODE, 1);
		if (rx_info.aml_phy.eq_retry)
			aml_eq_retry_t3x_20(port);
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_DFE_RSTB, 0, port);
		usleep_range(10, 20);
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ,
				      T3X_20_DFE_RSTB, 1, port);
		usleep_range(200, 220);
		if (rx_info.aml_phy.dfe_hold)
			hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ,
					      T3X_20_DFE_HOLD_EN, 1, port);
		if (log_level & PHY_LOG)
			rx_pr("dfe\n");
	}
}

/* phy offset calibration based on different chip and board */
void aml_phy_offset_cal_t3x_20(u8 port)
{
	/* PHY */
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, 0x70080050, port);
	usleep_range(10, 20);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, 0x04008013, port);
	usleep_range(10, 20);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_DFE, 0x40102459, port);
	usleep_range(10, 20);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_AFE, 0x02821666, port);
	usleep_range(10, 20);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_MISC2, 0x11c73220, port);
	usleep_range(10, 20);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_MISC1, 0xffe00100, port);
	usleep_range(10, 20);

	/* PLL */
	hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_0, 0x0500f800, port);
	usleep_range(10, 20);
	hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_1, 0x01481236, port);
	usleep_range(10, 20);
	hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_0, 0x0500f801, port);
	usleep_range(10, 20);
	hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_0, 0x0500f803, port);
	usleep_range(10, 20);
	hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_1, 0x01401236, port);
	usleep_range(10, 20);
	hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_0, 0x0500f807, port);
	usleep_range(10, 20);
	hdmirx_wr_amlphy_t3x(T3X_RG_RX20PLL_0, 0x4500f807, port);
	usleep_range(100, 200);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, _BIT(26), 1, port);
	usleep_range(10, 20);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, MSK(2, 12), 0X3, port);
	usleep_range(10, 20);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, _BIT(27), 1, port);
	usleep_range(200, 210);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, _BIT(27), 0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_DFE, _BIT(13), 0, port);
	if (log_level & PHY_LOG)
		rx_pr("ofst cal\n");
}

u32 min_ch_t3x_20(u32 a, u32 b, u32 c)
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
u32 aml_eq_eye_monitor_t3x_20(u8 port)
{
	u32 data32;
	u32 positive_eye_height0, positive_eye_height1, positive_eye_height2;

	usleep_range(50, 100);
	/* hold dfe tap1~tap8 */
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ,
			      T3X_20_DFE_HOLD_EN, 1, port);
	usleep_range(10, 20);
	/* disable hw scan mode */
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ,
			      T3X_20_EHM_HW_SCAN_EN, 0, port);
	usleep_range(10, 20);

	/* enable hw scan mode */
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ,
			      T3X_20_EHM_HW_SCAN_EN, 1, port);

	/* wait for scan done */
	usleep_range(rx_info.aml_phy.eye_delay, rx_info.aml_phy.eye_delay + 100);

	/* Check status */
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR,
			      T3X_20_EHM_DBG_SEL, 1, port);
	/* positive eye height  */
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR,
			      T3X_20_EHM_DBG_SEL, 1, port);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_STAT, port);
	positive_eye_height0 = data32 & 0xff;
	positive_eye_height1 = (data32 >> 8) & 0xff;
	positive_eye_height2 = (data32 >> 16) & 0xff;
	/* exit eye monitor scan mode */
	/* disable hw scan mode */
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ,
			      T3X_20_EHM_HW_SCAN_EN, 0, port);
	/* disable eye monitor */
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR,
			     T3X_20_EHM_DBG_SEL, 0, port);
	//hdmirx_wr_bits_amlphy_t3x(T7_HHI_RX_PHY_DCHA_CNTL2,
			      //T7_EYE_MONITOR_EN1, 0);
	usleep_range(10, 20);
	/* release dfe */
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ,
			      T3X_20_DFE_HOLD_EN, 0, port);
	positive_eye_height0 = positive_eye_height0 & 0x3f;
	positive_eye_height1 = positive_eye_height1 & 0x3f;
	positive_eye_height2 = positive_eye_height2 & 0x3f;
	rx_pr("eye height:[%d, %d, %d]\n",
		positive_eye_height0, positive_eye_height1, positive_eye_height2);
	return min_ch_t3x_20(positive_eye_height0, positive_eye_height1, positive_eye_height2);
}

void aml_eq_eye_monitor_t3x_21(void)
{
}

void aml_eq_eye_monitor_t3x(u8 port)
{
	if (port <= E_PORT1)
		aml_eq_eye_monitor_t3x_20(port);
	else
		aml_eq_eye_monitor_t3x_21();
}

void get_eq_val_t3x_20(u8 port)
{
	u32 data32 = 0;
	u32 eq_boost0, eq_boost1, eq_boost2;

	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_EHM_DBG_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_STATUS_MUX_SEL, 0x3, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_STAT, port);
	eq_boost0 = data32 & 0x1f;
	eq_boost1 = (data32 >> 8)  & 0x1f;
	eq_boost2 = (data32 >> 16)      & 0x1f;
	rx_pr("eq:%d-%d-%d\n", eq_boost0, eq_boost1, eq_boost2);
}

/* check eq_boost1 & tap0 status */
bool is_eq1_tap0_err_t3x_20(u8 port)
{
	u32 data32 = 0;
	u32 eq0, eq1, eq2;
	u32 tap0, tap1, tap2;
	u32 eq_avr, tap0_avr;
	bool ret = false;

	if (rx[port].phy.phy_bw < PHY_BW_5)
		return ret;
	/* get eq_boost1 val */
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_EHM_DBG_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_STATUS_MUX_SEL, 0x3, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_STAT, port);
	eq0 = data32 & 0x1f;
	eq1 = (data32 >> 8)  & 0x1f;
	eq2 = (data32 >> 16)      & 0x1f;
	eq_avr =  (eq0 + eq1 + eq2) / 3;
	if (log_level & EQ_LOG)
		rx_pr("eq0=0x%x, eq1=0x%x, eq2=0x%x avr=0x%x\n",
			eq0, eq1, eq2, eq_avr);

	/* get tap0 val */
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_EHM_DBG_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_STATUS_MUX_SEL, 0x3, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_DFE_OFST_DBG_SEL, 0x0, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_STAT, port);
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

void aml_agc_flow_t3x_20(u8 port)
{
	int i;
	u32 data32 = 0;
	u32 tap0, tap1, tap2;
	int flags = 0x7;

	for (i = 7; i > 0; i--) {
		if (flags & 0x1)
			hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_AFE, MSK(3, 0),
									decimaltogray_t3x(i),
									port);
		if (flags & 0x2)
			hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_AFE, MSK(3, 4),
									decimaltogray_t3x(i),
									port);
		if (flags & 0x4)
			hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_AFE, MSK(3, 8),
									decimaltogray_t3x(i),
									port);
		usleep_range(50, 60);
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR,
			T3X_20_EHM_DBG_SEL, 0x0, port);
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ,
			T3X_20_STATUS_MUX_SEL, 0x3, port);
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR,
			T3X_20_DFE_OFST_DBG_SEL, 0x0, port);
		data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_STAT, port);
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

u32 eq_eye_height_t3x_20(u32 wst_ch, u8 port)
{
	u32 data32;
	u32 positive_eye_height;

	/* hold dfe tap1~tap8 */
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ,
					  T3X_20_DFE_HOLD_EN, 1, port);
	usleep_range(10, 20);
	/* disable hw scan mode */
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ,
					  T3X_20_EHM_HW_SCAN_EN, 0, port);
	usleep_range(10, 20);
	/* enable hw scan mode */
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ,
			  T3X_20_EHM_HW_SCAN_EN, 1, port);

	/* wait for scan done */
	usleep_range(rx_info.aml_phy.eye_delay, rx_info.aml_phy.eye_delay + 100);
	/* positive eye height	*/
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR,
			  T3X_20_EHM_DBG_SEL, 1, port);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_STAT, port);
	positive_eye_height = data32 >> (8 * wst_ch) & 0xff;
	/* exit eye monitor scan mode */
	/* disable hw scan mode */
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ,
			  T3X_20_EHM_HW_SCAN_EN, 0, port);
	/* disable eye monitor */
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR,
			 T3X_20_EHM_DBG_SEL, 0, port);
	//hdmirx_wr_bits_amlphy_t3x(T7_HHI_RX_PHY_DCHA_CNTL2,
			  //T7_EYE_MONITOR_EN1, 0);
	usleep_range(10, 20);
	/* release dfe */
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ,
					  T3X_20_DFE_HOLD_EN, 0, port);
	positive_eye_height = positive_eye_height & 0x3f;
	return positive_eye_height;
}

void dump_cdr_info_t3x_20(u8 port)
{
	u32 cdr0_lock, cdr1_lock, cdr2_lock;
	u32 cdr0_int, cdr1_int, cdr2_int;
	u32 cdr0_code, cdr1_code, cdr2_code;
	u32 data32;

	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_EHM_DBG_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_STATUS_MUX_SEL, 0x22, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_MUX_CDR_DBG_SEL, 0x0, port);
	usleep_range(10, 20);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_STAT, port);
	cdr0_code = data32 & 0x7f;
	cdr0_lock = (data32 >> 7) & 0x1;
	cdr1_code = (data32 >> 8) & 0x7f;
	cdr1_lock = (data32 >> 15) & 0x1;
	cdr2_code = (data32 >> 16) & 0x7f;
	cdr2_lock = (data32 >> 23) & 0x1;
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_MUX_CDR_DBG_SEL, 0x1, port);
	usleep_range(10, 20);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_STAT, port);
	cdr0_int = data32 & 0x7f;
	cdr1_int = (data32 >> 8) & 0x7f;
	cdr2_int = (data32 >> 16) & 0x7f;
	rx_pr("cdr_code=[%d,%d,%d]\n", cdr0_code, cdr1_code, cdr2_code);
	rx_pr("cdr_lock=[%d,%d,%d]\n", cdr0_lock, cdr1_lock, cdr2_lock);
	comb_val_t3x(get_val_t3x, "cdr_int", cdr0_int, cdr1_int, cdr2_int, 7);
}

void cdr_retry_t3x_20(u8 port)
{
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, _BIT(6), 0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_EQ_RSTB, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_DFE_RSTB, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_CDR_RSTB, 0x0, port);
	usleep_range(10, 20);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_CDR_RSTB, 0x1, port);
	usleep_range(100, 200);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_EQ_RSTB, 0x1, port);
	usleep_range(100, 200);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_DFE_RSTB, 0x1, port);
	usleep_range(500, 600);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, _BIT(6), 1, port);
	usleep_range(100, 200);
	if (log_level & PHY_LOG)
		dump_cdr_info_t3x_20(port);
}

void dfe_tap0_pol_polling_t3x_20(u32 pos_min_eh, u32 pos_avg_eh, u32 wst_ch, u8 port)
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
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, MSK(2, 20), 0x0, port);
	usleep_range(100, 200);
	for (i = 0; i < 20; i++)
		int_eye_height[i] = eq_eye_height_t3x_20(wst_ch, port);
	quick_sort2_t3x_20(int_eye_height, 0, 19);
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
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, MSK(2, 20), 0x3, port);
	usleep_range(100, 200);
	for (k = 0; k < 20; k++)
		neg_eye_height[k] = eq_eye_height_t3x_20(wst_ch, port);
	quick_sort2_t3x_20(neg_eye_height, 0, 19);
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
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, MSK(2, 20), 0x2, port);
	usleep_range(100, 200);
	rx_pr("select pos eq\n");
}

void swap_num_t3x_20(int *a, int *b)
{
	int temp = *a;
	*a = *b;
	*b = temp;
}

int *num_divide_three_t3x_20(int arr[], int l, int r)
{
	int less = l - 1;
	int more = r;
	int *p = kmalloc(sizeof(int) * 2, GFP_KERNEL);

	while (l < more) {
		if (arr[l] < arr[r])
			swap_num_t3x_20(&arr[++less], &arr[l++]);
		else if (arr[l] > arr[r])
			swap_num_t3x_20(&arr[l], &arr[--more]);
		else
			l++;
	}
	swap_num_t3x_20(&arr[more], &arr[r]);
	p[0] = less + 1;
	p[1] = more;
	return p;
}

void quick_sort2_t3x_20(int arr[], int l, int r)
{
	int len = r + 1;
	int i, j;

	for (i = 0; i < len - 1; i++) {
		for (j = 0; j < len - 1 - i; j++) {
			if (arr[j] > arr[j + 1])
				swap_num_t3x_20(&arr[j], &arr[j + 1]);
		}
	}
}

void aml_enhance_dfe_old_t3x_20(u8 port)
{
	u32 wst_ch;
	int i, j;
	u32 pos_eye_height_sum = 0;
	u32 pos_min_eye_height = 63;
	u32 pos_eye_height[20];
	u32 pos_avg_eye_height;

	wst_ch = aml_eq_eye_monitor_t3x_20(port);
	for (i = 0; i < 20; i++)
		pos_eye_height[i] = eq_eye_height_t3x_20(wst_ch, port);
	quick_sort2_t3x_20(pos_eye_height, 0, 19);
	pos_min_eye_height = pos_eye_height[1];
	for (j = 1; j < 6; j++)
		pos_eye_height_sum += pos_eye_height[j];
	pos_avg_eye_height = pos_eye_height_sum;
	if (log_level & PHY_LOG) {
		rx_pr("pos_min_eye_height = %d\n", pos_min_eye_height);
		rx_pr("pos_avg_eye_height = %d / 5\n", pos_avg_eye_height);
	}
	if (pos_avg_eye_height < rx_info.aml_phy.eye_height * 5)
		dfe_tap0_pol_polling_t3x_20(pos_min_eye_height, pos_avg_eye_height, wst_ch, port);
}

void aml_enhance_dfe_new_t3x_20(u8 port)
{
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, MSK(2, 20), 0x0, port);
}

void eq_max_offset_t3x_20(int eq_boost0, int eq_boost1, int eq_boost2, u8 port)
{
	int offset_eq0, offset_eq1, offset_eq2;
	int ch = -1;
	int eq_initial = 0;

	offset_eq0 = abs(2 * eq_boost0 - eq_boost1 - eq_boost2);
	offset_eq1 = abs(2 * eq_boost1 - eq_boost0 - eq_boost2);
	offset_eq2 = abs(2 * eq_boost2 - eq_boost0 - eq_boost1);
	ch = max_offset_t3x_20(offset_eq0, offset_eq1, offset_eq2);
	if (ch == 0)
		eq_initial = (eq_boost1 + eq_boost2) / 2;
	if (ch == 1)
		eq_initial = (eq_boost0 + eq_boost2) / 2;
	if (ch == 2)
		eq_initial = (eq_boost0 + eq_boost1) / 2;
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_EQ_RSTB, 0x0, port);
	if (rx_info.aml_phy.eq_level & 0x2) {
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, MSK(5, 0), eq_initial, port);
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, _BIT(5), 0x1, port);
	}
	if (rx_info.aml_phy.eq_level & 0x4) {
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, MSK(5, 0), eq_initial, port);
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, _BIT(5), 0x0, port);
	}
	if (rx_info.aml_phy.eq_level & 0x8) {
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, MSK(5, 0), 15, port);
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, _BIT(5), 0x1, port);
	}
	if (rx_info.aml_phy.eq_level & 0x10) {
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, MSK(5, 0), 15, port);
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, _BIT(5), 0x0, port);
	}
	if (rx_info.aml_phy.eq_level & 0x20) {
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, MSK(5, 0), 15, port);
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, _BIT(13), 0, port);
		usleep_range(10, 20);
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, _BIT(13), 1, port);
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, _BIT(13), 0, port);
		usleep_range(100, 110);
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, _BIT(13), 1, port);
	}
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_EQ_RSTB, 0x1, port);
	usleep_range(100, 110);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, _BIT(5), 0x0, port);
	usleep_range(100, 110);
}

void aml_enhance_eq_t3x_20(u8 port)
{
	int eq_boost0, eq_boost1, eq_boost2;
	int data32;
	int offset_eq0, offset_eq1, offset_eq2;

	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_EHM_DBG_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_STATUS_MUX_SEL, 0x3, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_STAT, port);
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
		eq_max_offset_t3x_20(eq_boost0, eq_boost1, eq_boost2, port);
	/* read eq value after eq retry */
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR,
		T3X_20_EHM_DBG_SEL, 0x0, port);
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ,
			T3X_20_STATUS_MUX_SEL, 0x3, port);
		usleep_range(100, 110);
		data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_STAT, port);
		eq_boost0 = data32 & 0x1f;
		eq_boost1 = (data32 >> 8)  & 0x1f;
		eq_boost2 = (data32 >> 16)	& 0x1f;
		rx_pr("after enhance eq:%d-%d-%d\n", eq_boost0, eq_boost1, eq_boost2);
	} else {
		rx_pr("no enhance eq\n");
	}
}

void aml_eq_cfg_t3x_20(u8 port)
{
	u32 idx = rx[port].phy.phy_bw;
	u32 cdr0_int, cdr1_int, cdr2_int;
	u32 data32;
	int i = 0;
	/* dont need to run eq if no sqo_clk or pll not lock */
	if (!is_clk_stable(port))
		return;
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_CDR_RSTB, 1, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_CDR_EN, 1, port);
	usleep_range(200, 210);
	if (idx >= PHY_BW_3)
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_EN_BYP_EQ, 0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_EQ_EN, 1, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_EQ_RSTB, 1, port);
	usleep_range(200, 210);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_DFE_EN, 1, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_DFE_RSTB, 1, port);
	if (rx_info.aml_phy.cdr_fr_en) {
		udelay(rx_info.aml_phy.cdr_fr_en);
		/*cdr fr en*/
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, _BIT(6), 1, port);
	}
	usleep_range(10000, 10100);
	get_eq_val_t3x_20(port);
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
			/* hdmirx_wr_bits_amlphy_t3x( */
				/* HHI_RX_PHY_DCHD_CNTL2, */
				/* DFE_EN, 0); */
		/* } */
		/*udelay(100);*/
		/* hdmirx_wr_bits_amlphy_t3x(HHI_RX_PHY_DCHD_CNTL0, */
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
		aml_dfe_en_t3x_20(port);
	if (is_eq1_tap0_err_t3x_20(port)) {
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_AFE, T3X_20_LEQ_BUF_GAIN, 0x0, port);
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_AFE, T3X_20_LEQ_POLE, 0x0, port);
		if (log_level & EQ_LOG)
			rx_pr("eq1 & tap0 err, tune eq setting\n");
	}
	/*enable HYPER_GAIN calibration for 6G to fix 2.0 cts HF2-1 issue*/
	if (rx[port].phy.phy_bw >= PHY_BW_2 && rx_info.aml_phy.agc_enable)
		aml_agc_flow_t3x_20(port);
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
	if (rx[port].phy.phy_bw >= PHY_BW_2 && rx_info.aml_phy.enhance_eq)
		aml_enhance_eq_t3x_20(port);
	if (rx[port].phy.phy_bw == PHY_BW_2 || rx[port].phy.phy_bw == PHY_BW_1 ||
		rx_info.aml_phy.hyper_gain_en)
		aml_hyper_gain_tuning_t3x_20(port);
	usleep_range(200, 210);
	/*tmds valid det*/
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_CDR_LKDET_EN, 1, port);
	if (log_level & PHY_LOG)
		dump_cdr_info_t3x_20(port);
	for (i = 0; i < rx_info.aml_phy.cdr_retry_max && rx_info.aml_phy.cdr_retry_en; i++) {
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR,
			T3X_20_EHM_DBG_SEL, 0x0, port);
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ,
			T3X_20_STATUS_MUX_SEL, 0x22, port);
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR,
			T3X_20_MUX_CDR_DBG_SEL, 0x0, port);
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR,
			T3X_20_MUX_CDR_DBG_SEL, 0x1, port);
		usleep_range(10, 20);
		data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_STAT, port);
		cdr0_int = data32 & 0x7f;
		cdr1_int = (data32 >> 8) & 0x7f;
		cdr2_int = (data32 >> 16) & 0x7f;
		if (cdr0_int || cdr1_int || cdr2_int)
			cdr_retry_t3x_20(port);
		else
			break;
	}
	if (log_level & PHY_LOG)
		rx_pr("cdr retry times:%d!!!\n", i);
	if (i == rx_info.aml_phy.cdr_retry_max && rx_info.aml_phy.cdr_fr_en_auto) {
		if ((cdr0_int == 0 && cdr1_int == 0) ||
			(cdr0_int == 0 && cdr2_int == 0) ||
			(cdr1_int == 0 && cdr2_int == 0)) {
			hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, _BIT(6), 0, port);
			if (log_level & PHY_LOG)
				rx_pr("cdr_fr_en force 0!!!\n");
		}
	}
	if (rx[port].phy.phy_bw >= PHY_BW_5 && rx_info.aml_phy.enhance_dfe_en_old)
		aml_enhance_dfe_old_t3x_20(port);
	if (rx[port].phy.phy_bw >= PHY_BW_5 && rx_info.aml_phy.enhance_dfe_en_new)
		aml_enhance_dfe_new_t3x_20(port);
	if (rx[port].phy.phy_bw >= PHY_BW_2 && rx_info.aml_phy.enhance_eq)
		aml_enhance_eq_t3x_20(port);
	if (log_level & PHY_LOG) {
		rx_pr("%s,%s,%s\n", rx_info.aml_phy.enhance_dfe_en_new ? "new dfe" : "old dfe",
		rx_info.aml_phy.enhance_eq ? "eq en" : "no eq",
		rx_info.aml_phy.eq_en ? "eq triger" : "eq no triger");
		rx_pr("2.0 PHY Register:\n");
		rx_pr("dchd_eq-0x14=0x%x\n", hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, port));
		rx_pr("dchd_cdr-0x10=0x%x\n", hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, port));
		rx_pr("dcha_dfe-0xc=0x%x\n", hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_DFE, port));
		rx_pr("dcha_afe-0x8=0x%x\n", hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_AFE, port));
		rx_pr("misc2-0x1c=0x%x\n", hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_MISC2, port));
		rx_pr("misc1-0x18=0x%x\n", hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_MISC1, port));
		rx_pr("phy end\n");
	}
}

void aml_phy_get_trim_val_t3x(void)
{
	u32 data32;

	dts_debug_flag = (phy_term_lel >> 4) & 0x1;
	if (dts_debug_flag == 0) {
		data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_MISC1, E_PORT0); //todo
		rterm_trim_val_t3x = (data32 >> 12) & 0xf;
		rterm_trim_flag_t3x = data32 & 0x1;
	} else {
		rlevel = phy_term_lel & 0xf;
		if (rlevel > 15)
			rlevel = 15;
		rterm_trim_flag_t3x = dts_debug_flag;
	}
	if (rterm_trim_flag_t3x) {
		if (log_level & PHY_LOG)
			rx_pr("rterm trim=0x%x\n", rterm_trim_val_t3x);
	}
}

void aml_phy_cfg_t3x_20(u8 port)
{
	u32 idx = rx[port].phy.phy_bw;
	u32 data32;

	if (rx_info.aml_phy.pre_int) {
		if (log_level & PHY_LOG)
			rx_pr("\nphy reg bw: %d\n port = %d\n", idx, port);
		if (rx_info.aml_phy.ofst_en)
			aml_phy_offset_cal_t3x_20(port);
		data32 = phy_dcha_t3x_20[idx][0];
		if (rx_info.aml_phy.phy_debug_en && rx_info.aml_phy.afe_value)
			data32 = rx_info.aml_phy.afe_value;
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_AFE, data32, port);
		usleep_range(5, 10);
		data32 = phy_dcha_t3x_20[idx][1];
		if (rx_info.aml_phy.phy_debug_en && rx_info.aml_phy.dfe_value)
			data32 = rx_info.aml_phy.dfe_value;
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_DFE, data32, port);
		usleep_range(5, 10);
		data32 = phy_dchd_t3x_20[idx][0];
		if (rx_info.aml_phy.phy_debug_en && rx_info.aml_phy.cdr_value)
			data32 = rx_info.aml_phy.cdr_value;
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, data32, port);
		usleep_range(5, 10);
		data32 = phy_dchd_t3x_20[idx][1];
		if (rx_info.aml_phy.phy_debug_en && rx_info.aml_phy.eq_value)
			data32 = rx_info.aml_phy.eq_value;
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, data32, port);
		usleep_range(5, 10);
		data32 = phy_misc_t3x_20[idx][0];
		if (rx_info.aml_phy.phy_debug_en && rx_info.aml_phy.misc1_value)
			data32 = rx_info.aml_phy.misc1_value;
		if (rterm_trim_flag_t3x) {
			if (dts_debug_flag)
				rterm_trim_val_t3x = t3x_rlevel[rlevel];
			data32 = ((data32 & (~((0xf << 12) | 0x1))) |
				(rterm_trim_val_t3x << 12) | rterm_trim_flag_t3x);
		}
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_MISC1, data32, port);
		usleep_range(5, 10);
		data32 = phy_misc_t3x_20[idx][1];
		if (rx_info.aml_phy.phy_debug_en && rx_info.aml_phy.misc2_value)
			data32 = rx_info.aml_phy.misc2_value;
		/* port switch */
		data32 &= (~(0xf << 28));
		data32 |= (0xf << 28);
		data32 &= (~(0xf << 24));
		data32 |= ((1 << port) << 24);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_MISC2, data32, port);
		usleep_range(5, 10);
		if (!rx_info.aml_phy.pre_int_en)
			rx_info.aml_phy.pre_int = 0;
	}
	if (rx_info.aml_phy.sqrst_en) {
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_MISC1, T3X_20_SQ_RSTN, 0, port);
		usleep_range(5, 10);
		/*sq_rst*/
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_MISC1, T3X_20_SQ_RSTN, 1, port);
	}
}

/*
 * HDMIRX 2.1 PHY
 */
static const u32 phy_misc_t3x_21[][3] = {
		/* misc0	misc1		misc2 */
		/* 0x114	0x118		0x11c */
	{	 /* 24~35M */
		0x1ff777f, 0x10f7000f, 0x00001a00,
	},
	{	 /* 37~75M */
		0x1ff777f, 0x10f7000f, 0x00001a00,
	},
	{	 /* 75~150M */
		0x1ff777f, 0x10f7000f, 0x00001a00,
	},
	{	 /* 150~340M */
		0x1ff777f, 0x10f7000f, 0x00001a00,
	},
	{	 /* 340~525M */
		0x1ff777f, 0x10f7000f, 0x00001a00,
	},
	{	 /* 525~600M */
		0x1ff777f, 0x10f7000f, 0x00001a00,
	},
	{	 /* FRL 3G */
		0x01fcffff, 0x30f7000f, 0x00001a00,
	},
	{	 /* FRL 6G3L */
		0x01fcffff, 0x30f7000f, 0x00001a00,
	},
	{	 /* FRL 6G4L */
		0x01fcffff, 0x30f7000f, 0x00001a00,
	},
	{	 /* FRL 8G */
		0x01fcffff, 0x30f7000f, 0x00001a00,
	},
	{	 /* FRL 10G */
		0x01fcffff, 0x30f7000f, 0x00001a00,
	},
	{	 /* FRL 12G */
		0x01fcffff, 0x30f7000f, 0x00001a00,
	},
};

static const u32 phy_dcha_t3x_21[][4] = {
		/* dcha_afe		dcha_dfe	dcha_pi		dcha_ctrl*/
		/* 0x120		0x124		0x128		0x12c*/
	{	 /* 24~45M */
		0x03708888, 0x05ff1a05, 0x51070230, 0x07f06555,
	},
	{	 /* 35~75M */
		0x03708888, 0x05ff1a05, 0x51070130, 0x07f06555,
	},
	{	 /* 75~150M */
		0x0370ffff, 0x05ff1a05, 0x51070030, 0x07f06555,
	},
	{	 /* 150~340M */
		0x0370ffff, 0x05ff1a05, 0x51030020, 0x07f06555,
	},
	{	 /* 340~525M */
		0x2320ffff, 0x05ff1a05, 0x51000010, 0x07f06555,
	},
	{	 /* 525~600M */
		0x2320ffff, 0x05ff1a05, 0x51000010, 0x07f06555,
	},
	{	 /* FRL 3G */
		0x0370ffff, 0x05ff1a05, 0x51030010, 0x07f06555,
	},
	{	 /* FRL 6G3L */
		0x2320ffff, 0x05ff1a05, 0x51001010, 0x07f06555,
	},
	{	 /* FRL 6G4L */
		0x2320ffff, 0x05ff1a05, 0x51001010, 0x07f06555,
	},
	{	 /* FRL 8G */
		0x2340ffff, 0x05ff1a05, 0x51102000, 0x07f06555,
	},
	{	 /* FRL 10G */
		0x2320ffff, 0x05ff1a05, 0x51102000, 0x07f06555,
	},
	{	 /* FRL 12G */
		0x7330bbbb, 0x05ff1a05, 0x51102000, 0x07f06555,
	},
};

static const u32 phy_dchd_t3x_21[][2] = {
		/* dchd_cdr	 dchd_eq */
		/*  0x130	 0x134 */
	{	 /* 24~35M */
		0x04407ec2, 0x301b3060,
	},
	{	 /* 35~75M */
		0x04407dc2, 0x301b3060,
	},
	{	 /* 75~150M */
		0x04407cc2, 0x30133069,
	},
	{	 /* 140~340M */
		0x044078c2, 0x30133049,
	},
	{	 /* 340~525M */
		0x044074c2, 0x3013304f,
	},
	{	 /* 525~600M */
		0x044074c2, 0x3013304f,
	},
	{	 /* FRL 3G */
		0x044074c2, 0x3013304f,
	},
	{	 /* FRL 6G3L */
		0x044074c2, 0x3013304f,
	},
	{	 /* FRL 6G4L */
		0x044074c2, 0x3013304f,
	},
	{	 /* FRL 8G */
		0x044070c2, 0x3013304f,
	},
	{	 /* FRL 10G */
		0x044070c2, 0x3013304f,
	},
	{	 /* FRL12G */
		0x044070c2, 0x3013304f,
	},
};

int pll_level_en;
int pll_level;
int dts_debug_flag_t3x_21;
int rlevel_t3x_21;
int rterm_trim_val_t3x_21;
int rterm_trim_flag_t3x_21;
int phy_term_lel_t3x_21;

void aml_phy_get_trim_val_t3x_21(u8 port)
{
	u32 data32;

	dts_debug_flag_t3x_21 = (phy_term_lel_t3x_21 >> 4) & 0x1;
	if (dts_debug_flag_t3x_21 == 0) {
		data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PHY_MISC2, port); //todo
		rterm_trim_val_t3x_21 = data32 & 0xf;
		rterm_trim_flag_t3x_21 = (data32 >> 31) & 0x1;
	} else {
		rlevel_t3x_21 = phy_term_lel_t3x_21 & 0xf;
		if (rlevel_t3x_21 > 15)
			rlevel_t3x_21 = 15;
		rterm_trim_flag_t3x_21 = dts_debug_flag_t3x_21;
	}
	if (rterm_trim_flag_t3x_21) {
		if (log_level & PHY_LOG)
			rx_pr("rterm trim=0x%x\n", rterm_trim_val_t3x_21);
	}
}

void aml_phy_offset_cal_t3x_21(int port)
{
	u32 data32;
	/* rterm not enabled */
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_MISC0, 0x01bcfff0, port);
	/* squelch not enabled*/
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_MISC1, 0x3077000f, port);
	data32 = 0x00005a00;
	if (rterm_trim_flag_t3x_21) {
		data32 = ((data32 & (~(0xf | (0x1 << 31)))) |
		rterm_trim_val_t3x_21 | (rterm_trim_flag_t3x_21 << 31));
	}
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_MISC2,  data32, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_AFE,  0x2300ffff, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_DFE,  0x05ff1a05, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_PI,  0x51102000, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_CTRL,  0x07f06555, port);
	/* cdr lkdet reset = 0*/
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR,  0x044010c2, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ,  0x3011104f, port);
	usleep_range(10, 20);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_MISC0, DCH_RSTN, 0xf, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_MISC1, SQ_RSTN, 0x1, port);

	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x0500fa00, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL1, 0x014810e6, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL2, 0x0, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL3, 0x0, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL4, 0x0, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x0500fa01, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x0500fa03, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL1, 0x014010e6, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x0500fa07, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x4500fa07, port);
	usleep_range(20, 30);

	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_DFE, DFE_TAP_EN, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_DFE, DFE_H1_PD, 0x1, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, CDR_FR_EN, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, CDR_RSTB, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, CDR_LKDET_EN, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, EQ_RSTB, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, DFE_RSTB, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_DFE, DFE_OFST_CAL_START, 0x1, port);
	usleep_range(10, 20);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, OFST_CAL_START, 0x1, port);
	usleep_range(100, 110);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, CDR_RSTB, 0x1, port);
	if (log_level & PHY_LOG)
		rx_pr("h21 ofst cal\n");
}

void rx_21_frl_phy_cfg(u8 port)
{
	u32 data32 = 0;
	u32 idx = rx[port].phy.phy_bw;

	if (rx_info.aml_phy_21.pre_int) {
		if (rx_info.aml_phy_21.ofst_en) {
			aml_phy_offset_cal_t3x_21(port);
			usleep_range(1000, 1100);
		}
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, OFST_CAL_START, 0x0, port);
		usleep_range(100, 110);
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_DFE, DFE_OFST_CAL_START, 0x0, port);
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_DFE, DFE_TAP_EN, 0x1ff, port);
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_DFE, DFE_H1_PD, 0x0, port);

		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_MISC0, phy_misc_t3x_21[idx][0], port);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_MISC1, phy_misc_t3x_21[idx][1], port);
		aml_phy_get_trim_val_t3x_21(port);
		data32 = phy_misc_t3x_21[idx][2];
		if (rterm_trim_flag_t3x_21) {
			if (dts_debug_flag_t3x_21)
				rterm_trim_val_t3x_21 = t3x_rlevel[rlevel_t3x_21];
			data32 = ((data32 & (~(0xf | (0x1 << 31)))) |
			rterm_trim_val_t3x_21 | (rterm_trim_flag_t3x_21 << 31));
		}
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_MISC2, data32, port);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_AFE,  phy_dcha_t3x_21[idx][0], port);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_DFE,  phy_dcha_t3x_21[idx][1], port);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_PI,  phy_dcha_t3x_21[idx][2], port);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_CTRL,  phy_dcha_t3x_21[idx][3], port);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR,  phy_dchd_t3x_21[idx][0], port);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ,  phy_dchd_t3x_21[idx][1], port);
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_MISC0, _BIT(22), 0x1, port);
		if (!rx_info.aml_phy_21.pre_int_en)
			rx_info.aml_phy_21.pre_int = 0;
	}
	//rterm_en(dcha_misc0[22]) = 0x1
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, CDR_FR_EN, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, CDR_RSTB, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, CDR_LKDET_EN, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, EQ_MODE, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, EQ_RSTB, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, DFE_RSTB, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, DFE_HOLD, 0x0, port);
	if (log_level & PHY_LOG)
		rx_pr("rx_21_phy_cfg\n");
}

void t3x_480p_pll_cfg_21(u8 port)
{
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x0530a000, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL1, 0x01481236, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x0530a001, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x0530a003, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL1, 0x41401236, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x0530a007, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x4530a007, port);
	rx[port].phy.aud_div = 0;
	rx[port].phy.aud_div_1 = 0x408;
}

void t3x_720p_pll_cfg_21(u8 port)
{
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x0530a010, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL1, 0x01481236, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x0530a011, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x0530a013, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL1, 0x21401236, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x0530a017, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x4530a017, port);
	rx[port].phy.aud_div = 0;
	rx[port].phy.aud_div_1 = 0x408;
}

void t3x_1080p_pll_cfg_21(u8 port)
{
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x05302800, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL1, 0x01481236, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x05302801, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x05302803, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL1, 0x41401236, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x05302807, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x45302807, port);
	rx[port].phy.aud_div = 0;
	rx[port].phy.aud_div_1 = 0x8;
}

void t3x_4k30_pll_cfg_21(u8 port)
{
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x05302810, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL1, 0x01481236, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x05302811, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x05302813, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL1, 0x21401236, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x05302817, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x45302817, port);
	rx[port].phy.aud_div = 0;
	rx[port].phy.aud_div_1 = 0x8;
}

void t3x_4k60_pll_cfg_21(u8 port)
{
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x05302800, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL1, 0x01481236, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x05302801, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x05302803, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL1, 0x01401236, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x05302807, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x45302807, port);
	usleep_range(10, 20);
	rx[port].phy.aud_div = 0;
	rx[port].phy.aud_div_1 = 0x8;
}

void aml_pll_bw_cfg_t3x_21(int f_rate, u8 port)
{
	u32 data32;
	int pll_rst_cnt = 0;
	u32 idx = rx[port].phy.pll_bw;
	u32 clk_rate = 0;

	if (f_rate)
		idx = f_rate + 5;
	if (rx_info.aml_phy_21.phy_bwth) {
		switch (f_rate) {
		case FRL_6G_4LANE:
		case FRL_8G_4LANE:
		case FRL_10G_4LANE:
		case FRL_12G_4LANE:
			hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_MISC0, _BIT(7), 0x1, port);
			hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_MISC0, _BIT(11), 0x1, port);
			hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_MISC0, _BIT(15), 0x1, port);
			hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_MISC0, CCH_EN, 0x0, port);
			hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_MISC1, SQ_GATED, 0x1, port);
			break;
		case FRL_3G_3LANE:
		case FRL_6G_3LANE:
			hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_MISC0, _BIT(7), 0x0, port);
			hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_MISC0, _BIT(11), 0x0, port);
			hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_MISC0, _BIT(15), 0x0, port);
			hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_MISC0, CCH_EN, 0x0, port);
			hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_MISC1, SQ_GATED, 0x1, port);
			break;
		case FRL_OFF:
		default:
			hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_MISC0, _BIT(7), 0x0, port);
			hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_MISC0, _BIT(11), 0x0, port);
			hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_MISC0, _BIT(15), 0x0, port);
			hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_MISC0, CCH_EN, 0x3, port);
			hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_MISC1, SQ_GATED, 0x0, port);
			break;
		}
		data32 = phy_dcha_t3x_21[idx][0];
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_AFE, data32, port);
		data32 = phy_dcha_t3x_21[idx][2];
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_PI, data32, port);
		data32 = phy_dchd_t3x_21[idx][0];
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, CDR_OS_RATE,
			(data32 >> 8) & 0x3, port);
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, CDR_PI_DIV,
			(data32 >> 10) & 0x3, port);
		data32 = phy_dchd_t3x_21[idx][1];
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, EQ_BYP_VAL1,
			data32 & 0x1f, port);
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, EQ_BYP_EN,
			(data32 >> 5) & 0x1, port);
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, DFE_TAPS_DISABLE,
			(data32 >> 19) & 0x1, port);
		if (log_level & FRL_LOG)
			rx_pr("phy bwth\n");
	}
	//config pll
	clk_rate = rx_get_scdc_clkrate_sts(port);
	if (log_level & PHY_LOG)
		rx_pr("clk rate = %d\n", clk_rate);
	//rx_clkmsr_handler();
	idx = aml_phy_pll_band(rx[port].clk.cable_clk, clk_rate);
	if (log_level & PHY_LOG)
		rx_pr("idx = %d\n", idx);
	if (f_rate == FRL_OFF) {
		do {
			if (idx == PLL_BW_0)
				t3x_480p_pll_cfg_21(port);
			if (idx == PLL_BW_1)
				t3x_720p_pll_cfg_21(port);
			if (idx == PLL_BW_2)
				t3x_1080p_pll_cfg_21(port);
			if (idx == PLL_BW_3)
				t3x_4k30_pll_cfg_21(port);
			if (idx == PLL_BW_4)
				t3x_4k60_pll_cfg_21(port);
			if (log_level & PHY_LOG)
				rx_pr("PLL0=0x%x\n", hdmirx_rd_amlphy_t3x(T3X_RG_RX20PLL_0, port));
			if (pll_rst_cnt++ > pll_rst_max) {
				if (log_level & PHY_LOG)
					rx_pr("pll rst error\n");
				break;
			}
			if (log_level & PHY_LOG) {
				rx_pr("sq=%d,pll_lock=%d",
					hdmirx_rd_top(TOP_MISC_STAT0_T3X, port) & 0x1,
					is_pll_lock_t3x(port));
			}
		} while (!is_tmds_clk_stable(port) && is_clk_stable(port) &&
			!aml_phy_pll_lock(port));
	}
	//for debug
	if (rx_info.aml_phy_21.vga_gain <= 0xffff)
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_AFE, VGA_GAIN,
			rx_info.aml_phy_21.vga_gain, port);
	if (rx_info.aml_phy_21.eq_stg1 < 0x1f) {
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, EQ_BYP_VAL1,
			rx_info.aml_phy_21.eq_stg1, port);
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, EQ_BYP_EN,
			0x1, port);
	}
	if (rx_info.aml_phy_21.eq_stg2 <= 0x7)
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_AFE, BUF_BST,
			rx_info.aml_phy_21.eq_stg2, port);
	if (rx_info.aml_phy_21.eq_pole <= 0x7)
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_AFE, LEQ_POLE,
			rx_info.aml_phy_21.eq_pole, port);
	if (rx_info.aml_phy_21.buf_gain <= 0x7)
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_AFE, BUF_GAIN,
			rx_info.aml_phy_21.buf_gain, port);

}

void rx_21_frl_pll_cfg(int f_rate, u8 port)
{
	if (log_level & FRL_LOG)
		rx_pr("port-%d f_rate=%d\n", port, f_rate);
	if (f_rate == FRL_3G_3LANE) {
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x05007d00, port);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL1, 0x014810e6, port);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL2, 0x0, port);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL3, 0x0, port);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL4, 0x0, port);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL2, 0x0, port);
		usleep_range(10, 20);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL2, 0x0, port);
		usleep_range(10, 20);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x05007d01, port);
		usleep_range(10, 20);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x05007d03, port);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL1, 0x014010e6, port);
		usleep_range(10, 20);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x05007d07, port);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x45007d07, port);
	} else if (f_rate == FRL_6G_3LANE || f_rate == FRL_6G_4LANE) {
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x0500fa00, port);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL1, 0x014810e6, port);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL2, 0x0, port);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL3, 0x0, port);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL4, 0x0, port);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL2, 0x0, port);
		usleep_range(10, 20);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL2, 0x0, port);
		usleep_range(10, 20);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x0500fa01, port);
		usleep_range(10, 20);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x0500fa03, port);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL1, 0x014010e6, port);
		usleep_range(10, 20);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x0500fa07, port);
		usleep_range(10, 20);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x4500fa07, port);
	} else if (f_rate == FRL_8G_4LANE) {
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x05202000, port);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL1, 0x014817e6, port);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL2, 0x00187d06, port);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL3, 0xf0002dd3, port);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL4, 0x55813041, port);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL2, 0x00087d07, port);
		usleep_range(10, 20);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL2, 0x00187d01, port);
		usleep_range(10, 20);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x05202001, port);
		usleep_range(10, 20);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x05202003, port);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL1, 0x014017e6, port);
		usleep_range(10, 20);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x05202007, port);
		usleep_range(10, 20);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x45202007, port);
	} else if (f_rate == FRL_10G_4LANE) {
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x05202800, port);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL1, 0x014817e6, port);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL2, 0x00187d06, port);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL3, 0xf0002dd3, port);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL4, 0x55813041, port);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL2, 0x00187d07, port);
		usleep_range(10, 20);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL2, 0x00187d01, port);
		usleep_range(10, 20);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x05202801, port);
		usleep_range(10, 20);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x05202803, port);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL1, 0x014017e6, port);
		usleep_range(10, 20);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x05202807, port);
		usleep_range(10, 20);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x45202807, port);
	} else if (f_rate == FRL_12G_4LANE) {
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x0500fa00, port);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL1, 0x014810e6, port);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL2, 0x0, port);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL3, 0x0, port);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL4, 0x0, port);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL2, 0x0, port);
		usleep_range(10, 20);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL2, 0x0, port);
		usleep_range(10, 20);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x0500fa01, port);
		usleep_range(10, 20);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x0500fa03, port);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL1, 0x014010e6, port);
		usleep_range(10, 20);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x0500fa07, port);
		usleep_range(10, 20);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x4500fa07, port);
	}
	usleep_range(100, 110);
}

void rx_21_eq_get_val(u32 *ch0, u32 *ch1, u32 *ch2, u32 *ch3, u8 port)
{
	u32 data32;

	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_EYE_EN, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, MUX_BLOCK_SEL, 0x3, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PHY_DCH_STAT, port);
	*ch0 = data32 & 0x1f;
	*ch1 = (data32 >> 8) & 0x1f;
	*ch2 = (data32 >> 16) & 0x1f;
	*ch3 = (data32 >> 24) & 0x1f;
}

void rx_21_eq_retry(u8 port)
{
	u32 eq_boost0 = 0;
	u32 eq_boost1 = 0;
	u32 eq_boost2 = 0;
	u32 eq_boost3 = 0;
	int idx = rx[port].phy.pll_bw;

	if (rx[port].var.frl_rate)
		idx = rx[port].var.frl_rate + 5;
	if (idx >= 3) {
		if (rx_info.aml_phy_21.eq_hold)
			hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, EQ_MODE,
			rx_info.aml_phy_21.eq_hold, port);
		rx_21_eq_get_val(&eq_boost0, &eq_boost1, &eq_boost2, &eq_boost3, port);
		rx_pr("eq:0x%x-0x%x-0x%x-0x%x\n", eq_boost0, eq_boost1,
			eq_boost2, eq_boost3);
		if  (rx_info.aml_phy_21.eq_retry) {
			if (eq_boost0 == 0 || eq_boost0 == 31 ||
				eq_boost1 == 0 || eq_boost1 == 31 ||
				eq_boost2 == 0 || eq_boost2 == 31 ||
				eq_boost3 == 0 || eq_boost3 == 31) {
				hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ,
					EQ_MODE, 0x0, port);
				hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ,
					EQ_RSTB, 0x0, port);
				usleep_range(10, 20);
				hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ,
					EQ_RSTB, 0x1, port);
				mdelay(10);
				if (rx_info.aml_phy_21.eq_hold)
					hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, EQ_MODE,
						rx_info.aml_phy_21.eq_hold, port);
				rx_21_eq_get_val(&eq_boost0, &eq_boost1,
					&eq_boost2, &eq_boost3, port);
				rx_pr("eq_retry:0x%x-0x%x-0x%x-0x%x\n", eq_boost0, eq_boost1,
					eq_boost2, eq_boost3);
			}
		}
	}
}

void rx_21_dfe_en(u8 port)
{
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, DFE_RSTB, 0x0, port);
	usleep_range(10, 20);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, DFE_RSTB, 0x1, port);
	usleep_range(100, 110);
	if (rx_info.aml_phy_21.dfe_hold) {
		usleep_range(1000, 1100);
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, DFE_HOLD, 0x1, port);
	}
	if (log_level & PHY_LOG)
		rx_pr("dfe en\n");
}

void rx_21_eq_cfg(int f_rate, u8 port)
{
	u32 cdr0_int, cdr1_int, cdr2_int, cdr3_int, data32;

	//if pll unlock
		//return;
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, CDR_FR_EN, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, CDR_RSTB, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, CDR_LKDET_EN, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, EQ_MODE, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, EQ_RSTB, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, DFE_RSTB, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, DFE_HOLD, 0x0, port);
	//hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, DFE_RSTB, 0x1, port);
	//hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, EQ_MODE, 0x1, port);
	usleep_range(10, 15);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, CDR_RSTB, 0x1, port);
	//hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, CDR_LKDET_EN, 0x1, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, EQ_RSTB, 0x1, port);
	if (rx_info.aml_phy_21.cdr_fr_en) {
		usleep_range(rx_info.aml_phy_21.cdr_fr_en, rx_info.aml_phy_21.cdr_fr_en + 10);
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, CDR_FR_EN, 0x1, port);
	}
	usleep_range(10000, 10100);
	rx_21_eq_retry(port);
	if (rx_info.aml_phy_21.dfe_en)
		rx_21_dfe_en(port);
	if (rx_info.aml_phy_21.vga_tune && rx[port].var.frl_rate == FRL_12G_4LANE)
		hdmirx_vga_gain_tuning(port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, CDR_LKDET_EN, 0x1, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_EYE_EN, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, MUX_BLOCK_SEL, 0x2, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_CDR_DBG_SEL, 0x1, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PHY_DCH_STAT, port);
	cdr0_int = data32 & 0x7f;
	cdr1_int = (data32 >> 8) & 0x7f;
	cdr2_int = (data32 >> 16) & 0x7f;
	cdr3_int = (data32 >> 24) & 0x7f;
	if (log_level & FRL_LOG)
		rx_pr("cdr int=0x%x-0x%x-0x%x-0x%x\n", cdr0_int, cdr1_int, cdr2_int, cdr3_int);
}

void rx_21_dump_fpll_0(void)
{
	rx_pr("PLL0_CTRL0=0x%x\n", rd_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL0));
	rx_pr("PLL0_CTRL1=0x%x\n", rd_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL1));
	rx_pr("PLL0_CTRL2=0x%x\n", rd_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL2));
	rx_pr("PLL0_CTRL3=0x%x\n", rd_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL3));
}

void rx_21_dump_fpll_1(void)
{
	rx_pr("PLL1_CTRL0=0x%x\n", rd_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL0));
	rx_pr("PLL1_CTRL1=0x%x\n", rd_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL1));
	rx_pr("PLL1_CTRL2=0x%x\n", rd_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL2));
	rx_pr("PLL1_CTRL3=0x%x\n", rd_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL3));
}

bool is_fpll0_locked(void)
{
	return hdmirx_rd_bits_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL0, _BIT(31));
}

bool is_fpll1_locked(void)
{
	return hdmirx_rd_bits_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL0, _BIT(31));
}

unsigned long rx_get_flclk(u8 port)
{
	if (port == E_PORT2)
		return meson_clk_measure_with_precision(9, 32);
	else if (port == E_PORT3)
		return meson_clk_measure_with_precision(11, 32);
	return 0;
}

const u32 fpll_t3x[] = {
	0x21200035,
	0x30200035,
	0x83afa82a,
	0x00040000,
	0x0b0da001, //0x0b0da001
	0x10200035,
	0x0b0da201, //0x0b0da201
};

void rx_dump_reg_d_f_sts(u8 port)
{
	u32 data32;

	data32 = hdmirx_rd_top(TOP_FPLL21_STAT1, port);
	rx_pr("req_f=%d\n", (data32 >> 12) & 0x7fffff);
	rx_pr("req_d=%d\n", data32 & 0x1ff);
}

bool rx_get_overlap_sts(u8 port)
{
	hdmirx_wr_bits_cor(H21RXSB_INTR2_M42H_IVCRX, _BIT(4), 1, port);
	return hdmirx_rd_bits_cor(H21RXSB_INTR2_M42H_IVCRX, _BIT(4), port);
}

bool rx_get_clkready_sts(u8 port)
{
	return hdmirx_rd_bits_cor(H21RXSB_STATUS_M42H_IVCRX, _BIT(1), port);
}

bool rx_get_valid_m_sts(u8 port)
{
	return hdmirx_rd_bits_cor(H21RXSB_STATUS_M42H_IVCRX, _BIT(0), port);
}

bool is_fpll_err(u8 port)
{
	bool ret = true;

	rx_21_fpll_cfg(rx[port].var.frl_rate, port);
	mdelay(1);
	if (rx_get_clkready_sts(port)) {
		if (fpll_chk_lvl & 0x1)
			ret = rx_get_overlap_sts(port);
		else
			ret = false;
	}
	return ret;
}

void rx_21_fpll_calculation(int f_rate, u8 port)
{
	u8 reg_valid_m;
	unsigned long o_req_m, flclk, tclk, temp;
	//u32 min, max;
	u8 pre_div;
	int cnt = 0;
	unsigned long data = 0;

	if (log_level & FRL_LOG)
		rx_pr("fpll cal,port=%d\n", port);

	//0505 dbg config give_n
	/* config give_n to 0x2000(8192) */
	hdmirx_wr_cor(H21RXSB_GN2_M42H_IVCRX, 0x0, port);
	hdmirx_wr_cor(H21RXSB_GN1_M42H_IVCRX, 0x20, port);
	hdmirx_wr_cor(H21RXSB_GN0_M42H_IVCRX, 0x0, port);

	//0505 dbg config post_div
	hdmirx_wr_cor(H21RXSB_POSTDIV_M42H_IVCRX, 0x2, port);

	//0505 dbg config reg_n
	/* fpll ctrl0[20:16] */
	switch (f_rate) {
	case FRL_3G_3LANE:
		odn_reg_n_mul = 4;
		break;
	case FRL_6G_3LANE:
	case FRL_6G_4LANE:
		odn_reg_n_mul = 6;
		break;
	case FRL_8G_4LANE:
		odn_reg_n_mul = 8;
		break;
	case FRL_10G_4LANE:
		odn_reg_n_mul = 8;
		break;
	case FRL_12G_4LANE:
		odn_reg_n_mul = 10;
		break;
	default:
		odn_reg_n_mul = 6;
		break;
	}
	hdmirx_wr_cor(H21RXSB_NMUL_M42H_IVCRX, odn_reg_n_mul, port);

	//0505 dbg reset
	hdmirx_wr_bits_cor(RX_PWD_SRST2_PWD_IVCRX, _BIT(7), 1, port);
	udelay(1);
	hdmirx_wr_bits_cor(RX_PWD_SRST2_PWD_IVCRX, _BIT(7), 0, port);
	udelay(100);
	//wait valid_m
	while (cnt < valid_m_wait_max) {
		cnt++;
		reg_valid_m = hdmirx_rd_bits_cor(H21RXSB_STATUS_M42H_IVCRX, _BIT(0), port);

		//rx_pr("valid_m=%d\n", reg_valid_m);
		if (reg_valid_m)
			break;
		udelay(5);
	}
	cnt = 0;
	if (!reg_valid_m) {
		if (log_level & FRL_LOG)
			rx_pr("m not valid!-0x%x\n",
				hdmirx_rd_cor(H21RXSB_STATUS_M42H_IVCRX, port));
		return;
	}
	if (log_level & FRL_LOG)
		rx_pr("port-%d m valid\n", port);
	//or get flclk based on frl_rate!!! todo
	flclk = rx_get_flclk(port);
	if (!flclk) {
		if (log_level & FRL_LOG)
			rx_pr("flclk invalid\n");
		return;
	}
	o_req_m = ((hdmirx_rd_cor(H21RXSB_REQM2_M42H_IVCRX, port) & 0Xf) << 16) |
		(hdmirx_rd_cor(H21RXSB_REQM1_M42H_IVCRX, port) << 8) |
		hdmirx_rd_cor(H21RXSB_REQM0_M42H_IVCRX, port);

	if (reg_valid_m && o_req_m == 0) {
		if (log_level & FRL_LOG)
			rx_pr("port-%d m not really valid\n", port);
		return;
	}
	/* tclk = flclk * o_req_m / give_n / post_div */
	tclk = flclk * o_req_m / 8192 / 2;
	//rx_pr("flclk * o_req_m=%ld\n", flclk * o_req_m);
	//rx_pr("flclk * o_req_m / 8192=%ld\n", flclk * o_req_m / 8192);
	//rx_pr("flclk * o_req_m / 8192 / 2=%ld\n", flclk * o_req_m / 8192 / 2);
	if (log_level & FRL_LOG)
		rx_pr("flclk = %ld, tclk=%ld\n", flclk, tclk);

	/*
	 * 3. get pre_div
	 */
	//min = 800 * MHz / tclk;
	//max = 1600 * MHz / tclk;
	temp = (800 + 1600) * KHz * 10 / 2 / (tclk / KHz);
	if (temp % 10)
		pre_div = (temp / 10) + 1;
	else
		pre_div = (temp / 10);
	if (log_level & FRL_LOG)
		rx_pr("temp=%lu,pre_div=0x%x\n", temp, pre_div);
	/* check vco */
	if (tclk * pre_div * 2 / KHz < 1600 * KHz) {
		pre_div += 1;
		if (log_level & FRL_LOG)
			rx_pr("error,vco<1.6G!\n");
	}
	if (tclk * pre_div * 2 / KHz > 3200 * KHz) {
		pre_div -= 1;
		if (log_level & FRL_LOG)
			rx_pr("error,vco>3.2G!\n");
	}
	/* fpll cntl2[9:4]*/
	if (pre_div >= 64) {
		pre_div = 64;
		hdmirx_wr_cor(H21RXSB_PREDIV_M42H_IVCRX, 0, port);
	} else {
		if (pre_div == 33)
			pre_div = 31;
		hdmirx_wr_cor(H21RXSB_PREDIV_M42H_IVCRX, pre_div, port);
	}

	data = hdmirx_rd_cor(H21RXSB_NMUL_M42H_IVCRX, port);
	data = data * tclk * pre_div * 2;
	data = data / flclk;
	data = data & 0x1ff;
	if (log_level & FRL_LOG)
		rx_pr("data:%lu\n", data);
	wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL0, (fpll_t3x[0] & 0xfffffe00) | data |
		(odn_reg_n_mul << 16) | (pre_div == 64 ? (4 << 10) : 0));
	wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL0, (fpll_t3x[1] & 0xfffffe00) | data |
		(odn_reg_n_mul << 16) | (pre_div == 64 ? (4 << 10) : 0));
	wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL1, (fpll_t3x[2] & 0xfff00000) | 0xb9000);
	if (pre_div < 64) {
		wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL2, fpll_t3x[3] |
			((pre_div & 0x1f) << 4));
	} else {
		wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL2, fpll_t3x[3] |
			(4 << 4));
	}
	wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL3, fpll_t3x[4] | (1 << 7));
	wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL0, (fpll_t3x[5] & 0xfffffe00) | data |
		(odn_reg_n_mul << 16) | (pre_div == 64 ? (4 << 10) : 0));
	wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL3, fpll_t3x[6] | (1 << 7) | (1 << 19));
	udelay(10);
	hdmirx_wr_bits_cor(RX_PWD_SRST2_PWD_IVCRX, _BIT(7), 1, port);
	udelay(1);
	hdmirx_wr_bits_cor(RX_PWD_SRST2_PWD_IVCRX, _BIT(7), 0, port);
	udelay(100);
	//wait valid_m
	while (cnt < valid_m_wait_max) {
		cnt++;
		reg_valid_m = hdmirx_rd_bits_cor(H21RXSB_STATUS_M42H_IVCRX, _BIT(0), port);

		if (reg_valid_m)
			break;
		udelay(5);
	}
	cnt = 0;
	if (!reg_valid_m) {
		if (log_level & FRL_LOG)
			rx_pr("m not valid!-0x%x\n",
				hdmirx_rd_cor(H21RXSB_STATUS_M42H_IVCRX, port));
		return;
	}
	/*
	 * 4. fpll config
	 */
	do {
		wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL0, fpll_t3x[0] |
			(odn_reg_n_mul << 16) | (pre_div == 64 ? (4 << 10) : 0));
		wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL0, fpll_t3x[1] |
			(odn_reg_n_mul << 16) | (pre_div == 64 ? (4 << 10) : 0));
		wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL1, fpll_t3x[2]);
		if (pre_div < 64) {
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL2, fpll_t3x[3] |
				((pre_div & 0x1f) << 4));
		} else {
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL2, fpll_t3x[3] |
				(4 << 4));
		}
		wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL3, fpll_t3x[4]);
		wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL0, fpll_t3x[5] |
			(odn_reg_n_mul << 16) | (pre_div == 64 ? (6 << 10) : 0));
		wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL3, fpll_t3x[6]);
		udelay(10);
		if (cnt++ > 5) {
			rx_pr("fpll cfg err!\n");
			break;
		}
	} while (!is_fpll0_locked());
	if (!is_fpll0_locked() || !rx_get_overlap_sts(port) || !rx_get_clkready_sts(port)) {
		//rx_21_dump_fpll_0();
		if (log_level & FRL_LOG) {
			rx_dump_reg_d_f_sts(port);
			rx_pr("post_div=%d\n", hdmirx_rd_cor(H21RXSB_POSTDIV_M42H_IVCRX, port));
			rx_pr("pre_div=%d\n", hdmirx_rd_cor(H21RXSB_PREDIV_M42H_IVCRX, port));
			rx_pr("reg_n=%d\n", hdmirx_rd_cor(H21RXSB_NMUL_M42H_IVCRX, port));
			rx_pr("o_req_m=%ld\n", o_req_m);
			rx_pr("vco=%ld\n", tclk * pre_div * 2);
			rx_pr("0x1525=0x%x", hdmirx_rd_cor(0x1525, port));
		}
	} else {
		rx_pr("fpll locked\n");
	}
	//0505 dbg delay 10us,print fpll lock status
	//rx_pr("ctrl0=0x%x\n", rd_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL0));
}

void rx_21_fpll_calculation1(int f_rate, u8 port)
{
	u8 reg_valid_m;
	unsigned long o_req_m, flclk, tclk, temp;
	//u32 min, max;
	u8 pre_div;
	int cnt = 0;
	unsigned long data = 0;

	if (log_level & FRL_LOG)
		rx_pr("fpll cal,port=%d\n", port);
	//0505 dbg config give_n
	/* config give_n to 0x2000(8192) */
	hdmirx_wr_cor(H21RXSB_GN2_M42H_IVCRX, 0x0, port);
	hdmirx_wr_cor(H21RXSB_GN1_M42H_IVCRX, 0x20, port);
	hdmirx_wr_cor(H21RXSB_GN0_M42H_IVCRX, 0x0, port);

	//0505 dbg config post_div
	hdmirx_wr_cor(H21RXSB_POSTDIV_M42H_IVCRX, 0x2, port);

	//0505 dbg config reg_n
	/* fpll ctrl0[20:16] */
	switch (f_rate) {
	case FRL_3G_3LANE:
		odn_reg_n_mul = 4;
		break;
	case FRL_6G_3LANE:
	case FRL_6G_4LANE:
		odn_reg_n_mul = 6;
		break;
	case FRL_8G_4LANE:
		odn_reg_n_mul = 8;
		break;
	case FRL_10G_4LANE:
		odn_reg_n_mul = 8;
		break;
	case FRL_12G_4LANE:
		odn_reg_n_mul = 10;
		break;
	default:
		odn_reg_n_mul = 6;
		break;
	}
	hdmirx_wr_cor(H21RXSB_NMUL_M42H_IVCRX, odn_reg_n_mul, port);

	//0505 dbg reset
	hdmirx_wr_bits_cor(RX_PWD_SRST2_PWD_IVCRX, _BIT(7), 1, port);
	udelay(1);
	hdmirx_wr_bits_cor(RX_PWD_SRST2_PWD_IVCRX, _BIT(7), 0, port);
	udelay(100);
	//wait valid_m
	while (cnt < valid_m_wait_max) {
		cnt++;
		reg_valid_m = hdmirx_rd_bits_cor(H21RXSB_STATUS_M42H_IVCRX, _BIT(0), port);

		//rx_pr("valid_m=%d\n", reg_valid_m);
		if (reg_valid_m)
			break;
		udelay(5);
	}
	cnt = 0;
	if (!reg_valid_m) {
		if (log_level & FRL_LOG)
			rx_pr("m not valid!-0x%x\n",
				hdmirx_rd_cor(H21RXSB_STATUS_M42H_IVCRX, port));
		return;
	}
	if (log_level & FRL_LOG)
		rx_pr("port-%d m valid\n", port);
	//or get flclk based on frl_rate!!! todo
	flclk = rx_get_flclk(port);
	if (!flclk) {
		if (log_level & FRL_LOG)
			rx_pr("flclk invalid\n");
		return;
	}
	o_req_m = ((hdmirx_rd_cor(H21RXSB_REQM2_M42H_IVCRX, port) & 0Xf) << 16) |
		(hdmirx_rd_cor(H21RXSB_REQM1_M42H_IVCRX, port) << 8) |
		hdmirx_rd_cor(H21RXSB_REQM0_M42H_IVCRX, port);
	if (reg_valid_m && o_req_m == 0) {
		if (log_level & FRL_LOG)
			rx_pr("port-%d m not really valid\n", port);
		return;
	}
	/* tclk = flclk * o_req_m / give_n / post_div */
	tclk = flclk * o_req_m / 8192 / 2;
	//rx_pr("flclk * o_req_m=%ld\n", flclk * o_req_m);
	//rx_pr("flclk * o_req_m / 8192=%ld\n", flclk * o_req_m / 8192);
	//rx_pr("flclk * o_req_m / 8192 / 2=%ld\n", flclk * o_req_m / 8192 / 2);
	if (log_level & FRL_LOG)
		rx_pr("flclk = %ld, tclk=%ld\n", flclk, tclk);

	/*
	 * 3. get pre_div
	 */
	//min = 800 * MHz / tclk;
	//max = 1600 * MHz / tclk;
	temp = (800 + 1600) * KHz * 10 / 2 / (tclk / KHz);
	if (temp % 10)
		pre_div = (temp / 10) + 1;
	else
		pre_div = (temp / 10);
	if (log_level & FRL_LOG)
		rx_pr("temp=%lu,pre_div=0x%x\n", temp, pre_div);
	/* check vco */
	if (tclk * pre_div * 2 / KHz < 1600 * KHz) {
		pre_div += 1;
		if (log_level & FRL_LOG)
			rx_pr("error,vco<1.6G!\n");
	}
	if (tclk * pre_div * 2 / KHz > 3200 * KHz) {
		pre_div -= 1;
		if (log_level & FRL_LOG)
			rx_pr("error,vco>3.2G!\n");
	}
	/* fpll cntl2[9:4]*/
	if (pre_div >= 64) {
		pre_div = 64;
		hdmirx_wr_cor(H21RXSB_PREDIV_M42H_IVCRX, 0, port);
	} else {
		if (pre_div == 33)
			pre_div = 31;
		hdmirx_wr_cor(H21RXSB_PREDIV_M42H_IVCRX, pre_div, port);
	}

	/* dbg0606 */
	data = hdmirx_rd_cor(H21RXSB_NMUL_M42H_IVCRX, port);
	data = data * tclk * pre_div * 2;
	data = data / flclk;
	data = data & 0x1ff;
	wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL0, (fpll_t3x[0] & 0xfffffe00) | data |
		(odn_reg_n_mul << 16) | (pre_div == 64 ? (4 << 10) : 0));
	wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL0, (fpll_t3x[1] & 0xfffffe00) | data |
		(odn_reg_n_mul << 16) | (pre_div == 64 ? (4 << 10) : 0));
	wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL1, (fpll_t3x[2] & 0xfff00000) | 0xb9000);
	if (pre_div < 64) {
		wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL2, fpll_t3x[3] |
			((pre_div & 0x1f) << 4));
	} else {
		wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL2, fpll_t3x[3] |
			(4 << 4));
	}
	wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL3, fpll_t3x[4] | (1 << 7));
	wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL0, (fpll_t3x[5] & 0xfffffe00) | data |
		(odn_reg_n_mul << 16) | (pre_div == 64 ? (4 << 10) : 0));
	wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL3, fpll_t3x[6] | (1 << 7) | (1 << 19));
	udelay(10);
	hdmirx_wr_bits_cor(RX_PWD_SRST2_PWD_IVCRX, _BIT(7), 1, port);
	udelay(1);
	hdmirx_wr_bits_cor(RX_PWD_SRST2_PWD_IVCRX, _BIT(7), 0, port);
	udelay(100);
	//wait valid_m
	while (cnt < valid_m_wait_max) {
		cnt++;
		reg_valid_m = hdmirx_rd_bits_cor(H21RXSB_STATUS_M42H_IVCRX, _BIT(0), port);

		if (reg_valid_m)
			break;
		udelay(5);
	}
	cnt = 0;
	if (!reg_valid_m) {
		if (log_level & FRL_LOG)
			rx_pr("m not valid!-0x%x\n",
				hdmirx_rd_cor(H21RXSB_STATUS_M42H_IVCRX, port));
		return;
	}

	/*
	 * 4. fpll config
	 */
	do {
		wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL0, fpll_t3x[0] |
			(odn_reg_n_mul << 16) | (pre_div == 64 ? (4 << 10) : 0));
		wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL0, fpll_t3x[1] |
			(odn_reg_n_mul << 16) | (pre_div == 64 ? (4 << 10) : 0));
		wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL1, fpll_t3x[2]);
		if (pre_div < 64) {
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL2, fpll_t3x[3] |
				((pre_div & 0x1f) << 4));
		} else {
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL2, fpll_t3x[3] |
				(4 << 4));
		}
		wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL3, fpll_t3x[4]);
		wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL0, fpll_t3x[5] |
			(odn_reg_n_mul << 16) | (pre_div == 64 ? (4 << 10) : 0));
		wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL3, fpll_t3x[6]);
		udelay(10);
		if (cnt++ > 5) {
			rx_pr("fpll cfg err!\n");
			break;
		}
	} while (!is_fpll1_locked());
	if (!is_fpll1_locked() || !rx_get_overlap_sts(port) || !rx_get_clkready_sts(port)) {
		//rx_21_dump_fpll_0();
		if (log_level & FRL_LOG) {
			rx_dump_reg_d_f_sts(port);
			rx_pr("post_div=%d\n", hdmirx_rd_cor(H21RXSB_POSTDIV_M42H_IVCRX, port));
			rx_pr("pre_div=%d\n", hdmirx_rd_cor(H21RXSB_PREDIV_M42H_IVCRX, port));
			rx_pr("reg_n=%d\n", hdmirx_rd_cor(H21RXSB_NMUL_M42H_IVCRX, port));
			rx_pr("o_req_m=%ld\n", o_req_m);
			rx_pr("vco=%ld\n", tclk * pre_div * 2);
			rx_pr("0x1525=0x%x", hdmirx_rd_cor(0x1525, port));
		}
	} else {
		rx_pr("fpll locked\n");
	}
	//0505 dbg delay 10us,print fpll lock status
	//rx_pr("ctrl0=0x%x\n", rd_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL0));
}

void rx_21_fpll_cfg_0(int f_rate, u8 port)
{
	int retry_cnt = 0;

	if (fpll_sel) {
		rx_21_fpll_calculation(f_rate, port);
		return;
	}
	do {
		switch (f_rate) {
		case FRL_3G_3LANE:
			/* 1080p */
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL0, 0x21240035);
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL0, 0x30240035);
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL1, 0x83afa82a);
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL2, 0x000400c0);
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL3, 0x0b0da001);
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL0, 0x10240035);
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL3, 0x0b09a201);
			hdmirx_wr_cor(H21RXSB_POSTDIV_M42H_IVCRX, 0x2, port);
			hdmirx_wr_cor(H21RXSB_PREDIV_M42H_IVCRX, 0xc, port);
			odn_reg_n_mul = 4;
			break;
		case FRL_8G_4LANE:
			/* 4k120hz */
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL0, 0x21280035);
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL0, 0x30280035);
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL1, 0x83afa82a);
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL2, 0x00040020);
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL3, 0x0b0da001);
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL0, 0x10280035);
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL3, 0x0b09a201);
			hdmirx_wr_cor(H21RXSB_POSTDIV_M42H_IVCRX, 0x2, port);
			hdmirx_wr_cor(H21RXSB_PREDIV_M42H_IVCRX, 0x2, port);
			odn_reg_n_mul = 8;
			break;
		case FRL_12G_4LANE:
			/* 4k120hz */
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL0, 0x212a0035);
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL0, 0x302a0035);
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL1, 0x83afa82a);
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL2, 0x00040020);
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL3, 0x0b0da001);
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL0, 0x102a0035);
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL3, 0x0b09a201);
			hdmirx_wr_cor(H21RXSB_POSTDIV_M42H_IVCRX, 0x2, port);
			hdmirx_wr_cor(H21RXSB_PREDIV_M42H_IVCRX, 0x2, port);
			odn_reg_n_mul = 10;
			break;
		case FRL_6G_3LANE:
		case FRL_6G_4LANE:
		case FRL_10G_4LANE:
		default:
			break;
		}
		if (retry_cnt++ > 2)
			break;
		//rx_pr("fpll lock=%x\n", is_fpll0_locked());
	} while (!is_fpll0_locked());
	if (log_level & FRL_LOG)
		rx_pr("%s-%d\n", __func__, f_rate);
}

void rx_21_fpll_cfg_1(int f_rate, u8 port)
{
	int retry_cnt = 0;

	if (fpll_sel) {
		rx_21_fpll_calculation1(f_rate, port);
		return;
	}
	do {
		switch (f_rate) {
		case FRL_3G_3LANE:
			/* 1080p */
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL0, 0x21240035);
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL0, 0x30240035);
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL1, 0x83afa82a);
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL2, 0x000400c0);
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL3, 0x0b09a001);
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL0, 0x10240035);
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL3, 0x0b09a201);
			hdmirx_wr_cor(H21RXSB_POSTDIV_M42H_IVCRX, 0x2, port);
			hdmirx_wr_cor(H21RXSB_PREDIV_M42H_IVCRX, 0xc, port);
			odn_reg_n_mul = 4;
			break;
		case FRL_8G_4LANE:
			/* 4k120hz */
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL0, 0x21280035);
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL0, 0x30280035);
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL1, 0x83afa82a);
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL2, 0x00040020);
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL3, 0x0b09a001);
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL0, 0x10280035);
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL3, 0x0b09a201);
			hdmirx_wr_cor(H21RXSB_POSTDIV_M42H_IVCRX, 0x2, port);
			hdmirx_wr_cor(H21RXSB_PREDIV_M42H_IVCRX, 0x2, port);
			odn_reg_n_mul = 8;
			break;
		case FRL_12G_4LANE:
			/* 4k120hz */
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL0, 0x212a0035);
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL0, 0x302a0035);
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL1, 0x83afa82a);
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL2, 0x00040020);
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL3, 0x0b09a001);
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL0, 0x102a0035);
			wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL3, 0x0b09a201);
			hdmirx_wr_cor(H21RXSB_POSTDIV_M42H_IVCRX, 0x2, port);
			hdmirx_wr_cor(H21RXSB_PREDIV_M42H_IVCRX, 0x2, port);
			odn_reg_n_mul = 10;
			break;
		case FRL_6G_3LANE:
		case FRL_6G_4LANE:
		case FRL_10G_4LANE:
		default:
			break;
		}
		if (retry_cnt++ > 2)
			break;
		//rx_pr("fpll lock=%x\n", is_fpll0_locked());
	} while (!is_fpll0_locked());
	if (log_level & FRL_LOG)
		rx_pr("%s-%d\n", __func__, f_rate);
}

void rx_21_fpll_cfg(int f_rate, u8 port)
{
	if (port == E_PORT2)
		rx_21_fpll_cfg_0(f_rate, port);
	else if (port == E_PORT3)
		rx_21_fpll_cfg_1(f_rate, port);
}

void rx_21_dump_fpll(u8 port)
{
	if (port == E_PORT2)
		rx_21_dump_fpll_0();
	else if (port == E_PORT3)
		rx_21_dump_fpll_1();
}

 /* aml_hyper_gain_tuning */
void aml_hyper_gain_tuning_t3x_21(u8 port)
{
	u32 data32;
	u32 tap0, tap1, tap2, tap3;
	u32 hyper_gain_0, hyper_gain_1, hyper_gain_2, hyper_gain_3;

	/* use HYPER_GAIN calibration instead of vga */
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_EYE_EN, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, MUX_BLOCK_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_DFE_OFST_EYE, 0x0, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PHY_DCH_STAT, port);

	tap0 = data32 & 0x7f;
	tap1 = (data32 >> 8) & 0x7f;
	tap2 = (data32 >> 16) & 0x7f;
	tap3 = (data32 >> 24) & 0x7f;
	if (tap0 < 0x12) {
		hyper_gain_0 = 1;
		rx_pr("ch0 amp is too small\n");
	} else {
		hyper_gain_0 = 0;
	}
	if (tap1 < 0x12) {
		hyper_gain_1 = 1;
		rx_pr("ch1 amp is too small\n");
	} else {
		hyper_gain_1 = 0;
	}
	if (tap2 < 0x12) {
		hyper_gain_2 = 1;
		rx_pr("ch2 amp is too small\n");
	} else {
		hyper_gain_2 = 0;
	}
	if (tap3 < 0x12) {
		hyper_gain_3 = 1;
		rx_pr("ch3 amp is too small\n");
	} else {
		hyper_gain_3 = 0;
	}
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_AFE, HYPER_GAIN,
	hyper_gain_0 | (hyper_gain_1 << 1) |
	(hyper_gain_2 << 2) | (hyper_gain_3 << 3), port);
}

 /* For t3x */
void aml_phy_init_t3x_20(u8 port)
{
	if (rx[port].state == FSM_WAIT_CLK_STABLE && !rx[port].cableclk_stb_flg) {
		aml_phy_cfg_t3x_20(port);
		return;
	}
	aml_phy_cfg_t3x_20(port);
	usleep_range(10, 20);
	aml_pll_bw_cfg_t3x_20(port);
	usleep_range(10, 20);
	aml_eq_cfg_t3x_20(port);
}

void aml_phy_init_t3x_21(u8 port)
{
	rx_21_frl_phy_cfg(port);
	if (rx[port].state <= FSM_FRL_FLT_READY || rx[port].state == FSM_WAIT_CLK_STABLE)
		return;
	aml_pll_bw_cfg_t3x_21(rx[port].var.frl_rate, port);
	rx_21_frl_pll_cfg(rx[port].var.frl_rate, port);
	rx_21_eq_cfg(rx[port].var.frl_rate, port);
	if (!fpll_sel)
		rx_21_fpll_cfg(rx[port].var.frl_rate, port);
}

void aml_phy_init_t3x(u8 port)
{
	if (port <= E_PORT1)
		aml_phy_init_t3x_20(port);
	else
		aml_phy_init_t3x_21(port);
}

void dump_reg_phy_t3x_20(u8 port)
{
	rx_pr("2.0 PHY Register:\n");
	rx_pr("dchd_eq-0x14=0x%x\n", hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, port));
	rx_pr("dchd_cdr-0x10=0x%x\n", hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, port));
	rx_pr("dcha_dfe-0xc=0x%x\n", hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_DFE, port));
	rx_pr("dcha_afe-0x8=0x%x\n", hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_AFE, port));
	rx_pr("misc2-0x1c=0x%x\n", hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_MISC2, port));
	rx_pr("misc1-0x18=0x%x\n", hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_MISC1, port));
	rx_pr("pll0-0x0=0x%x\n", hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PLL_CTRL0, port));
	rx_pr("pll1-0x4=0x%x\n", hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PLL_CTRL1, port));
}

void dump_reg_phy_t3x_21(u8 port)
{
	rx_pr("2.1 PHY Register:\n");
	rx_pr("RX21PLL_CTRL0-0x100=0x%x\n",
		hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, port));
	rx_pr("RX21PLL_CTRL1-0x104=0x%x\n",
		hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PLL_CTRL1, port));
	rx_pr("RX21PLL_CTRL2-0x108=0x%x\n",
		hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PLL_CTRL2, port));
	rx_pr("RX21PLL_CTRL3-0x10c=0x%x\n",
		hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PLL_CTRL3, port));
	rx_pr("RX21PLL_CTRL4-0x110=0x%x\n",
		hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PLL_CTRL4, port));
	rx_pr("RX21PHY_MISC0-0x114=0x%x\n",
		hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PHY_MISC0, port));
	rx_pr("RX21PHY_MISC1-0x118=0x%x\n",
		hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PHY_MISC1, port));
	rx_pr("RX21PHY_MISC2-0x11c=0x%x\n",
		hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PHY_MISC2, port));
	rx_pr("RX21PHY_DCHA_AFE-0x120=0x%x\n",
		hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_AFE, port));
	rx_pr("RX21PHY_DCHA_DFE-0x124=0x%x\n",
		hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_DFE, port));
	rx_pr("RX21PHY_DCHA_PI-0x128=0x%x\n",
		hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_PI, port));
	rx_pr("RX21PHY_DCHA_CTRL-0x12c=0x%x\n",
		hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_CTRL, port));
	rx_pr("RX21PHY_DCHD_CDR-0x130=0x%x\n",
		hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, port));
	rx_pr("RX21PHY_DCHD_EQ-0x134=0x%x\n",
		hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, port));
}

void dump_reg_phy_t3x(u8 port)
{
	if (port <= E_PORT1) {
		dump_reg_phy_t3x_20(port);
	} else {
		dump_reg_phy_t3x_21(port);
		rx_21_dump_fpll(port);
	}
}
/*
 * rx phy v2 debug
 */
int count_one_bits_t3x(u32 value)
{
	int count = 0;

	for (; value != 0; value >>= 1) {
		if (value & 1)
			count++;
	}
	return count;
}

void get_val_t3x(char *temp, unsigned int val, int len)
{
	if ((val >> (len - 1)) == 0)
		sprintf(temp, "+%d", val & (~(1 << (len - 1))));
	else
		sprintf(temp, "-%d", val & (~(1 << (len - 1))));
}

void get_flag_val_t3x(char *temp, unsigned int val, int len)
{
	if ((val >> (len - 1)) == 0)
		sprintf(temp, "-%d", val & (~(1 << (len - 1))));
	else
		sprintf(temp, "+%d", val & (~(1 << (len - 1))));
}

void comb_val_t3x(void (*p)(char *, unsigned int, int),
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

void dump_aml_phy_sts_t3x_20(u8 port)
{
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
	terminal = (hdmirx_rd_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_MISC1, T3X_20_RTERM_CNTL, port));

	/* eq_boost1 status */
	/* mux_eye_en */
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_EHM_DBG_SEL, 0x0, port);
	/* mux_block_sel */
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_STATUS_MUX_SEL, 0x3, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_STAT, port);
	ch0_eq_boost1 = data32 & 0x1f;
	ch0_eq_err = (data32 >> 5) & 0x3;
	ch1_eq_boost1 = (data32 >> 8) & 0x1f;
	ch1_eq_err = (data32 >> 13) & 0x3;
	ch2_eq_boost1 = (data32 >> 16) & 0x1f;
	ch2_eq_err = (data32 >> 21) & 0x3;

	/* dfe tap0 sts */
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_EHM_DBG_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_STATUS_MUX_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_DFE_OFST_DBG_SEL, 0x0, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_STAT, port);
	dfe0_tap0 = data32 & 0x7f;
	dfe1_tap0 = (data32 >> 8) & 0x7f;
	dfe2_tap0 = (data32 >> 16) & 0x7f;
	dfe3_tap0 = (data32 >> 24) & 0x7f;
	/* dfe tap1 sts */
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_EHM_DBG_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_STATUS_MUX_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_DFE_OFST_DBG_SEL, 0x1, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_STAT, port);
	dfe0_tap1 = data32 & 0x3f;
	dfe1_tap1 = (data32 >> 8) & 0x3f;
	dfe2_tap1 = (data32 >> 16) & 0x3f;
	dfe3_tap1 = (data32 >> 24) & 0x3f;
	/* dfe tap2 sts */
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_EHM_DBG_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_STATUS_MUX_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_DFE_OFST_DBG_SEL, 0x2, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_STAT, port);
	dfe0_tap2 = data32 & 0x1f;
	dfe1_tap2 = (data32 >> 8) & 0x1f;
	dfe2_tap2 = (data32 >> 16) & 0x1f;
	dfe3_tap2 = (data32 >> 24) & 0x1f;
	/* dfe tap3 sts */
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_EHM_DBG_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_STATUS_MUX_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_DFE_OFST_DBG_SEL, 0x3, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_STAT, port);
	dfe0_tap3 = data32 & 0xf;
	dfe1_tap3 = (data32 >> 8) & 0xf;
	dfe2_tap3 = (data32 >> 16) & 0xf;
	dfe3_tap3 = (data32 >> 24) & 0xf;
	/* dfe tap4 sts */
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_EHM_DBG_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_STATUS_MUX_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_DFE_OFST_DBG_SEL, 0x4, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_STAT, port);
	dfe0_tap4 = data32 & 0xf;
	dfe1_tap4 = (data32 >> 8) & 0xf;
	dfe2_tap4 = (data32 >> 16) & 0xf;
	dfe3_tap4 = (data32 >> 24) & 0xf;
	/* dfe tap5 sts */
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_EHM_DBG_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_STATUS_MUX_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_DFE_OFST_DBG_SEL, 0x5, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_STAT, port);
	dfe0_tap5 = data32 & 0xf;
	dfe1_tap5 = (data32 >> 8) & 0xf;
	dfe2_tap5 = (data32 >> 16) & 0xf;
	dfe3_tap5 = (data32 >> 24) & 0xf;
	/* dfe tap6 sts */
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_EHM_DBG_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_STATUS_MUX_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_DFE_OFST_DBG_SEL, 0x6, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_STAT, port);
	dfe0_tap6 = data32 & 0xf;
	dfe1_tap6 = (data32 >> 8) & 0xf;
	dfe2_tap6 = (data32 >> 16) & 0xf;
	dfe3_tap6 = (data32 >> 24) & 0xf;
	/* dfe tap7/8 sts */
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_EHM_DBG_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_STATUS_MUX_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_DFE_OFST_DBG_SEL, 0x7, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_STAT, port);
	dfe0_tap7 = data32 & 0xf;
	dfe1_tap7 = (data32 >> 8) & 0xf;
	dfe2_tap7 = (data32 >> 16) & 0xf;
	dfe3_tap7 = (data32 >> 24) & 0xf;
	dfe0_tap8 = (data32 >> 4) & 0xf;
	dfe1_tap8 = (data32 >> 12) & 0xf;
	dfe2_tap8 = (data32 >> 20) & 0xf;
	dfe3_tap8 = (data32 >> 24) & 0xf;

	/* CDR status */
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_EHM_DBG_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_STATUS_MUX_SEL, 0x22, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_MUX_CDR_DBG_SEL, 0x0, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_STAT, port);
	cdr0_code = data32 & 0x7f;
	cdr0_lock = (data32 >> 7) & 0x1;
	cdr1_code = (data32 >> 8) & 0x7f;
	cdr1_lock = (data32 >> 15) & 0x1;
	cdr2_code = (data32 >> 16) & 0x7f;
	cdr2_lock = (data32 >> 23) & 0x1;
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_MUX_CDR_DBG_SEL, 0x1, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_STAT, port);
	cdr0_int = data32 & 0x7f;
	cdr1_int = (data32 >> 8) & 0x7f;
	cdr2_int = (data32 >> 16) & 0x7f;

	/* pll lock */
	pll_lock = hdmirx_rd_amlphy_t3x(T3X_RG_RX20PLL_0, port) >> 31;

	/* squelch */
	squelch = hdmirx_rd_top(TOP_MISC_STAT0, port) & 0x1;

	/* slicer offset status */
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_EHM_DBG_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_STATUS_MUX_SEL, 0x1, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_DFE_OFST_DBG_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_MUX_CDR_DBG_SEL, 0x1, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_STAT, port);
	sli0_ofst0 = data32 & 0x1f;
	sli1_ofst0 = (data32 >> 8) & 0x1f;
	sli2_ofst0 = (data32 >> 16) & 0x1f;

	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_EHM_DBG_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_STATUS_MUX_SEL, 0x1, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_DFE_OFST_DBG_SEL, 0x1, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_MUX_CDR_DBG_SEL, 0x1, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_STAT, port);
	sli0_ofst1 = data32 & 0x1f;
	sli1_ofst1 = (data32 >> 8) & 0x1f;
	sli2_ofst1 = (data32 >> 16) & 0x1f;

	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_EHM_DBG_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, T3X_20_STATUS_MUX_SEL, 0x1, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_DFE_OFST_DBG_SEL, 0x2, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, T3X_20_MUX_CDR_DBG_SEL, 0x1, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_STAT, port);
	sli0_ofst2 = data32 & 0x1f;
	sli1_ofst2 = (data32 >> 8) & 0x1f;
	sli2_ofst2 = (data32 >> 16) & 0x1f;

	rx_pr("\nhdmirx phy status:\n");
	rx_pr("pll_lock=%d, squelch=%d, terminal=%d\n", pll_lock, squelch, terminal);
	rx_pr("eq_boost1=[%d,%d,%d]\n",
	      ch0_eq_boost1, ch1_eq_boost1, ch2_eq_boost1);
	rx_pr("eq_err=[%d,%d,%d]\n",
	      ch0_eq_err, ch1_eq_err, ch2_eq_err);

	comb_val_t3x(get_val_t3x, "dfe_tap0", dfe0_tap0, dfe1_tap0, dfe2_tap0, 7);
	comb_val_t3x(get_val_t3x, "dfe_tap1", dfe0_tap1, dfe1_tap1, dfe2_tap1, 6);
	comb_val_t3x(get_val_t3x, "dfe_tap2", dfe0_tap2, dfe1_tap2, dfe2_tap2, 5);
	comb_val_t3x(get_val_t3x, "dfe_tap3", dfe0_tap3, dfe1_tap3, dfe2_tap3, 4);
	comb_val_t3x(get_val_t3x, "dfe_tap4", dfe0_tap4, dfe1_tap4, dfe2_tap4, 4);
	comb_val_t3x(get_val_t3x, "dfe_tap5", dfe0_tap5, dfe1_tap5, dfe2_tap5, 4);
	comb_val_t3x(get_val_t3x, "dfe_tap6", dfe0_tap6, dfe1_tap6, dfe2_tap6, 4);
	comb_val_t3x(get_val_t3x, "dfe_tap7", dfe0_tap7, dfe1_tap7, dfe2_tap7, 4);
	comb_val_t3x(get_val_t3x, "dfe_tap8", dfe0_tap8, dfe1_tap8, dfe2_tap8, 4);

	comb_val_t3x(get_val_t3x, "slicer_ofst0", sli0_ofst0, sli1_ofst0, sli2_ofst0, 5);
	comb_val_t3x(get_val_t3x, "slicer_ofst1", sli0_ofst1, sli1_ofst1, sli2_ofst1, 5);
	comb_val_t3x(get_val_t3x, "slicer_ofst2", sli0_ofst2, sli1_ofst2, sli2_ofst2, 5);

	rx_pr("cdr_code=[%d,%d,%d]\n", cdr0_code, cdr1_code, cdr2_code);
	rx_pr("cdr_lock=[%d,%d,%d]\n", cdr0_lock, cdr1_lock, cdr2_lock);
	comb_val_t3x(get_val_t3x, "cdr_int", cdr0_int, cdr1_int, cdr2_int, 7);
}

void get_val_21(char *temp, unsigned int val, int len)
{
	if ((val >> (len - 1)) == 0)
		sprintf(temp, "+%d", val & (~(1 << (len - 1))));
	else
		sprintf(temp, "-%d", val & (~(1 << (len - 1))));
}

void comb_val_21(char *type, unsigned int val_0, unsigned int val_1,
		 unsigned int val_2, unsigned int val_3, int len)
{
	char out[32], v0_buf[16], v1_buf[16], v2_buf[16], v3_buf[16];
	int pos = 0;

	get_val_21(v0_buf, val_0, len);
	get_val_21(v1_buf, val_1, len);
	get_val_21(v2_buf, val_2, len);
	get_val_21(v3_buf, val_3, len);
	pos += snprintf(out + pos, 32 - pos, "%s[", type);
	pos += snprintf(out + pos, 32 - pos, " %s,", v0_buf);
	pos += snprintf(out + pos, 32 - pos, " %s,", v1_buf);
	pos += snprintf(out + pos, 32 - pos, " %s,", v2_buf);
	pos += snprintf(out + pos, 32 - pos, " %s]", v3_buf);
	rx_pr("%s\n", out);
}

void dump_aml_phy_sts_t3x_21(u8 port)
{
	u32 data32;
	u32 vga_ch0, vga_ch1, vga_ch2, vga_ch3;
	u32 ch0_eq_boost1, ch1_eq_boost1, ch2_eq_boost1, ch3_eq_boost1;
	u32 ch0_eq_err, ch1_eq_err, ch2_eq_err, ch3_eq_err;
	u32 dfe0_tap0, dfe1_tap0, dfe2_tap0, dfe3_tap0;
	u32 dfe0_tap1, dfe1_tap1, dfe2_tap1, dfe3_tap1;
	u32 dfe0_tap2, dfe1_tap2, dfe2_tap2, dfe3_tap2;
	u32 dfe0_tap3, dfe1_tap3, dfe2_tap3, dfe3_tap3;
	u32 dfe0_tap4, dfe1_tap4, dfe2_tap4, dfe3_tap4;
	u32 dfe0_tap5, dfe1_tap5, dfe2_tap5, dfe3_tap5;
	u32 dfe0_tap6, dfe1_tap6, dfe2_tap6, dfe3_tap6;
	u32 dfe0_tap7, dfe1_tap7, dfe2_tap7, dfe3_tap7;
	u32 dfe0_tap8, dfe1_tap8, dfe2_tap8, dfe3_tap8;

	u32 cdr0_lock, cdr1_lock, cdr2_lock, cdr3_lock;
	u32 cdr0_int, cdr1_int, cdr2_int, cdr3_int;
	u32 cdr0_code, cdr1_code, cdr2_code, cdr3_code;
	bool pll_lock, lan0_lock, lan1_lock, lan2_lock, lan3_lock;
	bool squelch;

	u32 sli0_ofst0, sli1_ofst0, sli2_ofst0, sli3_ofst0;
	u32 sli0_ofst1, sli1_ofst1, sli2_ofst1, sli3_ofst1;
	u32 sli0_ofst2, sli1_ofst2, sli2_ofst2, sli3_ofst2;
	u32 sli0_ofst3, sli1_ofst3, sli2_ofst3, sli3_ofst3;
	u32 sli0_ofst4, sli1_ofst4, sli2_ofst4, sli3_ofst4;
	u32 sli0_ofst5, sli1_ofst5, sli2_ofst5, sli3_ofst5;
	/* eq_boost1 status */
	/* mux_eye_en */
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_EYE_EN, 0x0, port);
	/* mux_block_sel */
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, MUX_BLOCK_SEL, 0x3, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PHY_DCH_STAT, port);
	ch0_eq_boost1 = data32 & 0x1f;
	ch0_eq_err = (data32 >> 5) & 0x3;
	ch1_eq_boost1 = (data32 >> 8) & 0x1f;
	ch1_eq_err = (data32 >> 13) & 0x3;
	ch2_eq_boost1 = (data32 >> 16) & 0x1f;
	ch2_eq_err = (data32 >> 21) & 0x3;
	ch3_eq_boost1 = (data32 >> 24) & 0x1f;
	ch3_eq_err = (data32 >> 29) & 0x3;

	vga_ch0 = graytodecimal_t3x(hdmirx_rd_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_AFE,
		MSK(4, 0), port));
	vga_ch1 = graytodecimal_t3x(hdmirx_rd_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_AFE,
		MSK(4, 4), port));
	vga_ch2 = graytodecimal_t3x(hdmirx_rd_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_AFE,
		MSK(4, 8), port));
	vga_ch3 = graytodecimal_t3x(hdmirx_rd_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_AFE,
		MSK(4, 12), port));
	/* dfe tap0 sts */
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_EYE_EN, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, MUX_BLOCK_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_DFE_OFST_EYE, 0x0, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PHY_DCH_STAT, port);
	dfe0_tap0 = data32 & 0x7f;
	dfe1_tap0 = (data32 >> 8) & 0x7f;
	dfe2_tap0 = (data32 >> 16) & 0x7f;
	dfe3_tap0 = (data32 >> 24) & 0x7f;
	/* dfe tap1 sts */
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_EYE_EN, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, MUX_BLOCK_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_DFE_OFST_EYE, 0x1, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PHY_DCH_STAT, port);
	dfe0_tap1 = data32 & 0x3f;
	dfe1_tap1 = (data32 >> 8) & 0x3f;
	dfe2_tap1 = (data32 >> 16) & 0x3f;
	dfe3_tap1 = (data32 >> 24) & 0x3f;
	/* dfe tap2 sts */
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_EYE_EN, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, MUX_BLOCK_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_DFE_OFST_EYE, 0x2, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PHY_DCH_STAT, port);
	dfe0_tap2 = data32 & 0x1f;
	dfe1_tap2 = (data32 >> 8) & 0x1f;
	dfe2_tap2 = (data32 >> 16) & 0x1f;
	dfe3_tap2 = (data32 >> 24) & 0x1f;
	/* dfe tap3 sts */
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_EYE_EN, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, MUX_BLOCK_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_DFE_OFST_EYE, 0x3, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PHY_DCH_STAT, port);
	dfe0_tap3 = data32 & 0xf;
	dfe1_tap3 = (data32 >> 8) & 0xf;
	dfe2_tap3 = (data32 >> 16) & 0xf;
	dfe3_tap3 = (data32 >> 24) & 0xf;
	/* dfe tap4 sts */
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_EYE_EN, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, MUX_BLOCK_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_DFE_OFST_EYE, 0x4, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PHY_DCH_STAT, port);
	dfe0_tap4 = data32 & 0xf;
	dfe1_tap4 = (data32 >> 8) & 0xf;
	dfe2_tap4 = (data32 >> 16) & 0xf;
	dfe3_tap4 = (data32 >> 24) & 0xf;
	/* dfe tap5 sts */
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_EYE_EN, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, MUX_BLOCK_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_DFE_OFST_EYE, 0x5, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PHY_DCH_STAT, port);
	dfe0_tap5 = data32 & 0xf;
	dfe1_tap5 = (data32 >> 8) & 0xf;
	dfe2_tap5 = (data32 >> 16) & 0xf;
	dfe3_tap5 = (data32 >> 24) & 0xf;
	/* dfe tap6 sts */
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_EYE_EN, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, MUX_BLOCK_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_DFE_OFST_EYE, 0x6, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PHY_DCH_STAT, port);
	dfe0_tap6 = data32 & 0xf;
	dfe1_tap6 = (data32 >> 8) & 0xf;
	dfe2_tap6 = (data32 >> 16) & 0xf;
	dfe3_tap6 = (data32 >> 24) & 0xf;
	/* dfe tap7/8 sts */
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_EYE_EN, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, MUX_BLOCK_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_DFE_OFST_EYE, 0x7, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PHY_DCH_STAT, port);
	dfe0_tap7 = data32 & 0xf;
	dfe1_tap7 = (data32 >> 8) & 0xf;
	dfe2_tap7 = (data32 >> 16) & 0xf;
	dfe3_tap7 = (data32 >> 24) & 0xf;
	dfe0_tap8 = (data32 >> 4) & 0xf;
	dfe1_tap8 = (data32 >> 12) & 0xf;
	dfe2_tap8 = (data32 >> 20) & 0xf;
	dfe3_tap8 = (data32 >> 24) & 0xf;

	/* CDR status */
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_EYE_EN, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, MUX_BLOCK_SEL, 0x2, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_CDR_DBG_SEL, 0x0, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PHY_DCH_STAT, port);
	cdr0_code = data32 & 0x7f;
	cdr0_lock = (data32 >> 7) & 0x1;
	cdr1_code = (data32 >> 8) & 0x7f;
	cdr1_lock = (data32 >> 15) & 0x1;
	cdr2_code = (data32 >> 16) & 0x7f;
	cdr2_lock = (data32 >> 23) & 0x1;
	cdr3_code = (data32 >> 24) & 0x7f;
	cdr3_lock = (data32 >> 31) & 0x1;
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_CDR_DBG_SEL, 0x1, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PHY_DCH_STAT, port);
	cdr0_int = data32 & 0x7f;
	cdr1_int = (data32 >> 8) & 0x7f;
	cdr2_int = (data32 >> 16) & 0x7f;
	cdr3_int = (data32 >> 24) & 0x7f;

	/* pll lock */
	pll_lock = hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, port) >> 31;

	/* squelch */
	squelch = hdmirx_rd_top(TOP_MISC_STAT0, port) & 0x1;

	/* slicer offset status */
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_EYE_EN, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, MUX_BLOCK_SEL, 0x1, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_DFE_OFST_EYE, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_CDR_DBG_SEL, 0x1, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PHY_DCH_STAT, port);
	sli0_ofst0 = (data32 >> 1) & 0x1f;
	sli1_ofst0 = (data32 >> 9) & 0x1f;
	sli2_ofst0 = (data32 >> 17) & 0x1f;
	sli3_ofst0 = (data32 >> 25) & 0x1f;

	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_EYE_EN, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, MUX_BLOCK_SEL, 0x1, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_DFE_OFST_EYE, 0x1, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_CDR_DBG_SEL, 0x1, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PHY_DCH_STAT, port);
	sli0_ofst1 = (data32 >> 1) & 0x1f;
	sli1_ofst1 = (data32 >> 9) & 0x1f;
	sli2_ofst1 = (data32 >> 17) & 0x1f;
	sli3_ofst1 = (data32 >> 25) & 0x1f;

	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_EYE_EN, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, MUX_BLOCK_SEL, 0x1, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_DFE_OFST_EYE, 0x2, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_CDR_DBG_SEL, 0x1, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PHY_DCH_STAT, port);
	sli0_ofst2 = (data32 >> 1) & 0x1f;
	sli1_ofst2 = (data32 >> 9) & 0x1f;
	sli2_ofst2 = (data32 >> 17) & 0x1f;
	sli3_ofst2 = (data32 >> 25) & 0x1f;

	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_EYE_EN, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, MUX_BLOCK_SEL, 0x1, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_DFE_OFST_EYE, 0x3, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_CDR_DBG_SEL, 0x1, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PHY_DCH_STAT, port);
	sli0_ofst3 = (data32 >> 1) & 0x1f;
	sli1_ofst3 = (data32 >> 9) & 0x1f;
	sli2_ofst3 = (data32 >> 17) & 0x1f;
	sli3_ofst3 = (data32 >> 25) & 0x1f;

	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_EYE_EN, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, MUX_BLOCK_SEL, 0x1, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_DFE_OFST_EYE, 0x4, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_CDR_DBG_SEL, 0x1, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PHY_DCH_STAT, port);
	sli0_ofst4 = (data32 >> 1) & 0x1f;
	sli1_ofst4 = (data32 >> 9) & 0x1f;
	sli2_ofst4 = (data32 >> 17) & 0x1f;
	sli3_ofst4 = (data32 >> 25) & 0x1f;

	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_EYE_EN, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, MUX_BLOCK_SEL, 0x1, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_DFE_OFST_EYE, 0x5, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_CDR_DBG_SEL, 0x1, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PHY_DCH_STAT, port);
	sli0_ofst5 = (data32 >> 1) & 0x1f;
	sli1_ofst5 = (data32 >> 9) & 0x1f;
	sli2_ofst5 = (data32 >> 17) & 0x1f;
	sli3_ofst5 = (data32 >> 25) & 0x1f;

	data32 = hdmirx_rd_top(TOP_LANE01_ERRCNT, port);
	lan0_lock = (data32 >> 15) & 1;
	lan1_lock = (data32 >> 31) & 1;
	data32 = hdmirx_rd_top(TOP_LANE23_ERRCNT, port);
	lan2_lock = (data32 >> 15) & 1;
	lan3_lock = (data32 >> 31) & 1;
	rx_pr("\nhdmirx phy status:\n");
	if (rx[port].var.frl_rate == FRL_OFF)
		;//rx_pr("data_rate=tmds-%d\n", tmds_clk_msr());
	else if (rx[port].var.frl_rate == FRL_3G_3LANE)
		rx_pr("data_rate=FRL_3G_3LANE\n");
	else if (rx[port].var.frl_rate == FRL_6G_3LANE)
		rx_pr("data_rate=FRL_6G_3LANE\n");
	else if (rx[port].var.frl_rate == FRL_6G_4LANE)
		rx_pr("data_rate=FRL_6G_4LANE\n");
	else if (rx[port].var.frl_rate == FRL_8G_4LANE)
		rx_pr("data_rate=FRL_8G_4LANE\n");
	else if (rx[port].var.frl_rate == FRL_10G_4LANE)
		rx_pr("data_rate=FRL_10G_4LANE\n");
	else if (rx[port].var.frl_rate == FRL_12G_4LANE)
		rx_pr("data_rate=FRL_12G_4LANE\n");
	rx_pr("pll_lock=%d, squelch=%d\n", pll_lock, squelch);

	rx_pr("vga_gain =[%d,%d,%d,%d]\n",
	      vga_ch0, vga_ch1, vga_ch2, vga_ch3);
	rx_pr("eq_boost1=[%d,%d,%d,%d]\n",
	      ch0_eq_boost1, ch1_eq_boost1, ch2_eq_boost1, ch3_eq_boost1);
	rx_pr("eq_err=[%d,%d,%d,%d]\n",
	      ch0_eq_err, ch1_eq_err, ch2_eq_err, ch3_eq_err);

	comb_val_21("	 dfe_tap0", dfe0_tap0, dfe1_tap0, dfe2_tap0, dfe3_tap0, 7);
	comb_val_21("	 dfe_tap1", dfe0_tap1, dfe1_tap1, dfe2_tap1, dfe3_tap1, 6);
	comb_val_21("	 dfe_tap2", dfe0_tap2, dfe1_tap2, dfe2_tap2, dfe3_tap2, 5);
	comb_val_21("	 dfe_tap3", dfe0_tap3, dfe1_tap3, dfe2_tap3, dfe3_tap3, 4);
	comb_val_21("	 dfe_tap4", dfe0_tap4, dfe1_tap4, dfe2_tap4, dfe3_tap4, 4);
	comb_val_21("	 dfe_tap5", dfe0_tap5, dfe1_tap5, dfe2_tap5, dfe3_tap5, 4);
	comb_val_21("	 dfe_tap6", dfe0_tap6, dfe1_tap6, dfe2_tap6, dfe3_tap6, 4);
	comb_val_21("	 dfe_tap7", dfe0_tap7, dfe1_tap7, dfe2_tap7, dfe3_tap7, 4);
	comb_val_21("	 dfe_tap8", dfe0_tap8, dfe1_tap8, dfe2_tap8, dfe3_tap8, 4);

	comb_val_21("slicer_ofst0", sli0_ofst0, sli1_ofst0, sli2_ofst0, sli3_ofst0, 5);
	comb_val_21("slicer_ofst1", sli0_ofst1, sli1_ofst1, sli2_ofst1, sli3_ofst1, 5);
	comb_val_21("slicer_ofst2", sli0_ofst2, sli1_ofst2, sli2_ofst2, sli3_ofst2, 5);
	comb_val_21("slicer_ofst3", sli0_ofst3, sli1_ofst3, sli2_ofst3, sli3_ofst3, 5);
	comb_val_21("slicer_ofst4", sli0_ofst4, sli1_ofst4, sli2_ofst4, sli3_ofst4, 5);
	comb_val_21("slicer_ofst5", sli0_ofst5, sli1_ofst5, sli2_ofst5, sli3_ofst5, 5);

	rx_pr("cdr_code=[%d,%d,%d,%d]\n", cdr0_code, cdr1_code, cdr2_code, cdr3_code);
	rx_pr("cdr_lock=[%d,%d,%d,%d]\n", cdr0_lock, cdr1_lock, cdr2_lock, cdr3_lock);
	comb_val_21("cdr_int", cdr0_int, cdr1_int, cdr2_int, cdr3_int, 7);
}

void dump_aml_phy_sts_t3x(u8 port)
{
	if (port <= E_PORT1)
		dump_aml_phy_sts_t3x_20(port);
	else
		dump_aml_phy_sts_t3x_21(port);
}

bool aml_get_tmds_valid_t3x_20(u8 port)
{
	u32 tmdsclk_valid;
	u32 sqofclk;
	u32 tmds_align;
	u32 ret;
	/* digital tmds valid depends on PLL lock from analog phy. */
	/* it is not necessary and T7 has not it */
	/* tmds_valid = hdmirx_rd_dwc(DWC_HDMI_PLL_LCK_STS) & 0x01; */
	sqofclk = hdmirx_rd_top(TOP_MISC_STAT0_T3X, port) & 0x1;
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

bool aml_get_tmds_valid_t3x_21(u8 port)
{
	u32 tmdsclk_valid;
	u32 sqofclk;
	u32 tmds_align;
	u32 ret;

	/* frl_debug todo */
	if (rx[port].var.frl_rate)
		return true;
	/* digital tmds valid depends on PLL lock from analog phy. */
	/* it is not necessary and T7 has not it */
	/* tmds_valid = hdmirx_rd_dwc(DWC_HDMI_PLL_LCK_STS) & 0x01; */
	sqofclk = hdmirx_rd_top(TOP_MISC_STAT0_T3X, port) & 0x1;
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

bool aml_get_tmds_valid_t3x(u8 port)
{
	if (port <= E_PORT1)
		return aml_get_tmds_valid_t3x_20(port);
	else
		return aml_get_tmds_valid_t3x_21(port);
}

void aml_phy_short_bist_t3x_20(void)
{
	int data32;
	int bist_mode = 3;
	int port;
	int ch0_lock = 0;
	int ch1_lock = 0;
	int ch2_lock = 0;
	int lock_sts = 0;

	for (port = 0; port < 3; port++) {
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, 0x30000050, port);
		usleep_range(5, 10);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, 0x04007053, port);
		usleep_range(5, 10);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_DFE, 0x7ff00459, port);
		usleep_range(5, 10);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_MISC2, 0x11c73228, port);
		usleep_range(5, 10);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_MISC2, 0xfff00100, port);
		usleep_range(5, 10);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX20PLL_CTRL0, 0x0500f800, port);
		usleep_range(10, 20);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX20PLL_CTRL1, 0x01481236, port);
		usleep_range(10, 20);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX20PLL_CTRL0, 0x0500f801, port);
		usleep_range(10, 20);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX20PLL_CTRL0, 0x0500f803, port);
		usleep_range(10, 20);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX20PLL_CTRL1, 0x01401236, port);
		usleep_range(10, 20);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX20PLL_CTRL0, 0x0500f807, port);
		usleep_range(10, 20);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX20PLL_CTRL0, 0x4500f807, port);
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
		hdmirx_wr_top(TOP_PRBS_ANA_0, data32, port);
		usleep_range(5, 10);
		data32			= 0;
		// [19: 8] prbs_ana_time_window
		data32	|=	255 << 8;
		// [ 7: 0] prbs_ana_err_thr
		data32	|=	0;
		hdmirx_wr_top(TOP_PRBS_ANA_1, data32, port);
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

void aml_phy_short_bist_t3x_21(void)
{
	//todo
}

void aml_phy_short_bist_t3x(u8 port)
{
	if (port <= E_PORT1)
		aml_phy_short_bist_t3x_20();
	else
		aml_phy_short_bist_t3x_21();
}

int aml_phy_get_iq_skew_val_t3x(u32 val_0, u32 val_1)
{
	int val = val_0 - val_1;

	rx_pr("val=%d\n", val);
	if (val)
		return (val - 32);
	else
		return (val + 128 - 32);
}

/* IQ skew monitor */
void aml_phy_iq_skew_monitor_t3x(void)
{
}

void rx_set_term_value_t3x_20(unsigned char port, bool value)
{
	u32 data32;

	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_MISC2, port);
	if (value) {
		data32 |= (1 << (28 + port));
	} else {
		/* rst cdr to clr tmds_valid */
		//data32 &= ~(MSK(3, 7));
		data32 &= ~(1 << (28 + port));
	}
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_MISC2, data32, port);
}

void rx_set_term_value_t3x_21(unsigned char port, bool value)
{
	u32 data32;

	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PHY_MISC0, port);
	if (value) {
		data32 |= (1 << (22 + port));
	} else {
		/* rst cdr to clr tmds_valid */
		//data32 &= ~(MSK(3, 7));
		data32 &= ~(1 << (22 + port));
	}
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_MISC0, data32, port);
}

void rx_set_term_value_t3x(unsigned char port, bool value)
{
	if (port <= E_PORT1) {
		rx_set_term_value_t3x_20(port, value);
	} else if (port <= E_PORT3) {
		rx_set_term_value_t3x_21(port, value);
	} else {
		rx_set_term_value_t3x_20(E_PORT0, value);
		rx_set_term_value_t3x_20(E_PORT1, value);
		rx_set_term_value_t3x_21(E_PORT2, value);
		rx_set_term_value_t3x_21(E_PORT3, value);
	}
}

void aml_phy_power_off_t3x_20(u8 port)
{
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX20PLL_CTRL0, 0x0, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX20PLL_CTRL1, 0x0, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_AFE, 0x0, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_DFE, 0x0, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_CDR, 0x0, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_EQ, 0x0, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_MISC1, 0x0, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_MISC2, 0x0, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX20PHY_DCHD_STAT, 0x0, port);
	//hdmirx_wr_amlphy_t3x(T3X_HDMIRX_EARCTX_CNTL0, 0x0, E_PORT0);
	//hdmirx_wr_amlphy_t3x(T3X_HDMIRX_EARCTX_CNTL1, 0x0, E_PORT0);
	//hdmirx_wr_amlphy_t3x(T3X_HDMIRX_ARC_CNTL, 0x0, E_PORT0);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX_PHY_PROD_TEST0, 0x0, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX_PHY_PROD_TEST1, 0x0, port);
}

void aml_phy_power_off_t3x_21(u8 port)
{
	/* power off phy and pll */
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x0, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL1, 0x0, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL2, 0x0, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL3, 0x0, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL4, 0x0, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_MISC0, 0x0, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_MISC1, 0x0, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_MISC2, 0x0, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_AFE, 0x0, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_DFE, 0x0, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_PI, 0x0, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_CTRL, 0x0, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, 0x0, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, 0x0, port);
	/* poweroff port C FPLL */
	if (port == E_PORT2) {
		wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL0, 0x0);
		wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL1, 0x0);
		wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL2, 0x0);
		wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL3, 0x0);
	}

	if (port == E_PORT3) {
		wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL0, 0x0);
		wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL1, 0x0);
		wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL2, 0x0);
		wr_reg_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL3, 0x0);
	}
}

void aml_phy_power_off_t3x(u8 port)
{
	if (port <= E_PORT1) {
		aml_phy_power_off_t3x_20(port);
	} else if (port <= E_PORT3) {
		aml_phy_power_off_t3x_21(port);
	} else {
		aml_phy_power_off_t3x_20(port);
		aml_phy_power_off_t3x_21(port);
	}
}

void aml_phy_offset_cal_t3x(void)
{
	aml_phy_offset_cal_t3x_20(E_PORT0);
	usleep_range(10, 20);
	aml_phy_offset_cal_t3x_20(E_PORT1);
	usleep_range(10, 20);
	aml_phy_offset_cal_t3x_21(E_PORT2);
	usleep_range(10, 20);
	aml_phy_offset_cal_t3x_21(E_PORT3);
}

void aml_phy_switch_port_t3x(u8 port)
{
	u32 data32;

	/* reset and select data port */
	//data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_MISC2, port);
	//data32 &= (~(0xf << 24));
	//data32 |= ((1 << port) << 24);
	//hdmirx_wr_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_MISC2, data32, port);
	//hdmirx_wr_bits_top_common(TOP_PORT_SEL, MSK(4, 0), (1 << port));
	data32 = 0;
	if (rx_is_pip_on() && port == rx_info.sub_port) {
		data32 |= (1 << (8 + rx_info.sub_port * 2));
		data32 |= (1 << rx_info.sub_port);
	}
	data32 |= (2 << (8 + rx_info.main_port * 2));
	data32 |= (1 << (rx_info.main_port + 4));
	hdmirx_wr_top_common(HDMIRX_TOP_FSW_CNTL, data32);

	data32 = 0;
	data32 |= (1 << (4 + rx_info.main_port));
	if (rx_is_pip_on() && port == rx_info.sub_port)
		data32 |= (1 << (4 + rx_info.sub_port));
	data32 |= 3;
	hdmirx_wr_top_common(HDMIRX_TOP_FSW_CLK_CNTL, data32);

	hdmirx_wr_top_common(HDMIRX_TOP_SW_RESET_COMMON, 0x19a);//todo
	hdmirx_wr_top_common(HDMIRX_TOP_SW_RESET_COMMON, 0);

}

unsigned int rx_sec_hdcp_cfg_t3x(void)
{
	struct arm_smccc_res res;

	arm_smccc_smc(HDMI_RX_HDCP_CFG, 0, 0, 0, 0, 0, 0, 0, &res);

	return (unsigned int)((res.a0) & 0xffffffff);
}

/*
 * 0 SPDIF
 * 1  I2S
 * 2  TDM
 */
void rx_set_aud_output_t3x(u32 param, u8 port)
{
	if (param == 2) {
		hdmirx_wr_cor(RX_TDM_CTRL1_AUD_IVCRX, 0x0f, port);
		hdmirx_wr_cor(RX_TDM_CTRL2_AUD_IVCRX, 0xff, port);
		hdmirx_wr_cor(AAC_MCLK_SEL_AUD_IVCRX, 0x90, port); //TDM
	} else if (param == 1) {
		hdmirx_wr_cor(RX_TDM_CTRL1_AUD_IVCRX, 0x00, port);
		hdmirx_wr_cor(RX_TDM_CTRL2_AUD_IVCRX, 0x10, port);
		hdmirx_wr_cor(AAC_MCLK_SEL_AUD_IVCRX, 0x80, port); //I2S
		/* bit'15 1-spdif, 0- hbr */
		hdmirx_wr_bits_top(TOP_CLK_CNTL, _BIT(15), 0, port);
		hdmirx_wr_bits_top_common_1(TOP_ACR_CNTL2_T3X, _BIT(5), port);
	} else {
		hdmirx_wr_cor(RX_TDM_CTRL1_AUD_IVCRX, 0x00, port);
		hdmirx_wr_cor(RX_TDM_CTRL2_AUD_IVCRX, 0x10, port);
		hdmirx_wr_cor(AAC_MCLK_SEL_AUD_IVCRX, 0x80, port); //SPDIF
		//hdmirx_wr_bits_top(TOP_CLK_CNTL, _BIT(15), 1);
		hdmirx_wr_bits_top(TOP_CLK_CNTL, _BIT(4), 0, port);
	}
}

void rx_sw_reset_t3x(int level, u8 port)
{
	/* deep color fifo */
	hdmirx_wr_bits_cor(RX_PWD_SRST_PWD_IVCRX, _BIT(4), 1, port);
	udelay(1);
	hdmirx_wr_bits_cor(RX_PWD_SRST_PWD_IVCRX, _BIT(4), 0, port);
	//TODO..
}

void hdcp_init_t3x(u8 port)
{
	u8 data8;
	//key config and crc check
	//rx_sec_hdcp_cfg_t3x();
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
	data8 |= (hdmirx_repeat_support() &&
		rx[port].hdcp.repeat) << 1;
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

//void reset_pcs(void)
//{
	//hdmirx_wr_top(TOP_SW_RESET, 0x80);
	//hdmirx_wr_top(TOP_SW_RESET, 0);
//}

/* FRL training */
//scdc:
//0x31: [7:4] FFE_LEVELS; [3:0] FRL_rate
//FRL_Rate = 0: disable FRL
//           1: 3G/3Lanes
//           2: 6G/3Lanes
//           3: 6G/4Lanes
//           4: 8G/4Lanes
//           5: 10G/4Lanes
//           6: 12G/4Lanes
//
#define FRL_RATE_3G_3LANES 1
#define FRL_RATE_6G_3LANES 2
#define FRL_RATE_6G_4LANES 3
#define FRL_RATE_8G_4LANES 4
#define FRL_RATE_10G_4LANES 5
#define FRL_RATE_12G_4LANES 6

//================== LTS 2============================
void rx_lts_2_flt_ready(u8 port)
{
	u8 data8;

	//inv_hal_rx_lock_enable(); for config PHY
	//??? only has SR_DP_CTL3, no SR_CP_CTL0
	//data8  = 0;
	//TODO hdmirx_wr_cor(int_ext,SR_DP_CTL0_DPHY_IVCRX,data8, port);

	//data8  = 0;
	//TODO hdmirx_wr_cor(int_ext,SR_CP_CTL0_DPHY_IVCRX,data8, port);

	//-------
	data8  = 0;
	data8 |= (1 << 4); //reg_dual_pipe_en
	data8 |= (1 << 2); //reg_acr_div2mode
	data8 |= (0 << 1); //reg_acr_clk_sel
	data8 |= (1 << 0); //reg_tmds_clk_sel  0:tmds_clk ;1: hdmi21_tmds_clk

	//inv_hal_rx_h21_ctrl_write(port,0x00);
	hdmirx_wr_cor(RX_H21_CTRL_PWD_IVCRX, data8, port);

	//Error counter gets enabled only after SR_SYNC is detected by setting GP1 register to 0x6
	//hal_flt_rx_enable_error_counter(port);
	hdmirx_wr_cor(H21RXSB_GP1_REGISTER_M42H_IVCRX, 0x6, port); //

	//-always set BIST restart error counter through th LT
	//hal_flt_rx_bist_counter_ctrl(port,true);
	data8  = 0;
	data8 |= (0 << 7); //reg_en_bist
	data8 |= (0 << 6); //reg_csel
	data8 |= (0 << 5); //reg_scram_bswap
	data8 |= (1 << 4); //reg_resync
	hdmirx_wr_cor(SR_BIST_CTL0_DDPHY_IVCRX, data8, port);

	//--clears error counter and valid bit
	//
	//hal_flt_rx_clear_error_counter(port);
	data8  = 0;
	data8 |= (1    << 7); //ri_rs_clr_ecc_src
	data8 |= (0    << 6); //ri_rs_sel_sync
	data8 |= (0    << 3); //ri_rs_misc_ctrl
	data8 |= (0    << 2); //ri_rs_chk_en
	data8 |= (1    << 0); //ri_rs_err_cnt_sel1
	hdmirx_wr_cor(H21RXSB_CTRL3_M42H_IVCRX, data8, port); //

	//------
	//hal_flt_ready_mod(port,true)
	//---[6] FLT_READY; tell source the sink is ready for link training
	data8 = 0;
	data8 = hdmirx_rd_cor(SCDCS_STATUS_FLAGS0_SCDC_IVCRX, port);
	hdmirx_wr_cor(SCDCS_STATUS_FLAGS0_SCDC_IVCRX, (data8 | 0x40), port);
}

//=============== LTS 3====================

void RX_LTS_3_LTP_REQ_SEND_1111(u8 port)
{
	u8 data8;
	//------ sink clears(=0) FRL_START-----
	//hal_frl_start_mod(port,false);
	hdmirx_wr_cor(SCDCS_UPD_FLAGS_SCDC_IVCRX, 0x0, port);//sink clear(=0) FRL_START
	//==============  s_process_rate_change(p) =============
	//1)s_frl_rate_written(p)
	//
	//---------set LT mode true while link training.
	//2)hal_flt_rx_hdmi2p1_lt_mode_set(p,true);
	//(NONE)
	//---------initialize FFE levels, etc. for a new FRL rate
	//3)hal_flt_ltp_req_write(p->rx_port, 0x11, 0x11)
	//writes the Ln(x)_LTP_req registers for the source
	//
	hdmirx_wr_cor(SCDCS_STATUS_FLAGS1_SCDC_IVCRX, 0x65, port); //
	hdmirx_wr_cor(SCDCS_STATUS_FLAGS2_SCDC_IVCRX, 0x87, port); //

	//4)hal_flt_update_set(p->rx_port)
	//set the RX FLT_update flag
	data8 = hdmirx_rd_cor(SCDCS_UPD_FLAGS_SCDC_IVCRX, port);
	rx_pr("upd flg=0x%x\n", data8);
	hdmirx_wr_cor(SCDCS_UPD_FLAGS_SCDC_IVCRX, (data8 | 0x20), port);//
	rx_pr("upd flg-1=0x%x\n", hdmirx_rd_cor(SCDCS_UPD_FLAGS_SCDC_IVCRX, port));
}

void RX_LTS_3_FRL_CONFIG(u8 frl_rate, u8 port)
{
	u8 data8;

	//allow the PHY state machine to set up the RX PHY PLL
	//
	//----hal_frl_sarah_pll_set(p->rx_port, p->frl_rate)----!!!
	//TODO need config IP0_RX_ZONEVCO
	hdmirx_wr_cor(SR_EQ_CTL0_DPHY_IVCRX, 0x05, port); //TODO ??
	hdmirx_wr_cor(SR_EQ_CTL1_DPHY_IVCRX, 0x05, port); //TODO ??
	hdmirx_wr_cor(SR_CP_CTL0_DPHY_IVCRX, 0x01, port); //TODO ?? [0] CLK_EN

	//disable the sarah pll for programming.
	hdmirx_wr_cor(SR_DP_CTL0_DPHY_IVCRX, 0x00, port); //
	hdmirx_wr_cor(SR_PLL_CTL22_DPHY_IVCRX, 0x40, port); //
	hdmirx_wr_cor(SR_PLL_CTL0_DPHY_IVCRX, 0x13, port); //
	hdmirx_wr_cor(SR_PLL_CTL23_DPHY_IVCRX, 0x61, port); //

	hdmirx_wr_cor(SR_PLL_CTL10_DPHY_IVCRX, 0x00, port); //TODO
	hdmirx_wr_cor(SR_PLL_CTL12_DPHY_IVCRX, 0x00, port); //TODO
	hdmirx_wr_cor(SR_PLL_CTL5_DPHY_IVCRX, 0x00, port); //TODO
	hdmirx_wr_cor(SR_PLL_CTL8_DPHY_IVCRX, 0x00, port); //TODO
	//new for ES1.0 chips
	hdmirx_wr_cor(SR_PLL_CTL9_DPHY_IVCRX, 0x3b, port);
	hdmirx_wr_cor(SR_PLL_CTL13_DPHY_IVCRX, 0x03, port);
	hdmirx_wr_cor(SR_PLL_CTL3_DPHY_IVCRX, 0x01, port);
	//enable the sarah pll
	hdmirx_wr_cor(SR_DP_CTL0_DPHY_IVCRX, 0x9f, port);

	data8 = hdmirx_rd_cor(SR_CP_CTL0_DPHY_IVCRX, port);
	hdmirx_wr_cor(SR_CP_CTL0_DPHY_IVCRX, (data8 | 0x01), port);
	//restore defaults

	hdmirx_wr_cor(SR_DLL0_CTL0_DPHY_IVCRX, 0xf3, port);
	hdmirx_wr_cor(SR_DLL0_CTL1_DPHY_IVCRX, 0x00, port);
	hdmirx_wr_cor(SR_DLL0_CTL2_DPHY_IVCRX, 0x00, port);
	hdmirx_wr_cor(SR_DLL1_CTL0_DPHY_IVCRX, 0xf3, port);
	hdmirx_wr_cor(SR_DLL1_CTL1_DPHY_IVCRX, 0x00, port);
	hdmirx_wr_cor(SR_DLL1_CTL2_DPHY_IVCRX, 0x00, port);
	hdmirx_wr_cor(SR_DLL2_CTL0_DPHY_IVCRX, 0xf3, port);
	hdmirx_wr_cor(SR_DLL2_CTL1_DPHY_IVCRX, 0x00, port);
	hdmirx_wr_cor(SR_DLL2_CTL2_DPHY_IVCRX, 0x00, port);
	hdmirx_wr_cor(SR_DLL3_CTL0_DPHY_IVCRX, 0xf3, port);
	hdmirx_wr_cor(SR_DLL3_CTL1_DPHY_IVCRX, 0x00, port);
	hdmirx_wr_cor(SR_DLL3_CTL2_DPHY_IVCRX, 0x00, port);

	hdmirx_wr_cor(SR_CDR0_CTL0_DPHY_IVCRX, 0x00, port);
	hdmirx_wr_cor(SR_CDR1_CTL0_DPHY_IVCRX, 0x00, port);
	hdmirx_wr_cor(SR_CDR2_CTL0_DPHY_IVCRX, 0x00, port);
	hdmirx_wr_cor(SR_CDR3_CTL0_DPHY_IVCRX, 0x00, port);
	hdmirx_wr_cor(SR_CDR_CTL1_DPHY_IVCRX, 0x42, port);
	//enable 2nd CDR and if over 3G, voter mode

	//data8 = (frl_f_rate == FRL_RATE_3G_3LANES) ? 0x24 : 0x34;
	hdmirx_wr_cor(SR_CDR_CTL0_DPHY_IVCRX, 0x00, port);
	//[7:6] manual enable cdr due to code bug
	hdmirx_wr_cor(SR_CDR_CTL3_DPHY_IVCRX, 0xc0, port);
	//manual mode wait for lock detect
	hdmirx_wr_cor(SR_LCDT_CTL3_DDPHY_IVCRX, 0x47, port); //

	//=======hal_frl_sarah_eq_settings(p->rx_port,eq_index);=====
	hdmirx_wr_cor(SARAH_EQ_SET0_DPHY_IVCRX, 0x00, port); //TODO
	hdmirx_wr_cor(SARAH_EQ_SET8_DPHY_IVCRX, 0x00, port); //TODO

	//=======hal_frl_manual_offset_cal(p->rx_port,p->frl_rate)====
	//run calibration and LT with cpath disabled.
	hdmirx_wr_cor(SR_CP_CTL0_DPHY_IVCRX, 0x60, port);
	hdmirx_wr_cor(SR_CP_CTL1_DPHY_IVCRX, 0x98, port);
	//short EQ inputs to allow accurate offset calibration
	hdmirx_wr_cor(SR_EQ_CTL1_DPHY_IVCRX, 0xF0, port);
	//run os adap
	//=====s_run_cal_adap();========
	//enable calibration adaptor
	hdmirx_wr_cor(SR_ADAP_CTL25_DPHY_IVCRX, 0x01, port);
	hdmirx_wr_cor(SR_ADAP_CTL16_DPHY_IVCRX, 0x80, port);
	hdmirx_wr_cor(SR_ADAP_CTL11_DPHY_IVCRX, 0x0A, port);
	hdmirx_wr_cor(SR_ADAP_CTL1_DPHY_IVCRX, 0x30, port);
	//trigger calibration
	hdmirx_wr_cor(SR_ADAP_CTL0_DPHY_IVCRX, 0x60, port);
	hdmirx_wr_cor(SR_ADAP_CTL0_DPHY_IVCRX, 0x20, port);

	//delay for adap to complete
	usleep_range(2000, 2100);
	//disable calibration adaptor
	hdmirx_wr_cor(SR_ADAP_CTL25_DPHY_IVCRX, 0x00, port);
	//=============================================================

	//restore normal input to equalizer
	hdmirx_wr_cor(SR_EQ_CTL1_DPHY_IVCRX, 0x00, port); //TODO
}

void RX_LTS_3_LTP_REQ_SEND_5678(u8 port)
{
	u8 data8;
	//restore original LPT patterns and wait for source
	//hal_flt_ltp_req_write()
	//[3:0]  ln0_ltp_req [7:4] ln1_ltp_req
	hdmirx_wr_cor(SCDCS_STATUS_FLAGS1_SCDC_IVCRX, 0x65, port);
	//[3:0]  ln2_ltp_req [7:4] ln3_ltp_req
	hdmirx_wr_cor(SCDCS_STATUS_FLAGS2_SCDC_IVCRX, 0x87, port);

	//hal_flt_update_set();
	data8 = hdmirx_rd_cor(SCDCS_UPD_FLAGS_SCDC_IVCRX, port);
	hdmirx_wr_cor(SCDCS_UPD_FLAGS_SCDC_IVCRX, (data8 | 0x20), port);//
}

void RX_WAIT_FLT_UPDATE_CLEAR(u8 port)
{
	u8 data8;

	//rx_pr("before poll,flg=0x%x\n", hdmirx_rd_cor(SCDCS_UPD_FLAGS_SCDC_IVCRX, port));
	hdmirx_poll_cor(SCDCS_UPD_FLAGS_SCDC_IVCRX, 0 << 5, 0xdf, frl_sync_cnt, port); //flt_update
	//rx_pr("after poll,flg=0x%x\n", hdmirx_rd_cor(SCDCS_UPD_FLAGS_SCDC_IVCRX, port));

	data8 = hdmirx_rd_cor(SCDCS_UPD_FLAGS_SCDC_IVCRX, port);
	//rx_pr("upd flg=0x%x\n", data8);
	hdmirx_wr_cor(SCDCS_UPD_FLAGS_SCDC_IVCRX, (data8 | 0x20), port);//
}

void rx_lts_3_err_detect(u8 port)
{
	u8 bist0_err_cnt_0;
	u8 bist0_err_cnt_1;
	u8 bist0_err_cnt_2;
	u8 bist1_err_cnt_0;
	u8 bist1_err_cnt_1;
	u8 bist1_err_cnt_2;
	u8 bist2_err_cnt_0;
	u8 bist2_err_cnt_1;
	u8 bist2_err_cnt_2;
	u8 bist3_err_cnt_0;
	u8 bist3_err_cnt_1;
	u8 bist3_err_cnt_2;
	u8 bist0_err_cnt;
	u8 bist1_err_cnt;
	u8 bist2_err_cnt;
	u8 bist3_err_cnt;
	u8 data8;
	u8 frl_rate_sel;

	//-------hal_frl_eq_update(port)
	//s_run_cal_adap(port,0x01,0x02)//TODO
	//
	//reset BIST error counter and restart BIST
	hdmirx_wr_cor(SR_BIST_CTL0_DDPHY_IVCRX, 0x90, port);
	hdmirx_wr_cor(SR_BIST_CTL2_DDPHY_IVCRX, 0x00, port);
	hdmirx_wr_cor(SR_BIST_CTL3_DDPHY_IVCRX, 0x00, port);
	//frl_debug,dont need to config digital phy
	//hdmirx_wr_cor(SR_SCDC_CTL1_DDPHY_IVCRX, 0x65, port);
	//hdmirx_wr_cor(SR_SCDC_CTL2_DDPHY_IVCRX, 0x87, port);
	hdmirx_wr_cor(SR_BIST_CTL0_DDPHY_IVCRX, 0x80, port);

	//data8  =  0;
	//data8 |= (0 << 0); //reg_LTS_Cmd[3:0]
	//data8 |= (0 << 4); //reg_LTS_req
	//data8 |= (0 << 5); //reg_SCDC_sel
	//hdmirx_wr_cor(SR_SCDC_CTL0_DDPHY_IVCRX, data8, port);
	//==============

	//-------hal_flt_rx_bist_counter_ctrl()
	//data8 =  hdmirx_rd_COR(int_ext,SR_BIST_CTL0_DDPHY_IVCRX,a_b_sel );
	//hdmirx_wr_cor(int_ext,SR_BIST_CTL0_DDPHY_IVCRX,(data8 | 0x10),a_b_sel, port);

	//-------s_frl_check_eq_err(p)
	//
	//collect bist error count for each lane
	//channel 0
	hdmirx_poll_cor(SARAH_BIST_ST_2_DDPHY_IVCRX, 0 << 7, 0x7f, frl_sync_cnt, port); //sync_done
	bist0_err_cnt_0 = hdmirx_rd_cor(SARAH_BIST_ST_0_DDPHY_IVCRX, port);
	bist0_err_cnt_1 = hdmirx_rd_cor(SARAH_BIST_ST_1_DDPHY_IVCRX, port);
	bist0_err_cnt_2 = hdmirx_rd_cor(SARAH_BIST_ST_2_DDPHY_IVCRX, port);
	if (log_level & FRL_LOG)
		rx_pr("bist0:0x%x-0x%x-0x%x\n", bist0_err_cnt_0, bist0_err_cnt_1, bist0_err_cnt_2);
	bist0_err_cnt = ((bist0_err_cnt_2 & 0x7f) << 16) |
					 (bist0_err_cnt_1  << 8) | bist0_err_cnt_0;
	if (bist0_err_cnt > 0) {
		if (log_level & FRL_LOG)
			rx_pr("[FRL ERROR] **************Bist0 ERROR************\n");
	} else {
		if (log_level & FRL_LOG)
			rx_pr("[FRL ERROR] **************Bist0 PASS************\n");
	}
	//channel 1
	hdmirx_poll_cor(SARAH_BIST1_ST_2_DDPHY_IVCRX, 0 << 7, 0x7f, frl_sync_cnt, port); //sync_done
	//udelay(500);

	bist1_err_cnt_0 = hdmirx_rd_cor(SARAH_BIST1_ST_0_DDPHY_IVCRX, port);
	bist1_err_cnt_1 = hdmirx_rd_cor(SARAH_BIST1_ST_1_DDPHY_IVCRX, port);
	bist1_err_cnt_2 = hdmirx_rd_cor(SARAH_BIST1_ST_2_DDPHY_IVCRX, port);
	if (log_level & FRL_LOG)
		rx_pr("bist1:0x%x-0x%x-0x%x\n", bist1_err_cnt_0, bist1_err_cnt_1, bist1_err_cnt_2);
	bist1_err_cnt = ((bist1_err_cnt_2 & 0x7f) << 16) |
					 (bist1_err_cnt_1 << 8) | bist1_err_cnt_0;
	if (bist1_err_cnt > 0) {
		if (log_level & FRL_LOG)
			rx_pr("[FRL ERROR] **********Bist1 ERROR************\n");
	} else {
		if (log_level & FRL_LOG)
			rx_pr("[FRL ERROR] **************Bist1 PASS************\n");
	}
	//channel 2
	hdmirx_poll_cor(SARAH_BIST2_ST_2_DDPHY_IVCRX, 0 << 7, 0x7f, frl_sync_cnt, port); //sync_done
	//udelay(500);

	bist2_err_cnt_0 = hdmirx_rd_cor(SARAH_BIST2_ST_0_DDPHY_IVCRX, port);
	bist2_err_cnt_1 = hdmirx_rd_cor(SARAH_BIST2_ST_1_DDPHY_IVCRX, port);
	bist2_err_cnt_2 = hdmirx_rd_cor(SARAH_BIST2_ST_2_DDPHY_IVCRX, port);
	if (log_level & FRL_LOG)
		rx_pr("bist2:0x%x-0x%x-0x%x\n", bist2_err_cnt_0, bist2_err_cnt_1, bist2_err_cnt_2);
	bist2_err_cnt = ((bist2_err_cnt_2  &  0x7f) << 16) |
					 (bist2_err_cnt_1 << 8) | bist2_err_cnt_0;
	if (bist2_err_cnt > 0) {
		if (log_level & FRL_LOG)
			rx_pr("[FRL ERROR] **************Bist2 ERROR************\n");
	} else {
		if (log_level & FRL_LOG)
			rx_pr("[FRL ERROR] **************Bist2 PASS************\n");
	}
	if (rx[port].var.frl_rate > FRL_RATE_6G_4LANES) {
		//channel 3
		hdmirx_poll_cor(SARAH_BIST3_ST_2_DDPHY_IVCRX, 0 << 7,
			0x7f, frl_sync_cnt, port); //sync_done
		//udelay(500);

		bist3_err_cnt_0 = hdmirx_rd_cor(SARAH_BIST3_ST_0_DDPHY_IVCRX, port);
		bist3_err_cnt_1 = hdmirx_rd_cor(SARAH_BIST3_ST_1_DDPHY_IVCRX, port);
		bist3_err_cnt_2 = hdmirx_rd_cor(SARAH_BIST3_ST_2_DDPHY_IVCRX, port);
		if (log_level & FRL_LOG)
			rx_pr("bist3:0x%x-0x%x-0x%x\n", bist3_err_cnt_0,
				bist3_err_cnt_1, bist3_err_cnt_2);
		bist3_err_cnt = ((bist3_err_cnt_2 & 0x7f) << 16) |
						 (bist3_err_cnt_1 << 8) | bist3_err_cnt_0;
		if (bist3_err_cnt > 0) {
			if (log_level & FRL_LOG)
				rx_pr("[FRL ERROR] **************Bist3 ERROR************\n");
		} else {
			if (log_level & FRL_LOG)
				rx_pr("[FRL ERROR] **************Bist3 PASS************\n");
		}
	}
	if (log_level & FRL_LOG)
		rx_pr("[FRL TRAINING] **%s end************\n", __func__);
	//enable Rx hdmi21 module before Tx
	frl_rate_sel = (rx[port].var.frl_rate > FRL_RATE_6G_3LANES) ? 1 : 0;

	data8  = 0;
	data8 |= (0               << 6); //reg_debug_ctl
	data8 |= (0               << 5); //reg_filter_en
	//data8 |= (1               << 4); //reg_scramble_en
	data8 |= (frl_scrambler_en    << 4); //reg_scramble_en
	data8 |= (0               << 2); //reg_scdt_reset_mask
	data8 |= (frl_rate_sel    << 1); //reg_lane_sel_mode: 0-3Lane; 1:4Lane
	data8 |= (1               << 0); //reg_en
	hdmirx_wr_cor(H21RXSB_CTRL_M42H_IVCRX, data8, port); //
}

void RX_LTS_3_LTP_REQ_SEND_0000(u8 port)
{
	//u8 data8;
	//================== done =============
	//if ltp_0/1/2/3 ==0 then training done; let the source know we're done training
	//hal_flt_ltp_req_write(port,FLT_CODE_NO_LTP,FLT_CODE_NO_LTP)
	//hal_flt_ltp_req_write()
	//[3:0]  ln0_ltp_req [7:4] ln1_ltp_req
	hdmirx_wr_cor(SCDCS_STATUS_FLAGS1_SCDC_IVCRX, 0x00, port);
	//[3:0]  ln2_ltp_req [7:4] ln3_ltp_req
	hdmirx_wr_cor(SCDCS_STATUS_FLAGS2_SCDC_IVCRX, 0x00, port);
	//data8 = hdmirx_rd_cor(SCDCS_UPD_FLAGS_SCDC_IVCRX, port);
	//rx_pr("upd flg=0x%x\n", data8);
	//hdmirx_wr_cor(SCDCS_UPD_FLAGS_SCDC_IVCRX, (data8 | 0x20), port);//
}

//======================= LTS P ========================
int rx_lts_p_syn_detect(u8 frl_rate, u8 port)
{
	u8  frl_transmission_detected = 0;
	u8  channel_lock;
	u8  channel_lock_shift;
	u8  lane_count;
	u8  frl_rate_sel;
	u8  data8;
	int ret = true;
	u32 i = 0;

	// P state summary: FRL training has passed
	//TODO inv_hal_rx_sarah_phy_clk_gen(port)

	//======start super block decoder ====
	//-----hal_frl_rx_enable_sb(port, true, p->frl_rate);
	//enable RS error counters

	frl_rate_sel = (frl_rate > FRL_RATE_6G_3LANES) ? 1 : 0;
	//frl_debug
	//hdmirx_wr_cor(H21RXSB_NMUL_M42H_IVCRX, odn_reg_n_mul, port);
	data8  = 0;
	data8 |= (0               << 6); //reg_debug_ctl
	data8 |= (0               << 5); //reg_filter_en
	//data8 |= (1               << 4); //reg_scramble_en
	data8 |= (frl_scrambler_en    << 4); //reg_scramble_en
	data8 |= (0               << 2); //reg_scdt_reset_mask
	data8 |= (frl_rate_sel    << 1); //reg_lane_sel_mode: 0-3Lane; 1:4Lane
	data8 |= (1               << 0); //reg_en
	hdmirx_wr_cor(H21RXSB_CTRL_M42H_IVCRX, data8, port);

	//-----hal_frl_enable_cpath(port)
	//use to set CPATH after link training
	//frl_debug,dont need config digital phy
	//hdmirx_wr_cor(SR_CP_CTL0_DPHY_IVCRX, 0x43, port);
	//hdmirx_wr_cor(SR_CP_CTL1_DPHY_IVCRX, 0x8E, port);

	//wait here to get lock on all lanes!!!

	//[0]clk_detected;[1]ch0_locked;[2]ch1_locked;[3]ch2_locked;[4]ln3_locked
	lane_count = (frl_rate == 0) ? 0 : (frl_rate <= FRL_RATE_6G_3LANES) ? 3 : 4;
	if (log_level & FRL_LOG)
		rx_pr("[FRL TRAINING] **lane_count = %x**\n", lane_count);

	while (frl_transmission_detected == 0) {
		channel_lock = hdmirx_rd_cor(SCDCS_STATUS_FLAGS0_SCDC_IVCRX, port);
	    channel_lock_shift = (channel_lock >> 1) & 0xf;
		//if (log_level & FRL_LOG)
			//rx_pr("[FRL TRAINING] *channel_lock_shift = %x*\n", channel_lock_shift);

		if (lane_count == 3) {
			if (channel_lock_shift == 7)
				frl_transmission_detected = 1;
		} else if (lane_count == 4) {
			if (channel_lock_shift == 15)
				frl_transmission_detected = 1;
		}
		udelay(5);
		if (i++ > ext_cnt)
			break;
	}
	if (log_level & FRL_LOG)
		rx_pr("[FRL TRAINING] *channel_lock_shift = %x,i=%d*\n", channel_lock_shift, i);
	//======start super block decoder ====
	//-----hal_frl_rx_enable_sb(port, true, p->frl_rate);
	//enable RS error counters
	frl_rate_sel = (frl_rate > FRL_RATE_6G_3LANES) ? 1 : 0;

	data8  = 0;
	data8 |= (0 << 6); //reg_debug_ctl
	data8 |= (0 << 5); //reg_filter_en
	data8 |= (frl_scrambler_en << 4); //reg_scramble_en
	data8 |= (0 << 2); //reg_scdt_reset_mask
	data8 |= (frl_rate_sel << 1); //reg_lane_sel_mode: 0-3Lane; 1:4Lane
	data8 |= (1 << 0); //reg_en
	hdmirx_wr_cor(H21RXSB_CTRL_M42H_IVCRX, data8, port);

	//======= clear Link Training mode =========
	//----hal_flt_rx_clear_rscc(port)
	//wait at least 100ms for SR_SYNC to be enabled

	if (!hdmirx_poll_cor(H21RXSB_STATUS_M42H_IVCRX, 1 << 2, 0xfb, frl_sync_cnt, port)) //sb sync
		ret = false;
	if (log_level & FRL_LOG)
		rx_pr("[FRL TRAINING] Polling sb_sync Done ************\n");

	if (!hdmirx_poll_cor(H21RXSB_STATUS_M42H_IVCRX, 1 << 3, 0xf7, frl_sync_cnt, port)) //sr sync
		ret = false;
	if (log_level & FRL_LOG)
		rx_pr("[FRL TRAINING] Polling sr_sync Done ************\n");

	hdmirx_wr_cor(H21RXSB_CTRL3_M42H_IVCRX, 0x81, port); //
	hdmirx_wr_cor(H21RXSB_CTRL3_M42H_IVCRX, 0x09, port); //
	//
	//----hal_flt_rx_dpll_reset_toggle(port)
	hdmirx_wr_bits_cor(DPLL_CTRL2_DPLL_IVCRX, _BIT(1), 1, port);
	hdmirx_wr_bits_cor(DPLL_CTRL2_DPLL_IVCRX, _BIT(1), 1, port);
	//
	//----hal_flt_rx_hdmi2p1_lt_mode_set(port,false)
	//(NONE)
	//
	//----hal_flt_rx_bist_counter_ctrl(port,true)
	//clear the BIST error counters
	hdmirx_wr_cor(SR_BIST_CTL0_DDPHY_IVCRX, 0x10, port); //
	if (log_level & FRL_LOG)
		rx_pr("[FRL TRAINING] *%s End*\n", __func__);
	return ret;
}

void RX_LTS_P_FRL_START(u8 port)
{
	u8 data8;
	//====== tell source to send video =======
	//----hal_frl_start_mod(port,true)!!!!!!!!!!!!!!

	data8 = hdmirx_rd_cor(SCDCS_UPD_FLAGS_SCDC_IVCRX, port);
	hdmirx_wr_cor(SCDCS_UPD_FLAGS_SCDC_IVCRX, (data8 | 0x10), port);//frl_start
	force_clk_stable = 1;
	rx_set_frl_train_sts(E_FRL_TRAIN_FINISH, port);
}

enum frl_rate_e hdmirx_get_frl_rate(u8 port)
{
	if (rx[port].var.frl_rate > 0xf)
		rx[port].var.frl_rate &= 0xf;
	return rx[port].var.frl_rate;
}

bool hal_rx_tmds_channels_locked_query(u8 port)
{
	u8 lock;

	lock = hdmirx_rd_cor(SCDCS_STATUS_FLAGS0_SCDC_IVCRX, port);

	rx_pr("TMDS Lock: %02X", (u16)lock);
	return (lock & 0xf) == 0xf;
}

bool s_tmds_transmission_detected(u8 port)
{
	u8 frl_rate;
	bool b_detected;

	frl_rate = hdmirx_rd_cor(SCDCS_CONFIG1_SCDC_IVCRX, port) & 0xf;
	b_detected = false;
	if (frl_rate == 0) {
		if (hal_rx_tmds_channels_locked_query(port))
			b_detected = true;
	}
	if (b_detected)
		rx_pr("tmds detect\n");
	return b_detected;
}

void hal_flt_update_set(u8 port)
{
	u8 data8;

	data8 = hdmirx_rd_cor(SCDCS_UPD_FLAGS_SCDC_IVCRX, port);
	hdmirx_wr_cor(SCDCS_UPD_FLAGS_SCDC_IVCRX, (data8 | 0x20), port);
}

void hdmi_tx_rx_frl_training_main(u8 port)
{
	rx[port].var.frl_rate = hdmirx_rd_cor(SCDCS_CONFIG1_SCDC_IVCRX, port) & 0xf;
	//hdmirx_wr_bits_cor(SCDCS_SRC_TEST_CONFIG_SCDC_IVCRX, _BIT(5), 1, port);
	aml_phy_init_t3x(port);
	hdmirx_wr_cor(SCDCS_UPD_FLAGS_SCDC_IVCRX, 0x0, port);//sink clear(=0) FRL_START
	hdmirx_wr_cor(SCDCS_STATUS_FLAGS1_SCDC_IVCRX, 0x65, port); //
	hdmirx_wr_cor(SCDCS_STATUS_FLAGS2_SCDC_IVCRX, 0x87, port); //
	hal_flt_update_set(port);
	if (!hdmirx_flt_update_cleared_wait(SCDCS_UPD_FLAGS_SCDC_IVCRX, port)) {
		if (log_level & FRL_LOG)
			rx_pr("polling time out a\n");
	}
	aml_phy_init_t3x(port);
	mdelay(20);
	hdmirx_wr_top(TOP_SW_RESET, 0x80, port);
	hdmirx_wr_top(TOP_SW_RESET, 0, port);
	hdmirx_wr_bits_cor(DPLL_CTRL2_DPLL_IVCRX, _BIT(1), 0, port);
	hdmirx_wr_bits_cor(DPLL_CTRL2_DPLL_IVCRX, _BIT(1), 1, port);
	rx_lts_3_err_detect(port);
	RX_LTS_3_LTP_REQ_SEND_0000(port);
	hal_flt_update_set(port);
	if (!hdmirx_flt_update_cleared_wait(SCDCS_UPD_FLAGS_SCDC_IVCRX, port)) {
		if (log_level & FRL_LOG)
			rx_pr("polling time out b\n");
	}
	rx_set_frl_train_sts(E_FRL_TRAIN_FINISH, port);
}

enum frl_train_sts_e rx_get_frl_train_sts(u8 port)
{
	if (port == E_PORT2)
		return frl_train_sts;
	else
		return frl_train_sts1;
}

void rx_set_frl_train_sts(enum frl_train_sts_e sts, u8 port)
{
	if (port == E_PORT2)
		frl_train_sts = sts;
	else
		frl_train_sts1 = sts;
}

bool is_frl_train_finished(u8 port)
{
	bool ret = false;

	if (rx_get_frl_train_sts(port) == E_FRL_TRAIN_FINISH)
		ret = true;
	return ret;
}

void rx_i2c_dbg_monitor(void)
{
	u32 data32;

	if (rx_info.chip_id != CHIP_ID_T3X)
		return;
	data32 = rd_reg_clk_ctl(T3X_I2C_MONITOR_INTR_STATUS);

	if (data32 > 3) { //ignore start abnormal
		wr_reg_clk_ctl(T3X_I2C_MONITOR_SMP_START, 0x0);
		rx_i2c_dump();
		wr_reg_clk_ctl(T3X_I2C_MONITOR_INTR_STATUS, data32);
	}
}

static void rx_parse_i2c_data(u8 *buf, int size)
{
	static const char * const ack_print[] = {"ACK", "NAK"};
	static const char * const wr_print[] = {"Write", "Read"};
	u8 type, data, ack, wr;
	int i;

	if (!buf || size < 4)
		return;
	for (i = 0; i < size; i += 4) {
		/*
		 * |31..29|28.......9|8..1|  0|
		 * |type  |delta time|data|ack|
		 */
		if (!buf[i] && !buf[i + 1] && !buf[i + 2] && !buf[i + 3])
			break;
		type = (buf[i + 3] & MSK(3, 5)) >> 5;
		data = (buf[i] & MSK(7, 1)) >> 1 | (buf[i + 1] & 0x1) << 7;
		ack = buf[i] & 0x1;
		wr = data & 0x1;

		switch (type) {
		case E_DATA:
			rx_pr("0x%x + %s\n", data, ack_print[ack]);
			break;
		case E_START_DATA:
			rx_pr("%s to [0x%x] + %s\n", wr_print[wr], data, ack_print[ack]);
			break;
		case E_STOP_DATA:
			rx_pr("0x%x + %s + Stop\n", data, ack_print[ack]);
			break;
		case E_START_DATA_STOP:
			rx_pr("%s to [0x%x] + %s + Stop\n", wr_print[wr], data, ack_print[ack]);
			break;
		case E_STOP_ABNORMAL:
			rx_pr("!!STOP Abnormal(incomplete data)\n");
			break;
		case E_START_ABNORMAL:
			if (ack) {
				rx_pr("!!Start Abnormal: %s to [0x%x] + %s\n",
					wr_print[wr], data, ack_print[ack]);
			} else {
				rx_pr("!!Start Abnormal(incomplete data)\n");
			}
			break;
		case E_TIME_OUT:
			if (ack)
				rx_pr("!!time out(no data)\n");
			else
				rx_pr("!!time out + 0x%x\n", data);
			break;
		default:
			if ((data >> 5) == 0) {
				if (ack)
					rx_pr("HPD rise\n");
				else
					rx_pr("HPD fall\n");
			} else {
				rx_pr("!!other type:0x%x\n", data);
			}
			break;
		}
	}
	rx_pr("i:0x%x\n", i);
}

void rx_i2c_monitor(u8 sel, u8 smp_mod, u8 trig_mod, u8 dump_mod)
{
	u8 *src = NULL;
	u32 data32;

	data32 = 0;
	data32 |= (smp_mod == 3 ? 1 : 0) << 31; //[31]  bist_mode  exist bug, can't use
	data32 |= (dump_mod & 0x1f) << 16; //[20-16]dump mode
	data32 |= 0x0 << 4; //[4]   clk_free
	data32 |= (smp_mod & 0x3) << 2; //[3:2] smp_mode
	data32 |= 0x1 << 1; //[1]   hpd_en
	data32 |= (trig_mod & 0x1) << 0; //[0]   trig_mode
	wr_reg_clk_ctl(T3X_I2C_MONITOR_SMP_CNTL, data32);

	data32 = 0;
	data32 |= (sel >= 6 ? 1 << (sel - 6) : 0) << 24; //[31:24] sel input hpd to monitor
	data32 |= (1 << sel) <<  0; //[23:0]  sel input scl to monitor
	wr_reg_clk_ctl(T3X_I2C_MONITOR_SMP_SEL, data32);

	data32 = 0;
	data32 |= 0x2008 << 16; //[31:16] scl filter control
	data32 |= 0x2008 << 0;  //[15: 0] sda filter control
	wr_reg_clk_ctl(T3X_I2C_MONITOR_SMP_FLT, data32);

	data32 = 0;
	data32 |= 0x7 << 0; //[15: 0] sda filter control
	wr_reg_clk_ctl(T3X_I2C_MONITOR_SMP_FLT_HPD, data32);

	data32 = 0;
	data32 |= 0xa << 16; //[31:16] time tick in smp_clk
	data32 |= 0x1 << 8; //[    8] smp_clk enable
	data32 |= 0x5 << 0; //[ 7: 0] smp_clk div, cec sample:0x48
	wr_reg_clk_ctl(T3X_I2C_MONITOR_SMP_CLK, data32);

	data32 = 0;
	data32 |= 0x3ff << 0; //[10:0]
	wr_reg_clk_ctl(T3X_I2C_MONITOR_INTR_MASK, data32);

	rx_info.i2c_buff.pg_addr = alloc_pages(GFP_KERNEL, 0);
	src = (u8 *)kmap_atomic(rx_info.i2c_buff.pg_addr);
	memset(src, 0x0, I2C_BUFF_SIZE);
	kunmap_atomic(src);
	rx_info.i2c_buff.phy_addr = page_to_phys(rx_info.i2c_buff.pg_addr);

	if (rx_info.i2c_buff.phy_addr) {
		wr_reg_clk_ctl(T3X_I2C_MONITOR_DDR_START_ADDR,
			rx_info.i2c_buff.phy_addr >> 4);
		wr_reg_clk_ctl(T3X_I2C_MONITOR_DDR_END_ADDR,
			(rx_info.i2c_buff.phy_addr + I2C_BUFF_SIZE) >> 4);
	}

	data32 = 0;
	data32 |= 0x1 << 4; //[  4] endian in 32 bit
	data32 |= 0x0 << 0; //[3:0] burst length
	wr_reg_clk_ctl(T3X_I2C_MONITOR_DDR_CNTL, data32);

	//smp start,config this bit after all the other reg has been ready
	wr_reg_clk_ctl(T3X_I2C_MONITOR_SMP_START, 0x1);
}

static void rx_i2c_reg_dump(void)
{
	rx_pr("T3X_I2C_MONITOR_SMP_CNTL: 0x%x-0x%x\n", T3X_I2C_MONITOR_SMP_CNTL,
		rd_reg_clk_ctl(T3X_I2C_MONITOR_SMP_CNTL));
	rx_pr("T3X_I2C_MONITOR_SMP_SEL: 0x%x-0x%x\n", T3X_I2C_MONITOR_SMP_SEL,
		rd_reg_clk_ctl(T3X_I2C_MONITOR_SMP_SEL));
	rx_pr("T3X_I2C_MONITOR_SMP_FLT: 0x%x-0x%x\n", T3X_I2C_MONITOR_SMP_FLT,
		rd_reg_clk_ctl(T3X_I2C_MONITOR_SMP_FLT));
	rx_pr("T3X_I2C_MONITOR_SMP_FLT_HPD: 0x%x-0x%x\n", T3X_I2C_MONITOR_SMP_FLT_HPD,
		rd_reg_clk_ctl(T3X_I2C_MONITOR_SMP_FLT_HPD));
	rx_pr("T3X_I2C_MONITOR_SMP_CLK: 0x%x-0x%x\n", T3X_I2C_MONITOR_SMP_CLK,
		rd_reg_clk_ctl(T3X_I2C_MONITOR_SMP_CLK));
	rx_pr("T3X_I2C_MONITOR_INTR_MASK: 0x%x-0x%x\n", T3X_I2C_MONITOR_INTR_MASK,
		rd_reg_clk_ctl(T3X_I2C_MONITOR_INTR_MASK));
	rx_pr("T3X_I2C_MONITOR_DDR_START_ADDR: 0x%x-0x%x\n", T3X_I2C_MONITOR_DDR_START_ADDR,
		rd_reg_clk_ctl(T3X_I2C_MONITOR_DDR_START_ADDR));
	rx_pr("T3X_I2C_MONITOR_DDR_END_ADDR: 0x%x-0x%x\n", T3X_I2C_MONITOR_DDR_END_ADDR,
		rd_reg_clk_ctl(T3X_I2C_MONITOR_DDR_END_ADDR));
	rx_pr("T3X_I2C_MONITOR_DDR_CNTL: 0x%x-0x%x\n", T3X_I2C_MONITOR_DDR_CNTL,
		rd_reg_clk_ctl(T3X_I2C_MONITOR_DDR_CNTL));
}

void rx_i2c_dump(void)
{
	//int i, j;
	u8 *i2c_buff = NULL;
	u8 *src_buf = NULL;
	int buf_cnt;

	if (log_level & REG_LOG)
		rx_i2c_reg_dump();

	i2c_buff = kmalloc(I2C_BUFF_SIZE, GFP_KERNEL);
	if (!i2c_buff)
		return;
	memset(i2c_buff, 0, I2C_BUFF_SIZE);
	src_buf = (u8 *)kmap_atomic(rx_info.i2c_buff.pg_addr);
	if (!src_buf) {
		kfree(i2c_buff);
		return;
	}
	memcpy(i2c_buff, src_buf, I2C_BUFF_SIZE);
	/*
	 *for (i = 0; i < 128; i++) {
	 *	for (j = 0; j < 32; j++)
	 *		pr_cont("%02X ", i2c_buff[i * 32 + j]);
	 *	rx_pr("\n");
	 *}
	 */
	kunmap_atomic(src_buf);

	buf_cnt = (rd_reg_clk_ctl(T3X_I2C_MONITOR_DDR_WPTR) -
		rd_reg_clk_ctl(T3X_I2C_MONITOR_DDR_START_ADDR)) << 4;
	rx_pr("buf_cnt:wptr:0x%x-start:0x%x=0x%x\n",
		rd_reg_clk_ctl(T3X_I2C_MONITOR_DDR_WPTR) << 4,
		rd_reg_clk_ctl(T3X_I2C_MONITOR_DDR_START_ADDR) << 4,
		buf_cnt);
	rx_parse_i2c_data(i2c_buff, I2C_BUFF_SIZE);
	kfree(i2c_buff);
}

void rx_frl_train_handler(struct kthread_work *work)
{
	hdmi_tx_rx_frl_training_main(E_PORT2);
}

void rx_frl_train_handler_1(struct kthread_work *work)
{
	hdmi_tx_rx_frl_training_main(E_PORT3);
}

void rx_frl_train(u8 port)
{
	if (port == E_PORT2) {
		kthread_queue_work(&frl_worker, &frl_work);
		rx_set_frl_train_sts(E_FRL_TRAIN_START, port);
	} else {
		kthread_queue_work(&frl1_worker, &frl1_work);
		rx_set_frl_train_sts(E_FRL_TRAIN_START, port);
	}
}

void hdmirx_frl_config(u8 port)
{
	u8 data8;
	u32 data32;
	u32 frl_rate_sel;

	/* new for t3x */
	data32  = 0;
	data32 |= (0 << 31); //[   31] update_man
	data32 |= (0 << 30); //[30] load reverse
	data32 |= (2 << 27); //[29:27] delay cycle of update
	data32 |= (50 << 13); //[26:13] update wide
	data32 |= (200 << 0); //[12:0]  update hold width
	hdmirx_wr_top(TOP_TCR_CNTL, data32, port);

	hdmirx_wr_cor(DPLL_CFG6_DPLL_IVCRX, 0x0, port);
	hdmirx_wr_cor(H21RXSB_D2TH_M42H_IVCRX, 0x20, port);
	//clk ready threshold
	hdmirx_wr_cor(H21RXSB_DIFF1T_M42H_IVCRX, 0x20, port);

	//step adjust
	hdmirx_wr_cor(H21RXSB_STEPF1_M42H_IVCRX, 0x18, port); //fast step 0x1800
	hdmirx_wr_cor(H21RXSB_STEPFL1_IVCRX, 0x18, port);     //slow step 0x1800

	hdmirx_wr_cor(H21RXSB_RST_CTRL_M42H_IVCRX, 0x2, port);
	hdmirx_wr_cor(H21RXSB_CTH2_M42H_IVCRX, 0x60, port);   //clk en th
	hdmirx_wr_cor(H21RXSB_INTR2_MASK_M42H_IVCRX, 0x10, port); //rs error detect

	//hdmirx_wr_cor(H21RXSB_INTR3_MASK_M42H_IVCRX, 0x2, port);
	/* end */
	//used for debug,manual config mode
	hdmirx_wr_cor(SCDCS_CONFIG1_SCDC_IVCRX, rx[port].var.frl_rate, port);
	//============= H21RXSB_CTRL3 =================
	data8  = 0;
	data8 |= (0 << 7);  //[7]   ri_en_clr_ecc_src
	data8 |= (0 << 6);  //[6]   ri_rs_sel_sync
	data8 |= (1 << 3);  //[5:3] ri_rs_misc_ctrl
	data8 |= (0 << 2);  //[2]   ri_rs_chk_en
	data8 |= (0 << 0);  //[1:0] ri_rs_err_cnt_sel1
	hdmirx_wr_cor(H21RXSB_CTRL3_M42H_IVCRX, data8, port);

	//=============RX H21_CTRL=================
	data8  = 0;
	data8 |= (1 << 4); //reg_dual_pipe_en
	data8 |= (1 << 2); //reg_acr_div2mode
	data8 |= (1 << 1); //reg_acr_clk_sel
	data8 |= (1 << 0); //reg_tmds_clk_sel  0:tmds_clk ;1: hdmi21_tmds_clk
	hdmirx_wr_cor(RX_H21_CTRL_PWD_IVCRX, data8, port);
	//=============H21RXSB_CTRL1=================
	data8  = 0;
	data8 |= (1 << 7);  //[7] pace_clk_en buf fixed
	data8 |= (0 << 2);  //[2] ri_rs_blk_ln_er
	data8 |= (0 << 1);  //[1] ri_read_rs_err
	data8 |= (1 << 0);  //[0] ri_rs_errcnt_en
	hdmirx_wr_cor(H21RXSB_CTRL1_M42H_IVCRX, data8, port);
	//=============H21RXSB_CTRL2=================
	data8  = 0;
	data8 |= (0 << 6);  //[7:6]ri_rs_chk_mode
	data8 |= (0 << 5);  //[5]  ri_accm_err_manu_clr
	data8 |= (1 << 4);  //[4]  ri_mask_sw_reset
	data8 |= (0 << 3);  //[3]  ri_en_dis_cnt_mask
	data8 |= (0 << 2);  //[2]  ri_en_dis_vld_mask
	data8 |= (0 << 1);  //[1]  ri_en_mask
	data8 |= (0 << 0);  //[0]  ri_rs_err_cnt_sel
	hdmirx_wr_cor(H21RXSB_CTRL2_M42H_IVCRX, data8, port);

	//=============H21RXSB_CTRL4=================
	data8  = 0;
	data8 |= (0 << 4);  //[4]    ri_rserr_cnt_ld_en
	data8 |= (0 << 3);  //[3]    ri_rserr_vld_ld_en
	data8 |= (0 << 2);  //[2]    ri_rserr_ssb_ctl_en
	data8 |= (0 << 0);  //[1:0]  ri_rserr_chk_mod2_ctrl
	hdmirx_wr_cor(H21RXSB_CTRL4_M42H_IVCRX, data8, port);

	//=============H21RXSB_RS_ACC_ERR_TSH_LSB_M42H_IVCRX=================
	data8  = 0;
	data8 |= (0xf0 << 0);  //[7:0] ri_accm_err_thr_lsb
	hdmirx_wr_cor(H21RXSB_RS_ACC_ERR_TSH_LSB_M42H_IVCRX, data8, port);

	//============= H21RXSB_RS_ACC_ERR_TSH_LSB1_M42H_IVCRX=================
	data8  = 0;
	data8 |= (0x00 << 0);  //[7:0] ri_accm_err_thr_lsb1
	hdmirx_wr_cor(H21RXSB_RS_ACC_ERR_TSH_LSB1_M42H_IVCRX, data8, port);

	//============= H21RXSB_RS_ACC_ERR_TSH_MSB_M42H_IVCRX=================
	data8  = 0;
	data8 |= (0x00 << 0);  //[7:0] ri_accm_err_thr_msb
	hdmirx_wr_cor(H21RXSB_RS_ACC_ERR_TSH_MSB_M42H_IVCRX, data8, port);

	//============= H21RXSB_RS_CNT2CHK_TSH_LSB_M42H_IVCRX=================
	data8  = 0;
	data8 |= (0x3c << 0);  //[7:0] ri_cnt2chk_rs_lsb
	hdmirx_wr_cor(H21RXSB_RS_CNT2CHK_TSH_LSB_M42H_IVCRX, data8, port);

	//============= H21RXSB_RS_CNT2CHK_TSH_LSB_M42H_IVCRX=================
	data8  = 0;
	data8 |= (0x0 << 0);  //[7:0] ri_cnt2chk_rs_msb
	hdmirx_wr_cor(H21RXSB_RS_CNT2CHK_TSH_MSB_M42H_IVCRX, data8, port);

	//============= H21RXSB_FRAME_RS_ERR_TSH_LSB_M42H_IVCRX=================
	data8  = 0;
	data8 |= (0xf << 0);  //[7:0] ri_frame_rs_err_thr_lsb
	hdmirx_wr_cor(H21RXSB_FRAME_RS_ERR_TSH_LSB_M42H_IVCRX, data8, port);

	//============= H21RXSB_FRAME_RS_ERR_TSH_LSB1_M42H_IVCRX=================
	data8  = 0;
	data8 |= (0x0 << 0);  //[7:0] ri_frame_rs_err_thr_lsb1
	hdmirx_wr_cor(H21RXSB_FRAME_RS_ERR_TSH_LSB1_M42H_IVCRX, data8, port);

	//============= H21RXSB_FRAME_RS_ERR_TSH_MSB_M42H_IVCRX=================
	data8  = 0;
	data8 |= (0x0 << 0);  //[7:0] ri_frame_rs_err_thr_msb
	hdmirx_wr_cor(H21RXSB_FRAME_RS_ERR_TSH_MSB_M42H_IVCRX, data8, port);

	//============= H21RXSB_CONS_RS_ERR_TSH_LSB_M42H_IVCRX=================
	data8  = 0;
	data8 |= (0x10 << 0);  //[7:0] ri_cons_ri_err_thr_lsb
	hdmirx_wr_cor(H21RXSB_CONS_RS_ERR_TSH_LSB_M42H_IVCRX, data8, port);

	//============= H21RXSB_CONS_RS_ERR_TSH_MSB_M42H_IVCRX=================
	data8  = 0;
	data8 |= (0x0 << 0);  //[7:0] ri_cons_ri_err_thr_msb
	hdmirx_wr_cor(H21RXSB_CONS_RS_ERR_TSH_MSB_M42H_IVCRX, data8, port);

	//============= H21RXSB_CONS_RS_ERR_TSH_LSB_M42H_IVCRX=================
	data8  = 0;
	data8 |= (0xf0 << 0);  //[7:0] ri_cons_ri_err_thr_lsb
	hdmirx_wr_cor(H21RXSB_CONS_RS_ERR_TSH_LSB_M42H_IVCRX, data8, port);

	//============= H21RXSB_CONS_RS_ERR_TSH_MSB_M42H_IVCRX=================
	data8  = 0;
	data8 |= (0x0 << 0);  //[7:0] ri_cons_ri_err_thr_msb
	hdmirx_wr_cor(H21RXSB_CONS_RS_ERR_TSH_MSB_M42H_IVCRX, data8, port);

	//============= H21RXSB_FRAME_TSH_ACCM_RS_ERR_LSB_M42H_IVCRX=================
	data8  = 0;
	data8 |= (0x2 << 0);  //[7:0] ri_given_frame_lsb
	hdmirx_wr_cor(H21RXSB_FRAME_TSH_ACCM_RS_ERR_LSB_M42H_IVCRX, data8, port);

	//============= H21RXSB_FRAME_TSH_ACCM_RS_ERR_MSB_M42H_IVCRX=================
	data8  = 0;
	data8 |= (0x0   << 0);  //[7:0] ri_given_frame_msb
	hdmirx_wr_cor(H21RXSB_FRAME_TSH_ACCM_RS_ERR_MSB_M42H_IVCRX, data8, port);

	//============= H21RXSB_GIVEN_FRAME_RSERR_ACCM_TSH_LSB_M42H_IVCRX=================
	data8  = 0;
	data8 |= (0x0 << 0);  //[7:0] ri_given_frame_err_thr_lsb
	hdmirx_wr_cor(H21RXSB_GIVEN_FRAME_RSERR_ACCM_TSH_LSB_M42H_IVCRX, data8, port);

	//============= H21RXSB_GIVEN_FRAME_RSERR_ACCM_TSH_LSB1_M42H_IVCRX=================
	data8  = 0;
	data8 |= (0x10 << 0);  //[7:0] ri_given_frame_err_thr_lsb1
	hdmirx_wr_cor(H21RXSB_GIVEN_FRAME_RSERR_ACCM_TSH_LSB1_M42H_IVCRX, data8, port);

	//============= H21RXSB_GIVEN_FRAME_RSERR_ACCM_TSH_MSB_M42H_IVCRX=================
	data8  = 0;
	data8 |= (0x00 << 0);  //[7:0] ri_given_frame_err_thr_msb
	hdmirx_wr_cor(H21RXSB_GIVEN_FRAME_RSERR_ACCM_TSH_MSB_M42H_IVCRX, data8, port);

	//============= H21RXSB_RS_ERRCNT_TSH_MSB_M42H_IVCRX=================
	data8  = 0;
	data8 |= (0x10 << 0);  //[7:0] reg_rserrcnt_tsh_msb
	hdmirx_wr_cor(H21RXSB_RS_ERRCNT_TSH_MSB_M42H_IVCRX, data8, port);

	//============= H21RXSB_RS_ERRCNT_TSH_LSB_M42H_IVCRX=================
	data8  = 0;
	data8 |= (0x0 << 0);  //[7:0] reg_rserrcnt_tsh_lsb
	hdmirx_wr_cor(H21RXSB_RS_ERRCNT_TSH_LSB_M42H_IVCRX, data8, port);

	//============= H21RXSB_INTR1_MASK_M42H_IVCRX=================
	data8  = 0;
	data8 |= (0x0 << 6);  //[6] reg_intr1_mask6
	data8 |= (0x0 << 5);  //[5] reg_intr1_mask5
	data8 |= (0x0 << 4);  //[4] reg_intr1_mask4
	data8 |= (0x0 << 3);  //[3] reg_intr1_mask3
	data8 |= (0x0 << 2);  //[2] reg_intr1_mask2
	data8 |= (0x0 << 1);  //[1] reg_intr1_mask1
	data8 |= (0x1 << 0);  //[0] reg_intr1_mask0
	hdmirx_wr_cor(H21RXSB_INTR1_MASK_M42H_IVCRX, data8, port);

	/* t3x new */
	//============= H21RXSB_GP1_REGISTER_M42H_IVCRX=================
	data8 = 0;
	data8 |= (1 << 1); //[1] =0:use sb_checker sb_sync as frl_lock signal;=1 not use
	data8 |= (1 << 2); //[2] =1:use sb_checker sr_sync as frl_lock signal;=0 not use
	data8 |= (1 << 3); //[3] auto reset after overlap
	data8 |= (1 << 5); //[3] fifo reset enable both t_clk and l_clk
	hdmirx_wr_cor(H21RXSB_GP1_REGISTER_M42H_IVCRX, data8, port);

	//============= H21RXSB_GP3_REGISTER_M42H_IVCRX=================
	data8 = 0;
	data8 |= (1 << 5);   //[5] use sb_checker sb_sr_sync as frl_lock signal to frl pipe
	hdmirx_wr_cor(H21RXSB_GP3_REGISTER_M42H_IVCRX, data8, port);
	/* end */
	//============= H21RXSB_CTRL_M42H_IVCRX=================
	frl_rate_sel = (rx[port].var.frl_rate > FRL_6G_3LANE) ? 1 : 0;
	data8 = 0;
	data8 |= (0 << 6); //reg_debug_ctl
	data8 |= (1 << 5); //reg_filter_en
    //data8 |= (1 << 4); //reg_scramble_en
	data8 |= (frl_scrambler_en << 4); //reg_scramble_en
	data8 |= (0 << 2); //reg_scdt_reset_mask
	data8 |= (frl_rate_sel << 1); //reg_lane_sel_mode: 0-3Lane; 1:4Lane
	data8 |= (1 << 0); //reg_en
	hdmirx_wr_cor(H21RXSB_CTRL_M42H_IVCRX, data8, port); //

	//----- init th ---
	//default = 0x100000; sim time too long , make the value small.
	hdmirx_wr_cor(H21RXSB_ITH0_M42H_IVCRX, 0x0, port); //
	hdmirx_wr_cor(H21RXSB_ITH1_M42H_IVCRX, 0x0, port); //
	hdmirx_wr_cor(H21RXSB_ITH2_M42H_IVCRX, 0x2, port); //
	//tracking difference threshold high 8bit,0x10->0xf
	hdmirx_wr_cor(H21RXSB_D2TH_M42H_IVCRX, 0xf, port);
	//============= H21RXSB_CTRL6_M42H_IVCRX=================
	data8  = 0;
	data8 |= (0x1   << 0);  //[7:0] reg_lane_en_sel_scdcs_sb
	hdmirx_wr_cor(H21RXSB_CTRL6_M42H_IVCRX, data8, port); //0x1568

	hdmirx_wr_bits_top(TOP_CLK_CNTL, _BIT(0), 0, port);
}

void rx_pwrcntl_mem_pd_cfg(void)
{
	rx_hdcp22_wr_top(PWRCTRL_MEM_PD19, 0);
	rx_hdcp22_wr_top(PWRCTRL_MEM_PD20, 0);
	rx_hdcp22_wr_top(PWRCTRL_MEM_PD21, 0);
}

void hdmirx_rd_check_top(u32 addr, u32 exp_data, u32 mask, u8 port)
{
	u32 rd_data;

	rd_data = hdmirx_rd_top(addr, port);
	rx_pr("PRBS_ANA_BER=0x%x\n", rd_data);
	if ((rd_data | mask) != (exp_data | mask))
		rx_pr("top reg 0x%x,rd_data=0x%x, exp_data=0x%x mask=0x%x\n",
		addr, rd_data, exp_data, mask);
}

void hdmirx_poll_top(u32 addr, u32 exp_data, u32 mask, u8 port)
{
	u32 rd_data;
	int exit_cnt = 0;

	rd_data = hdmirx_rd_top(addr, port);
	while ((rd_data | mask) != (exp_data | mask)) {
		rd_data = hdmirx_rd_top(addr, port);
		if (exit_cnt++ == 100000) {
			rx_pr("break;\n");
			exit_cnt = 0;
			break;
		}
	}
}

void rx_t3x_prbs(void)
{
	u32 data32;
	u8 port = E_PORT2;

	rx_pr("prbs start\n");
	//Prbs/Pattern config
	//Release Bist Reset
	// [8] ~bist rst n = 0// [7]~chan rst n = 0
	hdmirx_wr_top(TOP_SW_RESET, 0xfffffe7f, port);
	//wait for PHY initial hdmirx delay us(50);
	rx_pr("prbs-1\n");
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL1, 0, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL2, 0, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL3, 0, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL4, 0, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_AFE, 0, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_CTRL, 0, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_DFE, 0, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_PI, 0, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, 0, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, 0, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_MISC0, 0, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_MISC1, 0, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_MISC2, 0, port);
	rx_pr("prbs-2\n");
	//phy reg set default value
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_AFE, 0x0370ffff, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_CTRL, 0x07f06555, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_DFE, 0x15ff1a05, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_PI, 0x51001010, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, 0x040074b3, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, 0x3013304f, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_MISC0, 0x01fcffff, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_MISC1, 0x20f70f0f, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_MISC2, 0x00005500, port);
	rx_pr("prbs-3\n");
	//program pll reg
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x45001800, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL1, 0x01401202, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL2, 0x000c7d01, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL3, 0xf0002dd3, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL4, 0x55813041, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL2, 0x000c7d07, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL2, 0x000c7d01, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x45001801, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x45001803, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL2, 0x000c7d07, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x45001807, port);
	//toggle phy dch_rstn and tmds clk_en
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_MISC0, 0x01fcfff0, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_CTRL, 0x07f06555, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_MISC0, 0x01fcffff, port);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_CTRL, 0x07f06555, port);
	rx_pr("prbs-4\n");
	//select tmds or frl mode
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_MISC0, 0x01fcffff, port);
	mdelay(1000);
	rx_pr("MISC_STAT0 = 0x%x\n", hdmirx_rd_top(TOP_MISC_STAT0_T3X, port));
	dump_state(RX_DUMP_PHY, port);
	mdelay(1000);
	dump_state(RX_DUMP_PHY, port);
	mdelay(1000);
	dump_state(RX_DUMP_PHY, port);
	mdelay(1000);
	dump_state(RX_DUMP_PHY, port);

	hdmirx_poll_top(TOP_MISC_STAT0_T3X, 0X5 << 16, ~(0X5 << 16), port);
	rx_pr("prbs phy end\n");

	//config channel switch
	data32  = 0;
	data32 |= (3 << 24); /* source_2 */
	data32 |= (0 << 4); /* [  4]  valid_always*/
	data32 |= (7 << 0); /* [3:0]  decoup_thresh*/
	hdmirx_wr_top(TOP_CHAN_SWITCH_1_T3X, data32, port);

	data32  = 0;
	data32 |= (2 << 28); /* [29:28]      source_2 */
	data32 |= (1 << 26); /* [27:26]      source_1 */
	data32 |= (0); /* [25:24]      source_0 */
	hdmirx_wr_top(TOP_CHAN_SWITCH_0, data32, port);
	data32 |= (1 << 0);// [0] enable
	hdmirx_wr_top(TOP_CHAN_SWITCH_0, data32, port);

	if (1) {
		rx_pr("prbs-5\n");
		//config prbs_gen
		data32  = 0;
		data32 |= (0 << 16);// in_bist_err
		data32 |= (3 << 5);// decouple_thresh_prbs
		data32 |= (1 << 2);//prbs_mode. 1=p7;2=p11,3=p15,5=p20,6=p23,7=p31
		data32 |= (1 << 0);//prbs_en
		hdmirx_wr_top(TOP_PRBS_GEN, data32, port);

		//config prbs_ana
		data32  = 0;
		data32 |= (1 << 28);
		data32 |= (0 << 25);
		data32 |= (0 << 24);
		data32 |= (1 << 20);
		data32 |= (0 << 17);
		data32 |= (0 << 16);
		data32 |= (1 << 12);
		data32 |= (0 << 9);
		data32 |= (0 << 8);
		data32 |= (1 << 4);
		data32 |= (0 << 1);
		data32 |= (0 << 0);
		hdmirx_wr_top(TOP_PRBS_ANA_0, data32, port);
		data32 |= (1 << 24);
		data32 |= (1 << 16);
		data32 |= (1 << 8);
		data32 |= (1 << 0);
		hdmirx_wr_top(TOP_PRBS_ANA_0, data32, port);
		rx_pr("prbs-6\n");
		mdelay(1);

		//check prbs
		data32 = 0;
		data32 |= (0 << 7);
		data32 |= (1 << 6);
		data32 |= (0 << 5);
		data32 |= (1 << 4);
		data32 |= (0 << 3);
		data32 |= (1 << 2);
		data32 |= (0 << 1);
		data32 |= (1 << 0);
		hdmirx_rd_check_top(TOP_PRBS_ANA_STAT, data32, 0, port);

		//check err cnt
		hdmirx_rd_check_top(TOP_PRBS_ANA_BER_CH0, 0, 0, port);
		hdmirx_rd_check_top(TOP_PRBS_ANA_BER_CH1, 0, 0, port);
		hdmirx_rd_check_top(TOP_PRBS_ANA_BER_CH2, 0, 0, port);
		hdmirx_rd_check_top(TOP_PRBS_ANA_BER_CH3, 0, 0, port);
		rx_pr("prbs end\n");
	} else {
		data32 = 0;
		data32 |= (0x133 << 20);
		data32 |= (0x122 << 10);
		data32 |= (0x111 << 0);
		hdmirx_wr_top_common(TOP_SHFT_PTTN_0, data32);

		data32 = 0;
		data32 |= (0x266 << 20);
		data32 |= (0x255 << 10);
		data32 |= (0x244 << 0);
		hdmirx_wr_top_common(TOP_SHFT_PTTN_1, data32);

		data32 = 0;
		data32 |= (0x399 << 20);
		data32 |= (0x388 << 10);
		data32 |= (0x377 << 0);
		hdmirx_wr_top_common(TOP_SHFT_PTTN_2, data32);

		data32 = 0;
		data32 |= (0x0cc << 20);
		data32 |= (0x0bb << 10);
		data32 |= (0x0aa << 0);
		hdmirx_wr_top_common(TOP_SHFT_PTTN_3, data32);

		//config prbs_gen
		data32  = 0;
		data32 |= (4 << 10);// in_bist_err
		data32 |= (3 << 5);// decouple_thresh_prbs
		data32 |= (1 << 9);//prbs_mode. 1=p7;2=p11,3=p15,5=p20,6=p23,7=p31
		hdmirx_wr_top(TOP_PRBS_GEN, data32, port);

		//config pattern ana
		data32  = 0;
		data32 |= (0xffff << 4);
		data32 |= (0 << 3);
		data32 |= (0 << 1);
		data32 |= (1 << 0);
		hdmirx_wr_top(TOP_SHFT_ANA_CNTL, data32, port);

		//config pattern ana
		data32  = 0;
		data32 |= (0 << 4);
		data32 |= (0 << 1);
		data32 |= (1 << 0);
		hdmirx_rd_check_top(TOP_SHFT_ANA_STAT, data32, 0, port);
	}
}

void rx_long_bist_t3x(void)
{
	u8 port = E_PORT2;
	u32 data32 = 0;

	//Release Bist Reset
	// [8] ~bist rst n = 0// [7]~chan rst n = 0
	hdmirx_wr_top(TOP_SW_RESET, 0xfffffe7f, port);

	//1. phy ldo en
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_MISC0, 0x01bcfff0, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_MISC1, 0x30f700ff, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_MISC2, 0x00005a00, port);
	usleep_range(20, 30);
	//2. pll frl 12g                            ,
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x0500fa00, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL1, 0x014810e6, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL2, 0x0, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL3, 0x0, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL4, 0x0, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL2, 0x0, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x0500fa01, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x0500fa03, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL1, 0x014010e6, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x0500fa07, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, 0x4500fa07, port);
	usleep_range(20, 30);
	rx_pr("ctrl0=0x%x\n", hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PLL_CTRL0, port));

	//3. phy cdr reset
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_AFE, 0x2300ffff, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_DFE, 0x05ff1a05, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_PI, 0x01100000, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_CTRL, 0x07f06555, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, 0x040010c2, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, 0x3091104f, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_MISC0, 0x01bcffff, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, 0x040070c2, port);
	usleep_range(20, 30);
	hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, 0x3091304f, port);

	hdmirx_poll_top(TOP_MISC_STAT0_T3X, 0X5 << 16, ~(0X5 << 16), port);
	rx_pr("long bist phy end\n");

	//config channel switch
	data32  = 0;
	data32 |= (3 << 24); /* source_2 */
	data32 |= (0 << 4); /* [  4]  valid_always*/
	data32 |= (7 << 0); /* [3:0]  decoup_thresh*/
	hdmirx_wr_top(TOP_CHAN_SWITCH_1_T3X, data32, port);

	data32  = 0;
	data32 |= (2 << 28); /* [29:28]      source_2 */
	data32 |= (1 << 26); /* [27:26]      source_1 */
	data32 |= 0; /* [25:24]      source_0 */
	hdmirx_wr_top(TOP_CHAN_SWITCH_0, data32, port);
	data32 |= (1 << 0);// [0] enable
	hdmirx_wr_top(TOP_CHAN_SWITCH_0, data32, port);

	//config prbs_gen
	data32  = 0;
	data32 |= (0 << 16);// in_bist_err
	data32 |= (3 << 5);// decouple_thresh_prbs
	data32 |= (1 << 2);//prbs_mode. 1=p7;2=p11,3=p15,5=p20,6=p23,7=p31
	data32 |= (1 << 0);//prbs_en
	hdmirx_wr_top(TOP_PRBS_GEN, data32, port);

	//config prbs_ana
	data32  = 0;
	data32 |= (1 << 28);
	data32 |= (0 << 25);
	data32 |= (0 << 24);
	data32 |= (1 << 20);
	data32 |= (0 << 17);
	data32 |= (0 << 16);
	data32 |= (1 << 12);
	data32 |= (0 << 9);
	data32 |= (0 << 8);
	data32 |= (1 << 4);
	data32 |= (0 << 1);
	data32 |= (0 << 0);
	hdmirx_wr_top(TOP_PRBS_ANA_0, data32, port);
	data32 |= (1 << 24);
	data32 |= (1 << 16);
	data32 |= (1 << 8);
	data32 |= (1 << 0);
	hdmirx_wr_top(TOP_PRBS_ANA_0, data32, port);
	mdelay(1);

	//check prbs
	data32 = 0;
	data32 |= (0 << 7);
	data32 |= (1 << 6);
	data32 |= (0 << 5);
	data32 |= (1 << 4);
	data32 |= (0 << 3);
	data32 |= (1 << 2);
	data32 |= (0 << 1);
	data32 |= (1 << 0);
	hdmirx_rd_check_top(TOP_PRBS_ANA_STAT, data32, 0, port);

	//check err cnt
	hdmirx_rd_check_top(TOP_PRBS_ANA_BER_CH0, 0, 0, port);
	hdmirx_rd_check_top(TOP_PRBS_ANA_BER_CH1, 0, 0, port);
	hdmirx_rd_check_top(TOP_PRBS_ANA_BER_CH2, 0, 0, port);
	hdmirx_rd_check_top(TOP_PRBS_ANA_BER_CH3, 0, 0, port);
	rx_pr("long bist end\n");
}

void audio_setting_for_aud21(int frl_rate, u8 port)
{
	if (rx[port].var.frl_rate == FRL_RATE_3G_3LANES) {
		wr_reg_clk_ctl(T3X_CLKCTRL_AUD21_PLL_CTRL1, 0x8);
		//aud div
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_PI,
		MSK(2, 12), 0x1, port);
		//Na
		hdmirx_wr_bits_clk_ctl(T3X_CLKCTRL_AUD21_PLL_CTRL0,
		_BIT(13), 0);
		//ctsa
		hdmirx_wr_bits_clk_ctl(T3X_CLKCTRL_AUD21_PLL_CTRL2,
		_BIT(19), 1);
		//ctsa
		hdmirx_wr_bits_clk_ctl(T3X_CLKCTRL_AUD21_PLL_CTRL1,
		_BIT(9), 1);
	} else if (rx[port].var.frl_rate == FRL_RATE_6G_3LANES) {
		wr_reg_clk_ctl(T3X_CLKCTRL_AUD21_PLL_CTRL1, 0x8);
		//aud div
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_PI,
		MSK(2, 12), 0x2, port);
		//Na
		hdmirx_wr_bits_clk_ctl(T3X_CLKCTRL_AUD21_PLL_CTRL0,
		_BIT(13), 0);
		//ctsa
		hdmirx_wr_bits_clk_ctl(T3X_CLKCTRL_AUD21_PLL_CTRL2,
		_BIT(19), 1);
		//ctsa
		hdmirx_wr_bits_clk_ctl(T3X_CLKCTRL_AUD21_PLL_CTRL1,
		_BIT(9), 0);
	} else if (rx[port].var.frl_rate == FRL_RATE_6G_4LANES) {
		wr_reg_clk_ctl(T3X_CLKCTRL_AUD21_PLL_CTRL1, 0x8);
		//aud div
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_PI,
		MSK(2, 12), 0x2, port);
		//Na
		hdmirx_wr_bits_clk_ctl(T3X_CLKCTRL_AUD21_PLL_CTRL0,
		_BIT(13), 0);
		//ctsa
		hdmirx_wr_bits_clk_ctl(T3X_CLKCTRL_AUD21_PLL_CTRL2,
		_BIT(19), 1);
		//ctsa
		hdmirx_wr_bits_clk_ctl(T3X_CLKCTRL_AUD21_PLL_CTRL1,
		_BIT(9), 0);
	} else if (rx[port].var.frl_rate == FRL_RATE_8G_4LANES) {
		wr_reg_clk_ctl(T3X_CLKCTRL_AUD21_PLL_CTRL1, 0x8);
		//aud div
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_PI,
		MSK(2, 12), 0x2, port);
		//Na
		hdmirx_wr_bits_clk_ctl(T3X_CLKCTRL_AUD21_PLL_CTRL0,
		_BIT(13), 0);
		//ctsa
		hdmirx_wr_bits_clk_ctl(T3X_CLKCTRL_AUD21_PLL_CTRL2,
		_BIT(19), 1);
		//ctsa
		hdmirx_wr_bits_clk_ctl(T3X_CLKCTRL_AUD21_PLL_CTRL1,
		_BIT(9), 0);
	} else if (rx[port].var.frl_rate == FRL_RATE_10G_4LANES) {
		wr_reg_clk_ctl(T3X_CLKCTRL_AUD21_PLL_CTRL1, 0x8);
		//aud div
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_PI,
		MSK(2, 12), 0x3, port);
		//Na
		hdmirx_wr_bits_clk_ctl(T3X_CLKCTRL_AUD21_PLL_CTRL0,
		_BIT(13), 1);
		//ctsa
		hdmirx_wr_bits_clk_ctl(T3X_CLKCTRL_AUD21_PLL_CTRL2,
		_BIT(19), 1);
		//ctsa
		hdmirx_wr_bits_clk_ctl(T3X_CLKCTRL_AUD21_PLL_CTRL1,
		_BIT(9), 0);
	} else {
		wr_reg_clk_ctl(T3X_CLKCTRL_AUD21_PLL_CTRL1, 0x8);
		//aud div
		hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_PI,
		MSK(2, 12), 0x2, port);
		//Na
		hdmirx_wr_bits_clk_ctl(T3X_CLKCTRL_AUD21_PLL_CTRL0,
		_BIT(13), 1);
		//ctsa
		hdmirx_wr_bits_clk_ctl(T3X_CLKCTRL_AUD21_PLL_CTRL2,
		_BIT(19), 1);
		//ctsa
		hdmirx_wr_bits_clk_ctl(T3X_CLKCTRL_AUD21_PLL_CTRL1,
		_BIT(9), 0);
	}
}

void dump_aud21_param(u8 port)
{
	u32 data0, data1, data2, data3, data32;
	u32 ctsa2 = 0;
	int n;
	u32 clk_test_after_mux, cts;

	//update todo
	hdmirx_wr_bits_top_common_1(TOP_ACR_CNTL_STAT, _BIT(11), 1);

	data0 = rd_reg_clk_ctl(T3X_CLKCTRL_AUD21_PLL_CTRL0);
	data1 = rd_reg_clk_ctl(T3X_CLKCTRL_AUD21_PLL_CTRL1);
	data2 = rd_reg_clk_ctl(T3X_CLKCTRL_AUD21_PLL_CTRL2);
	data3 = rd_reg_clk_ctl(T3X_CLKCTRL_AUD21_PLL_CTRL3);
	n = hdmirx_rd_top_common_1(TOP_ACR_N_STST);
	cts = hdmirx_rd_top_common_1(TOP_ACR_CTS_STST);
	clk_test_after_mux = meson_clk_measure_with_precision(143, 32);
	rx_pr("N=%d, CTS=%d\n", n, cts);
	rx_pr("clk1618=%d\n", meson_clk_measure_with_precision(9, 32));
	rx_pr("pll_clk_test=%d\n", meson_clk_measure_with_precision(14, 32));
	rx_pr("clk_audpll=%d\n", meson_clk_measure_with_precision(148, 32));
	rx_pr("clk_test_after_mux=%d\n", clk_test_after_mux);
	rx_pr("aud21_pll_clk1=%d\n", meson_clk_measure_with_precision(147, 32));
	rx_pr("aud_pll(N/CTS)=%d\n", clk_test_after_mux * n / cts);
	rx_pr("Na=0x%x\n", (((data0 >> 13) & 0x1) == 1) ? 2 : 1);
	if ((data2 & 0x7) == 0)
		rx_pr("CTSa = 1\n");
	else if ((data2 & 0x7) == 1)
		rx_pr("CTS_a = 2\n");
	else if ((data2 & 0x7) == 2)
		rx_pr("CTS_a = 4\n");
	else if ((data2 & 0x7) == 3)
		rx_pr("CTS_a = 8\n");
	ctsa2 = ((data2 >> 19) << 3) | ((data1 >> 9) & 0x7);
	switch (ctsa2) {
	case 0:
		rx_pr("ctsa_2=1\n");
		break;
	case 1:
		rx_pr("ctsa_2=40\n");
		break;
	case 2:
		rx_pr("ctsa_2=4\n");
		break;
	case 3:
		rx_pr("ctsa_2=8\n");
		break;
	case 4:
		rx_pr("ctsa_2=5\n");
		break;
	case 5:
		rx_pr("ctsa_2=10\n");
		break;
	case 6:
		rx_pr("ctsa_2=16\n");
		break;
	case 7:
		rx_pr("ctsa_2=20\n");
		break;
	case 8:
		rx_pr("ctsa_2=2.25\n");
		break;
	case 9:
		rx_pr("ctsa_2=4.5\n");
		break;
	case 10:
		rx_pr("ctsa_2=9\n");
		break;
	default:
		break;
	}

	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_PI, port);
	switch ((data32 >> 12) & 0x3) {
	case 0:
		rx_pr("clk_audpll_div=2\n");
		break;
	case 1:
		rx_pr("clk_audpll_div=4\n");
		break;
	case 2:
		rx_pr("clk_audpll_div=8\n");
		break;
	case 3:
		rx_pr("clk_audpll_div=16\n");
		break;
	}
	data32 = (hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PLL_CTRL2, port) >> 30) & 0x1;
	if (data32 == 0)
		rx_pr("select clk_test\n");
	else
		rx_pr("select clk_audpll21\n");
	rx_pr("ana 4x = 0x%x\n", hdmirx_rd_top_common_1(TOP_ACR_CNTL2_T3X));
}

void clk_init_cor_t3x(void)
{
	u32 data32;
	u8 port = rx_info.main_port;

	rx_pr("\n clk_init\n");
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

	/* new added for t3x emp */
	data32  = 0;
	data32 |= (2 << 9);     // [10: 9] clk_sel for cts_hdmirx_axi_clk: 2=fclk_div3
	data32 |= (0 << 8);     // [    8] clk_en for cts_hdmirx_axi_clk
	data32 |= (0 << 0);     // [ 6: 0] clk_div for cts_hdmirx_axi_clk: fclk_div3/1=666/1=666M
	wr_reg_clk_ctl(RX_CLK_CTRL4, data32);
	data32 |= (1 << 8);     // [    8] clk_en for cts_hdmirx_axi_clk
	wr_reg_clk_ctl(RX_CLK_CTRL4, data32);

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

void rx_dig_clk_en_t3x(bool en)
{
	hdmirx_wr_bits_clk_ctl(RX_CLK_CTRL, CLK_2M_EN, en);
	hdmirx_wr_bits_clk_ctl(RX_CLK_CTRL, CLK_5M_EN, en);
	hdmirx_wr_bits_clk_ctl(RX_CLK_CTRL1, HDCP2X_ECLK_EN, en);
	hdmirx_wr_bits_clk_ctl(RX_CLK_CTRL1, CFG_CLK_EN, en);
	hdmirx_wr_bits_clk_ctl(RX_CLK_CTRL3, METER_CLK_EN, en);
	/* added for t3x emp */
	hdmirx_wr_bits_clk_ctl(RX_CLK_CTRL4, AXI_CLK_EN, en);
}

void hdmirx_vga_gain_tuning(u8 port)
{
	//T3X_HDMIRX21PHY_DCHA_AFE
	u32 data32;
	u32 dfe0_tap0, dfe1_tap0, dfe2_tap0, dfe3_tap0;
	u32 tap0_0_def, tap0_1_def, tap0_2_def, tap0_3_def;
	u32 tap0_0_dec, tap0_1_dec, tap0_2_dec, tap0_3_dec;
	bool tap0_0_done, tap0_1_done, tap0_2_done, tap0_3_done;
	static int tap0_0_cnt, tap0_1_cnt, tap0_2_cnt, tap0_3_cnt;
	//int i;
	unsigned long timeout = jiffies + HZ / 100;
	//bool b_time_out;

	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_EYE_EN, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ, MUX_BLOCK_SEL, 0x0, port);
	hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR, MUX_DFE_OFST_EYE, 0x0, port);
	usleep_range(100, 110);
	data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PHY_DCH_STAT, port);
	dfe0_tap0 = data32 & 0x7f;
	dfe1_tap0 = (data32 >> 8) & 0x7f;
	dfe2_tap0 = (data32 >> 16) & 0x7f;
	dfe3_tap0 = (data32 >> 24) & 0x7f;
	tap0_0_done = 0;
	tap0_1_done = 0;
	tap0_2_done = 0;
	tap0_3_done = 0;
	while (time_before(jiffies, timeout)) {
		if (!tap0_0_done || !tap0_1_done || !tap0_2_done || !tap0_3_done) {
			hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR,
				MUX_EYE_EN, 0x0, port);
			hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_EQ,
				MUX_BLOCK_SEL, 0x0, port);
			hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHD_CDR,
				MUX_DFE_OFST_EYE, 0x0, port);
			usleep_range(100, 110);
			data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PHY_DCH_STAT, port);
			dfe0_tap0 = data32 & 0x7f;
			dfe1_tap0 = (data32 >> 8) & 0x7f;
			dfe2_tap0 = (data32 >> 16) & 0x7f;
			dfe3_tap0 = (data32 >> 24) & 0x7f;

			tap0_0_def = hdmirx_rd_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_AFE,
				MSK(4, 0), port);
			if (dfe0_tap0 < vga_tuning_min && !tap0_0_done) {
				tap0_0_dec = graytodecimal_t3x(tap0_0_def);
				tap0_0_dec += 1;
				hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_AFE,
					MSK(4, 0), decimaltogray_t3x(tap0_0_dec), port);
				if (tap0_0_dec == 15)
					tap0_0_done = 1;
			} else if (dfe0_tap0 > vga_tuning_max && !tap0_0_done) {
				tap0_0_dec = graytodecimal_t3x(tap0_0_def);
				tap0_0_dec -= 1;
				hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_AFE,
					MSK(4, 0), decimaltogray_t3x(tap0_0_dec), port);
				dfe0_tap0 = data32 & 0x7f;
				if (tap0_0_dec == 0)
					tap0_0_done = 1;
			} else {
				tap0_0_cnt++;
				if (tap0_0_cnt == 3) {
					tap0_0_done = 1;
					tap0_0_cnt = 0;
				}
			}
			tap0_1_def = hdmirx_rd_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_AFE,
			MSK(4, 4), port);
			if (dfe1_tap0 < vga_tuning_min && !tap0_1_done) {
				tap0_1_dec = graytodecimal_t3x(tap0_1_def);
				tap0_1_dec += 1;
				hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_AFE, MSK(4, 4),
				    decimaltogray_t3x(tap0_1_dec), port);
				if (tap0_1_dec == 15)
					tap0_1_done = 1;
			} else if (dfe1_tap0 > vga_tuning_max && !tap0_1_done) {
				tap0_1_dec = graytodecimal_t3x(tap0_1_def);
				tap0_1_dec -= 1;
				hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_AFE, MSK(4, 4),
				    decimaltogray_t3x(tap0_1_dec), port);
				if (tap0_1_dec == 0)
					tap0_1_done = 1;
			} else {
				tap0_1_cnt++;
				if (tap0_1_cnt == 3) {
					tap0_1_done = 1;
					tap0_1_cnt = 0;
				}
			}
			tap0_2_def = hdmirx_rd_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_AFE,
			MSK(4, 8), port);
			if (dfe2_tap0 < vga_tuning_min && !tap0_2_done) {
				tap0_2_dec = graytodecimal_t3x(tap0_2_def);
				tap0_2_dec += 1;
				hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_AFE, MSK(4, 8),
				    decimaltogray_t3x(tap0_2_dec), port);
				if (tap0_2_dec == 15)
					tap0_2_done = 1;
			} else if (dfe2_tap0 > vga_tuning_max && !tap0_2_done) {
				tap0_2_dec = graytodecimal_t3x(tap0_2_def);
				tap0_2_dec -= 1;
				hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_AFE,
					MSK(4, 8), decimaltogray_t3x(tap0_2_dec), port);
				if (tap0_2_dec == 0)
					tap0_2_done = 1;
			} else {
				tap0_2_cnt++;
				if (tap0_2_cnt == 3) {
					tap0_2_done = 1;
					tap0_2_cnt = 0;
				}
			}
			tap0_3_def = hdmirx_rd_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_AFE,
			MSK(4, 12), port);
			if (dfe3_tap0 < vga_tuning_min && !tap0_3_done) {
				tap0_3_dec = graytodecimal_t3x(tap0_3_def);
				tap0_3_dec += 1;
				hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_AFE, MSK(4, 12),
					decimaltogray_t3x(tap0_3_dec), port);
				if (tap0_3_dec == 15)
					tap0_3_done = 1;
			} else if (dfe3_tap0 > vga_tuning_max && !tap0_3_done) {
				tap0_3_dec = graytodecimal_t3x(tap0_3_def);
				tap0_3_dec -= 1;
				hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_AFE, MSK(4, 12),
				    decimaltogray_t3x(tap0_3_dec), port);
				dfe3_tap0 = (data32 >> 24) & 0x7f;
				if (tap0_3_dec == 0)
					tap0_3_done = 1;
			} else {
				tap0_3_cnt++;
				if (tap0_3_cnt == 3) {
					tap0_3_done = 1;
					tap0_3_cnt = 0;
				}
			}
		} else {
			if (log_level & FRL_LOG) {
				rx_pr("tuning dfe0_tap0 = 0x%x\n", dfe0_tap0);
				rx_pr("tuning dfe1_tap0 = 0x%x\n", dfe1_tap0);
				rx_pr("tuning dfe2_tap0 = 0x%x\n", dfe2_tap0);
				rx_pr("tuning dfe3_tap0 = 0x%x\n", dfe3_tap0);
			}
			break;
		}
	}
	if (!tap0_0_done || !tap0_1_done || !tap0_2_done || !tap0_3_done) {
		if (log_level & FRL_LOG) {
			rx_pr("vga gain timeout\n");
			rx_pr("tuning dfe0_tap0 = 0x%x\n", dfe0_tap0);
			rx_pr("tuning dfe1_tap0 = 0x%x\n", dfe1_tap0);
			rx_pr("tuning dfe2_tap0 = 0x%x\n", dfe2_tap0);
			rx_pr("tuning dfe3_tap0 = 0x%x\n", dfe3_tap0);
		}
	}
	tap0_0_cnt = 0;
	tap0_1_cnt = 0;
	tap0_2_cnt = 0;
	tap0_3_cnt = 0;
	if (log_level & FRL_LOG)
		rx_pr("vga tuning done\n");
}

void rx_cor_reset_t3x(u8 port)
{
	rx_pr("cor reset\n");
	hdmirx_wr_top(TOP_SW_RESET, 1, port);
	udelay(1);
	hdmirx_wr_top(TOP_SW_RESET, 0, port);
}

void clr_frl_fifo_status(u8 port)
{
	hdmirx_wr_cor(VP_FDET_IRQ_STATUS_VID_IVCRX, 0xff, port);
	udelay(1);
	hdmirx_wr_cor(VP_FDET_IRQ_STATUS_VID_IVCRX + 1, 0xff, port);
	udelay(1);
	hdmirx_wr_cor(VP_FDET_IRQ_STATUS_VID_IVCRX + 2, 0xff, port);
}

void cor_debug_t3x(u8 port)
{
	hdmirx_wr_cor(VP_FDET_IRQ_MASK_VID_IVCRX, 0x80, port);
	udelay(1);
	hdmirx_wr_cor(VP_FDET_IRQ_MASK_VID_IVCRX + 1, 0x7, port);
	udelay(1);
	hdmirx_wr_cor(VP_FDET_IRQ_MASK_VID_IVCRX + 2, 0x0, port);
	udelay(1);
	hdmirx_wr_bits_cor(RX_GRP_INTR1_MASK_PWD_IVCRX, _BIT(1), 1, port);
}

void rx_rcc_err_frl_config(u8 port)
{
	hdmirx_wr_bits_cor(DPLL_CTRL0_DPLL_IVCRX, MSK(3, 0), 0x7, port);
	hdmirx_wr_bits_cor(DPLL_CTRL0_DPLL_IVCRX, MSK(3, 0), 0x0, port);
	hdmirx_wr_cor(SCDCS_1S_IN_20MS_CNT_SCDC_IVCRX, err_cnt, port);
	//hdmirx_wr_cor(SCDCS_CHR_ERRCNT_MAX0_SCDC_IVCRX, err_cnt, port);
	//hdmirx_wr_cor(SCDCS_CHR_ERRCNT_MAX1_SCDC_IVCRX, 0x0, port);
	hdmirx_wr_bits_cor(SCDCS_STATUS_FLAGS0_SCDC_IVCRX, _BIT(6), 0, port);
}

void rx_read_ecc_err(u8 port)
{
	u32 ch0, ch1, ch2, ch3;

	rx_get_error_cnt(&ch0, &ch1, &ch2, &ch3, port);
	if (rx[port].state >= FLT_RX_LTS_P) {
		if (ch0 || ch1 || ch2 || ch3) {
			rx_pr("port %d err cnt:%d,%d,%d,%d\n", port, ch0, ch1, ch2, ch3);
			rx_pr("ced update flag 0x%x\n",
			hdmirx_rd_cor(SCDCS_UPD_FLAGS_SCDC_IVCRX, port));
		}
	}
}

bool is_fsm_ready_t3x(void)
{
	return rx_info.chip_id == CHIP_ID_T3X &&
		hdmi_cec_en != 0xff && is_valid_edid_data(edid_cur);
}

bool rx_is_power_off_t3x(u8 port)
{
	if (port <= E_PORT1)
		return hdmirx_rd_cor(T3X_HDMIRX20PHY_DCHA_MISC1, port) == 0;
	else
		return hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PHY_MISC0, port) == 0;
}

void rx_switch_to_self_hsync(u8 port, bool en)
{
	//hdmirx_wr_bits_cor(VP_FDET_CLEAR_VID_IVCRX, _BIT(0), 1, port);
	hdmirx_wr_bits_cor(RX_PWD_CTRL_PWD_IVCRX, _BIT(3), en, port);
	hdmirx_wr_bits_cor(HSYNC_GEN_EVEN_CTRL1_PWD_IVCRX, _BIT(5), en, port);
	hdmirx_wr_bits_cor(HSYNC_GEN_EVEN_CTRL1_PWD_IVCRX, _BIT(7), en, port);
	hdmirx_wr_bits_cor(HSYNC_GEN_EVEN_CTRL2_PWD_IVCRX, _BIT(2), en, port);
	hdmirx_wr_bits_cor(HSYNC_GEN_ODD_CTRL1_PWD_IVCRX, _BIT(5), en, port);
	hdmirx_wr_bits_cor(HSYNC_GEN_ODD_CTRL1_PWD_IVCRX, _BIT(7), en, port);
	hdmirx_wr_bits_cor(HSYNC_GEN_ODD_CTRL2_PWD_IVCRX, _BIT(2), en, port);
	//hdmirx_wr_bits_cor(VP_FDET_CLEAR_VID_IVCRX, _BIT(0), 0, port);
}

bool rx_is_switch_to_analog_clk(u8 port)
{
	//to do, now only 4k120 100 yuv 420 has this problem.
	if (rx[port].cur.vactive == 2160 &&
		rx[port].cur.hactive == 1920 &&
		rx[port].pre.colorspace == E_COLOR_YUV420 &&
		rx[port].pre.colordepth == 12 &&
		(rx[port].pre.frame_rate / 100 >= 98 &&
		rx[port].pre.frame_rate / 100 <= 122))
		return true;
	else
		return false;
}

void rx_switch_to_analog_clk(u8 port)
{
	hdmirx_wr_bits_top(TOP_CLK_CNTL, _BIT(0), 1, port);
	if (port == E_PORT2)
		hdmirx_wr_bits_clk_ctl(T3X_CLKCTRL_HDMI_PLL0_CTRL0,
		MSK(3, 13), 0x2);
	else
		hdmirx_wr_bits_clk_ctl(T3X_CLKCTRL_HDMI_PLL1_CTRL0,
		MSK(3, 13), 0x2);
}

void rx_clr_f_det(bool en, u8 port)
{
	hdmirx_wr_bits_cor(VP_FDET_CLEAR_VID_IVCRX, _BIT(0), en, port);
}

