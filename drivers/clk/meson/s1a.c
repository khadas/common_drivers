// SPDX-License-Identifier: GPL-2.0+
/*
 * Amlogic Meson-S1A Clock Controller Driver
 *
 * Copyright (c) 2018 Amlogic, inc.
 */

#include <linux/clk-provider.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/module.h>
#include <linux/clkdev.h>
#include <linux/of_address.h>
#include <linux/of_device.h>

#include "clk-pll.h"
#include "clk-regmap.h"
#include "clk-cpu-dyndiv.h"
#include "clk-dualdiv.h"
#include "s1a.h"
#include <dt-bindings/clock/amlogic,s1a-clkc.h>

#define __MESON_CLK_MUX(_name, _reg, _mask, _shift, _table,		\
			_smcid, _secid, _secid_rd, _dflags,		\
			_ops, _pname, _pdata, _phw, _pnub, _iflags)	\
static struct clk_regmap _name = {					\
	.data = &(struct clk_regmap_mux_data){				\
		.offset = _reg,						\
		.mask = _mask,						\
		.shift = _shift,					\
		.table = _table,					\
		.smc_id = _smcid,					\
		.secid = _secid,					\
		.secid_rd = _secid_rd,					\
		.flags = _dflags,					\
	},								\
	.hw.init = &(struct clk_init_data){				\
		.name = # _name,					\
		.ops = _ops,						\
		.parent_names = _pname,					\
		.parent_data = _pdata,					\
		.parent_hws = _phw,					\
		.num_parents = _pnub,					\
		.flags = _iflags,					\
	},								\
}

#define __MESON_CLK_DIV(_name, _reg, _shift, _width, _table,		\
			_smcid, _secid, _dflags,			\
			_ops, _pname, _pdata, _phw, _iflags)		\
static struct clk_regmap _name = {					\
	.data = &(struct clk_regmap_div_data){				\
		.offset = _reg,						\
		.shift = _shift,					\
		.width = _width,					\
		.table = _table,					\
		.smc_id = _smcid,					\
		.secid = _secid,					\
		.flags = _dflags,					\
	},								\
	.hw.init = &(struct clk_init_data){				\
		.name = # _name,					\
		.ops = _ops,						\
		.parent_names = _pname,					\
		.parent_data = _pdata,					\
		.parent_hws = _phw,					\
		.num_parents = 1,					\
		.flags = _iflags,					\
	},								\
}

#define __MESON_CLK_GATE(_name, _reg, _bit, _dflags,			\
			 _ops, _pname, _pdata, _phw, _iflags)		\
static struct clk_regmap _name = {					\
	.data = &(struct clk_regmap_gate_data){				\
		.offset = _reg,						\
		.bit_idx = _bit,					\
		.flags = _dflags,					\
	},								\
	.hw.init = &(struct clk_init_data) {				\
		.name = # _name,					\
		.ops = _ops,						\
		.parent_names = _pname,					\
		.parent_data = _pdata,					\
		.parent_hws = _phw,					\
		.num_parents = 1,					\
		.flags = _iflags,					\
	},								\
}

#define MEMBER_REG_PARM(_member_name, _reg, _shift, _width)		\
	._member_name = {						\
		.reg_off = _reg,					\
		.shift   = _shift,					\
		.width   = _width,					\
	}

#define __MESON_CLK_DUALDIV(_name, _n1_reg, _n1_shift, _n1_width,	\
			    _n2_reg, _n2_shift, _n2_width,		\
			    _m1_reg, _m1_shift, _m1_width,		\
			    _m2_reg, _m2_shift, _m2_width,		\
			    _d_reg, _d_shift, _d_width, _dualdiv_table,	\
			    _ops, _pname, _pdata, _phw, _iflags)	\
static struct clk_regmap _name = {					\
	.data = &(struct meson_clk_dualdiv_data){			\
		MEMBER_REG_PARM(n1,					\
			_n1_reg, _n1_shift, _n1_width),			\
		MEMBER_REG_PARM(n2,					\
			_n2_reg, _n2_shift, _n2_width),			\
		MEMBER_REG_PARM(m1,					\
			_m1_reg, _m1_shift, _m1_width),			\
		MEMBER_REG_PARM(m2,					\
			_m2_reg, _m2_shift, _m2_width),			\
		MEMBER_REG_PARM(dual,					\
			_d_reg, _d_shift, _d_width),			\
		.table = _dualdiv_table,				\
	},								\
	.hw.init = &(struct clk_init_data) {				\
		.name = # _name,					\
		.ops = _ops,						\
		.parent_names = _pname,					\
		.parent_data = _pdata,					\
		.parent_hws = _phw,					\
		.num_parents = 1,					\
		.flags = _iflags,					\
	},								\
}

#define __MESON_CLK_CPU_DYN_SEC(_name, _smcid, _secid_dyn,		\
				_secid_dyn_rd, _table, _table_cnt,	\
				_ops, _pname, _pdata, _phw, _pnub,	\
				_iflags)				\
static struct clk_regmap _name = {					\
	.data = &(struct meson_clk_cpu_dyn_data){			\
		.table = _table,					\
		.table_cnt = _table_cnt,				\
		.smc_id = _smcid,					\
		.secid_dyn_rd = _secid_dyn_rd,				\
		.secid_dyn = _secid_dyn,				\
	},								\
	.hw.init = &(struct clk_init_data) {				\
		.name = # _name,					\
		.ops = _ops,						\
		.parent_names = _pname,					\
		.parent_data = _pdata,					\
		.parent_hws = _phw,					\
		.num_parents = _pnub,					\
		.flags = _iflags,					\
	},								\
}

#define __MESON_CLK_FIXED_FACTOR(_name, _mult, _div, _ops,		\
				 _pname, _pdata, _phw, _iflags)		\
static struct clk_fixed_factor _name = {				\
	.mult = _mult,							\
	.div = _div,							\
	.hw.init = &(struct clk_init_data){				\
		.name = # _name,					\
		.ops = _ops,						\
		.parent_names = _pname,					\
		.parent_data = _pdata,					\
		.parent_hws = _phw,					\
		.num_parents = 1,					\
		.flags = _iflags,					\
	},								\
}

#ifdef CONFIG_ARM
#define __MESON_CLK_PLL(_name, _en_reg, _en_shift, _en_width,		\
			_m_reg, _m_shift, _m_width,			\
			_f_reg, _f_shift, _f_width,			\
			_n_reg, _n_shift, _n_width,			\
			_l_reg, _l_shift, _l_width,			\
			_r_reg, _r_shift, _r_width,			\
			_th_reg, _th_shift, _th_width,			\
			_init_reg, _init_reg_cnt, _range, _table,	\
			_smcid, _secid, _secid_dis, _dflags,		\
			_ops, _pname, _pdata, _phw, _iflags,		\
			_od_reg, _od_shift, _od_width)			\
static struct clk_regmap _name = {					\
	.data = &(struct meson_clk_pll_data){				\
		MEMBER_REG_PARM(en, _en_reg, _en_shift, _en_width),	\
		MEMBER_REG_PARM(m, _m_reg, _m_shift, _m_width),		\
		MEMBER_REG_PARM(frac, _f_reg, _f_shift, _f_width),	\
		MEMBER_REG_PARM(n, _n_reg, _n_shift, _n_width),		\
		MEMBER_REG_PARM(l, _l_reg, _l_shift, _l_width),		\
		MEMBER_REG_PARM(rst, _r_reg, _r_shift, _r_width),	\
		MEMBER_REG_PARM(th, _th_reg, _th_shift, _th_width),	\
		MEMBER_REG_PARM(od, _od_reg, _od_shift, _od_width),	\
		.range = _range,					\
		.table = _table,					\
		.init_regs = _init_reg,					\
		.init_count = _init_reg_cnt,				\
		.smc_id = _smcid,					\
		.secid = _secid,					\
		.secid_disable = _secid_dis,				\
		.flags = _dflags,					\
	},								\
	.hw.init = &(struct clk_init_data){				\
		.name = # _name,					\
		.ops = _ops,						\
		.parent_names = _pname,					\
		.parent_data = _pdata,					\
		.parent_hws = _phw,					\
		.num_parents = 1,					\
		.flags = _iflags,					\
	},								\
}
#else
#define __MESON_CLK_PLL(_name, _en_reg, _en_shift, _en_width,		\
			_m_reg, _m_shift, _m_width,			\
			_f_reg, _f_shift, _f_width,			\
			_n_reg, _n_shift, _n_width,			\
			_l_reg, _l_shift, _l_width,			\
			_r_reg, _r_shift, _r_width,			\
			_th_reg, _th_shift, _th_width,			\
			_init_reg, _init_reg_cnt, _range, _table,	\
			_smcid, _secid, _secid_dis, _dflags,		\
			_ops, _pname, _pdata, _phw, _iflags,		\
			_od_name, _od_reg, _od_shift, _od_width,	\
			_od_table, _od_smcid, _od_secid, _od_dflags,	\
			_od_ops, _od_iflags)				\
static struct clk_regmap _name = {					\
	.data = &(struct meson_clk_pll_data){				\
		MEMBER_REG_PARM(en, _en_reg, _en_shift, _en_width),	\
		MEMBER_REG_PARM(m, _m_reg, _m_shift, _m_width),		\
		MEMBER_REG_PARM(frac, _f_reg, _f_shift, _f_width),	\
		MEMBER_REG_PARM(n, _n_reg, _n_shift, _n_width),		\
		MEMBER_REG_PARM(l, _l_reg, _l_shift, _l_width),		\
		MEMBER_REG_PARM(rst, _r_reg, _r_shift, _r_width),	\
		MEMBER_REG_PARM(th, _th_reg, _th_shift, _th_width),	\
		MEMBER_REG_PARM(od, _od_reg, _od_shift, _od_width),	\
		.range = _range,					\
		.table = _table,					\
		.init_regs = _init_reg,					\
		.init_count = _init_reg_cnt,				\
		.smc_id = _smcid,					\
		.secid = _secid,					\
		.secid_disable = _secid_dis,				\
		.flags = _dflags,					\
	},								\
	.hw.init = &(struct clk_init_data){				\
		.name = # _name,					\
		.ops = _ops,						\
		.parent_names = _pname,					\
		.parent_data = _pdata,					\
		.parent_hws = _phw,					\
		.num_parents = 1,					\
		.flags = _iflags,					\
	},								\
};									\
static struct clk_regmap _od_name = {					\
	.data = &(struct clk_regmap_div_data){				\
		.offset = _od_reg,					\
		.shift = _od_shift,					\
		.width = _od_width,					\
		.table = _od_table,					\
		.smc_id = _od_smcid,					\
		.secid = _od_secid,					\
		.flags = _od_dflags,					\
	},								\
	.hw.init = &(struct clk_init_data){				\
		.name = # _od_name,					\
		.ops = _od_ops,						\
		.parent_hws = (const struct clk_hw *[]) {		\
			&_name.hw },					\
		.num_parents = 1,					\
		.flags = _od_iflags,					\
	},								\
}
#endif

#define MESON_CLK_MUX_RW(_name, _reg, _mask, _shift, _table, _dflags,	\
			 _pdata, _iflags)				\
	__MESON_CLK_MUX(_name, _reg, _mask, _shift, _table,		\
			0, 0, 0, _dflags,				\
			&clk_regmap_mux_ops, NULL, _pdata, NULL,	\
			ARRAY_SIZE(_pdata), _iflags)

#define MESON_CLK_MUX_RO(_name, _reg, _mask, _shift, _table, _dflags,	\
			 _pdata, _iflags)				\
	__MESON_CLK_MUX(_name, _reg, _mask, _shift, _table,		\
			0, 0, 0, CLK_MUX_READ_ONLY | (_dflags),		\
			&clk_regmap_mux_ro_ops, NULL, _pdata, NULL,	\
			ARRAY_SIZE(_pdata), _iflags)

#define MESON_CLK_MUX_SEC(_name, _reg, _mask, _shift, _table,		\
			  _smcid, _secid, _secid_rd, _dflags,		\
			  _pdata, _iflags)				\
	__MESON_CLK_MUX(_name, _reg, _mask, _shift, _table,		\
			_smcid, _secid, _secid_rd, _dflags,		\
			&clk_regmap_mux_ops, NULL, _pdata, NULL,	\
			ARRAY_SIZE(_pdata), _iflags)

#define MESON_CLK_DIV_RW(_name, _reg, _shift, _width, _table, _dflags,	\
			 _phw, _iflags)					\
	__MESON_CLK_DIV(_name, _reg, _shift, _width, _table,		\
			0, 0, _dflags,					\
			&clk_regmap_divider_ops, NULL, NULL,		\
			(const struct clk_hw *[]) { _phw }, _iflags)

#define MESON_CLK_DIV_RO(_name, _reg, _shift, _width, _table, _dflags,	\
			 _phw, _iflags)					\
	__MESON_CLK_DIV(_name, _reg, _shift, _width, _table,		\
			0, 0, CLK_DIVIDER_READ_ONLY | (_dflags),	\
			&clk_regmap_divider_ro_ops, NULL, NULL,		\
			(const struct clk_hw *[]) { _phw }, _iflags)

#define MESON_CLK_GATE_RW(_name, _reg, _bit, _dflags, _phw, _iflags)	\
	__MESON_CLK_GATE(_name, _reg, _bit, _dflags,			\
			 &clk_regmap_gate_ops, NULL, NULL,		\
			 (const struct clk_hw *[]) { _phw }, _iflags)

#define MESON_CLK_GATE_RO(_name, _reg, _bit, _dflags, _phw, _iflags)	\
	__MESON_CLK_GATE(_name, _reg, _bit, _dflags,			\
			 &clk_regmap_gate_ro_ops, NULL, NULL,		\
			 (const struct clk_hw *[]) { _phw }, _iflags)

#define MESON_CLK_CPU_DYN_SEC_RW(_name, _smcid, _secid_dyn,		\
				 _secid_dyn_rd, _table, _pdata, _iflags)\
	__MESON_CLK_CPU_DYN_SEC(_name, _smcid, _secid_dyn,		\
				_secid_dyn_rd, _table, ARRAY_SIZE(_table),\
				&meson_clk_cpu_dyn_ops,			\
				NULL, _pdata, NULL, ARRAY_SIZE(_pdata),	\
				_iflags)

#define MESON_CLK_FIXED_FACTOR(_name, _mult, _div, _phw, _iflags)	\
	__MESON_CLK_FIXED_FACTOR(_name, _mult, _div,			\
				 &clk_fixed_factor_ops, NULL, NULL,	\
				 (const struct clk_hw *[]) { _phw },	\
				 _iflags)

