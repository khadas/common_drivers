/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __DSC_DEC_CONFIG_H__
#define __DSC_DEC_CONFIG_H__

void init_pps_data(struct aml_dsc_dec_drv_s *dsc_dec_drv);
void init_pps_data_v1(struct aml_dsc_dec_drv_s *dsc_dec_drv);
void dsc_dec_clk_calculate(unsigned int integer, unsigned int frac);

#endif
