/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */
#ifndef __FRC_REG_H__
#define __FRC_REG_H__

#define FRC_TOP_SW_RESET                           0x0000
//Bit 31:30        reserved
//Bit 29:0         reg_top_sw_resets    // unsigned ,   RW, default = 0  ,
#define FRC_REG_TOP_CTRL1                          0x0001
//Bit 31:29        reserved
//Bit 28:16        reg_frc_probe_pt_y    // unsigned ,   RW, default = 100  reg_mc_probe_pt_y mc_probe positon registers
//Bit 15:13        reserved
//Bit 12:0         reg_frc_probe_pt_x    // unsigned ,   RW, default = 100  reg_mc_probe_pt_x mc_probe positon registers
#define FRC_INP_HOLD_CTRL                          0x0002
//Bit 31           pls_inp_frm_start     // unsigned ,   RW, default = 0,start by write high in pluse start mode
//Bit 30:21        reserved
//Bit 20           reg_frc_win_en        // unsigned ,   RW, default = 1,0:auto start mode 1:pluse start mode
//Bit 19:17        reserved
//Bit 16           reg_inp_frm_start_sel // unsigned ,   RW, default = 0,0:auto start mode 1:pluse start mode
//Bit 15:13        reserved
//Bit 12:0         reg_inp_hold_line     // unsigned ,   RW, default = 6,inp start reg_inp_hold_line after go_field in auto start mode
#define FRC_OUT_HOLD_CTRL                          0x0003
//Bit 31           reg_mc_frm_ctrl       // unsigned ,   RW, default = 1,1:use mc_frm_rst/en by frc_top 0:generate mc_frm_rst/en before first me out
//Bit 30:24        reg_mc_hold_regen     // unsigned ,   RW, default = 2,hold_line for  regenerate frm_rst/me
//Bit 23:16        reg_out_hold_line     // unsigned ,   RW, default = 2,output data reg_out_hold_line lines after go_field
//Bit 15:8         reg_mc_hold_line      // unsigned ,   RW, default = 2,start  work reg_mc_hold_line  lines after frm_mc_rst
//Bit 7 :0         reg_me_hold_line      // unsigned ,   RW, default = 2,start  work reg_me_hold_line  lines after frm_me_rst
#define FRC_TOP_ASYNC_CTRL                         0x0004
//Bit 31:24         reserved
//Bit 23:20         reg_async_mask_num   // unsigned ,   RW, default = 12       ,mask_num
//Bit 19:0          reg_axi_asyn_ctrl    // unsigned ,   RW, default = 20'h44444,reg_axi_asyn_ctrl
#define FRC_LINE_INT_CTRL0                         0x0005
//Bit 31:16         reg_me_line_int_num   // unsigned ,   RW, default = 100,get a interupt when ref_line = reg_me_line_int_num
//Bit 15:0          reg_inp_line_int_num  // unsigned ,   RW, default = 100,get a interupt when ref_line = reg_inp_line_int_num
#define FRC_LINE_INT_CTRL1                         0x0006
//Bit 31:16         reserved
//Bit 15:0          reg_mc_line_int_num   // unsigned ,   RW, default = 100,get a interupt when ref_line = reg_mc_line_int_num
#define FRC_REG_TOP_CTRL7                          0x0007
//Bit 31:29         reserved
//Bit 28:24         reg_buf_cfg_en       // unsigned ,    RW, default = 0  [0]:force mc buf_idx [1] force logo buf_idx [2]:force me_phase [3] force mc_phase [4] force input_buf_idx
//Bit 23:20         reg_bufwr_idx        // unsigned ,    RW, default = 0  input buf_idx setting less than logo_fb_num
//Bit 19:0          reg_bufrd_idx        // unsigned ,    RW, default = 0  19:16 logo_cur_idx 15:12 logo_pre_idx, 11:8 pix_nxt_idx,7:4 pix_cur_idx,3:0 pix_pre_idx
#define FRC_REG_TOP_CTRL8                          0x0008
//Bit 31:16         reserved
//Bit 15:8          reg_mc_phase         // unsigned ,    RW, default = 0   mc_phase in force condition
//Bit  7:0          reg_me_phase         // unsigned ,    RW, default = 0   me_phase in force condition
#define FRC_REG_TOP_CTRL9                          0x0009
//Bit 31            reg_sleep_sel        // unsigned ,    RW, default = 0 ,ddr sleep module enable
//Bit 30:29         reserved
//Bit 28            reg_frm_hold         // unsigned ,    RW, default = 0  buffer stop refresh when enable this bit
//Bit 27:24         reg_out_frm_dly_num  // unsigned ,    RW, default = 3  frm dly number of frc
//Bit 23:0          reserved
#define FRC_REG_TOP_CTRL10                         0x000a
//Bit 31:8          reserved
//Bit  7:0          reg_inp_wrmif_en     // unsigned ,RW, default = 32'hff 0:mc_wr_en 1:me_wr_en 2:hme_wr_en 3:iplogo_wr_en 4:ip_blklogo_wr_en 5:iplogo_iir0_wr_en 6 iplogo_iir1_wr_en
#define FRC_REG_TOP_CTRL11                         0x000b
//Bit 31:24         reg_frc_me_loss_slice_num      // unsigned ,    RW, default = 4
//Bit 23:16         reg_frc_mc_loss_slice_num      // unsigned ,    RW, default = 4
//Bit 15:4          reserved
//Bit  3            reg_frc_me_loss_slice_num_bpmd // unsigned ,    RW, default = 1
//Bit  2            reg_frc_mc_loss_slice_num_bpmd // unsigned ,    RW, default = 1
//Bit  1            reg_frc_me_loss_en             // unsigned ,    RW, default = 0
//Bit  0            reg_frc_mc_loss_en             // unsigned ,    RW, default = 0
#define FRC_AUTO_RST_SEL                           0x000c
//Bit 31:6  reserved
//Bit 5 :0  reg_auto_rst_sel      // unsigned ,   RW, default = 6'h3c,{lut,mc,mevp,inp,top,ctrl} 1:software reset 0:auto_reset
#define FRC_REG_TOP_CTRL13                         0x000d
//Bit 31:7          reserved
//Bit 6             reg_force_film_end       // unsigned ,    RW, default = 0  force film_end of every phase high
//Bit 5             reg_mevp_mc_sync_en      // unsigned ,    RW, default = 1  sync enable singal for mevp and mc
//Bit 4             reg_mevp_mv_force_en     // unsigned ,    RW, default = 0  force module mevp mv
//Bit 3             reg_mevp_logo_force_en   // unsigned ,    RW, default = 0  force module mevp logo
//Bit 2:0           reserved
#define FRC_REG_TOP_CTRL14                         0x000e
//Bit 31:16         reg_me_dly_vofst         // unsigned ,    RW, default = 8
//Bit 15:0          reg_post_dly_vofst       // unsigned ,    RW, default = 0
#define FRC_REG_TOP_CTRL15                         0x000f
//Bit 31:16         reg_mc_dly_vofst1        // unsigned ,    RW, default = 190   8x8 : 190  16x16: 160
//Bit 15:0          reg_mc_dly_vofst0        // unsigned ,    RW, default = 35
#define FRC_REG_TOP_CTRL16                         0x0010
//Bit 31:16         reserved
//Bit 15:0          reg_disp_dly_vofst       // unsigned ,    RW, default = 195
#define FRC_REG_TOP_CTRL17                         0x0011
//Bit 31:10         reserved
//Bit 9             reg_film_phs_test        // unsigned ,    RW, default = 0
//Bit 8             reg_post_buf_prot        // unsigned ,    RW, default = 0
//Bit 7 :5          reserved
//Bit 4             pls_lut_cfg_end          // unsigned ,    RW, default = 0 ,finish config phs_lut
//Bit 3             pls_lut_cfg_en           // unsigned ,    RW, default = 0 ,start  config phs_lut
//Bit 2             reg_lut_sel              // unsigned ,    RW, default = 0
//Bit 1             reg_inp_frm_en_sel       // unsigned ,    RW, default = 0
//Bit 0             reg_lutrd_latch_sel      // unsigned ,    RW, default = 0
#define FRC_REG_TOP_CTRL18                         0x0012
//Bit 31:0          reg_inp_frm_vld_lut_0    // unsigned ,    RW, default = 0  frc_phase
#define FRC_REG_TOP_CTRL19                         0x0013
//Bit 31:0          reg_inp_frm_vld_lut_1    // unsigned ,    RW, default = 0  frc_phase
#define FRC_FRM_VLD_LUT_2                          0x0014
//Bit 31:0          reg_inp_frm_vld_lut_2    // unsigned ,    RW, default = 0  frc_phase
#define FRC_FRM_VLD_LUT_3                          0x0015
//Bit 31:0          reg_inp_frm_vld_lut_3    // unsigned ,    RW, default = 0  frc_phase
#define FRC_FRM_VLD_LUT_4                          0x0016
//Bit 31:0          reg_inp_frm_vld_lut_4    // unsigned ,    RW, default = 0  frc_phase
#define FRC_FRM_VLD_LUT_5                          0x0017
//Bit 31:0          reg_inp_frm_vld_lut_5    // unsigned ,    RW, default = 0  frc_phase
#define FRC_FRM_VLD_LUT_6                          0x0018
//Bit 31:0          reg_inp_frm_vld_lut_6    // unsigned ,    RW, default = 0  frc_phase
#define FRC_FRM_VLD_LUT_7                          0x0019
//Bit 31:0          reg_inp_frm_vld_lut_7    // unsigned ,    RW, default = 0  frc_phase
#define FRC_RO_TOP_STAT1                           0x0021
//Bit 31:0          ro_frc_stat1             // unsigned ,   RW, default = 0  ro_frc_stat2
#define FRC_RO_TOP_STAT2                           0x0022
//Bit 31:0          ro_frc_stat2             // unsigned ,   RW, default = 0  ro_frc_stat2
#define FRC_RO_TOP_STAT3                           0x0023
//Bit 31:0          ro_frc_stat3             // unsigned ,   RW, default = 0  ro_frc_stat3
#define FRC_RO_TOP_STAT4                           0x0024
//Bit 31:0          ro_frc_stat4             // unsigned ,   RW, default = 0  ro_frc_stat4
#define FRC_SYNC_SW_RESETS                         0x0025
//Bit 31:6  reserved
//Bit 5 :0  pls_frc_sw_rst        // unsigned ,   RW, default = 0,{lut,mc,mevp,inp,top,ctrl}
#define FRC_REG_TOP_CTRL25                         0x0026
//Bit 31            reg_inp_padding_en       // unsigned ,   RW, default = 0  reg_inp_padding_en
//Bit 30:0          reg_padding_value        // unsigned ,   RW, default = 0  reg_padding_value
#define FRC_REG_TOP_CTRL26                         0x0027
//Bit 31:26         reserved
//Bit 25:13         reg_org_inp_frm_vsize    // unsigned ,   RW, default = 0  reg_org_inp_frm_vsize
//Bit 12:0          reg_org_inp_frm_hsize    // unsigned ,   RW, default = 0  reg_org_inp_frm_hsize
#define FRC_REG_TOP_CTRL27                         0x0028
//Bit 31:26         reserved
//Bit 25:13         reg_inp_padding_xofset   // unsigned ,   RW, default = 0  reg_inp_padding_xofset
//Bit 12:0          reg_inp_padding_yofset   // unsigned ,   RW, default = 0  reg_inp_padding_yofset
#define FRC_REG_TOP_CTRL28                         0x0029
//Bit 31:0         reserved
#define FRC_REG_TOP_CTRL29                         0x002a
//Bit 31:2         reserved
//Bit    1         reg_memc_post_hold        // unsigned ,   RW, default = 0
//Bit    0         reg_memc_top_hold         // unsigned ,   RW, default = 0
#define FRC_BYP_PATH_CTRL                          0x002d
//Bit 31:5         reserved
//Bit 4            reg_path_ds_en  default = 0  use down sample data in prefetch period
//Bit 3:1          reserved
//Bit 0            reg_path_byp_en default = 1
//                 1: data bypass to display when memc prefetch data
//                 0: data don't bypass to display when memc prefetch data
#define FRC_REG_INP_INT_MASK                       0x0030
//Bit 31:1         reserved
//Bit 0            reg_inp_int_mask          // unsigned ,   RW, default = 0  reg_inp_int_mask 1:mask interupt 0:open interupt
#define FRC_REG_INP_INT_FLAG                       0x0031
//Bit 31:5         reserved
//Bit 4            ro_out_of_pfth_phs        // unsigned ,   RO, default = 0
//Bit 3 :2         reserved
//Bit 1            ro_inp_int_24hz           // unsigned ,   RO, default = 0   inpu  interupt 24hz flag
//Bit 0            ro_inp_int_flag           // unsigned ,   RO, default = 0   input interupt flag
#define FRC_REG_INP_INT_CLR                        0x0032
//Bit 31:1         reserved
//Bit 0            pls_inp_int_clr           // unsigned ,   RW, default = 0   input interupt flag clr bit
#define FRC_REG_OUT_INT_SEL                        0x0033
//Bit 31:13        reserved
//Bit 12           reg_inp_int_sel           // unsigned ,   RW, default = 0   0:frm_en_pre as interupt 1:out_frm_en as interupt
//Bit 11           reserved
//Bit 10:8         reg_out_frm_int_sel       // unsigned ,   RW, default = 4   0:me_int 1:melogo_int 2:vp_int 3:mc_int 4:disp_int
//Bit 7 :5         reserved
//Bit 4 :0         reg_out_int_sel           // unsigned ,   RW, default = 0   0:frm_en as interupt 1:frm_end as interupt
#define FRC_REG_OUT_INT_MASK                       0x0034
//Bit 31:9         reserved
//Bit 8            reg_out_frm_int_mask       // unsigned ,  RW, default = 0   out_frm_int mask bit
//Bit 7 :5         reserved
//Bit 4 :0         reg_out_int_mask           // unsigned ,  RW, default = 0   0:me_int mask 1:melogo_int mask 2:vp_int mask 3:mc_int mask
#define FRC_REG_OUT_INT_FLAG                       0x0035
//Bit 31:9         reserved
//Bit 8            ro_out_frm_int_flag       // unsigned ,   RO, default = 0   out_frm_int flag bit
//Bit 7 :5         reserved
//Bit 4 :0         ro_out_int_flag           // unsigned ,   RO, default = 0   0:me_int flag 1:melogo_int flag 2:vp_int flag 3:mc_int flag
#define FRC_REG_OUT_INT_CLR                        0x0036
//Bit 31:9         reserved
//Bit 8            pls_out_frm_int_clr       // unsigned ,   RW, default = 0   output     interupt flag clr bit
//Bit 7 :5         reserved
//Bit 4            pls_out_int4_clr          // unsigned ,   RW, default = 0   out_domain interupt flag clr bit4
//Bit 3            pls_out_int3_clr          // unsigned ,   RW, default = 0   out_domain interupt flag clr bit3
//Bit 2            pls_out_int2_clr          // unsigned ,   RW, default = 0   out_domain interupt flag clr bit2
//Bit 1            pls_out_int1_clr          // unsigned ,   RW, default = 0   out_domain interupt flag clr bit1
//Bit 0            pls_out_int0_clr          // unsigned ,   RW, default = 0   out_domain interupt flag clr bit0
#define FRC_REG_INT_VCNT_DLY                       0x0037
//Bit 31:29        reserved
//Bit 28:16        reg_out_int_line_dly     // unsigned ,   RW, default = 0  reg_out_int_line_dly
//Bit 15:13        reserved
//Bit 12:0         reg_inp_int_line_dly     // unsigned ,   RW, default = 0  reg_inp_int_line_dly
#define FRC_INP_LINE_SEL                           0x0038
//Bit 31:3         reserved
//Bit 2 :0         reg_ref_line_sel        // unsigned ,   RW, default = 0   ,bit0:inp_line_int ,bit1:me_line_int ,bit2:mc_line_int 0:use hsync from venc 1:inner counter
#define FRC_REG_LINE_INT_MASK                      0x0039
//Bit 31:3         reserved
//Bit 2 :0         reg_line_int_mask       // unsigned ,   RW, default = 6   0:inp_line_int 1:me_line_int 2:mc_line_int
#define FRC_REG_LINE_INT_FLAG                      0x003a
//Bit 31:3         reserved
//Bit 2 :0         ro_line_int_flag        // unsigned ,   RO, default = 0   0:inp_line_int 1:me_line_int 2:mc_line_int
#define FRC_REG_LINE_INT_CLR                       0x003b
//Bit 31:3         reserved
//Bit 2 :0         pls_line_int_clr        // unsigned ,   RW, default = 0   0:inp_line_int 1:me_line_int 2:mc_line_int
#define FRC_REG_MODE_OPT                           0x003d
//Bit 31:7  reserved
//Bit 6             reg_apb_prot_opt         // unsigned ,    RW, default = 0   reg_out_int_opt
//Bit 5             reg_out_int_opt          // unsigned ,    RW, default = 0   reg_out_int_opt
//Bit 4             reg_load_frm_opt         // unsigned ,    RW, default = 0   reg_load_frm_opt
//Bit 3             reg_para_mux_opt         // unsigned ,    RW, default = 0   reg_para_mux_opt
//Bit 2             reg_bd_ro_syn_mode       // unsigned ,    RW, default = 0   reg_bd_ro_syn_mode
//Bit 1             reg_bd_inp_syn_mode      // unsigned ,    RW, default = 0   reg_bd_inp_syn_mode
//Bit 0             reg_cfg_syn_mode         // unsigned ,    RW, default = 0   reg_cfg_syn_mode
#define FRC_TOTAL_NUM_MODE                         0x003e
//Bit 31:1          reserved
//Bit 0             reg_total_num_mode      // unsigned ,    RW, default = 0   reg_total_num_mode
#define FRC_REG_BW_FRM_LINE_CNT                    0x0040
//Bit 31:8         reserved
//Bit 7 :3         reg_bw_frm_cnt            // unsigned ,   RW, default = 0   stat frm cnt
//Bit 2 :0         reg_bw_line_cnt           // unsigned ,   RW, default = 0   stat line cnt
#define FRC_REG_FRC0_R_BW_AVG                      0x0041
//Bit 31:0         ro_frc0_r_bw_avg          // unsigned,   RO, default = 0
#define FRC_REG_FRC0_R_BW_PEAK                     0x0042
//Bit 31:0         ro_frc0_r_bw_peak         // unsigned,   RO, default = 0
#define FRC_REG_FRC1_R_BW_AVG                      0x0043
//Bit 31:0         ro_frc1_r_bw_avg          // unsigned,   RO, default = 0
#define FRC_REG_FRC1_R_BW_PEAK                     0x0044
//Bit 31:0         ro_frc1_r_bw_peak         // unsigned,   RO, default = 0
#define FRC_REG_FRC2_R_BW_AVG                      0x0045
//Bit 31:0         ro_frc2_r_bw_avg          // unsigned,   RO, default = 0
#define FRC_REG_FRC2_R_BW_PEAK                     0x0046
//Bit 31:0         ro_frc2_r_bw_peak         // unsigned,   RO, default = 0
#define FRC_REG_FRC0_W_BW_AVG                      0x0047
//Bit 31:0         ro_frc0_w_bw_avg          // unsigned,   RO, default = 0
#define FRC_REG_FRC0_W_BW_PEAK                     0x0048
//Bit 31:0         ro_frc0_w_bw_peak         // unsigned,   RO, default = 0
#define FRC_REG_FRC1_W_BW_AVG                      0x0049
//Bit 31:0         ro_frc1_w_bw_avg          // unsigned,   RO, default = 0
#define FRC_REG_FRC1_W_BW_PEAK                     0x004a
//Bit 31:0         ro_frc1_w_bw_peak         // unsigned,   RO, default = 0
#define FRC_REG_INP_RESERVED                       0x004b
//Bit 31:0         reserved
#define FRC_REG_RST_CTRL                           0x004c
//Bit 31:16       reserved
//Bit 15:0        reg_frc_rst_ctrl           // unsigned,   RW, default = 16'h5555
#define FRC_RO_DBG0_STAT                           0x004d
//Bit 31:16       ro_ref_vs_dly_num          // unsigned,   RO, default = 0   the delay from ref_frm_en to vs
//Bit 15:0        ro_ref_de_dly_num          // unsigned,   RO, default = 0   the delay from ref_frm_en to de
#define FRC_RO_DBG1_STAT                           0x004e
//Bit 31:16       ro_mevp_dly_num            // unsigned,   RO, default = 0   the delay from me frm en to vp first out data
//Bit 15:0        ro_mc2out_dly_num          // unsigned,   RO, default = 0   the delay from mc frm en to mc first out data
#define FRC_RO_DBG2_STAT                           0x004f
//Bit 31          reserved
//Bit 30:29       ro_memc_corr_st            // unsigned,   RO, default = 0   1: me_wait_mc_st 2: mc_wait_me_st
//Bit 28:16       ro_memc_corr_dly_num       // unsigned,   RO, default = 0   wait delay
//Bit 15:14       reserved
//Bit 13          ro_out_dly_err             // unsigned,   RO, default = 0   out delay error, the frc output is later than enc DE start
//Bit 12:0        ro_out_de_dly_num          // unsigned,   RO, default = 0   out delay measure, the dly from mc dout to enc DE
#define FRC_TOP_MISC_CTRL                          0x0050
//Bit 31:4        reserved
//Bit 3           reg_be_latch_mode      // unsigned ,  RW, default = 1    1.output  domain ro registers of badedit latch with output interupt 0:latch with  input interupt
//Bit 2           reg_ref_mode           // unsigned ,  RW, default = 1    1:badedit singal latch!#!+
//Bit 1           reg_use_otb_cnt_en     // unsigned ,  RW, default = 1    1:look_up phase_tab from   0 to reg_otb_cnt 0:look_up phase_tab from 0 to 255
//Bit 0           reg_auto_align_en      // unsigned ,  RW, default = 1    1:memc proc size 16 align  0:memc proc size set by FRC_PROC_SIZE_IN
#define FRC_PROC_SIZE                              0x0051
//Bit 31:30       reserved
//Bit 29:16       reg_vsize_proc        // unsigned ,   RW, default = 1080  reg_vsize_proc
//Bit 15:14       reserved
//Bit 13:0        reg_hsize_proc        // unsigned ,   RW, default = 1920  reg_hsize_proc
#define FRC_BE_CLR_FLAG                            0x0054
//Bit 31:1        reserved
//Bit 0           pls_clr_fw_proc_err_flag    // unsigned ,  RW, default = 0,fw proc error flag clr bit
#define FRC_RO_INT_CNT                             0x0055
//Bit 31:29       reserved
//Bit 28:16       ro_out_int_cnt        // unsigned ,   RW, default = 1080 output interupt counter
//Bit 15:13       reserved
//Bit 12:0        ro_inp_int_cnt        // unsigned ,   RW, default = 1920 input  interupt counter
#define FRC_DDR_SLEEP_CTRL                         0x0056
//Bit 31:0        reg_sleep_ctrl_num    // unsigned  ,   RW, default =  32'h8000_0000 reg_sleep_ctrl_num
#define FRC_TOP_GCLK_CTRL                          0x0057
//Bit 31:2        reserved
//Bit 1 :0        reg_bd_reg_gclk_ctrl  // unsigned  ,   RW, default = 0 ,badedit reg gclk
#define FRC_RO_INP_PHS                             0x0058
//Bit 31:8         reserved
//Bit 7 :0         ro_frc_inp_phs       // unsigned ,   RO, default = 0  ro_frc_inp_phs,[0,input_n-1] for n:m mode
#define FRC_RO_FRM_SEC_STAT                        0x0059
//Bit 31:17     reserved
//Bit 16        ro_inp_sec_frm  // unsigned ,   RO,default = 0,current input  frame  is  security frame
//Bit 15:1      reserved
//Bit 0         ro_oup_sec_frm  // unsigned ,   RO,default = 0,current output frame  is  security frame
#define FRC_TOP_INP_RDSEC_CTRL                     0x005a
//Bit 31:18     reserved
//Bit 17:16     reg_inp_rd_sec_foc_en  // unsigned ,   RW,default = 0 , bit0:mc_rd       bit1:me_c_rd
//Bit 15:2      reserved
//Bit 1 :0      reg_inp_rd_sec_foc     // unsigned ,   RW,default = 3 , bit0:mc_rd       bit1:me_c_rd
#define FRC_TOP_ME_RDSEC_CTRL                      0x005b
//Bit 31:17     reserved
//Bit 16        reg_me_rd_sec_foc_en   // unsigned ,   RW,default = 0 , bit0:me_rd
//Bit 15:3      reserved
//Bit 2 :0      reg_me_rd_sec_foc      // unsigned ,   RW,default = 7 , bit0:pre_me_rd   bit1:cur_me_rd bit2:nxt_me_rd
#define FRC_TOP_MC_RDSEC_CTRL                      0x005c
//Bit 31:17     reserved
//Bit 16        reg_mc_rd_sec_foc_en  // unsigned ,   RW,default = 0 , bit0:mc_rd
//Bit 15:2      reserved
//Bit 1 :0      reg_mc_rd_sec_foc     // unsigned ,   RW,default = 3 , bit0:pre_mc_rd   bit1:cur_mc_rd
#define FRC_TOP_MC_HSC_CTRL                        0x005e
//Bit 31:2      reserved
//Bit 1:0       reg_mc_hsc_scale    // unsigned ,  RW, default = 0  downscale mode of x direction for me input data; 0: no downscale; 1:1/2 downscale; 2:1/4 downscale
#define FRC_REG_TOP_RESERVE0                       0x0060
//Bit 31:0        reg_top_reserve0  // unsigned ,   RW, default = 0 reg_top_reserve0 for fpga test
#define FRC_REG_TOP_RESERVE1                       0x0061
//Bit 31:0        reg_top_reserve1  // unsigned ,   RW, default = 0 reg_top_reserve1 for fpga test
#define FRC_REG_TOP_RESERVE2                       0x0062
//Bit 31:0        reg_top_reserve2  // unsigned ,   RW, default = 0 reg_top_reserve2 for fpga test
#define FRC_REG_TOP_RESERVE3                       0x0063
//Bit 31:0        reg_top_reserve3  // unsigned ,   RW, default = 0 reg_top_reserve3 for fpga test
#define FRC_REG_TOP_RESERVE4                       0x0064
//Bit 31:0        reg_top_reserve4  // unsigned ,   RW, default = 0 reg_top_reserve4 for fpga test
#define FRC_REG_TOP_RESERVE5                       0x0065
//Bit 31:0        reg_top_reserve5  // unsigned ,   RW, default = 0 reg_top_reserve5 for fpga test
#define FRC_REG_TOP_RESERVE6                       0x0066
//Bit 31:0        reg_top_reserve6  // unsigned ,   RW, default = 0 reg_top_reserve6 for fpga test
#define FRC_REG_TOP_RESERVE7                       0x0067
//Bit 31:0        reg_top_reserve7  // unsigned ,   RW, default = 0 reg_top_reserve7 for fpga test
#define FRC_REG_TOP_RESERVE8                       0x0068
//Bit 31:0        reg_top_reserve8  // unsigned ,   RW, default = 0 reg_top_reserve8 for fpga test
#define FRC_REG_TOP_RESERVE9                       0x0069
//Bit 31:0        reg_top_reserve9  // unsigned ,   RW, default = 0 reg_top_reserve9 for fpga test
#define FRC_REG_TOP_RESERVE10                      0x006a
//Bit 31:0        reg_top_reserve10 // unsigned ,   RW, default = 0 reg_top_reserve10 for fpga test
#define FRC_REG_TOP_RESERVE11                      0x006b
//Bit 31:0        reg_top_reserve11 // unsigned ,   RW, default = 0 reg_top_reserve11 for fpga test
#define FRC_REG_TOP_RESERVE12                      0x006c
//Bit 31:0        reg_top_reserve12 // unsigned ,   RW, default = 0 reg_top_reserve12 for fpga test
#define FRC_REG_TOP_RESERVE13                      0x006d
//Bit 31:0        reg_top_reserve13 // unsigned ,   RW, default = 0 reg_top_reserve13 for fpga test
#define FRC_REG_TOP_RESERVE14                      0x006e
//Bit 31:0        reg_top_reserve14 // unsigned ,   RW, default = 0 reg_top_reserve14 for fpga test
#define FRC_REG_TOP_RESERVE15                      0x006f
//Bit 31:0        reg_top_reserve15 // unsigned ,   RW, default = 0 reg_top_reserve15 for fpga test
// Closing file:  ./frc_inc/frc_top_regs.h
//
// Reading file:  ./frc_inc/frc_top_sec_regs.h
#define FRC_TOP_SEC_STATUS                         0x0070
//Bit 31:1      reserved
//Bit 0         reg_sec_reg_keep       // unsigned  ,  RW,default = 0    , security registers
#define FRC_TOP_OUT_SEC_CTRL                       0x0071
//Bit 31:17     reserved
//Bit 16        reg_frc_isec_foc_en    // unsigned  ,  RW,default = 0 , output security  force enable
//Bit 15:1      reserved
//Bit 0         reg_frc_isec_foc       // unsigned  ,  RW,default = 1 , output security  force value
#define FRC_TOP_IN_SEC_CTRL                        0x0072
//Bit 31:17     reserved
//Bit 16        reg_frc_osec_foc_en    // unsigned  ,  RW,default = 0 , output security  force enable
//Bit 15:1      reserved
//Bit 0         reg_frc_osec_foc       // unsigned  ,  RW,default = 1 , output security  force value
#define FRC_TOP_INP_SEC_CTRL                       0x0073
//Bit 31:18     reserved
//Bit 17:16     reg_inp_wr_sec_foc_en  // unsigned ,   RW,default = 0 , bit0:mc_wr       bit1:me_c_wr
//Bit 15:2      reserved
//Bit 1 :0      reg_inp_wr_sec_foc     // unsigned ,   RW,default = 3 , bit0:mc_wr       bit1:me_c_wr
// Closing file:  ./frc_inc/frc_top_sec_regs.h
//
// Reading file:  ./frc_inc/frc_top_lut.h
#define FRC_TOP_LUT_ADDR                           0x0080
//Bit  31: 0        frc_top_lut_addr            // unsigned ,    RW, default = 10  frc frame buffer lut addr port
#define FRC_TOP_LUT_DATA                           0x0081
//Bit  31: 0        frc_top_lut_data            // unsigned ,    RW, default = 10  frc frame buffer lut data port
// Closing file:  ./frc_inc/frc_top_lut.h
//
// -----------------------------------------------
// REG_BASE:  FRC_TOP2_APB_BASE = 0x01
// -----------------------------------------------
// Reading file:  ./frc_inc/frc_top_regs2.h
#define FRC_REG_INPUT_SIZE                         0x0100
//Bit 31            reserved
//Bit 30           reg_frc_en                // unsigned ,    RW, default = 0  enable frc
//Bit 29           reg_frc_inp_yuv422        // unsigned ,    RW, default = 0  input is yuv444, 0: yuv444, 1:yuv422, default=1
//Bit 28           reg_frc_ddr_yuv422        // unsigned ,    RW, default = 1  using yuv422 in the ddr storage data, 0: yuv444, 1:yuv422, default=1
//Bit 27:26        reserved
//Bit 25:13        reg_frc_input_xsize       // unsigned ,    RW, default = 1920  horizontal pixel number of input picture. constant4rtl
//Bit 12: 0        reg_frc_input_ysize       // unsigned ,    RW, default = 1080  vertical line number of input picture. constant4rtl
#define FRC_REG_INPUT_FUL_IDX                      0x0101
//Bit 31: 0        ro_frc_input_ful_idx      // unsigned ,    RO, default = 0  sim. latest input frame idx from the input sequence. in load_org_frame_HW, ++.
#define FRC_REG_PAT_POINTER                        0x0102
//Bit 31:24        reg_init_load_num         // unsigned ,    RW, default = 0  for sim only.    load frame num. set by fw in phase_table_core
//Bit 23:16        ro_frc_pat_pointer        // unsigned ,    RO, default = 0  0~reg_frc_pd_pat_num, current input frame phase in pd pattern. in fid_loop, ++.
//Bit 15:13        reserved
//Bit 12           ro_frc_load_frm_flag      // unsigned ,    RO, default = 0  load input frame flag. prm_phase_table[(ofrm_idx+1)%otb_cnt].load_frm_flag.set by HW from lut
//Bit 11: 8        ro_frc_input_fid_p        // unsigned ,    RO, default = 0  input port the frame id just before latest frame in the memory.in load_org_frame_HW, from ro_frc_input_fid
//Bit  7: 4        ro_frc_input_fid          // unsigned ,    RO, default = 0  input port latest frame id in the memory. in load_org_frame_HW, ++reg_ip_incr[reg_film_phase].
//Bit  3: 0        ro_frc_logo_input_fid     // unsigned ,    RO, default = 0  ip logo input port latest frame id in the memory
#define FRC_REG_LOAD_ORG_FRAME_0                   0x0103
//Bit 31:28        reg_ip_incr_0             // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 27           reg_ip_film_end_0         // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 26:24        reserved
//Bit 23:20        reg_ip_incr_1             // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 19           reg_ip_film_end_1         // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 18:16        reserved
//Bit 15:12        reg_ip_incr_2             // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 11           reg_ip_film_end_2         // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 10: 8        reserved
//Bit  7: 4        reg_ip_incr_3             // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit  3           reg_ip_film_end_3         // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit  2: 0        reserved
#define FRC_REG_LOAD_ORG_FRAME_1                   0x0104
//Bit 31:28        reg_ip_incr_4             // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 27           reg_ip_film_end_4         // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 26:24        reserved
//Bit 23:20        reg_ip_incr_5             // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 19           reg_ip_film_end_5         // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 18:16        reserved
//Bit 15:12        reg_ip_incr_6             // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 11           reg_ip_film_end_6         // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 10: 8        reserved
//Bit  7: 4        reg_ip_incr_7             // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit  3           reg_ip_film_end_7         // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit  2: 0        reserved
#define FRC_REG_LOAD_ORG_FRAME_2                   0x0105
//Bit 31:28        reg_ip_incr_8             // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 27           reg_ip_film_end_8         // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 26:24        reserved
//Bit 23:20        reg_ip_incr_9             // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 19           reg_ip_film_end_9         // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 18:16        reserved
//Bit 15:12        reg_ip_incr_10            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 11           reg_ip_film_end_10        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 10: 8        reserved
//Bit  7: 4        reg_ip_incr_11            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit  3           reg_ip_film_end_11        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit  2: 0        reserved
#define FRC_REG_LOAD_ORG_FRAME_3                   0x0106
//Bit 31:28        reg_ip_incr_12            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 27           reg_ip_film_end_12        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 26:24        reserved
//Bit 23:20        reg_ip_incr_13            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 19           reg_ip_film_end_13        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 18:16        reserved
//Bit 15:12        reg_ip_incr_14            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 11           reg_ip_film_end_14        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 10: 8        reserved
//Bit  7: 4        reg_ip_incr_15            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit  3           reg_ip_film_end_15        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit  2: 0        reserved
#define FRC_REG_LOAD_ORG_FRAME_4                   0x0107
//Bit 31:28        reg_ip_incr_16            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 27           reg_ip_film_end_16        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 26:24        reserved
//Bit 23:20        reg_ip_incr_17            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 19           reg_ip_film_end_17        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 18:16        reserved
//Bit 15:12        reg_ip_incr_18            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 11           reg_ip_film_end_18        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 10: 8        reserved
//Bit  7: 4        reg_ip_incr_19            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit  3           reg_ip_film_end_19        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit  2: 0        reserved
#define FRC_REG_LOAD_ORG_FRAME_5                   0x0108
//Bit 31:28        reg_ip_incr_20            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 27           reg_ip_film_end_20        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 26:24        reserved
//Bit 23:20        reg_ip_incr_21            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 19           reg_ip_film_end_21        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 18:16        reserved
//Bit 15:12        reg_ip_incr_22            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 11           reg_ip_film_end_22        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 10: 8        reserved
//Bit  7: 4        reg_ip_incr_23            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit  3           reg_ip_film_end_23        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit  2: 0        reserved
#define FRC_REG_LOAD_ORG_FRAME_6                   0x0109
//Bit 31:28        reg_ip_incr_24            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 27           reg_ip_film_end_24        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 26:24        reserved
//Bit 23:20        reg_ip_incr_25            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 19           reg_ip_film_end_25        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 18:16        reserved
//Bit 15:12        reg_ip_incr_26            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 11           reg_ip_film_end_26        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 10: 8        reserved
//Bit  7: 4        reg_ip_incr_27            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit  3           reg_ip_film_end_27        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit  2: 0        reserved
#define FRC_REG_LOAD_ORG_FRAME_7                   0x010a
//Bit 31:28        reg_ip_incr_28            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 27           reg_ip_film_end_28        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 26:24        reserved
//Bit 23:20        reg_ip_incr_29            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 19           reg_ip_film_end_29        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 18:16        reserved
//Bit 15:12        reg_ip_incr_30            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 11           reg_ip_film_end_30        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 10: 8        reserved
//Bit  7: 4        reg_ip_incr_31            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit  3           reg_ip_film_end_31        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit  2: 0        reserved
#define FRC_REG_LOAD_ORG_FRAME_8                   0x010b
//Bit 31:28        reg_ip_incr_32            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 27           reg_ip_film_end_32        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 26:24        reserved
//Bit 23:20        reg_ip_incr_33            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 19           reg_ip_film_end_33        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 18:16        reserved
//Bit 15:12        reg_ip_incr_34            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 11           reg_ip_film_end_34        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 10: 8        reserved
//Bit  7: 4        reg_ip_incr_35            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit  3           reg_ip_film_end_35        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit  2: 0        reserved
#define FRC_REG_LOAD_ORG_FRAME_9                   0x010c
//Bit 31:28        reg_ip_incr_36            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 27           reg_ip_film_end_36        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 26:24        reserved
//Bit 23:20        reg_ip_incr_37            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 19           reg_ip_film_end_37        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 18:16        reserved
//Bit 15:12        reg_ip_incr_38            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 11           reg_ip_film_end_38        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 10: 8        reserved
//Bit  7: 4        reg_ip_incr_39            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit  3           reg_ip_film_end_39        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit  2: 0        reserved
#define FRC_REG_LOAD_ORG_FRAME_10                  0x010d
//Bit 31:28        reg_ip_incr_40            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 27           reg_ip_film_end_40        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 26:24        reserved
//Bit 23:20        reg_ip_incr_41            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 19           reg_ip_film_end_41        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 18:16        reserved
//Bit 15:12        reg_ip_incr_42            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 11           reg_ip_film_end_42        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 10: 8        reserved
//Bit  7: 4        reg_ip_incr_43            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit  3           reg_ip_film_end_43        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit  2: 0        reserved
#define FRC_REG_LOAD_ORG_FRAME_11                  0x010e
//Bit 31:28        reg_ip_incr_44            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 27           reg_ip_film_end_44        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 26:24        reserved
//Bit 23:20        reg_ip_incr_45            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 19           reg_ip_film_end_45        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 18:16        reserved
//Bit 15:12        reg_ip_incr_46            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 11           reg_ip_film_end_46        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 10: 8        reserved
//Bit  7: 4        reg_ip_incr_47            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit  3           reg_ip_film_end_47        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit  2: 0        reserved
#define FRC_REG_LOAD_ORG_FRAME_12                  0x010f
//Bit 31:28        reg_ip_incr_48            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 27           reg_ip_film_end_48        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 26:24        reserved
//Bit 23:20        reg_ip_incr_49            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 19           reg_ip_film_end_49        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 18:16        reserved
//Bit 15:12        reg_ip_incr_50            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 11           reg_ip_film_end_50        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 10: 8        reserved
//Bit  7: 4        reg_ip_incr_51            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit  3           reg_ip_film_end_51        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit  2: 0        reserved
#define FRC_REG_LOAD_ORG_FRAME_13                  0x0110
//Bit 31:28        reg_ip_incr_52            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 27           reg_ip_film_end_52        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 26:24        reserved
//Bit 23:20        reg_ip_incr_53            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 19           reg_ip_film_end_53        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 18:16        reserved
//Bit 15:12        reg_ip_incr_54            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 11           reg_ip_film_end_54        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 10: 8        reserved
//Bit  7: 4        reg_ip_incr_55            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit  3           reg_ip_film_end_55        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit  2: 0        reserved
#define FRC_REG_LOAD_ORG_FRAME_14                  0x0111
//Bit 31:28        reg_ip_incr_56            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 27           reg_ip_film_end_56        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 26:24        reserved
//Bit 23:20        reg_ip_incr_57            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 19           reg_ip_film_end_57        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 18:16        reserved
//Bit 15:12        reg_ip_incr_58            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 11           reg_ip_film_end_58        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 10: 8        reserved
//Bit  7: 4        reg_ip_incr_59            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit  3           reg_ip_film_end_59        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit  2: 0        reserved
#define FRC_REG_LOAD_ORG_FRAME_15                  0x0112
//Bit 31:28        reg_ip_incr_60            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 27           reg_ip_film_end_60        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 26:24        reserved
//Bit 23:20        reg_ip_incr_61            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 19           reg_ip_film_end_61        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 18:16        reserved
//Bit 15:12        reg_ip_incr_62            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit 11           reg_ip_film_end_62        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit 10: 8        reserved
//Bit  7: 4        reg_ip_incr_63            // unsigned ,    RW, default = 0  used in load_org_frame, reg_ip_incr[reg_film_phase].set by fw in phase_table_core
//Bit  3           reg_ip_film_end_63        // unsigned ,    RW, default = 0  used in load_org_frame, when (reg_ip_film_end[reg_film_phase]==1).set by fw in phase_table_core
//Bit  2: 0        reserved
#define FRC_REG_OUT_FID                            0x0113
//Bit 31:24        reg_otb_cnt               // unsigned ,    RW, default = 0  for sim. output period table count, for ro_frc_load_frm_flag to drive input load_org_frame_HW, set by fw in phase_table_core.
//Bit 23:21        reserved
//Bit 20           ro_frc_otb_start          // unsigned ,    RO, default = 0  when 1, delay the mv frame buffers. prm_phase_table[index].data_lut.phase_start;set by HW from lut
//Bit 19:16        ro_frc_opre_fid           // unsigned ,    RO, default = 0  output phase previous (left) frame id in the memory;
//Bit 15:12        ro_frc_ocur_fid           // unsigned ,    RO, default = 0  output phase current (right) frame id in the memory;
//Bit 11: 8        ro_frc_onex_fid           // unsigned ,    RO, default = 0  output phase future  (right right) frame id in the memory
//Bit  7: 4        ro_frc_opre_logo_fid      // unsigned ,    RO, default = 0  phase previous (left) logo frame-id in the memory
//Bit  3: 0        ro_frc_ocur_logo_fid      // unsigned ,    RO, default = 0  phase current (right)logo frame-id in the memory
#define FRC_REG_OUT_PHS                            0x0114
//Bit 31:24        reserved
//Bit 23:12        ro_frc_output_phase             // unsigned ,    RO, default = 0  output phase between opre and ocur frames (0~4095) 12 bits
//Bit 11: 0        ro_frc_output_phase_me          // unsigned ,    RO, default = 0  output phase between opre and ocur frames (0~4095) 12 bits. for ME and VP.set by HW from lut
#define FRC_REG_OUT_FRAME_IDX                      0x0115
//Bit 31: 0        ro_frc_ofrm_idx           // unsigned ,    RO, default = 0  for sim ONLY. frame number index of the output frame. in main, ++
#define FRC_REG_PHS_TABLE                          0x0116
//Bit 31:24        reg_frc_input_n           // unsigned ,    RW, default = 2  input_n, constant4rtl
//Bit 23:16        reg_frc_output_m          // unsigned ,    RW, default = 5  output_m, constant4rtl
//Bit 15: 8        reg_frc_film_mode         // unsigned ,    RW, default = 0  film mode for firmware. 5bits should be enough. constant4rtl
//Bit  7: 0        reg_frc_film_mode_fw      // unsigned ,    RW, default = 0  film mode for firmware. 5bits should be enough.
#define FRC_REG_FILM_PHS_1                         0x0117
//Bit 31:24        reg_film_phase_fw         // unsigned ,    RW, default = 0  film_phase index. set by fw: fd_fwdet_core
//Bit 23:18        reserved
//Bit 17           reg_film_mix_mode_fw      // unsigned ,    RW, default = 0  film mix mode
//Bit 16           reg_film_hwfw_sel         // unsigned ,    RW, default = 0  0: use hw for film det; 1: use fw for film det
//Bit 15: 8        ro_frc_film_mode_hw       // unsigned ,    RO, default = 0  0: video; 1:22; 2:32; 3: 3223; 4: 2224; 5:32322; 6:44; 7:21111; 8:23322; 9:2111; 10 22224; 11: 33, 12:334; 13:55; 14: 64; 15:66; 16:87; 17:212;
//Bit  7: 0        ro_film_phase_hw          // unsigned ,    RO, default = 0  film_phase index. set by hw: fd_hwdet_core
#define FRC_REG_FILM_PHS_2                         0x0118
//Bit 31:24        ro_frc_frc_phase          // unsigned ,    RO, default = 0  output frc phase index. ro_frc_ofrm_idx%output_m. set by HW in update_ofrm_info_HW.
//Bit 23:16        ro_film_phase             // unsigned ,    RO, default = 0  film_phase index. set by fw: fd_fwdet_core
//Bit 15: 8        reserved
//Bit  7: 2        reserved
//Bit  1           ro_clear_vbuffer_trigger  // unsigned ,    RO, default = 0  signal to trigger clear vector buffer function.
//Bit  0           ro_clear_vbuffer_en       // unsigned ,    RO, default = 0  signal to clear vector buffer.
#define FRC_REG_BLACKBAR_XYXY_ST                   0x0119
//Bit 31:16        reg_blackbar_xyxy_0       // unsigned ,    RW, default = 0  blackbar x st
//Bit 15: 0        reg_blackbar_xyxy_1       // unsigned ,    RW, default = 0  blackbar y st
#define FRC_REG_BLACKBAR_XYXY_ED                   0x011a
//Bit 31:16        reg_blackbar_xyxy_2       // unsigned ,    RW, default = 1919  blackbar x ed
//Bit 15: 0        reg_blackbar_xyxy_3       // unsigned ,    RW, default = 1079  blackbar y ed
#define FRC_REG_BLACKBAR_XYXY_ME_ST                0x011b
//Bit 31:16        reg_blackbar_xyxy_me_0    // unsigned ,    RW, default = 0  me blackbar x st
//Bit 15: 0        reg_blackbar_xyxy_me_1    // unsigned ,    RW, default = 0  me blackbar y st
#define FRC_REG_BLACKBAR_XYXY_ME_ED                0x011c
//Bit 31:16        reg_blackbar_xyxy_me_2    // unsigned ,    RW, default = 1919  me blackbar x ed
//Bit 15: 0        reg_blackbar_xyxy_me_3    // unsigned ,    RW, default = 1079  me blackbar y ed
#define FRC_REG_DEMOWINDOW1_XYXY_ST                0x011d
//Bit 31:16        reg_demowindow1_xyxy_0    // unsigned ,    RW, default = 0  demowindow1 x st
//Bit 15: 0        reg_demowindow1_xyxy_1    // unsigned ,    RW, default = 0  demowindow1 y st
#define FRC_REG_DEMOWINDOW1_XYXY_ED                0x011e
//Bit 31:16        reg_demowindow1_xyxy_2    // unsigned ,    RW, default = 1919  demowindow1 x ed
//Bit 15: 0        reg_demowindow1_xyxy_3    // unsigned ,    RW, default = 1079  demowindow1 y ed
#define FRC_REG_DEMOWINDOW2_XYXY_ST                0x011f
//Bit 31:16        reg_demowindow2_xyxy_0    // unsigned ,    RW, default = 0  demowindow2 x st
//Bit 15: 0        reg_demowindow2_xyxy_1    // unsigned ,    RW, default = 0  demowindow2 y st
#define FRC_REG_DEMOWINDOW2_XYXY_ED                0x0120
//Bit 31:16        reg_demowindow2_xyxy_2    // unsigned ,    RW, default = 1919  demowindow2 x ed
//Bit 15: 0        reg_demowindow2_xyxy_3    // unsigned ,    RW, default = 1079  demowindow2 y ed
#define FRC_REG_DEMOWINDOW3_XYXY_ST                0x0121
//Bit 31:16        reg_demowindow3_xyxy_0    // unsigned ,    RW, default = 0  demowindow3 x st
//Bit 15: 0        reg_demowindow3_xyxy_1    // unsigned ,    RW, default = 0  demowindow3 y st
#define FRC_REG_DEMOWINDOW3_XYXY_ED                0x0122
//Bit 31:16        reg_demowindow3_xyxy_2    // unsigned ,    RW, default = 1919  demowindow3 x ed
//Bit 15: 0        reg_demowindow3_xyxy_3    // unsigned ,    RW, default = 1079  demowindow3 y ed
#define FRC_REG_DEMOWINDOW4_XYXY_ST                0x0123
//Bit 31:16        reg_demowindow4_xyxy_0    // unsigned ,    RW, default = 0  demowindow4 x st
//Bit 15: 0        reg_demowindow4_xyxy_1    // unsigned ,    RW, default = 0  demowindow4 y st
#define FRC_REG_DEMOWINDOW4_XYXY_ED                0x0124
//Bit 31:16        reg_demowindow4_xyxy_2    // unsigned ,    RW, default = 1919  demowindow4 x ed
//Bit 15: 0        reg_demowindow4_xyxy_3    // unsigned ,    RW, default = 1079  demowindow4 y ed
#define FRC_LOGO_BB_LFT_TOP                        0x0125
//Bit 31:16        reg_logo_bb_xyxy_0        // unsigned ,    RW, default = 0  logo lft,   dft0
//Bit 15: 0        reg_logo_bb_xyxy_1        // unsigned ,    RW, default = 0  logo top,   dft0
#define FRC_LOGO_BB_RIT_BOT                        0x0126
//Bit 31:16        reg_logo_bb_xyxy_2        // unsigned ,    RW, default = 1919  logo rit,   dft xsize - 1
//Bit 15: 0        reg_logo_bb_xyxy_3        // unsigned ,    RW, default = 1079  logo bot,   dft ysize - 1
#define FRC_REG_MELOGO_BB_BLK_XYXY_ST              0x0127
//Bit 31:16        reg_melogo_bb_blk_xyxy_0  // unsigned ,    RW, default = 0  melogo x st
//Bit 15: 0        reg_melogo_bb_blk_xyxy_1  // unsigned ,    RW, default = 0  melogo y st
#define FRC_REG_MELOGO_BB_BLK_XYXY_ED              0x0128
//Bit 31:16        reg_melogo_bb_blk_xyxy_2  // unsigned ,    RW, default = 239  melogo x ed
//Bit 15: 0        reg_melogo_bb_blk_xyxy_3  // unsigned ,    RW, default = 134  melogo y ed
#define FRC_REG_DS_WIN_SETTING_LFT_TOP             0x0129
//Bit 31:16        reg_bb_ds_xyxy_0          // unsigned ,    RW, default = 0  ds lft posi,    dft0
//Bit 15: 0        reg_bb_ds_xyxy_1          // unsigned ,    RW, default = 0  ds top posi,    dft0
#define FRC_BBD_DS_WIN_SETTING_RIT_BOT             0x012a
//Bit 31:16        reg_bb_ds_xyxy_2          // unsigned ,    RW, default = 1919  ds rit posi,    dft xsize-1
//Bit 15: 0        reg_bb_ds_xyxy_3          // unsigned ,    RW, default = 1079  ds bot posi,    dft ysize-1
#define FRC_REG_BLK_SIZE_XY                        0x012b
//Bit 31:26        reserved
//Bit 25           reg_me_logo_dsx_ratio     // unsigned ,    RW, default = 0  0: ME_image size/logo_image_size =1; 1: ME_image size/logo_image_size =2 (me_image_size=1920*1080);
//Bit 24           reg_me_logo_dsy_ratio     // unsigned ,    RW, default = 0  0: ME_image size/logo_image_size =1; 1: ME_image size/logo_image_size =2 (me_image_size=1920*1080);
//Bit 23:22        reserved
//Bit 21:19        reg_me_blksize_x          // unsigned ,    RW, default = 2  block size x(2^reg) in ME submodule under the Buf_blend data (downsampled); default = 2;
//Bit 18:16        reg_me_blksize_y          // unsigned ,    RW, default = 2  block size y(2^reg) in ME submodule under the Buf_blend data (downsampled); default = 2;
//Bit 15:14        reg_me_mvx_div_mode       // unsigned ,    RW, default = 0  0: MVx has 2bits decimal; 1: 3bits decimal; 2: 4bits decimal @ME resolution.
//Bit 13:12        reg_me_mvy_div_mode       // unsigned ,    RW, default = 0  0: MVy has 2bits decimal; 1: 3bits decimal; 2: 4bits decimal @ME resolution.
//Bit 11: 0        reserved
#define FRC_REG_BLK_SCALE                          0x012c
//Bit 31:20        reserved
//Bit 19:18        reg_osd_logo_ratio        // unsigned ,    RW, default = 1  ratio for osd to logo. 0, 1:1; 1, 1:2; 2, 1:4; 3, 1:8
//Bit 17:14        reg_osd_logo_ratio_th     // unsigned ,    RW, default = 1  aggregation threshold for osd alpha bit to iplogo path.
//Bit 13:12        reg_ip_blklogo_mode       // unsigned ,    RW, default = 2  0: fBuf_ip_blklogo; 1: fBuf_ip_blklogo_sync[frc_opre_logo_fid]; 2: fBuf_ip_blklogo_sync[frc_ocur_logo_fid]; 3: and.
//Bit 11: 9        reg_mc_blksize_xscale     // unsigned ,    RW, default = 3  (0~4), mc block horizontal size in full pixel scale = (1x2^xscal), set to (reg_me_dsx_scale + 2) as default
//Bit  8: 6        reg_mc_blksize_yscale     // unsigned ,    RW, default = 3  (0~4), mc block vertical size in full pixel scale = (1x2^yscal), set to (reg_me_dsy_scale + 2) as default
//Bit  5: 4        reg_logo_mc_ratio         // unsigned ,    RW, default = 1  0, 1:1; 1, 1:2; 2, 1:4; 3, 1:8
//Bit  3: 2        reserved
//Bit  1           reg_iplogo_osdmerge       // unsigned ,    RW, default = 0  0: do iplogo only at new original frame input. 1: do the last two stages of iplogo at every frame. work with reg_iplogo_osdbit.
//Bit  0           reg_iplogo_osdmode        // unsigned ,    RW, default = 0  0: use separate logo fid for both input/output.1: use mc fid for logo osdmode for both input/output.
#define FRC_REG_ME_SCALE                       0x012d
//Bit 31: 8        reserved
//Bit  7: 6        reg_me_dsx_scale          // unsigned ,    RW, default = 1  downscale mode of x direction for me input data; 0: no downscale; 1:1/2 downscale; 2:1/4 downscale
//Bit  5: 4        reg_me_dsy_scale          // unsigned ,    RW, default = 1  downscale mode of x direction for me input data; 0: no downscale; 1:1/2 downscale; 2:1/4 downscale
//Bit  3: 0        reserved
#define FRC_REG_CURSOR                             0x012e
//Bit 31:29        reserved
//Bit 28           reg_cursor_ipdisp_en      // unsigned ,    RW, default = 0  for ip cursor display: enable
//Bit 27:20        reg_cursor_ipdisp_color   // unsigned ,    RW, default = 200  for ip cursor display: luma
//Bit 19:17        reserved
//Bit 16           reg_cursor_mcdisp_en      // unsigned ,    RW, default = 0  for mc cursor display: enable
//Bit 15:11        reserved
//Bit 10: 8        reg_cursor_mcdisp_color   // unsigned ,    RW, default = 0  for mc cursor display: luma
//Bit  7: 0        reserved
#define FRC_REG_IP_CURSOR_0                        0x012f
//Bit 31:23        reserved
//Bit 22:13        ro_cursor_ip_0            // unsigned ,    RO, default = 0  for ip cursor readback.
//Bit 12: 0        reg_cursor_ip_coordi_0    // unsigned ,    RW, default = 0  for ip cursor readback: [0]:col; [1]:row
#define FRC_REG_IP_CURSOR_1                        0x0130
//Bit 31:23        reserved
//Bit 22:13        ro_cursor_ip_1            // unsigned ,    RO, default = 0  for ip cursor readback.
//Bit 12: 0        reg_cursor_ip_coordi_1    // unsigned ,    RW, default = 0  for ip cursor readback: [0]:col; [1]:row
#define FRC_REG_MC_CURSOR_0                        0x0131
//Bit 31:23        reserved
//Bit 22:13        ro_cursor_mc_0            // unsigned ,    RO, default = 0  for mc cursor readback.
//Bit 12: 0        reg_cursor_mc_coordi_0    // unsigned ,    RW, default = 0  for mc cursor readback: [0]:col; [1]:row
#define FRC_REG_MC_CURSOR_1                        0x0132
//Bit 31:23        reserved
//Bit 22:13        ro_cursor_mc_1            // unsigned ,    RO, default = 0  for mc cursor readback.
//Bit 12: 0        reg_cursor_mc_coordi_1    // unsigned ,    RW, default = 0  for mc cursor readback: [0]:col; [1]:row
#define FRC_REG_MC_CURSOR_2                        0x0133
//Bit 31:23        reserved
//Bit 22:13        ro_cursor_mc_2            // unsigned ,    RO, default = 0  for mc cursor readback.
//Bit 12: 0        reg_cursor_mc_coordi_2    // unsigned ,    RW, default = 0  for mc cursor readback: [0]:col; [1]:row
#define FRC_REG_DEBUG_PATH_TOP_BOT_MOTION_POSI2    0x0134
//Bit 31:16        reg_debug_top_motion_posi2 // unsigned ,    RW, default = 0  top motion posi2
//Bit 15: 0        reg_debug_bot_motion_posi2 // unsigned ,    RW, default = 1079  bot motion posi2
#define FRC_REG_DEBUG_PATH_LFT_RIT_MOTION_POSI2    0x0135
//Bit 31:16        reg_debug_lft_motion_posi2 // unsigned ,    RW, default = 0  lft motion posi2
//Bit 15: 0        reg_debug_rit_motion_posi2 // unsigned ,    RW, default = 1919  rit motion posi2
#define FRC_REG_DEBUG_PATH_TOP_BOT_MOTION_POSI1    0x0136
//Bit 31:16        reg_debug_top_motion_posi1 // unsigned ,    RW, default = 0  top motion posi1
//Bit 15: 0        reg_debug_bot_motion_posi1 // unsigned ,    RW, default = 1079  bot motion posi1
#define FRC_REG_DEBUG_PATH_LFT_RIT_MOTION_POSI1    0x0137
//Bit 31:16        reg_debug_lft_motion_posi1 // unsigned ,    RW, default = 0  lft motion posi1
//Bit 15: 0        reg_debug_rit_motion_posi1 // unsigned ,    RW, default = 1919  rit motion posi1
#define FRC_REG_DEBUG_PATH_TOP_BOT_EDGE_POSI2      0x0138
//Bit 31:16        reg_debug_top_edge_posi2  // unsigned ,    RW, default = 0  top edge posi2
//Bit 15: 0        reg_debug_bot_edge_posi2  // unsigned ,    RW, default = 1079  bot edge posi2
#define FRC_REG_DEBUG_PATH_LFT_RIT_EDGE_POSI2      0x0139
//Bit 31:16        reg_debug_lft_edge_posi2  // unsigned ,    RW, default = 0  lft edge posi2
//Bit 15: 0        reg_debug_rit_edge_posi2  // unsigned ,    RW, default = 1919  rit edge posi2
#define FRC_REG_DEBUG_PATH_TOP_BOT_EDGE_POSI1      0x013a
//Bit 31:16        reg_debug_top_edge_posi1  // unsigned ,    RW, default = 0  top edge posi1
//Bit 15: 0        reg_debug_bot_edge_posi1  // unsigned ,    RW, default = 1079  bot edge posi1
#define FRC_REG_DEBUG_PATH_LFT_RIT_EDGE_POSI1      0x013b
//Bit 31:16        reg_debug_lft_edge_posi1  // unsigned ,    RW, default = 0  lft edge posi1
//Bit 15: 0        reg_debug_rit_edge_posi1  // unsigned ,    RW, default = 1919  rit edge posi1
#define FRC_REG_DEBUG_FALSE_COLOR                  0x013c
//Bit 31: 2        reserved
//Bit  1: 0        reg_debug_false_color_mv_show_mode // unsigned ,    RW, default = 0  0, mvx on cb mvy on cr, 1 mvx on luma,2 mvy on luma, 3 abs mvx + abs mvy avg on luma, default 0
#define FRC_REG_ME_DEBUG1                          0x013d
//Bit 31           reg_debug_path_en         // unsigned ,    RW, default = 0
//Bit 30           reg_me_debug_cn_fs_en     // unsigned ,    RW, default = 0  enable signal of debug cn full search mv, 0: disable, 1:enable
//Bit 29           reg_me_debug_nc_fs_en     // unsigned ,    RW, default = 0  enable signal of debug nc full search mv, 0: disable, 1:enable
//Bit 28:26        reg_me_debug_pc_prj_mode  // unsigned ,    RW, default = 0  enable signal of debug pc proj mv , 0: disable, 1: prj mv0, 2: prj mv1, 3: prj mv2, 4: prj mv3
//Bit 25:23        reg_me_debug_cp_prj_mode  // unsigned ,    RW, default = 0  enable signal of debug cp proj mv , 0: disable, 1: prj mv0, 2: prj mv1, 3: prj mv2, 4: prj mv3
//Bit 22:20        reg_me_debug_pc_sad_mode  // unsigned ,    RW, default = 0  enable signal of debug sad, 0: disable ,1:pc_phs_mv sad,2:weighted sad_dc, 3:weighted sad_ac,4:pc_phs_mv.sad_4x4
//Bit 19:17        reg_me_debug_cn_sad_mode  // unsigned ,    RW, default = 0  enable signal of debug sad, 0: disable ,1:cn_uni_mv sad,2:weighted sad_dc, 3:weighted sad_ac,4:cn_uni_mv.sad_4x4
//Bit 16:14        reg_me_debug_nc_sad_mode  // unsigned ,    RW, default = 0  enable signal of debug sad, 0: disable ,1:nc_uni_mv sad,2:weighted sad_dc, 3:weighted sad_ac,4:nc_uni_mv.sad_4x4
//Bit 13:11        reg_me_debug_sad_div      // unsigned ,    RW, default = 2  the bits to clip,0:255 clip,1:511 clip,2:1023 clip
//Bit 10           reg_me_debug_pc_acdc_flg_en // unsigned ,    RW, default = 0  enable signal of debug acdc flag, 0: disable, 1:enable
//Bit  9           reg_me_debug_cn_acdc_flg_en // unsigned ,    RW, default = 0  enable signal of debug acdc flag, 0: disable, 1:enable
//Bit  8           reg_me_debug_nc_acdc_flg_en // unsigned ,    RW, default = 0  enable signal of debug acdc flag, 0: disable, 1:enable
//Bit  7            reserved
//Bit  6           reg_me_debug_pc_smooth0_en // unsigned ,    RW, default = 0  enable signal of debug mv_smooth0 func, 0: disable, 1:enable
//Bit  5           reg_me_debug_cn_smooth0_en // unsigned ,    RW, default = 0  enable signal of debug mv_smooth0 func, 0: disable, 1:enable
//Bit  4           reg_me_debug_nc_smooth0_en // unsigned ,    RW, default = 0  enable signal of debug mv_smooth0 func, 0: disable, 1:enable
//Bit  3            reserved
//Bit  2           reg_me_debug_pc_smooth1_en // unsigned ,    RW, default = 0  enable signal of debug mv_smooth1 func, 0: disable, 1:enable
//Bit  1           reg_me_debug_cn_smooth1_en // unsigned ,    RW, default = 0  enable signal of debug mv_smooth1 func, 0: disable, 1:enable
//Bit  0           reg_me_debug_nc_smooth1_en // unsigned ,    RW, default = 0  enable signal of debug mv_smooth1 func, 0: disable, 1:enable
#define FRC_REG_ME_DEBUG2                          0x013e
//Bit 31:24        reserved
//Bit 23           reg_me_debug_pc_smooth2_en // unsigned ,    RW, default = 0  enable signal of debug mv_smooth2 func, 0: disable, 1:enable
//Bit 22           reg_me_debug_cn_smooth2_en // unsigned ,    RW, default = 0  enable signal of debug mv_smooth2 func, 0: disable, 1:enable
//Bit 21           reg_me_debug_nc_smooth2_en // unsigned ,    RW, default = 0  enable signal of debug mv_smooth2 func, 0: disable, 1:enable
//Bit 20           reg_me_debug_pc_smobj_en  // unsigned ,    RW, default = 0  enable signal of debug limit func, 0: disable, 1:enable
//Bit 19            reserved
//Bit 18           reg_me_debug_pc_smooth3_en // unsigned ,    RW, default = 0  enable signal of debug mv_smooth2 func, 0: disable, 1:enable
//Bit 17           reg_me_debug_cn_smooth3_en // unsigned ,    RW, default = 0  enable signal of debug mv_smooth2 func, 0: disable, 1:enable
//Bit 16           reg_me_debug_nc_smooth3_en // unsigned ,    RW, default = 0  enable signal of debug mv_smooth2 func, 0: disable, 1:enable
//Bit 15            reserved
//Bit 14           reg_me_debug_pc_limit_en  // unsigned ,    RW, default = 0  enable signal of debug limit func, 0: disable, 1:enable
//Bit 13           reg_me_debug_cn_limit_en  // unsigned ,    RW, default = 0  enable signal of debug limit func, 0: disable, 1:enable
//Bit 12           reg_me_debug_nc_limit_en  // unsigned ,    RW, default = 0  enable signal of debug limit func, 0: disable, 1:enable
//Bit 11            reserved
//Bit 10: 8        reg_me_debug_raw_rp_flg_mode // unsigned ,    RW, default = 0  enable signal of debug raw_rp_flg, 0: disable,1:flg,2:t1_flg,3:t2_flg_strict,4:t2_flg,5:t3_flg
//Bit  7           reg_me_debug_fine_rp_flg_en // unsigned ,    RW, default = 0  enable signal of debug fine_rp_flg, 0: disable , 1:enable
//Bit  6           reg_me_debug_final_rp_flg_en // unsigned ,    RW, default = 0  enable signal of debug final_rp_flg, 0: disable , 1:enable
//Bit  5           reg_me_debug_line_flg_en  // unsigned ,    RW, default = 0  enable signal of debug line_flg, 0: disable , 1:enable
//Bit  4           reg_me_debug_map_drt_en   // unsigned ,    RW, default = 0  enable signal of debug map_drt, 0: disable , 1:enable
//Bit  3            reserved
//Bit  2           reg_me_debug_auto_flat_flg_en // unsigned ,    RW, default = 0  enable signal of debug auto_flag_flg, 0: disable , 1:enable
//Bit  1           reg_me_debug_hard_ab_flg_en // unsigned ,    RW, default = 0  enable signal of debug hard_ab_flg, 0: disable , 1:enable
//Bit  0           reg_me_debug_fs_en_flg_en // unsigned ,    RW, default = 0  enable signal of debug fs_en_flg, 0: disable , 1:enable
#define FRC_REG_ME_DEBUG3                          0x013f
//Bit 31:22        reserved
//Bit 21:20        reg_me_debug_sad_surface_mode // unsigned ,    RW, default = 0  enable signal of debug auto_sad_surface, 0:disable,1:40*24,2:80*48,3:160*96
//Bit 19:12        reg_me_debug_sad_surface_row // unsigned ,    RW, default = 0  row of debug auto sad surface
//Bit 11: 0        reg_me_debug_sad_surface_col // unsigned ,    RW, default = 0  col of debug auto sad surface
#define FRC_REG_VP_DEBUG1                          0x0140
//Bit 31:20        reserved
//Bit 19           reg_vp_debug_occl_type_en // unsigned ,    RW, default = 0  enable of occl_type
//Bit 18           reg_vp_debug_cpt_8_cn_flg_en // unsigned ,    RW, default = 0  enable of cpt_8_cp_flg
//Bit 17           reg_vp_debug_cnt_8_cp_flg_en // unsigned ,    RW, default = 0  enable of cnt_8_cp_flg
//Bit 16:13        reg_vp_debug_retimer_mvs_mode // unsigned ,    RW, default = 0  enable of retimer,1:NC,2:CN,3:CN2NCr,4:CN2NCrr,5:CP,6:CP2PCr,7:CP2PCrr,8:CN2PCr,9:CP2NCr
//Bit 12: 8        reg_vp_debug_dehalo_mvs_mode // unsigned ,    RW, default = 0  enable of dehalo,1:PC_PHS,2:CP,3:CPr,4:CPrr,5:CPr2PCr,6:CPrr2PCr,7:CPr4PBr,8:CPrr4PBr,9:avgCPr4PBr,10:PC,11:PCr,12:PCrr,13:PCr2CPr,14:PCrr2CPr,15:PCr4CNr,16:PCrr4CNr,17:avgPCrCNr
//Bit  7           reg_vp_debug_cover_mv_phs_en // unsigned ,    RW, default = 0  enable of cover mv phs
//Bit  6           reg_vp_debug_uncov_mv_phs_en // unsigned ,    RW, default = 0  enable of uncov mv phs
//Bit  5           reg_vp_debug_occl_fhri_en // unsigned ,    RW, default = 0  enable of occl_fhri
//Bit  4           reg_vp_debug_type_replace_phs_mv_en // unsigned ,    RW, default = 0  enable of type replace phs mv
//Bit  3           reg_vp_debug_phs_rp_flg_en // unsigned ,    RW, default = 0  enable of phs rp flg en
//Bit  2           reg_vp_debug_phs_sobj_flg_en // unsigned ,    RW, default = 0  enable of phs sobj flg en
//Bit  1           reg_mv_debug_var_level_en // unsigned ,    RW, default = 0  enable of var level debug
//Bit  0           reg_mv_debug_var2_level_en // unsigned ,    RW, default = 0  enable of var2 level debug
#define FRC_REG_MC_DEBUG1                          0x0141
//Bit 31:24        reserved
//Bit 23           reg_mc_debug_show_blksize_en // unsigned ,    RW, default = 0  enable of show blksize
//Bit 22           reg_mc_debug_show_grid_en // unsigned ,    RW, default = 0  enable of show grid
//Bit 21           reg_mc_debug_show_bbd_en  // unsigned ,    RW, default = 0  enable of show bbd
//Bit 20:17        reg_mc_debug_show_demowindow_mode // unsigned ,    RW, default = 0  enable of show demowindow 4321
//Bit 16           reg_mc_debug_show_deflicker_pix_en // unsigned ,    RW, default = 0  enable of show deflicker block en
//Bit 15           reg_mc_debug_show_pts_en  // unsigned ,    RW, default = 0  enable of show pts
//Bit 14:12        reg_mc_debug_show_pts_mode // unsigned ,    RW, default = 0  0: p_pts_y 1: p_pts_c 2: c_pts_y 3: c_pts_c 4: p_pts_y and p_pts_y 5:c_pts_c and c_pts_c
//Bit 11           reg_mc_debug_show_ptb_en  // unsigned ,    RW, default = 0  enable of show ptb
//Bit 10: 8        reg_mc_debug_show_ptb_mode // unsigned ,    RW, default = 0  0: p_ptb_y 1: p_ptb_c 2: c_ptb_y 3: c_ptb_c 4: p_ptb_y and p_ptb_y 5:c_ptb_c and c_ptb_c
//Bit  7           reg_mc_debug_show_ptl_en  // unsigned ,    RW, default = 0  enable of show ptl
//Bit  6: 4        reg_mc_debug_show_ptl_mode // unsigned ,    RW, default = 0  0: p_ptl 1: c_ptl 2: p_ptl or c_ptl
//Bit  3           reg_mc_debug_show_ptw_en  // unsigned ,    RW, default = 0  enable of show ptw
//Bit  2: 0        reg_mc_debug_show_ptw_mode // unsigned ,    RW, default = 0  0: p_ptl 1: c_ptl 2: p_ptl or c_ptl
#define FRC_LOGO_DEBUG                             0x0142
//Bit 31:20        reserved
//Bit 19           reg_logo_debug_ip_pix_logo_en // unsigned ,    RW, default = 0
//Bit 18           reg_logo_debug_ip_blk_logo_en // unsigned ,    RW, default = 0
//Bit 17           reg_logo_debug_me_blk_logo_en // unsigned ,    RW, default = 0
//Bit 16           reg_logo_debug_mc_logo_en // unsigned ,    RW, default = 0
//Bit 15           reg_iplogo_inner_pxl_debug_en // unsigned ,    RW, default = 0  dft=0; 0: close inner debug, show ip_pxllogo result;   1: open inner debug, show inner pxl signal in iplogo
//Bit 14:10        reg_iplogo_inner_pxl_debug_mode // unsigned ,    RW, default = 0  dft=0; MUX many inner pxl signals in iplogo
//Bit  9           reg_iplogo_pxl_fid_mode   // unsigned ,    RW, default = 0  0, pre, 1, cur
//Bit  8           reg_iplogo_inner_blk_debug_en // unsigned ,    RW, default = 0  dft=0; 0: close inner debug, show ip_blklogo result;   1: open inner debug, show inner blk signal in iplogo
//Bit  7: 3        reg_iplogo_inner_blk_debug_mode // unsigned ,    RW, default = 0  dft=0; MUX many inner blk signals in iplogo
//Bit  2           reg_melogo_inner_debug_en // unsigned ,    RW, default = 0  0: close inner debug, show me_blklogo result;   1: open inner debug, show inner signal in melogo
//Bit  1: 0        reg_melogo_inner_debug_mode // unsigned ,    RW, default = 0  0: show ip_blklogo;  1: show smv_clr_flag, 2: show pan_clr_flag, 3:  show disap_clr_flag;
#define FRC_BBD_DEBUG_LINE_EN                      0x0143
//Bit 31: 4        reserved
//Bit  3           reg_bbd_debug_motion_line1_en // unsigned ,    RW, default = 0  debug motion line 1 en
//Bit  2           reg_bbd_debug_motion_line2_en // unsigned ,    RW, default = 0  debug motion line2 en
//Bit  1           reg_bbd_debug_edge_line1_en // unsigned ,    RW, default = 0  debug edge line1 en
//Bit  0           reg_bbd_debug_edge_line2_en // unsigned ,    RW, default = 0  debug edge line2 en
#define FRC_FRAME_BUFFER_NUM                       0x0144
//Bit 31:16        reserved
//Bit 15:13        reserved
//Bit 12: 8        reg_logo_fb_num           // unsigned ,    RW, default = 10  logo frame buffer number
//Bit  7: 5        reserved
//Bit  4: 0        reg_frc_fb_num            // unsigned ,    RW, default = 10  frc frame buffer number
#define FRC_REG_DEBUG_PATH_MV                      0x0145
//Bit 31: 4        reserved
//Bit  3            reserved
//Bit  2           reg_mc_debug_oct1_en      // unsigned ,    RW, default = 0  show oct1 flag
//Bit  1            reserved
//Bit  0           reg_mc_debug_disp_pht1_en // unsigned ,    RW, default = 0  show pht mvx1 mvy1
// Closing file:  ./frc_inc/frc_top_regs2.h
//
// Reading file:  ./frc_inc/frc_badedit_regs.h
#define FRC_REG_FWD_PHS                            0x0146
//Bit 31:30        reserved
//Bit 29           reg_frc_phs_dec_sel       // unsigned ,    RW, default = 0  0 :HW lut method, 1 : HW auto calculation method, for Bad-edit
//Bit 28           reg_frc_logo_buf_ctrl     // unsigned ,    RW, default = 0  0 :HW lut method for logo id, 1 : HW auto calculation method for logo id, for Bad-edit
//Bit 27:26        reserved
//Bit 25           ro_frc_load_frm_flag_mux  // unsigned ,    RO, default = 0  load input frame flag. set by fw
//Bit 24           ro_frc_otb_start_mux      // unsigned ,    RO, default = 0  1, delay the mv frame buffers. prm_phase_table[index].data_lut.phase_start;set by fw from
//Bit 23:12        reg_frc_phase            // unsigned ,    RW, default = 0  phase between opre and ocur frames (0~4095) 12 bits  set by fw
//Bit 11: 0        reg_frc_phase_me         // unsigned ,    RW, default = 0  phase between opre and ocur frames (0~4095) 12 bits. for ME and VP.set by fw
#define FRC_REG_FWD_FID                            0x0147
//Bit 31:28        reg_frc_input_fid_p       // unsigned ,    RW, default = 0  input port the frame id just before latest frame in the memory.fw cal,from reg_frc_input_fid
//Bit 27:24        reg_frc_input_fid         // unsigned ,    RW, default = 3  input port latest frame id in the memory. in fw cal
//Bit 23:20        reg_frc_logo_input_fid    // unsigned ,    RW, default = 0  ip logo input port latest frame id in the memory. when (reg_ip_film_end[reg_film_phase]==1), ++.
//Bit 19:16        reg_frc_opre_logo_fid     // unsigned ,    RW, default = 1  phase previous (left) logo frame-id in the memory
//Bit 15:12        reg_frc_ocur_logo_fid     // unsigned ,    RW, default = 0  output phase current (right)logo frame-id in the memory
//Bit 11: 8        ro_frc_opre_fid_mux       // unsigned ,    RO, default = 0  pre fid set by fw
//Bit  7: 4        ro_frc_ocur_fid_mux       // unsigned ,    RO, default = 0  cur fid set by fw
//Bit  3: 0        ro_frc_onex_fid_mux       // unsigned ,    RO, default = 0  nex fid set by fw
#define FRC_REG_FWD_FID_POSI                       0x0148
//Bit 31:28        reg_frc_opre_point        // unsigned ,    RW, default = 0  fid position in lut
//Bit 27:24        reg_frc_ocur_point        // unsigned ,    RW, default = 1  fid position in lut
//Bit 23:20        reg_frc_onex_point        // unsigned ,    RW, default = 2  fid position in lut
//Bit 19:16        reg_frc_out_point         // unsigned ,    RW, default = 2  fid position in lut
//Bit 15            reserved
//Bit 14:12        reg_frc_opre_idx          // unsigned ,    RW, default = 0  pre fid index in lut
//Bit 11            reserved
//Bit 10: 8        reg_frc_ocur_idx          // unsigned ,    RW, default = 0  cur fid index in lut
//Bit  7            reserved
//Bit  6: 4        reg_frc_onex_idx          // unsigned ,    RW, default = 0  nex fid index in lut
//Bit  3            reserved
//Bit  2: 0        reg_frc_out_idx           // unsigned ,    RW, default = 0  current index in lut
#define FRC_REG_FWD_FID_LUT_1_0                    0x014b
//Bit 31:29        reserved
//Bit 28:24        reg_frc_out_id_lut_0_7    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 23:21        reserved
//Bit 20:16        reg_frc_out_id_lut_0_6    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 15:13        reserved
//Bit 12: 8        reg_frc_out_id_lut_0_5    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit  7: 5        reserved
//Bit  4: 0        reg_frc_out_id_lut_0_4    // unsigned ,    RW, default = 0  lut calculated by fw
#define FRC_REG_FWD_FID_LUT_2_0                    0x014c
//Bit 31:29        reserved
//Bit 28:24        reg_frc_out_id_lut_0_3    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 23:21        reserved
//Bit 20:16        reg_frc_out_id_lut_0_2    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 15:13        reserved
//Bit 12: 8        reg_frc_out_id_lut_0_1    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit  7: 5        reserved
//Bit  4: 0        reg_frc_out_id_lut_0_0    // unsigned ,    RW, default = 0  lut calculated by fw
#define FRC_REG_FWD_FID_LUT_1_1                    0x014d
//Bit 31:29        reserved
//Bit 28:24        reg_frc_out_id_lut_1_7    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 23:21        reserved
//Bit 20:16        reg_frc_out_id_lut_1_6    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 15:13        reserved
//Bit 12: 8        reg_frc_out_id_lut_1_5    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit  7: 5        reserved
//Bit  4: 0        reg_frc_out_id_lut_1_4    // unsigned ,    RW, default = 0  lut calculated by fw
#define FRC_REG_FWD_FID_LUT_2_1                    0x014e
//Bit 31:29        reserved
//Bit 28:24        reg_frc_out_id_lut_1_3    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 23:21        reserved
//Bit 20:16        reg_frc_out_id_lut_1_2    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 15:13        reserved
//Bit 12: 8        reg_frc_out_id_lut_1_1    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit  7: 5        reserved
//Bit  4: 0        reg_frc_out_id_lut_1_0    // unsigned ,    RW, default = 0  lut calculated by fw
#define FRC_REG_FWD_FID_LUT_1_2                    0x014f
//Bit 31:29        reserved
//Bit 28:24        reg_frc_out_id_lut_2_7    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 23:21        reserved
//Bit 20:16        reg_frc_out_id_lut_2_6    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 15:13        reserved
//Bit 12: 8        reg_frc_out_id_lut_2_5    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit  7: 5        reserved
//Bit  4: 0        reg_frc_out_id_lut_2_4    // unsigned ,    RW, default = 0  lut calculated by fw
#define FRC_REG_FWD_FID_LUT_2_2                    0x0150
//Bit 31:29        reserved
//Bit 28:24        reg_frc_out_id_lut_2_3    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 23:21        reserved
//Bit 20:16        reg_frc_out_id_lut_2_2    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 15:13        reserved
//Bit 12: 8        reg_frc_out_id_lut_2_1    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit  7: 5        reserved
//Bit  4: 0        reg_frc_out_id_lut_2_0    // unsigned ,    RW, default = 0  lut calculated by fw
#define FRC_REG_FWD_FID_LUT_1_3                    0x0151
//Bit 31:29        reserved
//Bit 28:24        reg_frc_out_id_lut_3_7    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 23:21        reserved
//Bit 20:16        reg_frc_out_id_lut_3_6    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 15:13        reserved
//Bit 12: 8        reg_frc_out_id_lut_3_5    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit  7: 5        reserved
//Bit  4: 0        reg_frc_out_id_lut_3_4    // unsigned ,    RW, default = 0  lut calculated by fw
#define FRC_REG_FWD_FID_LUT_2_3                    0x0152
//Bit 31:29        reserved
//Bit 28:24        reg_frc_out_id_lut_3_3    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 23:21        reserved
//Bit 20:16        reg_frc_out_id_lut_3_2    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 15:13        reserved
//Bit 12: 8        reg_frc_out_id_lut_3_1    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit  7: 5        reserved
//Bit  4: 0        reg_frc_out_id_lut_3_0    // unsigned ,    RW, default = 0  lut calculated by fw
#define FRC_REG_FWD_FID_LUT_1_4                    0x0153
//Bit 31:29        reserved
//Bit 28:24        reg_frc_out_id_lut_4_7    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 23:21        reserved
//Bit 20:16        reg_frc_out_id_lut_4_6    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 15:13        reserved
//Bit 12: 8        reg_frc_out_id_lut_4_5    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit  7: 5        reserved
//Bit  4: 0        reg_frc_out_id_lut_4_4    // unsigned ,    RW, default = 0  lut calculated by fw
#define FRC_REG_FWD_FID_LUT_2_4                    0x0154
//Bit 31:29        reserved
//Bit 28:24        reg_frc_out_id_lut_4_3    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 23:21        reserved
//Bit 20:16        reg_frc_out_id_lut_4_2    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 15:13        reserved
//Bit 12: 8        reg_frc_out_id_lut_4_1    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit  7: 5        reserved
//Bit  4: 0        reg_frc_out_id_lut_4_0    // unsigned ,    RW, default = 0  lut calculated by fw
#define FRC_REG_FWD_FID_LUT_1_5                    0x0155
//Bit 31:29        reserved
//Bit 28:24        reg_frc_out_id_lut_5_7    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 23:21        reserved
//Bit 20:16        reg_frc_out_id_lut_5_6    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 15:13        reserved
//Bit 12: 8        reg_frc_out_id_lut_5_5    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit  7: 5        reserved
//Bit  4: 0        reg_frc_out_id_lut_5_4    // unsigned ,    RW, default = 0  lut calculated by fw
#define FRC_REG_FWD_FID_LUT_2_5                    0x0156
//Bit 31:29        reserved
//Bit 28:24        reg_frc_out_id_lut_5_3    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 23:21        reserved
//Bit 20:16        reg_frc_out_id_lut_5_2    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 15:13        reserved
//Bit 12: 8        reg_frc_out_id_lut_5_1    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit  7: 5        reserved
//Bit  4: 0        reg_frc_out_id_lut_5_0    // unsigned ,    RW, default = 0  lut calculated by fw
#define FRC_REG_FWD_FID_LUT_1_6                    0x0157
//Bit 31:29        reserved
//Bit 28:24        reg_frc_out_id_lut_6_7    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 23:21        reserved
//Bit 20:16        reg_frc_out_id_lut_6_6    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 15:13        reserved
//Bit 12: 8        reg_frc_out_id_lut_6_5    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit  7: 5        reserved
//Bit  4: 0        reg_frc_out_id_lut_6_4    // unsigned ,    RW, default = 0  lut calculated by fw
#define FRC_REG_FWD_FID_LUT_2_6                    0x0158
//Bit 31:29        reserved
//Bit 28:24        reg_frc_out_id_lut_6_3    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 23:21        reserved
//Bit 20:16        reg_frc_out_id_lut_6_2    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 15:13        reserved
//Bit 12: 8        reg_frc_out_id_lut_6_1    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit  7: 5        reserved
//Bit  4: 0        reg_frc_out_id_lut_6_0    // unsigned ,    RW, default = 0  lut calculated by fw
#define FRC_REG_FWD_FID_LUT_1_7                    0x0159
//Bit 31:29        reserved
//Bit 28:24        reg_frc_out_id_lut_7_7    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 23:21        reserved
//Bit 20:16        reg_frc_out_id_lut_7_6    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 15:13        reserved
//Bit 12: 8        reg_frc_out_id_lut_7_5    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit  7: 5        reserved
//Bit  4: 0        reg_frc_out_id_lut_7_4    // unsigned ,    RW, default = 0  lut calculated by fw
#define FRC_REG_FWD_FID_LUT_2_7                    0x015a
//Bit 31:29        reserved
//Bit 28:24        reg_frc_out_id_lut_7_3    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 23:21        reserved
//Bit 20:16        reg_frc_out_id_lut_7_2    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 15:13        reserved
//Bit 12: 8        reg_frc_out_id_lut_7_1    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit  7: 5        reserved
//Bit  4: 0        reg_frc_out_id_lut_7_0    // unsigned ,    RW, default = 0  lut calculated by fw
#define FRC_REG_FWD_FID_LUT_1_8                    0x015b
//Bit 31:29        reserved
//Bit 28:24        reg_frc_out_id_lut_8_7    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 23:21        reserved
//Bit 20:16        reg_frc_out_id_lut_8_6    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 15:13        reserved
//Bit 12: 8        reg_frc_out_id_lut_8_5    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit  7: 5        reserved
//Bit  4: 0        reg_frc_out_id_lut_8_4    // unsigned ,    RW, default = 0  lut calculated by fw
#define FRC_REG_FWD_FID_LUT_2_8                    0x015c
//Bit 31:29        reserved
//Bit 28:24        reg_frc_out_id_lut_8_3    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 23:21        reserved
//Bit 20:16        reg_frc_out_id_lut_8_2    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 15:13        reserved
//Bit 12: 8        reg_frc_out_id_lut_8_1    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit  7: 5        reserved
//Bit  4: 0        reg_frc_out_id_lut_8_0    // unsigned ,    RW, default = 0  lut calculated by fw
#define FRC_REG_FWD_FID_LUT_1_9                    0x015d
//Bit 31:29        reserved
//Bit 28:24        reg_frc_out_id_lut_9_7    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 23:21        reserved
//Bit 20:16        reg_frc_out_id_lut_9_6    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 15:13        reserved
//Bit 12: 8        reg_frc_out_id_lut_9_5    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit  7: 5        reserved
//Bit  4: 0        reg_frc_out_id_lut_9_4    // unsigned ,    RW, default = 0  lut calculated by fw
#define FRC_REG_FWD_FID_LUT_2_9                    0x015e
//Bit 31:29        reserved
//Bit 28:24        reg_frc_out_id_lut_9_3    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 23:21        reserved
//Bit 20:16        reg_frc_out_id_lut_9_2    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 15:13        reserved
//Bit 12: 8        reg_frc_out_id_lut_9_1    // unsigned ,    RW, default = 0  lut calculated by fw
//Bit  7: 5        reserved
//Bit  4: 0        reg_frc_out_id_lut_9_0    // unsigned ,    RW, default = 0  lut calculated by fw
#define FRC_REG_FWD_FID_LUT_1_10                   0x015f
//Bit 31:29        reserved
//Bit 28:24        reg_frc_out_id_lut_10_7   // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 23:21        reserved
//Bit 20:16        reg_frc_out_id_lut_10_6   // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 15:13        reserved
//Bit 12: 8        reg_frc_out_id_lut_10_5   // unsigned ,    RW, default = 0  lut calculated by fw
//Bit  7: 5        reserved
//Bit  4: 0        reg_frc_out_id_lut_10_4   // unsigned ,    RW, default = 0  lut calculated by fw
#define FRC_REG_FWD_FID_LUT_2_10                   0x0160
//Bit 31:29        reserved
//Bit 28:24        reg_frc_out_id_lut_10_3   // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 23:21        reserved
//Bit 20:16        reg_frc_out_id_lut_10_2   // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 15:13        reserved
//Bit 12: 8        reg_frc_out_id_lut_10_1   // unsigned ,    RW, default = 0  lut calculated by fw
//Bit  7: 5        reserved
//Bit  4: 0        reg_frc_out_id_lut_10_0   // unsigned ,    RW, default = 0  lut calculated by fw
#define FRC_REG_FWD_FID_LUT_1_11                   0x0161
//Bit 31:29        reserved
//Bit 28:24        reg_frc_out_id_lut_11_7   // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 23:21        reserved
//Bit 20:16        reg_frc_out_id_lut_11_6   // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 15:13        reserved
//Bit 12: 8        reg_frc_out_id_lut_11_5   // unsigned ,    RW, default = 0  lut calculated by fw
//Bit  7: 5        reserved
//Bit  4: 0        reg_frc_out_id_lut_11_4   // unsigned ,    RW, default = 0  lut calculated by fw
#define FRC_REG_FWD_FID_LUT_2_11                   0x0162
//Bit 31:29        reserved
//Bit 28:24        reg_frc_out_id_lut_11_3   // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 23:21        reserved
//Bit 20:16        reg_frc_out_id_lut_11_2   // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 15:13        reserved
//Bit 12: 8        reg_frc_out_id_lut_11_1   // unsigned ,    RW, default = 0  lut calculated by fw
//Bit  7: 5        reserved
//Bit  4: 0        reg_frc_out_id_lut_11_0   // unsigned ,    RW, default = 0  lut calculated by fw
#define FRC_REG_FWD_FID_LUT_1_12                   0x0163
//Bit 31:29        reserved
//Bit 28:24        reg_frc_out_id_lut_12_7   // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 23:21        reserved
//Bit 20:16        reg_frc_out_id_lut_12_6   // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 15:13        reserved
//Bit 12: 8        reg_frc_out_id_lut_12_5   // unsigned ,    RW, default = 0  lut calculated by fw
//Bit  7: 5        reserved
//Bit  4: 0        reg_frc_out_id_lut_12_4   // unsigned ,    RW, default = 0  lut calculated by fw
#define FRC_REG_FWD_FID_LUT_2_12                   0x0164
//Bit 31:29        reserved
//Bit 28:24        reg_frc_out_id_lut_12_3   // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 23:21        reserved
//Bit 20:16        reg_frc_out_id_lut_12_2   // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 15:13        reserved
//Bit 12: 8        reg_frc_out_id_lut_12_1   // unsigned ,    RW, default = 0  lut calculated by fw
//Bit  7: 5        reserved
//Bit  4: 0        reg_frc_out_id_lut_12_0   // unsigned ,    RW, default = 0  lut calculated by fw
#define FRC_REG_FWD_FID_LUT_1_13                   0x0165
//Bit 31:29        reserved
//Bit 28:24        reg_frc_out_id_lut_13_7   // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 23:21        reserved
//Bit 20:16        reg_frc_out_id_lut_13_6   // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 15:13        reserved
//Bit 12: 8        reg_frc_out_id_lut_13_5   // unsigned ,    RW, default = 0  lut calculated by fw
//Bit  7: 5        reserved
//Bit  4: 0        reg_frc_out_id_lut_13_4   // unsigned ,    RW, default = 0  lut calculated by fw
#define FRC_REG_FWD_FID_LUT_2_13                   0x0166
//Bit 31:29        reserved
//Bit 28:24        reg_frc_out_id_lut_13_3   // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 23:21        reserved
//Bit 20:16        reg_frc_out_id_lut_13_2   // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 15:13        reserved
//Bit 12: 8        reg_frc_out_id_lut_13_1   // unsigned ,    RW, default = 0  lut calculated by fw
//Bit  7: 5        reserved
//Bit  4: 0        reg_frc_out_id_lut_13_0   // unsigned ,    RW, default = 0  lut calculated by fw
#define FRC_REG_FWD_FID_LUT_1_14                   0x0167
//Bit 31:29        reserved
//Bit 28:24        reg_frc_out_id_lut_14_7   // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 23:21        reserved
//Bit 20:16        reg_frc_out_id_lut_14_6   // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 15:13        reserved
//Bit 12: 8        reg_frc_out_id_lut_14_5   // unsigned ,    RW, default = 0  lut calculated by fw
//Bit  7: 5        reserved
//Bit  4: 0        reg_frc_out_id_lut_14_4   // unsigned ,    RW, default = 0  lut calculated by fw
#define FRC_REG_FWD_FID_LUT_2_14                   0x0168
//Bit 31:29        reserved
//Bit 28:24        reg_frc_out_id_lut_14_3   // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 23:21        reserved
//Bit 20:16        reg_frc_out_id_lut_14_2   // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 15:13        reserved
//Bit 12: 8        reg_frc_out_id_lut_14_1   // unsigned ,    RW, default = 0  lut calculated by fw
//Bit  7: 5        reserved
//Bit  4: 0        reg_frc_out_id_lut_14_0   // unsigned ,    RW, default = 0  lut calculated by fw
#define FRC_REG_FWD_FID_LUT_1_15                   0x0169
//Bit 31:29        reserved
//Bit 28:24        reg_frc_out_id_lut_15_7   // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 23:21        reserved
//Bit 20:16        reg_frc_out_id_lut_15_6   // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 15:13        reserved
//Bit 12: 8        reg_frc_out_id_lut_15_5   // unsigned ,    RW, default = 0  lut calculated by fw
//Bit  7: 5        reserved
//Bit  4: 0        reg_frc_out_id_lut_15_4   // unsigned ,    RW, default = 0  lut calculated by fw
#define FRC_REG_FWD_FID_LUT_2_15                   0x016a
//Bit 31:29        reserved
//Bit 28:24        reg_frc_out_id_lut_15_3   // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 23:21        reserved
//Bit 20:16        reg_frc_out_id_lut_15_2   // unsigned ,    RW, default = 0  lut calculated by fw
//Bit 15:13        reserved
//Bit 12: 8        reg_frc_out_id_lut_15_1   // unsigned ,    RW, default = 0  lut calculated by fw
//Bit  7: 5        reserved
//Bit  4: 0        reg_frc_out_id_lut_15_0   // unsigned ,    RW, default = 0  lut calculated by fw
#define FRC_REG_FWD_PHS_ADJ                        0x016b
//Bit 31           reg_frc_output_phase_reset      // unsigned ,    RW, default = 0  phase between opre and ocur reset 0 (eg :mode change)
//Bit 30           reg_frc_output_phase_ofset_en   // unsigned ,    RW, default = 1  phase_ofset enable
//Bit 29           reg_frc_phsoft_left_reset_en // unsigned ,    RW, default = 1  enable of phase ofset reset 0
//Bit 28           reg_frc_phsoft_right_reset_en // unsigned ,    RW, default = 1  enable of phase ofset reset 0
//Bit 27:22        reg_frc_zeros_phase_th    // unsigned ,    RW, default = 2  if phase less than th
//Bit 21: 0        reg_frc_phase_delta       // unsigned ,    RW, default = 0  phase delta
#define FRC_REG_FWD_TABLE_CNT_PHAOFS               0x016c
//Bit 31:12        ro_frc_output_phase_ofset       // unsigned ,    RO, default = 0  for fw mc_phase precision
//Bit 11: 4        reg_frc_table_cnt         // unsigned ,    RW, default = 0  for fw. output period table count, => ro_frc_load_frm_flag to drive input load_org_frame_HW, set by fw in fw_ofrm_det_core.
//Bit  3           reg_frc_otb_up_flag       // unsigned ,    RW, default = 0  when 1, reg_frc_otb_start=1, set by fw
//Bit  2           reg_frc_fid_balance_en    // unsigned ,    RW, default = 0  enable signal of balance frame id
//Bit  1           reg_fix_nex_idx_zero      // unsigned ,    RW, default = 1  fix next index to zero
//Bit  0           ro_frc_no_new_frm_in_flag // unsigned ,    RO, default = 0  no new frame flag
#define FRC_REG_PD_PAT_NUM                         0x016d
//Bit 31: 8        reserved
//Bit  7: 0        reg_frc_pd_pat_num        // unsigned ,    RW, default = 0  , pull-down period of sequence, if 32 pulldown, period = 5. set by fw in phase_table_core
#define FRC_REG_FWD_SIGN_RO                        0x016e
//Bit 31           reg_frc_tbl_wr_down_en    // unsigned ,    RW, default = 0  the signal for notifier hw the table have already write down
//Bit 30           ro_frc_hw_latch_error     // unsigned ,    RO, default = 0  the signal for hw vsync latch error
//Bit 29           reg_frc_force_point_idx_en // unsigned ,    RW, default = 1  force pre/cur/nex id point ,use the result of fw calculate
//Bit 28           reg_frc_force_phase_en    // unsigned ,    RW, default = 0  force me/mc phase
//Bit 27:24        ro_frc_opre_point         // unsigned ,    RO, default = 0  fid position in lut
//Bit 23:20        ro_frc_ocur_point         // unsigned ,    RO, default = 0  fid position in lut
//Bit 19:16        ro_frc_onex_point         // unsigned ,    RO, default = 0  fid position in lut
//Bit 15:12        reserved
//Bit 11            reserved
//Bit 10: 8        ro_frc_opre_idx           // unsigned ,    RO, default = 0  fid index in lut
//Bit  7            reserved
//Bit  6: 4        ro_frc_ocur_idx           // unsigned ,    RO, default = 0  fid index in lut
//Bit  3            reserved
//Bit  2: 0        ro_frc_onex_idx           // unsigned ,    RO, default = 0  fid index in lut
#define FRC_REG_FWD_PHS_RO                         0x016f
//Bit 31:24        reserved
//Bit 23:12        ro_frc_output_phase_mux         // unsigned ,    RO, default = 0  phase between opre and ocur frames(0-4095) 12 bits set by fw
//Bit 11: 0        ro_frc_output_phase_me_mux      // unsigned ,    RO, default = 0  phase between opre and ocur frames (0-4095) 12 bits for ME and VP. set by fw
#define FRC_INPUT_SIZE_ALIGN                       0x0170
//Bit 31: 2        reserved
//Bit  1           reg_h_size_align_mode     // unsigned ,    RW, default = 0  the alignment mode of Hsize, 0: 8 , 1: 16
//Bit  0           reg_v_size_align_mode     // unsigned ,    RW, default = 0  the alignment mode of Vsize, 0: 8	, 1: 16
#define FRC_REG_FWD_PHS_GAIN                       0x0171
//Bit 31:24        reg_frc_phase_mux_gain    // unsigned ,    RW, default = 255  mc phase gain for memc level
//Bit 23:16        reg_frc_phase_me_mux_gain // unsigned ,    RW, default = 255  me phase gain for memc level
//Bit 15:12        reserved
//Bit 11: 0        ro_frc_phase_mux_raw     // unsigned ,    RO, default = 0  phase between opre and ocur frames(0-4095) 12 bits set by fw, memc level "1";
#define FRC_BADEDIT_DBG0                           0x0181
//Bit 31:0         ro_frc_badedit_dbg0      // unsigned ,    RO, default = 0 frc_badedit_inf0
#define FRC_BADEDIT_DBG1                           0x0182
//Bit 31:0         ro_frc_badedit_dbg1      // unsigned ,    RO, default = 0 frc_badedit_inf1
// Closing file:  ./frc_inc/frc_badedit_regs.h
//
// Reading file:  ./frc_inc/frc_buff_addr_regs.h
#define FRC_REG_MCDW_YBUF_ADDRX_0                  0x0220
//Bit 31:0 reg_mcdw_ybuf_addrx_0          // unsigned ,   RW, default = 32'h00700000
#define FRC_REG_MCDW_YBUF_ADDRX_1                  0x0221
//Bit 31:0 reg_mcdw_ybuf_addrx_1          // unsigned ,   RW, default = 32'h00703000
#define FRC_REG_MCDW_YBUF_ADDRX_2                  0x0222
//Bit 31:0 reg_mcdw_ybuf_addrx_2          // unsigned ,   RW, default = 32'h00706000
#define FRC_REG_MCDW_YBUF_ADDRX_3                  0x0223
//Bit 31:0 reg_mcdw_ybuf_addrx_3          // unsigned ,   RW, default = 32'h00709000
#define FRC_REG_MCDW_YBUF_ADDRX_4                  0x0224
//Bit 31:0 reg_mcdw_ybuf_addrx_4          // unsigned ,   RW, default = 32'h0070c000
#define FRC_REG_MCDW_YBUF_ADDRX_5                  0x0225
//Bit 31:0 reg_mcdw_ybuf_addrx_5          // unsigned ,   RW, default = 32'h0070f000
#define FRC_REG_MCDW_YBUF_ADDRX_6                  0x0226
//Bit 31:0 reg_mcdw_ybuf_addrx_6          // unsigned ,   RW, default = 32'h00712000
#define FRC_REG_MCDW_YBUF_ADDRX_7                  0x0227
//Bit 31:0 reg_mcdw_ybuf_addrx_7          // unsigned ,   RW, default = 32'h00715000
#define FRC_REG_MCDW_YBUF_ADDRX_8                  0x0228
//Bit 31:0 reg_mcdw_ybuf_addrx_8          // unsigned ,   RW, default = 32'h00718000
#define FRC_REG_MCDW_YBUF_ADDRX_9                  0x0229
//Bit 31:0 reg_mcdw_ybuf_addrx_9          // unsigned ,   RW, default = 32'h0071b000
#define FRC_REG_MCDW_YBUF_ADDRX_10                 0x022a
//Bit 31:0 reg_mcdw_ybuf_addrx_10         // unsigned ,   RW, default = 32'h0071e000
#define FRC_REG_MCDW_YBUF_ADDRX_11                 0x022b
//Bit 31:0 reg_mcdw_ybuf_addrx_11         // unsigned ,   RW, default = 32'h00721000
#define FRC_REG_MCDW_YBUF_ADDRX_12                 0x022c
//Bit 31:0 reg_mcdw_ybuf_addrx_12         // unsigned ,   RW, default = 32'h00724000
#define FRC_REG_MCDW_YBUF_ADDRX_13                 0x022d
//Bit 31:0 reg_mcdw_ybuf_addrx_13         // unsigned ,   RW, default = 32'h00727000
#define FRC_REG_MCDW_YBUF_ADDRX_14                 0x022e
//Bit 31:0 reg_mcdw_ybuf_addrx_14         // unsigned ,   RW, default = 32'h0072a000
#define FRC_REG_MCDW_YBUF_ADDRX_15                 0x022f
//Bit 31:0 reg_mcdw_ybuf_addrx_15          // unsigned ,  RW, default = 32'h0072d000
#define FRC_REG_MCDW_CBUF_ADDRX_0                  0x0230
//Bit 31:0 reg_mcdw_cbuf_addrx_0          // unsigned ,   RW, default = 32'h00730000
#define FRC_REG_MCDW_CBUF_ADDRX_1                  0x0231
//Bit 31:0 reg_mcdw_cbuf_addrx_1          // unsigned ,   RW, default = 32'h00733000
#define FRC_REG_MCDW_CBUF_ADDRX_2                  0x0232
//Bit 31:0 reg_mcdw_cbuf_addrx_2          // unsigned ,   RW, default = 32'h00736000
#define FRC_REG_MCDW_CBUF_ADDRX_3                  0x0233
//Bit 31:0 reg_mcdw_cbuf_addrx_3          // unsigned ,   RW, default = 32'h00739000
#define FRC_REG_MCDW_CBUF_ADDRX_4                  0x0234
//Bit 31:0 reg_mcdw_cbuf_addrx_4          // unsigned ,   RW, default = 32'h0073c000
#define FRC_REG_MCDW_CBUF_ADDRX_5                  0x0235
//Bit 31:0 reg_mcdw_cbuf_addrx_5          // unsigned ,   RW, default = 32'h0073f000
#define FRC_REG_MCDW_CBUF_ADDRX_6                  0x0236
//Bit 31:0 reg_mcdw_cbuf_addrx_6          // unsigned ,   RW, default = 32'h00742000
#define FRC_REG_MCDW_CBUF_ADDRX_7                  0x0237
//Bit 31:0 reg_mcdw_cbuf_addrx_7          // unsigned ,   RW, default = 32'h00745000
#define FRC_REG_MCDW_CBUF_ADDRX_8                  0x0238
//Bit 31:0 reg_mcdw_cbuf_addrx_8          // unsigned ,   RW, default = 32'h00748000
#define FRC_REG_MCDW_CBUF_ADDRX_9                  0x0239
//Bit 31:0 reg_mcdw_cbuf_addrx_9          // unsigned ,   RW, default = 32'h0074b000
#define FRC_REG_MCDW_CBUF_ADDRX_10                 0x023a
//Bit 31:0 reg_mcdw_cbuf_addrx_10         // unsigned ,   RW, default = 32'h0074e000
#define FRC_REG_MCDW_CBUF_ADDRX_11                 0x023b
//Bit 31:0 reg_mcdw_cbuf_addrx_11         // unsigned ,   RW, default = 32'h00751000
#define FRC_REG_MCDW_CBUF_ADDRX_12                 0x023c
//Bit 31:0 reg_mcdw_cbuf_addrx_12         // unsigned ,   RW, default = 32'h00754000
#define FRC_REG_MCDW_CBUF_ADDRX_13                 0x023d
//Bit 31:0 reg_mcdw_cbuf_addrx_13         // unsigned ,   RW, default = 32'h00757000
#define FRC_REG_MCDW_CBUF_ADDRX_14                 0x023e
//Bit 31:0 reg_mcdw_cbuf_addrx_14         // unsigned ,   RW, default = 32'h075a000
#define FRC_REG_MCDW_CBUF_ADDRX_15                 0x023f
//Bit 31:0 reg_mcdw_cbuf_addrx_15         // unsigned ,   RW, default = 32'h0075d000
#define FRC_REG_MC_YINFO_BADDR                     0x0271
//Bit 31:0 reg_mc_yinfo_baddr        // unsigned ,    RW, default = 32'h00510000
#define FRC_REG_MC_CINFO_BADDR                     0x0272
//Bit 31:0 reg_mc_cinfo_baddr        // unsigned ,    RW, default = 32'h00512000
#define FRC_REG_MC_VINFO_BADDR                     0x0273
//Bit 31:0 reg_mc_vinfo_baddr        // unsigned ,    RW, default = 32'h00514000
#define FRC_REG_ME_XINFO_BADDR                     0x0274
//Bit 31:0 reg_me_xinfo_baddr        // unsigned ,    RW, default = 32'h00516000
#define FRC_REG_MCDW_YINFO_BADDR                   0x0275
//Bit 31:0 reg_mcdw_yinfo_baddr      // unsigned ,    RW, default = 32'h00518000
#define FRC_REG_MCDW_CINFO_BADDR                   0x0276
//Bit 31:0 reg_mcdw_cinfo_baddr      // unsigned ,    RW, default = 32'h0051a000
#define FRC_REG_VP_MC_MV_ADDRX_0                   0x027c
//Bit 31:0 reg_vp_mc_mv_addrx_0      // unsigned ,    RW, default = 32'h01800000 ,
// must be burst4 align
#define FRC_REG_VP_MC_MV_ADDRX_1                   0x027d
//Bit 31:0 reg_vp_mc_mv_addrx_1      // unsigned ,    RW, default = 32'h01900000 ,
// must be burst4 align
#define FRC_REG_MC_YBUF_ADDRX_0                    0x0280
//Bit 31:0 reg_mc_ybuf_addrx_0        // unsigned ,   RW, default = 32'h00600000  mif_addr
#define FRC_REG_MC_YBUF_ADDRX_1                    0x0281
//Bit 31:0 reg_mc_ybuf_addrx_1        // unsigned ,   RW, default = 32'h00603000  mif_addr
#define FRC_REG_MC_YBUF_ADDRX_2                    0x0282
//Bit 31:0 reg_mc_ybuf_addrx_2        // unsigned ,   RW, default = 32'h00606000  mif_addr
#define FRC_REG_MC_YBUF_ADDRX_3                    0x0283
//Bit 31:0 reg_mc_ybuf_addrx_3        // unsigned ,   RW, default = 32'h00609000  mif_addr
#define FRC_REG_MC_YBUF_ADDRX_4                    0x0284
//Bit 31:0 reg_mc_ybuf_addrx_4        // unsigned ,   RW, default = 32'h0060c000  mif_addr
#define FRC_REG_MC_YBUF_ADDRX_5                    0x0285
//Bit 31:0 reg_mc_ybuf_addrx_5        // unsigned ,   RW, default = 32'h0060f000 mif_addr
#define FRC_REG_MC_YBUF_ADDRX_6                    0x0286
//Bit 31:0 reg_mc_ybuf_addrx_6        // unsigned ,   RW, default = 32'h00612000 mif_addr
#define FRC_REG_MC_YBUF_ADDRX_7                    0x0287
//Bit 31:0 reg_mc_ybuf_addrx_7        // unsigned ,   RW, default = 32'h00615000 mif_addr
#define FRC_REG_MC_YBUF_ADDRX_8                    0x0288
//Bit 31:0 reg_mc_ybuf_addrx_8        // unsigned ,   RW, default = 32'h00618000 mif_addr
#define FRC_REG_MC_YBUF_ADDRX_9                    0x0289
//Bit 31:0 reg_mc_ybuf_addrx_9        // unsigned ,   RW, default = 32'h0061b000 mif_addr
#define FRC_REG_MC_YBUF_ADDRX_10                   0x028a
//Bit 31:0 reg_mc_ybuf_addrx_10       // unsigned ,   RW, default = 32'h0061e000 mif_addr
#define FRC_REG_MC_YBUF_ADDRX_11                   0x028b
//Bit 31:0 reg_mc_ybuf_addrx_11       // unsigned ,   RW, default = 32'h00621000 mif_addr
#define FRC_REG_MC_YBUF_ADDRX_12                   0x028c
//Bit 31:0 reg_mc_ybuf_addrx_12       // unsigned ,   RW, default = 32'h00624000 mif_addr
#define FRC_REG_MC_YBUF_ADDRX_13                   0x028d
//Bit 31:0 reg_mc_ybuf_addrx_13       // unsigned ,   RW, default = 32'h00627000 mif_addr
#define FRC_REG_MC_YBUF_ADDRX_14                   0x028e
//Bit 31:0 reg_mc_ybuf_addrx_14       // unsigned ,   RW, default = 32'h0062a000 mif_addr
#define FRC_REG_MC_YBUF_ADDRX_15                   0x028f
//Bit 31:0 reg_mc_ybuf_addrx_15        // unsigned ,  RW, default = 32'h0062d000 mif_addr
#define FRC_REG_MC_CBUF_ADDRX_0                    0x0290
//Bit 31:0 reg_mc_cbuf_addrx_0        // unsigned ,    RW, default = 32'h00630000 mif_addr
#define FRC_REG_MC_CBUF_ADDRX_1                    0x0291
//Bit 31:0 reg_mc_cbuf_addrx_1        // unsigned ,    RW, default = 32'h00633000 mif_addr
#define FRC_REG_MC_CBUF_ADDRX_2                    0x0292
//Bit 31:0 reg_mc_cbuf_addrx_2        // unsigned ,    RW, default = 32'h00636000 mif_addr
#define FRC_REG_MC_CBUF_ADDRX_3                    0x0293
//Bit 31:0 reg_mc_cbuf_addrx_3        // unsigned ,    RW, default = 32'h00639000 mif_addr
#define FRC_REG_MC_CBUF_ADDRX_4                    0x0294
//Bit 31:0 reg_mc_cbuf_addrx_4        // unsigned ,    RW, default = 32'h0063c000 mif_addr
#define FRC_REG_MC_CBUF_ADDRX_5                    0x0295
//Bit 31:0 reg_mc_cbuf_addrx_5        // unsigned ,    RW, default = 32'h0063f000 mif_addr
#define FRC_REG_MC_CBUF_ADDRX_6                    0x0296
//Bit 31:0 reg_mc_cbuf_addrx_6        // unsigned ,    RW, default = 32'h00642000 mif_addr
#define FRC_REG_MC_CBUF_ADDRX_7                    0x0297
//Bit 31:0 reg_mc_cbuf_addrx_7        // unsigned ,    RW, default = 32'h00645000 mif_addr
#define FRC_REG_MC_CBUF_ADDRX_8                    0x0298
//Bit 31:0 reg_mc_cbuf_addrx_8        // unsigned ,    RW, default = 32'h00648000 mif_addr
#define FRC_REG_MC_CBUF_ADDRX_9                    0x0299
//Bit 31:0 reg_mc_cbuf_addrx_9        // unsigned ,    RW, default = 32'h0064b000 mif_addr
#define FRC_REG_MC_CBUF_ADDRX_10                   0x029a
//Bit 31:0 reg_mc_cbuf_addrx_10       // unsigned ,    RW, default = 32'h0064e000 mif_addr
#define FRC_REG_MC_CBUF_ADDRX_11                   0x029b
//Bit 31:0 reg_mc_cbuf_addrx_11       // unsigned ,    RW, default = 32'h00651000 mif_addr
#define FRC_REG_MC_CBUF_ADDRX_12                   0x029c
//Bit 31:0 reg_mc_cbuf_addrx_12       // unsigned ,    RW, default = 32'h00654000 mif_addr
#define FRC_REG_MC_CBUF_ADDRX_13                   0x029d
//Bit 31:0 reg_mc_cbuf_addrx_13       // unsigned ,    RW, default = 32'h00657000 mif_addr
#define FRC_REG_MC_CBUF_ADDRX_14                   0x029e
//Bit 31:0 reg_mc_cbuf_addrx_14       // unsigned ,    RW, default = 32'h0065a000 mif_addr
#define FRC_REG_MC_CBUF_ADDRX_15                   0x029f
//Bit 31:0 reg_mc_cbuf_addrx_15       // unsigned ,    RW, default = 32'h0065d000 mif_addr
#define FRC_REG_MC_VBUF_ADDRX_0                    0x02a0
//Bit 31:0 reg_mc_vbuf_addrx_0       // unsigned ,    RW, default = 32'h00660000 mif_addr
#define FRC_REG_MC_VBUF_ADDRX_1                    0x02a1
//Bit 31:0 reg_mc_vbuf_addrx_1       // unsigned ,    RW, default = 32'h00663000 mif_addr
#define FRC_REG_MC_VBUF_ADDRX_2                    0x02a2
//Bit 31:0 reg_mc_vbuf_addrx_2       // unsigned ,    RW, default = 32'h00666000 mif_addr
#define FRC_REG_MC_VBUF_ADDRX_3                    0x02a3
//Bit 31:0 reg_mc_vbuf_addrx_3       // unsigned ,    RW, default = 32'h00669000 mif_addr
#define FRC_REG_MC_VBUF_ADDRX_4                    0x02a4
//Bit 31:0 reg_mc_vbuf_addrx_4       // unsigned ,    RW, default = 32'h0066c000 mif_addr
#define FRC_REG_MC_VBUF_ADDRX_5                    0x02a5
//Bit 31:0 reg_mc_vbuf_addrx_5       // unsigned ,    RW, default = 32'h0066f000 mif_addr
#define FRC_REG_MC_VBUF_ADDRX_6                    0x02a6
//Bit 31:0 reg_mc_vbuf_addrx_6       // unsigned ,    RW, default = 32'h00672000 mif_addr
#define FRC_REG_MC_VBUF_ADDRX_7                    0x02a7
//Bit 31:0 reg_mc_vbuf_addrx_7       // unsigned ,    RW, default = 32'h00675000 mif_addr
#define FRC_REG_MC_VBUF_ADDRX_8                    0x02a8
//Bit 31:0 reg_mc_vbuf_addrx_8       // unsigned ,    RW, default = 32'h00678000 mif_addr
#define FRC_REG_MC_VBUF_ADDRX_9                    0x02a9
//Bit 31:0 reg_mc_vbuf_addrx_9       // unsigned ,    RW, default = 32'h0067b000 mif_addr
#define FRC_REG_MC_VBUF_ADDRX_10                   0x02aa
//Bit 31:0 reg_mc_vbuf_addrx_10      // unsigned ,    RW, default = 32'h0067e000 mif_addr
#define FRC_REG_MC_VBUF_ADDRX_11                   0x02ab
//Bit 31:0 reg_mc_vbuf_addrx_11      // unsigned ,    RW, default = 32'h00681000 mif_addr
#define FRC_REG_MC_VBUF_ADDRX_12                   0x02ac
//Bit 31:0 reg_mc_vbuf_addrx_12      // unsigned ,    RW, default = 32'h00684000 mif_addr
#define FRC_REG_MC_VBUF_ADDRX_13                   0x02ad
//Bit 31:0 reg_mc_vbuf_addrx_13      // unsigned ,    RW, default = 32'h00687000 mif_addr
#define FRC_REG_MC_VBUF_ADDRX_14                   0x02ae
//Bit 31:0 reg_mc_vbuf_addrx_14      // unsigned ,    RW, default = 32'h0068a000 mif_addr
#define FRC_REG_MC_VBUF_ADDRX_15                   0x02af
//Bit 31:0 reg_mc_vbuf_addrx_15      // unsigned ,    RW, default = 32'h0068d000 mif_addr
#define FRC_REG_ME_BUF_ADDRX_0                     0x02b0
//Bit 31:0 reg_me_buf_addrx_0        // unsigned ,    RW, default = 32'h00690000 mif_addr
#define FRC_REG_ME_BUF_ADDRX_1                     0x02b1
//Bit 31:0 reg_me_buf_addrx_1        // unsigned ,    RW, default = 32'h00693000 mif_addr
#define FRC_REG_ME_BUF_ADDRX_2                     0x02b2
//Bit 31:0 reg_me_buf_addrx_2        // unsigned ,    RW, default = 32'h00696000 mif_addr
#define FRC_REG_ME_BUF_ADDRX_3                     0x02b3
//Bit 31:0 reg_me_buf_addrx_3        // unsigned ,    RW, default = 32'h00699000 mif_addr
#define FRC_REG_ME_BUF_ADDRX_4                     0x02b4
//Bit 31:0 reg_me_buf_addrx_4        // unsigned ,    RW, default = 32'h0069c000 mif_addr
#define FRC_REG_ME_BUF_ADDRX_5                     0x02b5
//Bit 31:0 reg_me_buf_addrx_5        // unsigned ,    RW, default = 32'h0069f000 mif_addr
#define FRC_REG_ME_BUF_ADDRX_6                     0x02b6
//Bit 31:0 reg_me_buf_addrx_6        // unsigned ,    RW, default = 32'h006a2000 mif_addr
#define FRC_REG_ME_BUF_ADDRX_7                     0x02b7
//Bit 31:0 reg_me_buf_addrx_7        // unsigned ,    RW, default = 32'h006a5000 mif_addr
#define FRC_REG_ME_BUF_ADDRX_8                     0x02b8
//Bit 31:0 reg_me_buf_addrx_8        // unsigned ,    RW, default = 32'h006a8000 mif_addr
#define FRC_REG_ME_BUF_ADDRX_9                     0x02b9
//Bit 31:0 reg_me_buf_addrx_9        // unsigned ,    RW, default = 32'h006ab000 mif_addr
#define FRC_REG_ME_BUF_ADDRX_10                    0x02ba
//Bit 31:0 reg_me_buf_addrx_10       // unsigned ,    RW, default = 32'h006ae000 mif_addr
#define FRC_REG_ME_BUF_ADDRX_11                    0x02bb
//Bit 31:0 reg_me_buf_addrx_11       // unsigned ,    RW, default = 32'h006b1000 mif_addr
#define FRC_REG_ME_BUF_ADDRX_12                    0x02bc
//Bit 31:0 reg_me_buf_addrx_12       // unsigned ,    RW, default = 32'h006b4000 mif_addr
#define FRC_REG_ME_BUF_ADDRX_13                    0x02bd
//Bit 31:0 reg_me_buf_addrx_13       // unsigned ,    RW, default = 32'h006b7000 mif_addr
#define FRC_REG_ME_BUF_ADDRX_14                    0x02be
//Bit 31:0 reg_me_buf_addrx_14       // unsigned ,    RW, default = 32'h006ba000 mif_addr
#define FRC_REG_ME_BUF_ADDRX_15                    0x02bf
//Bit 31:0 reg_me_buf_addrx_15       // unsigned ,    RW, default = 32'h006bd000 mif_addr
#define FRC_REG_HME_BUF_ADDRX_0                    0x02c0
//Bit 31:0 reg_hme_buf_addrx_0       // unsigned ,    RW, default = 32'h00100000 mif_addr
#define FRC_REG_HME_BUF_ADDRX_1                    0x02c1
//Bit 31:0 reg_hme_buf_addrx_1       // unsigned ,    RW, default = 32'h00120000 mif_addr
#define FRC_REG_HME_BUF_ADDRX_2                    0x02c2
//Bit 31:0 reg_hme_buf_addrx_2       // unsigned ,    RW, default = 32'h00140000 mif_addr
#define FRC_REG_HME_BUF_ADDRX_3                    0x02c3
//Bit 31:0 reg_hme_buf_addrx_3       // unsigned ,    RW, default = 32'h00160000 mif_addr
#define FRC_REG_HME_BUF_ADDRX_4                    0x02c4
//Bit 31:0 reg_hme_buf_addrx_4       // unsigned ,    RW, default = 32'h00180000 mif_addr
#define FRC_REG_HME_BUF_ADDRX_5                    0x02c5
//Bit 31:0 reg_hme_buf_addrx_5       // unsigned ,    RW, default = 32'h001a0000 mif_addr
#define FRC_REG_HME_BUF_ADDRX_6                    0x02c6
//Bit 31:0 reg_hme_buf_addrx_6       // unsigned ,    RW, default = 32'h001c0000 mif_addr
#define FRC_REG_HME_BUF_ADDRX_7                    0x02c7
//Bit 31:0 reg_hme_buf_addrx_7       // unsigned ,    RW, default = 32'h001e0000 mif_addr
#define FRC_REG_HME_BUF_ADDRX_8                    0x02c8
//Bit 31:0 reg_hme_buf_addrx_8       // unsigned ,    RW, default = 32'h00200000 mif_addr
#define FRC_REG_HME_BUF_ADDRX_9                    0x02c9
//Bit 31:0 reg_hme_buf_addrx_9       // unsigned ,    RW, default = 32'h00220000 mif_addr
#define FRC_REG_HME_BUF_ADDRX_10                   0x02ca
//Bit 31:0 reg_hme_buf_addrx_10      // unsigned ,    RW, default = 32'h00240000 mif_addr
#define FRC_REG_HME_BUF_ADDRX_11                   0x02cb
//Bit 31:0 reg_hme_buf_addrx_11      // unsigned ,    RW, default = 32'h00260000 mif_addr
#define FRC_REG_HME_BUF_ADDRX_12                   0x02cc
//Bit 31:0 reg_hme_buf_addrx_12      // unsigned ,    RW, default = 32'h00280000 mif_addr
#define FRC_REG_HME_BUF_ADDRX_13                   0x02cd
//Bit 31:0 reg_hme_buf_addrx_13      // unsigned ,    RW, default = 32'h002a0000 mif_addr
#define FRC_REG_HME_BUF_ADDRX_14                   0x02ce
//Bit 31:0 reg_hme_buf_addrx_14      // unsigned ,    RW, default = 32'h002c0000 mif_addr
#define FRC_REG_HME_BUF_ADDRX_15                   0x02cf
//Bit 31:0 reg_hme_buf_addrx_15      // unsigned ,    RW, default = 32'h002e0000 mif_addr
#define FRC_REG_IP_LOGO_ADDRX_0                    0x02d0
//Bit 31:0 reg_ip_logo_addrx_0        // unsigned ,    RW, default = 32'h00300000 mif_addr
#define FRC_REG_IP_LOGO_ADDRX_1                    0x02d1
//Bit 31:0 reg_ip_logo_addrx_1        // unsigned ,    RW, default = 32'h00318000 mif_addr
#define FRC_REG_IP_LOGO_ADDRX_2                    0x02d2
//Bit 31:0 reg_ip_logo_addrx_2        // unsigned ,    RW, default = 32'h00330000 mif_addr
#define FRC_REG_IP_LOGO_ADDRX_3                    0x02d3
//Bit 31:0 reg_ip_logo_addrx_3        // unsigned ,    RW, default = 32'h00348000 mif_addr
#define FRC_REG_IP_LOGO_ADDRX_4                    0x02d4
//Bit 31:0 reg_ip_logo_addrx_4        // unsigned ,    RW, default = 32'h00360000 mif_addr
#define FRC_REG_IP_LOGO_ADDRX_5                    0x02d5
//Bit 31:0 reg_ip_logo_addrx_5        // unsigned ,    RW, default = 32'h00378000 mif_addr
#define FRC_REG_IP_LOGO_ADDRX_6                    0x02d6
//Bit 31:0 reg_ip_logo_addrx_6        // unsigned ,    RW, default = 32'h00390000 mif_addr
#define FRC_REG_IP_LOGO_ADDRX_7                    0x02d7
//Bit 31:0 reg_ip_logo_addrx_7        // unsigned ,    RW, default = 32'h003a8000 mif_addr
#define FRC_REG_IP_LOGO_ADDRX_8                    0x02d8
//Bit 31:0 reg_ip_logo_addrx_8        // unsigned ,    RW, default = 32'h003c0000 mif_addr
#define FRC_REG_IP_LOGO_ADDRX_9                    0x02d9
//Bit 31:0 reg_ip_logo_addrx_9        // unsigned ,    RW, default = 32'h003d8000 mif_addr
#define FRC_REG_IP_LOGO_ADDRX_10                   0x02da
//Bit 31:0 reg_ip_logo_addrx_10       // unsigned ,    RW, default = 32'h003f0000 mif_addr
#define FRC_REG_IP_LOGO_ADDRX_11                   0x02db
//Bit 31:0 reg_ip_logo_addrx_11       // unsigned ,    RW, default = 32'h00408000 mif_addr
#define FRC_REG_IP_LOGO_ADDRX_12                   0x02dc
//Bit 31:0 reg_ip_logo_addrx_12       // unsigned ,    RW, default = 32'h00420000 mif_addr
#define FRC_REG_IP_LOGO_ADDRX_13                   0x02dd
//Bit 31:0 reg_ip_logo_addrx_13       // unsigned ,    RW, default = 32'h00438000 mif_addr
#define FRC_REG_IP_LOGO_ADDRX_14                   0x02de
//Bit 31:0 reg_ip_logo_addrx_14       // unsigned ,    RW, default = 32'h00450000 mif_addr
#define FRC_REG_IP_LOGO_ADDRX_15                   0x02df
//Bit 31:0 reg_ip_logo_addrx_15       // unsigned ,    RW, default = 32'h00468000 mif_addr
#define FRC_REG_ME_LOGO_ADDRX_0                    0x02e0
//Bit 31:0 reg_me_logo_addrx_0        // unsigned ,    RW, default = 32'h00480000 mif_addr
#define FRC_REG_ME_LOGO_ADDRX_1                    0x02e1
//Bit 31:0 reg_me_logo_addrx_1        // unsigned ,    RW, default = 32'h00488000 mif_addr
#define FRC_REG_ME_LOGO_ADDRX_2                    0x02e2
//Bit 31:0 reg_me_logo_addrx_2        // unsigned ,    RW, default = 32'h00490000 mif_addr
#define FRC_REG_ME_LOGO_ADDRX_3                    0x02e3
//Bit 31:0 reg_me_logo_addrx_3        // unsigned ,    RW, default = 32'h00498000 mif_addr
#define FRC_REG_ME_LOGO_ADDRX_4                    0x02e4
//Bit 31:0 reg_me_logo_addrx_4        // unsigned ,    RW, default = 32'h004a0000 mif_addr
#define FRC_REG_ME_LOGO_ADDRX_5                    0x02e5
//Bit 31:0 reg_me_logo_addrx_5        // unsigned ,    RW, default = 32'h004a8000 mif_addr
#define FRC_REG_ME_LOGO_ADDRX_6                    0x02e6
//Bit 31:0 reg_me_logo_addrx_6        // unsigned ,    RW, default = 32'h004b0000 mif_addr
#define FRC_REG_ME_LOGO_ADDRX_7                    0x02e7
//Bit 31:0 reg_me_logo_addrx_7        // unsigned ,    RW, default = 32'h004b8000 mif_addr
#define FRC_REG_ME_LOGO_ADDRX_8                    0x02e8
//Bit 31:0 reg_me_logo_addrx_8        // unsigned ,    RW, default = 32'h004c0000 mif_addr
#define FRC_REG_ME_LOGO_ADDRX_9                    0x02e9
//Bit 31:0 reg_me_logo_addrx_9        // unsigned ,    RW, default = 32'h004c8000 mif_addr
#define FRC_REG_ME_LOGO_ADDRX_10                   0x02ea
//Bit 31:0 reg_me_logo_addrx_10       // unsigned ,    RW, default = 32'h004d0000 mif_addr
#define FRC_REG_ME_LOGO_ADDRX_11                   0x02eb
//Bit 31:0 reg_me_logo_addrx_11       // unsigned ,    RW, default = 32'h004d8000 mif_addr
#define FRC_REG_ME_LOGO_ADDRX_12                   0x02ec
//Bit 31:0 reg_me_logo_addrx_12       // unsigned ,    RW, default = 32'h004e0000 mif_addr
#define FRC_REG_ME_LOGO_ADDRX_13                   0x02ed
//Bit 31:0 reg_me_logo_addrx_13       // unsigned ,    RW, default = 32'h004e8000 mif_addr
#define FRC_REG_ME_LOGO_ADDRX_14                   0x02ee
//Bit 31:0 reg_me_logo_addrx_14       // unsigned ,    RW, default = 32'h004f0000 mif_addr
#define FRC_REG_ME_LOGO_ADDRX_15                   0x02ef
//Bit 31:0 reg_me_logo_addrx_15       // unsigned ,    RW, default = 32'h004f8000 mif_addr
#define FRC_REG_ME_NC_UNI_MV_ADDRX_0               0x02f0
//Bit 31:0 reg_me_nc_uni_mv_addrx_0        // unsigned ,    RW, default = 32'h01000000 mif_addr
#define FRC_REG_ME_NC_UNI_MV_ADDRX_1               0x02f1
//Bit 31:0 reg_me_nc_uni_mv_addrx_1        // unsigned ,    RW, default = 32'h010a0000 mif_addr
#define FRC_REG_ME_NC_UNI_MV_ADDRX_2               0x02f2
//Bit 31:0 reg_me_nc_uni_mv_addrx_2        // unsigned ,    RW, default = 32'h01140000 mif_addr
#define FRC_REG_ME_CN_UNI_MV_ADDRX_0               0x02f3
//Bit 31:0 reg_me_cn_uni_mv_addrx_0        // unsigned ,    RW, default = 32'h011e0000 mif_addr
#define FRC_REG_ME_CN_UNI_MV_ADDRX_1               0x02f4
//Bit 31:0 reg_me_cn_uni_mv_addrx_1        // unsigned ,    RW, default = 32'h01280000 mif_addr
#define FRC_REG_ME_PC_PHS_MV_ADDR                  0x02f5
//Bit 31:0 reg_me_pc_phs_mv_addrx          // unsigned ,    RW, default = 32'h01320000 mif_addr
#define FRC_REG_HME_NC_UNI_MV_ADDRX_0              0x02f6
//Bit 31:0 reg_hme_nc_uni_mv_addrx_0       // unsigned ,    RW, default = 32'h013c0000  mif_addr
#define FRC_REG_HME_NC_UNI_MV_ADDRX_1              0x02f7
//Bit 31:0 reg_hme_nc_uni_mv_addrx_1       // unsigned ,    RW, default = 32'h013ca000  mif_addr
#define FRC_REG_HME_NC_UNI_MV_ADDRX_2              0x02f8
//Bit 31:0 reg_hme_nc_uni_mv_addrx_2       // unsigned ,    RW, default = 32'h013d4000  mif_addr
#define FRC_REG_HME_CN_UNI_MV_ADDRX_0              0x02f9
//Bit 31:0 reg_hme_cn_uni_mv_addrx_0       // unsigned ,    RW, default = 32'h013de000  mif_addr,
#define FRC_REG_HME_CN_UNI_MV_ADDRX_1              0x02fa
//Bit 31:0 reg_hme_cn_uni_mv_addrx_1       // unsigned ,    RW, default = 32'h013e8000  mif_addr
#define FRC_REG_HME_PC_PHS_MV_ADDR                 0x02fb
//Bit 31:0 reg_hme_pc_phs_mv_addrx         // unsigned ,    RW, default = 32'h01400000  mif_addr
#define FRC_REG_VP_PF_UNI_MV_ADDR                  0x02fc
//Bit 31:0 reg_vp_pf_uni_mv_addrx          // unsigned ,    RW, default = 32'h01410000  mif_addr
#define FRC_REG_LOGO_IIR_BUF_ADDR                  0x02fd
//Bit 31:0 reg_logo_iir_buf_baddr          // unsigned ,    RW, default = 32'h01500000 mif_addr
#define FRC_REG_LOGO_SCC_BUF_ADDR                  0x02fe
//Bit 31:0 reg_logo_scc_buf_baddr          // unsigned ,    RW, default = 32'h01600000  mif_addr
// Closing file:  ./frc_inc/frc_buff_addr_regs.h
//
// -----------------------------------------------
// REG_BASE:  FRC_INP_TOP_APB_BASE = 0x04
// -----------------------------------------------
//
// Reading file:  ./frc_inc/frc_inp_top_reg.h
#define FRC_REG_ME_BLD_COEF                        0x0400
//Bit 31            reserved
//Bit 30           reg_me_yc_bld_mode        // unsigned ,    RW, default = 0  yc blending mode, 0: cb/cr; 1: max(r,g,b)
//Bit 29:24        reg_me_bld_coef_0         // unsigned ,    RW, default = 12  blending coef of yuv to get yuv blended data for ME, normalized to 32 as 1 for Y
//Bit 23:18        reg_me_bld_coef_1         // unsigned ,    RW, default = 4  blending coef of yuv to get yuv blended data for ME, normalized to 32 as 1 for Y
//Bit 17:12        reg_me_bld_coef_2         // unsigned ,    RW, default = 4  blending coef of yuv to get yuv blended data for ME, normalized to 32 as 1 for Y
//Bit 11: 8        reserved
//Bit  7: 4        reg_me_dsx_ofset          // signed ,    RW, default = 0  horizontal pixel offset for the input pixel to downsample filter
//Bit  3: 0        reg_me_dsy_ofset          // signed ,    RW, default = 0  vertical pixel offset for the input pixel to downsample filter
#define FRC_REG_CORING                             0x0401
//Bit 31: 8        reserved
//Bit  7: 0        reg_ds_glb_motion_coring_th // unsigned ,    RW, default = 4  coring threshold for global motion calc.
#define FRC_REG_ME_DS_COEF_0                       0x0402
//Bit 31:16        reserved
//Bit 15: 8        reg_me_dsx_coef_0         // signed ,    RW, default = 24  coef of AA filter for horizontal downsampling of blended data, normalized to 128 as 1
//Bit  7: 0        reg_me_dsy_coef_0         // signed ,    RW, default = 24  coef of AA filter for vertical downsampling of blended data, normalized to 128 as 1
#define FRC_REG_ME_DS_COEF_1                       0x0403
//Bit 31:16        reserved
//Bit 15: 8        reg_me_dsx_coef_1         // signed ,    RW, default = 20  coef of AA filter for horizontal downsampling of blended data, normalized to 128 as 1
//Bit  7: 0        reg_me_dsy_coef_1         // signed ,    RW, default = 20  coef of AA filter for vertical downsampling of blended data, normalized to 128 as 1
#define FRC_REG_ME_DS_COEF_2                       0x0404
//Bit 31:16        reserved
//Bit 15: 8        reg_me_dsx_coef_2         // signed ,    RW, default = 16  coef of AA filter for horizontal downsampling of blended data, normalized to 128 as 1
//Bit  7: 0        reg_me_dsy_coef_2         // signed ,    RW, default = 16  coef of AA filter for vertical downsampling of blended data, normalized to 128 as 1
#define FRC_REG_ME_DS_COEF_3                       0x0405
//Bit 31:16        reserved
//Bit 15: 8        reg_me_dsx_coef_3         // signed ,    RW, default = 16  coef of AA filter for horizontal downsampling of blended data, normalized to 128 as 1
//Bit  7: 0        reg_me_dsy_coef_3         // signed ,    RW, default = 16  coef of AA filter for vertical downsampling of blended data, normalized to 128 as 1
#define FRC_REG_GLB_MOTION                         0x041b
//Bit 31: 0        ro_ds_glb_motion          // unsigned ,    RO, default = 0  global motion based on downsample data
// Closing file:  ./frc_inc/frc_inp_top_reg.h

