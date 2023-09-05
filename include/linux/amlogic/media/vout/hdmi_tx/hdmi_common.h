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

/*keep for hw verification*/
#define HDMI_3840x1080p120hz	HDMI_109_1280x720p48_64x27
#define HDMI_3840x1080p100hz	HDMI_110_1680x720p48_64x27
#define HDMI_3840x540p240hz		HDMI_111_1920x1080p48_16x9
#define HDMI_3840x540p200hz		HDMI_112_1920x1080p48_64x27

enum hdmi_audio_fs;

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

unsigned int hdmi_get_aud_n_paras(enum hdmi_audio_fs fs,
				  enum hdmi_color_depth cd,
				  unsigned int tmds_clk);

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
