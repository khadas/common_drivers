// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2021 Amlogic, Inc. All rights reserved.
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/amlogic/media/frame_provider/tvin/tvin.h>
#include <linux/amlogic/media/amvecm/hdr2_ext.h>
#include <linux/amlogic/media/vpu/vpu.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/amlogic/media/codec_mm/codec_mm.h>
#include <linux/amlogic/media/utils/vdec_reg.h>
#include <linux/highmem.h>
#include <linux/page-flags.h>
#include <linux/vmalloc.h>
#include <linux/dma-mapping.h>
#include <linux/dma-map-ops.h>
#include <linux/sched/clock.h>
#include <linux/amlogic/media/vout/vout_notify.h>
#include <linux/amlogic/media/video_sink/video.h>
#include <linux/amlogic/media/amdolbyvision/dolby_vision.h>
#include "../tvin_global.h"
#include "../tvin_format_table.h"
#include "../dsc_dec/dsc_dec_drv.h"
#include "vdin_ctl.h"
#include "vdin_regs.h"
#include "vdin_drv.h"
#include "vdin_vf.h"
#include "vdin_canvas.h"
#include "vdin_afbce.h"
#include "vdin_hdr.h"

#define VDIN_V_SHRINK_H_LIMIT 1280
#define TVIN_MAX_PIX_CLK 20000
#define META_RETRY_MAX 10
#define VDIN_MAX_H_ACTIVE 4096	/*the max h active of vdin*/
/*0: 1 word in 1burst, 1: 2 words in 1burst;
 *2: 4 words in 1burst;
 */
#define VDIN_WR_BURST_MODE 2

static bool rgb_info_enable;
static unsigned int rgb_info_x;
static unsigned int rgb_info_y;
static unsigned int rgb_info_r;
static unsigned int rgb_info_g;
static unsigned int rgb_info_b;
//static int vdin_det_idle_wait = 100;
static unsigned int delay_line_num;
//static bool invert_top_bot;
//static unsigned int vdin0_afbce_debug_force;
static unsigned int vdin_luma_max;

//static unsigned int vpu_reg_27af = 0x3;

//tmp
static bool cm_enable = 1;

/***************************Local defines**********************************/
#define BBAR_BLOCK_THR_FACTOR           3
#define BBAR_LINE_THR_FACTOR            7

#define VDIN_MUX_NULL                   0
#define VDIN_MUX_MPEG                   1
#define VDIN_MUX_656                    2
#define VDIN_MUX_TVFE                   3
#define VDIN_MUX_CVD2                   4
#define VDIN_MUX_HDMI                   5
#define VDIN_MUX_DVIN                   6
#define VDIN_MUX_VIU1_WB0		7
#define VDIN_MUX_MIPI                   8
#define VDIN_MUX_ISP			9
/* after g12a new add*/
#define VDIN_MUX_VIU1_WB1		9
#define VDIN_MUX_656_B                  10

#define VDIN_MAP_Y_G                    0
#define VDIN_MAP_BPB                    1
#define VDIN_MAP_RCR                    2

#define MEAS_MUX_NULL                   0
#define MEAS_MUX_656                    1
#define MEAS_MUX_TVFE                   2
#define MEAS_MUX_CVD2                   3
#define MEAS_MUX_HDMI                   4
#define MEAS_MUX_DVIN                   5
#define MEAS_MUX_DTV                    6
#define MEAS_MUX_ISP                    8
#define MEAS_MUX_656_B                  9
#define MEAS_MUX_VIU1                   6
#define MEAS_MUX_VIU2                   8

#define HDMI_DE_REPEAT_DONE_FLAG	0xF0
#define DECIMATION_REAL_RANGE		0x0F
#define VDIN_PIXEL_CLK_4K_30HZ		248832000
#define VDIN_PIXEL_CLK_4K_60HZ		530841600 //4096x2160x60

/***************************Local Structures**********************************/
//lht todo
static struct vdin_matrix_lup_s vdin_matrix_lup[] = {
	{0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x0400200, 0x00000200,},
	/* VDIN_MATRIX_RGB_YUV601 */
	/* 0     0.257  0.504  0.098     16 */
	/* 0    -0.148 -0.291  0.439    128 */
	/* 0     0.439 -0.368 -0.071    128 */
	{0x00000000, 0x00000000, 0x01070204, 0x00641f68, 0x1ed601c2, 0x01c21e87,
		0x00001fb7, 0x00400200, 0x00000200,},
	/* VDIN_MATRIX_GBR_YUV601 */
	/* 0	    0.504  0.098  0.257     16 */
	/* 0      -0.291  0.439 -0.148    128 */
	/* 0	   -0.368 -0.071  0.439    128 */
	{0x00000000, 0x00000000, 0x02040064, 0x01071ed6, 0x01c21f68, 0x1e871fb7,
		0x000001c2, 0x00400200, 0x00000200,},
	/* VDIN_MATRIX_BRG_YUV601 */
	/* 0	    0.098  0.257  0.504     16 */
	/* 0       0.439 -0.148 -0.291    128 */
	/* 0      -0.071  0.439 -0.368    128 */
	{0x00000000, 0x00000000, 0x00640107, 0x020401c2, 0x1f681ed6, 0x1fb701c2,
		0x00001e87, 0x00400200, 0x00000200,},
	/* VDIN_MATRIX_YUV601_RGB */
	/* -16     1.164  0      1.596      0 */
	/* -128     1.164 -0.391 -0.813      0 */
	/* -128     1.164  2.018  0          0 */
	{0x07c00600, 0x00000600, 0x04a80000, 0x066204a8, 0x1e701cbf, 0x04a80812,
		0x00000000, 0x00000000, 0x00000000,},
	/* VDIN_MATRIX_YUV601_GBR */
	/* -16     1.164 -0.391 -0.813      0 */
	/* -128     1.164  2.018  0	     0 */
	/* -128     1.164  0	  1.596      0 */
	{0x07c00600, 0x00000600, 0x04a81e70, 0x1cbf04a8, 0x08120000, 0x04a80000,
		0x00000662, 0x00000000, 0x00000000,},
	/* VDIN_MATRIX_YUV601_BRG */
	/* -16     1.164  2.018  0          0 */
	/* -128     1.164  0      1.596      0 */
	/* -128     1.164 -0.391 -0.813      0 */
	{0x07c00600, 0x00000600, 0x04a80812, 0x000004a8, 0x00000662, 0x04a81e70,
		0x00001cbf, 0x00000000, 0x00000000,},
	/* VDIN_MATRIX_RGB_YUV601F */
	/* 0     0.299  0.587  0.114      0 */
	/* 0    -0.169 -0.331  0.5      128 */
	/* 0     0.5   -0.419 -0.081    128 */
	{0x00000000, 0x00000000, 0x01320259, 0x00751f53, 0x1ead0200, 0x02001e53,
		0x00001fad, 0x00000200, 0x00000200,},
	/* VDIN_MATRIX_YUV601F_RGB */
	/* 0     1      0      1.402      0 */
	/* -128     1     -0.344 -0.714      0 */
	/* -128     1      1.772  0          0 */
	{0x00000600, 0x00000600, 0x04000000, 0x059c0400, 0x1ea01d25, 0x04000717,
		0x00000000, 0x00000000, 0x00000000,},
	/* VDIN_MATRIX_RGBS_YUV601 */
	/* -16     0.299  0.587  0.114     16 */
	/* -16    -0.173 -0.339  0.511    128 */
	/* -16     0.511 -0.429 -0.083    128 */
	{0x07c007c0, 0x000007c0, 0x01320259, 0x00751f4f, 0x1ea5020b, 0x020b1e49,
		0x00001fab, 0x00400200, 0x00000200,},
	/* VDIN_MATRIX_YUV601_RGBS */
	/* -16     1      0      1.371     16 */
	/* -128     1     -0.336 -0.698     16 */
	/* -128     1      1.733  0         16 */
	{0x07c00600, 0x00000600, 0x04000000, 0x057c0400, 0x1ea81d35, 0x040006ef,
		0x00000000, 0x00400040, 0x00000040,},
	/* VDIN_MATRIX_RGBS_YUV601F */
	/* -16     0.348  0.683  0.133      0 */
	/* -16    -0.197 -0.385  0.582    128 */
	/* -16     0.582 -0.488 -0.094    128 */
	{0x07c007c0, 0x000007c0, 0x016402bb, 0x00881f36, 0x1e760254, 0x02541e0c,
		0x00001fa0, 0x00000200, 0x00000200,},
	/* VDIN_MATRIX_YUV601F_RGBS */
	/* 0     0.859  0      1.204     16 */
	/* -128     0.859 -0.295 -0.613     16 */
	/* -128     0.859  1.522  0         16 */
	{0x00000600, 0x00000600, 0x03700000, 0x04d10370, 0x1ed21d8c, 0x03700617,
		0x00000000, 0x00400040, 0x00000040,},
	/* VDIN_MATRIX_YUV601F_YUV601 */
	/* 0     0.859  0      0         16 */
	/* -128     0      0.878  0        128 */
	/* -128     0      0      0.878    128 */
	{0x00000600, 0x00000600, 0x03700000, 0x00000000, 0x03830000, 0x00000000,
		0x00000383, 0x00400200, 0x00000200,},
	/* VDIN_MATRIX_YUV601_YUV601F */
	/* -16     1.164  0      0          0 */
	/* -128     0      1.138  0        128 */
	/* -128     0      0      1.138    128 */
	{0x07c00600, 0x00000600, 0x04a80000, 0x00000000, 0x048d0000, 0x00000000,
		0x0000048d, 0x00000200, 0x00000200,},
	/* VDIN_MATRIX_RGB_YUV709 */
	/* 0     0.183  0.614  0.062     16 */
	/* 0    -0.101 -0.338  0.439    128 */
	/* 0     0.439 -0.399 -0.04     128 */
	{0x00000000, 0x00000000, 0x00bb0275, 0x003f1f99, 0x1ea601c2, 0x01c21e67,
		0x00001fd7, 0x00400200, 0x00000200,},
	/* VDIN_MATRIX_YUV709_RGB */
	/* -16     1.164  0      1.793      0 */
	/* -128     1.164 -0.213 -0.534      0 */
	/* -128     1.164  2.115  0          0 */
	{0x07c00600, 0x00000600, 0x04a80000, 0x072c04a8, 0x1f261ddd, 0x04a80876,
		0x00000000, 0x00000000, 0x00000000,},
	/* VDIN_MATRIX_YUV709_GBR */
	/* -16	1.164 -0.213 -0.534	 0 */
	/* -128	1.164  2.115  0	 0 */
	/* -128	1.164  0      1.793	 0 */
	{0x07c00600, 0x00000600, 0x04a81f26, 0x1ddd04a8, 0x08760000, 0x04a80000,
		0x0000072c, 0x00000000, 0x00000000,},
	/* VDIN_MATRIX_YUV709_BRG */
	/* -16	1.164  2.115  0	 0 */
	/* -128	1.164  0      1.793	 0 */
	/* -128	1.164 -0.213 -0.534	 0 */
	{0x07c00600, 0x00000600, 0x04a80876, 0x000004a8, 0x0000072c, 0x04a81f26,
		0x00001ddd, 0x00000000, 0x00000000,},
	/* VDIN_MATRIX_RGB_YUV709F */
	/* 0     0.213  0.715  0.072      0 */
	/* 0    -0.115 -0.385  0.5      128 */
	/* 0     0.5   -0.454 -0.046    128 */
	{0x00000000, 0x00000000, 0x00da02dc, 0x004a1f8a, 0x1e760200, 0x02001e2f,
		0x00001fd1, 0x00000200, 0x00000200,},
	/* VDIN_MATRIX_YUV709F_RGB */
	/* 0     1      0      1.575      0 */
	/* -128     1     -0.187 -0.468      0 */
	/* -128     1      1.856  0          0 */
	{0x00000600, 0x00000600, 0x04000000, 0x064d0400, 0x1f411e21, 0x0400076d,
		0x00000000, 0x00000000, 0x00000000,},
	/* VDIN_MATRIX_RGBS_YUV709 */
	/* -16     0.213  0.715  0.072     16 */
	/* -16    -0.118 -0.394  0.511    128 */
	/* -16     0.511 -0.464 -0.047    128 */
	{0x07c007c0, 0x000007c0, 0x00da02dc, 0x004a1f87, 0x1e6d020b, 0x020b1e25,
		0x00001fd0, 0x00400200, 0x00000200,},
	/* VDIN_MATRIX_YUV709_RGBS */
	/* -16     1      0      1.54      16 */
	/* -128     1     -0.183 -0.459     16 */
	/* -128     1      1.816  0         16 */
	{0x07c00600, 0x00000600, 0x04000000, 0x06290400, 0x1f451e2a, 0x04000744,
		0x00000000, 0x00400040, 0x00000040,},
	/* VDIN_MATRIX_RGBS_YUV709F */
	/* -16     0.248  0.833  0.084      0 */
	/* -16    -0.134 -0.448  0.582    128 */
	/* -16     0.582 -0.529 -0.054    128 */
	{0x07c007c0, 0x000007c0, 0x00fe0355, 0x00561f77, 0x1e350254, 0x02541de2,
		0x00001fc9, 0x00000200, 0x00000200,},
	/* VDIN_MATRIX_YUV709F_RGBS */
	/* 0     0.859  0      1.353     16 */
	/* -128     0.859 -0.161 -0.402     16 */
	/* -128     0.859  1.594  0         16 */
	{0x00000600, 0x00000600, 0x03700000, 0x05690370, 0x1f5b1e64, 0x03700660,
		0x00000000, 0x00400040, 0x00000040,},
	/* VDIN_MATRIX_YUV709F_YUV709 */
	/* 0     0.859  0      0         16 */
	/* -128     0      0.878  0        128 */
	/* -128     0      0      0.878    128 */
	{0x00000600, 0x00000600, 0x03700000, 0x00000000, 0x03830000, 0x00000000,
		0x00000383, 0x00400200, 0x00000200,},
	/* VDIN_MATRIX_YUV709_YUV709F */
	/* -16     1.164  0      0          0 */
	/* -128     0      1.138  0        128 */
	/* -128     0      0      1.138    128 */
	{0x07c00600, 0x00000600, 0x04a80000, 0x00000000, 0x048d0000, 0x00000000,
		0x0000048d, 0x00000200, 0x00000200,},
	/* VDIN_MATRIX_YUV601_YUV709 */
	/* -16     1     -0.115 -0.207     16 */
	/* -128     0      1.018  0.114    128 */
	/* -128     0      0.075  1.025    128 */
	{0x07c00600, 0x00000600, 0x04001f8a, 0x1f2c0000, 0x04120075, 0x0000004d,
		0x0000041a, 0x00400200, 0x00000200,},
	/* VDIN_MATRIX_YUV709_YUV601 */
	/* -16     1      0.100  0.192     16 */
	/* -128     0      0.990 -0.110    128 */
	/* -128     0     -0.072  0.984    128 */
	{0x07c00600, 0x00000600, 0x04000066, 0x00c50000, 0x03f61f8f, 0x00001fb6,
		0x000003f0, 0x00400200, 0x00000200,},
	/* VDIN_MATRIX_YUV601_YUV709F */
	/* -16     1.164 -0.134 -0.241      0 */
	/* -128     0      1.160  0.129    128 */
	/* -128     0      0.085  1.167    128 */
	{0x07c00600, 0x00000600, 0x04a81f77, 0x1f090000, 0x04a40084, 0x00000057,
		0x000004ab, 0x00000200, 0x00000200,},
	/* VDIN_MATRIX_YUV709F_YUV601 */
	/* 0     0.859  0.088  0.169     16 */
	/* -128     0      0.869 -0.097    128 */
	/* -128     0     -0.063  0.864    128 */
	{0x00000600, 0x00000600, 0x0370005a, 0x00ad0000, 0x037a1f9d, 0x00001fbf,
		0x00000375, 0x00400200, 0x00000200,},
	/* VDIN_MATRIX_YUV601F_YUV709 */
	/* 0     0.859 -0.101 -0.182     16 */
	/* -128     0      0.894  0.100    128 */
	/* -128     0      0.066  0.900    128 */
	{0x00000600, 0x00000600, 0x03701f99, 0x1f460000, 0x03930066, 0x00000044,
		0x0000039a, 0x00400200, 0x00000200,},
	/* VDIN_MATRIX_YUV709_YUV601F */
	/* -16     1.164  0.116  0.223      0 */
	/* -128     0      1.128 -0.126    128 */
	/* -128     0     -0.082  1.120    128 */
	{0x07c00600, 0x00000600, 0x04a80077, 0x00e40000, 0x04831f7f, 0x00001fac,
		0x0000047b, 0x00000200, 0x00000200,},
	/* VDIN_MATRIX_YUV601F_YUV709F */
	/* 0     1     -0.118 -0.212     16 */
	/* -128     0      1.018  0.114    128 */
	/* -128     0      0.075  1.025    128 */
	{0x00000600, 0x00000600, 0x04001f87, 0x1f270000, 0x04120075, 0x0000004d,
		0x0000041a, 0x00400200, 0x00000200,},
	/* VDIN_MATRIX_YUV709F_YUV601F */
	/* 0     1      0.102  0.196      0 */
	/* -128     0      0.990 -0.111    128 */
	/* -128     0     -0.072  0.984    128 */
	{0x00000600, 0x00000600, 0x04000068, 0x00c90000, 0x03f61f8e, 0x00001fb6,
		0x000003f0, 0x00000200, 0x00000200,},
	/* VDIN_MATRIX_RGBS_RGB */
	/* -16     1.164  0      0          0 */
	/* -16     0      1.164  0          0 */
	/* -16     0      0      1.164      0 */
	{0x07c007c0, 0x000007c0, 0x04a80000, 0x00000000, 0x04a80000, 0x00000000,
		0x000004a8, 0x00000000, 0x00000000,},
	/* VDIN_MATRIX_RGB_RGBS */
	/* 0     0.859  0      0         16 */
	/* 0     0      0.859  0         16 */
	/* 0     0      0      0.859     16 */
	{0x00000000, 0x00000000, 0x03700000, 0x00000000, 0x03700000, 0x00000000,
		0x00000370, 0x00400040, 0x00000040,},
	/* VDIN_MATRIX_2020RGB_YUV2020 */
	/* 0	 0.224732	0.580008  0.050729	 16 */
	/* 0	-0.122176 -0.315324  0.437500	128 */
	/* 0	 0.437500 -0.402312 -0.035188	128 */
	{0x00000000, 0x00000000, 0x00e60252, 0x00341f84, 0x1ebe01c0, 0x01c01e65,
		0x00001fdd, 0x00400200, 0x00000200,},
	/* VDIN_MATRIX_YUV2020F_YUV2020 */
	/* 0 0.859 0 0 16 */
	/* -128 0 0.878 0 128 */
	/* -128 0 0 0.878 128 */
	{0x00000600, 0x00000600, 0x03700000, 0x00000000, 0x03830000, 0x00000000,
		0x00000383, 0x00400200, 0x00000200,},
};

/***************************Local function**********************************/

/*get prob of r/g/b
 *	r 9:0
 *	g 19:10
 *	b 29:20
 */
void vdin_prob_get_rgb_t3x(unsigned int offset,
		       unsigned int *r, unsigned int *g, unsigned int *b)
{
	*b = rgb_info_b = rd_bits(offset, VDIN0_MAT_PROBE_COLOR,
				  COMPONENT2_PROBE_COLOR_BIT,
				  COMPONENT0_PROBE_COLOR_WID);
	*g = rgb_info_g = rd_bits(offset, VDIN0_MAT_PROBE_COLOR,
				  COMPONENT1_PROBE_COLOR_BIT,
				  COMPONENT1_PROBE_COLOR_WID);
	*r = rgb_info_r = rd_bits(offset, VDIN0_MAT_PROBE_COLOR,
				  COMPONENT0_PROBE_COLOR_BIT,
				  COMPONENT0_PROBE_COLOR_WID);
}

void vdin_prob_get_yuv_t3x(unsigned int offset,
		       unsigned int *rgb_yuv0,
		       unsigned int *rgb_yuv1,
		       unsigned int *rgb_yuv2)
{
	*rgb_yuv0 = ((rd_bits(offset, VDIN0_MAT_PROBE_COLOR,
			      COMPONENT2_PROBE_COLOR_BIT,
			      COMPONENT2_PROBE_COLOR_WID) << 8) >> 10);
	*rgb_yuv1 = ((rd_bits(offset, VDIN0_MAT_PROBE_COLOR,
			      COMPONENT1_PROBE_COLOR_BIT,
			      COMPONENT1_PROBE_COLOR_WID) << 8) >> 10);
	*rgb_yuv2 = ((rd_bits(offset, VDIN0_MAT_PROBE_COLOR,
			      COMPONENT0_PROBE_COLOR_BIT,
			      COMPONENT0_PROBE_COLOR_WID) << 8) >> 10);
}

void vdin_prob_set_before_or_after_mat_t3x(unsigned int offset,
				       unsigned int x, struct vdin_dev_s *devp)
{
	if (x != 0 && x != 1) //?
		return;
	/* 1:probe pixel data after matrix */
	wr_bits(offset, VDIN0_MAT_CTRL, x, 1, 1);
}

void vdin_prob_matrix_sel_t3x(unsigned int offset,
			  unsigned int sel, struct vdin_dev_s *devp)
{
	unsigned int x;

	x = sel & 0x03;
	/* 1:select matrix 1 */
	wr_bits(offset, VDIN0_MAT_CTRL, x, 1, 1);
}

/* this function set flowing parameters:
 *a.rgb_info_x	b.rgb_info_y
 *debug usage:
 *echo rgb_xy x y > /sys/class/vdin/vdinx/attr
 */
void vdin_prob_set_xy_t3x(unsigned int offset,
		      unsigned int x, unsigned int y, struct vdin_dev_s *devp)
{
	/* set position */
	rgb_info_x = x;
	if (devp->fmt_info_p->scan_mode == TVIN_SCAN_MODE_INTERLACED)
		rgb_info_y = y / 2;
	else
		rgb_info_y = y;

	/* #if defined(VDIN_V1) */
	wr_bits(offset, VDIN0_MAT_PROBE_POS, rgb_info_y,
		PROBE_POX_Y_BIT, PROBE_POX_Y_WID);
	wr_bits(offset, VDIN0_MAT_PROBE_POS, rgb_info_x,
		PROBE_POS_X_BIT, PROBE_POS_X_WID);
}

/*function:
 *	1.set meas mux based on port_:
 *		0x01: /mpeg/			0x10: /CVBS/
 *		0x02: /bt656/			0x20: /SVIDEO/
 *		0x04: /VGA/				0x40: /hdmi/
 *		0x08: /COMPONENT/		0x80: /dvin/
 *		0xc0:/viu/				0x100:/dtv mipi/
 *		0x200:/isp/
 *	2.set VDIN_MEAS in accumulation mode
 *	3.set VPP_VDO_MEAS in accumulation mode
 *	4.set VPP_MEAS in latch-on-falling-edge mode
 *	5.set VDIN_MEAS mux
 *	6.manual reset VDIN_MEAS & VPP_VDO_MEAS at the same time
 */
