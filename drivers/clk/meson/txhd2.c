// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/clk-provider.h>
#include <linux/init.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/clkdev.h>
#include "clk-mpll.h"
#include "clk-pll.h"
#include "clk-regmap.h"
#include "clk-cpu-dyndiv.h"
#include "vid-pll-div.h"
#include "clk-dualdiv.h"
#include "txhd2.h"
#include <dt-bindings/clock/txhd2-clkc.h>

static const struct clk_ops meson_pll_clk_no_ops = {};

/*
 * the sys pll DCO value should be 3G~6G,
 * otherwise the sys pll can not lock.
 * od is for 32 bit.
 */

#ifdef CONFIG_ARM
static const struct pll_params_table txhd2_sys_pll_params_table[] = {
	PLL_PARAMS(42, 0, 0), /*DCO=1008M OD=1008M*/
	PLL_PARAMS(50, 0, 0), /*DCO=1200M OD=1200M*/
	PLL_PARAMS(54, 0, 0), /*DCO=1296M OD=1296M*/
	PLL_PARAMS(58, 0, 0), /*DCO=1392M OD=1392M*/
	PLL_PARAMS(59, 0, 0), /*DCO=1416M OD=1416M*/
	PLL_PARAMS(62, 0, 0), /*DCO=1488M OD=1488M*/
	PLL_PARAMS(63, 0, 0), /*DCO=1512M OD=1512M*/
	PLL_PARAMS(67, 0, 0), /*DCO=1608M OD=1608M*/
	{ /* sentinel */ },
};
#else
static const struct pll_params_table txhd2_sys_pll_params_table[] = {
	PLL_PARAMS(42, 0), /*DCO=1008M OD=1008M*/
	PLL_PARAMS(50, 0), /*DCO=1200M OD=1200M*/
	PLL_PARAMS(54, 0), /*DCO=1296M OD=1296M*/
	PLL_PARAMS(58, 0), /*DCO=1392M OD=1392M*/
	PLL_PARAMS(59, 0), /*DCO=1416M OD=1416M*/
	PLL_PARAMS(62, 0), /*DCO=1488M OD=1488M*/
	PLL_PARAMS(63, 0), /*DCO=1512M OD=1512M*/
	PLL_PARAMS(67, 0), /*DCO=1608M OD=1608M*/
	{ /* sentinel */ },
};
#endif

static struct clk_regmap txhd2_sys_pll_dco = {
	.data = &(struct meson_clk_pll_data){
		.en = {
			.reg_off = HHI_SYS_PLL_CNTL0,
			.shift   = 28,
			.width   = 1,
		},
		.m = {
			.reg_off = HHI_SYS_PLL_CNTL0,
			.shift   = 0,
			.width   = 9,
		},
		.n = {
			.reg_off = HHI_SYS_PLL_CNTL0,
			.shift   = 16,
			.width   = 2,
		},
#ifdef CONFIG_ARM
		/* od for 32bit */
		.od = {
			.reg_off = HHI_SYS_PLL_CNTL0,
			.shift	 = 12,
			.width	 = 2,
		},
#endif
		.table = txhd2_sys_pll_params_table,
		.l = {
			.reg_off = HHI_SYS_PLL_CNTL0,
			.shift   = 31,
			.width   = 1,
		},
		.rst = {
			.reg_off = HHI_SYS_PLL_CNTL0,
			.shift   = 29,
			.width   = 1,
		},
		.th = {
			.reg_off = HHI_SYS_PLL_CNTL0,
			.shift   = 24,
			.width   = 1,
		},
		.flags = CLK_MESON_PLL_POWER_OF_TWO
	},
	.hw.init = &(struct clk_init_data){
		.name = "sys_pll_dco",
		.ops = &meson_clk_pll_ops,
		.parent_data = &(const struct clk_parent_data) {
			.fw_name = "xtal",
		},
		.num_parents = 1,
		/*
		 * This clock feeds the CPU, avoid disabling it
		 * Register has the risk of being directly operated
		 */
		.flags = CLK_IS_CRITICAL | CLK_GET_RATE_NOCACHE,
	},
};

#ifdef CONFIG_ARM
/*
 * If DCO frequency is greater than 2.1G in 32bit,it will
 * overflow due to the callback .round_rate returns
 *  long (-2147483648 ~ +2147483647).
 * The OD output value is under 2G, For 32bit, the dco and
 * od should be described together to avoid overflow.
 * Beside, I have tried another methods but failed.
 * 1) change the freq unit to kHZ, it will crash (fixed xtal
 *   = 24000) and it will influences clock users.
 * 2) change the return value for .round_rate, a greater many
 *   code will be modified, related to whole CCF.
 * 3) dco pll using kHZ, other clock using HZ, when calculate pll
 *    it will be a lot of mass because of unit differences.
 *
 * Keep Consistent with 64bit, creat a Virtual clock for sys pll
 */
static struct clk_regmap txhd2_sys_pll = {
	.hw.init = &(struct clk_init_data){
		.name = "sys_pll",
		.ops = &meson_pll_clk_no_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_sys_pll_dco.hw
		},
		.num_parents = 1,
		/*
		 * sys pll is used by cpu clock , it is initialized
		 * to 1200M in bl2, CLK_IGNORE_UNUSED is needed to
		 * prevent the system hang up which will be called
		 * by clk_disable_unused
		 */
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};
#else
static struct clk_regmap txhd2_sys_pll = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_SYS_PLL_CNTL0,
		.shift = 12,
		.width = 2,
		.flags = CLK_DIVIDER_POWER_OF_TWO,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sys_pll",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_sys_pll_dco.hw
		},
		.num_parents = 1,
		/*
		 * sys pll is used for cpu frequency, the parent
		 * rate needs to be modified
		 * Register has the risk of being directly operated
		 */
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};
#endif

static struct clk_regmap txhd2_fixed_pll_dco = {
	.data = &(struct meson_clk_pll_data){
		.en = {
			.reg_off = HHI_FIX_PLL_CNTL0,
			.shift   = 28,
			.width   = 1,
		},
		.m = {
			.reg_off = HHI_FIX_PLL_CNTL0,
			.shift   = 0,
			.width   = 9,
		},
		.n = {
			.reg_off = HHI_FIX_PLL_CNTL0,
			.shift   = 10,
			.width   = 5,
		},
		.l = {
			.reg_off = HHI_FIX_PLL_CNTL0,
			.shift   = 31,
			.width   = 1,
		},
		.rst = {
			.reg_off = HHI_FIX_PLL_CNTL0,
			.shift   = 29,
			.width   = 1,
		},
	},
	.hw.init = &(struct clk_init_data){
		.name = "fixed_pll_dco",
		.ops = &meson_clk_pll_ro_ops,
		.parent_data = &(const struct clk_parent_data) {
			.fw_name = "xtal",
		},
		.num_parents = 1,
		/*
		 * This clock feeds the CPU, avoid disabling it
		 * Register has the risk of being directly operated
		 */
		.flags = CLK_IS_CRITICAL | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap txhd2_fixed_pll = {
	.data = &(struct clk_regmap_div_data) {
		.offset = HHI_FIX_PLL_CNTL0,
		.shift = 16,
		.width = 2,
		.flags = CLK_DIVIDER_POWER_OF_TWO,
	},
	.hw.init = &(struct clk_init_data){
		.name = "fixed_pll",
		.ops = &clk_regmap_divider_ro_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_fixed_pll_dco.hw
		},
		.num_parents = 1,
		/*
		 * This clock won't ever change at runtime so
		 * CLK_SET_RATE_PARENT is not required
		 * Never close , Register may be rewritten
		 */
		.flags = CLK_IS_CRITICAL | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_fixed_factor txhd2_fclk_div2_div = {
	.mult = 1,
	.div = 2,
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div2_div",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) { &txhd2_fixed_pll.hw },
		.num_parents = 1,
	},
};

static struct clk_regmap txhd2_fclk_div2 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_FIX_PLL_CNTL1,
		.bit_idx = 20,
	},
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div2",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_fclk_div2_div.hw
		},
		.num_parents = 1,
		/*
		 * This clock is used by the resident firmware and is required
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

static struct clk_fixed_factor txhd2_fclk_div3_div = {
	.mult = 1,
	.div = 3,
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div3_div",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) { &txhd2_fixed_pll.hw },
		.num_parents = 1,
	},
};

static struct clk_regmap txhd2_fclk_div3 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_FIX_PLL_CNTL1,
		.bit_idx = 16,
	},
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div3",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_fclk_div3_div.hw
		},
		.num_parents = 1,
		/*
		 * This clock is used by the resident firmware and is required
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

static struct clk_fixed_factor txhd2_fclk_div4_div = {
	.mult = 1,
	.div = 4,
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div4_div",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) { &txhd2_fixed_pll.hw },
		.num_parents = 1,
	},
};

static struct clk_regmap txhd2_fclk_div4 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_FIX_PLL_CNTL1,
		.bit_idx = 17,
	},
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div4",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_fclk_div4_div.hw
		},
		.num_parents = 1,
		/*
		 * This clock is used by the resident firmware and is required
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

static struct clk_fixed_factor txhd2_fclk_div5_div = {
	.mult = 1,
	.div = 5,
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div5_div",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) { &txhd2_fixed_pll.hw },
		.num_parents = 1,
	},
};

static struct clk_regmap txhd2_fclk_div5 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_FIX_PLL_CNTL1,
		.bit_idx = 18,
	},
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div5",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_fclk_div5_div.hw
		},
		.num_parents = 1,
		/*
		 * This clock is used by the resident firmware and is required
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

static struct clk_fixed_factor txhd2_fclk_div7_div = {
	.mult = 1,
	.div = 7,
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div7_div",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) { &txhd2_fixed_pll.hw },
		.num_parents = 1,
	},
};

static struct clk_regmap txhd2_fclk_div7 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_FIX_PLL_CNTL1,
		.bit_idx = 19,
	},
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div7",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_fclk_div7_div.hw
		},
		.num_parents = 1,
		/*
		 * This clock is used by the resident firmware and is required
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

static struct clk_fixed_factor txhd2_fclk_div2p5_div = {
	.mult = 2,
	.div = 5,
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div2p5_div",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_fixed_pll.hw
		},
		.num_parents = 1,
	},
};

static struct clk_regmap txhd2_fclk_div2p5 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_FIX_PLL_CNTL1,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div2p5",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_fclk_div2p5_div.hw
		},
		.num_parents = 1,
		/*
		 * This clock is used by the resident firmware and is required
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

static struct clk_fixed_factor txhd2_mpll_50m_div = {
	.mult = 1,
	.div = 80,
	.hw.init = &(struct clk_init_data){
		.name = "mpll_50m_div",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_fixed_pll_dco.hw
		},
		.num_parents = 1,
	},
};

static const struct clk_parent_data txhd2_mpll_50m_sel[] = {
	{ .fw_name = "xtal", },
	{ .hw = &txhd2_mpll_50m_div.hw }
};

static struct clk_regmap txhd2_mpll_50m = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_FIX_PLL_CNTL1,
		.mask = 0x1,
		.shift = 4,
	},
	.hw.init = &(struct clk_init_data){
		.name = "mpll_50m",
		.ops = &clk_regmap_mux_ro_ops,
		.parent_data = txhd2_mpll_50m_sel,
		.num_parents = ARRAY_SIZE(txhd2_mpll_50m_sel),
	},
};

#ifdef CONFIG_ARM
static const struct pll_params_table txhd2_gp0_pll_table[] = {
	PLL_PARAMS(48, 0, 2), /* DCO = 1152M OD = 2 PLL = 288M */
	PLL_PARAMS(62, 0, 1), /* DCO = 1488M OD = 1 PLL = 744M */
	PLL_PARAMS(48, 0, 0), /* DCO = 1152M OD = 0 PLL = 1152M */
	PLL_PARAMS(62, 0, 0), /* DCO = 1488M OD = 0 PLL = 1488M */

	{ /* sentinel */  }
};
#else
static const struct pll_params_table txhd2_gp0_pll_table[] = {
	PLL_PARAMS(35, 0), /* DCO = 840M OD = 0 PLL = 840M */
	PLL_PARAMS(33, 0), /* DCO = 792M OD = 0 PLL = 792M */
	PLL_PARAMS(31, 0), /* DCO = 744M OD = 0 PLL = 744M */
	PLL_PARAMS(32, 0), /* DCO = 768M OD = 0 PLL = 768M */
	PLL_PARAMS(48, 0), /* DCO = 1152M OD = 0 PLL = 1152M */
	{ /* sentinel */  }
};
#endif

