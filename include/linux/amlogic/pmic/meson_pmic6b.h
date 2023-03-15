/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * include/linux/amlogic/pmic/meson_pmic6b.h
 *
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#ifndef _MESON_PMIC6B_H
#define	_MESON_PMIC6B_H

#include <linux/interrupt.h>

/*Power Up and Down Slot Register*/
#define	PMIC6B_POWER_UP_SEL0		0x00
#define	PMIC6B_POWER_UP_SEL1		0x01
#define	PMIC6B_POWER_UP_SEL2		0x02
#define	PMIC6B_POWER_UP_SEL3		0x03

#define	PMIC6B_OTP_REG_0x04			0x04
#define	PMIC6B_OTP_REG_0x05			0x05
#define	PMIC6B_OTP_REG_0x06			0x06
#define	PMIC6B_OTP_REG_0x07			0x07

#define	PMIC6B_OTP_REG_0x08			0x08
#define	PMIC6B_OTP_REG_0x09			0x09
#define	PMIC6B_OTP_REG_0x0A			0x0A
#define	PMIC6B_OTP_REG_0x0B			0x0B
#define	PMIC6B_OTP_REG_0x0C			0x0C
#define	PMIC6B_OTP_REG_0x0D			0x0D
#define	PMIC6B_OTP_REG_0x0E			0x0E
#define	PMIC6B_OTP_REG_0x0F			0x0F

#define	PMIC6B_OTP_REG_0x10			0x10
#define	PMIC6B_OTP_REG_0x11			0x11
#define	PMIC6B_OTP_REG_0x12			0x12
#define	PMIC6B_OTP_REG_0x13			0x13
#define	PMIC6B_OTP_REG_0x14			0x14
#define	PMIC6B_OTP_REG_0x15			0x15
#define	PMIC6B_OTP_REG_0x16			0x16
#define	PMIC6B_OTP_REG_0x17			0x17
#define	PMIC6B_OTP_REG_0x18			0x18
#define	PMIC6B_OTP_REG_0x19			0x19
#define	PMIC6B_OTP_REG_0x1A			0x1A
#define	PMIC6B_OTP_REG_0x1B			0x1B
#define	PMIC6B_OTP_REG_0x1C			0x1C
#define	PMIC6B_OTP_REG_0x1D			0x1D
#define	PMIC6B_OTP_REG_0x1E			0x1E
#define	PMIC6B_OTP_REG_0x1F			0x1F

#define	PMIC6B_OTP_REG_0x20			0x20
#define	PMIC6B_OTP_REG_0x21			0x21
#define	PMIC6B_OTP_REG_0x22			0x22
#define	PMIC6B_OTP_REG_0x23			0x23
#define	PMIC6B_OTP_REG_0x24			0x24
#define	PMIC6B_OTP_REG_0x25			0x25
#define	PMIC6B_OTP_REG_0x26			0x26
#define	PMIC6B_OTP_REG_0x27			0x27
#define	PMIC6B_OTP_REG_0x28			0x28
#define	PMIC6B_OTP_REG_0x29			0x29
#define	PMIC6B_OTP_REG_0x2A			0x2A
#define	PMIC6B_OTP_REG_0x2B			0x2B
#define	PMIC6B_OTP_REG_0x2C			0x2C
#define	PMIC6B_OTP_REG_0x2D			0x2D
#define	PMIC6B_OTP_REG_0x2E			0x2E
#define	PMIC6B_OTP_REG_0x2F			0x2F

#define	PMIC6B_OTP_REG_0x30			0x30
#define	PMIC6B_OTP_REG_0x31			0x31
#define	PMIC6B_OTP_REG_0x32			0x32
#define	PMIC6B_OTP_REG_0x33			0x33
#define	PMIC6B_OTP_REG_0x34			0x34
#define	PMIC6B_OTP_REG_0x35			0x35
#define	PMIC6B_OTP_REG_0x36			0x36
#define	PMIC6B_OTP_REG_0x37			0x37
#define	PMIC6B_OTP_REG_0x38			0x38
#define	PMIC6B_OTP_REG_0x39			0x39
#define	PMIC6B_OTP_REG_0x3A			0x3A
#define	PMIC6B_OTP_REG_0x3B			0x3B
#define	PMIC6B_OTP_REG_0x3C			0x3C
#define	PMIC6B_OTP_REG_0x3D			0x3D
#define	PMIC6B_OTP_REG_0x3E			0x3E
#define	PMIC6B_OTP_REG_0x3F			0x3F

