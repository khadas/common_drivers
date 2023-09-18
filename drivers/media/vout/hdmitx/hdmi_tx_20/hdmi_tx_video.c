// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/mm.h>
#include <linux/major.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/cdev.h>

#include <linux/amlogic/media/vout/hdmi_tx/hdmi_info_global.h>
#include <linux/amlogic/media/vout/hdmi_tx/hdmi_tx_module.h>
#include <linux/amlogic/media/vout/hdmi_tx/hdmi_tx_compliance.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_common.h>

#include "hw/common.h"

/* Old definitions */
#define TYPE_AVI_INFOFRAMES       0x82
#define AVI_INFOFRAMES_VERSION    0x02
#define AVI_INFOFRAMES_LENGTH     0x0D

static void hdmitx_set_spd_info(struct hdmitx_dev *hdmitx_device);
static void hdmi_set_vend_spec_infofram(struct hdmitx_dev *hdev,
					enum hdmi_vic videocode);

static struct hdmitx_vidpara hdmi_tx_video_params[] = {
	{
		.VIC		= HDMI_1_640x480p60_4x3,
		.color_prefer   = HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio   = TV_ASPECT_RATIO_4_3,
		.cc		= CC_ITU601,
		.ss		= SS_SCAN_UNDER,
		.sc		= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMI_2_720x480p60_4x3,
		.color_prefer   = HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio   = TV_ASPECT_RATIO_4_3,
		.cc		= CC_ITU601,
		.ss		= SS_SCAN_UNDER,
		.sc		= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMI_3_720x480p60_16x9,
		.color_prefer   = HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio   = TV_ASPECT_RATIO_16_9,
		.cc		= CC_ITU601,
		.ss		= SS_SCAN_UNDER,
		.sc		= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMI_36_2880x480p60_16x9,
		.color_prefer   = HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= HDMI_4_TIMES_REPEAT,
		.aspect_ratio   = TV_ASPECT_RATIO_16_9,
		.cc		= CC_ITU601,
		.ss		= SS_SCAN_UNDER,
		.sc		= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMI_4_1280x720p60_16x9,
		.color_prefer   = HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio   = TV_ASPECT_RATIO_16_9,
		.cc		= CC_ITU709,
		.ss		= SS_SCAN_UNDER,
		.sc		= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMI_5_1920x1080i60_16x9,
		.color_prefer   = HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio   = TV_ASPECT_RATIO_16_9,
		.cc		= CC_ITU709,
		.ss		= SS_SCAN_UNDER,
		.sc		= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMI_6_720x480i60_4x3,
		.color_prefer   = HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= HDMI_2_TIMES_REPEAT,
		.aspect_ratio   = TV_ASPECT_RATIO_4_3,
		.cc		= CC_ITU601,
		.ss		= SS_SCAN_UNDER,
		.sc		= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMI_7_720x480i60_16x9,
		.color_prefer   = HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= HDMI_2_TIMES_REPEAT,
		.aspect_ratio   = TV_ASPECT_RATIO_16_9,
		.cc		= CC_ITU601,
		.ss		= SS_SCAN_UNDER,
		.sc		= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMI_11_2880x480i60_16x9,
		.color_prefer   = HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= HDMI_4_TIMES_REPEAT,
		.aspect_ratio   = TV_ASPECT_RATIO_16_9,
		.cc		= CC_ITU601,
		.ss		= SS_SCAN_UNDER,
		.sc		= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMI_14_1440x480p60_4x3,
		.color_prefer   = HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio   = TV_ASPECT_RATIO_4_3,
		.cc		= CC_ITU601,
		.ss		= SS_SCAN_UNDER,
		.sc		= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMI_16_1920x1080p60_16x9,
		.color_prefer   = HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio   = TV_ASPECT_RATIO_16_9,
		.cc		= CC_ITU709,
		.ss		= SS_SCAN_UNDER,
		.sc		= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMI_17_720x576p50_4x3,
		.color_prefer   = HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio   = TV_ASPECT_RATIO_4_3,
		.cc		= CC_ITU601,
		.ss		= SS_SCAN_UNDER,
		.sc		= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMI_18_720x576p50_16x9,
		.color_prefer   = HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio   = TV_ASPECT_RATIO_16_9,
		.cc		= CC_ITU601,
		.ss		= SS_SCAN_UNDER,
		.sc		= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMI_38_2880x576p50_16x9,
		.color_prefer   = HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= HDMI_4_TIMES_REPEAT,
		.aspect_ratio   = TV_ASPECT_RATIO_16_9,
		.cc		= CC_ITU601,
		.ss		= SS_SCAN_UNDER,
		.sc		= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMI_19_1280x720p50_16x9,
		.color_prefer   = HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio   = TV_ASPECT_RATIO_16_9,
		.cc		= CC_ITU709,
		.ss		= SS_SCAN_UNDER,
		.sc		= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMI_20_1920x1080i50_16x9,
		.color_prefer   = HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio   = TV_ASPECT_RATIO_16_9,
		.cc		= CC_ITU709,
		.ss		= SS_SCAN_UNDER,
		.sc		= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMI_21_720x576i50_4x3,
		.color_prefer   = HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= HDMI_2_TIMES_REPEAT,
		.aspect_ratio   = TV_ASPECT_RATIO_4_3,
		.cc		= CC_ITU601,
		.ss		= SS_SCAN_UNDER,
		.sc		= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMI_22_720x576i50_16x9,
		.color_prefer   = HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= HDMI_2_TIMES_REPEAT,
		.aspect_ratio   = TV_ASPECT_RATIO_16_9,
		.cc		= CC_ITU601,
		.ss		= SS_SCAN_UNDER,
		.sc		= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMI_26_2880x576i50_16x9,
		.color_prefer   = HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= HDMI_4_TIMES_REPEAT,
		.aspect_ratio   = TV_ASPECT_RATIO_16_9,
		.cc		= CC_ITU601,
		.ss		= SS_SCAN_UNDER,
		.sc		= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMI_31_1920x1080p50_16x9,
		.color_prefer   = HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio   = TV_ASPECT_RATIO_16_9,
		.cc		= CC_ITU709,
		.ss		= SS_SCAN_UNDER,
		.sc		= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMI_32_1920x1080p24_16x9,
		.color_prefer   = HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio   = TV_ASPECT_RATIO_16_9,
		.cc		= CC_ITU709,
		.ss		= SS_SCAN_UNDER,
		.sc		= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMI_33_1920x1080p25_16x9,
		.color_prefer   = HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio   = TV_ASPECT_RATIO_16_9,
		.cc		= CC_ITU709,
		.ss		= SS_SCAN_UNDER,
		.sc		= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMI_34_1920x1080p30_16x9,
		.color_prefer   = HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio   = TV_ASPECT_RATIO_16_9,
		.cc		= CC_ITU709,
		.ss		= SS_SCAN_UNDER,
		.sc		= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMI_63_1920x1080p120_16x9,
		.color_prefer   = HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio   = TV_ASPECT_RATIO_16_9,
		.cc		= CC_ITU709,
		.ss		= SS_SCAN_UNDER,
		.sc		= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMI_95_3840x2160p30_16x9,
		.color_prefer   = HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio   = TV_ASPECT_RATIO_16_9,
		.cc		= CC_ITU709,
		.ss		= SS_SCAN_UNDER,
		.sc		= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMI_94_3840x2160p25_16x9,
		.color_prefer   = HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio   = TV_ASPECT_RATIO_16_9,
		.cc		= CC_ITU709,
		.ss		= SS_SCAN_UNDER,
		.sc		= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMI_93_3840x2160p24_16x9,
		.color_prefer   = HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio   = TV_ASPECT_RATIO_16_9,
		.cc		= CC_ITU709,
		.ss		= SS_SCAN_UNDER,
		.sc		= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMI_98_4096x2160p24_256x135,
		.color_prefer   = HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio   = TV_ASPECT_RATIO_16_9,
		.cc		= CC_ITU709,
		.ss		= SS_SCAN_UNDER,
		.sc		= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMI_99_4096x2160p25_256x135,
		.color_prefer   = HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio   = TV_ASPECT_RATIO_16_9,
		.cc		= CC_ITU709,
		.ss		= SS_SCAN_UNDER,
		.sc		= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMI_100_4096x2160p30_256x135,
		.color_prefer   = HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio   = TV_ASPECT_RATIO_16_9,
		.cc		= CC_ITU709,
		.ss		= SS_SCAN_UNDER,
		.sc		= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMI_101_4096x2160p50_256x135,
		.color_prefer   = HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio   = TV_ASPECT_RATIO_16_9,
		.cc		= CC_ITU709,
		.ss		= SS_SCAN_UNDER,
		.sc		= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMI_102_4096x2160p60_256x135,
		.color_prefer   = HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio   = TV_ASPECT_RATIO_16_9,
		.cc		= CC_ITU709,
		.ss		= SS_SCAN_UNDER,
		.sc		= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMI_97_3840x2160p60_16x9,
		.color_prefer   = HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio   = TV_ASPECT_RATIO_16_9,
		.cc		= CC_ITU709,
		.ss		= SS_SCAN_UNDER,
		.sc		= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMI_96_3840x2160p50_16x9,
		.color_prefer   = HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio   = TV_ASPECT_RATIO_16_9,
		.cc		= CC_ITU709,
		.ss		= SS_SCAN_UNDER,
		.sc		= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMI_89_2560x1080p50_64x27,
		.color_prefer	= HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio	= TV_ASPECT_RATIO_16_9,
		.cc = CC_ITU709,
		.ss = SS_SCAN_UNDER,
		.sc = SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMI_90_2560x1080p60_64x27,
		.color_prefer	= HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio	= TV_ASPECT_RATIO_16_9,
		.cc = CC_ITU709,
		.ss = SS_SCAN_UNDER,
		.sc = SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMIV_640x480p60hz,
		.color_prefer	= HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio	= TV_ASPECT_RATIO_4_3,
		.cc			= CC_ITU709,
		.ss			= SS_SCAN_UNDER,
		.sc			= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMIV_800x480p60hz,
		.color_prefer	= HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio	= TV_ASPECT_RATIO_16_9,
		.cc			= CC_ITU709,
		.ss			= SS_SCAN_UNDER,
		.sc			= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMIV_800x600p60hz,
		.color_prefer	= HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio	= TV_ASPECT_RATIO_4_3,
		.cc			= CC_ITU709,
		.ss			= SS_SCAN_UNDER,
		.sc			= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMIV_854x480p60hz,
		.color_prefer	= HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio	= TV_ASPECT_RATIO_16_9,
		.cc			= CC_ITU709,
		.ss			= SS_SCAN_UNDER,
		.sc			= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMIV_852x480p60hz,
		.color_prefer	= HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio	= TV_ASPECT_RATIO_16_9,
		.cc			= CC_ITU709,
		.ss			= SS_SCAN_UNDER,
		.sc			= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMIV_1024x600p60hz,
		.color_prefer	= HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio	= TV_ASPECT_RATIO_16_9,
		.cc			= CC_ITU709,
		.ss			= SS_SCAN_UNDER,
		.sc			= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMIV_1024x768p60hz,
		.color_prefer	= HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio	= TV_ASPECT_RATIO_4_3,
		.cc			= CC_ITU709,
		.ss			= SS_SCAN_UNDER,
		.sc			= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMIV_1152x864p75hz,
		.color_prefer	= HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio	= TV_ASPECT_RATIO_4_3,
		.cc			= CC_ITU709,
		.ss			= SS_SCAN_UNDER,
		.sc			= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMIV_1280x600p60hz,
		.color_prefer	= HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio	= TV_ASPECT_RATIO_16_9,
		.cc			= CC_ITU709,
		.ss			= SS_SCAN_UNDER,
		.sc			= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMIV_1280x768p60hz,
		.color_prefer	= HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio	= TV_ASPECT_RATIO_16_9,
		.cc			= CC_ITU709,
		.ss			= SS_SCAN_UNDER,
		.sc			= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMIV_1280x800p60hz,
		.color_prefer	= HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio	= TV_ASPECT_RATIO_16_9,
		.cc			= CC_ITU709,
		.ss			= SS_SCAN_UNDER,
		.sc			= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMIV_1280x960p60hz,
		.color_prefer	= HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio	= TV_ASPECT_RATIO_16_9,
		.cc			= CC_ITU709,
		.ss			= SS_SCAN_UNDER,
		.sc			= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMIV_1280x1024p60hz,
		.color_prefer	= HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio	= TV_ASPECT_RATIO_4_3,
		.cc			= CC_ITU709,
		.ss			= SS_SCAN_UNDER,
		.sc			= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMIV_1360x768p60hz,
		.color_prefer	= HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio	= TV_ASPECT_RATIO_16_9,
		.cc			= CC_ITU709,
		.ss			= SS_SCAN_UNDER,
		.sc			= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMIV_1366x768p60hz,
		.color_prefer	= HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio	= TV_ASPECT_RATIO_16_9,
		.cc			= CC_ITU709,
		.ss			= SS_SCAN_UNDER,
		.sc			= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMIV_1400x1050p60hz,
		.color_prefer	= HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio	= TV_ASPECT_RATIO_16_9,
		.cc			= CC_ITU709,
		.ss			= SS_SCAN_UNDER,
		.sc			= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMIV_1440x900p60hz,
		.color_prefer	= HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio	= TV_ASPECT_RATIO_16_9,
		.cc			= CC_ITU709,
		.ss			= SS_SCAN_UNDER,
		.sc			= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMIV_1440x2560p60hz,
		.color_prefer	= HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio	= TV_ASPECT_RATIO_16_9,
		.cc		= CC_ITU709,
		.ss		= SS_SCAN_UNDER,
		.sc		= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMIV_1600x900p60hz,
		.color_prefer	= HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio	= TV_ASPECT_RATIO_16_9,
		.cc			= CC_ITU709,
		.ss			= SS_SCAN_UNDER,
		.sc			= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMIV_1600x1200p60hz,
		.color_prefer	= HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio	= TV_ASPECT_RATIO_16_9,
		.cc			= CC_ITU709,
		.ss			= SS_SCAN_UNDER,
		.sc			= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMIV_1680x1050p60hz,
		.color_prefer	= HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio	= TV_ASPECT_RATIO_16_9,
		.cc			= CC_ITU709,
		.ss			= SS_SCAN_UNDER,
		.sc			= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMIV_1920x1200p60hz,
		.color_prefer	= HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio	= TV_ASPECT_RATIO_16_9,
		.cc			= CC_ITU709,
		.ss			= SS_SCAN_UNDER,
		.sc			= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMIV_2048x1080p24hz,
		.color_prefer	= HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio	= ASPECT_RATIO_SAME_AS_SOURCE,
		.cc			= CC_ITU709,
		.ss			= SS_SCAN_UNDER,
		.sc			= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMIV_2160x1200p90hz,
		.color_prefer	= HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio	= TV_ASPECT_RATIO_4_3,
		.cc			= CC_ITU709,
		.ss			= SS_SCAN_UNDER,
		.sc			= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMIV_2560x1080p60hz,
		.color_prefer	= HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio	= TV_ASPECT_RATIO_16_9,
		.cc			= CC_ITU709,
		.ss			= SS_SCAN_UNDER,
		.sc			= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMIV_2560x1440p60hz,
		.color_prefer	= HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio	= TV_ASPECT_RATIO_16_9,
		.cc			= CC_ITU709,
		.ss			= SS_SCAN_UNDER,
		.sc			= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMIV_2560x1600p60hz,
		.color_prefer	= HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio	= TV_ASPECT_RATIO_16_9,
		.cc			= CC_ITU709,
		.ss			= SS_SCAN_UNDER,
		.sc			= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMIV_2560x1440p60hz,
		.color_prefer	= HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_BAR_VERT_HORIZ,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio	= TV_ASPECT_RATIO_16_9,
		.cc			= CC_ITU709,
		.ss			= SS_SCAN_UNDER,
		.sc			= SC_SCALE_HORIZ_VERT,
	},
	{
		.VIC		= HDMIV_3440x1440p60hz,
		.color_prefer	= HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_INVALID,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio	= ASPECT_RATIO_SAME_AS_SOURCE,
		.cc = CC_NO_DATA,
		.ss = SS_NO_DATA,
		.sc = SC_NO_UINFORM,
	},
	{
		.VIC		= HDMIV_2400x1200p90hz,
		.color_prefer	= HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_INVALID,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio	= ASPECT_RATIO_SAME_AS_SOURCE,
		.cc = CC_NO_DATA,
		.ss = SS_NO_DATA,
		.sc = SC_NO_UINFORM,
	},
	{
		.VIC		= HDMIV_3840x1080p60hz,
		.color_prefer	= HDMI_COLORSPACE_RGB,
		.color_depth	= COLORDEPTH_24B,
		.bar_info	= B_INVALID,
		.repeat_time	= NO_REPEAT,
		.aspect_ratio	= ASPECT_RATIO_SAME_AS_SOURCE,
		.cc = CC_NO_DATA,
		.ss = SS_NO_DATA,
		.sc = SC_NO_UINFORM,
	},
};

