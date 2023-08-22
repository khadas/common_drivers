// SPDX-License-Identifier: GPL-2.0+
/*
 * Amlogic Meson-C1 Clock Controller Driver
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
#include "c1.h"

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

#define __MESON_CLK_DYN(_name, _offset, _smc_id, _secid_dyn,		\
				_secid_dyn_rd, _table, _table_cnt,	\
				_ops, _pname, _pdata, _phw, _pnub,	\
				_iflags)				\
static struct clk_regmap _name = {					\
	.data = &(struct meson_clk_cpu_dyn_data){			\
		.table = _table,					\
		.offset = _offset,					\
		.smc_id = _smc_id,					\
		.table_cnt = _table_cnt,				\
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

#define MESON_CLK_DYN_RW(_name, _offset, _smc_id, _secid_dyn,		\
			 _secid_dyn_rd, _table, _pdata, _iflags)	\
	__MESON_CLK_DYN(_name, _offset, _smc_id, _secid_dyn,		\
			 _secid_dyn_rd, _table, ARRAY_SIZE(_table),	\
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
			 _init_reg, _range, _table,			\
			 _dflags, _pdata, _iflags,			\
			 _od_reg, _od_shift, _od_width)			\
	__MESON_CLK_PLL(_name, _en_reg, _en_shift, _en_width,		\
			_m_reg, _m_shift, _m_width,			\
			_f_reg, _f_shift, _f_width,			\
			_n_reg, _n_shift, _n_width,			\
			_l_reg, _l_shift, _l_width,			\
			_r_reg, _r_shift, _r_width,			\
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
			 _range, _table,				\
			 _dflags, _pdata, _iflags,			\
			 _od_reg, _od_shift, _od_width)			\
	__MESON_CLK_PLL(_name, _en_reg, _en_shift, _en_width,		\
			_m_reg, _m_shift, _m_width,			\
			_f_reg, _f_shift, _f_width,			\
			_n_reg, _n_shift, _n_width,			\
			_l_reg, _l_shift, _l_width,			\
			_r_reg, _r_shift, _r_width,			\
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
			&clk_regmap_divider_ro_ops, CLK_SET_RATE_PARENT)
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
	__MESON_CLK_COMPOSITE(_cname ## _mux, _m_reg, _m_mask, _m_shift,\
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
	__MESON_CLK_COMPOSITE(_cname ## _mux, _m_reg, _m_mask, _m_shift,\
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
		 ANACTRL_FIXPLL_CTRL1, 0,  19,  /* frac */
		 ANACTRL_FIXPLL_CTRL0, 10, 5,  /* n */
		 ANACTRL_FIXPLL_STS, 31, 1,  /* lock */
		 ANACTRL_FIXPLL_CTRL0, 29, 1,  /* rst */
		 NULL, NULL,
		 0, &pll_dco_parent, 0,
		 ANACTRL_FIXPLL_CTRL0, 16, 2
#ifdef CONFIG_ARM
		 );
#else
		 , NULL, CLK_DIVIDER_POWER_OF_TWO);
#endif

MESON_CLK_FIXED_FACTOR(fclk_div2_div, 1, 2, &fixed_pll.hw, 0);
MESON_CLK_GATE_RO(fclk_div2, ANACTRL_FIXPLL_CTRL1, 24, 0, &fclk_div2_div.hw, 0);
MESON_CLK_FIXED_FACTOR(fclk_div2p5_div, 2, 5, &fixed_pll.hw, 0);
MESON_CLK_GATE_RO(fclk_div2p5, ANACTRL_FIXPLL_CTRL1, 25, 0, &fclk_div2p5_div.hw, 0);
MESON_CLK_FIXED_FACTOR(fclk_div3_div, 1, 3, &fixed_pll.hw, 0);
MESON_CLK_GATE_RO(fclk_div3, ANACTRL_FIXPLL_CTRL1, 20, 0, &fclk_div3_div.hw, 0);
MESON_CLK_FIXED_FACTOR(fclk_div4_div, 1, 4, &fixed_pll.hw, 0);
MESON_CLK_GATE_RO(fclk_div4, ANACTRL_FIXPLL_CTRL1, 21, 0, &fclk_div4_div.hw, 0);
MESON_CLK_FIXED_FACTOR(fclk_div5_div, 1, 5, &fixed_pll.hw, 0);
MESON_CLK_GATE_RO(fclk_div5, ANACTRL_FIXPLL_CTRL1, 22, 0, &fclk_div5_div.hw, 0);
MESON_CLK_FIXED_FACTOR(fclk_div7_div, 1, 7, &fixed_pll.hw, 0);
MESON_CLK_GATE_RO(fclk_div7, ANACTRL_FIXPLL_CTRL1, 23, 0, &fclk_div7_div.hw, 0);
MESON_CLK_FIXED_FACTOR(fclk_div40_div, 1, 40, &fixed_pll.hw, 0);
MESON_CLK_GATE_RO(fclk_50m, ANACTRL_FIXPLL_CTRL3, 5, 0, &fclk_div40_div.hw, 0);

/*
 * HIFI PLL and SYS PLL rang from 768M to 1536M
 * the PLL parameter table is for hifi/sys pll.
 * Adtional, there is no OD in A1 PLL.
 */
#ifdef CONFIG_ARM
static const struct pll_params_table sys_pll_params_table[] = {
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
static const struct pll_params_table sys_pll_params_table[] = {
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
static const struct reg_sequence sys_init_regs[] = {
	{ .reg = ANACTRL_SYSPLL_CTRL1,	.def = 0x01800000 },
	{ .reg = ANACTRL_SYSPLL_CTRL2,	.def = 0x00001100 },
	{ .reg = ANACTRL_SYSPLL_CTRL3,	.def = 0x10022300 },
	{ .reg = ANACTRL_SYSPLL_CTRL4,	.def = 0x00300000 },
	{ .reg = ANACTRL_SYSPLL_CTRL0,  .def = 0x01f18000 },
	{ .reg = ANACTRL_SYSPLL_CTRL0,  .def = 0x11f18000, .delay_us = 10 },
	{ .reg = ANACTRL_SYSPLL_CTRL0,	.def = 0x15f18000, .delay_us = 40 },
	{ .reg = ANACTRL_SYSPLL_CTRL2,	.def = 0x00001120, .delay_us = 10 },
};

/*
 * Warning: The sys_pll output for c1 does not have a divider od. In order to be
 * compatible with the current pll software architecture, an invalid bit is
 * specified as od (this bit is always 0)
 */
MESON_CLK_PLL_RW(sys_pll, ANACTRL_SYSPLL_CTRL0, 28, 1,  /* en */
		 ANACTRL_SYSPLL_CTRL0, 0, 8,  /* m */
		 0, 0, 0,  /* frac */
		 ANACTRL_SYSPLL_CTRL0, 10, 5,  /* n */
		 ANACTRL_SYSPLL_STS, 31, 1,  /* lock */
		 ANACTRL_SYSPLL_CTRL0, 29, 1,  /* rst */
		 sys_init_regs, NULL, sys_pll_params_table,
		 CLK_MESON_PLL_IGNORE_INIT,
		 &pll_dco_parent, 0,
		 ANACTRL_SYSPLL_CTRL0, 8, 1  /*  */
#ifdef CONFIG_ARM
		 );
#else
		 , NULL, CLK_DIVIDER_POWER_OF_TWO);
#endif

#ifdef CONFIG_ARM
static const struct pll_params_table gp_pll_params_table[] = {
	PLL_PARAMS(99, 2, 0), /* DCO = 1188M */
	{ /* sentinel */ }
};
#else
static const struct pll_params_table gp_pll_params_table[] = {
	PLL_PARAMS(99, 2), /* DCO = 1188M */
	{ /* sentinel */ }
};
#endif

static const struct reg_sequence gp_init_regs[] = {
	{ .reg = ANACTRL_GPPLL_CTRL1,	.def = 0x01800000 },
	{ .reg = ANACTRL_GPPLL_CTRL2,	.def = 0x00001100 },
	{ .reg = ANACTRL_GPPLL_CTRL3,	.def = 0x10022300 },
	{ .reg = ANACTRL_GPPLL_CTRL4,	.def = 0x00300000 },
	{ .reg = ANACTRL_GPPLL_CTRL5,   .def = 0x00080000 },
	{ .reg = ANACTRL_GPPLL_CTRL0,   .def = 0x01f18000 },
	{ .reg = ANACTRL_GPPLL_CTRL0,   .def = 0x11f18000, .delay_us = 10 },
	{ .reg = ANACTRL_GPPLL_CTRL0,	.def = 0x15f18000, .delay_us = 40 },
	{ .reg = ANACTRL_GPPLL_CTRL2,	.def = 0x00001120, .delay_us = 10 },
//	{ .reg = ANACTRL_GPPLL_CTRL2,	.def = 0x00001100 },
};

/*
 * Warning: The gp_pll output for c1 does not have a divider od. In order to be
 * compatible with the current pll software architecture, an invalid bit is
 * specified as od (this bit is always 0)
 */
MESON_CLK_PLL_RW(gp_pll, ANACTRL_GPPLL_CTRL0, 28, 1,  /* en */
		 ANACTRL_GPPLL_CTRL0, 0, 8,  /* m */
		 ANACTRL_GPPLL_CTRL1, 0, 19,  /* frac */
		 ANACTRL_GPPLL_CTRL0, 10, 5,  /* n */
		 ANACTRL_GPPLL_STS, 31, 1,  /* lock */
		 ANACTRL_GPPLL_CTRL0, 29, 1,  /* rst */
		 gp_init_regs, NULL, gp_pll_params_table,
		 CLK_MESON_PLL_IGNORE_INIT,
		 &pll_dco_parent, 0,
		 ANACTRL_GPPLL_CTRL0, 8, 1
#ifdef CONFIG_ARM
		 );
#else
		 , NULL, CLK_DIVIDER_POWER_OF_TWO);
#endif

MESON_CLK_FIXED_FACTOR(gp_pll_div1_fix_div2, 1, 2, &gp_pll.hw, 0);
MESON_CLK_DIV_RW(gp_pll_div1, ANACTRL_GPPLL_CTRL5, 0, 5, NULL,
		 CLK_DIVIDER_ONE_BASED,
		 &gp_pll_div1_fix_div2.hw,
		 CLK_SET_RATE_PARENT);
MESON_CLK_GATE_RW(gp_pll_div1_gate, FCLK_DIV1_SEL, 7, 0, &gp_pll_div1.hw,
		  CLK_SET_RATE_PARENT);

MESON_CLK_FIXED_FACTOR(gp_pll_div2_fix_div2, 1, 2, &gp_pll.hw, 0);
MESON_CLK_DIV_RW(gp_pll_div2, ANACTRL_GPPLL_CTRL5, 8, 5, NULL,
		 CLK_DIVIDER_ONE_BASED,
		 &gp_pll_div2_fix_div2.hw,
		 CLK_SET_RATE_PARENT);
MESON_CLK_GATE_RW(gp_pll_div2_gate, FCLK_DIV1_SEL, 15, 0, &gp_pll_div2.hw,
		  CLK_SET_RATE_PARENT);

MESON_CLK_DIV_RW(gp_pll_div3, ANACTRL_GPPLL_CTRL5, 16, 2, NULL,
		 CLK_DIVIDER_ONE_BASED | CLK_DIVIDER_ALLOW_ZERO,
		 &gp_pll.hw,
		 CLK_SET_RATE_PARENT);
MESON_CLK_GATE_RW(gp_pll_div3_gate, FCLK_DIV1_SEL, 19, 0, &gp_pll_div2.hw,
		  CLK_SET_RATE_PARENT);

MESON_CLK_GATE_RW(fixed_pll_div2_gate, FCLK_DIV1_SEL, 1, 0, &fixed_pll.hw,
		  CLK_SET_RATE_PARENT);

