// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/printk.h>
#include <linux/amlogic/media/vout/hdmi_tx21/hdmi_tx_module.h>
#include <linux/delay.h>
#include "common.h"

void s1a_reset_div_clk(struct hdmitx_dev *hdev)
{
	hd21_write_reg(RESETCTRL_RESET0, 1 << 19); /* vid_pll_div */
}

void set21_s1a_hpll_clk_out(u32 frac_rate, u32 clk)
{
	switch (clk) {
	//hdmi txpll ctrl3<18> set 0
	case 5940000:
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x31204F7);
		if (frac_rate)
			hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x8148);
		else
			hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x00010000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x01000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x00218000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x04611001);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x00039300);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0xf0410000);
		usleep_range(10, 20);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x1, 28, 1);
		usleep_range(10, 20);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x331204f7);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0xf0400000);
		pr_info("HPLL: 0x%x\n", hd21_read_reg(ANACTRL_HDMIPLL_CTRL0));
		break;
	case 5680000:
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x030004ec);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x5540);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x01000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x40218000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x05501000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x00150500);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50450000);
		usleep_range(10, 20);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x1, 28, 1);
		usleep_range(10, 20);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x330004ec);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50440000);
		pr_info("HPLL: 0x%x\n", hd21_read_reg(ANACTRL_HDMIPLL_CTRL0));
		break;
	case 5405400:
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x030004e1);
		if (frac_rate)
			hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x00000000);
		else
			hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x7333);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x01000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x40218000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x05501000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x00150500);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50450000);
		usleep_range(10, 20);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x1, 28, 1);
		usleep_range(10, 20);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x330004e1);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50440000);
		pr_info("HPLL: 0x%x\n", hd21_read_reg(ANACTRL_HDMIPLL_CTRL0));
		break;
	case 5200000:
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x030004d8);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x15580);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x01000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x40218000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x05501000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x00150500);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50450000);
		usleep_range(10, 20);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x1, 28, 1);
		usleep_range(10, 20);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x330004d8);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50440000);
		pr_info("HPLL: 0x%x\n", hd21_read_reg(ANACTRL_HDMIPLL_CTRL0));
		break;
	case 4870000:
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x030004ca);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x1d580);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x01000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x40218000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x05501000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x00150500);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50450000);
		usleep_range(10, 20);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x1, 28, 1);
		usleep_range(10, 20);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x330004ca);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50440000);
		pr_info("HPLL: 0x%x\n", hd21_read_reg(ANACTRL_HDMIPLL_CTRL0));
		break;
	case 4455000:
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x030004b9);
		if (frac_rate)
			hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0xe10e);
		else
			hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x14000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x01000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x40218000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x05501000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x00150500);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50450000);
		usleep_range(10, 20);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x1, 28, 1);
		usleep_range(10, 20);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x330004b9);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50440000);
		pr_info("HPLL: 0x%x\n", hd21_read_reg(ANACTRL_HDMIPLL_CTRL0));
		break;
	case 3712500:
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x0300049a);
		if (frac_rate)
			hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x110e1);
		else
			hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x16000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x01000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x40218000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x05501000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x00150500);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50450000);
		usleep_range(10, 20);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x1, 28, 1);
		usleep_range(10, 20);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x3300049a);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50440000);
		pr_info("HPLL: 0x%x\n", hd21_read_reg(ANACTRL_HDMIPLL_CTRL0));
		break;
	case 3485000:
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x03000491);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x6a80);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x01000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x40218000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x05501000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x00150500);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50450000);
		usleep_range(10, 20);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x1, 28, 1);
		usleep_range(10, 20);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x33000491);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50440000);
		pr_info("HPLL: 0x%x\n", hd21_read_reg(ANACTRL_HDMIPLL_CTRL0));
		break;
	case 3450000:
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x0300048f);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x18000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x01000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x40218000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x05501000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x00150500);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50450000);
		usleep_range(10, 20);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x1, 28, 1);
		usleep_range(10, 20);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x3300048f);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50440000);
		pr_info("HPLL: 0x%x\n", hd21_read_reg(ANACTRL_HDMIPLL_CTRL0));
		break;
	case 3243240:
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x03000487);
		if (frac_rate)
			hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x00000000);
		else
			hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x451f);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x01000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x40218000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x05501000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x00150500);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50450000);
		usleep_range(10, 20);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x1, 28, 1);
		usleep_range(10, 20);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x33000487);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50440000);
		pr_info("HPLL: 0x%x\n", hd21_read_reg(ANACTRL_HDMIPLL_CTRL0));
		break;
	case 3240000:
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x03000487);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x00000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x01000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x40218000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x05501000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x00150500);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50450000);
		usleep_range(10, 20);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x1, 28, 1);
		usleep_range(10, 20);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x33000487);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50440000);
		pr_info("HPLL: 0x%x\n", hd21_read_reg(ANACTRL_HDMIPLL_CTRL0));
		break;
	case 2970000:
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x0300047b);
		if (frac_rate)
			hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x140b4);
		else
			hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x18000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x01000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x40218000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x05501000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x00150500);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50450000);
		usleep_range(10, 20);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x1, 28, 1);
		usleep_range(10, 20);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x3300047b);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50440000);
		pr_info("HPLL: 0x%x\n", hd21_read_reg(ANACTRL_HDMIPLL_CTRL0));
		break;
	case 4324320:
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x030004b4);
		if (frac_rate)
			hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x00000000);
		else
			hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x00005c29);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x01000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x40218000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x05501000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x00150500);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50450000);
		usleep_range(10, 20);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x1, 28, 1);
		usleep_range(10, 20);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x330004b4);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50440000);
		pr_info("HPLL: 0x%x\n", hd21_read_reg(ANACTRL_HDMIPLL_CTRL0));
		break;
	case 4320000:
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x030004b4);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x00000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x01000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x40218000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x05501000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x00150500);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50450000);
		usleep_range(10, 20);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x1, 28, 1);
		usleep_range(10, 20);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x330004b4);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50440000);
		pr_info("HPLL: 0x%x\n", hd21_read_reg(ANACTRL_HDMIPLL_CTRL0));
		break;
	case 3180000:
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x03000484);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x00000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x01000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x40218000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x05501000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x00150500);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50450000);
		usleep_range(10, 20);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x1, 28, 1);
		usleep_range(10, 20);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x33000484);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50440000);
		pr_info("HPLL: 0x%x\n", hd21_read_reg(ANACTRL_HDMIPLL_CTRL0));
		break;
	case 3200000:
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x03000485);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0xaa80);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x01000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x40218000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x05501000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x00150500);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50450000);
		usleep_range(10, 20);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x1, 28, 1);
		usleep_range(10, 20);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x33000485);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50440000);
		pr_info("HPLL: 0x%x\n", hd21_read_reg(ANACTRL_HDMIPLL_CTRL0));
		break;
	case 3340000:
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x0300048b);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x5580);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x01000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x40218000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x05501000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x00150500);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50450000);
		usleep_range(10, 20);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x1, 28, 1);
		usleep_range(10, 20);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x3300048b);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50440000);
		pr_info("HPLL: 0x%x\n", hd21_read_reg(ANACTRL_HDMIPLL_CTRL0));
		break;
	case 3420000:
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x0300048e);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x10000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x01000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x40218000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x05501000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x00150500);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50450000);
		usleep_range(10, 20);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x1, 28, 1);
		usleep_range(10, 20);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x3300048e);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50440000);
		pr_info("HPLL: 0x%x\n", hd21_read_reg(ANACTRL_HDMIPLL_CTRL0));
		break;
	case 3865000:
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x030004a1);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x1580);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x01000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x40218000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x05501000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x00150500);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50450000);
		usleep_range(10, 20);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x1, 28, 1);
		usleep_range(10, 20);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x330004a1);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50440000);
		pr_info("HPLL: 0x%x\n", hd21_read_reg(ANACTRL_HDMIPLL_CTRL0));
		break;
	case 4028000:
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x030004a7);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x1aa80);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x01000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x40218000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x05501000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x00150500);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50450000);
		usleep_range(10, 20);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x1, 28, 1);
		usleep_range(10, 20);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x330004a7);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50440000);
		pr_info("HPLL: 0x%x\n", hd21_read_reg(ANACTRL_HDMIPLL_CTRL0));
		break;
	case 4115866:
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x030004a8);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0xfd00);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x01000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x40218000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x05501000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x00150500);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50450000);
		usleep_range(10, 20);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x1, 28, 1);
		usleep_range(10, 20);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x330004a8);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50440000);
		pr_info("HPLL: 0x%x\n", hd21_read_reg(ANACTRL_HDMIPLL_CTRL0));
		break;
	case 4260000:
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x030004b1);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x10000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x01000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x40218000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x05501000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x00150500);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50450000);
		usleep_range(10, 20);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x1, 28, 1);
		usleep_range(10, 20);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x330004b1);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50440000);
		pr_info("HPLL: 0x%x\n", hd21_read_reg(ANACTRL_HDMIPLL_CTRL0));
		break;
	case 4761600:
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x030004c6);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0xcd00);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x01000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x40218000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x05501000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x00150500);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50450000);
		usleep_range(10, 20);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x1, 28, 1);
		usleep_range(10, 20);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x330004c6);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50440000);
		pr_info("HPLL: 0x%x\n", hd21_read_reg(ANACTRL_HDMIPLL_CTRL0));
		break;
	case 4838400:
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x030004c9);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x9980);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x01000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x40218000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x05501000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x00150500);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50450000);
		usleep_range(10, 20);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x1, 28, 1);
		usleep_range(10, 20);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x330004c9);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50440000);
		pr_info("HPLL: 0x%x\n", hd21_read_reg(ANACTRL_HDMIPLL_CTRL0));
		break;
	case 4897000:
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x030004cc);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x1580);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x01000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x40218000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x05501000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x00150500);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50450000);
		usleep_range(10, 20);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x1, 28, 1);
		usleep_range(10, 20);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x330004cc);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50440000);
		pr_info("HPLL: 0x%x\n", hd21_read_reg(ANACTRL_HDMIPLL_CTRL0));
		break;
	case 5371100:
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x030004df);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x19780);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x01000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x40218000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x05501000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x00150500);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50450000);
		usleep_range(10, 20);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x1, 28, 1);
		usleep_range(10, 20);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x330004df);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50440000);
		pr_info("HPLL: 0x%x\n", hd21_read_reg(ANACTRL_HDMIPLL_CTRL0));
		break;
	case 5600000:
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x030004e9);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0xaaab);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x01000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x40218000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x05501000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x00150500);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50450000);
		usleep_range(10, 20);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x1, 28, 1);
		usleep_range(10, 20);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x330004e9);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50440000);
		pr_info("HPLL: 0x%x\n", hd21_read_reg(ANACTRL_HDMIPLL_CTRL0));
		break;
	case 5850000:
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x030004f3);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x18000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x01000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x40218000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x05501000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x00150500);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50450000);
		usleep_range(10, 20);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x1, 28, 1);
		usleep_range(10, 20);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x330004f3);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50440000);
		pr_info("HPLL: 0x%x\n", hd21_read_reg(ANACTRL_HDMIPLL_CTRL0));
		break;
	default:
		pr_info("error hpll clk: %d\n", clk);
		break;
	}
}

void set21_phy_by_mode_s1a(u32 mode)
{
	switch (mode) {
	case HDMI_PHYPARA_270M: /* SD format, 480p/576p, 270Mbps*/
		hd21_write_reg(ANACTRL_HDMIPHY_CTRL5, 0x564);
		hd21_write_reg(ANACTRL_HDMIPHY_CTRL0, 0x9fe36263);
		hd21_write_reg(ANACTRL_HDMIPHY_CTRL3, 0x5af6fc1b);
		break;
	case HDMI_PHYPARA_LT3G: /* 1.485Gbps */
	default:
		hd21_write_reg(ANACTRL_HDMIPHY_CTRL5, 0x564);
		hd21_write_reg(ANACTRL_HDMIPHY_CTRL0, 0x9fe36284);
		hd21_write_reg(ANACTRL_HDMIPHY_CTRL3, 0x5af6fc1b);
		break;
	}
}