/* NON-OTP Registers */
#define	PMIC6B_GEN_CNTL0			0x40
#define	PMIC6B_SW_POWER_UP			0x41
#define	PMIC6B_SW_POWER_DOWN		0x42
#define	PMIC6B_GEN_STATUS0			0x43
#define	PMIC6B_GEN_STATUS1			0x44
#define	PMIC6B_WATCHDOG				0x45
#define	PMIC6B_WATCHDOG_CNT			0x46
#define	PMIC6B_OTP_CNTL				0x47
#define	PMIC6B_LOW_TIMEBASE			0x48
#define	PMIC6B_HIGH_TIMEBASE		0x49
#define	PMIC6B_CLK_DIV_CNTL			0x4A
#define	PMIC6B_SP_SOFT_RESET		0x4B
#define	PMIC6B_IRQ_MASK0			0x4C
#define	PMIC6B_IRQ_MASK1			0x4D
#define	PMIC6B_IRQ_STATUS_CLR0		0x4E
#define	PMIC6B_IRQ_STATUS_CLR1		0x4F
#define	PMIC6B_SP_DCDC_STATUS		0x50
#define	PMIC6B_SP_LDO_STATUS		0x51
#define	PMIC6B_PIN_MUX_REG0			0x52
#define	PMIC6B_PIN_MUX_REG1			0x53
#define	PMIC6B_PIN_MUX_REG2			0x54
#define	PMIC6B_PIN_MUX_REG3			0x55
#define	PMIC6B_PIN_MUX_REG4			0x56
#define	PMIC6B_PIN_MUX_REG5			0x57
#define	PMIC6B_PIN_MUX_REG6			0x58
#define	PMIC6B_PIN_MUX_REG7			0x59
#define	PMIC6B_PIN_MUX_REG8			0x5A
#define PMIC6B_GPIO_OUTPUT_CONTROL  0x5B
#define PMIC6B_GPIO_Input_LEVEL		0x5C
#define PMIC6B_I2C_OTP_ADDR			0x5d
#define PMIC6B_I2C_OTP_WDATA		0x5e
#define PMIC6B_I2C_OTP_RDATA_BYTE0	0x5f
#define PMIC6B_I2C_OTP_RDATA_BYTE1	0x60
#define PMIC6B_PWM1_TH_REG0			0x61
#define PMIC6B_PWM1_TH_REG1			0x62
#define PMIC6B_PWM1_TL_REG0			0x63
#define PMIC6B_PWM1_TL_REG1			0x64
#define PMIC6B_PWM2_TH_REG0			0x65
#define PMIC6B_PWM2_TH_REG1			0x66
#define PMIC6B_PWM2_TL_REG0			0x67
#define PMIC6B_PWM2_TL_REG1			0x68
#define PMIC6B_PWM_CLK_TCNT_REG		0x69
#define PMIC6B_PWM_ENABLE			0x6A
#define PMIC6B_CNTL_ADJUST_DC1		0x6B
#define PMIC6B_CNTL_ADJUST_DC2		0x6C
#define PMIC6B_BUCK_MUX_SEL			0x6D
#define PMIC6B_BUCK_EN				0x41
#define PMIC6B_AR_SW_EN				0x70
#define PMIC6B_SAR_CNTL_REG0		0x71
#define PMIC6B_SAR_CNTL_REG1		0x72
#define PMIC6B_SAR_CNTL_REG2		0x73
#define PMIC6B_SAR_CNTL_REG3		0x74
#define PMIC6B_SAR_CNTL_REG4		0x75
#define PMIC6B_SAR_CNTL_REG5		0x76
#define PMIC6B_SAR_CNTL_REG6		0x77
#define PMIC6B_SAR_CNTL_REG7		0x78
#define PMIC6B_SAR_CNTL_REG8		0x79
#define PMIC6B_SAR_CNTL_REG9		0x7A
#define PMIC6B_SAR_RD_IBAT_LAST		0x7B
#define PMIC6B_SAR_RD_VBAT_OFF		0x7C
#define PMIC6B_SAR_RD_VBAT_ACTIVE	0x7D
#define PMIC6B_SAR_RD_MANUAL		0x7E
#define PMIC6B_SAR_RD_IBAT_CHG0		0x7F
#define PMIC6B_SAR_RD_IBAT_CHG1		0x80
#define PMIC6B_SAR_RD_IBAT_CHG2		0x81
#define PMIC6B_SAR_RD_IBAT_DISCHG0  0x82
#define PMIC6B_SAR_RD_IBAT_DISCHG1	0x83
#define PMIC6B_SAR_RD_IBAT_DISCHG2	0x84
#define PMIC6B_SAR_RD_IBAT_CNT_CHG0		0x85
#define PMIC6B_SAR_RD_IBAT_CNT_CHG1		0x86
#define PMIC6B_SAR_RD_IBAT_CNT_DISCHG0	0x87
#define PMIC6B_SAR_RD_IBAT_CNT_DISCHG1	0x88
#define PMIC6B_SAR_RD_IBAT_LAST_RAW		0x89
#define PMIC6B_SAR_RD_IBA_RAW_CHG0		0X8A
#define PMIC6B_SAR_RD_IBA_RAW_CHG1		0x8B
#define PMIC6B_SAR_RD_IBA_RAW_CHG2		0x8C
#define PMIC6B_SAR_RD_IBAT_RAW_DISCHG0	0x8D
#define PMIC6B_SAR_RD_IBAT_RAW_DISCHG1	0x8E
#define PMIC6B_SAR_RD_IBAT_RAW_DISCHG2	0x8F
#define PMIC6B_SAR_RD_RAW				0x90
#define PMIC6B_CHG_SW_SEL0				0x91
#define PMIC6B_CHG_SW_SEL1				0x92
#define PMIC6B_CHG_SW_VAL0				0x93
#define PMIC6B_CHG_SW_VAL1				0x94
#define PMIC6B_CHG_TIMER0				0x95
#define PMIC6B_CHG_TIMER1				0x96
#define PMIC6B_CHG_FSM_SET				0x97
#define PMIC6B_CHG_FSM_CLR				0x98
#define PMIC6B_CHG_OUT_SEL				0x99
#define PMIC6B_CHG_OUT_VAL				0x9A
#define PMIC6B_CHG_DA_OUT_REG			0x9B
#define PMIC6B_CHG_DA_DB_REG0			0x9C
#define PMIC6B_CHG_DA_DB_REG1			0x9D
#define PMIC6B_CHG_AD_REG0				0x9E
#define PMIC6B_CHG_AD_REG1				0x9F
#define PMIC6B_CHG_DIG_REG0				0xA0
#define PMIC6B_CHG_ADJ_CTRL				0xA1
#define PMIC6B_CHG_ERR_MASK				0xA2
#define PMIC6B_CHG_INT_MASK				0xA3
#define PMIC6B_RG_RTC_REG0				0xA4
#define PMIC6B_RG_RTC_REG1				0XA5
#define PMIC6B_RG_RTC_REG2				0xA6
#define PMIC6B_RG_RTC_REG3				0xA7
#define PMIC6B_RG_RTC_REG4				0xA8
#define PMIC6B_RG_RTC_REG5				0xA9
#define PMIC6B_RG_RTC_REG6				0xAA
#define PMIC6B_RG_RTC_REG7				0xAB
#define PMIC6B_RG_RTC_REG8				0xAC
#define PMIC6B_RG_RTC_REG9				0xAD
#define PMIC6B_RG_RTC_REG10				0xAE
#define PMIC6B_RG_RTC_REG11				0xAF
#define PMIC6B_RG_RTC_REG12				0xB0
#define PMIC6B_RG_RTC_REG13				0xB1
#define PMIC6B_RG_RTC_REG14				0xB2
#define PMIC6B_RG_RTC_REG15				0xB3
#define PMIC6B_RG_RTC_REG16				0xB4
#define PMIC6B_RG_RTC_REG17				0xB5
#define PMIC6B_RG_RTC_REG18				0xB6
#define PMIC6B_ANALOG_REG_0				0xB7
#define PMIC6B_ANALOG_REG_1				0xB8
#define PMIC6B_ANALOG_REG_2				0xB9
#define PMIC6B_ANALOG_REG_3				0xBA
#define PMIC6B_ANALOG_REG_4				0xBB
#define PMIC6B_ANALOG_REG_5				0xBC
#define PMIC6B_ANALOG_REG_6				0xBD
#define PMIC6B_ANALOG_REG_7				0xBE
#define PMIC6B_ANALOG_REG_8				0xBF
#define PMIC6B_ANALOG_REG_9				0xC0
#define PMIC6B_ANALOG_REG_10			0xC1
#define PMIC6B_ANALOG_REG_11			0xC2
#define PMIC6B_ANALOG_REG_12			0xC3
#define PMIC6B_ANALOG_REG_13			0xC4
#define PMIC6B_ANALOG_REG_14			0xC5
#define PMIC6B_ANALOG_REG_15			0xC6
#define PMIC6B_ANALOG_REG_16			0xC7
#define PMIC6B_ANALOG_REG_17			0xC8
#define PMIC6B_ANALOG_REG_18			0xC9
#define PMIC6B_ANALOG_REG_19			0xCA
#define PMIC6B_ANALOG_REG_20			0xCB
#define PMIC6B_ANALOG_REG_21			0xCC
#define PMIC6B_ANALOG_REG_22			0xCD
#define PMIC6B_ANALOG_REG_23			0xCE
#define PMIC6B_ANALOG_REG_24			0xCF
#define PMIC6B_ANALOG_REG_25			0xD0
#define PMIC6B_ANALOG_REG_26			0xD1
#define PMIC6B_ANALOG_REG_27			0xD2
#define PMIC6B_ANALOG_REG_28			0xD3
#define PMIC6B_ANALOG_REG_29			0xD4
#define PMIC6B_ANALOG_REG_30			0xD5
#define	PMIC6B_REG_MAX					0xD6