/* 614.4M */
static const struct reg_sequence hifi_init_regs[] = {
	{ .reg = ANACTRL_HIFIPLL_CTRL1,	.def = 0x01800000 },
	{ .reg = ANACTRL_HIFIPLL_CTRL2,	.def = 0x00001120 },
	{ .reg = ANACTRL_HIFIPLL_CTRL3,	.def = 0x10022200 },
	{ .reg = ANACTRL_HIFIPLL_CTRL4,	.def = 0x00301000 },
	{ .reg = ANACTRL_HIFIPLL_CTRL0, .def = 0x01f19480 },
	{ .reg = ANACTRL_HIFIPLL_CTRL0, .def = 0x11f19480, .delay_us = 10 },
	{ .reg = ANACTRL_HIFIPLL_CTRL0,	.def = 0x15f11480, .delay_us = 40 },
	{ .reg = ANACTRL_HIFIPLL_CTRL2,	.def = 0x00001160 },
	{ .reg = ANACTRL_HIFIPLL_CTRL2,	.def = 0x00001120 },
};

/* 1467.648M */
static const struct reg_sequence hifi_init_regs1[] = {
	{ .reg = ANACTRL_HIFIPLL_CTRL1,	.def = 0x0f904dd3 },
	{ .reg = ANACTRL_HIFIPLL_CTRL2,	.def = 0x00001120 },
	{ .reg = ANACTRL_HIFIPLL_CTRL3,	.def = 0x100a1100 },
	{ .reg = ANACTRL_HIFIPLL_CTRL4,	.def = 0x00301000 },
	{ .reg = ANACTRL_HIFIPLL_CTRL0, .def = 0x01f1843d },
	{ .reg = ANACTRL_HIFIPLL_CTRL0, .def = 0x11f1843d, .delay_us = 10 },
	{ .reg = ANACTRL_HIFIPLL_CTRL0,	.def = 0x15f1843d, .delay_us = 40 },
	{ .reg = ANACTRL_HIFIPLL_CTRL2,	.def = 0x00001160 },
	{ .reg = ANACTRL_HIFIPLL_CTRL2,	.def = 0x00001120 },
};

#ifdef CONFIG_ARM
static const struct pll_params_table hifi_pll_params_table[] = {
	{ .m = 128, .n = 4, .od = 0, .regs = hifi_init_regs,
	  .regs_count = ARRAY_SIZE(hifi_init_regs)},  /*DCO = 768M */
	{ .m = 213, .n = 8, .od = 0, .regs = hifi_init_regs,
	  .regs_count = ARRAY_SIZE(hifi_init_regs)},  /*DCO = 639M */
	{ .m = 128, .n = 5, .od = 0, .regs = hifi_init_regs,
	  .regs_count = ARRAY_SIZE(hifi_init_regs)},  /*DCO = 614.4M */
	{ .m = 61, .n = 1, .od = 0, .regs = hifi_init_regs1,
	  .regs_count = ARRAY_SIZE(hifi_init_regs1)},  /*DCO = 1467.648M */
};
#else
static const struct pll_params_table hifi_pll_params_table[] = {
	{ .m = 128, .n = 4, .regs = hifi_init_regs,
	  .regs_count = ARRAY_SIZE(hifi_init_regs)},  /*DCO = 768M */
	{ .m = 213, .n = 8, .regs = hifi_init_regs,
	  .regs_count = ARRAY_SIZE(hifi_init_regs)},  /*DCO = 639M */
	{ .m = 128, .n = 5, .regs = hifi_init_regs,
	  .regs_count = ARRAY_SIZE(hifi_init_regs)},  /*DCO = 614.4M */
	{ .m = 61, .n = 1,  .regs = hifi_init_regs1,
	  .regs_count = ARRAY_SIZE(hifi_init_regs1)},  /*DCO = 1467.648M */
};
#endif

/*
 * Warning: The hifi_pll output for c1 does not have a divider od. In order to be
 * compatible with the current pll software architecture, an invalid bit is
 * specified as od (this bit is always 0)
 */
MESON_CLK_PLL_RW(hifi_pll, ANACTRL_HIFIPLL_CTRL0, 28, 1,  /* en */
		 ANACTRL_HIFIPLL_CTRL0, 0, 8,  /* m */
		 ANACTRL_HIFIPLL_CTRL1, 0, 19,  /* frac */
		 ANACTRL_HIFIPLL_CTRL0, 10, 5,  /* n */
		 ANACTRL_HIFIPLL_STS, 31, 1,  /* lock */
		 ANACTRL_HIFIPLL_CTRL0, 29, 1,  /* rst */
		 hifi_init_regs, NULL, hifi_pll_params_table,
		 CLK_MESON_PLL_IGNORE_INIT,
		 &pll_dco_parent, 0,
		 ANACTRL_HIFIPLL_CTRL0, 17, 1
#ifdef CONFIG_ARM
		 );
#else
		 , NULL, CLK_DIVIDER_POWER_OF_TWO);
#endif

/*
 * aud dds clock is not pll clock, not divider clock,
 * No clock model can describe it.
 * So we regard it as a gate, and the gate ops
 * should realize lonely.
 */
__MESON_CLK_GATE(aud_dds, ANACTRL_AUDDDS_CTRL0, 30, 0, &clk_regmap_gate_ops,
		 NULL, &pll_dco_parent, NULL, 0);

static const struct cpu_dyn_table cpu_dsu_dyn_table[] = {
	CPU_LOW_PARAMS(24000000, 0, 0, 0),
	CPU_LOW_PARAMS(100000000, 1, 1, 9),
	CPU_LOW_PARAMS(250000000, 1, 1, 3),
	CPU_LOW_PARAMS(333333333, 2, 1, 1),
	CPU_LOW_PARAMS(500000000, 1, 1, 1),
	CPU_LOW_PARAMS(666666666, 2, 0, 0),
	CPU_LOW_PARAMS(1000000000, 1, 0, 0)
};

static const struct clk_parent_data cpu_dsu_dyn_clk_sel[] = {
	{ .fw_name = "xtal", },
	{ .hw = &fclk_div2.hw },
	{ .hw = &fclk_div3.hw }
};

MESON_CLK_DYN_RW(cpu_dyn_clk, CPUCTRL_CLK_CTRL0, 0, 0, 0,
		 cpu_dsu_dyn_table, cpu_dsu_dyn_clk_sel, 0);

static const struct clk_parent_data cpu_parent_data[] = {
	{ .hw = &cpu_dyn_clk.hw },
	{ .hw = &sys_pll.hw }
};

MESON_CLK_MUX_RW(cpu_clk, CPUCTRL_CLK_CTRL0, 0x1, 11, NULL, 0,
		 cpu_parent_data, CLK_SET_RATE_PARENT);

MESON_CLK_DYN_RW(dsu_dyn_clk, CPUCTRL_CLK_CTRL5, 0, 0, 0,
		 cpu_dsu_dyn_table, cpu_dsu_dyn_clk_sel, 0);

static const struct clk_parent_data dsu_pre_parent_data[] = {
	{ .hw = &dsu_dyn_clk.hw },
	{ .hw = &sys_pll.hw }
};

MESON_CLK_MUX_RW(dsu_pre_clk, CPUCTRL_CLK_CTRL5, 0x1, 11, NULL, 0,
		 dsu_pre_parent_data, 0);

static const struct clk_parent_data dsu_parent_data[] = {
	{ .hw = &cpu_clk.hw },
	{ .hw = &dsu_pre_clk.hw }
};

MESON_CLK_MUX_RW(dsu_clk, CPUCTRL_CLK_CTRL5, 0x1, 27, NULL, 0,
		 dsu_parent_data, 0);

MESON_CLK_DIV_RW(dsu_axi_clk, CPUCTRL_CLK_CTRL6, 4, 3, NULL, 0,
		 &dsu_clk.hw, 0);

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

__MESON_CLK_GATE(rtc_32k_clkin, RTC_BY_OSCIN_CTRL0, 31, 0,
		 &clk_regmap_gate_ops, NULL, &rtc_xtal_clkin_parent, NULL, 0);

static const struct meson_clk_dualdiv_param rtc_32k_div_table[] = {
	{ 733, 732, 8, 11, 1 },
	{ /* sentinel */ }
};

MESON_CLK_DUALDIV_RW(rtc_32k_div, RTC_BY_OSCIN_CTRL0, 0,  12,
		     RTC_BY_OSCIN_CTRL0, 12, 12,
		     RTC_BY_OSCIN_CTRL1, 0,  12,
		     RTC_BY_OSCIN_CTRL1, 12, 12,
		     RTC_BY_OSCIN_CTRL0, 28, 1, rtc_32k_div_table,
		     &rtc_32k_clkin.hw, 0);

static const struct clk_parent_data rtc_32k_mux_parent_data[] = {
	{ .hw = &rtc_32k_div.hw },
	{ .hw = &rtc_32k_clkin.hw }
};

MESON_CLK_MUX_RW(rtc_32k_mux, RTC_BY_OSCIN_CTRL1, 0x1, 24, NULL, 0,
		 rtc_32k_mux_parent_data, CLK_SET_RATE_PARENT);

MESON_CLK_GATE_RW(rtc_32k, RTC_BY_OSCIN_CTRL0, 30, 0,
		  &rtc_32k_mux.hw, CLK_SET_RATE_PARENT);

static const struct clk_parent_data rtc_clk_mux_parent_data[] = {
	{ .fw_name = "xtal" },
	{ .hw = &rtc_32k.hw },
	{ .fw_name = "pad" }
};

MESON_CLK_MUX_RW(rtc_clk, RTC_CTRL, 0x3, 0, NULL, 0,
		 rtc_clk_mux_parent_data, CLK_SET_RATE_PARENT);

/*
 * Some clocks have multiple clock sources, and the parent clock and index are
 * discontinuous, Some channels corresponding to the clock index are not
 * actually connected inside the chip, or the clock source is invalid.
 */
static u32 sys_axi_parent_table[] = { 0, 1, 2, 3, 4, 6, 7 };
static const struct clk_parent_data sys_axi_parent_data[] = {
	{ .fw_name = "xtal" },
	{ .hw = &fclk_div2.hw },
	{ .hw = &fclk_div3.hw },
	{ .hw = &fclk_div5.hw },
	{ .hw = &fclk_div4.hw },
	{ .hw = &fclk_div7.hw },
	{ .hw = &rtc_clk.hw }
};

MESON_CLK_COMPOSITE_RW(sys_a, SYS_CLK_CTRL0, 0x7, 10,
		       sys_axi_parent_table, 0, sys_axi_parent_data, 0,
		       SYS_CLK_CTRL0, 0, 10, NULL, 0, 0,
		       SYS_CLK_CTRL0, 13, 0, CLK_IGNORE_UNUSED);
MESON_CLK_COMPOSITE_RW(sys_b, SYS_CLK_CTRL0, 0x7, 26,
		       sys_axi_parent_table, 0, sys_axi_parent_data, 0,
		       SYS_CLK_CTRL0, 16, 10, NULL, 0, 0,
		       SYS_CLK_CTRL0, 29, 0, CLK_IGNORE_UNUSED);
static const struct clk_parent_data sys_clk_parent_data[] = {
	{ .hw = &sys_a.hw },
	{ .hw = &sys_b.hw }
};

MESON_CLK_MUX_RW(sys_clk, SYS_CLK_CTRL0, 1, 15, NULL, 0,
		 sys_clk_parent_data, 0);

MESON_CLK_COMPOSITE_RW(axi_a, AXI_CLK_CTRL0, 0x7, 10,
		       sys_axi_parent_table, 0, sys_axi_parent_data, 0,
		       AXI_CLK_CTRL0, 0, 10, NULL, 0, 0,
		       AXI_CLK_CTRL0, 13, 0, CLK_IGNORE_UNUSED);
MESON_CLK_COMPOSITE_RW(axi_b, AXI_CLK_CTRL0, 0x7, 26,
		       sys_axi_parent_table, 0, sys_axi_parent_data, 0,
		       AXI_CLK_CTRL0, 16, 10, NULL, 0, 0,
		       AXI_CLK_CTRL0, 29, 0, CLK_IGNORE_UNUSED);
static const struct clk_parent_data axi_clk_parent_data[] = {
	{ .hw = &axi_a.hw },
	{ .hw = &axi_b.hw }
};

MESON_CLK_MUX_RW(axi_clk, AXI_CLK_CTRL0, 1, 15, NULL, 0,
		 axi_clk_parent_data, 0);

