// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/types.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include "am_dma_ctrl.h"
#include "reg_helper.h"

#define ADDR_PARAM(page, reg)  (((page) << 8) | (reg))
#define UNIT_SIZE 128

struct vpu_lut_dma_wr_s {
	enum lut_dma_wr_id_e dma_wr_id;
	u32 stride;
	u64 baddr0;
	u64 baddr1;
	u32 addr_mode;
	u32 rpt_num;
};

struct _dma_reg_cfg_s {
	unsigned char page;
	unsigned char reg_wrmif_ctrl;
	unsigned char reg_wrmif0_ctrl;
	unsigned char reg_wrmif0_badr0;
	unsigned char reg_wrmif_sel;
};

static struct _dma_reg_cfg_s dma_reg_cfg = {
	0x27,
	0xd5,
	0xd6,
	0xde,
	0xee,
};

static struct device vecm_dev;
static ulong alloc_size[EN_DMA_WR_ID_MAX];
static dma_addr_t dma_paddr[EN_DMA_WR_ID_MAX];
static void *dma_vaddr[EN_DMA_WR_ID_MAX];
static struct vpu_lut_dma_wr_s lut_dma_wr[EN_DMA_WR_ID_MAX];
static unsigned int dma_count[EN_DMA_WR_ID_MAX] = {
	12 * 8 * 4, 12 * 8 * 4,
	22, 22,
	12, 12,
	26, 26, 26,
	8 * 80,
};

static void _set_vpu_lut_dma_mif_wr_unit(int enable,
	struct vpu_lut_dma_wr_s *cfg_data)
{
	unsigned int addr;
	unsigned int val;
	int wr_sel = cfg_data->dma_wr_id < 8 ? 0 : 1;
	int offset = wr_sel ?
		(cfg_data->dma_wr_id - 8) << 2 : cfg_data->dma_wr_id << 2;

	/*
	 * Bit 31 ro_lut_wr_hs_r unsigned, RO, default = 0
	 * Bit 30:27 ro_lut_wr_id unsigned, RO, default = 0
	 * Bit 26 pls_hold_clr unsigned, pls, default = 0
	 * Bit 25 ro_hold_timeout unsigned, RO, default = 0
	 * Bit 24:12 ro_wrmif_cnt unsigned, RO, default = 0
	 * Bit 11:4 reg_hold_line unsigned, RW, default = 0xff
	 * Bit 3 ro_lut_wr_hs_s unsigned, RO, default = 0
	 * Bit 2 wrmif_hw_reset unsigned, RW, default = 0
	 * Bit 1 lut_wr_cnt_sel unsigned, RW, default = 0
	 * Bit 0 lut_wr_reg_sel unsigned, RW, default = 0,
	 *       0: sel lut0-7; 1: sel lut8-15
	 */
	addr = ADDR_PARAM(dma_reg_cfg.page,
		dma_reg_cfg.reg_wrmif_sel);
	val = READ_VPP_REG_S5(addr);
	val |= wr_sel;
	WRITE_VPP_REG_S5(addr, val);

	/*
	 * Bit 31 reserved
	 * Bit 30 reg_wr0_clr_fcnt unsigned, RW, default = 0
	 * Bit 29:28 reg_wr0_addr_mode unsigned, RW, default = 0
	 * Bit 27 reg_wr0_frm_set unsigned, RW, default = 0
	 * Bit 26 reg_wr0_frm_force unsigned, RW, default = 0
	 * Bit 25:18 reg_wr0_rpt_num unsigned, RW, default = 0
	 * Bit 17 reserved
	 * Bit 16 reg_wr0_enable unsigned, RW, default = 0 channel0
	 * Bit 15 reserved
	 * Bit 14 reg_wr0_swap_64bit unsigned, RW, default = 0
	 * Bit 13 reg_wr0_little_endian unsigned, RW, default = 0
	 * Bit 12:0 reg_wr0_stride unsigned, RW, default = 0x200
	 */
	addr = ADDR_PARAM(dma_reg_cfg.page,
		dma_reg_cfg.reg_wrmif0_ctrl) + offset;
	val = READ_VPP_REG_S5(addr);
	val |= (cfg_data->addr_mode & 0x3) << 28 |
		(cfg_data->rpt_num & 0xff) << 18 |
		(enable & 0x1) << 16 |
		(cfg_data->stride & 0x1fff);
	WRITE_VPP_REG_S5(addr, val);

