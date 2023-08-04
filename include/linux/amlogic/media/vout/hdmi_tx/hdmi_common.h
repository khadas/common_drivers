/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __HDMI_COMMON_H__
#define __HDMI_COMMON_H__

#include <linux/amlogic/media/vout/vinfo.h>
#include <linux/amlogic/media/vout/hdmi_tx_ext.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_common.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_edid.h>

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

/* Compliance with tx21 definitions */
#define HDMI_UNKNOWN			HDMI_0_UNKNOWN
#define HDMI_640x480p60_4x3		HDMI_1_640x480p60_4x3
#define HDMI_720x480p60_4x3		HDMI_2_720x480p60_4x3
#define HDMI_720x480p60_16x9	HDMI_3_720x480p60_16x9
#define HDMI_1280x720p60_16x9	HDMI_4_1280x720p60_16x9
#define HDMI_1920x1080i60_16x9	HDMI_5_1920x1080i60_16x9
#define HDMI_720x480i60_4x3		HDMI_6_720x480i60_4x3
#define HDMI_720x480i60_16x9	HDMI_7_720x480i60_16x9
#define HDMI_720x240p60_4x3		HDMI_8_720x240p60_4x3
#define HDMI_720x240p60_16x9	HDMI_9_720x240p60_16x9
#define HDMI_2880x480i60_4x3	HDMI_10_2880x480i60_4x3
#define HDMI_2880x480i60_16x9	HDMI_11_2880x480i60_16x9
#define HDMI_2880x240p60_4x3	HDMI_12_2880x240p60_4x3
#define HDMI_2880x240p60_16x9	HDMI_13_2880x240p60_16x9
#define HDMI_1440x480p60_4x3	HDMI_14_1440x480p60_4x3
#define HDMI_1440x480p60_16x9	HDMI_15_1440x480p60_16x9
#define HDMI_1920x1080p60_16x9	HDMI_16_1920x1080p60_16x9
#define HDMI_720x576p50_4x3		HDMI_17_720x576p50_4x3
#define HDMI_720x576p50_16x9	HDMI_18_720x576p50_16x9
#define HDMI_1280x720p50_16x9	HDMI_19_1280x720p50_16x9
#define HDMI_1920x1080i50_16x9	HDMI_20_1920x1080i50_16x9
#define HDMI_720x576i50_4x3		HDMI_21_720x576i50_4x3
#define HDMI_720x576i50_16x9	HDMI_22_720x576i50_16x9
#define HDMI_720x288p_4x3		HDMI_23_720x288p_4x3
#define HDMI_720x288p_16x9		HDMI_24_720x288p_16x9
#define HDMI_2880x576i50_4x3	HDMI_25_2880x576i50_4x3
#define HDMI_2880x576i50_16x9	HDMI_26_2880x576i50_16x9
#define HDMI_2880x288p50_4x3	HDMI_27_2880x288p50_4x3
#define HDMI_2880x288p50_16x9	HDMI_28_2880x288p50_16x9
#define HDMI_1440x576p_4x3		HDMI_29_1440x576p_4x3
#define HDMI_1440x576p_16x9		HDMI_30_1440x576p_16x9
#define HDMI_1920x1080p50_16x9	HDMI_31_1920x1080p50_16x9
#define HDMI_1920x1080p24_16x9	HDMI_32_1920x1080p24_16x9
#define HDMI_1920x1080p25_16x9	HDMI_33_1920x1080p25_16x9
#define HDMI_1920x1080p30_16x9	HDMI_34_1920x1080p30_16x9
#define HDMI_2880x480p60_4x3	HDMI_35_2880x480p60_4x3
#define HDMI_2880x480p60_16x9	HDMI_36_2880x480p60_16x9
#define HDMI_2880x576p50_4x3	HDMI_37_2880x576p50_4x3
#define HDMI_2880x576p50_16x9	HDMI_38_2880x576p50_16x9
#define HDMI_1920x1080i_t1250_50_16x9 HDMI_39_1920x1080i_t1250_50_16x9
#define HDMI_1920x1080i100_16x9	HDMI_40_1920x1080i100_16x9
#define HDMI_1280x720p100_16x9	HDMI_41_1280x720p100_16x9
#define HDMI_720x576p100_4x3	HDMI_42_720x576p100_4x3
#define HDMI_720x576p100_16x9	HDMI_43_720x576p100_16x9
#define HDMI_720x576i100_4x3	HDMI_44_720x576i100_4x3
#define HDMI_720x576i100_16x9	HDMI_45_720x576i100_16x9
#define HDMI_1920x1080i120_16x9	HDMI_46_1920x1080i120_16x9
#define HDMI_1280x720p120_16x9	HDMI_47_1280x720p120_16x9
#define HDMI_720x480p120_4x3	HDMI_48_720x480p120_4x3
#define HDMI_720x480p120_16x9	HDMI_49_720x480p120_16x9
#define HDMI_720x480i120_4x3	HDMI_50_720x480i120_4x3
#define HDMI_720x480i120_16x9	HDMI_51_720x480i120_16x9
#define HDMI_720x576p200_4x3	HDMI_52_720x576p200_4x3
#define HDMI_720x576p200_16x9	HDMI_53_720x576p200_16x9
#define HDMI_720x576i200_4x3	HDMI_54_720x576i200_4x3
#define HDMI_720x576i200_16x9	HDMI_55_720x576i200_16x9
#define HDMI_720x480p240_4x3	HDMI_56_720x480p240_4x3
#define HDMI_720x480p240_16x9	HDMI_57_720x480p240_16x9
#define HDMI_720x480i240_4x3	HDMI_58_720x480i240_4x3
#define HDMI_720x480i240_16x9	HDMI_59_720x480i240_16x9
#define HDMI_1280x720p24_16x9	HDMI_60_1280x720p24_16x9
#define HDMI_1280x720p25_16x9	HDMI_61_1280x720p25_16x9
#define HDMI_1280x720p30_16x9	HDMI_62_1280x720p30_16x9
#define HDMI_1920x1080p120_16x9	HDMI_63_1920x1080p120_16x9
#define HDMI_1920x1080p100_16x9	HDMI_64_1920x1080p100_16x9
#define HDMI_1280x720p24_64x27	HDMI_65_1280x720p24_64x27
#define HDMI_1280x720p25_64x27	HDMI_66_1280x720p25_64x27
#define HDMI_1280x720p30_64x27	HDMI_67_1280x720p30_64x27
#define HDMI_1280x720p50_64x27	HDMI_68_1280x720p50_64x27
#define HDMI_1280x720p60_64x27	HDMI_69_1280x720p60_64x27
#define HDMI_1280x720p100_64x27	HDMI_70_1280x720p100_64x27
#define HDMI_1280x720p120_64x27	HDMI_71_1280x720p120_64x27
#define HDMI_1920x1080p24_64x27	HDMI_72_1920x1080p24_64x27
#define HDMI_1920x1080p25_64x27	HDMI_73_1920x1080p25_64x27
#define HDMI_1920x1080p30_64x27	HDMI_74_1920x1080p30_64x27
#define HDMI_1920x1080p50_64x27	HDMI_75_1920x1080p50_64x27
#define HDMI_1920x1080p60_64x27	HDMI_76_1920x1080p60_64x27
#define HDMI_1920x1080p100_64x27	HDMI_77_1920x1080p100_64x27
#define HDMI_1920x1080p120_64x27	HDMI_78_1920x1080p120_64x27
#define HDMI_1680x720p24_64x27	HDMI_79_1680x720p24_64x27
#define HDMI_1680x720p25_64x27	HDMI_80_1680x720p25_64x27
#define HDMI_1680x720p30_64x27	HDMI_81_1680x720p30_64x27
#define HDMI_1680x720p50_64x27	HDMI_82_1680x720p50_64x27
#define HDMI_1680x720p60_64x27	HDMI_83_1680x720p60_64x27
#define HDMI_1680x720p100_64x27	HDMI_84_1680x720p100_64x27
#define HDMI_1680x720p120_64x27	HDMI_85_1680x720p120_64x27
#define HDMI_2560x1080p24_64x27	HDMI_86_2560x1080p24_64x27
#define HDMI_2560x1080p25_64x27	HDMI_87_2560x1080p25_64x27
#define HDMI_2560x1080p30_64x27	HDMI_88_2560x1080p30_64x27
#define HDMI_2560x1080p50_64x27	HDMI_89_2560x1080p50_64x27
#define HDMI_2560x1080p60_64x27	HDMI_90_2560x1080p60_64x27
#define HDMI_2560x1080p100_64x27	HDMI_91_2560x1080p100_64x27
#define HDMI_2560x1080p120_64x27	HDMI_92_2560x1080p120_64x27
#define HDMI_3840x2160p24_16x9	HDMI_93_3840x2160p24_16x9
#define HDMI_3840x2160p25_16x9	HDMI_94_3840x2160p25_16x9
#define HDMI_3840x2160p30_16x9	HDMI_95_3840x2160p30_16x9
#define HDMI_3840x2160p50_16x9	HDMI_96_3840x2160p50_16x9
#define HDMI_3840x2160p60_16x9	HDMI_97_3840x2160p60_16x9
#define HDMI_4096x2160p24_256x135	HDMI_98_4096x2160p24_256x135
#define HDMI_4096x2160p25_256x135	HDMI_99_4096x2160p25_256x135
#define HDMI_4096x2160p30_256x135	HDMI_100_4096x2160p30_256x135
#define HDMI_4096x2160p50_256x135	HDMI_101_4096x2160p50_256x135
#define HDMI_4096x2160p60_256x135	HDMI_102_4096x2160p60_256x135
#define HDMI_3840x2160p24_64x27	HDMI_103_3840x2160p24_64x27
#define HDMI_3840x2160p25_64x27	HDMI_104_3840x2160p25_64x27
#define HDMI_3840x2160p30_64x27	HDMI_105_3840x2160p30_64x27
#define HDMI_3840x2160p50_64x27	HDMI_106_3840x2160p50_64x27
#define HDMI_3840x2160p60_64x27	HDMI_107_3840x2160p60_64x27

