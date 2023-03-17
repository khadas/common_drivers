// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/types.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/errno.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/amlogic/media/video_sink/video.h>
#include <linux/amlogic/media/vout/vout_notify.h>

#include "../amvecm/arch/vpp_regs.h"
#include "../amvecm/arch/vpp_hdr_regs.h"
#include "../amvecm/arch/vpp_dolbyvision_regs.h"
#include "../amvecm/amcsc.h"
#include "../amvecm/reg_helper.h"
#include <linux/amlogic/media/registers/regs/viu_regs.h>
#include <linux/amlogic/media/amdolbyvision/dolby_vision.h>
#include <linux/amlogic/media/vpu/vpu.h>
#include <linux/dma-map-ops.h>
#include <linux/amlogic/iomap.h>
#include "md_config.h"
#include <linux/of.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/arm-smccc.h>
#include <linux/amlogic/media/vout/lcd/lcd_notify.h>
#include <linux/amlogic/media/lut_dma/lut_dma.h>

#include "amdv.h"
#include "amdv_hw5.h"
#include "amdv_regs_hw5.h"
#include "mmu_config.h"

struct top1_pyramid_addr top1_py_addr;
struct dolby5_top1_type top1_type;
static u32 isr_cnt;
bool enable_top1;//todo
bool top1_done;

#define L1L4_MD_COUNT 10
u32 l1l4_md[L1L4_MD_COUNT][4];
u32 l1l4_wr_index;
u32 l1l4_rd_index;

#define READ_REG_FROM_FILE 1

static void dolby5_top1_rdmif
	(int hsize,
	int vsize,
	int bit_mode,
	int fmt_mode,
	unsigned int baddr[3],
	int stride[3])
{
	unsigned int separate_en = 0; //420
	unsigned int yuv444 = 0; //444
	unsigned int yuv420 = 0;

	unsigned int luma_xbgn = 0;
	unsigned int luma_xend = hsize - 1;
	unsigned int luma_ybgn = 0;
	unsigned int luma_yend = vsize - 1;

	unsigned int chrm_xbgn = 0;
	unsigned int chrm_xend = 0;
	unsigned int chrm_ybgn = 0;
	unsigned int chrm_yend = 0;

	unsigned int luma_hsize = 0;
	unsigned int chrm_hsize = 0;

	unsigned int cntl_bits_mode;
	unsigned int cntl_pixel_bytes;
	unsigned int cntl_color_map = 0; //420 nv12/21

	unsigned int cvfmt_en = 0; //420
	unsigned int chfmt_en = 0; //420/422
	unsigned int yc_ratio = 0;

	if (fmt_mode == 2)
		separate_en = 1;

	if (fmt_mode == 0) {
		yuv444 = 1;
	} else if (fmt_mode == 2) {
		yuv420 = 1;
		cntl_color_map = 1;
		cvfmt_en = 1;
		chfmt_en = 1;
	} else {
		chfmt_en = 1;
	}

	if (yuv444 == 1)
		chrm_xend = hsize - 1;
	else
		chrm_xend = (hsize + 1) / 2 - 1;

	if (yuv420 == 1)
		chrm_yend = (vsize + 1) / 2 - 1;
	else
		chrm_yend = vsize - 1;

	luma_hsize = hsize;
	chrm_hsize = chrm_xend - chrm_xbgn + 1;

	if (chfmt_en == 1)
		yc_ratio = 1;
	else
		yc_ratio = 0;

	VSYNC_WR_DV_REG(DOLBY_TOP1_LUMA_X, luma_xend << 16 | luma_xbgn);
	VSYNC_WR_DV_REG(DOLBY_TOP1_LUMA_Y, luma_yend << 16 | luma_ybgn);
	VSYNC_WR_DV_REG(DOLBY_TOP1_CHROMA_X, chrm_xend << 16 | chrm_xbgn);
	VSYNC_WR_DV_REG(DOLBY_TOP1_CHROMA_Y, chrm_yend << 16 | chrm_ybgn);

	switch ((bit_mode << 2) | fmt_mode) {
	case 0x0:
		cntl_bits_mode = 2;
		cntl_pixel_bytes = 2;
		break; //10bit 444
	case 0x01:
		cntl_bits_mode = 1;
		cntl_pixel_bytes = 1;
		break; //10bit 422
	case 0x02:
		cntl_bits_mode = 3;
		cntl_pixel_bytes = 1;
		break; //10bit 420
	case 0x10:
		cntl_bits_mode = 0;
		cntl_pixel_bytes = 2;
		break; //8bit 444
	case 0x11:
		cntl_bits_mode = 0;
		cntl_pixel_bytes = 1;
		break; //8bit 422
	case 0x12:
		cntl_bits_mode = 0;
		cntl_pixel_bytes = 1;
		break; //8bit 420
	default:
		cntl_bits_mode = 2;
		cntl_pixel_bytes = 2;
		break; //10bit 444
	}

	pr_dv_dbg("bit_mode: %d fmt_mode: %d\n", bit_mode, fmt_mode);

	VSYNC_WR_DV_REG(DOLBY_TOP1_GEN_REG,
		(1 << 25) |                //luma last line end mode
		(0 << 19) |                //cntl hold lines
		(yuv444 << 16) |           //demux_mode
		(cntl_pixel_bytes << 14) | //cntl_pixel_bytes
		(1 << 4) |                 //little endian
		(separate_en << 1) |       //separate_en
		1);                        //cntl_enable

	VSYNC_WR_DV_REG_BITS(DOLBY_TOP1_GEN_REG2, cntl_color_map, 0, 2);
	VSYNC_WR_DV_REG_BITS(DOLBY_TOP1_GEN_REG3, cntl_bits_mode, 8, 2);
	VSYNC_WR_DV_REG_BITS(DOLBY_TOP1_GEN_REG3, 0, 0, 1);//cntl_64bit_rev

	VSYNC_WR_DV_REG(DOLBY_TOP1_FMT_CTRL,
		(1 << 28) | //hfmt repeat
		(yc_ratio << 21) |
		(chfmt_en << 20) |
		(1 << 18) | //vfmt repeat
		cvfmt_en);

	VSYNC_WR_DV_REG(DOLBY_TOP1_FMT_W, (luma_hsize << 16) | chrm_hsize);
	VSYNC_WR_DV_REG(DOLBY_TOP1_BADDR_Y, baddr[0]);
	VSYNC_WR_DV_REG(DOLBY_TOP1_BADDR_CB, baddr[1]);
	VSYNC_WR_DV_REG(DOLBY_TOP1_BADDR_CR, baddr[2]);
	VSYNC_WR_DV_REG(DOLBY_TOP1_STRIDE_0, (stride[1] << 16) | stride[0]); //stride_u | stride_y
	VSYNC_WR_DV_REG(DOLBY_TOP1_STRIDE_1,
		VSYNC_RD_DV_REG(DOLBY_TOP1_STRIDE_1) |
		(1 << 16) |
		stride[2]); //acc_mode | stride_v
};

