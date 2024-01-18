/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2021 Amlogic, Inc. All rights reserved.
 */

#ifndef _HDMI_RX_T3X_H
#define _HDMI_RX_T3X_H

/* 1st 2.0 PHY base addr: 0xfe39c000 */
/* 2st 2.0 PHY base addr: 0xfe39c400 */
/* 1st 2.1 PHY base addr: 0xfe39c800 */
/* 2st 2.0 PHY base addr: 0xfe39cc00 */

/* T3X HDMI2.0 PHY register */
#define T3X_HDMIRX20PLL_CTRL0			(0x000 << 2)
#define T3X_HDMIRX20PLL_CTRL1			(0x001 << 2)
#define T3X_HDMIRX20PHY_DCHA_AFE		(0x002 << 2)
	#define T3X_20_LEQ_HYPER_GAIN_CH0		_BIT(3)
	#define T3X_20_LEQ_HYPER_GAIN_CH1		_BIT(7)
	#define T3X_20_LEQ_HYPER_GAIN_CH2		_BIT(11)
	#define T3X_20_LEQ_BUF_GAIN            MSK(3, 16)
	#define T3X_20_LEQ_POLE                MSK(3, 12)
#define T3X_HDMIRX20PHY_DCHA_DFE		(0x003 << 2)
	#define T3X_20_SLICER_OFSTCAL_START	_BIT(13)
#define T3X_HDMIRX20PHY_DCHD_CDR		(0x004 << 2)
	#define T3X_20_EHM_DBG_SEL			_BIT(31)
	#define T3X_20_OFSET_CAL_START		_BIT(27)
	#define T3X_20_CDR_LKDET_EN		_BIT(14)
	#define T3X_20_CDR_RSTB			_BIT(13)
	#define T3X_20_CDR_EN              _BIT(12)
	#define T3X_20_CDR_FR_EN				_BIT(6)
	#define T3X_20_MUX_CDR_DBG_SEL		_BIT(19)
	#define T3X_20_CDR_OS_RATE			MSK(2, 8)
	#define T3X_20_DFE_OFST_DBG_SEL		MSK(3, 28)
	#define T3X_20_ERROR_CNT			0X0
	#define T3X_20_20_SCAN_STATE			0X1
	#define T3X_20_POSITIVE_EYE_HEIGHT		0x2
	#define T3X_20_NEGATIVE_EYE_HEIGHT		0x3
	#define T3X_20_LEFT_EYE_WIDTH		0x4
	#define T3X_20_RIGHT_EYE_WIDTH		0x5
#define T3X_HDMIRX20PHY_DCHD_EQ			(0x005 << 2)
	#define T3X_20_BYP_TAP0_EN			_BIT(30)
	#define T3X_20_BYP_TAP_EN			_BIT(19)
	#define T3X_20_DFE_HOLD_EN			_BIT(18)
	#define T3X_20_DFE_RSTB			_BIT(17)
	#define T3X_20_DFE_EN			_BIT(16)
	#define T3X_20_EHM_SW_SCAN_EN		_BIT(15)
	#define T3X_20_EHM_HW_SCAN_EN		_BIT(14)
	#define T3X_20_EQ_RSTB			_BIT(13)
	#define T3X_20_EQ_EN			_BIT(12)
	#define T3X_20_EN_BYP_EQ			_BIT(5)
	#define T3X_20_BYP_EQ			MSK(5, 0)
	#define T3X_20_EQ_MODE			MSK(2, 8)
	#define T3X_20_STATUS_MUX_SEL		MSK(2, 22)
#define T3X_HDMIRX20PHY_DCHA_MISC1		(0x006 << 2)
	#define T3X_20_SQ_RSTN			_BIT(26)
	#define T3X_20_VCO_TMDS_EN			_BIT(20)
	#define T3X_20_RTERM_CNTL			MSK(4, 12)
#define T3X_HDMIRX20PHY_DCHA_MISC2		(0x007 << 2)
	#define T3X_20_TMDS_VALID_SEL		_BIT(10)
	#define T3X_20_PLL_CLK_SEL			_BIT(9)
#define T3X_HDMIRX20PHY_DCHD_STAT       (0x009 << 2)
//#define T3X_HDMIRX_EARCTX_CNTL0         (0x040 << 2)
//#define T3X_HDMIRX_EARCTX_CNTL1         (0x041 << 2)
#define T3X_HDMIRX_PHY_PROD_TEST0       (0x020 << 2)
#define T3X_HDMIRX_PHY_PROD_TEST1       (0x021 << 2)

