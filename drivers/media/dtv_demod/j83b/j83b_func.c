// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include "demod_func.h"
#include "aml_demod.h"
#include "j83b_func.h"
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/mutex.h>

MODULE_PARM_DESC(atsc_j83b_agc_target, "");
static unsigned char atsc_j83b_agc_target = 0xe;
module_param(atsc_j83b_agc_target, byte, 0644);

static unsigned char j83b_qam_reg[15] = { 0 };
static unsigned int j83b_qam_value[15] = { 0 };

u32 atsc_j83b_get_status(struct aml_dtvdemod *demod)
{
	/*PR_DVBC("c4 is %x\n",dvbc_read_reg(QAM_BASE+0xc4));*/
	return qam_read_reg(demod, 0x31) & 0xf;
}

u32 atsc_j83b_get_ch_sts(struct aml_dtvdemod *demod)
{
	return qam_read_reg(demod, 0x6);
}

/*copy from dvbc_get_ch_power*/
static u32 atsc_j83b_get_ch_power(struct aml_dtvdemod *demod)
{
	u32 tmp;
	u32 ad_power;
	u32 agc_gain;
	u32 ch_power;

	tmp = qam_read_reg(demod, 0x27);

	ad_power = (tmp >> 22) & 0x1ff;
	agc_gain = (tmp >> 0) & 0x7ff;

	ad_power = ad_power >> 4;
	/* ch_power = lookuptable(agc_gain) + ad_power; TODO */
	ch_power = (ad_power & 0xffff) + ((agc_gain & 0xffff) << 16);

	return ch_power;
}

/*copy from dvbc_get_snr*/
u32 atsc_j83b_get_snr(struct aml_dtvdemod *demod)
{
	u32 tmp, snr;

	tmp = qam_read_reg(demod, 0x5) & 0xfff;
	snr = tmp * 100 / 32;	/* * 1e2 */

	return snr;
}

static u32 atsc_j83b_get_ber(struct aml_dtvdemod *demod)
{
	u32 rs_ber;
	u32 rs_packet_len;

	rs_packet_len = qam_read_reg(demod, 0x4) & 0xffff;
	rs_ber = qam_read_reg(demod, 0x5) >> 12 & 0xfffff;

	/* rs_ber = rs_ber / 204.0 / 8.0 / rs_packet_len; */
	if (rs_packet_len == 0)
		rs_ber = 1000000;
	else
		rs_ber = rs_ber * 613 / rs_packet_len;	/* 1e-6 */

	return rs_ber;
}

/*copy from dvbc_get_per*/
u32 atsc_j83b_get_per(struct aml_dtvdemod *demod)
{
	u32 rs_per;
	u32 rs_packet_len;
	u32 acc_rs_per_times;

	rs_packet_len = qam_read_reg(demod, 0x4) & 0xffff;
	rs_per = qam_read_reg(demod, 0x6) >> 16 & 0xffff;

	acc_rs_per_times = qam_read_reg(demod, 0x33) & 0xffff;
	/*rs_per = rs_per / rs_packet_len; */

	if (rs_packet_len == 0)
		rs_per = 10000;
	else
		rs_per = 10000 * rs_per / rs_packet_len;	/* 1e-4 */

	/*return rs_per; */
	return acc_rs_per_times;
}

/*copy from dvbc_get_symb_rate*/
u32 atsc_j83b_get_symb_rate(struct aml_dtvdemod *demod)
{
	u32 tmp;
	u32 adc_freq;
	u32 symb_rate;

	adc_freq = qam_read_reg(demod, 0xd) >> 16 & 0xffff;
	tmp = qam_read_reg(demod, 0x2e);

	if ((tmp >> 15) == 0)
		symb_rate = 0;
	else
		symb_rate = 10 * (adc_freq << 12) / (tmp >> 15);

	return symb_rate / 10;
}

/*copy from dvbc_get_freq_off*/
static int atsc_j83b_get_freq_off(struct aml_dtvdemod *demod)
{
	int tmp;
	int symb_rate;
	int freq_off;

	symb_rate = atsc_j83b_get_symb_rate(demod);
	tmp = qam_read_reg(demod, 0x38) & 0x3fffffff;
	if (tmp >> 29 & 1)
		tmp -= (1 << 30);

	freq_off = ((tmp >> 16) * 25 * (symb_rate >> 10)) >> 3;

	return freq_off;
}

int atsc_j83b_status(struct aml_dtvdemod *demod,
	struct aml_demod_sts *demod_sts, struct seq_file *seq)
{
	demod_sts->ch_sts = qam_read_reg(demod, 0x6);
	demod_sts->ch_pow = atsc_j83b_get_ch_power(demod);
	demod_sts->ch_snr = atsc_j83b_get_snr(demod);
	demod_sts->ch_ber = atsc_j83b_get_ber(demod);
	demod_sts->ch_per = atsc_j83b_get_per(demod);
	demod_sts->symb_rate = atsc_j83b_get_symb_rate(demod);
	demod_sts->freq_off = atsc_j83b_get_freq_off(demod);
	demod_sts->dat1 = tuner_get_ch_power(&demod->frontend);

	if (seq) {
		seq_printf(seq, "ch_sts:0x%x,snr:%ddB,ber:%d,per:%d,srate:%d,freqoff:%dkHz\n",
			demod_sts->ch_sts, demod_sts->ch_snr / 100, demod_sts->ch_ber,
			demod_sts->ch_per, demod_sts->symb_rate, demod_sts->freq_off);
		seq_printf(seq, "strength:%ddb,0xe0 status:%u,b4 status:%u,dagc_gain:%u\n",
			demod_sts->dat1, qam_read_reg(demod, 0x38) & 0xffff,
			qam_read_reg(demod, 0x2d) & 0xffff, qam_read_reg(demod, 0x29) & 0x7f);
		seq_printf(seq, "power:%ddb,0x31=0x%x\n", demod_sts->ch_pow & 0xffff,
			   qam_read_reg(demod, 0x31));
	} else {
		PR_DVBC("ch_sts is 0x%x, snr %ddB, ber %d, per %d, srate %d, freqoff %dkHz\n",
			demod_sts->ch_sts, demod_sts->ch_snr / 100, demod_sts->ch_ber,
			demod_sts->ch_per, demod_sts->symb_rate, demod_sts->freq_off);
		PR_DVBC("strength %ddb,0xe0 status %u,b4 status %u, dagc_gain %u, power %ddb\n",
			demod_sts->dat1, qam_read_reg(demod, 0x38) & 0xffff,
			qam_read_reg(demod, 0x2d) & 0xffff, qam_read_reg(demod, 0x29) & 0x7f,
			demod_sts->ch_pow & 0xffff);
		PR_DVBC("0x31=0x%x\n\n", qam_read_reg(demod, 0x31));
	}

