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
#include "clk-pll.h"
#include "clk-regmap.h"
#include "clk-cpu-dyndiv.h"
#include "vid-pll-div.h"
#include "clk-dualdiv.h"
#include "t3x.h"
#include <dt-bindings/clock/t3x-clkc.h>

static const struct clk_ops meson_pll_clk_no_ops = {};

/*
 * the sys pll DCO value should be 3G~6G,
 * otherwise the sys pll can not lock.
 * od is for 32 bit.
 */

#ifdef CONFIG_ARM
static const struct pll_params_table t3x_sys_pll_params_table[] = {
	PLL_PARAMS(100, 1, 1), /*DCO=2400M OD=1200M*/
	PLL_PARAMS(116, 1, 1), /*DCO=2784M OD=1392M*/
	PLL_PARAMS(126, 1, 1), /*DCO=3024M OD=1512M*/
	PLL_PARAMS(67, 1, 0), /*DCO=1608M OD=1608MM*/
	PLL_PARAMS(71, 1, 0), /*DCO=1704MM OD=1704M*/
	PLL_PARAMS(75, 1, 0), /*DCO=1800M OD=1800M*/
	PLL_PARAMS(79, 1, 0), /*DCO=1896M OD=1896M*/
	PLL_PARAMS(159, 2, 0), /*DCO=1908M OD=1908M*/
	PLL_PARAMS(155, 2, 0), /*DCO=1860M OD=1860M*/
	{ /* sentinel */ }
};
#else
static const struct pll_params_table t3x_sys_pll_params_table[] = {
	/*
	 *  The DCO range of syspll sys1pll on T3X is 1.6G-3.2G
	 *  OD=0 div=1  1.6G - 3.2G
	 *  OD=1 div=2  800M - 1.6G
	 *  OD=2 div=4  400M - 800M
	 *  OD=3 div=8  200M - 400M
	 *  OD=4 div=16 100M - 200M
	 */
	PLL_PARAMS(100, 1), /*DCO=2400M OD=1200M*/
	PLL_PARAMS(116, 1), /*DCO=2784 OD=1392M*/
	PLL_PARAMS(126, 1), /*DCO=3024 OD=1512M*/
	PLL_PARAMS(67, 1), /*DCO=1608M OD=1608MM*/
	PLL_PARAMS(71, 1), /*DCO=1704MM OD=1704M*/
	PLL_PARAMS(75, 1), /*DCO=1800M OD=1800M*/
	PLL_PARAMS(79, 1), /*DCO=1896M OD=1896M*/
	PLL_PARAMS(159, 2), /*DCO=1908M OD=1908M*/
	PLL_PARAMS(155, 2), /*DCO=1860M OD=1860M*/
	{ /* sentinel */ }
};
#endif

static const struct pll_mult_range t3x_pll_mult_range = {
	.min = 67,
	.max = 125,
};

static struct clk_regmap t3x_sys_pll_dco = {
	.data = &(struct meson_clk_pll_data){
		.en = {
			.reg_off = CLKCTRL_SYS0PLL_CTRL0,
			.shift   = 28,
			.width   = 1,
		},
		.m = {
			.reg_off = CLKCTRL_SYS0PLL_CTRL0,
			.shift   = 0,
			.width   = 8,
		},
		.n = {
			.reg_off = CLKCTRL_SYS0PLL_CTRL0,
			.shift   = 16,
			.width   = 5,
		},
		/* od is required when setting the same rate during STR */
		.od = {
			.reg_off = CLKCTRL_SYS0PLL_CTRL0,
			.shift	 = 12,
			.width	 = 3,
		},
		.table = t3x_sys_pll_params_table,
		.l = {
			.reg_off = CLKCTRL_SYS0PLL_CTRL0,
			.shift   = 31,
			.width   = 1,
		},
		.rst = {
			.reg_off = CLKCTRL_SYS0PLL_CTRL0,
			.shift   = 29,
			.width   = 1,
		},
		.smc_id = SECURE_PLL_CLK,
		.secid_disable = SECID_SYS0_DCO_PLL_DIS,
		.secid = SECID_SYS0_DCO_PLL
	},
	.hw.init = &(struct clk_init_data){
		.name = "sys_pll_dco",
		.ops = &meson_secure_pll_v2_ops,
		.parent_data = &(const struct clk_parent_data) {
			.fw_name = "xtal",
		},
		.num_parents = 1,
		/*
		 * This clock feeds the CPU, avoid disabling it
		 * Register has the risk of being directly operated
		 */
		.flags = CLK_IS_CRITICAL
	},
};

static struct clk_regmap t3x_sys1_pll_dco = {
	.data = &(struct meson_clk_pll_data){
		.en = {
			.reg_off = CLKCTRL_SYS1PLL_CTRL0,
			.shift   = 28,
			.width   = 1,
		},
		.m = {
			.reg_off = CLKCTRL_SYS1PLL_CTRL0,
			.shift   = 0,
			.width   = 8,
		},
		.n = {
			.reg_off = CLKCTRL_SYS1PLL_CTRL0,
			.shift   = 16,
			.width   = 5,
		},
		/* od for 32bit */
		/* set m, n, od in enable callback when set the same rate */
		.od = {
			.reg_off = CLKCTRL_SYS1PLL_CTRL0,
			.shift	 = 12,
			.width	 = 3,
		},
		.table = t3x_sys_pll_params_table,
		.l = {
			.reg_off = CLKCTRL_SYS1PLL_CTRL0,
			.shift   = 31,
			.width   = 1,
		},
		.rst = {
			.reg_off = CLKCTRL_SYS1PLL_CTRL0,
			.shift   = 29,
			.width   = 1,
		},
		.smc_id = SECURE_PLL_CLK,
		.secid_disable = SECID_SYS1_DCO_PLL_DIS,
		.secid = SECID_SYS1_DCO_PLL,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sys1_pll_dco",
		.ops = &meson_secure_pll_v2_ops,
		.parent_data = &(const struct clk_parent_data) {
			.fw_name = "xtal",
		},
		.num_parents = 1,
		/*
		 * Register has the risk of being directly operated
		 */
		.flags = CLK_GET_RATE_NOCACHE | CLK_IGNORE_UNUSED,
	},
};

#ifdef CONFIG_ARM
static const struct pll_params_table t3x_sys2_pll_params_table[] = {
		PLL_PARAMS(69, 1, 1), /* 828M */
		{ /* sentinel */ }
};
#else
static const struct pll_params_table t3x_sys2_pll_params_table[] = {
		PLL_PARAMS(69, 1),
		{ /* sentinel */ }
};
#endif

static struct clk_regmap t3x_sys2_pll_dco = {
	.data = &(struct meson_clk_pll_data){
		.en = {
			.reg_off = CLKCTRL_SYS2PLL_CTRL0,
			.shift   = 28,
			.width   = 1,
		},
		.m = {
			.reg_off = CLKCTRL_SYS2PLL_CTRL0,
			.shift   = 0,
			.width   = 8,
		},
		.n = {
			.reg_off = CLKCTRL_SYS2PLL_CTRL0,
			.shift   = 16,
			.width   = 1, /* keep n = 1 */
		},
		/* od for 32bit */
		/* set m, n, od in enable callback when set the same rate */
		.od = {
			.reg_off = CLKCTRL_SYS2PLL_CTRL0,
			.shift	 = 12,
			.width	 = 3,
		},
		.table = t3x_sys2_pll_params_table,
		.l = {
			.reg_off = CLKCTRL_SYS2PLL_CTRL0,
			.shift   = 31,
			.width   = 1,
		},
		.rst = {
			.reg_off = CLKCTRL_SYS2PLL_CTRL0,
			.shift   = 29,
			.width   = 1,
		},
		.smc_id = SECURE_PLL_CLK,
		.secid_disable = SECID_SYS2_DCO_PLL_DIS,
		.secid = SECID_SYS2_DCO_PLL,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sys2_pll_dco",
		.ops = &meson_secure_pll_v2_ops,
		.parent_data = &(const struct clk_parent_data) {
			.fw_name = "xtal",
		},
		.num_parents = 1,
		/*
		 * Register has the risk of being directly operated
		 */
		.flags = CLK_GET_RATE_NOCACHE | CLK_IGNORE_UNUSED,
	},
};

#ifdef CONFIG_ARM
static const struct pll_params_table t3x_sys3_pll_params_table[] = {
	PLL_PARAMS(100, 1, 1), /*DCO=2400M OD=DCO/2=1200M*/
	PLL_PARAMS(125, 1, 1), /*DCO=3000M OD=DCO/2=1500M*/
	{ /* sentinel */ }
};
#else
static const struct pll_params_table t3x_sys3_pll_params_table[] = {
	PLL_PARAMS(100, 1), /*DCO=2400M OD=DCO/2=1200M*/
	PLL_PARAMS(125, 1), /*DCO=3000M OD=DCO/2=1500M*/
	{ /* sentinel */ }
};
#endif

static struct clk_regmap t3x_sys3_pll_dco = {
	.data = &(struct meson_clk_pll_data){
		.en = {
			.reg_off = CLKCTRL_SYS3PLL_CTRL0,
			.shift   = 28,
			.width   = 1,
		},
		.m = {
			.reg_off = CLKCTRL_SYS3PLL_CTRL0,
			.shift   = 0,
			.width   = 8,
		},
		.n = {
			.reg_off = CLKCTRL_SYS3PLL_CTRL0,
			.shift   = 16,
			.width   = 5,
		},
		/* od for 32bit */
		/* set m, n, od in enable callback when set the same rate */
		.od = {
			.reg_off = CLKCTRL_SYS3PLL_CTRL0,
			.shift	 = 12,
			.width	 = 3,
		},
		.table = t3x_sys3_pll_params_table,
		.l = {
			.reg_off = CLKCTRL_SYS3PLL_CTRL0,
			.shift   = 31,
			.width   = 1,
		},
		.rst = {
			.reg_off = CLKCTRL_SYS3PLL_CTRL0,
			.shift   = 29,
			.width   = 1,
		},
		.smc_id = SECURE_PLL_CLK,
		.secid_disable = SECID_SYS3_DCO_PLL_DIS,
		.secid = SECID_SYS3_DCO_PLL,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sys3_pll_dco",
		.ops = &meson_secure_pll_v2_ops,
		.parent_data = &(const struct clk_parent_data) {
			.fw_name = "xtal",
		},
		.num_parents = 1,
		/*
		 * Register has the risk of being directly operated
		 */
		.flags = CLK_GET_RATE_NOCACHE | CLK_IGNORE_UNUSED,
	},
};

/* od can not set 5/6/7 in sys0/sys1/fixed/gp0/hifi/hifi1 pll */
static const struct clk_div_table t3x_pll_od_tab[] = {
	{0, 1},
	{1, 2},
	{2, 4},
	{3, 8},
	{4, 16},
	{ /* sentinel */ }
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
 *    it will be a lot of mass because of unit difference.
 *
 * Keep Consistent with 64bit, creat a Virtual clock for sys pll
 */
static struct clk_regmap t3x_sys_pll = {
	.hw.init = &(struct clk_init_data){
		.name = "sys_pll",
		.ops = &meson_pll_clk_no_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_sys_pll_dco.hw
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

static struct clk_regmap t3x_sys1_pll = {
	.hw.init = &(struct clk_init_data){
		.name = "sys1_pll",
		.ops = &meson_pll_clk_no_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_sys1_pll_dco.hw
		},
		.num_parents = 1,
		/*
		 * sys1 pll may be initialized in the bootloader
		 * CLK_IGNORE_UNUSED is needed to prevent the system
		 * hang up which will be called by clk_disable_unused
		 */
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};
#else
static struct clk_regmap t3x_sys_pll = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_SYS0PLL_CTRL0,
		.shift = 12,
		.width = 3,
		.table = t3x_pll_od_tab,
		.smc_id = SECURE_PLL_CLK,
		.secid = SECID_SYS0_PLL_OD
	},
	.hw.init = &(struct clk_init_data){
		.name = "sys_pll",
		.ops = &clk_regmap_secure_v2_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_sys_pll_dco.hw
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

static struct clk_regmap t3x_sys1_pll = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_SYS1PLL_CTRL0,
		.shift = 12,
		.width = 3,
		.table = t3x_pll_od_tab,
		.smc_id = SECURE_PLL_CLK,
		.secid = SECID_SYS1_PLL_OD
	},
	.hw.init = &(struct clk_init_data){
		.name = "sys1_pll",
		.ops = &clk_regmap_secure_v2_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_sys1_pll_dco.hw
		},
		.num_parents = 1,
		/*
		 * sys pll is used for other module, the parent
		 * rate needs to be modified
		 * Register has the risk of being directly operated
		 */
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap t3x_sys2_pll = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_SYS2PLL_CTRL0,
		.shift = 12,
		.width = 3,
		.table = t3x_pll_od_tab,
		.smc_id = SECURE_PLL_CLK,
		.secid = SECID_SYS2_PLL_OD
	},
	.hw.init = &(struct clk_init_data){
		.name = "sys2_pll",
		.ops = &clk_regmap_secure_v2_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_sys2_pll_dco.hw
		},
		.num_parents = 1,
		/*
		 * sys pll is used for other module, the parent
		 * rate needs to be modified
		 * Register has the risk of being directly operated
		 */
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap t3x_sys3_pll = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_SYS3PLL_CTRL0,
		.shift = 12,
		.width = 3,
		.table = t3x_pll_od_tab,
		.smc_id = SECURE_PLL_CLK,
		.secid = SECID_SYS3_PLL_OD
	},
	.hw.init = &(struct clk_init_data){
		.name = "sys3_pll",
		.ops = &clk_regmap_secure_v2_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_sys3_pll_dco.hw
		},
		.num_parents = 1,
		/*
		 * sys pll is used for other module, the parent
		 * rate needs to be modified
		 * Register has the risk of being directly operated
		 */
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};
#endif

static struct clk_regmap t3x_fixed_pll_dco = {
	.data = &(struct meson_clk_pll_data){
		.en = {
			.reg_off = CLKCTRL_FIXPLL_CTRL0,
			.shift   = 28,
			.width   = 1,
		},
		.m = {
			.reg_off = CLKCTRL_FIXPLL_CTRL0,
			.shift   = 0,
			.width   = 8,
		},
		.n = {
			.reg_off = CLKCTRL_FIXPLL_CTRL0,
			.shift   = 16,
			.width   = 5,
		},
		.l = {
			.reg_off = CLKCTRL_FIXPLL_CTRL0,
			.shift   = 31,
			.width   = 1,
		},
		.od = {
			.reg_off = CLKCTRL_FIXPLL_CTRL0,
			.shift	 = 12,
			.width	 = 3,
		},
		.rst = {
			.reg_off = CLKCTRL_FIXPLL_CTRL0,
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

#ifdef CONFIG_ARM
static struct clk_regmap t3x_fixed_pll = {
	.hw.init = &(struct clk_init_data){
		.name = "fixed_pll",
		.ops = &meson_pll_clk_no_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_fixed_pll_dco.hw
		},
		.num_parents = 1,
	},
};
#else
static struct clk_regmap t3x_fixed_pll = {
	.data = &(struct clk_regmap_div_data) {
		.offset = CLKCTRL_FIXPLL_CTRL0,
		.shift = 12,
		.width = 3,
		.table = t3x_pll_od_tab,
		.flags = CLK_DIVIDER_POWER_OF_TWO,
	},
	.hw.init = &(struct clk_init_data){
		.name = "fixed_pll",
		.ops = &clk_regmap_divider_ro_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_fixed_pll_dco.hw
		},
		.num_parents = 1,
	},
};
#endif

static struct clk_fixed_factor t3x_fclk_div2_div = {
	.mult = 1,
	.div = 2,
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div2_div",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_fixed_pll.hw },
		.num_parents = 1,
	},
};

static struct clk_regmap t3x_fclk_div2 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_FIXPLL_CTRL1,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div2",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_fclk_div2_div.hw
		},
		.num_parents = 1,
	},
};

static struct clk_fixed_factor t3x_fclk_div3_div = {
	.mult = 1,
	.div = 3,
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div3_div",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_fixed_pll.hw },
		.num_parents = 1,
	},
};

static struct clk_regmap t3x_fclk_div3 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_FIXPLL_CTRL1,
		.bit_idx = 20,
	},
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div3",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_fclk_div3_div.hw
		},
		.num_parents = 1,
	},
};

static struct clk_fixed_factor t3x_fclk_div4_div = {
	.mult = 1,
	.div = 4,
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div4_div",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_fixed_pll.hw },
		.num_parents = 1,
	},
};

static struct clk_regmap t3x_fclk_div4 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_FIXPLL_CTRL1,
		.bit_idx = 21,
	},
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div4",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_fclk_div4_div.hw
		},
		.num_parents = 1,
	},
};

static struct clk_fixed_factor t3x_fclk_div5_div = {
	.mult = 1,
	.div = 5,
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div5_div",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_fixed_pll.hw },
		.num_parents = 1,
	},
};

static struct clk_regmap t3x_fclk_div5 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_FIXPLL_CTRL1,
		.bit_idx = 22,
	},
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div5",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_fclk_div5_div.hw
		},
		.num_parents = 1,
	},
};

static struct clk_fixed_factor t3x_fclk_div7_div = {
	.mult = 1,
	.div = 7,
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div7_div",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_fixed_pll.hw },
		.num_parents = 1,
	},
};

static struct clk_regmap t3x_fclk_div7 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_FIXPLL_CTRL1,
		.bit_idx = 23,
	},
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div7",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_fclk_div7_div.hw
		},
		.num_parents = 1,
	},
};

static struct clk_fixed_factor t3x_fclk_div2p5_div = {
	.mult = 2,
	.div = 5,
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div2p5_div",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_fixed_pll.hw
		},
		.num_parents = 1,
	},
};

static struct clk_regmap t3x_fclk_div2p5 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_FIXPLL_CTRL1,
		.bit_idx = 25,
	},
	.hw.init = &(struct clk_init_data){
		.name = "fclk_div2p5",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_fclk_div2p5_div.hw
		},
		.num_parents = 1,
	},
};

#ifdef CONFIG_ARM
static const struct pll_params_table t3x_gp0_pll_table[] = {
	PLL_PARAMS(128, 1, 2), /* DCO = 3072M OD = 2 PLL = 768M */
	PLL_PARAMS(96, 1, 1), /* DCO = 2304M OD = 1 PLL = 1152M */
	PLL_PARAMS(128, 1, 1), /* DCO = 3072M OD = 1 PLL = 1536M */
	{ /* sentinel */  }
};
#else
static const struct pll_params_table t3x_gp0_pll_table[] = {
	PLL_PARAMS(128, 1), /* DCO = 3072M OD = 2 PLL = 768M */
	PLL_PARAMS(96, 1), /* DCO = 2304M OD = 1 PLL = 1152M */
	{ /* sentinel */  }
};
#endif

/*
 * Internal gp0 pll emulation configuration parameters
 */
static const struct reg_sequence t3x_gp0_init_regs[] = {
	{ .reg = CLKCTRL_GP0PLL_CTRL1,	.def = 0x03a00000 },
	{ .reg = CLKCTRL_GP0PLL_CTRL2,	.def = 0x00040000 },
	{ .reg = CLKCTRL_GP0PLL_CTRL3,	.def = 0x090da200 },
};

