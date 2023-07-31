// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/clk-provider.h>
#include <linux/init.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/module.h>
#include <linux/clkdev.h>
#include "clk-mpll.h"
#include "clk-pll.h"
#include "clk-regmap.h"
#include "clk-cpu-dyndiv.h"
#include "vid-pll-div.h"
#include "meson-eeclk.h"
#include "t5d.h"

static DEFINE_SPINLOCK(meson_clk_lock);

static struct clk_regmap t5d_fixed_pll_dco = {
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
		.frac = {
			.reg_off = HHI_FIX_PLL_CNTL1,
			.shift   = 0,
			.width   = 19,
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
	},
};

static struct clk_regmap t5d_fixed_pll = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_FIX_PLL_CNTL0,
		.shift = 16,
		.width = 2,
		.flags = CLK_DIVIDER_POWER_OF_TWO,
	},
	.hw.init = &(struct clk_init_data){
		.name = "fixed_pll",
		.ops = &clk_regmap_divider_ro_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_fixed_pll_dco.hw
		},
		.num_parents = 1,
		/*
		 * This clock won't ever change at runtime so
		 * CLK_SET_RATE_PARENT is not required
		 */
	},
};

static const struct clk_ops meson_pll_clk_no_ops = {};

/*
 * the sys pll DCO value should be 3G~6G,
 * otherwise the sys pll can not lock.
 * od is for 32 bit.
 */

#ifdef CONFIG_ARM
static const struct pll_params_table t5d_sys_pll_params_table[] = {
	PLL_PARAMS(168, 1, 2), /*DCO=4032M OD=1008M*/
	PLL_PARAMS(184, 1, 2), /*DCO=4416M OD=1104M*/
	PLL_PARAMS(200, 1, 2), /*DCO=4800M OD=1200M*/
	PLL_PARAMS(216, 1, 2), /*DCO=5184M OD=1296M*/
	PLL_PARAMS(233, 1, 2), /*DCO=5592M OD=1398M*/
	PLL_PARAMS(249, 1, 2), /*DCO=5976M OD=1494M*/
	PLL_PARAMS(126, 1, 1), /*DCO=3024M OD=1512M*/
	PLL_PARAMS(134, 1, 1), /*DCO=3216M OD=1608M*/
	PLL_PARAMS(142, 1, 1), /*DCO=3408M OD=1704M*/
	PLL_PARAMS(150, 1, 1), /*DCO=3600M OD=1800M*/
	PLL_PARAMS(158, 1, 1), /*DCO=3792M OD=1896M*/
	PLL_PARAMS(159, 1, 1), /*DCO=3816M OD=1908*/
	PLL_PARAMS(160, 1, 1), /*DCO=3840M OD=1920M*/
	PLL_PARAMS(168, 1, 1), /*DCO=4032M OD=2016M*/
	{ /* sentinel */ },
};
#else
static const struct pll_params_table t5d_sys_pll_params_table[] = {
	PLL_PARAMS(168, 1), /*DCO=4032M OD=1008M*/
	PLL_PARAMS(184, 1), /*DCO=4416M OD=1104M*/
	PLL_PARAMS(200, 1), /*DCO=4800M OD=1200M*/
	PLL_PARAMS(216, 1), /*DCO=5184M OD=1296M*/
	PLL_PARAMS(233, 1), /*DCO=5592M OD=1398M*/
	PLL_PARAMS(249, 1), /*DCO=5976M OD=1494M*/
	PLL_PARAMS(126, 1), /*DCO=3024M OD=1512M*/
	PLL_PARAMS(134, 1), /*DCO=3216M OD=1608M*/
	PLL_PARAMS(142, 1), /*DCO=3408M OD=1704M*/
	PLL_PARAMS(150, 1), /*DCO=3600M OD=1800M*/
	PLL_PARAMS(158, 1), /*DCO=3792M OD=1896M*/
	PLL_PARAMS(159, 1), /*DCO=3816M OD=1908*/
	PLL_PARAMS(160, 1), /*DCO=3840M OD=1920M*/
	PLL_PARAMS(168, 1), /*DCO=4032M OD=2016M*/
	{ /* sentinel */ },
};
#endif

static struct clk_regmap t5d_sys_pll_dco = {
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
			.shift   = 10,
			.width   = 5,
		},
#ifdef CONFIG_ARM
		/* od for 32bit */
		.od = {
			.reg_off = HHI_SYS_PLL_CNTL0,
			.shift	 = 16,
			.width	 = 3,
		},
#endif
		.table = t5d_sys_pll_params_table,
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
	},
	.hw.init = &(struct clk_init_data){
		.name = "sys_pll_dco",
		.ops = &meson_clk_pll_ops,
		.parent_data = &(const struct clk_parent_data) {
			.fw_name = "xtal",
		},
		.num_parents = 1,
		/* This clock feeds the CPU, avoid disabling it */
		.flags = CLK_IS_CRITICAL,
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
static struct clk_regmap t5d_sys_pll = {
	.hw.init = &(struct clk_init_data){
		.name = "sys_pll",
		.ops = &meson_pll_clk_no_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_sys_pll_dco.hw
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
static struct clk_regmap t5d_sys_pll = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_SYS_PLL_CNTL0,
		.shift = 16,
		.width = 3,
		.flags = CLK_DIVIDER_POWER_OF_TWO,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sys_pll",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_sys_pll_dco.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};
#endif

static struct clk_fixed_factor t5d_fclk_div2_div = {
	.mult = 1,
	.div = 2,
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div2_div",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) { &t5d_fixed_pll.hw },
		.num_parents = 1,
	},
};

static struct clk_regmap t5d_fclk_div2 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_FIX_PLL_CNTL1,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div2",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_fclk_div2_div.hw
		},
		.num_parents = 1,
		/*
		 * This clock feeds on CPU clock, it should be set
		 * by the platform to operate correctly.
		 */
		.flags = CLK_IS_CRITICAL,
	},
};

static struct clk_fixed_factor t5d_fclk_div3_div = {
	.mult = 1,
	.div = 3,
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div3_div",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) { &t5d_fixed_pll.hw },
		.num_parents = 1,
	},
};

static struct clk_regmap t5d_fclk_div3 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_FIX_PLL_CNTL1,
		.bit_idx = 20,
	},
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div3",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_fclk_div3_div.hw
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

static struct clk_fixed_factor t5d_fclk_div4_div = {
	.mult = 1,
	.div = 4,
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div4_div",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) { &t5d_fixed_pll.hw },
		.num_parents = 1,
	},
};

static struct clk_regmap t5d_fclk_div4 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_FIX_PLL_CNTL1,
		.bit_idx = 21,
	},
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div4",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_fclk_div4_div.hw
		},
		.num_parents = 1,
		/*
		 * This clock feeds on GPU, it should be set
		 * by the platform to operate correctly.
		 */
		.flags = CLK_IS_CRITICAL,
	},
};

static struct clk_fixed_factor t5d_fclk_div5_div = {
	.mult = 1,
	.div = 5,
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div5_div",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) { &t5d_fixed_pll.hw },
		.num_parents = 1,
	},
};

static struct clk_regmap t5d_fclk_div5 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_FIX_PLL_CNTL1,
		.bit_idx = 22,
	},
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div5",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_fclk_div5_div.hw
		},
		.num_parents = 1,
		/*
		 * This clock feeds on GPU, it should be set
		 * by the platform to operate correctly.
		 */
		.flags = CLK_IS_CRITICAL,
	},
};

static struct clk_fixed_factor t5d_fclk_div7_div = {
	.mult = 1,
	.div = 7,
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div7_div",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) { &t5d_fixed_pll.hw },
		.num_parents = 1,
	},
};

static struct clk_regmap t5d_fclk_div7 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_FIX_PLL_CNTL1,
		.bit_idx = 23,
	},
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div7",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_fclk_div7_div.hw
		},
		.num_parents = 1,
		/*
		 * This clock feeds on GPU, it should be set
		 * by the platform to operate correctly.
		 */
		.flags = CLK_IS_CRITICAL,
	},
};

static struct clk_fixed_factor t5d_fclk_div2p5_div = {
	.mult = 1,
	.div = 5,
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div2p5_div",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_fixed_pll_dco.hw
		},
		.num_parents = 1,
	},
};

static struct clk_regmap t5d_fclk_div2p5 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_FIX_PLL_CNTL1,
		.bit_idx = 25,
	},
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div2p5",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_fclk_div2p5_div.hw
		},
		.num_parents = 1,
		/*
		 * This clock feeds on GPU, it should be set
		 * by the platform to operate correctly.
		 */
		.flags = CLK_IS_CRITICAL,
	},
};

#ifdef CONFIG_ARM
static const struct pll_params_table t5d_gp0_pll_table[] = {
	PLL_PARAMS(141, 1, 2), /* DCO = 3384M OD = 2 PLL = 846M */
	PLL_PARAMS(130, 1, 2), /* DCO = 3120M OD = 2 PLL = 780M */
	PLL_PARAMS(132, 1, 2), /* DCO = 3168M OD = 2 PLL = 792M */
	PLL_PARAMS(128, 1, 2), /* DCO = 3072M OD = 2 PLL = 768M */
	PLL_PARAMS(248, 1, 3), /* DCO = 5952M OD = 3 PLL = 744M */
	{ /* sentinel */  },
};
#else
static const struct pll_params_table t5d_gp0_pll_table[] = {
	PLL_PARAMS(141, 1), /* DCO = 3384M OD = 2 PLL = 846M*/
	PLL_PARAMS(130, 1), /* DCO = 3120M OD = 2 PLL = 780M */
	PLL_PARAMS(132, 1), /* DCO = 3168M OD = 2 PLL = 792M */
	PLL_PARAMS(128, 1), /* DCO = 3072M OD = 2 PLL = 768M */
	PLL_PARAMS(248, 1), /* DCO = 5952M OD = 3 PLL = 744M */
	{0, 0},
};
#endif

/*
 * Internal gp0 pll emulation configuration parameters
 */
static const struct reg_sequence t5d_gp0_init_regs[] = {
	{ .reg = HHI_GP0_PLL_CNTL1,	.def = 0x00000000 },
	{ .reg = HHI_GP0_PLL_CNTL2,	.def = 0x00000180 },
	{ .reg = HHI_GP0_PLL_CNTL3,	.def = 0x4a681c00 },
	{ .reg = HHI_GP0_PLL_CNTL4,	.def = 0x33771290 },
	{ .reg = HHI_GP0_PLL_CNTL5,	.def = 0x39272000 },
	{ .reg = HHI_GP0_PLL_CNTL6,	.def = 0x56540000 },
};

static struct clk_regmap t5d_gp0_pll_dco = {
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
			.shift   = 10,
			.width   = 5,
		},
#ifdef CONFIG_ARM
		/* for 32bit */
		.od = {
			.reg_off = HHI_GP0_PLL_CNTL0,
			.shift	 = 16,
			.width	 = 3,
		},
#endif
		.frac = {
			.reg_off = HHI_GP0_PLL_CNTL1,
			.shift   = 0,
			.width   = 19,
		},
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
		.table = t5d_gp0_pll_table,
		.init_regs = t5d_gp0_init_regs,
		.init_count = ARRAY_SIZE(t5d_gp0_init_regs),
	},
	.hw.init = &(struct clk_init_data){
		.name = "gp0_pll_dco",
		.ops = &meson_clk_pll_ops,
		.parent_data = &(const struct clk_parent_data) {
			.fw_name = "xtal",
		},
		.num_parents = 1,
	},
};

