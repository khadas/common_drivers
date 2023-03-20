/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __VDIN_T3X_REGS_H
#define __VDIN_T3X_REGS_H

#define VDIN_TOP_OFFSET		     0x10
#define VDIN_REG_TOP_START_T3X       0x0100
#define VDIN_REG_TOP_END_T3X         0x0199
#define VDIN_REG_BANK0_START_T3X     0x0200
#define VDIN_REG_BANK0_END_T3X       0x027b
#define VDIN_REG_BANK1_START_T3X     0x0300
#define VDIN_REG_BANK1_END_T3X       0x03fd

enum vdin_vdi_x_t3x_e {
	VDIN_VDI0_MPEG_T3X		= 0,	/* mpeg in */
	VDIN_VDI1_BT656_T3X		= 1,	/* first bt656 input */
	VDIN_VDI2_RESERVED_T3X		= 2,	/* reserved */
	VDIN_VDI3_TV_DECODE_IN_T3X	= 3,	/* tv decode input */
	VDIN_VDI4A_HDMIRX_T3X		= 4,	/* hdmi rx dual pixel input */
	VDIN_VDI4B_HDMIRX_T3X		= 5,	/* hdmi rx dual pixel input */
	VDIN_VDI5_DVI_T3X		= 6,	/* digital video input */
	VDIN_VDI6_LOOPBACK_1_T3X	= 7,	/* first internal loopback input */
	VDIN_VDI7_MIPI_CSI2_T3X		= 8,	/* mipi csi2 input */
	VDIN_VDI8_LOOPBACK_2_T3X	= 9,	/* second internal loopback input */
	VDIN_VDI9_SECOND_BT656_T3X	= 10,	/* second bt656 input */
	VDIN_VDI_NULL_T3X
};

/* t3x vdin_preproc begin */
#define VPU_VDIN_HDMI0_CTRL0                       0x272c
// Bit 0        reg_hdmi_en
// Bit 1        reg_dsc_en
// Bit 3:2      reg_in_ppc      :   0:hdmi rx  1:dsc 1ppc  2:dsc 2ppc  3:dsc 4ppc
// Bit 5:4      reg_out_ppc     :   0: 1ppc  1:2ppc
// Bit 7:6      reg_dsc_ppc     :   0:rgb 1:422 2:444  3:420
// Bit 8        reg_fmt_set     :
// 1: when hdmi rx input (not dsc), use register instead of hdmi rx input signals
// Bit 15:12    wr_rate_pre
// Bit 19:16    rd_rate_dcp
// Bit 21:20    reg_din_mux
// Bit 26:24    reg_din_swap
// Bit 27       reg_dis_auto    :   //disable auto
// Bit 29:28    reg_cd_cnt_max  :   slow down speed de out
#define VPU_VDIN_HDMI0_CTRL1                       0x272d
// Bit 0        reg_hs_pol
// Bit 1        reg_vs_pol
// Bit 2        reg_uv_swap     :   0: u first   1:v first
// Bit 3        reg_rdwin_mode
// Bit 5:4      reg_hskip_mode  :   0: h skip disable
// 1: h skip enable , average mode    2: drop right pixel   3: drop left pixel
// Bit 6        reg_avg_mode    :   only 420
// Bit 7        reg_vskip_en
// Bit 15:8     reg_rdwin_auto
// Bit 27:16    reg_rdwin_manual
// Bit 28       reg_dbv422_mode
// Bit 29       reg_afifo_rate
// Bit 30       reg_upsmp_en    :   0:disable   1: 4ppc to 2ppc (not skip)
// bit 31       disable_rst_afifo
#define VPU_VDIN_HDMI1_CTRL0                       0x272e
// Bit 27:24    reg_yuv_swap
#define VPU_VDIN_HDMI1_CTRL1                       0x272f
// Bit 0        reg_hs_pol
// Bit 1        reg_vs_pol
// Bit 2        reg_uv_swap     :   0: u first   1:v first
// Bit 3        reg_rdwin_mode
// Bit 5:4      reg_hskip_mode  :   0: h skip disable
// 1: h skip enable , average mode    2: drop right pixel   3: drop left pixel
// Bit 6        reg_avg_mode    :   only 420
// Bit 7        reg_vskip_en
// Bit 15:8     reg_rdwin_auto
// Bit 27:16    reg_rdwin_manual
// Bit 28       reg_dbv422_mode
// Bit 29       reg_afifo_rate
// Bit 30       reg_upsmp_en    :   0:disable   1: 4ppc to 2ppc (not skip)
// bit 31       disable_rst_afifo
#define VPU_VDIN_HDMI0_TUNNEL                      0x2740
// Bit 23:0     reg_tunnel_sel  :  tunnel reorder
// Bit 24       reg_tunnel_uv   :   cbcr swap
// Bit 25       reg_1p2p_en
// Bit 29:26    reg_gclk_ctrl
// Bit 30       reg_tunnel_rnd  :   output rounding
// Bit 31       reg_tunnel_en   :   tunnel enable
#define VPU_VDIN_HDMI1_TUNNEL                      0x2741
// Bit 23:0     reg_tunnel_sel  :  tunnel reorder
// Bit 24       reg_tunnel_uv   :   cbcr swap
// Bit 25       reg_1p2p_en
// Bit 29:26    reg_gclk_ctrl
// Bit 30       reg_tunnel_rnd  :   output rounding
// Bit 31       reg_tunnel_en   :   tunnel enable
/* t3x vdin_preproc end */

