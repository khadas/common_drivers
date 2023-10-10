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
#include "amlfrontend.h"
#include "dvbc_frontend.h"
#include <linux/amlogic/aml_dtvdemod.h>

MODULE_PARM_DESC(dvbc_lock_continuous_cnt, "\n\t\t dvbc lock signal continuous counting");
static unsigned int dvbc_lock_continuous_cnt = 1;
module_param(dvbc_lock_continuous_cnt, int, 0644);

MODULE_PARM_DESC(dvbc_lost_continuous_cnt, "\n\t\t dvbc lost signal continuous counting");
static unsigned int dvbc_lost_continuous_cnt = 5;
module_param(dvbc_lost_continuous_cnt, int, 0644);

void dvbc_get_qam_name(enum qam_md_e qam_mode, char *str)
{
	switch (qam_mode) {
	case QAM_MODE_64:
		strcpy(str, "QAM_MODE_64");
		break;

	case QAM_MODE_256:
		strcpy(str, "QAM_MODE_256");
		break;

	case QAM_MODE_16:
		strcpy(str, "QAM_MODE_16");
		break;

	case QAM_MODE_32:
		strcpy(str, "QAM_MODE_32");
		break;

	case QAM_MODE_128:
		strcpy(str, "QAM_MODE_128");
		break;

	default:
		strcpy(str, "QAM_MODE UNKNOWN");
		break;
	}
}

void store_dvbc_qam_mode(struct aml_dtvdemod *demod,
		int qam_mode, int symbolrate)
{
	int qam_para;

	switch (qam_mode) {
	case QAM_16:
		qam_para = 4;
		break;
	case QAM_32:
		qam_para = 5;
		break;
	case QAM_64:
		qam_para = 6;
		break;
	case QAM_128:
		qam_para = 7;
		break;
	case QAM_256:
		qam_para = 8;
		break;
	case QAM_AUTO:
		qam_para = 6;
		break;
	default:
		qam_para = 6;
		break;
	}

	demod->para_demod.dvbc_qam = qam_para;
	demod->para_demod.dvbc_symbol = symbolrate;
}

void gxtv_demod_dvbc_release(struct dvb_frontend *fe)
{
}

int gxtv_demod_dvbc_read_status_timer(struct dvb_frontend *fe,
	enum fe_status *status)
{
	struct aml_dtvdemod *demod = (struct aml_dtvdemod *)fe->demodulator_priv;
	struct aml_demod_sts demod_sts;
	s16 strength = 0;
	int ilock = 0;

	gxtv_demod_dvbc_read_signal_strength(fe, &strength);
	if (strength < THRD_TUNER_STRENGTH_DVBC) {
		PR_DVBC("%s: tuner strength [%d] no signal(%d).\n",
				__func__, strength, THRD_TUNER_STRENGTH_DVBC);
		*status = FE_TIMEDOUT;
		return 0;
	}

	dvbc_status(demod, &demod_sts, NULL);
	demod->real_para.snr = demod_sts.ch_snr;

	if (demod_sts.ch_sts & 0x1) {
		ilock = 1;
		*status =
			FE_HAS_LOCK | FE_HAS_SIGNAL | FE_HAS_CARRIER |
			FE_HAS_VITERBI | FE_HAS_SYNC;
	} else {
		ilock = 0;

		if (timer_not_enough(demod, D_TIMER_DETECT)) {
			*status = 0;
			PR_TIME("s=0\n");
		} else {
			*status = FE_TIMEDOUT;
			timer_disable(demod, D_TIMER_DETECT);
		}
	}

	if (demod->last_lock != ilock) {
		PR_DBG("%s [id %d]: %s.\n", __func__, demod->id,
			ilock ? "!!  >> LOCK << !!" : "!! >> UNLOCK << !!");
		demod->last_lock = ilock;
		if (ilock == 0) {
			demod->fast_search_finish = false;
			demod->fsm_reset = false;
			if (cpu_after_eq(MESON_CPU_MAJOR_ID_T5W) &&
				!is_meson_s1a_cpu() &&
				demod->auto_qam_done &&
				fe->dtv_property_cache.modulation == QAM_AUTO) {
				demod->auto_qam_mode = QAM_MODE_256;
				demod_dvbc_set_qam(demod, demod->auto_qam_mode, false);
				demod->auto_qam_done = false;
				demod->auto_qam_index = 0;
				demod->auto_times = 0;
				demod->auto_done_times = 0;
			}
		}
	}

	return 0;
}

int gxtv_demod_dvbc_read_ber(struct dvb_frontend *fe, u32 *ber)
{
	struct aml_demod_sts demod_sts;
	struct aml_dtvdemod *demod = (struct aml_dtvdemod *)fe->demodulator_priv;

	dvbc_status(demod, &demod_sts, NULL);
	*ber = demod_sts.ch_per;

	return 0;
}

int gxtv_demod_dvbc_read_signal_strength(struct dvb_frontend *fe,
	s16 *strength)
{
	struct aml_dtvdemod *demod = (struct aml_dtvdemod *)fe->demodulator_priv;
	unsigned int agc_gain = 0;

	*strength = (s16)tuner_get_ch_power(fe);
	if (tuner_find_by_name(fe, "r842") ||
		tuner_find_by_name(fe, "r836") ||
		tuner_find_by_name(fe, "r850")) {
		*strength += 22;

		if (*strength <= -80) {
			agc_gain = qam_read_reg(demod, 0x27) & 0x7ff;
			*strength = dvbc_get_power_strength(agc_gain, *strength);
		}

		*strength += 10;
	} else if (tuner_find_by_name(fe, "mxl661")) {
		*strength += 3;
	}

	PR_DVBC("demod [id %d] signal strength %d dBm\n", demod->id, *strength);

	return 0;
}