static struct clk_regmap t3x_gp0_pll_dco = {
	.data = &(struct meson_clk_pll_data){
		.en = {
			.reg_off = CLKCTRL_GP0PLL_CTRL0,
			.shift   = 28,
			.width   = 1,
		},
		.m = {
			.reg_off = CLKCTRL_GP0PLL_CTRL0,
			.shift   = 0,
			.width   = 9,
		},
		.n = {
			.reg_off = CLKCTRL_GP0PLL_CTRL0,
			.shift   = 16,
			.width   = 5,
		},
#ifdef CONFIG_ARM
		/* for 32bit */
		.od = {
			.reg_off = CLKCTRL_GP0PLL_CTRL0,
			.shift	 = 10,
			.width	 = 3,
		},
#endif
		.l = {
			.reg_off = CLKCTRL_GP0PLL_CTRL0,
			.shift   = 31,
			.width   = 1,
		},
		.rst = {
			.reg_off = CLKCTRL_GP0PLL_CTRL0,
			.shift   = 29,
			.width   = 1,
		},
		.table = t3x_gp0_pll_table,
		.init_regs = t3x_gp0_init_regs,
		.init_count = ARRAY_SIZE(t3x_gp0_init_regs),
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
static const struct pll_params_table t3x_gp1_pll_table[] = {
	PLL_PARAMS(124, 1, 2), /* DCO = 2976M OD = 2 PLL = 744M */
	PLL_PARAMS(128, 1, 2), /* DCO = 3072M OD = 2 PLL = 768M */
	{ /* sentinel */  }
};
#else
static const struct pll_params_table t3x_gp1_pll_table[] = {
	PLL_PARAMS(124, 1), /* DCO = 2976M OD = 2 PLL = 744M */
	PLL_PARAMS(128, 1), /* DCO = 3072M OD = 2 PLL = 768M */
	{ /* sentinel */  }
};
#endif

static const struct reg_sequence t3x_gp1_init_regs[] = {
	{ .reg = CLKCTRL_GP1PLL_CTRL1,	.def = 0x03a00000 },
	{ .reg = CLKCTRL_GP1PLL_CTRL2,	.def = 0x00040000 },
	{ .reg = CLKCTRL_GP1PLL_CTRL3,	.def = 0x090da200 },
};

static struct clk_regmap t3x_gp1_pll_dco = {
	.data = &(struct meson_clk_pll_data){
		.en = {
			.reg_off = CLKCTRL_GP1PLL_CTRL0,
			.shift   = 28,
			.width   = 1,
		},
		.m = {
			.reg_off = CLKCTRL_GP1PLL_CTRL0,
			.shift   = 0,
			.width   = 9,
		},
		.n = {
			.reg_off = CLKCTRL_GP1PLL_CTRL0,
			.shift   = 16,
			.width   = 5,
		},
#ifdef CONFIG_ARM
		/* for 32bit */
		.od = {
			.reg_off = CLKCTRL_GP1PLL_CTRL0,
			.shift	 = 10,
			.width	 = 3,
		},
#endif
		.l = {
			.reg_off = CLKCTRL_GP1PLL_CTRL0,
			.shift   = 31,
			.width   = 1,
		},
		.rst = {
			.reg_off = CLKCTRL_GP1PLL_CTRL0,
			.shift   = 29,
			.width   = 1,
		},
		.table = t3x_gp1_pll_table,
		.init_regs = t3x_gp1_init_regs,
		.init_count = ARRAY_SIZE(t3x_gp1_init_regs),
	},
	.hw.init = &(struct clk_init_data){
		.name = "gp1_pll_dco",
		.ops = &meson_clk_pll_ops,
		.parent_data = &(const struct clk_parent_data) {
			.fw_name = "xtal",
		},
		.num_parents = 1,
	},
};

#ifdef CONFIG_ARM
static struct clk_regmap t3x_gp0_pll = {
	.hw.init = &(struct clk_init_data){
		.name = "gp0_pll",
		.ops = &meson_pll_clk_no_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_gp0_pll_dco.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_gp1_pll = {
	.hw.init = &(struct clk_init_data){
		.name = "gp1_pll",
		.ops = &meson_pll_clk_no_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_gp1_pll_dco.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};
#else
static struct clk_regmap t3x_gp0_pll = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_GP0PLL_CTRL0,
		.shift = 10,
		.width = 3,
		.table = t3x_pll_od_tab,
		.flags = CLK_DIVIDER_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "gp0_pll",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_gp0_pll_dco.hw
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

static struct clk_regmap t3x_gp1_pll = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_GP1PLL_CTRL0,
		.shift = 10,
		.width = 3,
		.table = t3x_pll_od_tab,
		.flags = CLK_DIVIDER_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "gp1_pll",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_gp1_pll_dco.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};
#endif

static const struct reg_sequence t3x_pcie_pll_init_regs[] = {
	{ .reg = CLKCTRL_PCIEPLL_CTRL0,	.def = 0x000f7d06,},
	{ .reg = CLKCTRL_PCIEPLL_CTRL1,	.def = 0xf0002dd3,},
	{ .reg = CLKCTRL_PCIEPLL_CTRL2,	.def = 0x55813041, .delay_us = 20 },
	{ .reg = CLKCTRL_PCIEPLL_CTRL0,	.def = 0x000f7d07, .delay_us = 20 },
	{ .reg = CLKCTRL_PCIEPLL_CTRL0,	.def = 0x000f7d01, .delay_us = 20 },
	{ .reg = CLKCTRL_PCIEPLL_CTRL3,	.def = 0x000300e1,}
};

static struct clk_regmap t3x_pcie_pll = {
	.data = &(struct meson_clk_pll_data){
		.l = {
			.reg_off = CLKCTRL_PCIEPLL_CTRL0,
			.shift   = 31,
			.width   = 1,
		},
		.init_regs = t3x_pcie_pll_init_regs,
		.init_count = ARRAY_SIZE(t3x_pcie_pll_init_regs),
	},
	.hw.init = &(struct clk_init_data){
		.name = "pcie_pll",
		.ops = &meson_clk_pcie_pll_ops,
		.parent_data = &(const struct clk_parent_data) {
			.fw_name = "xtal",
		},
		.num_parents = 1,
	},
};

static struct clk_regmap t3x_pcie_hcsl = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_PCIEPLL_CTRL3,
		.bit_idx = 17,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pcie_hcsl",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_pcie_pll.hw
		},
		.num_parents = 1,
	},
};

/* a55 cpu_clk, get the table from ucode */
static const struct cpu_dyn_table t3x_cpu_dyn_table[] = {
	CPU_LOW_PARAMS(500000000, 1, 1, 1),
	CPU_LOW_PARAMS(666666666, 2, 0, 0),
	CPU_LOW_PARAMS(1000000000, 1, 0, 0)
};

static const struct clk_parent_data t3x_cpu_dyn_clk_sel[] = {
	{ .fw_name = "xtal", },
	{ .hw = &t3x_fclk_div2.hw },
	{ .hw = &t3x_fclk_div3.hw },
	{ .hw = &t3x_fclk_div2p5.hw }
};

static struct clk_regmap t3x_cpu_dyn_clk = {
	.data = &(struct meson_clk_cpu_dyn_data){
		.table = t3x_cpu_dyn_table,
		.table_cnt = ARRAY_SIZE(t3x_cpu_dyn_table),
		.smc_id = SECURE_CPU_CLK,
		.secid_dyn_rd = SECID_CPU_CLK_RD,
		.secid_dyn = SECID_CPU_CLK_DYN,
	},
	.hw.init = &(struct clk_init_data){
		.name = "cpu_dyn_clk",
		.ops = &meson_clk_cpu_dyn_ops,
		.parent_data = t3x_cpu_dyn_clk_sel,
		.num_parents = ARRAY_SIZE(t3x_cpu_dyn_clk_sel),
	},
};

static struct clk_regmap t3x_cpu_clk = {
	.data = &(struct clk_regmap_mux_data){
		.mask = 0x1,
		.shift = 11,
		.smc_id = SECURE_CPU_CLK,
		.secid = SECID_CPU_CLK_SEL,
		.secid_rd = SECID_CPU_CLK_RD
	},
	.hw.init = &(struct clk_init_data){
		.name = "cpu_clk",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_cpu_dyn_clk.hw,
			&t3x_sys_pll.hw,
		},
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t3x_a76_dyn_clk = {
	.data = &(struct meson_clk_cpu_dyn_data){
		.table = t3x_cpu_dyn_table,
		.table_cnt = ARRAY_SIZE(t3x_cpu_dyn_table),
		.smc_id = SECURE_CPU_CLK,
		.secid_dyn_rd = SECID_A76_CLK_RD,
		.secid_dyn = SECID_A76_CLK_DYN,
	},
	.hw.init = &(struct clk_init_data){
		.name = "a76_dyn_clk",
		.ops = &meson_clk_cpu_dyn_ops,
		.parent_data = t3x_cpu_dyn_clk_sel,
		.num_parents = 4,
	},
};

static struct clk_regmap t3x_a76_clk = {
	.data = &(struct clk_regmap_mux_data){
		.mask = 0x1,
		.shift = 11,
		.smc_id = SECURE_CPU_CLK,
		.secid = SECID_A76_CLK_SEL,
		.secid_rd = SECID_A76_CLK_RD
	},
	.hw.init = &(struct clk_init_data){
		.name = "a76_clk",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_a76_dyn_clk.hw,
			&t3x_sys1_pll.hw,
		},
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t3x_dsu_dyn_clk = {
	.data = &(struct meson_clk_cpu_dyn_data){
		.table = t3x_cpu_dyn_table,
		.table_cnt = ARRAY_SIZE(t3x_cpu_dyn_table),
		.smc_id = SECURE_CPU_CLK,
		.secid_dyn_rd = SECID_DSU_CLK_RD,
		.secid_dyn = SECID_DSU_CLK_DYN,
	},
	.hw.init = &(struct clk_init_data){
		.name = "dsu_dyn_clk",
		.ops = &meson_clk_cpu_dyn_ops,
		.parent_data = t3x_cpu_dyn_clk_sel,
		.num_parents = 4,
	},
};

static struct clk_regmap t3x_dsu_clk = {
	.data = &(struct clk_regmap_mux_data){
		.mask = 0x1,
		.shift = 11,
		.smc_id = SECURE_CPU_CLK,
		.secid = SECID_DSU_CLK_SEL,
		.secid_rd = SECID_DSU_CLK_RD,
	},
	.hw.init = &(struct clk_init_data){
		.name = "dsu_clk",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_dsu_dyn_clk.hw,
			&t3x_sys3_pll.hw,
		},
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_dsu_final_clk = {
	.data = &(struct clk_regmap_mux_data){
		.mask = 0x1,
		.shift = 31,
		.smc_id = SECURE_CPU_CLK,
		.secid = SECID_DSU_FINAL_CLK_SEL,
		.secid_rd = SECID_DSU_FINAL_CLK_RD,
	},
	.hw.init = &(struct clk_init_data){
		.name = "dsu_final_clk",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_cpu_clk.hw,
			&t3x_dsu_clk.hw,
		},
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

struct t3x_sys_pll_nb_data {
	struct notifier_block nb;
	struct clk_hw *sys_pll;
	struct clk_hw *cpu_clk;
	struct clk_hw *cpu_dyn_clk;
};

static int t3x_sys_pll_notifier_cb(struct notifier_block *nb,
				   unsigned long event, void *data)
{
	struct t3x_sys_pll_nb_data *nb_data =
		container_of(nb, struct t3x_sys_pll_nb_data, nb);

	switch (event) {
	case PRE_RATE_CHANGE:
		/*
		 * This notifier means sys_pll clock will be changed
		 * to feed cpu_clk, this the current path :
		 * cpu_clk
		 *    \- sys_pll
		 *          \- sys_pll_dco
		 */
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

/*
 *static struct t3x_sys_pll_nb_data t3x_sys_pll_nb_data = {
 *	.sys_pll = &t3x_sys_pll.hw,
 *	.cpu_clk = &t3x_cpu_clk.hw,
 *	.cpu_dyn_clk = &t3x_cpu_dyn_clk.hw,
 *	.nb.notifier_call = t3x_sys_pll_notifier_cb,
 *};
 *
 *static struct t3x_sys_pll_nb_data t3x_sys1_pll_nb_data = {
 *	.sys_pll = &t3x_sys1_pll.hw,
 *	.cpu_clk = &t3x_a76_clk.hw,
 *	.cpu_dyn_clk = &t3x_a76_dyn_clk.hw,
 *	.nb.notifier_call = t3x_sys_pll_notifier_cb,
 *};
 */

static struct t3x_sys_pll_nb_data t3x_sys3_pll_nb_data = {
	.sys_pll = &t3x_sys3_pll.hw,
	.cpu_clk = &t3x_dsu_clk.hw,
	.cpu_dyn_clk = &t3x_dsu_dyn_clk.hw,
	.nb.notifier_call = t3x_sys_pll_notifier_cb,
};

static const struct reg_sequence t3x_hifi_init_regs[] = {
	{ .reg = CLKCTRL_HIFIPLL_CTRL1,	.def = 0x03a00000 },
	{ .reg = CLKCTRL_HIFIPLL_CTRL2,	.def = 0x00040000 },
	{ .reg = CLKCTRL_HIFIPLL_CTRL3,	.def = 0x090da200 },
};

static struct clk_regmap t3x_hifi_pll_dco = {
	.data = &(struct meson_clk_pll_data){
		.en = {
			.reg_off = CLKCTRL_HIFIPLL_CTRL0,
			.shift   = 28,
			.width   = 1,
		},
		.n = {
			.reg_off = CLKCTRL_HIFIPLL_CTRL0,
			.shift   = 16,
			.width   = 5,
		},
		.m = {
			.reg_off = CLKCTRL_HIFIPLL_CTRL0,
			.shift   = 0,
			.width   = 8,
		},
		.frac = {
			.reg_off = CLKCTRL_HIFIPLL_CTRL1,
			.shift   = 0,
			.width   = 19,
		},
		.l = {
			.reg_off = CLKCTRL_HIFIPLL_CTRL0,
			.shift   = 31,
			.width   = 1,
		},
		.rst = {
			.reg_off = CLKCTRL_HIFIPLL_CTRL0,
			.shift   = 29,
			.width   = 1,
		},
		.range = &t3x_pll_mult_range,
		.init_regs = t3x_hifi_init_regs,
		.init_count = ARRAY_SIZE(t3x_hifi_init_regs),
		.flags = CLK_MESON_PLL_FIXED_FRAC_WEIGHT_PRECISION,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hifi_pll_dco",
		.ops = &meson_clk_pll_ops,
		.parent_data = &(const struct clk_parent_data) {
			.fw_name = "xtal",
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap t3x_hifi_pll = {
	.data = &(struct clk_regmap_div_data) {
		.offset = CLKCTRL_HIFIPLL_CTRL0,
		.shift = 10,
		.width = 3,
		.table = t3x_pll_od_tab,
		.flags = CLK_DIVIDER_ROUND_CLOSEST
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hifi_pll",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_hifi_pll_dco.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

static const struct reg_sequence t3x_hifi1_init_regs[] = {
	{ .reg = CLKCTRL_HIFI1PLL_CTRL1, .def = 0x03a00000 },
	{ .reg = CLKCTRL_HIFI1PLL_CTRL2, .def = 0x00040000 },
	{ .reg = CLKCTRL_HIFI1PLL_CTRL3, .def = 0x090da200 },
};

static struct clk_regmap t3x_hifi1_pll_dco = {
	.data = &(struct meson_clk_pll_data){
		.en = {
			.reg_off = CLKCTRL_HIFI1PLL_CTRL0,
			.shift   = 28,
			.width   = 1,
		},
		.n = {
			.reg_off = CLKCTRL_HIFI1PLL_CTRL0,
			.shift   = 16,
			.width   = 5,
		},
		.m = {
			.reg_off = CLKCTRL_HIFI1PLL_CTRL0,
			.shift   = 0,
			.width   = 8,
		},
		.frac = {
			.reg_off = CLKCTRL_HIFI1PLL_CTRL1,
			.shift   = 0,
			.width   = 19,
		},
		.l = {
			.reg_off = CLKCTRL_HIFI1PLL_CTRL0,
			.shift   = 31,
			.width   = 1,
		},
		.rst = {
			.reg_off = CLKCTRL_HIFI1PLL_CTRL0,
			.shift   = 29,
			.width   = 1,
		},
		.range = &t3x_pll_mult_range,
		.init_regs = t3x_hifi1_init_regs,
		.init_count = ARRAY_SIZE(t3x_hifi1_init_regs),
		.flags = CLK_MESON_PLL_FIXED_FRAC_WEIGHT_PRECISION,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hifi1_pll_dco",
		.ops = &meson_clk_pll_ops,
		.parent_data = &(const struct clk_parent_data) {
			.fw_name = "xtal",
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap t3x_hifi1_pll = {
	.data = &(struct clk_regmap_div_data) {
		.offset = CLKCTRL_HIFI1PLL_CTRL0,
		.shift = 10,
		.width = 3,
		.table = t3x_pll_od_tab,
		.flags = CLK_DIVIDER_ROUND_CLOSEST
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hifi1_pll",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_hifi1_pll_dco.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_fixed_factor t3x_mpll_50m_div = {
	.mult = 1,
	.div = 80,
	.hw.init = &(struct clk_init_data){
		.name = "mpll_50m_div",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_fixed_pll_dco.hw
		},
		.num_parents = 1,
	},
};

static const struct clk_parent_data t3x_mpll_50m_sel[] = {
	{ .fw_name = "xtal", },
	{ .hw = &t3x_mpll_50m_div.hw }
};

static struct clk_regmap t3x_mpll_50m = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_FIXPLL_CTRL3,
		.mask = 0x1,
		.shift = 5,
	},
	.hw.init = &(struct clk_init_data){
		.name = "mpll_50m",
		.ops = &clk_regmap_mux_ro_ops,
		.parent_data = t3x_mpll_50m_sel,
		.num_parents = ARRAY_SIZE(t3x_mpll_50m_sel),
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
 *	         output N1 and N2 in turn.
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
static struct clk_regmap t3x_rtc_32k_clkin = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_RTC_BY_OSCIN_CTRL0,
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

static const struct meson_clk_dualdiv_param t3x_32k_div_table[] = {
	{
		.dual	= 1,
		.n1	= 733,
		.m1	= 8,
		.n2	= 732,
		.m2	= 11,
	},
	{}
};

static struct clk_regmap t3x_rtc_32k_div = {
	.data = &(struct meson_clk_dualdiv_data){
		.n1 = {
			.reg_off = CLKCTRL_RTC_BY_OSCIN_CTRL0,
			.shift   = 0,
			.width   = 12,
		},
		.n2 = {
			.reg_off = CLKCTRL_RTC_BY_OSCIN_CTRL0,
			.shift   = 12,
			.width   = 12,
		},
		.m1 = {
			.reg_off = CLKCTRL_RTC_BY_OSCIN_CTRL1,
			.shift   = 0,
			.width   = 12,
		},
		.m2 = {
			.reg_off = CLKCTRL_RTC_BY_OSCIN_CTRL1,
			.shift   = 12,
			.width   = 12,
		},
		.dual = {
			.reg_off = CLKCTRL_RTC_BY_OSCIN_CTRL0,
			.shift   = 28,
			.width   = 1,
		},
		.table = t3x_32k_div_table,
	},
	.hw.init = &(struct clk_init_data){
		.name = "rtc_32k_div",
		.ops = &meson_clk_dualdiv_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_rtc_32k_clkin.hw
		},
		.num_parents = 1,
	},
};

static struct clk_regmap t3x_rtc_32k_xtal = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_RTC_BY_OSCIN_CTRL1,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "rtc_32k_xtal",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_rtc_32k_clkin.hw
		},
		.num_parents = 1,
	},
};

/*
 * three parent for rtc clock out
 * pad is from where?
 */
static u32 rtc_32k_sel[] = {0, 1};
static struct clk_regmap t3x_rtc_32k_sel = {
	.data = &(struct clk_regmap_mux_data) {
		.offset = CLKCTRL_RTC_CTRL,
		.mask = 0x3,
		.shift = 0,
		.table = rtc_32k_sel,
		.flags = CLK_MUX_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "rtc_32k_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_rtc_32k_xtal.hw,
			&t3x_rtc_32k_div.hw
		},
		.num_parents = 2,
		/*
		 * rtc 32k is directly used in other modules, and the
		 * parent rate needs to be modified
		 */
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_rtc_clk = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_RTC_BY_OSCIN_CTRL0,
		.bit_idx = 30,
	},
	.hw.init = &(struct clk_init_data){
		.name = "rtc_clk",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_rtc_32k_sel.hw
		},
		.num_parents = 1,
		/*
		 * rtc 32k is directly used in other modules, and the
		 * parent rate needs to be modified
		 */
		.flags = CLK_SET_RATE_PARENT,
	},
};

static const struct cpu_dyn_table t3x_sys_clk_table[] = {
	/* sys clk no need dyn_post_mux */
	CPU_LOW_PARAMS(24000000, 0, 0, 0),
	CPU_LOW_PARAMS(166666666, 3, 1, 2)
};

static const struct clk_parent_data t3x_sys_clk_sel[] = {
	{ .fw_name = "xtal", },
	{ .hw = &t3x_fclk_div2.hw },
	{ .hw = &t3x_fclk_div3.hw },
	{ .hw = &t3x_fclk_div4.hw }
};

static struct clk_regmap t3x_sys_clk = {
	.data = &(struct meson_clk_cpu_dyn_data){
		.table = t3x_sys_clk_table,
		.table_cnt = ARRAY_SIZE(t3x_sys_clk_table),
		.smc_id = SECURE_CPU_CLK,
		.secid_dyn_rd = SECID_SYS_CLK_RD,
		.secid_dyn = SECID_SYS_CLK_DYN,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sys_clk",
		.ops = &meson_sec_sys_clk_ops,
		.parent_data = t3x_sys_clk_sel,
		.num_parents = ARRAY_SIZE(t3x_sys_clk_sel),
	},
};

static const struct cpu_dyn_table t3x_axi_clk_table[] = {
	/* axi clk no need dyn_post_mux */
	CPU_LOW_PARAMS(24000000, 0, 0, 0),
	/* switching 200M, cpu frequency needs switched to 1.2G first */
	CPU_LOW_PARAMS(200000000, 5, 0, 1),
	CPU_LOW_PARAMS(500000000, 3, 0, 0)
};

static const struct clk_parent_data t3x_axi_clk_sel[] = {
	{ .fw_name = "xtal", },
	{ .hw = &t3x_fclk_div2.hw },
	{ .hw = &t3x_fclk_div3.hw },
	{ .hw = &t3x_fclk_div4.hw },
	{ .hw = &t3x_fclk_div5.hw }
};

static struct clk_regmap t3x_axi_clk = {
	.data = &(struct meson_clk_cpu_dyn_data){
		.table = t3x_axi_clk_table,
		.table_cnt = ARRAY_SIZE(t3x_axi_clk_table),
		.smc_id = SECURE_CPU_CLK,
		.secid_dyn_rd = SECID_AXI_CLK_RD,
		.secid_dyn = SECID_AXI_CLK_DYN,
	},
	.hw.init = &(struct clk_init_data){
		.name = "axi_clk",
		.ops = &meson_sec_sys_clk_ops,
		.parent_data = t3x_axi_clk_sel,
		.num_parents = ARRAY_SIZE(t3x_axi_clk_sel),
	},
};

static struct clk_regmap t3x_ceca_32k_clkin = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_CECA_CTRL0,
		.bit_idx = 31,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "ceca_32k_clkin",
		.ops = &clk_regmap_gate_ops,
		.parent_data = &(const struct clk_parent_data) {
			.fw_name = "xtal",
		},
		.num_parents = 1,
	},
};

static struct clk_regmap t3x_ceca_32k_div = {
	.data = &(struct meson_clk_dualdiv_data){
		.n1 = {
			.reg_off = CLKCTRL_CECA_CTRL0,
			.shift   = 0,
			.width   = 12,
		},
		.n2 = {
			.reg_off = CLKCTRL_CECA_CTRL0,
			.shift   = 12,
			.width   = 12,
		},
		.m1 = {
			.reg_off = CLKCTRL_CECA_CTRL1,
			.shift   = 0,
			.width   = 12,
		},
		.m2 = {
			.reg_off = CLKCTRL_CECA_CTRL1,
			.shift   = 12,
			.width   = 12,
		},
		.dual = {
			.reg_off = CLKCTRL_CECA_CTRL0,
			.shift   = 28,
			.width   = 1,
		},
		.table = t3x_32k_div_table,
	},
	.hw.init = &(struct clk_init_data){
		.name = "ceca_32k_div",
		.ops = &meson_clk_dualdiv_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_ceca_32k_clkin.hw
		},
		.num_parents = 1,
	},
};

static struct clk_regmap t3x_ceca_32k_sel_pre = {
	.data = &(struct clk_regmap_mux_data) {
		.offset = CLKCTRL_CECA_CTRL1,
		.mask = 0x1,
		.shift = 24,
		.flags = CLK_MUX_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "ceca_32k_sel_pre",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_ceca_32k_div.hw,
			&t3x_ceca_32k_clkin.hw
		},
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_ceca_32k_sel = {
	.data = &(struct clk_regmap_mux_data) {
		.offset = CLKCTRL_CECA_CTRL1,
		.mask = 0x1,
		.shift = 31,
		.flags = CLK_MUX_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "ceca_32k_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_ceca_32k_sel_pre.hw,
			&t3x_rtc_clk.hw
		},
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_ceca_32k_clkout = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_CECA_CTRL0,
		.bit_idx = 30,
	},
	.hw.init = &(struct clk_init_data){
		.name = "ceca_32k_clkout",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_ceca_32k_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

/*cecb_clk*/
static struct clk_regmap t3x_cecb_32k_clkin = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_CECB_CTRL0,
		.bit_idx = 31,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "cecb_32k_clkin",
		.ops = &clk_regmap_gate_ops,
		.parent_data = &(const struct clk_parent_data) {
			.fw_name = "xtal",
		},
		.num_parents = 1,
	},
};

static struct clk_regmap t3x_cecb_32k_div = {
	.data = &(struct meson_clk_dualdiv_data){
		.n1 = {
			.reg_off = CLKCTRL_CECB_CTRL0,
			.shift   = 0,
			.width   = 12,
		},
		.n2 = {
			.reg_off = CLKCTRL_CECB_CTRL0,
			.shift   = 12,
			.width   = 12,
		},
		.m1 = {
			.reg_off = CLKCTRL_CECB_CTRL1,
			.shift   = 0,
			.width   = 12,
		},
		.m2 = {
			.reg_off = CLKCTRL_CECB_CTRL1,
			.shift   = 12,
			.width   = 12,
		},
		.dual = {
			.reg_off = CLKCTRL_CECB_CTRL0,
			.shift   = 28,
			.width   = 1,
		},
		.table = t3x_32k_div_table,
	},
	.hw.init = &(struct clk_init_data){
		.name = "cecb_32k_div",
		.ops = &meson_clk_dualdiv_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_cecb_32k_clkin.hw
		},
		.num_parents = 1,
	},
};

static struct clk_regmap t3x_cecb_32k_sel_pre = {
	.data = &(struct clk_regmap_mux_data) {
		.offset = CLKCTRL_CECB_CTRL1,
		.mask = 0x1,
		.shift = 24,
		.flags = CLK_MUX_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "cecb_32k_sel_pre",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_cecb_32k_div.hw,
			&t3x_cecb_32k_clkin.hw
		},
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_cecb_32k_sel = {
	.data = &(struct clk_regmap_mux_data) {
		.offset = CLKCTRL_CECB_CTRL1,
		.mask = 0x1,
		.shift = 31,
		.flags = CLK_MUX_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "cecb_32k_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_cecb_32k_sel_pre.hw,
			&t3x_rtc_clk.hw
		},
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_cecb_32k_clkout = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_CECB_CTRL0,
		.bit_idx = 30,
	},
	.hw.init = &(struct clk_init_data){
		.name = "cecb_32k_clkout",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_cecb_32k_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static u32 t3x_sc_mux_table[] = {0, 1, 2, 3, 4, 7};

static const struct clk_parent_data t3x_sc_parent_data[] = {
	{ .hw = &t3x_fclk_div2.hw },
	{ .hw = &t3x_fclk_div3.hw },
	{ .hw = &t3x_fclk_div5.hw },
	{ .fw_name = "xtal", },
	{ .hw = &t3x_fclk_div5.hw },
	{ .hw = &t3x_fclk_div7.hw },
};

static struct clk_regmap t3x_sc_clk_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_SC_CLK_CTRL,
		.mask = 0x7,
		.shift = 9,
		.table = t3x_sc_mux_table
	},
	.hw.init = &(struct clk_init_data) {
		.name = "sc_clk_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_sc_parent_data,
		.num_parents = ARRAY_SIZE(t3x_sc_parent_data),
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap t3x_sc_clk_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_SC_CLK_CTRL,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "sc_clk_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_sc_clk_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_sc_clk = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_SC_CLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sc_clk",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_sc_clk_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static const struct clk_parent_data t3x_dsp_parent_hws[] = {
	{ .fw_name = "xtal", },
	{ .hw = &t3x_fclk_div2p5.hw },
	{ .hw = &t3x_fclk_div3.hw },
	{ .hw = &t3x_fclk_div5.hw },
	{ .hw = &t3x_hifi_pll.hw },
	{ .hw = &t3x_fclk_div4.hw },
	{ .hw = &t3x_fclk_div7.hw },
	{ .hw = &t3x_rtc_clk.hw }
};

static struct clk_regmap t3x_dspa_0_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_DSPA_CLK_CTRL0,
		.mask = 0x7,
		.shift = 10,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "dspa_0_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_dsp_parent_hws,
		.num_parents = ARRAY_SIZE(t3x_dsp_parent_hws),
	},
};

static struct clk_regmap t3x_dspa_0_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_DSPA_CLK_CTRL0,
		.shift = 0,
		.width = 10,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "dspa_0_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_dspa_0_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_dspa_0 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_DSPA_CLK_CTRL0,
		.bit_idx = 13,
	},
	.hw.init = &(struct clk_init_data){
		.name = "dspa_0",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_dspa_0_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_dspa_1_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_DSPA_CLK_CTRL0,
		.mask = 0x7,
		.shift = 26,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "dspa_1_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_dsp_parent_hws,
		.num_parents = ARRAY_SIZE(t3x_dsp_parent_hws),
	},
};

static struct clk_regmap t3x_dspa_1_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_DSPA_CLK_CTRL0,
		.shift = 16,
		.width = 10,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "dspa_1_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_dspa_1_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_dspa_1 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_DSPA_CLK_CTRL0,
		.bit_idx = 29,
	},
	.hw.init = &(struct clk_init_data){
		.name = "dspa_1",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_dspa_1_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_dspa = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_DSPA_CLK_CTRL0,
		.mask = 0x1,
		.shift = 15,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "dspa",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_dspa_0.hw,
			&t3x_dspa_1.hw,
		},
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

/*12_24M clk*/
static struct clk_regmap t3x_24m_clk_gate = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_CLK12_24_CTRL,
		.bit_idx = 11,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "24m_clk_gate",
		.ops = &clk_regmap_gate_ops,
		.parent_data = &(const struct clk_parent_data) {
			.fw_name = "xtal",
		},
		.num_parents = 1,
	},
};