/*
 * Internal gp0 pll emulation configuration parameters
 */
static const struct reg_sequence txhd2_gp0_init_regs[] = {
	{ .reg = HHI_GP0_PLL_CNTL0,	.def = 0x60000030 },
	{ .reg = HHI_GP0_PLL_CNTL0,	.def = 0x70000030 },
	{ .reg = HHI_GP0_PLL_CNTL1,	.def = 0x39002000 },
	{ .reg = HHI_GP0_PLL_CNTL2,	.def = 0x00001140 },
	{ .reg = HHI_GP0_PLL_CNTL3,	.def = 0x00000000, .delay_us = 20 },
	{ .reg = HHI_GP0_PLL_CNTL0,	.def = 0x50000030, .delay_us = 20 },
	{ .reg = HHI_GP0_PLL_CNTL2,	.def = 0x00001100 }
};

static struct clk_regmap txhd2_gp0_pll_dco = {
	.data = &(struct meson_clk_pll_data){
		.en = {
			.reg_off = HHI_GP0_PLL_CNTL0,
			.shift   = 28,
			.width   = 1,
		},
		.m = {
			.reg_off = HHI_GP0_PLL_CNTL0,
			.shift   = 0,
			.width   = 9,
		},
		.n = {
			.reg_off = HHI_GP0_PLL_CNTL0,
			.shift   = 16,
			.width   = 2,
		},
#ifdef CONFIG_ARM
		/* for 32bit */
		.od = {
			.reg_off = HHI_GP0_PLL_CNTL0,
			.shift	 = 12,
			.width	 = 2,
		},
#endif
		.l = {
			.reg_off = HHI_GP0_PLL_CNTL0,
			.shift   = 31,
			.width   = 1,
		},
		.rst = {
			.reg_off = HHI_GP0_PLL_CNTL0,
			.shift   = 29,
			.width   = 1,
		},
		.th = {
			.reg_off = HHI_GP0_PLL_CNTL0,
			.shift   = 24,
			.width   = 1,
		},
		.table = txhd2_gp0_pll_table,
		.init_regs = txhd2_gp0_init_regs,
		.init_count = ARRAY_SIZE(txhd2_gp0_init_regs),
		.flags = CLK_MESON_PLL_POWER_OF_TWO
	},
	.hw.init = &(struct clk_init_data){
		.name = "gp0_pll_dco",
		.ops = &meson_clk_pll_ops,
		.parent_data = &(const struct clk_parent_data) {
			.fw_name = "xtal",
		},
		.num_parents = 1,
		/*
		 * Register has the risk of being directly operated
		 */
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

#ifdef CONFIG_ARM
static struct clk_regmap txhd2_gp0_pll = {
	.hw.init = &(struct clk_init_data){
		.name = "gp0_pll",
		.ops = &meson_pll_clk_no_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_gp0_pll_dco.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};
#else
static struct clk_regmap txhd2_gp0_pll = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_GP0_PLL_CNTL0,
		.shift = 12,
		.width = 3,
		.flags = (CLK_DIVIDER_POWER_OF_TWO |
			  CLK_DIVIDER_ROUND_CLOSEST),
	},
	.hw.init = &(struct clk_init_data){
		.name = "gp0_pll",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_gp0_pll_dco.hw
		},
		.num_parents = 1,
		/*
		 * gpo pll is directly used in other modules, and the
		 * parent rate needs to be modified
		 * Register has the risk of being directly operated.
		 */
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};
#endif

#ifdef CONFIG_ARM
static const struct pll_params_table txhd2_hifi_pll_table[] = {
	PLL_PARAMS(36, 3, 0), /* DCO = 1806.336M */
	{ /* sentinel */  }
};
#else
static const struct pll_params_table txhd2_hifi_pll_table[] = {
	PLL_PARAMS(36, 3),
	{ /* sentinel */  }
};
#endif

/*
 * Internal hifi pll emulation configuration parameters
 */
static const struct reg_sequence txhd2_hifi_init_regs[] = {
	{ .reg = HHI_HIFI_PLL_CNTL0,	.def = 0x61030024 },
	{ .reg = HHI_HIFI_PLL_CNTL0,	.def = 0x71030024 },
	{ .reg = HHI_HIFI_PLL_CNTL1,	.def = 0x25002000 },
	{ .reg = HHI_HIFI_PLL_CNTL2,	.def = 0x09001140 },
	{ .reg = HHI_HIFI_PLL_CNTL3,	.def = 0x00083180, .delay_us = 20 },
	{ .reg = HHI_HIFI_PLL_CNTL0,	.def = 0x51030024, .delay_us = 20 },
	{ .reg = HHI_HIFI_PLL_CNTL2,	.def = 0x09001100 },
};

static struct clk_regmap txhd2_hifi_pll_dco = {
	.data = &(struct meson_clk_pll_data){
		.en = {
			.reg_off = HHI_HIFI_PLL_CNTL0,
			.shift   = 28,
			.width   = 1,
		},
		.m = {
			.reg_off = HHI_HIFI_PLL_CNTL0,
			.shift   = 0,
			.width   = 9,
		},
		.n = {
			.reg_off = HHI_HIFI_PLL_CNTL0,
			.shift   = 16,
			.width   = 2,
		},
#ifdef CONFIG_ARM
		/* od for 32bit */
		.od = {
			.reg_off = HHI_HIFI_PLL_CNTL0,
			.shift	 = 12,
			.width	 = 2,
		},
#endif
		.frac_hifi = {
			.reg_off = HHI_HIFI_PLL_CNTL3,
			.shift   = 0,
			.width   = 19,
		},
		.l = {
			.reg_off = HHI_HIFI_PLL_CNTL0,
			.shift   = 31,
			.width   = 1,
		},
		.rst = {
			.reg_off = HHI_HIFI_PLL_CNTL0,
			.shift   = 29,
			.width   = 1,
		},
		.th = {
			.reg_off = HHI_HIFI_PLL_CNTL0,
			.shift   = 24,
			.width   = 1,
		},
		.table = txhd2_hifi_pll_table,
		.init_regs = txhd2_hifi_init_regs,
		.init_count = ARRAY_SIZE(txhd2_hifi_init_regs),
		.flags = CLK_MESON_PLL_ROUND_CLOSEST | CLK_MESON_PLL_FIXED_FRAC_WEIGHT_PRECISION |
			CLK_MESON_PLL_POWER_OF_TWO
	},
	.hw.init = &(struct clk_init_data){
		.name = "hifi_pll_dco",
		.ops = &meson_clk_pll_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_fclk_div5.hw
		},
		.num_parents = 1,
		/*
		 * Register has the risk of being directly operated
		 */
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

#ifdef CONFIG_ARM
static struct clk_regmap txhd2_hifi_pll = {
	.hw.init = &(struct clk_init_data){
		.name = "hifi_pll",
		.ops = &meson_pll_clk_no_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_hifi_pll_dco.hw
		},
		.num_parents = 1,
		/*
		 * sys pll is used by cpu clock , it is initialized
		 * to 1200M in bl2, CLK_IGNORE_UNUSED is needed to
		 * prevent the system hang up which will be called
		 * by clk_disable_unused
		 */
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};
#else
static struct clk_regmap txhd2_hifi_pll = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_HIFI_PLL_CNTL0,
		.shift = 12,
		.width = 2,
		.flags = (CLK_DIVIDER_POWER_OF_TWO |
			  CLK_DIVIDER_ROUND_CLOSEST),
	},
	.hw.init = &(struct clk_init_data){
		.name = "hifi_pll",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_hifi_pll_dco.hw
		},
		.num_parents = 1,
		/*
		 * hifi pll is directly used in other modules, and the
		 * parent rate needs to be modified
		 * Register has the risk of being directly operated.
		 */
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};
#endif

static const struct cpu_dyn_table txhd2_cpu_dyn_table[] = {
	CPU_LOW_PARAMS(100000000, 1, 1, 9),
	CPU_LOW_PARAMS(250000000, 1, 1, 3),
	//CPU_LOW_PARAMS(333333333, 2, 1, 1),
	CPU_LOW_PARAMS(500000000, 1, 1, 1),
	//CPU_LOW_PARAMS(667000000, 2, 0, 0),
	CPU_LOW_PARAMS(1000000000, 1, 0, 0),
};

static const struct clk_parent_data txhd2_cpu_dyn_clk_sel[] = {
	{ .fw_name = "xtal", },
	{ .hw = &txhd2_fclk_div2.hw },
	{ .hw = &txhd2_fclk_div3.hw },
};

static struct clk_regmap txhd2_cpu_dyn_clk = {
	.data = &(struct meson_clk_cpu_dyn_data){
		.table = txhd2_cpu_dyn_table,
		.table_cnt = ARRAY_SIZE(txhd2_cpu_dyn_table),
		.offset = HHI_SYS_CPU_CLK_CNTL,
	},
	.hw.init = &(struct clk_init_data){
		.name = "cpu_dyn_clk",
		.ops = &meson_clk_cpu_dyn_ops,
		.parent_data = txhd2_cpu_dyn_clk_sel,
		.num_parents = ARRAY_SIZE(txhd2_cpu_dyn_clk_sel),
	},
};

static struct clk_regmap txhd2_cpu_clk = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_SYS_CPU_CLK_CNTL,
		.mask = 0x1,
		.shift = 11,
		.flags = CLK_MUX_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "cpu_clk",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_cpu_dyn_clk.hw,
			&txhd2_sys_pll.hw,
		},
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

struct txhd2_sys_pll_nb_data {
	struct notifier_block nb;
	struct clk_hw *sys_pll;
	struct clk_hw *cpu_clk;
	struct clk_hw *cpu_dyn_clk;
};

static int txhd2_sys_pll_notifier_cb(struct notifier_block *nb,
				   unsigned long event, void *data)
{
	struct txhd2_sys_pll_nb_data *nb_data =
		container_of(nb, struct txhd2_sys_pll_nb_data, nb);

	switch (event) {
	case PRE_RATE_CHANGE:
		/*
		 * This notifier means sys_pll clock will be changed
		 * to feed cpu_clk, this the current path :
		 * cpu_clk
		 *    \- sys_pll
		 *          \- sys_pll_dco
		 */

		/* Configure cpu_clk to use cpu_clk_dyn */
		clk_hw_set_parent(nb_data->cpu_clk,
				  nb_data->cpu_dyn_clk);

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

		udelay(5);

		return NOTIFY_OK;

	case POST_RATE_CHANGE:
		/*
		 * The sys_pll has ben updated, now switch back cpu_clk to
		 * sys_pll
		 */

		/* Configure cpu_clk to use sys_pll */
		clk_hw_set_parent(nb_data->cpu_clk,
				  nb_data->sys_pll);

		udelay(5);

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

static struct txhd2_sys_pll_nb_data txhd2_sys_pll_nb_data = {
	.sys_pll = &txhd2_sys_pll.hw,
	.cpu_clk = &txhd2_cpu_clk.hw,
	.cpu_dyn_clk = &txhd2_cpu_dyn_clk.hw,
	.nb.notifier_call = txhd2_sys_pll_notifier_cb,
};

/*peripheral bus*/
static u32 mux_table_clk81[] = { 2, 5, 6, 7};

static const struct clk_hw *clk81_parent_hws[] = {
	&txhd2_fclk_div7.hw,
	&txhd2_fclk_div4.hw,
	&txhd2_fclk_div3.hw,
	&txhd2_fclk_div5.hw,
};

static struct clk_regmap txhd2_mpeg_clk_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_MPEG_CLK_CNTL,
		.mask = 0x7,
		.shift = 12,
		.table = mux_table_clk81,
	},
	.hw.init = &(struct clk_init_data){
		.name = "mpeg_clk_sel",
		.ops = &clk_regmap_mux_ro_ops,
		.parent_hws = clk81_parent_hws,
		.num_parents = ARRAY_SIZE(clk81_parent_hws),
	},
};

