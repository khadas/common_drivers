// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#define __DVB_CORE__	/*ary 2018-1-31*/

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/pm_domain.h>
#include <linux/firmware.h>
#include <linux/err.h>	/*IS_ERR*/
#include <linux/clk.h>	/*clk tree*/
#include <linux/of_device.h>
#include <linux/of_reserved_mem.h>
#include <linux/of_irq.h>
#include <linux/interrupt.h>
#include <linux/crc32.h>

#ifdef ARC_700
#include <asm/arch/am_regs.h>
#else
/* #include <mach/am_regs.h> */
#endif
#include <linux/i2c.h>
#include <linux/gpio.h>

#include "aml_demod.h"
#include "demod_func.h"
#include "demod_dbg.h"
#include "dtmb_frontend.h"
#include "amlfrontend.h"
#include "addr_dtmb_top.h"
#include <linux/amlogic/aml_dtvdemod.h>

void gxtv_demod_dtmb_release(struct dvb_frontend *fe)
{
}

int gxtv_demod_dtmb_read_status(struct dvb_frontend *fe, enum fe_status *status)
{
	struct aml_dtvdemod *demod = (struct aml_dtvdemod *)fe->demodulator_priv;
	struct poll_machie_s *pollm = &demod->poll_machie;

	*status = pollm->last_s;

	return 0;
}

void dtmb_save_status(struct aml_dtvdemod *demod, unsigned int s)
{
	struct poll_machie_s *pollm = &demod->poll_machie;

	if (s) {
		pollm->last_s =
			FE_HAS_LOCK | FE_HAS_SIGNAL | FE_HAS_CARRIER |
			FE_HAS_VITERBI | FE_HAS_SYNC;

	} else {
		pollm->last_s = FE_TIMEDOUT;
	}
}

void dtmb_poll_start(struct aml_dtvdemod *demod)
{
	struct poll_machie_s *pollm = &demod->poll_machie;

	pollm->last_s = 0;
	pollm->flg_restart = 1;
	PR_DTMB("dtmb_poll_start2\n");
}

void dtmb_poll_stop(struct aml_dtvdemod *demod)
{
	struct poll_machie_s *pollm = &demod->poll_machie;

	pollm->flg_stop = 1;
}

void dtmb_set_delay(struct aml_dtvdemod *demod, unsigned int delay)
{
	struct poll_machie_s *pollm = &demod->poll_machie;

	pollm->delayms = delay;
	pollm->flg_updelay = 1;
}

unsigned int dtmb_is_update_delay(struct aml_dtvdemod *demod)
{
	struct poll_machie_s *pollm = &demod->poll_machie;

	return pollm->flg_updelay;
}

unsigned int dtmb_get_delay_clear(struct aml_dtvdemod *demod)
{
	struct poll_machie_s *pollm = &demod->poll_machie;

	pollm->flg_updelay = 0;

	return pollm->delayms;
}

void dtmb_poll_clear(struct aml_dtvdemod *demod)
{
	struct poll_machie_s *pollm = &demod->poll_machie;

	memset(pollm, 0, sizeof(struct poll_machie_s));
}