static struct clk_fixed_factor t3x_24m_div2 = {
	.mult = 1,
	.div = 2,
	.hw.init = &(struct clk_init_data){
		.name = "24m_div2",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_24m_clk_gate.hw
		},
		.num_parents = 1,
	},
};

static struct clk_regmap t3x_12m_clk = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_CLK12_24_CTRL,
		.bit_idx = 10,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "12m_clk",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_24m_div2.hw
		},
		.num_parents = 1,
	},
};

static struct clk_regmap t3x_25m_clk_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_CLK12_24_CTRL,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "25m_clk_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_fclk_div2.hw
		},
		.num_parents = 1,
	},
};

static struct clk_regmap t3x_25m_clk = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_CLK12_24_CTRL,
		.bit_idx = 12,
	},
	.hw.init = &(struct clk_init_data){
		.name = "25m_clk",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_25m_clk_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static u32 t3x_vapb_table[] = { 0, 1, 2, 3, 7};
static const struct clk_hw *t3x_vapb_parent_hws[] = {
	&t3x_fclk_div4.hw,
	&t3x_fclk_div3.hw,
	&t3x_fclk_div5.hw,
	&t3x_fclk_div7.hw,
	&t3x_fclk_div2p5.hw
};

static struct clk_regmap t3x_vapb_0_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_VAPBCLK_CTRL,
		.mask = 0x7,
		.shift = 9,
		.table = t3x_vapb_table
	},
	.hw.init = &(struct clk_init_data){
		.name = "vapb_0_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = t3x_vapb_parent_hws,
		.num_parents = ARRAY_SIZE(t3x_vapb_parent_hws),
	},
};

static struct clk_regmap t3x_vapb_0_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_VAPBCLK_CTRL,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vapb_0_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_vapb_0_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_vapb_0 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_VAPBCLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vapb_0",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_vapb_0_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap t3x_vapb_1_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_VAPBCLK_CTRL,
		.mask = 0x7,
		.shift = 25,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vapb_1_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = t3x_vapb_parent_hws,
		.num_parents = ARRAY_SIZE(t3x_vapb_parent_hws),
		.flags = CLK_SET_RATE_NO_REPARENT,
	},
};

static struct clk_regmap t3x_vapb_1_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_VAPBCLK_CTRL,
		.shift = 16,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vapb_1_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_vapb_1_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_vapb_1 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_VAPBCLK_CTRL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vapb_1",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_vapb_1_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap t3x_vapb = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_VAPBCLK_CTRL,
		.mask = 1,
		.shift = 31,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vapb_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_vapb_0.hw,
			&t3x_vapb_1.hw
		},
		.num_parents = 2,
		.flags = CLK_SET_RATE_NO_REPARENT | CLK_SET_RATE_PARENT,
	},
};

static u32 t3x_ge2d_table[] = { 0, 1, 2, 5};

static const struct clk_hw *t3x_ge2d_parent_hws[] = {
	&t3x_vapb.hw,
	&t3x_fclk_div2p5.hw,
	&t3x_fclk_div3.hw,
	&t3x_fclk_div4.hw,
};

static struct clk_regmap t3x_ge2d_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_GE2DCLK_CTRL,
		.mask = 0x7,
		.shift = 9,
		.table = t3x_ge2d_table
	},
	.hw.init = &(struct clk_init_data){
		.name = "ge2d_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = t3x_ge2d_parent_hws,
		.num_parents = ARRAY_SIZE(t3x_ge2d_parent_hws),
	},
};

static struct clk_regmap t3x_ge2d_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_GE2DCLK_CTRL,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "ge2d_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_ge2d_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_ge2d = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_GE2DCLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "ge2d",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_ge2d_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static u32 t3x_adc_extclk_mux_table[] = { 0, 1, 2, 3, 4 };

static const struct clk_parent_data t3x_adc_extclk_parent_data[] = {
	{ .fw_name = "xtal", },
	{ .hw = &t3x_fclk_div4.hw },
	{ .hw = &t3x_fclk_div3.hw },
	{ .hw = &t3x_fclk_div5.hw },
	{ .hw = &t3x_fclk_div7.hw },
};

static struct clk_regmap t3x_adc_extclk_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_DEMOD_CLK_CNTL,
		.mask = 0x7,
		.shift = 25,
		.table = t3x_adc_extclk_mux_table
	},
	.hw.init = &(struct clk_init_data) {
		.name = "adc_extclk_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_adc_extclk_parent_data,
		.num_parents = ARRAY_SIZE(t3x_adc_extclk_parent_data),
	},
};

static struct clk_regmap t3x_adc_extclk_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_DEMOD_CLK_CNTL,
		.shift = 16,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "adc_extclk_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_adc_extclk_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_adc_extclk = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_DEMOD_CLK_CNTL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data){
		.name = "adc_extclk",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_adc_extclk_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static u32 t3x_vafe_mux_table[] = { 1 };

static struct clk_regmap t3x_vafe_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_ATV_DMD_SYS_CLK_CNTL,
		.mask = 0x3,
		.shift = 24,
		.table = t3x_vafe_mux_table
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

static struct clk_regmap t3x_vafe_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_ATV_DMD_SYS_CLK_CNTL,
		.shift = 16,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vafe_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_vafe_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_vafe = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_ATV_DMD_SYS_CLK_CNTL,
		.bit_idx = 23,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vafe",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_vafe_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static u32 t3x_demod_core_mux_table[] = { 0, 1, 2 };

static const struct clk_parent_data t3x_cts_demod_core_parent_data[] = {
	{ .fw_name = "xtal", },
	{ .hw = &t3x_fclk_div7.hw },
	{ .hw = &t3x_fclk_div4.hw },
};

static struct clk_regmap t3x_cts_demod_core_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_DEMOD_CLK_CNTL,
		.mask = 0x3,
		.shift = 9,
		.table = t3x_demod_core_mux_table
	},
	.hw.init = &(struct clk_init_data) {
		.name = "cts_demod_core_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_cts_demod_core_parent_data,
		.num_parents = ARRAY_SIZE(t3x_cts_demod_core_parent_data),
	},
};

static struct clk_regmap t3x_cts_demod_core_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_DEMOD_CLK_CNTL,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "cts_demod_core_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_cts_demod_core_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_cts_demod_core = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_DEMOD_CLK_CNTL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "cts_demod_core",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_cts_demod_core_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static u32 t3x_demod_core_t2_mux_table[] = { 0, 1, 2 };

static const struct clk_parent_data t3x_cts_demod_core_t2_parent_data[] = {
	{ .fw_name = "xtal", },
	{ .hw = &t3x_fclk_div5.hw },
	{ .hw = &t3x_fclk_div4.hw },
};

static struct clk_regmap t3x_cts_demod_core_t2_clk_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_DEMOD_CLK_CNTL1,
		.mask = 0x3,
		.shift = 9,
		.table = t3x_demod_core_t2_mux_table
	},
	.hw.init = &(struct clk_init_data) {
		.name = "cts_demod_core_t2_clk_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_cts_demod_core_t2_parent_data,
		.num_parents = ARRAY_SIZE(t3x_cts_demod_core_t2_parent_data),
	},
};

static struct clk_regmap t3x_cts_demod_core_t2_clk_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_DEMOD_CLK_CNTL1,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "cts_demod_core_t2_clk_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_cts_demod_core_t2_clk_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_cts_demod_core_t2_clk = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_DEMOD_CLK_CNTL1,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "cts_demod_core_t2_clk",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_cts_demod_core_t2_clk_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_demod_32k_clkin = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_DEMOD_32K_CNTL0,
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

static const struct meson_clk_dualdiv_param t3x_demod_32k_div_table[] = {
	{
		.dual	= 0,
		.n1	= 733,
	},
	{}
};

static struct clk_regmap t3x_demod_32k_div = {
	.data = &(struct meson_clk_dualdiv_data){
		.n1 = {
			.reg_off = CLKCTRL_DEMOD_32K_CNTL0,
			.shift   = 0,
			.width   = 12,
		},
		.n2 = {
			.reg_off = CLKCTRL_DEMOD_32K_CNTL0,
			.shift   = 12,
			.width   = 12,
		},
		.m1 = {
			.reg_off = CLKCTRL_DEMOD_32K_CNTL1,
			.shift   = 0,
			.width   = 12,
		},
		.m2 = {
			.reg_off = CLKCTRL_DEMOD_32K_CNTL1,
			.shift   = 12,
			.width   = 12,
		},
		.dual = {
			.reg_off = CLKCTRL_DEMOD_32K_CNTL0,
			.shift   = 28,
			.width   = 1,
		},
		.table = t3x_demod_32k_div_table,
	},
	.hw.init = &(struct clk_init_data){
		.name = "demod_32k_div",
		.ops = &meson_clk_dualdiv_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_demod_32k_clkin.hw
		},
		.num_parents = 1,
	},
};

static struct clk_regmap t3x_demod_32k = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_DEMOD_32K_CNTL0,
		.bit_idx = 30,
	},
	.hw.init = &(struct clk_init_data){
		.name = "demod_32k",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_demod_32k_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static u32 t3x_cdac_mux_table[] = {0, 1};

static const struct clk_parent_data t3x_cdac_parent_data[] = {
	{ .fw_name = "xtal", },
	{ .hw = &t3x_fclk_div5.hw },
};

static struct clk_regmap t3x_cdac_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_CDAC_CLK_CTRL,
		.mask = 0x1,
		.shift = 16,
		.table = t3x_cdac_mux_table
	},
	.hw.init = &(struct clk_init_data) {
		.name = "cdac_clk_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_cdac_parent_data,
		.num_parents = ARRAY_SIZE(t3x_cdac_parent_data),
		.flags = CLK_GET_RATE_NOCACHE
	},
};

static struct clk_regmap t3x_cdac_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_CDAC_CLK_CTRL,
		.shift = 0,
		.width = 16,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "cdac_clk_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_cdac_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t3x_cdac = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_CDAC_CLK_CTRL,
		.bit_idx = 20,
	},
	.hw.init = &(struct clk_init_data){
		.name = "cdac",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_cdac_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE |	CLK_SET_RATE_PARENT
	},
};

static const struct clk_parent_data t3x_hdmirx_sys_parent_data[] = {
	{ .fw_name = "xtal", },
	{ .hw = &t3x_fclk_div4.hw },
	{ .hw = &t3x_fclk_div3.hw },
	{ .hw = &t3x_fclk_div5.hw }
};

static struct clk_regmap t3x_hdmirx_5m_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_HRX_CLK_CTRL0,
		.mask = 0x3,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdmirx_5m_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_hdmirx_sys_parent_data,
		.num_parents = ARRAY_SIZE(t3x_hdmirx_sys_parent_data),
	},
};

static struct clk_regmap t3x_hdmirx_5m_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_HRX_CLK_CTRL0,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdmirx_5m_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_hdmirx_5m_sel.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t3x_hdmirx_5m  = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_HRX_CLK_CTRL0,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hdmirx_5m",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_hdmirx_5m_div.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t3x_hdmirx_2m_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_HRX_CLK_CTRL0,
		.mask = 0x3,
		.shift = 25,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdmirx_2m_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_hdmirx_sys_parent_data,
		.num_parents = ARRAY_SIZE(t3x_hdmirx_sys_parent_data),
	},
};

static struct clk_regmap t3x_hdmirx_2m_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_HRX_CLK_CTRL0,
		.shift = 16,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdmirx_2m_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_hdmirx_2m_sel.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t3x_hdmirx_2m = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_HRX_CLK_CTRL0,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hdmirx_2m",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_hdmirx_2m_div.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t3x_hdmirx_cfg_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_HRX_CLK_CTRL1,
		.mask = 0x3,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdmirx_cfg_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_hdmirx_sys_parent_data,
		.num_parents = ARRAY_SIZE(t3x_hdmirx_sys_parent_data),
	},
};

static struct clk_regmap t3x_hdmirx_cfg_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_HRX_CLK_CTRL1,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdmirx_cfg_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_hdmirx_cfg_sel.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t3x_hdmirx_cfg  = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_HRX_CLK_CTRL1,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hdmirx_cfg",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_hdmirx_cfg_div.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t3x_hdmirx_hdcp_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_HRX_CLK_CTRL1,
		.mask = 0x3,
		.shift = 25,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdmirx_hdcp_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_hdmirx_sys_parent_data,
		.num_parents = ARRAY_SIZE(t3x_hdmirx_sys_parent_data),
	},
};

static struct clk_regmap t3x_hdmirx_hdcp_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_HRX_CLK_CTRL1,
		.shift = 16,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdmirx_hdcp_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_hdmirx_hdcp_sel.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t3x_hdmirx_hdcp = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_HRX_CLK_CTRL1,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hdmirx_hdcp",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_hdmirx_hdcp_div.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t3x_hdmirx_acr_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_HRX_CLK_CTRL2,
		.mask = 0x3,
		.shift = 25,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdmirx_acr_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_hdmirx_sys_parent_data,
		.num_parents = ARRAY_SIZE(t3x_hdmirx_sys_parent_data),
	},
};

static struct clk_regmap t3x_hdmirx_acr_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_HRX_CLK_CTRL2,
		.shift = 16,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdmirx_acr_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_hdmirx_acr_sel.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t3x_hdmirx_acr = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_HRX_CLK_CTRL2,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hdmirx_acr",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_hdmirx_acr_div.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t3x_hdmirx_meter_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_HRX_CLK_CTRL3,
		.mask = 0x3,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdmirx_meter_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_hdmirx_sys_parent_data,
		.num_parents = ARRAY_SIZE(t3x_hdmirx_sys_parent_data),
	},
};

static struct clk_regmap t3x_hdmirx_meter_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_HRX_CLK_CTRL3,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdmirx_meter_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_hdmirx_meter_sel.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t3x_hdmirx_meter  = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_HRX_CLK_CTRL3,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hdmirx_meter",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_hdmirx_meter_div.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t3x_hdmirx_axi_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_HRX_CLK_CTRL4,
		.mask = 0x3,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdmirx_axi_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_hdmirx_sys_parent_data,
		.num_parents = ARRAY_SIZE(t3x_hdmirx_sys_parent_data),
	},
};

static struct clk_regmap t3x_hdmirx_axi_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_HRX_CLK_CTRL4,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdmirx_axi_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_hdmirx_axi_sel.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t3x_hdmirx_axi = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_HRX_CLK_CTRL4,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hdmirx_axi",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_hdmirx_axi_div.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static const struct clk_hw *t3x_vpu_parent_hws[] = {
	&t3x_fclk_div3.hw,
	&t3x_fclk_div4.hw,
	&t3x_fclk_div5.hw,
	&t3x_fclk_div2p5.hw,
	&t3x_gp1_pll.hw
};

static u32 t3x_vpu_table[] = {0, 1, 2, 3, 4};

static struct clk_regmap t3x_vpu_0_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_VPU_CLK_CTRL,
		.mask = 0x7,
		.shift = 9,
		.table = t3x_vpu_table
	},
	.hw.init = &(struct clk_init_data){
		.name = "vpu_0_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = t3x_vpu_parent_hws,
		.num_parents = ARRAY_SIZE(t3x_vpu_parent_hws),
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_vpu_0_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_VPU_CLK_CTRL,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vpu_0_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_vpu_0_sel.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_vpu_0 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_VPU_CLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vpu_0",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_vpu_0_div.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap t3x_vpu_1_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_VPU_CLK_CTRL,
		.mask = 0x7,
		.shift = 25,
		.table = t3x_vpu_table
	},
	.hw.init = &(struct clk_init_data){
		.name = "vpu_1_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = t3x_vpu_parent_hws,
		.num_parents = ARRAY_SIZE(t3x_vpu_parent_hws),
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_vpu_1_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_VPU_CLK_CTRL,
		.shift = 16,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vpu_1_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_vpu_1_sel.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_vpu_1 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_VPU_CLK_CTRL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vpu_1",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_vpu_1_div.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap t3x_vpu = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_VPU_CLK_CTRL,
		.mask = 1,
		.shift = 31,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vpu",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_vpu_0.hw,
			&t3x_vpu_1.hw
		},
		.num_parents = 2,
		.flags = CLK_SET_RATE_NO_REPARENT | CLK_IGNORE_UNUSED,
	},
};

static const struct clk_hw *vpu_clkb_tmp_parent_hws[] = {
	&t3x_vpu.hw,
	&t3x_fclk_div4.hw,
	&t3x_fclk_div5.hw,
	&t3x_fclk_div3.hw
};

static struct clk_regmap t3x_vpu_clkb_tmp_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_VPU_CLKB_CTRL,
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

static struct clk_regmap t3x_vpu_clkb_tmp_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_VPU_CLKB_CTRL,
		.shift = 16,
		.width = 4,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vpu_clkb_tmp_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_vpu_clkb_tmp_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_vpu_clkb_tmp = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_VPU_CLKB_CTRL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vpu_clkb_tmp",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_vpu_clkb_tmp_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_vpu_clkb_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_VPU_CLKB_CTRL,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vpu_clkb_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_vpu_clkb_tmp.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_vpu_clkb = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_VPU_CLKB_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vpu_clkb",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_vpu_clkb_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static const struct clk_parent_data t3x_vdin_parent_hws[] = {
	{ .fw_name = "xtal", },
	{ .hw = &t3x_fclk_div4.hw },
	{ .hw = &t3x_fclk_div3.hw },
	{ .hw = &t3x_fclk_div5.hw },
};

static u32 t3x_vdin_meas_table[] = {0, 1, 2, 3};

static struct clk_regmap t3x_vdin_meas_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_VDIN_MEAS_CLK_CTRL,
		.mask = 0x7,
		.shift = 9,
		.table = t3x_vdin_meas_table,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vdin_meas_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_vdin_parent_hws,
		.num_parents = ARRAY_SIZE(t3x_vdin_parent_hws),
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap t3x_vdin_meas_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_VDIN_MEAS_CLK_CTRL,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vdin_meas_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_vdin_meas_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_vdin_meas = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_VDIN_MEAS_CLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vdin_meas",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_vdin_meas_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_cmpr_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_CMPR_CLK_CTRL,
		.mask = 0x3,
		.shift = 25,
	},
	.hw.init = &(struct clk_init_data){
		.name = "cmpr_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_vdin_parent_hws,
		.num_parents = ARRAY_SIZE(t3x_vdin_parent_hws),
	},
};