	addr = ADDR_PARAM(dma_reg_cfg.page,
		dma_reg_cfg.reg_wrmif0_badr0) + (offset << 1);
	val = READ_VPP_REG_S5(addr);
	val |= cfg_data->baddr0;
	WRITE_VPP_REG_S5(addr, val);

	addr = ADDR_PARAM(dma_reg_cfg.page,
		dma_reg_cfg.reg_wrmif0_badr0) + 1 + (offset << 1);
	val = READ_VPP_REG_S5(addr);
	val |= cfg_data->baddr1;
	WRITE_VPP_REG_S5(addr, val);
}

void am_dma_buffer_malloc(struct platform_device *pdev,
	enum lut_dma_wr_id_e dma_wr_id)
{
	int i = 0;

	vecm_dev = pdev->dev;

	if (dma_wr_id == EN_DMA_WR_ID_MAX) {
		for (i = 0; i < EN_DMA_WR_ID_AMBIENT_LIGHT; i++) {
			alloc_size[i] = dma_count[i] * UNIT_SIZE;
			dma_vaddr[i] = dma_alloc_coherent(&vecm_dev,
				alloc_size[i], &dma_paddr[i], GFP_KERNEL);
			lut_dma_wr[i].dma_wr_id = i;
			if (dma_vaddr[i])
				lut_dma_wr[i].baddr0 = dma_paddr[i];
		}
	} else {
		alloc_size[dma_wr_id] = dma_count[dma_wr_id] * UNIT_SIZE;
		dma_vaddr[dma_wr_id] = dma_alloc_coherent(&vecm_dev,
			alloc_size[dma_wr_id], &dma_paddr[dma_wr_id], GFP_KERNEL);
		lut_dma_wr[dma_wr_id].dma_wr_id = dma_wr_id;
		if (dma_vaddr[dma_wr_id])
			lut_dma_wr[dma_wr_id].baddr0 = dma_paddr[dma_wr_id];
	}

	/*lut_dma_wr initial*/
	lut_dma_wr[EN_DMA_WR_ID_LC_STTS_0].stride = 12;
	lut_dma_wr[EN_DMA_WR_ID_LC_STTS_0].addr_mode = 1;
	lut_dma_wr[EN_DMA_WR_ID_LC_STTS_0].rpt_num = 32;

	lut_dma_wr[EN_DMA_WR_ID_LC_STTS_1].stride = 12;
	lut_dma_wr[EN_DMA_WR_ID_LC_STTS_1].addr_mode = 1;
	lut_dma_wr[EN_DMA_WR_ID_LC_STTS_1].rpt_num = 32;

	lut_dma_wr[EN_DMA_WR_ID_VI_HIST_SPL_0].stride = 22;
	lut_dma_wr[EN_DMA_WR_ID_VI_HIST_SPL_0].addr_mode = 0;
	lut_dma_wr[EN_DMA_WR_ID_VI_HIST_SPL_0].rpt_num = 0;

	lut_dma_wr[EN_DMA_WR_ID_VI_HIST_SPL_1].stride = 22;
	lut_dma_wr[EN_DMA_WR_ID_VI_HIST_SPL_1].addr_mode = 0;
	lut_dma_wr[EN_DMA_WR_ID_VI_HIST_SPL_1].rpt_num = 0;

	lut_dma_wr[EN_DMA_WR_ID_CM2_HIST_0].stride = 12;
	lut_dma_wr[EN_DMA_WR_ID_CM2_HIST_0].addr_mode = 0;
	lut_dma_wr[EN_DMA_WR_ID_CM2_HIST_0].rpt_num = 0;

	lut_dma_wr[EN_DMA_WR_ID_CM2_HIST_1].stride = 12;
	lut_dma_wr[EN_DMA_WR_ID_CM2_HIST_1].addr_mode = 0;
	lut_dma_wr[EN_DMA_WR_ID_CM2_HIST_1].rpt_num = 0;

	lut_dma_wr[EN_DMA_WR_ID_VD1_HDR_0].stride = 26;
	lut_dma_wr[EN_DMA_WR_ID_VD1_HDR_0].addr_mode = 0;
	lut_dma_wr[EN_DMA_WR_ID_VD1_HDR_0].rpt_num = 0;