static void vdin_set_meas_mux_t3x(struct vdin_dev_s *devp)
{
	unsigned int meas_mux = MEAS_MUX_NULL;
	unsigned int wide_en_bit;

	if (devp->index)
		return;

	switch (devp->parm.port >> 8) {
	case 0x01: /* mpeg */
		wide_en_bit = 27;
		meas_mux = VDIN_VDI0_MPEG_T3X;
		break;
	case 0x02: /* first bt656 */
		wide_en_bit = 26;
		meas_mux = VDIN_VDI1_BT656_T3X;
		break;
	case 0x04: /* reserved */
		wide_en_bit = 25;
		meas_mux = VDIN_VDI2_RESERVED_T3X;
		break;
	case 0x10: /* CVBS */
		wide_en_bit = 24;
		meas_mux = VDIN_VDI3_TV_DECODE_IN_T3X;
		break;
	case 0x20: /* hdmi rx vdi4b */
		wide_en_bit = 22;
		meas_mux = VDIN_VDI4B_HDMIRX_T3X;
		break;
	case 0x40:
		wide_en_bit = 23;
		meas_mux = VDIN_VDI4A_HDMIRX_T3X;
		break;
	case 0x80: /* digital video */
		wide_en_bit = 21;
		meas_mux = VDIN_VDI5_DVI_T3X;
		break;
	case 0xa0:/* viu1 */
		wide_en_bit = 18;
		meas_mux = VDIN_VDI8_LOOPBACK_2_T3X;
		break;
	case 0xc0: /* viu2 */
		wide_en_bit = 20;
		meas_mux = VDIN_VDI6_LOOPBACK_1_T3X;
		break;
	case 0xe0: /* venc0 */
		wide_en_bit = 20;
		meas_mux = VDIN_VDI6_LOOPBACK_1_T3X;
		break;
	case 0x100: /* mipi-csi2 */
		wide_en_bit = 19;
		meas_mux = VDIN_VDI7_MIPI_CSI2_T3X;
		break;
	default:
		wide_en_bit = 23;
		meas_mux = VDIN_VDI4A_HDMIRX_T3X;
		break;
	}

	/* set VDIN_MEAS in accumulation mode */
	wr_bits(0, VDIN_INTF_MEAS_CTRL, 1,
		MEAS_VS_TOTAL_CNT_EN_BIT, MEAS_VS_TOTAL_CNT_EN_WID);
	/* set VDIN_MEAS mux */
	wr_bits(0, VDIN_INTF_MEAS_CTRL, meas_mux, 12, 4);
	wr_bits(0, VDIN_INTF_MEAS_CTRL, 1, wide_en_bit, 1);
	/* manual reset VDIN_MEAS,
	 * rst = 1 & 0
	 */
	wr_bits(0, VDIN_INTF_MEAS_CTRL, 1, 29, 1);
	wr_bits(0, VDIN_INTF_MEAS_CTRL, 0, 29, 1);
}

/*function:set VDIN_COM_CTRL0
 *Bit 3:0 vdin selection,
 *	1: mpeg_in from dram, 2: bt656 input,3: component input
 *	4: tv decoder input, 5: hdmi rx input,6: digital video input,
 *	7: loopback from Viu1, 8: MIPI.
 */
/* Bit 7:6, component0 output switch,
 *	00: select component0 in,01: select component1 in,
 *	10: select component2 in
 */
/* Bit 9:8, component1 output switch,
 *	00: select component0 in,
 *	01: select component1 in, 10: select component2 in
 */
/* Bit 11:10, component2 output switch,
 *	00: select component0 in, 01: select component1 in,
 *	10: select component2 in
 */

/* attention:new add for bt656
 * 0x02: /bt656/
	a.BT_PATH_GPIO:	gxl & gxm & g12a
	b.BT_PATH_GPIO_B:gxtvbb & gxbb
	c.txl and txlx don't support bt656
 */
void vdin_set_top_t3x(struct vdin_dev_s *devp, enum tvin_port_e port,
		  enum tvin_color_fmt_e input_cfmt, enum bt_path_e bt_path)
{
	unsigned int offset = devp->addr_offset;
	unsigned int vdin_mux = VDIN_MUX_NULL;
	unsigned int vdi_size = 0;
	unsigned int vdin_data_bus_0 = VDIN_MAP_Y_G;
	unsigned int vdin_data_bus_1 = VDIN_MAP_BPB;
	unsigned int vdin_data_bus_2 = VDIN_MAP_RCR;

	pr_info("%s %d:port:%#x,input_cfmt:%d\n",
		__func__, __LINE__, port, input_cfmt);

	vdi_size = (devp->h_active_org << 16) | (devp->v_active_org << 0);
	switch (port >> 8) {
	case 0x01: /* mpeg */
		vdin_mux = VDIN_VDI0_MPEG_T3X;
		wr(0, VDIN_INTF_VDI0_CTRL, 0xe4);
		wr(0, VDIN_INTF_VDI0_SIZE, vdi_size);
		break;
	case 0x02: /* first bt656 */
		vdin_mux = VDIN_VDI1_BT656_T3X;
		wr(0, VDIN_INTF_VDI1_CTRL, 0xe4);
		wr(0, VDIN_INTF_VDI1_SIZE, vdi_size);
		break;
	case 0x04: /* reserved */
		vdin_mux = VDIN_VDI2_RESERVED_T3X;
		wr(0, VDIN_INTF_VDI2_CTRL, 0xe4);
		wr(0, VDIN_INTF_VDI2_SIZE, vdi_size);
		break;
	case 0x10: /* CVBS */
		vdin_mux = VDIN_VDI3_TV_DECODE_IN_T3X;
		wr(0, VDIN_INTF_VDI3_CTRL, 0xe4);
		wr(0, VDIN_INTF_VDI3_SIZE, vdi_size);
		break;
	case 0x20: /* hdmi rx vdi4b */
		vdin_mux = VDIN_VDI4B_HDMIRX_T3X;
		wr(0, VDIN_INTF_VDI4B_CTRL, 0xe4);
		wr(0, VDIN_INTF_VDI4B_SIZE, vdi_size);
		break;
	case 0x40: /* hdmi rx hdmi0~1,vp core0--->vdi4b */
		if (devp->index) { //4b--->vdin1
		//if (port == TVIN_PORT_HDMI0 || port == TVIN_PORT_HDMI1) {
			vdin_mux = VDIN_VDI4B_HDMIRX_T3X;
			wr(0, VDIN_INTF_VDI4B_CTRL, 0xe4);
			wr(0, VDIN_INTF_VDI4B_SIZE, vdi_size);
		} else { //4a--->vdin0
		/* hdmi rx hdmi2~3,vp core1--->vdi4a */
			vdin_mux = VDIN_VDI4A_HDMIRX_T3X;
			wr(0, VDIN_INTF_VDI4A_CTRL, 0xe4);
			wr(0, VDIN_INTF_VDI4A_SIZE, vdi_size);
		}
		vdin_data_bus_0 = VDIN_MAP_RCR;
		vdin_data_bus_1 = VDIN_MAP_Y_G;
		vdin_data_bus_2 = VDIN_MAP_BPB;
		break;
	case 0x80: /* digital video */
		vdin_mux = VDIN_VDI5_DVI_T3X;
		wr(0, VDIN_INTF_VDI5_CTRL, 0xe4);
		wr(0, VDIN_INTF_VDI5_SIZE, vdi_size);
		break;
	case 0xa0:/* viu1 */
		if (port >= TVIN_PORT_VIU1_WB0_VD1 && port <= TVIN_PORT_VIU1_WB0_POST_BLEND) {
			vdin_mux = VDIN_VDI6_LOOPBACK_1_T3X;
			wr(0, VDIN_INTF_VDI6_CTRL, 0xe4);
			wr(0, VDIN_INTF_VDI6_SIZE, vdi_size);
		} else if ((port >= TVIN_PORT_VIU1_WB1_VD1) &&
			   (port <= TVIN_PORT_VIU1_WB1_POST_BLEND)) {
			vdin_mux = VDIN_VDI8_LOOPBACK_2_T3X;
			wr(0, VDIN_INTF_VDI8_CTRL, 0xe4);
			wr(0, VDIN_INTF_VDI8_SIZE, vdi_size);
		}
		break;
	case 0xc0: /* viu2 */
		//vdin_mux = VDIN_MUX_VIU1_WB1;
		//wr(offset, VDIN_IF_TOP_VDI9_CTRL, 0xe4);
		break;
	case 0xe0: /* venc0 */
		vdin_mux = VDIN_VDI6_LOOPBACK_1_T3X;
		wr(0, VDIN_INTF_VDI6_CTRL, 0xf4);
		wr(0, VDIN_INTF_VDI6_SIZE, vdi_size);
		break;
	case 0x100: /* mipi-csi2 */
		vdin_mux = VDIN_VDI7_MIPI_CSI2_T3X;
		wr(0, VDIN_INTF_VDI7_CTRL, 0xf4);
		wr(0, VDIN_INTF_VDI7_SIZE, vdi_size);
		break;
	default:
		vdin_mux = VDIN_VDI_NULL_T3X;
		break;
	}

	switch (input_cfmt) {
	case TVIN_YVYU422:
		vdin_data_bus_1 = VDIN_MAP_RCR;
		vdin_data_bus_2 = VDIN_MAP_BPB;
		break;
	case TVIN_UYVY422:
		vdin_data_bus_0 = VDIN_MAP_BPB;
		vdin_data_bus_1 = VDIN_MAP_RCR;
		vdin_data_bus_2 = VDIN_MAP_Y_G;
		break;
	case TVIN_VYUY422:
		vdin_data_bus_0 = VDIN_MAP_BPB;
		vdin_data_bus_1 = VDIN_MAP_RCR;
		vdin_data_bus_2 = VDIN_MAP_Y_G;
		break;
	case TVIN_YUV444:
	case TVIN_YUV422:
	case TVIN_YUV420:
		vdin_data_bus_0 = VDIN_MAP_Y_G;
		vdin_data_bus_1 = VDIN_MAP_BPB;
		vdin_data_bus_2 = VDIN_MAP_RCR;
		break;
	case TVIN_RGB444:
		/*RGB mapping*/
		if (devp->set_canvas_manual == 1) {
			vdin_data_bus_0 = VDIN_MAP_RCR;
			vdin_data_bus_1 = VDIN_MAP_BPB;
			vdin_data_bus_2 = VDIN_MAP_Y_G;
		} else {
			vdin_data_bus_0 = VDIN_MAP_Y_G;
			vdin_data_bus_1 = VDIN_MAP_BPB;
			vdin_data_bus_2 = VDIN_MAP_RCR;
		}
		break;
	default:
		break;
	}
	if (vdin_dv_is_need_tunnel(devp)) {
		vdin_data_bus_0 = VDIN_MAP_Y_G;
		vdin_data_bus_1 = VDIN_MAP_BPB;
		vdin_data_bus_2 = VDIN_MAP_RCR;
	}
	pr_info("%s %d:vdin_mux:%d\n", __func__, __LINE__, vdin_mux);

	wr_bits(offset, VDIN0_CUTWIN_CTRL, vdin_data_bus_0, 0, 2);
	wr_bits(offset, VDIN0_CUTWIN_CTRL, vdin_data_bus_1, 2, 2);
	wr_bits(offset, VDIN0_CUTWIN_CTRL, vdin_data_bus_2, 4, 2);

	wr(offset, VDIN0_CORE_SIZE, devp->h_active_org << 16 | devp->v_active_org);
	wr(offset, VDIN0_PP_IN_SIZE, devp->h_active_org << 16 | devp->v_active_org);
	wr(offset, VDIN0_PP_OUT_SIZE, devp->h_active << 16 | devp->v_active);

	wr_bits(devp->index * VDIN_TOP_OFFSET, VDIN0_SYNC_CONVERT_SECURE_CTRL, vdin_mux, 0, 3);
	wr_bits(devp->index * VDIN_TOP_OFFSET, VDIN0_SYNC_CONVERT_CTRL, 1, 0, 1); /* reg_enable */
	/* vdin_preproc */
	wr(0, VPU_VDIN_HDMI0_CTRL0,
		(1 << 0) | /* reg_hdmi_en */
		(0 << 2) | /* reg_in_ppc,hdmi rx */
		(1 << 4) | /* reg_out_ppc,2ppc */
		(2 << 6));/* reg_dsc_ppc,444 */
	wr(0, VPU_VDIN_HDMI0_CTRL1,
		(0 << 0) |  /* xxx */
		(100 << 8));/* reg_rdwin_auto */
	wr(0, VPU_VDIN_HDMI1_CTRL1,
		(0 << 0) |  /* xxx */
		(100 << 8));/* reg_rdwin_auto */
	if (devp->h_skip_en)
		wr_bits(0, VPU_VDIN_HDMI0_CTRL1, 1, 4, 2); /* reg_hskip_mode */
	if (devp->v_skip_en)
		wr_bits(0, VPU_VDIN_HDMI0_CTRL1, 1, 7, 1); /* reg_vskip_en */
	if (devp->v_active >= 4320)
		wr_bits(0, VPU_VDIN_HDMI0_CTRL1, 0, 30, 1);
	else
		wr_bits(0, VPU_VDIN_HDMI0_CTRL1, devp->pre_prop.up_sample_en, 30, 1);

	vdin_set_scl_mode_t3x(devp, devp->dv_hw5.hw5_ctl & BIT0);
}

/*this function will set the bellow parameters of devp:
 *1.h_active
 *2.v_active
 */
void vdin_set_decimation_t3x(struct vdin_dev_s *devp)
{
	//unsigned int offset = devp->addr_offset;
	unsigned int new_clk = 0;
	bool decimation_in_frontend = false;

	if (devp->prop.decimation_ratio & HDMI_DE_REPEAT_DONE_FLAG) {
		decimation_in_frontend = true;
		if (vdin_ctl_dbg)
			pr_info("decimation_in_frontend\n");
	}
	devp->prop.decimation_ratio = devp->prop.decimation_ratio &
			DECIMATION_REAL_RANGE;

	new_clk = devp->fmt_info_p->pixel_clk /
			(devp->prop.decimation_ratio + 1);
	devp->h_active = devp->fmt_info_p->h_active /
			(devp->prop.decimation_ratio + 1);
	devp->v_active = devp->fmt_info_p->v_active;

	devp->h_skip_en = false;
	devp->v_skip_en = false;
	/* if exceed the maximum processing capacity,skipping in preproc */
	if (devp->h_active > VDIN_LITE_CORE_MAX_W &&
		devp->prop.color_format != TVIN_YUV422) {
		devp->h_active = devp->h_active / 2;
		if (devp->prop.color_format != TVIN_YUV420)
			devp->h_skip_en = true;
	}
	if (devp->v_active > VDIN_LITE_CORE_MAX_H &&
	   (devp->prop.color_format == TVIN_RGB444 ||
	    devp->prop.color_format == TVIN_YUV444 ||
		devp->prop.color_format == TVIN_YUV422)) {
		devp->v_active = devp->v_active / 2;
		devp->v_skip_en = true;
	}

	if (vdin_ctl_dbg)
		pr_info("%s decimation_ratio=%u,new_clk=%u.h:%d,v:%d\n",
			__func__, devp->prop.decimation_ratio, new_clk,
			devp->h_active, devp->v_active);

	if (devp->prop.decimation_ratio && !decimation_in_frontend)	{
		//VDIN_INTF_VDI4A_CTRL
//		/* ratio */
//		wr_bits(offset, VDIN_ASFIFO_CTRL2,
//			devp->prop.decimation_ratio,
//			ASFIFO_DECIMATION_NUM_BIT, ASFIFO_DECIMATION_NUM_WID);
//		/* en */
//		wr_bits(offset, VDIN_ASFIFO_CTRL2, 1,
//			ASFIFO_DECIMATION_DE_EN_BIT,
//			ASFIFO_DECIMATION_DE_EN_WID);
//		/* manual reset, rst = 1 & 0 */
//		wr_bits(offset, VDIN_ASFIFO_CTRL2, 1,
//			ASFIFO_DECIMATION_SYNC_WITH_DE_BIT,
//			ASFIFO_DECIMATION_SYNC_WITH_DE_WID);
//		wr_bits(offset, VDIN_ASFIFO_CTRL2, 0,
//			ASFIFO_DECIMATION_SYNC_WITH_DE_BIT,
//			ASFIFO_DECIMATION_SYNC_WITH_DE_WID);
	}

	/* output_width_m1 */
	//wr_bits(offset, VDIN_INTF_WIDTHM1, (devp->h_active - 1),
	//	VDIN_INTF_WIDTHM1_BIT, VDIN_INTF_WIDTHM1_WID);
	if (vdin_ctl_dbg)
		pr_info("%s: h_active=%u, v_active=%u\n",
			__func__, devp->h_active, devp->v_active);
}

void vdin_fix_nonstd_vsync_t3x(struct vdin_dev_s *devp)
{
	wr_bits(devp->addr_offset, VDIN0_WRMIF_URGENT_CTRL, 1,
		0, 16);
}

/* this function will set the bellow parameters of devp:
 * 1.h_active
 * 2.v_active
 *	set VDIN_WIN_H_START_END
 *		Bit 28:16 input window H start
 *		Bit 12:0  input window H end
 *	set VDIN_WIN_V_START_END
 *		Bit 28:16 input window V start
 *		Bit 12:0  input window V start
 */
void vdin_set_cutwin_t3x(struct vdin_dev_s *devp)
{
	unsigned int offset = devp->addr_offset;
	unsigned int he = 0, ve = 0;

	if ((devp->prop.hs || devp->prop.he ||
	     devp->prop.vs || devp->prop.ve) &&
	    devp->h_active > (devp->prop.hs + devp->prop.he) &&
	    devp->v_active > (devp->prop.vs + devp->prop.ve)) {
		devp->h_active -= (devp->prop.he + devp->prop.hs);
		devp->v_active -= (devp->prop.ve + devp->prop.vs);
		he = devp->prop.hs + devp->h_active - 1;
		ve = devp->prop.vs + devp->v_active - 1;
		wr(offset, VDIN0_CUTWIN_H_WIN,
		   (devp->prop.hs << INPUT_WIN_H_START_BIT) |
		   (he << INPUT_WIN_H_END_BIT));
		wr(offset, VDIN0_CUTWIN_V_WIN,
		   (devp->prop.vs << INPUT_WIN_V_START_BIT) |
		   (ve << INPUT_WIN_V_END_BIT));
		wr_bits(offset, VDIN0_PP_CTRL, 1,
			PP_WIN_EN_BIT, PP_WIN_EN_WID);
		if (vdin_ctl_dbg)
			pr_info("%s enable cutwin hs=%d, he=%d,  vs=%d, ve=%d\n",
				__func__,
			devp->prop.hs, devp->prop.he,
			devp->prop.vs, devp->prop.ve);
	} else {
		wr_bits(offset, VDIN0_PP_CTRL, 0,
			PP_WIN_EN_BIT, PP_WIN_EN_WID);
		wr(offset, VDIN0_CUTWIN_H_WIN, 0x1fff);
		wr(offset, VDIN0_CUTWIN_V_WIN, 0x1fff);
		if (vdin_ctl_dbg)
			pr_info("%s disable cutwin!!! hs=%d, he=%d,  vs=%d, ve=%d\n",
				__func__,
			devp->prop.hs, devp->prop.he,
			devp->prop.vs, devp->prop.ve);
	}
	if (vdin_ctl_dbg)
		pr_info("%s: h_active=%d, v_active=%d, hs:%u, he:%u, vs:%u, ve:%u\n",
			__func__, devp->h_active, devp->v_active,
			devp->prop.hs, devp->prop.he,
			devp->prop.vs, devp->prop.ve);
}

void vdin_change_matrix0_t3x(u32 offset, u32 matrix_csc)
{
	struct vdin_matrix_lup_s *matrix_tbl;

	if (matrix_csc == VDIN_MATRIX_NULL)	{
		wr_bits(offset, VDIN0_PP_CTRL, 0,
			PP_MAT0_EN_BIT, PP_MAT0_EN_WID);
	} else {
		matrix_tbl = &vdin_matrix_lup[matrix_csc - 1];

		/*coefficient index select matrix0*/
		wr(offset, VDIN0_MAT_PRE_OFFSET0_1, matrix_tbl->pre_offset0_1);
		wr(offset, VDIN0_MAT_PRE_OFFSET2,   matrix_tbl->pre_offset2);
		wr(offset, VDIN0_MAT_COEF00_01,     matrix_tbl->coef00_01);
		wr(offset, VDIN0_MAT_COEF02_10,     matrix_tbl->coef02_10);
		wr(offset, VDIN0_MAT_COEF11_12,     matrix_tbl->coef11_12);
		wr(offset, VDIN0_MAT_COEF20_21,     matrix_tbl->coef20_21);
		wr(offset, VDIN0_MAT_COEF22,        matrix_tbl->coef22);
		wr(offset, VDIN0_MAT_OFFSET0_1,     matrix_tbl->post_offset0_1);
		wr(offset, VDIN0_MAT_OFFSET2,       matrix_tbl->post_offset2);
		wr_bits(offset, VDIN0_PP_CTRL, 1, PP_MAT0_EN_BIT, PP_MAT0_EN_WID);
	}

	pr_info("%s id:%d\n", __func__, matrix_csc);
}

/* hdr2 used as only matrix,not hdr matrix
 * should use hdr matrix related registers
 * VDIN_MAT1_XX have no effect
 */
void vdin_change_matrix1_t3x(u32 offset, u32 matrix_csc)
{
	struct vdin_matrix_lup_s *matrix_tbl;

	if (matrix_csc == VDIN_MATRIX_NULL)	{
		wr_bits(offset, VDIN0_PP_CTRL, 0,
			PP_MAT1_EN_BIT, PP_MAT1_EN_WID);
	} else {
		matrix_tbl = &vdin_matrix_lup[matrix_csc - 1];

		wr(offset, VDIN0_HDR2_MATRIXI_PRE_OFFSET0_1, matrix_tbl->pre_offset0_1);
		wr(offset, VDIN0_HDR2_MATRIXI_PRE_OFFSET2, matrix_tbl->pre_offset2);
		wr(offset, VDIN0_HDR2_MATRIXI_COEF00_01, matrix_tbl->coef00_01);
		wr(offset, VDIN0_HDR2_MATRIXI_COEF02_10, matrix_tbl->coef02_10);
		wr(offset, VDIN0_HDR2_MATRIXI_COEF11_12, matrix_tbl->coef11_12);
		wr(offset, VDIN0_HDR2_MATRIXI_COEF20_21, matrix_tbl->coef20_21);
		wr(offset, VDIN0_HDR2_MATRIXI_COEF22, matrix_tbl->coef22);
		wr(offset, VDIN0_HDR2_MATRIXI_OFFSET0_1, matrix_tbl->post_offset0_1);
		wr(offset, VDIN0_HDR2_MATRIXI_OFFSET2, matrix_tbl->post_offset2);

		wr(offset, VDIN0_HDR2_MATRIXI_EN_CTRL, 1);
		wr_bits(offset, VDIN0_HDR2_CTRL, 1, 16, 1);/* reg_only_mat */
		/* Actually no mat1 on t3x */
		//wr_bits(offset, VDIN0_PP_CTRL, 0, PP_MAT1_EN_BIT, PP_MAT1_EN_WID);
	}

	pr_info("%s id:%d\n", __func__, matrix_csc);
}

