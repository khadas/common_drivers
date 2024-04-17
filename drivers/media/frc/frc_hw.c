// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/types.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_fdt.h>
#include <linux/of_reserved_mem.h>
#include <linux/of_irq.h>
#include <linux/of_clk.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/errno.h>
#include <linux/uaccess.h>
#include <linux/ioport.h>
#include <linux/ctype.h>
#include <linux/vmalloc.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/clk.h>
#include <linux/sched/clock.h>
#include <linux/amlogic/media/registers/register_map.h>
#include <linux/amlogic/iomap.h>
#include <linux/amlogic/media/frc/frc_reg.h>
#include <linux/amlogic/media/amvecm/amvecm.h>
#include "frc_drv.h"
#include "frc_hw.h"
#include "frc_regs_table.h"
#include "frc_proc.h"
#include "vlock.h"

void __iomem *frc_clk_base;
void __iomem *vpu_base;

struct stvlock_frc_param gst_frc_param;

int FRC_PARAM_NUM = 8;
module_param(FRC_PARAM_NUM, int, 0664);
MODULE_PARM_DESC(FRC_PARAM_NUM, "FRC_PARAM_NUM");

const struct vf_rate_table vf_rate_table[FRAME_RATE_CNT] = {
	{800,   FRC_VD_FPS_120},
	{801,   FRC_VD_FPS_120},
	{960,	FRC_VD_FPS_100},
	{961,	FRC_VD_FPS_100},
	{1600,  FRC_VD_FPS_60},
	{1601,	FRC_VD_FPS_60},
	{1920,  FRC_VD_FPS_50},
	{2000,  FRC_VD_FPS_48},
	{3199,	FRC_VD_FPS_30},
	{3200,	FRC_VD_FPS_30},
	{3203,	FRC_VD_FPS_30},
	{3840,	FRC_VD_FPS_25},
	{3999,	FRC_VD_FPS_24},
	{4000,	FRC_VD_FPS_24},
	{4001,  FRC_VD_FPS_24},
	{4004,  FRC_VD_FPS_24},
	{0000,	FRC_VD_FPS_00},
};

u32 vpu_reg_read(u32 addr)
{
	unsigned int value = 0;

	value = aml_read_vcbus_s(addr);
	return value;
}

void vpu_reg_write(u32 addr, u32 value)
{
	aml_write_vcbus_s(addr, value);
}

void vpu_reg_write_bits(unsigned int reg, unsigned int value,
					   unsigned int start, unsigned int len)
{
	aml_vcbus_update_bits_s(reg, value, start, len);
}

//.clk0: ( fclk_div3), //666 div: 0 1/1, 1:1/2, 2:1/4
//.clk1: ( fclk_div4), //500 div: 0 1/1, 1:1/2, 2:1/4
//.clk2: ( fclk_div5), //400 div: 0 1/1, 1:1/2, 2:1/4
//.clk3: ( fclk_div7), //285 div: 0 1/1, 1:1/2, 2:1/4
//.clk4: ( mp1_clk_out),
//.clk5: ( vid_pll0_clk),
//.clk6: ( hifi_pll_clk),
//.clk7: ( sys1_pll_clk),
void set_frc_div_clk(int sel, int div)
{
	unsigned int val = 0;
	unsigned int reg;

	reg = CLKCTRL_FRC_CLK_CNTL;

	val = readl(frc_clk_base + (reg << 2));

	if (val == 0 || (val & (1 << 31))) {
		val &= ~((0x7 << 9) |
				(1 << 8) |
				(0x7f << 0));
		val |= (sel << 9) |
			   (1 << 8) |
			   (div << 0);
		val &= ~(1 << 31);
		val |= 1 << 30;
		writel(val, (frc_clk_base + (reg << 2)));
	} else {
		val &= ~((0x7 << 25) |
				(1 << 24) |
				(0x7f << 16));
		val |= (sel << 25) |
			   (1 << 24) |
			   (div << 16);
		val |= 1 << 31;
		val |= 1 << 30;
		writel(val, (frc_clk_base + (reg << 2)));
	}
}

void set_me_div_clk(int sel, int div)
{
	unsigned int val = 0;
	unsigned int reg;

	reg = CLKCTRL_ME_CLK_CNTL;

	val = readl(frc_clk_base + (reg << 2));

	if (val == 0 || (val & (1 << 31))) {
		val &= ~((0x7 << 9) |
				(1 << 8) |
				(0x7f << 0));
		val |= (sel << 9) |
			   (1 << 8) |
			   (div << 0);
		val &= ~(1 << 31);
		val |= 1 << 30;
		writel(val, (frc_clk_base + (reg << 2)));
	} else {
		val &= ~((0x7 << 25) |
				(1 << 24) |
				(0x7f << 16));
		val |= (sel << 25) |
			   (1 << 24) |
			   (div << 16);
		val |= 1 << 31;
		val |= 1 << 30;
		writel(val, (frc_clk_base + (reg << 2)));
	}
}

void set_frc_clk_disable(struct frc_dev_s *frc_devp,  u8 disable)
{
	if (disable == 1) {
		if (frc_devp->clk_state == FRC_CLOCK_OFF)
			return;
		if (IS_ERR(frc_devp->clk_me))
			PR_ERR("%s: frc_devp->clk_me to disable", __func__);
		else
			clk_disable_unprepare(frc_devp->clk_me);
		if (IS_ERR(frc_devp->clk_frc))
			PR_ERR("%s: frc_devp->clk_me to disable", __func__);
		else
			clk_disable_unprepare(frc_devp->clk_frc);
		frc_devp->clk_state = FRC_CLOCK_OFF;
		pr_frc(0, "dis frc_clk\n");
	} else {
		if (frc_devp->clk_state == FRC_CLOCK_NOR)
			return;
		if (IS_ERR(frc_devp->clk_me))
			PR_ERR("%s: frc_devp->clk_me to enable", __func__);
		else
			clk_prepare_enable(frc_devp->clk_me);
		if (IS_ERR(frc_devp->clk_frc))
			PR_ERR("%s: frc_devp->clk_frc to enable", __func__);
		else
			clk_prepare_enable(frc_devp->clk_frc);
		frc_devp->clk_state = FRC_CLOCK_NOR;
		pr_frc(2, "en frc_clk\n");
	}
}

void frc_clk_init(struct frc_dev_s *frc_devp)
{
	int me_clk, mc_clk;
	unsigned int height, width;

	height = frc_devp->out_sts.vout_height;
	width = frc_devp->out_sts.vout_width;
	frc_devp->clk_state = FRC_CLOCK_OFF;
	if (get_chip_type() == ID_T3X) {
		me_clk = FRC_CLOCK_RATE_667;
		mc_clk = FRC_CLOCK_RATE_667;
	} else {
		me_clk = FRC_CLOCK_RATE_333;
		mc_clk = FRC_CLOCK_RATE_667;
	}
	clk_set_rate(frc_devp->clk_frc, mc_clk);
	clk_prepare_enable(frc_devp->clk_frc);
	frc_devp->clk_frc_frq = clk_get_rate(frc_devp->clk_frc);
	pr_frc(0, "clk_frc frq : %d Mhz\n",
			frc_devp->clk_frc_frq / 1000000);
	clk_set_rate(frc_devp->clk_me, me_clk);
	clk_prepare_enable(frc_devp->clk_me);
	frc_devp->clk_me_frq = clk_get_rate(frc_devp->clk_me);
	pr_frc(0, "clk_me frq : %d Mhz\n", frc_devp->clk_me_frq / 1000000);
	frc_devp->clk_state = FRC_CLOCK_NOR;
}

void frc_osdbit_setfalsecolor(struct frc_dev_s *devp, u32 falsecolor)
{
	enum chip_id chip;
	u32 tmp_reg1;

	chip = get_chip_type();

	if (chip == ID_T3) {
		frc_config_reg_value((falsecolor << 19), 0x80000, &regdata_logodbg_0142);
		WRITE_FRC_REG_BY_CPU(FRC_LOGO_DEBUG, regdata_logodbg_0142);
	} else if (chip == ID_T5M || chip == ID_T3X) {
		tmp_reg1 = READ_FRC_REG(FRC_MC_LBUF_LOGO_CTRL);
		regdata_logodbg_0142 = READ_FRC_REG(FRC_LOGO_DEBUG);
		if (falsecolor == 1) {
			tmp_reg1 |= (BIT_8 + BIT_7);
			regdata_logodbg_0142 |= BIT_19;
		} else {
			tmp_reg1 &= ~(BIT_8 + BIT_7);
			regdata_logodbg_0142 &= ~BIT_19;
		}
		WRITE_FRC_REG_BY_CPU(FRC_MC_LBUF_LOGO_CTRL, tmp_reg1);
		WRITE_FRC_REG_BY_CPU(FRC_LOGO_DEBUG, regdata_logodbg_0142);
	}
}

void frc_init_config(struct frc_dev_s *devp)
{
	enum chip_id chip;
	struct frc_fw_data_s *fw_data;
	struct frc_top_type_s *frc_top;

	if (!devp)
		return;
	fw_data = (struct frc_fw_data_s *)devp->fw_data;
	frc_top = &fw_data->frc_top_type;
	frc_load_reg_table(devp, 0);
	frc_set_val_from_reg();
	chip = get_chip_type();

	if (chip == ID_T3)
		frc_top->frc_fb_num = FRC_BUF_NUM_T3;
	else if (chip == ID_T5M)
		frc_top->frc_fb_num = FRC_BUF_NUM_T5M;
	else if (chip == ID_T3X)
		frc_top->frc_fb_num = FRC_BUF_NUM_T3X;
	else
		frc_top->frc_fb_num = FRC_TOTAL_BUF_NUM;

	frc_set_buf_num(frc_top->frc_fb_num);

	/*1: before postblend, 0: after postblend*/
	if (devp->frc_hw_pos == FRC_POS_AFTER_POSTBLEND) {
		vpu_reg_write_bits(devp->vpu_byp_frc_reg_addr, 0, 4, 1);
		frc_config_reg_value(0x02, 0x02, &regdata_blkscale_012c);
		WRITE_FRC_REG_BY_CPU(FRC_REG_BLK_SCALE, regdata_blkscale_012c);
		frc_config_reg_value(0x800000, 0xF00000, &regdata_iplogo_en_0503);
		WRITE_FRC_REG_BY_CPU(FRC_IPLOGO_EN, regdata_iplogo_en_0503);
	} else {
		vpu_reg_write_bits(devp->vpu_byp_frc_reg_addr, 1, 4, 1);
		frc_config_reg_value(0x0, 0x02, &regdata_blkscale_012c);
		WRITE_FRC_REG_BY_CPU(FRC_REG_BLK_SCALE, regdata_blkscale_012c);
		frc_config_reg_value(0x0, 0xF00000, &regdata_iplogo_en_0503);
		WRITE_FRC_REG_BY_CPU(FRC_IPLOGO_EN, regdata_iplogo_en_0503);
		frc_config_reg_value(0x0, 0x80000, &regdata_logodbg_0142);
		WRITE_FRC_REG_BY_CPU(FRC_LOGO_DEBUG, regdata_logodbg_0142);
	}
}

void frc_reset(u32 onoff)
{
	if (onoff) {
		WRITE_FRC_REG_BY_CPU(FRC_AUTO_RST_SEL, 0x3c);
		WRITE_FRC_REG_BY_CPU(FRC_SYNC_SW_RESETS, 0x3c);
	} else {
		WRITE_FRC_REG_BY_CPU(FRC_SYNC_SW_RESETS, 0x0);
		WRITE_FRC_REG_BY_CPU(FRC_AUTO_RST_SEL, 0x0);
	}
}

void frc_mc_reset(u32 onoff)
{
	if (onoff) {
		WRITE_FRC_REG_BY_CPU(FRC_MC_SW_RESETS, 0xffff);
		WRITE_FRC_REG_BY_CPU(FRC_MEVP_SW_RESETS, 0xffffff);
	} else {
		WRITE_FRC_REG_BY_CPU(FRC_MEVP_SW_RESETS, 0x0);
		WRITE_FRC_REG_BY_CPU(FRC_MC_SW_RESETS, 0x0);
	}
	pr_frc(2, "%s %d\n", __func__, onoff);
}

void set_frc_enable(u32 en)
{
	enum chip_id chip = get_chip_type();
	struct frc_dev_s *devp = get_frc_devp();

	pr_frc(2, "%s set(1122) %d\n", __func__, en);

	regdata_topctl_3f01 = READ_FRC_REG(FRC_TOP_CTRL);
	frc_config_reg_value(en, BIT_0, &regdata_topctl_3f01);
	WRITE_FRC_REG_BY_CPU(FRC_TOP_CTRL, regdata_topctl_3f01);
	if (en == 1) {
		if (chip == ID_T3X) {
			if (READ_FRC_REG(FRC_REG_TOP_CTRL7) & 0xFFFFFFFF) {
				PR_ERR("CTRL7 error\n");
				regdata_top_ctl_0007 = 0;
				WRITE_FRC_REG_BY_CPU(FRC_REG_TOP_CTRL7, regdata_top_ctl_0007);
			}
			WRITE_FRC_REG_BY_CPU(FRC_AUTO_RST_SEL, 0x3c);
			WRITE_FRC_REG_BY_CPU(FRC_SYNC_SW_RESETS, 0x3c);
			WRITE_FRC_REG_BY_CPU(FRC_SYNC_SW_RESETS, 0);
			WRITE_FRC_REG_BY_CPU(FRC_AUTO_RST_SEL, 0);
			frc_mc_reset(1);
			frc_mc_reset(0);
			WRITE_FRC_REG_BY_CPU(FRC_TOP_SW_RESET, 0xFFFF);
			WRITE_FRC_REG_BY_CPU(FRC_TOP_SW_RESET, 0x0);
		} else {
			frc_mc_reset(1);
			frc_mc_reset(0);
			WRITE_FRC_REG_BY_CPU(FRC_TOP_SW_RESET, 0xFFFF);
			WRITE_FRC_REG_BY_CPU(FRC_TOP_SW_RESET, 0x0);
			if (devp->dbg_mvrd_mode > 0)
				WRITE_FRC_REG_BY_CPU(FRC_MC_MVRD_CTRL, 0x101);
		}
	} else {
//		if ((READ_FRC_REG(FRC_REG_TOP_CTRL7) >> 24) > 0) {
//			frc_config_reg_value(0x0, 0x9000000, &regdata_top_ctl_0007);
//			WRITE_FRC_REG_BY_CPU(FRC_REG_TOP_CTRL7, regdata_top_ctl_0007);
//		}
		regdata_top_ctl_0007 = 0;
		WRITE_FRC_REG_BY_CPU(FRC_REG_TOP_CTRL7, regdata_top_ctl_0007);
		gst_frc_param.s2l_en = 0;
		gst_frc_param.frc_mcfixlines = 0;
		// WRITE_FRC_REG_BY_CPU(FRC_FRAME_SIZE, 0x0);
		if (vlock_sync_frc_vporch(gst_frc_param) < 0)
			pr_frc(1, "frc_off_set maxlnct fail !!!\n");
		else
			pr_frc(1, "frc_off_set maxlnct success!!!\n");
	}
	pr_frc(2, "%s ok\n", __func__);
}

void set_frc_bypass(u32 en)
{
	struct frc_dev_s *devp = get_frc_devp();
	u32 tmp_vpu_top_reg;

	if (get_chip_type() == ID_T3X) {
		devp->need_bypass = en ? 1 : 2;
	} else {
		tmp_vpu_top_reg = vpu_reg_read(VPU_FRC_TOP_CTRL);
		if (en && !(tmp_vpu_top_reg & BIT_0))
			vpu_reg_write(VPU_FRC_TOP_CTRL, (tmp_vpu_top_reg | BIT_0));
		else if (!en && (tmp_vpu_top_reg & BIT_0))
			vpu_reg_write(VPU_FRC_TOP_CTRL, (tmp_vpu_top_reg & 0xfffffffe));
		else
			pr_frc(2, "%s set:%d rd vpu:0x%X\n", __func__, en, tmp_vpu_top_reg);
	}
}

void frc_crc_enable(struct frc_dev_s *frc_devp)
{
	struct frc_crc_data_s *crc_data;
	unsigned int en;

	crc_data = &frc_devp->frc_crc_data;
	en = crc_data->me_wr_crc.crc_en;
	WRITE_FRC_BITS(INP_ME_WRMIF_CTRL, en, 31, 1);

	en = crc_data->me_rd_crc.crc_en;
	WRITE_FRC_BITS(INP_ME_RDMIF_CTRL, en, 31, 1);

	en = crc_data->mc_wr_crc.crc_en;
	WRITE_FRC_BITS(INP_MC_WRMIF_CTRL, en, 31, 1);
}

void frc_set_buf_num(u32 frc_fb_num)
{
	struct frc_fw_data_s *fw_data;
	struct frc_top_type_s *frc_top;
	u32 temp;
	u8  frm_buf_num;
	struct frc_dev_s *frc_devp = get_frc_devp();

	if (!frc_devp)
		return;

	fw_data = (struct frc_fw_data_s *)frc_devp->fw_data;
	frc_top = &fw_data->frc_top_type;

	temp = READ_FRC_REG(FRC_FRAME_BUFFER_NUM);
	frm_buf_num = temp & 0x1F;
	pr_frc(0, "buf num (read: %d, set: %d)\n",
			frm_buf_num, frc_fb_num);
	if (frc_fb_num != frm_buf_num) {
		WRITE_FRC_REG_BY_CPU(FRC_FRAME_BUFFER_NUM,
		frc_fb_num << 8 | frc_fb_num);
		frc_top->frc_fb_num = frc_fb_num;
		temp = READ_FRC_REG(FRC_FRAME_BUFFER_NUM);
		frm_buf_num = temp & 0x1F;
		if (frm_buf_num != frc_top->frc_fb_num)
			pr_frc(1, "set buf num error (set %d, read back %d)\n",
			frc_top->frc_fb_num, frm_buf_num);
	}
	frc_devp->buf.frm_buf_num = frc_fb_num;
	frc_devp->buf.logo_buf_num = frc_fb_num;
}

void frc_check_hw_stats(struct frc_dev_s *frc_devp, u8 checkflag)
{
	u32 val = 0;
	struct  frc_hw_stats_s  *tmpstats;

	tmpstats = &frc_devp->hw_stats;
	if (checkflag == 0) {
		tmpstats = &frc_devp->hw_stats;
		val = READ_FRC_REG(FRC_RO_DBG0_STAT);
		tmpstats->reg4dh.rawdat = val;
		val = READ_FRC_REG(FRC_RO_DBG1_STAT);
		tmpstats->reg4eh.rawdat = val;
		val = READ_FRC_REG(FRC_RO_DBG2_STAT);
		tmpstats->reg4fh.rawdat = val;
	} else if (checkflag == 1) {
		pr_frc(1, "[0x%x] ref_vs_dly_num = %d, ref_de_dly_num = %d\n",
			tmpstats->reg4dh.rawdat,
			tmpstats->reg4dh.bits.ref_vs_dly_num,
			tmpstats->reg4dh.bits.ref_de_dly_num);
		pr_frc(1, "[0x%x] mevp_dly_num = %d, mcout_dly_num = %d\n",
			tmpstats->reg4eh.rawdat,
			tmpstats->reg4eh.bits.mevp_dly_num,
			tmpstats->reg4eh.bits.mcout_dly_num);
		pr_frc(1, "[0x%x] memc_corr_st = %d, corr_dly_num = %d\n",
			tmpstats->reg4fh.rawdat,
			tmpstats->reg4fh.bits.memc_corr_st,
			tmpstats->reg4fh.bits.memc_corr_dly_num);
		pr_frc(1, "out_dly_err=%d, out_de_dly_num = %d\n",
			tmpstats->reg4fh.bits.out_dly_err,
			tmpstats->reg4fh.bits.out_de_dly_num);
	}
}

void frc_me_crc_read(struct frc_dev_s *frc_devp)
{
	struct frc_crc_data_s *crc_data;
	u32 val;

	crc_data = &frc_devp->frc_crc_data;
	if (crc_data->frc_crc_read) {
		val = READ_FRC_REG(INP_ME_WRMIF);
		crc_data->me_wr_crc.crc_done_flag = val & 0x1;
		if (crc_data->me_wr_crc.crc_en)
			crc_data->me_wr_crc.crc_data_cmp[0] = READ_FRC_REG(INP_ME_WRMIF_CRC1);
		else
			crc_data->me_wr_crc.crc_data_cmp[0] = 0;

		val = READ_FRC_REG(INP_ME_RDMIF);
		crc_data->me_rd_crc.crc_done_flag = val & 0x1;

		if (crc_data->me_rd_crc.crc_en) {
			crc_data->me_rd_crc.crc_data_cmp[0] = READ_FRC_REG(INP_ME_RDMIF_CRC1);
			crc_data->me_rd_crc.crc_data_cmp[1] = READ_FRC_REG(INP_ME_RDMIF_CRC2);
		} else {
			crc_data->me_rd_crc.crc_data_cmp[0] = 0;
			crc_data->me_rd_crc.crc_data_cmp[1] = 0;
		}
		if (crc_data->frc_crc_pr) {
			if (crc_data->me_wr_crc.crc_en && crc_data->me_rd_crc.crc_en)
				pr_frc(0,
					"invs_cnt = %d, mewr_done_flag = %d, mewr_cmp1 = 0x%x, merd_done_flag = %d, merd_cmp1 = 0x%x, merd_cmp2 = 0x%x\n",
					frc_devp->in_sts.vs_cnt,
					crc_data->me_wr_crc.crc_done_flag,
					crc_data->me_wr_crc.crc_data_cmp[0],
					crc_data->me_rd_crc.crc_done_flag,
					crc_data->me_rd_crc.crc_data_cmp[0],
					crc_data->me_rd_crc.crc_data_cmp[1]);
			else if (crc_data->me_wr_crc.crc_en)
				pr_frc(0,
					"invs_cnt = %d, mewr_done_flag = %d, mewr_cmp1 = 0x%x\n",
					frc_devp->in_sts.vs_cnt,
					crc_data->me_wr_crc.crc_done_flag,
					crc_data->me_wr_crc.crc_data_cmp[0]);
			else if (crc_data->me_rd_crc.crc_en)
				pr_frc(0,
					"invs_cnt = %d, merd_done_flag = %d, merd_cmp1 = 0x%x, merd_cmp2 = 0x%x\n",
					frc_devp->in_sts.vs_cnt,
					crc_data->me_rd_crc.crc_done_flag,
					crc_data->me_rd_crc.crc_data_cmp[0],
					crc_data->me_rd_crc.crc_data_cmp[1]);
		}
	}
}

void frc_mc_crc_read(struct frc_dev_s *frc_devp)
{
	struct frc_crc_data_s *crc_data;
	u32 val;

	crc_data = &frc_devp->frc_crc_data;
	if (crc_data->frc_crc_read) {
		val = READ_FRC_REG(INP_MC_WRMIF);
		crc_data->mc_wr_crc.crc_done_flag = val & 0x1;
		if (crc_data->mc_wr_crc.crc_en) {
			crc_data->mc_wr_crc.crc_data_cmp[0] = READ_FRC_REG(INP_MC_WRMIF_CRC1);
			crc_data->mc_wr_crc.crc_data_cmp[1] = READ_FRC_REG(INP_MC_WRMIF_CRC2);
		} else {
			crc_data->mc_wr_crc.crc_data_cmp[0] = 0;
			crc_data->mc_wr_crc.crc_data_cmp[1] = 0;
		}
		if (crc_data->frc_crc_pr && crc_data->mc_wr_crc.crc_en)
			pr_frc(0,
			"outvs_cnt = %d, mcwr_done_flag = %d, mcwr_cmp1 = 0x%x, mcwr_cmp2 = 0x%x\n",
				frc_devp->out_sts.vs_cnt,
				crc_data->mc_wr_crc.crc_done_flag,
				crc_data->mc_wr_crc.crc_data_cmp[0],
				crc_data->mc_wr_crc.crc_data_cmp[1]);
	}
}

void inp_undone_read(struct frc_dev_s *frc_devp)
{
	u32 offset = 0x0;
	enum chip_id chip;
	u32 inp_ud_flag, readval, timeout;

	if (!frc_devp)
		return;
	if (!frc_devp->probe_ok || !frc_devp->power_on_flag)
		return;
	if (frc_devp->frc_sts.re_config)
		return;
	//if (frc_devp->frc_sts.state != FRC_STATE_ENABLE ||
	//	frc_devp->frc_sts.new_state != FRC_STATE_ENABLE)
	//	return;

	chip = get_chip_type();

	if (chip == ID_T3)
		offset = 0xa0;
	else if (chip == ID_T5M || chip == ID_T3X)
		offset = 0x0;

	inp_ud_flag = READ_FRC_REG(FRC_INP_UE_DBG + offset) & 0x3f;
	if (inp_ud_flag == 0x10 && (chip == ID_T5M || chip == ID_T3X)) {
		return;
	} else if (inp_ud_flag != 0) {
		WRITE_FRC_REG_BY_CPU(FRC_INP_UE_CLR + offset, 0x3f);
		WRITE_FRC_REG_BY_CPU(FRC_INP_UE_CLR + offset, 0x0);
		frc_devp->ud_dbg.inp_undone_err++;
		frc_devp->frc_sts.inp_undone_cnt++;
		if (frc_devp->ud_dbg.inp_ud_dbg_en != 0) {
			PR_ERR("inp_ud_err=0x%x, isr_cnt=(%d,%d), vd_vs_cnt=%d\n",
				inp_ud_flag,
				frc_devp->in_sts.vs_cnt,
				frc_devp->out_sts.vs_cnt,
				frc_devp->frc_sts.vs_cnt);
		}
		timeout = 0;
		do {
			readval = READ_FRC_REG(FRC_INP_UE_CLR + offset) & 0x3f;
			if (readval == 0)
				break;
		} while (timeout++ < 100);
		if (frc_devp->ud_dbg.res1_time_en == 1) {
			if (frc_devp->ud_dbg.inp_undone_err == 100) {
				frc_devp->frc_sts.re_config = true;
				PR_ERR("inp undo more err, frc will reset\n");
			}
		}
	} else {
		frc_devp->ud_dbg.inp_undone_err = 0;
	}
}

void me_undone_read(struct frc_dev_s *frc_devp)
{
	u32 me_ud_flag;

	if (!frc_devp)
		return;
	if (!frc_devp->probe_ok || !frc_devp->power_on_flag)
		return;
	if (frc_devp->ud_dbg.meud_dbg_en) {
		me_ud_flag = READ_FRC_REG(FRC_MEVP_RO_STAT0) & BIT_0;
		if (me_ud_flag != 0) {
			frc_devp->frc_sts.me_undone_cnt++;
			WRITE_FRC_BITS(FRC_MEVP_CTRL0, 1, 31, 1);
			WRITE_FRC_BITS(FRC_MEVP_CTRL0, 0, 31, 1);
			frc_devp->ud_dbg.me_undone_err = me_ud_flag;
		} else {
			frc_devp->ud_dbg.me_undone_err = 0;
		}
		if (frc_devp->ud_dbg.me_undone_err == 1)
			PR_ERR("me_ud_err_cnt=%d, isr_cnt=(%d,%d), vd_vs_cnt=%d\n",
			frc_devp->frc_sts.me_undone_cnt,
			frc_devp->in_sts.vs_cnt,
			frc_devp->out_sts.vs_cnt,
			frc_devp->frc_sts.vs_cnt);
	}
}

void mc_undone_read(struct frc_dev_s *frc_devp)
{
	u32 val, mc_ud_flag;

	if (!frc_devp)
		return;
	if (!frc_devp->probe_ok || !frc_devp->power_on_flag)
		return;
	if (frc_devp->ud_dbg.mcud_dbg_en) {
		val = READ_FRC_REG(FRC_RO_MC_STAT);
		mc_ud_flag = (val >> 12) & 0x1;
		if (mc_ud_flag != 0) {
			frc_devp->frc_sts.mc_undone_cnt = (val >> 16) & 0xfff;
			frc_devp->ud_dbg.mc_undone_err = 1;
			WRITE_FRC_BITS(FRC_MC_HW_CTRL0, 1, 21, 1);
			WRITE_FRC_BITS(FRC_MC_HW_CTRL0, 0, 21, 1);
		} else {
			frc_devp->ud_dbg.mc_undone_err = 0;
		}
		if (frc_devp->ud_dbg.mc_undone_err == 1)
			PR_ERR("mc_undo_vcnt= %d, isr_cnt=(%d,%d), vd_vs_cnt=%d\n",
				frc_devp->frc_sts.mc_undone_cnt,
				frc_devp->in_sts.vs_cnt,
				frc_devp->out_sts.vs_cnt,
				frc_devp->frc_sts.vs_cnt);
	}
}

void vp_undone_read(struct frc_dev_s *frc_devp)
{
	u32 vp_ud_flag;

	if (!frc_devp)
		return;
	if (!frc_devp->probe_ok || !frc_devp->power_on_flag)
		return;
	if (frc_devp->ud_dbg.vpud_dbg_en) {
		vp_ud_flag = READ_FRC_REG(FRC_VP_TOP_STAT) & 0x03;
		if (vp_ud_flag != 0) {
			frc_devp->frc_sts.vp_undone_cnt++;
			frc_devp->ud_dbg.vp_undone_err = vp_ud_flag;
			WRITE_FRC_BITS(FRC_VP_TOP_CLR_STAT, 3, 0, 2);
			WRITE_FRC_BITS(FRC_VP_TOP_CLR_STAT, 0, 0, 2);
		} else {
			frc_devp->ud_dbg.vp_undone_err = 0;
		}
		if (frc_devp->ud_dbg.vp_undone_err != 0)
			PR_ERR("vp_err=%x, err_cnt=%d, outvs_cnt=%d, vd_vs_cnt:%d\n",
				frc_devp->ud_dbg.vp_undone_err,
				frc_devp->frc_sts.vp_undone_cnt,
				frc_devp->out_sts.vs_cnt,
				frc_devp->frc_sts.vs_cnt);
	}
}

