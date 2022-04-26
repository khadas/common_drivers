/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __LCD_REG_TEMP_H__
#define __LCD_REG_TEMP_H__

#define LCD_REG_OFFSET(reg)                   (((reg) << 2))
#define LCD_REG_OFFSET_BYTE(reg)              ((reg))

#define PADCTRL_PIN_MUX_REG0                       0x0000
#define PADCTRL_PIN_MUX_REG1                       0x0001
#define PADCTRL_PIN_MUX_REG3                       0x0003
#define PADCTRL_PIN_MUX_REG4                       0x0004
#define PADCTRL_PIN_MUX_REG9                       0x0009
#define PADCTRL_PIN_MUX_REGB                       0x000b
#define PADCTRL_PIN_MUX_REGG                       0x0010
#define PADCTRL_PIN_MUX_REGI                       0x0012
#define PADCTRL_PIN_MUX_REGJ                       0x0013
#define PADCTRL_PIN_MUX_REGK                       0x0014

/* HIU:  HHI_CBUS_BASE = 0x10 */
//#define HHI_VIID_CLK_DIV                           0x4a
    #define DAC0_CLK_SEL           28
    #define DAC1_CLK_SEL           24
    #define DAC2_CLK_SEL           20
    #define VCLK2_XD_RST           17
    #define VCLK2_XD_EN            16
    #define ENCL_CLK_SEL           12
    #define VCLK2_XD                0
//#define HHI_VIID_CLK_CNTL                          0x4b
    #define VCLK2_EN               19
    #define VCLK2_CLK_IN_SEL       16
    #define VCLK2_SOFT_RST         15
    #define VCLK2_DIV12_EN          4
    #define VCLK2_DIV6_EN           3
    #define VCLK2_DIV4_EN           2
    #define VCLK2_DIV2_EN           1
    #define VCLK2_DIV1_EN           0
//#define HHI_VIID_DIVIDER_CNTL                      0x4c
    #define DIV_CLK_IN_EN          16
    #define DIV_CLK_SEL            15
    #define DIV_POST_TCNT          12
    #define DIV_LVDS_CLK_EN        11
    #define DIV_LVDS_DIV2          10
    #define DIV_POST_SEL            8
    #define DIV_POST_SOFT_RST       7
    #define DIV_PRE_SEL             4
    #define DIV_PRE_SOFT_RST        3
    #define DIV_POST_RST            1
    #define DIV_PRE_RST             0
//#define HHI_VID_CLK_DIV                            0x59
    #define ENCI_CLK_SEL           28
    #define ENCP_CLK_SEL           24
    #define ENCT_CLK_SEL           20
    #define VCLK_XD_RST            17
    #define VCLK_XD_EN             16
    #define ENCL_CLK_SEL           12
    #define VCLK_XD1                8
    #define VCLK_XD0                0
//#define HHI_VID_CLK_CNTL2_T5W                      0xa4
    #define HDMI_TX_PIXEL_GATE_VCLK  5
    #define VDAC_GATE_VCLK           4
    #define ENCL_GATE_VCLK           3
    #define ENCP_GATE_VCLK           2
    #define ENCT_GATE_VCLK           1
    #define ENCI_GATE_VCLK           0

#define CLKCTRL_VID_CLK_CTRL                       0x0030
#define CLKCTRL_VID_CLK_CTRL2                      0x0031
#define CLKCTRL_VID_CLK_DIV                        0x0032
#define CLKCTRL_VIID_CLK_DIV                       0x0033
#define CLKCTRL_VIID_CLK_CTRL                      0x0034
#define CLKCTRL_HDMI_CLK_CTRL                      0x0038
#define CLKCTRL_VID_PLL_CLK_DIV                    0x0039
#define CLKCTRL_VPU_CLK_CTRL                       0x003a
#define CLKCTRL_VPU_CLKB_CTRL                      0x003b
#define CLKCTRL_VPU_CLKC_CTRL                      0x003c
#define CLKCTRL_VDIN_MEAS_CLK_CTRL                 0x003e
#define CLKCTRL_VAPBCLK_CTRL                       0x003f
#define CLKCTRL_MIPIDSI_PHY_CLK_CTRL               0x0041
#define CLKCTRL_VOUTENC_CLK_CTRL                   0x0046

#define ANACTRL_GP1PLL_CTRL0                       0x0030
#define ANACTRL_GP1PLL_CTRL1                       0x0031
#define ANACTRL_GP1PLL_CTRL2                       0x0032
#define ANACTRL_GP1PLL_CTRL3                       0x0033
#define ANACTRL_GP1PLL_CTRL4                       0x0034
#define ANACTRL_GP1PLL_CTRL5                       0x0035
#define ANACTRL_GP1PLL_CTRL6                       0x0036
#define ANACTRL_GP1PLL_STS                         0x0037

#define ANACTRL_MIPIDSI_CTRL0                      0x00a0
#define ANACTRL_MIPIDSI_CTRL1                      0x00a1
#define ANACTRL_MIPIDSI_CTRL2                      0x00a2
#define ANACTRL_MIPIDSI_STS                        0x00a3

/*  Global control:  RESET_CBUS_BASE = 0x11 */
#define VERSION_CTRL                               0x1100
#define RESET0_REGISTER                            0x1101
#define RESET1_REGISTER                            0x1102
#define RESET2_REGISTER                            0x1103
#define RESET3_REGISTER                            0x1104
#define RESET4_REGISTER                            0x1105
#define RESET5_REGISTER                            0x1106
#define RESET6_REGISTER                            0x1107
#define RESET7_REGISTER                            0x1108
#define RESET0_MASK                                0x1110
#define RESET1_MASK                                0x1111
#define RESET2_MASK                                0x1112
#define RESET3_MASK                                0x1113
#define RESET4_MASK                                0x1114
#define RESET5_MASK                                0x1115
#define RESET6_MASK                                0x1116
#define CRT_MASK                                   0x1117
#define RESET7_MASK                                0x1118

/* t5 */
#define RESET0_MASK_T5                             0x0010
#define RESET1_MASK_T5                             0x0011
#define RESET2_MASK_T5                             0x0012
#define RESET3_MASK_T5                             0x0013
#define RESET4_MASK_T5                             0x0014
#define RESET5_MASK_T5                             0x0015
#define RESET6_MASK_T5                             0x0016
#define RESET7_MASK_T5                             0x0017
#define RESET0_LEVEL_T5                            0x0020
#define RESET1_LEVEL_T5                            0x0021
#define RESET2_LEVEL_T5                            0x0022
#define RESET3_LEVEL_T5                            0x0023
#define RESET4_LEVEL_T5                            0x0024
#define RESET5_LEVEL_T5                            0x0025
#define RESET6_LEVEL_T5                            0x0026
#define RESET7_LEVEL_T5                            0x0027

