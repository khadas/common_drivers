/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * drivers/amlogic/media/frc/frc_drv.h
 *
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#ifndef FRC_DRV_H
#define FRC_DRV_H

#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/amlogic/media/frc/frc_common.h>
#include <uapi/amlogic/frc.h>
#include "frc_interface.h"

// frc_20210720_divided_reg_table
// frc_20210804_add_powerdown_in_shutdown
// frc_20210806 frc support 4k1k@120hz
// frc_20210811_add_reinit_fw_alg_when_resume
// frc_20210812 frc add demo function
// frc_20210818 frc add read version function
// frc_20210824 frc video delay interface
// frc_20210907 frc set delay frame
// frc_20210918 frc chg me_dly_vofst
// frc_20210929 frc chg hold fow for null-vframe
// frc_20211008 frc protect opt frc register
// frc_20211021 frc chg me little_window
// frc_20211022 frc reduce clk under noframe
// frc_20211027 frc add print alg speed-time
// frc_20211104 frc add clkctrl workaround
// frc_20211109 frc chg demo mode
// frc_20211118 frc chg set max line to vlock module
// frc_20211119 frc set off when vf pic type
// frc_20211125 frc chg me little_window fhd
// frc_20211201 frc add detect error
// frc_20211202 fix frc reboot crash
// frc_20211208 frc set limit size
// frc_20211209 sync frc alg from vlsi 1414
// frc_20211206 frc add fpp set level
// frc_20211215 sync frc alg and fpp set level 1457
// frc_20211215 frc change by2dis
// frc_20211217 frc fixed video flickers
// frc_20220101 frc build memc interface
// frc_20220110 frc protected when no video
// frc_20220112 frc sync frame-rate for alg
// frc_20220111 frc close in high bandwidth"
// frc_20220112 frc mark demo setting"
// frc_20220119 add frc secure mode protection
// frc_20220207 frc sync frc_fw glb setting
// frc_20220215 frc fix char flashing of video
// frc_20220222 frc bypass pc and check vout
// frc_20220224 frc fix memc state abnormal
// frc_20220310 fix frc dts_match memory leak
// frc_20220404 fix frc input not standard
// frc_20220401 frc reduce cma buffer alloc
// frc_20220408 frc chg mcdly under 4k1k
// frc_20220421 frc sync memc_alg_ko_1990
// frc_20220425 frc inform vlock when disable
// frc_20220426 frc compute mcdly for vlock
// frc_20220505 frc check dbg roreg
// frc_20220524 frc memory optimize
// frc_20220608 optimize video flag check
// frc_20220613 fix frc memory resume abnormal
// frc_20220620 integrated frc status
// frc_20220623 add frc debug control
// frc_20220704 fix frc secure mode abnormal
// frc_20220705 fix frc bypass frame when on
// frc_20220708 optimize frc off2on flow
// frc_20220722 add vlock st and time record
// frc_20220801 set force_mode and ctrl freq
// frc_20220919 sync 24p and frm reset check
// frc_20220920 add frc rdma access reg
// frc_20220927 film tell alg under hdmi
// frc_20221102 frc clean up typo err
// frc_20221024 frc support t5m pxp
// frc_20221215 frc t5m bringup
// frc_20221225 add powerdomain etc.
// frc_20230301 frc add seamless mode
// frc_20230411 frc add enable double buffer mc
// frc_20230511 frc chg size for both support
// frc_20230404 frc t3x probe secure
// frc_20230526 frc fixed flashing once when enable
// frc_20230602 frc add ctrl for ko load or not
// frc_20230607 frc fixed input framerate
// frc_20230706 frc clk new flow
// frc_20230724 optimize frc seamless mode
// frc_20230802 adjust loss num
// frc_20230828 init vlock in bootup
// frc_20230908 t3x revB configure set_2
// frc_20230918 t3x revB use pre_vsync from vpu
// frc_20230921 fix frc security abnormal
// frc_20230920 remove hme buffer in t3x/t5m
// frc_20231009 delay frc function on boot
// frc_20231016 add frc pattern dbg ctrl
// frc_20231016 set disable with input is 120Hz
// frc_20231018 dly frc enable in video window
// frc_20231020 set disable stats when video mute enable
// frc_20231102 fix frc fhd windows flash
// frc_20231102 restore enable setting
// frc_20231031 frc compress mc memory usage size
// frc_20240111 n2m and vpu slice workaround
// frc_20240104 open clk when sys resume
// frc_20240116 frc rdma process optimisation
// frc_20240306 high-priority task and timestamp debug
// frc_20240312 frc protect badedit effect
// frc_2024-0319 frc sync alg macro
// frc_2024-0315 buf num configure
// frc_2024-0325 frc sync motion from drv