	return 0;
}

unsigned int atsc_j83b_read_iqr_reg(void)
{
	unsigned int tmp;

	if (!get_dtvpll_init_flag())
		return 0;

	demod_mutex_lock();

	tmp = readl(gbase_atsc() + 8);

	demod_mutex_unlock();

	PR_DBG("[atsc irq] j83b %x\n", tmp);
	return tmp & 0xffffffff;
}

/*copy from demod_dvbc_fsm_reset*/
void demod_j83b_fsm_reset(struct aml_dtvdemod *demod)
{
	qam_write_reg(demod, 0x7, qam_read_reg(demod, 0x7) & ~(1 << 4));
	qam_write_reg(demod, 0x3a, 0x0);
	qam_write_reg(demod, 0x7, qam_read_reg(demod, 0x7) | (1 << 4));
	qam_write_reg(demod, 0x3a, 0x4);
	PR_DVBC("j83b reset fsm\n");
}

static unsigned int atsc_j83b_get_adc_freq(void)
{
	return 24000;
}

void demod_atsc_j83b_store_qam_cfg(struct aml_dtvdemod *demod)
{
	j83b_qam_reg[0] = 0x71;
	j83b_qam_reg[1] = 0x72;
	j83b_qam_reg[2] = 0x73;
	j83b_qam_reg[3] = 0x75;
	j83b_qam_reg[4] = 0x76;
	j83b_qam_reg[5] = 0x7a;
	j83b_qam_reg[6] = 0x93;
	j83b_qam_reg[7] = 0x94;
	j83b_qam_reg[8] = 0x77;
	j83b_qam_reg[9] = 0x7c;
	j83b_qam_reg[10] = 0x7d;
	j83b_qam_reg[11] = 0x7e;
	j83b_qam_reg[12] = 0x9c;
	j83b_qam_reg[13] = 0x78;
	j83b_qam_reg[14] = 0x57;

	j83b_qam_value[0] = qam_read_reg(demod, 0x71);
	j83b_qam_value[1] = qam_read_reg(demod, 0x72);
	j83b_qam_value[2] = qam_read_reg(demod, 0x73);
	j83b_qam_value[3] = qam_read_reg(demod, 0x75);
	j83b_qam_value[4] = qam_read_reg(demod, 0x76);
	j83b_qam_value[5] = qam_read_reg(demod, 0x7a);
	j83b_qam_value[6] = qam_read_reg(demod, 0x93);
	j83b_qam_value[7] = qam_read_reg(demod, 0x94);
	j83b_qam_value[8] = qam_read_reg(demod, 0x77);
	j83b_qam_value[9] = qam_read_reg(demod, 0x7c);
	j83b_qam_value[10] = qam_read_reg(demod, 0x7d);
	j83b_qam_value[11] = qam_read_reg(demod, 0x7e);
	j83b_qam_value[12] = qam_read_reg(demod, 0x9c);
	j83b_qam_value[13] = qam_read_reg(demod, 0x78);
	j83b_qam_value[14] = qam_read_reg(demod, 0x57);
}

void demod_atsc_j83b_restore_qam_cfg(struct aml_dtvdemod *demod)
{
	qam_write_reg(demod, j83b_qam_reg[0], j83b_qam_value[0]);
	qam_write_reg(demod, j83b_qam_reg[1], j83b_qam_value[1]);
	qam_write_reg(demod, j83b_qam_reg[2], j83b_qam_value[2]);
	qam_write_reg(demod, j83b_qam_reg[3], j83b_qam_value[3]);
	qam_write_reg(demod, j83b_qam_reg[4], j83b_qam_value[4]);
	qam_write_reg(demod, j83b_qam_reg[5], j83b_qam_value[5]);
	qam_write_reg(demod, j83b_qam_reg[6], j83b_qam_value[6]);
	qam_write_reg(demod, j83b_qam_reg[7], j83b_qam_value[7]);
	qam_write_reg(demod, j83b_qam_reg[8], j83b_qam_value[8]);
	qam_write_reg(demod, j83b_qam_reg[9], j83b_qam_value[9]);
	qam_write_reg(demod, j83b_qam_reg[10], j83b_qam_value[10]);
	qam_write_reg(demod, j83b_qam_reg[11], j83b_qam_value[11]);
	qam_write_reg(demod, j83b_qam_reg[12], j83b_qam_value[12]);
	qam_write_reg(demod, j83b_qam_reg[13], j83b_qam_value[13]);
	qam_write_reg(demod, j83b_qam_reg[14], j83b_qam_value[14]);
}