int gxtv_demod_dvbc_read_snr(struct dvb_frontend *fe, u16 *snr)
{
	struct aml_dtvdemod *demod = (struct aml_dtvdemod *)fe->demodulator_priv;

	*snr = demod->real_para.snr;

	PR_DVBC("demod[%d] snr %d dBx10\n", demod->id, *snr);

	return 0;
}

int gxtv_demod_dvbc_read_ucblocks(struct dvb_frontend *fe, u32 *ucblocks)
{
	*ucblocks = 0;
	return 0;
}

int gxtv_demod_dvbc_init(struct aml_dtvdemod *demod, int mode)
{
	int ret = 0;
	struct aml_demod_sys sys;
	struct amldtvdemod_device_s *devp = (struct amldtvdemod_device_s *)demod->priv;
	struct ddemod_dig_clk_addr *dig_clk = &devp->data->dig_clk;

	memset(&sys, 0, sizeof(sys));
	memset(&demod->demod_status, 0, sizeof(demod->demod_status));

	demod->demod_status.delsys = SYS_DVBC_ANNEX_A;

	if (mode == ADC_MODE) {
		sys.adc_clk = ADC_CLK_25M;
		sys.demod_clk = DEMOD_CLK_200M;
		demod->demod_status.tmp = ADC_MODE;
	} else {
		sys.adc_clk = ADC_CLK_24M;
		sys.demod_clk = DEMOD_CLK_72M;
		demod->demod_status.tmp = CRY_MODE;
	}

	if (cpu_after_eq(MESON_CPU_MAJOR_ID_TL1)) {
		sys.adc_clk = ADC_CLK_24M;
		sys.demod_clk = DEMOD_CLK_167M;
		demod->demod_status.tmp = CRY_MODE;
	}

	demod->demod_status.ch_if = DEMOD_5M_IF;
	demod->auto_flags_trig = 0;
	demod->demod_status.adc_freq = sys.adc_clk;
	demod->demod_status.clk_freq = sys.demod_clk;
	demod->last_status = 0;

	if (devp->dvbc_inited)
		return 0;

	devp->dvbc_inited = true;

	PR_DBG("[%s] adc_clk is %d, demod_clk is %d, Pll_Mode is %d\n",
			__func__, sys.adc_clk, sys.demod_clk, demod->demod_status.tmp);

	/* sys clk div */
	if (devp->data->hw_ver == DTVDEMOD_HW_S4 || devp->data->hw_ver == DTVDEMOD_HW_S4D) {
		//for new dvbc_blind_scan mode
		if (is_meson_s4d_cpu() && !devp->blind_scan_stop) {
			//CLKCTRL_DEMOD_CLK_CNTL,CLKCTRL_DEMOD_CLK_CNTL1
			//cts_demod_core_clk=24M,cts_demod_core_t2_clk=48M
			dd_hiu_reg_write(0x80, 0x70f);
			dd_hiu_reg_write(0x81, 0x707);
		} else {
			dd_hiu_reg_write(0x80, 0x501); //250M
		}
	} else if (devp->data->hw_ver >= DTVDEMOD_HW_TL1) {
		dd_hiu_reg_write(dig_clk->demod_clk_ctl, 0x502);
	}

	ret = demod_set_sys(demod, &sys);

	return ret;
}

int gxtv_demod_dvbc_set_frontend(struct dvb_frontend *fe)
{
	struct aml_dtvdemod *demod = (struct aml_dtvdemod *)fe->demodulator_priv;
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	struct aml_demod_dvbc param;    /*mode 0:16, 1:32, 2:64, 3:128, 4:256*/
	struct aml_demod_sts demod_sts;
	int ret = 0;

	PR_INFO("%s [id %d]: delsys:%d, freq:%d, symbol_rate:%d, bw:%d, modul:%d.\n",
			__func__, demod->id, c->delivery_system, c->frequency, c->symbol_rate,
			c->bandwidth_hz, c->modulation);

	memset(&param, 0, sizeof(param));
	param.ch_freq = c->frequency / 1000;
	param.mode = amdemod_qam(c->modulation);
	param.symb_rate = c->symbol_rate / 1000;

	if (demod->symb_rate_en)
		param.symb_rate = demod->symbol_rate_manu;

	/* symbol rate, 0=auto, set to max sr to track by hw */
	if (!param.symb_rate) {
		param.symb_rate = 7250;
		demod->auto_sr = 1;
		PR_DVBC("auto sr mode, set sr=%d\n", param.symb_rate);
	} else {
		demod->auto_sr = 0;
	}

	demod->sr_val_hw = param.symb_rate;

	store_dvbc_qam_mode(demod, c->modulation, param.symb_rate);

	if (!cpu_after_eq(MESON_CPU_MAJOR_ID_TL1)) {
		if (param.mode == 3 && demod->demod_status.tmp != ADC_MODE)
			ret = gxtv_demod_dvbc_init(demod, ADC_MODE);
	}

	if (demod->autoflags == 1 && demod->auto_flags_trig == 0 &&
		demod->freq_dvbc == param.ch_freq) {
		PR_DBG("now is auto symbrating\n");
		return 0;
	}

	demod->auto_flags_trig = 0;
	demod->last_lock = -1;
	demod->last_qam_mode = QAM_MODE_NUM;

	tuner_set_params(fe);

	dvbc_set_ch(demod, &param, fe);

	/*0xf33 dvbc mode, 0x10f33 j.83b mode*/
	if (cpu_after_eq(MESON_CPU_MAJOR_ID_TXLX) && !is_meson_txhd_cpu())
		dvbc_init_reg_ext(demod);

	if (demod->autoflags == 1) {
		PR_DBG("QAM_PLAYING mode,start auto sym\n");
		dvbc_set_auto_symtrack(demod);
		/* flag=1;*/
	}

	demod_dvbc_store_qam_cfg(demod);

	/* auto QAM mode, force to QAM256 */
	if (param.mode == QAM_MODE_AUTO) {
		demod_dvbc_set_qam(demod, QAM_MODE_256, demod->auto_sr);
		demod->auto_qam_mode = QAM_MODE_256;
	} else {
		demod_dvbc_set_qam(demod, param.mode, demod->auto_sr);
		demod->auto_qam_mode = param.mode;
	}

	demod_dvbc_fsm_reset(demod);

	/* Wait for R842 if output to stabilize when automatic sr. */
	if (demod->auto_sr &&
		(tuner_find_by_name(fe, "r842") ||
		tuner_find_by_name(fe, "r836") ||
		tuner_find_by_name(fe, "r850")))
		msleep(500);

	dvbc_status(demod, &demod_sts, NULL);
	demod->freq_dvbc = param.ch_freq;

	return ret;
}

