// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2015 Endless Mobile, Inc.
 * Author: Carlo Caione <carlo@endlessm.com>
 *
 * Copyright (c) 2018 Baylibre, SAS.
 * Author: Jerome Brunet <jbrunet@baylibre.com>
 */

/*
 * In the most basic form, a Meson PLL is composed as follows:
 *
 *                     PLL
 *        +--------------------------------+
 *        |                                |
 *        |             +--+               |
 *  in >>-----[ /N ]--->|  |      +-----+  |
 *        |             |  |------| DCO |---->> out
 *        |  +--------->|  |      +--v--+  |
 *        |  |          +--+         |     |
 *        |  |                       |     |
 *        |  +--[ *(M + (F/Fmax) ]<--+     |
 *        |                                |
 *        +--------------------------------+
 *
 * out = in * (m + frac / frac_max) / n
 */

#include <linux/clk-provider.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/math64.h>
#include <linux/module.h>
#include <linux/rational.h>
#include "clk-regmap.h"
#include "clk-pll.h"
#ifdef CONFIG_AMLOGIC_MODIFY
#include <linux/arm-smccc.h>
#endif

#define FIXED_FRAC_WEIGHT_PRECISION	100000

static inline struct meson_clk_pll_data *
meson_clk_pll_data(struct clk_regmap *clk)
{
	return (struct meson_clk_pll_data *)clk->data;
}

static int __pll_round_closest_mult(struct meson_clk_pll_data *pll)
{
	if ((pll->flags & CLK_MESON_PLL_ROUND_CLOSEST) &&
	    !MESON_PARM_APPLICABLE(&pll->frac))
		return 1;

	return 0;
}

#if defined CONFIG_AMLOGIC_MODIFY && defined CONFIG_ARM
static unsigned long __pll_params_to_rate(unsigned long parent_rate,
					  unsigned int m, unsigned int n,
					  unsigned int frac,
					  struct meson_clk_pll_data *pll,
					  unsigned int od)
#else
static unsigned long __pll_params_to_rate(unsigned long parent_rate,
					  unsigned int m, unsigned int n,
					  unsigned int frac,
					  struct meson_clk_pll_data *pll)
#endif
{
	u64 rate = (u64)parent_rate * m;
	u64 frac_rate;

	if (frac && MESON_PARM_APPLICABLE(&pll->frac)) {
		frac_rate = (u64)parent_rate * frac;
		if (frac & (1 << (pll->frac.width - 1))) {
			if (pll->flags & CLK_MESON_PLL_FIXED_FRAC_WEIGHT_PRECISION)
				rate -= DIV_ROUND_UP_ULL(frac_rate, FIXED_FRAC_WEIGHT_PRECISION);
			else
				rate -= DIV_ROUND_UP_ULL(frac_rate,
						 (1 << (pll->frac.width - 2)));
		} else {
			if (pll->flags & CLK_MESON_PLL_FIXED_FRAC_WEIGHT_PRECISION)
				rate += DIV_ROUND_UP_ULL(frac_rate, FIXED_FRAC_WEIGHT_PRECISION);
			else
				rate += DIV_ROUND_UP_ULL(frac_rate,
						 (1 << (pll->frac.width - 2)));
		}
	}

	if (n == 0)
		return 0;

#if defined CONFIG_AMLOGIC_MODIFY && defined CONFIG_ARM
	return DIV_ROUND_UP_ULL(rate, n) >> od;
#else
	return DIV_ROUND_UP_ULL(rate, n);
#endif
}