void demod_atsc_j83b_set_qam(struct aml_dtvdemod *demod, enum qam_md_e qam, bool auto_sr)
{
	PR_DVBC("%s last_qam_mode %d, qam %d\n",
			__func__, demod->last_qam_mode, qam);

	demod_atsc_j83b_restore_qam_cfg(demod);

	/* 0x2 bit[0-3]: qam_mode_cfg. */
	if (auto_sr)
		qam_write_reg(demod, J83B_QAM_MODE_CFG,
			(qam_read_reg(demod, J83B_QAM_MODE_CFG) & ~7)
				| (qam & 7) | (1 << 17));
	else
		qam_write_reg(demod, J83B_QAM_MODE_CFG,
			(qam_read_reg(demod, J83B_QAM_MODE_CFG) & ~7) | (qam & 7));

	demod->last_qam_mode = qam;

	switch (qam) {
	case QAM_MODE_16:
		qam_write_reg(demod, 0x71, 0x000a2200);

		if (cpu_after_eq(MESON_CPU_MAJOR_ID_TL1))
			qam_write_reg(demod, 0x72, 0xc2b0c49);
		else
			qam_write_reg(demod, 0x72, 0x0c2b04a9);

		qam_write_reg(demod, 0x73, 0x02020000);
		qam_write_reg(demod, 0x75, 0x000e9178);
		qam_write_reg(demod, 0x76, 0x0001c100);
		qam_write_reg(demod, 0x7a, 0x002ab7ff);
		qam_write_reg(demod, 0x93, 0x641a180c);
		qam_write_reg(demod, 0x94, 0x0c141400);
		break;

	case QAM_MODE_32:
		qam_write_reg(demod, 0x71, 0x00061200);
		qam_write_reg(demod, 0x72, 0x099301ae);
		qam_write_reg(demod, 0x73, 0x08080000);
		qam_write_reg(demod, 0x75, 0x000bf10c);
		qam_write_reg(demod, 0x76, 0x0000a05c);
		qam_write_reg(demod, 0x77, 0x001000d6);
		qam_write_reg(demod, 0x7a, 0x0019a7ff);
		qam_write_reg(demod, 0x7c, 0x00111222);

		if (cpu_after_eq(MESON_CPU_MAJOR_ID_TL1))
			qam_write_reg(demod, 0x7d, 0x2020305);
		else
			qam_write_reg(demod, 0x7d, 0x05050505);

		qam_write_reg(demod, 0x7e, 0x03000d0d);
		qam_write_reg(demod, 0x93, 0x641f1d0c);
		qam_write_reg(demod, 0x94, 0x0c1a1a00);
		break;

	case QAM_MODE_64:
		if (cpu_after_eq(MESON_CPU_MAJOR_ID_TL1)) {
			qam_write_reg(demod, 0x9c, 0x2a132100);
			qam_write_reg(demod, 0x57, 0x606060d);
		}
		break;

	case QAM_MODE_128:
		qam_write_reg(demod, 0x71, 0x0002c200);
		qam_write_reg(demod, 0x72, 0x0a6e0059);
		qam_write_reg(demod, 0x73, 0x08080000);
		qam_write_reg(demod, 0x75, 0x000a70e9);
		qam_write_reg(demod, 0x76, 0x00002013);
		qam_write_reg(demod, 0x77, 0x00035068);
		qam_write_reg(demod, 0x78, 0x000ab100);

		if (cpu_after_eq(MESON_CPU_MAJOR_ID_TL1))
			qam_write_reg(demod, 0x7a, 0xba7ff);
		else
			qam_write_reg(demod, 0x7a, 0x002ba7ff);

		qam_write_reg(demod, 0x7c, 0x00111222);

		if (cpu_after_eq(MESON_CPU_MAJOR_ID_TL1))
			qam_write_reg(demod, 0x7d, 0x2020305);
		else
			qam_write_reg(demod, 0x7d, 0x05050505);

		qam_write_reg(demod, 0x7e, 0x03000d0d);
		qam_write_reg(demod, 0x93, 0x642a240c);
		qam_write_reg(demod, 0x94, 0x0c262600);
		break;

	case QAM_MODE_256:
		if (cpu_after_eq(MESON_CPU_MAJOR_ID_TL1)) {
			qam_write_reg(demod, 0x9c, 0x2a232100);
			qam_write_reg(demod, 0x57, 0x606040d);
		}
		break;

	default:
		break;
	}
}

void atsc_j83b_cfg_sr_scan_speed(struct aml_dtvdemod *demod, enum dvbc_sym_speed spd)
{
	switch (spd) {
	case SYM_SPEED_MIDDLE:
		qam_write_reg(demod, J83B_SR_SCAN_SPEED, 0x245bf45c);
		break;
	case SYM_SPEED_HIGH:
		qam_write_reg(demod, J83B_SR_SCAN_SPEED, 0x234cf523);
		break;
	case SYM_SPEED_NORMAL:
	default:
		qam_write_reg(demod, J83B_SR_SCAN_SPEED, 0x235cf459);
		break;
	}
}

void atsc_j83b_cfg_tim_sweep_range(struct aml_dtvdemod *demod, enum dvbc_sym_speed spd)
{
	switch (spd) {
	case SYM_SPEED_MIDDLE:
		qam_write_reg(demod, J83B_TIM_SWEEP_RANGE_CFG, 0x220000);
		break;
	case SYM_SPEED_HIGH:
		qam_write_reg(demod, J83B_TIM_SWEEP_RANGE_CFG, 0x400000);
		break;
	case SYM_SPEED_NORMAL:
	default:
		qam_write_reg(demod, J83B_TIM_SWEEP_RANGE_CFG, 0x400);
		break;
	}
}

void atsc_j83b_qam_auto_scan(struct aml_dtvdemod *demod, int auto_qam_enable)
{
	if (auto_qam_enable) {
		if (demod->auto_sr)
			atsc_j83b_cfg_sr_scan_speed(demod, SYM_SPEED_MIDDLE);
		else
			atsc_j83b_cfg_sr_scan_speed(demod, SYM_SPEED_NORMAL);

		/* j83b */
		if (demod->atsc_mode == QAM_64 || demod->atsc_mode == QAM_256 ||
			demod->atsc_mode == QAM_AUTO ||
			!cpu_after_eq(MESON_CPU_MAJOR_ID_T5D)) {
			atsc_j83b_cfg_tim_sweep_range(demod, SYM_SPEED_NORMAL);
		} else {
			if (demod->auto_sr)
				atsc_j83b_cfg_tim_sweep_range(demod, SYM_SPEED_MIDDLE);
			else
				atsc_j83b_cfg_tim_sweep_range(demod, SYM_SPEED_NORMAL);
		}

		qam_write_reg(demod, 0x4e, 0x12010012);
	} else {
		qam_write_reg(demod, 0x4e, 0x12000012);
	}
}

static void atsc_j83b_cfg_sr_cnt(struct aml_dtvdemod *demod, enum dvbc_sym_speed spd)
{
	switch (spd) {
	case SYM_SPEED_HIGH:
		qam_write_reg(demod, J83B_SYMB_CNT_CFG, 0xffff03ff);
		break;
	case SYM_SPEED_MIDDLE:
	case SYM_SPEED_NORMAL:
	default:
		qam_write_reg(demod, J83B_SYMB_CNT_CFG, 0xffff8ffe);
		break;
	}
}