const char * const mtx_str[] = {
	"FRC_INPUT_CSC",
	"FRC_OUTPUT_CSC"
};

const char * const csc_str[] = {
	"CSC_OFF",
	"RGB_YUV709L",
	"RGB_YUV709F",
	"YUV709L_RGB",
	"YUV709F_RGB",
	"PAT_RED",
	"PAT_GREEN",
	"PAT_BLUE",
	"PAT_WHITE",
	"PAT_BLACK",
	"RETORE_DEF",
};

void frc_mtx_cfg(enum frc_mtx_e mtx_sel, enum frc_mtx_csc_e mtx_csc)
{
	unsigned int pre_offset01 = 0;
	unsigned int pre_offset2 = 0;
	unsigned int pst_offset01 = 0x0200, pst_offset2 = 0x0200;
	unsigned int coef00_01 = 0, coef02_10 = 0, coef11_12 = 0;
	unsigned int coef20_21 = 0, coef22 = 0;
	unsigned int en = 1;
	struct frc_dev_s *devp = get_frc_devp();

	if (!devp->probe_ok || !devp->power_on_flag)
		return;
	if (devp->clk_state == FRC_CLOCK_OFF)
		return;
	switch (mtx_csc) {
	case CSC_OFF:
		en = 0;
		break;
	case RGB_YUV709L:
		coef00_01 = 0x00bb0275;
		coef02_10 = 0x003f1f99;
		coef11_12 = 0x1ea601c2;
		coef20_21 = 0x01c21e67;
		coef22 = 0x00001fd7;
		pre_offset01 = 0x0;
		pre_offset2 = 0x0;
		pst_offset01 = 0x00400200;
		pst_offset2 = 0x00000200;
		break;
	case RGB_YUV709F:
		coef00_01 = 0xda02dc;
		coef02_10 = 0x4a1f8b;
		coef11_12 = 0x1e750200;
		coef20_21 = 0x2001e2f;
		coef22 = 0x1fd0;
		pre_offset01 = 0x0;
		pre_offset2 = 0x0;
		pst_offset01 = 0x200;
		pst_offset2 = 0x200;
		break;
	case YUV709L_RGB:
		coef00_01 = 0x04A80000;
		coef02_10 = 0x072C04A8;
		coef11_12 = 0x1F261DDD;
		coef20_21 = 0x04A80876;
		coef22 = 0x0;
		pre_offset01 = 0x1fc01e00;
		pre_offset2 = 0x000001e00;
		pst_offset01 = 0x0;
		pst_offset2 = 0x0;
		break;
	case YUV709F_RGB:
		coef00_01 = 0x04000000;
		coef02_10 = 0x064D0400;
		coef11_12 = 0x1F411E21;
		coef20_21 = 0x0400076D;
		coef22 = 0x0;
		pre_offset01 = 0x000001e00;
		pre_offset2 = 0x000001e00;
		pst_offset01 = 0x0;
		pst_offset2 = 0x0;
		break;
	case PAT_RD:
		coef00_01 = 0x0;
		coef02_10 = 0x0;
		coef11_12 = 0x0;
		coef20_21 = 0x0;
		coef22 = 0x0;
		pst_offset01 = 0x0;
		pst_offset2 = 0xFFF;
		break;
	case PAT_GR:
		coef00_01 = 0x0;
		coef02_10 = 0x0;
		coef11_12 = 0x0;
		coef20_21 = 0x0;
		coef22 = 0x0;
		pst_offset01 = 0x0FFF0000;
		pst_offset2 = 0x0;
		break;
	case PAT_BU:
		coef00_01 = 0x0;
		coef02_10 = 0x0;
		coef11_12 = 0x0;
		coef20_21 = 0x0;
		coef22 = 0x0;
		pst_offset01 = 0x0FFF;
		pst_offset2 = 0x0;
		break;
	case PAT_WT:
		coef00_01 = 0x0;
		coef02_10 = 0x0;
		coef11_12 = 0x0;
		coef20_21 = 0x0;
		coef22 = 0x0;
		pst_offset01 = 0x0FFF0FFF;
		pst_offset2 = 0x0200;
		break;
	case PAT_BK:
		coef00_01 = 0x0;
		coef02_10 = 0x0;
		coef11_12 = 0x0;
		coef20_21 = 0x0;
		coef22 = 0x0;
		pst_offset01 = 0x0200;
		pst_offset2 = 0x0200;
		break;
	case DEFAULT:
		if (mtx_sel == FRC_INPUT_CSC) {
			coef00_01 = devp->init_csc[0].coef00_01;
			coef02_10 = devp->init_csc[0].coef02_10;
			coef11_12 = devp->init_csc[0].coef11_12;
			coef20_21 = devp->init_csc[0].coef20_21;
			coef22 = devp->init_csc[0].coef22;
			pre_offset01 = devp->init_csc[0].pre_offset01;
			pre_offset2 = devp->init_csc[0].pre_offset2;
			pst_offset01 = devp->init_csc[0].pst_offset01;
			pst_offset2 = devp->init_csc[0].pst_offset2;
			en = devp->init_csc[0].enable;
		} else {
			coef00_01 = devp->init_csc[1].coef00_01;
			coef02_10 = devp->init_csc[1].coef02_10;
			coef11_12 = devp->init_csc[1].coef11_12;
			coef20_21 = devp->init_csc[1].coef20_21;
			coef22 = devp->init_csc[1].coef22;
			pre_offset01 = devp->init_csc[1].pre_offset01;
			pre_offset2 = devp->init_csc[1].pre_offset2;
			pst_offset01 = devp->init_csc[1].pst_offset01;
			pst_offset2 = devp->init_csc[1].pst_offset2;
			en = devp->init_csc[1].enable;
		}
		break;
	default:
		coef00_01 = 0x0;
		coef02_10 = 0x0;
		coef11_12 = 0x0;
		coef20_21 = 0x0;
		coef22 = 0x0;
		pst_offset01 = 0x0200;
		pst_offset2 = 0x0200;
		en = 0;  // invalid case
		break;
	}

	switch (mtx_sel) {
	case FRC_INPUT_CSC:
		WRITE_FRC_REG(FRC_INP_CSC_OFFSET_INP01, pre_offset01);
		WRITE_FRC_REG(FRC_INP_CSC_OFFSET_INP2, pre_offset2);
		WRITE_FRC_REG(FRC_INP_CSC_COEF_00_01, coef00_01);
		WRITE_FRC_REG(FRC_INP_CSC_COEF_02_10, coef02_10);
		WRITE_FRC_REG(FRC_INP_CSC_COEF_11_12, coef11_12);
		WRITE_FRC_REG(FRC_INP_CSC_COEF_20_21, coef20_21);
		WRITE_FRC_REG(FRC_INP_CSC_COEF_22, coef22);
		WRITE_FRC_REG(FRC_INP_CSC_OFFSET_OUP01, pst_offset01);
		WRITE_FRC_REG(FRC_INP_CSC_OFFSET_OUP2, pst_offset2);
		WRITE_FRC_BITS(FRC_INP_CSC_CTRL, en, 3, 1);
		break;
	case FRC_OUTPUT_CSC:
		WRITE_FRC_REG(FRC_MC_CSC_OFFSET_INP01, pre_offset01);
		WRITE_FRC_REG(FRC_MC_CSC_OFFSET_INP2, pre_offset2);
		WRITE_FRC_REG(FRC_MC_CSC_COEF_00_01, coef00_01);
		WRITE_FRC_REG(FRC_MC_CSC_COEF_02_10, coef02_10);
		WRITE_FRC_REG(FRC_MC_CSC_COEF_11_12, coef11_12);
		WRITE_FRC_REG(FRC_MC_CSC_COEF_20_21, coef20_21);
		WRITE_FRC_REG(FRC_MC_CSC_COEF_22, coef22);
		WRITE_FRC_REG(FRC_MC_CSC_OFFSET_OUP01, pst_offset01);
		WRITE_FRC_REG(FRC_MC_CSC_OFFSET_OUP2, pst_offset2);
		WRITE_FRC_BITS(FRC_MC_CSC_CTRL, en, 3, 1);
		break;
	default:
		break;
	}
	pr_frc(1, "%s, mtx sel: %s, en:%d, mtx csc : %s\n",
		__func__, mtx_str[mtx_sel - 1], en, csc_str[mtx_csc]);
}

void frc_get_init_csc(struct frc_dev_s *frc_devp)
{
	frc_devp->init_csc[0].coef00_01 = READ_FRC_REG(FRC_INP_CSC_COEF_00_01);
	frc_devp->init_csc[0].coef02_10 = READ_FRC_REG(FRC_INP_CSC_COEF_02_10);
	frc_devp->init_csc[0].coef11_12 = READ_FRC_REG(FRC_INP_CSC_COEF_11_12);
	frc_devp->init_csc[0].coef20_21 = READ_FRC_REG(FRC_INP_CSC_COEF_20_21);
	frc_devp->init_csc[0].coef22 = READ_FRC_REG(FRC_INP_CSC_COEF_22);
	frc_devp->init_csc[0].pre_offset01 = READ_FRC_REG(FRC_INP_CSC_OFFSET_INP01);
	frc_devp->init_csc[0].pre_offset2 = READ_FRC_REG(FRC_INP_CSC_OFFSET_INP2);
	frc_devp->init_csc[0].pst_offset01 = READ_FRC_REG(FRC_INP_CSC_OFFSET_OUP01);
	frc_devp->init_csc[0].pst_offset2 = READ_FRC_REG(FRC_INP_CSC_OFFSET_OUP2);
	frc_devp->init_csc[0].enable =
			(READ_FRC_REG(FRC_INP_CSC_CTRL) >> 3) & BIT_0;

	frc_devp->init_csc[1].coef00_01 = READ_FRC_REG(FRC_MC_CSC_COEF_00_01);
	frc_devp->init_csc[1].coef02_10 = READ_FRC_REG(FRC_MC_CSC_COEF_02_10);
	frc_devp->init_csc[1].coef11_12 = READ_FRC_REG(FRC_MC_CSC_COEF_11_12);
	frc_devp->init_csc[1].coef20_21 = READ_FRC_REG(FRC_MC_CSC_COEF_20_21);
	frc_devp->init_csc[1].coef22 = READ_FRC_REG(FRC_MC_CSC_COEF_22);
	frc_devp->init_csc[1].pre_offset01 = READ_FRC_REG(FRC_MC_CSC_OFFSET_INP01);
	frc_devp->init_csc[1].pre_offset2 = READ_FRC_REG(FRC_MC_CSC_OFFSET_INP2);
	frc_devp->init_csc[1].pst_offset01 = READ_FRC_REG(FRC_MC_CSC_OFFSET_OUP01);
	frc_devp->init_csc[1].pst_offset2 = READ_FRC_REG(FRC_MC_CSC_OFFSET_OUP2);
	frc_devp->init_csc[1].enable =
			(READ_FRC_REG(FRC_MC_CSC_CTRL) >> 3) & BIT_0;

	pr_frc(1, "%s, inp_csc_en:%d mtx(%x-%x-%x), mc_csc_en:%d mtx(%x-%x-%x)\n",
		__func__,  frc_devp->init_csc[0].enable,
		frc_devp->init_csc[0].coef00_01,
		frc_devp->init_csc[0].pre_offset01,
		frc_devp->init_csc[0].pre_offset01,
		frc_devp->init_csc[1].enable,
		frc_devp->init_csc[1].coef00_01,
		frc_devp->init_csc[1].pre_offset01,
		frc_devp->init_csc[1].pre_offset01);
}

void frc_mtx_set(struct frc_dev_s *frc_devp)
{
	if (frc_devp->frc_hw_pos == FRC_POS_AFTER_POSTBLEND) {
		frc_mtx_cfg(FRC_INPUT_CSC, RGB_YUV709F);
		frc_mtx_cfg(FRC_OUTPUT_CSC, YUV709F_RGB);
		return;
	}

	if (frc_devp->frc_test_ptn) {
		frc_mtx_cfg(FRC_INPUT_CSC, RGB_YUV709L);
		frc_mtx_cfg(FRC_OUTPUT_CSC, CSC_OFF);
	} else {
		frc_mtx_cfg(FRC_INPUT_CSC, CSC_OFF);
		frc_mtx_cfg(FRC_OUTPUT_CSC, CSC_OFF);
	}
	frc_get_init_csc(frc_devp);
}

static void set_vd1_out_size(struct frc_dev_s *frc_devp)
{
	unsigned int hsize = 0;
	unsigned int vsize = 0;
	enum chip_id chip;

	chip = get_chip_type();
	if (frc_devp->frc_hw_pos == FRC_POS_BEFORE_POSTBLEND) {
		if (frc_devp->force_size.force_en) {
			hsize = frc_devp->force_size.force_hsize - 1;
			vsize = frc_devp->force_size.force_vsize - 1;
			if (chip == ID_T3) {
				vpu_reg_write_bits(VD1_BLEND_SRC_CTRL_T3X, 1, 8, 4);
				vpu_reg_write_bits(VPP_POSTBLEND_VD1_H_START_END_T3X, hsize, 0, 13);
				vpu_reg_write_bits(VPP_POSTBLEND_VD1_V_START_END_T3X, vsize, 0, 13);
			} else {
				vpu_reg_write_bits(VD1_BLEND_SRC_CTRL, 1, 8, 4);
				vpu_reg_write_bits(VPP_POSTBLEND_VD1_H_START_END, hsize, 0, 13);
				vpu_reg_write_bits(VPP_POSTBLEND_VD1_V_START_END, vsize, 0, 13);
			}
		}
	}
	pr_frc(1, "hsize = %d, vsize = %d\n", hsize, vsize);
}

void frc_pattern_on(u32 en)
{
	u32 offset = 0x0;
	enum chip_id chip;

	chip = get_chip_type();

	if (chip == ID_T3)
		offset = 0xa0;
	else if (chip == ID_T5M || chip == ID_T3X)
		offset = 0x0;

	// WRITE_FRC_BITS(FRC_REG_INP_MODULE_EN, en, 6, 1);
	frc_config_reg_value(en << 6, 0x40, &regdata_inpmoden_04f9);
	WRITE_FRC_REG_BY_CPU(FRC_REG_INP_MODULE_EN + offset, regdata_inpmoden_04f9);

	if (en == 1) {
		frc_mtx_cfg(FRC_INPUT_CSC, RGB_YUV709L);
		frc_mtx_cfg(FRC_OUTPUT_CSC, CSC_OFF);

	} else {
		frc_mtx_cfg(FRC_INPUT_CSC, DEFAULT);
		frc_mtx_cfg(FRC_OUTPUT_CSC, DEFAULT);
	}
}

void frc_input_init(struct frc_dev_s *frc_devp,
	struct frc_top_type_s *frc_top)
{
	if (frc_devp->frc_test_ptn) {
		if (frc_devp->frc_hw_pos == FRC_POS_BEFORE_POSTBLEND &&
			frc_devp->force_size.force_en) {
			/*used for set testpattern size, only for debug*/
			frc_top->hsize = frc_devp->force_size.force_hsize;
			frc_top->vsize = frc_devp->force_size.force_vsize;
			frc_top->out_hsize = frc_devp->force_size.force_hsize;
			frc_top->out_vsize = frc_devp->force_size.force_vsize;
		} else {
			frc_top->hsize = frc_devp->out_sts.vout_width;
			frc_top->vsize = frc_devp->out_sts.vout_height;
			frc_top->out_hsize = frc_devp->out_sts.vout_width;
			frc_top->out_vsize = frc_devp->out_sts.vout_height;
		}
		frc_top->frc_ratio_mode = frc_devp->in_out_ratio;
		frc_top->film_mode = frc_devp->film_mode;
		/*hw film detect*/
		frc_top->film_hwfw_sel = frc_devp->film_mode_det;
		frc_top->frc_prot_mode = frc_devp->prot_mode;

		set_vd1_out_size(frc_devp);
	} else {
		if (frc_devp->frc_hw_pos == FRC_POS_AFTER_POSTBLEND) {
			frc_top->hsize = frc_devp->out_sts.vout_width;
			frc_top->vsize = frc_devp->out_sts.vout_height;
		} else {
			frc_top->hsize = frc_devp->in_sts.in_hsize;
			frc_top->vsize = frc_devp->in_sts.in_vsize;
		}
		frc_top->out_hsize = frc_devp->out_sts.vout_width;
		frc_top->out_vsize = frc_devp->out_sts.vout_height;
		frc_top->frc_ratio_mode = frc_devp->in_out_ratio;
		frc_top->film_mode = frc_devp->film_mode;
		/*sw film detect*/
		frc_top->film_hwfw_sel = frc_devp->film_mode_det;
		frc_top->frc_prot_mode = frc_devp->prot_mode;
	}

	if (frc_devp->out_sts.vout_width > frc_devp->buf.in_hsize ||
		frc_devp->out_sts.vout_height > frc_devp->buf.in_vsize)
		PR_ERR("FRC initial buf is not enough!\n");

	if (frc_top->out_vsize == HEIGHT_2K &&
			frc_top->out_hsize == WIDTH_4K)
		frc_top->panel_res = FRC_RES_4K2K;
	else if (frc_top->out_vsize == HEIGHT_1K &&
			frc_top->out_hsize == WIDTH_2K)
		frc_top->panel_res = FRC_RES_2K1K;
	else if (frc_top->out_vsize == HEIGHT_1K &&
			frc_top->out_hsize == WIDTH_4K)
		frc_top->panel_res = FRC_RES_4K1K;
	else
		frc_top->panel_res = FRC_RES_NONE;

	pr_frc(2, "top in: hsize:%d, vsize:%d\n", frc_top->hsize, frc_top->vsize);
	pr_frc(2, "top out: hsize:%d, vsize:%d\n", frc_top->out_hsize, frc_top->out_vsize);
	if (frc_top->hsize == 0 || frc_top->vsize == 0)
		pr_frc(0, "err: size in hsize:%d, vsize:%d !!!!\n", frc_top->hsize, frc_top->vsize);
}

void frc_drv_bbd_init_xyxy(struct frc_fw_data_s *pfw_data)
{
	u32     reg_data;
	u32     me_width;
	u32     me_height;
	u32     me_blk_xsize;
	u32     me_blk_ysize;
	u32     me_dsx;
	u32     me_dsy;
	u32     me_blksize_dsx;
	u32     me_blksize_dsy;
	u32     hsize_align, vsize_align;
	struct frc_top_type_s *gst_frc_top_item;

	if (!pfw_data)
		return;

	gst_frc_top_item = &pfw_data->frc_top_type;
	// regdata_hme_scale_012d = READ_FRC_REG(FRC_REG_ME_HME_SCALE);
	reg_data = regdata_hme_scale_012d;
	me_dsx  = (reg_data >> 6) & 0x3;
	me_dsy  = (reg_data >> 4) & 0x3;
	// regdata_blksizexy_012b = READ_FRC_REG(FRC_REG_BLK_SIZE_XY);
	reg_data = regdata_blksizexy_012b;
	me_blksize_dsx = (reg_data >> 19) & 0x7;
	me_blksize_dsy = (reg_data >> 16) & 0x7;

	//me  me_blksize_dsx me_blksize_dsy-> FRC_REG_BLK_SIZE_XY
	if (gst_frc_top_item->is_me1mc4 == 0) {
		hsize_align = (gst_frc_top_item->hsize + 7) / 8 * 8;
		vsize_align = (gst_frc_top_item->vsize + 7) / 8 * 8;
	} else {
		hsize_align = (gst_frc_top_item->hsize + 15) / 16 * 16;
		vsize_align = (gst_frc_top_item->vsize + 15) / 16 * 16;
	}
	me_width      = (hsize_align + (1 << me_dsx) - 1) / (1 << me_dsx);/*960*/
	me_height     = (vsize_align + (1 << me_dsy) - 1) / (1 << me_dsy);/*540*/
	me_blk_xsize  = (me_width + (1 << me_blksize_dsx) - 1) / (1 << me_blksize_dsx);/*240*/
	me_blk_ysize  = (me_height + (1 << me_blksize_dsy) - 1) / (1 << me_blksize_dsy);/*135*/

	reg_data = me_height - 1;
	reg_data |= (me_width - 1) << 16;
	frc_config_reg_value(reg_data, 0xfff0fff, &regdata_me_bbpixed_1108);
	WRITE_FRC_REG_BY_CPU(FRC_ME_BB_PIX_ED, regdata_me_bbpixed_1108);

	reg_data = (me_blk_ysize - 1);
	reg_data |= (me_blk_xsize - 1) << 16;
	frc_config_reg_value(reg_data, 0xfff0fff, &regdata_me_bbblked_110a);
	WRITE_FRC_REG_BY_CPU(FRC_ME_BB_BLK_ED, regdata_me_bbblked_110a);// 240*135

	/*reg_me_stat_region_hend 0*/
	frc_config_reg_value(0, 0x3ff, &regdata_me_stat12rhst_110b);
	WRITE_FRC_REG_BY_CPU(FRC_ME_STAT_12R_HST, regdata_me_stat12rhst_110b);

	reg_data = (me_blk_xsize * 2 / 4);
	reg_data |= me_blk_xsize * 1 / 4 << 16;
	frc_config_reg_value(reg_data, 0x3ff03ff, &regdata_me_stat12rh_110c);
	WRITE_FRC_REG_BY_CPU(FRC_ME_STAT_12R_H01, regdata_me_stat12rh_110c);
	reg_data = (me_blk_xsize * 4 / 4 - 1);
	reg_data |= (me_blk_xsize * 3 / 4) << 16;
	frc_config_reg_value(reg_data, 0xfff0fff, &regdata_me_stat12rh_110d);
	WRITE_FRC_REG_BY_CPU(FRC_ME_STAT_12R_H23, regdata_me_stat12rh_110d);

	/*reg_me_stat_region_vend[0]: 45*/
	/*reg_me_stat_region_vend[1]: 90*/
	/*reg_me_stat_region_vend[2]: 135*/
	frc_config_reg_value(me_blk_ysize * 1 / 3, 0x3ff03ff, &regdata_me_stat12rv_110e);
	frc_config_reg_value((me_blk_ysize * 2 / 3) << 16, 0x3ff0000, &regdata_me_stat12rv_110f);
	frc_config_reg_value((me_blk_ysize * 3 / 3 - 1), 0x3ff, &regdata_me_stat12rv_110f);
	WRITE_FRC_REG_BY_CPU(FRC_ME_STAT_12R_V0, regdata_me_stat12rv_110e);
	WRITE_FRC_REG_BY_CPU(FRC_ME_STAT_12R_V1, regdata_me_stat12rv_110f);

	//vp
	/*reg_vp_bb_xyxy[2] 240*/
	/*reg_vp_bb_xyxy[3] 135*/
	frc_config_reg_value((me_blk_ysize - 1) << 16, 0xffff0000, &regdata_vpbb2_1e04);
	frc_config_reg_value((me_blk_xsize - 1), 0xffff, &regdata_vpbb2_1e04);
	WRITE_FRC_REG_BY_CPU(FRC_VP_BB_1, 0x0);
	WRITE_FRC_REG_BY_CPU(FRC_VP_BB_2, regdata_vpbb2_1e04);

	/*reg_vp_me_bb_blk_xyxy[2] 240*/
	/*reg_vp_me_bb_blk_xyxy[3] 135*/
	frc_config_reg_value((me_blk_ysize - 1) << 16, 0xffff0000, &regdata_vpmebb2_1e06);
	frc_config_reg_value((me_blk_xsize - 1), 0xffff, &regdata_vpmebb2_1e06);
	WRITE_FRC_REG_BY_CPU(FRC_VP_ME_BB_1, 0x0);
	WRITE_FRC_REG_BY_CPU(FRC_VP_ME_BB_2, regdata_vpmebb2_1e06);

	frc_config_reg_value((me_blk_xsize * 1 / 4) << 8, 0xfffff, &regdata_vp_win1_1e58);
	frc_config_reg_value((me_blk_xsize * 2 / 4) << 20, 0xfff00000, &regdata_vp_win1_1e58);
	frc_config_reg_value((me_blk_xsize * 3 / 4) << 12, 0xfffff000, &regdata_vp_win2_1e59);
	frc_config_reg_value((me_blk_xsize * 4 / 4 - 1), 0xfff, &regdata_vp_win2_1e59);
	frc_config_reg_value((me_blk_ysize * 1 / 3), 0x0ff, &regdata_vp_win3_1e5a);
	frc_config_reg_value((me_blk_ysize * 2 / 3) << 8, 0xfff00, &regdata_vp_win3_1e5a);
	frc_config_reg_value((me_blk_ysize * 3 / 3 - 1) << 20, 0xfff00000, &regdata_vp_win3_1e5a);
	WRITE_FRC_REG_BY_CPU(FRC_VP_REGION_WINDOW_1, regdata_vp_win1_1e58);
	WRITE_FRC_REG_BY_CPU(FRC_VP_REGION_WINDOW_2, regdata_vp_win2_1e59);
	WRITE_FRC_REG_BY_CPU(FRC_VP_REGION_WINDOW_3, regdata_vp_win3_1e5a);

	//mc:
	WRITE_FRC_REG_BY_CPU(FRC_MC_BB_HANDLE_ME_BLK_BB_XYXY_RIT_AND_BOT,
			((me_blk_xsize - 1) << 16) | (me_blk_ysize - 1));

	//melogo:
	/*reg_melogo_bb_blk_xyxy[2] 240*/
	/*reg_melogo_bb_blk_xyxy[3] 135*/
	WRITE_FRC_REG_BY_CPU(FRC_MELOGO_BB_BLK_ST, 0x0);
	WRITE_FRC_REG_BY_CPU(FRC_MELOGO_BB_BLK_ED,
			((me_blk_xsize - 1) << 16) | (me_blk_ysize - 1));

	/*reg_melogo_stat_region_hend[0] 60*/
	/*reg_melogo_stat_region_hend[1] 120*/
	/*reg_melogo_stat_region_hend[2] 180*/
	/*reg_melogo_stat_region_hend[3] 240*/
	reg_data = me_blk_xsize * 1 / 4;
	reg_data |= (me_blk_xsize * 2 / 4) << 16;
	WRITE_FRC_REG_BY_CPU(FRC_MELOGO_REGION_HWINDOW_0, 0x0);
	WRITE_FRC_REG_BY_CPU(FRC_MELOGO_REGION_HWINDOW_1, reg_data);
	reg_data = me_blk_xsize * 3 / 4;
	reg_data |= (me_blk_xsize * 4 / 4 - 1) << 16;
	WRITE_FRC_REG_BY_CPU(FRC_MELOGO_REGION_HWINDOW_2, reg_data);

	/*reg_melogo_stat_region_vend[0] 45*/
	/*reg_melogo_stat_region_vend[1] 90*/
	/*reg_melogo_stat_region_vend[2] 135*/
	reg_data = me_blk_ysize * 1 / 3 << 16;
	WRITE_FRC_REG_BY_CPU(FRC_MELOGO_REGION_VWINDOW_0, reg_data);
	reg_data = (me_blk_ysize * 2 / 3);
	reg_data |= (me_blk_ysize * 3 / 3 - 1) << 16;
	WRITE_FRC_REG_BY_CPU(FRC_MELOGO_REGION_VWINDOW_1, reg_data);

	//IPLOGO
	/*reg_iplogo_bb_xyxy[2] 960*/
	/*reg_iplogo_bb_xyxy[3] 540*/
	WRITE_FRC_REG_BY_CPU(FRC_IPLOGO_BB_PIX_ST, 0x0);
	WRITE_FRC_REG_BY_CPU(FRC_IPLOGO_BB_PIX_ED,
			((me_width - 1) << 16) | (me_height - 1));
	/*reg_iplogo_stat_region_hend[0] 240*/
	/*reg_iplogo_stat_region_hend[1] 480*/
	/*reg_iplogo_stat_region_hend[2] 720*/
	/*reg_iplogo_stat_region_hend[3] 960*/
	reg_data = me_blk_xsize * 1 / 4;
	reg_data |= (me_blk_xsize * 2 / 4) << 16;
	WRITE_FRC_REG_BY_CPU(FRC_IPLOGO_REGION_HWINDOW_0, 0x0);
	WRITE_FRC_REG_BY_CPU(FRC_IPLOGO_REGION_HWINDOW_1, reg_data);
	reg_data = me_blk_xsize * 3 / 4;
	reg_data |= (me_blk_xsize * 4 / 4 - 1) << 16;
	WRITE_FRC_REG_BY_CPU(FRC_IPLOGO_REGION_HWINDOW_2, reg_data);
	/*reg_iplogo_stat_region_vend[0] 180*/
	/*reg_iplogo_stat_region_vend[1] 360*/
	/*reg_iplogo_stat_region_vend[2] 540*/
	reg_data = me_blk_ysize * 1 / 3 << 16;
	WRITE_FRC_REG_BY_CPU(FRC_IPLOGO_REGION_VWINDOW_0, reg_data);
	reg_data = (me_blk_ysize * 2 / 3);
	reg_data |= (me_blk_ysize * 3 / 3 - 1) << 16;
	WRITE_FRC_REG_BY_CPU(FRC_IPLOGO_REGION_VWINDOW_1, reg_data);

	WRITE_FRC_REG_BY_CPU(FRC_LOGO_BB_RIT_BOT,
		gst_frc_top_item->vsize | (gst_frc_top_item->hsize << 16));
}