/*dtmb_poll_v3 is same as dtmb_check_status_txl*/
void dtmb_poll_v3(struct aml_dtvdemod *demod)
{
	struct poll_machie_s *pollm = &demod->poll_machie;
	unsigned int bch_tmp;
	unsigned int s;

	if (!pollm->state) {
		/* idle */
		/* idle -> start check */
		if (!pollm->flg_restart) {
			PR_DBG("x");
			return;
		}
	} else {
		if (pollm->flg_stop) {
			PR_DBG("dtmb poll stop !\n");
			dtmb_poll_clear(demod);
			dtmb_set_delay(demod, 3 * HZ);

			return;
		}
	}

	/* restart: clear */
	if (pollm->flg_restart) {
		PR_DBG("dtmb poll restart!\n");
		dtmb_poll_clear(demod);
	}
	PR_DBG("-");
	s = check_dtmb_fec_lock();

	/* bch exceed the threshold: wait lock*/
	if (pollm->state & DTMBM_BCH_OVER_CHEK) {
		if (pollm->crrcnt < DTMBM_POLL_CNT_WAIT_LOCK) {
			pollm->crrcnt++;

			if (s) {
				PR_DBG("after reset get lock again!cnt=%d\n",
					pollm->crrcnt);
				dtmb_constell_check();
				pollm->state = DTMBM_HV_SIGNEL_CHECK;
				pollm->crrcnt = 0;
				pollm->bch = dtmb_reg_r_bch();

				dtmb_save_status(demod, s);
			}
		} else {
			PR_DBG("can't lock after reset!\n");
			pollm->state = DTMBM_NO_SIGNEL_CHECK;
			pollm->crrcnt = 0;
			/* to no signal*/
			dtmb_save_status(demod, s);
		}
		return;
	}

	if (s) {
		/*have signal*/
		if (!pollm->state) {
			pollm->state = DTMBM_CHEK_NO;
			PR_DBG("from idle to have signal wait 1\n");
			return;
		}
		if (pollm->state & DTMBM_CHEK_NO) {
			/*no to have*/
			PR_DBG("poll machie: from no signal to have signal\n");
			pollm->bch = dtmb_reg_r_bch();
			pollm->state = DTMBM_HV_SIGNEL_CHECK;

			dtmb_set_delay(demod, DTMBM_POLL_DELAY_HAVE_SIGNAL);
			return;
		}

		bch_tmp = dtmb_reg_r_bch();
		if (bch_tmp > (pollm->bch + 50)) {
			pollm->state = DTMBM_BCH_OVER_CHEK;

			PR_DBG("bch add ,need reset,wait not to reset\n");
			dtmb_reset();

			pollm->crrcnt = 0;
			dtmb_set_delay(demod, DTMBM_POLL_DELAY_HAVE_SIGNAL);
		} else {
			pollm->bch = bch_tmp;
			pollm->state = DTMBM_HV_SIGNEL_CHECK;

			dtmb_save_status(demod, s);
			/*have signale to have signal*/
			dtmb_set_delay(demod, 300);
		}
		return;
	}

	/*no signal */
	if (!pollm->state) {
		/* idle -> no signal */
		PR_DBG("poll machie: from idle to no signal\n");
		pollm->crrcnt = 0;

		pollm->state = DTMBM_NO_SIGNEL_CHECK;
	} else if (pollm->state & DTMBM_CHEK_HV) {
		/*have signal -> no signal*/
		PR_DBG("poll machie: from have signal to no signal\n");
		pollm->crrcnt = 0;
		pollm->state = DTMBM_NO_SIGNEL_CHECK;
		dtmb_save_status(demod, s);
	}

	/*no signal check process */
	if (pollm->crrcnt < DTMBM_POLL_CNT_NO_SIGNAL) {
		dtmb_no_signal_check_v3(demod);
		pollm->crrcnt++;

		dtmb_set_delay(demod, DTMBM_POLL_DELAY_NO_SIGNAL);
	} else {
		dtmb_no_signal_check_finishi_v3(demod);
		pollm->crrcnt = 0;

		dtmb_save_status(demod, s);
		/*no signal to no signal*/
		dtmb_set_delay(demod, 300);
	}
}

void dtmb_poll_start_tune(struct aml_dtvdemod *demod, unsigned int state)
{
	struct poll_machie_s *pollm = &demod->poll_machie;

	dtmb_poll_clear(demod);

	pollm->state = state;
	if (state & DTMBM_NO_SIGNEL_CHECK)
		dtmb_save_status(demod, 0);
	else
		dtmb_save_status(demod, 1);

	PR_DTMB("dtmb_poll_start tune to %d\n", state);
}

/*come from gxtv_demod_dtmb_read_status, have ms_delay*/
int dtmb_poll_v2(struct dvb_frontend *fe, enum fe_status *status)
{
	struct aml_dtvdemod *demod = (struct aml_dtvdemod *)fe->demodulator_priv;
	int ilock;
	unsigned char s = 0;

	s = dtmb_check_status_gxtv(fe);

	if (s == 1) {
		ilock = 1;
		*status =
			FE_HAS_LOCK | FE_HAS_SIGNAL | FE_HAS_CARRIER |
			FE_HAS_VITERBI | FE_HAS_SYNC;
	} else {
		ilock = 0;
		*status = FE_TIMEDOUT;
	}

	if (demod->last_lock != ilock) {
		PR_INFO("%s [id %d]: %s.\n", __func__, demod->id,
			 ilock ? "!!  >> LOCK << !!" : "!! >> UNLOCK << !!");
		demod->last_lock = ilock;
	}

	return 0;
}

/*this is ori gxtv_demod_dtmb_read_status*/
int gxtv_demod_dtmb_read_status_old(struct dvb_frontend *fe,
		enum fe_status *status)
{
	struct aml_dtvdemod *demod = (struct aml_dtvdemod *)fe->demodulator_priv;
	int ilock;
	unsigned char s = 0;

