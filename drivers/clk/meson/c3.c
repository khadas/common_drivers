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
#include "c3.h"
/*
 * GATE for c3
 * its parent clock is sys clock, the same the
 * clk81 in previos SoC
 */
static struct clk_regmap c3_sys_clk;

#define MESON_C3_SYS_GATE(_name, _reg, _bit)				\
struct clk_regmap _name = {						\
	.data = &(struct clk_regmap_gate_data){				\
		.offset = (_reg),					\
		.bit_idx = (_bit),					\
	},								\
	.hw.init = &(struct clk_init_data) {				\
		.name = #_name,						\
		.ops = &clk_regmap_gate_ops,				\
		.parent_hws = (const struct clk_hw *[]) {		\
			&c3_sys_clk.hw,					\
		},							\
		.num_parents = 1,					\
		.flags = CLK_IGNORE_UNUSED,				\
	},								\
}

static struct clk_regmap axi_clk;

#define MESON_C3_AXI_GATE(_name, _reg, _bit)				\
struct clk_regmap _name = {						\
	.data = &(struct clk_regmap_gate_data){				\
		.offset = (_reg),					\
		.bit_idx = (_bit),					\
	},								\
	.hw.init = &(struct clk_init_data) {				\
		.name = #_name,						\
		.ops = &clk_regmap_gate_ops,				\
		.parent_hws = (const struct clk_hw *[]) {		\
			&axi_clk.hw,					\
		},							\
		.num_parents = 1,					\
		.flags = CLK_IGNORE_UNUSED,				\
	},								\
}

/* CLKCTRL_OSCIN_CTRL is read-only from ree */
#define MESON_C3_XTAL_RO_GATE(_name, _reg, _bit)			\
struct clk_regmap _name = {						\
	.data = &(struct clk_regmap_gate_data){				\
		.offset = (_reg),					\
		.bit_idx = (_bit),					\
	},								\
	.hw.init = &(struct clk_init_data) {				\
		.name = #_name,						\
		.ops = &clk_regmap_gate_ro_ops,				\
		.parent_data = (const struct clk_parent_data []) {	\
			{ .fw_name = "xtal", }				\
		},							\
		.num_parents = 1,					\
		.flags = CLK_IS_CRITICAL | CLK_IGNORE_UNUSED,		\
	},								\
}

/* PLL clock in gates,its parent is xtal */
/* CLKTREE_OSCIN_CTRL */
static MESON_C3_XTAL_RO_GATE(xtal_ddrpll,		OSCIN_CTRL,	1);
static MESON_C3_XTAL_RO_GATE(xtal_ddrphy,		OSCIN_CTRL,	2);
static MESON_C3_XTAL_RO_GATE(xtal_plltop,		OSCIN_CTRL,	4);

static MESON_C3_XTAL_RO_GATE(xtal_usbpll,		OSCIN_CTRL,	6);
static MESON_C3_XTAL_RO_GATE(xtal_isp0_clk_vout,	OSCIN_CTRL,	7);
static MESON_C3_XTAL_RO_GATE(xtal_mclkpll,		OSCIN_CTRL,	8);
static MESON_C3_XTAL_RO_GATE(xtal_usbctrl,		OSCIN_CTRL,	9);
static MESON_C3_XTAL_RO_GATE(xtal_ethpll,		OSCIN_CTRL,	10);

/* Everything Else (EE) domain gates */
/* CLKTREE_SYS_CLK_EN0_REG0 */
static MESON_C3_SYS_GATE(sys_reset_ctrl,	SYS_CLK_EN0_REG0, 1);
static MESON_C3_SYS_GATE(sys_pwr_ctrl,		SYS_CLK_EN0_REG0, 3);
static MESON_C3_SYS_GATE(sys_pad_ctrl,		SYS_CLK_EN0_REG0, 4);
static MESON_C3_SYS_GATE(sys_ctrl,		SYS_CLK_EN0_REG0, 5);
static MESON_C3_SYS_GATE(sys_ts_pll,		SYS_CLK_EN0_REG0, 6);
static MESON_C3_SYS_GATE(sys_dev_arb,		SYS_CLK_EN0_REG0, 7);
static MESON_C3_SYS_GATE(sys_mmc_pclk,		SYS_CLK_EN0_REG0, 8);
static MESON_C3_SYS_GATE(sys_capu,		SYS_CLK_EN0_REG0, 9);
static MESON_C3_SYS_GATE(sys_cpu_ctrl,		SYS_CLK_EN0_REG0, 11);
static MESON_C3_SYS_GATE(sys_jtag_ctrl,		SYS_CLK_EN0_REG0, 12);
static MESON_C3_SYS_GATE(sys_ir_ctrl,		SYS_CLK_EN0_REG0, 13);
static MESON_C3_SYS_GATE(sys_irq_ctrl,		SYS_CLK_EN0_REG0, 14);
static MESON_C3_SYS_GATE(sys_msr_clk,		SYS_CLK_EN0_REG0, 15);
static MESON_C3_SYS_GATE(sys_rom,		SYS_CLK_EN0_REG0, 16);
static MESON_C3_SYS_GATE(sys_uart_f,		SYS_CLK_EN0_REG0, 17);
static MESON_C3_SYS_GATE(sys_cpu_apb,		SYS_CLK_EN0_REG0, 18);
static MESON_C3_SYS_GATE(sys_rsa,		SYS_CLK_EN0_REG0, 19);
static MESON_C3_SYS_GATE(sys_sar_adc,		SYS_CLK_EN0_REG0, 20);
static MESON_C3_SYS_GATE(sys_startup,		SYS_CLK_EN0_REG0, 21);
static MESON_C3_SYS_GATE(sys_secure,		SYS_CLK_EN0_REG0, 22);
static MESON_C3_SYS_GATE(sys_spifc,		SYS_CLK_EN0_REG0, 23);
static MESON_C3_SYS_GATE(sys_nna,		SYS_CLK_EN0_REG0, 25);
static MESON_C3_SYS_GATE(sys_eth_mac,		SYS_CLK_EN0_REG0, 26);
static MESON_C3_SYS_GATE(sys_gic,		SYS_CLK_EN0_REG0, 27);
static MESON_C3_SYS_GATE(sys_rama,		SYS_CLK_EN0_REG0, 28);
static MESON_C3_SYS_GATE(sys_big_nic,		SYS_CLK_EN0_REG0, 29);
static MESON_C3_SYS_GATE(sys_ramb,		SYS_CLK_EN0_REG0, 30);
static MESON_C3_SYS_GATE(sys_audio_PCLK_to_top,	SYS_CLK_EN0_REG0, 31);

/* CLKTREE_SYS_CLK_EN0_REG1 */
static MESON_C3_SYS_GATE(sys_pwm_kl,		SYS_CLK_EN0_REG1, 0);
static MESON_C3_SYS_GATE(sys_pwm_ij,		SYS_CLK_EN0_REG1, 1);
static MESON_C3_SYS_GATE(sys_usb,		SYS_CLK_EN0_REG1, 2);
static MESON_C3_SYS_GATE(sys_sd_emmc_a,		SYS_CLK_EN0_REG1, 3);
static MESON_C3_SYS_GATE(sys_sd_emmc_c,		SYS_CLK_EN0_REG1, 4);
static MESON_C3_SYS_GATE(sys_pwm_ab,		SYS_CLK_EN0_REG1, 5);
static MESON_C3_SYS_GATE(sys_pwm_cd,		SYS_CLK_EN0_REG1, 6);
static MESON_C3_SYS_GATE(sys_pwm_ef,		SYS_CLK_EN0_REG1, 7);
static MESON_C3_SYS_GATE(sys_pwm_gh,		SYS_CLK_EN0_REG1, 8);
static MESON_C3_SYS_GATE(sys_spicc_1,		SYS_CLK_EN0_REG1, 9);
static MESON_C3_SYS_GATE(sys_spicc_0,		SYS_CLK_EN0_REG1, 10);
static MESON_C3_SYS_GATE(sys_uart_a,		SYS_CLK_EN0_REG1, 11);
static MESON_C3_SYS_GATE(sys_uart_b,		SYS_CLK_EN0_REG1, 12);
static MESON_C3_SYS_GATE(sys_uart_c,		SYS_CLK_EN0_REG1, 13);
static MESON_C3_SYS_GATE(sys_uart_d,		SYS_CLK_EN0_REG1, 14);
static MESON_C3_SYS_GATE(sys_uart_e,		SYS_CLK_EN0_REG1, 15);
static MESON_C3_SYS_GATE(sys_i2c_m_a,		SYS_CLK_EN0_REG1, 16);
static MESON_C3_SYS_GATE(sys_i2c_m_b,		SYS_CLK_EN0_REG1, 17);
static MESON_C3_SYS_GATE(sys_i2c_m_c,		SYS_CLK_EN0_REG1, 18);
static MESON_C3_SYS_GATE(sys_i2c_m_d,		SYS_CLK_EN0_REG1, 19);
static MESON_C3_SYS_GATE(sys_i2c_s_a,		SYS_CLK_EN0_REG1, 20);
static MESON_C3_SYS_GATE(sys_rtc,		SYS_CLK_EN0_REG1, 21);
static MESON_C3_SYS_GATE(sys_ge2d,		SYS_CLK_EN0_REG1, 22);
static MESON_C3_SYS_GATE(sys_isp,		SYS_CLK_EN0_REG1, 23);
static MESON_C3_SYS_GATE(sys_gpv_isp_nic,	SYS_CLK_EN0_REG1, 24);
static MESON_C3_SYS_GATE(sys_gpv_cve_nic,	SYS_CLK_EN0_REG1, 25);
static MESON_C3_SYS_GATE(sys_mipi_dsi_host,	SYS_CLK_EN0_REG1, 26);
static MESON_C3_SYS_GATE(sys_mipi_dsi_phy,	SYS_CLK_EN0_REG1, 27);
static MESON_C3_SYS_GATE(sys_eth_phy,		SYS_CLK_EN0_REG1, 28);
static MESON_C3_SYS_GATE(sys_acodec,		SYS_CLK_EN0_REG1, 29);
static MESON_C3_SYS_GATE(sys_dwap,		SYS_CLK_EN0_REG1, 30);
static MESON_C3_SYS_GATE(sys_dos,		SYS_CLK_EN0_REG1, 31);

/* CLKTREE_SYS_CLK_EN0_REG2 */
static MESON_C3_SYS_GATE(sys_cve,		SYS_CLK_EN0_REG2, 0);
static MESON_C3_SYS_GATE(sys_vout,		SYS_CLK_EN0_REG2, 1);
static MESON_C3_SYS_GATE(sys_vc9000e,		SYS_CLK_EN0_REG2, 2);
static MESON_C3_SYS_GATE(sys_pwm_mn,		SYS_CLK_EN0_REG2, 3);
static MESON_C3_SYS_GATE(sys_sd_emmc_b,		SYS_CLK_EN0_REG2, 4);

/* CLKTREE_AXI_CLK_EN0 */
static MESON_C3_AXI_GATE(axi_sys_nic,		AXI_CLK_EN0, 2);
static MESON_C3_AXI_GATE(axi_isp_nic,		AXI_CLK_EN0, 3);
static MESON_C3_AXI_GATE(axi_cve_nic,		AXI_CLK_EN0, 4);
static MESON_C3_AXI_GATE(axi_ramb,		AXI_CLK_EN0, 5);
static MESON_C3_AXI_GATE(axi_rama,		AXI_CLK_EN0, 6);
static MESON_C3_AXI_GATE(axi_cpu_dmc,		AXI_CLK_EN0, 7);
static MESON_C3_AXI_GATE(axi_nic,		AXI_CLK_EN0, 8);
static MESON_C3_AXI_GATE(axi_dma,		AXI_CLK_EN0, 9);
static MESON_C3_AXI_GATE(axi_mux_nic,		AXI_CLK_EN0, 10);
static MESON_C3_AXI_GATE(axi_capu,		AXI_CLK_EN0, 11);
static MESON_C3_AXI_GATE(axi_cve,		AXI_CLK_EN0, 12);
static MESON_C3_AXI_GATE(axi_dev1_dmc,		AXI_CLK_EN0, 13);
static MESON_C3_AXI_GATE(axi_dev0_dmc,		AXI_CLK_EN0, 14);
static MESON_C3_AXI_GATE(axi_dsp_dmc,		AXI_CLK_EN0, 15);

static const struct clk_ops meson_pll_clk_no_ops = {};

/* fixed pll = 2000M read-only from ree
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
static struct clk_regmap c3_fixed_pll_vco = {
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
			.shift   = 16,
			.width   = 5,
		},
#ifdef CONFIG_ARM
		/* od for 32bit */
		.od = {
			.reg_off = ANACTRL_FIXPLL_CTRL0,
			.shift   = 12,
			.width   = 3,
		},
#endif  /* CONFIG_ARM */
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
		.name = "fixed_pll_vco",
		.ops = &meson_clk_pll_ro_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&xtal_plltop.hw,
		},
		.num_parents = 1,
		.flags = CLK_IS_CRITICAL | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap c3_fixed_pll = {
#ifdef CONFIG_ARM

#else
	.data = &(struct clk_regmap_div_data){
		.offset = ANACTRL_FIXPLL_CTRL0,
		.shift = 12,
		.width = 3,
		.flags = CLK_DIVIDER_POWER_OF_TWO | CLK_DIVIDER_READ_ONLY,
	},
#endif  /* CONFIG_ARM */
	.hw.init = &(struct clk_init_data){
		.name = "fixed_pll",
#ifdef CONFIG_ARM
		.ops = &meson_pll_clk_no_ops,
#else
		.ops = &clk_regmap_secure_v2_divider_ops,
#endif  /* CONFIG_ARM */
		.parent_hws = (const struct clk_hw *[]) {
			&c3_fixed_pll_vco.hw,
		},
		.num_parents = 1,
	},
};

static struct clk_fixed_factor c3_fclk50_div40 = {
	.mult = 1,
	.div = 40,
	.hw.init = &(struct clk_init_data){
		.name = "fixed_pll_clk50M_div40",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_fixed_pll.hw,
		},
		.num_parents = 1,
	},
};

static struct clk_regmap c3_fclk50M = {
	.data = &(struct clk_regmap_gate_data){
		.offset = ANACTRL_FIXPLL_CTRL4,
		.bit_idx = 0,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "fixed_pll_clk50M",
		.ops = &clk_regmap_gate_ro_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_fclk50_div40.hw,
		},
		.num_parents = 1,
	},
};

static struct clk_fixed_factor c3_fclk_div2_div = {
	.mult = 1,
	.div = 2,
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div2_div",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_fixed_pll.hw,
		},
		.num_parents = 1,
	},
};

static struct clk_regmap c3_fclk_div2 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = ANACTRL_FIXPLL_CTRL4,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div2",
		.ops = &clk_regmap_gate_ro_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_fclk_div2_div.hw,
		},
		.num_parents = 1,
	/*
	 * add CLK_IS_CRITICAL flag to avoid being disabled by clk core
	 * or its children clocks.
	 */
		.flags = CLK_IS_CRITICAL,
	},
};

static struct clk_fixed_factor c3_fclk_div2p5_div = {
	.mult = 2,
	.div = 5,
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div2p5_div",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_fixed_pll.hw,
		},
		.num_parents = 1,
	},
};

