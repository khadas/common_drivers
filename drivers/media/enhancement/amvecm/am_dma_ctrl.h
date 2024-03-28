/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef AM_DMA_CTRL_H
#define AM_DMA_CTRL_H

#include "set_hdr2_v0.h"

enum lut_dma_wr_id_e {
	EN_DMA_WR_ID_LC_STTS_0 = 0,
	EN_DMA_WR_ID_LC_STTS_1,
	EN_DMA_WR_ID_VI_HIST_SPL_0,
	EN_DMA_WR_ID_VI_HIST_SPL_1,
	EN_DMA_WR_ID_CM2_HIST_0,
	EN_DMA_WR_ID_CM2_HIST_1, /*5*/
	EN_DMA_WR_ID_VD1_HDR_0,
	EN_DMA_WR_ID_VD1_HDR_1,
	EN_DMA_WR_ID_VD2_HDR,
	EN_DMA_WR_ID_AMBIENT_LIGHT,
	EN_DMA_WR_ID_MAX,
};

void am_dma_init(void);
void am_dma_reset_lc(int enable, int rdma_mode, int vpp_index);
void am_dma_set_wr_cfg(enum lut_dma_wr_id_e dma_wr_id, int enable,
	unsigned int stride, unsigned int addr_mode, unsigned int rpt_num);

void am_dma_buffer_malloc(struct platform_device *pdev,
	enum lut_dma_wr_id_e dma_wr_id);
void am_dma_buffer_free(struct platform_device *pdev,
	enum lut_dma_wr_id_e dma_wr_id);

void am_dma_set_mif_wr_status(int enable);
void am_dma_set_mif_wr(enum lut_dma_wr_id_e dma_wr_id,
	int enable);

void am_dma_get_mif_data_lc_stts(int index,
	unsigned int *data, unsigned int length);
void am_dma_get_mif_data_vi_hist(int index,
	unsigned short *data, unsigned int length);
void am_dma_get_mif_data_vi_hist_low(int index,
	unsigned short *data, unsigned int length);
void am_dma_get_mif_data_cm2_hist_hue(int index,
	unsigned int *data, unsigned int length);
void am_dma_get_mif_data_cm2_hist_sat(int index,
	unsigned int *data, unsigned int length);
void am_dma_get_mif_data_hdr2_hist(int index,
	unsigned int *data, unsigned int length);

void am_dma_get_blend_vi_hist(unsigned short *data,
	unsigned int length);
void am_dma_get_blend_vi_hist_low(unsigned short *data,
	unsigned int length);
void am_dma_get_blend_cm2_hist_hue(unsigned int *data,
	unsigned int length);
void am_dma_get_blend_cm2_hist_sat(unsigned int *data,
	unsigned int length);
void am_dma_get_blend_hdr2_hist(unsigned int *data,
	unsigned int length);

void am_dma_lut3d_buffer_malloc(struct platform_device *pdev);
void am_dma_lut3d_buffer_free(struct platform_device *pdev);
void am_dma_lut3d_set_data(int *data, int length);
void am_dma_lut3d_get_data(int *data, int length);
#endif
