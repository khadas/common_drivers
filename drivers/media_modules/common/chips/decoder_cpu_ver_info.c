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
#include <linux/kernel.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/amlogic/media/registers/cpu_version.h>
#include "decoder_cpu_ver_info.h"
#include "../register/register.h"

#define AM_SUCCESS 0
#define MAJOR_ID_START AM_MESON_CPU_MAJOR_ID_M6

static enum AM_MESON_CPU_MAJOR_ID cpu_ver_id = AM_MESON_CPU_MAJOR_ID_MAX;

static int cpu_sub_id = 0;

static bool codec_dos_dev = 0;		//to compat old dts

static struct dos_of_dev_s *platform_dos_dev = NULL;

static struct dos_of_dev_s dos_dev_data[AM_MESON_CPU_MAJOR_ID_MAX - MAJOR_ID_START] = {
	[AM_MESON_CPU_MAJOR_ID_M8B - MAJOR_ID_START] = {
		.chip_id = AM_MESON_CPU_MAJOR_ID_M8B,
		.reg_compat = NULL,
		.max_vdec_clock  = 667,
		.max_hevcf_clock = 667,
		.max_hevcb_clock = 667,
		.hevc_clk_combine_flag  = true,
		.is_hw_parser_support   = true,
		.is_vdec_canvas_support = false,
		.is_support_h264_mmu    = false,
		.is_support_dual_core = false,
		.vdec_max_resolution = RESOLUTION_1080P,
		.hevc_max_resolution = RESOLUTION_1080P,
		.fmt_support_flags = FMT_VDEC_ALL | FMT_HEVC,
	},

	[AM_MESON_CPU_MAJOR_ID_GXL - MAJOR_ID_START] = {
		.chip_id = AM_MESON_CPU_MAJOR_ID_GXL,
		.reg_compat = NULL,
		.max_vdec_clock  = 667,
		.max_hevcf_clock = 667,
		.max_hevcb_clock = 667,
		.hevc_clk_combine_flag  = true,
		.is_hw_parser_support   = true,
		.is_vdec_canvas_support = false,
		.is_support_h264_mmu    = true,
		.is_support_dual_core = false,
		.vdec_max_resolution = RESOLUTION_4K,
		.hevc_max_resolution = RESOLUTION_4K,
		.fmt_support_flags = FMT_VDEC_ALL | FMT_HEVC | FMT_VP9,
	},

	[AM_MESON_CPU_MAJOR_ID_G12A - MAJOR_ID_START] = {
		.chip_id = AM_MESON_CPU_MAJOR_ID_G12A,
		.reg_compat = NULL,
		.max_vdec_clock  = 667,
		.max_hevcf_clock = 667,
		.max_hevcb_clock = 667,	//hevcb clk must be same with hevcf in g12a
		.hevc_clk_combine_flag  = false,
		.is_hw_parser_support   = true,
		.is_vdec_canvas_support = false,
		.is_support_h264_mmu    = true,
		.is_support_dual_core = false,
		.vdec_max_resolution = RESOLUTION_4K,
		.hevc_max_resolution = RESOLUTION_4K,
		.fmt_support_flags = FMT_VDEC_ALL | FMT_HEVC_VP9_AVS2,
	},

	[AM_MESON_CPU_MAJOR_ID_G12B - MAJOR_ID_START] = {
		.chip_id = AM_MESON_CPU_MAJOR_ID_G12B,
		.reg_compat = NULL,
		.max_vdec_clock  = 667,
		.max_hevcf_clock = 667,
		.max_hevcb_clock = 667,
		.hevc_clk_combine_flag	= false,
		.is_hw_parser_support   = true,
		.is_vdec_canvas_support = false,
		.is_support_h264_mmu	= true,
		.is_support_dual_core = false,
		.vdec_max_resolution = RESOLUTION_4K,
		.hevc_max_resolution = RESOLUTION_4K,
		.fmt_support_flags = FMT_VDEC_ALL | FMT_HEVC_VP9_AVS2,
	},

	[AM_MESON_CPU_MAJOR_ID_GXLX2 - MAJOR_ID_START] = {
		.chip_id = AM_MESON_CPU_MAJOR_ID_GXLX2,
		.reg_compat = NULL,
		.max_vdec_clock  = 667,
		.max_hevcf_clock = 667,
		.max_hevcb_clock = 667,
		.hevc_clk_combine_flag	= false,
		.is_hw_parser_support   = true,
		.is_vdec_canvas_support = false,
		.is_support_h264_mmu	= true,
		.is_support_dual_core = false,
		.vdec_max_resolution = RESOLUTION_4K,
		.hevc_max_resolution = RESOLUTION_4K,
		.fmt_support_flags = FMT_VDEC_ALL | FMT_HEVC_VP9_AVS2,
	},