/* sys_clk */
#define MESON_CLK_GATE_SYS_CLK(_name, _reg, _bit)			\
	MESON_CLK_GATE_RW(_name, _reg, _bit, 0, &sys_clk.hw, CLK_IGNORE_UNUSED)

/* axi_clk */
#define MESON_CLK_GATE_AXI_CLK(_name, _reg, _bit)			\
	MESON_CLK_GATE_RW(_name, _reg, _bit, 0, &axi_clk.hw, CLK_IGNORE_UNUSED)

/* Everything Else (EE) domain gates */
/* CLKTREE_SYS_CLK_EN0 */
MESON_CLK_GATE_SYS_CLK(sys_clk_tree,		SYS_CLK_EN0,	0);
MESON_CLK_GATE_SYS_CLK(sys_reset_ctrl,		SYS_CLK_EN0,	1);
MESON_CLK_GATE_SYS_CLK(sys_analog_ctrl,		SYS_CLK_EN0,	2);
MESON_CLK_GATE_SYS_CLK(sys_pwr_ctrl,		SYS_CLK_EN0,	3);
MESON_CLK_GATE_SYS_CLK(sys_pad_ctrl,		SYS_CLK_EN0,	4);
MESON_CLK_GATE_SYS_CLK(sys_ctrl,		SYS_CLK_EN0,	5);
MESON_CLK_GATE_SYS_CLK(sys_temp_sensor,		SYS_CLK_EN0,	6);
MESON_CLK_GATE_SYS_CLK(sys_am2axi_dev,		SYS_CLK_EN0,	7);
MESON_CLK_GATE_SYS_CLK(sys_spicc_b,		SYS_CLK_EN0,	8);
MESON_CLK_GATE_SYS_CLK(sys_spicc_a,		SYS_CLK_EN0,	9);
MESON_CLK_GATE_SYS_CLK(sys_clk_msr,		SYS_CLK_EN0,	10);
MESON_CLK_GATE_SYS_CLK(sys_audio,		SYS_CLK_EN0,	11);
MESON_CLK_GATE_SYS_CLK(sys_jtag_ctrl,		SYS_CLK_EN0,	12);
MESON_CLK_GATE_SYS_CLK(sys_saradc,		SYS_CLK_EN0,	13);
MESON_CLK_GATE_SYS_CLK(sys_pwm_ef,		SYS_CLK_EN0,	14);
MESON_CLK_GATE_SYS_CLK(sys_pwm_cd,		SYS_CLK_EN0,	15);
MESON_CLK_GATE_SYS_CLK(sys_pwm_ab,		SYS_CLK_EN0,	16);
MESON_CLK_GATE_SYS_CLK(sys_i2c_s,		SYS_CLK_EN0,	18);
MESON_CLK_GATE_SYS_CLK(sys_ir_ctrl,		SYS_CLK_EN0,	19);
MESON_CLK_GATE_SYS_CLK(sys_i2c_m_d,		SYS_CLK_EN0,	20);
MESON_CLK_GATE_SYS_CLK(sys_i2c_m_c,		SYS_CLK_EN0,	21);
MESON_CLK_GATE_SYS_CLK(sys_i2c_m_b,		SYS_CLK_EN0,	22);
MESON_CLK_GATE_SYS_CLK(sys_i2c_m_a,		SYS_CLK_EN0,	23);
MESON_CLK_GATE_SYS_CLK(sys_acodec,		SYS_CLK_EN0,	24);
MESON_CLK_GATE_SYS_CLK(sys_otp,			SYS_CLK_EN0,	25);
MESON_CLK_GATE_SYS_CLK(sys_sd_emmc_a,		SYS_CLK_EN0,	26);
MESON_CLK_GATE_SYS_CLK(sys_usb_phy,		SYS_CLK_EN0,	27);
MESON_CLK_GATE_SYS_CLK(sys_usb_ctrl,		SYS_CLK_EN0,	28);
MESON_CLK_GATE_SYS_CLK(sys_dspb,		SYS_CLK_EN0,	29);
MESON_CLK_GATE_SYS_CLK(sys_dspa,		SYS_CLK_EN0,	30);
MESON_CLK_GATE_SYS_CLK(sys_dma,			SYS_CLK_EN0,	31);
/* CLKTREE_SYS_CLK_EN1 */
MESON_CLK_GATE_SYS_CLK(sys_irq_ctrl,		SYS_CLK_EN1,	0);
MESON_CLK_GATE_SYS_CLK(sys_nic,			SYS_CLK_EN1,	1);
MESON_CLK_GATE_SYS_CLK(sys_gic,			SYS_CLK_EN1,	2);
MESON_CLK_GATE_SYS_CLK(sys_uart_c,		SYS_CLK_EN1,	3);
MESON_CLK_GATE_SYS_CLK(sys_uart_b,		SYS_CLK_EN1,	4);
MESON_CLK_GATE_SYS_CLK(sys_uart_a,		SYS_CLK_EN1,	5);
MESON_CLK_GATE_SYS_CLK(sys_rsa,			SYS_CLK_EN1,	8);
MESON_CLK_GATE_SYS_CLK(sys_coresight,		SYS_CLK_EN1,	9);
MESON_CLK_GATE_SYS_CLK(sys_csi_phy1,		SYS_CLK_EN1,	10);
MESON_CLK_GATE_SYS_CLK(sys_csi_phy0,		SYS_CLK_EN1,	11);
MESON_CLK_GATE_SYS_CLK(sys_mipi_isp,		SYS_CLK_EN1,	12);
MESON_CLK_GATE_SYS_CLK(sys_csi_dig,		SYS_CLK_EN1,	13);
MESON_CLK_GATE_SYS_CLK(sys_ge2d,		SYS_CLK_EN1,	14);
MESON_CLK_GATE_SYS_CLK(sys_gdc,			SYS_CLK_EN1,	15);
MESON_CLK_GATE_SYS_CLK(sys_dos_apb,		SYS_CLK_EN1,	16);
MESON_CLK_GATE_SYS_CLK(sys_nna,			SYS_CLK_EN1,	17);
MESON_CLK_GATE_SYS_CLK(sys_eth_phy,		SYS_CLK_EN1,	18);
MESON_CLK_GATE_SYS_CLK(sys_eth_mac,		SYS_CLK_EN1,	19);
MESON_CLK_GATE_SYS_CLK(sys_uart_e,		SYS_CLK_EN1,	20);
MESON_CLK_GATE_SYS_CLK(sys_uart_d,		SYS_CLK_EN1,	21);
MESON_CLK_GATE_SYS_CLK(sys_pwm_ij,		SYS_CLK_EN1,	22);
MESON_CLK_GATE_SYS_CLK(sys_pwm_gh,		SYS_CLK_EN1,	23);
MESON_CLK_GATE_SYS_CLK(sys_i2c_m_e,		SYS_CLK_EN1,	24);
MESON_CLK_GATE_SYS_CLK(sys_sd_emmc_c,		SYS_CLK_EN1,	25);
MESON_CLK_GATE_SYS_CLK(sys_sd_emmc_b,		SYS_CLK_EN1,	26);
MESON_CLK_GATE_SYS_CLK(sys_rom,			SYS_CLK_EN1,	27);
MESON_CLK_GATE_SYS_CLK(sys_spifc,		SYS_CLK_EN1,	28);
MESON_CLK_GATE_SYS_CLK(sys_prod_i2c,		SYS_CLK_EN1,	29);
MESON_CLK_GATE_SYS_CLK(sys_dos,			SYS_CLK_EN1,	30);
MESON_CLK_GATE_SYS_CLK(sys_cpu_ctrl,		SYS_CLK_EN1,	31);
/* CLKTREE_SYS_CLK_EN2 */
MESON_CLK_GATE_SYS_CLK(sys_rama,		SYS_CLK_EN2,	0);
MESON_CLK_GATE_SYS_CLK(sys_ramb,		SYS_CLK_EN2,	1);
MESON_CLK_GATE_SYS_CLK(sys_ramc,		SYS_CLK_EN2,	2);
/* CLKTREE_AXI_CLK_EN */
MESON_CLK_GATE_AXI_CLK(axi_am2axi_vad,		AXI_CLK_EN,	0);
MESON_CLK_GATE_AXI_CLK(axi_audio_vad,		AXI_CLK_EN,	1);
MESON_CLK_GATE_AXI_CLK(axi_dmc,			AXI_CLK_EN,	3);
MESON_CLK_GATE_AXI_CLK(axi_ramb,		AXI_CLK_EN,	5);
MESON_CLK_GATE_AXI_CLK(axi_rama,		AXI_CLK_EN,	6);
MESON_CLK_GATE_AXI_CLK(axi_nic,			AXI_CLK_EN,	8);
MESON_CLK_GATE_AXI_CLK(axi_dma,			AXI_CLK_EN,	9);
MESON_CLK_GATE_AXI_CLK(axi_ramc,		AXI_CLK_EN,	13);

static u32 dsp_parent_table[] = { 0, 1, 2, 3, 5, 6, 7 };
static const struct clk_parent_data dsp_parent_data[] = {
	{ .fw_name = "xtal" },
	{ .hw = &fclk_div2.hw },
	{ .hw = &fclk_div3.hw },
	{ .hw = &fclk_div5.hw },
	{ .hw = &fclk_div4.hw },
	{ .hw = &fclk_div7.hw },
	{ .hw = &rtc_clk.hw }
};

MESON_CLK_COMPOSITE_RW(dspa_a, DSPA_CLK_CTRL0, 0x7, 10,
		       dsp_parent_table, 0, dsp_parent_data, 0,
		       DSPA_CLK_CTRL0, 0, 10, NULL, 0,
		       CLK_SET_RATE_PARENT,
		       DSPA_CLK_CTRL0, 13, 0,
		       CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);
MESON_CLK_COMPOSITE_RW(dspa_b, DSPA_CLK_CTRL0, 0x7, 26,
		       dsp_parent_table, 0, dsp_parent_data, 0,
		       DSPA_CLK_CTRL0, 16, 10, NULL, 0,
		       CLK_SET_RATE_PARENT,
		       DSPA_CLK_CTRL0, 29, 0,
		       CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);
static const struct clk_parent_data dspa_parent_data[] = {
	{ .hw = &dspa_a.hw },
	{ .hw = &dspa_b.hw }
};

MESON_CLK_MUX_RW(dspa_clk, DSPA_CLK_CTRL0, 1, 15, NULL, 0,
		 dspa_parent_data, CLK_SET_RATE_PARENT);

MESON_CLK_GATE_RW(dspa_dspa, DSPA_CLK_EN, 1, 0, &dspa_clk.hw,
		  CLK_IGNORE_UNUSED);
MESON_CLK_GATE_RW(dspa_nic, DSPA_CLK_EN, 0, 0, &dspa_clk.hw,
		  CLK_IGNORE_UNUSED);

MESON_CLK_COMPOSITE_RW(dspb_a, DSPB_CLK_CTRL0, 0x7, 10,
		       dsp_parent_table, 0, dsp_parent_data, 0,
		       DSPB_CLK_CTRL0, 0, 10, NULL, 0,
		       CLK_SET_RATE_PARENT,
		       DSPB_CLK_CTRL0, 13, 0,
		       CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);
MESON_CLK_COMPOSITE_RW(dspb_b, DSPB_CLK_CTRL0, 0x7, 26,
		       dsp_parent_table, 0, dsp_parent_data, 0,
		       DSPB_CLK_CTRL0, 16, 10, NULL, 0,
		       CLK_SET_RATE_PARENT,
		       DSPB_CLK_CTRL0, 29, 0,
		       CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);
static const struct clk_parent_data dspb_parent_data[] = {
	{ .hw = &dspb_a.hw },
	{ .hw = &dspb_b.hw }
};

MESON_CLK_MUX_RW(dspb_clk, DSPB_CLK_CTRL0, 1, 15, NULL, 0,
		 dspb_parent_data, CLK_SET_RATE_PARENT);

MESON_CLK_GATE_RW(dspb_dspa, DSPB_CLK_EN, 1, 0, &dspb_clk.hw,
		  CLK_IGNORE_UNUSED);
MESON_CLK_GATE_RW(dspb_nic, DSPB_CLK_EN, 0, 0, &dspb_clk.hw,
		  CLK_IGNORE_UNUSED);

static const struct clk_parent_data clk_12_24m_in_parent = {
	.fw_name = "xtal",
};

