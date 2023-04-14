// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

/******************** READ ME ************************
 *
 * at afbce mode, 1 block = 32 * 4 pixel
 * there is a header in one block.
 * for example at 1080p,
 * header numbers = block numbers = 1920 * 1080 / (32 * 4)
 *
 * table map(only at non-mmu mode):
 * afbce data was saved at "body" region,
 * body region has been divided for every 4K(4096 bytes) and 4K unit,
 * table map contents is : (body addr >> 12)
 *
 * at non-mmu mode(just vdin non-mmu mode):
 * ------------------------------
 *          header
 *     (can analysis body addr)
 * ------------------------------
 *          table map
 *     (save body addr)
 * ------------------------------
 *          body
 *     (save afbce data)
 * ------------------------------
 */
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/cma.h>
#include <linux/amlogic/media/codec_mm/codec_mm.h>
#include <linux/dma-map-ops.h>
#include <linux/amlogic/media/vfm/vframe.h>
#include <linux/slab.h>
#include <linux/amlogic/media/frame_provider/tvin/tvin.h>
#include "../tvin_global.h"
#include "../tvin_format_table.h"
#include "vdin_ctl.h"
#include "vdin_regs.h"
#include "vdin_drv.h"
#include "vdin_vf.h"
#include "vdin_canvas.h"
#include "vdin_afbce.h"

void vdin_afbce_update_t3x(struct vdin_dev_s *devp)
{
	int hold_line_num = VDIN_AFBCE_HOLD_LINE_NUM;
	int reg_format_mode;/* 0:444 1:422 2:420 */
	int reg_fmt444_comb;
	int bits_num;
	int uncompress_bits;
	int uncompress_size;

	if (!devp->afbce_info)
		return;

#ifndef CONFIG_AMLOGIC_MEDIA_RDMA
	pr_info("##############################################\n");
	pr_info("vdin afbce must use RDMA,but it not be opened\n");
	pr_info("##############################################\n");
#endif
	reg_fmt444_comb = vdin_chk_is_comb_mode(devp);

	switch (devp->format_convert) {
	case VDIN_FORMAT_CONVERT_YUV_NV12:
	case VDIN_FORMAT_CONVERT_YUV_NV21:
	case VDIN_FORMAT_CONVERT_RGB_NV12:
	case VDIN_FORMAT_CONVERT_RGB_NV21:
		reg_format_mode = 2;
		bits_num = 12;
		break;
	case VDIN_FORMAT_CONVERT_YUV_YUV422:
	case VDIN_FORMAT_CONVERT_RGB_YUV422:
	case VDIN_FORMAT_CONVERT_GBR_YUV422:
	case VDIN_FORMAT_CONVERT_BRG_YUV422:
		reg_format_mode = 1;
		bits_num = 16;
		break;
	default:
		reg_format_mode = 0;
		bits_num = 24;
		break;
	}
	uncompress_bits = devp->source_bitdepth;

	/* bit size of uncompressed mode */
	uncompress_size = (((((16 * uncompress_bits * bits_num) + 7) >> 3) + 31)
		      / 32) << 1;
	rdma_write_reg(devp->rdma_handle, devp->addr_offset + VDIN0_AFBCE_MODE,
		       (0 & 0x7) << 29 | (0 & 0x3) << 26 | (3 & 0x3) << 24 |
		       (hold_line_num & 0x7f) << 16 |
		       (2 & 0x3) << 14 | (reg_fmt444_comb & 0x1));

	rdma_write_reg_bits(devp->rdma_handle,
			    devp->addr_offset + VDIN0_AFBCE_MIF_SIZE,
			    (uncompress_size & 0x1fff), 16, 5);/* uncmp_size */

	rdma_write_reg(devp->rdma_handle, devp->addr_offset + VDIN0_AFBCE_FORMAT,
		       (reg_format_mode  & 0x3) << 8 |
		       (uncompress_bits & 0xf) << 4 |
		       (uncompress_bits & 0xf));
}

