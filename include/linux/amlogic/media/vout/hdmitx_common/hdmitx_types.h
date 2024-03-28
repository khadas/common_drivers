/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __HDMITX_TYPES_H
#define __HDMITX_TYPES_H

#include <linux/types.h>
#include <linux/hdmi.h>
#include <linux/amlogic/media/vout/hdmi_tx_ext.h>

enum hdmi_hdr_transfer {
	T_UNKNOWN = 0,
	T_BT709,
	T_UNDEF,
	T_BT601,
	T_BT470M,
	T_BT470BG,
	T_SMPTE170M,
	T_SMPTE240M,
	T_LINEAR,
	T_LOG100,
	T_LOG316,
	T_IEC61966_2_4,
	T_BT1361E,
	T_IEC61966_2_1,
	T_BT2020_10,
	T_BT2020_12,
	T_SMPTE_ST_2084,
	T_SMPTE_ST_28,
	T_HLG,
};

enum hdmi_hdr_color {
	C_UNKNOWN = 0,
	C_BT709,
	C_UNDEF,
	C_BT601,
	C_BT470M,
	C_BT470BG,
	C_SMPTE170M,
	C_SMPTE240M,
	C_FILM,
	C_BT2020,
};

enum hdmi_tf_type {
	HDMI_NONE = 0,
	/* HDMI_HDR_TYPE, HDMI_DV_TYPE, and HDMI_HDR10P_TYPE
	 * should be mutexed with each other
	 */
	HDMI_HDR_TYPE = 0x10,
	HDMI_HDR_SMPTE_2084	= HDMI_HDR_TYPE | 1,
	HDMI_HDR_HLG		= HDMI_HDR_TYPE | 2,
	HDMI_HDR_HDR		= HDMI_HDR_TYPE | 3,
	HDMI_HDR_SDR		= HDMI_HDR_TYPE | 4,
	HDMI_DV_TYPE = 0x20,
	HDMI_DV_VSIF_STD	= HDMI_DV_TYPE | 1,
	HDMI_DV_VSIF_LL		= HDMI_DV_TYPE | 2,
	HDMI_HDR10P_TYPE = 0x30,
	HDMI_HDR10P_DV_VSIF	= HDMI_HDR10P_TYPE | 1,
};

enum hdmi_hdr_status {
	HDR10PLUS_VSIF = 0,
	dolbyvision_std = 1,
	dolbyvision_lowlatency = 2,
	HDR10_GAMMA_ST2084 = 3,
	HDR10_others,
	HDR10_GAMMA_HLG,
	SDR,
};

enum hdmi_3d_type {
	T3D_FRAME_PACKING = 0,
	T3D_FIELD_ALTER = 1,
	T3D_LINE_ALTER = 2,
	T3D_SBS_FULL = 3,
	T3D_L_DEPTH = 4,
	T3D_L_DEPTH_GRAPHICS = 5,
	T3D_TAB = 6, /* Top and Buttom */
	T3D_RSVD = 7,
	T3D_SBS_HALF = 8,
	T3D_DISABLE,
};

enum hdmi_aspect_ratio {
	ASPECT_RATIO_SAME_AS_SOURCE = 0x8,
	TV_ASPECT_RATIO_4_3 = 0x9,
	TV_ASPECT_RATIO_16_9 = 0xA,
	TV_ASPECT_RATIO_14_9 = 0xB,
	TV_ASPECT_RATIO_MAX
};

#define HDMI_INFOFRAME_TYPE_VENDOR2 (0x81 | 0x100)

enum frl_rate_enum {
	FRL_NONE = 0,
	FRL_3G3L = 1,
	FRL_6G3L = 2,
	FRL_6G4L = 3,
	FRL_8G4L = 4,
	FRL_10G4L = 5,
	FRL_12G4L = 6,
	FRL_RATE_MAX = 7,
};

enum hdmi_phy_para {
	HDMI_PHYPARA_6G = 1, /* 2160p60hz 444 8bit */
	HDMI_PHYPARA_4p5G, /* 2160p50hz 420 12bit */
	HDMI_PHYPARA_3p7G, /* 2160p30hz 444 10bit */
	HDMI_PHYPARA_3G, /* 2160p24hz 444 8bit */
	HDMI_PHYPARA_LT3G, /* 1080p60hz 444 12bit */
	HDMI_PHYPARA_DEF = HDMI_PHYPARA_LT3G,
	HDMI_PHYPARA_270M, /* 480p60hz 444 8bit */
};

enum vrr_type {
	T_VRR_NONE,
	T_VRR_GAME,
	T_VRR_QMS,
};