static struct
hdmitx_vidpara *hdmi_get_video_param(enum hdmi_vic videocode)
{
	struct hdmitx_vidpara *video_param = NULL;
	int  i;
	int count = ARRAY_SIZE(hdmi_tx_video_params);

	for (i = 0; i < count; i++) {
		if (videocode == hdmi_tx_video_params[i].VIC)
			break;
	}
	if (i < count)
		video_param = &hdmi_tx_video_params[i];
	return video_param;
}

static void hdmi_tx_construct_avi_packet(struct hdmitx_vidpara *video_param,
					 char *AVI_DB)
{
	unsigned char color, bar_info, aspect_ratio, cc, ss, sc, ec = 0;

	ss = video_param->ss;
	bar_info = video_param->bar_info;
	if (video_param->color == HDMI_COLORSPACE_YUV444)
		color = 2;
	else if (video_param->color == HDMI_COLORSPACE_YUV422)
		color = 1;
	else
		color = 0;
	AVI_DB[0] = (ss) | (bar_info << 2) | (1 << 4) | (color << 5);

	aspect_ratio = video_param->aspect_ratio;
	cc = video_param->cc;
	/*HDMI CT 7-24*/
	AVI_DB[1] = 8 | (aspect_ratio << 4) | (cc << 6);

	sc = video_param->sc;
	if (video_param->cc == CC_ITU601)
		ec = 0;
	if (video_param->cc == CC_ITU709)
		/*according to CEA-861-D, all other values are reserved*/
		ec = 1;
	AVI_DB[2] = (sc) | (ec << 4);

	AVI_DB[3] = video_param->VIC;
	if (video_param->VIC == HDMI_95_3840x2160p30_16x9 ||
	    video_param->VIC == HDMI_94_3840x2160p25_16x9 ||
	    video_param->VIC == HDMI_93_3840x2160p24_16x9 ||
	    video_param->VIC == HDMI_98_4096x2160p24_256x135)
		/*HDMI Spec V1.4b P151*/
		AVI_DB[3] = 0;

	AVI_DB[4] = video_param->repeat_time;
}