#if defined CONFIG_AMLOGIC_MODIFY && defined CONFIG_ARM
static unsigned long meson_clk_pll_recalc_rate(struct clk_hw *hw,
					       unsigned long parent_rate)
{
	struct clk_regmap *clk = to_clk_regmap(hw);
	struct meson_clk_pll_data *pll = meson_clk_pll_data(clk);
	unsigned int m, n, frac, od;

	n = meson_parm_read(clk->map, &pll->n);
	m = meson_parm_read(clk->map, &pll->m);
	od = MESON_PARM_APPLICABLE(&pll->od) ?
		meson_parm_read(clk->map, &pll->od) :
		0;

	frac = MESON_PARM_APPLICABLE(&pll->frac) ?
		meson_parm_read(clk->map, &pll->frac) :
		0;

	return __pll_params_to_rate(parent_rate, m, n, frac, pll, od);
}
#else
static unsigned long meson_clk_pll_recalc_rate(struct clk_hw *hw,
					       unsigned long parent_rate)
{
	struct clk_regmap *clk = to_clk_regmap(hw);
	struct meson_clk_pll_data *pll = meson_clk_pll_data(clk);
	unsigned int m, n, frac;

	n = meson_parm_read(clk->map, &pll->n);

	/*
	 * On some HW, N is set to zero on init. This value is invalid as
	 * it would result in a division by zero. The rate can't be
	 * calculated in this case
	 */
	if (n == 0)
		return 0;

	m = meson_parm_read(clk->map, &pll->m);

	frac = MESON_PARM_APPLICABLE(&pll->frac) ?
		meson_parm_read(clk->map, &pll->frac) :
		0;

	return __pll_params_to_rate(parent_rate, m, n, frac, pll);
}
#endif

#if defined CONFIG_AMLOGIC_MODIFY && defined CONFIG_ARM
static unsigned int __pll_params_with_frac(unsigned long rate,
					   unsigned long parent_rate,
					   unsigned int m,
					   unsigned int n,
					   unsigned int od,
					   struct meson_clk_pll_data *pll)
{
	unsigned int frac_max;
	u64 val = (u64)rate * n;

	if (pll->flags & CLK_MESON_PLL_FIXED_FRAC_WEIGHT_PRECISION)
		frac_max = FIXED_FRAC_WEIGHT_PRECISION;
	else
		frac_max = (1 << (pll->frac.width - 2));
	/* Bail out if we are already over the requested rate */
	if (rate < parent_rate * m / n)
		return 0;

	val = val * (1 << od);
	if (pll->flags & CLK_MESON_PLL_ROUND_CLOSEST)
		val = DIV_ROUND_CLOSEST_ULL(val * frac_max, parent_rate);
	else
		val = div_u64(val * frac_max, parent_rate);

	val -= m * frac_max;

	return min((unsigned int)val, (frac_max - 1));
}
#else
static unsigned int __pll_params_with_frac(unsigned long rate,
					   unsigned long parent_rate,
					   unsigned int m,
					   unsigned int n,
					   struct meson_clk_pll_data *pll)
{
	unsigned int frac_max;
	u64 val = (u64)rate * n;

	if (pll->flags & CLK_MESON_PLL_FIXED_FRAC_WEIGHT_PRECISION)
		frac_max = FIXED_FRAC_WEIGHT_PRECISION;
	else
		frac_max = (1 << (pll->frac.width - 2));
	/* Bail out if we are already over the requested rate */
	if (rate < parent_rate * m / n)
		return 0;

	if (pll->flags & CLK_MESON_PLL_ROUND_CLOSEST)
		val = DIV_ROUND_CLOSEST_ULL(val * frac_max, parent_rate);
	else
		val = div_u64(val * frac_max, parent_rate);

	val -= m * frac_max;

	return min((unsigned int)val, (frac_max - 1));
}
#endif

static bool meson_clk_pll_is_better(unsigned long rate,
				    unsigned long best,
				    unsigned long now,
				    struct meson_clk_pll_data *pll)
{
	if (__pll_round_closest_mult(pll)) {
		/* Round Closest */
		if (abs(now - rate) < abs(best - rate))
			return true;
	} else {
		/* Round down */
		if (now <= rate && best < now)
			return true;
	}

	return false;
}

#if defined CONFIG_AMLOGIC_MODIFY && defined CONFIG_ARM
static int meson_clk_get_pll_table_index(unsigned int index,
					 unsigned int *m,
					 unsigned int *n,
					 struct meson_clk_pll_data *pll,
					 unsigned int *od)