static void dolby5_ahb_reg_config(u32 *reg_baddr,
	u32 core_sel, u32 reg_num)
{
	int i;
	int reg_val, reg_addr;

	for (i = 0; i < reg_num; i = i + 1) {
		reg_val = reg_baddr[i * 2];
		reg_addr = reg_baddr[i * 2 + 1];

		reg_addr = reg_addr & 0xffff;
		reg_val = reg_val & 0xffffffff;
		if (i < 5)
			pr_dv_dbg("=== addr: %x val:%x ===\n", reg_addr, reg_val);

//#if READ_REG_FROM_FILE
		reg_addr = reg_addr >> 2;
//#endif

		if (core_sel == 0)//core1
			VSYNC_WR_DV_REG(DOLBY5_CORE1_REG_BASE + reg_addr, reg_val);
		else if (core_sel == 1)//core1b
			VSYNC_WR_DV_REG(DOLBY5_CORE1B_REG_BASE + reg_addr, reg_val);
		else if (core_sel == 2) {//core2
			if (reg_addr < 256)
				VSYNC_WR_DV_REG(DOLBY5_CORE2_REG_BASE0 + reg_addr, reg_val);
			else if (reg_addr < 512)
				VSYNC_WR_DV_REG(DOLBY5_CORE2_REG_BASE1 + reg_addr - 256, reg_val);
			else
				VSYNC_WR_DV_REG(DOLBY5_CORE2_REG_BASE2 + reg_addr - 512, reg_val);
		}
	}
}

static void dolby5_top1_ini(struct dolby5_top1_type *dolby5_top1)
{
	int core1_hsize = dolby5_top1->core1_hsize;
	int core1_vsize = dolby5_top1->core1_vsize;
	//int rdma_num = dolby5_top1->rdma_num;
	//int rdma_size = dolby5_top1->rdma_size;
	int bit_mode = dolby5_top1->bit_mode;
	int fmt_mode = dolby5_top1->fmt_mode;
	unsigned int rdmif_baddr[3];
	int rdmif_stride[3];
	int vsync_sel = dolby5_top1->vsync_sel;
	int i;

	dolby5_ahb_reg_config(dolby5_top1->core1_ahb_baddr, 0, dolby5_top1->core1_ahb_num);
	dolby5_ahb_reg_config(dolby5_top1->core1b_ahb_baddr, 1, dolby5_top1->core1b_ahb_num);

	pr_dv_dbg("==== config core1/1b_ahb_reg first before top reg! ====\n");

	for (i = 0; i < 3; i++) {
		rdmif_baddr[i]  = dolby5_top1->rdmif_baddr[i];
		rdmif_stride[i] = dolby5_top1->rdmif_stride[i];
	}

	VSYNC_WR_DV_REG(DOLBY_TOP1_PIC_SIZE, (core1_vsize << 16) | core1_hsize);

	VSYNC_WR_DV_REG_BITS(DOLBY_TOP1_PYWR_CTRL, dolby5_top1->py_level, 0, 1);
	VSYNC_WR_DV_REG(DOLBY_TOP1_PYWR_BADDR1, dolby5_top1->py_baddr[0]);
	VSYNC_WR_DV_REG(DOLBY_TOP1_PYWR_BADDR2, dolby5_top1->py_baddr[1]);
	VSYNC_WR_DV_REG(DOLBY_TOP1_PYWR_BADDR3, dolby5_top1->py_baddr[2]);
	VSYNC_WR_DV_REG(DOLBY_TOP1_PYWR_BADDR4, dolby5_top1->py_baddr[3]);
	VSYNC_WR_DV_REG(DOLBY_TOP1_PYWR_BADDR5, dolby5_top1->py_baddr[4]);
	VSYNC_WR_DV_REG(DOLBY_TOP1_PYWR_BADDR6, dolby5_top1->py_baddr[5]);
	VSYNC_WR_DV_REG(DOLBY_TOP1_PYWR_BADDR7, dolby5_top1->py_baddr[6]);

	VSYNC_WR_DV_REG(DOLBY_TOP1_PYWR_STRIDE12,
		(dolby5_top1->py_stride[1] << 16) | dolby5_top1->py_stride[0]);
	VSYNC_WR_DV_REG(DOLBY_TOP1_PYWR_STRIDE34,
		(dolby5_top1->py_stride[3] << 16) | dolby5_top1->py_stride[2]);
	VSYNC_WR_DV_REG(DOLBY_TOP1_PYWR_STRIDE56,
		(dolby5_top1->py_stride[5] << 16) | dolby5_top1->py_stride[4]);
	VSYNC_WR_DV_REG(DOLBY_TOP1_PYWR_STRIDE7, dolby5_top1->py_stride[6]);

	VSYNC_WR_DV_REG(DOLBY_TOP1_WDMA_BADDR0, dolby5_top1->wdma_baddr);

	dolby5_top1_rdmif(core1_hsize,
		core1_vsize,
		bit_mode,
		fmt_mode,
		rdmif_baddr,
		rdmif_stride);

	VSYNC_WR_DV_REG_BITS(DOLBY_TOP1_CTRL0, vsync_sel, 24, 2);

	if (vsync_sel == 1)
		VSYNC_WR_DV_REG(DOLBY_TOP1_CTRL0,
			VSYNC_RD_DV_REG(DOLBY_TOP1_CTRL0) |
			(dolby5_top1->reg_frm_rst << 30) |
			(vsync_sel << 24));
}