/************************************
 *	hdmitx protocol level interface
 *************************************/

/*
 * HDMI Identifier = HDMI_IEEE_OUI 0x000c03
 * If not, treated as a DVI Device
 */
static int is_dvi_device(struct rx_cap *prxcap)
{
	if (prxcap->ieeeoui != HDMI_IEEE_OUI)
		return 1;
	else
		return 0;
}

int hdmitx_set_display(struct hdmitx_dev *hdev, enum hdmi_vic videocode)
{
	struct hdmi_format_para *para = &hdev->tx_comm.fmt_para;
	struct hdmitx_vidpara *param = NULL;
	struct hdmitx_hw_common *tx_hw_base = &hdev->tx_hw.base;
	int i, ret = -1;
	unsigned char AVI_DB[32];
	unsigned char AVI_HB[32];

	AVI_HB[0] = TYPE_AVI_INFOFRAMES;
	AVI_HB[1] = AVI_INFOFRAMES_VERSION;
	AVI_HB[2] = AVI_INFOFRAMES_LENGTH;
	for (i = 0; i < 32; i++)
		AVI_DB[i] = 0;

	if (hdev->vend_id_hit)
		pr_info(VID "special tv detected\n");

	pr_info(VID "%s set VIC = %d\n", __func__, videocode);

	param = hdmi_get_video_param(videocode);
	hdev->cur_video_param = param;

	if (param) {
		/*cs/cd already validate before enter here, just set.*/
		param->color = para->cs;

		if (videocode >= HDMITX_VESA_OFFSET && para->cs != HDMI_COLORSPACE_RGB) {
			pr_err("hdmitx: VESA only support RGB format\n");
			para->cs = HDMI_COLORSPACE_RGB;
			para->cd = COLORDEPTH_24B;
		}

		if (hdev->hwop.setdispmode(hdev) >= 0) {
			/* HDMI CT 7-33 DVI Sink, no HDMI VSDB nor any
			 * other VSDB, No GB or DI expected
			 * TMDS_MODE[hdmi_config]
			 * 0: DVI Mode	   1: HDMI Mode
			 */
			if (is_dvi_device(&hdev->tx_comm.rxcap)) {
				pr_info(VID "Sink is DVI device\n");
				hdmitx_hw_cntl_config(tx_hw_base,
					CONF_HDMI_DVI_MODE, DVI_MODE);
			} else {
				pr_info(VID "Sink is HDMI device\n");
				hdmitx_hw_cntl_config(tx_hw_base,
					CONF_HDMI_DVI_MODE, HDMI_MODE);
			}
			hdmi_tx_construct_avi_packet(param, (char *)AVI_DB);

			if (videocode == HDMI_95_3840x2160p30_16x9 ||
			    videocode == HDMI_94_3840x2160p25_16x9 ||
			    videocode == HDMI_93_3840x2160p24_16x9 ||
			    videocode == HDMI_98_4096x2160p24_256x135)
				hdmi_set_vend_spec_infofram(hdev, videocode);
			else if ((!hdev->flag_3dfp) && (!hdev->flag_3dtb) &&
				 (!hdev->flag_3dss))
				hdmi_set_vend_spec_infofram(hdev, 0);
			else
				;

			if (hdev->tx_comm.allm_mode) {
				hdmitx_common_setup_vsif_packet(&hdev->tx_comm, VT_ALLM, 1, NULL);
				hdmitx_hw_cntl_config(tx_hw_base, CONF_CT_MODE,
					SET_CT_OFF);
			}
			hdmitx_hw_cntl_config(tx_hw_base, CONF_CT_MODE,
				hdev->tx_comm.ct_mode);
			ret = 0;
		}
	}
	hdmitx_set_spd_info(hdev);

	return ret;
}

