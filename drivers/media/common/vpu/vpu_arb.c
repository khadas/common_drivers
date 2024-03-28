// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/amlogic/media/vpu/vpu.h>
#include "vpu_reg.h"
#include "vpu.h"
#include "vpu_arb.h"

struct vpu_arb_table_s *vpu_rdarb_vpu0_2_level1_tables;
struct vpu_arb_table_s *vpu_rdarb_vpu0_2_level2_tables;
struct vpu_arb_table_s *vpu_rdarb_vpu1_tables;
struct vpu_arb_table_s *vpu_wrarb_vpu0_tables;
struct vpu_arb_table_s *vpu_wrarb_vpu1_tables;
struct vpu_urgent_table_s *vpu_urgent_table_rd_vpu0_2_level2_tables;
struct vpu_urgent_table_s *vpu_urgent_table_rd_vpu1_tables;
struct vpu_urgent_table_s *vpu_urgent_table_wr_vpu0_tables;
struct vpu_urgent_table_s *vpu_urgent_table_wr_vpu1_tables;

/*
 *vpu_read/write module info
 *vpu_read0/read2 has two level,level1 module mux to vpp_arb0 and vpp_arb1
 *READ0_2 ARB
 */
static struct vpu_arb_table_s vpu_rdarb_vpu0_2_level1_t7[] = {
	/* vpu module,        reg,             bit, len, bind_port,        name */
	{VPU_ARB_OSD1,        VPP_RDARB_MODE,  20,  1,   VPU_ARB_VPP_ARB0,  "osd1",
		/*slv_reg,             bit,        len*/
		VPP_RDARB_REQEN_SLV,   0,          2},
	{VPU_ARB_OSD2,        VPP_RDARB_MODE,  21,  1,   VPU_ARB_VPP_ARB1,  "osd2",
		VPP_RDARB_REQEN_SLV,   2,          2},
	{VPU_ARB_VD1,         VPP_RDARB_MODE,  22,  1,   VPU_ARB_VPP_ARB0,  "vd1",
		VPP_RDARB_REQEN_SLV,   4,          2},
	{VPU_ARB_VD2,         VPP_RDARB_MODE,  23,  1,   VPU_ARB_VPP_ARB1,  "vd2",
		VPP_RDARB_REQEN_SLV,   6,          2},
	{VPU_ARB_OSD3,        VPP_RDARB_MODE,  24,  1,   VPU_ARB_VPP_ARB0,  "osd3",
		VPP_RDARB_REQEN_SLV,   8,          2},
	{VPU_ARB_OSD4,        VPP_RDARB_MODE,  25,  1,   VPU_ARB_VPP_ARB1,  "osd4",
		VPP_RDARB_REQEN_SLV,   10,          2},
	{VPU_ARB_AMDOLBY0,    VPP_RDARB_MODE,  26,  1,   VPU_ARB_VPP_ARB0,  "amdolby0",
		VPP_RDARB_REQEN_SLV,   12,          2},
	{VPU_ARB_MALI_AFBCD,  VPP_RDARB_MODE,  27,  1,   VPU_ARB_VPP_ARB1,  "mali_afbc",
		VPP_RDARB_REQEN_SLV,   14,          2},
	{VPU_ARB_VD3,         VPP_RDARB_MODE,  28,  1,   VPU_ARB_VPP_ARB0,  "vd3",
		VPP_RDARB_REQEN_SLV,   16,          2},
		{}
};

static struct vpu_arb_table_s vpu_rdarb_vpu0_2_level1_s7[] = {
	/* vpu module,        reg,             bit, len, bind_port,        name */
	{VPU_ARB_OSD1,        VPP_RDARB_MODE,  20,  1,   VPU_ARB_VPP_ARB0,  "osd1",
		/*slv_reg,             bit,        len*/
		VPP_RDARB_REQEN_SLV,   0,          2},
	{VPU_ARB_OSD2,        VPP_RDARB_MODE,  21,  1,   VPU_ARB_VPP_ARB1,  "osd2",
		VPP_RDARB_REQEN_SLV,   2,          2},
	{VPU_ARB_VD1_RDMIF,   VPP_RDARB_MODE,  22,  1,   VPU_ARB_VPP_ARB0,  "vd1_rdmif",
		VPP_RDARB_REQEN_SLV,   4,          2},
	{VPU_ARB_VD1_AFBCD,   VPP_RDARB_MODE,  23,  1,   VPU_ARB_VPP_ARB1,  "vd1_afbcd",
		VPP_RDARB_REQEN_SLV,   6,          2},
	{VPU_ARB_OSD3,        VPP_RDARB_MODE,  24,  1,   VPU_ARB_VPP_ARB0,  "osd3",
		VPP_RDARB_REQEN_SLV,   8,          2},
	{VPU_ARB_AMDOLBY0,    VPP_RDARB_MODE,  26,  1,   VPU_ARB_VPP_ARB0,  "amdolby0",
		VPP_RDARB_REQEN_SLV,   12,          2},
	{VPU_ARB_MALI_AFBCD,  VPP_RDARB_MODE,  27,  1,   VPU_ARB_VPP_ARB1,  "mali_afbc",
		VPP_RDARB_REQEN_SLV,   14,          2},
		{}
};

static struct vpu_arb_table_s vpu_rdarb_vpu0_2_level2_t7[] = {
	/* vpu module,          reg,                  bit, len, bind_port,  name */
	{VPU_ARB_VPP_ARB0,      VPU_RDARB_MODE_L2C1,  16,  1,   VPU_READ0,  "vpp_arb0",
		/*slv_reg,                 bit,        len*/
		VPP_RDARB_REQEN_SLV_L2C1,   0,          2},
	{VPU_ARB_VPP_ARB1,      VPU_RDARB_MODE_L2C1,  17,  1,   VPU_READ0,  "vpp_arb1",
		VPP_RDARB_REQEN_SLV_L2C1,   2,          2},
	{VPU_ARB_RDMA_READ,     VPU_RDARB_MODE_L2C1,  18,  1,   VPU_READ0,  "rdma_read",
		VPP_RDARB_REQEN_SLV_L2C1,   4,          2},
	{VPU_ARB_LDIM_RD,       VPU_RDARB_MODE_L2C1,  23,  1,   VPU_READ2,  "ldim_rd",
		VPP_RDARB_REQEN_SLV_L2C1,   14,          2},
	{VPU_ARB_VDIN_AFBCE_RD, VPU_RDARB_MODE_L2C1,  24,  1,   VPU_READ0,  "vdin_afbce_rd",
		VPP_RDARB_REQEN_SLV_L2C1,   16,          2},
	{VPU_ARB_VPU_DMA,       VPU_RDARB_MODE_L2C1,  25,  1,   VPU_READ0,  "vpu_dma",
		VPP_RDARB_REQEN_SLV_L2C1,   18,          2},
		{}
};

static struct vpu_arb_table_s vpu_rdarb_vpu0_2_level2_s7[] = {
	/* vpu module,          reg,                  bit, len, bind_port,  name */
	{VPU_ARB_VPP_ARB0,      VPU_RDARB_MODE_L2C1,  16,  1,   VPU_READ0,  "vpp_arb0",
		/*slv_reg,                 bit,        len*/
		VPP_RDARB_REQEN_SLV_L2C1,   0,          2},
	{VPU_ARB_VPP_ARB1,      VPU_RDARB_MODE_L2C1,  17,  1,   VPU_READ0,  "vpp_arb1",
		VPP_RDARB_REQEN_SLV_L2C1,   2,          2},
	{VPU_ARB_RDMA_READ,     VPU_RDARB_MODE_L2C1,  18,  1,   VPU_READ0,  "rdma_read",
		VPP_RDARB_REQEN_SLV_L2C1,   4,          2},
	{VPU_ARB_TCON_P1,       VPU_RDARB_MODE_L2C1,  20,  1,   VPU_READ0,  "tcon_p1",
		VPP_RDARB_REQEN_SLV_L2C1,   8,          2},
	{VPU_ARB_TVFE_READ,     VPU_RDARB_MODE_L2C1,  21,  1,   VPU_READ0,  "tvfe",
		VPP_RDARB_REQEN_SLV_L2C1,   10,          2},
	{VPU_ARB_TCON_P2,       VPU_RDARB_MODE_L2C1,  22,  1,   VPU_READ0,  "tcon_p2",
		VPP_RDARB_REQEN_SLV_L2C1,   12,          2},
		{}
};

/*
 *READ1 ARB
 */
static struct vpu_arb_table_s vpu_rdarb_vpu1_t7[] = {
	/* vpu module,          reg,                    bit, len, bind_port,  name */
	{VPU_ARB_DI_IF1,        DI_RDARB_MODE_L1C1,     16,  1,   VPU_READ1,  "di_if1",
		/*slv_reg,                 bit,        len*/
		DI_RDARB_REQEN_SLV_L1C1,   0,          1},
	{VPU_ARB_DI_MEM,        DI_RDARB_MODE_L1C1,     17,  1,   VPU_READ1,  "di_mem",
		DI_RDARB_REQEN_SLV_L1C1,   1,          1},
	{VPU_ARB_DI_INP,        DI_RDARB_MODE_L1C1,     18,  1,   VPU_READ1,  "di_inp",
		DI_RDARB_REQEN_SLV_L1C1,   2,          1},
	{VPU_ARB_DI_CHAN2,      DI_RDARB_MODE_L1C1,     19,  1,   VPU_READ1,  "di_chan2",
		DI_RDARB_REQEN_SLV_L1C1,   3,          1},
	{VPU_ARB_DI_SUBAXI,     DI_RDARB_MODE_L1C1,     20,  1,   VPU_READ1,  "di_subaxi",
		DI_RDARB_REQEN_SLV_L1C1,   4,          1},
	{VPU_ARB_DI_IF2,        DI_RDARB_MODE_L1C1,     21,  1,   VPU_READ1,  "di_if2",
		DI_RDARB_REQEN_SLV_L1C1,   5,          1},
	{VPU_ARB_DI_IF0,        DI_RDARB_MODE_L1C1,     22,  1,   VPU_READ1,  "di_if0",
		DI_RDARB_REQEN_SLV_L1C1,   6,          1},
	{VPU_ARB_DI_AFBCD,      DI_RDARB_MODE_L1C1,     23,  1,   VPU_READ1,  "di_afbcd",
		DI_RDARB_REQEN_SLV_L1C1,   7,          1},
		{}
};