#define RESETCTRL_RESET0                           0x0000
#define RESETCTRL_RESET1                           0x0001
#define RESETCTRL_RESET2                           0x0002
#define RESETCTRL_RESET3                           0x0003
#define RESETCTRL_RESET4                           0x0004
#define RESETCTRL_RESET5                           0x0005
#define RESETCTRL_RESET6                           0x0006
#define RESETCTRL_RESET0_LEVEL                     0x0010
#define RESETCTRL_RESET1_LEVEL                     0x0011
#define RESETCTRL_RESET2_LEVEL                     0x0012
#define RESETCTRL_RESET3_LEVEL                     0x0013
#define RESETCTRL_RESET4_LEVEL                     0x0014
#define RESETCTRL_RESET5_LEVEL                     0x0015
#define RESETCTRL_RESET6_LEVEL                     0x0016
#define RESETCTRL_RESET0_MASK                      0x0020
#define RESETCTRL_RESET1_MASK                      0x0021
#define RESETCTRL_RESET2_MASK                      0x0022
#define RESETCTRL_RESET3_MASK                      0x0023
#define RESETCTRL_RESET4_MASK                      0x0024
#define RESETCTRL_RESET5_MASK                      0x0025
#define RESETCTRL_RESET6_MASK                      0x0026

/* ********************************
 * TCON:  VCBUS_BASE = 0x14
 */
#define VPU_VOUT_TOP_CTRL                          0x0000
//Bit 31:16  reg_gclk_ctrl              //unsigned,   RW,   default = 16'h0; todo  16bit or 10bit
//Bit 15: 0  reg_sw_resets              //unsigned,   RW,   default = 16'h0;
#define VPU_VOUT_SECURE_BIT_NOR                    0x0001
//Bit 31: 0  reg_secure_bits_nor        //unsigned,   RW,   default = 32'h0;
#define VPU_VOUT_SECURE_DATA                       0x0002
//Bit 31:30  reserved
//Bit 29: 0  reg_secure_data             //unsigned,   RW,   default = 30'h0;
#define VPU_VOUT_FRM_CTRL                          0x0003
//Bit 31:17  reserved
//Bit    16  reg_dsi_suspend_en          //unsigned,   RW,   default = 0;
//Bit    15  reg_scan_en                 //unsigned,   RW,   default = 0;
//Bit 14: 2  reg_hold_line_num           //unsigned,   RW,   default = 4;
//Bit     1  pls_frm_start
//Bit     0  reg_frm_start_sel           //unsigned,   RW,   default = 0;
#define VPU_VOUT_AXI_ARBIT                         0x0004
//Bit 31:15  reserved
//Bit 14:13  reg_arb_urg_sel             //unsigned,   RW,   default = 2;
//Bit    12  reg_am_prot                 //unsigned,   RW,   default = 1;
//Bit 11: 4  reg_arb_arqos               //unsigned,   RW,   default = 0;
//Bit  3: 0  reg_arb_arcache             //unsigned,   RW,   default = 0;
#define VPU_VOUT_RDARB_IDMAP0                      0x0005
//Bit  31:0  reg_rdarb_id_map0           //unsigned,   RW,   default = 32'h10101010 ;
#define VPU_VOUT_RDARB_IDMAP1                      0x0006
//Bit  31:0  reg_rdarb_id_map1           //unsigned,   RW,   default = 32'h10101010 ;
#define VPU_VOUT_IRQ_CTRL                          0x0007
//Bit     31  reserved
//Bit  30:16  reg_irq_line_num           //unsigned,   RW,   default = 200;
//Bit  15: 8  reg_irq_en                 //unsigned,   RW,   default = 1;
//Bit   7: 0  pls_irq_clr                //unsigned,   RW,   default = 0;
#define VPU_VOUT_VLK_CTRL                          0x0009
//Bit 31: 12 reserved
//Bit 11: 8  reg_vlock_vsrc_sel          //unsigned,   RW,   default = 0;
//Bit  7: 4  reg_vlock_lath_sel          //unsigned,   RW,   default = 1;
//Bit  3: 1  reserved
//Bit     0  reg_vlock_en                //unsigned,   RW,   default = 0;
//blend module
#define VPU_VOUT_BLEND_CTRL                        0x0010
//Bit 31:28  reserved
//Bit 27:26  reg_blend_use_latch        //unsigned,   RW,   default = 2'h0;
//Bit    25  reg_blend_bg_en            //unsigned,   RW,   default = 1'h0;
    //0:vd1 is  background   1:osd is background