int gxtv_demod_dvbc_get_frontend(struct dvb_frontend *fe)
{
	return 0;
}

enum qam_md_e dvbc_switch_qam(enum qam_md_e qam_mode)
{
	enum qam_md_e ret;

	switch (qam_mode) {
	case QAM_MODE_16:
		ret = QAM_MODE_256;
		break;

	case QAM_MODE_32:
		ret = QAM_MODE_16;
		break;

	case QAM_MODE_64:
		ret = QAM_MODE_128;
		break;

	case QAM_MODE_128:
		ret = QAM_MODE_32;
		break;

	case QAM_MODE_256:
		ret = QAM_MODE_64;
		break;

	default:
		ret = QAM_MODE_256;
		break;
	}

	return ret;
}

//return val : 0=no signal,1=has signal,2=waiting
unsigned int dvbc_auto_fast(struct dvb_frontend *fe, unsigned int *delay, bool re_tune)
{
	struct aml_dtvdemod *demod = (struct aml_dtvdemod *)fe->demodulator_priv;
	char qam_name[20];
	unsigned int next_qam = 0xf;
	unsigned int fsm_state = 0, eq_state = 0;
	int ret = 0;
	s16 strength = 0;
	static unsigned int time_start;
	static unsigned int sym_speed_high;

	if (re_tune) {
		/*avoid that the previous channel has not been locked/unlocked*/
		/*the next channel is set again*/
		/*the wait time is too long,then miss channel*/
		demod->auto_no_sig_cnt = 0;
		demod->auto_times = 0;
		demod->auto_done_times = 0;
		demod->auto_qam_done = false;
		demod->auto_qam_index = 0;
		demod->auto_sr_done = false;
		demod->sr_val_hw_stable = 0;
		demod->sr_val_hw_count = 0;
		demod->sr_val_uf_count = 0;
		demod->fsm_reset = false;
		sym_speed_high = 0;

		time_start = jiffies_to_msecs(jiffies);
		PR_DVBC("%s: retune reset, sr %d.\n", __func__, demod->sr_val_hw);
		return 2;
	}

	gxtv_demod_dvbc_read_signal_strength(fe, &strength);
	if (strength < THRD_TUNER_STRENGTH_DVBC) {
		demod->auto_no_sig_cnt = 0;
		demod->auto_times = 0;
		demod->auto_done_times = 0;

		*delay = HZ / 4;

		PR_DVBC("%s: [id %d] tuner strength [%d] no signal(%d).\n",
				__func__, demod->id, strength, THRD_TUNER_STRENGTH_DVBC);

		return 0;
	}

	/* symbol rate, 0=auto */
	if (demod->auto_sr) {
		if (!demod->auto_sr_done) {
			demod->sr_val_hw = dvbc_get_symb_rate(demod);
			eq_state = qam_read_reg(demod, 0x5d) & 0xf;
			fsm_state = qam_read_reg(demod, 0x31) & 0xf;

			if (abs(demod->sr_val_hw - demod->sr_val_hw_stable) <= 100)
				demod->sr_val_hw_count++;
			else
				demod->sr_val_hw_count = 0;

			PR_DVBC("%s: get sr %d, count %d",
				__func__, demod->sr_val_hw, demod->sr_val_hw_count);
			PR_DVBC("%s: eq_state(0x5d) 0x%x, fsm_state(0x31) 0x%x.\n",
				__func__, eq_state, fsm_state);

			demod->sr_val_hw_stable = demod->sr_val_hw;

			if (eq_state >= 2 || demod->sr_val_hw_count >= 3) {
				// slow down auto sr speed and range.
				dvbc_cfg_sr_scan_speed(demod, SYM_SPEED_NORMAL);
				dvbc_cfg_sr_cnt(demod, SYM_SPEED_NORMAL);
				dvbc_cfg_tim_sweep_range(demod, SYM_SPEED_NORMAL);
				dvbc_cfg_sw_hw_sr_max(demod, demod->sr_val_hw_stable);

				demod->auto_sr_done = true;
				PR_DVBC("%s: auto_sr_done[%d], sr_val_hw_stable %d, cost %d ms.\n",
						__func__, fe->dtv_property_cache.frequency,
						demod->sr_val_hw_stable,
						jiffies_to_msecs(jiffies) - time_start);
				if (fsm_state != 5) {
					demod_dvbc_fsm_reset(demod);
					*delay = HZ / 2;//500ms
					return 2; // wait qam256 lock.
				}
			} else {
				if (demod->sr_val_hw < 6820 &&
					sym_speed_high == 0) {
					// fast down auto sr speed and range.
					dvbc_cfg_sr_scan_speed(demod, SYM_SPEED_HIGH);
					dvbc_cfg_sr_cnt(demod, SYM_SPEED_HIGH);
					dvbc_cfg_tim_sweep_range(demod, SYM_SPEED_HIGH);
					sym_speed_high = 1;
					PR_DVBC("%s: fast scan[%d], sr_val_hw %d, cost %d ms.\n",
						__func__, fe->dtv_property_cache.frequency,
						demod->sr_val_hw_stable,
						jiffies_to_msecs(jiffies) - time_start);
				}
			}

			/* sr underflow */
			if (demod->sr_val_hw_stable < 3400) {
				if (++demod->sr_val_uf_count > 1) {
					PR_DVBC("%s: auto sr underflow(count %d) unlocked.\n",
							__func__, demod->sr_val_uf_count);
					return 0; // underflow, unlock.
				}
			}
		}

		// total wait 1250ms for auto sr done.
		if ((jiffies_to_msecs(jiffies) - time_start < 1250) && !demod->auto_sr_done) {
			// try every 100ms.
			*delay = HZ / 10;

			return 2; // wait.
		}

		if (!demod->auto_sr_done) {
			PR_DVBC("%s: auto sr timeout unlocked.\n", __func__);
			return 0; // timeout, unlock.
		}
	}

	if (cpu_after_eq(MESON_CPU_MAJOR_ID_T5W) && !is_meson_s1a_cpu()) {
		if (!demod->auto_qam_done) {
			demod->auto_done_times = 0;
			ret = dvbc_auto_qam_process(demod, demod->auto_qam_list);
			if (!ret) {
				demod->auto_qam_done = true;
				demod->auto_done_times = 0;
				demod->auto_qam_mode = demod->auto_qam_list[demod->auto_qam_index];
				dvbc_get_qam_name(demod->auto_qam_mode, qam_name);
				demod_dvbc_set_qam(demod, demod->auto_qam_mode, false);

				PR_INFO("%s: auto_times %d, auto qam done, get %d(%s), index %d.\n",
						__func__, demod->auto_times, demod->auto_qam_mode,
						qam_name, demod->auto_qam_index);

				demod->fsm_reset = false;
				demod->auto_qam_index++;
			}
		}
	}

	dvbc_get_qam_name(demod->auto_qam_mode, qam_name);

	fsm_state = qam_read_reg(demod, 0x31);
	PR_DVBC("%s: fsm(0x31):0x%x, sr:%d, auto_times:%d, auto_done_times:%d, qam:%d[%s].\n",
			__func__, fsm_state, dvbc_get_symb_rate(demod),
			demod->auto_times, demod->auto_done_times,
			demod->auto_qam_mode, qam_name);

	/* fsm_state: 0x31[bit0-3].
	 * 0: IDLE.               1: AGC.
	 * 2: COARSE_SYMBOL_RATE. 3: FINE_SYMBOL_RATE.
	 * 4: EQ_MMA_RESET.       5: FEC_LOCK.
	 * 6: FEC_LOST.           7: EQ_SMMA_RESET.
	 */
	if ((fsm_state & 0xf) < 3) {
		demod->auto_no_sig_cnt++;

		if (demod->auto_no_sig_cnt == 2 && demod->auto_times == 2) {//250ms
			demod->auto_no_sig_cnt = 0;
			demod->auto_times = 0;
			*delay = HZ / 4;

			return 0;
		}
	} else if ((fsm_state & 0xf) == 5) {
		demod->auto_no_sig_cnt = 0;
		demod->auto_times = 0;
		*delay = HZ / 4;
		demod->real_para.modulation = amdemod_qam_fe(demod->auto_qam_mode);
		demod->real_para.symbol = demod->auto_sr ?
			demod->sr_val_hw_stable * 1000 :
			fe->dtv_property_cache.symbol_rate;

		return 1;
	}

	if (demod->auto_times == 15 || ((cpu_after_eq(MESON_CPU_MAJOR_ID_T5W) &&
		!is_meson_s1a_cpu()) &&
		((demod->auto_times == 2 && !demod->auto_qam_done) ||
		(demod->auto_qam_index >= 5 && demod->auto_qam_done)))) {
		demod->auto_times = 0;
		demod->auto_no_sig_cnt = 0;
		*delay = HZ / 4;

		return 0;
	}

	demod->auto_times++;
	/* loop from 16 to 256 */
	if (!cpu_after_eq(MESON_CPU_MAJOR_ID_T5W) || is_meson_s1a_cpu()) {
		demod->auto_qam_mode = dvbc_switch_qam(demod->auto_qam_mode);
		demod_dvbc_set_qam(demod, demod->auto_qam_mode, false);
	} else {
		if (demod->auto_qam_done) {
			demod->auto_done_times++;
			next_qam = demod->auto_qam_list[demod->auto_qam_index];
			if ((demod->auto_done_times % 4 == 0) && next_qam != 0xf) {
				demod->auto_qam_mode = next_qam;
				dvbc_get_qam_name(demod->auto_qam_mode, qam_name);
				demod_dvbc_set_qam(demod, demod->auto_qam_mode, false);

				PR_INFO("%s: try next_qam %d(%s), index %d.\n",
						__func__, next_qam,
						qam_name, demod->auto_qam_index);

				demod->fsm_reset = false;
				demod->auto_qam_index++;
			} else {
				if (!demod->fsm_reset) {
					demod_dvbc_fsm_reset(demod);
					demod->fsm_reset = true;
				}
			}
		}
	}

	// after switch qam, check the sr.
	if (demod->auto_sr && demod->auto_sr_done) {
		demod->sr_val_hw = dvbc_get_symb_rate(demod);
		if (abs(demod->sr_val_hw - demod->sr_val_hw_stable) >= 100) {
			PR_DVBC("%s: switch qam rewrite hw sr from %d to %d.\n",
					__func__, demod->sr_val_hw, demod->sr_val_hw_stable);
			dvbc_cfg_sw_hw_sr_max(demod, demod->sr_val_hw_stable);
		}
	}

	return 2;
}

