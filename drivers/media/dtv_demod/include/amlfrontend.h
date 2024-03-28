/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2021 Amlogic, Inc. All rights reserved.
 */

#ifndef _AMLFRONTEND_H
#define _AMLFRONTEND_H

#include "depend.h"
#ifdef AML_DEMOD_SUPPORT_DVBS
#include "dvbs_diseqc.h"
#endif
#ifdef AML_DEMOD_SUPPORT_DVBC
#include "dvbc_func.h"
#endif

#include <linux/amlogic/cpu_version.h>
#include <linux/amlogic/aml_dtvdemod.h>

/****************************************************/
/*  V1.0.22  no audio output after random source switch */
/*  V1.0.23  fixed code and dts CMA config          */
/*  V1.0.24  dvbt2 add reset when unlocked for 3s   */
/*  V1.0.25  add demod version and t2 fw version node*/
/*  V1.0.26  weak signal display after dvbs search*/
/*  V1.0.27  s4d remove dvbs blindscan fastsearch threadhold*/
/*  V1.0.28  fix the hang when read mplp info in DVBT system*/
/*  V1.0.29  add auto recognition t/t2 r842 config*/
/*  V1.0.30  DVBT2 no audio output after random source switch*/
/*  V1.1.31  t5w back trunk,version fix              */
/*  V1.1.32  dvbc frc offset optimization              */
/*  V1.1.33  dvbs qpsk 1/4 C/N worse             */
/*  V1.1.34  dvbs blind scan new work plan with fft   */
/*  V1.1.35  fixed s4/s4d 2 * dvbc agc control  */
/*  V1.1.36  Stuck during the blindscan process  */
/*  V1.1.37  dvbt2 change channel will return to home  */
/*  V1.1.38  Redistribution blind scan progress value reporting  */
/*  V1.1.39  fixed DVBS blind scan workqueue quit */
/*  V1.1.40  t5w change list check abus audio problem */
/*  V1.1.41  support auto qam in j83b */
/*  V1.1.42  optimization of DVBS blind scan  */
/*  V1.1.43  rebuild dvbc to fix autosr and recovery is slowly */
/*  V1.1.44  t3 revb change list check abus audio problem */
/*  V1.1.45  optimize dvb-c auto symbol rate(all) and auto qam(t5w) */
/*  V1.1.46  use the codec_mm cma for DTMB(8M)/DVB-T2(40M)/ISDB-T(8M) */
/*  V1.1.47  support IRC and HRC in j83b auto qam mode */
/*  V1.1.48  fixed 16qam/32qam cost long time to lock up or error */
/*  V1.1.49  fix HRC freq of 79M lock failed */
/*  V1.1.50  fixed data type of memory address and read/write */
/*  V1.1.51  fixed dvbs/s2/isdbt aft test and support dvbt2 5/6/1.7M */
/*  V1.1.52  add ambus exit processing when switching mode */
/*  V1.1.53  fixed dvb-c auto qam unlock when 16qam/32qam */
/*  V1.1.54  rebuild atsc to improve signal locking performance */
/*  V1.1.55  improve atsc cci */
/*  V1.1.56  add dvbt2 fef info */
/*  V1.1.57  fix auto qam(t5w) and sr */
/*  V1.1.58  fix delsys exit setting and r842 dvbc auto sr */
/*  V1.1.59  fix dvbs/s2 aft range and different tuner blind window settings */
/*  V1.1.60  support diseqc2.0 */
/*  V1.1.61  fix T5W T and T2 channel switch unlock */
/*  V1.1.62  implement get RSSI function for av2018 */
/*  V1.1.63  fix dvbc 128/256qam unlock */
/*  V1.1.64  fix atsc static echo test failed in -30us */
/*  V1.1.65  fix locking qam64 signal failed in j83b */
/*  V1.1.66  improve dvbs blind scan and support single cable */
/*  V1.1.67  avoid dvbc missing channel */
/*  V1.1.68  add a function to invert the spectrum in dvbs blind scan */
/*  V1.1.69  fixed atsc agc speed test */
/*  V1.1.70  improve diseqc lnb control */
/*  V1.1.71  fix dvbt2 ddr abnormal access */
/*  V1.1.72  t5m ATVDemod/DTVDemod/Tuner bringup */
/*  V1.1.73  add tps cell id info and dmc notifier test */
/*  V1.1.74  fix s ber and adc fga gain and atsc ch if */
/*  V1.1.75  optimize identification of auto qam */
/*  V1.1.76  fix t5m DVB-C auto QAM timeout */
/*  V1.1.77  fix dvbc low sr and 16/32qam unlock */
/*  V1.1.78  optimize 8VSB CN to 15dB */
/*  V1.1.79  fix diseqc init state and add isdbt tmcc */
/*  V1.1.80  fix ATSC-T agc_target and lock signal timeout on r842 */
/*  V1.1.81  fix demod AMBUS issue on t5m */
/*  V1.1.82  revert optimize 8VSB CN to 15dB */
/*  V1.1.83  fix diseqc 22k on and off after tune */
/*  V1.1.84  rebuild isdbt to improve signal locking performance */
/*  V1.1.85  change the calculation method of ber */
/*  V1.1.86  remove S/S2 ber config */
/*  V1.1.87  fix t5m dvbc 128qam unlock */
/*  V1.1.88  fix dvb-s unicable blind scan failed */
/*  V1.1.89  fix T5D/T3 switch to T2 unlock */
/*  V1.1.90  fix dvb-s blind scan failed */
/*  V1.1.91  fix t2 agc target config for r842 */
/*  V1.1.92  fix diseqc2.0 rx */
/*  V1.1.93  add t5m dvbt2 mplp config */
/*  V1.1.94  fix j83b's status false alarm */
/*  V1.1.95  fix diseqc and lnb attach */
/*  V1.1.96  fix T2 pulse test failed when tuner is mxl661 */
/*  V1.1.97  optimize 8VSB CN */
/*  V1.1.98  fix r842 atsc missing TP and scan slowly */
/*  V1.1.99  fix init flow */
/*  V1.1.100 bring up dump ADC data feature */
/*  V3.1.101 fix dvb-s unicable blind scan miss TP */
/*  V3.1.102 bring up dump ADC data feature */
/*  V3.2.000 t3x bringup */
/*  V3.2.001 optimize isdbt stability of locking signal */
/*  V3.2.002 fix dvbc aft test unstable */
/*  V3.2.003 improve performance when atsc signal is weak */
/*  V3.2.004 fix dvbc new driver to solve missing channel */
/*  V3.2.005 fix dvbs blind scan new miss 2150M */
/*  V3.2.006 support for identifying rt720 by name */
/*  V3.2.007 increase the speed of isdb-t re-lock */
/*  V3.2.008 fix significant fluctuations of dvbs snr */
/*  V3.2.009 support dump demod ddr data */
/*  V3.2.010 improve compatibility with dvb-t signals */
/*  V3.2.011 fix the sync of shutdown and tune */
/*  V3.2.012 improved dvbc auto qam (t5w/t5m) */
/*  V3.2.013 fixed dvbc 16/32 qam (t5d/t3) */
/*  V3.3.001  txhd2 bringup */
/*  V3.3.002 fix dvbt signaling buf overflow when bw 6M and GI 1/4 */
/*  V3.3.003 add t2 ddr clock reset and arbit control when tune (t5m/t3x) */
/*  V3.3.004 add ATSC Monitor call for r842 */
/*  V3.3.005 fix no signal after system resume */
/*  V3.4.000 standard isolation and optimization */
/*  V3.4.001 fix dvbc qam instability or lock time is too long */
/*  V3.4.002 fix dvbt overflow by new methods when bw 6M GI 1/4 */
/*  V3.4.003 improve the stability of new dvbc driver scanning */
/*  V3.4.004 fix dvbc qam config and flow */
/*  V3.4.005 adjust the sensitivity limit of atbm253 tuner in DTMB mode */
/*  V3.5.000 s1a bringup */
/*  V3.5.001 fix rda5815m bandwidth and dvbs iq swap config */
/*  V3.5.002 s1a dvbc bringup */
/*  V3.5.003 multi path interference unqualified problem */
/*  V3.5.004 bring up new dvbc blind scan feature */
/*  V3.5.005 s1a add efuse config */
/*  V3.5.006 fix j83b qam setting when auto qam */
/*  V3.5.007 improve the accuracy of dvbs cn value */
/*  V3.5.008 fix discarding signal when dvbt2 level is very low */
/*  V3.5.009 fix s1a dvbc dvbs efuse bit */
/*  V3.5.010 add pm config for s1a */
/*  V3.5.011 delete debug logs in probe */
/*  V3.5.012 add DTV_STAT_SIGNAL_STRENGTH and DTV_STAT_CNR */
/*  V3.5.013 fix r842 dvbt/t2 LTE interferer test fail */
/*  V3.5.014 Support ignoring common plp in dvbt2 */
/*  V3.5.015 remove unused macro and code */
/*  V3.5.016 optimize dvbs rssi for rt720 */
/*  V3.5.017 increase the speed of dvbc blind scanning */
/*  V3.5.018 optimize dvbs blind scan progress */
/*  V3.5.019 fix dvbt lock failed in weak signal */
/*  V3.5.020 fix pm runtime error */
/*  V3.5.021 optimize the ddr size of cma(max 40M reduced to 16M) */
/*  V3.5.022 delsys code isolation */
/*  V3.5.023 fix no timeout when dvbc signal is abnormal */
/*  V3.5.024 fix dvbs diseqc timing */
/*  V3.5.025 fix dvbc blind scan init issue */
/*  V3.5.026 support dtmb 6M BW */
/*  V3.5.027 Support switching auto between dvb-t and t2 */
/*  V3.5.028 fix not lock signal in dvbt2 DTG161 case */
/*  V3.5.029 fix atsc-c signal strength issue */
/*  V3.5.030 fix dvbt overflow when 6M 1/4 and other param */
/*  V3.5.031 calibrate dvbc signal strength read from R842 */
/*  V3.5.032 rebuild dtmb driver */
/****************************************************/
/****************************************************************/
/*               AMLDTVDEMOD_VER  Description:                  */
/*->The first digit indicates:                                  */
/*    In case of kernel version replacement                     */
/*    or driver schema update, increase the corresponding value */
/*->The second digit indicates:                                 */
/*    The changes relate to electrical performance and          */
/*    important function development                            */
/*->The third digit indicates:                                  */
/*    Minor changes and bug fixes                               */
/*             AMLDTVDEMOD_T2_FW_VER  Description:              */
/*->The first four digits indicate the fw version number        */
/*->The last four digits indicate the release time              */
/****************************************************************/
#define KERNEL_4_9_EN		1
#define AMLDTVDEMOD_VER "V3.5.032"
#define DTVDEMOD_VER	"2024/02/27: rebuild dtmb driver"
#define AMLDTVDEMOD_T2_FW_VER "20231019_141000"
#define DEMOD_DEVICE_NAME  "dtvdemod"