//Bit 24:16  reg_blend0_dummy_alpha     //unsigned,   RW,   default = 9'h0;
//Bit 15:13  reserved
//Bit 12: 4  reg_vd1_alpha              //unsigned,   RW,   default = 9'h0;
//Bit  3: 2  reg_blend0_premult_en      //unsigned,   RW,   default = 2'h1;
//Bit  1: 0  reg_blend_din_en           //unsigned,   RW,   default = 2'h3;
#define VPU_VOUT_BLEND_DUMDATA                     0x0011
//Bit 31:30  reserved
//Bit 29: 0  reg_blend0_dummy_data      //unsigned,   RW,   default = 30'h0;
#define VPU_VOUT_BLEND_SIZE                        0x0012
//Bit 31:29  reserved
//Bit 28:16  reg_blend_hsize            //unsigned,   RW,   default = 720;
//Bit 15:13  reserved
//Bit 12:0   reg_blend_vsize            //unsigned,   RW,   default = 480;
//vd1 module
#define VPU_VOUT_BLD_SRC0_HPOS                     0x0020
//Bit 31:29  reserved
//Bit 28:16  reg_bld_src0_h_end          //unsigned,   RW,   default = 719;
//Bit 15:13  reserved
//Bit 12: 0  reg_bld_src0_h_start        //unsigned,   RW,   default = b0;
#define VPU_VOUT_BLD_SRC0_VPOS                     0x0021
//Bit 31:29  reserved
//Bit 28:16  reg_bld_src0_v_end          //unsigned,   RW,   default = 479;
//Bit 15:13  reserved
//Bit 12: 0  reg_bld_src0_v_start        //unsigned,   RW,   default = 0;
//osd1 module
#define VPU_VOUT_BLD_SRC1_HPOS                     0x0030
//Bit 31:29  reserved
//Bit 28:16  reg_bld_src1_h_end          //unsigned,   RW,   default = 719;
//Bit 15:13  reserved
//Bit 12: 0  reg_bld_src1_h_start        //unsigned,   RW,   default = 0;
#define VPU_VOUT_BLD_SRC1_VPOS                     0x0031
//Bit 31:29  reserved
//Bit 28:16  reg_bld_src1_v_end          //unsigned,   RW,   default = 479;
//Bit 15:13  reserved
//Bit 12: 0  reg_bld_src1_v_start        //unsigned,   RW,   default = 0;
//ofifo
#define VPU_VOUT_OFIFO_SIZE                        0x0040
//Bit 31:29  reserved
//Bit 28:16  reg_ofifo_line_lenm1        //unsigned,   RW,   default = 480;
//Bit 15:14  reserved
//Bit 13: 0  reg_ofifo_size              //unsigned,   RW,   default = 2048;
#define VPU_VOUT_OFIFO_URG_CTRL                    0x0041
//Bit  31:30  reserved
//Bit     29  reg_ofifo_urg_hold_en      //unsigned,   RW,   default = 1'h0;
//Bit  28:16  reg_ofifo_urg_hold_line_th //unsigned,   RW,   default = 13'h0;
//Bit  15: 0  reg_ofifo_urg_ctrl         //unsigned,   RW,   default = 16'h0;
//for ro
#define VPU_VOUT_RO_STATUS                         0x0050
//Bit 31:23  reserved
//Bit 22:15  ro_irq_status
//Bit 14: 2  ro_ofifo_buf_count
//Bit     1  ro_osd_sc_blki_done
//Bit     0  ro_osd1_nearfull            //unsigned  ro
#define VPU_VOUT_RO_BLD_CURXY                      0x0051
//Bit 31:0  ro_blend0_current_xy         //unsigned  ro
#define VPU_VOUT_RO_VLK_ISP_TCNT                   0x0052
//Bit 31:0  ro_vlock_isp_tim_cnt         //unsigned  ro
#define VPU_VOUT_RO_VLK_VOUT_TCNT                  0x0053
//Bit 31:0  ro_vlock_vout_tim_cnt        //unsigned  ro
#define VPU_VOUT_RO_VLK_HIG_TCNT                   0x0054
//Bit 31:0  ro_vlock_tim_cnt_h           //unsigned  ro
#define VPU_VOUT_RO_VLK_FRM_CNT                    0x0055
//Bit 31:16  ro_vlock_isp_frm_cnt       //unsigned  ro
//Bit 15 :0  ro_vlock_vout_frm_cnt      //unsigned  ro
#define VPU_VOUT_RO_VLK_LATCH_TCNT                 0x0056
//Bit 31:0  ro_vlock_lth_tim_cnt_l      //unsigned  ro
#define VPU_VOUT_RO_VLK_LATCH_FCNT                 0x0057
//Bit 31:16  ro_vlock_lth_tim_cnt_h     //unsigned  ro
//Bit 15 :0  ro_vlock_lth_frm_cnt       //unsigned  ro
//for hw secure only
#define VPU_VOUT_TOP_SEC_KP                        0x0070
#define VPU_VOUT_TOP_SEC_BIT                       0x0071
#define VPU_VOUT_TOP_SEC_RO                        0x0072
//
// Closing file:  ./vpu_inc/vpu_vout_top_reg.h
//
// -----------------------------------------------
// REG_BASE:  VOUT_TIMGEN_VCBUS_BASE = 0x01
// -----------------------------------------------
//
// Reading file:  ./vpu_inc/vpu_vout_timgen_reg.h
//
#define VPU_VOUT_CORE_CTRL                         0x0100
//Bit    31 reg_lath_size               // unsigned,  RW,  default = 0,
//Bit 30:29 reg_dth_glk_ctrl            // unsigned,  RW,  default = 0,
//Bit    28 reg_out_ctrl                // unsigned,  RW,  default = 0,
//Bit    27 reg_pos_switch              // unsigned,  RW,  default = 0,
    //0:lcd_vout[23:0]=lcd_out[23:0]   1:lcd_out[15:8] exchange with lcd_out[7:0]
//Bit    26 reg_bt1120_simp_tim         // unsigned,  RW,  default = 0,
    // 0:BT1120 with crc/ln0/ln1   1:BT1120 without crc/ln0/ln1
//Bit 25:16 reg_out_mode                // unsigned,  RW,  default = 0,
//Bit 15:12 reg_gclk_mode_ctrl          // unsigned,  RW,  default = 0,
    //[3:2] clk_1120_ctrl [1:0]clk_656_ctrl,
//Bit 11:10 reg_gclk_regs_ctrl          // unsigned,  RW,  default = 0,
    //[1:0] clk_reg
//Bit  9: 4 reg_dat_reo_sel             // unsigned,  RW,  default = 6'h24 //output data reorder
//Bit  3: 2 reg_serial_rate             // unsigned,  RW,  default = 0,
    //0:pix/1cylce    1:pix/2cycle  2:pix/3cycle
//Bit 1     reg_field_mode              // unsigned,  RW,  default = 0;
    //0:progressive   1:interlaced
//Bit 0     reg_venc_en                 // unsigned,  RW,  default = 0;
    //0:venc disable  1:venc enable
