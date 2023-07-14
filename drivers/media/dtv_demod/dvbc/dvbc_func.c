// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include "aml_demod.h"
#include "demod_func.h"
#include <linux/kthread.h>

struct timer_list mytimer;

static void dvbc_cci_timer(struct timer_list *timer)
{
}

int dvbc_timer_init(void)
{
	PR_DVBC("%s --\n", __func__);
	timer_setup(&mytimer, dvbc_cci_timer, 0);
	mytimer.expires = jiffies + 2 * HZ;
	add_timer(&mytimer);
	return 0;
}

void dvbc_timer_exit(void)
{
	PR_DVBC("%s --\n", __func__);
	del_timer(&mytimer);
}

int dvbc_set_ch(struct aml_dtvdemod *demod, struct aml_demod_dvbc *demod_dvbc,
		struct dvb_frontend *fe)
{
	int ret = 0;
	u16 symb_rate;
	u8 mode;
	u32 ch_freq;

	PR_DVBC("[id %d] f=%d, s=%d, q=%d\n", demod->id, demod_dvbc->ch_freq,
			demod_dvbc->symb_rate, demod_dvbc->mode);
	mode = demod_dvbc->mode;
	symb_rate = demod_dvbc->symb_rate;
	ch_freq = demod_dvbc->ch_freq;

	if (mode == QAM_MODE_AUTO) {
		/* auto QAM mode, force to QAM256 */
		mode = QAM_MODE_256;
		PR_DVBC("[id %d] auto QAM, set mode %d.\n", demod->id, mode);
	}

	demod->demod_status.ch_mode = mode;
	/* 0:16, 1:32, 2:64, 3:128, 4:256 */
	demod->demod_status.agc_mode = 1;
	/* 0:NULL, 1:IF, 2:RF, 3:both */
	demod->demod_status.ch_freq = ch_freq;
	demod->demod_status.ch_bw = 8000;
	if (demod->demod_status.ch_if == 0)
		demod->demod_status.ch_if = DEMOD_5M_IF;
	demod->demod_status.symb_rate = symb_rate;

	if (!cpu_after_eq(MESON_CPU_MAJOR_ID_TL1) && cpu_after_eq(MESON_CPU_MAJOR_ID_TXLX))
		demod->demod_status.adc_freq = demod_dvbc->dat0;

	demod_dvbc_qam_reset(demod);

	if (is_meson_gxtvbb_cpu() || is_meson_txl_cpu())
		dvbc_reg_initial_old(demod);
	else if (cpu_after_eq(MESON_CPU_MAJOR_ID_TXLX) && !is_meson_txhd_cpu())
		dvbc_reg_initial(demod, fe);

	return ret;
}