/* Refer to vdin afbce doc
 * format		YUV420		YUV422		YUV444
 * bit width		8 10 12		8 10 12		8  10 12
 * 32byte number	6 8  9		8 10 12		12 15 18
 */
//sum = reg_block_burst_num0 + reg_block_burst_num1 + reg_block_burst_num2 + reg_block_burst_num3;
//avg = sum/4;
//ratio = avg/uncmp_num;
void vdin_afbce_config_comp_ratio_t3x(struct vdin_dev_s *devp, unsigned int format_mode)
{
	unsigned int afbce_loss_burst_num, sum, byte_num;
	unsigned int reg_block_burst_num0, reg_block_burst_num1;
	unsigned int reg_block_burst_num2, reg_block_burst_num3;

	if (!devp->cr_lossy_param.cr_lossy_ratio)
		return;

	if (format_mode == 0) {//YUV444
		if (devp->source_bitdepth == 8)
			byte_num = 12;
		else if (devp->source_bitdepth == 12)
			byte_num = 18;
		else
			byte_num = 15;
	} else if (format_mode == 2) {//YUV420
		if (devp->source_bitdepth == 8)
			byte_num = 6;
		else if (devp->source_bitdepth == 12)
			byte_num = 9;
		else
			byte_num = 8;
	} else {//YUV422
		byte_num = devp->source_bitdepth;
	}

	sum = devp->cr_lossy_param.cr_lossy_ratio * byte_num * 4 / 100;
	reg_block_burst_num0 = (sum / 4);
	reg_block_burst_num1 = reg_block_burst_num0 + (sum % 4) / 2;
	reg_block_burst_num2 = reg_block_burst_num0;
	reg_block_burst_num3 = reg_block_burst_num0 + (sum % 4) / 2;

	afbce_loss_burst_num = (reg_block_burst_num0 << 24) | (reg_block_burst_num1 << 16) |
			       (reg_block_burst_num2 << 8)  | (reg_block_burst_num3 << 0);

	W_VCBUS(devp->addr_offset + VDIN0_AFBCE_LOSS_BURST_NUM, afbce_loss_burst_num);
}

