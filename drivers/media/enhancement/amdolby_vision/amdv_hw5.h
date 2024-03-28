/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _AMDV_HW5_H_
#define _AMDV_HW5_H_

/*  driver version */
#define HW5_DRIVER_VER "20230612"

#include <linux/types.h>

#define HIST_BUF_COUNT 3

struct dolby5_top1_md_hist {
	u32 l1l4_md[HIST_BUF_COUNT][4];
	u8 hist[256];
	void *hist_vaddr[2];
	dma_addr_t hist_paddr[2];
	u32 hist_size;
	u16 histogram[HIST_BUF_COUNT][128];
};

struct dolby5_top1_type {
	int core1_hsize;
	int core1_vsize;
	int rdma_num;
	int rdma_size;
	int py_level;
	dma_addr_t py_baddr[7];
	int py_stride[7];
	unsigned int wdma_baddr;

	u32 vsync_sel;//1:reg_frm_rst 2:vsync 0/3:hold
	int reg_frm_rst;

	int bit_mode;//0:8bit 1:10bit
	int fmt_mode;//0:444 1:422 2:420
	u32 block_mode;
	dma_addr_t rdmif_baddr[3];
	int rdmif_stride[3];

	u32 *core1_ahb_baddr;
	u32 core1_ahb_num;
	u32 *core1b_ahb_baddr;
	u32 core1b_ahb_num;
};

void dump_pyramid_buf(unsigned int idx);
extern u32 dump_pyramid;
extern u32 top1_crc_rd;
#endif
