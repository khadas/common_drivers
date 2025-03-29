/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __HDMITX_MODE_H_
#define __HDMITX_MODE_H_

#include <linux/types.h>
#include <linux/hdmi.h>
#include "hdmitx_types.h"

/* half for valid vic, half for vic with y420*/
#define VIC_MAX_NUM 512
#define SVD_VIC_MAX_NUM 128
#define VESA_MAX_TIMING 64
#define Y420_VIC_MAX_NUM 32 /* vic numbers for y420 */

#define HDMITX_VESA_OFFSET	0x300

enum hdmi_vic {
	/* Refer to CEA 861-D */
	HDMI_0_UNKNOWN = 0,
	HDMI_1_640x480p60_4x3		= 1,
	HDMI_2_720x480p60_4x3		= 2,
	HDMI_3_720x480p60_16x9		= 3,
	HDMI_4_1280x720p60_16x9		= 4,
	HDMI_5_1920x1080i60_16x9	= 5,
	HDMI_6_720x480i60_4x3		= 6,
	HDMI_7_720x480i60_16x9		= 7,
	HDMI_8_720x240p60_4x3		= 8,
	HDMI_9_720x240p60_16x9		= 9,
	HDMI_10_2880x480i60_4x3		= 10,
	HDMI_11_2880x480i60_16x9	= 11,
	HDMI_12_2880x240p60_4x3		= 12,
	HDMI_13_2880x240p60_16x9	= 13,
	HDMI_14_1440x480p60_4x3		= 14,
	HDMI_15_1440x480p60_16x9	= 15,
	HDMI_16_1920x1080p60_16x9	= 16,
	HDMI_17_720x576p50_4x3		= 17,
	HDMI_18_720x576p50_16x9		= 18,
	HDMI_19_1280x720p50_16x9	= 19,
	HDMI_20_1920x1080i50_16x9	= 20,
	HDMI_21_720x576i50_4x3		= 21,
	HDMI_22_720x576i50_16x9		= 22,
	HDMI_23_720x288p_4x3		= 23,
	HDMI_24_720x288p_16x9		= 24,
	HDMI_25_2880x576i50_4x3		= 25,
	HDMI_26_2880x576i50_16x9	= 26,
	HDMI_27_2880x288p50_4x3		= 27,
	HDMI_28_2880x288p50_16x9	= 28,
	HDMI_29_1440x576p_4x3		= 29,
	HDMI_30_1440x576p_16x9		= 30,
	HDMI_31_1920x1080p50_16x9	= 31,
	HDMI_32_1920x1080p24_16x9	= 32,
	HDMI_33_1920x1080p25_16x9	= 33,
	HDMI_34_1920x1080p30_16x9	= 34,
	HDMI_35_2880x480p60_4x3		= 35,
	HDMI_36_2880x480p60_16x9	= 36,
	HDMI_37_2880x576p50_4x3		= 37,
	HDMI_38_2880x576p50_16x9	= 38,
	HDMI_39_1920x1080i_t1250_50_16x9 = 39,
	HDMI_40_1920x1080i100_16x9	= 40,
	HDMI_41_1280x720p100_16x9	= 41,
	HDMI_42_720x576p100_4x3		= 42,
	HDMI_43_720x576p100_16x9	= 43,
	HDMI_44_720x576i100_4x3		= 44,
	HDMI_45_720x576i100_16x9	= 45,
	HDMI_46_1920x1080i120_16x9	= 46,
	HDMI_47_1280x720p120_16x9	= 47,
	HDMI_48_720x480p120_4x3		= 48,
	HDMI_49_720x480p120_16x9	= 49,
	HDMI_50_720x480i120_4x3		= 50,
	HDMI_51_720x480i120_16x9	= 51,
	HDMI_52_720x576p200_4x3		= 52,
	HDMI_53_720x576p200_16x9	= 53,
	HDMI_54_720x576i200_4x3		= 54,
	HDMI_55_720x576i200_16x9	= 55,
	HDMI_56_720x480p240_4x3		= 56,
	HDMI_57_720x480p240_16x9	= 57,
	HDMI_58_720x480i240_4x3		= 58,
	HDMI_59_720x480i240_16x9	= 59,
	HDMI_60_1280x720p24_16x9	= 60,
	HDMI_61_1280x720p25_16x9	= 61,
	HDMI_62_1280x720p30_16x9	= 62,
	HDMI_63_1920x1080p120_16x9	= 63,
	HDMI_64_1920x1080p100_16x9	= 64,
	HDMI_65_1280x720p24_64x27	= 65,
	HDMI_66_1280x720p25_64x27	= 66,
	HDMI_67_1280x720p30_64x27	= 67,
	HDMI_68_1280x720p50_64x27	= 68,
	HDMI_69_1280x720p60_64x27	= 69,
	HDMI_70_1280x720p100_64x27	= 70,
	HDMI_71_1280x720p120_64x27	= 71,
	HDMI_72_1920x1080p24_64x27	= 72,
	HDMI_73_1920x1080p25_64x27	= 73,
	HDMI_74_1920x1080p30_64x27	= 74,
	HDMI_75_1920x1080p50_64x27	= 75,
	HDMI_76_1920x1080p60_64x27	= 76,
	HDMI_77_1920x1080p100_64x27	= 77,
	HDMI_78_1920x1080p120_64x27	= 78,
	HDMI_79_1680x720p24_64x27	= 79,
	HDMI_80_1680x720p25_64x27	= 80,
	HDMI_81_1680x720p30_64x27	= 81,
	HDMI_82_1680x720p50_64x27	= 82,
	HDMI_83_1680x720p60_64x27	= 83,
	HDMI_84_1680x720p100_64x27	= 84,
	HDMI_85_1680x720p120_64x27	= 85,
	HDMI_86_2560x1080p24_64x27	= 86,
	HDMI_87_2560x1080p25_64x27	= 87,
	HDMI_88_2560x1080p30_64x27	= 88,
	HDMI_89_2560x1080p50_64x27	= 89,
	HDMI_90_2560x1080p60_64x27	= 90,
	HDMI_91_2560x1080p100_64x27	= 91,
	HDMI_92_2560x1080p120_64x27	= 92,
	HDMI_93_3840x2160p24_16x9	= 93,
	HDMI_94_3840x2160p25_16x9	= 94,
	HDMI_95_3840x2160p30_16x9	= 95,
	HDMI_96_3840x2160p50_16x9	= 96,
	HDMI_97_3840x2160p60_16x9	= 97,
	HDMI_98_4096x2160p24_256x135	= 98,
	HDMI_99_4096x2160p25_256x135	= 99,
	HDMI_100_4096x2160p30_256x135	= 100,
	HDMI_101_4096x2160p50_256x135	= 101,
	HDMI_102_4096x2160p60_256x135	= 102,
	HDMI_103_3840x2160p24_64x27	= 103,
	HDMI_104_3840x2160p25_64x27	= 104,
	HDMI_105_3840x2160p30_64x27	= 105,
	HDMI_106_3840x2160p50_64x27	= 106,
	HDMI_107_3840x2160p60_64x27	= 107,
	HDMI_108_1280x720p48_16x9	= 108,
	HDMI_109_1280x720p48_64x27	= 109,
	HDMI_110_1680x720p48_64x27	= 110,
	HDMI_111_1920x1080p48_16x9	= 111,
	HDMI_112_1920x1080p48_64x27	= 112,
	HDMI_113_2560x1080p48_64x27	= 113,
	HDMI_114_3840x2160p48_16x9	= 114,
	HDMI_115_4096x2160p48_256x135	= 115,
	HDMI_116_3840x2160p48_64x27	= 116,
	HDMI_117_3840x2160p100_16x9	= 117,
	HDMI_118_3840x2160p120_16x9	= 118,
	HDMI_119_3840x2160p100_64x27	= 119,
	HDMI_120_3840x2160p120_64x27	= 120,
	HDMI_121_5120x2160p24_64x27	= 121,
	HDMI_122_5120x2160p25_64x27	= 122,
	HDMI_123_5120x2160p30_64x27	= 123,
	HDMI_124_5120x2160p48_64x27	= 124,
	HDMI_125_5120x2160p50_64x27	= 125,
	HDMI_126_5120x2160p60_64x27	= 126,
	HDMI_127_5120x2160p100_64x27	= 127,
	/* 128 ~ 192 reserved */
	HDMI_193_5120x2160p120_64x27	= 193,
	HDMI_194_7680x4320p24_16x9	= 194,
	HDMI_195_7680x4320p25_16x9	= 195,
	HDMI_196_7680x4320p30_16x9	= 196,
	HDMI_197_7680x4320p48_16x9	= 197,
	HDMI_198_7680x4320p50_16x9	= 198,
	HDMI_199_7680x4320p60_16x9	= 199,
	HDMI_200_7680x4320p100_16x9	= 200,
	HDMI_201_7680x4320p120_16x9	= 201,
	HDMI_202_7680x4320p24_64x27	= 202,
	HDMI_203_7680x4320p25_64x27	= 203,
	HDMI_204_7680x4320p30_64x27	= 204,
	HDMI_205_7680x4320p48_64x27	= 205,
	HDMI_206_7680x4320p50_64x27	= 206,
	HDMI_207_7680x4320p60_64x27	= 207,
	HDMI_208_7680x4320p100_64x27	= 208,
	HDMI_209_7680x4320p120_64x27	= 209,
	HDMI_210_10240x4320p24_64x27	= 210,
	HDMI_211_10240x4320p25_64x27	= 211,
	HDMI_212_10240x4320p30_64x27	= 212,
	HDMI_213_10240x4320p48_64x27	= 213,
	HDMI_214_10240x4320p50_64x27	= 214,
	HDMI_215_10240x4320p60_64x27	= 215,
	HDMI_216_10240x4320p100_64x27	= 216,
	HDMI_217_10240x4320p120_64x27	= 217,
	HDMI_218_4096x2160p100_256x135	= 218,
	HDMI_219_4096x2160p120_256x135	= 219,
	HDMI_CEA_VIC_END,