// Reading file:  ./frc_inc/frc_inp_hw_reg.h
#define FRC_INP_UE_CLR                             0x0450
//Bit 31:6          reserved
//Bit 5:0           pls_inp_ue_clr          // unsigned ,   WO, default = 0 error flag clear
#define FRC_INP_UE_DBG                             0x0451
//Bit 31:6          reserved
//Bit 5:0           ro_inp_ue_dbg           // unsigned ,    RO, default = 0 inp undone error flag
#define FRC_REG_INP_HS_DBG1                        0x0452
//Bit 31:0          ro_inp_hs_dbg1         // unsigned ,    RO, default = 0 inp handshake package
#define FRC_REG_INP_HS_DBG2                        0x0453
//Bit 31:0          ro_inp_hs_dbg2         // unsigned ,    RO, default = 0 inp handshake package
#define FRC_REG_INP_HS_DBG3                        0x0454
//Bit 31:0          ro_inp_hs_dbg3         // unsigned ,    RO, default = 0 inp handshake package
#define FRC_REG_MIF_INT_FLAG_DBG                   0x0455
//Bit 31:28         reserved
//Bit 27:0          ro_mif_int_flag_dbg    // unsigned ,    RO, default = 0 inp handshake package
#define FRC_REG_INP_CTRL1                          0x0456
//Bit 31:28         reserved
//Bit 27            reg_osd_pat_gen_en         // unsigned ,   RW, default = 0  reg_osd_pat_gen_en
//Bit 26            reg_osd_pat_gen_mode       // unsigned ,   RW, default = 0  reg_osd_pat_gen_mode
//Bit 25:13         reg_pat_gen_xyxy_v0        // unsigned ,   RW, default = 0  reg_pat_gen_xyxy_v0
//Bit 12:0          reg_pat_gen_xyxy_h0        // unsigned ,   RW, default = 0  reg_pat_gen_xyxy_h0
#define FRC_REG_INP_CTRL2                          0x0457
//Bit 31:26         reserved
//Bit 25:13         reg_pat_gen_xyxy_v1        // unsigned ,   RW, default = 0  reg_pat_gen_xyxy_v1
//Bit 12:0          reg_pat_gen_xyxy_h1        // unsigned ,   RW, default = 0  reg_pat_gen_xyxy_h1
#define FRC_REG_INP_GCLK_CTRL                      0x0458
//Bit 31:22         reserved
//Bit 21:20         reg_inp_mc_hds_gclk_ctrl   // unsigned,   RW, default = 0 clk gating ctrl signal
//Bit 19:18         reg_inp_hw_reg_gclk_ctrl   // unsigned,   RW, default = 0 clk gating ctrl signal
//Bit 17:16         reg_inp_top_reg_gclk_ctrl  // unsigned,   RW, default = 0 clk gating ctrl signal
//Bit 15:14         reg_cur_wbuf_gclk_ctrl     // unsigned,   RW, default = 0 clk gating ctrl signal
//Bit 13:12         reg_blend_gclk_ctrl        // unsigned,   RW, default = 0 clk gating ctrl signal
//Bit 11:10         reg_menr_gclk_ctrl         // unsigned,   RW, default = 0 clk gating ctrl signal
//Bit 9:8           reg_inp_reg_gclk_ctrl      // unsigned,   RW, default = 0 clk gating ctrl signal
//Bit 7:6           reg_hme_dsc_gclk_ctrl      // unsigned,   RW, default = 0 clk gating ctrl signal
//Bit 5:4           reg_me_dsc_gclk_ctrl       // unsigned,   RW, default = 0 clk gating ctrl signal
//Bit 3:2           reg_inp_osd_gclk_ctrl      // unsigned,   RW, default = 0 clk gating ctrl signal
//Bit 1:0           reg_inp_fmt_gclk_ctrl      // unsigned,   RW, default = 0 clk gating ctrl signal
#define FRC_REG_INP_MODULE_EN                      0x0459
//Bit 31:14         reserved
//Bit 13            reg_fmt_en                 // unsigned,   RW, default = 1 yuv444toyuv422 convert for mc enable signal,active high
//Bit 12:10         reg_inp_fmt422_mode        // unsigned,   RW, default = 0 input fmt yuv444 or yuv422
//Bit 9             reg_mc_nr_en               // unsigned,   RW, default = 0 mc nr enable signal,active high
//Bit 8             reg_inp_logo_en            // unsigned,   RW, default = 0 iplogo data path enable signal,active high
//Bit 7             reg_inp_bbd_en             // unsigned,   RW, default = 0 bb detection enable signal,active high
//Bit 6             reg_inp_pat_gen_en         // unsigned,   RW, default = 0 pat_gen mode enable signal,active high
//Bit 5             reg_menr_en                // unsigned,   RW, default = 1 me nr enable signal,active high
//Bit 4             reg_me_vdsc_en             // unsigned,   RW, default = 1 vertical downsample of blend for me enale signal,active high
//Bit 3             reg_me_hdsc_en             // unsigned,   RW, default = 1 horizontal downsample of blend for me enale signal,active high
//Bit 2             reg_hme_vdsc_en            // unsigned,   RW, default = 1 vertical downsample of blend for hme enale signal,active high
//Bit 1             reg_hme_hdsc_en            // unsigned,   RW, default = 1 horizontal downsample of blend for hme enale signal,active high
//Bit 0             reg_inp_bld_fmt422_mode    // unsigned,   RW, default = 0 input fmt of blend,0-yuv444,1-yuv422
#define FRC_REG_INP_DBG_CTRL1                      0x045a
//Bit 31:16         reserved
//Bit 15:12         reg_inp_dbg_ctrl          // unsigned ,    RW, default = 0  reg_inp_dbg_ctrl
//Bit 11: 0         reserved
#define FRC_REG_INP_DBG_CTRL2                      0x045b
//Bit 31:30         reserved
//Bit 29:0          reg_inp_dbg_data          // unsigned ,    RW, default = 0  reg_inp_dbg_data
#define FRC_REG_INP_DBG_CTRL3                      0x045c
//Bit 31:30         reg_osd_force_en          // unsigned ,    RW, default = 0  osd debug force enable
//Bit 29: 0         reg_osd_force_yuv         // unsigned ,    RW, default = 0  osd debug force color
#define FRC_REG_INP_MIF_GCLK_CTRL                  0x045d
//Bit 31:20         reserved
//Bit 19:18         reg_hme_wrmif_gclk_ctrl    // unsigned,   RW, default = 0 clk gating ctrl signal
//Bit 17:16         reg_logo_rdmif0_gclk_ctrl  // unsigned,   RW, default = 0 clk gating ctrl signal
//Bit 15:14         reg_logo_rdmif1_gclk_ctrl  // unsigned,   RW, default = 0 clk gating ctrl signal
//Bit 13:12         reg_logo_wrmif0_gclk_ctrl  // unsigned,   RW, default = 0 clk gating ctrl signal
//Bit 11:10         reg_logo_wrmif1_gclk_ctrl  // unsigned,   RW, default = 0 clk gating ctrl signal
//Bit 9:8           reg_melogo_wrmif_gclk_ctrl // unsigned,   RW, default = 0 clk gating ctrl signal
//Bit 7:6           reg_iplogo_wrmif_gclk_ctrl // unsigned,   RW, default = 0 clk gating ctrl signal
//Bit 5:4           reg_rd_arb_gclk_ctrl       // unsigned,   RW, default = 0 clk gating ctrl signal
//Bit 3:2           reg_wr_arb_1x4_gclk_ctrl   // unsigned,   RW, default = 0 clk gating ctrl signal
//Bit 1:0           reg_wr_arb_1x8_gclk_ctrl   // unsigned,   RW, default = 0 clk gating ctrl signal
#define FRC_INP_LOSS_SLICE_SEC                     0x045e
//Bit 31:1          reserved
//Bit 0             reg_inp_loss_slice_sec    // unsigned,   RW, default = 0,1:same as lossy-body 0:non_security
#define FRC_INP_MC_HDS_CTRL0                       0x0470
//Bit 31:4          reserved
//Bit 3:0           reg_inp_mc_hds_dsx_ofset   // signed ,    RW, default = 0  horizontal pixel offset for the input pixel to downsample filter
#define FRC_INP_MC_HDS_WIN_SETTING_SCP             0x0471
//Bit 31:16         reg_inp_mc_hds_hscp0       // unsigned ,    RW, default = 0
//Bit 15: 0         reg_inp_mc_hds_hscp1       // unsigned ,    RW, default = 0
#define FRC_INP_MC_HDS_COEFX3                      0x0472
//Bit 31:24         reg_inp_mc_hds_coef_3      // signed ,    RW, default = 16  coef of AA filter for horizontal downsampling of csc_out data, normalized to 128 as 1
//Bit 23:16         reg_inp_mc_hds_coef_2      // signed ,    RW, default = 16  coef of AA filter for horizontal downsampling of csc_out data, normalized to 128 as 1
//Bit 15:8          reg_inp_mc_hds_coef_1      // signed ,    RW, default = 16  coef of AA filter for horizontal downsampling of csc_out data, normalized to 128 as 1
//Bit 7:0           reg_inp_mc_hds_coef_0      // signed ,    RW, default = 16  coef of AA filter for horizontal downsampling of csc_out data, normalized to 128 as 1
#define FRC_INP_CUR_ME_ORECON_EN                   0x0473
//Bit 31:1          reserved
//Bit 0             reg_cur_me_orecon_en       // unsigned ,  RW, default = 1
#define FRC_INP_PD_DIFF_MODE                       0x0474
//Bit 31:2          reserved
//Bit 1:0           reg_pd_diff_mode           // unsigned ,  RW, default = 0
#define FRC_INP_HME_RAM_DLY_DEPTH                  0x0475
//Bit 31:13         reserved
//Bit 12:0          reg_inp_me_ram_dly_depth   // unsigned ,  RW, default = 200
#define FRC_INP_MCDW_CTRL                          0x047a
//Bit 31:29         reserved
//Bit 28            reg_mcdw_lpf_mode          // unsigned , RW, default = 0
//Bit 27:26         reserved
//Bit 25:24         reg_mcdw_ds_mode_hv        // unsigned , RW, default = 3   0:h/2   1:v/2
//Bit 23:18         reserved
//Bit 17:8          reg_mcdw_loss_slice_num    // unsigned , RW, default = 4
//Bit 7:5           reserved
//Bit 4             reg_mcdw_loss_en           // unsigned , RW, default = 0
//Bit 3:1           reserved
//Bit 0             reg_mcdw_path_en           // unsigned , RW, default = 0,1:open mc double write path
#define FRC_INP_PATH_OPT                           0x047c
//Bit 31:5          reserved
//Bit 4             reg_inp_mc_nr_opt          // unsigned , RW, default = 0
//Bit 3             reg_inp_bbd_opt            // unsigned , RW, default = 0
//Bit 2             reg_frc_me_loss_opt        // unsigned , RW, default = 0
//Bit 1             reg_frc_mc_loss_opt        // unsigned , RW, default = 0
//Bit 0             reg_mcwr_path_opt          // unsigned , RW, default = 0
// Closing file:  ./frc_inc/frc_inp_hw_reg.h
// Reading file:  ./frc_inc/frc_inp_csc_regs.h
#define FRC_INP_CSC_CTRL                           0x04e0
//Bit 31: 10       reserved
//Bit 9 : 8        reg_glk_ctrl      // unsigned ,    RW, default = 0  csc reg_glk_ctrl enable 2'b00:gating 2'b01:close 2'b1x:always open
//Bit 7: 5         reserved
//Bit  4           reg_sync_en       // unsigned ,    RW, default = 0  reg_csc_en sync enable
//Bit  3           reg_csc_en        // unsigned ,    RW, default = 1  enable rgb2yuv matrix for ip pattern generation
//Bit 2:0          reg_csc_rs        // unsigned ,    RW, default = 0  0: normalized to 1024 as 1; 1: norm to 2048; 2: norm to 4096; 3: norm to 8192
#define FRC_INP_CSC_OFFSET_INP01                   0x04e5
//Bit 31:29        reserved
//Bit 28:16        reg_csc_offst_inp_0 // signed ,    RW, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_csc_offst_inp_1 // signed ,    RW, default = 0
#define FRC_INP_CSC_OFFSET_INP2                    0x04e6
//Bit 31:13        reserved
//Bit 12: 0        reg_csc_offst_inp_2 // signed ,    RW, default = 0
#define FRC_INP_CSC_COEF_00_01                     0x04e7
//Bit 31:29        reserved
//Bit 28:16        reg_csc_coef_0_0  // signed ,    RW, default = 218
//Bit 15:13        reserved
//Bit 12: 0        reg_csc_coef_0_1  // signed ,    RW, default = 732
#define FRC_INP_CSC_COEF_02_10                     0x04e8
//Bit 31:29        reserved
//Bit 28:16        reg_csc_coef_0_2  // signed ,    RW, default = 74
//Bit 15:13        reserved
//Bit 12: 0        reg_csc_coef_1_0  // signed ,    RW, default = -117
#define FRC_INP_CSC_COEF_11_12                     0x04e9
//Bit 31:29        reserved
//Bit 28:16        reg_csc_coef_1_1  // signed ,    RW, default = -395
//Bit 15:13        reserved
//Bit 12: 0        reg_csc_coef_1_2  // signed ,    RW, default = 512
#define FRC_INP_CSC_COEF_20_21                     0x04ea
//Bit 31:29        reserved
//Bit 28:16        reg_csc_coef_2_0  // signed ,    RW, default = 512
//Bit 15:13        reserved
//Bit 12: 0        reg_csc_coef_2_1  // signed ,    RW, default = -465
#define FRC_INP_CSC_COEF_22                        0x04eb
//Bit 31:13        reserved
//Bit 12: 0        reg_csc_coef_2_2  // signed ,    RW, default = -48
#define FRC_INP_CSC_OFFSET_OUP01                   0x04ec
//Bit 31:29        reserved
//Bit 28:16        reg_csc_offst_oup_0 // signed ,    RW, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_csc_offst_oup_1 // signed ,    RW, default = 512
#define FRC_INP_CSC_OFFSET_OUP2                    0x04ee
//Bit 31:13        reserved
//Bit 12: 0        reg_csc_offst_oup_2 // signed ,    RW, default = 512
//Closing file:  ./frc_inc/frc_inp_csc_regs.h
//
#define FRC_IPLOGO_EN                              0x0503
//Bit 31:25        reserved
//Bit 24           reg_iplogo_imgiir_en      // unsigned ,    RW, default = 1  dft=1; 0: imgiir disable, use cur and pre diff  1: imgiir enable;
//Bit 23:20        reg_iplogo_osdbit         // unsigned ,    RW, default = 0  dft=0; b0: logo_ip_blklogo_detection, b1:remap_to_1b, b2:dilation, b3: erosion
//Bit 19           reg_iplogo_edge_dir_en_0  // unsigned ,    RW, default = 1  dft=1; 1: open 0   degree
//Bit 18           reg_iplogo_edge_dir_en_1  // unsigned ,    RW, default = 1  dft=1; 1: open 45  degree
//Bit 17           reg_iplogo_edge_dir_en_2  // unsigned ,    RW, default = 1  dft=1; 1: open 90  degree
//Bit 16           reg_iplogo_edge_dir_en_3  // unsigned ,    RW, default = 1  dft=1; 1: open 135 degree
//Bit 15           reg_iplogo_scc_gray_dif_en // unsigned ,    RW, default = 1  dft=1;  scc_gray_dif  en
//Bit 14           reg_iplogo_scc_sad4_corr_en // unsigned ,    RW, default = 1  dft=1;  scc_sad4_corr en
//Bit 13           reg_iplogo_pxl_clr_en     // unsigned ,    RW, default = 1  dft=1;  pxl clr master en
//Bit 12           reg_iplogo_blk_clr_en     // unsigned ,    RW, default = 1  dft=1;  blk clr master en
//Bit 11            reserved
//Bit 10           reg_iplogo_edge_strength_clr_en // unsigned ,    RW, default = 1  dft=1;  0: edge_strength_clr disable, 1: edge_strength_clr enable
//Bit  9           reg_iplogo_edge_dif_clr_en // unsigned ,    RW, default = 1  dft=0;  0: edge_dif_clr disable, 1: edge_dif_clr enable
//Bit  8           reg_iplogo_gray_dif_clr_en // unsigned ,    RW, default = 1  dft=1;  0: gray_dif_clr disable, 1: gray_dif_clr enable
//Bit  7           reg_iplogo_blk_logodir4_corr_clr_en // unsigned ,    RW, default = 1  dft=1;  0: blk_logodir4_corr_clr disable, 1: blk_logodir4_corr_clr enable
//Bit  6           reg_iplogo_blk_edgedir4_corr_clr_en // unsigned ,    RW, default = 1  dft=1;  0: blk_edgedir4_corr_clr disable, 1: blk_edgedir4_corr_clr enable
//Bit  5           reg_iplogo_blk_dir4_clr_scc_en // unsigned ,    RW, default = 0  dft=1;  0: blk dir4 ratio clr disable, 1: blk dir4 ratio clr enable
//Bit  4           reg_iplogo_blk_disappear_clr_scc_en // unsigned ,    RW, default = 1  dft=0;  0: blk sudden disappear clr disable, 1: blk sudden disappear clr enable
//Bit  3           reg_iplogo_scc_remap_imclose_en // unsigned ,    RW, default = 1  dft=1;  0: ip pxllogo imclose disable, 1: ip pxllogo imclose enable
//Bit  2           reg_iplogo_blk_dil_en     // unsigned ,    RW, default = 1  dft=1;  0: ip blklogo dilate disable, 1: ip blklogo dilate enable
//Bit  1           reg_iplogo_blk_ero_en     // unsigned ,    RW, default = 1  dft=1;  0: ip blklogo erosion disable, 1: ip blklogo erosion enable
//Bit  0           reg_iplogo_lpf_en         // unsigned ,    RW, default = 0  dft=0;  0: ip pxllogo glpf disable, 1: ip pxllogo glpf enable
#define FRC_IPLOGO_REGION_HWINDOW_2                0x0586
#define FRC_IPLOGO_REGION_HWINDOW_1                0x0587
#define FRC_IPLOGO_REGION_HWINDOW_0                0x0588
#define FRC_IPLOGO_REGION_VWINDOW_1                0x0589
#define FRC_IPLOGO_REGION_VWINDOW_0                0x058a
#define FRC_BBD_DETECT_DETAIL_H_TOP2BOT            0x0606
#define FRC_BBD_DETECT_DETAIL_H_LFT2RIT            0x0607
#define FRC_BBD_DETECT_DETAIL_V_TOP2BOT            0x0608
#define FRC_BBD_DETECT_DETAIL_V_LFT2RIT            0x0609
#define FRC_BBD_DETECT_MOTION_REGION_TOP2BOT       0x060a
#define FRC_BBD_DETECT_MOTION_REGION_LFT2RIT       0x060b
#define FRC_BBD_DETECT_REGION_TOP2BOT              0x0604
#define FRC_BBD_DETECT_REGION_LFT2RIT              0x0605
#define FRC_BBD_DETECT_MOTION_REGION_TOP2BOT       0x060a
#define FRC_BBD_DETECT_MOTION_REGION_LFT2RIT       0x060b
#define FRC_IPLOGO_BB_PIX_ST                       0x0501
#define FRC_IPLOGO_BB_PIX_ED                       0x0502
#define FRC_BBD_APL_HIST_WIN_LFT_TOP               0x062b
#define FRC_BBD_APL_HIST_WIN_RIT_BOT               0x062c
#define FRC_BBD_OOB_APL_CAL_LFT_TOP_RANGE          0x0611
#define FRC_BBD_OOB_APL_CAL_RIT_BOT_RANGE          0x0612
#define FRC_BBD_OOB_DETAIL_WIN_LFT_TOP             0x0614
#define FRC_BBD_OOB_DETAIL_WIN_RIT_BOT             0x0615
#define FRC_BBD_OOB_V_DETAIL_WIN_LFT_TOP           0x0616
#define FRC_BBD_OOB_V_DETAIL_WIN_RIT_BOT           0x0617
#define FRC_BBD_MOTION_DETEC_REGION_LFT_TOP_DS     0x0619
#define FRC_BBD_MOTION_DETEC_REGION_RIT_BOT_DS     0x061a
#define FRC_BBD_FLATNESS_DETEC_REGION_LFT_TOP      0x061e
#define FRC_BBD_FLATNESS_DETEC_REGION_RIT_BOT      0x061f
#define FRC_BBD_RO_HIST_IDX                        0x0694
#define FRC_BBD_RO_MAX1_HIST_CNT                   0x0695
#define FRC_BBD_RO_MAX2_HIST_CNT                   0x0696
#define FRC_BBD_RO_MIN1_HIST_CNT                   0x0697
#define FRC_BBD_RO_APL_GLB_SUM                     0x0698
#define FRC_FD_DIF_GL                              0x071e
//Bit 31: 0        ro_fd_glb_mot_all         // unsigned ,    RO, default = 0  global ,difference of cur and pre
#define FRC_FD_DIF_COUNT_GL                        0x071f
//Bit 31:20        reserved
//Bit 19: 0        ro_fd_mot_count_all       // unsigned ,    RO, default = 0  global ,count of difference of cur and pre
#define FRC_FD_DIF_GL_FILM                         0x0720
//Bit 31: 0        ro_fd_glb_mot_all_film    // unsigned ,    RO, default = 0  global ,difference of cur and pre
#define FRC_FD_DIF_COUNT_GL_FILM                   0x0721
//Bit 31:20        reserved
//Bit 19: 0        ro_fd_mot_count_all_film  // unsigned ,    RO, default = 0  global ,count of difference of cur and pre
#define FRC_NR_MISC                                0x0800
//Bit 31: 0        reg_nr_misc               // unsigned ,    RW, default = 0  register