	[AM_MESON_CPU_MAJOR_ID_SM1 - MAJOR_ID_START] = {
		.chip_id = AM_MESON_CPU_MAJOR_ID_SM1,
		.reg_compat = NULL,
		.max_vdec_clock  = 800,
		.max_hevcf_clock = 800,
		.max_hevcb_clock = 800,
		.hevc_clk_combine_flag  = false,
		.is_hw_parser_support   = true,
		.is_vdec_canvas_support = false,
		.is_support_h264_mmu    = true,
		.is_support_dual_core = false,
		.vdec_max_resolution = RESOLUTION_4K,
		.hevc_max_resolution = RESOLUTION_8K,  //support 8kp24
		.fmt_support_flags = FMT_VDEC_ALL | FMT_HEVC_VP9_AVS2,
	},

	[AM_MESON_CPU_MAJOR_ID_TL1 - MAJOR_ID_START] = {
		.chip_id = AM_MESON_CPU_MAJOR_ID_TL1,
		.reg_compat = NULL,
		.max_vdec_clock  = 667,
		.max_hevcf_clock = 800,
		.max_hevcb_clock = 800,
		.hevc_clk_combine_flag  = false,
		.is_hw_parser_support   = true,
		.is_vdec_canvas_support = false,
		.is_support_h264_mmu    = true,
		.is_support_dual_core = false,
		.vdec_max_resolution = RESOLUTION_4K,
		.hevc_max_resolution = RESOLUTION_8K, //support 8kp24
		.fmt_support_flags = FMT_VDEC_ALL | FMT_HEVC_VP9_AVS2,
	},

	[AM_MESON_CPU_MAJOR_ID_TM2 - MAJOR_ID_START] = {
		.chip_id = AM_MESON_CPU_MAJOR_ID_TM2,
		.reg_compat = NULL,
		.max_vdec_clock  = 800,
		.max_hevcf_clock = 800,
		.max_hevcb_clock = 800,
		.hevc_clk_combine_flag  = false,
		.is_hw_parser_support   = true,
		.is_vdec_canvas_support = false,
		.is_support_h264_mmu    = true,
		.is_support_dual_core = false,
		.vdec_max_resolution = RESOLUTION_4K,
		.hevc_max_resolution = RESOLUTION_8K,
		.fmt_support_flags = FMT_VDEC_ALL | FMT_HEVC_VP9_AVS2,
	},

	[AM_MESON_CPU_MAJOR_ID_C1 - MAJOR_ID_START] = {
		.chip_id = AM_MESON_CPU_MAJOR_ID_C1,
		.reg_compat = NULL,
		.is_vdec_canvas_support = true,
		.fmt_support_flags = FMT_JPEG_ENC,
	},


	[AM_MESON_CPU_MAJOR_ID_SC2 - MAJOR_ID_START] = {
		.chip_id = AM_MESON_CPU_MAJOR_ID_SC2,
		.reg_compat = NULL,
		.max_vdec_clock  = 800,
		.max_hevcf_clock = 800,
		.max_hevcb_clock = 800,
		.hevc_clk_combine_flag  = false,
		.is_hw_parser_support   = false,
		.is_vdec_canvas_support = false,
		.is_support_h264_mmu    = true,
		.is_support_dual_core = false,
		.vdec_max_resolution = RESOLUTION_4K,
		.hevc_max_resolution = RESOLUTION_8K,
		.fmt_support_flags = FMT_VDEC_ALL | FMT_HEVC_VP9_AVS2_AV1,
	},

	[AM_MESON_CPU_MAJOR_ID_T5 - MAJOR_ID_START] = {
		.chip_id = AM_MESON_CPU_MAJOR_ID_T5,
		.reg_compat = NULL,
		.max_vdec_clock  = 667,
		.max_hevcf_clock = 667,
		.max_hevcb_clock = 667,
		.hevc_clk_combine_flag  = true,
		.is_hw_parser_support   = true,
		.is_vdec_canvas_support = false,
		.is_support_h264_mmu    = true,
		.is_support_dual_core = false,
		.vdec_max_resolution = RESOLUTION_4K,
		.hevc_max_resolution = RESOLUTION_4K, //unsupport vp9 & av1
		.fmt_support_flags = FMT_VDEC_ALL | FMT_HEVC | FMT_AVS2,
	},

	[AM_MESON_CPU_MAJOR_ID_T5D - MAJOR_ID_START] = {
		.chip_id = AM_MESON_CPU_MAJOR_ID_T5D,
		.reg_compat = NULL,
		.max_vdec_clock  = 667,
		.max_hevcf_clock = 667,
		.max_hevcb_clock = 667,
		.hevc_clk_combine_flag  = true,
		.is_hw_parser_support   = true,
		.is_vdec_canvas_support = false,
		.is_support_h264_mmu    = true,
		.is_support_dual_core = false,
		.vdec_max_resolution = RESOLUTION_1080P,
		.hevc_max_resolution = RESOLUTION_1080P,	//unsupport 4k and avs2
		.fmt_support_flags = FMT_VDEC_ALL | FMT_HEVC_VP9_AV1,
	},