//return val : 0=no signal,1=has signal,2=waiting
unsigned int dvbc_fast_search(struct dvb_frontend *fe, unsigned int *delay, bool re_tune)
{
	struct aml_dtvdemod *demod = (struct aml_dtvdemod *)fe->demodulator_priv;
	static unsigned int time_start;
	unsigned int fsm_state = 0, eq_state = 0;
	s16 strength = 0;
	static unsigned int sym_speed_high;

	if (re_tune) {
		/*avoid that the previous channel has not been locked/unlocked*/
		/*the next channel is set again*/
		/*the wait time is too long,then miss channel*/
		demod->auto_no_sig_cnt = 0;
		demod->auto_times = 1;
		demod->auto_sr_done = false;
		demod->fsm_reset = false;
		demod->sr_val_hw_stable = 0;
		demod->sr_val_hw_count = 0;
		demod->sr_val_uf_count = 0;
		sym_speed_high = 0;

		time_start = jiffies_to_msecs(jiffies);
		PR_DVBC("%s: retune reset, sr %d.\n", __func__, demod->sr_val_hw);
		return 2;
	}

	gxtv_demod_dvbc_read_signal_strength(fe, &strength);
	if (strength < THRD_TUNER_STRENGTH_DVBC) {
		demod->auto_times = 0;
		demod->auto_no_sig_cnt = 0;
		*delay = HZ / 4;
		PR_DVBC("%s: [id %d] tuner strength [%d] no signal(%d).\n",
				__func__, demod->id, strength, THRD_TUNER_STRENGTH_DVBC);

		return 0;
	}

	/* symbol rate, 0=auto */
	if (demod->auto_sr) {
		if (!demod->auto_sr_done) {
			demod->sr_val_hw = dvbc_get_symb_rate(demod);
			eq_state = qam_read_reg(demod, 0x5d) & 0xf;
			fsm_state = qam_read_reg(demod, 0x31) & 0xf;

			if (abs(demod->sr_val_hw - demod->sr_val_hw_stable) <= 100)
				demod->sr_val_hw_count++;
			else
				demod->sr_val_hw_count = 0;

			PR_DVBC("%s get sr %d,count %d,eq_state(0x5d) 0x%x,fsm_state(0x31) 0x%x.\n",
					__func__, demod->sr_val_hw, demod->sr_val_hw_count,
					eq_state, fsm_state);

			demod->sr_val_hw_stable = demod->sr_val_hw;

			if (eq_state >= 2 || demod->sr_val_hw_count >= 3) {
				// slow down auto sr speed and range.
				dvbc_cfg_sr_scan_speed(demod, SYM_SPEED_NORMAL);
				dvbc_cfg_sr_cnt(demod, SYM_SPEED_NORMAL);
				dvbc_cfg_tim_sweep_range(demod, SYM_SPEED_NORMAL);
				dvbc_cfg_sw_hw_sr_max(demod, demod->sr_val_hw_stable);

				demod->auto_sr_done = true;
				PR_DVBC("%s: auto_sr_done, sr_val_hw_stable %d, cost %d ms.\n",
						__func__, demod->sr_val_hw_stable,
						jiffies_to_msecs(jiffies) - time_start);
			} else {
				if (demod->sr_val_hw < 6820 &&
					sym_speed_high == 0) {
					// fast down auto sr speed and range.
					dvbc_cfg_sr_scan_speed(demod, SYM_SPEED_HIGH);
					dvbc_cfg_sr_cnt(demod, SYM_SPEED_HIGH);
					dvbc_cfg_tim_sweep_range(demod, SYM_SPEED_HIGH);
					sym_speed_high = 1;
					PR_DVBC("%s: fast scan, sr_val_hw_stable %d, cost %d ms.\n",
							__func__, demod->sr_val_hw_stable,
							jiffies_to_msecs(jiffies) - time_start);
				}
			}

			/* sr underflow */
			if (demod->sr_val_hw_stable < 3400) {
				if (++demod->sr_val_uf_count > 1) {
					PR_DVBC("%s: auto sr underflow(count %d) unlocked.\n",
							__func__, demod->sr_val_uf_count);
					return 0; // underflow, unlock.
				}
			}
		}

		// total wait 1250ms for auto sr done.
		if ((jiffies_to_msecs(jiffies) - time_start < 1250) && !demod->auto_sr_done) {
			// try every 100ms.
			*delay = HZ / 10;

			return 2; // wait.
		}

		if (!demod->auto_sr_done) {
			PR_DVBC("%s: auto sr timeout unlocked.\n", __func__);
			return 0; // timeout, unlock.
		}
	}

	if (demod->auto_times < 3)
		*delay = HZ / 8;//125ms
	else
		*delay = HZ / 2;//500ms

	fsm_state = qam_read_reg(demod, 0x31);
	PR_DVBC("%s: fsm_state(0x31): 0x%x, auto_times: %d.\n",
			__func__, fsm_state, demod->auto_times);

	if ((fsm_state & 0xf) < 3) {
		demod->auto_no_sig_cnt++;

		if (demod->auto_no_sig_cnt == 2 && demod->auto_times == 2) {//250ms
			demod->auto_no_sig_cnt = 0;
			demod->auto_times = 0;
			*delay = HZ / 4;
			return 0;
		}
	} else if ((fsm_state & 0xf) == 5) {
		demod->auto_no_sig_cnt = 0;
		demod->auto_times = 0;
		*delay = HZ / 4;
		demod->real_para.modulation = fe->dtv_property_cache.modulation;
		demod->real_para.symbol = demod->auto_sr ?
				demod->sr_val_hw_stable * 1000 :
				fe->dtv_property_cache.symbol_rate;
		return 1;
	}

	if (demod->auto_times == 7) {
		demod->auto_times = 0;
		demod->auto_no_sig_cnt = 0;
		*delay = HZ / 4;
		return 0;
	}

	if (!demod->fsm_reset) {
		demod_dvbc_fsm_reset(demod);
		demod->fsm_reset = true;
	}

	demod->auto_times++;

	return 2;
}