#define T3X_RG_RX20PLL_0		0x000
#define T3X_RG_RX20PLL_1		0x004

/* T3X HDMI2.1 PHY register */
#define T3X_HDMIRX21PLL_CTRL0           (0x40 << 2)
#define T3X_HDMIRX21PLL_CTRL1           (0x41 << 2)
#define T3X_HDMIRX21PLL_CTRL2           (0x42 << 2)
#define T3X_HDMIRX21PLL_CTRL3           (0x43 << 2)
#define T3X_HDMIRX21PLL_CTRL4           (0x44 << 2)
#define T3X_HDMIRX21PHY_MISC0           (0x45 << 2)
	#define DCH_RSTN	MSK(4, 0)
	#define CCH_EN		MSK(2, 16)
#define T3X_HDMIRX21PHY_MISC1           (0x46 << 2)
	#define SQ_RSTN	_BIT(23)
	#define SQ_GATED	_BIT(29)
#define T3X_HDMIRX21PHY_MISC2           (0x47 << 2)
#define T3X_HDMIRX21PHY_DCHA_AFE        (0x48 << 2)
	#define HYPER_GAIN	MSK(4, 16)
	#define BUF_BST		MSK(3, 28)
	#define LEQ_POLE	MSK(3, 20)
	#define BUF_GAIN	MSK(3, 24)
#define T3X_HDMIRX21PHY_DCHA_DFE        (0x49 << 2)
	#define VGA_GAIN	MSK(16, 0)
	#define DFE_TAP_EN	MSK(9, 16)
	#define DFE_OFST_CAL_START _BIT(27)
	#define DFE_H1_PD	_BIT(29)
#define T3X_HDMIRX21PHY_DCHA_PI         (0x4a << 2)
#define T3X_HDMIRX21PHY_DCHA_CTRL       (0x4b << 2)
#define T3X_HDMIRX21PHY_DCHD_CDR        (0x4c << 2)
	#define CDR_FR_EN	_BIT(6)
	#define CDR_OS_RATE	MSK(2, 8)
	#define CDR_PI_DIV	MSK(2, 10)
	#define CDR_RSTB	_BIT(13)
	#define CDR_LKDET_EN	_BIT(14)
	#define MUX_CDR_DBG_SEL	_BIT(19)
	#define OFST_CAL_START _BIT(27)
	#define MUX_DFE_OFST_EYE	MSK(3, 28)
	#define MUX_EYE_EN		_BIT(31)
#define T3X_HDMIRX21PHY_DCHD_EQ         (0x4d << 2)
	#define EQ_BYP_VAL1	MSK(5, 0)
	#define EQ_BYP_EN	_BIT(5)
	#define EQ_MODE	MSK(2, 8)
	#define EQ_RSTB	_BIT(13)
	#define DFE_RSTB _BIT(17)
	#define DFE_HOLD _BIT(18)
	#define DFE_TAPS_DISABLE	_BIT(19)
	#define MUX_BLOCK_SEL		MSK(2, 22)
#define T3X_HDMIRX21PHY_DCH_STAT        (0x4e << 2)
#define T3X_HDMIRX21PHY_TEST			(0x4f << 2)
#define T3X_HDMIRX21PHY_PROD_TEST0      (0x60 << 2)
#define T3X_HDMIRX21PHY_PROD_TEST1      (0x61 << 2)
/* T3X HDMI2.1 PHY FPLL register */
#define T3X_CLKCTRL_HDMI_PLL0_CTRL0		(0x02e0  << 2)
#define T3X_CLKCTRL_HDMI_PLL0_CTRL1		(0x02e1  << 2)
#define T3X_CLKCTRL_HDMI_PLL0_CTRL2		(0x02e2  << 2)
#define T3X_CLKCTRL_HDMI_PLL0_CTRL3		(0x02e3  << 2)
#define T3X_CLKCTRL_HDMI_PLL0_STS		(0x02e4  << 2)
#define T3X_CLKCTRL_HDMI_PLL1_CTRL0		(0x02e5  << 2)
#define T3X_CLKCTRL_HDMI_PLL1_CTRL1		(0x02e6  << 2)
#define T3X_CLKCTRL_HDMI_PLL1_CTRL2		(0x02e7  << 2)
#define T3X_CLKCTRL_HDMI_PLL1_CTRL3		(0x02e8  << 2)
#define T3X_CLKCTRL_HDMI_PLL1_STS		(0x02e9  << 2)