void vdin_afbce_config_t3x(struct vdin_dev_s *devp)
{
	int hold_line_num = VDIN_AFBCE_HOLD_LINE_NUM;
	int lbuf_depth = 256;
	int lossy_luma_en = 0;
	int lossy_chrm_en = 0;
	int cur_mmu_used = 0;
	int reg_format_mode;//0:444 1:422 2:420
	int reg_fmt444_comb;
	int sub_block_num = 16;
	int uncompress_bits;
	int uncompress_size;
	int def_color_0 = 0x3ff;
	int def_color_1 = 0x80;
	int def_color_2 = 0x80;
	int def_color_3 = 0;
	int h_blk_size_out = (devp->h_active + 31) >> 5;
	int v_blk_size_out = (devp->v_active + 3)  >> 2;
	int blk_out_end_h;//output blk scope
	int blk_out_bgn_h;//output blk scope
	int blk_out_end_v;//output blk scope
	int blk_out_bgn_v;//output blk scope
	int enc_win_bgn_h;//input scope
	int enc_win_end_h;//input scope
	int enc_win_bgn_v;//input scope
	int enc_win_end_v;//input scope
	int reg_fmt444_rgb_en = 0;
	enum vdin_format_convert_e vdin_out_fmt;
	//unsigned int bit_mode_shift = 0;
	unsigned int offset;

	if (!devp->afbce_info)
		return;

#ifndef CONFIG_AMLOGIC_MEDIA_RDMA
	pr_info("##############################################\n");
	pr_info("vdin afbce must use RDMA,but it not be opened\n");
	pr_info("##############################################\n");
#endif
	offset = devp->addr_offset;
	enc_win_bgn_h = 0;
	enc_win_end_h = devp->h_active - 1;
	enc_win_bgn_v = 0;
	enc_win_end_v = devp->v_active - 1;

	blk_out_end_h	=  enc_win_bgn_h      >> 5 ;//output blk scope
	blk_out_bgn_h	= (enc_win_end_h + 31)	>> 5 ;//output blk scope
	blk_out_end_v	=  enc_win_bgn_v      >> 2 ;//output blk scope
	blk_out_bgn_v	= (enc_win_end_v + 3) >> 2 ;//output blk scope

	vdin_out_fmt = devp->format_convert;
	if (vdin_out_fmt == VDIN_FORMAT_CONVERT_YUV_RGB ||
	    vdin_out_fmt == VDIN_FORMAT_CONVERT_YUV_GBR ||
	    vdin_out_fmt == VDIN_FORMAT_CONVERT_YUV_BRG ||
	    vdin_out_fmt == VDIN_FORMAT_CONVERT_RGB_RGB)
		reg_fmt444_rgb_en = 1;

	reg_fmt444_comb = vdin_chk_is_comb_mode(devp);
	if (vdin_out_fmt == VDIN_FORMAT_CONVERT_YUV_NV12 ||
	    vdin_out_fmt == VDIN_FORMAT_CONVERT_YUV_NV21 ||
	    vdin_out_fmt == VDIN_FORMAT_CONVERT_RGB_NV12 ||
	    vdin_out_fmt == VDIN_FORMAT_CONVERT_RGB_NV21) {
		reg_format_mode = 2;/*420*/
	    sub_block_num = 12;
	} else if ((vdin_out_fmt == VDIN_FORMAT_CONVERT_YUV_YUV422) ||
		(vdin_out_fmt == VDIN_FORMAT_CONVERT_RGB_YUV422) ||
		(vdin_out_fmt == VDIN_FORMAT_CONVERT_GBR_YUV422) ||
		(vdin_out_fmt == VDIN_FORMAT_CONVERT_BRG_YUV422)) {
		reg_format_mode = 1;/*422*/
	    sub_block_num = 16;
	} else {
		reg_format_mode = 0;/*444*/
	    sub_block_num = 24;
	}

	uncompress_bits = devp->source_bitdepth;

	//bit size of uncompressed mode
	uncompress_size = (((((16 * uncompress_bits * sub_block_num) + 7) >> 3) + 31)
		      / 32) << 1;
	pr_info("%s fmt_convert:%d comb:%d;%d,%d;%d,%d\n", __func__,
		devp->format_convert, reg_fmt444_comb, devp->h_active, h_blk_size_out,
		uncompress_bits, sub_block_num);
	W_VCBUS_BIT(offset + VDIN0_AFBCE_MODE_EN, 1, 18, 1);/* disable order mode */

	//W_VCBUS_BIT(VDIN_WRARB_REQEN_SLV, 0x1, 3, 1);//vpu arb axi_enable
	//W_VCBUS_BIT(VDIN_WRARB_REQEN_SLV, 0x1, 7, 1);//vpu arb axi_enable

	W_VCBUS(offset + VDIN0_AFBCE_MODE,
		(0 & 0x7) << 29 | (0 & 0x3) << 26 | (3 & 0x3) << 24 |
		(hold_line_num & 0x7f) << 16 |
		(2 & 0x3) << 14 | (reg_fmt444_comb & 0x1));

	W_VCBUS_BIT(offset + VDIN0_AFBCE_QUANT_ENABLE, (lossy_luma_en & 0x1), 0, 1);//lossy
	W_VCBUS_BIT(offset + VDIN0_AFBCE_QUANT_ENABLE, (lossy_chrm_en & 0x1), 4, 1);//lossy

	if (devp->afbce_flag & VDIN_AFBCE_EN_LOSSY) {
		W_VCBUS(offset + VDIN0_AFBCE_QUANT_ENABLE, 0xc11);
		if (devp->cr_lossy_param.lossy_mode) {/* cr_lossy*/
			W_VCBUS_BIT(offset + VDIN0_AFBCE_LOSS_CTRL, 1, 31, 1);//reg_fix_cr_en
			//reg_quant_diff_root_leave
			W_VCBUS_BIT(offset + VDIN0_AFBCE_LOSS_CTRL,
				devp->cr_lossy_param.quant_diff_root_leave, 12, 4);
			//compression ratio setting
			vdin_afbce_config_comp_ratio_t3x(devp, reg_format_mode);
		} else {
			W_VCBUS_BIT(offset + VDIN0_AFBCE_LOSS_CTRL, 0, 31, 1);//reg_fix_cr_en
		}
		pr_info("vdin%d,afbce cr_lossy:%d,leave:%d\n",
			devp->index, devp->cr_lossy_param.lossy_mode,
			devp->cr_lossy_param.quant_diff_root_leave);
	}

	W_VCBUS(offset + VDIN0_AFBCE_SIZE_IN,
		((devp->h_active & 0x1fff) << 16) |  // hsize_in of afbc input
		((devp->v_active & 0x1fff) << 0)     // vsize_in of afbc input
		);

	W_VCBUS(offset + VDIN0_AFBCE_BLK_SIZE_IN,
		((h_blk_size_out & 0x1fff) << 16) |     // out blk hsize
		((v_blk_size_out & 0x1fff) << 0)	// out blk vsize
		);

	//head addr of compressed data
	if (devp->dtdata->hw_ver >= VDIN_HW_T7)
		W_VCBUS(offset + VDIN0_AFBCE_HEAD_BADDR,
			devp->afbce_info->fm_head_paddr[0] >> 4);
	else
		W_VCBUS(offset + VDIN0_AFBCE_HEAD_BADDR,
			devp->afbce_info->fm_head_paddr[0]);
	//uncompress_size
	W_VCBUS_BIT(offset + VDIN0_AFBCE_MIF_SIZE, (uncompress_size & 0x1fff), 16, 5);

	/* how to set reg when we use crop ? */
	// scope of hsize_in ,should be a integer multiple of 32
	// scope of vsize_in ,should be a integer multiple of 4
	W_VCBUS(offset + VDIN0_AFBCE_PIXEL_IN_HOR_SCOPE,
		((enc_win_end_h & 0x1fff) << 16) |
		((enc_win_bgn_h & 0x1fff) << 0));

	// scope of hsize_in ,should be a integer multiple of 32
	// scope of vsize_in ,should be a integer multiple of 4
	W_VCBUS(offset + VDIN0_AFBCE_PIXEL_IN_VER_SCOPE,
		((enc_win_end_v & 0x1fff) << 16) |
		((enc_win_bgn_v & 0x1fff) << 0));

	W_VCBUS_BIT(offset + VDIN0_AFBCE_CONV_CTRL, lbuf_depth, 0, 12);//fix 256

	W_VCBUS(offset + VDIN0_AFBCE_MIF_HOR_SCOPE,
		((blk_out_bgn_h & 0x3ff) << 16) |  // scope of out blk h_size
		((blk_out_end_h & 0xfff) << 0)	  // scope of out blk v_size
		);

	W_VCBUS(offset + VDIN0_AFBCE_MIF_VER_SCOPE,
		((blk_out_bgn_v & 0x3ff) << 16) |  // scope of out blk h_size
		((blk_out_end_v & 0xfff) << 0)	  // scope of out blk v_size
		);

	W_VCBUS(offset + VDIN0_AFBCE_FORMAT,
		(2 << 12) | /* reg_burst_length_add_value */
		(reg_format_mode & 0x3) << 8 |
		(uncompress_bits & 0xf) << 4 |
		(uncompress_bits & 0xf));

	W_VCBUS(offset + VDIN0_AFBCE_DEFCOLOR_1,
		((def_color_3 & 0xfff) << 12) |  // def_color_a
		((def_color_0 & 0xfff) << 0)	// def_color_y
		);

//	if (devp->source_bitdepth >= VDIN_COLOR_DEEPS_8BIT &&
//	    devp->source_bitdepth <= VDIN_COLOR_DEEPS_12BIT)
//		bit_mode_shift = devp->source_bitdepth - VDIN_COLOR_DEEPS_8BIT;
	/*def_color_v*/
	/*def_color_u*/
//	W_VCBUS(offset + VDIN0_AFBCE_DEFCOLOR_2,
//		(((def_color_2 << bit_mode_shift) & 0xfff) << 12) |
//		(((def_color_1 << bit_mode_shift) & 0xfff) << 0));
	W_VCBUS(offset + VDIN0_AFBCE_DEFCOLOR_2,
		((def_color_2 & 0xfff) << 12) |
		((def_color_1 & 0xfff) << 0));

	W_VCBUS_BIT(offset + VDIN0_AFBCE_MMU_RMIF_CTRL4,
		    devp->afbce_info->fm_table_paddr[0] >> 4, 0, 32);
	W_VCBUS_BIT(offset + VDIN0_AFBCE_MMU_RMIF_CTRL1, 0x1, 6, 1);

	W_VCBUS_BIT(offset + VDIN0_AFBCE_MMU_RMIF_SCOPE_X, cur_mmu_used, 0, 12);
	/*for almost uncompressed pattern,garbage at bottom
	 *(h_active * v_active * bytes per pixel + 3M) / page_size - 1
	 *where 3M is the rest bytes of block,since every block must not be\
	 *separated by 2 pages
	 */
	//W_VCBUS_BIT(offset + VDIN0_AFBCE_MMU_RMIF_SCOPE_X, 0x1c4f, 16, 13);
	W_VCBUS_BIT(offset + VDIN0_AFBCE_MMU_RMIF_SCOPE_X, 0x1ffe, 16, 13);
	W_VCBUS_BIT(offset + VDIN0_AFBCE_MMU_RMIF_CTRL3, 0x1fff, 0, 13);
	W_VCBUS_BIT(offset + VDIN0_AFBCE_ENABLE, 1, AFBCE_WORK_MD_BIT, AFBCE_WORK_MD_WID);
	W_VCBUS_BIT(offset + VDIN0_AFBCE_ROT_CTRL, 0, 1, 0);/* reg_rot_en */

	if (devp->double_wr) {
		//W_VCBUS_BIT(offset + VDIN0_AFBCE_ENABLE, 1, AFBCE_EN_BIT, AFBCE_EN_WID);
		//W_VCBUS_BIT(offset + VDIN0_AFBCE_ENABLE, 1,
		//	AFBCE_START_PULSE_BIT, AFBCE_START_PULSE_WID);
		W_VCBUS_BIT(offset + VDIN0_CORE_CTRL, 0, 8, 1);/* reg_afbce_path_en */
		W_VCBUS_BIT(offset + VDIN0_CORE_CTRL, 1, 7, 1);/* reg_dith_path_en */
	} else {
		W_VCBUS_BIT(offset + VDIN0_CORE_CTRL, 0, 8, 1);/* reg_afbce_path_en */
		W_VCBUS_BIT(offset + VDIN0_CORE_CTRL, 0, 7, 1);/* reg_dith_path_en */
		//W_VCBUS_BIT(offset + VDIN0_AFBCE_ENABLE, 0, AFBCE_EN_BIT, AFBCE_EN_WID);
	}
}