	[AM_MESON_CPU_MAJOR_ID_T7 - MAJOR_ID_START] = {
		.chip_id = AM_MESON_CPU_MAJOR_ID_T7,
		.reg_compat = NULL,
		.max_vdec_clock  = 800,
		.max_hevcf_clock = 800,
		.max_hevcb_clock = 800,
		.hevc_clk_combine_flag  = false,
		.is_hw_parser_support   = false,
		.is_vdec_canvas_support = true,
		.is_support_h264_mmu    = true,
		.is_support_dual_core = false,
		.is_support_axi_ctrl = true,
		.vdec_max_resolution = RESOLUTION_4K,
		.hevc_max_resolution = RESOLUTION_8K,
		.fmt_support_flags = FMT_VDEC_ALL | FMT_HEVC_VP9_AVS2_AV1,
	},

	[AM_MESON_CPU_MAJOR_ID_S4 - MAJOR_ID_START] = {
		.chip_id = AM_MESON_CPU_MAJOR_ID_S4,
		.reg_compat = NULL,
		.max_vdec_clock  = 800,
		.max_hevcf_clock = 800,
		.max_hevcb_clock = 800,
		.hevc_clk_combine_flag  = true,
		.is_hw_parser_support   = false,
		.is_vdec_canvas_support = false,
		.is_support_h264_mmu    = true,
		.is_support_dual_core = false,
		.vdec_max_resolution = RESOLUTION_4K,
		.hevc_max_resolution = RESOLUTION_8K,
		.fmt_support_flags = FMT_VDEC_ALL | FMT_HEVC_VP9_AVS2_AV1,
	},

	[AM_MESON_CPU_MAJOR_ID_T3 - MAJOR_ID_START] = {
		.chip_id = AM_MESON_CPU_MAJOR_ID_T3,
		.reg_compat = t3_mm_registers_compat,
		.max_vdec_clock  = 800,
		.max_hevcf_clock = 800,
		.max_hevcb_clock = 800,
		.hevc_clk_combine_flag  = false,
		.is_hw_parser_support   = false,
		.is_vdec_canvas_support = true,
		.is_support_h264_mmu    = true,
		.is_support_dual_core = false,
		.is_support_rdma     = true,
		.is_support_mmu_copy = true,
		.is_support_axi_ctrl = true,
		.vdec_max_resolution = RESOLUTION_4K,
		.hevc_max_resolution = RESOLUTION_8K,	//8kp30, rdma, mmu copy
		.fmt_support_flags = FMT_VDEC_ALL | FMT_HEVC_VP9_AVS2_AV1,
	},

	[AM_MESON_CPU_MAJOR_ID_S4D - MAJOR_ID_START] = {
		.chip_id = AM_MESON_CPU_MAJOR_ID_S4D,
		.reg_compat = NULL,
		.max_vdec_clock  = 800,
		.max_hevcf_clock = 800,
		.max_hevcb_clock = 800,
		.hevc_clk_combine_flag  = true,
		.is_hw_parser_support   = false,
		.is_vdec_canvas_support = false,
		.is_support_h264_mmu    = true,
		.is_support_dual_core = false,
		.vdec_max_resolution = RESOLUTION_4K,
		.hevc_max_resolution = RESOLUTION_8K,
		.fmt_support_flags = FMT_VDEC_ALL | FMT_HEVC_VP9_AVS2_AV1,
	},

	[AM_MESON_CPU_MAJOR_ID_T5W - MAJOR_ID_START] = {
		.chip_id = AM_MESON_CPU_MAJOR_ID_T5W,
		.reg_compat = NULL,
		.max_vdec_clock  = 800,
		.max_hevcf_clock = 800,
		.max_hevcb_clock = 800,
		.hevc_clk_combine_flag  = true,
		.is_hw_parser_support   = false,
		.is_vdec_canvas_support = true,
		.is_support_h264_mmu    = true,
		.is_support_dual_core = false,
		.vdec_max_resolution = RESOLUTION_4K,
		.hevc_max_resolution = RESOLUTION_8K,
		.fmt_support_flags = FMT_VDEC_ALL | FMT_HEVC_VP9_AVS2_AV1,
	},

	[AM_MESON_CPU_MAJOR_ID_S5 - MAJOR_ID_START] = {
		.chip_id = AM_MESON_CPU_MAJOR_ID_S5,
		.reg_compat = s5_mm_registers_compat,	//register compact
		.max_vdec_clock        = 800,
		.max_hevcf_clock       = 800,
		.max_hevcb_clock       = 800,
		.hevc_clk_combine_flag = true,
		.is_hw_parser_support  = false,
		.is_vdec_canvas_support = true,
		.is_support_h264_mmu   = true,
		.is_support_dual_core  = true,
		.is_support_axi_ctrl = true,
		.hevc_stream_extra_shift = 8,
		.vdec_max_resolution = RESOLUTION_4K,
		.hevc_max_resolution = RESOLUTION_8K,
		.fmt_support_flags = FMT_VDEC_ALL | FMT_HEVC_VP9_AVS2_AV1_AVS3,
	},

