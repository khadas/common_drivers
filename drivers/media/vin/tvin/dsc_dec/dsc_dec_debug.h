/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __DSC_DEC_DEBUG_H__
#define __DSC_DEC_DEBUG_H__

/* ver:20220808: initial version */
#define DSC_DEC_DRV_VERSION  "20210808"

#define DSC_DEC_PR(fmt, args...)      pr_info("dsc_dec: " fmt "", ## args)
#define DSC_DEC_ERR(fmt, args...)     pr_err("dsc_dec error: " fmt "", ## args)

int dsc_dec_debug_file_create(struct aml_dsc_dec_drv_s *dsc_dec_drv);
int dsc_dec_debug_file_remove(struct aml_dsc_dec_drv_s *dsc_dec_drv);

#endif