#define MESON_CLK_DUALDIV_RW(_name, _n1_reg, _n1_shift, _n1_width,	\
			     _n2_reg, _n2_shift, _n2_width,		\
			     _m1_reg, _m1_shift, _m1_width,		\
			     _m2_reg, _m2_shift, _m2_width,		\
			     _d_reg, _d_shift, _d_width, _dualdiv_table,\
			     _phw, _iflags)				\
	__MESON_CLK_DUALDIV(_name, _n1_reg, _n1_shift, _n1_width,	\
			    _n2_reg, _n2_shift, _n2_width,		\
			    _m1_reg, _m1_shift, _m1_width,		\
			    _m2_reg, _m2_shift, _m2_width,		\
			    _d_reg, _d_shift, _d_width, _dualdiv_table,	\
			    &meson_clk_dualdiv_ops, NULL, NULL,		\
			    (const struct clk_hw *[]) { _phw }, _iflags)

#ifdef CONFIG_ARM
#define MESON_CLK_PLL_RW(_name, _en_reg, _en_shift, _en_width,		\
			 _m_reg, _m_shift, _m_width,			\
			 _f_reg, _f_shift, _f_width,			\
			 _n_reg, _n_shift, _n_width,			\
			 _l_reg, _l_shift, _l_width,			\
			 _r_reg, _r_shift, _r_width,			\
			 _th_reg, _th_shift, _th_width,			\
			 _init_reg, _range, _table,			\
			 _dflags, _pdata, _iflags,			\
			 _od_reg, _od_shift, _od_width)			\
	__MESON_CLK_PLL(_name, _en_reg, _en_shift, _en_width,		\
			_m_reg, _m_shift, _m_width,			\
			_f_reg, _f_shift, _f_width,			\
			_n_reg, _n_shift, _n_width,			\
			_l_reg, _l_shift, _l_width,			\
			_r_reg, _r_shift, _r_width,			\
			_th_reg, _th_shift, _th_width,			\
			_init_reg, ARRAY_SIZE(_init_reg), _range, _table,\
			0, 0, 0, _dflags, &meson_clk_pll_v3_ops,	\
			NULL, _pdata, NULL, _iflags,			\
			_od_reg, _od_shift, _od_width)

#define MESON_CLK_PLL_RO(_name, _en_reg, _en_shift, _en_width,		\
			 _m_reg, _m_shift, _m_width,			\
			 _f_reg, _f_shift, _f_width,			\
			 _n_reg, _n_shift, _n_width,			\
			 _l_reg, _l_shift, _l_width,			\
			 _r_reg, _r_shift, _r_width,			\
			 _th_reg, _th_shift, _th_width,			\
			 _range, _table,				\
			 _dflags, _pdata, _iflags,			\
			 _od_reg, _od_shift, _od_width)			\
	__MESON_CLK_PLL(_name, _en_reg, _en_shift, _en_width,		\
			_m_reg, _m_shift, _m_width,			\
			_f_reg, _f_shift, _f_width,			\
			_n_reg, _n_shift, _n_width,			\
			_l_reg, _l_shift, _l_width,			\
			_r_reg, _r_shift, _r_width,			\
			_th_reg, _th_shift, _th_width,			\
			NULL, 0, _range, _table,			\
			0, 0, 0, _dflags, &meson_clk_pll_ro_ops,	\
			NULL, _pdata, NULL, _iflags,			\
			_od_reg, _od_shift, _od_width)

#define MESON_CLK_PLL_SEC(_name, _en_reg, _en_shift, _en_width,		\
			  _m_reg, _m_shift, _m_width,			\
			  _f_reg, _f_shift, _f_width,			\
			  _n_reg, _n_shift, _n_width,			\
			  _l_reg, _l_shift, _l_width,			\
			  _r_reg, _r_shift, _r_width,			\
			  _th_reg, _th_shift, _th_width,		\
			  _range, _table,				\
			  _smcid, _secid, _secid_dis, _dflags,		\
			  _pdata, _iflags,				\
			  _od_reg, _od_shift, _od_width)		\
	__MESON_CLK_PLL(_name, _en_reg, _en_shift, _en_width,		\
			_m_reg, _m_shift, _m_width,			\
			_f_reg, _f_shift, _f_width,			\
			_n_reg, _n_shift, _n_width,			\
			_l_reg, _l_shift, _l_width,			\
			_r_reg, _r_shift, _r_width,			\
			_th_reg, _th_shift, _th_width,			\
			NULL, 0, _range, _table,			\
			_smcid, _secid, _secid_dis, _dflags,		\
			&meson_secure_pll_v2_ops,			\
			NULL, _pdata, NULL, _iflags,			\
			_od_reg, _od_shift, _od_width)
#else
#define MESON_CLK_PLL_RW(_name, _en_reg, _en_shift, _en_width,		\
			 _m_reg, _m_shift, _m_width,			\
			 _f_reg, _f_shift, _f_width,			\
			 _n_reg, _n_shift, _n_width,			\
			 _l_reg, _l_shift, _l_width,			\
			 _r_reg, _r_shift, _r_width,			\
			 _th_reg, _th_shift, _th_width,			\
			 _init_reg, _range, _table,			\
			 _dflags, _pdata, _iflags,			\
			 _od_reg, _od_shift, _od_width, _od_table,	\
			 _od_dflags)					\
	__MESON_CLK_PLL(_name ## _dco, _en_reg, _en_shift, _en_width,	\
			_m_reg, _m_shift, _m_width,			\
			_f_reg, _f_shift, _f_width,			\
			_n_reg, _n_shift, _n_width,			\
			_l_reg, _l_shift, _l_width,			\
			_r_reg, _r_shift, _r_width,			\
			_th_reg, _th_shift, _th_width,			\
			_init_reg, ARRAY_SIZE(_init_reg), _range, _table,\
			0, 0, 0, _dflags,				\
			&meson_clk_pll_v3_ops,				\
			NULL, _pdata, NULL, _iflags,			\
			_name, _od_reg, _od_shift, _od_width,		\
			_od_table, 0, 0, _od_dflags,			\
			&clk_regmap_divider_ops, CLK_SET_RATE_PARENT)

#define MESON_CLK_PLL_RO(_name, _en_reg, _en_shift, _en_width,		\
			 _m_reg, _m_shift, _m_width,			\
			 _f_reg, _f_shift, _f_width,			\
			 _n_reg, _n_shift, _n_width,			\
			 _l_reg, _l_shift, _l_width,			\
			 _r_reg, _r_shift, _r_width,			\
			 _th_reg, _th_shift, _th_width,			\
			 _range, _table,				\
			 _dflags, _pdata, _iflags,			\
			 _od_reg, _od_shift, _od_width, _od_table,	\
			 _od_dflags)	\
	__MESON_CLK_PLL(_name ## _dco, _en_reg, _en_shift, _en_width,	\
			_m_reg, _m_shift, _m_width,			\
			_f_reg, _f_shift, _f_width,			\
			_n_reg, _n_shift, _n_width,			\
			_l_reg, _l_shift, _l_width,			\
			_r_reg, _r_shift, _r_width,			\
			_th_reg, _th_shift, _th_width,			\
			NULL, 0, _range, _table,			\
			0, 0, 0, _dflags,				\
			&meson_clk_pll_ro_ops,				\
			NULL, _pdata, NULL, _iflags,			\
			_name, _od_reg, _od_shift, _od_width,		\
			_od_table, 0, 0, _od_dflags,			\
			&clk_regmap_divider_ro_ops, CLK_SET_RATE_PARENT)

#define MESON_CLK_PLL_SEC(_name, _en_reg, _en_shift, _en_width,		\
			 _m_reg, _m_shift, _m_width,			\
			 _f_reg, _f_shift, _f_width,			\
			 _n_reg, _n_shift, _n_width,			\
			 _l_reg, _l_shift, _l_width,			\
			 _r_reg, _r_shift, _r_width,			\
			 _th_reg, _th_shift, _th_width,			\
			 _range, _table,				\
			 _smcid, _secid, _secid_dis, _dflags,		\
			 _pdata, _iflags,				\
			 _od_reg, _od_shift, _od_width, _od_table,	\
			 _od_smcid, _od_secid, _od_dflags)		\
	__MESON_CLK_PLL(_name ## _dco, _en_reg, _en_shift, _en_width,	\
			_m_reg, _m_shift, _m_width,			\
			_f_reg, _f_shift, _f_width,			\
			_n_reg, _n_shift, _n_width,			\
			_l_reg, _l_shift, _l_width,			\
			_r_reg, _r_shift, _r_width,			\
			_od_reg, _od_shift, _od_width,			\
			NULL, 0, _range, _table,			\
			_smcid, _secid, _secid_dis, _dflags,		\
			&meson_secure_pll_v2_ops,			\
			NULL, _pdata, NULL, _iflags,			\
			_name, _od_reg, _od_shift, _od_width,		\
			_od_table, _od_smcid, _od_secid, _od_dflags,	\
			&clk_regmap_secure_v2_divider_ops, CLK_SET_RATE_PARENT)
#endif

#define __MESON_CLK_COMPOSITE(_m_name, _m_reg, _m_mask, _m_shift,	\
			      _m_table, _m_dflags, _m_ops, _pname,	\
			      _pdata, _phw, _pnub, _m_iflags,		\
			      _d_name, _d_reg, _d_shift, _d_width,	\
			      _d_table, _d_dflags, _d_ops, _d_iflags,	\
			      _g_name, _g_reg, _g_bit, _g_dflags,	\
			      _g_ops, _g_iflags)			\
static struct clk_regmap _m_name = {					\
	.data = &(struct clk_regmap_mux_data){				\
		.offset = _m_reg,					\
		.mask = _m_mask,					\
		.shift = _m_shift,					\
		.table = _m_table,					\
		.flags = _m_dflags,					\
	},								\
	.hw.init = &(struct clk_init_data){				\
		.name = # _m_name,					\
		.ops = _m_ops,						\
		.parent_names = _pname,					\
		.parent_data = _pdata,					\
		.parent_hws = _phw,					\
		.num_parents = _pnub,					\
		.flags = _m_iflags,					\
	},								\
};									\
static struct clk_regmap _d_name = {					\
	.data = &(struct clk_regmap_div_data){				\
		.offset = _d_reg,					\
		.shift = _d_shift,					\
		.width = _d_width,					\
		.table = _d_table,					\
		.flags = _d_dflags,					\
	},								\
	.hw.init = &(struct clk_init_data){				\
		.name = # _d_name,					\
		.ops = _d_ops,						\
		.parent_hws = (const struct clk_hw *[]) {		\
			&_m_name.hw },					\
		.num_parents = 1,					\
		.flags = _d_iflags,					\
	},								\
};									\
static struct clk_regmap _g_name = {					\
	.data = &(struct clk_regmap_gate_data){				\
		.offset = _g_reg,					\
		.bit_idx = _g_bit,					\
		.flags = _g_dflags,					\
	},								\
	.hw.init = &(struct clk_init_data) {				\
		.name = # _g_name,					\
		.ops = _g_ops,						\
		.parent_hws = (const struct clk_hw *[]) {		\
			&_d_name.hw },					\
		.num_parents = 1,					\
		.flags = _g_iflags,					\
	},								\
}

#define MESON_CLK_COMPOSITE_RW(_cname, _m_reg, _m_mask, _m_shift,	\
			       _m_table, _m_dflags, _m_pdata, _m_iflags,\
			       _d_reg, _d_shift, _d_width, _d_table,	\
			       _d_dflags, _d_iflags,			\
			       _g_reg, _g_bit, _g_dflags, _g_iflags)	\
	__MESON_CLK_COMPOSITE(_cname ## _sel, _m_reg, _m_mask, _m_shift,\
			      _m_table, _m_dflags, &clk_regmap_mux_ops,	\
			      NULL, _m_pdata, NULL,			\
			      ARRAY_SIZE(_m_pdata), _m_iflags,		\
			      _cname ## _div,				\
			      _d_reg, _d_shift, _d_width, _d_table,	\
			      _d_dflags,				\
			      &clk_regmap_divider_ops, _d_iflags,	\
			      _cname, _g_reg, _g_bit, _g_dflags,	\
			      &clk_regmap_gate_ops, _g_iflags)

#define MESON_CLK_COMPOSITE_RO(_cname, _m_reg, _m_mask, _m_shift,	\
			       _m_table, _m_dflags, _m_pdata, _m_iflags,\
			       _d_reg, _d_shift, _d_width, _d_table,	\
			       _d_dflags, _d_iflags,			\
			       _g_reg, _g_bit, _g_dflags, _g_iflags)	\
	__MESON_CLK_COMPOSITE(_cname ## _sel, _m_reg, _m_mask, _m_shift,\
			      _m_table, CLK_MUX_READ_ONLY | (_m_dflags),\
			      &clk_regmap_mux_ro_ops,			\
			      NULL, _m_pdata, NULL,			\
			      ARRAY_SIZE(_m_pdata), _m_iflags,		\
			      _cname ## _div,				\
			      _d_reg, _d_shift, _d_width, _d_table,	\
			      CLK_DIVIDER_READ_ONLY | (_d_dflags),	\
			      &clk_regmap_divider_ro_ops, _d_iflags,	\
			      _cname, _g_reg, _g_bit, _g_dflags,	\
			      &clk_regmap_gate_ro_ops, _g_iflags)

static const struct clk_parent_data pll_dco_parent = {
	.fw_name = "xtal",
};

MESON_CLK_PLL_RO(fixed_pll, ANACTRL_FIXPLL_CTRL0, 28, 1,  /* en */
		 ANACTRL_FIXPLL_CTRL0, 0,  9,  /* m */
		 0, 0,  0,  /* frac */
		 ANACTRL_FIXPLL_CTRL0, 10, 5,  /* n */
		 ANACTRL_FIXPLL_CTRL0, 31, 1,  /* lock */
		 ANACTRL_FIXPLL_CTRL0, 29, 1,  /* rst */
		 0, 0, 0,  /* th */
		 NULL, NULL,
		 0, &pll_dco_parent, 0,
		 ANACTRL_FIXPLL_CTRL0, 16, 2
#ifdef CONFIG_ARM
		 );
#else
		 , NULL, CLK_DIVIDER_POWER_OF_TWO);
#endif

