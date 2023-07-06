/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __C1_CLKC_H
#define __C1_CLKC_H

/*
 * CLKID index values
 */

#define CLKID_PLL_BASE			0
#define CLKID_FIXED_PLL_DCO		(CLKID_PLL_BASE + 0)
#define CLKID_FIXED_PLL			(CLKID_PLL_BASE + 1)
#define CLKID_FCLK_DIV2_DIV		(CLKID_PLL_BASE + 2)
#define CLKID_FCLK_DIV2			(CLKID_PLL_BASE + 3)
#define CLKID_FCLK_DIV2P5_DIV		(CLKID_PLL_BASE + 4)
#define CLKID_FCLK_DIV2P5		(CLKID_PLL_BASE + 5)
#define CLKID_FCLK_DIV3_DIV		(CLKID_PLL_BASE + 6)
#define CLKID_FCLK_DIV3			(CLKID_PLL_BASE + 7)
#define CLKID_FCLK_DIV4_DIV		(CLKID_PLL_BASE + 8)
#define CLKID_FCLK_DIV4			(CLKID_PLL_BASE + 9)
#define CLKID_FCLK_DIV5_DIV		(CLKID_PLL_BASE + 10)
#define CLKID_FCLK_DIV5			(CLKID_PLL_BASE + 11)
#define CLKID_FCLK_DIV7_DIV		(CLKID_PLL_BASE + 12)
#define CLKID_FCLK_DIV7			(CLKID_PLL_BASE + 13)
#define CLKID_FCLK_DIV40_DIV		(CLKID_PLL_BASE + 14)
#define CLKID_FCLK50M			(CLKID_PLL_BASE + 15)
#define CLKID_SYS_PLL_DCO		(CLKID_PLL_BASE + 16)
#define CLKID_SYS_PLL			(CLKID_PLL_BASE + 17)
#define CLKID_GP_PLL_DCO		(CLKID_PLL_BASE + 18)
#define CLKID_GP_PLL			(CLKID_PLL_BASE + 19)
#define CLKID_GP_DIV1_DIV2		(CLKID_PLL_BASE + 20)
#define CLKID_GP_DIV1_DIV		(CLKID_PLL_BASE + 21)
#define CLKID_GP_DIV2_DIV2		(CLKID_PLL_BASE + 22)
#define CLKID_GP_DIV2_DIV		(CLKID_PLL_BASE + 23)
#define CLKID_GP_DIV3_DIV		(CLKID_PLL_BASE + 24)
#define CLKID_HIFI_PLL_DCO		(CLKID_PLL_BASE + 25)
#define CLKID_HIFI_PLL			(CLKID_PLL_BASE + 26)
#define CLKID_AUD_DDS			(CLKID_PLL_BASE + 27)

#define CLKID_CPU_BASE			(CLKID_PLL_BASE + 28)
#define CLKID_CPU_DYN_CLK		(CLKID_CPU_BASE + 0)
#define CLKID_CPU_CLK			(CLKID_CPU_BASE + 1)
#define CLKID_DSU_DYN_CLK		(CLKID_CPU_BASE + 2)
#define CLKID_DSU_PRE_CLK		(CLKID_CPU_BASE + 3)
#define CLKID_DSU_CLK			(CLKID_CPU_BASE + 4)
#define CLKID_DSU_AXI_CLK		(CLKID_CPU_BASE + 5)