#else
static int meson_clk_get_pll_table_index(unsigned int index,
					 unsigned int *m,
					 unsigned int *n,
					 struct meson_clk_pll_data *pll)
#endif
{
	if (!pll->table[index].n)
		return -EINVAL;

	*m = pll->table[index].m;
	*n = pll->table[index].n;
#if defined CONFIG_AMLOGIC_MODIFY && defined CONFIG_ARM
	*od = pll->table[index].od;
#endif

	return 0;
}

#if defined CONFIG_AMLOGIC_MODIFY && defined CONFIG_ARM
static int meson_clk_get_pll_get_index(unsigned long rate,
				       unsigned long parent_rate,
				       unsigned int index,
				       unsigned int *m,
				       unsigned int *n,
				       struct meson_clk_pll_data *pll,
				       unsigned int *od)
{
	/* only support table in arm32 */
	if (pll->table)
		return meson_clk_get_pll_table_index(index, m, n, pll, od);

	return -EINVAL;
}
#else
static unsigned int meson_clk_get_pll_range_m(unsigned long rate,
					      unsigned long parent_rate,
					      unsigned int n,
					      struct meson_clk_pll_data *pll)
{
	u64 val = (u64)rate * n;

	if (__pll_round_closest_mult(pll))
		return DIV_ROUND_CLOSEST_ULL(val, parent_rate);

	return div_u64(val,  parent_rate);
}

static int meson_clk_get_pll_range_index(unsigned long rate,
					 unsigned long parent_rate,
					 unsigned int index,
					 unsigned int *m,
					 unsigned int *n,
					 struct meson_clk_pll_data *pll)
{
	*n = index + 1;

	/* Check the predivider range */
	if (*n >= (1 << pll->n.width))
		return -EINVAL;

	if (*n == 1) {
		/* Get the boundaries out the way */
		if (rate <= pll->range->min * parent_rate) {
			*m = pll->range->min;
			return -ENODATA;
		} else if (rate >= pll->range->max * parent_rate) {
			*m = pll->range->max;
			return -ENODATA;
		}
	}

	*m = meson_clk_get_pll_range_m(rate, parent_rate, *n, pll);

	/* the pre-divider gives a multiplier too big - stop */
	if (*m >= (1 << pll->m.width))
		return -EINVAL;

	return 0;
}

static int meson_clk_get_pll_get_index(unsigned long rate,
				       unsigned long parent_rate,
				       unsigned int index,
				       unsigned int *m,
				       unsigned int *n,
				       struct meson_clk_pll_data *pll)
{
	if (pll->range)
		return meson_clk_get_pll_range_index(rate, parent_rate,
						     index, m, n, pll);
	else if (pll->table)
		return meson_clk_get_pll_table_index(index, m, n, pll);

	return -EINVAL;
}
#endif

#if defined CONFIG_AMLOGIC_MODIFY && defined CONFIG_ARM
static int meson_clk_get_pll_settings(unsigned long rate,
				      unsigned long parent_rate,
				      unsigned int *best_m,
				      unsigned int *best_n,
				      struct meson_clk_pll_data *pll,
				      unsigned int *best_od)
{
	unsigned long best = 0, now = 0;
	unsigned int i, m, n, od;
	int ret;

	for (i = 0, ret = 0; !ret; i++) {
		ret = meson_clk_get_pll_get_index(rate, parent_rate,
						  i, &m, &n, pll, &od);
		if (ret == -EINVAL)
			break;

		now = __pll_params_to_rate(parent_rate, m, n, 0, pll, od);
		if (meson_clk_pll_is_better(rate, best, now, pll)) {
			best = now;
			*best_m = m;
			*best_n = n;
			*best_od = od;

			if (now == rate)
				break;
		}
	}

	return best ? 0 : -EINVAL;
}
#else
static int meson_clk_get_pll_settings(unsigned long rate,
				      unsigned long parent_rate,
				      unsigned int *best_m,
				      unsigned int *best_n,
				      struct meson_clk_pll_data *pll)
{
	unsigned long best = 0, now = 0;
	unsigned int i, m, n;
	int ret;