void frc_drv_fw_param_init(u32 frm_hsize, u32 frm_vsize, u32 is_me1mc4)
{
	u32 reg_me_mvx_div_mode;
	u32 reg_me_mvy_div_mode;
	u32 reg_demo_window1_en;
	u32 reg_demo_window2_en;
	u32 reg_demo_window3_en;
	u32 reg_demo_window4_en;
	u32 mc_org_me_bb_2;
	u32 mc_org_me_bb_3;
	u32 mc_org_me_blk_bb_2;
	u32 mc_org_me_blk_bb_3;
	u32 reg_me_dsx_scale;
	u32 reg_me_dsy_scale;
	u32 reg_mc_blksize_xscale;
	u32 reg_mc_blksize_yscale;
	u32 reg_mc_mvx_scale;
	u32 reg_mc_mvy_scale;
	u32 reg_mc_fetch_size;
	u32 reg_mc_blk_x;
	u32 reg_data;
	// u32 reg_data1;
	u32 reg_bb_det_top;
	u32 reg_bb_det_bot;
	u32 reg_bb_det_lft;
	u32 reg_bb_det_rit;
	u32 reg_bb_det_motion_top;
	u32 reg_bb_det_motion_bot;
	u32 reg_bb_det_motion_lft;
	u32 reg_bb_det_motion_rit;
	u32 bb_mo_ds_2;
	u32 bb_mo_ds_3;
	u32 bb_oob_v_2;
	u32 tmp_reg_value, tmp_reg_value_2;
	u32 me_mc_x_ratio;
	u32 me_mc_y_ratio;
	u32 logo_mc_x_ratio;
	u32 logo_mc_y_ratio;
	u32 osd_mc_x_ratio;
	u32 osd_mc_y_ratio;
	u32 regdata_bb_xy_st_0119;	 // FRC_REG_BLACKBAR_XYXY_ST 0x0119
	u32 regdata_bb_xy_ed_011a;	 // FRC_REG_BLACKBAR_XYXY_ED	0x011a
	u32 regdata_bb_xy_me_st_011b;	 // FRC_REG_BLACKBAR_XYXY_ME_ST  0x011b
	u32 regdata_bb_xy_me_ed_011c;	// FRC_REG_BLACKBAR_XYXY_ME_ED	0x011c

	if (is_me1mc4 == 0) {
		reg_me_dsx_scale = 1;
		reg_me_dsy_scale = 1;
		me_mc_x_ratio = 1;
		me_mc_y_ratio = 1;
		logo_mc_x_ratio = 1;
		logo_mc_y_ratio = 1;
	} else if (is_me1mc4 == 1) {
		reg_me_dsx_scale = 2;
		reg_me_dsy_scale = 2;
		me_mc_x_ratio = 2;
		me_mc_y_ratio = 2;
		logo_mc_x_ratio = 2;
		logo_mc_y_ratio = 2;
	} else if (is_me1mc4 == 2) {
		reg_me_dsx_scale = 2;
		reg_me_dsy_scale = 1;
		me_mc_x_ratio = 2;
		me_mc_y_ratio = 1;
		logo_mc_x_ratio = 2;
		logo_mc_y_ratio = 1;
	} else {
		reg_me_dsx_scale = 1;
		reg_me_dsy_scale = 2;
		me_mc_x_ratio = 1;
		me_mc_y_ratio = 2;
		logo_mc_x_ratio = 1;
		logo_mc_y_ratio = 2;
	}
	osd_mc_x_ratio = 0;
	osd_mc_y_ratio = 0;

	frc_config_reg_value((reg_me_dsx_scale << 6 | reg_me_dsy_scale << 4),
		0xf0, &regdata_hme_scale_012d);
	WRITE_FRC_REG_BY_CPU(FRC_REG_ME_SCALE, regdata_hme_scale_012d);

	frc_config_reg_value((logo_mc_x_ratio << 4 | logo_mc_y_ratio << 2),
		0xf << 2, &regdata_blkscale_012c); //reg_logo_mc_dsx_ratio | reg_logo_mc_dsy_ratio

	frc_config_reg_value((logo_mc_x_ratio - osd_mc_x_ratio) << 20 |
		(logo_mc_y_ratio - osd_mc_y_ratio) << 18,
		0xf << 18, &regdata_blkscale_012c); //reg_osd_logo_dsx_ratio  reg_osd_logo_dsy_ratio

	frc_config_reg_value(0x1 << 14, 0x3 << 14, &regdata_blkscale_012c);//reg_osd_logo_ratio_th

	WRITE_FRC_REG_BY_CPU(FRC_REG_BLK_SCALE, regdata_blkscale_012c);

	//reg_me_logo_dsx_ratio    reg_me_logo_dsy_ratio
	// tmp_reg_value = logo_mc_ratio - me_mc_ratio;
	// tmp_reg_value |= (logo_mc_ratio - me_mc_ratio) << 1;
	frc_config_reg_value((logo_mc_x_ratio - me_mc_x_ratio) << 25 |
		(logo_mc_y_ratio - me_mc_y_ratio) << 24, 0x3 << 24, &regdata_blksizexy_012b);
	// frc_config_reg_value(tmp_reg_value<<24, 0x03000000, &regdata_blksizexy_012b);
	WRITE_FRC_REG_BY_CPU(FRC_REG_BLK_SIZE_XY, regdata_blksizexy_012b);

	reg_me_mvx_div_mode = (regdata_me_en_1100 >> 4) & 0x3;
	reg_me_mvy_div_mode = (regdata_me_en_1100 >> 0) & 0x3;

	tmp_reg_value =
	((ME_MVY_BIT - 2 + reg_me_mvy_div_mode) - (ME_FINER_HIST_BIT + ME_ROUGH_Y_HIST_BIT)) & 0x7;
	tmp_reg_value |=
	((ME_MVX_BIT - 2 + reg_me_mvx_div_mode) - (ME_FINER_HIST_BIT + ME_ROUGH_X_HIST_BIT)) << 4;
	WRITE_FRC_REG_BY_CPU(FRC_ME_GMV_CTRL, tmp_reg_value | 0xc00);

	//reg_region_rp_gmv_mvx_sft
	//reg_region_rp_gmv_mvy_sft
	tmp_reg_value = (reg_me_mvy_div_mode + 2) << 14 & 0x1c000;
	tmp_reg_value |= (reg_me_mvy_div_mode + 2) << 17 & 0xe0000;
	WRITE_FRC_REG_BY_CPU(FRC_ME_REGION_RP_GMV_2, tmp_reg_value | 0x90400);

	/* because me_mc_ratio only equal 1 or 2
	 *if (me_mc_ratio == 2) {
	 *	reg_mc_blksize_xscale = 4;
	 *	reg_mc_blksize_yscale = 4;
	 *	reg_mc_mvx_scale =  2;
	 *	reg_mc_mvy_scale =  2;
	 *	reg_mc_fetch_size = 9;
	 *	reg_mc_blk_x      = 16;
	 *} else {
	 *	reg_mc_blksize_xscale = 3;
	 *	reg_mc_blksize_yscale = 3;
	 *	reg_mc_mvx_scale =  1;
	 *	reg_mc_mvy_scale =  1;
	 *	reg_mc_fetch_size = 5;
	 *	reg_mc_blk_x      = 8;
	 *}
	 */
	if (me_mc_x_ratio == 2) {
		reg_mc_blksize_xscale = 4;
		reg_mc_mvx_scale = 2;
		reg_mc_fetch_size = 9;
		reg_mc_blk_x = 16;
	} else {  // if (me_mc_x_ratio == 1)
		reg_mc_blksize_xscale = 3;
		reg_mc_mvx_scale = 1;
		reg_mc_fetch_size = 5;
		reg_mc_blk_x = 8;
	}
	/*
	 *else if (me_mc_x_ratio == 0) {
	 *	reg_mc_blksize_xscale = 2;
	 *	reg_mc_mvx_scale = 0;
	 *	reg_mc_fetch_size = 3;
	 *	reg_mc_blk_x = 4;
	 *} else if (me_mc_x_ratio == 3) {
	 *	reg_mc_blksize_xscale = 5;
	 *	reg_mc_mvx_scale = 3;
	 *	reg_mc_fetch_size = 17;
	 *	reg_mc_blk_x = 32;
	 *}
	 */

	if (me_mc_y_ratio == 2) {
		reg_mc_blksize_yscale = 4;
		reg_mc_mvy_scale = 2;
	} else {  // if (me_mc_y_ratio == 1)
		reg_mc_blksize_yscale = 3;
		reg_mc_mvy_scale = 1;
	}
	/*
	 *else if (me_mc_y_ratio == 0) {
	 *	reg_mc_blksize_yscale = 2;
	 *	reg_mc_mvy_scale = 0;
	 *} else if (me_mc_y_ratio == 3) {
	 *	reg_mc_blksize_yscale = 5;
	 *	reg_mc_mvy_scale = 3;
	 *}
	 */
	tmp_reg_value = reg_mc_blksize_yscale << 6 & 0x01c0;
	tmp_reg_value |= reg_mc_blksize_xscale << 9 & 0xe00;
	frc_config_reg_value(tmp_reg_value, 0xfc0, &regdata_blkscale_012c);
	WRITE_FRC_REG_BY_CPU(FRC_REG_BLK_SCALE, regdata_blkscale_012c);

	tmp_reg_value = reg_mc_mvy_scale & 0xf;
	tmp_reg_value |= (reg_mc_mvx_scale << 8) & 0xf00;
	frc_config_reg_value(tmp_reg_value, 0xf0f, &regdata_mcset1_3000);
	WRITE_FRC_REG_BY_CPU(FRC_MC_SETTING1, regdata_mcset1_3000);
	tmp_reg_value = reg_mc_blk_x & 0x3f;
	tmp_reg_value |= (reg_mc_fetch_size << 8) & 0x3f00;
	frc_config_reg_value(tmp_reg_value, 0x3f3f, &regdata_mcset2_3001);
	WRITE_FRC_REG_BY_CPU(FRC_MC_SETTING2, regdata_mcset2_3001);

	//  MC
	//reg_mc_vsrch_rng_luma  reg_mc_vsrch_rng_chrm
	tmp_reg_value = MAX_MC_C_VRANG;
	tmp_reg_value |= (MAX_MC_Y_VRANG << 8) & 0xff00;
	WRITE_FRC_REG_BY_CPU(FRC_NOW_SRCH_REG, tmp_reg_value | 0x20200000);  // 0x308e, 0x2020a0a0

	// bbd
	reg_bb_det_top  = 0;
	reg_bb_det_bot  = frm_vsize - 1;
	reg_bb_det_lft  = 0;
	reg_bb_det_rit  = frm_hsize - 1;

	regdata_bbd_t2b_0604 = reg_bb_det_bot;
	regdata_bbd_t2b_0604 |= reg_bb_det_top << 16;
	WRITE_FRC_REG_BY_CPU(FRC_BBD_DETECT_REGION_TOP2BOT, regdata_bbd_t2b_0604);
	regdata_bbd_l2r_0605 = reg_bb_det_rit;
	regdata_bbd_l2r_0605 |= reg_bb_det_lft << 16;
	WRITE_FRC_REG_BY_CPU(FRC_BBD_DETECT_REGION_LFT2RIT, regdata_bbd_l2r_0605);

	reg_bb_det_motion_top  = reg_bb_det_top >> reg_me_dsy_scale;
	reg_bb_det_motion_bot  = ((reg_bb_det_bot + 1) >> reg_me_dsy_scale) - 1;
	reg_bb_det_motion_lft  = reg_bb_det_lft >> reg_me_dsx_scale;
	reg_bb_det_motion_rit  = ((reg_bb_det_rit + 1) >> reg_me_dsx_scale) - 1;

	// reg_bb_det_motion_top _bot _lft _rit
	tmp_reg_value = reg_bb_det_motion_bot;
	tmp_reg_value |= reg_bb_det_motion_top << 16;
	WRITE_FRC_REG_BY_CPU(FRC_BBD_DETECT_MOTION_REGION_TOP2BOT, tmp_reg_value);
	tmp_reg_value_2 = reg_bb_det_motion_rit;
	tmp_reg_value_2 |= reg_bb_det_motion_lft << 16;
	WRITE_FRC_REG_BY_CPU(FRC_BBD_DETECT_MOTION_REGION_LFT2RIT, tmp_reg_value_2);

	// TODO....
	//reg_bb_det_detail_h_top  _h_bot _h_lft _h_rit
	// pr_frc(0, "FRC_BBD_DETECT_DETAIL_H_TOP2BOT(0x0606):%08x\n", regdata_bbd_t2b_0604);
	WRITE_FRC_REG_BY_CPU(FRC_BBD_DETECT_DETAIL_H_TOP2BOT, regdata_bbd_t2b_0604);
	WRITE_FRC_REG_BY_CPU(FRC_BBD_DETECT_DETAIL_H_LFT2RIT, regdata_bbd_l2r_0605);

	// reg_bb_det_detail_v_top  _v_bot _v_lft   _v_rit
	WRITE_FRC_REG_BY_CPU(FRC_BBD_DETECT_DETAIL_V_TOP2BOT, regdata_bbd_t2b_0604);
	WRITE_FRC_REG_BY_CPU(FRC_BBD_DETECT_DETAIL_V_LFT2RIT, tmp_reg_value_2);  // to check

	//reg_debug_top/bot/lft/rit_motion_posi2
	WRITE_FRC_REG_BY_CPU(FRC_REG_DEBUG_PATH_TOP_BOT_MOTION_POSI2, regdata_bbd_t2b_0604);
	WRITE_FRC_REG_BY_CPU(FRC_REG_DEBUG_PATH_LFT_RIT_MOTION_POSI2, regdata_bbd_l2r_0605);

	//reg_debug_top/bot/lft/rit_motion_posi1
	WRITE_FRC_REG_BY_CPU(FRC_REG_DEBUG_PATH_TOP_BOT_MOTION_POSI1, regdata_bbd_t2b_0604);
	WRITE_FRC_REG_BY_CPU(FRC_REG_DEBUG_PATH_LFT_RIT_MOTION_POSI1, regdata_bbd_l2r_0605);

	//reg_debug_top/bot/lft/rit_edge_posi2
	WRITE_FRC_REG_BY_CPU(FRC_REG_DEBUG_PATH_TOP_BOT_EDGE_POSI2, regdata_bbd_t2b_0604);
	WRITE_FRC_REG_BY_CPU(FRC_REG_DEBUG_PATH_LFT_RIT_EDGE_POSI2, regdata_bbd_l2r_0605);

	//reg_debug_top/bot/lft/rit_edge_posi1
	WRITE_FRC_REG_BY_CPU(FRC_REG_DEBUG_PATH_TOP_BOT_EDGE_POSI1, regdata_bbd_t2b_0604);
	WRITE_FRC_REG_BY_CPU(FRC_REG_DEBUG_PATH_LFT_RIT_EDGE_POSI1, regdata_bbd_l2r_0605);

	// black bar location  (will be changed oframe base by fw algorithm)
	//reg_blackbar_xyxy_0 _xyxy_1 _xyxy_2 _xyxy_3
	regdata_bb_xy_st_0119 = reg_bb_det_top;
	regdata_bb_xy_st_0119 |= (reg_bb_det_lft << 16);
	WRITE_FRC_REG_BY_CPU(FRC_REG_BLACKBAR_XYXY_ST, regdata_bb_xy_st_0119);
	regdata_bb_xy_ed_011a = reg_bb_det_bot;
	regdata_bb_xy_ed_011a |= (reg_bb_det_rit << 16);
	WRITE_FRC_REG_BY_CPU(FRC_REG_BLACKBAR_XYXY_ED, regdata_bb_xy_ed_011a);

	//reg_blackbar_xyxy_me_0
	regdata_bb_xy_me_st_011b = reg_bb_det_top;
	regdata_bb_xy_me_st_011b |= (reg_bb_det_lft << 16);
	WRITE_FRC_REG_BY_CPU(FRC_REG_BLACKBAR_XYXY_ME_ST, regdata_bb_xy_me_st_011b);
	regdata_bb_xy_me_ed_011c = reg_bb_det_bot;
	regdata_bb_xy_me_ed_011c |= (reg_bb_det_rit << 16);
	WRITE_FRC_REG_BY_CPU(FRC_REG_BLACKBAR_XYXY_ME_ED, regdata_bb_xy_me_ed_011c);

	// TODO: related to mc registers??
	// reg_data = READ_FRC_REG(FRC_MC_DEMO_WINDOW);
	reg_data = regdata_mcdemo_win_3200;
	reg_demo_window1_en = (reg_data >> 3) & 0x1;
	reg_demo_window2_en = (reg_data >> 2) & 0x1;
	reg_demo_window3_en = (reg_data >> 1) & 0x1;
	reg_demo_window4_en = reg_data & 0x1;
	if (reg_demo_window1_en) {
	// WRITE_FRC_REG_BY_CPU(FRC_REG_DEMOWINDOW1_XYXY_ST, (frm_hsize>>1)<<16);
	// WRITE_FRC_REG_BY_CPU(FRC_REG_DEMOWINDOW1_XYXY_ED,  (frm_vsize-1) | (frm_hsize-1)<<16);
	}
	if (reg_demo_window2_en) {
	//reg_demowindow2_xyxy_0
	// WRITE_FRC_REG_BY_CPU(FRC_REG_DEMOWINDOW2_XYXY_ST, (frm_hsize>>1)<<16);
	// WRITE_FRC_REG_BY_CPU(FRC_REG_DEMOWINDOW2_XYXY_ED,  (frm_vsize-1) | (frm_hsize-1)<<16);
	}
	if (reg_demo_window3_en) {
	//reg_demowindow3_xyxy_0
	//WRITE_FRC_REG_BY_CPU(FRC_REG_DEMOWINDOW3_XYXY_ST, (frm_hsize>>1)<<16);
	// WRITE_FRC_REG_BY_CPU(FRC_REG_DEMOWINDOW3_XYXY_ED,  (frm_vsize-1) | (frm_hsize-1)<<16);
	}
	if (reg_demo_window4_en) {
	//reg_demowindow4_xyxy_0
	// WRITE_FRC_REG_BY_CPU(FRC_REG_DEMOWINDOW4_XYXY_ST, (frm_hsize>>1)<<16);
	// WRITE_FRC_REG_BY_CPU(FRC_REG_DEMOWINDOW4_XYXY_ED,  (frm_vsize-1) | (frm_hsize-1)<<16);
	}
	//reg_bb_ds_xyxy_0 _xyxy_1
	WRITE_FRC_REG_BY_CPU(FRC_REG_DS_WIN_SETTING_LFT_TOP,  0);
	//reg_bb_ds_xyxy_2 xyxy_3
	WRITE_FRC_REG_BY_CPU(FRC_BBD_DS_WIN_SETTING_RIT_BOT,
			(frm_vsize - 1) | (frm_hsize - 1) << 16);

	//reg_mc_org_me_bb_xyxy_0  xyxy_1
	WRITE_FRC_REG_BY_CPU(FRC_MC_BB_HANDLE_ORG_ME_BB_XYXY_LEFT_TOP,  0);

	regdata_hme_scale_012d = READ_FRC_REG(FRC_REG_ME_SCALE);
	mc_org_me_bb_2 = (regdata_hme_scale_012d >> 6) & 0x3;
	mc_org_me_bb_3 = (regdata_hme_scale_012d >> 4) & 0x3;
	//reg_mc_org_me_bb_xyxy_2 xyxy_3
	tmp_reg_value = (frm_vsize >> mc_org_me_bb_3) - 1;
	tmp_reg_value |= ((tmp_reg_value >> mc_org_me_bb_2) - 1) << 16;
	WRITE_FRC_REG_BY_CPU(FRC_MC_BB_HANDLE_ORG_ME_BB_XYXY_RIGHT_BOT,  tmp_reg_value);
	//reg_mc_org_me_blk_bb_xyxy_0/1
	WRITE_FRC_REG_BY_CPU(FRC_MC_BB_HANDLE_ORG_ME_BLK_BB_XYXY_LFT_AND_TOP,  0);

	regdata_blksizexy_012b = READ_FRC_REG(FRC_REG_BLK_SIZE_XY);
	mc_org_me_blk_bb_2 = (regdata_blksizexy_012b >> 19) & 0x7;
	mc_org_me_blk_bb_3 = (regdata_blksizexy_012b >> 16) & 0x7;
	//reg_mc_org_me_blk_bb_xyxy_2/3
	tmp_reg_value = ((frm_vsize >> mc_org_me_bb_3) >> mc_org_me_blk_bb_3) - 1;
	tmp_reg_value |= (((frm_hsize >> mc_org_me_bb_2) >> mc_org_me_blk_bb_2) - 1) << 16;
	WRITE_FRC_REG_BY_CPU(FRC_MC_BB_HANDLE_ORG_ME_BLK_BB_XYXY_RIT_AND_BOT,  tmp_reg_value);

	tmp_reg_value = reg_bb_det_top;
	tmp_reg_value |= (reg_bb_det_lft << 16);
	tmp_reg_value_2 = reg_bb_det_bot;
	tmp_reg_value_2 |= (reg_bb_det_rit << 16);

	//reg_bb_apl_hist_xyxy_0/1/2/3
	WRITE_FRC_REG_BY_CPU(FRC_BBD_APL_HIST_WIN_LFT_TOP, tmp_reg_value);
	WRITE_FRC_REG_BY_CPU(FRC_BBD_APL_HIST_WIN_RIT_BOT, tmp_reg_value_2);

	//reg_bb_oob_apl_xyxy_0/1/2/3
	WRITE_FRC_REG_BY_CPU(FRC_BBD_OOB_APL_CAL_LFT_TOP_RANGE, tmp_reg_value);
	WRITE_FRC_REG_BY_CPU(FRC_BBD_OOB_APL_CAL_RIT_BOT_RANGE, tmp_reg_value_2);

	//reg_bb_oob_h_detail_xyxy_0/1/2/3
	WRITE_FRC_REG_BY_CPU(FRC_BBD_OOB_DETAIL_WIN_LFT_TOP, tmp_reg_value);
	WRITE_FRC_REG_BY_CPU(FRC_BBD_OOB_DETAIL_WIN_RIT_BOT, tmp_reg_value_2);

	bb_mo_ds_2 = MIN((frm_hsize >> mc_org_me_bb_2) - 1, (reg_bb_det_rit >> mc_org_me_bb_2));
	bb_mo_ds_3 = MIN((frm_vsize >> mc_org_me_bb_3) - 1, (reg_bb_det_bot >> mc_org_me_bb_3));
	//reg_bb_motion_xyxy_ds_0/1/2/3
	tmp_reg_value = reg_bb_det_top >> mc_org_me_bb_3;
	tmp_reg_value |= (reg_bb_det_lft >> mc_org_me_bb_2) << 16;
	tmp_reg_value_2 = bb_mo_ds_3;
	tmp_reg_value_2 |= bb_mo_ds_2 << 16;
	WRITE_FRC_REG_BY_CPU(FRC_BBD_MOTION_DETEC_REGION_LFT_TOP_DS,  tmp_reg_value);
	WRITE_FRC_REG_BY_CPU(FRC_BBD_MOTION_DETEC_REGION_RIT_BOT_DS,  tmp_reg_value_2);

	bb_oob_v_2 = MIN((frm_hsize >> mc_org_me_bb_2) - 1, (reg_bb_det_rit >> mc_org_me_bb_2));
	//reg_bb_oob_v_detail_xyxy_0/1/2/3
	tmp_reg_value = reg_bb_det_top;
	tmp_reg_value |= (reg_bb_det_lft >> mc_org_me_bb_2) << 16;
	tmp_reg_value_2 = reg_bb_det_bot;
	tmp_reg_value_2 |= bb_oob_v_2 << 16;
	WRITE_FRC_REG_BY_CPU(FRC_BBD_OOB_V_DETAIL_WIN_LFT_TOP,  tmp_reg_value);
	WRITE_FRC_REG_BY_CPU(FRC_BBD_OOB_V_DETAIL_WIN_RIT_BOT,  tmp_reg_value_2);

	//reg_bb_flat_xyxy_0/1/2/3
	WRITE_FRC_REG_BY_CPU(FRC_BBD_FLATNESS_DETEC_REGION_LFT_TOP,
			reg_bb_det_top | (reg_bb_det_lft << 16));
	WRITE_FRC_REG_BY_CPU(FRC_BBD_FLATNESS_DETEC_REGION_RIT_BOT,
			reg_bb_det_bot | (reg_bb_det_rit << 16));
}