/* PMIC6 modules */
#define PMIC6B_DRVNAME_REGULATORS	"pmic6b-regulators"
#define PMIC6B_DRVNAME_LEDS		"pmic6b-leds"
#define PMIC6B_DRVNAME_WATCHDOG		"pmic6b-watchdog"
#define PMIC6B_DRVNAME_ONKEY		"pmic6b-pwrkey"
#define PMIC6B_DRVNAME_BATTERY		"pmic6b-battery"
#define PMIC6B_DRVNAME_GPIO		"pmic6b-gpio"
#define PMIC6B_DRVNAME_RTC		"pmic6b-rtc"

#define PMIC6B_DCDC1_VOLTAGE_NUM	0x39
#define PMIC6B_DCDC2_VOLTAGE_NUM	0x59
#define PMIC6B_DCDC3_VOLTAGE_NUM	5

#define PMIC6B_LDO1_VOLTAGE_NUM	16
#define PMIC6B_LDO2_VOLTAGE_NUM	0x3f
#define PMIC6B_LDO3_VOLTAGE_NUM	0x3c

#define IRQ_MASK_BITS_POS_16	0x0
#define IRQ_MASK_BITS_POS_17	0x1
#define IRQ_MASK_BITS_POS_18	0x2
#define IRQ_MASK_BITS_POS_19	0x3
#define IRQ_MASK_BITS_POS_20	0x4
#define IRQ_MASK_BITS_POS_21	0x5
#define IRQ_MASK_BITS_POS_22	0x6
#define IRQ_MASK_BITS_POS_23	0x7
#define IRQ_MASK_BITS_POS_24	0x8
#define IRQ_MASK_BITS_POS_25	0x9
#define IRQ_MASK_BITS_POS_26	0xA
#define IRQ_MASK_BITS_POS_27	0xB
#define IRQ_MASK_BITS_POS_28	0xC
#define IRQ_MASK_BITS_POS_29	0xD
#define IRQ_MASK_BITS_POS_30	0xE
#define IRQ_MASK_BITS_POS_31	0xF