#define T3X_CLKCTRL_AUD21_PLL_CTRL0		(0x02ea  << 2)
#define T3X_CLKCTRL_AUD21_PLL_CTRL1		(0x02eb  << 2)
#define T3X_CLKCTRL_AUD21_PLL_CTRL2		(0x02ec  << 2)
#define T3X_CLKCTRL_AUD21_PLL_CTRL3		(0x02ed  << 2)
#define T3X_CLKCTRL_AUD21_PLL_STS		(0x02ee  << 2)

/* i2c monitor reg */
#define T3X_I2C_MONITOR_BASE			0xfe014000
#define T3X_I2C_MONITOR_SMP_START		(0x14000 + (0x000 << 2))
#define T3X_I2C_MONITOR_SMP_SEL			(0x14000 + (0x001 << 2))
#define T3X_I2C_MONITOR_SMP_CNTL		(0x14000 + (0x002 << 2))
#define T3X_I2C_MONITOR_SMP_FLT			(0x14000 + (0x003 << 2))
#define T3X_I2C_MONITOR_SMP_FLT_HPD		(0x14000 + (0x004 << 2))
#define T3X_I2C_MONITOR_SMP_I2C_TIMEOUT_TH	(0x14000 + (0x005 << 2))
#define T3X_I2C_MONITOR_SMP_CLK			(0x14000 + (0x006 << 2))
#define T3X_I2C_MONITOR_DDR_START_ADDR		(0x14000 + (0x007 << 2))
#define T3X_I2C_MONITOR_DDR_END_ADDR		(0x14000 + (0x008 << 2))
#define T3X_I2C_MONITOR_DDR_CNTL		(0x14000 + (0x009 << 2))
#define T3X_I2C_MONITOR_INTR_MASK		(0x14000 + (0x00a << 2))
#define T3X_I2C_MONITOR_DDR_WPTR		(0x14000 + (0x00b << 2))
#define T3X_I2C_MONITOR_DDR_BOUND_CNT		(0x14000 + (0x00c << 2))
#define T3X_I2C_MONITOR_AXI_CMD_PENDING		(0x14000 + (0x00d << 2))
#define T3X_I2C_MONITOR_SMP_STATUS		(0x14000 + (0x00e << 2))
#define T3X_I2C_MONITOR_SMP_I2C_BUSY_CNT	(0x14000 + (0x00f << 2))
#define T3X_I2C_MONITOR_AXI_STATUS		(0x14000 + (0x010 << 2))
#define T3X_I2C_MONITOR_INTR_STATUS		(0x14000 + (0x011 << 2))
#define T3X_I2C_MONITOR_AXI_CMD_CNT		(0x14000 + (0x012 << 2))

enum i2c_sample_mode_e {
	E_FUNC_SAMPLE,
	E_I2C_WAVE_SAMPLE,
	E_CEC_WAVE_SAMPLE,
	E_BIST_MODE
};

enum i2c_trigger_mode_e {
	E_HW_TRIGGER,
	E_SW_TRIGGER
};

enum i2c_dump_mode_e {
	E_ABNORMAL_START0 = 0x1,
	E_ABNORMAL_START1 = 0x2,
	E_ABNORMAL_STOP = 0x4,
	E_I2C_TIMEOUT = 0x8,
	E_HPD_CHANGE = 0x10,
	E_DUMP_ALL = 0x1f
};

enum i2c_data_type_e {
	E_DATA,
	E_START_DATA,
	E_STOP_DATA,
	E_START_DATA_STOP,
	E_STOP_ABNORMAL,
	E_START_ABNORMAL,
	E_TIME_OUT
};

enum frl_train_sts_e {
	E_FRL_TRAIN_START,
	E_FRL_TRAIN_FINISH,
	E_FRL_TRAIN_PASS,
	E_FRL_TRAIN_FAIL
};

#define PWRCTRL_MEM_PD19	((0x0023 << 2) + 0xfe00c000)
#define PWRCTRL_MEM_PD20	((0x0024 << 2) + 0xfe00c000)
#define PWRCTRL_MEM_PD21	((0x0025 << 2) + 0xfe00c000)

