// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * drivers/amlogic/media/video_sink/video_common.c
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

u32 is_crop_left_odd(struct vpp_frame_par_s *frame_par)
{
	int crop_left_odd;

	/* odd, not even aligned*/
	if (frame_par->VPP_hd_start_lines_ & 0x01)
		crop_left_odd = 1;
	else
		crop_left_odd = 0;
	return crop_left_odd;
}

void get_pre_hscaler_para(u8 layer_id, int *ds_ratio, int *flt_num)
{
	switch (pre_scaler[layer_id].pre_hscaler_ntap) {
	case PRE_HSCALER_2TAP:
		*ds_ratio = pre_scaler[layer_id].pre_hscaler_rate;
		*flt_num = 2;
		break;
	case PRE_HSCALER_4TAP:
		*ds_ratio = pre_scaler[layer_id].pre_hscaler_rate;
		*flt_num = 4;
		break;
	case PRE_HSCALER_6TAP:
		*ds_ratio = pre_scaler[layer_id].pre_hscaler_rate;
		*flt_num = 6;
		break;
	case PRE_HSCALER_8TAP:
		*ds_ratio = pre_scaler[layer_id].pre_hscaler_rate;
		*flt_num = 8;
		break;
	}
}

void get_pre_vscaler_para(u8 layer_id, int *ds_ratio, int *flt_num)
{
	switch (pre_scaler[layer_id].pre_vscaler_ntap) {
	case PRE_VSCALER_2TAP:
		*ds_ratio = pre_scaler[layer_id].pre_vscaler_rate;
		*flt_num = 2;
		break;
	case PRE_VSCALER_4TAP:
		*ds_ratio = pre_scaler[layer_id].pre_vscaler_rate;
		*flt_num = 4;
		break;
	}
}

void get_pre_hscaler_coef(u8 layer_id, int *pre_hscaler_table)
{
	if (pre_scaler[layer_id].pre_hscaler_coef_set) {
		pre_hscaler_table[0] = pre_scaler[layer_id].pre_hscaler_coef[0];
		pre_hscaler_table[1] = pre_scaler[layer_id].pre_hscaler_coef[1];
		pre_hscaler_table[2] = pre_scaler[layer_id].pre_hscaler_coef[2];
		pre_hscaler_table[3] = pre_scaler[layer_id].pre_hscaler_coef[3];
	} else {
		switch (pre_scaler[layer_id].pre_hscaler_ntap) {
		case PRE_HSCALER_2TAP:
			pre_hscaler_table[0] = 0x100;
			pre_hscaler_table[1] = 0x0;
			pre_hscaler_table[2] = 0x0;
			pre_hscaler_table[3] = 0x0;
			break;
		case PRE_HSCALER_4TAP:
			pre_hscaler_table[0] = 0xc0;
			pre_hscaler_table[1] = 0x40;
			pre_hscaler_table[2] = 0x0;
			pre_hscaler_table[3] = 0x0;
			break;
		case PRE_HSCALER_6TAP:
			pre_hscaler_table[0] = 0x9c;
			pre_hscaler_table[1] = 0x44;
			pre_hscaler_table[2] = 0x20;
			pre_hscaler_table[3] = 0x0;
			break;
		case PRE_HSCALER_8TAP:
			pre_hscaler_table[0] = 0x90;
			pre_hscaler_table[1] = 0x40;
			pre_hscaler_table[2] = 0x20;
			pre_hscaler_table[3] = 0x10;
			break;
		}
	}
}