//TODO:hdr
void vdin_change_matrix_hdr_t3x(u32 offset, u32 matrix_csc)
{
	struct vdin_matrix_lup_s *matrix_tbl;

	if (matrix_csc == VDIN_MATRIX_NULL)	{
		/* reg_mat1_en */
		wr_bits(offset, VDIN0_PP_CTRL, 0, PP_MAT1_EN_BIT, PP_MAT1_EN_WID);
		/* reg_mtrxi_en */
		wr_bits(offset, VDIN0_HDR2_CTRL, 1, 14, 1);
	} else {
		matrix_tbl = &vdin_matrix_lup[matrix_csc - 1];

		wr(offset, VDIN0_HDR2_MATRIXI_PRE_OFFSET0_1, matrix_tbl->pre_offset0_1);
		wr(offset, VDIN0_HDR2_MATRIXI_PRE_OFFSET2, matrix_tbl->pre_offset2);
		wr(offset, VDIN0_HDR2_MATRIXI_COEF00_01, matrix_tbl->coef00_01);
		wr(offset, VDIN0_HDR2_MATRIXI_COEF02_10, matrix_tbl->coef02_10);
		wr(offset, VDIN0_HDR2_MATRIXI_COEF11_12, matrix_tbl->coef11_12);
		wr(offset, VDIN0_HDR2_MATRIXI_COEF20_21, matrix_tbl->coef20_21);
		wr(offset, VDIN0_HDR2_MATRIXI_COEF22, matrix_tbl->coef22);
		wr(offset, VDIN0_HDR2_MATRIXI_OFFSET0_1, matrix_tbl->post_offset0_1);
		wr(offset, VDIN0_HDR2_MATRIXI_OFFSET2, matrix_tbl->post_offset2);

//		matrix_tbl = &vdin_matrix_lup[matrix_csc - 1];
//		wr(offset, VDIN0_HDR2_MATRIXO_PRE_OFFSET0_1, matrix_tbl->pre_offset0_1);
//		wr(offset, VDIN0_HDR2_MATRIXO_PRE_OFFSET2, matrix_tbl->pre_offset2);
//		wr(offset, VDIN0_HDR2_MATRIXO_COEF00_01, matrix_tbl->coef00_01);
//		wr(offset, VDIN0_HDR2_MATRIXO_COEF02_10, matrix_tbl->coef02_10);
//		wr(offset, VDIN0_HDR2_MATRIXO_COEF11_12, matrix_tbl->coef11_12);
//		wr(offset, VDIN0_HDR2_MATRIXO_COEF20_21, matrix_tbl->coef20_21);
//		wr(offset, VDIN0_HDR2_MATRIXO_COEF22, matrix_tbl->coef22);
//		wr(offset, VDIN0_HDR2_MATRIXO_OFFSET0_1, matrix_tbl->post_offset0_1);
//		wr(offset, VDIN0_HDR2_MATRIXO_OFFSET2, matrix_tbl->post_offset2);
//		wr(offset, VDIN0_HDR2_CGAIN_OFFT,
//			(matrix_tbl->post_offset2 << 16) | (matrix_tbl->post_offset2) & 0xfff);

		wr_bits(offset, VDIN0_HDR2_CTRL, 0, 16, 1);/* reg_only_mat */
		wr_bits(offset, VDIN0_HDR2_CTRL, 1, 15, 1);/* reg_mtrxo_en */
		wr_bits(offset, VDIN0_HDR2_CTRL, 1, 14, 1);/* reg_mtrxi_en */
		wr_bits(offset, VDIN0_HDR2_CTRL, 1, 13, 1);/* reg_hdr2_top_en */
		wr_bits(offset, VDIN0_PP_CTRL, 1, PP_MAT1_EN_BIT, PP_MAT1_EN_WID);
	}

	pr_info("%s id:%d\n", __func__, matrix_csc);
}

static enum vdin_matrix_csc_e
vdin_set_color_matrix_t3x(enum vdin_matrix_sel_e matrix_sel, unsigned int offset,
		       struct tvin_format_s *tvin_fmt_p,
		       enum vdin_format_convert_e format_convert,
		       enum tvin_port_e port,
		       enum tvin_color_fmt_range_e color_fmt_range,
		       unsigned int vdin_hdr_flag,
		       unsigned int color_range_mode)
{
	enum vdin_matrix_csc_e    matrix_csc = VDIN_MATRIX_NULL;
	/*struct vdin_matrix_lup_s *matrix_tbl;*/
	struct tvin_format_s *fmt_info = tvin_fmt_p;

	pr_info("%s tvin_fmt_p:%p %p,format_convert:%d,matrix_sel:%d\n",
		__func__, fmt_info, tvin_fmt_p, format_convert, matrix_sel);

	if (!fmt_info) {
		pr_info("error %s tvin_fmt_p:%p\n", __func__, tvin_fmt_p);
		return VDIN_MATRIX_NULL;
	}
	switch (format_convert)	{
	case VDIN_MATRIX_XXX_YUV_BLACK:
		matrix_csc = VDIN_MATRIX_XXX_YUV601_BLACK;
		break;
	case VDIN_FORMAT_CONVERT_RGB_YUV422:
	case VDIN_FORMAT_CONVERT_RGB_NV12:
	case VDIN_FORMAT_CONVERT_RGB_NV21:
		if (IS_HDMI_SRC(port)) {
			if (color_range_mode == 1) {
				if (color_fmt_range == TVIN_RGB_FULL) {
					matrix_csc = VDIN_MATRIX_RGB_YUV709F;
					if (vdin_hdr_flag == 1)
						matrix_csc =
						VDIN_MATRIX_RGB_YUV709;
				} else {
					matrix_csc =
						VDIN_MATRIX_RGBS_YUV709F;
					if (vdin_hdr_flag == 1)
						matrix_csc =
						VDIN_MATRIX_RGBS_YUV709;
				}
			} else {
				if (color_fmt_range == TVIN_RGB_FULL) {
					if (vdin_hdr_flag == 1)
						matrix_csc =
						VDIN_MATRIX_RGB2020_YUV2020;
					else
						matrix_csc =
						VDIN_MATRIX_RGB_YUV709;
				} else {
					matrix_csc = VDIN_MATRIX_RGBS_YUV709;
				}
			}
		} else {
			if (color_range_mode == 1)
				matrix_csc = VDIN_MATRIX_RGB_YUV709F;
			else
				matrix_csc = VDIN_MATRIX_RGB_YUV709;
		}
		break;
	case VDIN_FORMAT_CONVERT_GBR_YUV422:
		matrix_csc = VDIN_MATRIX_GBR_YUV601;
		break;
	case VDIN_FORMAT_CONVERT_BRG_YUV422:
		matrix_csc = VDIN_MATRIX_BRG_YUV601;
		break;
	case VDIN_FORMAT_CONVERT_RGB_YUV444:
		if (IS_HDMI_SRC(port)) {
			if (color_range_mode == 1) {
				if (color_fmt_range == TVIN_RGB_FULL) {
					matrix_csc = VDIN_MATRIX_RGB_YUV709F;
					if (vdin_hdr_flag == 1)
						matrix_csc =
						VDIN_MATRIX_RGB_YUV709;
				} else {
					matrix_csc = VDIN_MATRIX_RGBS_YUV709F;
					if (vdin_hdr_flag == 1)
						matrix_csc =
						VDIN_MATRIX_RGBS_YUV709;
				}
			} else {
				if (color_fmt_range == TVIN_RGB_FULL) {
					if (vdin_hdr_flag == 1)
						matrix_csc =
						VDIN_MATRIX_RGB2020_YUV2020;
					else
						matrix_csc =
						VDIN_MATRIX_RGB_YUV709F;
				} else {
					matrix_csc = VDIN_MATRIX_RGBS_YUV709;
				}
			}
		} else {
			if (color_range_mode == 1)
				matrix_csc = VDIN_MATRIX_RGB_YUV709F;
			else
				matrix_csc = VDIN_MATRIX_RGB_YUV709;
		}
		break;
	case VDIN_FORMAT_CONVERT_YUV_RGB:
		if ((fmt_info->scan_mode == TVIN_SCAN_MODE_PROGRESSIVE &&
		     fmt_info->v_active >= 720) || /* 720p & above */
		    (fmt_info->scan_mode == TVIN_SCAN_MODE_INTERLACED &&
		     fmt_info->v_active >= 540))  /* 1080i & above */
			matrix_csc = VDIN_MATRIX_YUV709_RGB;
		else
			matrix_csc = VDIN_MATRIX_YUV601_RGB;
		break;
	case VDIN_FORMAT_CONVERT_YUV_GBR:
		if ((fmt_info->scan_mode == TVIN_SCAN_MODE_PROGRESSIVE &&
		     fmt_info->v_active >= 720) || /* 720p & above */
		    (fmt_info->scan_mode == TVIN_SCAN_MODE_INTERLACED &&
		     fmt_info->v_active >= 540))    /* 1080i & above */
			matrix_csc = VDIN_MATRIX_YUV709_GBR;
		else
			matrix_csc = VDIN_MATRIX_YUV601_GBR;
		break;
	case VDIN_FORMAT_CONVERT_YUV_BRG:
		if ((fmt_info->scan_mode == TVIN_SCAN_MODE_PROGRESSIVE &&
		     fmt_info->v_active >= 720) || /* 720p & above */
		    (fmt_info->scan_mode == TVIN_SCAN_MODE_INTERLACED &&
		     fmt_info->v_active >= 540))    /* 1080i & above */
			matrix_csc = VDIN_MATRIX_YUV709_BRG;
		else
			matrix_csc = VDIN_MATRIX_YUV601_BRG;
		break;
	case VDIN_FORMAT_CONVERT_YUV_YUV422:
	case VDIN_FORMAT_CONVERT_YUV_YUV444:
	case VDIN_FORMAT_CONVERT_YUV_NV12:
	case VDIN_FORMAT_CONVERT_YUV_NV21:
		if ((fmt_info->scan_mode == TVIN_SCAN_MODE_PROGRESSIVE &&
		     fmt_info->v_active >= 720) || /* 720p & above */
		    (fmt_info->scan_mode == TVIN_SCAN_MODE_INTERLACED &&
		     fmt_info->v_active >= 540)) { /* 1080i & above */

			if (color_range_mode == 1 &&
			    color_fmt_range != TVIN_YUV_FULL)
				matrix_csc = VDIN_MATRIX_YUV709_YUV709F;
			else if (color_range_mode == 0 &&
				 color_fmt_range == TVIN_YUV_FULL)
				matrix_csc = VDIN_MATRIX_YUV709F_YUV709;
		} else {
			if (color_range_mode == 1) {
				if (color_fmt_range == TVIN_YUV_FULL)
					matrix_csc =
						VDIN_MATRIX_YUV601F_YUV709F;
				else
					matrix_csc = VDIN_MATRIX_YUV601_YUV709F;
			} else {
				if (color_fmt_range == TVIN_YUV_FULL)
					matrix_csc = VDIN_MATRIX_YUV601F_YUV709;
				else
					matrix_csc = VDIN_MATRIX_YUV601_YUV709;
			}
		}
		if (vdin_hdr_flag == 1) {
			if (color_fmt_range == TVIN_YUV_FULL)
				matrix_csc = VDIN_MATRIX_YUV2020F_YUV2020;
			else
				matrix_csc = VDIN_MATRIX_NULL;
		}
		break;
	default:
		matrix_csc = VDIN_MATRIX_NULL;
		break;
	}

	//vdin_manual_matrix_csc_t3x(&matrix_csc);

	if (matrix_sel == VDIN_SEL_MATRIX0)
		vdin_change_matrix0_t3x(offset, matrix_csc);
	else if (matrix_sel == VDIN_SEL_MATRIX1)
		vdin_change_matrix1_t3x(offset, matrix_csc);
	else if (matrix_sel == VDIN_SEL_MATRIX_HDR)
		vdin_change_matrix_hdr_t3x(offset, matrix_csc);
	else
		matrix_csc = VDIN_MATRIX_NULL;

	return matrix_csc;
}

void vdin_set_hdr_t3x(struct vdin_dev_s *devp)
{
	enum vd_format_e video_format = SIGNAL_SDR;
	unsigned int vdin_hdr = VDIN1_HDR;

	if ((devp->hw_core == VDIN_HW_CORE_NORMAL && !devp->dtdata->vdin0_set_hdr) ||
	    (devp->hw_core == VDIN_HW_CORE_LITE && !devp->dtdata->vdin1_set_hdr) ||
		devp->debug.vdin1_set_hdr_bypass)
		return;

	switch (devp->parm.port) {
	case TVIN_PORT_VIU1_VIDEO:
	case TVIN_PORT_VIU1_WB0_VD1:
	case TVIN_PORT_VIU1_WB0_VD2:
	case TVIN_PORT_VIU1_WB0_POST_BLEND:
	case TVIN_PORT_VIU1_WB1_POST_BLEND:
	case TVIN_PORT_VIU1_WB1_VD1:
	case TVIN_PORT_VIU1_WB1_VD2:
	case TVIN_PORT_VIU1_WB0_OSD1:
	case TVIN_PORT_VIU1_WB0_OSD2:
	case TVIN_PORT_VIU1_WB1_OSD1:
	case TVIN_PORT_VIU1_WB1_OSD2:
		video_format = devp->vd1_fmt;
		break;

	case TVIN_PORT_VIU1_WB0_VPP:
	case TVIN_PORT_VIU1_WB1_VPP:
	case TVIN_PORT_VIU2_ENCL:
	case TVIN_PORT_VIU2_ENCI:
	case TVIN_PORT_VIU2_ENCP:
		video_format = devp->tx_fmt;
		break;
	case TVIN_PORT_HDMI0 ... TVIN_PORT_HDMI7:
		if (devp->prop.hdr10p_info.hdr10p_on)
			video_format = SIGNAL_HDR10PLUS;
		else if (devp->prop.vdin_hdr_flag)
			video_format = SIGNAL_HDR10;
		else
			video_format = SIGNAL_SDR;
		break;

	default:
		video_format = devp->tx_fmt;
		break;
	}

#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
	if (for_amdv_certification()) {
		/*not modify data*/
		return;
	}
#endif
	//video_format = SIGNAL_HDR10; // force for debug
	if (devp->index == 1)
		vdin_hdr = VDIN1_HDR;
	else if (devp->index == 2)
		vdin_hdr = VDIN2_HDR;
	else
		vdin_hdr = VDIN0_HDR;

#ifndef VDIN_BRINGUP_NO_AMLVECM
	pr_info("%s fmt:%d\n", __func__, video_format);
	switch (video_format) {
	case SIGNAL_HDR10:
	case SIGNAL_HDR10PLUS:
		/* parameters:
		 * 1st, module sel: 6=VDIN0_HDR, 7=VDIN1_HDR
		 * 2nd, process sel: bit1=HDR_SDR
		 * bit11=HDR10P_SDR
		 */
		hdr_set(vdin_hdr, HDR_SDR, VPP_TOP0);
		break;

	case SIGNAL_SDR:
		/* HDR_BYPASS(bit0) | HLG_BYPASS(bit3) */
		hdr_set(vdin_hdr, HDR_BYPASS | HLG_BYPASS, VPP_TOP0);
		break;

	case SIGNAL_HLG:
		/* HLG_SDR(bit4) */
		hdr_set(vdin_hdr, HLG_SDR, VPP_TOP0);
		break;

	/* VDIN DON'T support dv loopback currently */
	case SIGNAL_DOVI:
		pr_err("err:  don't support dv signal loopback");
		break;

	default:
		break;
	}
#endif
	/* hdr set uses rdma, will delay 1 frame to take effect */
	devp->frame_drop_num = 1;
}

/*set matrix based on rgb_info_enable:
 * 0:set matrix0, disable matrix1
 * 1:set matrix1, set matrix0
 * after equal g12a: matrix1, matrix_hdr2
 */
void vdin_set_matrix_t3x(struct vdin_dev_s *devp)
{
//	enum vdin_format_convert_e format_convert_matrix0;
//	enum vdin_format_convert_e format_convert_matrix1;
//	unsigned int offset = devp->addr_offset;
	enum vdin_matrix_sel_e matrix_sel;

	if (rgb_info_enable == 0) {
		/* matrix1 disable */
//	wr_bits(offset, VDIN_MATRIX_CTRL, 0,
//		VDIN_MATRIX1_EN_BIT, VDIN_MATRIX1_EN_WID);
//		if (cpu_after_eq(MESON_CPU_MAJOR_ID_G12A)) {
//			matrix_sel = VDIN_SEL_MATRIX1;/*VDIN_SEL_MATRIX_HDR*/
//		} else {
//			matrix_sel = VDIN_SEL_MATRIX0;
//		}

		if (devp->debug.dbg_sel_mat & 0x10)
			matrix_sel = devp->debug.dbg_sel_mat & 0xf;
		else
			matrix_sel = VDIN_SEL_MATRIX0;/*VDIN_SEL_MATRIX_HDR*/
		pr_info("%s %d:%p,conv:%d,port:%#x,fmt_range:%d,color_range:%d\n",
			__func__, __LINE__,
			devp->fmt_info_p, devp->format_convert,
			devp->parm.port, devp->prop.color_fmt_range,
			devp->color_range_mode);

		if (!devp->dv.dv_flag) {
			devp->csc_idx = vdin_set_color_matrix_t3x(matrix_sel,
				devp->addr_offset,
				devp->fmt_info_p,
				devp->format_convert,
				devp->parm.port,
				devp->prop.color_fmt_range,
				devp->prop.vdin_hdr_flag | devp->dv.dv_flag,
				devp->color_range_mode);
		} else {
			devp->csc_idx = VDIN_MATRIX_NULL;
		}

		vdin_set_hdr_t3x(devp);

//		#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
//		if (vdin_is_dolby_signal_in(devp) ||
//		    devp->parm.info.fmt == TVIN_SIG_FMT_CVBS_SECAM)
//			wr_bits(offset, VDIN_MATRIX_CTRL, 0,
//				VDIN_MATRIX_EN_BIT, VDIN_MATRIX_EN_WID);
//		#endif
//		wr_bits(offset, VDIN_MATRIX_CTRL, 3,
//			VDIN_PROBE_SEL_BIT, VDIN_PROBE_SEL_WID);
	}

	if (devp->matrix_pattern_mode)
		vdin_set_matrix_color_t3x(devp);
}

void vdin_select_matrix_t3x(struct vdin_dev_s *devp, unsigned char id,
		      enum vdin_format_convert_e csc)
{
	pr_info("%s %d:id=%d, csc=%d\n", __func__, __LINE__, id, csc);
	switch (id) {
	case 0:
		if (cpu_after_eq(MESON_CPU_MAJOR_ID_G12A))
			devp->csc_idx =
				vdin_set_color_matrix_t3x(VDIN_SEL_MATRIX1,
				devp->addr_offset,
				devp->fmt_info_p,
				devp->format_convert,
				devp->parm.port,
				devp->prop.color_fmt_range,
				devp->prop.vdin_hdr_flag | devp->dv.dv_flag,
				devp->color_range_mode);
		else
			devp->csc_idx =
				vdin_set_color_matrix_t3x(VDIN_SEL_MATRIX0,
				devp->addr_offset,
				devp->fmt_info_p, csc,
				devp->parm.port,
				devp->prop.color_fmt_range,
				devp->prop.vdin_hdr_flag | devp->dv.dv_flag,
				devp->color_range_mode);
		break;
	case 1:
		devp->csc_idx = vdin_set_color_matrix_t3x(VDIN_SEL_MATRIX1,
				devp->addr_offset,
				devp->fmt_info_p, csc,
				devp->parm.port,
				devp->prop.color_fmt_range,
				devp->prop.vdin_hdr_flag | devp->dv.dv_flag,
				devp->color_range_mode);
		break;
	default:
		break;
	}
}

static void vdin_blkbar_param_init(struct vdin_blkbar_s *blkbar)
{
	blkbar->gclk_ctrl	= 0;
	blkbar->soft_rst	= 0;
	blkbar->conv_num	= 9;
	blkbar->hwidth		= 512;
	blkbar->vwidth		= 360;
	blkbar->row_top		= 1035;
	blkbar->row_bot		= 920;
	blkbar->top_bot_hstart	= 384;
	blkbar->top_bot_hend	= 1536;
	blkbar->top_vstart	= 0;
	blkbar->bot_vend	= 1079;
	blkbar->left_hstart	= 512;
	blkbar->right_hend	= 1408;
	blkbar->lef_rig_vstart	= 216;
	blkbar->lef_rig_vend	= 864;
	blkbar->black_level	= 16;
	blkbar->blk_col_th	= 576;
	blkbar->white_level	= 16;
	blkbar->top_pos		= 0;
	blkbar->bot_pos		= 0;
	blkbar->left_pos	= 0;
	blkbar->right_pos	= 0;
	blkbar->debug_top_cnt	= 0;
	blkbar->debug_bot_cnt	= 0;
	blkbar->debug_lef_cnt	= 0;
	blkbar->debug_rig_cnt	= 0;
	blkbar->pat_gclk_ctrl	= 0;
	blkbar->pat_bw_flag	= 0;
	blkbar->blk_wht_th	= 1680;
	blkbar->black_th	= 17;
	blkbar->white_th	= 236;
	blkbar->win_en		= 0;
	blkbar->h_bgn		= 0;
	blkbar->h_end		= 100;
	blkbar->v_bgn		= 0;
	blkbar->v_end		= 10;
}
/*set block bar
 *base on flowing parameters:
 *a.h_active	b.v_active
 */
static inline void vdin_set_bbar_t3x(struct vdin_dev_s *devp,
	unsigned int v, unsigned int h)
{
	u32 offset = devp->addr_offset;
	u32 blkbar_conv_num = 9;
	u32 blkbar_hwidth   = 512;
	u32 black_level = 16;
	u32 white_level = 16;
	//u32 win_en = 0;
	//u32 h_bgn = 0;
	//u32 h_end = 100;

	struct vdin_blkbar_s vdin_blkbar;

	vdin_blkbar_param_init(&vdin_blkbar);

	if (h < 128) {
		pr_info("%s,vdin%d h = %d\n", __func__, devp->index, h);
		return;
	}

	if (h >= 128 && h < (128 + 64)) {
		blkbar_conv_num = 5;
		blkbar_hwidth   = 32;
	} else if (h >= (256 - 64) && h < (256 + 128)) {
		blkbar_conv_num = 6;
		blkbar_hwidth   = 64;
	} else if (h >= (512 - 128) && h < (512 + 256)) {
		blkbar_conv_num = 7;
		blkbar_hwidth   = 128;
	} else if (h >= (1024 - 256) && h < (1024 + 512)) {
		blkbar_conv_num = 8;
		blkbar_hwidth   = 256;
	} else if (h >= (2048 - 512) && h < (2048 + 1024)) {
		blkbar_conv_num = 9;
		blkbar_hwidth   = 512;
	} else if (h >= (4096 - 1024) && h < (4096 + 2048)) {
		blkbar_conv_num = 10;
		blkbar_hwidth   = 1024;
	}

	vdin_blkbar.soft_rst		=  1;
	vdin_blkbar.conv_num		=  blkbar_conv_num;
	vdin_blkbar.hwidth		=  blkbar_hwidth;
	vdin_blkbar.vwidth		=  v / 3;
	vdin_blkbar.row_top		=  ((h / 5) * 3 - 1) / 10 * 9;
	vdin_blkbar.row_bot		=  ((h / 5) * 3 - 1) / 3 * 1;
	vdin_blkbar.top_bot_hstart	=  (h / 5) * 1;
	vdin_blkbar.top_bot_hend	=  (h / 5) * 4;
	vdin_blkbar.top_vstart		=  0;
	vdin_blkbar.bot_vend		=  v - 1;
	vdin_blkbar.left_hstart		=  blkbar_hwidth;
	if ((h % 2) == 0)
		vdin_blkbar.right_hend = h - blkbar_hwidth;
	else
		vdin_blkbar.right_hend = h - blkbar_hwidth - 1;

	vdin_blkbar.lef_rig_vstart	=  v / 5;
	vdin_blkbar.lef_rig_vend	=  (v / 5) * 4;
	vdin_blkbar.black_level		=  black_level;
	vdin_blkbar.blk_col_th		=  ((v / 5) * 3) / 10 * 9;
	vdin_blkbar.white_level		=  white_level;

	//if (win_en)
	//	vdin_blkbar.blk_wht_th = (h_end - h_bgn) / 8 * 7;
	//else
	//	vdin_blkbar.blk_wht_th = h / 8 * 7;
	vdin_blkbar.blk_wht_th = h / 8 * 7;

	wr(offset, VDIN0_BLKBAR_CTRL,  vdin_blkbar.gclk_ctrl      << 30 |
					vdin_blkbar.pat_gclk_ctrl << 28 |
					vdin_blkbar.soft_rst      << 4  |
					vdin_blkbar.conv_num      << 0);

	wr(offset, VDIN0_BLKBAR_H_V_WIDTH, vdin_blkbar.hwidth << 16 |
					   vdin_blkbar.vwidth << 0);

	wr(offset, VDIN0_BLKBAR_ROW_TOP_BOT, vdin_blkbar.row_top << 16 |
					     vdin_blkbar.row_bot << 0);

	wr(offset, VDIN0_BLKBAR_TOP_BOT_H_START_END, vdin_blkbar.top_bot_hstart << 16 |
						     vdin_blkbar.top_bot_hend   << 0);

	wr(offset, VDIN0_BLKBAR_TOP_BOT_V_START_END, vdin_blkbar.top_vstart << 16 |
						     vdin_blkbar.bot_vend   << 0);

	wr(offset, VDIN0_BLKBAR_LEF_RIG_H_START_END, vdin_blkbar.left_hstart << 16 |
						     vdin_blkbar.right_hend  << 0);

	wr(offset, VDIN0_BLKBAR_LEF_RIG_V_START_END, vdin_blkbar.lef_rig_vstart << 16 |
						     vdin_blkbar.lef_rig_vend   << 0);

	wr(offset, VDIN0_BLKBAR_BLK_WHT_LEVEL, vdin_blkbar.black_level  << 24 |
						vdin_blkbar.blk_col_th  << 8  |
						vdin_blkbar.white_level << 0);

	wr(offset, VDIN0_BLKBAR_PAT_BLK_WHT_TH, vdin_blkbar.blk_wht_th << 16 |
						 vdin_blkbar.black_th  << 8 |
						 vdin_blkbar.white_th  << 0);

	wr(offset, VDIN0_BLKBAR_PAT_H_START_END, vdin_blkbar.win_en << 31 |
						 vdin_blkbar.h_bgn  << 16 |
						 vdin_blkbar.h_end  << 0);

	wr(offset, VDIN0_BLKBAR_PAT_V_START_END, vdin_blkbar.v_bgn << 16 |
						 vdin_blkbar.v_end << 0);

	/* reg_blkbar_gclk_ctrl */
	wr_bits(offset, VDIN0_PP_GCLK_CTRL, 0, 0, 2);
	/* reg_blkbar_det_sel */
	//00:before mat0;01:after mat0;10:after scale;11:after mat1
	wr_bits(offset, VDIN0_PP_CTRL, 0, 14, 2);

	/* reg_blkbar_det_en */
	wr_bits(offset, VDIN0_PP_CTRL, 1, 8, 1);
	/* manual reset, rst = 0 & 1, raising edge mode */
	//reg_blkbar_soft_rst
	wr_bits(offset, VDIN0_BLKBAR_CTRL, 0, 4, 1);
	wr_bits(offset, VDIN0_BLKBAR_CTRL, 1, 4, 1);
	pr_info("%s done,vdin%d h = %d,v = %d\n", __func__, devp->index, h, v);
}

