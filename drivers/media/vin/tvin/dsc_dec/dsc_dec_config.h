/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __DSC_DEC_CONFIG_H__
#define __DSC_DEC_CONFIG_H__

void init_pps_data_4k_120hz(struct aml_dsc_dec_drv_s *dsc_dec_drv);
void init_pps_data_4k_60hz(struct aml_dsc_dec_drv_s *dsc_dec_drv);
void init_pps_data_8k_30hz(struct aml_dsc_dec_drv_s *dsc_dec_drv);
void init_pps_data_8k_60hz_8bpc(struct aml_dsc_dec_drv_s *dsc_dec_drv);
void init_pps_data_8k_60hz_10bpc(struct aml_dsc_dec_drv_s *dsc_dec_drv);
void dsc_dec_clk_calculate(unsigned int integer, unsigned int frac);
void dsc_dec_config_init(struct aml_dsc_dec_drv_s *dsc_dec_drv);

#endif