#define VPU_VOUT_INT_CTRL                          0x0101
//Bit 31:15 reserved
//Bit    14 reg_dth_en                  // unsigned,  RW,  default = 0
//Bit 13:12 reg_sw_rst_sel              // unsigned,  RW,  default = 0
//Bit 11:8  reg_tim_rev                 // unsigned,  RW,  default = 0
//Bit 7     reg_int_error_ctrl          // unsigned,  RW,  default = 0
//Bit 6     reg_int_disable_rst_afifo   // unsigned,  RW,  default = 0
//Bit 5     pls_int_force_go_field      // unsigned,  RW,  default = 0
//Bit 4     pls_int_force_go_line       // unsigned,  RW,  default = 0
//Bit 3:2   reg_int_force_field_ctrl    // unsigned,  RW,  default = 0
//Bit 1:0   reg_int_vs_hs_ctrl          // unsigned,  RW,  default = 0
//
//
#define VPU_VOUT_DETH_CTRL                         0x0102
//Bit 31:19  reg_dth_vsize               // unsigned,  RW,  default = 0
//Bit 18:6   reg_dth_hsize               // unsigned,  RW,  default = 0
//Bit    5   reg_dth_bw                  // unsigned,  RW,  default = 0
//Bit  4:2   reg_dth_force_cnt_val       // unsigned,  RW,  default = 0
//Bit    1   reg_dth_force_cnt_en        // unsigned,  RW,  default = 0
//Bit    0   reg_sw_rst                  // unsigned,  RW,  default = 0
#define VPU_VOUT_DTH_DATA                          0x0103
//Bit 31:0   reg_dth_data                      // unsigned ,    RW, default = 32'h0
#define VPU_VOUT_DTH_ADDR                          0x0104
//Bit 31:5   reserved
//Bit 4 :0   reg_dth_addr                      // unsigned ,    RW, default = 5'h0/
#define VPU_VOUT_HS_POS                            0x0112
//Bit 31:29 reserved
//Bit 28:16 reg_hs_px_bgn    // unsigned,  RW,  default = 0;  pixel begin
//Bit 15:13 reserved
//Bit 12: 0 reg_hs_px_end    // unsigned,  RW,  default = 1;  pixel end
#define VPU_VOUT_VSLN_E_POS                        0x0113
//Bit 31:29 reserved
//Bit 28:16 reg_vs_ln_bgn_e  // unsigned,  RW,  default = 0;  (even field) line begin
//Bit 15:13 reserved
//Bit 12: 0 reg_vs_ln_end_e  // unsigned,  RW,  default = 0;  (even field) line end
#define VPU_VOUT_VSPX_E_POS                        0x0114
//Bit 31:29 reserved
//Bit 28:16 reg_vs_px_bgn_e  // unsigned,  RW,  default = 0;  (even field) pixel begin
//Bit 15:13 reserved
//Bit 12: 0 reg_vs_px_end_e  // unsigned,  RW,  default = 0;  (even field) pixel end
#define VPU_VOUT_VSLN_O_POS                        0x0115
//Bit 31:29 reserved
//Bit 28:16 reg_vs_ln_bgn_o  // unsigned,  RW,  default = 0;  (odd  field) line begin
//Bit 15:13 reserved
//Bit 12: 0 reg_vs_ln_end_o  // unsigned,  RW,  default = 0;  (odd  field) line end
#define VPU_VOUT_VSPX_O_POS                        0x0116
//Bit 31:29 reserved
//Bit 28:16 reg_vs_px_bgn_o  // unsigned,  RW,  default = 0;  (odd  field) pixel begin
//Bit 15:13 reserved
//Bit 12: 0 reg_vs_px_end_o  // unsigned,  RW,  default = 0;  (odd  field) pixel end
#define VPU_VOUT_DE_PX_EN                          0x0117
//Bit 31:29 reserved
//Bit 28:16 reg_de_px_bgn    // unsigned,  RW,  default = 100;  (even field) pixel begin
//Bit 15:13 reserved
//Bit 12: 0 reg_de_px_end    // unsigned,  RW,  default = 2019;  (even field) pixel end
#define VPU_VOUT_DELN_E_POS                        0x0118
//Bit 31:29 reserved
//Bit 28:16 reg_de_ln_bgn_e  // unsigned,  RW,  default = 41;  (even field) line begin
//Bit 15:13 reserved
//Bit 12: 0 reg_de_ln_end_e  // unsigned,  RW,  default = 1120;  (even field) line end
#define VPU_VOUT_DELN_O_POS                        0x0119
//Bit 31:29 reserved
//Bit 28:16 reg_de_ln_bgn_o  // unsigned,  RW,  default = 0;  (odd  field) line begin
//Bit 15:13 reserved
//Bit 12: 0 reg_de_ln_end_o  // unsigned,  RW,  default = 0;  (odd  field) line end
#define VPU_VOUT_MAX_SIZE                          0x011a
//Bit 31:29 reserved
//Bit 28:16 reg_total_hsize     // unsigned,  RW,  default = 2200;  maximum pixel count
//Bit 15:13 reserved
//Bit 12: 0 reg_total_vsize     // unsigned,  RW,  default = 1125;  maximum line  count
#define VPU_VOUT_FLD_BGN_LINE                      0x011b
//Bit 31:29 reserved
//Bit 28:16 reg_field_ln_bgn_o
    // unsigned,  RW,  default = 564;  BT1120 60I:1;   BT656_525:4;  odd field begin line
//Bit 15:13 reserved
//Bit 12: 0 reg_field_ln_bgn_e
    // unsigned,  RW,  default = 1;    BT1120 60I:564; BT656_525:268;  even field begin line
//////////////////////////////////////////
//for independent timgen crtl_signal out
#define VPU_VOUTO_HS_POS                           0x0120
//Bit 31:29 reserved
//Bit 28:16 reg_venco_hs_px_bgn    // unsigned,  RW,  default = 0;  pixel begin
//Bit 15:13 reserved
//Bit 12: 0 reg_venco_hs_px_end    // unsigned,  RW,  default = 1;  pixel end
#define VPU_VOUTO_VSLN_E_POS                       0x0121
//Bit 31:29 reserved
//Bit 28:16 reg_venco_vs_ln_bgn_e  // unsigned,  RW,  default = 0;  (even field) line begin
//Bit 15:13 reserved
//Bit 12: 0 reg_venco_vs_ln_end_e  // unsigned,  RW,  default = 0;  (even field) line end
#define VPU_VOUTO_VSPX_E_POS                       0x0122
//Bit 31:29 reserved
//Bit 28:16 reg_venco_vs_px_bgn_e  // unsigned,  RW,  default = 0;  (even field) pixel begin
//Bit 15:13 reserved
//Bit 12: 0 reg_venco_vs_px_end_e  // unsigned,  RW,  default = 0;  (even field) pixel end
#define VPU_VOUTO_VSLN_O_POS                       0x0123
//Bit 31:29 reserved
//Bit 28:16 reg_venco_vs_ln_bgn_o  // unsigned,  RW,  default = 0;  (odd  field) line begin
//Bit 15:13 reserved
//Bit 12: 0 reg_venco_vs_ln_end_o  // unsigned,  RW,  default = 0;  (odd  field) line end
#define VPU_VOUTO_VSPX_O_POS                       0x0124
//Bit 31:29 reserved
//Bit 28:16 reg_venco_vs_px_bgn_o  // unsigned,  RW,  default = 0;  (odd  field) pixel begin
//Bit 15:13 reserved
//Bit 12: 0 reg_venco_vs_px_end_o  // unsigned,  RW,  default = 0;  (odd  field) pixel end
#define VPU_VOUTO_DE_PX_EN                         0x0125
//Bit 31:29 reserved
//Bit 28:16 reg_venco_de_px_bgn    // unsigned,  RW,  default = 100;  (even field) pixel begin
//Bit 15:13 reserved
//Bit 12: 0 reg_venco_de_px_end    // unsigned,  RW,  default = 2019;  (even field) pixel end
#define VPU_VOUTO_DELN_E_POS                       0x0126
//Bit 31:29 reserved
//Bit 28:16 reg_venco_de_ln_bgn_e  // unsigned,  RW,  default = 41;  (even field) line begin
//Bit 15:13 reserved
//Bit 12: 0 reg_venco_de_ln_end_e  // unsigned,  RW,  default = 1120;  (even field) line end
#define VPU_VOUTO_DELN_O_POS                       0x0127
//Bit 31:29 reserved
//Bit 28:16 reg_venco_de_ln_bgn_o  // unsigned,  RW,  default = 0;  (odd  field) line begin
//Bit 15:13 reserved
//Bit 12: 0 reg_venco_de_ln_end_o  // unsigned,  RW,  default = 0;  (odd  field) line end
#define VPU_VOUTO_MAX_SIZE                         0x0128
//Bit 31:29 reserved
//Bit 28:16 reg_venco_total_hsize     // unsigned,  RW,  default = 2200;  maximum pixel count
//Bit 15:13 reserved
//Bit 12: 0 reg_venco_total_vsize     // unsigned,  RW,  default = 1125;  maximum line  count
#define VPU_VOUTO_FLD_BGN_LINE                     0x0129
//Bit 31:29 reserved
//Bit 28:16 reg_venco_field_ln_bgn_o
    // unsigned,  RW,  default = 564;  BT1120 60I:1;   BT656_525:4;  odd field begin line