__MESON_CLK_GATE(clk_24m, CLK12_24_CTRL, 11, 0, &clk_regmap_gate_ops,
		 NULL, &clk_12_24m_in_parent, NULL, 0);
MESON_CLK_FIXED_FACTOR(clk_24m_div2, 1, 2, &clk_24m.hw, 0);
MESON_CLK_GATE_RW(clk_12m, CLK12_24_CTRL, 10, 0, &clk_24m_div2.hw, 0);
MESON_CLK_DIV_RW(fclk_div2_divn_pre, CLK12_24_CTRL, 0, 8, NULL, 0,
		 &fclk_div2.hw, 0);
MESON_CLK_GATE_RW(fclk_div2_divn, CLK12_24_CTRL, 12, 0,
		  &fclk_div2_divn_pre.hw, CLK_SET_RATE_PARENT);

static u32 gen_parent_table[] = { 0, 1, 2, 3, 5, 6, 7, 8, 9, 10, 11, 12, 13 };
static const struct clk_parent_data gen_parent_data[] = {
	{ .fw_name = "xtal" },
	{ .hw = &rtc_clk.hw },
	{ .hw = &hifi_pll.hw },
	{ .hw = &fclk_div2.hw },
	{ .hw = &fclk_div3.hw },
	{ .hw = &fclk_div5.hw },
	{ .hw = &fclk_div7.hw },
	{ .hw = &gp_pll.hw },
	{ .hw = &fclk_div2p5.hw },
	{ .hw = &fclk_div4.hw },
	{ .hw = &sys_clk.hw },
	{ .hw = &axi_clk.hw }
};

MESON_CLK_COMPOSITE_RW(gen, GEN_CLK_CTRL, 0xf, 12,
		       gen_parent_table, 0, gen_parent_data, 0,
		       GEN_CLK_CTRL, 0, 11, NULL,
		       0, 0,
		       GEN_CLK_CTRL, 11,
		       0, 0);

static const struct clk_parent_data saradc_parent_data[] = {
	{ .fw_name = "xtal" },
	{ .hw = &sys_clk.hw }
};

MESON_CLK_COMPOSITE_RW(saradc, SAR_ADC_CLK_CTRL, 0x1, 9,
		       NULL, 0, saradc_parent_data, 0,
		       SAR_ADC_CLK_CTRL, 0, 8, NULL,
		       0, 0,
		       SAR_ADC_CLK_CTRL, 8,
		       0, 0);

static const struct clk_parent_data pwm_parent_data[] = {
	{ .fw_name = "xtal" },
	{ .hw = &sys_clk.hw },
	{ .hw = &rtc_clk.hw },
	{ .hw = &fclk_div4.hw }
};

MESON_CLK_COMPOSITE_RW(pwm_a, PWM_CLK_AB_CTRL, 0x3, 9,
		       NULL, 0, pwm_parent_data, 0,
		       PWM_CLK_AB_CTRL, 0, 8, NULL,
		       0, 0,
		       PWM_CLK_AB_CTRL, 8,
		       0, 0);
MESON_CLK_COMPOSITE_RW(pwm_b, PWM_CLK_AB_CTRL, 0x3, 25,
		       NULL, 0, pwm_parent_data, 0,
		       PWM_CLK_AB_CTRL, 16, 8, NULL,
		       0, 0,
		       PWM_CLK_AB_CTRL, 24,
		       0, 0);
MESON_CLK_COMPOSITE_RW(pwm_c, PWM_CLK_CD_CTRL, 0x3, 9,
		       NULL, 0, pwm_parent_data, 0,
		       PWM_CLK_CD_CTRL, 0, 8, NULL,
		       0, 0,
		       PWM_CLK_CD_CTRL, 8,
		       0, 0);
MESON_CLK_COMPOSITE_RW(pwm_d, PWM_CLK_CD_CTRL, 0x3, 25,
		       NULL, 0, pwm_parent_data, 0,
		       PWM_CLK_CD_CTRL, 16, 8, NULL,
		       0, 0,
		       PWM_CLK_CD_CTRL, 24,
		       0, 0);
MESON_CLK_COMPOSITE_RW(pwm_e, PWM_CLK_EF_CTRL, 0x3, 9,
		       NULL, 0, pwm_parent_data, 0,
		       PWM_CLK_EF_CTRL, 0, 8, NULL,
		       0, 0,
		       PWM_CLK_EF_CTRL, 8,
		       0, 0);
MESON_CLK_COMPOSITE_RW(pwm_f, PWM_CLK_EF_CTRL, 0x3, 25,
		       NULL, 0, pwm_parent_data, 0,
		       PWM_CLK_EF_CTRL, 16, 8, NULL,
		       0, 0,
		       PWM_CLK_EF_CTRL, 24,
		       0, 0);
MESON_CLK_COMPOSITE_RW(pwm_g, PWM_CLK_GH_CTRL, 0x3, 9,
		       NULL, 0, pwm_parent_data, 0,
		       PWM_CLK_GH_CTRL, 0, 8, NULL,
		       0, 0,
		       PWM_CLK_GH_CTRL, 8,
		       0, 0);
MESON_CLK_COMPOSITE_RW(pwm_h, PWM_CLK_GH_CTRL, 0x3, 25,
		       NULL, 0, pwm_parent_data, 0,
		       PWM_CLK_GH_CTRL, 16, 8, NULL,
		       0, 0,
		       PWM_CLK_GH_CTRL, 24,
		       0, 0);
MESON_CLK_COMPOSITE_RW(pwm_i, PWM_CLK_IJ_CTRL, 0x3, 9,
		       NULL, 0, pwm_parent_data, 0,
		       PWM_CLK_IJ_CTRL, 0, 8, NULL,
		       0, 0,
		       PWM_CLK_IJ_CTRL, 8,
		       0, 0);
MESON_CLK_COMPOSITE_RW(pwm_j, PWM_CLK_IJ_CTRL, 0x3, 25,
		       NULL, 0, pwm_parent_data, 0,
		       PWM_CLK_IJ_CTRL, 16, 8, NULL,
		       0, 0,
		       PWM_CLK_IJ_CTRL, 24,
		       0, 0);

static u32 spicc_pre_parent_table[] = { 0, 1, 2, 5, 6, 7 };

static const struct clk_parent_data spicc_pre_parent_data[] = {
	{ .hw = &fclk_div2.hw },
	{ .hw = &fclk_div3.hw },
	{ .hw = &fclk_div2p5.hw },
	{ .hw = &fclk_div4.hw },
	{ .hw = &fclk_div5.hw },
	{ .hw = &fclk_div7.hw }
};

MESON_CLK_MUX_RW(spicc_a_pre_mux, SPICC_CLK_CTRL, 0x7, 9, spicc_pre_parent_table, 0,
		 spicc_pre_parent_data, 0);
MESON_CLK_DIV_RW(spicc_a_pre_div, SPICC_CLK_CTRL, 0, 8, NULL, 0,
		 &spicc_a_pre_mux.hw, CLK_SET_RATE_PARENT);

static const struct clk_parent_data spicc_a_parent_data[] = {
	{ .hw = &spicc_a_pre_div.hw },
	{ .fw_name = "xtal" }
};

MESON_CLK_MUX_RW(spicc_a_mux, SPICC_CLK_CTRL, 0x1, 15, NULL, 0,
		 spicc_a_parent_data, CLK_SET_RATE_PARENT);

MESON_CLK_GATE_RW(spicc_a, SPICC_CLK_CTRL, 8, 0,
		  &spicc_a_mux.hw, CLK_SET_RATE_PARENT);

MESON_CLK_MUX_RW(spicc_b_pre_mux, SPICC_CLK_CTRL, 0x7, 25, spicc_pre_parent_table, 0,
		 spicc_pre_parent_data, 0);
MESON_CLK_DIV_RW(spicc_b_pre_div, SPICC_CLK_CTRL, 16, 8, NULL, 0,
		 &spicc_b_pre_mux.hw, CLK_SET_RATE_PARENT);

static const struct clk_parent_data spicc_b_parent_data[] = {
	{ .hw = &spicc_b_pre_div.hw },
	{ .fw_name = "xtal" }
};

MESON_CLK_MUX_RW(spicc_b_mux, SPICC_CLK_CTRL, 0x1, 31, NULL, 0,
		 spicc_b_parent_data, CLK_SET_RATE_PARENT);

MESON_CLK_GATE_RW(spicc_b, SPICC_CLK_CTRL, 24, 0,
		  &spicc_b_mux.hw, CLK_SET_RATE_PARENT);

MESON_CLK_MUX_RW(spifc_pre_mux, SPIFC_CLK_CTRL, 0x7, 9, spicc_pre_parent_table, 0,
		 spicc_pre_parent_data, 0);
MESON_CLK_DIV_RW(spifc_pre_div, SPIFC_CLK_CTRL, 0, 8, NULL, 0,
		 &spifc_pre_mux.hw, CLK_SET_RATE_PARENT);

static const struct clk_parent_data spifc_parent_data[] = {
	{ .hw = &spifc_pre_div.hw },
	{ .fw_name = "xtal" }
};

MESON_CLK_MUX_RW(spifc_mux, SPIFC_CLK_CTRL, 0x1, 15, NULL, 0,
		 spicc_a_parent_data, CLK_SET_RATE_PARENT);

MESON_CLK_GATE_RW(spifc, SPIFC_CLK_CTRL, 8, 0,
		  &spicc_a_mux.hw, CLK_SET_RATE_PARENT);

/* temperature sensor */
static const struct clk_parent_data ts_parent = {
	.fw_name = "xtal",
};

__MESON_CLK_DIV(ts_div, TS_CLK_CTRL, 0, 8, NULL, 0, 0, 0,
		&clk_regmap_divider_ops, NULL, &ts_parent, NULL, 0);
MESON_CLK_GATE_RW(ts, TS_CLK_CTRL, 8, 0, &ts_div.hw, CLK_SET_RATE_PARENT);

static const struct clk_parent_data usb_bus_parent_data[] = {
	{ .fw_name = "xtal" },
	{ .hw = &sys_clk.hw },
	{ .hw = &fclk_div3.hw },
	{ .hw = &fclk_div5.hw }
};

MESON_CLK_COMPOSITE_RW(usb_bus, USB_BUSCLK_CTRL, 0x3, 9,
		       NULL, 0, usb_bus_parent_data, 0,
		       USB_BUSCLK_CTRL, 0, 8, NULL,
		       0, 0,
		       USB_BUSCLK_CTRL, 8,
		       0, 0);

static u32 sd_emmc_pre_parent_table[] = { 0, 1, 2, 4, 5, 6, 7 };

static const struct clk_parent_data sd_emmc_pre_parent_data[] = {
	{ .hw = &fclk_div2.hw },
	{ .hw = &fclk_div3.hw },
	{ .hw = &fclk_div2p5.hw },
	{ .hw = &gp_pll.hw },
	{ .hw = &fclk_div4.hw },
	{ .hw = &fclk_div5.hw },
	{ .hw = &fclk_div7.hw }
};

MESON_CLK_MUX_RW(sd_emmc_a_pre_mux, SD_EMMC_CLK_CTRL, 0x7, 9,
		 sd_emmc_pre_parent_table, 0, sd_emmc_pre_parent_data, 0);
MESON_CLK_DIV_RW(sd_emmc_a_pre_div, SD_EMMC_CLK_CTRL, 0, 8, NULL, 0,
		 &sd_emmc_a_pre_mux.hw, 0);

static const struct clk_parent_data sd_emmc_a_parent_data[] = {
	{ .hw = &sd_emmc_a_pre_div.hw },
	{ .fw_name = "xtal" }
};

MESON_CLK_MUX_RW(sd_emmc_a_mux, SD_EMMC_CLK_CTRL, 0x1, 15, NULL, 0,
		 sd_emmc_a_parent_data, 0);

MESON_CLK_GATE_RW(sd_emmc_a, SD_EMMC_CLK_CTRL, 8, 0,
		  &sd_emmc_a_mux.hw, CLK_SET_RATE_PARENT);

MESON_CLK_MUX_RW(sd_emmc_b_pre_mux, SD_EMMC_CLK_CTRL, 0x7, 25,
		 sd_emmc_pre_parent_table, 0, sd_emmc_pre_parent_data, 0);