static struct clk_regmap txhd2_mpeg_clk_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_MPEG_CLK_CNTL,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "mpeg_clk_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_mpeg_clk_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap txhd2_clk81 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_MPEG_CLK_CNTL,
		.bit_idx = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "clk81",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_mpeg_clk_div.hw
		},
		.num_parents = 1,
		.flags = (CLK_SET_RATE_PARENT | CLK_IS_CRITICAL),
	},
};

static u32 mux_table_vdec_sel[] = {0, 1, 2, 3, 4, 5, 7};

/* cts_vdec_clk */
static const struct clk_parent_data txhd2_dec_parent_hws[] = {
	{ .hw = &txhd2_fclk_div2p5.hw },
	{ .hw = &txhd2_fclk_div3.hw },
	{ .hw = &txhd2_fclk_div4.hw },
	{ .hw = &txhd2_fclk_div5.hw },
	{ .hw = &txhd2_fclk_div7.hw },
	{ .hw = &txhd2_hifi_pll.hw },
	// { .hw = &txhd2_gp0_pll.hw },
	{ .fw_name = "xtal", }
};

static struct clk_regmap txhd2_vdec_0_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_VDEC_CLK_CNTL,
		.mask = 0x7,
		.shift = 9,
		.table = mux_table_vdec_sel,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vdec_0_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = txhd2_dec_parent_hws,
		.num_parents = ARRAY_SIZE(txhd2_dec_parent_hws),
	},
};

static struct clk_regmap txhd2_vdec_0_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_VDEC_CLK_CNTL,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vdec_0_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_vdec_0_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap txhd2_vdec_0 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_VDEC_CLK_CNTL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vdec_0",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_vdec_0_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap txhd2_vdec_1_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_VDEC3_CLK_CNTL,
		.mask = 0x7,
		.shift = 9,
		.table = mux_table_vdec_sel,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vdec_1_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = txhd2_dec_parent_hws,
		.num_parents = ARRAY_SIZE(txhd2_dec_parent_hws),
	},
};

static struct clk_regmap txhd2_vdec_1_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_VDEC3_CLK_CNTL,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vdec_1_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_vdec_1_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap txhd2_vdec_1 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_VDEC3_CLK_CNTL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vdec_1",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_vdec_1_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap txhd2_vdec = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_VDEC3_CLK_CNTL,
		.mask = 0x1,
		.shift = 15,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vdec",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_vdec_0.hw,
			&txhd2_vdec_1.hw
		},
		.num_parents = 2,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap txhd2_hevcf_0_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_VDEC2_CLK_CNTL,
		.mask = 0x7,
		.shift = 9,
		.table = mux_table_vdec_sel,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hevcf_0_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = txhd2_dec_parent_hws,
		.num_parents = ARRAY_SIZE(txhd2_dec_parent_hws),
	},
};

static struct clk_regmap txhd2_hevcf_0_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_VDEC2_CLK_CNTL,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hevcf_0_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_hevcf_0_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap txhd2_hevcf_0 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_VDEC2_CLK_CNTL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hevcf_0",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_hevcf_0_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap txhd2_hevcf_1_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_VDEC4_CLK_CNTL,
		.mask = 0x7,
		.shift = 9,
		.table = mux_table_vdec_sel,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hevcf_1_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = txhd2_dec_parent_hws,
		.num_parents = ARRAY_SIZE(txhd2_dec_parent_hws),
	},
};

static struct clk_regmap txhd2_hevcf_1_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_VDEC4_CLK_CNTL,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hevcf_1_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_hevcf_1_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap txhd2_hevcf_1 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_VDEC4_CLK_CNTL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hevcf_1",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_hevcf_1_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap txhd2_hevcf = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_VDEC4_CLK_CNTL,
		.mask = 0x1,
		.shift = 15,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hevcf",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_hevcf_0.hw,
			&txhd2_hevcf_1.hw
		},
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static u32 mux_table_ge2d_sel[] = {0, 1, 2, 3, 7};

static const struct clk_hw *txhd2_ge2d_parent_hws[] = {
	&txhd2_fclk_div4.hw,
	&txhd2_fclk_div3.hw,
	&txhd2_fclk_div5.hw,
	&txhd2_fclk_div7.hw,
	&txhd2_fclk_div2p5.hw
};

static struct clk_regmap txhd2_vapb_0_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_VAPBCLK_CNTL,
		.mask = 0x7,
		.shift = 9,
		.table = mux_table_ge2d_sel,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vapb_0_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = txhd2_ge2d_parent_hws,
		.num_parents = ARRAY_SIZE(txhd2_ge2d_parent_hws),
	},
};

static struct clk_regmap txhd2_vapb_0_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_VAPBCLK_CNTL,
		.shift = 0,
		.width = 7,
		.flags = CLK_DIVIDER_ALLOW_ZERO
	},
	.hw.init = &(struct clk_init_data){
		.name = "vapb_0_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_vapb_0_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap txhd2_vapb_0 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_VAPBCLK_CNTL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vapb_0",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_vapb_0_div.hw
		},
		.num_parents = 1,
		/*
		 * vapb clk is used for display module, vpu driver is a KO,
		 * It is too late to enable to clk again.
		 * add CLK_IGNORE_UNUSED to avoid display abnormal
		 */
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED
	},
};

static struct clk_regmap txhd2_vapb_1_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_VAPBCLK_CNTL,
		.mask = 0x7,
		.shift = 25,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vapb_1_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = txhd2_ge2d_parent_hws,
		.num_parents = ARRAY_SIZE(txhd2_ge2d_parent_hws),
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap txhd2_vapb_1_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_VAPBCLK_CNTL,
		.shift = 16,
		.width = 7,
		.flags = CLK_DIVIDER_ALLOW_ZERO
	},
	.hw.init = &(struct clk_init_data){
		.name = "vapb_1_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_vapb_1_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap txhd2_vapb_1 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_VAPBCLK_CNTL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vapb_1",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_vapb_1_div.hw
		},
		.num_parents = 1,
		/*
		 * vapb clk is used for display module, vpu driver is a KO,
		 * It is too late to enable to clk again.
		 * add CLK_IGNORE_UNUSED to avoid display abnormal
		 */
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED
	},
};

static struct clk_regmap txhd2_vapb = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_VAPBCLK_CNTL,
		.mask = 0x1,
		.shift = 31,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vapb_mux",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_vapb_0.hw,
			&txhd2_vapb_1.hw
		},
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap txhd2_ge2d = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_VAPBCLK_CNTL,
		.bit_idx = 30,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "ge2d_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_vapb.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_GATE | CLK_SET_RATE_PARENT,
	},
};

/*mali_clk*/
/*
 * The MALI IP is clocked by two identical clocks (mali_0 and mali_1)
 * muxed by a glitch-free switch on Meson8b and Meson8m2 and later.
 *
 * CLK_SET_RATE_PARENT is added for mali_0_sel clock
 * 1.gp0 pll only support the 846M, avoid other rate 500/400M from it
 * 2.hifi pll is used for other module, skip it, avoid some rate from it
 */
static u32 mux_table_mali[] = { 0, 1, 3, 4, 5, 6, 7};

static const struct clk_parent_data txhd2_mali_0_1_parent_data[] = {
	{ .fw_name = "xtal", },
	{ .hw = &txhd2_gp0_pll.hw },
	{ .hw = &txhd2_fclk_div2p5.hw },
	{ .hw = &txhd2_fclk_div3.hw },
	{ .hw = &txhd2_fclk_div4.hw },
	{ .hw = &txhd2_fclk_div5.hw },
	{ .hw = &txhd2_fclk_div7.hw },
};

static struct clk_regmap txhd2_mali_0_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_MALI_CLK_CNTL,
		.mask = 0x7,
		.shift = 9,
		.table = mux_table_mali,
	},
	.hw.init = &(struct clk_init_data){
		.name = "mali_0_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = txhd2_mali_0_1_parent_data,
		.num_parents = ARRAY_SIZE(txhd2_mali_0_1_parent_data),
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap txhd2_mali_0_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_MALI_CLK_CNTL,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "mali_0_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_mali_0_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap txhd2_mali_0 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_MALI_CLK_CNTL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "mali_0",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_mali_0_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_GATE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap txhd2_mali_1_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_MALI_CLK_CNTL,
		.mask = 0x7,
		.shift = 25,
		.table = mux_table_mali,
	},
	.hw.init = &(struct clk_init_data){
		.name = "mali_1_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = txhd2_mali_0_1_parent_data,
		.num_parents = ARRAY_SIZE(txhd2_mali_0_1_parent_data),
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap txhd2_mali_1_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_MALI_CLK_CNTL,
		.shift = 16,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "mali_1_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_mali_1_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap txhd2_mali_1 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_MALI_CLK_CNTL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data){
		.name = "mali_1",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_mali_1_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_GATE | CLK_SET_RATE_PARENT,
	},
};

static const struct clk_hw *txhd2_mali_parent_hws[] = {
	&txhd2_mali_0.hw,
	&txhd2_mali_1.hw
};

static struct clk_regmap txhd2_mali = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_MALI_CLK_CNTL,
		.mask = 1,
		.shift = 31,
	},
	.hw.init = &(struct clk_init_data){
		.name = "mali",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = txhd2_mali_parent_hws,
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

/*ts clk*/
static struct clk_regmap txhd2_ts_clk_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_TS_CLK_CNTL,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "ts_clk_div",
		.ops = &clk_regmap_divider_ops,
		.parent_data = &(const struct clk_parent_data) {
			.fw_name = "xtal",
		},
		.num_parents = 1,
	},
};

static struct clk_regmap txhd2_ts_clk = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_TS_CLK_CNTL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "ts_clk",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_ts_clk_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static const struct clk_parent_data txhd2_sd_emmc_parent_data[] = {
	{ .fw_name = "xtal", },
	{ .hw = &txhd2_fclk_div2.hw },
	{ .hw = &txhd2_fclk_div3.hw },
	{ .hw = &txhd2_fclk_div5.hw },
	{ .hw = &txhd2_fclk_div7.hw },
	// { .hw = &txhd2_gp0_pll.hw }
};

static struct clk_regmap txhd2_sd_emmc_c_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_NAND_CLK_CNTL,
		.mask = 0x7,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "sd_emmc_c_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = txhd2_sd_emmc_parent_data,
		.num_parents = ARRAY_SIZE(txhd2_sd_emmc_parent_data),
		.flags = CLK_GET_RATE_NOCACHE
	},
};

static struct clk_regmap txhd2_sd_emmc_c_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_NAND_CLK_CNTL,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "sd_emmc_c_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_sd_emmc_c_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE//| CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap txhd2_sd_emmc_c = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_NAND_CLK_CNTL,
		.bit_idx = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sd_emmc_c",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_sd_emmc_c_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE// | CLK_SET_RATE_PARENT
	},
};

/* cts_vdin_meas_clk */
static u32 txhd2_vdin_meas_table[] = {0, 1, 2, 3};
static const struct clk_parent_data txhd2_vdin_parent_hws[] = {
	{ .fw_name = "xtal", },
	{ .hw = &txhd2_fclk_div4.hw },
	{ .hw = &txhd2_fclk_div3.hw },
	{ .hw = &txhd2_fclk_div5.hw },
};