	for (i = 0, ret = 0; !ret; i++) {
		ret = meson_clk_get_pll_get_index(rate, parent_rate,
						  i, &m, &n, pll);
		if (ret == -EINVAL)
			break;

		now = __pll_params_to_rate(parent_rate, m, n, 0, pll);
		if (meson_clk_pll_is_better(rate, best, now, pll)) {
			best = now;
			*best_m = m;
			*best_n = n;

			if (now == rate)
				break;
		}
	}

	return best ? 0 : -EINVAL;
}
#endif

#if defined CONFIG_AMLOGIC_MODIFY && defined CONFIG_ARM
static long meson_clk_pll_round_rate(struct clk_hw *hw, unsigned long rate,
				     unsigned long *parent_rate)
{
	struct clk_regmap *clk = to_clk_regmap(hw);
	struct meson_clk_pll_data *pll = meson_clk_pll_data(clk);
	unsigned int m, n, frac, od;
	unsigned long round;
	int ret;

	ret = meson_clk_get_pll_settings(rate, *parent_rate, &m, &n, pll, &od);
	if (ret)
		return meson_clk_pll_recalc_rate(hw, *parent_rate);

	round = __pll_params_to_rate(*parent_rate, m, n, 0, pll, od);

	if (!MESON_PARM_APPLICABLE(&pll->frac) || rate == round)
		return round;

	/*
	 * The rate provided by the setting is not an exact match, let's
	 * try to improve the result using the fractional parameter
	 */
	frac = __pll_params_with_frac(rate, *parent_rate, m, n, od, pll);

	return __pll_params_to_rate(*parent_rate, m, n, frac, pll, od);
}
#else
static long meson_clk_pll_round_rate(struct clk_hw *hw, unsigned long rate,
				     unsigned long *parent_rate)
{
	struct clk_regmap *clk = to_clk_regmap(hw);
	struct meson_clk_pll_data *pll = meson_clk_pll_data(clk);
	unsigned int m, n, frac;
	unsigned long round;
	int ret;

	ret = meson_clk_get_pll_settings(rate, *parent_rate, &m, &n, pll);
	if (ret)
		return meson_clk_pll_recalc_rate(hw, *parent_rate);

	round = __pll_params_to_rate(*parent_rate, m, n, 0, pll);

	if (!MESON_PARM_APPLICABLE(&pll->frac) || rate == round)
		return round;

	/*
	 * The rate provided by the setting is not an exact match, let's
	 * try to improve the result using the fractional parameter
	 */
	frac = __pll_params_with_frac(rate, *parent_rate, m, n, pll);

	return __pll_params_to_rate(*parent_rate, m, n, frac, pll);
}
#endif

static int meson_clk_pll_wait_lock(struct clk_hw *hw)
{
	struct clk_regmap *clk = to_clk_regmap(hw);
	struct meson_clk_pll_data *pll = meson_clk_pll_data(clk);
#ifdef CONFIG_AMLOGIC_MODIFY
	int delay = 1000;

	do {
		/* Is the clock locked now ? */
		if (meson_parm_read(clk->map, &pll->l))
			return 0;
		udelay(1);
	} while (delay--);
#endif

	return -ETIMEDOUT;
}

static int meson_clk_pll_init(struct clk_hw *hw)
{
	struct clk_regmap *clk = to_clk_regmap(hw);
	struct meson_clk_pll_data *pll = meson_clk_pll_data(clk);

	/*
	 * Do not init pll
	 * 1. it will gate pll which is needed in RTOS
	 * 2. it will gate sys pll who is feeding CPU
	 */
	if (pll->flags & CLK_MESON_PLL_IGNORE_INIT) {
		pr_warn("ignore %s clock init\n", clk_hw_get_name(hw));
		return 0;
	}

	if (pll->init_count) {
		meson_parm_write(clk->map, &pll->rst, 1);
		regmap_multi_reg_write(clk->map, pll->init_regs,
				       pll->init_count);
		meson_parm_write(clk->map, &pll->rst, 0);
	}

	return 0;
}