#define FRC_ARB_UGT_RD_BASIC                       0x0974
//Bit 7: 0         module_urg+arb_ugt_basic  // unsigned ,    RW, default = 0x55
#define FRC_ARB_UGT_WR_BASIC                       0x0994
//Bit 3: 0         module_urg+arb_ugt_basic  // unsigned ,    RW, default = 5

#define FRC_MEVP_CTRL0                             0x1080
//Bit 31            reg_mevp_clr_me_undone_flag  //unsigned  , RW, default = 0 ,me_process undone flag clear, write pulse
//Bit 30:4          reserved
//Bit 3 :1          reg_mevp_wrmif_en            //unsigned  , RW, default = 7 ,
//Bit 0             reg_hme_en                   //unsigned  , RW, default = 1 ,
#define FRC_MEVP_SW_RESETS                         0x1084
//Bit 31:24         reserved
//Bit 23: 0         reg_mevp_sw_resets           // unsigned , RW, default = 0 ,
#define FRC_MEVP_RO_STAT0                          0x1088
//Bit 31:1          reserved
//Bit  0            ro_me_undone_flag            // unsigned , RO , default = 0  me_process undone flag

#define FRC_ME_EN                                  0x1100
//Bit 31           reg_me_en                 // unsigned ,    RW, default = 0  enable me function.
//Bit 30           reg_me_lpf_en             // unsigned ,    RW, default = 0  lpf enable for me data
//Bit 29            reserved
//Bit 28:24        reg_me_max_num_cand_me    // unsigned ,    RW, default = 13  maximum number for ME candidates
//Bit 23:16        reserved
//Bit 15            reserved
//Bit 14:12        reg_me_blksize_x          // unsigned ,    RW, default = 2  block size x(2^reg) in ME sub module under the Buf_blend data (downsampled); default = 2;
//Bit 11            reserved
//Bit 10: 8        reg_me_blksize_y          // unsigned ,    RW, default = 2  block size y(2^reg) in ME sub module under the Buf_blend data (downsampled); default = 2;
//Bit  7: 6        reserved
//Bit  5: 4        reg_me_mvx_div_mode       // unsigned ,    RW, default = 0  0: MVx has 2bits decimal; 1: 3bits decimal; 2: 4bits decimal @ME resolution.
//Bit  3: 2        reserved
//Bit  1: 0        reg_me_mvy_div_mode       // unsigned ,    RW, default = 0  0: MVy has 2bits decimal; 1: 3bits decimal; 2: 4bits decimal @ME resolution.
#define FRC_ME_BB_PIX_ED                           0x1108
//Bit 31:28        reserved
//Bit 27:16        reg_me_bb_xyxy_2          // unsigned ,    RW, default = 959  me part black bar index x_ed updated by firmware, xsize/(2^prm_me->reg_me_dsx_scale) - 1;
//Bit 15:12        reserved
//Bit 11: 0        reg_me_bb_xyxy_3          // unsigned ,    RW, default = 539  me part black bar index y_ed updated by firmware, ysize/(2^prm_me->reg_me_dsy_scale) - 1;
#define FRC_ME_BB_BLK_ED                           0x110a
//Bit 31:26        reserved
//Bit 25:16        reg_me_bb_blk_xyxy_2      // unsigned ,    RW, default = 239  me part black bar index x_ed updated by firmware, (xsize/(2^prm_me.reg_me_dsx_scale))/prm_me.reg_me_blksize_x - 1;
//Bit 15:10        reserved
//Bit  9: 0        reg_me_bb_blk_xyxy_3      // unsigned ,    RW, default = 134  me part black bar index y_ed updated by firmware, (ysize/(2^prm_me.reg_me_dsy_scale))/prm_me.reg_me_blksize_y - 1;
#define FRC_ME_STAT_12R_HST                        0x110b
//Bit 31:24        reg_gmv_rough_max_dst_th  // unsigned ,    RW, default = 0  rough max distance threshold for gmv calc.
//Bit 23:16        reg_gmv_finer_max_dst_th  // unsigned ,    RW, default = 3  finer max distance threshold for gmv calc.
//Bit 15:12        reserved
//Bit 11:10        reg_region_rp_use_neighbor_gmv_mode // unsigned ,    RW, default = 2  mode of use region rp gmv, 0: self-region 1: check neighbor region when self-region is invalid, 2: check 4 region rp gmv
//Bit  9: 0        reg_me_stat_region_hstart // unsigned ,    RW, default = 0  me statistic region horizontal start, updated by fw
#define FRC_ME_STAT_12R_H01                        0x110c
//Bit 31:26        reserved
//Bit 25:16        reg_me_stat_region_hend_0 // unsigned ,    RW, default = 59  me statistic region horizontal end0, relative to hstart, updated by fw
//Bit 15:10        reserved
//Bit  9: 0        reg_me_stat_region_hend_1 // unsigned ,    RW, default = 118  me statistic region horizontal end1, relative to hstart, updated by fw
#define FRC_ME_STAT_12R_H23                        0x110d
//Bit 31:26        reserved
//Bit 25:16        reg_me_stat_region_hend_2 // unsigned ,    RW, default = 177  me statistic region horizontal end2, relative to hstart, updated by fw
//Bit 15:10        reserved
//Bit  9: 0        reg_me_stat_region_hend_3 // unsigned ,    RW, default = 239  me statistic region horizontal end3, relative to hstart, updated by fw
#define FRC_ME_STAT_12R_V0                         0x110e
//Bit 31:26        reserved
//Bit 25:16        reg_me_stat_region_vstart // unsigned ,    RW, default = 0  me statistic region vertical start, updated by fw
//Bit 15:10        reserved
//Bit  9: 0        reg_me_stat_region_vend_0 // unsigned ,    RW, default = 44  me statistic region vertical end0, relative to vstart, updated by fw
#define FRC_ME_STAT_12R_V1                         0x110f
//Bit 31:26        reserved
//Bit 25:16        reg_me_stat_region_vend_1 // unsigned ,    RW, default = 88  me statistic region vertical end1, relative to vstart, updated by fw
//Bit 15:10        reserved
//Bit  9: 0        reg_me_stat_region_vend_2 // unsigned ,    RW, default = 134  me statistic region vertical end2, relative to vstart, updated by fw
#define FRC_ME_GCV_EN                              0x1119
#define FRC_ME_GCV2_EN                             0x113b
#define FRC_ME_CMV_MAX_MV                          0x11a0
//Bit 31:28        reserved
//Bit 27:16        reg_me_cmv_max_mvx       // unsigned ,    RW, default = 511         cmv max_mvx
//Bit 15:11        reserved
//Bit 10:0         reg_me_cmv_max_mvy       // unsigned ,    RW, default = 160         cmv max_mvx
#define FRC_ME_CMV_CTRL                            0x11a1
//Bit 31           reg_me_cmv_rand_pulse    // unsigned ,    RW, default = 0         cmv_misc
//Bit 30:0         reg_me_cmv_ctrl          // unsigned ,    RW, default = 255       cmv_misc 10:max_mv_sel 7:fs_en 6:proj_en 5:zmv_en 4:gmv_en 3:hier_en 2:rand_en 1:st1_en 0:st0_en
#define FRC_ME_REGION_RP_GMV_2                     0x1708
#define FRC_NOW_SRCH_REG                           0x308e
#define FRC_ME_GMV_CTRL                            0x1702
#define FRC_VP_BB_1                                0x1e03
//Bit 31:16        reg_vp_bb_xyxy_1          // unsigned ,    RW, default = 0  black bar block index of top
//Bit 15: 0        reg_vp_bb_xyxy_0          // unsigned ,    RW, default = 0  black bar block index of left
#define FRC_VP_BB_2                                0x1e04
//Bit 31:16        reg_vp_bb_xyxy_3          // unsigned ,    RW, default = 134  black bar block index of bottom
//Bit 15: 0        reg_vp_bb_xyxy_2          // unsigned ,    RW, default = 239  black bar block index of right
#define FRC_VP_ME_BB_1                             0x1e05
#define FRC_VP_ME_BB_2                             0x1e06
#define FRC_VP_REGION_WINDOW_1                     0x1e58
#define FRC_VP_REGION_WINDOW_2                     0x1e59
#define FRC_VP_REGION_WINDOW_3                     0x1e5a
#define FRC_VP_REGION_WINDOW_4                     0x1e5b
#define FRC_MELOGO_REGION_HWINDOW_2                0x1f11
#define FRC_MELOGO_REGION_HWINDOW_1                0x1f12
#define FRC_MELOGO_REGION_HWINDOW_0                0x1f13
#define FRC_MELOGO_REGION_VWINDOW_1                0x1f14
#define FRC_MELOGO_REGION_VWINDOW_0                0x1f15
#define FRC_VP_TOP_STAT                            0x1ef7
#define FRC_VP_TOP_CLR_STAT                        0x1ef8
#define FRC_MELOGO_BB_BLK_ST                       0x1f02
#define FRC_MELOGO_BB_BLK_ED                       0x1f03
#define FRC_MC_SETTING1                            0x3000
#define FRC_MC_SETTING2                            0x3001
#define FRC_MC_LOSS_SLICE_SEC                      0x3905
#define FRC_MC_PRB_CTRL1                           0x3989
//Bit 31            reserved
//Bit 30            reg_mc_probe_en           // unsigned ,    RW, default = 0    reg_mc_probe_en
//Bit 29            reg_mc_probe_en_csc       // unsigned ,    RW, default = 0    reg_mc_probe_en_csc
//Bit 28:16         reg_mc_probe_pt_y         // unsigned ,    RW, default = 100  reg_mc_probe_pt_y
//Bit 15:13         reserved
//Bit 12:0          reg_mc_probe_pt_x         // unsigned ,    RW, default = 100  reg_mc_probe_pt_x
#define FRC_SRCH_RNG_MODE                          0x3003
//Bit 31: 8        reserved
//Bit  7: 4        reg_mc_luma_srch_rng_mode // unsigned ,    RW, default = 0  search range mode
//Bit  3: 0        reg_mc_chrm_srch_rng_mode // unsigned ,    RW, default = 0  search range mode
#define FRC_MC_H2V2_SETTING                        0x3098
//Bit 31           reg_mc_get_lbuf_wrap_lpf_mode get lbuf warp lpf mode: 0, [0 4 0],  1, [1 2 1];
//Bit 30           reg_mc_srch_rng_luma_scale_en // unsigned , RW, default = 1  h2v2 luma en
//Bit 29           reg_mc_srch_rng_luma_scale_force_en  RW, default = 0  h2v2 luma force setting
//Bit 28           reg_mc_srch_rng_luma_abv_xscale // unsigned ,    RW, default = 0  h2v2 abv x en
//Bit 27           reg_mc_srch_rng_luma_abv_yscale // unsigned ,    RW, default = 0  h2v2 abv y en
//Bit 26           reg_mc_srch_rng_luma_blw_xscale // unsigned ,    RW, default = 0  h2v2 blw x en
//Bit 25           reg_mc_srch_rng_luma_blw_yscale // unsigned ,    RW, default = 0  h2v2 blw y en
//Bit 24           reg_mc_srch_rng_chrm_scale_en // unsigned ,    RW, default = 1  h2v2 chrm en
//Bit 23           reg_mc_srch_rng_chrm_scale_force_en  RW, default = 0  h2v2 chrm force setting
//Bit 22           reg_mc_srch_rng_chrm_abv_xscale // unsigned ,    RW, default = 0  h2v2 abv x en
//Bit 21           reg_mc_srch_rng_chrm_abv_yscale // unsigned ,    RW, default = 0  h2v2 abv y en
//Bit 20           reg_mc_srch_rng_chrm_blw_xscale // unsigned ,    RW, default = 0  h2v2 blw x en
//Bit 19           reg_mc_srch_rng_chrm_blw_yscale // unsigned ,    RW, default = 0  h2v2 blw y en
//Bit 18:13        reg_mc_srch_rng_ofst     RW, default = 8  even and h2v2 ofst setting
//Bit 12: 0        reserved

