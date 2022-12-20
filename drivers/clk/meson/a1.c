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
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/of_address.h>
#include <linux/clkdev.h>

#include "clk-regmap.h"
#include "clk-pll.h"
#include "clk-dualdiv.h"
#include "a1.h"

//static DEFINE_SPINLOCK(meson_clk_lock);

/*
 * GATE for a1, delete the flag CLK_IGNORE_UNUSED
 * its parent clock is sys clock, the same the
 * clk81 in previos SoC
 */
#define MESON_A1_GATE(_name, _reg, _bit)				\
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

#define MESON_A1_XTAL_GATE(_name, _reg, _bit)				\
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
		.flags = CLK_IGNORE_UNUSED | CLK_IS_CRITICAL,		\
	},								\
}

/* PLL clock in gates,its parent is xtal */
static MESON_A1_XTAL_GATE(xtal_clktree,		SYS_OSCIN_CTRL,	0);
static MESON_A1_XTAL_GATE(xtal_fixpll,		SYS_OSCIN_CTRL,	1);
static MESON_A1_XTAL_GATE(xtal_usb_phy,		SYS_OSCIN_CTRL,	2);
static MESON_A1_XTAL_GATE(xtal_usb_ctrl,	SYS_OSCIN_CTRL,	3);
static MESON_A1_XTAL_GATE(xtal_hifipll,		SYS_OSCIN_CTRL,	4);
/*avoid syspll xtal gate disabling, do not describe it*/
static MESON_A1_XTAL_GATE(xtal_dds,		SYS_OSCIN_CTRL,	6);

/* Everything Else (EE) domain gates */
/* CLKTREE_SYS_CLK_EN0 */
static MESON_A1_GATE(sys_clk_clk_tree,		SYS_CLK_EN0,	0);
static MESON_A1_GATE(sys_clk_reset_ctrl,	SYS_CLK_EN0,	1);
static MESON_A1_GATE(sys_clk_analog_ctrl,	SYS_CLK_EN0,	2);
static MESON_A1_GATE(sys_clk_pwr_ctrl,		SYS_CLK_EN0,	3);
static MESON_A1_GATE(sys_clk_pad_ctrl,		SYS_CLK_EN0,	4);
static MESON_A1_GATE(sys_clk_sys_ctrl,		SYS_CLK_EN0,	5);
static MESON_A1_GATE(sys_clk_temp_sensor,	SYS_CLK_EN0,	6);
static MESON_A1_GATE(sys_clk_am2axi_dev,	SYS_CLK_EN0,	7);
static MESON_A1_GATE(sys_clk_spicc_b,		SYS_CLK_EN0,	8);
static MESON_A1_GATE(sys_clk_spicc_a,		SYS_CLK_EN0,	9);
static MESON_A1_GATE(sys_clk_clk_msr,		SYS_CLK_EN0,	10);
static MESON_A1_GATE(sys_clk_audio,		SYS_CLK_EN0,	11);
static MESON_A1_GATE(sys_clk_jtag_ctrl,		SYS_CLK_EN0,	12);
static MESON_A1_GATE(sys_clk_saradc,		SYS_CLK_EN0,	13);
static MESON_A1_GATE(sys_clk_pwm_ef,		SYS_CLK_EN0,	14);
static MESON_A1_GATE(sys_clk_pwm_cd,		SYS_CLK_EN0,	15);
static MESON_A1_GATE(sys_clk_pwm_ab,		SYS_CLK_EN0,	16);
static MESON_A1_GATE(sys_clk_cec,		SYS_CLK_EN0,	17);
static MESON_A1_GATE(sys_clk_i2c_s,		SYS_CLK_EN0,	18);
static MESON_A1_GATE(sys_clk_ir_ctrl,		SYS_CLK_EN0,	19);
static MESON_A1_GATE(sys_clk_i2c_m_d,		SYS_CLK_EN0,	20);
static MESON_A1_GATE(sys_clk_i2c_m_c,		SYS_CLK_EN0,	21);
static MESON_A1_GATE(sys_clk_i2c_m_b,		SYS_CLK_EN0,	22);
static MESON_A1_GATE(sys_clk_i2c_m_a,		SYS_CLK_EN0,	23);
static MESON_A1_GATE(sys_clk_acodec,		SYS_CLK_EN0,	24);
static MESON_A1_GATE(sys_clk_otp,		SYS_CLK_EN0,	25);
static MESON_A1_GATE(sys_clk_sd_emmc_a,		SYS_CLK_EN0,	26);
static MESON_A1_GATE(sys_clk_usb_phy,		SYS_CLK_EN0,	27);
static MESON_A1_GATE(sys_clk_usb_ctrl,		SYS_CLK_EN0,	28);
static MESON_A1_GATE(sys_clk_sys_dspb,		SYS_CLK_EN0,	29);
static MESON_A1_GATE(sys_clk_sys_dspa,		SYS_CLK_EN0,	30);
static MESON_A1_GATE(sys_clk_dma,		SYS_CLK_EN0,	31);
/* CLKTREE_SYS_CLK_EN1 */
static MESON_A1_GATE(sys_clk_irq_ctrl,		SYS_CLK_EN1,	0);
static MESON_A1_GATE(sys_clk_nic,		SYS_CLK_EN1,	1);
static MESON_A1_GATE(sys_clk_gic,		SYS_CLK_EN1,	2);
static MESON_A1_GATE(sys_clk_uart_c,		SYS_CLK_EN1,	3);
static MESON_A1_GATE(sys_clk_uart_b,		SYS_CLK_EN1,	4);
static MESON_A1_GATE(sys_clk_uart_a,		SYS_CLK_EN1,	5);
static MESON_A1_GATE(sys_clk_sys_psram,		SYS_CLK_EN1,	6);
static MESON_A1_GATE(sys_clk_rsa,		SYS_CLK_EN1,	8);
static MESON_A1_GATE(sys_clk_coresight,		SYS_CLK_EN1,	9);
/* CLKTREE_AXI_CLK_EN */
static MESON_A1_GATE(sys_clk_am2axi_vad,	AXI_CLK_EN,	0);
static MESON_A1_GATE(sys_clk_audio_vad,		AXI_CLK_EN,	1);
static MESON_A1_GATE(sys_clk_axi_dmc,		AXI_CLK_EN,	3);
static MESON_A1_GATE(sys_clk_axi_psram,		AXI_CLK_EN,	4);
static MESON_A1_GATE(sys_clk_ramb,		AXI_CLK_EN,	5);
static MESON_A1_GATE(sys_clk_rama,		AXI_CLK_EN,	6);
static MESON_A1_GATE(sys_clk_axi_spifc,		AXI_CLK_EN,	7);
static MESON_A1_GATE(sys_clk_axi_nic,		AXI_CLK_EN,	8);
static MESON_A1_GATE(sys_clk_axi_dma,		AXI_CLK_EN,	9);
static MESON_A1_GATE(sys_clk_cpu_ctrl,		AXI_CLK_EN,	10);
static MESON_A1_GATE(sys_clk_rom,		AXI_CLK_EN,	11);
static MESON_A1_GATE(sys_clk_prod_i2c,		AXI_CLK_EN,	12);

/* fixed pll = 1536M
 *
 * fixed pll ----- fclk_div2 = 768M
 *           |
 *           ----- fclk_div3 = 512M
 *           |
 *           ----- fclk_div5 = 307.2M
 *           |
 *           ----- fclk_div7 = 219.4M
 */
static struct clk_regmap a1_fixed_pll_dco = {
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
		.frac = {
			.reg_off = ANACTRL_FIXPLL_CTRL1,
			.shift   = 0,
			.width   = 19,
		},
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
		.name = "fixed_pll_dco",
		.ops = &meson_clk_pll_ro_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&xtal_fixpll.hw
		},
		.num_parents = 1,
	},
};

static struct clk_regmap a1_fixed_pll = {
	.data = &(struct clk_regmap_gate_data){
		.offset = ANACTRL_FIXPLL_CTRL0,
		.bit_idx = 20,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "fixed_pll",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_fixed_pll_dco.hw
		},
		.num_parents = 1,
		.flags = CLK_IGNORE_UNUSED,
	},
};

static struct clk_fixed_factor a1_fclk_div2_div = {
	.mult = 1,
	.div = 2,
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div2_div",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_fixed_pll.hw
		},
		.num_parents = 1,
	},
};

static struct clk_regmap a1_fclk_div2 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = ANACTRL_FIXPLL_CTRL0,
		.bit_idx = 21,
	},
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div2",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_fclk_div2_div.hw
		},
		.num_parents = 1,
		/*
		 * This clock is used by dmc in BL2 firmware and is required
		 * by the platform to operate correctly.
		 * Until the following condition are met, we need this clock to
		 * be marked as critical:
		 * a) Mark the clock used by a firmware resource, if possible
		 * b) CCF has a clock hand-off mechanism to make the sure the
		 *    clock stays on until the proper driver comes along
		 */
		.flags = CLK_IS_CRITICAL,
	},
};

static struct clk_fixed_factor a1_fclk_div3_div = {
	.mult = 1,
	.div = 3,
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div3_div",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_fixed_pll.hw
		},
		.num_parents = 1,
	},
};

static struct clk_regmap a1_fclk_div3 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = ANACTRL_FIXPLL_CTRL0,
		.bit_idx = 22,
	},
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div3",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_fclk_div3_div.hw
		},
		.num_parents = 1,
		/*
		 * This clock is used by sys clock (APB bus working clock)
		 * and is required by the platform to operate correctly.
		 * Until the following condition are met, we need this clock to
		 * be marked as critical:
		 * a) Mark the clock used by a firmware resource, if possible
		 * b) CCF has a clock hand-off mechanism to make the sure the
		 *    clock stays on until the proper driver comes along
		 */
		.flags = CLK_IS_CRITICAL,
	},
};

static struct clk_fixed_factor a1_fclk_div5_div = {
	.mult = 1,
	.div = 5,
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div5_div",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_fixed_pll.hw
		},
		.num_parents = 1,
	},
};