#define CLKID_CLK_BASE			(CLKID_CPU_BASE + 6)
#define CLKID_GP_DIV1			(CLKID_CLK_BASE + 0)
#define CLKID_GP_DIV2			(CLKID_CLK_BASE + 1)
#define CLKID_GP_DIV3			(CLKID_CLK_BASE + 2)
#define CLKID_FIXED_PLL_DIV2		(CLKID_CLK_BASE + 3)
#define CLKID_RTC_32K_CLKIN		(CLKID_CLK_BASE + 4)
#define CLKID_RTC_32K_DIV		(CLKID_CLK_BASE + 5)
#define CLKID_RTC_32K_MUX		(CLKID_CLK_BASE + 6)
#define CLKID_RTC_32K			(CLKID_CLK_BASE + 7)
#define CLKID_RTC_CLK			(CLKID_CLK_BASE + 8)
#define CLKID_SYS_CLK_A_MUX		(CLKID_CLK_BASE + 9)
#define CLKID_SYS_CLK_A_DIV		(CLKID_CLK_BASE + 10)
#define CLKID_SYS_CLK_A_GATE		(CLKID_CLK_BASE + 11)
#define CLKID_SYS_CLK_B_MUX		(CLKID_CLK_BASE + 12)
#define CLKID_SYS_CLK_B_DIV		(CLKID_CLK_BASE + 13)
#define CLKID_SYS_CLK_B_GATE		(CLKID_CLK_BASE + 14)
#define CLKID_SYS_CLK			(CLKID_CLK_BASE + 15)
#define CLKID_AXI_CLK_A_MUX		(CLKID_CLK_BASE + 16)
#define CLKID_AXI_CLK_A_DIV		(CLKID_CLK_BASE + 17)
#define CLKID_AXI_CLK_A_GATE		(CLKID_CLK_BASE + 18)
#define CLKID_AXI_CLK_B_MUX		(CLKID_CLK_BASE + 19)
#define CLKID_AXI_CLK_B_DIV		(CLKID_CLK_BASE + 20)
#define CLKID_AXI_CLK_B_GATE		(CLKID_CLK_BASE + 21)
#define CLKID_AXI_CLK			(CLKID_CLK_BASE + 22)