// Reading file:  ./frc_inc/frc_mc_csc_regs.h
#define FRC_MC_CSC_CTRL                            0x30f0
//Bit 31: 10       reserved
//Bit 9 : 8        reg_glk_ctrl      // unsigned ,    RW, default = 0  csc reg_glk_ctrl enable 2'b00:gating 2'b01:close 2'b1x:always open
//Bit 7: 5         reserved
//Bit  4           reg_sync_en       // unsigned ,    RW, default = 0  reg_csc_en sync enable
//Bit  3           reg_csc_en        // unsigned ,    RW, default = 1  enable rgb2yuv mtrix for ip pattern generation
//Bit 2:0          reg_csc_rs        // unsigned ,    RW, default = 0  0: normalized to 1024 as 1; 1: norm to 2048; 2: norm to 4096; 3: norm to 8192
#define FRC_MC_CSC_OFFSET_INP01                    0x30f5
//Bit 31:29        reserved
//Bit 28:16        reg_csc_offst_inp_0 // signed ,    RW, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_csc_offst_inp_1 // signed ,    RW, default = -512
#define FRC_MC_CSC_OFFSET_INP2                     0x30f6
//Bit 31:13        reserved
//Bit 12: 0        reg_csc_offst_inp_2 // signed ,    RW, default = -512
#define FRC_MC_CSC_COEF_00_01                      0x30f7
//Bit 31:29        reserved
//Bit 28:16        reg_csc_coef_0_0  // signed ,    RW, default = 1024
//Bit 15:13        reserved
//Bit 12: 0        reg_csc_coef_0_1  // signed ,    RW, default = 0
#define FRC_MC_CSC_COEF_02_10                      0x30f8
//Bit 31:29        reserved
//Bit 28:16        reg_csc_coef_0_2  // signed ,    RW, default = 1577
//Bit 15:13        reserved
//Bit 12: 0        reg_csc_coef_1_0  // signed ,    RW, default = 1024
#define FRC_MC_CSC_COEF_11_12                      0x30f9
//Bit 31:29        reserved
//Bit 28:16        reg_csc_coef_1_1  // signed ,    RW, default = -187
//Bit 15:13        reserved
//Bit 12: 0        reg_csc_coef_1_2  // signed ,    RW, default = -470
#define FRC_MC_CSC_COEF_20_21                      0x30fa
//Bit 31:29        reserved
//Bit 28:16        reg_csc_coef_2_0  // signed ,    RW, default = 1024
//Bit 15:13        reserved
//Bit 12: 0        reg_csc_coef_2_1  // signed ,    RW, default = 1860
#define FRC_MC_CSC_COEF_22                         0x30fb
//Bit 31:13        reserved
//Bit 12: 0        reg_csc_coef_2_2  // signed ,    RW, default = 0
#define FRC_MC_CSC_OFFSET_OUP01                    0x30fc
//Bit 31:29        reserved
//Bit 28:16        reg_csc_offst_oup_0 // signed ,    RW, default = 0
//Bit 15:13        reserved
//Bit 12: 0        reg_csc_offst_oup_1 // signed ,    RW, default = 0
#define FRC_MC_CSC_OFFSET_OUP2                     0x30fe
//Bit 31:13        reserved
//Bit 12: 0        reg_csc_offst_oup_2 // signed ,    RW, default = 0
// synopsys translate_off
// synopsys translate_on
//
// Closing file:  ./frc_inc/frc_mc_csc_regs.h
#define FRC_MC_BB_HANDLE_ORG_ME_BB_XYXY_LEFT_TOP   0x3005
#define FRC_MC_BB_HANDLE_ORG_ME_BB_XYXY_RIGHT_BOT  0x3006
#define FRC_MC_BB_HANDLE_ORG_ME_BLK_BB_XYXY_LFT_AND_TOP 0x3007
#define FRC_MC_BB_HANDLE_ORG_ME_BLK_BB_XYXY_RIT_AND_BOT 0x3008
#define FRC_MC_BB_HANDLE_ME_BLK_BB_XYXY_LFT_AND_TOP 0x3009
#define FRC_MC_BB_HANDLE_ME_BLK_BB_XYXY_RIT_AND_BOT 0x300a
#define FRC_MC_DEMO_WINDOW                         0x3200
//Bit 31: 5        reserved
//Bit  4           reg_mc_demo_window_inverse // unsigned ,    RW, default = 0  enable of inverse-demo-window:  0:do memc in demo-window;     1:dont do memc in demo-window
//Bit  3           reg_mc_demo_window1_en    // unsigned ,    RW, default = 0  enable of demo-window:        0:no demo-window;      1:right-half is demo-window
//Bit  2           reg_mc_demo_window2_en    // unsigned ,    RW, default = 0  enable of demo-window:        0:no demo-window;      1:right-half is demo-window
//Bit  1           reg_mc_demo_window3_en    // unsigned ,    RW, default = 0  enable of demo-window:        0:no demo-window;      1:right-half is demo-window
//Bit  0           reg_mc_demo_window4_en    // unsigned ,    RW, default = 0  enable of demo-window:        0:no demo-window;      1:right-half is demo-window