MESON_CLK_FIXED_FACTOR(fclk50m_div40, 1, 40, &fixed_pll.hw, 0);
MESON_CLK_GATE_RO(fclk50m, ANACTRL_FIXPLL_CTRL1, 4, 0, &fclk50m_div40.hw, 0);
MESON_CLK_FIXED_FACTOR(fclk_div2_div, 1, 2, &fixed_pll.hw, 0);
MESON_CLK_GATE_RO(fclk_div2, ANACTRL_FIXPLL_CTRL1, 20, 0, &fclk_div2_div.hw, 0);
MESON_CLK_FIXED_FACTOR(fclk_div2p5_div, 2, 5, &fixed_pll.hw, 0);
MESON_CLK_GATE_RO(fclk_div2p5, ANACTRL_FIXPLL_CTRL1, 8, 0, &fclk_div2p5_div.hw, 0);
MESON_CLK_FIXED_FACTOR(fclk_div3_div, 1, 3, &fixed_pll.hw, 0);
MESON_CLK_GATE_RO(fclk_div3, ANACTRL_FIXPLL_CTRL1, 16, 0, &fclk_div3_div.hw, 0);
MESON_CLK_FIXED_FACTOR(fclk_div4_div, 1, 4, &fixed_pll.hw, 0);
MESON_CLK_GATE_RO(fclk_div4, ANACTRL_FIXPLL_CTRL1, 17, 0, &fclk_div4_div.hw, 0);
MESON_CLK_FIXED_FACTOR(fclk_div5_div, 1, 5, &fixed_pll.hw, 0);
MESON_CLK_GATE_RO(fclk_div5, ANACTRL_FIXPLL_CTRL1, 18, 0, &fclk_div5_div.hw, 0);
MESON_CLK_FIXED_FACTOR(fclk_div7_div, 1, 7, &fixed_pll.hw, 0);
MESON_CLK_GATE_RO(fclk_div7, ANACTRL_FIXPLL_CTRL1, 19, 0, &fclk_div7_div.hw, 0);

#ifdef CONFIG_ARM
static const struct pll_params_table sys_pll_params_table[] = {
	PLL_PARAMS(42, 0, 0), /* DCO=1008M OD=1008M */
	PLL_PARAMS(46, 0, 0), /* DCO=1104M OD=1104M */
	PLL_PARAMS(50, 0, 0), /* DCO=1200M OD=1200M */
	PLL_PARAMS(54, 0, 0), /* DCO=1296M OD=1296M */
	PLL_PARAMS(55, 0, 0), /* DCO=1320M OD=1320M */
	PLL_PARAMS(58, 0, 0), /* DCO=1392M OD=1392M */
	PLL_PARAMS(59, 0, 0), /* DCO=1416M OD=1416M */
	PLL_PARAMS(62, 0, 0), /* DCO=1488M OD=1488M */
	PLL_PARAMS(63, 0, 0), /* DCO=1512M OD=1512M */
	PLL_PARAMS(67, 0, 0), /* DCO=1608M OD=1608M */
	PLL_PARAMS(71, 0, 0), /* DCO=1704M OD=1704M */
	PLL_PARAMS(75, 0, 0), /* DCO=1800M OD=1800M */
	PLL_PARAMS(79, 0, 0), /* DCO=1896M OD=1896M */
	PLL_PARAMS(80, 0, 0), /* DCO=1908M OD=1908M */
	PLL_PARAMS(83, 0, 0), /* DCO=1992M OD=1992M */
	PLL_PARAMS(84, 0, 0), /* DCO=2016M OD=2016M */
	{ /* sentinel */ }
};
#else
static const struct pll_params_table sys_pll_params_table[] = {
	PLL_PARAMS(42, 0), /* DCO=1008M */
	PLL_PARAMS(46, 0), /* DCO=1104M */
	PLL_PARAMS(50, 0), /* DCO=1200M */
	PLL_PARAMS(54, 0), /* DCO=1296M */
	PLL_PARAMS(55, 0), /* DCO=1320M */
	PLL_PARAMS(58, 0), /* DCO=1392M */
	PLL_PARAMS(59, 0), /* DCO=1416M */
	PLL_PARAMS(62, 0), /* DCO=1488M */
	PLL_PARAMS(63, 0), /* DCO=1512M */
	PLL_PARAMS(67, 0), /* DCO=1608M */
	PLL_PARAMS(71, 0), /* DCO=1704M */
	PLL_PARAMS(75, 0), /* DCO=1800M */
	PLL_PARAMS(79, 0), /* DCO=1896M */
	PLL_PARAMS(80, 0), /* DCO=1908M */
	PLL_PARAMS(83, 0), /* DCO=1992M */
	PLL_PARAMS(84, 0), /* DCO=2016M */
	{ /* sentinel */ }
};
#endif

MESON_CLK_PLL_SEC(sys_pll, ANACTRL_SYSPLL_CTRL0, 28, 1,  /* en */
		  ANACTRL_SYSPLL_CTRL0, 0, 9,  /* m */
		  0, 0, 0,  /* frac */
		  ANACTRL_SYSPLL_CTRL0, 16, 2,  /* n */
		  ANACTRL_SYSPLL_CTRL0, 31, 1,  /* lock */
		  ANACTRL_SYSPLL_CTRL0, 29, 1,  /* rst */
		  ANACTRL_SYSPLL_CTRL0, 24, 1,  /* th */
		  NULL, sys_pll_params_table,
		  SECURE_PLL_CLK, SECID_SYS_DCO_PLL, SECID_SYS_DCO_PLL_DIS,
		  CLK_MESON_PLL_IGNORE_INIT | CLK_MESON_PLL_POWER_OF_TWO,
		  &pll_dco_parent, CLK_IGNORE_UNUSED,
		  ANACTRL_SYSPLL_CTRL0, 12, 2
#ifdef CONFIG_ARM
		  );
#else
		  , NULL, SECURE_PLL_CLK, SECID_SYS_PLL_OD,
		  CLK_DIVIDER_POWER_OF_TWO);
#endif

#ifdef CONFIG_ARM
static const struct pll_params_table gp0_pll_table[] = {
	PLL_PARAMS(70, 0, 1), /* DCO = 1680M OD = 1 PLL = 840M */
	PLL_PARAMS(66, 0, 1), /* DCO = 1584M OD = 1 PLL = 792M */
	PLL_PARAMS(62, 0, 1), /* DCO = 1488M OD = 1 PLL = 744M */
	{ /* sentinel */ }
};
#else
static const struct pll_params_table gp0_pll_table[] = {
	PLL_PARAMS(70, 0), /* DCO = 1680M */
	PLL_PARAMS(66, 0), /* DCO = 1584M */
	PLL_PARAMS(62, 0), /* DCO = 1488M */
	{ /* sentinel */ }
};
#endif

static const struct reg_sequence gp0_init_regs[] = {
	{ .reg = ANACTRL_GP0PLL_CTRL0,	.def = 0x61001053 },
	{ .reg = ANACTRL_GP0PLL_CTRL0,	.def = 0x71001053 },
	{ .reg = ANACTRL_GP0PLL_CTRL1,	.def = 0x39002000 },
	{ .reg = ANACTRL_GP0PLL_CTRL2,	.def = 0x00001140 },
	{ .reg = ANACTRL_GP0PLL_CTRL3,	.def = 0x00000000, .delay_us = 20 },
	{ .reg = ANACTRL_GP0PLL_CTRL0,	.def = 0x51001053, .delay_us = 20 },
	{ .reg = ANACTRL_GP0PLL_CTRL2,	.def = 0x00001100 },
};

MESON_CLK_PLL_RW(gp0_pll, ANACTRL_GP0PLL_CTRL0, 28, 1,  /* en */
		 ANACTRL_GP0PLL_CTRL0, 0, 9,  /* m */
		 0, 0, 0,  /* frac */
		 ANACTRL_GP0PLL_CTRL0, 16, 2,  /* n */
		 ANACTRL_GP0PLL_CTRL0, 31, 1,  /* lock */
		 ANACTRL_GP0PLL_CTRL0, 29, 1,  /* rst */
		 ANACTRL_GP0PLL_CTRL0, 24, 1,  /* th */
		 gp0_init_regs, NULL, gp0_pll_table,
		 CLK_MESON_PLL_IGNORE_INIT | CLK_MESON_PLL_POWER_OF_TWO,
		 &pll_dco_parent, 0,
		 ANACTRL_GP0PLL_CTRL0, 12, 2
#ifdef CONFIG_ARM
		 );
#else
		 , NULL, CLK_DIVIDER_POWER_OF_TWO);
#endif

static const struct clk_parent_data hifi_pll_dco_parent = {
	.hw = &fclk_div5.hw,
};

#ifdef CONFIG_ARM
static const struct pll_params_table hifi_pll_table[] = {
	PLL_PARAMS(39, 3, 2), /* DCO=1950M OD=487.5M */
	PLL_PARAMS(36, 3, 2), /* DCO=1800M OD=450M */
	PLL_PARAMS(36, 3, 0), /* DCO=1800M OD=1800M */
	{ /* sentinel */  }
};
#else
static const struct pll_params_table hifi_pll_table[] = {
	PLL_PARAMS(39, 3), /* DCO=1950M */
	PLL_PARAMS(36, 3), /* DCO=1800M */
	{ /* sentinel */  }
};
#endif

static const struct reg_sequence hifi_init_regs[] = {
	{ .reg = ANACTRL_HIFIPLL_CTRL0,	.def = 0x61030024 },
	{ .reg = ANACTRL_HIFIPLL_CTRL0,	.def = 0x71030024 },
	{ .reg = ANACTRL_HIFIPLL_CTRL1,	.def = 0x25002000 },
	{ .reg = ANACTRL_HIFIPLL_CTRL2,	.def = 0x05001140 },
	{ .reg = ANACTRL_HIFIPLL_CTRL3,	.def = 0x00091940, .delay_us = 20 },
	{ .reg = ANACTRL_HIFIPLL_CTRL0,	.def = 0x51030024, .delay_us = 20 },
	{ .reg = ANACTRL_HIFIPLL_CTRL2,	.def = 0x05001100 },
};

MESON_CLK_PLL_RW(hifi_pll, ANACTRL_HIFIPLL_CTRL0, 28, 1,  /* en */
		 ANACTRL_HIFIPLL_CTRL0, 0, 9,  /* m */
		 ANACTRL_HIFIPLL_CTRL3, 0, 19,  /* frac */
		 ANACTRL_HIFIPLL_CTRL0, 16, 2,  /* n */
		 ANACTRL_HIFIPLL_CTRL0, 31, 1,  /* lock */
		 ANACTRL_HIFIPLL_CTRL0, 29, 1,  /* rst */
		 ANACTRL_HIFIPLL_CTRL0, 24, 1,  /* th */
		 hifi_init_regs, NULL, hifi_pll_table,
		 CLK_MESON_PLL_ROUND_CLOSEST | CLK_MESON_PLL_POWER_OF_TWO |
		 CLK_MESON_PLL_FIXED_FRAC_WEIGHT_PRECISION,
		 &hifi_pll_dco_parent, 0,
		 ANACTRL_HIFIPLL_CTRL0, 12, 2
#ifdef CONFIG_ARM
		 );
#else
		 , NULL, CLK_DIVIDER_POWER_OF_TWO);
#endif

static const struct reg_sequence hifi1_init_regs[] = {
	{ .reg = ANACTRL_HIFI1PLL_CTRL0,	.def = 0x61030024 },
	{ .reg = ANACTRL_HIFI1PLL_CTRL0,	.def = 0x71030024 },
	{ .reg = ANACTRL_HIFI1PLL_CTRL1,	.def = 0x25002000 },
	{ .reg = ANACTRL_HIFI1PLL_CTRL2,	.def = 0x05001140 },
	{ .reg = ANACTRL_HIFI1PLL_CTRL3,	.def = 0x00091940, .delay_us = 20 },
	{ .reg = ANACTRL_HIFI1PLL_CTRL0,	.def = 0x51030024, .delay_us = 20 },
	{ .reg = ANACTRL_HIFI1PLL_CTRL2,	.def = 0x05001100 },
};

MESON_CLK_PLL_RW(hifi1_pll, ANACTRL_HIFI1PLL_CTRL0, 28, 1,  /* en */
		 ANACTRL_HIFI1PLL_CTRL0, 0, 9,  /* m */
		 ANACTRL_HIFI1PLL_CTRL3, 0, 19,  /* frac */
		 ANACTRL_HIFI1PLL_CTRL0, 16, 2,  /* n */
		 ANACTRL_HIFI1PLL_CTRL0, 31, 1,  /* lock */
		 ANACTRL_HIFI1PLL_CTRL0, 29, 1,  /* rst */
		 ANACTRL_HIFI1PLL_CTRL0, 24, 1,  /* th */
		 hifi1_init_regs, NULL, hifi_pll_table,
		 CLK_MESON_PLL_ROUND_CLOSEST | CLK_MESON_PLL_POWER_OF_TWO |
		 CLK_MESON_PLL_FIXED_FRAC_WEIGHT_PRECISION,
		 &hifi_pll_dco_parent, 0,
		 ANACTRL_HIFI1PLL_CTRL0, 12, 2
#ifdef CONFIG_ARM
		 );
#else
		 , NULL, CLK_DIVIDER_POWER_OF_TWO);
#endif

/* cpu clock */
static const struct cpu_dyn_table cpu_dyn_parent_table[] = {
	CPU_LOW_PARAMS(24000000,   0, 0, 0),
	CPU_LOW_PARAMS(100000000,  1, 1, 9),
	CPU_LOW_PARAMS(250000000,  1, 1, 3),
	CPU_LOW_PARAMS(333333333,  2, 1, 1),
	CPU_LOW_PARAMS(500000000,  1, 1, 1),
	CPU_LOW_PARAMS(666666666,  2, 0, 0),
	CPU_LOW_PARAMS(1000000000, 1, 0, 0),
};

static const struct clk_parent_data cpu_dyn_parent_data[] = {
	{ .fw_name = "xtal", },
	{ .hw = &fclk_div2.hw },
	{ .hw = &fclk_div3.hw }
};

MESON_CLK_CPU_DYN_SEC_RW(cpu_dyn_clk, SECURE_CPU_CLK, SECID_CPU_CLK_DYN,
			 SECID_CPU_CLK_RD, cpu_dyn_parent_table,
			 cpu_dyn_parent_data, 0);

static const struct clk_parent_data cpu_parent_data[] = {
	{ .hw = &cpu_dyn_clk.hw },
	{ .hw = &sys_pll.hw }
};

MESON_CLK_MUX_SEC(cpu_clk, CPUCTRL_SYS_CPU_CLK_CTRL, 0x1, 11,
		  NULL, SECURE_CPU_CLK, SECID_CPU_CLK_SEL, SECID_CPU_CLK_RD,
		  0, cpu_parent_data,
		  CLK_SET_RATE_PARENT);

/*
 *rtc 32k clock
 *                                                         |\
 * xtal--+------------------------------------------------>| \
 *       |   ______       ________                         |  |
 *	 |  |      |     |        |    |\       ______     |  |
 *	 +->| GATE |---->| divN/M |--->| \     |      |    |  |
 *	    |______|  |  |________|    |  |--->| GATE |--->|  | rtc_clk
 *                    |                |  |    |______|    |  |--------->
 *                    +--------------->| /                 |  |
 *                                     |/                  |  |
 *                                                         |  |
 * PAD---------------------------------------------------->| /
 *	                                                   |/
 **/
static const struct clk_parent_data rtc_xtal_clkin_parent = {
	.fw_name = "xtal",
};

__MESON_CLK_GATE(rtc_32k_clkin, CLKCTRL_RTC_BY_OSCIN_CTRL0, 31, 0,
		 &clk_regmap_gate_ops, NULL, &rtc_xtal_clkin_parent, NULL, 0);