//Bit 15:13 reserved
//Bit 12: 0 reg_venco_field_ln_bgn_e
    // unsigned,  RW,  default = 1;    BT1120 60I:564; BT656_525:268;  even field begin line
// bt1120
#define VPU_VOUT_BT_CTRL                           0x0130
//Bit 31:13  reserved
//Bit 12     reg_field_en       // unsigned,  RW,  default = 0;   //for interlace output
//Bit 11:4   reserved
//Bit 3: 2   reg_422_mode       // unsigned,  RW,  default = 0;   //0:left, 1:right, 2:average
//Bit 1      reg_cr_fst         // unsigned,  RW,  default = 0;   //0:cb first   1:cr first
//Bit 0: 0   reg_yc_switch      // unsigned,  RW,  default = 0;
#define VPU_VOUT_BT_PLD_LINE                       0x0131
//Bit 31:29  reserved
//Bit 28:16  reg_payload_line_e // unsigned,  RW,  default = 10;
//Bit 15:13  reserved
//Bit 12: 0  reg_payload_line_o // unsigned,  RW,  default = 572;
#define VPU_VOUT_BT_PLDIDT0                        0x0132
//Bit 31:26  reserved
//Bit 25:16  reg_payload_idt1   // unsigned,  RW,  default = 10'h32c;  // payload identifier byte1
//Bit 15:10  reserved
//Bit 9:0    reg_payload_idt0   // unsigned,  RW,  default = 10'h224; // payload identifier byte0
#define VPU_VOUT_BT_PLDIDT1                        0x0133
//Bit 31:26  reserved
//Bit 25:16  reg_payload_idt3   // unsigned,  RW,  default = 10'h4;  // payload identifier byte3
//Bit 15:10  reserved
//Bit 9:0    reg_payload_idt2   // unsigned,  RW,  default = 10'h80; // payload identifier byte2
#define VPU_VOUT_BT_BLK_DATA                       0x0134
//Bit 31:26  reserved
//Bit 25:16  reg_blank_data0    // unsigned,  RW,  default = 64;   //for Y,R,G,B
//Bit 15:10  reserved
//Bit  9: 0  reg_blank_data1    // unsigned,  RW,  default = 512;  //for Cb,Cr
#define VPU_VOUT_BT_DAT_CLPY                       0x0135
//Bit 31:26  reserved
//Bit 25:16  reg_dat_clp_max_y    // unsigned,  RW,  default = 940;   //for y max clip
//Bit 15:10  reserved
//Bit  9: 0  reg_dat_clp_min_y    // unsigned,  RW,  default = 0;      //for y min clip
#define VPU_VOUT_BT_DAT_CLPC                       0x0136
//Bit 31:26  reserved
//Bit 25:16  reg_dat_clp_max_c    // unsigned,  RW,  default = 960;   //for C max clip
//Bit 15:10  reserved
//Bit  9: 0  reg_dat_clp_min_c    // unsigned,  RW,  default = 64;      //for C min clip
#define VPU_VOUT_RO_INT                            0x0140
//Bit  31:0  ro_vout_timgen_status  //unsigned, RO  {afifo_count[4:0],ro_int_error_cnt[7:0]}
//
/* ******************************** */

#define VPP_VD1_MATRIX_COEF00_01                   0x0280
#define VPP_VD1_MATRIX_COEF02_10                   0x0281
#define VPP_VD1_MATRIX_COEF11_12                   0x0282
#define VPP_VD1_MATRIX_COEF20_21                   0x0283
#define VPP_VD1_MATRIX_COEF22                      0x0284
#define VPP_VD1_MATRIX_COEF13_14                   0x0285
#define VPP_VD1_MATRIX_COEF23_24                   0x0286
#define VPP_VD1_MATRIX_COEF15_25                   0x0287
#define VPP_VD1_MATRIX_OFFSET0_1                   0x0289
#define VPP_VD1_MATRIX_OFFSET2                     0x028a

/* ***********************************************
 * DSI Host Controller register offset address define
 * VCBUS_BASE = 0x2c(0x2c00 - 0x2cff)
 */
/* DWC IP registers */
#define MIPI_DSI_DWC_VERSION_OS                    0x0000
#define MIPI_DSI_DWC_PWR_UP_OS                     0x0001
#define MIPI_DSI_DWC_CLKMGR_CFG_OS                 0x0002
#define MIPI_DSI_DWC_DPI_VCID_OS                   0x0003
#define MIPI_DSI_DWC_DPI_COLOR_CODING_OS           0x0004
#define MIPI_DSI_DWC_DPI_CFG_POL_OS                0x0005
#define MIPI_DSI_DWC_DPI_LP_CMD_TIM_OS             0x0006
#define MIPI_DSI_DWC_PCKHDL_CFG_OS                 0x000b
#define MIPI_DSI_DWC_GEN_VCID_OS                   0x000c
#define MIPI_DSI_DWC_MODE_CFG_OS                   0x000d
#define MIPI_DSI_DWC_VID_MODE_CFG_OS               0x000e
#define MIPI_DSI_DWC_VID_PKT_SIZE_OS               0x000f
#define MIPI_DSI_DWC_VID_NUM_CHUNKS_OS             0x0010
#define MIPI_DSI_DWC_VID_NULL_SIZE_OS              0x0011
#define MIPI_DSI_DWC_VID_HSA_TIME_OS               0x0012
#define MIPI_DSI_DWC_VID_HBP_TIME_OS               0x0013
#define MIPI_DSI_DWC_VID_HLINE_TIME_OS             0x0014
#define MIPI_DSI_DWC_VID_VSA_LINES_OS              0x0015
#define MIPI_DSI_DWC_VID_VBP_LINES_OS              0x0016
#define MIPI_DSI_DWC_VID_VFP_LINES_OS              0x0017
#define MIPI_DSI_DWC_VID_VACTIVE_LINES_OS          0x0018
#define MIPI_DSI_DWC_EDPI_CMD_SIZE_OS              0x0019
#define MIPI_DSI_DWC_CMD_MODE_CFG_OS               0x001a
#define MIPI_DSI_DWC_GEN_HDR_OS                    0x001b
#define MIPI_DSI_DWC_GEN_PLD_DATA_OS               0x001c
#define MIPI_DSI_DWC_CMD_PKT_STATUS_OS             0x001d
#define MIPI_DSI_DWC_TO_CNT_CFG_OS                 0x001e
#define MIPI_DSI_DWC_HS_RD_TO_CNT_OS               0x001f
#define MIPI_DSI_DWC_LP_RD_TO_CNT_OS               0x0020
#define MIPI_DSI_DWC_HS_WR_TO_CNT_OS               0x0021
#define MIPI_DSI_DWC_LP_WR_TO_CNT_OS               0x0022
#define MIPI_DSI_DWC_BTA_TO_CNT_OS                 0x0023
#define MIPI_DSI_DWC_SDF_3D_OS                     0x0024
#define MIPI_DSI_DWC_LPCLK_CTRL_OS                 0x0025
#define MIPI_DSI_DWC_PHY_TMR_LPCLK_CFG_OS          0x0026
#define MIPI_DSI_DWC_PHY_TMR_CFG_OS                0x0027
#define MIPI_DSI_DWC_PHY_RSTZ_OS                   0x0028
#define MIPI_DSI_DWC_PHY_IF_CFG_OS                 0x0029
#define MIPI_DSI_DWC_PHY_ULPS_CTRL_OS              0x002a
#define MIPI_DSI_DWC_PHY_TX_TRIGGERS_OS            0x002b
#define MIPI_DSI_DWC_PHY_STATUS_OS                 0x002c
#define MIPI_DSI_DWC_PHY_TST_CTRL0_OS              0x002d
#define MIPI_DSI_DWC_PHY_TST_CTRL1_OS              0x002e
#define MIPI_DSI_DWC_INT_ST0_OS                    0x002f
#define MIPI_DSI_DWC_INT_ST1_OS                    0x0030
#define MIPI_DSI_DWC_INT_MSK0_OS                   0x0031
#define MIPI_DSI_DWC_INT_MSK1_OS                   0x0032