static struct clk_regmap txhd2_vdin_meas_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_VDIN_MEAS_CLK_CNTL,
		.mask = 0x7,
		.shift = 9,
		.table = txhd2_vdin_meas_table,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vdin_meas_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = txhd2_vdin_parent_hws,
		.num_parents = ARRAY_SIZE(txhd2_vdin_parent_hws),
	},
};

static struct clk_regmap txhd2_vdin_meas_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_VDIN_MEAS_CLK_CNTL,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vdin_meas_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_vdin_meas_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap txhd2_vdin_meas = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_VDIN_MEAS_CLK_CNTL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vdin_meas",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_vdin_meas_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap txhd2_vid_lock_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_VID_LOCK_CLK_CNTL,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vid_lock_div",
		.ops = &clk_regmap_divider_ops,
		.parent_data = &(const struct clk_parent_data) {
			.fw_name = "xtal",
		},
		.num_parents = 1,
	},
};

static struct clk_regmap txhd2_vid_lock_clk  = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_VID_LOCK_CLK_CNTL,
		.bit_idx = 7,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vid_lock_clk",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_vid_lock_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static const struct clk_hw *txhd2_vpu_parent_hws[] = {
	&txhd2_fclk_div3.hw,
	&txhd2_fclk_div4.hw,
	&txhd2_fclk_div5.hw,
	&txhd2_fclk_div7.hw
};

static struct clk_regmap txhd2_vpu_0_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_VPU_CLK_CNTL,
		.mask = 0x3,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vpu_0_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = txhd2_vpu_parent_hws,
		.num_parents = ARRAY_SIZE(txhd2_vpu_parent_hws),
	},
};

static struct clk_regmap txhd2_vpu_0_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_VPU_CLK_CNTL,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vpu_0_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) { &txhd2_vpu_0_sel.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap txhd2_vpu_0 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_VPU_CLK_CNTL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vpu_0",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) { &txhd2_vpu_0_div.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap txhd2_vpu_1_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_VPU_CLK_CNTL,
		.mask = 0x3,
		.shift = 25,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vpu_1_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = txhd2_vpu_parent_hws,
		.num_parents = ARRAY_SIZE(txhd2_vpu_parent_hws),
	},
};

static struct clk_regmap txhd2_vpu_1_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_VPU_CLK_CNTL,
		.shift = 16,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vpu_1_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) { &txhd2_vpu_1_sel.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap txhd2_vpu_1 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_VPU_CLK_CNTL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vpu_1",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) { &txhd2_vpu_1_div.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap txhd2_vpu = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_VPU_CLK_CNTL,
		.mask = 1,
		.shift = 31,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vpu",
		.ops = &clk_regmap_mux_ops,
		/*
		 * bit 31 selects from 2 possible parents:
		 * vpu_0 or vpu_1
		 */
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_vpu_0.hw,
			&txhd2_vpu_1.hw
		},
		.num_parents = 2,
		.flags = CLK_SET_RATE_NO_REPARENT,
	},
};

static const struct clk_hw *vpu_clkb_tmp_parent_hws[] = {
	&txhd2_vpu.hw,
	&txhd2_fclk_div4.hw,
	&txhd2_fclk_div5.hw,
	&txhd2_fclk_div7.hw
};

static struct clk_regmap txhd2_vpu_clkb_tmp_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_VPU_CLKB_CNTL,
		.mask = 0x3,
		.shift = 20,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vpu_clkb_tmp_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = vpu_clkb_tmp_parent_hws,
		.num_parents = ARRAY_SIZE(vpu_clkb_tmp_parent_hws),
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap txhd2_vpu_clkb_tmp_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_VPU_CLKB_CNTL,
		.shift = 16,
		.width = 4,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vpu_clkb_tmp_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_vpu_clkb_tmp_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap txhd2_vpu_clkb_tmp = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_VPU_CLKB_CNTL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vpu_clkb_tmp",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_vpu_clkb_tmp_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap txhd2_vpu_clkb_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_VPU_CLKB_CNTL,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vpu_clkb_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_vpu_clkb_tmp.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap txhd2_vpu_clkb = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_VPU_CLKB_CNTL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vpu_clkb",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_vpu_clkb_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static const struct clk_hw *vpu_clkc_parent_hws[] = {
	&txhd2_fclk_div4.hw,
	&txhd2_fclk_div3.hw,
	&txhd2_fclk_div5.hw,
	&txhd2_fclk_div7.hw
};

static struct clk_regmap txhd2_vpu_clkc_0_sel  = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_VPU_CLKC_CNTL,
		.mask = 0x3,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vpu_clkc_0_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = vpu_clkc_parent_hws,
		.num_parents = ARRAY_SIZE(vpu_clkc_parent_hws),
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap txhd2_vpu_clkc_0_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_VPU_CLKC_CNTL,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vpu_clkc_0_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_vpu_clkc_0_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap txhd2_vpu_clkc_0 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_VPU_CLKC_CNTL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vpu_clkc_0",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_vpu_clkc_0_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap txhd2_vpu_clkc_1_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_VPU_CLKC_CNTL,
		.mask = 0x3,
		.shift = 25,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vpu_clkc_1_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = vpu_clkc_parent_hws,
		.num_parents = ARRAY_SIZE(vpu_clkc_parent_hws),
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap txhd2_vpu_clkc_1_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_VPU_CLKC_CNTL,
		.shift = 16,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vpu_clkc_1_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_vpu_clkc_1_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap txhd2_vpu_clkc_1 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_VPU_CLKC_CNTL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vpu_clkc_1",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_vpu_clkc_1_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap txhd2_vpu_clkc = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_VPU_CLKC_CNTL,
		.mask = 0x1,
		.shift = 31,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vpu_clkc",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_vpu_clkc_0.hw,
			&txhd2_vpu_clkc_1.hw
		},
		.num_parents = 2,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

/*cts tcon pll clk*/
static const struct clk_parent_data txhd2_tcon_pll_parent_data[] = {
	{ .fw_name = "xtal", },
	{ .hw = &txhd2_fclk_div5.hw },
	{ .hw = &txhd2_fclk_div4.hw },
	{ .hw = &txhd2_fclk_div3.hw },
};

static struct clk_regmap txhd2_cts_tcon_pll_clk_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_TCON_CLK_CNTL,
		.mask = 0x7,
		.shift = 7,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "cts_tcon_pll_clk_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = txhd2_tcon_pll_parent_data,
		.num_parents = ARRAY_SIZE(txhd2_tcon_pll_parent_data),
	},
};

static struct clk_regmap txhd2_cts_tcon_pll_clk_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_TCON_CLK_CNTL,
		.shift = 0,
		.width = 6,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "cts_tcon_pll_clk_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_cts_tcon_pll_clk_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap txhd2_cts_tcon_pll_clk = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_TCON_CLK_CNTL,
		.bit_idx = 6,
	},
	.hw.init = &(struct clk_init_data){
		.name = "cts_tcon_pll_clk",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_cts_tcon_pll_clk_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

/*adc extclk in clkl*/
static u32 mux_table_adc_ext_sel[] = {0, 1, 2, 3, 4, 6};
static const struct clk_parent_data txhd2_adc_extclk_parent_data[] = {
	{ .fw_name = "xtal", },
	{ .hw = &txhd2_fclk_div4.hw },
	{ .hw = &txhd2_fclk_div3.hw },
	{ .hw = &txhd2_fclk_div5.hw },
	{ .hw = &txhd2_fclk_div7.hw },
	// { .hw = &txhd2_gp0_pll.hw },
	{ .hw = &txhd2_hifi_pll.hw }
};

static struct clk_regmap txhd2_adc_extclk_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_DEMOD_CLK_CNTL,
		.mask = 0x7,
		.shift = 25,
		.table = mux_table_adc_ext_sel,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "adc_extclk_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = txhd2_adc_extclk_parent_data,
		.num_parents = ARRAY_SIZE(txhd2_adc_extclk_parent_data),
	},
};

static struct clk_regmap txhd2_adc_extclk_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_DEMOD_CLK_CNTL,
		.shift = 16,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "adc_extclk_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_adc_extclk_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap txhd2_adc_extclk = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_DEMOD_CLK_CNTL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data){
		.name = "adc_extclk",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_adc_extclk_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

/*cts demod core*/
static const struct clk_parent_data txhd2_demod_parent_data[] = {
	{ .fw_name = "xtal", },
	{ .hw = &txhd2_fclk_div7.hw },
	{ .hw = &txhd2_fclk_div4.hw },
	//todo: need add adc_dpll_intclk
};

static struct clk_regmap txhd2_cts_demod_core_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_DEMOD_CLK_CNTL,
		.mask = 0x3,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "cts_demod_core_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = txhd2_demod_parent_data,
		.num_parents = ARRAY_SIZE(txhd2_demod_parent_data),
	},
};

static struct clk_regmap txhd2_cts_demod_core_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_DEMOD_CLK_CNTL,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "cts_demod_core_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_cts_demod_core_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap txhd2_cts_demod_core = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_DEMOD_CLK_CNTL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "cts_demod_core",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_cts_demod_core_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap txhd2_demod_32k_clkin = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_DEMOD_32K_CNTL0,
		.bit_idx = 31,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "demod_32k_clkin",
		.ops = &clk_regmap_gate_ops,
		.parent_data = &(const struct clk_parent_data) {
			.fw_name = "xtal",
		},
		.num_parents = 1,
	},
};

static const struct meson_clk_dualdiv_param txhd2_demod_32k_div_table[] = {
	{
		.dual	= 0,
		.n1	= 733,
	},
	{}
};

static struct clk_regmap txhd2_demod_32k_div = {
	.data = &(struct meson_clk_dualdiv_data){
		.n1 = {
			.reg_off = HHI_DEMOD_32K_CNTL0,
			.shift   = 0,
			.width   = 12,
		},
		.n2 = {
			.reg_off = HHI_DEMOD_32K_CNTL0,
			.shift   = 12,
			.width   = 12,
		},
		.m1 = {
			.reg_off = HHI_DEMOD_32K_CNTL1,
			.shift   = 0,
			.width   = 12,
		},
		.m2 = {
			.reg_off = HHI_DEMOD_32K_CNTL1,
			.shift   = 12,
			.width   = 12,
		},
		.dual = {
			.reg_off = HHI_DEMOD_32K_CNTL0,
			.shift   = 28,
			.width   = 1,
		},
		.table = txhd2_demod_32k_div_table,
	},
	.hw.init = &(struct clk_init_data){
		.name = "demod_32k_div",
		.ops = &meson_clk_dualdiv_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_demod_32k_clkin.hw
		},
		.num_parents = 1,
	},
};

static struct clk_regmap txhd2_demod_32k = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_DEMOD_32K_CNTL0,
		.bit_idx = 30,
	},
	.hw.init = &(struct clk_init_data){
		.name = "demod_32k",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_demod_32k_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static const struct clk_parent_data txhd2_hdmirx_sys_parent_data[] = {
	{ .fw_name = "xtal", },
	{ .hw = &txhd2_fclk_div4.hw },
	{ .hw = &txhd2_fclk_div3.hw },
	{ .hw = &txhd2_fclk_div5.hw }
};

static struct clk_regmap txhd2_hdmirx_5m_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_HDMIRX_CLK_CNTL0,
		.mask = 0x3,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdmirx_5m_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = txhd2_hdmirx_sys_parent_data,
		.num_parents = ARRAY_SIZE(txhd2_hdmirx_sys_parent_data),
	},
};

