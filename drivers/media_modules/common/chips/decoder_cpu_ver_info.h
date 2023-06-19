/*
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Description:
 */
#ifndef DECODER_CPU_VER_INFO_H
#define DECODER_CPU_VER_INFO_H
#include <linux/platform_device.h>
#include <linux/amlogic/media/registers/cpu_version.h>
#include "../register/register.h"
#include <linux/amlogic/media/utils/vformat.h>

/* majoy chip id define */
#define MAJOY_ID_MASK (0x000000ff)

#define DECODE_CPU_VER_ID_NODE_NAME "cpu_ver_name"
#define CODEC_DOS_DEV_ID_NODE_NAME  "vcodec_dos_dev"

enum AM_MESON_CPU_MAJOR_ID {
	AM_MESON_CPU_MAJOR_ID_M6	= 0x16,
	AM_MESON_CPU_MAJOR_ID_M6TV	= 0x17,
	AM_MESON_CPU_MAJOR_ID_M6TVL	= 0x18,
	AM_MESON_CPU_MAJOR_ID_M8	= 0x19,
	AM_MESON_CPU_MAJOR_ID_MTVD	= 0x1A,
	AM_MESON_CPU_MAJOR_ID_M8B	= 0x1B,
	AM_MESON_CPU_MAJOR_ID_MG9TV	= 0x1C,
	AM_MESON_CPU_MAJOR_ID_M8M2	= 0x1D,
	AM_MESON_CPU_MAJOR_ID_UNUSE	= 0x1E,
	AM_MESON_CPU_MAJOR_ID_GXBB	= 0x1F,
	AM_MESON_CPU_MAJOR_ID_GXTVBB	= 0x20,
	AM_MESON_CPU_MAJOR_ID_GXL	= 0x21,
	AM_MESON_CPU_MAJOR_ID_GXM	= 0x22,
	AM_MESON_CPU_MAJOR_ID_TXL	= 0x23,
	AM_MESON_CPU_MAJOR_ID_TXLX	= 0x24,
	AM_MESON_CPU_MAJOR_ID_AXG	= 0x25,
	AM_MESON_CPU_MAJOR_ID_GXLX	= 0x26,
	AM_MESON_CPU_MAJOR_ID_TXHD	= 0x27,
	AM_MESON_CPU_MAJOR_ID_G12A	= 0x28,
	AM_MESON_CPU_MAJOR_ID_G12B	= 0x29,
	AM_MESON_CPU_MAJOR_ID_GXLX2	= 0x2a,
	AM_MESON_CPU_MAJOR_ID_SM1	= 0x2b,
	AM_MESON_CPU_MAJOR_ID_A1	= 0x2c,
	AM_MESON_CPU_MAJOR_ID_RES_0x2d,
	AM_MESON_CPU_MAJOR_ID_TL1	= 0x2e,
	AM_MESON_CPU_MAJOR_ID_TM2	= 0x2f,
	AM_MESON_CPU_MAJOR_ID_C1	= 0x30,
	AM_MESON_CPU_MAJOR_ID_RES_0x31,
	AM_MESON_CPU_MAJOR_ID_SC2	= 0x32,
	AM_MESON_CPU_MAJOR_ID_C2	= 0x33,
	AM_MESON_CPU_MAJOR_ID_T5	= 0x34,
	AM_MESON_CPU_MAJOR_ID_T5D	= 0x35,
	AM_MESON_CPU_MAJOR_ID_T7	= 0x36,
	AM_MESON_CPU_MAJOR_ID_S4	= 0x37,
	AM_MESON_CPU_MAJOR_ID_T3	= 0x38,
	AM_MESON_CPU_MAJOR_ID_P1	= 0x39,
	AM_MESON_CPU_MAJOR_ID_S4D	= 0x3a,
	AM_MESON_CPU_MAJOR_ID_T5W	= 0x3b,
	AM_MESON_CPU_MAJOR_ID_C3	= 0x3c,
	AM_MESON_CPU_MAJOR_ID_S5	= 0x3e,
	AM_MESON_CPU_MAJOR_ID_GXLX3	= 0x3f,
	AM_MESON_CPU_MAJOR_ID_T5M	= 0x41,
	AM_MESON_CPU_MAJOR_ID_T3X	= 0x42,
	AM_MESON_CPU_MAJOR_ID_RES_0x43,
	AM_MESON_CPU_MAJOR_ID_TXHD2	= 0x44,
	AM_MESON_CPU_MAJOR_ID_S1A	= 0x45,
	AM_MESON_CPU_MAJOR_ID_MAX,
};

/* chips sub id define */
#define CHIP_REVA 0x0
#define CHIP_REVB 0x1
#define CHIP_REVC 0x2
#define CHIP_REVX 0x10

#define REVB_MASK (CHIP_REVB << 8)
#define REVC_MASK (CHIP_REVC << 8)
#define REVX_MASK (CHIP_REVX << 8)

#define MAJOR_ID_MASK 0xFF
#define SUB_ID_MASK (REVB_MASK | REVC_MASK | REVX_MASK)