void get_pre_vscaler_coef(u8 layer_id, int *pre_hscaler_table)
{
	if (pre_scaler[layer_id].pre_vscaler_coef_set) {
		pre_hscaler_table[0] = pre_scaler[layer_id].pre_vscaler_coef[0];
		pre_hscaler_table[1] = pre_scaler[layer_id].pre_vscaler_coef[1];
	} else {
		switch (pre_scaler[layer_id].pre_vscaler_ntap) {
		case PRE_VSCALER_2TAP:
			if (has_pre_hscaler_8tap(0)) {
				pre_hscaler_table[0] = 0x100;
				pre_hscaler_table[1] = 0x0;
			} else {
				pre_hscaler_table[0] = 0x40;
				pre_hscaler_table[1] = 0x0;
			}
			break;
		case PRE_VSCALER_4TAP:
			if (has_pre_hscaler_8tap(0)) {
				pre_hscaler_table[0] = 0xc0;
				pre_hscaler_table[1] = 0x40;
			} else {
				pre_hscaler_table[0] = 0xf8;
				pre_hscaler_table[1] = 0x48;
			}
			break;
		}
	}
}

u32 viu_line_stride(u32 buffr_width)
{
	u32 line_stride;

	/* input: buffer width not hsize */
	/* 1 stride = 16 byte */
	line_stride = (buffr_width + 15) / 16;
	return line_stride;
}

void init_layer_canvas(struct video_layer_s *layer,
			      u32 start_canvas)
{
	u32 i, j;

	if (!layer)
		return;

	for (i = 0; i < CANVAS_TABLE_CNT; i++) {
		for (j = 0; j < 6; j++)
			layer->canvas_tbl[i][j] =
				start_canvas++;
		layer->disp_canvas[i][0] =
			(layer->canvas_tbl[i][2] << 16) |
			(layer->canvas_tbl[i][1] << 8) |
			layer->canvas_tbl[i][0];
		layer->disp_canvas[i][1] =
			(layer->canvas_tbl[i][5] << 16) |
			(layer->canvas_tbl[i][4] << 8) |
			layer->canvas_tbl[i][3];
	}
}

void vframe_canvas_set(struct canvas_config_s *config,
	u32 planes,
	u32 *index)
{
	int i;
	u32 *canvas_index = index;

	struct canvas_config_s *cfg = config;

	for (i = 0; i < planes; i++, canvas_index++, cfg++)
		canvas_config_config(*canvas_index, cfg);
}

bool is_layer_aisr_supported(struct video_layer_s *layer)
{
	/* only vd1 has aisr for t3 */
	if (!layer || layer->layer_id != 0)
		return false;
	else
		return true;
}