static struct clk_regmap txhd2_hdmirx_5m_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_HDMIRX_CLK_CNTL0,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdmirx_5m_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_hdmirx_5m_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap txhd2_hdmirx_5m  = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_HDMIRX_CLK_CNTL0,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hdmirx_5m",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_hdmirx_5m_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap txhd2_hdmirx_2m_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_HDMIRX_CLK_CNTL0,
		.mask = 0x3,
		.shift = 25,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdmirx_2m_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = txhd2_hdmirx_sys_parent_data,
		.num_parents = ARRAY_SIZE(txhd2_hdmirx_sys_parent_data),
	},
};

static struct clk_regmap txhd2_hdmirx_2m_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_HDMIRX_CLK_CNTL0,
		.shift = 16,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdmirx_2m_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_hdmirx_2m_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap txhd2_hdmirx_2m = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_HDMIRX_CLK_CNTL0,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hdmirx_2m",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_hdmirx_2m_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap txhd2_hdmirx_cfg_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_HDMIRX_CLK_CNTL1,
		.mask = 0x3,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdmirx_cfg_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = txhd2_hdmirx_sys_parent_data,
		.num_parents = ARRAY_SIZE(txhd2_hdmirx_sys_parent_data),
	},
};

static struct clk_regmap txhd2_hdmirx_cfg_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_HDMIRX_CLK_CNTL1,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdmirx_cfg_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_hdmirx_cfg_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap txhd2_hdmirx_cfg  = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_HDMIRX_CLK_CNTL1,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hdmirx_cfg",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_hdmirx_cfg_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap txhd2_hdmirx_hdcp_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_HDMIRX_CLK_CNTL1,
		.mask = 0x3,
		.shift = 25,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdmirx_hdcp_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = txhd2_hdmirx_sys_parent_data,
		.num_parents = ARRAY_SIZE(txhd2_hdmirx_sys_parent_data),
	},
};

static struct clk_regmap txhd2_hdmirx_hdcp_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_HDMIRX_CLK_CNTL1,
		.shift = 16,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdmirx_hdcp_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_hdmirx_hdcp_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap txhd2_hdmirx_hdcp = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_HDMIRX_CLK_CNTL1,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hdmirx_hdcp",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_hdmirx_hdcp_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap txhd2_hdmirx_aud_pll_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_HDMIRX_CLK_CNTL2,
		.mask = 0x3,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdmirx_aud_pll_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = txhd2_hdmirx_sys_parent_data,
		.num_parents = ARRAY_SIZE(txhd2_hdmirx_sys_parent_data),
	},
};

static struct clk_regmap txhd2_hdmirx_aud_pll_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_HDMIRX_CLK_CNTL2,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdmirx_aud_pll_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_hdmirx_aud_pll_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap txhd2_hdmirx_aud_pll  = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_HDMIRX_CLK_CNTL2,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hdmirx_aud_pll",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_hdmirx_aud_pll_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap txhd2_hdmirx_acr_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_HDMIRX_CLK_CNTL2,
		.mask = 0x3,
		.shift = 25,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdmirx_acr_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = txhd2_hdmirx_sys_parent_data,
		.num_parents = ARRAY_SIZE(txhd2_hdmirx_sys_parent_data),
	},
};

static struct clk_regmap txhd2_hdmirx_acr_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_HDMIRX_CLK_CNTL2,
		.shift = 16,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdmirx_acr_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_hdmirx_acr_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap txhd2_hdmirx_acr = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_HDMIRX_CLK_CNTL2,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hdmirx_acr",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_hdmirx_acr_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap txhd2_hdmirx_meter_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_HDMIRX_CLK_CNTL3,
		.mask = 0x3,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdmirx_meter_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = txhd2_hdmirx_sys_parent_data,
		.num_parents = ARRAY_SIZE(txhd2_hdmirx_sys_parent_data),
	},
};

static struct clk_regmap txhd2_hdmirx_meter_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_HDMIRX_CLK_CNTL3,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdmirx_meter_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_hdmirx_meter_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap txhd2_hdmirx_meter  = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_HDMIRX_CLK_CNTL3,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hdmirx_meter",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_hdmirx_meter_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

/*cts_cdac_clk*/
static const struct clk_parent_data txhd2_cdac_parent_data[] = {
	{ .fw_name = "xtal", },
	{ .hw = &txhd2_fclk_div5.hw },
};

static struct clk_regmap txhd2_cdac_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_CDAC_CLK_CNTL,
		.mask = 0x3,
		.shift = 16,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "cdac_clk_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = txhd2_cdac_parent_data,
		.num_parents = ARRAY_SIZE(txhd2_cdac_parent_data),
		.flags = CLK_GET_RATE_NOCACHE
	},
};

static struct clk_regmap txhd2_cdac_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_CDAC_CLK_CNTL,
		.shift = 0,
		.width = 16,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "cdac_clk_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_cdac_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap txhd2_cdac = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_CDAC_CLK_CNTL,
		.bit_idx = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "cdac",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_cdac_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE |	CLK_SET_RATE_PARENT
	},
};

/*eth clk*/
static u32 txhd2_eth_rmii_table[] = { 0 };
static struct clk_regmap txhd2_eth_rmii_sel = {
	.data = &(struct clk_regmap_mux_data) {
		.offset = HHI_ETH_CLK_CNTL,
		.mask = 0x7,
		.shift = 9,
		.table = txhd2_eth_rmii_table
	},
	.hw.init = &(struct clk_init_data){
		.name = "eth_rmii_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_fclk_div2.hw,
		},
		.num_parents = 1
	},
};

static struct clk_regmap txhd2_eth_rmii_div = {
	.data = &(struct clk_regmap_div_data) {
		.offset = HHI_ETH_CLK_CNTL,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "eth_rmii_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_eth_rmii_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap txhd2_eth_rmii = {
	.data = &(struct clk_regmap_gate_data) {
		.offset = HHI_ETH_CLK_CNTL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "eth_rmii",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_eth_rmii_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap txhd2_eth_125m = {
	.data = &(struct clk_regmap_gate_data) {
		.offset = HHI_ETH_CLK_CNTL,
		.bit_idx = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "eth_125m",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_fclk_div2.hw
		},
		.num_parents = 1,
	},
};

/*spicc clk*/
static u32 txhd2_spicc_mux_table[] = { 0, 1, 2, 3, 4, 5, 6};
static const struct clk_parent_data txhd2_spicc_parent_hws[] = {
	{ .fw_name = "xtal", },
	{ .hw = &txhd2_clk81.hw },
	{ .hw = &txhd2_fclk_div4.hw },
	{ .hw = &txhd2_fclk_div3.hw },
	{ .hw = &txhd2_fclk_div2.hw },
	{ .hw = &txhd2_fclk_div5.hw },
	{ .hw = &txhd2_fclk_div7.hw },
};

static struct clk_regmap txhd2_spicc0_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_SPICC_CLK_CNTL,
		.mask = 0x7,
		.shift = 7,
		.table = txhd2_spicc_mux_table
	},
	.hw.init = &(struct clk_init_data) {
		.name = "spicc0_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = txhd2_spicc_parent_hws,
		.num_parents = ARRAY_SIZE(txhd2_spicc_parent_hws),
	},
};

static struct clk_regmap txhd2_spicc0_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_SPICC_CLK_CNTL,
		.shift = 0,
		.width = 6,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "spicc0_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_spicc0_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap txhd2_spicc0 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_SPICC_CLK_CNTL,
		.bit_idx = 6,
	},
	.hw.init = &(struct clk_init_data){
		.name = "spicc0",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_spicc0_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

/*cts tvfe*/
static u32 txhd2_vafe_mux_table[] = { 1 };

static struct clk_regmap txhd2_vafe_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_ATV_DMD_SYS_CLK_CNTL,
		.mask = 0x3,
		.shift = 24,
		.table = txhd2_vafe_mux_table
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vafe_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = &(const struct clk_parent_data) {
						.fw_name = "xtal",
		},
		.num_parents = 1,
	},
};

static struct clk_regmap txhd2_vafe_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_ATV_DMD_SYS_CLK_CNTL,
		.shift = 16,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vafe_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_vafe_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap txhd2_vafe = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_ATV_DMD_SYS_CLK_CNTL,
		.bit_idx = 23,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vafe",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_vafe_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static u32 txhd2_tsin_deglich_mux_table[] = {0, 2, 3, 4, 5, 6};
static const struct clk_hw *txhd2_tsin_parent_parent_hws[] = {
	&txhd2_fclk_div2.hw,
	&txhd2_fclk_div4.hw,
	&txhd2_fclk_div3.hw,
	&txhd2_fclk_div2p5.hw,
	&txhd2_fclk_div5.hw,
	&txhd2_fclk_div7.hw,
};

static struct clk_regmap txhd2_tsin_deglich_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_TSIN_DEGLITCH_CLK_CNTL,
		.mask = 0x7,
		.shift = 7,
		.table = txhd2_tsin_deglich_mux_table
	},
	.hw.init = &(struct clk_init_data) {
		.name = "tsin_deglich_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = txhd2_tsin_parent_parent_hws,
		.num_parents = ARRAY_SIZE(txhd2_tsin_parent_parent_hws),
	},
};

static struct clk_regmap txhd2_tsin_deglich_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_TSIN_DEGLITCH_CLK_CNTL,
		.shift = 0,
		.width = 6,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "tsin_deglich_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_tsin_deglich_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap txhd2_tsin_deglich = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_TSIN_DEGLITCH_CLK_CNTL,
		.bit_idx = 6,
	},
	.hw.init = &(struct clk_init_data){
		.name = "tsin_deglich",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&txhd2_tsin_deglich_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

#define MESON_TXHD2_SYS_GATE(_name, _reg, _bit)				\
struct clk_regmap _name = {						\
	.data = &(struct clk_regmap_gate_data) {			\
		.offset = (_reg),					\
		.bit_idx = (_bit),					\
	},								\
	.hw.init = &(struct clk_init_data) {				\
		.name = #_name,						\
		.ops = &clk_regmap_gate_ops,				\
		.parent_hws = (const struct clk_hw *[]) {		\
			&txhd2_clk81.hw					\
		},							\
		.num_parents = 1,					\
		.flags = CLK_IGNORE_UNUSED,				\
	},								\
}

static MESON_TXHD2_SYS_GATE(txhd2_clk81_ddr,		HHI_GCLK_MPEG0, 0);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_dos,		HHI_GCLK_MPEG0, 1);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_ethphy,		HHI_GCLK_MPEG0, 4);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_isa,		HHI_GCLK_MPEG0, 5);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_pl310,		HHI_GCLK_MPEG0, 6);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_periphs,		HHI_GCLK_MPEG0, 7);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_spicc0,		HHI_GCLK_MPEG0, 8);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_i2c,		HHI_GCLK_MPEG0, 9);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_sana,		HHI_GCLK_MPEG0, 10);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_smartcard,		HHI_GCLK_MPEG0, 11);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_uart0,		HHI_GCLK_MPEG0, 13);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_stream,		HHI_GCLK_MPEG0, 15);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_async_fifo,		HHI_GCLK_MPEG0, 16);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_tvfe,		HHI_GCLK_MPEG0, 18);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_hiu_reg,		HHI_GCLK_MPEG0, 19);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_hdmi20_aes,		HHI_GCLK_MPEG0, 20);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_hdmirx_pclk,	HHI_GCLK_MPEG0, 21);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_atv_demod,		HHI_GCLK_MPEG0, 22);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_assist_misc,	HHI_GCLK_MPEG0, 23);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_pwr_ctrl,		HHI_GCLK_MPEG0, 24);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_emmc_c,		HHI_GCLK_MPEG0, 26);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_adec,		HHI_GCLK_MPEG0, 27);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_acodec,		HHI_GCLK_MPEG0, 28);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_tcon,		HHI_GCLK_MPEG0, 29);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_spi,		HHI_GCLK_MPEG0, 30);

