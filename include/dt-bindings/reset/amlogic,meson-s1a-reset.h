/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _DT_BINDINGS_AMLOGIC_MESON_S1A_RESET_H
#define _DT_BINDINGS_AMLOGIC_MESON_S1A_RESET_H

/* RESET0 */
/*					0-3	*/
#define RESET_USBCTRL			4
#define RESET_U2DRDX0			5
/*					6-7	*/
#define RESET_USBPHY20			8
/*					9-14	*/
#define RESET_HDMI20_AES		15
#define RESET_HDMITX_APB		16
#define RESET_BRG_VCBUS_DEC		17
#define RESET_VCBUS			18
#define RESET_VID_PLL_DIV		19
#define RESET_VIDEO6			20
#define RESET_GE2D			21
#define RESET_HDMITXPHY			22
#define RESET_VID_LOCK			23
#define RESET_VENCL			24
#define RESET_VDAC			25
#define RESET_VENCP			26
#define RESET_VENCI			27
#define RESET_RDMA			28
#define RESET_HDMI_TX			29
#define RESET_VIU			30
#define RESET_VENC			31

/* RESET1 */
#define RESET_AUDIO			32
/*					33-36	*/
#define RESET_DOS_APB			37
#define RESET_DOS			38
/*					39-47	*/
#define RESET_ETH			48
/*					49-51	*/
#define RESET_DEMOD			52
/*					53-63	*/

/* RESET2 */
#define RESET_AM2AXI			64
#define RESET_IR_CTRL			65
/*					66	*/
#define RESET_TEMPSENSOR_PLL		67
/*					68-71	*/
#define RESET_SMART_CARD		72
/*					73-79	*/
#define RESET_MSR_CLK			80
/*					81	*/
#define RESET_SARADC			82
/*					83-87	*/
#define RESET_ACODEC			88
#define RESET_CEC			89
/*					90	*/
#define RESET_WATCHDOG			91
/*					92-95	*/

/* RESET3 */
/* 96 ~ 127 */

/* RESET4 */
/*					128-131	*/
#define RESET_PWM_AB			132
#define RESET_PWM_CD			133
/*					134-137	*/
#define RESET_UART_A			138
#define RESET_UART_B			139
/*					140-143	*/
#define RESET_I2C_M_A			145
#define RESET_I2C_M_B			146
#define RESET_I2C_M_C			147
/*					148-153	*/
#define RESET_SDEMMC			154
/*					155-159	*/

/* RESET5 */
#define RESET_VDEC_PIPEL		160
#define RESET_HEVCF_DMC_PIPEL		161
#define RESET_GE2D_DMC_PIPEL		162
#define RESET_VPU_DMC_PIPEL		163
#define RESET_A53_DMC_PIPEL		164
#define RESET_BRG_HEVCF_PIPEL		165
/*					166-185	*/
#define RESET_BRG_NIC_EMMC		186
#define RESET_BRG_NIC_VAPB		187
#define RESET_BRG_NIC_DSU		188
#define RESET_BRG_NIC_SYSCLK		189
#define RESET_BRG_NIC_MAIN		190
#define RESET_BRG_NIC_ALL		191

#endif
