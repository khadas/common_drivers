// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * drivers/amlogic/media/video_sink/vpp_post_s5.c
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

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/amlogic/major.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/ctype.h>
#include <linux/amlogic/media/vfm/vframe.h>
#include <linux/amlogic/media/vfm/vframe_provider.h>
#include <linux/amlogic/media/vfm/vframe_receiver.h>
#include <linux/amlogic/media/utils/amstream.h>
#include <linux/amlogic/media/vout/vout_notify.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/clk.h>
#include <linux/arm-smccc.h>
#include <linux/debugfs.h>
#include <linux/amlogic/media/canvas/canvas.h>
#include <linux/amlogic/media/canvas/canvas_mgr.h>
#include <linux/sched.h>
#include <linux/amlogic/media/video_sink/video_keeper.h>
#include "video_priv.h"
#include "video_hw_s5.h"
#include "video_reg_s5.h"
#include "vpp_post_s5.h"

#if defined(CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_VECM)
#include <linux/amlogic/media/amvecm/amvecm.h>
#endif
#include <linux/amlogic/media/utils/vdec_reg.h>

#include <linux/amlogic/media/registers/register.h>
#include <linux/uaccess.h>
#include <linux/amlogic/media/utils/amports_config.h>
#include <linux/amlogic/media/vpu/vpu.h>
#include "videolog.h"

#include <linux/amlogic/media/video_sink/vpp.h>
#ifdef CONFIG_AMLOGIC_MEDIA_TVIN
#include "linux/amlogic/media/frame_provider/tvin/tvin_v4l2.h"
#endif
#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
#include "../common/rdma/rdma.h"
#endif
#include <linux/amlogic/media/video_sink/video.h>
#include "../common/vfm/vfm.h"
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
#include <linux/amlogic/media/amdolbyvision/dolby_vision.h>
#endif
#include "video_receiver.h"
#ifdef CONFIG_AMLOGIC_MEDIA_LUT_DMA
#include <linux/amlogic/media/lut_dma/lut_dma.h>
#endif
#ifdef CONFIG_AMLOGIC_MEDIA_SECURITY
#include <linux/amlogic/media/vpu_secure/vpu_secure.h>
#endif
#include <linux/amlogic/media/video_sink/video_signal_notify.h>
#ifdef CONFIG_AMLOGIC_MEDIA_DEINTERLACE
#include <linux/amlogic/media/di/di_interface.h>
#endif

static int g_post_overlap_size = 32;
module_param(g_post_overlap_size, uint, 0664);
MODULE_PARM_DESC(g_post_overlap_size, "\n g_post_overlap_size\n");

static struct vpp_post_input_s g_vpp_input;
static struct vpp_post_input_s g_vpp_input_pre;
static struct vpp_post_input_s g_vpp1_input;
static struct vpp_post_input_s g_vpp1_input_pre;
static struct vpp_post_in_padding_s g_vpp_in_padding;
#define SIZE_ALIG32(frm_hsize)   ((((frm_hsize) + 31) >> 5) << 5)
#define SIZE_ALIG16(frm_hsize)   ((((frm_hsize) + 15) >> 4) << 4)
#define SIZE_ALIG8(frm_hsize)    ((((frm_hsize) + 7) >> 3) << 3)
#define SIZE_ALIG4(frm_hsize)    ((((frm_hsize) + 3) >> 2) << 2)

static u32 g_post_slice_num = 0xff;
MODULE_PARM_DESC(g_post_slice_num, "\n g_post_slice_num\n");
module_param(g_post_slice_num, uint, 0664);

static u32 g_vpp1_bypass_slice1_pre;
static u32 g_vpp1_bypass_slice1 = 0xff;
MODULE_PARM_DESC(g_vpp1_bypass_slice1, "\n g_vpp1_bypass_slice1\n");
module_param(g_vpp1_bypass_slice1, uint, 0664);

void (*get_vpp_din3_scope)(struct vpp_postblend_scope_s *scope);

static u32 get_reg_slice_vpost(int reg_addr, int slice_idx)
{
	u32 reg_offset;
	u32 reg_addr_tmp;

	reg_offset = slice_idx == 0 ? 0 :
		slice_idx == 1 ? 0x100 :
		slice_idx == 2 ? 0x700 : 0x1900;
	reg_addr_tmp = reg_addr + reg_offset;
	return reg_addr_tmp;
}

static void dump_vpp_blend_reg(void)
{
	u32 reg_addr, reg_val = 0;
	struct vpp_post_blend_reg_s *vpp_post_blend_reg = NULL;

	vpp_post_blend_reg = &vpp_post_reg.vpp_post_blend_reg;
	pr_info("vpp blend regs:\n");
	reg_addr = vpp_post_blend_reg->vpp_osd1_bld_h_scope;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [vpp_osd1_bld_h_scope]\n",
		   reg_addr, reg_val);
	reg_addr = vpp_post_blend_reg->vpp_osd1_bld_v_scope;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [vpp_osd1_bld_v_scope]\n",
		   reg_addr, reg_val);
	reg_addr = vpp_post_blend_reg->vpp_osd2_bld_h_scope;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [vpp_osd2_bld_h_scope]\n",
		   reg_addr, reg_val);
	reg_addr = vpp_post_blend_reg->vpp_osd2_bld_v_scope;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [vpp_osd2_bld_v_scope]\n",
		   reg_addr, reg_val);

	reg_addr = vpp_post_blend_reg->vpp_postblend_vd1_h_start_end;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [vpp_postblend_vd1_h_start_end]\n",
		   reg_addr, reg_val);
	reg_addr = vpp_post_blend_reg->vpp_postblend_vd1_v_start_end;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [vpp_postblend_vd1_v_start_end]\n",
		   reg_addr, reg_val);
	reg_addr = vpp_post_blend_reg->vpp_postblend_vd2_h_start_end;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [vpp_postblend_vd2_h_start_end]\n",
		   reg_addr, reg_val);
	reg_addr = vpp_post_blend_reg->vpp_postblend_vd2_v_start_end;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [vpp_postblend_vd2_v_start_end]\n",
		   reg_addr, reg_val);
	reg_addr = vpp_post_blend_reg->vpp_postblend_vd3_h_start_end;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [vpp_postblend_vd3_h_start_end]\n",
		   reg_addr, reg_val);
	reg_addr = vpp_post_blend_reg->vpp_postblend_vd3_v_start_end;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [vpp_postblend_vd3_v_start_end]\n",
		   reg_addr, reg_val);
	reg_addr = vpp_post_blend_reg->vpp_postblend_h_v_size;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [vpp_postblend_h_v_size]\n",
		   reg_addr, reg_val);
	reg_addr = vpp_post_blend_reg->vpp_post_blend_blend_dummy_data;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [vpp_post_blend_blend_dummy_data]\n",
		   reg_addr, reg_val);
	reg_addr = vpp_post_blend_reg->vpp_post_blend_dummy_alpha;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [vpp_post_blend_dummy_alpha]\n",
		   reg_addr, reg_val);
	reg_addr = vpp_post_blend_reg->vpp_post_blend_dummy_alpha1;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [vpp_post_blend_dummy_alpha1]\n",
		   reg_addr, reg_val);

	reg_addr = vpp_post_blend_reg->vd1_blend_src_ctrl;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [vd1_blend_src_ctrl]\n",
		   reg_addr, reg_val);
	reg_addr = vpp_post_blend_reg->vd2_blend_src_ctrl;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [vd2_blend_src_ctrl]\n",
		   reg_addr, reg_val);
	reg_addr = vpp_post_blend_reg->vd3_blend_src_ctrl;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [vd3_blend_src_ctrl]\n",
		   reg_addr, reg_val);
	reg_addr = vpp_post_blend_reg->osd1_blend_src_ctrl;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [osd1_blend_src_ctrl]\n",
		   reg_addr, reg_val);
	reg_addr = vpp_post_blend_reg->osd2_blend_src_ctrl;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [osd2_blend_src_ctrl]\n",
		   reg_addr, reg_val);
	reg_addr = vpp_post_blend_reg->vd1_postblend_alpha;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [vd1_postblend_alpha]\n",
		   reg_addr, reg_val);
	reg_addr = vpp_post_blend_reg->vd2_postblend_alpha;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [vd2_postblend_alpha]\n",
		   reg_addr, reg_val);
	reg_addr = vpp_post_blend_reg->vd3_postblend_alpha;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [vd3_postblend_alpha]\n",
		   reg_addr, reg_val);
	reg_addr = vpp_post_blend_reg->vpp_postblend_ctrl;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [vpp_postblend_ctrl]\n",
		   reg_addr, reg_val);
}

static void dump_vpp_post_misc_reg(void)
{
	int i;
	u32 reg_addr, reg_val = 0;
	struct vpp_post_misc_reg_s *vpp_post_misc_reg = NULL;

	vpp_post_misc_reg = &vpp_post_reg.vpp_post_misc_reg;
	pr_info("vpp post misc regs:\n");
	reg_addr = vpp_post_misc_reg->vpp_postblend_ctrl;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [vpp_postblend_ctrl]\n",
		   reg_addr, reg_val);
	reg_addr = vpp_post_misc_reg->vpp_obuf_ram_ctrl;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [vpp_obuf_ram_ctrl]\n",
		   reg_addr, reg_val);
	reg_addr = vpp_post_misc_reg->vpp_4p4s_ctrl;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [vpp_4p4s_ctrl]\n",
		   reg_addr, reg_val);
	reg_addr = vpp_post_misc_reg->vpp_4s4p_ctrl;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [vpp_4s4p_ctrl]\n",
		   reg_addr, reg_val);
	reg_addr = vpp_post_misc_reg->vpp_post_vd1_win_cut_ctrl;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [vpp_post_vd1_win_cut_ctrl]\n",
		   reg_addr, reg_val);
	reg_addr = vpp_post_misc_reg->vpp_post_win_cut_ctrl;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [vpp_post_win_cut_ctrl]\n",
		   reg_addr, reg_val);
	reg_addr = vpp_post_misc_reg->vpp_post_pad_hsize;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [vpp_post_pad_hsize]\n",
		   reg_addr, reg_val);
	reg_addr = vpp_post_misc_reg->vpp_post_pad_ctrl;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [vpp_post_pad_ctrl]\n",
		   reg_addr, reg_val);
	for (i = 0; i < SLICE_NUM; i++) {
		reg_addr = vpp_post_misc_reg->vpp_out_h_v_size;
		reg_addr = get_reg_slice_vpost(reg_addr, i);
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X [vpp_out_h_v_size]\n",
		   reg_addr, reg_val);
		reg_addr = vpp_post_misc_reg->vpp_ofifo_size;
		reg_addr = get_reg_slice_vpost(reg_addr, i);
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X [vpp_ofifo_size]\n",
			   reg_addr, reg_val);
		reg_addr = vpp_post_misc_reg->vpp_slc_deal_ctrl;
		reg_addr = get_reg_slice_vpost(reg_addr, i);
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X [vpp_slc_deal_ctrl]\n",
			   reg_addr, reg_val);
		reg_addr = vpp_post_misc_reg->vpp_hwin_size;
		reg_addr = get_reg_slice_vpost(reg_addr, i);
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X [vpp_hwin_size]\n",
			   reg_addr, reg_val);
		reg_addr = vpp_post_misc_reg->vpp_align_fifo_size;
		reg_addr = get_reg_slice_vpost(reg_addr, i);
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X [vpp_align_fifo_size]\n",
			   reg_addr, reg_val);
	}
}