static int meson_clk_pll_is_enabled(struct clk_hw *hw)
{
	struct clk_regmap *clk = to_clk_regmap(hw);
	struct meson_clk_pll_data *pll = meson_clk_pll_data(clk);

	/* Enable and lock bit equal 1, it locks */
	if (meson_parm_read(clk->map, &pll->en) &&
	    meson_parm_read(clk->map, &pll->l))
		return 1;

	return 0;
}

static int meson_clk_pcie_pll_enable(struct clk_hw *hw)
{
	struct clk_regmap *clk = to_clk_regmap(hw);
	struct meson_clk_pll_data *pll = meson_clk_pll_data(clk);
	int retry = 10, ret = 10;

	do {
		meson_clk_pll_init(hw);
		if (!meson_clk_pll_wait_lock(hw)) {
			if (!MESON_PARM_APPLICABLE(&pll->pcie_hcsl))
				return 0;
			break;
		}
		pr_info("%s:%d retry = %d\n", __func__, __LINE__, retry);
	} while (retry--);

	if (retry <= 0)
		return -EIO;

	/*pcie pll clk share use for usb phy, so add this operation from ASIC*/
	do {
		if (meson_parm_read(clk->map, &pll->pcie_hcsl)) {
			meson_parm_write(clk->map, &pll->pcie_exen, 0);
			return 0;
		}
		udelay(1);
	} while (ret--);

	if (ret <= 0)
		pr_info("%s:%d pcie reg1 clear bit29 failed\n", __func__, __LINE__);

	return -EIO;
}

static int meson_clk_pll_enable(struct clk_hw *hw)
{
	struct clk_regmap *clk = to_clk_regmap(hw);
	struct meson_clk_pll_data *pll = meson_clk_pll_data(clk);

	/* do nothing if the PLL is already enabled */
	if (clk_hw_is_enabled(hw))
		return 0;

	/* Make sure the pll is in reset */
	meson_parm_write(clk->map, &pll->rst, 1);

	/* Enable the pll */
	meson_parm_write(clk->map, &pll->en, 1);
	/*
	 * Make the PLL more stable, if not,
	 * It will probably lock failed (GP0 PLL)
	 */
#ifdef CONFIG_AMLOGIC_MODIFY
	udelay(50);
#endif

	/* Take the pll out reset */
	meson_parm_write(clk->map, &pll->rst, 0);

	if (meson_clk_pll_wait_lock(hw))
		return -EIO;

	return 0;
}

static void meson_clk_pll_disable(struct clk_hw *hw)
{
	struct clk_regmap *clk = to_clk_regmap(hw);
	struct meson_clk_pll_data *pll = meson_clk_pll_data(clk);

	/* Put the pll is in reset */
	meson_parm_write(clk->map, &pll->rst, 1);

	/* Disable the pll */
	meson_parm_write(clk->map, &pll->en, 0);
}