// Closing file:  ./REG_LIST_RTL.h
//
//
// Reading file:  ./vcbus_regs.h
//
// synopsys translate_off
// synopsys translate_on
//===========================================================================
// -----------------------------------------------
// CBUS_BASE:  VDIN_MISC_TOP_VCBUS_BASE = 0x01
// -----------------------------------------------
//===========================================================================
//
// Reading file:  ./vdin_misc_top_reg.h
//
#define VDIN_TOP_GCLK_CTRL                         0x0100
//Bit 31:28        reg_top_gclk_ctrl                    // unsigned ,    RW, default = 0
//Bit 27:10        reserved
//Bit  9: 8        reg_intf_gclk_ctrl                   // unsigned ,    RW, default = 0
//Bit  7: 6        reg_vdin0_gclk_ctrl                  // unsigned ,    RW, default = 0
//Bit  5: 4        reg_vdin1_gclk_ctrl                  // unsigned ,    RW, default = 0
//Bit  3: 2        reg_vdin2_gclk_ctrl                  // unsigned ,    RW, default = 0
//Bit  1: 0        reg_local_arb_gclk_ctrl              // unsigned ,    RW, default = 0
#define T3X_VDIN_TOP_CTRL                              0x0101
//Bit 31:26        reserved
//Bit 25           reg_vdin0to1_en                      // unsigned ,    RW, default = 0
//Bit 24:23        reg_hsk_mode                         // unsigned ,    RW, default = 0
//Bit 22:21        reg_vdin_out_sel                     // unsigned ,    RW, default = 0
//Bit 20           reg_dv_path_sel                      // unsigned ,    RW, default = 0
//Bit 19           reg_out_path_sel                     // unsigned ,    RW, default = 0
//Bit 18:13        reg_meas0_pol_ctrl                   // unsigned ,    RW, default = 0
//Bit 12: 7        reg_meas1_pol_ctrl                   // unsigned ,    RW, default = 0
//Bit  6: 2        reg_reset                            // unsigned ,    RW, default = 0
//Bit  1: 0        reg_line_int_sel                     // unsigned ,    RW, default = 0
#define VDIN_TOP_SECURE0_COMP0_T3X                     0x0102
//Bit 31:12        reserved
//Bit 11: 0        reg_vdin0_secure_comp0               // unsigned ,    RW, default = 0
#define VDIN_TOP_SECURE0_COMP1_T3X                     0x0103
//Bit 31:12        reserved
//Bit 11: 0        reg_vdin0_secure_comp1               // unsigned ,    RW, default = 0
#define VDIN_TOP_SECURE0_COMP2_T3X                     0x0104
//Bit 31:12        reserved
//Bit 11: 0        reg_vdin0_secure_comp2               // unsigned ,    RW, default = 0
#define VDIN_TOP_SECURE0_CTRL_T3X                      0x0105
//Bit 31: 2        reserved
//Bit  1           reg_vdin0_sec_reg_bk_keep            // unsigned ,    RW, default = 0
//secure register, need special access
//Bit  0           reg_vdin0_sec_reg_keep               // unsigned ,    RW, default = 0
//secure register, need special access
#define VDIN_TOP_SECURE0_REG_T3X                       0x0106
//Bit 31: 0        reg_vdin0_secure_reg                 // unsigned ,    RW,default = 32'hffffffff
//secure register, need special access
#define VDIN_TOP_SECURE0_MAX_SIZE_T3X                  0x0107
//Bit 31:29        reserved
//Bit 28:16        reg_vdin0_max_allow_pic_h            // unsigned ,    RW, default = 0
//secure register, need special access
//Bit 15:13        reserved
//Bit 12: 0        reg_vdin0_max_allow_pic_w            // unsigned ,    RW, default = 0
//secure register, need special access
#define VDIN_TOP_SECURE1_COMP0_T3X                     0x0108
//Bit 31:12        reserved
//Bit 11: 0        reg_vdin1_secure_comp0               // unsigned ,    RW, default = 0
#define VDIN_TOP_SECURE1_COMP1_T3X                     0x0109
//Bit 31:12        reserved
//Bit 11: 0        reg_vdin1_secure_comp1               // unsigned ,    RW, default = 0
#define VDIN_TOP_SECURE1_COMP2_T3X                     0x010a
//Bit 31:12        reserved
//Bit 11: 0        reg_vdin1_secure_comp2               // unsigned ,    RW, default = 0
#define VDIN_TOP_SECURE1_CTRL_T3X                      0x010b
//Bit 31: 2        reserved
//Bit  1           reg_vdin1_sec_reg_bk_keep            // unsigned ,    RW, default = 0
//secure register, need special access
//Bit  0           reg_vdin1_sec_reg_keep               // unsigned ,    RW, default = 0
//secure register, need special access
#define VDIN_TOP_SECURE1_REG_T3X                       0x010c
//Bit 31: 0        reg_vdin1_secure_reg                 // unsigned ,    RW,default = 32'hffffffff
//secure register, need special access
#define VDIN_TOP_SECURE1_MAX_SIZE_T3X                  0x010d
//Bit 31:29        reserved
//Bit 28:16        reg_vdin1_max_allow_pic_h            // unsigned ,    RW, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_vdin1_max_allow_pic_w            // unsigned ,    RW, default = 0
#define VDIN_TOP_SECURE2_COMP0                     0x010e
//Bit 31:12        reserved
//Bit 11: 0        reg_vdin2_secure_comp0               // unsigned ,    RW, default = 0
#define VDIN_TOP_SECURE2_COMP1                     0x010f
//Bit 31:12        reserved
//Bit 11: 0        reg_vdin2_secure_comp1               // unsigned ,    RW, default = 0
#define VDIN_TOP_SECURE2_COMP2                     0x0110
//Bit 31:12        reserved
//Bit 11: 0        reg_vdin2_secure_comp2               // unsigned ,    RW, default = 0
#define VDIN_TOP_SECURE2_CTRL                      0x0111
//Bit 31: 2        reserved
//Bit  1           reg_vdin2_sec_reg_bk_keep            // unsigned ,    RW, default = 0
//secure register, need special access
//Bit  0           reg_vdin2_sec_reg_keep               // unsigned ,    RW, default = 0
//secure register, need special access
#define VDIN_TOP_SECURE2_REG                       0x0112
//Bit 31: 0        reg_vdin2_secure_reg                 // unsigned ,    RW,default = 32'hffffffff
//secure register, need special access
#define VDIN_TOP_SECURE2_MAX_SIZE                  0x0113
//Bit 31:29        reserved
//Bit 28:16        reg_vdin2_max_allow_pic_h            // unsigned ,    RW, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_vdin2_max_allow_pic_w            // unsigned ,    RW, default = 0
#define VDIN_TOP_MEAS0_LINE                        0x0114
//Bit 31:16        reg_meas0_vf_lcnt                    // unsigned ,    RO, default = 0
//Bit 15: 0        reg_meas0_vb_lcnt                    // unsigned ,    RO, default = 0
#define VDIN_TOP_MEAS0_PIXF                        0x0115
//Bit 31: 0        reg_meas0_vf_pcnt                    // unsigned ,    RO, default = 0
#define VDIN_TOP_MEAS0_PIXB                        0x0116
//Bit 31: 0        reg_meas0_vb_pcnt                    // unsigned ,    RO, default = 0
#define VDIN_TOP_MEAS1_LINE                        0x0117
//Bit 31:16        reg_meas1_vf_lcnt                    // unsigned ,    RO, default = 0
//Bit 15: 0        reg_meas1_vb_lcnt                    // unsigned ,    RO, default = 0
#define VDIN_TOP_MEAS1_PIXF                        0x0118
//Bit 31: 0        reg_meas1_vf_pcnt                    // unsigned ,    RO, default = 0
#define VDIN_TOP_MEAS1_PIXB                        0x0119
//Bit 31: 0        reg_meas1_vb_pcnt                    // unsigned ,    RO, default = 0
#define VDIN_INTF_VDI0_CTRL                        0x0120
//Bit 31:18        reserved
//Bit 17: 8        reg_vdi0_decimate                    // unsigned ,    RW, default = 0
//Bit  7: 0        reg_vdi0_ctrl                        // unsigned ,    RW, default = 0
#define VDIN_INTF_VDI1_CTRL                        0x0121
//Bit 31:18        reserved
//Bit 17: 8        reg_vdi1_decimate                    // unsigned ,    RW, default = 0
//Bit  7: 0        reg_vdi1_ctrl                        // unsigned ,    RW, default = 0
#define VDIN_INTF_VDI2_CTRL                        0x0122
//Bit 31:18        reserved
//Bit 17: 8        reg_vdi2_decimate                    // unsigned ,    RW, default = 0
//Bit  7: 0        reg_vdi2_ctrl                        // unsigned ,    RW, default = 0
#define VDIN_INTF_VDI3_CTRL                        0x0123
//Bit 31:18        reserved
//Bit 17: 8        reg_vdi3_decimate                    // unsigned ,    RW, default = 0
//Bit  7: 0        reg_vdi3_ctrl                        // unsigned ,    RW, default = 0
#define VDIN_INTF_VDI4A_CTRL                       0x0124
//Bit 31:18        reserved
//Bit 17: 8        reg_vdi4a_decimate                   // unsigned ,    RW, default = 0
//Bit  7: 0        reg_vdi4a_ctrl                       // unsigned ,    RW, default = 0
#define VDIN_INTF_VDI4B_CTRL                       0x0125
//Bit 31:18        reserved
//Bit 17: 8        reg_vdi4b_decimate                   // unsigned ,    RW, default = 0
//Bit  7: 0        reg_vdi4b_ctrl                       // unsigned ,    RW, default = 0
#define VDIN_INTF_VDI5_CTRL                        0x0126
//Bit 31:18        reserved
//Bit 17: 8        reg_vdi5_decimate                    // unsigned ,    RW, default = 0
//Bit  7: 0        reg_vdi5_ctrl                        // unsigned ,    RW, default = 0
#define VDIN_INTF_VDI6_CTRL                        0x0127
//Bit 31:18        reserved
//Bit 17: 8        reg_vdi6_decimate                    // unsigned ,    RW, default = 0
//Bit  7: 0        reg_vdi6_ctrl                        // unsigned ,    RW, default = 0
#define VDIN_INTF_VDI7_CTRL                        0x0128
//Bit 31:18        reserved
//Bit 17: 8        reg_vdi7_decimate                    // unsigned ,    RW, default = 0
//Bit  7: 0        reg_vdi7_ctrl                        // unsigned ,    RW, default = 0
#define VDIN_INTF_VDI8_CTRL                        0x0129
//Bit 31:18        reserved
//Bit 17: 8        reg_vdi8_decimate                    // unsigned ,    RW, default = 0
//Bit  7: 0        reg_vdi8_ctrl                        // unsigned ,    RW, default = 0
#define VDIN_INTF_VDI9_CTRL                        0x012a
//Bit 31:18        reserved
//Bit 17: 8        reg_vdi9_decimate                    // unsigned ,    RW, default = 0
//Bit  7: 0        reg_vdi9_ctrl                        // unsigned ,    RW, default = 0
#define VDIN_INTF_VDI4A_TUNNEL_CTRL                0x012b
//Bit 31: 0        reg_vdi4a_tunnel_ctrl                // unsigned ,    RW, default = 0
#define VDIN_INTF_VDI4B_TUNNEL_CTRL                0x012c
//Bit 31: 0        reg_vdi4b_tunnel_ctrl                // unsigned ,    RW, default = 0
#define VDIN_INTF_VDI0_SIZE                        0x012d
//Bit 31:29        reserved
//Bit 28:16        reg_vdi0_hsize                       // unsigned ,    RW, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_vdi0_vsize                       // unsigned ,    RW, default = 0
#define VDIN_INTF_VDI1_SIZE                        0x012e
//Bit 31:29        reserved
//Bit 28:16        reg_vdi1_hsize                       // unsigned ,    RW, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_vdi1_vsize                       // unsigned ,    RW, default = 0
#define VDIN_INTF_VDI2_SIZE                        0x012f
//Bit 31:29        reserved
//Bit 28:16        reg_vdi2_hsize                       // unsigned ,    RW, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_vdi2_vsize                       // unsigned ,    RW, default = 0
#define VDIN_INTF_VDI3_SIZE                        0x0130
//Bit 31:29        reserved
//Bit 28:16        reg_vdi3_hsize                       // unsigned ,    RW, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_vdi3_vsize                       // unsigned ,    RW, default = 0
#define VDIN_INTF_VDI4A_SIZE                       0x0131
//Bit 31:29        reserved
//Bit 28:16        reg_vdi4a_hsize                      // unsigned ,    RW, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_vdi4a_vsize                      // unsigned ,    RW, default = 0
#define VDIN_INTF_VDI4B_SIZE                       0x0132
//Bit 31:29        reserved
//Bit 28:16        reg_vdi4b_hsize                      // unsigned ,    RW, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_vdi4b_vsize                      // unsigned ,    RW, default = 0
#define VDIN_INTF_VDI5_SIZE                        0x0133
//Bit 31:29        reserved
//Bit 28:16        reg_vdi5_hsize                       // unsigned ,    RW, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_vdi5_vsize                       // unsigned ,    RW, default = 0
#define VDIN_INTF_VDI6_SIZE                        0x0134
//Bit 31:29        reserved
//Bit 28:16        reg_vdi6_hsize                       // unsigned ,    RW, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_vdi6_vsize                       // unsigned ,    RW, default = 0
#define VDIN_INTF_VDI7_SIZE                        0x0135
//Bit 31:29        reserved
//Bit 28:16        reg_vdi7_hsize                       // unsigned ,    RW, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_vdi7_vsize                       // unsigned ,    RW, default = 0
#define VDIN_INTF_VDI8_SIZE                        0x0136
//Bit 31:29        reserved
//Bit 28:16        reg_vdi8_hsize                       // unsigned ,    RW, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_vdi8_vsize                       // unsigned ,    RW, default = 0
#define VDIN_INTF_VDI9_SIZE                        0x0137
//Bit 31:29        reserved
//Bit 28:16        reg_vdi9_hsize                       // unsigned ,    RW, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_vdi9_vsize                       // unsigned ,    RW, default = 0
#define VDIN_INTF_PTGEN_CTRL                       0x0138
//Bit 31:24        reserved
//Bit 23: 0        reg_ptgen_ctrl                       // unsigned ,    RW, default = 0
#define VDIN_INTF_MEAS_CTRL                        0x0139
//Bit 31:30        reserved
//Bit 29           reg_meas_rst                         // unsigned ,    RW, default = 0
//Bit 28           reg_meas_scan                        // unsigned ,    RW, default = 0
//Bit 27           reg_meas_vdi0_hsvs_wide_en           // unsigned ,    RW, default = 0
//Bit 26           reg_meas_vdi1_hsvs_wide_en           // unsigned ,    RW, default = 0
//Bit 25           reg_meas_vdi2_hsvs_wide_en           // unsigned ,    RW, default = 0
//Bit 24           reg_meas_vdi3_hsvs_wide_en           // unsigned ,    RW, default = 0
//Bit 23           reg_meas_vdi4a_hsvs_wide_en          // unsigned ,    RW, default = 0
//Bit 22           reg_meas_vdi4b_hsvs_wide_en          // unsigned ,    RW, default = 0
//Bit 21           reg_meas_vdi5_hsvs_wide_en           // unsigned ,    RW, default = 0
//Bit 20           reg_meas_vdi6_hsvs_wide_en           // unsigned ,    RW, default = 0
//Bit 19           reg_meas_vdi7_hsvs_wide_en           // unsigned ,    RW, default = 0
//Bit 18           reg_meas_vdi8_hsvs_wide_en           // unsigned ,    RW, default = 0
//Bit 17           reg_meas_vdi9_hsvs_wide_en           // unsigned ,    RW, default = 0
//Bit 16           reg_meas_total_count_accum_en        // unsigned ,    RW, default = 0
//Bit 15:12        reg_meas_sel                         // unsigned ,    RW, default = 0
//Bit 11: 4        reg_meas_sync_span                   // unsigned ,    RW, default = 0
//Bit  3: 0        reserved
#define VDIN_INTF_MEAS_FIRST_RANGE                 0x013a
//Bit 31:29        reserved
//Bit 28:16        reg_meas_first_count_end             // unsigned ,    RW, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_meas_first_count_start           // unsigned ,    RW, default = 0
#define VDIN_INTF_MEAS_SECOND_RANGE                 0x013b
//Bit 31:29        reserved
//Bit 28:16        reg_meas_second_count_end            // unsigned ,    RW, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_meas_second_count_start          // unsigned ,    RW, default = 0
#define VDIN_INTF_MEAS_THIRD_RANGE                 0x013c
//Bit 31:29        reserved
//Bit 28:16        reg_meas_third_count_end             // unsigned ,    RW, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_meas_third_count_start           // unsigned ,    RW, default = 0
#define VDIN_INTF_MEAS_FOURTH_RANGE                 0x013d
//Bit 31:29        reserved
//Bit 28:16        reg_meas_fourth_count_end            // unsigned ,    RW, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_meas_fourth_count_start          // unsigned ,    RW, default = 0
#define VDIN_INTF_MEAS_IND_TOTAL_COUNT_N           0x013e
//Bit 31: 4        reserved
//Bit  3: 0        reg_meas_ind_total_count_n           // unsigned ,    RO, default = 0
#define VDIN_INTF_MEAS_IND_TOTAL_COUNT0            0x013f
//Bit 31:24        reserved
//Bit 23: 0        reg_meas_ind_total_count0            // unsigned ,    RO, default = 0
#define VDIN_INTF_MEAS_IND_TOTAL_COUNT1            0x0140
//Bit 31:24        reserved
//Bit 23: 0        reg_meas_ind_total_count1            // unsigned ,    RO, default = 0
#define VDIN_INTF_MEAS_IND_FIRST_COUNT             0x0141
//Bit 31:24        reserved
//Bit 23: 0        reg_meas_ind_first_count             // unsigned ,    RO, default = 0
#define VDIN_INTF_MEAS_IND_SECOND_COUNT             0x0142
//Bit 31:24        reserved
//Bit 23: 0        reg_meas_ind_second_count            // unsigned ,    RO, default = 0
#define VDIN_INTF_MEAS_IND_THIRD_COUNT             0x0143
//Bit 31:24        reserved
//Bit 23: 0        reg_meas_ind_third_count             // unsigned ,    RO, default = 0
#define VDIN_INTF_MEAS_IND_FOURTH_COUNT             0x0144
//Bit 31:24        reserved
//Bit 23: 0        reg_meas_ind_fourth_count            // unsigned ,    RO, default = 0
#define VDIN_INTF_VDI_INT_STATUS0                  0x0145
//Bit 31           reg_vdi0_overflow                    // unsigned ,    RO, default = 0
//Bit 30           reserved
//Bit 29:24        reg_vdi0_asfifo_cnt                  // unsigned ,    RO, default = 0
//Bit 23           reg_vdi1_overflow                    // unsigned ,    RO, default = 0
//Bit 22           reserved
//Bit 21:16        reg_vdi1_asfifo_cnt                  // unsigned ,    RO, default = 0
//Bit 15           reg_vdi2_overflow                    // unsigned ,    RO, default = 0
//Bit 14           reserved
//Bit 13: 8        reg_vdi2_asfifo_cnt                  // unsigned ,    RO, default = 0
//Bit  7           reg_vdi3_overflow                    // unsigned ,    RO, default = 0
//Bit  6           reserved
//Bit  5: 0        reg_vdi3_asfifo_cnt                  // unsigned ,    RO, default = 0
#define VDIN_INTF_VDI_INT_STATUS1                  0x0146
//Bit 31           reg_vdi4a_overflow                   // unsigned ,    RO, default = 0
//Bit 30           reserved
//Bit 29:24        reg_vdi4a_asfifo_cnt                 // unsigned ,    RO, default = 0
//Bit 23           reg_vdi4b_overflow                   // unsigned ,    RO, default = 0
//Bit 22           reserved
//Bit 21:16        reg_vdi4b_asfifo_cnt                 // unsigned ,    RO, default = 0
//Bit 15           reg_vdi5_overflow                    // unsigned ,    RO, default = 0
//Bit 14           reserved
//Bit 13: 8        reg_vdi5_asfifo_cnt                  // unsigned ,    RO, default = 0
//Bit  7           reg_vdi6_overflow                    // unsigned ,    RO, default = 0
//Bit  6           reserved
//Bit  5: 0        reg_vdi6_asfifo_cnt                  // unsigned ,    RO, default = 0
#define VDIN_INTF_VDI_INT_STATUS2                  0x0147
//Bit 31:24        reserved
//Bit 23           reg_vdi7_overflow                    // unsigned ,    RO, default = 0
//Bit 22           reserved
//Bit 21:16        reg_vdi7_asfifo_cnt                  // unsigned ,    RO, default = 0
//Bit 15           reg_vdi8_overflow                    // unsigned ,    RO, default = 0
//Bit 14           reserved
//Bit 13: 8        reg_vdi8_asfifo_cnt                  // unsigned ,    RO, default = 0
//Bit  7           reg_vdi9_overflow                    // unsigned ,    RO, default = 0
//Bit  6           reserved
//Bit  5: 0        reg_vdi9_asfifo_cnt                  // unsigned ,    RO, default = 0
#define VDIN0_SYNC_CONVERT_SECURE_CTRL             0x0150
//Bit 31: 6        reserved
//Bit  5           reg_secure_latch_mask                // unsigned ,    RW, default = 0
//Bit  4           reg_secure_out                       // unsigned ,    RW, default = 0
//Bit  3: 0        reg_secure_sel                       // unsigned ,    RW, default = 0
#define VDIN0_SYNC_CONVERT_CTRL                    0x0151
//Bit 31: 1        reserved
//Bit  0           reg_enable                           // unsigned ,    RW, default = 0
#define VDIN0_SYNC_CONVERT_SYNC_CTRL0              0x0152
//Bit 31           reg_force_go_field                   // unsigned ,    RW, default = 0
//Bit 30           reg_force_go_line                    // unsigned ,    RW, default = 0
//Bit 29:24        reserved
//Bit 23           reg_dly_go_field_en                  // unsigned ,    RW, default = 0
//Bit 22:16        reg_dly_go_field_lines               // unsigned ,    RW, default = 0
//Bit 15:14        reserved
//Bit 13           reg_clk_cyc_cnt_clr                  // unsigned ,    RW, default = 0
//Bit 12           reg_clk_cyc_go_line_en               // unsigned ,    RW, default = 0
//Bit 11           reserved
//Bit 10: 4        reg_hold_lines                       // unsigned ,    RW, default = 0
//Bit  3           reserved
//Bit  2           reg_hsync_mask_en                    // unsigned ,    RW, default = 0
//Bit  1           reg_vsync_mask_en                    // unsigned ,    RW, default = 0
//Bit  0           reg_frm_rst_en                       // unsigned ,    RW, default = 1
#define VDIN0_SYNC_CONVERT_SYNC_CTRL1              0x0153
//Bit 31:16        reg_clk_cyc_line_widthm1             // unsigned ,    RW, default = 0
//Bit 15: 8        reg_hsync_mask_num                   // unsigned ,    RW, default = 8'h40
//Bit  7: 0        reg_vsync_mask_num                   // unsigned ,    RW, default = 8'h40
#define VDIN0_SYNC_CONVERT_LINE_INT_CTRL           0x0154
//Bit 31:14        reserved
//Bit 13           reg_line_cnt_clr                     // unsigned ,    RW, default = 0
//Bit 12: 0        reg_line_cnt_thd                     // unsigned ,    RW, default = 0
#define VDIN0_SYNC_CONVERT_STATUS                  0x0155
//Bit 31: 5        reserved
//Bit  4           reg_secure_cur_pic0                  // unsigned ,    RO, default = 0
//Bit  3           reg_secure_cur_pic0_sav              // unsigned ,    RO, default = 0
//Bit  2           reg_secure_cur_pic1                  // unsigned ,    RO, default = 0
//Bit  1           reg_secure_cur_pic1_sav              // unsigned ,    RO, default = 0
//Bit  0           reg_field_in                         // unsigned ,    RO, default = 0
#define VDIN0_SYNC_CONVERT_LINE_CNT_STATUS         0x0156
//Bit 31:29        reserved
//Bit 28:16        reg_go_line_cnt                      // unsigned ,    RO, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_active_line_cnt                  // unsigned ,    RO, default = 0
#define VDIN0_SYNC_CONVERT_LINE_CNT_SHADOW_STATUS  0x0157
//Bit 31:29        reserved
//Bit 28:16        reg_go_line_cnt_shadow               // unsigned ,    RO, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_active_line_cnt_shadow           // unsigned ,    RO, default = 0
#define VDIN0_SYNC_CONVERT_ACTIVE_MAX_PIX_CNT_STATUS 0x0158
//Bit 31:29        reserved
//Bit 28:16        reg_active_max_pix_cnt               // unsigned ,    RO, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_active_max_pix_cnt_shadow        // unsigned ,    RO, default = 0
#define VDIN0_SYNC_CONVERT_SECURE_ILLEGAL          0x0159
//Bit 31: 0        reg_secure_illegal                   // unsigned ,    RO, default = 0
#define VDIN1_SYNC_CONVERT_SECURE_CTRL             0x0160
//Bit 31: 6        reserved
//Bit  5           reg_secure_latch_mask                // unsigned ,    RW, default = 0
//Bit  4           reg_secure_out                       // unsigned ,    RW, default = 0
//Bit  3: 0        reg_secure_sel                       // unsigned ,    RW, default = 0
#define VDIN1_SYNC_CONVERT_CTRL                    0x0161
//Bit 31: 1        reserved
//Bit  0           reg_enable                           // unsigned ,    RW, default = 0
#define VDIN1_SYNC_CONVERT_SYNC_CTRL0              0x0162
//Bit 31           reg_force_go_field                   // unsigned ,    RW, default = 0
//Bit 30           reg_force_go_line                    // unsigned ,    RW, default = 0
//Bit 29:24        reserved
//Bit 23           reg_dly_go_field_en                  // unsigned ,    RW, default = 0
//Bit 22:16        reg_dly_go_field_lines               // unsigned ,    RW, default = 0
//Bit 15:14        reserved
//Bit 13           reg_clk_cyc_cnt_clr                  // unsigned ,    RW, default = 0
//Bit 12           reg_clk_cyc_go_line_en               // unsigned ,    RW, default = 0
//Bit 11           reserved
//Bit 10: 4        reg_hold_lines                       // unsigned ,    RW, default = 0
//Bit  3           reserved
//Bit  2           reg_hsync_mask_en                    // unsigned ,    RW, default = 0
//Bit  1           reg_vsync_mask_en                    // unsigned ,    RW, default = 0
//Bit  0           reg_frm_rst_en                       // unsigned ,    RW, default = 1
#define VDIN1_SYNC_CONVERT_SYNC_CTRL1              0x0163
//Bit 31:16        reg_clk_cyc_line_widthm1             // unsigned ,    RW, default = 0
//Bit 15: 8        reg_hsync_mask_num                   // unsigned ,    RW, default = 8'h40
//Bit  7: 0        reg_vsync_mask_num                   // unsigned ,    RW, default = 8'h40
#define VDIN1_SYNC_CONVERT_LINE_INT_CTRL           0x0164
//Bit 31:14        reserved
//Bit 13           reg_line_cnt_clr                     // unsigned ,    RW, default = 0
//Bit 12: 0        reg_line_cnt_thd                     // unsigned ,    RW, default = 0
#define VDIN1_SYNC_CONVERT_STATUS                  0x0165
//Bit 31: 5        reserved
//Bit  4           reg_secure_cur_pic0                  // unsigned ,    RO, default = 0
//Bit  3           reg_secure_cur_pic0_sav              // unsigned ,    RO, default = 0
//Bit  2           reg_secure_cur_pic1                  // unsigned ,    RO, default = 0
//Bit  1           reg_secure_cur_pic1_sav              // unsigned ,    RO, default = 0
//Bit  0           reg_field_in                         // unsigned ,    RO, default = 0
#define VDIN1_SYNC_CONVERT_LINE_CNT_STATUS         0x0166
//Bit 31:29        reserved
//Bit 28:16        reg_go_line_cnt                      // unsigned ,    RO, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_active_line_cnt                  // unsigned ,    RO, default = 0
#define VDIN1_SYNC_CONVERT_LINE_CNT_SHADOW_STATUS  0x0167
//Bit 31:29        reserved
//Bit 28:16        reg_go_line_cnt_shadow               // unsigned ,    RO, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_active_line_cnt_shadow           // unsigned ,    RO, default = 0
#define VDIN1_SYNC_CONVERT_ACTIVE_MAX_PIX_CNT_STATUS 0x0168
//Bit 31:29        reserved
//Bit 28:16        reg_active_max_pix_cnt               // unsigned ,    RO, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_active_max_pix_cnt_shadow        // unsigned ,    RO, default = 0
#define VDIN1_SYNC_CONVERT_SECURE_ILLEGAL          0x0169
//Bit 31: 0        reg_secure_illegal                   // unsigned ,    RO, default = 0
#define VDIN2_SYNC_CONVERT_SECURE_CTRL             0x0170
//Bit 31: 6        reserved
//Bit  5           reg_secure_latch_mask                // unsigned ,    RW, default = 0
//Bit  4           reg_secure_out                       // unsigned ,    RW, default = 0
//Bit  3: 0        reg_secure_sel                       // unsigned ,    RW, default = 0
#define VDIN2_SYNC_CONVERT_CTRL                    0x0171
//Bit 31: 1        reserved
//Bit  0           reg_enable                           // unsigned ,    RW, default = 0
#define VDIN2_SYNC_CONVERT_SYNC_CTRL0              0x0172
//Bit 31           reg_force_go_field                   // unsigned ,    RW, default = 0
//Bit 30           reg_force_go_line                    // unsigned ,    RW, default = 0
//Bit 29:24        reserved
//Bit 23           reg_dly_go_field_en                  // unsigned ,    RW, default = 0
//Bit 22:16        reg_dly_go_field_lines               // unsigned ,    RW, default = 0
//Bit 15:14        reserved
//Bit 13           reg_clk_cyc_cnt_clr                  // unsigned ,    RW, default = 0
//Bit 12           reg_clk_cyc_go_line_en               // unsigned ,    RW, default = 0
//Bit 11           reserved
//Bit 10: 4        reg_hold_lines                       // unsigned ,    RW, default = 0
//Bit  3           reserved
//Bit  2           reg_hsync_mask_en                    // unsigned ,    RW, default = 0
//Bit  1           reg_vsync_mask_en                    // unsigned ,    RW, default = 0
//Bit  0           reg_frm_rst_en                       // unsigned ,    RW, default = 1
#define VDIN2_SYNC_CONVERT_SYNC_CTRL1              0x0173
//Bit 31:16        reg_clk_cyc_line_widthm1             // unsigned ,    RW, default = 0
//Bit 15: 8        reg_hsync_mask_num                   // unsigned ,    RW, default = 8'h40
//Bit  7: 0        reg_vsync_mask_num                   // unsigned ,    RW, default = 8'h40
#define VDIN2_SYNC_CONVERT_LINE_INT_CTRL           0x0174
//Bit 31:14        reserved
//Bit 13           reg_line_cnt_clr                     // unsigned ,    RW, default = 0
//Bit 12: 0        reg_line_cnt_thd                     // unsigned ,    RW, default = 0
#define VDIN2_SYNC_CONVERT_STATUS                  0x0175
//Bit 31: 5        reserved
//Bit  4           reg_secure_cur_pic0                  // unsigned ,    RO, default = 0
//Bit  3           reg_secure_cur_pic0_sav              // unsigned ,    RO, default = 0
//Bit  2           reg_secure_cur_pic1                  // unsigned ,    RO, default = 0
//Bit  1           reg_secure_cur_pic1_sav              // unsigned ,    RO, default = 0
//Bit  0           reg_field_in                         // unsigned ,    RO, default = 0
#define VDIN2_SYNC_CONVERT_LINE_CNT_STATUS         0x0176
//Bit 31:29        reserved
//Bit 28:16        reg_go_line_cnt                      // unsigned ,    RO, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_active_line_cnt                  // unsigned ,    RO, default = 0
#define VDIN2_SYNC_CONVERT_LINE_CNT_SHADOW_STATUS  0x0177
//Bit 31:29        reserved
//Bit 28:16        reg_go_line_cnt_shadow               // unsigned ,    RO, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_active_line_cnt_shadow           // unsigned ,    RO, default = 0
#define VDIN2_SYNC_CONVERT_ACTIVE_MAX_PIX_CNT_STATUS 0x0178
//Bit 31:29        reserved
//Bit 28:16        reg_active_max_pix_cnt               // unsigned ,    RO, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_active_max_pix_cnt_shadow        // unsigned ,    RO, default = 0
#define VDIN2_SYNC_CONVERT_SECURE_ILLEGAL          0x0179
//Bit 31: 0        reg_secure_illegal                   // unsigned ,    RO, default = 0
#define VDIN_RARB_MODE                             0x0180
//Bit 31:24        reserved
//Bit 23:16        reg_arb_sel                  // unsigned ,    RW, default = 0
//Bit 15:10        reserved
//Bit  9: 8        reg_arb_arb_mode             // unsigned ,    RW, default = 2'd3
//Bit  7: 4        reserved
//Bit  3: 0        reg_arb_gclk_ctrl            // unsigned ,    RW, default = 0
#define VDIN_RARB_REQEN_SLV                        0x0181
//Bit 31:16        reserved
//Bit 15: 0        reg_arb_dc_req_en            // unsigned ,    RW, default = 16'hffff
#define VDIN_RARB_WEIGH0_SLV                       0x0182
//Bit 31:24        reserved
//Bit 23: 0        reg_arb_dc_weigh_sxn[23:0]   // unsigned ,    RW, default = 24'h3cf3cf
#define VDIN_RARB_WEIGH1_SLV                       0x0183
//Bit 31:24        reserved
//Bit 23: 0        reg_arb_dc_weigh_sxn[47:24]  // unsigned ,    RW, default = 24'h3cf3cf
#define VDIN_RARB_UGT                              0x0184
//Bit 31:16        reserved
//Bit 15: 0        reg_arb_ugt_basic            // unsigned ,    RW, default = 16'h5555
#define VDIN_RARB_LIMT0                            0x0185
//Bit 31: 0        reg_arb_req_limt_num         // unsigned ,    RW, default = 32'h3f3f3f3f
#define VDIN_RARB_STATUS                           0x0186
//Bit 31: 2        reg_arb_det_dbg_stat         // unsigned ,    RO, default = 0
//Bit  1: 0        reg_arb_arb_busy             // unsigned ,    RO, default = 0
#define VDIN_RARB_DBG_CTRL                         0x0187
//Bit 31: 0        reg_arb_det_cmd_ctrl         // unsigned ,    RW, default = 8
#define VDIN_RARB_PROT                             0x0188
//Bit 31:22        reserved
//Bit 21: 0        reg_arb_axi_prot_ctrl        // unsigned ,    RW, default = 0
#define VDIN_RARB_PROT_STAT                        0x0189
//Bit 31:20        reserved
//Bit 19: 0        reg_arb_axi_prot_stat        // unsigned ,    RO, default = 0
#define VDIN_WARB_MODE                             0x0190
//Bit 31:24        reserved
//Bit 23:16        reg_arb_sel                  // unsigned ,    RW, default = 8'd248
//Bit 15:10        reserved
//Bit  9: 8        reg_arb_arb_mode             // unsigned ,    RW, default = 2'd3
//Bit  7: 4        reserved
//Bit  3: 0        reg_arb_gclk_ctrl            // unsigned ,    RW, default = 0
#define VDIN_WARB_REQEN_SLV                        0x0191
//Bit 31:16        reserved
//Bit 15: 0        reg_arb_dc_req_en            // unsigned ,    RW, default = 16'hffff
#define VDIN_WARB_WEIGH0_SLV                       0x0192
//Bit 31:24        reserved
//Bit 23: 0        reg_arb_dc_weigh_sxn[23:0]   // unsigned ,    RW, default = 24'h3cf3cf
#define VDIN_WARB_WEIGH1_SLV                       0x0193
//Bit 31:24        reserved
//Bit 23: 0        reg_arb_dc_weigh_sxn[47:24]  // unsigned ,    RW, default = 24'h3cf3cf
#define VDIN_WARB_UGT                              0x0194
//Bit 31:16        reserved
//Bit 15: 0        reg_arb_ugt_basic            // unsigned ,    RW, default = 16'h5555
#define VDIN_WARB_LIMT0                            0x0195
//Bit 31: 0        reg_arb_req_limt_num         // unsigned ,    RW, default = 32'h3f3f3f3f
#define VDIN_WARB_STATUS                           0x0196
//Bit 31: 2        reg_arb_det_dbg_stat         // unsigned ,    RO, default = 0
//Bit  1: 0        reg_arb_arb_busy             // unsigned ,    RO, default = 0
#define VDIN_WARB_DBG_CTRL                         0x0197
//Bit 31: 0        reg_arb_det_cmd_ctrl         // unsigned ,    RW, default = 8
#define VDIN_WARB_PROT                             0x0198
//Bit 31:22        reserved
//Bit 21: 0        reg_arb_axi_prot_ctrl        // unsigned ,    RW, default = 0
#define VDIN_WARB_PROT_STAT                        0x0199
//Bit 31:20        reserved
//Bit 19: 0        reg_arb_axi_prot_stat        // unsigned ,    RO, default = 0