#define FRC_MC_SEVEN_FLAG_NUM13_NUM14_NUM15_NUM16  0x322a
 //Bit 31	    reg_mc_7_flag1_debug_en3  // unsigned ,    RW, default = 0	flag en
 //Bit 30:28	    reg_mc_7_flag1_color3_mode // unsigned ,	RW, default = 0  flag color mode
 //Bit 27:24	    reg_mc_7_flag1_num3       // unsigned ,    RW, default = 0	flag num
 //Bit 23	    reg_mc_7_flag1_debug_en4  // unsigned ,    RW, default = 0	flag en
 //Bit 22:20	    reg_mc_7_flag1_color4_mode // unsigned ,	RW, default = 0  flag color mode
 //Bit 19:16	    reg_mc_7_flag1_num4       // unsigned ,    RW, default = 0	flag num
 //Bit 15	    reg_mc_7_flag1_debug_en5  // unsigned ,    RW, default = 0	flag en
 //Bit 14:12	    reg_mc_7_flag1_color5_mode // unsigned ,	RW, default = 0  flag color mode
 //Bit 11: 8	    reg_mc_7_flag1_num5       // unsigned ,    RW, default = 0	flag num
 //Bit	7	    reg_mc_7_flag1_debug_en6  // unsigned ,    RW, default = 0	flag en
 //Bit	6: 4	    reg_mc_7_flag1_color6_mode // unsigned ,	RW, default = 0  flag color mode
 //Bit	3: 0	    reg_mc_7_flag1_num6       // unsigned ,    RW, default = 0	flag num