static void dump_vpp_in_padcut_reg(void)
{
	u32 reg_addr, reg_val = 0;
	struct vpp_post_in_padcut_reg_s *vpp_post_in_padcut_regs = NULL;

	vpp_post_in_padcut_regs = &vpp_post_reg.vpp_post_in_padcut_reg;
	pr_info("vpp post in_padcut regs:\n");
	reg_addr = vpp_post_in_padcut_regs->vpp_post_padcut_ctrl;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [vpp_post_padcut_ctrl]\n",
		   reg_addr, reg_val);
	reg_addr = vpp_post_in_padcut_regs->vpp_post_padcut_hsize;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [vpp_post_padcut_hsize]\n",
		   reg_addr, reg_val);
	reg_addr = vpp_post_in_padcut_regs->vpp_post_padcut_vsize;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [vpp_post_padcut_vsize]\n",
		   reg_addr, reg_val);
	reg_addr = vpp_post_in_padcut_regs->vpp_post_win_cut_ctrl;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [vpp_post_win_cut_ctrl]\n",
		   reg_addr, reg_val);
}

static void dump_vpp1_blend_reg(void)
{
	u32 reg_addr, reg_val = 0;
	struct vpp1_post_blend_reg_s *vpp1_post_blend_reg = NULL;

	vpp1_post_blend_reg = &vpp_post_reg.vpp1_post_blend_reg;
	pr_info("vpp1 blend regs:\n");
	reg_addr = vpp1_post_blend_reg->vpp_osd1_bld_h_scope;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [vpp_osd1_bld_h_scope]\n",
		   reg_addr, reg_val);
	reg_addr = vpp1_post_blend_reg->vpp_osd1_bld_v_scope;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [vpp_osd1_bld_v_scope]\n",
		   reg_addr, reg_val);
	reg_addr = vpp1_post_blend_reg->vpp_postblend_vd1_h_start_end;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [vpp_postblend_vd1_h_start_end]\n",
		   reg_addr, reg_val);
	reg_addr = vpp1_post_blend_reg->vpp_postblend_vd1_v_start_end;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [vpp_postblend_vd1_v_start_end]\n",
		   reg_addr, reg_val);
	reg_addr = vpp1_post_blend_reg->vpp_postblend_h_v_size;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [vpp_postblend_h_v_size]\n",
		   reg_addr, reg_val);
	reg_addr = vpp1_post_blend_reg->vpp_post_blend_blend_dummy_data;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [vpp_post_blend_blend_dummy_data]\n",
		   reg_addr, reg_val);
	reg_addr = vpp1_post_blend_reg->vpp_post_blend_dummy_alpha;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [vpp_post_blend_dummy_alpha]\n",
		   reg_addr, reg_val);
	reg_addr = vpp1_post_blend_reg->vpp_post_blend_dummy_alpha1;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [vpp_post_blend_dummy_alpha1]\n",
		   reg_addr, reg_val);
	reg_addr = vpp1_post_blend_reg->vpp_postblend_ctrl;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X [vpp_postblend_ctrl]\n",
		   reg_addr, reg_val);
}

void dump_vpp_post_reg(void)
{
	dump_vpp_blend_reg();
	dump_vpp_post_misc_reg();
	if (cur_dev->vpp_in_padding_support)
		dump_vpp_in_padcut_reg();
	if (cur_dev->has_vpp1)
		/* vpp1 post reg */
		dump_vpp1_blend_reg();
}

static void wr_slice_vpost(u8 vpp_index, int reg_addr, int val, int slice_idx)
{
	rdma_wr_op rdma_wr = cur_dev->rdma_func[vpp_index].rdma_wr;
	u32 reg_offset;
	u32 reg_addr_tmp;

	reg_offset = slice_idx == 0 ? 0 :
		slice_idx == 1 ? 0x100 :
		slice_idx == 2 ? 0x700 : 0x1900;
	reg_addr_tmp = reg_addr + reg_offset;
	rdma_wr(reg_addr_tmp, val);
};

static void wr_slice_vpost_vcbus(int reg_addr, int val, int slice_idx)
{
	u32 reg_offset;
	u32 reg_addr_tmp;

	reg_offset = slice_idx == 0 ? 0 :
		slice_idx == 1 ? 0x100 :
		slice_idx == 2 ? 0x700 : 0x1900;
	reg_addr_tmp = reg_addr + reg_offset;
	WRITE_VCBUS_REG(reg_addr_tmp, val);
};

static void wr_reg_bits_slice_vpost(u8 vpp_index,
	int reg_addr, int val, int start, int len, int slice_idx)
{
	rdma_wr_bits_op rdma_wr_bits = cur_dev->rdma_func[vpp_index].rdma_wr_bits;
	u32 reg_offset;
	u32 reg_addr_tmp;

	reg_offset = slice_idx == 0 ? 0 :
		slice_idx == 1 ? 0x100 :
		slice_idx == 2 ? 0x700 : 0x1900;
	reg_addr_tmp = reg_addr + reg_offset;
	rdma_wr_bits(reg_addr_tmp, val, start, len);
};

/* hw reg info set */
static void vpp_post_in_padcut_set(u32 vpp_index,
	struct vpp0_post_s *vpp_post)
{
	rdma_wr_op rdma_wr = cur_dev->rdma_func[vpp_index].rdma_wr;
	struct vpp_post_in_padcut_reg_s *vpp_reg = NULL;
	u32 slice_num;

	//slice_num = vpp_post->slice_num;
	/* for t3x vpp_in_padding calc need slice_num = 2;*/
	if (video_is_meson_t3x_cpu())
		slice_num = 2;
	else
		slice_num = vpp_post->slice_num;
	vpp_reg = &vpp_post_reg.vpp_post_in_padcut_reg;
	rdma_wr(vpp_reg->vpp_post_padcut_ctrl,
		(vpp_post->vpp_pad_cut.cut_en & 1) << 31 |
		(vpp_post->vpp_pad_cut.cut_in_vsize & 0x1fff) << 16 |
		((vpp_post->vpp_pad_cut.cut_in_hsize /
			slice_num) & 0x1fff) << 0);
	rdma_wr(vpp_reg->vpp_post_padcut_vsize,
		(vpp_post->vpp_pad_cut.cut_v_end & 0x1fff) << 16 |
		(vpp_post->vpp_pad_cut.cut_v_bgn & 0x1fff) << 0);

	rdma_wr(vpp_reg->vpp_post_padcut_hsize,
		(((vpp_post->vpp_pad_cut.cut_h_end + 1) / slice_num - 1) & 0x1fff) << 16 |
		((vpp_post->vpp_pad_cut.cut_h_bgn / slice_num) & 0x1fff) << 0);
}

static void vpp_post_blend_set(u32 vpp_index,
	struct vpp_post_blend_s *vpp_blend)
{
	rdma_wr_op rdma_wr = cur_dev->rdma_func[vpp_index].rdma_wr;
	rdma_wr_bits_op rdma_wr_bits = cur_dev->rdma_func[vpp_index].rdma_wr_bits;
	struct vpp_post_blend_reg_s *vpp_reg = &vpp_post_reg.vpp_post_blend_reg;
	u32 postbld_src1_sel, postbld_src2_sel;
	u32 postbld_src3_sel, postbld_src4_sel;
	u32 postbld_src5_sel;
	u32 postbld_vd1_premult, postbld_vd2_premult;
	u32 postbld_vd3_premult, postbld_osd1_premult;
	u32 postbld_osd2_premult;
	u32 vd1_port, vd2_port;
	u32 port_val[3] = {0, 0, 0};

	/* setting blend scope */
	rdma_wr_bits(vpp_reg->vpp_postblend_vd1_h_start_end,
		(vpp_blend->bld_din0_h_start << 16) |
		vpp_blend->bld_din0_h_end, 0, 32);
	rdma_wr_bits(vpp_reg->vpp_postblend_vd1_v_start_end,
		(vpp_blend->bld_din0_v_start << 16) |
		vpp_blend->bld_din0_v_end, 0, 32);
	rdma_wr_bits(vpp_reg->vpp_postblend_vd2_h_start_end,
		(vpp_blend->bld_din1_h_start << 16) |
		vpp_blend->bld_din1_h_end, 0, 32);
	rdma_wr_bits(vpp_reg->vpp_postblend_vd2_v_start_end,
		(vpp_blend->bld_din1_v_start << 16) |
		vpp_blend->bld_din1_v_end, 0, 32);
	if (is_vpp0(1)) {
		rdma_wr_bits(vpp_reg->vpp_postblend_vd3_h_start_end,
			(vpp_blend->bld_din2_h_start << 16) |
			vpp_blend->bld_din2_h_end, 0, 32);
		rdma_wr_bits(vpp_reg->vpp_postblend_vd3_v_start_end,
			(vpp_blend->bld_din2_v_start << 16) |
			vpp_blend->bld_din2_v_end, 0, 32);
	}
	if (cur_dev->vpp_in_padding_support) {
		/* for t3x vpp padding */
		rdma_wr_bits(vpp_reg->vpp_osd1_bld_h_scope,
			(vpp_blend->bld_din3_h_start << 16) |
			vpp_blend->bld_din3_h_end, 0, 32);
		rdma_wr_bits(vpp_reg->vpp_osd1_bld_v_scope,
			(vpp_blend->bld_din3_v_start << 16) |
			vpp_blend->bld_din3_v_end, 0, 32);
	}
	rdma_wr(vpp_reg->vpp_postblend_h_v_size,
		vpp_blend->bld_out_padding_w | vpp_blend->bld_out_padding_h << 16);
	rdma_wr(vpp_reg->vpp_post_blend_blend_dummy_data,
		vpp_blend->bld_dummy_data);
	rdma_wr_bits(vpp_reg->vpp_post_blend_dummy_alpha,
		0x100 | 0x000 << 16, 0, 32);
	// blend0_dummy_alpha|blend1_dummy_alpha<<16
	rdma_wr_bits(vpp_reg->vpp_post_blend_dummy_alpha1,
		0x000 | 0x000 << 16, 0, 32);
	// blend2_dummy_alpha|blend3_dummy_alpha<<16

	postbld_vd1_premult  = vpp_blend->bld_din0_premult_en;
	postbld_vd2_premult  = vpp_blend->bld_din1_premult_en;
	postbld_vd3_premult  = vpp_blend->bld_din2_premult_en;
	postbld_osd1_premult = vpp_blend->bld_din3_premult_en;
	postbld_osd2_premult = vpp_blend->bld_din4_premult_en;