static void dolby5_top2_ini(u32 core2_hsize,
	u32 core2_vsize,
	u32 py_baddr[7],
	u32 py_stride[7],
	u32 py_level,
	u64 *core2_ahb_baddr)
{
	int rdma_num = 2;/*fix*/
	int rdma1_num = 1;/*fix*/
	int rdma_size[12] = {166, 258};/*fix*/
	u32 *p_reg;
	int core2_ahb_num = TOP2_REG_NUM;

#if READ_REG_FROM_FILE
	p_reg = (u32 *)top2_reg_buf;
	core2_ahb_num = top2_reg_num;
#else
	p_reg = (u32 *)core2_ahb_baddr;
#endif
	dolby5_ahb_reg_config(p_reg, 2, core2_ahb_num);

	pr_dv_dbg("==== config core2_ahb_reg first before top reg! ====\n");

	VSYNC_WR_DV_REG(DOLBY_TOP2_PIC_SIZE, (core2_vsize << 16) | core2_hsize);
	VSYNC_WR_DV_REG_BITS(DOLBY_TOP2_RDMA_CTRL, rdma_num, 0, 5);
	VSYNC_WR_DV_REG_BITS(DOLBY_TOP2_RDMA_CTRL, rdma1_num, 8, 3);

	VSYNC_WR_DV_REG(DOLBY_TOP2_RDMA_SIZE0, (rdma_size[0] << 16) | rdma_size[1]);
	VSYNC_WR_DV_REG(DOLBY_TOP2_RDMA_SIZE1, (rdma_size[2] << 16) | rdma_size[3]);
	VSYNC_WR_DV_REG(DOLBY_TOP2_RDMA_SIZE2, (rdma_size[4] << 16) | rdma_size[5]);
	VSYNC_WR_DV_REG(DOLBY_TOP2_RDMA_SIZE3, (rdma_size[6] << 16) | rdma_size[7]);
	VSYNC_WR_DV_REG(DOLBY_TOP2_RDMA_SIZE4, (rdma_size[8] << 16) | rdma_size[9]);
	VSYNC_WR_DV_REG(DOLBY_TOP2_RDMA_SIZE5, (rdma_size[10] << 16) | rdma_size[11]);

	VSYNC_WR_DV_REG_BITS(DOLBY_TOP2_PYRD_CTRL, py_level, 0, 2);

	VSYNC_WR_DV_REG(DOLBY_TOP2_PYRD_BADDR1, py_baddr[0]);
	VSYNC_WR_DV_REG(DOLBY_TOP2_PYRD_BADDR2, py_baddr[1]);
	VSYNC_WR_DV_REG(DOLBY_TOP2_PYRD_BADDR3, py_baddr[2]);
	VSYNC_WR_DV_REG(DOLBY_TOP2_PYRD_BADDR4, py_baddr[3]);
	VSYNC_WR_DV_REG(DOLBY_TOP2_PYRD_BADDR5, py_baddr[4]);
	VSYNC_WR_DV_REG(DOLBY_TOP2_PYRD_BADDR6, py_baddr[5]);
	VSYNC_WR_DV_REG(DOLBY_TOP2_PYRD_BADDR7, py_baddr[6]);

	VSYNC_WR_DV_REG(DOLBY_TOP2_PYRD_STRIDE12, (py_stride[1] << 16) | py_stride[0]);
	VSYNC_WR_DV_REG(DOLBY_TOP2_PYRD_STRIDE34, (py_stride[3] << 16) | py_stride[2]);
	VSYNC_WR_DV_REG(DOLBY_TOP2_PYRD_STRIDE56, (py_stride[5] << 16) | py_stride[4]);
	VSYNC_WR_DV_REG(DOLBY_TOP2_PYRD_STRIDE7, py_stride[6]);

	pr_dv_dbg("==== don't forget config the pyramid_sequence_lut by ddr! ====\n");
}