#define FRC_FW_VER			"2024-0407 frc modify urgent control"
#define FRC_KERDRV_VER                  3500

#define FRC_DEVNO	1
#define FRC_NAME	"frc"
#define FRC_CLASS_NAME	"frc"

#define CONFIG_AMLOGIC_MEDIA_FRC_RDMA

/*
extern int frc_dbg_en;
#define pr_frc(level, fmt, arg...)			\
	do {						\
		if ((frc_dbg_en >= (level) && frc_dbg_en < 3) || frc_dbg_en == level)	\
			pr_info("frc: " fmt, ## arg);	\
	} while (0)
*/
//------------------------------------------------------- buf define start
#define FRC_COMPRESS_RATE		60                 /*100: means no compress,60,80,50,55*/
#define FRC_COMPRESS_RATE_60_SIZE       (212 * 1024 * 1024)    // Need 209.2MB ( 0xD60000) 4MB Align
#define FRC_COMPRESS_RATE_80_SIZE       (276 * 1024 * 1024)    // Need 274.7MB  4MB Align
#define FRC_COMPRESS_RATE_50_SIZE       (180 * 1024 * 1024)    // Need 176.4MB  4MB Align
#define FRC_COMPRESS_RATE_55_SIZE       (196 * 1024 * 1024)    // Need 192.7MB  4MB Align
// mc-y 48%  mc-c 39%  me 60%
#define FRC_COMPRESS_RATE_MC_Y		45
#define FRC_COMPRESS_RATE_MC_C		35
#define FRC_COMPRESS_RATE_MC_Y_T3X	78 // 48 // 100
#define FRC_COMPRESS_RATE_MC_C_T3X	36 // 80 // 0

#define FRC_COMPRESS_RATE_ME_T3		60
#define FRC_COMPRESS_RATE_ME_T5M	100

#define FRC_COMPRESS_RATE_MCDW_Y	78 // 48
#define FRC_COMPRESS_RATE_MCDW_C	36 // 80 // 39

#define FRC_INFO_BUF_FACTOR_T3		2
#define FRC_INFO_BUF_FACTOR_T5M		2
#define FRC_INFO_BUF_FACTOR_T3X		32

#define FRC_COMPRESS_RATE_MARGIN    2

#define FRC_TOTAL_BUF_NUM		16
#define FRC_BUF_NUM_T3		FRC_TOTAL_BUF_NUM
#define FRC_BUF_NUM_T5M		FRC_TOTAL_BUF_NUM //  / 2
#define FRC_BUF_NUM_T3X		FRC_TOTAL_BUF_NUM

#define FRC_MEMV_BUF_NUM		6
#define FRC_MEMV2_BUF_NUM		7
#define FRC_MEVP_BUF_NUM		2

#define FRC_SLICER_NUM			4

/* mcdw setting */
#define FRC_MCDW_H_SIZE_RATE		2 // 1
#define FRC_MCDW_V_SIZE_RATE		1

/*down scaler config*/
#define FRC_ME_SD_RATE_HD		2
#define FRC_ME_SD_RATE_4K		4
#define FRC_LOGO_SD_RATE		1

/*only t3*/
#define FRC_HME_SD_RATE			4

#define FRC_HVSIZE_ALIGN_SIZE		16

#define FRC_V_LIMIT			144
#define FRC_H_LIMIT			128

/*bit number config*/
#define FRC_MC_BITS_NUM			10
#define FRC_ME_BITS_NUM			8

