/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _SY602X_H_
#define _SY602X_H_

//SY602X I2C-bus register addresses
#define SY602X_CLK_CTL_REG			0x00	// 1 byte
#define SY602X_DEV_ID_REG			0x01	// 1 byte
#define SY602X_ERR_STATUS_REG		0x02	// 1 byte
#define SY602X_SYS_CTL_REG1			0x03	// 1 byte
#define SY602X_SYS_CTL_REG2			0x04	// 1 byte
#define SY602X_SYS_CTL_REG3			0x05	// 1 byte
#define SY602X_SOFT_MUTE			0x06	// 1 byte
#define SY602X_MAS_VOL				0x07	// 1 byte , Master volume
#define SY602X_CH1_VOL				0x08	// 1 byte , Channel 1 vol
#define SY602X_CH2_VOL				0x09	// 1 byte , Channel 2 vol
#define SY602X_POST_SCALER			0x0A	// 1 byte , Postscaler
#define SY602X_FINE_VOL				0x0B	// 1 byte , mvol_fine_tune
#define SY602X_DC_SOFT_RESET		0x0F	// 1 byte
#define SY602X_MOD_LMT_REG			0x10	// 1 byte
#define SY602X_PWM_DLA_CH1P			0x11	// 1 byte
#define SY602X_PWM_DLA_CH1N			0x12	// 1 byte
#define SY602X_PWM_DLA_CH2P			0x13	// 1 byte
#define SY602X_PWM_DLA_CH2N			0x14	// 1 byte
#define SY602X_I2S_CONTROL			0x15	// 1 byte
#define SY602X_DSP_CTL_REG1			0x16	// 1 byte
#define SY602X_MONITOR_CFG1			0x17	// 1 byte
#define SY602X_MONITOR_CFG2			0x18	// 1 byte
#define SY602X_PWM_DC_THRESH		0x19	// 1 byte
#define SY602X_HP_CTRL				0x1A	// 1 byte
#define SY602X_OC_CTRL				0x1B	// 1 byte
#define SY602X_CHECKSUM_CTL			0x1F	// 1 byte
#define SY602X_IN_MUX_REG			0x20	// 1 byte
#define SY602X_DSP_CTL_REG2			0x21	// 1 byte
#define SY602X_PWM_CTRL				0x22	// 1 byte
#define SY602X_FAULT_SELECT			0x23	// 1 byte
#define SY602X_CH1_EQ_CTRL1			0x25	// 1 byte
#define SY602X_CH1_EQ_CTRL2			0x26	// 1 byte
#define SY602X_CH2_EQ_CTRL1			0x27	// 1 byte
#define SY602X_CH2_EQ_CTRL2			0x28	// 1 byte
#define SY602X_SPEQ_CTRL_2			0x2A	// 1 byte
#define SY602X_SPEQ_CTRL_3			0x2B	// 1 byte
#define SY602X_CH12_BQ0				0x30	// 20 byte , BQ0 coefficient
#define SY602X_CH12_BQ1				0x31	// 20 byte , BQ1 coefficient
#define SY602X_CH12_BQ2				0x32	// 20 byte , BQ2 coefficient
#define SY602X_CH12_BQ3				0x33	// 20 byte , BQ3 coefficient
#define SY602X_CH12_BQ4				0x34	// 20 byte , BQ4 coefficient
#define SY602X_CH12_BQ5				0x35	// 20 byte , BQ5 coefficient
#define SY602X_PRE_SCALER			0x36	// 4 byte , pre_scaler
#define SY602X_CH12_BQ6				0x37	// 20 byte , BQ6 coefficient
#define SY602X_CH12_BQ7				0x38	// 20 byte , BQ7 coefficient
#define SY602X_CH12_BQ8				0x39	// 20 byte , BQ8 coefficient
#define SY602X_CH12_BQ9				0x3A	// 20 byte , BQ9 coefficient
#define SY602X_CH12_BQ10			0x3B	// 20 byte , BQ10 coefficient
#define SY602X_CH12_BQ11			0x3C	// 20 byte , BQ11 coefficient
#define SY602X_CH12_BQ12			0x3D	// 20 byte , BQ12 coefficient
#define SY602X_CH12_BQ13			0x3E	// 20 byte , BQ13 coefficient
#define SY602X_CH12_COEF_SPEQ0		0x43	// 12 byte , SPEQ0 coefficient
#define SY602X_CH12_COEF_SPEQ1		0x44	// 12 byte , SPEQ1 coefficient
#define SY602X_CH12_COEF_SPEQ2		0x45	// 12 byte , SPEQ2 coefficient
#define SY602X_CH12_COEF_SPEQ3		0x46	// 12 byte , SPEQ3 coefficient
#define SY602X_LDRC_BQ0				0x47	// 20 byte , Low band DRC BQ0 coefficient
#define SY602X_LDRC_BQ1				0x48	// 20 byte , Low band DRC BQ1 coefficient
#define SY602X_MDRC_BQ0				0x49	// 20 byte , Mid band DRC BQ0 coefficient
#define SY602X_MDRC_BQ1				0x4A	// 20 byte , Mid band DRC BQ1 coefficient
#define SY602X_HDRC_BQ0				0x4B	// 20 byte , High band DRC BQ0 coefficient
#define SY602X_HDRC_BQ1				0x4C	// 20 byte , High band DRC BQ1 coefficient
#define SY602X_LOUDNESS_LR			0x4D	// 12 byte , Loudness gain
// 3 byte ,the attack coefficient of LDRC signal level envelop detection
#define SY602X_LDRC_ENVLP_TC_UP		0x4E
// 3 byte , the release coefficient of LDRC signal level envelop detection
#define SY602X_LDRC_ENVLP_TC_DN		0x4F
// 3 byte , the attack coefficient of MDRC signal level envelop detection
#define SY602X_MDRC_ENVLP_TC_UP		0x50
// 3 byte , the release coefficient of MDRC signal level envelop detection
#define SY602X_MDRC_ENVLP_TC_DN		0x51
// 3 byte , the attack coefficient of HDRC signal level envelop detection
#define SY602X_HDRC_ENVLP_TC_UP		0x52
// 3 byte , the release coefficient of HDRC signal level envelop detection
#define SY602X_HDRC_ENVLP_TC_DN		0x53
#define SY602X_PWM_MUX_REG			0x54	// 4 byte , PWM MUX register
#define SY602X_PWM_OUTFLIP			0x55	// 4 byte , PWM Outflip register
#define SY602X_PWM_CTL_REG			0x56	// 4 byte , PWM Control register
// 6 byte , power meter envelop detection attack and release coefficient
#define SY602X_PM_COEF				0x57
#define SY602X_PM_CTL_RB1			0x58	// 3 byte , power meter control_1
#define SY602X_PM_CTL_RB2			0x59	// 3 byte , power meter control_2
#define SY602X_PBQ_CHECKSUM			0x5A	// 4 byte
#define SY602X_DRC_CHECKSUM			0x5B	// 4 byte
#define SY602X_SPEQ_ATK_REL_TC_1	0x5D	// 12 byte , speq_atk_rel_tc_1
#define SY602X_SPEQ_ATK_REL_TC_2	0x5E	// 12 byte , speq_atk_rel_tc_2
#define SY602X_MIXER_CTRL			0x5F	// 4 byte , ch12 mixer gain
#define SY602X_DRC_CTRL				0x60	// 4 byte , DRC control
#define SY602X_LDRC_LMT_CFG1		0x61	// 3 byte , LDRC makeup gain and threshold
#define SY602X_LDRC_LMT_CFG2		0x62	// 3 byte , LDRC attack time control
#define SY602X_LDRC_LMT_CFG3		0x63	// 3 byte , LDRC release time control
#define SY602X_MDRC_LMT_CFG1		0x64	// 3 byte , MDRC makeup gain and threshold
#define SY602X_MDRC_LMT_CFG2		0x65	// 3 byte , MDRC attack time control
#define SY602X_MDRC_LMT_CFG3		0x66	// 3 byte , LDRC release time control
#define SY602X_HDRC_LMT_CFG1		0x67	// 3 byte , HDRC makeup gain and threshold
#define SY602X_HDRC_LMT_CFG2		0x68	// 3 byte , HDRC attack time control
#define SY602X_HDRC_LMT_CFG3		0x69	// 3 byte , LDRC release time control
#define SY602X_PDRC_LMT_CFG1		0x6A	// 3 byte , PDRC threshold
#define SY602X_PDRC_LMT_CFG2		0x6B	// 3 byte , PDRC attack time control
#define SY602X_PDRC_LMT_CFG3		0x6C	// 3 byte , PDRC release time control
// 3 byte , the attack coefficient of PDRC signal level envelop detection
#define SY602X_PDRC_ENVLP_TC_UP		0x6D
// 3 byte , the release coefficient of PDRC signal level envelop detection
#define SY602X_PDRC_ENVLP_TC_DN		0x6E
#define SY602X_PROT_SYS_CNTL		0x76	// 1 byte
#define SY602X_INTER_PRIVATE		0x82	// 4 byte
#define SY602X_OCDET_WIND_WIDTH		0x85	// 4 byte
#define SY602X_FAULT_OC_TH			0x86	// 4 byte
#define SY602X_DSP_CTL3				0x8B	// 1 byte
#define SY602X_FUNC_DEBUG			0x8C	// 1 byte

