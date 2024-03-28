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

static int am_dma_ctrl_dbg;
module_param(am_dma_ctrl_dbg, int, 0644);
MODULE_PARM_DESC(am_dma_ctrl_dbg, "am_dma_ctrl_dbg after t3x");

#define pr_am_dma(fmt, args...)\
	do {\
		if (am_dma_ctrl_dbg == 0x1) {\
			pr_info("am_dma_ctrl: " fmt, ## args);\
		} \
	} while (0)\

#define pr_am_dma_vi(fmt, args...)\
	do {\
		if (am_dma_ctrl_dbg == 0x2) {\
			pr_info("am_dma_ctrl: " fmt, ## args);\
		} \
	} while (0)\

#define pr_am_dma_hdr2(fmt, args...)\
	do {\
		if (am_dma_ctrl_dbg == 0x3) {\
			pr_info("am_dma_ctrl: " fmt, ## args);\
		} \
	} while (0)\

#define pr_am_dma_lc(fmt, args...)\
	do {\
		if (am_dma_ctrl_dbg == 0x4) {\
			pr_info("am_dma_ctrl: " fmt, ## args);\
		} \
	} while (0)\

#define ADDR_PARAM(page, reg)  (((page) << 8) | (reg))
#define UNIT_SIZE (16) /*128/8*/

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
	unsigned char reg_wrmif0_badr1;
	unsigned char reg_wrmif_sel;
	unsigned char reg_sr_mode_l2c1;
};

struct _viu_dma_reg_cfg_s {
	unsigned char page;
	unsigned char reg_ctrl0;
	unsigned char reg_ctrl1;
};

static struct _dma_reg_cfg_s dma_reg_cfg = {
	0x27,
	0xd5,
	0xd6,
	0xde,
	0xdf,
	0xee,
	0xa2,
};

static struct _viu_dma_reg_cfg_s viu_dma_reg_cfg = {
	0x1a,
	0x28,
	0x29,
};

static struct device vecm_dev;
static ulong alloc_size[EN_DMA_WR_ID_MAX];
static dma_addr_t dma_paddr[EN_DMA_WR_ID_MAX];
static void *dma_vaddr[EN_DMA_WR_ID_MAX];
static struct vpu_lut_dma_wr_s lut_dma_wr[EN_DMA_WR_ID_MAX];
static unsigned int dma_count[EN_DMA_WR_ID_MAX] = {
	12 * 8 * 4, 12 * 8 * 4,
	22, 22,
	16, 16,
	26, 26, 26,
	8 * 80,
};

static void _set_vpu_lut_dma_mif_wr_unit(int enable,
	struct vpu_lut_dma_wr_s *cfg_data)
{
	unsigned int addr;
	unsigned int val;
	int wr_sel;
	int offset;

	if (!cfg_data) {
		pr_info("%s: cfg data is NULL.\n", __func__);
		return;
	}

	if (cfg_data->dma_wr_id < 8) {
		wr_sel = 0;
		offset = cfg_data->dma_wr_id;
	} else {
		wr_sel = 1;
		offset = cfg_data->dma_wr_id - 8;
	}

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
	val = 0xff0 + wr_sel;
	WRITE_VPP_REG_S5(addr, val);

	pr_am_dma("%s: reg = %x, val = %x\n",
		__func__, addr, val);

	/*
	 * Bit 31 reserved
	 * Bit 30 reg_wr0_clr_fcnt unsigned, RW, default = 0
	 * Bit 29:28 reg_wr0_addr_mode unsigned, RW, default = 0
	 *           3 = only addr0, 0 = addr0/1 alternate
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
	val |= ((cfg_data->addr_mode & 0x3) << 28) |
		((cfg_data->rpt_num & 0xff) << 18) |
		((enable & 0x1) << 16) |
		(cfg_data->stride & 0x1fff);
	WRITE_VPP_REG_S5(addr, val);

	pr_am_dma("%s: reg = %x, val = %x\n",
		__func__, addr, val);

	addr = ADDR_PARAM(dma_reg_cfg.page,
		dma_reg_cfg.reg_wrmif0_badr0) + (offset << 1);
	WRITE_VPP_REG_S5(addr, cfg_data->baddr0);

	addr = ADDR_PARAM(dma_reg_cfg.page,
		dma_reg_cfg.reg_wrmif0_badr1) + (offset << 1);
	WRITE_VPP_REG_S5(addr, cfg_data->baddr1);
}

void am_dma_init(void)
{
	/*lut_dma_wr initial*/
	lut_dma_wr[EN_DMA_WR_ID_LC_STTS_0].stride = 12;/*24;*/
	lut_dma_wr[EN_DMA_WR_ID_LC_STTS_0].addr_mode = 1;
	lut_dma_wr[EN_DMA_WR_ID_LC_STTS_0].rpt_num = 32;/*16;*/

	lut_dma_wr[EN_DMA_WR_ID_LC_STTS_1].stride = 12;
	lut_dma_wr[EN_DMA_WR_ID_LC_STTS_1].addr_mode = 1;
	lut_dma_wr[EN_DMA_WR_ID_LC_STTS_1].rpt_num = 32;

	lut_dma_wr[EN_DMA_WR_ID_VI_HIST_SPL_0].stride = 22;/*2;*/
	lut_dma_wr[EN_DMA_WR_ID_VI_HIST_SPL_0].addr_mode = 3;/*1;*/
	lut_dma_wr[EN_DMA_WR_ID_VI_HIST_SPL_0].rpt_num = 0;/*11;*/

	lut_dma_wr[EN_DMA_WR_ID_VI_HIST_SPL_1].stride = 22;
	lut_dma_wr[EN_DMA_WR_ID_VI_HIST_SPL_1].addr_mode = 3;
	lut_dma_wr[EN_DMA_WR_ID_VI_HIST_SPL_1].rpt_num = 0;

	lut_dma_wr[EN_DMA_WR_ID_CM2_HIST_0].stride = 12;
	lut_dma_wr[EN_DMA_WR_ID_CM2_HIST_0].addr_mode = 3;
	lut_dma_wr[EN_DMA_WR_ID_CM2_HIST_0].rpt_num = 0;

	lut_dma_wr[EN_DMA_WR_ID_CM2_HIST_1].stride = 12;
	lut_dma_wr[EN_DMA_WR_ID_CM2_HIST_1].addr_mode = 3;
	lut_dma_wr[EN_DMA_WR_ID_CM2_HIST_1].rpt_num = 0;

	lut_dma_wr[EN_DMA_WR_ID_VD1_HDR_0].stride = 26;/*2;*/
	lut_dma_wr[EN_DMA_WR_ID_VD1_HDR_0].addr_mode = 3;/*1;*/
	lut_dma_wr[EN_DMA_WR_ID_VD1_HDR_0].rpt_num = 0;/*13;*/

	lut_dma_wr[EN_DMA_WR_ID_VD1_HDR_1].stride = 26;
	lut_dma_wr[EN_DMA_WR_ID_VD1_HDR_1].addr_mode = 3;
	lut_dma_wr[EN_DMA_WR_ID_VD1_HDR_1].rpt_num = 0;

	lut_dma_wr[EN_DMA_WR_ID_VD2_HDR].stride = 26;
	lut_dma_wr[EN_DMA_WR_ID_VD2_HDR].addr_mode = 3;
	lut_dma_wr[EN_DMA_WR_ID_VD2_HDR].rpt_num = 0;
}

void am_dma_reset_lc(int enable, int rdma_mode, int vpp_index)
{
	unsigned int addr;
	unsigned int val;
	unsigned int rpt_num;
	unsigned int stride_num;

	if (enable) {
		rpt_num = 1;
		stride_num = 4;
	} else {
		rpt_num = 8;
		stride_num = 48;
	}

	//pr_am_dma("%s: rpt_num = %d, stride_num = %d, rdma_mode = %d, vpp_index = %d\n",
	pr_info("%s: rpt_num = %d, stride_num = %d, rdma_mode = %d, vpp_index = %d\n",
		__func__, rpt_num, stride_num, rdma_mode, vpp_index);

	addr = ADDR_PARAM(dma_reg_cfg.page,
		dma_reg_cfg.reg_wrmif0_ctrl) + EN_DMA_WR_ID_LC_STTS_0;
	val = READ_VPP_REG_S5(addr);
	val = (val & 0xfc03e000) |
		((rpt_num & 0xff) << 18) |
		(stride_num & 0x1fff);
	if (rdma_mode)
		VSYNC_WRITE_VPP_REG_VPP_SEL(addr, val, vpp_index);
	else
		WRITE_VPP_REG_S5(addr, val);
	//pr_am_dma("%s: enable = %d, reg = 0x%x, val = 0x%08x\n",
	pr_info("%s: enable = %d, reg = 0x%x, val = 0x%08x\n",
		__func__, enable, addr, val);
}

void am_dma_set_wr_cfg(enum lut_dma_wr_id_e dma_wr_id, int enable,
	unsigned int stride, unsigned int addr_mode, unsigned int rpt_num)
{
	if (dma_wr_id != EN_DMA_WR_ID_MAX) {
		lut_dma_wr[dma_wr_id].stride = stride;
		lut_dma_wr[dma_wr_id].addr_mode = addr_mode;
		lut_dma_wr[dma_wr_id].rpt_num = rpt_num;

		if (dma_vaddr[dma_wr_id])
			_set_vpu_lut_dma_mif_wr_unit(enable,
				&lut_dma_wr[dma_wr_id]);
	}
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
				lut_dma_wr[i].baddr0 =
					(u32)(dma_paddr[i]) >> 4;
		}
	} else {
		alloc_size[dma_wr_id] = dma_count[dma_wr_id] * UNIT_SIZE;
		dma_vaddr[dma_wr_id] = dma_alloc_coherent(&vecm_dev,
			alloc_size[dma_wr_id], &dma_paddr[dma_wr_id], GFP_KERNEL);
		lut_dma_wr[dma_wr_id].dma_wr_id = dma_wr_id;
		if (dma_vaddr[dma_wr_id])
			lut_dma_wr[dma_wr_id].baddr0 =
				(u32)(dma_paddr[dma_wr_id]) >> 4;
	}
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

	pr_am_dma("%s: reg = %x, enable = %d\n",
		__func__, addr, enable);

	addr = ADDR_PARAM(dma_reg_cfg.page,
		dma_reg_cfg.reg_sr_mode_l2c1);
	WRITE_VPP_REG_BITS_S5(addr, 1, 22, 1);
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
	unsigned int offset = 0;
	unsigned int min_max = 0;

	if (!data || length == 0) {
		pr_am_dma("%s: data or length not fit.\n",
			__func__);
		return;
	}

	if (index > 1)
		index = 1;

	if (size > 12 * 8 * 17)
		size = 1632;

	if (!dma_vaddr[EN_DMA_WR_ID_LC_STTS_0 + index]) {
		pr_am_dma("%s: dma_vaddr %d is NULL.\n",
			__func__, index);
		return;
	}

	val = (unsigned int *)dma_vaddr[EN_DMA_WR_ID_LC_STTS_0 + index];
	pr_am_dma_lc("%s: index =  %d\n", __func__, index);

	for (i = 0; i < 12 * 8; i++) {
		for (j = 0; j < 16; j++) {
			data[k] = val[i * 16 + j] & 0xffffff;
			k++;

			pr_am_dma_lc("%s: dma_data[%d] = %x\n",
				__func__, i * 16 + j, val[i * 16 + j]);

			if (j == 3 || j == 7 || j == 11 || j == 15) {
				tmp = (val[i * 16 + j] >> 24) & 0x1f;
				min_max |= (tmp << offset);

				if (j == 15) {
					data[k] = min_max;
					min_max = 0;
					offset = 0;
					k++;
				} else {
					offset += 5;
				}
			}

			if (k == size)
				break;
		}
	}
}

void am_dma_get_mif_data_vi_hist(int index,
	unsigned short *data, unsigned int length)
{
	int i = 0;
	unsigned int size = length;
	unsigned short *val;

	if (!data || length == 0) {
		pr_am_dma("%s: data or length not fit.\n",
			__func__);
		return;
	}

	if (index > 1)
		index = 1;

	if (size > 64)
		size = 64;

	if (!dma_vaddr[EN_DMA_WR_ID_VI_HIST_SPL_0 + index]) {
		pr_am_dma("%s: dma_vaddr %d is NULL.\n",
			__func__, index);
		return;
	}

	val = (unsigned short *)dma_vaddr[EN_DMA_WR_ID_VI_HIST_SPL_0 + index];

	for (i = 0; i < size; i++) {
		data[i] = val[i];
		pr_am_dma_vi("%s: val[%d] is %d.\n",
			__func__, i, val[i]);
	}
}

void am_dma_get_mif_data_vi_hist_low(int index,
	unsigned short *data, unsigned int length)
{
	int i = 0;
	int j = 0;
	int k = 0;
	int offset = 0;
	unsigned short tmp = 0;
	unsigned int size = length;
	unsigned char *val;

	if (!data || length == 0) {
		pr_am_dma("%s: data or length not fit.\n",
			__func__);
		return;
	}

	if (index > 1)
		index = 1;

	if (size > 64)
		size = 64;

	if (!dma_vaddr[EN_DMA_WR_ID_VI_HIST_SPL_0 + index]) {
		pr_am_dma("%s: dma_vaddr %d is NULL.\n",
			__func__, index);
		return;
	}

	val = (unsigned char *)dma_vaddr[EN_DMA_WR_ID_VI_HIST_SPL_0 + index] +
		144;

	for (i = 0; i < 13; i++) {
		offset = 16 * i;
		for (j = 0; j < 5; j++) {
			data[k] = val[offset + 3 * j];
			tmp = val[offset + 3 * j + 1];
			tmp = tmp << 8;
			data[k] |= tmp;
			/*tmp = val[offset + 3 * j + 2];*/
			/*tmp = tmp << 16;*/
			/*data[k] |= tmp;*/
			k++;

			if (k == size)
				break;
		}
	}
}

void am_dma_get_mif_data_cm2_hist_hue(int index,
	unsigned int *data, unsigned int length)
{
	int i = 0;
	unsigned int size = length;
	unsigned int *val;

	if (!data || length == 0) {
		pr_am_dma("%s: data or length not fit.\n",
			__func__);
		return;
	}

	if (index > 1)
		index = 1;

	if (size > 32)
		size = 32;

	if (!dma_vaddr[EN_DMA_WR_ID_CM2_HIST_0 + index]) {
		pr_am_dma("%s: dma_vaddr %d is NULL.\n",
			__func__, index);
		return;
	}

	val = (unsigned int *)dma_vaddr[EN_DMA_WR_ID_CM2_HIST_0 + index];

	for (i = 0; i < size; i++)
		data[i] = val[i];
}

void am_dma_get_mif_data_cm2_hist_sat(int index,
	unsigned int *data, unsigned int length)
{
	int i = 0;
	unsigned int size = length;
	unsigned int *val;

	if (!data || length == 0) {
		pr_am_dma("%s: data or length not fit.\n",
			__func__);
		return;
	}

	if (index > 1)
		index = 1;

	if (size > 32)
		size = 32;

	if (!dma_vaddr[EN_DMA_WR_ID_CM2_HIST_0 + index]) {
		pr_am_dma("%s: dma_vaddr %d is NULL.\n",
			__func__, index);
		return;
	}

	val = (unsigned int *)dma_vaddr[EN_DMA_WR_ID_CM2_HIST_0 + index] +
		32;

	for (i = 0; i < size; i++)
		data[i] = val[i];
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

	if (!data || length == 0) {
		pr_am_dma("%s: data or length not fit.\n",
			__func__);
		return;
	}

	if (index > 2)
		index = 2;

	if (size > 128)
		size = 128;

	if (!dma_vaddr[EN_DMA_WR_ID_VD1_HDR_0 + index]) {
		pr_am_dma("%s: dma_vaddr %d is NULL.\n",
			__func__, index);
		return;
	}

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

void am_dma_get_blend_vi_hist(unsigned short *data,
	unsigned int length)
{
	int i = 0;
	unsigned int size = length;
	unsigned short *val_0;
	unsigned short *val_1;

	if (!data || length == 0) {
		pr_am_dma("%s: data or length not fit.\n",
			__func__);
		return;
	}

	if (size > 64)
		size = 64;

	if (!dma_vaddr[EN_DMA_WR_ID_VI_HIST_SPL_0] ||
		!dma_vaddr[EN_DMA_WR_ID_VI_HIST_SPL_1]) {
		pr_am_dma("%s: dma_vaddr is NULL.\n",
			__func__);
		return;
	}

	val_0 = (unsigned short *)dma_vaddr[EN_DMA_WR_ID_VI_HIST_SPL_0];
	val_1 = (unsigned short *)dma_vaddr[EN_DMA_WR_ID_VI_HIST_SPL_1];

	for (i = 0; i < size; i++) {
		data[i] = val_0[i] + val_1[i];
		pr_am_dma_vi("%s: dma_val_0/1[%d] = %d/%d\n",
			__func__, i, val_0[i], val_1[i]);
	}
}

void am_dma_get_blend_vi_hist_low(unsigned short *data,
	unsigned int length)
{
	int i = 0;
	unsigned int size = length;
	unsigned char *val_0;
	unsigned char *val_1;
	unsigned short tmp = 0;
	unsigned short tmp_0 = 0;
	unsigned short tmp_1 = 0;

	if (!data || length == 0) {
		pr_am_dma("%s: data or length not fit.\n",
			__func__);
		return;
	}

	if (size > 64)
		size = 64;

	if (!dma_vaddr[EN_DMA_WR_ID_VI_HIST_SPL_0] ||
		!dma_vaddr[EN_DMA_WR_ID_VI_HIST_SPL_1]) {
		pr_am_dma("%s: dma_vaddr is NULL.\n",
			__func__);
		return;
	}

	val_0 = (unsigned char *)dma_vaddr[EN_DMA_WR_ID_VI_HIST_SPL_0] +
		144;
	val_1 = (unsigned char *)dma_vaddr[EN_DMA_WR_ID_VI_HIST_SPL_1] +
		144;

	for (i = 0; i < size; i++) {
		tmp_0 = val_0[3 * i];
		tmp = val_0[3 * i + 1];
		tmp = tmp << 8;
		tmp_0 |= tmp;
		/*tmp = val_0[3 * i + 2];*/
		/*tmp = tmp << 16;*/
		/*tmp_0 |= tmp;*/

		tmp_1 = val_1[3 * i];
		tmp = val_1[3 * i + 1];
		tmp = tmp << 8;
		tmp_1 |= tmp;
		/*tmp = val_1[3 * i + 2];*/
		/*tmp = tmp << 16;*/
		/*tmp_1 |= tmp;*/

		data[i] = tmp_0 + tmp_1;
		pr_am_dma_vi("%s: val_0/1[%d] = %d/%d\n",
			__func__, i, tmp_0, tmp_1);
	}
}

void am_dma_get_blend_cm2_hist_hue(unsigned int *data,
	unsigned int length)
{
	int i = 0;
	unsigned int size = length;
	unsigned char *val_0;
	unsigned char *val_1;
	unsigned int tmp = 0;
	unsigned int tmp_0 = 0;
	unsigned int tmp_1 = 0;

	if (!data || length == 0) {
		pr_am_dma("%s: data or length not fit.\n",
			__func__);
		return;
	}

	if (size > 32)
		size = 32;

	if (!dma_vaddr[EN_DMA_WR_ID_CM2_HIST_0] ||
		!dma_vaddr[EN_DMA_WR_ID_CM2_HIST_1]) {
		pr_am_dma("%s: dma_vaddr is NULL.\n",
			__func__);
		return;
	}

	val_0 = (unsigned char *)dma_vaddr[EN_DMA_WR_ID_CM2_HIST_0];
	val_1 = (unsigned char *)dma_vaddr[EN_DMA_WR_ID_CM2_HIST_1];

	for (i = 0; i < size; i++) {
		tmp_0 = val_0[3 * i];
		tmp = val_0[3 * i + 1];
		tmp = tmp << 8;
		tmp_0 |= tmp;
		tmp = val_0[3 * i + 2];
		tmp = tmp << 16;
		tmp_0 |= tmp;

		tmp_1 = val_1[3 * i];
		tmp = val_1[3 * i + 1];
		tmp = tmp << 8;
		tmp_1 |= tmp;
		tmp = val_1[3 * i + 2];
		tmp = tmp << 16;
		tmp_1 |= tmp;

		data[i] = tmp_0 + tmp_1;
	}
}

void am_dma_get_blend_cm2_hist_sat(unsigned int *data,
	unsigned int length)
{
	int i = 0;
	unsigned int size = length;
	unsigned char *val_0;
	unsigned char *val_1;
	unsigned int tmp = 0;
	unsigned int tmp_0 = 0;
	unsigned int tmp_1 = 0;

	if (!data || length == 0) {
		pr_am_dma("%s: data or length not fit.\n",
			__func__);
		return;
	}

	if (size > 32)
		size = 32;

	if (!dma_vaddr[EN_DMA_WR_ID_CM2_HIST_0] ||
		!dma_vaddr[EN_DMA_WR_ID_CM2_HIST_1]) {
		pr_am_dma("%s: dma_vaddr is NULL.\n",
			__func__);
		return;
	}

	val_0 = (unsigned char *)dma_vaddr[EN_DMA_WR_ID_CM2_HIST_0] +
		96;
	val_1 = (unsigned char *)dma_vaddr[EN_DMA_WR_ID_CM2_HIST_1] +
		96;

	for (i = 0; i < size; i++) {
		tmp_0 = val_0[3 * i];
		tmp = val_0[3 * i + 1];
		tmp = tmp << 8;
		tmp_0 |= tmp;
		tmp = val_0[3 * i + 2];
		tmp = tmp << 16;
		tmp_0 |= tmp;

		tmp_1 = val_1[3 * i];
		tmp = val_1[3 * i + 1];
		tmp = tmp << 8;
		tmp_1 |= tmp;
		tmp = val_1[3 * i + 2];
		tmp = tmp << 16;
		tmp_1 |= tmp;

		data[i] = tmp_0 + tmp_1;
	}
}

void am_dma_get_blend_hdr2_hist(unsigned int *data,
	unsigned int length)
{
	int i = 0;
	int j = 0;
	int k = 0;
	int offset = 0;
	unsigned int size = length;
	unsigned char *val_0;
	unsigned char *val_1;
	unsigned int tmp = 0;
	unsigned int tmp_0 = 0;
	unsigned int tmp_1 = 0;

	if (!data || length == 0) {
		pr_am_dma("%s: data or length not fit.\n",
			__func__);
		return;
	}

	if (size > 128)
		size = 128;

	if (!dma_vaddr[EN_DMA_WR_ID_VD1_HDR_0] ||
		!dma_vaddr[EN_DMA_WR_ID_VD1_HDR_1]) {
		pr_am_dma("%s: dma_vaddr is NULL.\n",
			__func__);
		return;
	}

	val_0 = (unsigned char *)dma_vaddr[EN_DMA_WR_ID_VD1_HDR_0];
	val_1 = (unsigned char *)dma_vaddr[EN_DMA_WR_ID_VD1_HDR_1];

	for (i = 0; i < 26; i++) {
		offset = 16 * i;
		for (j = 0; j < 5; j++) {
			tmp_0 = val_0[offset + 3 * j];
			tmp = val_0[offset + 3 * j + 1];
			tmp = tmp << 8;
			tmp_0 |= tmp;
			tmp = val_0[offset + 3 * j + 2];
			tmp = tmp << 16;
			tmp_0 |= tmp;

			tmp_1 = val_1[offset + 3 * j];
			tmp = val_1[offset + 3 * j + 1];
			tmp = tmp << 8;
			tmp_1 |= tmp;
			tmp = val_1[offset + 3 * j + 2];
			tmp = tmp << 16;
			tmp_1 |= tmp;

			data[k] = tmp_0 + tmp_1;
			k++;

			pr_am_dma_hdr2("%s: val_0/1[%d] = %d/%d\n",
				__func__, i, tmp_0, tmp_1);

			if (k == size)
				break;
		}
	}
}

/*For 3D LUT*/
#define VPU_DMA_RDMIF0_CTRL   0x2750
#define VPU_DMA_RDMIF1_CTRL   0x2751
#define VPU_DMA_RDMIF2_CTRL   0x2752
#define VPU_DMA_RDMIF3_CTRL   0x2753
#define VPU_DMA_RDMIF4_CTRL   0x2754
#define VPU_DMA_RDMIF5_CTRL   0x2755
#define VPU_DMA_RDMIF6_CTRL   0x2756
#define VPU_DMA_RDMIF7_CTRL   0x2757

#define VPU_DMA_RDMIF0_BADR0  0x2758
#define VPU_DMA_RDMIF0_BADR1  0x2759
#define VPU_DMA_RDMIF0_BADR2  0x275a
#define VPU_DMA_RDMIF0_BADR3  0x275b
#define VPU_DMA_RDMIF1_BADR0  0x275c
#define VPU_DMA_RDMIF1_BADR1  0x275d
#define VPU_DMA_RDMIF1_BADR2  0x275e
#define VPU_DMA_RDMIF1_BADR3  0x275f
#define VPU_DMA_RDMIF2_BADR0  0x2760
#define VPU_DMA_RDMIF2_BADR1  0x2761
#define VPU_DMA_RDMIF2_BADR2  0x2762
#define VPU_DMA_RDMIF2_BADR3  0x2763
#define VPU_DMA_RDMIF3_BADR0  0x2764
#define VPU_DMA_RDMIF3_BADR1  0x2765
#define VPU_DMA_RDMIF3_BADR2  0x2766
#define VPU_DMA_RDMIF3_BADR3  0x2767
#define VPU_DMA_RDMIF4_BADR0  0x2768
#define VPU_DMA_RDMIF4_BADR1  0x2769
#define VPU_DMA_RDMIF4_BADR2  0x276a
#define VPU_DMA_RDMIF4_BADR3  0x276b
#define VPU_DMA_RDMIF5_BADR0  0x276c
#define VPU_DMA_RDMIF5_BADR1  0x276d
#define VPU_DMA_RDMIF5_BADR2  0x276e
#define VPU_DMA_RDMIF5_BADR3  0x276f
#define VPU_DMA_RDMIF6_BADR0  0x2770
#define VPU_DMA_RDMIF6_BADR1  0x2771
#define VPU_DMA_RDMIF6_BADR2  0x2772
#define VPU_DMA_RDMIF6_BADR3  0x2773
#define VPU_DMA_RDMIF7_BADR0  0x2774
#define VPU_DMA_RDMIF7_BADR1  0x2775
#define VPU_DMA_RDMIF7_BADR2  0x2776
#define VPU_DMA_RDMIF7_BADR3  0x2777
#define VPU_DMA_RDMIF_SEL     0x2778

#define VPU_DMA_WRMIF_CTRL1   0x27d1
#define VPU_DMA_WRMIF_CTRL2   0x27d2
#define VPU_DMA_WRMIF_CTRL3   0x27d3
#define VPU_DMA_WRMIF_BADDR0  0x27d4
#define VPU_DMA_WRMIF_RO_STAT 0x27d7
#define VPU_DMA_RDMIF_CTRL    0x27d8
#define VPU_DMA_RDMIF_BADDR1  0x27d9
#define VPU_DMA_RDMIF_BADDR2  0x27da
#define VPU_DMA_RDMIF_BADDR3  0x27db
#define VPU_DMA_WRMIF_CTRL    0x27dc
#define VPU_DMA_WRMIF_BADDR1  0x27dd
#define VPU_DMA_WRMIF_BADDR2  0x27de
#define VPU_DMA_WRMIF_BADDR3  0x27df

#define VPU_DMA_RDMIF_CTRL1   0x27ca
#define VPU_DMA_RDMIF_CTRL2   0x27cb
#define VPU_DMA_RDMIF_RO_STAT 0x27d0

#define DMA_SIZE_TOTAL_LUT3D (17 * 17 * 17)

enum lut_dma_id_e {
	EN_DMA_ID_LDIM_STTS = 0,
	EN_DMA_ID_DI_FILM,
	EN_DMA_ID_VD1_S0_FILM,
	EN_DMA_ID_VD1_S1_FILM,
	EN_DMA_ID_VD1_S2_FILM,
	EN_DMA_ID_VD1_S3_FILM, /*5*/
	EN_DMA_ID_VD2_FILM,
	EN_DMA_ID_TCON,
	EN_DMA_ID_LUT3D,
	EN_DMA_ID_HDR,
};

struct vpu_lut_dma_type_s {
	enum lut_dma_id_e dma_id;
	/*reg_hdr_dma_sel:*/
	u32 reg_hdr_dma_sel_vd1s0; /*4bits, default 1*/
	u32 reg_hdr_dma_sel_vd1s1; /*4bits, default 2*/
	u32 reg_hdr_dma_sel_vd1s2; /*4bits, default 3*/
	u32 reg_hdr_dma_sel_vd1s3; /*4bits, default 4*/
	u32 reg_hdr_dma_sel_vd2;   /*4bits, default 5*/
	u32 reg_hdr_dma_sel_osd1;  /*4bits, default 6*/
	u32 reg_hdr_dma_sel_osd2;  /*4bits, default 7*/
	u32 reg_hdr_dma_sel_osd3;  /*4bits, default 8*/
	/*reg_dma_mode : 1: cfg hdr2 lut with lut_dma, 0: with cbus mode*/
	u32 reg_vd1s0_hdr_dma_mode;  /*1bit*/
	u32 reg_vd1s1_hdr_dma_mode;  /*1bit*/
	u32 reg_vd1s2_hdr_dma_mode;  /*1bit*/
	u32 reg_vd1s3_hdr_dma_mode;  /*1bit*/
	u32 reg_vd2_hdr_dma_mode;    /*1bit*/
	u32 reg_osd1_hdr_dma_mode;   /*1bit*/
	u32 reg_osd2_hdr_dma_mode;   /*1bit*/
	u32 reg_osd3_hdr_dma_mode;   /*1bit*/

	/*mif info : TODO more params*/
	u32 rd_wr_sel;
	u32 mif_baddr[16][4];
	u32 chan_rd_bytes_num[16]; /*rdmif rd num*/
	u32 chan_sel_src_num[16];  /*channel select interrupt source num*/
	u32 chan_little_endian[16];
	u32 chan_swap_64bit[16];
};

static struct device vecm_dev_lut3d;
static ulong alloc_size_lut3d;
static void *dma_vaddr_lut3d;
static dma_addr_t dma_paddr_lut3d;

static void _init_vpu_lut_dma(struct vpu_lut_dma_type_s *vpu_lut_dma)
{
	memset((void *)vpu_lut_dma, 0,
		sizeof(struct vpu_lut_dma_type_s));

	/*default: must identical to coef data in DDR BACKDOOR*/
	vpu_lut_dma->reg_hdr_dma_sel_vd1s0 = 1;
	vpu_lut_dma->reg_hdr_dma_sel_vd1s1 = 2;
	vpu_lut_dma->reg_hdr_dma_sel_vd1s2 = 3;
	vpu_lut_dma->reg_hdr_dma_sel_vd1s3 = 4;
	vpu_lut_dma->reg_hdr_dma_sel_vd2 = 5;
	vpu_lut_dma->reg_hdr_dma_sel_osd1 = 6;
	vpu_lut_dma->reg_hdr_dma_sel_osd2 = 7;
	vpu_lut_dma->reg_hdr_dma_sel_osd3 = 8;
}

static void _set_vpu_lut_dma_mif(struct vpu_lut_dma_type_s      *vpu_lut_dma)
{
	u32 mif_num = 0;
	u32 lut_reg_sel8_15 = 0;
	u32 reg_badr0 = 0;
	u32 reg_badr1 = 0;
	u32 reg_badr2 = 0;
	u32 reg_badr3 = 0;
	u32 reg_ctrl = 0;

	if (vpu_lut_dma->dma_id == EN_DMA_ID_LDIM_STTS) {
		mif_num = 0;
		reg_badr0 = VPU_DMA_RDMIF0_BADR0;
		reg_badr1 = VPU_DMA_RDMIF0_BADR1;
		reg_badr2 = VPU_DMA_RDMIF0_BADR2;
		reg_badr3 = VPU_DMA_RDMIF0_BADR3;
		reg_ctrl = VPU_DMA_RDMIF0_CTRL;
	} else if (vpu_lut_dma->dma_id == EN_DMA_ID_DI_FILM) {
		mif_num = 1;
		reg_badr0 = VPU_DMA_RDMIF1_BADR0;
		reg_badr1 = VPU_DMA_RDMIF1_BADR1;
		reg_badr2 = VPU_DMA_RDMIF1_BADR2;
		reg_badr3 = VPU_DMA_RDMIF1_BADR3;
		reg_ctrl = VPU_DMA_RDMIF1_CTRL;
	} else if (vpu_lut_dma->dma_id == EN_DMA_ID_VD1_S0_FILM) {
		mif_num = 2;
		reg_badr0 = VPU_DMA_RDMIF2_BADR0;
		reg_badr1 = VPU_DMA_RDMIF2_BADR1;
		reg_badr2 = VPU_DMA_RDMIF2_BADR2;
		reg_badr3 = VPU_DMA_RDMIF2_BADR3;
		reg_ctrl = VPU_DMA_RDMIF2_CTRL;
	} else if (vpu_lut_dma->dma_id == EN_DMA_ID_VD1_S1_FILM) {
		mif_num = 3;
		reg_badr0 = VPU_DMA_RDMIF3_BADR0;
		reg_badr1 = VPU_DMA_RDMIF3_BADR1;
		reg_badr2 = VPU_DMA_RDMIF3_BADR2;
		reg_badr3 = VPU_DMA_RDMIF3_BADR3;
		reg_ctrl = VPU_DMA_RDMIF3_CTRL;
	} else if (vpu_lut_dma->dma_id == EN_DMA_ID_VD1_S2_FILM) {
		mif_num = 4;
		reg_badr0 = VPU_DMA_RDMIF4_BADR0;
		reg_badr1 = VPU_DMA_RDMIF4_BADR1;
		reg_badr2 = VPU_DMA_RDMIF4_BADR2;
		reg_badr3 = VPU_DMA_RDMIF4_BADR3;
		reg_ctrl = VPU_DMA_RDMIF4_CTRL;
	} else if (vpu_lut_dma->dma_id == EN_DMA_ID_VD1_S3_FILM) {
		mif_num = 5;
		reg_badr0 = VPU_DMA_RDMIF5_BADR0;
		reg_badr1 = VPU_DMA_RDMIF5_BADR1;
		reg_badr2 = VPU_DMA_RDMIF5_BADR2;
		reg_badr3 = VPU_DMA_RDMIF5_BADR3;
		reg_ctrl = VPU_DMA_RDMIF5_CTRL;
	} else if (vpu_lut_dma->dma_id == EN_DMA_ID_VD2_FILM) {
		mif_num = 6;
		reg_badr0 = VPU_DMA_RDMIF6_BADR0;
		reg_badr1 = VPU_DMA_RDMIF6_BADR1;
		reg_badr2 = VPU_DMA_RDMIF6_BADR2;
		reg_badr3 = VPU_DMA_RDMIF6_BADR3;
		reg_ctrl = VPU_DMA_RDMIF6_CTRL;
	} else if (vpu_lut_dma->dma_id == EN_DMA_ID_TCON) {
		mif_num = 7;
		reg_badr0 = VPU_DMA_RDMIF7_BADR0;
		reg_badr1 = VPU_DMA_RDMIF7_BADR1;
		reg_badr2 = VPU_DMA_RDMIF7_BADR2;
		reg_badr3 = VPU_DMA_RDMIF7_BADR3;
		reg_ctrl = VPU_DMA_RDMIF7_CTRL;
	} else if (vpu_lut_dma->dma_id == EN_DMA_ID_LUT3D) {
		mif_num = 8;
		lut_reg_sel8_15 = 1;
		reg_badr0 = VPU_DMA_RDMIF0_BADR0;
		reg_badr1 = VPU_DMA_RDMIF0_BADR1;
		reg_badr2 = VPU_DMA_RDMIF0_BADR2;
		reg_badr3 = VPU_DMA_RDMIF0_BADR3;
		reg_ctrl = VPU_DMA_RDMIF0_CTRL;
	} else if (vpu_lut_dma->dma_id == EN_DMA_ID_HDR) {
		mif_num = 9;
		lut_reg_sel8_15 = 1;
		reg_badr0 = VPU_DMA_RDMIF1_BADR0;
		reg_badr1 = VPU_DMA_RDMIF1_BADR1;
		reg_badr2 = VPU_DMA_RDMIF1_BADR2;
		reg_badr3 = VPU_DMA_RDMIF1_BADR3;
		reg_ctrl = VPU_DMA_RDMIF1_CTRL;
	} else {
		return;
	}

	WRITE_VPP_REG_BITS_S5(VPU_DMA_RDMIF_SEL,
		lut_reg_sel8_15, 0, 1);

	WRITE_VPP_REG_S5(reg_badr0,
		vpu_lut_dma->mif_baddr[mif_num][0]);
	WRITE_VPP_REG_S5(reg_badr1,
		vpu_lut_dma->mif_baddr[mif_num][1]);
	WRITE_VPP_REG_S5(reg_badr2,
		vpu_lut_dma->mif_baddr[mif_num][2]);
	WRITE_VPP_REG_S5(reg_badr3,
		vpu_lut_dma->mif_baddr[mif_num][3]);

	/*reg_rd0_stride*/
	WRITE_VPP_REG_BITS_S5(reg_ctrl,
		vpu_lut_dma->chan_rd_bytes_num[mif_num], 0, 13);

	/*Bit 13 little_endian*/
	WRITE_VPP_REG_BITS_S5(reg_ctrl,
		vpu_lut_dma->chan_little_endian[mif_num], 13, 1);

	/*Bit 14 swap_64bit*/
	WRITE_VPP_REG_BITS_S5(reg_ctrl,
		vpu_lut_dma->chan_swap_64bit[mif_num], 14, 1);

	/*Bit 23:16*/
	/*reg_rd0_enable_int,*/
	/*channel0 select interrupt source*/
	WRITE_VPP_REG_BITS_S5(reg_ctrl,
		vpu_lut_dma->chan_sel_src_num[mif_num], 16, 8);
}

static void _set_vpu_lut_dma(struct vpu_lut_dma_type_s      *vpu_lut_dma)
{
	unsigned int val;
	unsigned int addr;

	addr = ADDR_PARAM(viu_dma_reg_cfg.page,
		viu_dma_reg_cfg.reg_ctrl0);
	val = READ_VPP_REG_S5(addr);
	val |= vpu_lut_dma->reg_vd1s0_hdr_dma_mode << 0 |
		vpu_lut_dma->reg_vd1s1_hdr_dma_mode << 1 |
		vpu_lut_dma->reg_vd1s2_hdr_dma_mode << 2 |
		vpu_lut_dma->reg_vd1s3_hdr_dma_mode << 3 |
		vpu_lut_dma->reg_vd2_hdr_dma_mode << 4 |
		vpu_lut_dma->reg_osd1_hdr_dma_mode << 6 |
		vpu_lut_dma->reg_osd2_hdr_dma_mode << 7 |
		vpu_lut_dma->reg_osd3_hdr_dma_mode << 8;
	WRITE_VPP_REG_S5(addr, val);

	addr = ADDR_PARAM(viu_dma_reg_cfg.page,
		viu_dma_reg_cfg.reg_ctrl1);
	val = READ_VPP_REG_S5(addr);
	val |= vpu_lut_dma->reg_hdr_dma_sel_vd1s0 << 0  |
		vpu_lut_dma->reg_hdr_dma_sel_vd1s1 << 4 |
		vpu_lut_dma->reg_hdr_dma_sel_vd1s2 << 8 |
		vpu_lut_dma->reg_hdr_dma_sel_vd1s3 << 12 |
		vpu_lut_dma->reg_hdr_dma_sel_vd2 << 16 |
		vpu_lut_dma->reg_hdr_dma_sel_osd1 << 20 |
		vpu_lut_dma->reg_hdr_dma_sel_osd2 << 24 |
		vpu_lut_dma->reg_hdr_dma_sel_osd3 << 28;
	WRITE_VPP_REG_S5(addr, val);

	_set_vpu_lut_dma_mif(vpu_lut_dma);
}

static void _fill_dma_data_lut3d(void *dma_vaddr,
	int *lut_data, int length)
{
	if (!dma_vaddr || !lut_data)
		return;
}

void am_dma_lut3d_buffer_malloc(struct platform_device *pdev)
{
	vecm_dev_lut3d = pdev->dev;
	alloc_size_lut3d = DMA_SIZE_TOTAL_LUT3D;
	dma_vaddr_lut3d = dma_alloc_coherent(&vecm_dev_lut3d,
		alloc_size_lut3d, &dma_paddr_lut3d, GFP_KERNEL);
}

void am_dma_lut3d_buffer_free(struct platform_device *pdev)
{
	vecm_dev_lut3d = pdev->dev;
	dma_free_coherent(&vecm_dev_lut3d, alloc_size_lut3d,
		dma_vaddr_lut3d, dma_paddr_lut3d);
}

void am_dma_lut3d_set_data(int *data, int length)
{
	u32 dma_id_int;
	struct vpu_lut_dma_type_s vpu_lut_dma;

	if (!data || length <= 0)
		return;

	_fill_dma_data_lut3d(NULL, NULL, 0);

	_init_vpu_lut_dma(&vpu_lut_dma);

	vpu_lut_dma.dma_id = EN_DMA_ID_LUT3D;
	dma_id_int = (u32)(vpu_lut_dma.dma_id);
	/*0:config wr_mif, 1:config rd_mif*/
	vpu_lut_dma.rd_wr_sel = 1;
	vpu_lut_dma.mif_baddr[dma_id_int][0] = (u32)(dma_paddr_lut3d) >> 4;
	vpu_lut_dma.mif_baddr[dma_id_int][1] = (u32)(dma_paddr_lut3d) >> 4;
	vpu_lut_dma.mif_baddr[dma_id_int][2] = (u32)(dma_paddr_lut3d) >> 4;
	vpu_lut_dma.mif_baddr[dma_id_int][3] = (u32)(dma_paddr_lut3d) >> 4;
	vpu_lut_dma.chan_rd_bytes_num[dma_id_int] = DMA_SIZE_TOTAL_LUT3D;
	/*select viu_vsync_int_i[0]*/
	vpu_lut_dma.chan_sel_src_num[dma_id_int] = 1;

	_set_vpu_lut_dma(&vpu_lut_dma);
}

void am_dma_lut3d_get_data(int *data, int length)
{
	if (!data || length <= 0)
		return;
}