MESON_CLK_DIV_RW(sd_emmc_b_pre_div, SD_EMMC_CLK_CTRL, 16, 8, NULL, 0,
		 &sd_emmc_b_pre_mux.hw, 0);

static const struct clk_parent_data sd_emmc_b_parent_data[] = {
	{ .hw = &sd_emmc_b_pre_div.hw },
	{ .fw_name = "xtal" }
};

MESON_CLK_MUX_RW(sd_emmc_b_mux, SD_EMMC_CLK_CTRL, 0x1, 31, NULL, 0,
		 sd_emmc_b_parent_data, 0);

MESON_CLK_GATE_RW(sd_emmc_b, SD_EMMC_CLK_CTRL, 24, 0,
		  &sd_emmc_b_mux.hw, CLK_SET_RATE_PARENT);

MESON_CLK_MUX_RW(sd_emmc_c_pre_mux, SD_EMMC_CLK_CTRL1, 0x7, 9,
		 sd_emmc_pre_parent_table, 0, sd_emmc_pre_parent_data, 0);
MESON_CLK_DIV_RW(sd_emmc_c_pre_div, SD_EMMC_CLK_CTRL1, 0, 8, NULL, 0,
		 &sd_emmc_c_pre_mux.hw, 0);

static const struct clk_parent_data sd_emmc_c_parent_data[] = {
	{ .hw = &sd_emmc_c_pre_div.hw },
	{ .fw_name = "xtal" }
};

MESON_CLK_MUX_RW(sd_emmc_c_mux, SD_EMMC_CLK_CTRL1, 0x1, 15, NULL, 0,
		 sd_emmc_c_parent_data, 0);

MESON_CLK_GATE_RW(sd_emmc_c, SD_EMMC_CLK_CTRL1, 8, 0,
		  &sd_emmc_c_mux.hw, CLK_SET_RATE_PARENT);

/* wave clk */
static u32 wave_pre_parent_table[] = { 0, 1, 2, 4, 5, 6, 7 };

static const struct clk_parent_data wave_pre_parent_data[] = {
	{ .hw = &fclk_div2.hw },
	{ .hw = &fclk_div3.hw },
	{ .hw = &fclk_div2p5.hw },
	{ .hw = &hifi_pll.hw },
	{ .hw = &fclk_div4.hw },
	{ .hw = &fclk_div5.hw },
	{ .hw = &fclk_div7.hw }
};

MESON_CLK_MUX_RW(wave_a_pre_mux, WAVE_CLK_CTRL0, 0x7, 9,
		 wave_pre_parent_table, 0, wave_pre_parent_data, 0);
MESON_CLK_DIV_RW(wave_a_pre_div, WAVE_CLK_CTRL0, 0, 8, NULL, 0,
		 &wave_a_pre_mux.hw, CLK_SET_RATE_PARENT);

static const struct clk_parent_data wave_a_parent_data[] = {
	{ .hw = &wave_a_pre_div.hw },
	{ .fw_name = "xtal" }
};

MESON_CLK_MUX_RW(wave_a_mux, WAVE_CLK_CTRL0, 0x1, 15, NULL, 0,
		 wave_a_parent_data, CLK_SET_RATE_PARENT);

MESON_CLK_GATE_RW(wave_a, WAVE_CLK_CTRL0, 8, 0,
		  &wave_a_mux.hw, CLK_SET_RATE_PARENT);

MESON_CLK_MUX_RW(wave_b_pre_mux, WAVE_CLK_CTRL0, 0x7, 25,
		 wave_pre_parent_table, 0, wave_pre_parent_data, 0);
MESON_CLK_DIV_RW(wave_b_pre_div, WAVE_CLK_CTRL0, 16, 8, NULL, 0,
		 &wave_b_pre_mux.hw, CLK_SET_RATE_PARENT);

static const struct clk_parent_data wave_b_parent_data[] = {
	{ .hw = &wave_b_pre_div.hw },
	{ .fw_name = "xtal" }
};

MESON_CLK_MUX_RW(wave_b_mux, WAVE_CLK_CTRL0, 0x1, 31, NULL, 0,
		 wave_b_parent_data, CLK_SET_RATE_PARENT);

MESON_CLK_GATE_RW(wave_b, WAVE_CLK_CTRL0, 24, 0,
		  &wave_b_mux.hw, CLK_SET_RATE_PARENT);

MESON_CLK_MUX_RW(wave_c_pre_mux, WAVE_CLK_CTRL1, 0x7, 9,
		 wave_pre_parent_table, 0, wave_pre_parent_data, 0);
MESON_CLK_DIV_RW(wave_c_pre_div, WAVE_CLK_CTRL1, 0, 8, NULL, 0,
		 &wave_c_pre_mux.hw, CLK_SET_RATE_PARENT);

static const struct clk_parent_data wave_c_parent_data[] = {
	{ .hw = &wave_c_pre_div.hw },
	{ .fw_name = "xtal" }
};

MESON_CLK_MUX_RW(wave_c_mux, WAVE_CLK_CTRL1, 0x1, 15, NULL, 0,
		 wave_c_parent_data, CLK_SET_RATE_PARENT);

MESON_CLK_GATE_RW(wave_c, WAVE_CLK_CTRL1, 8, 0,
		  &wave_c_mux.hw, CLK_SET_RATE_PARENT);

/* jpeg clk */
static u32 media_pre_parent_table[] = { 0, 1, 2, 5, 6, 7 };

static const struct clk_parent_data media_pre_parent_data[] = {
	{ .hw = &fclk_div2.hw },
	{ .hw = &fclk_div3.hw },
	{ .hw = &fclk_div2p5.hw },
	{ .hw = &fclk_div4.hw },
	{ .hw = &fclk_div5.hw },
	{ .hw = &fclk_div7.hw }
};

MESON_CLK_MUX_RW(jpeg_pre_mux, JPEG_CLK_CTRL, 0x7, 9,
		 media_pre_parent_table, 0, media_pre_parent_data, 0);
MESON_CLK_DIV_RW(jpeg_pre_div, JPEG_CLK_CTRL, 0, 8, NULL, 0,
		 &jpeg_pre_mux.hw, CLK_SET_RATE_PARENT);

static const struct clk_parent_data jpeg_parent_data[] = {
	{ .hw = &jpeg_pre_div.hw },
	{ .fw_name = "xtal" }
};

MESON_CLK_MUX_RW(jpeg_mux, JPEG_CLK_CTRL, 0x1, 15, NULL, 0,
		 jpeg_parent_data, CLK_SET_RATE_PARENT);

MESON_CLK_GATE_RW(jpeg, JPEG_CLK_CTRL, 8, 0,
		  &jpeg_mux.hw, CLK_SET_RATE_PARENT);

/* mipi csi phy */
MESON_CLK_MUX_RW(mipi_csi_phy_pre_mux, MIPI_ISP_CLK_CTRL, 0x7, 25,
		 media_pre_parent_table, 0, media_pre_parent_data, 0);
MESON_CLK_DIV_RW(mipi_csi_phy_pre_div, MIPI_ISP_CLK_CTRL, 16, 8, NULL, 0,
		 &mipi_csi_phy_pre_mux.hw, CLK_SET_RATE_PARENT);

static const struct clk_parent_data mipi_csi_phy_parent_data[] = {
	{ .hw = &mipi_csi_phy_pre_div.hw },
	{ .fw_name = "xtal" }
};

MESON_CLK_MUX_RW(mipi_csi_phy_mux, MIPI_ISP_CLK_CTRL, 0x1, 31, NULL, 0,
		 mipi_csi_phy_parent_data, CLK_SET_RATE_PARENT);

MESON_CLK_GATE_RW(mipi_csi_phy, MIPI_ISP_CLK_CTRL, 24, 0,
		  &mipi_csi_phy_mux.hw, CLK_SET_RATE_PARENT);

/* mipi isp */
MESON_CLK_MUX_RW(mipi_isp_pre_mux, MIPI_ISP_CLK_CTRL, 0x7, 9,
		 media_pre_parent_table, 0, media_pre_parent_data, 0);
MESON_CLK_DIV_RW(mipi_isp_pre_div, MIPI_ISP_CLK_CTRL, 0, 8, NULL, 0,
		 &mipi_isp_pre_mux.hw, CLK_SET_RATE_PARENT);

static const struct clk_parent_data mipi_isp_parent_data[] = {
	{ .hw = &mipi_isp_pre_div.hw },
	{ .fw_name = "xtal" }
};

MESON_CLK_MUX_RW(mipi_isp_mux, MIPI_ISP_CLK_CTRL, 0x1, 15, NULL, 0,
		 mipi_isp_parent_data, CLK_SET_RATE_PARENT);

MESON_CLK_GATE_RW(mipi_isp, MIPI_ISP_CLK_CTRL, 8, 0,
		  &mipi_isp_mux.hw, CLK_SET_RATE_PARENT);

/* nna axi */
MESON_CLK_MUX_RW(nna_axi_pre_mux, NNA_CLK_CTRL, 0x7, 25,
		 media_pre_parent_table, 0, media_pre_parent_data, 0);
MESON_CLK_DIV_RW(nna_axi_pre_div, NNA_CLK_CTRL, 16, 8, NULL, 0,
		 &nna_axi_pre_mux.hw, CLK_SET_RATE_PARENT);

static const struct clk_parent_data nna_axi_parent_data[] = {
	{ .hw = &nna_axi_pre_div.hw },
	{ .fw_name = "xtal" }
};

MESON_CLK_MUX_RW(nna_axi_mux, NNA_CLK_CTRL, 0x1, 31, NULL, 0,
		 nna_axi_parent_data, CLK_SET_RATE_PARENT);

MESON_CLK_GATE_RW(nna_axi, NNA_CLK_CTRL, 24, 0,
		  &nna_axi_mux.hw, CLK_SET_RATE_PARENT);

/* nna core */
MESON_CLK_MUX_RW(nna_core_pre_mux, NNA_CLK_CTRL, 0x7, 9,
		 media_pre_parent_table, 0, media_pre_parent_data, 0);
MESON_CLK_DIV_RW(nna_core_pre_div, NNA_CLK_CTRL, 0, 8, NULL, 0,
		 &nna_core_pre_mux.hw, CLK_SET_RATE_PARENT);

static const struct clk_parent_data nna_core_parent_data[] = {
	{ .hw = &nna_core_pre_div.hw },
	{ .fw_name = "xtal" }
};

MESON_CLK_MUX_RW(nna_core_mux, NNA_CLK_CTRL, 0x1, 15, NULL, 0,
		 nna_core_parent_data, CLK_SET_RATE_PARENT);

MESON_CLK_GATE_RW(nna_core, NNA_CLK_CTRL, 8, 0,
		  &nna_core_mux.hw, CLK_SET_RATE_PARENT);

/* gdc axi */
MESON_CLK_MUX_RW(gdc_axi_pre_mux, GDC_CLK_CTRL, 0x7, 25,
		 media_pre_parent_table, 0, media_pre_parent_data, 0);
MESON_CLK_DIV_RW(gdc_axi_pre_div, GDC_CLK_CTRL, 16, 8, NULL, 0,
		 &gdc_axi_pre_mux.hw, CLK_SET_RATE_PARENT);

static const struct clk_parent_data gdc_axi_parent_data[] = {
	{ .hw = &gdc_axi_pre_div.hw },
	{ .fw_name = "xtal" }
};

MESON_CLK_MUX_RW(gdc_axi_mux, GDC_CLK_CTRL, 0x1, 31, NULL, 0,
		 gdc_axi_parent_data, CLK_SET_RATE_PARENT);

MESON_CLK_GATE_RW(gdc_axi, GDC_CLK_CTRL, 24, 0,
		  &gdc_axi_mux.hw, CLK_SET_RATE_PARENT);

/* gdc core */
MESON_CLK_MUX_RW(gdc_core_pre_mux, GDC_CLK_CTRL, 0x7, 9,
		 media_pre_parent_table, 0, media_pre_parent_data, 0);
MESON_CLK_DIV_RW(gdc_core_pre_div, GDC_CLK_CTRL, 0, 8, NULL, 0,
		 &gdc_core_pre_mux.hw, CLK_SET_RATE_PARENT);