/*keep for hw verification*/
#define HDMI_3840x1080p120hz	HDMI_109_1280x720p48_64x27
#define HDMI_3840x1080p100hz	HDMI_110_1680x720p48_64x27
#define HDMI_3840x540p240hz		HDMI_111_1920x1080p48_16x9
#define HDMI_3840x540p200hz		HDMI_112_1920x1080p48_64x27

/* Compliance with old definitions */
#define HDMI_640x480p60         HDMI_640x480p60_4x3
#define HDMI_480p60             HDMI_720x480p60_4x3
#define HDMI_480p60_16x9        HDMI_720x480p60_16x9
#define HDMI_720p60             HDMI_1280x720p60_16x9
#define HDMI_1080i60            HDMI_1920x1080i60_16x9
#define HDMI_480i60             HDMI_720x480i60_4x3
#define HDMI_480i60_16x9        HDMI_720x480i60_16x9
#define HDMI_480i60_16x9_rpt    HDMI_2880x480i60_16x9
#define HDMI_1440x480p60        HDMI_1440x480p60_4x3
#define HDMI_1080p60            HDMI_1920x1080p60_16x9
#define HDMI_576p50             HDMI_720x576p50_4x3
#define HDMI_576p50_16x9        HDMI_720x576p50_16x9
#define HDMI_720p50             HDMI_1280x720p50_16x9
#define HDMI_1080i50            HDMI_1920x1080i50_16x9
#define HDMI_576i50             HDMI_720x576i50_4x3
#define HDMI_576i50_16x9        HDMI_720x576i50_16x9
#define HDMI_576i50_16x9_rpt    HDMI_2880x576i50_16x9
#define HDMI_1080p50            HDMI_1920x1080p50_16x9
#define HDMI_1080p24            HDMI_1920x1080p24_16x9
#define HDMI_1080p25            HDMI_1920x1080p25_16x9
#define HDMI_1080p30            HDMI_1920x1080p30_16x9
#define HDMI_1080p120           HDMI_1920x1080p120_16x9
#define HDMI_480p60_16x9_rpt    HDMI_2880x480p60_16x9
#define HDMI_576p50_16x9_rpt    HDMI_2880x576p50_16x9

