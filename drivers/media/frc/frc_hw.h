/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __FRC_CLK_H__
#define __FRC_CLK_H__

#define HEIGHT_2K 2160
#define WIDTH_4K  3840
#define HEIGHT_1K 1080
#define WIDTH_2K  1920

#define VSIZE_MAX_2K  900
#define VSIZE_MIN_2K  500
#define VSIZE_MAX_1K  390
#define VSIZE_MIN_1K  260

#define HSIZE_MAX_4K  1500
#define HSIZE_MIN_4K  900
#define HSIZE_MAX_2K  680
#define HSIZE_MIN_2K  470

#define  HME_ME_OFFSET 0x100

//#define HME_MVX_BIT		10	//include sign bit for HME@960x540
//#define HME_MVY_BIT		9	//include sign bit for HME@960x540
//#define MAX_HME_MVX       127  // for HME@960x540
//#define MAX_HME_MVY       40   // for HME@960x540
//#define HME_FINER_HIST_BIT     4  // actually not used for HME
//#define HME_ROUGH_X_HIST_BIT   4  // include sign bit for HME@960x540
//#define HME_ROUGH_Y_HIST_BIT   3  // include sign bit for HME@960x540

/*osd only always support 960x540  0: 960x540, 1: 1920x1080*/
#define FW_OSD_MC_RATIO 0  //0->1:1; 1->1:2

/*ME max size 960x540*/
#define ME_MVX_BIT		12	//include sign bit for ME@960x540
#define ME_MVY_BIT		11	//include sign bit for ME@960x540
#define	MAX_ME_MVX		511  // for ME@960x540  => means +/-128 ME pixels for MVx
#define	MAX_ME_MVY		160  // for ME@960x540  => means (40+2)+(40+4+2) line memory

#define ME_FINER_HIST_BIT     4 //
#define ME_ROUGH_X_HIST_BIT   4 // include sign bit for ME@960x540
#define ME_ROUGH_Y_HIST_BIT   3 // include sign bit for ME@960x540

#define MAX_MC_Y_VRANG   64  // one side of Luma range(under MC scale)
#define MAX_MC_C_VRANG   64  // one side of Chroma range

#define MAX_INP_UNDONE_CNT         0x3F // 1
#define MAX_ME_UNDONE_CNT          1
#define MAX_MC_UNDONE_CNT          1
#define MAX_VP_UNDONE_CNT          1

//Bit 28:24  reg_buf_cfg_en       // unsigned ,    RW, default = 0
// [0]:force mc buf_idx [1] force logo buf_idx [2]:force me_phase
// [3] force mc_phase [4] force input_buf_idx
#define FORCE_MC_BUFIDX            0x01000000
#define FORCE_LOGO_BUFIDX          0x02000000
#define FORCE_ME_PHASE             0x04000000
#define FORCE_MC_PHASE             0x08000000
#define FORCE_INPUT_BUFIDX         0x10000000

#define FRAME_RATE_CNT      17
#define FRC_DBG_DUMP_TABLE_NUM       18

extern void __iomem *frc_clk_base;
extern void __iomem *vpu_base;
void frc_clk_init(struct frc_dev_s *frc_devp);
void set_frc_clk_disable(struct frc_dev_s *frc_devp,  u8 disable);
void frc_init_config(struct frc_dev_s *devp);
void set_frc_enable(u32 en);
void set_frc_bypass(u32 en);
void frc_pattern_on(u32 en);
void frc_set_buf_num(u32 frc_fb_num);
void frc_top_init(struct frc_dev_s *frc_devp);
// void frc_inp_init(u32 frc_fb_num, u32 film_hwfw_sel);
void frc_inp_init(void);
void config_phs_lut(enum frc_ratio_mode_type frc_ratio_mode,
	enum en_drv_film_mode film_mode);
void config_phs_regs(enum frc_ratio_mode_type frc_ratio_mode,
	enum en_drv_film_mode film_mode);
