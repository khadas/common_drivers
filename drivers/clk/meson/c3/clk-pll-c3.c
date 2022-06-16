// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
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
#include <linux/of_address.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "clk-pll.h"
#include "clk-regmap.h"
#ifdef CONFIG_AMLOGIC_MODIFY
#include <linux/arm-smccc.h>
#endif

static int meson_c3_clk_pll_set_rate(struct clk_hw *hw, unsigned long rate,
				  unsigned long parent_rate);

static inline struct meson_clk_pll_data *
meson_clk_pll_data(struct clk_regmap *clk)
{
	return (struct meson_clk_pll_data *)clk->data;
}

static unsigned long __pll_params_to_rate(unsigned long parent_rate,
					  const struct pll_params_table *pllt,
					  u32 frac,
					  struct meson_clk_pll_data *pll)
{
	u64 rate = (u64)parent_rate * pllt->m;
	u64 result;

	if (frac && MESON_PARM_APPLICABLE(&pll->frac) &&
	    pll->frac.width > 2) {
		u64 frac_rate = (u64)parent_rate * frac;

		/* frac equal Positive */
		if (!((frac >> (pll->frac.width - 1)) & 0x1))
			rate += DIV_ROUND_UP_ULL(frac_rate,
					1 << (pll->frac.width - 2));
		else  /* frac equal Negative */
			rate -= DIV_ROUND_UP_ULL(frac_rate,
					1 << (pll->frac.width - 2));
	}

	if (pllt->n == 0)
		result = rate;
	else
		result = DIV_ROUND_UP_ULL(rate, pllt->n);

#ifdef CONFIG_ARM
	result = result >> pllt->od;
#endif  /* CONFIG_ARM */

	return result;
}

static unsigned long meson_c3_clk_pll_recalc_rate(struct clk_hw *hw,
					       unsigned long parent_rate)
{
	struct clk_regmap *clk = to_clk_regmap(hw);
	struct meson_clk_pll_data *pll = meson_clk_pll_data(clk);
	struct pll_params_table pllt;
	u32 frac;

	pllt.n = meson_parm_read(clk->map, &pll->n);
	pllt.m = meson_parm_read(clk->map, &pll->m);
#ifdef CONFIG_ARM
	pllt.od = meson_parm_read(clk->map, &pll->od);
#endif  /* CONFIG_ARM */

	frac = MESON_PARM_APPLICABLE(&pll->frac) ?
		meson_parm_read(clk->map, &pll->frac) :
		0;
	return __pll_params_to_rate(parent_rate, &pllt, frac, pll);
}

static u32 __pll_params_with_frac(unsigned long rate,
				  unsigned long parent_rate,
				  const struct pll_params_table *pllt,
				  struct meson_clk_pll_data *pll)
{
	u32 frac_max = (1 << (pll->frac.width - 2));
	u64 val = (u64)rate * (pllt->n ? pllt->n : 1);

#ifdef CONFIG_ARM
	/* od for 32bit */
	val = val * (1 << pllt->od);
#endif  /* CONFIG_ARM */

	if (pll->flags & CLK_MESON_PLL_ROUND_CLOSEST)
		val = DIV_ROUND_CLOSEST_ULL(val * frac_max, parent_rate);
	else
		val = div_u64(val * frac_max, parent_rate);

	val -= pllt->m * frac_max;

	return min((u32)val, (frac_max - 1));
}

static bool meson_clk_pll_is_better(unsigned long rate,
				    unsigned long best,
				    unsigned long now,
				    struct meson_clk_pll_data *pll)
{
	if (!(pll->flags & CLK_MESON_PLL_ROUND_CLOSEST) ||
	    MESON_PARM_APPLICABLE(&pll->frac)) {
		/* Round down */
		if (now < rate && best < now)
			return true;
	} else {
		/* Round Closest */
		if (abs(now - rate) < abs(best - rate))
			return true;
	}

	return false;
}

static const struct pll_params_table *
meson_clk_get_pll_settings(unsigned long rate,
			   unsigned long parent_rate,
			   struct meson_clk_pll_data *pll)
{
	const struct pll_params_table *table = pll->table;
	unsigned long best = 0, now = 0;
	unsigned int i, best_i = 0;