	postbld_src1_sel     = vpp_blend->bld_src1_sel;
	postbld_src2_sel     = vpp_blend->bld_src2_sel;
	postbld_src3_sel     = vpp_blend->bld_src3_sel;
	postbld_src4_sel     = vpp_blend->bld_src4_sel;
	postbld_src5_sel     = vpp_blend->bld_src5_sel;

	/* just reset the select port */
	if (glayer_info[0].cur_sel_port > 2 ||
		glayer_info[1].cur_sel_port > 2) {
		glayer_info[0].cur_sel_port = 0;
		glayer_info[1].cur_sel_port = 1;
	}
	vd1_port = glayer_info[0].cur_sel_port;
	vd2_port = glayer_info[1].cur_sel_port;
	/* vd2 path sel */
	if (vd_layer[0].post_blend_en)
		port_val[vd1_port] = postbld_src1_sel;
	if (vd_layer[1].post_blend_en)
		port_val[vd2_port] = postbld_src2_sel;
	if (debug_flag_s5 & DEBUG_VPP_POST)
		pr_info("%s:vd1_port:%d, vd2_port:%d, val:0x%x, 0x%x\n",
			__func__,
			vd1_port,
			vd2_port,
			port_val[vd1_port],
			port_val[vd2_port]);
	rdma_wr(vpp_reg->vd1_blend_src_ctrl,
		(port_val[0] & 0xf) |
		(postbld_vd1_premult & 0x1) << 4);
	rdma_wr(vpp_reg->vd2_blend_src_ctrl,
		(port_val[1] & 0xf) |
		(postbld_vd2_premult & 0x1) << 4);

	rdma_wr(vpp_reg->vd3_blend_src_ctrl,
		(postbld_src3_sel & 0xf) |
		(postbld_vd3_premult & 0x1) << 4);
	rdma_wr(vpp_reg->osd2_blend_src_ctrl,
		(port_val[2] & 0xf) |
		(postbld_src5_sel & 0x1) << 4);

	rdma_wr(vpp_reg->vd1_postblend_alpha,
		vpp_blend->bld_din0_alpha);
	rdma_wr(vpp_reg->vd2_postblend_alpha,
		vpp_blend->bld_din1_alpha);
	rdma_wr(vpp_reg->vd3_postblend_alpha,
		vpp_blend->bld_din2_alpha);

	rdma_wr_bits(vpp_reg->vpp_postblend_ctrl,
		vpp_blend->bld_out_en, 8, 1);
	/* vpp postblend v size after vpp in padding v cut module */
	if (cur_dev->vpp_in_padding_support)
		rdma_wr(vpp_reg->vpp_post_slice2ppc_v_size, vpp_blend->bld_out_h);
	if (debug_flag_s5 & DEBUG_VPP_POST) {
		pr_info("%s, vpp_index=%d: vpp_postblend_h_v_size=%x\n",
			__func__, vpp_index, vpp_blend->bld_out_padding_w |
			vpp_blend->bld_out_padding_h << 16);
		pr_info("%s: vpp_postblend_vd1_h_start_end=%x\n",
			__func__, vpp_blend->bld_din0_h_start << 16 |
			vpp_blend->bld_din0_h_end);
		pr_info("%s: vpp_postblend_vd1_v_start_end=%x\n",
			__func__, vpp_blend->bld_din0_v_start << 16 |
			vpp_blend->bld_din0_v_end);
		pr_info("%s: vpp_postblend_vd2_h_start_end=%x\n",
			__func__, vpp_blend->bld_din1_h_start << 16 |
			vpp_blend->bld_din1_h_end);
		pr_info("%s: vpp_postblend_vd2_v_start_end=%x\n",
			__func__, vpp_blend->bld_din1_v_start << 16 |
			vpp_blend->bld_din1_v_end);
	}
}

static void vpp1_post_blend_set(struct vpp1_post_blend_s *vpp_blend)
{
	rdma_wr_op rdma_wr = cur_dev->rdma_func[VPP1].rdma_wr;
	rdma_wr_bits_op rdma_wr_bits = cur_dev->rdma_func[VPP1].rdma_wr_bits;
	struct vpp1_post_blend_reg_s *vpp_reg = &vpp_post_reg.vpp1_post_blend_reg;

	/* setting blend scope */
	/* vd3 scope */
	rdma_wr_bits(vpp_reg->vpp_postblend_vd1_h_start_end,
		(vpp_blend->bld_din0_h_start << 16) |
		vpp_blend->bld_din0_h_end, 0, 32);
	rdma_wr_bits(vpp_reg->vpp_postblend_vd1_v_start_end,
		(vpp_blend->bld_din0_v_start << 16) |
		vpp_blend->bld_din0_v_end, 0, 32);
	/* osd3 scope */

	rdma_wr(vpp_reg->vpp_postblend_h_v_size,
		vpp_blend->bld_out_w | vpp_blend->bld_out_h << 16);
	rdma_wr(vpp_reg->vpp_post_blend_blend_dummy_data,
		vpp_blend->bld_dummy_data);
	rdma_wr_bits(vpp_reg->vpp_post_blend_dummy_alpha,
		0x100 | 0x000 << 16, 0, 32);
	rdma_wr_bits(vpp_reg->vpp_post_blend_dummy_alpha1,
		0x000 | 0x000 << 16, 0, 32);
	rdma_wr(vpp_reg->vpp_postblend_ctrl,
		vpp_blend->bld_out_en << 31 |
		vpp_blend->vpp1_dpath_sel << 30 |
		vpp_blend->vd3_dpath_sel << 29 |
		vpp_blend->bld_din0_alpha << 20 |
		vpp_blend->bld_din0_premult_en << 16 |
		vpp_blend->bld_din1_premult_en << 17 |
		vpp_blend->bld_src2_sel << 4 |
		vpp_blend->bld_src1_sel);

	if (debug_flag_s5 & DEBUG_VPP1_POST) {
		pr_info("%s: vpp1_postblend_h_v_size=%x\n",
			__func__, vpp_blend->bld_out_w | vpp_blend->bld_out_h << 16);
		pr_info("%s: vpp1_postblend_vd1_h_start_end=%x\n",
			__func__, vpp_blend->bld_din0_h_start << 16 |
			vpp_blend->bld_din0_h_end);
		pr_info("%s: vpp1_postblend_vd1_v_start_end=%x\n",
			__func__, vpp_blend->bld_din0_v_start << 16 |
			vpp_blend->bld_din0_v_end);
		pr_info("%s: bld_src2_sel:%d, bld_src1_sel:%d\n", __func__,
			vpp_blend->bld_src2_sel,
			vpp_blend->bld_src1_sel);
	}
}

static void vpp_post_slice_set(u32 vpp_index,
	struct vpp0_post_s *vpp_post)
{
	rdma_wr_bits_op rdma_wr_bits = cur_dev->rdma_func[vpp_index].rdma_wr_bits;
	struct vpp_post_misc_reg_s *vpp_reg = &vpp_post_reg.vpp_post_misc_reg;
	u32 slice_set;

	/* 2ppc2slice overlap size */
	rdma_wr_bits(vpp_reg->vpp_postblend_ctrl,
		vpp_post->overlap_hsize, 0, 8);
	/* slice mode */
	if (vd_layer_vpp[0].vpp_index == VPP1)
		rdma_wr_bits(vpp_reg->vpp_obuf_ram_ctrl, 1, 0, 2);
	else
		rdma_wr_bits(vpp_reg->vpp_obuf_ram_ctrl,
			     vpp_post->slice_num - 1, 0, 2);

	/* default = 0, 0: 4ppc to 4slice
	 * 1: 4ppc to 2slice
	 * 2: 4ppc to 1slice
	 * 3: disable
	 */
	switch (vpp_post->slice_num) {
	case 1:
		slice_set = 2;
		break;
	case 2:
		slice_set = 1;
		break;
	case 4:
		slice_set = 0;
		break;
	default:
		slice_set = 3;
		break;
	}
	rdma_wr_bits(vpp_reg->vpp_4p4s_ctrl, slice_set, 0, 2);
	rdma_wr_bits(vpp_reg->vpp_4s4p_ctrl, slice_set, 0, 2);
	if (debug_flag_s5 & DEBUG_VPP_POST) {
		pr_info("%s, vpp_index=%d: vpp_4p4s_ctrl=%x\n",
			__func__, vpp_index, slice_set);
	}
}

static void vpp_vd1_hwin_set(u32 vpp_index,
	struct vpp0_post_s *vpp_post)
{
	rdma_wr_op rdma_wr = cur_dev->rdma_func[vpp_index].rdma_wr;
	struct vpp_post_misc_reg_s *vpp_reg = &vpp_post_reg.vpp_post_misc_reg;
	u32 vd1_win_in_hsize = 0, vd1_slice_num = 0;

	if (vpp_post->vd1_hwin.vd1_hwin_en) {
		if (video_is_meson_s5_cpu()) {
			vd1_win_in_hsize =
				(vpp_post->vd1_hwin.vd1_hwin_in_hsize +
				SLICE_NUM - 1) / SLICE_NUM;
			rdma_wr(vpp_reg->vpp_post_vd1_win_cut_ctrl,
				 vpp_post->vd1_hwin.vd1_hwin_en << 31  |
				 vd1_win_in_hsize);
		} else {
			/* for t3x vd1 padding is used for postblend cut eco */
			vd1_slice_num = vpp_post->vd1_hwin.slice_num;
			vd1_win_in_hsize = (vpp_post->vd1_hwin.vd1_hwin_in_hsize +
				vd1_slice_num - 1) / vd1_slice_num;
			/* update v size for t3x */
			rdma_wr(vpp_reg->vpp_post_vd1_win_cut_ctrl,
				 vpp_post->vd1_hwin.vd1_hwin_en << 31  |
				 vpp_post->vd1_hwin.vd1_win_vsize << 16 |
				 vd1_win_in_hsize);
		}
		if (debug_flag_s5 & DEBUG_VPP_POST)
			pr_info("%s, vpp_index=%d: vpp_post_vd1_win_cut_ctrl:vd1_win_in_hsize=%d, vd1_win_vsize=%d\n",
				__func__, vpp_index, vd1_win_in_hsize,
				vpp_post->vd1_hwin.vd1_win_vsize);
	} else {
		rdma_wr(vpp_reg->vpp_post_vd1_win_cut_ctrl, 0);
	}
}

static void vpp_post_proc_set(u8 vpp_index,
	struct vpp0_post_s *vpp_post)
{
	struct vpp_post_proc_s *vpp_post_proc = NULL;
	struct vpp_post_proc_slice_s *vpp_post_proc_slice = NULL;
	struct vpp_post_proc_hwin_s *vpp_post_proc_hwin = NULL;
	struct vpp_post_misc_reg_s *vpp_reg = &vpp_post_reg.vpp_post_misc_reg;
	u32 slice_num;
	int i;