/* t3x new added registers misc end */

/* t3x new added registers bank 0 begin */
//
// Closing file:  ./vdin_misc_top_reg.h
//
//===========================================================================
// -----------------------------------------------
// CBUS_BASE:  VDIN0_CORE_TOP_P0_VCBUS_BASE = 0x02
// -----------------------------------------------
//===========================================================================
//
// Reading file:  ./vdin0_core_top_p0_reg.h
//
#define VDIN0_CORE_CTRL                            0x0200
//Bit 31:30        reg_top_gclk_ctrl            // unsigned ,    RW, default = 0
//Bit 29:28        reg_pp_gclk_ctrl             // unsigned ,    RW, default = 0
//Bit 27:26        reg_dw_gclk_ctrl             // unsigned ,    RW, default = 0
//Bit 25:24        reg_dith_gclk_ctrl           // unsigned ,    RW, default = 0
//Bit 23:22        reg_afbce_gclk_ctrl          // unsigned ,    RW, default = 0
//Bit 21:20        reg_meta_gclk_ctrl           // unsigned ,    RW, default = 0
//Bit 19           reserved
//Bit 18           reg_dw_frm_end_clr           // unsigned ,    RW, default = 0
//Bit 17           reg_dout_frm_end_clr         // unsigned ,    RW, default = 0
//Bit 16           reg_afbce_frm_end_clr        // unsigned ,    RW, default = 0
//Bit 15           reg_meta_frm_end_clr         // unsigned ,    RW, default = 0
//Bit 14:10        reserved
//Bit  9           reg_di_path_en                // unsigned ,   RW, default = 0
//Bit  8           reg_afbce_path_en             // unsigned ,   RW, default = 0
//Bit  7           reg_dith_path_en              // unsigned ,   RW, default = 0
//Bit  6           reg_dw_path_en                // unsigned ,   RW, default = 0
//Bit  5           reg_pp_path_en                // unsigned ,   RW, default = 0
//Bit  4           reg_meta_path_en              // unsigned ,   RW, default = 0
//Bit  3           reg_meta_en                  // unsigned ,    RW, default = 0
//Bit  2           reg_dith_en                  // unsigned ,    RW, default = 0
//Bit  1           reg_dolby_en                 // unsigned ,    RW, default = 0
//Bit  0           reg_detunnel_en              // unsigned ,    RW, default = 0
#define VDIN0_CORE_DETUNNEL                        0x0201
//Bit 31:19        reserved
//Bit 18           reg_detunnel_u_start         // unsigned ,    RW, default = 0
//Bit 17: 0        reg_detunnel_sel             // unsigned ,    RW, default = 0
#define VDIN0_CORE_SIZE                            0x0202
//Bit 31:29        reserved
//Bit 28:16        reg_hsize                    // unsigned ,    RW, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_vsize                    // unsigned ,    RW, default = 0
#define VDIN0_CORE_META_CRC                        0x0203
//Bit 31:10        reserved
//Bit  9           reg_meta_crc_pass            // unsigned ,    RO, default = 0
//Bit  8:1         reg_meta_crc_count           // unsigned ,    RO, default = 0
//Bit  0           reg_meta_crc_interrupt       // unsigned ,    RO, default = 0
#define VDIN0_CORE_FRM_END                         0x0204
//Bit 31: 4        reserved
//Bit  3           reg_dw_frm_end               // unsigned ,    RO, default = 0
//Bit  2           reg_dout_frm_end             // unsigned ,    RO, default = 0
//Bit  1           reg_afbce_frm_end            // unsigned ,    RO, default = 0
//Bit  0           reg_meta_frm_end             // unsigned ,    RO, default = 0
#define VDIN0_DITH0_CTRL                           0x0210
//Bit 31:30     reg_dith_gclk_ctrl  // unsigned ,    RW, default = 2'h0
//Bit 29:20     reserved
//Bit 19:16     reg_dith22_cntl     // unsigned ,    RW, default = 4'h0
//Bit 15:14     reserved
//Bit 13:4      reg_dith44_cntl     // unsigned ,    RW, default = 10'h3b4
//Bit 3         reserved
//Bit 2         reg_dith_md         // unsigned ,    RW, default = 1'h0
//Bit 1         reg_round_en        // unsigned ,    RW, default = 1'h0
//Bit 0         reserved
#define VDIN0_AFBCE_FORMAT                         0x0220
//Bit 31:15       reserved
//Bit 14:12       reg_burst_length_add_value      // unsigned ,   RW, default = 2
//Bit 11          reg_ofset_burst4_en             // unsigned ,   RW, default = 0
//Bit 10          reg_burst_length_add_en         // unsigned ,   RW, default = 0
//Bit  9: 8       reg_format_mode                 // unsigned ,   RW, default = 2
//data format 0 : YUV444  1:YUV422  2:YUV420  3:RGB
//Bit  7: 4       reg_compbits_c                  // unsigned ,   RW, default = 10 chroma bitwidth
//Bit  3: 0       reg_compbits_y                  // unsigned ,   RW, default = 10 luma bitwidth
#define VDIN0_AFBCE_MODE_EN                        0x0221
//Bit 31:28       reserved
//Bit 27:26       reserved
//Bit 25          reg_adpt_interleave_ymode       // unsigned ,   RW, default = 0
//force 0 to disable it: no  HW implementation
//Bit 24          reg_adpt_interleave_cmode       // unsigned ,   RW, default = 0
//force 0 to disable it: not HW implementation
//Bit 23          reg_adpt_yinterleave_luma_ride  // unsigned ,   RW, default = 1
//vertical interleave piece luma reorder ride    0: no reorder ride  1: w/4 as ride
//Bit 22          reg_adpt_yinterleave_chrm_ride  // unsigned ,   RW, default = 1
//vertical interleave piece chroma reorder ride  0: no reorder ride  1: w/2 as ride
//Bit 21          reg_adpt_xinterleave_luma_ride  // unsigned ,   RW, default = 1
//vertical interleave piece luma reorder ride    0: no reorder ride  1: w/4 as ride
//Bit 20          reg_adpt_xinterleave_chrm_ride  // unsigned ,   RW, default = 1
//vertical interleave piece chroma reorder ride  0: no reorder ride  1: w/2 as ride
//Bit 19          reserved
//Bit 18          reg_disable_order_mode_i_6      // unsigned ,   RW, default = 0
//disable order mode0~6: each mode with one  disable bit: 0: no disable  1: disable
//Bit 17          reg_disable_order_mode_i_5      // unsigned ,   RW, default = 0
//disable order mode0~6: each mode with one  disable bit: 0: no disable  1: disable
//Bit 16          reg_disable_order_mode_i_4      // unsigned ,   RW, default = 0
//disable order mode0~6: each mode with one  disable bit: 0: no disable  1: disable
//Bit 15          reg_disable_order_mode_i_3      // unsigned ,   RW, default = 0
//disable order mode0~6: each mode with one  disable bit: 0: no disable  1: disable
//Bit 14          reg_disable_order_mode_i_2      // unsigned ,   RW, default = 0
//disable order mode0~6: each mode with one  disable bit: 0: no disable  1: disable
//Bit 13          reg_disable_order_mode_i_1      // unsigned ,   RW, default = 0
//disable order mode0~6: each mode with one  disable bit: 0: no disable  1: disable
//Bit 12          reg_disable_order_mode_i_0      // unsigned ,   RW, default = 0
//disable order mode0~6: each mode with one  disable bit: 0: no disable  1: disable
//Bit 11          reserved
//Bit 10          reg_minval_yenc_en              // unsigned ,   RW, default = 0
//force disable  final decision to remove this ws 1% performance loss
//Bit  9          reg_16x4block_enable            // unsigned ,   RW, default = 0
//block as mission  but permit 16x4 block
//Bit  8          reg_uncompress_split_mode       // unsigned ,   RW, default = 0
//0: no split  1: split
//Bit  7: 6       reserved
//Bit  5          reg_input_padding_uv128         // unsigned ,   RW, default = 0
//input picture 32x4 block gap mode: 0:  pad uv=0  1: pad uv=128
//Bit  4          reg_dwds_padding_uv128          // unsigned ,   RW, default = 0
//downsampled image for double write 32x gap mode: 0:  pad uv=0  1: pad uv=128
//Bit  3: 1       reg_force_order_mode_value      // unsigned ,   RW, default = 0
//force order mode 0~7
//Bit  0          reg_force_order_mode_en         // unsigned ,   RW, default = 0
//force order mode enable: 0: no force  1: forced to force_value
#define VDIN0_AFBCE_DWSCALAR                       0x0222
//Bit 31: 8       reserved
//Bit  7: 6       reg_dwscalar_w0                 // unsigned ,   RW, default = 3
//horizontal 1st step scalar mode: 0: 1:1 no scalar  1: 2:1 data drop (0 2 4  6) pixel kept
//2: 2:1 data drop (1  3  5 7..) pixels kept  3: avg
//Bit  5: 4       reg_dwscalar_w1                 // unsigned ,   RW, default = 0
//horizontal 2nd step scalar mode: 0: 1:1 no scalar  1: 2:1 data drop (0 2 4  6) pixel kept
//2: 2:1 data drop (1  3  5 7..) pixels kept  3: avg
//Bit  3: 2       reg_dwscalar_h0                 // unsigned ,   RW, default = 2
//vertical 1st step scalar mode: 0: 1:1 no scalar  1: 2:1 data drop (0 2 4  6) pixel kept
//2: 2:1 data drop (1  3  5 7..) pixels kept  3: avg
//Bit  1: 0       reg_dwscalar_h1                 // unsigned ,   RW, default = 3
//vertical 2nd step scalar mode: 0: 1:1 no scalar  1: 2:1 data drop (0 2 4  6) pixel kept
//2: 2:1 data drop (1  3  5 7..) pixels kept  3: avg
#define VDIN0_AFBCE_DEFCOLOR_1                     0x0223
//Bit 31:24       reserved
//Bit 23:12       reg_enc_defaultcolor_3          // unsigned ,   RW, default = 4095
//Picture wise default color value in [Y Cb Cr]
//Bit 11: 0       reg_enc_defaultcolor_0          // unsigned ,   RW, default = 4095
//Picture wise default color value in [Y Cb Cr]
#define VDIN0_AFBCE_DEFCOLOR_2                     0x0224
//Bit 31:24       reserved
//Bit 23:12       reg_enc_defaultcolor_2          // unsigned ,   RW, default = 4095
//wise default color value in [Y Cb Cr]
//Bit 11: 0       reg_enc_defaultcolor_1          // unsigned ,   RW, default = 4095
//wise default color value in [Y Cb Cr]
#define VDIN0_AFBCE_QUANT_ENABLE                   0x0225
//Bit 31:12       reserved
//Bit 11          reg_quant_expand_en_1           // unsigned ,   RW, default = 0
//enable for quantization value expansion
//Bit 10          reg_quant_expand_en_0           // unsigned ,   RW, default = 0
//enable for quantization value expansion
//Bit  9: 8       reg_bcleav_ofst                 //   signed ,   RW, default = 0
//bcleave ofset to get lower range especially under lossy  for v1/v2 x=0 is equivalent default = -1
//Bit  7: 5       reserved
//Bit  4          reg_quant_enable_1              // unsigned ,   RW, default = 0
//enable for quant to get some lossy
//Bit  3: 1       reserved
//Bit  0          reg_quant_enable_0              // unsigned ,   RW, default = 0
//enable for quant to get some lossy
#define VDIN0_AFBCE_IQUANT_LUT_1                   0x0226
//Bit 31          reserved
//Bit 30:28       reg_iquant_yclut_0_11           // unsigned ,   RW, default = 0
//quantization lut for mintree leavs  iquant=2^lut(bc_leav_q+1)
//Bit 27          reserved
//Bit 26:24       reg_iquant_yclut_0_10           // unsigned ,   RW, default = 1
//quantization lut for mintree leavs  iquant=2^lut(bc_leav_q+1)
//Bit 23          reserved
//Bit 22:20       reg_iquant_yclut_0_9            // unsigned ,   RW, default = 2
//quantization lut for mintree leavs  iquant=2^lut(bc_leav_q+1)
//Bit 19          reserved
//Bit 18:16       reg_iquant_yclut_0_8            // unsigned ,   RW, default = 3
//quantization lut for mintree leavs  iquant=2^lut(bc_leav_q+1)
//Bit 15          reserved
//Bit 14:12       reg_iquant_yclut_0_7            // unsigned ,   RW, default = 4
//quantization lut for mintree leavs  iquant=2^lut(bc_leav_q+1)
//Bit 11          reserved
//Bit 10: 8       reg_iquant_yclut_0_6            // unsigned ,   RW, default = 5
//quantization lut for mintree leavs  iquant=2^lut(bc_leav_q+1)
//Bit  7          reserved
//Bit  6: 4       reg_iquant_yclut_0_5            // unsigned ,   RW, default = 5
//quantization lut for mintree leavs  iquant=2^lut(bc_leav_q+1)
//Bit  3          reserved
//Bit  2: 0       reg_iquant_yclut_0_4            // unsigned ,   RW, default = 4
//quantization lut for mintree leavs  iquant=2^lut(bc_leav_q+1)
#define VDIN0_AFBCE_IQUANT_LUT_2                   0x0227
//Bit 31:16       reserved
//Bit 15          reserved
//Bit 14:12       reg_iquant_yclut_0_3            // unsigned ,   RW, default = 3
//quantization lut for mintree leavs  iquant=2^lut(bc_leav_q+1)
//Bit 11          reserved
//Bit 10: 8       reg_iquant_yclut_0_2            // unsigned ,   RW, default = 2
//quantization lut for mintree leavs  iquant=2^lut(bc_leav_q+1)
//Bit  7          reserved
//Bit  6: 4       reg_iquant_yclut_0_1            // unsigned ,   RW, default = 1
//quantization lut for mintree leavs  iquant=2^lut(bc_leav_q+1)
//Bit  3          reserved
//Bit  2: 0       reg_iquant_yclut_0_0            // unsigned ,   RW, default = 0
//quantization lut for mintree leavs  iquant=2^lut(bc_leav_q+1)
#define VDIN0_AFBCE_IQUANT_LUT_3                   0x0228
//Bit 31          reserved
//Bit 30:28       reg_iquant_yclut_1_11           // unsigned ,   RW, default = 0
//quantization lut for mintree leavs  iquant=2^lut(bc_leav_q+1)
//Bit 27          reserved
//Bit 26:24       reg_iquant_yclut_1_10           // unsigned ,   RW, default = 1
//quantization lut for mintree leavs  iquant=2^lut(bc_leav_q+1)
//Bit 23          reserved
//Bit 22:20       reg_iquant_yclut_1_9            // unsigned ,   RW, default = 2
//quantization lut for mintree leavs  iquant=2^lut(bc_leav_q+1)
//Bit 19          reserved
//Bit 18:16       reg_iquant_yclut_1_8            // unsigned ,   RW, default = 3
//quantization lut for mintree leavs  iquant=2^lut(bc_leav_q+1)
//Bit 15          reserved
//Bit 14:12       reg_iquant_yclut_1_7            // unsigned ,   RW, default = 4
//quantization lut for mintree leavs  iquant=2^lut(bc_leav_q+1)
//Bit 11          reserved
//Bit 10: 8       reg_iquant_yclut_1_6            // unsigned ,   RW, default = 5
//quantization lut for mintree leavs  iquant=2^lut(bc_leav_q+1)
//Bit  7          reserved
//Bit  6: 4       reg_iquant_yclut_1_5            // unsigned ,   RW, default = 5
//quantization lut for mintree leavs  iquant=2^lut(bc_leav_q+1)
//Bit  3          reserved
//Bit  2: 0       reg_iquant_yclut_1_4            // unsigned ,   RW, default = 4
//quantization lut for mintree leavs  iquant=2^lut(bc_leav_q+1)
#define VDIN0_AFBCE_IQUANT_LUT_4                   0x0229
//Bit 31:16       reserved
//Bit 15          reserved
//Bit 14:12       reg_iquant_yclut_1_3            // unsigned ,   RW, default = 3
//quantization lut for mintree leavs  iquant=2^lut(bc_leav_q+1)
//Bit 11          reserved
//Bit 10: 8       reg_iquant_yclut_1_2            // unsigned ,   RW, default = 2
//quantization lut for mintree leavs  iquant=2^lut(bc_leav_q+1)
//Bit  7          reserved
//Bit  6: 4       reg_iquant_yclut_1_1            // unsigned ,   RW, default = 1
//quantization lut for mintree leavs  iquant=2^lut(bc_leav_q+1)
//Bit  3          reserved
//Bit  2: 0       reg_iquant_yclut_1_0            // unsigned ,   RW, default = 0
//quantization lut for mintree leavs  iquant=2^lut(bc_leav_q+1)
#define VDIN0_AFBCE_RQUANT_LUT_1                   0x022a
//Bit 31          reserved
//Bit 30:28       reg_rquant_yclut_0_11           // unsigned ,   RW, default = 5
//quantization lut for bctree leavs  quant=2^lut(bc_leav_r+1)
//can be calculated from iquant_yclut(fw_setting)
//Bit 27          reserved
//Bit 26:24       reg_rquant_yclut_0_10           // unsigned ,   RW, default = 5
//Bit 23          reserved
//Bit 22:20       reg_rquant_yclut_0_9            // unsigned ,   RW, default = 4
//Bit 19          reserved
//Bit 18:16       reg_rquant_yclut_0_8            // unsigned ,   RW, default = 4
//Bit 15          reserved
//Bit 14:12       reg_rquant_yclut_0_7            // unsigned ,   RW, default = 3
//Bit 11          reserved
//Bit 10: 8       reg_rquant_yclut_0_6            // unsigned ,   RW, default = 3
//Bit  7          reserved
//Bit  6: 4       reg_rquant_yclut_0_5            // unsigned ,   RW, default = 2
//Bit  3          reserved
//Bit  2: 0       reg_rquant_yclut_0_4            // unsigned ,   RW, default = 2
#define VDIN0_AFBCE_RQUANT_LUT_2                   0x022b
//Bit 31:16       reserved
//Bit 15          reserved
//Bit 14:12       reg_rquant_yclut_0_3            // unsigned ,   RW, default = 1
//Bit 11          reserved
//Bit 10: 8       reg_rquant_yclut_0_2            // unsigned ,   RW, default = 1
//Bit  7          reserved
//Bit  6: 4       reg_rquant_yclut_0_1            // unsigned ,   RW, default = 0
//Bit  3          reserved
//Bit  2: 0       reg_rquant_yclut_0_0            // unsigned ,   RW, default = 0
#define VDIN0_AFBCE_RQUANT_LUT_3                   0x022c
//Bit 31          reserved
//Bit 30:28       reg_rquant_yclut_1_11           // unsigned ,   RW, default = 5
// quantization lut for bctree leavs  quant=2^lut(bc_leav_r+1)
// can be calculated from iquant_yclut(fw_setting)
//Bit 27          reserved
//Bit 26:24       reg_rquant_yclut_1_10           // unsigned ,   RW, default = 5
//Bit 23          reserved
//Bit 22:20       reg_rquant_yclut_1_9            // unsigned ,   RW, default = 4
//Bit 19          reserved
//Bit 18:16       reg_rquant_yclut_1_8            // unsigned ,   RW, default = 4
//Bit 15          reserved
//Bit 14:12       reg_rquant_yclut_1_7            // unsigned ,   RW, default = 3
//Bit 11          reserved
//Bit 10: 8       reg_rquant_yclut_1_6            // unsigned ,   RW, default = 3
//Bit  7          reserved
//Bit  6: 4       reg_rquant_yclut_1_5            // unsigned ,   RW, default = 2
//Bit  3          reserved
//Bit  2: 0       reg_rquant_yclut_1_4            // unsigned ,   RW, default = 2
#define VDIN0_AFBCE_RQUANT_LUT_4                   0x022d
//Bit 31:16       reserved
//Bit 15          reserved
//Bit 14:12       reg_rquant_yclut_1_3            // unsigned ,   RW, default = 1
//Bit 11          reserved
//Bit 10: 8       reg_rquant_yclut_1_2            // unsigned ,   RW, default = 1
//Bit  7          reserved
//Bit  6: 4       reg_rquant_yclut_1_1            // unsigned ,   RW, default = 0
//Bit  3          reserved
//Bit  2: 0       reg_rquant_yclut_1_0            // unsigned ,   RW, default = 0
#define VDIN0_AFBCE_YUV_FORMAT_CONV_MODE           0x022e
//Bit 31: 8       reserved
//Bit  7          reserved
//Bit  6: 4       reg_444to422_mode               // unsigned ,   RW, default = 0
//Bit  3          reserved
//Bit  2: 0       reg_422to420_mode               // unsigned ,   RW, default = 0
#define VDIN0_AFBCE_LOSS_CTRL                      0x022f
//Bit 31         reg_fix_cr_en                    // unsigned , RW, default = 1 enable of fix CR
//Bit 30         reg_rc_en                        // unsigned , RW, default = 0 enable of rc
//Bit 29         reg_write_q_level_mode           // unsigned , RW, default = 1 q_level write mode
//Bit 28         reg_debug_q_level_en             // unsigned , RW, default = 0 debug en
//Bit 27:20      reserved
//Bit 19:18      reserved
//Bit 17:16      reg_cal_bit_early_mode           // unsigned,RW, default = 2 early mode,RTL fix 2
//Bit 15:12      reg_quant_diff_root_leave        // unsigned,RW, default = 2
//diff value of bctroot and bcleave
//Bit 11: 8      reg_debug_q_level_value          // unsigned,RW, default = 0 debug q_level value
//Bit  7: 4      reg_debug_q_level_max_0          // unsigned,RW, default = 10 debug q_level value
//Bit  3: 0      reg_debug_q_level_max_1          // unsigned,RW, default = 10 debug q_level value
#define VDIN0_AFBCE_LOSS_BURST_NUM                 0x0230
//Bit 31:29      reserved
//Bit 28:24      reg_block_burst_num_0            // unsigned ,    RW, default = 5
//the num of burst of block
//Bit 23:21      reserved
//Bit 20:16      reg_block_burst_num_1            // unsigned ,    RW, default = 5
//the num of burst of block
//Bit 15:13      reserved
//Bit 12: 8      reg_block_burst_num_2            // unsigned ,    RW, default = 5
//the num of burst of block
//Bit  7: 5      reserved
//Bit  4: 0      reg_block_burst_num_3            // unsigned ,    RW, default = 5
//the num of burst of block
#define VDIN0_AFBCE_LOSS_RC                        0x0231
//Bit 31:29      reserved
//Bit 28         reg_rc_fifo_margin_mode          // unsigned ,    RW, default = 1
//enable of fifo margin mode
//Bit 27:24      reg_rc_1stbl_xbdgt               // unsigned ,    RW, default = 0
//extra bit budget for the first block in each line
//Bit 23:20      reg_rc_1stln_xbdgt               // unsigned ,    RW, default = 0
//extra bit budget for the first line
//Bit 19:18      reserved
//Bit 17:16      reg_rc_q_level_chk_mode          // unsigned ,    RW, default = 0
//0: max q_level 1: avg q_leve 2:max luma q_level 3 : avg luma q_level
//Bit 15: 8      reg_fix_cr_sub_b_bit_adj_0        // signed ,    RW, default = 0
//luma each sub block add bits normal >=0
//Bit  7: 0      reg_fix_cr_sub_b_bit_adj_1        // signed ,    RW, default = 0
//chroma each sub block minus bits normal <= 0
#define VDIN0_AFBCE_LOSS_RC_FIFO_THD               0x0232
//Bit 31:28      reserved
//Bit 27:16      reg_rc_fifo_margin_thd_0         // unsigned ,    RW, default = 8
//threshold of fifo level to guard the rc loop by adding delta to burst
//Bit 15:12      reserved
//Bit 11: 0      reg_rc_fifo_margin_thd_1         // unsigned ,    RW, default = 16
#define VDIN0_AFBCE_LOSS_RC_FIFO_BUGET             0x0233
//Bit 31:28      reserved
//Bit 27:16      reg_rc_fifo_margin_thd_2         // unsigned ,    RW, default = 32
//Bit 15:12      reg_rc_fifo_margin_buget_0       // unsigned ,    RW, default = 2
//delta of fifo level to guard the rc loop
//Bit 11: 8      reg_rc_fifo_margin_buget_1       // unsigned ,    RW, default = 1
//Bit  7: 4      reg_rc_fifo_margin_buget_2       // unsigned ,    RW, default = 0
//Bit  3: 0      reg_rc_max_add_buget             // unsigned ,    RW, default = 2
//the max add burst num
#define VDIN0_AFBCE_LOSS_RC_ACCUM_THD_0            0x0234
//Bit 31:28      reserved
//Bit 27:16      reg_rc_accum_margin_thd_2_2      // unsigned ,    RW, default = 8
//threshold of accum to guard the rc loop by adding delta to burst,
//Bit 15:12      reserved
//Bit 11: 0      reg_rc_accum_margin_thd_2_1      // unsigned ,    RW, default = 8
//threshold of accum to guard the rc loop by adding delta to burst,
#define VDIN0_AFBCE_LOSS_RC_ACCUM_THD_1            0x0235
//Bit 31:28      reserved
//Bit 27:16      reg_rc_accum_margin_thd_2_0      // unsigned ,    RW, default = 8
//threshold of accum to guard the rc loop by adding delta to burst,
//Bit 15:12      reserved
//Bit 11: 0      reg_rc_accum_margin_thd_1_2      // unsigned ,    RW, default = 128
//threshold of accum to guard the rc loop by adding delta to burst,
#define VDIN0_AFBCE_LOSS_RC_ACCUM_THD_2            0x0236
//Bit 31:28      reserved
//Bit 27:16      reg_rc_accum_margin_thd_1_1      // unsigned ,    RW, default = 64
//threshold of accum to guard the rc loop by adding delta to burst,
//Bit 15:12      reserved
//Bit 11: 0      reg_rc_accum_margin_thd_1_0      // unsigned ,    RW, default = 4
//threshold of accum to guard the rc loop by adding delta to burst,
#define VDIN0_AFBCE_LOSS_RC_ACCUM_THD_3            0x0237
//Bit 31:28      reserved
//Bit 27:16      reg_rc_accum_margin_thd_0_2      // unsigned ,    RW, default = 256
//threshold of accum to guard the rc loop by adding delta to burst,
//Bit 15:12      reserved
//Bit 11: 0      reg_rc_accum_margin_thd_0_1      // unsigned ,    RW, default = 128
//threshold of accum to guard the rc loop by adding delta to burst,
#define VDIN0_AFBCE_LOSS_RC_ACCUM_BUGET_0          0x0238
//Bit 31:20      reg_rc_accum_margin_thd_0_0      // unsigned ,    RW, default = 0
//threshold of accum to guard the rc loop by adding delta to burst,
//Bit 19:16      reserved
//Bit 15:12      reg_rc_accum_margin_buget_2_2    // unsigned ,    RW, default = 1
//delta of accum to guard the rc loop by adding delta to burst, default=[]
//Bit 11: 8      reg_rc_accum_margin_buget_2_1    // unsigned ,    RW, default = 0
//delta of accum to guard the rc loop by adding delta to burst, default=[]
//Bit  7: 4      reg_rc_accum_margin_buget_2_0    // unsigned ,    RW, default = 0
//delta of accum to guard the rc loop by adding delta to burst, default=[]
//Bit  3: 0      reg_rc_accum_margin_buget_1_2    // unsigned ,    RW, default = 2
//delta of accum to guard the rc loop by adding delta to burst, default=[]
#define VDIN0_AFBCE_LOSS_RC_ACCUM_BUGET_1          0x0239
//Bit 31:28      reg_rc_accum_margin_buget_1_1    // unsigned ,    RW, default = 1
//delta of accum to guard the rc loop by adding delta to burst, default=[]
//Bit 27:24      reg_rc_accum_margin_buget_1_0    // unsigned ,    RW, default = 1
//delta of accum to guard the rc loop by adding delta to burst, default=[]
//Bit 23:20      reg_rc_accum_margin_buget_0_2    // unsigned ,    RW, default = 2
//delta of accum to guard the rc loop by adding delta to burst, default=[]
//Bit 19:16      reg_rc_accum_margin_buget_0_1    // unsigned ,    RW, default = 1
//delta of accum to guard the rc loop by adding delta to burst, default=[]
//Bit 15:12      reg_rc_accum_margin_buget_0_0    // unsigned ,    RW, default = 1
//delta of accum to guard the rc loop by adding delta to burst, default=[]
//Bit 11: 8      reserved
//Bit  7: 4      reg_rc_q_level_chk_th_0          // unsigned ,    RW, default = 5
//threshold of q_level for adjust burst based on accum
//Bit  3: 0      reg_rc_q_level_chk_th_1          // unsigned ,    RW, default = 4
//threshold of q_level for adjust burst based on accum
#define VDIN0_AFBCE_LOSS_RO_ERROR_L_0              0x023a
//Bit 31: 0      ro_error_acc_mode0_l_0           // unsigned ,    RO, default = 0 square
#define VDIN0_AFBCE_LOSS_RO_COUNT_0                0x023b
//Bit 31:24      reserved
//Bit 23: 0      ro_enter_mode0_num_0             // unsigned ,    RO, default = 0 mode num contour
#define VDIN0_AFBCE_LOSS_RO_ERROR_L_1              0x023c
//Bit 31: 0      ro_error_acc_mode0_l_1           // unsigned ,    RO, default = 0 square
#define VDIN0_AFBCE_LOSS_RO_COUNT_1                0x023d
//Bit 31:24      reserved
//Bit 23: 0      ro_enter_mode0_num_1             // unsigned ,    RO, default = 0 mode num contour
#define VDIN0_AFBCE_LOSS_RO_ERROR_L_2              0x023e
//Bit 31: 0      ro_error_acc_mode0_l_2           // unsigned ,    RO, default = 0 square
#define VDIN0_AFBCE_LOSS_RO_COUNT_2                0x023f
//Bit 31:24      reserved
//Bit 23: 0      ro_enter_mode0_num_2             // unsigned ,    RO, default = 0 mode num contour
#define VDIN0_AFBCE_LOSS_RO_ERROR_H_0              0x0240
//Bit 31:16      ro_error_acc_mode0_h_2           // unsigned ,    RO, default = 0  square
//Bit 15: 0      ro_error_acc_mode0_h_1           // unsigned ,    RO, default = 0  square
#define VDIN0_AFBCE_LOSS_RO_ERROR_H_1              0x0241
//Bit 31:16      reserved
//Bit 15: 0      ro_error_acc_mode0_h_0           // unsigned ,    RO, default = 0  square
#define VDIN0_AFBCE_LOSS_RO_MAX_ERROR_0            0x0242
//Bit 31:28      reserved
//Bit 27:16      ro_max_error_mode0_2             // unsigned ,    RO, default = 0  error
//Bit 15:12      reserved
//Bit 11: 0      ro_max_error_mode0_1             // unsigned ,    RO, default = 0  error
#define VDIN0_AFBCE_LOSS_RO_MAX_ERROR_1            0x0243
//Bit 31:12      reserved
//Bit 11: 0      ro_max_error_mode0_0             // unsigned ,    RO, default = 0  error
#define VDIN0_AFBCE_ENABLE                         0x0244
//Bit 31:20       reg_gclk_ctrl                   // unsigned ,   RW, default = 0
//12'hfff:always open 12'h000:gating clock
//Bit 19:16       reg_afbce_sync_sel              // unsigned ,   RW, default = 0
//4'hf:registers sync by vsync 4'h0:registers don't sync
//Bit 15:14       reserved
//Bit 13          reg_enc_rst_mode                // unsigned ,   RW, default = 0
//1:soft reset by write pulse into registers AFBCE_MODE[29]  0:auto reset by vsync
//Bit 12          reg_enc_en_mode                 // unsigned ,   RW, default = 0
//1:start afbce by write a pulse into AFBCE_ENABLE[0]  0:auto start several lines after vsync
//Bit 11: 9       reserved
//Bit  8          reg_enc_enable                  // unsigned ,   RW, default = 0
//1:afbce enable 0:afbce disable
//Bit  7: 1       reserved
//Bit  0          reg_pls_enc_frm_start           // unsigned ,   W1C, default = 0
//reg_pls_enc_frm_start pulse
#define VDIN0_AFBCE_MODE                           0x0245
//Bit 31:29       reg_soft_rst                    // unsigned ,   RW, default = 0
//active high bit0:async reset afbce by write a pulse bit1:async reset afbce after vsync
//bit2:sync reset after vsync
//Bit 28          reserved
//Bit 27:26       reg_rev_mode                    // unsigned ,   RW, default = 0
//reverse mode bit0:x reverse 1:y reverse
//Bit 25:24       reg_mif_urgent                  // unsigned ,   RW, default = 3
//info mif and data mif urgent
//Bit 23          reserved
//Bit 22:16       reg_hold_line_num               // unsigned ,   RW, default = 4
//0: burst1 1:burst2 2:burst4
//Bit 15:14       reg_burst_mode                  // unsigned ,   RW, default = 2
//0: burst1 1:burst2 2:burst4
//Bit 13: 1       reserved
//Bit  0          reg_fmt444_comb                 // unsigned ,   RW, default = 0
//0: 444 8bit comb mode
#define VDIN0_AFBCE_SIZE_IN                        0x0246
//Bit 31:16       reg_hsize_bg                    // unsigned ,   RW, default = 1920
//pic horz background size in  unit pixel
//Bit 15: 0       reg_vsize_bg                    // unsigned ,   RW, default = 1080
//pic vert background size in  unit pixel
#define VDIN0_AFBCE_BLK_SIZE_IN                    0x0247
//Bit 31:16       reg_hblk_size                   // unsigned ,   RW, default = 60
//pic horz size in block
//Bit 15: 0       reg_vblk_size                   // unsigned ,   RW, default = 270
//pic vert size in block
#define VDIN0_AFBCE_HEAD_BADDR                     0x0248
//Bit 31: 0       reg_head_baddr                  // unsigned ,   RW, default = 32'h00 head address
#define VDIN0_AFBCE_MIF_SIZE                       0x0249
//Bit 31:30       reserved
//Bit 29:28       reg_ddr_blk_size                // unsigned ,   RW, default = 1
//Bit 27          reserved
//Bit 26:24       reg_cmd_blk_size                // unsigned ,   RW, default = 3
//Bit 23:16       reg_uncmp_size                  // unsigned ,   RW, default = 20
//Bit 15: 0       reg_mmu_page_size               // unsigned ,   RW, default = 4096
#define VDIN0_AFBCE_PIXEL_IN_HOR_SCOPE             0x024a
//Bit 31:16       reg_enc_win_end_h               // unsigned ,   RW, default = 1919
//Bit 15: 0       reg_enc_win_bgn_h               // unsigned ,   RW, default = 0
#define VDIN0_AFBCE_PIXEL_IN_VER_SCOPE             0x024b
//Bit 31:16       reg_enc_win_end_v               // unsigned ,   RW, default = 1079
//Bit 15: 0       reg_enc_win_bgn_v               // unsigned ,   RW, default = 0
#define VDIN0_AFBCE_CONV_CTRL                      0x024c
//Bit 31:29       reserved
//Bit 28:16       reg_fmt_ybuf_depth              // unsigned ,   RW, default = 2048
//Bit 15:12       reserved
//Bit 11: 0       reg_lbuf_depth                  // unsigned ,   RW, default = 256
//unit=16 pixel need to set = 2^n
#define VDIN0_AFBCE_MIF_HOR_SCOPE                  0x024d
//Bit 31:28       reserved
//Bit 27:16       reg_blk_end_h                   // unsigned ,   RW, default = 0
//Bit 15:12       reserved
//Bit 11: 0       reg_blk_bgn_h                   // unsigned ,   RW, default = 59
#define VDIN0_AFBCE_MIF_VER_SCOPE                  0x024e
//Bit 31:28       reserved
//Bit 27:16       reg_blk_end_v                   // unsigned ,   RW, default = 0
//Bit 15:12       reserved
//Bit 11: 0       reg_blk_bgn_v                   // unsigned ,   RW, default = 269
#define VDIN0_AFBCE_STAT1                          0x024f
//Bit 31          ro_frm_end_pulse1               // unsigned ,   RO  default = 0 frame end status
//Bit 30: 0       ro_dbg_top_info1                // unsigned ,   RO  default = 0
#define VDIN0_AFBCE_STAT2                          0x0250
//Bit 31: 0       ro_dbg_top_info2                // unsigned ,   RO  default = 0
#define VDIN0_AFBCE_DUMMY_DATA                     0x0251
//Bit 31:30       reserved
//Bit 29: 0       reg_dummy_data                  // unsigned ,   RW, default = 0
#define VDIN0_AFBCE_CLR_FLAG                       0x0252
//Bit 31: 0       reg_pls_afbce_clr_flag          // unsigned ,   RW, default = 0
#define VDIN0_AFBCE_STA_FLAGT                      0x0253
//Bit 31: 0       ro_afbce_sta_flag               // unsigned ,   RO  default = 0
#define VDIN0_AFBCE_MMU_NUM                        0x0254
//Bit 31:16       reserved
//Bit 15: 0       ro_frm_mmu_num                  // unsigned ,   RO  default = 0
#define VDIN0_AFBCE_PIP_CTRL                       0x0255
//Bit 31: 3       reserved
//Bit  2          reg_enc_align_en                // unsigned ,   RW, default = 1
//Bit  1          reg_pip_ini_ctrl                // unsigned ,   RW, default = 0
//Bit  0          reg_pip_mode                    // unsigned ,   RW, default = 0
#define VDIN0_AFBCE_ROT_CTRL                       0x0256
//Bit 31: 5       reserved
//Bit  4          reg_rot_en                      // unsigned ,   RW, default = 0
//rotation enable
//Bit  3: 0       reg_vstep                       // unsigned ,   RW, default = 8
//rotation vstep  setting acorrding rotation shrink mode
#define VDIN0_AFBCE_DIMM_CTRL                      0x0257
//Bit 31          reg_dimm_layer_en               // unsigned ,   RW, default = 0
//dimm_layer enable signal
//Bit 30          reserved
//Bit 29: 0       reg_dimm_data                   // unsigned ,   RW, default = 29'h00080200
//dimm_layer data
#define VDIN0_AFBCE_BND_DEC_MISC                   0x0258
//Bit 31:28       reserved
//Bit 27:26       reg_bnd_dec_rev_mode            // unsigned ,   RW, default = 0
//only pip mode use those bits usually don't need configure
//Bit 25:24       reg_bnd_dec_mif_urgent          // unsigned ,   RW, default = 3
//only pip mode use those bits usually don't need configure
//Bit 23:22       reg_bnd_dec_burst_len           // unsigned ,   RW, default = 2
//only pip mode use those bits usually don't need configure
//Bit 21:20       reg_bnd_dec_ddr_blk_size        // unsigned ,   RW, default = 1
//only pip mode use those bits usually don't need configure
//Bit 19          reserved
//Bit 18:16       reg_bnd_dec_cmd_blk_size        // unsigned ,   RW, default = 3
//only pip mode use those bits usually don't need configure
//Bit 15          reserved
//Bit 14          reg_bnd_dec_blk_mem_mode        // unsigned ,   RW, default = 0
//only pip mode use those bits usually don't need configure
//Bit 13          reg_bnd_dec_addr_link_en        // unsigned ,   RW, default = 1
//only pip mode use those bits usually don't need configure
//Bit 12          reg_bnd_dec_always_body_rden    // unsigned ,   RW, default = 0
//only pip mode use those bits usually don't need configure
//Bit 11: 0       reg_bnd_dec_mif_lbuf_depth      // unsigned ,   RW, default = 128
//only pip mode use those bits usually don't need configure
#define VDIN0_AFBCE_RD_ARB_MISC                    0x0259
//Bit 31:13       reserved
//Bit 12          reg_arb_sw_rst                  // unsigned ,   RW, default = 0
//only pip mode use those bits usually don't need configure
//Bit 11:10       reserved
//Bit  9          reg_arb_arblk_last1             // unsigned ,   RW, default = 0
//only pip mode use those bits usually don't need configure
//Bit  8          reg_arb_arblk_last0             // unsigned ,   RW, default = 0
//only pip mode use those bits usually don't need configure
//Bit  7: 4       reg_arb_weight_ch1              // unsigned ,   RW, default = 4
//only pip mode use those bits usually don't need configure
//Bit  3: 0       reg_arb_weight_ch0              // unsigned ,   RW, default = 10
//only pip mode use those bits usually don't need configure
#define VDIN0_AFBCE_MMU_RMIF_CTRL1                 0x025a
//Bit 31:26       reserved
//Bit 25:24       reg_sync_sel                    // unsigned ,   RW, default = 0
//axi canvas id sync with frm rst
//Bit 23:16       reg_canvas_id                   // unsigned ,   RW, default = 0
//axi canvas id num
//Bit 15          reserved
//Bit 14:12       reg_cmd_intr_len                // unsigned ,   RW, default = 1
//interrupt send cmd when how many series axi cmd, 0=12 1=16 2=24 3=32 4=40 5=48 6=56 7=64
//Bit 11:10       reg_cmd_req_size                // unsigned ,   RW, default = 1
//how many room fifo have, then axi send series req, 0=16 1=32 2=24 3=64
//Bit  9: 8       reg_burst_len                   // unsigned ,   RW, default = 2
//burst type: 0-single 1-bst2 2-bst4
//Bit  7          reg_swap_64bit                  // unsigned ,   RW, default = 0
//64bits of 128bit swap enable
//Bit  6          reg_little_endian               // unsigned ,   RW, default = 1
//big endian enable
//Bit  5          reg_y_rev                       // unsigned ,   RW, default = 0
//vertical reverse enable
//Bit  4          reg_x_rev                       // unsigned ,   RW, default = 0
//horizontal reverse enable
//Bit  3          reserved
//Bit  2: 0       reg_pack_mode                   // unsigned ,   RW, default = 3
//0:4bit 1:8bit 2:16bit 3:32bit 4:64bit 5:128bit
#define VDIN0_AFBCE_MMU_RMIF_CTRL2                 0x025b
//Bit 31:30       reg_sw_rst                      // unsigned ,   RW, default = 0
//Bit 29:22       reserved
//Bit 21:20       reg_int_clr                     // unsigned ,   RW, default = 0
//Bit 19:18       reg_gclk_ctrl                   // unsigned ,   RW, default = 0
//Bit 17          reserved
//Bit 16: 0       reg_urgent_ctrl                 // unsigned ,   RW, default = 0
//urgent control {[16] reg_ugt_init: urgent initial value}, {[15] reg_ugt_en: urgent enable},
//{[14] reg_ugt_type: 1= wrmif 0=rdmif}, {[7:4] reg_ugt_top_th: urgent top threshold},
//{[3:0] reg_ugt_bot_th: urgent bottom threshold}
#define VDIN0_AFBCE_MMU_RMIF_CTRL3                 0x025c
//Bit 31:24       reserved
//Bit 23:20       reg_vstep                       // unsigned ,   RW, default = 1
//Bit 19:17       reserved
//Bit 16          reg_acc_mode                    // unsigned ,   RW, default = 1
//Bit 15:13       reserved
//Bit 12: 0       reg_stride                      // unsigned ,   RW, default = 4096
#define VDIN0_AFBCE_MMU_RMIF_CTRL4                 0x025d
//Bit 31: 0       reg_baddr                       // unsigned ,   RW, default = 0
#define VDIN0_AFBCE_MMU_RMIF_SCOPE_X               0x025e
//Bit 31:29       reserved
//Bit 28:16       reg_x_end                       // unsigned ,   RW, default = 4095
//the canvas hor end pixel position
//Bit 15:13       reserved
//Bit 12: 0       reg_x_start                     // unsigned ,   RW, default = 0
//the canvas hor start pixel position
#define VDIN0_AFBCE_MMU_RMIF_SCOPE_Y               0x025f
//Bit 31:29       reserved
//Bit 28:16       reg_y_end                       // unsigned ,   RW, default = 0
//the canvas ver end pixel position
//Bit 15:13       reserved
//Bit 12: 0       reg_y_start                     // unsigned ,   RW, default = 0
//the canvas ver start pixel position
#define VDIN0_AFBCE_MMU_RMIF_RO_STAT               0x0260
//Bit 31:16       reserved
//Bit 15: 0       reg_status                      // unsigned ,   RO, default = 0
#define VDIN0_META_DSC_CTRL0                       0x0270
//Bit 31           reg_meta_dolby_check_en          // unsigned ,    RW, default = 1
//Bit 30           reg_meta_tunnel_swap_en          // unsigned ,    RW, default = 0
//Bit 29:24        reg_meta_soft_rst                // unsigned ,    RW, default = 0
//Bit 23:18        reserved
//Bit 17           reg_meta_frame_rst               // unsigned ,    RW, default = 1
//Bit 16           reg_meta_lsb                     // unsigned ,    RW, default = 1
//Bit 15: 0        reg_meta_data_bytes              // unsigned ,    RW, default = 128
#define VDIN0_META_DSC_CTRL1                       0x0271
//Bit 31:16        reg_meta_data_start              // unsigned ,    RW, default = 0
//Bit 15: 8        reserved
//Bit  7: 0        reg_meta_crc_ctrl                // unsigned ,    RW, default = 3
#define VDIN0_META_DSC_CTRL2                       0x0272
//Bit 31           reg_meta_rd_en                   // unsigned ,    RW, default = 0
//Bit 30           reg_meta_rd_mode                 // unsigned ,    RW, default = 0
//Bit 29:20        reg_meta_wr_sum                  // unsigned ,    RW, default = 280
//Bit 19:18        reserved
//Bit 17: 0        reg_meta_tunnel_sel              // unsigned ,    RW, default = 18'h2c2d0
#define VDIN0_META_DSC_CTRL3                       0x0273
//Bit 31:16        reg_meta_start_addr              // unsigned ,    RW, default = 0
//Bit 15: 0        reg_meta_byte_select             // unsigned ,    RW, default = 0
#define VDIN0_META_AXI_CTRL0                       0x0274
//Bit 31: 0        reg_meta_axi_ctrl0               // unsigned ,    RW, default = 32'h97181000
#define VDIN0_META_AXI_CTRL1                       0x0275
//Bit 31: 0        reg_meta_axi_ctrl1               // unsigned ,    RW, default = 32'h8000000
#define VDIN0_META_AXI_CTRL2                       0x0276
//Bit 31: 0        reg_meta_axi_ctrl2               // unsigned ,    RW, default = 32'h10001000
#define VDIN0_META_AXI_CTRL3                       0x0277
//Bit 31: 0        reg_meta_axi_ctrl3               // unsigned ,    RW, default = 32'h10
#define VDIN0_META_DSC_STATUS0                     0x0278
//Bit 31: 8        reg_meta_axi_status0             // unsigned ,    RO, default = 0
//Bit  7: 0        reg_meta_byte                    // unsigned ,    RO, default = 0
#define VDIN0_META_DSC_STATUS1                     0x0279
//Bit 31:0         reg_meta_sram                    // unsigned ,    RO, default = 0
#define VDIN0_META_DSC_STATUS2                     0x027a
//Bit 31:28        reg_meta_axi_status1             // unsigned ,    RO, default = 0
//Bit 27           reg_meta_valid                   // unsigned ,    RO, default = 0
//Bit 26:21        reserved
//Bit 20:19        reg_meta_crc_status              // unsigned ,    RO, default = 0
//Bit 18:11        reg_meta_crc_count               // unsigned ,    RO, default = 0
//Bit 10           reg_meta_crc_pass                // unsigned ,    RO, default = 0
//Bit  9: 0        reg_meta_addr                    // unsigned ,    RO, default = 0
#define VDIN0_META_DSC_STATUS3                     0x027b
//Bit 31: 0        reg_meta_axi_status2             // unsigned ,    RO, default = 0
/* t3x new added registers bank 0 end */