static struct clk_regmap a1_fclk_div5 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = ANACTRL_FIXPLL_CTRL0,
		.bit_idx = 23,
	},
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div5",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_fclk_div5_div.hw
		},
		.num_parents = 1,
		/*
		 * This clock is used by AXI bus working clock
		 * and is required by the platform to operate correctly.
		 * Until the following condition are met, we need this clock to
		 * be marked as critical:
		 * a) Mark the clock used by a firmware resource, if possible
		 * b) CCF has a clock hand-off mechanism to make the sure the
		 *    clock stays on until the proper driver comes along
		 */
		.flags = CLK_IS_CRITICAL,
	},
};

static struct clk_fixed_factor a1_fclk_div7_div = {
	.mult = 1,
	.div = 7,
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div7_div",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_fixed_pll.hw
		},
		.num_parents = 1,
	},
};

static struct clk_regmap a1_fclk_div7 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = ANACTRL_FIXPLL_CTRL0,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div7",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_fclk_div7.hw
		},
		.num_parents = 1,
	},
};

/* 614.4M */
static const struct reg_sequence a1_hifi_init_regs[] = {
	{ .reg = ANACTRL_HIFIPLL_CTRL1,	.def = 0x01800000 },
	{ .reg = ANACTRL_HIFIPLL_CTRL2,	.def = 0x00001100 },
	{ .reg = ANACTRL_HIFIPLL_CTRL3,	.def = 0x10022200 },
	{ .reg = ANACTRL_HIFIPLL_CTRL4,	.def = 0x00301000 },
	{ .reg = ANACTRL_HIFIPLL_CTRL0, .def = 0x01f19480 },
	{ .reg = ANACTRL_HIFIPLL_CTRL0, .def = 0x11f19480, .delay_us = 10 },
	{ .reg = ANACTRL_HIFIPLL_CTRL0,	.def = 0x15f11480, .delay_us = 40 },
	{ .reg = ANACTRL_HIFIPLL_CTRL2,	.def = 0x00001140 },
	{ .reg = ANACTRL_HIFIPLL_CTRL2,	.def = 0x00001100 },
};

#ifdef CONFIG_ARM
static const struct pll_params_table a1_hifi_pll_params_table[] = {
	PLL_PARAMS(128, 5, 0), /* DCO = 614.4M */
};
#else
static const struct pll_params_table a1_hifi_pll_params_table[] = {
	PLL_PARAMS(128, 5), /* DCO = 614.4M */
};
#endif

static struct clk_regmap a1_hifi_pll = {
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
			.reg_off = ANACTRL_HIFIPLL_CTRL1,
			.shift   = 0,
			.width   = 19,
		},
		.l = {
			.reg_off = ANACTRL_HIFIPLL_STS,
			.shift   = 31,
			.width   = 1,
		},
		.table = a1_hifi_pll_params_table,
		.init_regs = a1_hifi_init_regs,
		.init_count = ARRAY_SIZE(a1_hifi_init_regs),
	},
	.hw.init = &(struct clk_init_data){
		.name = "hifi_pll",
		.ops = &meson_clk_pll_v3_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&xtal_hifipll.hw
		},
		.num_parents = 1,
	},
};

/*
 * SYS PLL rang from 768M to 1536M
 * the PLL parameter table is for hifi/sys pll.
 * Additional, there is no OD in A1 PLL.
 */
#ifdef CONFIG_ARM
static const struct pll_params_table a1_pll_params_table[] = {
	PLL_PARAMS(32, 1, 0), /* DCO = 768M */
	PLL_PARAMS(33, 1, 0), /* DCO = 792M */
	PLL_PARAMS(34, 1, 0), /* DCO = 816M */
	PLL_PARAMS(35, 1, 0), /* DCO = 840M */
	PLL_PARAMS(36, 1, 0), /* DCO = 864M */
	PLL_PARAMS(37, 1, 0), /* DCO = 888M */
	PLL_PARAMS(38, 1, 0), /* DCO = 912M */
	PLL_PARAMS(39, 1, 0), /* DCO = 936M */
	PLL_PARAMS(40, 1, 0), /* DCO = 960M */
	PLL_PARAMS(41, 1, 0), /* DCO = 984M */
	PLL_PARAMS(42, 1, 0), /* DCO = 1008M */
	PLL_PARAMS(43, 1, 0), /* DCO = 1032M */
	PLL_PARAMS(44, 1, 0), /* DCO = 1056M */
	PLL_PARAMS(45, 1, 0), /* DCO = 1080M */
	PLL_PARAMS(46, 1, 0), /* DCO = 1104M */
	PLL_PARAMS(47, 1, 0), /* DCO = 1128M */
	PLL_PARAMS(48, 1, 0), /* DCO = 1152M */
	PLL_PARAMS(49, 1, 0), /* DCO = 1176M */
	PLL_PARAMS(50, 1, 0), /* DCO = 1200M */
	PLL_PARAMS(51, 1, 0), /* DCO = 1224M */
	PLL_PARAMS(52, 1, 0), /* DCO = 1248M */
	PLL_PARAMS(53, 1, 0), /* DCO = 1272M */
	PLL_PARAMS(54, 1, 0), /* DCO = 1296M */
	PLL_PARAMS(55, 1, 0), /* DCO = 1320M */
	PLL_PARAMS(56, 1, 0), /* DCO = 1344M */
	PLL_PARAMS(57, 1, 0), /* DCO = 1368M */
	PLL_PARAMS(58, 1, 0), /* DCO = 1392M */
	PLL_PARAMS(59, 1, 0), /* DCO = 1416M */
	PLL_PARAMS(60, 1, 0), /* DCO = 1440M */
	PLL_PARAMS(61, 1, 0), /* DCO = 1464M */
	PLL_PARAMS(62, 1, 0), /* DCO = 1488M */
	PLL_PARAMS(63, 1, 0), /* DCO = 1512M */
	PLL_PARAMS(64, 1, 0), /* DCO = 1536M */

	{ /* sentinel */ },
};
#else
static const struct pll_params_table a1_pll_params_table[] = {
	PLL_PARAMS(32, 1), /* DCO = 768M */
	PLL_PARAMS(33, 1), /* DCO = 792M */
	PLL_PARAMS(34, 1), /* DCO = 816M */
	PLL_PARAMS(35, 1), /* DCO = 840M */
	PLL_PARAMS(36, 1), /* DCO = 864M */
	PLL_PARAMS(37, 1), /* DCO = 888M */
	PLL_PARAMS(38, 1), /* DCO = 912M */
	PLL_PARAMS(39, 1), /* DCO = 936M */
	PLL_PARAMS(40, 1), /* DCO = 960M */
	PLL_PARAMS(41, 1), /* DCO = 984M */
	PLL_PARAMS(42, 1), /* DCO = 1008M */
	PLL_PARAMS(43, 1), /* DCO = 1032M */
	PLL_PARAMS(44, 1), /* DCO = 1056M */
	PLL_PARAMS(45, 1), /* DCO = 1080M */
	PLL_PARAMS(46, 1), /* DCO = 1104M */
	PLL_PARAMS(47, 1), /* DCO = 1128M */
	PLL_PARAMS(48, 1), /* DCO = 1152M */
	PLL_PARAMS(49, 1), /* DCO = 1176M */
	PLL_PARAMS(50, 1), /* DCO = 1200M */
	PLL_PARAMS(51, 1), /* DCO = 1224M */
	PLL_PARAMS(52, 1), /* DCO = 1248M */
	PLL_PARAMS(53, 1), /* DCO = 1272M */
	PLL_PARAMS(54, 1), /* DCO = 1296M */
	PLL_PARAMS(55, 1), /* DCO = 1320M */
	PLL_PARAMS(56, 1), /* DCO = 1344M */
	PLL_PARAMS(57, 1), /* DCO = 1368M */
	PLL_PARAMS(58, 1), /* DCO = 1392M */
	PLL_PARAMS(59, 1), /* DCO = 1416M */
	PLL_PARAMS(60, 1), /* DCO = 1440M */
	PLL_PARAMS(61, 1), /* DCO = 1464M */
	PLL_PARAMS(62, 1), /* DCO = 1488M */
	PLL_PARAMS(63, 1), /* DCO = 1512M */
	PLL_PARAMS(64, 1), /* DCO = 1536M */

	{ /* sentinel */ },
};
#endif

/*
 * Internal sys pll emulation configuration parameters
 */
static const struct reg_sequence a1_sys_init_regs[] = {
	{ .reg = ANACTRL_SYSPLL_CTRL1,	.def = 0x01800000 },
	{ .reg = ANACTRL_SYSPLL_CTRL2,	.def = 0x00001100 },
	{ .reg = ANACTRL_SYSPLL_CTRL3,	.def = 0x10022300 },
	{ .reg = ANACTRL_SYSPLL_CTRL4,	.def = 0x00300000 },
	{ .reg = ANACTRL_SYSPLL_CTRL0,  .def = 0x01f18440 },
	{ .reg = ANACTRL_SYSPLL_CTRL0,  .def = 0x11f18440, .delay_us = 10 },
	{ .reg = ANACTRL_SYSPLL_CTRL0,	.def = 0x15f18440, .delay_us = 40 },
	{ .reg = ANACTRL_SYSPLL_CTRL2,	.def = 0x00001140 },
	{ .reg = ANACTRL_SYSPLL_CTRL2,	.def = 0x00001100 },
};

/*
 * sys pll = 768M ~ 1536M
 */
static struct clk_regmap a1_sys_pll = {
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
		.frac = {
			.reg_off = ANACTRL_SYSPLL_CTRL1,
			.shift   = 0,
			.width   = 19,
		},
		.l = {
			.reg_off = ANACTRL_SYSPLL_STS,
			.shift   = 31,
			.width   = 1,
		},
		.table = a1_pll_params_table,
		.init_regs = a1_sys_init_regs,
		.init_count = ARRAY_SIZE(a1_sys_init_regs),
		.flags = CLK_MESON_PLL_IGNORE_INIT
	},
	.hw.init = &(struct clk_init_data){
		.name = "sys_pll",
		.ops = &meson_clk_pll_v3_ops,
		.parent_names = (const char *[]){ "xtal" },
		.num_parents = 1,
		.flags = CLK_IGNORE_UNUSED,
	},
};