enum hdmi_phy_para {
	HDMI_PHYPARA_6G = 1, /* 2160p60hz 444 8bit */
	HDMI_PHYPARA_4p5G, /* 2160p50hz 420 12bit */
	HDMI_PHYPARA_3p7G, /* 2160p30hz 444 10bit */
	HDMI_PHYPARA_3G, /* 2160p24hz 444 8bit */
	HDMI_PHYPARA_LT3G, /* 1080p60hz 444 12bit */
	HDMI_PHYPARA_DEF = HDMI_PHYPARA_LT3G,
	HDMI_PHYPARA_270M, /* 480p60hz 444 8bit */
};

enum hdmi_audio_fs;
struct dtd;

#define hdmi_cea_timing hdmi_timing

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

/* get hdmi cea timing */
/* t: struct hdmi_cea_timing * */
#define GET_TIMING(name)      (t->(name))

/* HDMI Packet Type Definitions */
#define PT_NULL_PKT                 0x00
#define PT_AUD_CLK_REGENERATION     0x01
#define PT_AUD_SAMPLE               0x02
#define PT_GENERAL_CONTROL          0x03
#define PT_ACP                      0x04
#define PT_ISRC1                    0x05
#define PT_ISRC2                    0x06
#define PT_ONE_BIT_AUD_SAMPLE       0x07
#define PT_DST_AUD                  0x08
#define PT_HBR_AUD_STREAM           0x09
#define PT_GAMUT_METADATA           0x0A
#define PT_3D_AUD_SAMPLE            0x0B
#define PT_ONE_BIT_3D_AUD_SAMPLE    0x0C
#define PT_AUD_METADATA             0x0D
#define PT_MULTI_SREAM_AUD_SAMPLE   0x0E
#define PT_ONE_BIT_MULTI_SREAM_AUD_SAMPLE   0x0F
/* Infoframe Packet */
#define PT_IF_VENDOR_SPECIFIC       0x81
#define PT_IF_AVI                   0x82
#define PT_IF_SPD                   0x83
#define PT_IF_AUD                   0x84
#define PT_IF_MPEG_SOURCE           0x85