/*et histogram window
 * pow\h_start\h_end\v_start\v_end
 */
static inline void vdin_set_histogram_t3x(unsigned int offset, unsigned int hs,
				      unsigned int he, unsigned int vs,
				      unsigned int ve)
{
	unsigned int pixel_sum = 0, record_len = 0, hist_pow = 0;
	//pr_info("%s %d:hs=%d,he=%d,vs=%d,ve=%d\n",
	//	__func__, __LINE__, hs, he, vs, ve);
	if (hs < he && vs < ve) {
		pixel_sum = (he - hs + 1) * (ve - vs + 1);
		record_len = 0xffff << 3;
		while (pixel_sum > record_len && hist_pow < 3) {
			hist_pow++;
			record_len <<= 1;
		}
//		pr_info("%s %d:hs=%d,he=%d,vs=%d,ve=%d,hist_pow=%d\n",
//			__func__, __LINE__, hs, he, vs, ve, hist_pow);
		/* #ifdef CONFIG_MESON2_CHIP */
		/* pow */
		wr_bits(offset, VDIN0_LUMA_HIST_CTRL, hist_pow,
			HIST_POW_BIT, HIST_POW_WID);
		/* win_hs */
		wr_bits(offset, VDIN0_LUMA_HIST_H_START_END, hs,
			HIST_HSTART_BIT, HIST_HSTART_WID);
		/* win_he */
		wr_bits(offset, VDIN0_LUMA_HIST_H_START_END, he,
			HIST_HEND_BIT, HIST_HEND_WID);
		/* win_vs */
		wr_bits(offset, VDIN0_LUMA_HIST_V_START_END, vs,
			HIST_VSTART_BIT, HIST_VSTART_WID);
		/* win_ve */
		wr_bits(offset, VDIN0_LUMA_HIST_V_START_END, ve,
			HIST_VEND_BIT, HIST_VEND_WID);
	}
}

/* set hist mux
 * hist_spl_sel, 0: win; 1: mat0; 2: vsc; 3: hdr2_mat1
 */
static inline void vdin_set_hist_mux_t3x(struct vdin_dev_s *devp)
{
	enum tvin_port_e port = TVIN_PORT_NULL;

	port = devp->parm.port;
	/*if ((port < TVIN_PORT_HDMI0) || (port > TVIN_PORT_HDMI7))*/
	/*	return;*/
	/* For AV input no correct data, from vlsi fei.jun
	 * AV, HDMI all set 3
	 */
	/* use 11: form matrix1 din */
//	wr_bits(devp->addr_offset, VDIN_HIST_CTRL, 3,
//		HIST_HIST_DIN_SEL_BIT, HIST_HIST_DIN_SEL_WID);

	/* reg_luma_hist_win_en,hist used for full picture */
	wr_bits(devp->addr_offset, VDIN0_LUMA_HIST_CTRL, 0, 1, 1);

	wr_bits(devp->addr_offset, VDIN0_LUMA_HIST_CTRL, 0x7f, 24, 8);//reg_luma_hist_pix_white_th
	wr_bits(devp->addr_offset, VDIN0_LUMA_HIST_CTRL, 0x7f, 16, 8);//reg_luma_hist_pix_black_th

	wr_bits(devp->addr_offset, VDIN0_PP_CTRL, 0, 12, 2);//reg_luma_hist_sel
	wr_bits(devp->addr_offset, VDIN0_PP_CTRL, 1, 7, 1);//reg_luma_hist_en
}

/* urgent ctr config */
/* if vdin fifo over up_th,will trigger increase
 *  urgent responds to vdin write,
 * if vdin fifo lower dn_th,will trigger decrease
 *  urgent responds to vdin write
 */
void vdin_urgent_patch_t3x(unsigned int offset, unsigned int v,
			      unsigned int h)
{
	if (h >= 1920 && v >= 1080) {
		wr_bits(offset, VDIN0_LFIFO_CTRL, 1,
			VDIN_LFIFO_URG_CTRL_EN_BIT, VDIN_LFIFO_URG_CTRL_EN_WID);
		wr_bits(offset, VDIN0_LFIFO_CTRL, 1,
			VDIN_LFIFO_URG_WR_EN_BIT, VDIN_LFIFO_URG_WR_EN_WID);
		wr_bits(offset, VDIN0_LFIFO_CTRL, 20,
			VDIN_LFIFO_URG_UP_TH_BIT, VDIN_LFIFO_URG_UP_TH_WID);
		wr_bits(offset, VDIN0_LFIFO_CTRL, 8,
			VDIN_LFIFO_URG_DN_TH_BIT, VDIN_LFIFO_URG_DN_TH_WID);
		/*vlsi guys suggest setting:*/
//		W_VCBUS_BIT(VPU_ARB_URG_CTRL, 1,
//			    VDIN_LFF_URG_CTRL_BIT, VDIN_LFF_URG_CTRL_WID);
//		W_VCBUS_BIT(VPU_ARB_URG_CTRL, 1,
//			    VPP_OFF_URG_CTRL_BIT, VPP_OFF_URG_CTRL_WID);
	} else {
		wr(offset, VDIN0_LFIFO_CTRL, 0);
		//aml_write_vcbus(VPU_ARB_URG_CTRL, 0);
	}
}

void vdin_urgent_patch_resume_t3x(unsigned int offset)
{
	//TODO vdin urgent setting
}

static unsigned int vdin_is_support_10bit_for_dw_t3x(struct vdin_dev_s *devp)
{
	if (devp->double_wr) {
		if (devp->double_wr_10bit_sup)
			return 1;
		else
			return 0;
	} else {
		return 1;
	}
}

/*static unsigned int vdin_wr_mode = 0xff;*/
/*module_param(vdin_wr_mode, uint, 0644);*/
/*MODULE_PARM_DESC(vdin_wr_mode, "vdin_wr_mode");*/

/* set write ctrl regs:
 * VDIN_WR_H_START_END
 * VDIN_WR_V_START_END
 * VDIN_WR_CTRL
 * VDIN_LFIFO_URG_CTRL
 */
static inline void vdin_set_wr_ctrl_t3x(struct vdin_dev_s *devp,
				    unsigned int offset, unsigned int v,
				    unsigned int h,
				    enum vdin_format_convert_e format_convert,
				    unsigned int full_pack,
				    unsigned int source_bitdepth)
{
	enum vdin_mif_fmt write_fmt = MIF_FMT_YUV422;
	unsigned int swap_cbcr = 0;
	unsigned int hconv_mode = 2;

	if (devp->vf_mem_size_small) {
		h = devp->h_shrink_out;
		v = devp->v_shrink_out;
	}

	pr_info("%s,%d.h=%d,v=%d\n", __func__, __LINE__, h, v);
	switch (format_convert)	{
	case VDIN_FORMAT_CONVERT_YUV_YUV422:
	case VDIN_FORMAT_CONVERT_RGB_YUV422:
	case VDIN_FORMAT_CONVERT_GBR_YUV422:
	case VDIN_FORMAT_CONVERT_BRG_YUV422:
		write_fmt = MIF_FMT_YUV422;
		break;

	case VDIN_FORMAT_CONVERT_YUV_NV12:
	case VDIN_FORMAT_CONVERT_RGB_NV12:
		write_fmt = MIF_FMT_NV12_21;
		swap_cbcr = 1;
		break;

	case VDIN_FORMAT_CONVERT_YUV_NV21:
	case VDIN_FORMAT_CONVERT_RGB_NV21:
		write_fmt = MIF_FMT_NV12_21;
		swap_cbcr = 0;
		break;

	default:
		write_fmt = MIF_FMT_YUV444;
		break;
	}

	/* yuv422 full pack mode for 10bit
	 * only support 8bit at vpp side when double write
	 */
	if ((format_convert == VDIN_FORMAT_CONVERT_YUV_YUV422 ||
	     format_convert == VDIN_FORMAT_CONVERT_RGB_YUV422 ||
	     format_convert == VDIN_FORMAT_CONVERT_GBR_YUV422 ||
	     format_convert == VDIN_FORMAT_CONVERT_BRG_YUV422) &&
	    full_pack == VDIN_422_FULL_PK_EN &&
	    source_bitdepth > VDIN_COLOR_DEEPS_8BIT &&
	    vdin_is_support_10bit_for_dw_t3x(devp)) {
		write_fmt = MIF_FMT_YUV422_FULL_PACK;

		/* IC bug, fixed at tm2 revB */
		if (devp->dtdata->hw_ver == VDIN_HW_ORG)
			hconv_mode = 0;
	}

	/* win_he */
	if ((h % 2) && devp->source_bitdepth > VDIN_COLOR_DEEPS_8BIT &&
	    devp->full_pack == VDIN_422_FULL_PK_EN &&
	    (devp->format_convert == VDIN_FORMAT_CONVERT_YUV_YUV422 ||
	     devp->format_convert == VDIN_FORMAT_CONVERT_RGB_YUV422 ||
	     devp->format_convert == VDIN_FORMAT_CONVERT_GBR_YUV422 ||
	     devp->format_convert == VDIN_FORMAT_CONVERT_BRG_YUV422))
		h += 1;

	wr(offset, VDIN0_DW_IN_SIZE,
		devp->s5_data.h_scale_out << 16 | devp->s5_data.v_scale_out << 0);
	wr(offset, VDIN0_DW_OUT_SIZE, devp->h_shrink_out << 16 | devp->v_shrink_out << 0);
	wr_bits(offset, VDIN0_WRMIF_H_START_END,
		(devp->h_shrink_out - 1), WR_HEND_BIT, WR_HEND_WID);
	/* win_ve */
	wr_bits(offset, VDIN0_WRMIF_V_START_END,
		(devp->v_shrink_out - 1), WR_VEND_BIT, WR_VEND_WID);
	/* hconv_mode */
	wr_bits(offset, VDIN0_WRMIF_CTRL, hconv_mode, 26, 2);
	/* vconv_mode */
	/* output even lines's cbcr */
	wr_bits(offset, VDIN0_WRMIF_CTRL, 0, 28, 2);

	if (write_fmt == MIF_FMT_NV12_21) {
		/* swap_cbcr */
		wr_bits(offset, VDIN0_WRMIF_CTRL, swap_cbcr, 30, 1);
	} else if (write_fmt == MIF_FMT_YUV444) {
		/* output all cbcr */
		wr_bits(offset, VDIN0_WRMIF_CTRL, 3, 28, 2);
	} else {
		/* swap_cbcr */
		wr_bits(offset, VDIN0_WRMIF_CTRL, 0, 30, 1);
		/* chroma canvas */
		/* wr_bits(offset, VDIN_WR_CTRL2, 0,
		 *  WRITE_CHROMA_CANVAS_ADDR_BIT,
		 */
		/* WRITE_CHROMA_CANVAS_ADDR_WID); */
	}

	/* format444 */
	wr_bits(offset, VDIN0_WRMIF_CTRL, write_fmt, WR_FMT_BIT, WR_FMT_WID);
	/* req_urgent */
	wr_bits(offset, VDIN0_WRMIF_CTRL, 1, WR_REQ_URGENT_BIT, WR_REQ_URGENT_WID);
	/* req_en */
	wr_bits(offset, VDIN0_WRMIF_CTRL, 0, WR_REQ_EN_BIT, WR_REQ_EN_WID);

	/*only for vdin0*/
	if (devp->dts_config.urgent_en && devp->hw_core == VDIN_HW_CORE_NORMAL)
		vdin_urgent_patch_t3x(offset, v, h);
	/* dis ctrl reg w pulse */
	/*if (is_meson_g9tv_cpu() || is_meson_m8_cpu() ||
	 *	is_meson_m8m2_cpu() || is_meson_gxbb_cpu() ||
	 *	is_meson_m8b_cpu())
	 */
	//wr_bits(offset, VDIN_WR_CTRL, 1,
	//	VDIN_WR_CTRL_REG_PAUSE_BIT, VDIN_WR_CTRL_REG_PAUSE_WID);
	/*  swap the 2 64bits word in 128 words */
	/*if (is_meson_gxbb_cpu())*/
	if (devp->set_canvas_manual == 1 || devp->cfg_dma_buf ||
		devp->work_mode == VDIN_WORK_MD_V4L || devp->dbg_no_swap_en) {
		/*not swap 2 64bits words in 128 words */
		wr_bits(offset, VDIN0_WRMIF_CTRL, 0, 31, 1);
		/*little endian*/
		wr_bits(offset, VDIN0_WRMIF_CTRL2, 1, 25, 1);
	} else {
		wr_bits(offset, VDIN0_WRMIF_CTRL, 1, 31, 1);
	}
	wr_bits(offset, VDIN0_WRMIF_CTRL, 1, 19, 1);
}

void vdin_set_wr_ctrl_vsync_t3x(struct vdin_dev_s *devp,
			    unsigned int offset,
			    enum vdin_format_convert_e format_convert,
			    unsigned int full_pack,
			    unsigned int source_bitdepth,
			    unsigned int rdma_enable)
{
	enum vdin_mif_fmt write_fmt = MIF_FMT_YUV422;
	unsigned int swap_cbcr = 0;
	unsigned int hconv_mode = 2, vconv_mode;

	switch (format_convert)	{
	case VDIN_FORMAT_CONVERT_YUV_YUV422:
	case VDIN_FORMAT_CONVERT_RGB_YUV422:
	case VDIN_FORMAT_CONVERT_GBR_YUV422:
	case VDIN_FORMAT_CONVERT_BRG_YUV422:
		write_fmt = MIF_FMT_YUV422;
		break;

	case VDIN_FORMAT_CONVERT_YUV_NV12:
	case VDIN_FORMAT_CONVERT_RGB_NV12:
		write_fmt = MIF_FMT_NV12_21;
		swap_cbcr = 1;

		break;
	case VDIN_FORMAT_CONVERT_YUV_NV21:
	case VDIN_FORMAT_CONVERT_RGB_NV21:
		write_fmt = MIF_FMT_NV12_21;
		swap_cbcr = 0;
		break;

	default:
		write_fmt = MIF_FMT_YUV444;
		break;
	}

	/* yuv422 full pack mode for 10bit
	 * only support 8bit at vpp side when double write
	 */
	if ((format_convert == VDIN_FORMAT_CONVERT_YUV_YUV422 ||
	     format_convert == VDIN_FORMAT_CONVERT_RGB_YUV422 ||
	     format_convert == VDIN_FORMAT_CONVERT_GBR_YUV422 ||
	     format_convert == VDIN_FORMAT_CONVERT_BRG_YUV422) &&
	    full_pack == VDIN_422_FULL_PK_EN && source_bitdepth > 8 &&
	    vdin_is_support_10bit_for_dw_t3x(devp)) {
		write_fmt = MIF_FMT_YUV422_FULL_PACK;

		/* IC bug, fixed at tm2 revB */
		if (devp->dtdata->hw_ver == VDIN_HW_ORG)
			hconv_mode = 0;
	}

	/* vconv_mode */
	vconv_mode = 0;

	if (write_fmt == MIF_FMT_NV12_21)
		vconv_mode = 0;
	else if (write_fmt == MIF_FMT_YUV444)
		vconv_mode = 3;
	else
		swap_cbcr = 0;

#ifdef CONFIG_AMLOGIC_MEDIA_RDMA
	if (rdma_enable) {
		rdma_write_reg_bits(devp->rdma_handle,
				    VDIN0_WRMIF_CTRL + devp->addr_offset,
				    hconv_mode, 26, 2);
		rdma_write_reg_bits(devp->rdma_handle,
				    VDIN0_WRMIF_CTRL + devp->addr_offset,
				    vconv_mode, 28, 2);
		rdma_write_reg_bits(devp->rdma_handle,
				    VDIN0_WRMIF_CTRL + devp->addr_offset,
				    swap_cbcr, 30, 1);
		rdma_write_reg_bits(devp->rdma_handle,
				    VDIN0_WRMIF_CTRL + devp->addr_offset,
				    write_fmt, WR_FMT_BIT, WR_FMT_WID);
	} else {
#endif
		wr_bits(offset, VDIN0_WRMIF_CTRL, hconv_mode,
			26, 2);
		wr_bits(offset, VDIN0_WRMIF_CTRL, vconv_mode,
			28, 2);
		wr_bits(offset, VDIN0_WRMIF_CTRL, swap_cbcr,
			30, 1);
		wr_bits(offset, VDIN0_WRMIF_CTRL, write_fmt,
			WR_FMT_BIT, WR_FMT_WID);
#ifdef CONFIG_AMLOGIC_MEDIA_RDMA
	}
#endif
}

/* set vdin_wr_mif for video only */
void vdin_set_wr_mif_t3x(struct vdin_dev_s *devp)
{
//	int height, width;
//	static unsigned int temp_height;
//	static unsigned int temp_width;
//
//	height = ((rd(0, VPP_POSTBLEND_VD1_V_START_END) & 0xfff) -
//		((rd(0, VPP_POSTBLEND_VD1_V_START_END) >> 16) & 0xfff) + 1);
//	width = ((rd(0, VPP_POSTBLEND_VD1_H_START_END) & 0xfff) -
//		((rd(0, VPP_POSTBLEND_VD1_H_START_END) >> 16) & 0xfff) + 1);
//	if (devp->parm.port == TVIN_PORT_VIU1_VIDEO &&
//	    devp->index == 1 &&
//	    height != temp_height &&
//	    width != temp_width) {
//		if ((width % 2) &&
//		    devp->source_bitdepth > VDIN_MIN_SOURCE_BITDEPTH &&
//		    (devp->full_pack == VDIN_422_FULL_PK_EN) &&
//		    (devp->format_convert == VDIN_FORMAT_CONVERT_YUV_YUV422 ||
//		    devp->format_convert == VDIN_FORMAT_CONVERT_RGB_YUV422 ||
//		    devp->format_convert == VDIN_FORMAT_CONVERT_GBR_YUV422 ||
//		    devp->format_convert == VDIN_FORMAT_CONVERT_BRG_YUV422))
//			width += 1;
//
//		wr_bits(devp->addr_offset, VDIN_WR_H_START_END,
//			(width - 1), WR_HEND_BIT, WR_HEND_WID);
//		wr_bits(devp->addr_offset, VDIN_WR_V_START_END,
//			(height - 1), WR_VEND_BIT, WR_VEND_WID);
//		temp_height = height;
//		temp_width = width;
//	}
}

void vdin_set_mif_on_off_t3x(struct vdin_dev_s *devp, unsigned int rdma_enable)
{
	unsigned int offset = devp->addr_offset;

	if (devp->vframe_wr_en_pre == devp->vframe_wr_en)
		return;

	#if CONFIG_AMLOGIC_MEDIA_RDMA
	if (rdma_enable) {
		rdma_write_reg_bits(devp->rdma_handle, VDIN0_PP_CTRL + offset,
			devp->vframe_wr_en ? 0 : 1, 21, 1);
		rdma_write_reg_bits(devp->rdma_handle, VDIN0_WRMIF_CTRL + offset,
			devp->vframe_wr_en, WR_REQ_EN_BIT, WR_REQ_EN_WID);
		rdma_write_reg_bits(devp->rdma_handle, VDIN0_CORE_CTRL + offset,
			devp->vframe_wr_en, 6, 1);
	}
	#endif
	devp->vframe_wr_en_pre = devp->vframe_wr_en;
}

/***************************global function**********************************/
unsigned int vdin_get_meas_h_cnt64_t3x(unsigned int offset)
{
	return 0;
}

unsigned int vdin_get_meas_v_stamp_t3x(struct vdin_dev_s *devp)
{
	unsigned int low_cnt, high_cnt;

	low_cnt  = rd(0, VDIN_INTF_MEAS_IND_TOTAL_COUNT0);
	high_cnt = rd(0, VDIN_INTF_MEAS_IND_TOTAL_COUNT1);

	if (vdin_isr_monitor & VDIN_ISR_MONITOR_CYCLE)
		pr_info("low_cnt = %#x,high_cnt = %#x\n",
			low_cnt, high_cnt);
	/* [23:0] */
	low_cnt  = low_cnt  & 0xffffff;
	high_cnt = high_cnt & 0xffffff;

	return ((high_cnt << 24) | low_cnt) / 2;
}

unsigned int vdin_get_active_h_t3x(unsigned int offset)
{
	return rd_bits(offset, VDIN0_SYNC_CONVERT_ACTIVE_MAX_PIX_CNT_STATUS,
		       ACTIVE_MAX_PIX_CNT_SDW_BIT, ACTIVE_MAX_PIX_CNT_SDW_WID);
}

unsigned int vdin_get_active_v_t3x(unsigned int offset)
{
	return rd_bits(offset, VDIN0_SYNC_CONVERT_LINE_CNT_SHADOW_STATUS,
		       ACTIVE_LN_CNT_SDW_BIT, ACTIVE_LN_CNT_SDW_WID);
}

unsigned int vdin_get_total_v_t3x(unsigned int offset)
{
	return rd_bits(offset, VDIN0_SYNC_CONVERT_LINE_CNT_SHADOW_STATUS,
		       GO_LN_CNT_SDW_BIT, GO_LN_CNT_SDW_WID);
}

void vdin_set_frame_mif_write_addr_t3x(struct vdin_dev_s *devp,
			unsigned int rdma_enable,
			struct vf_entry *vfe)
{
	u32 stride_luma, stride_chroma = 0;
	u32 hsize;
	u32 phy_addr_luma = 0, phy_addr_chroma = 0;
	bool reg_en = true;

	if (devp->vf_mem_size_small)
		hsize = devp->h_shrink_out;
	else
		hsize = devp->h_active;

	stride_luma = devp->canvas_w >> 4;
	if (vfe->vf.plane_num == 2)
		stride_chroma = devp->canvas_w >> 4;

	/* one region mode only have one buffer RGB,YUV */
	phy_addr_luma = vfe->vf.canvas0_config[0].phy_addr;
	/* two region mode have Y and uv two buffer(NV12, NV21) */
	if (vfe->vf.plane_num == 2)
		phy_addr_chroma = vfe->vf.canvas0_config[1].phy_addr;