int gxtv_demod_dvbc_tune(struct dvb_frontend *fe, bool re_tune,
	unsigned int mode_flags, unsigned int *delay, enum fe_status *status)
{
	int ret = 0;
	unsigned int sig_flg = 0;
	struct aml_dtvdemod *demod = (struct aml_dtvdemod *)fe->demodulator_priv;
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	bool auto_qam = (c->modulation == QAM_AUTO);

	*delay = HZ / 4;

	if (re_tune) {
		/*first*/
		demod->en_detect = 1;

		real_para_clear(&demod->real_para);

		gxtv_demod_dvbc_set_frontend(fe);

		demod->fast_search_finish = false;
		*status = 0;
		*delay = HZ / 8;

		if (auto_qam)
			dvbc_auto_fast(fe, delay, re_tune);

		return ret;
	}

	if (!demod->en_detect) {
		PR_DBGL("%s: [id %d] not enable.\n", __func__, demod->id);
		return ret;
	}

	if (!demod->fast_search_finish) {
		if (auto_qam)
			sig_flg = dvbc_auto_fast(fe, delay, re_tune);
		else
			sig_flg = dvbc_fast_search(fe, delay, re_tune);

		switch (sig_flg) {
		case 0:
			*status = FE_TIMEDOUT;
			demod->last_status = *status;
			real_para_clear(&demod->real_para);
			demod->fast_search_finish = false;
			demod->fsm_reset = false;
			/* loss lock, reset 256qam, start auto qam again. */
			if (cpu_after_eq(MESON_CPU_MAJOR_ID_T5W) &&
				!is_meson_s1a_cpu() &&
				demod->auto_qam_done &&
				auto_qam) {
				demod->auto_qam_mode = QAM_MODE_256;
				demod_dvbc_set_qam(demod, demod->auto_qam_mode, false);
				demod->auto_qam_done = false;
				demod->auto_qam_index = 0;
				demod->auto_times = 0;
				demod->auto_done_times = 0;
			}
			PR_DBG("%s [id %d] [%d] >>>unlock<<<\n",
				__func__, demod->id, c->frequency);
			break;
		case 1:
			*status =
			FE_HAS_LOCK | FE_HAS_SIGNAL | FE_HAS_CARRIER |
			FE_HAS_VITERBI | FE_HAS_SYNC;
			demod->last_status = *status;
			demod->fast_search_finish = true;
			PR_DBG("%s [id %d] [%d] >>>lock<<< [qam %d]\n",
				__func__, demod->id, c->frequency,
				auto_qam ? demod->real_para.modulation : c->modulation);
			break;
		case 2:
			*status = 0;
			break;
		default:
			PR_DVBC("[id %d] wrong return value\n", demod->id);
			break;
		}
	} else {
		gxtv_demod_dvbc_read_status_timer(fe, status);
	}

	return ret;
}