/* Old definitions */
#define TYPE_AVI_INFOFRAMES       0x82
#define AVI_INFOFRAMES_VERSION    0x02
#define AVI_INFOFRAMES_LENGTH     0x0D

struct hdmi_csc_coef_table {
	unsigned char input_format;
	unsigned char output_format;
	unsigned char color_depth;
	unsigned char color_format; /* 0 for ITU601, 1 for ITU709 */
	unsigned char coef_length;
	const unsigned char *coef;
};

enum hdmi_audio_packet {
	hdmi_audio_packet_SMP = 0x02,
	hdmi_audio_packet_1BT = 0x07,
	hdmi_audio_packet_DST = 0x08,
	hdmi_audio_packet_HBR = 0x09,
};

enum hdmi_aspect_ratio {
	ASPECT_RATIO_SAME_AS_SOURCE = 0x8,
	TV_ASPECT_RATIO_4_3 = 0x9,
	TV_ASPECT_RATIO_16_9 = 0xA,
	TV_ASPECT_RATIO_14_9 = 0xB,
	TV_ASPECT_RATIO_MAX
};

struct vesa_standard_timing;

unsigned int hdmi_get_csc_coef(unsigned int input_format,
			       unsigned int output_format,
			       unsigned int color_depth,
			       unsigned int color_format,
			       const unsigned char **coef_array,
			       unsigned int *coef_length);
int hdmi_get_fmt_para(enum hdmi_vic vic,
		char const *attr, struct hdmi_format_para *para);
int hdmitx_construct_format_para_from_timing(const struct hdmi_timing *timing,
	struct hdmi_format_para *para);
const char *hdmi_get_str_cd(struct hdmi_format_para *para);
const char *hdmi_get_str_cs(struct hdmi_format_para *para);
const char *hdmi_get_str_cr(struct hdmi_format_para *para);
unsigned int hdmi_get_aud_n_paras(enum hdmi_audio_fs fs,
				  enum hdmi_color_depth cd,
				  unsigned int tmds_clk);
int hdmitx_format_list_init(void);

struct size_map {
	unsigned int sample_bits;
	enum hdmi_audio_sampsize ss;
};