#define CLKID_SYS_AXI_GATE_BASE		(CLKID_CLK_BASE + 23)
#define CLKID_SYS_CLKTREE		(CLKID_SYS_AXI_GATE_BASE + 0)
#define CLKID_SYS_RESET_CTRL		(CLKID_SYS_AXI_GATE_BASE + 1)
#define CLKID_SYS_ANALOG_CTRL		(CLKID_SYS_AXI_GATE_BASE + 2)
#define CLKID_SYS_PWR_CTRL		(CLKID_SYS_AXI_GATE_BASE + 3)
#define CLKID_SYS_PAD_CTRL		(CLKID_SYS_AXI_GATE_BASE + 4)
#define CLKID_SYS_CTRL			(CLKID_SYS_AXI_GATE_BASE + 5)
#define CLKID_SYS_TEMP_SENSOR		(CLKID_SYS_AXI_GATE_BASE + 6)
#define CLKID_SYS_AM2AXI_DIV		(CLKID_SYS_AXI_GATE_BASE + 7)
#define CLKID_SYS_SPICC_B		(CLKID_SYS_AXI_GATE_BASE + 8)
#define CLKID_SYS_SPICC_A		(CLKID_SYS_AXI_GATE_BASE + 9)
#define CLKID_SYS_CLK_MSR		(CLKID_SYS_AXI_GATE_BASE + 10)
#define CLKID_SYS_AUDIO			(CLKID_SYS_AXI_GATE_BASE + 11)
#define CLKID_SYS_JTAG_CTRL		(CLKID_SYS_AXI_GATE_BASE + 12)
#define CLKID_SYS_SARADC		(CLKID_SYS_AXI_GATE_BASE + 13)
#define CLKID_SYS_PWM_EF		(CLKID_SYS_AXI_GATE_BASE + 14)
#define CLKID_SYS_PWM_CD		(CLKID_SYS_AXI_GATE_BASE + 15)
#define CLKID_SYS_PWM_AB		(CLKID_SYS_AXI_GATE_BASE + 16)
#define CLKID_SYS_I2C_S			(CLKID_SYS_AXI_GATE_BASE + 17)
#define CLKID_SYS_IR_CTRL		(CLKID_SYS_AXI_GATE_BASE + 18)
#define CLKID_SYS_I2C_M_D		(CLKID_SYS_AXI_GATE_BASE + 19)
#define CLKID_SYS_I2C_M_C		(CLKID_SYS_AXI_GATE_BASE + 20)
#define CLKID_SYS_I2C_M_B		(CLKID_SYS_AXI_GATE_BASE + 21)
#define CLKID_SYS_I2C_M_A		(CLKID_SYS_AXI_GATE_BASE + 22)
#define CLKID_SYS_ACODEC		(CLKID_SYS_AXI_GATE_BASE + 23)
#define CLKID_SYS_OTP			(CLKID_SYS_AXI_GATE_BASE + 24)
#define CLKID_SYS_SD_EMMC_A		(CLKID_SYS_AXI_GATE_BASE + 25)
#define CLKID_SYS_USB_PHY		(CLKID_SYS_AXI_GATE_BASE + 26)
#define CLKID_SYS_USB_CTRL		(CLKID_SYS_AXI_GATE_BASE + 27)
#define CLKID_SYS_DSPB			(CLKID_SYS_AXI_GATE_BASE + 28)
#define CLKID_SYS_DSPA			(CLKID_SYS_AXI_GATE_BASE + 29)
#define CLKID_SYS_DMA			(CLKID_SYS_AXI_GATE_BASE + 30)
#define CLKID_SYS_IRQ_CTRL		(CLKID_SYS_AXI_GATE_BASE + 31)
#define CLKID_SYS_NIC			(CLKID_SYS_AXI_GATE_BASE + 32)
#define CLKID_SYS_GIC			(CLKID_SYS_AXI_GATE_BASE + 33)
#define CLKID_SYS_UART_C		(CLKID_SYS_AXI_GATE_BASE + 34)
#define CLKID_SYS_UART_B		(CLKID_SYS_AXI_GATE_BASE + 35)
#define CLKID_SYS_UART_A		(CLKID_SYS_AXI_GATE_BASE + 36)
#define CLKID_SYS_RSA			(CLKID_SYS_AXI_GATE_BASE + 37)
#define CLKID_SYS_CORESIGHT		(CLKID_SYS_AXI_GATE_BASE + 38)
#define CLKID_SYS_CSI_PHY1		(CLKID_SYS_AXI_GATE_BASE + 39)
#define CLKID_SYS_CSI_PHY0		(CLKID_SYS_AXI_GATE_BASE + 40)
#define CLKID_SYS_MIPI_ISP		(CLKID_SYS_AXI_GATE_BASE + 41)
#define CLKID_SYS_CSI_DIG		(CLKID_SYS_AXI_GATE_BASE + 42)
#define CLKID_SYS_G2ED			(CLKID_SYS_AXI_GATE_BASE + 43)
#define CLKID_SYS_GDC			(CLKID_SYS_AXI_GATE_BASE + 44)
#define CLKID_SYS_DOS_APB		(CLKID_SYS_AXI_GATE_BASE + 45)
#define CLKID_SYS_NNA			(CLKID_SYS_AXI_GATE_BASE + 46)
#define CLKID_SYS_ETH_PHY		(CLKID_SYS_AXI_GATE_BASE + 47)
#define CLKID_SYS_ETH_MAC		(CLKID_SYS_AXI_GATE_BASE + 48)
#define CLKID_SYS_UART_E		(CLKID_SYS_AXI_GATE_BASE + 49)
#define CLKID_SYS_UART_D		(CLKID_SYS_AXI_GATE_BASE + 50)
#define CLKID_SYS_PWM_IJ		(CLKID_SYS_AXI_GATE_BASE + 51)
#define CLKID_SYS_PWM_GH		(CLKID_SYS_AXI_GATE_BASE + 52)
#define CLKID_SYS_I2C_M_E		(CLKID_SYS_AXI_GATE_BASE + 53)
#define CLKID_SYS_SD_EMMC_C		(CLKID_SYS_AXI_GATE_BASE + 54)
#define CLKID_SYS_SD_EMMC_B		(CLKID_SYS_AXI_GATE_BASE + 55)
#define CLKID_SYS_ROM			(CLKID_SYS_AXI_GATE_BASE + 56)
#define CLKID_SYS_SPIFC			(CLKID_SYS_AXI_GATE_BASE + 57)
#define CLKID_SYS_PROD_I2C		(CLKID_SYS_AXI_GATE_BASE + 58)
#define CLKID_SYS_DOS			(CLKID_SYS_AXI_GATE_BASE + 59)
#define CLKID_SYS_CPU_CTRL		(CLKID_SYS_AXI_GATE_BASE + 60)
#define CLKID_SYS_RAMA			(CLKID_SYS_AXI_GATE_BASE + 61)
#define CLKID_SYS_RAMB			(CLKID_SYS_AXI_GATE_BASE + 62)
#define CLKID_SYS_RAMC			(CLKID_SYS_AXI_GATE_BASE + 63)
#define CLKID_AXI_AM2AXI_VAD		(CLKID_SYS_AXI_GATE_BASE + 64)
#define CLKID_AXI_AUDIO_VAD		(CLKID_SYS_AXI_GATE_BASE + 65)
#define CLKID_AXI_DMC			(CLKID_SYS_AXI_GATE_BASE + 66)
#define CLKID_AXI_RAMB			(CLKID_SYS_AXI_GATE_BASE + 67)
#define CLKID_AXI_RAMA			(CLKID_SYS_AXI_GATE_BASE + 68)
#define CLKID_AXI_NIC			(CLKID_SYS_AXI_GATE_BASE + 69)
#define CLKID_AXI_DMA			(CLKID_SYS_AXI_GATE_BASE + 70)
#define CLKID_AXI_RAMC			(CLKID_SYS_AXI_GATE_BASE + 71)