static struct vpu_arb_table_s vpu_rdarb_vpu1_s7[] = {
	/* vpu module,          reg,                    bit, len, bind_port,  name */
	{VPU_ARB_DI_IF1,        DI_RDARB_MODE_L1C1,     16,  1,   VPU_READ1,  "di_if1",
		/*slv_reg,                 bit,        len*/
		DI_RDARB_REQEN_SLV_L1C1,   0,          1},
	{VPU_ARB_DI_MEM,        DI_RDARB_MODE_L1C1,     17,  1,   VPU_READ1,  "di_mem",
		DI_RDARB_REQEN_SLV_L1C1,   1,          1},
	{VPU_ARB_DI_INP,        DI_RDARB_MODE_L1C1,     18,  1,   VPU_READ1,  "di_inp",
		DI_RDARB_REQEN_SLV_L1C1,   2,          1},
	{VPU_ARB_DI_CHAN2,      DI_RDARB_MODE_L1C1,     19,  1,   VPU_READ1,  "di_chan2",
		DI_RDARB_REQEN_SLV_L1C1,   3,          1},
	{VPU_ARB_DI_SUBAXI,     DI_RDARB_MODE_L1C1,     20,  1,   VPU_READ1,  "di_subaxi",
		DI_RDARB_REQEN_SLV_L1C1,   4,          1},
	{VPU_ARB_DI_IF2,        DI_RDARB_MODE_L1C1,     21,  1,   VPU_READ1,  "di_if2",
		DI_RDARB_REQEN_SLV_L1C1,   5,          1},
	{VPU_ARB_DI_IF0,        DI_RDARB_MODE_L1C1,     22,  1,   VPU_READ1,  "di_if0",
		DI_RDARB_REQEN_SLV_L1C1,   6,          1},
		{}
};

/*
 *WRITE0 ARB
 */
static struct vpu_arb_table_s vpu_wrarb_vpu0_t7[] = {
	/* vpu module,          reg,                    bit, len, bind_port,  name */
	{VPU_ARB_VDIN_WR,       VPU_WRARB_MODE_L2C1,    16,  1,   NO_PORT,    "vdin_wr",
		/*slv_reg,                 bit,        len*/
		VPU_WRARB_REQEN_SLV_L2C1,   0,          1},
	{VPU_ARB_RDMA_WR,       VPU_WRARB_MODE_L2C1,    17,  1,   VPU_WRITE0, "rdma_wr",
		VPU_WRARB_REQEN_SLV_L2C1,   1,          1},
	{VPU_ARB_TVFE_WR,       VPU_WRARB_MODE_L2C1,    18,  1,   NO_PORT,    "tvfe_wr",
		VPU_WRARB_REQEN_SLV_L2C1,   2,          1},
	{VPU_ARB_TCON1_WR,      VPU_WRARB_MODE_L2C1,    19,  1,   NO_PORT,    "tcon1_wr",
		VPU_WRARB_REQEN_SLV_L2C1,   3,          1},
	{VPU_ARB_TCON2_WR,      VPU_WRARB_MODE_L2C1,    20,  1,   NO_PORT,    "tcon2_wr",
		VPU_WRARB_REQEN_SLV_L2C1,   4,          1},
	{VPU_ARB_TCON3_WR,      VPU_WRARB_MODE_L2C1,    21,  1,   NO_PORT,    "tcon3_wr",
		VPU_WRARB_REQEN_SLV_L2C1,   5,          1},
	{VPU_ARB_VPU_DMA_WR,    VPU_WRARB_MODE_L2C1,    22,  1,   NO_PORT,    "vpu_dma_wr",
		VPU_WRARB_REQEN_SLV_L2C1,   6,          1},
		{}
};

static struct vpu_arb_table_s vpu_wrarb_vpu0_s7[] = {
	/* vpu module,          reg,                    bit, len, bind_port,  name */
	{VPU_ARB_VDIN_WR,       VPU_WRARB_MODE_L2C1,    16,  1,   NO_PORT,    "vdin_wr",
		/*slv_reg,                 bit,        len*/
		VPU_WRARB_REQEN_SLV_L2C1,   0,          1},
	{VPU_ARB_RDMA_WR,       VPU_WRARB_MODE_L2C1,    17,  1,   VPU_WRITE0, "rdma_wr",
		VPU_WRARB_REQEN_SLV_L2C1,   1,          1},
	{VPU_ARB_TVFE_WR,       VPU_WRARB_MODE_L2C1,    18,  1,   NO_PORT,    "tvfe_wr",
		VPU_WRARB_REQEN_SLV_L2C1,   2,          1},
	{VPU_ARB_TCON1_WR,      VPU_WRARB_MODE_L2C1,    19,  1,   NO_PORT,    "tcon1_wr",
		VPU_WRARB_REQEN_SLV_L2C1,   3,          1},
	/*
	 *{VPU_ARB_DI_AXI1_WR,	VPU_WRARB_MODE_L2C1,	20,  1,  NO_PORT,	  "tcon1_wr",
	 *		VPU_WRARB_REQEN_SLV_L2C1,	4,			1},
	 */
		{}
};

/*
 *WRITE1 ARB
 */
static struct vpu_arb_table_s vpu_wrarb_vpu1_t7[] = {
	/* vpu module,          reg,                    bit, len, bind_port,  name */
	{VPU_ARB_NR_WR,         DI_WRARB_MODE_L1C1,     16,  1,   VPU_WRITE1,    "nr_wr",
			/*slv_reg,             bit,        len*/
		DI_WRARB_REQEN_SLV_L1C1,   0,          1},
	{VPU_ARB_DI_WR,         DI_WRARB_MODE_L1C1,     17,  1,   VPU_WRITE1,    "di_wr",
		DI_WRARB_REQEN_SLV_L1C1,   1,          1},
	{VPU_ARB_DI_SUBAXI_WR,  DI_WRARB_MODE_L1C1,     18,  1,   VPU_WRITE1,    "di_subaxi_wr",
		DI_WRARB_REQEN_SLV_L1C1,   2,          1},
	{VPU_ARB_DI_AFBCE0,     DI_WRARB_MODE_L1C1,     19,  1,   VPU_WRITE1,    "di_afbce0",
		DI_WRARB_REQEN_SLV_L1C1,   3,          1},
	{VPU_ARB_DI_AFBCE1,     DI_WRARB_MODE_L1C1,     20,  1,   VPU_WRITE1,    "di_afbce1",
		DI_WRARB_REQEN_SLV_L1C1,   4,          1},
		{}
};

static struct vpu_arb_table_s vpu_wrarb_vpu1_s7[] = {
	/* vpu module,          reg,                    bit, len, bind_port,  name */
	{VPU_ARB_NR_WR,         DI_WRARB_MODE_L1C1,     16,  1,   VPU_WRITE1,    "nr_wr",
			/*slv_reg,             bit,        len*/
		DI_WRARB_REQEN_SLV_L1C1,   0,          1},
	{VPU_ARB_DI_WR,         DI_WRARB_MODE_L1C1,     17,  1,   VPU_WRITE1,    "di_wr",
		DI_WRARB_REQEN_SLV_L1C1,   1,          1},
	{VPU_ARB_DI_SUBAXI_WR,  DI_WRARB_MODE_L1C1,     18,  1,   VPU_WRITE1,    "di_subaxi_wr",
		DI_WRARB_REQEN_SLV_L1C1,   2,          1},
		{}
};

/*
 *vpu_read/write urgent info
 *vpu_read0/read2 urgent by setting level2 module
 *READ0_2 URGENT
 */
static struct vpu_urgent_table_s vpu_urgent_table_rd_vpu0_2_level2_t7[] = {
	/*vpu module,           reg,                port,     val, start, len, name*/
	{VPU_ARB_VPP_ARB0,      VPU_RDARB_UGT_L2C1, VPU_READ0, 3,   0,    2,  "vpp_arb0"},
	{VPU_ARB_VPP_ARB1,      VPU_RDARB_UGT_L2C1, VPU_READ0, 3,   2,    2,  "vpp_arb1"},
	{VPU_ARB_RDMA_READ,     VPU_RDARB_UGT_L2C1, VPU_READ0, 3,   4,    2,  "rdma_read"},
	{VPU_ARB_VIU2,          VPU_RDARB_UGT_L2C1, VPU_READ0, 3,   6,    2,  "viu2(no use)"},
	{VPU_ARB_TCON_P1,       VPU_RDARB_UGT_L2C1, VPU_READ0, 3,   8,    2,  "tcon1(no use)"},
	{VPU_ARB_TVFE_READ,     VPU_RDARB_UGT_L2C1, VPU_READ0, 3,   10,   2,  "tvfe_read(no use)"},
	{VPU_ARB_TCON_P2,       VPU_RDARB_UGT_L2C1, VPU_READ0, 3,   12,   2,  "tcon_p2(no use)"},
	{VPU_ARB_LDIM_RD,       VPU_RDARB_UGT_L2C1, VPU_READ2, 3,   14,   2,  "ldim_rd"},
	{VPU_ARB_VDIN_AFBCE_RD, VPU_RDARB_UGT_L2C1, VPU_READ0, 0,   16,   2,  "vdin_afbce_rd"},
	{VPU_ARB_VPU_DMA,       VPU_RDARB_UGT_L2C1, VPU_READ0, 0,   18,   2,  "vpu_dma"},
		{}
};

static struct vpu_urgent_table_s vpu_urgent_table_rd_vpu0_2_level2_s7[] = {
	/*vpu module,           reg,                port,     val, start, len, name*/
	{VPU_ARB_VPP_ARB0,      VPU_RDARB_UGT_L2C1, VPU_READ0, 3,   0,    2,  "vpp_arb0"},
	{VPU_ARB_VPP_ARB1,      VPU_RDARB_UGT_L2C1, VPU_READ0, 3,   2,    2,  "vpp_arb1"},
	{VPU_ARB_RDMA_READ,     VPU_RDARB_UGT_L2C1, VPU_READ0, 3,   4,    2,  "rdma_read"},
	{VPU_ARB_VIU2,          VPU_RDARB_UGT_L2C1, VPU_READ0, 3,   6,    2,  "viu2(no use)"},
	{VPU_ARB_TCON_P1,       VPU_RDARB_UGT_L2C1, VPU_READ0, 3,   8,    2,  "tcon1"},
	{VPU_ARB_TVFE_READ,     VPU_RDARB_UGT_L2C1, VPU_READ0, 3,   10,   2,  "tvfe_read"},
	{VPU_ARB_TCON_P2,       VPU_RDARB_UGT_L2C1, VPU_READ0, 3,   12,   2,  "tcon_p2"},
	{VPU_ARB_LDIM_RD,       VPU_RDARB_UGT_L2C1, VPU_READ2, 3,   14,   2,  "ldim_rd(no use)"},
	{VPU_ARB_VDIN_AFBCE_RD, VPU_RDARB_UGT_L2C1, VPU_READ0, 0,   16,   2,  "vdin_afbce(no use)"},
	{VPU_ARB_VPU_DMA,       VPU_RDARB_UGT_L2C1, VPU_READ0, 0,   18,   2,  "vpu_dma(no use)"},
		{}
};

