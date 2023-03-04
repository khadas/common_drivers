/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _AMDV_HW5_H_
#define _AMDV_HW5_H_

/*  driver version */
#define HW5_DRIVER_VER "20230302"

#include <linux/types.h>

struct dolby5_top1_type {
	int core1_hsize;
	int core1_vsize;
	int rdma_num;
	int rdma_size;
	int py_level;
	unsigned int py_baddr[7];
	int py_stride[7];
	unsigned int wdma_baddr;

	int vsync_sel;//1:reg_frm_rst 2:vsync 0/3:hold
	int reg_frm_rst;

	int bit_mode;//0:8bit 1:10bit
	int fmt_mode;//0:444 1:422 2:420
	unsigned int rdmif_baddr[3];
	int rdmif_stride[3];

	u32 *core1_ahb_baddr;
	u32 core1_ahb_num;
	u32 *core1b_ahb_baddr;
	u32 core1b_ahb_num;
};

#endif