static const struct clk_parent_data gdc_core_parent_data[] = {
	{ .hw = &gdc_core_pre_div.hw },
	{ .fw_name = "xtal" }
};

MESON_CLK_MUX_RW(gdc_core_mux, GDC_CLK_CTRL, 0x1, 15, NULL, 0,
		 gdc_core_parent_data, CLK_SET_RATE_PARENT);

MESON_CLK_GATE_RW(gdc_core, GDC_CLK_CTRL, 8, 0,
		  &gdc_core_mux.hw, CLK_SET_RATE_PARENT);

/* ge2d */
MESON_CLK_MUX_RW(ge2d_pre_mux, GE2D_CLK_CTRL, 0x7, 9,
		 media_pre_parent_table, 0, media_pre_parent_data, 0);
MESON_CLK_DIV_RW(ge2d_pre_div, GE2D_CLK_CTRL, 0, 8, NULL, 0,
		 &ge2d_pre_mux.hw, CLK_SET_RATE_PARENT);

static const struct clk_parent_data ge2d_parent_data[] = {
	{ .hw = &ge2d_pre_div.hw },
	{ .fw_name = "xtal" }
};

MESON_CLK_MUX_RW(ge2d_mux, GE2D_CLK_CTRL, 0x1, 15, NULL, 0,
		 ge2d_parent_data, CLK_SET_RATE_PARENT);

MESON_CLK_GATE_RW(ge2d, GE2D_CLK_CTRL, 8, 0,
		  &ge2d_mux.hw, CLK_SET_RATE_PARENT);

/* eth */
static const struct clk_parent_data eth_parent_data[] = {
	{ .hw = &fclk_div2.hw },
	{ .hw = &fclk_div3.hw },
	{ .hw = &fclk_div5.hw }
};

MESON_CLK_COMPOSITE_RW(eth_125m, ETH_CLK_CTRL, 0x3, 25,
		       NULL, 0, eth_parent_data, 0,
		       ETH_CLK_CTRL, 16, 8, NULL,
		       0, 0,
		       ETH_CLK_CTRL, 24,
		       0, 0);

MESON_CLK_COMPOSITE_RW(eth_rmii, ETH_CLK_CTRL, 0x3, 9,
		       NULL, 0, eth_parent_data, 0,
		       ETH_CLK_CTRL, 0, 8, NULL,
		       0, 0,
		       ETH_CLK_CTRL, 8,
		       0, 0);