#if defined AML_DEMOD_SUPPORT_ATSC || defined AML_DEMOD_SUPPORT_J83B
#define THRD_TUNER_STRENGTH_ATSC (-87)
#define TIMEOUT_ATSC		3000
#define TIMEOUT_ATSC_STD	1500
#endif

#ifdef AML_DEMOD_SUPPORT_J83B
#define THRD_TUNER_STRENGTH_J83 (-76)
#endif

#ifdef AML_DEMOD_SUPPORT_DVBT
/* tested on BTC, sensitivity of demod is -100dBm */
#define THRD_TUNER_STRENGTH_DVBT (-101)
#define TIMEOUT_DVBT		3000
#define TIMEOUT_DVBT2		5000
#endif

#ifdef AML_DEMOD_SUPPORT_DVBS
#define THRD_TUNER_STRENGTH_DVBS (-79)
#define TIMEOUT_DVBS		2000
#endif

#ifdef AML_DEMOD_SUPPORT_DTMB
#define THRD_TUNER_STRENGTH_DTMB (-100)
#define TIMEOUT_DTMB		2500
#endif

#ifdef AML_DEMOD_SUPPORT_DVBC
#define THRD_TUNER_STRENGTH_DVBC (-87)
#define TIMEOUT_DVBC		3000
#endif