void atsc_j83b_reg_initial(struct aml_dtvdemod *demod, struct dvb_frontend *fe)
{
	u32 clk_freq;
	u32 adc_freq;
	/*ary no use u8 tuner;*/
	u8 ch_mode;
	u8 agc_mode;
	u32 ch_freq;
	u16 ch_if;
	u16 ch_bw;
	u16 symb_rate;
	u32 phs_cfg;
	int afifo_ctr;
	int max_frq_off, tmp, adc_format;

	clk_freq = demod->demod_status.clk_freq;	/* kHz */
	if (cpu_after_eq(MESON_CPU_MAJOR_ID_TL1))
		adc_freq = demod->demod_status.adc_freq;
	else
		adc_freq  = atsc_j83b_get_adc_freq();/*24000*/;
	adc_format = 1;
	ch_mode = demod->demod_status.ch_mode;
	agc_mode = demod->demod_status.agc_mode;
	ch_freq = demod->demod_status.ch_freq;	/* kHz */
	ch_if = demod->demod_status.ch_if;	/* kHz */
	ch_bw = demod->demod_status.ch_bw;	/* kHz */
	symb_rate = demod->demod_status.symb_rate;	/* k/sec */
	PR_DVBC("ch_if %d,  %d,	%d,  %d, %d %d\n",
		ch_if, ch_mode, ch_freq, ch_bw, symb_rate, adc_freq);
	/* disable irq */
	qam_write_reg(demod, 0x34, 0);

	/* reset */
	/*dvbc_reset(); */
	qam_write_reg(demod, 0x7, qam_read_reg(demod, 0x7) & ~(1 << 4));
	/* disable fsm_en */
	qam_write_reg(demod, 0x7, qam_read_reg(demod, 0x7) & ~(1 << 0));
	/* Sw disable demod */
	qam_write_reg(demod, 0x7, qam_read_reg(demod, 0x7) | (1 << 0));

	if (cpu_after_eq(MESON_CPU_MAJOR_ID_TL1)) {
		if (agc_mode == 1) {
			qam_write_reg(demod, 0x25,
				qam_read_reg(demod, 0x25) & ~(0x1 << 10));
			qam_write_reg(demod, 0x24,
				qam_read_reg(demod, 0x24) | (0x1 << 17));
/*
 *			qam_write_reg(demod, 0x3d,
 *				qam_read_reg(demod, 0x3d) | 0xf);
 */
	}

		if (tuner_find_by_name(fe, "r842") ||
			tuner_find_by_name(fe, "r836") ||
			tuner_find_by_name(fe, "r850")) {
			if (demod->atsc_mode == QAM_64 || demod->atsc_mode == QAM_256 ||
				demod->atsc_mode == QAM_AUTO)
				qam_write_reg(demod, 0x25,
					(qam_read_reg(demod, 0x25) & 0xFFFFFFF0) |
						atsc_j83b_agc_target);
			//else
			//	qam_write_reg(demod, 0x25,
			//		(qam_read_reg(demod, 0x25) & 0xFFFFFFF0) | dvbc_agc_target);
		}
	}

	/* Sw enable demod */
	qam_write_reg(demod, 0x0, 0x0);
	/* QAM_STATUS */
	qam_write_reg(demod, 0x7, 0x00000f00);
	//demod_dvbc_set_qam(demod, ch_mode, demod->auto_sr);
	/*dvbc_write_reg(QAM_BASE+0x00c, 0xfffffffe);*/
	/* // adc_cnt, symb_cnt*/

	if (demod->auto_sr)
		atsc_j83b_cfg_sr_cnt(demod, SYM_SPEED_MIDDLE);
	else
		atsc_j83b_cfg_sr_cnt(demod, SYM_SPEED_NORMAL);

	/* adc_cnt, symb_cnt	by raymond 20121213 */
	if (clk_freq == 0)
		afifo_ctr = 0;
	else
		afifo_ctr = (adc_freq * 256 / clk_freq) + 2;
	if (afifo_ctr > 255)
		afifo_ctr = 255;
	qam_write_reg(demod, 0x4, (afifo_ctr << 16) | 8000);
	/* afifo, rs_cnt_cfg */

	/*dvbc_write_reg(QAM_BASE+0x020, 0x21353e54);*/
	 /* // PHS_reset & TIM_CTRO_ACCURATE  sw_tim_select=0*/
	/*dvbc_write_reg(QAM_BASE+0x020, 0x21b53e54);*/
	 /* //modified by qiancheng*/
	qam_write_reg(demod, J83B_SR_OFFSET_ACC, 0x61b53e54);
	/*modified by qiancheng by raymond 20121208  0x63b53e54 for cci */
	/*	dvbc_write_reg(QAM_BASE+0x020, 0x6192bfe2);*/
	/* //modified by ligg 20130613 auto symb_rate scan*/
	if (adc_freq == 0)
		phs_cfg = 0;
	else
		phs_cfg = (1 << 31) / adc_freq * ch_if / (1 << 8);
	/*	8*fo/fs*2^20 fo=36.125, fs = 28.57114, = 21d775 */
	/* PR_DVBC("phs_cfg = %x\n", phs_cfg); */
	qam_write_reg(demod, 0x9, 0x4c000000 | (phs_cfg & 0x7fffff));
	/* PHS_OFFSET, IF offset, */

	if (adc_freq == 0) {
		max_frq_off = 0;
	} else {
		max_frq_off = (1 << 29) / symb_rate;
		/* max_frq_off = (400KHz * 2^29) /	*/
		/*	 (AD=28571 * symbol_rate=6875) */
		tmp = 40000000 / adc_freq;
		max_frq_off = tmp * max_frq_off;
	}
	PR_DVBC("max_frq_off %x\n", max_frq_off);

	if (cpu_after_eq(MESON_CPU_MAJOR_ID_TL1)) {
		/* j83b */
		if (demod->atsc_mode == QAM_64 || demod->atsc_mode == QAM_256 ||
			demod->atsc_mode == QAM_AUTO ||
			!cpu_after_eq(MESON_CPU_MAJOR_ID_T5D))
			qam_write_reg(demod, J83B_SR_SCAN_SPEED, 0x245cf450);
		else
			qam_write_reg(demod, J83B_SR_SCAN_SPEED, 0x235cf459);
	} else {
		qam_write_reg(demod, 0xb, max_frq_off & 0x3fffffff);
	}
	/* max frequency offset, by raymond 20121208 */

	/* modified by ligg 20130613 --auto symb_rate scan */
	qam_write_reg(demod, 0xd, ((adc_freq & 0xffff) << 16) | (symb_rate & 0xffff));

	/************* hw state machine config **********/
	/*dvbc_write_reg(QAM_BASE + (0x10 << 2), 0x003c);*/
	/* configure symbol rate step 0*/

	/* modified 0x44 0x48 */
	qam_write_reg(demod, 0x11, (symb_rate & 0xffff) * 256);
	/* support CI+ card */
	//ts_clk and ts_data direction 0x98 0x90
	if (demod->ci_mode == 1)
		qam_write_bits(demod, 0x11, 0x00, 24, 8);
	else
		qam_write_bits(demod, 0x11, 0x90, 24, 8);
	/* blind search, configure max symbol_rate		for 7218  fb=3.6M */
	/*dvbc_write_reg(QAM_BASE+0x048, 3600*256);*/
	/* // configure min symbol_rate fb = 6.95M*/
	qam_write_reg(demod, 0x12, (qam_read_reg(demod, 0x12) & ~(0xff << 8)) | 3400 * 256);

	/************* hw state machine config **********/
	if (!cpu_after_eq(MESON_CPU_MAJOR_ID_TL1)) {
		if ((agc_mode & 1) == 0)
			/* freeze if agc */
			qam_write_reg(demod, 0x25,
			qam_read_reg(demod, 0x25) | (0x1 << 10));
		if ((agc_mode & 2) == 0) {
			/* IF control */
			/*freeze rf agc */
			qam_write_reg(demod, 0x25,
			qam_read_reg(demod, 0x25) | (0x1 << 13));
		}
	}

	if (!cpu_after_eq(MESON_CPU_MAJOR_ID_TL1))
		qam_write_reg(demod, 0x28,
		qam_read_reg(demod, 0x28) | (adc_format << 27));

	qam_write_reg(demod, 0x7, qam_read_reg(demod, 0x7) | 0x33);
	/* IMQ, QAM Enable */

	/* start hardware machine */
	qam_write_reg(demod, 0x7, qam_read_reg(demod, 0x7) | (1 << 4));
	qam_write_reg(demod, 0x3a, qam_read_reg(demod, 0x3a) | (1 << 2));

	/* clear irq status */
	qam_read_reg(demod, 0x35);

	/* enable irq */
	qam_write_reg(demod, 0x34, 0x7fff << 3);

	if (is_meson_txlx_cpu() || cpu_after_eq(MESON_CPU_MAJOR_ID_TL1)) {
		/*my_tool setting j83b mode*/
		qam_write_reg(demod, 0x7, 0x10f33);

		switch (demod->demod_status.delsys) {
		case SYS_ATSC:
		case SYS_ATSCMH:
		case SYS_DVBC_ANNEX_B:
			if (is_meson_txlx_cpu()) {
				/*j83b filter para*/
				qam_write_reg(demod, 0x40, 0x3f010201);
				qam_write_reg(demod, 0x41, 0x0a003a3b);
				qam_write_reg(demod, 0x42, 0xe1ee030e);
				qam_write_reg(demod, 0x43, 0x002601f2);
				qam_write_reg(demod, 0x44, 0x009b006b);
				qam_write_reg(demod, 0x45, 0xb3a1905);
				qam_write_reg(demod, 0x46, 0x1c396e07);
				qam_write_reg(demod, 0x47, 0x3801cc08);
				qam_write_reg(demod, 0x48, 0x10800a2);
				qam_write_reg(demod, 0x12, 0x50e1000);
				qam_write_reg(demod, 0x30, 0x41f2f69);
				/*j83b_symbolrate(please see register doc)*/
				qam_write_reg(demod, 0x4d, 0x23d125f7);
				/*for phase noise case 256qam*/
				qam_write_reg(demod, 0x9c, 0x2a232100);
				qam_write_reg(demod, 0x57, 0x606040d);
				/*for phase noise case 64qam*/
				qam_write_reg(demod, 0x54, 0x606050d);
				qam_write_reg(demod, 0x52, 0x346dc);
			}
			break;
		default:
			break;
		}

		atsc_j83b_qam_auto_scan(demod, 1);
	}

	qam_write_reg(demod, 0x65, 0x700c); // offset
	qam_write_reg(demod, 0xb4, 0x32030);
	qam_write_reg(demod, 0xb7, 0x3084);

	// agc gain
	qam_write_reg(demod, 0x24, (qam_read_reg(demod, 0x24) | (1 << 17)));
	qam_write_reg(demod, 0x60, 0x10466000);
	qam_write_reg(demod, 0xac, (qam_read_reg(demod, 0xac) & (~0xff00))
		| 0x800);
	qam_write_reg(demod, 0xae, (qam_read_reg(demod, 0xae)
		& (~0xff000000)) | 0x8000000);

	qam_write_reg(demod, 0x7, 0x10f23);
	qam_write_reg(demod, 0x3a, 0x0);
	qam_write_reg(demod, 0x7, 0x10f33);
	/*enable fsm, sm start work, need wait some time(2ms) for AGC stable*/
	qam_write_reg(demod, 0x3a, 0x4);
	/*auto track*/
	/*dvbc_set_auto_symtrack(demod); */
}