enum fe_modulation dvbc_get_dvbc_qam(enum qam_md_e am_qam)
{
	switch (am_qam) {
	case QAM_MODE_16:
		return QAM_16;
	case QAM_MODE_32:
		return QAM_32;
	case QAM_MODE_64:
		return QAM_64;
	case QAM_MODE_128:
		return QAM_128;
	case QAM_MODE_256:
	default:
		return QAM_256;
	}

	return QAM_256;
}

void dvbc_set_speedup(struct aml_dtvdemod *demod)
{
	qam_write_reg(demod, 0x65, 0x400c);
	qam_write_reg(demod, 0x60, 0x10466000);
	qam_write_reg(demod, 0xac, (qam_read_reg(demod, 0xac) & (~0xff00))
		| 0x800);
	qam_write_reg(demod, 0xae, (qam_read_reg(demod, 0xae)
		& (~0xff000000)) | 0x8000000);
}

void dvbc_set_srspeed(struct aml_dtvdemod *demod, int high)
{
	if (high) {
		qam_write_reg(demod, SYMB_CNT_CFG, 0xffff03ff);
		qam_write_reg(demod, SR_SCAN_SPEED, 0x234cf523);
	} else {
		qam_write_reg(demod, SYMB_CNT_CFG, 0xffff8ffe);
		qam_write_reg(demod, SR_SCAN_SPEED, 0x235cf459);
	}
}

int dvbc_set_frontend(struct dvb_frontend *fe)
{
	struct aml_dtvdemod *demod = (struct aml_dtvdemod *)fe->demodulator_priv;
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	struct aml_demod_dvbc param;    /*mode 0:16, 1:32, 2:64, 3:128, 4:256*/
	int ret = 0;

	PR_INFO("%s [id %d]: delsys:%d, freq:%d, symbol_rate:%d, bw:%d, modul:%d.\n",
			__func__, demod->id, c->delivery_system, c->frequency, c->symbol_rate,
			c->bandwidth_hz, c->modulation);

	c->bandwidth_hz = 8000000;

	memset(&param, 0, sizeof(param));
	param.ch_freq = c->frequency / 1000;
	if (c->modulation == QAM_AUTO)
		param.mode = QAM_MODE_256;
	else
		param.mode = amdemod_qam(c->modulation);
	demod->auto_qam_mode = param.mode;

	if (c->symbol_rate == 0) {
		param.symb_rate = 7000;
		demod->auto_sr = 1;
	} else {
		param.symb_rate = c->symbol_rate / 1000;
		demod->auto_sr = 0;
	}

	demod->sr_val_hw = param.symb_rate;
	demod->last_qam_mode = QAM_MODE_NUM;
	demod->last_lock = 0;
	demod->time_start = jiffies_to_msecs(jiffies);

	tuner_set_params(fe);
	dvbc_set_ch(demod, &param, fe);

	dvbc_init_reg_ext(demod);

	dvbc_set_speedup(demod);

	demod_dvbc_store_qam_cfg(demod);

	demod_dvbc_set_qam(demod, param.mode, demod->auto_sr);
	demod_dvbc_fsm_reset(demod);

	return ret;
}