static const struct meson_clk_dualdiv_param rtc_32k_div_table[] = {
	{ 733, 732, 8, 11, 1 },
	{ /* sentinel */ }
};

MESON_CLK_DUALDIV_RW(rtc_32k_div, CLKCTRL_RTC_BY_OSCIN_CTRL0, 0,  12,
		     CLKCTRL_RTC_BY_OSCIN_CTRL0, 12, 12,
		     CLKCTRL_RTC_BY_OSCIN_CTRL1, 0,  12,
		     CLKCTRL_RTC_BY_OSCIN_CTRL1, 12, 12,
		     CLKCTRL_RTC_BY_OSCIN_CTRL0, 28, 1, rtc_32k_div_table,
		     &rtc_32k_clkin.hw, 0);

static const struct clk_parent_data rtc_32k_sel_parent_data[] = {
	{ .hw = &rtc_32k_div.hw },
	{ .hw = &rtc_32k_clkin.hw }
};

MESON_CLK_MUX_RW(rtc_32k_sel, CLKCTRL_RTC_BY_OSCIN_CTRL1, 0x1, 24, NULL, 0,
		 rtc_32k_sel_parent_data, CLK_SET_RATE_PARENT);

MESON_CLK_GATE_RW(rtc_32k, CLKCTRL_RTC_BY_OSCIN_CTRL0, 30, 0,
		  &rtc_32k.hw, CLK_SET_RATE_PARENT);

static const struct clk_parent_data rtc_clk_sel_parent_data[] = {
	{ .fw_name = "xtal" },
	{ .hw = &rtc_32k.hw },
	{ .fw_name = "pad" }
};

MESON_CLK_MUX_RW(rtc_clk, CLKCTRL_RTC_CTRL, 0x3, 0, NULL, 0,
		 rtc_clk_sel_parent_data, CLK_SET_RATE_PARENT);

/*
 * Some clocks have multiple clock sources, and the parent clock and index are
 * discontinuous, Some channels corresponding to the clock index are not
 * actually connected inside the chip, or the clock source is invalid.
 */
static u32 sys_axi_parent_table[] = { 0, 1, 2, 3, 4, 7 };
static const struct clk_parent_data sys_axi_parent_data[] = {
	{ .fw_name = "xtal" },
	{ .hw = &fclk_div2.hw },
	{ .hw = &fclk_div3.hw },
	{ .hw = &fclk_div4.hw },
	{ .hw = &fclk_div5.hw },
	{ .hw = &rtc_clk.hw }
};

MESON_CLK_COMPOSITE_RW(sys_0, CLKCTRL_SYS_CLK_CTRL0, 0x7, 10,
		       sys_axi_parent_table, 0, sys_axi_parent_data, 0,
		       CLKCTRL_SYS_CLK_CTRL0, 0, 10, NULL, 0, 0,
		       CLKCTRL_SYS_CLK_CTRL0, 13, 0, CLK_IGNORE_UNUSED);
MESON_CLK_COMPOSITE_RW(sys_1, CLKCTRL_SYS_CLK_CTRL0, 0x7, 26,
		       sys_axi_parent_table, 0, sys_axi_parent_data, 0,
		       CLKCTRL_SYS_CLK_CTRL0, 16, 10, NULL, 0, 0,
		       CLKCTRL_SYS_CLK_CTRL0, 29, 0, CLK_IGNORE_UNUSED);
static const struct clk_parent_data sys_clk_parent_data[] = {
	{ .hw = &sys_0.hw },
	{ .hw = &sys_1.hw }
};

MESON_CLK_MUX_RW(sys_clk, CLKCTRL_SYS_CLK_CTRL0, 1, 15, NULL, 0,
		 sys_clk_parent_data, 0);

MESON_CLK_COMPOSITE_RW(axi_0, CLKCTRL_AXI_CLK_CTRL0, 0x7, 10,
		       sys_axi_parent_table, 0, sys_axi_parent_data, 0,
		       CLKCTRL_AXI_CLK_CTRL0, 0, 10, NULL, 0, 0,
		       CLKCTRL_AXI_CLK_CTRL0, 13, 0, CLK_IGNORE_UNUSED);
MESON_CLK_COMPOSITE_RW(axi_1, CLKCTRL_AXI_CLK_CTRL0, 0x7, 26,
		       sys_axi_parent_table, 0, sys_axi_parent_data, 0,
		       CLKCTRL_AXI_CLK_CTRL0, 16, 10, NULL, 0, 0,
		       CLKCTRL_AXI_CLK_CTRL0, 29, 0, CLK_IGNORE_UNUSED);
static const struct clk_parent_data axi_clk_parent_data[] = {
	{ .hw = &axi_0.hw },
	{ .hw = &axi_1.hw }
};

MESON_CLK_MUX_RW(axi_clk, CLKCTRL_AXI_CLK_CTRL0, 1, 15, NULL, 0,
		 axi_clk_parent_data, 0);

/* sys_clk */
#define MESON_CLK_GATE_SYS_CLK(_name, _reg, _bit)			\
	MESON_CLK_GATE_RW(_name, _reg, _bit, 0, &sys_clk.hw, CLK_IGNORE_UNUSED)

/* axi_clk */
#define MESON_CLK_GATE_AXI_CLK(_name, _reg, _bit)			\
	MESON_CLK_GATE_RW(_name, _reg, _bit, 0, &axi_clk.hw, CLK_IGNORE_UNUSED)

MESON_CLK_GATE_SYS_CLK(sys_dev_arb, CLKCTRL_SYS_CLK_EN0_REG0, 0);
MESON_CLK_GATE_SYS_CLK(sys_dos, CLKCTRL_SYS_CLK_EN0_REG0, 1);
MESON_CLK_GATE_SYS_CLK(sys_eth_phy, CLKCTRL_SYS_CLK_EN0_REG0, 2);
MESON_CLK_GATE_SYS_CLK(sys_aocpu, CLKCTRL_SYS_CLK_EN0_REG0, 3);
MESON_CLK_GATE_SYS_CLK(sys_cec, CLKCTRL_SYS_CLK_EN0_REG0, 4);
MESON_CLK_GATE_SYS_CLK(sys_nic, CLKCTRL_SYS_CLK_EN0_REG0, 5);
MESON_CLK_GATE_SYS_CLK(sys_sd_emmc_c, CLKCTRL_SYS_CLK_EN0_REG0, 6);
MESON_CLK_GATE_SYS_CLK(sys_smart_card, CLKCTRL_SYS_CLK_EN0_REG0, 7);
MESON_CLK_GATE_SYS_CLK(sys_acodec, CLKCTRL_SYS_CLK_EN0_REG0, 8);
MESON_CLK_GATE_SYS_CLK(sys_msr_clk, CLKCTRL_SYS_CLK_EN0_REG0, 9);
MESON_CLK_GATE_SYS_CLK(sys_ir_ctrl, CLKCTRL_SYS_CLK_EN0_REG0, 10);
MESON_CLK_GATE_SYS_CLK(sys_audio, CLKCTRL_SYS_CLK_EN0_REG0, 11);
MESON_CLK_GATE_SYS_CLK(sys_eth_mac, CLKCTRL_SYS_CLK_EN0_REG0, 12);
MESON_CLK_GATE_SYS_CLK(sys_uart_a, CLKCTRL_SYS_CLK_EN0_REG0, 13);
MESON_CLK_GATE_SYS_CLK(sys_uart_b, CLKCTRL_SYS_CLK_EN0_REG0, 14);
MESON_CLK_GATE_SYS_CLK(sys_ts_pll, CLKCTRL_SYS_CLK_EN0_REG0, 16);
MESON_CLK_GATE_SYS_CLK(sys_ge2d, CLKCTRL_SYS_CLK_EN0_REG0, 17);
MESON_CLK_GATE_SYS_CLK(sys_usb, CLKCTRL_SYS_CLK_EN0_REG0, 19);
MESON_CLK_GATE_SYS_CLK(sys_i2c_m_a, CLKCTRL_SYS_CLK_EN0_REG0, 20);
MESON_CLK_GATE_SYS_CLK(sys_i2c_m_b, CLKCTRL_SYS_CLK_EN0_REG0, 21);
MESON_CLK_GATE_SYS_CLK(sys_i2c_m_c, CLKCTRL_SYS_CLK_EN0_REG0, 22);
MESON_CLK_GATE_SYS_CLK(sys_hdmitx, CLKCTRL_SYS_CLK_EN0_REG0, 24);
MESON_CLK_GATE_SYS_CLK(sys_h2mi20_aes, CLKCTRL_SYS_CLK_EN0_REG0, 25);
MESON_CLK_GATE_SYS_CLK(sys_hdcp22, CLKCTRL_SYS_CLK_EN0_REG0, 26);
MESON_CLK_GATE_SYS_CLK(sys_mmc, CLKCTRL_SYS_CLK_EN0_REG0, 27);
MESON_CLK_GATE_SYS_CLK(sys_cpu_dbg, CLKCTRL_SYS_CLK_EN0_REG0, 28);
MESON_CLK_GATE_SYS_CLK(sys_vpu_intr, CLKCTRL_SYS_CLK_EN0_REG0, 29);
MESON_CLK_GATE_SYS_CLK(sys_demod, CLKCTRL_SYS_CLK_EN0_REG0, 30);
MESON_CLK_GATE_SYS_CLK(sys_sar_adc, CLKCTRL_SYS_CLK_EN0_REG0, 31);
MESON_CLK_GATE_SYS_CLK(sys_gic, CLKCTRL_SYS_CLK_EN0_REG1, 0);
MESON_CLK_GATE_SYS_CLK(sys_pwm_ab, CLKCTRL_SYS_CLK_EN0_REG1, 1);
MESON_CLK_GATE_SYS_CLK(sys_pwm_cd, CLKCTRL_SYS_CLK_EN0_REG1, 2);
MESON_CLK_GATE_SYS_CLK(sys_pad_ctrl, CLKCTRL_SYS_CLK_EN0_REG1, 4);
MESON_CLK_GATE_SYS_CLK(sys_startup, CLKCTRL_SYS_CLK_EN0_REG1, 5);
MESON_CLK_GATE_SYS_CLK(sys_securetop, CLKCTRL_SYS_CLK_EN0_REG1, 6);
MESON_CLK_GATE_SYS_CLK(sys_rom, CLKCTRL_SYS_CLK_EN0_REG1, 7);
MESON_CLK_GATE_SYS_CLK(sys_clktree, CLKCTRL_SYS_CLK_EN0_REG1, 8);
MESON_CLK_GATE_SYS_CLK(sys_pwr_ctrl, CLKCTRL_SYS_CLK_EN0_REG1, 9);
MESON_CLK_GATE_SYS_CLK(sys_sys_ctrl, CLKCTRL_SYS_CLK_EN0_REG1, 10);
MESON_CLK_GATE_SYS_CLK(sys_cpu_ctrl, CLKCTRL_SYS_CLK_EN0_REG1, 11);
MESON_CLK_GATE_SYS_CLK(sys_sram, CLKCTRL_SYS_CLK_EN0_REG1, 12);
MESON_CLK_GATE_SYS_CLK(sys_mailbox, CLKCTRL_SYS_CLK_EN0_REG1, 13);
MESON_CLK_GATE_SYS_CLK(sys_ana_ctrl, CLKCTRL_SYS_CLK_EN0_REG1, 14);
MESON_CLK_GATE_SYS_CLK(sys_jtag_ctrl, CLKCTRL_SYS_CLK_EN0_REG1, 15);
MESON_CLK_GATE_SYS_CLK(sys_irq_ctrl, CLKCTRL_SYS_CLK_EN0_REG1, 16);
MESON_CLK_GATE_SYS_CLK(sys_reset_ctrl, CLKCTRL_SYS_CLK_EN0_REG1, 17);
MESON_CLK_GATE_SYS_CLK(sys_capu, CLKCTRL_SYS_CLK_EN0_REG1, 18);

MESON_CLK_GATE_AXI_CLK(axi_sys_nic, CLKCTRL_AXI_CLK_EN0, 0);
MESON_CLK_GATE_AXI_CLK(axi_sram, CLKCTRL_AXI_CLK_EN0, 1);
MESON_CLK_GATE_AXI_CLK(axi_dev0_mmc, CLKCTRL_AXI_CLK_EN0, 2);

static u32 pwm_parent_table[] = { 0, 2, 3 };
static const struct clk_parent_data pwm_parent_data[] = {
	{ .fw_name = "xtal" },
	{ .hw = &fclk_div4.hw },
	{ .hw = &fclk_div3.hw }
};

MESON_CLK_COMPOSITE_RW(pwm_a, CLKCTRL_PWM_CLK_AB_CTRL, 0x3, 9,
		       pwm_parent_table, 0, pwm_parent_data, 0,
		       CLKCTRL_PWM_CLK_AB_CTRL, 0, 8, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_PWM_CLK_AB_CTRL, 8, 0,
		       CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);
				/* Used by regulator, and the regulator feeds the CPU.
				 * Add IGNORE flag to avoid CCF to disable it
				 * which has been set in Bootloader
				 */

MESON_CLK_COMPOSITE_RW(pwm_b, CLKCTRL_PWM_CLK_AB_CTRL, 0x3, 25,
		       pwm_parent_table, 0, pwm_parent_data, 0,
		       CLKCTRL_PWM_CLK_AB_CTRL, 16, 8, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_PWM_CLK_AB_CTRL, 24,
		       0, CLK_SET_RATE_PARENT);
MESON_CLK_COMPOSITE_RW(pwm_c, CLKCTRL_PWM_CLK_CD_CTRL, 0x3, 9,
		       pwm_parent_table, 0, pwm_parent_data, 0,
		       CLKCTRL_PWM_CLK_CD_CTRL, 0, 8, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_PWM_CLK_CD_CTRL, 8,
		       0, CLK_SET_RATE_PARENT);
MESON_CLK_COMPOSITE_RW(pwm_d, CLKCTRL_PWM_CLK_CD_CTRL, 0x3, 25,
		       pwm_parent_table, 0, pwm_parent_data, 0,
		       CLKCTRL_PWM_CLK_CD_CTRL, 16, 8, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_PWM_CLK_CD_CTRL, 24,
		       0, CLK_SET_RATE_PARENT);

static u32 emmc_parent_table[] = { 0, 1, 2, 4, 7 };
static const struct clk_parent_data emmc_parent_data[] = {
	{ .fw_name = "xtal" },
	{ .hw = &fclk_div2.hw },
	{ .hw = &fclk_div3.hw },
	{ .hw = &fclk_div2p5.hw },
	{ .hw = &gp0_pll.hw }
};