#ifdef CONFIG_ARM
static struct clk_regmap t5d_gp0_pll = {
	.hw.init = &(struct clk_init_data){
		.name = "gp0_pll",
		.ops = &meson_pll_clk_no_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_gp0_pll_dco.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};
#else
static struct clk_regmap t5d_gp0_pll = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_GP0_PLL_CNTL0,
		.shift = 16,
		.width = 3,
		.flags = (CLK_DIVIDER_POWER_OF_TWO |
			  CLK_DIVIDER_ROUND_CLOSEST),
	},
	.hw.init = &(struct clk_init_data){
		.name = "gp0_pll",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_gp0_pll_dco.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};
#endif

#ifdef CONFIG_ARM
static const struct pll_params_table t5w_hifi_pll_table[] = {
	PLL_PARAMS(163, 1, 3), /* DCO = 3932.16M */
	{ /* sentinel */  }
};
#else
static const struct pll_mult_range t5d_hifi_pll_mult_range = {
	.min = 128,
	.max = 250,
};
#endif
/*
 * Internal hifi pll emulation configuration parameters
 */
static const struct reg_sequence t5d_hifi_init_regs[] = {
	{ .reg = HHI_HIFI_PLL_CNTL0,	.def = 0x0803047d },
	{ .reg = HHI_HIFI_PLL_CNTL0,	.def = 0x3803047d },
	{ .reg = HHI_HIFI_PLL_CNTL1,	.def = 0x00014820 },
	{ .reg = HHI_HIFI_PLL_CNTL2,	.def = 0x00000000 },
	{ .reg = HHI_HIFI_PLL_CNTL3,	.def = 0x6a285c00 },
	{ .reg = HHI_HIFI_PLL_CNTL4,	.def = 0x65771290 },
	{ .reg = HHI_HIFI_PLL_CNTL5,	.def = 0x39272000 },
	{ .reg = HHI_HIFI_PLL_CNTL6,	.def = 0x56540000 },
	{ .reg = HHI_HIFI_PLL_CNTL0,	.def = 0x1803047d },
};

static struct clk_regmap t5d_hifi_pll_dco = {
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
			.shift   = 10,
			.width   = 5,
		},
		.frac = {
			.reg_off = HHI_HIFI_PLL_CNTL1,
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
#ifdef CONFIG_ARM
		.od = {
			.reg_off = HHI_HIFI_PLL_CNTL0,
			.shift	 = 16,
			.width	 = 2,
		},
		.table = t5w_hifi_pll_table,
#else
		.range = &t5d_hifi_pll_mult_range,
#endif
		.init_regs = t5d_hifi_init_regs,
		.init_count = ARRAY_SIZE(t5d_hifi_init_regs),
		.flags = CLK_MESON_PLL_ROUND_CLOSEST |
			 CLK_MESON_PLL_FIXED_FRAC_WEIGHT_PRECISION
	},
	.hw.init = &(struct clk_init_data){
		.name = "hifi_pll_dco",
		.ops = &meson_clk_pll_ops,
		.parent_data = &(const struct clk_parent_data) {
			.fw_name = "xtal",
		},
		.num_parents = 1,
	},
};

#ifdef CONFIG_ARM
static struct clk_regmap t5d_hifi_pll = {
	.hw.init = &(struct clk_init_data){
		.name = "hifi_pll",
		.ops = &meson_pll_clk_no_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_hifi_pll_dco.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};
#else
static struct clk_regmap t5d_hifi_pll = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_HIFI_PLL_CNTL0,
		.shift = 16,
		.width = 2,
		.flags = (CLK_DIVIDER_POWER_OF_TWO |
			  CLK_DIVIDER_ROUND_CLOSEST),
	},
	.hw.init = &(struct clk_init_data){
		.name = "hifi_pll",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_hifi_pll_dco.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};
#endif

static struct clk_fixed_factor t5d_mpll_50m_div = {
	.mult = 1,
	.div = 80,
	.hw.init = &(struct clk_init_data){
		.name = "mpll_50m_div",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_fixed_pll_dco.hw
		},
		.num_parents = 1,
	},
};

static struct clk_regmap t5d_mpll_50m = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_FIX_PLL_CNTL3,
		.mask = 0x1,
		.shift = 5,
	},
	.hw.init = &(struct clk_init_data){
		.name = "mpll_50m",
		.ops = &clk_regmap_mux_ro_ops,
		.parent_data = (const struct clk_parent_data []) {
			{ .fw_name = "xtal", },
			{ .hw = &t5d_mpll_50m_div.hw },
		},
		.num_parents = 2,
	},
};

static struct clk_fixed_factor t5d_mpll_prediv = {
	.mult = 1,
	.div = 2,
	.hw.init = &(struct clk_init_data){
		.name = "mpll_prediv",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_fixed_pll_dco.hw
		},
		.num_parents = 1,
	},
};

/*
 * put it in init regs, in kernel 4.9 it is dealing in set rate
 */
static const struct reg_sequence t5d_mpll0_init_regs[] = {
	{ .reg = HHI_MPLL_CNTL0,	.def = 0x00000543 },
	{ .reg = HHI_MPLL_CNTL2,	.def = 0x40000033 },
};

static struct clk_regmap t5d_mpll0_div = {
	.data = &(struct meson_clk_mpll_data){
		.sdm = {
			.reg_off = HHI_MPLL_CNTL1,
			.shift   = 0,
			.width   = 14,
		},
		.sdm_en = {
			.reg_off = HHI_MPLL_CNTL1,
			.shift   = 30,
			.width	 = 1,
		},
		.n2 = {
			.reg_off = HHI_MPLL_CNTL1,
			.shift   = 20,
			.width   = 9,
		},
		.ssen = {
			.reg_off = HHI_MPLL_CNTL1,
			.shift   = 29,
			.width	 = 1,
		},
		.lock = &meson_clk_lock,
		.init_regs = t5d_mpll0_init_regs,
		.init_count = ARRAY_SIZE(t5d_mpll0_init_regs),
	},
	.hw.init = &(struct clk_init_data){
		.name = "mpll0_div",
		.ops = &meson_clk_mpll_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_mpll_prediv.hw
		},
		.num_parents = 1,
	},
};

static struct clk_regmap t5d_mpll0 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_MPLL_CNTL1,
		.bit_idx = 31,
	},
	.hw.init = &(struct clk_init_data){
		.name = "mpll0",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) { &t5d_mpll0_div.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static const struct reg_sequence t5d_mpll1_init_regs[] = {
	{ .reg = HHI_MPLL_CNTL4,	.def = 0x40000033 },
};

static struct clk_regmap t5d_mpll1_div = {
	.data = &(struct meson_clk_mpll_data){
		.sdm = {
			.reg_off = HHI_MPLL_CNTL3,
			.shift   = 0,
			.width   = 14,
		},
		.sdm_en = {
			.reg_off = HHI_MPLL_CNTL3,
			.shift   = 30,
			.width	 = 1,
		},
		.n2 = {
			.reg_off = HHI_MPLL_CNTL3,
			.shift   = 20,
			.width   = 9,
		},
		.ssen = {
			.reg_off = HHI_MPLL_CNTL3,
			.shift   = 29,
			.width	 = 1,
		},
		.lock = &meson_clk_lock,
		.init_regs = t5d_mpll1_init_regs,
		.init_count = ARRAY_SIZE(t5d_mpll1_init_regs),
	},
	.hw.init = &(struct clk_init_data){
		.name = "mpll1_div",
		.ops = &meson_clk_mpll_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_mpll_prediv.hw
		},
		.num_parents = 1,
	},
};

static struct clk_regmap t5d_mpll1 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_MPLL_CNTL3,
		.bit_idx = 31,
	},
	.hw.init = &(struct clk_init_data){
		.name = "mpll1",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) { &t5d_mpll1_div.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static const struct reg_sequence t5d_mpll2_init_regs[] = {
	{ .reg = HHI_MPLL_CNTL6,	.def = 0x40000033 },
};

static struct clk_regmap t5d_mpll2_div = {
	.data = &(struct meson_clk_mpll_data){
		.sdm = {
			.reg_off = HHI_MPLL_CNTL5,
			.shift   = 0,
			.width   = 14,
		},
		.sdm_en = {
			.reg_off = HHI_MPLL_CNTL5,
			.shift   = 30,
			.width	 = 1,
		},
		.n2 = {
			.reg_off = HHI_MPLL_CNTL5,
			.shift   = 20,
			.width   = 9,
		},
		.ssen = {
			.reg_off = HHI_MPLL_CNTL5,
			.shift   = 29,
			.width	 = 1,
		},
		.lock = &meson_clk_lock,
		.init_regs = t5d_mpll2_init_regs,
		.init_count = ARRAY_SIZE(t5d_mpll2_init_regs),
	},
	.hw.init = &(struct clk_init_data){
		.name = "mpll2_div",
		.ops = &meson_clk_mpll_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_mpll_prediv.hw
		},
		.num_parents = 1,
	},
};

static struct clk_regmap t5d_mpll2 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_MPLL_CNTL5,
		.bit_idx = 31,
	},
	.hw.init = &(struct clk_init_data){
		.name = "mpll2",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) { &t5d_mpll2_div.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static const struct reg_sequence t5d_mpll3_init_regs[] = {
	{ .reg = HHI_MPLL_CNTL8,	.def = 0x40000033 },
};

static struct clk_regmap t5d_mpll3_div = {
	.data = &(struct meson_clk_mpll_data){
		.sdm = {
			.reg_off = HHI_MPLL_CNTL7,
			.shift   = 0,
			.width   = 14,
		},
		.sdm_en = {
			.reg_off = HHI_MPLL_CNTL7,
			.shift   = 30,
			.width	 = 1,
		},
		.n2 = {
			.reg_off = HHI_MPLL_CNTL7,
			.shift   = 20,
			.width   = 9,
		},
		.ssen = {
			.reg_off = HHI_MPLL_CNTL7,
			.shift   = 29,
			.width	 = 1,
		},
		.lock = &meson_clk_lock,
		.init_regs = t5d_mpll3_init_regs,
		.init_count = ARRAY_SIZE(t5d_mpll3_init_regs),
	},
	.hw.init = &(struct clk_init_data){
		.name = "mpll3_div",
		.ops = &meson_clk_mpll_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_mpll_prediv.hw
		},
		.num_parents = 1,
	},
};

static struct clk_regmap t5d_mpll3 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_MPLL_CNTL7,
		.bit_idx = 31,
	},
	.hw.init = &(struct clk_init_data){
		.name = "mpll3",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) { &t5d_mpll3_div.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static const struct cpu_dyn_table t5d_cpu_dyn_table[] = {
	CPU_LOW_PARAMS(100000000, 1, 1, 9),
	CPU_LOW_PARAMS(250000000, 1, 1, 3),
	CPU_LOW_PARAMS(333333333, 2, 1, 1),
	CPU_LOW_PARAMS(500000000, 1, 1, 1),
	CPU_LOW_PARAMS(666666666, 2, 0, 0),
	CPU_LOW_PARAMS(1000000000, 1, 0, 0)
};

static const struct clk_parent_data t5d_cpu_dyn_clk_sel[] = {
	{ .fw_name = "xtal", },
	{ .hw = &t5d_fclk_div2.hw },
	{ .hw = &t5d_fclk_div3.hw }
};

static struct clk_regmap t5d_cpu_dyn_clk = {
	.data = &(struct meson_clk_cpu_dyn_data){
		.table = t5d_cpu_dyn_table,
		.table_cnt = ARRAY_SIZE(t5d_cpu_dyn_table),
		.offset = HHI_SYS_CPU_CLK_CNTL0,
	},
	.hw.init = &(struct clk_init_data){
		.name = "cpu_dyn_clk",
		.ops = &meson_clk_cpu_dyn_ops,
		.parent_data = t5d_cpu_dyn_clk_sel,
		.num_parents = ARRAY_SIZE(t5d_cpu_dyn_clk_sel),
	},
};

static struct clk_regmap t5d_cpu_clk = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_SYS_CPU_CLK_CNTL0,
		.mask = 0x1,
		.shift = 11,
		.flags = CLK_MUX_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "cpu_clk",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_cpu_dyn_clk.hw,
			&t5d_sys_pll.hw,
		},
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static u32 mux_table_clk81[]	= { 6, 5, 7 };