	vpp_post_proc = &vpp_post->vpp_post_proc;
	vpp_post_proc_slice = &vpp_post_proc->vpp_post_proc_slice;
	vpp_post_proc_hwin = &vpp_post_proc->vpp_post_proc_hwin;
	slice_num = vpp_post->slice_num;

	for (i = 0; i < slice_num; i++) {
		wr_slice_vpost(vpp_index, vpp_reg->vpp_out_h_v_size,
			vpp_post_proc_slice->hsize[i] << 16 |
			vpp_post_proc_slice->vsize[i], i);
		wr_reg_bits_slice_vpost(vpp_index, vpp_reg->vpp_ofifo_size,
			0x800, 0, 14, i);
		/* slice hwin deal */
		wr_reg_bits_slice_vpost(vpp_index, vpp_reg->vpp_slc_deal_ctrl,
			vpp_post_proc_hwin->hwin_en[i], 3, 1, i);
		wr_slice_vpost(vpp_index, vpp_reg->vpp_hwin_size,
			vpp_post_proc_hwin->hwin_end[i] << 16 |
			vpp_post_proc_hwin->hwin_bgn[i], i);
		wr_reg_bits_slice_vpost(vpp_index, vpp_reg->vpp_align_fifo_size,
			vpp_post_proc->align_fifo_size[i], 0, 14, i);
		/* todo: for other unit bypass handle */
		if (debug_flag_s5 & DEBUG_VPP_POST) {
			pr_info("%s, vpp_index=%d: vpp_out_h_v_size=%x\n",
				__func__, vpp_index, vpp_post_proc_slice->hsize[i] << 16 |
			vpp_post_proc_slice->vsize[i]);
			pr_info("%s: vpp_hwin_size=%x\n",
				__func__, vpp_post_proc_hwin->hwin_end[i] << 16 |
			vpp_post_proc_hwin->hwin_bgn[i]);
		}
	}
}

static void vpp_post_padding_set(u32 vpp_index,
	struct vpp0_post_s *vpp_post)
{
	rdma_wr_op rdma_wr = cur_dev->rdma_func[vpp_index].rdma_wr;
	struct vpp_post_misc_reg_s *vpp_reg = &vpp_post_reg.vpp_post_misc_reg;

	if (vpp_post->vpp_post_pad.vpp_post_pad_en) {
		/* reg_pad_hsize */
		rdma_wr(vpp_reg->vpp_post_pad_hsize,
			(vpp_post->vpp_post_pad.vpp_post_pad_hsize)	<< 0);
		rdma_wr(vpp_reg->vpp_post_pad_ctrl,
			vpp_post->vpp_post_pad.vpp_post_pad_dummy << 0 |
			vpp_post->vpp_post_pad.vpp_post_pad_rpt_lcol << 30 |
			vpp_post->vpp_post_pad.vpp_post_pad_en << 31);
		if (debug_flag_s5 & DEBUG_VPP_POST) {
			pr_info("%s, vpp_index=%d: vpp_post_pad_hsize=%x\n",
				__func__, vpp_index, vpp_post->vpp_post_pad.vpp_post_pad_hsize);
		}
	} else {
		rdma_wr(vpp_reg->vpp_post_pad_ctrl, 0);
	}
}

static void vpp_post_win_cut_set(u32 vpp_index,
	struct vpp0_post_s *vpp_post)
{
	//rdma_wr_op rdma_wr = cur_dev->rdma_func[vpp_index].rdma_wr;
	//struct vpp_post_misc_reg_s *vpp_reg = &vpp_post_reg.vpp_post_misc_reg;
	struct vpp_post_pad_s *vpp_post_pad = NULL;

	vpp_post_pad = &vpp_post->vpp_post_pad;
	//if (vpp_post_pad->vpp_post_pad_en &&
	//	)
}

void vpp_post_set(u32 vpp_index, struct vpp_post_s *vpp_post)
{
	if (!vpp_post)
		return;
	if (vpp_index == VPP0 || vpp_index == PRE_VSYNC) {
		struct vpp0_post_s *vpp0_post;

		vpp0_post = &vpp_post->vpp0_post;
		/* vpp post in padding for oled */
		if (cur_dev->vpp_in_padding_support)
			vpp_post_in_padcut_set(vpp_index, vpp0_post);
		/* cfg slice mode */
		vpp_post_slice_set(vpp_index, vpp0_post);
		/* cfg vd1 hwin cut */
		vpp_vd1_hwin_set(vpp_index, vpp0_post);
		/* cfg vpp_blend */
		vpp_post_blend_set(vpp_index, &vpp0_post->vpp_post_blend);
		/* vpp post units set */
		vpp_post_proc_set(vpp_index, vpp0_post);
		/* cfg vpp_post pad if enable */
		vpp_post_padding_set(vpp_index, vpp0_post);
		/* cfg vpp_post hwin cut if expected vpp post
		 * dout hsize less than blend or pad hsize
		 */
		vpp_post_win_cut_set(vpp_index, vpp0_post);
	} else if (vpp_index == VPP1) {
		struct vpp1_post_s *vpp1_post;
		u32 vpp1_slice = 1;
		u32 align_fifo_size[POST_SLICE_NUM] = {2048, 1536, 1024, 512};
		/* rdma_wr_bits_op rdma_wr_bits = cur_dev->rdma_func[vpp_index].rdma_wr_bits; */
		struct vpp_post_misc_reg_s *vpp_reg = &vpp_post_reg.vpp_post_misc_reg;

		vpp1_post = &vpp_post->vpp1_post;
		/* slice mode */
		/* rdma_wr_bits(vpp_reg->vpp_obuf_ram_ctrl, 1, 0, 2); */
		if (!vpp1_post->vpp1_bypass_slice1) {
			/* slice1 vpp output need set */
			wr_slice_vpost(vpp_index, vpp_reg->vpp_out_h_v_size,
				vpp1_post->vpp1_post_blend.bld_out_w << 16 |
				vpp1_post->vpp1_post_blend.bld_out_h, vpp1_slice);
			wr_reg_bits_slice_vpost(vpp_index, vpp_reg->vpp_ofifo_size,
				0x800, 0, 14, vpp1_slice);
			/* slice1 hwin disable */
			wr_reg_bits_slice_vpost(vpp_index, vpp_reg->vpp_slc_deal_ctrl,
				0, 3, 1, vpp1_slice);
			wr_reg_bits_slice_vpost(vpp_index, vpp_reg->vpp_align_fifo_size,
						align_fifo_size[vpp1_slice],
						0, 14, vpp1_slice);
		}
		vpp1_post_blend_set(&vpp1_post->vpp1_post_blend);
	}
}

/* hw reg param info set */
static int vpp_post_in_padcut_param_set(struct vpp_post_input_s *vpp_input,
	struct vpp0_post_s *vpp_post)
{
	u32 vpp_post_in_pad_hsize = 0;

	if (!vpp_input || !vpp_post)
		return -1;
	/* need check padding or not */
	if (vd_layer[0].vpp_index == VPP0 ||
		vd_layer[0].vpp_index == PRE_VSYNC) {
		if (vpp_input->vpp_post_in_pad_en)
			vpp_post->vpp_pad_cut.cut_en = 1;
		else
			vpp_post->vpp_pad_cut.cut_en = 0;

		if (vpp_input->vpp_post_in_pad_en &&
			vpp_input->vpp_post_in_pad_hsize > 0)
			vpp_post->vpp_pad_cut.h_cut_en = 1;
		else
			vpp_post->vpp_pad_cut.h_cut_en = 0;

		if (vpp_input->vpp_post_in_pad_en &&
			vpp_input->vpp_post_in_pad_vsize > 0)
			vpp_post->vpp_pad_cut.v_cut_en = 1;
		else
			vpp_post->vpp_pad_cut.v_cut_en = 0;

		vpp_post_in_pad_hsize = roundup(vpp_input->vpp_post_in_pad_hsize, 2);
		vpp_post->vpp_pad_cut.cut_in_hsize = vpp_input->bld_out_hsize +
			vpp_post_in_pad_hsize;
		vpp_post->vpp_pad_cut.cut_in_vsize = vpp_input->bld_out_vsize +
			vpp_input->vpp_post_in_pad_vsize;

		if (vpp_input->down_move) {
			vpp_post->vpp_pad_cut.cut_v_bgn = 0;
			vpp_post->vpp_pad_cut.cut_v_end = vpp_input->bld_out_vsize - 1;
		} else {
			vpp_post->vpp_pad_cut.cut_v_bgn = vpp_input->vpp_post_in_pad_vsize;
			vpp_post->vpp_pad_cut.cut_v_end = vpp_post->vpp_pad_cut.cut_in_vsize - 1;
		}

		if (vpp_input->right_move) {
			vpp_post->vpp_pad_cut.cut_h_bgn = 0;
			vpp_post->vpp_pad_cut.cut_h_end = vpp_input->bld_out_hsize - 1;
		} else {
			vpp_post->vpp_pad_cut.cut_h_bgn = vpp_post_in_pad_hsize;
			vpp_post->vpp_pad_cut.cut_h_end = vpp_post->vpp_pad_cut.cut_in_hsize - 1;
		}
		if (debug_flag_s5 & DEBUG_VPP_POST) {
			pr_info("%s :cut_in h/v: %d, %d, vcut_en:%d, dir:%d, vcut pos: %d, %d hcut_en:%d, dir: %d, hcut pos: %d, %d\n",
				__func__,
				vpp_post->vpp_pad_cut.cut_in_hsize,
				vpp_post->vpp_pad_cut.cut_in_vsize,
				vpp_post->vpp_pad_cut.v_cut_en,
				vpp_input->down_move,
				vpp_post->vpp_pad_cut.cut_v_bgn,
				vpp_post->vpp_pad_cut.cut_v_end,
				vpp_post->vpp_pad_cut.h_cut_en,
				vpp_input->right_move,
				vpp_post->vpp_pad_cut.cut_h_bgn,
				vpp_post->vpp_pad_cut.cut_h_end);
		}
	}
	return 0;
}

static int vpp_post_hwincut_param_set(struct vpp_post_input_s *vpp_input,
	struct vpp0_post_s *vpp_post)
{
	if (!vpp_input || !vpp_post)
		return -1;
	/* need check vd1 padding or not */
	if (vpp_input->vd1_padding_en) {
		vpp_post->vd1_hwin.vd1_hwin_en = 1;
		vpp_post->vd1_hwin.vd1_hwin_in_hsize =
			vpp_input->vd1_size_after_padding;
		vpp_post->vd1_hwin.vd1_hwin_out_hsize =
			vpp_input->vd1_size_before_padding;
		vpp_post->vd1_hwin.vd1_win_vsize = vpp_input->din_vsize[0];
		vpp_post->vd1_hwin.slice_num = vpp_input->vd1_proc_slice;
		vpp_input->din_hsize[0] = vpp_post->vd1_hwin.vd1_hwin_out_hsize;
		if (debug_flag_s5 & DEBUG_VPP_POST)
			pr_info("%s:vd1 slice_num=%d, vd1 cut for padding:vd1_hwin_in_hsize:%d, out_hsize:%d\n",
				__func__,
				vpp_post->vd1_hwin.slice_num,
				vpp_post->vd1_hwin.vd1_hwin_in_hsize,
				vpp_post->vd1_hwin.vd1_hwin_out_hsize);
	} else {
		vpp_post->vd1_hwin.vd1_hwin_en = 0;
	}
	return 0;
}

