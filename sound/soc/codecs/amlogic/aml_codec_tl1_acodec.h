/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _TL1_ACODEC_H
#define _TL1_ACODEC_H

#define DEV_NAME "tl1_acodec"

/* AML TL1 CODEC register space (in decimal to match datasheet) */
//#define ACODEC_BASE_ADD    0xFF632000
#define ACODEC_TOP_ADDR(x) (x)

/* AML TL1 CODEC register define */
#define ACODEC_0   ACODEC_TOP_ADDR(0x00)
#define ACODEC_1   ACODEC_TOP_ADDR(0x04)
#define ACODEC_2   ACODEC_TOP_ADDR(0x08)
#define ACODEC_3   ACODEC_TOP_ADDR(0x0c)
#define ACODEC_4   ACODEC_TOP_ADDR(0x10)
#define ACODEC_5   ACODEC_TOP_ADDR(0x14)
#define ACODEC_6   ACODEC_TOP_ADDR(0x18)
#define ACODEC_7   ACODEC_TOP_ADDR(0x1C)
#define ACODEC_8   ACODEC_TOP_ADDR(0x20)

/* ACODEC_0 ~ ACODEC_8 */
#define ACODEC_REG_NUM              9

/* AML TL1 CODEC register-bitfield define */

// bitfield def of ACODEC_0
#define MCLK_FREQ                   31
#define I2S_MODE                    30
#define ADC_HPF_EN                  29
#define ADC_HPF_MODE                28
#define ADC_OVERLOAD_DET_EN         27
#define ADC_DEM_EN                  26
#define ADC_CLK_TO_GPIO_EN          25
#define DAC_CLK_TO_GPIO_EN          24
#define DACL_DATA_SOURCE            23
#define DACR_DATA_SOURCE            22
#define DACL_INV                    21
#define DACR_INV                    20
#define ADCDATL_SOURCE              19
#define ADCDATR_SOURCE              18
#define ADCL_INV                    17
#define ADCR_INV                    16
#define VMID_GEN_EN                 15
#define VMID_GEN_FAST               14
#define BIAS_CURRENT_EN             13
#define REFP_BUF_EN                 12
#define PGAL_IN_EN                  11
#define PGAR_IN_EN                  10
#define PGAL_IN_ZC_EN               9
#define PGAR_IN_ZC_EN               8
#define ADCL_EN                     7
#define ADCR_EN                     6
//#define DACL_EN                     5
//#define DACR_EN                     4
#define LO1L_EN                     3
#define LO1R_EN                     2
#define LO2L_EN                     1
#define LO2R_EN                     0

// bitfield def of ACODEC_1
#define REG_DAC_GAIN_SEL_1          31
#define ADCL_VC                     24 /* bit 30-24 */
#define REG_DAC_GAIN_SEL_0			23
#define ADCR_VC						16 /* bit 22-16 */
#define PGAL_IN_SEL					13 /* bit 15-13 */
#define PGAL_IN_GAIN                8  /* bit 12-8 */
#define PGAR_IN_SEL                 5  /* bit 7-5 */
#define PGAR_IN_GAIN                0  /* bit 4-0 */

// bitfield def of  ACODEC_2
#define DACL_VC                     24 /* bit 31-24 */
#define DACR_VC                     16 /* bit 23-16 */
#define DAC_SOFT_MUTE               15
#define DAC_UNMUTE_MODE             14
#define DAC_MUTE_MODE               13
#define DAC_VC_RAMP_MODE            12
#define DAC_RAMP_RATE               10 /* bit 11-10 */
#define DAC_MONO                    8
#define MUTE_DAC_PD_EN				7

// bitfield def of  ACODEC_3
#define REG_MICBIAS_EN				31
#define REG_MICBIAS_SEL				29 /* bit 29, 30 */
//#define REG_ANA_RESERVED			16 /* bit 16 ~ 28 */
#define LO1L_SEL_DAC1R_INV			14
#define LO1L_SEL_DAC1L				13
#define LO1L_SEL_INL				12
#define LO1R_SEL_DAC1L_INV			10
#define LO1R_SEL_DAC1R				9
#define LO1R_SEL_INR				8
#define LO2L_SEL_DAC2R_INV		    6
#define LO2L_SEL_DAC2L				5
#define LO2L_SEL_INL			    4
#define LO2R_SEL_DAC2L_INV			2
#define LO2R_SEL_DAC2R				1
#define LO2R_SEL_INR				0

// bitfield def of ACODEC_4
#define MUTE_DAC_WHEN_POWER_DOWN    31
#define IB_CON                      16 /* bit 16, 17 */
#define REG_ADCL_SAT_SEL			2  /* bit 2, 3 */
#define REG_ADCR_SAT_SEL			0  /* bit 0, 1 */

// bitfield def of ACODEC_5
#define DAC2L_VC					24 /* bit 24~31 */
#define DAC2R_VC					16 /* bit 16~23 */
#define DAC2L_EN					5
#define DAC2R_EN					4
#define DACL_EN					    1
#define DACR_EN					    0

// bitfield def of ACODEC_6
#define DAC2_SOFT_MUTE				31
#define DAC2_UNMUTE_MODE			30
#define DAC2_MUTE_MODE				29
#define DAC2_VC_RAMP_MODE			28
#define DAC2_RAMP_RATE				26 /* bit 27-26 */
#define DAC2_MONO					24
#define MUTE_DAC2_PD_EN				23
#define DAC2_CLK_TO_GPIO_EN			8
#define DAC2L_DATA_SOURCE			7
#define DAC2R_DATA_SOURCE			6
#define DAC2L_INV					5
#define DAC2R_INV					4

// bitfield def of ACODEC_7
#define DEBUG_BUS_SEL				16 /* bit 16~18 */
#define REG_DAC2_GAIN_SEL_1			15
#define REG_DAC2_GAIN_SEL_0			7

#endif /*_TL1_ACODEC_H*/