	if (vdin_isr_monitor & VDIN_ISR_MONITOR_BUFFER) {
		pr_info("vdin%d,phy addr luma:0x%x chroma:0x%x\n", devp->index,
			phy_addr_luma, phy_addr_chroma);
		pr_info("vf[%d],stride luma:0x%x, chroma:0x%x\n", vfe->vf.index,
			stride_luma, stride_chroma);
	}

	if (devp->pause_dec || devp->debug.pause_mif_dec)
		reg_en = false;

	if (rdma_enable) {
		rdma_write_reg(devp->rdma_handle,
			       VDIN0_WRMIF_BADDR_LUMA + devp->addr_offset,
			       phy_addr_luma >> 4);
		rdma_write_reg(devp->rdma_handle,
			       VDIN0_WRMIF_STRIDE_LUMA + devp->addr_offset,
			       stride_luma);

		if (vfe->vf.plane_num == 2) {
			rdma_write_reg(devp->rdma_handle,
				       VDIN0_WRMIF_BADDR_CHROMA + devp->addr_offset,
				       phy_addr_chroma >> 4);
			rdma_write_reg(devp->rdma_handle,
				       VDIN0_WRMIF_STRIDE_CHROMA + devp->addr_offset,
				       stride_chroma);
		}
		rdma_write_reg_bits(devp->rdma_handle,
			VDIN0_CORE_CTRL + devp->addr_offset, reg_en, 6, 1);
		rdma_write_reg_bits(devp->rdma_handle,
			VDIN0_WRMIF_CTRL + devp->addr_offset, reg_en,
			WR_REQ_EN_BIT, WR_REQ_EN_WID);
	} else {
		wr(devp->addr_offset, VDIN0_WRMIF_BADDR_LUMA, phy_addr_luma >> 4);
		wr(devp->addr_offset, VDIN0_WRMIF_STRIDE_LUMA, stride_luma);

		if (vfe->vf.plane_num == 2) {
			wr(devp->addr_offset, VDIN0_WRMIF_BADDR_CHROMA,
			   phy_addr_chroma >> 4);
			wr(devp->addr_offset, VDIN0_WRMIF_STRIDE_CHROMA,
			   stride_chroma);
		}
		/* reg_dw_path_en */
		wr_bits(devp->addr_offset, VDIN0_CORE_CTRL, reg_en, 6, 1);
		/* reg_wr_req_en */
		wr_bits(devp->addr_offset, VDIN0_WRMIF_CTRL, reg_en,
			WR_REQ_EN_BIT, WR_REQ_EN_WID);
	}
}

void vdin_set_chroma_canvas_id_t3x(struct vdin_dev_s *devp, unsigned int rdma_enable,
		struct vf_entry *vfe)
{
	u32 canvas_id;

	if (!vfe)
		return;

	if (vfe->vf.canvas0Addr != (u32)-1)
		canvas_id = vfe->vf.canvas0Addr;
	else
		canvas_id =
		devp->vf_canvas_id[vfe->vf.index];
	canvas_id = (canvas_id >> 8);
#ifdef CONFIG_AMLOGIC_MEDIA_RDMA
	if (rdma_enable)
		rdma_write_reg_bits(devp->rdma_handle,
				    VDIN0_WRMIF_CTRL2 + devp->addr_offset,
				    canvas_id, WRITE_CHROMA_CANVAS_ADDR_BIT,
				    WRITE_CHROMA_CANVAS_ADDR_WID);
	else
#endif
		wr_bits(devp->addr_offset, VDIN0_WRMIF_CTRL2, canvas_id,
			WRITE_CHROMA_CANVAS_ADDR_BIT,
			WRITE_CHROMA_CANVAS_ADDR_WID);
}

void vdin_set_canvas_id_t3x(struct vdin_dev_s *devp, unsigned int rdma_enable,
			struct vf_entry *vfe)
{
	u32 canvas_id, vf_idx = -1;

	if (vfe) {
		if (vfe->vf.canvas0Addr != (u32)-1)
			canvas_id = vfe->vf.canvas0Addr;
		else
			canvas_id = devp->vf_canvas_id[vfe->vf.index];
		vf_idx = vfe->vf.index;
	} else {
		canvas_id = vdin_canvas_ids[devp->index][irq_max_count];
	}
	canvas_id = (canvas_id & 0xff);
	if (vdin_isr_monitor & VDIN_ISR_MONITOR_BUFFER) {
		pr_info("vdin%d,vf[%d],canvas_id:%d\n", devp->index,
			vf_idx, canvas_id);
	}
#ifdef CONFIG_AMLOGIC_MEDIA_RDMA
	if (rdma_enable) {
		rdma_write_reg_bits(devp->rdma_handle,
				    VDIN0_WRMIF_CTRL + devp->addr_offset,
				    canvas_id, WR_CANVAS_BIT, WR_CANVAS_WID);

		if (devp->pause_dec || devp->debug.pause_mif_dec)
			rdma_write_reg_bits(devp->rdma_handle, VDIN0_WRMIF_CTRL + devp->addr_offset,
					    0, WR_REQ_EN_BIT, WR_REQ_EN_WID);
		else
			rdma_write_reg_bits(devp->rdma_handle, VDIN0_WRMIF_CTRL + devp->addr_offset,
					    1, WR_REQ_EN_BIT, WR_REQ_EN_WID);
	} else {
#endif
		wr_bits(devp->addr_offset, VDIN0_WRMIF_CTRL, canvas_id,
			WR_CANVAS_BIT, WR_CANVAS_WID);

		if (devp->pause_dec || devp->debug.pause_mif_dec)
			wr_bits(devp->addr_offset, VDIN0_WRMIF_CTRL, 0,
				WR_REQ_EN_BIT, WR_REQ_EN_WID);
		else
			wr_bits(devp->addr_offset, VDIN0_WRMIF_CTRL, 1,
				WR_REQ_EN_BIT, WR_REQ_EN_WID);
#ifdef CONFIG_AMLOGIC_MEDIA_RDMA
	}
#endif
}

void vdin_pause_mif_write_t3x(struct vdin_dev_s *devp, unsigned int rdma_enable)
{
#ifdef CONFIG_AMLOGIC_MEDIA_RDMA
	if (rdma_enable) {
		rdma_write_reg_bits(devp->rdma_handle,
			VDIN0_CORE_CTRL + devp->addr_offset, 0, 6, 1);
		rdma_write_reg_bits(devp->rdma_handle, VDIN0_WRMIF_CTRL + devp->addr_offset,
			0, WR_REQ_EN_BIT, WR_REQ_EN_WID);
	}
	else
#endif
	{
		wr_bits(devp->addr_offset, VDIN0_CORE_CTRL, 0, 6, 1);
		wr_bits(devp->addr_offset, VDIN0_WRMIF_CTRL, 0,
			WR_REQ_EN_BIT, WR_REQ_EN_WID);
	}
}

void vdin_set_crc_pulse_t3x(struct vdin_dev_s *devp)
{
//#ifdef CONFIG_AMLOGIC_MEDIA_RDMA
//	if (devp->flags & VDIN_FLAG_RDMA_ENABLE) {
//		rdma_write_reg(devp->rdma_handle,
//			       VDIN_CRC_CHK + devp->addr_offset, 1);
//	} else {
//#endif
//		wr(devp->addr_offset, VDIN_CRC_CHK, 1);
//#ifdef CONFIG_AMLOGIC_MEDIA_RDMA
//	}
//#endif
}

/*set local dimming*/
#ifdef CONFIG_AML_LOCAL_DIMMING

#define VDIN_LDIM_PIC_ROWMAX 1080
#define VDIN_LDIM_PIC_COLMAX 1920
#define VDIN_LDIM_MAX_HIDX 4095
#define VDIN_LDIM_BLK_VNUM 2
#define VDIN_LDIM_BLK_HNUM 8

static void vdin_set_ldim_max_init_t3x(unsigned int offset,
				   int pic_h, int pic_v, int blk_vnum,
				   int blk_hnum)
{
//	int k;
//	struct ldim_max_s ldim_max;
//	int ldim_pic_rowmax = VDIN_LDIM_PIC_ROWMAX;
//	int ldim_pic_colmax = VDIN_LDIM_PIC_COLMAX;
//	int ldim_blk_vnum = VDIN_LDIM_BLK_VNUM;
//	int ldim_blk_hnum = VDIN_LDIM_BLK_HNUM;
//
//	ldim_pic_rowmax = pic_v;
//	ldim_pic_colmax = pic_h;
//	ldim_blk_vnum = blk_vnum; /* 8; */
//	ldim_blk_hnum = blk_hnum; /* 2; */
//	ldim_max.ld_stamax_hidx[0] = 0;
//
//	/* check ic type */
//	if (!is_meson_gxtvbb_cpu())
//		return;
//	if (vdin_ctl_dbg)
//		pr_info("\n****************%s:hidx start********\n", __func__);
//	for (k = 1; k < 11; k++) {
//		ldim_max.ld_stamax_hidx[k] =
//			((ldim_pic_colmax + ldim_blk_hnum - 1) /
//			ldim_blk_hnum) * k;
//		if (ldim_max.ld_stamax_hidx[k] > VDIN_LDIM_MAX_HIDX)
//			/* clip U12 */
//			ldim_max.ld_stamax_hidx[k] = VDIN_LDIM_MAX_HIDX;
//		if (ldim_max.ld_stamax_hidx[k] == ldim_pic_colmax)
//			ldim_max.ld_stamax_hidx[k] = ldim_pic_colmax - 1;
//		if (vdin_ctl_dbg)
//			pr_info("%d\t", ldim_max.ld_stamax_hidx[k]);
//	}
//	if (vdin_ctl_dbg)
//		pr_info("\n****************%s:hidx end*********\n", __func__);
//	ldim_max.ld_stamax_vidx[0] = 0;
//	if (vdin_ctl_dbg)
//		pr_info("\n***********%s:vidx start************\n", __func__);
//	for (k = 1; k < 11; k++) {
//		ldim_max.ld_stamax_vidx[k] = ((ldim_pic_rowmax +
//					      ldim_blk_vnum - 1) /
//					     ldim_blk_vnum) * k;
//		if (ldim_max.ld_stamax_vidx[k] > VDIN_LDIM_MAX_HIDX)
//			/* clip to U12 */
//			ldim_max.ld_stamax_vidx[k] = VDIN_LDIM_MAX_HIDX;
//		if (ldim_max.ld_stamax_vidx[k] == ldim_pic_rowmax)
//			ldim_max.ld_stamax_vidx[k] = ldim_pic_rowmax - 1;
//		if (vdin_ctl_dbg)
//			pr_info("%d\t", ldim_max.ld_stamax_vidx[k]);
//	}
//	if (vdin_ctl_dbg)
//		pr_info("\n*******%s:vidx end*******\n", __func__);
//	wr(offset, VDIN_LDIM_STTS_HIST_REGION_IDX,
//	   (1 << LOCAL_DIM_STATISTIC_EN_BIT)  |
//	   (0 << EOL_EN_BIT)                  |
//	   (2 << VLINE_OVERLAP_NUMBER_BIT)    |
//	   (1 << HLINE_OVERLAP_NUMBER_BIT)    |
//	   (1 << LPF_BEFORE_STATISTIC_EN_BIT) |
//	   (1 << REGION_RD_INDEX_INC_BIT));
//	wr_bits(offset, VDIN_LDIM_STTS_HIST_REGION_IDX, 0,
//		BLK_HV_POS_IDX_BIT, BLK_HV_POS_IDX_WID);
//	wr(offset, VDIN_LDIM_STTS_HIST_SET_REGION,
//	   ldim_max.ld_stamax_vidx[0] << 16 | ldim_max.ld_stamax_hidx[0]);
//	wr_bits(offset, VDIN_LDIM_STTS_HIST_REGION_IDX, 1,
//		BLK_HV_POS_IDX_BIT, BLK_HV_POS_IDX_WID);
//	wr(offset, VDIN_LDIM_STTS_HIST_SET_REGION,
//	   ldim_max.ld_stamax_hidx[2] << 16 | ldim_max.ld_stamax_hidx[1]);
//	wr_bits(offset, VDIN_LDIM_STTS_HIST_REGION_IDX, 2,
//		BLK_HV_POS_IDX_BIT, BLK_HV_POS_IDX_WID);
//	wr(offset, VDIN_LDIM_STTS_HIST_SET_REGION,
//	   ldim_max.ld_stamax_vidx[2] << 16 | ldim_max.ld_stamax_vidx[1]);
//	wr_bits(offset, VDIN_LDIM_STTS_HIST_REGION_IDX, 3,
//		BLK_HV_POS_IDX_BIT, BLK_HV_POS_IDX_WID);
//	wr(offset, VDIN_LDIM_STTS_HIST_SET_REGION,
//	   ldim_max.ld_stamax_hidx[4] << 16 | ldim_max.ld_stamax_hidx[3]);
//	wr_bits(offset, VDIN_LDIM_STTS_HIST_REGION_IDX, 4,
//		BLK_HV_POS_IDX_BIT, BLK_HV_POS_IDX_WID);
//	wr(offset, VDIN_LDIM_STTS_HIST_SET_REGION,
//	   ldim_max.ld_stamax_vidx[4] << 16 | ldim_max.ld_stamax_vidx[3]);
//	wr_bits(offset, VDIN_LDIM_STTS_HIST_REGION_IDX, 5,
//		BLK_HV_POS_IDX_BIT, BLK_HV_POS_IDX_WID);
//	wr(offset, VDIN_LDIM_STTS_HIST_SET_REGION,
//	   ldim_max.ld_stamax_hidx[6] << 16 | ldim_max.ld_stamax_hidx[5]);
//	wr_bits(offset, VDIN_LDIM_STTS_HIST_REGION_IDX, 6,
//		BLK_HV_POS_IDX_BIT, BLK_HV_POS_IDX_WID);
//	wr(offset, VDIN_LDIM_STTS_HIST_SET_REGION,
//	   ldim_max.ld_stamax_vidx[6] << 16 | ldim_max.ld_stamax_vidx[5]);
//	wr_bits(offset, VDIN_LDIM_STTS_HIST_REGION_IDX, 7,
//		BLK_HV_POS_IDX_BIT, BLK_HV_POS_IDX_WID);
//	wr(offset, VDIN_LDIM_STTS_HIST_SET_REGION,
//	   ldim_max.ld_stamax_hidx[8] << 16 | ldim_max.ld_stamax_hidx[7]);
//	wr_bits(offset, VDIN_LDIM_STTS_HIST_REGION_IDX, 8,
//		BLK_HV_POS_IDX_BIT, BLK_HV_POS_IDX_WID);
//	wr(offset, VDIN_LDIM_STTS_HIST_SET_REGION,
//	   ldim_max.ld_stamax_vidx[8] << 16 | ldim_max.ld_stamax_vidx[7]);
//	wr_bits(offset, VDIN_LDIM_STTS_HIST_REGION_IDX, 9,
//		BLK_HV_POS_IDX_BIT, BLK_HV_POS_IDX_WID);
//	wr(offset, VDIN_LDIM_STTS_HIST_SET_REGION,
//	   ldim_max.ld_stamax_hidx[10] << 16 | ldim_max.ld_stamax_hidx[9]);
//	wr_bits(offset, VDIN_LDIM_STTS_HIST_REGION_IDX, 10,
//		BLK_HV_POS_IDX_BIT, BLK_HV_POS_IDX_WID);
//	wr(offset, VDIN_LDIM_STTS_HIST_SET_REGION,
//	   ldim_max.ld_stamax_vidx[10] << 16 | ldim_max.ld_stamax_vidx[9]);
//	wr_bits(offset, VDIN_HIST_CTRL, 3, 9, 2);
//	wr_bits(offset, VDIN_HIST_CTRL, 1, 8, 1);
}

#endif

//static unsigned int vdin_luma_max;

void vdin_set_vframe_prop_info_t3x(struct vframe_s *vf,
			       struct vdin_dev_s *devp)
{
	int i;
	unsigned int offset = devp->addr_offset;
	u64 divid;
	struct vframe_bbar_s bbar = {0};
//
//#ifdef CONFIG_AML_LOCAL_DIMMING
//	/*int i;*/
//#endif
	/* fetch hist info */
	/* vf->prop.hist.luma_sum   = READ_CBUS_REG_BITS(VDIN_HIST_SPL_VAL,
	 * HIST_LUMA_SUM_BIT,    HIST_LUMA_SUM_WID   );
	 */
	vf->prop.hist.hist_pow   = rd_bits(offset, VDIN0_LUMA_HIST_CTRL,
					   HIST_POW_BIT, HIST_POW_WID);
	vf->prop.hist.luma_sum   = rd(offset, VDIN0_LUMA_HIST_SPL_VAL);
	/* vf->prop.hist.chroma_sum = READ_CBUS_REG_BITS(VDIN_HIST_CHROMA_SUM,
	 * HIST_CHROMA_SUM_BIT,  HIST_CHROMA_SUM_WID );
	 */
	vf->prop.hist.chroma_sum = rd(offset, VDIN0_LUMA_HIST_CHROMA_SUM);
	vf->prop.hist.pixel_sum  = rd(offset, VDIN0_LUMA_HIST_SPL_CNT) & 0xffffff;
	vf->prop.hist.height     =
		rd_bits(offset, VDIN0_LUMA_HIST_V_START_END, HIST_VEND_BIT, HIST_VEND_WID) -
		rd_bits(offset, VDIN0_LUMA_HIST_V_START_END, HIST_VSTART_BIT, HIST_VSTART_WID) + 1;
	vf->prop.hist.width      =
		rd_bits(offset, VDIN0_LUMA_HIST_H_START_END, HIST_HEND_BIT, HIST_HEND_WID) -
		rd_bits(offset, VDIN0_LUMA_HIST_H_START_END, HIST_HSTART_BIT, HIST_HSTART_WID) + 1;
	vf->prop.hist.luma_max   = rd_bits(offset, VDIN0_LUMA_HIST_MAX_MIN,
					HIST_MAX_BIT, HIST_MAX_WID);
	vf->prop.hist.luma_min   = rd_bits(offset, VDIN0_LUMA_HIST_MAX_MIN,
					HIST_MIN_BIT, HIST_MIN_WID);
	for (i = 0; i < 64; i++) {
		wr(offset, VDIN0_LUMA_HIST_DNLP_IDX, i);
		vf->prop.hist.gamma[i] = rd(offset, VDIN0_LUMA_HIST_DNLP_GRP);
	}
	if (vdin_isr_monitor & VDIN_ISR_MONITOR_HIST_INFO)
		pr_info("%s:vdin%d,wxh:%dx%d,sum[%d %d %d],min/max:%d/%d\n",
			__func__, devp->index, vf->prop.hist.width, vf->prop.hist.height,
			vf->prop.hist.pixel_sum, vf->prop.hist.luma_sum, vf->prop.hist.chroma_sum,
			vf->prop.hist.luma_min, vf->prop.hist.luma_max);

	/* fetch bbar info */
	bbar.top = rd_bits(offset, VDIN0_BLKBAR_RO_TOP_BOT_POS,
			   BLKBAR_TOP_POS_BIT, BLKBAR_TOP_POS_WID);
	bbar.bottom = rd_bits(offset, VDIN0_BLKBAR_RO_TOP_BOT_POS,
			      BLKBAR_BTM_POS_BIT,   BLKBAR_BTM_POS_WID);
	bbar.left = rd_bits(offset, VDIN0_BLKBAR_RO_LEF_RIG_POS,
			    BLKBAR_LEFT_POS_BIT, BLKBAR_LEFT_POS_WID);
	bbar.right = rd_bits(offset, VDIN0_BLKBAR_RO_LEF_RIG_POS,
			     BLKBAR_RIGHT_POS_BIT, BLKBAR_RIGHT_POS_WID);
	if (bbar.top > bbar.bottom) {
		bbar.top = 0;
		bbar.bottom = vf->height - 1;
	}
	if (bbar.left > bbar.right) {
		bbar.left = 0;
		bbar.right = vf->width - 1;
	}
	if (vdin_isr_monitor & VDIN_ISR_MONITOR_BLACK_BAR)
		pr_info("%s:vdin%d,[%d %d %d %d]", __func__, devp->index,
			bbar.top, bbar.bottom, bbar.left, bbar.right);

	/* Update Histgram windown with detected BlackBar window */
	if (devp->hist_bar_enable)
		vdin_set_histogram_t3x(offset, bbar.left, bbar.right, bbar.top, bbar.bottom);
	else
		vdin_set_histogram_t3x(offset, 0, vf->width - 1, 0, vf->height - 1);

	if (devp->black_bar_enable) {
		vf->prop.bbar.top        = bbar.top;
		vf->prop.bbar.bottom     = bbar.bottom;
		vf->prop.bbar.left       = bbar.left;
		vf->prop.bbar.right      = bbar.right;
	} else {
		memset(&vf->prop.bbar, 0, sizeof(struct vframe_bbar_s));
	}

	/* fetch meas info - For M2 or further chips only, not for M1 chip */
	vf->prop.meas.vs_stamp = devp->stamp;
	vf->prop.meas.vs_cycle = devp->cycle;
	if ((vdin_ctl_dbg & CTL_DEBUG_LUMA_MAX) &&
	    vdin_luma_max != vf->prop.hist.luma_max) {
		vf->ready_clock_hist[0] = sched_clock();
		divid = vf->ready_clock_hist[0];
		do_div(divid, 1000);
		pr_info("vdin write done %lld us. lum_max(0x%x-->0x%x)\n",
			divid, vdin_luma_max, vf->prop.hist.luma_max);
		vdin_luma_max = vf->prop.hist.luma_max;
	}
}

void vdin_get_crc_val_t3x(struct vframe_s *vf, struct vdin_dev_s *devp)
{
	/* fetch CRC value of the previous frame */
	vf->crc = rd(devp->addr_offset, VDIN0_PP_CRC_OUT);
}

static inline ulong vdin_reg_limit_t3x(ulong val, ulong wid)
{
	if (val < (1 << wid))
		return val;
	else
		return (1 << wid) - 1;
}

void vdin_set_all_regs_t3x(struct vdin_dev_s *devp)
{
	/* matrix sub-module */
	vdin_set_matrix_t3x(devp);

	/* bbar sub-module */
	vdin_set_bbar_t3x(devp, devp->v_active, devp->h_active);
#ifdef CONFIG_AML_LOCAL_DIMMING
	/* ldim sub-module */
	/* vdin_set_ldim_max_init(devp->addr_offset, 1920, 1080, 8, 2); */
	vdin_set_ldim_max_init_t3x(devp->addr_offset, devp->h_active,
			       devp->v_active,
			       VDIN_LDIM_BLK_HNUM, VDIN_LDIM_BLK_VNUM);
#endif

	/* hist sub-module */
	vdin_set_histogram_t3x(devp->addr_offset, 0,
		devp->h_active - 1, 0, devp->v_active - 1);
	/* hist mux select */
	vdin_set_hist_mux_t3x(devp);
	/* write sub-module */
	vdin_set_wr_ctrl_t3x(devp, devp->addr_offset, devp->v_active,
			devp->h_active, devp->format_convert,
			devp->full_pack, devp->source_bitdepth);
	/* top sub-module */
	vdin_set_top_t3x(devp, devp->parm.port,
		     devp->prop.color_format,
		     devp->bt_path);

	vdin_set_meas_mux_t3x(devp);

//	/* for t7 vdin2 write meta data */
//	if (devp->dtdata->hw_ver == VDIN_HW_T7) {
//		vdin_wrmif2_initial(devp);
//		vdin_wrmif2_addr_update(devp);
//		vdin_wrmif2_enable(devp, 0);
//	}
}

void vdin_set_dv_tunnel_t3x(struct vdin_dev_s *devp)
{
	if (!IS_HDMI_SRC(devp->parm.port))
		return;

	if (vdin_dv_is_need_tunnel(devp)) {
		wr(0, VPU_VDIN_HDMI0_TUNNEL, 0x80304512);
		if (vdin_dbg_en)
			pr_info("vdin%d,enable hdmi_if tunnel\n", devp->index);
	} else {
		if (vdin_dbg_en)
			pr_info("vdin%d,disable hdmi_if tunnel\n", devp->index);
		wr(0, VPU_VDIN_HDMI0_TUNNEL, 0x0);
	}
}

