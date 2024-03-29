/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __J83B_FRONTEND_H__

#define __J83B_FRONTEND_H__

int dtvdemod_atsc_j83b_init(struct aml_dtvdemod *demod);
int gxtv_demod_atsc_j83b_set_frontend(struct dvb_frontend *fe);
int gxtv_demod_atsc_j83b_get_frontend(struct dvb_frontend *fe);
int gxtv_demod_atsc_j83b_read_ber(struct dvb_frontend *fe, u32 *ber);
int gxtv_demod_atsc_j83b_read_signal_strength(struct dvb_frontend *fe,
	s16 *strength);
int gxtv_demod_atsc_j83b_read_snr(struct dvb_frontend *fe, u16 *snr);
int gxtv_demod_atsc_j83b_read_ucblocks(struct dvb_frontend *fe, u32 *ucblocks);
int gxtv_demod_atsc_j83b_read_status(struct dvb_frontend *fe,
	enum fe_status *status);
void atsc_j83b_switch_qam(struct dvb_frontend *fe, enum qam_md_e qam);
void gxtv_demod_atsc_j83b_release(struct dvb_frontend *fe);
int atsc_j83b_read_status(struct dvb_frontend *fe, enum fe_status *status, bool re_tune);
int atsc_j83b_set_frontend_mode(struct dvb_frontend *fe, int mode);
int atsc_j83b_polling(struct dvb_frontend *fe, enum fe_status *s);
int gxtv_demod_atsc_j83b_tune(struct dvb_frontend *fe, bool re_tune,
	unsigned int mode_flags, unsigned int *delay, enum fe_status *status);
int amdemod_stat_j83b_islock(struct aml_dtvdemod *demod,
	enum fe_delivery_system delsys);

#endif
