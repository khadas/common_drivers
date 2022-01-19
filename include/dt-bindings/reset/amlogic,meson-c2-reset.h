/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _DT_BINDINGS_AMLOGIC_MESON_C2_RESET_H
#define _DT_BINDINGS_AMLOGIC_MESON_C2_RESET_H

/* RESET0 */
#define RESET_AM2AXI_VAD		1
#define RESET_PWM_IJ			2
#define RESET_PWM_GH			3
#define RESET_PAD_CTRL			5
#define RESET_TEMP_SENSOR		7
#define RESET_AM2AXI_DEV		8
#define RESET_SPICC_B			9
#define RESET_SPICC_A			10
#define RESET_MSR_CLK			11
#define RESET_AUDIO			12
#define RESET_ANALOG_CTRL		13
#define RESET_SAR_ADC			14
#define RESET_AUDIO_VAD			15
#define RESET_PWM_EF			17
#define RESET_PWM_CD			18
#define RESET_PWM_AB			19
#define RESET_I2C_CTRL			21
#define RESET_I2C_S_A			22
#define RESET_I2C_M_E			23
#define RESET_I2C_M_D			24
#define RESET_I2C_M_C			25
#define RESET_I2C_M_B			26
#define RESET_I2C_M_A			27
#define RESET_I2C_PROD_AHB		28
#define RESET_I2C_PROD			29
#define RESET_UART_E			30
#define RESET_UART_D			31

/* RESET1 */
#define RESET_ACODEC			32
#define RESET_DMA			33
#define RESET_SD_EMMC_A			34
#define RESET_USBCTRL			35
#define RESET_USBPHY			36
#define RESET_RSA			37
#define RESET_MIPI_CSI2_PHY1		38
#define RESET_MIPI_CSI2_PHY0		39
#define RESET_MIPI_ISP			40
#define RESET_GDC			41
#define RESET_GE2D			42
#define RESET_DMC			43
#define RESET_CC			44
#define RESET_IRQ_CTRL			45
#define RESET_BC			46
#define RESET_NIC_VAD			47
#define RESET_RAMA			49
#define RESET_NNA			52
#define RESET_ROM			53
#define RESET_SPIFC			54
#define RESET_GIC			55
#define RESET_UART_C			56
#define RESET_UART_B			57
#define RESET_UART_A			58
#define RESET_OSC_RING			59
#define RESET_DOS_TOP			60
#define RESET_ETH			61
#define RESET_SD_EMMC_B			62
#define RESET_SD_EMMC_C			63

/* RESET2 */
#define RESET_NIC_AXI_WAVE		83
#define RESET_NIC_AIX_USB		84
#define RESET_NIC_AXI_SYS		85
#define	RESET_NIC_AXI_SE		86
#define	RESET_NIC_AXI_NNA		87
#define RESET_NIC_AXI_JPEG		88
#define	RESET_NIC_AXI_ISP		89
#define	RESET_NIC_AXI_GE2D		90
#define	RESET_NIC_AXI_GDC		91
#define RESET_NIC_AXI_DSPA		92
#define RESET_NIC_AXI_CPU		93
#define RESET_NIC_AXI_AXI		94
#define RESET_NIC_ALL			95

#endif