void atsc_j83b_reg_initial_old(struct aml_dtvdemod *demod)
{
	u32 clk_freq;
	u32 adc_freq;
	/*ary no use u8 tuner;*/
	u8 ch_mode;
	u8 agc_mode;
	u32 ch_freq;
	u16 ch_if;
	u16 ch_bw;
	u16 symb_rate;
	u32 phs_cfg;
	int afifo_ctr;
	int max_frq_off, tmp;

	clk_freq = demod->demod_status.clk_freq; /* kHz */
	adc_freq = demod->demod_status.adc_freq; /* kHz */
	/* adc_freq = 25414;*/
	/*ary  no use tuner = demod->demod_status.tuner;*/
	ch_mode = demod->demod_status.ch_mode;
	agc_mode = demod->demod_status.agc_mode;
	ch_freq = demod->demod_status.ch_freq;	/* kHz */
	ch_if = demod->demod_status.ch_if;	/* kHz */
	ch_bw = demod->demod_status.ch_bw;	/* kHz */
	symb_rate = demod->demod_status.symb_rate;	/* k/sec */
	PR_DVBC("ch_if %d, %d, %d, %d, %d\n",
		ch_if, ch_mode, ch_freq, ch_bw, symb_rate);
/*	  ch_mode=4;*/
/*		dvbc_write_reg(DEMOD_CFG_BASE,0x00000007);*/
	/* disable irq */
	dvbc_write_reg(0xd0, 0);

	/* reset */
	/*dvbc_reset(); */
	dvbc_write_reg(0x4,
				dvbc_read_reg(0x4) & ~(1 << 4));
	/* disable fsm_en */
	dvbc_write_reg(0x4,
				dvbc_read_reg(0x4) & ~(1 << 0));
	/* Sw disable demod */
	dvbc_write_reg(0x4,
				dvbc_read_reg(0x4) | (1 << 0));
	/* Sw enable demod */

	dvbc_write_reg(0x000, 0x00000000);
	/* QAM_STATUS */
	dvbc_write_reg(0x004, 0x00000f00);
	/* QAM_GCTL0 */
	dvbc_write_reg(0x008, (ch_mode & 7));
	/* qam mode */

	switch (ch_mode) {
	case 0:/* 16 QAM */
		dvbc_write_reg(0x054, 0x23460224);
		/* EQ_FIR_CTL, */
		dvbc_write_reg(0x068, 0x00c000c0);
		/* EQ_CRTH_SNR */
		dvbc_write_reg(0x074, 0x50001a0);
		/* EQ_TH_LMS  40db	13db */
		dvbc_write_reg(0x07c, 0x003001e9);
		/* EQ_NORM and EQ_TH_MMA */
		/*dvbc_write_reg(QAM_BASE+0x080, 0x000be1ff);*/
		/* // EQ_TH_SMMA0*/
		dvbc_write_reg(0x080, 0x000e01fe);
		/* EQ_TH_SMMA0 */
		dvbc_write_reg(0x084, 0x00000000);
		/* EQ_TH_SMMA1 */
		dvbc_write_reg(0x088, 0x00000000);
		/* EQ_TH_SMMA2 */
		dvbc_write_reg(0x08c, 0x00000000);
		/* EQ_TH_SMMA3 */
		/*dvbc_write_reg(QAM_BASE+0x094, 0x7f800d2b);*/
		/* // AGC_CTRL	ALPS tuner*/
		/*dvbc_write_reg(QAM_BASE+0x094, 0x7f80292b);*/
		/* // Pilips Tuner*/
		/*dvbc_write_reg(QAM_BASE+0x094, 0x7f80292d);*/
		/* // Pilips Tuner*/
		dvbc_write_reg(0x094, 0x7f80092d);
		/* Pilips Tuner */
		dvbc_write_reg(0x0c0, 0x061f2f67);
		/* by raymond 20121213 */
		break;

	case 1:/* 32 QAM */
		dvbc_write_reg(0x054, 0x24560506);
		/* EQ_FIR_CTL, */
		dvbc_write_reg(0x068, 0x00c000c0);
		/* EQ_CRTH_SNR */
		/*dvbc_write_reg(QAM_BASE+0x074, 0x5000260);*/
		/* // EQ_TH_LMS  40db  19db*/
		dvbc_write_reg(0x074, 0x50001f0);
		/* EQ_TH_LMS  40db	17.5db */
		dvbc_write_reg(0x07c, 0x00500102);
		/* EQ_TH_MMA  0x000001cc */
		dvbc_write_reg(0x080, 0x00077140);
		/* EQ_TH_SMMA0 */
		dvbc_write_reg(0x084, 0x001fb000);
		/* EQ_TH_SMMA1 */
		dvbc_write_reg(0x088, 0x00000000);
		/* EQ_TH_SMMA2 */
		dvbc_write_reg(0x08c, 0x00000000);
		/* EQ_TH_SMMA3 */
		/*dvbc_write_reg(QAM_BASE+0x094, 0x7f800d2b);*/
		/* // AGC_CTRL	ALPS tuner*/
		/*dvbc_write_reg(QAM_BASE+0x094, 0x7f80292b);*/
		/* // Pilips Tuner*/
		dvbc_write_reg(0x094, 0x7f80092b);
		/* Pilips Tuner */
		dvbc_write_reg(0x0c0, 0x061f2f67);
		/* by raymond 20121213 */
		break;

	case 2:/* 64 QAM */
		/*dvbc_write_reg(QAM_BASE+0x054, 0x2256033a);*/
		/* // EQ_FIR_CTL,*/
		dvbc_write_reg(0x054, 0x2336043a);
		/* EQ_FIR_CTL, by raymond */
		dvbc_write_reg(0x068, 0x00c000c0);
		/* EQ_CRTH_SNR */
		/*dvbc_write_reg(QAM_BASE+0x074, 0x5000260);*/
		/* // EQ_TH_LMS  40db  19db*/
		dvbc_write_reg(0x074, 0x5000230);
		/* EQ_TH_LMS  40db	17.5db */
		dvbc_write_reg(0x07c, 0x007001bd);
		/* EQ_TH_MMA */
		dvbc_write_reg(0x080, 0x000580ed);
		/* EQ_TH_SMMA0 */
		dvbc_write_reg(0x084, 0x001771fb);
		/* EQ_TH_SMMA1 */
		dvbc_write_reg(0x088, 0x00000000);
		/* EQ_TH_SMMA2 */
		dvbc_write_reg(0x08c, 0x00000000);
		/* EQ_TH_SMMA3 */
		/*dvbc_write_reg(QAM_BASE+0x094, 0x7f800d2c);*/
		/* // AGC_CTRL	ALPS tuner*/
		/*dvbc_write_reg(QAM_BASE+0x094, 0x7f80292c);*/
		/* // Pilips & maxlinear Tuner*/
		dvbc_write_reg(0x094, 0x7f802b3d);
		/* Pilips Tuner & maxlinear Tuner */
		/*dvbc_write_reg(QAM_BASE+0x094, 0x7f802b3a);*/
		/* // Pilips Tuner & maxlinear Tuner*/
		dvbc_write_reg(0x0c0, 0x061f2f67);
		/* by raymond 20121213 */
		break;

	case 3:/* 128 QAM */
		/*dvbc_write_reg(QAM_BASE+0x054, 0x2557046a);*/
		/* // EQ_FIR_CTL,*/
		dvbc_write_reg(0x054, 0x2437067a);
		/* EQ_FIR_CTL, by raymond 20121213 */
		dvbc_write_reg(0x068, 0x00c000d0);
		/* EQ_CRTH_SNR */
		/* dvbc_write_reg(QAM_BASE+0x074, 0x02440240);*/
		/* // EQ_TH_LMS  18.5db  18db*/
		/* dvbc_write_reg(QAM_BASE+0x074, 0x04000400);*/
		/* // EQ_TH_LMS  22db  22.5db*/
		dvbc_write_reg(0x074, 0x5000260);
		/* EQ_TH_LMS  40db	19db */
		/*dvbc_write_reg(QAM_BASE+0x07c, 0x00b000f2);*/
		/* // EQ_TH_MMA0x000000b2*/
		dvbc_write_reg(0x07c, 0x00b00132);
		/* EQ_TH_MMA0x000000b2 by raymond 20121213 */
		dvbc_write_reg(0x080, 0x0003a09d);
		/* EQ_TH_SMMA0 */
		dvbc_write_reg(0x084, 0x000f8150);
		/* EQ_TH_SMMA1 */
		dvbc_write_reg(0x088, 0x001a51f8);
		/* EQ_TH_SMMA2 */
		dvbc_write_reg(0x08c, 0x00000000);
		/* EQ_TH_SMMA3 */
		/*dvbc_write_reg(QAM_BASE+0x094, 0x7f800d2c);*/
		/* // AGC_CTRL	ALPS tuner*/
		/*dvbc_write_reg(QAM_BASE+0x094, 0x7f80292c);*/
		/* // Pilips Tuner*/
		dvbc_write_reg(0x094, 0x7f80092c);
		/* Pilips Tuner */
		dvbc_write_reg(0x0c0, 0x061f2f67);
		/* by raymond 20121213 */
		break;

	case 4:/* 256 QAM */
		/*dvbc_write_reg(QAM_BASE+0x054, 0xa2580588);*/
		/* // EQ_FIR_CTL,*/
		dvbc_write_reg(0x054, 0xa25905f9);
		/* EQ_FIR_CTL, by raymond 20121213 */
		dvbc_write_reg(0x068, 0x01e00220);
		/* EQ_CRTH_SNR */
		/*dvbc_write_reg(QAM_BASE+0x074,  0x50002a0);*/
		/* // EQ_TH_LMS  40db  19db*/
		dvbc_write_reg(0x074, 0x5000270);
		/* EQ_TH_LMS  40db	19db by raymond 201211213 */
		dvbc_write_reg(0x07c, 0x00f001a5);
		/* EQ_TH_MMA */
		dvbc_write_reg(0x080, 0x0002c077);
		/* EQ_TH_SMMA0 */
		dvbc_write_reg(0x084, 0x000bc0fe);
		/* EQ_TH_SMMA1 */
		dvbc_write_reg(0x088, 0x0013f17e);
		/* EQ_TH_SMMA2 */
		dvbc_write_reg(0x08c, 0x01bc01f9);
		/* EQ_TH_SMMA3 */
		/*dvbc_write_reg(QAM_BASE+0x094, 0x7f800d2c);*/
		/* // AGC_CTRL	ALPS tuner*/
		/*dvbc_write_reg(QAM_BASE+0x094, 0x7f80292c);*/
		/* // Pilips Tuner*/
		/*dvbc_write_reg(QAM_BASE+0x094, 0x7f80292d);*/
		/* // Maxlinear Tuner*/
		dvbc_write_reg(0x094, 0x7f80092d);
		/* Maxlinear Tuner */
		dvbc_write_reg(0x0c0, 0x061f2f67);
		/* by raymond 20121213, when adc=35M,sys=70M,*/
		/* its better than 0x61f2f66*/
		break;
	default:		/*64qam */
		/*dvbc_write_reg(QAM_BASE+0x054, 0x2256033a);*/
		/* // EQ_FIR_CTL,*/
		dvbc_write_reg(0x054, 0x2336043a);
		/* EQ_FIR_CTL, by raymond */
		dvbc_write_reg(0x068, 0x00c000c0);
		/* EQ_CRTH_SNR */
		/* EQ_TH_LMS  40db	19db */
		dvbc_write_reg(0x074, 0x5000230);
		/* EQ_TH_LMS  40db	17.5db */
		dvbc_write_reg(0x07c, 0x007001bd);
		/* EQ_TH_MMA */
		dvbc_write_reg(0x080, 0x000580ed);
		/* EQ_TH_SMMA0 */
		dvbc_write_reg(0x084, 0x001771fb);
		/* EQ_TH_SMMA1 */
		dvbc_write_reg(0x088, 0x00000000);
		/* EQ_TH_SMMA2 */
		dvbc_write_reg(0x08c, 0x00000000);
		/* EQ_TH_SMMA3 */
		/*dvbc_write_reg(QAM_BASE+0x094, 0x7f800d2c);*/
		/* // AGC_CTRL	ALPS tuner*/
		/*dvbc_write_reg(QAM_BASE+0x094, 0x7f80292c);*/
		/* // Pilips & maxlinear Tuner*/
		dvbc_write_reg(0x094, 0x7f802b3d);
		/* Pilips Tuner & maxlinear Tuner */
		/*dvbc_write_reg(QAM_BASE+0x094, 0x7f802b3a);*/
		/* // Pilips Tuner & maxlinear Tuner*/
		dvbc_write_reg(0x0c0, 0x061f2f67);
		/* by raymond 20121213 */
		break;
	}

	/*dvbc_write_reg(QAM_BASE+0x00c, 0xfffffffe);*/
	/* // adc_cnt, symb_cnt*/
	dvbc_write_reg(0x00c, 0xffff8ffe);
	/* adc_cnt, symb_cnt	by raymond 20121213 */
	if (clk_freq == 0)
		afifo_ctr = 0;
	else
		afifo_ctr = (adc_freq * 256 / clk_freq) + 2;
	if (afifo_ctr > 255)
		afifo_ctr = 255;
	dvbc_write_reg(0x010, (afifo_ctr << 16) | 8000);
	/* afifo, rs_cnt_cfg */

	/*dvbc_write_reg(QAM_BASE+0x020, 0x21353e54);*/
	/* // PHS_reset & TIM_CTRO_ACCURATE  sw_tim_select=0*/
	/*dvbc_write_reg(QAM_BASE+0x020, 0x21b53e54);*/
	/* //modified by qiancheng*/
	dvbc_write_reg(0x020, 0x61b53e54);
	/*modified by qiancheng by raymond 20121208  0x63b53e54 for cci */
	/*	dvbc_write_reg(QAM_BASE+0x020, 0x6192bfe2);*/
	/* //modified by ligg 20130613 auto symb_rate scan*/
	if (adc_freq == 0)
		phs_cfg = 0;
	else
		phs_cfg = (1 << 31) / adc_freq * ch_if / (1 << 8);
	/*	8*fo/fs*2^20 fo=36.125, fs = 28.57114, = 21d775 */
	/* PR_DVBC("phs_cfg = %x\n", phs_cfg); */
	dvbc_write_reg(0x024, 0x4c000000 | (phs_cfg & 0x7fffff));
	/* PHS_OFFSET, IF offset, */

	if (adc_freq == 0) {
		max_frq_off = 0;
	} else {
		max_frq_off = (1 << 29) / symb_rate;
		/* max_frq_off = (400KHz * 2^29) / */
		/*   (AD=28571 * symbol_rate=6875) */
		tmp = 40000000 / adc_freq;
		max_frq_off = tmp * max_frq_off;
	}
	PR_DVBC("max_frq_off %x\n", max_frq_off);
	dvbc_write_reg(0x02c, max_frq_off & 0x3fffffff);
	/* max frequency offset, by raymond 20121208 */

	/*dvbc_write_reg(QAM_BASE+0x030, 0x011bf400);*/
	/* // TIM_CTL0 start speed is 0,  when know symbol rate*/
	dvbc_write_reg(0x030, 0x245cf451);
	/*MODIFIED BY QIANCHENG */
/*		dvbc_write_reg(QAM_BASE+0x030, 0x245bf451);*/
/* //modified by ligg 20130613 --auto symb_rate scan*/
	dvbc_write_reg(0x034,
			  ((adc_freq & 0xffff) << 16) | (symb_rate & 0xffff));

	dvbc_write_reg(0x038, 0x00400000);
	/* TIM_SWEEP_RANGE 16000 */

/************* hw state machine config **********/
	dvbc_write_reg(0x040, 0x003c);
/* configure symbol rate step 0*/

	/* modified 0x44 0x48 */
	dvbc_write_reg(0x044, (symb_rate & 0xffff) * 256);
	/* blind search, configure max symbol_rate for 7218  fb=3.6M */
	/*dvbc_write_reg(QAM_BASE+0x048, 3600*256);*/
	/* // configure min symbol_rate fb = 6.95M*/
	dvbc_write_reg(0x048, 3400 * 256);
	/* configure min symbol_rate fb = 6.95M */

	/*dvbc_write_reg(QAM_BASE+0x0c0, 0xffffff68); // threshold */
	/*dvbc_write_reg(QAM_BASE+0x0c0, 0xffffff6f); // threshold */
	/*dvbc_write_reg(QAM_BASE+0x0c0, 0xfffffd68); // threshold */
	/*dvbc_write_reg(QAM_BASE+0x0c0, 0xffffff68); // threshold */
	/*dvbc_write_reg(QAM_BASE+0x0c0, 0xffffff68); // threshold */
	/*dvbc_write_reg(QAM_BASE+0x0c0, 0xffff2f67);*/
	/* // threshold for skyworth*/
	/* dvbc_write_reg(QAM_BASE+0x0c0, 0x061f2f67); // by raymond 20121208 */
	/* dvbc_write_reg(QAM_BASE+0x0c0, 0x061f2f66);*/
	/* // by raymond 20121213, remove it to every constellation*/
/************* hw state machine config **********/

	dvbc_write_reg(0x04c, 0x00008800);	/* reserved */

	/*dvbc_write_reg(QAM_BASE+0x050, 0x00000002);  // EQ_CTL0 */
	dvbc_write_reg(0x050, 0x01472002);
	/* EQ_CTL0 by raymond 20121208 */

	/*dvbc_write_reg(QAM_BASE+0x058, 0xff550e1e);  // EQ_FIR_INITPOS */
	dvbc_write_reg(0x058, 0xff100e1e);
	/* EQ_FIR_INITPOS for skyworth */

	dvbc_write_reg(0x05c, 0x019a0000);	/* EQ_FIR_INITVAL0 */
	dvbc_write_reg(0x060, 0x019a0000);	/* EQ_FIR_INITVAL1 */

	/*dvbc_write_reg(QAM_BASE+0x064, 0x01101128);  // EQ_CRTH_TIMES */
	dvbc_write_reg(0x064, 0x010a1128);
	/* EQ_CRTH_TIMES for skyworth */
	dvbc_write_reg(0x06c, 0x00041a05);	/* EQ_CRTH_PPM */

	dvbc_write_reg(0x070, 0xffb9aa01);	/* EQ_CRLP */

	/*dvbc_write_reg(QAM_BASE+0x090, 0x00020bd5); // agc control */
	dvbc_write_reg(0x090, 0x00000bd5);	/* agc control */

	/* agc control */
	/* dvbc_write_reg(QAM_BASE+0x094, 0x7f800d2c);// AGC_CTRL  ALPS tuner */
	/* dvbc_write_reg(QAM_BASE+0x094, 0x7f80292c);	  // Pilips Tuner */
	if ((agc_mode & 1) == 0)
		/* freeze if agc */
		dvbc_write_reg(0x094,
				  dvbc_read_reg(0x94) | (0x1 << 10));
	if ((agc_mode & 2) == 0) {
		/* IF control */
		/*freeze rf agc */
		dvbc_write_reg(0x094,
				  dvbc_read_reg(0x94) | (0x1 << 13));
	}
	/*Maxlinear Tuner */
	/*dvbc_write_reg(QAM_BASE+0x094, 0x7f80292d); */
	dvbc_write_reg(0x098, 0x9fcc8190);
	/* AGC_IFGAIN_CTRL */
	/*dvbc_write_reg(QAM_BASE+0x0a0, 0x0e028c00);*/
	/* // AGC_RFGAIN_CTRL 0x0e020800*/
	/*dvbc_write_reg(QAM_BASE+0x0a0, 0x0e03cc00);*/
	/* // AGC_RFGAIN_CTRL 0x0e020800*/
	/*dvbc_write_reg(QAM_BASE+0x0a0, 0x0e028700);*/
	/* // AGC_RFGAIN_CTRL 0x0e020800 now*/
	/*dvbc_write_reg(QAM_BASE+0x0a0, 0x0e03cd00);*/
	/* // AGC_RFGAIN_CTRL 0x0e020800*/
	/*dvbc_write_reg(QAM_BASE+0x0a0, 0x0603cd11);*/
	/* // AGC_RFGAIN_CTRL 0x0e020800 by raymond,*/
	/* if Adjcent channel test, maybe it need change.20121208 ad invert*/
	dvbc_write_reg(0x0a0, 0x0603cd10);
	/* AGC_RFGAIN_CTRL 0x0e020800 by raymond,*/
	/* if Adjcent channel test, maybe it need change.*/
	/* 20121208 ad invert,20130221, suit for two path channel.*/

	dvbc_write_reg(0x004,
			dvbc_read_reg(0x004) | 0x33);
	/* IMQ, QAM Enable */

	/* start hardware machine */
	/*dvbc_sw_reset(0x004, 4); */
	dvbc_write_reg(0x4,
			dvbc_read_reg(0x4) | (1 << 4));
	dvbc_write_reg(0x0e8,
			(dvbc_read_reg(0x0e8) | (1 << 2)));

	/* clear irq status */
	dvbc_read_reg(0xd4);

	/* enable irq */
	dvbc_write_reg(0xd0, 0x7fff << 3);

	/*auto track*/
	/*dvbc_set_auto_symtrack(demod); */
}

void demod_atsc_j83b_qam_reset(struct aml_dtvdemod *demod)
{
	/* reset qam register */
	qam_write_reg(demod, 0x1, qam_read_reg(demod, 0x1) | (1 << 0));
	qam_write_reg(demod, 0x1, qam_read_reg(demod, 0x1) & ~(1 << 0));

	PR_DVBC("j83b reset qam\n");
}

int atsc_j83b_set_ch(struct aml_dtvdemod *demod, struct aml_demod_dvbc *demod_dvbc,
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
		PR_DVBC("[id %d] auto QAM, set mode %d\n", demod->id, mode);
	}

	if (ch_freq < 1000 || ch_freq > 900000) {
		PR_DVBC("[id %d] Error: Invalid Channel Freq option %d\n", demod->id, ch_freq);
		ch_freq = 474000;
		ret = -1;
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

	demod_atsc_j83b_qam_reset(demod);

	if (is_meson_gxtvbb_cpu() || is_meson_txl_cpu())
		atsc_j83b_reg_initial_old(demod);
	else if (cpu_after_eq(MESON_CPU_MAJOR_ID_TXLX) && !is_meson_txhd_cpu())
		atsc_j83b_reg_initial(demod, fe);

	return ret;
}
