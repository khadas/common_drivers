/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __C3_H
#define __C3_H

//  CPU_CTRL
//========================================================================
// -----------------------------------------------
// REG_BASE:  REGISTER_BASE_ADDR = 0xfe00e000
// -----------------------------------------------
#define CPUCTRL_RESET_CNTL                 ((0x0000  << 2))
#define CPUCTRL_CLK_CTRL0                  ((0x0001  << 2))
#define CPUCTRL_CLK_CTRL1                  ((0x0002  << 2))
#define CPUCTRL_CLK_CTRL2                  ((0x0003  << 2))
#define CPUCTRL_CLK_CTRL3                  ((0x0004  << 2))
#define CPUCTRL_CLK_CTRL4                  ((0x0005  << 2))
#define CPUCTRL_CLK_CTRL5                  ((0x0006  << 2))
#define CPUCTRL_CLK_CTRL6                  ((0x0007  << 2))

#define ANACTRL_SYSPLL_CTRL0                       ((0x0000  << 2))
#define ANACTRL_SYSPLL_CTRL1                       ((0x0001  << 2))
#define ANACTRL_SYSPLL_CTRL2                       ((0x0002  << 2))
#define ANACTRL_SYSPLL_CTRL3                       ((0x0003  << 2))
#define ANACTRL_SYSPLL_CTRL4                       ((0x0004  << 2))
#define ANACTRL_SYSPLL_CTRL5                       ((0x0005  << 2))
#define ANACTRL_SYSPLL_CTRL6                       ((0x0006  << 2))
#define ANACTRL_SYSPLL_STS                         ((0x0007  << 2))
#define ANACTRL_FIXPLL_CTRL0                       ((0x0010  << 2))
#define ANACTRL_FIXPLL_CTRL1                       ((0x0011  << 2))
#define ANACTRL_FIXPLL_CTRL2                       ((0x0012  << 2))
#define ANACTRL_FIXPLL_CTRL3                       ((0x0013  << 2))
#define ANACTRL_FIXPLL_CTRL4                       ((0x0014  << 2))
#define ANACTRL_FIXPLL_CTRL5                       ((0x0015  << 2))
#define ANACTRL_FIXPLL_CTRL6                       ((0x0016  << 2))
#define ANACTRL_FIXPLL_STS                         ((0x0017  << 2))
#define ANACTRL_GP0PLL_CTRL0                       ((0x0020  << 2))
#define ANACTRL_GP0PLL_CTRL1                       ((0x0021  << 2))
#define ANACTRL_GP0PLL_CTRL2                       ((0x0022  << 2))
#define ANACTRL_GP0PLL_CTRL3                       ((0x0023  << 2))
#define ANACTRL_GP0PLL_CTRL4                       ((0x0024  << 2))
#define ANACTRL_GP0PLL_CTRL5                       ((0x0025  << 2))
#define ANACTRL_GP0PLL_CTRL6                       ((0x0026  << 2))
#define ANACTRL_GP0PLL_STS                         ((0x0027  << 2))
#define ANACTRL_GP1PLL_CTRL0                       ((0x0030  << 2))
#define ANACTRL_GP1PLL_CTRL1                       ((0x0031  << 2))
#define ANACTRL_GP1PLL_CTRL2                       ((0x0032  << 2))
#define ANACTRL_GP1PLL_CTRL3                       ((0x0033  << 2))
#define ANACTRL_GP1PLL_CTRL4                       ((0x0034  << 2))
#define ANACTRL_GP1PLL_CTRL5                       ((0x0035  << 2))
#define ANACTRL_GP1PLL_CTRL6                       ((0x0036  << 2))
#define ANACTRL_GP1PLL_STS                         ((0x0037  << 2))
#define ANACTRL_HIFIPLL_CTRL0                      ((0x0040  << 2))
#define ANACTRL_HIFIPLL_CTRL1                      ((0x0041  << 2))
#define ANACTRL_HIFIPLL_CTRL2                      ((0x0042  << 2))
#define ANACTRL_HIFIPLL_CTRL3                      ((0x0043  << 2))
#define ANACTRL_HIFIPLL_CTRL4                      ((0x0044  << 2))
#define ANACTRL_HIFIPLL_CTRL5                      ((0x0045  << 2))
#define ANACTRL_HIFIPLL_CTRL6                      ((0x0046  << 2))
#define ANACTRL_HIFIPLL_STS                        ((0x0047  << 2))
#define ANACTRL_MPLL_CTRL0                         ((0x0060  << 2))
#define ANACTRL_MPLL_CTRL1                         ((0x0061  << 2))
#define ANACTRL_MPLL_CTRL2                         ((0x0062  << 2))
#define ANACTRL_MPLL_CTRL3                         ((0x0063  << 2))
#define ANACTRL_MPLL_CTRL4                         ((0x0064  << 2))
#define ANACTRL_MPLL_CTRL5                         ((0x0065  << 2))
#define ANACTRL_MPLL_CTRL6                         ((0x0066  << 2))
#define ANACTRL_MPLL_CTRL7                         ((0x0067  << 2))
#define ANACTRL_MPLL_CTRL8                         ((0x0068  << 2))
#define ANACTRL_MPLL_STS                           ((0x0069  << 2))
#define ANACTRL_MIPIDSI_CTRL0                      ((0x00a0  << 2))
#define ANACTRL_MIPIDSI_CTRL1                      ((0x00a1  << 2))
#define ANACTRL_MIPIDSI_CTRL2                      ((0x00a2  << 2))
#define ANACTRL_MIPIDSI_STS                        ((0x00a3  << 2))
// REG_BASE:  REGISTER_BASE_ADDR = 0xfe000000
// -----------------------------------------------
#define OSCIN_CTRL                         ((0x0001  << 2))
#define RTC_BY_OSCIN_CTRL0                 ((0x0002  << 2))
#define RTC_BY_OSCIN_CTRL1                 ((0x0003  << 2))
#define RTC_CTRL                           ((0x0004  << 2))
#define CHECK_CLK_RESULT                   ((0x0005  << 2))
#define MBIST_ATSPEED_CTRL                 ((0x0006  << 2))
#define LOCK_BIT_REG0                      ((0x0008  << 2))
#define LOCK_BIT_REG1                      ((0x0009  << 2))
#define LOCK_BIT_REG2                      ((0x000a  << 2))
#define LOCK_BIT_REG3                      ((0x000b  << 2))
#define PROT_BIT_REG0                      ((0x000c  << 2))
#define PROT_BIT_REG1                      ((0x000d  << 2))
#define PROT_BIT_REG2                      ((0x000e  << 2))
#define PROT_BIT_REG3                      ((0x000f  << 2))
#define SYS_CLK_CTRL0                      ((0x0010  << 2))
#define SYS_CLK_EN0_REG0                   ((0x0011  << 2))
#define SYS_CLK_EN0_REG1                   ((0x0012  << 2))
#define SYS_CLK_EN0_REG2                   ((0x0013  << 2))
#define SYS_CLK_EN0_REG3                   ((0x0014  << 2))
#define SYS_CLK_EN1_REG0                   ((0x0015  << 2))
#define SYS_CLK_EN1_REG1                   ((0x0016  << 2))
#define SYS_CLK_EN1_REG2                   ((0x0017  << 2))
#define SYS_CLK_EN1_REG3                   ((0x0018  << 2))
#define SYS_CLK_VPU_EN0                    ((0x0019  << 2))
#define SYS_CLK_VPU_EN1                    ((0x001a  << 2))
#define AXI_CLK_CTRL0                      ((0x001b  << 2))
#define SYSOSCIN_CTRL                      ((0x001c  << 2))
#define TST_CTRL0                          ((0x0020  << 2))
#define TST_CTRL1                          ((0x0021  << 2))
#define CECA_CTRL0                         ((0x0022  << 2))
#define CECA_CTRL1                         ((0x0023  << 2))
#define CECB_CTRL0                         ((0x0024  << 2))
#define CECB_CTRL1                         ((0x0025  << 2))
#define SC_CLK_CTRL                        ((0x0026  << 2))
#define DSPA_CLK_CTRL0                     ((0x0027  << 2))
//`define DSPB_CLK_CTRL0          10'h28
#define RAMA_CLK_CTRL0                     ((0x0029  << 2))
#define CLK12_24_CTRL                      ((0x002a  << 2))
#define AXI_CLK_EN0                        ((0x002b  << 2))
#define AXI_CLK_EN1                        ((0x002c  << 2))
//`define RTCPLL_CTRL0            10'h2b
//`define RTCPLL_CTRL1            10'h2c
//`define RTCPLL_CTRL2            10'h2d
//`define RTCPLL_CTRL3            10'h2e
//`define RTCPLL_CTRL4            10'h2f
//`define RTCPLL_STS              10'h28
#define VID_CLK_CTRL                       ((0x0030  << 2))
#define VID_CLK_CTRL2                      ((0x0031  << 2))
#define VID_CLK_DIV                        ((0x0032  << 2))
#define VIID_CLK_DIV                       ((0x0033  << 2))
#define VIID_CLK_CTRL                      ((0x0034  << 2))
#define HDMI_CLK_CTRL                      ((0x0038  << 2))
#define VID_PLL_CLK_DIV                    ((0x0039  << 2))
#define VPU_CLK_CTRL                       ((0x003a  << 2))
#define VPU_CLKB_CTRL                      ((0x003b  << 2))
#define VPU_CLKC_CTRL                      ((0x003c  << 2))
#define VID_LOCK_CLK_CTRL                  ((0x003d  << 2))
#define VDIN_MEAS_CLK_CTRL                 ((0x003e  << 2))
#define VAPB_CLK_CTRL                      ((0x003f  << 2))
#define HDCP22_CLK_CTRL                    ((0x0040  << 2))
#define MIPIDSI_PHY_CLK_CTRL               ((0x0041  << 2))
#define CDAC_CLK_CTRL                      ((0x0042  << 2))
#define GE2D_CLK_CTRL                      ((0x0043  << 2))
//`define MIPI_CSI_PHY_CLK_CTRL   10'h43
//`define CSI2_ADAPT_CLK_CTRL     10'h44
#define ISP0_CLK_CTRL                      ((0x0044  << 2))
#define DEWARPA_CLK_CTRL                   ((0x0045  << 2))
#define VOUTENC_CLK_CTRL                   ((0x0046  << 2))
#define VDEC_CLK_CTRL                      ((0x0050  << 2))
#define VDEC2_CLK_CTRL                     ((0x0051  << 2))
#define VDEC3_CLK_CTRL                     ((0x0052  << 2))
#define VDEC4_CLK_CTRL                     ((0x0053  << 2))
//`define WAVE420L_CLK_CTRL       10'h54
//`define WAVE420L_CLK_CTRL2      10'h55
#define TS_CLK_CTRL                        ((0x0056  << 2))
#define MALI_CLK_CTRL                      ((0x0057  << 2))
//`define VIPNANOQ_CLK_CTRL       10'h58
#define ETH_CLK_CTRL                       ((0x0059  << 2))
#define NAND_CLK_CTRL                      ((0x005a  << 2))
#define SD_EMMC_CLK_CTRL                   ((0x005b  << 2))
//`define BT656_CLK_CTRL          10'h5C
#define SPICC_CLK_CTRL                     ((0x005d  << 2))
#define GEN_CLK_CTRL                       ((0x005e  << 2))
#define SAR_CLK_CTRL0                      ((0x005f  << 2))
#define PWM_CLK_AB_CTRL                    ((0x0060  << 2))
#define PWM_CLK_CD_CTRL                    ((0x0061  << 2))
#define PWM_CLK_EF_CTRL                    ((0x0062  << 2))
#define PWM_CLK_GH_CTRL                    ((0x0063  << 2))
#define PWM_CLK_IJ_CTRL                    ((0x0064  << 2))
#define PWM_CLK_KL_CTRL                    ((0x0065  << 2))
#define PWM_CLK_MN_CTRL                    ((0x0066  << 2))
#define VC9000E_CLK_CTRL                   ((0x0067  << 2))
#define SPIFC_CLK_CTRL                     ((0x0068  << 2))
#define DEMOD_CLK_CTRL                     ((0x0080  << 2))
#define NNA_CLK_CTRL                       ((0x0088  << 2))
#define TIMESTAMP_CTRL                     ((0x0100  << 2))
#define TIMESTAMP_CTRL1                    ((0x0101  << 2))
#define TIMESTAMP_CTRL2                    ((0x0103  << 2))
#define TIMESTAMP_RD0                      ((0x0104  << 2))
#define TIMESTAMP_RD1                      ((0x0105  << 2))
#define TIMEBASE_CTRL0                     ((0x0106  << 2))
#define TIMEBASE_CTRL1                     ((0x0107  << 2))
#define EFUSE_CPU_CFG01                    ((0x0120  << 2))
#define EFUSE_CPU_CFG2                     ((0x0121  << 2))
#define EFUSE_ENCP_CFG0                    ((0x0122  << 2))
#define EFUSE_MALI_CFG01                   ((0x0123  << 2))
//`define EFUSE_HEVCB_CFG01       10'h124
//`define EFUSE_HEVCB_CFG2        10'h125
#define EFUSE_LOCK                         ((0x0126  << 2))
//========================================================================

#include <dt-bindings/clock/c3-clkc.h>
#define NR_CLKS				(CLKID_END_BASE)

#define SECURE_PLL_CLK			0x82000098
#define SECURE_CPU_CLK			0x82000099

/* PLL secure clock index */
enum sec_pll {
	SECID_SYS_DCO_PLL = 0,
	SECID_SYS_DCO_PLL_DIS,
	SECID_SYS_PLL_OD,
	SECID_CPU_CLK_SEL,
	SECID_CPU_CLK_RD,
	SECID_CPU_CLK_DYN,
	SECID_DSU_PRE_CLK_SEL,
	SECID_DSU_PRE_CLK_RD,
	SECID_DSU_PRE_CLK_DYN,
	SECID_DSU_CLK_SEL,
	SECID_DSU_CLK_RD,
	SECID_GP1_DCO_PLL,
	SECID_GP1_DCO_PLL_DIS,
	SECID_GP1_PLL_OD,
};

extern const struct clk_ops meson_c3_clk_pll_ro_ops;
extern const struct clk_ops meson_c3_clk_pll_ops;
extern const struct clk_ops meson_c3_clk_hifi_pll_ops;
extern const struct clk_ops meson_c3_clk_gp_pll_ops;
#endif /* __C3_H */