/*buff define and config*/
#define LOSSY_MC_INFO_LINE_SIZE		128	/*bytes*/
#define LOSSY_ME_INFO_LINE_SIZE		128	/*bytes*/
//------------------------------------------------------- buf define end
//------------------------------------------------------- clock defined start
#define FRC_CLOCK_OFF                0
#define FRC_CLOCK_OFF2MIN            1
#define FRC_CLOCK_MIN                2
#define FRC_CLOCK_MIN2NOR            3
#define FRC_CLOCK_NOR                4
#define FRC_CLOCK_NOR2MIN            5
#define FRC_CLOCK_MIN2OFF            6
#define FRC_CLOCK_OFF2NOR            7
#define FRC_CLOCK_NOR2OFF            8

#define FRC_CLOCK_XXX2NOR            9
#define FRC_CLOCK_NOR2XXX            10

#define FRC_CLOCK_MAX	             11

#define FRC_CLOCK_FIXED              0
#define FRC_CLOCK_DYNAMIC_0	     1
#define FRC_CLOCK_DYNAMIC_1          2

#define FRC_CLOCK_RATE_333                333333333
#define FRC_CLOCK_RATE_667                667000000
#define FRC_CLOCK_RATE_800                667000000

//------------------------------------------------------- clock defined end
// vd fps
#define FRC_VD_FPS_00    0
#define FRC_VD_FPS_60    60
#define FRC_VD_FPS_50    50
#define FRC_VD_FPS_48    48
#define FRC_VD_FPS_30    30
#define FRC_VD_FPS_25    25
#define FRC_VD_FPS_24    24
#define FRC_VD_FPS_120   120
#define FRC_VD_FPS_100   100
#define FRC_VD_FPS_144   144
#define FRC_VD_FPS_288   288

// ddr shift bits
#define DDR_SHFT_0_BITS   0
#define DDR_SHFT_2_BITS   2
#define DDR_SHFT_4_BITS   4

// frc flag define
#define FRC_FLAG_NORM_VIDEO		0x00
#define FRC_FLAG_GAME_MODE		0x01
#define FRC_FLAG_PC_MODE		0x02
#define FRC_FLAG_PIC_MODE		0x04
#define FRC_FLAG_HIGH_BW		0x08
#define FRC_FLAG_LIMIT_SIZE		0x10 // out of use
#define FRC_FLAG_VLOCK_ST		0x20
#define FRC_FLAG_INSIZE_ERR		0x40
#define FRC_FLAG_HIGH_FREQ		0x80
#define FRC_FLAG_ZERO_FREQ		0x100
#define FRC_FLAG_LIMIT_FREQ		0x200 // 144hz 288hz
#define FRC_FLAG_OTHER			0x400
#define FRC_FLAG_MUTE_ST		0x800

#define FRC_BUF_MC_Y_IDX	0
#define FRC_BUF_MC_C_IDX	1
#define FRC_BUF_MC_V_IDX	2
#define FRC_BUF_ME_IDX		3
#define FRC_BUF_MCDW_Y_IDX	4
#define FRC_BUF_MCDW_C_IDX	5
#define FRC_BUF_MAX_IDX		6

#define FRC_WIN_ALIGN_AUTO      0
#define FRC_WIN_ALIGN_MANU      1
#define FRC_WIN_ALIGN_DGB1      2
#define FRC_WIN_ALIGN_DGB2      3

#define FRC_RES_NONE	0
#define FRC_RES_2K1K	1
#define FRC_RES_4K2K	2
#define FRC_RES_4K1K	3
#define FRC_RES__MAX	4

#define PRE_VSYNC_120HZ   BIT_0
#define PRE_VSYNC_060HZ   BIT_1

#define FRC_BOOT_TIMESTAMP 35

#define FRC_DELAY_FRAME_2OPEN  1

#define FRC_BYPASS_FRAME_NUM  3
#define FRC_FREEZE_FRAME_NUM  3    // should less than 16

#define FRC_BYPASS_FRAME_NUM_T3X  7
#define FRC_FREEZE_FRAME_NUM_T3X  8  // should less than 16

enum chip_id {
	ID_NULL = 0,
	ID_T3,
	ID_T5M,
	ID_T3X,
	ID_TMAX,
};

struct dts_match_data {
	enum chip_id chip;
};

struct frc_data_s {
	const struct dts_match_data *match_data;
};

struct vf_rate_table {
	u16 duration;
	u16 framerate;
};