void frc_top_init(struct frc_dev_s *frc_devp)
{
	////Config frc ctrl timming
	u32 me_hold_line;//me_hold_line
	u32 mc_hold_line;//mc_hold_line
	u32 inp_hold_line;
	u32 reg_post_dly_vofst;//fixed
	u32 reg_mc_dly_vofst0 ;//fixed
	u32 reg_mc_out_line;
	u32 reg_me_dly_vofst;
	u32 mevp_frm_dly;//Read RO ro_mevp_dly_num
	u32 mc_frm_dly;//Read RO ro_mc2out_dly_num
	u32 memc_frm_dly;//total delay
	u32 reg_mc_dly_vofst1;
	u32 log = 2;
	u32 tmpvalue;
	u32 tmpvalue2;
	u32 frc_v_porch;
	u32 frc_vporch_cal;
	u32 frc_porch_delta;
	u32 adj_mc_dly;
	enum chip_id chip;

	struct frc_fw_data_s *fw_data;
	struct frc_top_type_s *frc_top;

	fw_data = (struct frc_fw_data_s *)frc_devp->fw_data;
	frc_top = &fw_data->frc_top_type;

	regdata_outholdctl_0003 = READ_FRC_REG(FRC_OUT_HOLD_CTRL);
	regdata_inpholdctl_0002 = READ_FRC_REG(FRC_INP_HOLD_CTRL);

	me_hold_line = regdata_outholdctl_0003 & 0xff;
	mc_hold_line = (regdata_outholdctl_0003 >> 8) & 0xff;
	inp_hold_line = regdata_inpholdctl_0002 & 0x1fff;

	fw_data->holdline_parm.me_hold_line = me_hold_line;
	fw_data->holdline_parm.mc_hold_line = mc_hold_line;
	fw_data->holdline_parm.inp_hold_line = inp_hold_line;

	reg_post_dly_vofst = fw_data->holdline_parm.reg_post_dly_vofst;
	reg_mc_dly_vofst0 = fw_data->holdline_parm.reg_mc_dly_vofst0;
	chip = get_chip_type();

	frc_input_init(frc_devp, frc_top);
	config_phs_lut(frc_top->frc_ratio_mode, frc_top->film_mode);
	config_phs_regs(frc_top->frc_ratio_mode, frc_top->film_mode);
	pr_frc(log, "%s\n", __func__);
	// Config frc input size & vpu register
	if (chip == ID_T3X) {
		//if (frc_top->frc_ratio_mode == FRC_RATIO_1_2)
		//	tmpvalue = (frc_top->hsize + 15) & 0xFFF0;
		//else
		//	tmpvalue = frc_top->hsize;
		tmpvalue = frc_top->hsize;
		tmpvalue |= (frc_top->vsize) << 16;
		WRITE_FRC_REG_BY_CPU(FRC_FRAME_SIZE, tmpvalue);
		frc_top->vfb =
			(vpu_reg_read(ENCL_VIDEO_VAVON_BLINE_T3X) >> 16) & 0xffff;
	} else {
		tmpvalue = frc_top->hsize;
		tmpvalue |= (frc_top->vsize) << 16;
		WRITE_FRC_REG_BY_CPU(FRC_FRAME_SIZE, tmpvalue);
		frc_top->vfb = vpu_reg_read(ENCL_VIDEO_VAVON_BLINE);
	}
	pr_frc(log, "ENCL_VIDEO_VAVON_BLINE:%d\n", frc_top->vfb);

	// frc_win_align_apply(frc_devp, frc_top);

	//(frc_top->vfb / 4) * 3; 3/4 point of front vblank, default
	reg_mc_out_line = frc_init_out_line();
	adj_mc_dly = frc_devp->out_line;    // from user debug

	// reg_me_dly_vofst = reg_mc_out_line;
	reg_me_dly_vofst = reg_mc_dly_vofst0;  // change for keep me first run
	if (frc_top->hsize <= 1920 && (frc_top->hsize * frc_top->vsize <= 1920 * 1080)) {
		frc_top->is_me1mc4 = 0;/*me:mc 1:2*/
		if (frc_top->hsize * frc_top->vsize == 1920 * 1080) {
			WRITE_FRC_REG_BY_CPU(FRC_INPUT_SIZE_ALIGN, 0x0);  //8*8 align
		} else {
			WRITE_FRC_REG_BY_CPU(FRC_INPUT_SIZE_ALIGN, 0x3);   //16*16 align
		}
	} else {
		frc_top->is_me1mc4 = 1;/*me:mc 1:4*/
		WRITE_FRC_REG_BY_CPU(FRC_INPUT_SIZE_ALIGN, 0x3);   //16*16 align
	}
	// little window
	if (frc_top->hsize * frc_top->vsize <
		frc_top->out_hsize * frc_top->out_vsize &&
		frc_top->out_hsize * frc_top->out_vsize >= 3840 * 1080) {
		frc_top->is_me1mc4 = 1;/*me:mc 1:4*/
		WRITE_FRC_REG_BY_CPU(FRC_INPUT_SIZE_ALIGN, 0x3);   //16*16 align
		pr_frc(log, "me:mc 1:4\n");
		if (frc_top->hsize < WIDTH_2K && frc_top->out_hsize == WIDTH_4K)
			frc_devp->little_win = 1;
	} else if (chip == ID_T3X) {
		//if (frc_devp->out_sts.out_framerate > 80)
		//	frc_devp->ud_dbg.res2_dbg_en = 0;
		//else
		//	frc_devp->ud_dbg.res2_dbg_en = 1;
	} else {
		frc_devp->ud_dbg.res2_dbg_en = 3;
	}
	/*The resolution of the input is not standard for test
	 * input size adjust within 8 lines and output size > 1920 * 1080
	 * default FRC_PARAM_NUM: 8 adjustable
	 */
	if (!frc_devp->in_sts.inp_size_adj_en &&
		frc_top->out_hsize - FRC_PARAM_NUM < frc_top->hsize &&
		frc_top->hsize <= frc_top->out_hsize &&
		frc_top->out_vsize - FRC_PARAM_NUM < frc_top->vsize &&
		frc_top->vsize < frc_top->out_vsize &&
		frc_top->out_hsize * frc_top->out_vsize >= 1920 * 1080) {
		frc_top->is_me1mc4 = 1;/*me:mc 1:4*/
		WRITE_FRC_REG_BY_CPU(FRC_INPUT_SIZE_ALIGN, 0x0);  //8*8 align
		pr_frc(log, " %s this input case is not standard\n", __func__);
	}

	if (frc_top->out_hsize == 1920 && frc_top->out_vsize == 1080) {
		// mevp_frm_dly = 110;
		// mc_frm_dly   = 5;
		//inp performace issue, need frc_clk > enc0_clk
		mevp_frm_dly = frc_devp->frm_dly_set[0].mevp_frm_dly;
		mc_frm_dly  = frc_devp->frm_dly_set[0].mc_frm_dly;
	} else if (frc_top->out_hsize == 3840 && frc_top->out_vsize == 2160) {
		//mevp_frm_dly = 222; // reg readback  under 333MHz
		//mc_frm_dly = 28;   // reg readback (14)  under 333MHz
		// mevp_frm_dly = 260;   // under 400MHz
		// mc_frm_dly = 28;      // under 400MHz
		if (frc_devp->out_sts.out_framerate > 80) { // 4k2k-120Hz
			mevp_frm_dly = frc_devp->frm_dly_set[3].mevp_frm_dly;
			mc_frm_dly  = frc_devp->frm_dly_set[3].mc_frm_dly;
		} else {
			mevp_frm_dly = frc_devp->frm_dly_set[1].mevp_frm_dly;
			mc_frm_dly  = frc_devp->frm_dly_set[1].mc_frm_dly;
		}
	} else if (frc_top->out_hsize == 3840 && frc_top->out_vsize == 1080) {
		reg_mc_out_line = (frc_top->vfb / 2) * 1;
		reg_me_dly_vofst = reg_mc_out_line;
		//mevp_frm_dly = 222; // reg readback  under 333MHz
		//mc_frm_dly = 20;   // reg readback (34)  under 333MHz
		mevp_frm_dly = frc_devp->frm_dly_set[2].mevp_frm_dly;
		mc_frm_dly  = frc_devp->frm_dly_set[2].mc_frm_dly;
		pr_frc(log, "4k1k_mc_frm_dly:%d\n", mc_frm_dly);
		//if (chip == ID_T3X)
		//	frc_top->is_me1mc4 = 2;
	} else {
		//mevp_frm_dly = 140;
		//mc_frm_dly = 10;
		mevp_frm_dly = frc_devp->frm_dly_set[3].mevp_frm_dly;
		mc_frm_dly  = frc_devp->frm_dly_set[3].mc_frm_dly;
	}
	//memc_frm_dly
	memc_frm_dly      = reg_me_dly_vofst + me_hold_line + mevp_frm_dly +
				mc_frm_dly  + mc_hold_line + adj_mc_dly;
	reg_mc_dly_vofst1 = memc_frm_dly - mc_frm_dly   - mc_hold_line ;
	frc_vporch_cal    = memc_frm_dly - reg_mc_out_line;

	/*bug only for T3*/
	if (chip == ID_T3)
		WRITE_FRC_REG_BY_CPU(FRC_REG_TOP_CTRL27, frc_vporch_cal);

	if (chip == ID_T3 && is_meson_rev_a()) {
		if ((frc_top->out_hsize > 1920 && frc_top->out_vsize > 1080) ||
			(frc_top->out_hsize == 3840 && frc_top->out_vsize == 1080 &&
			frc_devp->out_sts.out_framerate > 80)) {
			/*
			 * MEMC 4K ENCL setting, vlock will change the ENCL_VIDEO_MAX_LNCNT,
			 * so need dynamic change this register
			 */
			//u32 frc_v_porch = vpu_reg_read(ENCL_FRC_CTRL);/*0x1cdd*/
			u32 max_lncnt   = vpu_reg_read(ENCL_VIDEO_MAX_LNCNT);/*0x1cbb*/
			u32 max_pxcnt   = vpu_reg_read(ENCL_VIDEO_MAX_PXCNT);/*0x1cb0*/

			pr_frc(log, "max_lncnt=%d max_pxcnt=%d\n", max_lncnt, max_pxcnt);

			//frc_v_porch     = max_lncnt > 1800 ? max_lncnt - 1800 : frc_vporch_cal;
			frc_v_porch = ((max_lncnt - frc_vporch_cal) <= 1950) ?
					frc_vporch_cal : (max_lncnt - 1950);

			gst_frc_param.frc_v_porch = frc_v_porch;
			gst_frc_param.max_lncnt = max_lncnt;
			gst_frc_param.max_pxcnt = max_pxcnt;
			gst_frc_param.frc_mcfixlines =
				mc_frm_dly + mc_hold_line - reg_mc_out_line;
			if (mc_frm_dly + mc_hold_line < reg_mc_out_line)
				gst_frc_param.frc_mcfixlines = 0;
			if (frc_devp->vlock_flag == 1) {
				gst_frc_param.s2l_en = 0;
				frc_devp->vlock_flag = 0;
			} else {
				gst_frc_param.s2l_en = 1;
			}
			if (vlock_sync_frc_vporch(gst_frc_param) < 0)
				PR_ERR("frc_infrom vlock fail, check vlock st!\n");
			else
				pr_frc(0, "frc_on_set maxlnct success!!!\n");

			//vpu_reg_write(ENCL_SYNC_TO_LINE_EN,
			// (1 << 13) | (max_lncnt - frc_v_porch));
			//vpu_reg_write(ENCL_SYNC_LINE_LENGTH, max_lncnt - frc_v_porch - 1);
			//vpu_reg_write(ENCL_SYNC_PIXEL_EN, (1 << 15) | (max_pxcnt - 1));

			pr_frc(log, "ENCL_SYNC_TO_LINE_EN=0x%x\n",
				vpu_reg_read(ENCL_SYNC_TO_LINE_EN));
			pr_frc(log, "ENCL_SYNC_PIXEL_EN=0x%x\n",
				vpu_reg_read(ENCL_SYNC_PIXEL_EN));
			pr_frc(log, "ENCL_SYNC_LINE_LENGTH=0x%x\n",
				vpu_reg_read(ENCL_SYNC_LINE_LENGTH));
		} else {
			frc_v_porch = frc_vporch_cal;
			gst_frc_param.frc_mcfixlines =
				mc_frm_dly + mc_hold_line - reg_mc_out_line;
			if (mc_frm_dly + mc_hold_line < reg_mc_out_line)
				gst_frc_param.frc_mcfixlines = 0;
			gst_frc_param.s2l_en = 0;
			if (vlock_sync_frc_vporch(gst_frc_param) < 0)
				PR_ERR("frc_infrom vlock fail, check vlock st!\n");
			else
				pr_frc(1, "frc_infrom vlock success!!!\n");
		}
		frc_porch_delta = frc_v_porch - frc_vporch_cal;
		pr_frc(log, "frc_v_porch=%d frc_porch_delta=%d\n",
			frc_v_porch, frc_porch_delta);
		vpu_reg_write_bits(ENCL_FRC_CTRL, frc_v_porch, 0, 16);
		tmpvalue = (reg_post_dly_vofst + frc_porch_delta);
		tmpvalue |= (reg_me_dly_vofst  + frc_porch_delta) << 16;
		WRITE_FRC_REG_BY_CPU(FRC_REG_TOP_CTRL14, tmpvalue);
		tmpvalue2 = (reg_mc_dly_vofst0 + frc_porch_delta);
		tmpvalue2 |= (reg_mc_dly_vofst1 + frc_porch_delta) << 16;
		WRITE_FRC_REG_BY_CPU(FRC_REG_TOP_CTRL15, tmpvalue2);
	} else {
		/*T3 revB,t5m,t3x*/
		frc_v_porch = frc_vporch_cal;
		pr_frc(2, "%s t3b/t5m/t3x inform vlock\n", __func__);
		gst_frc_param.frc_mcfixlines =
			mc_frm_dly + mc_hold_line - reg_mc_out_line;
		if (mc_frm_dly + mc_hold_line < reg_mc_out_line)
			gst_frc_param.frc_mcfixlines = 0;
		gst_frc_param.s2l_en = 2; /* rev B chip*/
		if (vlock_sync_frc_vporch(gst_frc_param) < 0)
			PR_ERR("frc_infrom vlock fail, check vlock st!\n");
		else
			pr_frc(1, "frc_infrom vlock success!!!\n");
		if (chip == ID_T3X)
			vpu_reg_write_bits(ENCL_FRC_CTRL_T3X,
				memc_frm_dly - reg_mc_out_line, 0, 16);
		else
			vpu_reg_write_bits(ENCL_FRC_CTRL, memc_frm_dly - reg_mc_out_line, 0, 16);
		// WRITE_FRC_BITS(FRC_REG_TOP_CTRL14, reg_post_dly_vofst, 0, 16);
		// WRITE_FRC_BITS(FRC_REG_TOP_CTRL14, reg_me_dly_vofst, 16, 16);
		// WRITE_FRC_BITS(FRC_REG_TOP_CTRL15, reg_mc_dly_vofst0, 0, 16);
		// WRITE_FRC_BITS(FRC_REG_TOP_CTRL15, reg_mc_dly_vofst1, 16,  16);
		WRITE_FRC_REG_BY_CPU(FRC_REG_TOP_CTRL14,
			reg_post_dly_vofst | (reg_me_dly_vofst << 16));
		WRITE_FRC_REG_BY_CPU(FRC_REG_TOP_CTRL15,
			reg_mc_dly_vofst0 | (reg_mc_dly_vofst1 << 16));
	}
	pr_frc(log, "reg_mc_out_line   = %d\n", reg_mc_out_line);
	pr_frc(log, "me_hold_line      = %d\n", me_hold_line);
	pr_frc(log, "mc_hold_line      = %d\n", mc_hold_line);
	pr_frc(log, "mevp_frm_dly      = %d\n", mevp_frm_dly);
	pr_frc(log, "adj_mc_dly        = %d\n", adj_mc_dly);
	pr_frc(log, "mc_frm_dly        = %d\n", mc_frm_dly);
	pr_frc(log, "memc_frm_dly      = %d\n", memc_frm_dly);
	pr_frc(log, "reg_mc_dly_vofst1 = %d\n", reg_mc_dly_vofst1);
	pr_frc(log, "frc_ratio_mode = %d\n", frc_top->frc_ratio_mode);
	pr_frc(log, "frc_fb_num = %d\n", frc_top->frc_fb_num);

	if (chip >= ID_T3X && frc_devp->no_ko_mode == 1) {
		frc_drv_bbd_init_xyxy(fw_data);
		frc_drv_fw_param_init(frc_top->hsize, frc_top->vsize, frc_top->is_me1mc4);
	}
}

/*buffer number can dynamic kmalloc,  film_hwfw_sel ????*/
void frc_inp_init(void)
{
	u32 offset = 0x0;
	enum chip_id chip;

	chip = get_chip_type();

	if (chip == ID_T3)
		offset = 0xa0;
	else if (chip == ID_T5M || chip == ID_T3X)
		offset = 0x0;

	WRITE_FRC_REG_BY_CPU(FRC_MC_PRB_CTRL1, 0x00640164); // bit8 set 1 close reg_mc_probe_en

	// WRITE_FRC_BITS(FRC_REG_INP_MODULE_EN + offset, 1, 5, 1);//close reg_inp_logo_en
	// WRITE_FRC_BITS(FRC_REG_INP_MODULE_EN + offset, 1, 8, 1);//open
	// WRITE_FRC_BITS(FRC_REG_INP_MODULE_EN + offset, 1, 7, 1);//open  bbd en
	regdata_inpmoden_04f9 = READ_FRC_REG(FRC_REG_INP_MODULE_EN + offset);
	// frc_config_reg_value((BIT_5 + BIT_7 + BIT_8), (BIT_5 + BIT_7 + BIT_8),
	//					&regdata_inpmoden_04f9);
	frc_config_reg_value((BIT_7 + BIT_8), (BIT_5 + BIT_7 + BIT_8),
						&regdata_inpmoden_04f9);
	WRITE_FRC_REG_BY_CPU(FRC_REG_INP_MODULE_EN + offset, regdata_inpmoden_04f9);
	WRITE_FRC_REG_BY_CPU(FRC_REG_TOP_CTRL25, 0x4080200); //aligned padding value

	// WRITE_FRC_BITS(FRC_MEVP_CTRL0, 0, 0, 1);//close hme
	WRITE_FRC_REG_BY_CPU(FRC_MEVP_CTRL0, 0xe);  //close hme

	/*for fw signal latch, should always enable*/
	//WRITE_FRC_BITS(FRC_REG_MODE_OPT, 1, 1, 1);
	WRITE_FRC_REG_BY_CPU(FRC_REG_MODE_OPT, 0x02);
}

void config_phs_lut(enum frc_ratio_mode_type frc_ratio_mode,
	enum en_drv_film_mode film_mode)
{
	#define lut_ary_size	18
	u32 input_n;
	u32 tmpregdata;
	u32 output_m;
	u64 phs_lut_table[lut_ary_size];
	int i;

	for (i = 0; i < lut_ary_size; i++)
		phs_lut_table[i] =  0xffffffffffffffff;