static const struct clk_hw *clk81_parent_hws[] = {
	&t5d_fclk_div3.hw,
	&t5d_fclk_div4.hw,
	&t5d_fclk_div5.hw,
};

static struct clk_regmap t5d_mpeg_clk_sel = {
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

static struct clk_regmap t5d_mpeg_clk_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_MPEG_CLK_CNTL,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "mpeg_clk_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_mpeg_clk_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t5d_clk81 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_MPEG_CLK_CNTL,
		.bit_idx = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "clk81",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_mpeg_clk_div.hw
		},
		.num_parents = 1,
		.flags = (CLK_SET_RATE_PARENT | CLK_IS_CRITICAL),
	},
};

static struct clk_regmap t5d_ts_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_TS_CLK_CNTL,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "ts_div",
		.ops = &clk_regmap_divider_ops,
		.parent_data = &(const struct clk_parent_data) {
			.fw_name = "xtal",
		},
		.num_parents = 1,
	},
};

static struct clk_regmap t5d_ts = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_TS_CLK_CNTL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "ts",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_ts_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

const char *t5d_gpu_parent_names[] = { "xtal", "gp0_pll", "hifi_pll",
	"fclk_div2p5", "fclk_div3", "fclk_div4", "fclk_div5", "fclk_div7"};

static struct clk_regmap t5d_gpu_p0_mux = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_MALI_CLK_CNTL,
		.mask = 0x7,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "gpu_p0_mux",
		.ops = &clk_regmap_mux_ops,
		.parent_names = t5d_gpu_parent_names,
		.num_parents = 8,
	},
};

static struct clk_regmap t5d_gpu_p0_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_MALI_CLK_CNTL,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "gpu_p0_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_gpu_p0_mux.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t5d_gpu_p0_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_MALI_CLK_CNTL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "gpu_p0_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_gpu_p0_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_GATE | CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t5d_gpu_p1_mux = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_MALI_CLK_CNTL,
		.mask = 0x7,
		.shift = 25,
	},
	.hw.init = &(struct clk_init_data){
		.name = "gpu_p1_mux",
		.ops = &clk_regmap_mux_ops,
		.parent_names = t5d_gpu_parent_names,
		.num_parents = 8,
	},
};

static struct clk_regmap t5d_gpu_p1_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_MALI_CLK_CNTL,
		.shift = 16,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "gpu_p1_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_gpu_p1_mux.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t5d_gpu_p1_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_MALI_CLK_CNTL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "gpu_p1_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_gpu_p1_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_GATE | CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t5d_gpu_mux = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_MALI_CLK_CNTL,
		.mask = 0x1,
		.shift = 31,
	},
	.hw.init = &(struct clk_init_data){
		.name = "gpu_mux",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_gpu_p0_gate.hw,
			&t5d_gpu_p1_gate.hw
		},
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT
	},
};

static const char * const t5d_vpu_parent_names[] = { "fclk_div3",
	"fclk_div4", "fclk_div5", "fclk_div7", "null", "null",
	"null",  "null"};

static struct clk_regmap t5d_vpu_p0_mux = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_VPU_CLK_CNTL,
		.mask = 0x7,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vpu_p0_mux",
		.ops = &clk_regmap_mux_ops,
		.parent_names = t5d_vpu_parent_names,
		.num_parents = 8,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap t5d_vpu_p0_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_VPU_CLK_CNTL,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vpu_p0_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_vpu_p0_mux.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t5d_vpu_p0_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_VPU_CLK_CNTL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vpu_p0_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_vpu_p0_div.hw
		},
		.num_parents = 1,
		/*
		 * vpu clk is used for display module, vpu driver is a KO, It is too late
		 * to enable to clk again. add CLK_IGNORE_UNUSED to avoid display abnormal
		 */
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap t5d_vpu_p1_mux = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_VPU_CLK_CNTL,
		.mask = 0x7,
		.shift = 25,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vpu_p1_mux",
		.ops = &clk_regmap_mux_ops,
		.parent_names = t5d_vpu_parent_names,
		.num_parents = 8,
	},
};

static struct clk_regmap t5d_vpu_p1_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_VPU_CLK_CNTL,
		.shift = 16,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vpu_p1_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_vpu_p1_mux.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t5d_vpu_p1_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_VPU_CLK_CNTL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vpu_p1_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_vpu_p1_div.hw
		},
		.num_parents = 1,
		/*
		 * vpu clk p1 may be used for display module, vpu driver is a KO,
		 * It is too late to enable to clk again. add CLK_IGNORE_UNUSED
		 * to avoid display abnormal.
		 */
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED
	},
};

static struct clk_regmap t5d_vpu_mux = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_VPU_CLK_CNTL,
		.mask = 0x1,
		.shift = 31,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vpu_mux",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_vpu_p0_gate.hw,
			&t5d_vpu_p1_gate.hw
		},
		.num_parents = 2,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

/* vpu clkc clock, different with vpu clock */
static const char * const t5d_vpu_clkc_parent_names[] = { "fclk_div4",
	"fclk_div3", "fclk_div5", "fclk_div7", "null", "null",
	"null",  "null"};

static struct clk_regmap t5d_vpu_clkc_p0_mux = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_VPU_CLKC_CNTL,
		.mask = 0x7,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vpu_clkc_p0_mux",
		.ops = &clk_regmap_mux_ops,
		.parent_names = t5d_vpu_clkc_parent_names,
		.num_parents = 8,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap t5d_vpu_clkc_p0_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_VPU_CLKC_CNTL,
		.shift = 0,
		.width = 7,
		.flags = CLK_DIVIDER_ALLOW_ZERO
	},
	.hw.init = &(struct clk_init_data){
		.name = "vpu_clkc_p0_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_vpu_clkc_p0_mux.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t5d_vpu_clkc_p0_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_VPU_CLKC_CNTL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vpu_clkc_p0_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_vpu_clkc_p0_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED
	},
};

static struct clk_regmap t5d_vpu_clkc_p1_mux = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_VPU_CLKC_CNTL,
		.mask = 0x7,
		.shift = 25,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vpu_clkc_p1_mux",
		.ops = &clk_regmap_mux_ops,
		.parent_names = t5d_vpu_clkc_parent_names,
		.num_parents = 8,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap t5d_vpu_clkc_p1_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_VPU_CLKC_CNTL,
		.shift = 16,
		.width = 7,
		.flags = CLK_DIVIDER_ALLOW_ZERO
	},
	.hw.init = &(struct clk_init_data){
		.name = "vpu_clkc_p1_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_vpu_clkc_p1_mux.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t5d_vpu_clkc_p1_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_VPU_CLKC_CNTL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vpu_clkc_p1_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_vpu_clkc_p1_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED
	},
};

static struct clk_regmap t5d_vpu_clkc_mux = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_VPU_CLKC_CNTL,
		.mask = 0x1,
		.shift = 31,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vpu_clkc_mux",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_vpu_clkc_p0_gate.hw,
			&t5d_vpu_clkc_p1_gate.hw
		},
		.num_parents = 2,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static const char * const t5d_adc_extclk_in_parent_names[] = { "xtal",
	"fclk_div4", "fclk_div3", "fclk_div5",
	"fclk_div7", "mpll2", "gp0_pll", "hifi_pll" };

static struct clk_regmap t5d_adc_extclk_in_mux = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_DEMOD_CLK_CNTL,
		.mask = 0x7,
		.shift = 25,
	},
	.hw.init = &(struct clk_init_data){
		.name = "adc_extclk_in_mux",
		.ops = &clk_regmap_mux_ops,
		.parent_names = t5d_adc_extclk_in_parent_names,
		.num_parents = ARRAY_SIZE(t5d_adc_extclk_in_parent_names),
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap t5d_adc_extclk_in_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_DEMOD_CLK_CNTL,
		.shift = 16,
		.width = 7,
		.flags = CLK_DIVIDER_ALLOW_ZERO
	},
	.hw.init = &(struct clk_init_data){
		.name = "adc_extclk_in_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_adc_extclk_in_mux.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t5d_adc_extclk_in = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_DEMOD_CLK_CNTL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "adc_extclk_in",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_adc_extclk_in_div.hw
		},
		.num_parents = 1,
		/* delete CLK_IGNORE_UNUSED in real chip */
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static const char * const t5d_demod_parent_names[] = { "xtal",
	"fclk_div7", "fclk_div4", "adc_extclk_in" };

static struct clk_regmap t5d_demod_core_clk_mux = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_DEMOD_CLK_CNTL,
		.mask = 0x3,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "demod_core_clk_mux",
		.ops = &clk_regmap_mux_ops,
		.parent_names = t5d_demod_parent_names,
		.num_parents = 4,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap t5d_demod_core_clk_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_DEMOD_CLK_CNTL,
		.shift = 0,
		.width = 7,
		.flags = CLK_DIVIDER_ALLOW_ZERO
	},
	.hw.init = &(struct clk_init_data){
		.name = "demod_core_clk_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_demod_core_clk_mux.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t5d_demod_core_clk = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_DEMOD_CLK_CNTL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "demod_core_clk",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_demod_core_clk_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT
	},
};

const char *t5d_dec_parent_names[] = { "fclk_div2p5", "fclk_div3",
	"fclk_div4", "fclk_div5", "fclk_div7", "hifi_pll", "gp0_pll", "xtal"};

static struct clk_regmap t5d_vdec_p0_mux = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_VDEC_CLK_CNTL,
		.mask = 0x7,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vdec_p0_mux",
		.ops = &clk_regmap_mux_ops,
		.parent_names = t5d_dec_parent_names,
		.num_parents = 8,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap t5d_vdec_p0_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_VDEC_CLK_CNTL,
		.shift = 0,
		.width = 7,
		.flags = CLK_DIVIDER_ALLOW_ZERO
	},
	.hw.init = &(struct clk_init_data){
		.name = "vdec_p0_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_vdec_p0_mux.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t5d_vdec_p0_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_VDEC_CLK_CNTL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vdec_p0_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_vdec_p0_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t5d_hevcf_p0_mux = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_VDEC2_CLK_CNTL,
		.mask = 0x7,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hevcf_p0_mux",
		.ops = &clk_regmap_mux_ops,
		.parent_names = t5d_dec_parent_names,
		.num_parents = 8,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap t5d_hevcf_p0_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_VDEC2_CLK_CNTL,
		.shift = 0,
		.width = 7,
		.flags = CLK_DIVIDER_ALLOW_ZERO
	},
	.hw.init = &(struct clk_init_data){
		.name = "hevcf_p0_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_hevcf_p0_mux.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t5d_hevcf_p0_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_VDEC2_CLK_CNTL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hevcf_p0_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_hevcf_p0_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t5d_vdec_p1_mux = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_VDEC3_CLK_CNTL,
		.mask = 0x7,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vdec_p1_mux",
		.ops = &clk_regmap_mux_ops,
		.parent_names = t5d_dec_parent_names,
		.num_parents = 8,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap t5d_vdec_p1_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_VDEC3_CLK_CNTL,
		.shift = 0,
		.width = 7,
		.flags = CLK_DIVIDER_ALLOW_ZERO
	},
	.hw.init = &(struct clk_init_data){
		.name = "vdec_p1_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_vdec_p1_mux.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t5d_vdec_p1_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_VDEC3_CLK_CNTL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vdec_p1_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_vdec_p1_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t5d_vdec_mux = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_VDEC3_CLK_CNTL,
		.mask = 0x1,
		.shift = 15,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vdec_mux",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_vdec_p0_gate.hw,
			&t5d_vdec_p1_gate.hw
		},
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap t5d_hevcf_p1_mux = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_VDEC4_CLK_CNTL,
		.mask = 0x7,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hevcf_p1_mux",
		.ops = &clk_regmap_mux_ops,
		.parent_names = t5d_dec_parent_names,
		.num_parents = 8,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap t5d_hevcf_p1_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_VDEC4_CLK_CNTL,
		.shift = 0,
		.width = 7,
		.flags = CLK_DIVIDER_ALLOW_ZERO
	},
	.hw.init = &(struct clk_init_data){
		.name = "hevcf_p1_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_hevcf_p1_mux.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t5d_hevcf_p1_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_VDEC4_CLK_CNTL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hevcf_p1_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_hevcf_p1_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t5d_hevcf_mux = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_VDEC4_CLK_CNTL,
		.mask = 0x1,
		.shift = 15,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hevcf_mux",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_hevcf_p0_gate.hw,
			&t5d_hevcf_p0_gate.hw
		},
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