/* following is vpp post parameters calc for hw setting */
static int vpp_blend_param_set(struct vpp_post_input_s *vpp_input,
	struct vpp_post_blend_s *vpp_post_blend)
{
	u32 padding_h_bgn = 0, padding_v_bgn = 0;

	if (!vpp_input || !vpp_post_blend)
		return -1;
	vpp_post_blend->bld_dummy_data = 0x008080;
	vpp_post_blend->bld_out_en = 1;

	/* 1:din0	2:din1 3:din2 4:din3 5:din4 else :close */
	if (vd_layer[0].vpp_index == VPP0 ||
		vd_layer[0].vpp_index == PRE_VSYNC) {
		if (vd_layer[0].post_blend_en)
			vpp_post_blend->bld_src1_sel = 1;
		else
			vpp_post_blend->bld_src1_sel = 0;
		if (vd_layer[1].post_blend_en)
			vpp_post_blend->bld_src2_sel = 2;
		else
			vpp_post_blend->bld_src2_sel = 0;
	} else if (vd_layer_vpp[0].vpp_index == VPP1) {
		if (vd_layer_vpp[0].post_blend_en)
			vpp_post_blend->bld_src1_sel = 1;
		else
			vpp_post_blend->bld_src1_sel = 0;
	}
#ifdef CHECK_LATER
	vpp_post_blend->bld_src3_sel = 0;
	vpp_post_blend->bld_src4_sel = 0;
	vpp_post_blend->bld_src5_sel = 0;
#endif
	vpp_post_blend->bld_out_w = vpp_input->bld_out_hsize;
	vpp_post_blend->bld_out_h = vpp_input->bld_out_vsize;

	if (vpp_input->vpp_post_in_pad_en) {
		vpp_post_blend->bld_out_padding_w = vpp_post_blend->bld_out_w +
			roundup(vpp_input->vpp_post_in_pad_hsize, 2);
		vpp_post_blend->bld_out_padding_h = vpp_post_blend->bld_out_h +
			vpp_input->vpp_post_in_pad_vsize;

		if (vpp_input->right_move)
			padding_h_bgn = vpp_input->vpp_post_in_pad_hsize;
		if (vpp_input->down_move)
			padding_v_bgn = vpp_input->vpp_post_in_pad_vsize;
		/* workaround for odd padding */
		if (vpp_input->vpp_post_in_pad_hsize & 1) {
			if (vpp_input->right_move)
				padding_h_bgn--;
			else
				padding_h_bgn++;
			if (debug_flag_s5 & DEBUG_VPP_POST)
				pr_info("padding_h_bgn adjust to->%d, bld_out_padding_w:%d\n",
					padding_h_bgn,
					vpp_post_blend->bld_out_padding_w);
		}
	} else {
		vpp_post_blend->bld_out_padding_w = vpp_post_blend->bld_out_w;
		vpp_post_blend->bld_out_padding_h = vpp_post_blend->bld_out_h;
	}
	vpp_post_blend->bld_din0_h_start = vpp_input->din_x_start[0] + padding_h_bgn;
	vpp_post_blend->bld_din0_h_end = vpp_post_blend->bld_din0_h_start +
		vpp_input->din_hsize[0] - 1;
	vpp_post_blend->bld_din0_v_start = vpp_input->din_y_start[0] + padding_v_bgn;
	vpp_post_blend->bld_din0_v_end = vpp_post_blend->bld_din0_v_start +
		vpp_input->din_vsize[0] - 1;
	vpp_post_blend->bld_din0_alpha = 0x100;
	vpp_post_blend->bld_din0_premult_en	= 1;

	vpp_post_blend->bld_din1_h_start = vpp_input->din_x_start[1] + padding_h_bgn;
	vpp_post_blend->bld_din1_h_end = vpp_post_blend->bld_din1_h_start +
		vpp_input->din_hsize[1] - 1;
	vpp_post_blend->bld_din1_v_start = vpp_input->din_y_start[1] + padding_v_bgn;
	vpp_post_blend->bld_din1_v_end = vpp_post_blend->bld_din1_v_start +
		vpp_input->din_vsize[1] - 1;
	vpp_post_blend->bld_din1_alpha = 0x100;
	vpp_post_blend->bld_din1_premult_en	= 0;

	vpp_post_blend->bld_din2_h_start = vpp_input->din_x_start[2] + padding_h_bgn;
	vpp_post_blend->bld_din2_h_end = vpp_post_blend->bld_din2_h_start +
		vpp_input->din_hsize[2] - 1;
	vpp_post_blend->bld_din2_v_start = vpp_input->din_y_start[2] + padding_v_bgn;
	vpp_post_blend->bld_din2_v_end = vpp_post_blend->bld_din2_v_start +
		vpp_input->din_vsize[2] - 1;
	vpp_post_blend->bld_din2_alpha = 0x100;
	vpp_post_blend->bld_din2_premult_en	= 0;

	vpp_post_blend->bld_din3_h_start = vpp_input->din_x_start[3] + padding_h_bgn;
	vpp_post_blend->bld_din3_h_end = vpp_post_blend->bld_din3_h_start +
		vpp_input->din_hsize[3] - 1;
	vpp_post_blend->bld_din3_v_start = vpp_input->din_x_start[3] + padding_v_bgn;
	vpp_post_blend->bld_din3_v_end = vpp_post_blend->bld_din3_v_start +
		vpp_input->din_vsize[3] - 1;
	vpp_post_blend->bld_din3_premult_en	= 0;

	vpp_post_blend->bld_din4_h_start = 0;
	vpp_post_blend->bld_din4_h_end = vpp_input->din_hsize[4] - 1;
	vpp_post_blend->bld_din4_v_start = 0;
	vpp_post_blend->bld_din4_v_end = vpp_input->din_vsize[4] - 1;
	vpp_post_blend->bld_din4_premult_en	= 0;
	if (debug_flag_s5 & DEBUG_VPP_POST)
		pr_info("vpp_post_blend:bld_out: %d, %d, after padding(%d, %d), bld_din0: %d, %d, %d, %d, bld_din1: %d, %d, %d, %d, bld_din3: %d, %d, %d, %d\n",
			vpp_post_blend->bld_out_w,
			vpp_post_blend->bld_out_h,
			vpp_post_blend->bld_out_padding_w,
			vpp_post_blend->bld_out_padding_h,
			vpp_post_blend->bld_din0_h_start,
			vpp_post_blend->bld_din0_h_end,
			vpp_post_blend->bld_din0_v_start,
			vpp_post_blend->bld_din0_v_end,
			vpp_post_blend->bld_din1_h_start,
			vpp_post_blend->bld_din1_h_end,
			vpp_post_blend->bld_din1_v_start,
			vpp_post_blend->bld_din1_v_end,
			vpp_post_blend->bld_din3_h_start,
			vpp_post_blend->bld_din3_h_end,
			vpp_post_blend->bld_din3_v_start,
			vpp_post_blend->bld_din3_v_end);
	return 0;
}

static int vpp1_blend_param_set(struct vpp_post_input_s *vpp1_input,
	struct vpp1_post_blend_s *vpp1_post_blend)
{
	static int first_config = true;
	int vpp1_osd_en = 0;
	struct vpp1_post_blend_reg_s *vpp_reg = &vpp_post_reg.vpp1_post_blend_reg;

	if (!vpp1_input || !vpp1_post_blend)
		return -1;
	memset(vpp1_post_blend, 0x0, sizeof(struct vpp1_post_blend_s));
	vpp1_post_blend->bld_dummy_data = 0x008080;

	if (first_config) {
		u32 vpp_postblend_ctrl;

		vpp_postblend_ctrl = READ_VCBUS_REG(vpp_reg->vpp_postblend_ctrl);
		if (vpp_postblend_ctrl & 0xf0)
			osd_vpp1_bld_ctrl = 1;
		first_config = false;
	}
	vpp1_osd_en = osd_vpp1_bld_ctrl;

	if (vd_layer_vpp[0].vppx_blend_en || vpp1_osd_en) {
		vpp1_post_blend->bld_out_en = 1;
		vpp1_post_blend->vd3_dpath_sel = 1;
	} else {
		vpp1_post_blend->bld_out_en = 0;
		vpp1_post_blend->vd3_dpath_sel = 0;
	}
	/* 1:din0(vd3)  2:din1(osd3) 3:din2 4:din3 5:din4 else :close */
	if (vd_layer_vpp[0].vppx_blend_en)
		vpp1_post_blend->bld_src1_sel = 1;
	else
		vpp1_post_blend->bld_src1_sel = 0;

	if (vpp1_osd_en)
		vpp1_post_blend->bld_src2_sel = 2;
	else
		vpp1_post_blend->bld_src2_sel = 0;

	vpp1_post_blend->bld_out_w = vpp1_input->bld_out_hsize;
	vpp1_post_blend->bld_out_h = vpp1_input->bld_out_vsize;

	vpp1_post_blend->bld_din0_h_start = vpp1_input->din_x_start[1];
	vpp1_post_blend->bld_din0_h_end = vpp1_post_blend->bld_din0_h_start +
		vpp1_input->din_hsize[1] - 1;
	vpp1_post_blend->bld_din0_v_start = vpp1_input->din_y_start[1];
	vpp1_post_blend->bld_din0_v_end = vpp1_post_blend->bld_din0_v_start +
		vpp1_input->din_vsize[1] - 1;
	vpp1_post_blend->bld_din0_alpha = 0x100;
	vpp1_post_blend->bld_din0_premult_en = 1;

	/* for osd input */
	vpp1_post_blend->bld_din1_alpha = 0x100;
	vpp1_post_blend->bld_din1_premult_en = 0;

	if (debug_flag_s5 & DEBUG_VPP1_POST)
		pr_info("vpp_post_blend:bld_out:en(%d) out: %d, %d,bld_din0: %d, %d, %d, %d\n",
			vpp1_post_blend->bld_out_en,
			vpp1_post_blend->bld_out_w,
			vpp1_post_blend->bld_out_h,
			vpp1_post_blend->bld_din0_h_start,
			vpp1_post_blend->bld_din0_h_end,
			vpp1_post_blend->bld_din0_v_start,
			vpp1_post_blend->bld_din0_v_end);
	return 0;
}