	if (frc_ratio_mode == FRC_RATIO_1_2) {
	//phs_st plog_dif clog_dif pfrm_dif cfrm_dif nfrm_dif mc_ph me_ph film_ph frc_ph
		if (film_mode == EN_DRV_VIDEO) {
		//phs_lut_table[0] = {1'h1,4'h3,4'h2,4'h3,4'h2,4'h1,8'h00,8'h40,8'h00,8'h00};
		//phs_lut_table[1] = {1'h0,4'h3,4'h2,4'h3,4'h2,4'h1,8'h40,8'h40,8'h00,8'h01};
			phs_lut_table[0] = 0x13232100400000;
			phs_lut_table[1] = 0x03232140400001;
		} else if (film_mode == EN_DRV_FILM22) {
		//cadence = 22_1/2
		//phs_lut_table[0] = {1'h1,4'h4,4'h3,4'h4,4'h3,4'h1,8'h00,8'h20,8'h01,8'h00};
		//phs_lut_table[1] = {1'h0,4'h4,4'h3,4'h4,4'h3,4'h1,8'h20,8'h20,8'h02,8'h01};
		//phs_lut_table[2] = {1'h0,4'h5,4'h3,4'h5,4'h3,4'h2,8'h40,8'h40,8'h03,8'h00};
		//phs_lut_table[3] = {1'h0,4'h5,4'h3,4'h5,4'h3,4'h2,8'h60,8'h60,8'h00,8'h01};
			phs_lut_table[0] = 0x14343100200100;
			phs_lut_table[1] = 0x04343120200201;
			phs_lut_table[2] = 0x05353240400300;
			phs_lut_table[3] = 0x05353260600001;
		} else if (film_mode == EN_DRV_FILM32) {
		//cadence = 32_1/2,table_cnt=   10,match_data=0x14
		//mem[0]={ 1'd1, 4'd3, 4'd2, 4'd5, 4'd4, 4'd1, 8'd0,   8'd25,  8'd1, 8'd0};
		//mem[1]={ 1'd0, 4'd3, 4'd2, 4'd5, 4'd4, 4'd1, 8'd25,  8'd25,  8'd1, 8'd1};
		//mem[2]={ 1'd0, 4'd4, 4'd3, 4'd6, 4'd4, 4'd2, 8'd51,  8'd51,  8'd2, 8'd0};
		//mem[3]={ 1'd0, 4'd4, 4'd3, 4'd6, 4'd4, 4'd2, 8'd76,  8'd76,  8'd2, 8'd1};
		//mem[4]={ 1'd0, 4'd4, 4'd3, 4'd7, 4'd5, 4'd3, 8'd102, 8'd102, 8'd3, 8'd0};
		//mem[5]={ 1'd1, 4'd3, 4'd2, 4'd5, 4'd3, 4'd1, 8'd0,   8'd25,  8'd3, 8'd1};
		//mem[6]={ 1'd0, 4'd3, 4'd2, 4'd6, 4'd3, 4'd2, 8'd25,  8'd25,  8'd4, 8'd0};
		//mem[7]={ 1'd0, 4'd3, 4'd2, 4'd6, 4'd3, 4'd2, 8'd51,  8'd51,  8'd4, 8'd1};
		//mem[8]={ 1'd0, 4'd4, 4'd3, 4'd7, 4'd4, 4'd3, 8'd76,  8'd76,  8'd0, 8'd0};
		//mem[9]={ 1'd0, 4'd4, 4'd3, 4'd7, 4'd4, 4'd3, 8'd102, 8'd102, 8'd0, 8'd1};
			phs_lut_table[0] = 0x13254100190100;
			phs_lut_table[1] = 0x03254119190101;
			phs_lut_table[2] = 0x04364233330200;
			phs_lut_table[3] = 0x0436424c4c0201;
			phs_lut_table[4] = 0x04375366660300;
			phs_lut_table[5] = 0x13253100190301;
			phs_lut_table[6] = 0x03263219190400;
			phs_lut_table[7] = 0x03263233330401;
			phs_lut_table[8] = 0x0437434c4c0000;
			phs_lut_table[9] = 0x04374366660001;
		}
		input_n          = 1;
		output_m         = 2;
	} else if (frc_ratio_mode == FRC_RATIO_2_3) {
	//phs_lut_table[0] = {1'h1, 4'h3, 4'h2, 4'h3, 4'h2, 4'h1, 8'h00   ,8'h55   ,8'h00, 8'h00};
	//phs_lut_table[1] = {1'h0, 4'h3, 4'h2, 4'h3, 4'h2, 4'h1, 8'h55   ,8'h55   ,8'h00, 8'h01};
	//phs_lut_table[2] = {1'h1, 4'h3, 4'h2, 4'h3, 4'h2, 4'h1, 8'h2a   ,8'h2a   ,8'h00, 8'h02};
		phs_lut_table[0] = 0x13232100550000;
		phs_lut_table[1] = 0x03232155550001;
		phs_lut_table[2] = 0x1323212a2a0002;
		input_n          = 2;
		output_m         = 3;
	} else if (frc_ratio_mode == FRC_RATIO_2_5) {
	//phs_lut_table[0] = {1'h1, 4'h3, 4'h2, 4'h3, 4'h2, 4'h1, 8'h00   ,8'h33   ,8'h00, 8'h00};
	//phs_lut_table[1] = {1'h0, 4'h3, 4'h2, 4'h3, 4'h2, 4'h1, 8'h33   ,8'h33   ,8'h00, 8'h01};
	//phs_lut_table[2] = {1'h0, 4'h3, 4'h2, 4'h3, 4'h2, 4'h1, 8'h66   ,8'h66   ,8'h00, 8'h02};
	//phs_lut_table[3] = {1'h1, 4'h3, 4'h2, 4'h3, 4'h2, 4'h1, 8'h19   ,8'h19   ,8'h00, 8'h03};
	//phs_lut_table[4] = {1'h0, 4'h3, 4'h2, 4'h3, 4'h2, 4'h1, 8'h4c   ,8'h4c   ,8'h00, 8'h04};
		phs_lut_table[0] = 0x13232100330000;
		phs_lut_table[1] = 0x03232133330001;
		phs_lut_table[2] = 0x03232166660002;
		phs_lut_table[3] = 0x13232119190003;
		phs_lut_table[4] = 0x0323214c4c0004;
		input_n          = 2;
		output_m         = 5;
	} else if (frc_ratio_mode == FRC_RATIO_5_6) {
	//phs_lut_table[0] = {1'h1, 4'h3, 4'h2, 4'h3, 4'h2, 4'h1, 8'h00   ,8'h6a   ,8'h00, 8'h00};
	//phs_lut_table[1] = {1'h0, 4'h3, 4'h2, 4'h3, 4'h2, 4'h1, 8'h6a   ,8'h6a   ,8'h00, 8'h01};
	//phs_lut_table[2] = {1'h1, 4'h3, 4'h2, 4'h3, 4'h2, 4'h1, 8'h55   ,8'h55   ,8'h00, 8'h02};
	//phs_lut_table[3] = {1'h1, 4'h3, 4'h2, 4'h3, 4'h2, 4'h1, 8'h40   ,8'h40   ,8'h00, 8'h03};
	//phs_lut_table[4] = {1'h1, 4'h3, 4'h2, 4'h3, 4'h2, 4'h1, 8'h2a   ,8'h2a   ,8'h00, 8'h04};
	//phs_lut_table[5] = {1'h1, 4'h3, 4'h2, 4'h3, 4'h2, 4'h1, 8'h15   ,8'h15   ,8'h00, 8'h05};
		phs_lut_table[0] = 0x132321006a0000;
		phs_lut_table[1] = 0x0323216a6a0001;
		phs_lut_table[2] = 0x13232155550002;
		phs_lut_table[3] = 0x13232140400003;
		phs_lut_table[4] = 0x1323212a2a0004;
		phs_lut_table[5] = 0x13232115150005;
		input_n          = 5;
		output_m         = 6;
	} else if (frc_ratio_mode == FRC_RATIO_5_12) {
	//phs_lut_table[0] = {1'h1, 4'h3, 4'h2, 4'h3, 4'h2, 4'h1, 8'h00   ,8'h35   ,8'h00, 8'h00};
	//phs_lut_table[1] = {1'h0, 4'h3, 4'h2, 4'h3, 4'h2, 4'h1, 8'h35   ,8'h35   ,8'h00, 8'h01};
	//phs_lut_table[2] = {1'h0, 4'h3, 4'h2, 4'h3, 4'h2, 4'h1, 8'h6a   ,8'h6a   ,8'h00, 8'h02};
	//phs_lut_table[3] = {1'h1, 4'h3, 4'h2, 4'h3, 4'h2, 4'h1, 8'h20   ,8'h20   ,8'h00, 8'h03};
	//phs_lut_table[4] = {1'h0, 4'h3, 4'h2, 4'h3, 4'h2, 4'h1, 8'h55   ,8'h55   ,8'h00, 8'h04};
	//phs_lut_table[5] = {1'h1, 4'h3, 4'h2, 4'h3, 4'h2, 4'h1, 8'h0a   ,8'h0a   ,8'h00, 8'h05};
	//phs_lut_table[6] = {1'h0, 4'h3, 4'h2, 4'h3, 4'h2, 4'h1, 8'h40   ,8'h40   ,8'h00, 8'h06};
	//phs_lut_table[7] = {1'h0, 4'h3, 4'h2, 4'h3, 4'h2, 4'h1, 8'h75   ,8'h75   ,8'h00, 8'h07};
	//phs_lut_table[8] = {1'h1, 4'h3, 4'h2, 4'h3, 4'h2, 4'h1, 8'h2a   ,8'h2a   ,8'h00, 8'h08};
	//phs_lut_table[9] = {1'h0, 4'h3, 4'h2, 4'h3, 4'h2, 4'h1, 8'h60   ,8'h60   ,8'h00, 8'h09};
	//phs_lut_table[10]= {1'h1, 4'h3, 4'h2, 4'h3, 4'h2, 4'h1, 8'h15   ,8'h15   ,8'h00, 8'h0a};
	//phs_lut_table[11]= {1'h0, 4'h3, 4'h2, 4'h3, 4'h2, 4'h1, 8'h4a   ,8'h4a   ,8'h00, 8'h0b};
		phs_lut_table[0] = 0x13232100350000;
		phs_lut_table[1] = 0x03232135350001;
		phs_lut_table[2] = 0x0323216a6a0002;
		phs_lut_table[3] = 0x13232120200003;
		phs_lut_table[4] = 0x03232155550004;
		phs_lut_table[5] = 0x1323210a0a0005;
		phs_lut_table[6] = 0x03232140400006;
		phs_lut_table[7] = 0x03232175750007;
		phs_lut_table[8] = 0x1323212a2a0008;
		phs_lut_table[9] = 0x03232160600009;
		phs_lut_table[10]= 0x1323211515000a;
		phs_lut_table[11]= 0x0323214a4a000b;
		input_n          = 5;
		output_m         = 12;
	} else if (frc_ratio_mode == FRC_RATIO_1_1) {
		if (film_mode == EN_DRV_VIDEO) {
	//phs_lut_table[0] = {1'h1, 4'h3,4'h2, 4'h3, 4'h2, 4'h1, 8'h00   ,8'h00  ,8'h00, 8'h00};
			phs_lut_table[0] = 0x13232100000000;
		}
		//else if(film_mode == EN_FILM22) {
	//phs_lut_table[0] = {1'h1, 4'h3, 4'h2, 4'h4, 4'h3, 4'h1, 8'd0   ,8'd64   ,8'h01, 8'h00};
	//phs_lut_table[1] = {1'h0, 4'h3, 4'h2, 4'h5, 4'h3, 4'h2, 8'd64  ,8'd64   ,8'h00, 8'h00};
		//}
		else if (film_mode == EN_DRV_FILM32) {
	//phs_lut_table[0] = {1'h1, 4'h3, 4'h2, 4'h5, 4'h4, 4'h1, 8'h00   ,8'h33   ,8'h01, 8'h00};
	//phs_lut_table[1] = {1'h0, 4'h3, 4'h2, 4'h6, 4'h4, 4'h2, 8'h33   ,8'h33   ,8'h02, 8'h00};
	//phs_lut_table[2] = {1'h0, 4'h4, 4'h3, 4'h7, 4'h4, 4'h3, 8'h66   ,8'h66   ,8'h03, 8'h00};
	//phs_lut_table[3] = {1'h1, 4'h3, 4'h2, 4'h5, 4'h4, 4'h2, 8'h19   ,8'h19   ,8'h04, 8'h00};
	//phs_lut_table[4] = {1'h0, 4'h3, 4'h2, 4'h6, 4'h4, 4'h3, 8'h4c   ,8'h4c   ,8'h00, 8'h00};
			phs_lut_table[0] = 0x13254100330100;
			phs_lut_table[1] = 0x03264233330200;
			phs_lut_table[2] = 0x04374366660300;
			phs_lut_table[3] = 0x13254219190400;
			phs_lut_table[4] = 0x0326434c4c0000;
		}
	//else if(film_mode == EN_DRV_FILM3223) {
	//phs_lut_table[0] = { 1'd1, 4'd3, 4'd2, 4'd6, 4'd4, 4'd1, 8'd0,   8'd51, 8'd1, 8'h00};
	//phs_lut_table[1] = { 1'd0, 4'd3, 4'd2, 4'd7, 4'd4, 4'd2, 8'd51,  8'd51, 8'd2, 8'h00};
	//phs_lut_table[2] = { 1'd0, 4'd4, 4'd3, 4'd8, 4'd4, 4'd3, 8'd102, 8'd102, 8'd3, 8'h00};
	//phs_lut_table[3] = { 1'd1, 4'd3, 4'd2, 4'd5, 4'd4, 4'd2, 8'd25,  8'd25, 8'd4, 8'h00};
	//phs_lut_table[4] = { 1'd0, 4'd4, 4'd3, 4'd6, 4'd4, 4'd3, 8'd76,  8'd76, 8'd5, 8'h00};
	//phs_lut_table[5] = { 1'd1, 4'd3, 4'd2, 4'd5, 4'd4, 4'd2, 8'd0,   8'd51, 8'd6, 8'h00};
	//phs_lut_table[6] = { 1'd0, 4'd3, 4'd2, 4'd6, 4'd4, 4'd3, 8'd51,  8'd51, 8'd7, 8'h00};
	//phs_lut_table[7] = { 1'd0, 4'd4, 4'd3, 4'd7, 4'd5, 4'd4, 8'd102, 8'd102, 8'd8, 8'h00};
	//phs_lut_table[8] = { 1'd1, 4'd3, 4'd2, 4'd6, 4'd5, 4'd2, 8'd25,  8'd25, 8'd9, 8'h00};
	//phs_lut_table[9] = { 1'd0, 4'd3, 4'd2, 4'd7, 4'd5, 4'd3, 8'd76,  8'd76, 8'd0, 8'h00};
	//}
	//else if(film_mode == EN_DRV_FILM2224) {
	//phs_lut_table[0] = { 1'd1, 4'd3, 4'd2, 4'd6, 4'd5, 4'd1, 8'd0, 8'd51, 8'd1, 8'h00};
	//phs_lut_table[1] = { 1'd0, 4'd3, 4'd2, 4'd7, 4'd5, 4'd2, 8'd51,   8'd51, 8'd2, 8'h00};
	//phs_lut_table[2] = { 1'd0, 4'd4, 4'd3, 4'd8, 4'd5, 4'd3, 8'd102,  8'd102, 8'd3, 8'h00};
	//phs_lut_table[3] = { 1'd1, 4'd3, 4'd2, 4'd6, 4'd4, 4'd2, 8'd25,   8'd25, 8'd4, 8'h00};
	//phs_lut_table[4] = { 1'd0, 4'd4, 4'd3, 4'd7, 4'd4, 4'd3, 8'd76,   8'd76, 8'd5, 8'h00};
	//phs_lut_table[5] = { 1'd1, 4'd3, 4'd2, 4'd5, 4'd4, 4'd2, 8'd0, 8'd51, 8'd6, 8'h00};
	//phs_lut_table[6] = { 1'd0, 4'd4, 4'd3, 4'd6, 4'd4, 4'd3, 8'd51,   8'd51, 8'd7, 8'h00};
	//phs_lut_table[7] = { 1'd0, 4'd4, 4'd3, 4'd7, 4'd5, 4'd4, 8'd102,  8'd102, 8'd8, 8'h00};
	//phs_lut_table[8] = { 1'd1, 4'd4, 4'd3, 4'd6, 4'd5, 4'd3, 8'd25,   8'd25, 8'd9, 8'h00};
	//phs_lut_table[9] = { 1'd0, 4'd4, 4'd3, 4'd7, 4'd5, 4'd4, 8'd76,   8'd76, 8'd0, 8'h00};
	//}
	//else if(film_mode == EN_DRV_FILM32322) {
	//phs_lut_table[0] =  { 1'd1, 4'd3, 4'd2, 4'd5, 4'd4, 4'd1, 8'd0,   8'd53, 8'd1,  8'h00};
	//phs_lut_table[1] =  { 1'd0, 4'd3, 4'd2, 4'd6, 4'd4, 4'd2, 8'd53,   8'd53, 8'd2, 8'h00};
	//phs_lut_table[2] =  { 1'd0, 4'd4, 4'd3, 4'd7, 4'd4, 4'd3, 8'd106,   8'd106, 8'd3, 8'h00};
	//phs_lut_table[3] =  { 1'd1, 4'd3, 4'd2, 4'd5, 4'd4, 4'd2, 8'd32,   8'd32, 8'd4, 8'h00};
	//phs_lut_table[4] =  { 1'd0, 4'd4, 4'd3, 4'd6, 4'd4, 4'd3, 8'd85,   8'd85, 8'd5, 8'h00};
	//phs_lut_table[5] =  { 1'd1, 4'd3, 4'd2, 4'd5, 4'd4, 4'd2, 8'd10,   8'd10, 8'd6, 8'h00};
	//phs_lut_table[6] =  { 1'd0, 4'd3, 4'd2, 4'd6, 4'd4, 4'd3, 8'd64,   8'd64, 8'd7, 8'h00};
	//phs_lut_table[7] =  { 1'd0, 4'd4, 4'd3, 4'd7, 4'd5, 4'd4, 8'd117,   8'd117, 8'd8, 8'h00};
	//phs_lut_table[8] =  { 1'd1, 4'd3, 4'd2, 4'd6, 4'd5, 4'd2, 8'd42,   8'd42, 8'd9, 8'h00};
	//phs_lut_table[9] =  { 1'd0, 4'd4, 4'd3, 4'd7, 4'd5, 4'd3, 8'd96,   8'd96, 8'd10, 8'h00};
	//phs_lut_table[10] = { 1'd1, 4'd3, 4'd2, 4'd6, 4'd4, 4'd2, 8'd21,   8'd21, 8'd11, 8'h00};
	//phs_lut_table[11] = { 1'd0, 4'd3, 4'd2, 4'd7, 4'd4, 4'd3, 8'd74,   8'd74, 8'd0, 8'h00};
	//}
	//else if(film_mode == EN_DRV_FILM44) {
	//phs_lut_table[0] =  { 1'd1, 4'd3, 4'd2, 4'd6, 4'd5, 4'd1, 8'd0,   8'd32, 8'd1, 8'h00};
	//phs_lut_table[1] =  { 1'd0, 4'd3, 4'd2, 4'd7, 4'd5, 4'd2, 8'd32,   8'd32, 8'd2, 8'h00};
	//phs_lut_table[2] =  { 1'd0, 4'd3, 4'd2, 4'd8, 4'd5, 4'd3, 8'd64,   8'd64, 8'd3, 8'h00};
	//phs_lut_table[3] =  { 1'd0, 4'd3, 4'd2, 4'd9, 4'd5, 4'd4, 8'd96,   8'd96, 8'd0, 8'h00};
	//}
	//else if(film_mode == EN_FILM21111) {
	//phs_lut_table[0] =  { 1'd1, 4'd3, 4'd2, 4'd4, 4'd3, 4'd1, 8'd0,   8'd106, 8'd1, 8'h00};
	//phs_lut_table[1] =  { 1'd0, 4'd4, 4'd3, 4'd5, 4'd3, 4'd2, 8'd106,   8'd106, 8'd2, 8'h00};
	//phs_lut_table[2] =  { 1'd1, 4'd4, 4'd3, 4'd4, 4'd3, 4'd2, 8'd85,   8'd85, 8'd3, 8'h00};
	//phs_lut_table[3] =  { 1'd1, 4'd4, 4'd3, 4'd4, 4'd3, 4'd2, 8'd64,   8'd64, 8'd4, 8'h00};
	//phs_lut_table[4] =  { 1'd1, 4'd4, 4'd3, 4'd4, 4'd3, 4'd2, 8'd42,   8'd42, 8'd5, 8'h00};
	//phs_lut_table[5] =  { 1'd1, 4'd3, 4'd2, 4'd4, 4'd3, 4'd2, 8'd21,   8'd21, 8'd0, 8'h00};
	//}
	//else if(film_mode == EN_DRV_FILM23322) {
	//phs_lut_table[0] =  { 1'd1, 4'd3, 4'd2, 4'd6, 4'd4, 4'd1, 8'd0,   8'd53, 8'd1, 8'h00};
	//phs_lut_table[1] =  { 1'd0, 4'd3, 4'd2, 4'd7, 4'd4, 4'd2, 8'd53,   8'd53, 8'd2,   8'h00};
	//phs_lut_table[2] =  { 1'd0, 4'd4, 4'd3, 4'd8, 4'd4, 4'd3, 8'd106,   8'd106, 8'd3, 8'h00};
	//phs_lut_table[3] =  { 1'd1, 4'd3, 4'd2, 4'd5, 4'd4, 4'd2, 8'd32,   8'd32, 8'd4,   8'h00};
	//phs_lut_table[4] =  { 1'd0, 4'd4, 4'd3, 4'd6, 4'd4, 4'd3, 8'd85,   8'd85, 8'd5,   8'h00};
	//phs_lut_table[5] =  { 1'd1, 4'd3, 4'd2, 4'd5, 4'd4, 4'd2, 8'd10,   8'd10, 8'd6,   8'h00};
	//phs_lut_table[6] =  { 1'd0, 4'd4, 4'd3, 4'd6, 4'd4, 4'd3, 8'd64,   8'd64, 8'd7,   8'h00};
	//phs_lut_table[7] =  { 1'd0, 4'd4, 4'd3, 4'd7, 4'd5, 4'd4, 8'd117,   8'd117, 8'd8, 8'h00};
	//phs_lut_table[8] =  { 1'd1, 4'd3, 4'd2, 4'd6, 4'd5, 4'd3, 8'd42,   8'd42, 8'd9,   8'h00};
	//phs_lut_table[9] =  { 1'd0, 4'd4, 4'd3, 4'd7, 4'd5, 4'd4, 8'd96,   8'd96, 8'd10, 8'h00};
	//phs_lut_table[10] = { 1'd1, 4'd3, 4'd2, 4'd6, 4'd5, 4'd2, 8'd21,   8'd21, 8'd11, 8'h00};
	//phs_lut_table[11] = { 1'd0, 4'd3, 4'd2, 4'd7, 4'd5, 4'd3, 8'd74,   8'd74, 8'd0,  'h00};
	//}
	//else if(film_mode == EN_DRV_FILM2111) {
	//phs_lut_table[0] =  { 1'd1, 4'd3, 4'd2, 4'd4, 4'd3, 4'd1, 8'd0,   8'd102, 8'd1,   8'h00};
	//phs_lut_table[1] =  { 1'd0, 4'd4, 4'd3, 4'd5, 4'd3, 4'd2, 8'd102,   8'd102, 8'd2, 8'h00};
	//phs_lut_table[2] =  { 1'd1, 4'd4, 4'd3, 4'd4, 4'd3, 4'd2, 8'd76,   8'd76, 8'd3,   8'h00};
	//phs_lut_table[3] =  { 1'd1, 4'd4, 4'd3, 4'd4, 4'd3, 4'd2, 8'd51,   8'd51, 8'd4,   8'h00};
	//phs_lut_table[4] =  { 1'd1, 4'd3, 4'd2, 4'd4, 4'd3, 4'd2, 8'd25,   8'd25, 8'd0,   8'h00};
	//}

	//else if(film_mode == EN_DRV_FILM22224) {
	//    //cadence=22224(10),table_cnt=  12,match_data=0xaa8
	//phs_lut_table[0]={ 1'd1, 4'd3, 4'd2, 4'd6, 4'd5, 4'd1, 8'd0,  8'd53, 8'd1, 8'd0};
	//phs_lut_table[1]={ 1'd0, 4'd3, 4'd2, 4'd7, 4'd5, 4'd2, 8'd53, 8'd53, 8'd2, 8'd0};
	//phs_lut_table[2]={ 1'd0, 4'd4, 4'd3, 4'd8, 4'd5, 4'd3, 8'd106, 8'd106, 8'd3, 8'd0};
	//phs_lut_table[3]={ 1'd1, 4'd3, 4'd2, 4'd6, 4'd4, 4'd2, 8'd32, 8'd32, 8'd4, 8'd0};
	//phs_lut_table[4]={ 1'd0, 4'd4, 4'd3, 4'd7, 4'd4, 4'd3, 8'd85, 8'd85, 8'd5, 8'd0};
	//phs_lut_table[5]={ 1'd1, 4'd3, 4'd2, 4'd5, 4'd4, 4'd2, 8'd10, 8'd10, 8'd6, 8'd0};
	//phs_lut_table[6]={ 1'd0, 4'd4, 4'd3, 4'd6, 4'd4, 4'd3, 8'd64, 8'd64, 8'd7, 8'd0};
	//phs_lut_table[7]={ 1'd0, 4'd4, 4'd3, 4'd7, 4'd5, 4'd4, 8'd117, 8'd117, 8'd8, 8'd0};
	//phs_lut_table[8]={ 1'd1, 4'd4, 4'd3, 4'd6, 4'd5, 4'd3, 8'd42, 8'd42, 8'd9, 8'd0};
	//phs_lut_table[9]={ 1'd0, 4'd4, 4'd3, 4'd7, 4'd5, 4'd4, 8'd96, 8'd96, 8'd10, 8'd0};
	//phs_lut_table[10]={ 1'd1, 4'd3, 4'd2, 4'd6, 4'd5, 4'd3, 8'd21, 8'd21, 8'd11, 8'd0};
	//phs_lut_table[11]={ 1'd0, 4'd3, 4'd2, 4'd7, 4'd5, 4'd4, 8'd74, 8'd74, 8'd0, 8'd0};
	//}
	//else if(film_mode == EN_DRV_FILM33) {
	//    //cadence=33(11),table_cnt=   3,match_data=0x4
	//phs_lut_table[0]={ 1'd1, 4'd3, 4'd2, 4'd5, 4'd4, 4'd1, 8'd0, 8'd42, 8'd1, 8'd0};
	//phs_lut_table[1]={ 1'd0, 4'd3, 4'd2, 4'd6, 4'd4, 4'd2, 8'd42, 8'd42, 8'd2, 8'd0};
	//phs_lut_table[2]={ 1'd0, 4'd3, 4'd2, 4'd7, 4'd4, 4'd3, 8'd85, 8'd85, 8'd0, 8'd0};
	//}
	//else if(film_mode == EN_DRV_FILM334) {
	//    //cadence=334(12),table_cnt=  10,match_data=0x248
	//phs_lut_table[0]={ 1'd1, 4'd3, 4'd2, 4'd6, 4'd5, 4'd1, 8'd0,   8'd38, 8'd1, 8'd0};
	//phs_lut_table[1]={ 1'd0, 4'd3, 4'd2, 4'd7, 4'd5, 4'd2, 8'd38,   8'd38, 8'd2, 8'd0};
	//phs_lut_table[2]={ 1'd0, 4'd3, 4'd2, 4'd8, 4'd5, 4'd3, 8'd76,   8'd76, 8'd3, 8'd0};
	//phs_lut_table[3]={ 1'd0, 4'd4, 4'd3, 4'd9, 4'd5, 4'd4, 8'd115,   8'd115, 8'd4, 8'd0};
	//phs_lut_table[4]={ 1'd1, 4'd3, 4'd2, 4'd6, 4'd5, 4'd2, 8'd25,   8'd25, 8'd5, 8'd0};
	//phs_lut_table[5]={ 1'd0, 4'd3, 4'd2, 4'd7, 4'd5, 4'd3, 8'd64,   8'd64, 8'd6, 8'd0};
	//phs_lut_table[6]={ 1'd0, 4'd4, 4'd3, 4'd8, 4'd5, 4'd4, 8'd102,   8'd102, 8'd7, 8'd0};
	//phs_lut_table[7]={ 1'd1, 4'd3, 4'd2, 4'd6, 4'd5, 4'd2, 8'd12,   8'd12, 8'd8, 8'd0};
	//phs_lut_table[8]={ 1'd0, 4'd3, 4'd2, 4'd7, 4'd5, 4'd3, 8'd51,   8'd51, 8'd9, 8'd0};
	//phs_lut_table[9]={ 1'd0, 4'd3, 4'd2, 4'd8, 4'd5, 4'd4, 8'd89,   8'd89, 8'd0, 8'd0};
	//
	//}
	//else if(film_mode == EN_DRV_FILM55) {
	//    //cadence=55(13),table_cnt=   5,match_data=0x10
	//phs_lut_table[0]={ 1'd1, 4'd3, 4'd2, 4'd7, 4'd6, 4'd1, 8'd0, 8'd25, 8'd1, 8'd0};
	//phs_lut_table[1]={ 1'd0, 4'd3, 4'd2, 4'd8, 4'd6, 4'd2, 8'd25,   8'd25, 8'd2, 8'd0};
	//phs_lut_table[2]={ 1'd0, 4'd3, 4'd2, 4'd9, 4'd6, 4'd3, 8'd51,   8'd51, 8'd3, 8'd0};
	//phs_lut_table[3]={ 1'd0, 4'd3, 4'd2, 4'd10, 4'd6, 4'd4, 8'd76,   8'd76, 8'd4, 8'd0};
	//phs_lut_table[4]={ 1'd0, 4'd3, 4'd2, 4'd11, 4'd6, 4'd5, 8'd102,  8'd102, 8'd0, 8'd0};
	//
	//}
	//else if(film_mode == EN_DRV_FILM64) {
	//    //cadence=64(14),table_cnt=  10,match_data=0x220
	//phs_lut_table[0]={ 1'd1, 4'd3, 4'd2, 4'd8, 4'd7, 4'd1, 8'd0, 8'd25, 8'd1, 8'd0};
	//phs_lut_table[1]={ 1'd0, 4'd3, 4'd2, 4'd9, 4'd7, 4'd2, 8'd25,   8'd25, 8'd2, 8'd0};
	//phs_lut_table[2]={ 1'd0, 4'd3, 4'd2, 4'd10, 4'd7, 4'd3, 8'd51,   8'd51, 8'd3, 8'd0};
	//phs_lut_table[3]={ 1'd0, 4'd3, 4'd2, 4'd11, 4'd7, 4'd4, 8'd76,   8'd76, 8'd4, 8'd0};
	//phs_lut_table[4]={ 1'd0, 4'd4, 4'd3, 4'd12, 4'd7, 4'd5, 8'd102,  8'd102, 8'd5, 8'd0};
	//phs_lut_table[5]={ 1'd1, 4'd3, 4'd2, 4'd8, 4'd6, 4'd2, 8'd0, 8'd25, 8'd6, 8'd0};
	//phs_lut_table[6]={ 1'd0, 4'd3, 4'd2, 4'd9, 4'd6, 4'd3, 8'd25,   8'd25, 8'd7, 8'd0};
	//phs_lut_table[7]={ 1'd0, 4'd3, 4'd2, 4'd10, 4'd6, 4'd4, 8'd51,   8'd51, 8'd8, 8'd0};
	//phs_lut_table[8]={ 1'd0, 4'd3, 4'd2, 4'd11, 4'd6, 4'd5, 8'd76,   8'd76, 8'd9, 8'd0};
	//phs_lut_table[9]={ 1'd0, 4'd3, 4'd2, 4'd12, 4'd7, 4'd6, 8'd102,  8'd102, 8'd0, 8'd0};
	//}
	//else if(film_mode == EN_DRV_FILM66) {
	//    //cadence=66(15),table_cnt=   6,match_data=0x20
	//phs_lut_table[0]={ 1'd1, 4'd3, 4'd2, 4'd8, 4'd7, 4'd1, 8'd0,   8'd21, 8'd1, 8'd0};
	//phs_lut_table[1]={ 1'd0, 4'd3, 4'd2, 4'd9, 4'd7, 4'd2, 8'd21,   8'd21, 8'd2, 8'd0};
	//phs_lut_table[2]={ 1'd0, 4'd3, 4'd2, 4'd10, 4'd7, 4'd3, 8'd42,   8'd42, 8'd3, 8'd0};
	//phs_lut_table[3]={ 1'd0, 4'd3, 4'd2, 4'd11, 4'd7, 4'd4, 8'd64,   8'd64, 8'd4, 8'd0};
	//phs_lut_table[4]={ 1'd0, 4'd3, 4'd2, 4'd12, 4'd7, 4'd5, 8'd85,   8'd85, 8'd5, 8'd0};
	//phs_lut_table[5]={ 1'd0, 4'd3, 4'd2, 4'd13, 4'd7, 4'd6, 8'd106,   8'd106, 8'd0, 8'd0};
	//}
	//else if(film_mode == EN_DRV_FILM87) {
	//    //cadence=87(16),table_cnt=  15,match_data=0x4080
	//phs_lut_table[0]= { 1'd1, 4'd3, 4'd2, 4'd4, 4'd10, 4'd0, 8'd0, 8'd17, 8'd1, 8'd0};
	//phs_lut_table[1]= { 1'd0, 4'd3, 4'd2, 4'd4, 4'd1, 4'd0, 8'd17,   8'd17, 8'd2, 8'd0};
	//phs_lut_table[2]= { 1'd0, 4'd3, 4'd2, 4'd4, 4'd11, 4'd0, 8'd34,   8'd34, 8'd3, 8'd0};
	//phs_lut_table[3]= { 1'd0, 4'd3, 4'd2, 4'd4, 4'd5, 4'd0, 8'd51,   8'd51, 8'd4, 8'd0};
	//phs_lut_table[4]= { 1'd0, 4'd3, 4'd2, 4'd4, 4'd14, 4'd0, 8'd68,   8'd68, 8'd5, 8'd0};
	//phs_lut_table[5]= { 1'd0, 4'd3, 4'd2, 4'd13, 4'd2, 4'd9, 8'd85,   8'd85, 8'd6, 8'd0};
	//phs_lut_table[6]= { 1'd0, 4'd3, 4'd2, 4'd7, 4'd6, 4'd3, 8'd102,  8'd102,   8'd7, 8'd0};
	//phs_lut_table[7]= { 1'd0, 4'd4, 4'd3, 4'd1, 4'd11, 4'd12, 8'd119,  8'd119,   8'd8, 8'd0};
	//phs_lut_table[8]= { 1'd1, 4'd3, 4'd2, 4'd1, 4'd2, 4'd14, 8'd8, 8'd8, 8'd9, 8'd0};
	//phs_lut_table[9]= { 1'd0, 4'd3, 4'd2, 4'd7, 4'd8, 4'd5, 8'd25,   8'd25, 8'd10, 8'd0};
	//phs_lut_table[10]={ 1'd0, 4'd3, 4'd2, 4'd13, 4'd14, 4'd11, 8'd42,   8'd42, 8'd11, 8'd0};
	//phs_lut_table[11]={ 1'd0, 4'd3, 4'd2, 4'd4, 4'd5, 4'd2, 8'd59,   8'd59, 8'd12, 8'd0};
	//phs_lut_table[12]={ 1'd0, 4'd3, 4'd2, 4'd9, 4'd10, 4'd7, 8'd76,   8'd76, 8'd13, 8'd0};
	//phs_lut_table[13]={ 1'd0, 4'd3, 4'd2, 4'd13, 4'd14, 4'd11, 8'd93,   8'd93, 8'd14, 8'd0};
	//phs_lut_table[14]={ 1'd0, 4'd3, 4'd12, 4'd12, 4'd4, 4'd10, 8'd110,  8'd110,   8'd0, 8'd0};
	//
	//phs_lut_table[0]= { 1'd1, 4'd3, 4'd2, 4'd8, 4'd5, 4'd2, 8'd0, 8'd17, 8'd1, 8'd0};
	//phs_lut_table[1]= { 1'd0, 4'd3, 4'd2, 4'd10, 4'd7, 4'd4, 8'd17,   8'd17, 8'd2, 8'd0};
	//phs_lut_table[2]= { 1'd0, 4'd3, 4'd2, 4'd12, 4'd9, 4'd6, 8'd34,   8'd34, 8'd3, 8'd0};
	//phs_lut_table[3]= { 1'd0, 4'd3, 4'd2, 4'd14, 4'd11, 4'd8, 8'd51,   8'd51, 8'd4, 8'd0};
	//phs_lut_table[4]= { 1'd0, 4'd3, 4'd2, 4'd1, 4'd14, 4'd11, 8'd68,   8'd68, 8'd5, 8'd0};
	//phs_lut_table[5]= { 1'd0, 4'd3, 4'd2, 4'd5, 4'd2, 4'd15, 8'd85,   8'd85, 8'd6, 8'd0};
	//phs_lut_table[6]= { 1'd0, 4'd3, 4'd2, 4'd8, 4'd5, 4'd2, 8'd102,  8'd102,   8'd7, 8'd0};
	//phs_lut_table[7]= { 1'd0, 4'd4, 4'd3, 4'd10, 4'd7, 4'd4, 8'd119,  8'd119,   8'd8, 8'd0};
	//phs_lut_table[8]= { 1'd1, 4'd3, 4'd2, 4'd9, 4'd7, 4'd4, 8'd8, 8'd8, 8'd9, 8'd0};
	//phs_lut_table[9]= { 1'd0, 4'd3, 4'd2, 4'd11, 4'd9, 4'd6, 8'd25,   8'd25, 8'd10, 8'd0};
	//phs_lut_table[10]={ 1'd0, 4'd3, 4'd2, 4'd13, 4'd11, 4'd8, 8'd42,   8'd42, 8'd11, 8'd0};
	//phs_lut_table[11]={ 1'd0, 4'd3, 4'd2, 4'd15, 4'd13, 4'd10, 8'd59,   8'd59, 8'd12, 8'd0};
	//phs_lut_table[12]={ 1'd0, 4'd3, 4'd2, 4'd3, 4'd1, 4'd14, 8'd76,   8'd76, 8'd13, 8'd0};
	//phs_lut_table[13]={ 1'd0, 4'd3, 4'd2, 4'd5, 4'd3, 4'd2, 8'd93,   8'd93, 8'd14, 8'd0};
	//phs_lut_table[14]={ 1'd0, 4'd3, 4'd12, 4'd8, 4'd6, 4'd3, 8'd110,  8'd110,   8'd0, 8'd0};
	//}
	//else if(film_mode == EN_DRV_FILM212) {
	//    //cadence=212(17),table_cnt=   5,match_data=0x1a
	//phs_lut_table[0]={ 1'd1, 4'd3, 4'd2, 4'd5, 4'd3, 4'd1, 8'd0,   8'd76,      8'd1, 8'd0};
	//phs_lut_table[1]={ 1'd0, 4'd4, 4'd3, 4'd6, 4'd3, 4'd2, 8'd76,   8'd76, 8'd2, 8'd0};
	//phs_lut_table[2]={ 1'd1, 4'd3, 4'd2, 4'd4, 4'd3, 4'd2, 8'd25,   8'd25, 8'd3, 8'd0};
	//phs_lut_table[3]={ 1'd0, 4'd4, 4'd3, 4'd5, 4'd4, 4'd3, 8'd102,   8'd102,   8'd4, 8'd0};
	//phs_lut_table[4]={ 1'd1, 4'd3, 4'd2, 4'd5, 4'd4, 4'd2, 8'd51,   8'd51, 8'd0, 8'd0};
	//}
	//else {
	//      stimulus_print ("==== USE ERROR CADENCE ====");
	//}
		input_n          = 1;
		output_m         = 1;
	}
	regdata_top_ctl_0011 = READ_FRC_REG(FRC_REG_TOP_CTRL17);
	frc_config_reg_value(0x08, 0x18, &regdata_top_ctl_0011); //lut_cfg_en
	WRITE_FRC_REG_BY_CPU(FRC_REG_TOP_CTRL17, regdata_top_ctl_0011);
	WRITE_FRC_REG_BY_CPU(FRC_TOP_LUT_ADDR, 0);
	// for (i = 0; i < 256; i++) {
	for (i = 0; i < lut_ary_size + 2; i++) {

		if (i < lut_ary_size) {
			WRITE_FRC_REG_BY_CPU(FRC_TOP_LUT_DATA, phs_lut_table[i]       & 0xffffffff);
			WRITE_FRC_REG_BY_CPU(FRC_TOP_LUT_DATA, phs_lut_table[i] >> 32 & 0xffffffff);
		} else {
			WRITE_FRC_REG_BY_CPU(FRC_TOP_LUT_DATA, 0xffffffff);
			WRITE_FRC_REG_BY_CPU(FRC_TOP_LUT_DATA, 0xffffffff);
		}
	}
	frc_config_reg_value(0x10, 0x18, &regdata_top_ctl_0011); //lut_cfg_en
	WRITE_FRC_REG_BY_CPU(FRC_REG_TOP_CTRL17, regdata_top_ctl_0011);

	tmpregdata = input_n << 24 | output_m << 16;
	frc_config_reg_value(tmpregdata, 0xffff0000, &regdata_phs_tab_0116);
	WRITE_FRC_REG_BY_CPU(FRC_REG_PHS_TABLE, regdata_phs_tab_0116);

}

/*get new frame mode*/
void config_phs_regs(enum frc_ratio_mode_type frc_ratio_mode,
	enum en_drv_film_mode film_mode)
{
	u32            reg_out_frm_dly_num = 3;
	u32            reg_frc_pd_pat_num  = 1;
	u32            reg_ip_film_end     = 1;
	u64            inp_frm_vld_lut     = 1;
	u32            reg_ip_incr[16]     = {0,1,3,1,1,1,1,1,2,3,2,1,1,1,1,2};
	u32            record_value = 0;
	//config ip_film_end
	if(frc_ratio_mode == FRC_RATIO_1_1) {
		inp_frm_vld_lut = 1;

		if (film_mode == EN_DRV_VIDEO) {
			reg_ip_film_end     = 1;
			reg_frc_pd_pat_num  = 1;
			reg_out_frm_dly_num = 3;   // 10
		} else if (film_mode == EN_DRV_FILM22) {
			reg_ip_film_end    = 0x2;
			reg_frc_pd_pat_num = 2;
			reg_out_frm_dly_num = 7;
		} else if (film_mode == EN_DRV_FILM32) {
			reg_ip_film_end    = 0x12;
			reg_frc_pd_pat_num = 5;
			reg_out_frm_dly_num = 10;
		} else if (film_mode == EN_DRV_FILM3223) {
			reg_ip_film_end    = 0x24a;
			reg_frc_pd_pat_num = 10;
			reg_out_frm_dly_num = 15;
		} else if (film_mode == EN_DRV_FILM2224) {
			reg_ip_film_end    = 0x22a;
			reg_frc_pd_pat_num = 10;
			reg_out_frm_dly_num = 15;
		} else if (film_mode == EN_DRV_FILM32322) {
			reg_ip_film_end    = 0x94a;
			reg_frc_pd_pat_num = 12;
			reg_out_frm_dly_num = 15;
		} else if (film_mode == EN_DRV_FILM44) {
			reg_ip_film_end    = 0x8;
			reg_frc_pd_pat_num = 4;
			reg_out_frm_dly_num = 14;
		} else if (film_mode == EN_DRV_FILM21111) {
			reg_ip_film_end    = 0x2f;
			reg_frc_pd_pat_num = 6;
			reg_out_frm_dly_num = 10;
		} else if (film_mode == EN_DRV_FILM23322) {
			reg_ip_film_end    = 0x92a;
			reg_frc_pd_pat_num = 12;
			reg_out_frm_dly_num = 15;
		} else if (film_mode == EN_DRV_FILM2111) {
			reg_ip_film_end    = 0x17;
			reg_frc_pd_pat_num = 5;
			reg_out_frm_dly_num = 10;
		} else if (film_mode == EN_DRV_FILM22224) {
			reg_ip_film_end    = 0x8aa;
			reg_frc_pd_pat_num = 12;
			reg_out_frm_dly_num = 15;
		} else if (film_mode == EN_DRV_FILM33) {
			reg_ip_film_end    = 0x4;
			reg_frc_pd_pat_num = 3;
			reg_out_frm_dly_num = 11;
		} else if (film_mode == EN_DRV_FILM334) {
			reg_ip_film_end    = 0x224;
			reg_frc_pd_pat_num = 10;
			reg_out_frm_dly_num = 15;
		} else if (film_mode == EN_DRV_FILM55) {
			reg_ip_film_end    = 0x10;
			reg_frc_pd_pat_num = 5;
			reg_out_frm_dly_num = 15;
		} else if (film_mode == EN_DRV_FILM64) {
			reg_ip_film_end    = 0x208;
			reg_frc_pd_pat_num = 10;
			reg_out_frm_dly_num = 15;
		} else if (film_mode == EN_DRV_FILM66) {
			reg_ip_film_end    = 0x20;
			reg_frc_pd_pat_num = 6;
			reg_out_frm_dly_num = 15;
		} else if (film_mode == EN_DRV_FILM87) {
			 //memc need pre-load 3 different frames, so at least input 8+7+8=23 frames,
			 //but memc lbuf number max is 16<23, so skip some repeated frames
			reg_ip_film_end     = 0x4040;
			reg_frc_pd_pat_num  = 15;
			reg_out_frm_dly_num = 15;
		} else if (film_mode == EN_DRV_FILM212) {
			reg_ip_film_end    = 0x15;
			reg_frc_pd_pat_num = 5;
			reg_out_frm_dly_num = 15;
		} else if (film_mode == EN_DRV_FILM_MAX) {
			reg_ip_film_end = 0xffff;  // fix deadcode
		} else {
			pr_frc(0, "==== USE ERROR CADENCE ====\n");
		}
	} else {
		reg_ip_film_end    = 0x1;
		reg_frc_pd_pat_num = 1;
		if (frc_ratio_mode == FRC_RATIO_1_2)
			inp_frm_vld_lut = 0x2;
		else if (frc_ratio_mode == FRC_RATIO_2_3)
			inp_frm_vld_lut = 0x6;
		else if (frc_ratio_mode == FRC_RATIO_2_5)
			inp_frm_vld_lut = 0x14;
		else if (frc_ratio_mode == FRC_RATIO_5_6)
			inp_frm_vld_lut = 0x3e;
		else if (frc_ratio_mode == FRC_RATIO_5_12)
			inp_frm_vld_lut = 0xa94;
	}
	// input [63:0] reg_inp_frm_vld_lut;
	WRITE_FRC_REG_BY_CPU(FRC_REG_TOP_CTRL18, (inp_frm_vld_lut >> 0) & 0xffffffff);
	WRITE_FRC_REG_BY_CPU(FRC_REG_TOP_CTRL19, (inp_frm_vld_lut >> 32) & 0xffffffff);
	WRITE_FRC_REG_BY_CPU(FRC_REG_PD_PAT_NUM, reg_frc_pd_pat_num);