/*
 *READ1 URGENT
 */
static struct vpu_urgent_table_s vpu_urgent_table_rd_vpu1_t7[] = {
	/*vpu module,           reg,                 port,       val, start, len,  name*/
	{VPU_ARB_DI_IF1,        DI_RDARB_UGT_L1C1,   VPU_READ1,  1,   0,     2,    "di_if1"},
	{VPU_ARB_DI_MEM,        DI_RDARB_UGT_L1C1,   VPU_READ1,  1,   2,     2,    "di_mem"},
	{VPU_ARB_DI_INP,        DI_RDARB_UGT_L1C1,   VPU_READ1,  1,   4,     2,    "di_inp"},
	{VPU_ARB_DI_CHAN2,      DI_RDARB_UGT_L1C1,   VPU_READ1,  1,   6,     2,    "di_chan2"},
	{VPU_ARB_DI_SUBAXI,     DI_RDARB_UGT_L1C1,   VPU_READ1,  1,   8,     2,    "di_subaxi"},
	{VPU_ARB_DI_IF2,        DI_RDARB_UGT_L1C1,   VPU_READ1,  1,   10,    2,    "di_if2"},
	{VPU_ARB_DI_IF0,        DI_RDARB_UGT_L1C1,   VPU_READ1,  1,   12,    2,    "di_if0"},
	{VPU_ARB_DI_AFBCD,      DI_RDARB_UGT_L1C1,   VPU_READ1,  1,   14,    2,    "di_afbcd"},
		{}
};

static struct vpu_urgent_table_s vpu_urgent_table_rd_vpu1_s7[] = {
	/*vpu module,           reg,                 port,       val, start, len,  name*/
	{VPU_ARB_DI_IF1,        DI_RDARB_UGT_L1C1,   VPU_READ1,  1,   0,     2,    "di_if1"},
	{VPU_ARB_DI_MEM,        DI_RDARB_UGT_L1C1,   VPU_READ1,  1,   2,     2,    "di_mem"},
	{VPU_ARB_DI_INP,        DI_RDARB_UGT_L1C1,   VPU_READ1,  1,   4,     2,    "di_inp"},
	{VPU_ARB_DI_CHAN2,      DI_RDARB_UGT_L1C1,   VPU_READ1,  1,   6,     2,    "di_chan2"},
	{VPU_ARB_DI_SUBAXI,     DI_RDARB_UGT_L1C1,   VPU_READ1,  1,   8,     2,    "di_subaxi"},
	{VPU_ARB_DI_IF2,        DI_RDARB_UGT_L1C1,   VPU_READ1,  1,   10,    2,    "di_if2"},
	{VPU_ARB_DI_IF0,        DI_RDARB_UGT_L1C1,   VPU_READ1,  1,   12,    2,    "di_if0"},
		{}
};

/*
 *WRITE0 URGENT
 */
static struct vpu_urgent_table_s vpu_urgent_table_wr_vpu0_t7[] = {
	/*vpu module,           reg,                 port,       val, start, len,  name*/
	{VPU_ARB_VDIN_WR,       VPU_WRARB_UGT_L2C1,  VPU_WRITE0,  0,   0,    2,    "vdin_wr"},
	{VPU_ARB_RDMA_WR,       VPU_WRARB_UGT_L2C1,  VPU_WRITE0,  0,   2,    2,    "rdma_wr"},
	{VPU_ARB_TVFE_WR,       VPU_WRARB_UGT_L2C1,  VPU_WRITE0,  0,   4,    2,    "tvfe_wr"},
	{VPU_ARB_TCON1_WR,      VPU_WRARB_UGT_L2C1,  VPU_WRITE0,  0,   6,    2,    "tcon1_wr"},
	{VPU_ARB_TCON2_WR,      VPU_WRARB_UGT_L2C1,  VPU_WRITE0,  0,   8,    2,    "tcon2_wr"},
	{VPU_ARB_TCON3_WR,      VPU_WRARB_UGT_L2C1,  VPU_WRITE0,  0,   10,   2,    "tcon3_wr"},
	{VPU_ARB_VPU_DMA_WR,    VPU_WRARB_UGT_L2C1,  VPU_WRITE0,  0,   12,   2,    "vpu_dma_wr"},
		{}
};

static struct vpu_urgent_table_s vpu_urgent_table_wr_vpu0_s7[] = {
	/*vpu module,           reg,                 port,       val, start, len,  name*/
	{VPU_ARB_VDIN_WR,       VPU_WRARB_UGT_L2C1,  VPU_WRITE0,  0,   0,    2,    "vdin_wr"},
	{VPU_ARB_RDMA_WR,       VPU_WRARB_UGT_L2C1,  VPU_WRITE0,  0,   2,    2,    "rdma_wr"},
	{VPU_ARB_TVFE_WR,       VPU_WRARB_UGT_L2C1,  VPU_WRITE0,  0,   4,    2,    "tvfe_wr"},
	{VPU_ARB_TCON1_WR,      VPU_WRARB_UGT_L2C1,  VPU_WRITE0,  0,   6,    2,    "tcon1_wr"},
	{VPU_ARB_DI_AXI1_WR,    VPU_WRARB_UGT_L2C1,  VPU_WRITE0,  0,   8,    2,    "di_axi1_wr"},
		{}
};

/*
 *WRITE1 URGENT
 */
static struct vpu_urgent_table_s vpu_urgent_table_wr_vpu1_t7[] = {
	/*vpu module,           reg,                 port,       val, start, len,  name*/
	{VPU_ARB_NR_WR,         DI_WRARB_UGT_L1C1,  VPU_WRITE0,  1,   0,    2,    "nr_wr"},
	{VPU_ARB_DI_WR,         DI_WRARB_UGT_L1C1,  VPU_WRITE0,  1,   2,    2,    "di_wr"},
	{VPU_ARB_DI_SUBAXI_WR,  DI_WRARB_UGT_L1C1,  VPU_WRITE0,  1,   4,    2,    "di_subaxi_wr"},
	{VPU_ARB_DI_AFBCE0,     DI_WRARB_UGT_L1C1,  VPU_WRITE0,  1,   6,    2,    "di_afbce0"},
	{VPU_ARB_DI_AFBCE1,     DI_WRARB_UGT_L1C1,  VPU_WRITE0,  1,   8,    2,    "di_afbce1"},
		{}
};

static struct vpu_urgent_table_s vpu_urgent_table_wr_vpu1_s7[] = {
	/*vpu module,           reg,                 port,    val,start,len,  name*/
	{VPU_ARB_NR_WR,         DI_WRARB_UGT_L1C1,  VPU_WRITE0, 1, 0,    2,  "nr_wr"},
	{VPU_ARB_DI_WR,         DI_WRARB_UGT_L1C1,  VPU_WRITE0, 1, 2,    2,  "di_wr"},
	{VPU_ARB_DI_SUBAXI_WR,  DI_WRARB_UGT_L1C1,  VPU_WRITE0, 1, 4,    2,  "di_subaxi_wr"},
	{VPU_ARB_DI_AFBCE0,     DI_WRARB_UGT_L1C1,  VPU_WRITE0, 1, 6,    2,  "di_afbce0(no use)"},
	{VPU_ARB_DI_AFBCE1,     DI_WRARB_UGT_L1C1,  VPU_WRITE0, 1, 8,    2,  "di_afbce1(no use)"},
		{}
};

static struct vpu_arb_info vpu_arb_ins[ARB_MODULE_MAX];

//static struct vpu_urgent_table_s vpu_urgent_table_rd_vpu0_2_level2_s1a[] = {
//{VPU_VPP_ARB0,		VPU_RDARB_UGT_L2C1, VPU_READ0, 3,	0, 2,	"vpp_arb0"},
//{VPU_VPP_ARB1,		VPU_RDARB_UGT_L2C1, VPU_READ0, 3,	2, 2,	"vpp_arb1"},
//{}
//};

int vpu_rdarb0_2_bind_l1(enum vpu_arb_mod_e level1_module, enum vpu_arb_mod_e level2_module)
{
	u32 val;
	struct vpu_arb_table_s *vpu0_2_rdarb_level1_module = vpu_rdarb_vpu0_2_level1_tables;

	if (!vpu0_2_rdarb_level1_module ||
		level1_module > VPU_ARB_DI_IF1 ||
		level2_module > VPU_ARB_DI_IF1)
		return -1;

	while (vpu0_2_rdarb_level1_module->vmod) {
		if (level1_module == vpu0_2_rdarb_level1_module->vmod)
			break;
		vpu0_2_rdarb_level1_module++;
	}
	if (!vpu0_2_rdarb_level1_module->vmod) {
		VPUERR("level1_table no such module\n");
		return -1;
	}
	switch (level2_module) {
	case VPU_ARB_VPP_ARB0:
		vpu_vcbus_setb(vpu0_2_rdarb_level1_module->reg, 0,
				vpu0_2_rdarb_level1_module->bit,
				vpu0_2_rdarb_level1_module->len);
		vpu0_2_rdarb_level1_module->bind_port = VPU_ARB_VPP_ARB0;
		vpu_pr_debug(MODULE_ARB_MODE, "%s bind to vpp_arb0\n",
				vpu0_2_rdarb_level1_module->name);
		break;
	case VPU_ARB_VPP_ARB1:
		vpu_vcbus_setb(vpu0_2_rdarb_level1_module->reg, 1,
				vpu0_2_rdarb_level1_module->bit,
				vpu0_2_rdarb_level1_module->len);
		vpu0_2_rdarb_level1_module->bind_port = VPU_ARB_VPP_ARB1;
		vpu_pr_debug(MODULE_ARB_MODE, "%s bind to vpp_arb1\n",
				vpu0_2_rdarb_level1_module->name);
		break;
	default:
		break;
	}
	if (vpu0_2_rdarb_level1_module->reqen_slv_len == 1)
		vpu_vcbus_setb(vpu0_2_rdarb_level1_module->reqen_slv_reg, 1,
			vpu0_2_rdarb_level1_module->reqen_slv_bit,
			vpu0_2_rdarb_level1_module->reqen_slv_len);
	else if (vpu0_2_rdarb_level1_module->reqen_slv_len == 2)
		vpu_vcbus_setb(vpu0_2_rdarb_level1_module->reqen_slv_reg, 3,
			vpu0_2_rdarb_level1_module->reqen_slv_bit,
			vpu0_2_rdarb_level1_module->reqen_slv_len);
	val = vpu_vcbus_read(vpu0_2_rdarb_level1_module->reg);
	vpu_pr_debug(MODULE_ARB_MODE, "VPU_RDARB_MOD[0x%x] = 0x%x\n",
		vpu0_2_rdarb_level1_module->reg, val);
	return 0;
}

