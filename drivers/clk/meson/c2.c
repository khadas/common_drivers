// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/init.h>
#include <linux/of_device.h>
#include <linux/mfd/syscon.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/clkdev.h>

#include "clk-dualdiv.h"
#include "clk-cpu-dyndiv.h"
#include "clk-pll.h"
#include "clk-regmap.h"
#include "c2.h"
/*
 * GATE for c2
 * its parent clock is sys clock, the same the
 * clk81 in previos SoC
 */
#define MESON_C2_SYS_GATE(_name, _reg, _bit)				\
struct clk_regmap _name = {						\
	.data = &(struct clk_regmap_gate_data){				\
		.offset = (_reg),					\
		.bit_idx = (_bit),					\
	},								\
	.hw.init = &(struct clk_init_data) {				\
		.name = #_name,						\
		.ops = &clk_regmap_gate_ops,				\
		.parent_names = (const char *[]){ "sys_clk" },		\
		.num_parents = 1,					\
		.flags = CLK_IGNORE_UNUSED,				\
	},								\
}

#define MESON_C2_AXI_GATE(_name, _reg, _bit)				\
struct clk_regmap _name = {						\
	.data = &(struct clk_regmap_gate_data){				\
		.offset = (_reg),					\
		.bit_idx = (_bit),					\
	},								\
	.hw.init = &(struct clk_init_data) {				\
		.name = #_name,						\
		.ops = &clk_regmap_gate_ops,				\
		.parent_names = (const char *[]){ "axi_clk" },		\
		.num_parents = 1,					\
		.flags = CLK_IGNORE_UNUSED,				\
	},								\
}

#define MESON_C2_XTAL_GATE(_name, _reg, _bit)				\
struct clk_regmap _name = {						\
	.data = &(struct clk_regmap_gate_data){				\
		.offset = (_reg),					\
		.bit_idx = (_bit),					\
	},								\
	.hw.init = &(struct clk_init_data) {				\
		.name = #_name,						\
		.ops = &clk_regmap_gate_ops,				\
		.parent_names = (const char *[]){ "xtal" },	\
		.num_parents = 1,					\
		.flags = CLK_IS_CRITICAL,		\
	},								\
}

/* PLL clock in gates,its parent is xtal */
/* CLKTREE_SYS_OSCIN_CTRL */
static MESON_C2_XTAL_GATE(xtal_clktree,		SYS_OSCIN_CTRL,	0);
static MESON_C2_XTAL_GATE(xtal_fixpll,		SYS_OSCIN_CTRL,	1);
static MESON_C2_XTAL_GATE(xtal_ddrpll,		SYS_OSCIN_CTRL,	2);
static MESON_C2_XTAL_GATE(xtal_usb_ctrl,	SYS_OSCIN_CTRL,	3);
static MESON_C2_XTAL_GATE(xtal_hifipll,		SYS_OSCIN_CTRL,	4);
static MESON_C2_XTAL_GATE(xtal_syspll,		SYS_OSCIN_CTRL,	5);
static MESON_C2_XTAL_GATE(xtal_dds,		SYS_OSCIN_CTRL,	6);
static MESON_C2_XTAL_GATE(xtal_ethpll,		SYS_OSCIN_CTRL,	7);
static MESON_C2_XTAL_GATE(xtal_usbphy,		SYS_OSCIN_CTRL,	8);
static MESON_C2_XTAL_GATE(xtal_gppll,		SYS_OSCIN_CTRL,	9);
static MESON_C2_XTAL_GATE(xtal_gpio_m10,	SYS_OSCIN_CTRL,	10);
static MESON_C2_XTAL_GATE(xtal_gpio_m13,	SYS_OSCIN_CTRL,	11);

static MESON_C2_XTAL_GATE(xtal_pad_ds0,		SYS_OSCIN_CTRL,	16);
static MESON_C2_XTAL_GATE(xtal_pad_ds1,		SYS_OSCIN_CTRL,	17);

/* Everything Else (EE) domain gates */
/* CLKTREE_SYS_CLK_EN0 */
static MESON_C2_SYS_GATE(clk_tree,		SYS_CLK_EN0, 0);
static MESON_C2_SYS_GATE(reset_ctrl,		SYS_CLK_EN0, 1);
static MESON_C2_SYS_GATE(analog_ctrl,		SYS_CLK_EN0, 2);
static MESON_C2_SYS_GATE(pwr_ctrl,		SYS_CLK_EN0, 3);
static MESON_C2_SYS_GATE(pad_ctrl,		SYS_CLK_EN0, 4);
static MESON_C2_SYS_GATE(sys_ctrl,		SYS_CLK_EN0, 5);
static MESON_C2_SYS_GATE(temp_sensor,		SYS_CLK_EN0, 6);
static MESON_C2_SYS_GATE(am2axi_dev,		SYS_CLK_EN0, 7);
static MESON_C2_SYS_GATE(spicc_b,		SYS_CLK_EN0, 8);
static MESON_C2_SYS_GATE(spicc_a,		SYS_CLK_EN0, 9);
static MESON_C2_SYS_GATE(clk_msr,		SYS_CLK_EN0, 10);
static MESON_C2_SYS_GATE(audio,			SYS_CLK_EN0, 11);
static MESON_C2_SYS_GATE(jtag_ctrl,		SYS_CLK_EN0, 12);
static MESON_C2_SYS_GATE(saradc,		SYS_CLK_EN0, 13);
static MESON_C2_SYS_GATE(pwm_ef,		SYS_CLK_EN0, 14);
static MESON_C2_SYS_GATE(pwm_cd,		SYS_CLK_EN0, 15);
static MESON_C2_SYS_GATE(pwm_ab,		SYS_CLK_EN0, 16);
static MESON_C2_SYS_GATE(i2c_s,			SYS_CLK_EN0, 18);
static MESON_C2_SYS_GATE(ir_ctrl,		SYS_CLK_EN0, 19);
static MESON_C2_SYS_GATE(i2c_m_d,		SYS_CLK_EN0, 20);
static MESON_C2_SYS_GATE(i2c_m_c,		SYS_CLK_EN0, 21);
static MESON_C2_SYS_GATE(i2c_m_b,		SYS_CLK_EN0, 22);
static MESON_C2_SYS_GATE(i2c_m_a,		SYS_CLK_EN0, 23);
static MESON_C2_SYS_GATE(acodec,		SYS_CLK_EN0, 24);
static MESON_C2_SYS_GATE(otp,			SYS_CLK_EN0, 25);
static MESON_C2_SYS_GATE(sys_sd_emmc_a,		SYS_CLK_EN0, 26);
static MESON_C2_SYS_GATE(usb_phy,		SYS_CLK_EN0, 27);
static MESON_C2_SYS_GATE(usb_ctrl,		SYS_CLK_EN0, 28);
static MESON_C2_SYS_GATE(sys_dspa,		SYS_CLK_EN0, 30);
static MESON_C2_SYS_GATE(dma,			SYS_CLK_EN0, 31);

/* CLKTREE_SYS_CLK_EN1 */
static MESON_C2_SYS_GATE(irq_ctrl,		SYS_CLK_EN1, 0);
static MESON_C2_SYS_GATE(nic,			SYS_CLK_EN1, 1);
static MESON_C2_SYS_GATE(gic,			SYS_CLK_EN1, 2);
static MESON_C2_SYS_GATE(uart_c,		SYS_CLK_EN1, 3);
static MESON_C2_SYS_GATE(uart_b,		SYS_CLK_EN1, 4);
static MESON_C2_SYS_GATE(uart_a,		SYS_CLK_EN1, 5);
static MESON_C2_SYS_GATE(mmc,			SYS_CLK_EN1, 7);
static MESON_C2_SYS_GATE(rsa,			SYS_CLK_EN1, 8);
static MESON_C2_SYS_GATE(coresight,		SYS_CLK_EN1, 9);
static MESON_C2_SYS_GATE(csi_ph1,		SYS_CLK_EN1, 10);
static MESON_C2_SYS_GATE(csi_phy0,		SYS_CLK_EN1, 11);
static MESON_C2_SYS_GATE(mipi_isp,		SYS_CLK_EN1, 12);
static MESON_C2_SYS_GATE(csi_dig,		SYS_CLK_EN1, 13);
static MESON_C2_SYS_GATE(ge2d,			SYS_CLK_EN1, 14);
static MESON_C2_SYS_GATE(gdc,			SYS_CLK_EN1, 15);
static MESON_C2_SYS_GATE(dos_apb,		SYS_CLK_EN1, 16);
static MESON_C2_SYS_GATE(nna,			SYS_CLK_EN1, 17);
static MESON_C2_SYS_GATE(eth_mac,		SYS_CLK_EN1, 18);
static MESON_C2_SYS_GATE(eth_mac_ddr,		SYS_CLK_EN1, 19);
static MESON_C2_SYS_GATE(uart_e,		SYS_CLK_EN1, 20);
static MESON_C2_SYS_GATE(uart_d,		SYS_CLK_EN1, 21);
static MESON_C2_SYS_GATE(pwm_ij,		SYS_CLK_EN1, 22);
static MESON_C2_SYS_GATE(pwm_gh,		SYS_CLK_EN1, 23);
static MESON_C2_SYS_GATE(i2c_m_e,		SYS_CLK_EN1, 24);
static MESON_C2_SYS_GATE(sd_emmc_C,		SYS_CLK_EN1, 25);
static MESON_C2_SYS_GATE(sd_emmc_B,		SYS_CLK_EN1, 26);
static MESON_C2_SYS_GATE(rom,			SYS_CLK_EN1, 27);
static MESON_C2_SYS_GATE(spifc,			SYS_CLK_EN1, 28);
static MESON_C2_SYS_GATE(prod_i2c,		SYS_CLK_EN1, 29);
static MESON_C2_SYS_GATE(dos,			SYS_CLK_EN1, 30);
static MESON_C2_SYS_GATE(cpu_ctrl,		SYS_CLK_EN1, 31);

/* CLKTREE_SYS_CLK_EN2 */
static MESON_C2_SYS_GATE(sys_rama,		SYS_CLK_EN2, 2);
static MESON_C2_SYS_GATE(capu_secpu,		SYS_CLK_EN2, 3);
static MESON_C2_SYS_GATE(mailbox,		SYS_CLK_EN2, 4);

/* CLKTREE_AXI_CLK_EN */
static MESON_C2_AXI_GATE(axi_am2axi_vad,	AXI_CLK_EN, 0);
static MESON_C2_AXI_GATE(axi_audio_vad,		AXI_CLK_EN, 1);
static MESON_C2_AXI_GATE(axi_dmc,		AXI_CLK_EN, 3);
static MESON_C2_AXI_GATE(axi_rama,		AXI_CLK_EN, 6);
static MESON_C2_AXI_GATE(axi_nic,		AXI_CLK_EN, 8);
static MESON_C2_AXI_GATE(axi_dma,		AXI_CLK_EN, 9);
static MESON_C2_AXI_GATE(axi_nic_vad,		AXI_CLK_EN, 10);
static MESON_C2_AXI_GATE(axi_capu,		AXI_CLK_EN, 11);
//static MESON_C2_AXI_GATE(axi_ramc,		AXI_CLK_EN, 13);

/* fixed pll = 2000M
 *
 * fixed pll ----- fclk_div2 = 1000M
 *           |
 *           ----- fclk_div2.5 = 800M
 *           |
 *           ----- fclk_div3 = 666M
 *           |
 *           ----- fclk_div4 = 500M
 *           |
 *           ----- fclk_div5 = 400M
 *           |
 *           ----- fclk_div7 = 286M
 */
static struct clk_regmap c2_fixed_pll = {
	.data = &(struct meson_clk_pll_data){
		.en = {
			.reg_off = ANACTRL_FIXPLL_CTRL0,
			.shift   = 28,
			.width   = 1,
		},
		.m = {
			.reg_off = ANACTRL_FIXPLL_CTRL0,
			.shift   = 0,
			.width   = 8,
		},
		.n = {
			.reg_off = ANACTRL_FIXPLL_CTRL0,
			.shift   = 10,
			.width   = 5,
		},
		/*
		 * .od = {
		 *	.reg_off = ANACTRL_FIXPLL_CTRL0,
		 *	.shift   = 16,
		 *	.width   = 2,
		 * },
		 * .frac = {
		 *	.reg_off = ANACTRL_FIXPLL_CTRL1,
		 *	.shift   = 0,
		 *	.width   = 19,
		 * },
		 */
		.l = {
			.reg_off = ANACTRL_FIXPLL_CTRL0,
			.shift   = 31,
			.width   = 1,
		},
		.rst = {
			.reg_off = ANACTRL_FIXPLL_CTRL0,
			.shift   = 29,
			.width   = 1,
		},
	},
	.hw.init = &(struct clk_init_data){
		.name = "fixed_pll",
		.ops = &meson_clk_pll_ro_ops,
		.parent_names = (const char *[]){ "xtal_fixpll" },
		.num_parents = 1,
		.flags = CLK_IS_CRITICAL,
	},
};

static struct clk_fixed_factor c2_fclk50_div40 = {
	.mult = 1,
	.div = 40,
	.hw.init = &(struct clk_init_data){
		.name = "fixed_pll_clk50M_div40",
		.ops = &clk_fixed_factor_ops,
		.parent_names = (const char *[]){ "fixed_pll"},
		.num_parents = 1,
	},
};

static struct clk_regmap c2_fclk50M = {
	.data = &(struct clk_regmap_gate_data){
		.offset = ANACTRL_FIXPLL_CTRL2,
		.bit_idx = 31,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "fixed_pll_clk50M",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "fixed_pll_clk50M_div40" },
		.num_parents = 1,
	},
};

static struct clk_fixed_factor c2_fclk_div2_div = {
	.mult = 1,
	.div = 2,
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div2_div",
		.ops = &clk_fixed_factor_ops,
		.parent_names = (const char *[]){ "fixed_pll"},
		.num_parents = 1,
	},
};

static struct clk_regmap c2_fclk_div2 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = ANACTRL_FIXPLL_CTRL1,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div2",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "fclk_div2_div" },
		.num_parents = 1,
	/*
	 * add CLK_IS_CRITICAL flag to avoid being disabled by clk core
	 * or its children clocks.
	 */
		.flags = CLK_IS_CRITICAL,
	},
};

static struct clk_fixed_factor c2_fclk_div2p5_div = {
	.mult = 2,
	.div = 5,
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div2p5_div",
		.ops = &clk_fixed_factor_ops,
		.parent_names = (const char *[]){ "fixed_pll"},
		.num_parents = 1,
	},
};

static struct clk_regmap c2_fclk_div2p5 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = ANACTRL_FIXPLL_CTRL1,
		.bit_idx = 25,
	},
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div2p5",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "fclk_div2p5_div" },
		.num_parents = 1,
	/*
	 * add CLK_IS_CRITICAL flag to avoid being disabled by clk core
	 * or its children clocks.
	 */
		.flags = CLK_IS_CRITICAL,
	},
};

static struct clk_fixed_factor c2_fclk_div3_div = {
	.mult = 1,
	.div = 3,
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div3_div",
		.ops = &clk_fixed_factor_ops,
		.parent_names = (const char *[]){ "fixed_pll"},
		.num_parents = 1,
	},
};

static struct clk_regmap c2_fclk_div3 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = ANACTRL_FIXPLL_CTRL1,
		.bit_idx = 20,
	},
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div3",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "fclk_div3_div" },
		.num_parents = 1,
	/*
	 * add CLK_IS_CRITICAL flag to avoid being disabled by clk core
	 * its children clocks.
	 */
		.flags = CLK_IS_CRITICAL,
	},
};

static struct clk_fixed_factor c2_fclk_div4_div = {
	.mult = 1,
	.div = 4,
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div4_div",
		.ops = &clk_fixed_factor_ops,
		.parent_names = (const char *[]){ "fixed_pll"},
		.num_parents = 1,
	},
};