#define FRC_MC_SEVEN_FLAG_NUM17_NUM18_NUM21_NUM22  0x322b
 //Bit 31	    reg_mc_7_flag1_debug_en7  // unsigned ,    RW, default = 0	flag en
 //Bit 30:28	    reg_mc_7_flag1_color7_mode // unsigned ,	RW, default = 0  flag color mode
 //Bit 27:24	    reg_mc_7_flag1_num7       // unsigned ,    RW, default = 0	flag num
 //Bit 23	    reg_mc_7_flag1_debug_en8  // unsigned ,    RW, default = 0	flag en
 //Bit 22:20	    reg_mc_7_flag1_color8_mode // unsigned ,	RW, default = 0  flag color mode
 //Bit 19:16	    reg_mc_7_flag1_num8       // unsigned ,    RW, default = 0	flag num
 //Bit 15	    reg_mc_7_flag2_debug_en1  // unsigned ,    RW, default = 0	flag en
 //Bit 14:12	    reg_mc_7_flag2_color1_mode // unsigned ,	RW, default = 0  flag color mode
 //Bit 11: 8	    reg_mc_7_flag2_num1       // unsigned ,    RW, default = 0	flag num
 //Bit	7	    reg_mc_7_flag2_debug_en2  // unsigned ,    RW, default = 0	flag en
 //Bit	6: 4	    reg_mc_7_flag2_color2_mode // unsigned ,	RW, default = 0  flag color mode
 //Bit	3: 0	    reg_mc_7_flag2_num2       // unsigned ,    RW, default = 0	flag num
#define FRC_MC_SEVEN_FLAG_NUM23_NUM24_NUM25_NUM26  0x322c
 //Bit 31	    reg_mc_7_flag2_debug_en3  // unsigned ,    RW, default = 0	flag en
 //Bit 30:28	    reg_mc_7_flag2_color3_mode // unsigned ,	RW, default = 0  flag color mode
 //Bit 27:24	    reg_mc_7_flag2_num3       // unsigned ,    RW, default = 0	flag num
 //Bit 23	    reg_mc_7_flag2_debug_en4  // unsigned ,    RW, default = 0	flag en
 //Bit 22:20	    reg_mc_7_flag2_color4_mode // unsigned ,	RW, default = 0  flag color mode
 //Bit 19:16	    reg_mc_7_flag2_num4       // unsigned ,    RW, default = 0	flag num
 //Bit 15	    reg_mc_7_flag2_debug_en5  // unsigned ,    RW, default = 0	flag en
 //Bit 14:12	    reg_mc_7_flag2_color5_mode // unsigned ,	RW, default = 0  flag color mode
 //Bit 11: 8	    reg_mc_7_flag2_num5       // unsigned ,    RW, default = 0	flag num
 //Bit	7	    reg_mc_7_flag2_debug_en6  // unsigned ,    RW, default = 0	flag en
 //Bit	6: 4	    reg_mc_7_flag2_color6_mode // unsigned ,	RW, default = 0  flag color mode
 //Bit	3: 0	    reg_mc_7_flag2_num6       // unsigned ,    RW, default = 0	flag num
#define FRC_MC_SEVEN_FLAG_NUM27_NUM28              0x322d
 //Bit 31	    reg_mc_7_flag2_debug_en7  // unsigned ,    RW, default = 0	flag en
 //Bit 30:28	    reg_mc_7_flag2_color7_mode // unsigned ,	RW, default = 0  flag color mode
 //Bit 27:24	    reg_mc_7_flag2_num7       // unsigned ,    RW, default = 0	flag num
 //Bit 23	    reg_mc_7_flag2_debug_en8  // unsigned ,    RW, default = 0	flag en
 //Bit 22:20	    reg_mc_7_flag2_color8_mode // unsigned ,	RW, default = 0  flag color mode
 //Bit 19:16	    reg_mc_7_flag2_num8       // unsigned ,    RW, default = 0	flag num
 //Bit 15: 0	    reserved
#define FRC_MC_SEVEN_FLAG_POSI_AND_NUM31_NUM32     0x322e
 //Bit 31:28	    reg_mc_7_flag_x_posi3     // unsigned ,    RW, default = 0	the first 1x8 seven seg flag posi x direction setting
 //Bit 27:24	    reg_mc_7_flag_y_posi3     // unsigned ,    RW, default = 2	the first 1x8 seven seg flag posi y direction setting
 //Bit 23:16	    reserved
 //Bit 15	    reg_mc_7_flag3_debug_en1  // unsigned ,    RW, default = 0	flag en
 //Bit 14:12	    reg_mc_7_flag3_color1_mode // unsigned ,	RW, default = 0  flag color mode
 //Bit 11: 8	    reg_mc_7_flag3_num1       // unsigned ,    RW, default = 0	flag num
 //Bit	7	    reg_mc_7_flag3_debug_en2  // unsigned ,    RW, default = 0	flag en
 //Bit	6: 4	    reg_mc_7_flag3_color2_mode // unsigned ,	RW, default = 0  flag color mode
 //Bit	3: 0	    reg_mc_7_flag3_num2       // unsigned ,    RW, default = 0	flag num
#define FRC_MC_SEVEN_FLAG_NUM33_NUM34_NUM35_NUM36  0x322f
 //Bit 31	    reg_mc_7_flag3_debug_en3  // unsigned ,    RW, default = 0	flag en
 //Bit 30:28	    reg_mc_7_flag3_color3_mode // unsigned ,	RW, default = 0  flag color mode
 //Bit 27:24	    reg_mc_7_flag3_num3       // unsigned ,    RW, default = 0	flag num
 //Bit 23	    reg_mc_7_flag3_debug_en4  // unsigned ,    RW, default = 0	flag en
 //Bit 22:20	    reg_mc_7_flag3_color4_mode // unsigned ,	RW, default = 0  flag color mode
 //Bit 19:16	    reg_mc_7_flag3_num4       // unsigned ,    RW, default = 0	flag num
 //Bit 15	    reg_mc_7_flag3_debug_en5  // unsigned ,    RW, default = 0	flag en
 //Bit 14:12	    reg_mc_7_flag3_color5_mode // unsigned ,	RW, default = 0  flag color mode
 //Bit 11: 8	    reg_mc_7_flag3_num5       // unsigned ,    RW, default = 0	flag num
 //Bit	7	    reg_mc_7_flag3_debug_en6  // unsigned ,    RW, default = 0	flag en
 //Bit	6: 4	    reg_mc_7_flag3_color6_mode // unsigned ,	RW, default = 0  flag color mode
 //Bit	3: 0	    reg_mc_7_flag3_num6       // unsigned ,    RW, default = 0	flag num
#define FRC_MC_SEVEN_FLAG_NUM37_NUM38              0x3230
 //Bit 31	    reg_mc_7_flag3_debug_en7  // unsigned ,    RW, default = 0	flag en
 //Bit 30:28	    reg_mc_7_flag3_color7_mode // unsigned ,	RW, default = 0  flag color mode
 //Bit 27:24	    reg_mc_7_flag3_num7       // unsigned ,    RW, default = 0	flag num
 //Bit 23	    reg_mc_7_flag3_debug_en8  // unsigned ,    RW, default = 0	flag en
 //Bit 22:20	    reg_mc_7_flag3_color8_mode // unsigned ,	RW, default = 0  flag color mode
 //Bit 19:16	    reg_mc_7_flag3_num8       // unsigned ,    RW, default = 0	flag num
 //Bit 15: 0	    reserved
#define FRC_MC_SEVEN_FLAG_POSI_AND_NUM41_NUM42     0x3231
 //Bit 31:28	    reg_mc_7_flag_x_posi4     // unsigned ,    RW, default = 0	the first 1x8 seven seg flag posi x direction setting
 //Bit 27:24	    reg_mc_7_flag_y_posi4     // unsigned ,    RW, default = 3	the first 1x8 seven seg flag posi y direction setting
 //Bit 23:16	    reserved
 //Bit 15	    reg_mc_7_flag4_debug_en1  // unsigned ,    RW, default = 0	flag en
 //Bit 14:12	    reg_mc_7_flag4_color1_mode // unsigned ,	RW, default = 0  flag color mode
 //Bit 11: 8	    reg_mc_7_flag4_num1       // unsigned ,    RW, default = 0	flag num
 //Bit	7	    reg_mc_7_flag4_debug_en2  // unsigned ,    RW, default = 0	flag en
 //Bit	6: 4	    reg_mc_7_flag4_color2_mode // unsigned ,	RW, default = 0  flag color mode
 //Bit	3: 0	    reg_mc_7_flag4_num2       // unsigned ,    RW, default = 0	flag num
#define FRC_MC_SEVEN_FLAG_NUM43_NUM44_NUM45_NUM46  0x3232
 //Bit 31	    reg_mc_7_flag4_debug_en3  // unsigned ,    RW, default = 0	flag en
 //Bit 30:28	    reg_mc_7_flag4_color3_mode // unsigned ,	RW, default = 0  flag color mode
 //Bit 27:24	    reg_mc_7_flag4_num3       // unsigned ,    RW, default = 0	flag num
 //Bit 23	    reg_mc_7_flag4_debug_en4  // unsigned ,    RW, default = 0	flag en
 //Bit 22:20	    reg_mc_7_flag4_color4_mode // unsigned ,	RW, default = 0  flag color mode
 //Bit 19:16	    reg_mc_7_flag4_num4       // unsigned ,    RW, default = 0	flag num
 //Bit 15	    reg_mc_7_flag4_debug_en5  // unsigned ,    RW, default = 0	flag en
 //Bit 14:12	    reg_mc_7_flag4_color5_mode // unsigned ,	RW, default = 0  flag color mode
 //Bit 11: 8	    reg_mc_7_flag4_num5       // unsigned ,    RW, default = 0	flag num
 //Bit	7	    reg_mc_7_flag4_debug_en6  // unsigned ,    RW, default = 0	flag en
 //Bit	6: 4	    reg_mc_7_flag4_color6_mode // unsigned ,	RW, default = 0  flag color mode
 //Bit	3: 0	    reg_mc_7_flag4_num6       // unsigned ,    RW, default = 0	flag num
#define FRC_MC_SEVEN_FLAG_NUM47_NUM48              0x3233
 //Bit 31	    reg_mc_7_flag4_debug_en7  // unsigned ,    RW, default = 0	flag en
 //Bit 30:28	    reg_mc_7_flag4_color7_mode // unsigned ,	RW, default = 0  flag color mode
 //Bit 27:24	    reg_mc_7_flag4_num7       // unsigned ,    RW, default = 0	flag num
 //Bit 23	    reg_mc_7_flag4_debug_en8  // unsigned ,    RW, default = 0	flag en
 //Bit 22:20	    reg_mc_7_flag4_color8_mode // unsigned ,	RW, default = 0  flag color mode
 //Bit 19:16	    reg_mc_7_flag4_num8       // unsigned ,    RW, default = 0	flag num
 //Bit 15:14	    reserved
 //Bit 13: 8	    reg_mc_7_flag_seg_len     // unsigned ,    RW, default = 16  7 flag seg length
 //Bit	7: 6	    reserved
 //Bit	5: 0	    reg_mc_7_flag_line_width  // unsigned ,    RW, default = 4	7 flag line width

#define FRC_MC_MVRD_CTRL                           0x3901
//Bit 31:9       reserved
//Bit 8          reg_mc_mv_in_sel    // unsigned,RW,default = 1,1:mv from ddr 0:mv from mevp ports
//Bit 7 :1       reserved
//Bit 0          reg_mvrd_mode          // unsigned,RW, default = 0,
					//only active when reg_mc_mv_in_sel == 1,
					//1:always read mv from ddr
					//0:ead mv when mvwr_cnt > mvrd_cnt