	[AM_MESON_CPU_MAJOR_ID_T5M - MAJOR_ID_START] = {
		.chip_id = AM_MESON_CPU_MAJOR_ID_T5M,
		.reg_compat = NULL,
		.max_vdec_clock  = 800,
		.max_hevcf_clock = 800,
		.max_hevcb_clock = 800,
		.hevc_clk_combine_flag  = true,
		.is_hw_parser_support   = false,
		.is_vdec_canvas_support = true,
		.is_support_h264_mmu    = true,
		.is_support_dual_core = false,
		.is_support_axi_ctrl = true,
		.vdec_max_resolution = RESOLUTION_4K,
		.hevc_max_resolution = RESOLUTION_4K,
		.fmt_support_flags = FMT_VDEC_ALL | FMT_HEVC_VP9_AVS2_AV1,
	},

	[AM_MESON_CPU_MAJOR_ID_T3X - MAJOR_ID_START] = {
		.chip_id = AM_MESON_CPU_MAJOR_ID_T3X,
		.reg_compat = s5_mm_registers_compat,	//register compact
		.max_vdec_clock        = 800,
		.max_hevcf_clock       = 800,
		.max_hevcb_clock       = 800,
		.hevc_clk_combine_flag = true,
		.is_hw_parser_support   = false,
		.is_vdec_canvas_support = true,
		.is_support_h264_mmu    = true,
		.is_support_dual_core = false,
		.is_support_rdma     = true,
		.is_support_axi_ctrl = true,
		.hevc_stream_extra_shift = 8,
		.vdec_max_resolution = RESOLUTION_4K,
		.hevc_max_resolution = RESOLUTION_8K,
		.fmt_support_flags = FMT_VDEC_ALL | FMT_HEVC_VP9_AVS2_AV1_AVS3,
	},

	[AM_MESON_CPU_MAJOR_ID_TXHD2 - MAJOR_ID_START] = {
		.chip_id = AM_MESON_CPU_MAJOR_ID_TXHD2,
		.reg_compat = NULL,
		.max_vdec_clock  = 667,
		.max_hevcf_clock = 500,
		.max_hevcb_clock = 500,
		.hevc_clk_combine_flag  = true,
		.is_hw_parser_support   = true,
		.is_vdec_canvas_support = false,
		.is_support_h264_mmu    = true,
		.is_support_dual_core = false,
		.vdec_max_resolution = RESOLUTION_1080P,
		.hevc_max_resolution = RESOLUTION_4K,	//unsupport avs2,av1
		.fmt_support_flags = FMT_VDEC_ALL | FMT_HEVC | FMT_VP9,
	},

	[AM_MESON_CPU_MAJOR_ID_S1A - MAJOR_ID_START] = {
		.chip_id = AM_MESON_CPU_MAJOR_ID_S1A,
		.reg_compat = NULL,
		.max_vdec_clock  = 667,
		.max_hevcf_clock = 500,
		.max_hevcb_clock = 500,
		.hevc_clk_combine_flag  = true,
		.is_hw_parser_support   = false,
		.is_vdec_canvas_support = false,
		.is_support_h264_mmu    = false,
		.is_support_dual_core = false,
		.vdec_max_resolution = RESOLUTION_1080P,
		.hevc_max_resolution = RESOLUTION_1080P,
		.fmt_support_flags = FMT_HEVC | FMT_H264 | FMT_MPEG2 | FMT_MPEG4 | FMT_VC1,
	},
};

/* sub id features */
static struct dos_of_dev_s dos_dev_sub_table[] = {
	{	/* g12b revb */
		.chip_id = AM_MESON_CPU_MINOR_ID_REVB_G12B,
		.reg_compat = NULL,
		.max_vdec_clock  = 667,
		.max_hevcf_clock = 800,
		.max_hevcb_clock = 800,	//g12b revb hevc clk support 800mhz
		.hevc_clk_combine_flag	= true,
		.is_hw_parser_support   = true,
		.is_vdec_canvas_support = false,
		.is_support_h264_mmu	= true,
		.is_support_dual_core = false,
		.vdec_max_resolution = RESOLUTION_4K,
		.hevc_max_resolution = RESOLUTION_8K,
		.fmt_support_flags = FMT_VDEC_ALL | FMT_HEVC_VP9_AVS2,
	},

	{	/* tm2 revb */
		.chip_id = AM_MESON_CPU_MINOR_ID_REVB_TM2,
		.reg_compat = NULL,
		.max_vdec_clock  = 800,
		.max_hevcf_clock = 800,
		.max_hevcb_clock = 800,
		.hevc_clk_combine_flag  = false,
		.is_hw_parser_support   = true,
		.is_vdec_canvas_support = false,
		.is_support_h264_mmu    = true,
		.is_support_dual_core = false,
		.vdec_max_resolution = RESOLUTION_4K,
		.hevc_max_resolution = RESOLUTION_8K,
		.fmt_support_flags = FMT_VDEC_ALL | FMT_HEVC_VP9_AVS2_AV1,
	},