	if (is_meson_gxtvbb_cpu()) {
#ifndef CONFIG_AMLOGIC_REMOVE_OLD
		dtmb_check_status_gxtv(fe);
#endif
	} else if (cpu_after_eq(MESON_CPU_MAJOR_ID_TXL)) {
		dtmb_bch_check(fe);
		if (!cpu_after_eq(MESON_CPU_MAJOR_ID_TL1))
			dtmb_check_status_txl(fe);
	} else {
		return -1;
	}

	s = amdemod_stat_dtmb_islock(demod, SYS_DTMB);

	if (s == 1) {
		ilock = 1;
		*status =
			FE_HAS_LOCK | FE_HAS_SIGNAL | FE_HAS_CARRIER |
			FE_HAS_VITERBI | FE_HAS_SYNC;
	} else {
		ilock = 0;
		*status = FE_TIMEDOUT;
	}
	if (demod->last_lock != ilock) {
		PR_INFO("%s [%d]: %s.\n", __func__, demod->id,
			 ilock ? "!!  >> LOCK << !!" : "!! >> UNLOCK << !!");
		demod->last_lock = ilock;
	}

	return 0;
}

int gxtv_demod_dtmb_read_ber(struct dvb_frontend *fe, u32 *ber)
{
	unsigned int value = 0;

	value = dtmb_read_reg(DTMB_TOP_FEC_LDPC_IT_AVG);
	value = (value >> 16) & 0x1fff;
	*ber = 1000 * value / (8 * 188 * 8); // x 1e6.

	PR_DTMB("%s: value %d, ber %d.\n", __func__, value, *ber);

	return 0;
}

int gxtv_demod_dtmb_read_signal_strength(struct dvb_frontend *fe,
		s16 *strength)
{
	int tuner_sr = tuner_get_ch_power(fe);

	if (tuner_find_by_name(fe, "r842"))
		tuner_sr += 16;

	*strength = (s16)tuner_sr;

	return 0;
}

int gxtv_demod_dtmb_read_snr(struct dvb_frontend *fe, u16 *snr)
{
	int tmp = 00;

	/* tmp = dtmb_read_reg(DTMB_TOP_FEC_LOCK_SNR);*/
	tmp = dtmb_reg_r_che_snr();

	*snr = convert_snr(tmp) * 10;

	PR_DTMB("demod snr is %d.%d\n", *snr / 10, *snr % 10);

	return 0;
}

int gxtv_demod_dtmb_read_ucblocks(struct dvb_frontend *fe, u32 *ucblocks)
{
	*ucblocks = 0;
	return 0;
}

int gxtv_demod_dtmb_set_frontend(struct dvb_frontend *fe)
{
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	struct aml_dtvdemod *demod = (struct aml_dtvdemod *)fe->demodulator_priv;
	struct amldtvdemod_device_s *devp = (struct amldtvdemod_device_s *)demod->priv;
	struct aml_demod_dtmb param;
	int times;
	/*[0]: spectrum inverse(1),normal(0); [1]:if_frequency*/
	unsigned int tuner_freq[2] = {0};

	PR_INFO("%s [id %d]: delsys:%d, freq:%d, symbol_rate:%d, bw:%d, modul:%d, invert:%d.\n",
			__func__, demod->id, c->delivery_system, c->frequency, c->symbol_rate,
			c->bandwidth_hz, c->modulation, c->inversion);

	if (!devp->demod_thread)
		return 0;

	times = 2;
	memset(&param, 0, sizeof(param));
	param.ch_freq = c->frequency / 1000;

	demod->last_lock = -1;

#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
	if (is_meson_t3_cpu()) {
		PR_DTMB("dtmb set ddr\n");
		dtmb_write_reg(0x7, 0x6ffffd);
		//dtmb_write_reg(0x47, 0xed33221);
		dtmb_write_reg_bits(0x47, 0x1, 22, 1);
		dtmb_write_reg_bits(0x47, 0x1, 23, 1);

		if (is_meson_t3_cpu() && is_meson_rev_b())
			t3_revb_set_ambus_state(false, false);
	}
#endif

	tuner_set_params(fe);
	msleep(100);

	if (cpu_after_eq(MESON_CPU_MAJOR_ID_TL1)) {
		if (fe->ops.tuner_ops.get_if_frequency)
			fe->ops.tuner_ops.get_if_frequency(fe, tuner_freq);
		if (tuner_freq[0] == 0)
			demod->demod_status.spectrum = 0;
		else if (tuner_freq[0] == 1)
			demod->demod_status.spectrum = 1;
		else
			pr_err("wrong spectrum val get from tuner\n");
	}

	dtmb_set_ch(demod, &param);

	return 0;
}