enum emp_type {
	EMP_TYPE_NONE,
	EMP_TYPE_VRR_GAME = T_VRR_GAME,
	EMP_TYPE_VRR_QMS = T_VRR_QMS,
	EMP_TYPE_SBTM,
	EMP_TYPE_DSC,
	EMP_TYPE_DHDR,
};

/* refer to HDMI2.1A P447 */
enum TARGET_FRAME_RATE {
	TFR_QMSVRR_INACTIVE = 0,
	TFR_23P97,
	TFR_24,
	TFR_25,
	TFR_29P97,
	TFR_30,
	TFR_47P95,
	TFR_48,
	TFR_50,
	TFR_59P94,
	TFR_60,
	TFR_100,
	TFR_119P88,
	TFR_120,
	TFR_MAX,
};

struct size_map {
	unsigned int sample_bits;
	enum hdmi_audio_sampsize ss;
};

enum hd_ctrl {
	VID_EN, VID_DIS, AUD_EN, AUD_DIS, EDID_EN, EDID_DIS, HDCP_EN, HDCP_DIS,
};

enum hdmitx_aspect_ratio {
	AR_UNKNOWN = 0,
	AR_4X3,
	AR_16X9,
};

#define HDMI_PACKET_TYPE_GCP 0x3

struct hdmitx_infoframe {
	u32 enable;
	union hdmi_infoframe vend;
	union hdmi_infoframe avi;
	union hdmi_infoframe spd;
	union hdmi_infoframe aud;
	union hdmi_infoframe drm;
	union hdmi_infoframe emp;
};

enum hdmi_pixel_repeat {
	NO_REPEAT = 0,
	HDMI_2_TIMES_REPEAT,
	HDMI_3_TIMES_REPEAT,
	HDMI_4_TIMES_REPEAT,
	HDMI_5_TIMES_REPEAT,
	HDMI_6_TIMES_REPEAT,
	HDMI_7_TIMES_REPEAT,
	HDMI_8_TIMES_REPEAT,
	HDMI_9_TIMES_REPEAT,
	HDMI_10_TIMES_REPEAT,
	MAX_TIMES_REPEAT,
};

enum hdmi_scan {
	SS_NO_DATA = 0,
	/* where some active pixelsand lines at the edges are not displayed. */
	SS_SCAN_OVER,
	/* where all active pixels&lines are displayed,
	 * with or without a border.
	 */
	SS_SCAN_UNDER,
	SS_RSV
};

enum hdmi_barinfo {
	B_INVALID = 0, B_BAR_VERT, /* Vert. Bar Infovalid */
	B_BAR_HORIZ, /* Horiz. Bar Infovalid */
	B_BAR_VERT_HORIZ,
/* Vert.and Horiz. Bar Info valid */
};

enum hdmi_colourimetry {
	CC_NO_DATA = 0, CC_ITU601, CC_ITU709, CC_XVYCC601, CC_XVYCC709,
};

enum hdmi_scaling {
	SC_NO_UINFORM = 0,
	/* Picture has been scaled horizontally */
	SC_SCALE_HORIZ,
	SC_SCALE_VERT, /* Picture has been scaled vertically */
	SC_SCALE_HORIZ_VERT,
/* Picture has been scaled horizontally & SC_SCALE_H_V */
};

#define AUDIO_PARA_MAX_NUM       14
struct hdmi_audio_fs_ncts {
	struct {
		u32 tmds_clk;
		unsigned int n; /* 24 bit */
		unsigned int cts; /* 24 bit */
		unsigned int n_30bit; /* 30 bit */
		unsigned int cts_30bit; /* 30bit */
		u32 n_36bit;
		u32 cts_36bit;
		u32 n_48bit;
		u32 cts_48bit;
	} array[AUDIO_PARA_MAX_NUM];
	u32 def_n;
};

struct rate_map_fs {
	unsigned int rate;
	enum hdmi_audio_fs fs;
};

enum hdmi_event_t {
	HDMI_TX_NONE = 0,
	HDMI_TX_HPD_PLUGIN = 1,
	HDMI_TX_HPD_PLUGOUT = 2,
	HDMI_TX_INTERNAL_INTR = 4,
};

/***********************************************************************
 *                   hdmi debug printk
 **********************************************************************/
#define VID         "video: "
#define AUD         "audio: "
#define CEC         "cec: "
#define EDID        "edid: "
#define HDCP        "hdcp: "
#define SYS         "system: "
#define HPD         "hpd: "
#define HW          "hw: "
#define REG         "reg: "

#endif