void dolby5_mmu_config(u8 *baddr, int py_level)
{
	int num = py_level == SIX_LEVEL ? L6_MMU_NUM : L7_MMU_NUM;
	u32 mmu_lut;
	u32 lut0;
	u32 lut1;
	u32 lut2;
	u32 lut3;
	int i;

	VSYNC_WR_DV_REG(DOLBY_TOP2_PYRMIF_MMU_ADDR, 0);

	for (i = 0; i < (num + 7) >> 3; i = i + 1) {
		mmu_lut = 0;
		lut0 = (baddr[i * 8]) & 0x7;
		lut1 = (baddr[i * 8 + 1]) & 0x7;
		lut2 = (baddr[i * 8 + 2]) & 0x7;
		lut3 = (baddr[i * 8 + 3]) & 0x7;
		mmu_lut = (lut0 << 9) | (lut1 << 6) | (lut2 << 3) | lut3;
		if (i < 5)
			pr_dv_dbg("dolby mmu %x %x %x %x\n", lut0, lut1, lut2, lut3);

		lut0 = (baddr[i * 8 + 4]) & 0x7;
		lut1 = (baddr[i * 8 + 5]) & 0x7;
		lut2 = (baddr[i * 8 + 6]) & 0x7;
		lut3 = (baddr[i * 8 + 7]) & 0x7;
		mmu_lut = (((lut0 << 9) | (lut1 << 6) | (lut2 << 3) | lut3) << 12) | mmu_lut;

		if (i < 5) {
			pr_dv_dbg("dolby mmu %x %x %x %x\n", lut0, lut1, lut2, lut3);
			pr_dv_dbg("%x\n", mmu_lut);
		}
		VSYNC_WR_DV_REG(DOLBY_TOP2_PYRMIF_MMU_DATA, mmu_lut);
	}
}