extern int pll_level_en;
extern int pll_level;
extern int vpcore_debug;
extern int phy_rate;
extern u32 odn_reg_n_mul;
extern u32 ext_cnt;
extern int tr_delay0;
extern int tr_delay1;
extern int fpll_sel;
extern int fpll_chk_lvl;
extern int dts_debug_flag_t3x_21;
extern int rlevel_t3x_21;
extern int rterm_trim_val_t3x_21;
extern int rterm_trim_flag_t3x_21;
extern int phy_term_lel_t3x_21;
extern int tuning_cnt;
extern int vga_tuning_min;
extern int vga_tuning_max;
extern int cal_phy_time;

/*--------------------------function declare------------------*/
/* T3X */
void aml_phy_init_t3x(u8 port);
void aml_eq_eye_monitor_t3x(u8 port);
void dump_reg_phy_t3x(u8 port);
void dump_aml_phy_sts_t3x(u8 port);
void aml_phy_short_bist_t3x(u8 port);
bool aml_get_tmds_valid_t3x(u8 port);
void aml_phy_power_off_t3x(u8 port);
void aml_phy_switch_port_t3x(u8 port);
void aml_phy_iq_skew_monitor_t3x(void);
void get_val_t3x(char *temp, unsigned int val, int len);
unsigned int rx_sec_hdcp_cfg_t3x(void);
void rx_set_irq_t3x(bool en, u8 port);
void rx_set_aud_output_t3x(u32 param, u8 port);
void rx_sw_reset_t3x(int level, u8 port);
void hdcp_init_t3x(u8 port);
void aml_phy_get_trim_val_t3x(void);
void comb_val_t3x(void (*p)(char *, unsigned int, int),
	     char *type, unsigned int val_0, unsigned int val_1,
		 unsigned int val_2, int len);
void get_flag_val_t3x(char *temp, unsigned int val, int len);
void aml_phy_offset_cal_t3x(void);
void quick_sort2_t3x_20(int arr[], int l, int r);
void rx_pwrcntl_mem_pd_cfg(void);
void rx_frl_train(u8 port);
void rx_frl_train_handler(struct kthread_work *work);
void rx_frl_train_handler_1(struct kthread_work *work);
enum frl_train_sts_e rx_get_frl_train_sts(u8 port);
void rx_set_frl_train_sts(enum frl_train_sts_e sts, u8 port);
enum frl_rate_e hdmirx_get_frl_rate(u8 port);
bool is_frl_train_finished(u8 port);
void rx_long_bist_t3x(void);
void rx_t3x_prbs(void);
void dump_aud21_param(u8 port);
void rx_21_fpll_cfg(int f_rate, u8 port);
bool is_fpll_err(u8 port);
void audio_setting_for_aud21(int frl_rate, u8 port);
void clk_init_cor_t3x(void);
void rx_dig_clk_en_t3x(bool en);
void rx_lts_2_flt_ready(u8 port);
void RX_LTS_3_LTP_REQ_SEND_1111(u8 port);
void RX_LTS_3_LTP_REQ_SEND_0000(u8 port);
void hal_flt_update_set(u8 port);
int rx_lts_p_syn_detect(u8 frl_rate, u8 port);
void aml_phy_init_t3x_21(u8 port);
void rx_lts_3_err_detect(u8 port);
void RX_LTS_P_FRL_START(u8 port);
bool s_tmds_transmission_detected(u8 port);
bool hdmirx_flt_update_cleared_wait(u32 addr, u8 port);
void hdmirx_vga_gain_tuning(u8 port);
void rx_set_term_value_t3x(unsigned char port, bool value);
void aml_phy_power_off_t3x_20(u8 port);
void aml_phy_power_off_t3x_21(u8 port);
void rx_cor_reset_t3x(u8 port);
void cor_debug_t3x(u8 port);
void clr_frl_fifo_status(u8 port);
void rx_rcc_err_frl_config(u8 port);
void rx_read_ecc_err(u8 port);
bool is_fsm_ready_t3x(void);
void rx_switch_to_self_hsync(u8 port, bool en);
bool rx_is_switch_to_analog_clk(u8 port);
void rx_switch_to_analog_clk(u8 port);
void rx_clr_f_det(bool en, u8 port);
bool rx_get_clkready_sts(u8 port);
bool rx_get_valid_m_sts(u8 port);

void rx_i2c_dbg_monitor(void);
void rx_i2c_monitor(u8 sel, u8 smp_mod, u8 trig_mod, u8 dump_mod);
void rx_i2c_dump(void);
bool rx_is_power_off_t3x(u8 port);

//void reset_pcs(void);

/*function declare end*/

#endif /*_HDMI_RX_T3X_H*/