static struct clk_regmap t3x_cmpr_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_CMPR_CLK_CTRL,
		.shift = 16,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "cmpr_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_cmpr_sel.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_cmpr = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_CMPR_CLK_CTRL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "cmpr",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_cmpr_div.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static u32 mux_table_mali[] = { 0, 1, 3, 4, 5, 6};

static const struct clk_parent_data t3x_mali_0_1_parent_data[] = {
	{ .fw_name = "xtal", },
	{ .hw = &t3x_sys2_pll.hw },
	{ .hw = &t3x_fclk_div2p5.hw },
	{ .hw = &t3x_fclk_div3.hw },
	{ .hw = &t3x_fclk_div4.hw },
	{ .hw = &t3x_fclk_div5.hw },
};

static struct clk_regmap t3x_mali_0_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_MALI_CLK_CTRL,
		.mask = 0x7,
		.shift = 9,
		.table = mux_table_mali,
	},
	.hw.init = &(struct clk_init_data){
		.name = "mali_0_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_mali_0_1_parent_data,
		.num_parents = ARRAY_SIZE(t3x_mali_0_1_parent_data),
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_mali_0_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_MALI_CLK_CTRL,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "mali_0_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_mali_0_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_mali_0 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_MALI_CLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "mali_0",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_mali_0_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_GATE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_mali_1_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_MALI_CLK_CTRL,
		.mask = 0x7,
		.shift = 25,
		.table = mux_table_mali,
	},
	.hw.init = &(struct clk_init_data){
		.name = "mali_1_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_mali_0_1_parent_data,
		.num_parents = ARRAY_SIZE(t3x_mali_0_1_parent_data),
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_mali_1_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_MALI_CLK_CTRL,
		.shift = 16,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "mali_1_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_mali_1_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_mali_1 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_MALI_CLK_CTRL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data){
		.name = "mali_1",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_mali_1_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_GATE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_mali = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_MALI_CLK_CTRL,
		.mask = 1,
		.shift = 31,
	},
	.hw.init = &(struct clk_init_data){
		.name = "mali",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_mali_0.hw,
			&t3x_mali_1.hw
		},
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static u32 mux_table_dec[] = { 0, 1, 2, 3, 4};

static const struct clk_parent_data t3x_dec_parent_hws[] = {
	{ .hw = &t3x_fclk_div2p5.hw },
	{ .hw = &t3x_fclk_div3.hw },
	{ .hw = &t3x_fclk_div4.hw },
	{ .hw = &t3x_fclk_div5.hw },
	{ .hw = &t3x_fclk_div7.hw },
};

static struct clk_regmap t3x_vdec_0_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_VDEC_CLK_CTRL,
		.mask = 0x7,
		.shift = 9,
		.table = mux_table_dec
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vdec_0_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_dec_parent_hws,
		.num_parents = ARRAY_SIZE(t3x_dec_parent_hws),
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap t3x_vdec_0_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_VDEC_CLK_CTRL,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vdec_0_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_vdec_0_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_vdec_0 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_VDEC_CLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vdec_0",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_vdec_0_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_vdec_1_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_VDEC3_CLK_CTRL,
		.mask = 0x7,
		.shift = 9,
		.table = mux_table_dec
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vdec_1_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_dec_parent_hws,
		.num_parents = ARRAY_SIZE(t3x_dec_parent_hws),
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap t3x_vdec_1_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_VDEC3_CLK_CTRL,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vdec_1_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_vdec_1_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_vdec_1 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_VDEC3_CLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vdec_1",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_vdec_1_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_vdec = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_VDEC3_CLK_CTRL,
		.mask = 0x1,
		.shift = 15,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vdec",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_vdec_0.hw,
			&t3x_vdec_1.hw
		},
		.num_parents = 2,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_hcodec_0_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_VDEC_CLK_CTRL,
		.mask = 0x7,
		.shift = 25,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hcodec_0_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_dec_parent_hws,
		.num_parents = ARRAY_SIZE(t3x_dec_parent_hws),
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap t3x_hcodec_0_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_VDEC_CLK_CTRL,
		.shift = 16,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hcodec_0_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_hcodec_0_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_hcodec_0 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_VDEC_CLK_CTRL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hcodec_0",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_hcodec_0_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_hcodec_1_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_VDEC3_CLK_CTRL,
		.mask = 0x7,
		.shift = 25,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hcodec_1_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_dec_parent_hws,
		.num_parents = ARRAY_SIZE(t3x_dec_parent_hws),
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap t3x_hcodec_1_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_VDEC3_CLK_CTRL,
		.shift = 16,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hcodec_1_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_hcodec_1_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_hcodec_1 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_VDEC3_CLK_CTRL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hcodec_1",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_hcodec_1_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_hcodec = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_VDEC3_CLK_CTRL,
		.mask = 0x1,
		.shift = 31,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hcodec",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_hcodec_0.hw,
			&t3x_hcodec_1.hw
		},
		.num_parents = 2,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static u32 mux_table_vdec[] = { 0, 1, 2, 3, 4};

static struct clk_regmap t3x_hevc_0_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_VDEC2_CLK_CTRL,
		.mask = 0x7,
		.shift = 25,
		.flags = CLK_MUX_ROUND_CLOSEST,
		.table = mux_table_vdec,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hevc_0_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_dec_parent_hws,
		.num_parents = ARRAY_SIZE(t3x_dec_parent_hws),
	},
};

static struct clk_regmap t3x_hevc_0_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_VDEC2_CLK_CTRL,
		.shift = 16,
		.width = 7,
		.flags = CLK_DIVIDER_ROUND_CLOSEST,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hevc_0_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_hevc_0_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_hevc_0 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_VDEC2_CLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hevc_0",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_hevc_0_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_hevc_1_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_VDEC4_CLK_CTRL,
		.mask = 0x7,
		.shift = 25,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hevc_1_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_dec_parent_hws,
		.num_parents = ARRAY_SIZE(t3x_dec_parent_hws),
	},
};

static struct clk_regmap t3x_hevc_1_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_VDEC4_CLK_CTRL,
		.shift = 16,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hevc_1_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_hevc_1_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_hevc_1 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_VDEC4_CLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hevc_1",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_hevc_1_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_hevc = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_VDEC4_CLK_CTRL,
		.mask = 0x1,
		.shift = 15,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hevc",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_hevc_0.hw,
			&t3x_hevc_1.hw
		},
		.num_parents = 2,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static u32 mux_table_adla[] = { 0, 1, 2, 5};

static const struct clk_parent_data t3x_adla_0_1_parent_data[] = {
	{ .fw_name = "xtal", },
	{ .hw = &t3x_fclk_div2p5.hw },
	{ .hw = &t3x_fclk_div3.hw },
	{ .hw = &t3x_fclk_div4.hw },
};

static struct clk_regmap t3x_adla_0_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_NNA_CLK_CTRL0,
		.mask = 0x7,
		.shift = 9,
		.table = mux_table_adla,
	},
	.hw.init = &(struct clk_init_data){
		.name = "adla_0_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_adla_0_1_parent_data,
		.num_parents = ARRAY_SIZE(t3x_adla_0_1_parent_data),
	},
};

static struct clk_regmap t3x_adla_0_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_NNA_CLK_CTRL0,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "adla_0_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_adla_0_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_adla_0 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_NNA_CLK_CTRL0,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "adla_0",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_adla_0_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_GATE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_adla_1_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_NNA_CLK_CTRL0,
		.mask = 0x7,
		.shift = 25,
		.table = mux_table_adla,
	},
	.hw.init = &(struct clk_init_data){
		.name = "adla_1_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_adla_0_1_parent_data,
		.num_parents = ARRAY_SIZE(t3x_adla_0_1_parent_data),
	},
};

static struct clk_regmap t3x_adla_1_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_NNA_CLK_CTRL0,
		.shift = 16,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "adla_1_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_adla_1_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_adla_1 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_NNA_CLK_CTRL0,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data){
		.name = "adla_1",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_adla_1_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_GATE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_adla = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_NNA_CLK_CTRL0,
		.mask = 1,
		.shift = 31,
	},
	.hw.init = &(struct clk_init_data){
		.name = "adla",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_adla_0.hw,
			&t3x_adla_1.hw
		},
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static u32 t3x_sd_emmc_mux_table[] = {0, 1, 2, 4, 7};

static const struct clk_parent_data t3x_sd_emmc_parent_data[]  = {
	{ .fw_name = "xtal", },
	{ .hw = &t3x_fclk_div2.hw },
	{ .hw = &t3x_fclk_div3.hw },
	{ .hw = &t3x_fclk_div2p5.hw },
	{ .hw = &t3x_gp0_pll.hw }
};

static struct clk_regmap t3x_sd_emmc_c_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_NAND_CLK_CTRL,
		.mask = 0x7,
		.shift = 9,
		.table = t3x_sd_emmc_mux_table
	},
	.hw.init = &(struct clk_init_data) {
		.name = "sd_emmc_c_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_sd_emmc_parent_data,
		.num_parents = ARRAY_SIZE(t3x_sd_emmc_parent_data),
		.flags = CLK_GET_RATE_NOCACHE
	},
};

static struct clk_regmap t3x_sd_emmc_c_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_NAND_CLK_CTRL,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "sd_emmc_c_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_sd_emmc_c_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE //| CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t3x_sd_emmc_c = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_NAND_CLK_CTRL,
		.bit_idx = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sd_emmc_c",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_sd_emmc_c_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE //| CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t3x_sd_emmc_b_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_SD_EMMC_CLK_CTRL,
		.mask = 0x7,
		.shift = 25,
		.table = t3x_sd_emmc_mux_table
	},
	.hw.init = &(struct clk_init_data) {
		.name = "sd_emmc_b_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_sd_emmc_parent_data,
		.num_parents = ARRAY_SIZE(t3x_sd_emmc_parent_data),
		.flags = CLK_GET_RATE_NOCACHE
	},
};

static struct clk_regmap t3x_sd_emmc_b_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_SD_EMMC_CLK_CTRL,
		.shift = 16,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "sd_emmc_b_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_sd_emmc_b_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE //| CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t3x_sd_emmc_b = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_SD_EMMC_CLK_CTRL,
		.bit_idx = 23,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sd_emmc_b",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_sd_emmc_b_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE //| CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t3x_sd_emmc_a_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_SD_EMMC_CLK_CTRL,
		.mask = 0x7,
		.shift = 9,
		.table = t3x_sd_emmc_mux_table
	},
	.hw.init = &(struct clk_init_data) {
		.name = "sd_emmc_a_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_sd_emmc_parent_data,
		.num_parents = ARRAY_SIZE(t3x_sd_emmc_parent_data),
		.flags = CLK_GET_RATE_NOCACHE
	},
};

static struct clk_regmap t3x_sd_emmc_a_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_SD_EMMC_CLK_CTRL,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "sd_emmc_a_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_sd_emmc_a_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE
	},
};

static struct clk_regmap t3x_sd_emmc_a = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_SD_EMMC_CLK_CTRL,
		.bit_idx = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "sd_emmc_a",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_sd_emmc_a_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE
	},
};

static u32 t3x_eth_rmii_table[] = { 0 };

static struct clk_regmap t3x_eth_rmii_sel = {
	.data = &(struct clk_regmap_mux_data) {
		.offset = CLKCTRL_ETH_CLK_CTRL,
		.mask = 0x3,
		.shift = 9,
		.table = t3x_eth_rmii_table
	},
	.hw.init = &(struct clk_init_data){
		.name = "eth_rmii_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_fclk_div2.hw,
		},
		.num_parents = 1
	},
};

static struct clk_regmap t3x_eth_rmii_div = {
	.data = &(struct clk_regmap_div_data) {
		.offset = CLKCTRL_ETH_CLK_CTRL,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "eth_rmii_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_eth_rmii_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t3x_eth_rmii = {
	.data = &(struct clk_regmap_gate_data) {
		.offset = CLKCTRL_ETH_CLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "eth_rmii",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_eth_rmii_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_fixed_factor t3x_eth_div8 = {
	.mult = 1,
	.div = 8,
	.hw.init = &(struct clk_init_data){
		.name = "eth_div8",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_fclk_div2.hw },
		.num_parents = 1,
	},
};

static struct clk_regmap t3x_eth_125m = {
	.data = &(struct clk_regmap_gate_data) {
		.offset = CLKCTRL_ETH_CLK_CTRL,
		.bit_idx = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "eth_125m",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_eth_div8.hw
		},
		.num_parents = 1,
	},
};

static struct clk_regmap t3x_ts_clk_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_TS_CLK_CTRL,
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

static struct clk_regmap t3x_ts_clk = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_TS_CLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "ts_clk",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_ts_clk_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static u32 t3x_saradc_mux_table[] = { 0, 1, 2 };

static const struct clk_parent_data t3x_saradc_sel_clk_sel[] = {
	{ .fw_name = "xtal", },
	{ .hw = &t3x_sys_clk.hw },
	{ .hw = &t3x_fclk_div5.hw },
};

static struct clk_regmap t3x_saradc_sel = {
	.data = &(struct clk_regmap_mux_data) {
		.offset = CLKCTRL_SAR_CLK_CTRL0,
		.mask = 0x3,
		.shift = 9,
		.table = t3x_saradc_mux_table
	},
	.hw.init = &(struct clk_init_data){
		.name = "saradc_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_saradc_sel_clk_sel,
		.num_parents = ARRAY_SIZE(t3x_saradc_sel_clk_sel),
	},
};

static struct clk_regmap t3x_saradc_div = {
	.data = &(struct clk_regmap_div_data) {
		.offset = CLKCTRL_SAR_CLK_CTRL0,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "saradc_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_saradc_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_saradc = {
	.data = &(struct clk_regmap_gate_data) {
		.offset = CLKCTRL_SAR_CLK_CTRL0,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "saradc",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_saradc_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static u32 t3x_usb_clk_table[] = { 0, 1, 2, 3 };

static const struct clk_hw *t3x_usb_clk_parent_hws[] = {
	&t3x_fclk_div4.hw,
	&t3x_fclk_div3.hw,
	&t3x_fclk_div5.hw,
	&t3x_fclk_div2.hw,
};

static struct clk_regmap t3x_usb_250m_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_USB_CLK_CTRL,
		.mask = 0x7,
		.shift = 9,
		.table = t3x_usb_clk_table
	},
	.hw.init = &(struct clk_init_data) {
		.name = "usb_250m_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = t3x_usb_clk_parent_hws,
		.num_parents = ARRAY_SIZE(t3x_usb_clk_parent_hws),
	},
};

static struct clk_regmap t3x_usb_250m_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_USB_CLK_CTRL,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "usb_250m_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_usb_250m_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_usb_250m = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_USB_CLK_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "usb_250m",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_usb_250m_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_pcie_400m_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_USB_CLK_CTRL,
		.mask = 0x7,
		.shift = 25,
		.table = t3x_usb_clk_table
	},
	.hw.init = &(struct clk_init_data) {
		.name = "pcie_400m_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = t3x_usb_clk_parent_hws,
		.num_parents = ARRAY_SIZE(t3x_usb_clk_parent_hws),
	},
};

static struct clk_regmap t3x_pcie_400m_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_USB_CLK_CTRL,
		.shift = 16,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "pcie_400m_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_pcie_400m_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_pcie_400m = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_USB_CLK_CTRL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pcie_400m",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_pcie_400m_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_pcie_clk_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_USB_CLK_CTRL1,
		.mask = 0x7,
		.shift = 9,
		.table = t3x_usb_clk_table
	},
	.hw.init = &(struct clk_init_data) {
		.name = "pcie_clk_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = t3x_usb_clk_parent_hws,
		.num_parents = ARRAY_SIZE(t3x_usb_clk_parent_hws),
	},
};

static struct clk_regmap t3x_pcie_clk_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_USB_CLK_CTRL1,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "pcie_clk_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_pcie_clk_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_pcie_clk = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_USB_CLK_CTRL1,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pcie_clk",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_pcie_clk_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_pcie_tl_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_USB_CLK_CTRL1,
		.mask = 0x7,
		.shift = 25,
		.table = t3x_usb_clk_table
	},
	.hw.init = &(struct clk_init_data) {
		.name = "pcie_tl_clk_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = t3x_usb_clk_parent_hws,
		.num_parents = ARRAY_SIZE(t3x_usb_clk_parent_hws),
	},
};

static struct clk_regmap t3x_pcie_tl_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_USB_CLK_CTRL1,
		.shift = 16,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "pcie_tl_clk_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_pcie_tl_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_pcie_tl = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_USB_CLK_CTRL1,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pcie_tl_clk",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_pcie_tl_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_vid_lock_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_VID_LOCK_CLK_CTRL,
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

static struct clk_regmap t3x_vid_lock_clk  = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_VID_LOCK_CLK_CTRL,
		.bit_idx = 7,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vid_lock_clk",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_vid_lock_div.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static u32 mux_table_vclk_sel[] = {1, 2, 4, 5, 6, 7};

static const struct clk_hw *t3x_vclk_parent_hws[] = {
	&t3x_gp0_pll.hw,
	&t3x_hifi_pll.hw,
	&t3x_fclk_div3.hw,
	&t3x_fclk_div4.hw,
	&t3x_fclk_div5.hw,
	&t3x_fclk_div7.hw
};

static struct clk_regmap t3x_vclk_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_VID_CLK0_CTRL,
		.mask = 0x7,
		.shift = 16,
		.table = mux_table_vclk_sel,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vclk_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = t3x_vclk_parent_hws,
		.num_parents = ARRAY_SIZE(t3x_vclk_parent_hws),
		.flags = CLK_SET_RATE_NO_REPARENT | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap t3x_vclk2_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_VIID_CLK0_CTRL,
		.mask = 0x7,
		.shift = 16,
		.table = mux_table_vclk_sel,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vclk2_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = t3x_vclk_parent_hws,
		.num_parents = ARRAY_SIZE(t3x_vclk_parent_hws),
		.flags = CLK_SET_RATE_NO_REPARENT | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap t3x_vclk_input = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_VID_CLK0_DIV,
		.bit_idx = 16,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vclk_input",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_vclk_sel.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap t3x_vclk2_input = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_VIID_CLK0_DIV,
		.bit_idx = 16,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vclk2_input",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_vclk2_sel.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap t3x_vclk_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_VID_CLK0_DIV,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vclk_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_vclk_input.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap t3x_vclk2_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_VIID_CLK0_DIV,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "vclk2_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_vclk2_input.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap t3x_vclk = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_VID_CLK0_CTRL,
		.bit_idx = 19,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vclk",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_vclk_div.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap t3x_vclk2 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_VIID_CLK0_CTRL,
		.bit_idx = 19,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vclk2",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_vclk2_div.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap t3x_vclk_div1 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_VID_CLK0_CTRL,
		.bit_idx = 0,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vclk_div1",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_vclk.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap t3x_vclk_div2_en = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_VID_CLK0_CTRL,
		.bit_idx = 1,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vclk_div2_en",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_vclk.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap t3x_vclk_div4_en = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_VID_CLK0_CTRL,
		.bit_idx = 2,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vclk_div4_en",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_vclk.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap t3x_vclk_div6_en = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_VID_CLK0_CTRL,
		.bit_idx = 3,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vclk_div6_en",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_vclk.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap t3x_vclk_div12_en = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_VID_CLK0_CTRL,
		.bit_idx = 4,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vclk_div12_en",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_vclk.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap t3x_vclk2_div1 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_VIID_CLK0_CTRL,
		.bit_idx = 0,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vclk2_div1",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_vclk2.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap t3x_vclk2_div2_en = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_VIID_CLK0_CTRL,
		.bit_idx = 1,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vclk2_div2_en",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_vclk2.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap t3x_vclk2_div4_en = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_VIID_CLK0_CTRL,
		.bit_idx = 2,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vclk2_div4_en",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_vclk2.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap t3x_vclk2_div6_en = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_VIID_CLK0_CTRL,
		.bit_idx = 3,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vclk2_div6_en",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_vclk2.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap t3x_vclk2_div12_en = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_VIID_CLK0_CTRL,
		.bit_idx = 4,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "vclk2_div12_en",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_vclk2.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_fixed_factor t3x_vclk_div2 = {
	.mult = 1,
	.div = 2,
	.hw.init = &(struct clk_init_data){
		.name = "vclk_div2",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_vclk_div2_en.hw
		},
		.num_parents = 1,
	},
};

static struct clk_fixed_factor t3x_vclk_div4 = {
	.mult = 1,
	.div = 4,
	.hw.init = &(struct clk_init_data){
		.name = "vclk_div4",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_vclk_div4_en.hw
		},
		.num_parents = 1,
	},
};

static struct clk_fixed_factor t3x_vclk_div6 = {
	.mult = 1,
	.div = 6,
	.hw.init = &(struct clk_init_data){
		.name = "vclk_div6",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_vclk_div6_en.hw
		},
		.num_parents = 1,
	},
};

static struct clk_fixed_factor t3x_vclk_div12 = {
	.mult = 1,
	.div = 12,
	.hw.init = &(struct clk_init_data){
		.name = "vclk_div12",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_vclk_div12_en.hw
		},
		.num_parents = 1,
	},
};

static struct clk_fixed_factor t3x_vclk2_div2 = {
	.mult = 1,
	.div = 2,
	.hw.init = &(struct clk_init_data){
		.name = "vclk2_div2",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_vclk2_div2_en.hw
		},
		.num_parents = 1,
	},
};

static struct clk_fixed_factor t3x_vclk2_div4 = {
	.mult = 1,
	.div = 4,
	.hw.init = &(struct clk_init_data){
		.name = "vclk2_div4",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_vclk2_div4_en.hw
		},
		.num_parents = 1,
	},
};

static struct clk_fixed_factor t3x_vclk2_div6 = {
	.mult = 1,
	.div = 6,
	.hw.init = &(struct clk_init_data){
		.name = "vclk2_div6",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_vclk2_div6_en.hw
		},
		.num_parents = 1,
	},
};

static struct clk_fixed_factor t3x_vclk2_div12 = {
	.mult = 1,
	.div = 12,
	.hw.init = &(struct clk_init_data){
		.name = "vclk2_div12",
		.ops = &clk_fixed_factor_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_vclk2_div12_en.hw
		},
		.num_parents = 1,
	},
};

static u32 mux_table_cts_sel[] = { 0, 1, 2, 3, 4, 8, 9, 10, 11, 12 };

