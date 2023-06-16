/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __T5D_H
#define __T5D_H

/*
 * Clock controller register offsets
 *
 * Register offsets from the data sheet are listed in comment blocks below.
 * Those offsets must be multiplied by 4 before adding them to the base address
 * to get the right value
 */
#define HHI_GP0_PLL_CNTL0		0x40 /* 0x10 offset in datasheet very*/
#define HHI_GP0_PLL_CNTL1		0x44 /* 0x11 offset in datasheet */
#define HHI_GP0_PLL_CNTL2		0x48 /* 0x12 offset in datasheet */
#define HHI_GP0_PLL_CNTL3		0x4c /* 0x13 offset in datasheet */
#define HHI_GP0_PLL_CNTL4		0x50 /* 0x14 offset in datasheet */
#define HHI_GP0_PLL_CNTL5		0x54 /* 0x15 offset in datasheet */
#define HHI_GP0_PLL_CNTL6		0x58 /* 0x16 offset in datasheet */
#define HHI_GP0_PLL_STS			0x5c /* 0x17 offset in datasheet */

#define HHI_HIFI_PLL_CNTL0		0xd8 /* 0x36 offset in datasheet very*/
#define HHI_HIFI_PLL_CNTL1		0xdc /* 0x37 offset in datasheet */
#define HHI_HIFI_PLL_CNTL2		0xe0 /* 0x38 offset in datasheet */
#define HHI_HIFI_PLL_CNTL3		0xe4 /* 0x39 offset in datasheet */
#define HHI_HIFI_PLL_CNTL4		0xe8 /* 0x3a offset in datasheet */
#define HHI_HIFI_PLL_CNTL5		0xec /* 0x3b offset in datasheet */
#define HHI_HIFI_PLL_CNTL6		0xf0 /* 0x3c offset in datasheet */
#define HHI_HIFI_PLL_STS		0xf4 /* 0x3d offset in datasheet very*/

#define HHI_GCLK_MPEG0			0xc0 /* 0x30 offset in datasheet */
#define HHI_GCLK_MPEG1			0xc4 /* 0x31 offset in datasheet */
#define HHI_GCLK_MPEG2			0xc8 /* 0x32 offset in datasheet */
#define HHI_GCLK_OTHER			0xd0 /* 0x34 offset in datasheet */
#define HHI_GCLK_AO                     0x154

#define HHI_GCLK_SP_MPEG		0x154 /* 0x55 offset in datasheet */
#define HHI_SYS_CPU_CLK_CNTL1		0x15C /* 0x57 offset in datasheet1 */
#define HHI_SYS_CPU_CLK_CNTL0		0x19c /* 0x67 offset in datasheet */
#define HHI_MPLL_CNTL0			0x278 /* 0x9e offset in datasheet very*/
#define HHI_MPLL_CNTL1			0x27c /* 0x9f offset in datasheet */
#define HHI_MPLL_CNTL2			0x280 /* 0xa0 offset in datasheet */
#define HHI_MPLL_CNTL3			0x284 /* 0xa1 offset in datasheet */
#define HHI_MPLL_CNTL4			0x288 /* 0xa2 offset in datasheet */
#define HHI_MPLL_CNTL5			0x28c /* 0xa3 offset in datasheet */
#define HHI_MPLL_CNTL6			0x290 /* 0xa4 offset in datasheet */
#define HHI_MPLL_CNTL7			0x294 /* 0xa5 offset in datasheet */
#define HHI_MPLL_CNTL8			0x298 /* 0xa6 offset in datasheet */
#define HHI_MPLL_STS			0x29c /* 0xa7 offset in datasheet very*/
#define HHI_FIX_PLL_CNTL0		0x2a0 /* 0xa8 offset in datasheet very*/
#define HHI_FIX_PLL_CNTL1		0x2a4 /* 0xa9 offset in datasheet */
#define HHI_FIX_PLL_CNTL2		0x2a8 /* 0xaa offset in datasheet */
#define HHI_FIX_PLL_CNTL3		0x2ac /* 0xab offset in datasheet */
#define HHI_FIX_PLL_CNTL4		0x2b0 /* 0xac offset in datasheet */
#define HHI_FIX_PLL_CNTL5		0x2b4 /* 0xad offset in datasheet */
#define HHI_FIX_PLL_CNTL6		0x2b8 /* 0xae offset in datasheet */
#define HHI_FIX_PLL_STS			0x2bc /* 0xaf offset in datasheet very*/
#define HHI_MPLL3_CNTL0			0x2E0 /* 0xb8 offset in datasheet */
#define HHI_MPLL3_CNTL1			0x2E4 /* 0xb9 offset in datasheet */
#define HHI_PLL_TOP_MISC		0x2E8 /* 0xba offset in datasheet */
#define HHI_ADC_PLL_CNTL		0x2A8 /* 0xaa offset in datasheet */
#define HHI_ADC_PLL_CNTL2		0x2AC /* 0xab offset in datasheet */
#define HHI_ADC_PLL_CNTL3		0x2B0 /* 0xac offset in datasheet */
#define HHI_ADC_PLL_CNTL4		0x2B4 /* 0xad offset in datasheet */
#define HHI_HDMIRX_AXI_CLK_CNTL		0x2E0 /* 0xb8 offset in datasheet */
#define HHI_SYS_PLL_CNTL0		0x2f4 /* 0xbd offset in datasheet */
#define HHI_SYS_PLL_CNTL1		0x2f8 /* 0xbe offset in datasheet */
#define HHI_SYS_PLL_CNTL2		0x2fc /* 0xbf offset in datasheet */
#define HHI_SYS_PLL_CNTL3		0x300 /* 0xc0 offset in datasheet */
#define HHI_SYS_PLL_CNTL4		0x304 /* 0xc1 offset in datasheet */
#define HHI_SYS_PLL_CNTL5		0x308 /* 0xc2 offset in datasheet */
#define HHI_SYS_PLL_CNTL6		0x30c /* 0xc3 offset in datasheet */
#define HHI_HDMI_PLL_CNTL0		0x320 /* 0xc8 offset in datasheet */
#define HHI_HDMI_PLL_CNTL1		0x324 /* 0xc9 offset in datasheet */
#define HHI_HDMI_PLL_CNTL2		0x328 /* 0xca offset in datasheet */
#define HHI_HDMI_PLL_CNTL3		0x32C /* 0xcb offset in datasheet */
#define HHI_HDMI_PLL_CNTL4		0x330 /* 0xcc offset in datasheet */
#define HHI_HDMI_PLL_CNTL5		0x334 /* 0xcd offset in datasheet */
#define HHI_HDMI_PLL_CNTL6		0x338 /* 0xce offset in datasheet */
#define HHI_HDMI_PLL_STS		0x33c /* 0xcf offset in datasheet */