	{
		.chip_id = AM_MESON_CPU_MINOR_ID_S4_S805X2,
		.reg_compat = NULL,
		.max_vdec_clock        = 500,
		.max_hevcf_clock       = 500,
		.max_hevcb_clock       = 500,
		.hevc_clk_combine_flag = true,
		.is_hw_parser_support   = false,
		.is_vdec_canvas_support = false,
		.is_support_h264_mmu    = true,
		.is_support_dual_core = false,
		.vdec_max_resolution = RESOLUTION_1080P,
		.hevc_max_resolution = RESOLUTION_1080P,
		.fmt_support_flags = FMT_VDEC_ALL | FMT_HEVC_VP9_AVS2_AV1,
	},

	{
		.chip_id = AM_MESON_CPU_MINOR_ID_T7C,
		.reg_compat = NULL,
		.max_vdec_clock  = 800,
		.max_hevcf_clock = 800,
		.max_hevcb_clock = 800,
		.hevc_clk_combine_flag  = false,
		.is_hw_parser_support   = false,
		.is_vdec_canvas_support = true,
		.is_support_h264_mmu    = true,
		.is_support_dual_core = false,
		.is_support_axi_ctrl = true,
		.vdec_max_resolution = RESOLUTION_4K,
		.hevc_max_resolution = RESOLUTION_8K,  //fixed endian issue
		.fmt_support_flags = FMT_VDEC_ALL | FMT_HEVC_VP9_AVS2_AV1,
	}
};

/* dos device match table */
static const struct of_device_id cpu_ver_of_match[] = {
	{
		.compatible = "amlogic, cpu-major-id-axg",
		.data = &dos_dev_data[AM_MESON_CPU_MAJOR_ID_AXG - MAJOR_ID_START],
	},

	{
		.compatible = "amlogic, cpu-major-id-g12a",
		.data = &dos_dev_data[AM_MESON_CPU_MAJOR_ID_G12A - MAJOR_ID_START],
	},

	{
		.compatible = "amlogic, cpu-major-id-gxl",
		.data = &dos_dev_data[AM_MESON_CPU_MAJOR_ID_GXL - MAJOR_ID_START],
	},

	{
		.compatible = "amlogic, cpu-major-id-gxm",
		.data = &dos_dev_data[AM_MESON_CPU_MAJOR_ID_GXM - MAJOR_ID_START],
	},

	{
		.compatible = "amlogic, cpu-major-id-txl",
		.data = &dos_dev_data[AM_MESON_CPU_MAJOR_ID_TXL - MAJOR_ID_START],
	},

	{
		.compatible = "amlogic, cpu-major-id-txlx",
		.data = &dos_dev_data[AM_MESON_CPU_MAJOR_ID_TXLX - MAJOR_ID_START],
	},

	{
		.compatible = "amlogic, cpu-major-id-sm1",
		.data = &dos_dev_data[AM_MESON_CPU_MAJOR_ID_SM1 - MAJOR_ID_START],
	},

	{
		.compatible = "amlogic, cpu-major-id-tl1",
		.data = &dos_dev_data[AM_MESON_CPU_MAJOR_ID_TL1 - MAJOR_ID_START],
	},
	{
		.compatible = "amlogic, cpu-major-id-tm2",
		.data = &dos_dev_data[AM_MESON_CPU_MAJOR_ID_TM2 - MAJOR_ID_START],
	},
	{
		.compatible = "amlogic, cpu-major-id-c1",
		.data = &dos_dev_data[AM_MESON_CPU_MAJOR_ID_C1 - MAJOR_ID_START],
	},
	{
		.compatible = "amlogic, cpu-major-id-sc2",
		.data = &dos_dev_data[AM_MESON_CPU_MAJOR_ID_SC2 - MAJOR_ID_START],
	},
	{
		.compatible = "amlogic, cpu-major-id-t5",
		.data = &dos_dev_data[AM_MESON_CPU_MAJOR_ID_T5 - MAJOR_ID_START],
	},
	{
		.compatible = "amlogic, cpu-major-id-t5d",
		.data = &dos_dev_data[AM_MESON_CPU_MAJOR_ID_T5D - MAJOR_ID_START],
	},
	{
		.compatible = "amlogic, cpu-major-id-t7",
		.data = &dos_dev_data[AM_MESON_CPU_MAJOR_ID_T7 - MAJOR_ID_START],
	},
	{
		.compatible = "amlogic, cpu-major-id-s4",
		.data = &dos_dev_data[AM_MESON_CPU_MAJOR_ID_S4 - MAJOR_ID_START],
	},
	{
		.compatible = "amlogic, cpu-major-id-t3",
		.data = &dos_dev_data[AM_MESON_CPU_MAJOR_ID_T3 - MAJOR_ID_START],
	},
	{
		.compatible = "amlogic, cpu-major-id-p1",
		.data = &dos_dev_data[AM_MESON_CPU_MAJOR_ID_P1 - MAJOR_ID_START],
	},
	{
		.compatible = "amlogic, cpu-major-id-s4d",
		.data = &dos_dev_data[AM_MESON_CPU_MAJOR_ID_S4D - MAJOR_ID_START],
	},
	{
		.compatible = "amlogic, cpu-major-id-t5w",
		.data = &dos_dev_data[AM_MESON_CPU_MAJOR_ID_T5W - MAJOR_ID_START],
	},
	{
		.compatible = "amlogic, cpu-major-id-s5",
		.data = &dos_dev_data[AM_MESON_CPU_MAJOR_ID_S5 - MAJOR_ID_START],
	},
	{
		.compatible = "amlogic, cpu-major-id-t5m",
		.data = &dos_dev_data[AM_MESON_CPU_MAJOR_ID_T5M - MAJOR_ID_START],
	},
	{
		.compatible = "amlogic, cpu-major-id-t3x",
		.data = &dos_dev_data[AM_MESON_CPU_MAJOR_ID_T3X - MAJOR_ID_START],
	},
	{
		.compatible = "amlogic, cpu-major-id-txhd2",
		.data = &dos_dev_data[AM_MESON_CPU_MAJOR_ID_TXHD2 - MAJOR_ID_START],
	},
	{
		.compatible = "amlogic, cpu-major-id-s1a",
		.data = &dos_dev_data[AM_MESON_CPU_MAJOR_ID_S1A - MAJOR_ID_START],
	},
	{},
};