static void dump_vout_blend_reg(void)
{
	u32 reg_addr, reg_val = 0;

	if (cur_dev->display_module != C3_DISPLAY_MODULE)
		return;
	pr_info("vout blend reg:\n");
	reg_addr = VPU_VOUT_BLEND_CTRL;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("VPU_VOUT_BLEND_CTRL[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = VPU_VOUT_BLEND_DUMDATA;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("VPU_VOUT_BLEND_DUMDATA[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = VPU_VOUT_BLEND_SIZE;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("VPU_VOUT_BLEND_SIZE[0x%x] = 0x%X\n",
		   reg_addr, reg_val);

	reg_addr = VPU_VOUT_BLD_SRC0_HPOS;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("VPU_VOUT_BLD_SRC0_HPOS[0x%x](vd1) = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = VPU_VOUT_BLD_SRC0_VPOS;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("VPU_VOUT_BLD_SRC0_VPOS[0x%x](vd1) = 0x%X\n",
		   reg_addr, reg_val);

	reg_addr = VPU_VOUT_BLD_SRC1_HPOS;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("VPU_VOUT_BLD_SRC1_HPOS[0x%x](osd) = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = VPU_VOUT_BLD_SRC1_VPOS;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("VPU_VOUT_BLD_SRC1_VPOS[0x%x](osd) = 0x%X\n",
		   reg_addr, reg_val);

	pr_info("vout top reg:\n");
	reg_addr = VPU_VOUT_TOP_CTRL;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("VPU_VOUT_TOP_CTRL[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = VPU_VOUT_SECURE_BIT_NOR;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("VPU_VOUT_SECURE_BIT_NOR[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = VPU_VOUT_SECURE_DATA;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("VPU_VOUT_SECURE_DATA[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = VPU_VOUT_FRM_CTRL;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("VPU_VOUT_FRM_CTRL[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = VPU_VOUT_IRQ_CTRL;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("VPU_VOUT_IRQ_CTRL[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = VPU_VOUT_VLK_CTRL;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("VPU_VOUT_VLK_CTRL[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
}

static void dump_mif_reg(void)
{
	int i;
	u32 reg_addr, reg_val = 0;

	for (i = 0; i < cur_dev->max_vd_layers; i++) {
		pr_info("vd%d mif regs:\n", i);
		reg_addr = vd_layer[i].vd_mif_reg.vd_if0_gen_reg;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].vd_mif_reg.vd_if0_canvas0;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].vd_mif_reg.vd_if0_canvas1;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].vd_mif_reg.vd_if0_luma_x0;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].vd_mif_reg.vd_if0_luma_y0;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].vd_mif_reg.vd_if0_chroma_x0;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].vd_mif_reg.vd_if0_chroma_y0;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].vd_mif_reg.vd_if0_luma_x1;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].vd_mif_reg.vd_if0_luma_y1;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].vd_mif_reg.vd_if0_chroma_x1;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].vd_mif_reg.vd_if0_chroma_y1;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].vd_mif_reg.vd_if0_rpt_loop;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].vd_mif_reg.vd_if0_luma0_rpt_pat;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].vd_mif_reg.vd_if0_chroma0_rpt_pat;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].vd_mif_reg.vd_if0_luma1_rpt_pat;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].vd_mif_reg.vd_if0_chroma1_rpt_pat;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].vd_mif_reg.vd_if0_luma_psel;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].vd_mif_reg.vd_if0_chroma_psel;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].vd_mif_reg.vd_if0_luma_fifo_size;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].vd_mif_reg.vd_if0_gen_reg2;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].vd_mif_reg.vd_if0_gen_reg3;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].vd_mif_reg.viu_vd_fmt_ctrl;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].vd_mif_reg.viu_vd_fmt_w;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);

		if (cur_dev->mif_linear) {
			pr_info("vd%d mif linear regs:\n", i);
			reg_addr = vd_layer[i].vd_mif_linear_reg.vd_if0_baddr_y;
			reg_val = READ_VCBUS_REG(reg_addr);
			pr_info("[0x%x] = 0x%X\n",
				   reg_addr, reg_val);
			reg_addr = vd_layer[i].vd_mif_linear_reg.vd_if0_baddr_cb;
			reg_val = READ_VCBUS_REG(reg_addr);
			pr_info("[0x%x] = 0x%X\n",
				   reg_addr, reg_val);
			reg_addr = vd_layer[i].vd_mif_linear_reg.vd_if0_baddr_cr;
			reg_val = READ_VCBUS_REG(reg_addr);
			pr_info("[0x%x] = 0x%X\n",
				   reg_addr, reg_val);
			reg_addr = vd_layer[i].vd_mif_linear_reg.vd_if0_stride_0;
			reg_val = READ_VCBUS_REG(reg_addr);
			pr_info("[0x%x] = 0x%X\n",
				   reg_addr, reg_val);
			reg_addr = vd_layer[i].vd_mif_linear_reg.vd_if0_stride_1;
			reg_val = READ_VCBUS_REG(reg_addr);
			pr_info("[0x%x] = 0x%X\n",
				   reg_addr, reg_val);
			reg_addr = vd_layer[i].vd_mif_linear_reg.vd_if0_baddr_y_f1;
			reg_val = READ_VCBUS_REG(reg_addr);
			pr_info("[0x%x] = 0x%X\n",
				   reg_addr, reg_val);
			reg_addr = vd_layer[i].vd_mif_linear_reg.vd_if0_baddr_cb_f1;
			reg_val = READ_VCBUS_REG(reg_addr);
			pr_info("[0x%x] = 0x%X\n",
				   reg_addr, reg_val);

			reg_addr = vd_layer[i].vd_mif_linear_reg.vd_if0_baddr_cr_f1;
			reg_val = READ_VCBUS_REG(reg_addr);
			pr_info("[0x%x] = 0x%X\n",
				   reg_addr, reg_val);
			reg_addr = vd_layer[i].vd_mif_linear_reg.vd_if0_stride_0_f1;
			reg_val = READ_VCBUS_REG(reg_addr);
			pr_info("[0x%x] = 0x%X\n",
				   reg_addr, reg_val);
			reg_addr = vd_layer[i].vd_mif_linear_reg.vd_if0_stride_1_f1;
			reg_val = READ_VCBUS_REG(reg_addr);
			pr_info("[0x%x] = 0x%X\n",
				   reg_addr, reg_val);
		}
	}
}