#define MIPI_DSI_TOP_SHADOW_HSA_TIME               0x00e8
#define MIPI_DSI_TOP_SHADOW_HBP_TIME               0x00e9
#define MIPI_DSI_TOP_SHADOW_HLINE_TIME             0x00ea
#define MIPI_DSI_TOP_SHADOW_VSA_LINES              0x00eb
#define MIPI_DSI_TOP_SHADOW_VBP_LINES              0x00ec
#define MIPI_DSI_TOP_SHADOW_VFP_LINES              0x00ed
#define MIPI_DSI_TOP_SHADOW_VACTIVE_LINES          0x00ee
#define MIPI_DSI_TOP_SHADOW_TIME_CTRL              0x00ef
/* Top-level registers */
/* [31: 4]    Reserved.     Default 0.
 *     [3] RW ~tim_rst_n:   Default 1.
 *         1=Assert SW reset on mipi_dsi_host_timing block.   0=Release reset.
 *     [2] RW dpi_rst_n: Default 1.
 *         1=Assert SW reset on mipi_dsi_host_dpi block.   0=Release reset.
 *     [1] RW intr_rst_n: Default 1.
 *         1=Assert SW reset on mipi_dsi_host_intr block.  0=Release reset.
 *     [0] RW dwc_rst_n:  Default 1.
 *         1=Assert SW reset on IP core.   0=Release reset.
 */
#define MIPI_DSI_TOP_SW_RESET                      0x00f0
/* [31: 5] Reserved.   Default 0.
 *     [4] RW manual_edpihalt: Default 0.
 *		1=Manual suspend VencL; 0=do not suspend VencL.
 *     [3] RW auto_edpihalt_en: Default 0.
 *		1=Enable IP's edpihalt signal to suspend VencL;
 *		0=IP's edpihalt signal does not affect VencL.
 *     [2] RW clock_freerun: Apply to auto-clock gate only. Default 0.
 *		0=Default, use auto-clock gating to save power;
 *		1=use free-run clock, disable auto-clock gating, for debug mode.
 *     [1] RW enable_pixclk: A manual clock gate option, due to DWC IP does not
 *		have auto-clock gating. 1=Enable pixclk.      Default 0.
 *     [0] RW enable_sysclk: A manual clock gate option, due to DWC IP does not
 *		have auto-clock gating. 1=Enable sysclk.      Default 0.
 */
#define MIPI_DSI_TOP_CLK_CNTL                      0x00f1
/* [31:24]    Reserved. Default 0.
 * [23:20] RW dpi_color_mode: Define DPI pixel format. Default 0.
 *		0=16-bit RGB565 config 1;
 *		1=16-bit RGB565 config 2;
 *		2=16-bit RGB565 config 3;
 *		3=18-bit RGB666 config 1;
 *		4=18-bit RGB666 config 2;
 *		5=24-bit RGB888;
 *		6=20-bit YCbCr 4:2:2;
 *		7=24-bit YCbCr 4:2:2;
 *		8=16-bit YCbCr 4:2:2;
 *		9=30-bit RGB;
 *		10=36-bit RGB;
 *		11=12-bit YCbCr 4:2:0.
 *    [19] Reserved. Default 0.
 * [18:16] RW in_color_mode:  Define VENC data width. Default 0.
 *		0=30-bit pixel;
 *		1=24-bit pixel;
 *		2=18-bit pixel, RGB666;
 *		3=16-bit pixel, RGB565.
 * [15:14] RW chroma_subsample: Define method of chroma subsampling. Default 0.
 *		Applicable to YUV422 or YUV420 only.
 *		0=Use even pixel's chroma;
 *		1=Use odd pixel's chroma;
 *		2=Use averaged value between even and odd pair.
 * [13:12] RW comp2_sel:  Select which component to be Cr or B: Default 2.
 *		0=comp0; 1=comp1; 2=comp2.
 * [11:10] RW comp1_sel:  Select which component to be Cb or G: Default 1.
 *		0=comp0; 1=comp1; 2=comp2.
 *  [9: 8] RW comp0_sel:  Select which component to be Y  or R: Default 0.
 *		0=comp0; 1=comp1; 2=comp2.
 *     [7]    Reserved. Default 0.
 *     [6] RW de_pol:  Default 0.
 *		If DE input is active low, set to 1 to invert to active high.
 *     [5] RW hsync_pol: Default 0.
 *		If HS input is active low, set to 1 to invert to active high.
 *     [4] RW vsync_pol: Default 0.
 *		If VS input is active low, set to 1 to invert to active high.
 *     [3] RW dpicolorm: Signal to IP.   Default 0.
 *     [2] RW dpishutdn: Signal to IP.   Default 0.
 *     [1]    Reserved.  Default 0.
 *     [0]    Reserved.  Default 0.
 */