#ifdef AML_DEMOD_SUPPORT_ISDBT
#define THRD_TUNER_STRENGTH_ISDBT (-90)
#define TIMEOUT_ISDBT		3000
#endif

#define TIMEOUT_DDR_LEAVE	50

enum qam_md_e {
	QAM_MODE_16,
	QAM_MODE_32,
	QAM_MODE_64,
	QAM_MODE_128,
	QAM_MODE_256,
	QAM_MODE_AUTO,
	QAM_MODE_NUM
};

enum DEMOD_TUNER_IF {
	DEMOD_4M_IF = 4000,
	DEMOD_4_57M_IF = 4570,
	DEMOD_5M_IF = 5000,
	DEMOD_5_5M_IF = 5500,
	DEMOD_6M_IF = 6000,
	DEMOD_36_13M_IF = 36130,
};

#define ADC_CLK_24M	24000
#define ADC_CLK_25M	25000
#define ADC_CLK_54M	54000
#define ADC_CLK_135M	135000

#define DEMOD_CLK_60M	60000
#define DEMOD_CLK_72M	72000
#define DEMOD_CLK_125M	125000
#define DEMOD_CLK_167M	167000
#define DEMOD_CLK_200M	200000
#define DEMOD_CLK_216M	216000
#define DEMOD_CLK_225M	225000
#define DEMOD_CLK_250M	250000
#define DEMOD_CLK_270M	270000