static void hdmi_set_vend_spec_infofram(struct hdmitx_dev *hdev,
					enum hdmi_vic videocode)
{
	int i;
	unsigned char VEN_DB[6];
	unsigned char VEN_HB[3];
	struct hdmitx_hw_common *tx_hw_base = &hdev->tx_hw.base;

	VEN_HB[0] = 0x81;
	VEN_HB[1] = 0x01;
	VEN_HB[2] = 0x5;

	if (videocode == 0) {	   /* For non-4kx2k mode setting */
		hdmitx_hw_set_packet(tx_hw_base, HDMI_PACKET_VEND, NULL, VEN_HB);
		return;
	}

	if (hdev->tx_comm.rxcap.dv_info.block_flag == CORRECT ||
	    hdev->dv_src_feature == 1) {	   /* For dolby */
		return;
	}

	for (i = 0; i < 0x6; i++)
		VEN_DB[i] = 0;
	VEN_DB[0] = GET_OUI_BYTE0(HDMI_IEEE_OUI);
	VEN_DB[1] = GET_OUI_BYTE1(HDMI_IEEE_OUI);
	VEN_DB[2] = GET_OUI_BYTE2(HDMI_IEEE_OUI);
	VEN_DB[3] = 0x00;    /* 4k x 2k  Spec P156 */