static int vpp_post_padding_param_set(struct vpp0_post_s *vpp_post)
{
	u32 bld_out_w;
	u32 padding_en = 0, pad_hsize = 0;

	if (!vpp_post)
		return -1;

	/* need check post blend out hsize */
	bld_out_w = vpp_post->vpp_post_blend.bld_out_w;
	if (cur_dev->vpp_in_padding_support) {
		padding_en = 1;
		pad_hsize = bld_out_w;
	} else {
		switch (vpp_post->slice_num) {
		case 4:
			/* bld out need 32 aligned if 4 slices */
			if (bld_out_w % 32) {
				padding_en = 1;
				pad_hsize = ALIGN(bld_out_w, 32);
			} else {
				padding_en = 0;
				pad_hsize = bld_out_w;
			}
			break;
		case 2:
			/* bld out need 8 aligned if 2 slices */
			if (bld_out_w % 8) {
				padding_en = 1;
				pad_hsize = ALIGN(bld_out_w, 8);
			} else {
				padding_en = 0;
				pad_hsize = bld_out_w;
			}
			break;
		case 1:
			padding_en = 0;
			pad_hsize = bld_out_w;
			break;
		default:
			pr_err("invalid slice_num[%d] number\n", vpp_post->slice_num);
			return -1;
		}
	}
	vpp_post->vpp_post_pad.vpp_post_pad_en = padding_en;
	vpp_post->vpp_post_pad.vpp_post_pad_hsize = pad_hsize;
	vpp_post->vpp_post_pad.vpp_post_pad_rpt_lcol = 1;
	return 0;
}

static int vpp_post_proc_slice_param_set(struct vpp_post_input_s *vpp_input,
	struct vpp0_post_s *vpp_post)
{
	u32 frm_hsize, frm_vsize;
	u32 slice_num, overlap_hsize;
	struct vpp_post_proc_slice_s *vpp_post_proc_slice = NULL;
	int i;

	if (!vpp_post)
		return -1;

	vpp_post_proc_slice = &vpp_post->vpp_post_proc.vpp_post_proc_slice;
	frm_hsize = vpp_post->vpp_post_pad.vpp_post_pad_hsize;
	frm_vsize = vpp_post->vpp_post_blend.bld_out_padding_h;
	slice_num = vpp_post->slice_num;
	overlap_hsize = vpp_post->overlap_hsize;
	switch (slice_num) {
	case 4:
		for (i = 0; i < POST_SLICE_NUM; i++) {
			if (i == 0 || i == 3)
				vpp_post_proc_slice->hsize[i] =
					(frm_hsize + POST_SLICE_NUM - 1) /
					POST_SLICE_NUM + overlap_hsize;
			else
				vpp_post_proc_slice->hsize[i] =
					(frm_hsize + POST_SLICE_NUM - 1) /
					POST_SLICE_NUM + overlap_hsize * 2;
			vpp_post_proc_slice->vsize[i] = frm_vsize;
			if (debug_flag_s5 & DEBUG_VPP_POST)
				pr_info("%s: slice(%d), slice hsize: %d, vsize:%d\n",
					__func__,
					i,
					vpp_post_proc_slice->hsize[i],
					vpp_post_proc_slice->vsize[i]);
		}
		break;
	case 2:
		for (i = 0; i < POST_SLICE_NUM; i++) {
			if (i < 2) {
				vpp_post_proc_slice->hsize[i] =
					(frm_hsize + 2 - 1) /
					2 + overlap_hsize;
				vpp_post_proc_slice->vsize[i] = frm_vsize;
				if (debug_flag_s5 & DEBUG_VPP_POST)
					pr_info("%s: slice(%d), slice hsize: %d, vsize:%d\n",
						__func__,
						i,
						vpp_post_proc_slice->hsize[i],
						vpp_post_proc_slice->vsize[i]);
			} else {
				vpp_post_proc_slice->hsize[i] = 0;
				vpp_post_proc_slice->vsize[i] = 0;
			}
		}
		break;
	case 1:
		for (i = 0; i < POST_SLICE_NUM; i++) {
			if (i == 0) {
				vpp_post_proc_slice->hsize[i] = frm_hsize;
				vpp_post_proc_slice->vsize[i] = frm_vsize;
				if (debug_flag_s5 & DEBUG_VPP_POST)
					pr_info("%s: slice(%d), slice hsize: %d, vsize:%d\n",
						__func__,
						i,
						vpp_post_proc_slice->hsize[i],
						vpp_post_proc_slice->vsize[i]);
			} else {
				vpp_post_proc_slice->hsize[i] = 0;
				vpp_post_proc_slice->vsize[i] = 0;
			}
		}
		break;
	default:
		break;
	}
	return 0;
}

static int vpp_post_proc_hwin_param_set(struct vpp0_post_s *vpp_post)
{
	u32 slice_num, overlap_hsize;
	struct vpp_post_proc_slice_s *vpp_post_proc_slice = NULL;
	struct vpp_post_proc_hwin_s *vpp_post_proc_hwin = NULL;
	int i;

	if (!vpp_post)
		return -1;
	vpp_post_proc_slice = &vpp_post->vpp_post_proc.vpp_post_proc_slice;
	vpp_post_proc_hwin = &vpp_post->vpp_post_proc.vpp_post_proc_hwin;
	slice_num = vpp_post->slice_num;
	overlap_hsize = vpp_post->overlap_hsize;

	switch (slice_num) {
	case 4:
		vpp_post_proc_hwin->hwin_en[0] = 1;
		vpp_post_proc_hwin->hwin_bgn[0] = 0;
		vpp_post_proc_hwin->hwin_end[0] =
			vpp_post_proc_slice->hsize[0] - overlap_hsize - 1;

		vpp_post_proc_hwin->hwin_en[1] = 1;
		vpp_post_proc_hwin->hwin_bgn[1] = overlap_hsize;
		vpp_post_proc_hwin->hwin_end[1] =
			vpp_post_proc_slice->hsize[1] - overlap_hsize - 1;

		vpp_post_proc_hwin->hwin_en[2] = 1;
		vpp_post_proc_hwin->hwin_bgn[2] = overlap_hsize;
		vpp_post_proc_hwin->hwin_end[2] =
			vpp_post_proc_slice->hsize[2] - overlap_hsize - 1;

		vpp_post_proc_hwin->hwin_en[3] = 1;
		vpp_post_proc_hwin->hwin_bgn[3] = overlap_hsize;
		vpp_post_proc_hwin->hwin_end[3] =
			vpp_post_proc_slice->hsize[3] - 1;
		break;
	case 2:
		vpp_post_proc_hwin->hwin_en[0] = 1;
		vpp_post_proc_hwin->hwin_bgn[0] = 0;
		vpp_post_proc_hwin->hwin_end[0] =
			vpp_post_proc_slice->hsize[0] - overlap_hsize - 1;

		vpp_post_proc_hwin->hwin_en[1] = 1;
		vpp_post_proc_hwin->hwin_bgn[1] = overlap_hsize;
		vpp_post_proc_hwin->hwin_end[1] =
			vpp_post_proc_slice->hsize[1] - 1;
		for (i = 2; i < POST_SLICE_NUM; i++) {
			vpp_post_proc_hwin->hwin_en[i] = 0;
			vpp_post_proc_hwin->hwin_bgn[i] = 0;
			vpp_post_proc_hwin->hwin_end[i] = 0;
		}
		break;
	case 1:
		vpp_post_proc_hwin->hwin_en[0] = 1;
		vpp_post_proc_hwin->hwin_bgn[0] = 0;
		vpp_post_proc_hwin->hwin_end[0] =
			vpp_post_proc_slice->hsize[0] - 1;
		for (i = 1; i < POST_SLICE_NUM; i++) {
			vpp_post_proc_hwin->hwin_en[i] = 0;
			vpp_post_proc_hwin->hwin_bgn[i] = 0;
			vpp_post_proc_hwin->hwin_end[i] = 0;
		}
		break;
	default:
		break;
	}
	return 0;
}

static int vpp_post_proc_param_set(struct vpp_post_input_s *vpp_input,
	struct vpp0_post_s *vpp_post)
{
	struct vpp_post_proc_s *vpp_post_proc = NULL;

	vpp_post_proc = &vpp_post->vpp_post_proc;
	vpp_post_proc_slice_param_set(vpp_input, vpp_post);
	vpp_post_proc_hwin_param_set(vpp_post);
	vpp_post_proc->align_fifo_size[0] = 2048;
	vpp_post_proc->align_fifo_size[1] = 1536;
	vpp_post_proc->align_fifo_size[2] = 1024;
	vpp_post_proc->align_fifo_size[3] = 512;
	return 0;
}

/* calc all related vpp_post_param */
int vpp_post_param_set(struct vpp_post_input_s *vpp_input,
	struct vpp0_post_s *vpp_post)
{
	int ret = 0;

	if (!vpp_input || !vpp_post)
		return -1;
	memset(vpp_post, 0, sizeof(struct vpp0_post_s));

	vpp_post->slice_num = vpp_input->slice_num;
	vpp_post->overlap_hsize = vpp_input->overlap_hsize;
	vpp_post_hwincut_param_set(vpp_input, vpp_post);

	ret = vpp_blend_param_set(vpp_input, &vpp_post->vpp_post_blend);
	if (ret < 0)
		return ret;

	ret = vpp_post_padding_param_set(vpp_post);
	if (ret < 0)
		return ret;

	ret = vpp_post_proc_param_set(vpp_input, vpp_post);
	if (ret < 0)
		return ret;
	vpp_post_in_padcut_param_set(vpp_input, vpp_post);

	return 0;
}

int vpp1_post_param_set(struct vpp_post_input_s *vpp_input,
	struct vpp1_post_s *vpp_post)
{
	int ret = 0;

	if (!vpp_input || !vpp_post)
		return -1;
	memset(vpp_post, 0, sizeof(struct vpp1_post_s));

	vpp_post->vpp1_en = true;
	vpp_post->vpp1_bypass_slice1 = false;
	if (g_vpp1_bypass_slice1 != 0xff)
		vpp_post->vpp1_bypass_slice1 = g_vpp1_bypass_slice1;
	if (vpp_post->vpp1_bypass_slice1)
		vpp_post->slice_num = 0;
	else
		vpp_post->slice_num = vpp_input->slice_num;
	vpp_post->overlap_hsize = vpp_input->overlap_hsize;

	ret = vpp1_blend_param_set(vpp_input, &vpp_post->vpp1_post_blend);
	if (ret < 0)
		return ret;
	vpp_post->vpp1_post_blend.vpp1_dpath_sel =
		vpp_post->vpp1_bypass_slice1 ? 0 : 1;

	g_vpp1_bypass_slice1_pre = vpp_post->vpp1_bypass_slice1;

	return 0;
}