#define FIRMWARE_NAME	"dtvdemod_t2.bin"
#define FIRMWARE_DIR	"dtvdemod"
#define PATH_MAX_LEN	50
#define FW_BUFF_SIZE	(100 * 1024)

enum M6_Demod_Pll_Mode {
	CRY_MODE = 0,
	ADC_MODE = 1
};

/*
 * e: enum
 * s: system
 */
enum es_map_addr {
	ES_MAP_ADDR_DEMOD,
	ES_MAP_ADDR_IOHIU,
	ES_MAP_ADDR_AOBUS,
	ES_MAP_ADDR_RESET,
	ES_MAP_ADDR_NUM
};
struct ss_reg_phy {
	unsigned int phy_addr;
	unsigned int size;
	/*void __iomem *p;*/
	/*int flag;*/
};
struct ss_reg_vt {
	void __iomem *v;
	int flag;
};

struct ddemod_reg_off {
	unsigned int off_demod_top;
	unsigned int off_dvbc;
	unsigned int off_dvbc_2;
	unsigned int off_dtmb;
	unsigned int off_dvbt_isdbt;
	unsigned int off_dvbt_t2;
	unsigned int off_dvbs;
	unsigned int off_atsc;
	unsigned int off_front;
	unsigned int off_isdbt;
};

enum dtv_demod_hw_ver_e {
	DTVDEMOD_HW_ORG = 0,
	DTVDEMOD_HW_TXLX,
	DTVDEMOD_HW_SM1,
	DTVDEMOD_HW_TL1,
	DTVDEMOD_HW_TM2,
	DTVDEMOD_HW_TM2_B,
	DTVDEMOD_HW_T5,
	DTVDEMOD_HW_T5D,
	DTVDEMOD_HW_S4,
	DTVDEMOD_HW_T5D_B,
	DTVDEMOD_HW_T3,
	DTVDEMOD_HW_S4D,
	DTVDEMOD_HW_T5W,
	DTVDEMOD_HW_T5M,
	DTVDEMOD_HW_T3X,
	DTVDEMOD_HW_TXHD2,
	DTVDEMOD_HW_S1A
};

struct ddemod_dig_clk_addr {
	unsigned int demod_clk_ctl;
	unsigned int demod_clk_ctl_1;
};

struct meson_ddemod_data {
	struct ddemod_dig_clk_addr dig_clk;
	struct ddemod_reg_off regoff;
	enum dtv_demod_hw_ver_e hw_ver;
};

/*polling*/
struct poll_machie_s {
	unsigned int flg_stop;	/**/
	unsigned int flg_restart;

	unsigned int state;	/*idel, work,*/


	/**/
	unsigned int delayms;
	unsigned int flg_updelay;

	unsigned int crrcnt;
	unsigned int maxcnt;

