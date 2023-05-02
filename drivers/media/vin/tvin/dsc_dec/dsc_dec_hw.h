/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __DSC_DEC_HW_H__
#define __DSC_DEC_HW_H__
#include "dsc_dec_drv.h"

#define CLK_FRACTION 10000

unsigned int R_DSC_DEC_CLKCTRL_REG(unsigned int reg);
void W_DSC_DEC_CLKCTRL_REG(unsigned int reg, unsigned int val);
unsigned int R_DSC_DEC_CLKCTRL_BIT(u32 reg, const u32 start, const u32 len);
void W_DSC_DEC_CLKCTRL_BIT(u32 reg, const u32 value, const u32 start, const u32 len);

unsigned int R_DSC_DEC_REG(unsigned int reg);
void W_DSC_DEC_REG(unsigned int reg, unsigned int val);
unsigned int R_DSC_DEC_BIT(u32 reg, const u32 start, const u32 len);
void W_DSC_DEC_BIT(u32 reg, const u32 value, const u32 start, const u32 len);

void dsc_dec_config_pll_clk(unsigned int od, unsigned int dpll_m,
				unsigned int dpll_n, unsigned int div_frac);
void dsc_dec_config_register(struct aml_dsc_dec_drv_s *dsc_dec_drv);
void set_dsc_dec_en(unsigned int enable);
#endif

