// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2021 Amlogic, Inc. All rights reserved.
 */

#define __DVB_CORE__

#include "demod_func.h"
#include "amlfrontend.h"
#ifdef AML_DEMOD_SUPPORT_DVBT
#include "dvbt_func.h"
#endif
#include "aml_demod.h"

#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/unistd.h>
#include <linux/delay.h>
/*#include "acf_filter_coefficient.h"*/
#include <linux/mutex.h>
#include <linux/amlogic/media/frame_provider/tvin/tvin.h>
#include "acf_filter_coefficient.h"

MODULE_PARM_DESC(front_agc_target, "\n\t\t front_agc_target");
static unsigned int front_agc_target;
module_param(front_agc_target, int, 0644);

/* protect register access */
static struct mutex mp;
static struct mutex dtvpll_init_lock;
static int dtvpll_init;

#if defined DEMOD_FPGA_VERSION
static int fpga_version = 1;
#else
static int fpga_version = -1;
#endif

void dtvpll_lock_init(void)
{
	mutex_init(&dtvpll_init_lock);
}

void dtvpll_init_flag(int on)
{
	mutex_lock(&dtvpll_init_lock);
	dtvpll_init = on;
	mutex_unlock(&dtvpll_init_lock);
	pr_err("%s %d\n", __func__, on);
}

int get_dtvpll_init_flag(void)
{
	int val;

	/*mutex_lock(&dtvpll_init_lock);*/
	val = dtvpll_init;
	/*mutex_unlock(&dtvpll_init_lock);*/
	if (!val)
		pr_err("%s: %d\n", __func__, val);
	return val;
}

int adc_dpll_setup(int clk_a, int clk_b, int clk_sys, struct aml_demod_sta *demod_sta)
{
	int unit, found, ena, enb, div2;
	int pll_m, pll_n, pll_od_a, pll_od_b, pll_xd_a, pll_xd_b;
	long freq_osc, freq_dco, freq_b, freq_a, freq_sys;
	long freq_b_act, freq_a_act, freq_sys_act, err_tmp, best_err;
	union adc_pll_cntl adc_pll_cntl;
	union adc_pll_cntl2 adc_pll_cntl2;
	union adc_pll_cntl3 adc_pll_cntl3;
	union adc_pll_cntl4 adc_pll_cntl4;
	union demod_dig_clk dig_clk_cfg;

	struct dfe_adcpll_para ddemod_pll;
	int sts_pll = 0;

	if (cpu_after_eq(MESON_CPU_MAJOR_ID_TL1)) {
		dtvpll_init_flag(1);

		return sts_pll;
	}

	dig_clk_cfg.d32 = 0;
	adc_pll_cntl.d32 = 0;
	adc_pll_cntl2.d32 = 0;
	adc_pll_cntl3.d32 = 0;
	adc_pll_cntl4.d32 = 0;
	dig_clk_cfg.d32 = 0;

	PR_DBG("target clk_a %d  clk_b %d\n", clk_a, clk_b);

	unit = 10000;		/* 10000 as 1 MHz, 0.1 kHz resolution. */
	freq_osc = 24 * unit;

	if (clk_a < 1000)
		freq_a = clk_a * unit;
	else
		freq_a = clk_a * (unit / 1000);

	if (clk_b < 1000)
		freq_b = clk_b * unit;
	else
		freq_b = clk_b * (unit / 1000);

	ena = clk_a > 0 ? 1 : 0;
	enb = clk_b > 0 ? 1 : 0;

	if (ena || enb)
		adc_pll_cntl3.b.enable = 1;
	adc_pll_cntl3.b.reset = 1;

	found = 0;
	best_err = 100 * unit;
	pll_od_a = 1;
	pll_od_b = 1;
	pll_n = 1;
	for (pll_m = 1; pll_m < 512; pll_m++) {
		/* for (pll_n=1; pll_n<=5; pll_n++) { */
		#if 0
		if ((is_meson_txl_cpu()) || (is_meson_txlx_cpu())
			|| (is_meson_gxlx_cpu()) || (is_meson_txhd_cpu())) {
			freq_dco = freq_osc * pll_m / pll_n / 2;
			/*txl add div2*/
			if (freq_dco < 700 * unit || freq_dco > 1000 * unit)
				continue;
		} else {
			freq_dco = freq_osc * pll_m / pll_n;
			if (freq_dco < 750 * unit || freq_dco > 1550 * unit)
				continue;
		}
		#else
		if (cpu_after_eq(MESON_CPU_MAJOR_ID_TXL)) {
			/*txl add div2*/
			freq_dco = freq_osc * pll_m / pll_n / 2;
			if (freq_dco < 700 * unit || freq_dco > 1000 * unit)
				continue;

		} else {
			freq_dco = freq_osc * pll_m / pll_n;
			if (freq_dco < 750 * unit || freq_dco > 1550 * unit)
				continue;

		}
		#endif
		pll_xd_a = freq_dco / (1 << pll_od_a) / freq_a;
		pll_xd_b = freq_dco / (1 << pll_od_b) / freq_b;

		freq_a_act = freq_dco / (1 << pll_od_a) / pll_xd_a;
		freq_b_act = freq_dco / (1 << pll_od_b) / pll_xd_b;

		err_tmp = (freq_a_act - freq_a) * ena + (freq_b_act - freq_b) *
		    enb;

		if (err_tmp >= best_err)
			continue;

		adc_pll_cntl.b.pll_m = pll_m;
		adc_pll_cntl.b.pll_n = pll_n;
		adc_pll_cntl.b.pll_od0 = pll_od_b;
		adc_pll_cntl.b.pll_od1 = pll_od_a;
		adc_pll_cntl.b.pll_xd0 = pll_xd_b;
		adc_pll_cntl.b.pll_xd1 = pll_xd_a;
		#if 0
		if ((is_meson_txl_cpu()) || (is_meson_txlx_cpu())
			|| (is_meson_gxlx_cpu()) || (is_meson_txhd_cpu())) {
			adc_pll_cntl4.b.pll_od3 = 0;
			adc_pll_cntl.b.pll_od2 = 0;
		} else {
			adc_pll_cntl2.b.div2_ctrl =
				freq_dco > 1000 * unit ? 1 : 0;
		}
		#else
		/*if (cpu_after_eq(MESON_CPU_MAJOR_ID_TXL)) {*/
		if (cpu_after_eq(MESON_CPU_MAJOR_ID_TXL)) {
			adc_pll_cntl4.b.pll_od3 = 0;
			adc_pll_cntl.b.pll_od2 = 0;

		} else {
			adc_pll_cntl2.b.div2_ctrl =
				freq_dco > 1000 * unit ? 1 : 0;

		}
		#endif
		found = 1;
		best_err = err_tmp;
		/* } */
	}

	pll_m = adc_pll_cntl.b.pll_m;
	pll_n = adc_pll_cntl.b.pll_n;
	pll_od_b = adc_pll_cntl.b.pll_od0;
	pll_od_a = adc_pll_cntl.b.pll_od1;
	pll_xd_b = adc_pll_cntl.b.pll_xd0;
	pll_xd_a = adc_pll_cntl.b.pll_xd1;

	#if 0
	if ((is_meson_txl_cpu()) || (is_meson_txlx_cpu())
		|| (is_meson_gxlx_cpu()) || (is_meson_txhd_cpu()))
		div2 = 1;
	else
		div2 = adc_pll_cntl2.b.div2_ctrl;

	#else
	/*if (cpu_after_eq(MESON_CPU_MAJOR_ID_TXL))*/
	if (cpu_after_eq(MESON_CPU_MAJOR_ID_TXL))
		div2 = 1;
	else
		div2 = adc_pll_cntl2.b.div2_ctrl;

	#endif
	/*
	 * p_adc_pll_cntl  =  adc_pll_cntl.d32;
	 * p_adc_pll_cntl2 = adc_pll_cntl2.d32;
	 * p_adc_pll_cntl3 = adc_pll_cntl3.d32;
	 * p_adc_pll_cntl4 = adc_pll_cntl4.d32;
	 */
	adc_pll_cntl3.b.reset = 0;
	/* *p_adc_pll_cntl3 = adc_pll_cntl3.d32; */
	if (!found) {
		PR_DBG(" ERROR can't setup %7ld kHz %7ld kHz\n",
		       freq_b / (unit / 1000), freq_a / (unit / 1000));
	} else {
	#if 0
		if ((is_meson_txl_cpu()) || (is_meson_txlx_cpu())
			|| (is_meson_gxlx_cpu()) || (is_meson_txhd_cpu()))
			freq_dco = freq_osc * pll_m / pll_n / 2;
		else
			freq_dco = freq_osc * pll_m / pll_n;
	#else
		/*if (cpu_after_eq(MESON_CPU_MAJOR_ID_TXL))*/
		if (cpu_after_eq(MESON_CPU_MAJOR_ID_TXL))
			freq_dco = freq_osc * pll_m / pll_n / 2;
		else
			freq_dco = freq_osc * pll_m / pll_n;
	#endif
		PR_DBG(" ADC PLL  M %3d   N %3d\n", pll_m, pll_n);
		PR_DBG(" ADC PLL DCO %ld kHz\n", freq_dco / (unit / 1000));

		PR_DBG(" ADC PLL XD %3d  OD %3d\n", pll_xd_b, pll_od_b);
		PR_DBG(" ADC PLL XD %3d  OD %3d\n", pll_xd_a, pll_od_a);

		freq_a_act = freq_dco / (1 << pll_od_a) / pll_xd_a;
		freq_b_act = freq_dco / (1 << pll_od_b) / pll_xd_b;

		PR_DBG(" B %7ld kHz %7ld kHz\n",
		       freq_b / (unit / 1000), freq_b_act / (unit / 1000));
		PR_DBG(" A %7ld kHz %7ld kHz\n",
		       freq_a / (unit / 1000), freq_a_act / (unit / 1000));

		if (clk_sys > 0) {
			dig_clk_cfg.b.demod_clk_en = 1;
			dig_clk_cfg.b.demod_clk_sel = 3;
			if (clk_sys < 1000)
				freq_sys = clk_sys * unit;
			else
				freq_sys = clk_sys * (unit / 1000);

			dig_clk_cfg.b.demod_clk_div = freq_dco / (1 + div2) /
			    freq_sys - 1;
			freq_sys_act = freq_dco / (1 + div2) /
			    (dig_clk_cfg.b.demod_clk_div + 1);
			PR_DBG(" SYS %7ld kHz div %d+1  %7ld kHz\n",
			       freq_sys / (unit / 1000),
			       dig_clk_cfg.b.demod_clk_div,
			       freq_sys_act / (unit / 1000));
		} else {
			dig_clk_cfg.b.demod_clk_en = 0;
		}

		/* *p_demod_dig_clk = dig_clk_cfg.d32; */
	}

	ddemod_pll.adcpllctl = adc_pll_cntl.d32;
	ddemod_pll.demodctl = dig_clk_cfg.d32;
	ddemod_pll.mode = 0;

	switch (demod_sta->delsys) {
	case SYS_ATSC:
	case SYS_ATSCMH:
	case SYS_DVBC_ANNEX_B:
		ddemod_pll.mode = 1; //for atsc
		break;

	default:
		break;
	}

#ifdef CONFIG_AMLOGIC_MEDIA_ADC
	sts_pll = adc_set_pll_cntl(1, ADC_DTV_DEMODPLL, &ddemod_pll);
#endif
	if (sts_pll < 0) {
		/*set pll fail*/
		PR_ERR("%s: set pll fail %d !\n", __func__, sts_pll);
	} else {
		dtvpll_init_flag(1);
	}

	return sts_pll;
}

int demod_set_adc_core_clk(int adc_clk, int sys_clk, struct aml_demod_sta *demod_sta)
{
	return adc_dpll_setup(25, adc_clk, sys_clk, demod_sta);
}

void demod_set_cbus_reg(unsigned int data, unsigned int addr)
{
	void __iomem *vaddr;

	PR_DBG("[cbus][write]%x\n", (IO_CBUS_PHY_BASE + (addr << 2)));
	vaddr = ioremap((IO_CBUS_PHY_BASE + (addr << 2)), 0x4);
	writel(data, vaddr);
	iounmap(vaddr);
}

unsigned int demod_read_cbus_reg(unsigned int addr)
{
/* return __raw_readl(CBUS_REG_ADDR(addr)); */
	unsigned int tmp;
	void __iomem *vaddr;

	vaddr = ioremap((IO_CBUS_PHY_BASE + (addr << 2)), 0x4);
	tmp = readl(vaddr);
	iounmap(vaddr);
/* tmp = aml_read_cbus(addr); */
	PR_DBG("[cbus][read]%x,data is %x\n",
	(IO_CBUS_PHY_BASE + (addr << 2)), tmp);
	return tmp;
}

void demod_set_ao_reg(unsigned int data, unsigned int addr)
{
	writel(data, gbase_aobus() + addr);
}

unsigned int demod_read_ao_reg(unsigned int addr)
{
	unsigned int tmp;

	tmp = readl(gbase_aobus() + addr);

	return tmp;
}