#define OVLAP_HSIZE 96
static void dolby5_dpth_ctrl(u32 slice_num)
{
	enum top2_source core2_src = FROM_VD1;/*0:from vdin  1:from vd1 2:from di*/
	u32 ovlp_en = 0;/*1: ovlp enable 0:bypass*/
	u32 ovlp_ihsize = 1920;/*ovlp input hsize*/
	u32 ovlp_ivsize = 2160;/*ovlp input vsize*/
	u32 ovlp_ahsize = 32;/*ovlp added(ovlp) hsize, todo, need update according to vpp*/

	struct ovlp_win_para win0 = {0, 1920 + 96, 2160, 0, 1920 - 1, 0, 2160 - 1};
	struct ovlp_win_para win1 = {0, 1920 + 96, 2160, 96, 1920 - 1 + 96, 0, 2160 - 1};
	struct ovlp_win_para win2 = {0, 1920 + 32, 2160, 0, 1920 - 1 + 7, 0, 2160 - 1};
	struct ovlp_win_para win3 = {0, 1920 + 32, 2160, 32 - 9, 1920 + 32 - 1, 0, 2160 - 1};

	u32 vdin_p2s_hsize = 0;
	u32 vdin_p2s_vsize = 0;
	u32 vdin_s2p_hsize = 0;
	u32 vdin_s2p_vsize = 0;

	/*update from vpp*/
	u32 vd1_hsize = 3840;/*3840*/
	u32 vd1_vsize = 2160;/*2160*/
	u32 vd1_slice0_hsize = 1920 + 96;/*1920 + 96*/
	u32 vd1_slice0_vsize = 2160;/*2160*/
	u32 vd1_slice1_hsize = 1920 + 96;/*1920 + 96*/
	u32 vd1_slice1_vsize = 2160;/*2160*/
	u32 slice0_extra_size = 7;
	u32 slice1_extra_size = 9;
	u32 max_output_extra;
	u32 input_extra = OVLAP_HSIZE;
	u32 slice_en;

	slice_en = slice_num == 2 ? 0x3 : 0x1;
	ovlp_en = slice_num == 2 ? 0x1 : 0x0;

	if (debug_dolby & 1)
		pr_info("slice_num %d, size %d %d %d %d %d %d %d %d\n",
				slice_num, vd1_hsize, vd1_vsize,
				vd1_slice0_hsize, vd1_slice0_vsize,
				vd1_slice1_hsize, vd1_slice1_vsize,
				slice0_extra_size, slice1_extra_size);

	VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_CTRL, core2_src & 0x3, 0, 2);//reg_dv_out_sel
	VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_CTRL, core2_src & 0x3, 2, 2);//reg_dv_in_sel
	VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_CTRL, slice_en & 0x3, 12, 2);//reg_dv_in_hs_en
	VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_CTRL, slice_en & 0x3, 14, 2);//reg_dv_out_hs_en

	if (core2_src == FROM_VDIN0) {//vdin0
		//reg_dvpath_sel, 0:vind0 2ppc 1:vdin1 1ppc
		VSYNC_WR_DV_REG_BITS(VDIN_TOP_CTRL, 0, 20, 1);
		VSYNC_WR_DV_REG_BITS(VDIN0_CORE_CTRL, 1, 1, 1);//vdin0 reg_dolby_en
		VSYNC_WR_DV_REG_BITS(VDIN1_CORE_CTRL, 0, 1, 1);//vdin1 reg_dolby_en
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_CTRL, 96, 4,  8);//reg_vdin_p2s_ovlp_size
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_P2S, 1, 29, 2);//reg_vdin_p2s_mode  ppc->2slce
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_VDIN_CTRL, slice_en ^ 0x3, 8, 2);//dv_ppc_out
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_VDIN_CTRL, slice_en ^ 0x3, 0, 2);//dv_ppc_in
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_VDIN_CTRL, slice_en ^ 0x3, 4, 2);//dv_slc_out
	} else if (core2_src == FROM_VDIN1) {//vdin1
		//reg_dvpath_sel,0:vind0 2ppc 1:vdin1 1ppc
		VSYNC_WR_DV_REG_BITS(VDIN_TOP_CTRL, 1, 20, 1);
		VSYNC_WR_DV_REG_BITS(VDIN0_CORE_CTRL, 0, 1, 1);//vdin0 reg_dolby_en
		VSYNC_WR_DV_REG_BITS(VDIN1_CORE_CTRL, 1, 1, 1);//vdin1 reg_dolby_en
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_VDIN_CTRL, 1, 8, 2);//vdin1 reg_vdin_hs_ppc_out
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_VDIN_CTRL, 1, 6, 2);//reg_vdin_hs_slc_byp
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_VDIN_CTRL, 1, 0, 2);//reg_dv_ppc_in
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_VDIN_CTRL, 1, 2, 2);//reg_vdin_hs_byp_in
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_VDIN_CTRL, 1, 4, 2);//reg_dv_slc_out
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_CTRL, 0, 0, 2);//reg_dv_out_sel
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_CTRL, 3, 2, 2);//reg_dv_in_sel
	} else if (core2_src == FROM_VD1) { //vd1
		VSYNC_WR_DV_REG_BITS(T3X_VD1_S0_DV_BYPASS_CTRL, 1, 0, 1);//vd1_slice0 dv_en
		VSYNC_WR_DV_REG_BITS(T3X_VD1_S1_DV_BYPASS_CTRL, slice_num == 2 ? 1 : 0, 0, 1);
	} else if (core2_src == FROM_DI) {  //di
		//[1:0]inp_out_sel={[16]dvtv_di_sel,inp_2vdin_sel};
		//inp mif:0:to di_pre 1:to vdin 2:to dolby tv
		//[1:0]inp_in_sel={[16]dvtv_di_sel,vdin_2inp_sel};
		//pre current data : 0: mif 1:vdin 2:dolby tv
		VSYNC_WR_DV_REG_BITS(VIUB_MISC_CTRL0, 1, 16, 1);   //inp_mif(m0) to dv_slc0

		if (slice_num == 2) {
			//[4]: //0: wrmif0 input from nrwr   1:from dvs0_out
			VSYNC_WR_DV_REG_BITS(DI_TOP_CTRL1, 1, 4, 1);
			//[5]: //0: wrmif1 input from di_out 1:from dvs1_out
			VSYNC_WR_DV_REG_BITS(DI_TOP_CTRL1, 1, 5, 1);
			//[6]: //mem_mif(m2) to  0:nr 1:dv
			VSYNC_WR_DV_REG_BITS(DI_TOP_CTRL1, 1, 6, 1);
			//[7]: //dv_out_slc0 to  0:di 1:wrmif0
			VSYNC_WR_DV_REG_BITS(DI_TOP_CTRL1, 1, 7, 1);
			//post_frm_rst sel: 0:viu  1:internal 2:pre-post link, use pre timming
			VSYNC_WR_DV_REG_BITS(DI_TOP_POST_CTRL, 2, 30, 2);
		}
	} else {
		pr_dv_dbg("dolby5_path_ctrl: Error :  err config");
	}

	if (slice_num == 2) {
		if (vd1_slice0_hsize > vd1_hsize / 2) {
			input_extra = vd1_slice0_hsize - vd1_hsize / 2;
			if (input_extra != OVLAP_HSIZE)
				pr_info("please check input extra overlap %d\n", input_extra);
		}
		max_output_extra = slice1_extra_size > slice0_extra_size ?
							slice1_extra_size : slice0_extra_size;
		ovlp_ahsize = (max_output_extra + 15) / 16 * 16;/*align to 16*/

		ovlp_ihsize = vd1_hsize / 2;
		ovlp_ivsize = vd1_vsize;

		win0.ovlp_win_en = 1;
		win0.ovlp_win_hsize = vd1_slice0_hsize;
		win0.ovlp_win_vsize = vd1_slice0_vsize;
		win0.ovlp_win_hbgn = 0;
		win0.ovlp_win_hend = vd1_slice0_hsize - 1;
		win0.ovlp_win_vbgn = 0;
		win0.ovlp_win_vend = vd1_slice0_vsize - 1;

		win1.ovlp_win_en = 1;
		win1.ovlp_win_hsize = vd1_slice1_hsize;
		win1.ovlp_win_vsize = vd1_slice1_vsize;
		win1.ovlp_win_hbgn = input_extra;
		win1.ovlp_win_hend = vd1_hsize / 2 - 1 + win1.ovlp_win_hbgn;
		win1.ovlp_win_vbgn = 0;
		win1.ovlp_win_vend = vd1_slice1_vsize - 1;

		win2.ovlp_win_en = 1;
		win2.ovlp_win_hsize = vd1_hsize + ovlp_ahsize;
		win2.ovlp_win_vsize = vd1_slice0_vsize;
		win2.ovlp_win_hbgn = 0;
		win2.ovlp_win_hend = vd1_hsize - 1 + slice0_extra_size;
		win2.ovlp_win_vbgn = 0;
		win2.ovlp_win_vend = vd1_slice0_vsize - 1;

		win3.ovlp_win_en = 1;
		win3.ovlp_win_hsize = vd1_hsize + ovlp_ahsize;
		win3.ovlp_win_vsize = vd1_slice1_vsize;
		win3.ovlp_win_hbgn = ovlp_ahsize - slice1_extra_size;
		win3.ovlp_win_hend = vd1_hsize + ovlp_ahsize - 1;
		win3.ovlp_win_vbgn = 0;
		win3.ovlp_win_vend = vd1_slice1_vsize - 1;

		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_OVLP_SIZE,
			ovlp_ihsize & 0x1fff, 16, 13); //ovlp_ihsize
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_OVLP_SIZE,
			ovlp_ivsize & 0x1fff, 0, 13); //ovlp_ivsize

		//ovlp setting
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_OVLP,
		ovlp_en & 0x1, 31, 1);//reg_ovlp_en
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_OVLP,
			win0.ovlp_win_en & 0x1, 7, 1);//reg_ovlp_win0_en
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_OVLP,
			win1.ovlp_win_en & 0x1, 8, 1);//reg_ovlp_win1_en
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_OVLP,
			win2.ovlp_win_en & 0x1, 9, 1);//reg_ovlp_win2_en
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_OVLP,
			win3.ovlp_win_en & 0x1, 10, 1);//reg_ovlp_win3_en
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_OVLP,
			ovlp_ahsize & 0x1fff, 16, 13);//reg_ovlp_size}

		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_OVLP_WIN0_SIZE,
			win0.ovlp_win_hsize & 0x1fff, 16, 13);//ovlp_ivsize
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_OVLP_WIN0_SIZE,
			win0.ovlp_win_vsize & 0x1fff, 0, 13);//ovlp_ihsize
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_OVLP_WIN0_H,
			win0.ovlp_win_hbgn & 0x1fff, 16, 13);//ovlp_ivsize
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_OVLP_WIN0_H,
			win0.ovlp_win_hend & 0x1fff, 0, 13);//ovlp_ihsize
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_OVLP_WIN0_V,
			win0.ovlp_win_vbgn & 0x1fff, 16, 13);//ovlp_ivsize
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_OVLP_WIN0_V,
			win0.ovlp_win_vend & 0x1fff, 0, 13);//ovlp_ihsize

		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_OVLP_WIN1_SIZE,
			win1.ovlp_win_hsize & 0x1fff, 16, 13);
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_OVLP_WIN1_SIZE,
			win1.ovlp_win_vsize & 0x1fff, 0, 13);
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_OVLP_WIN1_H,
			win1.ovlp_win_hbgn & 0x1fff, 16, 13);
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_OVLP_WIN1_H,
			win1.ovlp_win_hend & 0x1fff, 0, 13);
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_OVLP_WIN1_V,
			win1.ovlp_win_vbgn & 0x1fff, 16, 13);
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_OVLP_WIN1_V,
			win1.ovlp_win_vend & 0x1fff, 0, 13);

		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_OVLP_WIN2_SIZE,
			win2.ovlp_win_hsize & 0x1fff, 16, 13);
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_OVLP_WIN2_SIZE,
			win2.ovlp_win_vsize & 0x1fff, 0, 13);
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_OVLP_WIN2_H,
			win2.ovlp_win_hbgn & 0x1fff, 16, 13);
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_OVLP_WIN2_H,
			win2.ovlp_win_hend & 0x1fff, 0, 13);
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_OVLP_WIN2_V,
			win2.ovlp_win_vbgn & 0x1fff, 16, 13);
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_OVLP_WIN2_V,
			win2.ovlp_win_vend & 0x1fff, 0, 13);

		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_OVLP_WIN3_SIZE,
			win3.ovlp_win_hsize & 0x1fff, 16, 13);
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_OVLP_WIN3_SIZE,
			win3.ovlp_win_vsize & 0x1fff, 0, 13);
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_OVLP_WIN3_H,
			win3.ovlp_win_hbgn & 0x1fff, 16, 13);
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_OVLP_WIN3_H,
			win3.ovlp_win_hend & 0x1fff, 0, 13);
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_OVLP_WIN3_V,
			win3.ovlp_win_vbgn & 0x1fff, 16, 13);
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_OVLP_WIN3_V,
			win3.ovlp_win_vend & 0x1fff, 0, 13);
	} else {
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_OVLP, 0, 31, 1);
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_OVLP, 0, 7, 1);
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_OVLP, 0, 8, 1);
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_OVLP, 0, 9, 1);
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_OVLP, 0, 10, 1);
	}

	if (core2_src == FROM_VDIN0 || core2_src == FROM_VDIN1) {//vdin
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_P2S, vdin_p2s_hsize & 0x1fff, 0, 13);
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_P2S, vdin_p2s_vsize & 0x1fff, 16, 13);

		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_S2P, vdin_s2p_hsize & 0x1fff, 0, 13);
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_S2P, vdin_s2p_vsize & 0x1fff, 16, 13);
	}
}