static const struct clk_hw *t3x_cts_parent_hws[] = {
	&t3x_vclk_div1.hw,
	&t3x_vclk_div2.hw,
	&t3x_vclk_div4.hw,
	&t3x_vclk_div6.hw,
	&t3x_vclk_div12.hw,
	&t3x_vclk2_div1.hw,
	&t3x_vclk2_div2.hw,
	&t3x_vclk2_div4.hw,
	&t3x_vclk2_div6.hw,
	&t3x_vclk2_div12.hw
};

static struct clk_regmap t3x_cts_enci_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_VID_CLK0_DIV,
		.mask = 0xf,
		.shift = 28,
		.table = mux_table_cts_sel,
	},
	.hw.init = &(struct clk_init_data){
		.name = "cts_enci_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = t3x_cts_parent_hws,
		.num_parents = ARRAY_SIZE(t3x_cts_parent_hws),
		.flags = CLK_SET_RATE_NO_REPARENT | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap t3x_cts_encp_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_VID_CLK1_DIV,
		.mask = 0xf,
		.shift = 20,
		.table = mux_table_cts_sel,
	},
	.hw.init = &(struct clk_init_data){
		.name = "cts_encp_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = t3x_cts_parent_hws,
		.num_parents = ARRAY_SIZE(t3x_cts_parent_hws),
		.flags = CLK_SET_RATE_NO_REPARENT | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap t3x_cts_vdac_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_VIID_CLK1_DIV,
		.mask = 0xf,
		.shift = 28,
		.table = mux_table_cts_sel,
	},
	.hw.init = &(struct clk_init_data){
		.name = "cts_vdac_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = t3x_cts_parent_hws,
		.num_parents = ARRAY_SIZE(t3x_cts_parent_hws),
		.flags = CLK_SET_RATE_NO_REPARENT | CLK_GET_RATE_NOCACHE,
	},
};

/* TOFIX: add support for cts_tcon */
static u32 mux_table_hdmi_tx_sel[] = { 0, 1, 2, 3, 4, 8, 9, 10, 11, 12 };
static const struct clk_hw *t3x_cts_hdmi_tx_parent_hws[] = {
	&t3x_vclk_div1.hw,
	&t3x_vclk_div2.hw,
	&t3x_vclk_div4.hw,
	&t3x_vclk_div6.hw,
	&t3x_vclk_div12.hw,
	&t3x_vclk2_div1.hw,
	&t3x_vclk2_div2.hw,
	&t3x_vclk2_div4.hw,
	&t3x_vclk2_div6.hw,
	&t3x_vclk2_div12.hw
};

static struct clk_regmap t3x_hdmi_tx_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_HDMI_CLK_CTRL,
		.mask = 0xf,
		.shift = 16,
		.table = mux_table_hdmi_tx_sel,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdmi_tx_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = t3x_cts_hdmi_tx_parent_hws,
		.num_parents = ARRAY_SIZE(t3x_cts_hdmi_tx_parent_hws),
		.flags = CLK_SET_RATE_NO_REPARENT | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap t3x_cts_enci = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_VID_CLK2_CTRL2,
		.bit_idx = 0,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "cts_enci",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_cts_enci_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap t3x_cts_encp = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_VID_CLK2_CTRL2,
		.bit_idx = 2,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "cts_encp",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_cts_encp_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap t3x_cts_vdac = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_VID_CLK2_CTRL2,
		.bit_idx = 4,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "cts_vdac",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_cts_vdac_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap t3x_hdmi_tx = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_VID_CLK2_CTRL2,
		.bit_idx = 5,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hdmi_tx",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_hdmi_tx_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap t3x_hdmirx_aud_pll_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_HRX_CLK_CTRL2,
		.mask = 0x3,
		.shift = 9,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdmirx_aud_pll_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_hdmirx_sys_parent_data,
		.num_parents = ARRAY_SIZE(t3x_hdmirx_sys_parent_data),
	},
};

static struct clk_regmap t3x_hdmirx_aud_pll_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_HRX_CLK_CTRL2,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "hdmirx_aud_pll_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_hdmirx_aud_pll_sel.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t3x_hdmirx_aud_pll  = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_HRX_CLK_CTRL2,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "hdmirx_aud_pll",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_hdmirx_aud_pll_div.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static u32 t3x_gen_clk_mux_table[] = { 0, 1, 4, 5, 6, 7, 19, 20, 21, 22, 23, 24 };

static const struct clk_parent_data t3x_gen_sel_clk_sel[] = {
	{ .fw_name = "xtal", },
	{ .hw = &t3x_rtc_clk.hw },
	{ .hw = &t3x_sys2_pll.hw },
	{ .hw = &t3x_gp0_pll.hw },
	{ .hw = &t3x_sys1_pll.hw },
	{ .hw = &t3x_hifi_pll.hw },
	{ .hw = &t3x_fclk_div2.hw },
	{ .hw = &t3x_fclk_div2p5.hw },
	{ .hw = &t3x_fclk_div3.hw },
	{ .hw = &t3x_fclk_div4.hw },
	{ .hw = &t3x_fclk_div5.hw },
	{ .hw = &t3x_fclk_div7.hw },
};

static struct clk_regmap t3x_gen_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_GEN_CLK_CTRL,
		.mask = 0x1f,
		.shift = 12,
		.table = t3x_gen_clk_mux_table,
	},
	.hw.init = &(struct clk_init_data){
		.name = "gen_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_gen_sel_clk_sel,
		.num_parents = ARRAY_SIZE(t3x_gen_sel_clk_sel),
	},
};

static struct clk_regmap t3x_gen_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_GEN_CLK_CTRL,
		.shift = 0,
		.width = 11,
	},
	.hw.init = &(struct clk_init_data){
		.name = "gen_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_gen_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_gen = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_GEN_CLK_CTRL,
		.bit_idx = 11,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "gen",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_gen_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static u32 t3x_spicc_mux_table[] = {0, 1, 2, 3, 4, 5, 6};

static const struct clk_parent_data t3x_spicc_parent_hws[] = {
	{ .fw_name = "xtal", },
	{ .hw = &t3x_sys_clk.hw },
	{ .hw = &t3x_fclk_div4.hw },
	{ .hw = &t3x_fclk_div3.hw },
	{ .hw = &t3x_fclk_div2.hw },
	{ .hw = &t3x_fclk_div5.hw },
	{ .hw = &t3x_fclk_div7.hw },
};

static struct clk_regmap t3x_spicc0_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_SPICC_CLK_CTRL,
		.mask = 0x7,
		.shift = 7,
		.table = t3x_spicc_mux_table
	},
	.hw.init = &(struct clk_init_data) {
		.name = "spicc0_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_spicc_parent_hws,
		.num_parents = ARRAY_SIZE(t3x_spicc_parent_hws),
	},
};

static struct clk_regmap t3x_spicc0_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_SPICC_CLK_CTRL,
		.shift = 0,
		.width = 6,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "spicc0_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_spicc0_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_spicc0 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_SPICC_CLK_CTRL,
		.bit_idx = 6,
	},
	.hw.init = &(struct clk_init_data){
		.name = "spicc0",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_spicc0_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_spicc1_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_SPICC_CLK_CTRL,
		.mask = 0x7,
		.shift = 23,
		.table = t3x_spicc_mux_table
	},
	.hw.init = &(struct clk_init_data) {
		.name = "spicc1_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_spicc_parent_hws,
		.num_parents = ARRAY_SIZE(t3x_spicc_parent_hws),
	},
};

static struct clk_regmap t3x_spicc1_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_SPICC_CLK_CTRL,
		.shift = 16,
		.width = 6,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "spicc1_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_spicc1_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_spicc1 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_SPICC_CLK_CTRL,
		.bit_idx = 22,
	},
	.hw.init = &(struct clk_init_data){
		.name = "spicc1",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_spicc1_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_spicc2_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_SPICC_CLK_CTRL1,
		.mask = 0x7,
		.shift = 7,
		.table = t3x_spicc_mux_table
	},
	.hw.init = &(struct clk_init_data) {
		.name = "spicc2_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_spicc_parent_hws,
		.num_parents = ARRAY_SIZE(t3x_spicc_parent_hws),
	},
};

static struct clk_regmap t3x_spicc2_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_SPICC_CLK_CTRL1,
		.shift = 0,
		.width = 6,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "spicc2_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_spicc2_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_spicc2 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_SPICC_CLK_CTRL1,
		.bit_idx = 6,
	},
	.hw.init = &(struct clk_init_data){
		.name = "spicc2",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_spicc2_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_spicc3_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_SPICC_CLK_CTRL1,
		.mask = 0x7,
		.shift = 23,
		.table = t3x_spicc_mux_table
	},
	.hw.init = &(struct clk_init_data) {
		.name = "spicc3_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_spicc_parent_hws,
		.num_parents = ARRAY_SIZE(t3x_spicc_parent_hws),
	},
};

static struct clk_regmap t3x_spicc3_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_SPICC_CLK_CTRL1,
		.shift = 16,
		.width = 6,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "spicc3_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_spicc3_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_spicc3 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_SPICC_CLK_CTRL1,
		.bit_idx = 22,
	},
	.hw.init = &(struct clk_init_data){
		.name = "spicc3",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_spicc3_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_spicc4_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_SPICC_CLK_CTRL2,
		.mask = 0x7,
		.shift = 7,
		.table = t3x_spicc_mux_table
	},
	.hw.init = &(struct clk_init_data) {
		.name = "spicc4_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_spicc_parent_hws,
		.num_parents = ARRAY_SIZE(t3x_spicc_parent_hws),
	},
};

static struct clk_regmap t3x_spicc4_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_SPICC_CLK_CTRL2,
		.shift = 0,
		.width = 6,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "spicc4_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_spicc4_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_spicc4 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_SPICC_CLK_CTRL2,
		.bit_idx = 6,
	},
	.hw.init = &(struct clk_init_data){
		.name = "spicc4",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_spicc4_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_spicc5_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_SPICC_CLK_CTRL2,
		.mask = 0x7,
		.shift = 23,
		.table = t3x_spicc_mux_table
	},
	.hw.init = &(struct clk_init_data) {
		.name = "spicc5_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_spicc_parent_hws,
		.num_parents = ARRAY_SIZE(t3x_spicc_parent_hws),
	},
};