//void vdin_afbce_maptable_init(struct vdin_dev_s *devp)
//{
//	unsigned int i, j;
//	unsigned int highmem_flag = 0;
//	unsigned long phys_addr = 0;
//	unsigned int *virt_addr = NULL;
//	unsigned int body;
//	unsigned int size;
//	void *p = NULL;
//
//	if (!devp->afbce_info)
//		return;
//
//	size = roundup(devp->afbce_info->frame_body_size, 4096);
//
//	phys_addr = devp->afbce_info->fm_table_paddr[0];
//	if (devp->cma_config_flag == 0x101)
//		highmem_flag = PageHighMem(phys_to_page(phys_addr));
//	else
//		highmem_flag = PageHighMem(phys_to_page(phys_addr));
//
//	for (i = 0; i < devp->vf_mem_max_cnt; i++) {
//		phys_addr = devp->afbce_info->fm_table_paddr[i];
//		if (highmem_flag == 0) {
//			if (devp->cma_config_flag == 0x101)
//				virt_addr = codec_mm_phys_to_virt(phys_addr);
//			else if (devp->cma_config_flag == 0)
//				virt_addr = phys_to_virt(phys_addr);
//			else
//				virt_addr = phys_to_virt(phys_addr);
//		} else {
//			virt_addr = (unsigned int *)vdin_vmap(phys_addr,
//				devp->afbce_info->frame_table_size);
//			if (vdin_dbg_en) {
//				pr_err("----vdin vmap v: %p, p: %lx, size: %d\n",
//				       virt_addr, phys_addr,
//				       devp->afbce_info->frame_table_size);
//			}
//			if (!virt_addr) {
//				pr_err("vmap fail, size: %d.\n",
//				       devp->afbce_info->frame_table_size);
//				return;
//			}
//		}
//
//		p = virt_addr;
//		body = devp->afbce_info->fm_body_paddr[i] & 0xffffffff;
//		for (j = 0; j < size; j += 4096) {
//			*virt_addr = ((j + body) >> 12) & 0x000fffff;
//			virt_addr++;
//		}
//
//		/* clean tail data. */
//		memset(virt_addr, 0, devp->afbce_info->frame_table_size -
//		       ((char *)virt_addr - (char *)p));
//
//		vdin_dma_flush(devp, p,
//			       devp->afbce_info->frame_table_size,
//			       DMA_TO_DEVICE);
//
//		if (highmem_flag)
//			vdin_unmap_phyaddr(p);
//
//		virt_addr = NULL;
//	}
//}