#define CLKID_DSP_BASE			(CLKID_SYS_AXI_GATE_BASE + 72)
#define CLKID_DSPA_A_MUX		(CLKID_DSP_BASE + 0)
#define CLKID_DSPA_A_DIV		(CLKID_DSP_BASE + 1)
#define CLKID_DSPA_A			(CLKID_DSP_BASE + 2)
#define CLKID_DSPA_B_MUX		(CLKID_DSP_BASE + 3)
#define CLKID_DSPA_B_DIV		(CLKID_DSP_BASE + 4)
#define CLKID_DSPA_B			(CLKID_DSP_BASE + 5)
#define CLKID_DSPA			(CLKID_DSP_BASE + 6)
#define CLKID_DSPA_DSPA			(CLKID_DSP_BASE + 7)
#define CLKID_DSPA_NIC			(CLKID_DSP_BASE + 8)
#define CLKID_DSPB_A_MUX		(CLKID_DSP_BASE + 9)
#define CLKID_DSPB_A_DIV		(CLKID_DSP_BASE + 10)
#define CLKID_DSPB_A			(CLKID_DSP_BASE + 11)
#define CLKID_DSPB_B_MUX		(CLKID_DSP_BASE + 12)
#define CLKID_DSPB_B_DIV		(CLKID_DSP_BASE + 13)
#define CLKID_DSPB_B			(CLKID_DSP_BASE + 14)
#define CLKID_DSPB			(CLKID_DSP_BASE + 15)
#define CLKID_DSPB_DSPB			(CLKID_DSP_BASE + 16)
#define CLKID_DSPB_NIC			(CLKID_DSP_BASE + 17)