#define AM_MESON_CPU_MINOR_ID_REVB_G12B  (REVB_MASK | AM_MESON_CPU_MAJOR_ID_G12B)
#define AM_MESON_CPU_MINOR_ID_REVB_TM2   (REVB_MASK | AM_MESON_CPU_MAJOR_ID_TM2)
#define AM_MESON_CPU_MINOR_ID_S4_S805X2  (REVX_MASK | AM_MESON_CPU_MAJOR_ID_S4)
#define AM_MESON_CPU_MINOR_ID_T7C        (REVC_MASK | AM_MESON_CPU_MAJOR_ID_T7)

/* for dos_of_dev_s max resolution define */
#define RESOLUTION_1080P  (1920 * 1088)
#define RESOLUTION_4K     (4302 * 2176)  //4k
#define RESOLUTION_8K     (8192 * 4352)  //8k

/* fmt_support */
//vdec
#define FMT_MPEG2    BIT(VFORMAT_MPEG12)
#define FMT_MPEG4    BIT(VFORMAT_MPEG4)
#define FMT_H264     BIT(VFORMAT_H264)
#define FMT_MJPEG    BIT(VFORMAT_MJPEG)
#define FMT_VC1      BIT(VFORMAT_VC1)
#define FMT_AVS      BIT(VFORMAT_AVS)
#define FMT_MVC      BIT(VFORMAT_H264MVC)
//hevc
#define FMT_HEVC     BIT(VFORMAT_HEVC)
#define FMT_VP9      BIT(VFORMAT_VP9)
#define FMT_AVS2     BIT(VFORMAT_AVS2)
#define FMT_AV1      BIT(VFORMAT_AV1)
#define FMT_AVS3     BIT(VFORMAT_AVS3)
//hcodec
#define FMT_H264_ENC BIT(VFORMAT_H264_ENC)
#define FMT_JPEG_ENC BIT(VFORMAT_JPEG_ENC)

//frequently-used combination
#define FMT_VDEC_ALL               (FMT_MPEG2 | FMT_MPEG4 | FMT_H264 | FMT_MJPEG | FMT_VC1 | FMT_MVC | FMT_AVS)
#define FMT_VDEC_NO_AVS            (FMT_MPEG2 | FMT_MPEG4 | FMT_H264 | FMT_MJPEG | FMT_VC1 | FMT_MVC)

#define FMT_HEVC_VP9_AV1           (FMT_HEVC | FMT_VP9 | FMT_AV1)
#define FMT_HEVC_VP9_AVS2          (FMT_HEVC | FMT_VP9 | FMT_AVS2)
#define FMT_HEVC_VP9_AVS2_AV1      (FMT_AV1  | FMT_HEVC_VP9_AVS2)
#define FMT_HEVC_VP9_AVS2_AV1_AVS3 (FMT_AVS3 | FMT_HEVC_VP9_AVS2_AV1)

/* dos hardware feature define. */
struct dos_of_dev_s {
	enum AM_MESON_CPU_MAJOR_ID chip_id;

	/* register*/
	reg_compat_func reg_compat;

	/* clock, Mhz. necessary!! */
	u32 max_vdec_clock;
	u32 max_hevcf_clock;
	u32 max_hevcb_clock;
	bool hevc_clk_combine_flag;

	/* resolution. necessary!! */
	u32 vdec_max_resolution;	//just for h264
	u32 hevc_max_resolution;

	/* esparser */
	bool is_hw_parser_support;

	/* vdec */
	bool is_vdec_canvas_support;
	bool is_support_h264_mmu;

	/* hevc */
	bool is_support_dual_core;
	bool is_support_p010;
	bool is_support_triple_write;
	bool is_support_rdma;
	bool is_support_mmu_copy;
	int hevc_stream_extra_shift;

	bool is_support_axi_ctrl;  /*dos pipeline ctrl by dos or dmc */

	u32 fmt_support_flags;
};


/* export functions */
struct platform_device *initial_dos_device(void);

enum AM_MESON_CPU_MAJOR_ID get_cpu_major_id(void);

bool is_cpu_meson_revb(void);

bool is_cpu_tm2_revb(void);

int get_cpu_sub_id(void);

bool is_cpu_s4_s805x2(void);

bool is_cpu_t7(void);
bool is_cpu_t7c(void);

inline bool is_support_new_dos_dev(void);

struct dos_of_dev_s *dos_dev_get(void);

inline bool is_hevc_align32(int blkmod);

/* clk get */
inline u32 vdec_max_clk_get(void);

inline u32 hevcf_max_clk_get(void);

inline u32 hevcb_max_clk_get(void);

inline bool is_hevc_clk_combined(void);

/* resolution check */
inline int vdec_is_support_4k(void);

inline int hevc_is_support_4k(void);

inline int hevc_is_support_8k(void);

inline bool is_oversize_vdec(int w, int h);

inline bool is_oversize_hevc(int w, int h);

/* hardware features */
inline bool is_support_no_parser(void);

inline bool is_support_vdec_canvas(void);

inline bool is_support_dual_core(void);

inline bool is_support_p010_mode(void);

inline bool is_support_triple_write(void);

inline bool is_support_rdma(void);

inline bool is_support_mmu_copy(void);

inline bool is_support_axi_ctrl(void);

inline bool is_support_format(int format);

inline int get_hevc_stream_extra_shift_bytes(void);

void pr_dos_infos(void);

void dos_info_debug(void);

#endif