int dvbc_read_status(struct dvb_frontend *fe, enum fe_status *status, bool re_tune)
{
	struct aml_dtvdemod *demod = (struct aml_dtvdemod *)fe->demodulator_priv;
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	s16 strength = 0;
	int fsm_status = 0;//0:none;1:lock;-1:lost
	unsigned int s = 0;
	unsigned int sr = 0;
	unsigned int curtime = 0, time_passed_qam = 0;
	static int peak;
	static int AQAM_times;
	static int ASR_times;
	static unsigned int time_start_qam;
	static int ASR_end = 3480;
	static int lock_status;
	int lock_continuous_cnt = dvbc_lock_continuous_cnt > 1 ? dvbc_lock_continuous_cnt : 1;
	int lost_continuous_cnt = dvbc_lost_continuous_cnt > 1 ? dvbc_lost_continuous_cnt : 1;

	if (re_tune) {
		peak = 0;
		AQAM_times = 0;
		time_start_qam = 0;
		if (c->symbol_rate == 0) {
			ASR_times = 0;
			demod->sr_val_hw = 7100;
			ASR_end = 3480;
		} else {
			ASR_times = 2;
			demod->sr_val_hw = c->symbol_rate / 1000;
			ASR_end = c->symbol_rate / 1000 - 20;
		}
		qam_write_bits(demod, 0xd, demod->sr_val_hw, 0, 16);
		qam_write_bits(demod, 0x11, demod->sr_val_hw, 8, 16);
		dvbc_set_srspeed(demod, c->symbol_rate == 0 ? 1 : 0);
		demod_dvbc_fsm_reset(demod);

		lock_status = 0;
		*status = 0;
		demod->last_status = 0;

		return 0;
	}

	gxtv_demod_dvbc_read_signal_strength(fe, &strength);
	if (strength < THRD_TUNER_STRENGTH_DVBC) {
		PR_DVBC("%s: tuner strength [%d] no signal(%d).\n",
				__func__, strength, THRD_TUNER_STRENGTH_DVBC);
		*status = FE_TIMEDOUT;
		demod->last_status = *status;
		real_para_clear(&demod->real_para);
		time_start_qam = 0;

		goto finish;
	}

	curtime = jiffies_to_msecs(jiffies);
	demod->time_passed = curtime - demod->time_start;
	s = qam_read_reg(demod, 0x31) & 0xf;
	sr = dvbc_get_symb_rate(demod);
	PR_DVBC("s=%d, demod->time_passed=%u\n", s, demod->time_passed);

	if (s != 5)
		real_para_clear(&demod->real_para);

	if (s == 5) {
		time_start_qam = 0;
		fsm_status = 1;
		*status = FE_HAS_LOCK | FE_HAS_SIGNAL | FE_HAS_CARRIER |
			FE_HAS_VITERBI | FE_HAS_SYNC;
		demod->real_para.modulation = dvbc_get_dvbc_qam(demod->auto_qam_mode);
		demod->real_para.symbol = sr * 1000;

		PR_DVBC("locked at sr:%d,Qam:%s\n", sr, get_qam_name(demod->auto_qam_mode));
	} else if (s < 3) {
		time_start_qam = 0;
		if (demod->last_lock == 0 && (demod->time_passed < 600 ||
			(demod->time_passed < 2000 && ASR_times < 2) ||
			(peak == 1 && AQAM_times < 3 && c->modulation == QAM_AUTO)))
			fsm_status = 0;
		else
			fsm_status = -1;

		PR_DVBC("sr : %d, c->symbol_rate=%d\n", sr, c->symbol_rate);
		if (c->symbol_rate == 0 && sr < ASR_end) {
			demod->sr_val_hw = 7000;
			qam_write_bits(demod, 0xd, 7000, 0, 16);
			qam_write_bits(demod, 0x11, 7000, 8, 16);
			PR_DVBC("sr reset from %d.freq=%d\n", 7000, c->frequency);
			ASR_end = ASR_end == 3480 ? 6780 : 3480;
			dvbc_set_srspeed(demod, ASR_end == 3480 ? 1 : 0);
			demod_dvbc_fsm_reset(demod);
			ASR_times++;
		} else if (c->symbol_rate != 0 && sr < ASR_end) {
			demod->sr_val_hw = c->symbol_rate / 1000;
			qam_write_bits(demod, 0xd, c->symbol_rate / 1000, 0, 16);
			qam_write_bits(demod, 0x11, c->symbol_rate / 1000, 8, 16);
			dvbc_set_srspeed(demod, 0);
			demod_dvbc_fsm_reset(demod);
		}
	} else if (s == 4 || s == 7) {
		fsm_status = 0;
	} else {
		if (peak == 0)
			peak = 1;

		PR_DVBC("true sr:%d, try Qam:%s, eq_state=0x%x, time_passed:%u\n",
			sr, get_qam_name(demod->auto_qam_mode),
			qam_read_reg(demod, 0x5d), demod->time_passed);
		PR_DVBC("time_start_qam=%u\n", time_start_qam);
		if (demod->last_lock == 0 && (demod->time_passed < TIMEOUT_DVBC ||
			(AQAM_times < 3 && c->modulation == QAM_AUTO)))
			fsm_status = 0;
		else
			fsm_status = -1;

		if (c->modulation != QAM_AUTO) {
			PR_DVBC("Qam is not auto\n");
		} else if (time_start_qam == 0) {
			time_start_qam = curtime;
		} else {
			time_passed_qam = curtime - time_start_qam;
			if ((demod->auto_qam_mode == QAM_MODE_256 && time_passed_qam >
				(650 + 300 * (AQAM_times % 3))) ||
				(demod->auto_qam_mode != QAM_MODE_256 && time_passed_qam >
				(350 + 100 * (AQAM_times % 3)))) {
				time_start_qam = curtime;
				demod->auto_qam_mode = dvbc_switch_qam(demod->auto_qam_mode);
				PR_DVBC("to next qam:%s\n", get_qam_name(demod->auto_qam_mode));
				demod_dvbc_set_qam(demod, demod->auto_qam_mode, false);
				if (demod->auto_qam_mode == QAM_MODE_16) {
					time_start_qam = 0;
					PR_DVBC("set QAM_16, so reset\n");
					if (sr >= 3500) {
						PR_DVBC("set sr start from:%d\n", sr);
						qam_write_bits(demod, 0xd, sr, 0, 16);
						qam_write_bits(demod, 0x11, sr, 8, 16);
					}
					demod_dvbc_fsm_reset(demod);
				} else if (demod->auto_qam_mode == QAM_MODE_256) {
					AQAM_times++;
				}
			}
		}
	}

	//The status is updated only when the status continuously reaches the threshold of times
	if (fsm_status == -1) {
		if (lock_status >= 0) {
			lock_status = -1;
			PR_ATSC("==> lost signal first\n");
		} else if (lock_status <= -lost_continuous_cnt) {
			lock_status = -lost_continuous_cnt;
			PR_ATSC("==> lost signal continue\n");
		} else {
			lock_status--;
			PR_ATSC("==> lost signal times%d\n", lock_status);
		}

		if (lock_status <= -lost_continuous_cnt)
			*status = FE_TIMEDOUT;
		else
			*status = 0;
	} else if (fsm_status == 1) {
		if (lock_status <= 0) {
			lock_status = 1;
			PR_ATSC("==> lock signal first\n");
		} else if (lock_status >= lock_continuous_cnt) {
			lock_status = lock_continuous_cnt;
			PR_ATSC("==> lock signal continue\n");
		} else {
			lock_status++;
			PR_ATSC("==> lock signal times:%d\n", lock_status);
		}

		if (lock_status >= lock_continuous_cnt)
			*status = FE_HAS_LOCK | FE_HAS_SIGNAL |
				FE_HAS_CARRIER | FE_HAS_VITERBI | FE_HAS_SYNC;
		else
			*status = 0;
	} else {
		*status = 0;
	}

finish:
	if (*status == 0) {
		PR_DVBC("!! >> wait << !!\n");
	} else if (*status == FE_TIMEDOUT) {
		if (demod->last_lock == -1)
			PR_DVBC("!! >> lost again << !!\n");
		else
			PR_INFO("!! >> UNLOCK << !!, freq=%d, time_passed:%u\n",
				c->frequency, demod->time_passed);
		demod->last_lock = -1;
	} else {
		if (demod->last_lock == 1)
			PR_DVBC("!! >> lock continue << !!\n");
		else
			PR_INFO("!! >> LOCK << !!, freq=%d, time_passed:%u\n",
				c->frequency, demod->time_passed);
		demod->last_lock = 1;
	}
	demod->last_status = *status;

	return 0;
}