	/*FAKE VIC*/
	HDMI_VIC_FAKE = 0X200,

	/*Vesa mode which dont have vic, we specify value for them also*/
	HDMIV_0_640x480p60hz = HDMITX_VESA_OFFSET,
	HDMIV_1_800x480p60hz,
	HDMIV_2_800x600p60hz,
	HDMIV_3_852x480p60hz,
	HDMIV_4_854x480p60hz,
	HDMIV_5_1024x600p60hz,
	HDMIV_6_1024x768p60hz,
	HDMIV_7_1152x864p75hz,
	HDMIV_8_1280x768p60hz,
	HDMIV_9_1280x800p60hz,
	HDMIV_10_1280x960p60hz,
	HDMIV_11_1280x1024p60hz,
	HDMIV_12_1360x768p60hz,
	HDMIV_13_1366x768p60hz,
	HDMIV_14_1400x1050p60hz,
	HDMIV_15_1440x900p60hz,
	HDMIV_16_1440x2560p60hz,
	HDMIV_17_1600x900p60hz,
	HDMIV_18_1600x1200p60hz,
	HDMIV_19_1680x1050p60hz,
	HDMIV_20_1920x1200p60hz,
	HDMIV_21_2048x1080p24hz,
	HDMIV_22_2160x1200p90hz,
	HDMIV_23_2560x1600p60hz,
	HDMIV_24_3440x1440p60hz,
	HDMIV_25_2400x1200p90hz,
	HDMIV_26_3840x1080p60hz,
	HDMIV_27_2560x1440p60hz,
	HDMIV_28_1440x1440p60hz,
	HDMIV_29_2880x1440p60hz,
	/*not supported in timing*/
	HDMIV_2560x1080p60hz,
	HDMIV_1280x600p60hz,
	HDMIV_2560x1600p60hz,
	HDMIV_2400x1200p90hz,