int vpu_rdarb0_2_bind_l2(enum vpu_arb_mod_e level2_module, u32 vpu_read_port)
{
	u32 val;
	struct vpu_arb_table_s *vpu0_2_rdarb_level2_module = vpu_rdarb_vpu0_2_level2_tables;

	if (!vpu0_2_rdarb_level2_module ||
		level2_module > VPU_ARB_DI_IF1 ||
		vpu_read_port > VPU_WRITE1)
		return -1;
	while (vpu0_2_rdarb_level2_module->vmod) {
		if (level2_module == vpu0_2_rdarb_level2_module->vmod)
			break;
		vpu0_2_rdarb_level2_module++;
	}
	if (!vpu0_2_rdarb_level2_module->vmod) {
		VPUERR("level2_table no such module\n");
		return -1;
	}
	switch (vpu_read_port) {
	case VPU_READ0:
		vpu_vcbus_setb(vpu0_2_rdarb_level2_module->reg, 0,
				vpu0_2_rdarb_level2_module->bit,
				vpu0_2_rdarb_level2_module->len);
		vpu0_2_rdarb_level2_module->bind_port = VPU_READ0;
		vpu_pr_debug(MODULE_ARB_MODE, "%s bind to vpu_read0\n",
				vpu0_2_rdarb_level2_module->name);
		break;
	case VPU_READ2:
		vpu_vcbus_setb(vpu0_2_rdarb_level2_module->reg, 1,
				vpu0_2_rdarb_level2_module->bit,
				vpu0_2_rdarb_level2_module->len);
		vpu0_2_rdarb_level2_module->bind_port = VPU_READ2;
		vpu_pr_debug(MODULE_ARB_MODE, "%s bind to vpu_read2\n",
				vpu0_2_rdarb_level2_module->name);
		break;
	default:
		break;
	}
	if (vpu0_2_rdarb_level2_module->reqen_slv_len == 1)
		vpu_vcbus_setb(vpu0_2_rdarb_level2_module->reqen_slv_reg, 1,
			vpu0_2_rdarb_level2_module->reqen_slv_bit,
			vpu0_2_rdarb_level2_module->reqen_slv_len);
	else if (vpu0_2_rdarb_level2_module->reqen_slv_len == 2)
		vpu_vcbus_setb(vpu0_2_rdarb_level2_module->reqen_slv_reg, 3,
			vpu0_2_rdarb_level2_module->reqen_slv_bit,
			vpu0_2_rdarb_level2_module->reqen_slv_len);
	val = vpu_vcbus_read(vpu0_2_rdarb_level2_module->reg);
	vpu_pr_debug(MODULE_ARB_MODE, "VPU_RDARB_MOD[0x%x] = 0x%x\n",
		vpu0_2_rdarb_level2_module->reg, val);
	return 0;
}

int vpu_rdarb1_bind(enum vpu_arb_mod_e rdarb1_module, u32 vpu_read_port)
{
	struct vpu_arb_table_s *vpu1_rdarb_module = vpu_rdarb_vpu1_tables;

	if (!vpu1_rdarb_module ||
		rdarb1_module < VPU_ARB_DI_IF1 ||
		rdarb1_module > VPU_ARB_DI_AFBCD ||
		vpu_read_port != VPU_READ1)
		return -1;
	while (vpu1_rdarb_module->vmod) {
		if (rdarb1_module == vpu1_rdarb_module->vmod)
			break;
		vpu1_rdarb_module++;
	}
	if (!vpu1_rdarb_module->vmod) {
		VPUERR("rdarb1_table no such module\n");
		return -1;
	}
	if (vpu1_rdarb_module->reqen_slv_len == 1)
		vpu_vcbus_setb(vpu1_rdarb_module->reqen_slv_reg, 1,
			vpu1_rdarb_module->reqen_slv_bit,
			vpu1_rdarb_module->reqen_slv_len);
	else if (vpu1_rdarb_module->reqen_slv_len == 2)
		vpu_vcbus_setb(vpu1_rdarb_module->reqen_slv_reg, 3,
			vpu1_rdarb_module->reqen_slv_bit,
			vpu1_rdarb_module->reqen_slv_len);
	/*DI only 1r1w so set to 0*/
	vpu_vcbus_setb(vpu1_rdarb_module->reg, 0,
				vpu1_rdarb_module->bit,
				vpu1_rdarb_module->len);
	vpu1_rdarb_module->bind_port = VPU_READ1;
	vpu_pr_debug(MODULE_ARB_MODE, "%s bind to vpu_read1\n",
		vpu1_rdarb_module->name);
	return 0;
}

int vpu_wrarb0_bind(enum vpu_arb_mod_e wrarb0_module, u32 vpu_write_port)
{
	struct vpu_arb_table_s *vpu0_wrarb_module = vpu_wrarb_vpu0_tables;

	if (!vpu0_wrarb_module ||
		wrarb0_module < VPU_ARB_VDIN_WR ||
		wrarb0_module > VPU_ARB_VPU_DMA_WR ||
		vpu_write_port != VPU_WRITE0)
		return -1;
	while (vpu0_wrarb_module->vmod) {
		if (wrarb0_module == vpu0_wrarb_module->vmod)
			break;
		vpu0_wrarb_module++;
	}
	if (!vpu0_wrarb_module->vmod) {
		VPUERR("rdarb1_table no such module\n");
		return -1;
	}
	if (vpu0_wrarb_module->reqen_slv_len == 1)
		vpu_vcbus_setb(vpu0_wrarb_module->reqen_slv_reg, 1,
			vpu0_wrarb_module->reqen_slv_bit,
			vpu0_wrarb_module->reqen_slv_len);
	else if (vpu0_wrarb_module->reqen_slv_len == 2)
		vpu_vcbus_setb(vpu0_wrarb_module->reqen_slv_reg, 3,
			vpu0_wrarb_module->reqen_slv_bit,
			vpu0_wrarb_module->reqen_slv_len);
	vpu_vcbus_setb(vpu0_wrarb_module->reg, 0,
				vpu0_wrarb_module->bit,
				vpu0_wrarb_module->len);
	vpu0_wrarb_module->bind_port = VPU_WRITE0;
	vpu_pr_debug(MODULE_ARB_MODE, "%s bind to vpu_write0\n",
		vpu0_wrarb_module->name);
	return 0;
}

int vpu_wrarb1_bind(enum vpu_arb_mod_e wrarb1_module, u32 vpu_write_port)
{
	struct vpu_arb_table_s *vpu1_wrarb_module = vpu_wrarb_vpu1_tables;

	if (!vpu1_wrarb_module ||
		wrarb1_module < VPU_ARB_VDIN_WR ||
		wrarb1_module > VPU_ARB_VPU_DMA_WR ||
		vpu_write_port != VPU_WRITE1)
		return -1;
	while (vpu1_wrarb_module->vmod) {
		if (wrarb1_module == vpu1_wrarb_module->vmod)
			break;
		vpu1_wrarb_module++;
	}
	if (!vpu1_wrarb_module->vmod) {
		VPUERR("rdarb1_table no such module\n");
		return -1;
	}
	if (vpu1_wrarb_module->reqen_slv_len == 1)
		vpu_vcbus_setb(vpu1_wrarb_module->reqen_slv_reg, 1,
			vpu1_wrarb_module->reqen_slv_bit, vpu1_wrarb_module->reqen_slv_len);
	else if (vpu1_wrarb_module->reqen_slv_len == 2)
		vpu_vcbus_setb(vpu1_wrarb_module->reqen_slv_reg, 3,
			vpu1_wrarb_module->reqen_slv_bit, vpu1_wrarb_module->reqen_slv_len);
	vpu_vcbus_setb(vpu1_wrarb_module->reg, 0,
				vpu1_wrarb_module->bit,
				vpu1_wrarb_module->len);
	vpu1_wrarb_module->bind_port = VPU_WRITE1;
	vpu_pr_debug(MODULE_ARB_MODE, "%s bind to vpu_write1\n",
		vpu1_wrarb_module->name);
	return 0;
}

void get_rdarb0_2_module_info(void)
{
	struct vpu_arb_table_s *vpu0_2_rdarb_level1_tables = vpu_rdarb_vpu0_2_level1_tables;
	struct vpu_arb_table_s *vpu0_2_rdarb_level2_tables = vpu_rdarb_vpu0_2_level2_tables;

	if (!vpu0_2_rdarb_level1_tables ||
		!vpu0_2_rdarb_level2_tables)
		return;
	vpu_pr_debug(MODULE_ARB_MODE, "rdarb0_2 level1 module:\n");
	while (vpu0_2_rdarb_level1_tables->vmod) {
		vpu_pr_debug(MODULE_ARB_MODE, "module_name:%-15s module_index:%d (bind_reg:[0x%x],bit%d)\n",
			vpu0_2_rdarb_level1_tables->name,
			vpu0_2_rdarb_level1_tables->vmod,
			vpu0_2_rdarb_level1_tables->reg,
			vpu0_2_rdarb_level1_tables->bit);
		vpu0_2_rdarb_level1_tables++;
	}
	vpu0_2_rdarb_level1_tables = vpu_rdarb_vpu0_2_level1_tables;
	vpu_pr_debug(MODULE_ARB_MODE, "rdarb0_2 level2 module:\n");
	while (vpu0_2_rdarb_level2_tables->vmod) {
		vpu_pr_debug(MODULE_ARB_MODE, "module_name:%-15s module_index:%d (bind_reg[0x%x],bit%d)\n",
			vpu0_2_rdarb_level2_tables->name,
			vpu0_2_rdarb_level2_tables->vmod,
			vpu0_2_rdarb_level2_tables->reg,
			vpu0_2_rdarb_level2_tables->bit);
		vpu0_2_rdarb_level2_tables++;
	}
	vpu0_2_rdarb_level2_tables = vpu_rdarb_vpu0_2_level2_tables;
	vpu_pr_debug(MODULE_ARB_MODE, "vpu port_name:VPU_READ0     port_index:%d\n", VPU_READ0);
	vpu_pr_debug(MODULE_ARB_MODE, "vpu port_name:VPU_READ2     port_index:%d\n", VPU_READ2);
}