void enable_amdv_hw5(int enable)
{
	int gd_en = 0;

	if (debug_dolby & 8)
		pr_dv_dbg("enable %d, dv on %d, mode %d %d\n",
			  enable, dolby_vision_on, dolby_vision_mode,
			  get_amdv_target_mode());
	if (enable) {
		if (!dolby_vision_on) {
			set_amdv_wait_on();
			if (is_aml_t3x()) {
				update_dma_buf();
				if (!amdv_core1_on)
					set_frame_count(0);
				if ((amdv_mask & 1) &&
					amdv_setting_video_flag) {
					VSYNC_WR_DV_REG_BITS
						(T3X_VD1_S0_DV_BYPASS_CTRL,
						 1,
						 0, 1); /* top core2 enable */
					amdv_core1_on = true;
				} else {
					VSYNC_WR_DV_REG_BITS
						(T3X_VD1_S0_DV_BYPASS_CTRL,
						 0,
						 0, 1); /* top core2 disable */
					VSYNC_WR_DV_REG_BITS
						(T3X_VD1_S1_DV_BYPASS_CTRL,
						 0,
						 0, 1); /* top core2 disable */
					amdv_core1_on = false;
					VSYNC_WR_DV_REG(VPU_DOLBY_WRAP_GCLK, 0x55);
					//dv_mem_power_off(VPU_DOLBY0);
				}
			}
			if (dolby_vision_flags & FLAG_CERTIFICATION) {
				/* bypass dither/PPS/SR/CM, EO/OE */
				bypass_pps_sr_gamma_gainoff(3);
				/* bypass all video effect */
				video_effect_bypass(1);
				//WRITE_VPP_DV_REG(AMDV_TV_DIAG_CTRL, 0xb);//todo
			} else {
				/* bypass all video effect */
				if (dolby_vision_flags & FLAG_BYPASS_VPP)
					video_effect_bypass(1);
			}
			pr_info("DV TV core turn on\n");
			amdv_core1_on_cnt = 0;
		} else {
			if (!amdv_core1_on &&
				(amdv_mask & 1) &&
				amdv_setting_video_flag) {
				if (is_aml_t3x()) {
					/*enable core1a*/
					VSYNC_WR_DV_REG_BITS
						(T3X_VD1_S0_DV_BYPASS_CTRL,
						 1, 0, 1);
					hdr_vd1_off(VPP_TOP0);
				}
				amdv_core1_on = true;
				amdv_core1_on_cnt = 0;
				pr_dv_dbg("DV TV core turn on\n");
			} else if (amdv_core1_on &&
					   (!(amdv_mask & 1) ||
						!amdv_setting_video_flag)) {
				if (is_aml_t3x()) {
					/*disable tvcore*/
					VSYNC_WR_DV_REG_BITS
					(T3X_VD1_S0_DV_BYPASS_CTRL,
					 0, 0, 1);
					VSYNC_WR_DV_REG_BITS
					(T3X_VD1_S1_DV_BYPASS_CTRL,
					 0, 0, 1);
					VSYNC_WR_DV_REG(VPU_DOLBY_WRAP_GCLK, 0x55);//todo
					//dv_mem_power_off(VPU_DOLBY0);//todo
				}
				amdv_core1_on = false;
				amdv_core1_on_cnt = 0;
				set_frame_count(0);
				set_vf_crc_valid(0);
				pr_dv_dbg("DV core1 turn off\n");
			}
		}
		dolby_vision_on = true;
		clear_dolby_vision_wait();
		set_vsync_count(0);
	} else {
		if (dolby_vision_on) {
			if (is_aml_tvmode()) {
				if (is_aml_t3x()) {
					VSYNC_WR_DV_REG_BITS
					(T3X_VD1_S0_DV_BYPASS_CTRL,
					 0, 0, 1);
					VSYNC_WR_DV_REG_BITS
					(T3X_VD1_S1_DV_BYPASS_CTRL,
					 0, 0, 1);
					VSYNC_WR_DV_REG(VPU_DOLBY_WRAP_GCLK, 0x55);
					dv_mem_power_off(VPU_DOLBY0); //todo
					vpu_module_clk_disable(0, DV_TVCORE, 0);
				}
				if (p_funcs_tv) /* destroy ctx */
					p_funcs_tv->tv_control_path
						(FORMAT_INVALID, 0,
						NULL, 0,
						NULL, 0,
						0,	0,
						SIGNAL_RANGE_SMPTE,
						NULL, NULL,
						0,
						NULL,
						NULL,
						NULL, 0,
						NULL,
						NULL);
				pr_dv_dbg("DV TV core turn off\n");
				if (tv_dovi_setting)
					tv_dovi_setting->src_format =
					FORMAT_SDR;
			}
			video_effect_bypass(0);
		}
		set_vf_crc_valid(0);
		reset_dv_param();
		clear_dolby_vision_wait();
		if (!is_aml_gxm() && !is_aml_txlx()) {
			hdr_osd_off(VPP_TOP0);
			hdr_vd1_off(VPP_TOP0);
		}
		/*dv release control of pwm*/
		if (is_aml_tvmode())
			gd_en = 0;
	}
}