	HDMI_VIC_END = 0xFFFF,
};

/* Compliance with old definitions */
#define HDMIV_640x480p60hz		HDMIV_0_640x480p60hz
#define HDMIV_800x480p60hz		HDMIV_1_800x480p60hz
#define HDMIV_800x600p60hz		HDMIV_2_800x600p60hz
#define HDMIV_852x480p60hz		HDMIV_3_852x480p60hz
#define HDMIV_854x480p60hz		HDMIV_4_854x480p60hz
#define HDMIV_1024x600p60hz		HDMIV_5_1024x600p60hz
#define HDMIV_1024x768p60hz		HDMIV_6_1024x768p60hz
#define HDMIV_1152x864p75hz		HDMIV_7_1152x864p75hz
#define HDMIV_1280x768p60hz		HDMIV_8_1280x768p60hz
#define HDMIV_1280x800p60hz		HDMIV_9_1280x800p60hz
#define HDMIV_1280x960p60hz		HDMIV_10_1280x960p60hz
#define HDMIV_1280x1024p60hz	HDMIV_11_1280x1024p60hz
#define HDMIV_1360x768p60hz		HDMIV_12_1360x768p60hz
#define HDMIV_1366x768p60hz		HDMIV_13_1366x768p60hz
#define HDMIV_1400x1050p60hz	HDMIV_14_1400x1050p60hz
#define HDMIV_1440x900p60hz		HDMIV_15_1440x900p60hz
#define HDMIV_1440x2560p60hz	HDMIV_16_1440x2560p60hz
#define HDMIV_1600x900p60hz		HDMIV_17_1600x900p60hz
#define HDMIV_1600x1200p60hz	HDMIV_18_1600x1200p60hz
#define HDMIV_1680x1050p60hz	HDMIV_19_1680x1050p60hz
#define HDMIV_1920x1200p60hz	HDMIV_20_1920x1200p60hz
#define HDMIV_2048x1080p24hz	HDMIV_21_2048x1080p24hz
#define HDMIV_2160x1200p90hz	HDMIV_22_2160x1200p90hz
#define HDMIV_2560x1600p60hz	HDMIV_23_2560x1600p60hz
#define HDMIV_3440x1440p60hz	HDMIV_24_3440x1440p60hz
#define HDMIV_2400x1200p90hz	HDMIV_25_2400x1200p90hz
#define HDMIV_3840x1080p60hz	HDMIV_26_3840x1080p60hz
#define HDMIV_2560x1440p60hz	HDMIV_27_2560x1440p60hz
#define HDMIV_1440x1440p60hz    HDMIV_28_1440x1440p60hz
#define HDMIV_2880x1440p60hz    HDMIV_29_2880x1440p60hz