static const char * const t5d_hdcp22_esm_parent_names[] = { "fclk_div7",
	"fclk_div4", "fclk_div3", "fclk_div5" };

static struct clk_regmap t5d_hdcp22_esm_mux = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_HDCP22_CLK_CNTL,
		.mask = 0x3,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdcp22_esm_mux",
		.ops = &clk_regmap_mux_ops,
		.parent_names = t5d_hdcp22_esm_parent_names,
		.num_parents = 4,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap t5d_hdcp22_esm_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_HDCP22_CLK_CNTL,
		.shift = 0,
		.width = 7,
		.flags = CLK_DIVIDER_ALLOW_ZERO
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdcp22_esm_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_hdcp22_esm_mux.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t5d_hdcp22_esm_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_HDCP22_CLK_CNTL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hdcp22_esm_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_hdcp22_esm_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT
	},
};

static const char * const t5d_hdcp22_skp_parent_names[] = { "xtal",
	"fclk_div4", "fclk_div3", "fclk_div5" };

static struct clk_regmap t5d_hdcp22_skp_mux = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_HDCP22_CLK_CNTL,
		.mask = 0x3,
		.shift = 25,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdcp22_skp_mux",
		.ops = &clk_regmap_mux_ops,
		.parent_names = t5d_hdcp22_skp_parent_names,
		.num_parents = 4,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap t5d_hdcp22_skp_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_HDCP22_CLK_CNTL,
		.shift = 16,
		.width = 7,
		.flags = CLK_DIVIDER_ALLOW_ZERO
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdcp22_skp_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_hdcp22_skp_mux.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t5d_hdcp22_skp_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_HDCP22_CLK_CNTL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hdcp22_skp_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_hdcp22_skp_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT
	},
};

static const char * const t5d_vapb_parent_names[] = { "fclk_div4",
	"fclk_div3", "fclk_div5", "fclk_div7", "mpll1", "null",
	"t5d_mpll2",  "fclk_div2p5"};

static struct clk_regmap t5d_vapb_p0_mux = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_VAPBCLK_CNTL,
		.mask = 0x7,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vapb_p0_mux",
		.ops = &clk_regmap_mux_ops,
		.parent_names = t5d_vapb_parent_names,
		.num_parents = 8,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap t5d_vapb_p0_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_VAPBCLK_CNTL,
		.shift = 0,
		.width = 7,
		.flags = CLK_DIVIDER_ALLOW_ZERO
	},
	.hw.init = &(struct clk_init_data){
		.name = "vapb_p0_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_vapb_p0_mux.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t5d_vapb_p0_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_VAPBCLK_CNTL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vapb_p0_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_vapb_p0_div.hw
		},
		.num_parents = 1,
		/*
		 * vapb clk is used for display module, vpu driver is a KO, It is too late
		 * to enable to clk again. add CLK_IGNORE_UNUSED to avoid display abnormal
		 */
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED
	},
};

static struct clk_regmap t5d_vapb_p1_mux = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_VAPBCLK_CNTL,
		.mask = 0x7,
		.shift = 25,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vapb_p1_mux",
		.ops = &clk_regmap_mux_ops,
		.parent_names = t5d_vapb_parent_names,
		.num_parents = 8,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap t5d_vapb_p1_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_VAPBCLK_CNTL,
		.shift = 16,
		.width = 7,
		.flags = CLK_DIVIDER_ALLOW_ZERO
	},
	.hw.init = &(struct clk_init_data){
		.name = "vapb_p1_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_vapb_p1_mux.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t5d_vapb_p1_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_VAPBCLK_CNTL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vapb_p1_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_vapb_p1_div.hw
		},
		.num_parents = 1,
		/*
		 * vapb clk p1 may be used for display module, vpu driver is a KO,
		 * It is too late to enable to clk again. add CLK_IGNORE_UNUSED
		 * to avoid display abnormal
		 */
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED
	},
};

static struct clk_regmap t5d_vapb_mux = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_VAPBCLK_CNTL,
		.mask = 0x1,
		.shift = 31,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vapb_mux",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_vapb_p0_gate.hw,
			&t5d_vapb_p1_gate.hw
		},
		.num_parents = 2,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t5d_ge2d_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_VAPBCLK_CNTL,
		.bit_idx = 30,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "ge2d_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_vapb_mux.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static const char * const t5d_hdmirx_parent_names[] = { "xtal",
	"fclk_div4", "fclk_div3", "fclk_div5" };

static struct clk_regmap t5d_hdmirx_cfg_mux = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_HDMIRX_CLK_CNTL,
		.mask = 0x3,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdmirx_cfg_mux",
		.ops = &clk_regmap_mux_ops,
		.parent_names = t5d_hdmirx_parent_names,
		.num_parents = 4,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap t5d_hdmirx_cfg_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_HDMIRX_CLK_CNTL,
		.shift = 0,
		.width = 7,
		.flags = CLK_DIVIDER_ALLOW_ZERO
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdmirx_cfg_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_hdmirx_cfg_mux.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t5d_hdmirx_cfg_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_HDMIRX_CLK_CNTL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hdmirx_cfg_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_hdmirx_cfg_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t5d_hdmirx_modet_mux = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_HDMIRX_CLK_CNTL,
		.mask = 0x3,
		.shift = 25,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdmirx_modet_mux",
		.ops = &clk_regmap_mux_ops,
		.parent_names = t5d_hdmirx_parent_names,
		.num_parents = 4,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap t5d_hdmirx_modet_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_HDMIRX_CLK_CNTL,
		.shift = 16,
		.width = 7,
		.flags = CLK_DIVIDER_ALLOW_ZERO
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdmirx_modet_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_hdmirx_modet_mux.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t5d_hdmirx_modet_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_HDMIRX_CLK_CNTL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hdmirx_modet_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_hdmirx_modet_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT
	},
};

static const char * const t5d_hdmirx_acr_parent_names[] = { "fclk_div4",
	"fclk_div3", "fclk_div5", "fclk_div7" };

static struct clk_regmap t5d_hdmirx_acr_mux = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_HDMIRX_AUD_CLK_CNTL,
		.mask = 0x3,
		.shift = 25,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdmirx_acr_mux",
		.ops = &clk_regmap_mux_ops,
		.parent_names = t5d_hdmirx_acr_parent_names,
		.num_parents = 4,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap t5d_hdmirx_acr_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_HDMIRX_AUD_CLK_CNTL,
		.shift = 16,
		.width = 7,
		.flags = CLK_DIVIDER_ALLOW_ZERO
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdmirx_acr_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_hdmirx_acr_mux.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t5d_hdmirx_acr_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_HDMIRX_AUD_CLK_CNTL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hdmirx_acr_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_hdmirx_acr_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t5d_hdmirx_meter_mux = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_HDMIRX_METER_CLK_CNTL,
		.mask = 0x3,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdmirx_meter_mux",
		.ops = &clk_regmap_mux_ops,
		.parent_names = t5d_hdmirx_parent_names,
		.num_parents = 4,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap t5d_hdmirx_meter_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_HDMIRX_METER_CLK_CNTL,
		.shift = 0,
		.width = 7,
		.flags = CLK_DIVIDER_ALLOW_ZERO
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdmirx_meter_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_hdmirx_meter_mux.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t5d_hdmirx_meter_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_HDMIRX_METER_CLK_CNTL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hdmirx_meter_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_hdmirx_meter_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT
	},
};

static const char * const t5d_vdin_meas_parent_names[] = { "xtal", "fclk_div4",
	"fclk_div3", "fclk_div5" };

static u32 mux_table_vdin_meas[] = { 0, 1, 2, 3 };

static struct clk_regmap t5d_vdin_meas_mux = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_VDIN_MEAS_CLK_CNTL,
		.mask = 0x7,
		.shift = 9,
		.table = mux_table_vdin_meas
	},
	.hw.init = &(struct clk_init_data){
		.name = "vdin_meas_mux",
		.ops = &clk_regmap_mux_ops,
		.parent_names = t5d_vdin_meas_parent_names,
		.num_parents = 4,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap t5d_vdin_meas_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_VDIN_MEAS_CLK_CNTL,
		.shift = 0,
		.width = 7,
		.flags = CLK_DIVIDER_ALLOW_ZERO
	},
	.hw.init = &(struct clk_init_data){
		.name = "vdin_meas_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_vdin_meas_mux.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t5d_vdin_meas_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_VDIN_MEAS_CLK_CNTL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vdin_meas_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_vdin_meas_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT
	},
};

static const char * const sd_emmc_parent_names[] = { "xtal", "fclk_div2",
	"fclk_div3", "fclk_div5", "fclk_div2p5", "mpll2", "mpll3", "gp0_pll" };

static struct clk_regmap t5d_sd_emmc_c_mux = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_NAND_CLK_CNTL,
		.mask = 0x7,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sd_emmc_mux_c",
		.ops = &clk_regmap_mux_ops,
		.parent_names = sd_emmc_parent_names,
		.num_parents = 8,
		.flags = (CLK_GET_RATE_NOCACHE),
	},
};

static struct clk_regmap t5d_sd_emmc_c_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_NAND_CLK_CNTL,
		.shift = 0,
		.width = 7,
		.flags = CLK_DIVIDER_ALLOW_ZERO
	},
	.hw.init = &(struct clk_init_data){
		.name = "sd_emmc_div_c",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_sd_emmc_c_mux.hw
		},
		.num_parents = 1,
		//.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t5d_sd_emmc_c_gate  = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_NAND_CLK_CNTL,
		.bit_idx = 7,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "sd_emmc_gate_c",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_sd_emmc_c_div.hw
		},
		.num_parents = 1,
		//.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT
	},
};

static const char * const spicc_parent_names[] = { "xtal",
	"clk81", "fclk_div4", "fclk_div3", "fclk_div2", "fclk_div5",
	"fclk_div7"};

static struct clk_regmap t5d_spicc0_mux = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_SPICC_CLK_CNTL,
		.mask = 0x7,
		.shift = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "spicc0_mux",
		.ops = &clk_regmap_mux_ops,
		.parent_names = spicc_parent_names,
		.num_parents = 7,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap t5d_spicc0_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_SPICC_CLK_CNTL,
		.shift = 0,
		.width = 6,
		.flags = CLK_DIVIDER_ALLOW_ZERO
	},
	.hw.init = &(struct clk_init_data){
		.name = "spicc0_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_spicc0_mux.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t5d_spicc0_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_SPICC_CLK_CNTL,
		.bit_idx = 6,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "spicc0_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_spicc0_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t5d_vdac_clkc_mux = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_CDAC_CLK_CNTL,
		.mask = 0x1,
		.shift = 16,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vdac_clkc_mux",
		.ops = &clk_regmap_mux_ops,
		.parent_names = (const char *[]){ "xtal", "fclk_div5" },
		.num_parents = 2,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap t5d_vdac_clkc_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_CDAC_CLK_CNTL,
		.shift = 0,
		.width = 16,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vdac_clkc_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_vdac_clkc_mux.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t5d_vdac_clkc_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_CDAC_CLK_CNTL,
		.bit_idx = 20,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vdac_clkc_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_vdac_clkc_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT
	},
};