#define MIPI_DSI_TOP_CNTL                          0x00f2
#define MIPI_DSI_TOP_SUSPEND_CNTL                  0x00f3
#define MIPI_DSI_TOP_SUSPEND_LINE                  0x00f4
#define MIPI_DSI_TOP_SUSPEND_PIX                   0x00f5
#define MIPI_DSI_TOP_MEAS_CNTL                     0x00f6
/* [0] R  stat_edpihalt:  edpihalt signal from IP.    Default 0. */
#define MIPI_DSI_TOP_STAT                          0x00f7
#define MIPI_DSI_TOP_MEAS_STAT_TE0                 0x00f8
#define MIPI_DSI_TOP_MEAS_STAT_TE1                 0x00f9
#define MIPI_DSI_TOP_MEAS_STAT_VS0                 0x00fa
#define MIPI_DSI_TOP_MEAS_STAT_VS1                 0x00fb
/* [31:16] RW intr_stat/clr. Default 0.
 *		For each bit, read as this interrupt level status,
 *		write 1 to clear.
 * [31:22] Reserved
 * [   21] stat/clr of eof interrupt
 * [   21] vde_fall interrupt
 * [   19] stat/clr of de_rise interrupt
 * [   18] stat/clr of vs_fall interrupt
 * [   17] stat/clr of vs_rise interrupt
 * [   16] stat/clr of dwc_edpite interrupt
 * [15: 0] RW intr_enable. Default 0.
 *		For each bit, 1=enable this interrupt, 0=disable.
 *	[15: 6] Reserved
 *	[    5] eof interrupt
 *	[    4] de_fall interrupt
 *	[    3] de_rise interrupt
 *	[    2] vs_fall interrupt
 *	[    1] vs_rise interrupt
 *	[    0] dwc_edpite interrupt
 */
#define MIPI_DSI_TOP_INTR_CNTL_STAT                0x00fc
// 31: 2    Reserved.   Default 0.
//  1: 0 RW mem_pd.     Default 3.
#define MIPI_DSI_TOP_MEM_PD                        0x00fd

/* ***********************************************
 * DSI PHY register offset address define
 */
/* [31] soft reset for the phy.
 *		1: reset. 0: dessert the reset.
 * [30] clock lane soft reset.
 * [29] data byte lane 3 soft reset.
 * [28] data byte lane 2 soft reset.
 * [27] data byte lane 1 soft reset.
 * [26] data byte lane 0 soft reset.
 * [25] mipi dsi pll clock selection.
 *		1:  clock from fixed 850Mhz clock source. 0: from VID2 PLL.
 * [12] mipi HSbyteclk enable.
 * [11] mipi divider clk selection.
 *		1: select the mipi DDRCLKHS from clock divider.
 *		0: from PLL clock.
 * [10] mipi clock divider control.
 *		1: /4. 0: /2.
 * [9]  mipi divider output enable.
 * [8]  mipi divider counter enable.
 * [7]  PLL clock enable.
 * [5]  LPDT data endian.
 *		1 = transfer the high bit first. 0 : transfer the low bit first.
 * [4]  HS data endian.
 * [3]  force data byte lane in stop mode.
 * [2]  force data byte lane 0 in receiver mode.
 * [1]  write 1 to sync the txclkesc input. the internal logic have to
 *	use txclkesc to decide Txvalid and Txready.
 * [0]  enalbe the MIPI DSI PHY TxDDRClk.
 */
#define MIPI_DSI_PHY_CTRL       0x0
/* [31] clk lane tx_hs_en control selection.
 *		1: from register. 0: use clk lane state machine.
 * [30] register bit for clock lane tx_hs_en.
 * [29] clk lane tx_lp_en contrl selection.
 *		1: from register. 0: from clk lane state machine.
 * [28] register bit for clock lane tx_lp_en.
 * [27] chan0 tx_hs_en control selection.
 *		1: from register. 0: from chan0 state machine.
 * [26] register bit for chan0 tx_hs_en.
 * [25] chan0 tx_lp_en control selection.
 *		1: from register. 0: from chan0 state machine.
 * [24] register bit from chan0 tx_lp_en.
 * [23] chan0 rx_lp_en control selection.
 *		1: from register. 0: from chan0 state machine.
 * [22] register bit from chan0 rx_lp_en.
 * [21] chan0 contention detection enable control selection.
 *		1: from register. 0: from chan0 state machine.
 * [20] register bit from chan0 contention dectection enable.
 * [19] chan1 tx_hs_en control selection.
 *		1: from register. 0: from chan0 state machine.
 * [18] register bit for chan1 tx_hs_en.
 * [17] chan1 tx_lp_en control selection.
 *		1: from register. 0: from chan0 state machine.
 * [16] register bit from chan1 tx_lp_en.
 * [15] chan2 tx_hs_en control selection.
 *		1: from register. 0: from chan0 state machine.
 * [14] register bit for chan2 tx_hs_en.
 * [13] chan2 tx_lp_en control selection.
 *		1: from register. 0: from chan0 state machine.
 * [12] register bit from chan2 tx_lp_en.
 * [11] chan3 tx_hs_en control selection.
 *		1: from register. 0: from chan0 state machine.
 * [10] register bit for chan3 tx_hs_en.
 * [9]  chan3 tx_lp_en control selection.
 *		1: from register. 0: from chan0 state machine.
 * [8]  register bit from chan3 tx_lp_en.
 * [4]  clk chan power down. this bit is also used as the power down
 *	of the whole MIPI_DSI_PHY.
 * [3]  chan3 power down.
 * [2]  chan2 power down.
 * [1]  chan1 power down.
 * [0]  chan0 power down.
 */
#define MIPI_DSI_CHAN_CTRL      0x1
/* [24]   rx turn watch dog triggered.
 * [23]   rx esc watchdog  triggered.
 * [22]   mbias ready.
 * [21]   txclkesc  synced and ready.
 * [20:17] clk lane state. {mbias_ready, tx_stop, tx_ulps, tx_hs_active}
 * [16:13] chan3 state{0, tx_stop, tx_ulps, tx_hs_active}
 * [12:9]  chan2 state.{0, tx_stop, tx_ulps, tx_hs_active}
 * [8:5]   chan1 state. {0, tx_stop, tx_ulps, tx_hs_active}
 * [4:0]   chan0 state. {TX_STOP, tx_ULPS, hs_active, direction, rxulpsesc}
 */
#define MIPI_DSI_CHAN_STS       0x2
/* [31:24] TCLK_PREPARE.
 * [23:16] TCLK_ZERO.
 * [15:8]  TCLK_POST.
 * [7:0]   TCLK_TRAIL.
 */
#define MIPI_DSI_CLK_TIM        0x3
/* [31:24] THS_PREPARE.
 * [23:16] THS_ZERO.
 * [15:8]  THS_TRAIL.
 * [7:0]   THS_EXIT.
 */
#define MIPI_DSI_HS_TIM         0x4
/* [31:24] tTA_GET.
 * [23:16] tTA_GO.
 * [15:8]  tTA_SURE.
 * [7:0]   tLPX.
 */
