/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _HDMI_RX_TXHD2_H
#define _HDMI_RX_TXHD2_H

/* T5M PHY register */
#define TXHD2_HDMIRX20PLL_CTRL0			(0x000 << 2)
#define TXHD2_HDMIRX20PLL_CTRL1			(0x001 << 2)
#define TXHD2_HDMIRX20PHY_DCHA_AFE		(0x002 << 2)
	#define TXHD2_LEQ_HYPER_GAIN_CH0		_BIT(3)
	#define TXHD2_LEQ_HYPER_GAIN_CH1		_BIT(7)
	#define TXHD2_LEQ_HYPER_GAIN_CH2		_BIT(11)
	#define TXHD2_LEQ_BUF_GAIN            MSK(3, 16)
	#define TXHD2_LEQ_POLE                MSK(3, 12)
#define TXHD2_HDMIRX20PHY_DCHA_DFE		(0x003 << 2)
	#define TXHD2_SLICER_OFSTCAL_START	_BIT(13)
#define TXHD2_HDMIRX20PHY_DCHD_CDR		(0x004 << 2)
	#define TXHD2_EHM_DBG_SEL			_BIT(31)
	#define TXHD2_OFSET_CAL_START		_BIT(27)
	#define TXHD2_CDR_LKDET_EN		_BIT(14)
	#define TXHD2_CDR_RSTB			_BIT(13)
	#define TXHD2_CDR_EN              _BIT(12)
	#define TXHD2_CDR_FR_EN				_BIT(6)
	#define TXHD2_MUX_CDR_DBG_SEL		_BIT(19)
	#define TXHD2_CDR_OS_RATE			MSK(2, 8)
	#define TXHD2_DFE_OFST_DBG_SEL		MSK(3, 28)
	#define TXHD2_ERROR_CNT			0X0
	#define TXHD2_SCAN_STATE			0X1
	#define TXHD2_POSITIVE_EYE_HEIGHT		0x2
	#define TXHD2_NEGATIVE_EYE_HEIGHT		0x3
	#define TXHD2_LEFT_EYE_WIDTH		0x4
	#define TXHD2_RIGHT_EYE_WIDTH		0x5
#define TXHD2_HDMIRX20PHY_DCHD_EQ			(0x005 << 2)
	#define TXHD2_BYP_TAP0_EN			_BIT(30)
	#define TXHD2_BYP_TAP_EN			_BIT(19)
	#define TXHD2_DFE_HOLD_EN			_BIT(18)
	#define TXHD2_DFE_RSTB			_BIT(17)
	#define TXHD2_DFE_EN			_BIT(16)
	#define TXHD2_EHM_SW_SCAN_EN		_BIT(15)
	#define TXHD2_EHM_HW_SCAN_EN		_BIT(14)
	#define TXHD2_EQ_RSTB			_BIT(13)
	#define TXHD2_EQ_EN			_BIT(12)
	#define TXHD2_EN_BYP_EQ			_BIT(5)
	#define TXHD2_BYP_EQ			MSK(5, 0)
	#define TXHD2_EQ_MODE			MSK(2, 8)
	#define TXHD2_STATUS_MUX_SEL		MSK(2, 22)
#define TXHD2_HDMIRX20PHY_DCHA_MISC1		(0x006 << 2)
	#define TXHD2_SQ_RSTN			_BIT(26)
	#define TXHD2_VCO_TMDS_EN			_BIT(20)
	#define TXHD2_RTERM_CNTL			MSK(4, 12)
#define TXHD2_HDMIRX20PHY_DCHA_MISC2		(0x007 << 2)
	#define TXHD2_TMDS_VALID_SEL		_BIT(10)
	#define TXHD2_PLL_CLK_SEL			_BIT(9)
#define TXHD2_HDMIRX20PHY_DCHD_STAT       (0x009 << 2)
#define TXHD2_HDMIRX_EARCTX_CNTL0         (0x040 << 2)
#define TXHD2_HDMIRX_EARCTX_CNTL1         (0x041 << 2)
#define TXHD2_HDMIRX_ARC_CNTL             (0x042 << 2)
#define TXHD2_HDMIRX_PHY_PROD_TEST0       (0x080 << 2)
#define TXHD2_HDMIRX_PHY_PROD_TEST1       (0x081 << 2)

#define TXHD2_RG_RX20PLL_0		0x000
#define TXHD2_RG_RX20PLL_1		0x004

extern int tapx_value;
extern int agc_enable;
extern u32 afe_value;
extern u32 dfe_value;
extern u32 cdr_value;
extern u32 eq_value;
extern u32 misc2_value;
extern u32 misc1_value;
/*--------------------------function declare------------------*/
/* T5m */
void aml_phy_init_txhd2(void);
u32 aml_eq_eye_monitor_txhd2(int ch_monitor);
void dump_reg_phy_txhd2(void);
void dump_aml_phy_sts_txhd2(void);
void aml_phy_short_bist_txhd2(void);
bool aml_get_tmds_valid_txhd2(void);
void aml_phy_power_off_txhd2(void);
void aml_phy_switch_port_txhd2(void);
void aml_phy_iq_skew_monitor_txhd2(void);
void dump_vsi_reg_txhd2(u8 port);
unsigned int rx_sec_hdcp_cfg_txhd2(void);
void rx_set_aud_output_txhd2(u32 param);
void rx_sw_reset_txhd2(int level);
void hdcp_init_txhd2(void);
void aml_phy_get_trim_val_txhd2(void);
void comb_val_txhd2(void (*p)(char *, unsigned int, int),
	     char *type, unsigned int val_0, unsigned int val_1,
		 unsigned int val_2, int len);
void get_flag_val_txhd2(char *temp, unsigned int val, int len);
void get_val_txhd2(char *temp, unsigned int val, int len);
void get_eq_val_txhd2(void);
void aml_phy_offset_cal_txhd2(void);

void bubble_sort(u32 *sort_array);
void quick_sort2_txhd2(int arr[], int l, int r);
void txhd2_pbist(void);
void clk_init_cor_txhd2(void);
void rx_dig_clk_en_txhd2(bool en);
void txhd2_aud_clk_cal(void);

/*function declare end*/

#endif /*_HDMI_RX_TXHD2_H*/