void config_me_top_hw_reg(void);
void sys_fw_param_frc_init(u32 frm_hsize, u32 frm_vsize, u32 is_me1mc4);
void config_loss_out(u32 fmt422);
void enable_nr(void);
void set_mc_lbuf_ext(void);
void frc_cfg_memc_loss(u32 memc_loss_en);
void frc_cfg_mcdw_loss(u32 mcdw_loss_en);
void recfg_memc_mif_base_addr(u32 base_ofst);
void frc_internal_initial(struct frc_dev_s *frc_devp);
void frc_dump_reg_tab(void);
void frc_mtx_set(struct frc_dev_s *frc_devp);
void frc_crc_enable(struct frc_dev_s *frc_devp);
void frc_me_crc_read(struct frc_dev_s *frc_devp);
void frc_mc_crc_read(struct frc_dev_s *frc_devp);
void me_undone_read(struct frc_dev_s *frc_devp);
void mc_undone_read(struct frc_dev_s *frc_devp);
void frc_dump_fixed_table(void);
void frc_reset(u32 onoff);
void frc_mc_reset(u32 onoff);
void frc_force_secure(u32 onoff);
void frc_osdbit_setfalsecolor(struct frc_dev_s *frc_devp, u32 falsecolor);
u8 frc_frame_forcebuf_enable(u8 enable);
void frc_frame_forcebuf_count(u8 forceidx);
void inp_undone_read(struct frc_dev_s *frc_devp);
void vp_undone_read(struct frc_dev_s *frc_devp);
u32 vpu_reg_read(u32 addr);
void vpu_reg_write(u32 addr, u32 value);
void frc_check_hw_stats(struct frc_dev_s *frc_devp, u8 checkflag);
u16 frc_check_vf_rate(u16 duration, struct frc_dev_s *frc_devp);
void frc_get_film_base_vf(struct frc_dev_s *frc_devp);
void frc_set_enter_forcefilm(struct frc_dev_s *frc_devp, u16 flag);
void frc_set_notell_film(struct frc_dev_s *frc_devp, u16 flag);
void frc_set_val_from_reg(void);
/* pat: 1-red,2-green,3-blue,4-white,5-black */
void frc_set_input_pattern(u8 enpat);
void frc_set_output_pattern(u8 enpat);
void frc_set_urgent_cfg(u8 ch, u8 level);
void frc_input_init(struct frc_dev_s *frc_devp,
	struct frc_top_type_s *frc_top);
void frc_set_n2m(u8 ratio_value);
void frc_set_axi_crash_irq(struct frc_dev_s *frc_devp, u8 enable);
int get_chip_type(void);
void frc_set_h2v2(u32 enable);
void frc_set_mcdw_buffer_ratio(u32 ratio);
void frc_memc_120hz_patch(struct frc_dev_s *frc_devp);
void frc_memc_120hz_patch_1(struct frc_dev_s *frc_devp);
void frc_memc_120hz_patch_2(struct frc_dev_s *frc_devp);
void frc_memc_120hz_patch_3(struct frc_dev_s *frc_devp);
void frc_debug_table_print(struct work_struct *work);
void frc_debug_print(struct frc_dev_s *devp);
void frc_memc_clr_vbuffer(struct frc_dev_s *frc_devp, u8 flag);
void frc_load_reg_table(struct frc_dev_s *frc_devp, u8 flag);
void frc_in_sts_init(struct st_frc_in_sts *sts);
void frc_chg_loss_slice_num(u8 num);
void t3x_eco_initial(void);
void t3x_eco_qp_cfg(u32 qp);
void t3x_revB_patch_apply(void);
void t3x_verB_set_cfg(u8 flag, struct frc_dev_s *frc_devp);
void frc_pattern_dbg_ctrl(struct frc_dev_s *devp);
void t3x_verB_60hz_patch(void);
void frc_clr_badedit_effect_before_enable(void);

#endif