/* need some logic to calc vpp_input */
int get_vpp_slice_num(const struct vinfo_s *info)
{
	int slice_num = 1;

	#ifdef AUTO_CAL
	/* 8k case 4 slice */
	if (info->width > 4096 && info->field_height > 2160)
		slice_num = 4;
	/* 4k120hz */
	else if (info->width == 3840 &&
		info->field_height == 2160 &&
		info->sync_duration_num / info->sync_duration_den > 60)
		slice_num = 2;
	else
		slice_num = 1;
	#else
	slice_num = info->cur_enc_ppc;
	#endif
	if (g_post_slice_num != 0xff)
		slice_num = g_post_slice_num;
	return slice_num;
}

static int check_vpp_info_changed(struct vpp_post_input_s *vpp_input, u8 vpp_index)
{
	int i = 0, changed = 0;

	/* check input param */
	for (i = 0; i < 4; i++) {
		if (vpp_input->din_hsize[i] != g_vpp_input_pre.din_hsize[i] ||
			vpp_input->din_vsize[i] != g_vpp_input_pre.din_vsize[i] ||
			vpp_input->din_x_start[i] != g_vpp_input_pre.din_x_start[i] ||
			vpp_input->din_y_start[i] != g_vpp_input_pre.din_y_start[i]) {
			changed = 1;
			if (debug_flag_s5 & DEBUG_VPP_POST)
				pr_info("%s %d hit vpp_input vd[%d]:new:%d, %d, %d, %d, pre: %d, %d, %d, %d\n",
				__func__, vpp_index,
			i,
			vpp_input->din_x_start[i],
			vpp_input->din_y_start[i],
			vpp_input->din_hsize[i],
			vpp_input->din_vsize[i],
			g_vpp_input_pre.din_x_start[i],
			g_vpp_input_pre.din_y_start[i],
			g_vpp_input_pre.din_hsize[i],
			g_vpp_input_pre.din_vsize[i]);
			break;
		}
	}
	/* check output param */
	if (!changed) {
		if (vpp_input->slice_num != g_vpp_input_pre.slice_num ||
			vpp_input->bld_out_hsize != g_vpp_input_pre.bld_out_hsize ||
			vpp_input->bld_out_vsize != g_vpp_input_pre.bld_out_vsize) {
			changed = 1;
			if (debug_flag_s5 & DEBUG_VPP_POST)
				pr_info("vpp_index=%d:hit vpp_input->slice_num=%d, %d, %d, %d, %d, %d\n",
					vpp_index,
					vpp_input->slice_num,
					vpp_input->bld_out_hsize,
					vpp_input->bld_out_vsize,
					g_vpp_input_pre.slice_num,
					g_vpp_input_pre.bld_out_hsize,
					g_vpp_input_pre.bld_out_vsize);
		}
	}
	if (!changed) {
		if (vpp_input->vd1_padding_en != g_vpp_input_pre.vd1_padding_en ||
			vpp_input->vd1_size_before_padding !=
			g_vpp_input_pre.vd1_size_before_padding ||
			vpp_input->vd1_size_after_padding !=
			g_vpp_input_pre.vd1_size_after_padding) {
			changed = 1;
			pr_info("vpp_index=%d: hit vpp_input->vd1_padding_en=%d, %d, before padding: %d, %d, after padding: %d, %d\n",
				vpp_index,
				vpp_input->vd1_padding_en,
				g_vpp_input_pre.vd1_padding_en,
				vpp_input->vd1_size_before_padding,
				vpp_input->vd1_size_after_padding,
				g_vpp_input_pre.vd1_size_before_padding,
				g_vpp_input_pre.vd1_size_after_padding);
		}
	}

	/* check padding pram */
	if (cur_dev->vpp_in_padding_support) {
		if (vpp_input->vpp_post_in_pad_en !=
			g_vpp_input_pre.vpp_post_in_pad_en ||
			vpp_input->vpp_post_in_pad_hsize !=
			g_vpp_input_pre.vpp_post_in_pad_hsize ||
			vpp_input->vpp_post_in_pad_vsize !=
			g_vpp_input_pre.vpp_post_in_pad_vsize ||
			g_vpp_input_pre.down_move != vpp_input->down_move ||
			g_vpp_input_pre.right_move != vpp_input->right_move) {
			changed = 1;
			if (debug_flag_s5 & DEBUG_VPP_POST)
				pr_info("vpp_index=%d:hit vpp_input->vpp_post_in_pad_en=%d, %d, %d, %d, %d, %d\n",
					vpp_index,
					vpp_input->vpp_post_in_pad_en,
					vpp_input->vpp_post_in_pad_hsize,
					vpp_input->vpp_post_in_pad_vsize,
					g_vpp_input_pre.vpp_post_in_pad_en,
					g_vpp_input_pre.vpp_post_in_pad_hsize,
					g_vpp_input_pre.vpp_post_in_pad_vsize);
		}
	}
	memcpy(&g_vpp_input, vpp_input, sizeof(struct vpp_post_input_s));
	memcpy(&g_vpp_input_pre, vpp_input, sizeof(struct vpp_post_input_s));
	return changed;
}

struct vpp_post_input_s *get_vpp_input_info(void)
{
	return &g_vpp_input;
}

int update_vpp_input_info(const struct vinfo_s *info, u8 vpp_index)
{
	int update = 0;
	struct vd_proc_s *vd_proc;
	struct vpp_post_input_s vpp_input;
	struct vd_proc_vd1_info_s *vd_proc_vd1_info;
	struct vd_proc_vd2_info_s *vd_proc_vd2_info;
	struct vpp_postblend_scope_s scope;
	static u32 din_hsize_pre[2] = {0};
	static u32 din_vsize_pre[2] = {0};
	static u32 din_x_start_pre[2] = {0};
	static u32 din_y_start_pre[2] = {0};
	u32 din_hsize = 0, din_vsize = 0, din_x_start = 0, din_y_start = 0;

	if (!info)
		return 0;
	memset(&scope, 0, sizeof(scope));
	memset(&vpp_input, 0, sizeof(vpp_input));
	vpp_input.slice_num = get_vpp_slice_num(info);
	vpp_input.overlap_hsize = g_post_overlap_size;
	vpp_input.bld_out_hsize = info->width;
	vpp_input.bld_out_vsize = info->field_height;
	vpp_input.vd1_proc_slice = 1;
	/* need set vdx and osd input size */
	/* for hard code test */
	/* vd1 vd2 vd3 osd1 osd2 */
	vd_proc = get_vd_proc_info();
	vd_proc_vd1_info = &vd_proc->vd_proc_vd1_info;
	vd_proc_vd2_info = &vd_proc->vd_proc_vd2_info;
	/* vd1 */
	if (vd_proc_vd1_info->vd1_slices_dout_dpsel == VD1_SLICES_DOUT_4S4P) {
		if (vd_proc_vd1_info->vd1_work_mode == VD1_2_2SLICES_MODE) {
			din_hsize = SIZE_ALIG32(vd_proc_vd1_info->vd1_whole_hsize);
			din_vsize = vd_proc_vd1_info->vd1_whole_vsize;
			din_x_start = vd_proc_vd1_info->vd1_whole_dout_x_start;
			din_y_start = vd_proc_vd1_info->vd1_whole_dout_y_start;
		} else {
			din_hsize = SIZE_ALIG32(vd_proc_vd1_info->vd1_dout_hsize[0]);
			din_vsize = vd_proc_vd1_info->vd1_dout_vsize[0];
			din_x_start = vd_proc_vd1_info->vd1_dout_x_start[0];
			din_y_start = vd_proc_vd1_info->vd1_dout_y_start[0];
		}
	} else if (vd_proc_vd1_info->vd1_slices_dout_dpsel == VD1_SLICES_DOUT_2S4P) {
		if (video_is_meson_s5_cpu())
			din_hsize = SIZE_ALIG16(vd_proc_vd1_info->vd1_dout_hsize[0]);
		else
			din_hsize = vd_proc_vd1_info->vd1_dout_hsize[0];
		din_vsize = vd_proc_vd1_info->vd1_dout_vsize[0];
		din_x_start = vd_proc_vd1_info->vd1_dout_x_start[0];
		din_y_start = vd_proc_vd1_info->vd1_dout_y_start[0];
	} else {
		if (vd_proc->vd_proc_pi.pi_en) {
			din_hsize = vd_proc->vd_proc_blend.bld_out_w * 2;
			din_vsize = vd_proc->vd_proc_blend.bld_out_h * 2;
			din_x_start = 0;
			din_y_start = 0;
		} else {
			din_hsize = vd_proc->vd_proc_blend.bld_out_w;
			din_vsize = vd_proc->vd_proc_blend.bld_out_h;
			din_x_start = vd_proc_vd1_info->vd1_dout_x_start[0];
			din_y_start = vd_proc_vd1_info->vd1_dout_y_start[0];
		}
	}

	/* check vd1, update size when vpp_index matched */
	if (vpp_index == vd_layer[0].vpp_index) {
		vpp_input.din_hsize[0] = din_hsize;
		vpp_input.din_vsize[0] = din_vsize;
		vpp_input.din_x_start[0] = din_x_start;
		vpp_input.din_y_start[0] = din_y_start;
	} else {
		vpp_input.din_hsize[0] = din_hsize_pre[0];
		vpp_input.din_vsize[0] = din_vsize_pre[0];
		vpp_input.din_x_start[0] = din_x_start_pre[0];
		vpp_input.din_y_start[0] = din_y_start_pre[0];
	}
	din_hsize_pre[0] = din_hsize;
	din_vsize_pre[0] = din_vsize;
	din_x_start_pre[0] = din_x_start;
	din_y_start_pre[0] = din_y_start;

	/* check vd2, update size when vpp_index matched */
	if (vpp_index == vd_layer[1].vpp_index) {
		vpp_input.din_hsize[1] = vd_proc->vd2_proc.dout_hsize;
		vpp_input.din_vsize[1] = vd_proc->vd2_proc.dout_vsize;
		vpp_input.din_x_start[1] = vd_proc->vd2_proc.vd2_dout_x_start;
		vpp_input.din_y_start[1] = vd_proc->vd2_proc.vd2_dout_y_start;
	} else {
		vpp_input.din_hsize[1] = din_hsize_pre[1];
		vpp_input.din_vsize[1] = din_vsize_pre[1];
		vpp_input.din_x_start[1] = din_x_start_pre[1];
		vpp_input.din_y_start[1] = din_y_start_pre[1];
	}
	din_hsize_pre[1] = vd_proc->vd2_proc.dout_hsize;
	din_vsize_pre[1] = vd_proc->vd2_proc.dout_vsize;
	din_x_start_pre[1] = vd_proc->vd2_proc.vd2_dout_x_start;
	din_y_start_pre[1] = vd_proc->vd2_proc.vd2_dout_y_start;

	/* vd3 */
	vpp_input.din_hsize[2] = 0;
	vpp_input.din_vsize[2] = 0;

	/* osd1 */
	if (cur_dev->vpp_in_padding_support) {
		if (get_vpp_din3_scope) {
			get_vpp_din3_scope(&scope);
			vpp_input.din_x_start[3] = scope.h_start;
			vpp_input.din_y_start[3] = scope.v_start;
			vpp_input.din_hsize[3] = scope.h_end - scope.h_start + 1;
			vpp_input.din_vsize[3] = scope.v_end - scope.v_start + 1;
		} else {
			vpp_input.din_x_start[3] = 0;
			vpp_input.din_y_start[3] = 0;
			vpp_input.din_hsize[3] = vpp_input.bld_out_hsize;
			vpp_input.din_vsize[3] = vpp_input.bld_out_vsize;
		}
	} else {
		vpp_input.din_hsize[3] = 0;
		vpp_input.din_vsize[3] = 0;
	}
	/* osd2 */
	vpp_input.din_hsize[4] = 0;
	vpp_input.din_vsize[4] = 0;

	if (vd_proc_vd1_info->vd1_slices_dout_dpsel == VD1_SLICES_DOUT_4S4P ||
		vd_proc_vd1_info->vd1_slices_dout_dpsel == VD1_SLICES_DOUT_2S4P) {
		vpp_input.vd1_padding_en = 0;

		if (vd_proc_vd1_info->vd1_work_mode == VD1_2_2SLICES_MODE)
			vpp_input.vd1_size_before_padding = vd_proc_vd1_info->vd1_whole_hsize;
		else
			vpp_input.vd1_size_before_padding = vd_proc_vd1_info->vd1_dout_hsize[0];
		if (video_is_meson_s5_cpu())
			vpp_input.vd1_size_after_padding = vpp_input.din_hsize[0];
		else
			vpp_input.vd1_size_after_padding =
				SIZE_ALIG32(vpp_input.din_hsize[0]);
		if (vpp_input.vd1_size_before_padding !=
			vpp_input.vd1_size_after_padding)
			vpp_input.vd1_padding_en = 1;
		if (vd_proc_vd1_info->vd1_slices_dout_dpsel == VD1_SLICES_DOUT_4S4P)
			vpp_input.vd1_proc_slice = 4;
		else if (vd_proc_vd1_info->vd1_slices_dout_dpsel == VD1_SLICES_DOUT_2S4P) {
			vpp_input.vd1_proc_slice = 2;
		}
	} else {
		vpp_input.vd1_padding_en = 0;
		vpp_input.vd1_size_before_padding = vpp_input.din_hsize[0];
		vpp_input.vd1_size_after_padding = vpp_input.din_hsize[0];
	}
	/* vpp post in padding */
	vpp_input.vpp_post_in_pad_en = g_vpp_in_padding.vpp_post_in_pad_en;
	vpp_input.vpp_post_in_pad_hsize = g_vpp_in_padding.vpp_post_in_pad_hsize;
	vpp_input.vpp_post_in_pad_vsize = g_vpp_in_padding.vpp_post_in_pad_vsize;
	vpp_input.down_move = g_vpp_in_padding.down_move;
	vpp_input.right_move = g_vpp_in_padding.right_move;
	update = check_vpp_info_changed(&vpp_input, vpp_index);
	return update;
}