	enum fe_status last_s;
	unsigned int bch;
};

struct aml_demod_para {
	u32_t dvbc_symbol;
	u32_t dvbc_qam;
	u32_t dtmb_qam;
	u32_t dtmb_coderate;
};

struct aml_demod_para_real {
	u32_t modulation;
	u32_t coderate;
	u32_t symbol;
	u32_t snr;
	u32_t plp_num;
	u64_t plp_common;
	u32_t fef_info;
	u32_t tps_cell_id;
	u32_t ber;
};

#define CAP_NAME_LEN	100
struct dtvdemod_capture_s {
	char cap_dev_name[CAP_NAME_LEN];
	unsigned int cap_size;
};

struct timer_t {
	int enable;
	unsigned int start;
	unsigned int max;
};

enum ddemod_timer_s {
	D_TIMER_DETECT,
	D_TIMER_SET,
	D_TIMER_DBG1,
	D_TIMER_DBG2,
};

struct aml_dtvdemod {
	bool act_dtmb;

	struct poll_machie_s poll_machie;

	unsigned int en_detect;

	enum fe_delivery_system last_delsys;

	struct dvb_frontend frontend;

	unsigned int symbol_rate_manu;
	unsigned int sr_val_hw;
	unsigned int sr_val_hw_stable;
	unsigned int sr_val_hw_count;
	unsigned int sr_val_uf_count;
	unsigned int symb_rate_en;
	bool auto_sr;
	bool auto_sr_done;
	unsigned int freq;
	unsigned int freq_dvbc;
	enum fe_modulation atsc_mode;
	struct aml_demod_para para_demod;
	unsigned int timeout_atsc_ms;
#ifdef AML_DEMOD_SUPPORT_DVBT
	unsigned int timeout_dvbt_ms;
#endif
#ifdef AML_DEMOD_SUPPORT_DVBS
	unsigned int timeout_dvbs_ms;
#endif
	unsigned int time_start;
	unsigned int time_passed;
	enum fe_status last_status;
#ifdef AML_DEMOD_SUPPORT_DVBC
	unsigned int timeout_dvbc_ms;
#endif
	int autoflags;
	int auto_flags_trig;
	unsigned int p1_peak;

	enum qam_md_e auto_qam_mode;
	enum qam_md_e auto_qam_list[5];
	enum qam_md_e last_qam_mode;
	unsigned int auto_times;
	unsigned int auto_done_times;
	bool auto_qam_done;
	bool fsm_reset;
	unsigned int auto_qam_index;
	unsigned int auto_no_sig_cnt;
	bool fast_search_finish;

	enum fe_status atsc_dbg_lst_status;

	unsigned int t_cnt;

	/* select dvbc module for s4 */
	unsigned int dvbc_sel;

	unsigned int bw;
	unsigned int plp_id;
	int last_lock;

	struct aml_demod_sta demod_status;

	struct task_struct *cci_task;
	int cciflag;

	struct timer_t gtimer[4];

	struct list_head list;
	void *priv;
	int id;
	bool inited;
	bool suspended;
	bool reseted;
	unsigned int ci_mode;
	unsigned int timeout_ddr_leave;

	struct aml_demod_para_real real_para;

	struct pinctrl *pin_agc;    /*agc pinctrl*/
	struct pinctrl *pin_diseqc_out; /*diseqc out pin*/
	struct pinctrl *pin_diseqc_in; /*diseqc in pin*/

	u32 blind_result_frequency;
	u32 blind_result_symbol_rate;
};

struct amldtvdemod_device_s {

	struct class *clsp;
	struct device *dev;
	struct mutex lock;	/*aml_lock*/
	struct ss_reg_phy reg_p[ES_MAP_ADDR_NUM];
	struct ss_reg_vt reg_v[ES_MAP_ADDR_NUM];
	unsigned int dmc_phy_addr;
	unsigned int dmc_saved;
	void __iomem *dmc_v_addr;
	unsigned int ddr_phy_addr;
	void __iomem *ddr_v_addr;