/* t3x new added registers bank 1 begin */
//
// Closing file:  ./vdin0_core_top_p0_reg.h
//
//===========================================================================
// -----------------------------------------------
// CBUS_BASE:  VDIN0_CORE_TOP_P1_VCBUS_BASE = 0x03
// -----------------------------------------------
//===========================================================================
//
// Reading file:  ./vdin0_core_top_p1_reg.h
//
#define VDIN0_PP_GCLK_CTRL                         0x0300
//Bit 31:30           reg_top_gclk_ctrl       // unsigned ,    RW, default = 0
//Bit 29:24           reserved
//Bit 23:22           reg_mat0_gclk_ctrl      // unsigned ,    RW, default = 0
//Bit 21:16           reg_cm_gclk_ctrl        // unsigned ,    RW, default = 0
//Bit 15:14           reg_bri_gclk_ctrl       // unsigned ,    RW, default = 0
//Bit 13:12           reg_phsc_gclk_ctrl      // unsigned ,    RW, default = 0
//Bit 11:10           reg_hsc_gclk_ctrl       // unsigned ,    RW, default = 0
//Bit  9: 8           reg_vsc_gclk_ctrl       // unsigned ,    RW, default = 0
//Bit  7: 6           reg_hdr2_mat1_gclk_ctrl // unsigned ,    RW, default = 0
//Bit  5: 4           reg_lfifo_gclk_ctrl     // unsigned ,    RW, default = 0
//Bit  3: 2           reg_hist_gclk_ctrl      // unsigned ,    RW, default = 0
//Bit  1: 0           reg_blkbar_gclk_ctrl    // unsigned ,    RW, default = 0
#define VDIN0_PP_CTRL                              0x0301
//Bit 31:30           reg_crc_ctrl            // unsigned ,    RW, default = 0
//Bit 29:23           reserved
//Bit 22              reg_cutwin_en           // unsigned ,    RW, default = 0
//Bit 21              reg_discard_data_en     // unsigned ,    RW, default = 0
//Bit 20:16           reserved
//Bit 15:14           reg_blkbar_det_sel      // unsigned ,    RW, default = 0
//Bit 13:12           reg_luma_hist_sel       // unsigned ,    RW, default = 0
//Bit 11:10           reserved
//Bit  9              reg_scl_mode            // unsigned ,    RW, default = 0
//Bit  8              reg_blkbar_det_en       // unsigned ,    RW, default = 0
//Bit  7              reg_luma_hist_en        // unsigned ,    RW, default = 0
//Bit  6              reg_cm_en               // unsigned ,    RW, default = 0
//Bit  5              reg_bri_en              // unsigned ,    RW, default = 0
//Bit  4              reg_phsc_en             // unsigned ,    RW, default = 0
//Bit  3              reg_hsc_en              // unsigned ,    RW, default = 0
//Bit  2              reg_vsc_en              // unsigned ,    RW, default = 0
//Bit  1              reg_mat0_en             // unsigned ,    RW, default = 0
//Bit  0              reg_mat1_en             // unsigned ,    RW, default = 0
#define VDIN0_PP_IN_SIZE                           0x0302
//Bit 31:29           reserved
//Bit 28:16           reg_in_hsize            // unsigned ,    RW, default = 0
//Bit 15:13           reserved
//Bit 12:0            reg_in_vsize            // unsigned ,    RW, default = 0
#define VDIN0_PP_OUT_SIZE                          0x0303
//Bit 31:29           reserved
//Bit 28:16           reg_out_hsize           // unsigned ,    RW, default = 0
//Bit 15:13           reserved
//Bit 12:0            reg_out_vsize           // unsigned ,    RW, default = 0
#define VDIN0_PP_SCL_IN_SIZE                       0x0304
//Bit 31:29           reserved
//Bit 28:16           reg_scl_in_hsize        // unsigned ,    RW, default = 0
//Bit 15:13           reserved
//Bit 12:0            reg_scl_in_vsize        // unsigned ,    RW, default = 0
#define VDIN0_PP_SCL_OUT_SIZE                      0x0305
//Bit 31:29           reserved
//Bit 28:16           reg_scl_out_hsize       // unsigned ,    RW, default = 0
//Bit 15:13           reserved
//Bit 12:0            reg_scl_out_vsize       // unsigned ,    RW, default = 0
#define VDIN0_PP_CRC_OUT                           0x0306
//Bit 31: 0           reg_crc_out             // unsigned ,    RO, default = 0
#define VDIN0_CUTWIN_CTRL                          0x0308
//Bit 31: 6           reserved
//Bit  5: 4           reg_cutwin_swap2           // unsigned ,    RW, default = 2
//Bit  3: 2           reg_cutwin_swap1           // unsigned ,    RW, default = 1
//Bit  1: 0           reg_cutwin_swap0           // unsigned ,    RW, default = 0
#define VDIN0_CUTWIN_H_WIN                         0x0309
//Bit 31:29           reserved
//Bit 28:16           reg_cutwin_h_start         // unsigned ,    RW, default = 0
//Bit 15:13           reserved
//Bit 12:0            reg_cutwin_h_end           // unsigned ,    RW, default = 13'h1fff
#define VDIN0_CUTWIN_V_WIN                         0x030a
//Bit 31:29           reserved
//Bit 28:16           reg_cutwin_v_start         // unsigned ,    RW, default = 0
//Bit 15:13           reserved
//Bit 12:0            reg_cutwin_v_end           // unsigned ,    RW, default = 13'h1fff
#define VDIN0_MAT_CTRL                             0x030c
//Bit 31:30        reg_mat_gclk_ctrl      // unsigned ,    RW, default = 0
//Bit 29:27        reserved
//Bit 26:24        reg_mat_conv_rs        // unsigned ,    RW, default = 0
//Bit 23: 9        reserved
//Bit  8           reg_mat_hl_en          // unsigned ,    RW, default = 0
//Bit  7: 2        reserved
//Bit  1           reg_mat_probe_sel      // unsigned ,    RW, default = 1
//Bit  0           reg_mat_probe_post     // unsigned ,    RW, default = 0
#define VDIN0_MAT_HL_COLOR                         0x030d
//Bit 31:24        reserved
//Bit 23: 0        reg_mat_hl_color       // unsigned ,    RW, default = 0
#define VDIN0_MAT_PROBE_POS                        0x030e
//Bit 31:16        reg_mat_probe_posx     // unsigned ,    RW, default = 0
//Bit 15: 0        reg_mat_probe_posy     // unsigned ,    RW, default = 0
#define VDIN0_MAT_COEF00_01                        0x030f
//Bit 31:29        reserved
//Bit 28:16        reg_mat_coef00         // unsigned ,    RW, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_mat_coef01         // unsigned ,    RW, default = 0
#define VDIN0_MAT_COEF02_10                        0x0310
//Bit 31:29        reserved
//Bit 28:16        reg_mat_coef02         // unsigned ,    RW, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_mat_coef10         // unsigned ,    RW, default = 0
#define VDIN0_MAT_COEF11_12                        0x0311
//Bit 31:29        reserved
//Bit 28:16        reg_mat_coef11         // unsigned ,    RW, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_mat_coef12         // unsigned ,    RW, default = 0
#define VDIN0_MAT_COEF20_21                        0x0312
//Bit 31:29        reserved
//Bit 28:16        reg_mat_coef20         // unsigned ,    RW, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_mat_coef21         // unsigned ,    RW, default = 0
#define VDIN0_MAT_COEF22                           0x0313
//Bit 31:13        reserved
//Bit 12: 0        reg_mat_coef22         // unsigned ,    RW, default = 0
#define VDIN0_MAT_OFFSET0_1                        0x0314
//Bit 31:29        reserved
//Bit 28:16        reg_mat_offset0        // unsigned ,    RW, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_mat_offset1        // unsigned ,    RW, default = 0
#define VDIN0_MAT_OFFSET2                          0x0315
//Bit 31:13        reserved
//Bit 12: 0        reg_mat_offset2        // unsigned ,    RW, default = 0
#define VDIN0_MAT_PRE_OFFSET0_1                    0x0316
//Bit 31:29        reserved
//Bit 28:16        reg_mat_pre_offset0    // unsigned ,    RW, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_mat_pre_offset1    // unsigned ,    RW, default = 0
#define VDIN0_MAT_PRE_OFFSET2                      0x0317
//Bit 31:13        reserved
//Bit 12: 0        reg_mat_pre_offset2    // unsigned ,    RW, default = 0
#define VDIN0_MAT_PROBE_COLOR                      0x0318
//Bit 31:0         reg_mat_probe_color    // unsigned ,    RO, default = 0
#define VDIN0_CM_ADDR_PORT                         0x031c
//Bit 31:10   reserved
//Bit  9: 0   reg_cm_addr_port    // unsigned ,    RW, default = 0
#define VDIN0_CM_DATA_PORT                         0x031d
//Bit 31: 0   reg_cm_data_port    // unsigned ,    RW, default = 0
#define VDIN0_BRI_CTRL                             0x0320
//Bit 31:30        reg_bri_gclk_ctrl      // unsigned ,    RW, default = 0
//Bit 29           reserved
//Bit 28:26        reg_bri_sde_yuvinven   // unsigned ,    RW, default = 0
//Bit 25:23        reserved
//Bit 22:12        reg_bri_adj_bri        // unsigned ,    RW, default = 0
//Bit 11: 0        reg_bri_adj_con        // unsigned ,    RW, default = 0
#define VDIN0_PRE_HSCALE_INI_CTRL                  0x0322
//Bit 31:24        reserved
//Bit 23:22        reg_phsc_gclk_ctrl        // unsigned ,    RW, default = 0
//Bit 21           reserved
//Bit 20:17        reg_prehsc_mode           // unsigned ,    RW, default = 0  ,
//prehsc_mode, bit 3:2, prehsc odd line	interp mode, bit 1:0, prehsc even line interp mode,
//each 2bit, 00 pix0+pix1/2, average,	01:pix1,10:pix0
//Bit 16           reserved
//Bit 15:8         reg_prehsc_pattern        // unsigned ,    RW, default = 0  ,
//prehsc	pattern,   each	patten	1	bit,	from	lsb	->	msb
//Bit 7            reserved
//Bit 6:4          reg_prehsc_pat_star       // unsigned ,    RW, default = 0  ,
//prehsc	pattern	start
//Bit 3            reserved
//Bit 2:0          reg_prehsc_pat_end        // unsigned ,    RW, default = 0  ,
//prehsc	pattern	end
#define VDIN0_PRE_HSCALE_COEF_0                    0x0323
//Bit 31:26        reserved
//Bit 25:16        reg_prehsc_coef_1         // signed   ,    RW, default = 0  ,
//coefficient0  pre horizontal	filter
//Bit 15:10        reserved
//Bit  9: 0        reg_prehsc_coef_0         // signed   ,    RW, default = 256,
//coefficient1	pre horizontal	filter
#define VDIN0_PRE_HSCALE_FLT_NUM                   0x0324
//Bit 31:29        reserved
//Bit 28:25        reg_preh_hb_num           // unsigned ,    RW, default = 8  ,
//prehsc rtl h blank number
//Bit 24:21        reg_preh_vb_num           // unsigned ,    RW, default = 8  ,
//prehsc rtl v blank number
//Bit 20:12        reserved
//Bit 11: 8        reg_prehsc_flt_num        // unsigned ,    RW, default = 2  ,
//prehsc filter tap num
//Bit  7: 2        reserved
//Bit  1: 0        reg_prehsc_rate           // unsigned ,    RW, default = 1  ,
//pre hscale down rate, 0:width,1:width/2,2:width/4,3:width/8
#define VDIN0_PRE_HSCALE_COEF_1                    0x0325
//Bit 31:26        reserved
//Bit 25:16        reg_prehsc_coef_3         // signed   ,    RW, default = 0  ,
//coefficient2	pre horizontal	filter
//Bit 15:10        reserved
//Bit  9: 0        reg_prehsc_coef_2         // signed   ,    RW, default = 0  ,
//coefficient3	pre horizontal	filter
#define VDIN0_HSC_REGION12_STARTP                  0x0326
//Bit 31:29        reserved
//Bit 28:16        reg_hsc_region1_startp    // unsigned ,    RW, default = 0 ,region1	startp
//Bit 15:13        reserved
//Bit 12: 0        reg_hsc_region2_startp    // unsigned ,    RW, default = 0 ,region2	startp
#define VDIN0_HSC_REGION34_STARTP                  0x0327
//Bit 31:29        reserved
//Bit 28:16        reg_hsc_region3_startp    // unsigned ,    RW, default = 13'h780,region3 startp
//Bit 15:13        reserved
//Bit 12: 0        reg_hsc_region4_startp    // unsigned ,    RW, default = 13'h780,region4 startp
#define VDIN0_HSC_REGION4_ENDP                     0x0328
//Bit 31:24        reg_hsc_ini_phase1_exp    // unsigned ,    RW, default = 0  ,
//horizontal	scaler	for slice expand 8bit
//Bit 23:16        reg_hsc_ini_phase0_exp    // unsigned ,    RW, default = 0  ,
//horizontal	scaler  for slice expand 8bit
//Bit 15:13        reserved
//Bit 12: 0        reg_hsc_region4_endp      // unsigned ,    RW, default = 1919 ,region4 startp
#define VDIN0_HSC_START_PHASE_STEP                 0x0329
//Bit 31:28        reserved
//Bit 27:24        reg_hsc_integer_part      // unsigned ,    RW, default = 1,integer part of step
//Bit 23: 0        reg_hsc_fraction_part     // unsigned ,    RW, default = 0,fraction part of step
#define VDIN0_HSC_REGION0_PHASE_SLOPE              0x032a
//Bit 31:25        reserved
//Bit 24: 0        reg_hsc_region0_phase_slope // unsigned ,  RW, default = 0  ,region0	phase slope
#define VDIN0_HSC_REGION1_PHASE_SLOPE              0x032b
//Bit 31:25        reserved
//Bit 24: 0        reg_hsc_region1_phase_slope // unsigned ,  RW, default = 0  ,region1	phase slope
#define VDIN0_HSC_REGION3_PHASE_SLOPE              0x032c
//Bit 31:25        reserved
//Bit 24: 0        reg_hsc_region3_phase_slope // unsigned ,  RW, default = 0  ,region3	phase slope
#define VDIN0_HSC_REGION4_PHASE_SLOPE              0x032d
//Bit 31:25        reserved
//Bit 24: 0        reg_hsc_region4_phase_slope // unsigned ,  RW, default = 0  ,region4 phase slope
#define VDIN0_HSC_PHASE_CTRL                       0x032e
//Bit 31:27        reserved
//Bit 26:24        reg_hsc_ini_rcv_num0_exp  // unsigned ,    RW, default = 0  ,
//horizontal	scaler	initial	receiving number0 expand
//Bit 23:20        reg_hsc_rpt_p0_num0       // unsigned ,    RW, default = 1  ,
//horizontal	scaler	initial	repeat	pixel0	number0
//Bit 19:16        reg_hsc_ini_rcv_num0      // unsigned ,    RW, default = 4  ,
//horizontal	scaler	initial	receiving number0
//Bit 15: 0        reg_hsc_ini_phase0        // unsigned ,    RW, default = 0  ,
//horizontal	scaler	top	field initial phase0
#define VDIN0_HSC_SC_MISC                          0x032f
//Bit 31:7        reserved
//Bit 6           reg_sc_coef_s11_mode         // unsigned ,  RW, default = 0  ,
//sc coef bit-width 0:s9, 1:s11
//Bit 5           reg_hf_sep_coef_4srnet_en    // unsigned ,  RW, default = 0  ,
//if true, horizontal separated coef in normal path for SRNet enable
//Bit 4           reg_hsc_nonlinear_4region_en // unsigned ,  RW, default = 0  ,
//if	true, region0,region4 are nonlinear	regions,
//otherwise they	are	not	scaling	regions, for horizontal	scaler
//Bit 3:0         reg_hsc_bank_length        // unsigned ,    RW, default = 4 ,
//horizontal	scaler	bank	length
#define VDIN0_HSC_PHASE_CTRL1                      0x0330
//Bit 31:29        reg_hsc_ini_rcv_num1_exp  // unsigned ,    RW, default = 0 ,
//horizontal	scaler	initial	receiving number1 expand
//Bit 28           reg_hsc_double_pix_mode   // unsigned ,    RW, default = 0 ,
//horizontal	scaler	double	pixel	mode
//Bit 27:24        reserved
//Bit 23:20        reg_hsc_rpt_p0_num1       // unsigned ,    RW, default = 1 ,
//horizontal	scaler	initial	repeat	pixel0	number1
//Bit 19:16        reg_hsc_ini_rcv_num1      // unsigned ,    RW, default = 4 ,
//horizontal	scaler	initial	receiving	number1
//Bit 15: 0        reg_hsc_ini_phase1        // unsigned ,    RW, default = 0 ,
//horizontal	scaler	top	field	initial	phase1
#define VDIN0_HSC_INI_PAT_CTRL                     0x0331
//Bit 31:16        reserved
//Bit 15: 8        reg_hsc_pattern           // unsigned ,    RW, default = 0 ,
//hsc	pattern,	each	patten	1	bit,	from	lsb	->	msb
//Bit  7           reserved
//Bit  6: 4        reg_hsc_pat_start         // unsigned ,    RW, default = 0 , hsc pattern start
//Bit  3           reserved
//Bit  2: 0        reg_hsc_pat_end           // unsigned ,    RW, default = 0 , hsc pattern end
#define VDIN0_HSC_EXTRA                            0x0332
//Bit 31:19        reserved
//Bit 18:17        reg_hsc_gclk_ctrl         // unsigned ,    RW, default = 0
//Bit 16:13        reg_hsc_nor_rs_bits       // unsigned ,    RW, default = 7
//normalize right shift bits of hsc
//Bit 12:11        reg_hsc_sp422_mode        // unsigned ,    RW, default = 0
//Bit 10           reg_hsc_phase0_always_en  // unsigned ,    RW, default = 0
//Bit  9           reg_hsc_nearest_en        // unsigned ,    RW, default = 0
//Bit  8           reg_hsc_short_lineo_en    // unsigned ,    RW, default = 0
//Bit  7           reserved
//Bit  6:0         reg_hsc_ini_pixi_ptr      // unsigned ,    RW, default = 0
#define VDIN0_CNTL_SCALE_COEF_IDX                  0x0333
//Bit 31:21        reserved
//Bit 20:17        reg_type_index_ext        // unsigned ,    RW, default = 0 ,
//type of index, 000: vertical coef, 001: vertical chroma coef: 010: horizontal coef part A,
//011: horizontal coef part B 100: horizontal chroma coef part A 101:horizontal chroma coef part B
//Bit 16           reg_ctype_ext_mode        // unsigned ,    RW, default = 0 ,
//if true use type_index_ext rather than reg_type_index
//Bit 15           reg_index_inc             // unsigned ,    RW, default = 0 ,
//default = 0x0 ,index increment, if bit9 == 1  then (0: index increase 1,
//1: index increase 2) else (index increase 2)
//Bit 14           reg_rd_cbus_coef_en       // unsigned ,    RW, default = 0 ,
//1: read coef through cbus enable, just for debug purpose in case when
//we wanna check the coef in ram in correct or not
//Bit 13           reg_vf_sep_coef_en        // unsigned ,    RW, default = 0 ,
//if true, vertical separated coef enable
//Bit 12:10        reserved
//Bit  9           reg_high_reso_en          // unsigned ,    RW, default = 0 ,
//default = 0x0 ,if true, use 9bit resolution coef, other use 8bit resolution coef
//Bit  8: 7        reg_type_index            // unsigned ,    RW, default = 0 ,
//default = 0x0 ,type of index, 00: vertical coef,
//01: vertical chroma coef: 10: horizontal coef, 11: resevered
//Bit  6: 0        reg_coef_index            // unsigned ,    RW, default = 0 ,coef index
#define VDIN0_CNTL_SCALE_COEF                      0x0334
//Bit 31:24        reg_coef0                 // signed ,      RW, default = 0 ,
//coefficients for vertical filter and horizontal	filter
//Bit 23:16        reg_coef1                 // signed ,      RW, default = 0 ,
//coefficients for vertical filter and horizontal	filter
//Bit 15: 8        reg_coef2                 // signed ,      RW, default = 0 ,
//coefficients for vertical filter and horizontal	filter
//Bit  7: 0        reg_coef3                 // signed ,      RW, default = 0 ,
//coefficients for vertical filter and horizontal	filter
#define VDIN0_VSC_CTRL                             0x0336
//Bit 31:28        reg_vsc_gclk_ctrl            // unsigned ,    RW, default = 0
//Bit 27:21        reserved
//Bit 20:16        reg_vsc_skip_line_num        // unsigned ,    RW, default = 0
//Bit 15: 9        reserved
//Bit  8           reg_vsc_phase0_always_en     // unsigned ,    RW, default = 0
//Bit  7: 2        reserved
//Bit  1           reg_vsc_outside_pic_pad_en   // unsigned ,    RW, default = 0
//Bit  0           reg_vsc_rpt_last_ln_mode     // unsigned ,    RW, default = 0
#define VDIN0_VSC_PHASE_STEP                       0x0337
//Bit 31:25        reserved
//Bit 24: 0        reg_vsc_phase_step           // unsigned ,    RW, default = 0
#define VDIN0_VSC_DUMMY_DATA                       0x0338
//Bit 31:24        reserved
//Bit 23: 0        reg_vsc_dummy_data           // unsigned ,    RW, default = 24'h8080
#define VDIN0_VSC_INI_PHASE                        0x0339
//Bit 31:16        reserved
//Bit 15: 0        reg_vsc_ini_phase            // unsigned ,    RW, default = 0
#define VDIN0_HDR2_CTRL                            0x033d
//Bit 31:21        reserved
//Bit 20:18        reg_din_swap           // unsigned , RW, default = 0
//Bit 17           reg_out_fmt            // unsigned , RW, default = 0
//Bit 16           reg_only_mat           // unsigned , RW, default = 0
//Bit 15           reg_mtrxo_en           // unsigned , RW, default = 0
//Bit 14           reg_mtrxi_en           // unsigned , RW, default = 0
//Bit 13           reg_hdr2_top_en        // unsigned , RW, default = 0
//Bit 12           reg_cgain_mode         // unsigned , RW, default = 1
//Bit 11:8         reserved
//Bit  7: 6        reg_gmut_mode          // unsigned , RW, default = 1
//Bit  5           reg_in_shift           // unsigned , RW, default = 0
//Bit  4           reg_in_fmt             // unsigned , RW, default = 1
//Bit  3           reg_eo_enable          // unsigned , RW, default = 1
//Bit  2           reg_oe_enable          // unsigned , RW, default = 1
//Bit  1           reg_ogain_enable       // unsigned , RW, default = 1
//Bit  0           reg_cgain_enable       // unsigned , RW, default = 1
#define VDIN0_HDR2_CLK_GATE                        0x033e
//Bit  31:0        reg_gclk_ctrl          // unsigned , RW, default = 0
#define VDIN0_HDR2_MATRIXI_COEF00_01               0x033f
//Bit 31:29        reserved
//Bit 28:16        reg_mtrxi_coef00       // signed , RW, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_mtrxi_coef01       // signed , RW, default = 0
#define VDIN0_HDR2_MATRIXI_COEF02_10               0x0340
//Bit 31:29        reserved
//Bit 28:16        reg_mtrxi_coef02       // signed , RW, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_mtrxi_coef10       // signed , RW, default = 0
#define VDIN0_HDR2_MATRIXI_COEF11_12               0x0341
//Bit 31:29        reserved
//Bit 28:16        reg_mtrxi_coef11       // signed , RW, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_mtrxi_coef12       // signed , RW, default = 0
#define VDIN0_HDR2_MATRIXI_COEF20_21               0x0342
//Bit 31:29        reserved
//Bit 28:16        reg_mtrxi_coef20       // signed , RW, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_mtrxi_coef21       // signed , RW, default = 0
#define VDIN0_HDR2_MATRIXI_COEF22                  0x0343
//Bit 31:13        reserved
//Bit 12: 0        reg_mtrxi_coef22       // unsigned , RW, default = 0
#define VDIN0_HDR2_MATRIXI_COEF30_31               0x0344
//Bit 31:29        reserved
//Bit 28:16        reg_mtrxi_coef30       // signed , RW, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_mtrxi_coef31       // signed , RW, default = 0
#define VDIN0_HDR2_MATRIXI_COEF32_40               0x0345
//Bit 31:29        reserved
//Bit 28:16        reg_mtrxi_coef32       // signed , RW, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_mtrxi_coef40       // signed , RW, default = 0
#define VDIN0_HDR2_MATRIXI_COEF41_42               0x0346
//Bit 31:29        reserved
//Bit 28:16        reg_mtrxi_coef41       // signed , RW, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_mtrxi_coef42       // signed , RW, default = 0
#define VDIN0_HDR2_MATRIXI_OFFSET0_1               0x0347
//Bit 31:27        reserved
//Bit 26:16        reg_mtrxi_offst_oup0   // signed , RW, default = 0
//Bit 15:11        reserved
//Bit 10: 0        reg_mtrxi_offst_oup1   // signed , RW, default = 0
#define VDIN0_HDR2_MATRIXI_OFFSET2                 0x0348
//Bit 31:11        reserved
//Bit 10: 0        reg_mtrxi_offst_oup2   // signed , RW, default = 0
#define VDIN0_HDR2_MATRIXI_PRE_OFFSET0_1           0x0349
//Bit 31:27        reserved
//Bit 26:16        reg_mtrxi_offst_inp0   // signed , RW, default = 0
//Bit 15:11        reserved
//Bit 10: 0        reg_mtrxi_offst_inp1   // signed , RW, default = 0
#define VDIN0_HDR2_MATRIXI_PRE_OFFSET2             0x034a
//Bit 31:11        reserved
//Bit 10: 0        reg_mtrxi_offst_inp2   // signed , RW, default = 0
#define VDIN0_HDR2_MATRIXO_COEF00_01               0x034b
//Bit 31:29        reserved
//Bit 28:16        reg_mtrxo_coef00       // signed , RW, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_mtrxo_coef01       // signed , RW, default = 0
#define VDIN0_HDR2_MATRIXO_COEF02_10               0x034c
//Bit 31:29        reserved
//Bit 28:16        reg_mtrxo_coef02       // signed , RW, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_mtrxo_coef10       // signed , RW, default = 0
#define VDIN0_HDR2_MATRIXO_COEF11_12               0x034d
//Bit 31:29        reserved
//Bit 28:16        reg_mtrxo_coef11       // signed , RW, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_mtrxo_coef12       // signed , RW, default = 0
#define VDIN0_HDR2_MATRIXO_COEF20_21               0x034e
//Bit 31:29        reserved
//Bit 28:16        reg_mtrxo_coef20       // signed , RW, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_mtrxo_coef21       // signed , RW, default = 0
#define VDIN0_HDR2_MATRIXO_COEF22                  0x034f
//Bit 31:13        reserved
//Bit 12: 0        reg_mtrxo_coef22       // signed , RW, default = 0
#define VDIN0_HDR2_MATRIXO_COEF30_31               0x0350
//Bit 31:29        reserved
//Bit 28:16        reg_mtrxo_coef30       // signed , RW, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_mtrxo_coef31       // signed , RW, default = 0
#define VDIN0_HDR2_MATRIXO_COEF32_40               0x0351
//Bit 31:29        reserved
//Bit 28:16        reg_mtrxo_coef32       // signed , RW, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_mtrxo_coef40       // signed , RW, default = 0
#define VDIN0_HDR2_MATRIXO_COEF41_42               0x0352
//Bit 31:29        reserved
//Bit 28:16        reg_mtrxo_coef41       // signed , RW, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_mtrxo_coef42       // signed , RW, default = 0
#define VDIN0_HDR2_MATRIXO_OFFSET0_1               0x0353
//Bit 31:27        reserved
//Bit 26:16        reg_mtrxo_offst_oup0   // signed , RW, default = 0
//Bit 15:11        reserved
//Bit 10: 0        reg_mtrxo_offst_oup1   // signed , RW, default = 0
#define VDIN0_HDR2_MATRIXO_OFFSET2                 0x0354
//Bit 31:11        reserved
//Bit 10: 0        reg_mtrxo_offst_oup2   // signed , RW, default = 0
#define VDIN0_HDR2_MATRIXO_PRE_OFFSET0_1           0x0355
//Bit 31:27        reserved
//Bit 26:16        reg_mtrxo_offst_inp0   // signed , RW, default = 0
//Bit 15:11        reserved
//Bit 10: 0        reg_mtrxo_offst_inp1   // signed , RW, default = 0
#define VDIN0_HDR2_MATRIXO_PRE_OFFSET2             0x0356
//Bit 31:11        reserved
//Bit 10: 0        reg_mtrxo_offst_inp2   // signed , RW, default = 0
#define VDIN0_HDR2_MATRIXI_CLIP                    0x0357
//Bit 31:20        reserved
//Bit 19:8         reg_mtrxi_comp_thrd    // signed , RW, default = 0
//Bit 7:5          reg_mtrxi_rs           // unsigned , RW, default = 0
//Bit 4:3          reg_mtrxi_clmod        // unsigned , RW, default = 0
//Bit 2:0          reserved
#define VDIN0_HDR2_MATRIXO_CLIP                    0x0358
//Bit 31:20        reserved
//Bit 19:8         reg_mtrxo_comp_thrd    // signed , RW, default = 0
//Bit 7:5          reg_mtrxo_rs           // unsigned , RW, default = 0
//Bit 4:3          reg_mtrxo_clmod        // unsigned , RW, default = 0
//Bit 2:0          reserved
#define VDIN0_HDR2_CGAIN_OFFT                      0x0359
//Bit 31:27        reserved
//Bit 26:16        reg_cgain_oft2         // signed , RW, default = 11'h200
//Bit 15:11        reserved
//Bit 10: 0        reg_cgain_oft1         // signed , RW, default = 11'h200
#define VDIN0_HDR2_CGAIN_COEF0                     0x0361
//Bit 31:28        reserved
//Bit 27:16        c_gain_lim_coef1       // unsigned , RW, default = 12'd2376
//Bit 15:12        reserved
//Bit 11: 0        c_gain_lim_coef0       // unsigned , RW, default = 12'd920
#define VDIN0_HDR2_CGAIN_COEF1                     0x0362
//Bit 31:29        reserved
//Bit 28:16        reg_maxrgb             // unsigned , RW, default = 13'h400
//Bit 15:12        reserved
//Bit 11: 0        c_gain_lim_coef2       // unsigned , RW, default = 12'd208
#define VDIN0_HDR2_ADPS_CTRL                       0x0365
//Bit 31:14        reserved
//Bit 13: 8        reg_adpscl_max         // unsigned , RW, default = 6'd24
//Bit  7           reg_adpscl_clip_en     // unsigned , RW, default = 0
//Bit  6           reg_adpscl_enable2     // unsigned , RW, default = 1
//Bit  5           reg_adpscl_enable1     // unsigned , RW, default = 1
//Bit  4           reg_adpscl_enable0     // unsigned , RW, default = 1
//Bit  3: 2        reserved
//Bit  1: 0        reg_adpscl_mode        // unsigned , RW, default = 1
#define VDIN0_HDR2_ADPS_ALPHA0                     0x0366
//Bit 31:30        reserved
//Bit 29:16        reg_adpscl_alpha1      // unsigned , RW, default = 14'h1000
//Bit 15:14        reserved
//Bit 13: 0        reg_adpscl_alpha0      // unsigned , RW, default = 14'h1000
#define VDIN0_HDR2_ADPS_ALPHA1                     0x0367
//Bit 31:28        reserved
//Bit 27:24        reg_adpscl_shift0      // unsigned , RW, default = 4'hc
//Bit 23:20        reg_adpscl_shift1      // unsigned , RW, default = 4'hc
//Bit 19:16        reg_adpscl_shift2      // unsigned , RW, default = 4'hc
//Bit 15:14        reserved
//Bit 13: 0        reg_adpscl_alpha2      // unsigned , RW, default = 14'h1000
#define VDIN0_HDR2_ADPS_BETA0                      0x0368
//Bit 31:21        reserved
//Bit 20: 0        reg_adpscl_beta0       // unsigned , RW, default = 21'hfc000
#define VDIN0_HDR2_ADPS_BETA1                      0x0369
//Bit 31:21        reserved
//Bit 20: 0        reg_adpscl_beta1       // unsigned , RW, default = 21'hfc000
#define VDIN0_HDR2_ADPS_BETA2                      0x036a
//Bit 31:21        reserved
//Bit 20: 0        reg_adpscl_beta2       // unsigned , RW, default = 21'hfc000
#define VDIN0_HDR2_ADPS_COEF0                      0x036b
//Bit 31:28        reserved
//Bit 27:16        reg_adpscl_ys_coef1    // unsigned , RW, default = 12'd1188
//Bit 15:12        reserved
//Bit 11: 0        reg_adpscl_ys_coef0    // unsigned , RW, default = 12'd460
#define VDIN0_HDR2_ADPS_COEF1                      0x036c
//Bit 31:12        reserved
//Bit 11: 0        reg_adpscl_ys_coef2    // unsigned , RW, default = 12'd104
#define VDIN0_HDR2_GMUT_CTRL                       0x036d
//Bit 31: 5        reserved
//Bit  4           reg_new_mode           // unsigned , RW, default = 0
//Bit  3: 0        reg_gmut_shift         // unsigned , RW, default = 4'd14
#define VDIN0_HDR2_GMUT_COEF0                      0x036e
//Bit 31:16        reg_gmut_coef01        // unsigned , RW, default = 16'd674
//Bit 15: 0        reg_gmut_coef00        // unsigned , RW, default = 16'd1285
#define VDIN0_HDR2_GMUT_COEF1                      0x036f
//Bit 31:16        reg_gmut_coef10        // unsigned , RW, default = 16'd142
//Bit 15: 0        reg_gmut_coef02        // unsigned , RW, default = 16'd89
#define VDIN0_HDR2_GMUT_COEF2                      0x0370
//Bit 31:16        reg_gmut_coef12        // unsigned , RW, default = 16'd23
//Bit 15: 0        reg_gmut_coef11        // unsigned , RW, default = 16'd1883
#define VDIN0_HDR2_GMUT_COEF3                      0x0371
//Bit 31:16        reg_gmut_coef21        // unsigned , RW, default = 16'd180
//Bit 15: 0        reg_gmut_coef20        // unsigned , RW, default = 16'd34
#define VDIN0_HDR2_GMUT_COEF4                      0x0372
//Bit 31:16        reserved
//Bit 15: 0        reg_gmut_coef22        // unsigned , RW, default = 16'd1834
#define VDIN0_EOTF_LUT_ADDR_PORT                   0x035b
//Bit 31: 8        reserved
//Bit  7: 0        reg_eotf_lut_addr      // unsigned , RW, default = 0
#define VDIN0_EOTF_LUT_DATA_PORT                   0x035c
//Bit 31: 0        reg_eotf_lut_data      // unsigned , RW, default = 0
#define VDIN0_OETF_LUT_ADDR_PORT                   0x035d
//Bit 31: 8        reserved
//Bit  7: 0        reg_oetf_lut_addr      // unsigned , RW, default = 0
#define VDIN0_OETF_LUT_DATA_PORT                   0x035e
//Bit 31: 0        reg_oetf_lut_data      // unsigned , RW, default = 0
#define VDIN0_OGAIN_LUT_ADDR_PORT                  0x0363
//Bit 31: 8        reserved
//Bit  7: 0        reg_ogain_lut_addr     // unsigned , RW, default = 0
#define VDIN0_OGAIN_LUT_DATA_PORT                  0x0364
//Bit 31: 0        reg_ogain_lut_data     // unsigned , RW, default = 0
#define VDIN0_CGAIN_LUT_ADDR_PORT                  0x035f
//Bit 31: 8        reserved
//Bit  7: 0        reg_cgain_lut_addr     // unsigned , RW, default = 0
#define VDIN0_CGAIN_LUT_DATA_PORT                  0x0360
//Bit 31: 0        reg_cgain_lut_data     // unsigned , RW, default = 0
#define VDIN0_HDR2_PIPE_CTRL1                      0x0373
//Bit 31:0        reg_pipe_ctrl1          // unsigned , RW, default = 32'h04040a0a
#define VDIN0_HDR2_PIPE_CTRL2                      0x0374
//Bit 31:0        reg_pipe_ctrl2          // unsigned , RW, default = 32'h0c0c0b0b
#define VDIN0_HDR2_PIPE_CTRL3                      0x0375
//Bit 31:0        reg_pipe_ctrl3          // unsigned , RW, default = 32'h16160404
#define VDIN0_HDR2_PROC_WIN1                       0x0376
//Bit 31:0        reg_proc_win1           // unsigned , RW, default = 0
#define VDIN0_HDR2_PROC_WIN2                       0x0377
//Bit 31:0        reg_proc_win2           // unsigned , RW, default = 0
#define VDIN0_HDR2_MATRIXI_EN_CTRL                 0x0378
//Bit 31:8        reserved
//Bit 7:0         reg_matrixi_en_ctrl     // unsigned , RW, default = 0
#define VDIN0_HDR2_MATRIXO_EN_CTRL                 0x0379
//Bit 31:8        reserved
//Bit 7:0         reg_mattrixo_en_ctrl    // unsigned , RW, default = 0
#define VDIN0_HDR2_HIST_CTRL                       0x037a
//Bit 31:25       reserved
//Bit 24:17       reg_vcbus_rd_idx        // unsigned , RW, default = 0
//Bit 16:0        reg_hist_ctrl           // unsigned , RW, default = 17'h01400
#define VDIN0_HDR2_HIST_H_START_END                0x037b
//Bit 31:29       reserved
//Bit 28:0        reg_hist_h_start_end    // unsigned , RW, default = 0
#define VDIN0_HDR2_HIST_V_START_END                0x037c
//Bit 31:29       reserved
//Bit 28:0        reg_hist_v_start_end    // unsigned , RW, default = 0
#define VDIN0_HDR2_HIST_RD                         0x035a
//Bit 31:24       reserved
//Bit 23:0        reg_hist_status         // unsigned , RO, default = 0
#define VDIN0_LFIFO_CTRL                           0x037d
//Bit 31:30        reg_lfifo_gclk_ctrl   // unsigned ,    RW, default = 0
//Bit 29           reg_lfifo_soft_rst_en // unsigned ,    RW, default = 1
//Bit 28:16        reg_lfifo_buf_size    // unsigned ,    RW, default = 2048
//Bit 15: 0        reg_lfifo_urg_ctrl    // unsigned ,    RW, default = 0
#define VDIN0_LFIFO_BUF_COUNT                      0x037e
//Bit 31:13        reserved
//Bit 12: 0        reg_lfifo_buf_count   // unsigned ,    RO, default = 0
#define VDIN0_LUMA_HIST_GCLK_CTRL                  0x0382
//Bit 31: 2        reserved
//Bit  1: 0        reg_luma_hist_gclk_ctrl     // unsigned , RW, default = 0
#define VDIN0_LUMA_HIST_CTRL                       0x0383
//Bit 31:24        reg_luma_hist_pix_white_th  // unsigned , RW, default = 0
//larger than this th is counted as white pixel
//Bit 23:16        reg_luma_hist_pix_black_th  // unsigned , RW, default = 0
//less than this th is counted as black pixel
//Bit 15:12        reserved
//Bit 11           reg_luma_hist_only_34bin    // unsigned , RW, default = 0
//34 bin only mode, including white/black
//Bit 10:9         reg_luma_hist_spl_sft       // unsigned , RW, default = 0
//Bit 8            reserved
//Bit 7:5          reg_luma_hist_dnlp_low      // unsigned , RW, default = 0
//the real pixels in each bins got by VDIN_DNLP_LUMA_HISTXX should multiple with 2^(dnlp_low+3)
//Bit  4: 2        reserved
//Bit  1           reg_luma_hist_win_en        // unsigned , RW, default = 0
//1'b0: hist used for full picture; 1'b1: hist used for pixels within hist window
//Bit  0           reserved
#define VDIN0_LUMA_HIST_H_START_END                0x0384
//Bit 31:29        reserved
//Bit 28:16        reg_luma_hist_hstart        // unsigned , RW, default = 0
//horizontal start value to define hist window
//Bit 15:13        reserved
//Bit 12:0         reg_luma_hist_hend          // unsigned , RW, default = 0
//horizontal end value to define hist window
#define VDIN0_LUMA_HIST_V_START_END                0x0385
//Bit 31:29        reserved
//Bit 28:16        reg_luma_hist_vstart        // unsigned , RW, default = 0
//vertical start value to define hist window
//Bit 15:13        reserved
//Bit 12:0         reg_luma_hist_vend          // unsigned , RW, default = 0
//vertical end value to define hist window
#define VDIN0_LUMA_HIST_DNLP_IDX                   0x0386
//Bit 31: 6        reserved
//Bit  5: 0        reg_luma_hist_dnlp_index    // unsigned , RW, default = 0
#define VDIN0_LUMA_HIST_DNLP_GRP                   0x0387
//Bit 31:0         reg_luma_hist_dnlp_group    // unsigned , RO, default = 0
#define VDIN0_LUMA_HIST_MAX_MIN                    0x0388
//Bit 31:16        reserved
//Bit 15: 8        reg_luma_hist_max           // unsigned , RO, default = 0        maximum value
//Bit  7: 0        reg_luma_hist_min           // unsigned , RO, default = 8'hff    minimum value
#define VDIN0_LUMA_HIST_SPL_VAL                    0x0389
//Bit 31:0         reg_luma_hist_spl_val       // unsigned , RO, default = 0
//counts for the total luma value
#define VDIN0_LUMA_HIST_SPL_CNT                    0x038a
//Bit 31:24        reserved
//Bit 23:0         reg_luma_hist_spl_cnt       // unsigned , RO, default = 0
//counts for the total calculated pixels
#define VDIN0_LUMA_HIST_CHROMA_SUM                 0x038b
//Bit 31:0         reg_luma_hist_chroma_sum    // unsigned , RO, default = 0
//counts for the total chroma value
#define VDIN0_BLKBAR_CTRL                          0x0392
//Bit 31:30   reg_blkbar_gclk_ctrl              // unsigned , RW, default = 0
//Bit 29:28   reg_blkbar_pat_gclk_ctrl          // unsigned , RW, default = 0
//Bit 27:5    reserved
//Bit 4:4     reg_blkbar_soft_rst               // unsigned , RW, default = 0
//Bit 3:0     reg_blkbar_conv_num               // unsigned , RW, default = 9
#define VDIN0_BLKBAR_H_V_WIDTH                     0x0393
//Bit 31:29   reserved
//Bit 28:16   reg_blkbar_hwidth                 // unsigned , RW, default = 512
//Bit 15:13   reserved
//Bit 12:0    reg_blkbar_vwidth                 // unsigned , RW, default = 360
#define VDIN0_BLKBAR_ROW_TOP_BOT                   0x0394
//Bit 31:29   reserved
//Bit 28:16   reg_blkbar_row_top                // unsigned , RW, default = 1035
//Bit 15:13   reserved
//Bit 12:0    reg_blkbar_row_bot                // unsigned , RW, default = 383
#define VDIN0_BLKBAR_TOP_BOT_H_START_END           0x0395
//Bit 31:29   reserved
//Bit 28:16   reg_blkbar_top_bot_hstart         // unsigned , RW, default = 384
//Bit 15:13   reserved
//Bit 12:0    reg_blkbar_top_bot_hend           // unsigned , RW, default = 1536
#define VDIN0_BLKBAR_TOP_BOT_V_START_END           0x0396
//Bit 31:29   reserved
//Bit 28:16   reg_blkbar_top_vstart             // unsigned , RW, default = 0
//Bit 15:13   reserved
//Bit 12:0    reg_blkbar_bot_vend               // unsigned , RW, default = 1079
#define VDIN0_BLKBAR_LEF_RIG_H_START_END           0x0397
//Bit 31:29   reserved
//Bit 28:16   reg_blkbar_left_hstart            // unsigned , RW, default = 512
//Bit 15:13   reserved
//Bit 12:0    reg_blkbar_right_hend             // unsigned , RW, default = 1408
#define VDIN0_BLKBAR_LEF_RIG_V_START_END           0x0398
//Bit 31:29   reserved
//Bit 28:16   reg_blkbar_lef_rig_vstart         // unsigned , RW, default = 216
//Bit 15:13   reserved
//Bit 12:0    reg_blkbar_lef_rig_vend           // unsigned , RW, default = 864
#define VDIN0_BLKBAR_BLK_WHT_LEVEL                 0x0399
//Bit 31:24   reg_blkbar_black_level            // unsigned , RW, default = 16
//Bit 23:21   reserved
//Bit 20:8    reg_blkbar_blk_col_th             // unsigned , RW, default = 576
//Bit 7:0     reg_blkbar_white_level            // unsigned , RW, default = 16
#define VDIN0_BLKBAR_RO_TOP_BOT_POS                0x039a
//Bit 31:29   reserved
//Bit 28:16   reg_blkbar_top_pos                // unsigned , RO, default = 0
//Bit 15:13   reserved
//Bit 12:0    reg_blkbar_bot_pos                // unsigned , RO, default = 0
#define VDIN0_BLKBAR_RO_LEF_RIG_POS                0x039b
//Bit 31:29   reserved
//Bit 28:16   reg_blkbar_left_pos               // unsigned , RO, default = 0
//Bit 15:13   reserved
//Bit 12:0    reg_blkbar_right_pos              // unsigned , RO, default = 0
#define VDIN0_BLKBAR_RO_DEBUG_TOP_BOT_CNT          0x039c
//Bit 31:31   reg_blkbar_pat_bw_flag            // unsigned , RO, default = 0
//Bit 30:29   reserved
//Bit 28:16   reg_blkbar_debug_top_cnt          // unsigned , RO, default = 0
//Bit 15:13   reserved
//Bit 12:0    reg_blkbar_debug_bot_cnt          // unsigned , RO, default = 0
#define VDIN0_BLKBAR_RO_DEBUG_LEF_RIG_CNT          0x039d
//Bit 31:29   reserved
//Bit 28:16   reg_blkbar_debug_lef_cnt          // unsigned , RO, default = 0
//Bit 15:13   reserved
//Bit 12:0    reg_blkbar_debug_rig_cnt          // unsigned , RO, default = 0
#define VDIN0_BLKBAR_PAT_BLK_WHT_TH                0x039e
//Bit 31:29   reserved
//Bit 28:16   reg_blkbar_blk_wht_th             // unsigned , RW, default = 1680
//Bit 15:8    reg_blkbar_black_th               // unsigned , RW, default = 17
//Bit 7:0     reg_blkbar_white_th               // unsigned , RW, default = 236
#define VDIN0_BLKBAR_PAT_H_START_END               0x039f
//Bit 31:31   reg_blkbar_win_en                 // unsigned , RW, default = 0
//Bit 30:29   reserved
//Bit 28:16   reg_blkbar_h_bgn                  // unsigned , RW, default = 0
//Bit 15:13   reserved
//Bit 12:0    reg_blkbar_h_end                  // unsigned , RW, default = 100
#define VDIN0_BLKBAR_PAT_V_START_END               0x03a0
//Bit 31:29   reserved
//Bit 28:16   reg_blkbar_v_bgn                  // unsigned , RW, default = 0
//Bit 15:13   reserved
//Bit 12:0    reg_blkbar_v_end                  // unsigned , RW, default = 10
#define VDIN0_DW_GCLK_CTRL                         0x03b0
//Bit 31:30           reg_top_gclk_ctrl       // unsigned ,    RW, default = 0
//Bit 29:10           reserved
//Bit  9: 8           reg_dsc_gclk_ctrl       // unsigned ,    RW, default = 0
//Bit  7: 6           reg_scb_gclk_ctrl       // unsigned ,    RW, default = 0
//Bit  5: 4           reg_hsk_gclk_ctrl       // unsigned ,    RW, default = 0
//Bit  3: 2           reg_vsk_gclk_ctrl       // unsigned ,    RW, default = 0
//Bit  1: 0           reg_dith_gclk_ctrl      // unsigned ,    RW, default = 0
#define VDIN0_DW_CTRL                              0x03b1
//Bit 31:30           reg_fix_disable         // unsigned ,    RW, default = 0
//Bit 29: 5           reserved
//Bit 4               reg_dsc_en              // unsigned ,    RW, default = 0
//Bit 3               reg_scb_en              // unsigned ,    RW, default = 0
//Bit 2               reg_hsk_en              // unsigned ,    RW, default = 0
//Bit 1               reg_vsk_en              // unsigned ,    RW, default = 0
//Bit 0               reg_dith_en             // unsigned ,    RW, default = 0
#define VDIN0_DW_IN_SIZE                           0x03b2
//Bit 31:29           reserved
//Bit 28:16           reg_in_hsize            // unsigned ,    RW, default = 0
//Bit 15:13           reserved
//Bit 12: 0           reg_in_vsize            // unsigned ,    RW, default = 0
#define VDIN0_DW_OUT_SIZE                          0x03b3
//Bit 31:29           reserved
//Bit 28:16           reg_out_hsize           // unsigned ,    RW, default = 0
//Bit 15:13           reserved
//Bit 12: 0           reg_out_vsize           // unsigned ,    RW, default = 0
#define VDIN0_DSC_CTRL                             0x03b8
//Bit 31:30     reg_dsc_gclk_ctrl             // unsigned ,    RW, default = 0
//Bit 29: 9     reserved
//Bit  8: 3     reg_dsc_dithout_switch        // unsigned ,    RW, default = 36
//Bit  2        reg_dsc_detunnel_en           // unsigned ,    RW, default = 1
//Bit  1        reg_dsc_detunnel_u_start      // unsigned ,    RW, default = 0
//Bit  0        reg_dsc_comp_mode             // unsigned ,    RW, default = 1
#define VDIN0_DSC_CFMT_CTRL                        0x03b9
//Bit 31: 9     reserved
//Bit  8        reg_dsc_chfmt_rpt_pix         // unsigned ,    RW, default = 0
//if true, horizontal formatter use repeating to generate pixel,
//otherwise use bilinear interpolation
//Bit  7: 4     reg_dsc_chfmt_ini_phase       // unsigned ,    RW, default = 0
//horizontal formatter initial phase
//Bit  3        reg_dsc_chfmt_rpt_p0_en       // unsigned ,    RW, default = 0
//horizontal formatter repeat pixel 0 enable
//Bit  2: 1     reg_dsc_chfmt_yc_ratio        // unsigned ,    RW, default = 1
//horizontal Y/C ratio, 00: 1:1, 01: 2:1, 10: 4:1
//Bit  0        reg_dsc_chfmt_en              // unsigned ,    RW, default = 1
//horizontal formatter enable
#define VDIN0_DSC_CFMT_W                           0x03ba
//Bit 31:29     reserved
//Bit 28:16     reg_dsc_chfmt_w               // unsigned ,    RW, default = 1920
//horizontal formatter width
//Bit 15:13     reserved
//Bit 12: 0     reg_dsc_cvfmt_w               // unsigned ,    RW, default = 960
//vertical formatter width
#define VDIN0_DSC_HSIZE                            0x03bb
//Bit 31:29     reserved
//Bit 28:16     reg_dsc_detunnel_hsize        // unsigned ,    RW, default = 1920
//Bit 15: 0     reserved
#define VDIN0_DSC_DETUNNEL_SEL                     0x03bc
//Bit 31:18     reserved
//Bit 17:0      reg_dsc_detunnel_sel          // unsigned ,    RW, default = 34658
#define VDIN0_HSK_CTRL                             0x03c0
//Bit 31:30    reg_hsk_gclk_ctrl    // unsigned,    RW, default = 0
//Bit 29: 7    reserved
//Bit  6: 0    reg_hsk_mode         // unsigned,    RW, default = 4
#define VDIN0_HSK_COEF_0                           0x03c1
//Bit 31: 0    reg_hsk_coef00       // unsigned,    RW, default = 32'h10101010
#define VDIN0_HSK_COEF_1                           0x03c2
//Bit 31: 0    reg_hsk_coef01       // unsigned,    RW, default = 32'h10101010
#define VDIN0_HSK_COEF_2                           0x03c3
//Bit 31: 0    reg_hsk_coef02       // unsigned,    RW, default = 32'h10101010
#define VDIN0_HSK_COEF_3                           0x03c4
//Bit 31: 0    reg_hsk_coef03       // unsigned,    RW, default = 32'h10101010
#define VDIN0_HSK_COEF_4                           0x03c5
//Bit 31: 0    reg_hsk_coef04       // unsigned,    RW, default = 32'h10101010
#define VDIN0_HSK_COEF_5                           0x03c6
//Bit 31: 0    reg_hsk_coef05       // unsigned,    RW, default = 32'h10101010
#define VDIN0_HSK_COEF_6                           0x03c7
//Bit 31: 0    reg_hsk_coef06       // unsigned,    RW, default = 32'h10101010
#define VDIN0_HSK_COEF_7                           0x03c8
//Bit 31: 0    reg_hsk_coef07       // unsigned,    RW, default = 32'h10101010
#define VDIN0_HSK_COEF_8                           0x03c9
//Bit 31: 0    reg_hsk_coef08       // unsigned,    RW, default = 32'h10101010
#define VDIN0_HSK_COEF_9                           0x03ca
//Bit 31: 0    reg_hsk_coef09       // unsigned,    RW, default = 32'h10101010
#define VDIN0_HSK_COEF_10                          0x03cb
//Bit 31: 0    reg_hsk_coef10       // unsigned,    RW, default = 32'h10101010
#define VDIN0_HSK_COEF_11                          0x03cc
//Bit 31: 0    reg_hsk_coef11       // unsigned,    RW, default = 32'h10101010
#define VDIN0_HSK_COEF_12                          0x03cd
//Bit 31: 0    reg_hsk_coef12       // unsigned,    RW, default = 32'h10101010
#define VDIN0_HSK_COEF_13                          0x03ce
//Bit 31: 0    reg_hsk_coef13       // unsigned,    RW, default = 32'h10101010
#define VDIN0_HSK_COEF_14                          0x03cf
//Bit 31: 0    reg_hsk_coef14       // unsigned,    RW, default = 32'h10101010
#define VDIN0_HSK_COEF_15                          0x03d0
//Bit 31: 0    reg_hsk_coef15       // unsigned,    RW, default = 32'h10101010
#define VDIN0_VSK_CTRL                             0x03e0
//Bit 31:28           reg_vsk_gclk_ctrl       // unsigned ,    RW, default = 0
//Bit 27: 4           reserved
//Bit  3: 2           reg_vsk_mode            // unsigned ,    RW, default = 0
//Bit  1              reg_vsk_lpf_mode        // unsigned ,    RW, default = 0
//Bit  0              reg_vsk_fix_en          // unsigned ,    RW, default = 0
#define VDIN0_VSK_PTN_DATA0                        0x03e1
//Bit 31:24           reserved
//Bit 23: 0           reg_vsk_ptn_data0       // unsigned ,    RW, default = 24'h8080
#define VDIN0_VSK_PTN_DATA1                        0x03e2
//Bit 31:24           reserved
//Bit 23: 0           reg_vsk_ptn_data1       // unsigned ,    RW, default = 24'h8080
#define VDIN0_VSK_PTN_DATA2                        0x03e3
//Bit 31:24           reserved
//Bit 23: 0           reg_vsk_ptn_data2       // unsigned ,    RW, default = 24'h8080
#define VDIN0_VSK_PTN_DATA3                        0x03e4
//Bit 31:24           reserved
//Bit 23: 0           reg_vsk_ptn_data3       // unsigned ,    RW, default = 24'h8080
#define VDIN0_SCB_CTRL                             0x03e8
//Bit 31:30     reg_scb_gclk_ctrl           // unsigned , RW, default = 0
//Bit 29: 5     reserved
//Bit  4        reg_scb_444c422_gofield_en  // unsigned , RW, default = 1
//Bit  3: 2     reg_scb_444c422_mode        // unsigned , RW, default = 0  0:left 1:right 2,3:avg
//Bit  1        reg_scb_444c422_bypass      // unsigned , RW, default = 0  1:bypass
//Bit  0        reg_scb_444c422_frmen       // unsigned , RW, default = 0
#define VDIN0_SCB_SIZE                             0x03e9
//Bit 31:29     reserved
//Bit 28:16     reg_scb_444c422_hsize       // unsigned , RW, default = 1920  horizontal size
//Bit 15:13     reserved
//Bit 12: 0     reg_scb_444c422_vsize       // unsigned , RW, default = 960   vertical size
#define VDIN0_SCB_TUNNEL                           0x03ea
//Bit 31:25     reserved
//Bit 24:19     reg_tunnel_outswitch        // unsigned , RW, default = 36
//Bit 18: 1     reg_tunnel_sel              // unsigned , RW, default = 18'h110ec
//Bit  0        reg_tunnel_en               // unsigned , RW, default = 1
#define VDIN0_DITH1_CTRL                           0x03eb
//Bit 31:30     reg_dith_gclk_ctrl  // unsigned ,    RW, default = 2'h0
//Bit 29:20     reserved
//Bit 19:16     reg_dith22_cntl     // unsigned ,    RW, default = 4'h0
//Bit 15:14     reserved
//Bit 13:4      reg_dith44_cntl     // unsigned ,    RW, default = 10'h3b4
//Bit 3         reserved
//Bit 2         reg_dith_md         // unsigned ,    RW, default = 1'h0
//Bit 1         reg_round_en        // unsigned ,    RW, default = 1'h0
//Bit 0         reserved
#define VDIN0_WRMIF_CTRL                           0x03f0
//Bit 31      reg_swap_word                 // unsigned ,    RW, default = 1
//applicable only to reg_wr_format=0/2, 0: Output every even pixels' CbCr;
//1: Output every odd pixels' CbCr;2: Output an average value per even&odd pair of pixels;
//3: Output all CbCr. (This does NOT apply to bit[13:12]=0 -- 4:2:2 mode.  // RW
//Bit 30      reg_swap_cbcr                 // unsigned ,    RW, default = 0
//applicable only to reg_wr_format=2, 0: Output CbCr (NV12); 1: Output CrCb (NV21).
//Bit 29:28   reg_vconv_mode                // unsigned ,    RW, default = 0
//applicable only to reg_wr_format=2, 0: Output every even lines' CbCr;
//1: Output every odd lines' CbCr;2: Reserved;3: Output all CbCr.
//Bit 27:26   reg_hconv_mode                // unsigned ,    RW, default = 0
//Bit 25      reg_no_clk_gate               // unsigned ,    RW, default = 0
//disable vid_wr_mif clock gating function.
//Bit 24      reg_clr_wrrsp                 // unsigned ,    RW, default = 0
//Bit 23      reg_eol_sel                   // unsigned ,    RW, default = 1
//eol_sel, 1: use eol as the line end indication,
//0: use width as line end indication in the vdin write memory interface
//Bit 22:20   reserved
//Bit 19      reg_frame_rst_en              // unsigned ,    RW, default = 1
//frame reset enable, if true, it will provide frame reset during
//go_field(vsync) to the modules after that
//Bit 18      reg_field_done_clr_bit        // unsigned ,    RW, default = 0
//write done status clear bit
//Bit 17      reg_pending_ddr_wrrsp_clr_bit // unsigned ,    RW, default = 0
//clear write response counter in the vdin write memory interface
//Bit 16:15   reserved
//Bit 14      reg_dbg_sample_mode           // unsigned ,    RW, default = 0
//Bit 13:12   reg_wr_format                 // unsigned ,    RW, default = 0
//write format, 0: 4:2:2 to luma canvas, 1: 4:4:4 to luma canvas,
//2: Y to luma canvas, CbCr to chroma canvas.
//For NV12/21, also define Bit 31:30, 17:16, and bit 18.
//Bit 11      reg_wr_canvas_dbuf_en         // unsigned ,    RW, default = 0
//write canvas double buffer enable, means the canvas address will be latched
//by vsync before using
//Bit 10      reg_dis_ctrl_reg_w_pulse       // unsigned ,    RW, default = 0
//disable ctrl_reg write pulse which will reset internal counter. when bit 11 is 1,
//this bit should be 1.
//Bit  9      reg_wr_req_urgent             // unsigned ,    RW, default = 0
//write request urgent
//Bit  8      reg_wr_req_en                 // unsigned ,    RW, default = 0
//write request enable
//Bit  7: 0   reg_wr_canvas_direct_luma     // unsigned ,    RW, default = 0
//Write luma canvas address
#define VDIN0_WRMIF_CTRL2                          0x03f1
//Bit 31      reg_debug_mode                // unsigned ,    RW, default = 0
//Bit 30:26   reserved
//Bit 25      reg_wr_little_endian          // unsigned ,    RW, default = 0
//Bit 24:22   reg_wr_field_mode             // unsigned ,    RW, default = 0
//0 frame mode, 4 for field mode botton field, 5 for field mode top field, , 6 for blank line mode
//Bit 21      reg_wr_h_rev                  // unsigned ,    RW, default = 0
//Bit 20      reg_wr_v_rev                  // unsigned ,    RW, default = 0
//Bit 19      reg_wr_bit10_mode             // unsigned ,    RW, default = 0
//Bit 18      reg_wr_data_ext_en            // unsigned ,    RW, default = 0
//Bit 17:14   reg_wr_words_lim              // unsigned ,    RW, default = 1
//Bit 13:10   reg_wr_burst_lim              // unsigned ,    RW, default = 2
//Bit  9: 8   reserved
//Bit  7: 0   reg_wr_canvas_direct_chroma   // unsigned ,    RW, default = 0
//Write chroma canvas address
#define VDIN0_WRMIF_H_START_END                    0x03f2
//Bit 31:29   reserved
//Bit 28:16   reg_wr_h_start                // unsigned ,    RW, default = 0
//Bit 15:13   reserved
//Bit 12: 0   reg_wr_h_end                  // unsigned ,    RW, default = 13'h1fff
#define VDIN0_WRMIF_V_START_END                    0x03f3
//Bit 31:29   reserved
//Bit 28:16   reg_wr_v_start                // unsigned ,    RW, default = 0
//Bit 15:13   reserved
//Bit 12: 0   reg_wr_v_end                  // unsigned ,    RW, default = 13'h1fff
#define VDIN0_WRMIF_URGENT_CTRL                    0x03f4
//Bit 31:16   reserved
//Bit 15: 0   reg_wr_req_urgent_ctrl        // unsigned ,    RW, default = 0
#define VDIN0_WRMIF_BADDR_LUMA                     0x03f5
//Bit 31: 0   reg_canvas_baddr_luma         // unsigned ,    RW, default = 0
#define VDIN0_WRMIF_BADDR_CHROMA                   0x03f6
//Bit 31: 0   reg_canvas_baddr_chroma       // unsigned ,    RW, default = 0
#define VDIN0_WRMIF_STRIDE_LUMA                    0x03f7
//Bit 31: 0   reg_canvas_stride_luma        // unsigned ,    RW, default = 0
#define VDIN0_WRMIF_STRIDE_CHROMA                  0x03f8
//Bit 31: 0   reg_canvas_stride_chroma      // unsigned ,    RW, default = 0
#define VDIN0_WRMIF_DSC_CTRL                       0x03f9
//Bit 31:19   reserved
//Bit 18: 0   reg_descramble_ctrl           // unsigned ,    RW, default = 0
#define VDIN0_WRMIF_CTRL3                          0x03fa
//Bit 31: 0   reg_wrmif_ctrl3               // unsigned ,    RW, default = 0
#define VDIN0_WRMIF_DBG_AXI_CMD_CNT                0x03fb
//Bit 31: 0   reg_dbg_axi_cmd_cnt           // unsigned ,    RO, default = 0
#define VDIN0_WRMIF_DBG_AXI_DAT_CNT                0x03fc
//Bit 31: 0   reg_dbg_axi_dat_cnt           // unsigned ,    RO, default = 0
#define VDIN0_WRMIF_RO_STATUS                      0x03fd
//Bit 31: 2   reserved
//Bit  1      reg_pending_ddr_wrrsp         // unsigned ,    RO, default = 0
//Bit  0      reg_field_done                // unsigned ,    RO, default = 0

/* t3x new added registers bank 1 end */

#endif /* __VDIN_S5_REGS_H */