	// frc_config_reg_value(reg_out_frm_dly_num << 24,
	//					0xF000000, &regdata_top_ctl_0009);
	// WRITE_FRC_REG_BY_CPU(FRC_REG_TOP_CTRL9, regdata_top_ctl_0009);
	regdata_loadorgframe[0] = READ_FRC_REG(FRC_REG_LOAD_ORG_FRAME_0);
	regdata_loadorgframe[1] = READ_FRC_REG(FRC_REG_LOAD_ORG_FRAME_1);
	regdata_loadorgframe[2] = READ_FRC_REG(FRC_REG_LOAD_ORG_FRAME_2);
	regdata_loadorgframe[3] = READ_FRC_REG(FRC_REG_LOAD_ORG_FRAME_3);
	record_value  = 0x0;
	if ((reg_ip_film_end >> 0) & 0x1)
		record_value |= 0x08000000;
	if ((reg_ip_film_end >> 1) & 0x1)
		record_value |= 0x00080000;
	if ((reg_ip_film_end >> 2) & 0x1)
		record_value |= 0x00000800;
	if ((reg_ip_film_end >> 3) & 0x1)
		record_value |= 0x00000008;
	frc_config_reg_value(record_value, 0x08080808, &regdata_loadorgframe[0]);
	record_value  = 0x0;
	if ((reg_ip_film_end >> 4) & 0x1)
		record_value |= 0x08000000;
	if ((reg_ip_film_end >> 5) & 0x1)
		record_value |= 0x00080000;
	if ((reg_ip_film_end >> 6) & 0x1)
		record_value |= 0x00000800;
	if ((reg_ip_film_end >> 7) & 0x1)
		record_value |= 0x00000008;
	frc_config_reg_value(record_value, 0x08080808,	&regdata_loadorgframe[1]);
	record_value  = 0x0;
	if ((reg_ip_film_end >> 8) & 0x1)
		record_value |= 0x08000000;
	if ((reg_ip_film_end >> 9) & 0x1)
		record_value |= 0x00080000;
	if ((reg_ip_film_end >> 10) & 0x1)
		record_value |= 0x00000800;
	if ((reg_ip_film_end >> 11) & 0x1)
		record_value |= 0x00000008;
	frc_config_reg_value(record_value, 0x08080808,	&regdata_loadorgframe[2]);
	record_value  = 0x0;
	if ((reg_ip_film_end >> 12) & 0x1)
		record_value |= 0x08000000;
	if ((reg_ip_film_end >> 13) & 0x1)
		record_value |= 0x00080000;
	if ((reg_ip_film_end >> 14) & 0x1)
		record_value |= 0x00000800;
	if ((reg_ip_film_end >> 15) & 0x1)
		record_value |= 0x00000008;
	frc_config_reg_value(record_value, 0x08080808,	&regdata_loadorgframe[3]);

	if (film_mode == EN_DRV_FILM87 && frc_ratio_mode == FRC_RATIO_1_1) {
		frc_config_reg_value(reg_ip_incr[0] << 28, 0xF0000000, &regdata_loadorgframe[0]);
		frc_config_reg_value(reg_ip_incr[1] << 20, 0x00F00000, &regdata_loadorgframe[0]);
		frc_config_reg_value(reg_ip_incr[2] << 12, 0x0000F000, &regdata_loadorgframe[0]);
		frc_config_reg_value(reg_ip_incr[3] << 4,  0x000000F0, &regdata_loadorgframe[0]);

		frc_config_reg_value(reg_ip_incr[4] << 28, 0xF0000000, &regdata_loadorgframe[1]);
		frc_config_reg_value(reg_ip_incr[5] << 20, 0x00F00000, &regdata_loadorgframe[1]);
		frc_config_reg_value(reg_ip_incr[6] << 12, 0x0000F000, &regdata_loadorgframe[1]);
		frc_config_reg_value(reg_ip_incr[7] << 4,  0x000000F0, &regdata_loadorgframe[1]);

		frc_config_reg_value(reg_ip_incr[8] << 28,  0xF0000000, &regdata_loadorgframe[2]);
		frc_config_reg_value(reg_ip_incr[9] << 20,  0x00F00000, &regdata_loadorgframe[2]);
		frc_config_reg_value(reg_ip_incr[10] << 12, 0x0000F000, &regdata_loadorgframe[2]);
		frc_config_reg_value(reg_ip_incr[11] << 4,  0x000000F0, &regdata_loadorgframe[2]);

		frc_config_reg_value(reg_ip_incr[12] << 28, 0xF0000000, &regdata_loadorgframe[3]);
		frc_config_reg_value(reg_ip_incr[13] << 20, 0x00F00000, &regdata_loadorgframe[3]);
		frc_config_reg_value(reg_ip_incr[14] << 12, 0x0000F000, &regdata_loadorgframe[3]);
		frc_config_reg_value(reg_ip_incr[15] << 4,  0x000000F0, &regdata_loadorgframe[3]);
	} else {
		frc_config_reg_value(0, 0xF0000000, &regdata_loadorgframe[0]);
		frc_config_reg_value(0, 0x00F00000, &regdata_loadorgframe[0]);
		frc_config_reg_value(0, 0x0000F000, &regdata_loadorgframe[0]);
		frc_config_reg_value(0, 0x000000F0, &regdata_loadorgframe[0]);

		frc_config_reg_value(0, 0xF0000000, &regdata_loadorgframe[1]);
		frc_config_reg_value(0, 0x00F00000, &regdata_loadorgframe[1]);
		frc_config_reg_value(0, 0x0000F000, &regdata_loadorgframe[1]);
		frc_config_reg_value(0, 0x000000F0, &regdata_loadorgframe[1]);

		frc_config_reg_value(0, 0xF0000000, &regdata_loadorgframe[2]);
		frc_config_reg_value(0, 0x00F00000, &regdata_loadorgframe[2]);
		frc_config_reg_value(0, 0x0000F000, &regdata_loadorgframe[2]);
		frc_config_reg_value(0, 0x000000F0, &regdata_loadorgframe[2]);

		frc_config_reg_value(0, 0xF0000000, &regdata_loadorgframe[3]);
		frc_config_reg_value(0, 0x00F00000, &regdata_loadorgframe[3]);
		frc_config_reg_value(0, 0x0000F000, &regdata_loadorgframe[3]);
		frc_config_reg_value(0, 0x000000F0, &regdata_loadorgframe[3]);
	}
	WRITE_FRC_REG_BY_CPU(FRC_REG_LOAD_ORG_FRAME_0, regdata_loadorgframe[0]);
	WRITE_FRC_REG_BY_CPU(FRC_REG_LOAD_ORG_FRAME_1, regdata_loadorgframe[1]);
	WRITE_FRC_REG_BY_CPU(FRC_REG_LOAD_ORG_FRAME_2, regdata_loadorgframe[2]);
	WRITE_FRC_REG_BY_CPU(FRC_REG_LOAD_ORG_FRAME_3, regdata_loadorgframe[3]);
}
EXPORT_SYMBOL(config_phs_regs);

void config_me_top_hw_reg(void)
{
	u32    reg_data;
	u32    mvx_div_mode;
	u32    mvy_div_mode;
	u32    me_max_mvx  ;
	u32    me_max_mvy  ;

	reg_data = READ_FRC_REG(FRC_REG_BLK_SIZE_XY);
	// reg_data = regdata_blksizexy_012b;
	mvx_div_mode = (reg_data>>14) & 0x3;
	mvy_div_mode = (reg_data>>12) & 0x3;

	me_max_mvx   = (MAX_ME_MVX << mvx_div_mode  < (1<<(ME_MVX_BIT-1))-1) ? (MAX_ME_MVX << mvx_div_mode) :  ((1<<(ME_MVX_BIT-1))-1);
	me_max_mvy   = (MAX_ME_MVY << mvy_div_mode  < (1<<(ME_MVY_BIT-1))-1) ? (MAX_ME_MVY << mvy_div_mode) :  ((1<<(ME_MVY_BIT-1))-1);

	WRITE_FRC_REG_BY_CPU(FRC_ME_CMV_MAX_MV, (me_max_mvx << 16) | me_max_mvy);

	WRITE_FRC_REG_BY_CPU(FRC_ME_CMV_CTRL, 0x800000ff);
	WRITE_FRC_REG_BY_CPU(FRC_ME_CMV_CTRL, 0x000000ff);
}

void config_loss_out(u32 fmt422)
{
#ifdef LOSS_TEST
	WRITE_FRC_BITS(CLOSS_MISC, 1, 0, 1); //reg_enable
	WRITE_FRC_BITS(CLOSS_MISC, fmt422, 1, 1); //reg_inp_422;   1: 422, 0:444

	WRITE_FRC_BITS(CLOSS_MISC, (6 << 8) | (7 << 16), 12, 20); //reg_misc
	printf("config_loss_out\n");
#endif
}

void enable_nr(void)
{
	u32  reg_data;
	u32  reg_nr_misc_data;
	u32 offset = 0x0;
	enum chip_id chip;

	chip = get_chip_type();

	if (chip == ID_T3)
		offset = 0xa0;
	else if (chip == ID_T5M || chip == ID_T3X)
		offset = 0x0;

	WRITE_FRC_BITS(FRC_REG_INP_MODULE_EN + offset, 1, 9, 1); //reg_mc_nr_en
	reg_data = READ_FRC_REG(FRC_NR_MISC);
	reg_nr_misc_data = reg_data & 0x00000001;
	WRITE_FRC_BITS(FRC_NR_MISC, reg_nr_misc_data, 0, 32); //reg_nr_misc
}

void enable_bbd(void)
{
	u32 offset = 0x0;
	enum chip_id chip;

	chip = get_chip_type();

	if (chip == ID_T3)
		offset = 0xa0;
	else if (chip == ID_T5M || chip == ID_T3X)
		offset = 0x0;

	// WRITE_FRC_BITS(FRC_REG_INP_MODULE_EN + offset, 1, 7, 1); //reg_inp_bbd_en
	frc_config_reg_value(0x80, 0x80, &regdata_inpmoden_04f9);
	WRITE_FRC_REG_BY_CPU(FRC_REG_INP_MODULE_EN + offset, regdata_inpmoden_04f9);
}

void frc_cfg_memc_loss(u32 memc_loss_en)
{
	u32 memcloss = READ_FRC_REG(FRC_REG_TOP_CTRL11); // 0x0404000c;

	if (memc_loss_en > 3)
		memc_loss_en = 3;
	memcloss &= 0xFFFFFFFC;
	memcloss |= memc_loss_en;
	WRITE_FRC_REG_BY_CPU(FRC_REG_TOP_CTRL11, memcloss);
}

void frc_cfg_mcdw_loss(u32 mcdw_loss_en)
{
	u32 temp = 0;

	temp = READ_FRC_REG(FRC_INP_MCDW_CTRL);
	if (mcdw_loss_en == 1)
		temp |= 0x10;
	else if (mcdw_loss_en == 0)
		temp &= 0xFFFFFFEF;
	WRITE_FRC_REG_BY_CPU(FRC_INP_MCDW_CTRL, temp);
	pr_frc(2, "%s FRC_INP_MCDW_CTRL=0x%x\n", __func__,
				READ_FRC_REG(FRC_INP_MCDW_CTRL));

}

void frc_force_secure(u32 onoff)
{
	if (onoff)
		WRITE_FRC_REG_BY_CPU(FRC_MC_LOSS_SLICE_SEC, 1);//reg_mc_loss_slice_sec
	else
		WRITE_FRC_REG_BY_CPU(FRC_MC_LOSS_SLICE_SEC, 0);//reg_mc_loss_slice_sec
}

void recfg_memc_mif_base_addr(u32 base_ofst)
{
	u32 reg_addr;
	u32 reg_data;

	for(reg_addr = FRC_REG_MC_YINFO_BADDR; reg_addr < FRC_REG_LOGO_SCC_BUF_ADDR; reg_addr += 4) {
		reg_data = READ_FRC_REG(reg_addr);
		reg_data += base_ofst;
		WRITE_FRC_REG_BY_CPU(reg_addr, reg_data);
	}
}

static const struct dbg_dump_tab frc_reg_tab[FRC_DBG_DUMP_TABLE_NUM] = {
	{"FRC_TOP_CTRL->reg_frc_en_in", FRC_TOP_CTRL, 0, 1},
	{"FRC_REG_INP_MODULE_EN_T5M", FRC_REG_INP_MODULE_EN, 0, 0},
	{"FRC_REG_INP_MODULE_EN_T3", FRC_REG_INP_MODULE_EN + 0xA0, 0, 0},

	{"FRC_REG_TOP_CTRL14->post_dly_vofst", FRC_REG_TOP_CTRL14, 0, 16},
	{"FRC_REG_TOP_CTRL14->reg_me_dly_vofst", FRC_REG_TOP_CTRL14, 16, 16},
	{"FRC_REG_TOP_CTRL15->reg_mc_dly_vofst0", FRC_REG_TOP_CTRL15, 0, 16},
	{"FRC_REG_TOP_CTRL15->reg_mc_dly_vofst1", FRC_REG_TOP_CTRL15, 16, 16},

	{"FRC_OUT_HOLD_CTRL->me_hold_line", FRC_OUT_HOLD_CTRL, 0, 8},
	{"FRC_OUT_HOLD_CTRL->mc_hold_line", FRC_OUT_HOLD_CTRL, 8, 8},
	{"FRC_INP_HOLD_CTRL->inp_hold_line", FRC_INP_HOLD_CTRL, 0, 13},

	{"FRC_FRAME_BUFFER_NUM->logo_fb_num", FRC_FRAME_BUFFER_NUM, 0, 5},
	{"FRC_FRAME_BUFFER_NUM->frc_fb_num", FRC_FRAME_BUFFER_NUM, 8, 5},

	{"FRC_REG_BLK_SIZE_XY->reg_me_mvx_div_mode", FRC_REG_BLK_SIZE_XY, 14, 2},
	{"FRC_REG_BLK_SIZE_XY->reg_me_mvy_div_mode", FRC_REG_BLK_SIZE_XY, 12, 2},
	// {"FRC_REG_BLK_SIZE_XY->reg_hme_mvx_div_mode", FRC_REG_BLK_SIZE_XY, 2, 2},
	// {"FRC_REG_BLK_SIZE_XY->reg_hme_mvy_div_mode", FRC_REG_BLK_SIZE_XY, 0, 2},

	{"FRC_REG_ME_DS_COEF_0", FRC_REG_ME_DS_COEF_0,	0, 0},
	{"FRC_REG_ME_DS_COEF_1", FRC_REG_ME_DS_COEF_1,	0, 0},
	{"FRC_REG_ME_DS_COEF_2", FRC_REG_ME_DS_COEF_2,	0, 0},
	{"FRC_REG_ME_DS_COEF_3", FRC_REG_ME_DS_COEF_3,	0, 0},
};

void frc_dump_reg_tab(void)
{
	u32 i = 0;
	enum chip_id chip;

	chip = get_chip_type();
	if (chip == ID_T3X) {
		pr_frc(0, "VIU_FRC_MISC (0x%x)->bypass val:0x%x (1:bypass)\n",
			VIU_FRC_MISC, vpu_reg_read(VIU_FRC_MISC) & 0x1);
		pr_frc(0, "VIU_FRC_MISC (0x%x)->pos_sel val:0x%x (0:after blend)\n",
			VIU_FRC_MISC, (vpu_reg_read(VIU_FRC_MISC) >> 4) & 0x1);
		pr_frc(0, "ENCL_FRC_CTRL (0x%x)->val:0x%x\n",
			ENCL_FRC_CTRL_T3X, vpu_reg_read(ENCL_FRC_CTRL_T3X) & 0xffff);
		pr_frc(0, "ENCL_VIDEO_VAVON_BLINE:0x%x->val: 0x%x\n",
			ENCL_VIDEO_VAVON_BLINE_T3X,
			(vpu_reg_read(ENCL_VIDEO_VAVON_BLINE_T3X) >> 16) & 0xffff);
	} else if (chip == ID_T5M || chip == ID_T3) {
		pr_frc(0, "VPU_FRC_TOP_CTRL (0x%x)->bypass val:0x%x (1:bypass)\n",
			VPU_FRC_TOP_CTRL, vpu_reg_read(VPU_FRC_TOP_CTRL) & 0x1);

	pr_frc(0, "VPU_FRC_TOP_CTRL (0x%x)->pos_sel val:0x%x (0:after blend)\n",
		VPU_FRC_TOP_CTRL, (vpu_reg_read(VPU_FRC_TOP_CTRL) >> 4) & 0x1);

	pr_frc(0, "ENCL_FRC_CTRL (0x%x)->val:0x%x\n",
		ENCL_FRC_CTRL, vpu_reg_read(ENCL_FRC_CTRL) & 0xffff);

		pr_frc(0, "ENCL_VIDEO_VAVON_BLINE:0x%x->val: 0x%x\n",
			ENCL_VIDEO_VAVON_BLINE,
				vpu_reg_read(ENCL_VIDEO_VAVON_BLINE) & 0xffff);
	}
	for (i = 0; i < FRC_DBG_DUMP_TABLE_NUM; i++) {
		if (frc_reg_tab[i].len != 0) {
			pr_frc(0, "%s (0x%x) val:0x%x\n",
			frc_reg_tab[i].name, frc_reg_tab[i].addr,
			READ_FRC_BITS(frc_reg_tab[i].addr,
			frc_reg_tab[i].start, frc_reg_tab[i].len));
		} else {
			pr_frc(0, "%s (0x%x) val:0x%x\n",
			frc_reg_tab[i].name, frc_reg_tab[i].addr,
			READ_FRC_REG(frc_reg_tab[i].addr));
		}
	}
}

/*request from xianjun, dump fixed table reg*/
void frc_dump_fixed_table(void)
{
	int i = 0;
	unsigned int value = 0;
	enum chip_id chip;

	chip = get_chip_type();

	if (chip == ID_T3) {
		for (i = 0; i < T3_DRV_REG_NUM; i++) {
			value = READ_FRC_REG(t3_drv_regs_table[i].addr);
			pr_frc(0, "0x%04x, 0x%08x\n", t3_drv_regs_table[i].addr, value);
		}
	} else if (chip == ID_T5M) {
		for (i = 0; i < T5M_REG_NUM; i++) {
			value = READ_FRC_REG(t5m_regs_table[i].addr);
			pr_frc(0, "0x%04x, 0x%08x\n", t5m_regs_table[i].addr, value);
		}
	} else if (chip == ID_T3X) {
		for (i = 0; i < T3X_REG_NUM; i++) {
			value = READ_FRC_REG(t3x_regs_table[i].addr);
			pr_frc(0, "0x%04x, 0x%08x\n", t3x_regs_table[i].addr, value);
		}
	}
}

void frc_set_val_from_reg(void)
{
	regdata_loadorgframe[0] = 0;
	regdata_loadorgframe[1] = 0;
	regdata_loadorgframe[2] = 0;

	regdata_inpholdctl_0002 = READ_FRC_REG(FRC_INP_HOLD_CTRL);
	regdata_outholdctl_0003 = READ_FRC_REG(FRC_OUT_HOLD_CTRL);
	regdata_top_ctl_0007 = READ_FRC_REG(FRC_REG_TOP_CTRL7);
	regdata_top_ctl_0009 = READ_FRC_REG(FRC_REG_TOP_CTRL9);
	regdata_top_ctl_0011 =  READ_FRC_REG(FRC_REG_TOP_CTRL17);

	regdata_pat_pointer_0102 = READ_FRC_REG(FRC_REG_PAT_POINTER);
	// regdata_loadorgframe[16];		// 0x0103

	regdata_phs_tab_0116 = READ_FRC_REG(FRC_REG_PHS_TABLE);
	regdata_blksizexy_012b = READ_FRC_REG(FRC_REG_BLK_SIZE_XY);
	regdata_blkscale_012c = READ_FRC_REG(FRC_REG_BLK_SCALE);
	regdata_hme_scale_012d = READ_FRC_REG(FRC_REG_ME_SCALE);

	regdata_logodbg_0142 = READ_FRC_REG(FRC_LOGO_DEBUG);

	regdata_iplogo_en_0503 = READ_FRC_REG(FRC_IPLOGO_EN);
	regdata_bbd_t2b_0604 = READ_FRC_REG(FRC_BBD_DETECT_REGION_TOP2BOT);
	regdata_bbd_l2r_0605 = READ_FRC_REG(FRC_BBD_DETECT_REGION_LFT2RIT);

	regdata_me_en_1100 = READ_FRC_REG(FRC_ME_EN);
	regdata_me_bbpixed_1108 = READ_FRC_REG(FRC_ME_BB_PIX_ED);
	regdata_me_bbblked_110a = READ_FRC_REG(FRC_ME_BB_BLK_ED);
	regdata_me_stat12rhst_110b = READ_FRC_REG(FRC_ME_STAT_12R_HST);
	regdata_me_stat12rh_110c = READ_FRC_REG(FRC_ME_STAT_12R_H01);
	regdata_me_stat12rh_110d = READ_FRC_REG(FRC_ME_STAT_12R_H23);
	regdata_me_stat12rv_110e = READ_FRC_REG(FRC_ME_STAT_12R_V0);
	regdata_me_stat12rv_110f = READ_FRC_REG(FRC_ME_STAT_12R_V1);

	regdata_vpbb1_1e03 = READ_FRC_REG(FRC_VP_BB_1);
	regdata_vpbb2_1e04 = READ_FRC_REG(FRC_VP_BB_2);
	regdata_vpmebb1_1e05 = READ_FRC_REG(FRC_VP_ME_BB_1);
	regdata_vpmebb2_1e06 = READ_FRC_REG(FRC_VP_ME_BB_2);

	regdata_vp_win1_1e58 = READ_FRC_REG(FRC_VP_REGION_WINDOW_1);
	regdata_vp_win2_1e59 = READ_FRC_REG(FRC_VP_REGION_WINDOW_2);
	regdata_vp_win3_1e5a = READ_FRC_REG(FRC_VP_REGION_WINDOW_3);
	regdata_vp_win4_1e5b = READ_FRC_REG(FRC_VP_REGION_WINDOW_4);

	regdata_mcset1_3000 = READ_FRC_REG(FRC_MC_SETTING1);
	regdata_mcset2_3001 = READ_FRC_REG(FRC_MC_SETTING2);

	regdata_mcdemo_win_3200  = READ_FRC_REG(FRC_MC_DEMO_WINDOW);

	regdata_topctl_3f01 = READ_FRC_REG(FRC_TOP_CTRL);
}
EXPORT_SYMBOL(frc_set_val_from_reg);

void frc_load_reg_table(struct frc_dev_s *frc_devp, u8 flag)
{
	enum chip_id chip;
	int i;

	if (!frc_devp)
		return;

	chip = get_chip_type();
	if (chip == ID_T3) {
		for (i = 0; i < T3_DRV_REG_NUM; i++)
			WRITE_FRC_REG_BY_CPU(t3_drv_regs_table[i].addr, t3_drv_regs_table[i].value);
		pr_frc(0, "t3_drv_regs_table[%d] init done\n", T3_DRV_REG_NUM);
		regdata_inpmoden_04f9 = READ_FRC_REG(FRC_REG_INP_MODULE_EN + 0xA0);
	} else if (chip == ID_T5M) {
		for (i = 0; i < T5M_DRV_REG_NUM; i++)
			WRITE_FRC_REG_BY_CPU(t5m_regs_table[i].addr, t5m_regs_table[i].value);
		pr_frc(0, "t5m_drv_regs_table[%d] init done\n", T5M_DRV_REG_NUM);
		regdata_inpmoden_04f9 = READ_FRC_REG(FRC_REG_INP_MODULE_EN);
	} else if (chip == ID_T3X && flag == 0) {
		for (i = 0; i < T3X_REG_NUM; i++)
			WRITE_FRC_REG_BY_CPU(t3x_regs_table[i].addr,
				t3x_regs_table[i].value);
		pr_frc(0, "t3x_regs_table[%d] init done\n", T3X_REG_NUM);
		regdata_inpmoden_04f9 = READ_FRC_REG(FRC_REG_INP_MODULE_EN);
	}
}

/* driver probe call */
void frc_internal_initial(struct frc_dev_s *frc_devp)
{
	// int i;
	enum chip_id chip;
	struct frc_fw_data_s *fw_data;
	struct frc_top_type_s *frc_top;
	u32 me_hold_line;//me_hold_line
	u32 mc_hold_line;//mc_hold_line
	u32 inp_hold_line;
	u32 out_frm_dly_num = 0;
	u32 ctrl_frame_num = 0;

	if (!frc_devp)
		return;

	fw_data = (struct frc_fw_data_s *)frc_devp->fw_data;
	frc_top = &fw_data->frc_top_type;
	chip = get_chip_type();

	frc_load_reg_table(frc_devp, 0);
	frc_set_val_from_reg();

	me_hold_line = fw_data->holdline_parm.me_hold_line;
	mc_hold_line = fw_data->holdline_parm.mc_hold_line;
	inp_hold_line = fw_data->holdline_parm.inp_hold_line;

	regdata_outholdctl_0003 = READ_FRC_REG(FRC_OUT_HOLD_CTRL);
	regdata_inpholdctl_0002 = READ_FRC_REG(FRC_INP_HOLD_CTRL);
	frc_config_reg_value(me_hold_line, 0xff, &regdata_outholdctl_0003);
	frc_config_reg_value(mc_hold_line << 8, 0xff00, &regdata_outholdctl_0003);
	WRITE_FRC_REG_BY_CPU(FRC_OUT_HOLD_CTRL, regdata_outholdctl_0003);
	frc_config_reg_value(inp_hold_line, 0x1fff, &regdata_inpholdctl_0002);
	WRITE_FRC_REG_BY_CPU(FRC_INP_HOLD_CTRL, regdata_inpholdctl_0002);
	frc_set_arb_ugt_cfg(ARB_UGT_WR, 1, 15);
	// sys_fw_param_frc_init(frc_top->hsize, frc_top->vsize, frc_top->is_me1mc4);
	// init_bb_xyxy(frc_top->hsize, frc_top->vsize, frc_top->is_me1mc4);
	frc_inp_init();
	if (frc_devp->in_out_ratio == FRC_RATIO_1_1)
		frc_devp->ud_dbg.res2_time_en = 3;
	else
		frc_devp->ud_dbg.res2_time_en = 0;
	// config_phs_lut(frc_top->frc_ratio_mode, frc_top->film_mode);
	// config_phs_regs(frc_top->frc_ratio_mode, frc_top->film_mode);
	config_me_top_hw_reg();
	if (chip >= ID_T3X) {
		frc_top->memc_loss_en = 0x13;
		frc_cfg_memc_loss(frc_top->memc_loss_en & 0x3);
		frc_cfg_mcdw_loss((frc_top->memc_loss_en >> 4) & 0x01);
		out_frm_dly_num = 0x0;
		ctrl_frame_num = (FRC_FREEZE_FRAME_NUM_T3X |
				FRC_BYPASS_FRAME_NUM_T3X << 4);
		// WRITE_FRC_BITS(FRC_REG_TOP_RESERVE0, 0x17, 0, 8); // rev.A
		// WRITE_FRC_BITS(FRC_REG_TOP_RESERVE0, 0x46, 0, 8); // rev.B
		// WRITE_FRC_BITS(FRC_REG_TOP_RESERVE0, 0x78, 0, 8); // rev.B + prevsync
		WRITE_FRC_BITS(FRC_REG_TOP_RESERVE0, ctrl_frame_num, 0, 8);

		// frc_memc_120hz_patch_1(frc_devp);
	} else {
		frc_top->memc_loss_en = 0x03;
		frc_cfg_memc_loss(frc_top->memc_loss_en & 0x3);
		out_frm_dly_num = 0x03000000;
		ctrl_frame_num = (FRC_FREEZE_FRAME_NUM |
				FRC_BYPASS_FRAME_NUM << 4);
		WRITE_FRC_BITS(FRC_REG_TOP_RESERVE0, ctrl_frame_num, 0, 8);
	}
	if (frc_devp->ud_dbg.res2_dbg_en == 3) {
		if (frc_devp->out_sts.out_framerate > 90) {
			t3x_eco_initial();
			t3x_eco_qp_cfg(2);
		} else if (frc_devp->out_sts.out_framerate < 70)
			t3x_verB_60hz_patch();
	}

	// protect mode, enable: memc delay 2 frame
	if (frc_devp->prot_mode) {
		regdata_top_ctl_0009 = READ_FRC_REG(FRC_REG_TOP_CTRL9);
		regdata_top_ctl_0011 = READ_FRC_REG(FRC_REG_TOP_CTRL17);
		frc_config_reg_value(out_frm_dly_num, 0x0F000000,
				&regdata_top_ctl_0009);
		WRITE_FRC_REG_BY_CPU(FRC_REG_TOP_CTRL9, regdata_top_ctl_0009);
		frc_config_reg_value(0x100, 0x100, &regdata_top_ctl_0011);
		WRITE_FRC_REG_BY_CPU(FRC_REG_TOP_CTRL17, regdata_top_ctl_0011);
		WRITE_FRC_REG_BY_CPU(FRC_MODE_OPT, 0x6); // set bit1/bit2
	} else {
		WRITE_FRC_REG_BY_CPU(FRC_MODE_OPT, 0x0); // clear bit1/bit2
	}
	WRITE_FRC_REG_BY_CPU(FRC_PROC_SIZE,
			(frc_devp->out_sts.vout_height & 0x3fff) << 16 |
			(frc_devp->out_sts.vout_width & 0x3fff));

	return;
}

