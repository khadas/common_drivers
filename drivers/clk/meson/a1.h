/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __A1_H
#define __A1_H

/*
 * Clock controller register address
 * APB_BASE:  APB0_BASE_ADDR = 0xfe000800
 */
#define SYS_OSCIN_CTRL                  0x0
#define RTC_BY_OSCIN_CTRL0              0x4
#define RTC_BY_OSCIN_CTRL1              0x8
#define RTC_CTRL                        0xc
#define SYS_CLK_CTRL0                   0x10
#define AXI_CLK_CTRL0                   0x14
#define SYS_CLK_EN0                     0x1c
#define SYS_CLK_EN1                     0x20
#define AXI_CLK_EN                      0x24
#define DSPA_CLK_EN                     0x28
#define DSPB_CLK_EN                     0x2c
#define DSPA_CLK_CTRL0                  0x30
#define DSPB_CLK_CTRL0                  0x34
#define CLK12_24_CTRL                   0x38
#define GEN_CLK_CTRL                    0x3c
#define TIMESTAMP_CTRL0                 0x40
#define TIMESTAMP_CTRL1                 0x44
#define TIMESTAMP_CTRL2                 0x48
#define TIMESTAMP_VAL0                  0x4c
#define TIMESTAMP_VAL1                  0x50
#define	TIMEBASE_CTRL0                  0x54
#define TIMEBASE_CTRL1                  0x58
#define SAR_ADC_CLK_CTRL                0xc0
#define PWM_CLK_AB_CTRL                 0xc4
#define PWM_CLK_CD_CTRL                 0xc8
#define PWM_CLK_EF_CTRL                 0xcc
#define SPICC_CLK_CTRL                  0xd0
#define TS_CLK_CTRL                     0xd4
#define SPIFC_CLK_CTRL                  0xd8
#define USB_BUSCLK_CTRL                 0xdc
#define SD_EMMC_CLK_CTRL                0xe0
#define CECA_CLK_CTRL0                  0xe4
#define CECA_CLK_CTRL1                  0xe8
#define CECB_CLK_CTRL0                  0xec
#define CECB_CLK_CTRL1                  0xf0
#define PSRAM_CLK_CTRL                  0xf4
#define DMC_CLK_CTRL                    0xf8
#define FCLK_DIV1_SEL			0xfc
#define TST_CTRL			0x100

/*
 * For PLl register offset
 * APB_BASE:  APB0_BASE_ADDR = 0xfe007c00
 */
#define ANACTRL_FIXPLL_CTRL0		0x80
#define ANACTRL_FIXPLL_CTRL1            0x84
#define ANACTRL_FIXPLL_CTRL2            0x88
#define ANACTRL_FIXPLL_CTRL3            0x8c
#define ANACTRL_FIXPLL_CTRL4            0x90
#define ANACTRL_FIXPLL_STS              0x94
#define ANACTRL_SYSPLL_CTRL0            0x100
#define ANACTRL_SYSPLL_CTRL1            0x104
#define ANACTRL_SYSPLL_CTRL2		0x108
#define ANACTRL_SYSPLL_CTRL3            0x10c
#define ANACTRL_SYSPLL_CTRL4            0x110
#define ANACTRL_SYSPLL_STS              0x114
#define ANACTRL_HIFIPLL_CTRL0           0x140
#define ANACTRL_HIFIPLL_CTRL1           0x144
#define ANACTRL_HIFIPLL_CTRL2           0x148
#define ANACTRL_HIFIPLL_CTRL3           0x14c
#define ANACTRL_HIFIPLL_CTRL4           0x150
#define ANACTRL_HIFIPLL_STS             0x154
#define ANACTRL_AUDDDS_CTRL0            0x180
#define ANACTRL_AUDDDS_CTRL1            0x184
#define ANACTRL_AUDDDS_CTRL2            0x188
#define ANACTRL_AUDDDS_CTRL3            0x18c
#define ANACTRL_AUDDDS_CTRL4            0x190
#define ANACTRL_AUDDDS_STS              0x194
#define ANACTRL_MISCTOP_CTRL0           0x1c0
#define ANACTRL_POR_CNTL                0x208

/*
 * CPU clok register offset
 * APB_BASE:  APB1_BASE_ADDR = 0xfd000000
 */

#define CPUCTRL_CLK_CTRL0		0x80
#define CPUCTRL_CLK_CTRL1		0x84

#include <dt-bindings/clock/a1-clkc.h>
#define NR_CLKS				(BASIC_BASE2 + 20)

#endif /* __A1_H */