/* FL-- Front Left */
/* FC --Front Center */
/* FR --Front Right */
/* FLC-- Front Left Center */
/* FRC-- Front RiQhtCenter */
/* RL-- Rear Left */
/* RC --Rear Center */
/* RR-- Rear Right */
/* RLC-- Rear Left Center */
/* RRC --Rear RiQhtCenter */
/* LFE-- Low Frequency Effect */
enum hdmi_speak_location {
	CA_FR_FL = 0,
	CA_LFE_FR_FL,
	CA_FC_FR_FL,
	CA_FC_LFE_FR_FL,

	CA_RC_FR_FL,
	CA_RC_LFE_FR_FL,
	CA_RC_FC_FR_FL,
	CA_RC_FC_LFE_FR_FL,

	CA_RR_RL_FR_FL,
	CA_RR_RL_LFE_FR_FL,
	CA_RR_RL_FC_FR_FL,
	CA_RR_RL_FC_LFE_FR_FL,

	CA_RC_RR_RL_FR_FL,
	CA_RC_RR_RL_LFE_FR_FL,
	CA_RC_RR_RL_FC_FR_FL,
	CA_RC_RR_RL_FC_LFE_FR_FL,

	CA_RRC_RC_RR_RL_FR_FL,
	CA_RRC_RC_RR_RL_LFE_FR_FL,
	CA_RRC_RC_RR_RL_FC_FR_FL,
	CA_RRC_RC_RR_RL_FC_LFE_FR_FL,

	CA_FRC_RLC_FR_FL,
	CA_FRC_RLC_LFE_FR_FL,
	CA_FRC_RLC_FC_FR_FL,
	CA_FRC_RLC_FC_LFE_FR_FL,

	CA_FRC_RLC_RC_FR_FL,
	CA_FRC_RLC_RC_LFE_FR_FL,
	CA_FRC_RLC_RC_FC_FR_FL,
	CA_FRC_RLC_RC_FC_LFE_FR_FL,

	CA_FRC_RLC_RR_RL_FR_FL,
	CA_FRC_RLC_RR_RL_LFE_FR_FL,
	CA_FRC_RLC_RR_RL_FC_FR_FL,
	CA_FRC_RLC_RR_RL_FC_LFE_FR_FL,
};

enum hdmi_audio_downmix {
	LSV_0DB = 0,
	LSV_1DB,
	LSV_2DB,
	LSV_3DB,
	LSV_4DB,
	LSV_5DB,
	LSV_6DB,
	LSV_7DB,
	LSV_8DB,
	LSV_9DB,
	LSV_10DB,
	LSV_11DB,
	LSV_12DB,
	LSV_13DB,
	LSV_14DB,
	LSV_15DB,
};

enum hdmi_rx_audio_state {
	STATE_AUDIO__MUTED = 0,
	STATE_AUDIO__REQUEST_AUDIO = 1,
	STATE_AUDIO__AUDIO_READY = 2,
	STATE_AUDIO__ON = 3,
};

struct rate_map_fs {
	unsigned int rate;
	enum hdmi_audio_fs fs;
};

struct hdmi_rx_audioinfo {
	/* !< Signal decoding type -- TvAudioType */
	enum hdmi_audio_type type;
	enum hdmi_audio_format format;
	/* !< active audio channels bit mask. */
	enum hdmi_audio_chnnum channels;
	enum hdmi_audio_fs fs; /* !< Signal sample rate in Hz */
	enum hdmi_audio_sampsize ss;
	enum hdmi_speak_location speak_loc;
	enum hdmi_audio_downmix lsv;
	unsigned int N_value;
	unsigned int CTS;
};

#define AUDIO_PARA_MAX_NUM       14
struct hdmi_audio_fs_ncts {
	struct {
		unsigned int tmds_clk;
		unsigned int n; /* 24 or 30 bit */
		unsigned int cts; /* 24 or 30 bit */
		unsigned int n_36bit;
		unsigned int cts_36bit;
		unsigned int n_48bit;
		unsigned int cts_48bit;
	} array[AUDIO_PARA_MAX_NUM];
	unsigned int def_n;
};

#endif