u8 frc_frame_forcebuf_enable(u8 enable)
{
	u8  ro_frc_input_fid = 0;//u4

	if (enable == 1) {	//frc off->on
		ro_frc_input_fid = READ_FRC_REG(FRC_REG_PAT_POINTER) >> 4 & 0xF;
		//force phase 0
		regdata_top_ctl_0007 = READ_FRC_REG(FRC_REG_TOP_CTRL7);
		frc_config_reg_value(0x9000000, 0x9000000, &regdata_top_ctl_0007);
//		WRITE_FRC_REG_BY_CPU(FRC_REG_TOP_CTRL7, regdata_top_ctl_0007);
//		WRITE_FRC_REG_BY_CPU(FRC_REG_TOP_CTRL8, 0);
		FRC_RDMA_WR_REG_IN(FRC_REG_TOP_CTRL7, regdata_top_ctl_0007);
		FRC_RDMA_WR_REG_IN(FRC_REG_TOP_CTRL8, 0);
	} else {
		frc_config_reg_value(0x0, 0x9000000, &regdata_top_ctl_0007);
//		WRITE_FRC_REG_BY_CPU(FRC_REG_TOP_CTRL7, regdata_top_ctl_0007);
		FRC_RDMA_WR_REG_IN(FRC_REG_TOP_CTRL7, regdata_top_ctl_0007);
	}
	return ro_frc_input_fid;
}

void frc_frame_forcebuf_count(u8 forceidx)
{
	// UPDATE_FRC_REG_BITS(FRC_REG_TOP_CTRL7,
	// (forceidx | forceidx << 4 | forceidx << 8), 0xFFF);//0-11bit
	// regdata_top_ctl_0007 = READ_FRC_REG(FRC_REG_TOP_CTRL7);
	frc_config_reg_value((forceidx | forceidx << 4 | forceidx << 8),
		0xFFF, &regdata_top_ctl_0007);
	// WRITE_FRC_REG_BY_CPU(FRC_REG_TOP_CTRL7, regdata_top_ctl_0007);
	FRC_RDMA_WR_REG_IN(FRC_REG_TOP_CTRL7, regdata_top_ctl_0007);
}

u16 frc_check_vf_rate(u16 duration, struct frc_dev_s *frc_devp)
{
	int i, getflag;
	u16 framerate = 0;

	/*duration: 1600(60fps) 1920(50fps) 3200(30fps) 3203(29.97)*/
	/*3840(25fps) 4000(24fps) 4004(23.976fps)*/
	i = 0;
	getflag = 0;
	while (i < FRAME_RATE_CNT) {
		if (vf_rate_table[i].duration == duration) {
			framerate = vf_rate_table[i].framerate;
			getflag = 1;
			break;
		}
		i++;
	}
	if (!getflag && duration > 333) {
		framerate = 96000 / duration;
		getflag = 1;
	}
	if (getflag == 1 && framerate != frc_devp->in_sts.frc_vf_rate) {
		pr_frc(2, "input vf rate changed [%d->%d, duration:%d].\n",
			frc_devp->in_sts.frc_vf_rate, framerate,
			duration);
		frc_devp->in_sts.frc_vf_rate = framerate;
		getflag = 0;
	}
	return frc_devp->in_sts.frc_vf_rate;
}

void frc_get_film_base_vf(struct frc_dev_s *frc_devp)
{
	struct frc_fw_data_s *pfw_data;
	u16 in_frame_rate = frc_devp->in_sts.frc_vf_rate;
	u16 outfrmrate = frc_devp->out_sts.out_framerate;
	// u32 vf_duration = frc_devp->in_sts.duration;

	pfw_data = (struct frc_fw_data_s *)frc_devp->fw_data;
	pfw_data->frc_top_type.film_mode = EN_DRV_VIDEO;
	pfw_data->frc_top_type.vfp &= 0xFFFFFFF0;
	if (frc_devp->in_sts.frc_is_tvin)  // HDMI_src no inform alg
		return;
	if ((pfw_data->frc_top_type.vfp & BIT_7) == BIT_7)
		return;
	if ((pfw_data->frc_top_type.vfp & BIT_4) == 0)
		return;
	pfw_data->frc_top_type.vfp &= 0xfffffff0;
	pfw_data->frc_top_type.vfp |= 0x4;
	pr_frc(1, "get in_rate:%d,out_rate:%d\n", in_frame_rate, outfrmrate);
	switch (in_frame_rate) {
	case FRC_VD_FPS_24:
		if (outfrmrate == 50) {
			pfw_data->frc_top_type.film_mode = EN_DRV_FILM1123;
		} else if (outfrmrate == 60 || outfrmrate == 59) {
			pfw_data->frc_top_type.film_mode = EN_DRV_FILM32;
			pfw_data->frc_top_type.vfp |= 0x07;
		} else if (outfrmrate == 120) {
			pfw_data->frc_top_type.film_mode = EN_DRV_FILM55;
		}
		break;
	case FRC_VD_FPS_25:
		if (outfrmrate == 50) {
			pfw_data->frc_top_type.film_mode = EN_DRV_FILM22;
			pfw_data->frc_top_type.vfp |= 0x04;
		} else if (outfrmrate == 60 || outfrmrate == 59) {
			pfw_data->frc_top_type.film_mode = EN_DRV_FILM32322;
		} else if (outfrmrate == 100) {
			pfw_data->frc_top_type.film_mode = EN_DRV_FILM44;
		}
		break;
	case FRC_VD_FPS_30:
		if (outfrmrate == 50) {
			pfw_data->frc_top_type.film_mode = EN_DRV_FILM212;
		} else if (outfrmrate == 60 || outfrmrate == 59) {
			pfw_data->frc_top_type.film_mode = EN_DRV_FILM22;
			pfw_data->frc_top_type.vfp |= 0x04;
		} else if (outfrmrate == 120) {
			pfw_data->frc_top_type.film_mode = EN_DRV_FILM44;
		}
		break;
	case FRC_VD_FPS_50:
		if (outfrmrate == 60 || outfrmrate == 59) {
			pfw_data->frc_top_type.film_mode = EN_DRV_FILM21111;
		} else if (outfrmrate == 100) {
			pfw_data->frc_top_type.film_mode = EN_DRV_FILM22;
			pfw_data->frc_top_type.vfp |= 0x04;
		}
		break;
	case FRC_VD_FPS_60:
		if (outfrmrate == 120) {
			pfw_data->frc_top_type.film_mode = EN_DRV_FILM22;
			pfw_data->frc_top_type.vfp |= 0x04;
		}
		break;
	default:
		pfw_data->frc_top_type.film_mode = EN_DRV_VIDEO;
		break;
	}
}

void frc_set_enter_forcefilm(struct frc_dev_s *frc_devp, u16 flag)
{
	struct frc_fw_data_s *pfw_data;

	pfw_data = (struct frc_fw_data_s *)frc_devp->fw_data;

	if (flag)
		pfw_data->frc_top_type.vfp |= BIT_4;
	else
		pfw_data->frc_top_type.vfp &= (~BIT_4);
}

void frc_set_notell_film(struct frc_dev_s *frc_devp, u16 flag)
{
	struct frc_fw_data_s *pfw_data;

	pfw_data = (struct frc_fw_data_s *)frc_devp->fw_data;

	if (flag == 1)
		pfw_data->frc_top_type.vfp |= BIT_7;
	else
		pfw_data->frc_top_type.vfp &= ~BIT_7;
	pfw_data->frc_top_type.vfp &= ~BIT_6;
}

void frc_set_output_pattern(u8 enpat)
{
	u8 realpattern =  4;

	if (enpat) {
		realpattern = realpattern + enpat;
		if (realpattern > 9)
			realpattern = 9;
		frc_mtx_cfg(FRC_OUTPUT_CSC, realpattern);
	} else {
		frc_mtx_cfg(FRC_OUTPUT_CSC, DEFAULT);
	}
}

void frc_set_input_pattern(u8 enpat)
{
	u8 realpattern =  4;

	if (enpat) {
		realpattern = realpattern + enpat;
		if (realpattern > 9)
			realpattern = 9;
		frc_mtx_cfg(FRC_INPUT_CSC, realpattern);
	} else {
		frc_mtx_cfg(FRC_INPUT_CSC, DEFAULT);
	}
}

void frc_set_arb_ugt_cfg(enum frc_arb_ugt ch, u8 urgent, u8 level)
{
	u32 urgent_val;
	enum chip_id chip;

	u32 reg_0x0974 = READ_FRC_REG(FRC_ARB_UGT_RD_BASIC);
	u32 reg_0x0994 = READ_FRC_REG(FRC_ARB_UGT_WR_BASIC);
	u32 reg_0x3f07 = READ_FRC_REG(FRC_AXIRD0_QLEVEL);
	u32 reg_0x3f08 = READ_FRC_REG(FRC_AXIRD1_QLEVEL);
	u32 reg_0x3f09 = READ_FRC_REG(FRC_AXIWR0_QLEVEL);

	chip = get_chip_type();
	urgent_val = (urgent > 3) ? 3 : urgent;
	pr_frc(1, "%s set (ch:%d, urgent:%d, level:%d)\n", __func__, ch, urgent, level);
	if ((chip == ID_T3X || chip == ID_T5M) && urgent_val > 1) {
		pr_frc(1, "T3X/T5M only 0 or 1 urgent\n");
		urgent_val = 1;
	}
	switch (ch) {
	case ARB_UGT_R0:
		frc_config_reg_value(urgent_val, BIT_0 + BIT_1, &reg_0x0974);
		WRITE_FRC_REG_BY_CPU(FRC_ARB_UGT_RD_BASIC, reg_0x0974);
		reg_0x3f07 &= 0xFFFF0000;
		WRITE_FRC_REG_BY_CPU(FRC_AXIRD0_QLEVEL, reg_0x3f07 | level << urgent_val * 4);
		break;
	case ARB_UGT_R1:
		frc_config_reg_value(urgent_val << 2, BIT_2 + BIT_3, &reg_0x0974);
		WRITE_FRC_REG_BY_CPU(FRC_ARB_UGT_RD_BASIC, reg_0x0974);
		reg_0x3f07 &= 0x0000FFFF;
		WRITE_FRC_REG_BY_CPU(FRC_AXIRD0_QLEVEL,
			reg_0x3f07 | level << (urgent_val  * 4 + 16));
		break;
	case ARB_UGT_R2:
		frc_config_reg_value(urgent_val << 4, BIT_4 + BIT_5, &reg_0x0974);
		WRITE_FRC_REG_BY_CPU(FRC_ARB_UGT_RD_BASIC, reg_0x0974);
		reg_0x3f08 &= 0xFFFF0000;
		WRITE_FRC_REG_BY_CPU(FRC_AXIRD1_QLEVEL, reg_0x3f08 | level << urgent_val * 4);
		break;
	case ARB_UGT_R3:
		frc_config_reg_value(urgent_val << 6, BIT_6 + BIT_7, &reg_0x0974);
		WRITE_FRC_REG_BY_CPU(FRC_ARB_UGT_RD_BASIC, reg_0x0974);
		reg_0x3f08 &= 0x0000FFFF;
		WRITE_FRC_REG_BY_CPU(FRC_AXIRD1_QLEVEL,
			reg_0x3f08 | level << (urgent_val  * 4 + 16));
		break;
	case ARB_UGT_W0:
		frc_config_reg_value(urgent_val, BIT_0 + BIT_1, &reg_0x0994);
		WRITE_FRC_REG_BY_CPU(FRC_ARB_UGT_WR_BASIC, reg_0x0994);
		reg_0x3f09 &= 0xFFFF0000;
		WRITE_FRC_REG_BY_CPU(FRC_AXIWR0_QLEVEL, reg_0x3f09 | level << urgent_val * 4);
		break;
	case ARB_UGT_W1:
		frc_config_reg_value(urgent_val << 2, BIT_2 + BIT_3, &reg_0x0994);
		WRITE_FRC_REG_BY_CPU(FRC_ARB_UGT_WR_BASIC, reg_0x0994);
		reg_0x3f09 &= 0x0000FFFF;
		WRITE_FRC_REG_BY_CPU(FRC_AXIWR0_QLEVEL,
			reg_0x3f09 | level << (urgent_val * 4  + 16));
		break;
	case ARB_UGT_WR:
		reg_0x0974 &= 0xFF00;
		reg_0x0974 |= urgent_val | (urgent_val << 2) |
			(urgent_val << 4) | (urgent_val << 6);
		WRITE_FRC_REG_BY_CPU(FRC_ARB_UGT_RD_BASIC, reg_0x0974);
		reg_0x0994 &= 0xFFF0;
		reg_0x0994 |= urgent_val | (urgent_val << 2);
		WRITE_FRC_REG_BY_CPU(FRC_ARB_UGT_WR_BASIC, reg_0x0994);
		reg_0x3f07 = (level << (urgent_val & 0x3) * 4) |
			(level << ((urgent_val & 0x3) * 4 + 16));
		WRITE_FRC_REG_BY_CPU(FRC_AXIRD0_QLEVEL, reg_0x3f07);
		reg_0x3f08 = reg_0x3f07;
		WRITE_FRC_REG_BY_CPU(FRC_AXIRD1_QLEVEL, reg_0x3f08);
		reg_0x3f09 = reg_0x3f08;
		WRITE_FRC_REG_BY_CPU(FRC_AXIWR0_QLEVEL, reg_0x3f09);
		break;
	default:
		PR_FRC("%s ch:%d error", __func__, ch);
		break;
	}
	pr_frc(1, "FRC_ARB_UGT_RD_BASIC=0x%x\n",
					READ_FRC_REG(FRC_ARB_UGT_RD_BASIC));
	pr_frc(1, "FRC_ARB_UGT_WR_BASIC=0x%x\n",
					READ_FRC_REG(FRC_ARB_UGT_WR_BASIC));
	pr_frc(1, "FRC_AXIRD0_QLEVEL=0x%x\n",
					READ_FRC_REG(FRC_AXIRD0_QLEVEL));
	pr_frc(1, "FRC_AXIRD1_QLEVEL=0x%x\n",
					READ_FRC_REG(FRC_AXIRD1_QLEVEL));
	pr_frc(1, "FRC_AXIWR0_QLEVEL=0x%x\n",
					READ_FRC_REG(FRC_AXIWR0_QLEVEL));
}

void frc_set_n2m(u8 ratio_value)
{
	struct frc_dev_s *devp = get_frc_devp();

	if (get_chip_type() == ID_T3)
		return;

	if (ratio_value == devp->in_out_ratio)
		return;
	if (ratio_value <= (u8)FRC_RATIO_1_1) {
		devp->in_out_ratio = (enum frc_ratio_mode_type)ratio_value;
		if ((devp->use_pre_vsync & PRE_VSYNC_120HZ) ==
			PRE_VSYNC_120HZ) {
			set_vsync_2to1_mode(0);
			set_pre_vsync_mode(1);
		} else if ((devp->use_pre_vsync & PRE_VSYNC_060HZ) ==
			PRE_VSYNC_060HZ) {
			set_vsync_2to1_mode(0);
			set_pre_vsync_mode(1);
		} else {
			set_vsync_2to1_mode((ratio_value > 0) ? 0 : 1);
			set_pre_vsync_mode(0);
		}
		if (ratio_value == FRC_RATIO_1_1)
			devp->ud_dbg.res2_time_en = 3;
		else
			devp->ud_dbg.res2_time_en = 0;
		if (devp->frc_sts.state == FRC_STATE_ENABLE)
			devp->frc_sts.re_config = true;
	}
}

void frc_set_axi_crash_irq(struct frc_dev_s *frc_devp, u8 enable)
{
	if (frc_devp->axi_crash_irq <= 0)
		return;

	if (enable == 1)
		enable_irq(frc_devp->axi_crash_irq);
	else if (enable == 0)
		disable_irq(frc_devp->axi_crash_irq);
	else
		pr_frc(1, "%s invalid param\n",  __func__);
}

/* frc chip type
 * return chip ID
 */
int get_chip_type(void)
{
	enum chip_id chip;
	struct frc_dev_s *devp;
	struct frc_data_s *frc_data;

	devp = get_frc_devp();
	frc_data = (struct frc_data_s *)devp->data;
	chip = frc_data->match_data->chip;

	if (chip == ID_T3)
		return ID_T3;
	else if (chip == ID_T5M)
		return ID_T5M;
	else if (chip == ID_T3X)
		return ID_T3X;
	else if (chip == ID_TMAX)
		return ID_TMAX;
	else
		return ID_NULL;
}

static void frc_force_h2v2(u32 mcdw_path_en,  u32 h2v2_mode, u32 mcdw_loss_en)
{
	u32  tmp = 0;

	tmp = READ_FRC_REG(FRC_MC_H2V2_SETTING);
	tmp |= ((h2v2_mode >> 1) & 0x1) << 30;
	tmp |= (h2v2_mode & 0x1) << 24;
	WRITE_FRC_REG_BY_CPU(FRC_MC_H2V2_SETTING, tmp);

	tmp = READ_FRC_REG(FRC_INP_MCDW_CTRL);
	tmp |= (h2v2_mode & 0x3) << 24;
	tmp |= (mcdw_path_en & 0x1);
	tmp |= (mcdw_loss_en & 0x1) << 4;
	WRITE_FRC_REG_BY_CPU(FRC_INP_MCDW_CTRL, tmp);

	tmp = READ_FRC_REG(FRC_MCDW_PATH_CTRL);
	tmp |= h2v2_mode & 0x3;
	tmp |= (mcdw_path_en & 0x1) << 4;
	WRITE_FRC_REG_BY_CPU(FRC_MCDW_PATH_CTRL, tmp);

	tmp = READ_FRC_REG(FRC_BYP_PATH_CTRL);
	tmp |= BIT_4;
	WRITE_FRC_REG_BY_CPU(FRC_BYP_PATH_CTRL, tmp);

	tmp = READ_FRC_REG(FRC_INP_PATH_OPT);
	tmp |= 0x01E;
	WRITE_FRC_REG_BY_CPU(FRC_INP_PATH_OPT, tmp);

	pr_frc(1, "%s set\n", __func__);
	pr_frc(1, "FRC_MC_H2V2_SETTING=0x%x\n",
				READ_FRC_REG(FRC_MC_H2V2_SETTING));
	pr_frc(1, "FRC_INP_MCDW_CTRL=0x%x\n",
				READ_FRC_REG(FRC_INP_MCDW_CTRL));
	pr_frc(1, "FRC_MCDW_PATH_CTRL=0x%x\n",
				READ_FRC_REG(FRC_MCDW_PATH_CTRL));
	pr_frc(1, "FRC_BYP_PATH_CTRL=0x%x\n",
				READ_FRC_REG(FRC_BYP_PATH_CTRL));
	pr_frc(1, "FRC_INP_PATH_OPT=0x%x\n",
				READ_FRC_REG(FRC_INP_PATH_OPT));
}

static void frc_disable_h2v2(void)
{
	u32  tmp = 0;

	tmp = READ_FRC_REG(FRC_MC_H2V2_SETTING);
	tmp &= 0xBEFFFFFF; // clear bit24,30
	WRITE_FRC_REG_BY_CPU(FRC_MC_H2V2_SETTING, tmp);

	tmp = READ_FRC_REG(FRC_INP_MCDW_CTRL);
	tmp &= 0xFCFFFFEE  ; // clear bit24/25,0,4
	WRITE_FRC_REG_BY_CPU(FRC_INP_MCDW_CTRL, tmp);

	tmp = READ_FRC_REG(FRC_MCDW_PATH_CTRL);
	tmp &= 0xFFFFFFEC; // clear bit4, 0/1
	WRITE_FRC_REG_BY_CPU(FRC_MCDW_PATH_CTRL, tmp);

	tmp = READ_FRC_REG(FRC_BYP_PATH_CTRL);
	tmp &= 0xFFFFFFEF; // clear bit4
	WRITE_FRC_REG_BY_CPU(FRC_BYP_PATH_CTRL, tmp);

	tmp = READ_FRC_REG(FRC_INP_PATH_OPT);
	tmp &= 0xFFFFFFE1;  // clear bit 4,3,2,1
	WRITE_FRC_REG_BY_CPU(FRC_INP_PATH_OPT, tmp);

	pr_frc(1, "%s clear\n", __func__);
	pr_frc(1, "FRC_MC_H2V2_SETTING=0x%x\n",
				READ_FRC_REG(FRC_MC_H2V2_SETTING));
	pr_frc(1, "FRC_INP_MCDW_CTRL=0x%x\n",
				READ_FRC_REG(FRC_INP_MCDW_CTRL));
	pr_frc(1, "FRC_MCDW_PATH_CTRL=0x%x\n",
				READ_FRC_REG(FRC_MCDW_PATH_CTRL));
	pr_frc(1, "FRC_BYP_PATH_CTRL=0x%x\n",
				READ_FRC_REG(FRC_BYP_PATH_CTRL));
	pr_frc(1, "FRC_INP_PATH_OPT=0x%x\n",
				READ_FRC_REG(FRC_INP_PATH_OPT));
}

void frc_set_h2v2(u32 enable)
{
	enum chip_id chip;

	chip = get_chip_type();
	if (chip < ID_T3X) {
		pr_frc(0, "%s is not supported\n", __func__);
		return;
	}
	if (enable == 1)
		frc_force_h2v2(1, 3, 1);
	else
		frc_disable_h2v2();
}

void frc_set_mcdw_buffer_ratio(u32 ratio)
{
	enum chip_id chip;
	struct frc_dev_s *devp = get_frc_devp();

	chip = get_chip_type();
	if (chip < ID_T3X) {
		pr_frc(1, "%s is not supported\n", __func__);
		return;
	}
	if (!(ratio & 0xf) || !((ratio >> 4) & 0xf)) {
		pr_frc(1, "%s ratio value is err\n", __func__);
		return;
	}
	devp->buf.mcdw_size_rate = (ratio & 0xff);
}

void frc_memc_120hz_patch(struct frc_dev_s *frc_devp)
{
	enum chip_id chip;
	struct frc_fw_data_s *fw_data;
	struct frc_top_type_s *frc_top;
	u32  tmp = 0;

	if (!frc_devp)
		return;

	fw_data = (struct frc_fw_data_s *)frc_devp->fw_data;
	frc_top = &fw_data->frc_top_type;
	chip = get_chip_type();

	if (chip != ID_T3X)
		return;

	// h2v2 setting
	tmp = READ_FRC_REG(FRC_MC_H2V2_SETTING);
	tmp |= BIT_30 + BIT_24 + BIT_23 + BIT_21 + BIT_19;
	WRITE_FRC_REG_BY_CPU(FRC_MC_H2V2_SETTING, tmp);

	// hold mc chroma path
	tmp = READ_FRC_REG(0x927);
	tmp &= 0xFFFFFF8E;
	tmp |= 0xFFFFFF7E;
	WRITE_FRC_REG_BY_CPU(0x927, tmp);

	// close bbd opt
	tmp = READ_FRC_REG(FRC_INP_PATH_OPT);
	tmp |= BIT_3;
	WRITE_FRC_REG_BY_CPU(FRC_INP_PATH_OPT, tmp);

	// mcdw
	tmp = READ_FRC_REG(FRC_INP_MCDW_CTRL);
	tmp |= BIT_28 + BIT_4 + BIT_0 + BIT_25;
	tmp &= 0xFEFFFFFF;
	WRITE_FRC_REG_BY_CPU(FRC_INP_MCDW_CTRL, tmp);

	tmp = READ_FRC_REG(FRC_MCDW_PATH_CTRL);
	tmp |= BIT_4 + BIT_1;
	tmp &= 0xFFFFFFFE;
	WRITE_FRC_REG_BY_CPU(FRC_MCDW_PATH_CTRL, tmp);

	tmp = READ_FRC_REG(FRC_BYP_PATH_CTRL);
	tmp |= BIT_0;
	WRITE_FRC_REG_BY_CPU(FRC_BYP_PATH_CTRL, tmp);

	// mc lp mode set
	tmp = READ_FRC_REG(FRC_MC_HW_CTRL0);
	tmp |= BIT_3;
	WRITE_FRC_REG_BY_CPU(FRC_MC_HW_CTRL0, tmp);

	// close mc loss
	frc_top->memc_loss_en = 0x12;
	frc_cfg_memc_loss(frc_top->memc_loss_en & 0x3);

	// mcdw setting
	pr_frc(1, "%s set\n", __func__);
	pr_frc(1, "FRC_MC_H2V2_SETTING=0x%x\n",
				READ_FRC_REG(FRC_MC_H2V2_SETTING));
	pr_frc(1, "FRC_INP_MCDW_CTRL=0x%x\n",
				READ_FRC_REG(FRC_INP_MCDW_CTRL));
	pr_frc(1, "FRC_MCDW_PATH_CTRL=0x%x\n",
				READ_FRC_REG(FRC_MCDW_PATH_CTRL));
	pr_frc(1, "FRC_BYP_PATH_CTRL=0x%x\n",
				READ_FRC_REG(FRC_BYP_PATH_CTRL));
	pr_frc(1, "FRC_INP_PATH_OPT=0x%x\n",
				READ_FRC_REG(FRC_INP_PATH_OPT));
}

void frc_memc_120hz_patch_1(struct frc_dev_s *frc_devp)
{
	enum chip_id chip;
	struct frc_fw_data_s *fw_data;
	struct frc_top_type_s *frc_top;
	u32  tmp = 0;

	if (!frc_devp)
		return;

	fw_data = (struct frc_fw_data_s *)frc_devp->fw_data;
	frc_top = &fw_data->frc_top_type;
	chip = get_chip_type();

	if (chip != ID_T3X)
		return;

	if (frc_devp->ud_dbg.res2_dbg_en == 1 ||
		frc_devp->ud_dbg.res2_dbg_en == 3)
		return;

	// h2v2 setting
	tmp = READ_FRC_REG(FRC_MC_H2V2_SETTING);
	tmp |= 0x7E000000;
	tmp |= 0x01F80000;
	WRITE_FRC_REG_BY_CPU(FRC_MC_H2V2_SETTING, tmp);

	// hold mc chroma path
	tmp = READ_FRC_REG(0x927);
	tmp &= 0xFFFFFF8E;
	tmp |= 0xFFFFFF7E;
	WRITE_FRC_REG_BY_CPU(0x927, tmp);

	// close opt for prefetch phase
	tmp = READ_FRC_REG(FRC_INP_PATH_OPT);
	tmp |= 0x1B;
	WRITE_FRC_REG_BY_CPU(FRC_INP_PATH_OPT, tmp);

	// OUT_INT_OPT
	tmp = READ_FRC_REG(FRC_REG_MODE_OPT);
	tmp |= 0x20;
	WRITE_FRC_REG_BY_CPU(FRC_REG_MODE_OPT, tmp);

	// mcdw
	tmp = READ_FRC_REG(FRC_INP_MCDW_CTRL);
	tmp |= BIT_28 + BIT_4 + BIT_0 + BIT_25 + BIT_24;
	// tmp &= 0xFEFFFFFF;
	WRITE_FRC_REG_BY_CPU(FRC_INP_MCDW_CTRL, tmp);

	tmp = READ_FRC_REG(FRC_MCDW_PATH_CTRL);
	tmp |= BIT_4 + BIT_1 + BIT_0;
	// tmp &= 0xFFFFFFFE;
	WRITE_FRC_REG_BY_CPU(FRC_MCDW_PATH_CTRL, tmp);

	tmp = READ_FRC_REG(FRC_BYP_PATH_CTRL);
	tmp |= BIT_0 + BIT_4;
	WRITE_FRC_REG_BY_CPU(FRC_BYP_PATH_CTRL, tmp);

	// mc lp mode set
	tmp = READ_FRC_REG(FRC_MC_HW_CTRL0);
	tmp |= BIT_3;
	WRITE_FRC_REG_BY_CPU(FRC_MC_HW_CTRL0, tmp);

	// close mc loss
	frc_top->memc_loss_en = 0x12;
	frc_cfg_memc_loss(frc_top->memc_loss_en & 0x3);

	// mcdw setting
	pr_frc(1, "%s set\n", __func__);
	pr_frc(1, "FRC_MC_H2V2_SETTING=0x%x\n",
				READ_FRC_REG(FRC_MC_H2V2_SETTING));
	pr_frc(1, "FRC_INP_MCDW_CTRL=0x%x\n",
				READ_FRC_REG(FRC_INP_MCDW_CTRL));
	pr_frc(1, "FRC_MCDW_PATH_CTRL=0x%x\n",
				READ_FRC_REG(FRC_MCDW_PATH_CTRL));
	pr_frc(1, "FRC_BYP_PATH_CTRL=0x%x\n",
				READ_FRC_REG(FRC_BYP_PATH_CTRL));
	pr_frc(1, "FRC_INP_PATH_OPT=0x%x\n",
				READ_FRC_REG(FRC_INP_PATH_OPT));
}