	lut_dma_wr[EN_DMA_WR_ID_VD1_HDR_1].stride = 26;
	lut_dma_wr[EN_DMA_WR_ID_VD1_HDR_1].addr_mode = 0;
	lut_dma_wr[EN_DMA_WR_ID_VD1_HDR_1].rpt_num = 0;

	lut_dma_wr[EN_DMA_WR_ID_VD2_HDR].stride = 26;
	lut_dma_wr[EN_DMA_WR_ID_VD2_HDR].addr_mode = 0;
	lut_dma_wr[EN_DMA_WR_ID_VD2_HDR].rpt_num = 0;
}

void am_dma_buffer_free(struct platform_device *pdev,
	enum lut_dma_wr_id_e dma_wr_id)
{
	int i = 0;

	vecm_dev = pdev->dev;

	if (dma_wr_id == EN_DMA_WR_ID_MAX) {
		for (i = 0; i < EN_DMA_WR_ID_AMBIENT_LIGHT; i++)
			dma_free_coherent(&vecm_dev,
				alloc_size[i], dma_vaddr[i], dma_paddr[i]);
	} else {
		dma_free_coherent(&vecm_dev, alloc_size[dma_wr_id],
			dma_vaddr[dma_wr_id], dma_paddr[dma_wr_id]);
	}
}

void am_dma_set_mif_wr_status(int enable)
{
	unsigned int addr;

	addr = ADDR_PARAM(dma_reg_cfg.page,
		dma_reg_cfg.reg_wrmif_ctrl);
	WRITE_VPP_REG_BITS_S5(addr, enable, 13, 1);
}

void am_dma_set_mif_wr(enum lut_dma_wr_id_e dma_wr_id,
	int enable)
{
	int i = 0;

	if (dma_wr_id == EN_DMA_WR_ID_MAX) {
		for (i = 0; i < EN_DMA_WR_ID_AMBIENT_LIGHT; i++) {
			if (dma_vaddr[i])
				_set_vpu_lut_dma_mif_wr_unit(enable,
					&lut_dma_wr[i]);
		}
	} else {
		if (dma_vaddr[dma_wr_id])
			_set_vpu_lut_dma_mif_wr_unit(enable,
				&lut_dma_wr[dma_wr_id]);
	}
}

void am_dma_get_mif_data_lc_stts(int index,
	unsigned int *data, unsigned int length)
{
	int i = 0;
	int j = 0;
	int k = 0;
	unsigned int tmp = 0;
	unsigned int size = length;
	unsigned int *val;

	if (!data || length == 0)
		return;

	if (index > 1)
		index = 1;

	if (size > 12 * 8 * 17)
		size = 1632;

	if (!dma_vaddr[EN_DMA_WR_ID_LC_STTS_0 + index])
		return;

	val = (unsigned int *)dma_vaddr[EN_DMA_WR_ID_LC_STTS_0 + index];

	for (i = 0; i < 12 * 8; i++) {
		for (j = 0; j < 16; j++) {
			data[k] = val[i * 16 + j] & 0xffffff;
			k++;

			if (j == 3 || j == 7 || j == 11 || j == 15) {
				tmp |= ((val[i * 16 + j] >> 24) & 0x1f);
				if (j == 15) {
					data[k] = tmp;
					k++;
				}
			}

			if (k == size)
				break;
		}
	}
}

void am_dma_get_mif_data_vi_hist(int index,
	unsigned int *data, unsigned int length)
{
	int i = 0;
	unsigned int size = length;
	unsigned int *val;

	if (!data || length == 0)
		return;

	if (index > 1)
		index = 1;

	if (size > 33)
		size = 33;

	if (!dma_vaddr[EN_DMA_WR_ID_VI_HIST_SPL_0 + index])
		return;

	val = (unsigned int *)dma_vaddr[EN_DMA_WR_ID_VI_HIST_SPL_0 + index];

	for (i = 0; i < size; i++)
		data[i] = val[i];
}