static MESON_TXHD2_SYS_GATE(txhd2_clk81_audio,		HHI_GCLK_MPEG1, 0);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_eth,		HHI_GCLK_MPEG1, 3);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_dmux,		HHI_GCLK_MPEG1, 4);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_clk_rst,		HHI_GCLK_MPEG1, 5);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_aififo,		HHI_GCLK_MPEG1, 11);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_uart1,		HHI_GCLK_MPEG1, 16);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_g2d,		HHI_GCLK_MPEG1, 20);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_reset,		HHI_GCLK_MPEG1, 23);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_parser,		HHI_GCLK_MPEG1, 25);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_usb_gene,		HHI_GCLK_MPEG1, 26);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_parser1,		HHI_GCLK_MPEG1, 28);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_ahb_arb0,		HHI_GCLK_MPEG1, 29);

static MESON_TXHD2_SYS_GATE(txhd2_clk81_ahb_data,		HHI_GCLK_MPEG2, 1);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_ahb_ctrl,		HHI_GCLK_MPEG2, 2);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_usb1_to_ddr,	HHI_GCLK_MPEG2, 8);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_mmc_pclk,		HHI_GCLK_MPEG2, 11);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_hdmirx_axi,		HHI_GCLK_MPEG2, 12);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_hdcp22_pclk,	HHI_GCLK_MPEG2, 13);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_uart2,		HHI_GCLK_MPEG2, 15);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_ciplus,		HHI_GCLK_MPEG2, 21);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_ts,			HHI_GCLK_MPEG2, 22);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_vpu_int,		HHI_GCLK_MPEG2, 25);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_demod_com,		HHI_GCLK_MPEG2, 28);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_mute,		HHI_GCLK_MPEG2, 29);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_gic,		HHI_GCLK_MPEG2, 30);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_aucpu,		HHI_GCLK_MPEG2, 31);

static MESON_TXHD2_SYS_GATE(txhd2_clk81_vclk2_venci0,	HHI_GCLK_OTHER, 1);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_vclk2_venci1,	HHI_GCLK_OTHER, 2);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_vclk2_vencp0,	HHI_GCLK_OTHER, 3);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_vclk2_vencp1,	HHI_GCLK_OTHER, 4);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_vclk2_venct0,	HHI_GCLK_OTHER, 5);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_vclk2_venct1,	HHI_GCLK_OTHER, 6);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_vclk2_other,	HHI_GCLK_OTHER, 7);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_vclk2_enci,		HHI_GCLK_OTHER, 8);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_vclk2_encp,		HHI_GCLK_OTHER, 9);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_dac_clk,		HHI_GCLK_OTHER, 10);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_enc480p,		HHI_GCLK_OTHER, 20);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_random,		HHI_GCLK_OTHER, 21);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_vclk2_enct,		HHI_GCLK_OTHER, 22);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_vclk2_encl,		HHI_GCLK_OTHER, 23);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_vclk2_venclmmc,	HHI_GCLK_OTHER, 24);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_vclk2_vencl,	HHI_GCLK_OTHER, 25);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_vclk2_other1,	HHI_GCLK_OTHER, 26);

static MESON_TXHD2_SYS_GATE(txhd2_clk81_dma,		HHI_GCLK_SP_MPEG, 0);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_efuse,		HHI_GCLK_SP_MPEG, 1);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_rom_boot,		HHI_GCLK_SP_MPEG, 2);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_reset_sec,		HHI_GCLK_SP_MPEG, 3);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_sec_ahb,		HHI_GCLK_SP_MPEG, 4);
static MESON_TXHD2_SYS_GATE(txhd2_clk81_rsa,		HHI_GCLK_SP_MPEG, 5);