struct frm_dly_dat_s {
	u16 mevp_frm_dly;
	u16 mc_frm_dly;
};

struct st_frc_buf {
	/*cma memory define*/
	u32 cma_mem_size;
	struct page *cma_mem_paddr_pages;
	phys_addr_t cma_mem_paddr_start;
	u8  cma_mem_alloced;
	u8  secured;
	u8  frm_buf_num;
	u8  logo_buf_num;
	u8  rate_margin;
	u8  addr_shft_bits;
	u8  mcdw_size_rate;

	u8  me_comprate;
	u8  mc_y_comprate;
	u8  mc_c_comprate;
	u8  memc_comprate;
	u8  mcdw_y_comprate;
	u8  mcdw_c_comprate;
	u8  info_factor;

	/*frame size*/
	u32 in_hsize;
	u32 in_vsize;

	/*align size*/
	u32 in_align_hsize;
	u32 in_align_vsize;
	u32 me_hsize;
	u32 me_vsize;

	/*only t3*/
	u32 hme_hsize;
	u32 hme_vsize;
	u32 hme_blk_hsize;
	u32 hme_blk_vsize;

	u32 me_blk_hsize;
	u32 me_blk_vsize;
	u32 logo_hsize;
	u32 logo_vsize;

	/*info buffer*/
	u32 lossy_mc_y_info_buf_size;
	u32 lossy_mc_c_info_buf_size;
	u32 lossy_mc_v_info_buf_size;
	u32 lossy_me_x_info_buf_size;
	/*info buffer for t3x*/
	u32 lossy_mcdw_y_info_buf_size;
	u32 lossy_mcdw_c_info_buf_size;

	u32 lossy_mc_y_info_buf_paddr;
	u32 lossy_mc_c_info_buf_paddr;
	u32 lossy_mc_v_info_buf_paddr;
	u32 lossy_me_x_info_buf_paddr;
	/*info buffer for t3x*/
	u32 lossy_mcdw_y_info_buf_paddr;
	u32 lossy_mcdw_c_info_buf_paddr;

	/*data buffer*/
	u32 lossy_mc_y_data_buf_size[FRC_TOTAL_BUF_NUM];
	u32 lossy_mc_c_data_buf_size[FRC_TOTAL_BUF_NUM];
	u32 lossy_mc_v_data_buf_size[FRC_TOTAL_BUF_NUM];/*444 mode use*/
	u32 lossy_me_data_buf_size[FRC_TOTAL_BUF_NUM];
	/*data buffer for t3x*/
	u32 lossy_mcdw_y_data_buf_size[FRC_TOTAL_BUF_NUM];
	u32 lossy_mcdw_c_data_buf_size[FRC_TOTAL_BUF_NUM];

	u32 lossy_mc_y_data_buf_paddr[FRC_TOTAL_BUF_NUM];
	u32 lossy_mc_c_data_buf_paddr[FRC_TOTAL_BUF_NUM];
	u32 lossy_mc_v_data_buf_paddr[FRC_TOTAL_BUF_NUM];
	u32 lossy_me_data_buf_paddr[FRC_TOTAL_BUF_NUM];
	/*data buffer for t3x*/
	u32 lossy_mcdw_y_data_buf_paddr[FRC_TOTAL_BUF_NUM];
	u32 lossy_mcdw_c_data_buf_paddr[FRC_TOTAL_BUF_NUM];

	/*link buffer*/
	u32 lossy_mc_y_link_buf_size[FRC_TOTAL_BUF_NUM];
	u32 lossy_mc_c_link_buf_size[FRC_TOTAL_BUF_NUM];
	u32 lossy_mc_v_link_buf_size[FRC_TOTAL_BUF_NUM];/*444 mode use*/
	u32 lossy_me_link_buf_size[FRC_TOTAL_BUF_NUM];
	/*link buffer size for t3x*/
	u32 lossy_mcdw_y_link_buf_size[FRC_TOTAL_BUF_NUM];
	u32 lossy_mcdw_c_link_buf_size[FRC_TOTAL_BUF_NUM];