static const struct of_device_id cpu_sub_id_of_match[] = {
	{
		.compatible = "amlogic, cpu-major-id-g12b-b",
		.data = &dos_dev_sub_table[0],
	},
	{
		.compatible = "amlogic, cpu-major-id-tm2-b",
		.data = &dos_dev_sub_table[1],
	},
	{
		.compatible = "amlogic, cpu-major-id-s4-805x2",
		.data = &dos_dev_sub_table[2],
	},
	{
		.compatible = "amlogic, cpu-major-id-t7c",
		.data = &dos_dev_sub_table[3],
	},
	{}
};

static struct platform_device *get_dos_dev_from_dtb(void)
{
	struct device_node *pnode = NULL;

	pnode = of_find_node_by_name(NULL, CODEC_DOS_DEV_ID_NODE_NAME);
	if (pnode == NULL) {
		codec_dos_dev = false;
		pnode = of_find_node_by_name(NULL, DECODE_CPU_VER_ID_NODE_NAME);
		if (pnode == NULL) {
			pr_err("No find node.\n");
			return NULL;
		}
	} else {
		codec_dos_dev = true;
	}

	return of_find_device_by_node(pnode);
}

static struct dos_of_dev_s * get_dos_of_dev_data(struct platform_device *pdev)
{
	const struct of_device_id *pmatch = NULL;

	if (pdev == NULL)
		return NULL;

	pmatch = of_match_device(cpu_ver_of_match, &pdev->dev);
	if (NULL == pmatch) {
		pmatch = of_match_device(cpu_sub_id_of_match, &pdev->dev);
		if (NULL == pmatch) {
			pr_err("No find of_match_device\n");
			return NULL;
		}
	}

	return (struct dos_of_dev_s *)pmatch->data;
}

/* dos to get platform data */
static int dos_device_search_data(int id, int sub_id)
{
	int i, j;
	int sub_dev_id;

	for (i = 0; i < ARRAY_SIZE(dos_dev_data); i++) {
		if (id == dos_dev_data[i].chip_id) {
			platform_dos_dev = &dos_dev_data[i];
			pr_info("%s, get major %d dos dev data success\n", __func__, i);

			if (sub_id) {
				for (j = 0; j < ARRAY_SIZE(dos_dev_sub_table); j++) {
					if (id == (dos_dev_sub_table[j].chip_id & MAJOR_ID_MASK)) {
						sub_dev_id = (dos_dev_sub_table[j].chip_id & SUB_ID_MASK) >> 8;
						if (sub_id == sub_dev_id) {
							platform_dos_dev = &dos_dev_sub_table[j];
							pr_info("%s, get sub %d dos dev data success\n", __func__, j);
						}
					}
				}
			}
		}
	}
	if (platform_dos_dev)
		return 0;
	else
		return -ENODEV;
}

struct platform_device *initial_dos_device(void)
{
	struct platform_device *pdev = NULL;
	struct dos_of_dev_s *of_dev_data;

	pdev = get_dos_dev_from_dtb();

	of_dev_data = get_dos_of_dev_data(pdev);

	if (of_dev_data) {
		cpu_ver_id = of_dev_data->chip_id & MAJOY_ID_MASK;
		cpu_sub_id = (of_dev_data->chip_id & SUB_ID_MASK) >> 8;
		platform_dos_dev = of_dev_data;
	} else {
		cpu_ver_id = (enum AM_MESON_CPU_MAJOR_ID)get_cpu_type();
		cpu_sub_id = is_meson_rev_b() ? CHIP_REVB :
					(is_meson_rev_c() ? CHIP_REVC : CHIP_REVA);

		pr_info("get dos dev failed, id %d(%d), try to search\n",
			cpu_ver_id, cpu_sub_id);

		if (dos_device_search_data(cpu_ver_id, cpu_sub_id) < 0) {
			pr_err("get dos device failed, dos maybe out of work\n");
			//return NULL;
		}
	}
	if ((cpu_ver_id == AM_MESON_CPU_MAJOR_ID_G12B) &&
		(cpu_sub_id == CHIP_REVB))
		cpu_ver_id = AM_MESON_CPU_MAJOR_ID_TL1;