void get_rdarb1_module_info(void)
{
	struct vpu_arb_table_s *vpu1_rdarb_tables = vpu_rdarb_vpu1_tables;

	if (!vpu1_rdarb_tables)
		return;
	vpu_pr_debug(MODULE_ARB_MODE, "rdarb1 module:\n");
	while (vpu1_rdarb_tables->vmod) {
		vpu_pr_debug(MODULE_ARB_MODE, "module_name:%-15s module_index:%d (bind_reg:[0x%x],bit%d)\n",
			vpu1_rdarb_tables->name,
			vpu1_rdarb_tables->vmod,
			vpu1_rdarb_tables->reg,
			vpu1_rdarb_tables->bit);
		vpu1_rdarb_tables++;
	}
	vpu1_rdarb_tables = vpu_rdarb_vpu1_tables;
	vpu_pr_debug(MODULE_ARB_MODE, "vpu port_name:VPU_READ1     port_index:%d\n", VPU_READ1);
}

void get_wrarb0_module_info(void)
{
	struct vpu_arb_table_s *vpu0_wrarb_tables = vpu_wrarb_vpu0_tables;

	if (!vpu0_wrarb_tables)
		return;
	vpu_pr_debug(MODULE_ARB_MODE, "wrarb0 module:\n");
	while (vpu0_wrarb_tables->vmod) {
		vpu_pr_debug(MODULE_ARB_MODE, "module_name:%-15s module_index:%d (bind_reg:[0x%x],bit%d)\n",
			vpu0_wrarb_tables->name,
			vpu0_wrarb_tables->vmod,
			vpu0_wrarb_tables->reg,
			vpu0_wrarb_tables->bit);
		vpu0_wrarb_tables++;
	}
	vpu0_wrarb_tables = vpu_wrarb_vpu0_tables;
	vpu_pr_debug(MODULE_ARB_MODE, "vpu port_name:VPU_WRITE0    port_index:%d\n", VPU_WRITE0);
}

void get_wrarb1_module_info(void)
{
	struct vpu_arb_table_s *vpu_wrarb1_tables = vpu_wrarb_vpu1_tables;

	if (!vpu_wrarb1_tables)
		return;
	vpu_pr_debug(MODULE_ARB_MODE, "wrarb1 module\n");
	while (vpu_wrarb1_tables->vmod) {
		vpu_pr_debug(MODULE_ARB_MODE, "module_name:%-15s module_index:%d (bind_reg:[0x%x],bit%d)\n",
			vpu_wrarb1_tables->name,
			vpu_wrarb1_tables->vmod,
			vpu_wrarb1_tables->reg,
			vpu_wrarb1_tables->bit);
		vpu_wrarb1_tables++;
	}
	vpu_wrarb1_tables = vpu_wrarb_vpu1_tables;
	vpu_pr_debug(MODULE_ARB_MODE, "vpu port_name:VPU_WRITE1    port_index:%d\n", VPU_WRITE1);
}

struct vpu_qos_table_s {
	unsigned int vmod;
	unsigned int reg;
	unsigned int urgent;
	unsigned int val;
	unsigned int start_bit;
	unsigned int len;
	char *name;
};

//struct vpu_qos_table_s vpu_qos_table[] = {
//	/*vpu module,    reg,                urgent, val,  start, len,  name*/
//	{VPU_READ0,      VPU_RDARB_UGT_L2C1,  0,     0x3,  0,     4,    "vpu_read0"},
//	{VPU_READ1,      DI_RDARB_UGT_L1C1,   1,     0x7,  20,    4,    "vpu_read1"},
//	{VPU_READ2,      VPU_RDARB_UGT_L2C1,  2,     0xb,  8,     4,    "vpu_read2"},
//	{VPU_WRITE0,     DI_RDARB_UGT_L1C1,   3,     0xf,  12,    4,    "vpu_write0"},
//	{VPU_WRITE1,     DI_RDARB_UGT_L1C1,   3,     0xf,  28,    4,    "vpu_write1"},
//		{}
	//VPU_AXI_QOS_RD1,
	//VPU_AXI_QOS_WR0
//};

//static int vpu_qos_table_set(u32 vpu_port, u32 urgent_level, u32 urgent_value);
//static int vpu_qos_table_get(u32 vpu_port, u32 urgent_level, u32 *urgent_value);

struct vpu_urgent_ctrl_s {
	unsigned int vpu_port;
	unsigned int vmod;
	unsigned int urgent;
};

int vpu_rdarb0_2_urgent_set(enum vpu_arb_mod_e vmod, u32 urgent_value)
{
	u32 val;
	struct vpu_urgent_table_s *rd_vpu0_2_level2_urgent_tables =
					vpu_urgent_table_rd_vpu0_2_level2_tables;
	struct vpu_urgent_table_s *set_urgent_module = NULL;

	if (!rd_vpu0_2_level2_urgent_tables) {
		VPUERR("read0_2 level2 urgent tables is error");
		return -1;
	}
	while (rd_vpu0_2_level2_urgent_tables->vmod) {
		if (rd_vpu0_2_level2_urgent_tables->vmod == vmod)
			set_urgent_module = rd_vpu0_2_level2_urgent_tables;
		rd_vpu0_2_level2_urgent_tables++;
	}
	if (!set_urgent_module) {
		VPUERR("rdarb0_2 no such module\n");
		return -1;
	}
	rd_vpu0_2_level2_urgent_tables = vpu_urgent_table_rd_vpu0_2_level2_tables;
	set_urgent_module->val = urgent_value;
	vpu_vcbus_setb(set_urgent_module->reg, urgent_value,
			set_urgent_module->start_bit, set_urgent_module->len);
	vpu_pr_debug(MODULE_ARB_URGENT, "%s urgent set to %d\n",
			set_urgent_module->name, urgent_value);
	val = vpu_vcbus_read(set_urgent_module->reg);
	vpu_pr_debug(MODULE_ARB_URGENT, "VPU_RDARB_UGT_L2C1[0x%x] = 0x%x\n",
			set_urgent_module->reg, val);
	return 0;
}

int vpu_rdarb1_urgent_set(enum vpu_arb_mod_e vmod, u32 urgent_value)
{
	u32 val;
	struct vpu_urgent_table_s *rd_vpu1_urgent_tables =
					vpu_urgent_table_rd_vpu1_tables;
	struct vpu_urgent_table_s *set_urgent_module = NULL;

	if (!rd_vpu1_urgent_tables) {
		VPUERR("read1 urgent tables is error");
		return -1;
	}
	while (rd_vpu1_urgent_tables->vmod) {
		if (rd_vpu1_urgent_tables->vmod == vmod)
			set_urgent_module = rd_vpu1_urgent_tables;
		rd_vpu1_urgent_tables++;
	}
	if (!set_urgent_module) {
		VPUERR("rdarb1 no such module\n");
		return -1;
	}
	rd_vpu1_urgent_tables = vpu_urgent_table_rd_vpu1_tables;
	set_urgent_module->val = urgent_value;
	vpu_vcbus_setb(set_urgent_module->reg, urgent_value,
			set_urgent_module->start_bit, set_urgent_module->len);
	vpu_pr_debug(MODULE_ARB_URGENT, "%s urgent set to %d\n",
			set_urgent_module->name, urgent_value);
	val = vpu_vcbus_read(set_urgent_module->reg);
	vpu_pr_debug(MODULE_ARB_URGENT, "DI_RDARB_UGT_L1C1[0x%x] = 0x%x\n",
			set_urgent_module->reg, val);
	return 0;
}

int vpu_wrarb0_urgent_set(enum vpu_arb_mod_e vmod, u32 urgent_value)
{
	u32 val;
	struct vpu_urgent_table_s *wr_vpu0_urgent_tables =
					vpu_urgent_table_wr_vpu0_tables;
	struct vpu_urgent_table_s *set_urgent_module = NULL;

	if (!wr_vpu0_urgent_tables) {
		VPUERR("write0 urgent tables is error");
		return -1;
	}
	while (wr_vpu0_urgent_tables->vmod) {
		if (wr_vpu0_urgent_tables->vmod == vmod)
			set_urgent_module = wr_vpu0_urgent_tables;
		wr_vpu0_urgent_tables++;
	}
	if (!set_urgent_module) {
		VPUERR("wrarb0 no such module\n");
		return -1;
	}
	wr_vpu0_urgent_tables = vpu_urgent_table_wr_vpu0_tables;
	set_urgent_module->val = urgent_value;
	vpu_vcbus_setb(set_urgent_module->reg, urgent_value,
			set_urgent_module->start_bit, set_urgent_module->len);
	vpu_pr_debug(MODULE_ARB_URGENT, "%s urgent set to %d\n",
			set_urgent_module->name, urgent_value);
	val = vpu_vcbus_read(set_urgent_module->reg);
	vpu_pr_debug(MODULE_ARB_URGENT, "VPU_WRARB_UGT_L2C1[0x%x] = 0x%x\n",
			set_urgent_module->reg, val);
	return 0;
}

int vpu_wrarb1_urgent_set(enum vpu_arb_mod_e vmod, u32 urgent_value)
{
	u32 val;
	struct vpu_urgent_table_s *wr_vpu1_urgent_tables =
					vpu_urgent_table_wr_vpu1_tables;
	struct vpu_urgent_table_s *set_urgent_module = NULL;

	if (!wr_vpu1_urgent_tables) {
		VPUERR("write1 urgent tables is error");
		return -1;
	}
	while (wr_vpu1_urgent_tables->vmod) {
		if (wr_vpu1_urgent_tables->vmod == vmod)
			set_urgent_module = wr_vpu1_urgent_tables;
		wr_vpu1_urgent_tables++;
	}
	if (!set_urgent_module) {
		VPUERR("wrarb1 no such module\n");
		return -1;
	}
	wr_vpu1_urgent_tables = vpu_urgent_table_wr_vpu1_tables;
	set_urgent_module->val = urgent_value;
	vpu_vcbus_setb(set_urgent_module->reg, urgent_value,
			set_urgent_module->start_bit, set_urgent_module->len);
	vpu_pr_debug(MODULE_ARB_URGENT, "%s urgent set to %d\n",
			set_urgent_module->name, urgent_value);
	val = vpu_vcbus_read(set_urgent_module->reg);
	vpu_pr_debug(MODULE_ARB_URGENT, "VPU_WRARB_UGT_L1C1[0x%x] = 0x%x\n",
			set_urgent_module->reg, val);
	return 0;
}

int vpu_urgent_set(enum vpu_arb_mod_e vmod, u32 urgent_value)
{
	u32 ret = -1;

	if (vmod < VPU_ARB_VPP_ARB0 ||
		VPU_ARB_OSD1 >= ARB_MODULE_MAX) {
		VPUERR("module error!\n");
		return ret;
	}
	if (urgent_value > 3) {
		VPUERR("urgent value error!\n");
		return ret;
	}
	if (vmod >= VPU_ARB_VPP_ARB0 &&
		vmod <= VPU_ARB_VPU_DMA)
		ret = vpu_rdarb0_2_urgent_set(vmod, urgent_value);
	else if (vmod >= VPU_ARB_DI_IF1 &&
			vmod <= VPU_ARB_DI_AFBCD)
		ret = vpu_rdarb1_urgent_set(vmod, urgent_value);
	else if (vmod >= VPU_ARB_VDIN_WR &&
			vmod <= VPU_ARB_VPU_DMA_WR)
		ret = vpu_wrarb0_urgent_set(vmod, urgent_value);
	else if (vmod >= VPU_ARB_NR_WR &&
			vmod <= VPU_ARB_DI_AFBCE1)
		ret = vpu_wrarb1_urgent_set(vmod, urgent_value);
	return ret;
}