static int meson_clk_pll_set_rate(struct clk_hw *hw, unsigned long rate,
				  unsigned long parent_rate)
{
	struct clk_regmap *clk = to_clk_regmap(hw);
	struct meson_clk_pll_data *pll = meson_clk_pll_data(clk);
	unsigned int enabled, m, n, frac = 0;
	unsigned long old_rate;
	int ret;
#if defined CONFIG_AMLOGIC_MODIFY && defined CONFIG_ARM
	unsigned int od;
#endif

	if (parent_rate == 0 || rate == 0)
		return -EINVAL;

	old_rate = clk_hw_get_rate(hw);

#if defined CONFIG_AMLOGIC_MODIFY && defined CONFIG_ARM
	ret = meson_clk_get_pll_settings(rate, parent_rate, &m, &n, pll, &od);
#else
	ret = meson_clk_get_pll_settings(rate, parent_rate, &m, &n, pll);
#endif
	if (ret)
		return ret;

	enabled = meson_parm_read(clk->map, &pll->en);
#ifdef CONFIG_AMLOGIC_MODIFY
	/* Don't disable pll if it's just changing frac */
	if ((meson_parm_read(clk->map, &pll->m) != m ||
	     meson_parm_read(clk->map, &pll->n) != n) && enabled)
		meson_clk_pll_disable(hw);
#endif

	meson_parm_write(clk->map, &pll->n, n);
	meson_parm_write(clk->map, &pll->m, m);
#if defined CONFIG_AMLOGIC_MODIFY && defined CONFIG_ARM
	meson_parm_write(clk->map, &pll->od, od);
#endif

	if (MESON_PARM_APPLICABLE(&pll->frac)) {
#if defined CONFIG_AMLOGIC_MODIFY && defined CONFIG_ARM
		frac = __pll_params_with_frac(rate, parent_rate, m, n, od, pll);
#else
		frac = __pll_params_with_frac(rate, parent_rate, m, n, pll);
#endif
		meson_parm_write(clk->map, &pll->frac, frac);
	}

	/*
	 * The PLL should set together required by the
	 * PLL sequence.
	 * This scenes will cause PLL lock failed
	 *  clk_set_rate(pll);
	 *  wait for a long time, several seconds
	 *  clk_prepare_enable(pll);
	 */
	/* If the pll is stopped, bail out now */
#ifndef CONFIG_AMLOGIC_MODIFY
	if (!enabled)
		return 0;
#endif

	ret = meson_clk_pll_enable(hw);
	if (ret) {
		pr_warn("%s: pll did not lock, trying to restore old rate %lu\n",
			__func__, old_rate);
		/*
		 * FIXME: Do we really need/want this HACK ?
		 * It looks unsafe. what happens if the clock gets into a
		 * broken state and we can't lock back on the old_rate ? Looks
		 * like an infinite recursion is possible
		 */
		meson_clk_pll_set_rate(hw, old_rate, parent_rate);
	}

	return ret;
}

/*
 * The Meson G12A PCIE PLL is fined tuned to deliver a very precise
 * 100MHz reference clock for the PCIe Analog PHY, and thus requires
 * a strict register sequence to enable the PLL.
 * To simplify, re-use the _init() op to enable the PLL and keep
 * the other ops except set_rate since the rate is fixed.
 */
const struct clk_ops meson_clk_pcie_pll_ops = {
	.recalc_rate	= meson_clk_pll_recalc_rate,
	.round_rate	= meson_clk_pll_round_rate,
	.is_enabled	= meson_clk_pll_is_enabled,
	.enable		= meson_clk_pcie_pll_enable,
	.disable	= meson_clk_pll_disable
};
EXPORT_SYMBOL_GPL(meson_clk_pcie_pll_ops);

const struct clk_ops meson_clk_pll_ops = {
	.init		= meson_clk_pll_init,
	.recalc_rate	= meson_clk_pll_recalc_rate,
	.round_rate	= meson_clk_pll_round_rate,
	.set_rate	= meson_clk_pll_set_rate,
	.is_enabled	= meson_clk_pll_is_enabled,
	.enable		= meson_clk_pll_enable,
	.disable	= meson_clk_pll_disable
};
EXPORT_SYMBOL_GPL(meson_clk_pll_ops);

#ifdef CONFIG_AMLOGIC_MODIFY
static void meson_secure_pll_v2_disable(struct clk_hw *hw)
{
	struct clk_regmap *clk = to_clk_regmap(hw);
	struct meson_clk_pll_data *pll = meson_clk_pll_data(clk);
	struct arm_smccc_res res;

	arm_smccc_smc(pll->smc_id, pll->secid_disable,
		      0, 0, 0, 0, 0, 0, &res);
}