/* sys clk = 64M */
static u32 mux_table_sys_ab_clk_sel[] = { 0, 1, 2, 3 };

static const struct clk_parent_data a1_sys_clk_parent_data[] = {
	{ .fw_name = "xtal", },
	{ .hw = &a1_fclk_div2.hw },
	{ .hw = &a1_fclk_div3.hw },
	{ .hw = &a1_fclk_div5.hw },
};

static struct clk_regmap a1_sys_b_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = SYS_CLK_CTRL0,
		.mask = 0x7,
		.shift = 26,
		.table = mux_table_sys_ab_clk_sel,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sys_b_sel",
		.ops = &clk_regmap_mux_ro_ops,
		.parent_data = a1_sys_clk_parent_data,
		.num_parents = ARRAY_SIZE(a1_sys_clk_parent_data),
	},
};

static struct clk_regmap a1_sys_b_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = SYS_CLK_CTRL0,
		.shift = 16,
		.width = 10,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sys_b_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_sys_b_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap a1_sys_b = {
	.data = &(struct clk_regmap_gate_data){
		.offset = SYS_CLK_CTRL0,
		.bit_idx = 29,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "sys_b",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_sys_b_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IS_CRITICAL,
	},
};

static struct clk_regmap a1_sys_a_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = SYS_CLK_CTRL0,
		.mask = 0x7,
		.shift = 10,
		.table = mux_table_sys_ab_clk_sel,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sys_a_sel",
		.ops = &clk_regmap_mux_ro_ops,
		.parent_data = a1_sys_clk_parent_data,
		.num_parents = ARRAY_SIZE(a1_sys_clk_parent_data),
	},
};

static struct clk_regmap a1_sys_a_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = SYS_CLK_CTRL0,
		.shift = 0,
		.width = 10,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sys_a_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_sys_a_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap a1_sys_a = {
	.data = &(struct clk_regmap_gate_data){
		.offset = SYS_CLK_CTRL0,
		.bit_idx = 13,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "sys_a",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_sys_a_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IS_CRITICAL,
	},
};

static struct clk_regmap a1_sys_clk = {
	.data = &(struct clk_regmap_mux_data){
		.offset = SYS_CLK_CTRL0,
		.mask = 0x1,
		.shift = 31,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sys_clk",
		.ops = &clk_regmap_mux_ro_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_sys_a.hw,
			&a1_sys_b.hw
		},
		.num_parents = 2,
	},
};

static u32 mux_table_dspab_sel[] = { 0, 1, 2, 3 };

static const struct clk_parent_data a1_dsp_clk_parent_data[] = {
	{ .fw_name = "xtal", },
	{ .hw = &a1_fclk_div2.hw },
	{ .hw = &a1_fclk_div3.hw },
	{ .hw = &a1_fclk_div5.hw },
};

static struct clk_regmap a1_dspa_a_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = DSPA_CLK_CTRL0,
		.mask = 0x7,
		.shift = 10,
		.table = mux_table_dspab_sel,
	},
	.hw.init = &(struct clk_init_data){
		.name = "dspa_a_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = a1_dsp_clk_parent_data,
		.num_parents = ARRAY_SIZE(a1_dsp_clk_parent_data),
	},
};

static struct clk_regmap a1_dspa_a_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = DSPA_CLK_CTRL0,
		.shift = 0,
		.width = 10,
	},
	.hw.init = &(struct clk_init_data){
		.name = "dspa_a_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_dspa_a_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap a1_dspa_a = {
	.data = &(struct clk_regmap_gate_data){
		.offset = DSPA_CLK_CTRL0,
		.bit_idx = 13,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "dspa_a",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_dspa_a_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap a1_dspa_b_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = DSPA_CLK_CTRL0,
		.mask = 0x7,
		.shift = 26,
		.table = mux_table_dspab_sel,
	},
	.hw.init = &(struct clk_init_data){
		.name = "dspa_b_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = a1_dsp_clk_parent_data,
		.num_parents = ARRAY_SIZE(a1_dsp_clk_parent_data),
	},
};

static struct clk_regmap a1_dspa_b_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = DSPA_CLK_CTRL0,
		.shift = 16,
		.width = 10,
	},
	.hw.init = &(struct clk_init_data){
		.name = "dspa_b_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_dspa_b_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap a1_dspa_b = {
	.data = &(struct clk_regmap_gate_data){
		.offset = DSPA_CLK_CTRL0,
		.bit_idx = 29,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "dspa_b",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_dspa_b_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap a1_dspa_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = DSPA_CLK_CTRL0,
		.mask = 0x1,
		.shift = 15,
	},
	.hw.init = &(struct clk_init_data){
		.name = "dspa_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_dspa_a.hw,
			&a1_dspa_b.hw
		},
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap a1_dspa_en_dspa = {
	.data = &(struct clk_regmap_gate_data){
		.offset = DSPA_CLK_EN,
		.bit_idx = 1,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "dspa_en_dspa",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_dspa_sel.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap a1_dspa_en_nic = {
	.data = &(struct clk_regmap_gate_data){
		.offset = DSPA_CLK_EN,
		.bit_idx = 0,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "dspa_en_nic",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_dspa_sel.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap a1_dspb_a_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = DSPB_CLK_CTRL0,
		.mask = 0x7,
		.shift = 10,
		.table = mux_table_dspab_sel,
	},
	.hw.init = &(struct clk_init_data){
		.name = "dspb_a_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = a1_dsp_clk_parent_data,
		.num_parents = ARRAY_SIZE(a1_dsp_clk_parent_data),
	},
};

static struct clk_regmap a1_dspb_a_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = DSPB_CLK_CTRL0,
		.shift = 0,
		.width = 10,
	},
	.hw.init = &(struct clk_init_data){
		.name = "dspb_a_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_dspb_a_sel.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap a1_dspb_a = {
	.data = &(struct clk_regmap_gate_data){
		.offset = DSPB_CLK_CTRL0,
		.bit_idx = 13,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "dspb_a",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_dspb_a_div.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap a1_dspb_b_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = DSPB_CLK_CTRL0,
		.mask = 0x7,
		.shift = 26,
		.table = mux_table_dspab_sel,
	},
	.hw.init = &(struct clk_init_data){
		.name = "dspb_b_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = a1_dsp_clk_parent_data,
		.num_parents = ARRAY_SIZE(a1_dsp_clk_parent_data),
	},
};

static struct clk_regmap a1_dspb_b_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = DSPB_CLK_CTRL0,
		.shift = 16,
		.width = 10,
	},
	.hw.init = &(struct clk_init_data){
		.name = "dspb_b_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_dspb_b_sel.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap a1_dspb_b = {
	.data = &(struct clk_regmap_gate_data){
		.offset = DSPB_CLK_CTRL0,
		.bit_idx = 29,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "dspb_b",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_dspb_b_div.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap a1_dspb_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = DSPB_CLK_CTRL0,
		.mask = 0x1,
		.shift = 15,
	},
	.hw.init = &(struct clk_init_data){
		.name = "dspb_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_dspb_a.hw,
			&a1_dspb_b.hw
		},
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap a1_dspb_en_dspb = {
	.data = &(struct clk_regmap_gate_data){
		.offset = DSPB_CLK_EN,
		.bit_idx = 1,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "dspb_en_dspb",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_dspb_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap a1_dspb_en_nic = {
	.data = &(struct clk_regmap_gate_data){
		.offset = DSPB_CLK_EN,
		.bit_idx = 0,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "dspb_en_nic",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_dspb_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap a1_24m = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLK12_24_CTRL,
		.bit_idx = 11,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "24m",
		.ops = &clk_regmap_gate_ops,
		.parent_data = &(const struct clk_parent_data) {
			.fw_name = "xtal",
		},
		.num_parents = 1,
	},
};

static struct clk_fixed_factor a1_24m_div2 = {
	.mult = 1,
	.div = 2,
	.hw.init = &(struct clk_init_data){
		.name = "24m_div2",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_24m.hw
		},
		.num_parents = 1,
	},
};

static struct clk_regmap a1_12m = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLK12_24_CTRL,
		.bit_idx = 10,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "12m",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_24m_div2.hw
		},
		.num_parents = 1,
	},
};

static struct clk_regmap a1_fclk_div2_divn_pre = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLK12_24_CTRL,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div2_divn_pre",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_fclk_div2.hw
		},
		.num_parents = 1,
	},
};

static struct clk_regmap a1_fclk_div2_divn = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLK12_24_CTRL,
		.bit_idx = 12,
	},
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div2_divn",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_fclk_div2_divn_pre.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static u32 mux_table_gen_sel[] = { 0, 5, 6, 7, 8 };

static const struct clk_parent_data a1_gen_clk_parent_data[] = {
	{ .fw_name = "xtal", },
	{ .hw = &a1_fclk_div2.hw },
	{ .hw = &a1_fclk_div3.hw },
	{ .hw = &a1_fclk_div5.hw },
	{ .hw = &a1_fclk_div7.hw }
};

static struct clk_regmap a1_gen_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = GEN_CLK_CTRL,
		.mask = 0xf,
		.shift = 12,
		.table = mux_table_gen_sel
	},
	.hw.init = &(struct clk_init_data){
		.name = "gen_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = a1_gen_clk_parent_data,
		.num_parents = ARRAY_SIZE(a1_gen_clk_parent_data),
	},
};