static int vpu_rdarb0_2_urgent_get(enum vpu_arb_mod_e vmod)
{
	u32 urgent_val;
	struct vpu_urgent_table_s *rd_vpu0_2_level2_urgent_tables =
					vpu_urgent_table_rd_vpu0_2_level2_tables;
	struct vpu_urgent_table_s *get_urgent_module = NULL;

	if (!rd_vpu0_2_level2_urgent_tables) {
		VPUERR("rdarb0_2 urgent table is error\n");
		return -1;
	}
	while (rd_vpu0_2_level2_urgent_tables->vmod) {
		if (rd_vpu0_2_level2_urgent_tables->vmod == vmod)
			get_urgent_module = rd_vpu0_2_level2_urgent_tables;
		rd_vpu0_2_level2_urgent_tables++;
	}
	if (!get_urgent_module) {
		VPUERR("rdarb0_2 no such module\n");
		return -1;
	}
	rd_vpu0_2_level2_urgent_tables = vpu_urgent_table_rd_vpu0_2_level2_tables;
	urgent_val = get_urgent_module->val;

	return urgent_val;
}

static int vpu_rdarb1_urgent_get(enum vpu_arb_mod_e vmod)
{
	u32 urgent_val;
	struct vpu_urgent_table_s *rd_vpu1_urgent_tables =
					vpu_urgent_table_rd_vpu1_tables;
	struct vpu_urgent_table_s *get_urgent_module = NULL;

	if (!rd_vpu1_urgent_tables) {
		VPUERR("rdarb1 urgent table is error\n");
		return -1;
	}
	while (rd_vpu1_urgent_tables->vmod) {
		if (rd_vpu1_urgent_tables->vmod == vmod)
			get_urgent_module = rd_vpu1_urgent_tables;
		rd_vpu1_urgent_tables++;
	}
	if (!get_urgent_module) {
		VPUERR("rdarb1 no such module\n");
		return -1;
	}
	rd_vpu1_urgent_tables = vpu_urgent_table_rd_vpu1_tables;
	urgent_val = get_urgent_module->val;

	return urgent_val;
}

static int vpu_wrarb0_urgent_get(enum vpu_arb_mod_e vmod)
{
	u32 urgent_val;
	struct vpu_urgent_table_s *wr_vpu1_urgent_tables =
					vpu_urgent_table_wr_vpu1_tables;
	struct vpu_urgent_table_s *get_urgent_module = NULL;

	if (!wr_vpu1_urgent_tables) {
		VPUERR("wrarb0 urgent table is error\n");
		return -1;
	}
	while (wr_vpu1_urgent_tables->vmod) {
		if (wr_vpu1_urgent_tables->vmod == vmod)
			get_urgent_module = wr_vpu1_urgent_tables;
		wr_vpu1_urgent_tables++;
	}
	if (!get_urgent_module) {
		VPUERR("wrarb0 no such module\n");
		return -1;
	}
	wr_vpu1_urgent_tables = vpu_urgent_table_wr_vpu1_tables;
	urgent_val = get_urgent_module->val;

	return urgent_val;
}

static int vpu_wrarb1_urgent_get(enum vpu_arb_mod_e vmod)
{
	u32 urgent_val;
	struct vpu_urgent_table_s *wr_vpu1_urgent_tables =
					vpu_urgent_table_wr_vpu1_tables;
	struct vpu_urgent_table_s *get_urgent_module = NULL;

	if (!wr_vpu1_urgent_tables) {
		VPUERR("wrarb1 urgent table is error\n");
		return -1;
	}
	while (wr_vpu1_urgent_tables->vmod) {
		if (wr_vpu1_urgent_tables->vmod == vmod)
			get_urgent_module = wr_vpu1_urgent_tables;
		wr_vpu1_urgent_tables++;
	}
	if (!get_urgent_module) {
		VPUERR("write1 urgent no such module\n");
		return -1;
	}
	wr_vpu1_urgent_tables = vpu_urgent_table_wr_vpu1_tables;
	urgent_val = get_urgent_module->val;

	return urgent_val;
}

int vpu_urgent_get(enum vpu_arb_mod_e vmod)
{
	u32 urgent_value = -1;

	if (vmod < VPU_ARB_VPP_ARB0 ||
		vmod >= ARB_MODULE_MAX) {
		VPUERR("module error!\n");
		return urgent_value;
	}

	if (vmod >= VPU_ARB_VPP_ARB0 &&
		vmod <= VPU_ARB_VPU_DMA)
		urgent_value = vpu_rdarb0_2_urgent_get(vmod);
	else if (vmod >= VPU_ARB_DI_IF1 &&
			vmod <= VPU_ARB_DI_AFBCD)
		urgent_value = vpu_rdarb1_urgent_get(vmod);
	else if (vmod >= VPU_ARB_VDIN_WR &&
			vmod <= VPU_ARB_VPU_DMA_WR)
		urgent_value = vpu_wrarb0_urgent_get(vmod);
	else if (vmod >= VPU_ARB_NR_WR &&
		vmod <= VPU_ARB_DI_AFBCE1)
		urgent_value = vpu_wrarb1_urgent_get(vmod);

	return urgent_value;
}

/*
 *DI module use clk1,other vpu module use clk2
 *clk1_rdarb corresponds to vpu_read1
 *clk2_rdarb corresponds to vpu_read0 and vpu_read2
 */
void dump_vpu_clk1_rdarb_table(void)
{
	u32 val;
	struct vpu_arb_table_s *vpu1_rdarb_tables = vpu_rdarb_vpu1_tables;

	if (!vpu1_rdarb_tables) {
		VPUERR("clk1 rdarb table error!\n");
		return;
	}
	vpu_pr_debug(MODULE_ARB_MODE, "vpu clk1 rdarb table\n");
	while (vpu1_rdarb_tables->vmod) {
		vpu_pr_debug(MODULE_ARB_MODE, "%9s ---> vpu_read1\n",
			vpu1_rdarb_tables->name);
		vpu1_rdarb_tables++;
	}
	vpu1_rdarb_tables = vpu_rdarb_vpu1_tables;
	val = vpu_vcbus_read(vpu_rdarb_vpu1_tables[0].reg);
	vpu_pr_debug(MODULE_ARB_MODE, "DI_RDARB_MODE_L1C1[0x%x] = 0x%x\n",
		vpu_rdarb_vpu1_tables[0].reg, val);
}

void dump_vpu_clk2_rdarb_table(void)
{
	u32 val;
	struct vpu_arb_table_s *vpu0_2_rdarb_level1_tables =
					vpu_rdarb_vpu0_2_level1_tables;
	struct vpu_arb_table_s *vpu0_2_rdarb_level2_tables =
					vpu_rdarb_vpu0_2_level2_tables;

	if (!vpu0_2_rdarb_level1_tables ||
		!vpu0_2_rdarb_level2_tables) {
		VPUERR("clk2 rdarb table error!\n");
		return;
	}
	vpu_pr_debug(MODULE_ARB_MODE, "vpu clk2 rdarb table\n");
	/*
	 *1.VPU_READ0
	 * 1.1 VPP_ARB0/1->VPU_READ0
	 */
	while (vpu0_2_rdarb_level2_tables->vmod) {
		if (vpu0_2_rdarb_level2_tables->bind_port ==
				VPU_READ0) {
			if (vpu0_2_rdarb_level2_tables->vmod ==
					VPU_ARB_VPP_ARB0) {
			/*VPU_VPP_ARB0*/
				while (vpu0_2_rdarb_level1_tables->vmod) {
					if (vpu0_2_rdarb_level1_tables->bind_port ==
							VPU_ARB_VPP_ARB0) {
						vpu_pr_debug(MODULE_ARB_MODE, "%-10s ---> vpp_arb0 ---> vpu_read0\n",
							vpu0_2_rdarb_level1_tables->name);
					}
					vpu0_2_rdarb_level1_tables++;
				}
				vpu0_2_rdarb_level1_tables = vpu_rdarb_vpu0_2_level1_tables;
			} else if (vpu0_2_rdarb_level2_tables->vmod ==
							VPU_ARB_VPP_ARB1) {
			/*VPU_VPP_ARB1*/
				while (vpu0_2_rdarb_level1_tables->vmod) {
					if (vpu0_2_rdarb_level1_tables->bind_port ==
							VPU_ARB_VPP_ARB1) {
						vpu_pr_debug(MODULE_ARB_MODE, "%-10s ---> vpp_arb1 ---> vpu_read0\n",
							vpu0_2_rdarb_level1_tables->name);
					}
					vpu0_2_rdarb_level1_tables++;
				}
				vpu0_2_rdarb_level1_tables = vpu_rdarb_vpu0_2_level1_tables;
			}
		}
		vpu0_2_rdarb_level2_tables++;
	}
	vpu0_2_rdarb_level2_tables = vpu_rdarb_vpu0_2_level2_tables;

	/* 1.2 level2 other module-> VPU_READ0*/
	while (vpu0_2_rdarb_level2_tables->vmod) {
		if (vpu0_2_rdarb_level2_tables->bind_port ==
				VPU_READ0) {
			if (vpu0_2_rdarb_level2_tables->vmod !=
					VPU_ARB_VPP_ARB0 &&
				vpu0_2_rdarb_level2_tables->vmod !=
					VPU_ARB_VPP_ARB1) {
				/*level2 other module*/
				vpu_pr_debug(MODULE_ARB_MODE, "%24s ---> vpu_read0\n",
					vpu0_2_rdarb_level2_tables->name);
				}
			}
		vpu0_2_rdarb_level2_tables++;
	}
	vpu0_2_rdarb_level2_tables = vpu_rdarb_vpu0_2_level2_tables;

	/*
	 *2.VPU_READ2
	 * 2.1 VPP_ARB0/1->VPU_READ2
	 */
	while (vpu0_2_rdarb_level2_tables->vmod) {
		if (vpu0_2_rdarb_level2_tables->bind_port ==
				VPU_READ2) {
			if (vpu0_2_rdarb_level2_tables->vmod ==
					VPU_ARB_VPP_ARB0) {
			/*VPU_VPP_ARB0*/
				while (vpu0_2_rdarb_level1_tables->vmod) {
					if (vpu0_2_rdarb_level1_tables->bind_port ==
							VPU_ARB_VPP_ARB0) {
						vpu_pr_debug(MODULE_ARB_MODE, "%-10s ---> vpp_arb0 ---> vpu_read2\n",
							vpu0_2_rdarb_level1_tables->name);
					}
					vpu0_2_rdarb_level1_tables++;
				}
				vpu0_2_rdarb_level1_tables = vpu_rdarb_vpu0_2_level1_tables;
			} else if (vpu0_2_rdarb_level2_tables->vmod ==
					VPU_ARB_VPP_ARB1) {
			/*VPU_VPP_ARB1*/
				while (vpu0_2_rdarb_level1_tables->vmod) {
					if (vpu0_2_rdarb_level1_tables->bind_port ==
							VPU_ARB_VPP_ARB1) {
						vpu_pr_debug(MODULE_ARB_MODE, "%-10s ---> vpp_arb1 ---> vpu_read2\n",
							vpu0_2_rdarb_level1_tables->name);
					}
					vpu0_2_rdarb_level1_tables++;
				}
				vpu0_2_rdarb_level1_tables = vpu_rdarb_vpu0_2_level1_tables;
			}
		}
		vpu0_2_rdarb_level2_tables++;
	}
	vpu0_2_rdarb_level2_tables = vpu_rdarb_vpu0_2_level2_tables;

	/* 2.2 level2 other module-> VPU_READ2*/
	while (vpu0_2_rdarb_level2_tables->vmod) {
		if (vpu0_2_rdarb_level2_tables->bind_port ==
				VPU_READ2) {
			if (vpu0_2_rdarb_level2_tables->vmod !=
					VPU_ARB_VPP_ARB0 &&
				vpu0_2_rdarb_level2_tables->vmod !=
					VPU_ARB_VPP_ARB1) {
				vpu_pr_debug(MODULE_ARB_MODE, "%24s ---> vpu_read2\n",
					vpu0_2_rdarb_level2_tables->name);
				}
			}
		vpu0_2_rdarb_level2_tables++;
	}
	vpu0_2_rdarb_level2_tables = vpu_rdarb_vpu0_2_level2_tables;

	val = vpu_vcbus_read(vpu_rdarb_vpu0_2_level1_tables[0].reg);
	vpu_pr_debug(MODULE_ARB_MODE, "VPU_RDARB_MOD[0x%x] = 0x%x\n",
		vpu_rdarb_vpu0_2_level1_tables[0].reg, val);
	val = vpu_vcbus_read(vpu_rdarb_vpu0_2_level2_tables[0].reg);
	vpu_pr_debug(MODULE_ARB_MODE, "VPU_RDARB_MODE_L2C1[0x%x] = 0x%x\n",
		vpu_rdarb_vpu0_2_level2_tables[0].reg, val);
}

