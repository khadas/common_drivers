/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __J83B_FUNC_H__
#define __J83B_FUNC_H__

/*#include "demod_func.h"*/
#define J83B_QAM_MODE_CFG		0x2
#define J83B_SYMB_CNT_CFG		0x3
#define J83B_SR_OFFSET_ACC		0x8
#define J83B_SR_SCAN_SPEED		0xc
#define J83B_TIM_SWEEP_RANGE_CFG	0xe

u32 atsc_j83b_get_status(struct aml_dtvdemod *demod);
u32 atsc_j83b_get_ch_sts(struct aml_dtvdemod *demod);
unsigned int atsc_j83b_read_iqr_reg(void);
u32 atsc_j83b_get_snr(struct aml_dtvdemod *demod);
u32 atsc_j83b_get_per(struct aml_dtvdemod *demod);
void demod_j83b_fsm_reset(struct aml_dtvdemod *demod);
u32 atsc_j83b_get_symb_rate(struct aml_dtvdemod *demod);
void demod_atsc_j83b_set_qam(struct aml_dtvdemod *demod, enum qam_md_e qam, bool auto_sr);
int atsc_j83b_status(struct aml_dtvdemod *demod,
	struct aml_demod_sts *demod_sts, struct seq_file *seq);
int atsc_j83b_set_ch(struct aml_dtvdemod *demod, struct aml_demod_dvbc *demod_dvbc,
	struct dvb_frontend *fe);
void demod_atsc_j83b_store_qam_cfg(struct aml_dtvdemod *demod);
void demod_atsc_j83b_restore_qam_cfg(struct aml_dtvdemod *demod);

#endif