static struct clk_regmap a1_gen_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = GEN_CLK_CTRL,
		.shift = 0,
		.width = 11,
	},
	.hw.init = &(struct clk_init_data){
		.name = "gen_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_gen_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap a1_gen = {
	.data = &(struct clk_regmap_gate_data){
		.offset = GEN_CLK_CTRL,
		.bit_idx = 11,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "gen",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_gen_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static const struct clk_parent_data a1_saradc_clk_parent_data[] = {
	{ .fw_name = "xtal", },
	{ .hw = &a1_sys_clk.hw }
};

static struct clk_regmap a1_saradc_sel = {
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
		.parent_data = a1_saradc_clk_parent_data,
		.num_parents = ARRAY_SIZE(a1_saradc_clk_parent_data),
	},
};

static struct clk_regmap a1_saradc_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = SAR_ADC_CLK_CTRL,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "saradc_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_saradc_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap a1_saradc_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = SAR_ADC_CLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "saradc_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_saradc_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

/*
 * add CLK_IGNORE_UNUSED flag for pwm controller GATE
 * clk core will disable unused clock, it may disable
 * vddcore voltage which controlled by one pwm in bl21.
 * add the flag to avoid changing cpu voltage.
 */
static struct clk_regmap a1_pwm_a_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = PWM_CLK_AB_CTRL,
		.mask = 0x1,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pwm_a_sel",
		.ops = &clk_regmap_mux_ops,
		/* parents is the same with saradc sel */
		.parent_data = a1_saradc_clk_parent_data,
		.num_parents = ARRAY_SIZE(a1_saradc_clk_parent_data),
	},
};

static struct clk_regmap a1_pwm_a_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = PWM_CLK_AB_CTRL,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pwm_a_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_pwm_a_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap a1_pwm_a = {
	.data = &(struct clk_regmap_gate_data){
		.offset = PWM_CLK_AB_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "pwm_a",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_pwm_a_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap a1_pwm_b_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = PWM_CLK_AB_CTRL,
		.mask = 0x1,
		.shift = 25,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pwm_b_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = a1_saradc_clk_parent_data,
		.num_parents = ARRAY_SIZE(a1_saradc_clk_parent_data),
	},
};

static struct clk_regmap a1_pwm_b_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = PWM_CLK_AB_CTRL,
		.shift = 16,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pwm_b_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_pwm_b_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap a1_pwm_b = {
	.data = &(struct clk_regmap_gate_data){
		.offset = PWM_CLK_AB_CTRL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "pwm_b",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_pwm_b_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap a1_pwm_c_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = PWM_CLK_CD_CTRL,
		.mask = 0x1,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pwm_c_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = a1_saradc_clk_parent_data,
		.num_parents = ARRAY_SIZE(a1_saradc_clk_parent_data),
	},
};

static struct clk_regmap a1_pwm_c_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = PWM_CLK_CD_CTRL,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pwm_c_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_pwm_c_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap a1_pwm_c = {
	.data = &(struct clk_regmap_gate_data){
		.offset = PWM_CLK_CD_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "pwm_c",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_pwm_c_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap a1_pwm_d_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = PWM_CLK_CD_CTRL,
		.mask = 0x1,
		.shift = 25,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pwm_d_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = a1_saradc_clk_parent_data,
		.num_parents = ARRAY_SIZE(a1_saradc_clk_parent_data),
	},
};

static struct clk_regmap a1_pwm_d_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = PWM_CLK_CD_CTRL,
		.shift = 16,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pwm_d_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_pwm_d_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap a1_pwm_d = {
	.data = &(struct clk_regmap_gate_data){
		.offset = PWM_CLK_CD_CTRL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "pwm_d",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_pwm_d_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static const struct clk_parent_data a1_pwm_ef_parent_data[] = {
	{ .fw_name = "xtal", },
	{ .hw = &a1_sys_clk.hw },
	{ .hw = &a1_fclk_div5.hw },
	{ .hw = &a1_fclk_div7.hw }
};

static struct clk_regmap a1_pwm_e_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = PWM_CLK_EF_CTRL,
		.mask = 0x3,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pwm_e_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = a1_pwm_ef_parent_data,
		.num_parents = ARRAY_SIZE(a1_pwm_ef_parent_data),
	},
};

static struct clk_regmap a1_pwm_e_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = PWM_CLK_EF_CTRL,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pwm_e_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_pwm_e_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap a1_pwm_e = {
	.data = &(struct clk_regmap_gate_data){
		.offset = PWM_CLK_EF_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "pwm_e",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_pwm_e_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap a1_pwm_f_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = PWM_CLK_EF_CTRL,
		.mask = 0x3,
		.shift = 25,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pwm_f_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = a1_pwm_ef_parent_data,
		.num_parents = ARRAY_SIZE(a1_pwm_ef_parent_data),
	},
};

static struct clk_regmap a1_pwm_f_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = PWM_CLK_EF_CTRL,
		.shift = 16,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pwm_f_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_pwm_f_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap a1_pwm_f = {
	.data = &(struct clk_regmap_gate_data){
		.offset = PWM_CLK_EF_CTRL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "pwm_f",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_pwm_f_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

/* spicc clk */

/*    div2   |\         |\       _____
 *  ---------| |---DIV--| |     |     |    spicc out
 *  ---------| |        | |-----| GATE|---------
 *     ..... |/         | /     |_____|
 *  --------------------|/
 *                 24M
 */
static const struct clk_hw *spicc_parent_hws[] = {
	&a1_fclk_div2.hw,
	&a1_fclk_div3.hw,
	&a1_fclk_div5.hw
};

static struct clk_regmap a1_spicc_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = SPICC_CLK_CTRL,
		.mask = 0x3,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "spicc_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = spicc_parent_hws,
		.num_parents = ARRAY_SIZE(spicc_parent_hws),
	},
};

static struct clk_regmap a1_spicc_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = SPICC_CLK_CTRL,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "spicc_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_spicc_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static const struct clk_parent_data a1_spicc_parent_data[] = {
	{ .hw = &a1_spicc_div.hw },
	{ .fw_name = "xtal" }
};

static struct clk_regmap a1_spicc = {
	.data = &(struct clk_regmap_mux_data){
		.offset = SPICC_CLK_CTRL,
		.mask = 0x1,
		.shift = 15,
	},
	.hw.init = &(struct clk_init_data){
		.name = "spicc",
		.ops = &clk_regmap_mux_ops,
		.parent_data = a1_spicc_parent_data,
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap a1_spicc_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = SPICC_CLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "spicc_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_spicc.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap a1_ts_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = TS_CLK_CTRL,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "ts_div",
		.ops = &clk_regmap_divider_ops,
		.parent_data = &(const struct clk_parent_data) {
			.fw_name = "xtal"
		},
		.num_parents = 1,
	},
};

static struct clk_regmap a1_ts = {
	.data = &(struct clk_regmap_gate_data){
		.offset = TS_CLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "ts",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_ts_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static const struct clk_hw *spifc_parent_hws[] = {
	&a1_fclk_div2.hw,
	&a1_fclk_div3.hw,
	&a1_fclk_div5.hw,
	&a1_hifi_pll.hw
};

static struct clk_regmap a1_spifc_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = SPIFC_CLK_CTRL,
		.mask = 0x3,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "spifc_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = spifc_parent_hws,
		.num_parents = ARRAY_SIZE(spifc_parent_hws),
	},
};

static struct clk_regmap a1_spifc_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = SPIFC_CLK_CTRL,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "spifc_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_spifc_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static const struct clk_parent_data a1_spifc_parent_data[] = {
	{ .hw = &a1_spifc_div.hw },
	{ .fw_name = "xtal" }
};

static struct clk_regmap a1_spifc = {
	.data = &(struct clk_regmap_mux_data){
		.offset = SPIFC_CLK_CTRL,
		.mask = 0x1,
		.shift = 15,
	},
	.hw.init = &(struct clk_init_data){
		.name = "spifc",
		.ops = &clk_regmap_mux_ops,
		.parent_data = a1_spifc_parent_data,
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap a1_spifc_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = SPIFC_CLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "spifc_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_spifc.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static const struct clk_parent_data a1_usb_bus_parent_data[] = {
	{ .fw_name = "xtal", },
	{ .hw = &a1_sys_clk.hw },
	{ .hw = &a1_fclk_div3.hw },
	{ .hw = &a1_fclk_div5.hw }
};

static struct clk_regmap a1_usb_bus_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = USB_BUSCLK_CTRL,
		.mask = 0x3,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "usb_bus_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = a1_usb_bus_parent_data,
		.num_parents = ARRAY_SIZE(a1_usb_bus_parent_data),
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap a1_usb_bus_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = USB_BUSCLK_CTRL,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "usb_bus_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_usb_bus_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap a1_usb_bus = {
	.data = &(struct clk_regmap_gate_data){
		.offset = USB_BUSCLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "usb_bus",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_usb_bus_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static u32 mux_table_sd_emmc[] = { 0, 1, 2 };
/* delete the forth parent : hifi_pll
 * hifi pll only work for Audio
 */
static const struct clk_hw *a1_sd_emmc_parent_hws[] = {
	&a1_fclk_div2.hw,
	&a1_fclk_div3.hw,
	&a1_fclk_div5.hw
};

static struct clk_regmap a1_sd_emmc_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = SD_EMMC_CLK_CTRL,
		.mask = 0x3,
		.shift = 9,
		.table = mux_table_sd_emmc,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sd_emmc_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = a1_sd_emmc_parent_hws,
		.num_parents = ARRAY_SIZE(a1_sd_emmc_parent_hws),
	},
};

static struct clk_regmap a1_sd_emmc_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = SD_EMMC_CLK_CTRL,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sd_emmc_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_sd_emmc_sel.hw
		},
		.num_parents = 1,
	},
};

static const struct clk_parent_data a1_sd_emmc_parent_data[] = {
	{ .hw = &a1_sd_emmc_div.hw },
	{ .fw_name = "xtal" }
};

static struct clk_regmap a1_sd_emmc = {
	.data = &(struct clk_regmap_mux_data){
		.offset = SD_EMMC_CLK_CTRL,
		.mask = 0x1,
		.shift = 15,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sd_emmc",
		.ops = &clk_regmap_mux_ops,
		.parent_data = a1_sd_emmc_parent_data,
		.num_parents = 2,
	},
};