void dump_vpu_rdarb_table(void)
{
	dump_vpu_clk1_rdarb_table();
	dump_vpu_clk2_rdarb_table();
}

/*
 *DI module use clk1,other vpu module use clk2
 *clk1_wrarb corresponds to VPU_WRITE1
 *clk2_wrarb corresponds to vpu_WRITE0
 */
void dump_vpu_clk2_wrarb_table(void)
{
	u32 val;
	struct vpu_arb_table_s *vpu0_wrarb_tables = vpu_wrarb_vpu0_tables;

	if (!vpu0_wrarb_tables) {
		VPUERR("clk2 wrarb table error!\n");
		return;
	}
	vpu_pr_debug(MODULE_ARB_MODE, "vpu clk2_wrarb table:\n");
	while (vpu0_wrarb_tables->vmod) {
		vpu_pr_debug(MODULE_ARB_MODE, "%10s ---> vpu_write0\n",
			vpu0_wrarb_tables->name);
		vpu0_wrarb_tables++;
	}
	vpu0_wrarb_tables = vpu_wrarb_vpu0_tables;
	val = vpu_vcbus_read(vpu_wrarb_vpu0_tables[0].reg);
	vpu_pr_debug(MODULE_ARB_MODE, "VPU_WRARB_MODE_L2C1[0x%x] = 0x%x\n",
		vpu_wrarb_vpu0_tables[0].reg, val);
}

void dump_vpu_clk1_wrarb_table(void)
{
	u32 val;
	struct vpu_arb_table_s *vpu1_wrarb_tables = vpu_wrarb_vpu1_tables;

	if (!vpu1_wrarb_tables) {
		VPUERR("clk1 wrarb table error!\n");
		return;
	}
	vpu_pr_debug(MODULE_ARB_MODE, "vpu clk1_wrarb table\n");
	while (vpu1_wrarb_tables->vmod) {
		vpu_pr_debug(MODULE_ARB_MODE, "%12s ---> vpu_write1\n",
			vpu1_wrarb_tables->name);
		vpu1_wrarb_tables++;
	}
	vpu1_wrarb_tables = vpu_wrarb_vpu1_tables;
	val = vpu_vcbus_read(vpu_wrarb_vpu1_tables[0].reg);
	vpu_pr_debug(MODULE_ARB_MODE, "DI_WRARB_MODE_L1C1[0x%x] = 0x%x\n",
		vpu_wrarb_vpu1_tables[0].reg, val);
}

void dump_vpu_wrarb_table(void)
{
	dump_vpu_clk1_wrarb_table();
	dump_vpu_clk2_wrarb_table();
}

static void sort_urgent_table(struct vpu_urgent_table_s *urgent_table)
{
	int i, j;
	int array_size = 0;
	struct vpu_urgent_table_s temp;
	struct vpu_urgent_table_s *urgent_module = urgent_table;

	while (urgent_module->vmod) {
		array_size++;
		urgent_module++;
	}
	urgent_module = urgent_table;

	for (i = 0; i < array_size - 1; i++) {
		for (j = 0; j < array_size - i - 1; j++) {
			if (urgent_module[j].val < urgent_module[j + 1].val) {
				temp = urgent_module[j];
				urgent_module[j] = urgent_module[j + 1];
				urgent_module[j + 1] = temp;
			}
		}
	}
	for (i = 0; i < array_size; i++)
		urgent_table[i] = urgent_module[i];
}

static void dump_vpu_rdarb0_2_urgent(void)
{
	u32 val;
	struct vpu_urgent_table_s *rd_vpu0_2_level2_urgent_tables =
						vpu_urgent_table_rd_vpu0_2_level2_tables;

	if (!rd_vpu0_2_level2_urgent_tables) {
		VPUERR("vpu_read0_2 urgent table is error\n");
		return;
	}
	vpu_pr_debug(MODULE_ARB_URGENT, "vpu rdarb0_2 urgent table\n");
	sort_urgent_table(rd_vpu0_2_level2_urgent_tables);
	while (rd_vpu0_2_level2_urgent_tables->vmod) {
		vpu_pr_debug(MODULE_ARB_URGENT, "module name:%17s urgent_value: %d\n",
			rd_vpu0_2_level2_urgent_tables->name,
			rd_vpu0_2_level2_urgent_tables->val);
		rd_vpu0_2_level2_urgent_tables++;
	}
	rd_vpu0_2_level2_urgent_tables = vpu_urgent_table_rd_vpu0_2_level2_tables;
	val = vpu_vcbus_read(vpu_urgent_table_rd_vpu0_2_level2_tables[0].reg);
	vpu_pr_debug(MODULE_ARB_URGENT, "VPU_RDARB_UGT_L2C1[0x%x] = 0x%x\n",
		vpu_urgent_table_rd_vpu0_2_level2_tables[0].reg, val);
}

static void dump_vpu_rdarb1_urgent(void)
{
	u32 val;
	struct vpu_urgent_table_s *rd_vpu1_urgent_tables =
						vpu_urgent_table_rd_vpu1_tables;

	if (!rd_vpu1_urgent_tables) {
		VPUERR("vpu_read1 urgent table is error\n");
		return;
	}
	vpu_pr_debug(MODULE_ARB_URGENT, "vpu rdarb1 urgent table\n");
	sort_urgent_table(rd_vpu1_urgent_tables);
	while (rd_vpu1_urgent_tables->vmod) {
		vpu_pr_debug(MODULE_ARB_URGENT, "module name:%15s urgent_value: %d\n",
			rd_vpu1_urgent_tables->name,
			rd_vpu1_urgent_tables->val);
		rd_vpu1_urgent_tables++;
	}
	rd_vpu1_urgent_tables = vpu_urgent_table_rd_vpu1_tables;
	val = vpu_vcbus_read(vpu_urgent_table_rd_vpu1_tables[0].reg);
	vpu_pr_debug(MODULE_ARB_URGENT, "DI_RDARB_UGT_L1C1[0x%x] = 0x%x\n",
		vpu_urgent_table_rd_vpu1_tables[0].reg, val);
}

static void dump_vpu_wrarb0_urgent(void)
{
	u32 val;
	struct vpu_urgent_table_s *wr_vpu0_urgent_tables =
						vpu_urgent_table_wr_vpu0_tables;

	if (!wr_vpu0_urgent_tables) {
		VPUERR("vpu_write0 urgent table is error\n");
		return;
	}
	vpu_pr_debug(MODULE_ARB_URGENT, "vpu wrarb0 urgent table\n");
	sort_urgent_table(wr_vpu0_urgent_tables);
	while (wr_vpu0_urgent_tables->vmod) {
		vpu_pr_debug(MODULE_ARB_URGENT, "module name:%15s urgent_value: %d\n",
			wr_vpu0_urgent_tables->name,
			wr_vpu0_urgent_tables->val);
		wr_vpu0_urgent_tables++;
	}
	wr_vpu0_urgent_tables = vpu_urgent_table_wr_vpu0_tables;
	val = vpu_vcbus_read(vpu_urgent_table_wr_vpu0_tables[0].reg);
	vpu_pr_debug(MODULE_ARB_URGENT, "VPU_WRARB_UGT_L2C1[0x%x] = 0x%x\n",
		vpu_urgent_table_wr_vpu0_tables[0].reg, val);
}

static void dump_vpu_wrarb1_urgent(void)
{
	u32 val;
	struct vpu_urgent_table_s *wr_vpu1_urgent_tables =
						vpu_urgent_table_wr_vpu1_tables;

	if (!wr_vpu1_urgent_tables) {
		VPUERR("vpu_write1 urgent table is error\n");
		return;
	}
	vpu_pr_debug(MODULE_ARB_URGENT, "vpu wrarb1 urgent table\n");
	sort_urgent_table(wr_vpu1_urgent_tables);
	while (wr_vpu1_urgent_tables->vmod) {
		vpu_pr_debug(MODULE_ARB_URGENT, "module name:%15s urgent_value: %d\n",
			wr_vpu1_urgent_tables->name,
			wr_vpu1_urgent_tables->val);
		wr_vpu1_urgent_tables++;
	}
	wr_vpu1_urgent_tables = vpu_urgent_table_wr_vpu1_tables;
	val = vpu_vcbus_read(vpu_urgent_table_wr_vpu1_tables[0].reg);
	vpu_pr_debug(MODULE_ARB_URGENT, "DI_WRARB_UGT_L1C1[0x%x] = 0x%x\n",
		vpu_urgent_table_wr_vpu1_tables[0].reg, val);
}