static struct clk_regmap c3_fclk_div2p5 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = ANACTRL_FIXPLL_CTRL4,
		.bit_idx = 4,
	},
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div2p5",
		.ops = &clk_regmap_gate_ro_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_fclk_div2p5_div.hw,
		},
		.num_parents = 1,
	/*
	 * add CLK_IS_CRITICAL flag to avoid being disabled by clk core
	 * or its children clocks.
	 */
		.flags = CLK_IS_CRITICAL,
	},
};

static struct clk_fixed_factor c3_fclk_div3_div = {
	.mult = 1,
	.div = 3,
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div3_div",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_fixed_pll.hw,
		},
		.num_parents = 1,
	},
};

static struct clk_regmap c3_fclk_div3 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = ANACTRL_FIXPLL_CTRL4,
		.bit_idx = 20,
	},
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div3",
		.ops = &clk_regmap_gate_ro_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_fclk_div3_div.hw,
		},
		.num_parents = 1,
	/*
	 * add CLK_IS_CRITICAL flag to avoid being disabled by clk core
	 * its children clocks.
	 */
		.flags = CLK_IS_CRITICAL,
	},
};

static struct clk_fixed_factor c3_fclk_div4_div = {
	.mult = 1,
	.div = 4,
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div4_div",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_fixed_pll.hw,
		},
		.num_parents = 1,
	},
};

static struct clk_regmap c3_fclk_div4 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = ANACTRL_FIXPLL_CTRL4,
		.bit_idx = 21,
	},
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div4",
		.ops = &clk_regmap_gate_ro_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_fclk_div4_div.hw,
		},
		.num_parents = 1,
	/*
	 * add CLK_IS_CRITICAL flag to avoid being disabled by clk core
	 * or its children clocks.
	 */
		.flags = CLK_IS_CRITICAL,
	},
};

static struct clk_fixed_factor c3_fclk_div5_div = {
	.mult = 1,
	.div = 5,
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div5_div",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_fixed_pll.hw,
		},
		.num_parents = 1,
	},
};

static struct clk_regmap c3_fclk_div5 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = ANACTRL_FIXPLL_CTRL4,
		.bit_idx = 22,
	},
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div5",
		.ops = &clk_regmap_gate_ro_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_fclk_div5_div.hw,
		},
		.num_parents = 1,
	/*
	 * add CLK_IS_CRITICAL flag to avoid being disabled by clk core
	 * its children clocks.
	 */
		.flags = CLK_IS_CRITICAL,
	},
};

static struct clk_fixed_factor c3_fclk_div7_div = {
	.mult = 1,
	.div = 7,
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div7_div",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_fixed_pll.hw,
		},
		.num_parents = 1,
	},
};

static struct clk_regmap c3_fclk_div7 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = ANACTRL_FIXPLL_CTRL4,
		.bit_idx = 23,
	},
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div7",
		.ops = &clk_regmap_gate_ro_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_fclk_div7_div.hw,
		},
		.num_parents = 1,
	/*
	 * add CLK_IS_CRITICAL flag to avoid being disabled
	 * by clk core or its children clock.
	 */
		.flags = CLK_IS_CRITICAL,
	},
};

#ifdef CONFIG_ARM
static const struct pll_params_table c3_gp0_pll_params_table[] = {
	PLL_PARAMS(150, 1, 1), /* DCO = 3600M, CLK_OUT = 1800M */
	PLL_PARAMS(130, 1, 1), /* DCO = 3120M, CLK_OUT = 1560M */
	PLL_PARAMS(192, 1, 2), /* DCO = 4608M, CLK_OUT = 1152M */
	PLL_PARAMS(125, 1, 2), /* DCO = 3000M, CLK_OUT = 750M */
	{ /* sentinel */  },
};
#else
static const struct pll_params_table c3_gp0_pll_params_table[] = {
	PLL_PARAMS(150, 1), /* DCO = 3600M */
	PLL_PARAMS(130, 1), /* DCO = 3120M */
	PLL_PARAMS(192, 1), /* DCO = 4608M */
	PLL_PARAMS(125, 1), /* DCO = 3000M */
	{ /* sentinel */  },
};
#endif
static const struct reg_sequence c3_gp0_init_regs[] = {
	{ .reg = ANACTRL_GP0PLL_CTRL1,	.def = 0x0 },
	{ .reg = ANACTRL_GP0PLL_CTRL2,	.def = 0x0 },
	{ .reg = ANACTRL_GP0PLL_CTRL3,	.def = 0x48681c00 },
	{ .reg = ANACTRL_GP0PLL_CTRL4,  .def = 0x88770290 },
	{ .reg = ANACTRL_GP0PLL_CTRL5,  .def = 0x3927200a },
	{ .reg = ANACTRL_GP0PLL_CTRL6,	.def = 0x56540000, .delay_us = 10 },
	{ .reg = ANACTRL_GP0PLL_CTRL0,	.def = 0x080304fa },
	{ .reg = ANACTRL_GP0PLL_CTRL0,	.def = 0x380304fa, .delay_us = 10 },
	{ .reg = ANACTRL_GP0PLL_CTRL0,	.def = 0X180304fa },
};

