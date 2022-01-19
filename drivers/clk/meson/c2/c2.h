/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __C2_H
#define __C2_H

/*
 * Clock controller register address
 * APB_BASE:  APB0_BASE_ADDR = 0xfe000800
 */
#define SYS_OSCIN_CTRL			(0x0000 << 2)
#define RTC_BY_OSCIN_CTRL0		(0x0001 << 2)
#define RTC_BY_OSCIN_CTRL1		(0x0002 << 2)
#define RTC_CTRL			(0x0003 << 2)
#define SYS_CLK_CTRL0			(0x0004 << 2)
#define AXI_CLK_CTRL0			(0x0005 << 2)
#define SYS_CLK_EN0			(0x0006 << 2)
#define SYS_CLK_EN1			(0x0007 << 2)
#define SYS_CLK_EN2			(0x0008 << 2)
#define AXI_CLK_EN			(0x0009 << 2)
#define DSPA_CLK_EN			(0x0009 << 2)
#define DSPB_CLK_EN			(0x000b << 2)
#define DSPA_CLK_CTRL0			(0x000c << 2)
#define DSPB_CLK_CTRL0			(0x000d << 2)
#define CLK12_24_CTRL			(0x000e << 2)
#define GEN_CLK_CTRL			(0x000f << 2)
#define TIMESTAMP_CTRL0			(0x0010 << 2)
#define TIMESTAMP_CTRL1			(0x0011 << 2)
#define TIMESTAMP_CTRL2			(0x0012 << 2)
#define TIMESTAMP_VAL0			(0x0013 << 2)
#define TIMESTAMP_VAL1			(0x0014 << 2)
#define TIMEBASE_CTRL0			(0x0015 << 2)
#define TIMEBASE_CTRL1			(0x0016 << 2)
#define SAR_ADC_CLK_CTRL		(0x0030 << 2)
#define PWM_CLK_AB_CTRL			(0x0031 << 2)
#define PWM_CLK_CD_CTRL			(0x0032 << 2)
#define PWM_CLK_EF_CTRL			(0x0033 << 2)
#define SPICC_CLK_CTRL			(0x0034 << 2)
#define TS_CLK_CTRL			(0x0035 << 2)
#define SPIFC_CLK_CTRL			(0x0036 << 2)
#define USB_BUSCLK_CTRL			(0x0037 << 2)
#define SD_EMMC_CLK_CTRL		(0x0038 << 2)
#define CECA_CLK_CTRL0			(0x0039 << 2)
#define CECA_CLK_CTRL1			(0x003a << 2)
#define CECB_CLK_CTRL0			(0x003b << 2)
#define CECB_CLK_CTRL1			(0x003c << 2)
#define PSRAM_CLK_CTRL			(0x003d << 2)
#define DMC_CLK_CTRL			(0x003e << 2)
#define FCLK_DIV1_SEL			(0x003f << 2)
#define TST_CTRL0			(0x0040 << 2)
#define WAVE_CLK_CTRL0			(0x0041 << 2)
#define WAVE_CLK_CTRL1			(0x0042 << 2)
#define JPEG_CLK_CTRL			(0x0043 << 2)
#define MIPI_ISP_CLK_CTRL		(0x0044 << 2)
#define NNA_CLK_CTRL			(0x0045 << 2)
#define GDC_CLK_CTRL			(0x0046 << 2)
#define GE2D_CLK_CTRL			(0x0047 << 2)
#define SD_EMMC_CLK_CTRL1		(0x0048 << 2)
#define ETH_CLK_CTRL			(0x0049 << 2)
#define PWM_CLK_GH_CTRL			(0x004a << 2)
#define PWM_CLK_IJ_CTRL			(0x004b << 2)
#define MBIST_ATSPEED_CTRL		(0x004c << 2)
#define SECPU_CLK_CTRL			(0x004d << 2)

/*
 * For PLl register offset
 * APB_BASE:  APB0_BASE_ADDR = 0xfe007c00
 */
#define ANACTRL_PLL_GATE_DIS		(0x0010 << 2)
#define ANACTRL_FIXPLL_CTRL0		(0x0020 << 2)
#define ANACTRL_FIXPLL_CTRL1            (0x0021 << 2)
#define ANACTRL_FIXPLL_CTRL2            (0x0022 << 2)
#define ANACTRL_FIXPLL_CTRL3            (0x0023 << 2)
#define ANACTRL_FIXPLL_CTRL4            (0x0024 << 2)
#define ANACTRL_FIXPLL_CTRL5            (0x0025 << 2)
#define ANACTRL_FIXPLL_CTRL6            (0x0026 << 2)
#define ANACTRL_FIXPLL_STS              (0x0027 << 2)
#define ANACTRL_GPPLL_CTRL0		(0x0030 << 2)
#define ANACTRL_GPPLL_CTRL1             (0x0031 << 2)
#define ANACTRL_GPPLL_CTRL2             (0x0032 << 2)
#define ANACTRL_GPPLL_CTRL3             (0x0033 << 2)
#define ANACTRL_GPPLL_CTRL4             (0x0034 << 2)
#define ANACTRL_GPPLL_CTRL5             (0x0035 << 2)
#define ANACTRL_GPPLL_STS               (0x0036 << 2)
#define ANACTRL_GPPLL_CTRL6             (0x0037 << 2)
#define ANACTRL_SYSPLL_CTRL0            (0x0040 << 2)
#define ANACTRL_SYSPLL_CTRL1            (0x0041 << 2)
#define ANACTRL_SYSPLL_CTRL2		(0x0042 << 2)
#define ANACTRL_SYSPLL_CTRL3            (0x0043 << 2)
#define ANACTRL_SYSPLL_CTRL4            (0x0044 << 2)
#define ANACTRL_SYSPLL_STS              (0x0045 << 2)
#define ANACTRL_HIFIPLL_CTRL0           (0x0050 << 2)
#define ANACTRL_HIFIPLL_CTRL1           (0x0051 << 2)
#define ANACTRL_HIFIPLL_CTRL2           (0x0052 << 2)
#define ANACTRL_HIFIPLL_CTRL3           (0x0053 << 2)
#define ANACTRL_HIFIPLL_CTRL4           (0x0054 << 2)
#define ANACTRL_HIFIPLL_STS             (0x0055 << 2)
#define ANACTRL_MISCTOP_CTRL0           (0x0070 << 2)
#define ANACTRL_POR_CNTL                (0x0082 << 2)

/*
 * CPU clok register offset
 * APB_BASE:  APB1_BASE_ADDR = 0xfe007400
 */

#define CPUCTRL_CLK_CTRL0		0x0
#define CPUCTRL_CLK_CTRL1		0x4
#define CPUCTRL_CLK_CTRL5		0x14
#define CPUCTRL_CLK_CTRL6		0x18

#include <dt-bindings/clock/c2-clkc.h>
#define NR_CLKS				(CLKID_END_BASE)

#endif /* __C2_H */
