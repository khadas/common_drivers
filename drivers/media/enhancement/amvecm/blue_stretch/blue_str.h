/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef BLUE_STRETCH_H
#define BLUE_STRETCH_H

#define BLUE_STR_GAIN_LUMA_HIGH       BIT(13)
#define BLUE_STR_GAIN_ERR_CBN_INV     BIT(12)
#define BLUE_STR_GAIN_ERR_CBN         BIT(11)
#define BLUE_STR_GAIN_ERR_CBP_INV     BIT(10)
#define BLUE_STR_GAIN_ERR_CBP         BIT(9)
#define BLUE_STR_GAIN_ERR_CRN_INV     BIT(8)
#define BLUE_STR_GAIN_ERR_CRN         BIT(7)
#define BLUE_STR_GAIN_ERR_CRP_INV     BIT(6)
#define BLUE_STR_GAIN_ERR_CRP         BIT(5)
#define BLUE_STR_GAIN_CB4CR           BIT(4)
#define BLUE_STR_GAIN                 BIT(3)
#define BLUE_STR_CB_INC               BIT(2)
#define BLUE_STR_CR_INC               BIT(1)
#define BLUE_STR_EN                   BIT(0)

#define BITDEPTH 10
ssize_t bls_par_show(char *buf);
int bls_par_dbg(char **param);
void set_blue_str_parm(struct blue_str_parm_s *parm);
//void lut3d_yuv2rgb(int en);
extern int bs_proc_en;
void bls_set(void);
void bs_reg_set(void);
#endif
