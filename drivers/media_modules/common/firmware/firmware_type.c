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
#include <linux/bits.h>
#include "firmware_type.h"
#include "../chips/decoder_cpu_ver_info.h"

static const struct format_name_s format_name[] = {
	{VIDEO_DEC_MPEG12,		"mpeg12",		BIT_ULL(0)},
	{VIDEO_DEC_MPEG12_MULTI,	"mpeg12_multi",		BIT_ULL(1)},
	{VIDEO_DEC_MPEG4_3,		"mpeg4_3",		BIT_ULL(2)},
	{VIDEO_DEC_MPEG4_4,		"mpeg4_4",		BIT_ULL(3)},
	{VIDEO_DEC_MPEG4_4_MULTI,	"mpeg4_4_multi",	BIT_ULL(4)},
	{VIDEO_DEC_MPEG4_5,		"xvid",			BIT_ULL(5)},
	{VIDEO_DEC_MPEG4_5_MULTI,	"xvid_multi",		BIT_ULL(6)},
	{VIDEO_DEC_H263,		"h263",			BIT_ULL(7)},
	{VIDEO_DEC_H263_MULTI,		"h263_multi",		BIT_ULL(8)},
	{VIDEO_DEC_MJPEG,		"mjpeg",		BIT_ULL(9)},
	{VIDEO_DEC_MJPEG_MULTI,		"mjpeg_multi",		BIT_ULL(10)},
	{VIDEO_DEC_REAL_V8,		"real_v8",		BIT_ULL(11)},
	{VIDEO_DEC_REAL_V9,		"real_v9",		BIT_ULL(12)},
	{VIDEO_DEC_VC1,			"vc1",			BIT_ULL(13)},
	{VIDEO_DEC_VC1_G12A,		"vc1_g12a",		BIT_ULL(14)},
	{VIDEO_DEC_AVS,			"avs",			BIT_ULL(15)},
	{VIDEO_DEC_AVS_GXM,		"avs_gxm",		BIT_ULL(16)},
	{VIDEO_DEC_AVS_NOCABAC,		"avs_no_cabac",		BIT_ULL(17)},
	{VIDEO_DEC_AVS_MULTI,		"avs_multi",		BIT_ULL(18)},
	{VIDEO_DEC_H264,		"h264",			BIT_ULL(19)},
	{VIDEO_DEC_H264_MVC,		"h264_mvc",		BIT_ULL(20)},
	{VIDEO_DEC_H264_MVC_GXM,	"h264_mvc_gxm",		BIT_ULL(21)},
	{VIDEO_DEC_H264_MULTI,		"h264_multi",		BIT_ULL(22)},
	{VIDEO_DEC_H264_MULTI_MMU,	"h264_multi_mmu",	BIT_ULL(23)},
	{VIDEO_DEC_H264_MULTI_GXM,	"h264_multi_gxm",	BIT_ULL(24)},
	{VIDEO_DEC_HEVC,		"hevc",			BIT_ULL(25)},
	{VIDEO_DEC_HEVC_MMU,		"hevc_mmu",		BIT_ULL(26)},
	{VIDEO_DEC_HEVC_MMU_SWAP,	"hevc_mmu_swap",	BIT_ULL(27)},
	{VIDEO_DEC_HEVC_G12A,		"hevc_g12a",		BIT_ULL(28)},
	{VIDEO_DEC_VP9,			"vp9",			BIT_ULL(29)},
	{VIDEO_DEC_VP9_MMU,		"vp9_mmu",		BIT_ULL(30)},
	{VIDEO_DEC_VP9_G12A,		"vp9_g12a",		BIT_ULL(31)},
	{VIDEO_DEC_AVS2,		"avs2",			BIT_ULL(32)},
	{VIDEO_DEC_AVS2_MMU,		"avs2_mmu",		BIT_ULL(33)},
	{VIDEO_DEC_AV1_MMU,		"av1_mmu",		BIT_ULL(34)},
	{VIDEO_DEC_AVS3,		"avs3_mmu",		BIT_ULL(35)},
	/* front/back ucode */
	{VIDEO_DEC_HEVC_FRONT,		"hevc_front",		BIT_ULL(36)},
	{VIDEO_DEC_HEVC_BACK,		"hevc_back",		BIT_ULL(37)},
	{VIDEO_DEC_VP9_FRONT,		"vp9_front",		BIT_ULL(38)},
	{VIDEO_DEC_VP9_BACK,		"vp9_back",		BIT_ULL(39)},
	{VIDEO_DEC_AV1_FRONT,		"av1_front",		BIT_ULL(40)},
	{VIDEO_DEC_AV1_BACK,		"av1_back",		BIT_ULL(41)},
	{VIDEO_DEC_AVS2_FRONT,		"avs2_front",		BIT_ULL(42)},
	{VIDEO_DEC_AVS2_BACK,		"avs2_back",		BIT_ULL(43)},
	{VIDEO_DEC_AVS3_FRONT,		"avs3_front",		BIT_ULL(44)},
	{VIDEO_DEC_AVS3_BACK,		"avs3_back",		BIT_ULL(45)},

	{VIDEO_ENC_H264,		"h264_enc",		BIT_ULL(46)},
	{VIDEO_ENC_JPEG,		"jpeg_enc",		BIT_ULL(47)},
	{FIRMWARE_MAX,			"unknown"},
};