MESON_CLK_COMPOSITE_RW(sd_emmc_c, CLKCTRL_NAND_CLK_CTRL, 0x7, 9,
		       emmc_parent_table, 0, emmc_parent_data, 0,
		       CLKCTRL_NAND_CLK_CTRL, 0, 7, NULL,
		       0, 0,
		       CLKCTRL_NAND_CLK_CTRL, 7,
		       0, 0);

MESON_CLK_FIXED_FACTOR(eth_125m_div, 1, 8, &fclk_div2.hw, 0);
MESON_CLK_GATE_RW(eth_125m, CLKCTRL_ETH_CLK_CTRL, 7, 0, &eth_125m_div.hw, 0);
MESON_CLK_DIV_RW(eth_rmii_div, CLKCTRL_ETH_CLK_CTRL, 0, 7, NULL, 0,
		 &fclk_div2.hw, 0);
MESON_CLK_GATE_RW(eth_rmii, CLKCTRL_ETH_CLK_CTRL, 8, 0, &eth_rmii_div.hw,
		  CLK_SET_RATE_PARENT);

/* temperature sensor */
static const struct clk_parent_data ts_parent = {
	.fw_name = "xtal",
};

__MESON_CLK_DIV(ts_div, CLKCTRL_TS_CLK_CTRL, 0, 8, NULL, 0, 0, 0,
		&clk_regmap_divider_ops, NULL, &ts_parent, NULL, 0);
MESON_CLK_GATE_RW(ts, CLKCTRL_TS_CLK_CTRL, 8, 0, &ts_div.hw, CLK_SET_RATE_PARENT);

/* smart card */
static const struct clk_parent_data sc_parent_data[] = {
	{ .hw = &fclk_div2.hw },
	{ .hw = &fclk_div3.hw },
	{ .hw = &fclk_div5.hw },
	{ .fw_name = "xtal" },
};

MESON_CLK_COMPOSITE_RW(sc, CLKCTRL_SC_CLK_CTRL, 0x3, 9,
		       NULL, 0, sc_parent_data, 0,
		       CLKCTRL_SC_CLK_CTRL, 0, 8, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_SC_CLK_CTRL, 8,
		       0, CLK_SET_RATE_PARENT);

static const struct clk_parent_data cecb_xtal_clkin_parent = {
	.fw_name = "xtal",
};

__MESON_CLK_GATE(cecb_32k_clkin, CLKCTRL_CECB_CTRL0, 31, 0,
		 &clk_regmap_gate_ops, NULL, &cecb_xtal_clkin_parent, NULL, 0);

static const struct meson_clk_dualdiv_param cecb_32k_div_table[] = {
	{ 733, 732, 8, 11, 1 },
	{ /* sentinel */ }
};

MESON_CLK_DUALDIV_RW(cecb_32k_div, CLKCTRL_CECB_CTRL0, 0,  12,
		     CLKCTRL_CECB_CTRL0, 12, 12,
		     CLKCTRL_CECB_CTRL1, 0,  12,
		     CLKCTRL_CECB_CTRL1, 12, 12,
		     CLKCTRL_CECB_CTRL0, 28, 1, cecb_32k_div_table,
		     &cecb_32k_clkin.hw, 0);

static const struct clk_parent_data cecb_32k_div_sel_parent_data[] = {
	{ .hw = &cecb_32k_div.hw },
	{ .hw = &cecb_32k_clkin.hw }
};

MESON_CLK_MUX_RW(cecb_32k_sel, CLKCTRL_CECB_CTRL1, 0x1, 24, NULL, 0,
		 cecb_32k_div_sel_parent_data, CLK_SET_RATE_PARENT);

static const struct clk_parent_data cecb_32k_sel_parent_data[] = {
	{ .hw = &cecb_32k_sel.hw },
	{ .hw = &rtc_clk.hw }
};

MESON_CLK_MUX_RW(cecb_clk_sel, CLKCTRL_CECB_CTRL1, 0x1, 31, NULL, 0,
		 cecb_32k_sel_parent_data, CLK_SET_RATE_PARENT);

MESON_CLK_GATE_RW(cecb_clk, CLKCTRL_CECB_CTRL0, 30, 0,
		  &cecb_32k_sel.hw, CLK_SET_RATE_PARENT);

static const struct clk_parent_data hdmitx_sys_parent_data[] = {
	{ .fw_name = "xtal" },
	{ .hw = &fclk_div4.hw },
	{ .hw = &fclk_div3.hw },
	{ .hw = &fclk_div5.hw }
};