static void dump_afbc_reg(void)
{
	int i;
	u32 reg_addr, reg_val = 0;

	for (i = 0; i < cur_dev->max_vd_layers; i++) {
		pr_info("vd%d afbc regs:\n", i);
		reg_addr = vd_layer[i].vd_afbc_reg.afbc_enable;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].vd_afbc_reg.afbc_mode;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].vd_afbc_reg.afbc_size_in;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].vd_afbc_reg.afbc_dec_def_color;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].vd_afbc_reg.afbc_conv_ctrl;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].vd_afbc_reg.afbc_lbuf_depth;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].vd_afbc_reg.afbc_head_baddr;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].vd_afbc_reg.afbc_body_baddr;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].vd_afbc_reg.afbc_size_out;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].vd_afbc_reg.afbc_out_yscope;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].vd_afbc_reg.afbc_vd_cfmt_ctrl;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].vd_afbc_reg.afbc_vd_cfmt_w;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].vd_afbc_reg.afbc_mif_hor_scope;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].vd_afbc_reg.afbc_mif_ver_scope;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].vd_afbc_reg.afbc_pixel_hor_scope;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].vd_afbc_reg.afbc_pixel_ver_scope;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].vd_afbc_reg.afbc_vd_cfmt_h;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].vd_afbc_reg.afbc_top_ctrl;
		if (cur_dev->display_module == T7_DISPLAY_MODULE && reg_addr) {
			reg_val = READ_VCBUS_REG(reg_addr);
			pr_info("[0x%x] = 0x%X\n",
				   reg_addr, reg_val);
		}
	}
}

static void dump_pps_reg(void)
{
	int i;
	u32 reg_addr, reg_val = 0;

	for (i = 0; i < cur_dev->max_vd_layers; i++) {
		pr_info("vd%d pps regs:\n", i);
		reg_addr = vd_layer[i].pps_reg.vd_vsc_region12_startp;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].pps_reg.vd_vsc_region34_startp;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].pps_reg.vd_vsc_region4_endp;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].pps_reg.vd_vsc_start_phase_step;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].pps_reg.vd_vsc_region1_phase_slope;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].pps_reg.vd_vsc_region3_phase_slope;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].pps_reg.vd_vsc_phase_ctrl;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].pps_reg.vd_vsc_init_phase;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].pps_reg.vd_hsc_region12_startp;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].pps_reg.vd_hsc_region34_startp;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].pps_reg.vd_hsc_region4_endp;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].pps_reg.vd_hsc_start_phase_step;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].pps_reg.vd_hsc_region1_phase_slope;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].pps_reg.vd_hsc_region3_phase_slope;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].pps_reg.vd_hsc_phase_ctrl;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].pps_reg.vd_sc_misc;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].pps_reg.vd_hsc_phase_ctrl1;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].pps_reg.vd_prehsc_coef;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].pps_reg.vd_pre_scale_ctrl;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].pps_reg.vd_prevsc_coef;
		if (reg_addr) {
			reg_val = READ_VCBUS_REG(reg_addr);
			pr_info("[0x%x] = 0x%X\n",
				   reg_addr, reg_val);
		}
		reg_addr = vd_layer[i].pps_reg.vd_prehsc_coef1;
		if (reg_addr) {
			reg_val = READ_VCBUS_REG(reg_addr);
			pr_info("[0x%x] = 0x%X\n",
				   reg_addr, reg_val);
		}
	}
}