#define FRC_MC_SW_RESETS                           0x3904
//Bit 31:16      reserved
//Bit 15: 0      reg_mc_sw_resets                 // unsigned ,    RW, default = 0,

#define FRC_MC_LBUF_LOGO_CTRL                      0x3950
 //Bit 31:9	   reserved
 //Bit	8	   reg_mc_force_melg_en      // unsigned, RW, default=0
 //Bit	7	   reg_mc_force_melg	     // unsigned ,    RW, default = 0
 //Bit	6	   reg_mc_force_iplogo	     // unsigned ,    RW, default = 0
 //Bit	5	   reg_mc_force_iplogo_en      // unsigned ,	RW, default = 0
 //Bit	4	   reg_mc_mv_dbg_mode	     // unsigned ,    RW, default = 0  used for mv dbg mode, force mv_din_srdy as 1
 //Bit	3:0	   reg_mc_melg_dbg_mode      // unsigned ,    RW, default = 0  used for mixlogo dbg mode
#define FRC_MC_HW_CTRL0                            0x398c
//Bit 31:22         reserved
//Bit 21            reg_mc_undone_clr         // unsigned ,    RW, default = 0
//Bit 20            reg_bypass_mc_core_en     // unsigned ,    RW, default = 0
//Bit 19:5          reserved
//Bit 4             reg_olap_mode             // unsigned ,    RW, default = 1
//Bit 3             reg_mc_lp_mode            // unsigned ,    RW, default = 0
//Bit 2             reg_mcp_byp_en            // unsigned ,    RW, default = 0
//Bit 1             reg_mc_byp_ctrl           // unsigned ,    RW, default = 0
//Bit 0             reg_mc_bypass_en          // unsigned ,    RW, default = 0
#define FRC_RO_MC_STAT                             0x3994
//Bit 31:29         reserved
//Bit 28:16         ro_undone_vcnt               // unsigned ,   RO, default = 0  ro_undone_vcnt
//Bit 15:13         reserved
//Bit 12            ro_undone_flag               // unsigned ,   RO, default = 0  ro_undone_flag
//Bit 11: 8         ro_logo_corr_info            // unsigned ,   RO, default = 0  ro_logo_corr_info
//Bit  7: 4         ro_chrm_corr_info            // unsigned ,   RO, default = 0  ro_chrm_corr_info
//Bit  3: 0         ro_luma_corr_info            // unsigned ,   RO, default = 0  ro_luma_corr_info
#define FRC_MC_DBG_MC_WRAP                         0x39ee
 //Bit 31	   ro_melogo_din_srdy		   // unsigned ,    RO, default = 0
 //Bit 30	   ro_melogo_din_rrdy		   // unsigned ,    RO, default = 0
 //Bit 29	   ro_mv_din_srdy		   // unsigned ,    RO, default = 0
 //Bit 28	   ro_mv_din_rrdy		   // unsigned ,    RO, default = 0
 //Bit 27	   ro_syn_fst_phs		   // unsigned ,    RO, default = 0
 //Bit 26	   ro_pre_fst_phs		   // unsigned ,    RO, default = 0
 //Bit 25	   ro_mc_proc_phs		   // unsigned ,    RO, default = 0
 //Bit 24	   ro_mc_undone_flag		   // unsigned ,    RO, default = 0
 //Bit 23:21	   reserved
 //Bit 20: 8	   ro_mc_undone_vcnt		   // unsigned ,    RO, default = 0
 //Bit	7: 3	   reserved
 //Bit	1: 0	   ro_vp2mc_syn_st		   // unsigned ,    RO, default = 0
#define FRC_MCDW_PATH_CTRL                         0x39fd
//Bit 31:5          reserved
//Bit 4             reg_frc_mcdw_path_en          //unsigned, RW, default=0
//Bit 3:2           reserved
//Bit 1             reg_frc_mcdw_v2_en            //unsigned, RW, default=0
//Bit 0             reg_frc_mcdw_h2_en            //unsigned, RW, default=0
// -----------------------------------------------
// REG_BASE:  FRC_RDMA_APB_BASE = 0x3b
// -----------------------------------------------
//
// Reading file:  ./frc_inc/frc_rdma_regs.h
//
//===========================================================================
// FRC_RDMA registers 0x00 - 0xff
//===========================================================================
// Bit 31: 0 RW AHB start address for manual start DMA
#define FRC_RDMA_AHB_START_ADDR_MAN                0x3b00
// Bit 31: 0 RW AHB end address for manual start DMA
#define FRC_RDMA_AHB_END_ADDR_MAN                  0x3b01
// Bit 31: 0 RW AHB start address for auto start source 1
#define FRC_RDMA_AHB_START_ADDR_1                  0x3b02
// Bit 31: 0 RW AHB end address for auto start source 1
#define FRC_RDMA_AHB_END_ADDR_1                    0x3b03
// Bit 31: 0 RW AHB start address for auto start source 2
#define FRC_RDMA_AHB_START_ADDR_2                  0x3b04
// Bit 31: 0 RW AHB end address for auto start source 2
#define FRC_RDMA_AHB_END_ADDR_2                    0x3b05
// Bit 31: 0 RW AHB start address for auto start source 3
#define FRC_RDMA_AHB_START_ADDR_3                  0x3b06
// Bit 31: 0 RW AHB end address for auto start source 3
#define FRC_RDMA_AHB_END_ADDR_3                    0x3b07
// Bit 31: 0 RW AHB start address for auto start source 4
#define FRC_RDMA_AHB_START_ADDR_4                  0x3b08
// Bit 31: 0 RW AHB end address for auto start source 4
#define FRC_RDMA_AHB_END_ADDR_4                    0x3b09
// Bit 31: 0 RW AHB start address for auto start source 5
#define FRC_RDMA_AHB_START_ADDR_5                  0x3b0a
// Bit 31: 0 RW AHB end address for auto start source 5
#define FRC_RDMA_AHB_END_ADDR_5                    0x3b0b
// Bit 31: 0 RW AHB start address for auto start source 6
#define FRC_RDMA_AHB_START_ADDR_6                  0x3b0c
// Bit 31: 0 RW AHB end address for auto start source 6
#define FRC_RDMA_AHB_END_ADDR_6                    0x3b0d
// Bit 31: 0 RW AHB start address for auto start source 7
#define FRC_RDMA_AHB_START_ADDR_7                  0x3b0e
// Bit 31: 0 RW AHB end address for auto start source 7
#define FRC_RDMA_AHB_END_ADDR_7                    0x3b0f
// Auto start DMA control:
// Bit 31:24 RW ctrl_enable_int_3. Interrupt inputs enable mask for source 3.
// Bit 23:16 RW ctrl_enable_int_2. Interrupt inputs enable mask for source 2.
// Bit 15: 8 RW ctrl_enable_int_1. Interrupt inputs enable mask for source 1.
// Bit     7 RW ctrl_cbus_write_3. Register read/write mode for auto-start 3. 1=Register write; 0=Register read.
// Bit     6 RW ctrl_cbus_write_3. Register read/write mode for auto-start 2. 1=Register write; 0=Register read.
// Bit     5 RW ctrl_cbus_write_3. Register read/write mode for auto-start 1. 1=Register write; 0=Register read.
// Bit     4 R  Rsrv.
// Bit     3 RW ctrl_cbus_addr_incr_3. 1=Incremental register access for auto-start 3; 0=Non-incremental (individual) register access.
// Bit     2 RW ctrl_cbus_addr_incr_2. 1=Incremental register access for auto-start 2; 0=Non-incremental (individual) register access.
// Bit     1 RW ctrl_cbus_addr_incr_1. 1=Incremental register access for auto-start 1; 0=Non-incremental (individual) register access.
// Bit     0 R  Rsrv.
#define FRC_RDMA_ACCESS_AUTO                       0x3b10
#define FRC_RDMA_ACCESS_AUTO2                      0x3b11
// Manual start DMA control:
// Bit 31: 3 R  Rsrv.
// Bit     2 RW ctrl_cbus_write_man. Register read/write mode for manual-start. 1=Register write; 0=Register read.
// Bit     1 RW ctrl_cbus_addr_incr_man. 1=Incremental register access for manual-start; 0=Non-incremental (individual) register access.
// Bit     0 W  ctrl_start_man. Write 1 to this bit to manual-start DMA. This bit always read back 0.
#define FRC_RDMA_ACCESS_MAN                        0x3b13
// RDMA general control:
// Bit 31:25 R  Rsrv.
// Bit    24 W  ctrl_clr_FRC_RDMA_done_int. Write 1 to reset FRC_RDMA_int level to 0. No need to clear this bit.
// Bit 23:19 R  Rsrv.
// Bit 18:13 R  Rsrv.
// Bit 12: 7 R  Rsrv.
// Bit     6 RW ctrl_ddr_urgent.
// Bit  5: 4 RW ctrl_ahb_wr_burst_size. 0=ABH write request burst size 16;
//                                      1=ABH write request burst size 24;
//                                      2=ABH write request burst size 32;
//                                      3=ABH write request burst size 48.
// Bit  3: 2 RW ctrl_ahb_rd_burst_size. 0=ABH read request burst size 16;
//                                      1=ABH read request burst size 24;
//                                      2=ABH read request burst size 32;
//                                      3=ABH read request burst size 48.
// Bit     1 RW ctrl_sw_reset. 1=Reset RDMA logic except register.
// Bit     0 RW ctrl_free_clk_enable. 0=Default, Enable clock gating. 1=No clock gating, enable free clock.
#define FRC_RDMA_CTRL                              0x3b14
// Read only.
// Bit 31:29 R  Rsrv.
// Bit    28 R  FRC_RDMA_done_int.
// Bit 27:25 R  Rsrv.
// Bit 24:18 R  ahb_wrfifo_cnt. FIFO for buffering CBus read data to be sent to AHB
// Bit 17:11 R  ahb_rdfifo_cnt. FIFO for buffering data read from AHB.
// Bit 10: 8 R  ddr_req_st. =0 -- Idle; !=0 -- AHB interfacing ongoing.
// Bit  7: 4 R  curr_req. Latest requests that is being/been serviced. E.g. 0000=Idle; 0010=Latest serviced request is Req 1.
// Bit  3: 0 R  req_latch. Requests that are yet to be serviced. E.g. 0000=No request; 0001=Req 0 waiting; 1100=Req 2 and 3 waiting.
#define FRC_RDMA_STATUS                            0x3b15
#define FRC_RDMA_STATUS2                           0x3b16
#define FRC_RDMA_STATUS3                           0x3b17
#define FRC_RDMA_ACCESS_AUTO4                      0x3b18
#define FRC_RDMA_SRAM_CNTL                         0x3b20
#define FRC_RDMA_SRAM_REGADDR                      0x3b21
#define FRC_RDMA_SRAM_REGDATA                      0x3b22
#define FRC_RDMA_AUTO_SRC1_SEL                     0x3b23
#define FRC_RDMA_AUTO_SRC2_SEL                     0x3b24
#define FRC_RDMA_AUTO_SRC3_SEL                     0x3b25
#define FRC_RDMA_AUTO_SRC4_SEL                     0x3b26
#define FRC_RDMA_AUTO_SRC5_SEL                     0x3b27
#define FRC_RDMA_AUTO_SRC6_SEL                     0x3b28
#define FRC_RDMA_AUTO_SRC7_SEL                     0x3b29
// Bit  1: 0 RW AHB start address[33:32] for manual start DMA
#define FRC_RDMA_AHB_START_ADDR_MAN_MSB            0x3b30
// Bit  1: 0 RW AHB end address[33:32] for manual start DMA
#define FRC_RDMA_AHB_END_ADDR_MAN_MSB              0x3b31
// Bit  1: 0 RW AHB start address[33:32] for auto start source 1
#define FRC_RDMA_AHB_START_ADDR_1_MSB              0x3b32
// Bit  1: 0 RW AHB end address[33:32] for auto start source 1
#define FRC_RDMA_AHB_END_ADDR_1_MSB                0x3b33
// Bit  1: 0 RW AHB start address[33:32] for auto start source 2
#define FRC_RDMA_AHB_START_ADDR_2_MSB              0x3b34
// Bit  1: 0 RW AHB end address[33:32] for auto start source 2
#define FRC_RDMA_AHB_END_ADDR_2_MSB                0x3b35
// Bit  1: 0 RW AHB start address[33:32] for auto start source 3
#define FRC_RDMA_AHB_START_ADDR_3_MSB              0x3b36
// Bit  1: 0 RW AHB end address[33:32] for auto start source 3
#define FRC_RDMA_AHB_END_ADDR_3_MSB                0x3b37
// Bit  1: 0 RW AHB start address[33:32] for auto start source 4
#define FRC_RDMA_AHB_START_ADDR_4_MSB              0x3b38
// Bit  1: 0 RW AHB end address[33:32] for auto start source 4
#define FRC_RDMA_AHB_END_ADDR_4_MSB                0x3b39
// Bit  1: 0 RW AHB start address[33:32] for auto start source 5
#define FRC_RDMA_AHB_START_ADDR_5_MSB              0x3b3a
// Bit  1: 0 RW AHB end address[33:32] for auto start source 5
#define FRC_RDMA_AHB_END_ADDR_5_MSB                0x3b3b
// Bit  1: 0 RW AHB start address[33:32] for auto start source 6
#define FRC_RDMA_AHB_START_ADDR_6_MSB              0x3b3c
// Bit  1: 0 RW AHB end address[33:32] for auto start source 6
#define FRC_RDMA_AHB_END_ADDR_6_MSB                0x3b3d
// Bit  1: 0 RW AHB start address[33:32] for auto start source 7
#define FRC_RDMA_AHB_START_ADDR_7_MSB              0x3b3e
// Bit  1: 0 RW AHB end address[33:32] for auto start source 7
#define FRC_RDMA_AHB_END_ADDR_7_MSB                0x3b3f
//
// Closing file:  ./frc_inc/frc_rdma_regs.h
//
//
// Reading file:  ./frc_inc/frc_rdma_regs.h  for T3X
//
//===========================================================================
// FRC_RDMA registers 0x00 - 0xff
//===========================================================================
//==========================================ctrl regs==========================================
#define FRC_RDMA_CTRL_T3X                              0x3b00
//Bit 31:16     ctrl_clr_rdma_done_int                  //unsigned, WO, default = 0, clear int. Write 1 to reset rdma_int level to 0. No need to clear this bit.
//Bit 15        ctrl_clr_err                            //unsigned, WO, default = 0, clear rdma err
//Bit 11:10     ctrl_ram_wr_burst_size                  //unsigned, RW, default = 0
//Bit 9:8       ctrl_ram_rd_burst_size                  //unsigned, RW, default = 0
//Bit 7         ctrl_ddr_wr_urgent                      //unsigned, RW, default = 0
//Bit 6         ctrl_ddr_rd_urgent                      //unsigned, RW, default = 0,
//Bit 5:4       ctrl_ahb_wr_burst_size                  //unsigned, RW, default = 0, ABH write request burst size, 0:16 1:24: 2:32 3:48
//Bit 3:2       ctrl_ahb_rd_burst_size                  //unsigned, RW, default = 0, ABH read request burst size, 0:16 1:24: 2:32 3:48
//Bit 1         ctrl_sw_reset                           //unsigned, RW, default = 0, 1=Reset RDMA logic except register
//Bit 0         ctrl_free_clk_enable                    //unsigned, RW, default = 0, Enable clock gating. 1=No clock gating, enable free clock
#define FRC_RDMA_SRAM_CNTL_T3X                         0x3b01
//Bit 31:18     reserved
//Bit 17        ram_incr_mode                           //unsigned, RW, default = 0, increase mode, 0=SRAM address increment after RDMA_SRAM_REGDATA; 1=SRAM address increment after RDMA_SRAM_REGADDR
//Bit 16        ram_wr_int                              //unsigned, RW, default = 0, rd/wr mode, 0=Write; 1=Read
//Bit 15:10     reserved
//Bit 9:0       ram_addr_int                            //unsigned, RW, default = 0, init sram addr
#define FRC_RDMA_SRAM_REGADDR_T3X                      0x3b02
//Bit 31:16     reserved
//Bit 15:0      ram_regaddr                             //unsigned, RW, default = 0, reg addr which write to sram or read from sram
#define FRC_RDMA_SRAM_REGDATA_T3X                       0x3b03
//Bit 31:0      ram_regdata                             //unsigned, RW, default = 0, reg data which write to sram or read from sram
#define FRC_RDMA_STATUS_T3X                             0x3b04
//Bit 31:30     reserved
//Bit 29:28     axi_w_st                                //unsigned, RO, default = 0
//Bit 27:26     axi_aw_st                               //unsigned, RO, default = 0
//Bit 25:24     axi_ar_st                               //unsigned, RO, default = 0
//Bit 23:20     rdma_id_axi                             //unsigned, RO, default = 0, axi rdma channel id
//Bit 19:16     rdma_id_pipe                            //unsigned, RO, default = 0, sram rdma channel id
//Bit 15:0      req_latch                               //unsigned, RO, default = 0, req_latch. Requests that are yet to be serviced. E.g. 0000=No request; 0001=Req 0 waiting; 1100=Req 2 and 3 waiting.
#define FRC_RDMA_STATUS1_T3X                            0x3b05
//Bit 31:20     reserved
//Bit 19:4      rdma_done_flag                          //unsigned, RO, default = 0
//Bit 3         reserved
//Bit 2         err_axi_wrfifo_underflow                //unsigned, RO, default = 0
//Bit 1         err_axi_wrfifo_overflow                  //unsigned, RO, default = 0
//Bit 0         err_axi_rdfifo_overflow                  //unsigned, RO, default = 0
#define FRC_RDMA_STATUS2_T3X                            0x3b06
//Bit 31:30     reserved
//Bit 29:24     shr_wrfifo_cnt                          //unsigned, RO, default = 0
//Bit 23:22     reserved
//Bit 21:16     shr_rdfifo_cnt                          //unsigned, RO, default = 0
//Bit 15:14     reserved
//Bit 13:8      axi_wrfifo_room                         //unsigned, RO, default = 0
//Bit 7:6       reserved
//Bit 5:0       axi_rdfifo_room                         //unsigned, RO, default = 0
#define FRC_RDMA_STATUS3_T3X                            0x3b07
//Bit 31:30     reserved
//Bit 29:28     ram_req_st                              //unsigned, RO, default = 0
//Bit 27:24     rdma_id_ram                             //unsigned, RO, default = 0
//Bit 23:8      axi_b_pending                           //unsigned, RO, default = 0
//Bit 7:0       axi_aw_pending                          //unsigned, RO, default = 0
//==========================================man regs==========================================
#define FRC_RDMA_ACCESS_MAN_T3X                         0x3b10
//Bit 31:4      reserved
//Bit 3         ctrl_buf_sel_man                        //unsigned, RW, default = 0, 0:select ddr 1:select sram
//Bit 2         ctrl_cbus_write_man                     //unsigned, RW, default = 0, Register read/write mode for manual-start. 1=Register write; 0=Register read
//Bit 1         ctrl_cbus_addr_incr_man                 //unsigned, RW, default = 0, 1=Incremental register access for manual-start; 0=Non-incremental (individual) register access
//Bit 0         ctrl_start_man                          //unsigned, WO, default = 0, Write 1 to this bit to manual-start DMA. This bit always read back 0
#define FRC_RDMA_AHB_START_ADDR_MAN_MSB_T3X             0x3b11
//Bit 31:2      reserved
//Bit  1:0      AHB start address_high                  //unsigned, RW, default = 0, AHB start address[33:32] for manual start DMA
#define FRC_RDMA_AHB_START_ADDR_MAN_T3X                 0x3b12
//Bit 31:0      AHB start address_low                   //unsigned, RW, default = 0, AHB start address[31:0] for manual start DMA
#define FRC_RDMA_AHB_END_ADDR_MAN_MSB_T3X               0x3b13
//Bit 31:2      reserved
//Bit  1:0      AHB end address_high                    //unsigned, RW, default = 0, AHB end address[33:32] for manual start DMA
#define FRC_RDMA_AHB_END_ADDR_MAN_T3X                   0x3b14
//Bit 31:0      AHB end address_low                     //unsigned, RW, default = 0, AHB end address[31:0] for manual start DMA
//==========================================auto regs==========================================
#define FRC_RDMA_ACCESS_AUTO_T3X                        0x3b20
//Bit 24:8      reserved
//Bit    7      ctrl_cbus_write_3                       //unsigned, RW, default = 0, Register read/write mode for auto-start 3. 1=Register write; 0=Register read
//Bit    6      ctrl_cbus_write_2                       //unsigned, RW, default = 0, Register read/write mode for auto-start 2. 1=Register write; 0=Register read
//Bit    5      ctrl_cbus_write_1                       //unsigned, RW, default = 0, Register read/write mode for auto-start 1. 1=Register write; 0=Register read
//Bit    4      reserved
//Bit    3      ctrl_cbus_addr_incr_3                   //unsigned, RW, default = 0, 1=Incremental register access for auto-start 3; 0=Non-incremental (individual) register access
//Bit    2      ctrl_cbus_addr_incr_2                   //unsigned, RW, default = 0, 1=Incremental register access for auto-start 2; 0=Non-incremental (individual) register access
//Bit    1      ctrl_cbus_addr_incr_1                   //unsigned, RW, default = 0, 1=Incremental register access for auto-start 1; 0=Non-incremental (individual) register access
//Bit    0      reserved
#define FRC_RDMA_ACCESS_AUTO2_T3X                       0x3b21
//Bit 31:8      reserved
//Bit    7      ctrl_cbus_write_7                       //unsigned, RW, default = 0, Register read/write mode for auto-start 7. 1=Register write; 0=Register read
//Bit    6      ctrl_cbus_write_6                       //unsigned, RW, default = 0, Register read/write mode for auto-start 6. 1=Register write; 0=Register read
//Bit    5      ctrl_cbus_write_5                       //unsigned, RW, default = 0, Register read/write mode for auto-start 5. 1=Register write; 0=Register read
//Bit    4      ctrl_cbus_write_4                       //unsigned, RW, default = 0, Register read/write mode for auto-start 4. 1=Register write; 0=Register read
//Bit    3      ctrl_cbus_addr_incr_7                   //unsigned, RW, default = 0, 1=Incremental register access for auto-start 7; 0=Non-incremental (individual) register access
//Bit    2      ctrl_cbus_addr_incr_6                   //unsigned, RW, default = 0, 1=Incremental register access for auto-start 6; 0=Non-incremental (individual) register access
//Bit    1      ctrl_cbus_addr_incr_5                   //unsigned, RW, default = 0, 1=Incremental register access for auto-start 5; 0=Non-incremental (individual) register access
//Bit    0      ctrl_cbus_addr_incr_4                   //unsigned, RW, default = 0, 1=Incremental register access for auto-start 4; 0=Non-incremental (individual) register access
#define FRC_RDMA_ACCESS_AUTO3_T3X                       0x3b22
//Bit   31:16   reserved
//Bit   15      ctrl_cbus_write_15                      //unsigned, RW, default = 0, Register read/write mode for auto-start 15. 1=Register write; 0=Register read
//Bit   14      ctrl_cbus_write_14                      //unsigned, RW, default = 0, Register read/write mode for auto-start 14. 1=Register write; 0=Register read
//Bit   13      ctrl_cbus_write_13                      //unsigned, RW, default = 0, Register read/write mode for auto-start 13. 1=Register write; 0=Register read
//Bit   12      ctrl_cbus_write_12                      //unsigned, RW, default = 0, Register read/write mode for auto-start 12. 1=Register write; 0=Register read
//Bit   11      ctrl_cbus_write_11                      //unsigned, RW, default = 0, Register read/write mode for auto-start 11. 1=Register write; 0=Register read
//Bit   10      ctrl_cbus_write_10                      //unsigned, RW, default = 0, Register read/write mode for auto-start 10. 1=Register write; 0=Register read
//Bit    9      ctrl_cbus_write_9                       //unsigned, RW, default = 0, Register read/write mode for auto-start 9. 1=Register write; 0=Register read
//Bit    8      ctrl_cbus_write_8                       //unsigned, RW, default = 0, Register read/write mode for auto-start 8. 1=Register write; 0=Register read
//Bit    7      ctrl_cbus_addr_incr_15                  //unsigned, RW, default = 0, 1=Incremental register access for auto-start 15; 0=Non-incremental (individual) register access
//Bit    6      ctrl_cbus_addr_incr_14                  //unsigned, RW, default = 0, 1=Incremental register access for auto-start 14; 0=Non-incremental (individual) register access
//Bit    5      ctrl_cbus_addr_incr_13                  //unsigned, RW, default = 0, 1=Incremental register access for auto-start 13; 0=Non-incremental (individual) register access
//Bit    4      ctrl_cbus_addr_incr_12                  //unsigned, RW, default = 0, 1=Incremental register access for auto-start 12; 0=Non-incremental (individual) register access
//Bit    3      ctrl_cbus_addr_incr_11                  //unsigned, RW, default = 0, 1=Incremental register access for auto-start 11; 0=Non-incremental (individual) register access
//Bit    2      ctrl_cbus_addr_incr_10                  //unsigned, RW, default = 0, 1=Incremental register access for auto-start 10; 0=Non-incremental (individual) register access
//Bit    1      ctrl_cbus_addr_incr_9                   //unsigned, RW, default = 0, 1=Incremental register access for auto-start 9; 0=Non-incremental (individual) register access
//Bit    0      ctrl_cbus_addr_incr_8                   //unsigned, RW, default = 0, 1=Incremental register access for auto-start 8; 0=Non-incremental (individual) register access
#define FRC_RDMA_ACCESS_AUTO4_T3X                       0x3b23
//Bit 31:16  reserved
//Bit 15     ctrl_buf_sel_15                     //unsigned, RW, default = 0
//Bit 14     ctrl_buf_sel_14                     //unsigned, RW, default = 0
//Bit 13     ctrl_buf_sel_13                     //unsigned, RW, default = 0
//Bit 12     ctrl_buf_sel_12                     //unsigned, RW, default = 0
//Bit 11     ctrl_buf_sel_11                     //unsigned, RW, default = 0
//Bit 10     ctrl_buf_sel_10                     //unsigned, RW, default = 0
//Bit 9      ctrl_buf_sel_9                      //unsigned, RW, default = 0
//Bit 8      ctrl_buf_sel_8                      //unsigned, RW, default = 0
//Bit 7      ctrl_buf_sel_7                      //unsigned, RW, default = 0
//Bit 6      ctrl_buf_sel_6                      //unsigned, RW, default = 0
//Bit 5      ctrl_buf_sel_5                      //unsigned, RW, default = 0
//Bit 4      ctrl_buf_sel_4                      //unsigned, RW, default = 0
//Bit 3      ctrl_buf_sel_3                      //unsigned, RW, default = 0
//Bit 2      ctrl_buf_sel_2                      //unsigned, RW, default = 0
//Bit 1      ctrl_buf_sel_1                      //unsigned, RW, default = 0
//Bit 0      reserved
#define FRC_RDMA_AUTO_SRC1_SEL_T3X                      0x3b24
//Bit 31:0      ctrl_enable_int_1                       //unsigned, RW, default = 0
#define FRC_RDMA_AHB_START_ADDR_1_MSB_T3X               0x3b25
//Bit 31:2      reserved
//Bit  1:0      AHB start address_high                  //unsigned, RW, default = 0, AHB start address[33:32] for  auto start source 1
#define FRC_RDMA_AHB_START_ADDR_1_T3X                   0x3b26
//Bit 31:0      AHB start address_low                   //unsigned, RW, default = 0, AHB start address[31:0] for auto start source 1
#define FRC_RDMA_AHB_END_ADDR_1_MSB_T3X                 0x3b27
//Bit 31:2      reserved
//Bit  1:0      AHB end address_high                    //unsigned, RW, default = 0, AHB end address[33:32] for  auto start source 1
#define FRC_RDMA_AHB_END_ADDR_1_T3X                     0x3b28
//Bit 31:0      AHB end address_low                     //unsigned, RW, default = 0, AHB end address[31:0] for auto start source 1
#define FRC_RDMA_AUTO_SRC2_SEL_T3X                      0x3b29
//Bit 31:0      ctrl_enable_int_2                       //unsigned, RW, default = 0
#define FRC_RDMA_AHB_START_ADDR_2_MSB_T3X               0x3b2a
//Bit 31:2      reserved
//Bit  1:0      AHB start address_high                  //unsigned, RW, default = 0, AHB start address[33:32] for  auto start source 2
#define FRC_RDMA_AHB_START_ADDR_2_T3X                   0x3b2b
//Bit 31:0      AHB start address_low                   //unsigned, RW, default = 0, AHB start address[31:0] for auto start source 2
#define FRC_RDMA_AHB_END_ADDR_2_MSB_T3X                 0x3b2c
//Bit 31:2      reserved
//Bit  1:0      AHB end address_high                    //unsigned, RW, default = 0, AHB end address[33:32] for  auto start source 2
#define FRC_RDMA_AHB_END_ADDR_2_T3X                     0x3b2d
//Bit 31:0      AHB end address_low                     //unsigned, RW, default = 0, AHB end address[31:0] for auto start source 2
#define FRC_RDMA_AUTO_SRC3_SEL_T3X                      0x3b2e
//Bit 31:0      ctrl_enable_int_3                       //unsigned, RW, default = 0
#define FRC_RDMA_AHB_START_ADDR_3_MSB_T3X               0x3b2f
//Bit 31:2      reserved
//Bit  1:0      AHB start address_high                  //unsigned, RW, default = 0, AHB start address[33:32] for  auto start source 3
#define FRC_RDMA_AHB_START_ADDR_3_T3X                   0x3b30
//Bit 31:0      AHB start address_low                   //unsigned, RW, default = 0, AHB start address[31:0] for auto start source 3
#define FRC_RDMA_AHB_END_ADDR_3_MSB_T3X                 0x3b31
//Bit 31:2      reserved
//Bit  1:0      AHB end address_high                    //unsigned, RW, default = 0, AHB end address[33:32] for  auto start source 3
#define FRC_RDMA_AHB_END_ADDR_3_T3X                     0x3b32
//Bit 31:0      AHB end address_low                     //unsigned, RW, default = 0, AHB end address[31:0] for auto start source 3
#define FRC_RDMA_AUTO_SRC4_SEL_T3X                      0x3b33
//Bit 31:0      ctrl_enable_int_4                       //unsigned, RW, default = 0
#define FRC_RDMA_AHB_START_ADDR_4_MSB_T3X               0x3b34
//Bit 31:2      reserved
//Bit  1:0      AHB start address_high                  //unsigned, RW, default = 0, AHB start address[33:32] for  auto start source 4
#define FRC_RDMA_AHB_START_ADDR_4_T3X                   0x3b35
//Bit 31:0      AHB start address_low                   //unsigned, RW, default = 0, AHB start address[31:0] for auto start source 4
#define FRC_RDMA_AHB_END_ADDR_4_MSB_T3X                 0x3b36
//Bit 31:2      reserved
//Bit  1:0      AHB end address_high                    //unsigned, RW, default = 0, AHB end address[33:32] for  auto start source 4
#define FRC_RDMA_AHB_END_ADDR_4_T3X                     0x3b37
//Bit 31:0      AHB end address_low                     //unsigned, RW, default = 0, AHB end address[31:0] for auto start source 4
#define FRC_RDMA_AUTO_SRC5_SEL_T3X                      0x3b38
//Bit 31:0      ctrl_enable_int_5                       //unsigned, RW, default = 0
#define FRC_RDMA_AHB_START_ADDR_5_MSB_T3X               0x3b39
//Bit 31:2      reserved
//Bit  1:0      AHB start address_high                  //unsigned, RW, default = 0, AHB start address[33:32] for  auto start source 5
#define FRC_RDMA_AHB_START_ADDR_5_T3X                   0x3b3a
//Bit 31:0      AHB start address_low                   //unsigned, RW, default = 0, AHB start address[31:0] for auto start source 5
#define FRC_RDMA_AHB_END_ADDR_5_MSB_T3X                 0x3b3b
//Bit 31:2      reserved
//Bit  1:0      AHB end address_high                    //unsigned, RW, default = 0, AHB end address[33:32] for  auto start source 5
#define FRC_RDMA_AHB_END_ADDR_5_T3X                     0x3b3c
//Bit 31:0      AHB end address_low                     //unsigned, RW, default = 0, AHB end address[31:0] for auto start source 5
#define FRC_RDMA_AUTO_SRC6_SEL_T3X                      0x3b3d
//Bit 31:0      ctrl_enable_int_6                       //unsigned, RW, default = 0
#define FRC_RDMA_AHB_START_ADDR_6_MSB_T3X               0x3b3e
//Bit 31:2      reserved
//Bit  1:0      AHB start address_high                  //unsigned, RW, default = 0, AHB start address[33:32] for  auto start source 6
#define FRC_RDMA_AHB_START_ADDR_6_T3X                   0x3b3f
//Bit 31:0      AHB start address_low                   //unsigned, RW, default = 0, AHB start address[31:0] for auto start source 6
#define FRC_RDMA_AHB_END_ADDR_6_MSB_T3X                 0x3b40
//Bit 31:2      reserved
//Bit  1:0      AHB end address_high                    //unsigned, RW, default = 0, AHB end address[33:32] for  auto start source 6
#define FRC_RDMA_AHB_END_ADDR_6_T3X                     0x3b41
//Bit 31:0      AHB end address_low                     //unsigned, RW, default = 0, AHB end address[31:0] for auto start source 6
#define FRC_RDMA_AUTO_SRC7_SEL_T3X                      0x3b42
//Bit 31:0      ctrl_enable_int_7                       //unsigned, RW, default = 0
#define FRC_RDMA_AHB_START_ADDR_7_MSB_T3X               0x3b43
//Bit 31:2      reserved
//Bit  1:0      AHB start address_high                  //unsigned, RW, default = 0, AHB start address[33:32] for  auto start source 7
#define FRC_RDMA_AHB_START_ADDR_7_T3X                   0x3b44
//Bit 31:0      AHB start address_low                   //unsigned, RW, default = 0, AHB start address[31:0] for auto start source 7
#define FRC_RDMA_AHB_END_ADDR_7_MSB_T3X                 0x3b45
//Bit 31:2      reserved
//Bit  1:0      AHB end address_high                    //unsigned, RW, default = 0, AHB end address[33:32] for  auto start source 7
#define FRC_RDMA_AHB_END_ADDR_7_T3X                     0x3b46
//Bit 31:0      AHB end address_low                     //unsigned, RW, default = 0, AHB end address[31:0] for auto start source 7
#define FRC_RDMA_AUTO_SRC8_SEL_T3X                      0x3b47
//Bit 31:0      ctrl_enable_int_8                       //unsigned, RW, default = 0
#define FRC_RDMA_AHB_START_ADDR_8_MSB_T3X               0x3b48
//Bit 31:2      reserved
//Bit  1:0      AHB start address_high                  //unsigned, RW, default = 0, AHB start address[33:32] for  auto start source 8
#define FRC_RDMA_AHB_START_ADDR_8_T3X                   0x3b49
//Bit 31:0      AHB start address_low                   //unsigned, RW, default = 0, AHB start address[31:0] for auto start source 8
#define FRC_RDMA_AHB_END_ADDR_8_MSB_T3X                 0x3b4a
//Bit 31:2      reserved
//Bit  1:0      AHB end address_high                    //unsigned, RW, default = 0, AHB end address[33:32] for  auto start source 8
#define FRC_RDMA_AHB_END_ADDR_8_T3X                     0x3b4b
//Bit 31:0      AHB end address_low                     //unsigned, RW, default = 0, AHB end address[31:0] for auto start source 8
#define FRC_RDMA_AUTO_SRC9_SEL_T3X                      0x3b4c
//Bit 31:0      ctrl_enable_int_9                       //unsigned, RW, default = 0
#define FRC_RDMA_AHB_START_ADDR_9_MSB_T3X               0x3b4d
//Bit 31:2      reserved
//Bit  1:0      AHB start address_high                  //unsigned, RW, default = 0, AHB start address[33:32] for  auto start source 9
#define FRC_RDMA_AHB_START_ADDR_9_T3X                   0x3b4e
//Bit 31:0      AHB start address_low                   //unsigned, RW, default = 0, AHB start address[31:0] for auto start source 9
#define FRC_RDMA_AHB_END_ADDR_9_MSB_T3X                 0x3b4f
//Bit 31:2      reserved
//Bit  1:0      AHB end address_high                    //unsigned, RW, default = 0, AHB end address[33:32] for  auto start source 9
#define FRC_RDMA_AHB_END_ADDR_9_T3X                     0x3b50
//Bit 31:0      AHB end address_low                     //unsigned, RW, default = 0, AHB end address[31:0] for auto start source 9
#define FRC_RDMA_AUTO_SRC10_SEL_T3X                     0x3b51
//Bit 31:0      ctrl_enable_int_10                      //unsigned, RW, default = 0
#define FRC_RDMA_AHB_START_ADDR_10_MSB_T3X              0x3b52
//Bit 31:2      reserved
//Bit  1:0      AHB start address_high                  //unsigned, RW, default = 0, AHB start address[33:32] for  auto start source 10
#define FRC_RDMA_AHB_START_ADDR_10_T3X                  0x3b53
//Bit 31:0      AHB start address_low                   //unsigned, RW, default = 0, AHB start address[31:0] for auto start source 10
#define FRC_RDMA_AHB_END_ADDR_10_MSB_T3X                0x3b54
//Bit 31:2      reserved
//Bit  1:0      AHB end address_high                    //unsigned, RW, default = 0, AHB end address[33:32] for  auto start source 10
#define FRC_RDMA_AHB_END_ADDR_10_T3X                    0x3b55
//Bit 31:0      AHB end address_low                     //unsigned, RW, default = 0, AHB end address[31:0] for auto start source 10
#define FRC_RDMA_AUTO_SRC11_SEL_T3X                     0x3b56
//Bit 31:0      ctrl_enable_int_11                      //unsigned, RW, default = 0
#define FRC_RDMA_AHB_START_ADDR_11_MSB_T3X              0x3b57
//Bit 31:2      reserved
//Bit  1:0      AHB start address_high                  //unsigned, RW, default = 0, AHB start address[33:32] for  auto start source 11
#define FRC_RDMA_AHB_START_ADDR_11_T3X                  0x3b58
//Bit 31:0      AHB start address_low                   //unsigned, RW, default = 0, AHB start address[31:0] for auto start source 11
#define FRC_RDMA_AHB_END_ADDR_11_MSB_T3X                0x3b59
//Bit 31:2      reserved
//Bit  1:0      AHB end address_high                    //unsigned, RW, default = 0, AHB end address[33:32] for  auto start source 11
#define FRC_RDMA_AHB_END_ADDR_11_T3X                    0x3b5a
//Bit 31:0      AHB end address_low                     //unsigned, RW, default = 0, AHB end address[31:0] for auto start source 11
#define FRC_RDMA_AUTO_SRC12_SEL_T3X                     0x3b5b
//Bit 31:0      ctrl_enable_int_12                      //unsigned, RW, default = 0
#define FRC_RDMA_AHB_START_ADDR_12_MSB_T3X              0x3b5c
//Bit 31:2      reserved
//Bit  1:0      AHB start address_high                  //unsigned, RW, default = 0, AHB start address[33:32] for  auto start source 12
#define FRC_RDMA_AHB_START_ADDR_12_T3X                  0x3b5d
//Bit 31:0      AHB start address_low                   //unsigned, RW, default = 0, AHB start address[31:0] for auto start source 12
#define FRC_RDMA_AHB_END_ADDR_12_MSB_T3X                0x3b5e
//Bit 31:2      reserved
//Bit  1:0      AHB end address_high                    //unsigned, RW, default = 0, AHB end address[33:32] for  auto start source 12
#define FRC_RDMA_AHB_END_ADDR_12_T3X                    0x3b5f
//Bit 31:0      AHB end address_low                     //unsigned, RW, default = 0, AHB end address[31:0] for auto start source 12
#define FRC_RDMA_AUTO_SRC13_SEL_T3X                     0x3b60
//Bit 31:0      ctrl_enable_int_13                      //unsigned, RW, default = 0
#define FRC_RDMA_AHB_START_ADDR_13_MSB_T3X              0x3b61
//Bit 31:2      reserved
//Bit  1:0      AHB start address_high                  //unsigned, RW, default = 0, AHB start address[33:32] for  auto start source 13
#define FRC_RDMA_AHB_START_ADDR_13_T3X                  0x3b62
//Bit 31:0      AHB start address_low                   //unsigned, RW, default = 0, AHB start address[31:0] for auto start source 13
#define FRC_RDMA_AHB_END_ADDR_13_MSB_T3X                0x3b63
//Bit 31:2      reserved
//Bit  1:0      AHB end address_high                    //unsigned, RW, default = 0, AHB end address[33:32] for  auto start source 13
#define FRC_RDMA_AHB_END_ADDR_13_T3X                    0x3b64
//Bit 31:0      AHB end address_low                     //unsigned, RW, default = 0, AHB end address[31:0] for auto start source 13
#define FRC_RDMA_AUTO_SRC14_SEL_T3X                     0x3b65
//Bit 31:0      ctrl_enable_int_14                      //unsigned, RW, default = 0
#define FRC_RDMA_AHB_START_ADDR_14_MSB_T3X              0x3b66
//Bit 31:2      reserved
//Bit  1:0      AHB start address_high                  //unsigned, RW, default = 0, AHB start address[33:32] for  auto start source 14
#define FRC_RDMA_AHB_START_ADDR_14_T3X                  0x3b67
//Bit 31:0      AHB start address_low                   //unsigned, RW, default = 0, AHB start address[31:0] for auto start source 14
#define FRC_RDMA_AHB_END_ADDR_14_MSB_T3X                0x3b68
//Bit 31:2      reserved
//Bit  1:0      AHB end address_high                    //unsigned, RW, default = 0, AHB end address[33:32] for  auto start source 14
#define FRC_RDMA_AHB_END_ADDR_14_T3X                    0x3b69
//Bit 31:0      AHB end address_low                     //unsigned, RW, default = 0, AHB end address[31:0] for auto start source 14
#define FRC_RDMA_AUTO_SRC15_SEL_T3X                     0x3b6a
//Bit 31:0      ctrl_enable_int_15                      //unsigned, RW, default = 0
#define FRC_RDMA_AHB_START_ADDR_15_MSB_T3X              0x3b6b
//Bit 31:2      reserved
//Bit  1:0      AHB start address_high                  //unsigned, RW, default = 0, AHB start address[33:32] for  auto start source 15
#define FRC_RDMA_AHB_START_ADDR_15_T3X                  0x3b6c
//Bit 31:0      AHB start address_low                   //unsigned, RW, default = 0, AHB start address[31:0] for auto start source 15
#define FRC_RDMA_AHB_END_ADDR_15_MSB_T3X                0x3b6d
//Bit 31:2      reserved
//Bit  1:0      AHB end address_high                    //unsigned, RW, default = 0, AHB end address[33:32] for  auto start source 15
#define FRC_RDMA_AHB_END_ADDR_15_T3X                    0x3b6e
//Bit 31:0      AHB end address_low                     //unsigned, RW, default = 0, AHB end address[31:0] for auto start source 15
//
// Closing file:  ./frc_inc/frc_rdma_regs.h
//
// -----------------------------------------------
// REG_BASE:  FRC_WRAP_APB_BASE = 0x3f
// -----------------------------------------------
//
// Reading file:  ./frc_inc/frc_wrap_reg.h
//
// synopsys translate_off
// synopsys translate_on
#define FRC_ASYNC_SW_RESETS                        0x3f00
//Bit 31:16 reserved
//Bit 15:0  reg_sw_resets         // unsigned ,   RW,default = 0,
#define FRC_TOP_CTRL                               0x3f01
//Bit 31:9  reserved
//Bit 8     reg_byp_mode_sel      // unsigned ,   RW, default = 1, frc Bypass singal sel byp_en = reg_byp_mode_sel ? reg_frc_top_byp :~reg_frc_en_in;
//Bit 7 :5  reserved
//Bit 4     reg_frc_top_byp       // unsigned ,   RW, default = 0, frc Bypass singal of frc,1:byp  frc 0:close byp
//Bit 3 :1  reserved
//Bit 0     reg_frc_en_in         // unsigned ,   RW, default = 0, frc enable singal of frc,1:open frc 0:close frc
#define FRC_AXI_ADDR_EXT_CTRL                      0x3f02
//Bit 31:14 reserved
//Bit 13:12 reg_default_addr_ext   // unsigned ,    RW,default = 0,axi address extend to 34bits,00:[0:4G) 01:[4G:8G) 01:[8G:12G) 11:[12G:16G)
//Bit 11:10 reserved
//Bit 9 :8  reg_me_addr_ext        // unsigned ,    RW,default = 0,axi address extend to 34bits,00:[0:4G) 01:[4G:8G) 01:[8G:12G) 11:[12G:16G)
//Bit 7 :6  reserved
//Bit 5 :4  reg_mc_chrm_addr_ext   // unsigned ,    RW,default = 0,axi address extend to 34bits,00:[0:4G) 01:[4G:8G) 01:[8G:12G) 11:[12G:16G)
//Bit 3 :2  reserved
//Bit 1 :0  reg_mc_luma_addr_ext   // unsigned ,    RW,default = 0,axi address extend to 34bits,00:[0:4G) 01:[4G:8G) 01:[8G:12G) 11:[12G:16G)
#define FRC_TOP_SCAN_REG                           0x3f03
//Bit 31:1  reserved
//Bit 0     reg_scan_reg          // unsigned ,    RW,default = 0,
#define FRC_TOTAL_SIZE                             0x3f04
//Bit 31:16 reg_frc_disp_vsize    // unsigned ,   RW,default = 1080, vtotal size of frc input
//Bit 15:0  reg_frc_disp_hsize    // unsigned ,   RW,default = 1920, htotal vtotal size of frc input
#define FRC_FRAME_SIZE                             0x3f05
//Bit 31:29 reserved
//Bit 28:16 reg_frc_frm_vsize     // unsigned ,    RW,default = 1080, vsize of frc input
//Bit 15:13 reserved
//Bit 12:0  reg_frc_frm_hsize     // unsigned ,    RW,default = 1920, hsize of frc input
#define FRC_AXI_CACHE                              0x3f06
//Bit 31:0  reg_frc_axi_cache     // unsigned ,    RW,default = 0,
#define FRC_AXIRD0_QLEVEL                          0x3f07
//Bit 31:0  reg_axird0_qlevel     // unsigned ,    RW,default = 0,
#define FRC_AXIRD1_QLEVEL                          0x3f08
//Bit 31:0  reg_axird1_qlevel     // unsigned ,    RW,default = 0,
#define FRC_AXIWR0_QLEVEL                          0x3f09
//Bit 31:0  reg_axiwr0_qlevel      // unsigned ,    RW,default = 0,
#define FRC_AXI_SYNC_CRC1                          0x3f0a
//Bit 31:12 reg_frc_axi_intf_ctrl  // unsigned ,    RW,default = 20'h44444,
//Bit 11    reserved
//Bit 10:8  reg_frc_bist_crc_ctrl  // unsigned ,    RW,default = 0,
//Bit 7 :3  pls_frc_crc_start      // unsigned ,    RW,default = 0,
//Bit 2 :0  reg_frc_intf_sw_rst    // unsigned ,    RW,default = 0,
#define FRC_ARB_BAK_CTRL                           0x3f0b
//Bit 31:26 reserved
//Bit 25:4  reg_apb_prot_ctrl     // unsigned ,    RW,default = 22'h200006,
//Bit 3 :1  reserved
//Bit 0     reg_arb_bak_ctrl      // unsigned ,    RW,default = 0,
#define FRC_AXI_URG_CTRL                           0x3f0c
//Bit 31:5  reserved
//Bit 4     reg_arb2_rd_urg       // unsigned ,    RW,default = 0,
//Bit 3     reg_arb1_rd_urg       // unsigned ,    RW,default = 0,
//Bit 2     reg_arb0_rd_urg       // unsigned ,    RW,default = 0,
//Bit 1     reg_arb1_wr_urg       // unsigned ,    RW,default = 0,
//Bit 0     reg_arb0_wr_urg       // unsigned ,    RW,default = 0,
#define FRC_RDMA_SYNC_CTRL                         0x3f0d
//Bit 31:8  reserved
//Bit 7     reg_rdma_rd_req_en        //unsigned ,    RW,default = 1,
//Bit 6     reg_rdma_rd_auto_gclk_en  //unsigned ,    RW,default = 0,
//Bit 5     reg_rdma_rd_disable_clk   //unsigned ,    RW,default = 0,
//Bit 4     reg_rdma_rd_sw_rst        //unsigned ,    RW,default = 0,
//Bit 3     reg_rdma_wr_req_en        //unsigned ,    RW,default = 1,
//Bit 2     reg_rdma_wr_auto_gclk_en  //unsigned ,    RW,default = 0,
//Bit 1     reg_rdma_wr_disable_clk   //unsigned ,    RW,default = 0,
//Bit 0     reserved
#define FRC_AXIRD0_CRC                             0x3f10
//Bit 31:0  ro_axird0_crc         // unsigned ,    RO, default = 0
#define FRC_AXIRD1_CRC                             0x3f11
//Bit 31:0  ro_axird1_crc         // unsigned ,    RO, default = 0
#define FRC_AXIRD2_CRC                             0x3f12
//Bit 31:0  ro_axird2_crc         // unsigned ,    RO, default = 0
#define FRC_AXIWR0_CRC0                            0x3f13
//Bit 31:0  ro_axiwr0_crc0        // unsigned ,    RO, default = 0
#define FRC_AXIWR0_CRC1                            0x3f14
//Bit 31:0  ro_axiwr0_crc1        // unsigned ,    RO, default = 0
#define FRC_AXIWR1_CRC0                            0x3f15
//Bit 31:0  ro_axiwr1_crc0        // unsigned ,    RO, default = 0
#define FRC_AXIWR1_CRC1                            0x3f16
//Bit 31:0  ro_axiwr1_crc1        // unsigned ,    RO, default = 0
#define FRC_APB_REQ_STAT                           0x3f17
//Bit 31:18 reserved
//Bit 17:8  ro_apb_prot_stat      // unsigned ,    RO, default = 0
//Bit 7 :1  reserved
//Bit 0     ro_rdma_ddr_req_busy  // unsigned ,    RO, default = 0
#define FRC_APB_CRASH_ADDR                         0x3f18
//Bit 31:16 reserved
//Bit 15:0  ro_apb_crash_addr      // unsigned ,    RO, default = 0
#define FRC_MODE_OPT                               0x3f20
//Bit 31:6  reserved
//Bit 5:0   reg_frc_mode_opt      // unsigned ,    RW, default = 0
#define FRC_RDAXI0_PROT_CTRL                       0x3f30
//Bit 31:22 reserved
//Bit 21:0  reg_rdaxi0_prot_ctrl          // unsigned ,RW, default = 22'h200002,{reg_prot_phs_en,reg_hold_num[18:0],reg_prot_en,sw_rst}
#define FRC_RDAXI0_PROT_STAT                       0x3f31
//Bit 31:14 reserved
//Bit 13:0  ro_rdaxi0_prot_stat           // unsigned ,RO, default = 0,{axi_rd_crash_num[9:0],axi_rd_crash_id[3:0]}
#define FRC_RDAXI1_PROT_CTRL                       0x3f35
//Bit 31:22 reserved
//Bit 21:0  reg_rdaxi1_prot_ctrl          // unsigned ,RW, default = 22'h200002,{reg_prot_phs_en,reg_hold_num[18:0],reg_prot_en,sw_rst}
#define FRC_RDAXI1_PROT_STAT                       0x3f36
//Bit 31:14 reserved
//Bit 13:0  ro_rdaxi1_prot_stat           // unsigned ,RO, default = 0,{axi_rd_crash_num[9:0],axi_rd_crash_id[3:0]}
#define FRC_RDAXI2_PROT_CTRL                       0x3f3a
//Bit 31:22 reserved
//Bit 21:0  reg_rdaxi2_prot_ctrl          // unsigned ,RW, default = 22'h200002,{reg_prot_phs_en,reg_hold_num[18:0],reg_prot_en,sw_rst}
#define FRC_RDAXI2_PROT_STAT                       0x3f3b
//Bit 31:14 reserved
//Bit 13:0  ro_rdaxi2_prot_stat           // unsigned ,RO, default = 0,{axi_rd_crash_num[9:0],axi_rd_crash_id[3:0]}
#define FRC_WRAXI0_PROT_CTRL                       0x3f40
//Bit 31:22 reserved
//Bit 21:0  reg_wraxi0_prot_ctrl          // unsigned ,RW, default = 22'h200002,{reg_prot_phs_en,reg_hold_num[18:0],reg_prot_en,sw_rst}
#define FRC_WRAXI0_PROT_STAT                       0x3f41
//Bit 31:14 reserved
//Bit 13:0  ro_wraxi0_prot_stat           // unsigned ,RO, default = 0,{axi_wr_crash_num[9:0],axi_wr_crash_id[3:0]}
#define FRC_WRAXI1_PROT_CTRL                       0x3f45
//Bit 31:22 reserved
//Bit 21:0  reg_wraxi1_prot_ctrl          // unsigned ,RW, default = 22'h200002,{reg_prot_phs_en,reg_hold_num[18:0],reg_prot_en,sw_rst}
#define FRC_WRAXI1_PROT_STAT                       0x3f46
//Bit 31:14 reserved
//Bit 13:0  ro_wraxi1_prot_stat           // unsigned ,RO, default = 0,{axi_wr_crash_num[9:0],axi_wr_crash_id[3:0]}
//==================================================================================
#define CLKCTRL_ME_CLK_CNTL                    0x89
#define CLKCTRL_FRC_CLK_CNTL                   0x8a