void vdin_afbce_set_next_frame_t3x(struct vdin_dev_s *devp,
			       unsigned int rdma_enable, struct vf_entry *vfe)
{
	unsigned char i;

	if (!devp->afbce_info)
		return;

	i = vfe->af_num;
	vfe->vf.compHeadAddr = devp->afbce_info->fm_head_paddr[i];
	vfe->vf.compBodyAddr = devp->afbce_info->fm_body_paddr[i];
	vdin_set_lossy_param(devp, &vfe->vf);

#ifdef CONFIG_AMLOGIC_MEDIA_RDMA
	if (rdma_enable) {
		rdma_write_reg(devp->rdma_handle,
				VDIN0_AFBCE_HEAD_BADDR + devp->addr_offset,
			       devp->afbce_info->fm_head_paddr[i] >> 4);
		rdma_write_reg_bits(devp->rdma_handle,
				    VDIN0_AFBCE_MMU_RMIF_CTRL4 + devp->addr_offset,
				    devp->afbce_info->fm_table_paddr[i] >> 4,
				    0, 32);
		rdma_write_reg_bits(devp->rdma_handle,
				    VDIN0_AFBCE_ENABLE + devp->addr_offset, 1,
				    AFBCE_START_PULSE_BIT, AFBCE_START_PULSE_WID);
		if (devp->pause_dec) {
			rdma_write_reg_bits(devp->rdma_handle,
					    VDIN0_CORE_CTRL + devp->addr_offset, 0,
					    AFBCE_EN_BIT, AFBCE_EN_WID);
			rdma_write_reg_bits(devp->rdma_handle,
					    VDIN0_AFBCE_ENABLE + devp->addr_offset, 0,
					    AFBCE_EN_BIT, AFBCE_EN_WID);
		} else {
			rdma_write_reg_bits(devp->rdma_handle,
					    VDIN0_CORE_CTRL + devp->addr_offset, 1,
					    AFBCE_EN_BIT, AFBCE_EN_WID);
			rdma_write_reg_bits(devp->rdma_handle,
					    VDIN0_AFBCE_ENABLE + devp->addr_offset, 1,
					    AFBCE_EN_BIT, AFBCE_EN_WID);
		}
	} else {
		W_VCBUS(VDIN0_AFBCE_HEAD_BADDR + devp->addr_offset,
			devp->afbce_info->fm_head_paddr[i] >> 4);
		W_VCBUS(VDIN0_AFBCE_MMU_RMIF_CTRL4 + devp->addr_offset,
			devp->afbce_info->fm_table_paddr[i] >> 4);
		W_VCBUS_BIT(VDIN0_AFBCE_ENABLE + devp->addr_offset, 1,
			AFBCE_START_PULSE_BIT, AFBCE_START_PULSE_WID);
		W_VCBUS_BIT(VDIN0_AFBCE_ENABLE + devp->addr_offset, 0,
			AFBCE_EN_BIT, AFBCE_EN_WID);
		W_VCBUS_BIT(VDIN0_CORE_CTRL + devp->addr_offset, 0,
			AFBCE_EN_BIT, AFBCE_EN_WID);
	}
#endif
	vdin_afbce_clear_write_down_flag(devp);
}