int tv_top1_set(void)
{
	/*first frame, not enable top2*/
	if (!dolby_vision_on)
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_CTRL, 1, 31, 1);//bypass dolby core2
	else
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_CTRL, 0, 31, 1);//enable dolby core2

	dolby5_top1_ini(&top1_type);//todo, locate here??
	VSYNC_WR_DV_REG_BITS(DOLBY_TOP1_RDMA_CTRL, 1, 30, 1);//open top1 rdma //todo

	return 0;
}

int tv_top2_set(u64 *reg_data,
			     int hsize,
			     int vsize,
			     int bl_enable,
			     int el_enable,
			     int el_41_mode,
			     int src_chroma_format,
			     bool hdmi,
			     bool hdr10,
			     bool reset)
{
	int composer_enable = el_enable;
	bool bypass_core1 = (!hsize || !vsize || !(amdv_mask & 1));
	bool core1_on_flag = amdv_core1_on;
	u32 py_baddr[7] = {top1_py_addr.top1_py0,
					   top1_py_addr.top1_py1,
					   top1_py_addr.top1_py2,
					   top1_py_addr.top1_py3,
					   top1_py_addr.top1_py4,
					   top1_py_addr.top1_py5,
					   top1_py_addr.top1_py6};

	u32 py_stride[7] = {128, 128, 128, 128, 128, 128, 128};
	u32 py_level = NO_LEVEL;/*todo*/
	static u32 last_py_level = NO_LEVEL;

	u32 vd1_slice0_hsize = hsize;/*1920 + 96, todo*/
	u32 vd1_slice0_vsize = vsize;/*2160*/
	u32 slice_num = 1;/*1 or 2, todo, need update from vpp*/

	if (dolby_vision_on &&
		(dolby_vision_flags & FLAG_DISABE_CORE_SETTING))
		return 0;

	if (is_aml_t3x()) {
		//VSYNC_WR_DV_REG_BITS(VPP_TOP_VTRL, 0, 0, 1); //AMDV TV select
		//enable tvcore clk
		if (!dolby_vision_on) {/*enable once*/
			vpu_module_clk_enable(0, DV_TVCORE, 1);
			vpu_module_clk_enable(0, DV_TVCORE, 0);
		}
	}

	if (is_aml_t3x()) {
		/* mempd for ipcore */
		/*if (get_dv_vpu_mem_power_status(VPU_DOLBY0) ==*/
		/*	VPU_MEM_POWER_DOWN ||*/
		/*	get_dv_mem_power_flag(VPU_DOLBY0) ==*/
		/*	VPU_MEM_POWER_DOWN)*/
		/*	dv_mem_power_on(VPU_DOLBY0);*/
		WRITE_VPP_DV_REG(VPU_DOLBY_WRAP_GCLK, 0);
	}
	if (reset) {
		amdv_core_reset(AMDV_TVCORE);
		VSYNC_WR_DV_REG(VPU_DOLBY_WRAP_GCLK, 0);
	}

	if (dolby_vision_flags & FLAG_DISABLE_COMPOSER)
		composer_enable = 0;

	/*t3x from ucode 0x152b*/
	VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_DTNL, 0x152b, 0, 18);
	VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_DTNL, hsize, 18, 13);

	if (hdmi && !hdr10) {
		/*hdmi DV STD and DV LL:  need detunnel*/
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_IRQ, 1, 18, 1);
	} else {
		VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_IRQ, 0, 18, 1);
	}

	/*if (src_chroma_format == 2)*/
	/*	VSYNC_WR_DV_REG_BITS(AMDV_TV_SWAP_CTRL6, 1, 29, 1);*/
	/*else if (src_chroma_format == 1)*/
	/*	VSYNC_WR_DV_REG_BITS(AMDV_TV_SWAP_CTRL6, 0, 29, 1);*/

	if (core1_on_flag &&
		!bypass_core1) {
		if (is_aml_t3x()) {
			VSYNC_WR_DV_REG_BITS
				(T3X_VD1_S0_DV_BYPASS_CTRL,
				 1, 0, 1);
			VSYNC_WR_DV_REG_BITS
				(T3X_VD1_S1_DV_BYPASS_CTRL,
				 slice_num == 2 ? 1 : 0, 0, 1);
		}
	} else if (core1_on_flag &&
		bypass_core1) {
		if (is_aml_t3x())
			VSYNC_WR_DV_REG_BITS
				(T3X_VD1_S0_DV_BYPASS_CTRL,
				 0, 0, 2);
	}

	dolby5_dpth_ctrl(slice_num);

	VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_CTRL, 0, 31, 1);//top2 enable
	VSYNC_WR_DV_REG_BITS(VPU_DOLBY_WRAP_IRQ, 2, 0, 2); //top2 int, pulse

	dolby5_top2_ini(vd1_slice0_hsize, vd1_slice0_vsize,
					py_baddr, py_stride, py_level, reg_data);

	//VSYNC_WR_DV_REG_BITS(DOLBY_TOP2_RDMA_CTRL, 1, 28, 1);//shadow_en
    //VSYNC_WR_DV_REG_BITS(DOLBY_TOP2_RDMA_CTRL, 1, 29, 1);//shadow_rst

	/*VPU_DOLBY_WRAP_GCLK bit16 reset total dolby core, need reload pyramid mmu*/
	if (last_py_level != py_level || reset) {
		VSYNC_WR_DV_REG_BITS(DOLBY_TOP2_PYRD_CTRL, 1, 30, 1);//pyramid mmu rst
		dolby5_mmu_config(pyrd_seq_lvl7, py_level);
	}
	last_py_level = py_level;

	VSYNC_WR_DV_REG_BITS(DOLBY_TOP2_RDMA_CTRL, 1, 30, 1);//open top2 rdma

	dma_lut_write();
	set_dovi_setting_update_flag(true);
	return 0;
}