static struct clk_regmap a1_sd_emmc_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = SD_EMMC_CLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "sd_emmc_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_sd_emmc.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap a1_psram_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = PSRAM_CLK_CTRL,
		.mask = 0x3,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "psram_sel",
		.ops = &clk_regmap_mux_ops,
		/* share the same parent with emmc*/
		.parent_hws = a1_sd_emmc_parent_hws,
		.num_parents = ARRAY_SIZE(a1_sd_emmc_parent_hws),
	},
};

static struct clk_regmap a1_psram_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = PSRAM_CLK_CTRL,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "psram_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_psram_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static const struct clk_parent_data a1_psram_parent_data[] = {
	{ .hw = &a1_psram_div.hw },
	{ .fw_name = "xtal" }
};

static struct clk_regmap a1_psram = {
	.data = &(struct clk_regmap_mux_data){
		.offset = PSRAM_CLK_CTRL,
		.mask = 0x1,
		.shift = 15,
	},
	.hw.init = &(struct clk_init_data){
		.name = "psram",
		.ops = &clk_regmap_mux_ops,
		.parent_data = a1_psram_parent_data,
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap a1_psram_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = PSRAM_CLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "psram_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_psram.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap a1_dmc_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = DMC_CLK_CTRL,
		.mask = 0x3,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "dmc_sel",
		.ops = &clk_regmap_mux_ops,
		/* share the same parent with sd emmc */
		.parent_hws = a1_sd_emmc_parent_hws,
		.num_parents = ARRAY_SIZE(a1_sd_emmc_parent_hws),
	},
};

static struct clk_regmap a1_dmc_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = DMC_CLK_CTRL,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "dmc_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_dmc_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static const struct clk_parent_data a1_dmc_parent_data[] = {
	{ .hw = &a1_psram_div.hw },
	{ .fw_name = "xtal" }
};

static struct clk_regmap a1_dmc = {
	.data = &(struct clk_regmap_mux_data){
		.offset = DMC_CLK_CTRL,
		.mask = 0x1,
		.shift = 15,
	},
	.hw.init = &(struct clk_init_data){
		.name = "dmc",
		.ops = &clk_regmap_mux_ops,
		.parent_data = a1_dmc_parent_data,
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap a1_dmc_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = DMC_CLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "dmc_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_dmc.hw
		},
		.num_parents = 1,
		/* add CLK_IGNORE_UNUSED to avoid hangup */
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

/*
 * change fixed pll 1536M to 768M
 */
static struct clk_regmap a1_fixed_pll_div2_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = FCLK_DIV1_SEL,
		.bit_idx = 0,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "fixed_div2_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_fixed_pll.hw
		},
		.num_parents = 1,
		/*
		 * add CLK_IGNORE_UNUSED to avoid fixed pll disable
		 * by clk core or enable it in clock driver.
		 * disable fixed_pll_div2_gate-->disable fixed pll
		 */
		.flags = CLK_IGNORE_UNUSED,
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
static struct clk_regmap a1_rtc_32k_clkin = {
	.data = &(struct clk_regmap_gate_data){
		.offset = RTC_BY_OSCIN_CTRL0,
		.bit_idx = 31,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "rtc_32k_clkin",
		.ops = &clk_regmap_gate_ops,
		.parent_data = &(const struct clk_parent_data) {
			.fw_name = "xtal",
		},
		.num_parents = 1,
	},
};

static const struct meson_clk_dualdiv_param a1_32k_div_table[] = {
	{
		.dual		= 1,
		.n1		= 733,
		.m1		= 8,
		.n2		= 732,
		.m2		= 11,
	}, {}
};

static struct clk_regmap a1_rtc_32k_div = {
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
		.table = a1_32k_div_table,
	},
	.hw.init = &(struct clk_init_data){
		.name = "rtc_32k_div",
		.ops = &meson_clk_dualdiv_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_rtc_32k_clkin.hw
		},
		.num_parents = 1,
	},
};

static struct clk_regmap a1_rtc_32k_xtal = {
	.data = &(struct clk_regmap_gate_data){
		.offset = RTC_BY_OSCIN_CTRL1,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "rtc_32k_xtal",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_rtc_32k_clkin.hw
		},
		.num_parents = 1,
	},
};

static struct clk_regmap a1_rtc_32k_sel = {
	.data = &(struct clk_regmap_mux_data) {
		.offset = RTC_CTRL,
		.mask = 0x3,
		.shift = 0,
		.flags = CLK_MUX_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "rtc_32k_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_rtc_32k_xtal.hw,
			&a1_rtc_32k_div.hw
		},
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap a1_rtc_clk = {
	.data = &(struct clk_regmap_gate_data){
		.offset = RTC_BY_OSCIN_CTRL0,
		.bit_idx = 30,
	},
	.hw.init = &(struct clk_init_data){
		.name = "rtc_clk",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_rtc_32k_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap a1_ceca_32k_clkin = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CECA_CLK_CTRL0,
		.bit_idx = 31,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "ceca_32k_clkin",
		.ops = &clk_regmap_gate_ops,
		.parent_data = &(const struct clk_parent_data) {
			.fw_name = "xtal"
		},
		.num_parents = 1,
	},
};

static struct clk_regmap a1_ceca_32k_div = {
	.data = &(struct meson_clk_dualdiv_data){
		.n1 = {
			.reg_off = CECA_CLK_CTRL0,
			.shift   = 0,
			.width   = 12,
		},
		.n2 = {
			.reg_off = CECA_CLK_CTRL0,
			.shift   = 12,
			.width   = 12,
		},
		.m1 = {
			.reg_off = CECA_CLK_CTRL1,
			.shift   = 0,
			.width   = 12,
		},
		.m2 = {
			.reg_off = CECA_CLK_CTRL1,
			.shift   = 12,
			.width   = 12,
		},
		.dual = {
			.reg_off = CECA_CLK_CTRL0,
			.shift   = 28,
			.width   = 1,
		},
		.table = a1_32k_div_table,
	},
	.hw.init = &(struct clk_init_data){
		.name = "ceca_32k_div",
		.ops = &meson_clk_dualdiv_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_ceca_32k_clkin.hw
		},
		.num_parents = 1,
	},
};

static struct clk_regmap a1_ceca_32k_sel_pre = {
	.data = &(struct clk_regmap_mux_data) {
		.offset = CECA_CLK_CTRL1,
		.mask = 0x1,
		.shift = 24,
		.flags = CLK_MUX_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "ceca_32k_sel_pre",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_ceca_32k_div.hw,
			&a1_ceca_32k_clkin.hw
		},
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap a1_ceca_32k_sel = {
	.data = &(struct clk_regmap_mux_data) {
		.offset = CECA_CLK_CTRL1,
		.mask = 0x1,
		.shift = 31,
		.flags = CLK_MUX_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "ceca_32k_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_ceca_32k_sel_pre.hw,
			&a1_rtc_clk.hw
		},
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap a1_ceca_32k_clkout = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CECA_CLK_CTRL0,
		.bit_idx = 30,
	},
	.hw.init = &(struct clk_init_data){
		.name = "ceca_32k_clkout",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_ceca_32k_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap a1_cecb_32k_clkin = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CECB_CLK_CTRL0,
		.bit_idx = 31,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "cecb_32k_clkin",
		.ops = &clk_regmap_gate_ops,
		.parent_data = &(const struct clk_parent_data) {
			.fw_name = "xtal"
		},
		.num_parents = 1,
	},
};

static struct clk_regmap a1_cecb_32k_div = {
	.data = &(struct meson_clk_dualdiv_data){
		.n1 = {
			.reg_off = CECB_CLK_CTRL0,
			.shift   = 0,
			.width   = 12,
		},
		.n2 = {
			.reg_off = CECB_CLK_CTRL0,
			.shift   = 12,
			.width   = 12,
		},
		.m1 = {
			.reg_off = CECB_CLK_CTRL1,
			.shift   = 0,
			.width   = 12,
		},
		.m2 = {
			.reg_off = CECB_CLK_CTRL1,
			.shift   = 12,
			.width   = 12,
		},
		.dual = {
			.reg_off = CECB_CLK_CTRL0,
			.shift   = 28,
			.width   = 1,
		},
		.table = a1_32k_div_table,
	},
	.hw.init = &(struct clk_init_data){
		.name = "cecb_32k_div",
		.ops = &meson_clk_dualdiv_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_cecb_32k_clkin.hw
		},
		.num_parents = 1,
	},
};