#define CALENDAR_EN_MASK_BITS_POS	0x9
#define ALARM_WKP_EN_MASK_BITS_POS	0xA
#define ALARM_CLEAN_MASK_BITS_POS	0x8

#define ALARM_SEL_POS			0xE
#define ALARM_SEL_DATE_HOUR_MIN_SEC	0x0
#define ALARM_SEL_MIN_SEC		0x1
#define ALARM_SEL_HOUR_MIN_SEC		0x2
#define ALARM_SEL_DAY_HOUR_MIN_SEC	0x3

#define CALENDAR_SEC_MASK_BITS	0x3F
#define CALENDAR_SEC_MASK_POS	0x0
#define CALENDAR_MIN_MASK_BITS	0x3F
#define CALENDAR_MIN_MASK_POS	0x8
#define CALENDAR_HOUR_MASK_BITS	0x1F
#define CALENDAR_HOUR_MASK_POS	0x0
#define CALENDAR_DAY_MASK_BITS	0x1F
#define CALENDAR_DAY_MASK_POS	0x8
#define CALENDAR_WEEK_MASK_BITS	0x7
#define CALENDAR_WEEK_MASK_POS	0x0
#define CALENDAR_MON_MASK_BITS	0x7
#define CALENDAR_MON_MASK_POS	0x8
#define CALENDAR_YEAR_MASK_BITS	0xFFF
#define CALENDAR_YEAR_MASK_POS	0x0