static int meson_secure_pll_v2_set_rate(struct clk_hw *hw, unsigned long rate,
					unsigned long parent_rate)
{
	struct clk_regmap *clk = to_clk_regmap(hw);
	struct meson_clk_pll_data *pll = meson_clk_pll_data(clk);
	struct arm_smccc_res res;
	unsigned int enabled, m, n, ret = 0;
	unsigned long old_rate;
#if defined CONFIG_AMLOGIC_MODIFY && defined CONFIG_ARM
	unsigned int od;
#endif

	if (parent_rate == 0 || rate == 0)
		return -EINVAL;

	old_rate = rate;

#if defined CONFIG_AMLOGIC_MODIFY && defined CONFIG_ARM
	ret = meson_clk_get_pll_settings(rate, parent_rate, &m, &n, pll, &od);
#else
	ret = meson_clk_get_pll_settings(rate, parent_rate, &m, &n, pll);
#endif
	if (ret)
		return ret;

	enabled = meson_parm_read(clk->map, &pll->en);
	if (enabled)
		meson_secure_pll_v2_disable(hw);
	/*Send m,n for arm64 */
#if defined CONFIG_AMLOGIC_MODIFY && defined CONFIG_ARM
	arm_smccc_smc(pll->smc_id, pll->secid,
		      m, n, od, 0, 0, 0, &res);
#else
	arm_smccc_smc(pll->smc_id, pll->secid,
		      m, n, 0, 0, 0, 0, &res);
#endif

	return 0;
}

static int meson_secure_pll_v2_enable(struct clk_hw *hw)
{
	struct clk_regmap *clk = to_clk_regmap(hw);
	struct meson_clk_pll_data *pll = meson_clk_pll_data(clk);
	unsigned int m, n, od;
	struct arm_smccc_res res;

	/*
	 * In most case,  do nothing if the PLL is already enabled
	 */
	if (clk_hw_is_enabled(hw))
		return 0;

	/* If PLL is not enabled because setting the same rate,
	 * Enable it again, CCF will return when set the same rate
	 */

	n = meson_parm_read(clk->map, &pll->n);
	m = meson_parm_read(clk->map, &pll->m);
	od = meson_parm_read(clk->map, &pll->od);

	arm_smccc_smc(pll->smc_id, pll->secid,
		      m, n, od, 0, 0, 0, &res);

	return 0;
}

const struct clk_ops meson_secure_pll_v2_ops = {
	.recalc_rate	= meson_clk_pll_recalc_rate,
	.round_rate	= meson_clk_pll_round_rate,
	.set_rate	= meson_secure_pll_v2_set_rate,
	.is_enabled	= meson_clk_pll_is_enabled,
	.enable		= meson_secure_pll_v2_enable,
	.disable	= meson_secure_pll_v2_disable
};
EXPORT_SYMBOL_GPL(meson_secure_pll_v2_ops);