static struct clk_regmap c2_fclk_div4 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = ANACTRL_FIXPLL_CTRL1,
		.bit_idx = 21,
	},
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div4",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "fclk_div4_div" },
		.num_parents = 1,
	/*
	 * add CLK_IS_CRITICAL flag to avoid being disabled by clk core
	 * or its children clocks.
	 */
		.flags = CLK_IS_CRITICAL,
	},
};

static struct clk_fixed_factor c2_fclk_div5_div = {
	.mult = 1,
	.div = 5,
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div5_div",
		.ops = &clk_fixed_factor_ops,
		.parent_names = (const char *[]){ "fixed_pll"},
		.num_parents = 1,
	},
};

static struct clk_regmap c2_fclk_div5 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = ANACTRL_FIXPLL_CTRL1,
		.bit_idx = 22,
	},
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div5",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "fclk_div5_div" },
		.num_parents = 1,
	/*
	 * add CLK_IS_CRITICAL flag to avoid being disabled by clk core
	 * its children clocks.
	 */
		.flags = CLK_IS_CRITICAL,
	},
};

static struct clk_fixed_factor c2_fclk_div7_div = {
	.mult = 1,
	.div = 7,
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div7_div",
		.ops = &clk_fixed_factor_ops,
		.parent_names = (const char *[]){ "fixed_pll"},
		.num_parents = 1,
	},
};

static struct clk_regmap c2_fclk_div7 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = ANACTRL_FIXPLL_CTRL1,
		.bit_idx = 23,
	},
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div7",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "fclk_div7_div" },
		.num_parents = 1,
	/*
	 * add CLK_IS_CRITICAL flag to avoid being disabled
	 * by clk core or its children clock.
	 */
		.flags = CLK_IS_CRITICAL,
	},
};

/* gp Pll */
/* DCO = 768 - 1526M */
static const struct reg_sequence c2_gp_init_regs[] = {
	{ .reg = ANACTRL_GPPLL_CTRL0,	.def = 0x20040863 },
	{ .reg = ANACTRL_GPPLL_CTRL1,	.def = 0x802000b1 },
	{ .reg = ANACTRL_GPPLL_CTRL2,	.def = 0x11002320 },
	{ .reg = ANACTRL_GPPLL_CTRL3,	.def = 0xd0010000 },
	{ .reg = ANACTRL_GPPLL_CTRL4,   .def = 0x45004000 },
	{ .reg = ANACTRL_GPPLL_CTRL5,   .def = 0x001a001a },
	{ .reg = ANACTRL_GPPLL_CTRL6,   .def = 0x50b, .delay_us = 5 },
	{ .reg = ANACTRL_GPPLL_CTRL0,	.def = 0x30040863, .delay_us = 10 },
	{ .reg = ANACTRL_GPPLL_CTRL0,	.def = 0x10040863, .delay_us = 10 },
	{ .reg = ANACTRL_GPPLL_CTRL4,	.def = 0x45004001, .delay_us = 400 },
};

#ifdef CONFIG_ARM
static const struct pll_params_table c2_gp_pll_params_table[] = {
	PLL_PARAMS(99, 2, 0), /* DCO = 1188M*/
	{ /* sentinel */  },
};
#else
static const struct pll_params_table c2_gp_pll_params_table[] = {
	PLL_PARAMS(99, 2), /* DCO = 1188M */
	{ /* sentinel */  },
};
#endif

/*
 * gp pll = 1188M
 */
static struct clk_regmap c2_gp_pll_vco = {
	.data = &(struct meson_clk_pll_data){
		.en = {
			.reg_off = ANACTRL_GPPLL_CTRL0,
			.shift   = 28,
			.width   = 1,
		},
		.m = {
			.reg_off = ANACTRL_GPPLL_CTRL0,
			.shift   = 0,
			.width   = 8,
		},
		.n = {
			.reg_off = ANACTRL_GPPLL_CTRL0,
			.shift   = 10,
			.width   = 5,
		},
		/*
		 * .frac = {
		 *	.reg_off = ANACTRL_GPPLL_CTRL3,
		 *	.shift   = 0,
		 *	.width   = 19,
		 * },
		 */
		.l = {
			.reg_off = ANACTRL_GPPLL_STS,
			.shift   = 31,
			.width   = 1,
		},
		.table = c2_gp_pll_params_table,
		.init_regs = c2_gp_init_regs,
		.init_count = ARRAY_SIZE(c2_gp_init_regs),
	},
	.hw.init = &(struct clk_init_data){
		.name = "gp_pll_vco",
		.ops = &meson_clk_pll_v3_ops,
		.parent_names = (const char *[]){ "xtal_gppll" },
		.num_parents = 1,
	},
};