/*
 * T5 : the last clk source is fclk_div7
 * T5D: the last clk source is fclk_div3.
 * And the last clk source is never used in T5, put fclk_div3 here for T5D
 */
static const char * const t5d_vpu_clkb_parent_names[] = { "vpu_mux", "fclk_div4",
				"fclk_div5", "fclk_div3" };

static struct clk_regmap t5d_vpu_clkb_tmp_mux = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_VPU_CLKB_CNTL,
		.mask = 0x3,
		.shift = 20,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vpu_clkb_tmp_mux",
		.ops = &clk_regmap_mux_ops,
		.parent_names = t5d_vpu_clkb_parent_names,
		.num_parents = 4,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap t5d_vpu_clkb_tmp_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_VPU_CLKB_CNTL,
		.shift = 16,
		.width = 4,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vpu_clkb_tmp_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_vpu_clkb_tmp_mux.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t5d_vpu_clkb_tmp_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_VPU_CLKB_CNTL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vpu_clkb_tmp_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_vpu_clkb_tmp_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t5d_vpu_clkb_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_VPU_CLKB_CNTL,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vpu_clkb_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_vpu_clkb_tmp_gate.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t5d_vpu_clkb_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_VPU_CLKB_CNTL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vpu_clkb_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_vpu_clkb_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static const char * const t5d_tcon_pll_clk_parent_names[] = { "xtal", "fclk_div5",
				"fclk_div4", "fclk_div3", "mpll2", "mpll3",
				"null", "null" };

static struct clk_regmap t5d_tcon_pll_clk_mux = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_TCON_CLK_CNTL,
		.mask = 0x7,
		.shift = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "tcon_pll_clk_mux",
		.ops = &clk_regmap_mux_ops,
		.parent_names = t5d_tcon_pll_clk_parent_names,
		.num_parents = 8,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap t5d_tcon_pll_clk_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_TCON_CLK_CNTL,
		.shift = 0,
		.width = 6,
	},
	.hw.init = &(struct clk_init_data){
		.name = "tcon_pll_clk_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_tcon_pll_clk_mux.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t5d_tcon_pll_clk_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_TCON_CLK_CNTL,
		.bit_idx = 6,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "tcon_pll_clk_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_tcon_pll_clk_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t5d_vid_lock_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_VID_LOCK_CLK_CNTL,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vid_lock_div",
		.ops = &clk_regmap_divider_ops,
		.parent_names = (const char *[]){ "xtal" },
		.num_parents = 1,
	},
};

static struct clk_regmap t5d_vid_lock_clk = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_VID_LOCK_CLK_CNTL,
		.bit_idx = 7,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vid_lock_clk",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_vid_lock_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static const char * const t5d_hdmi_axi_parent_names[] = { "xtal", "fclk_div4",
				"fclk_div3", "fclk_div5"};

static struct clk_regmap t5d_hdmi_axi_mux = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_HDMI_AXI_CLK_CNTL,
		.mask = 0x3,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdmi_axi_mux",
		.ops = &clk_regmap_mux_ops,
		.parent_names = t5d_hdmi_axi_parent_names,
		.num_parents = 4,
	},
};

static struct clk_regmap t5d_hdmi_axi_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_HDMI_AXI_CLK_CNTL,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdmi_axi_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_hdmi_axi_mux.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t5d_hdmi_axi_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_HDMI_AXI_CLK_CNTL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hdmi_axi_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_hdmi_axi_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT
	},
};

static const char * const t5d_demod_t2_parent_names[] = { "xtal",
	"fclk_div5", "fclk_div4", "adc_extclk_in" };

static struct clk_regmap t5d_demod_core_t2_mux = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_DEMOD_CLK_CNTL1,
		.mask = 0x3,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "demod_core_t2_mux",
		.ops = &clk_regmap_mux_ops,
		.parent_names = t5d_demod_t2_parent_names,
		.num_parents = 4,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap t5d_demod_core_t2_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_DEMOD_CLK_CNTL1,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "demod_core_t2_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_demod_core_t2_mux.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t5d_demod_core_t2 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_DEMOD_CLK_CNTL1,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "demod_core_t2",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_demod_core_t2_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT
	},
};

static const char * const t5d_tsin_parent_names[] = { "fclk_div2",
	"xtal", "fclk_div4", "fclk_div3", "fclk_div2p5",
	"fclk_div7"};