static void vdin_delay_line_t3x(struct vdin_dev_s *devp, unsigned short num)
{
	unsigned int offset;

	offset = devp->index * VDIN_TOP_OFFSET;
	wr_bits(offset, VDIN0_SYNC_CONVERT_SYNC_CTRL0, num,
		DLY_GO_FIELD_LINES_BIT, DLY_GO_FIELD_LINES_WID);
	if (num)
		wr_bits(offset, VDIN0_SYNC_CONVERT_SYNC_CTRL0, 1,
			DLY_GO_FIELD_EN_BIT, DLY_GO_FIELD_EN_WID);
	else
		wr_bits(offset, VDIN0_SYNC_CONVERT_SYNC_CTRL0, 0,
			DLY_GO_FIELD_EN_BIT, DLY_GO_FIELD_EN_WID);
}

//enable dw and afbce
void vdin_set_double_write_regs_t3x(struct vdin_dev_s *devp)
{
	unsigned int offset = 0;

	offset = devp->addr_offset;
	if (devp->double_wr && devp->hw_core == VDIN_HW_CORE_NORMAL) {
		/* reg_pp_path_en */
		wr_bits(offset, VDIN0_CORE_CTRL, 1, 5, 1);
		/* reg_dw_path_en */
		wr_bits(offset, VDIN0_CORE_CTRL, 1, 6, 1);
		/* reg_dith_path_en */
		wr_bits(offset, VDIN0_CORE_CTRL, 1, 7, 1);
		/* reg_afbce_path_en */
		wr_bits(offset, VDIN0_CORE_CTRL, 0, 8, 1);
		/* reg_fix_disable */
		wr_bits(offset, VDIN0_DW_CTRL, 0, 30, 2);
		wr_bits(offset, VDIN0_AFBCE_ENABLE, 0, AFBCE_EN_BIT, AFBCE_EN_WID);
	} else { //TODO:if di_path enabled,we need enable dith_path
		/* reg_dith_path_en */
		wr_bits(offset, VDIN0_CORE_CTRL, 0, 7, 1);
	}
}

/* vdin interface default setting */
void vdin_if_default_t3x(struct vdin_dev_s *devp)
{
	wr(devp->addr_offset, VDIN0_CORE_CTRL, (1 << 5) | (1 << 7));
	//sync convert
	wr(devp->index * VDIN_TOP_OFFSET, VDIN0_SYNC_CONVERT_SECURE_CTRL, 0x0);
	wr(devp->index * VDIN_TOP_OFFSET, VDIN0_SYNC_CONVERT_CTRL, 0x0);
	wr(devp->index * VDIN_TOP_OFFSET, VDIN0_SYNC_CONVERT_SYNC_CTRL0, 0x1);
	wr(devp->index * VDIN_TOP_OFFSET, VDIN0_SYNC_CONVERT_SYNC_CTRL1, 0x4040);
	wr(devp->index * VDIN_TOP_OFFSET, VDIN0_SYNC_CONVERT_LINE_INT_CTRL, 0x0);
}

/* vdin pp default setting */
void vdin_pp_default_t3x(struct vdin_dev_s *devp)
{
	unsigned int offset;
	unsigned int lfifo_buf_size = 0;

	offset = devp->addr_offset;
	if (devp->hw_core == VDIN_HW_CORE_NORMAL)
		lfifo_buf_size = devp->dtdata->vdin0_line_buff_size;
	else
		lfifo_buf_size = devp->dtdata->vdin1_line_buff_size;

	wr(offset, VDIN0_PP_GCLK_CTRL,	  0x0);
	wr(offset, VDIN0_PP_CTRL,	  0x0);
	wr(offset, VDIN0_PP_IN_SIZE,	  0x0);
	wr(offset, VDIN0_PP_OUT_SIZE,	  0x0);
	wr(offset, VDIN0_PP_SCL_IN_SIZE,  0x0);
	wr(offset, VDIN0_PP_SCL_OUT_SIZE, 0x0);

	wr(offset, VDIN0_LFIFO_CTRL,
		(0 << PP_LFIFO_GCLK_CTRL_BIT) |
		(1 << PP_LFIFO_SOFT_RST_BIT)  |
		(lfifo_buf_size << PP_LFIFO_BUF_SIZE_BIT) |
		(0xc281 << PP_LFIFO_URG_CTRL_BIT));
}

void vdin_dw_default_t3x(struct vdin_dev_s *devp)
{
	unsigned int offset;

	offset = devp->addr_offset;
	wr(offset, VDIN0_DW_GCLK_CTRL,	0x0);
	wr(offset, VDIN0_DW_CTRL,	0x0);
	wr(offset, VDIN0_DW_IN_SIZE,	0x0);
	wr(offset, VDIN0_DW_OUT_SIZE,	0x0);
}

void vdin_local_arb_default_t3x(struct vdin_dev_s *devp)
{
	//TODO
}

void vdin_set_default_regmap_t3x(struct vdin_dev_s *devp)
{
	vdin_if_default_t3x(devp);
	vdin_pp_default_t3x(devp);
	vdin_dw_default_t3x(devp);
	vdin_local_arb_default_t3x(devp);

	vdin_delay_line_t3x(devp, delay_line_num);
}

/*
 * this config for filter unstable vysnc especially atv unplug
 */
static void filter_unstable_vsync_t3x(struct vdin_dev_s *devp)
{
	unsigned int offset = devp->addr_offset;

	if (devp->index || devp->debug.bypass_filter_vsync ||
	    vdin_is_convert_to_nv21(devp->format_convert))
		return;

	wr_bits(offset, VDIN0_SYNC_CONVERT_SYNC_CTRL0, 1, HSYNC_MASK_EN_BIT, HSYNC_MASK_EN_WID);
	wr_bits(offset, VDIN0_SYNC_CONVERT_SYNC_CTRL0, 1, VSYNC_MASK_EN_BIT, VSYNC_MASK_EN_WID);
	wr_bits(offset, VDIN0_WRMIF_CTRL2, 1, T3X_WR_DATA_EXT_EN_BIT, T3X_WR_DATA_EXT_EN_WID);
	wr_bits(offset, VDIN0_WRMIF_CTRL2, 7, T3X_WR_WORDS_LIM_BIT, T3X_WR_WORDS_LIM_WID);
	wr_bits(offset, VDIN0_WRMIF_CTRL3, 1, VDIN0_WRMIF_CTRL3_31_BIT, VDIN0_WRMIF_CTRL3_31_WID);
	wr_bits(offset, VDIN0_WRMIF_CTRL3, 1, VDIN0_WRMIF_CTRL3_1_BIT, VDIN0_WRMIF_CTRL3_1_WID);
	wr_bits(offset, VDIN0_WRMIF_CTRL3, 0, VDIN0_WRMIF_CTRL3_3_BIT, VDIN0_WRMIF_CTRL3_3_WID);
	wr_bits(offset, VDIN0_WRMIF_CTRL, 0, T3X_EOL_SEL_BIT, T3X_EOL_SEL_WID);
}

void vdin_hw_enable_t3x(struct vdin_dev_s *devp)
{
	unsigned int offset = devp->addr_offset;

	/* enable video data input */
	/* [    4]  top.datapath_en  = 1 */
	//wr_bits(offset, VDIN_COM_CTRL0, 1,
	//	VDIN_COMMON_INPUT_EN_BIT, VDIN_COMMON_INPUT_EN_WID);

	vdin_clk_on_off_t3x(devp, true);
	filter_unstable_vsync_t3x(devp);
	/* req_en */
	wr_bits(offset, VDIN0_WRMIF_CTRL, 0, WR_REQ_EN_BIT, WR_REQ_EN_WID);
	/* reg_enable */
	wr_bits(devp->index * VDIN_TOP_OFFSET, VDIN0_SYNC_CONVERT_CTRL, 1, 0, 1);
	/* reg_secure_sel */
	//wr_bits(offset, VDIN0_SYNC_CONVERT_SECURE_CTRL, 11, 0, 4);

	/* wr(offset, VDIN_COM_GCLK_CTRL, 0x0); */
}

/*
 * 1) disable cm2
 * 2) disable video input and set mux input to null
 * 3) set delay line number
 * 4) disable vdin clk
 */
void vdin_hw_disable_t3x(struct vdin_dev_s *devp)
{
	/* reg_dw_path_en */
	wr_bits(devp->addr_offset, VDIN0_CORE_CTRL, 0, 6, 1);
	/* req_en */
	wr_bits(devp->addr_offset, VDIN0_WRMIF_CTRL, 0, WR_REQ_EN_BIT, WR_REQ_EN_WID);
	/* reg_enable */
	wr_bits(devp->index * VDIN_TOP_OFFSET, VDIN0_SYNC_CONVERT_CTRL, 0, 0, 1);
	/* reg_secure_sel */
	wr_bits(devp->index * VDIN_TOP_OFFSET, VDIN0_SYNC_CONVERT_SECURE_CTRL, 11, 0, 4);

	vdin_clk_on_off_t3x(devp, false);

	/*if (devp->dtdata->de_tunnel_tunnel) */{
		//todo vdin_dolby_desc_to_4448bit(devp, 0);
		//todo vdin_dolby_de_tunnel_to_44410bit(devp, 0);
	}
}

bool vdin_check_vdi6_afifo_overflow_t3x(unsigned int offset)
{
	//return rd_bits(offset, VDIN_COM_STATUS2, 15, 1);
	return 0;
}

void vdin_clear_vdi6_afifo_overflow_flg_t3x(unsigned int offset)
{
//	wr_bits(offset, VDIN_ASFIFO_CTRL3, 0x1, 1, 1);
//	wr_bits(offset, VDIN_ASFIFO_CTRL3, 0x0, 1, 1);
}

//static unsigned int vdin_reset_flag;
inline int vdin_vsync_reset_mif_t3x(int index)
{
//	int i;
//	int start_line = aml_read_vcbus(VDIN_LCNT_STATUS) & 0xfff;
//
//	if (!enable_reset || vdin_reset_flag || start_line > 0)
//		return 0;
//
//	vdin_reset_flag = 1;
//	if (index == 0) {
//		/* vdin->vdin mif wr en */
//		W_VCBUS_BIT(VDIN_WR_CTRL, 0, VCP_WR_EN_BIT, VCP_WR_EN_WID);
//		W_VCBUS_BIT(VDIN_WR_CTRL, 1, NO_CLOCK_GATE_BIT,	NO_CLOCK_GATE_WID);
//		/* wr req en */
//		W_VCBUS_BIT(VDIN_WR_CTRL, 0, WR_REQ_EN_BIT, WR_REQ_EN_WID);
//		aml_write_vcbus(VPU_WRARB_REQEN_SLV_L1C2, vpu_reg_27af &
//						(~(1 << VDIN0_REQ_EN_BIT)));
//		vpu_reg_27af &= (~VDIN0_REQ_EN_BIT);
//		if (R_VCBUS_BIT(VPU_ARB_DBG_STAT_L1C2, VDIN_DET_IDLE_BIT,
//				VDIN_DET_IDLE_WIDTH) & VDIN0_IDLE_MASK) {
//			vdin0_wr_mif_reset_t3x();
//		} else {
//			for (i = 0; i < vdin_det_idle_wait; i++) {
//				if (R_VCBUS_BIT(VPU_ARB_DBG_STAT_L1C2,
//						VDIN_DET_IDLE_BIT,
//						VDIN_DET_IDLE_WIDTH) &
//				    VDIN0_IDLE_MASK) {
//					vdin0_wr_mif_reset_t3x();
//					break;
//				}
//			}
//			if (i >= vdin_det_idle_wait && vdin_ctl_dbg)
//				pr_info("============== !!! idle wait timeout\n");
//		}
//
//		aml_write_vcbus(VPU_WRARB_REQEN_SLV_L1C2,
//				vpu_reg_27af | (1 << VDIN0_REQ_EN_BIT));
//
//		W_VCBUS_BIT(VDIN_WR_CTRL, 1, WR_REQ_EN_BIT, WR_REQ_EN_WID);
//		W_VCBUS_BIT(VDIN_WR_CTRL, 0, NO_CLOCK_GATE_BIT,	NO_CLOCK_GATE_WID);
//		W_VCBUS_BIT(VDIN_WR_CTRL, 1, VCP_WR_EN_BIT, VCP_WR_EN_WID);
//
//		vpu_reg_27af |= VDIN0_REQ_EN_BIT;
//	} else if (index == 1) {
//		W_VCBUS_BIT(VDIN1_WR_CTRL2, 1, VDIN1_VCP_WR_EN_BIT,
//			    VDIN1_VCP_WR_EN_WID); /* vdin->vdin mif wr en */
//		W_VCBUS_BIT(VDIN1_WR_CTRL, 1, VDIN1_DISABLE_CLOCK_GATE_BIT,
//			    VDIN1_DISABLE_CLOCK_GATE_WID); /* clock gate */
//		/* wr req en */
//		W_VCBUS_BIT(VDIN1_WR_CTRL, 0, WR_REQ_EN_BIT, WR_REQ_EN_WID);
//		aml_write_vcbus(VPU_WRARB_REQEN_SLV_L1C2,
//				vpu_reg_27af & (~(1 << VDIN1_REQ_EN_BIT)));
//		vpu_reg_27af &= (~(1 << VDIN1_REQ_EN_BIT));
//		if (R_VCBUS_BIT(VPU_ARB_DBG_STAT_L1C2, VDIN_DET_IDLE_BIT,
//				VDIN_DET_IDLE_WIDTH) & VDIN1_IDLE_MASK) {
//			vdin1_wr_mif_reset_t3x();
//		} else {
//			for (i = 0; i < vdin_det_idle_wait; i++) {
//				if (R_VCBUS_BIT(VPU_ARB_DBG_STAT_L1C2,
//						VDIN_DET_IDLE_BIT,
//						VDIN_DET_IDLE_WIDTH) &
//				    VDIN1_IDLE_MASK) {
//					vdin1_wr_mif_reset_t3x();
//					break;
//				}
//			}
//		}
//		aml_write_vcbus(VPU_WRARB_REQEN_SLV_L1C2,
//				vpu_reg_27af | (1 << VDIN1_REQ_EN_BIT));
//		vpu_reg_27af |= (1 << VDIN1_REQ_EN_BIT);
//		W_VCBUS_BIT(VDIN1_WR_CTRL, 1, WR_REQ_EN_BIT, WR_REQ_EN_WID);
//		W_VCBUS_BIT(VDIN1_WR_CTRL, 0, VDIN1_DISABLE_CLOCK_GATE_BIT,
//			    VDIN1_DISABLE_CLOCK_GATE_WID);
//		W_VCBUS_BIT(VDIN1_WR_CTRL2, 0,
//			    VDIN1_VCP_WR_EN_BIT, VDIN1_VCP_WR_EN_WID);
//	}
//	vdin_reset_flag = 0;
//
//	return vsync_reset_mask & 0x08;
	return 0;
}

void vdin_enable_module_t3x(struct vdin_dev_s *devp, bool enable)
{
	//unsigned int offset = devp->addr_offset;

	W_VCBUS_BIT(VPU_WRARB_UGT_L2C1, 3,
		VPU_WRARB_UGT_VDIN_BIT, VPU_WRARB_UGT_VDIN_WID);

	if (enable)	{
		/* set VDIN_MEAS_CLK_CNTL, select XTAL clock */
		/* if (is_meson_gxbb_cpu()) */
		/* ; */
		/* else */
		/* aml_write_cbus(HHI_VDIN_MEAS_CLK_CNTL, 0x00000100); */
		/* vdin_hw_enable_t3x(devp); */
		/* todo: check them */
	} else {
		/* set VDIN_MEAS_CLK_CNTL, select XTAL clock */
		/* if (is_meson_gxbb_cpu()) */
		/* ; */
		/* else */
		/* aml_write_cbus(HHI_VDIN_MEAS_CLK_CNTL, 0x00000000); */
		vdin_hw_disable_t3x(devp);
		//todo vdin_dobly_mdata_write_en(offset, false);
	}
}

bool vdin_write_done_check_t3x(unsigned int offset, struct vdin_dev_s *devp)
{
	bool ret = false;

	devp->stats.write_done_check++;

	if (vdin_is_wrmif_done_t3x(devp)) {
		devp->stats.wmif_normal_cnt++;
		ret = true;
	} else {
		devp->stats.wmif_abnormal_cnt++;
		ret = false;
	}

#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
	if (devp->afbce_valid && devp->double_wr) { /* afbce */
		if (vdin_afbce_read_write_down_flag_t3x(devp)) {
			devp->stats.afbce_normal_cnt++;
			ret = true;
		} else {
			devp->stats.afbce_abnormal_cnt++;
			ret = false;
		}
	}
#endif

	vdin_clr_write_done_t3x(devp);

	if (vdin_isr_monitor & VDIN_ISR_MONITOR_WRITE_DONE)
		pr_info("%s vdin%d irq:%d,check:%d,afbce:%d %d mif:%d %d\n",
			__func__, devp->index, devp->stats.wr_done_irq_cnt,
			devp->stats.write_done_check,
			devp->stats.afbce_normal_cnt, devp->stats.afbce_abnormal_cnt,
			devp->stats.wmif_normal_cnt, devp->stats.wmif_abnormal_cnt);

	return ret;
}

/* just for horizontal down scale src_w is origin width,
 * dst_w is width after scale down
 */
static void vdin_set_hscale_t3x(struct vdin_dev_s *devp, unsigned int dst_w)
{
	unsigned int offset = devp->addr_offset;
	unsigned int src_w = devp->h_active;
	//unsigned int tmp;
	int phase_step, i;
	u32 hsc_integer, hsc_fraction;
	//u32 phsc_en = 0, phsc_mode = 0;
	int in_hsize1, in_hsize3, in_hsize7;

//	u32 coef_lut[3][33] = {
//		{ 0x00800000, 0x007f0100, 0xff7f0200, 0xfe7f0300,
//		  0xfd7e0500, 0xfc7e0600, 0xfb7d0800, 0xfb7c0900,
//		  0xfa7b0b00, 0xfa7a0dff, 0xf9790fff, 0xf97711ff,
//		  0xf87613ff, 0xf87416fe, 0xf87218fe, 0xf8701afe,
//		  0xf76f1dfd, 0xf76d1ffd, 0xf76b21fd, 0xf76824fd,
//		  0xf76627fc, 0xf76429fc, 0xf7612cfc, 0xf75f2ffb,
//		  0xf75d31fb, 0xf75a34fb, 0xf75837fa, 0xf7553afa,
//		  0xf8523cfa, 0xf8503ff9, 0xf84d42f9, 0xf84a45f9,
//	      0xf84848f8 }, // bicubic
//		{ 0x00800000, 0x007e0200, 0x007c0400, 0x007a0600,
//		  0x00780800, 0x00760a00, 0x00740c00, 0x00720e00,
//		  0x00701000, 0x006e1200, 0x006c1400, 0x006a1600,
//		  0x00681800, 0x00661a00, 0x00641c00, 0x00621e00,
//		  0x00602000, 0x005e2200, 0x005c2400, 0x005a2600,
//		  0x00582800, 0x00562a00, 0x00542c00, 0x00522e00,
//		  0x00503000, 0x004e3200, 0x004c3400, 0x004a3600,
//		  0x00483800, 0x00463a00, 0x00443c00, 0x00423e00,
//		  0x00404000 }, // 2 point bilinear
//		{ 0x80000000, 0x7e020000, 0x7c040000, 0x7a060000,
//		  0x78080000, 0x760a0000, 0x740c0000, 0x720e0000,
//		  0x70100000, 0x6e120000, 0x6c140000, 0x6a160000,
//		  0x68180000, 0x661a0000, 0x641c0000, 0x621e0000,
//		  0x60200000, 0x5e220000, 0x5c240000, 0x5a260000,
//		  0x58280000, 0x562a0000, 0x542c0000, 0x522e0000,
//		  0x50300000, 0x4e320000, 0x4c340000, 0x4a360000,
//		  0x48380000, 0x463a0000, 0x443c0000, 0x423e0000,
//		  0x40400000 }}; // 2 point bilinear, bank_length == 2
	int pps_lut_tap4[33][4] =    { {0,   128, 0,   0},
				       {-1,  128, 1,   0},
				       {-2,  128, 3,  -1},
				       {-3,  127, 4,   0},
				       {-4,  127, 6,  -1},
				       {-5,  126, 7,   0},
				       {-6,  126, 9,  -1},
				       {-7,  125, 11, -1},
				       {-8,  124, 13, -1},
				       {-8,  123, 15, -2},
				       {-9,  122, 17, -2},
				       {-9,  120, 19, -2},
				       {-10, 119, 21, -2},
				       {-10, 117, 23, -2},
				       {-10, 116, 25, -3},
				       {-11, 114, 28, -3},
				       {-11, 112, 30, -3},
				       {-11, 110, 33, -4},
				       {-11, 108, 35, -4},
				       {-12, 106, 38, -4},
				       {-11, 104, 40, -5},
				       {-12, 102, 43, -5},
				       {-12, 100, 46, -6},
				       {-11, 97,  48, -6},
				       {-12, 95,  51, -6},
				       {-11, 92,  54, -7},
				       {-11, 90,  57, -8},
				       {-11, 87,  59, -7},
				       {-10, 84,  62, -8},
				       {-10, 82,  65, -9},
				       {-10, 79,  68, -9},
				       {-10, 76,  71, -9},
				       {-9,  73,  73, -9}};

	if (!dst_w) {
		pr_err("[vdin..]%s parameter dst_w error.\n", __func__);
		return;
	}
	/* enable reg_hsc_gclk_ctrl */
	wr_bits(offset, VDIN0_PP_GCLK_CTRL, 0, PP_HSC_GCLK_CTRL_BIT, PP_HSC_GCLK_CTRL_WID);
	/* disable hscale */
	wr_bits(offset, VDIN0_PP_CTRL, 0, PP_HSC_EN_BIT, PP_HSC_EN_WID);
	/* write horz filter coefs */
	//wr_bits(offset, VDIN_HSC_COEF_IDX, 0x0100, 0, 7);
	wr(offset, VDIN0_CNTL_SCALE_COEF_IDX, (2 << 17) | (1 << 16) | (2 << 7));
	for (i = 0; i < 33; i++)
		wr(offset, VDIN0_CNTL_SCALE_COEF,
			((pps_lut_tap4[i][0] & 0xff) << 24) |
			((pps_lut_tap4[i][1] & 0xff) << 16) |
			((pps_lut_tap4[i][2] & 0xff) << 8)  |
			((pps_lut_tap4[i][3] & 0xff) << 0));

	in_hsize1 = src_w + 1;
	in_hsize3 = src_w + 3;
	in_hsize7 = src_w + 7;
	//src_w = (phsc_en) ? (phsc_mode == 0) ? src_w :
	//		    (phsc_mode == 1) ? ((in_hsize1) >> 1) :
	//		    (phsc_mode == 2) ? ((in_hsize3) >> 2) :
	//		    (phsc_mode == 3) ? ((in_hsize7) >> 3) : src_w : src_w;
	if (src_w >= 2048) {/* for src_w >= 4096, avoid data overflow. */
		phase_step = ((src_w << 18) / dst_w) << 2;
		//horz_phase_step = (horz_phase_step << 6);
	} else {
		phase_step = (src_w << 20) / dst_w;
		//horz_phase_step = (horz_phase_step << 4);
	}
	phase_step = (phase_step << 4);

	hsc_integer  = (phase_step & 0xf000000) >> 24;
	hsc_fraction = phase_step & 0x0ffffff;

	wr(offset, VDIN0_HSC_START_PHASE_STEP, (hsc_integer << 24) | (hsc_fraction << 0));

	wr(offset, VDIN0_HSC_REGION12_STARTP, 0);
	wr(offset, VDIN0_HSC_REGION34_STARTP, (dst_w << 16) | dst_w);
	wr(offset, VDIN0_HSC_REGION4_ENDP,
		(0 << 24)  | /* hsc_ini_phase1_exp */
		(0 << 16)  | /* hsc_ini_phase0_exp */
		(dst_w - 1));/* hsc_region4_endp */

	wr(offset, VDIN0_HSC_REGION0_PHASE_SLOPE, 0);
	wr(offset, VDIN0_HSC_REGION1_PHASE_SLOPE, 0);
	wr(offset, VDIN0_HSC_REGION3_PHASE_SLOPE, 0);
	wr(offset, VDIN0_HSC_REGION4_PHASE_SLOPE, 0);

	wr(offset, VDIN0_HSC_SC_MISC,
		(0 << 6) |  /* sc_coef_s11_mode */
		(0 << 5) |  /* hf_sep_coef_4srnet_en */
		(0 << 4) |  /* hsc_nonlinear_4region_en */
		(4 << 0));  /* hsc_bank_length */

	wr(offset, VDIN0_HSC_PHASE_CTRL,
		(0 << 24) |  /* hsc_ini_rcv_num0_exp */
		(1 << 20) |  /* hsc_rpt_p0_num0 */
		(4 << 16) |  /* hsc_ini_rcv_num0 */
		(0 << 0));   /* hsc_ini_phase0 */

	wr(offset, VDIN0_HSC_PHASE_CTRL1,
		(0 << 29) |  /* hsc_ini_rcv_num1_exp */
		(0 << 28) |  /* hsc_double_pix_mode */
		(1 << 20) |  /* hsc_rpt_p0_num1 */
		(4 << 16) |  /* hsc_ini_rcv_num1 */
		(0 << 0)); /* hsc_ini_phase1 */

	wr(offset, VDIN0_HSC_INI_PAT_CTRL, 0);

	wr(offset, VDIN0_HSC_EXTRA,
	   (0 << 17)	| /* gclk_ctrl */
	   (7 << 13)	| /* reg_hsc_nor_rs_bits */
	   (0 << 11)	| /* sp422_mode */
	   (0 << 10)	| /* phase0_always_en */
	   (1 << 9)	| /* nearest_en */
	   (1 << 8)	| /* short_line_o_en */
	   (0 << 0));	  /* reg_hsc_ini_pixi_ptr */
	/* enable hscale */
	wr_bits(offset, VDIN0_PP_CTRL, 1, PP_HSC_EN_BIT, PP_HSC_EN_WID);

	if (!(devp->dv_hw5.hw5_ctl & BIT0)) //if scaler in vdin_pp_top
		devp->h_active = dst_w;
}