unsigned long long g_fw_mask = (~0ULL);

static const struct cpu_type_s cpu_type[] = {
	{AM_MESON_CPU_MAJOR_ID_GXL,	"gxl"},
	{AM_MESON_CPU_MAJOR_ID_GXM,	"gxm"},
	{AM_MESON_CPU_MAJOR_ID_TXL,	"txl"},
	{AM_MESON_CPU_MAJOR_ID_TXLX,	"txlx"},
	{AM_MESON_CPU_MAJOR_ID_AXG,	"axg"},
	{AM_MESON_CPU_MAJOR_ID_GXLX,	"gxlx"},
	{AM_MESON_CPU_MAJOR_ID_TXHD,	"txhd"},
	{AM_MESON_CPU_MAJOR_ID_G12A,	"g12a"},
	{AM_MESON_CPU_MAJOR_ID_G12B,	"g12b"},
	{AM_MESON_CPU_MAJOR_ID_GXLX2,	"gxlx2"},
	{AM_MESON_CPU_MAJOR_ID_SM1,	"sm1"},
	{AM_MESON_CPU_MAJOR_ID_TL1,	"tl1"},
	{AM_MESON_CPU_MAJOR_ID_TM2,	"tm2"},
	{AM_MESON_CPU_MAJOR_ID_C1,	"c1"},
	{AM_MESON_CPU_MAJOR_ID_SC2,	"sc2"},
	{AM_MESON_CPU_MAJOR_ID_C2,	"c2"},
	{AM_MESON_CPU_MAJOR_ID_T5,	"t5"},
	{AM_MESON_CPU_MAJOR_ID_T5D,	"t5d"},
	{AM_MESON_CPU_MAJOR_ID_T7,	"t7"},
	{AM_MESON_CPU_MAJOR_ID_S4,	"s4"},
	{AM_MESON_CPU_MAJOR_ID_T3,	"t3"},
	{AM_MESON_CPU_MAJOR_ID_P1,	"p1"},
	{AM_MESON_CPU_MAJOR_ID_S4D,	"s4d"},
	{AM_MESON_CPU_MAJOR_ID_S5,	"s5"},
	{AM_MESON_CPU_MAJOR_ID_GXLX3,	"gxlx3"},
	{AM_MESON_CPU_MAJOR_ID_T5M,	"t5m"},
	{AM_MESON_CPU_MAJOR_ID_T3X,	"t3x"},
	{AM_MESON_CPU_MAJOR_ID_TXHD2,	"txhd2"},
	{AM_MESON_CPU_MAJOR_ID_S1A,	"s1a"},
};

const char *get_fw_format_name(unsigned int format)
{
	const char *name = "unknown";
	int i, size = ARRAY_SIZE(format_name);

	for (i = 0; i < size; i++) {
		if (format == format_name[i].format)
			name = format_name[i].name;
	}

	return name;
}
EXPORT_SYMBOL(get_fw_format_name);

unsigned int get_fw_format(const char *name)
{
	unsigned int format = FIRMWARE_MAX;
	int i, size = ARRAY_SIZE(format_name);

	for (i = 0; i < size; i++) {
		if (!strcmp(name, format_name[i].name))
			format = format_name[i].format;
	}

	return format;
}
EXPORT_SYMBOL(get_fw_format);

int fw_get_cpu(const char *name)
{
	int type = 0;
	int i, size = ARRAY_SIZE(cpu_type);

	for (i = 0; i < size; i++) {
		if (!strcmp(name, cpu_type[i].name))
			type = cpu_type[i].type;
	}

	return type;
}
EXPORT_SYMBOL(fw_get_cpu);

int fw_check_need_load(const char *name)
{
	int idx, size = ARRAY_SIZE(format_name);

	for (idx = 0; idx < size; idx++) {
		if (!strcmp(name, format_name[idx].name)) {
			if (g_fw_mask & format_name[idx].format_mask)
				return true;
			else
				return false;
		}
	}

	pr_info("no %s load info, load it\n", name);
	return -1;
}
EXPORT_SYMBOL(fw_check_need_load);