#define MIPI_DSI_LP_TIM         0x5
/* wait time to  MIPI DIS analog ready. */
#define MIPI_DSI_ANA_UP_TIM     0x6
/* TINIT. */
#define MIPI_DSI_INIT_TIM       0x7
/* TWAKEUP. */
#define MIPI_DSI_WAKEUP_TIM     0x8
/* when in RxULPS check state, after the logic enable the analog,
 *	how long we should wait to check the lP state .
 */
#define MIPI_DSI_LPOK_TIM       0x9
/* Watchdog for RX low power state no finished. */
#define MIPI_DSI_LP_WCHDOG      0xa
/* tMBIAS,  after send power up signals to analog,
 *	how long we should wait for analog powered up.
 */
#define MIPI_DSI_ANA_CTRL       0xb
/* [31:8]  reserved for future.
 * [7:0]   tCLK_PRE.
 */
#define MIPI_DSI_CLK_TIM1       0xc
/* watchdog for turn around waiting time. */
#define MIPI_DSI_TURN_WCHDOG    0xd
/* When in RxULPS state, how frequency we should to check
 *	if the TX side out of ULPS state.
 */
#define MIPI_DSI_ULPS_CHECK     0xe

#define MIPI_DSI_TEST_CTRL0     0xf

#define MIPI_DSI_TEST_CTRL1     0x10

/*******************backlight***********************/
#define VPU_VPU_PWM_V0                             0x2730
#define VPU_VPU_PWM_V1                             0x2731
#define VPU_VPU_PWM_V2                             0x2732
#define VPU_VPU_PWM_V3                             0x2733
#define VPU_VPU_PWM_H0                             0x2734
/* ***********************************************
 * register access api
 */
int lcd_ioremap(struct platform_device *pdev);
unsigned int lcd_vcbus_read(unsigned int reg);
void lcd_vcbus_write(unsigned int reg, unsigned int value);
void lcd_vcbus_setb(unsigned int reg, unsigned int value,
		    unsigned int start, unsigned int len);
unsigned int lcd_vcbus_getb(unsigned int reg, unsigned int start, unsigned int len);
void lcd_vcbus_set_mask(unsigned int reg, unsigned int mask);
void lcd_vcbus_clr_mask(unsigned int reg, unsigned int mask);

unsigned int lcd_clk_read(unsigned int reg);
void lcd_clk_write(unsigned int reg, unsigned int value);
void lcd_clk_setb(unsigned int reg, unsigned int value,
		  unsigned int start, unsigned int len);
unsigned int lcd_clk_getb(unsigned int reg, unsigned int start, unsigned int len);
void lcd_clk_set_mask(unsigned int reg, unsigned int mask);
void lcd_clk_clr_mask(unsigned int reg, unsigned int mask);

unsigned int lcd_ana_read(unsigned int reg);
void lcd_ana_write(unsigned int reg, unsigned int value);
void lcd_ana_setb(unsigned int reg, unsigned int value,
		  unsigned int start, unsigned int len);
unsigned int lcd_ana_getb(unsigned int reg,
			  unsigned int start, unsigned int len);

unsigned int lcd_cbus_read(unsigned int reg);
void lcd_cbus_write(unsigned int reg, unsigned int value);
void lcd_cbus_setb(unsigned int reg, unsigned int value,
		   unsigned int start, unsigned int len);

unsigned int lcd_periphs_read(unsigned int reg);
void lcd_periphs_write(unsigned int reg, unsigned int value);

unsigned int dsi_host_read(unsigned int reg);
void dsi_host_write(unsigned int reg, unsigned int value);
void dsi_host_setb(unsigned int reg, unsigned int value,
		   unsigned int start, unsigned int len);
unsigned int dsi_host_getb(unsigned int reg, unsigned int start, unsigned int len);
void dsi_host_set_mask(unsigned int reg, unsigned int mask);
void dsi_host_clr_mask(unsigned int reg, unsigned int mask);
unsigned int dsi_phy_read(unsigned int reg);
void dsi_phy_write(unsigned int reg, unsigned int value);
void dsi_phy_setb(unsigned int reg, unsigned int value,
		  unsigned int start, unsigned int len);
unsigned int dsi_phy_getb(unsigned int reg,
			  unsigned int start, unsigned int len);
void dsi_phy_set_mask(unsigned int reg, unsigned int mask);
void dsi_phy_clr_mask(unsigned int reg, unsigned int mask);

unsigned int lcd_tcon_read(unsigned int reg);
void lcd_tcon_write(unsigned int reg, unsigned int value);
void lcd_tcon_setb(unsigned int reg, unsigned int value,
		   unsigned int start, unsigned int len);
unsigned int lcd_tcon_getb(unsigned int reg,
			   unsigned int start, unsigned int len);
void lcd_tcon_set_mask(unsigned int reg, unsigned int mask);
void lcd_tcon_clr_mask(unsigned int reg, unsigned int mask);
void lcd_tcon_update_bits(unsigned int reg,
			  unsigned int mask, unsigned int value);
int lcd_tcon_check_bits(unsigned int reg, unsigned int mask, unsigned int value);
unsigned char lcd_tcon_read_byte(unsigned int reg);
void lcd_tcon_write_byte(unsigned int reg, unsigned char value);
void lcd_tcon_setb_byte(unsigned int reg, unsigned char value,
			unsigned int start, unsigned int len);
unsigned char lcd_tcon_getb_byte(unsigned int reg,
				 unsigned int start, unsigned int len);
void lcd_tcon_update_bits_byte(unsigned int reg,
			       unsigned char mask, unsigned char value);
int lcd_tcon_check_bits_byte(unsigned int reg,
			     unsigned char mask, unsigned char value);

unsigned int dptx_reg_read(unsigned int reg);
void dptx_reg_write(unsigned int reg, unsigned int value);
void dptx_reg_setb(unsigned int reg, unsigned int value,
		   unsigned int start, unsigned int len);
unsigned int dptx_reg_getb(unsigned int reg, unsigned int start, unsigned int len);

unsigned int lcd_combo_dphy_read(unsigned int reg);
void lcd_combo_dphy_write(unsigned int reg, unsigned int value);
void lcd_combo_dphy_setb(unsigned int reg, unsigned int value,
			 unsigned int start, unsigned int len);
unsigned int lcd_combo_dphy_getb(unsigned int reg,
				 unsigned int start, unsigned int len);
unsigned int lcd_reset_read(unsigned int reg);
void lcd_reset_write(unsigned int reg, unsigned int value);
void lcd_reset_setb(unsigned int reg,
		    unsigned int value, unsigned int start, unsigned int len);
unsigned int lcd_reset_getb(unsigned int reg, unsigned int start, unsigned int len);
void lcd_reset_set_mask(unsigned int reg, unsigned int mask);
void lcd_reset_clr_mask(unsigned int reg, unsigned int mask);
#endif