/*
 *just for vertical scale src_w is origin height,
 *just dst_h is the height after scale
 */
static void vdin_set_vscale_t3x(struct vdin_dev_s *devp)
{
	int ver_phase_step;
	unsigned int offset = devp->addr_offset;
	unsigned int src_h = devp->v_active;
	unsigned int dst_h = devp->prop.scaling4h;

	if (!dst_h) {
		pr_err("[vdin..]%s parameter dst_h error.\n", __func__);
		return;
	}
	/* enable reg_hsc_gclk_ctrl */
	wr_bits(offset, VDIN0_PP_GCLK_CTRL, 0, PP_VSC_GCLK_CTRL_BIT, PP_VSC_GCLK_CTRL_WID);
	/* disable vscale */
	wr_bits(offset, VDIN0_PP_CTRL, 0, PP_VSC_EN_BIT, PP_VSC_EN_WID);

	if (src_h >= 2048)
		ver_phase_step = ((src_h << 18) / dst_h) << 2;
	else
		ver_phase_step = (src_h << 20) / dst_h;

	if ((ver_phase_step >> 25) != 0) {
		pr_err("%s,vdin%d phase_step=%#x.\n", __func__, devp->index, ver_phase_step);
		return;
	}
	wr(offset, VDIN0_VSC_PHASE_STEP, ver_phase_step);
	if (!(ver_phase_step >> 20)) {/* scale up the bit should be 1 */
		wr(offset, VDIN0_VSC_CTRL, (1 << 8));/* reg_vsc_phase0_always_en */
		/* scale phase is 0 */
		wr(offset, VDIN0_VSC_INI_PHASE, 0);
	} else {
		wr(offset, VDIN0_VSC_CTRL, 0);
		/* scale phase is 0x0 */
		wr(offset, VDIN0_VSC_INI_PHASE, 0);
	}
	/* skip 0 line in the beginning */
	wr_bits(offset, VDIN0_VSC_CTRL, 0,
		PP_VSC_SKIP_LINE_NUM_BIT, PP_VSC_SKIP_LINE_NUM_WID);

	//wr_bits(offset, VDIN_SCIN_HEIGHTM1, src_h - 1,
	//	SCALER_INPUT_HEIGHT_BIT, SCALER_INPUT_HEIGHT_WID);
	wr(offset, VDIN0_VSC_DUMMY_DATA, 0x8080);
	/* enable vscale */
	wr_bits(offset, VDIN0_PP_CTRL, 1, PP_VSC_EN_BIT, PP_VSC_EN_WID);

	if (!(devp->dv_hw5.hw5_ctl & BIT0)) //if scaler in vdin_pp_top
		devp->v_active = dst_h;
}

static void vdin_pre_hsc_param_init(struct vdin_dev_s *devp, struct vdin_pre_hsc_s *pre_hsc)
{
	pre_hsc->phsc_gclk_ctrl  = 0;
	//prehsc_mode,bit 3:2, prehsc odd line interp mode, bit 1:0, prehsc even line interp mode,
	//each 2bit, 00 pix0+pix1/2, average, 01:pix1, 10:pix0
	pre_hsc->prehsc_mode	 = 0;
	pre_hsc->prehsc_pattern  = 85;
	pre_hsc->prehsc_flt_num  = 4;
	pre_hsc->prehsc_rate	 = 1;//0:width,1:width/2,2:width/4,3:width/8
	pre_hsc->prehsc_pat_star = 0;
	pre_hsc->prehsc_pat_end  = 7;
	pre_hsc->preh_hb_num	 = 0;
	pre_hsc->preh_vb_num	 = 0;
	pre_hsc->prehsc_coef_0   = 128;
	pre_hsc->prehsc_coef_1   = 128;
	pre_hsc->prehsc_coef_2   = 0;
	pre_hsc->prehsc_coef_3   = 0;
}

/* new add pre_hscale module
 * do hscaler down when scaling4w is smaller than half of h_active
 * which is closed by default
 */
static void vdin_set_pre_h_scale_t3x(struct vdin_dev_s *devp)
{
	struct vdin_pre_hsc_s hsc;

	memset(&hsc, 0, sizeof(hsc));
	vdin_pre_hsc_param_init(devp, &hsc);

	wr(devp->addr_offset, VDIN0_PRE_HSCALE_INI_CTRL,
		(hsc.phsc_gclk_ctrl  << 22) |
		(hsc.prehsc_mode     << 17) |
		(hsc.prehsc_pattern  << 8) |
		(hsc.prehsc_pat_star << 4) |
		(hsc.prehsc_pat_end  << 0));

	wr(devp->addr_offset, VDIN0_PRE_HSCALE_COEF_0,
		(hsc.prehsc_coef_1  << 16) |
		(hsc.prehsc_coef_0  << 0));

	wr(devp->addr_offset, VDIN0_PRE_HSCALE_FLT_NUM,
		(hsc.preh_hb_num    << 22) |
		(hsc.preh_vb_num    << 17) |
		(hsc.prehsc_flt_num << 8) |
		(hsc.prehsc_rate    << 0));

	wr(devp->addr_offset, VDIN0_PRE_HSCALE_COEF_1,
		(hsc.prehsc_coef_3  << 16) |
		(hsc.prehsc_coef_2  << 0));

	wr_bits(devp->addr_offset, VDIN0_PP_CTRL, 1, PP_PHSC_EN_BIT,
		PP_PHSC_EN_WID);
	devp->h_active >>= 1;
	pr_info("set_prehsc done!h_active:%d\n", devp->h_active);
}

/*tm2 new add*/
static void vdin_set_h_shrink_t3x(struct vdin_dev_s *devp)
{
	unsigned int offset = devp->addr_offset;
	unsigned int src_w = devp->h_active;
	unsigned int dst_w = devp->h_shrink_out;
	unsigned int hshrk_mode = 0;
	unsigned int i = 0;
	unsigned int coef = 0;

	if (!cpu_after_eq(MESON_CPU_MAJOR_ID_TM2)) {
		pr_err("vdin.%d don't supports h_shrink before TM2\n",
		       devp->index);
		return;
	}

	if (devp->dv_hw5.hw5_ctl & BIT0) //if scaler in vdin_pp_top
		return;

	if (dst_w)
		hshrk_mode = src_w / dst_w;

	/*check maximum value*/
	if (hshrk_mode < 2 || hshrk_mode > 64) {
		pr_err("vdin.%d h_shrink: size is out of range\n", devp->index);
		return;
	}
	pr_err("%s vdin.%d h:%d,v:%d,src:%d %d;mode:%d\n", __func__,
		   devp->index, devp->h_active, devp->v_active,
		   src_w, dst_w, hshrk_mode);
	/*check legality, valid is 2,4,8,16,32,64*/
	switch (hshrk_mode) {
	case 2:
	case 4:
	case 8:
	case 16:
	case 32:
	case 64:
		break;

	default:
		pr_err("vdin.%d invalid h_shrink mode : %d\n", devp->index,
		       hshrk_mode);
		return;
	}
	//int hsk_mode = (int)(ceil((float)(vdin->vdin_dw_top.in_hsize)
	//(float)(vdin->vdin_dw_top.out_hsize)));
	coef = 64 / hshrk_mode;
	coef = (coef << 24) | (coef << 16) | (coef << 8) | coef;

	/*hshrk_mode: 2->1/2, 4->1/4... 64->1/64(max)*/
	wr_bits(offset, VDIN0_HSK_CTRL, hshrk_mode, DW_HSK_MODE_BIT, DW_HSK_MODE_WID);
	//wr_bits(offset, VDIN_DW_HSK_CTRL, src_w, HSK_HSIZE_IN_BIT,
	//	HSK_HSIZE_IN_WID);

	/*h shrink coef*/
	for (i = VDIN0_HSK_COEF_0; i <= VDIN0_HSK_COEF_15; i++)
		wr(offset, i, coef);

	/* reg_hsk_en */
	wr_bits(offset, VDIN0_DW_CTRL, 1, 2, 1);

	//devp->h_active = devp->h_shrink_out;
	pr_info("vdin.%d set_h_shrink done! h shrink mode = %d\n", devp->index,
		hshrk_mode);
}

/* new add v_shrink module
 * do vertical scale down,when scaling4h is smaller than half of v_active
 * which is closed by default
 * vshrk_mode:0-->1:2; 1-->1:4; 2-->1:8
 * chip <= TL1, only vdin1 has this module.
 * chip >= TM2 && != T5, both vdin0 and vdin1 are supported.
 * chip == T5, only vdin0 support
 */
static void vdin_set_v_shrink_t3x(struct vdin_dev_s *devp)
{
	unsigned int offset = devp->addr_offset;
	unsigned int src_w = devp->h_shrink_out;
	unsigned int src_h = devp->v_active;
	unsigned int vshrk_mode = 0;
	int lpf_mode = 1;

	if (src_w > VDIN_V_SHRINK_H_LIMIT) {
		pr_err("vdin.%d v_shrink: only support input width <= %d\n",
		       devp->index, VDIN_V_SHRINK_H_LIMIT);
		return;
	}

	if (devp->dv_hw5.hw5_ctl & BIT0) //if scaler in vdin_pp_top
		return;

	if (devp->v_shrink_out)
		vshrk_mode = src_h / devp->v_shrink_out;

	pr_err("%s vdin.%d h:%d,v:%d,src:%d %d;mode:%d\n", __func__,
		   devp->index, devp->h_active, devp->v_active,
		   src_h, devp->v_shrink_out, vshrk_mode);

	if (vshrk_mode < 2) {
		pr_info("vdin.%d v shrink: out > in/2,return\n", devp->index);
		return;
	}

	if (vshrk_mode >= 8)
		vshrk_mode = 2;
	else if (vshrk_mode >= 4)
		vshrk_mode = 1;
	else//if (vshrk_mode >= 2)
		vshrk_mode = 0;
	// mode: 0 for 2:1, 1 for 4:1, 2 for 8:1

	pr_info("vdin.%d vshrk mode = %d\n", devp->index, vshrk_mode);

	wr(offset, VDIN0_VSK_CTRL, (vshrk_mode << 2) | (lpf_mode << 1));
	wr(offset, VDIN0_VSK_PTN_DATA0, 0x8080);
	wr(offset, VDIN0_VSK_PTN_DATA0, 0x8080);
	wr(offset, VDIN0_VSK_PTN_DATA0, 0x8080);
	wr(offset, VDIN0_VSK_PTN_DATA0, 0x8080);
	/* reg_vsk_gclk_ctrl */
	wr_bits(offset, VDIN0_DW_GCLK_CTRL, 0, 2, 2);
	/* reg_vsk_en */
	wr_bits(offset, VDIN0_DW_CTRL, 1, 1, 1);
	//devp->v_active = devp->v_shrink_out;
	pr_info("vdin.%d set_v_shrink done!\n", devp->index);
}

/*function:set horizontal and vertical scale
 *vdin scale down path:
 *	vdin0:pre hsc-->horizontal scaling-->v scaling;
 *	vdin1:pre hsc-->horizontal scaling-->v_shrink-->v scaling
 *for tm2, scaling path, both vdin are same:
 *	prehsc-->h scaling-->v scaling-->optional h/v shrink;
 */
void vdin_set_hv_scale_t3x(struct vdin_dev_s *devp)
{
	/*backup current h v size*/
	devp->h_active_org = devp->h_active;
	devp->v_active_org = devp->v_active;
	devp->s5_data.h_scale_out = devp->h_active;
	devp->s5_data.v_scale_out = devp->v_active;

	if (K_FORCE_HV_SHRINK)
		goto set_hv_shrink;

	pr_info("vdin%d %s h_active:%u,v_active:%u.scaling:%dx%d\n", devp->index,
		__func__, devp->h_active, devp->v_active,
		devp->prop.scaling4w, devp->prop.scaling4h);

	if (devp->prop.scaling4w < devp->h_active &&
	    devp->prop.scaling4w > 0) {
		if (devp->pre_h_scale_en && (devp->prop.scaling4w <=
		    (devp->h_active >> 1)))
			vdin_set_pre_h_scale_t3x(devp);

		if (devp->prop.scaling4w < devp->h_active)
			vdin_set_hscale_t3x(devp, devp->prop.scaling4w);
	} else if (devp->h_active > VDIN_MAX_H_ACTIVE) {
		vdin_set_hscale_t3x(devp, VDIN_MAX_H_ACTIVE);
	}

	if (devp->prop.scaling4h < devp->v_active &&
	    devp->prop.scaling4h > 0) {
		if (devp->v_shrink_en && !cpu_after_eq(MESON_CPU_MAJOR_ID_TM2) &&
		    (devp->prop.scaling4h <= (devp->v_active >> 1))) {
			vdin_set_v_shrink_t3x(devp);
		}

		if (devp->prop.scaling4h < devp->v_active)
			vdin_set_vscale_t3x(devp);
	}

set_hv_shrink:

	if ((devp->double_wr || K_FORCE_HV_SHRINK) &&
	    devp->h_active > 1920 && devp->v_active > 1080) {
		devp->h_shrink_times = H_SHRINK_TIMES_4k;
		devp->v_shrink_times = V_SHRINK_TIMES_4k;
	} else if (devp->double_wr && devp->h_active > 1280 &&
		   devp->v_active > 720) {
		devp->h_shrink_times = H_SHRINK_TIMES_1080;
		devp->v_shrink_times = V_SHRINK_TIMES_1080;
	} else {
		devp->h_shrink_times = 1;
		devp->v_shrink_times = 1;
	}
	if (devp->debug.dbg_force_shrink_en) {
		devp->h_shrink_times = H_SHRINK_TIMES_1080;
		devp->v_shrink_times = V_SHRINK_TIMES_1080;
	}
	devp->h_shrink_out = devp->h_active / devp->h_shrink_times;
	devp->v_shrink_out = devp->v_active / devp->v_shrink_times;

	if (devp->dv_hw5.hw5_ctl & BIT0) {
		devp->h_shrink_out = devp->prop.scaling4w;
		devp->v_shrink_out = devp->prop.scaling4h;
	}

	if (devp->double_wr || K_FORCE_HV_SHRINK || devp->debug.dbg_force_shrink_en) {
		if (devp->h_shrink_out < devp->h_active)
			vdin_set_h_shrink_t3x(devp);

		if (devp->v_shrink_out < devp->v_active)
			vdin_set_v_shrink_t3x(devp);
	}

	if (vdin_dbg_en) {
		pr_info("[vdin.%d] %s h_active:%u,v_active:%u.\n", devp->index,
			__func__, devp->h_active, devp->v_active);
		pr_info("[vdin.%d] %s shrink out h:%d,v:%d\n", devp->index,
			__func__, devp->h_shrink_out, devp->v_shrink_out);
	}
}

/*set source_bitdepth
 *	base on color_depth_config:
 *		10, 8, 0, other
 */
void vdin_set_bitdepth_t3x(struct vdin_dev_s *devp)
{
	//unsigned int offset = devp->addr_offset;
	unsigned int set_width = 0;
	unsigned int port;
	enum vdin_color_deeps_e bit_dep;
	unsigned int convert_fmt, offset;

	offset = devp->addr_offset;
	/* yuv 422 full pack check */
	if (devp->color_depth_support &
	    VDIN_WR_COLOR_DEPTH_10BIT_FULL_PACK_MODE)
		devp->full_pack = VDIN_422_FULL_PK_EN;
	else
		devp->full_pack = VDIN_422_FULL_PK_DIS;

	/*hw verify:de-tunnel 444 to 422 12bit*/
	//if (devp->dtdata->ipt444_to_422_12bit && vdin_cfg_444_to_422_wmif_en)
	//	devp->full_pack = VDIN_422_FULL_PK_DIS;

	if (devp->output_color_depth &&
	    (devp->prop.fps == 50 || devp->prop.fps == 60) &&
	    (devp->parm.info.fmt == TVIN_SIG_FMT_HDMI_3840_2160_00HZ ||
	     devp->parm.info.fmt == TVIN_SIG_FMT_HDMI_4096_2160_00HZ) &&
	    devp->prop.colordepth == VDIN_COLOR_DEEPS_10BIT) {
		set_width = devp->output_color_depth;
		pr_info("set output color depth %d bit from dts\n", set_width);
	}

	switch (devp->color_depth_config & 0xff) {
	case COLOR_DEEPS_8BIT:
		bit_dep = VDIN_COLOR_DEEPS_8BIT;
		break;
	case COLOR_DEEPS_10BIT:
		bit_dep = VDIN_COLOR_DEEPS_10BIT;
		break;
	/*
	 * vdin not support 12bit now, when rx submit is 12bit,
	 * vdin config it as 10bit , 12 to 10
	 */
	case COLOR_DEEPS_12BIT:
		bit_dep = VDIN_COLOR_DEEPS_10BIT;
		break;
	case COLOR_DEEPS_AUTO:
		/* vdin_bitdepth is set to 0 by default, in this case,
		 * devp->source_bitdepth is controlled by colordepth
		 * change default to 10bit for 8in8out detail maybe lost
		 */
		if (vdin_is_convert_to_444(devp->format_convert) &&
		    vdin_is_4k(devp)) {
			bit_dep = VDIN_COLOR_DEEPS_8BIT;
		} else if (devp->prop.colordepth == VDIN_COLOR_DEEPS_8BIT) {
			/* hdmi YUV422, 8 or 10 bit valid is unknown*/
			/* so need vdin 10bit to frame buffer*/
			port = devp->parm.port;
			if (IS_HDMI_SRC(port)) {
				convert_fmt = devp->format_convert;
				if ((vdin_is_convert_to_422(convert_fmt) &&
				     (devp->color_depth_support &
				      VDIN_WR_COLOR_DEPTH_10BIT)) ||
				    devp->prop.vdin_hdr_flag)
					bit_dep = VDIN_COLOR_DEEPS_10BIT;
				else
					bit_dep = VDIN_COLOR_DEEPS_8BIT;

				if (vdin_is_dolby_tunnel_444_input(devp))
					bit_dep = VDIN_COLOR_DEEPS_8BIT;
			} else {
				bit_dep = VDIN_COLOR_DEEPS_8BIT;
			}
		} else if ((devp->color_depth_support & VDIN_WR_COLOR_DEPTH_10BIT) &&
			   ((devp->prop.colordepth == VDIN_COLOR_DEEPS_10BIT) ||
			   (devp->prop.colordepth == VDIN_COLOR_DEEPS_12BIT))) {
			if (set_width == VDIN_COLOR_DEEPS_8BIT)
				bit_dep = VDIN_COLOR_DEEPS_8BIT;
			else
				bit_dep = VDIN_COLOR_DEEPS_10BIT;
		} else {
			bit_dep = VDIN_COLOR_DEEPS_8BIT;
		}

		break;
	default:
		bit_dep = VDIN_COLOR_DEEPS_8BIT;
		break;
	}

	if (devp->work_mode == VDIN_WORK_MD_V4L)
		bit_dep = VDIN_COLOR_DEEPS_8BIT;

	devp->source_bitdepth = bit_dep;
	devp->source_bitdepth_dw = devp->source_bitdepth;

	/* only support 8bit mode at vpp side when double wr */
	if (!vdin_is_support_10bit_for_dw_t3x(devp))
		devp->source_bitdepth_dw = VDIN_COLOR_DEEPS_8BIT;

	if (devp->dv_hw5.hw5_ctl & BIT3) {
		pr_info("%s dw_wrmif_444_10bit\n", __func__);
		devp->source_bitdepth_dw = VDIN_COLOR_DEEPS_10BIT;
	}

	if (devp->source_bitdepth_dw == VDIN_COLOR_DEEPS_8BIT) {
		wr_bits(offset, VDIN0_WRMIF_CTRL2, MIF_8BIT,
			VDIN_WR_10BIT_MODE_BIT, VDIN_WR_10BIT_MODE_WID);
	} else {
		wr_bits(offset, VDIN0_WRMIF_CTRL2, MIF_10BIT,
			VDIN_WR_10BIT_MODE_BIT, VDIN_WR_10BIT_MODE_WID);
	}
	if (vdin_dbg_en)
		pr_info("%s,bitdepth:%d %d, cfg:0x%x prop.dep:%x\n", __func__,
			devp->source_bitdepth, devp->source_bitdepth_dw,
			devp->color_depth_config, devp->prop.colordepth);
}

/* do horizontal reverse and vertical reverse
 * h reverse:
 * VDIN_WR_H_START_END
 * Bit29:	1.reverse	0.do not reverse
 *
 * v reverse:
 * VDIN_WR_V_START_END
 * Bit29:	1.reverse	0.do not reverse
 */
void vdin_wr_reverse_t3x(unsigned int offset, bool h_reverse, bool v_reverse)
{
	if (h_reverse)
		wr_bits(offset, VDIN0_WRMIF_CTRL2, 1, 21, 1);
	else
		wr_bits(offset, VDIN0_WRMIF_CTRL2, 0, 21, 1);
	if (v_reverse)
		wr_bits(offset, VDIN0_WRMIF_CTRL2, 1, 20, 1);
	else
		wr_bits(offset, VDIN0_WRMIF_CTRL2, 0, 20, 1);
}

void vdin_bypass_isp_t3x(unsigned int offset)
{
//	wr_bits(offset, VDIN_CM_BRI_CON_CTRL, 0, CM_TOP_EN_BIT, CM_TOP_EN_WID);
//	wr_bits(offset, VDIN_MATRIX_CTRL, 0,
//		VDIN_MATRIX_EN_BIT, VDIN_MATRIX_EN_WID);
//	wr_bits(offset, VDIN_MATRIX_CTRL, 0,
//		VDIN_MATRIX1_EN_BIT, VDIN_MATRIX1_EN_WID);
}