#define CALENDAR_TIME_SET_POS	0x0
#define CALENDAR_ALARM_SET_POS	0x4
#define CALENDAR_ALARM_CLEAN_POS	0x4

enum {
	PMIC6B_DCDC1,
	PMIC6B_DCDC2,
	PMIC6B_DCDC3,
	PMIC6B_LDO1,
	PMIC6B_LDO2,
	PMIC6B_LDO3,
	PMIC6B_REGU_MAX,
};

/* Interrupts */
enum pmic6b_irqs {
	PMIC6B_RTC_IRQ = 0,
	PMIC6B_IRQ_CHG_ERROR,
	PMIC6B_IRQ_CHG_BAT_OCOV,
	PMIC6B_IRQ_CHG_EOC,
	PMIC6B_IRQ_CHG_NOBAT,
	PMIC6B_IRQ_CHG_CHGING,
	PMIC6B_IRQ_CHG_BAT_TEMP,
	PMIC6B_IRQ_CHG_TIMEOUT,
	PMIC6B_IRQ_CHG_STATE_DIFF,
	PMIC6B_IRQ_CHG_PLUGOUT,
	PMIC6B_IRQ_CHG_PLUGIN,
	PMIC6B_DC1_OC_SP,
	PMIC6B_DC2_OC_SP,
	PMIC6B_DC3_OC_SP,
	PMIC6B_LDO1_OV_SP,
	PMIC6B_LDO1_UV_SP,

	PMIC6B_LDO1_OC_SP,
	PMIC6B_LDO2_UV_SP,
	PMIC6B_LDO2_OC_SP,
	PMIC6B_LDO3_OV_SP,
	PMIC6B_LDO3_UV_SP,
	PMIC6B_LDO3_OC_SP,
	PMIC6B_DCIN_OVP_DB,
	PMIC6B_DCIN_UVLO_DB,
	PMIC6B_SYS_UVLO_DB,
	PMIC6B_SYS_OVP_DB,
	PMIC6B_OTP_SHUT,
	PMIC6B_RTC_TRING_DEEP_SLEEP,
	PMIC6B_RTC_TRING_SLEEP,
	PMIC6B_RTC_WAKE_UP,
	PMIC6B_PWR_KEY_PRESS,
	PMIC6B_SAR_IRQ,
};

struct meson_pmic6b {
	/* Device */
	struct device	*dev;
	unsigned int	flags;

	/* Control interface */
	struct regmap	*regmap;

	/* Interrupts */
	int		chip_irq;
	unsigned int	irq_base;
	struct regmap_irq_chip_data *regmap_irq;
};

/* Regulators platform data */
struct pmic6b_regulator_data {
	int				id;
	struct regulator_init_data	*initdata;
};

struct pmic6b_regulators_pdata {
	unsigned int			n_regulators;
	struct pmic6b_regulator_data	*regulator_data;
};

/* PMIC6 platform data */
struct pmic6b_pdata {
	int				(*init)(struct meson_pmic6b *pmic6b);
	int				irq_base;
	int				key_power;
	unsigned int			flags;
	struct pmic6b_regulators_pdata	*regulators_pdata;
	struct led_platform_data	*leds_pdata;
};

#endif /* _MESON_PMIC6B_H */