void vdin_pause_afbce_write_t3x(struct vdin_dev_s *devp, unsigned int rdma_enable)
{
#ifdef CONFIG_AMLOGIC_MEDIA_RDMA
	if (rdma_enable) {
		/* reg_afbce_path_en */
		rdma_write_reg_bits(devp->rdma_handle, devp->addr_offset + VDIN0_CORE_CTRL, 0,
				    AFBCE_EN_BIT, AFBCE_EN_WID);
		rdma_write_reg_bits(devp->rdma_handle, devp->addr_offset + VDIN0_AFBCE_ENABLE, 0,
				    AFBCE_EN_BIT, AFBCE_EN_WID);
	}
#endif
	vdin_afbce_clear_write_down_flag_t3x(devp);
}

/* frm_end will not pull up if using rdma IF to clear afbce flag */
void vdin_afbce_clear_write_down_flag_t3x(struct vdin_dev_s *devp)
{
	W_VCBUS(VDIN0_AFBCE_CLR_FLAG + devp->addr_offset, 0);
}

/* return 1: write down */
int vdin_afbce_read_write_down_flag_t3x(struct vdin_dev_s *devp)
{
	int frm_end = -1, wr_abort = -1;

	//frm_end = rd_bits(0, AFBCE_STA_FLAG, 0, 1);
	frm_end = rd_bits(0, devp->addr_offset + VDIN0_AFBCE_STAT1, 31, 1);
	wr_abort = rd_bits(0, devp->addr_offset + VDIN0_AFBCE_STA_FLAGT, 2, 2);

	if (vdin_isr_monitor & VDIN_ISR_MONITOR_WRITE_DONE)
		pr_info("frm_end:%#x,wr_abort:%#x\n",
			frm_end, wr_abort);

	if (frm_end == 1 && wr_abort == 0)
		return 1;
	else
		return 0;
}