void frc_memc_120hz_patch_2(struct frc_dev_s *frc_devp)
{
	enum chip_id chip;
	struct frc_fw_data_s *fw_data;
	struct frc_top_type_s *frc_top;
	u32  tmp = 0;

	if (!frc_devp)
		return;

	fw_data = (struct frc_fw_data_s *)frc_devp->fw_data;
	frc_top = &fw_data->frc_top_type;
	chip = get_chip_type();

	if (chip != ID_T3X)
		return;
	if (frc_devp->ud_dbg.res2_dbg_en == 1 ||
		frc_devp->ud_dbg.res2_dbg_en == 3)
		return;
	if (frc_devp->little_win == 1 && chip == ID_T3X)
		return;
	// mcdw
	tmp = READ_FRC_REG(FRC_INP_MCDW_CTRL);
	tmp |= BIT_25;
	tmp &= 0xFEFFFFFF;
	WRITE_FRC_REG_BY_CPU(FRC_INP_MCDW_CTRL, tmp);

	// TOP setting
	//tmp = READ_FRC_REG(FRC_REG_TOP_CTRL7);
	//tmp |= 0x03000000;
	//tmp &= 0xFFE00000;
	//WRITE_FRC_REG_BY_CPU(FRC_REG_TOP_CTRL7, tmp);

	//regdata_top_ctl_0007 = READ_FRC_REG(FRC_REG_TOP_CTRL7);
	//frc_config_reg_value(0x0300000, 0x031FFFFF, &regdata_top_ctl_0007);
	//WRITE_FRC_REG_BY_CPU(FRC_REG_TOP_CTRL7, regdata_top_ctl_0007);

	pr_frc(1, "%s set\n", __func__);
	pr_frc(1, "FRC_INP_MCDW_CTRL=0x%x\n",
				READ_FRC_REG(FRC_INP_MCDW_CTRL));
	//pr_frc(1, "FRC_REG_TOP_CTRL7=0x%x\n",
	//			READ_FRC_REG(FRC_REG_TOP_CTRL7));
}

void frc_memc_120hz_patch_3(struct frc_dev_s *frc_devp)
{
	enum chip_id chip;
	struct frc_fw_data_s *fw_data;
	struct frc_top_type_s *frc_top;
	u32  tmp = 0;

	if (!frc_devp)
		return;

	fw_data = (struct frc_fw_data_s *)frc_devp->fw_data;
	frc_top = &fw_data->frc_top_type;
	chip = get_chip_type();

	if (chip != ID_T3X)
		return;
	if (frc_devp->ud_dbg.res2_dbg_en == 1 ||
		frc_devp->ud_dbg.res2_dbg_en == 3)
		return;
	if (frc_devp->little_win == 1 && chip == ID_T3X)
		return;
	// TOP setting
	//regdata_top_ctl_0007 = READ_FRC_REG(FRC_REG_TOP_CTRL7);
	//frc_config_reg_value(0x0, 0x03000000, &regdata_top_ctl_0007);
	//WRITE_FRC_REG_BY_CPU(FRC_REG_TOP_CTRL7, regdata_top_ctl_0007);

	tmp = READ_FRC_REG(FRC_MCDW_PATH_CTRL);
	tmp &= 0xFFFFFFFE;
	WRITE_FRC_REG_BY_CPU(FRC_MCDW_PATH_CTRL, tmp);

	// h2v2 setting
	tmp = READ_FRC_REG(FRC_MC_H2V2_SETTING);
	tmp &= 0x41AFFFFF;
	WRITE_FRC_REG_BY_CPU(FRC_MC_H2V2_SETTING, tmp);

	pr_frc(1, "%s set\n", __func__);
	pr_frc(1, "FRC_MCDW_PATH_CTRL=0x%x\n",
				READ_FRC_REG(FRC_MCDW_PATH_CTRL));
	pr_frc(1, "FRC_MC_H2V2_SETTING=0x%x\n",
				READ_FRC_REG(FRC_MC_H2V2_SETTING));
	//pr_frc(1, "FRC_REG_TOP_CTRL7=0x%x\n",
	//			READ_FRC_REG(FRC_REG_TOP_CTRL7));
}

void frc_debug_table_print(struct work_struct *work)
{
	int i;
	struct frc_dev_s *devp = container_of(work,
		struct frc_dev_s, frc_print_work);

	if (unlikely(!devp)) {
		PR_ERR("%s err, devp is NULL\n", __func__);
		return;
	}
	if (!devp->probe_ok)
		return;
	if (!devp->power_on_flag)
		return;

	pr_frc(0, "print reg table\n");
	if (get_chip_type() == ID_T3X) {
		for (i = 0; i < T3X_REG_NUM; i++) {
			pr_frc(0, "0x%04x, 0x%08x\n",
				frc_regs_dbg_table[i].addr,
				frc_regs_dbg_table[i].value);
		}
	} else if (get_chip_type() == ID_T5M) {
		for (i = 0; i < T5M_REG_NUM; i++) {
			pr_frc(0, "0x%04x, 0x%08x\n",
				frc_regs_dbg_table[i].addr,
				frc_regs_dbg_table[i].value);
		}
	} else if (get_chip_type() == ID_T3) {
		for (i = 0; i < T3_REG_NUM; i++) {
			pr_frc(0, "0x%04x, 0x%08x\n",
				frc_regs_dbg_table[i].addr,
				frc_regs_dbg_table[i].value);
		}
	}
	devp->ud_dbg.pr_dbg = 0;
}

void frc_debug_print(struct frc_dev_s *devp)
{
	int i;

	if (get_chip_type() == ID_T3X) {
		for (i = 0; i < T3X_DRV_REG_NUM; i++) {
			frc_regs_dbg_table[i].addr =
				t3x_regs_table[i].addr;
			frc_regs_dbg_table[i].value =
				READ_FRC_REG(frc_regs_dbg_table[i].addr);
		}
	} else if (get_chip_type() == ID_T5M) {
		for (i = 0; i < T5M_REG_NUM; i++) {
			frc_regs_dbg_table[i].addr =
				t5m_regs_table[i].addr;
			frc_regs_dbg_table[i].value =
				READ_FRC_REG(frc_regs_dbg_table[i].addr);
		}
	} else if (get_chip_type() == ID_T3) {
		for (i = 0; i < T3_REG_NUM; i++) {
			frc_regs_dbg_table[i].addr =
				t3_regs_table[i].addr;
			frc_regs_dbg_table[i].value =
				READ_FRC_REG(frc_regs_dbg_table[i].addr);
		}
	}
	schedule_work(&devp->frc_print_work);
}

void frc_memc_clr_vbuffer(struct frc_dev_s *frc_devp, u8 flag)
{
	enum chip_id chip;
	u32  tmp_value, tmp_value2;

	if (!frc_devp)
		return;
	if (frc_devp->other1_flag == 0)
		return;

	chip = get_chip_type();
	if (flag == 0) {
		tmp_value = READ_FRC_REG(FRC_ME_GCV_EN);
		tmp_value |= BIT_0 + BIT_16;
		WRITE_FRC_REG_BY_CPU(FRC_ME_GCV_EN, tmp_value);
		if (chip >= ID_T5M) {
			tmp_value2 = READ_FRC_REG(FRC_ME_GCV2_EN);
			tmp_value2 |= BIT_12 + BIT_15;
			WRITE_FRC_REG_BY_CPU(FRC_ME_GCV2_EN, tmp_value2);
		}
	} else {
		tmp_value = READ_FRC_REG(FRC_ME_GCV_EN);
		tmp_value &= 0xFFFEFFFE;
		WRITE_FRC_REG_BY_CPU(FRC_ME_GCV_EN, tmp_value);
		if (chip >= ID_T5M) {
			tmp_value2 = READ_FRC_REG(FRC_ME_GCV2_EN);
			tmp_value2 &= 0xFFFF6FFF;
			WRITE_FRC_REG_BY_CPU(FRC_ME_GCV2_EN, tmp_value2);
		}
	}
	pr_frc(1, "%s set %d\n", __func__, flag);
	pr_frc(1, "FRC_ME_GCV_EN=0x%x\n",
				READ_FRC_REG(FRC_ME_GCV_EN));
}

void frc_in_sts_init(struct st_frc_in_sts *sts)
{
	if (!sts)
		return;

	sts->vf = NULL;
	sts->vf_sts = 0;
	sts->vf_type = 0;
	sts->duration = 0;
	sts->in_hsize = 0;
	sts->in_vsize = 0;
	sts->signal_type = 0;
	sts->source_type = 0;
	sts->frc_hd_start_lines = 0;
	sts->frc_hd_end_lines = 0;
	sts->frc_vd_start_lines = 0;
	sts->frc_vd_end_lines = 0;
	sts->frc_vsc_startp = 0;
	sts->frc_vsc_endp = 0;
	sts->frc_hsc_startp = 0;
	sts->frc_hsc_endp = 0;
};

void frc_chg_loss_slice_num(u8 num)
{
	enum chip_id chip;
	u32  tmp_value, tmp_value2;

	chip = get_chip_type();
	if (chip != ID_T3X)
		return;

	tmp_value = READ_FRC_REG(FRC_INP_MCDW_CTRL);
	tmp_value &= 0xFFC00FF;
	tmp_value2 = READ_FRC_REG(FRC_REG_TOP_CTRL11);
	tmp_value2 &= 0x0000FFFF;
	tmp_value |= (num & 0x3FF) << 8;
	WRITE_FRC_REG_BY_CPU(FRC_INP_MCDW_CTRL, tmp_value);
	tmp_value2 |= (num & 0xff) << 16;
	tmp_value2 |= (num & 0xff) << 24;
	WRITE_FRC_REG_BY_CPU(FRC_REG_TOP_CTRL11, tmp_value2);

	pr_frc(1, "%s set\n", __func__);
	pr_frc(1, "FRC_REG_TOP_CTRL11=0x%x\n",
			READ_FRC_REG(FRC_REG_TOP_CTRL11));
	pr_frc(1, "FRC_INP_MCDW_CTRL=0x%x\n",
			READ_FRC_REG(FRC_INP_MCDW_CTRL));
}

void t3x_eco_initial(void)
{
	enum chip_id chip;
	u32  tmp_value;
	int log = 2;

	chip = get_chip_type();
	if (chip != ID_T3X)
		return;
	// reg_mc_nr_en
	//tmp_value = READ_FRC_REG(FRC_REG_INP_MODULE_EN);
	//tmp_value |= BIT_9;
	//WRITE_FRC_REG_BY_CPU(FRC_REG_INP_MODULE_EN, tmp_value);
	//reg_nr_misc 32bits
	// WRITE_FRC_REG_BY_CPU(FRC_NR_MISC, 0x801);

	//Close Nr/bbd/mcwr opt for prefetch phase
	tmp_value = READ_FRC_REG(FRC_INP_PATH_OPT);
	// tmp_value |= BIT_0 + BIT_1 + BIT_3 + BIT_4;
	tmp_value |= BIT_0 + BIT_1 + BIT_3;
	WRITE_FRC_REG_BY_CPU(FRC_INP_PATH_OPT, tmp_value);

	//Use h2v2 pic in prefetch phase
	tmp_value = READ_FRC_REG(FRC_BYP_PATH_CTRL);
	tmp_value |= BIT_0 + BIT_4;
	WRITE_FRC_REG_BY_CPU(FRC_BYP_PATH_CTRL, tmp_value);

	// h2v2 setting force luma&chroma, h2+v2 downsample 20230907 add
	tmp_value = READ_FRC_REG(FRC_MC_H2V2_SETTING);
	tmp_value |= 0x5E000000;
	tmp_value |= 0x01780000;
	WRITE_FRC_REG_BY_CPU(FRC_MC_H2V2_SETTING, tmp_value);

	WRITE_FRC_REG_BY_CPU(FRC_SRCH_RNG_MODE, 0x77); // add 20230907

	//Use h2v2 pic in prefetch phase
	tmp_value = READ_FRC_REG(FRC_MCDW_PATH_CTRL);
	// reg_frc_mcdw_path_en  reg_frc_mcdw_v2_en reg_frc_mcdw_h2_en
	// tmp_value |= BIT_0 + BIT_1 + BIT_4;
	tmp_value &= 0xFFFFFFFC;
	tmp_value |= BIT_1 + BIT_4;
	WRITE_FRC_REG_BY_CPU(FRC_MCDW_PATH_CTRL, tmp_value);

	//generate h2v2 pic in prefetch phase
	tmp_value = READ_FRC_REG(FRC_INP_MCDW_CTRL);
	// reg_mcdw_lpf_mode  reg_mcdw_ds_mode_hv
	// reg_mcdw_loss_en reg_mcdw_path_en
	// tmp_value |= BIT_0 + BIT_4 + BIT_24 + BIT_25 + BIT_28;
	tmp_value &= 0xFC000000; // bit24,25  set 01
	tmp_value |= BIT_0 + BIT_4 + BIT_24 + BIT_25 + BIT_28;
	WRITE_FRC_REG_BY_CPU(FRC_INP_MCDW_CTRL, tmp_value);

	//slice_num == 1
	frc_chg_loss_slice_num(1);

	//CLOSS_MISC:encoder+decoder reg_use_sw_preslc_fifolevel == 0
	tmp_value = READ_FRC_REG(0x0C00);
	// reg_use_sw_preslc_fifolevel
	tmp_value &= 0xFFFFFFF7;
	WRITE_FRC_REG_BY_CPU(0x0C00, tmp_value);
	tmp_value = READ_FRC_REG(0x0D00);
	// reg_use_sw_preslc_fifolevel
	tmp_value &= 0xFFFFFFF7;
	WRITE_FRC_REG_BY_CPU(0x0D00, tmp_value);
	tmp_value = READ_FRC_REG(0x3500);
	// reg_use_sw_preslc_fifolevel
	tmp_value &= 0xFFFFFFF7;
	WRITE_FRC_REG_BY_CPU(0x3500, tmp_value);
	tmp_value = READ_FRC_REG(0x3700);
	// reg_use_sw_preslc_fifolevel
	tmp_value &= 0xFFFFFFF7;
	WRITE_FRC_REG_BY_CPU(0x3700, tmp_value);

	//CLOSS_DEBUG_MODE:encoder reg_force_qp_0 ==0x80~0x87
	tmp_value = READ_FRC_REG(0x0c0b);
	tmp_value &= 0xFFFFFF00;
	tmp_value |= 0x82;
	WRITE_FRC_REG_BY_CPU(0x0c0b, tmp_value);
	//CLOSS0_RATIO_0: encoder QP Value //reg_ratio_bppx16_0_3
	tmp_value = READ_FRC_REG(0x0c59);
	tmp_value &= 0x00FFFFFF;
	tmp_value |= 0x82000000;
	WRITE_FRC_REG_BY_CPU(0x0c59, tmp_value);

	//CLOSS_DEBUG_MODE:decoder reg_force_qp_0 ==0x80~0x87
	tmp_value = READ_FRC_REG(0x0d0b);
	tmp_value &= 0xFFFFFF00;
	tmp_value |= 0x80;
	WRITE_FRC_REG_BY_CPU(0x0d0b, tmp_value);
	tmp_value = READ_FRC_REG(0x350b);
	tmp_value &= 0xFFFFFF00;
	tmp_value |= 0x80;
	WRITE_FRC_REG_BY_CPU(0x350b, tmp_value);
	tmp_value = READ_FRC_REG(0x370b);
	tmp_value &= 0xFFFFFF00;
	tmp_value |= 0x80;
	WRITE_FRC_REG_BY_CPU(0x370b, tmp_value);

	//CLOSS_MISC:decoder reg_misc[1]
	tmp_value = READ_FRC_REG(0x0d00);
	tmp_value &= 0xFFFFDFFF;
	WRITE_FRC_REG_BY_CPU(0x0d00, tmp_value);
	tmp_value = READ_FRC_REG(0x3500);
	tmp_value &= 0xFFFFDFFF;
	WRITE_FRC_REG_BY_CPU(0x3500, tmp_value);
	tmp_value = READ_FRC_REG(0x3700);
	tmp_value &= 0xFFFFDFFF;
	WRITE_FRC_REG_BY_CPU(0x3700, tmp_value);

	// print setting
	pr_frc(log, "%s set\n", __func__);
	pr_frc(log, "FRC_REG_INP_MODULE_EN=0x%x\n",
				READ_FRC_REG(FRC_REG_INP_MODULE_EN));
	pr_frc(log, "FRC_NR_MISC=0x%x\n",
				READ_FRC_REG(FRC_NR_MISC));
	pr_frc(log, "FRC_INP_PATH_OPT=0x%x\n",
				READ_FRC_REG(FRC_INP_PATH_OPT));
	pr_frc(log, "FRC_BYP_PATH_CTRL=0x%x\n",
				READ_FRC_REG(FRC_BYP_PATH_CTRL));
	pr_frc(log, "FRC_MCDW_PATH_CTRL=0x%x\n",
				READ_FRC_REG(FRC_MCDW_PATH_CTRL));
	pr_frc(log, "FRC_INP_MCDW_CTRL=0x%x\n",
				READ_FRC_REG(FRC_INP_MCDW_CTRL));
	pr_frc(log, "FRC_REG_TOP_CTRL11=0x%x\n",
				READ_FRC_REG(FRC_REG_TOP_CTRL11));
	pr_frc(log, "0xc00=0x%x, 0xd00=0x%x, 0xc0b=0x%x, 0xc59=0x%x,0xd0b=0x%x\n",
		READ_FRC_REG(0x0c00), READ_FRC_REG(0x0d00),
		READ_FRC_REG(0x0c0b), READ_FRC_REG(0x0c59),
		READ_FRC_REG(0x0d0b));
	pr_frc(log, "0x3500=0x%x, 0x3700=0x%x, 0x350b=0x%x, 0x370b=0x%x\n",
		READ_FRC_REG(0x3500), READ_FRC_REG(0x3700),
		READ_FRC_REG(0x350b), READ_FRC_REG(0x370b));
}

void t3x_eco_qp_cfg(u32 qp)
{
	enum chip_id chip;
	u32  tmp_value;
	int log = 0;

	chip = get_chip_type();
	if (chip != ID_T3X)
		return;

	//CLOSS_DEBUG_MODE:encoder reg_force_qp_0 ==0x80~0x87
	tmp_value = READ_FRC_REG(0x0c0b);
	tmp_value &= 0xFFFFFF80;
	tmp_value |= (qp & 0x7F);
	WRITE_FRC_REG_BY_CPU(0x0c0b, tmp_value);

	//CLOSS0_RATIO_0: encoder QP Value
	tmp_value = READ_FRC_REG(0x0c59);
	tmp_value &= 0x80FFFFFF;
	tmp_value |= (qp & 0x7F) << 24;
	WRITE_FRC_REG_BY_CPU(0x0c59, tmp_value);

	pr_frc(log, "%s set\n", __func__);
	pr_frc(log, "0x0c0b=0x%x, 0x0c59=0x%x\n",
		READ_FRC_REG(0x0c0b), READ_FRC_REG(0x0c59));
}

void t3x_revB_patch_apply(void)
{
	enum chip_id chip;
	struct frc_dev_s *devp = get_frc_devp();

	if (!devp->probe_ok || !devp->power_on_flag)
		return;
	chip = get_chip_type();
	if (chip != ID_T3X)
		return;
	if (devp->ud_dbg.res2_dbg_en == 3) {
		t3x_eco_initial();
		t3x_eco_qp_cfg(2);
	}
}

void t3x_verB_set_cfg(u8 flag, struct frc_dev_s *frc_devp)
{
	enum chip_id chip;
	struct frc_dev_s *devp = get_frc_devp();
	u32 tmp_isr_cnt; // tmp_value;
	u32 inp_mcdw_ctrl;
	u32 mcdw_path_ctrl;

	if (!devp->probe_ok || !devp->power_on_flag)
		return;
	chip = get_chip_type();
	if (chip != ID_T3X || frc_devp->out_sts.out_framerate < 70)
		return;

	if (flag == 0) {   //  set before frc enable
		WRITE_FRC_BITS(FRC_INP_MCDW_CTRL, 3, 24, 2);
		WRITE_FRC_BITS(FRC_MCDW_PATH_CTRL, 3, 0, 2);
		WRITE_FRC_REG_BY_CPU(FRC_SRCH_RNG_MODE, 0x77);
		// tmp_value = READ_FRC_REG(FRC_MC_H2V2_SETTING);
		// tmp_value |= 0x20800000;
		// WRITE_FRC_REG_BY_CPU(FRC_MC_H2V2_SETTING, tmp_value);
		devp->ud_dbg.other1_err = 1;

		FRC_RDMA_WR_REG_IN(FRC_INP_MCDW_CTRL, READ_FRC_REG(FRC_INP_MCDW_CTRL));
		FRC_RDMA_WR_REG_IN(FRC_MCDW_PATH_CTRL, READ_FRC_REG(FRC_MCDW_PATH_CTRL));
		FRC_RDMA_WR_REG_IN(FRC_SRCH_RNG_MODE, 0x77);
		pr_frc(2, "%s set:%d INP_MCDW_CTRL=0x%x, MCDW_CTRL=0x%x vs_cnt=%d\n",
			__func__,
			flag,
			READ_FRC_REG(FRC_INP_MCDW_CTRL),
			READ_FRC_REG(FRC_MCDW_PATH_CTRL),
			devp->frc_sts.vs_cnt);
	} else {  //   restore after frc enable
		if (devp->ud_dbg.other1_err == 1) {
			tmp_isr_cnt = READ_FRC_REG(FRC_RO_INT_CNT);
			// tmp_value = READ_FRC_REG(FRC_REG_INP_INT_FLAG);
			pr_frc(2, "%s set:%d ro_isr_cnt=(%d,%d), isr_cnt=(%d,%d)\n",
				__func__,
				flag,
				tmp_isr_cnt & 0xFFF,
				tmp_isr_cnt >> 16,
				devp->in_sts.vs_cnt,
				devp->out_sts.vs_cnt);
				// if (1){  // ((tmp_value & BIT_4) == 0)
				// WRITE_FRC_BITS(FRC_INP_MCDW_CTRL, 2, 24, 2);
				// WRITE_FRC_BITS(FRC_MCDW_PATH_CTRL, 0, 0, 1);
				// WRITE_FRC_REG_BY_CPU(FRC_SRCH_RNG_MODE, 0x00);
				pr_frc(2, "%s before_INP_MCDW=0x%x,MCDW_CTRL=0x%x,SRCH_RNG=%x\n",
				__func__,
				READ_FRC_REG(FRC_INP_MCDW_CTRL),
				READ_FRC_REG(FRC_MCDW_PATH_CTRL),
				READ_FRC_REG(FRC_SRCH_RNG_MODE)
				);
				inp_mcdw_ctrl = READ_FRC_REG(FRC_INP_MCDW_CTRL);
				frc_config_reg_value((2 << 24), 0x3000000, &inp_mcdw_ctrl);
				FRC_RDMA_WR_REG_IN(FRC_INP_MCDW_CTRL, inp_mcdw_ctrl);

				mcdw_path_ctrl = READ_FRC_REG(FRC_MCDW_PATH_CTRL);
				frc_config_reg_value(0x0, 0x1, &mcdw_path_ctrl);
				FRC_RDMA_WR_REG_IN(FRC_MCDW_PATH_CTRL, mcdw_path_ctrl);

				// srch_rng_mode = READ_FRC_REG(FRC_SRCH_RNG_MODE);
				FRC_RDMA_WR_REG_IN(FRC_SRCH_RNG_MODE, 0x0);

				// tmp_value = READ_FRC_REG(FRC_MC_H2V2_SETTING);
				//  tmp_value &= 0xDF7FFFFF;
				// WRITE_FRC_REG_BY_CPU(FRC_MC_H2V2_SETTING, tmp_value);
				pr_frc(2, "%s set_INP_MCDW=0x%x,MCDW_CTRL=0x%x\n",
				__func__,
				inp_mcdw_ctrl, mcdw_path_ctrl);
				// READ_FRC_REG(FRC_SRCH_RNG_MODE));
				devp->ud_dbg.other1_err = 0;
				// }
		}
	}
}

void frc_pattern_dbg_ctrl(struct frc_dev_s *devp)
{
	if (!devp || !devp->pat_dbg.pat_en)
		return;

	if (devp->pat_dbg.pat_type & BIT_0)
		frc_set_input_pattern(devp->pat_dbg.pat_color);
	if (devp->pat_dbg.pat_type & BIT_1)
		frc_set_output_pattern(devp->pat_dbg.pat_color);
	if (devp->in_sts.enable_mute_flag == 1) {
		frc_set_output_pattern(5); // mute blank
		pr_frc(1, "enable set mute wait %d(%d) frames",
			devp->in_sts.mute_vsync_cnt,
			devp->frc_sts.vs_data_cnt);
	}
}

void t3x_verB_60hz_patch(void)
{
	enum chip_id chip;

	chip = get_chip_type();

	if (chip != ID_T3X)
		return;

	regdata_top_ctl_000a = READ_FRC_REG(FRC_REG_TOP_CTRL10);
	frc_config_reg_value(0x7f, 0xff, &regdata_top_ctl_000a);
	WRITE_FRC_REG_BY_CPU(FRC_REG_TOP_CTRL10, regdata_top_ctl_000a);

	inp_mcdw_ctl_047a = READ_FRC_REG(FRC_INP_MCDW_CTRL);
	inp_mcdw_ctl_047a &= ~0x11;
	WRITE_FRC_REG_BY_CPU(FRC_INP_MCDW_CTRL, inp_mcdw_ctl_047a);

	inp_path_opt_047c = READ_FRC_REG(FRC_INP_PATH_OPT);
	inp_path_opt_047c &= ~0x1;
	WRITE_FRC_REG_BY_CPU(FRC_INP_PATH_OPT, inp_path_opt_047c);

	mcdw_path_ctl_39fd = READ_FRC_REG(FRC_MCDW_PATH_CTRL);
	mcdw_path_ctl_39fd &= ~0x10;
	WRITE_FRC_REG_BY_CPU(FRC_MCDW_PATH_CTRL, mcdw_path_ctl_39fd);

	pr_frc(0, "%s ok", __func__);
}

void frc_clr_badedit_effect_before_enable(void)
{
	regdata_fwd_sign_ro_016e = READ_FRC_REG(FRC_REG_FWD_SIGN_RO);
	if (((regdata_fwd_sign_ro_016e >> 28) & 0x3) != 0) {
		pr_frc(1, "%s  read FRC_REG_FWD_SIGN_RO %8x\n", __func__,
			regdata_fwd_sign_ro_016e);
		regdata_fwd_sign_ro_016e &= 0xCFFFFFFF;
		WRITE_FRC_REG_BY_CPU(FRC_REG_FWD_SIGN_RO, regdata_fwd_sign_ro_016e);
		pr_frc(1, "set FRC_REG_FWD_SIGN_RO %8x\n", regdata_fwd_sign_ro_016e);
	}
	regdata_fwd_phs_0146 = READ_FRC_REG(FRC_REG_FWD_PHS);
	if ((regdata_fwd_phs_0146 & BIT_29) == BIT_29) {
		pr_frc(1, "%s  read FRC_REG_FWD_PHS %8x\n", __func__, regdata_fwd_phs_0146);
		regdata_fwd_phs_0146 &= 0xDFFFFFFF;
		WRITE_FRC_REG_BY_CPU(FRC_REG_FWD_PHS, regdata_fwd_phs_0146);
		pr_frc(1, "set FRC_REG_FWD_PHS %8x\n", regdata_fwd_phs_0146);
	}
	regdata_fwd_fid_0147 = READ_FRC_REG(FRC_REG_FWD_FID);
	if (regdata_fwd_fid_0147 != 0) {
		pr_frc(1, "%s  read FRC_REG_FWD_FID %8x\n", __func__, regdata_fwd_fid_0147);
		regdata_fwd_fid_0147 = 0;
		WRITE_FRC_REG_BY_CPU(FRC_REG_FWD_FID, regdata_fwd_fid_0147);
		pr_frc(1, "set FRC_REG_FWD_FID %8x\n", regdata_fwd_fid_0147);
	}
	regdata_top_ctl_0007 = 0;
	WRITE_FRC_REG_BY_CPU(FRC_REG_TOP_CTRL7, regdata_top_ctl_0007);
}