	struct ddemod_reg_off ireg;
	struct meson_ddemod_data *data;

#ifdef KERNEL_4_9_EN
	/* clktree */
	unsigned int clk_gate_state;
	struct clk *vdac_clk_gate;
	unsigned int clk_demod_32k_state;
	struct clk *demod_32k;
#endif

	bool agc_direction;

#if 1 /*move to aml_dtv_demod*/
	/*for mem reserved*/
	unsigned int		mem_start;
	unsigned int		mem_end;
	unsigned int		mem_size;
	unsigned int		cma_flag;
	bool		flg_cma_allc;

#ifdef CONFIG_CMA
	struct platform_device	*this_pdev;
	struct page			*venc_pages;
	unsigned int			cma_mem_size;/* BYTE */
	unsigned int			cma_mem_alloc;
#endif

	/*for atsc version*/
	int			atsc_version;
	/*for dtv priv*/
#endif

	const struct amlfe_exp_config *afe_cfg;
	struct dentry *demod_root;

	struct dtvdemod_capture_s capture_para;
	unsigned int stop_reg_wr;
	struct delayed_work fw_dwork;
	char firmware_path[PATH_MAX_LEN];
	char *fw_buf;

	struct work_struct blind_scan_work;

	/* diseqc */
#ifdef AML_DEMOD_SUPPORT_DVBS
	struct aml_diseqc diseqc;
#endif

	unsigned int print_on;
	int tuner_strength_limit;
	unsigned int debug_on;

	unsigned int demod_thread;
	unsigned int fw_wr_done;
	unsigned int blind_min_fre;
	unsigned int blind_max_fre;
	unsigned int blind_min_srate;
	unsigned int blind_max_srate;
	unsigned int blind_fre_range;
	unsigned int blind_fre_step;
	unsigned int blind_timeout;
	unsigned int blind_scan_stop;
	unsigned int blind_debug_max_frc;
	unsigned int blind_debug_min_frc;
	unsigned int blind_same_frec;

	struct dvbsx_singlecable_parameters singlecable_param;

	bool vdac_enable;
	bool dvbc_inited;
	unsigned int peak[2048];
	unsigned int ber_base;
	unsigned int atsc_cr_step_size_dbg;
	unsigned char index;
	struct list_head demod_list;
};

/*int M6_Demod_Dtmb_Init(struct aml_fe_dev *dev);*/
int convert_snr(int in_snr);

struct amldtvdemod_device_s *dtvdemod_get_dev(void);

#if defined AML_DEMOD_SUPPORT_ISDBT || defined AML_DEMOD_SUPPORT_DVBT
static inline void __iomem *gbase_dvbt_isdbt(void)
{
	struct amldtvdemod_device_s *devp = dtvdemod_get_dev();

	return devp->reg_v[ES_MAP_ADDR_DEMOD].v + devp->data->regoff.off_dvbt_isdbt;
}
#endif

#ifdef AML_DEMOD_SUPPORT_DVBT
static inline void __iomem *gbase_dvbt_t2(void)
{
	struct amldtvdemod_device_s *devp = dtvdemod_get_dev();

	return devp->reg_v[ES_MAP_ADDR_DEMOD].v + devp->data->regoff.off_dvbt_t2;
}
#endif

#if defined AML_DEMOD_SUPPORT_DVBS || defined AML_DEMOD_SUPPORT_DVBC || \
	defined AML_DEMOD_SUPPORT_J83B
static inline void __iomem *gbase_dvbs(void)
{
	struct amldtvdemod_device_s *devp = dtvdemod_get_dev();

	return devp->reg_v[ES_MAP_ADDR_DEMOD].v + devp->data->regoff.off_dvbs;
}

static inline void __iomem *gbase_dvbc(void)
{
	struct amldtvdemod_device_s *devp = dtvdemod_get_dev();

	return devp->reg_v[ES_MAP_ADDR_DEMOD].v + devp->data->regoff.off_dvbc;
}

static inline void __iomem *gbase_dvbc_2(void)
{
	struct amldtvdemod_device_s *devp = dtvdemod_get_dev();

	return devp->reg_v[ES_MAP_ADDR_DEMOD].v + devp->data->regoff.off_dvbc_2;
}
#endif