	if (videocode == HDMI_95_3840x2160p30_16x9) {
		VEN_DB[3] = 0x20;
		VEN_DB[4] = 0x1;
	} else if (videocode == HDMI_94_3840x2160p25_16x9) {
		VEN_DB[3] = 0x20;
		VEN_DB[4] = 0x2;
	} else if (videocode == HDMI_93_3840x2160p24_16x9) {
		VEN_DB[3] = 0x20;
		VEN_DB[4] = 0x3;
	} else if (videocode == HDMI_98_4096x2160p24_256x135) {
		VEN_DB[3] = 0x20;
		VEN_DB[4] = 0x4;
	} else {
		;
	}
	hdmitx_hw_set_packet(tx_hw_base, HDMI_PACKET_VEND, VEN_DB, VEN_HB);
}

int hdmi_set_3d(struct hdmitx_dev *hdev, int type, unsigned int param)
{
	int i;
	unsigned char VEN_DB[6];
	unsigned char VEN_HB[3];
	struct hdmitx_hw_common *tx_hw_base = &hdev->tx_hw.base;

	VEN_HB[0] = 0x81;
	VEN_HB[1] = 0x01;
	VEN_HB[2] = 0x6;
	if (type == T3D_DISABLE) {
		hdmitx_hw_set_packet(tx_hw_base, HDMI_PACKET_VEND, NULL, VEN_HB);
	} else {
		for (i = 0; i < 0x6; i++)
			VEN_DB[i] = 0;
		VEN_DB[0] = GET_OUI_BYTE0(HDMI_IEEE_OUI);
		VEN_DB[1] = GET_OUI_BYTE1(HDMI_IEEE_OUI);
		VEN_DB[2] = GET_OUI_BYTE2(HDMI_IEEE_OUI);
		VEN_DB[3] = 0x40;
		VEN_DB[4] = type << 4;
		VEN_DB[5] = param << 4;
		hdmitx_hw_set_packet(tx_hw_base, HDMI_PACKET_VEND, VEN_DB, VEN_HB);
	}
	return 0;
}

/* Set Source Product Descriptor InfoFrame
 */
static void hdmitx_set_spd_info(struct hdmitx_dev *hdev)
{
	unsigned char SPD_DB[25] = {0x00};
	unsigned char SPD_HB[3] = {0x83, 0x1, 0x19};
	unsigned int len = 0;
	struct vendor_info_data *vend_data;
	struct hdmitx_hw_common *tx_hw_base = &hdev->tx_hw.base;

	if (hdev->config_data.vend_data) {
		vend_data = hdev->config_data.vend_data;
	} else {
		pr_info(VID "packet: can\'t get vendor data\n");
		return;
	}
	if (vend_data->vendor_name) {
		len = strlen(vend_data->vendor_name);
		strncpy(&SPD_DB[0], vend_data->vendor_name,
			(len > 8) ? 8 : len);
	}
	if (vend_data->product_desc) {
		len = strlen(vend_data->product_desc);
		strncpy(&SPD_DB[8], vend_data->product_desc,
			(len > 16) ? 16 : len);
	}
	SPD_DB[24] = 0x1;
	hdmitx_hw_set_packet(tx_hw_base, HDMI_SOURCE_DESCRIPTION, SPD_DB, SPD_HB);
}