#define CLKID_PERIPHERAL_BASE		(CLKID_DSP_BASE + 18)
#define CLKID_24M			(CLKID_PERIPHERAL_BASE + 0)
#define CLKID_24M_DIV2			(CLKID_PERIPHERAL_BASE + 1)
#define CLKID_12M			(CLKID_PERIPHERAL_BASE + 2)
#define CLKID_FCLK_DIV2_DIVN_PRE	(CLKID_PERIPHERAL_BASE + 3)
#define CLKID_FCLK_DIV2_DIVN		(CLKID_PERIPHERAL_BASE + 4)
#define CLKID_GEN_MUX			(CLKID_PERIPHERAL_BASE + 5)
#define CLKID_GEN_DIV			(CLKID_PERIPHERAL_BASE + 6)
#define CLKID_GEN			(CLKID_PERIPHERAL_BASE + 7)
#define CLKID_SARADC_MUX		(CLKID_PERIPHERAL_BASE + 8)
#define CLKID_SARADC_DIV		(CLKID_PERIPHERAL_BASE + 9)
#define CLKID_SARADC			(CLKID_PERIPHERAL_BASE + 10)
#define CLKID_PWM_A_MUX			(CLKID_PERIPHERAL_BASE + 11)
#define CLKID_PWM_A_DIV			(CLKID_PERIPHERAL_BASE + 12)
#define CLKID_PWM_A			(CLKID_PERIPHERAL_BASE + 13)
#define CLKID_PWM_B_MUX			(CLKID_PERIPHERAL_BASE + 14)
#define CLKID_PWM_B_DIV			(CLKID_PERIPHERAL_BASE + 15)
#define CLKID_PWM_B			(CLKID_PERIPHERAL_BASE + 16)
#define CLKID_PWM_C_MUX			(CLKID_PERIPHERAL_BASE + 17)
#define CLKID_PWM_C_DIV			(CLKID_PERIPHERAL_BASE + 18)
#define CLKID_PWM_C			(CLKID_PERIPHERAL_BASE + 19)
#define CLKID_PWM_D_MUX			(CLKID_PERIPHERAL_BASE + 20)
#define CLKID_PWM_D_DIV			(CLKID_PERIPHERAL_BASE + 21)
#define CLKID_PWM_D			(CLKID_PERIPHERAL_BASE + 22)
#define CLKID_PWM_E_MUX			(CLKID_PERIPHERAL_BASE + 23)
#define CLKID_PWM_E_DIV			(CLKID_PERIPHERAL_BASE + 24)
#define CLKID_PWM_E			(CLKID_PERIPHERAL_BASE + 25)
#define CLKID_PWM_F_MUX			(CLKID_PERIPHERAL_BASE + 26)
#define CLKID_PWM_F_DIV			(CLKID_PERIPHERAL_BASE + 27)
#define CLKID_PWM_F			(CLKID_PERIPHERAL_BASE + 28)
#define CLKID_PWM_G_MUX			(CLKID_PERIPHERAL_BASE + 29)
#define CLKID_PWM_G_DIV			(CLKID_PERIPHERAL_BASE + 30)
#define CLKID_PWM_G			(CLKID_PERIPHERAL_BASE + 31)
#define CLKID_PWM_H_MUX			(CLKID_PERIPHERAL_BASE + 32)
#define CLKID_PWM_H_DIV			(CLKID_PERIPHERAL_BASE + 33)
#define CLKID_PWM_H			(CLKID_PERIPHERAL_BASE + 34)
#define CLKID_PWM_I_MUX			(CLKID_PERIPHERAL_BASE + 35)
#define CLKID_PWM_I_DIV			(CLKID_PERIPHERAL_BASE + 36)
#define CLKID_PWM_I			(CLKID_PERIPHERAL_BASE + 37)
#define CLKID_PWM_J_MUX			(CLKID_PERIPHERAL_BASE + 38)
#define CLKID_PWM_J_DIV			(CLKID_PERIPHERAL_BASE + 39)
#define CLKID_PWM_J			(CLKID_PERIPHERAL_BASE + 40)
#define CLKID_SPICC_A_PRE_MUX		(CLKID_PERIPHERAL_BASE + 41)
#define CLKID_SPICC_A_PRE_DIV		(CLKID_PERIPHERAL_BASE + 42)
#define CLKID_SPICC_A_MUX		(CLKID_PERIPHERAL_BASE + 43)
#define CLKID_SPICC_A			(CLKID_PERIPHERAL_BASE + 44)
#define CLKID_SPICC_B_PRE_MUX		(CLKID_PERIPHERAL_BASE + 45)
#define CLKID_SPICC_B_PRE_DIV		(CLKID_PERIPHERAL_BASE + 46)
#define CLKID_SPICC_B_MUX		(CLKID_PERIPHERAL_BASE + 47)
#define CLKID_SPICC_B			(CLKID_PERIPHERAL_BASE + 48)
#define CLKID_SPIFC_PRE_MUX		(CLKID_PERIPHERAL_BASE + 49)
#define CLKID_SPIFC_PRE_DIV		(CLKID_PERIPHERAL_BASE + 50)
#define CLKID_SPIFC_MUX			(CLKID_PERIPHERAL_BASE + 51)
#define CLKID_SPIFC			(CLKID_PERIPHERAL_BASE + 52)
#define CLKID_TS_DIV			(CLKID_PERIPHERAL_BASE + 53)
#define CLKID_TS			(CLKID_PERIPHERAL_BASE + 54)
#define CLKID_USB_BUS_MUX		(CLKID_PERIPHERAL_BASE + 55)
#define CLKID_USB_BUS_DIV		(CLKID_PERIPHERAL_BASE + 56)
#define CLKID_USB_BUS			(CLKID_PERIPHERAL_BASE + 57)
#define CLKID_SD_EMMC_A_PRE_MUX		(CLKID_PERIPHERAL_BASE + 58)
#define CLKID_SD_EMMC_A_PRE_DIV		(CLKID_PERIPHERAL_BASE + 59)
#define CLKID_SD_EMMC_A_MUX		(CLKID_PERIPHERAL_BASE + 60)
#define CLKID_SD_EMMC_A			(CLKID_PERIPHERAL_BASE + 61)
#define CLKID_SD_EMMC_B_PRE_MUX		(CLKID_PERIPHERAL_BASE + 62)
#define CLKID_SD_EMMC_B_PRE_DIV		(CLKID_PERIPHERAL_BASE + 63)
#define CLKID_SD_EMMC_B_MUX		(CLKID_PERIPHERAL_BASE + 64)
#define CLKID_SD_EMMC_B			(CLKID_PERIPHERAL_BASE + 65)
#define CLKID_SD_EMMC_C_PRE_MUX		(CLKID_PERIPHERAL_BASE + 66)
#define CLKID_SD_EMMC_C_PRE_DIV		(CLKID_PERIPHERAL_BASE + 67)
#define CLKID_SD_EMMC_C_MUX		(CLKID_PERIPHERAL_BASE + 68)
#define CLKID_SD_EMMC_C			(CLKID_PERIPHERAL_BASE + 69)
#define CLKID_WAVE_A_PRE_MUX		(CLKID_PERIPHERAL_BASE + 70)
#define CLKID_WAVE_A_PRE_DIV		(CLKID_PERIPHERAL_BASE + 71)
#define CLKID_WAVE_A_MUX		(CLKID_PERIPHERAL_BASE + 72)
#define CLKID_WAVE_A			(CLKID_PERIPHERAL_BASE + 73)
#define CLKID_WAVE_B_PRE_MUX		(CLKID_PERIPHERAL_BASE + 74)
#define CLKID_WAVE_B_PRE_DIV		(CLKID_PERIPHERAL_BASE + 75)
#define CLKID_WAVE_B_MUX		(CLKID_PERIPHERAL_BASE + 76)
#define CLKID_WAVE_B			(CLKID_PERIPHERAL_BASE + 77)
#define CLKID_WAVE_C_PRE_MUX		(CLKID_PERIPHERAL_BASE + 78)
#define CLKID_WAVE_C_PRE_DIV		(CLKID_PERIPHERAL_BASE + 79)
#define CLKID_WAVE_C_MUX		(CLKID_PERIPHERAL_BASE + 80)
#define CLKID_WAVE_C			(CLKID_PERIPHERAL_BASE + 81)
#define CLKID_JPEG_PRE_MUX		(CLKID_PERIPHERAL_BASE + 82)
#define CLKID_JPEG_PRE_DIV		(CLKID_PERIPHERAL_BASE + 83)
#define CLKID_JPEG_MUX			(CLKID_PERIPHERAL_BASE + 84)
#define CLKID_JPEG			(CLKID_PERIPHERAL_BASE + 85)
#define CLKID_MIPI_CSI_PHY_PRE_MUX	(CLKID_PERIPHERAL_BASE + 86)
#define CLKID_MIPI_CSI_PHY_PRE_DIV	(CLKID_PERIPHERAL_BASE + 87)
#define CLKID_MIPI_CSI_PHY_MUX		(CLKID_PERIPHERAL_BASE + 88)
#define CLKID_MIPI_CSI_PHY		(CLKID_PERIPHERAL_BASE + 89)
#define CLKID_MIPI_ISP_PRE_MUX		(CLKID_PERIPHERAL_BASE + 90)
#define CLKID_MIPI_ISP_PRE_DIV		(CLKID_PERIPHERAL_BASE + 91)
#define CLKID_MIPI_ISP_MUX		(CLKID_PERIPHERAL_BASE + 92)
#define CLKID_MIPI_ISP			(CLKID_PERIPHERAL_BASE + 93)
#define CLKID_NNA_AXI_PRE_MUX		(CLKID_PERIPHERAL_BASE + 94)
#define CLKID_NNA_AXI_PRE_DIV		(CLKID_PERIPHERAL_BASE + 95)
#define CLKID_NNA_AXI_MUX		(CLKID_PERIPHERAL_BASE + 96)
#define CLKID_NNA_AXI			(CLKID_PERIPHERAL_BASE + 97)
#define CLKID_NNA_CORE_PRE_MUX		(CLKID_PERIPHERAL_BASE + 98)
#define CLKID_NNA_CORE_PRE_DIV		(CLKID_PERIPHERAL_BASE + 99)
#define CLKID_NNA_CORE_MUX		(CLKID_PERIPHERAL_BASE + 100)
#define CLKID_NNA_CORE			(CLKID_PERIPHERAL_BASE + 101)
#define CLKID_GDC_AXI_PRE_MUX		(CLKID_PERIPHERAL_BASE + 102)
#define CLKID_GDC_AXI_PRE_DIV		(CLKID_PERIPHERAL_BASE + 103)
#define CLKID_GDC_AXI_MUX		(CLKID_PERIPHERAL_BASE + 104)
#define CLKID_GDC_AXI			(CLKID_PERIPHERAL_BASE + 105)
#define CLKID_GDC_CORE_PRE_MUX		(CLKID_PERIPHERAL_BASE + 106)
#define CLKID_GDC_CORE_PRE_DIV		(CLKID_PERIPHERAL_BASE + 107)
#define CLKID_GDC_CORE_MUX		(CLKID_PERIPHERAL_BASE + 108)
#define CLKID_GDC_CORE			(CLKID_PERIPHERAL_BASE + 109)
#define CLKID_GE2D_PRE_MUX		(CLKID_PERIPHERAL_BASE + 110)
#define CLKID_GE2D_PRE_DIV		(CLKID_PERIPHERAL_BASE + 111)
#define CLKID_GE2D_MUX			(CLKID_PERIPHERAL_BASE + 112)
#define CLKID_GE2D			(CLKID_PERIPHERAL_BASE + 113)
#define CLKID_ETH_125M_MUX		(CLKID_PERIPHERAL_BASE + 114)
#define CLKID_ETH_125M_DIV		(CLKID_PERIPHERAL_BASE + 115)
#define CLKID_ETH_125M			(CLKID_PERIPHERAL_BASE + 116)
#define CLKID_ETH_RMII_MUX		(CLKID_PERIPHERAL_BASE + 117)
#define CLKID_ETH_RMII_DIV		(CLKID_PERIPHERAL_BASE + 118)
#define CLKID_ETH_RMII			(CLKID_PERIPHERAL_BASE + 119)

#define CLKID_END_BASE			(CLKID_PERIPHERAL_BASE + 120)
#endif /* __C1_CLKC_H */