/* Array of all clocks provided by this provider */
static struct clk_hw_onecell_data c1_hw_onecell_data = {
	.hws = {
#ifndef CONFIG_ARM
		[CLKID_FIXED_PLL_DCO]		= &fixed_pll_dco.hw,
#endif
		[CLKID_FIXED_PLL]		= &fixed_pll.hw,
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
		[CLKID_FCLK_DIV40_DIV]		= &fclk_div40_div.hw,
		[CLKID_FCLK50M]			= &fclk_50m.hw,
#ifndef CONFIG_ARM
		[CLKID_SYS_PLL_DCO]		= &sys_pll_dco.hw,
#endif
		[CLKID_SYS_PLL]			= &sys_pll.hw,
#ifndef CONFIG_ARM
		[CLKID_GP_PLL_DCO]		= &gp_pll_dco.hw,
#endif
		[CLKID_GP_PLL]			= &gp_pll.hw,
		[CLKID_GP_DIV1_DIV2]		= &gp_pll_div1_fix_div2.hw,
		[CLKID_GP_DIV1_DIV]		= &gp_pll_div1.hw,
		[CLKID_GP_DIV2_DIV2]		= &gp_pll_div2_fix_div2.hw,
		[CLKID_GP_DIV2_DIV]		= &gp_pll_div2.hw,
		[CLKID_GP_DIV3_DIV]		= &gp_pll_div3.hw,
#ifndef CONFIG_ARM
		[CLKID_HIFI_PLL_DCO]		= &hifi_pll_dco.hw,
#endif
		[CLKID_HIFI_PLL]		= &hifi_pll.hw,
		[CLKID_AUD_DDS]			= &aud_dds.hw,

		[CLKID_CPU_DYN_CLK]		= &cpu_dyn_clk.hw,
		[CLKID_CPU_CLK]			= &cpu_clk.hw,

		[CLKID_DSU_DYN_CLK]		= &dsu_dyn_clk.hw,
		[CLKID_DSU_PRE_CLK]		= &dsu_pre_clk.hw,
		[CLKID_DSU_CLK]			= &dsu_clk.hw,
		[CLKID_DSU_AXI_CLK]		= &dsu_axi_clk.hw,

		[CLKID_GP_DIV1]			= &gp_pll_div1_gate.hw,
		[CLKID_GP_DIV2]			= &gp_pll_div2_gate.hw,
		[CLKID_GP_DIV3]			= &gp_pll_div3_gate.hw,
		[CLKID_FIXED_PLL_DIV2]		= &fixed_pll_div2_gate.hw,
		[CLKID_RTC_32K_CLKIN]		= &rtc_32k_clkin.hw,
		[CLKID_RTC_32K_DIV]		= &rtc_32k_div.hw,
		[CLKID_RTC_32K_MUX]		= &rtc_32k_mux.hw,
		[CLKID_RTC_32K]			= &rtc_32k.hw,
		[CLKID_RTC_CLK]			= &rtc_clk.hw,
		[CLKID_SYS_CLK_A_MUX]		= &sys_a_mux.hw,
		[CLKID_SYS_CLK_A_DIV]		= &sys_a_div.hw,
		[CLKID_SYS_CLK_A_GATE]		= &sys_a.hw,
		[CLKID_SYS_CLK_B_MUX]		= &sys_b_mux.hw,
		[CLKID_SYS_CLK_B_DIV]		= &sys_b_div.hw,
		[CLKID_SYS_CLK_B_GATE]		= &sys_b.hw,
		[CLKID_SYS_CLK]			= &sys_clk.hw,
		[CLKID_AXI_CLK_A_MUX]		= &axi_a_mux.hw,
		[CLKID_AXI_CLK_A_DIV]		= &axi_a_div.hw,
		[CLKID_AXI_CLK_A_GATE]		= &axi_a.hw,
		[CLKID_AXI_CLK_B_MUX]		= &axi_b_mux.hw,
		[CLKID_AXI_CLK_B_DIV]		= &axi_b_div.hw,
		[CLKID_AXI_CLK_B_GATE]		= &axi_b.hw,
		[CLKID_AXI_CLK]			= &axi_clk.hw,
		[CLKID_SYS_CLKTREE]		= &sys_clk_tree.hw,
		[CLKID_SYS_RESET_CTRL]		= &sys_reset_ctrl.hw,
		[CLKID_SYS_ANALOG_CTRL]		= &sys_analog_ctrl.hw,
		[CLKID_SYS_PWR_CTRL]		= &sys_pwr_ctrl.hw,
		[CLKID_SYS_PAD_CTRL]		= &sys_pad_ctrl.hw,
		[CLKID_SYS_CTRL]		= &sys_ctrl.hw,
		[CLKID_SYS_TEMP_SENSOR]		= &sys_temp_sensor.hw,
		[CLKID_SYS_AM2AXI_DIV]		= &sys_am2axi_dev.hw,
		[CLKID_SYS_SPICC_B]		= &sys_spicc_b.hw,
		[CLKID_SYS_SPICC_A]		= &sys_spicc_a.hw,
		[CLKID_SYS_CLK_MSR]		= &sys_clk_msr.hw,
		[CLKID_SYS_AUDIO]		= &sys_audio.hw,
		[CLKID_SYS_JTAG_CTRL]		= &sys_jtag_ctrl.hw,
		[CLKID_SYS_SARADC]		= &sys_saradc.hw,
		[CLKID_SYS_PWM_EF]		= &sys_pwm_ef.hw,
		[CLKID_SYS_PWM_CD]		= &sys_pwm_cd.hw,
		[CLKID_SYS_PWM_AB]		= &sys_pwm_ab.hw,
		[CLKID_SYS_I2C_S]		= &sys_i2c_s.hw,
		[CLKID_SYS_IR_CTRL]		= &sys_ir_ctrl.hw,
		[CLKID_SYS_I2C_M_D]		= &sys_i2c_m_d.hw,
		[CLKID_SYS_I2C_M_C]		= &sys_i2c_m_c.hw,
		[CLKID_SYS_I2C_M_B]		= &sys_i2c_m_b.hw,
		[CLKID_SYS_I2C_M_A]		= &sys_i2c_m_a.hw,
		[CLKID_SYS_ACODEC]		= &sys_acodec.hw,
		[CLKID_SYS_OTP]			= &sys_otp.hw,
		[CLKID_SYS_SD_EMMC_A]		= &sys_sd_emmc_a.hw,
		[CLKID_SYS_USB_PHY]		= &sys_usb_phy.hw,
		[CLKID_SYS_USB_CTRL]		= &sys_usb_ctrl.hw,
		[CLKID_SYS_DSPB]		= &sys_dspb.hw,
		[CLKID_SYS_DSPA]		= &sys_dspa.hw,
		[CLKID_SYS_DMA]			= &sys_dma.hw,
		[CLKID_SYS_IRQ_CTRL]		= &sys_irq_ctrl.hw,
		[CLKID_SYS_NIC]			= &sys_nic.hw,
		[CLKID_SYS_GIC]			= &sys_gic.hw,
		[CLKID_SYS_UART_C]		= &sys_uart_c.hw,
		[CLKID_SYS_UART_B]		= &sys_uart_b.hw,
		[CLKID_SYS_UART_A]		= &sys_uart_a.hw,
		[CLKID_SYS_RSA]			= &sys_rsa.hw,
		[CLKID_SYS_CORESIGHT]		= &sys_coresight.hw,
		[CLKID_SYS_CSI_PHY1]		= &sys_csi_phy1.hw,
		[CLKID_SYS_CSI_PHY0]		= &sys_csi_phy0.hw,
		[CLKID_SYS_MIPI_ISP]		= &sys_mipi_isp.hw,
		[CLKID_SYS_CSI_DIG]		= &sys_csi_dig.hw,
		[CLKID_SYS_G2ED]		= &sys_ge2d.hw,
		[CLKID_SYS_GDC]			= &sys_gdc.hw,
		[CLKID_SYS_DOS_APB]		= &sys_dos_apb.hw,
		[CLKID_SYS_NNA]			= &sys_nna.hw,
		[CLKID_SYS_ETH_PHY]		= &sys_eth_phy.hw,
		[CLKID_SYS_ETH_MAC]		= &sys_eth_mac.hw,
		[CLKID_SYS_UART_E]		= &sys_uart_e.hw,
		[CLKID_SYS_UART_D]		= &sys_uart_d.hw,
		[CLKID_SYS_PWM_IJ]		= &sys_pwm_ij.hw,
		[CLKID_SYS_PWM_GH]		= &sys_pwm_gh.hw,
		[CLKID_SYS_I2C_M_E]		= &sys_i2c_m_e.hw,
		[CLKID_SYS_SD_EMMC_C]		= &sys_sd_emmc_c.hw,
		[CLKID_SYS_SD_EMMC_B]		= &sys_sd_emmc_b.hw,
		[CLKID_SYS_ROM]			= &sys_rom.hw,
		[CLKID_SYS_SPIFC]		= &sys_spifc.hw,
		[CLKID_SYS_PROD_I2C]		= &sys_prod_i2c.hw,
		[CLKID_SYS_DOS]			= &sys_dos.hw,
		[CLKID_SYS_CPU_CTRL]		= &sys_cpu_ctrl.hw,
		[CLKID_SYS_RAMA]		= &sys_rama.hw,
		[CLKID_SYS_RAMB]		= &sys_ramb.hw,
		[CLKID_SYS_RAMC]		= &sys_ramc.hw,
		[CLKID_AXI_AM2AXI_VAD]		= &axi_am2axi_vad.hw,
		[CLKID_AXI_AUDIO_VAD]		= &axi_audio_vad.hw,
		[CLKID_AXI_DMC]			= &axi_dmc.hw,
		[CLKID_AXI_RAMB]		= &axi_ramb.hw,
		[CLKID_AXI_RAMA]		= &axi_rama.hw,
		[CLKID_AXI_NIC]			= &axi_nic.hw,
		[CLKID_AXI_DMA]			= &axi_dma.hw,
		[CLKID_AXI_RAMC]		= &axi_ramc.hw,
		[CLKID_DSPA_A_MUX]		= &dspa_a_mux.hw,
		[CLKID_DSPA_A_DIV]		= &dspa_a_div.hw,
		[CLKID_DSPA_A]			= &dspa_a.hw,
		[CLKID_DSPA_B_MUX]		= &dspa_b_mux.hw,
		[CLKID_DSPA_B_DIV]		= &dspa_b_div.hw,
		[CLKID_DSPA_B]			= &dspa_b.hw,
		[CLKID_DSPA]			= &dspa_clk.hw,
		[CLKID_DSPA_DSPA]		= &dspa_dspa.hw,
		[CLKID_DSPA_NIC]		= &dspa_nic.hw,
		[CLKID_DSPB_A_MUX]		= &dspb_a_mux.hw,
		[CLKID_DSPB_A_DIV]		= &dspb_a_div.hw,
		[CLKID_DSPB_A]			= &dspb_a.hw,
		[CLKID_DSPB_B_MUX]		= &dspb_b_mux.hw,
		[CLKID_DSPB_B_DIV]		= &dspb_b_div.hw,
		[CLKID_DSPB_B]			= &dspb_b.hw,
		[CLKID_DSPB]			= &dspb_clk.hw,
		[CLKID_DSPB_DSPB]		= &dspb_dspa.hw,
		[CLKID_DSPB_NIC]		= &dspb_nic.hw,
		[CLKID_24M]			= &clk_24m.hw,
		[CLKID_24M_DIV2]		= &clk_24m_div2.hw,
		[CLKID_12M]			= &clk_12m.hw,
		[CLKID_FCLK_DIV2_DIVN_PRE]	= &fclk_div2_divn_pre.hw,
		[CLKID_FCLK_DIV2_DIVN]		= &fclk_div2_divn.hw,
		[CLKID_GEN_MUX]			= &gen_mux.hw,
		[CLKID_GEN_DIV]			= &gen_div.hw,
		[CLKID_GEN]			= &gen.hw,
		[CLKID_SARADC_MUX]		= &saradc_mux.hw,
		[CLKID_SARADC_DIV]		= &saradc_div.hw,
		[CLKID_SARADC]			= &saradc.hw,
		[CLKID_PWM_A_MUX]		= &pwm_a_mux.hw,
		[CLKID_PWM_A_DIV]		= &pwm_a_div.hw,
		[CLKID_PWM_A]			= &pwm_a.hw,
		[CLKID_PWM_B_MUX]		= &pwm_b_mux.hw,
		[CLKID_PWM_B_DIV]		= &pwm_b_div.hw,
		[CLKID_PWM_B]			= &pwm_b.hw,
		[CLKID_PWM_C_MUX]		= &pwm_c_mux.hw,
		[CLKID_PWM_C_DIV]		= &pwm_c_div.hw,
		[CLKID_PWM_C]			= &pwm_c.hw,
		[CLKID_PWM_D_MUX]		= &pwm_d_mux.hw,
		[CLKID_PWM_D_DIV]		= &pwm_d_div.hw,
		[CLKID_PWM_D]			= &pwm_d.hw,
		[CLKID_PWM_E_MUX]		= &pwm_e_mux.hw,
		[CLKID_PWM_E_DIV]		= &pwm_e_div.hw,
		[CLKID_PWM_E]			= &pwm_e.hw,
		[CLKID_PWM_F_MUX]		= &pwm_f_mux.hw,
		[CLKID_PWM_F_DIV]		= &pwm_f_div.hw,
		[CLKID_PWM_F]			= &pwm_f.hw,
		[CLKID_PWM_G_MUX]		= &pwm_g_mux.hw,
		[CLKID_PWM_G_DIV]		= &pwm_g_div.hw,
		[CLKID_PWM_G]			= &pwm_g.hw,
		[CLKID_PWM_H_MUX]		= &pwm_h_mux.hw,
		[CLKID_PWM_H_DIV]		= &pwm_h_div.hw,
		[CLKID_PWM_H]			= &pwm_h.hw,
		[CLKID_PWM_I_MUX]		= &pwm_i_mux.hw,
		[CLKID_PWM_I_DIV]		= &pwm_i_div.hw,
		[CLKID_PWM_I]			= &pwm_i.hw,
		[CLKID_PWM_J_MUX]		= &pwm_j_mux.hw,
		[CLKID_PWM_J_DIV]		= &pwm_j_div.hw,
		[CLKID_PWM_J]			= &pwm_j.hw,
		[CLKID_SPICC_A_PRE_MUX]		= &spicc_a_pre_mux.hw,
		[CLKID_SPICC_A_PRE_DIV]		= &spicc_a_pre_div.hw,
		[CLKID_SPICC_A_MUX]		= &spicc_a_mux.hw,
		[CLKID_SPICC_A]			= &spicc_a.hw,
		[CLKID_SPICC_B_PRE_MUX]		= &spicc_b_pre_mux.hw,
		[CLKID_SPICC_B_PRE_DIV]		= &spicc_b_pre_div.hw,
		[CLKID_SPICC_B_MUX]		= &spicc_b_mux.hw,
		[CLKID_SPICC_B]			= &spicc_b.hw,
		[CLKID_SPIFC_PRE_MUX]		= &spifc_pre_mux.hw,
		[CLKID_SPIFC_PRE_DIV]		= &spifc_pre_div.hw,
		[CLKID_SPIFC_MUX]		= &spifc_mux.hw,
		[CLKID_SPIFC]			= &spifc.hw,
		[CLKID_TS_DIV]			= &ts_div.hw,
		[CLKID_TS]			= &ts.hw,
		[CLKID_USB_BUS_MUX]		= &usb_bus_mux.hw,
		[CLKID_USB_BUS_DIV]		= &usb_bus_div.hw,
		[CLKID_USB_BUS]			= &usb_bus.hw,
		[CLKID_SD_EMMC_A_PRE_MUX]	= &sd_emmc_a_pre_mux.hw,
		[CLKID_SD_EMMC_A_PRE_DIV]	= &sd_emmc_a_pre_div.hw,
		[CLKID_SD_EMMC_A_MUX]		= &sd_emmc_a_mux.hw,
		[CLKID_SD_EMMC_A]		= &sd_emmc_a.hw,
		[CLKID_SD_EMMC_B_PRE_MUX]	= &sd_emmc_b_pre_mux.hw,
		[CLKID_SD_EMMC_B_PRE_DIV]	= &sd_emmc_b_pre_div.hw,
		[CLKID_SD_EMMC_B_MUX]		= &sd_emmc_b_mux.hw,
		[CLKID_SD_EMMC_B]		= &sd_emmc_b.hw,
		[CLKID_SD_EMMC_C_PRE_MUX]	= &sd_emmc_c_pre_mux.hw,
		[CLKID_SD_EMMC_C_PRE_DIV]	= &sd_emmc_c_pre_div.hw,
		[CLKID_SD_EMMC_C_MUX]		= &sd_emmc_c_mux.hw,
		[CLKID_SD_EMMC_C]		= &sd_emmc_c.hw,
		[CLKID_WAVE_A_PRE_MUX]		= &wave_a_pre_mux.hw,
		[CLKID_WAVE_A_PRE_DIV]		= &wave_a_pre_div.hw,
		[CLKID_WAVE_A_MUX]		= &wave_a_mux.hw,
		[CLKID_WAVE_A]			= &wave_a.hw,
		[CLKID_WAVE_B_PRE_MUX]		= &wave_b_pre_mux.hw,
		[CLKID_WAVE_B_PRE_DIV]		= &wave_b_pre_div.hw,
		[CLKID_WAVE_B_MUX]		= &wave_b_mux.hw,
		[CLKID_WAVE_B]			= &wave_b.hw,
		[CLKID_WAVE_C_PRE_MUX]		= &wave_c_pre_mux.hw,
		[CLKID_WAVE_C_PRE_DIV]		= &wave_c_pre_div.hw,
		[CLKID_WAVE_C_MUX]		= &wave_c_mux.hw,
		[CLKID_WAVE_C]			= &wave_c.hw,
		[CLKID_JPEG_PRE_MUX]		= &jpeg_pre_mux.hw,
		[CLKID_JPEG_PRE_DIV]		= &jpeg_pre_div.hw,
		[CLKID_JPEG_MUX]		= &jpeg_mux.hw,
		[CLKID_JPEG]			= &jpeg.hw,
		[CLKID_MIPI_CSI_PHY_PRE_MUX]	= &mipi_csi_phy_pre_mux.hw,
		[CLKID_MIPI_CSI_PHY_PRE_DIV]	= &mipi_csi_phy_pre_div.hw,
		[CLKID_MIPI_CSI_PHY_MUX]	= &mipi_csi_phy_mux.hw,
		[CLKID_MIPI_CSI_PHY]		= &mipi_csi_phy.hw,
		[CLKID_MIPI_ISP_PRE_MUX]	= &mipi_isp_pre_mux.hw,
		[CLKID_MIPI_ISP_PRE_DIV]	= &mipi_isp_pre_div.hw,
		[CLKID_MIPI_ISP_MUX]		= &mipi_isp_mux.hw,
		[CLKID_MIPI_ISP]		= &mipi_isp.hw,
		[CLKID_NNA_AXI_PRE_MUX]		= &nna_axi_pre_mux.hw,
		[CLKID_NNA_AXI_PRE_DIV]		= &nna_axi_pre_div.hw,
		[CLKID_NNA_AXI_MUX]		= &nna_axi_mux.hw,
		[CLKID_NNA_AXI]			= &nna_axi.hw,
		[CLKID_NNA_CORE_PRE_MUX]	= &nna_core_pre_mux.hw,
		[CLKID_NNA_CORE_PRE_DIV]	= &nna_core_pre_div.hw,
		[CLKID_NNA_CORE_MUX]		= &nna_core_mux.hw,
		[CLKID_NNA_CORE]		= &nna_core.hw,
		[CLKID_GDC_AXI_PRE_MUX]		= &gdc_axi_pre_mux.hw,
		[CLKID_GDC_AXI_PRE_DIV]		= &gdc_axi_pre_div.hw,
		[CLKID_GDC_AXI_MUX]		= &gdc_axi_mux.hw,
		[CLKID_GDC_AXI]			= &gdc_axi.hw,
		[CLKID_GDC_CORE_PRE_MUX]	= &gdc_core_pre_mux.hw,
		[CLKID_GDC_CORE_PRE_DIV]	= &gdc_core_pre_div.hw,
		[CLKID_GDC_CORE_MUX]		= &gdc_core_mux.hw,
		[CLKID_GDC_CORE]		= &gdc_core.hw,
		[CLKID_GE2D_PRE_MUX]		= &ge2d_pre_mux.hw,
		[CLKID_GE2D_PRE_DIV]		= &ge2d_pre_div.hw,
		[CLKID_GE2D_MUX]		= &ge2d_mux.hw,
		[CLKID_GE2D]			= &ge2d.hw,
		[CLKID_ETH_125M_MUX]		= &eth_125m_mux.hw,
		[CLKID_ETH_125M_DIV]		= &eth_125m_div.hw,
		[CLKID_ETH_125M]		= &eth_125m.hw,
		[CLKID_ETH_RMII_MUX]		= &eth_rmii_mux.hw,
		[CLKID_ETH_RMII_DIV]		= &eth_rmii_div.hw,
		[CLKID_ETH_RMII]		= &eth_rmii.hw,
	},
	.num = NR_CLKS,
};