	u32 lossy_mc_y_link_buf_paddr[FRC_TOTAL_BUF_NUM];
	u32 lossy_mc_c_link_buf_paddr[FRC_TOTAL_BUF_NUM];
	u32 lossy_mc_v_link_buf_paddr[FRC_TOTAL_BUF_NUM];
	u32 lossy_me_link_buf_paddr[FRC_TOTAL_BUF_NUM];
	/*link buffer addr for t3x*/
	u32 lossy_mcdw_y_link_buf_paddr[FRC_TOTAL_BUF_NUM];
	u32 lossy_mcdw_c_link_buf_paddr[FRC_TOTAL_BUF_NUM];

	/*norm buffer*/
	u32 norm_hme_data_buf_size[FRC_TOTAL_BUF_NUM];// only t3
	u32 norm_memv_buf_size[FRC_MEMV_BUF_NUM];
	u32 norm_hmemv_buf_size[FRC_MEMV2_BUF_NUM];// only t3
	u32 norm_mevp_out_buf_size[FRC_MEVP_BUF_NUM];

	u32 norm_hme_data_buf_paddr[FRC_TOTAL_BUF_NUM];// only t3
	u32 norm_memv_buf_paddr[FRC_MEMV_BUF_NUM];
	u32 norm_hmemv_buf_paddr[FRC_MEMV2_BUF_NUM];// only t3
	u32 norm_mevp_out_buf_paddr[FRC_MEVP_BUF_NUM];

	/*logo buffer*/
	u32 norm_iplogo_buf_size[FRC_TOTAL_BUF_NUM];
	u32 norm_logo_irr_buf_size;
	u32 norm_logo_scc_buf_size;
	u32 norm_melogo_buf_size[FRC_TOTAL_BUF_NUM];

	u32 norm_iplogo_buf_paddr[FRC_TOTAL_BUF_NUM];
	u32 norm_logo_irr_buf_paddr;
	u32 norm_logo_scc_buf_paddr;
	u32 norm_melogo_buf_paddr[FRC_TOTAL_BUF_NUM];

	s32 link_tab_size;
	phys_addr_t secure_start;
	s32 secure_size;
	u32 total_size;
	u32 real_total_size;
};

struct st_frc_sts {
	u32 auto_ctrl;
	enum frc_state_e state;
	enum frc_state_e new_state;
	u32 state_transing;
	u32 frame_cnt;
	u8 changed_flag;
	u32 vs_cnt;
	u32 re_cfg_cnt;
	u32 out_put_mode_changed;
	u32 re_config;
	u32 inp_undone_cnt;
	u32 me_undone_cnt;
	u32 mc_undone_cnt;
	u32 vp_undone_cnt;
	u32 retrycnt;
	u32 video_mute_cnt;
	u32 vs_data_cnt;
};

struct st_frc_in_sts {
	u32 vf_sts;		/*0:no vframe input, 1:have vframe input, other: unknown*/
	u32 vf_type;		/*vframe type*/
	u32 duration;		/*vf duration*/
	u32 in_hsize;
	u32 in_vsize;
	u32 signal_type;
	u32 source_type;	/*enum vframe_source_type_e*/
	struct vframe_s *vf;
	u32 vf_repeat_cnt;
	u32 vf_null_cnt;

	u32 vs_cnt;
	u32 vs_tsk_cnt;
	u32 vs_duration;
	u64 vs_timestamp;

	u32 have_vf_cnt;
	u32 no_vf_cnt;
	u32 vf_index;

	u32 secure_mode;
	u32 st_flag;

	u32  high_freq_en;
	u32  high_freq_flash; /*0 default, 1: high freq char flash*/
	u8  inp_size_adj_en;  /*input non-standard size, default 0 is open*/

	/*vd status sync*/
	u8 frc_is_tvin;
	u8 frc_source_chg;
	u16 frc_vf_rate;
	u32 frc_last_disp_count;

	u32 frc_hd_start_lines;
	u32 frc_hd_end_lines;
	u32 frc_vd_start_lines;
	u32 frc_vd_end_lines;
	u32 frc_vsc_startp;
	u32 frc_vsc_endp;
	u32 frc_hsc_startp;
	u32 frc_hsc_endp;
	u8 frc_seamless_en;
	u8 size_chged;
	u8 boot_timestamp_en;
	u8 boot_check_finished;
	u8 auto_ctrl_reserved;
	u8 enable_mute_flag;
	u8 mute_vsync_cnt;
	u8 hi_en;
	u8 frm_en;
	u8 t3x_proc_size_chg;
};