	if (pdev && of_dev_data)
		dos_register_probe(pdev, of_dev_data->reg_compat);

	pr_info("initial_dos_device end, chip %d(%d)\n",
		cpu_ver_id, cpu_sub_id);

	return pdev;
}
EXPORT_SYMBOL(initial_dos_device);

enum AM_MESON_CPU_MAJOR_ID get_cpu_major_id(void)
{
	if (AM_MESON_CPU_MAJOR_ID_MAX == cpu_ver_id)
		initial_dos_device();

	return cpu_ver_id;
}
EXPORT_SYMBOL(get_cpu_major_id);

int get_cpu_sub_id(void)
{
	return cpu_sub_id;
}
EXPORT_SYMBOL(get_cpu_sub_id);

bool is_cpu_meson_revb(void)
{
	if (AM_MESON_CPU_MAJOR_ID_MAX == cpu_ver_id)
		initial_dos_device();

	return (cpu_sub_id == CHIP_REVB);
}
EXPORT_SYMBOL(is_cpu_meson_revb);

bool is_cpu_tm2_revb(void)
{
	return ((get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_TM2)
		&& (is_cpu_meson_revb()));
}
EXPORT_SYMBOL(is_cpu_tm2_revb);

bool is_cpu_s4_s805x2(void)
{
	return ((get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_S4)
		&& (get_cpu_sub_id() == CHIP_REVX));
}
EXPORT_SYMBOL(is_cpu_s4_s805x2);

bool is_cpu_t7(void)
{
	return ((get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_T7)
		&& (get_cpu_sub_id() != CHIP_REVC));
}
EXPORT_SYMBOL(is_cpu_t7);

bool is_cpu_t7c(void)
{
	return ((get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_T7)
		&& (get_cpu_sub_id() == CHIP_REVC));
}
EXPORT_SYMBOL(is_cpu_t7c);

/*
	feature from dos dev functions
*/
/*
bit0: force support no_parser;
bit1: force support all video format;
*/
#define FORCE_VDEC_NO_PARSER     BIT(0)
#define FORCE_VDEC_SUPPORT_FMT   BIT(1)
static u32 force_dos_support;

inline bool is_hevc_align32(int blkmod)
{
	if ((get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_TXHD2) ||
		(get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_S1A) ||
		(get_cpu_major_id() == AM_MESON_CPU_MAJOR_ID_G12A))
		return true;

	return false;
}
EXPORT_SYMBOL(is_hevc_align32);

inline bool is_support_new_dos_dev(void)
{
	return codec_dos_dev;
}
EXPORT_SYMBOL(is_support_new_dos_dev);

inline struct dos_of_dev_s *dos_dev_get(void)
{
	return platform_dos_dev;
}
EXPORT_SYMBOL(dos_dev_get);


/* vdec & hevc clock */
inline u32 vdec_max_clk_get(void)
{
	return platform_dos_dev->max_vdec_clock;
}
EXPORT_SYMBOL(vdec_max_clk_get);

inline u32 hevcf_max_clk_get(void)
{
	return platform_dos_dev->max_hevcf_clock;
}
EXPORT_SYMBOL(hevcf_max_clk_get);

inline u32 hevcb_max_clk_get(void)
{
	return platform_dos_dev->max_hevcb_clock;
}
EXPORT_SYMBOL(hevcb_max_clk_get);

inline bool is_hevc_clk_combined(void)
{
	return (platform_dos_dev->hevc_clk_combine_flag);
}
EXPORT_SYMBOL(is_hevc_clk_combined);

/* ressolution */
inline int vdec_is_support_4k(void)
{
	if (platform_dos_dev->vdec_max_resolution > RESOLUTION_1080P)
		return true;
	else
		return false;
}
EXPORT_SYMBOL(vdec_is_support_4k);

inline int hevc_is_support_4k(void)
{
	if (platform_dos_dev->hevc_max_resolution > RESOLUTION_1080P)
		return true;
	else
		return false;
}
EXPORT_SYMBOL(hevc_is_support_4k);

inline int hevc_is_support_8k(void)
{
	if (platform_dos_dev->hevc_max_resolution > RESOLUTION_4K)
		return true;

	return false;
}
EXPORT_SYMBOL(hevc_is_support_8k);

inline bool is_oversize_vdec(int w, int h)
{
	if (w < 0 || h < 0)
		return true;

	if (h != 0 && (w > platform_dos_dev->vdec_max_resolution / h))
		return true;

	return false;
}
EXPORT_SYMBOL(is_oversize_vdec);

inline bool is_oversize_hevc(int w, int h)
{
	if (w < 0 || h < 0)
		return true;

	if (h != 0 && (w > platform_dos_dev->hevc_max_resolution / h))
		return true;

	return false;
}
EXPORT_SYMBOL(is_oversize_hevc);