void vdin_set_cm2_t3x(unsigned int offset, unsigned int w,
		  unsigned int h, unsigned int *cm2)
{
	unsigned int i = 0, j = 0, start_addr = 0x100;

	if (!cm_enable)
		return;
	pr_info("%s %d,w=%d,h=%d\n", __func__, __LINE__, w, h);

	/* reg_cm_en */
	wr_bits(offset, VDIN0_PP_CTRL, 0, PP_CM_EN_BIT, PP_CM_EN_WID);
	for (i = 0; i < 160; i++) {
		j = i / 5;
		wr(offset, VDIN0_CM_ADDR_PORT, start_addr + (j << 3) + (i % 5));
		wr(offset, VDIN0_CM_DATA_PORT, cm2[i]);
	}
	for (i = 0; i < 28; i++) {
		wr(offset, VDIN0_CM_ADDR_PORT, 0x200 + i);
		wr(offset, VDIN0_CM_DATA_PORT, cm2[160 + i]);
	}
	/*config cm2 frame size*/
	wr(offset, VDIN0_CM_ADDR_PORT, 0x205);
	wr(offset, VDIN0_CM_DATA_PORT, h << 16 | w);
	wr_bits(offset, VDIN0_PP_CTRL, 1, PP_CM_EN_BIT, PP_CM_EN_WID);
}

/*vdin0 output ctrl
 * Bit 26 vcp_nr_en. Only used in VDIN0. NOT used in VDIN1.
 * Bit 25 vcp_wr_en. Only used in VDIN0. NOT used in VDIN1.
 * Bit 24 vcp_in_en. Only used in VDIN0. NOT used in VDIN1.
 */
static void vdin_output_ctl_t3x(unsigned int offset, unsigned int output_nr_flag)
{
//	wr_bits(offset, VDIN_WR_CTRL, 1, VCP_IN_EN_BIT, VCP_IN_EN_WID);
//	if (output_nr_flag) {
//		wr_bits(offset, VDIN_WR_CTRL, 0, VCP_WR_EN_BIT, VCP_WR_EN_WID);
//		wr_bits(offset, VDIN_WR_CTRL, 1, VCP_NR_EN_BIT, VCP_NR_EN_WID);
//	} else {
//		wr_bits(offset, VDIN_WR_CTRL, 1, VCP_WR_EN_BIT, VCP_WR_EN_WID);
//		wr_bits(offset, VDIN_WR_CTRL, 0, VCP_NR_EN_BIT, VCP_NR_EN_WID);
//	}
}

/* set mpegin
 * VDIN_COM_CTRL0
 * Bit 31,   mpeg_to_vdin_sel,
 * 0: mpeg source to NR directly,
 * 1: mpeg source pass through here
 */
void vdin_set_mpegin_t3x(struct vdin_dev_s *devp)
{
	unsigned int offset = devp->addr_offset;

	/* set VDIN_MEAS_CLK_CNTL, select XTAL clock */
	/* if (is_meson_gxbb_cpu()) */
	/* ; */
	/* else */
	/* aml_write_cbus(HHI_VDIN_MEAS_CLK_CNTL, 0x00000100); */

//	wr(offset, VDIN_COM_CTRL0, 0x80000911);
//	vdin_delay_line_t3x(devp, delay_line_num);
//	wr(offset, VDIN_COM_GCLK_CTRL, 0x0);
//
//	wr(offset, VDIN_INTF_WIDTHM1, devp->h_active - 1);
//	wr(offset, VDIN_WR_CTRL2, 0x0);
//
//	wr(offset, VDIN_HIST_CTRL, 0x3);
//	wr(offset, VDIN_HIST_H_START_END, devp->h_active - 1);
//	wr(offset, VDIN_HIST_V_START_END, devp->v_active - 1);
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
	vdin_output_ctl_t3x(offset, 1);
#endif
}

void vdin_force_go_filed_t3x(struct vdin_dev_s *devp)
{
//	unsigned int offset = devp->addr_offset;

	wr_bits(devp->index * VDIN_TOP_OFFSET, VDIN0_SYNC_CONVERT_SYNC_CTRL0, 1, 31, 1);
	wr_bits(devp->index * VDIN_TOP_OFFSET, VDIN0_SYNC_CONVERT_SYNC_CTRL0, 0, 31, 1);
}

void vdin_dolby_addr_update_t3x(struct vdin_dev_s *devp, unsigned int index)
{
	unsigned int offset = devp->addr_offset;
	u32 *p;
	unsigned int value = 0;

	if (index >= devp->canvas_max_num)
		return;
	devp->vfp->dv_buf[index] = NULL;
	devp->vfp->dv_buf_size[index] = 0;
	p = (u32 *)devp->vfp->dv_buf_vmem[index];
	p[0] = 0;
	p[1] = 0;
	if (dv_dbg_mask & DV_READ_MODE_AXI) {
		wr(offset, VDIN0_META_AXI_CTRL1,
		   devp->vfp->dv_buf_mem[index]);
		wr_bits(offset, VDIN0_META_AXI_CTRL0, 1, 4, 1);
		wr_bits(offset, VDIN0_META_AXI_CTRL0, 0, 4, 1);
		if (dv_dbg_log & DV_DEBUG_NORMAL)
			pr_info("%s:index:%d dma:%x\n", __func__,
				index, devp->vfp->dv_buf_mem[index]);
	} else {
		value  = 0;
		value |= (0 << 31);/* reg_meta_rd_en */
		value |= (1 << 30);/* reg_meta_rd_mode */
		value |= (0x0c0d5 << 0); /* reg_meta_tunnel_sel */
		value |= (0x280 << 20);/* reg_meta_wr_sum */
//		if (devp->h_active_org >= 3840)
//			value |= (280 << 20);/* reg_meta_wr_sum */
//		else
//			value |= ((devp->h_active_org * 3 / 40) << 20);
		wr(offset, VDIN0_META_DSC_CTRL2, value);
		wr(offset, VDIN0_META_DSC_CTRL3, 0x0);
	}
}

void vdin_dolby_config_t3x(struct vdin_dev_s *devp)
{
	unsigned int offset = devp->addr_offset;
	unsigned int value = 0;

	devp->dv.dv_config = 1;
	devp->dv.dv_path_idx = devp->index;
	devp->vfp->low_latency = devp->dv.low_latency;
	memcpy(&devp->vfp->dv_vsif,
	       &devp->dv.dv_vsif, sizeof(struct tvin_dv_vsif_s));
	value  = 0;
	value |= (1 << 31);/* reg_meta_dolby_check_en */
	value |= (1 << 30);/* reg_meta_tunnel_swap_en */
	value |= (1 << 17);/* reg_meta_frame_rst */
	value |= (1 << 16);/* reg_meta_lsb */
	value |= (128 << 0);/* reg_meta_lsb */
	wr(offset, VDIN0_META_DSC_CTRL0, value);
	wr_bits(offset, VDIN0_META_DSC_CTRL0, 0x4, 24, 6);
	wr_bits(offset, VDIN0_META_DSC_CTRL0, 0x0, 24, 6);

	wr(offset, VDIN0_META_DSC_CTRL1, 0x3);/* reg_meta_crc_ctrl */

	if (dv_dbg_mask & DV_READ_MODE_AXI) {
		wr(offset, VDIN0_META_AXI_CTRL1, devp->vfp->dv_buf_mem[0]);
		/* hold line = 0 */
		wr_bits(offset, VDIN0_META_AXI_CTRL0, 0, 8, 8);//ucode:0x10
		wr_bits(offset, VDIN0_META_AXI_CTRL0, 1, 4, 1);
		wr_bits(offset, VDIN0_META_AXI_CTRL0, 1, 5, 1);
		wr_bits(offset, VDIN0_META_AXI_CTRL0, 0, 5, 1);
		wr_bits(offset, VDIN0_META_AXI_CTRL0, 0, 4, 1);
		/*enable wr memory*/
		vdin_dolby_mdata_write_en_t3x(offset, 1);
	} else {
		/*disable wr memory*/
		vdin_dolby_mdata_write_en_t3x(offset, 0);
		value  = 0;
		value |= (0 << 31);/* reg_meta_rd_en */
		value |= (1 << 30);/* reg_meta_rd_mode */
		value |= (0x0c0d5 << 0); /* reg_meta_tunnel_sel */
		value |= (0x280 << 20);/* reg_meta_wr_sum */
//		if (devp->h_active_org >= 3840)
//			value |= (0x280 << 20);/* reg_meta_wr_sum */
//		else
//			value |= ((devp->h_active_org * 3 / 40) << 20);
		wr(offset, VDIN0_META_DSC_CTRL2, value);
		wr(offset, VDIN0_META_DSC_CTRL3, 0x0);
	}

	if (vdin_dbg_en)
		pr_info("%s %d\n", __func__, __LINE__);
}

void vdin_dolby_mdata_write_en_t3x(unsigned int offset, unsigned int en)
{
	/*printk("=========>> wr memory %d\n", en);*/
	if (en) {
		/* reg_dolby_en */
		//wr_bits(offset, VDIN0_CORE_CTRL, 1, 0, 1);
		/* reg_meta_en */
		wr_bits(offset, VDIN0_CORE_CTRL, 1, 3, 1);
		/* reg_metapath_en */
		wr_bits(offset, VDIN0_CORE_CTRL, 1, 4, 1);
		wr_bits(offset, VDIN0_META_AXI_CTRL0, 1, 30, 1);
	} else {
		/* reg_dolby_en */
		wr_bits(offset, VDIN0_CORE_CTRL, 0, 0, 1);
		/* reg_meta_en */
		wr_bits(offset, VDIN0_CORE_CTRL, 1, 3, 1);
		/* reg_metapath_en */
		wr_bits(offset, VDIN0_CORE_CTRL, 1, 4, 1);
		wr_bits(offset, VDIN0_META_AXI_CTRL0, 0, 30, 1);
	}
}

/*
 * For shrink module working in amdolby mode,dw path
 * detunnel
 * 12bit_422--->12bit_444
 * 12bit_444--->10bit_444
 * on:1 off:0
 */
void vdin_descramble_setting_t3x(struct vdin_dev_s *devp,
			       unsigned int on_off)
{
	unsigned int offset = devp->addr_offset;

	if (devp->index >= 2) /* vdin2 */
		return;
	if (devp->dv_hw5.hw5_ctl & BIT1)
		return;

	if (vdin_dbg_en)
		pr_info("vdin DSC in:(%d, %d)(%d, %d)(%d, %d)\n",
			devp->h_active_org, devp->v_active_org,
			devp->h_active,     devp->v_active,
			devp->h_shrink_out, devp->v_shrink_out);

	/* reg_dsc_cvfmt_w */
	wr_bits(offset, VDIN0_DSC_CFMT_W, devp->h_active_org / 2, 0, 13);
	/* reg_dsc_chfmt_w */
	wr_bits(offset, VDIN0_DSC_CFMT_W, devp->h_active_org, 16, 13);
	/* reg_dsc_detunnel_hsize */
	wr_bits(offset, VDIN0_DSC_HSIZE, devp->h_active_org, 16, 13);
	wr(offset, VDIN0_DSC_DETUNNEL_SEL, 0x2c2d0);
	wr(offset, VDIN0_DSC_CFMT_CTRL, 0x3);
	wr(offset, VDIN0_DSC_CTRL, 0x125);
	/* TODO:If reg_dolby_en = 1,we should set detunnel before dolby in vdin core */

	if (on_off) {
		/* reg_dsc_en */
		wr_bits(offset, VDIN0_DW_CTRL, 1, 4, 1);
		/* reg_dsc_detunnel_en */
		wr_bits(offset, VDIN0_DSC_CTRL, 0x1, 2, 1);
	} else {
		/* reg_dsc_en */
		wr_bits(offset, VDIN0_DW_CTRL, 0, 4, 1);
		/* reg_dsc_detunnel_en */
		wr_bits(offset, VDIN0_DSC_CTRL, 0x0, 2, 1);
	}
}

/*
 * yuv format, DV scramble setting for dw path
 */
void vdin_scramble_setting_t3x(struct vdin_dev_s *devp,
				   unsigned int on_off)
{
	unsigned int offset = devp->addr_offset;

	if (devp->dv_hw5.hw5_ctl & BIT2)
		return;

	wr(offset, VDIN0_SCB_CTRL, 0x10);
	wr(offset, VDIN0_SCB_TUNNEL, 0xc07622);
	wr_bits(offset, VDIN0_SCB_SIZE, devp->v_active, 0, 13);
	wr_bits(offset, VDIN0_SCB_SIZE, devp->h_active, 16, 13);
	if (on_off) {
		/* reg_tunnel_en */
		wr_bits(offset, VDIN0_SCB_TUNNEL, 0x1, 0, 1);
		/* reg_scb_en */
		wr_bits(offset, VDIN0_DW_CTRL, 0x1, 3, 1);
	} else {
		/* reg_tunnel_en */
		wr_bits(offset, VDIN0_SCB_TUNNEL, 0x0, 0, 1);
		/* reg_scb_en */
		wr_bits(offset, VDIN0_DW_CTRL, 0x0, 3, 1);
	}
}

void vdin_dv_tunnel_set_t3x(struct vdin_dev_s *devp)
{
	if (!vdin_is_dolby_signal_in(devp))
		return;

	if (!devp->dtdata->de_tunnel_tunnel && !devp->dv_hw5.hw5_ctl)
		return;

	if (vdin_dbg_en) {
		pr_info("vdin dv tunel_set shrink:(%d, %d)\n",
			devp->h_active, devp->h_shrink_out);
	}

	/* h shrink on*/
	if (devp->h_shrink_out < devp->h_active) {
		/*hw verify:de-tunnel 444 to 422 12bit*/
		vdin_descramble_setting_t3x(devp, true);
		/*vdin de tunnel and tunnel for vdin scaling*/
		vdin_scramble_setting_t3x(devp, true);
	} else {
		vdin_descramble_setting_t3x(devp, false);
		vdin_scramble_setting_t3x(devp, false);
	}
}

/*@20170905new add for support dynamic adj dest_format yuv422/yuv444,
 *not support nv21 dynamic adj!!!
 */
void vdin_source_bitdepth_reinit_t3x(struct vdin_dev_s *devp)
{
	int i = 0;
	struct vf_entry *master;
	struct vframe_s *vf;
	struct vf_pool *p = devp->vfp;

	for (i = 0; i < p->size; ++i) {
		master = vf_get_master(p, i);
		vf = &master->vf;
		vdin_set_source_bitdepth(devp, vf);
	}
}

void vdin_clk_on_off_t3x(struct vdin_dev_s *devp, bool on_off)
{
	/* gclk_ctrl,00: auto, 01: off, 1x: on */
	if (on_off) { /* on */
		/* reg_top_gclk_ctrl */
		wr_bits(0, VDIN_TOP_GCLK_CTRL, 0, 28, 4);
		/* reg_intf_gclk_ctrl */
		wr_bits(0, VDIN_TOP_GCLK_CTRL, 0, 8, 2);
		/* reg_vdinx_gclk_ctrl */
		wr_bits(0, VDIN_TOP_GCLK_CTRL, 0, 6 - devp->index * 2, 2);
		/* reg_local_arb_gclk_ctrl */
		wr_bits(0, VDIN_TOP_GCLK_CTRL, 0, 0, 2);
	} else { /* off */
		/* reg_vdinx_gclk_ctrl */
		wr_bits(0, VDIN_TOP_GCLK_CTRL, 0x1, 6 - devp->index * 2, 2);
	}
}

void vdin_set_matrix_color_t3x(struct vdin_dev_s *devp)
{
	unsigned int offset = devp->addr_offset;
	unsigned int mode = devp->matrix_pattern_mode;

	/*vdin bist mode RGB:black*/
	wr(offset, VDIN0_MAT_COEF00_01, 0x0);
	wr(offset, VDIN0_MAT_COEF02_10, 0x0);
	wr(offset, VDIN0_MAT_COEF11_12, 0x0);
	wr(offset, VDIN0_MAT_COEF20_21, 0x0);
	wr(offset, VDIN0_MAT_COEF22, 0x0);
	wr(offset, VDIN0_MAT_PRE_OFFSET0_1, 0x0);
	wr(offset, VDIN0_MAT_PRE_OFFSET2, 0x0);
	if (mode == 1) {
		wr(offset, VDIN0_MAT_OFFSET0_1, 0x10f010f);
		wr(offset, VDIN0_MAT_OFFSET2, 0x2ff);
	} else if (mode == 2) {
		wr(offset, VDIN0_MAT_OFFSET0_1, 0x10f010f);
		wr(offset, VDIN0_MAT_OFFSET2, 0x1ff);
	} else if (mode == 3) {
		wr(offset, VDIN0_MAT_OFFSET0_1, 0x00003ff);
		wr(offset, VDIN0_MAT_OFFSET2, 0x0);
	} else if (mode == 4) {
		wr(offset, VDIN0_MAT_OFFSET0_1, 0x1ff01ff);
		wr(offset, VDIN0_MAT_OFFSET2, 0x1ff);
	} else {
		wr(offset, VDIN0_MAT_OFFSET0_1, 0x1ff010f);
		wr(offset, VDIN0_MAT_OFFSET2, 0x2ff);
	}

	if (mode)
		wr_bits(offset, VDIN0_PP_CTRL, 1, PP_MAT0_EN_BIT, PP_MAT0_EN_WID);
	else
		wr_bits(offset, VDIN0_PP_CTRL, 0, PP_MAT0_EN_BIT, PP_MAT0_EN_WID);
	if (vdin_dbg_en)
		pr_info("%s offset:%d, md:%d\n", __func__, offset, mode);
}

/* only active under vdi6 loopback case */
void vdin_set_bist_pattern_t3x(struct vdin_dev_s *devp, unsigned int on_off, unsigned int pat)
{
//	unsigned int offset = devp->addr_offset;
//	unsigned int de_start = 0x7;
//	unsigned int v_blank = 0x3f;
//	unsigned int h_blank = 0xff;

//	if (on_off) {
//		wr_bits(offset, VDIN_ASFIFO_CTRL0, de_start,
//			VDI6_BIST_DE_START_BIT, VDI6_BIST_DE_START_WID);
//		wr_bits(offset, VDIN_ASFIFO_CTRL0, v_blank,
//			VDI6_BIST_VBLANK_BIT, VDI6_BIST_VBLANK_WID);
//		wr_bits(offset, VDIN_ASFIFO_CTRL0, h_blank,
//			VDI6_BIST_HBLANK_BIT, VDI6_BIST_HBLANK_WID);
//
//		/* 0:horizontal gray scale, 1:vertical gray scale, 2,3:random data */
//		wr_bits(offset, VDIN_ASFIFO_CTRL0, pat,
//			VDI6_BIST_SEL_BIT, VDI6_BIST_SEL_WID);
//		wr_bits(offset, VDIN_ASFIFO_CTRL0, 1,
//			VDI6_BIST_EN_BIT, VDI6_BIST_EN_WID);
//	} else {
//		wr_bits(offset, VDIN_ASFIFO_CTRL0, 0,
//			VDI6_BIST_EN_BIT, VDI6_BIST_EN_WID);
//	}
}

void vdin_dmc_ctrl_t3x(struct vdin_dev_s *devp, bool on_off)
{
	unsigned int reg;

	if (on_off) {
		/* for 4k nr, dmc vdin write band width not good
		 * dmc write 1 set to supper urgent
		 */
		if (devp->dtdata->hw_ver == VDIN_HW_T5 ||
		    devp->dtdata->hw_ver == VDIN_HW_T5D ||
		    devp->dtdata->hw_ver == VDIN_HW_T5W) {
			reg = READ_DMCREG(0x6c) & (~(1 << 17));
			if (!(reg & (1 << 18)))
				WRITE_DMCREG(0x6c, reg | (1 << 18));
		}
	} else {
		/* for 4k nr, dmc vdin write band width not good
		 * dmc write 1 set to urgent
		 */
		if (devp->dtdata->hw_ver == VDIN_HW_T5 ||
		    devp->dtdata->hw_ver == VDIN_HW_T5D ||
		    devp->dtdata->hw_ver == VDIN_HW_T5W) {
			reg = READ_DMCREG(0x6c) & (~(1 << 18));
			WRITE_DMCREG(0x6c, reg | (1 << 17));
		}
	}
}

void vdin_sw_reset_t3x(struct vdin_dev_s *devp)
{
//	unsigned int offset = 0;

	//offset = devp->addr_offset;
	//reg_reset[6:2],hardware reset control
	//bit2:reset vdin_intf;bit3:reset vdin_core0;bit4:reset vdin_core1;
	//bit5:reset vdin_core2;bit6:reset vdin_local_arb;
	wr_bits(0, T3X_VDIN_TOP_CTRL, 0x1, 3 + devp->index, 1);
	wr_bits(0, T3X_VDIN_TOP_CTRL, 0x0, 3 + devp->index, 1);
	//wr_bits(offset, T3X_VDIN_TOP_CTRL, 0x0, 2, 5);
}

/*
 *[0]:enable/disable;[1~31]:pattern mode 0/1/2/...
 */
void vdin_bist_t3x(struct vdin_dev_s *devp, unsigned int mode)
{
	unsigned int bist_en = 0, test_pat = 0;
	unsigned int hblank = 0, vblank = 0, vde_start = 0;

	if (!(mode & 0x1)) {
		bist_en = 0;
	} else {
		bist_en = 1;
		test_pat = (mode >> 1);
		if (devp->h_active == 320) {
			//320x240
			hblank = 11;
			vblank = 8;
			vde_start = 2;
		} else {
			//1920x1080
			hblank = 128;//280;
			vblank = 128;//45;
			vde_start = 2;
		}
	}
	wr(0, VDIN_INTF_PTGEN_CTRL,
		(bist_en << 0) | (test_pat << 1) | (hblank << 4) |
		(vblank << 12) | (vde_start << 20));
}

bool vdin_is_wrmif_done_t3x(struct vdin_dev_s *devp)
{
	if (rd_bits(devp->addr_offset, VDIN0_CORE_FRM_END, 3, 1))
	//if (rd_bits(devp->addr_offset, VDIN0_WRMIF_RO_STATUS, 0, 1))
		return true;
	return false;
}

void vdin_clr_write_done_t3x(struct vdin_dev_s *devp)
{
	wr_bits(devp->addr_offset, VDIN0_CORE_CTRL, 0xf, 15, 4);
	wr_bits(devp->addr_offset, VDIN0_CORE_CTRL, 0x0, 15, 4);
}

/* In 2/4 ppc mode,vdin report value will be half/quarter */
unsigned int vdin_get_div_t3x(struct vdin_dev_s *devp)
{
	unsigned int div_val = 1;
	static const struct vinfo_s *vinfo;

	if (devp->parm.port >= TVIN_PORT_VIU1 &&
		devp->parm.port <= TVIN_PORT_VIU1_MAX)
		vinfo = get_current_vinfo();
	else if (devp->parm.port >= TVIN_PORT_VIU2 &&
		devp->parm.port <= TVIN_PORT_VIU2_MAX)
	#ifdef CONFIG_AMLOGIC_VOUT2_SERVE
		vinfo = get_current_vinfo2();
	#else
		;
	#endif
	else
		return 1;

	if (vinfo && (vinfo->cur_enc_ppc == 2 || vinfo->cur_enc_ppc == 1))
		div_val = 2;
	else if (vinfo && vinfo->cur_enc_ppc == 4)
		div_val = 4;
	else
		div_val = 1;

	return div_val;
}

void vdin_set_scl_mode_t3x(struct vdin_dev_s *devp, bool on_off)
{
	if (on_off)
		wr_bits(devp->addr_offset, VDIN0_PP_CTRL, 0x1, 9, 1);
	else
		wr_bits(devp->addr_offset, VDIN0_PP_CTRL, 0x0, 9, 1);
}

void vdin_set_dsc_config_t3x(struct vdin_dev_s *devp, bool on_off)
{
	struct aml_dsc_dec_drv_s dsc_dec_drv;

	if (!is_meson_t3x_cpu())
		return;
	dsc_dec_drv.pps_data = devp->prop.pps_data;
	if (devp->prop.dsc_flag && on_off) {
		pr_info("dsc_en\n");
		dsc_dec_en(true, &dsc_dec_drv.pps_data);
	} else {
		pr_info("dsc_off\n");
		dsc_dec_en(false, &dsc_dec_drv.pps_data);
	}
}