int dvbc_tune(struct dvb_frontend *fe, bool re_tune,
	unsigned int mode_flags, unsigned int *delay, enum fe_status *status)
{
	struct aml_dtvdemod *demod = (struct aml_dtvdemod *)fe->demodulator_priv;

	*delay = HZ / 10;

	if (re_tune) {
		PR_INFO("%s [id %d]: re_tune.\n", __func__, demod->id);
		demod->en_detect = 1;

		dvbc_set_frontend(fe);
		dvbc_read_status(fe, status, re_tune);

		*status = 0;

		return 0;
	}

	dvbc_read_status(fe, status, re_tune);

	return 0;
}

void dvbc_blind_scan_work(struct aml_dtvdemod *demod)
{
	struct amldtvdemod_device_s *devp = (struct amldtvdemod_device_s *)demod->priv;
	struct dvb_frontend *fe = NULL;
	struct dtv_frontend_properties *c = NULL;
	enum fe_status status;
	//char qam_name[20] = {0};

	if (devp->blind_scan_stop) {
		PR_ERR("error: %s dvbc blind scan not start yet!\n", __func__);
		return;
	}

	fe = &demod->frontend;
	if (unlikely(!fe)) {
		PR_ERR("error: %s fe is NULL\n", __func__);
		devp->blind_scan_stop = 1;
		return;
	}

	c = &fe->dtv_property_cache;
	demod->blind_result_frequency = 0;
	demod->blind_result_symbol_rate = 0;
	PR_INFO("[id %d][%d] %s start ...\n", demod->id, demod->dvbc_sel, __func__);

	//1.init the new dvbc_blind_scan mode for stage 1;
	PR_INFO("init new dvbc_blind_scan mode\n");
	devp->dvbc_inited = false;
	gxtv_demod_dvbc_init(demod, ADC_MODE);

	//2.use frontend agc instead;
	if (cpu_after_eq(MESON_CPU_MAJOR_ID_T5D))
		demod_enable_frontend_agc(demod, c->delivery_system, true);

	//3.start to check signal in the dvbc frequency range [48000, 859000KHz]
	//  and report blind scan result(valid TPs).
	dvbc_blind_scan_process(demod);

	//4.exit stage 1: exit dvbc_blind_scan mode and switch back to local agc
	if (cpu_after_eq(MESON_CPU_MAJOR_ID_T5D))
		demod_enable_frontend_agc(demod, c->delivery_system, false);

	//disable dvbc_blind_scan mode to avoid hang when switch to other demod
	demod_top_write_reg(DEMOD_TOP_REGC, 0x11);

	//5.init dvbc for stage 2
	devp->dvbc_inited = false;
	gxtv_demod_dvbc_init(demod, ADC_MODE);

	if (!devp->blind_scan_stop) {
		//the time usage of first stage is 10% of the overall time usage.
		demod->blind_result_frequency = 100;
		demod->blind_result_symbol_rate = 0;
		status = BLINDSCAN_UPDATEPROCESS | FE_HAS_LOCK;
		PR_INFO("%s: force 100%% to upper layer\n", __func__);
		dvb_frontend_add_event(fe, status);
		devp->blind_scan_stop = 1;
	}

	PR_INFO("[id %d] %s exit.\n", demod->id, __func__);
}