void am_dma_get_mif_data_vi_hist_low(int index,
	unsigned int *data, unsigned int length)
{
	int i = 0;
	unsigned int tmp = 0;
	unsigned int size = length;
	unsigned char *val;

	if (!data || length == 0)
		return;

	if (index > 1)
		index = 1;

	if (size > 64)
		size = 64;

	if (!dma_vaddr[EN_DMA_WR_ID_VI_HIST_SPL_0 + index])
		return;

	val = (unsigned char *)dma_vaddr[EN_DMA_WR_ID_VI_HIST_SPL_0 + index] +
		144;

	for (i = 0; i < size; i++) {
		data[i] = val[3 * i];
		tmp = val[3 * i + 1];
		tmp = tmp << 8;
		data[i] |= tmp;
		tmp = val[3 * i + 2];
		tmp = tmp << 16;
		data[i] |= tmp;
	}
}

void am_dma_get_mif_data_cm2_hist_hue(int index,
	unsigned int *data, unsigned int length)
{
	int i = 0;
	unsigned int tmp = 0;
	unsigned int size = length;
	unsigned char *val;

	if (!data || length == 0)
		return;

	if (index > 1)
		index = 1;

	if (size > 32)
		size = 32;

	if (!dma_vaddr[EN_DMA_WR_ID_CM2_HIST_0 + index])
		return;

	val = (unsigned char *)dma_vaddr[EN_DMA_WR_ID_CM2_HIST_0 + index];

	for (i = 0; i < size; i++) {
		data[i] = val[3 * i];
		tmp = val[3 * i + 1];
		tmp = tmp << 8;
		data[i] |= tmp;
		tmp = val[3 * i + 2];
		tmp = tmp << 16;
		data[i] |= tmp;
	}
}

void am_dma_get_mif_data_cm2_hist_sat(int index,
	unsigned int *data, unsigned int length)
{
	int i = 0;
	unsigned int tmp = 0;
	unsigned int size = length;
	unsigned char *val;

	if (!data || length == 0)
		return;

	if (index > 1)
		index = 1;

	if (size > 32)
		size = 32;

	if (!dma_vaddr[EN_DMA_WR_ID_CM2_HIST_0 + index])
		return;

	val = (unsigned char *)dma_vaddr[EN_DMA_WR_ID_CM2_HIST_0 + index] +
		96;

	for (i = 0; i < size; i++) {
		data[i] = val[3 * i];
		tmp = val[3 * i + 1];
		tmp = tmp << 8;
		data[i] |= tmp;
		tmp = val[3 * i + 2];
		tmp = tmp << 16;
		data[i] |= tmp;
	}
}

void am_dma_get_mif_data_hdr2_hist(int index,
	unsigned int *data, unsigned int length)
{
	int i = 0;
	int j = 0;
	int k = 0;
	int offset = 0;
	unsigned int tmp = 0;
	unsigned int size = length;
	unsigned char *val;

	if (!data || length == 0)
		return;

	if (index > 2)
		index = 2;

	if (size > 128)
		size = 128;

	if (!dma_vaddr[EN_DMA_WR_ID_VD1_HDR_0 + index])
		return;

	val = (unsigned char *)dma_vaddr[EN_DMA_WR_ID_VD1_HDR_0 + index];

	for (i = 0; i < 26; i++) {
		offset = 16 * i;
		for (j = 0; j < 5; j++) {
			data[k] = val[offset + 3 * j];
			tmp = val[offset + 3 * j + 1];
			tmp = tmp << 8;
			data[k] |= tmp;
			tmp = val[offset + 3 * j + 2];
			tmp = tmp << 16;
			data[k] |= tmp;
			k++;

			if (k == size)
				break;
		}
	}
}

void am_dma_get_blend_vi_hist(unsigned int *data,
	unsigned int length)
{
	int i = 0;
	unsigned int size = length;
	unsigned int *val_0;
	unsigned int *val_1;
	unsigned int blend_val;

	if (!data || length == 0)
		return;

	if (size > 33)
		size = 33;

	if (!dma_vaddr[EN_DMA_WR_ID_VI_HIST_SPL_0] ||
		!dma_vaddr[EN_DMA_WR_ID_VI_HIST_SPL_0])
		return;

	val_0 = (unsigned int *)dma_vaddr[EN_DMA_WR_ID_VI_HIST_SPL_0];
	val_1 = (unsigned int *)dma_vaddr[EN_DMA_WR_ID_VI_HIST_SPL_1];

	for (i = 0; i < size; i++) {
		blend_val = val_0[i] + val_1[i];
		data[i] = blend_val >> 1;
	}
}