static struct clk_regmap c2_gp_pll_od1_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = ANACTRL_GPPLL_CTRL0,
		.shift = 18,
		.width = 2,
		.flags = CLK_DIVIDER_POWER_OF_TWO,
	},
	.hw.init = &(struct clk_init_data){
		.name = "gp_pll_od1_div",
		.ops = &clk_regmap_divider_ops,
		.parent_names = (const char *[]){ "gp_pll_vco" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap c2_gp_pll_gate1 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = ANACTRL_GPPLL_CTRL6,
		.bit_idx = 7,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "gp_pll_gate1",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "gp_pll_od1_div" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap c2_gp_pll_gate2 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = ANACTRL_GPPLL_CTRL6,
		.bit_idx = 15,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "gp_pll_gate2",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "gp_pll_od1_div" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap c2_gp_pll_gate3 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = ANACTRL_GPPLL_CTRL6,
		.bit_idx = 19,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "gp_pll_gate3",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "gp_pll_od1_div" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_div_table gp_pll_table[] = {
	{.val = 0, .div = 1},
	{.val = 1, .div = 2},
	{.val = 2, .div = 10},
	{.val = 3, .div = 11}
};

static struct clk_regmap c2_gp_pll = {
	.data = &(struct clk_regmap_div_data){
		.offset = ANACTRL_GPPLL_CTRL6,
		.shift = 16,
		.width = 2,
		.table = gp_pll_table,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "gp_pll",
		.ops = &clk_regmap_divider_ops,
		.parent_names = (const char *[]){ "gp_pll_gate3" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap c2_gp_pll_out1_od = {
	.data = &(struct clk_regmap_div_data){
		.offset = ANACTRL_GPPLL_CTRL6,
		.shift = 0,
		.width = 5,
		.flags = CLK_DIVIDER_ONE_BASED | CLK_DIVIDER_ALLOW_ZERO,
	},
	.hw.init = &(struct clk_init_data){
		.name = "gp_pll_out1_od",
		.ops = &clk_regmap_divider_ops,
		.parent_names = (const char *[]){ "gp_pll_gate1" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap c2_gp_pll_out2_od = {
	.data = &(struct clk_regmap_div_data){
		.offset = ANACTRL_GPPLL_CTRL6,
		.shift = 8,
		.width = 5,
		.flags = CLK_DIVIDER_ONE_BASED | CLK_DIVIDER_ALLOW_ZERO,
	},
	.hw.init = &(struct clk_init_data){
		.name = "gp_pll_out2_od",
		.ops = &clk_regmap_divider_ops,
		.parent_names = (const char *[]){ "gp_pll_gate2" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_fixed_factor c2_gp_pll_out1_div2_div = {
	.mult = 1,
	.div = 2,
	.hw.init = &(struct clk_init_data){
		.name = "gp_pll_out1",
		.ops = &clk_fixed_factor_ops,
		.parent_names = (const char *[]){ "gp_pll_out1_od" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_fixed_factor c2_gp_pll_out2_div2_div = {
	.mult = 1,
	.div = 2,
	.hw.init = &(struct clk_init_data){
		.name = "gp_pll_out2",
		.ops = &clk_fixed_factor_ops,
		.parent_names = (const char *[]){ "gp_pll_out2_od" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static u32 mux_gp_pll_mclk_table[] = {1, 2, 3};

static struct clk_regmap c2_gp_pll_mclk1_mux = {
	.data = &(struct clk_regmap_mux_data){
		.offset = ANACTRL_GPPLL_CTRL5,
		/*gp_pre_driver1 4bit but only use 2bit*/
		.mask = 0x3,
		.shift = 8,
		.table = mux_gp_pll_mclk_table,
	},
	.hw.init = &(struct clk_init_data){
		.name = "gp_pll_mclk1_mux",
		.ops = &clk_regmap_mux_ops,
		.parent_names = (const char *[]){ "gp_pll_out1",
						"xtal",
						"fixed_pll_clk50M" },
		.num_parents = 3,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap c2_gp_pll_mclk1_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = ANACTRL_GPPLL_CTRL5,
		.shift = 10,
		.width = 1,
		.flags = CLK_DIVIDER_POWER_OF_TWO,
	},
	.hw.init = &(struct clk_init_data){
		.name = "gp_pll_mclk1_div",
		.ops = &clk_regmap_divider_ops,
		.parent_names = (const char *[]){ "gp_pll_mclk1_mux" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap c2_gp_pll_mclk1_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = ANACTRL_GPPLL_CTRL6,
		.bit_idx = 23,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "gp_pll_mclk1",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "gp_pll_mclk1_div" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap c2_gp_pll_mclk2_mux = {
	.data = &(struct clk_regmap_mux_data){
		.offset = ANACTRL_GPPLL_CTRL5,
		/*gp_pre_driver2 4bit but only use 2bit*/
		.mask = 0x3,
		.shift = 24,
		.table = mux_gp_pll_mclk_table,
	},
	.hw.init = &(struct clk_init_data){
		.name = "gp_pll_mclk2_mux",
		.ops = &clk_regmap_mux_ops,
		.parent_names = (const char *[]){ "gp_pll_out2",
						"xtal",
						"fixed_pll_clk50M" },
		.num_parents = 3,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap c2_gp_pll_mclk2_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = ANACTRL_GPPLL_CTRL5,
		.shift = 26,
		.width = 1,
		.flags = CLK_DIVIDER_POWER_OF_TWO,
	},
	.hw.init = &(struct clk_init_data){
		.name = "gp_pll_mclk2_div",
		.ops = &clk_regmap_divider_ops,
		.parent_names = (const char *[]){ "gp_pll_mclk2_mux" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap c2_gp_pll_mclk2_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = ANACTRL_GPPLL_CTRL6,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "gp_pll_mclk2",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "gp_pll_mclk2_div" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

/* hifi pll */
/* pll 1536M */
static const struct reg_sequence c2_hifi_init_regs[] = {
	{ .reg = ANACTRL_HIFIPLL_CTRL0,	.def = 0x20000432 },
	{ .reg = ANACTRL_HIFIPLL_CTRL1,	.def = 0x802000b1 },
	{ .reg = ANACTRL_HIFIPLL_CTRL2,	.def = 0x15002320 },
	{ .reg = ANACTRL_HIFIPLL_CTRL3,	.def = 0xf0005555 },
	{ .reg = ANACTRL_HIFIPLL_CTRL4, .def = 0x45004000, .delay_us = 5 },
	{ .reg = ANACTRL_HIFIPLL_CTRL0, .def = 0x30000432, .delay_us = 5 },
	{ .reg = ANACTRL_HIFIPLL_CTRL0,	.def = 0x10000432, .delay_us = 10 },
	{ .reg = ANACTRL_HIFIPLL_CTRL4,	.def = 0x45004001, .delay_us = 400 },
};

#ifdef CONFIG_ARM
static const struct pll_params_table c2_hifi_pll_params_table[] = {
	PLL_PARAMS(50, 1, 0), /* DCO = 1536 */
	PLL_PARAMS(48, 1, 0), /* DCO = 1155.072 */
	{ /* sentinel */  },
};
#else
static const struct pll_params_table c2_hifi_pll_params_table[] = {
	PLL_PARAMS(50, 1), /* DCO = 1536M */
	PLL_PARAMS(48, 1), /* DCO = 1155.072 */
	{ /* sentinel */  },
};
#endif

/*
 * hifi vco = 768M - 1536M
 * hifi pll: 12M - 1536M
 */
static struct clk_regmap c2_hifi_pll = {
	.data = &(struct meson_clk_pll_data){
		.en = {
			.reg_off = ANACTRL_HIFIPLL_CTRL0,
			.shift   = 28,
			.width   = 1,
		},
		.m = {
			.reg_off = ANACTRL_HIFIPLL_CTRL0,
			.shift   = 0,
			.width   = 8,
		},
		.n = {
			.reg_off = ANACTRL_HIFIPLL_CTRL0,
			.shift   = 10,
			.width   = 5,
		},
		.frac = {
			.reg_off = ANACTRL_HIFIPLL_CTRL3,
			.shift   = 0,
			.width   = 19,
		},
		.l = {
			.reg_off = ANACTRL_HIFIPLL_STS,
			.shift   = 31,
			.width   = 1,
		},
		.table = c2_hifi_pll_params_table,
		.init_regs = c2_hifi_init_regs,
		.init_count = ARRAY_SIZE(c2_hifi_init_regs),
	},
	.hw.init = &(struct clk_init_data){
		.name = "hifi_pll",
		.ops = &meson_clk_pll_v3_ops,
		.parent_names = (const char *[]){ "xtal_hifipll" },
		.num_parents = 1,
		.flags = CLK_IGNORE_UNUSED,
	},
};

/*
 * Internal sys pll emulation configuration parameters
 */
static const struct reg_sequence c2_sys_init_regs[] = {
	{ .reg = ANACTRL_SYSPLL_CTRL0,	.def = 0x21000460 },
	{ .reg = ANACTRL_SYSPLL_CTRL1,	.def = 0x00021092 },
	{ .reg = ANACTRL_SYSPLL_CTRL2,	.def = 0x09023302 },
	{ .reg = ANACTRL_SYSPLL_CTRL3,	.def = 0x02008000 },
	{ .reg = ANACTRL_SYSPLL_CTRL0,  .def = 0x31000460, .delay_us = 5 },
	{ .reg = ANACTRL_SYSPLL_CTRL0,	.def = 0x11000460, .delay_us = 20 },
	{ .reg = ANACTRL_SYSPLL_CTRL0,	.def = 0x18100460, .delay_us = 400 },
};

#ifdef CONFIG_ARM
static const struct clk_ops meson_pll_clk_no_ops = {};

static const struct pll_params_table c2_pll_params_table[] = {
	PLL_PARAMS(133, 1, 1), /* VCO = 3192M*/
	PLL_PARAMS(126, 1, 1), /* VCO = 3024M*/
	PLL_PARAMS(125, 1, 1), /* VCO = 3000M*/
	PLL_PARAMS(117, 1, 1), /* VCO = 2808M*/
	PLL_PARAMS(100, 1, 1), /* VCO = 2400M*/
	PLL_PARAMS(96, 1, 0), /* VCO = 2304M */
	PLL_PARAMS(88, 1, 0), /* VCO = 2112M */
	PLL_PARAMS(84, 1, 0), /* VCO = 2016M */
	PLL_PARAMS(80, 1, 0), /* VCO = 1920 */
	PLL_PARAMS(75, 1, 0), /* VCO = 1800M*/
	PLL_PARAMS(71, 1, 0), /* VCO = 1704M*/
	PLL_PARAMS(67, 1, 0), /* VCO = 1608M */
	{ /* sentinel */ }
};
#else
static const struct pll_params_table c2_pll_params_table[] = {
	PLL_PARAMS(133, 1), /* VCO = 3192M*/
	PLL_PARAMS(126, 1), /* VCO = 3024M*/
	PLL_PARAMS(125, 1), /* VCO = 3000M*/
	PLL_PARAMS(117, 1), /* VCO = 2808M*/
	PLL_PARAMS(100, 1), /* VCO = 2400M */
	PLL_PARAMS(96, 1), /* VCO = 2304M*/
	PLL_PARAMS(88, 1), /* VCO = 2112M */
	PLL_PARAMS(84, 1), /* VCO = 2016M */
	PLL_PARAMS(80, 1), /* VCO = 1920 */
	PLL_PARAMS(75, 1), /* VCO = 1800M*/
	PLL_PARAMS(71, 1), /* VCO = 1704M*/
	PLL_PARAMS(67, 1), /* VCO = 1608M */
	{ /* sentinel */ }
};
#endif
/*
 * sys pll = 25M ~ 3200M
 */
static struct clk_regmap c2_sys_pll_vco = {
	.data = &(struct meson_clk_pll_data){
		.en = {
			.reg_off = ANACTRL_SYSPLL_CTRL0,
			.shift   = 28,
			.width   = 1,
		},
		.m = {
			.reg_off = ANACTRL_SYSPLL_CTRL0,
			.shift   = 0,
			.width   = 8,
		},
		.n = {
			.reg_off = ANACTRL_SYSPLL_CTRL0,
			.shift   = 10,
			.width   = 5,
		},
#ifdef CONFIG_ARM
		/* od for 32bit */
		.od = {
			.reg_off = ANACTRL_SYSPLL_CTRL0,
			.shift   = 20,
			.width   = 2,
		},
#endif
		.l = {
			.reg_off = ANACTRL_SYSPLL_STS,
			.shift   = 31,
			.width   = 1,
		},
		.table = c2_pll_params_table,
		.init_regs = c2_sys_init_regs,
		.init_count = ARRAY_SIZE(c2_sys_init_regs),
		.flags = CLK_MESON_PLL_IGNORE_INIT,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sys_pll_vco",
		.ops = &meson_clk_pll_v3_ops,
		.parent_names = (const char *[]){ "xtal_syspll" },
		.num_parents = 1,
		.flags = CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap c2_sys_pll_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = ANACTRL_SYSPLL_CTRL3,
		.bit_idx = 15,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "sys_pll_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "sys_pll_vco" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

#ifdef CONFIG_ARM
static struct clk_regmap c2_sys_pll = {
	.hw.init = &(struct clk_init_data){
		.name = "sys_pll",
		.ops = &meson_pll_clk_no_ops,
		.parent_names = (const char *[]){ "sys_pll_gate" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};
#else
static struct clk_regmap c2_sys_pll = {
	.data = &(struct clk_regmap_div_data){
		.offset = ANACTRL_SYSPLL_CTRL0,
		.shift = 20,
		.width = 2,
		.flags = CLK_DIVIDER_POWER_OF_TWO,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sys_pll",
		.ops = &clk_regmap_divider_ops,
		.parent_names = (const char *[]){ "sys_pll_gate" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};
#endif

/* sys clk = 64M */
static u32 mux_table_sys_ab_clk_sel[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
static const char * const sys_ab_clk_parent_names[] = {
	"xtal", "fclk_div2", "fclk_div3", "fclk_div5",
	"fclk_div4", "axi_clk_frcpu", "fclk_div7", "rtc_clk"
};

static struct clk_regmap c2_sys_b_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = SYS_CLK_CTRL0,
		.mask = 0x7,
		.shift = 26,
		.table = mux_table_sys_ab_clk_sel,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sys_b_sel",
		.ops = &clk_regmap_mux_ro_ops,
		.parent_names = sys_ab_clk_parent_names,
		.num_parents = ARRAY_SIZE(sys_ab_clk_parent_names),
	},
};

static struct clk_regmap c2_sys_b_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = SYS_CLK_CTRL0,
		.shift = 16,
		.width = 10,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sys_b_div",
		.ops = &clk_regmap_divider_ops,
		.parent_names = (const char *[]){ "sys_b_sel" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap c2_sys_b = {
	.data = &(struct clk_regmap_gate_data){
		.offset = SYS_CLK_CTRL0,
		.bit_idx = 29,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "sys_b",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "sys_b_div" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED
			 | CLK_IS_CRITICAL,
	},
};

static struct clk_regmap c2_sys_a_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = SYS_CLK_CTRL0,
		.mask = 0x7,
		.shift = 10,
		.table = mux_table_sys_ab_clk_sel,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sys_a_sel",
		.ops = &clk_regmap_mux_ro_ops,
		.parent_names = sys_ab_clk_parent_names,
		.num_parents = ARRAY_SIZE(sys_ab_clk_parent_names),
	},
};

static struct clk_regmap c2_sys_a_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = SYS_CLK_CTRL0,
		.shift = 0,
		.width = 10,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sys_a_div",
		.ops = &clk_regmap_divider_ops,
		.parent_names = (const char *[]){ "sys_a_sel" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap c2_sys_a = {
	.data = &(struct clk_regmap_gate_data){
		.offset = SYS_CLK_CTRL0,
		.bit_idx = 13,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "sys_a",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "sys_a_div" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED
			 | CLK_IS_CRITICAL,
	},
};

static const char * const sys_clk_parent_names[] = {
	"sys_a", "sys_b"};

static struct clk_regmap c2_sys_clk = {
	.data = &(struct clk_regmap_mux_data){
		.offset = SYS_CLK_CTRL0,
		.mask = 0x1,
		.shift = 31,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sys_clk",
		.ops = &clk_regmap_mux_ro_ops,
		.parent_names = sys_clk_parent_names,
		.num_parents = ARRAY_SIZE(sys_clk_parent_names),
		.flags = CLK_SET_RATE_PARENT,
	},
};

/* axi */
static const char * const axi_ab_clk_parent_names[] = {
	"xtal", "fclk_div2", "fclk_div3", "fclk_div5",
	"fclk_div4", "axi_clk_frcpu", "fclk_div7", "rtc_clk"
};

static struct clk_regmap axi_b_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = AXI_CLK_CTRL0,
		.mask = 0x7,
		.shift = 26,
	},
	.hw.init = &(struct clk_init_data){
		.name = "axi_b_sel",
		.ops = &clk_regmap_mux_ro_ops,
		.parent_names = axi_ab_clk_parent_names,
		.num_parents = ARRAY_SIZE(axi_ab_clk_parent_names),
	},
};

static struct clk_regmap axi_b_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = AXI_CLK_CTRL0,
		.shift = 16,
		.width = 10,
	},
	.hw.init = &(struct clk_init_data){
		.name = "axi_b_div",
		.ops = &clk_regmap_divider_ops,
		.parent_names = (const char *[]){ "axi_b_sel" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap axi_b = {
	.data = &(struct clk_regmap_gate_data){
		.offset = AXI_CLK_CTRL0,
		.bit_idx = 29,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "axi_b",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "axi_b_div" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap axi_a_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = AXI_CLK_CTRL0,
		.mask = 0x7,
		.shift = 10,
	},
	.hw.init = &(struct clk_init_data){
		.name = "axi_a_sel",
		.ops = &clk_regmap_mux_ro_ops,
		.parent_names = axi_ab_clk_parent_names,
		.num_parents = ARRAY_SIZE(axi_ab_clk_parent_names),
	},
};

static struct clk_regmap axi_a_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = AXI_CLK_CTRL0,
		.shift = 0,
		.width = 10,
	},
	.hw.init = &(struct clk_init_data){
		.name = "axi_a_div",
		.ops = &clk_regmap_divider_ops,
		.parent_names = (const char *[]){ "axi_a_sel" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap axi_a = {
	.data = &(struct clk_regmap_gate_data){
		.offset = AXI_CLK_CTRL0,
		.bit_idx = 13,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "axi_a",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "axi_a_div" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static const char * const axi_clk_parent_names[] = {
	"axi_a", "axi_b"};

static struct clk_regmap axi_clk = {
	.data = &(struct clk_regmap_mux_data){
		.offset = AXI_CLK_CTRL0,
		.mask = 0x1,
		.shift = 31,
	},
	.hw.init = &(struct clk_init_data){
		.name = "axi_clk",
		.ops = &clk_regmap_mux_ro_ops,
		.parent_names = axi_clk_parent_names,
		.num_parents = ARRAY_SIZE(axi_clk_parent_names),
		.flags = CLK_SET_RATE_PARENT,
	},
};

/* dspa clk */
static const char * const dspab_parent_names[] = {
	"xtal", "fclk_div2", "fclk_div3", "fclk_div5",
	"hifi_pll", "fclk_div4", "fclk_div7", "rtc_clk"
};

static struct clk_regmap c2_dspa_a_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = DSPA_CLK_CTRL0,
		.mask = 0x7,
		.shift = 10,
	},
	.hw.init = &(struct clk_init_data){
		.name = "dspa_a_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_names = dspab_parent_names,
		.num_parents = ARRAY_SIZE(dspab_parent_names),
	},
};

static struct clk_regmap c2_dspa_a_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = DSPA_CLK_CTRL0,
		.shift = 0,
		.width = 10,
	},
	.hw.init = &(struct clk_init_data){
		.name = "dspa_a_div",
		.ops = &clk_regmap_divider_ops,
		.parent_names = (const char *[]){ "dspa_a_sel" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap c2_dspa_a = {
	.data = &(struct clk_regmap_gate_data){
		.offset = DSPA_CLK_CTRL0,
		.bit_idx = 13,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "dspa_a",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "dspa_a_div" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap c2_dspa_b_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = DSPA_CLK_CTRL0,
		.mask = 0x7,
		.shift = 26,
	},
	.hw.init = &(struct clk_init_data){
		.name = "dspa_b_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_names = dspab_parent_names,
		.num_parents = ARRAY_SIZE(dspab_parent_names),
	},
};

static struct clk_regmap c2_dspa_b_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = DSPA_CLK_CTRL0,
		.shift = 16,
		.width = 10,
	},
	.hw.init = &(struct clk_init_data){
		.name = "dspa_b_div",
		.ops = &clk_regmap_divider_ops,
		.parent_names = (const char *[]){ "dspa_b_sel" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap c2_dspa_b = {
	.data = &(struct clk_regmap_gate_data){
		.offset = DSPA_CLK_CTRL0,
		.bit_idx = 29,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "dspa_b",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "dspa_b_div" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static const char * const dspa_parent_names[] = {
	"dspa_a", "dspa_b"
};

static struct clk_regmap c2_dspa_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = DSPA_CLK_CTRL0,
		.mask = 0x1,
		.shift = 15,
	},
	.hw.init = &(struct clk_init_data){
		.name = "dspa_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_names = dspa_parent_names,
		.num_parents = ARRAY_SIZE(dspa_parent_names),
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap c2_dspa_en_dspa = {
	.data = &(struct clk_regmap_gate_data){
		.offset = DSPA_CLK_EN,
		.bit_idx = 1,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "dspa_en_dspa",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "dspa_sel" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap c2_dspa_en_nic = {
	.data = &(struct clk_regmap_gate_data){
		.offset = DSPA_CLK_EN,
		.bit_idx = 0,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "dspa_en_nic",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "dspa_sel" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

/* 12M/24M clock */
static struct clk_regmap c2_24m = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLK12_24_CTRL,
		.bit_idx = 11,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "24m",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "xtal" },
		.num_parents = 1,
	},
};

static struct clk_fixed_factor c2_24m_div2 = {
	.mult = 1,
	.div = 2,
	.hw.init = &(struct clk_init_data){
		.name = "24m_div2",
		.ops = &clk_fixed_factor_ops,
		.parent_names = (const char *[]){ "xtal" },
		.num_parents = 1,
	},
};

static struct clk_regmap c2_12m = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLK12_24_CTRL,
		.bit_idx = 10,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "12m",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "24m_div2" },
		.num_parents = 1,
	},
};

static struct clk_regmap c2_fclk_div2_divn_pre = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLK12_24_CTRL,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div2_divn_pre",
		.ops = &clk_regmap_divider_ops,
		.parent_names = (const char *[]){ "fclk_div2" },
		.num_parents = 1,
	},
};

static struct clk_regmap c2_fclk_div2_divn = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLK12_24_CTRL,
		.bit_idx = 12,
	},
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div2_divn",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "fclk_div2_divn_pre" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

/* gen clk */
static const char * const gen_parent_names[] = {
	"xtal", "rtc_clk", "sys_pll_div16", "hifi",
	"clk_msr_src", "fclk_div2", "fclk_div3", "fclk_div5",
	"fclk_div7", "gp_pll", "fclk_div2p5", "fclk_div4",
	"sys_clk", "axi_clk"
};

static struct clk_regmap c2_gen_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = GEN_CLK_CTRL,
		.mask = 0xf,
		.shift = 12,
	},
	.hw.init = &(struct clk_init_data){
		.name = "gen_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_names = gen_parent_names,
		.num_parents = ARRAY_SIZE(gen_parent_names),
	},
};

static struct clk_regmap c2_gen_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = GEN_CLK_CTRL,
		.shift = 0,
		.width = 11,
	},
	.hw.init = &(struct clk_init_data){
		.name = "gen_div",
		.ops = &clk_regmap_divider_ops,
		.parent_names = (const char *[]){ "gen_sel" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap c2_gen = {
	.data = &(struct clk_regmap_gate_data){
		.offset = GEN_CLK_CTRL,
		.bit_idx = 11,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "gen",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "gen_div" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

/* saradc */
static const char * const saradc_parent_names[] = {
	"xtal", "sys_clk"
};

static struct clk_regmap c2_saradc_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = SAR_ADC_CLK_CTRL,
		/*
		 * bit9 and bit10 to select parent
		 * no clock for third and forth parent
		 * So mask is 0x1.
		 */
		.mask = 0x1,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "saradc_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_names = saradc_parent_names,
		.num_parents = ARRAY_SIZE(saradc_parent_names),
	},
};

static struct clk_regmap c2_saradc_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = SAR_ADC_CLK_CTRL,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "saradc_div",
		.ops = &clk_regmap_divider_ops,
		.parent_names = (const char *[]){ "saradc_sel" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap c2_saradc_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = SAR_ADC_CLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "saradc_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "saradc_div" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

/* pwm */
static const char * const pwm_parent_names[] = {
	"xtal", "sys_clk", "rtc_clk", "fclk_div4"
};

/*
 * add CLK_IGNORE_UNUSED flag for pwm controller GATE
 * clk core will disable unused clock, it may disable
 * vddcore voltage which controlled by one pwm in bl21.
 * add the flag to avoid changing cpu voltage.
 */
#define MESON_C2_PWM_SEL(_name, _reg, _mask, _shift)			\
struct clk_regmap _name = {						\
	.data = &(struct clk_regmap_mux_data) {				\
		.offset = (_reg),					\
		.mask = (_mask),					\
		.shift = (_shift),					\
	},								\
	.hw.init = &(struct clk_init_data) {				\
		.name = #_name,						\
		.ops = &clk_regmap_mux_ops,				\
		.parent_names = pwm_parent_names,			\
		.num_parents =  ARRAY_SIZE(pwm_parent_names),		\
	},								\
}

#define MESON_C2_PWM_DIV(_name, _reg, _shift, _width, _parent) \
struct clk_regmap _name = {						\
	.data = &(struct clk_regmap_div_data) {				\
		.offset = (_reg),					\
		.shift = (_shift),					\
		.width = (_width),					\
	},								\
	.hw.init = &(struct clk_init_data) {				\
		.name = #_name,						\
		.ops = &clk_regmap_divider_ops,				\
		.parent_names = (const char *[]){ #_parent }, \
		.num_parents =  1,		\
		.flags = CLK_SET_RATE_PARENT,				\
	},								\
}

#define MESON_C2_PWM_GATE(_name, _reg, _bit, _parent)			\
struct clk_regmap _name = {						\
	.data = &(struct clk_regmap_gate_data){				\
		.offset = (_reg),					\
		.bit_idx = (_bit),					\
	},								\
	.hw.init = &(struct clk_init_data) {				\
		.name = #_name,						\
		.ops = &clk_regmap_gate_ops,				\
		.parent_names = (const char *[]){ #_parent },		\
		.num_parents = 1,					\
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,	\
	},								\
}

MESON_C2_PWM_SEL(c2_pwm_a_sel, PWM_CLK_AB_CTRL, 0x3, 9);
MESON_C2_PWM_DIV(c2_pwm_a_div, PWM_CLK_AB_CTRL, 0, 8, c2_pwm_a_sel);
MESON_C2_PWM_GATE(c2_pwm_a, PWM_CLK_AB_CTRL, 8, c2_pwm_a_div);
MESON_C2_PWM_SEL(c2_pwm_b_sel, PWM_CLK_AB_CTRL, 0x3, 25);
MESON_C2_PWM_DIV(c2_pwm_b_div, PWM_CLK_AB_CTRL, 16, 8, c2_pwm_b_sel);
MESON_C2_PWM_GATE(c2_pwm_b, PWM_CLK_AB_CTRL, 24, c2_pwm_b_div);

MESON_C2_PWM_SEL(c2_pwm_c_sel, PWM_CLK_CD_CTRL, 0x3, 9);
MESON_C2_PWM_DIV(c2_pwm_c_div, PWM_CLK_CD_CTRL, 0, 8, c2_pwm_c_sel);
MESON_C2_PWM_GATE(c2_pwm_c, PWM_CLK_CD_CTRL, 8, c2_pwm_c_div);
MESON_C2_PWM_SEL(c2_pwm_d_sel, PWM_CLK_CD_CTRL, 0x3, 25);
MESON_C2_PWM_DIV(c2_pwm_d_div, PWM_CLK_CD_CTRL, 16, 8, c2_pwm_d_sel);
MESON_C2_PWM_GATE(c2_pwm_d, PWM_CLK_CD_CTRL, 24, c2_pwm_d_div);

MESON_C2_PWM_SEL(c2_pwm_e_sel, PWM_CLK_EF_CTRL, 0x3, 9);
MESON_C2_PWM_DIV(c2_pwm_e_div, PWM_CLK_EF_CTRL, 0, 8, c2_pwm_e_sel);
MESON_C2_PWM_GATE(c2_pwm_e, PWM_CLK_EF_CTRL, 8, c2_pwm_e_div);
MESON_C2_PWM_SEL(c2_pwm_f_sel, PWM_CLK_EF_CTRL, 0x3, 25);
MESON_C2_PWM_DIV(c2_pwm_f_div, PWM_CLK_EF_CTRL, 16, 8, c2_pwm_f_sel);
MESON_C2_PWM_GATE(c2_pwm_f, PWM_CLK_EF_CTRL, 24, c2_pwm_f_div);

MESON_C2_PWM_SEL(c2_pwm_g_sel, PWM_CLK_GH_CTRL, 0x3, 9);
MESON_C2_PWM_DIV(c2_pwm_g_div, PWM_CLK_GH_CTRL, 0, 8, c2_pwm_g_sel);
MESON_C2_PWM_GATE(c2_pwm_g, PWM_CLK_GH_CTRL, 8, c2_pwm_g_div);
MESON_C2_PWM_SEL(c2_pwm_h_sel, PWM_CLK_GH_CTRL, 0x3, 25);
MESON_C2_PWM_DIV(c2_pwm_h_div, PWM_CLK_GH_CTRL, 16, 8, c2_pwm_h_sel);
MESON_C2_PWM_GATE(c2_pwm_h, PWM_CLK_GH_CTRL, 24, c2_pwm_h_div);

MESON_C2_PWM_SEL(c2_pwm_i_sel, PWM_CLK_IJ_CTRL, 0x3, 9);
MESON_C2_PWM_DIV(c2_pwm_i_div, PWM_CLK_IJ_CTRL, 0, 8, c2_pwm_i_sel);
MESON_C2_PWM_GATE(c2_pwm_i, PWM_CLK_IJ_CTRL, 8, c2_pwm_i_div);
MESON_C2_PWM_SEL(c2_pwm_j_sel, PWM_CLK_IJ_CTRL, 0x3, 25);
MESON_C2_PWM_DIV(c2_pwm_j_div, PWM_CLK_IJ_CTRL, 16, 8, c2_pwm_j_sel);
MESON_C2_PWM_GATE(c2_pwm_j, PWM_CLK_IJ_CTRL, 24, c2_pwm_j_div);

/* spicc clk */

/*    div2   |\         |\       _____
 *  ---------| |---DIV--| |     |     |    spicc out
 *  ---------| |        | |-----| GATE|---------
 *     ..... |/         | /     |_____|
 *  --------------------|/
 *                 24M
 */
static const char * const spicc_parent_names[] = {
	"fclk_div2", "fclk_div3", "fclk_div2p5", "hifi_pll",
	"gp_pll", "fclk_div4", "fclk_div5", "fclk_div7"
};

static struct clk_regmap c2_spicc_a_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = SPICC_CLK_CTRL,
		.mask = 0x7,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "spicc_a_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_names = spicc_parent_names,
		.num_parents = ARRAY_SIZE(spicc_parent_names),
	},
};

static struct clk_regmap c2_spicc_a_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = SPICC_CLK_CTRL,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "spicc_a_div",
		.ops = &clk_regmap_divider_ops,
		.parent_names = (const char *[]){ "spicc_a_sel" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap c2_spicc_a = {
	.data = &(struct clk_regmap_mux_data){
		.offset = SPICC_CLK_CTRL,
		.mask = 0x1,
		.shift = 15,
	},
	.hw.init = &(struct clk_init_data){
		.name = "spicc_a_mux",
		.ops = &clk_regmap_mux_ops,
		.parent_names = (const char *[]){ "spicc_a_div",
						"xtal" },
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap c2_spicc_a_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = SPICC_CLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "spicc_a_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "spicc_a_mux" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap c2_spicc_b_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = SPICC_CLK_CTRL,
		.mask = 0x7,
		.shift = 25,
	},
	.hw.init = &(struct clk_init_data){
		.name = "spicc_b_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_names = spicc_parent_names,
		.num_parents = ARRAY_SIZE(spicc_parent_names),
	},
};

static struct clk_regmap c2_spicc_b_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = SPICC_CLK_CTRL,
		.shift = 16,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "spicc_b_div",
		.ops = &clk_regmap_divider_ops,
		.parent_names = (const char *[]){ "spicc_b_sel" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap c2_spicc_b = {
	.data = &(struct clk_regmap_mux_data){
		.offset = SPICC_CLK_CTRL,
		.mask = 0x1,
		.shift = 31,
	},
	.hw.init = &(struct clk_init_data){
		.name = "spicc_b_mux",
		.ops = &clk_regmap_mux_ops,
		.parent_names = (const char *[]){ "spicc_b_div",
						"xtal" },
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap c2_spicc_b_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = SPICC_CLK_CTRL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "spicc_b_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "spicc_b_mux" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

/* ts clk */
static struct clk_regmap c2_ts_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = TS_CLK_CTRL,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "ts_div",
		.ops = &clk_regmap_divider_ops,
		.parent_names = (const char *[]){ "xtal" },
		.num_parents = 1,
	},
};

static struct clk_regmap c2_ts = {
	.data = &(struct clk_regmap_gate_data){
		.offset = TS_CLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "ts",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "ts_div" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

/* spifc clk */
static const char * const spifc_parent_names[] = {
	"fclk_div2", "fclk_div3", "fclk_div2p5",
	"fclk_div4", "fclk_div5", "fclk_div7"
};

static u32 mux_table_spifc[] = { 0, 1, 2, 5, 6, 7 };

static struct clk_regmap c2_spifc_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = SPIFC_CLK_CTRL,
		.mask = 0x7,
		.shift = 9,
		.table = mux_table_spifc,
	},
	.hw.init = &(struct clk_init_data){
		.name = "spifc_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_names = spifc_parent_names,
		.num_parents = ARRAY_SIZE(spifc_parent_names),
	},
};

static struct clk_regmap c2_spifc_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = SPIFC_CLK_CTRL,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "spifc_div",
		.ops = &clk_regmap_divider_ops,
		.parent_names = (const char *[]){ "spifc_sel" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap c2_spifc = {
	.data = &(struct clk_regmap_mux_data){
		.offset = SPIFC_CLK_CTRL,
		.mask = 0x1,
		.shift = 15,
	},
	.hw.init = &(struct clk_init_data){
		.name = "spifc_mux",
		.ops = &clk_regmap_mux_ops,
		.parent_names = (const char *[]){ "spifc_div",
						"xtal" },
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap c2_spifc_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = SPIFC_CLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "spifc_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "spifc_mux" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

/* usb bus clk */
static const char * const usb_bus_parent_names[] = {
	"xtal", "sys_clk", "fclk_div3", "fclk_div5"
};

static struct clk_regmap c2_usb_bus_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = USB_BUSCLK_CTRL,
		.mask = 0x3,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "usb_bus_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_names = usb_bus_parent_names,
		.num_parents = ARRAY_SIZE(usb_bus_parent_names),
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap c2_usb_bus_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = USB_BUSCLK_CTRL,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "usb_bus_div",
		.ops = &clk_regmap_divider_ops,
		.parent_names = (const char *[]){ "usb_bus_sel" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap c2_usb_bus = {
	.data = &(struct clk_regmap_gate_data){
		.offset = USB_BUSCLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "usb_bus",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "usb_bus_div" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

/* sd emmc clk */
static u32 mux_table_sd_emmc[] = { 0, 1, 2, 3, 5, 6, 7 };
/* delete the forth parent : hifi_pll
 * hifi pll only work for Audio
 */
static const char * const sd_emmc_parent_names[] = {
	"fclk_div2", "fclk_div3", "fclk_div2p5", "hifi_pll",
	 "fclk_div4", "fclk_div5", "fclk_div7"
};

static struct clk_regmap sd_emmc_a_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = SD_EMMC_CLK_CTRL,
		.mask = 0x7,
		.shift = 9,
		.table = mux_table_sd_emmc,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sd_emmc_a_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_names = sd_emmc_parent_names,
		.num_parents = ARRAY_SIZE(sd_emmc_parent_names),
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap sd_emmc_a_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = SD_EMMC_CLK_CTRL,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sd_emmc_a_div",
		.ops = &clk_regmap_divider_ops,
		.parent_names = (const char *[]){ "sd_emmc_a_sel" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap sd_emmc_a = {
	.data = &(struct clk_regmap_mux_data){
		.offset = SD_EMMC_CLK_CTRL,
		.mask = 0x1,
		.shift = 15,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sd_emmc_a",
		.ops = &clk_regmap_mux_ops,
		.parent_names = (const char *[]){ "sd_emmc_a_div",
						  "xtal" },
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap sd_emmc_a_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = SD_EMMC_CLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "sd_emmc_a_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "sd_emmc_a" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap sd_emmc_b_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = SD_EMMC_CLK_CTRL,
		.mask = 0x7,
		.shift = 25,
		.table = mux_table_sd_emmc,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sd_emmc_b_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_names = sd_emmc_parent_names,
		.num_parents = ARRAY_SIZE(sd_emmc_parent_names),
	//	.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap sd_emmc_b_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = SD_EMMC_CLK_CTRL,
		.shift = 16,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sd_emmc_b_div",
		.ops = &clk_regmap_divider_ops,
		.parent_names = (const char *[]){ "sd_emmc_b_sel" },
		.num_parents = 1,
	//	.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap sd_emmc_b = {
	.data = &(struct clk_regmap_mux_data){
		.offset = SD_EMMC_CLK_CTRL,
		.mask = 0x1,
		.shift = 31,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sd_emmc_b",
		.ops = &clk_regmap_mux_ops,
		.parent_names = (const char *[]){ "sd_emmc_b_div",
						  "xtal" },
		.num_parents = 2,
	//	.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap sd_emmc_b_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = SD_EMMC_CLK_CTRL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "sd_emmc_b_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "sd_emmc_b" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap sd_emmc_c_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = SD_EMMC_CLK_CTRL1,
		.mask = 0x7,
		.shift = 9,
		.table = mux_table_sd_emmc,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sd_emmc_c_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_names = sd_emmc_parent_names,
		.num_parents = ARRAY_SIZE(sd_emmc_parent_names),
		//.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap sd_emmc_c_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = SD_EMMC_CLK_CTRL1,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sd_emmc_c_div",
		.ops = &clk_regmap_divider_ops,
		.parent_names = (const char *[]){ "sd_emmc_c_sel" },
		.num_parents = 1,
		//.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap sd_emmc_c = {
	.data = &(struct clk_regmap_mux_data){
		.offset = SD_EMMC_CLK_CTRL1,
		.mask = 0x1,
		.shift = 15,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sd_emmc_c",
		.ops = &clk_regmap_mux_ops,
		.parent_names = (const char *[]){ "sd_emmc_c_div",
						  "xtal"
						},
		.num_parents = 2,
		//.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap sd_emmc_c_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = SD_EMMC_CLK_CTRL1,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "sd_emmc_c_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "sd_emmc_c" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

/*
 *rtc 32k clock
 *
 *xtal--GATE------------------GATE---------------------|\
 *	              |  --------                      | \
 *	              |  |      |                      |  \
 *	              ---| DUAL |----------------------|   |
 *	                 |      |                      |   |____GATE__
 *	                 --------                      |   |     rtc_32k_out
 *	   PAD-----------------------------------------|  /
 *	                                               | /
 *	   DUAL function:                              |/
 *	   bit 28 in RTC_BY_OSCIN_CTRL0 control the dual function.
 *	   when bit 28 = 0
 *	         f = 24M/N0
 *	   when bit 28 = 1
 *	         output N1 and N2 in run.
 *	   T = (x*T1 + y*T2)/x+y
 *	   f = (24M/(N0*M0 + N1*M1)) * (M0 + M1)
 *	   f: the frequecy value (HZ)
 *	       |      | |      |
 *	       | Div1 |-| Cnt1 |
 *	      /|______| |______|\
 *	    -|  ______   ______  ---> Out
 *	      \|      | |      |/
 *	       | Div2 |-| Cnt2 |
 *	       |______| |______|
 **/

/*
 * rtc 32k clock in gate
 */
static struct clk_regmap c2_rtc_32k_clkin = {
	.data = &(struct clk_regmap_gate_data){
		.offset = RTC_BY_OSCIN_CTRL0,
		.bit_idx = 31,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "rtc_32k_clkin",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "xtal" },
		.num_parents = 1,
	},
};

static const struct meson_clk_dualdiv_param c2_32k_div_table[] = {
	{
		.dual	= 1,
		.n1	= 733,
		.m1	= 8,
		.n2	= 732,
		.m2	= 11,
	},
	{},
};

static struct clk_regmap c2_rtc_32k_div = {
	.data = &(struct meson_clk_dualdiv_data){
		.n1 = {
			.reg_off = RTC_BY_OSCIN_CTRL0,
			.shift   = 0,
			.width   = 12,
		},
		.n2 = {
			.reg_off = RTC_BY_OSCIN_CTRL0,
			.shift   = 12,
			.width   = 12,
		},
		.m1 = {
			.reg_off = RTC_BY_OSCIN_CTRL1,
			.shift   = 0,
			.width   = 12,
		},
		.m2 = {
			.reg_off = RTC_BY_OSCIN_CTRL1,
			.shift   = 12,
			.width   = 12,
		},
		.dual = {
			.reg_off = RTC_BY_OSCIN_CTRL0,
			.shift   = 28,
			.width   = 1,
		},
		.table = c2_32k_div_table,
	},
	.hw.init = &(struct clk_init_data){
		.name = "rtc_32k_div",
		.ops = &meson_clk_dualdiv_ops,
		.parent_names = (const char *[]){ "rtc_32k_clkin" },
		.num_parents = 1,
	},
};

static struct clk_regmap c2_rtc_32k_xtal = {
	.data = &(struct clk_regmap_gate_data){
		.offset = RTC_BY_OSCIN_CTRL1,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "rtc_32k_xtal",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "rtc_32k_clkin" },
		.num_parents = 1,
	},
};

/*
 * three parent for rtc clock out
 * pad is from where?
 */
static u32 rtc_32k_sel[] = { 0, 1, 2 };
static const char * const rtc_32k_sel_parent_names[] = {
	"rtc_32k_xtal", "rtc_32k_div", "pad"
};

static struct clk_regmap c2_rtc_32k_sel = {
	.data = &(struct clk_regmap_mux_data) {
		.offset = RTC_CTRL,
		.mask = 0x3,
		.shift = 0,
		.table = rtc_32k_sel,
		.flags = CLK_MUX_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "rtc_32k_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_names = rtc_32k_sel_parent_names,
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap c2_rtc_clk = {
	.data = &(struct clk_regmap_gate_data){
		.offset = RTC_BY_OSCIN_CTRL0,
		.bit_idx = 30,
	},
	.hw.init = &(struct clk_init_data){
		.name = "rtc_clk",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "rtc_32k_sel" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

/* cpu clock */
static u32 cpu_fixed_source_sel_table[]	= { 0, 1, 2 };

static const char * const cpu_fixed_source_sel_parent_names[] = {
	"xtal", "fclk_div2", "fclk_div3"
};

/* cpu_fixed_sel0 */
static struct clk_regmap c2_cpu_fixed_source_sel0 = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CPUCTRL_CLK_CTRL0,
		.mask = 0x3,
		.shift = 0,
		.table = cpu_fixed_source_sel_table,
	},
	.hw.init = &(struct clk_init_data){
		.name = "cpu_fixed_source_sel0",
		.ops = &clk_regmap_mux_ops,
		.parent_names = cpu_fixed_source_sel_parent_names,
		.num_parents = ARRAY_SIZE(cpu_fixed_source_sel_parent_names),
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap c2_cpu_fixed_source_div0 = {
	.data = &(struct clk_regmap_div_data){
		.offset = CPUCTRL_CLK_CTRL0,
		.shift = 4,
		.width = 6,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "cpu_fixed_source_div0",
		.ops = &clk_regmap_divider_ops,
		.parent_names = (const char *[]){ "cpu_fixed_source_sel0" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap c2_cpu_fixed_sel0 = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CPUCTRL_CLK_CTRL0,
		.mask = 0x1,
		.shift = 2,
	},
	.hw.init = &(struct clk_init_data){
		.name = "cpu_fixed_sel0",
		.ops = &clk_regmap_mux_ops,
		.parent_names = (const char *[]){ "cpu_fixed_source_sel0",
						"cpu_fixed_source_div0" },
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

/* cpu_fixed_sel1 */
static struct clk_regmap c2_cpu_fixed_source_sel1 = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CPUCTRL_CLK_CTRL0,
		.mask = 0x3,
		.shift = 16,
		.table = cpu_fixed_source_sel_table,
	},
	.hw.init = &(struct clk_init_data){
		.name = "cpu_fixed_source_sel1",
		.ops = &clk_regmap_mux_ops,
		.parent_names = cpu_fixed_source_sel_parent_names,
		.num_parents = ARRAY_SIZE(cpu_fixed_source_sel_parent_names),
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap c2_cpu_fixed_source_div1 = {
	.data = &(struct clk_regmap_div_data){
		.offset = CPUCTRL_CLK_CTRL0,
		.shift = 20,
		.width = 6,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "cpu_fixed_source_div1",
		.ops = &clk_regmap_divider_ops,
		.parent_names = (const char *[]){ "cpu_fixed_source_sel1" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap c2_cpu_fixed_sel1 = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CPUCTRL_CLK_CTRL0,
		.mask = 0x1,
		.shift = 18,
	},
	.hw.init = &(struct clk_init_data){
		.name = "cpu_fixed_sel1",
		.ops = &clk_regmap_mux_ops,
		.parent_names = (const char *[]){ "cpu_fixed_source_sel1",
						"cpu_fixed_source_div1" },
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

/* cpu_fixed_clk */
static struct clk_regmap c2_cpu_fixed_clk = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CPUCTRL_CLK_CTRL0,
		.mask = 0x1,
		.shift = 10,
	},
	.hw.init = &(struct clk_init_data){
		.name = "cpu_fixed_clk",
		.ops = &clk_regmap_mux_ops,
		.parent_names = (const char *[]){ "cpu_fixed_sel0",
						"cpu_fixed_sel1" },
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

/* cpu clocks */
/* cpu_fixed_clk |\
 *---------------| \     cts_cpu_clk
 *  sys_pll      |  |--------
 *---------------| /
 *               |/
 */

static struct clk_regmap c2_cpu_clk = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CPUCTRL_CLK_CTRL0,
		.mask = 0x1,
		.shift = 11,
	},
	.hw.init = &(struct clk_init_data){
		.name = "cpu_clk",
		.ops = &clk_regmap_mux_ops,
		.parent_names = (const char *[]){ "cpu_fixed_clk", "sys_pll" },
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

/*axi bus*/
static struct clk_div_table axi_bus_table[] = {
	{.val = 0, .div = 2},
	{.val = 1, .div = 3},
	{.val = 2, .div = 4},
	{.val = 3, .div = 5},
	{.val = 4, .div = 6},
	{.val = 5, .div = 7},
	{.val = 6, .div = 8}
};

static struct clk_regmap c2_axi_clk_frcpu_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CPUCTRL_CLK_CTRL1,
		.shift = 9,
		.width = 3,
		.table = axi_bus_table,
	},
	.hw.init = &(struct clk_init_data){
		.name = "axi_clk_frcpu_div",
		.ops = &clk_regmap_divider_ops,
		.parent_names = (const char *[]){ "cpu_clk" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap c2_cts_pclk_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CPUCTRL_CLK_CTRL1,
		.shift = 3,
		.width = 3,
		.table = axi_bus_table,
	},
	.hw.init = &(struct clk_init_data){
		.name = "cts_pclk_div",
		.ops = &clk_regmap_divider_ops,
		.parent_names = (const char *[]){ "cpu_clk" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap c2_trace_clk_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CPUCTRL_CLK_CTRL1,
		.shift = 20,
		.width = 3,
		.table = axi_bus_table,
	},
	.hw.init = &(struct clk_init_data){
		.name = "trace_clk_div",
		.ops = &clk_regmap_divider_ops,
		.parent_names = (const char *[]){ "cpu_clk" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap c2_axi_clk_frcpu_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CPUCTRL_CLK_CTRL1,
		.bit_idx = 18,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "axi_clk_frcpu",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "axi_clk_frcpu_div" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap c2_cts_pclk_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CPUCTRL_CLK_CTRL1,
		.bit_idx = 16,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "cts_pclk",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "cts_pclk_div" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap c2_trace_clk_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CPUCTRL_CLK_CTRL1,
		.bit_idx = 23,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "trace_clk",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "trace_clk_div" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

/* wave clk */
static const char * const wave_parent_names[] = {
	"fclk_div2", "fclk_div3", "fclk_div2p5", "hifi_pll",
	"gp_pll", "fclk_div4", "fclk_div5", "fclk_div7"
};

static struct clk_regmap wave_a_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = WAVE_CLK_CTRL0,
		.mask = 0x7,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "wave_a_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_names = wave_parent_names,
		.num_parents = ARRAY_SIZE(wave_parent_names),
	},
};

static struct clk_regmap wave_a_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = WAVE_CLK_CTRL0,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "wave_a_div",
		.ops = &clk_regmap_divider_ops,
		.parent_names = (const char *[]){ "wave_a_sel" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap wave_aclk = {
	.data = &(struct clk_regmap_mux_data){
		.offset = WAVE_CLK_CTRL0,
		.mask = 0x1,
		.shift = 15,
	},
	.hw.init = &(struct clk_init_data){
		.name = "wave_aclk",
		.ops = &clk_regmap_mux_ops,
		.parent_names = (const char *[]){ "wave_a_div",
						  "xtal" },
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap wave_a_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = WAVE_CLK_CTRL0,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "wave_a_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "wave_aclk" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap wave_b_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = WAVE_CLK_CTRL0,
		.mask = 0x7,
		.shift = 25,
	},
	.hw.init = &(struct clk_init_data){
		.name = "wave_b_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_names = wave_parent_names,
		.num_parents = ARRAY_SIZE(wave_parent_names),
	},
};

static struct clk_regmap wave_b_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = WAVE_CLK_CTRL0,
		.shift = 16,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "wave_b_div",
		.ops = &clk_regmap_divider_ops,
		.parent_names = (const char *[]){ "wave_b_sel" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap wave_bclk = {
	.data = &(struct clk_regmap_mux_data){
		.offset = WAVE_CLK_CTRL0,
		.mask = 0x1,
		.shift = 31,
	},
	.hw.init = &(struct clk_init_data){
		.name = "wave_bclk",
		.ops = &clk_regmap_mux_ops,
		.parent_names = (const char *[]){ "wave_b_div",
						  "xtal" },
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap wave_b_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = WAVE_CLK_CTRL0,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "wave_b_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "wave_bclk" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap wave_c_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = WAVE_CLK_CTRL1,
		.mask = 0x7,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "wave_c_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_names = wave_parent_names,
		.num_parents = ARRAY_SIZE(wave_parent_names),
	},
};

static struct clk_regmap wave_c_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = WAVE_CLK_CTRL1,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "wave_c_div",
		.ops = &clk_regmap_divider_ops,
		.parent_names = (const char *[]){ "wave_c_sel" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap wave_cclk = {
	.data = &(struct clk_regmap_mux_data){
		.offset = WAVE_CLK_CTRL0,
		.mask = 0x1,
		.shift = 15,
	},
	.hw.init = &(struct clk_init_data){
		.name = "wave_cclk",
		.ops = &clk_regmap_mux_ops,
		.parent_names = (const char *[]){ "wave_c_div",
						  "xtal" },
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap wave_c_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = WAVE_CLK_CTRL1,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "wave_c_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "wave_cclk" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

/* jpeg */
static u32 mux_table_jpeg[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
static const char * const jpeg_parent_names[] = {
	"fclk_div2", "fclk_div3", "fclk_div2p5", "hifi_pll",
	"gp_pll", "fclk_div4", "fclk_div5", "fclk_div7"
};

static struct clk_regmap jpeg_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = JPEG_CLK_CTRL,
		.mask = 0x7,
		.shift = 9,
		.table = mux_table_jpeg,
	},
	.hw.init = &(struct clk_init_data){
		.name = "jpeg_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_names = jpeg_parent_names,
		.num_parents = ARRAY_SIZE(jpeg_parent_names),
	},
};

static struct clk_regmap jpeg_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = JPEG_CLK_CTRL,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "jpeg_div",
		.ops = &clk_regmap_divider_ops,
		.parent_names = (const char *[]){ "jpeg_sel" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap jpeg_clk = {
	.data = &(struct clk_regmap_mux_data){
		.offset = JPEG_CLK_CTRL,
		.mask = 0x1,
		.shift = 15,
	},
	.hw.init = &(struct clk_init_data){
		.name = "jpeg_clk",
		.ops = &clk_regmap_mux_ops,
		.parent_names = (const char *[]){ "jpeg_div",
						  "xtal" },
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap jpeg_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = JPEG_CLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "jpeg_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "jpeg_clk" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

/* MIPI */
static const char * const mipi_parent_names[] = {
	"fclk_div2", "fclk_div3", "fclk_div2p5", "hifi_pll",
	"gp_pll", "fclk_div4", "fclk_div5", "fclk_div7"
};

static struct clk_regmap mipi_csi_phy_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = MIPI_ISP_CLK_CTRL,
		.mask = 0x7,
		.shift = 25,
	},
	.hw.init = &(struct clk_init_data){
		.name = "mipi_csi_phy_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_names = mipi_parent_names,
		.num_parents = ARRAY_SIZE(jpeg_parent_names),
	},
};

static struct clk_regmap mipi_csi_phy_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = MIPI_ISP_CLK_CTRL,
		.shift = 16,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "mipi_csi_phy_div",
		.ops = &clk_regmap_divider_ops,
		.parent_names = (const char *[]){ "mipi_csi_phy_sel" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap mipi_csi_phy_clk = {
	.data = &(struct clk_regmap_mux_data){
		.offset = MIPI_ISP_CLK_CTRL,
		.mask = 0x1,
		.shift = 31,
	},
	.hw.init = &(struct clk_init_data){
		.name = "mipi_csi_phy_clk",
		.ops = &clk_regmap_mux_ops,
		.parent_names = (const char *[]){ "mipi_csi_phy_div",
						  "xtal" },
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap mipi_csi_phy_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = MIPI_ISP_CLK_CTRL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "mipi_csi_phy_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "mipi_csi_phy_clk" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap mipi_isp_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = MIPI_ISP_CLK_CTRL,
		.mask = 0x7,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "mipi_isp_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_names = mipi_parent_names,
		.num_parents = ARRAY_SIZE(jpeg_parent_names),
	},
};

static struct clk_regmap mipi_isp_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = MIPI_ISP_CLK_CTRL,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "mipi_isp_div",
		.ops = &clk_regmap_divider_ops,
		.parent_names = (const char *[]){ "mipi_isp_sel" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap mipi_isp_clk = {
	.data = &(struct clk_regmap_mux_data){
		.offset = MIPI_ISP_CLK_CTRL,
		.mask = 0x1,
		.shift = 15,
	},
	.hw.init = &(struct clk_init_data){
		.name = "mipi_isp_clk",
		.ops = &clk_regmap_mux_ops,
		.parent_names = (const char *[]){ "mipi_isp_div",
						  "xtal" },
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap mipi_isp_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = MIPI_ISP_CLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "mipi_isp_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "mipi_isp_clk" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

/* nna */
static const char * const nna_parent_names[] = {
	"fclk_div2", "fclk_div3", "fclk_div2p5", "hifi_pll",
	"gp_pll", "fclk_div4", "fclk_div5", "fclk_div7"
};

static struct clk_regmap nna_axi_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = NNA_CLK_CTRL,
		.mask = 0x7,
		.shift = 25,
	},
	.hw.init = &(struct clk_init_data){
		.name = "nna_axi_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_names = nna_parent_names,
		.num_parents = ARRAY_SIZE(jpeg_parent_names),
	},
};

static struct clk_regmap nna_axi_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = NNA_CLK_CTRL,
		.shift = 16,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "nna_axi_div",
		.ops = &clk_regmap_divider_ops,
		.parent_names = (const char *[]){ "nna_axi_sel" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap nna_axi_clk = {
	.data = &(struct clk_regmap_mux_data){
		.offset = NNA_CLK_CTRL,
		.mask = 0x1,
		.shift = 31,
	},
	.hw.init = &(struct clk_init_data){
		.name = "nna_axi_clk",
		.ops = &clk_regmap_mux_ops,
		.parent_names = (const char *[]){ "nna_axi_div",
						  "xtal" },
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap nna_axi_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = NNA_CLK_CTRL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "nna_axi_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "nna_axi_clk" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap nna_core_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = NNA_CLK_CTRL,
		.mask = 0x7,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "nna_core_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_names = nna_parent_names,
		.num_parents = ARRAY_SIZE(jpeg_parent_names),
	},
};

static struct clk_regmap nna_core_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = NNA_CLK_CTRL,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "nna_core_div",
		.ops = &clk_regmap_divider_ops,
		.parent_names = (const char *[]){ "nna_core_sel" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap nna_core_clk = {
	.data = &(struct clk_regmap_mux_data){
		.offset = NNA_CLK_CTRL,
		.mask = 0x1,
		.shift = 15,
	},
	.hw.init = &(struct clk_init_data){
		.name = "nna_core_clk",
		.ops = &clk_regmap_mux_ops,
		.parent_names = (const char *[]){ "nna_core_div",
						  "xtal" },
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap nna_core_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = NNA_CLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "nna_core_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "nna_core_clk" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

/* gdc */
static const char * const gdc_parent_names[] = {
	"fclk_div2", "fclk_div3", "fclk_div2p5", "hifi_pll",
	"gp_pll", "fclk_div4", "fclk_div5", "fclk_div7"
};

static struct clk_regmap gdc_axi_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = GDC_CLK_CTRL,
		.mask = 0x7,
		.shift = 25,
	},
	.hw.init = &(struct clk_init_data){
		.name = "gdc_axi_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_names = gdc_parent_names,
		.num_parents = ARRAY_SIZE(jpeg_parent_names),
	},
};

static struct clk_regmap gdc_axi_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = GDC_CLK_CTRL,
		.shift = 16,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "gdc_axi_div",
		.ops = &clk_regmap_divider_ops,
		.parent_names = (const char *[]){ "gdc_axi_sel" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap gdc_axi_clk = {
	.data = &(struct clk_regmap_mux_data){
		.offset = GDC_CLK_CTRL,
		.mask = 0x1,
		.shift = 31,
	},
	.hw.init = &(struct clk_init_data){
		.name = "gdc_axi_clk",
		.ops = &clk_regmap_mux_ops,
		.parent_names = (const char *[]){ "gdc_axi_div",
						  "xtal" },
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap gdc_axi_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = GDC_CLK_CTRL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "gdc_axi_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "gdc_axi_clk" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap gdc_core_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = GDC_CLK_CTRL,
		.mask = 0x7,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "gdc_core_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_names = gdc_parent_names,
		.num_parents = ARRAY_SIZE(gdc_parent_names),
	},
};

static struct clk_regmap gdc_core_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = GDC_CLK_CTRL,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "gdc_core_div",
		.ops = &clk_regmap_divider_ops,
		.parent_names = (const char *[]){ "gdc_core_sel" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap gdc_core_clk = {
	.data = &(struct clk_regmap_mux_data){
		.offset = GDC_CLK_CTRL,
		.mask = 0x1,
		.shift = 15,
	},
	.hw.init = &(struct clk_init_data){
		.name = "gdc_core_clk",
		.ops = &clk_regmap_mux_ops,
		.parent_names = (const char *[]){ "gdc_core_div",
						  "xtal" },
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap gdc_core_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = GDC_CLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "gdc_core_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "gdc_core_clk" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

/* ge2d */
static const char * const ge2d_parent_names[] = {
	"fclk_div2", "fclk_div3", "fclk_div2p5", "hifi_pll",
	"gp_pll", "fclk_div4", "fclk_div5", "fclk_div7"
};

static struct clk_regmap ge2d_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = GE2D_CLK_CTRL,
		.mask = 0x7,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "ge2d_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_names = ge2d_parent_names,
		.num_parents = ARRAY_SIZE(ge2d_parent_names),
	},
};

static struct clk_regmap ge2d_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = GE2D_CLK_CTRL,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "ge2d_div",
		.ops = &clk_regmap_divider_ops,
		.parent_names = (const char *[]){ "ge2d_sel" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap ge2d_clk = {
	.data = &(struct clk_regmap_mux_data){
		.offset = GE2D_CLK_CTRL,
		.mask = 0x1,
		.shift = 15,
	},
	.hw.init = &(struct clk_init_data){
		.name = "ge2d_clk",
		.ops = &clk_regmap_mux_ops,
		.parent_names = (const char *[]){ "ge2d_div",
						  "xtal" },
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap ge2d_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = GE2D_CLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "ge2d_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "ge2d_clk" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

/* secpu */
static const char * const secpu_parent_names[] = {
	"fclk_div2", "fclk_div3", "fclk_div2p5", "hifi_pll",
	"gp_pll", "fclk_div4", "fclk_div5", "fclk_div7"
};

static struct clk_regmap secpu_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = GE2D_CLK_CTRL,
		.mask = 0x7,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "secpu_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_names = secpu_parent_names,
		.num_parents = ARRAY_SIZE(secpu_parent_names),
	},
};

static struct clk_regmap secpu_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = GE2D_CLK_CTRL,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "secpu_div",
		.ops = &clk_regmap_divider_ops,
		.parent_names = (const char *[]){ "secpu_sel" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap secpu_clk = {
	.data = &(struct clk_regmap_mux_data){
		.offset = GE2D_CLK_CTRL,
		.mask = 0x1,
		.shift = 15,
	},
	.hw.init = &(struct clk_init_data){
		.name = "secpu_clk",
		.ops = &clk_regmap_mux_ops,
		.parent_names = (const char *[]){ "secpu_div",
						  "xtal" },
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap secpu_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = GE2D_CLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "secpu_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_names = (const char *[]){ "secpu_clk" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

/* Array of all clocks provided by this provider */
static struct clk_hw_onecell_data c2_hw_onecell_data = {
	.hws = {
		[CLKID_FIXED_PLL]		= &c2_fixed_pll.hw,
		[CLKID_FCLK50M_DIV40]		= &c2_fclk50_div40.hw,
		[CLKID_FCLK50M]			= &c2_fclk50M.hw,
		[CLKID_FCLK_DIV2_DIV]		= &c2_fclk_div2_div.hw,
		[CLKID_FCLK_DIV2]		= &c2_fclk_div2.hw,
		[CLKID_FCLK_DIV2P5_DIV]		= &c2_fclk_div2p5_div.hw,
		[CLKID_FCLK_DIV2P5]		= &c2_fclk_div2p5.hw,
		[CLKID_FCLK_DIV3_DIV]		= &c2_fclk_div3_div.hw,
		[CLKID_FCLK_DIV3]		= &c2_fclk_div3.hw,
		[CLKID_FCLK_DIV4_DIV]		= &c2_fclk_div4_div.hw,
		[CLKID_FCLK_DIV4]		= &c2_fclk_div4.hw,
		[CLKID_FCLK_DIV5_DIV]		= &c2_fclk_div5_div.hw,
		[CLKID_FCLK_DIV5]		= &c2_fclk_div5.hw,
		[CLKID_FCLK_DIV7_DIV]		= &c2_fclk_div7_div.hw,
		[CLKID_FCLK_DIV7]		= &c2_fclk_div7.hw,
		[CLKID_GP_PLL_VCO]		= &c2_gp_pll_vco.hw,
		[CLKID_GP_PLL_OD1_DIV]		= &c2_gp_pll_od1_div.hw,
		[CLKID_GP_PLL_GATE1]		= &c2_gp_pll_gate1.hw,
		[CLKID_GP_PLL_GATE2]		= &c2_gp_pll_gate2.hw,
		[CLKID_GP_PLL_GATE3]		= &c2_gp_pll_gate3.hw,
		[CLKID_GP_PLL]			= &c2_gp_pll.hw,
		[CLKID_GP_PLL_OUT1_OD]		= &c2_gp_pll_out1_od.hw,
		[CLKID_GP_PLL_OUT2_OD]		= &c2_gp_pll_out2_od.hw,
		[CLKID_GP_PLL_OUT1_DIV2]	= &c2_gp_pll_out1_div2_div.hw,
		[CLKID_GP_PLL_OUT2_DIV2]	= &c2_gp_pll_out2_div2_div.hw,
		[CLKID_GP_PLL_MCLK1_MUX]	= &c2_gp_pll_mclk1_mux.hw,
		[CLKID_GP_PLL_MCLK1_DIV]	= &c2_gp_pll_mclk1_div.hw,
		[CLKID_GP_PLL_MCLK1_GATE]	= &c2_gp_pll_mclk1_gate.hw,
		[CLKID_GP_PLL_MCLK2_MUX]	= &c2_gp_pll_mclk2_mux.hw,
		[CLKID_GP_PLL_MCLK2_DIV]	= &c2_gp_pll_mclk2_div.hw,
		[CLKID_GP_PLL_MCLK2_GATE]	= &c2_gp_pll_mclk2_gate.hw,
		[CLKID_HIFI_PLL]		= &c2_hifi_pll.hw,
		[CLKID_SYS_PLL_VCO]		= &c2_sys_pll_vco.hw,
		[CLKID_SYS_PLL_GATE]		= &c2_sys_pll_gate.hw,
		[CLKID_SYS_PLL]			= &c2_sys_pll.hw,
		[CLKID_SYS_B_SEL]		= &c2_sys_b_sel.hw,
		[CLKID_SYS_B_DIV]		= &c2_sys_b_div.hw,
		[CLKID_SYS_B]			= &c2_sys_b.hw,
		[CLKID_SYS_A_SEL]		= &c2_sys_a_sel.hw,
		[CLKID_SYS_A_DIV]		= &c2_sys_a_div.hw,
		[CLKID_SYS_A]			= &c2_sys_a.hw,
		[CLKID_SYS_CLK]			= &c2_sys_clk.hw,
		[CLKID_AXI_B_SEL]		= &axi_b_sel.hw,
		[CLKID_AXI_B_DIV]		= &axi_b_div.hw,
		[CLKID_AXI_B]			= &axi_b.hw,
		[CLKID_AXI_A_SEL]		= &axi_a_sel.hw,
		[CLKID_AXI_A_DIV]		= &axi_a_div.hw,
		[CLKID_AXI_A]			= &axi_a.hw,
		[CLKID_AXI_CLK]			= &axi_clk.hw,
		[CLKID_CPU_FSOURCE_SEL0]	= &c2_cpu_fixed_source_sel0.hw,
		[CLKID_CPU_FSOURCE_DIV0]	= &c2_cpu_fixed_source_div0.hw,
		[CLKID_CPU_FSEL0]		= &c2_cpu_fixed_sel0.hw,
		[CLKID_CPU_FSOURCE_SEL1]	= &c2_cpu_fixed_source_sel1.hw,
		[CLKID_CPU_FSOURCE_DIV1]	= &c2_cpu_fixed_source_div1.hw,
		[CLKID_CPU_FSEL1]		= &c2_cpu_fixed_sel1.hw,
		[CLKID_CPU_FCLK]		= &c2_cpu_fixed_clk.hw,
		[CLKID_CPU_CLK]			= &c2_cpu_clk.hw,
		[CLKID_AXI_CLK_FRCPU_DIV]	= &c2_axi_clk_frcpu_div.hw,
		[CLKID_CTS_PCLK_DIV]		= &c2_cts_pclk_div.hw,
		[CLKID_TRACE_CLK_DIV]		= &c2_trace_clk_div.hw,
		[CLKID_AXI_CLK_FRCPU_GATE]	= &c2_axi_clk_frcpu_gate.hw,
		[CLKID_CTS_PCLK_GATE]		= &c2_cts_pclk_gate.hw,
		[CLKID_TRACE_CLK_GATE]		= &c2_trace_clk_gate.hw,
		[CLKID_XTAL_CLKTREE]		= &xtal_clktree.hw,
		[CLKID_XTAL_FIXPLL]		= &xtal_fixpll.hw,
		[CLKID_XTAL_DDRPLL]		= &xtal_ddrpll.hw,
		[CLKID_XTAL_USB_CTRL]		= &xtal_usb_ctrl.hw,
		[CLKID_XTAL_HIFIPLL]		= &xtal_hifipll.hw,
		[CLKID_XTAL_SYSPLL]		= &xtal_syspll.hw,
		[CLKID_XTAL_DDS]		= &xtal_dds.hw,
		[CLKID_XTAL_ETHPLL]		= &xtal_ethpll.hw,
		[CLKID_XTAL_USBPHY]		= &xtal_usbphy.hw,
		[CLKID_XTAL_GPPLL]		= &xtal_gppll.hw,
		[CLKID_XTAL_GPIOM10]		= &xtal_gpio_m10.hw,
		[CLKID_XTAL_GPIOM13]		= &xtal_gpio_m13.hw,
		[CLKID_XTAL_PAD_DS0]		= &xtal_pad_ds0.hw,
		[CLKID_XTAL_PAD_DS1]		= &xtal_pad_ds1.hw,
		[CLKID_CLKTREE]			= &clk_tree.hw,
		[CLKID_RESET_CTRL]		= &reset_ctrl.hw,
		[CLKID_ANALOG_CTRL]		= &analog_ctrl.hw,
		[CLKID_PWR_CTRL]		= &pwr_ctrl.hw,
		[CLKID_PAD_CTRL]		= &pad_ctrl.hw,
		[CLKID_SYS_CTRL]		= &sys_ctrl.hw,
		[CLKID_TEMP_SENSOR]		= &temp_sensor.hw,
		[CLKID_AM2AXI_DIV]		= &am2axi_dev.hw,
		[CLKID_SPICC_B]			= &spicc_b.hw,
		[CLKID_SPICC_A]			= &spicc_a.hw,
		[CLKID_CLK_MSR]			= &clk_msr.hw,
		[CLKID_AUDIO]			= &audio.hw,
		[CLKID_JTAG_CTRL]		= &jtag_ctrl.hw,
		[CLKID_SARADC]			= &saradc.hw,
		[CLKID_PWM_EF]			= &pwm_ef.hw,
		[CLKID_PWM_CD]			= &pwm_cd.hw,
		[CLKID_PWM_AB]			= &pwm_ab.hw,
		[CLKID_I2C_S]			= &i2c_s.hw,
		[CLKID_IR_CTRL]			= &ir_ctrl.hw,
		[CLKID_I2C_M_D]			= &i2c_m_d.hw,
		[CLKID_I2C_M_C]			= &i2c_m_c.hw,
		[CLKID_I2C_M_B]			= &i2c_m_b.hw,
		[CLKID_I2C_M_A]			= &i2c_m_a.hw,
		[CLKID_ACODEC]			= &acodec.hw,
		[CLKID_OTP]			= &otp.hw,
		[CLKID_SYS_SD_EMMC_A]		= &sys_sd_emmc_a.hw,
		[CLKID_USB_PHY]			= &usb_phy.hw,
		[CLKID_USB_CTRL]		= &usb_ctrl.hw,
		[CLKID_SYS_DSPA]		= &sys_dspa.hw,
		[CLKID_DMA]			= &dma.hw,
		[CLKID_IRQ_CTRL]		= &irq_ctrl.hw,
		[CLKID_NIC]			= &nic.hw,
		[CLKID_GIC]			= &gic.hw,
		[CLKID_UART_C]			= &uart_c.hw,
		[CLKID_UART_B]			= &uart_b.hw,
		[CLKID_UART_A]			= &uart_a.hw,
		[CLKID_MMC]			= &mmc.hw,
		[CLKID_RSA]			= &rsa.hw,
		[CLKID_SYS_CORESIGHT]		= &coresight.hw,
		[CLKID_CSI_PH1]			= &csi_ph1.hw,
		[CLKID_CSI_PHY0]		= &csi_phy0.hw,
		[CLKID_MIPI_ISP]		= &mipi_isp.hw,
		[CLKID_CSI_DIG]			= &csi_dig.hw,
		[CLKID_SYS_G2ED]		= &ge2d.hw,
		[CLKID_SYS_GDC]			= &gdc.hw,
		[CLKID_DOS_APB]			= &dos_apb.hw,
		[CLKID_SYS_NNA]			= &nna.hw,
		[CLKID_SYS_ETH_MAC]		= &eth_mac.hw,
		[CLKID_SYS_ETH_MAC_DDR]		= &eth_mac_ddr.hw,
		[CLKID_SYS_UART_E]		= &uart_e.hw,
		[CLKID_SYS_UART_D]		= &uart_d.hw,
		[CLKID_SYS_PWM_IJ]		= &pwm_ij.hw,
		[CLKID_SYS_PWM_GH]		= &pwm_gh.hw,
		[CLKID_SYS_I2C_M_E]		= &i2c_m_e.hw,
		[CLKID_SYS_SD_EMMC_C]		= &sd_emmc_C.hw,
		[CLKID_SYS_SD_EMMC_B]		= &sd_emmc_B.hw,
		[CLKID_SYS_ROM]			= &rom.hw,
		[CLKID_SYS_SPIFC]		= &spifc.hw,
		[CLKID_SYS_PROD_I2C]		= &prod_i2c.hw,
		[CLKID_SYS_DOS]			= &dos.hw,
		[CLKID_SYS_CPU_CTRL]		= &cpu_ctrl.hw,
		[CLKID_SYS_RAMA]		= &sys_rama.hw,
		[CLKID_SYS_CAPU_SECPU]		= &capu_secpu.hw,
		[CLKID_SYS_MAILBOX]		= &mailbox.hw,
		[CLKID_AM2AXI_VAD]		= &axi_am2axi_vad.hw,
		[CLKID_AUDIO_VAD]		= &axi_audio_vad.hw,
		[CLKID_AXI_DMC]			= &axi_dmc.hw,
		[CLKID_AXI_RAMA]		= &axi_rama.hw,
		[CLKID_AXI_NIC]			= &axi_nic.hw,
		[CLKID_AXI_DMA]			= &axi_dma.hw,
		[CLKID_AXI_NIC_VAD]		= &axi_nic_vad.hw,
		[CLKID_AXI_CAPU]		= &axi_capu.hw,
		[CLKID_DSPA_A_SEL]		= &c2_dspa_a_sel.hw,
		[CLKID_DSPA_A_DIV]		= &c2_dspa_a_div.hw,
		[CLKID_DSPA_A]			= &c2_dspa_a.hw,
		[CLKID_DSPA_B_SEL]		= &c2_dspa_b_sel.hw,
		[CLKID_DSPA_B_DIV]		= &c2_dspa_b_div.hw,
		[CLKID_DSPA_B]			= &c2_dspa_b.hw,
		[CLKID_DSPA_SEL]		= &c2_dspa_sel.hw,
		[CLKID_DSPA_EN_DSPA]		= &c2_dspa_en_dspa.hw,
		[CLKID_DSPA_EN_NIC]		= &c2_dspa_en_nic.hw,
		[CLKID_24M]			= &c2_24m.hw,
		[CLKID_24M_DIV2]		= &c2_24m_div2.hw,
		[CLKID_12M]			= &c2_12m.hw,
		[CLKID_DIV2_PRE]		= &c2_fclk_div2_divn_pre.hw,
		[CLKID_FCLK_DIV2_DIVN]		= &c2_fclk_div2_divn.hw,
		[CLKID_GEN_SEL]			= &c2_gen_sel.hw,
		[CLKID_GEN_DIV]			= &c2_gen_div.hw,
		[CLKID_GEN]			= &c2_gen.hw,
		[CLKID_SARADC_SEL]		= &c2_saradc_sel.hw,
		[CLKID_SARADC_DIV]		= &c2_saradc_div.hw,
		[CLKID_SARADC_GATE]		= &c2_saradc_gate.hw,
		[CLKID_PWM_A_SEL]		= &c2_pwm_a_sel.hw,
		[CLKID_PWM_A_DIV]		= &c2_pwm_a_div.hw,
		[CLKID_PWM_A]			= &c2_pwm_a.hw,
		[CLKID_PWM_B_SEL]		= &c2_pwm_b_sel.hw,
		[CLKID_PWM_B_DIV]		= &c2_pwm_b_div.hw,
		[CLKID_PWM_B]			= &c2_pwm_b.hw,
		[CLKID_PWM_C_SEL]		= &c2_pwm_c_sel.hw,
		[CLKID_PWM_C_DIV]		= &c2_pwm_c_div.hw,
		[CLKID_PWM_C]			= &c2_pwm_c.hw,
		[CLKID_PWM_D_SEL]		= &c2_pwm_d_sel.hw,
		[CLKID_PWM_D_DIV]		= &c2_pwm_d_div.hw,
		[CLKID_PWM_D]			= &c2_pwm_d.hw,
		[CLKID_PWM_E_SEL]		= &c2_pwm_e_sel.hw,
		[CLKID_PWM_E_DIV]		= &c2_pwm_e_div.hw,
		[CLKID_PWM_E]			= &c2_pwm_e.hw,
		[CLKID_PWM_F_SEL]		= &c2_pwm_f_sel.hw,
		[CLKID_PWM_F_DIV]		= &c2_pwm_f_div.hw,
		[CLKID_PWM_F]			= &c2_pwm_f.hw,
		[CLKID_PWM_G_SEL]		= &c2_pwm_g_sel.hw,
		[CLKID_PWM_G_DIV]		= &c2_pwm_g_div.hw,
		[CLKID_PWM_G]			= &c2_pwm_g.hw,
		[CLKID_PWM_H_SEL]		= &c2_pwm_h_sel.hw,
		[CLKID_PWM_H_DIV]		= &c2_pwm_h_div.hw,
		[CLKID_PWM_H]			= &c2_pwm_h.hw,
		[CLKID_PWM_I_SEL]		= &c2_pwm_i_sel.hw,
		[CLKID_PWM_I_DIV]		= &c2_pwm_i_div.hw,
		[CLKID_PWM_I]			= &c2_pwm_i.hw,
		[CLKID_PWM_J_SEL]		= &c2_pwm_j_sel.hw,
		[CLKID_PWM_J_DIV]		= &c2_pwm_j_div.hw,
		[CLKID_PWM_J]			= &c2_pwm_j.hw,
		[CLKID_SPICC_A_SEL]		= &c2_spicc_a_sel.hw,
		[CLKID_SPICC_A_DIV]		= &c2_spicc_a_div.hw,
		[CLKID_SPICC_A_MUX]		= &c2_spicc_a.hw,
		[CLKID_SPICC_A_GATE]		= &c2_spicc_a_gate.hw,
		[CLKID_SPICC_B_SEL]		= &c2_spicc_b_sel.hw,
		[CLKID_SPICC_B_DIV]		= &c2_spicc_b_div.hw,
		[CLKID_SPICC_B_MUX]		= &c2_spicc_b.hw,
		[CLKID_SPICC_B_GATE]		= &c2_spicc_b_gate.hw,
		[CLKID_TS_DIV]			= &c2_ts_div.hw,
		[CLKID_TS]			= &c2_ts.hw,
		[CLKID_SPIFC_SEL]		= &c2_spifc_sel.hw,
		[CLKID_SPIFC_DIV]		= &c2_spifc_div.hw,
		[CLKID_SPIFC]			= &c2_spifc.hw,
		[CLKID_SPIFC_GATE]		= &c2_spifc_gate.hw,
		[CLKID_USB_BUS_SEL]		= &c2_usb_bus_sel.hw,
		[CLKID_USB_BUS_DIV]		= &c2_usb_bus_div.hw,
		[CLKID_USB_BUS]			= &c2_usb_bus.hw,
		[CLKID_SD_EMMC_A_SEL]		= &sd_emmc_a_sel.hw,
		[CLKID_SD_EMMC_A_DIV]		= &sd_emmc_a_div.hw,
		[CLKID_SD_EMMC_A]		= &sd_emmc_a.hw,
		[CLKID_SD_EMMC_A_GATE]		= &sd_emmc_a_gate.hw,
		[CLKID_SD_EMMC_B_SEL]		= &sd_emmc_b_sel.hw,
		[CLKID_SD_EMMC_B_DIV]		= &sd_emmc_b_div.hw,
		[CLKID_SD_EMMC_B]		= &sd_emmc_b.hw,
		[CLKID_SD_EMMC_B_GATE]		= &sd_emmc_b_gate.hw,
		[CLKID_SD_EMMC_C_SEL]		= &sd_emmc_c_sel.hw,
		[CLKID_SD_EMMC_C_DIV]		= &sd_emmc_c_div.hw,
		[CLKID_SD_EMMC_C]		= &sd_emmc_c.hw,
		[CLKID_SD_EMMC_C_GATE]		= &sd_emmc_c_gate.hw,
		[CLKID_RTC_32K_CLKIN]		= &c2_rtc_32k_clkin.hw,
		[CLKID_RTC_32K_DIV]		= &c2_rtc_32k_div.hw,
		[CLKID_RTC_32K_XTAL]		= &c2_rtc_32k_xtal.hw,
		[CLKID_RTC_32K_SEL]		= &c2_rtc_32k_sel.hw,
		[CLKID_RTC_CLK]			= &c2_rtc_clk.hw,
		[CLKID_WAVE_A_SEL]		= &wave_a_sel.hw,
		[CLKID_WAVE_A_DIV]		= &wave_a_div.hw,
		[CLKID_WAVE_A_CLK]		= &wave_aclk.hw,
		[CLKID_WAVE_A_GATE]		= &wave_a_gate.hw,
		[CLKID_WAVE_B_SEL]		= &wave_b_sel.hw,
		[CLKID_WAVE_B_DIV]		= &wave_b_div.hw,
		[CLKID_WAVE_B_CLK]		= &wave_bclk.hw,
		[CLKID_WAVE_B_GATE]		= &wave_b_gate.hw,
		[CLKID_WAVE_C_SEL]		= &wave_c_sel.hw,
		[CLKID_WAVE_C_DIV]		= &wave_c_div.hw,
		[CLKID_WAVE_C_CLK]		= &wave_cclk.hw,
		[CLKID_WAVE_C_GATE]		= &wave_c_gate.hw,
		[CLKID_JPEG_SEL]		= &jpeg_sel.hw,
		[CLKID_JPEG_DIV]		= &jpeg_div.hw,
		[CLKID_JPEG_CLK]		= &jpeg_clk.hw,
		[CLKID_JPEG_GATE]		= &jpeg_gate.hw,
		[CLKID_MIPI_CSI_PHY_SEL]	= &mipi_csi_phy_sel.hw,
		[CLKID_MIPI_CSI_PHY_DIV]	= &mipi_csi_phy_div.hw,
		[CLKID_MIPI_CSI_PHY_CLK]	= &mipi_csi_phy_clk.hw,
		[CLKID_MIPI_CSI_PHY_GATE]	= &mipi_csi_phy_gate.hw,
		[CLKID_MIPI_ISP_SEL]		= &mipi_isp_sel.hw,
		[CLKID_MIPI_ISP_DIV]		= &mipi_isp_div.hw,
		[CLKID_MIPI_ISP_CLK]		= &mipi_isp_clk.hw,
		[CLKID_MIPI_ISP_GATE]		= &mipi_isp_gate.hw,
		[CLKID_NNA_AXI_SEL]		= &nna_axi_sel.hw,
		[CLKID_NNA_AXI_DIV]		= &nna_axi_div.hw,
		[CLKID_NNA_AXI_CLK]		= &nna_axi_clk.hw,
		[CLKID_NNA_AXI_GATE]		= &nna_axi_gate.hw,
		[CLKID_NNA_CORE_SEL]		= &nna_core_sel.hw,
		[CLKID_NNA_CORE_DIV]		= &nna_core_div.hw,
		[CLKID_NNA_CORE_CLK]		= &nna_core_clk.hw,
		[CLKID_NNA_CORE_GATE]		= &nna_core_gate.hw,
		[CLKID_GDC_AXI_SEL]		= &gdc_axi_sel.hw,
		[CLKID_GDC_AXI_DIV]		= &gdc_axi_div.hw,
		[CLKID_GDC_AXI_CLK]		= &gdc_axi_clk.hw,
		[CLKID_GDC_AXI_GATE]		= &gdc_axi_gate.hw,
		[CLKID_GDC_CORE_SEL]		= &gdc_core_sel.hw,
		[CLKID_GDC_CORE_DIV]		= &gdc_core_div.hw,
		[CLKID_GDC_CORE_CLK]		= &gdc_core_clk.hw,
		[CLKID_GDC_CORE_GATE]		= &gdc_core_gate.hw,
		[CLKID_GE2D_SEL]		= &ge2d_sel.hw,
		[CLKID_GE2D_DIV]		= &ge2d_div.hw,
		[CLKID_GE2D_CLK]		= &ge2d_clk.hw,
		[CLKID_GE2D_GATE]		= &ge2d_gate.hw,
		[CLKID_SECPU_SEL]		= &secpu_sel.hw,
		[CLKID_SECPU_DIV]		= &secpu_div.hw,
		[CLKID_SECPU_CLK]		= &secpu_clk.hw,
		[CLKID_SECPU_GATE]		= &secpu_gate.hw,
		[NR_CLKS]			= NULL,
	},
	.num = NR_CLKS,
};

/* Convenience table to populate regmap in .probe */
static struct clk_regmap *const c2_clk_regmaps[] = {
	&xtal_clktree,
	&xtal_fixpll,
	&xtal_ddrpll,
	&xtal_usb_ctrl,
	&xtal_hifipll,
	&xtal_syspll,
	&xtal_dds,
	&xtal_ethpll,
	&xtal_usbphy,
	&xtal_gppll,
	&xtal_gpio_m10,
	&xtal_gpio_m13,
	&xtal_pad_ds0,
	&xtal_pad_ds1,
	&clk_tree,
	&reset_ctrl,
	&analog_ctrl,
	&pwr_ctrl,
	&pad_ctrl,
	&sys_ctrl,
	&temp_sensor,
	&am2axi_dev,
	&spicc_b,
	&spicc_a,
	&clk_msr,
	&audio,
	&jtag_ctrl,
	&saradc,
	&pwm_ef,
	&pwm_cd,
	&pwm_ab,
	&i2c_s,
	&ir_ctrl,
	&i2c_m_d,
	&i2c_m_c,
	&i2c_m_b,
	&i2c_m_a,
	&acodec,
	&otp,
	&sys_sd_emmc_a,
	&usb_phy,
	&usb_ctrl,
	&sys_dspa,
	&dma,
	&irq_ctrl,
	&nic,
	&gic,
	&uart_c,
	&uart_b,
	&uart_a,
	&mmc,
	&rsa,
	&coresight,
	&csi_ph1,
	&csi_phy0,
	&mipi_isp,
	&csi_dig,
	&ge2d,
	&gdc,
	&dos_apb,
	&nna,
	&eth_mac,
	&eth_mac_ddr,
	&uart_e,
	&uart_d,
	&pwm_ij,
	&pwm_gh,
	&i2c_m_e,
	&sd_emmc_C,
	&sd_emmc_B,
	&rom,
	&spifc,
	&prod_i2c,
	&dos,
	&cpu_ctrl,
	&sys_rama,
	&capu_secpu,
	&mailbox,
	&axi_am2axi_vad,
	&axi_audio_vad,
	&axi_dmc,
	&axi_rama,
	&axi_nic,
	&axi_dma,
	&axi_nic_vad,
	&axi_capu,
	&c2_sys_b_sel,
	&c2_sys_b_div,
	&c2_sys_b,
	&c2_sys_a_sel,
	&c2_sys_a_div,
	&c2_sys_a,
	&c2_sys_clk,
	&axi_b_sel,
	&axi_b_div,
	&axi_b,
	&axi_a_sel,
	&axi_a_div,
	&axi_a,
	&axi_clk,
	&c2_dspa_a_sel,
	&c2_dspa_a_div,
	&c2_dspa_a,
	&c2_dspa_b_sel,
	&c2_dspa_b_div,
	&c2_dspa_b,
	&c2_dspa_sel,
	&c2_dspa_en_dspa,
	&c2_dspa_en_nic,
	&c2_24m,
	&c2_12m,
	&c2_fclk_div2_divn_pre,
	&c2_fclk_div2_divn,
	&c2_gen_sel,
	&c2_gen_div,
	&c2_gen,
	&c2_saradc_sel,
	&c2_saradc_div,
	&c2_saradc_gate,
	&c2_pwm_a_sel,
	&c2_pwm_a_div,
	&c2_pwm_a,
	&c2_pwm_b_sel,
	&c2_pwm_b_div,
	&c2_pwm_b,
	&c2_pwm_c_sel,
	&c2_pwm_c_div,
	&c2_pwm_c,
	&c2_pwm_d_sel,
	&c2_pwm_d_div,
	&c2_pwm_d,
	&c2_pwm_e_sel,
	&c2_pwm_e_div,
	&c2_pwm_e,
	&c2_pwm_f_sel,
	&c2_pwm_f_div,
	&c2_pwm_f,
	&c2_pwm_g_sel,
	&c2_pwm_g_div,
	&c2_pwm_g,
	&c2_pwm_h_sel,
	&c2_pwm_h_div,
	&c2_pwm_h,
	&c2_pwm_i_sel,
	&c2_pwm_i_div,
	&c2_pwm_i,
	&c2_pwm_j_sel,
	&c2_pwm_j_div,
	&c2_pwm_j,
	&c2_spicc_a_sel,
	&c2_spicc_a_div,
	&c2_spicc_a,
	&c2_spicc_a_gate,
	&c2_spicc_b_sel,
	&c2_spicc_b_div,
	&c2_spicc_b,
	&c2_spicc_b_gate,
	&c2_ts_div,
	&c2_ts,
	&c2_spifc_sel,
	&c2_spifc_div,
	&c2_spifc,
	&c2_spifc_gate,
	&c2_usb_bus_sel,
	&c2_usb_bus_div,
	&c2_usb_bus,
	&sd_emmc_a_sel,
	&sd_emmc_a_div,
	&sd_emmc_a,
	&sd_emmc_a_gate,
	&sd_emmc_b_sel,
	&sd_emmc_b_div,
	&sd_emmc_b,
	&sd_emmc_b_gate,
	&sd_emmc_c_sel,
	&sd_emmc_c_div,
	&sd_emmc_c,
	&sd_emmc_c_gate,
	&c2_rtc_32k_clkin,
	&c2_rtc_32k_div,
	&c2_rtc_32k_xtal,
	&c2_rtc_32k_sel,
	&c2_rtc_clk,
	&wave_a_sel,
	&wave_a_div,
	&wave_aclk,
	&wave_a_gate,
	&wave_b_sel,
	&wave_b_div,
	&wave_bclk,
	&wave_b_gate,
	&wave_c_sel,
	&wave_c_div,
	&wave_cclk,
	&wave_c_gate,
	&jpeg_sel,
	&jpeg_div,
	&jpeg_clk,
	&jpeg_gate,
	&mipi_csi_phy_sel,
	&mipi_csi_phy_div,
	&mipi_csi_phy_clk,
	&mipi_csi_phy_gate,
	&mipi_isp_sel,
	&mipi_isp_div,
	&mipi_isp_clk,
	&mipi_isp_gate,
	&nna_axi_sel,
	&nna_axi_div,
	&nna_axi_clk,
	&nna_axi_gate,
	&nna_core_sel,
	&nna_core_div,
	&nna_core_clk,
	&nna_core_gate,
	&gdc_axi_sel,
	&gdc_axi_div,
	&gdc_axi_clk,
	&gdc_axi_gate,
	&gdc_core_sel,
	&gdc_core_div,
	&gdc_core_clk,
	&gdc_core_gate,
	&ge2d_sel,
	&ge2d_div,
	&ge2d_clk,
	&ge2d_gate,
	&secpu_sel,
	&secpu_div,
	&secpu_clk,
	&secpu_gate,
};

/*
 * cpu clock register base is 0xfd000000
 * the clk_regmap init alone
 */
static struct clk_regmap *const c2_cpu_clk_regmaps[] = {
	&c2_cpu_fixed_source_sel0,
	&c2_cpu_fixed_source_div0,
	&c2_cpu_fixed_sel0,
	&c2_cpu_fixed_source_sel1,
	&c2_cpu_fixed_source_div1,
	&c2_cpu_fixed_sel1,
	&c2_cpu_fixed_clk,
	&c2_cpu_clk,
	&c2_axi_clk_frcpu_div,
	&c2_cts_pclk_div,
	&c2_trace_clk_div,
	&c2_axi_clk_frcpu_gate,
	&c2_cts_pclk_gate,
	&c2_trace_clk_gate,
};

/*
 * pll clock register base is 0xfe007c00
 * the clk_regmap init alone
 */
static struct clk_regmap *const c2_pll_clk_regmaps[] = {
	&c2_fixed_pll,
	&c2_fclk50M,
	&c2_fclk_div2,
	&c2_fclk_div2p5,
	&c2_fclk_div3,
	&c2_fclk_div4,
	&c2_fclk_div5,
	&c2_fclk_div7,
	&c2_gp_pll_vco,
	&c2_gp_pll_od1_div,
	&c2_gp_pll_gate1,
	&c2_gp_pll_gate2,
	&c2_gp_pll_gate3,
	&c2_gp_pll,
	&c2_gp_pll_out1_od,
	&c2_gp_pll_out2_od,
	&c2_gp_pll_mclk1_mux,
	&c2_gp_pll_mclk1_div,
	&c2_gp_pll_mclk1_gate,
	&c2_gp_pll_mclk2_mux,
	&c2_gp_pll_mclk2_div,
	&c2_gp_pll_mclk2_gate,
	&c2_hifi_pll,
	&c2_sys_pll_vco,
	&c2_sys_pll_gate,
	&c2_sys_pll,
};

struct c2_sys_pll_nb_data {
	struct notifier_block nb;
	struct clk_hw *sys_pll;
	struct clk_hw *cpu_clk;
	struct clk_hw *cpu_dyn_clk;
};

static int c2_sys_pll_notifier_cb(struct notifier_block *nb, unsigned long event, void *data)
{
	struct c2_sys_pll_nb_data *nb_data =
		container_of(nb, struct c2_sys_pll_nb_data, nb);

	switch (event) {
	case PRE_RATE_CHANGE:
		/*
		 * This notifier means sys_pll clock will be changed
		 * to feed cpu_clk, this the current path :
		 * cpu_clk
		 *    \- sys_pll
		 *          \- sys_pll_dco
		 */

		/*
		 * Configure cpu_clk to use cpu_clk_dyn
		 * Make sure cpu clk is 1G, cpu_clk_dyn may equal 24M
		 */

		if (clk_set_rate(nb_data->cpu_dyn_clk->clk, 1000000000))
			pr_err("%s: set CPU dyn clock to 1G failed\n", __func__);

		clk_hw_set_parent(nb_data->cpu_clk,
				  nb_data->cpu_dyn_clk);
		/*
		 * Now, cpu_clk uses the dyn path
		 * cpu_clk
		 *    \- cpu_clk_dyn
		 *          \- cpu_clk_dynX
		 *                \- cpu_clk_dynX_sel
		 *                   \- cpu_clk_dynX_div
		 *                      \- xtal/fclk_div2/fclk_div3
		 *                   \- xtal/fclk_div2/fclk_div3
		 */

		return NOTIFY_OK;

	case POST_RATE_CHANGE:
		/*
		 * The sys_pll has ben updated, now switch back cpu_clk to
		 * sys_pll
		 */

		/* Configure cpu_clk to use sys_pll */
		clk_hw_set_parent(nb_data->cpu_clk,
				  nb_data->sys_pll);
		/* new path :
		 * cpu_clk
		 *    \- sys_pll
		 *          \- sys_pll_dco
		 */

		return NOTIFY_OK;

	default:
		return NOTIFY_DONE;
	}
}

static struct c2_sys_pll_nb_data c2_sys_pll_nb_data = {
	.sys_pll = &c2_sys_pll.hw,
	.cpu_clk = &c2_cpu_clk.hw,
	.cpu_dyn_clk = &c2_cpu_fixed_clk.hw,
	.nb.notifier_call = c2_sys_pll_notifier_cb,
};

struct c2_nb_data {
	struct notifier_block nb;
	struct clk_hw_onecell_data *onecell_data;
};

static int c2_cpu_fixed_clk_notifier_cb(struct notifier_block *nb,
					unsigned long event, void *data)
{
	struct clk_notifier_data *ndata = data;
	struct clk *cpu_fixed_clk, *parent_clk;
	int ret;

	switch (event) {
	case PRE_RATE_CHANGE:
	parent_clk = c2_cpu_fixed_sel1.hw.clk;
	ret = clk_set_rate(parent_clk, ndata->new_rate);
	if (ret)
		pr_err("set fixed sel1 to new rate failed\n");
		break;
	case POST_RATE_CHANGE:
	parent_clk = c2_cpu_fixed_sel0.hw.clk;
		break;
	default:
		return NOTIFY_DONE;
	}

	cpu_fixed_clk = c2_cpu_fixed_clk.hw.clk;
	ret = clk_set_parent(cpu_fixed_clk, parent_clk);
	if (ret)
		return notifier_from_errno(ret);

	return NOTIFY_OK;
}

static struct c2_nb_data c2_cpu_fixed_nb_data = {
	.nb.notifier_call = c2_cpu_fixed_clk_notifier_cb,
	.onecell_data = &c2_hw_onecell_data,
};

static const struct of_device_id clkc_match_table[] = {
	{ .compatible = "amlogic,c2-clkc" },
	{}
};

static struct regmap_config clkc_regmap_config = {
	.reg_bits       = 32,
	.val_bits       = 32,
	.reg_stride     = 4,
};

static struct regmap *c2_regmap_resource(struct device *dev, char *name)
{
	struct resource res;
	void __iomem *base;
	int i;
	struct device_node *node = dev->of_node;

	i = of_property_match_string(node, "reg-names", name);
	if (of_address_to_resource(node, i, &res))
		return ERR_PTR(-ENOENT);

	base = devm_ioremap_resource(dev, &res);
	if (IS_ERR(base))
		return ERR_CAST(base);

	clkc_regmap_config.max_register = resource_size(&res) - 4;
	clkc_regmap_config.name = devm_kasprintf(dev, GFP_KERNEL,
						 "%s-%s", node->name,
						 name);
	if (!clkc_regmap_config.name)
		return ERR_PTR(-ENOMEM);

	return devm_regmap_init_mmio(dev, base, &clkc_regmap_config);
}

static int c2_clkc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct regmap *basic_map;
	struct regmap *pll_map;
	struct regmap *cpu_clk_map;
	struct clk *clk;
	int ret, i;

	/* Get the hhi system controller node */
	/*
	 *map = syscon_node_to_regmap(of_get_parent(dev->of_node));
	 *if (IS_ERR(map)) {
	 *	dev_err(dev,
	 *		"failed to get HHI regmap\n");
	 *	return PTR_ERR(map);
	 *}
	 */
	clk = devm_clk_get(dev, "xtal");
	if (IS_ERR(clk)) {
		pr_err("%s: clock source xtal not found\n", dev_name(&pdev->dev));
		return PTR_ERR(clk);
	}

#ifdef CONFIG_AMLOGIC_CLK_DEBUG
		ret = devm_clk_hw_register_clkdev(dev, __clk_get_hw(clk),
						  NULL,
						  __clk_get_name(clk));
		if (ret < 0) {
			dev_err(dev, "Failed to clkdev register: %d\n", ret);
			return ret;
		}
#endif
	/* Get regmap for different clock area */
	basic_map = c2_regmap_resource(dev, "basic");
	if (IS_ERR(basic_map)) {
		dev_err(dev, "basic clk registers not found\n");
		return PTR_ERR(basic_map);
	}

	pll_map = c2_regmap_resource(dev, "pll");
	if (IS_ERR(pll_map)) {
		dev_err(dev, "pll clk registers not found\n");
		return PTR_ERR(pll_map);
	}

	cpu_clk_map = c2_regmap_resource(dev, "cpu_clk");
	if (IS_ERR(cpu_clk_map)) {
		dev_err(dev, "cpu clk registers not found\n");
		return PTR_ERR(cpu_clk_map);
	}

	/* Populate regmap for the regmap backed clocks */
	for (i = 0; i < ARRAY_SIZE(c2_clk_regmaps); i++)
		c2_clk_regmaps[i]->map = basic_map;

	for (i = 0; i < ARRAY_SIZE(c2_cpu_clk_regmaps); i++)
		c2_cpu_clk_regmaps[i]->map = cpu_clk_map;

	for (i = 0; i < ARRAY_SIZE(c2_pll_clk_regmaps); i++)
		c2_pll_clk_regmaps[i]->map = pll_map;

	for (i = 0; i < c2_hw_onecell_data.num; i++) {
		/* array might be sparse */
		if (!c2_hw_onecell_data.hws[i])
			continue;
		/*
		 * dev_err(dev, "register %d  %s\n",i,
		 *		c2_hw_onecell_data.hws[i]->init->name);
		 */
		ret = devm_clk_hw_register(dev, c2_hw_onecell_data.hws[i]);
		if (ret) {
			dev_err(dev, "Clock registration failed\n");
			return ret;
		}

#ifdef CONFIG_AMLOGIC_CLK_DEBUG
		ret = devm_clk_hw_register_clkdev(dev, c2_hw_onecell_data.hws[i],
						  NULL,
						  clk_hw_get_name(c2_hw_onecell_data.hws[i]));
		if (ret < 0) {
			dev_err(dev, "Failed to clkdev register: %d\n", ret);
			return ret;
		}
#endif
	}
	/*
	 * FIXME we shouldn't program the muxes in notifier handlers. The
	 * tricky programming sequence will be handled by the forthcoming
	 * coordinated clock rates mechanism once that feature is released.
	 */
	ret = clk_notifier_register(c2_sys_pll.hw.clk, &c2_sys_pll_nb_data.nb);
	if (ret) {
		pr_err("%s: failed to register sys pll notifier\n", __func__);
		return ret;
	}

	ret = clk_notifier_register(c2_cpu_fixed_sel0.hw.clk,
				    &c2_cpu_fixed_nb_data.nb);
	if (ret) {
		pr_err("%s: failed to register the CPU Fixed clock:fsel0 notifier\n",
		       __func__);
		return ret;
	}
	/*
	 * keep cpu_fixed_clk's parent as cpu_fixed_sel0 clock
	 */
	ret = clk_set_parent(c2_cpu_fixed_clk.hw.clk, c2_cpu_fixed_sel0.hw.clk);
	if (ret) {
		pr_err("%s: failed to set cpu_fixed_sel1 as cpu fixed clk's parent\n",
		       __func__);
		return ret;
	}

	/*
	 * fclk_div2, fclk_div3, fclk_div5, fclk_div7 should enable
	 * default it may be disabled when operate the child clock.
	 */
	ret = clk_prepare_enable(c2_fclk_div2.hw.clk);
	if (ret) {
		pr_err("%s: enable %s failed\n",
		       __func__,
		       c2_fclk_div2.hw.init->name);
		return ret;
	}
	ret = clk_prepare_enable(c2_fclk_div3.hw.clk);
	if (ret) {
		pr_err("%s: enable %s failed\n",
		       __func__,
		       c2_fclk_div3.hw.init->name);
		return ret;
	}
	ret = clk_prepare_enable(c2_fclk_div5.hw.clk);
	if (ret) {
		pr_err("%s: enable %s failed\n",
		       __func__,
		       c2_fclk_div5.hw.init->name);
		return ret;
	}
	ret = clk_prepare_enable(c2_fclk_div7.hw.clk);
	if (ret) {
		pr_err("%s: enable %s failed\n",
		       __func__,
		       c2_fclk_div7.hw.init->name);
		return ret;
	}

	return devm_of_clk_add_hw_provider(dev, of_clk_hw_onecell_get,
					   &c2_hw_onecell_data);
}

static struct platform_driver c2_driver = {
	.probe		= c2_clkc_probe,
	.driver		= {
		.name	= "c2-clkc",
		.of_match_table = clkc_match_table,
	},
};

static int c2_clkc_init(void)
{
	return platform_driver_register(&c2_driver);
}
subsys_initcall(c2_clkc_init);

MODULE_LICENSE("GPL v2");