static int check_vpp1_info_changed(struct vpp_post_input_s *vpp_input)
{
	int i = 0, changed = 0;

	/* check input param for vd3(vd proc vd2)*/
	for (i = 1; i < 2; i++) {
		if (vpp_input->din_hsize[i] != g_vpp1_input_pre.din_hsize[i] ||
			vpp_input->din_vsize[i] != g_vpp1_input_pre.din_vsize[i] ||
			vpp_input->din_x_start[i] != g_vpp1_input_pre.din_x_start[i] ||
			vpp_input->din_y_start[i] != g_vpp1_input_pre.din_y_start[i]) {
			changed = 1;
			if (debug_flag_s5 & DEBUG_VPP1_POST)
				pr_info("%s hit vpp_input vd[%d]:new:%d, %d, %d, %d, pre: %d, %d, %d, %d\n",
				__func__,
				i,
				vpp_input->din_x_start[i],
				vpp_input->din_y_start[i],
				vpp_input->din_hsize[i],
				vpp_input->din_vsize[i],
				g_vpp1_input_pre.din_x_start[i],
				g_vpp1_input_pre.din_y_start[i],
				g_vpp1_input_pre.din_hsize[i],
				g_vpp1_input_pre.din_vsize[i]);
			break;
		}
	}
	/* check output param */
	if (!changed) {
		if (vpp_input->slice_num != g_vpp1_input_pre.slice_num ||
			vpp_input->bld_out_hsize != g_vpp1_input_pre.bld_out_hsize ||
			vpp_input->bld_out_vsize != g_vpp1_input_pre.bld_out_vsize) {
			changed = 1;
			if (debug_flag_s5 & DEBUG_VPP1_POST)
				pr_info("hit vpp_input->slice_num=%d, %d, %d, %d, %d, %d\n",
					vpp_input->slice_num,
					vpp_input->bld_out_hsize,
					vpp_input->bld_out_vsize,
					g_vpp1_input_pre.slice_num,
					g_vpp1_input_pre.bld_out_hsize,
					g_vpp1_input_pre.bld_out_vsize);
		}
	}
	/* check other param */
	if (!changed) {
		if (g_vpp1_bypass_slice1 != 0xff &&
		    g_vpp1_bypass_slice1 != g_vpp1_bypass_slice1_pre) {
			changed = 1;
			if (debug_flag_s5 & DEBUG_VPP1_POST)
				pr_info("%s hit vpp1_bypass_slice1=%d\n",
					__func__,
					g_vpp1_bypass_slice1);
		}
	}
	memcpy(&g_vpp1_input, vpp_input, sizeof(struct vpp_post_input_s));
	memcpy(&g_vpp1_input_pre, vpp_input, sizeof(struct vpp_post_input_s));
	return changed;
}

struct vpp_post_input_s *get_vpp1_input_info(void)
{
	return &g_vpp1_input;
}

int update_vpp1_input_info(const struct vinfo_s *info)
{
	int update = 0;
	struct vd_proc_s *vd_proc;
	struct vpp_post_input_s vpp1_input;
	struct vd_proc_vd2_info_s *vd_proc_vd2_info;

	memset(&vpp1_input, 0, sizeof(vpp1_input));
	vpp1_input.slice_num = 1;
	vpp1_input.overlap_hsize = 0;
	vpp1_input.bld_out_hsize = info->width;
	vpp1_input.bld_out_vsize = info->field_height;
	vpp1_input.vd1_proc_slice = 1;
	/* need set vdx and osd input size */
	/* for hard code test */
	/* vd1 vd2 vd3 osd1 osd2 */
	vd_proc = get_vd_proc_info();
	vd_proc_vd2_info = &vd_proc->vd_proc_vd2_info;

	/* vd3 */
	vpp1_input.din_hsize[1] = vd_proc->vd2_proc.dout_hsize;
	vpp1_input.din_vsize[1] = vd_proc->vd2_proc.dout_vsize;
	vpp1_input.din_x_start[1] = vd_proc->vd2_proc.vd2_dout_x_start;
	vpp1_input.din_y_start[1] = vd_proc->vd2_proc.vd2_dout_y_start;

	vpp1_input.vd1_padding_en = 0;
	vpp1_input.vd1_size_before_padding = vpp1_input.din_hsize[0];
	vpp1_input.vd1_size_after_padding = vpp1_input.din_hsize[0];

	update = check_vpp1_info_changed(&vpp1_input);
	return update;
}

void get_vpp_in_padding_axis(u32 *enable, int *h_padding, int *v_padding)
{
	if (cur_dev->vpp_in_padding_support) {
		*enable = g_vpp_in_padding.vpp_post_in_pad_en;
		*h_padding = g_vpp_in_padding.h_padding;
		*v_padding = g_vpp_in_padding.v_padding;
	} else {
		*enable = 0;
		*h_padding = 0;
		*v_padding = 0;
	}
}
EXPORT_SYMBOL(get_vpp_in_padding_axis);

void set_vpp_in_padding_axis(u32 enable, int h_padding, int v_padding)
{
	if (cur_dev->vpp_in_padding_support) {
		g_vpp_in_padding.vpp_post_in_pad_en = enable;
		g_vpp_in_padding.h_padding = h_padding;
		g_vpp_in_padding.v_padding = v_padding;

		if (h_padding > 0) {
			g_vpp_in_padding.right_move = 1;
			g_vpp_in_padding.vpp_post_in_pad_hsize = h_padding;
		} else {
			g_vpp_in_padding.right_move = 0;
			g_vpp_in_padding.vpp_post_in_pad_hsize = -h_padding;
		}
		if (v_padding > 0) {
			g_vpp_in_padding.down_move = 1;
			if (v_padding > 16) {
				pr_err("%s: v_padding=%d > 16(max)\n",
					__func__, v_padding);
				v_padding = 16;
			}
			g_vpp_in_padding.vpp_post_in_pad_vsize = v_padding;
		} else {
			g_vpp_in_padding.down_move = 0;
			if (v_padding < -16) {
				pr_err("%s: v_padding=%d < -16(min)\n",
					__func__, v_padding);
				v_padding = -16;
			}

			g_vpp_in_padding.vpp_post_in_pad_vsize = -v_padding;
		}
		if (debug_flag_s5 & DEBUG_VPP_POST)
			pr_info("right_move:%d, pad_hsize:%d, down_move:%d, pad_vsize:%d\n",
				g_vpp_in_padding.right_move,
				g_vpp_in_padding.vpp_post_in_pad_hsize,
				g_vpp_in_padding.down_move,
				g_vpp_in_padding.vpp_post_in_pad_vsize);
	}
}
EXPORT_SYMBOL(set_vpp_in_padding_axis);

void vpp_clip_setting_s5(u8 vpp_index, struct clip_setting_s *setting)
{
	u32 slice_num = 0;
	struct vpp_post_misc_reg_s *vpp_misc_reg = &vpp_post_reg.vpp_post_misc_reg;
	const struct vinfo_s *info;
	int i;

	info = get_current_vinfo();
	slice_num = get_vpp_slice_num(info);
	for (i = 0; i < slice_num; i++) {
		wr_slice_vpost(vpp_index, vpp_misc_reg->vpp_clip_misc0,
			setting->clip_max, i);
		wr_slice_vpost(vpp_index, vpp_misc_reg->vpp_clip_misc1,
			setting->clip_min, i);
		if (!(setting->clip_max == 0x3fffffff &&
			setting->clip_min == 0x0)) {
			wr_slice_vpost_vcbus(vpp_misc_reg->vpp_clip_misc0,
				setting->clip_max, i);
			wr_slice_vpost_vcbus(vpp_misc_reg->vpp_clip_misc1,
				setting->clip_min, i);
		}
	}
}

int register_vpp_postblend_info_func(void (*get_vpp_osd1_scope)
	(struct vpp_postblend_scope_s *scope))
{
	get_vpp_din3_scope = get_vpp_osd1_scope;
	return 0;
}
EXPORT_SYMBOL(register_vpp_postblend_info_func);