void vdin_afbce_soft_reset_t3x(struct vdin_dev_s *devp)
{
	/* reg_afbce_path_en */
	W_VCBUS_BIT(devp->addr_offset + VDIN0_CORE_CTRL, 0, 8, 1);
	/* reg_enc_enable */
	W_VCBUS_BIT(devp->addr_offset + VDIN0_AFBCE_ENABLE, 0,
		AFBCE_EN_BIT, AFBCE_EN_WID);
	W_VCBUS_BIT(devp->addr_offset + VDIN0_AFBCE_MODE, 0, 30, 1);
	W_VCBUS_BIT(devp->addr_offset + VDIN0_AFBCE_MODE, 1, 30, 1);
	W_VCBUS_BIT(devp->addr_offset + VDIN0_AFBCE_MODE, 0, 30, 1);
}

void vdin_afbce_mode_update_t3x(struct vdin_dev_s *devp)
{
	/* vdin mif/afbce mode update */
//	if (devp->afbce_mode)
//		vdin_write_mif_or_afbce(devp, VDIN_OUTPUT_TO_AFBCE);
//	else
//		vdin_write_mif_or_afbce(devp, VDIN_OUTPUT_TO_MIF);

	if (vdin_dbg_en) {
		pr_info("vdin.%d: change afbce_mode %d->%d\n",
			devp->index, devp->afbce_mode_pre, devp->afbce_mode);
	}
	devp->afbce_mode_pre = devp->afbce_mode;
}