#define HHI_CHECK_CLK_RESULT		(0x04 << 2)
#define HHI_VIID_CLK_DIV		(0x4a << 2)
#define HHI_VIID_CLK_CNTL		(0x4b << 2)
#define HHI_VID_CLK_DIV			(0x59 << 2)
#define HHI_MPEG_CLK_CNTL		(0x5d << 2)
#define HHI_VID_CLK_CNTL		(0x5f << 2)
#define HHI_TSIN_DEGLITCH_CLK_CNTL	(0x60 << 2)
#define HHI_TS_CLK_CNTL			(0x64 << 2)
#define HHI_VID_CLK_CNTL2		(0x65 << 2)
#define HHI_MALI_CLK_CNTL		(0x6c << 2)
#define HHI_VPU_CLKC_CNTL		(0x6d << 2)
#define HHI_VPU_CLK_CNTL		(0x6f << 2)
#define HHI_DEMOD_CLK_CNTL		(0x74 << 2)
#define HHI_DEMOD_CLK_CNTL1		(0x75 << 2) /* T5D newly*/
#define HHI_ETH_CLK_CNTL		(0x76 << 2)
#define HHI_VDEC_CLK_CNTL		(0x78 << 2)
#define HHI_VDEC2_CLK_CNTL		(0x79 << 2)
#define HHI_VDEC3_CLK_CNTL		(0x7a << 2)
#define HHI_VDEC4_CLK_CNTL		(0x7b << 2)
#define HHI_HDCP22_CLK_CNTL		(0x7c << 2)
#define HHI_VAPBCLK_CNTL		(0x7d << 2)
#define HHI_HDMIRX_CLK_CNTL		(0x80 << 2)
#define HHI_HDMIRX_AUD_CLK_CNTL		(0x81 << 2)
#define HHI_VPU_CLKB_CNTL		(0x83 << 2)
#define HHI_GEN_CLK_CNTL		(0x8a << 2)
#define HHI_AUDPLL_CLK_OUT_CNTL		(0x8c << 2)
#define HHI_HDMIRX_METER_CLK_CNTL	(0x8d << 2)
#define HHI_VDIN_MEAS_CLK_CNTL		(0x94 << 2)
#define HHI_NAND_CLK_CNTL		(0x97 << 2)
#define HHI_TCON_CLK_CNTL		(0x9c << 2)
#define HHI_HDMI_AXI_CLK_CNTL		(0xb8 << 2)
#define HHI_VID_LOCK_CLK_CNTL		(0xf2 << 2)
#define HHI_ATV_DMD_SYS_CLK_CNTL	(0xf3 << 2)
#define HHI_CDAC_CLK_CNTL		(0xf6 << 2)
#define HHI_SPICC_CLK_CNTL		(0xf7 << 2)

/* include the CLKIDs that have been made part of the DT binding */
#include <dt-bindings/clock/t5d-clkc.h>

#endif /* __T5D_H */