void demod_set_demod_reg(unsigned int data, unsigned int addr)
{
	void __iomem *vaddr;

	demod_mutex_lock();
	vaddr = ioremap((addr), 0x4);
	writel(data, vaddr);
	iounmap(vaddr);
	demod_mutex_unlock();
}

void demod_set_tvfe_reg(unsigned int data, unsigned int addr)
{
	void __iomem *vaddr;

	demod_mutex_lock();
	/* printk("[demod][write]%x,data is %x\n",(addr),data);*/
	vaddr = ioremap((addr), 0x4);
	writel(data, vaddr);
	iounmap(vaddr);
	demod_mutex_unlock();
}

unsigned int demod_read_demod_reg(unsigned int addr)
{
	unsigned int tmp;
	void __iomem *vaddr;

	demod_mutex_lock();
	vaddr = ioremap((addr), 0x4);
	tmp = readl(vaddr);
	iounmap(vaddr);
	demod_mutex_unlock();

	return tmp;
}

void power_sw_hiu_reg(int on)
{
	if (on == PWR_ON) {
		if (cpu_after_eq(MESON_CPU_MAJOR_ID_TL1))
			dd_hiu_reg_write(HHI_DEMOD_MEM_PD_REG, 0);
		else
			dd_hiu_reg_write(HHI_DEMOD_MEM_PD_REG,
			(dd_hiu_reg_read(HHI_DEMOD_MEM_PD_REG)
			& (~0x2fff)));
	} else {
		if (cpu_after_eq(MESON_CPU_MAJOR_ID_TL1))
			dd_hiu_reg_write(HHI_DEMOD_MEM_PD_REG,
				0xffffffff);
		else
			dd_hiu_reg_write(HHI_DEMOD_MEM_PD_REG,
			(dd_hiu_reg_read(HHI_DEMOD_MEM_PD_REG) | 0x2fff));

	}
}
void power_sw_reset_reg(int en)
{
	if (en) {
		reset_reg_write(RESET_RESET0_LEVEL,
			(reset_reg_read(RESET_RESET0_LEVEL) & (~(0x1 << 8))));
	} else {
		reset_reg_write(RESET_RESET0_LEVEL,
			(reset_reg_read(RESET_RESET0_LEVEL) | (0x1 << 8)));

	}
}

void demod_power_switch(int pwr_cntl)
{
	int reg_data;
	unsigned int pwr_slp_bit;
	unsigned int pwr_iso_bit;
	struct amldtvdemod_device_s *devp = dtvdemod_get_dev();

	if (!devp) {
		pr_err("%s devp is NULL\n", __func__);
		return;
	}

	/* DO NOT set any power related regs in T5 */
	if (devp->data->hw_ver >= DTVDEMOD_HW_T5) {
		if (devp->data->hw_ver >= DTVDEMOD_HW_T5D) {
			/* UART pin mux for RISCV debug */
			/* demod_set_ao_reg(0x104411, 0x14); */
		}

		return;
	}

	if (cpu_after_eq(MESON_CPU_MAJOR_ID_TM2)) {
		pwr_slp_bit = 23;
		pwr_iso_bit = 23;
	} else {
		pwr_slp_bit = 10;
		pwr_iso_bit = 14;
	}

	if (is_meson_gxlx_cpu()) {
		PR_DBG("[PWR]: GXLX not support power switch,power mem\n");
		power_sw_hiu_reg(PWR_ON);
	} else if (pwr_cntl == PWR_ON) {
		PR_DBG("[PWR]: Power on demod_comp %x,%x\n",
		       AO_RTI_GEN_PWR_SLEEP0, AO_RTI_GEN_PWR_ISO0);
		/* Powerup demod_comb */
		reg_data = demod_read_ao_reg(AO_RTI_GEN_PWR_SLEEP0);
		demod_set_ao_reg((reg_data & (~(0x1 << pwr_slp_bit))),
				 AO_RTI_GEN_PWR_SLEEP0);
		/* Power up memory */
		power_sw_hiu_reg(PWR_ON);
		/* reset */
		power_sw_reset_reg(1);

		if (cpu_after_eq(MESON_CPU_MAJOR_ID_TM2))
			/* remove isolation */
			demod_set_ao_reg(
				(demod_read_ao_reg(AO_RTI_GEN_PWR_ISO0) &
				  (~(0x1 << pwr_iso_bit))),
				  AO_RTI_GEN_PWR_ISO0);
		else
			/* remove isolation */
			demod_set_ao_reg(
				(demod_read_ao_reg(AO_RTI_GEN_PWR_ISO0) &
				  (~(0x3 << pwr_iso_bit))),
				  AO_RTI_GEN_PWR_ISO0);

		/* pull up reset */
		power_sw_reset_reg(0);
	} else {
		PR_DBG("[PWR]: Power off demod_comp\n");

		if (cpu_after_eq(MESON_CPU_MAJOR_ID_TM2))
			/* add isolation */
			demod_set_ao_reg(
				(demod_read_ao_reg(AO_RTI_GEN_PWR_ISO0) |
				  (0x1 << pwr_iso_bit)), AO_RTI_GEN_PWR_ISO0);
		else
			/* add isolation */
			demod_set_ao_reg(
				(demod_read_ao_reg(AO_RTI_GEN_PWR_ISO0) |
				  (0x3 << pwr_iso_bit)), AO_RTI_GEN_PWR_ISO0);

		/* power down memory */
		power_sw_hiu_reg(PWR_OFF);
		/* power down demod_comb */
		reg_data = demod_read_ao_reg(AO_RTI_GEN_PWR_SLEEP0);
		demod_set_ao_reg((reg_data | (0x1 << pwr_slp_bit)),
				 AO_RTI_GEN_PWR_SLEEP0);
	}
}

