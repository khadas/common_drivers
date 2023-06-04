// SPDX-License-Identifier: (GPL-2.0+ or MIT)
/*
 * Second generation of pinmux driver for Amlogic Meson-AXG SoC.
 *
 * Copyright (c) 2017 Baylibre SAS.
 * Author:  Jerome Brunet  <jbrunet@baylibre.com>
 *
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 * Author: Xingyu Chen <xingyu.chen@amlogic.com>
 */

/*
 * This new generation of pinctrl IP is mainly adopted by the
 * Meson-AXG SoC and later series, which use 4-width continuous
 * register bit to select the function for each pin.
 *
 * The value 0 is always selecting the GPIO mode, while other
 * values (start from 1) for selecting the function mode.
 */
#include <linux/device.h>
#include <linux/regmap.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinmux.h>

#include "pinctrl-meson.h"
#include "pinctrl-meson-axg-pmx.h"

static int meson_axg_pmx_get_bank(struct meson_pinctrl *pc,
				  unsigned int pin,
				  struct meson_pmx_bank **bank)
{
	int i;
	struct meson_axg_pmx_data *pmx = pc->data->pmx_data;

	for (i = 0; i < pmx->num_pmx_banks; i++)
		if (pin >= pmx->pmx_banks[i].first &&
		    pin <= pmx->pmx_banks[i].last) {
			*bank = &pmx->pmx_banks[i];
			return 0;
		}

	return -EINVAL;
}

static int meson_pmx_calc_reg_and_offset(struct meson_pmx_bank *bank,
					 unsigned int pin, unsigned int *reg,
					 unsigned int *offset)
{
	int shift;

	shift = pin - bank->first;

	*reg = bank->reg + (bank->offset + (shift << 2)) / 32;
	*offset = (bank->offset + (shift << 2)) % 32;

	return 0;
}

#ifdef CONFIG_AMLOGIC_MODIFY
static int meson_pmx_expand_update(struct meson_pinctrl *pc,
				   unsigned int pin,
				   unsigned int func)
{
	struct meson_pinctrl_data *data = pc->data;
	unsigned int pin_idx, reg_idx;

	if (!pc->mux_expand_num || !pc->mux_expand_reg)
		return 0;

	/* find pin */
	for (pin_idx = 0; pin_idx < data->pmx_expand_num; pin_idx++)
		if (data->pmx_expand[pin_idx].pin == pin)
			break;
	if (pin_idx == data->pmx_expand_num)
		return 0;
	dev_dbg(pc->dev, "find pin_idx: %u\n", pin_idx);

	if (data->pmx_expand[pin_idx].func != func)
		return 0;

	/* find reg */
	for (reg_idx = 0; reg_idx < pc->mux_expand_num; reg_idx++)
		if (!strcmp(pc->mux_expand_reg[reg_idx].name,
			    data->pmx_expand[pin_idx].reg_name))
			break;
	if (reg_idx == pc->mux_expand_num)
		return 0;
	dev_dbg(pc->dev, "find reg_idx: %u\n", reg_idx);

	/* update bits */
	writel((readl(pc->mux_expand_reg[reg_idx].reg) &
	       ~data->pmx_expand[pin_idx].mask) |
	       data->pmx_expand[pin_idx].value,
	       pc->mux_expand_reg[reg_idx].reg);

	dev_dbg(pc->dev, "update reg:%p,mask:%08x,val:%08x\n",
		pc->mux_expand_reg[reg_idx].reg,
		~data->pmx_expand[pin_idx].mask,
		data->pmx_expand[pin_idx].value);

	return 0;
}
#endif

static int meson_axg_pmx_update_function(struct meson_pinctrl *pc,
					 unsigned int pin,
					 unsigned int func)
{
	int ret;
	int reg;
	int offset;
	struct meson_pmx_bank *bank;

	ret = meson_axg_pmx_get_bank(pc, pin, &bank);
	if (ret)
		return ret;

	meson_pmx_calc_reg_and_offset(bank, pin, &reg, &offset);

	ret = regmap_update_bits(pc->reg_mux, reg << 2,
				 0xf << offset, (func & 0xf) << offset);
#ifdef CONFIG_AMLOGIC_MODIFY
	if (!ret)
		ret = meson_pmx_expand_update(pc, pin, func);
#endif

	return ret;
}

static int meson_axg_pmx_set_mux(struct pinctrl_dev *pcdev,
				 unsigned int func_num,
				 unsigned int group_num)
{
	int i;
	int ret;
	struct meson_pinctrl *pc = pinctrl_dev_get_drvdata(pcdev);
	struct meson_pmx_func *func = &pc->data->funcs[func_num];
	struct meson_pmx_group *group = &pc->data->groups[group_num];
	struct meson_pmx_axg_data *pmx_data =
		(struct meson_pmx_axg_data *)group->data;

	dev_dbg(pc->dev, "enable function %s, group %s\n", func->name,
		group->name);

	for (i = 0; i < group->num_pins; i++) {
		ret = meson_axg_pmx_update_function(pc, group->pins[i],
						    pmx_data->func);
		if (ret)
			return ret;
	}

	return 0;
}

static int meson_axg_pmx_request_gpio(struct pinctrl_dev *pcdev,
				      struct pinctrl_gpio_range *range,
				      unsigned int offset)
{
	struct meson_pinctrl *pc = pinctrl_dev_get_drvdata(pcdev);

	return meson_axg_pmx_update_function(pc, offset, 0);
}

const struct pinmux_ops meson_axg_pmx_ops = {
	.set_mux = meson_axg_pmx_set_mux,
	.get_functions_count = meson_pmx_get_funcs_count,
	.get_function_name = meson_pmx_get_func_name,
	.get_function_groups = meson_pmx_get_groups,
	.gpio_request_enable = meson_axg_pmx_request_gpio,
#ifdef CONFIG_AMLOGIC_MODIFY
	.strict = true,
#endif
};
EXPORT_SYMBOL(meson_axg_pmx_ops);