static struct clk_regmap a1_cecb_32k_sel_pre = {
	.data = &(struct clk_regmap_mux_data) {
		.offset = CECB_CLK_CTRL1,
		.mask = 0x1,
		.shift = 24,
		.flags = CLK_MUX_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "cecb_32k_sel_pre",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_cecb_32k_div.hw,
			&a1_cecb_32k_clkin.hw
		},
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap a1_cecb_32k_sel = {
	.data = &(struct clk_regmap_mux_data) {
		.offset = CECB_CLK_CTRL1,
		.mask = 0x1,
		.shift = 31,
		.flags = CLK_MUX_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "cecb_32k_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_cecb_32k_sel_pre.hw,
			&a1_rtc_clk.hw
		},
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap a1_cecb_32k_clkout = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CECB_CLK_CTRL0,
		.bit_idx = 30,
	},
	.hw.init = &(struct clk_init_data){
		.name = "cecb_32k_clkout",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_cecb_32k_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static u32 cpu_fixed_source_sel_table[]	= { 0, 1, 2 };

static const struct clk_parent_data a1_fixed_source_sel_parent_data[] = {
	{ .fw_name = "xtal", },
	{ .hw = &a1_fclk_div2.hw },
	{ .hw = &a1_fclk_div3.hw }
};

/* cpu_fixed_sel0 */
static struct clk_regmap a1_cpu_fixed_source_sel0 = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CPUCTRL_CLK_CTRL0,
		.mask = 0x3,
		.shift = 0,
		.table = cpu_fixed_source_sel_table,
	},
	.hw.init = &(struct clk_init_data){
		.name = "cpu_fixed_source_sel0",
		.ops = &clk_regmap_mux_ops,
		.parent_data = a1_fixed_source_sel_parent_data,
		.num_parents = ARRAY_SIZE(a1_fixed_source_sel_parent_data),
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap a1_cpu_fixed_source_div0 = {
	.data = &(struct clk_regmap_div_data){
		.offset = CPUCTRL_CLK_CTRL0,
		.shift = 4,
		.width = 6,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "cpu_fixed_source_div0",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_cpu_fixed_source_sel0.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap a1_cpu_fixed_sel0 = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CPUCTRL_CLK_CTRL0,
		.mask = 0x1,
		.shift = 2,
	},
	.hw.init = &(struct clk_init_data){
		.name = "cpu_fixed_sel0",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_cpu_fixed_source_sel0.hw,
			&a1_cpu_fixed_source_div0.hw
		},
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

/* cpu_fixed_sel1 */
static struct clk_regmap a1_cpu_fixed_source_sel1 = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CPUCTRL_CLK_CTRL0,
		.mask = 0x3,
		.shift = 16,
		.table = cpu_fixed_source_sel_table,
	},
	.hw.init = &(struct clk_init_data){
		.name = "cpu_fixed_source_sel1",
		.ops = &clk_regmap_mux_ops,
		.parent_data = a1_fixed_source_sel_parent_data,
		.num_parents = ARRAY_SIZE(a1_fixed_source_sel_parent_data),
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap a1_cpu_fixed_source_div1 = {
	.data = &(struct clk_regmap_div_data){
		.offset = CPUCTRL_CLK_CTRL0,
		.shift = 20,
		.width = 6,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "cpu_fixed_source_div1",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_cpu_fixed_source_sel1.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap a1_cpu_fixed_sel1 = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CPUCTRL_CLK_CTRL0,
		.mask = 0x1,
		.shift = 18,
	},
	.hw.init = &(struct clk_init_data){
		.name = "cpu_fixed_sel1",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_cpu_fixed_source_sel1.hw,
			&a1_cpu_fixed_source_div1.hw
		},
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

/* cpu_fixed_clk */
static struct clk_regmap a1_cpu_fixed_clk = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CPUCTRL_CLK_CTRL0,
		.mask = 0x1,
		.shift = 10,
	},
	.hw.init = &(struct clk_init_data){
		.name = "cpu_fixed_clk",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_cpu_fixed_sel0.hw,
			&a1_cpu_fixed_sel1.hw
		},
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT | CLK_SET_RATE_NO_REPARENT,
	},
};

/* cpu clocks */
/* cpu_fixed_clk |\
 *---------------| \     cts_cpu_clk
 *  sys_pll      |  |--------
 *---------------| /
 *               |/
 */

static struct clk_regmap a1_cpu_clk = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CPUCTRL_CLK_CTRL0,
		.mask = 0x1,
		.shift = 11,
	},
	.hw.init = &(struct clk_init_data){
		.name = "cpu_clk",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&a1_cpu_fixed_clk.hw,
			&a1_sys_pll.hw
		},
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