#ifdef AML_DEMOD_SUPPORT_DTMB
static inline void __iomem *gbase_dtmb(void)
{
	struct amldtvdemod_device_s *devp = dtvdemod_get_dev();

	return devp->reg_v[ES_MAP_ADDR_DEMOD].v + devp->data->regoff.off_dtmb;
}
#endif

#if defined AML_DEMOD_SUPPORT_ATSC || defined AML_DEMOD_SUPPORT_J83B
static inline void __iomem *gbase_atsc(void)
{
	struct amldtvdemod_device_s *devp = dtvdemod_get_dev();

	return devp->reg_v[ES_MAP_ADDR_DEMOD].v + devp->data->regoff.off_atsc;
}
#endif

#ifdef AML_DEMOD_SUPPORT_ISDBT
static inline void __iomem *gbase_isdbt(void)
{
	struct amldtvdemod_device_s *devp = dtvdemod_get_dev();

	return devp->reg_v[ES_MAP_ADDR_DEMOD].v + devp->data->regoff.off_isdbt;
}
#endif

static inline void __iomem *gbase_demod(void)
{
	struct amldtvdemod_device_s *devp = dtvdemod_get_dev();

	return devp->reg_v[ES_MAP_ADDR_DEMOD].v + devp->data->regoff.off_demod_top;
}

static inline void __iomem *gbase_front(void)
{
	struct amldtvdemod_device_s *devp = dtvdemod_get_dev();

	return devp->reg_v[ES_MAP_ADDR_DEMOD].v + devp->data->regoff.off_front;
}

static inline void __iomem *gbase_ambus(void)
{
	struct amldtvdemod_device_s *devp = dtvdemod_get_dev();

	return devp->reg_v[ES_MAP_ADDR_DEMOD].v + devp->data->regoff.off_isdbt;
}

static inline void __iomem *gbase_aobus(void)
{
	struct amldtvdemod_device_s *devp = dtvdemod_get_dev();

	return devp->reg_v[ES_MAP_ADDR_AOBUS].v;
}

static inline void __iomem *gbase_iohiu(void)
{
	struct amldtvdemod_device_s *devp = dtvdemod_get_dev();

	return devp->reg_v[ES_MAP_ADDR_IOHIU].v;
}

static inline void __iomem *gbase_reset(void)
{
	struct amldtvdemod_device_s *devp = dtvdemod_get_dev();

	return devp->reg_v[ES_MAP_ADDR_RESET].v;
}

static inline void __iomem *gbase_dmc(void)
{
	struct amldtvdemod_device_s *devp = dtvdemod_get_dev();

	return devp->dmc_v_addr;
}

static inline void __iomem *gbase_ddr(void)
{
	struct amldtvdemod_device_s *devp = dtvdemod_get_dev();

	return devp->ddr_v_addr;
}

static inline unsigned int gphybase_demod(void)
{
	struct amldtvdemod_device_s *devp = dtvdemod_get_dev();

	return devp->reg_p[ES_MAP_ADDR_DEMOD].phy_addr;
}

static inline unsigned int gphybase_demodcfg(void)
{
	struct amldtvdemod_device_s *devp = dtvdemod_get_dev();

	return devp->reg_p[ES_MAP_ADDR_DEMOD].phy_addr +
			devp->data->regoff.off_demod_top;
}

static inline unsigned int gphybase_hiu(void)
{
	struct amldtvdemod_device_s *devp = dtvdemod_get_dev();

	return devp->reg_p[ES_MAP_ADDR_IOHIU].phy_addr;
}

//int amdemod_stat_islock(struct aml_dtvdemod *demod, enum fe_delivery_system delsys);

unsigned int dtmb_is_have_check(void);
const char *dtvdemod_get_cur_delsys(enum fe_delivery_system delsys);
void aml_dtv_demode_isr_en(struct amldtvdemod_device_s *devp, u32 en);
unsigned int demod_is_t5d_cpu(struct amldtvdemod_device_s *devp);
#ifdef AML_DEMOD_SUPPORT_DTMB
int dtmb_information(struct seq_file *seq);
#endif
#ifdef MODULE
struct dvb_frontend *aml_dtvdm_attach(const struct demod_config *config);
#endif

#endif