/* Convenience table to populate regmap in .probe */
static struct clk_regmap *const c1_clk_regmaps[] = {
	&gp_pll_div1_gate,
	&gp_pll_div2_gate,
	&gp_pll_div3_gate,
	&fixed_pll_div2_gate,
	&rtc_32k_clkin,
	&rtc_32k_div,
	&rtc_32k_mux,
	&rtc_32k,
	&rtc_clk,
	&sys_a_mux,
	&sys_a_div,
	&sys_a,
	&sys_b_mux,
	&sys_b_div,
	&sys_b,
	&sys_clk,
	&axi_a_mux,
	&axi_a_div,
	&axi_a,
	&axi_b_mux,
	&axi_b_div,
	&axi_b,
	&axi_clk,
	&sys_clk_tree,
	&sys_reset_ctrl,
	&sys_analog_ctrl,
	&sys_pwr_ctrl,
	&sys_pad_ctrl,
	&sys_ctrl,
	&sys_temp_sensor,
	&sys_am2axi_dev,
	&sys_spicc_b,
	&sys_spicc_a,
	&sys_clk_msr,
	&sys_audio,
	&sys_jtag_ctrl,
	&sys_saradc,
	&sys_pwm_ef,
	&sys_pwm_cd,
	&sys_pwm_ab,
	&sys_i2c_s,
	&sys_ir_ctrl,
	&sys_i2c_m_d,
	&sys_i2c_m_c,
	&sys_i2c_m_b,
	&sys_i2c_m_a,
	&sys_acodec,
	&sys_otp,
	&sys_sd_emmc_a,
	&sys_usb_phy,
	&sys_usb_ctrl,
	&sys_dspb,
	&sys_dspa,
	&sys_dma,
	&sys_irq_ctrl,
	&sys_nic,
	&sys_gic,
	&sys_uart_c,
	&sys_uart_b,
	&sys_uart_a,
	&sys_rsa,
	&sys_coresight,
	&sys_csi_phy1,
	&sys_csi_phy0,
	&sys_mipi_isp,
	&sys_csi_dig,
	&sys_ge2d,
	&sys_gdc,
	&sys_dos_apb,
	&sys_nna,
	&sys_eth_phy,
	&sys_eth_mac,
	&sys_uart_e,
	&sys_uart_d,
	&sys_pwm_ij,
	&sys_pwm_gh,
	&sys_i2c_m_e,
	&sys_sd_emmc_c,
	&sys_sd_emmc_b,
	&sys_rom,
	&sys_spifc,
	&sys_prod_i2c,
	&sys_dos,
	&sys_cpu_ctrl,
	&sys_rama,
	&sys_ramb,
	&sys_ramc,
	&axi_am2axi_vad,
	&axi_audio_vad,
	&axi_dmc,
	&axi_ramb,
	&axi_rama,
	&axi_nic,
	&axi_dma,
	&axi_ramc,
	&dspa_a_mux,
	&dspa_a_div,
	&dspa_a,
	&dspa_b_mux,
	&dspa_b_div,
	&dspa_b,
	&dspa_clk,
	&dspa_dspa,
	&dspa_nic,
	&dspb_a_mux,
	&dspb_a_div,
	&dspb_a,
	&dspb_b_mux,
	&dspb_b_div,
	&dspb_b,
	&dspb_clk,
	&dspb_dspa,
	&dspb_nic,
	&clk_24m,
	&clk_12m,
	&fclk_div2_divn_pre,
	&fclk_div2_divn,
	&gen_mux,
	&gen_div,
	&gen,
	&saradc_mux,
	&saradc_div,
	&saradc,
	&pwm_a_mux,
	&pwm_a_div,
	&pwm_a,
	&pwm_b_mux,
	&pwm_b_div,
	&pwm_b,
	&pwm_c_mux,
	&pwm_c_div,
	&pwm_c,
	&pwm_d_mux,
	&pwm_d_div,
	&pwm_d,
	&pwm_e_mux,
	&pwm_e_div,
	&pwm_e,
	&pwm_f_mux,
	&pwm_f_div,
	&pwm_f,
	&pwm_g_mux,
	&pwm_g_div,
	&pwm_g,
	&pwm_h_mux,
	&pwm_h_div,
	&pwm_h,
	&pwm_i_mux,
	&pwm_i_div,
	&pwm_i,
	&pwm_j_mux,
	&pwm_j_div,
	&pwm_j,
	&spicc_a_pre_mux,
	&spicc_a_pre_div,
	&spicc_a_mux,
	&spicc_a,
	&spicc_b_pre_mux,
	&spicc_b_pre_div,
	&spicc_b_mux,
	&spicc_b,
	&spifc_pre_mux,
	&spifc_pre_div,
	&spifc_mux,
	&spifc,
	&ts_div,
	&ts,
	&usb_bus_mux,
	&usb_bus_div,
	&usb_bus,
	&sd_emmc_a_pre_mux,
	&sd_emmc_a_pre_div,
	&sd_emmc_a_mux,
	&sd_emmc_a,
	&sd_emmc_b_pre_mux,
	&sd_emmc_b_pre_div,
	&sd_emmc_b_mux,
	&sd_emmc_b,
	&sd_emmc_c_pre_mux,
	&sd_emmc_c_pre_div,
	&sd_emmc_c_mux,
	&sd_emmc_c,
	&wave_a_pre_mux,
	&wave_a_pre_div,
	&wave_a_mux,
	&wave_a,
	&wave_b_pre_mux,
	&wave_b_pre_div,
	&wave_b_mux,
	&wave_b,
	&wave_c_pre_mux,
	&wave_c_pre_div,
	&wave_c_mux,
	&wave_c,
	&jpeg_pre_mux,
	&jpeg_pre_div,
	&jpeg_mux,
	&jpeg,
	&mipi_csi_phy_pre_mux,
	&mipi_csi_phy_pre_div,
	&mipi_csi_phy_mux,
	&mipi_csi_phy,
	&mipi_isp_pre_mux,
	&mipi_isp_pre_div,
	&mipi_isp_mux,
	&mipi_isp,
	&nna_axi_pre_mux,
	&nna_axi_pre_div,
	&nna_axi_mux,
	&nna_axi,
	&nna_core_pre_mux,
	&nna_core_pre_div,
	&nna_core_mux,
	&nna_core,
	&gdc_axi_pre_mux,
	&gdc_axi_pre_div,
	&gdc_axi_mux,
	&gdc_axi,
	&gdc_core_pre_mux,
	&gdc_core_pre_div,
	&gdc_core_mux,
	&gdc_core,
	&ge2d_pre_mux,
	&ge2d_pre_div,
	&ge2d_mux,
	&ge2d,
	&eth_125m_mux,
	&eth_125m_div,
	&eth_125m,
	&eth_rmii_mux,
	&eth_rmii_div,
	&eth_rmii,
};

static struct clk_regmap *const c1_cpu_clk_regmaps[] = {
	&cpu_dyn_clk,
	&cpu_clk,
	&dsu_dyn_clk,
	&dsu_pre_clk,
	&dsu_clk,
	&dsu_axi_clk,
};

static struct clk_regmap *const c1_pll_regmaps[] = {
#ifndef CONFIG_ARM
	&fixed_pll_dco,
#endif
	&fixed_pll,
	&fclk_div2,
	&fclk_div2p5,
	&fclk_div3,
	&fclk_div4,
	&fclk_div5,
	&fclk_div7,
	&fclk_50m,
#ifndef CONFIG_ARM
	&sys_pll_dco,
#endif
	&sys_pll,
#ifndef CONFIG_ARM
	&gp_pll_dco,
#endif
	&gp_pll,
	&gp_pll_div1,
	&gp_pll_div2,
	&gp_pll_div3,
#ifndef CONFIG_ARM
	&hifi_pll_dco,
#endif
	&hifi_pll,
	&aud_dds,
};

struct sys_pll_nb_data {
	struct notifier_block nb;
	struct clk_hw *sys_pll;
	struct clk_hw *cpu_clk;
	struct clk_hw *cpu_dyn_clk;
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
		if (clk_set_rate(nb_data->cpu_dyn_clk->clk, 1000000000))
			pr_err("%s in %d\n", __func__, __LINE__);
		/* Configure cpu_clk to use cpu_dyn_clk */
		clk_hw_set_parent(nb_data->cpu_clk,
				  nb_data->cpu_dyn_clk);

		/*
		 * Now, cpu_clk uses the dyn path
		 * cpu_clk
		 *    \- cpu_dyn_clk
		 *          \- cpu_dyn_clkX
		 *                \- cpu_dyn_clkX_sel
		 *		     \- cpu_dyn_clkX_div
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
	.cpu_dyn_clk = &cpu_dyn_clk.hw,
	.nb.notifier_call = sys_pll_notifier_cb,
};

static int meson_c1_dvfs_setup(struct platform_device *pdev)
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

static struct regmap *c1_regmap_resource(struct device *dev, char *name)
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

/* 24.567M */
static const struct reg_sequence c1_aud_dds_init_regs[] = {
	{ .reg = ANACTRL_MISCTOP_CTRL0,	.def = 0x4, .delay_us = 20 },
	{ .reg = ANACTRL_AUDDDS_CTRL0,	.def = 0x50021340, .delay_us = 20 },
	{ .reg = ANACTRL_AUDDDS_CTRL1,	.def = 0x0,	   .delay_us = 20 },
	{ .reg = ANACTRL_AUDDDS_CTRL2,	.def = 0x0,	   .delay_us = 20 },
	{ .reg = ANACTRL_AUDDDS_CTRL3,	.def = 0x4f79, .delay_us = 20 },
	{ .reg = ANACTRL_AUDDDS_CTRL4,  .def = 0x3e8, .delay_us = 20 },
	{ .reg = ANACTRL_AUDDDS_CTRL0,  .def = 0x60041f40, .delay_us = 20 },
	{ .reg = ANACTRL_AUDDDS_CTRL1,	.def = 0x80000000, .delay_us = 20 },
};

static void meson_c1_aud_dds_init(void)
{
	regmap_multi_reg_write(aud_dds.map, c1_aud_dds_init_regs,
			       ARRAY_SIZE(c1_aud_dds_init_regs));
}

static int meson_c1_probe(struct platform_device *pdev)
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

	basic_map = c1_regmap_resource(dev, "basic");
	if (IS_ERR(basic_map)) {
		dev_err(dev, "basic clk registers not found\n");
		return PTR_ERR(basic_map);
	}

	pll_map = c1_regmap_resource(dev, "pll");
	if (IS_ERR(pll_map)) {
		dev_err(dev, "pll clk registers not found\n");
		return PTR_ERR(pll_map);
	}

	cpu_clk_map = c1_regmap_resource(dev, "cpu_clk");
	if (IS_ERR(cpu_clk_map)) {
		dev_err(dev, "cpu clk registers not found\n");
		return PTR_ERR(cpu_clk_map);
	}

	/* Populate regmap for the regmap backed clocks */
	for (i = 0; i < ARRAY_SIZE(c1_clk_regmaps); i++)
		c1_clk_regmaps[i]->map = basic_map;

	for (i = 0; i < ARRAY_SIZE(c1_cpu_clk_regmaps); i++)
		c1_cpu_clk_regmaps[i]->map = cpu_clk_map;

	for (i = 0; i < ARRAY_SIZE(c1_pll_regmaps); i++)
		c1_pll_regmaps[i]->map = pll_map;

	for (i = 0; i < c1_hw_onecell_data.num; i++) {
		/* array might be sparse */
		if (!c1_hw_onecell_data.hws[i])
			continue;

		ret = devm_clk_hw_register(dev, c1_hw_onecell_data.hws[i]);
		if (ret) {
			dev_err(dev, "Clock registration failed\n");
			return ret;
		}

#ifdef CONFIG_AMLOGIC_CLK_DEBUG
		ret = devm_clk_hw_register_clkdev(dev, c1_hw_onecell_data.hws[i],
						  NULL,
						  clk_hw_get_name(c1_hw_onecell_data.hws[i]));
		if (ret < 0) {
			dev_err(dev, "Failed to clkdev register: %d\n", ret);
			return ret;
		}
#endif
	}

	meson_c1_dvfs_setup(pdev);
	meson_c1_aud_dds_init();

	return devm_of_clk_add_hw_provider(dev, of_clk_hw_onecell_get,
					   (void *)&c1_hw_onecell_data);
}

static const struct of_device_id clkc_match_table[] = {
	{ .compatible = "amlogic,c1-clkc" },
	{}
};

static struct platform_driver c1_driver = {
	.probe		= meson_c1_probe,
	.driver		= {
		.name	= "c1-clkc",
		.of_match_table = clkc_match_table,
	},
};

builtin_platform_driver(c1_driver);

MODULE_LICENSE("GPL v2");