	if (!table)
		return NULL;

	for (i = 0; table[i].n; i++) {
		now = __pll_params_to_rate(parent_rate, &table[i], 0, pll);

		/* If we get an exact match, don't bother any further */
		if (now == rate) {
			return &table[i];
		} else if (meson_clk_pll_is_better(rate, best, now, pll)) {
			best = now;
			best_i = i;
		}
	}

	return (struct pll_params_table *)&table[best_i];
}

static long meson_c3_clk_pll_round_rate(struct clk_hw *hw, unsigned long rate,
				     unsigned long *parent_rate)
{
	struct clk_regmap *clk = to_clk_regmap(hw);
	struct meson_clk_pll_data *pll = meson_clk_pll_data(clk);
	const struct pll_params_table *pllt =
		meson_clk_get_pll_settings(rate, *parent_rate, pll);
	unsigned long round;
	u32 frac;

	if (!pllt)
		return meson_c3_clk_pll_recalc_rate(hw, *parent_rate);

	round = __pll_params_to_rate(*parent_rate, pllt, 0, pll);

	if (!MESON_PARM_APPLICABLE(&pll->frac) || rate == round)
		return round;

	/*
	 * The rate provided by the setting is not an exact match, let's
	 * try to improve the result using the fractional parameter
	 */
	frac = __pll_params_with_frac(rate, *parent_rate, pllt, pll);

	return __pll_params_to_rate(*parent_rate, pllt, frac, pll);
}

static int meson_clk_pll_wait_lock(struct clk_hw *hw)
{
	struct clk_regmap *clk = to_clk_regmap(hw);
	struct meson_clk_pll_data *pll = meson_clk_pll_data(clk);
	int delay = 1000;

	do {
		/* Is the clock locked now ? */
		if (meson_parm_read(clk->map, &pll->l))
			return 0;

		udelay(1);
	} while (delay--);

	return -ETIMEDOUT;
}

static int meson_c3_clk_pll_is_enabled(struct clk_hw *hw)
{
	struct clk_regmap *clk = to_clk_regmap(hw);
	struct meson_clk_pll_data *pll = meson_clk_pll_data(clk);

	if (meson_parm_read(clk->map, &pll->en) &&
	    meson_parm_read(clk->map, &pll->l))
		return 1;

	return 0;
}

static int meson_c3_clk_pll_enable(struct clk_hw *hw)
{
	unsigned long rate;
	struct clk_hw *parent;
	unsigned int ret;

	/* do nothing if the PLL is already enabled */
	if (clk_hw_is_enabled(hw))
		return 0;

	/*
	 * clk_set_rate failed to set pll, do it here
	 * when pll is disabled, the rate is the same with the last
	 * rate, when call clk_set_rate, the pll does not work in fact
	 * set again pll using internal functions.
	 */
	parent = clk_hw_get_parent(hw);
	rate = meson_c3_clk_pll_recalc_rate(hw, clk_hw_get_rate(parent));

	ret = meson_c3_clk_pll_set_rate(hw, rate, clk_hw_get_rate(parent));

	if (meson_clk_pll_wait_lock(hw))
		return -EIO;

	return 0;
}

static void meson_c3_clk_pll_disable(struct clk_hw *hw)
{
	struct clk_regmap *clk = to_clk_regmap(hw);
	struct meson_clk_pll_data *pll = meson_clk_pll_data(clk);

	/* Put the pll is in reset */
	meson_parm_write(clk->map, &pll->rst, 1);

	/* Disable the pll */
	meson_parm_write(clk->map, &pll->en, 0);
}