void dump_vpu_urgent_table(u32 vpu_port)
{
	if (vpu_port == VPU_READ0 ||
		vpu_port == VPU_READ2)
		dump_vpu_rdarb0_2_urgent();
	else if (vpu_port == VPU_READ1)
		dump_vpu_rdarb1_urgent();
	else if (vpu_port == VPU_WRITE0)
		dump_vpu_wrarb0_urgent();
	else if (vpu_port == VPU_WRITE1)
		dump_vpu_wrarb1_urgent();
}

int vpu_arb_register(enum vpu_arb_mod_e module, void *cb)
{
	struct vpu_arb_info *ins = &vpu_arb_ins[module];
	struct mutex *lock = NULL;

	if (module > ARB_MODULE_MAX) {
		VPUERR("%s failed, module = %d\n", __func__, module);
		return -1;
	}
	if (!ins->registered) {
		lock = &ins->vpu_arb_lock;
		mutex_lock(lock);
		ins->registered = 1;
		ins->arb_cb = cb;
		mutex_unlock(lock);
	}
	pr_info("%s module=%d vpu_arb register ok\n", __func__, module);
	return 0;
}
EXPORT_SYMBOL(vpu_arb_register);

int vpu_arb_unregister(enum vpu_arb_mod_e module)
{
	struct vpu_arb_info *ins = &vpu_arb_ins[module];
	struct mutex *lock = NULL;

	if (module > ARB_MODULE_MAX) {
		VPUERR("%s failed, module = %d\n", __func__, module);
		return -1;
	}
	if (ins->registered) {
		lock = &ins->vpu_arb_lock;
		mutex_lock(lock);
		ins->registered = 0;
		ins->arb_cb = NULL;
		mutex_unlock(lock);
	}
	return 0;
}
EXPORT_SYMBOL(vpu_arb_unregister);

int vpu_arb_config(enum vpu_arb_mod_e module, u32 urgent_value)
{
	struct vpu_arb_info *ins = &vpu_arb_ins[module];
	u32 pre_value;

	if (module > ARB_MODULE_MAX) {
		VPUERR("%s failed, module = %d\n", __func__, module);
		return -1;
	}
	if (ins->registered) {
		pre_value = vpu_urgent_get(module);
		vpu_urgent_set(module, urgent_value);
		if (ins->arb_cb)
			ins->arb_cb(module, pre_value, urgent_value);
	}
	return 0;
}
EXPORT_SYMBOL(vpu_arb_config);

void init_read0_2_bind(void)
{
	struct vpu_arb_table_s *vpu_rdarb_vpu0_2_level1 = vpu_rdarb_vpu0_2_level1_tables;
	struct vpu_arb_table_s *vpu_rdarb_vpu0_2_level2 = vpu_rdarb_vpu0_2_level2_tables;

	if (!vpu_rdarb_vpu0_2_level1 || !vpu_rdarb_vpu0_2_level2) {
		VPUERR("%s table error\n", __func__);
		return;
	}
	//level1 bind
	while (vpu_rdarb_vpu0_2_level1->vmod &&
		vpu_rdarb_vpu0_2_level1->bind_port != NO_PORT) {
		vpu_rdarb0_2_bind_l1(vpu_rdarb_vpu0_2_level1->vmod,
			vpu_rdarb_vpu0_2_level1->bind_port);
		vpu_rdarb_vpu0_2_level1++;
	}
	//level2 bind
	while (vpu_rdarb_vpu0_2_level2->vmod &&
		vpu_rdarb_vpu0_2_level2->bind_port != NO_PORT) {
		vpu_rdarb0_2_bind_l2(vpu_rdarb_vpu0_2_level2->vmod,
			vpu_rdarb_vpu0_2_level2->bind_port);
		vpu_rdarb_vpu0_2_level2++;
	}
}

void init_read1_bind(void)
{
	struct vpu_arb_table_s *vpu_rdarb_vpu1 = vpu_rdarb_vpu1_tables;

	if (!vpu_rdarb_vpu1) {
		VPUERR("%s table error\n", __func__);
		return;
	}
	while (vpu_rdarb_vpu1->vmod && vpu_rdarb_vpu1->bind_port != NO_PORT) {
		vpu_rdarb1_bind(vpu_rdarb_vpu1->vmod,
			vpu_rdarb_vpu1->bind_port);
		vpu_rdarb_vpu1++;
	}
}

void init_write0_bind(void)
{
	struct vpu_arb_table_s *vpu_wrarb_vpu0 = vpu_wrarb_vpu0_tables;

	if (!vpu_wrarb_vpu0) {
		VPUERR("%s table error\n", __func__);
		return;
	}
	while (vpu_wrarb_vpu0->vmod && vpu_wrarb_vpu0->bind_port != NO_PORT) {
		vpu_wrarb0_bind(vpu_wrarb_vpu0->vmod,
			vpu_wrarb_vpu0->bind_port);
		vpu_wrarb_vpu0++;
	}
}

void init_write1_bind(void)
{
	struct vpu_arb_table_s *vpu_wrarb_vpu1 = vpu_wrarb_vpu1_tables;

	if (!vpu_wrarb_vpu1) {
		VPUERR("%s table error\n", __func__);
		return;
	}
	while (vpu_wrarb_vpu1->vmod && vpu_wrarb_vpu1->bind_port != NO_PORT) {
		vpu_wrarb0_bind(vpu_wrarb_vpu1->vmod,
			vpu_wrarb_vpu1->bind_port);
		vpu_wrarb_vpu1++;
	}
}

void init_read0_2_write1_urgent(void)
{
	struct vpu_urgent_table_s *vpu_urgent_table_rd_vpu0_2 =
				vpu_urgent_table_rd_vpu0_2_level2_tables;
	struct vpu_urgent_table_s *vpu_urgent_table_wr_vpu0 =
				vpu_urgent_table_wr_vpu0_tables;

	if (!vpu_urgent_table_rd_vpu0_2 ||
		!vpu_urgent_table_wr_vpu0) {
		VPUERR("%s urgent table error\n", __func__);
		return;
	}
	while (vpu_urgent_table_rd_vpu0_2->vmod) {
		vpu_rdarb0_2_urgent_set(vpu_urgent_table_rd_vpu0_2->vmod,
			vpu_urgent_table_rd_vpu0_2->val);
		vpu_urgent_table_rd_vpu0_2++;
	}
	while (vpu_urgent_table_wr_vpu0->vmod) {
		vpu_wrarb0_urgent_set(vpu_urgent_table_wr_vpu0->vmod,
			vpu_urgent_table_wr_vpu0->val);
		vpu_urgent_table_wr_vpu0++;
	}
}

void init_read1_write1_urgent(void)
{
	struct vpu_urgent_table_s *vpu_urgent_table_rd_vpu1 =
				vpu_urgent_table_rd_vpu1_tables;
	struct vpu_urgent_table_s *vpu_urgent_table_wr_vpu1 =
				vpu_urgent_table_wr_vpu1_tables;
	if (!vpu_urgent_table_rd_vpu1 ||
		!vpu_urgent_table_wr_vpu1) {
		VPUERR("%s urgent table error\n", __func__);
		return;
	}
	while (vpu_urgent_table_rd_vpu1->vmod) {
		vpu_rdarb1_urgent_set(vpu_urgent_table_rd_vpu1->vmod,
			vpu_urgent_table_rd_vpu1->val);
		vpu_urgent_table_rd_vpu1++;
	}
	while (vpu_urgent_table_wr_vpu1->vmod) {
		vpu_wrarb1_urgent_set(vpu_urgent_table_wr_vpu1->vmod,
			vpu_urgent_table_wr_vpu1->val);
		vpu_urgent_table_wr_vpu1++;
	}
}

void init_di_arb_urgent(void)
{
	init_read1_bind();
	init_write1_bind();
	init_read1_write1_urgent();
}

int init_arb_urgent_table(void)
{
	int i;
	if (vpu_conf.data->chip_type == VPU_CHIP_T7) {
		vpu_rdarb_vpu0_2_level1_tables = vpu_rdarb_vpu0_2_level1_t7;
		vpu_rdarb_vpu0_2_level2_tables = vpu_rdarb_vpu0_2_level2_t7;
		vpu_rdarb_vpu1_tables = vpu_rdarb_vpu1_t7;
		vpu_wrarb_vpu0_tables = vpu_wrarb_vpu0_t7;
		vpu_wrarb_vpu1_tables = vpu_wrarb_vpu1_t7;

		vpu_urgent_table_rd_vpu0_2_level2_tables = vpu_urgent_table_rd_vpu0_2_level2_t7;
		vpu_urgent_table_rd_vpu1_tables = vpu_urgent_table_rd_vpu1_t7;
		vpu_urgent_table_wr_vpu0_tables = vpu_urgent_table_wr_vpu0_t7;
		vpu_urgent_table_wr_vpu1_tables = vpu_urgent_table_wr_vpu1_t7;
	} else if (vpu_conf.data->chip_type == VPU_CHIP_S7) {
		vpu_rdarb_vpu0_2_level1_tables = vpu_rdarb_vpu0_2_level1_s7;
		vpu_rdarb_vpu0_2_level2_tables = vpu_rdarb_vpu0_2_level2_s7;
		vpu_rdarb_vpu1_tables = vpu_rdarb_vpu1_s7;
		vpu_wrarb_vpu0_tables = vpu_wrarb_vpu0_s7;
		vpu_wrarb_vpu1_tables = vpu_wrarb_vpu1_s7;

		vpu_urgent_table_rd_vpu0_2_level2_tables = vpu_urgent_table_rd_vpu0_2_level2_s7;
		vpu_urgent_table_rd_vpu1_tables = vpu_urgent_table_rd_vpu1_s7;
		vpu_urgent_table_wr_vpu0_tables = vpu_urgent_table_wr_vpu0_s7;
		vpu_urgent_table_wr_vpu1_tables = vpu_urgent_table_wr_vpu1_s7;
	}

	for (i = 1; i < ARB_MODULE_MAX; i++)
		mutex_init(&vpu_arb_ins[i].vpu_arb_lock);
	init_read0_2_bind();
	init_write0_bind();
	init_read0_2_write1_urgent();
	return 0;
}