int dma_lut_init(void)
{
	int ret;
	u32 channel = DV_CHAN;
	struct lut_dma_set_t lut_dma_set;

	lut_dma_set.channel = channel;
	lut_dma_set.dma_dir = LUT_DMA_WR;
	lut_dma_set.irq_source = VIU1_VSYNC;
	lut_dma_set.mode = LUT_DMA_MANUAL;
	lut_dma_set.table_size = lut_dma_info[0].dma_total_size;
	ret = lut_dma_register(&lut_dma_set);
	if (ret >= 0) {
		lut_dma_support = true;
	} else {
		pr_info("%s failed, not support\n", __func__);
		lut_dma_support = false;
	}
	return ret;
}

void dma_lut_uninit(void)
{
	u32 channel = DV_CHAN;

	lut_dma_unregister(LUT_DMA_WR, channel);
}

int dma_lut_write(void)
{
	int dma_size;
	ulong phy_addr;
	u32 channel = DV_CHAN;

	if (!lut_dma_support)
		return 0;

	if (enable_top1) {
		dma_size = lut_dma_info[0].dma_total_size;
		phy_addr = (ulong)(lut_dma_info[cur_dmabuf_id].dma_paddr);
	} else {
		dma_size = lut_dma_info[0].dma_top2_size;
		phy_addr = (ulong)(lut_dma_info[cur_dmabuf_id].dma_paddr_top2);
	}

	cur_dmabuf_id = cur_dmabuf_id ^ 1;
	lut_dma_write_phy_addr(channel,
			       phy_addr,
			       dma_size);
	return 1;
}

irqreturn_t amdv_isr(int irq, void *dev_id)
{
	/*1: top1, 2+3: top1+top2, 4+5: top3+top4*/
	u32 metadata0;
	u32 metadata1;

	isr_cnt++;
	if (debug_dolby & 0x1000)
		pr_info("isr_cnt %d\n", isr_cnt);

	if (isr_cnt % 2 == 1) {/*top1 done*/
		top1_done = true;
		metadata0 = READ_VPP_DV_REG(DOLBY5_CORE1_L1_MINMAX);
		metadata1 = READ_VPP_DV_REG(DOLBY5_CORE1_L1_MID_L4);
		l1l4_md[l1l4_wr_index][0] = metadata0 & 0xffff;
		l1l4_md[l1l4_wr_index][1] = metadata0 >> 16;
		l1l4_md[l1l4_wr_index][2] = metadata1 & 0xffff;
		l1l4_md[l1l4_wr_index][3] = metadata1 >> 16;
		l1l4_wr_index  = (l1l4_wr_index + 1) % L1L4_MD_COUNT;
	}

	return IRQ_HANDLED;
}