static int meson_clk_pll_v3_set_rate(struct clk_hw *hw, unsigned long rate,
				  unsigned long parent_rate)
{
	struct clk_regmap *clk = to_clk_regmap(hw);
	struct meson_clk_pll_data *pll = meson_clk_pll_data(clk);
	struct parm *pm = &pll->m;
	struct parm *pn = &pll->n;
	struct parm *pfrac = &pll->frac;
	unsigned int enabled, m, n, frac;
	unsigned long old_rate;
	unsigned int val;
	const struct reg_sequence *init_regs = pll->init_regs;
	int i, ret = 0, retry = 10;
#if defined CONFIG_AMLOGIC_MODIFY && defined CONFIG_ARM
	unsigned int od;
	struct parm *pod = &pll->od;
#endif

	if (parent_rate == 0 || rate == 0)
		return -EINVAL;

	old_rate = rate;

	/* calculate M, N, OD*/
#if defined CONFIG_AMLOGIC_MODIFY && defined CONFIG_ARM
	ret = meson_clk_get_pll_settings(rate, parent_rate, &m, &n, pll, &od);
#else
	ret = meson_clk_get_pll_settings(rate, parent_rate, &m, &n, pll);
#endif
	if (ret)
		return ret;

	/* calute frac */
	if (MESON_PARM_APPLICABLE(&pll->frac)) {
#if defined CONFIG_AMLOGIC_MODIFY && defined CONFIG_ARM
		frac = __pll_params_with_frac(rate, parent_rate, m, n, od, pll);
#else
		frac = __pll_params_with_frac(rate, parent_rate, m, n, pll);
#endif
		/*
		 * Can update the register corresponding to frac directly
		 * if just change frac and pll to enable.
		 */
		if (meson_parm_read(clk->map, &pll->m) == m &&
		    meson_parm_read(clk->map, &pll->n) == n &&
		    meson_parm_read(clk->map, &pll->en)) {
			regmap_read(clk->map, pfrac->reg_off, &val);
			/* Clear Frac bits and Update frac value */
			val &= CLRPMASK(pfrac->width, pfrac->shift);
			val |= frac << pfrac->shift;
			regmap_write(clk->map, pfrac->reg_off, val);

			if (!meson_clk_pll_wait_lock(hw))
				return 0;
		}
	}

	enabled = meson_parm_read(clk->map, &pll->en);
	if (enabled)
		meson_clk_pll_disable(hw);

	do {
		for (i = 0; i < pll->init_count; i++) {
			if (pn->reg_off == init_regs[i].reg) {
				/* Clear M N bits and Update M N value */
				val = init_regs[i].def;
				val &= CLRPMASK(pn->width, pn->shift);
				val &= CLRPMASK(pm->width, pm->shift);
				val |= n << pn->shift;
				val |= m << pm->shift;
#if defined CONFIG_AMLOGIC_MODIFY && defined CONFIG_ARM
				val &= CLRPMASK(pod->width, pod->shift);
				val |= od << pod->shift;
#endif
				regmap_write(clk->map, pn->reg_off, val);
			} else if (pfrac->reg_off == init_regs[i].reg &&
				(MESON_PARM_APPLICABLE(&pll->frac))) {
				/* Clear Frac bits and Update Frac value */
				val = init_regs[i].def;
				val &= CLRPMASK(pfrac->width, pfrac->shift);
				val |= frac << pfrac->shift;
				regmap_write(clk->map, pfrac->reg_off, val);
			} else {
				val = init_regs[i].def;
				regmap_write(clk->map, init_regs[i].reg, val);
			}
			if (init_regs[i].delay_us)
				udelay(init_regs[i].delay_us);
		}

		if (!meson_clk_pll_wait_lock(hw))
			break;
		retry--;
	} while (retry > 0);

	if (retry == 0)
		pr_err("%s pll locked failed\n", clk_hw_get_name(hw));

	return 0;
}

static int meson_clk_pll_v3_enable(struct clk_hw *hw)
{
	unsigned long rate, parent_rate;

	/* do nothing if the PLL is already enabled */
	if (clk_hw_is_enabled(hw))
		return 0;

	/* Deal clk_set_rate return when set the same rate */
	parent_rate = clk_hw_get_rate(clk_hw_get_parent(hw));
	rate = meson_clk_pll_recalc_rate(hw, parent_rate);
	meson_clk_pll_v3_set_rate(hw, rate, parent_rate);

	if (meson_clk_pll_wait_lock(hw))
		return -EIO;

	return 0;
}

const struct clk_ops meson_clk_pll_v3_ops = {
	.init		= meson_clk_pll_init,
	.recalc_rate	= meson_clk_pll_recalc_rate,
	.round_rate	= meson_clk_pll_round_rate,
	.set_rate	= meson_clk_pll_v3_set_rate,
	.is_enabled	= meson_clk_pll_is_enabled,
	.enable		= meson_clk_pll_v3_enable,
	.disable	= meson_clk_pll_disable
};
EXPORT_SYMBOL_GPL(meson_clk_pll_v3_ops);
#endif

const struct clk_ops meson_clk_pll_ro_ops = {
	.recalc_rate	= meson_clk_pll_recalc_rate,
	.is_enabled	= meson_clk_pll_is_enabled,
};
EXPORT_SYMBOL_GPL(meson_clk_pll_ro_ops);

MODULE_DESCRIPTION("Amlogic PLL driver");
MODULE_AUTHOR("Carlo Caione <carlo@endlessm.com>");
MODULE_AUTHOR("Jerome Brunet <jbrunet@baylibre.com>");
MODULE_LICENSE("GPL v2");