int gxtv_demod_dtmb_get_frontend(struct dvb_frontend *fe)
{
	/* these content will be written into eeprom. */

	return 0;
}

int gxtv_demod_dtmb_tune(struct dvb_frontend *fe, bool re_tune,
	unsigned int mode_flags, unsigned int *delay, enum fe_status *status)
{
	int ret = 0;
	unsigned int firstdetect = 0;
	struct aml_dtvdemod *demod = (struct aml_dtvdemod *)fe->demodulator_priv;

	if (re_tune) {
		/*first*/
		demod->en_detect = 1;

		*delay = HZ / 4;
		gxtv_demod_dtmb_set_frontend(fe);
		firstdetect = dtmb_detect_first();

		if (firstdetect == 1) {
			*status = FE_TIMEDOUT;
			/*polling mode*/
			dtmb_poll_start_tune(demod, DTMBM_NO_SIGNEL_CHECK);

		} else if (firstdetect == 2) {  /*need check*/
			*status = FE_HAS_LOCK | FE_HAS_SIGNAL | FE_HAS_CARRIER |
			FE_HAS_VITERBI | FE_HAS_SYNC;
			dtmb_poll_start_tune(demod, DTMBM_HV_SIGNEL_CHECK);

		} else if (firstdetect == 0) {
			PR_DTMB("[id %d] use read_status\n", demod->id);
			gxtv_demod_dtmb_read_status_old(fe, status);
			if (*status == (0x1f))
				dtmb_poll_start_tune(demod, DTMBM_HV_SIGNEL_CHECK);
			else
				dtmb_poll_start_tune(demod, DTMBM_NO_SIGNEL_CHECK);
		}

		PR_DTMB("[id %d] tune finish!\n", demod->id);

		return ret;
	}

	if (!demod->en_detect) {
		PR_DBGL("%s: [id %d] not enable.\n", __func__, demod->id);
		return ret;
	}

	*delay = HZ / 4;
	gxtv_demod_dtmb_read_status_old(fe, status);

	if (*status == (FE_HAS_LOCK | FE_HAS_SIGNAL | FE_HAS_CARRIER |
		FE_HAS_VITERBI | FE_HAS_SYNC))
		dtmb_poll_start_tune(demod, DTMBM_HV_SIGNEL_CHECK);
	else
		dtmb_poll_start_tune(demod, DTMBM_NO_SIGNEL_CHECK);

	return ret;
}

int Gxtv_Demod_Dtmb_Init(struct aml_dtvdemod *demod)
{
	int ret = 0;
	struct aml_demod_sys sys;
	struct amldtvdemod_device_s *devp = (struct amldtvdemod_device_s *)demod->priv;
	struct ddemod_dig_clk_addr *dig_clk;

	PR_DBG("AML Demod DTMB init\r\n");
	dig_clk = &devp->data->dig_clk;
	memset(&sys, 0, sizeof(sys));
	memset(&demod->demod_status, 0, sizeof(demod->demod_status));
	demod->demod_status.delsys = SYS_DTMB;

	if (is_meson_gxtvbb_cpu()) {
#ifndef CONFIG_AMLOGIC_REMOVE_OLD
		sys.adc_clk = ADC_CLK_25M;
		sys.demod_clk = DEMOD_CLK_200M;
#endif
	} else if (cpu_after_eq(MESON_CPU_MAJOR_ID_TXL)) {
		if (is_meson_txl_cpu()) {
			sys.adc_clk = ADC_CLK_25M;
			sys.demod_clk = DEMOD_CLK_225M;
		} else if (cpu_after_eq(MESON_CPU_MAJOR_ID_TL1)) {
			sys.adc_clk = ADC_CLK_24M;
			sys.demod_clk = DEMOD_CLK_250M;
		} else {
			sys.adc_clk = ADC_CLK_24M;
			sys.demod_clk = DEMOD_CLK_225M;
		}
	} else {
		return -1;
	}

	demod->demod_status.ch_if = DEMOD_5M_IF;
	demod->demod_status.tmp = ADC_MODE;
	demod->demod_status.adc_freq = sys.adc_clk;
	demod->demod_status.clk_freq = sys.demod_clk;
	demod->last_status = 0;

	if (devp->data->hw_ver >= DTVDEMOD_HW_TL1)
		dd_hiu_reg_write(dig_clk->demod_clk_ctl, 0x501);

	ret = demod_set_sys(demod, &sys);

	return ret;
}

int amdemod_stat_dtmb_islock(struct aml_dtvdemod *demod,
		enum fe_delivery_system delsys)
{
	unsigned int ret = 0;

	ret = dtmb_reg_r_fec_lock();
	return ret;
}