/* CEA TIMING STRUCT DEFINITION */
struct hdmi_timing {
	unsigned int vic;
	unsigned char *name;
	unsigned char *sname;

	unsigned short pi_mode; /* 1: progressive  0: interlaced */
	unsigned int h_freq; /* in Hz */
	unsigned int v_freq; /* in 0.001 Hz */
	unsigned int pixel_freq; /* Unit: 1000 */

	unsigned short h_total;
	unsigned short h_blank;
	unsigned short h_front;
	unsigned short h_sync;
	unsigned short h_back;
	unsigned short h_active;
	unsigned short v_total;
	unsigned short v_blank;
	unsigned short v_front;
	unsigned short v_sync;
	unsigned short v_back;
	unsigned short v_active;
	unsigned short v_sync_ln;

	unsigned short h_pol; /*hsync_polarity*/
	unsigned short v_pol; /*vsync_polarity*/
	unsigned short h_pict; /* aspect ratio */
	unsigned short v_pict; /* aspect ratio */
	unsigned short h_pixel;
	unsigned short v_pixel;

	/*Same as DRM_MODE_FLAG_DBLCLK, only for 480i&576i*/
	u32 pixel_repetition_factor:1;
};

/* Refer CEA861-D Page 116 Table 55 */
struct dtd {
	unsigned short pixel_clock;
	unsigned short h_active;
	unsigned short h_blank;
	unsigned short v_active;
	unsigned short v_blank;
	unsigned short h_sync_offset;
	unsigned short h_sync;
	unsigned short v_sync_offset;
	unsigned short v_sync;
	unsigned short h_image_size;
	unsigned short v_image_size;
	unsigned char h_border;
	unsigned char v_border;
	unsigned char flags;

	u32 vic;
};

struct vesa_standard_timing {
	unsigned short hactive;
	unsigned short vactive;
	unsigned short hblank;
	unsigned short vblank;
	unsigned short vsync;
	unsigned short tmds_clk; /* Value = Pixel clock ?? 10,000 */
	enum hdmi_vic vesa_timing;
};

enum hdmi_color_depth {
	COLORDEPTH_24B = 4,
	COLORDEPTH_30B = 5,
	COLORDEPTH_36B = 6,
	COLORDEPTH_48B = 7,
	COLORDEPTH_RESERVED = 11,
};

struct parse_cd {
	enum hdmi_color_depth cd;
	const char *name;
};

struct parse_cs {
	enum hdmi_colorspace cs;
	const char *name;
};

struct parse_cr {
	enum hdmi_quantization_range cr;
	const char *name;
};

/**************************APIs to get search hdmi_timing**************************/
/* always return valid struct hdmi_timing.
 * for invalid vic, return NULL.
 */
const struct hdmi_timing *hdmitx_mode_vic_to_hdmi_timing(enum hdmi_vic vic);
/* always return valid struct hdmi_timing.
 * when failed, return timing with invalid vic(0).
 */
const struct hdmi_timing *hdmitx_mode_match_dtd_timing(struct dtd *t);
/* always return valid struct hdmi_timing.
 * when failed, return timing with invalid vic(0).
 */
const struct hdmi_timing *hdmitx_mode_match_vesa_timing(struct vesa_standard_timing *t);

void hdmitx_mode_print_hdmi_timing(const struct hdmi_timing *timing);
void hdmitx_mode_print_all_mode_table(void);

int hdmi_timing_vrefresh(const struct hdmi_timing *t);

/**
 * This function try to update hdmi_timing to alternate fraction_mode or not.
 * return < 0: means this timing cannot support fraction_mode.
 * return 0: current timing match frac_mode, nothing update.
 * return > 0: current timing have modified to match frac_mode.
 */
int hdmitx_mode_update_timing(struct hdmi_timing *t, bool to_frac_mode);

#endif