/* 0:DVBC/J.83B, 1:DVBT/ISDBT, 2:ATSC, 3:DTMB */
void demod_set_mode_ts(struct aml_dtvdemod *demod, enum fe_delivery_system delsys)
{
	struct amldtvdemod_device_s *devp = (struct amldtvdemod_device_s *)demod->priv;
	union demod_cfg0 cfg0 = { .d32 = 0, };
	unsigned int dvbt_mode = 0x11; // demod_cfg3

	cfg0.b.adc_format = 1;
	cfg0.b.adc_regout = 1;

	switch (delsys) {
	case SYS_DTMB:
		cfg0.b.ts_sel = 1;
		cfg0.b.mode = 1;
		if (cpu_after_eq(MESON_CPU_MAJOR_ID_TL1)) {
			cfg0.b.adc_format = 0;
			cfg0.b.adc_regout = 0;
		}
		break;

	case SYS_ISDBT:
		cfg0.b.ts_sel = 1<<1;
		cfg0.b.mode = 1<<1;
		cfg0.b.adc_format = 0;
		cfg0.b.adc_regout = 0;
		break;

	case SYS_ATSC:
	case SYS_ATSCMH:
	case SYS_DVBC_ANNEX_B:
		cfg0.b.ts_sel = 1<<2;
		cfg0.b.mode = 1<<2;
		cfg0.b.adc_format = 0;
		if (!cpu_after_eq(MESON_CPU_MAJOR_ID_TL1)) {
			cfg0.b.adc_regout = 1;
			cfg0.b.adc_regadj = 2;
		}
		break;

	case SYS_DVBC_ANNEX_A:
	case SYS_DVBC_ANNEX_C:
#ifndef CONFIG_AMLOGIC_REMOVE_OLD
		if (is_meson_gxtvbb_cpu() || is_meson_txl_cpu()) {
			cfg0.b.ts_sel = 2;
			cfg0.b.mode = 7;
			cfg0.b.adc_format = 1;
			cfg0.b.adc_regout = 0;
		} else {
#else
		{
#endif
			cfg0.b.ts_sel = 1<<3;
			cfg0.b.mode = 1<<3;
			cfg0.b.adc_format = 0;
			cfg0.b.adc_regout = 0;
			//for new dvbc_blind_scan mode
			if (!devp->blind_scan_stop && demod->dvbc_sel == 1)
				//when use DVB-C ch1, ADC I/Q should be swapped.
				cfg0.b.adc_swap = 1;
		}
		break;

	case SYS_DVBT:
	case SYS_DVBT2:
		dvbt_mode = 0x110011;

		/* fix T and T2 channel switch unlock. */
		if (demod_is_t5d_cpu(devp) || is_meson_t3_cpu() ||
		is_meson_t5w_cpu() || is_meson_t5m_cpu() || is_meson_t3x_cpu())
			cfg0.b.adc_regout = 0;

		break;

	case SYS_DVBS:
	case SYS_DVBS2:
		dvbt_mode = 0x220011;
		break;

	default:
		break;
	}

	PR_DBG("%s: delsys %d cfg0 %#x\n", __func__, delsys, cfg0.d32);
	demod_top_write_reg(DEMOD_TOP_REG0, cfg0.d32);
	demod_top_write_reg(DEMOD_TOP_REGC, dvbt_mode);
}

int clocks_set_sys_defaults(struct aml_dtvdemod *demod, unsigned int adc_clk)
{
	struct amldtvdemod_device_s *devp = (struct amldtvdemod_device_s *)demod->priv;
	union demod_cfg2 cfg2 = { .d32 = 0, };
	int sts_pll = 0;
#ifdef CONFIG_AMLOGIC_MEDIA_ADC
	struct dfe_adcpll_para ddemod_pll;
#endif
	demod_power_switch(PWR_ON);

#ifdef CONFIG_AMLOGIC_MEDIA_ADC
	memset(&ddemod_pll, 0, sizeof(ddemod_pll));
	ddemod_pll.delsys = demod->demod_status.delsys;
	if (demod->demod_status.delsys == SYS_DVBC_ANNEX_A && !devp->blind_scan_stop) {
		//mode[0:7]=2 is new dvbc_blind_scan mode, mode[8:15] DVB-C channel;
		ddemod_pll.mode = 2;
		if (demod->dvbc_sel == 1)
			//[8:15], 0: use DVB-C ch0, 1: use ch1;
			ddemod_pll.mode |= 1 << 8;
	}

	adc_set_ddemod_default(&ddemod_pll);
#endif
	demod_set_demod_default();
#ifdef CONFIG_AMLOGIC_MEDIA_ADC
	ddemod_pll.adc_clk = adc_clk;
	if (tuner_find_by_name(&demod->frontend, "av2018"))
		ddemod_pll.pga_gain = 2;
	else
		ddemod_pll.pga_gain = 0;

	sts_pll = adc_set_pll_cntl(1, ADC_DTV_DEMOD, &ddemod_pll);
#endif
	if (sts_pll < 0) {
		/*set pll fail*/
		PR_ERR("%s: set pll default fail %d !\n", __func__, sts_pll);

		return sts_pll;
	}

	demod_set_mode_ts(demod, demod->demod_status.delsys);
	cfg2.b.biasgen_en = 1;
	cfg2.b.en_adc = 1;
	demod_top_write_reg(DEMOD_TOP_REG8, cfg2.d32);

	PR_ERR("%s:done!\n", __func__);

	return sts_pll;
}

void dtmb_write_reg(unsigned int reg_addr, unsigned int reg_data)
{
	if (!get_dtvpll_init_flag())
		return;

	demod_mutex_lock();
	/* printk("[demod][write]%x,data is %x\n",(addr),data);*/

	writel(reg_data, gbase_dtmb() + (reg_addr << 2));

	demod_mutex_unlock();
}

unsigned int dtmb_read_reg(unsigned int reg_addr)
{
	unsigned int tmp;

	if (!get_dtvpll_init_flag())
		return 0;

	/*demod_mutex_lock();*/

	tmp = readl(gbase_dtmb() + (reg_addr << 2));

	/*demod_mutex_unlock();*/

	return tmp;
}

void dtmb_write_reg_bits(u32 addr, const u32 data, const u32 start, const u32 len)
{
	unsigned int val;

	if (!get_dtvpll_init_flag())
		return;

	val = dtmb_read_reg(addr);
	val &= ~(((1L << (len)) - 1) << (start));
	val |= (((data) & ((1L << (len)) - 1)) << (start));
	dtmb_write_reg(addr, val);
}

void dvbt_isdbt_wr_reg(unsigned int addr, unsigned int data)
{
	if (!get_dtvpll_init_flag())
		return;

	/*demod_mutex_lock();*/
	writel(data, gbase_dvbt_isdbt() + addr);
	/*demod_mutex_unlock();*/
}

void dvbt_isdbt_wr_reg_new(unsigned int addr, unsigned int data)
{
	if (!get_dtvpll_init_flag())
		return;

	/*demod_mutex_lock();*/

	writel(data, gbase_dvbt_isdbt() + (addr << 2));

	/*demod_mutex_unlock();*/
}

void dvbt_isdbt_wr_bits_new(u32 reg_addr, const u32 reg_data,
		    const u32 start, const u32 len)
{
	unsigned int val;

	if (!get_dtvpll_init_flag())
		return;

	/*demod_mutex_lock();*/
	val = readl(gbase_dvbt_isdbt() + (reg_addr << 2));
	val &= ~(((1L << (len)) - 1) << (start));
	val |= (((reg_data) & ((1L << (len)) - 1)) << (start));
	writel(val, gbase_dvbt_isdbt() + (reg_addr << 2));
	/*demod_mutex_unlock();*/
}

unsigned int dvbt_isdbt_rd_reg(unsigned int addr)
{
	unsigned int tmp = 0;

	if (!get_dtvpll_init_flag())
		return 0;

	/*demod_mutex_lock();*/

	tmp = readl(gbase_dvbt_isdbt() + addr);

	/*demod_mutex_unlock();*/

	return tmp;
}

unsigned int dvbt_isdbt_rd_reg_new(unsigned int addr)
{
	unsigned int tmp;

	if (!get_dtvpll_init_flag())
		return 0;

	demod_mutex_lock();

	tmp = readl(gbase_dvbt_isdbt() + (addr << 2));

	demod_mutex_unlock();

	return tmp;
}

void dvbt_t2_wrb(unsigned int addr, unsigned char data)
{
	struct amldtvdemod_device_s *devp = dtvdemod_get_dev();

	if (!get_dtvpll_init_flag() || unlikely(!devp))
		return;

	demod_mutex_lock();
	__raw_writeb(data, gbase_dvbt_t2() + addr);

	if (devp->print_on)
		PR_INFO("t2 wrB 0x%x=0x%x\n", addr, data);

	demod_mutex_unlock();
}

void dvbt_t2_wr_byte_bits(u32 addr, const u32 data, const u32 start, const u32 len)
{
	unsigned int val;

	if (!get_dtvpll_init_flag())
		return;

	val = dvbt_t2_rdb(addr);
	val &= ~(((1L << (len)) - 1) << (start));
	val |= (((data) & ((1L << (len)) - 1)) << (start));
	dvbt_t2_wrb(addr, val);
}

void dvbt_t2_write_w(unsigned int addr, unsigned int data)
{
	struct amldtvdemod_device_s *devp = dtvdemod_get_dev();

	if (!get_dtvpll_init_flag() || unlikely(!devp))
		return;

	writel(data, gbase_dvbt_t2() + addr);

	if (devp->print_on)
		PR_INFO("t2 wrW 0x%x=0x%x\n", addr, data);
}

void dvbt_t2_wr_word_bits(u32 addr, const u32 data, const u32 start, const u32 len)
{
	unsigned int val;

	if (!get_dtvpll_init_flag())
		return;

	/*demod_mutex_lock();*/
	val = readl(gbase_dvbt_t2() + addr);
	val &= ~(((1L << (len)) - 1) << (start));
	val |= (((data) & ((1L << (len)) - 1)) << (start));
	writel(val, gbase_dvbt_t2() + addr);
	/*demod_mutex_unlock();*/
}

unsigned int dvbt_t2_read_w(unsigned int addr)
{
	unsigned int tmp;

	if (!get_dtvpll_init_flag())
		return 0;

	tmp = readl(gbase_dvbt_t2() + addr);

	return tmp;
}

char dvbt_t2_rdb(unsigned int addr)
{
	char tmp = 0;

	if (!get_dtvpll_init_flag())
		return 0;

	demod_mutex_lock();
	tmp = __raw_readb(gbase_dvbt_t2() + addr);
	demod_mutex_unlock();

	return tmp;
}

/* only for T5D T2 use, have to set top 0x10 = 0x97 before any access */
void riscv_ctl_write_reg(unsigned int addr, unsigned int data)
{
	dvbt_t2_write_w(addr, data);
}

unsigned int riscv_ctl_read_reg(unsigned int addr)
{
	return dvbt_t2_read_w(addr);
}

void dvbs_wr_byte(unsigned int addr, unsigned char data)
{
	if (!get_dtvpll_init_flag())
		return;

	/*demod_mutex_lock();*/

	__raw_writeb(data, gbase_dvbs() + addr);

	/*demod_mutex_unlock();*/
}

unsigned char dvbs_rd_byte(unsigned int addr)
{
	unsigned char tmp = 0;

	if (!get_dtvpll_init_flag())
		return 0;

	/*demod_mutex_lock();*/

	tmp = __raw_readb(gbase_dvbs() + addr);

	/*demod_mutex_unlock();*/

	return tmp;
}

void dvbs_write_bits(u32 reg_addr, const u32 reg_data,
		    const u32 start, const u32 len)
{
	unsigned int val;

	if (!get_dtvpll_init_flag())
		return;

	/*demod_mutex_lock();*/
	val =  dvbs_rd_byte(reg_addr);
	val &= ~(((1L << (len)) - 1) << (start));
	val |= (((reg_data) & ((1L << (len)) - 1)) << (start));
	dvbs_wr_byte(reg_addr, val);
	/*demod_mutex_unlock();*/
}

#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
void t3_revb_set_ambus_state(bool enable, bool is_t2)
{
	unsigned int reg = 0;

	/* DEMOD_TOP_CFG_REG_4: dvbt2_mode_sel reg.
	 * bit[15:0] == 0, other demod sel.
	 * bit[15:0] != 0, dvbt2 sel, bit[15:0] as dvbt2 paddr high 16bit.
	 */
	if (is_t2) {
		reg = demod_top_read_reg(DEMOD_TOP_CFG_REG_4);
		if (reg)
			demod_top_write_reg(DEMOD_TOP_CFG_REG_4, 0);

		PR_DBGL("%s: read DEMOD_TOP_CFG_REG_4(0x%x): 0x%x.\n",
				__func__, DEMOD_TOP_CFG_REG_4, reg);
	}

	/* TEST_BUS_VLD[0x36]: bit31: dc_lbrst.
	 * normal mode to accesses ddr, pull down dc_lbrst.
	 * single mode to accesses ddr, pull up dc_lbrst.
	 */
	front_write_bits(TEST_BUS_VLD, enable ? 1 : 0, 31, 1);

	PR_DBGL("%s: read TEST_BUS_VLD(0x%x): 0x%x.\n",
			__func__, TEST_BUS_VLD, front_read_reg(TEST_BUS_VLD));

	if (is_t2 && reg)
		demod_top_write_reg(DEMOD_TOP_CFG_REG_4, reg);
}

unsigned int t5w_read_ambus_reg(unsigned int addr)
{
	unsigned int val = 0;
	void __iomem *vaddr = NULL;

	if (!get_dtvpll_init_flag())
		return 0;

	vaddr = ioremap((0xffd00000 + (addr << 2)), 0x4);
	if (vaddr) {
		/*demod_mutex_lock();*/
		val = readl(vaddr);

		iounmap(vaddr);
	}

	return val;
}

void t5w_write_ambus_reg(u32 addr,
	const u32 data, const u32 start, const u32 len)
{
	unsigned int val = 0;
	void __iomem *vaddr = NULL;

	if (!get_dtvpll_init_flag())
		return;

	vaddr = ioremap((0xffd00000 + (addr << 2)), 0x4);
	if (vaddr) {
		/*demod_mutex_lock();*/
		val = readl(vaddr);
		val &= ~(((1L << (len)) - 1) << (start));
		val |= (((data) & ((1L << (len)) - 1)) << (start));
		writel(val, vaddr);

		iounmap(vaddr);
	}
}
#endif

void demod_init_mutex(void)
{
	mutex_init(&mp);
}

void demod_mutex_lock(void)
{
	mutex_lock(&mp);
}

void demod_mutex_unlock(void)
{
	mutex_unlock(&mp);
}
int demod_set_sys(struct aml_dtvdemod *demod, struct aml_demod_sys *demod_sys)
{
	int ret = 0;
	unsigned int clk_adc = 0, clk_dem = 0;
	int nco_rate = 0;
	struct amldtvdemod_device_s *devp = (struct amldtvdemod_device_s *)demod->priv;

	clk_adc = demod_sys->adc_clk;
	clk_dem = demod_sys->demod_clk;
	nco_rate = (clk_adc * 256) / clk_dem + 2;
	PR_DBG("%s: clk_adc is %d, clk_demod is %d.\n", __func__, clk_adc, clk_dem);
	ret = clocks_set_sys_defaults(demod, clk_adc);
	if (ret)
		return ret;

	/* set adc clk */
	ret = demod_set_adc_core_clk(clk_adc, clk_dem, &demod->demod_status);
	if (ret)
		return ret;

	switch (demod->demod_status.delsys) {
	case SYS_DTMB:
		if (cpu_after_eq(MESON_CPU_MAJOR_ID_TL1)) {
			demod_top_write_reg(DEMOD_TOP_REGC, 0x11);
			demod_top_write_reg(DEMOD_TOP_REGC, 0x10);
			usleep_range(1000, 1001);
			demod_top_write_reg(DEMOD_TOP_REGC, 0x11);
			front_write_bits(AFIFO_ADC, nco_rate,
					 AFIFO_NCO_RATE_BIT, AFIFO_NCO_RATE_WID);
			front_write_bits(AFIFO_ADC, 1, ADC_2S_COMPLEMENT_BIT,
					 ADC_2S_COMPLEMENT_WID);
			front_write_bits(TEST_BUS, 1, DC_ARB_EN_BIT, DC_ARB_EN_WID);
		} else {
			demod_top_write_reg(DEMOD_TOP_REGC, 0x8);
			PR_DBG("[open arbit]dtmb\n");
		}
		break;

	case SYS_ISDBT:
		if (is_meson_txlx_cpu()) {
			demod_top_write_reg(DEMOD_TOP_REGC, 0x8);
		} else if (devp->data->hw_ver >= DTVDEMOD_HW_T5D) {
			demod_top_write_reg(DEMOD_TOP_REGC, 0x11);
			demod_top_write_reg(DEMOD_TOP_REGC, 0x10);
			usleep_range(1000, 1001);
			demod_top_write_reg(DEMOD_TOP_REGC, 0x11);
			front_write_bits(AFIFO_ADC, nco_rate, AFIFO_NCO_RATE_BIT,
					 AFIFO_NCO_RATE_WID);
			front_write_bits(AFIFO_ADC, 1, ADC_2S_COMPLEMENT_BIT,
					 ADC_2S_COMPLEMENT_WID);
			front_write_reg(SFIFO_OUT_LENS, 1);
			front_write_bits(TEST_BUS, 1, DC_ARB_EN_BIT, DC_ARB_EN_WID);
		}
		PR_DBG("[open arbit]dvbt,txlx\n");
		break;

	case SYS_ATSC:
	case SYS_ATSCMH:
	case SYS_DVBC_ANNEX_B:
		if (cpu_after_eq(MESON_CPU_MAJOR_ID_TL1)) {
			if (devp->data->hw_ver == DTVDEMOD_HW_S4D ||
				devp->data->hw_ver == DTVDEMOD_HW_S1A) {
				demod_top_write_reg(DEMOD_TOP_REGC, 0x11);
				demod_top_write_reg(DEMOD_TOP_REGC, 0x10);
				usleep_range(1000, 2000);
				demod_top_write_reg(DEMOD_TOP_REGC, 0xcc0011);
				front_write_bits(0x6c, nco_rate,
					AFIFO_NCO_RATE_BIT,
					AFIFO_NCO_RATE_WID);
				/* s1a only one adc, i signal and q signal are reversed */
				/* set bit[1]:adc_swap to 1 */
				if (devp->data->hw_ver == DTVDEMOD_HW_S1A)
					demod_top_write_reg(DEMOD_TOP_CFG_REG_6, 2);
			} else {
				demod_top_write_reg(DEMOD_TOP_REGC, 0x11);
				demod_top_write_reg(DEMOD_TOP_REGC, 0x10);
				usleep_range(1000, 1001);
				demod_top_write_reg(DEMOD_TOP_REGC, 0x11);
				front_write_bits(AFIFO_ADC, nco_rate,
					AFIFO_NCO_RATE_BIT,
					AFIFO_NCO_RATE_WID);
			}
			front_write_bits(AFIFO_ADC, 1, ADC_2S_COMPLEMENT_BIT,
					 ADC_2S_COMPLEMENT_WID);
		}
		break;

	case SYS_DVBC_ANNEX_A:
	case SYS_DVBC_ANNEX_C:
		if (cpu_after_eq(MESON_CPU_MAJOR_ID_TL1)) {
			if (devp->data->hw_ver == DTVDEMOD_HW_S4 ||
				devp->data->hw_ver == DTVDEMOD_HW_S4D ||
				devp->data->hw_ver == DTVDEMOD_HW_S1A) {
				demod_top_write_reg(DEMOD_TOP_REGC, 0x11);
				demod_top_write_reg(DEMOD_TOP_REGC, 0x10);
				usleep_range(1000, 2000);

				//enable dvbc and dvbs mode for new dvbc_blind_scan mode
				if (!devp->blind_scan_stop)
					demod_top_write_reg(DEMOD_TOP_REGC,
							demod->dvbc_sel == 1 ? 0xaa0011 : 0x660011);
				else
					demod_top_write_reg(DEMOD_TOP_REGC, 0xcc0011);

				front_write_bits(AFIFO_ADC_S4D, nco_rate,
					AFIFO_NCO_RATE_BIT, AFIFO_NCO_RATE_WID);
				/* s1a only one adc, i signal and q signal are reversed */
				/* set bit[1]:adc_swap to 1 */
				if (devp->data->hw_ver == DTVDEMOD_HW_S1A)
					demod_top_write_reg(DEMOD_TOP_CFG_REG_6, 2);
			} else {
				demod_top_write_reg(DEMOD_TOP_REGC, 0x11);
				demod_top_write_reg(DEMOD_TOP_REGC, 0x10);
				usleep_range(1000, 2000);
				demod_top_write_reg(DEMOD_TOP_REGC, 0x11);
				front_write_bits(AFIFO_ADC, nco_rate, AFIFO_NCO_RATE_BIT,
						 AFIFO_NCO_RATE_WID);
			}

			//for new dvbc_blind_scan mode
			if (!devp->blind_scan_stop) {
				front_write_reg(0x6C, 0); //config afifo2
				front_write_reg(SFIFO_OUT_LENS, 0);
				front_write_reg(0x27, 0x03555555); //config ddc_bypass
			} else {
				front_write_reg(SFIFO_OUT_LENS, 0x05);
			}

			front_write_bits(AFIFO_ADC, 1, ADC_2S_COMPLEMENT_BIT,
					ADC_2S_COMPLEMENT_WID);
		}
		break;

	case SYS_DVBT:
	case SYS_DVBT2:
		break;

	case SYS_DVBS:
	case SYS_DVBS2:
		if (devp->data->hw_ver >= DTVDEMOD_HW_T5D) {
			nco_rate = 0x0;
			if (devp->data->hw_ver >= DTVDEMOD_HW_T3) {
				demod_top_write_reg(DEMOD_TOP_REGC, 0x80220011);
				demod_top_write_reg(DEMOD_TOP_REGC, 0x80220010);
				usleep_range(1000, 1001);
				demod_top_write_reg(DEMOD_TOP_REGC, 0x80220011);
			} else {
				demod_top_write_reg(DEMOD_TOP_REGC, 0x220011);
				demod_top_write_reg(DEMOD_TOP_REGC, 0x220010);
				usleep_range(1000, 1001);
				demod_top_write_reg(DEMOD_TOP_REGC, 0x220011);
			}
			if (devp->data->hw_ver == DTVDEMOD_HW_S4D ||
					devp->data->hw_ver == DTVDEMOD_HW_S1A)
				front_write_bits(AFIFO_ADC_S4D, nco_rate,
					AFIFO_NCO_RATE_BIT,
					AFIFO_NCO_RATE_WID);
			else
				front_write_bits(AFIFO_ADC, nco_rate,
					AFIFO_NCO_RATE_BIT,
					AFIFO_NCO_RATE_WID);
			front_write_reg(SFIFO_OUT_LENS, 0x0);
			front_write_reg(0x22, 0x7200a06);

			if (devp->data->hw_ver != DTVDEMOD_HW_S4)
				demod_top_write_reg(DEMOD_TOP_REG0, 0x0);
		}
		break;

	default:
		break;
	}

	PR_ERR("%s: %d done!\n", __func__, demod->demod_status.delsys);

	return 0;
}

/*TL1*/
void set_j83b_filter_reg_v4(struct aml_dtvdemod *demod)
{
	//j83_1
	qam_write_reg(demod, 0x40, 0x3F010201);//25M:0x36333c0d
	qam_write_reg(demod, 0x41, 0xA003A3B);//25M:0xa110d01
	qam_write_reg(demod, 0x42, 0xE1EE030E);//25M:0xf0e4ea7a
	qam_write_reg(demod, 0x43, 0x2601F2);//25M:0x3c0010
	qam_write_reg(demod, 0x44, 0x9B006B);//25M:0x7e0065

	//j83_2
	qam_write_reg(demod, 0x45, 0xb3a1905);
	qam_write_reg(demod, 0x46, 0x1c396e07);
	qam_write_reg(demod, 0x47, 0x3801cc08);
	qam_write_reg(demod, 0x48, 0x10800a2);

	qam_write_reg(demod, 0x49, 0x53b1f03);
	qam_write_reg(demod, 0x4a, 0x18377407);
	qam_write_reg(demod, 0x4b, 0x3401cf0b);
	qam_write_reg(demod, 0x4c, 0x10d00a1);
}

void demod_set_reg(struct aml_dtvdemod *demod, struct aml_demod_reg *demod_reg)
{
	if (fpga_version == 1) {
#if defined DEMOD_FPGA_VERSION
		fpga_write_reg(demod_reg->mode, demod_reg->addr,
				demod_reg->val);
#endif
	} else {
		switch (demod_reg->mode) {
		case REG_MODE_CFG:
			demod_reg->addr = demod_reg->addr * 4
				+ gphybase_demod() + DEMOD_CFG_BASE;
			break;
		case REG_MODE_BASE:
			demod_reg->addr = demod_reg->addr + gphybase_demod();
			break;
		default:
			break;
		}

		switch (demod_reg->mode) {
		case REG_MODE_DTMB:
			dtmb_write_reg(demod_reg->addr, demod_reg->val);
			break;

		case REG_MODE_DVBT_ISDBT:
			if (demod_reg->access_mode == ACCESS_WORD)
				dvbt_isdbt_wr_reg_new(demod_reg->addr, demod_reg->val);
			else if (demod_reg->access_mode == ACCESS_BITS)
				dvbt_isdbt_wr_bits_new(demod_reg->addr, demod_reg->val,
						    demod_reg->start_bit, demod_reg->bit_width);
			break;

		case REG_MODE_DVBT_T2:
			if (demod_reg->access_mode == ACCESS_BYTE)
				dvbt_t2_wrb(demod_reg->addr, demod_reg->val);
			break;
#ifdef AML_DEMOD_SUPPORT_ATSC
		case REG_MODE_ATSC:
			if (cpu_after_eq(MESON_CPU_MAJOR_ID_TL1))
				atsc_write_reg_v4(demod_reg->addr, demod_reg->val);
			else
				atsc_write_reg(demod_reg->addr, demod_reg->val);
			break;
#endif
		case REG_MODE_OTHERS:
			demod_set_cbus_reg(demod_reg->val, demod_reg->addr);
			break;

		case REG_MODE_DVBC_J83B:
			qam_write_reg(demod, demod_reg->addr, demod_reg->val);
			break;

		case REG_MODE_FRONT:
			if (demod_reg->access_mode == ACCESS_BITS)
				front_write_bits(demod_reg->addr, demod_reg->val,
						 demod_reg->start_bit, demod_reg->bit_width);
			else if (demod_reg->access_mode == ACCESS_WORD)
				front_write_reg(demod_reg->addr, demod_reg->val);
			break;

		case REG_MODE_TOP:
			if (demod_reg->access_mode == ACCESS_BITS)
				demod_top_write_bits(demod_reg->addr, demod_reg->val,
						 demod_reg->start_bit, demod_reg->bit_width);
			else if (demod_reg->access_mode == ACCESS_WORD)
				demod_top_write_reg(demod_reg->addr, demod_reg->val);
			break;

		case REG_MODE_COLLECT_DATA:
			apb_write_reg_collect(demod_reg->addr, demod_reg->val);
			break;

		default:
			demod_set_demod_reg(demod_reg->val, demod_reg->addr);
			break;
		}

	}
}

void demod_get_reg(struct aml_dtvdemod *demod, struct aml_demod_reg *demod_reg)
{
	if (fpga_version == 1) {
		#if defined DEMOD_FPGA_VERSION
		demod_reg->val = fpga_read_reg(demod_reg->mode,
			demod_reg->addr);
		#endif
	} else {
		if (demod_reg->mode == REG_MODE_CFG)
			demod_reg->addr = demod_reg->addr * 4
				+ gphybase_demod() + DEMOD_CFG_BASE;
		else if (demod_reg->mode == REG_MODE_BASE)
			demod_reg->addr = demod_reg->addr + gphybase_demod();

		switch (demod_reg->mode) {
		case REG_MODE_DTMB:
			demod_reg->val = dtmb_read_reg(demod_reg->addr);
			break;

		case REG_MODE_DVBT_ISDBT:
			demod_reg->val = dvbt_isdbt_rd_reg_new(demod_reg->addr);
			break;

		case REG_MODE_DVBT_T2:
			if (demod_reg->access_mode == ACCESS_BYTE)
				demod_reg->val = dvbt_t2_rdb(demod_reg->addr);

			break;
#ifdef AML_DEMOD_SUPPORT_ATSC
		case REG_MODE_ATSC:
			if (cpu_after_eq(MESON_CPU_MAJOR_ID_TL1))
				demod_reg->val = atsc_read_reg_v4(demod_reg->addr);
			else
				demod_reg->val = atsc_read_reg(demod_reg->addr);
			break;
#endif
		case REG_MODE_DVBC_J83B:
			demod_reg->val = qam_read_reg(demod, demod_reg->addr);
			break;

		case REG_MODE_FRONT:
			demod_reg->val = front_read_reg(demod_reg->addr);
			break;

		case REG_MODE_TOP:
			demod_reg->val = demod_top_read_reg(demod_reg->addr);
			break;

		case REG_MODE_OTHERS:
			demod_reg->val = demod_read_cbus_reg(demod_reg->addr);
			break;

		case REG_MODE_COLLECT_DATA:
			demod_reg->val = apb_read_reg_collect(demod_reg->addr);
			break;

		default:
			demod_reg->val = demod_read_demod_reg(demod_reg->addr);
			break;
		}
	}
}

void apb_write_reg_collect(unsigned int addr, unsigned int data)
{
	writel(data, ((void __iomem *)(phys_to_virt(addr))));
/* *(volatile unsigned int*)addr = data; */
}

unsigned long apb_read_reg_collect(unsigned long addr)
{
	unsigned long tmp;

	tmp = readl((void __iomem *)(phys_to_virt(addr)));

	return tmp & 0xffffffff;
}

void apb_write_reg(unsigned int addr, unsigned int data)
{
	demod_set_demod_reg(data, addr);
}

unsigned long apb_read_reg_high(unsigned long addr)
{
	return 0;
}

unsigned long apb_read_reg(unsigned long addr)
{
	return demod_read_demod_reg(addr);
}

#if 0
void monitor_isdbt(void)
{
	int SNR;
	int SNR_SP = 500;
	int SNR_TPS = 0;
	int SNR_CP = 0;
	int timeStamp = 0;
	int SFO_residual = 0;
	int SFO_esti = 0;
	int FCFO_esti = 0;
	int FCFO_residual = 0;
	int AGC_Gain = 0;
	int RF_AGC = 0;
	int Signal_power = 0;
	int FECFlag = 0;
	int EQ_seg_ratio = 0;
	int tps_0 = 0;
	int tps_1 = 0;
	int tps_2 = 0;

	int time_stamp;
	int SFO;
	int FCFO;
	int timing_adj;
	int RS_CorrectNum;

	int cnt;
	int tmpAGCGain;

	tmpAGCGain = 0;
	cnt = 0;

/* app_apb_write_reg(0x8, app_apb_read_reg(0x8) & ~(1 << 17));*/
/* // TPS symbol index update : active high */
	time_stamp = app_apb_read_reg(0x07) & 0xffff;
	SNR = app_apb_read_reg(0x0a);
	FECFlag = (app_apb_read_reg(0x00) >> 11) & 0x3;
	SFO = app_apb_read_reg(0x47) & 0xfff;
	SFO_esti = app_apb_read_reg(0x60) & 0xfff;
	FCFO_esti = (app_apb_read_reg(0x60) >> 11) & 0xfff;
	FCFO = (app_apb_read_reg(0x26)) & 0xffffff;
	RF_AGC = app_apb_read_reg(0x0c) & 0x1fff;
	timing_adj = app_apb_read_reg(0x6f) & 0x1fff;
	RS_CorrectNum = app_apb_read_reg(0xc1) & 0xfffff;
	Signal_power = (app_apb_read_reg(0x1b)) & 0x1ff;
	EQ_seg_ratio = app_apb_read_reg(0x6e) & 0x3ffff;
	tps_0 = app_apb_read_reg(0x64);
	tps_1 = app_apb_read_reg(0x65);
	tps_2 = app_apb_read_reg(0x66) & 0xf;

	timeStamp = (time_stamp >> 8) * 68 + (time_stamp & 0x7f);
	SFO_residual = (SFO > 0x7ff) ? (SFO - 0x1000) : SFO;
	FCFO_residual = (FCFO > 0x7fffff) ? (FCFO - 0x1000000) : FCFO;
	/* RF_AGC          = (RF_AGC>0x3ff)? (RF_AGC - 0x800): RF_AGC; */
	FCFO_esti = (FCFO_esti > 0x7ff) ? (FCFO_esti - 0x1000) : FCFO_esti;
	SNR_CP = (SNR) & 0x3ff;
	SNR_TPS = (SNR >> 10) & 0x3ff;
	SNR_SP = (SNR >> 20) & 0x3ff;
	SNR_SP = (SNR_SP > 0x1ff) ? SNR_SP - 0x400 : SNR_SP;
	SNR_TPS = (SNR_TPS > 0x1ff) ? SNR_TPS - 0x400 : SNR_TPS;
	SNR_CP = (SNR_CP > 0x1ff) ? SNR_CP - 0x400 : SNR_CP;
	AGC_Gain = tmpAGCGain >> 4;
	tmpAGCGain = (AGC_Gain > 0x3ff) ? AGC_Gain - 0x800 : AGC_Gain;
	timing_adj = (timing_adj > 0xfff) ? timing_adj - 0x2000 : timing_adj;
	EQ_seg_ratio =
	    (EQ_seg_ratio > 0x1ffff) ? EQ_seg_ratio - 0x40000 : EQ_seg_ratio;

	PR_DBG
	    ("T %4x SP %3d TPS %3d CP %3d EQS %8x RSC %4d",
	     app_apb_read_reg(0xbf)
	     , SNR_SP, SNR_TPS, SNR_CP
/* ,EQ_seg_ratio */
	     , app_apb_read_reg(0x62)
	     , RS_CorrectNum);
	PR_DBG
	    ("SFO %4d FCFO %4d Vit %4x Timing %3d SigP %3x",
	    SFO_residual, FCFO_residual, RF_AGC, timing_adj,
	     Signal_power);
	PR_DBG
	    ("FEC %x RSErr %8x ReSyn %x tps %03x%08x",
	    FECFlag, app_apb_read_reg(0x0b)
	     , (app_apb_read_reg(0xc0) >> 20) & 0xff,
	     app_apb_read_reg(0x05) & 0xfff, app_apb_read_reg(0x04)
	    );
	PR_DBG("\n");
}
#endif

/*dvbc_write_reg -> apb_write_reg in dvbc_func*/
/*dvbc_read_reg -> apb_read_reg in dvbc_func*/
#if 0
void dvbc_write_reg(unsigned int addr, unsigned int data)
{
	demod_set_demod_reg(data, ddemod_reg_base + addr);
}
unsigned int dvbc_read_reg(unsigned int addr)
{
	return demod_read_demod_reg(ddemod_reg_base + addr);
}
#else
void dvbc_write_reg(unsigned int addr, unsigned int data)
{
	/*demod_mutex_lock();*/
	/* printk("[demod][write]%x,data is %x\n",(addr),data);*/

	writel(data, gbase_dvbc() + addr);

	/*demod_mutex_unlock();*/
}
unsigned int dvbc_read_reg(unsigned int addr)
{
	unsigned int tmp;

	/*demod_mutex_lock();*/

	tmp = readl(gbase_dvbc() + addr);

	/*demod_mutex_unlock();*/

	return tmp;
}
#endif

void demod_top_write_reg(unsigned int addr, unsigned int data)
{
	struct amldtvdemod_device_s *devp = dtvdemod_get_dev();

	if (unlikely(!devp))
		return;

	demod_mutex_lock();
	if (cpu_after_eq(MESON_CPU_MAJOR_ID_TL1))
		writel(data, gbase_demod() + (addr << 2));
	else
		writel(data, gbase_demod() + addr);

	demod_mutex_unlock();
	if (devp->print_on)
		PR_INFO("top wrW 0x%x=0x%x\n", addr, data);
}

void demod_top_write_bits(u32 reg_addr, const u32 reg_data, const u32 start, const u32 len)
{
	unsigned int val;
	struct amldtvdemod_device_s *devp = dtvdemod_get_dev();

	if (!get_dtvpll_init_flag() || unlikely(!devp))
		return;

	demod_mutex_lock();
	val = readl(gbase_demod() + (reg_addr << 2));
	val &= ~(((1L << (len)) - 1) << (start));
	val |= (((reg_data) & ((1L << (len)) - 1)) << (start));
	writel(val, gbase_demod() + (reg_addr << 2));

	if (devp->print_on)
		PR_INFO("top wrBit 0x%x=0x%x,s:%d,l:%d\n", reg_addr, reg_data, start, len);
	demod_mutex_unlock();
}

unsigned int demod_top_read_reg(unsigned int addr)
{
	unsigned int tmp;

	demod_mutex_lock();

	if (cpu_after_eq(MESON_CPU_MAJOR_ID_TL1))
		tmp = readl(gbase_demod() + (addr << 2));
	else
		tmp = readl(gbase_demod() + addr);

	demod_mutex_unlock();

	return tmp;
}

/*TL1*/
void front_write_reg(unsigned int addr, unsigned int data)
{
	struct amldtvdemod_device_s *devp = dtvdemod_get_dev();

	if (!get_dtvpll_init_flag() || unlikely(!devp))
		return;

	demod_mutex_lock();

	writel(data, gbase_front() + (addr << 2));

	if (devp->print_on)
		PR_INFO("front wrW 0x%x=0x%x\n", addr, data);

	demod_mutex_unlock();
}

void front_write_bits(u32 reg_addr, const u32 reg_data, const u32 start, const u32 len)
{
	unsigned int val;
	struct amldtvdemod_device_s *devp = dtvdemod_get_dev();

	if (!get_dtvpll_init_flag() || unlikely(!devp))
		return;

	demod_mutex_lock();
	val = readl(gbase_front() + (reg_addr << 2));
	val &= ~(((1L << (len)) - 1) << (start));
	val |= (((reg_data) & ((1L << (len)) - 1)) << (start));
	writel(val, gbase_front() + (reg_addr << 2));

	if (devp->print_on)
		PR_INFO("front wrBit 0x%x=0x%x, s:%d,l:%d\n", reg_addr, reg_data, start, len);
	demod_mutex_unlock();

}

unsigned int front_read_reg(unsigned int addr)
{
	unsigned int tmp;

	if (!get_dtvpll_init_flag())
		return 0;

	demod_mutex_lock();

	tmp = readl(gbase_front() + (addr << 2));

	demod_mutex_unlock();

	return tmp;

}

void  isdbt_write_reg_v4(unsigned int addr, unsigned int data)
{
	demod_mutex_lock();
	/* printk("[demod][write]%x,data is %x\n",(addr),data);*/

	writel(data, gbase_isdbt() + addr);

	demod_mutex_unlock();
}

unsigned int isdbt_read_reg_v4(unsigned int addr)
{
	unsigned int tmp;

	demod_mutex_lock();

	tmp = readl(gbase_isdbt() + addr);

	demod_mutex_unlock();

	return tmp;

}

/*dvbc v3:*/
void qam_write_reg(struct aml_dtvdemod *demod,
		unsigned int reg_addr, unsigned int reg_data)
{
	struct amldtvdemod_device_s *devp = (struct amldtvdemod_device_s *)demod->priv;

	if (!get_dtvpll_init_flag())
		return;

	if (devp && devp->stop_reg_wr)
		return;

	demod_mutex_lock();
	if (demod->dvbc_sel)
		writel(reg_data, gbase_dvbc_2() + (reg_addr << 2));
	else
		writel(reg_data, gbase_dvbc() + (reg_addr << 2));
	demod_mutex_unlock();
}

unsigned int qam_read_reg(struct aml_dtvdemod *demod, unsigned int reg_addr)
{
	unsigned int tmp;

	if (!get_dtvpll_init_flag())
		return 0;

	demod_mutex_lock();
	if (demod->dvbc_sel)
		tmp = readl(gbase_dvbc_2() + (reg_addr << 2));
	else
		tmp = readl(gbase_dvbc() + (reg_addr << 2));
	demod_mutex_unlock();

	return tmp;
}

void qam_write_bits(struct aml_dtvdemod *demod,
		u32 reg_addr, const u32 reg_data,
		const u32 start, const u32 len)
{
	unsigned int val;
	struct amldtvdemod_device_s *devp = (struct amldtvdemod_device_s *)demod->priv;

	if (!get_dtvpll_init_flag())
		return;

	if (devp && devp->stop_reg_wr)
		return;

	demod_mutex_lock();

	if (demod->dvbc_sel)
		val = readl(gbase_dvbc_2() + (reg_addr << 2));
	else
		val = readl(gbase_dvbc() + (reg_addr << 2));

	val &= ~(((1L << (len)) - 1) << (start));
	val |= (((reg_data) & ((1L << (len)) - 1)) << (start));

	if (demod->dvbc_sel)
		writel(val, gbase_dvbc_2() + (reg_addr << 2));
	else
		writel(val, gbase_dvbc() + (reg_addr << 2));

	demod_mutex_unlock();
}

int dd_hiu_reg_write(unsigned int reg, unsigned int val)
{
	demod_mutex_lock();

	writel(val, gbase_iohiu() + (reg << 2));

	demod_mutex_unlock();

	return 0;
}

unsigned int dd_hiu_reg_read(unsigned int addr)
{
	unsigned int tmp;

	demod_mutex_lock();

	tmp = readl(gbase_iohiu() + (addr << 2));

	demod_mutex_unlock();

	return tmp;
}

int reset_reg_write(unsigned int reg, unsigned int val)
{
	demod_mutex_lock();

	writel(val, gbase_reset() + reg);

	demod_mutex_unlock();

	return 0;
}
unsigned int reset_reg_read(unsigned int addr)
{
	unsigned int tmp;

	demod_mutex_lock();

	tmp = readl(gbase_reset() + addr);

	demod_mutex_unlock();

	return tmp;
}

void dtvdemod_dmc_reg_write(unsigned int reg, unsigned int val)
{
	writel(val, gbase_dmc() + reg);
}

unsigned int dtvdemod_dmc_reg_read(unsigned int addr)
{
	unsigned int tmp;

	tmp = readl(gbase_dmc() + addr);

	return tmp;
}

void dtvdemod_ddr_reg_write(unsigned int reg, unsigned int val)
{
	writel(val, gbase_ddr() + reg);
}

unsigned int dtvdemod_ddr_reg_read(unsigned int addr)
{
	unsigned int tmp;

	tmp = readl(gbase_ddr() + addr);

	return tmp;
}

void demod_set_demod_default(void)
{
	if (cpu_after_eq(MESON_CPU_MAJOR_ID_TL1))
		return;

	demod_top_write_reg(DEMOD_TOP_REG0, DEMOD_REG0_VALUE);
	demod_top_write_reg(DEMOD_TOP_REG4, DEMOD_REG4_VALUE);
	demod_top_write_reg(DEMOD_TOP_REG8, DEMOD_REG8_VALUE);
}

int timer_set_max(struct aml_dtvdemod *demod,
		enum ddemod_timer_s tmid, unsigned int max_val)
{
	demod->gtimer[tmid].max = max_val;

	return 0;
}

int timer_begain(struct aml_dtvdemod *demod, enum ddemod_timer_s tmid)
{
	demod->gtimer[tmid].start = jiffies_to_msecs(jiffies);
	demod->gtimer[tmid].enable = 1;

	PR_TIME("st %d=%d\n", tmid, (int)demod->gtimer[tmid].start);
	return 0;
}

int timer_disable(struct aml_dtvdemod *demod, enum ddemod_timer_s tmid)
{
	demod->gtimer[tmid].enable = 0;

	return 0;
}

int timer_is_en(struct aml_dtvdemod *demod, enum ddemod_timer_s tmid)
{
	return demod->gtimer[tmid].enable;
}

int timer_not_enough(struct aml_dtvdemod *demod, enum ddemod_timer_s tmid)
{
	int ret = 0;
	unsigned int time;

	if (demod->gtimer[tmid].enable) {
		time = jiffies_to_msecs(jiffies);
		if ((time - demod->gtimer[tmid].start) < demod->gtimer[tmid].max)
			ret = 1;
	}
	return ret;
}

int timer_is_enough(struct aml_dtvdemod *demod, enum ddemod_timer_s tmid)
{
	int ret = 0;
	unsigned int time;

	/*Signal stability takes 200ms */
	if (demod->gtimer[tmid].enable) {
		time = jiffies_to_msecs(jiffies);
		if ((time - demod->gtimer[tmid].start) >= demod->gtimer[tmid].max) {
			PR_TIME("now=%d\n", (int)time);
			ret = 1;
		}
	}

	return ret;
}

int timer_tuner_not_enough(struct aml_dtvdemod *demod)
{
	int ret = 0;
	unsigned int time;
	enum ddemod_timer_s tmid;

	tmid = D_TIMER_DETECT;

	/*Signal stability takes 200ms */
	if (demod->gtimer[tmid].enable) {
		time = jiffies_to_msecs(jiffies);
		if ((time - demod->gtimer[tmid].start) < 200) {
			PR_TIME("nowt=%d\n", (int)time);
			ret = 1;
		}
	}

	return ret;
}

int amdemod_qam(enum fe_modulation qam)
{
	switch (qam) {
	case QAM_16:
		return QAM_MODE_16;
	case QAM_32:
		return QAM_MODE_32;
	case QAM_64:
		return QAM_MODE_64;
	case QAM_128:
		return QAM_MODE_128;
	case QAM_256:
		return QAM_MODE_256;
	case QAM_AUTO:
		return QAM_MODE_AUTO;
	default:
		return QAM_MODE_64;
	}

	return QAM_MODE_64;
}

enum fe_modulation amdemod_qam_fe(enum qam_md_e qam)
{
	switch (qam) {
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

const char *qam_name[] = {
	"QAM_16",
	"QAM_32",
	"QAM_64",
	"QAM_128",
	"QAM_256",
	"QAM_UNDEF"
};

const char *get_qam_name(enum qam_md_e qam)
{
	if (qam >= QAM_MODE_16 && qam <= QAM_MODE_256)
		return qam_name[qam];
	else
		return qam_name[5];
}

void real_para_clear(struct aml_demod_para_real *para)
{
	para->modulation = -1;
	para->coderate = -1;
	para->symbol = 0;
	para->snr = 0;
	para->plp_num = 0;
	para->fef_info = 0;
	para->tps_cell_id = 0;
	para->ber = 0;
}

enum fe_bandwidth dtvdemod_convert_bandwidth(unsigned int input)
{
	enum fe_bandwidth output = BANDWIDTH_AUTO;

	switch (input) {
	case 10000000:
		output = BANDWIDTH_10_MHZ;
		break;

	case 8000000:
		output = BANDWIDTH_8_MHZ;
		break;

	case 7000000:
		output = BANDWIDTH_7_MHZ;
		break;

	case 6000000:
		output = BANDWIDTH_6_MHZ;
		break;

	case 5000000:
		output = BANDWIDTH_5_MHZ;
		break;

	case 1712000:
		output = BANDWIDTH_1_712_MHZ;
		break;

	case 0:
		output = BANDWIDTH_AUTO;
		break;
	}

	return output;
}

int is_s1a_dvbs_disabled(void)
{
	int ret = 0;
	unsigned int val;
	void __iomem *reg_addr;

	reg_addr = ioremap(OTP_LIC13, sizeof(unsigned int));
	if (!reg_addr) {
		PR_ERR("%s ioremap fail !!\n", __func__);
		return ret;
	}
	val = readl(reg_addr);
	/* bit 12: FEAT_DISABLE_DVB_C */
	/* ret = 1 & (val >> 12); */

	/* bit 13: FEAT_DISABLE_DVB_S2*/
	ret = 1 & (val >> 13);

	iounmap(reg_addr);
	PR_INFO("%s: val 0x%x, ret %d .\n", __func__, val, ret);

	return ret;
}

#if defined AML_DEMOD_SUPPORT_DVBT || defined AML_DEMOD_SUPPORT_ISDBT
void calculate_cordic_para(void)
{
	dvbt_isdbt_wr_reg(0x0c, 0x00000040);
}

static int coef[] = {
	0xf900, 0xfe00, 0x0000, 0x0000, 0x0100, 0x0100, 0x0000, 0x0000,
	0xfd00, 0xf700, 0x0000, 0x0000, 0x4c00, 0x0000, 0x0000, 0x0000,
	0x2200, 0x0c00, 0x0000, 0x0000, 0xf700, 0xf700, 0x0000, 0x0000,
	0x0300, 0x0900, 0x0000, 0x0000, 0x0600, 0x0600, 0x0000, 0x0000,
	0xfc00, 0xf300, 0x0000, 0x0000, 0x2e00, 0x0000, 0x0000, 0x0000,
	0x3900, 0x1300, 0x0000, 0x0000, 0xfa00, 0xfa00, 0x0000, 0x0000,
	0x0100, 0x0200, 0x0000, 0x0000, 0xf600, 0x0000, 0x0000, 0x0000,
	0x0700, 0x0700, 0x0000, 0x0000, 0xfe00, 0xfb00, 0x0000, 0x0000,
	0x0900, 0x0000, 0x0000, 0x0000, 0x3200, 0x1100, 0x0000, 0x0000,
	0x0400, 0x0400, 0x0000, 0x0000, 0xfe00, 0xfb00, 0x0000, 0x0000,
	0x0e00, 0x0000, 0x0000, 0x0000, 0xfb00, 0xfb00, 0x0000, 0x0000,
	0x0100, 0x0200, 0x0000, 0x0000, 0xf400, 0x0000, 0x0000, 0x0000,
	0x3900, 0x1300, 0x0000, 0x0000, 0x1700, 0x1700, 0x0000, 0x0000,
	0xfc00, 0xf300, 0x0000, 0x0000, 0x0c00, 0x0000, 0x0000, 0x0000,
	0x0300, 0x0900, 0x0000, 0x0000, 0xee00, 0x0000, 0x0000, 0x0000,
	0x2200, 0x0c00, 0x0000, 0x0000, 0x2600, 0x2600, 0x0000, 0x0000,
	0xfd00, 0xf700, 0x0000, 0x0000, 0x0200, 0x0000, 0x0000, 0x0000,
	0xf900, 0xfe00, 0x0000, 0x0000, 0x0400, 0x0b00, 0x0000, 0x0000,
	0xf900, 0x0000, 0x0000, 0x0000, 0x0700, 0x0200, 0x0000, 0x0000,
	0x2100, 0x2100, 0x0000, 0x0000, 0x0200, 0x0700, 0x0000, 0x0000,
	0xf900, 0x0000, 0x0000, 0x0000, 0x0b00, 0x0400, 0x0000, 0x0000,
	0xfe00, 0xf900, 0x0000, 0x0000, 0x0200, 0x0000, 0x0000, 0x0000,
	0xf700, 0xfd00, 0x0000, 0x0000, 0x2600, 0x2600, 0x0000, 0x0000,
	0x0c00, 0x2200, 0x0000, 0x0000, 0xee00, 0x0000, 0x0000, 0x0000,
	0x0900, 0x0300, 0x0000, 0x0000, 0x0c00, 0x0000, 0x0000, 0x0000,
	0xf300, 0xfc00, 0x0000, 0x0000, 0x1700, 0x1700, 0x0000, 0x0000,
	0x1300, 0x3900, 0x0000, 0x0000, 0xf400, 0x0000, 0x0000, 0x0000,
	0x0200, 0x0100, 0x0000, 0x0000, 0xfb00, 0xfb00, 0x0000, 0x0000,
	0x0e00, 0x0000, 0x0000, 0x0000, 0xfb00, 0xfe00, 0x0000, 0x0000,
	0x0400, 0x0400, 0x0000, 0x0000, 0x1100, 0x3200, 0x0000, 0x0000,
	0x0900, 0x0000, 0x0000, 0x0000, 0xfb00, 0xfe00, 0x0000, 0x0000,
	0x0700, 0x0700, 0x0000, 0x0000, 0xf600, 0x0000, 0x0000, 0x0000,
	0x0200, 0x0100, 0x0000, 0x0000, 0xfa00, 0xfa00, 0x0000, 0x0000,
	0x1300, 0x3900, 0x0000, 0x0000, 0x2e00, 0x0000, 0x0000, 0x0000,
	0xf300, 0xfc00, 0x0000, 0x0000, 0x0600, 0x0600, 0x0000, 0x0000,
	0x0900, 0x0300, 0x0000, 0x0000, 0xf700, 0xf700, 0x0000, 0x0000,
	0x0c00, 0x2200, 0x0000, 0x0000, 0x4c00, 0x0000, 0x0000, 0x0000,
	0xf700, 0xfd00, 0x0000, 0x0000, 0x0100, 0x0100, 0x0000, 0x0000,
	0xfe00, 0xf900, 0x0000, 0x0000, 0x0b00, 0x0400, 0x0000, 0x0000,
	0xfc00, 0xfc00, 0x0000, 0x0000, 0x0200, 0x0700, 0x0000, 0x0000,
	0x4200, 0x0000, 0x0000, 0x0000, 0x0700, 0x0200, 0x0000, 0x0000,
	0xfc00, 0xfc00, 0x0000, 0x0000, 0x0400, 0x0b00, 0x0000, 0x0000
};

void tfd_filter_coff_ini(void)
{
	int i = 0;

	for (i = 0; i < 336; i++) {
		dvbt_isdbt_wr_reg(0x99 * 4, (i << 16) | coef[i]);
		dvbt_isdbt_wr_reg(0x03 * 4, (1 << 12));
	}
}

void ini_icfo_pn_index(int mode)
{				/* 00:DVBT,01:ISDBT */
	if (mode == 0) {
		dvbt_isdbt_wr_reg(0x3f8, 0x00000031);
		dvbt_isdbt_wr_reg(0x3fc, 0x00030000);
		dvbt_isdbt_wr_reg(0x3f8, 0x00000032);
		dvbt_isdbt_wr_reg(0x3fc, 0x00057036);
		dvbt_isdbt_wr_reg(0x3f8, 0x00000033);
		dvbt_isdbt_wr_reg(0x3fc, 0x0009c08d);
		dvbt_isdbt_wr_reg(0x3f8, 0x00000034);
		dvbt_isdbt_wr_reg(0x3fc, 0x000c90c0);
		dvbt_isdbt_wr_reg(0x3f8, 0x00000035);
		dvbt_isdbt_wr_reg(0x3fc, 0x001170ff);
		dvbt_isdbt_wr_reg(0x3f8, 0x00000036);
		dvbt_isdbt_wr_reg(0x3fc, 0x0014d11a);
	} else if (mode == 1) {
		dvbt_isdbt_wr_reg(0x3f8, 0x00000031);
		dvbt_isdbt_wr_reg(0x3fc, 0x00085046);
		dvbt_isdbt_wr_reg(0x3f8, 0x00000032);
		dvbt_isdbt_wr_reg(0x3fc, 0x0019a0e9);
		dvbt_isdbt_wr_reg(0x3f8, 0x00000033);
		dvbt_isdbt_wr_reg(0x3fc, 0x0024b1dc);
		dvbt_isdbt_wr_reg(0x3f8, 0x00000034);
		dvbt_isdbt_wr_reg(0x3fc, 0x003b3313);
		dvbt_isdbt_wr_reg(0x3f8, 0x00000035);
		dvbt_isdbt_wr_reg(0x3fc, 0x0048d409);
		dvbt_isdbt_wr_reg(0x3f8, 0x00000036);
		dvbt_isdbt_wr_reg(0x3fc, 0x00527509);
	}
}

void dvbt_write_regb(unsigned long addr, int index, unsigned long data)
{
	/*to achieve write func*/
}

void ofdm_initial(int bandwidth,
		/* 00:8M 01:7M 10:6M 11:5M */
		int samplerate,
		/* 00:45M 01:20.8333M 10:20.7M 11:28.57 100: 24.00 */
		int IF,
		/* 000:36.13M 001:-5.5M 010:4.57M 011:4M 100:5M */
		int mode,
		/* 00:DVBT,01:ISDBT */
		int tc_mode
		/* 0: Unsigned, 1:TC */
		)
{
	int tmp;
	int ch_if;
	int adc_freq;
	/*int memstart;*/
	PR_DVBT("[%s]bandwidth is %d,samplerate is %d",
		__func__, bandwidth, samplerate);
	PR_DVBT("IF is %d, mode is %d,tc_mode is %d\n",
		IF, mode, tc_mode);
	switch (IF) {
	case 0:
		ch_if = DEMOD_36_13M_IF;
		break;
	case 1:
		ch_if = (-1) * DEMOD_5_5M_IF;
		break;
	case 2:
		ch_if = DEMOD_4_57M_IF;
		break;
	case 3:
		ch_if = DEMOD_4M_IF;
		break;
	case 4:
		ch_if = DEMOD_5M_IF;
		break;
	default:
		ch_if = DEMOD_4M_IF;
		break;
	}
	switch (samplerate) {
	case 0:
		adc_freq = 45000;
		break;
	case 1:
		adc_freq = 20833;
		break;
	case 2:
		adc_freq = 20700;
		break;
	case 3:
		adc_freq = 28571;
		break;
	case 4:
		adc_freq = 24000;
		break;
	case 5:
		adc_freq = 25000;
		break;
	default:
		adc_freq = 28571;
		break;
	}

	dvbt_isdbt_wr_reg((0x02 << 2), 0x00800000);
	/* SW reset bit[23] ; write anything to zero */
	dvbt_isdbt_wr_reg((0x00 << 2), 0x00000000);

	dvbt_isdbt_wr_reg((0xe << 2), 0xffff);
	/* enable interrupt */

	if (mode == 0) {	/* DVBT */
		switch (samplerate) {
		case 0:
			dvbt_isdbt_wr_reg((0x08 << 2), 0x00005a00);
			break;	/* 45MHz */
		case 1:
			dvbt_isdbt_wr_reg((0x08 << 2), 0x000029aa);
			break;	/* 20.833 */
		case 2:
			dvbt_isdbt_wr_reg((0x08 << 2), 0x00002966);
			break;	/* 20.7   SAMPLERATE*512 */
		case 3:
			dvbt_isdbt_wr_reg((0x08 << 2), 0x00003924);
			break;	/* 28.571 */
		case 4:
			dvbt_isdbt_wr_reg((0x08 << 2), 0x00003000);
			break;	/* 24 */
		case 5:
			dvbt_isdbt_wr_reg((0x08 << 2), 0x00003200);
			break;	/* 25 */
		default:
			dvbt_isdbt_wr_reg((0x08 << 2), 0x00003924);
			break;	/* 28.571 */
		}
	} else {		/* ISDBT */
		switch (samplerate) {
		case 0:
			dvbt_isdbt_wr_reg((0x08 << 2), 0x0000580d);
			break;	/* 45MHz */
		case 1:
			dvbt_isdbt_wr_reg((0x08 << 2), 0x0000290d);
			break;	/* 20.833 = 56/7 * 20.8333 / (512/63)*512 */
		case 2:
			dvbt_isdbt_wr_reg((0x08 << 2), 0x000028da);
			break;	/* 20.7 */
		case 3:
			dvbt_isdbt_wr_reg((0x08 << 2), 0x0000383F);
			break;	/* 28.571  3863 */
		case 4:
			dvbt_isdbt_wr_reg((0x08 << 2), 0x00002F40);
			break;	/* 24 */
		default:
			dvbt_isdbt_wr_reg((0x08 << 2), 0x00003863);
			break;	/* 28.571 */
		}
	}
	/* memstart = 0x35400000;*/
	/* PR_DVBT("memstart is %x\n", memstart);*/
	/* dvbt_write_reg((0x10 << 2), memstart);*/
	/* 0x8f300000 */

	dvbt_isdbt_wr_reg((0x14 << 2), 0xe81c4ff6);
	/* AGC_TARGET 0xf0121385 */

	switch (samplerate) {
	case 0:
		dvbt_isdbt_wr_reg((0x15 << 2), 0x018c2df2);
		break;
	case 1:
		dvbt_isdbt_wr_reg((0x15 << 2), 0x0185bdf2);
		break;
	case 2:
		dvbt_isdbt_wr_reg((0x15 << 2), 0x0185bdf2);
		break;
	case 3:
		dvbt_isdbt_wr_reg((0x15 << 2), 0x0187bdf2);
		break;
	case 4:
		dvbt_isdbt_wr_reg((0x15 << 2), 0x0187bdf2);
		break;
	default:
		dvbt_isdbt_wr_reg((0x15 << 2), 0x0187bdf2);
		break;
	}
	if (tc_mode == 1)
		dvbt_write_regb((0x15 << 2), 11, 0);
	/* For TC mode. Notice, For ADC input is Unsigned,*/
	/* For Capture Data, It is TC. */
	dvbt_write_regb((0x15 << 2), 26, 1);
	/* [19:0] = [I , Q], I is high, Q is low. This bit is swap I/Q. */

	dvbt_isdbt_wr_reg((0x16 << 2), 0x00047f80);
	/* AGC_IFGAIN_CTRL */
	dvbt_isdbt_wr_reg((0x17 << 2), 0x00027f80);
	/* AGC_RFGAIN_CTRL */
	dvbt_isdbt_wr_reg((0x18 << 2), 0x00000190);
	/* AGC_IFGAIN_ACCUM */
	dvbt_isdbt_wr_reg((0x19 << 2), 0x00000190);
	/* AGC_RFGAIN_ACCUM */
	if (ch_if < 0)
		ch_if += adc_freq;
	if (ch_if > adc_freq)
		ch_if -= adc_freq;

	tmp = ch_if * (1 << 15) / adc_freq;
	dvbt_isdbt_wr_reg((0x20 << 2), tmp);

	dvbt_isdbt_wr_reg((0x21 << 2), 0x001ff000);
	/* DDC CS_FCFO_ADJ_CTRL */
	dvbt_isdbt_wr_reg((0x22 << 2), 0x00000000);
	/* DDC ICFO_ADJ_CTRL */
	dvbt_isdbt_wr_reg((0x23 << 2), 0x00004000);
	/* DDC TRACK_FCFO_ADJ_CTRL */

	dvbt_isdbt_wr_reg((0x27 << 2), (1 << 23)
	| (3 << 19) | (3 << 15) |  (1000 << 4) | 9);
	/* {8'd0,1'd1,4'd3,4'd3,11'd50,4'd9});//FSM_1 */
	dvbt_isdbt_wr_reg((0x28 << 2), (100 << 13) | 1000);
	/* {8'd0,11'd40,13'd50});//FSM_2 */
	dvbt_isdbt_wr_reg((0x29 << 2), (31 << 20) | (1 << 16) |
	(24 << 9) | (3 << 6) | 20);
	/* {5'd0,7'd127,1'd0,3'd0,7'd24,3'd5,6'd20}); */
	/*8K cannot sync*/
	dvbt_isdbt_wr_reg((0x29 << 2),
			dvbt_isdbt_rd_reg((0x29 << 2)) |
			0x7f << 9 | 0x7f << 20);

	if (mode == 0) {	/* DVBT */
		if (bandwidth == 0) {	/* 8M */
			switch (samplerate) {
			case 0:
				ini_acf_iireq_src_45m_8m();
				dvbt_isdbt_wr_reg((0x44 << 2),
					      0x004ebf2e);
				break;	/* 45M */
			case 1:
				ini_acf_iireq_src_207m_8m();
				dvbt_isdbt_wr_reg((0x44 << 2),
					      0x00247551);
				break;	/* 20.833M */
			case 2:
				ini_acf_iireq_src_207m_8m();
				dvbt_isdbt_wr_reg((0x44 << 2),
					      0x00243999);
				break;	/* 20.7M */
			case 3:
				ini_acf_iireq_src_2857m_8m();
				dvbt_isdbt_wr_reg((0x44 << 2),
					      0x0031ffcd);
				break;	/* 28.57M */
			case 4:
				ini_acf_iireq_src_24m_8m();
				dvbt_isdbt_wr_reg((0x44 << 2),
					      0x002A0000);
				break;	/* 24M */
			default:
				ini_acf_iireq_src_2857m_8m();
				dvbt_isdbt_wr_reg((0x44 << 2),
					      0x0031ffcd);
				break;	/* 28.57M */
			}
		} else if (bandwidth == 1) {	/* 7M */
			switch (samplerate) {
			case 0:
				ini_acf_iireq_src_45m_7m();
				dvbt_isdbt_wr_reg((0x44 << 2),
					      0x0059ff10);
				break;	/* 45M */
			case 1:
				ini_acf_iireq_src_207m_7m();
				dvbt_isdbt_wr_reg((0x44 << 2),
					      0x0029aaa6);
				break;	/* 20.833M */
			case 2:
				ini_acf_iireq_src_207m_7m();
				dvbt_isdbt_wr_reg((0x44 << 2),
					      0x00296665);
				break;	/* 20.7M */
			case 3:
				ini_acf_iireq_src_2857m_7m();
				dvbt_isdbt_wr_reg((0x44 << 2),
					      0x00392491);
				break;	/* 28.57M */
			case 4:
				ini_acf_iireq_src_24m_7m();
				dvbt_isdbt_wr_reg((0x44 << 2),
					      0x00300000);
				break;	/* 24M */
			default:
				ini_acf_iireq_src_2857m_7m();
				dvbt_isdbt_wr_reg((0x44 << 2),
					      0x00392491);
				break;	/* 28.57M */
			}
		} else if (bandwidth == 2) {	/* 6M */
			switch (samplerate) {
			case 0:
				ini_acf_iireq_src_45m_6m();
				dvbt_isdbt_wr_reg((0x44 << 2),
					      0x00690000);
				break;	/* 45M */
			case 1:
				ini_acf_iireq_src_207m_6m();
				dvbt_isdbt_wr_reg((0x44 << 2),
					      0x00309c3e);
				break;	/* 20.833M */
			case 2:
				ini_acf_iireq_src_207m_6m();
				dvbt_isdbt_wr_reg((0x44 << 2),
					      0x002eaaaa);
				break;	/* 20.7M */
			case 3:
				ini_acf_iireq_src_2857m_6m();
				dvbt_isdbt_wr_reg((0x44 << 2),
					      0x0042AA69);
				break;	/* 28.57M */
			case 4:
				ini_acf_iireq_src_24m_6m();
				dvbt_isdbt_wr_reg((0x44 << 2),
					      0x00380000);
				break;	/* 24M */
			default:
				ini_acf_iireq_src_2857m_6m();
				dvbt_isdbt_wr_reg((0x44 << 2),
					      0x0042AA69);
				break;	/* 28.57M */
			}
		} else {	/* 5M */
			switch (samplerate) {
			case 0:
				ini_acf_iireq_src_45m_5m();
				dvbt_isdbt_wr_reg((0x44 << 2),
					      0x007dfbe0);
				break;	/* 45M */
			case 1:
				ini_acf_iireq_src_207m_5m();
				dvbt_isdbt_wr_reg((0x44 << 2),
					      0x003a554f);
				break;	/* 20.833M */
			case 2:
				ini_acf_iireq_src_207m_5m();
				dvbt_isdbt_wr_reg((0x44 << 2),
					      0x0039f5c0);
				break;	/* 20.7M */
			case 3:
				ini_acf_iireq_src_2857m_5m();
				dvbt_isdbt_wr_reg((0x44 << 2),
					      0x004FFFFE);
				break;	/* 28.57M */
			case 4:
				ini_acf_iireq_src_24m_5m();
				dvbt_isdbt_wr_reg((0x44 << 2),
					      0x00433333);
				break;	/* 24M */
			default:
				ini_acf_iireq_src_2857m_5m();
				dvbt_isdbt_wr_reg((0x44 << 2),
					      0x004FFFFE);
				break;	/* 28.57M */
			}
		}
	} else {		/* ISDBT */
		switch (samplerate) {
		case 0:
			ini_acf_iireq_src_45m_6m();
			dvbt_isdbt_wr_reg((0x44 << 2), 0x00589800);
			break;
			/* 45M */
			/*SampleRate/(symbolRate)*2^20, */
			/*symbolRate = 512/63 for isdbt */
		case 1:
			ini_acf_iireq_src_207m_6m();
			dvbt_isdbt_wr_reg((0x44 << 2), 0x002903d4);
			break;	/* 20.833M */
		case 2:
			ini_acf_iireq_src_207m_6m();
			dvbt_isdbt_wr_reg((0x44 << 2), 0x00280ccc);
			break;	/* 20.7M */
		case 3:
			ini_acf_iireq_src_2857m_6m();
			dvbt_isdbt_wr_reg((0x44 << 2), 0x00383fc8);
			break;	/* 28.57M */
		case 4:
			ini_acf_iireq_src_24m_6m();
			dvbt_isdbt_wr_reg((0x44 << 2), 0x002F4000);
			break;	/* 24M */
		default:
			ini_acf_iireq_src_2857m_6m();
			dvbt_isdbt_wr_reg((0x44 << 2), 0x00383fc8);
			break;	/* 28.57M */
		}
	}

	if (mode == 0)		/* DVBT */
		dvbt_isdbt_wr_reg((0x02 << 2),
			      (bandwidth << 20) | 0x10002);
	else			/* ISDBT */
		dvbt_isdbt_wr_reg((0x02 << 2), (1 << 20) | 0x1001a);
	/* {0x000,2'h1,20'h1_001a});    //For ISDBT , bandwidth should be 1,*/

	dvbt_isdbt_wr_reg((0x45 << 2), 0x00000000);
	/* SRC SFO_ADJ_CTRL */
	dvbt_isdbt_wr_reg((0x46 << 2), 0x02004000);
	/* SRC SFO_ADJ_CTRL */
	dvbt_isdbt_wr_reg((0x48 << 2), 0x000c0287);
	/* DAGC_CTRL1 */
	dvbt_isdbt_wr_reg((0x49 << 2), 0x00000005);
	/* DAGC_CTRL2 */
	dvbt_isdbt_wr_reg((0x4c << 2), 0x00000bbf);
	/* CCI_RP */
	dvbt_isdbt_wr_reg((0x4d << 2), 0x00000376);
	/* CCI_RPSQ */
	dvbt_isdbt_wr_reg((0x4e << 2), 0x0f0f1d09);
	/* CCI_CTRL */
	dvbt_isdbt_wr_reg((0x4f << 2), 0x00000000);
	/* CCI DET_INDEX1 */
	dvbt_isdbt_wr_reg((0x50 << 2), 0x00000000);
	/* CCI DET_INDEX2 */
	dvbt_isdbt_wr_reg((0x51 << 2), 0x00000000);
	/* CCI_NOTCH1_A1 */
	dvbt_isdbt_wr_reg((0x52 << 2), 0x00000000);
	/* CCI_NOTCH1_A2 */
	dvbt_isdbt_wr_reg((0x53 << 2), 0x00000000);
	/* CCI_NOTCH1_B1 */
	dvbt_isdbt_wr_reg((0x54 << 2), 0x00000000);
	/* CCI_NOTCH2_A1 */
	dvbt_isdbt_wr_reg((0x55 << 2), 0x00000000);
	/* CCI_NOTCH2_A2 */
	dvbt_isdbt_wr_reg((0x56 << 2), 0x00000000);
	/* CCI_NOTCH2_B1 */
	dvbt_isdbt_wr_reg((0x58 << 2), 0x00000885);
	/* MODE_DETECT_CTRL // 582 */
	if (mode == 0)		/* DVBT */
		dvbt_isdbt_wr_reg((0x5c << 2), 0x00001011);	/*  */
	else
		dvbt_isdbt_wr_reg((0x5c << 2), 0x00000453); // Q_threshold
	/* ICFO_EST_CTRL ISDBT ICFO thres = 2 */

	dvbt_isdbt_wr_reg((0x5f << 2), 0x0ffffe10);
	/* TPS_FCFO_CTRL */
	dvbt_isdbt_wr_reg((0x61 << 2), 0x0000006c);
	/* FWDT ctrl */
	dvbt_isdbt_wr_reg((0x68 << 2), 0x128c3929);
	dvbt_isdbt_wr_reg((0x69 << 2), 0x91017f2d);
	/* 0x1a8 */
	dvbt_isdbt_wr_reg((0x6b << 2), 0x00442211);
	/* 0x1a8 */
	dvbt_isdbt_wr_reg((0x6c << 2), 0x01fc400a);
	/* 0x */
	dvbt_isdbt_wr_reg((0x6d << 2), 0x0030303f);
	/* 0x */
	dvbt_isdbt_wr_reg((0x73 << 2), 0xffffffff);
	/* CCI0_PILOT_UPDATE_CTRL */
	dvbt_isdbt_wr_reg((0x74 << 2), 0xffffffff);
	/* CCI0_DATA_UPDATE_CTRL */
	dvbt_isdbt_wr_reg((0x75 << 2), 0xffffffff);
	/* CCI1_PILOT_UPDATE_CTRL */
	dvbt_isdbt_wr_reg((0x76 << 2), 0xffffffff);
	/* CCI1_DATA_UPDATE_CTRL */

	tmp = mode == 0 ? 0x000001a2 : 0x00000da2;
	dvbt_isdbt_wr_reg((0x78 << 2), tmp);	/* FEC_CTR */

	dvbt_isdbt_wr_reg((0x7d << 2), 0x0000009d);
	dvbt_isdbt_wr_reg((0x7e << 2), 0x00004000);
	dvbt_isdbt_wr_reg((0x7f << 2), 0x00008000);

	dvbt_isdbt_wr_reg(((0x8b + 0) << 2), 0x20002000);
	dvbt_isdbt_wr_reg(((0x8b + 1) << 2), 0x20002000);
	dvbt_isdbt_wr_reg(((0x8b + 2) << 2), 0x20002000);
	dvbt_isdbt_wr_reg(((0x8b + 3) << 2), 0x20002000);
	dvbt_isdbt_wr_reg(((0x8b + 4) << 2), 0x20002000);
	dvbt_isdbt_wr_reg(((0x8b + 5) << 2), 0x20002000);
	dvbt_isdbt_wr_reg(((0x8b + 6) << 2), 0x20002000);
	dvbt_isdbt_wr_reg(((0x8b + 7) << 2), 0x20002000);

	dvbt_isdbt_wr_reg((0x93 << 2), 0x31);
	dvbt_isdbt_wr_reg((0x94 << 2), 0x00);
	dvbt_isdbt_wr_reg((0x95 << 2), 0x7f1);
	dvbt_isdbt_wr_reg((0x96 << 2), 0x20);

	dvbt_isdbt_wr_reg((0x98 << 2), 0x03f9115a);
	dvbt_isdbt_wr_reg((0x9b << 2), 0x000005df);

	dvbt_isdbt_wr_reg((0x9c << 2), 0x00100000);
	/* TestBus write valid, 0 is system clk valid */
	dvbt_isdbt_wr_reg((0x9d << 2), 0x01000000);
	/* DDR Start address */
	dvbt_isdbt_wr_reg((0x9e << 2), 0x02000000);
	/* DDR End   address */

	dvbt_write_regb((0x9b << 2), 7, 0);
	/* Enable Testbus dump to DDR */
	dvbt_write_regb((0x9b << 2), 8, 0);
	/* Run Testbus dump to DDR */

	dvbt_isdbt_wr_reg((0xd6 << 2), 0x00000003);
	dvbt_isdbt_wr_reg((0xd8 << 2), 0x00000120);
	dvbt_isdbt_wr_reg((0xd9 << 2), 0x01010101);

	ini_icfo_pn_index(mode);
	tfd_filter_coff_ini();

	calculate_cordic_para();
	msleep(20);
	/* delay_us(1); */

	dvbt_isdbt_wr_reg((0x02 << 2),
		      dvbt_isdbt_rd_reg((0x02 << 2)) | (1 << 0));
	dvbt_isdbt_wr_reg((0x02 << 2),
		      dvbt_isdbt_rd_reg((0x02 << 2)) | (1 << 24));
/* dvbt_check_status(); */
}

#endif

//when frontend top AGC is enabled, sub-module local AGC
//will be disabled automatically.
void demod_enable_frontend_agc(struct aml_dtvdemod *demod,
		enum fe_delivery_system delsys, bool enable)
{
	unsigned int top_saved = 0, polling_en = 0;
	struct amldtvdemod_device_s *devp = (struct amldtvdemod_device_s *)demod->priv;

	if (!enable) {
		//enable frontend agc, 0x20[18]: 0x1 use frontend agc, 0x0 use local agc.
		front_write_bits(DEMOD_FRONT_AFIFO_ADC, 0, 18, 1);
		return;
	}

	if (delsys == SYS_DVBT || delsys == SYS_DVBT2) {
		//set f040 = 0x0, disable T/T2 mode, stop to
		//access T/T2 regs, so should stop demod_thread to access T/T2 status first.
		polling_en = devp->demod_thread;
		devp->demod_thread = 0;
		top_saved = demod_top_read_reg(DEMOD_TOP_CFG_REG_4);
		demod_top_write_reg(DEMOD_TOP_CFG_REG_4, 0x0);
	}

	PR_DBG("frontagc 0x20 %#x 0x21 %#x 0x22 %#x 0x23 %#x 0x26 %#x 0x28 %#x top_saved %#x\n",
				front_read_reg(0x20), front_read_reg(0x21), front_read_reg(0x22),
				front_read_reg(0x23), front_read_reg(0x26), front_read_reg(0x28),
				top_saved);

	front_write_reg(DEMOD_FRONT_AGC_CFG1, 0x10122);

	if (delsys == SYS_DVBC_ANNEX_A && !devp->blind_scan_stop &&
			demod->dvbc_sel == 1)
		//when use DVB-C ch1, adc_iq_exchange = 1.
		front_write_reg(DEMOD_FRONT_AGC_CFG2, 0x7200a36);
	else
		front_write_reg(DEMOD_FRONT_AGC_CFG2, 0x7200a16); //config same as dtmb 0x22

	front_write_reg(DEMOD_FRONT_AGC_CFG6, 0x1a000f0f); //config same as dtmb 0x46

	if (front_agc_target)
		front_write_bits(DEMOD_FRONT_AGC_CFG6, front_agc_target, 24, 6);

	//disable dc remove1 bypass
	front_write_bits(DEMOD_FRONT_DC_CFG1, 1, 31, 1);

	//enable frontend agc, 0x20[18]: 0x1 use frontend agc, 0x0 use local agc.
	//0x20[17]: adc_real_only.
	if (delsys == SYS_DVBS || delsys == SYS_DVBS2)
		front_write_bits(DEMOD_FRONT_AFIFO_ADC, 0x2, 17, 2);
	else
		front_write_bits(DEMOD_FRONT_AFIFO_ADC, 0x3, 17, 2);

	PR_DBG("frontagc 0x20 %#x 0x21 %#x 0x22 %#x 0x23 %#x 0x26 %#x 0x28 %#x\n",
			front_read_reg(0x20), front_read_reg(0x21), front_read_reg(0x22),
			front_read_reg(0x23), front_read_reg(0x26), front_read_reg(0x28));

	if (delsys == SYS_DVBT || delsys == SYS_DVBT2) {
		//f040 = 0x182: host only can access top regs and T/T2 regs
		demod_top_write_reg(DEMOD_TOP_CFG_REG_4, top_saved);
		devp->demod_thread = polling_en;
	}
}

static int x_to_power_y(int number, unsigned int power)
{
	unsigned int i;
	int result = 1;

	for (i = 0; i < power; i++)
		result *= number;

	return result;
}

void fe_l2a_set_symbol_rate(struct fe_l2a_internal_param *pparams, unsigned int symbol_rate)
{
	//unsigned int reg_field2, reg_field1, reg_field0;
	unsigned int reg32;
	int tmp;
	int tmp_f;

	//reg_field2 = FLD_FL2A_DVBSX_DEMOD_SFRINIT2_SFR_INIT;
	//reg_field1 = FLD_FL2A_DVBSX_DEMOD_SFRINIT1_SFR_INIT;
	//reg_field0 = FLD_FL2A_DVBSX_DEMOD_SFRINIT0_SFR_INIT;

	/* sfr_init = sfr_init(MHz) * 2^24 / ckadc (unsigned), ckadc = nsamples * Mclk */
	/* max SR: MCLK/2=67.5MS/s rounded to 70MS/s */

	/*reg32 = (symbol_rate / 1000) * (1 << 15);
	 *reg32 = reg32 / (pParams->master_clock / 1000);
	 *reg32 = reg32 * (1 << 9);

	 *error |= fe_write_field(pParams->handle_demod, reg_field2,
	 *		((s32)reg32 >> 16) & 0xFF);
	 *error |= fe_write_field(pParams->handle_demod, reg_field1,
	 *		((s32)reg32 >> 8) & 0xFF);
	 *error |= fe_write_field(pParams->handle_demod, reg_field0,
	 *		((s32)reg32) & 0xFF);
	 */

	//reg32 = (symbol_rate * 16777216)/master_clock;
	reg32 = (symbol_rate / 1000) * (1 << 15);
	//printf("pParams->master_clock is %d\n",pParams->master_clock);
	//pParams->master_clock = 135000000;

	//reg32 = reg32 / (pParams->master_clock / 1000);
	reg32 = reg32 / (135000000 / 1000);
	reg32 = reg32 * (1 << 9);
	PR_DVBC("reg32: %d, symb_rate: %d.\n", reg32, symbol_rate);

	dvbs_wr_byte(0x9f0, (reg32 >> 16) & 0xff);
	dvbs_wr_byte(0x9f1, (reg32 >> 8) & 0xff);
	dvbs_wr_byte(0x9f2, reg32 & 0xff);
	tmp = (((dvbs_rd_byte(0x9f0)) << 16) + ((dvbs_rd_byte(0x9f1)) << 8) +
			(dvbs_rd_byte(0x9f2)));
	tmp_f = tmp * 135 / 16777216;
	PR_DVBC(" after %s init 9f0 sr = %d %d Mbps\n", __func__, tmp, tmp_f);
}

void fe_l2a_get_agc2accu(struct fe_l2a_internal_param *pparams, unsigned int *pintegrator)
{
	unsigned int agc2acc_mant, agc2acc_exp, fld_value[2] = {0};

	unsigned int Mantissa;
	signed int Exponent;
	//unsigned long long Value;
	//unsigned int Value;
	unsigned int AGC2I1, AGC2I0;
	unsigned short mant;
	unsigned char exp;
	signed int exp_abs_s32 = 0, exp_s32 = 0;

	fld_value[0] = dvbs_rd_byte(0x9a0);
	fld_value[1] = (dvbs_rd_byte(0x9a1) & 0xc0) >> 6;//9a1&c0
	Mantissa = fld_value[1] + (fld_value[0] << 2);
	fld_value[0] = (dvbs_rd_byte(0x9a1) & 0x3f);
	Exponent = (signed int)(fld_value[0]);

	*pintegrator = Mantissa * (unsigned int)POWOF2(Exponent + 5 - 9); /* 2^5=32 */

	/* Georg's method */
	fld_value[0] = dvbs_rd_byte(0x9a0);
	fld_value[1] = (dvbs_rd_byte(0x9a1) & 0xc0) >> 6;//9a1&c0
	agc2acc_mant = (MAKEWORD(fld_value[0], fld_value[1])) >> 6;
	agc2acc_exp = dvbs_rd_byte(0x9a1) & 0x3f;
	if (((int)(agc2acc_exp - 9)) >= 0)
		*pintegrator = agc2acc_mant * (unsigned int)POWOF2(agc2acc_exp - 9);
	//printf("Integrator is %d\n",*pIntegrator);

	AGC2I1 = dvbs_rd_byte(0x9a0);
	//printf("0x9a0 is %x\n",AGC2I1);
	AGC2I0 = dvbs_rd_byte(0x9a1);
	mant = (unsigned short)((AGC2I1 * 4) + ((AGC2I0 >> 6) & 0x3));
	exp = (unsigned char)(AGC2I0 & 0x3f);
	PR_DVBC("mant is %d\n", mant);
	/*evaluate exp-9 */
	exp_s32 = (signed int)(exp - 9);

	/*evaluate exp -9 sign */
	if (exp_s32 < 0) {
		/* if exp_s32<0 divide the mantissa  by 2^abs(exp_s32)*/
		exp_abs_s32 = x_to_power_y(2, (unsigned int)(-exp_s32));
		*pintegrator = (unsigned int)((1000 * (mant)) / exp_abs_s32);
		PR_DVBC("Integrator is %d\n", *pintegrator);
	} else {
		/*if exp_s32> 0 multiply the mantissa  by 2^(exp_s32)*/
		exp_abs_s32 = x_to_power_y(2, (unsigned int)(exp_s32));
		*pintegrator = (unsigned int)((1000 * mant) * exp_abs_s32);
		PR_DVBC("Integrator is %d\n", *pintegrator);
	}
}