MESON_CLK_COMPOSITE_RW(hdmitx_sys, CLKCTRL_HDMI_CLK_CTRL, 0x3, 9,
		       NULL, 0, hdmitx_sys_parent_data, 0,
		       CLKCTRL_HDMI_CLK_CTRL, 0, 7, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_HDMI_CLK_CTRL, 8,
		       0, CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

MESON_CLK_COMPOSITE_RW(hdmitx_prif, CLKCTRL_HTX_CLK_CTRL0, 0x3, 9,
		       NULL, 0, hdmitx_sys_parent_data, 0,
		       CLKCTRL_HTX_CLK_CTRL0, 0, 7, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_HTX_CLK_CTRL0, 8,
		       0, CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

MESON_CLK_COMPOSITE_RW(hdmitx_200m, CLKCTRL_HTX_CLK_CTRL0, 0x3, 25,
		       NULL, 0, hdmitx_sys_parent_data, 0,
		       CLKCTRL_HTX_CLK_CTRL0, 16, 7, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_HTX_CLK_CTRL0, 24,
		       0, CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

MESON_CLK_COMPOSITE_RW(hdmitx_aud, CLKCTRL_HTX_CLK_CTRL1, 0x3, 9,
		       NULL, 0, hdmitx_sys_parent_data, 0,
		       CLKCTRL_HTX_CLK_CTRL1, 0, 7, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_HTX_CLK_CTRL1, 8,
		       0, CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

static u32 vclk_parent_table[] = {4, 5, 6, 7};
static const struct clk_parent_data vclk_parent_data[] = {
	{ .hw = &fclk_div3.hw },
	{ .hw = &fclk_div4.hw },
	{ .hw = &fclk_div5.hw },
	{ .hw = &fclk_div7.hw }
};

/* vclk */
MESON_CLK_MUX_RW(vclk_sel, CLKCTRL_VID_CLK_CTRL, 0x7, 16,
		 vclk_parent_table, 0, vclk_parent_data, 0);
MESON_CLK_GATE_RW(vclk_input, CLKCTRL_VID_CLK_DIV, 16, 0,
		  &vclk_sel.hw, CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);
MESON_CLK_DIV_RW(vclk_div, CLKCTRL_VID_CLK_DIV, 0, 8, NULL, 0,
		 &vclk_input.hw, CLK_SET_RATE_PARENT);
MESON_CLK_GATE_RW(vclk, CLKCTRL_VID_CLK_CTRL, 19, 0,
		  &vclk_div.hw, CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

MESON_CLK_GATE_RW(vclk_div1, CLKCTRL_VID_CLK_CTRL, 0, 0,
		  &vclk.hw, CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

MESON_CLK_GATE_RW(vclk_div2_gate, CLKCTRL_VID_CLK_CTRL, 1, 0,
		  &vclk.hw, CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);
MESON_CLK_FIXED_FACTOR(vclk_div2, 1, 2, &vclk_div2_gate.hw, CLK_SET_RATE_PARENT);

MESON_CLK_GATE_RW(vclk_div4_gate, CLKCTRL_VID_CLK_CTRL, 2, 0,
		  &vclk.hw, CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);
MESON_CLK_FIXED_FACTOR(vclk_div4, 1, 4, &vclk_div4_gate.hw, CLK_SET_RATE_PARENT);

MESON_CLK_GATE_RW(vclk_div6_gate, CLKCTRL_VID_CLK_CTRL, 3, 0,
		  &vclk.hw, CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);
MESON_CLK_FIXED_FACTOR(vclk_div6, 1, 6, &vclk_div6_gate.hw, CLK_SET_RATE_PARENT);

MESON_CLK_GATE_RW(vclk_div12_gate, CLKCTRL_VID_CLK_CTRL, 4, 0,
		  &vclk.hw, CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);
MESON_CLK_FIXED_FACTOR(vclk_div12, 1, 12, &vclk_div12_gate.hw, CLK_SET_RATE_PARENT);

/* vclk2 */
MESON_CLK_MUX_RW(vclk2_sel, CLKCTRL_VIID_CLK_CTRL, 0x7, 16,
		 vclk_parent_table, 0, vclk_parent_data, 0);
MESON_CLK_GATE_RW(vclk2_input, CLKCTRL_VIID_CLK_DIV, 16, 0,
		  &vclk2_sel.hw, CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);
MESON_CLK_DIV_RW(vclk2_div, CLKCTRL_VIID_CLK_DIV, 0, 8, NULL, 0,
		 &vclk2_input.hw, CLK_SET_RATE_PARENT);
MESON_CLK_GATE_RW(vclk2, CLKCTRL_VIID_CLK_CTRL, 19, 0,
		  &vclk2_div.hw, CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

MESON_CLK_GATE_RW(vclk2_div1, CLKCTRL_VIID_CLK_CTRL, 0, 0,
		  &vclk2.hw, CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

MESON_CLK_GATE_RW(vclk2_div2_gate, CLKCTRL_VIID_CLK_CTRL, 1, 0,
		  &vclk2.hw, CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);
MESON_CLK_FIXED_FACTOR(vclk2_div2, 1, 2, &vclk2_div2_gate.hw, CLK_SET_RATE_PARENT);

MESON_CLK_GATE_RW(vclk2_div4_gate, CLKCTRL_VIID_CLK_CTRL, 2, 0,
		  &vclk2.hw, CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);
MESON_CLK_FIXED_FACTOR(vclk2_div4, 1, 4, &vclk2_div4_gate.hw, CLK_SET_RATE_PARENT);

MESON_CLK_GATE_RW(vclk2_div6_gate, CLKCTRL_VIID_CLK_CTRL, 3, 0,
		  &vclk2.hw, CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);
MESON_CLK_FIXED_FACTOR(vclk2_div6, 1, 6, &vclk2_div6_gate.hw, CLK_SET_RATE_PARENT);

MESON_CLK_GATE_RW(vclk2_div12_gate, CLKCTRL_VIID_CLK_CTRL, 4, 0,
		  &vclk2.hw, CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);
MESON_CLK_FIXED_FACTOR(vclk2_div12, 1, 12, &vclk2_div12_gate.hw, CLK_SET_RATE_PARENT);

/* video clock */
static u32 vid_parent_table[] = { 0, 1, 2, 3, 4, 8, 9, 10, 11, 12 };
static const struct clk_parent_data vid_parent_data[] = {
	{ .hw = &vclk_div1.hw },
	{ .hw = &vclk_div2.hw },
	{ .hw = &vclk_div4.hw },
	{ .hw = &vclk_div6.hw },
	{ .hw = &vclk_div12.hw },
	{ .hw = &vclk2_div1.hw },
	{ .hw = &vclk2_div2.hw },
	{ .hw = &vclk2_div4.hw },
	{ .hw = &vclk2_div6.hw },
	{ .hw = &vclk2_div12.hw }
};

MESON_CLK_MUX_RW(hdmitx_pixel_sel, CLKCTRL_HDMI_CLK_CTRL, 0xf, 16,
		 vid_parent_table, 0, vid_parent_data, 0);
MESON_CLK_GATE_RW(hdmitx_pixel, CLKCTRL_VID_CLK_CTRL2, 5, 0,
		  &hdmitx_pixel_sel.hw, CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

MESON_CLK_MUX_RW(hdmitx_fe_sel, CLKCTRL_HDMI_CLK_CTRL, 0xf, 20,
		 vid_parent_table, 0, vid_parent_data, 0);
MESON_CLK_GATE_RW(hdmitx_fe, CLKCTRL_VID_CLK_CTRL2, 9, 0,
		  &hdmitx_fe_sel.hw, CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

MESON_CLK_MUX_RW(enci_sel, CLKCTRL_VID_CLK_DIV, 0xf, 28,
		 vid_parent_table, 0, vid_parent_data, 0);
MESON_CLK_GATE_RW(enci, CLKCTRL_VID_CLK_CTRL2, 0, 0,
		  &enci_sel.hw, CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

MESON_CLK_MUX_RW(encp_sel, CLKCTRL_VID_CLK_DIV, 0xf, 24,
		 vid_parent_table, 0, vid_parent_data, 0);
MESON_CLK_GATE_RW(encp, CLKCTRL_VID_CLK_CTRL2, 2, 0,
		  &encp_sel.hw, CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

MESON_CLK_MUX_RW(encl_sel, CLKCTRL_VIID_CLK_DIV, 0xf, 12,
		 vid_parent_table, 0, vid_parent_data, 0);
MESON_CLK_GATE_RW(encl, CLKCTRL_VID_CLK_CTRL2, 3, 0,
		  &encl_sel.hw, CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

MESON_CLK_MUX_RW(vdac_sel, CLKCTRL_VIID_CLK_DIV, 0xf, 28,
		 vid_parent_table, 0, vid_parent_data, 0);
MESON_CLK_GATE_RW(vdac, CLKCTRL_VID_CLK_CTRL2, 4, 0,
		  &vdac_sel.hw, CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

/* vpu */
static const struct clk_parent_data vpu_pre_parent_data[] = {
	{ .hw = &fclk_div3.hw },
	{ .hw = &fclk_div4.hw },
	{ .hw = &fclk_div5.hw },
	{ .hw = &fclk_div7.hw },
	{ .hw = &fclk_div2.hw }
};

MESON_CLK_COMPOSITE_RW(vpu_0, CLKCTRL_VPU_CLK_CTRL, 0x7, 9,
		       NULL, 0, vpu_pre_parent_data, 0,
		       CLKCTRL_VPU_CLK_CTRL, 0, 7, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_VPU_CLK_CTRL, 8,
		       0, CLK_SET_RATE_PARENT);
MESON_CLK_COMPOSITE_RW(vpu_1, CLKCTRL_VPU_CLK_CTRL, 0x7, 25,
		       NULL, 0, vpu_pre_parent_data, 0,
		       CLKCTRL_VPU_CLK_CTRL, 16, 7, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_VPU_CLK_CTRL, 24,
		       0, CLK_SET_RATE_PARENT);

static const struct clk_parent_data vpu_parent_data[] = {
	{ .hw = &vpu_0.hw },
	{ .hw = &vpu_1.hw }
};

MESON_CLK_MUX_RW(vpu, CLKCTRL_VPU_CLK_CTRL, 0x1, 31, NULL, 0,
		 vpu_parent_data, CLK_SET_RATE_PARENT | CLK_OPS_PARENT_ENABLE);

/* vpu_clkb */
static const struct clk_parent_data vpu_clkb_parent_data[] = {
	{ .hw = &vpu.hw },
	{ .hw = &fclk_div4.hw },
	{ .hw = &fclk_div5.hw },
	{ .hw = &fclk_div7.hw }
};

MESON_CLK_COMPOSITE_RW(vpu_clkb_temp, CLKCTRL_VPU_CLKB_CTRL, 0x3, 20,
		       NULL, 0, vpu_clkb_parent_data, 0,
		       CLKCTRL_VPU_CLKB_CTRL, 16, 4, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_VPU_CLKB_CTRL, 24,
		       0, CLK_SET_RATE_PARENT);
MESON_CLK_DIV_RW(vpu_clkb_div, CLKCTRL_VPU_CLKB_CTRL, 0, 8, NULL, 0,
		 &vpu_clkb_temp.hw, 0);
MESON_CLK_GATE_RW(vpu_clkb, CLKCTRL_VPU_CLKB_CTRL, 8, 0,
		  &vpu_clkb_div.hw, CLK_SET_RATE_PARENT);

/* vpu_clkc */
static const struct clk_parent_data vpu_clkc_pre_parent_data[] = {
	{ .hw = &fclk_div4.hw },
	{ .hw = &fclk_div3.hw },
	{ .hw = &fclk_div5.hw },
	{ .hw = &fclk_div7.hw },
	{ .hw = &fclk_div2.hw }
};

MESON_CLK_COMPOSITE_RW(vpu_clkc_0, CLKCTRL_VPU_CLKC_CTRL, 0x7, 9,
		       NULL, 0, vpu_clkc_pre_parent_data, 0,
		       CLKCTRL_VPU_CLKC_CTRL, 0, 7, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_VPU_CLKC_CTRL, 8,
		       0, CLK_SET_RATE_PARENT);
MESON_CLK_COMPOSITE_RW(vpu_clkc_1, CLKCTRL_VPU_CLKC_CTRL, 0x7, 25,
		       NULL, 0, vpu_clkc_pre_parent_data, 0,
		       CLKCTRL_VPU_CLKC_CTRL, 16, 7, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_VPU_CLKC_CTRL, 24,
		       0, CLK_SET_RATE_PARENT);

static const struct clk_parent_data vpu_clkc_parent_data[] = {
	{ .hw = &vpu_clkc_0.hw },
	{ .hw = &vpu_clkc_1.hw }
};

MESON_CLK_MUX_RW(vpu_clkc, CLKCTRL_VPU_CLKC_CTRL, 0x1, 31, NULL, 0,
		 vpu_clkc_parent_data, CLK_SET_RATE_PARENT | CLK_OPS_PARENT_ENABLE);

/* vapb */
static u32 vapb_pre_parent_table[] = { 0, 1, 2, 3, 4, 7 };
static const struct clk_parent_data vapb_pre_parent_data[] = {
	{ .hw = &fclk_div4.hw },
	{ .hw = &fclk_div3.hw },
	{ .hw = &fclk_div5.hw },
	{ .hw = &fclk_div7.hw },
	{ .hw = &fclk_div2.hw },
	{ .hw = &fclk_div2p5.hw }
};

MESON_CLK_COMPOSITE_RW(vapb_0, CLKCTRL_VAPBCLK_CTRL, 0x7, 9,
		       vapb_pre_parent_table, 0, vapb_pre_parent_data, 0,
		       CLKCTRL_VAPBCLK_CTRL, 0, 7, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_VAPBCLK_CTRL, 8,
		       0, CLK_SET_RATE_PARENT);
MESON_CLK_COMPOSITE_RW(vapb_1, CLKCTRL_VAPBCLK_CTRL, 0x7, 25,
		       vapb_pre_parent_table, 0, vapb_pre_parent_data, 0,
		       CLKCTRL_VAPBCLK_CTRL, 16, 7, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_VAPBCLK_CTRL, 24,
		       0, CLK_SET_RATE_PARENT);

static const struct clk_parent_data vapb_parent_data[] = {
	{ .hw = &vapb_0.hw },
	{ .hw = &vapb_1.hw }
};

MESON_CLK_MUX_RW(vapb, CLKCTRL_VAPBCLK_CTRL, 0x1, 31, NULL, 0,
		 vapb_parent_data, CLK_SET_RATE_PARENT | CLK_OPS_PARENT_ENABLE);

MESON_CLK_GATE_RW(ge2d, CLKCTRL_VAPBCLK_CTRL, 30, 0,
		  &vapb.hw, CLK_SET_RATE_PARENT);

/* vdin_meas */
static const struct clk_parent_data vdin_meas_parent_data[] = {
	{ .fw_name = "xtal" },
	{ .hw = &fclk_div4.hw },
	{ .hw = &fclk_div3.hw },
	{ .hw = &fclk_div5.hw }
};

MESON_CLK_COMPOSITE_RW(vdin_meas, CLKCTRL_VDIN_MEAS_CLK_CTRL, 0x7, 9,
		       NULL, 0, vdin_meas_parent_data, 0,
		       CLKCTRL_VDIN_MEAS_CLK_CTRL, 0, 7, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_VDIN_MEAS_CLK_CTRL, 8,
		       0, CLK_SET_RATE_PARENT);

/* vid_lock */
static const struct clk_parent_data vid_lock_parent_data[] = {
	{ .fw_name = "xtal" },
	{ .hw = &encl.hw },
	{ .hw = &enci.hw },
	{ .hw = &encp.hw }
};

MESON_CLK_COMPOSITE_RW(vid_lock, CLKCTRL_VID_LOCK_CLK_CTRL, 0x3, 8,
		       NULL, 0, vid_lock_parent_data, 0,
		       CLKCTRL_VID_LOCK_CLK_CTRL, 0, 7, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_VID_LOCK_CLK_CTRL, 7,
		       0, CLK_SET_RATE_PARENT);

/* vdec */
static u32 vdec_pre_parent_table[] = { 0, 1, 2, 3, 4, 7 };
static const struct clk_parent_data vdec_pre_parent_data[] = {
	{ .hw = &fclk_div2p5.hw },
	{ .hw = &fclk_div3.hw },
	{ .hw = &fclk_div4.hw },
	{ .hw = &fclk_div5.hw },
	{ .hw = &fclk_div7.hw },
	{ .fw_name = "xtal" }
};

MESON_CLK_COMPOSITE_RW(vdec_0, CLKCTRL_VDEC_CLK_CTRL, 0x7, 9,
		       vdec_pre_parent_table, 0, vdec_pre_parent_data, 0,
		       CLKCTRL_VDEC_CLK_CTRL, 0, 7, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_VDEC_CLK_CTRL, 8,
		       0, CLK_SET_RATE_PARENT);
MESON_CLK_COMPOSITE_RW(vdec_1, CLKCTRL_VDEC3_CLK_CTRL, 0x7, 9,
		       vdec_pre_parent_table, 0, vdec_pre_parent_data, 0,
		       CLKCTRL_VDEC3_CLK_CTRL, 0, 7, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_VDEC3_CLK_CTRL, 8,
		       0, CLK_SET_RATE_PARENT);

static const struct clk_parent_data vdec_parent_data[] = {
	{ .hw = &vdec_0.hw },
	{ .hw = &vdec_1.hw }
};

MESON_CLK_MUX_RW(vdec, CLKCTRL_VDEC3_CLK_CTRL, 0x1, 15, NULL, 0,
		 vdec_parent_data, CLK_SET_RATE_PARENT | CLK_OPS_PARENT_ENABLE);

/* hevcf */
MESON_CLK_COMPOSITE_RW(hevcf_0, CLKCTRL_VDEC2_CLK_CTRL, 0x7, 9,
		       vdec_pre_parent_table, 0, vdec_pre_parent_data, 0,
		       CLKCTRL_VDEC2_CLK_CTRL, 0, 7, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_VDEC2_CLK_CTRL, 8,
		       0, CLK_SET_RATE_PARENT);
MESON_CLK_COMPOSITE_RW(hevcf_1, CLKCTRL_VDEC4_CLK_CTRL, 0x7, 9,
		       vdec_pre_parent_table, 0, vdec_pre_parent_data, 0,
		       CLKCTRL_VDEC4_CLK_CTRL, 0, 7, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_VDEC4_CLK_CTRL, 8,
		       0, CLK_SET_RATE_PARENT);

static const struct clk_parent_data hevcf_parent_data[] = {
	{ .hw = &hevcf_0.hw },
	{ .hw = &hevcf_1.hw }
};

MESON_CLK_MUX_RW(hevcf, CLKCTRL_VDEC4_CLK_CTRL, 0x1, 15, NULL, 0,
		 hevcf_parent_data, CLK_SET_RATE_PARENT | CLK_OPS_PARENT_ENABLE);

/* demod_core */
static const struct clk_parent_data demod_core_parent_data[] = {
	{ .fw_name = "xtal" },
	{ .hw = &fclk_div7.hw },
	{ .hw = &fclk_div4.hw }
};

MESON_CLK_COMPOSITE_RW(demod_core, CLKCTRL_DEMOD_CLK_CTRL, 0x3, 9,
		       NULL, 0, demod_core_parent_data, 0,
		       CLKCTRL_DEMOD_CLK_CTRL, 0, 7, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_DEMOD_CLK_CTRL, 8,
		       0, CLK_SET_RATE_PARENT);

static const struct clk_parent_data demod_core_t2_parent_data[] = {
	{ .fw_name = "xtal" },
	{ .hw = &fclk_div5.hw },
	{ .hw = &fclk_div4.hw }
};

MESON_CLK_COMPOSITE_RW(demod_core_t2, CLKCTRL_DEMOD_CLK_CTRL1, 0x3, 9,
		       NULL, 0, demod_core_t2_parent_data, 0,
		       CLKCTRL_DEMOD_CLK_CTRL1, 0, 7, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_DEMOD_CLK_CTRL1, 8,
		       0, CLK_SET_RATE_PARENT);

/* adc_extclk_in */
static const struct clk_parent_data adc_extclk_in_parent_data[] = {
	{ .fw_name = "xtal" },
	{ .hw = &fclk_div4.hw },
	{ .hw = &fclk_div3.hw },
	{ .hw = &fclk_div5.hw },
	{ .hw = &fclk_div7.hw }
};

MESON_CLK_COMPOSITE_RW(adc_extclk_in, CLKCTRL_DEMOD_CLK_CTRL, 0x7, 25,
		       NULL, 0, adc_extclk_in_parent_data, 0,
		       CLKCTRL_DEMOD_CLK_CTRL, 16, 7, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_DEMOD_CLK_CTRL, 24,
		       0, CLK_SET_RATE_PARENT);

/* demod_32k */
static const struct clk_parent_data demod_32k_xtal_clkin_parent = {
	.fw_name = "xtal",
};

__MESON_CLK_GATE(demod_32k_clkin, CLKCTRL_DEMOD_32K_CTRL0, 31, 0,
		 &clk_regmap_gate_ops,
		 NULL, &demod_32k_xtal_clkin_parent, NULL, 0);

static const struct meson_clk_dualdiv_param demod_32k_div_table[] = {
	{ 733, 732, 8, 11, 1 },
	{ /* sentinel */ }
};

MESON_CLK_DUALDIV_RW(demod_32k_div, CLKCTRL_DEMOD_32K_CTRL0, 0,  12,
		     CLKCTRL_DEMOD_32K_CTRL0, 12, 12,
		     CLKCTRL_DEMOD_32K_CTRL1, 0,  12,
		     CLKCTRL_DEMOD_32K_CTRL1, 12, 12,
		     CLKCTRL_DEMOD_32K_CTRL0, 28, 1, demod_32k_div_table,
		     &demod_32k_clkin.hw, 0);

static const struct clk_parent_data demod_32k_div_sel_parent_data[] = {
	{ .hw = &demod_32k_div.hw },
	{ .hw = &demod_32k_clkin.hw }
};

MESON_CLK_MUX_RW(demod_32k_sel, CLKCTRL_DEMOD_32K_CTRL1, 0x1, 24, NULL, 0,
		 demod_32k_div_sel_parent_data, CLK_SET_RATE_PARENT);

MESON_CLK_GATE_RW(demod_32k, CLKCTRL_DEMOD_32K_CTRL0, 30, 0,
		  &demod_32k_sel.hw, CLK_SET_RATE_PARENT);

/*
 * clk_12_24m model
 *
 *          |------|     |-----| clk_12m_24m |-----|
 * xtal---->| gate |---->| div |------------>| pad |
 *          |------|     |-----|             |-----|
 */
static const struct clk_parent_data clk_12_24m_in_parent = {
	.fw_name = "xtal",
};

__MESON_CLK_GATE(clk_12_24m_in, CLKCTRL_CLK12_24_CTRL, 11, 0,
		 &clk_regmap_gate_ops,
		 NULL, &clk_12_24m_in_parent, NULL, 0);
MESON_CLK_DIV_RW(clk_12_24m, CLKCTRL_CLK12_24_CTRL, 10, 1, NULL, 0,
		 &clk_12_24m_in.hw, 0);

static const struct clk_parent_data sar_adc_sel_parent_data[] = {
	{ .fw_name = "xtal" },
	{ .hw = &sys_clk.hw }
};

MESON_CLK_COMPOSITE_RW(sar_adc, CLKCTRL_SAR_CLK_CTRL, 0x3, 9,
		       NULL, 0, sar_adc_sel_parent_data, 0,
		       CLKCTRL_SAR_CLK_CTRL, 0, 8, NULL,
		       0, CLK_SET_RATE_PARENT,
		       CLKCTRL_SAR_CLK_CTRL, 8,
		       0, CLK_SET_RATE_PARENT);

/* Array of all clocks provided by this provider */
static struct clk_hw_onecell_data s1a_hw_onecell_data = {
	.hws = {
#ifndef CONFIG_ARM
		[CLKID_FIXED_PLL_DCO]		= &fixed_pll_dco.hw,
#endif
		[CLKID_FIXED_PLL]		= &fixed_pll.hw,
		[CLKID_FCLK50M_DIV40]		= &fclk50m_div40.hw,
		[CLKID_FCLK50M]			= &fclk50m.hw,
		[CLKID_FCLK_DIV2_DIV]		= &fclk_div2_div.hw,
		[CLKID_FCLK_DIV2]		= &fclk_div2.hw,
		[CLKID_FCLK_DIV2P5_DIV]		= &fclk_div2p5_div.hw,
		[CLKID_FCLK_DIV2P5]		= &fclk_div2p5.hw,
		[CLKID_FCLK_DIV3_DIV]		= &fclk_div3_div.hw,
		[CLKID_FCLK_DIV3]		= &fclk_div3.hw,
		[CLKID_FCLK_DIV4_DIV]		= &fclk_div4_div.hw,
		[CLKID_FCLK_DIV4]		= &fclk_div4.hw,
		[CLKID_FCLK_DIV5_DIV]		= &fclk_div5_div.hw,
		[CLKID_FCLK_DIV5]		= &fclk_div5.hw,
		[CLKID_FCLK_DIV7_DIV]		= &fclk_div7_div.hw,
		[CLKID_FCLK_DIV7]		= &fclk_div7.hw,
#ifndef CONFIG_ARM
		[CLKID_SYS_PLL_DCO]		= &sys_pll_dco.hw,
#endif
		[CLKID_SYS_PLL]			= &sys_pll.hw,
#ifndef CONFIG_ARM
		[CLKID_GP0_PLL_DCO]		= &gp0_pll_dco.hw,
#endif
		[CLKID_GP0_PLL]			= &gp0_pll.hw,
#ifndef CONFIG_ARM
		[CLKID_HIFI_PLL_DCO]		= &hifi_pll_dco.hw,
#endif
		[CLKID_HIFI_PLL]		= &hifi_pll.hw,
#ifndef CONFIG_ARM
		[CLKID_HIFI1_PLL_DCO]		= &hifi1_pll_dco.hw,
#endif
		[CLKID_HIFI1_PLL]		= &hifi1_pll.hw,

		[CLKID_CPU_DYN_CLK]		= &cpu_dyn_clk.hw,
		[CLKID_CPU_CLK]			= &cpu_clk.hw,

		[CLKID_RTC_32K_CLKIN]		= &rtc_32k_clkin.hw,
		[CLKID_RTC_32K_DIV]		= &rtc_32k_div.hw,
		[CLKID_RTC_32K_MUX]		= &rtc_32k_sel.hw,
		[CLKID_RTC_32K]			= &rtc_32k.hw,
		[CLKID_RTC_CLK]			= &rtc_clk.hw,
		[CLKID_SYS_CLK_A_MUX]		= &sys_0_sel.hw,
		[CLKID_SYS_CLK_A_DIV]		= &sys_0_div.hw,
		[CLKID_SYS_CLK_A_GATE]		= &sys_0.hw,
		[CLKID_SYS_CLK_B_MUX]		= &sys_1_sel.hw,
		[CLKID_SYS_CLK_B_DIV]		= &sys_1_div.hw,
		[CLKID_SYS_CLK_B_GATE]		= &sys_1.hw,
		[CLKID_SYS_CLK]			= &sys_clk.hw,
		[CLKID_AXI_CLK_A_MUX]		= &axi_0_sel.hw,
		[CLKID_AXI_CLK_A_DIV]		= &axi_0_div.hw,
		[CLKID_AXI_CLK_A_GATE]		= &axi_0.hw,
		[CLKID_AXI_CLK_B_MUX]		= &axi_1_sel.hw,
		[CLKID_AXI_CLK_B_DIV]		= &axi_1_div.hw,
		[CLKID_AXI_CLK_B_GATE]		= &axi_1.hw,
		[CLKID_AXI_CLK]			= &axi_clk.hw,
		[CLKID_SYS_DEV_ARB]		= &sys_dev_arb.hw,
		[CLKID_SYS_DOS]			= &sys_dos.hw,
		[CLKID_SYS_ETH_PHY]		= &sys_eth_phy.hw,
		[CLKID_SYS_AOCPU]		= &sys_aocpu.hw,
		[CLKID_SYS_CEC]			= &sys_cec.hw,
		[CLKID_SYS_NIC]			= &sys_nic.hw,
		[CLKID_SYS_SD_EMMC_C]		= &sys_sd_emmc_c.hw,
		[CLKID_SYS_SC]			= &sys_smart_card.hw,
		[CLKID_SYS_ACODEC]		= &sys_acodec.hw,
		[CLKID_SYS_MSR_CLK]		= &sys_msr_clk.hw,
		[CLKID_SYS_IR_CTRL]		= &sys_ir_ctrl.hw,
		[CLKID_SYS_AUDIO]		= &sys_audio.hw,
		[CLKID_SYS_ETH_MAC]		= &sys_eth_mac.hw,
		[CLKID_SYS_UART_A]		= &sys_uart_a.hw,
		[CLKID_SYS_UART_B]		= &sys_uart_b.hw,
		[CLKID_SYS_TS_PLL]		= &sys_ts_pll.hw,
		[CLKID_SYS_G2ED]		= &sys_ge2d.hw,
		[CLKID_SYS_USB]			= &sys_usb.hw,
		[CLKID_SYS_I2C_M_A]		= &sys_i2c_m_a.hw,
		[CLKID_SYS_I2C_M_B]		= &sys_i2c_m_b.hw,
		[CLKID_SYS_I2C_M_C]		= &sys_i2c_m_c.hw,
		[CLKID_SYS_HDMITX]		= &sys_hdmitx.hw,
		[CLKID_SYS_H2MI20_AES]		= &sys_h2mi20_aes.hw,
		[CLKID_SYS_HDCP22]		= &sys_hdcp22.hw,
		[CLKID_SYS_MMC]			= &sys_mmc.hw,
		[CLKID_SYS_CPU_DBG]		= &sys_cpu_dbg.hw,
		[CLKID_SYS_VPU_INTR]		= &sys_vpu_intr.hw,
		[CLKID_SYS_DEMOD]		= &sys_demod.hw,
		[CLKID_SYS_SAR_ADC]		= &sys_sar_adc.hw,
		[CLKID_SYS_GIC]			= &sys_gic.hw,
		[CLKID_SYS_PWM_AB]		= &sys_pwm_ab.hw,
		[CLKID_SYS_PWM_CD]		= &sys_pwm_cd.hw,
		[CLKID_SYS_PAD_CTRL]		= &sys_pad_ctrl.hw,
		[CLKID_SYS_STARTUP]		= &sys_startup.hw,
		[CLKID_SYS_SECURETOP]		= &sys_securetop.hw,
		[CLKID_SYS_ROM]			= &sys_rom.hw,
		[CLKID_SYS_CLKTREE]		= &sys_clktree.hw,
		[CLKID_SYS_PWR_CTRL]		= &sys_pwr_ctrl.hw,
		[CLKID_SYS_SYS_CTRL]		= &sys_sys_ctrl.hw,
		[CLKID_SYS_CPU_CTRL]		= &sys_cpu_ctrl.hw,
		[CLKID_SYS_SRAM]		= &sys_sram.hw,
		[CLKID_SYS_MAILBOX]		= &sys_mailbox.hw,
		[CLKID_SYS_ANA_CTRL]		= &sys_ana_ctrl.hw,
		[CLKID_SYS_JTAG_CTRL]		= &sys_jtag_ctrl.hw,
		[CLKID_SYS_IRQ_CTRL]		= &sys_irq_ctrl.hw,
		[CLKID_SYS_RESET_CTRL]		= &sys_reset_ctrl.hw,
		[CLKID_SYS_CAPU]		= &sys_capu.hw,
		[CLKID_AXI_SYS_NIC]		= &axi_sys_nic.hw,
		[CLKID_AXI_SRAM]		= &axi_sram.hw,
		[CLKID_AXI_DEV0_DMC]		= &axi_dev0_mmc.hw,
		[CLKID_PWM_A_MUX]		= &pwm_a_sel.hw,
		[CLKID_PWM_A_DIV]		= &pwm_a_div.hw,
		[CLKID_PWM_A]			= &pwm_a.hw,
		[CLKID_PWM_B_MUX]		= &pwm_b_sel.hw,
		[CLKID_PWM_B_DIV]		= &pwm_b_div.hw,
		[CLKID_PWM_B]			= &pwm_b.hw,
		[CLKID_PWM_C_MUX]		= &pwm_c_sel.hw,
		[CLKID_PWM_C_DIV]		= &pwm_c_div.hw,
		[CLKID_PWM_C]			= &pwm_c.hw,
		[CLKID_PWM_D_MUX]		= &pwm_d_sel.hw,
		[CLKID_PWM_D_DIV]		= &pwm_d_div.hw,
		[CLKID_PWM_D]			= &pwm_d.hw,
		[CLKID_SD_EMMC_C_MUX]		= &sd_emmc_c_sel.hw,
		[CLKID_SD_EMMC_C_DIV]		= &sd_emmc_c_div.hw,
		[CLKID_SD_EMMC_C]		= &sd_emmc_c.hw,
		[CLKID_ETH_125M_DIV]		= &eth_125m_div.hw,
		[CLKID_ETH_125M]		= &eth_125m.hw,
		[CLKID_ETH_RMII_DIV]		= &eth_rmii_div.hw,
		[CLKID_ETH_RMII]		= &eth_rmii.hw,
		[CLKID_TS_DIV]			= &ts_div.hw,
		[CLKID_TS]			= &ts.hw,
		[CLKID_SC_MUX]			= &sc_sel.hw,
		[CLKID_SC_DIV]			= &sc_div.hw,
		[CLKID_SC]			= &sc.hw,
		[CLKID_CECB_32K_CLKIN]		= &cecb_32k_clkin.hw,
		[CLKID_CECB_32K_DIV]		= &cecb_32k_div.hw,
		[CLKID_CECB_32K_MUX]		= &cecb_32k_sel.hw,
		[CLKID_CECB_CLK_SEL]		= &cecb_clk_sel.hw,
		[CLKID_CECB_CLK]		= &cecb_clk.hw,
		[CLKID_HDMITX_SYS_MUX]		= &hdmitx_sys_sel.hw,
		[CLKID_HDMITX_SYS_DIV]		= &hdmitx_sys_div.hw,
		[CLKID_HDMITX_SYS]		= &hdmitx_sys.hw,
		[CLKID_HDMITX_PRIF_MUX]		= &hdmitx_prif_sel.hw,
		[CLKID_HDMITX_PRIF_DIV]		= &hdmitx_prif_div.hw,
		[CLKID_HDMITX_PRIF]		= &hdmitx_prif.hw,
		[CLKID_HDMITX_200M_MUX]		= &hdmitx_200m_sel.hw,
		[CLKID_HDMITX_200M_DIV]		= &hdmitx_200m_div.hw,
		[CLKID_HDMITX_200M]		= &hdmitx_200m.hw,
		[CLKID_HDMITX_AUD_MUX]		= &hdmitx_aud_sel.hw,
		[CLKID_HDMITX_AUD_DIV]		= &hdmitx_aud_div.hw,
		[CLKID_HDMITX_AUD]		= &hdmitx_aud.hw,
		[CLKID_VCLK_MUX]		= &vclk_sel.hw,
		[CLKID_VCLK_INPUT]		= &vclk_input.hw,
		[CLKID_VCLK]			= &vclk.hw,
		[CLKID_VCLK_DIV]		= &vclk_div.hw,
		[CLKID_VCLK_DIV1]		= &vclk_div1.hw,
		[CLKID_VCLK_DIV2_EN]		= &vclk_div2_gate.hw,
		[CLKID_VCLK_DIV2]		= &vclk_div2.hw,
		[CLKID_VCLK_DIV4_EN]		= &vclk_div4_gate.hw,
		[CLKID_VCLK_DIV4]		= &vclk_div4.hw,
		[CLKID_VCLK_DIV6_EN]		= &vclk_div6_gate.hw,
		[CLKID_VCLK_DIV6]		= &vclk_div6.hw,
		[CLKID_VCLK_DIV12_EN]		= &vclk_div12_gate.hw,
		[CLKID_VCLK_DIV12]		= &vclk_div12.hw,
		[CLKID_VCLK2_MUX]		= &vclk2_sel.hw,
		[CLKID_VCLK2_INPUT]		= &vclk2_input.hw,
		[CLKID_VCLK2]			= &vclk2.hw,
		[CLKID_VCLK2_DIV]		= &vclk2_div.hw,
		[CLKID_VCLK2_DIV1]		= &vclk2_div1.hw,
		[CLKID_VCLK2_DIV2_EN]		= &vclk2_div2_gate.hw,
		[CLKID_VCLK2_DIV2]		= &vclk2_div2.hw,
		[CLKID_VCLK2_DIV4_EN]		= &vclk2_div4_gate.hw,
		[CLKID_VCLK2_DIV4]		= &vclk2_div4.hw,
		[CLKID_VCLK2_DIV6_EN]		= &vclk2_div6_gate.hw,
		[CLKID_VCLK2_DIV6]		= &vclk2_div6.hw,
		[CLKID_VCLK2_DIV12_EN]		= &vclk2_div12_gate.hw,
		[CLKID_VCLK2_DIV12]		= &vclk2_div12.hw,
		[CLKID_HDMITX_PIXEL_MUX]	= &hdmitx_pixel_sel.hw,
		[CLKID_HDMITX_PIXEL]		= &hdmitx_pixel.hw,
		[CLKID_HDMITX_FE_MUX]		= &hdmitx_fe_sel.hw,
		[CLKID_HDMITX_FE]		= &hdmitx_fe.hw,
		[CLKID_ENCI_MUX]		= &enci_sel.hw,
		[CLKID_ENCI]			= &enci.hw,
		[CLKID_ENCP_MUX]		= &encp_sel.hw,
		[CLKID_ENCP]			= &encp.hw,
		[CLKID_ENCL_MUX]		= &encl_sel.hw,
		[CLKID_ENCL]			= &encl.hw,
		[CLKID_VDAC_MUX]		= &vdac_sel.hw,
		[CLKID_VDAC]			= &vdac.hw,
		[CLKID_VPU_0_MUX]		= &vpu_0_sel.hw,
		[CLKID_VPU_0_DIV]		= &vpu_0_div.hw,
		[CLKID_VPU_0]			= &vpu_0.hw,
		[CLKID_VPU_1_MUX]		= &vpu_1_sel.hw,
		[CLKID_VPU_1_DIV]		= &vpu_1_div.hw,
		[CLKID_VPU_1]			= &vpu_1.hw,
		[CLKID_VPU]			= &vpu.hw,
		[CLKID_VPU_CLKB_TMP_MUX]	= &vpu_clkb_temp_sel.hw,
		[CLKID_VPU_CLKB_TMP_DIV]	= &vpu_clkb_temp_div.hw,
		[CLKID_VPU_CLKB_TMP]		= &vpu_clkb_temp.hw,
		[CLKID_VPU_CLKB_DIV]		= &vpu_clkb_div.hw,
		[CLKID_VPU_CLKB]		= &vpu_clkb.hw,
		[CLKID_VPU_CLKC_0_MUX]		= &vpu_clkc_0_sel.hw,
		[CLKID_VPU_CLKC_0_DIV]		= &vpu_clkc_0_div.hw,
		[CLKID_VPU_CLKC_0]		= &vpu_clkc_0.hw,
		[CLKID_VPU_CLKC_1_MUX]		= &vpu_clkc_1_sel.hw,
		[CLKID_VPU_CLKC_1_DIV]		= &vpu_clkc_1_div.hw,
		[CLKID_VPU_CLKC_1]		= &vpu_clkc_1.hw,
		[CLKID_VPU_CLKC]		= &vpu_clkc.hw,
		[CLKID_VAPB_0_MUX]		= &vapb_0_sel.hw,
		[CLKID_VAPB_0_DIV]		= &vapb_0_div.hw,
		[CLKID_VAPB_0]			= &vapb_0.hw,
		[CLKID_VAPB_1_MUX]		= &vapb_1_sel.hw,
		[CLKID_VAPB_1_DIV]		= &vapb_1_div.hw,
		[CLKID_VAPB_1]			= &vapb_1.hw,
		[CLKID_VAPB]			= &vapb.hw,
		[CLKID_GE2D]			= &ge2d.hw,
		[CLKID_VDIN_MEAS_MUX]		= &vdin_meas_sel.hw,
		[CLKID_VDIN_MEAS_DIV]		= &vdin_meas_div.hw,
		[CLKID_VDIN_MEAS]		= &vdin_meas.hw,
		[CLKID_VID_LOCK_MUX]		= &vid_lock_sel.hw,
		[CLKID_VID_LOCK_DIV]		= &vid_lock_div.hw,
		[CLKID_VID_LOCK]		= &vid_lock.hw,
		[CLKID_VDEC_0_MUX]		= &vdec_0_sel.hw,
		[CLKID_VDEC_0_DIV]		= &vdec_0_div.hw,
		[CLKID_VDEC_0]			= &vdec_0.hw,
		[CLKID_VDEC_1_MUX]		= &vdec_1_sel.hw,
		[CLKID_VDEC_1_DIV]		= &vdec_1_div.hw,
		[CLKID_VDEC_1]			= &vdec_1.hw,
		[CLKID_VDEC]			= &vdec.hw,
		[CLKID_HEVCF_0_MUX]		= &hevcf_0_sel.hw,
		[CLKID_HEVCF_0_DIV]		= &hevcf_0_div.hw,
		[CLKID_HEVCF_0]			= &hevcf_0.hw,
		[CLKID_HEVCF_1_MUX]		= &hevcf_1_sel.hw,
		[CLKID_HEVCF_1_DIV]		= &hevcf_1_div.hw,
		[CLKID_HEVCF_1]			= &hevcf_1.hw,
		[CLKID_HEVCF]			= &hevcf.hw,
		[CLKID_DEMOD_CORE_MUX]		= &demod_core_sel.hw,
		[CLKID_DEMOD_CORE_DIV]		= &demod_core_div.hw,
		[CLKID_DEMOD_CORE]		= &demod_core.hw,
		[CLKID_DEMOD_CORE_T2_MUX]	= &demod_core_t2_sel.hw,
		[CLKID_DEMOD_CORE_T2_DIV]	= &demod_core_t2_div.hw,
		[CLKID_DEMOD_CORE_T2]		= &demod_core_t2.hw,
		[CLKID_ADC_EXTCLK_IN_MUX]	= &adc_extclk_in_sel.hw,
		[CLKID_ADC_EXTCLK_IN_DIV]	= &adc_extclk_in_div.hw,
		[CLKID_ADC_EXTCLK_IN]		= &adc_extclk_in.hw,
		[CLKID_DEMOD_32K_CLKIN]		= &demod_32k_clkin.hw,
		[CLKID_DEMOD_32K_DIV]		= &demod_32k_div.hw,
		[CLKID_DEMOD_32K_DIV_MUX]	= &demod_32k_sel.hw,
		[CLKID_DEMOD_CLK]		= &demod_32k.hw,
		[CLKID_12_24M_IN]		= &clk_12_24m_in.hw,
		[CLKID_12_24M]			= &clk_12_24m.hw,
		[CLKID_SAR_ADC_MUX]		= &sar_adc_sel.hw,
		[CLKID_SAR_ADC_DIV]		= &sar_adc_div.hw,
		[CLKID_SAR_ADC]			= &sar_adc.hw,

		[NR_CLKS]			= NULL
		},
	.num = NR_CLKS,
};

/* Convenience table to populate regmap in .probe */
static struct clk_regmap *const s1a_clk_regmaps[] = {
	&rtc_32k_clkin,
	&rtc_32k_div,
	&rtc_32k_sel,
	&rtc_32k,
	&rtc_clk,
	&sys_0_sel,
	&sys_0_div,
	&sys_0,
	&sys_1_sel,
	&sys_1_div,
	&sys_1,
	&sys_clk,
	&axi_0_sel,
	&axi_0_div,
	&axi_0,
	&axi_1_sel,
	&axi_1_div,
	&axi_1,
	&axi_clk,
	&sys_dev_arb,
	&sys_dos,
	&sys_eth_phy,
	&sys_aocpu,
	&sys_cec,
	&sys_nic,
	&sys_sd_emmc_c,
	&sys_smart_card,
	&sys_acodec,
	&sys_msr_clk,
	&sys_ir_ctrl,
	&sys_audio,
	&sys_eth_mac,
	&sys_uart_a,
	&sys_uart_b,
	&sys_ts_pll,
	&sys_ge2d,
	&sys_usb,
	&sys_i2c_m_a,
	&sys_i2c_m_b,
	&sys_i2c_m_c,
	&sys_hdmitx,
	&sys_h2mi20_aes,
	&sys_hdcp22,
	&sys_mmc,
	&sys_cpu_dbg,
	&sys_vpu_intr,
	&sys_demod,
	&sys_sar_adc,
	&sys_gic,
	&sys_pwm_ab,
	&sys_pwm_cd,
	&sys_pad_ctrl,
	&sys_startup,
	&sys_securetop,
	&sys_rom,
	&sys_clktree,
	&sys_pwr_ctrl,
	&sys_sys_ctrl,
	&sys_cpu_ctrl,
	&sys_sram,
	&sys_mailbox,
	&sys_ana_ctrl,
	&sys_jtag_ctrl,
	&sys_irq_ctrl,
	&sys_reset_ctrl,
	&sys_capu,
	&axi_sys_nic,
	&axi_sram,
	&axi_dev0_mmc,
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
	&sd_emmc_c_sel,
	&sd_emmc_c_div,
	&sd_emmc_c,
	&eth_125m,
	&eth_rmii_div,
	&eth_rmii,
	&ts_div,
	&ts,
	&sc_sel,
	&sc_div,
	&sc,
	&cecb_32k_clkin,
	&cecb_32k_div,
	&cecb_32k_sel,
	&cecb_clk_sel,
	&cecb_clk,
	&hdmitx_sys_sel,
	&hdmitx_sys_div,
	&hdmitx_sys,
	&hdmitx_prif_sel,
	&hdmitx_prif_div,
	&hdmitx_prif,
	&hdmitx_200m_sel,
	&hdmitx_200m_div,
	&hdmitx_200m,
	&hdmitx_aud_sel,
	&hdmitx_aud_div,
	&hdmitx_aud,
	&vclk_sel,
	&vclk_input,
	&vclk_div,
	&vclk,
	&vclk_div1,
	&vclk_div2_gate,
	&vclk_div4_gate,
	&vclk_div6_gate,
	&vclk_div12_gate,
	&vclk2_sel,
	&vclk2_input,
	&vclk2_div,
	&vclk2,
	&vclk2_div1,
	&vclk2_div2_gate,
	&vclk2_div4_gate,
	&vclk2_div6_gate,
	&vclk2_div12_gate,
	&hdmitx_pixel_sel,
	&hdmitx_pixel,
	&hdmitx_fe_sel,
	&hdmitx_fe,
	&enci_sel,
	&enci,
	&encp_sel,
	&encp,
	&encl_sel,
	&encl,
	&vdac_sel,
	&vdac,
	&vpu_0_sel,
	&vpu_0_div,
	&vpu_0,
	&vpu_1_sel,
	&vpu_1_div,
	&vpu_1,
	&vpu,
	&vpu_clkb_temp_sel,
	&vpu_clkb_temp_div,
	&vpu_clkb_temp,
	&vpu_clkb_div,
	&vpu_clkb,
	&vpu_clkc_0_sel,
	&vpu_clkc_0_div,
	&vpu_clkc_0,
	&vpu_clkc_1_sel,
	&vpu_clkc_1_div,
	&vpu_clkc_1,
	&vpu_clkc,
	&vapb_0_sel,
	&vapb_0_div,
	&vapb_0,
	&vapb_1_sel,
	&vapb_1_div,
	&vapb_1,
	&vapb,
	&ge2d,
	&vdin_meas_sel,
	&vdin_meas_div,
	&vdin_meas,
	&vid_lock_sel,
	&vid_lock_div,
	&vid_lock,
	&vdec_0_sel,
	&vdec_0_div,
	&vdec_0,
	&vdec_1_sel,
	&vdec_1_div,
	&vdec_1,
	&vdec,
	&hevcf_0_sel,
	&hevcf_0_div,
	&hevcf_0,
	&hevcf_1_sel,
	&hevcf_1_div,
	&hevcf_1,
	&hevcf,
	&demod_core_sel,
	&demod_core_div,
	&demod_core,
	&demod_core_t2_sel,
	&demod_core_t2_div,
	&demod_core_t2,
	&adc_extclk_in_sel,
	&adc_extclk_in_div,
	&adc_extclk_in,
	&demod_32k_clkin,
	&demod_32k_div,
	&demod_32k_sel,
	&demod_32k,
	&clk_12_24m_in,
	&clk_12_24m,
	&sar_adc_sel,
	&sar_adc_div,
	&sar_adc
};

static struct clk_regmap *const s1a_cpu_clk_regmaps[] = {
	&cpu_dyn_clk,
	&cpu_clk
};

static struct clk_regmap *const s1a_pll_regmaps[] = {
#ifndef CONFIG_ARM
	&fixed_pll_dco,
#endif
	&fixed_pll,
	&fclk50m,
	&fclk_div2,
	&fclk_div2p5,
	&fclk_div3,
	&fclk_div4,
	&fclk_div5,
	&fclk_div7,
#ifndef CONFIG_ARM
	&sys_pll_dco,
#endif
	&sys_pll,
#ifndef CONFIG_ARM
	&gp0_pll_dco,
#endif
	&gp0_pll,
#ifndef CONFIG_ARM
	&hifi_pll_dco,
#endif
	&hifi_pll,
#ifndef CONFIG_ARM
	&hifi1_pll_dco,
#endif
	&hifi1_pll
};

struct sys_pll_nb_data {
	struct notifier_block nb;
	struct clk_hw *sys_pll;
	struct clk_hw *cpu_clk;
	struct clk_hw *cpu_clk_dyn;
};

static int sys_pll_notifier_cb(struct notifier_block *nb,
			       unsigned long event, void *data)
{
	struct sys_pll_nb_data *nb_data =
		container_of(nb, struct sys_pll_nb_data, nb);

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
		if (clk_set_rate(nb_data->cpu_clk_dyn->clk, 1000000000))
			pr_err("%s in %d\n", __func__, __LINE__);
		/* Configure cpu_clk to use cpu_clk_dyn */
		clk_hw_set_parent(nb_data->cpu_clk,
				  nb_data->cpu_clk_dyn);

		/*
		 * Now, cpu_clk uses the dyn path
		 * cpu_clk
		 *    \- cpu_clk_dyn
		 *          \- cpu_clk_dynX
		 *                \- cpu_clk_dynX_mux
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

static struct sys_pll_nb_data sys_pll_nb_data = {
	.sys_pll = &sys_pll.hw,
	.cpu_clk = &cpu_clk.hw,
	.cpu_clk_dyn = &cpu_dyn_clk.hw,
	.nb.notifier_call = sys_pll_notifier_cb,
};

static int meson_s1a_dvfs_setup(struct platform_device *pdev)
{
	int ret;

	/*for pxp*/

	/* Setup clock notifier for sys_pll */
	ret = clk_notifier_register(sys_pll.hw.clk,
				    &sys_pll_nb_data.nb);
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

static struct regmap *s1a_regmap_resource(struct device *dev, char *name)
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

static int meson_s1a_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct regmap *basic_map;
	struct regmap *pll_map;
	struct regmap *cpu_clk_map;
	struct clk *clk;
	int ret, i;

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

	basic_map = s1a_regmap_resource(dev, "basic");
	if (IS_ERR(basic_map)) {
		dev_err(dev, "basic clk registers not found\n");
		return PTR_ERR(basic_map);
	}

	pll_map = s1a_regmap_resource(dev, "pll");
	if (IS_ERR(pll_map)) {
		dev_err(dev, "pll clk registers not found\n");
		return PTR_ERR(pll_map);
	}

	cpu_clk_map = s1a_regmap_resource(dev, "cpu_clk");
	if (IS_ERR(cpu_clk_map)) {
		dev_err(dev, "cpu clk registers not found\n");
		return PTR_ERR(cpu_clk_map);
	}

	/* Populate regmap for the regmap backed clocks */
	for (i = 0; i < ARRAY_SIZE(s1a_clk_regmaps); i++)
		s1a_clk_regmaps[i]->map = basic_map;

	for (i = 0; i < ARRAY_SIZE(s1a_cpu_clk_regmaps); i++)
		s1a_cpu_clk_regmaps[i]->map = cpu_clk_map;

	for (i = 0; i < ARRAY_SIZE(s1a_pll_regmaps); i++)
		s1a_pll_regmaps[i]->map = pll_map;

	for (i = 0; i < s1a_hw_onecell_data.num; i++) {
		/* array might be sparse */
		if (!s1a_hw_onecell_data.hws[i])
			continue;
		/*
		 *dev_err(dev, "register %d  %s\n",i,
		 *        clk_hw_get_name(hw_onecell_data->hws[i]));
		 */
		ret = devm_clk_hw_register(dev, s1a_hw_onecell_data.hws[i]);
		if (ret) {
			dev_err(dev, "Clock registration failed\n");
			return ret;
		}

#ifdef CONFIG_AMLOGIC_CLK_DEBUG
		ret = devm_clk_hw_register_clkdev(dev, s1a_hw_onecell_data.hws[i],
						  NULL,
						  clk_hw_get_name(s1a_hw_onecell_data.hws[i]));
		if (ret < 0) {
			dev_err(dev, "Failed to clkdev register: %d\n", ret);
			return ret;
		}
#endif
	}

	meson_s1a_dvfs_setup(pdev);

	return devm_of_clk_add_hw_provider(dev, of_clk_hw_onecell_get,
					   (void *)&s1a_hw_onecell_data);
}

static const struct of_device_id clkc_match_table[] = {
	{
		.compatible = "amlogic,s1a-clkc",
		.data = &s1a_hw_onecell_data
	},
	{}
};

static struct platform_driver s1a_driver = {
	.probe		= meson_s1a_probe,
	.driver		= {
		.name	= "s1a-clkc",
		.of_match_table = clkc_match_table,
	},
};

#ifndef CONFIG_AMLOGIC_MODIFY
builtin_platform_driver(s1a_driver);
#else
#ifndef MODULE
static int __init s1a_clkc_init(void)
{
	return platform_driver_register(&s1a_driver);
}
arch_initcall_sync(s1a_clkc_init);
#else
int __init meson_s1a_clkc_init(void)
{
	return platform_driver_register(&s1a_driver);
}
module_init(meson_s1a_clkc_init);
#endif
#endif

MODULE_LICENSE("GPL v2");