/* Array of all clocks provided by this provider */
static struct clk_hw_onecell_data a1_hw_onecell_data = {
	.hws = {
		[CLKID_FIXED_PLL_DCO]		= &a1_fixed_pll_dco.hw,
		[CLKID_FIXED_PLL]		= &a1_fixed_pll.hw,
		[CLKID_FCLK_DIV2]		= &a1_fclk_div2.hw,
		[CLKID_FCLK_DIV3]		= &a1_fclk_div3.hw,
		[CLKID_FCLK_DIV5]		= &a1_fclk_div5.hw,
		[CLKID_FCLK_DIV7]		= &a1_fclk_div7.hw,
		[CLKID_FCLK_DIV2_DIV]		= &a1_fclk_div2_div.hw,
		[CLKID_FCLK_DIV3_DIV]		= &a1_fclk_div3_div.hw,
		[CLKID_FCLK_DIV5_DIV]		= &a1_fclk_div5_div.hw,
		[CLKID_FCLK_DIV7_DIV]		= &a1_fclk_div7_div.hw,
		[CLKID_SYS_B_SEL]		= &a1_sys_b_sel.hw,
		[CLKID_SYS_B_DIV]		= &a1_sys_b_div.hw,
		[CLKID_SYS_B]			= &a1_sys_b.hw,
		[CLKID_SYS_A_SEL]		= &a1_sys_a_sel.hw,
		[CLKID_SYS_A_DIV]		= &a1_sys_a_div.hw,
		[CLKID_SYS_A]			= &a1_sys_a.hw,
		[CLKID_SYS_CLK]			= &a1_sys_clk.hw,
		[CLKID_HIFI_PLL]		= &a1_hifi_pll.hw,
		[CLKID_SYS_PLL]			= &a1_sys_pll.hw,
		[CLKID_CPU_FSOURCE_SEL0]	=
						&a1_cpu_fixed_source_sel0.hw,
		[CLKID_CPU_FSOURCE_DIV0]	=
						&a1_cpu_fixed_source_div0.hw,
		[CLKID_CPU_FSEL0]		= &a1_cpu_fixed_sel0.hw,
		[CLKID_CPU_FSOURCE_SEL1]	=
						&a1_cpu_fixed_source_sel1.hw,
		[CLKID_CPU_FSOURCE_DIV1]	=
						&a1_cpu_fixed_source_div1.hw,
		[CLKID_CPU_FSEL1]	= &a1_cpu_fixed_sel1.hw,
		[CLKID_CPU_FCLK]	= &a1_cpu_fixed_clk.hw,
		[CLKID_CPU_CLK]		= &a1_cpu_clk.hw,
		[CLKID_XTAL_CLKTREE]	= &xtal_clktree.hw,
		[CLKID_XTAL_FIXPLL]	= &xtal_fixpll.hw,
		[CLKID_XTAL_USB_PHY]	= &xtal_usb_phy.hw,
		[CLKID_XTAL_USB_CTRL]	= &xtal_usb_ctrl.hw,
		[CLKID_XTAL_HIFIPLL]	= &xtal_hifipll.hw,
		[CLKID_XTAL_DDS]	= &xtal_dds.hw,
		[CLKID_CLKTREE]		= &sys_clk_clk_tree.hw,
		[CLKID_RESET_CTRL]	= &sys_clk_reset_ctrl.hw,
		[CLKID_ANALOG_CTRL]	= &sys_clk_analog_ctrl.hw,
		[CLKID_PWR_CTRL]	= &sys_clk_pwr_ctrl.hw,
		[CLKID_PAD_CTRL]	= &sys_clk_pad_ctrl.hw,
		[CLKID_SYS_CTRL]	= &sys_clk_sys_ctrl.hw,
		[CLKID_TEMP_SENSOR]	= &sys_clk_temp_sensor.hw,
		[CLKID_AM2AXI_DIV]	= &sys_clk_am2axi_dev.hw,
		[CLKID_SPICC_B]		= &sys_clk_spicc_b.hw,
		[CLKID_SPICC_A]		= &sys_clk_spicc_a.hw,
		[CLKID_CLK_MSR]		= &sys_clk_clk_msr.hw,
		[CLKID_AUDIO]		= &sys_clk_audio.hw,
		[CLKID_JTAG_CTRL]	= &sys_clk_jtag_ctrl.hw,
		[CLKID_SARADC]		= &sys_clk_saradc.hw,
		[CLKID_PWM_EF]		= &sys_clk_pwm_ef.hw,
		[CLKID_PWM_CD]		= &sys_clk_pwm_cd.hw,
		[CLKID_PWM_AB]		= &sys_clk_pwm_ab.hw,
		[CLKID_CEC]		= &sys_clk_cec.hw,
		[CLKID_I2C_S]		= &sys_clk_i2c_s.hw,
		[CLKID_IR_CTRL]		= &sys_clk_ir_ctrl.hw,
		[CLKID_I2C_M_D]		= &sys_clk_i2c_m_d.hw,
		[CLKID_I2C_M_C]		= &sys_clk_i2c_m_c.hw,
		[CLKID_I2C_M_B]		= &sys_clk_i2c_m_b.hw,
		[CLKID_I2C_M_A]		= &sys_clk_i2c_m_a.hw,
		[CLKID_ACODEC]		= &sys_clk_acodec.hw,
		[CLKID_OTP]		= &sys_clk_otp.hw,
		[CLKID_SD_EMMC_A]	= &sys_clk_sd_emmc_a.hw,
		[CLKID_USB_PHY]		= &sys_clk_usb_phy.hw,
		[CLKID_USB_CTRL]	= &sys_clk_usb_ctrl.hw,
		[CLKID_SYS_DSPB]	= &sys_clk_sys_dspb.hw,
		[CLKID_SYS_DSPA]	= &sys_clk_sys_dspa.hw,
		[CLKID_DMA]		= &sys_clk_dma.hw,
		[CLKID_IRQ_CTRL]	= &sys_clk_irq_ctrl.hw,
		[CLKID_NIC]		= &sys_clk_nic.hw,
		[CLKID_GIC]		= &sys_clk_gic.hw,
		[CLKID_UART_C]		= &sys_clk_uart_c.hw,
		[CLKID_UART_B]		= &sys_clk_uart_b.hw,
		[CLKID_UART_A]		= &sys_clk_uart_a.hw,
		[CLKID_SYS_PSRAM]	= &sys_clk_sys_psram.hw,
		[CLKID_RSA]		= &sys_clk_rsa.hw,
		[CLKID_CORESIGHT]	= &sys_clk_coresight.hw,
		[CLKID_AM2AXI_VAD]	= &sys_clk_am2axi_vad.hw,
		[CLKID_AUDIO_VAD]	= &sys_clk_audio_vad.hw,
		[CLKID_AXI_DMC]		= &sys_clk_axi_dmc.hw,
		[CLKID_AXI_PSRAM]	= &sys_clk_axi_psram.hw,
		[CLKID_RAMB]		= &sys_clk_ramb.hw,
		[CLKID_RAMA]		= &sys_clk_rama.hw,
		[CLKID_AXI_SPIFC]	= &sys_clk_axi_spifc.hw,
		[CLKID_AXI_NIC]		= &sys_clk_axi_nic.hw,
		[CLKID_AXI_DMA]		= &sys_clk_axi_dma.hw,
		[CLKID_CPU_CTRL]	= &sys_clk_cpu_ctrl.hw,
		[CLKID_ROM]		= &sys_clk_rom.hw,
		[CLKID_PROC_I2C]	= &sys_clk_prod_i2c.hw,
		[CLKID_DSPA_A_SEL]	= &a1_dspa_a_sel.hw,
		[CLKID_DSPA_A_DIV]	= &a1_dspa_a_div.hw,
		[CLKID_DSPA_A]		= &a1_dspa_a.hw,
		[CLKID_DSPA_B_SEL]	= &a1_dspa_b_sel.hw,
		[CLKID_DSPA_B_DIV]	= &a1_dspa_b_div.hw,
		[CLKID_DSPA_B]		= &a1_dspa_b.hw,
		[CLKID_DSPA_SEL]	= &a1_dspa_sel.hw,
		[CLKID_DSPB_A_SEL]	= &a1_dspb_a_sel.hw,
		[CLKID_DSPB_A_DIV]	= &a1_dspb_a_div.hw,
		[CLKID_DSPB_A]		= &a1_dspb_a.hw,
		[CLKID_DSPB_B_SEL]	= &a1_dspb_b_sel.hw,
		[CLKID_DSPB_B_DIV]	= &a1_dspb_b_div.hw,
		[CLKID_DSPB_B]		= &a1_dspb_b.hw,
		[CLKID_DSPB_SEL]	= &a1_dspb_sel.hw,
		[CLKID_DSPA_EN_DSPA]	= &a1_dspa_en_dspa.hw,
		[CLKID_DSPA_EN_NIC]	= &a1_dspa_en_nic.hw,
		[CLKID_DSPB_EN_DSPB]	= &a1_dspb_en_dspb.hw,
		[CLKID_DSPB_EN_NIC]	= &a1_dspb_en_nic.hw,
		[CLKID_24M]		= &a1_24m.hw,
		[CLKID_24M_DIV2]	= &a1_24m_div2.hw,
		[CLKID_12M]		= &a1_12m.hw,
		[CLKID_DIV2_PRE]	= &a1_fclk_div2_divn_pre.hw,
		[CLKID_FCLK_DIV2_DIVN]	= &a1_fclk_div2_divn.hw,
		[CLKID_GEN_SEL]		= &a1_gen_sel.hw,
		[CLKID_GEN_DIV]		= &a1_gen_div.hw,
		[CLKID_GEN]		= &a1_gen.hw,
		[CLKID_SARADC_SEL]	= &a1_saradc_sel.hw,
		[CLKID_SARADC_DIV]	= &a1_saradc_div.hw,
		[CLKID_SARADC_GATE]	= &a1_saradc_gate.hw,
		[CLKID_PWM_A_SEL]	= &a1_pwm_a_sel.hw,
		[CLKID_PWM_A_DIV]	= &a1_pwm_a_div.hw,
		[CLKID_PWM_A]		= &a1_pwm_a.hw,
		[CLKID_PWM_B_SEL]	= &a1_pwm_b_sel.hw,
		[CLKID_PWM_B_DIV]	= &a1_pwm_b_div.hw,
		[CLKID_PWM_B]		= &a1_pwm_b.hw,
		[CLKID_PWM_C_SEL]	= &a1_pwm_c_sel.hw,
		[CLKID_PWM_C_DIV]	= &a1_pwm_c_div.hw,
		[CLKID_PWM_C]		= &a1_pwm_c.hw,
		[CLKID_PWM_D_SEL]	= &a1_pwm_d_sel.hw,
		[CLKID_PWM_D_DIV]	= &a1_pwm_d_div.hw,
		[CLKID_PWM_D]		= &a1_pwm_d.hw,
		[CLKID_PWM_E_SEL]	= &a1_pwm_e_sel.hw,
		[CLKID_PWM_E_DIV]	= &a1_pwm_e_div.hw,
		[CLKID_PWM_E]		= &a1_pwm_e.hw,
		[CLKID_PWM_F_SEL]	= &a1_pwm_f_sel.hw,
		[CLKID_PWM_F_DIV]	= &a1_pwm_f_div.hw,
		[CLKID_PWM_F]		= &a1_pwm_f.hw,
		[CLKID_SPICC_SEL]	= &a1_spicc_sel.hw,
		[CLKID_SPICC_DIV]	= &a1_spicc_div.hw,
		[CLKID_SPICC_GATE]	= &a1_spicc_gate.hw,
		[CLKID_SPICC]		= &a1_spicc.hw,
		[CLKID_TS_DIV]		= &a1_ts_div.hw,
		[CLKID_TS]		= &a1_ts.hw,
		[CLKID_SPIFC_SEL]	= &a1_spifc_sel.hw,
		[CLKID_SPIFC_DIV]	= &a1_spifc_div.hw,
		[CLKID_SPIFC_GATE]	= &a1_spifc_gate.hw,
		[CLKID_SPIFC]		= &a1_spifc.hw,
		[CLKID_USB_BUS_SEL]	= &a1_usb_bus_sel.hw,
		[CLKID_USB_BUS_DIV]	= &a1_usb_bus_div.hw,
		[CLKID_USB_BUS]		= &a1_usb_bus.hw,
		[CLKID_SD_EMMC_SEL]	= &a1_sd_emmc_sel.hw,
		[CLKID_SD_EMMC_DIV]	= &a1_sd_emmc_div.hw,
		[CLKID_SD_EMMC_GATE]	= &a1_sd_emmc_gate.hw,
		[CLKID_SD_EMMC]		= &a1_sd_emmc.hw,
		[CLKID_PSRAM_SEL]	= &a1_psram_sel.hw,
		[CLKID_PSRAM_DIV]	= &a1_psram_div.hw,
		[CLKID_PSRAM_GATE]	= &a1_psram_gate.hw,
		[CLKID_PSRAM]		= &a1_psram.hw,
		[CLKID_DMC_SEL]		= &a1_dmc_sel.hw,
		[CLKID_DMC_DIV]		= &a1_dmc_div.hw,
		[CLKID_DMC_GATE]	= &a1_dmc_gate.hw,
		[CLKID_DMC]		= &a1_dmc.hw,
		[CLKID_FIXED_PLL_DIV2]	= &a1_fixed_pll_div2_gate.hw,
		[CLKID_RTC_32K_CLKIN]   = &a1_rtc_32k_clkin.hw,
		[CLKID_RTC_32K_DIV]	= &a1_rtc_32k_div.hw,
		[CLKID_RTC_32K_XTAL]	= &a1_rtc_32k_xtal.hw,
		[CLKID_RTC_32K_SEL]	= &a1_rtc_32k_sel.hw,
		[CLKID_RTC_CLK]		= &a1_rtc_clk.hw,
		[CLKID_CECA_32K_CLKIN]	= &a1_ceca_32k_clkin.hw,
		[CLKID_CECA_32K_DIV]	= &a1_ceca_32k_div.hw,
		[CLKID_CECA_32K_SEL_PRE] = &a1_ceca_32k_sel_pre.hw,
		[CLKID_CECA_32K_SEL]	= &a1_ceca_32k_sel.hw,
		[CLKID_CECA_32K]	= &a1_ceca_32k_clkout.hw,
		[CLKID_CECB_32K_CLKIN]	= &a1_cecb_32k_clkin.hw,
		[CLKID_CECB_32K_DIV]	= &a1_cecb_32k_div.hw,
		[CLKID_CECB_32K_SEL_PRE] = &a1_cecb_32k_sel_pre.hw,
		[CLKID_CECB_32K_SEL]	= &a1_cecb_32k_sel.hw,
		[CLKID_CECB_32K]	= &a1_cecb_32k_clkout.hw,
		[NR_CLKS]		= NULL,
	},
	.num = NR_CLKS,
};