#define SY602X_MAX_REGISTER            0x8F

// 0x03: SY602X_SYS_CTL_REG1 : 1 byte
#define SY602X_I2C_ACCESS_RAM_MASK     BIT(0)
#define SY602X_I2C_ACCESS_RAM_ENABLE   BIT(0)
#define SY602X_I2C_ACCESS_RAM_DISABLE  0

// 0x04: SY602X_SYS_CTL_REG2 : 1 byte
#define SY602X_EQ_ENABLE_SHIFT         (4)

// 0x06: SY602X_SOFT_MUTE    : 1 byte
#define SY602X_SOFT_MUTE_ALL           (BIT(3) | BIT(1) | BIT(0))
#define SY602X_SOFT_MUTE_MASTER_SHIFT  (3)
#define SY602X_SOFT_MUTE_CH1_SHIFT     (0)
#define SY602X_SOFT_MUTE_CH2_SHIFT     (1)

// 0x60: SY602X_DRC_CTRL     : 4 byte
#define SY602X_DRC_EN_MASK             (BIT(3) | BIT(2) | BIT(1) | BIT(0))
#define SY602X_DRC_ENABLE              (BIT(3) | BIT(2) | BIT(1) | BIT(0))
#define SY602X_DRC_DISABLE             0
#endif  /* _SY602X_H_ */