/* Array of all clocks provided by this provider */
static struct clk_hw_onecell_data txhd2_hw_onecell_data = {
	.hws = {
		[CLKID_FIXED_PLL_DCO]		= &txhd2_fixed_pll_dco.hw,
		[CLKID_FIXED_PLL]		= &txhd2_fixed_pll.hw,
		[CLKID_FCLK_DIV2_DIV]		= &txhd2_fclk_div2_div.hw,
		[CLKID_FCLK_DIV3_DIV]		= &txhd2_fclk_div3_div.hw,
		[CLKID_FCLK_DIV4_DIV]		= &txhd2_fclk_div4_div.hw,
		[CLKID_FCLK_DIV5_DIV]		= &txhd2_fclk_div5_div.hw,
		[CLKID_FCLK_DIV7_DIV]		= &txhd2_fclk_div7_div.hw,
		[CLKID_FCLK_DIV2P5_DIV]		= &txhd2_fclk_div2p5_div.hw,
		[CLKID_FCLK_DIV2]		= &txhd2_fclk_div2.hw,
		[CLKID_FCLK_DIV3]		= &txhd2_fclk_div3.hw,
		[CLKID_FCLK_DIV4]		= &txhd2_fclk_div4.hw,
		[CLKID_FCLK_DIV5]		= &txhd2_fclk_div5.hw,
		[CLKID_FCLK_DIV7]		= &txhd2_fclk_div7.hw,
		[CLKID_FCLK_DIV2P5]		= &txhd2_fclk_div2p5.hw,
		[CLKID_MPLL_50M_DIV]		= &txhd2_mpll_50m_div.hw,
		[CLKID_MPLL_50M]		= &txhd2_mpll_50m.hw,
		[CLKID_SYS_PLL_DCO]		= &txhd2_sys_pll_dco.hw,
		[CLKID_SYS_PLL]			= &txhd2_sys_pll.hw,
		[CLKID_GP0_PLL_DCO]		= &txhd2_gp0_pll_dco.hw,
		[CLKID_GP0_PLL]			= &txhd2_gp0_pll.hw,
		[CLKID_HIFI_PLL_DCO]		= &txhd2_hifi_pll_dco.hw,
		[CLKID_HIFI_PLL]		= &txhd2_hifi_pll.hw,
		[CLKID_CPU_CLK_DYN]		= &txhd2_cpu_dyn_clk.hw,
		[CLKID_CPU_CLK]			= &txhd2_cpu_clk.hw,
		[CLKID_CLK81_DDR]		= &txhd2_clk81_ddr.hw,
		[CLKID_CLK81_DOS]		= &txhd2_clk81_dos.hw,
		[CLKID_CLK81_ETH_PHY]		= &txhd2_clk81_ethphy.hw,
		[CLKID_CLK81_ISA]		= &txhd2_clk81_isa.hw,
		[CLKID_CLK81_PL310]		= &txhd2_clk81_pl310.hw,
		[CLKID_CLK81_PERIPHS]		= &txhd2_clk81_periphs.hw,
		[CLKID_CLK81_SPICC0]		= &txhd2_clk81_spicc0.hw,
		[CLKID_CLK81_I2C]		= &txhd2_clk81_i2c.hw,
		[CLKID_CLK81_SANA]		= &txhd2_clk81_sana.hw,
		[CLKID_CLK81_SMARTCARD]		= &txhd2_clk81_smartcard.hw,
		[CLKID_CLK81_UART0]		= &txhd2_clk81_uart0.hw,
		[CLKID_CLK81_STREAM]		= &txhd2_clk81_stream.hw,
		[CLKID_CLK81_ASYNC_FIFO]	= &txhd2_clk81_async_fifo.hw,
		[CLKID_CLK81_TVFE]		= &txhd2_clk81_tvfe.hw,
		[CLKID_CLK81_HIU_REG]		= &txhd2_clk81_hiu_reg.hw,
		[CLKID_CLK81_HDMIRX_PCLK]	= &txhd2_clk81_hdmirx_pclk.hw,
		[CLKID_CLK81_ATV_DEMOD]		= &txhd2_clk81_atv_demod.hw,
		[CLKID_CLK81_ASSIST_MISC]	= &txhd2_clk81_assist_misc.hw,
		[CLKID_CLK81_PWR_CTRL]		= &txhd2_clk81_pwr_ctrl.hw,
		[CLKID_CLK81_SD_EMMC_C]		= &txhd2_clk81_emmc_c.hw,
		[CLKID_CLK81_ADEC]		= &txhd2_clk81_adec.hw,
		[CLKID_CLK81_ACODEC]		= &txhd2_clk81_acodec.hw,
		[CLKID_CLK81_TCON]		= &txhd2_clk81_tcon.hw,
		[CLKID_CLK81_SPI]		= &txhd2_clk81_spi.hw,
		[CLKID_CLK81_AUDIO]		= &txhd2_clk81_audio.hw,
		[CLKID_CLK81_ETH_CORE]		= &txhd2_clk81_eth.hw,
		[CLKID_CLK81_CLK_RST]		= &txhd2_clk81_clk_rst.hw,
		[CLKID_CLK81_DMUX]		= &txhd2_clk81_dmux.hw,
		[CLKID_CLK81_PARSER]		= &txhd2_clk81_parser.hw,
		[CLKID_CLK81_PARSER1]		= &txhd2_clk81_parser1.hw,
		[CLKID_CLK81_AIFIFO]		= &txhd2_clk81_aififo.hw,
		[CLKID_CLK81_UART1]		= &txhd2_clk81_uart1.hw,
		[CLKID_CLK81_G2D]		= &txhd2_clk81_g2d.hw,
		[CLKID_CLK81_RESET]		= &txhd2_clk81_reset.hw,
		[CLKID_CLK81_USB_GENERAL]	= &txhd2_clk81_usb_gene.hw,
		[CLKID_CLK81_AHB_ARB0]		= &txhd2_clk81_ahb_arb0.hw,
		[CLKID_CLK81_AHB_DATA_BUS]	= &txhd2_clk81_ahb_data.hw,
		[CLKID_CLK81_AHB_CTRL_BUS]	= &txhd2_clk81_ahb_ctrl.hw,
		[CLKID_CLK81_USB1_TO_DDR]	= &txhd2_clk81_usb1_to_ddr.hw,
		[CLKID_CLK81_MMC_PCLK]		= &txhd2_clk81_mmc_pclk.hw,
		[CLKID_CLK81_HDMIRX_AXI]	= &txhd2_clk81_hdmirx_axi.hw,
		[CLKID_CLK81_HDCP22_PCLK]	= &txhd2_clk81_hdcp22_pclk.hw,
		[CLKID_CLK81_UART2]		= &txhd2_clk81_uart2.hw,
		[CLKID_CLK81_CLK81_TS]		= &txhd2_clk81_ts.hw,
		[CLKID_CLK81_VPU_INTR]		= &txhd2_clk81_vpu_int.hw,
		[CLKID_CLK81_DEMOD_COMB]	= &txhd2_clk81_demod_com.hw,
		[CLKID_CLK81_GIC]		= &txhd2_clk81_gic.hw,
		[CLKID_CLK81_VCLK2_VENCI0]	= &txhd2_clk81_vclk2_venci0.hw,
		[CLKID_CLK81_VCLK2_VENCI1]	= &txhd2_clk81_vclk2_venci1.hw,
		[CLKID_CLK81_VCLK2_VENCP0]	= &txhd2_clk81_vclk2_vencp0.hw,
		[CLKID_CLK81_VCLK2_VENCP1]	= &txhd2_clk81_vclk2_vencp1.hw,
		[CLKID_CLK81_VCLK2_VENCT0]	= &txhd2_clk81_vclk2_venct0.hw,
		[CLKID_CLK81_VCLK2_VENCT1]	= &txhd2_clk81_vclk2_venct1.hw,
		[CLKID_CLK81_VCLK2_OTHER]	= &txhd2_clk81_vclk2_other.hw,
		[CLKID_CLK81_VCLK2_ENCI]	= &txhd2_clk81_vclk2_enci.hw,
		[CLKID_CLK81_VCLK2_ENCP]	= &txhd2_clk81_vclk2_encp.hw,
		[CLKID_CLK81_DAC_CLK]		= &txhd2_clk81_dac_clk.hw,
		[CLKID_CLK81_ENC480P]		= &txhd2_clk81_enc480p.hw,
		[CLKID_CLK81_RANDOM]		= &txhd2_clk81_random.hw,
		[CLKID_CLK81_VCLK2_ENCT]	= &txhd2_clk81_vclk2_enct.hw,
		[CLKID_CLK81_VCLK2_ENCL]	= &txhd2_clk81_vclk2_encl.hw,
		[CLKID_CLK81_VCLK2_VENCLMMC]	= &txhd2_clk81_vclk2_venclmmc.hw,
		[CLKID_CLK81_VCLK2_VENCL]	= &txhd2_clk81_vclk2_vencl.hw,
		[CLKID_CLK81_VCLK2_OTHER1]	= &txhd2_clk81_vclk2_other1.hw,
		[CLKID_CLK81_DMA]		= &txhd2_clk81_dma.hw,
		[CLKID_CLK81_EFUSE]		= &txhd2_clk81_efuse.hw,
		[CLKID_CLK81_ROM_BOOT]		= &txhd2_clk81_rom_boot.hw,
		[CLKID_CLK81_RESET_SEC]		= &txhd2_clk81_reset_sec.hw,
		[CLKID_CLK81_SEC_AHB]		= &txhd2_clk81_sec_ahb.hw,
		[CLKID_CLK81_RSA]		= &txhd2_clk81_rsa.hw,
		[CLKID_MPEG_SEL]		= &txhd2_mpeg_clk_sel.hw,
		[CLKID_MPEG_DIV]		= &txhd2_mpeg_clk_div.hw,
		[CLKID_CLK81]			= &txhd2_clk81.hw,
		[CLKID_VDEC_0_SEL]		= &txhd2_vdec_0_sel.hw,
		[CLKID_VDEC_0_DIV]		= &txhd2_vdec_0_div.hw,
		[CLKID_VDEC_0]			= &txhd2_vdec_0.hw,
		[CLKID_VDEC_1_SEL]		= &txhd2_vdec_1_sel.hw,
		[CLKID_VDEC_1_DIV]		= &txhd2_vdec_1_div.hw,
		[CLKID_VDEC_1]			= &txhd2_vdec_1.hw,
		[CLKID_VDEC]			= &txhd2_vdec.hw,
		[CLKID_HEVCF_0_SEL]		= &txhd2_hevcf_0_sel.hw,
		[CLKID_HEVCF_0_DIV]		= &txhd2_hevcf_0_div.hw,
		[CLKID_HEVCF_0]			= &txhd2_hevcf_0.hw,
		[CLKID_HEVCF_1_SEL]		= &txhd2_hevcf_1_sel.hw,
		[CLKID_HEVCF_1_DIV]		= &txhd2_hevcf_1_div.hw,
		[CLKID_HEVCF_1]			= &txhd2_hevcf_1.hw,
		[CLKID_HEVCF]			= &txhd2_hevcf.hw,
		[CLKID_GE2D]			= &txhd2_ge2d.hw,
		[CLKID_VAPB_0_SEL]		= &txhd2_vapb_0_sel.hw,
		[CLKID_VAPB_0_DIV]		= &txhd2_vapb_0_div.hw,
		[CLKID_VAPB_0]			= &txhd2_vapb_0.hw,
		[CLKID_VAPB_1_SEL]		= &txhd2_vapb_1_sel.hw,
		[CLKID_VAPB_1_DIV]		= &txhd2_vapb_1_div.hw,
		[CLKID_VAPB_1]			= &txhd2_vapb_1.hw,
		[CLKID_VAPB]			= &txhd2_vapb.hw,
		[CLKID_MALI_0_SEL]		= &txhd2_mali_0_sel.hw,
		[CLKID_MALI_0_DIV]		= &txhd2_mali_0_div.hw,
		[CLKID_MALI_0]			= &txhd2_mali_0.hw,
		[CLKID_MALI_1_SEL]		= &txhd2_mali_1_sel.hw,
		[CLKID_MALI_1_DIV]		= &txhd2_mali_1_div.hw,
		[CLKID_MALI_1]			= &txhd2_mali_1.hw,
		[CLKID_MALI]			= &txhd2_mali.hw,
		[CLKID_TS_CLK_DIV]		= &txhd2_ts_clk_div.hw,
		[CLKID_TS_CLK]			= &txhd2_ts_clk.hw,
		[CLKID_SD_EMMC_C_SEL]		= &txhd2_sd_emmc_c_sel.hw,
		[CLKID_SD_EMMC_C_DIV]		= &txhd2_sd_emmc_c_div.hw,
		[CLKID_SD_EMMC_C]		= &txhd2_sd_emmc_c.hw,
		[CLKID_VDIN_MEAS_SEL]		= &txhd2_vdin_meas_sel.hw,
		[CLKID_VDIN_MEAS_DIV]		= &txhd2_vdin_meas_div.hw,
		[CLKID_VDIN_MEAS]		= &txhd2_vdin_meas.hw,
		[CLKID_VID_LOCK_DIV]		= &txhd2_vid_lock_div.hw,
		[CLKID_VID_LOCK]		= &txhd2_vid_lock_clk.hw,
		[CLKID_VPU_0_SEL]		= &txhd2_vpu_0_sel.hw,
		[CLKID_VPU_0_DIV]		= &txhd2_vpu_0_div.hw,
		[CLKID_VPU_0]			= &txhd2_vpu_0.hw,
		[CLKID_VPU_1_SEL]		= &txhd2_vpu_1_sel.hw,
		[CLKID_VPU_1_DIV]		= &txhd2_vpu_1_div.hw,
		[CLKID_VPU_1]			= &txhd2_vpu_1.hw,
		[CLKID_VPU]			= &txhd2_vpu.hw,
		[CLKID_VPU_CLKB_TMP_SEL]	= &txhd2_vpu_clkb_tmp_sel.hw,
		[CLKID_VPU_CLKB_TMP_DIV]	= &txhd2_vpu_clkb_tmp_div.hw,
		[CLKID_VPU_CLKB_TMP]		= &txhd2_vpu_clkb_tmp.hw,
		[CLKID_VPU_CLKB_DIV]		= &txhd2_vpu_clkb_div.hw,
		[CLKID_VPU_CLKB]		= &txhd2_vpu_clkb.hw,
		[CLKID_VPU_CLKC_0_SEL]		= &txhd2_vpu_clkc_0_sel.hw,
		[CLKID_VPU_CLKC_0_DIV]		= &txhd2_vpu_clkc_0_div.hw,
		[CLKID_VPU_CLKC_0]		= &txhd2_vpu_clkc_0.hw,
		[CLKID_VPU_CLKC_1_SEL]		= &txhd2_vpu_clkc_1_sel.hw,
		[CLKID_VPU_CLKC_1_DIV]		= &txhd2_vpu_clkc_1_div.hw,
		[CLKID_VPU_CLKC_1]		= &txhd2_vpu_clkc_1.hw,
		[CLKID_VPU_CLKC]		= &txhd2_vpu_clkc.hw,
		[CLKID_CTS_TCON_PLL_CLK_SEL]	= &txhd2_cts_tcon_pll_clk_sel.hw,
		[CLKID_CTS_TCON_PLL_CLK_DIV]	= &txhd2_cts_tcon_pll_clk_div.hw,
		[CLKID_CTS_TCON_PLL_CLK]	= &txhd2_cts_tcon_pll_clk.hw,
		[CLKID_ADC_EXTCLK_SEL]		= &txhd2_adc_extclk_sel.hw,
		[CLKID_ADC_EXTCLK_DIV]		= &txhd2_adc_extclk_div.hw,
		[CLKID_ADC_EXTCLK]		= &txhd2_adc_extclk.hw,
		[CLKID_CTS_DEMOD_CORE_SEL]	= &txhd2_cts_demod_core_sel.hw,
		[CLKID_CTS_DEMOD_CORE_DIV]	= &txhd2_cts_demod_core_div.hw,
		[CLKID_CTS_DEMOD_CORE]		= &txhd2_cts_demod_core.hw,
		[CLKID_DEMOD_32K_CLKIN]		= &txhd2_demod_32k_clkin.hw,
		[CLKID_DEMOD_32K_DIV]		= &txhd2_demod_32k_div.hw,
		[CLKID_DEMOD_32K]		= &txhd2_demod_32k.hw,
		[CLKID_HDMIRX_5M_SEL]		= &txhd2_hdmirx_5m_sel.hw,
		[CLKID_HDMIRX_5M_DIV]		= &txhd2_hdmirx_5m_div.hw,
		[CLKID_HDMIRX_5M]		= &txhd2_hdmirx_5m.hw,
		[CLKID_HDMIRX_2M_SEL]		= &txhd2_hdmirx_2m_sel.hw,
		[CLKID_HDMIRX_2M_DIV]		= &txhd2_hdmirx_2m_div.hw,
		[CLKID_HDMIRX_2M]		= &txhd2_hdmirx_2m.hw,
		[CLKID_HDMIRX_CFG_SEL]		= &txhd2_hdmirx_cfg_sel.hw,
		[CLKID_HDMIRX_CFG_DIV]		= &txhd2_hdmirx_cfg_div.hw,
		[CLKID_HDMIRX_CFG]		= &txhd2_hdmirx_cfg.hw,
		[CLKID_HDMIRX_HDCP_SEL]		= &txhd2_hdmirx_hdcp_sel.hw,
		[CLKID_HDMIRX_HDCP_DIV]		= &txhd2_hdmirx_hdcp_div.hw,
		[CLKID_HDMIRX_HDCP]		= &txhd2_hdmirx_hdcp.hw,
		[CLKID_HDMIRX_AUD_PLL_SEL]	= &txhd2_hdmirx_aud_pll_sel.hw,
		[CLKID_HDMIRX_AUD_PLL_DIV]	= &txhd2_hdmirx_aud_pll_div.hw,
		[CLKID_HDMIRX_AUD_PLL]		= &txhd2_hdmirx_aud_pll.hw,
		[CLKID_HDMIRX_ACR_SEL]		= &txhd2_hdmirx_acr_sel.hw,
		[CLKID_HDMIRX_ACR_DIV]		= &txhd2_hdmirx_acr_div.hw,
		[CLKID_HDMIRX_ACR]		= &txhd2_hdmirx_acr.hw,
		[CLKID_HDMIRX_METER_SEL]	= &txhd2_hdmirx_meter_sel.hw,
		[CLKID_HDMIRX_METER_DIV]	= &txhd2_hdmirx_meter_div.hw,
		[CLKID_HDMIRX_METER]		= &txhd2_hdmirx_meter.hw,
		[CLKID_CDAC_CLK_SEL]		= &txhd2_cdac_sel.hw,
		[CLKID_CDAC_CLK_DIV]		= &txhd2_cdac_div.hw,
		[CLKID_CDAC_CLK]		= &txhd2_cdac.hw,
		[CLKID_ETH_RMII_SEL]		= &txhd2_eth_rmii_sel.hw,
		[CLKID_ETH_RMII_DIV]		= &txhd2_eth_rmii_div.hw,
		[CLKID_ETH_RMII]		= &txhd2_eth_rmii.hw,
		[CLKID_ETH_125M]		= &txhd2_eth_125m.hw,
		[CLKID_SPICC0_SEL]		= &txhd2_spicc0_sel.hw,
		[CLKID_SPICC0_DIV]		= &txhd2_spicc0_div.hw,
		[CLKID_SPICC0]			= &txhd2_spicc0.hw,
		[CLKID_TVFE_CLK_SEL]		= &txhd2_vafe_sel.hw,
		[CLKID_TVFE_CLK_DIV]		= &txhd2_vafe_div.hw,
		[CLKID_TVFE_CLK]		= &txhd2_vafe.hw,
		[CLKID_TSIN_DEGLICH_CLK_SEL]	= &txhd2_tsin_deglich_sel.hw,
		[CLKID_TSIN_DEGLICH_CLK_DIV]	= &txhd2_tsin_deglich_div.hw,
		[CLKID_TSIN_DEGLICH_CLK]	= &txhd2_tsin_deglich.hw,
		[NR_CLKS]			= NULL
	},
	.num = NR_CLKS,
};