static struct clk_regmap *const a1_clk_regmaps[] = {
	&xtal_clktree,
	&xtal_fixpll,
	&xtal_usb_phy,
	&xtal_usb_ctrl,
	&xtal_hifipll,
	&xtal_dds,
	&sys_clk_clk_tree,
	&sys_clk_reset_ctrl,
	&sys_clk_analog_ctrl,
	&sys_clk_pwr_ctrl,
	&sys_clk_pad_ctrl,
	&sys_clk_sys_ctrl,
	&sys_clk_temp_sensor,
	&sys_clk_am2axi_dev,
	&sys_clk_spicc_b,
	&sys_clk_spicc_a,
	&sys_clk_clk_msr,
	&sys_clk_audio,
	&sys_clk_jtag_ctrl,
	&sys_clk_saradc,
	&sys_clk_pwm_ef,
	&sys_clk_pwm_cd,
	&sys_clk_pwm_ab,
	&sys_clk_cec,
	&sys_clk_i2c_s,
	&sys_clk_ir_ctrl,
	&sys_clk_i2c_m_d,
	&sys_clk_i2c_m_c,
	&sys_clk_i2c_m_b,
	&sys_clk_i2c_m_a,
	&sys_clk_acodec,
	&sys_clk_otp,
	&sys_clk_sd_emmc_a,
	&sys_clk_usb_phy,
	&sys_clk_usb_ctrl,
	&sys_clk_sys_dspb,
	&sys_clk_sys_dspa,
	&sys_clk_dma,
	&sys_clk_irq_ctrl,
	&sys_clk_nic,
	&sys_clk_gic,
	&sys_clk_uart_c,
	&sys_clk_uart_b,
	&sys_clk_uart_a,
	&sys_clk_sys_psram,
	&sys_clk_rsa,
	&sys_clk_coresight,
	&sys_clk_am2axi_vad,
	&sys_clk_audio_vad,
	&sys_clk_axi_dmc,
	&sys_clk_axi_psram,
	&sys_clk_ramb,
	&sys_clk_rama,
	&sys_clk_axi_spifc,
	&sys_clk_axi_nic,
	&sys_clk_axi_dma,
	&sys_clk_cpu_ctrl,
	&sys_clk_rom,
	&sys_clk_prod_i2c,
	&a1_dspa_a_sel,
	&a1_dspa_a_div,
	&a1_dspa_a,
	&a1_dspa_b_sel,
	&a1_dspa_b_div,
	&a1_dspa_b,
	&a1_dspa_sel,
	&a1_dspb_a_sel,
	&a1_dspb_a_div,
	&a1_dspb_a,
	&a1_dspb_b_sel,
	&a1_dspb_b_div,
	&a1_dspb_b,
	&a1_dspb_sel,
	&a1_dspa_en_dspa,
	&a1_dspa_en_nic,
	&a1_dspb_en_dspb,
	&a1_dspb_en_nic,
	&a1_24m,
	&a1_12m,
	&a1_fclk_div2_divn_pre,
	&a1_fclk_div2_divn,
	&a1_gen_sel,
	&a1_gen_div,
	&a1_gen,
	&a1_saradc_sel,
	&a1_saradc_div,
	&a1_saradc_gate,
	&a1_pwm_a_sel,
	&a1_pwm_a_div,
	&a1_pwm_a,
	&a1_pwm_b_sel,
	&a1_pwm_b_div,
	&a1_pwm_b,
	&a1_pwm_c_sel,
	&a1_pwm_c_div,
	&a1_pwm_c,
	&a1_pwm_d_sel,
	&a1_pwm_d_div,
	&a1_pwm_d,
	&a1_pwm_e_sel,
	&a1_pwm_e_div,
	&a1_pwm_e,
	&a1_pwm_f_sel,
	&a1_pwm_f_div,
	&a1_pwm_f,
	&a1_spicc_sel,
	&a1_spicc_div,
	&a1_spicc_gate,
	&a1_spicc,
	&a1_ts_div,
	&a1_ts,
	&a1_spifc_sel,
	&a1_spifc_div,
	&a1_spifc_gate,
	&a1_spifc,
	&a1_usb_bus_sel,
	&a1_usb_bus_div,
	&a1_usb_bus,
	&a1_sd_emmc_sel,
	&a1_sd_emmc_div,
	&a1_sd_emmc_gate,
	&a1_sd_emmc,
	&a1_psram_sel,
	&a1_psram_div,
	&a1_psram_gate,
	&a1_psram,
	&a1_dmc_sel,
	&a1_dmc_div,
	&a1_dmc_gate,
	&a1_dmc,
	&a1_fixed_pll_div2_gate,
	&a1_sys_b_sel,
	&a1_sys_b_div,
	&a1_sys_b,
	&a1_sys_a_sel,
	&a1_sys_a_div,
	&a1_sys_a,
	&a1_sys_clk,
	&a1_rtc_32k_clkin,
	&a1_rtc_32k_div,
	&a1_rtc_32k_xtal,
	&a1_rtc_32k_sel,
	&a1_rtc_clk,
	&a1_ceca_32k_clkin,
	&a1_ceca_32k_div,
	&a1_ceca_32k_sel_pre,
	&a1_ceca_32k_sel,
	&a1_ceca_32k_clkout,
	&a1_cecb_32k_clkin,
	&a1_cecb_32k_div,
	&a1_cecb_32k_sel_pre,
	&a1_cecb_32k_sel,
	&a1_cecb_32k_clkout,
};

static struct clk_regmap *const a1_cpu_clk_regmaps[] = {
	&a1_cpu_fixed_source_sel0,
	&a1_cpu_fixed_source_div0,
	&a1_cpu_fixed_sel0,
	&a1_cpu_fixed_source_sel1,
	&a1_cpu_fixed_source_div1,
	&a1_cpu_fixed_sel1,
	&a1_cpu_fixed_clk,
	&a1_cpu_clk,
};

static struct clk_regmap *const a1_pll_clk_regmaps[] = {
	&a1_fixed_pll_dco,
	&a1_fixed_pll,
	&a1_fclk_div2,
	&a1_fclk_div3,
	&a1_fclk_div5,
	&a1_fclk_div7,
	&a1_hifi_pll,
	&a1_sys_pll,
};

struct a1_sys_pll_nb_data {
	struct notifier_block nb;
	struct clk_hw *sys_pll;
	struct clk_hw *cpu_clk;
	struct clk_hw *cpu_dyn_clk;
};

static int a1_sys_pll_notifier_cb(struct notifier_block *nb, unsigned long event, void *data)
{
	struct a1_sys_pll_nb_data *nb_data =
		container_of(nb, struct a1_sys_pll_nb_data, nb);

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

static struct a1_sys_pll_nb_data a1_sys_pll_nb_data = {
	.sys_pll = &a1_sys_pll.hw,
	.cpu_clk = &a1_cpu_clk.hw,
	.cpu_dyn_clk = &a1_cpu_fixed_clk.hw,
	.nb.notifier_call = a1_sys_pll_notifier_cb,
};

struct a1_nb_data {
	struct notifier_block nb;
	struct clk_hw_onecell_data *onecell_data;
};

static int a1_cpu_fixed_clk_notifier_cb(struct notifier_block *nb,
					unsigned long event, void *data)
{
	struct clk_notifier_data *ndata = data;
	struct clk *cpu_fixed_clk, *parent_clk;
	int ret;

	switch (event) {
	case PRE_RATE_CHANGE:
	parent_clk = a1_cpu_fixed_sel1.hw.clk;
	ret = clk_set_rate(parent_clk, ndata->new_rate);
	if (ret)
		pr_err("set fixed sel1 to new rate failed\n");
		break;
	case POST_RATE_CHANGE:
	parent_clk = a1_cpu_fixed_sel0.hw.clk;
		break;
	default:
		return NOTIFY_DONE;
	}

	cpu_fixed_clk = a1_cpu_fixed_clk.hw.clk;

	ret = clk_set_parent(cpu_fixed_clk, parent_clk);
	if (ret)
		return notifier_from_errno(ret);

	return NOTIFY_OK;
}

static struct a1_nb_data a1_cpu_fixed_nb_data = {
	.nb.notifier_call = a1_cpu_fixed_clk_notifier_cb,
	.onecell_data = &a1_hw_onecell_data,
};

static const struct of_device_id clkc_match_table[] = {
	{ .compatible = "amlogic,a1-clkc" },
	{}
};

static struct regmap_config clkc_regmap_config = {
	.reg_bits       = 32,
	.val_bits       = 32,
	.reg_stride     = 4,
};

static struct regmap *a1_regmap_resource(struct device *dev, char *name)
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

static int a1_clkc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct regmap *basic_map;
	struct regmap *pll_map;
	struct regmap *cpu_clk_map;
	int ret, i;

	/* Get regmap for different clock area */
	basic_map = a1_regmap_resource(dev, "basic");
	if (IS_ERR(basic_map)) {
		dev_err(dev, "basic clk registers not found\n");
		return PTR_ERR(basic_map);
	}

	pll_map = a1_regmap_resource(dev, "pll");
	if (IS_ERR(pll_map)) {
		dev_err(dev, "pll clk registers not found\n");
		return PTR_ERR(pll_map);
	}

	cpu_clk_map = a1_regmap_resource(dev, "cpu_clk");
	if (IS_ERR(cpu_clk_map)) {
		dev_err(dev, "cpu clk registers not found\n");
		return PTR_ERR(cpu_clk_map);
	}

	/* Populate regmap for the regmap backed clocks */
	for (i = 0; i < ARRAY_SIZE(a1_clk_regmaps); i++)
		a1_clk_regmaps[i]->map = basic_map;

	for (i = 0; i < ARRAY_SIZE(a1_cpu_clk_regmaps); i++)
		a1_cpu_clk_regmaps[i]->map = cpu_clk_map;

	for (i = 0; i < ARRAY_SIZE(a1_pll_clk_regmaps); i++)
		a1_pll_clk_regmaps[i]->map = pll_map;

	for (i = 1; i < a1_hw_onecell_data.num; i++) {
		/* array might be sparse */
		if (!a1_hw_onecell_data.hws[i])
			continue;

		//pr_info( "registering %d  %s\n",i,
		//	     a1_hw_onecell_data.hws[i]->init->name);

		ret = devm_clk_hw_register(dev, a1_hw_onecell_data.hws[i]);
		if (ret) {
			dev_err(dev, "Clock registration failed\n");
			return ret;
		}
		ret = devm_clk_hw_register_clkdev(dev, a1_hw_onecell_data.hws[i], NULL,
					clk_hw_get_name(a1_hw_onecell_data.hws[i]));
		if (ret < 0) {
			dev_err(dev, "Failed to clkdev register: %d\n", ret);
			return ret;
		}
	}
	ret = clk_notifier_register(a1_sys_pll.hw.clk, &a1_sys_pll_nb_data.nb);
	if (ret) {
		pr_err("%s: failed to register sys pll notifier\n", __func__);
		return ret;
	}

	ret = clk_notifier_register(a1_cpu_fixed_sel0.hw.clk,
				    &a1_cpu_fixed_nb_data.nb);
	if (ret) {
		pr_err("%s: failed to register the CPU Fixed clock:fsel0 notifier\n",
		       __func__);
		return ret;
	}

	/*
	 *  keep cpu_fixed_clk's parent as cpu_fixed_sel0 clock
	 */
	ret = clk_set_parent(a1_cpu_fixed_clk.hw.clk, a1_cpu_fixed_sel0.hw.clk);
	if (ret) {
		pr_err("%s: failed to set cpu_fixed_sel1 as cpu fixed clk's parent\n",
	       __func__);
		return ret;
	}

	return devm_of_clk_add_hw_provider(dev, of_clk_hw_onecell_get,
					   &a1_hw_onecell_data);
}

static struct platform_driver a1_driver = {
	.probe		= a1_clkc_probe,
	.driver		= {
		.name	= "a1-clkc",
		.of_match_table = clkc_match_table,
	},
};

static int a1_clkc_init(void)
{
	return platform_driver_register(&a1_driver);
}

subsys_initcall(a1_clkc_init);

MODULE_LICENSE("GPL v2");