struct st_frc_out_sts {
	u16 out_framerate;
	u32 vout_height;
	u32 vout_width;

	u32 vs_cnt;
	u32 vs_tsk_cnt;
	u32 vs_duration;
	u64 vs_timestamp;
	u8 hi_en;
};

struct tool_debug_s {
	u32 reg_read;
	u32 reg_read_val;
};

struct dbg_dump_tab {
	u8 *name;
	u32 addr;
	u32 start;
	u32 len;
};

enum frc_mtx_e {
	FRC_INPUT_CSC = 1,
	FRC_OUTPUT_CSC,
};

enum frc_mtx_csc_e {
	CSC_OFF = 0,
	RGB_YUV709L,
	RGB_YUV709F,
	YUV709L_RGB,
	YUV709F_RGB,
	PAT_RD,
	PAT_GR,
	PAT_BU,
	PAT_WT,
	PAT_BK,
	DEFAULT,
};

struct frc_csc_set_s {
	u32 enable;
	u32 coef00_01;
	u32 coef02_10;
	u32 coef11_12;
	u32 coef20_21;
	u32 coef22;
	u32 pre_offset01;
	u32 pre_offset2;
	u32 pst_offset01;
	u32 pst_offset2;
};

struct crc_parm_s {
	u32 crc_en;
	u32 crc_done_flag;
	u32 crc_data_cmp[2];/*3cmp*/
};

struct frc_crc_data_s {
	u32 frc_crc_read;
	u32 frc_crc_pr;
	struct crc_parm_s me_wr_crc;
	struct crc_parm_s me_rd_crc;
	struct crc_parm_s mc_wr_crc;
};

struct frc_dmc_cfg_s {
	unsigned int id;
	unsigned long ddr_addr;
	unsigned long ddr_size;
};

struct frc_ud_s {
	unsigned inp_ud_dbg_en:1;
	unsigned meud_dbg_en:1;
	unsigned mcud_dbg_en:1;
	unsigned vpud_dbg_en:1;
	unsigned res0_dbg_en:1;
	unsigned res1_dbg_en:1;
	unsigned res2_dbg_en:2;

	unsigned inud_time_en:1;
	unsigned outud_time_en:1;
	unsigned res1_time_en:1;
	unsigned res2_time_en:5;

	// unsigned yuv444to422_err:1;
	// unsigned blend_ud_err:1;
	// unsigned me_dwscl_err:1;
	// unsigned smp_nr_err:1;
	// unsigned hme_dwscl_err:1;only t3
	unsigned inp_undone_err:8;

	unsigned other4_err:1;
	unsigned other3_err:1;
	unsigned other2_err:1;
	unsigned other1_err:1; // t3xb used
	unsigned mc_undone_err:1;
	unsigned me_undone_err:1;
	unsigned vp_undone_err:2;

	u8 pr_dbg;
	u8 align_dbg_en;
};

struct frc_force_size_s {
	u32 force_en;
	u32 force_hsize;
	u32 force_vsize;
};

union frc_ro_dbg0_stat_u {
	u32 rawdat;
	struct {
		unsigned ref_de_dly_num:16;  /*15:0*/
		unsigned ref_vs_dly_num:16;  /*31:16*/
	} bits;
};

union frc_ro_dbg1_stat_u {
	u32 rawdat;
	struct {
		unsigned mcout_dly_num:16; /*15:0*/
		unsigned mevp_dly_num:16;  /*31:16*/
	} bits;
};

union frc_ro_dbg2_stat_u {
	u32 rawdat;
	struct {
	    unsigned out_de_dly_num:13;     /*12:0*/
		unsigned out_dly_err: 1;        /*13*/
		unsigned reserved2: 2;          /*15:14*/
		unsigned memc_corr_dly_num :13; /*28:16*/
		unsigned memc_corr_st:2;        /*30:29*/
		unsigned reserved1:1;
	} bits;
};

struct frc_hw_stats_s {
	union frc_ro_dbg0_stat_u reg4dh;
	union frc_ro_dbg1_stat_u reg4eh;
	union frc_ro_dbg2_stat_u reg4fh;
};