static struct clk_regmap *const txhd2_clk_regmaps[] __initconst = {
	&txhd2_mpeg_clk_sel,
	&txhd2_mpeg_clk_div,
	&txhd2_clk81,
	&txhd2_vdec_0_sel,
	&txhd2_vdec_0_div,
	&txhd2_vdec_0,
	&txhd2_vdec_1_sel,
	&txhd2_vdec_1_div,
	&txhd2_vdec_1,
	&txhd2_vdec,
	&txhd2_hevcf_0_sel,
	&txhd2_hevcf_0_div,
	&txhd2_hevcf_0,
	&txhd2_hevcf_1_sel,
	&txhd2_hevcf_1_div,
	&txhd2_hevcf_1,
	&txhd2_hevcf,
	&txhd2_ge2d,
	&txhd2_vapb_0_sel,
	&txhd2_vapb_0_div,
	&txhd2_vapb_0,
	&txhd2_vapb_1_sel,
	&txhd2_vapb_1_div,
	&txhd2_vapb_1,
	&txhd2_vapb,
	&txhd2_mali_0_sel,
	&txhd2_mali_0_div,
	&txhd2_mali_0,
	&txhd2_mali_1_sel,
	&txhd2_mali_1_div,
	&txhd2_mali_1,
	&txhd2_mali,
	&txhd2_ts_clk_div,
	&txhd2_ts_clk,
	&txhd2_sd_emmc_c_sel,
	&txhd2_sd_emmc_c_div,
	&txhd2_sd_emmc_c,
	&txhd2_vdin_meas_sel,
	&txhd2_vdin_meas_div,
	&txhd2_vdin_meas,
	&txhd2_vid_lock_div,
	&txhd2_vid_lock_clk,
	&txhd2_vpu_0_sel,
	&txhd2_vpu_0_div,
	&txhd2_vpu_0,
	&txhd2_vpu_1_sel,
	&txhd2_vpu_1_div,
	&txhd2_vpu_1,
	&txhd2_vpu,
	&txhd2_vpu_clkb_tmp_sel,
	&txhd2_vpu_clkb_tmp_div,
	&txhd2_vpu_clkb_tmp,
	&txhd2_vpu_clkb_div,
	&txhd2_vpu_clkb,
	&txhd2_vpu_clkc_0_sel,
	&txhd2_vpu_clkc_0_div,
	&txhd2_vpu_clkc_0,
	&txhd2_vpu_clkc_1_sel,
	&txhd2_vpu_clkc_1_div,
	&txhd2_vpu_clkc_1,
	&txhd2_vpu_clkc,
	&txhd2_cts_tcon_pll_clk_sel,
	&txhd2_cts_tcon_pll_clk_div,
	&txhd2_cts_tcon_pll_clk,
	&txhd2_adc_extclk_sel,
	&txhd2_adc_extclk_div,
	&txhd2_adc_extclk,
	&txhd2_cts_demod_core_sel,
	&txhd2_cts_demod_core_div,
	&txhd2_cts_demod_core,
	&txhd2_demod_32k_clkin,
	&txhd2_demod_32k_div,
	&txhd2_demod_32k,
	&txhd2_hdmirx_5m_sel,
	&txhd2_hdmirx_5m_div,
	&txhd2_hdmirx_5m,
	&txhd2_hdmirx_2m_sel,
	&txhd2_hdmirx_2m_div,
	&txhd2_hdmirx_2m,
	&txhd2_hdmirx_cfg_sel,
	&txhd2_hdmirx_cfg_div,
	&txhd2_hdmirx_cfg,
	&txhd2_hdmirx_hdcp_sel,
	&txhd2_hdmirx_hdcp_div,
	&txhd2_hdmirx_hdcp,
	&txhd2_hdmirx_aud_pll_sel,
	&txhd2_hdmirx_aud_pll_div,
	&txhd2_hdmirx_aud_pll,
	&txhd2_hdmirx_acr_sel,
	&txhd2_hdmirx_acr_div,
	&txhd2_hdmirx_acr,
	&txhd2_hdmirx_meter_sel,
	&txhd2_hdmirx_meter_div,
	&txhd2_hdmirx_meter,
	&txhd2_cdac_sel,
	&txhd2_cdac_div,
	&txhd2_cdac,
	&txhd2_eth_rmii_sel,
	&txhd2_eth_rmii_div,
	&txhd2_eth_rmii,
	&txhd2_eth_125m,
	&txhd2_spicc0_sel,
	&txhd2_spicc0_div,
	&txhd2_spicc0,
	&txhd2_vafe_sel,
	&txhd2_vafe_div,
	&txhd2_vafe,
	&txhd2_tsin_deglich_sel,
	&txhd2_tsin_deglich_div,
	&txhd2_tsin_deglich,
};

static struct clk_regmap *const txhd2_cpu_clk_regmaps[] __initconst = {
	&txhd2_fixed_pll_dco,
	&txhd2_fixed_pll,
	&txhd2_fclk_div2,
	&txhd2_fclk_div3,
	&txhd2_fclk_div4,
	&txhd2_fclk_div5,
	&txhd2_fclk_div7,
	&txhd2_fclk_div2p5,
	&txhd2_mpll_50m,
	&txhd2_sys_pll_dco,
	&txhd2_sys_pll,
	&txhd2_gp0_pll_dco,
	&txhd2_gp0_pll,
	&txhd2_hifi_pll_dco,
	&txhd2_hifi_pll,
	&txhd2_cpu_dyn_clk,
	&txhd2_cpu_clk,
	&txhd2_clk81_ddr,
	&txhd2_clk81_dos,
	&txhd2_clk81_ethphy,
	&txhd2_clk81_isa,
	&txhd2_clk81_pl310,
	&txhd2_clk81_periphs,
	&txhd2_clk81_spicc0,
	&txhd2_clk81_i2c,
	&txhd2_clk81_sana,
	&txhd2_clk81_smartcard,
	&txhd2_clk81_uart0,
	&txhd2_clk81_stream,
	&txhd2_clk81_async_fifo,
	&txhd2_clk81_tvfe,
	&txhd2_clk81_hiu_reg,
	&txhd2_clk81_hdmi20_aes,
	&txhd2_clk81_hdmirx_pclk,
	&txhd2_clk81_atv_demod,
	&txhd2_clk81_assist_misc,
	&txhd2_clk81_pwr_ctrl,
	&txhd2_clk81_emmc_c,
	&txhd2_clk81_adec,
	&txhd2_clk81_acodec,
	&txhd2_clk81_tcon,
	&txhd2_clk81_spi,
	&txhd2_clk81_audio,
	&txhd2_clk81_eth,
	&txhd2_clk81_dmux,
	&txhd2_clk81_parser,
	&txhd2_clk81_parser1,
	&txhd2_clk81_clk_rst,
	&txhd2_clk81_aififo,
	&txhd2_clk81_uart1,
	&txhd2_clk81_g2d,
	&txhd2_clk81_reset,
	&txhd2_clk81_usb_gene,
	&txhd2_clk81_ahb_arb0,
	&txhd2_clk81_ahb_data,
	&txhd2_clk81_ahb_ctrl,
	&txhd2_clk81_usb1_to_ddr,
	&txhd2_clk81_mmc_pclk,
	&txhd2_clk81_hdmirx_axi,
	&txhd2_clk81_hdcp22_pclk,
	&txhd2_clk81_uart2,
	&txhd2_clk81_ciplus,
	&txhd2_clk81_ts,
	&txhd2_clk81_vpu_int,
	&txhd2_clk81_demod_com,
	&txhd2_clk81_mute,
	&txhd2_clk81_gic,
	&txhd2_clk81_aucpu,
	&txhd2_clk81_vclk2_venci0,
	&txhd2_clk81_vclk2_venci1,
	&txhd2_clk81_vclk2_vencp0,
	&txhd2_clk81_vclk2_vencp1,
	&txhd2_clk81_vclk2_venct0,
	&txhd2_clk81_vclk2_venct1,
	&txhd2_clk81_vclk2_other,
	&txhd2_clk81_vclk2_enci,
	&txhd2_clk81_vclk2_encp,
	&txhd2_clk81_dac_clk,
	&txhd2_clk81_enc480p,
	&txhd2_clk81_random,
	&txhd2_clk81_vclk2_enct,
	&txhd2_clk81_vclk2_encl,
	&txhd2_clk81_vclk2_venclmmc,
	&txhd2_clk81_vclk2_vencl,
	&txhd2_clk81_vclk2_other1,
	&txhd2_clk81_dma,
	&txhd2_clk81_efuse,
	&txhd2_clk81_rom_boot,
	&txhd2_clk81_reset_sec,
	&txhd2_clk81_sec_ahb,
	&txhd2_clk81_rsa,
};

static int meson_txhd2_dvfs_setup(struct platform_device *pdev)
{
	int ret;
	/*Setup cluster 0 clock notifier for sys_pll */
	ret = clk_notifier_register(txhd2_sys_pll.hw.clk,
				    &txhd2_sys_pll_nb_data.nb);
	if (ret) {
		dev_err(&pdev->dev, "failed to register sys_pll notifier\n");
		return ret;
	}

	return 0;
}

static struct regmap_config clkc_regmap_config = {
	.reg_bits       = 32,
	.val_bits       = 32,
	.reg_stride     = 4,
};

static struct regmap *txhd2_regmap_resource(struct device *dev, char *name)
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

static int __ref meson_txhd2_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct regmap *basic_map;
	struct regmap *cpu_map;
	int ret, i;

	/* Get regmap for different clock area */
	basic_map = txhd2_regmap_resource(dev, "basic");
	if (IS_ERR(basic_map)) {
		dev_err(dev, "basic clk registers not found\n");
		return PTR_ERR(basic_map);
	}

	cpu_map = txhd2_regmap_resource(dev, "cpu");
	if (IS_ERR(cpu_map)) {
		dev_err(dev, "pll clk registers not found\n");
		return PTR_ERR(cpu_map);
	}

	/* Populate regmap for the regmap backed clocks */
	for (i = 0; i < ARRAY_SIZE(txhd2_clk_regmaps); i++)
		txhd2_clk_regmaps[i]->map = basic_map;

	for (i = 0; i < ARRAY_SIZE(txhd2_cpu_clk_regmaps); i++)
		txhd2_cpu_clk_regmaps[i]->map = cpu_map;

	for (i = 0; i < txhd2_hw_onecell_data.num; i++) {
		/* array might be sparse */
		if (!txhd2_hw_onecell_data.hws[i])
			continue;

		dev_dbg(dev, "registering %d  %s\n", i,
				txhd2_hw_onecell_data.hws[i]->init->name);

		ret = devm_clk_hw_register(dev, txhd2_hw_onecell_data.hws[i]);
		if (ret) {
			dev_err(dev, "Clock registration failed\n");
			return ret;
		}
#ifdef CONFIG_AMLOGIC_CLK_DEBUG
		ret = devm_clk_hw_register_clkdev(dev, txhd2_hw_onecell_data.hws[i],
					NULL, clk_hw_get_name(txhd2_hw_onecell_data.hws[i]));
		if (ret < 0) {
			dev_err(dev, "Failed to clkdev register: %d\n", ret);
			return ret;
		}
#endif
	}

	meson_txhd2_dvfs_setup(pdev);

	return devm_of_clk_add_hw_provider(dev, of_clk_hw_onecell_get,
					   &txhd2_hw_onecell_data);

	return 0;
}

static const struct of_device_id clkc_match_table[] = {
	{
		.compatible = "amlogic,txhd2-clkc"
	},
	{}
};

static struct platform_driver txhd2_driver = {
	.probe		= meson_txhd2_probe,
	.driver		= {
		.name	= "txhd2-clkc",
		.of_match_table = clkc_match_table,
	},
};

builtin_platform_driver(txhd2_driver);

MODULE_LICENSE("GPL v2");