static struct clk_regmap t3x_spicc5_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_SPICC_CLK_CTRL2,
		.shift = 16,
		.width = 6,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "spicc5_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_spicc5_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_spicc5 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_SPICC_CLK_CTRL2,
		.bit_idx = 22,
	},
	.hw.init = &(struct clk_init_data){
		.name = "spicc5",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_spicc5_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static u32 t3x_pwm_clk_table[] = {0, 2, 3, 6};

static const struct clk_parent_data t3x_pwm_parent_data[] = {
	{ .fw_name = "xtal", },
	{ .hw = &t3x_fclk_div4.hw },
	{ .hw = &t3x_fclk_div3.hw },
	{ .hw = &t3x_fclk_div5.hw }
};

static struct clk_regmap t3x_pwm_a_sel = {
	.data = &(struct clk_regmap_mux_data) {
		.offset = CLKCTRL_PWM_CLK_AB_CTRL,
		.mask = 0x3,
		.shift = 9,
		.table = t3x_pwm_clk_table,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pwm_a_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_pwm_parent_data,
		.num_parents = ARRAY_SIZE(t3x_pwm_parent_data),
	},
};

static struct clk_regmap t3x_pwm_a_div = {
	.data = &(struct clk_regmap_div_data) {
		.offset = CLKCTRL_PWM_CLK_AB_CTRL,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pwm_a_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_pwm_a_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_pwm_a = {
	.data = &(struct clk_regmap_gate_data) {
		.offset = CLKCTRL_PWM_CLK_AB_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pwm_a",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_pwm_a_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t3x_pwm_b_sel = {
	.data = &(struct clk_regmap_mux_data) {
		.offset = CLKCTRL_PWM_CLK_AB_CTRL,
		.mask = 0x3,
		.shift = 25,
		.table = t3x_pwm_clk_table,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pwm_b_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_pwm_parent_data,
		.num_parents = ARRAY_SIZE(t3x_pwm_parent_data),
	},
};

static struct clk_regmap t3x_pwm_b_div = {
	.data = &(struct clk_regmap_div_data) {
		.offset = CLKCTRL_PWM_CLK_AB_CTRL,
		.shift = 16,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pwm_b_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_pwm_b_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_pwm_b = {
	.data = &(struct clk_regmap_gate_data) {
		.offset = CLKCTRL_PWM_CLK_AB_CTRL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pwm_b",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_pwm_b_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t3x_pwm_c_sel = {
	.data = &(struct clk_regmap_mux_data) {
		.offset = CLKCTRL_PWM_CLK_CD_CTRL,
		.mask = 0x3,
		.shift = 9,
		.table = t3x_pwm_clk_table,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pwm_c_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_pwm_parent_data,
		.num_parents = ARRAY_SIZE(t3x_pwm_parent_data),
	},
};

static struct clk_regmap t3x_pwm_c_div = {
	.data = &(struct clk_regmap_div_data) {
		.offset = CLKCTRL_PWM_CLK_CD_CTRL,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pwm_c_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_pwm_c_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_pwm_c = {
	.data = &(struct clk_regmap_gate_data) {
		.offset = CLKCTRL_PWM_CLK_CD_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pwm_c",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_pwm_c_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t3x_pwm_d_sel = {
	.data = &(struct clk_regmap_mux_data) {
		.offset = CLKCTRL_PWM_CLK_CD_CTRL,
		.mask = 0x3,
		.shift = 25,
		.table = t3x_pwm_clk_table,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pwm_d_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_pwm_parent_data,
		.num_parents = ARRAY_SIZE(t3x_pwm_parent_data),
	},
};

static struct clk_regmap t3x_pwm_d_div = {
	.data = &(struct clk_regmap_div_data) {
		.offset = CLKCTRL_PWM_CLK_CD_CTRL,
		.shift = 16,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pwm_d_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_pwm_d_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t3x_pwm_d = {
	.data = &(struct clk_regmap_gate_data) {
		.offset = CLKCTRL_PWM_CLK_CD_CTRL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pwm_d",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_pwm_d_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t3x_pwm_e_sel = {
	.data = &(struct clk_regmap_mux_data) {
		.offset = CLKCTRL_PWM_CLK_EF_CTRL,
		.mask = 0x3,
		.shift = 9,
		.table = t3x_pwm_clk_table,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pwm_e_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_pwm_parent_data,
		.num_parents = ARRAY_SIZE(t3x_pwm_parent_data),
	},
};

static struct clk_regmap t3x_pwm_e_div = {
	.data = &(struct clk_regmap_div_data) {
		.offset = CLKCTRL_PWM_CLK_EF_CTRL,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pwm_e_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_pwm_e_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t3x_pwm_e = {
	.data = &(struct clk_regmap_gate_data) {
		.offset = CLKCTRL_PWM_CLK_EF_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pwm_e",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_pwm_e_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t3x_pwm_f_sel = {
	.data = &(struct clk_regmap_mux_data) {
		.offset = CLKCTRL_PWM_CLK_EF_CTRL,
		.mask = 0x3,
		.shift = 25,
		.table = t3x_pwm_clk_table,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pwm_f_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_pwm_parent_data,
		.num_parents = ARRAY_SIZE(t3x_pwm_parent_data),
	},
};

static struct clk_regmap t3x_pwm_f_div = {
	.data = &(struct clk_regmap_div_data) {
		.offset = CLKCTRL_PWM_CLK_EF_CTRL,
		.shift = 16,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pwm_f_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_pwm_f_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t3x_pwm_f = {
	.data = &(struct clk_regmap_gate_data) {
		.offset = CLKCTRL_PWM_CLK_EF_CTRL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pwm_f",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_pwm_f_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t3x_pwm_g_sel = {
	.data = &(struct clk_regmap_mux_data) {
		.offset = CLKCTRL_PWM_CLK_GH_CTRL,
		.mask = 0x3,
		.shift = 9,
		.table = t3x_pwm_clk_table,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pwm_g_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_pwm_parent_data,
		.num_parents = ARRAY_SIZE(t3x_pwm_parent_data),
	},
};

static struct clk_regmap t3x_pwm_g_div = {
	.data = &(struct clk_regmap_div_data) {
		.offset = CLKCTRL_PWM_CLK_GH_CTRL,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pwm_g_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_pwm_g_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t3x_pwm_g = {
	.data = &(struct clk_regmap_gate_data) {
		.offset = CLKCTRL_PWM_CLK_GH_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pwm_g",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_pwm_g_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t3x_pwm_h_sel = {
	.data = &(struct clk_regmap_mux_data) {
		.offset = CLKCTRL_PWM_CLK_GH_CTRL,
		.mask = 0x3,
		.shift = 25,
		.table = t3x_pwm_clk_table,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pwm_h_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_pwm_parent_data,
		.num_parents = ARRAY_SIZE(t3x_pwm_parent_data),
	},
};

static struct clk_regmap t3x_pwm_h_div = {
	.data = &(struct clk_regmap_div_data) {
		.offset = CLKCTRL_PWM_CLK_GH_CTRL,
		.shift = 16,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pwm_h_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_pwm_h_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t3x_pwm_h = {
	.data = &(struct clk_regmap_gate_data) {
		.offset = CLKCTRL_PWM_CLK_GH_CTRL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pwm_h",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_pwm_h_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t3x_pwm_i_sel = {
	.data = &(struct clk_regmap_mux_data) {
		.offset = CLKCTRL_PWM_CLK_IJ_CTRL,
		.mask = 0x3,
		.shift = 9,
		.table = t3x_pwm_clk_table,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pwm_i_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_pwm_parent_data,
		.num_parents = ARRAY_SIZE(t3x_pwm_parent_data),
	},
};

static struct clk_regmap t3x_pwm_i_div = {
	.data = &(struct clk_regmap_div_data) {
		.offset = CLKCTRL_PWM_CLK_IJ_CTRL,
		.shift = 0,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pwm_i_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_pwm_i_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t3x_pwm_i = {
	.data = &(struct clk_regmap_gate_data) {
		.offset = CLKCTRL_PWM_CLK_IJ_CTRL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pwm_i",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_pwm_i_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t3x_pwm_j_sel = {
	.data = &(struct clk_regmap_mux_data) {
		.offset = CLKCTRL_PWM_CLK_IJ_CTRL,
		.mask = 0x3,
		.shift = 25,
		.table = t3x_pwm_clk_table,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pwm_j_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_pwm_parent_data,
		.num_parents = ARRAY_SIZE(t3x_pwm_parent_data),
	},
};

static struct clk_regmap t3x_pwm_j_div = {
	.data = &(struct clk_regmap_div_data) {
		.offset = CLKCTRL_PWM_CLK_IJ_CTRL,
		.shift = 16,
		.width = 8,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pwm_j_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_pwm_j_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static struct clk_regmap t3x_pwm_j = {
	.data = &(struct clk_regmap_gate_data) {
		.offset = CLKCTRL_PWM_CLK_IJ_CTRL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data){
		.name = "pwm_j",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_pwm_j_div.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT
	},
};

static u32 t3x_tcon_pll_mux_table[] = { 0, 1, 2, 3 };

static const struct clk_parent_data t3x_cts_tcon_pll_clk_parent_data[] = {
	{ .fw_name = "xtal", },
	{ .hw = &t3x_fclk_div5.hw },
	{ .hw = &t3x_fclk_div4.hw },
	{ .hw = &t3x_fclk_div3.hw },
};

static struct clk_regmap t3x_cts_tcon_pll_clk_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_TCON_CLK_CNTL,
		.mask = 0x7,
		.shift = 7,
		.table = t3x_tcon_pll_mux_table
	},
	.hw.init = &(struct clk_init_data) {
		.name = "cts_tcon_pll_clk_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_data = t3x_cts_tcon_pll_clk_parent_data,
		.num_parents = ARRAY_SIZE(t3x_cts_tcon_pll_clk_parent_data),
	},
};

static struct clk_regmap t3x_cts_tcon_pll_clk_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_TCON_CLK_CNTL,
		.shift = 0,
		.width = 6,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "cts_tcon_pll_clk_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_cts_tcon_pll_clk_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_cts_tcon_pll_clk = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_TCON_CLK_CNTL,
		.bit_idx = 6,
	},
	.hw.init = &(struct clk_init_data){
		.name = "cts_tcon_pll_clk",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_cts_tcon_pll_clk_div.hw
		},
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static u32 t3x_frc_mux_table[] = { 0, 1, 2, 3 };

static const struct clk_hw *t3x_frc_parent_hws[] = {
	&t3x_fclk_div3.hw,
	&t3x_fclk_div4.hw,
	&t3x_fclk_div5.hw,
	&t3x_fclk_div2p5.hw
};

static struct clk_regmap t3x_frc_0_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_FRC_CLK_CNTL,
		.mask = 0x3,
		.shift = 9,
		.table = t3x_frc_mux_table
	},
	.hw.init = &(struct clk_init_data){
		.name = "frc_0_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = t3x_frc_parent_hws,
		.num_parents = ARRAY_SIZE(t3x_frc_parent_hws),
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap t3x_frc_0_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_FRC_CLK_CNTL,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "frc_0_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_frc_0_sel.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_frc_0 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_FRC_CLK_CNTL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "frc_0",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_frc_0_div.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_GATE | CLK_SET_RATE_PARENT
				 | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap t3x_frc_1_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_FRC_CLK_CNTL,
		.mask = 0x3,
		.shift = 25,
	},
	.hw.init = &(struct clk_init_data){
		.name = "frc_1_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = t3x_frc_parent_hws,
		.num_parents = ARRAY_SIZE(t3x_frc_parent_hws),
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_frc_1_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_FRC_CLK_CNTL,
		.shift = 16,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "frc_1_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_frc_1_sel.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_frc_1 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_FRC_CLK_CNTL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "frc_1",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_frc_1_div.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_GATE | CLK_SET_RATE_PARENT
				 | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap t3x_frc = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_FRC_CLK_CNTL,
		.mask = 0x1,
		.shift = 31,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "frc",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_frc_0.hw,
			&t3x_frc_1.hw
		},
		.num_parents = 2,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_me_0_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_ME_CLK_CNTL,
		.mask = 0x3,
		.shift = 9,
		.table = t3x_frc_mux_table
	},
	.hw.init = &(struct clk_init_data){
		.name = "me_0_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = t3x_frc_parent_hws,
		.num_parents = ARRAY_SIZE(t3x_frc_parent_hws),
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap t3x_me_0_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_ME_CLK_CNTL,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "me_0_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_me_0_sel.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_me_0 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_ME_CLK_CNTL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "me_0",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_me_0_div.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_GATE | CLK_SET_RATE_PARENT
				 | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap t3x_me_1_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_ME_CLK_CNTL,
		.mask = 0x3,
		.shift = 25,
		.table = t3x_frc_mux_table
	},
	.hw.init = &(struct clk_init_data){
		.name = "me_1_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = t3x_frc_parent_hws,
		.num_parents = ARRAY_SIZE(t3x_frc_parent_hws),
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_me_1_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_ME_CLK_CNTL,
		.shift = 16,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "me_1_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_me_1_sel.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_me_1 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_ME_CLK_CNTL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "me_1",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_me_1_div.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_GATE | CLK_SET_RATE_PARENT
				 | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap t3x_me = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_ME_CLK_CNTL,
		.mask = 0x1,
		.shift = 31,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "me",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_me_0.hw,
			&t3x_me_1.hw
		},
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static u32 t3x_tsin_mux_table[] = { 0, 2, 3, 4, 5, 6 };

static const struct clk_hw *t3x_tsin_parent_hws[] = {
	&t3x_fclk_div2.hw,
	&t3x_fclk_div4.hw,
	&t3x_fclk_div3.hw,
	&t3x_fclk_div2p5.hw,
	&t3x_fclk_div5.hw,
	&t3x_fclk_div7.hw,
};

static struct clk_regmap t3x_tsin_0_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_TSIN_CLK_CNTL,
		.mask = 0x3,
		.shift = 9,
		.table = t3x_tsin_mux_table
	},
	.hw.init = &(struct clk_init_data){
		.name = "tsin_0_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = t3x_tsin_parent_hws,
		.num_parents = ARRAY_SIZE(t3x_tsin_parent_hws),
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_regmap t3x_tsin_0_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_TSIN_CLK_CNTL,
		.shift = 0,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "tsin_0_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_tsin_0_sel.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_tsin_0 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_TSIN_CLK_CNTL,
		.bit_idx = 8,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "tsin_0",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_tsin_0_div.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_GATE | CLK_SET_RATE_PARENT
				 | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap t3x_tsin_1_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_TSIN_CLK_CNTL,
		.mask = 0x3,
		.shift = 25,
	},
	.hw.init = &(struct clk_init_data){
		.name = "tsin_1_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = t3x_tsin_parent_hws,
		.num_parents = ARRAY_SIZE(t3x_tsin_parent_hws),
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_tsin_1_div = {
	.data = &(struct clk_regmap_div_data){
		.offset = CLKCTRL_TSIN_CLK_CNTL,
		.shift = 16,
		.width = 7,
	},
	.hw.init = &(struct clk_init_data){
		.name = "tsin_1_div",
		.ops = &clk_regmap_divider_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_tsin_1_sel.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_tsin_1 = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_TSIN_CLK_CNTL,
		.bit_idx = 24,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "tsin_1",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) { &t3x_tsin_1_div.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_GATE | CLK_SET_RATE_PARENT
				 | CLK_IGNORE_UNUSED,
	},
};

static struct clk_regmap t3x_tsin_sel = {
	.data = &(struct clk_regmap_mux_data){
		.offset = CLKCTRL_TSIN_CLK_CNTL,
		.mask = 0x1,
		.shift = 31,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "tsin_sel",
		.ops = &clk_regmap_mux_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_tsin_0.hw,
			&t3x_tsin_1.hw
		},
		.num_parents = 2,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap t3x_tsin = {
	.data = &(struct clk_regmap_gate_data){
		.offset = CLKCTRL_TSIN_CLK_CNTL,
		.bit_idx = 30,
	},
	.hw.init = &(struct clk_init_data) {
		.name = "tsin",
		.ops = &clk_regmap_gate_ops,
		.parent_hws = (const struct clk_hw *[]) {
			&t3x_tsin_sel.hw
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

#define MESON_t3x_SYS_GATE(_name, _reg, _bit)				\
struct clk_regmap _name = {						\
	.data = &(struct clk_regmap_gate_data) {			\
		.offset = (_reg),					\
		.bit_idx = (_bit),					\
	},								\
	.hw.init = &(struct clk_init_data) {				\
		.name = #_name,						\
		.ops = &clk_regmap_gate_ops,				\
		.parent_hws = (const struct clk_hw *[]) {		\
			&t3x_sys_clk.hw					\
		},							\
		.num_parents = 1,					\
		.flags = CLK_IGNORE_UNUSED,				\
	},								\
}

static MESON_t3x_SYS_GATE(t3x_sys_clk_ddr,		CLKCTRL_SYS_CLK_EN0_REG0, 0);
static MESON_t3x_SYS_GATE(t3x_sys_clk_ethphy,		CLKCTRL_SYS_CLK_EN0_REG0, 4);
static MESON_t3x_SYS_GATE(t3x_sys_clk_mali,		CLKCTRL_SYS_CLK_EN0_REG0, 6);
static MESON_t3x_SYS_GATE(t3x_sys_clk_aocpu,		CLKCTRL_SYS_CLK_EN0_REG0, 13);
static MESON_t3x_SYS_GATE(t3x_sys_clk_aucpu,		CLKCTRL_SYS_CLK_EN0_REG0, 14);
static MESON_t3x_SYS_GATE(t3x_sys_clk_dewarpc,		CLKCTRL_SYS_CLK_EN0_REG0, 16);
static MESON_t3x_SYS_GATE(t3x_sys_clk_dewarpb,		CLKCTRL_SYS_CLK_EN0_REG0, 17);
static MESON_t3x_SYS_GATE(t3x_sys_clk_dewarpa,		CLKCTRL_SYS_CLK_EN0_REG0, 18);
static MESON_t3x_SYS_GATE(t3x_sys_clk_ampipe_nand,	CLKCTRL_SYS_CLK_EN0_REG0, 29);
static MESON_t3x_SYS_GATE(t3x_sys_clk_ampipe_eth,	CLKCTRL_SYS_CLK_EN0_REG0, 20);
static MESON_t3x_SYS_GATE(t3x_sys_clk_am2axi0,		CLKCTRL_SYS_CLK_EN0_REG0, 21);
static MESON_t3x_SYS_GATE(t3x_sys_clk_sdemmca,		CLKCTRL_SYS_CLK_EN0_REG0, 24);
static MESON_t3x_SYS_GATE(t3x_sys_clk_sdemmcb,		CLKCTRL_SYS_CLK_EN0_REG0, 25);
static MESON_t3x_SYS_GATE(t3x_sys_clk_sdemmcc,		CLKCTRL_SYS_CLK_EN0_REG0, 26);
static MESON_t3x_SYS_GATE(t3x_sys_clk_vc9000e,		CLKCTRL_SYS_CLK_EN0_REG0, 27);
static MESON_t3x_SYS_GATE(t3x_sys_clk_acodec,		CLKCTRL_SYS_CLK_EN0_REG0, 28);
static MESON_t3x_SYS_GATE(t3x_sys_clk_spifc,		CLKCTRL_SYS_CLK_EN0_REG0, 29);
static MESON_t3x_SYS_GATE(t3x_sys_clk_msr_clk,		CLKCTRL_SYS_CLK_EN0_REG0, 30);
static MESON_t3x_SYS_GATE(t3x_sys_clk_ir_ctrl,		CLKCTRL_SYS_CLK_EN0_REG0, 31);

static MESON_t3x_SYS_GATE(t3x_sys_clk_audio,		CLKCTRL_SYS_CLK_EN0_REG1, 0);
static MESON_t3x_SYS_GATE(t3x_sys_clk_dos,		CLKCTRL_SYS_CLK_EN0_REG1, 2);
static MESON_t3x_SYS_GATE(t3x_sys_clk_eth,		CLKCTRL_SYS_CLK_EN0_REG1, 3);
static MESON_t3x_SYS_GATE(t3x_sys_clk_uart_a,		CLKCTRL_SYS_CLK_EN0_REG1, 5);
static MESON_t3x_SYS_GATE(t3x_sys_clk_uart_b,		CLKCTRL_SYS_CLK_EN0_REG1, 6);
static MESON_t3x_SYS_GATE(t3x_sys_clk_uart_c,		CLKCTRL_SYS_CLK_EN0_REG1, 7);
static MESON_t3x_SYS_GATE(t3x_sys_clk_uart_d,		CLKCTRL_SYS_CLK_EN0_REG1, 8);
static MESON_t3x_SYS_GATE(t3x_sys_clk_uart_e,		CLKCTRL_SYS_CLK_EN0_REG1, 9);
static MESON_t3x_SYS_GATE(t3x_sys_clk_uart_f,		CLKCTRL_SYS_CLK_EN0_REG1, 10);
static MESON_t3x_SYS_GATE(t3x_sys_clk_spicc2,		CLKCTRL_SYS_CLK_EN0_REG1, 12);
static MESON_t3x_SYS_GATE(t3x_sys_clk_ts_a55,		CLKCTRL_SYS_CLK_EN0_REG1, 16);
static MESON_t3x_SYS_GATE(t3x_sys_clk_ge2d,		CLKCTRL_SYS_CLK_EN0_REG1, 20);
static MESON_t3x_SYS_GATE(t3x_sys_clk_spicc0,		CLKCTRL_SYS_CLK_EN0_REG1, 21);
static MESON_t3x_SYS_GATE(t3x_sys_clk_spicc1,		CLKCTRL_SYS_CLK_EN0_REG1, 22);
static MESON_t3x_SYS_GATE(t3x_sys_clk_smartcard,	CLKCTRL_SYS_CLK_EN0_REG1, 23);
static MESON_t3x_SYS_GATE(t3x_sys_clk_pcie,		CLKCTRL_SYS_CLK_EN0_REG1, 24);
static MESON_t3x_SYS_GATE(t3x_sys_clk_pciephy,		CLKCTRL_SYS_CLK_EN0_REG1, 25);
static MESON_t3x_SYS_GATE(t3x_sys_clk_usb,		CLKCTRL_SYS_CLK_EN0_REG1, 26);
static MESON_t3x_SYS_GATE(t3x_sys_clk_pciephy0,		CLKCTRL_SYS_CLK_EN0_REG1, 27);
static MESON_t3x_SYS_GATE(t3x_sys_clk_pciephy1,		CLKCTRL_SYS_CLK_EN0_REG1, 28);
static MESON_t3x_SYS_GATE(t3x_sys_clk_pciephy2,		CLKCTRL_SYS_CLK_EN0_REG1, 29);
static MESON_t3x_SYS_GATE(t3x_sys_clk_i2c_m_a,		CLKCTRL_SYS_CLK_EN0_REG1, 30);
static MESON_t3x_SYS_GATE(t3x_sys_clk_i2c_m_b,		CLKCTRL_SYS_CLK_EN0_REG1, 31);

static MESON_t3x_SYS_GATE(t3x_sys_clk_i2c_m_c,		CLKCTRL_SYS_CLK_EN0_REG2, 0);
static MESON_t3x_SYS_GATE(t3x_sys_clk_i2c_m_d,		CLKCTRL_SYS_CLK_EN0_REG2, 1);
static MESON_t3x_SYS_GATE(t3x_sys_clk_i2c_m_e,		CLKCTRL_SYS_CLK_EN0_REG2, 2);
static MESON_t3x_SYS_GATE(t3x_sys_clk_i2c_m_f,		CLKCTRL_SYS_CLK_EN0_REG2, 3);
static MESON_t3x_SYS_GATE(t3x_sys_clk_i2c_s_a,		CLKCTRL_SYS_CLK_EN0_REG2, 8);
static MESON_t3x_SYS_GATE(t3x_sys_clk_cmpr,		CLKCTRL_SYS_CLK_EN0_REG2, 10);
static MESON_t3x_SYS_GATE(t3x_sys_clk_mmc_pclk,		CLKCTRL_SYS_CLK_EN0_REG2, 11);
static MESON_t3x_SYS_GATE(t3x_sys_clk_hdmi_pclk,	CLKCTRL_SYS_CLK_EN0_REG2, 16);
static MESON_t3x_SYS_GATE(t3x_sys_clk_hdmi20_aes_clk,	CLKCTRL_SYS_CLK_EN0_REG2, 17);
static MESON_t3x_SYS_GATE(t3x_sys_clk_pclk_sys_cpu_apb,	CLKCTRL_SYS_CLK_EN0_REG2, 19);
static MESON_t3x_SYS_GATE(t3x_sys_clk_cec,		CLKCTRL_SYS_CLK_EN0_REG2, 23);
static MESON_t3x_SYS_GATE(t3x_sys_clk_vpu_intr,		CLKCTRL_SYS_CLK_EN0_REG2, 25);
static MESON_t3x_SYS_GATE(t3x_sys_clk_sar_adc,		CLKCTRL_SYS_CLK_EN0_REG2, 28);
static MESON_t3x_SYS_GATE(t3x_sys_clk_gic,		CLKCTRL_SYS_CLK_EN0_REG2, 30);
static MESON_t3x_SYS_GATE(t3x_sys_clk_ts_gpu,		CLKCTRL_SYS_CLK_EN0_REG2, 31);

static MESON_t3x_SYS_GATE(t3x_sys_clk_ts_nna,		CLKCTRL_SYS_CLK_EN0_REG3, 0);
static MESON_t3x_SYS_GATE(t3x_sys_clk_ts_vpu,		CLKCTRL_SYS_CLK_EN0_REG3, 1);
static MESON_t3x_SYS_GATE(t3x_sys_clk_ts_dos,		CLKCTRL_SYS_CLK_EN0_REG3, 2);
static MESON_t3x_SYS_GATE(t3x_sys_clk_pwm_ab,		CLKCTRL_SYS_CLK_EN0_REG3, 5);
static MESON_t3x_SYS_GATE(t3x_sys_clk_pwm_cd,		CLKCTRL_SYS_CLK_EN0_REG3, 6);
static MESON_t3x_SYS_GATE(t3x_sys_clk_pwm_ef,		CLKCTRL_SYS_CLK_EN0_REG3, 7);
static MESON_t3x_SYS_GATE(t3x_sys_clk_pwm_gh,		CLKCTRL_SYS_CLK_EN0_REG3, 8);
static MESON_t3x_SYS_GATE(t3x_sys_clk_pwm_ij,		CLKCTRL_SYS_CLK_EN0_REG3, 9);

/* Array of all clocks provided by this provider */
static struct clk_hw_onecell_data t3x_hw_onecell_data = {
	.hws = {
		[CLKID_SYS_PLL_DCO]			= &t3x_sys_pll_dco.hw,
		[CLKID_SYS_PLL]				= &t3x_sys_pll.hw,
		[CLKID_SYS1_PLL_DCO]			= &t3x_sys1_pll_dco.hw,
		[CLKID_SYS1_PLL]			= &t3x_sys1_pll.hw,
		[CLKID_SYS2_PLL_DCO]			= &t3x_sys2_pll_dco.hw,
		[CLKID_SYS2_PLL]			= &t3x_sys2_pll.hw,
		[CLKID_SYS3_PLL_DCO]			= &t3x_sys3_pll_dco.hw,
		[CLKID_SYS3_PLL]			= &t3x_sys3_pll.hw,
		[CLKID_FIXED_PLL_DCO]			= &t3x_fixed_pll_dco.hw,
		[CLKID_FIXED_PLL]			= &t3x_fixed_pll.hw,
		[CLKID_FCLK_DIV2_DIV]			= &t3x_fclk_div2_div.hw,
		[CLKID_FCLK_DIV2]			= &t3x_fclk_div2.hw,
		[CLKID_FCLK_DIV3_DIV]			= &t3x_fclk_div3_div.hw,
		[CLKID_FCLK_DIV3]			= &t3x_fclk_div3.hw,
		[CLKID_FCLK_DIV4_DIV]			= &t3x_fclk_div4_div.hw,
		[CLKID_FCLK_DIV4]			= &t3x_fclk_div4.hw,
		[CLKID_FCLK_DIV5_DIV]			= &t3x_fclk_div5_div.hw,
		[CLKID_FCLK_DIV5]			= &t3x_fclk_div5.hw,
		[CLKID_FCLK_DIV7_DIV]			= &t3x_fclk_div7_div.hw,
		[CLKID_FCLK_DIV7]			= &t3x_fclk_div7.hw,
		[CLKID_FCLK_DIV2P5_DIV]			= &t3x_fclk_div2p5_div.hw,
		[CLKID_FCLK_DIV2P5]			= &t3x_fclk_div2p5.hw,
		[CLKID_GP0_PLL_DCO]			= &t3x_gp0_pll_dco.hw,
		[CLKID_GP0_PLL]				= &t3x_gp0_pll.hw,
		[CLKID_GP1_PLL_DCO]			= &t3x_gp1_pll_dco.hw,
		[CLKID_GP1_PLL]				= &t3x_gp1_pll.hw,
		[CLKID_CPU_DYN_CLK]			= &t3x_cpu_dyn_clk.hw,
		[CLKID_CPU_CLK]				= &t3x_cpu_clk.hw,
		[CLKID_A76_DYN_CLK]			= &t3x_a76_dyn_clk.hw,
		[CLKID_A76_CLK]				= &t3x_a76_clk.hw,
		[CLKID_DSU_DYN_CLK]			= &t3x_dsu_dyn_clk.hw,
		[CLKID_DSU_CLK]				= &t3x_dsu_clk.hw,
		[CLKID_DSU_FINAL_CLK]			= &t3x_dsu_final_clk.hw,
		[CLKID_HIFI_PLL_DCO]			= &t3x_hifi_pll_dco.hw,
		[CLKID_HIFI_PLL]			= &t3x_hifi_pll.hw,
		[CLKID_HIFI1_PLL_DCO]			= &t3x_hifi1_pll_dco.hw,
		[CLKID_HIFI1_PLL]			= &t3x_hifi1_pll.hw,
		[CLKID_PCIE_PLL]			= &t3x_pcie_pll.hw,
		[CLKID_PCIE_HCSL]			= &t3x_pcie_hcsl.hw,
		[CLKID_MPLL_50M_DIV]			= &t3x_mpll_50m_div.hw,
		[CLKID_MPLL_50M]			= &t3x_mpll_50m.hw,
		[CLKID_RTC_32K_CLKIN]			= &t3x_rtc_32k_clkin.hw,
		[CLKID_RTC_32K_DIV]			= &t3x_rtc_32k_div.hw,
		[CLKID_RTC_32K_XATL]			= &t3x_rtc_32k_xtal.hw,
		[CLKID_RTC_32K_SEL]			= &t3x_rtc_32k_sel.hw,
		[CLKID_RTC_CLK]				= &t3x_rtc_clk.hw,
		[CLKID_SYS_CLK]				= &t3x_sys_clk.hw,
		[CLKID_AXI_CLK]				= &t3x_axi_clk.hw,
		[CLKID_CECA_32K_CLKIN]			= &t3x_ceca_32k_clkin.hw,
		[CLKID_CECA_32K_DIV]			= &t3x_ceca_32k_div.hw,
		[CLKID_CECA_32K_SEL_PRE]		= &t3x_ceca_32k_sel_pre.hw,
		[CLKID_CECA_32K_SEL]			= &t3x_ceca_32k_sel.hw,
		[CLKID_CECA_32K_CLKOUT]			= &t3x_ceca_32k_clkout.hw,
		[CLKID_CECB_32K_CLKIN]			= &t3x_cecb_32k_clkin.hw,
		[CLKID_CECB_32K_DIV]			= &t3x_cecb_32k_div.hw,
		[CLKID_CECB_32K_SEL_PRE]		= &t3x_cecb_32k_sel_pre.hw,
		[CLKID_CECB_32K_SEL]			= &t3x_cecb_32k_sel.hw,
		[CLKID_CECB_32K_CLKOUT]			= &t3x_cecb_32k_clkout.hw,
		[CLKID_SC_CLK_SEL]			= &t3x_sc_clk_sel.hw,
		[CLKID_SC_CLK_DIV]			= &t3x_sc_clk_div.hw,
		[CLKID_SC_CLK]				= &t3x_sc_clk.hw,
		[CLKID_DSPA_0_SEL]			= &t3x_dspa_0_sel.hw,
		[CLKID_DSPA_0_DIV]			= &t3x_dspa_0_div.hw,
		[CLKID_DSPA_0]				= &t3x_dspa_0.hw,
		[CLKID_DSPA_1_SEL]			= &t3x_dspa_1_sel.hw,
		[CLKID_DSPA_1_DIV]			= &t3x_dspa_1_div.hw,
		[CLKID_DSPA_1]				= &t3x_dspa_1.hw,
		[CLKID_DSPA]				= &t3x_dspa.hw,
		[CLKID_24M_CLK_GATE]			= &t3x_24m_clk_gate.hw,
		[CLKID_24M_DIV2]			= &t3x_24m_div2.hw,
		[CLKID_12M_CLK]				= &t3x_12m_clk.hw,
		[CLKID_25M_CLK_DIV]			= &t3x_25m_clk_div.hw,
		[CLKID_25M_CLK]				= &t3x_25m_clk.hw,
		[CLKID_VCLK_SEL]			= &t3x_vclk_sel.hw,
		[CLKID_VCLK2_SEL]			= &t3x_vclk2_sel.hw,
		[CLKID_VCLK_INPUT]			= &t3x_vclk_input.hw,
		[CLKID_VCLK2_INPUT]			= &t3x_vclk2_input.hw,
		[CLKID_VCLK_DIV]			= &t3x_vclk_div.hw,
		[CLKID_VCLK2_DIV]			= &t3x_vclk2_div.hw,
		[CLKID_VCLK]				= &t3x_vclk.hw,
		[CLKID_VCLK2]				= &t3x_vclk2.hw,
		[CLKID_VCLK_DIV1]			= &t3x_vclk_div1.hw,
		[CLKID_VCLK_DIV2_EN]			= &t3x_vclk_div2_en.hw,
		[CLKID_VCLK_DIV4_EN]			= &t3x_vclk_div4_en.hw,
		[CLKID_VCLK_DIV6_EN]			= &t3x_vclk_div6_en.hw,
		[CLKID_VCLK_DIV12_EN]			= &t3x_vclk_div12_en.hw,
		[CLKID_VCLK2_DIV1]			= &t3x_vclk2_div1.hw,
		[CLKID_VCLK2_DIV2_EN]			= &t3x_vclk2_div2_en.hw,
		[CLKID_VCLK2_DIV4_EN]			= &t3x_vclk2_div4_en.hw,
		[CLKID_VCLK2_DIV6_EN]			= &t3x_vclk2_div6_en.hw,
		[CLKID_VCLK2_DIV12_EN]			= &t3x_vclk2_div12_en.hw,
		[CLKID_VCLK_DIV2]			= &t3x_vclk_div2.hw,
		[CLKID_VCLK_DIV4]			= &t3x_vclk_div4.hw,
		[CLKID_VCLK_DIV6]			= &t3x_vclk_div6.hw,
		[CLKID_VCLK_DIV12]			= &t3x_vclk_div12.hw,
		[CLKID_VCLK2_DIV2]			= &t3x_vclk2_div2.hw,
		[CLKID_VCLK2_DIV4]			= &t3x_vclk2_div4.hw,
		[CLKID_VCLK2_DIV6]			= &t3x_vclk2_div6.hw,
		[CLKID_VCLK2_DIV12]			= &t3x_vclk2_div12.hw,
		[CLKID_CTS_ENCI_SEL]			= &t3x_cts_enci_sel.hw,
		[CLKID_CTS_ENCP_SEL]			= &t3x_cts_encp_sel.hw,
		[CLKID_CTS_VDAC_SEL]			= &t3x_cts_vdac_sel.hw,
		[CLKID_HDMI_TX_SEL]			= &t3x_hdmi_tx_sel.hw,
		[CLKID_CTS_ENCI]			= &t3x_cts_enci.hw,
		[CLKID_CTS_ENCP]			= &t3x_cts_encp.hw,
		[CLKID_CTS_VDAC]			= &t3x_cts_vdac.hw,
		[CLKID_HDMI_TX]				= &t3x_hdmi_tx.hw,
		[CLKID_MALI_0_SEL]			= &t3x_mali_0_sel.hw,
		[CLKID_MALI_0_DIV]			= &t3x_mali_0_div.hw,
		[CLKID_MALI_0]				= &t3x_mali_0.hw,
		[CLKID_MALI_1_SEL]			= &t3x_mali_1_sel.hw,
		[CLKID_MALI_1_DIV]			= &t3x_mali_1_div.hw,
		[CLKID_MALI_1]				= &t3x_mali_1.hw,
		[CLKID_MALI]				= &t3x_mali.hw,
		[CLKID_VDEC_0_SEL]			= &t3x_vdec_0_sel.hw,
		[CLKID_VDEC_0_DIV]			= &t3x_vdec_0_div.hw,
		[CLKID_VDEC_0]				= &t3x_vdec_0.hw,
		[CLKID_VDEC_1_SEL]			= &t3x_vdec_1_sel.hw,
		[CLKID_VDEC_1_DIV]			= &t3x_vdec_1_div.hw,
		[CLKID_VDEC_1]				= &t3x_vdec_1.hw,
		[CLKID_VDEC]				= &t3x_vdec.hw,
		[CLKID_HCODEC_0_SEL]			= &t3x_hcodec_0_sel.hw,
		[CLKID_HCODEC_0_DIV]			= &t3x_hcodec_0_div.hw,
		[CLKID_HCODEC_0]			= &t3x_hcodec_0.hw,
		[CLKID_HCODEC_1_SEL]			= &t3x_hcodec_1_sel.hw,
		[CLKID_HCODEC_1_DIV]			= &t3x_hcodec_1_div.hw,
		[CLKID_HCODEC_1]			= &t3x_hcodec_1.hw,
		[CLKID_HCODEC]				= &t3x_hcodec.hw,
		[CLKID_HEVC_0_SEL]			= &t3x_hevc_0_sel.hw,
		[CLKID_HEVC_0_DIV]			= &t3x_hevc_0_div.hw,
		[CLKID_HEVC_0]				= &t3x_hevc_0.hw,
		[CLKID_HEVC_1_SEL]			= &t3x_hevc_1_sel.hw,
		[CLKID_HEVC_1_DIV]			= &t3x_hevc_1_div.hw,
		[CLKID_HEVC_1]				= &t3x_hevc_1.hw,
		[CLKID_HEVC]				= &t3x_hevc.hw,
		[CLKID_VPU_0_SEL]			= &t3x_vpu_0_sel.hw,
		[CLKID_VPU_0_DIV]			= &t3x_vpu_0_div.hw,
		[CLKID_VPU_0]				= &t3x_vpu_0.hw,
		[CLKID_VPU_1_SEL]			= &t3x_vpu_1_sel.hw,
		[CLKID_VPU_1_DIV]			= &t3x_vpu_1_div.hw,
		[CLKID_VPU_1]				= &t3x_vpu_1.hw,
		[CLKID_VPU]				= &t3x_vpu.hw,
		[CLKID_VPU_CLKB_TMP_SEL]		= &t3x_vpu_clkb_tmp_sel.hw,
		[CLKID_VPU_CLKB_TMP_DIV]		= &t3x_vpu_clkb_tmp_div.hw,
		[CLKID_VPU_CLKB_TMP]			= &t3x_vpu_clkb_tmp.hw,
		[CLKID_VPU_CLKB_DIV]			= &t3x_vpu_clkb_div.hw,
		[CLKID_VPU_CLKB]			= &t3x_vpu_clkb.hw,
		[CLKID_VAPB_0_SEL]			= &t3x_vapb_0_sel.hw,
		[CLKID_VAPB_0_DIV]			= &t3x_vapb_0_div.hw,
		[CLKID_VAPB_0]				= &t3x_vapb_0.hw,
		[CLKID_VAPB_1_SEL]			= &t3x_vapb_1_sel.hw,
		[CLKID_VAPB_1_DIV]			= &t3x_vapb_1_div.hw,
		[CLKID_VAPB_1]				= &t3x_vapb_1.hw,
		[CLKID_VAPB]				= &t3x_vapb.hw,
		[CLKID_GE2D_SEL]			= &t3x_ge2d_sel.hw,
		[CLKID_GE2D_DIV]			= &t3x_ge2d_div.hw,
		[CLKID_GE2D]				= &t3x_ge2d.hw,
		[CLKID_VDIN_MEAS_SEL]			= &t3x_vdin_meas_sel.hw,
		[CLKID_VDIN_MEAS_DIV]			= &t3x_vdin_meas_div.hw,
		[CLKID_VDIN_MEAS]			= &t3x_vdin_meas.hw,
		[CLKID_CMPR_SEL]			= &t3x_cmpr_sel.hw,
		[CLKID_CMPR_DIV]			= &t3x_cmpr_div.hw,
		[CLKID_CMPR]				= &t3x_cmpr.hw,
		[CLKID_ADLA_0_SEL]			= &t3x_adla_0_sel.hw,
		[CLKID_ADLA_0_DIV]			= &t3x_adla_0_div.hw,
		[CLKID_ADLA_0]				= &t3x_adla_0.hw,
		[CLKID_ADLA_1_SEL]			= &t3x_adla_1_sel.hw,
		[CLKID_ADLA_1_DIV]			= &t3x_adla_1_div.hw,
		[CLKID_ADLA_1]				= &t3x_adla_1.hw,
		[CLKID_ADLA]				= &t3x_adla.hw,
		[CLKID_VID_LOCK_DIV]			= &t3x_vid_lock_div.hw,
		[CLKID_VID_LOCK]			= &t3x_vid_lock_clk.hw,
		[CLKID_SD_EMMC_C_SEL]			= &t3x_sd_emmc_c_sel.hw,
		[CLKID_SD_EMMC_C_DIV]			= &t3x_sd_emmc_c_div.hw,
		[CLKID_SD_EMMC_C]			= &t3x_sd_emmc_c.hw,
		[CLKID_SD_EMMC_B_SEL]			= &t3x_sd_emmc_b_sel.hw,
		[CLKID_SD_EMMC_B_DIV]			= &t3x_sd_emmc_b_div.hw,
		[CLKID_SD_EMMC_B]			= &t3x_sd_emmc_b.hw,
		[CLKID_SD_EMMC_A_SEL]			= &t3x_sd_emmc_a_sel.hw,
		[CLKID_SD_EMMC_A_DIV]			= &t3x_sd_emmc_a_div.hw,
		[CLKID_SD_EMMC_A]			= &t3x_sd_emmc_a.hw,
		[CLKID_USB_250M_SEL]			= &t3x_usb_250m_sel.hw,
		[CLKID_USB_250M_DIV]			= &t3x_usb_250m_div.hw,
		[CLKID_USB_250M]			= &t3x_usb_250m.hw,
		[CLKID_PCIE_400M_SEL]			= &t3x_pcie_400m_sel.hw,
		[CLKID_PCIE_400M_DIV]			= &t3x_pcie_400m_div.hw,
		[CLKID_PCIE_400M]			= &t3x_pcie_400m.hw,
		[CLKID_PCIE_CLK_SEL]			= &t3x_pcie_clk_sel.hw,
		[CLKID_PCIE_CLK_DIV]			= &t3x_pcie_clk_div.hw,
		[CLKID_PCIE_CLK]			= &t3x_pcie_clk.hw,
		[CLKID_PCIE_TL_CLK_SEL]			= &t3x_pcie_tl_sel.hw,
		[CLKID_PCIE_TL_CLK_DIV]			= &t3x_pcie_tl_div.hw,
		[CLKID_PCIE_TL_CLK]			= &t3x_pcie_tl.hw,
		[CLKID_CDAC_CLK_SEL]			= &t3x_cdac_sel.hw,
		[CLKID_CDAC_CLK_DIV]			= &t3x_cdac_div.hw,
		[CLKID_CDAC_CLK]			= &t3x_cdac.hw,
		[CLKID_SPICC0_SEL]			= &t3x_spicc0_sel.hw,
		[CLKID_SPICC0_DIV]			= &t3x_spicc0_div.hw,
		[CLKID_SPICC0]				= &t3x_spicc0.hw,
		[CLKID_SPICC1_SEL]			= &t3x_spicc1_sel.hw,
		[CLKID_SPICC1_DIV]			= &t3x_spicc1_div.hw,
		[CLKID_SPICC1]				= &t3x_spicc1.hw,
		[CLKID_SPICC2_SEL]			= &t3x_spicc2_sel.hw,
		[CLKID_SPICC2_DIV]			= &t3x_spicc2_div.hw,
		[CLKID_SPICC2]				= &t3x_spicc2.hw,
		[CLKID_SPICC3_SEL]			= &t3x_spicc3_sel.hw,
		[CLKID_SPICC3_DIV]			= &t3x_spicc3_div.hw,
		[CLKID_SPICC3]				= &t3x_spicc3.hw,
		[CLKID_SPICC4_SEL]			= &t3x_spicc4_sel.hw,
		[CLKID_SPICC4_DIV]			= &t3x_spicc4_div.hw,
		[CLKID_SPICC4]				= &t3x_spicc4.hw,
		[CLKID_SPICC5_SEL]			= &t3x_spicc5_sel.hw,
		[CLKID_SPICC5_DIV]			= &t3x_spicc5_div.hw,
		[CLKID_SPICC5]				= &t3x_spicc5.hw,
		[CLKID_PWM_A_SEL]			= &t3x_pwm_a_sel.hw,
		[CLKID_PWM_A_DIV]			= &t3x_pwm_a_div.hw,
		[CLKID_PWM_A]				= &t3x_pwm_a.hw,
		[CLKID_PWM_B_SEL]			= &t3x_pwm_b_sel.hw,
		[CLKID_PWM_B_DIV]			= &t3x_pwm_b_div.hw,
		[CLKID_PWM_B]				= &t3x_pwm_b.hw,
		[CLKID_PWM_C_SEL]			= &t3x_pwm_c_sel.hw,
		[CLKID_PWM_C_DIV]			= &t3x_pwm_c_div.hw,
		[CLKID_PWM_C]				= &t3x_pwm_c.hw,
		[CLKID_PWM_D_SEL]			= &t3x_pwm_d_sel.hw,
		[CLKID_PWM_D_DIV]			= &t3x_pwm_d_div.hw,
		[CLKID_PWM_D]				= &t3x_pwm_d.hw,
		[CLKID_PWM_E_SEL]			= &t3x_pwm_e_sel.hw,
		[CLKID_PWM_E_DIV]			= &t3x_pwm_e_div.hw,
		[CLKID_PWM_E]				= &t3x_pwm_e.hw,
		[CLKID_PWM_F_SEL]			= &t3x_pwm_f_sel.hw,
		[CLKID_PWM_F_DIV]			= &t3x_pwm_f_div.hw,
		[CLKID_PWM_F]				= &t3x_pwm_f.hw,
		[CLKID_PWM_G_SEL]			= &t3x_pwm_g_sel.hw,
		[CLKID_PWM_G_DIV]			= &t3x_pwm_g_div.hw,
		[CLKID_PWM_G]				= &t3x_pwm_g.hw,
		[CLKID_PWM_H_SEL]			= &t3x_pwm_h_sel.hw,
		[CLKID_PWM_H_DIV]			= &t3x_pwm_h_div.hw,
		[CLKID_PWM_H]				= &t3x_pwm_h.hw,
		[CLKID_PWM_I_SEL]			= &t3x_pwm_i_sel.hw,
		[CLKID_PWM_I_DIV]			= &t3x_pwm_i_div.hw,
		[CLKID_PWM_I]				= &t3x_pwm_i.hw,
		[CLKID_PWM_J_SEL]			= &t3x_pwm_j_sel.hw,
		[CLKID_PWM_J_DIV]			= &t3x_pwm_j_div.hw,
		[CLKID_PWM_J]				= &t3x_pwm_j.hw,
		[CLKID_SARADC_SEL]			= &t3x_saradc_sel.hw,
		[CLKID_SARADC_DIV]			= &t3x_saradc_div.hw,
		[CLKID_SARADC]				= &t3x_saradc.hw,
		[CLKID_GEN_SEL]				= &t3x_gen_sel.hw,
		[CLKID_GEN_DIV]				= &t3x_gen_div.hw,
		[CLKID_GEN]				= &t3x_gen.hw,
		[CLKID_ETH_RMII_SEL]			= &t3x_eth_rmii_sel.hw,
		[CLKID_ETH_RMII_DIV]			= &t3x_eth_rmii_div.hw,
		[CLKID_ETH_RMII]			= &t3x_eth_rmii.hw,
		[CLKID_ETH_DIV8]			= &t3x_eth_div8.hw,
		[CLKID_ETH_125M]			= &t3x_eth_125m.hw,
		[CLKID_TS_CLK_DIV]			= &t3x_ts_clk_div.hw,
		[CLKID_TS_CLK]				= &t3x_ts_clk.hw,
		[CLKID_HDMIRX_5M_SEL]			= &t3x_hdmirx_5m_sel.hw,
		[CLKID_HDMIRX_5M_DIV]			= &t3x_hdmirx_5m_div.hw,
		[CLKID_HDMIRX_5M]			= &t3x_hdmirx_5m.hw,
		[CLKID_HDMIRX_2M_SEL]			= &t3x_hdmirx_2m_sel.hw,
		[CLKID_HDMIRX_2M_DIV]			= &t3x_hdmirx_2m_div.hw,
		[CLKID_HDMIRX_2M]			= &t3x_hdmirx_2m.hw,
		[CLKID_HDMIRX_CFG_SEL]			= &t3x_hdmirx_cfg_sel.hw,
		[CLKID_HDMIRX_CFG_DIV]			= &t3x_hdmirx_cfg_div.hw,
		[CLKID_HDMIRX_CFG]			= &t3x_hdmirx_cfg.hw,
		[CLKID_HDMIRX_HDCP_SEL]			= &t3x_hdmirx_hdcp_sel.hw,
		[CLKID_HDMIRX_HDCP_DIV]			= &t3x_hdmirx_hdcp_div.hw,
		[CLKID_HDMIRX_HDCP]			= &t3x_hdmirx_hdcp.hw,
		[CLKID_HDMIRX_AUD_PLL_SEL]		= &t3x_hdmirx_aud_pll_sel.hw,
		[CLKID_HDMIRX_AUD_PLL_DIV]		= &t3x_hdmirx_aud_pll_div.hw,
		[CLKID_HDMIRX_AUD_PLL]			= &t3x_hdmirx_aud_pll.hw,
		[CLKID_HDMIRX_ACR_SEL]			= &t3x_hdmirx_acr_sel.hw,
		[CLKID_HDMIRX_ACR_DIV]			= &t3x_hdmirx_acr_div.hw,
		[CLKID_HDMIRX_ACR]			= &t3x_hdmirx_acr.hw,
		[CLKID_HDMIRX_METER_SEL]		= &t3x_hdmirx_meter_sel.hw,
		[CLKID_HDMIRX_METER_DIV]		= &t3x_hdmirx_meter_div.hw,
		[CLKID_HDMIRX_METER]			= &t3x_hdmirx_meter.hw,
		[CLKID_HDMIRX_AXI_SEL]			= &t3x_hdmirx_axi_sel.hw,
		[CLKID_HDMIRX_AXI_DIV]			= &t3x_hdmirx_axi_div.hw,
		[CLKID_HDMIRX_AXI]			= &t3x_hdmirx_axi.hw,
		[CLKID_ADC_EXTCLK_SEL]			= &t3x_adc_extclk_sel.hw,
		[CLKID_ADC_EXTCLK_DIV]			= &t3x_adc_extclk_div.hw,
		[CLKID_ADC_EXTCLK]			= &t3x_adc_extclk.hw,
		[CLKID_CTS_DEMOD_CORE_SEL]		= &t3x_cts_demod_core_sel.hw,
		[CLKID_CTS_DEMOD_CORE_DIV]		= &t3x_cts_demod_core_div.hw,
		[CLKID_CTS_DEMOD_CORE]			= &t3x_cts_demod_core.hw,
		[CLKID_CTS_DEMOD_CORE_T2_SEL]		= &t3x_cts_demod_core_t2_clk_sel.hw,
		[CLKID_CTS_DEMOD_CORE_T2_DIV]		= &t3x_cts_demod_core_t2_clk_div.hw,
		[CLKID_CTS_DEMOD_CORE_T2]		= &t3x_cts_demod_core_t2_clk.hw,
		[CLKID_CTS_TCON_PLL_CLK_SEL]		= &t3x_cts_tcon_pll_clk_sel.hw,
		[CLKID_CTS_TCON_PLL_CLK_DIV]		= &t3x_cts_tcon_pll_clk_div.hw,
		[CLKID_CTS_TCON_PLL_CLK]		= &t3x_cts_tcon_pll_clk.hw,
		[CLKID_ME_0_SEL]			= &t3x_me_0_sel.hw,
		[CLKID_ME_0_DIV]			= &t3x_me_0_div.hw,
		[CLKID_ME_0]				= &t3x_me_0.hw,
		[CLKID_ME_1_SEL]			= &t3x_me_1_sel.hw,
		[CLKID_ME_1_DIV]			= &t3x_me_1_div.hw,
		[CLKID_ME_1]				= &t3x_me_1.hw,
		[CLKID_ME]				= &t3x_me.hw,
		[CLKID_FRC_0_SEL]			= &t3x_frc_0_sel.hw,
		[CLKID_FRC_0_DIV]			= &t3x_frc_0_div.hw,
		[CLKID_FRC_0]				= &t3x_frc_0.hw,
		[CLKID_FRC_1_SEL]			= &t3x_frc_1_sel.hw,
		[CLKID_FRC_1_DIV]			= &t3x_frc_1_div.hw,
		[CLKID_FRC_1]				= &t3x_frc_1.hw,
		[CLKID_FRC]				= &t3x_frc.hw,
		[CLKID_DEMOD_32K_CLKIN]			= &t3x_demod_32k_clkin.hw,
		[CLKID_DEMOD_32K_DIV]			= &t3x_demod_32k_div.hw,
		[CLKID_DEMOD_32K]			= &t3x_demod_32k.hw,
		[CLKID_TSIN_0_SEL]			= &t3x_tsin_0_sel.hw,
		[CLKID_TSIN_0_DIV]			= &t3x_tsin_0_div.hw,
		[CLKID_TSIN_0]				= &t3x_tsin_0.hw,
		[CLKID_TSIN_1_SEL]			= &t3x_tsin_1_sel.hw,
		[CLKID_TSIN_1_DIV]			= &t3x_tsin_1_div.hw,
		[CLKID_TSIN_1]				= &t3x_tsin_1.hw,
		[CLKID_TSIN_SEL]			= &t3x_tsin_sel.hw,
		[CLKID_TSIN]				= &t3x_tsin.hw,
		[CLKID_VAFE_SEL]			= &t3x_vafe_sel.hw,
		[CLKID_VAFE_DIV]			= &t3x_vafe_div.hw,
		[CLKID_VAFE]				= &t3x_vafe.hw,
		[CLKID_SYS_CLK_DDR]			= &t3x_sys_clk_ddr.hw,
		[CLKID_SYS_CLK_ETHPHY]			= &t3x_sys_clk_ethphy.hw,
		[CLKID_SYS_CLK_MALI]			= &t3x_sys_clk_mali.hw,
		[CLKID_SYS_CLK_AOCPU]			= &t3x_sys_clk_aocpu.hw,
		[CLKID_SYS_CLK_AUCPU]			= &t3x_sys_clk_aucpu.hw,
		[CLKID_SYS_CLK_DEWARPC]			= &t3x_sys_clk_dewarpc.hw,
		[CLKID_SYS_CLK_DEWARPB]			= &t3x_sys_clk_dewarpb.hw,
		[CLKID_SYS_CLK_DEWARPA]			= &t3x_sys_clk_dewarpa.hw,
		[CLKID_SYS_CLK_AMPIPE_NAND]		= &t3x_sys_clk_ampipe_nand.hw,
		[CLKID_SYS_CLK_AMPIPE_ETH]		= &t3x_sys_clk_ampipe_eth.hw,
		[CLKID_SYS_CLK_AM2AXI0]			= &t3x_sys_clk_am2axi0.hw,
		[CLKID_SYS_CLK_SD_EMMC_A]		= &t3x_sys_clk_sdemmca.hw,
		[CLKID_SYS_CLK_SD_EMMC_B]		= &t3x_sys_clk_sdemmcb.hw,
		[CLKID_SYS_CLK_SD_EMMC_C]		= &t3x_sys_clk_sdemmcc.hw,
		[CLKID_SYS_CLK_SD_VC9000E]		= &t3x_sys_clk_vc9000e.hw,
		[CLKID_SYS_CLK_ACODEC]			= &t3x_sys_clk_acodec.hw,
		[CLKID_SYS_CLK_SPIFC]			= &t3x_sys_clk_spifc.hw,
		[CLKID_SYS_CLK_MSR_CLK]			= &t3x_sys_clk_msr_clk.hw,
		[CLKID_SYS_CLK_IR_CTRL]			= &t3x_sys_clk_ir_ctrl.hw,
		[CLKID_SYS_CLK_AUDIO]			= &t3x_sys_clk_audio.hw,
		[CLKID_SYS_CLK_DOS]			= &t3x_sys_clk_dos.hw,
		[CLKID_SYS_CLK_ETH]			= &t3x_sys_clk_eth.hw,
		[CLKID_SYS_CLK_UART_A]			= &t3x_sys_clk_uart_a.hw,
		[CLKID_SYS_CLK_UART_B]			= &t3x_sys_clk_uart_b.hw,
		[CLKID_SYS_CLK_UART_C]			= &t3x_sys_clk_uart_c.hw,
		[CLKID_SYS_CLK_UART_D]			= &t3x_sys_clk_uart_d.hw,
		[CLKID_SYS_CLK_UART_E]			= &t3x_sys_clk_uart_e.hw,
		[CLKID_SYS_CLK_UART_F]			= &t3x_sys_clk_uart_f.hw,
		[CLKID_SYS_CLK_SPICC2]			= &t3x_sys_clk_spicc2.hw,
		[CLKID_SYS_CLK_TS_A55]			= &t3x_sys_clk_ts_a55.hw,
		[CLKID_SYS_CLK_GE2D]			= &t3x_sys_clk_ge2d.hw,
		[CLKID_SYS_CLK_SPICC0]			= &t3x_sys_clk_spicc0.hw,
		[CLKID_SYS_CLK_SPICC1]			= &t3x_sys_clk_spicc1.hw,
		[CLKID_SYS_CLK_SMARTCARD]		= &t3x_sys_clk_smartcard.hw,
		[CLKID_SYS_CLK_PCIE]			= &t3x_sys_clk_pcie.hw,
		[CLKID_SYS_CLK_PCIEPHY]			= &t3x_sys_clk_pciephy.hw,
		[CLKID_SYS_CLK_USB]			= &t3x_sys_clk_usb.hw,
		[CLKID_SYS_CLK_PCIEPHY0]		= &t3x_sys_clk_pciephy0.hw,
		[CLKID_SYS_CLK_PCIEPHY1]		= &t3x_sys_clk_pciephy1.hw,
		[CLKID_SYS_CLK_PCIEPHY2]		= &t3x_sys_clk_pciephy2.hw,
		[CLKID_SYS_CLK_I2C_M_A]			= &t3x_sys_clk_i2c_m_a.hw,
		[CLKID_SYS_CLK_I2C_M_B]			= &t3x_sys_clk_i2c_m_b.hw,
		[CLKID_SYS_CLK_I2C_M_C]			= &t3x_sys_clk_i2c_m_c.hw,
		[CLKID_SYS_CLK_I2C_M_D]			= &t3x_sys_clk_i2c_m_d.hw,
		[CLKID_SYS_CLK_I2C_M_E]			= &t3x_sys_clk_i2c_m_e.hw,
		[CLKID_SYS_CLK_I2C_M_F]			= &t3x_sys_clk_i2c_m_f.hw,
		[CLKID_SYS_CLK_I2C_S_A]			= &t3x_sys_clk_i2c_s_a.hw,
		[CLKID_SYS_CLK_CMPR]			= &t3x_sys_clk_cmpr.hw,
		[CLKID_SYS_CLK_MMC_PCLK]		= &t3x_sys_clk_mmc_pclk.hw,
		[CLKID_SYS_CLK_HDMI_PCLK]		= &t3x_sys_clk_hdmi_pclk.hw,
		[CLKID_SYS_CLK_HDMI20_AEC_CLK]		= &t3x_sys_clk_hdmi20_aes_clk.hw,
		[CLKID_SYS_CLK_PCLK_SYS_CPU_APB]	= &t3x_sys_clk_pclk_sys_cpu_apb.hw,
		[CLKID_SYS_CLK_CEC]			= &t3x_sys_clk_cec.hw,
		[CLKID_SYS_CLK_VPU_INTR]		= &t3x_sys_clk_vpu_intr.hw,
		[CLKID_SYS_CLK_SAR_ADC]			= &t3x_sys_clk_sar_adc.hw,
		[CLKID_SYS_CLK_GIC]			= &t3x_sys_clk_gic.hw,
		[CLKID_SYS_CLK_TS_GPU]			= &t3x_sys_clk_ts_gpu.hw,
		[CLKID_SYS_CLK_TS_NNA]			= &t3x_sys_clk_ts_nna.hw,
		[CLKID_SYS_CLK_TS_VPU]			= &t3x_sys_clk_ts_vpu.hw,
		[CLKID_SYS_CLK_DOS]			= &t3x_sys_clk_ts_dos.hw,
		[CLKID_SYS_CLK_PWM_AB]			= &t3x_sys_clk_pwm_ab.hw,
		[CLKID_SYS_CLK_PWM_CD]			= &t3x_sys_clk_pwm_cd.hw,
		[CLKID_SYS_CLK_PWM_EF]			= &t3x_sys_clk_pwm_ef.hw,
		[CLKID_SYS_CLK_PWM_GH]			= &t3x_sys_clk_pwm_gh.hw,
		[CLKID_SYS_CLK_PWM_IJ]			= &t3x_sys_clk_pwm_ij.hw,
		[NR_CLKS]				= NULL
	},
	.num = NR_CLKS,
};

/* Convenience table to populate regmap in .probe */
static struct clk_regmap *const t3x_clk_regmaps[] = {
	&t3x_rtc_32k_clkin,
	&t3x_rtc_32k_div,
	&t3x_rtc_32k_xtal,
	&t3x_rtc_32k_sel,
	&t3x_rtc_clk,
	&t3x_sys_clk,
	&t3x_axi_clk,
	&t3x_ceca_32k_clkin,
	&t3x_ceca_32k_div,
	&t3x_ceca_32k_sel_pre,
	&t3x_ceca_32k_sel,
	&t3x_ceca_32k_clkout,
	&t3x_cecb_32k_clkin,
	&t3x_cecb_32k_div,
	&t3x_cecb_32k_sel_pre,
	&t3x_cecb_32k_sel,
	&t3x_cecb_32k_clkout,
	&t3x_sc_clk_sel,
	&t3x_sc_clk_div,
	&t3x_dspa_0_sel,
	&t3x_dspa_0_div,
	&t3x_dspa_0,
	&t3x_dspa_1_sel,
	&t3x_dspa_1_div,
	&t3x_dspa_1,
	&t3x_dspa,
	&t3x_sc_clk,
	&t3x_24m_clk_gate,
	&t3x_12m_clk,
	&t3x_25m_clk_div,
	&t3x_25m_clk,
	&t3x_vclk_sel,
	&t3x_vclk2_sel,
	&t3x_vclk_input,
	&t3x_vclk2_input,
	&t3x_vclk_div,
	&t3x_vclk2_div,
	&t3x_vclk,
	&t3x_vclk2,
	&t3x_vclk_div1,
	&t3x_vclk_div2_en,
	&t3x_vclk_div4_en,
	&t3x_vclk_div6_en,
	&t3x_vclk_div12_en,
	&t3x_vclk2_div1,
	&t3x_vclk2_div2_en,
	&t3x_vclk2_div4_en,
	&t3x_vclk2_div6_en,
	&t3x_vclk2_div12_en,
	&t3x_cts_enci_sel,
	&t3x_cts_encp_sel,
	&t3x_cts_vdac_sel,
	&t3x_hdmi_tx_sel,
	&t3x_cts_enci,
	&t3x_cts_encp,
	&t3x_cts_vdac,
	&t3x_hdmi_tx,
	&t3x_mali_0_sel,
	&t3x_mali_0_div,
	&t3x_mali_0,
	&t3x_mali_1_sel,
	&t3x_mali_1_div,
	&t3x_mali_1,
	&t3x_mali,
	&t3x_vdec_0_sel,
	&t3x_vdec_0_div,
	&t3x_vdec_0,
	&t3x_vdec_1_sel,
	&t3x_vdec_1_div,
	&t3x_vdec_1,
	&t3x_vdec,
	&t3x_hcodec_0_sel,
	&t3x_hcodec_0_div,
	&t3x_hcodec_0,
	&t3x_hcodec_1_sel,
	&t3x_hcodec_1_div,
	&t3x_hcodec_1,
	&t3x_hcodec,
	&t3x_hevc_0_sel,
	&t3x_hevc_0_div,
	&t3x_hevc_0,
	&t3x_hevc_1_sel,
	&t3x_hevc_1_div,
	&t3x_hevc_1,
	&t3x_hevc,
	&t3x_vpu_0_sel,
	&t3x_vpu_0_div,
	&t3x_vpu_0,
	&t3x_vpu_1_sel,
	&t3x_vpu_1_div,
	&t3x_vpu_1,
	&t3x_vpu,
	&t3x_vpu_clkb_tmp_sel,
	&t3x_vpu_clkb_tmp_div,
	&t3x_vpu_clkb_tmp,
	&t3x_vpu_clkb_div,
	&t3x_vpu_clkb,
	&t3x_vapb_0_sel,
	&t3x_vapb_0_div,
	&t3x_vapb_0,
	&t3x_vapb_1_sel,
	&t3x_vapb_1_div,
	&t3x_vapb_1,
	&t3x_vapb,
	&t3x_ge2d_sel,
	&t3x_ge2d_div,
	&t3x_ge2d,
	&t3x_vdin_meas_sel,
	&t3x_vdin_meas_div,
	&t3x_vdin_meas,
	&t3x_cmpr_sel,
	&t3x_cmpr_div,
	&t3x_cmpr,
	&t3x_adla_0_sel,
	&t3x_adla_0_div,
	&t3x_adla_0,
	&t3x_adla_1_sel,
	&t3x_adla_1_div,
	&t3x_adla_1,
	&t3x_adla,
	&t3x_vid_lock_div,
	&t3x_vid_lock_clk,
	&t3x_sd_emmc_c_sel,
	&t3x_sd_emmc_c_div,
	&t3x_sd_emmc_c,
	&t3x_sd_emmc_b_sel,
	&t3x_sd_emmc_b_div,
	&t3x_sd_emmc_b,
	&t3x_sd_emmc_a_sel,
	&t3x_sd_emmc_a_div,
	&t3x_sd_emmc_a,
	&t3x_usb_250m_sel,
	&t3x_usb_250m_div,
	&t3x_usb_250m,
	&t3x_pcie_400m_sel,
	&t3x_pcie_400m_div,
	&t3x_pcie_400m,
	&t3x_pcie_clk_sel,
	&t3x_pcie_clk_div,
	&t3x_pcie_clk,
	&t3x_pcie_tl_sel,
	&t3x_pcie_tl_div,
	&t3x_pcie_tl,
	&t3x_cdac_sel,
	&t3x_cdac_div,
	&t3x_cdac,
	&t3x_spicc0_sel,
	&t3x_spicc0_div,
	&t3x_spicc0,
	&t3x_spicc1_sel,
	&t3x_spicc1_div,
	&t3x_spicc1,
	&t3x_spicc2_sel,
	&t3x_spicc2_div,
	&t3x_spicc2,
	&t3x_spicc3_sel,
	&t3x_spicc3_div,
	&t3x_spicc3,
	&t3x_spicc4_sel,
	&t3x_spicc4_div,
	&t3x_spicc4,
	&t3x_spicc5_sel,
	&t3x_spicc5_div,
	&t3x_spicc5,
	&t3x_pwm_a_sel,
	&t3x_pwm_a_div,
	&t3x_pwm_a,
	&t3x_pwm_b_sel,
	&t3x_pwm_b_div,
	&t3x_pwm_b,
	&t3x_pwm_c_sel,
	&t3x_pwm_c_div,
	&t3x_pwm_c,
	&t3x_pwm_d_sel,
	&t3x_pwm_d_div,
	&t3x_pwm_d,
	&t3x_pwm_e_sel,
	&t3x_pwm_e_div,
	&t3x_pwm_e,
	&t3x_pwm_f_sel,
	&t3x_pwm_f_div,
	&t3x_pwm_f,
	&t3x_pwm_g_sel,
	&t3x_pwm_g_div,
	&t3x_pwm_g,
	&t3x_pwm_h_sel,
	&t3x_pwm_h_div,
	&t3x_pwm_h,
	&t3x_pwm_i_sel,
	&t3x_pwm_i_div,
	&t3x_pwm_i,
	&t3x_pwm_j_sel,
	&t3x_pwm_j_div,
	&t3x_pwm_j,
	&t3x_saradc_sel,
	&t3x_saradc_div,
	&t3x_saradc,
	&t3x_gen_sel,
	&t3x_gen_div,
	&t3x_gen,
	&t3x_eth_rmii_sel,
	&t3x_eth_rmii_div,
	&t3x_eth_rmii,
	&t3x_eth_125m,
	&t3x_ts_clk_div,
	&t3x_ts_clk,
	&t3x_hdmirx_5m_sel,
	&t3x_hdmirx_5m_div,
	&t3x_hdmirx_5m,
	&t3x_hdmirx_2m_sel,
	&t3x_hdmirx_2m_div,
	&t3x_hdmirx_2m,
	&t3x_hdmirx_cfg_sel,
	&t3x_hdmirx_cfg_div,
	&t3x_hdmirx_cfg,
	&t3x_hdmirx_hdcp_sel,
	&t3x_hdmirx_hdcp_div,
	&t3x_hdmirx_hdcp,
	&t3x_hdmirx_aud_pll_sel,
	&t3x_hdmirx_aud_pll_div,
	&t3x_hdmirx_aud_pll,
	&t3x_hdmirx_acr_sel,
	&t3x_hdmirx_acr_div,
	&t3x_hdmirx_acr,
	&t3x_hdmirx_meter_sel,
	&t3x_hdmirx_meter_div,
	&t3x_hdmirx_meter,
	&t3x_hdmirx_axi_sel,
	&t3x_hdmirx_axi_div,
	&t3x_hdmirx_axi,
	&t3x_adc_extclk_sel,
	&t3x_adc_extclk_div,
	&t3x_adc_extclk,
	&t3x_cts_demod_core_sel,
	&t3x_cts_demod_core_div,
	&t3x_cts_demod_core,
	&t3x_cts_demod_core_t2_clk_sel,
	&t3x_cts_demod_core_t2_clk_div,
	&t3x_cts_demod_core_t2_clk,
	&t3x_cts_tcon_pll_clk_sel,
	&t3x_cts_tcon_pll_clk_div,
	&t3x_cts_tcon_pll_clk,
	&t3x_me_0_sel,
	&t3x_me_0_div,
	&t3x_me_0,
	&t3x_me_1_sel,
	&t3x_me_1_div,
	&t3x_me_1,
	&t3x_me_1_sel,
	&t3x_me,
	&t3x_frc_0_sel,
	&t3x_frc_0_div,
	&t3x_frc_0,
	&t3x_frc_1_sel,
	&t3x_frc_1_div,
	&t3x_frc_1,
	&t3x_frc_1_sel,
	&t3x_frc,
	&t3x_demod_32k_clkin,
	&t3x_demod_32k_div,
	&t3x_demod_32k,
	&t3x_tsin_0_sel,
	&t3x_tsin_0_div,
	&t3x_tsin_0,
	&t3x_tsin_1_sel,
	&t3x_tsin_1_div,
	&t3x_tsin_1,
	&t3x_tsin_sel,
	&t3x_tsin,
	&t3x_vafe_sel,
	&t3x_vafe_div,
	&t3x_vafe,
	&t3x_sys_clk_ddr,
	&t3x_sys_clk_ethphy,
	&t3x_sys_clk_mali,
	&t3x_sys_clk_aocpu,
	&t3x_sys_clk_aucpu,
	&t3x_sys_clk_dewarpc,
	&t3x_sys_clk_dewarpb,
	&t3x_sys_clk_dewarpa,
	&t3x_sys_clk_ampipe_nand,
	&t3x_sys_clk_ampipe_eth,
	&t3x_sys_clk_am2axi0,
	&t3x_sys_clk_sdemmca,
	&t3x_sys_clk_sdemmcb,
	&t3x_sys_clk_sdemmcc,
	&t3x_sys_clk_vc9000e,
	&t3x_sys_clk_acodec,
	&t3x_sys_clk_spifc,
	&t3x_sys_clk_msr_clk,
	&t3x_sys_clk_ir_ctrl,
	&t3x_sys_clk_audio,
	&t3x_sys_clk_dos,
	&t3x_sys_clk_eth,
	&t3x_sys_clk_uart_a,
	&t3x_sys_clk_uart_b,
	&t3x_sys_clk_uart_c,
	&t3x_sys_clk_uart_d,
	&t3x_sys_clk_uart_e,
	&t3x_sys_clk_uart_f,
	&t3x_sys_clk_spicc2,
	&t3x_sys_clk_ts_a55,
	&t3x_sys_clk_ge2d,
	&t3x_sys_clk_spicc0,
	&t3x_sys_clk_spicc1,
	&t3x_sys_clk_smartcard,
	&t3x_sys_clk_pcie,
	&t3x_sys_clk_pciephy,
	&t3x_sys_clk_usb,
	&t3x_sys_clk_pciephy0,
	&t3x_sys_clk_pciephy1,
	&t3x_sys_clk_pciephy2,
	&t3x_sys_clk_i2c_m_a,
	&t3x_sys_clk_i2c_m_b,
	&t3x_sys_clk_i2c_m_c,
	&t3x_sys_clk_i2c_m_d,
	&t3x_sys_clk_i2c_m_e,
	&t3x_sys_clk_i2c_m_f,
	&t3x_sys_clk_i2c_s_a,
	&t3x_sys_clk_cmpr,
	&t3x_sys_clk_mmc_pclk,
	&t3x_sys_clk_hdmi_pclk,
	&t3x_sys_clk_hdmi20_aes_clk,
	&t3x_sys_clk_pclk_sys_cpu_apb,
	&t3x_sys_clk_cec,
	&t3x_sys_clk_vpu_intr,
	&t3x_sys_clk_sar_adc,
	&t3x_sys_clk_gic,
	&t3x_sys_clk_ts_gpu,
	&t3x_sys_clk_ts_nna,
	&t3x_sys_clk_ts_vpu,
	&t3x_sys_clk_ts_dos,
	&t3x_sys_clk_pwm_ab,
	&t3x_sys_clk_pwm_cd,
	&t3x_sys_clk_pwm_ef,
	&t3x_sys_clk_pwm_gh,
	&t3x_sys_clk_pwm_ij,
	&t3x_sys_pll_dco,
	&t3x_sys_pll,
	&t3x_sys1_pll_dco,
	&t3x_sys1_pll,
	&t3x_sys2_pll_dco,
	&t3x_sys2_pll,
	&t3x_sys3_pll_dco,
	&t3x_sys3_pll,
	&t3x_fixed_pll_dco,
	&t3x_fixed_pll,
	&t3x_fclk_div2,
	&t3x_fclk_div3,
	&t3x_fclk_div4,
	&t3x_fclk_div5,
	&t3x_fclk_div7,
	&t3x_fclk_div2p5,
	&t3x_gp0_pll_dco,
	&t3x_gp0_pll,
	&t3x_gp1_pll_dco,
	&t3x_gp1_pll,
	&t3x_hifi_pll_dco,
	&t3x_hifi_pll,
	&t3x_hifi1_pll_dco,
	&t3x_hifi1_pll,
	&t3x_mpll_50m
};

static struct clk_regmap *const t3x_cpu_clk_regmaps[] = {
	&t3x_cpu_dyn_clk,
	&t3x_cpu_clk,
	&t3x_a76_dyn_clk,
	&t3x_a76_clk,
	&t3x_dsu_dyn_clk,
	&t3x_dsu_clk,
	&t3x_dsu_final_clk,
};

static struct clk_regmap *const t3x_pll_clk_regmaps[] = {
	&t3x_pcie_pll,
	&t3x_pcie_hcsl,
};

static int meson_t3x_dvfs_setup(struct platform_device *pdev)
{
	int ret;

	/* avoid cpu/dsu run at 24M in dvfs, remove it here.
	 * cpu or a76 do it in dvfs init
	 */
	if (clk_set_rate(t3x_dsu_dyn_clk.hw.clk, 1000000000))
		pr_err("%s: set dsu dyn clock to 1G failed\n", __func__);
	/* Setup cluster 0 clock notifier for sys_pll */
/*
 *	ret = clk_notifier_register(t3x_sys_pll.hw.clk,
 *				    &t3x_sys_pll_nb_data.nb);
 *	if (ret) {
 *		dev_err(&pdev->dev, "failed to register sys_pll notifier\n");
 *		return ret;
 *	}
 */
	/* Setup cluster 1 clock notifier for sys1_pll */
 /*	ret = clk_notifier_register(t3x_sys1_pll.hw.clk,
  *				    &t3x_sys1_pll_nb_data.nb);
  *	if (ret) {
  *		dev_err(&pdev->dev, "failed to register sys1_pll notifier\n");
  *		return ret;
  *	}
  */
	/* Setup DSU clock notifier for sys3_pll */
	ret = clk_notifier_register(t3x_sys3_pll.hw.clk,
				    &t3x_sys3_pll_nb_data.nb);
	if (ret) {
		dev_err(&pdev->dev, "failed to register sys3_pll notifier\n");
		return ret;
	}

	return 0;
}

static int meson_t3x_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct regmap *basic_map, *pll_map, *cpu_map;
	int ret, i;

	/* Get regmap for different clock area */
	basic_map = meson_clk_regmap_resource(pdev, dev, 0);
	if (IS_ERR(basic_map)) {
		dev_err(dev, "basic clk registers not found\n");
		return PTR_ERR(basic_map);
	}

	pll_map = meson_clk_regmap_resource(pdev, dev, 1);
	if (IS_ERR(pll_map)) {
		dev_err(dev, "pll clk registers not found\n");
		return PTR_ERR(basic_map);
	}

	cpu_map = meson_clk_regmap_resource(pdev, dev, 2);
	if (IS_ERR(cpu_map)) {
		dev_err(dev, "cpu clk registers not found\n");
		return PTR_ERR(cpu_map);
	}

	/* Populate regmap for the regmap backed clocks */
	for (i = 0; i < ARRAY_SIZE(t3x_clk_regmaps); i++)
		t3x_clk_regmaps[i]->map = basic_map;

	for (i = 0; i < ARRAY_SIZE(t3x_cpu_clk_regmaps); i++)
		t3x_cpu_clk_regmaps[i]->map = cpu_map;

	for (i = 0; i < ARRAY_SIZE(t3x_pll_clk_regmaps); i++)
		t3x_pll_clk_regmaps[i]->map = pll_map;

	for (i = 0; i < t3x_hw_onecell_data.num; i++) {
		/* array might be sparse */
		if (!t3x_hw_onecell_data.hws[i])
			continue;

		pr_debug("registering %dth  %s\n", i,
			t3x_hw_onecell_data.hws[i]->init->name);

		ret = devm_clk_hw_register(dev, t3x_hw_onecell_data.hws[i]);
		if (ret) {
			dev_err(dev, "Clock registration failed\n");
			return ret;
		}
#ifdef CONFIG_AMLOGIC_CLK_DEBUG
		ret = devm_clk_hw_register_clkdev(dev, t3x_hw_onecell_data.hws[i],
					  NULL,
					  clk_hw_get_name(t3x_hw_onecell_data.hws[i]));
		if (ret < 0) {
			dev_err(dev, "Failed to clkdev register: %d\n", ret);
			return ret;
		}
#endif
	}

	meson_t3x_dvfs_setup(pdev);

	return devm_of_clk_add_hw_provider(dev, of_clk_hw_onecell_get,
					   &t3x_hw_onecell_data);
}

static const struct of_device_id clkc_match_table[] = {
	{
		.compatible = "amlogic,t3x-clkc"
	},
	{}
};

static struct platform_driver t3x_driver = {
	.probe		= meson_t3x_probe,
	.driver		= {
		.name	= "t3x-clkc",
		.of_match_table = clkc_match_table,
	},
};

builtin_platform_driver(t3x_driver);

MODULE_LICENSE("GPL v2");