static int meson_c3_clk_pll_set_rate(struct clk_hw *hw, unsigned long rate,
				  unsigned long parent_rate)
{
	struct clk_regmap *clk = to_clk_regmap(hw);
	struct meson_clk_pll_data *pll = meson_clk_pll_data(clk);
	const struct pll_params_table *pllt;
	unsigned int enabled;
	unsigned long old_rate;
	u32 frac_value = 0;
	struct parm *m = &pll->m;
	struct parm *n = &pll->n;
#ifdef CONFIG_ARM
	struct parm *od = &pll->od;
#endif
	struct parm *frac = &pll->frac;
	unsigned int val = 0;
	const struct reg_sequence *init_regs = pll->init_regs;
	int i, cnt = 10;

	if (parent_rate == 0 || rate == 0)
		return -EINVAL;

	old_rate = rate;

	/* calculate M and N */
	pllt = meson_clk_get_pll_settings(rate, parent_rate, pll);
	if (!pllt)
		return -EINVAL;

	/* calute frac */
	if (MESON_PARM_APPLICABLE(&pll->frac))
		frac_value = __pll_params_with_frac(rate,
							parent_rate,
							pllt, pll);

#ifdef CONFIG_ARM
	pr_debug("%s:%s pll_m = %d, pll_frac = %d, pll_n = %d, pll_od = %d\r",
		__func__, clk_hw_get_name(hw), pllt->m, frac_value, pllt->n, pllt->od);
#else
	pr_debug("%s:%s pll_m = %d, pll_frac = %d, pll_n = %d\r",
		__func__, clk_hw_get_name(hw), pllt->m, frac_value, pllt->n);
#endif  /* CONFIG_ARM */

retry:
	enabled = meson_parm_read(clk->map, &pll->en);
	if (enabled)
		meson_c3_clk_pll_disable(hw);

	for (i = 0; i < pll->init_count; i++) {
		if (n->reg_off == init_regs[i].reg) {
			/* Clear M N bits and Update M N value */
			val = init_regs[i].def;
			val &= CLRPMASK(n->width, n->shift);
			val &= CLRPMASK(m->width, m->shift);
			val |= (pllt->n) << n->shift;
			val |= (pllt->m) << m->shift;
#ifdef CONFIG_ARM
			val &= CLRPMASK(od->width, od->shift);
			val |= (pllt->od) << od->shift;
#endif
			regmap_write(clk->map, n->reg_off, val);
		} else if (frac->reg_off == init_regs[i].reg &&
			(MESON_PARM_APPLICABLE(&pll->frac))) {
			/* Clear Frac bits and Update Frac value */
			val = init_regs[i].def;
			val &= CLRPMASK(frac->width, frac->shift);
			val |= frac_value << frac->shift;
			regmap_write(clk->map, frac->reg_off, val);
		} else {
			val = init_regs[i].def;
			regmap_write(clk->map, init_regs[i].reg, val);
		}

		if (init_regs[i].delay_us)
			udelay(init_regs[i].delay_us);
	}

	if (meson_clk_pll_wait_lock(hw)) {
		pr_info("%s:%s pll did not lock, trying to lock rate:%lu\n",
			__func__, clk_hw_get_name(hw), rate);
		if (cnt--)
			goto retry;
	}

	return 0;
}

static int meson_c3_clk_pll_init(struct clk_hw *hw)
{
	struct clk_regmap *clk = to_clk_regmap(hw);
	struct meson_clk_pll_data *pll = meson_clk_pll_data(clk);

	if (pll->init_count) {
		meson_parm_write(clk->map, &pll->rst, 1);
		regmap_multi_reg_write(clk->map, pll->init_regs,
				       pll->init_count);
		meson_parm_write(clk->map, &pll->rst, 0);

		if (meson_clk_pll_wait_lock(hw))
			pr_warn("%s:%s pll did not lock\n",	__func__, clk_hw_get_name(hw));

	}

	return 0;
}

const struct clk_ops meson_c3_clk_pll_ops = {
	.init		= meson_c3_clk_pll_init,
	.recalc_rate	= meson_c3_clk_pll_recalc_rate,
	.round_rate	= meson_c3_clk_pll_round_rate,
	.set_rate	= meson_c3_clk_pll_set_rate,
	.is_enabled	= meson_c3_clk_pll_is_enabled,
	.enable		= meson_c3_clk_pll_enable,
	.disable	= meson_c3_clk_pll_disable,
};

const struct clk_ops meson_c3_clk_pll_ro_ops = {
	.recalc_rate	= meson_c3_clk_pll_recalc_rate,
	.is_enabled	= meson_c3_clk_pll_is_enabled,
};