#define INP_ME_WRMIF                           0x90b
/*mif RO stat, bit[0] = done flag*/
#define INP_ME_WRMIF_CTRL                      0x900
/*mif reg_crc_on , bit[31]*/
#define INP_ME_WRMIF_CRC1                      0x901
/*mif RO stat, component1 ro crc*/
#define INP_ME_WRMIF_CRC2                      0x902
/*mif RO stat, component2 ro crc*/
#define INP_ME_WRMIF_CRC3                      0x903
/*mif RO stat, component3 ro crc*/

#define INP_ME_RDMIF                           0x913
/*mif RO stat,no done flag, but can check ddr cmd/data difference*/
#define INP_ME_RDMIF_CTRL                      0x910
/*mif reg_crc_on , bit[31]*/
#define INP_ME_RDMIF_CRC1                      0x914
/*mif RO crc, odd pix crc*/
#define INP_ME_RDMIF_CRC2                      0x915
/*mif RO crc, even pix crc*/

#define INP_MC_WRMIF                           0x92B
/*mif RO stat, bit[0] = done flag*/
#define INP_MC_WRMIF_CTRL                      0x920
/*mif reg_crc_on , bit[31]*/
#define INP_MC_WRMIF_CRC1                      0x921
/*mif RO stat, component1 ro crc*/
#define INP_MC_WRMIF_CRC2                      0x922
/*mif RO stat, component2 ro crc*/
#define INP_MC_WRMIF_CRC3                      0x923
/*mif RO stat, component3 ro crc*/

/*vpu top ctl base addr: 0xff000000*/
#define VPU_FRC_TOP_CTRL                 0x278d
#define VIU_FRC_MISC                     0x1a1c

#define ENCL_SYNC_LINE_LENGTH            0x1c4c
#define ENCL_SYNC_PIXEL_EN               0x1c4d
#define ENCL_SYNC_TO_LINE_EN             0x1c4e

#define ENCL_VIDEO_MAX_LNCNT             0x1cbb
#define ENCL_FRC_CTRL                    0x1cdd
#define ENCL_VIDEO_VAVON_BLINE           0x1cb4
#define ENCL_VIDEO_MAX_PXCNT             0x1cb0

#define VD1_BLEND_SRC_CTRL               0x1dfb
#define VPP_POSTBLEND_VD1_H_START_END    0x1d1c
#define VPP_POSTBLEND_VD1_V_START_END    0x1d1d

#define ENCL_FRC_CTRL_T3X                    0x14D9
#define ENCL_VIDEO_VAVON_BLINE_T3X           0x14CA
#define VD1_BLEND_SRC_CTRL_T3X               0x1d0d
#define VPP_POSTBLEND_VD1_H_START_END_T3X    0x1d03
#define VPP_POSTBLEND_VD1_V_START_END_T3X    0x1d04

extern void __iomem *frc_base;

/******************************************************************************/
extern u32 regdata_inpholdctl_0002;     // FRC_INP_HOLD_CTRL 0x0002
extern u32 regdata_outholdctl_0003;     // FRC_OUT_HOLD_CTRL 0x0003
extern u32 regdata_top_ctl_0007;        // FRC_REG_TOP_CTRL7  0x0007
extern u32 regdata_top_ctl_0009;        // FRC_REG_TOP_CTRL9
extern u32 regdata_top_ctl_000a;        // FRC_REG_TOP_CTRL10
extern u32 regdata_top_ctl_0011;        // FRC_REG_TOP_CTRL17

extern u32 regdata_pat_pointer_0102;
extern u32 regdata_loadorgframe[16];    // 0x0103

extern u32 regdata_phs_tab_0116;

extern u32 regdata_blksizexy_012b;
extern u32 regdata_blkscale_012c;
extern u32 regdata_hme_scale_012d;

extern u32 regdata_logodbg_0142;         // FRC_LOGO_DEBUG    0x0142
extern u32 inp_mcdw_ctl_047a;            // FRC_INP_MCDW_CTRL
extern u32 inp_path_opt_047c;            // FRC_INP_PATH_OPT
extern u32 regdata_inpmoden_04f9;        // FRC_REG_INP_MODULE_EN  0x04f9
extern u32 regdata_iplogo_en_0503;       // FRC_IPLOGO_EN    0x0503
extern u32 regdata_bbd_t2b_0604;         // FRC_BBD_DETECT_REGION_TOP2BOT  0x0604
extern u32 regdata_bbd_l2r_0605;         // FRC_BBD_DETECT_REGION_LFT2RIT  0x0605

extern u32 regdata_me_en_1100;           // FRC_ME_EN   0x1100
extern u32 regdata_me_bbpixed_1108;      // FRC_ME_BB_PIX_ED  0x1108
extern u32 regdata_me_bbblked_110a;      // FRC_ME_BB_BLK_ED  0x110a
extern u32 regdata_me_stat12rhst_110b;   // FRC_ME_STAT_12R_HST  0x110b
extern u32 regdata_me_stat12rh_110c;     // FRC_ME_STAT_12R_H01  0x110c
extern u32 regdata_me_stat12rh_110d;     // FRC_ME_STAT_12R_H23  0x110d
extern u32 regdata_me_stat12rv_110e;     // FRC_ME_STAT_12R_V0   0x110e
extern u32 regdata_me_stat12rv_110f;     // FRC_ME_STAT_12R_V1   0x110f

extern u32 regdata_vpbb1_1e03;          // FRC_VP_BB_1      0x1e03
extern u32 regdata_vpbb2_1e04;          // FRC_VP_BB_2      0x1e04
extern u32 regdata_vpmebb1_1e05;        // FRC_VP_ME_BB_1   0x1e05
extern u32 regdata_vpmebb2_1e06;        // FRC_VP_ME_BB_2   0x1e06

extern u32 regdata_vp_win1_1e58;        // FRC_VP_REGION_WINDOW_1 0x1e58
extern u32 regdata_vp_win2_1e59;        // FRC_VP_REGION_WINDOW_2 0x1e59
extern u32 regdata_vp_win3_1e5a;        // FRC_VP_REGION_WINDOW_3 0x1e5a
extern u32 regdata_vp_win4_1e5b;        // FRC_VP_REGION_WINDOW_4 0x1e5b

extern u32 regdata_mcset1_3000;         // FRC_MC_SETTING1   0x3000
extern u32 regdata_mcset2_3001;         // FRC_MC_SETTING2   0x3001

extern u32 regdata_mcdemo_win_3200;     // FRC_MC_DEMO_WINDOW  0x3200
extern u32 mcdw_path_ctl_39fd;          // FRC_MCDW_PATH_CTRL

extern u32 regdata_topctl_3f01;

///////////////////////////////////////////////////////////////////////////////

extern u32 regdata_fd_enable_0700;      // FRC_FD_ENABLE     0x0700
extern u32 regdata_film_phs1_0117;      // FRC_REG_FILM_PHS_1     0x0117
extern u32 regdata_me_stat_glb_apl_156c;// FRC_ME_STAT_GLB_APL    0x156c

extern u32 regdata_fwd_phs_0146;        // FRC_REG_FWD_PHS     0x0146
extern u32 regdata_fwd_phs_ro_016f;     //  FRC_REG_FWD_PHS_RO   0x016f
extern u32 regdata_fwd_phs_adj_016b;    // FRC_REG_FWD_PHS_ADJ       0x016b
extern u32 regdata_load_frame_flag0_0149;     // FRC_REG_LOAD_FRAME_FLAG_0  0x0149
extern u32 regdata_load_frame_flag1_014a;     //  FRC_REG_LOAD_FRAME_FLAG_1  0x014a
extern u32 regdata_fwd_table_cnt_phaofs_016c; //  FRC_REG_FWD_TABLE_CNT_PHAOFS 0x016c
extern u32 regdata_fwd_sign_ro_016e;          //  FRC_REG_FWD_SIGN_RO  x016e
extern u32 regdata_fwd_fid_0147;              // FRC_REG_FWD_FID     0x0147
extern u32 regdata_fwd_fid_posi_0148;         // FRC_REG_FWD_FID_POSI   0x0148

extern int fw_idx;

/******************************************************************************/
inline void WRITE_FRC_REG(unsigned int reg, unsigned int val);
inline void WRITE_FRC_REG_BY_CPU(unsigned int reg, unsigned int val);
inline void FRC_RDMA_WR_REG_IN(unsigned int reg, unsigned int val);
inline void FRC_RDMA_WR_REG_OUT(unsigned int reg, unsigned int val);
inline int READ_FRC_RDMA_REG(unsigned int reg);

inline void WRITE_FRC_BITS(unsigned int reg, unsigned int value,
    unsigned int start, unsigned int len);
inline void UPDATE_FRC_REG_BITS(unsigned int reg, unsigned int value, unsigned int mask);
// #define UPDATE_FRC_REG_BITS(addr, val, mask) FRC_RDMA_VSYNC_REG_UPDATE(addr, val, mask)
inline void UPDATE_FRC_REG_BITS_1(unsigned int reg, unsigned int value, unsigned int mask);
inline int is_rdma_enable(void);

inline int READ_FRC_REG(unsigned int reg);
inline u32 READ_FRC_BITS(u32 reg, const u32 start, const u32 len);
inline u32 floor_rs(u32 ix, u32 rs);
inline u32 ceil_rx(u32 ix, u32 rs);
inline s32  negative_convert(s32 data, u32 fbits);
inline void frc_config_reg_value(u32 need_val, u32 mask, u32 *reg_val);
void check_fw_table(u8 flag);

#endif