inline bool is_support_no_parser(void)
{
	return ((force_dos_support & FORCE_VDEC_NO_PARSER) ||
		(!platform_dos_dev->is_hw_parser_support));
}
EXPORT_SYMBOL(is_support_no_parser);

/* vdec intra canvas feature */
inline bool is_support_vdec_canvas(void)
{
	return (platform_dos_dev->is_vdec_canvas_support);
}
EXPORT_SYMBOL(is_support_vdec_canvas);

inline bool is_support_dual_core(void)
{
	return (platform_dos_dev->is_support_dual_core);
}
EXPORT_SYMBOL(is_support_dual_core);

inline bool is_support_p010_mode(void)
{
	return (platform_dos_dev->is_support_p010);
}
EXPORT_SYMBOL(is_support_p010_mode);

inline bool is_support_triple_write(void)
{
	return (platform_dos_dev->is_support_triple_write);
}
EXPORT_SYMBOL(is_support_triple_write);

inline bool is_support_rdma(void)
{
	return (platform_dos_dev->is_support_rdma);
}
EXPORT_SYMBOL(is_support_rdma);

inline bool is_support_mmu_copy(void)
{
	return (platform_dos_dev->is_support_mmu_copy);
}
EXPORT_SYMBOL(is_support_mmu_copy);

inline bool is_support_axi_ctrl(void)
{
	return platform_dos_dev->is_support_axi_ctrl;
}
EXPORT_SYMBOL(is_support_axi_ctrl);

inline bool is_support_format(int format)
{
	if ((platform_dos_dev->fmt_support_flags == 0) ||
		(force_dos_support & FORCE_VDEC_SUPPORT_FMT))
		return true;

	return ((1 << format) &
		platform_dos_dev->fmt_support_flags);
}
EXPORT_SYMBOL(is_support_format);

inline int get_hevc_stream_extra_shift_bytes(void)
{
	return platform_dos_dev->hevc_stream_extra_shift;
}
EXPORT_SYMBOL(get_hevc_stream_extra_shift_bytes);

void pr_dos_infos(void)
{
	pr_info("dos device info:\n");
	pr_info("chip id: 0x%x(0x%x)\n", cpu_ver_id, cpu_sub_id);

	pr_info("max vdec  clock: %03d MHz\n", vdec_max_clk_get());
	pr_info("max hevcf clock: %03d MHz\n", hevcf_max_clk_get());
	if (!is_hevc_clk_combined()) {
		pr_info("max hevcb clock: %03d MHz\n", hevcb_max_clk_get());
	}

	pr_info("max vdec resolution : %s\n",
		platform_dos_dev->vdec_max_resolution > RESOLUTION_1080P ? "4K":"1080P");
	pr_info("max hevc resolution : %s\n",
		platform_dos_dev->hevc_max_resolution < RESOLUTION_4K ? "1080P" :
		(platform_dos_dev->hevc_max_resolution > RESOLUTION_4K ? "8K":"4K"));

	pr_info("support no parser   : %d\n", is_support_no_parser());
	pr_info("support vdec canvas : %d\n", is_support_vdec_canvas());
	pr_info("support dual core   : %d\n", is_support_dual_core());
	pr_info("support p010 mode   : %d\n", is_support_p010_mode());
	pr_info("support triple write: %d\n", is_support_triple_write());
	pr_info("support rdma        : %d\n", is_support_rdma());
	pr_info("support mmu copy    : %d\n", is_support_mmu_copy());
	pr_info("support dos axi ctrl: %d\n", is_support_axi_ctrl());
	pr_info("support format      : 0x%x\n", platform_dos_dev->fmt_support_flags);
	pr_info("hevc_stream_extra_shift_bytes: %d\n", get_hevc_stream_extra_shift_bytes());
}
EXPORT_SYMBOL(pr_dos_infos);

void dos_info_debug(void)
{
	int i;
	struct dos_of_dev_s *save = platform_dos_dev;
	int save_major_id = cpu_ver_id;
	int save_sub_id = cpu_sub_id;

	for (i = 0; i < ARRAY_SIZE(dos_dev_data); i++) {
		platform_dos_dev = &dos_dev_data[i];
		if (platform_dos_dev->chip_id) {
			cpu_ver_id = platform_dos_dev->chip_id;
			pr_dos_infos();
			pr_info("\n");
		}
	}
	for (i = 0; i < ARRAY_SIZE(dos_dev_sub_table); i++) {
		platform_dos_dev = &dos_dev_sub_table[i];
		if (platform_dos_dev->chip_id) {
			cpu_ver_id = platform_dos_dev->chip_id & MAJOR_ID_MASK;
			cpu_sub_id = (platform_dos_dev->chip_id & SUB_ID_MASK) >> 8;
			pr_dos_infos();
			pr_info("\n");
		}
	}

	platform_dos_dev = save;
	cpu_ver_id = save_major_id;
	cpu_sub_id = save_sub_id;
}
EXPORT_SYMBOL(dos_info_debug);


module_param(force_dos_support, uint, 0664);