struct frc_pat_dbg_s {
	u8 pat_en;
	u8 pat_type;
	u8 pat_color;
	u8 pat_reserved;
};

struct frc_timer_dbg {
	u8 timer_en;
	u8 time_interval;
	u16 timer_level; // dbg level
};

struct frc_dev_s {
	dev_t devt;
	struct cdev cdev;
	dev_t devno;
	struct device *dev;
	/*power domain for dsp*/
	struct device *pd_dev;
	struct class *clsp;
	struct platform_device	*pdev;

	unsigned int frc_en;		/*0:frc disabled in dts; 1:frc enable in dts*/
	enum eFRC_POS frc_hw_pos;	/*0:before postblend; 1:after postblend*/
	unsigned int frc_test_ptn;
	unsigned int frc_fw_pause;
	u32 probe_ok;
	u32 power_on_flag;
	u32 power_off_flag;
	struct frc_data_s *data;
	void *fw_data;

	int in_irq;
	char in_irq_name[20];
	int out_irq;
	char out_irq_name[20];
	int axi_crash_irq;
	char axi_crash_irq_name[20];
	int rdma_irq;
	char rdma_irq_name[20];

	void __iomem *reg;
	void __iomem *clk_reg;
	void __iomem *vpu_reg;
	struct clk *clk_frc;
	u32 clk_frc_frq;
	struct clk *clk_me;
	u32 clk_me_frq;
	unsigned int clk_state;
	u32 clk_chg;

	/* vframe check */
	u32 vs_duration;	/*vpu int duration*/
	u64 vs_timestamp;	/*vpu int time stamp*/
	u32 in_out_ratio;
	u8 st_change;
	u8 need_bypass;	/*notify vpu to bypass frc*/
	u8 next_frame;

	u32 dbg_force_en;
	u32 dbg_in_out_ratio;
	u32 dbg_input_hsize;
	u32 dbg_input_vsize;
	u32 dbg_reg_monitor_i;
	u32 dbg_in_reg[MONITOR_REG_MAX];
	u32 dbg_reg_monitor_o;
	u32 dbg_out_reg[MONITOR_REG_MAX];
	u32 dbg_buf_len;
	u32 dbg_vf_monitor;
	u16 dbg_mvrd_mode;
	u16 dbg_mute_disable;
	u8  little_win;
	u8  vlock_flag;
	u8  use_pre_vsync; /* bit_0:120hz_enable , bit_1: 60hz enable */
	u8  test2;         /* test patch function*/

	u8  prot_mode;
	u8  no_ko_mode;
	u8  other1_flag;
	u8  other2_flag;

	u32 film_mode;
	u32 film_mode_det;/*0: hw detect, 1: sw detect*/
	u32 auto_n2m;
	u32 out_line;/*ctl mc out line for user*/

	u32 vpu_byp_frc_reg_addr;

	struct tasklet_struct input_tasklet;
	struct tasklet_struct output_tasklet;

	//struct workqueue_struct *frc_wq;
	//struct work_struct frc_work;
	struct work_struct frc_clk_work;
	struct work_struct frc_print_work;
	struct work_struct frc_secure_work;

	struct st_frc_sts frc_sts;
	struct st_frc_in_sts in_sts;
	struct st_frc_out_sts out_sts;

	struct st_frc_buf buf;
	struct frm_dly_dat_s frm_dly_set[4];

	struct tool_debug_s tool_dbg;
	struct frc_crc_data_s frc_crc_data;
	struct frc_ud_s ud_dbg;
	struct frc_force_size_s force_size;
	struct frc_hw_stats_s hw_stats;
	struct frc_dmc_cfg_s  dmc_cfg[3];
	struct frc_csc_set_s init_csc[2];
	struct frc_pat_dbg_s pat_dbg;
	struct frc_timer_dbg timer_dbg;
};

extern struct hrtimer frc_hi_timer;

struct frc_dev_s *get_frc_devp(void);
void get_vout_info(struct frc_dev_s *frc_devp);
int frc_buf_set(struct frc_dev_s *frc_devp);
void set_vsync_2to1_mode(u8 enable);
void set_pre_vsync_mode(u8 enable);
struct frc_fw_data_s *get_fw_data(void);
#endif