static void dump_vpp_blend_reg(void)
{
	int i;
	u32 reg_addr, reg_val = 0;

	for (i = 0; i < cur_dev->max_vd_layers; i++) {
		pr_info("vd%d pps blend regs:\n", i);
		reg_addr = vd_layer[i].vpp_blend_reg.preblend_h_start_end;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("preblend_h_start_end[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].vpp_blend_reg.preblend_v_start_end;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("preblend_v_start_end[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].vpp_blend_reg.preblend_h_size;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("preblend_h_size[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].vpp_blend_reg.postblend_h_start_end;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("postblend_h_start_end[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
		reg_addr = vd_layer[i].vpp_blend_reg.postblend_v_start_end;
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("postblend_v_start_end[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
	}
}

static void dump_fgrain_reg(void)
{
	int i;
	u32 reg_addr, reg_val = 0;

	for (i = 0; i < cur_dev->max_vd_layers; i++) {
		if (glayer_info[i].fgrain_support) {
			pr_info("vd%d fgrain regs:\n", i);
			reg_addr = vd_layer[i].fg_reg.fgrain_ctrl;
			reg_val = READ_VCBUS_REG(reg_addr);
			pr_info("[0x%x] = 0x%X\n",
				   reg_addr, reg_val);
			reg_addr = vd_layer[i].fg_reg.fgrain_win_h;
			reg_val = READ_VCBUS_REG(reg_addr);
			pr_info("[0x%x] = 0x%X\n",
				   reg_addr, reg_val);
			reg_addr = vd_layer[i].fg_reg.fgrain_win_v;
			reg_val = READ_VCBUS_REG(reg_addr);
			pr_info("[0x%x] = 0x%X\n",
				   reg_addr, reg_val);
		}
	}
}

static void dump_aisr_reg(void)
{
	u32 reg_addr, reg_val = 0;

	pr_info("aisr reshape regs:\n");
	reg_addr = aisr_reshape_reg.aisr_reshape_ctrl0;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = aisr_reshape_reg.aisr_reshape_ctrl1;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = aisr_reshape_reg.aisr_reshape_scope_x;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = aisr_reshape_reg.aisr_reshape_scope_y;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = aisr_reshape_reg.aisr_reshape_baddr00;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = aisr_reshape_reg.aisr_reshape_baddr01;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = aisr_reshape_reg.aisr_reshape_baddr02;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = aisr_reshape_reg.aisr_reshape_baddr03;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = aisr_reshape_reg.aisr_reshape_baddr10;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = aisr_reshape_reg.aisr_reshape_baddr11;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = aisr_reshape_reg.aisr_reshape_baddr12;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = aisr_reshape_reg.aisr_reshape_baddr13;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = aisr_reshape_reg.aisr_reshape_baddr20;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = aisr_reshape_reg.aisr_reshape_baddr21;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = aisr_reshape_reg.aisr_reshape_baddr22;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = aisr_reshape_reg.aisr_reshape_baddr23;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = aisr_reshape_reg.aisr_reshape_baddr30;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = aisr_reshape_reg.aisr_reshape_baddr31;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = aisr_reshape_reg.aisr_reshape_baddr32;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = aisr_reshape_reg.aisr_reshape_baddr33;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = aisr_reshape_reg.aisr_post_ctrl;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = aisr_reshape_reg.aisr_post_size;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = aisr_reshape_reg.aisr_sr1_nn_post_top;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	pr_info("aisr pps regs:\n");
	reg_addr = cur_dev->aisr_pps_reg.vd_vsc_region12_startp;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = cur_dev->aisr_pps_reg.vd_vsc_region34_startp;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = cur_dev->aisr_pps_reg.vd_vsc_region4_endp;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = cur_dev->aisr_pps_reg.vd_vsc_start_phase_step;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = cur_dev->aisr_pps_reg.vd_vsc_region1_phase_slope;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = cur_dev->aisr_pps_reg.vd_vsc_region3_phase_slope;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = cur_dev->aisr_pps_reg.vd_vsc_phase_ctrl;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = cur_dev->aisr_pps_reg.vd_vsc_init_phase;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = cur_dev->aisr_pps_reg.vd_hsc_region12_startp;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = cur_dev->aisr_pps_reg.vd_hsc_region34_startp;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = cur_dev->aisr_pps_reg.vd_hsc_region4_endp;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = cur_dev->aisr_pps_reg.vd_hsc_start_phase_step;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = cur_dev->aisr_pps_reg.vd_hsc_region1_phase_slope;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = cur_dev->aisr_pps_reg.vd_hsc_region3_phase_slope;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = cur_dev->aisr_pps_reg.vd_hsc_phase_ctrl;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = cur_dev->aisr_pps_reg.vd_sc_misc;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = cur_dev->aisr_pps_reg.vd_hsc_phase_ctrl1;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = cur_dev->aisr_pps_reg.vd_prehsc_coef;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = cur_dev->aisr_pps_reg.vd_pre_scale_ctrl;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = cur_dev->aisr_pps_reg.vd_prevsc_coef;
	if (reg_addr) {
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
	}
	reg_addr = cur_dev->aisr_pps_reg.vd_prehsc_coef1;
	if (reg_addr) {
		reg_val = READ_VCBUS_REG(reg_addr);
		pr_info("[0x%x] = 0x%X\n",
			   reg_addr, reg_val);
	}
}

static void dump_vpp_path_size_reg(void)
{
	u32 reg_addr, reg_val = 0;

	if (cur_dev->display_module == OLD_DISPLAY_MODULE)
		return;
	pr_info("vpp path size reg:\n");
	reg_addr = vpp_path_size_reg.vd1_hdr_in_size;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("vd1_hdr_in_size[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = vpp_path_size_reg.vd2_hdr_in_size;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("vd2_hdr_in_size[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = vpp_path_size_reg.vd3_hdr_in_size;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("vd3_hdr_in_size[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = vpp_path_size_reg.vpp_line_in_length;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("vpp_line_in_length[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = vpp_path_size_reg.vpp_pic_in_height;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("vpp_pic_in_height[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = vpp_path_size_reg.vd1_sc_h_startp;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("vd1_sc_h_startp[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = vpp_path_size_reg.vd1_sc_h_endp;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("vd1_sc_h_endp[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = vpp_path_size_reg.vd1_sc_v_startp;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("vd1_sc_v_startp[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = vpp_path_size_reg.vd1_sc_v_endp;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("vd1_sc_v_endp[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = vpp_path_size_reg.vd2_sc_h_startp;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("vd2_sc_h_startp[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = vpp_path_size_reg.vd2_sc_h_endp;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("vd2_sc_h_endp[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = vpp_path_size_reg.vd2_sc_v_startp;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("vd2_sc_v_startp[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = vpp_path_size_reg.vd2_sc_v_endp;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("vd2_sc_v_endp[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = vpp_path_size_reg.vd3_sc_h_startp;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("vd3_sc_h_startp[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = vpp_path_size_reg.vd3_sc_h_endp;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("vd3_sc_h_endp[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = vpp_path_size_reg.vd3_sc_v_startp;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("vd3_sc_v_startp[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = vpp_path_size_reg.vd3_sc_v_endp;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("vd3_sc_v_endp[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = vpp_path_size_reg.schn_sc_h_startp;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("schn_sc_h_startp[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = vpp_path_size_reg.schn_sc_h_endp;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("schn_sc_h_endp[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = vpp_path_size_reg.schn_sc_v_startp;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("schn_sc_v_startp[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = vpp_path_size_reg.schn_sc_v_endp;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("schn_sc_v_endp[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = vpp_path_size_reg.sr0_in_size;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("sr0_in_size[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = vpp_path_size_reg.sr1_in_size;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("sr1_in_size[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = vpp_path_size_reg.aisr_post_size;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("aisr_post_size[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = vpp_path_size_reg.vpp_preblend_h_size;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("vpp_preblend_h_size[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = vpp_path_size_reg.preblend_vd1_h_start_end;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("preblend_vd1_h_start_end[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = vpp_path_size_reg.preblend_vd1_v_start_end;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("preblend_vd1_v_start_end[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = vpp_path_size_reg.vpp_ve_h_v_size;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("vpp_ve_h_v_size[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = vpp_path_size_reg.vpp_postblend_h_size;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("vpp_postblend_h_size[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = vpp_path_size_reg.vpp_out_h_v_size;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("vpp_out_h_v_size[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = vpp_path_size_reg.postblend_vd1_h_start_end;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("postblend_vd1_h_start_end[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = vpp_path_size_reg.postlend_vd1_v_start_end;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("postlend_vd1_v_start_end[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = vpp_path_size_reg.blend_vd2_h_start_end;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("blend_vd2_h_start_end[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = vpp_path_size_reg.blend_vd2_v_start_end;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("blend_vd2_v_start_end[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = vpp_path_size_reg.blend_vd3_h_start_end;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("blend_vd3_h_start_end[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = vpp_path_size_reg.blend_vd3_v_start_end;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("blend_vd3_v_start_end[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
}

static void dump_vpp_misc_reg(void)
{
	u32 reg_addr, reg_val = 0;

	if (cur_dev->display_module == OLD_DISPLAY_MODULE)
		return;
	pr_info("vpp misc reg:\n");
	reg_addr = viu_misc_reg.mali_afbcd_top_ctrl;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("mali_afbcd_top_ctrl[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = viu_misc_reg.mali_afbcd1_top_ctrl;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("mali_afbcd1_top_ctrl[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = viu_misc_reg.mali_afbcd2_top_ctrl;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("mali_afbcd2_top_ctrl[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = viu_misc_reg.vpp_vd1_top_ctrl;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("vpp_vd1_top_ctrl[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = viu_misc_reg.vpp_vd2_top_ctrl;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("vpp_vd2_top_ctrl[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = viu_misc_reg.vpp_vd3_top_ctrl;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("vpp_vd3_top_ctrl[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = viu_misc_reg.vd_path_misc_ctrl;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("vd_path_misc_ctrl[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = viu_misc_reg.path_start_sel;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("path_start_sel[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = viu_misc_reg.vpp_misc;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("vpp_misc[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = viu_misc_reg.vpp_misc1;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("vpp_misc1[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
}

static void dump_zorder_reg(void)
{
	u32 reg_addr, reg_val = 0;

	if (cur_dev->display_module == OLD_DISPLAY_MODULE)
		return;
	pr_info("vpp zorder reg:\n");
	reg_addr = VD1_BLEND_SRC_CTRL;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("VD1_BLEND_SRC_CTRL[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = VD2_BLEND_SRC_CTRL;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("VD2_BLEND_SRC_CTRL[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = VD3_BLEND_SRC_CTRL;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("VD3_BLEND_SRC_CTRL[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = OSD1_BLEND_SRC_CTRL;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("OSD1_BLEND_SRC_CTRL[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
	reg_addr = OSD2_BLEND_SRC_CTRL;
	reg_val = READ_VCBUS_REG(reg_addr);
	pr_info("OSD2_BLEND_SRC_CTRL[0x%x] = 0x%X\n",
		   reg_addr, reg_val);
}

ssize_t reg_dump_store(struct class *cla,
				 struct class_attribute *attr,
				const char *buf, size_t count)
{
	int res = 0;
	int ret = 0;

	ret = kstrtoint(buf, 0, &res);
	if (ret) {
		pr_err("kstrtoint err\n");
		return -EINVAL;
	}

	if (res) {
		if (cur_dev->display_module == S5_DISPLAY_MODULE) {
			dump_s5_vd_proc_regs();
			dump_vpp_post_reg();
		} else if (cur_dev->display_module == C3_DISPLAY_MODULE) {
			dump_mif_reg();
			dump_vout_blend_reg();
		} else {
			dump_mif_reg();
			dump_afbc_reg();
			dump_pps_reg();
			dump_vpp_blend_reg();
			dump_vpp_path_size_reg();
			dump_vpp_misc_reg();
			dump_zorder_reg();
			dump_fgrain_reg();
			if (cur_dev->aisr_support)
				dump_aisr_reg();
		}
	}
	return count;
}