static struct clk_regmap t5d_tsin_deglich_mux = {
	.data = &(struct clk_regmap_mux_data){
		.offset = HHI_TSIN_DEGLITCH_CLK_CNTL,
		.mask = 0x3,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "tsin_deglich_mux",
		.ops = &clk_regmap_mux_ops,
		.parent_names = t5d_tsin_parent_names,
		.num_parents = ARRAY_SIZE(t5d_tsin_parent_names),
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap t5d_tsin_deglich_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = HHI_TSIN_DEGLITCH_CLK_CNTL,
		.shift = 0,
		.width = 6,
	},
	.hw.init = &(struct clk_init_data){
		.name = "tsin_deglich_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_tsin_deglich_mux.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t5d_tsin_deglich = {
	.data = &(struct clk_regmap_gate_data){
		.offset = HHI_TSIN_DEGLITCH_CLK_CNTL,
		.bit_idx = 6,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "tsin_deglich",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t5d_tsin_deglich_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT
	},
};

struct t5d_sys_pll_nb_data {
	struct notifier_block nb;
	struct clk_hw *sys_pll;
	struct clk_hw *cpu_clk;
	struct clk_hw *cpu_dyn_clk;
};

static int t5d_sys_pll_notifier_cb(struct notifier_block *nb,
				   unsigned long event, void *data)
{
	struct t5d_sys_pll_nb_data *nb_data =
		container_of(nb, struct t5d_sys_pll_nb_data, nb);

	switch (event) {
	case PRE_RATE_CHANGE:
		/*
		 * This notifier means sys_pll clock will be changed
		 * to feed cpu_clk, this the current path :
		 * cpu_clk
		 *    \- sys_pll
		 *          \- sys_pll_dco
		 */

		/* Configure cpu_clk to use cpu_dyn_clk */
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

static struct t5d_sys_pll_nb_data t5d_sys_pll_nb_data = {
	.sys_pll = &t5d_sys_pll.hw,
	.cpu_clk = &t5d_cpu_clk.hw,
	.cpu_dyn_clk = &t5d_cpu_dyn_clk.hw,
	.nb.notifier_call = t5d_sys_pll_notifier_cb,
};

#define MESON_GATE(_name, _reg, _bit)						\
struct clk_regmap _name = {                                             \
		.data = &(struct clk_regmap_gate_data){                         \
			.offset = (_reg),                                       \
			.bit_idx = (_bit),                                      \
		},                                                              \
		.hw.init = &(struct clk_init_data) {                            \
			.name = #_name,                                         \
			.ops = &clk_regmap_gate_ops,                            \
			.parent_names = (const char *[]){ "clk81" },		\
			.num_parents = 1,                                       \
			.flags = (CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED),     \
		},                                                              \
}

static MESON_GATE(t5d_clk81_ddr,		HHI_GCLK_MPEG0, 0);
static MESON_GATE(t5d_clk81_dos,		HHI_GCLK_MPEG0, 1);
static MESON_GATE(t5d_clk81_ethphy,		HHI_GCLK_MPEG0, 4);
static MESON_GATE(t5d_clk81_isa,		HHI_GCLK_MPEG0, 5);
static MESON_GATE(t5d_clk81_pl310,		HHI_GCLK_MPEG0, 6);
static MESON_GATE(t5d_clk81_periphs,		HHI_GCLK_MPEG0, 7);
static MESON_GATE(t5d_clk81_spicc0,		HHI_GCLK_MPEG0, 8);
static MESON_GATE(t5d_clk81_i2c,		HHI_GCLK_MPEG0, 9);
static MESON_GATE(t5d_clk81_sana,		HHI_GCLK_MPEG0, 10);
static MESON_GATE(t5d_clk81_smartcard,		HHI_GCLK_MPEG0, 11);
static MESON_GATE(t5d_clk81_uart0,		HHI_GCLK_MPEG0, 13);
static MESON_GATE(t5d_clk81_stream,		HHI_GCLK_MPEG0, 15);
static MESON_GATE(t5d_clk81_async_fifo,		HHI_GCLK_MPEG0, 16);
static MESON_GATE(t5d_clk81_tvfe,		HHI_GCLK_MPEG0, 18);
static MESON_GATE(t5d_clk81_hiu_reg,		HHI_GCLK_MPEG0, 19);
static MESON_GATE(t5d_clk81_hdmirx_pclk,	HHI_GCLK_MPEG0, 21);
static MESON_GATE(t5d_clk81_atv_demod,		HHI_GCLK_MPEG0, 22);
static MESON_GATE(t5d_clk81_assist_misc,	HHI_GCLK_MPEG0, 23);
static MESON_GATE(t5d_clk81_pwr_ctrl,		HHI_GCLK_MPEG0, 24);
static MESON_GATE(t5d_clk81_emmc_c,		HHI_GCLK_MPEG0, 26);
static MESON_GATE(t5d_clk81_adec,		HHI_GCLK_MPEG0, 27);
static MESON_GATE(t5d_clk81_acodec,		HHI_GCLK_MPEG0, 28);
static MESON_GATE(t5d_clk81_tcon,		HHI_GCLK_MPEG0, 29);
static MESON_GATE(t5d_clk81_spi,		HHI_GCLK_MPEG0, 30);
static MESON_GATE(t5d_clk81_audio,		HHI_GCLK_MPEG1, 0);
static MESON_GATE(t5d_clk81_eth,		HHI_GCLK_MPEG1, 3);
static MESON_GATE(t5d_clk81_top_demux,		HHI_GCLK_MPEG1, 4);
static MESON_GATE(t5d_clk81_clk_rst,		HHI_GCLK_MPEG1, 5);
static MESON_GATE(t5d_clk81_aififo,		HHI_GCLK_MPEG1, 11);
static MESON_GATE(t5d_clk81_uart1,		HHI_GCLK_MPEG1, 16);
static MESON_GATE(t5d_clk81_g2d,		HHI_GCLK_MPEG1, 20);
static MESON_GATE(t5d_clk81_reset,		HHI_GCLK_MPEG1, 23);
static MESON_GATE(t5d_clk81_parser0,		HHI_GCLK_MPEG1, 25);
static MESON_GATE(t5d_clk81_usb_gene,		HHI_GCLK_MPEG1, 26);
static MESON_GATE(t5d_clk81_parser1,		HHI_GCLK_MPEG1, 28);
static MESON_GATE(t5d_clk81_ahb_arb0,		HHI_GCLK_MPEG1, 29);
static MESON_GATE(t5d_clk81_ahb_data,		HHI_GCLK_MPEG2, 1);
static MESON_GATE(t5d_clk81_ahb_ctrl,		HHI_GCLK_MPEG2, 2);
static MESON_GATE(t5d_clk81_usb1_to_ddr,	HHI_GCLK_MPEG2, 8);
static MESON_GATE(t5d_clk81_mmc_pclk,		HHI_GCLK_MPEG2, 11);
static MESON_GATE(t5d_clk81_hdmirx_axi,		HHI_GCLK_MPEG2, 12);
static MESON_GATE(t5d_clk81_hdcp22_pclk,	HHI_GCLK_MPEG2, 13);
static MESON_GATE(t5d_clk81_uart2,		HHI_GCLK_MPEG2, 15);
static MESON_GATE(t5d_clk81_ts,			HHI_GCLK_MPEG2, 22);
static MESON_GATE(t5d_clk81_vpu_int,		HHI_GCLK_MPEG2, 25);
static MESON_GATE(t5d_clk81_demod_com,		HHI_GCLK_MPEG2, 28);
static MESON_GATE(t5d_clk81_gic,		HHI_GCLK_MPEG2, 30);
static MESON_GATE(t5d_clk81_vclk2_venci0,	HHI_GCLK_OTHER, 1);
static MESON_GATE(t5d_clk81_vclk2_venci1,	HHI_GCLK_OTHER, 2);
static MESON_GATE(t5d_clk81_vclk2_vencp0,	HHI_GCLK_OTHER, 3);
static MESON_GATE(t5d_clk81_vclk2_vencp1,	HHI_GCLK_OTHER, 4);
static MESON_GATE(t5d_clk81_vclk2_venct0,	HHI_GCLK_OTHER, 5);
static MESON_GATE(t5d_clk81_vclk2_venct1,	HHI_GCLK_OTHER, 6);
static MESON_GATE(t5d_clk81_vclk2_other,	HHI_GCLK_OTHER, 7);
static MESON_GATE(t5d_clk81_vclk2_enci,		HHI_GCLK_OTHER, 8);
static MESON_GATE(t5d_clk81_vclk2_encp,		HHI_GCLK_OTHER, 9);
static MESON_GATE(t5d_clk81_dac_clk,		HHI_GCLK_OTHER, 10);
static MESON_GATE(t5d_clk81_enc480p,		HHI_GCLK_OTHER, 20);
static MESON_GATE(t5d_clk81_random,		HHI_GCLK_OTHER, 21);
static MESON_GATE(t5d_clk81_vclk2_enct,		HHI_GCLK_OTHER, 22);
static MESON_GATE(t5d_clk81_vclk2_encl,		HHI_GCLK_OTHER, 23);
static MESON_GATE(t5d_clk81_vclk2_venclmmc,	HHI_GCLK_OTHER, 24);
static MESON_GATE(t5d_clk81_vclk2_vencl,	HHI_GCLK_OTHER, 25);
static MESON_GATE(t5d_clk81_vclk2_other1,	HHI_GCLK_OTHER, 26);
static MESON_GATE(t5d_clk81_dma,		HHI_GCLK_AO, 0);
static MESON_GATE(t5d_clk81_efuse,		HHI_GCLK_AO, 1);
static MESON_GATE(t5d_clk81_rom_boot,		HHI_GCLK_AO, 2);
static MESON_GATE(t5d_clk81_reset_sec,		HHI_GCLK_AO, 3);
static MESON_GATE(t5d_clk81_sec_ahb,		HHI_GCLK_AO, 4);
static MESON_GATE(t5d_clk81_rsa,		HHI_GCLK_AO, 5);

/* Array of all clocks provided by this provider */
static struct clk_hw_onecell_data t5d_hw_onecell_data = {
	.hws = {
		[CLKID_FIXED_PLL_DCO]		= &t5d_fixed_pll_dco.hw,
		[CLKID_SYS_PLL_DCO]		= &t5d_sys_pll_dco.hw,
		[CLKID_GP0_PLL_DCO]		= &t5d_gp0_pll_dco.hw,
		[CLKID_HIFI_PLL_DCO]		= &t5d_hifi_pll_dco.hw,
		[CLKID_FIXED_PLL]		= &t5d_fixed_pll.hw,
		[CLKID_SYS_PLL]			= &t5d_sys_pll.hw,
		[CLKID_GP0_PLL]			= &t5d_gp0_pll.hw,
		[CLKID_HIFI_PLL]		= &t5d_hifi_pll.hw,
		[CLKID_FCLK_DIV2_DIV]		= &t5d_fclk_div2_div.hw,
		[CLKID_FCLK_DIV3_DIV]		= &t5d_fclk_div3_div.hw,
		[CLKID_FCLK_DIV4_DIV]		= &t5d_fclk_div4_div.hw,
		[CLKID_FCLK_DIV5_DIV]		= &t5d_fclk_div5_div.hw,
		[CLKID_FCLK_DIV7_DIV]		= &t5d_fclk_div7_div.hw,
		[CLKID_FCLK_DIV2P5_DIV]		= &t5d_fclk_div2p5_div.hw,
		[CLKID_FCLK_DIV2]		= &t5d_fclk_div2.hw,
		[CLKID_FCLK_DIV3]		= &t5d_fclk_div3.hw,
		[CLKID_FCLK_DIV4]		= &t5d_fclk_div4.hw,
		[CLKID_FCLK_DIV5]		= &t5d_fclk_div5.hw,
		[CLKID_FCLK_DIV7]		= &t5d_fclk_div7.hw,
		[CLKID_FCLK_DIV2P5]		= &t5d_fclk_div2p5.hw,
		[CLKID_PRE_MPLL]		= &t5d_mpll_prediv.hw,
		[CLKID_MPLL0_DIV]		= &t5d_mpll0_div.hw,
		[CLKID_MPLL1_DIV]		= &t5d_mpll1_div.hw,
		[CLKID_MPLL2_DIV]		= &t5d_mpll2_div.hw,
		[CLKID_MPLL3_DIV]		= &t5d_mpll3_div.hw,
		[CLKID_MPLL0]			= &t5d_mpll0.hw,
		[CLKID_MPLL1]			= &t5d_mpll1.hw,
		[CLKID_MPLL2]			= &t5d_mpll2.hw,
		[CLKID_MPLL3]			= &t5d_mpll3.hw,
		[CLKID_CPU_DYN_CLK]		= &t5d_cpu_dyn_clk.hw,
		[CLKID_CPU_CLK]			= &t5d_cpu_clk.hw,
		[CLKID_MPLL_50M_DIV]		= &t5d_mpll_50m_div.hw,
		[CLKID_MPLL_50M]		= &t5d_mpll_50m.hw,
		[CLKID_CLK81_DDR]		= &t5d_clk81_ddr.hw,
		[CLKID_CLK81_DOS]		= &t5d_clk81_dos.hw,
		[CLKID_CLK81_ETH_PHY]		= &t5d_clk81_ethphy.hw,
		[CLKID_CLK81_ISA]		= &t5d_clk81_isa.hw,
		[CLKID_CLK81_PL310]		= &t5d_clk81_pl310.hw,
		[CLKID_CLK81_PERIPHS]		= &t5d_clk81_periphs.hw,
		[CLKID_CLK81_SPICC0]		= &t5d_clk81_spicc0.hw,
		[CLKID_CLK81_I2C]		= &t5d_clk81_i2c.hw,
		[CLKID_CLK81_SANA]		= &t5d_clk81_sana.hw,
		[CLKID_CLK81_SMARTCARD]		= &t5d_clk81_smartcard.hw,
		[CLKID_CLK81_UART0]		= &t5d_clk81_uart0.hw,
		[CLKID_CLK81_STREAM]		= &t5d_clk81_stream.hw,
		[CLKID_CLK81_ASYNC_FIFO]	= &t5d_clk81_async_fifo.hw,
		[CLKID_CLK81_TVFE]		= &t5d_clk81_tvfe.hw,
		[CLKID_CLK81_HIU_REG]		= &t5d_clk81_hiu_reg.hw,
		[CLKID_CLK81_HDMIRX_PCLK]	= &t5d_clk81_hdmirx_pclk.hw,
		[CLKID_CLK81_ATV_DEMOD]		= &t5d_clk81_atv_demod.hw,
		[CLKID_CLK81_ASSIST_MISC]	= &t5d_clk81_assist_misc.hw,
		[CLKID_CLK81_PWR_CTRL]		= &t5d_clk81_pwr_ctrl.hw,
		[CLKID_CLK81_SD_EMMC_C]		= &t5d_clk81_emmc_c.hw,
		[CLKID_CLK81_ADEC]		= &t5d_clk81_adec.hw,
		[CLKID_CLK81_ACODEC]		= &t5d_clk81_acodec.hw,
		[CLKID_CLK81_TCON]		= &t5d_clk81_tcon.hw,
		[CLKID_CLK81_SPI]		= &t5d_clk81_spi.hw,
		[CLKID_CLK81_AUDIO]		= &t5d_clk81_audio.hw,
		[CLKID_CLK81_ETH_CORE]		= &t5d_clk81_eth.hw,
		[CLKID_CLK81_DEMUX]		= &t5d_clk81_top_demux.hw,
		[CLKID_CLK81_CLK_RST]		= &t5d_clk81_clk_rst.hw,
		[CLKID_CLK81_AIFIFO]		= &t5d_clk81_aififo.hw,
		[CLKID_CLK81_UART1]		= &t5d_clk81_uart1.hw,
		[CLKID_CLK81_G2D]		= &t5d_clk81_g2d.hw,
		[CLKID_CLK81_RESET]		= &t5d_clk81_reset.hw,
		[CLKID_CLK81_PARSER0]		= &t5d_clk81_parser0.hw,
		[CLKID_CLK81_USB_GENERAL]	= &t5d_clk81_usb_gene.hw,
		[CLKID_CLK81_PARSER1]		= &t5d_clk81_parser1.hw,
		[CLKID_CLK81_AHB_ARB0]		= &t5d_clk81_ahb_arb0.hw,
		[CLKID_CLK81_AHB_DATA_BUS]	= &t5d_clk81_ahb_data.hw,
		[CLKID_CLK81_AHB_CTRL_BUS]	= &t5d_clk81_ahb_ctrl.hw,
		[CLKID_CLK81_USB1_TO_DDR]	= &t5d_clk81_usb1_to_ddr.hw,
		[CLKID_CLK81_MMC_PCLK]		= &t5d_clk81_mmc_pclk.hw,
		[CLKID_CLK81_HDMIRX_AXI]	= &t5d_clk81_hdmirx_axi.hw,
		[CLKID_CLK81_HDCP22_PCLK]	= &t5d_clk81_hdcp22_pclk.hw,
		[CLKID_CLK81_UART2]		= &t5d_clk81_uart2.hw,
		[CLKID_CLK81_CLK81_TS]		= &t5d_clk81_ts.hw,
		[CLKID_CLK81_VPU_INTR]		= &t5d_clk81_vpu_int.hw,
		[CLKID_CLK81_DEMOD_COMB]	= &t5d_clk81_demod_com.hw,
		[CLKID_CLK81_GIC]		= &t5d_clk81_gic.hw,
		[CLKID_CLK81_VCLK2_VENCI0]	= &t5d_clk81_vclk2_venci0.hw,
		[CLKID_CLK81_VCLK2_VENCI1]	= &t5d_clk81_vclk2_venci1.hw,
		[CLKID_CLK81_VCLK2_VENCP0]	= &t5d_clk81_vclk2_vencp0.hw,
		[CLKID_CLK81_VCLK2_VENCP1]	= &t5d_clk81_vclk2_vencp1.hw,
		[CLKID_CLK81_VCLK2_VENCT0]	= &t5d_clk81_vclk2_venct0.hw,
		[CLKID_CLK81_VCLK2_VENCT1]	= &t5d_clk81_vclk2_venct1.hw,
		[CLKID_CLK81_VCLK2_OTHER]	= &t5d_clk81_vclk2_other.hw,
		[CLKID_CLK81_VCLK2_ENCI]	= &t5d_clk81_vclk2_enci.hw,
		[CLKID_CLK81_VCLK2_ENCP]	= &t5d_clk81_vclk2_encp.hw,
		[CLKID_CLK81_DAC_CLK]		= &t5d_clk81_dac_clk.hw,
		[CLKID_CLK81_ENC480P]		= &t5d_clk81_enc480p.hw,
		[CLKID_CLK81_RANDOM]		= &t5d_clk81_random.hw,
		[CLKID_CLK81_VCLK2_ENCT]	= &t5d_clk81_vclk2_enct.hw,
		[CLKID_CLK81_VCLK2_ENCL]	= &t5d_clk81_vclk2_encl.hw,
		[CLKID_CLK81_VCLK2_VENCLMMC]	= &t5d_clk81_vclk2_venclmmc.hw,
		[CLKID_CLK81_VCLK2_VENCL]	= &t5d_clk81_vclk2_vencl.hw,
		[CLKID_CLK81_VCLK2_OTHER1]	= &t5d_clk81_vclk2_other1.hw,
		[CLKID_CLK81_DMA]		= &t5d_clk81_dma.hw,
		[CLKID_CLK81_EFUSE]		= &t5d_clk81_efuse.hw,
		[CLKID_CLK81_ROM_BOOT]		= &t5d_clk81_rom_boot.hw,
		[CLKID_CLK81_RESET_SEC]		= &t5d_clk81_reset_sec.hw,
		[CLKID_CLK81_SEC_AHB]		= &t5d_clk81_sec_ahb.hw,
		[CLKID_CLK81_RSA]		= &t5d_clk81_rsa.hw,
		[CLKID_MPEG_SEL]		= &t5d_mpeg_clk_sel.hw,
		[CLKID_MPEG_DIV]		= &t5d_mpeg_clk_div.hw,
		[CLKID_CLK81]			= &t5d_clk81.hw,
		[CLKID_GPU_P0_MUX]		= &t5d_gpu_p0_mux.hw,
		[CLKID_GPU_P0_DIV]		= &t5d_gpu_p0_div.hw,
		[CLKID_GPU_P0_GATE]		= &t5d_gpu_p0_gate.hw,
		[CLKID_GPU_P1_MUX]		= &t5d_gpu_p1_mux.hw,
		[CLKID_GPU_P1_DIV]		= &t5d_gpu_p1_div.hw,
		[CLKID_GPU_P1_GATE]		= &t5d_gpu_p1_gate.hw,
		[CLKID_GPU_MUX]			= &t5d_gpu_mux.hw,
		[CLKID_VPU_P0_MUX]		= &t5d_vpu_p0_mux.hw,
		[CLKID_VPU_P0_DIV]		= &t5d_vpu_p0_div.hw,
		[CLKID_VPU_P0_GATE]		= &t5d_vpu_p0_gate.hw,
		[CLKID_VPU_P1_MUX]		= &t5d_vpu_p1_mux.hw,
		[CLKID_VPU_P1_DIV]		= &t5d_vpu_p1_div.hw,
		[CLKID_VPU_P1_GATE]		= &t5d_vpu_p1_gate.hw,
		[CLKID_VPU_MUX]			= &t5d_vpu_mux.hw,
		[CLKID_VPU_CLKC_P0_MUX]		= &t5d_vpu_clkc_p0_mux.hw,
		[CLKID_VPU_CLKC_P0_DIV]		= &t5d_vpu_clkc_p0_div.hw,
		[CLKID_VPU_CLKC_P0_GATE]	= &t5d_vpu_clkc_p0_gate.hw,
		[CLKID_VPU_CLKC_P1_MUX]		= &t5d_vpu_clkc_p1_mux.hw,
		[CLKID_VPU_CLKC_P1_DIV]		= &t5d_vpu_clkc_p1_div.hw,
		[CLKID_VPU_CLKC_P1_GATE]	= &t5d_vpu_clkc_p1_gate.hw,
		[CLKID_VPU_CLKC_MUX]		= &t5d_vpu_clkc_mux.hw,
		[CLKID_ADC_EXTCLK_IN_MUX]	= &t5d_adc_extclk_in_mux.hw,
		[CLKID_ADC_EXTCLK_IN_DIV]	= &t5d_adc_extclk_in_div.hw,
		[CLKID_ADC_EXTCLK_IN]		= &t5d_adc_extclk_in.hw,
		[CLKID_DEMOD_CORE_CLK_MUX]	= &t5d_demod_core_clk_mux.hw,
		[CLKID_DEMOD_CORE_CLK_DIV]	= &t5d_demod_core_clk_div.hw,
		[CLKID_DEMOD_CORE_CLK]		= &t5d_demod_core_clk.hw,
		[CLKID_VDEC_P0_MUX]		= &t5d_vdec_p0_mux.hw,
		[CLKID_VDEC_P0_DIV]		= &t5d_vdec_p0_div.hw,
		[CLKID_VDEC_P0_GATE]		= &t5d_vdec_p0_gate.hw,
		[CLKID_HEVCF_P0_MUX]		= &t5d_hevcf_p0_mux.hw,
		[CLKID_HEVCF_P0_DIV]		= &t5d_hevcf_p0_div.hw,
		[CLKID_HEVCF_P0_GATE]		= &t5d_hevcf_p0_gate.hw,
		[CLKID_VDEC_P1_MUX]		= &t5d_vdec_p1_mux.hw,
		[CLKID_VDEC_P1_DIV]		= &t5d_vdec_p1_div.hw,
		[CLKID_VDEC_P1_GATE]		= &t5d_vdec_p1_gate.hw,
		[CLKID_VDEC_MUX]		= &t5d_vdec_mux.hw,
		[CLKID_HEVCF_P1_MUX]		= &t5d_hevcf_p1_mux.hw,
		[CLKID_HEVCF_P1_DIV]		= &t5d_hevcf_p1_div.hw,
		[CLKID_HEVCF_P1_GATE]		= &t5d_hevcf_p1_gate.hw,
		[CLKID_HEVCF_MUX]		= &t5d_hevcf_mux.hw,
		[CLKID_HDCP22_ESM_MUX]		= &t5d_hdcp22_esm_mux.hw,
		[CLKID_HDCP22_SKP_MUX]		= &t5d_hdcp22_skp_mux.hw,
		[CLKID_HDCP22_ESM_DIV]		= &t5d_hdcp22_esm_div.hw,
		[CLKID_HDCP22_SKP_DIV]		= &t5d_hdcp22_skp_div.hw,
		[CLKID_HDCP22_ESM_GATE]		= &t5d_hdcp22_esm_gate.hw,
		[CLKID_HDCP22_SKP_GATE]		= &t5d_hdcp22_skp_gate.hw,
		[CLKID_VAPB_P0_MUX]		= &t5d_vapb_p0_mux.hw,
		[CLKID_VAPB_P1_MUX]		= &t5d_vapb_p1_mux.hw,
		[CLKID_VAPB_P0_DIV]		= &t5d_vapb_p0_div.hw,
		[CLKID_VAPB_P1_DIV]		= &t5d_vapb_p1_div.hw,
		[CLKID_VAPB_P0_GATE]		= &t5d_vapb_p0_gate.hw,
		[CLKID_VAPB_P1_GATE]		= &t5d_vapb_p1_gate.hw,
		[CLKID_VAPB_MUX]		= &t5d_vapb_mux.hw,
		[CLKID_HDMIRX_CFG_MUX]		= &t5d_hdmirx_cfg_mux.hw,
		[CLKID_HDMIRX_MODET_MUX]	= &t5d_hdmirx_modet_mux.hw,
		[CLKID_HDMIRX_CFG_DIV]		= &t5d_hdmirx_cfg_div.hw,
		[CLKID_HDMIRX_MODET_DIV]	= &t5d_hdmirx_modet_div.hw,
		[CLKID_HDMIRX_CFG_GATE]		= &t5d_hdmirx_cfg_gate.hw,
		[CLKID_HDMIRX_MODET_GATE]	= &t5d_hdmirx_modet_gate.hw,
		[CLKID_HDMIRX_ACR_MUX]		= &t5d_hdmirx_acr_mux.hw,
		[CLKID_HDMIRX_ACR_DIV]		= &t5d_hdmirx_acr_div.hw,
		[CLKID_HDMIRX_ACR_GATE]		= &t5d_hdmirx_acr_gate.hw,
		[CLKID_HDMIRX_METER_MUX]	= &t5d_hdmirx_meter_mux.hw,
		[CLKID_HDMIRX_METER_DIV]	= &t5d_hdmirx_meter_div.hw,
		[CLKID_HDMIRX_METER_GATE]	= &t5d_hdmirx_meter_gate.hw,
		[CLKID_VDIN_MEAS_MUX]		= &t5d_vdin_meas_mux.hw,
		[CLKID_VDIN_MEAS_DIV]		= &t5d_vdin_meas_div.hw,
		[CLKID_VDIN_MEAS_GATE]		= &t5d_vdin_meas_gate.hw,
		[CLKID_SD_EMMC_C_MUX]		= &t5d_sd_emmc_c_mux.hw,
		[CLKID_SD_EMMC_C_DIV]		= &t5d_sd_emmc_c_div.hw,
		[CLKID_SD_EMMC_C_GATE]		= &t5d_sd_emmc_c_gate.hw,
		[CLKID_SPICC0_MUX]		= &t5d_spicc0_mux.hw,
		[CLKID_SPICC0_DIV]		= &t5d_spicc0_div.hw,
		[CLKID_SPICC0_GATE]		= &t5d_spicc0_gate.hw,
		[CLKID_VDAC_CLKC_MUX]		= &t5d_vdac_clkc_mux.hw,
		[CLKID_VDAC_CLKC_DIV]		= &t5d_vdac_clkc_div.hw,
		[CLKID_VDAC_CLKC_GATE]		= &t5d_vdac_clkc_gate.hw,
		[CLKID_GE2D_GATE]               = &t5d_ge2d_gate.hw,
		[CLKID_VPU_CLKB_TMP_MUX]	= &t5d_vpu_clkb_tmp_mux.hw,
		[CLKID_VPU_CLKB_TMP_DIV]	= &t5d_vpu_clkb_tmp_div.hw,
		[CLKID_VPU_CLKB_TMP_GATE]	= &t5d_vpu_clkb_tmp_gate.hw,
		[CLKID_VPU_CLKB_DIV]		= &t5d_vpu_clkb_div.hw,
		[CLKID_VPU_CLKB_GATE]		= &t5d_vpu_clkb_gate.hw,
		[CLKID_TCON_PLL_CLK_MUX]	= &t5d_tcon_pll_clk_mux.hw,
		[CLKID_TCON_PLL_CLK_DIV]	= &t5d_tcon_pll_clk_div.hw,
		[CLKID_TCON_PLL_CLK_GATE]	= &t5d_tcon_pll_clk_gate.hw,
		[CLKID_VID_LOCK_DIV]		= &t5d_vid_lock_div.hw,
		[CLKID_VID_LOCK_CLK]		= &t5d_vid_lock_clk.hw,
		[CLKID_HDMI_AXI_MUX]		= &t5d_hdmi_axi_mux.hw,
		[CLKID_HDMI_AXI_DIV]		= &t5d_hdmi_axi_div.hw,
		[CLKID_HDMI_AXI_GATE]		= &t5d_hdmi_axi_gate.hw,
		[CLKID_DEMOD_T2_MUX]		= &t5d_demod_core_t2_mux.hw,
		[CLKID_DEMOD_T2_DIV]		= &t5d_demod_core_t2_div.hw,
		[CLKID_DEMOD_T2_GATE]		= &t5d_demod_core_t2.hw,
		[CLKID_TSIN_DEGLICH_MUX]	= &t5d_tsin_deglich_mux.hw,
		[CLKID_TSIN_DEGLICH_DIV]	= &t5d_tsin_deglich_div.hw,
		[CLKID_TSIN_DEGLICH_GATE]	= &t5d_tsin_deglich.hw,
		[NR_CLKS]			= NULL,
	},
	.num = NR_CLKS,
};

/* Convenience table to populate regmap in .probe */
static struct clk_regmap *const t5d_clk_regmaps[] __initconst = {
	&t5d_fixed_pll,
	&t5d_sys_pll,
	&t5d_gp0_pll,
	&t5d_hifi_pll,
	&t5d_fixed_pll_dco,
	&t5d_sys_pll_dco,
	&t5d_gp0_pll_dco,
	&t5d_hifi_pll_dco,
	&t5d_fclk_div2,
	&t5d_fclk_div3,
	&t5d_fclk_div4,
	&t5d_fclk_div5,
	&t5d_fclk_div7,
	&t5d_fclk_div2p5,
	&t5d_mpll0_div,
	&t5d_mpll1_div,
	&t5d_mpll2_div,
	&t5d_mpll3_div,
	&t5d_mpll0,
	&t5d_mpll1,
	&t5d_mpll2,
	&t5d_mpll3,
	&t5d_clk81_ddr,
	&t5d_clk81_dos,
	&t5d_clk81_ethphy,
	&t5d_clk81_isa,
	&t5d_clk81_pl310,
	&t5d_clk81_periphs,
	&t5d_clk81_spicc0,
	&t5d_clk81_i2c,
	&t5d_clk81_sana,
	&t5d_clk81_smartcard,
	&t5d_clk81_uart0,
	&t5d_clk81_stream,
	&t5d_clk81_async_fifo,
	&t5d_clk81_tvfe,
	&t5d_clk81_hiu_reg,
	&t5d_clk81_hdmirx_pclk,
	&t5d_clk81_atv_demod,
	&t5d_clk81_assist_misc,
	&t5d_clk81_pwr_ctrl,
	&t5d_clk81_emmc_c,
	&t5d_clk81_adec,
	&t5d_clk81_acodec,
	&t5d_clk81_tcon,
	&t5d_clk81_spi,
	&t5d_clk81_audio,
	&t5d_clk81_eth,
	&t5d_clk81_top_demux,
	&t5d_clk81_clk_rst,
	&t5d_clk81_aififo,
	&t5d_clk81_uart1,
	&t5d_clk81_g2d,
	&t5d_clk81_reset,
	&t5d_clk81_parser0,
	&t5d_clk81_usb_gene,
	&t5d_clk81_parser1,
	&t5d_clk81_ahb_arb0,
	&t5d_clk81_ahb_data,
	&t5d_clk81_ahb_ctrl,
	&t5d_clk81_usb1_to_ddr,
	&t5d_clk81_mmc_pclk,
	&t5d_clk81_hdmirx_axi,
	&t5d_clk81_hdcp22_pclk,
	&t5d_clk81_uart2,
	&t5d_clk81_ts,
	&t5d_clk81_vpu_int,
	&t5d_clk81_demod_com,
	&t5d_clk81_gic,
	&t5d_clk81_vclk2_venci0,
	&t5d_clk81_vclk2_venci1,
	&t5d_clk81_vclk2_vencp0,
	&t5d_clk81_vclk2_vencp1,
	&t5d_clk81_vclk2_venct0,
	&t5d_clk81_vclk2_venct1,
	&t5d_clk81_vclk2_other,
	&t5d_clk81_vclk2_enci,
	&t5d_clk81_vclk2_encp,
	&t5d_clk81_dac_clk,
	&t5d_clk81_enc480p,
	&t5d_clk81_random,
	&t5d_clk81_vclk2_enct,
	&t5d_clk81_vclk2_encl,
	&t5d_clk81_vclk2_venclmmc,
	&t5d_clk81_vclk2_vencl,
	&t5d_clk81_vclk2_other1,
	&t5d_clk81_dma,
	&t5d_clk81_efuse,
	&t5d_clk81_rom_boot,
	&t5d_clk81_reset_sec,
	&t5d_clk81_sec_ahb,
	&t5d_clk81_rsa,/* clk81 gate over */
	/*cpu regmap*/
	&t5d_cpu_dyn_clk,
	&t5d_cpu_clk,
	&t5d_mpll_50m,
};

/* Convenience table to populate regmap in .probe */
static struct clk_regmap *const t5d_periph_clk_regmaps[] __initconst = {
	&t5d_mpeg_clk_sel,
	&t5d_mpeg_clk_div,
	&t5d_clk81,
	&t5d_ts_div,
	&t5d_ts,
	&t5d_gpu_p0_mux,
	&t5d_gpu_p1_mux,
	&t5d_gpu_mux,
	&t5d_vpu_p0_mux,
	&t5d_vpu_p1_mux,
	&t5d_vpu_mux,
	&t5d_vpu_clkc_p0_mux,
	&t5d_vpu_clkc_p1_mux,
	&t5d_vpu_clkc_mux,
	&t5d_adc_extclk_in_mux,
	&t5d_demod_core_clk_mux,
	&t5d_vdec_p0_mux,
	&t5d_hevcf_p0_mux,
	&t5d_vdec_p1_mux,
	&t5d_vdec_mux,
	&t5d_hevcf_p1_mux,
	&t5d_hevcf_mux,
	&t5d_hdcp22_esm_mux,
	&t5d_hdcp22_skp_mux,
	&t5d_vapb_p0_mux,
	&t5d_vapb_p1_mux,
	&t5d_vapb_mux,
	&t5d_hdmirx_cfg_mux,
	&t5d_hdmirx_modet_mux,
	&t5d_hdmirx_acr_mux,
	&t5d_hdmirx_meter_mux,
	&t5d_vdin_meas_mux,
	&t5d_sd_emmc_c_mux,
	&t5d_spicc0_mux,
	&t5d_vdac_clkc_mux,
	&t5d_vpu_clkb_tmp_mux,
	&t5d_tcon_pll_clk_mux,
	&t5d_hdmi_axi_mux,
	&t5d_demod_core_t2_mux,
	&t5d_tsin_deglich_mux,
	&t5d_gpu_p0_div,
	&t5d_gpu_p1_div,
	&t5d_vpu_p0_div,
	&t5d_vpu_p1_div,
	&t5d_vpu_clkc_p0_div,
	&t5d_vpu_clkc_p1_div,
	&t5d_adc_extclk_in_div,
	&t5d_demod_core_clk_div,
	&t5d_vdec_p0_div,
	&t5d_hevcf_p0_div,
	&t5d_vdec_p1_div,
	&t5d_hevcf_p1_div,
	&t5d_hdcp22_esm_div,
	&t5d_hdcp22_skp_div,
	&t5d_vapb_p0_div,
	&t5d_vapb_p1_div,
	&t5d_hdmirx_cfg_div,
	&t5d_hdmirx_modet_div,
	&t5d_hdmirx_acr_div,
	&t5d_hdmirx_meter_div,
	&t5d_vdin_meas_div,
	&t5d_sd_emmc_c_div,
	&t5d_spicc0_div,
	&t5d_vdac_clkc_div,
	&t5d_vpu_clkb_tmp_div,
	&t5d_vpu_clkb_div,
	&t5d_tcon_pll_clk_div,
	&t5d_vid_lock_div,
	&t5d_hdmi_axi_div,
	&t5d_demod_core_t2_div,
	&t5d_tsin_deglich_div,
	&t5d_gpu_p0_gate,
	&t5d_gpu_p1_gate,
	&t5d_vpu_p0_gate,
	&t5d_vpu_p1_gate,
	&t5d_vpu_clkc_p0_gate,
	&t5d_vpu_clkc_p1_gate,
	&t5d_adc_extclk_in,
	&t5d_demod_core_clk,
	&t5d_vdec_p0_gate,
	&t5d_hevcf_p0_gate,
	&t5d_vdec_p1_gate,
	&t5d_hevcf_p1_gate,
	&t5d_hdcp22_esm_gate,
	&t5d_hdcp22_skp_gate,
	&t5d_vapb_p0_gate,
	&t5d_vapb_p1_gate,
	&t5d_hdmirx_cfg_gate,
	&t5d_hdmirx_modet_gate,
	&t5d_hdmirx_acr_gate,
	&t5d_hdmirx_meter_gate,
	&t5d_vdin_meas_gate,
	&t5d_sd_emmc_c_gate,
	&t5d_spicc0_gate,
	&t5d_vdac_clkc_gate,
	&t5d_ge2d_gate,
	&t5d_vpu_clkb_tmp_gate,
	&t5d_vpu_clkb_gate,
	&t5d_tcon_pll_clk_gate,
	&t5d_vid_lock_clk,
	&t5d_hdmi_axi_gate,
	&t5d_demod_core_t2,
	&t5d_tsin_deglich,
};

static const struct reg_sequence t5d_init_regs[] = {
	{ .reg = HHI_MPLL_CNTL0,	.def = 0x00000543 },
};

static int meson_t5d_dvfs_setup(struct platform_device *pdev)
{
	struct clk *notifier_clk;
	int ret;

	/* Setup clock notifier for sys_pll */
	notifier_clk = t5d_sys_pll.hw.clk;
	ret = clk_notifier_register(notifier_clk, &t5d_sys_pll_nb_data.nb);
	if (ret) {
		dev_err(&pdev->dev, "failed to register the sys_pll notifier\n");
		return ret;
	}

	return 0;
}

static struct regmap_config clkc_regmap_config = {
	.reg_bits       = 32,
	.val_bits       = 32,
	.reg_stride     = 4,
};

static struct regmap *t5d_regmap_resource(struct device *dev, char *name)
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

static int __ref meson_t5d_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct regmap *basic_map;
	struct regmap *periph_map;
	int ret, i;

	/* Get regmap for different clock area */
	basic_map = t5d_regmap_resource(dev, "basic");
	if (IS_ERR(basic_map)) {
		dev_err(dev, "basic clk registers not found\n");
		return PTR_ERR(basic_map);
	}

	periph_map = t5d_regmap_resource(dev, "periph");
	if (IS_ERR(periph_map)) {
		dev_err(dev, "periph clk registers not found\n");
		return PTR_ERR(periph_map);
	}

	/* Populate regmap for the regmap backed clocks */
	for (i = 0; i < ARRAY_SIZE(t5d_clk_regmaps); i++)
		t5d_clk_regmaps[i]->map = basic_map;

	for (i = 0; i < ARRAY_SIZE(t5d_periph_clk_regmaps); i++)
		t5d_periph_clk_regmaps[i]->map = periph_map;

	regmap_write(basic_map, HHI_MPLL_CNTL0, 0x00000543);

	for (i = 0; i < t5d_hw_onecell_data.num; i++) {
		/* array might be sparse */
		if (!t5d_hw_onecell_data.hws[i])
			continue;

		pr_debug("registering %dth  %s\n", i,
			t5d_hw_onecell_data.hws[i]->init->name);

		ret = devm_clk_hw_register(dev, t5d_hw_onecell_data.hws[i]);
		if (ret) {
			dev_err(dev, "Clock registration failed\n");
			return ret;
		}
#ifdef CONFIG_AMLOGIC_CLK_DEBUG
		ret = devm_clk_hw_register_clkdev(dev, t5d_hw_onecell_data.hws[i],
					NULL, clk_hw_get_name(t5d_hw_onecell_data.hws[i]));
		if (ret < 0) {
			dev_err(dev, "Failed to clkdev register: %d\n", ret);
			return ret;
		}
#endif
	}

	meson_t5d_dvfs_setup(pdev);

	return devm_of_clk_add_hw_provider(dev, of_clk_hw_onecell_get,
					   &t5d_hw_onecell_data);
}

static const struct of_device_id clkc_match_table[] = {
	{
		.compatible = "amlogic,t5d-clkc",
	},
	{}
};

static struct platform_driver t5d_driver = {
	.probe		= meson_t5d_probe,
	.driver		= {
		.name	= "t5d-clkc",
		.of_match_table = clkc_match_table,
	},
};

builtin_platform_driver(t5d_driver);

MODULE_LICENSE("GPL v2");