static struct clk_regmap c3_gp0_pll_vco = {
	.data = &(struct meson_clk_pll_data){
		.en = {
			.reg_off = ANACTRL_GP0PLL_CTRL0,
			.shift   = 28,
			.width   = 1,
		},
		.m = {
			.reg_off = ANACTRL_GP0PLL_CTRL0,
			.shift   = 0,
			.width   = 9,
		},
		.n = {
			.reg_off = ANACTRL_GP0PLL_CTRL0,
			.shift   = 10,
			.width   = 5,
		},
#ifdef CONFIG_ARM
		/* od for 32bit */
		.od = {
			.reg_off = ANACTRL_GP0PLL_CTRL0,
			.shift   = 16,
			.width   = 3,
		},
#endif  /* CONFIG_ARM */
		.frac = {
			.reg_off = ANACTRL_GP0PLL_CTRL1,
			.shift   = 0,
			.width   = 19,
		},
		.l = {
			.reg_off = ANACTRL_GP0PLL_CTRL0,
			.shift   = 31,
			.width   = 1,
		},
		.rst = {
			.reg_off = ANACTRL_GP0PLL_CTRL0,
			.shift   = 29,
			.width   = 1,
		},
		.table = c3_gp0_pll_params_table,
		.init_regs = c3_gp0_init_regs,
		.init_count = ARRAY_SIZE(c3_gp0_init_regs),
	},
	.hw.init = &(struct clk_init_data){
		.name = "gp0_pll_vco",
		.ops = &meson_clk_pll_v3_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&xtal_plltop.hw,
		},
		.num_parents = 1,
		.flags = CLK_IGNORE_UNUSED | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap c3_gp0_pll = {
#ifdef CONFIG_ARM

#else
	.data = &(struct clk_regmap_div_data){
		.offset = ANACTRL_GP0PLL_CTRL0,
		.shift = 16,
		.width = 3,
		.flags = CLK_DIVIDER_POWER_OF_TWO | CLK_DIVIDER_ROUND_CLOSEST,
	},
#endif  /* CONFIG_ARM */
	.hw.init = &(struct clk_init_data){
		.name = "gp0_pll",
#ifdef CONFIG_ARM
		.ops = &meson_pll_clk_no_ops,
#else
		.ops = &clk_regmap_divider_ops,
#endif  /* CONFIG_ARM */
		.parent_hws = (const struct clk_hw *[]) {
			&c3_gp0_pll_vco.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

/* gp Pll */

#ifdef CONFIG_ARM
static const struct pll_params_table c3_gp1_pll_params_table[] = {
	PLL_PARAMS(150, 1, 1), /* DCO = 3600M, CLK_OUT = 1800M */
	PLL_PARAMS(130, 1, 1), /* DCO = 3120M, CLK_OUT = 1560M */
	PLL_PARAMS(192, 1, 2), /* DCO = 4608M, CLK_OUT = 1152M */
	PLL_PARAMS(125, 1, 2), /* DCO = 3000M, CLK_OUT = 750M */
	{ /* sentinel */  },
};
#else
static const struct pll_params_table c3_gp1_pll_params_table[] = {
	PLL_PARAMS(150, 1), /* DCO = 3600M */
	PLL_PARAMS(130, 1), /* DCO = 3120M */
	PLL_PARAMS(192, 1), /* DCO = 4608M */
	PLL_PARAMS(125, 1), /* DCO = 3000M */
	{ /* sentinel */  },
};
#endif

/*
 * gp pll = 1188M
 */
static struct clk_regmap c3_gp1_pll_vco = {
	.data = &(struct meson_clk_pll_data){
		.en = {
			.reg_off = ANACTRL_GP1PLL_CTRL0,
			.shift   = 28,
			.width   = 1,
		},
		.m = {
			.reg_off = ANACTRL_GP1PLL_CTRL0,
			.shift   = 0,
			.width   = 9,
		},
		.n = {
			.reg_off = ANACTRL_GP1PLL_CTRL0,
			.shift   = 10,
			.width   = 5,
		},
#ifdef CONFIG_ARM
		/* od for 32bit */
		.od = {
			.reg_off = ANACTRL_GP1PLL_CTRL0,
			.shift   = 16,
			.width   = 3,
		},
#endif  /* CONFIG_ARM */
		.frac = {
			.reg_off = ANACTRL_GP1PLL_CTRL1,
			.shift   = 0,
			.width   = 19,
		},
		.l = {
			.reg_off = ANACTRL_GP1PLL_CTRL0,
			.shift   = 31,
			.width   = 1,
		},
		.rst = {
			.reg_off = ANACTRL_GP1PLL_CTRL0,
			.shift   = 29,
			.width   = 1,
		},
		.table = c3_gp1_pll_params_table,
		.smc_id = SECURE_PLL_CLK,
		.secid_disable = SECID_GP1_DCO_PLL_DIS,
		.secid = SECID_GP1_DCO_PLL,
	},
	.hw.init = &(struct clk_init_data){
		.name = "gp1_pll_vco",
		.ops = &meson_secure_pll_v2_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&xtal_plltop.hw,
		},
		.num_parents = 1,
		.flags = CLK_IGNORE_UNUSED | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap c3_gp1_pll = {
#ifdef CONFIG_ARM

#else
	.data = &(struct clk_regmap_div_data){
		.offset = ANACTRL_GP1PLL_CTRL0,
		.shift = 16,
		.width = 3,
		.flags = CLK_DIVIDER_POWER_OF_TWO | CLK_DIVIDER_ROUND_CLOSEST,
		.smc_id = SECURE_PLL_CLK,
		.secid = SECID_GP1_PLL_OD,
	},
#endif  /* CONFIG_ARM */
	.hw.init = &(struct clk_init_data){
		.name = "gp1_pll",
#ifdef CONFIG_ARM
		.ops = &meson_pll_clk_no_ops,
#else
		.ops = &clk_regmap_secure_v2_divider_ops,
#endif  /* CONFIG_ARM */
		.parent_hws = (const struct clk_hw *[]) {
			&c3_gp1_pll_vco.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

/* hifi pll */
/* pll 1536M */
static const struct reg_sequence c3_hifi_init_regs[] = {
	{ .reg = ANACTRL_HIFIPLL_CTRL0,	.def = 0x08010496 },
	{ .reg = ANACTRL_HIFIPLL_CTRL0,	.def = 0x38010496 },
	{ .reg = ANACTRL_HIFIPLL_CTRL1,	.def = 0x0000ce40 },
	{ .reg = ANACTRL_HIFIPLL_CTRL2,	.def = 0x00000000 },
	{ .reg = ANACTRL_HIFIPLL_CTRL3,	.def = 0x6a285c00 },
	{ .reg = ANACTRL_HIFIPLL_CTRL4, .def = 0x65771290 },
	{ .reg = ANACTRL_HIFIPLL_CTRL5, .def = 0x3927200a },
	{ .reg = ANACTRL_HIFIPLL_CTRL6,	.def = 0x56540000, .delay_us = 50 },
	{ .reg = ANACTRL_HIFIPLL_CTRL0,	.def = 0x18010496, .delay_us = 20 },
};

#ifdef CONFIG_ARM
static const struct pll_params_table c3_hifi_pll_params_table[] = {
	PLL_PARAMS(150, 1, 1), /* DCO = 3600M, CLK_OUT = 1800M */
	PLL_PARAMS(130, 1, 1), /* DCO = 3120M, CLK_OUT = 1560M */
	PLL_PARAMS(192, 1, 2), /* DCO = 4608M, CLK_OUT = 1152M */
	PLL_PARAMS(125, 1, 2), /* DCO = 3000M, CLK_OUT = 750M */
	{ /* sentinel */  },
};
#else
static const struct pll_params_table c3_hifi_pll_params_table[] = {
	PLL_PARAMS(150, 1), /* DCO = 3600M */
	PLL_PARAMS(130, 1), /* DCO = 3120M */
	PLL_PARAMS(192, 1), /* DCO = 4608M */
	PLL_PARAMS(125, 1), /* DCO = 3000M */
	{ /* sentinel */  },
};
#endif

/*
 * hifi vco = 768M - 1536M
 * hifi pll: 12M - 1536M
 */
static struct clk_regmap c3_hifi_pll_vco = {
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
#ifdef CONFIG_ARM
		/* od for 32bit */
		.od = {
			.reg_off = ANACTRL_HIFIPLL_CTRL0,
			.shift   = 16,
			.width   = 2,
		},
#endif  /* CONFIG_ARM */
		.frac = {
			.reg_off = ANACTRL_HIFIPLL_CTRL1,
			.shift   = 0,
			.width   = 19,
		},
		.l = {
			.reg_off = ANACTRL_HIFIPLL_CTRL0,
			.shift   = 31,
			.width   = 1,
		},
		.rst = {
			.reg_off = ANACTRL_HIFIPLL_CTRL0,
			.shift   = 29,
			.width   = 1,
		},
		.table = c3_hifi_pll_params_table,
		.init_regs = c3_hifi_init_regs,
		.init_count = ARRAY_SIZE(c3_hifi_init_regs),
	},
	.hw.init = &(struct clk_init_data){
		.name = "hifi_pll_vco",
		.ops = &meson_clk_pll_v3_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&xtal_plltop.hw,
		},
		.num_parents = 1,
		.flags = CLK_IGNORE_UNUSED | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap c3_hifi_pll = {
#ifdef CONFIG_ARM

#else
	.data = &(struct clk_regmap_div_data){
		.offset = ANACTRL_HIFIPLL_CTRL0,
		.shift = 16,
		.width = 2,
		.flags = CLK_DIVIDER_POWER_OF_TWO | CLK_DIVIDER_ROUND_CLOSEST,
	},
#endif  /* CONFIG_ARM */
	.hw.init = &(struct clk_init_data){
		.name = "hifi_pll",
#ifdef CONFIG_ARM
		.ops = &meson_pll_clk_no_ops,
#else
		.ops = &clk_regmap_divider_ops,
#endif  /* CONFIG_ARM */
		.parent_hws = (const struct clk_hw *[]) {
			&c3_hifi_pll_vco.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

#ifdef CONFIG_ARM
static const struct pll_params_table c3_mclk_pll_params_table[] = {
	PLL_PARAMS(99, 1, 1), /* VCO = 2376M, CLK_OUT = 1188M */
	{ /* sentinel */  },
};
#else
static const struct pll_params_table c3_mclk_pll_params_table[] = {
	PLL_PARAMS(99, 1), /* VCO = 2376M */
	{ /* sentinel */  },
};
#endif
static const struct reg_sequence c3_mclk_init_regs[] = {
	{ .reg = ANACTRL_MPLL_CTRL0,	.def = 0x20011063 },
	{ .reg = ANACTRL_MPLL_CTRL0,	.def = 0x30011063 },
	{ .reg = ANACTRL_MPLL_CTRL1,	.def = 0x1420500f },
	{ .reg = ANACTRL_MPLL_CTRL2,	.def = 0x00023041 },
	{ .reg = ANACTRL_MPLL_CTRL3,	.def = 0x18180000 },
	{ .reg = ANACTRL_MPLL_CTRL0,	.def = 0x10011063 },
	{ .reg = ANACTRL_MPLL_CTRL2,	.def = 0x00023001 },
};

static struct clk_regmap c3_mclk_pll_vco = {
	.data = &(struct meson_clk_pll_data){
		.en = {
			.reg_off = ANACTRL_MPLL_CTRL0,
			.shift   = 28,
			.width   = 1,
		},
		.m = {
			.reg_off = ANACTRL_MPLL_CTRL0,
			.shift   = 0,
			.width   = 8,
		},
		.n = {
			.reg_off = ANACTRL_MPLL_CTRL0,
			.shift   = 16,
			.width   = 5,
		},
#ifdef CONFIG_ARM
		/* od for 32bit */
		.od = {
			.reg_off = ANACTRL_MPLL_CTRL0,
			.shift   = 12,
			.width   = 3,
		},
#endif  /* CONFIG_ARM */
		.l = {
			.reg_off = ANACTRL_MPLL_CTRL0,
			.shift   = 31,
			.width   = 1,
		},
		.rst = {
			.reg_off = ANACTRL_MPLL_CTRL0,
			.shift   = 29,
			.width   = 1,
		},
		.table = c3_mclk_pll_params_table,
		.init_regs = c3_mclk_init_regs,
		.init_count = ARRAY_SIZE(c3_mclk_init_regs),
		.flags = CLK_MESON_PLL_IGNORE_INIT,
	},
	.hw.init = &(struct clk_init_data){
		.name = "mclk_pll_vco",
		.ops = &meson_clk_pll_v3_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&xtal_plltop.hw,
		},
		.num_parents = 1,
		.flags = CLK_IGNORE_UNUSED | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap c3_mclk_pll = {
#ifdef CONFIG_ARM

#else
	.data = &(struct clk_regmap_div_data){
		.offset = ANACTRL_MPLL_CTRL0,
		.shift = 12,
		.width = 3,
		.flags = CLK_DIVIDER_POWER_OF_TWO | CLK_DIVIDER_ROUND_CLOSEST |
			 CLK_DIVIDER_ALLOW_ZERO,
	},
#endif  /* CONFIG_ARM */
	.hw.init = &(struct clk_init_data){
		.name = "mclk_pll",
#ifdef CONFIG_ARM
		.ops = &meson_pll_clk_no_ops,
#else
		.ops = &clk_regmap_divider_ops,
#endif  /* CONFIG_ARM */
		.parent_hws = (const struct clk_hw *[]) {
			&c3_mclk_pll_vco.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap c3_mclk_pll_clk = {
	.data = &(struct clk_regmap_div_data){
		.offset = ANACTRL_MPLL_CTRL4,
		.shift = 16,
		.width = 5,
		.flags = CLK_DIVIDER_ONE_BASED | CLK_DIVIDER_ROUND_CLOSEST |
			 CLK_DIVIDER_ALLOW_ZERO,
	},
	.hw.init = &(struct clk_init_data){
		.name = "mclk_pll_clk",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_mclk_pll.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

static const struct clk_parent_data mclk_clk_parent_names[] = {
	{ .hw = &c3_mclk_pll_clk.hw },
	{ .fw_name = "xtal", },
	{ .hw = &c3_fclk50M.hw },
};

static struct clk_regmap c3_mclk_0_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = ANACTRL_MPLL_CTRL4,
		.mask = 0x3,
		.shift = 4,
	},
	.hw.init = &(struct clk_init_data){
		.name = "mclk_0_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = mclk_clk_parent_names,
		.num_parents = ARRAY_SIZE(mclk_clk_parent_names),
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap c3_mclk_0_sel_out = {
	.data = &(struct clk_regmap_gate_data){
		.offset = ANACTRL_MPLL_CTRL4,
		.bit_idx = 1,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "mclk_0_sel_out",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_mclk_0_sel.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap c3_mclk_0_div2 = {
	.data = &(struct clk_regmap_div_data){
		.offset = ANACTRL_MPLL_CTRL4,
		.shift = 2,
		.width = 1,
		.flags = CLK_DIVIDER_ALLOW_ZERO | CLK_DIVIDER_POWER_OF_TWO,
	},
	.hw.init = &(struct clk_init_data){
		.name = "mclk_0_div2",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_mclk_0_sel_out.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap c3_mclk_0 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = ANACTRL_MPLL_CTRL4,
		.bit_idx = 0,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "mclk_0",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_mclk_0_div2.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap c3_mclk_1_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = ANACTRL_MPLL_CTRL4,
		.mask = 0x3,
		.shift = 12,
	},
	.hw.init = &(struct clk_init_data){
		.name = "mclk_1_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = mclk_clk_parent_names,
		.num_parents = ARRAY_SIZE(mclk_clk_parent_names),
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap c3_mclk_1_sel_out = {
	.data = &(struct clk_regmap_gate_data){
		.offset = ANACTRL_MPLL_CTRL4,
		.bit_idx = 9,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "mclk_1_sel_out",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_mclk_1_sel.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap c3_mclk_1_div2 = {
	.data = &(struct clk_regmap_div_data){
		.offset = ANACTRL_MPLL_CTRL4,
		.shift = 10,
		.width = 1,
		.flags = CLK_DIVIDER_ALLOW_ZERO | CLK_DIVIDER_POWER_OF_TWO,
	},
	.hw.init = &(struct clk_init_data){
		.name = "mclk_1_div2",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_mclk_1_sel_out.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap c3_mclk_1 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = ANACTRL_MPLL_CTRL4,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "mclk_1",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_mclk_1_div2.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

/*
 * Internal sys pll emulation configuration parameters
 */

#ifdef CONFIG_ARM

static const struct pll_params_table c3_pll_params_table[] = {
	PLL_PARAMS(133, 1, 1), /* VCO = 3192M; CLK_OUT = 1596M */
	PLL_PARAMS(126, 1, 1), /* VCO = 3024M; CLK_OUT = 1512M */
	PLL_PARAMS(125, 1, 1), /* VCO = 3000M; CLK_OUT = 1500M */
	PLL_PARAMS(117, 1, 1), /* VCO = 2808M; CLK_OUT = 1404M */
	PLL_PARAMS(100, 1, 1), /* VCO = 2400M; CLK_OUT = 1200M */
	PLL_PARAMS(96, 1, 0), /* VCO = 2304M; CLK_OUT = 2304M */
	PLL_PARAMS(88, 1, 0), /* VCO = 2112M; CLK_OUT = 2112M */
	PLL_PARAMS(84, 1, 0), /* VCO = 2016M; CLK_OUT = 2016M */
	PLL_PARAMS(80, 1, 0), /* VCO = 1920M; CLK_OUT = 1920M */
	PLL_PARAMS(75, 1, 0), /* VCO = 1800M; CLK_OUT = 1800M */
	PLL_PARAMS(71, 1, 0), /* VCO = 1704M; CLK_OUT = 1704M */
	PLL_PARAMS(67, 1, 0), /* VCO = 1608M; CLK_OUT = 1608M */
	{ /* sentinel */ }
};
#else
static const struct pll_params_table c3_pll_params_table[] = {
	PLL_PARAMS(133, 1), /* VCO = 3192M*/
	PLL_PARAMS(126, 1), /* VCO = 3024M*/
	PLL_PARAMS(125, 1), /* VCO = 3000M*/
	PLL_PARAMS(117, 1), /* VCO = 2808M*/
	PLL_PARAMS(100, 1), /* VCO = 2400M */
	PLL_PARAMS(96, 1), /* VCO = 2304M*/
	PLL_PARAMS(88, 1), /* VCO = 2112M */
	PLL_PARAMS(84, 1), /* VCO = 2016M */
	PLL_PARAMS(80, 1), /* VCO = 1920M */
	PLL_PARAMS(75, 1), /* VCO = 1800M*/
	PLL_PARAMS(71, 1), /* VCO = 1704M*/
	PLL_PARAMS(67, 1), /* VCO = 1608M */
	{ /* sentinel */ }
};
#endif
/*
 * sys pll = 25M ~ 3200M
 */
static struct clk_regmap c3_sys_pll_vco = {
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
			.shift   = 16,
			.width   = 5,
		},
#ifdef CONFIG_ARM
		/* od for 32bit */
		.od = {
			.reg_off = ANACTRL_SYSPLL_CTRL0,
			.shift   = 12,
			.width   = 3,
		},
#endif
		.l = {
			.reg_off = ANACTRL_SYSPLL_CTRL0,
			.shift   = 31,
			.width   = 1,
		},
		.table = c3_pll_params_table,
		.smc_id = SECURE_PLL_CLK,
		.secid_disable = SECID_SYS_DCO_PLL_DIS,
		.secid = SECID_SYS_DCO_PLL,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sys_pll_vco",
		.ops = &meson_secure_pll_v2_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&xtal_plltop.hw,
		},
		.num_parents = 1,
		.flags = CLK_IGNORE_UNUSED | CLK_GET_RATE_NOCACHE,
	},
};

#ifdef CONFIG_ARM
static struct clk_regmap c3_sys_pll = {
	.hw.init = &(struct clk_init_data){
		.name = "sys_pll",
		.ops = &meson_pll_clk_no_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_sys_pll_vco.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};
#else
static struct clk_regmap c3_sys_pll = {
	.data = &(struct clk_regmap_div_data){
		.offset = ANACTRL_SYSPLL_CTRL0,
		.shift = 12,
		.width = 3,
		.flags = CLK_DIVIDER_POWER_OF_TWO | CLK_DIVIDER_ROUND_CLOSEST,
		.smc_id = SECURE_PLL_CLK,
		.secid = SECID_SYS_PLL_OD,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sys_pll",
		.ops =  &clk_regmap_secure_v2_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_sys_pll_vco.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};
#endif

static struct clk_regmap c3_rtc_clk;

/* sys clk = 64M */
static u32 mux_table_sys_ab_clk_sel[] = { 0, 1, 2, 3, 4, 7 };
static const struct clk_parent_data sys_ab_clk_parent_names[] = {
	{ .fw_name = "xtal", },
	{ .hw = &c3_gp1_pll.hw },
	{ .hw = &c3_fclk_div3.hw },
	{ .hw = &c3_fclk_div4.hw },
	{ .hw = &c3_fclk_div5.hw },
	{ .hw = &c3_rtc_clk.hw },
};

static struct clk_regmap c3_sys_b_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = SYS_CLK_CTRL0,
		.mask = 0x7,
		.shift = 26,
		.table = mux_table_sys_ab_clk_sel,
		.flags = CLK_MUX_READ_ONLY,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sys_b_sel",
		.ops = &clk_regmap_mux_ro_ops,  /* read only from ree */
		.parent_data = sys_ab_clk_parent_names,
		.num_parents = ARRAY_SIZE(sys_ab_clk_parent_names),
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap c3_sys_b_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = SYS_CLK_CTRL0,
		.shift = 16,
		.width = 10,
		.flags = CLK_DIVIDER_READ_ONLY,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sys_b_div",
		.ops = &clk_regmap_divider_ro_ops,  /* read only from ree */
		.parent_hws = (const struct clk_hw *[]) {
			&c3_sys_b_sel.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap c3_sys_b = {
	.data = &(struct clk_regmap_gate_data){
		.offset = SYS_CLK_CTRL0,
		.bit_idx = 29,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "sys_b",
		.ops = &clk_regmap_gate_ro_ops,  /* read only from ree */
		.parent_hws = (const struct clk_hw *[]) {
			&c3_sys_b_div.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED
			 | CLK_IS_CRITICAL,
	},
};

static struct clk_regmap c3_sys_a_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = SYS_CLK_CTRL0,
		.mask = 0x7,
		.shift = 10,
		.table = mux_table_sys_ab_clk_sel,
		.flags = CLK_MUX_READ_ONLY,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sys_a_sel",
		.ops = &clk_regmap_mux_ro_ops,  /* read only from ree */
		.parent_data = sys_ab_clk_parent_names,
		.num_parents = ARRAY_SIZE(sys_ab_clk_parent_names),
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap c3_sys_a_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = SYS_CLK_CTRL0,
		.shift = 0,
		.width = 10,
		.flags = CLK_DIVIDER_READ_ONLY,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sys_a_div",
		.ops = &clk_regmap_divider_ro_ops,  /* read only from ree */
		.parent_hws = (const struct clk_hw *[]) {
			&c3_sys_a_sel.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap c3_sys_a = {
	.data = &(struct clk_regmap_gate_data){
		.offset = SYS_CLK_CTRL0,
		.bit_idx = 13,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "sys_a",
		.ops = &clk_regmap_gate_ro_ops,  /* read only from ree */
		.parent_hws = (const struct clk_hw *[]) {
			&c3_sys_a_div.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED
			 | CLK_IS_CRITICAL,
	},
};

static const struct clk_parent_data sys_clk_parent_names[] = {
	{ .hw = &c3_sys_a.hw },
	{ .hw = &c3_sys_b.hw }
};

static struct clk_regmap c3_sys_clk = {
	.data = &(struct clk_regmap_mux_data){
		.offset = SYS_CLK_CTRL0,
		.mask = 0x1,
		.shift = 15,
		.flags = CLK_MUX_READ_ONLY,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sys_clk",
		.ops = &clk_regmap_mux_ro_ops,  /* read only from ree */
		.parent_data = sys_clk_parent_names,
		.num_parents = ARRAY_SIZE(sys_clk_parent_names),
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

/* axi */
static u32 mux_table_axi[] = { 0, 1, 2, 3, 4, 7 };
static const struct clk_parent_data axi_ab_clk_parent_names[] = {
	{ .fw_name = "xtal", },
	{ .hw = &c3_gp1_pll.hw },
	{ .hw = &c3_fclk_div3.hw },
	{ .hw = &c3_fclk_div4.hw },
	{ .hw = &c3_fclk_div5.hw },
	{ .hw = &c3_rtc_clk.hw },
};

static struct clk_regmap axi_b_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = AXI_CLK_CTRL0,
		.mask = 0x7,
		.shift = 26,
		.table = mux_table_axi,
	},
	.hw.init = &(struct clk_init_data){
		.name = "axi_b_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = axi_ab_clk_parent_names,
		.num_parents = ARRAY_SIZE(axi_ab_clk_parent_names),
		.flags = CLK_GET_RATE_NOCACHE,
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
		.parent_hws = (const struct clk_hw *[]) {
			&axi_b_sel.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
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
		.parent_hws = (const struct clk_hw *[]) {
			&axi_b_div.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED |
			CLK_IS_CRITICAL,
	},
};

static struct clk_regmap axi_a_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = AXI_CLK_CTRL0,
		.mask = 0x7,
		.shift = 10,
		.table = mux_table_axi,
	},
	.hw.init = &(struct clk_init_data){
		.name = "axi_a_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = axi_ab_clk_parent_names,
		.num_parents = ARRAY_SIZE(axi_ab_clk_parent_names),
		.flags = CLK_GET_RATE_NOCACHE,
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
		.parent_hws = (const struct clk_hw *[]) {
			&axi_a_sel.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
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
		.parent_hws = (const struct clk_hw *[]) {
			&axi_a_div.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED |
			CLK_IS_CRITICAL,
	},
};

static const struct clk_parent_data axi_clk_parent_names[] = {
	{ .hw = &axi_a.hw },
	{ .hw = &axi_b.hw }
};

static struct clk_regmap axi_clk = {
	.data = &(struct clk_regmap_mux_data){
		.offset = AXI_CLK_CTRL0,
		.mask = 0x1,
		.shift = 15,
	},
	.hw.init = &(struct clk_init_data){
		.name = "axi_clk",
		.ops = &clk_regmap_mux_ops,
		.parent_data = axi_clk_parent_names,
		.num_parents = ARRAY_SIZE(axi_clk_parent_names),
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

/* 12M/24M clock */
static struct clk_regmap c3_24m = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLK12_24_CTRL,
		.bit_idx = 11,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "24m",
		.ops = &clk_regmap_gate_ops,
		.parent_data = (const struct clk_parent_data []) {
			{ .fw_name = "xtal", }
		},
		.num_parents = 1,
	},
};

static struct clk_fixed_factor c3_24m_div2 = {
	.mult = 1,
	.div = 2,
	.hw.init = &(struct clk_init_data){
		.name = "24m_div2",
		.ops = &clk_fixed_factor_ops,
		.parent_data = (const struct clk_parent_data []) {
			{ .fw_name = "xtal", }
		},
		.num_parents = 1,
	},
};

static struct clk_regmap c3_12m = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLK12_24_CTRL,
		.bit_idx = 10,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "12m",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_24m_div2.hw,
		},
		.num_parents = 1,
	},
};

static struct clk_regmap c3_fclk_div2_divn_pre = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLK12_24_CTRL,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div2_divn_pre",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_fixed_pll.hw,
		},
		.num_parents = 1,
	},
};

static struct clk_regmap c3_fclk_div2_divn = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLK12_24_CTRL,
		.bit_idx = 12,
	},
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div2_divn",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_fclk_div2_divn_pre.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

/* gen clk */
static u32 mux_table_gen[] = { 0, 1, 5, 6, 7 };
static const struct clk_parent_data gen_parent_names[] = {
	{ .fw_name = "xtal", },
	{ .hw = &c3_rtc_clk.hw },
	{ .hw = &c3_gp0_pll.hw },
	{ .hw = &c3_gp1_pll.hw },
	{ .hw = &c3_hifi_pll.hw },
};

static struct clk_regmap c3_gen_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = GEN_CLK_CTRL,
		.mask = 0x1f,
		.shift = 12,
		.table = mux_table_gen,
	},
	.hw.init = &(struct clk_init_data){
		.name = "gen_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = gen_parent_names,
		.num_parents = ARRAY_SIZE(gen_parent_names),
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap c3_gen_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = GEN_CLK_CTRL,
		.shift = 0,
		.width = 11,
		.flags = CLK_DIVIDER_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "gen_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_gen_sel.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap c3_gen = {
	.data = &(struct clk_regmap_gate_data){
		.offset = GEN_CLK_CTRL,
		.bit_idx = 11,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "gen",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_gen_div.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

/* saradc */
static const struct clk_parent_data saradc_parent_names[] = {
	{ .fw_name = "xtal", },
	{ .hw = &c3_sys_clk.hw },
};

static struct clk_regmap c3_saradc_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = SAR_CLK_CTRL0,
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
		.parent_data = saradc_parent_names,
		.num_parents = ARRAY_SIZE(saradc_parent_names),
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap c3_saradc_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = SAR_CLK_CTRL0,
		.shift = 0,
		.width = 8,
		.flags = CLK_DIVIDER_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "saradc_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_saradc_sel.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap c3_saradc_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = SAR_CLK_CTRL0,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "saradc_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_saradc_div.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

/* pwm */
static const struct clk_parent_data pwm_parent_names[] = {
	{ .fw_name = "xtal", },
	{ .hw = &c3_gp1_pll.hw },
	{ .hw = &c3_fclk_div4.hw },
	{ .hw = &c3_fclk_div3.hw },
};

/*
 * add CLK_IGNORE_UNUSED flag for pwm controller GATE
 * clk core will disable unused clock, it may disable
 * vddcore voltage which controlled by one pwm in bl21.
 * add the flag to avoid changing cpu voltage.
 */
#define MESON_C3_PWM_SEL(_name, _reg, _mask, _shift)			\
struct clk_regmap _name = {						\
	.data = &(struct clk_regmap_mux_data) {				\
		.offset = (_reg),					\
		.mask = (_mask),					\
		.shift = (_shift),					\
	},								\
	.hw.init = &(struct clk_init_data) {				\
		.name = #_name,						\
		.ops = &clk_regmap_mux_ops,				\
		.parent_data = pwm_parent_names,			\
		.num_parents =  ARRAY_SIZE(pwm_parent_names),		\
		.flags = CLK_GET_RATE_NOCACHE,				\
	},								\
}

#define MESON_C3_PWM_DIV(_name, _reg, _shift, _width, _parent)		\
struct clk_regmap _name = {						\
	.data = &(struct clk_regmap_div_data) {				\
		.offset = (_reg),					\
		.shift = (_shift),					\
		.width = (_width),					\
		.flags = CLK_DIVIDER_ROUND_CLOSEST,			\
	},								\
	.hw.init = &(struct clk_init_data) {				\
		.name = #_name,						\
		.ops = &clk_regmap_divider_ops,				\
		.parent_hws = (const struct clk_hw *[]) {		\
			&(_parent).hw,					\
		},							\
		.num_parents =  1,					\
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,	\
	},								\
}

#define MESON_C3_PWM_GATE(_name, _reg, _bit, _parent)			\
struct clk_regmap _name = {						\
	.data = &(struct clk_regmap_gate_data){				\
		.offset = (_reg),					\
		.bit_idx = (_bit),					\
	},								\
	.hw.init = &(struct clk_init_data) {				\
		.name = #_name,						\
		.ops = &clk_regmap_gate_ops,				\
		.parent_hws = (const struct clk_hw *[]) {		\
			&(_parent).hw,					\
		},							\
		.num_parents = 1,					\
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,	\
	},								\
}

MESON_C3_PWM_SEL(pwm_a_sel, PWM_CLK_AB_CTRL, 0x3, 9);
MESON_C3_PWM_DIV(pwm_a_div, PWM_CLK_AB_CTRL, 0, 8, pwm_a_sel);
MESON_C3_PWM_GATE(pwm_a, PWM_CLK_AB_CTRL, 8, pwm_a_div);
MESON_C3_PWM_SEL(pwm_b_sel, PWM_CLK_AB_CTRL, 0x3, 25);
MESON_C3_PWM_DIV(pwm_b_div, PWM_CLK_AB_CTRL, 16, 8, pwm_b_sel);
MESON_C3_PWM_GATE(pwm_b, PWM_CLK_AB_CTRL, 24, pwm_b_div);

MESON_C3_PWM_SEL(pwm_c_sel, PWM_CLK_CD_CTRL, 0x3, 9);
MESON_C3_PWM_DIV(pwm_c_div, PWM_CLK_CD_CTRL, 0, 8, pwm_c_sel);
MESON_C3_PWM_GATE(pwm_c, PWM_CLK_CD_CTRL, 8, pwm_c_div);
MESON_C3_PWM_SEL(pwm_d_sel, PWM_CLK_CD_CTRL, 0x3, 25);
MESON_C3_PWM_DIV(pwm_d_div, PWM_CLK_CD_CTRL, 16, 8, pwm_d_sel);
MESON_C3_PWM_GATE(pwm_d, PWM_CLK_CD_CTRL, 24, pwm_d_div);

MESON_C3_PWM_SEL(pwm_e_sel, PWM_CLK_EF_CTRL, 0x3, 9);
MESON_C3_PWM_DIV(pwm_e_div, PWM_CLK_EF_CTRL, 0, 8, pwm_e_sel);
MESON_C3_PWM_GATE(pwm_e, PWM_CLK_EF_CTRL, 8, pwm_e_div);
MESON_C3_PWM_SEL(pwm_f_sel, PWM_CLK_EF_CTRL, 0x3, 25);
MESON_C3_PWM_DIV(pwm_f_div, PWM_CLK_EF_CTRL, 16, 8, pwm_f_sel);
MESON_C3_PWM_GATE(pwm_f, PWM_CLK_EF_CTRL, 24, pwm_f_div);

MESON_C3_PWM_SEL(pwm_g_sel, PWM_CLK_GH_CTRL, 0x3, 9);
MESON_C3_PWM_DIV(pwm_g_div, PWM_CLK_GH_CTRL, 0, 8, pwm_g_sel);
MESON_C3_PWM_GATE(pwm_g, PWM_CLK_GH_CTRL, 8, pwm_g_div);
MESON_C3_PWM_SEL(pwm_h_sel, PWM_CLK_GH_CTRL, 0x3, 25);
MESON_C3_PWM_DIV(pwm_h_div, PWM_CLK_GH_CTRL, 16, 8, pwm_h_sel);
MESON_C3_PWM_GATE(pwm_h, PWM_CLK_GH_CTRL, 24, pwm_h_div);

MESON_C3_PWM_SEL(pwm_i_sel, PWM_CLK_IJ_CTRL, 0x3, 9);
MESON_C3_PWM_DIV(pwm_i_div, PWM_CLK_IJ_CTRL, 0, 8, pwm_i_sel);
MESON_C3_PWM_GATE(pwm_i, PWM_CLK_IJ_CTRL, 8, pwm_i_div);
MESON_C3_PWM_SEL(pwm_j_sel, PWM_CLK_IJ_CTRL, 0x3, 25);
MESON_C3_PWM_DIV(pwm_j_div, PWM_CLK_IJ_CTRL, 16, 8, pwm_j_sel);
MESON_C3_PWM_GATE(pwm_j, PWM_CLK_IJ_CTRL, 24, pwm_j_div);

MESON_C3_PWM_SEL(pwm_k_sel, PWM_CLK_KL_CTRL, 0x3, 9);
MESON_C3_PWM_DIV(pwm_k_div, PWM_CLK_KL_CTRL, 0, 8, pwm_k_sel);
MESON_C3_PWM_GATE(pwm_k, PWM_CLK_KL_CTRL, 8, pwm_k_div);
MESON_C3_PWM_SEL(pwm_l_sel, PWM_CLK_KL_CTRL, 0x3, 25);
MESON_C3_PWM_DIV(pwm_l_div, PWM_CLK_KL_CTRL, 16, 8, pwm_l_sel);
MESON_C3_PWM_GATE(pwm_l, PWM_CLK_KL_CTRL, 24, pwm_l_div);

MESON_C3_PWM_SEL(pwm_m_sel, PWM_CLK_MN_CTRL, 0x3, 9);
MESON_C3_PWM_DIV(pwm_m_div, PWM_CLK_MN_CTRL, 0, 8, pwm_m_sel);
MESON_C3_PWM_GATE(pwm_m, PWM_CLK_MN_CTRL, 8, pwm_m_div);
MESON_C3_PWM_SEL(pwm_n_sel, PWM_CLK_MN_CTRL, 0x3, 25);
MESON_C3_PWM_DIV(pwm_n_div, PWM_CLK_MN_CTRL, 16, 8, pwm_n_sel);
MESON_C3_PWM_GATE(pwm_n, PWM_CLK_MN_CTRL, 24, pwm_n_div);
/* spicc clk */

/*    div2   |\         |\       _____
 *  ---------| |---DIV--| |     |     |    spicc out
 *  ---------| |        | |-----| GATE|---------
 *     ..... |/         | /     |_____|
 *  --------------------|/
 *                 24M
 */
static u32 mux_table_spicc[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
static const struct clk_parent_data spicc_parent_names[] = {
	{ .fw_name = "xtal", },
	{ .hw = &c3_sys_pll.hw },
	{ .hw = &c3_fclk_div4.hw },
	{ .hw = &c3_fclk_div3.hw },
	{ .hw = &c3_fclk_div2.hw },
	{ .hw = &c3_fclk_div5.hw },
	{ .hw = &c3_fclk_div7.hw },
	{ .hw = &c3_gp1_pll.hw },
};

static struct clk_regmap c3_spicc_0_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = SPICC_CLK_CTRL,
		.mask = 0x7,
		.shift = 7,
		.table = mux_table_spicc,
	},
	.hw.init = &(struct clk_init_data){
		.name = "spicc_0_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = spicc_parent_names,
		.num_parents = ARRAY_SIZE(spicc_parent_names),
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap c3_spicc_0_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = SPICC_CLK_CTRL,
		.shift = 0,
		.width = 6,
		.flags = CLK_DIVIDER_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "spicc_0_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_spicc_0_sel.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap c3_spicc_0 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = SPICC_CLK_CTRL,
		.bit_idx = 6,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "spicc_0",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_spicc_0_div.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap c3_spicc_1_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = SPICC_CLK_CTRL,
		.mask = 0x7,
		.shift = 23,
		.table = mux_table_spicc,
	},
	.hw.init = &(struct clk_init_data){
		.name = "spicc_1_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = spicc_parent_names,
		.num_parents = ARRAY_SIZE(spicc_parent_names),
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap c3_spicc_1_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = SPICC_CLK_CTRL,
		.shift = 16,
		.width = 6,
		.flags = CLK_DIVIDER_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "spicc_1_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_spicc_1_sel.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap c3_spicc_1 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = SPICC_CLK_CTRL,
		.bit_idx = 22,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "spicc_1",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_spicc_1_div.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

/* ts clk */
static struct clk_regmap c3_ts_clk_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = TS_CLK_CTRL,
		.shift = 0,
		.width = 8,
		.flags = CLK_DIVIDER_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "ts_clk_div",
		.ops = &clk_regmap_divider_ops,
		.parent_data = (const struct clk_parent_data []) {
			{ .fw_name = "xtal", }
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap c3_ts_clk = {
	.data = &(struct clk_regmap_gate_data){
		.offset = TS_CLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "ts_clk",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_ts_clk_div.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

/* spifc clk */
static u32 mux_table_spifc[] = { 0, 1, 2, 3, 5, 6, 7 };
static const struct clk_parent_data spifc_parent_names[] = {
	{ .hw = &c3_gp0_pll.hw },
	{ .hw = &c3_fclk_div2.hw },
	{ .hw = &c3_fclk_div3.hw },
	{ .hw = &c3_fclk_div2p5.hw },
	{ .hw = &c3_fclk_div4.hw },
	{ .hw = &c3_fclk_div5.hw },
	{ .hw = &c3_fclk_div7.hw },
};

static struct clk_regmap c3_spifc_clk_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = SPIFC_CLK_CTRL,
		.mask = 0x7,
		.shift = 9,
		.table = mux_table_spifc,
	},
	.hw.init = &(struct clk_init_data){
		.name = "spifc_clk_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = spifc_parent_names,
		.num_parents = ARRAY_SIZE(spifc_parent_names),
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap c3_spifc_clk_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = SPIFC_CLK_CTRL,
		.shift = 0,
		.width = 7,
		.flags = CLK_DIVIDER_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "spifc_clk_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_spifc_clk_sel.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap c3_spifc_clk = {
	.data = &(struct clk_regmap_gate_data){
		.offset = SPIFC_CLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "spifc_clk",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_spifc_clk_div.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

/* vdin meas clk */
static const struct clk_parent_data vdin_meas_clk_parent_names[] = {
	{ .fw_name = "xtal", },
	{ .hw = &c3_fclk_div4.hw },
	{ .hw = &c3_fclk_div3.hw },
	{ .hw = &c3_fclk_div5.hw },
	{ .hw = &c3_gp1_pll.hw },
	{ .hw = &c3_gp0_pll.hw },
	{ .hw = &c3_fclk_div2.hw },
	{ .hw = &c3_fclk_div7.hw },
};

static struct clk_regmap c3_vdin_meas_clk_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = VDIN_MEAS_CLK_CTRL,
		.mask = 0x7,
		.shift = 21,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vdin_meas_clk_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = vdin_meas_clk_parent_names,
		.num_parents = ARRAY_SIZE(vdin_meas_clk_parent_names),
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap c3_vdin_meas_clk_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = VDIN_MEAS_CLK_CTRL,
		.shift = 12,
		.width = 7,
		.flags = CLK_DIVIDER_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vdin_meas_clk_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_vdin_meas_clk_sel.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap c3_vdin_meas_clk = {
	.data = &(struct clk_regmap_gate_data){
		.offset = VDIN_MEAS_CLK_CTRL,
		.bit_idx = 20,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vdin_meas_clk",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_vdin_meas_clk_div.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

/* sd emmc clk */
static u32 mux_table_sd_emmc[] = { 0, 1, 2, 4, 5, 6, 7 };
static const struct clk_parent_data emmc_nand_parent_names[] = {
	{ .fw_name = "xtal", },
	{ .hw = &c3_fclk_div2.hw },
	{ .hw = &c3_fclk_div3.hw },
	{ .hw = &c3_fclk_div2p5.hw },
	{ .hw = &c3_fclk_div4.hw },
	{ .hw = &c3_gp1_pll.hw },
	{ .hw = &c3_gp0_pll.hw },
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
		.parent_data = emmc_nand_parent_names,
		.num_parents = ARRAY_SIZE(emmc_nand_parent_names),
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap sd_emmc_a_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = SD_EMMC_CLK_CTRL,
		.shift = 0,
		.width = 7,
		.flags = CLK_DIVIDER_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sd_emmc_a_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&sd_emmc_a_sel.hw,
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap sd_emmc_a = {
	.data = &(struct clk_regmap_gate_data){
		.offset = SD_EMMC_CLK_CTRL,
		.bit_idx = 7,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "sd_emmc_a",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&sd_emmc_a_div.hw,
		},
		.num_parents = 1,
		.flags = CLK_IGNORE_UNUSED,
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
		.parent_data = emmc_nand_parent_names,
		.num_parents = ARRAY_SIZE(emmc_nand_parent_names),
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap sd_emmc_b_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = SD_EMMC_CLK_CTRL,
		.shift = 16,
		.width = 7,
		.flags = CLK_DIVIDER_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sd_emmc_b_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&sd_emmc_b_sel.hw,
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap sd_emmc_b = {
	.data = &(struct clk_regmap_gate_data){
		.offset = SD_EMMC_CLK_CTRL,
		.bit_idx = 23,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "sd_emmc_b",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&sd_emmc_b_div.hw,
		},
		.num_parents = 1,
		.flags = CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap nand_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = NAND_CLK_CTRL,
		.mask = 0x7,
		.shift = 9,
		.table = mux_table_sd_emmc,
	},
	.hw.init = &(struct clk_init_data){
		.name = "nand_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = emmc_nand_parent_names,
		.num_parents = ARRAY_SIZE(emmc_nand_parent_names),
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap nand_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = NAND_CLK_CTRL,
		.shift = 0,
		.width = 7,
		.flags = CLK_DIVIDER_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "nand_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&nand_sel.hw,
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap nand = {
	.data = &(struct clk_regmap_gate_data){
		.offset = NAND_CLK_CTRL,
		.bit_idx = 7,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "nand",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&nand_div.hw,
		},
		.num_parents = 1,
		.flags = CLK_IGNORE_UNUSED,
	},
};

static struct clk_fixed_factor c3_eth_125m_div = {
	.mult = 1,
	.div = 8,
	.hw.init = &(struct clk_init_data){
		.name = "eth_125m_div",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_fclk_div2.hw,
		},
		.num_parents = 1,
	},
};

static struct clk_regmap c3_eth_125m = {
	.data = &(struct clk_regmap_gate_data){
		.offset = ETH_CLK_CTRL,
		.bit_idx = 7,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "eth_125m",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_eth_125m_div.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap c3_eth_rmii_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = ETH_CLK_CTRL,
		.shift = 0,
		.width = 7,
		.flags = CLK_DIVIDER_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "eth_rmii_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_fclk_div2.hw,
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap c3_eth_rmii = {
	.data = &(struct clk_regmap_gate_data){
		.offset = ETH_CLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "eth_rmii",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_eth_rmii_div.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

/*
 *rtc 32k clock (32k-xtal is unsupported)
 *
 *
 *                   rtc_xtal_clkin
 * 24M-xtal----GATE---------------------------|\
 *                   |                        | \
 *                   |------------|\          |  \
 *                   |            | \         |   \
 *	                 |  --------  |  |--GATE--|    |
 *	                 |  |      |  | /         |    |--GATE--rtc_clkout
 *	                 ---| DUAL |--|/          |   /
 *	                    |      |              |  /
 *	                    --------              | /
 * PAD----------------------------------------|/
 *
 *	   DUAL function:
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
 * rtc clock in gate
 */
static struct clk_regmap c3_rtc_xtal_clkin = {
	.data = &(struct clk_regmap_gate_data){
		.offset = RTC_BY_OSCIN_CTRL0,
		.bit_idx = 31,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "rtc_xtal_clkin",
		.ops = &clk_regmap_gate_ops,
		.parent_data = (const struct clk_parent_data []) {
			{ .fw_name = "xtal", }
		},
		.num_parents = 1,
	},
};

static const struct meson_clk_dualdiv_param c3_32k_div_table[] = {
	{
		.dual	= 1,
		.n1	= 733,
		.m1	= 8,
		.n2	= 732,
		.m2	= 11,
	},
	{},
};

static struct clk_regmap c3_rtc_32k_div = {
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
		.table = c3_32k_div_table,
	},
	.hw.init = &(struct clk_init_data){
		.name = "rtc_32k_div",
		.ops = &meson_clk_dualdiv_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_rtc_xtal_clkin.hw,
		},
		.num_parents = 1,
	},
};

static struct clk_regmap c3_rtc_32k_div_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = RTC_BY_OSCIN_CTRL1,
		.mask = 0x1,
		.shift = 24,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "rtc_32k_div_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_rtc_32k_div.hw,
			&c3_rtc_xtal_clkin.hw,
		},
		.num_parents = 2,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

/*
 * three parent for rtc clock out
 * pad is from where?
 */
static u32 rtc_clk_sel[] = { 0, 1, 2 };
static const struct clk_parent_data rtc_clk_sel_parent_names[] = {
	{ .hw = &c3_rtc_xtal_clkin.hw },
	{ .hw = &c3_rtc_32k_div_sel.hw },
	{ .fw_name = "pad", },
};

static struct clk_regmap c3_rtc_32k_sel = {
	.data = &(struct clk_regmap_mux_data) {
		.offset = RTC_CTRL,
		.mask = 0x3,
		.shift = 0,
		.table = rtc_clk_sel,
		.flags = CLK_MUX_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "rtc_clk_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = rtc_clk_sel_parent_names,
		.num_parents = 3,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap c3_rtc_clk = {
	.data = &(struct clk_regmap_gate_data){
		.offset = RTC_BY_OSCIN_CTRL0,
		.bit_idx = 30,
	},
	.hw.init = &(struct clk_init_data){
		.name = "rtc_clk",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_rtc_32k_sel.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

/* cpu clock */
static const struct cpu_dyn_table c3_cpu_dyn_table[] = {
	CPU_LOW_PARAMS(24000000,   0, 0, 0),
	CPU_LOW_PARAMS(100000000,  1, 1, 9),
	CPU_LOW_PARAMS(250000000,  1, 1, 3),
	CPU_LOW_PARAMS(333333333,  2, 1, 1),
	CPU_LOW_PARAMS(500000000,  1, 1, 1),
	CPU_LOW_PARAMS(667000000,  2, 0, 0),
	CPU_LOW_PARAMS(1000000000, 1, 0, 0),
};

static const struct clk_parent_data cpu_parent_names[] = {
	{ .fw_name = "xtal", },
	{ .hw = &c3_fclk_div2.hw },
	{ .hw = &c3_fclk_div3.hw },
};

static struct clk_regmap c3_cpu_dyn_clk = {
	.data = &(struct meson_sec_cpu_dyn_data){
		.table = c3_cpu_dyn_table,
		.table_cnt = ARRAY_SIZE(c3_cpu_dyn_table),
		.secid_dyn_rd = SECID_CPU_CLK_RD,
		.secid_dyn = SECID_CPU_CLK_DYN,
	},
	.hw.init = &(struct clk_init_data){
		.name = "cpu_dyn_clk",
		.ops = &meson_sec_cpu_dyn_ops,
		.parent_data = cpu_parent_names,
		.num_parents = ARRAY_SIZE(cpu_parent_names),
	},
};

/* cpu clocks */
/*  cpu_dyn_clk  |\
 *---------------| \     cts_cpu_clk
 *  sys_pll      |  |--------
 *---------------| /
 *               |/
 */
static struct clk_regmap c3_cpu_clk = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CPUCTRL_CLK_CTRL0,
		.mask = 0x1,
		.shift = 11,
		.flags = CLK_MUX_ROUND_CLOSEST,
		.smc_id = SECURE_CPU_CLK,
		.secid = SECID_CPU_CLK_SEL,
		.secid_rd = SECID_CPU_CLK_RD,
	},
	.hw.init = &(struct clk_init_data){
		.name = "cpu_clk",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&c3_cpu_dyn_clk.hw,
			&c3_sys_pll.hw,
		},
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

struct c3_cpu_clk_nb_data {
	struct notifier_block nb;
	struct clk_hw *sys_pll;
	struct clk_hw *cpu_clk;
	struct clk_hw *cpu_clk_dyn;
};

static int c3_cpu_clk_notifier_cb(struct notifier_block *nb,
				   unsigned long event, void *data)
{
	struct c3_cpu_clk_nb_data *nb_data =
		container_of(nb, struct c3_cpu_clk_nb_data, nb);
	struct clk_notifier_data *cnd = (struct clk_notifier_data *)data;

	switch (event) {
	case PRE_RATE_CHANGE:
		/*
		 * This notifier means sys_pll clock will be changed
		 * to feed cpu_clk, this the current path :
		 * cpu_clk
		 *    \- sys_pll
		 *          \- sys_pll_dco
		 */

		/* make sure cpu_clk 1G*/
		if (clk_set_rate(nb_data->cpu_clk_dyn->clk,
			cnd->new_rate > 1000000000 ? 1000000000 : cnd->new_rate))
			pr_err("%s in %d\n", __func__, __LINE__);
		/* Configure cpu_clk to use cpu_clk_dyn */
		clk_hw_set_parent(nb_data->cpu_clk,
				  nb_data->cpu_clk_dyn);

		/*
		 * Now, cpu_clk uses the dyn path
		 * cpu_clk
		 *    \- cpu_clk_dyn
		 *          \- cpu_clk_dynX
		 *                \- cpu_clk_dynX_sel
		 *		     \- cpu_clk_dynX_div
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

static struct c3_cpu_clk_nb_data c3_cpu_clk_nb_data = {
	.sys_pll = &c3_sys_pll.hw,
	.cpu_clk = &c3_cpu_clk.hw,
	.cpu_clk_dyn = &c3_cpu_dyn_clk.hw,
	.nb.notifier_call = c3_cpu_clk_notifier_cb,
};

/* hcodec clk */
static u32 mux_table_hcodec[] = { 0, 1, 2, 3, 4, 6, 7 };
static const struct clk_parent_data hcodec_parent_names[] = {
	{ .hw = &c3_fclk_div2p5.hw },
	{ .hw = &c3_fclk_div3.hw },
	{ .hw = &c3_fclk_div4.hw },
	{ .hw = &c3_fclk_div5.hw },
	{ .hw = &c3_fclk_div7.hw },
	{ .hw = &c3_gp0_pll.hw },
	{ .fw_name = "xtal", },
};

static struct clk_regmap hcodec_a_clk_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = VDEC_CLK_CTRL,
		.mask = 0x7,
		.shift = 9,
		.table = mux_table_hcodec,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hcodec_a_clk_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = hcodec_parent_names,
		.num_parents = ARRAY_SIZE(hcodec_parent_names),
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap hcodec_a_clk_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = VDEC_CLK_CTRL,
		.shift = 0,
		.width = 7,
		.flags = CLK_DIVIDER_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hcodec_a_clk_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&hcodec_a_clk_sel.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap hcodec_a_clk = {
	.data = &(struct clk_regmap_gate_data){
		.offset = VDEC_CLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hcodec_a_clk",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&hcodec_a_clk_div.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap hcodec_b_clk_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = VDEC3_CLK_CTRL,
		.mask = 0x7,
		.shift = 9,
		.table = mux_table_hcodec,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hcodec_b_clk_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = hcodec_parent_names,
		.num_parents = ARRAY_SIZE(hcodec_parent_names),
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap hcodec_b_clk_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = VDEC3_CLK_CTRL,
		.shift = 0,
		.width = 7,
		.flags = CLK_DIVIDER_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hcodec_b_clk_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&hcodec_b_clk_sel.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap hcodec_b_clk = {
	.data = &(struct clk_regmap_gate_data){
		.offset = VDEC3_CLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hcodec_b_clk",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&hcodec_b_clk_div.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap hcodec_clk = {
	.data = &(struct clk_regmap_mux_data){
		.offset = VDEC3_CLK_CTRL,
		.mask = 0x1,
		.shift = 15,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hcodec_clk",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&hcodec_a_clk.hw,
			&hcodec_b_clk.hw,
		},
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

static u32 mux_table_vc9000e[] = { 0, 1, 2, 3, 4, 5, 7 };
static const struct clk_parent_data vc9000e_parent_names[] = {
	{ .fw_name = "xtal", },
	{ .hw = &c3_fclk_div4.hw },
	{ .hw = &c3_fclk_div3.hw },
	{ .hw = &c3_fclk_div5.hw },
	{ .hw = &c3_fclk_div7.hw },
	{ .hw = &c3_fclk_div2p5.hw },
	{ .hw = &c3_gp0_pll.hw },
};

static struct clk_regmap vc9000e_aclk_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = VC9000E_CLK_CTRL,
		.mask = 0x7,
		.shift = 9,
		.table = mux_table_vc9000e,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vc9000e_aclk_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = vc9000e_parent_names,
		.num_parents = ARRAY_SIZE(vc9000e_parent_names),
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap vc9000e_aclk_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = VC9000E_CLK_CTRL,
		.shift = 0,
		.width = 7,
		.flags = CLK_DIVIDER_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vc9000e_aclk_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&vc9000e_aclk_sel.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap vc9000e_aclk = {
	.data = &(struct clk_regmap_gate_data){
		.offset = VC9000E_CLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vc9000e_aclk",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&vc9000e_aclk_div.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap vc9000e_core_clk_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = VC9000E_CLK_CTRL,
		.mask = 0x7,
		.shift = 25,
		.table = mux_table_vc9000e,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vc9000e_core_clk_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = vc9000e_parent_names,
		.num_parents = ARRAY_SIZE(vc9000e_parent_names),
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap vc9000e_core_clk_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = VC9000E_CLK_CTRL,
		.shift = 16,
		.width = 7,
		.flags = CLK_DIVIDER_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vc9000e_core_clk_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&vc9000e_core_clk_sel.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap vc9000e_core_clk = {
	.data = &(struct clk_regmap_gate_data){
		.offset = VC9000E_CLK_CTRL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vc9000e_core_clk",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&vc9000e_core_clk_div.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

/* dewarpa */
static u32 mux_table_dewarpa[] = { 0, 1, 2, 3, 4, 6, 7 };
static const struct clk_parent_data dewarpa_clk_parent_names[] = {
	{ .hw = &c3_fclk_div2p5.hw },
	{ .hw = &c3_fclk_div3.hw },
	{ .hw = &c3_fclk_div4.hw },
	{ .hw = &c3_fclk_div5.hw },
	{ .hw = &c3_gp0_pll.hw },
	{ .hw = &c3_gp1_pll.hw },
	{ .hw = &c3_fclk_div7.hw },
};

static struct clk_regmap dewarpa_clk_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = DEWARPA_CLK_CTRL,
		.mask = 0x7,
		.shift = 9,
		.table = mux_table_dewarpa,
	},
	.hw.init = &(struct clk_init_data){
		.name = "dewarpa_clk_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = dewarpa_clk_parent_names,
		.num_parents = ARRAY_SIZE(dewarpa_clk_parent_names),
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap dewarpa_clk_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = DEWARPA_CLK_CTRL,
		.shift = 0,
		.width = 7,
		.flags = CLK_DIVIDER_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "dewarpa_clk_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&dewarpa_clk_sel.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap dewarpa_clk = {
	.data = &(struct clk_regmap_gate_data){
		.offset = DEWARPA_CLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "dewarpa_clk",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&dewarpa_clk_div.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

/* MIPI */
static u32 mux_table_isp[] = { 0, 1, 2, 3, 4, 6, 7 };
static const struct clk_parent_data isp_parent_names[] = {
	{ .hw = &c3_fclk_div2p5.hw },
	{ .hw = &c3_fclk_div3.hw },
	{ .hw = &c3_fclk_div4.hw },
	{ .hw = &c3_fclk_div5.hw },
	{ .hw = &c3_gp0_pll.hw },
	{ .hw = &c3_gp1_pll.hw },
	{ .fw_name = "xtal", },
};

static struct clk_regmap csi_phy0_clk_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = ISP0_CLK_CTRL,
		.mask = 0x7,
		.shift = 25,
		.table = mux_table_isp,
	},
	.hw.init = &(struct clk_init_data){
		.name = "csi_phy0_clk_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = isp_parent_names,
		.num_parents = ARRAY_SIZE(isp_parent_names),
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap csi_phy0_clk_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = ISP0_CLK_CTRL,
		.shift = 16,
		.width = 7,
		.flags = CLK_DIVIDER_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "csi_phy0_clk_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&csi_phy0_clk_sel.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap csi_phy0_clk = {
	.data = &(struct clk_regmap_gate_data){
		.offset = ISP0_CLK_CTRL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "csi_phy0_clk",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&csi_phy0_clk_div.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap isp0_clk_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = ISP0_CLK_CTRL,
		.mask = 0x7,
		.shift = 9,
		.table = mux_table_isp,
	},
	.hw.init = &(struct clk_init_data){
		.name = "isp0_clk_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = isp_parent_names,
		.num_parents = ARRAY_SIZE(isp_parent_names),
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap isp0_clk_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = ISP0_CLK_CTRL,
		.shift = 0,
		.width = 7,
		.flags = CLK_DIVIDER_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "isp0_clk_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&isp0_clk_sel.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap isp0_clk = {
	.data = &(struct clk_regmap_gate_data){
		.offset = ISP0_CLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "isp0_clk",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&isp0_clk_div.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

/* nna */
static u32 mux_table_nna[] = { 0, 1, 2, 3, 4, 5, 6 };
static const struct clk_parent_data nna_parent_names[] = {
	{ .fw_name = "xtal" },
	{ .hw = &c3_fclk_div2p5.hw },
	{ .hw = &c3_fclk_div4.hw },
	{ .hw = &c3_fclk_div3.hw },
	{ .hw = &c3_fclk_div5.hw },
	{ .hw = &c3_fclk_div2.hw },
	{ .hw = &c3_gp1_pll.hw },
};

static struct clk_regmap nna_core_clk_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = NNA_CLK_CTRL,
		.mask = 0x7,
		.shift = 9,
		.table = mux_table_nna,
	},
	.hw.init = &(struct clk_init_data){
		.name = "nna_core_clk_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = nna_parent_names,
		.num_parents = ARRAY_SIZE(nna_parent_names),
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap nna_core_clk_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = NNA_CLK_CTRL,
		.shift = 0,
		.width = 7,
		.flags = CLK_DIVIDER_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "nna_core_clk_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&nna_core_clk_sel.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap nna_core_clk = {
	.data = &(struct clk_regmap_gate_data){
		.offset = NNA_CLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "nna_core_clk",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&nna_core_clk_div.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

/* vout clk */
static u32 mux_table_vout[] = { 0, 1, 2, 3, 4, 6, 7 };
static const struct clk_parent_data vout_parent_names[] = {
	{ .hw = &c3_gp1_pll.hw },
	{ .hw = &c3_fclk_div3.hw },
	{ .hw = &c3_fclk_div4.hw },
	{ .hw = &c3_fclk_div5.hw },
	{ .hw = &c3_gp0_pll.hw },
	{ .hw = &c3_fclk_div2p5.hw },
	{ .hw = &c3_fclk_div7.hw },
};

static struct clk_regmap vout_mclk_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = VOUTENC_CLK_CTRL,
		.mask = 0x7,
		.shift = 9,
		.table = mux_table_vout,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vout_mclk_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = vout_parent_names,
		.num_parents = ARRAY_SIZE(vout_parent_names),
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap vout_mclk_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = VOUTENC_CLK_CTRL,
		.shift = 0,
		.width = 7,
		.flags = CLK_DIVIDER_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vout_mclk_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&vout_mclk_sel.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap vout_mclk = {
	.data = &(struct clk_regmap_gate_data){
		.offset = VOUTENC_CLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vout_mclk",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&vout_mclk_div.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap vout_enc_clk_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = VOUTENC_CLK_CTRL,
		.mask = 0x7,
		.shift = 25,
		.table = mux_table_vout,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vout_enc_clk_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = vout_parent_names,
		.num_parents = ARRAY_SIZE(vout_parent_names),
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap vout_enc_clk_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = VOUTENC_CLK_CTRL,
		.shift = 16,
		.width = 7,
		.flags = CLK_DIVIDER_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vout_enc_clk_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&vout_enc_clk_sel.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap vout_enc_clk = {
	.data = &(struct clk_regmap_gate_data){
		.offset = VOUTENC_CLK_CTRL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vout_enc_clk",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&vout_enc_clk_div.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

//vout_enc_clk and vout_enc_clk_ph need check with vlsi
/* ge2d */
static u32 ge2d_table_vout[] = { 0, 1, 2, 3, 5, 6 };
static const struct clk_parent_data ge2d_clk_parent_names[] = {
	{ .fw_name = "xtal" },
	{ .hw = &c3_fclk_div2p5.hw },
	{ .hw = &c3_fclk_div3.hw },
	{ .hw = &c3_fclk_div4.hw },
	{ .hw = &c3_fclk_div5.hw },
	{ .hw = &c3_gp0_pll.hw },
};

static struct clk_regmap ge2d_clk_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = GE2D_CLK_CTRL,
		.mask = 0x7,
		.shift = 9,
		.table = ge2d_table_vout,
	},
	.hw.init = &(struct clk_init_data){
		.name = "ge2d_clk_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = ge2d_clk_parent_names,
		.num_parents = ARRAY_SIZE(ge2d_clk_parent_names),
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap ge2d_clk_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = GE2D_CLK_CTRL,
		.shift = 0,
		.width = 7,
		.flags = CLK_DIVIDER_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "ge2d_clk_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&ge2d_clk_sel.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap ge2d_clk = {
	.data = &(struct clk_regmap_gate_data){
		.offset = GE2D_CLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "ge2d_clk",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&ge2d_clk_div.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

/* vapb clk */
static u32 mux_table_vapb[] = { 0, 1, 2, 3, 4, 6 };
static const struct clk_parent_data vapb_clk_parent_names[] = {
	{ .hw = &c3_fclk_div2p5.hw },
	{ .hw = &c3_fclk_div3.hw },
	{ .hw = &c3_fclk_div4.hw },
	{ .hw = &c3_fclk_div5.hw },
	{ .hw = &c3_gp0_pll.hw },
	{ .hw = &c3_gp1_pll.hw },
};

static struct clk_regmap vapb_clk_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = VAPB_CLK_CTRL,
		.mask = 0x7,
		.shift = 9,
		.table = mux_table_vapb,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vapb_clk_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = vapb_clk_parent_names,
		.num_parents = ARRAY_SIZE(vapb_clk_parent_names),
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap vapb_clk_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = VAPB_CLK_CTRL,
		.shift = 0,
		.width = 7,
		.flags = CLK_DIVIDER_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vapb_clk_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&vapb_clk_sel.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap vapb_clk = {
	.data = &(struct clk_regmap_gate_data){
		.offset = VAPB_CLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vapb_clk",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&vapb_clk_div.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

/* dsi_phy clk */
static u32 mux_table_dsi_phy[] = { 0, 1, 3, 4, 5, 6, 7 };
static const struct clk_parent_data dsi_phy_clk_parent_names[] = {
	{ .hw = &c3_gp1_pll.hw },
	{ .hw = &c3_gp0_pll.hw },
	{ .hw = &c3_fclk_div3.hw },
	{ .hw = &c3_fclk_div2.hw },
	{ .hw = &c3_fclk_div2p5.hw },
	{ .hw = &c3_fclk_div4.hw },
	{ .hw = &c3_fclk_div7.hw },
};

static struct clk_regmap dsi_phy_clk_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = MIPIDSI_PHY_CLK_CTRL,
		.mask = 0x7,
		.shift = 12,
		.table = mux_table_dsi_phy,
	},
	.hw.init = &(struct clk_init_data){
		.name = "dsi_phy_clk_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = dsi_phy_clk_parent_names,
		.num_parents = ARRAY_SIZE(dsi_phy_clk_parent_names),
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap dsi_phy_clk_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = MIPIDSI_PHY_CLK_CTRL,
		.shift = 0,
		.width = 7,
		.flags = CLK_DIVIDER_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "dsi_phy_clk_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&dsi_phy_clk_sel.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap dsi_phy_clk = {
	.data = &(struct clk_regmap_gate_data){
		.offset = MIPIDSI_PHY_CLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "dsi_phy_clk",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&dsi_phy_clk_div.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

/* Array of all clocks provided by this provider */
static struct clk_hw_onecell_data c3_hw_onecell_data = {
	.hws = {
		[CLKID_FIXED_PLL_VCO]		= &c3_fixed_pll_vco.hw,
		[CLKID_FIXED_PLL]		= &c3_fixed_pll.hw,
		[CLKID_FCLK50M_DIV40]		= &c3_fclk50_div40.hw,
		[CLKID_FCLK50M]			= &c3_fclk50M.hw,
		[CLKID_FCLK_DIV2_DIV]		= &c3_fclk_div2_div.hw,
		[CLKID_FCLK_DIV2]		= &c3_fclk_div2.hw,
		[CLKID_FCLK_DIV2P5_DIV]		= &c3_fclk_div2p5_div.hw,
		[CLKID_FCLK_DIV2P5]		= &c3_fclk_div2p5.hw,
		[CLKID_FCLK_DIV3_DIV]		= &c3_fclk_div3_div.hw,
		[CLKID_FCLK_DIV3]		= &c3_fclk_div3.hw,
		[CLKID_FCLK_DIV4_DIV]		= &c3_fclk_div4_div.hw,
		[CLKID_FCLK_DIV4]		= &c3_fclk_div4.hw,
		[CLKID_FCLK_DIV5_DIV]		= &c3_fclk_div5_div.hw,
		[CLKID_FCLK_DIV5]		= &c3_fclk_div5.hw,
		[CLKID_FCLK_DIV7_DIV]		= &c3_fclk_div7_div.hw,
		[CLKID_FCLK_DIV7]		= &c3_fclk_div7.hw,
		[CLKID_GP0_PLL_VCO]		= &c3_gp0_pll_vco.hw,
		[CLKID_GP0_PLL]			= &c3_gp0_pll.hw,
		[CLKID_GP1_PLL_VCO]		= &c3_gp1_pll_vco.hw,
		[CLKID_GP1_PLL]			= &c3_gp1_pll.hw,
		[CLKID_MCLK_PLL_VCO]		= &c3_mclk_pll_vco.hw,
		[CLKID_MCLK_PLL]		= &c3_mclk_pll.hw,
		[CLKID_MCLK_PLL_CLK]		= &c3_mclk_pll_clk.hw,
		[CLKID_MCLK_0_SEL]		= &c3_mclk_0_sel.hw,
		[CLKID_MCLK_0_SEL_OUT]		= &c3_mclk_0_sel_out.hw,
		[CLKID_MCLK_0_DIV2]		= &c3_mclk_0_div2.hw,
		[CLKID_MCLK_0]			= &c3_mclk_0.hw,
		[CLKID_MCLK_1_SEL]		= &c3_mclk_1_sel.hw,
		[CLKID_MCLK_1_SEL_OUT]		= &c3_mclk_1_sel_out.hw,
		[CLKID_MCLK_1_DIV2]		= &c3_mclk_1_div2.hw,
		[CLKID_MCLK_1]			= &c3_mclk_1.hw,
		[CLKID_HIFI_PLL_VCO]		= &c3_hifi_pll_vco.hw,
		[CLKID_HIFI_PLL]		= &c3_hifi_pll.hw,
		[CLKID_SYS_PLL_VCO]		= &c3_sys_pll_vco.hw,
		[CLKID_SYS_PLL]			= &c3_sys_pll.hw,
		[CLKID_SYS_B_SEL]		= &c3_sys_b_sel.hw,
		[CLKID_SYS_B_DIV]		= &c3_sys_b_div.hw,
		[CLKID_SYS_B]			= &c3_sys_b.hw,
		[CLKID_SYS_A_SEL]		= &c3_sys_a_sel.hw,
		[CLKID_SYS_A_DIV]		= &c3_sys_a_div.hw,
		[CLKID_SYS_A]			= &c3_sys_a.hw,
		[CLKID_SYS_CLK]			= &c3_sys_clk.hw,
		[CLKID_AXI_B_SEL]		= &axi_b_sel.hw,
		[CLKID_AXI_B_DIV]		= &axi_b_div.hw,
		[CLKID_AXI_B]			= &axi_b.hw,
		[CLKID_AXI_A_SEL]		= &axi_a_sel.hw,
		[CLKID_AXI_A_DIV]		= &axi_a_div.hw,
		[CLKID_AXI_A]			= &axi_a.hw,
		[CLKID_AXI_CLK]			= &axi_clk.hw,
		[CLKID_CPU_DYN_CLK]		= &c3_cpu_dyn_clk.hw,
		[CLKID_CPU_CLK]			= &c3_cpu_clk.hw,
		[CLKID_XTAL_DDRPLL]		= &xtal_ddrpll.hw,
		[CLKID_XTAL_DDRPHY]		= &xtal_ddrphy.hw,
		[CLKID_XTAL_PLLTOP]		= &xtal_plltop.hw,
		[CLKID_XTAL_USBPLL]		= &xtal_usbpll.hw,
		[CLKID_XTAL_MIPI_ISP_VOUT]	= &xtal_isp0_clk_vout.hw,
		[CLKID_XTAL_MCLKPLL]		= &xtal_mclkpll.hw,
		[CLKID_XTAL_USBCTRL]		= &xtal_usbctrl.hw,
		[CLKID_XTAL_ETHPLL]		= &xtal_ethpll.hw,

		[CLKID_SYS_RESET_CTRL]		= &sys_reset_ctrl.hw,
		[CLKID_SYS_PWR_CTRL]		= &sys_pwr_ctrl.hw,
		[CLKID_SYS_PAD_CTRL]		= &sys_pad_ctrl.hw,
		[CLKID_SYS_CTRL]		= &sys_ctrl.hw,
		[CLKID_SYS_TS_PLL]		= &sys_ts_pll.hw,
		[CLKID_SYS_DEV_ARB]		= &sys_dev_arb.hw,
		[CLKID_SYS_MMC_PCLK]		= &sys_mmc_pclk.hw,
		[CLKID_SYS_CAPU]		= &sys_capu.hw,
		[CLKID_SYS_CPU_CTRL]		= &sys_cpu_ctrl.hw,
		[CLKID_SYS_JTAG_CTRL]		= &sys_jtag_ctrl.hw,
		[CLKID_SYS_IR_CTRL]		= &sys_ir_ctrl.hw,
		[CLKID_SYS_IRQ_CTRL]		= &sys_irq_ctrl.hw,
		[CLKID_SYS_MSR_CLK]		= &sys_msr_clk.hw,
		[CLKID_SYS_ROM]			= &sys_rom.hw,
		[CLKID_SYS_UART_F]		= &sys_uart_f.hw,
		[CLKID_SYS_CPU_ARB]		= &sys_cpu_apb.hw,
		[CLKID_SYS_RSA]			= &sys_rsa.hw,
		[CLKID_SYS_SAR_ADC]		= &sys_sar_adc.hw,
		[CLKID_SYS_STARTUP]		= &sys_startup.hw,
		[CLKID_SYS_SECURE]		= &sys_secure.hw,
		[CLKID_SYS_SPIFC]		= &sys_spifc.hw,
		[CLKID_SYS_NNA]			= &sys_nna.hw,
		[CLKID_SYS_ETH_MAC]		= &sys_eth_mac.hw,
		[CLKID_SYS_GIC]			= &sys_gic.hw,
		[CLKID_SYS_RAMA]		= &sys_rama.hw,
		[CLKID_SYS_BIG_NIC]		= &sys_big_nic.hw,
		[CLKID_SYS_RAMB]		= &sys_ramb.hw,
		[CLKID_SYS_AUDIO_PCLK]		= &sys_audio_PCLK_to_top.hw,
		[CLKID_SYS_PWM_KL]		= &sys_pwm_kl.hw,
		[CLKID_SYS_PWM_IJ]		= &sys_pwm_ij.hw,
		[CLKID_SYS_USB]			= &sys_usb.hw,
		[CLKID_SYS_SD_EMMC_A]		= &sys_sd_emmc_a.hw,
		[CLKID_SYS_SD_EMMC_C]		= &sys_sd_emmc_c.hw,
		[CLKID_SYS_PWM_AB]		= &sys_pwm_ab.hw,
		[CLKID_SYS_PWM_CD]		= &sys_pwm_cd.hw,
		[CLKID_SYS_PWM_EF]		= &sys_pwm_ef.hw,
		[CLKID_SYS_PWM_GH]		= &sys_pwm_gh.hw,
		[CLKID_SYS_SPICC_1]		= &sys_spicc_1.hw,
		[CLKID_SYS_SPICC_0]		= &sys_spicc_0.hw,
		[CLKID_SYS_UART_A]		= &sys_uart_a.hw,
		[CLKID_SYS_UART_B]		= &sys_uart_b.hw,
		[CLKID_SYS_UART_C]		= &sys_uart_c.hw,
		[CLKID_SYS_UART_D]		= &sys_uart_d.hw,
		[CLKID_SYS_UART_E]		= &sys_uart_e.hw,
		[CLKID_SYS_I2C_M_A]		= &sys_i2c_m_a.hw,
		[CLKID_SYS_I2C_M_B]		= &sys_i2c_m_b.hw,
		[CLKID_SYS_I2C_M_C]		= &sys_i2c_m_c.hw,
		[CLKID_SYS_I2C_M_D]		= &sys_i2c_m_d.hw,
		[CLKID_SYS_I2S_S_A]		= &sys_i2c_s_a.hw,
		[CLKID_SYS_RTC]			= &sys_rtc.hw,
		[CLKID_SYS_GE2D]		= &sys_ge2d.hw,
		[CLKID_SYS_ISP]			= &sys_isp.hw,
		[CLKID_SYS_GPV_ISP_NIC]		= &sys_gpv_isp_nic.hw,
		[CLKID_SYS_GPV_CVE_NIC]		= &sys_gpv_cve_nic.hw,
		[CLKID_SYS_MIPI_DSI_HOST]	= &sys_mipi_dsi_host.hw,
		[CLKID_SYS_MIPI_DSI_PHY]	= &sys_mipi_dsi_phy.hw,
		[CLKID_SYS_ETH_PHY]		= &sys_eth_phy.hw,
		[CLKID_SYS_ACODEC]		= &sys_acodec.hw,
		[CLKID_SYS_DWAP]		= &sys_dwap.hw,
		[CLKID_SYS_DOS]			= &sys_dos.hw,
		[CLKID_SYS_CVE]			= &sys_cve.hw,
		[CLKID_SYS_VOUT]		= &sys_vout.hw,
		[CLKID_SYS_VC9000E]		= &sys_vc9000e.hw,
		[CLKID_SYS_PWM_MN]		= &sys_pwm_mn.hw,
		[CLKID_SYS_SD_EMMC_B]		= &sys_sd_emmc_b.hw,

		[CLKID_AXI_SYS_NIC]		= &axi_sys_nic.hw,
		[CLKID_AXI_ISP_NIC]		= &axi_isp_nic.hw,
		[CLKID_AXI_CVE_NIC]		= &axi_cve_nic.hw,
		[CLKID_AXI_RAMB]		= &axi_ramb.hw,
		[CLKID_AXI_RAMA]		= &axi_rama.hw,
		[CLKID_AXI_CPU_DMC]		= &axi_cpu_dmc.hw,
		[CLKID_AXI_NIC]			= &axi_nic.hw,
		[CLKID_AXI_DMA]			= &axi_dma.hw,
		[CLKID_AXI_MUX_NIC]		= &axi_mux_nic.hw,
		[CLKID_AXI_CAPU]		= &axi_capu.hw,
		[CLKID_AXI_CVE]			= &axi_cve.hw,
		[CLKID_AXI_DEV1_DMC]		= &axi_dev1_dmc.hw,
		[CLKID_AXI_DEV0_DMC]		= &axi_dev0_dmc.hw,
		[CLKID_AXI_DSP_DMC]		= &axi_dsp_dmc.hw,
		[CLKID_DSI_PHY_SEL]		= &dsi_phy_clk_sel.hw,
		[CLKID_DSI_PHY_DIV]		= &dsi_phy_clk_div.hw,
		[CLKID_DSI_PHY_CLK]		= &dsi_phy_clk.hw,
		[CLKID_24M]			= &c3_24m.hw,
		[CLKID_24M_DIV2]		= &c3_24m_div2.hw,
		[CLKID_12M]			= &c3_12m.hw,
		[CLKID_DIV2_PRE]		= &c3_fclk_div2_divn_pre.hw,
		[CLKID_FCLK_DIV2_DIVN]		= &c3_fclk_div2_divn.hw,
		[CLKID_GEN_SEL]			= &c3_gen_sel.hw,
		[CLKID_GEN_DIV]			= &c3_gen_div.hw,
		[CLKID_GEN]			= &c3_gen.hw,
		[CLKID_SARADC_SEL]		= &c3_saradc_sel.hw,
		[CLKID_SARADC_DIV]		= &c3_saradc_div.hw,
		[CLKID_SARADC_GATE]		= &c3_saradc_gate.hw,
		[CLKID_PWM_A_SEL]		= &pwm_a_sel.hw,
		[CLKID_PWM_A_DIV]		= &pwm_a_div.hw,
		[CLKID_PWM_A]			= &pwm_a.hw,
		[CLKID_PWM_B_SEL]		= &pwm_b_sel.hw,
		[CLKID_PWM_B_DIV]		= &pwm_b_div.hw,
		[CLKID_PWM_B]			= &pwm_b.hw,
		[CLKID_PWM_C_SEL]		= &pwm_c_sel.hw,
		[CLKID_PWM_C_DIV]		= &pwm_c_div.hw,
		[CLKID_PWM_C]			= &pwm_c.hw,
		[CLKID_PWM_D_SEL]		= &pwm_d_sel.hw,
		[CLKID_PWM_D_DIV]		= &pwm_d_div.hw,
		[CLKID_PWM_D]			= &pwm_d.hw,
		[CLKID_PWM_E_SEL]		= &pwm_e_sel.hw,
		[CLKID_PWM_E_DIV]		= &pwm_e_div.hw,
		[CLKID_PWM_E]			= &pwm_e.hw,
		[CLKID_PWM_F_SEL]		= &pwm_f_sel.hw,
		[CLKID_PWM_F_DIV]		= &pwm_f_div.hw,
		[CLKID_PWM_F]			= &pwm_f.hw,
		[CLKID_PWM_G_SEL]		= &pwm_g_sel.hw,
		[CLKID_PWM_G_DIV]		= &pwm_g_div.hw,
		[CLKID_PWM_G]			= &pwm_g.hw,
		[CLKID_PWM_H_SEL]		= &pwm_h_sel.hw,
		[CLKID_PWM_H_DIV]		= &pwm_h_div.hw,
		[CLKID_PWM_H]			= &pwm_h.hw,
		[CLKID_PWM_I_SEL]		= &pwm_i_sel.hw,
		[CLKID_PWM_I_DIV]		= &pwm_i_div.hw,
		[CLKID_PWM_I]			= &pwm_i.hw,
		[CLKID_PWM_J_SEL]		= &pwm_j_sel.hw,
		[CLKID_PWM_J_DIV]		= &pwm_j_div.hw,
		[CLKID_PWM_J]			= &pwm_j.hw,
		[CLKID_PWM_K_SEL]		= &pwm_k_sel.hw,
		[CLKID_PWM_K_DIV]		= &pwm_k_div.hw,
		[CLKID_PWM_K]			= &pwm_k.hw,
		[CLKID_PWM_L_SEL]		= &pwm_l_sel.hw,
		[CLKID_PWM_L_DIV]		= &pwm_l_div.hw,
		[CLKID_PWM_L]			= &pwm_l.hw,
		[CLKID_PWM_M_SEL]		= &pwm_m_sel.hw,
		[CLKID_PWM_M_DIV]		= &pwm_m_div.hw,
		[CLKID_PWM_M]			= &pwm_m.hw,
		[CLKID_PWM_N_SEL]		= &pwm_n_sel.hw,
		[CLKID_PWM_N_DIV]		= &pwm_n_div.hw,
		[CLKID_PWM_N]			= &pwm_n.hw,
		[CLKID_SPICC_A_SEL]		= &c3_spicc_0_sel.hw,
		[CLKID_SPICC_A_DIV]		= &c3_spicc_0_div.hw,
		[CLKID_SPICC_A]			= &c3_spicc_0.hw,
		[CLKID_TS_DIV]			= &c3_ts_clk_div.hw,
		[CLKID_TS]			= &c3_ts_clk.hw,
		[CLKID_ETH_RMII_DIV]		= &c3_eth_rmii_div.hw,
		[CLKID_ETH_RMII]		= &c3_eth_rmii.hw,
		[CLKID_ETH_125M_DIV]		= &c3_eth_125m_div.hw,
		[CLKID_ETH_125M]		= &c3_eth_125m.hw,
		[CLKID_SPICC_B_SEL]		= &c3_spicc_1_sel.hw,
		[CLKID_SPICC_B_DIV]		= &c3_spicc_1_div.hw,
		[CLKID_SPICC_B]			= &c3_spicc_1.hw,
		[CLKID_SPIFC_SEL]		= &c3_spifc_clk_sel.hw,
		[CLKID_SPIFC_DIV]		= &c3_spifc_clk_div.hw,
		[CLKID_SPIFC]			= &c3_spifc_clk.hw,
		[CLKID_USB_BUS_SEL]		= &c3_vdin_meas_clk_sel.hw,
		[CLKID_USB_BUS_DIV]		= &c3_vdin_meas_clk_div.hw,
		[CLKID_USB_BUS]			= &c3_vdin_meas_clk.hw,
		[CLKID_SD_EMMC_A_SEL]		= &sd_emmc_a_sel.hw,
		[CLKID_SD_EMMC_A_DIV]		= &sd_emmc_a_div.hw,
		[CLKID_SD_EMMC_A]		= &sd_emmc_a.hw,
		[CLKID_SD_EMMC_B_SEL]		= &sd_emmc_b_sel.hw,
		[CLKID_SD_EMMC_B_DIV]		= &sd_emmc_b_div.hw,
		[CLKID_SD_EMMC_B]		= &sd_emmc_b.hw,
		[CLKID_SD_EMMC_C_SEL]		= &nand_sel.hw,
		[CLKID_SD_EMMC_C_DIV]		= &nand_div.hw,
		[CLKID_SD_EMMC_C]		= &nand.hw,
		[CLKID_RTC_XTAL_CLKIN]		= &c3_rtc_xtal_clkin.hw,
		[CLKID_RTC_32K_DIV]		= &c3_rtc_32k_div.hw,
		[CLKID_RTC_32K_DIV_SEL]		= &c3_rtc_32k_div_sel.hw,
		[CLKID_RTC_32K_SEL]		= &c3_rtc_32k_sel.hw,
		[CLKID_RTC_CLK]			= &c3_rtc_clk.hw,
		[CLKID_HCODEC_A_SEL]		= &hcodec_a_clk_sel.hw,
		[CLKID_HCODEC_A_DIV]		= &hcodec_a_clk_div.hw,
		[CLKID_HCODEC_A_CLK]		= &hcodec_a_clk.hw,
		[CLKID_HCODEC_B_SEL]		= &hcodec_b_clk_sel.hw,
		[CLKID_HCODEC_B_DIV]		= &hcodec_b_clk_div.hw,
		[CLKID_HCODEC_B_CLK]		= &hcodec_b_clk.hw,
		[CLKID_HCODEC_CLK]		= &hcodec_clk.hw,
		[CLKID_VC9000E_ACLK_SEL]	= &vc9000e_aclk_sel.hw,
		[CLKID_VC9000E_ACLK_DIV]	= &vc9000e_aclk_div.hw,
		[CLKID_VC9000E_ACLK]		= &vc9000e_aclk.hw,
		[CLKID_DEWARPA_CLK_SEL]		= &dewarpa_clk_sel.hw,
		[CLKID_DEWARPA_CLK_DIV]		= &dewarpa_clk_div.hw,
		[CLKID_DEWARPA_CLK]		= &dewarpa_clk.hw,
		[CLKID_VC9000E_CORE_CLK_SEL]	= &vc9000e_core_clk_sel.hw,
		[CLKID_VC9000E_CORE_CLK_DIV]	= &vc9000e_core_clk_div.hw,
		[CLKID_VC9000E_CORE_CLK]	= &vc9000e_core_clk.hw,
		[CLKID_CSI_PHY0_CLK_SEL]	= &csi_phy0_clk_sel.hw,
		[CLKID_CSI_PHY0_CLK_DIV]	= &csi_phy0_clk_div.hw,
		[CLKID_CSI_PHY0_CLK]		= &csi_phy0_clk.hw,
		[CLKID_ISP0_CLK_SEL]		= &isp0_clk_sel.hw,
		[CLKID_ISP0_CLK_DIV]		= &isp0_clk_div.hw,
		[CLKID_ISP0_CLK]		= &isp0_clk.hw,
		[CLKID_NNA_CORE_SEL]		= &nna_core_clk_sel.hw,
		[CLKID_NNA_CORE_DIV]		= &nna_core_clk_div.hw,
		[CLKID_NNA_CORE_CLK]		= &nna_core_clk.hw,
		[CLKID_VOUT_MCLK_SEL]		= &vout_mclk_sel.hw,
		[CLKID_VOUT_MCLK_DIV]		= &vout_mclk_div.hw,
		[CLKID_VOUT_MCLK]		= &vout_mclk.hw,
		[CLKID_VOUT_ENC_CLK_SEL]	= &vout_enc_clk_sel.hw,
		[CLKID_VOUT_ENC_CLK_DIV]	= &vout_enc_clk_div.hw,
		[CLKID_VOUT_ENC_CLK]		= &vout_enc_clk.hw,
		[CLKID_GE2D_SEL]		= &ge2d_clk_sel.hw,
		[CLKID_GE2D_DIV]		= &ge2d_clk_div.hw,
		[CLKID_GE2D_CLK]		= &ge2d_clk.hw,
		[CLKID_VAPB_CLK_SEL]		= &vapb_clk_sel.hw,
		[CLKID_VAPB_CLK_DIV]		= &vapb_clk_div.hw,
		[CLKID_VAPB_CLK]		= &vapb_clk.hw,
		[NR_CLKS]			= NULL,
	},
	.num = NR_CLKS,
};

/* Convenience table to populate regmap in .probe */
static struct clk_regmap *const c3_clk_regmaps[] = {
	&xtal_ddrpll,
	&xtal_ddrphy,
	&xtal_plltop,
	&xtal_usbpll,
	&xtal_isp0_clk_vout,
	&xtal_mclkpll,
	&xtal_usbctrl,
	&xtal_ethpll,
	&sys_reset_ctrl,
	&sys_pwr_ctrl,
	&sys_pad_ctrl,
	&sys_ctrl,
	&sys_ts_pll,
	&sys_dev_arb,
	&sys_mmc_pclk,
	&sys_capu,
	&sys_cpu_ctrl,
	&sys_jtag_ctrl,
	&sys_ir_ctrl,
	&sys_irq_ctrl,
	&sys_msr_clk,
	&sys_rom,
	&sys_uart_f,
	&sys_cpu_apb,
	&sys_rsa,
	&sys_sar_adc,
	&sys_startup,
	&sys_secure,
	&sys_spifc,
	&sys_nna,
	&sys_eth_mac,
	&sys_gic,
	&sys_rama,
	&sys_big_nic,
	&sys_ramb,
	&sys_audio_PCLK_to_top,
	&sys_pwm_kl,
	&sys_pwm_ij,
	&sys_usb,
	&sys_sd_emmc_a,
	&sys_sd_emmc_c,
	&sys_pwm_ab,
	&sys_pwm_cd,
	&sys_pwm_ef,
	&sys_pwm_gh,
	&sys_spicc_1,
	&sys_spicc_0,
	&sys_uart_a,
	&sys_uart_b,
	&sys_uart_c,
	&sys_uart_d,
	&sys_uart_e,
	&sys_i2c_m_a,
	&sys_i2c_m_b,
	&sys_i2c_m_c,
	&sys_i2c_m_d,
	&sys_i2c_s_a,
	&sys_rtc,
	&sys_ge2d,
	&sys_isp,
	&sys_gpv_isp_nic,
	&sys_gpv_cve_nic,
	&sys_mipi_dsi_host,
	&sys_mipi_dsi_phy,
	&sys_eth_phy,
	&sys_acodec,
	&sys_dwap,
	&sys_dos,
	&sys_cve,
	&sys_vout,
	&sys_vc9000e,
	&sys_pwm_mn,
	&sys_sd_emmc_b,
	&axi_sys_nic,
	&axi_isp_nic,
	&axi_cve_nic,
	&axi_ramb,
	&axi_rama,
	&axi_cpu_dmc,
	&axi_nic,
	&axi_dma,
	&axi_mux_nic,
	&axi_capu,
	&axi_cve,
	&axi_dev1_dmc,
	&axi_dev0_dmc,
	&axi_dsp_dmc,
	&c3_sys_b_sel,
	&c3_sys_b_div,
	&c3_sys_b,
	&c3_sys_a_sel,
	&c3_sys_a_div,
	&c3_sys_a,
	&c3_sys_clk,
	&axi_b_sel,
	&axi_b_div,
	&axi_b,
	&axi_a_sel,
	&axi_a_div,
	&axi_a,
	&axi_clk,
	&dsi_phy_clk_sel,
	&dsi_phy_clk_div,
	&dsi_phy_clk,
	&c3_24m,
	&c3_12m,
	&c3_fclk_div2_divn_pre,
	&c3_fclk_div2_divn,
	&c3_gen_sel,
	&c3_gen_div,
	&c3_gen,
	&c3_saradc_sel,
	&c3_saradc_div,
	&c3_saradc_gate,
	&pwm_a_sel,
	&pwm_a_div,
	&pwm_a,
	&pwm_b_sel,
	&pwm_b_div,
	&pwm_b,
	&pwm_c_sel,
	&pwm_c_div,
	&pwm_c,
	&pwm_d_sel,
	&pwm_d_div,
	&pwm_d,
	&pwm_e_sel,
	&pwm_e_div,
	&pwm_e,
	&pwm_f_sel,
	&pwm_f_div,
	&pwm_f,
	&pwm_g_sel,
	&pwm_g_div,
	&pwm_g,
	&pwm_h_sel,
	&pwm_h_div,
	&pwm_h,
	&pwm_i_sel,
	&pwm_i_div,
	&pwm_i,
	&pwm_j_sel,
	&pwm_j_div,
	&pwm_j,
	&pwm_k_sel,
	&pwm_k_div,
	&pwm_k,
	&pwm_l_sel,
	&pwm_l_div,
	&pwm_l,
	&pwm_m_sel,
	&pwm_m_div,
	&pwm_m,
	&pwm_n_sel,
	&pwm_n_div,
	&pwm_n,
	&c3_spicc_0_sel,
	&c3_spicc_0_div,
	&c3_spicc_0,
	&c3_spicc_1_sel,
	&c3_spicc_1_div,
	&c3_spicc_1,
	&c3_ts_clk_div,
	&c3_ts_clk,
	&c3_eth_rmii_div,
	&c3_eth_rmii,
	&c3_eth_125m,
	&c3_spifc_clk_sel,
	&c3_spifc_clk_div,
	&c3_spifc_clk,
	&c3_vdin_meas_clk_sel,
	&c3_vdin_meas_clk_div,
	&c3_vdin_meas_clk,
	&sd_emmc_a_sel,
	&sd_emmc_a_div,
	&sd_emmc_a,
	&sd_emmc_b_sel,
	&sd_emmc_b_div,
	&sd_emmc_b,
	&nand_sel,
	&nand_div,
	&nand,
	&c3_rtc_xtal_clkin,
	&c3_rtc_32k_div,
	&c3_rtc_32k_div_sel,
	&c3_rtc_32k_sel,
	&c3_rtc_clk,
	&hcodec_a_clk_sel,
	&hcodec_a_clk_div,
	&hcodec_a_clk,
	&hcodec_b_clk_sel,
	&hcodec_b_clk_div,
	&hcodec_b_clk,
	&hcodec_clk,
	&vc9000e_aclk_sel,
	&vc9000e_aclk_div,
	&vc9000e_aclk,
	&vc9000e_core_clk_sel,
	&vc9000e_core_clk_div,
	&vc9000e_core_clk,
	&dewarpa_clk_sel,
	&dewarpa_clk_div,
	&dewarpa_clk,
	&csi_phy0_clk_sel,
	&csi_phy0_clk_div,
	&csi_phy0_clk,
	&isp0_clk_sel,
	&isp0_clk_div,
	&isp0_clk,
	&nna_core_clk_sel,
	&nna_core_clk_div,
	&nna_core_clk,
	&vout_mclk_sel,
	&vout_mclk_div,
	&vout_mclk,
	&vout_enc_clk_sel,
	&vout_enc_clk_div,
	&vout_enc_clk,
	&ge2d_clk_sel,
	&ge2d_clk_div,
	&ge2d_clk,
	&vapb_clk_sel,
	&vapb_clk_div,
	&vapb_clk,
};

/*
 * cpu clock register base is 0xfd000000
 * the clk_regmap init alone
 */
static struct clk_regmap *const c3_cpu_clk_regmaps[] = {
	&c3_cpu_dyn_clk,
	&c3_cpu_clk,
};

/*
 * pll clock register base is 0xfe007c00
 * the clk_regmap init alone
 */
static struct clk_regmap *const c3_pll_clk_regmaps[] = {
	&c3_fixed_pll_vco,
	&c3_fixed_pll,
	&c3_fclk50M,
	&c3_fclk_div2,
	&c3_fclk_div2p5,
	&c3_fclk_div3,
	&c3_fclk_div4,
	&c3_fclk_div5,
	&c3_fclk_div7,
	&c3_gp1_pll_vco,
	&c3_gp1_pll,
	&c3_gp0_pll_vco,
	&c3_gp0_pll,
	&c3_mclk_pll_vco,
	&c3_mclk_pll,
	&c3_mclk_pll_clk,
	&c3_mclk_0_sel,
	&c3_mclk_0_sel_out,
	&c3_mclk_0_div2,
	&c3_mclk_0,
	&c3_mclk_1_sel,
	&c3_mclk_1_sel_out,
	&c3_mclk_1_div2,
	&c3_mclk_1,
	&c3_hifi_pll_vco,
	&c3_hifi_pll,
	&c3_sys_pll_vco,
	&c3_sys_pll,
};

static int meson_c3_dvfs_setup(struct platform_device *pdev)
{
	int ret;

	/* Setup clock notifier for sys_pll */
	ret = clk_notifier_register(c3_sys_pll.hw.clk,
				    &c3_cpu_clk_nb_data.nb);
	if (ret) {
		dev_err(&pdev->dev, "failed to register sys_pll notifier\n");
		return ret;
	}

	return 0;
}

static const struct of_device_id clkc_match_table[] = {
	{ .compatible = "amlogic,c3-clkc" },
	{}
};

static struct regmap_config clkc_regmap_config = {
	.reg_bits       = 32,
	.val_bits       = 32,
	.reg_stride     = 4,
};

static struct regmap *c3_regmap_resource(struct device *dev, char *name)
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

static int c3_clkc_probe(struct platform_device *pdev)
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
	basic_map = c3_regmap_resource(dev, "basic");
	if (IS_ERR(basic_map)) {
		dev_err(dev, "basic clk registers not found\n");
		return PTR_ERR(basic_map);
	}

	pll_map = c3_regmap_resource(dev, "pll");
	if (IS_ERR(pll_map)) {
		dev_err(dev, "pll clk registers not found\n");
		return PTR_ERR(pll_map);
	}

	cpu_clk_map = c3_regmap_resource(dev, "cpu_clk");
	if (IS_ERR(cpu_clk_map)) {
		dev_err(dev, "cpu clk registers not found\n");
		return PTR_ERR(cpu_clk_map);
	}

	/* Populate regmap for the regmap backed clocks */
	for (i = 0; i < ARRAY_SIZE(c3_clk_regmaps); i++)
		c3_clk_regmaps[i]->map = basic_map;

	for (i = 0; i < ARRAY_SIZE(c3_cpu_clk_regmaps); i++)
		c3_cpu_clk_regmaps[i]->map = cpu_clk_map;

	for (i = 0; i < ARRAY_SIZE(c3_pll_clk_regmaps); i++)
		c3_pll_clk_regmaps[i]->map = pll_map;

	for (i = 0; i < c3_hw_onecell_data.num; i++) {
		/* array might be sparse */
		if (!c3_hw_onecell_data.hws[i])
			continue;
		/* dev_err(dev, "register %d  %s\n", i,
		 *	c3_hw_onecell_data.hws[i]->init->name);
		 */

		ret = devm_clk_hw_register(dev, c3_hw_onecell_data.hws[i]);
		if (ret) {
			dev_err(dev, "Clock registration failed\n");
			return ret;
		}

#ifdef CONFIG_AMLOGIC_CLK_DEBUG
		ret = devm_clk_hw_register_clkdev(dev, c3_hw_onecell_data.hws[i],
						  NULL,
						  clk_hw_get_name(c3_hw_onecell_data.hws[i]));
		if (ret < 0) {
			dev_err(dev, "Failed to clkdev register: %d\n", ret);
			return ret;
		}
#endif
	}

	meson_c3_dvfs_setup(pdev);

	return devm_of_clk_add_hw_provider(dev, of_clk_hw_onecell_get,
					   &c3_hw_onecell_data);
}

static struct platform_driver c3_driver = {
	.probe		= c3_clkc_probe,
	.driver		= {
		.name	= "c3-clkc",
		.of_match_table = clkc_match_table,
	},
};

static int c3_clkc_init(void)
{
	return platform_driver_register(&c3_driver);
}
subsys_initcall(c3_clkc_init);

MODULE_LICENSE("GPL v2");
