/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __INC_PORTING_WEAK_H__
#define __INC_PORTING_WEAK_H__
#include <uapi/linux/dvb/frontend.h>
#include <media/dvb_frontend.h>

//#define CONFIG_AMLOGIC_DVB_COMPAT
#ifdef CONFIG_AMLOGIC_DVB_COMPAT

#define DTV_DVBT2_PLP_ID    DTV_DVBT2_PLP_ID_LEGACY
/* Get tne TS input of the frontend */
#define DTV_TS_INPUT                    100
/* Blind scan */
#define DTV_START_BLIND_SCAN            101
#define DTV_CANCEL_BLIND_SCAN           102
#define DTV_BLIND_SCAN_MIN_FRE          103
#define DTV_BLIND_SCAN_MAX_FRE          104
#define DTV_BLIND_SCAN_MIN_SRATE        105
#define DTV_BLIND_SCAN_MAX_SRATE        106
#define DTV_BLIND_SCAN_FRE_RANGE        107
#define DTV_BLIND_SCAN_FRE_STEP         108
#define DTV_BLIND_SCAN_TIMEOUT          109
/* Blind scan end*/
#define DTV_DELIVERY_SUB_SYSTEM                 110
#undef DTV_MAX_COMMAND
#define DTV_MAX_COMMAND         DTV_DELIVERY_SUB_SYSTEM
#else /*!defined(CONFIG_AMLOGIC_DVB_COMPAT)*/
#undef DTV_MAX_COMMAND
#define DTV_MAX_COMMAND         DTV_SCRAMBLING_SEQUENCE_INDEX
#endif /*CONFIG_AMLOGIC_DVB_COMPAT*/


struct dma_buf * __weak ion_alloc(size_t len, unsigned int heap_id_mask,
			  unsigned int flags)
{
	return NULL;
}

void __weak dvb_frontend_add_event(struct dvb_frontend *fe,
			    enum fe_status status)
{
	return;
}

#endif
