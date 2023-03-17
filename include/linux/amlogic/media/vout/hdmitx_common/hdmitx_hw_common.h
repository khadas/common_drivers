/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __HDMITX_HW_COMMON_H
#define __HDMITX_HW_COMMON_H

#include <linux/types.h>

/*hw cntl cmd define, abstract from hdmi_tx_module.h*/
#define CMD_DDC_OFFSET          (0x10 << 24)
#define CMD_STATUS_OFFSET       (0x11 << 24)
#define CMD_PACKET_OFFSET       (0x12 << 24)
#define CMD_MISC_OFFSET         (0x13 << 24)
#define CMD_CONF_OFFSET         (0x14 << 24)
#define CMD_STAT_OFFSET         (0x15 << 24)

/***********************************************************************
 *             MISC control, hpd, hpll //cntlmisc
 **********************************************************************/
#define MISC_HPD_MUX_OP         (CMD_MISC_OFFSET + 0x00)
#define MISC_HPD_GPI_ST         (CMD_MISC_OFFSET + 0x02)
#define MISC_HPLL_OP            (CMD_MISC_OFFSET + 0x03)
	#define HPLL_ENABLE         0x1
	#define HPLL_DISABLE        0x2
	#define HPLL_SET            0x3

#define MISC_TMDS_PHY_OP        (CMD_MISC_OFFSET + 0x04)
	#define TMDS_PHY_ENABLE     0x1
	#define TMDS_PHY_DISABLE    0x2

#define MISC_VIID_IS_USING      (CMD_MISC_OFFSET + 0x05)
#define MISC_CONF_MODE420       (CMD_MISC_OFFSET + 0x06)
#define MISC_TMDS_CLK_DIV40     (CMD_MISC_OFFSET + 0x07)
#define MISC_COMP_HPLL         (CMD_MISC_OFFSET + 0x08)
#define COMP_HPLL_SET_OPTIMISE_HPLL1    0x1
#define COMP_HPLL_SET_OPTIMISE_HPLL2    0x2
#define MISC_COMP_AUDIO         (CMD_MISC_OFFSET + 0x09)
#define COMP_AUDIO_SET_N_6144x2          0x1
#define COMP_AUDIO_SET_N_6144x3          0x2

#define MISC_AVMUTE_OP          (CMD_MISC_OFFSET + 0x0a)
	#define OFF_AVMUTE      0x0
	#define CLR_AVMUTE      0x1
	#define SET_AVMUTE      0x2

#define MISC_FINE_TUNE_HPLL     (CMD_MISC_OFFSET + 0x0b)
#define MISC_HPLL_FAKE			(CMD_MISC_OFFSET + 0x0c)
#define MISC_ESM_RESET		(CMD_MISC_OFFSET + 0x0d)
#define MISC_HDCP_CLKDIS	(CMD_MISC_OFFSET + 0x0e)
#define MISC_TMDS_RXSENSE	(CMD_MISC_OFFSET + 0x0f)
#define MISC_I2C_REACTIVE       (CMD_MISC_OFFSET + 0x10) /* For gxl */
#define MISC_I2C_RESET		(CMD_MISC_OFFSET + 0x11) /* For g12 */
#define MISC_READ_AVMUTE_OP     (CMD_MISC_OFFSET + 0x12)
#define MISC_TMDS_CEDST		(CMD_MISC_OFFSET + 0x13)
#define MISC_TRIGGER_HPD        (CMD_MISC_OFFSET + 0X14)
#define MISC_SUSFLAG		(CMD_MISC_OFFSET + 0X15)
#define MISC_AUDIO_RESET	(CMD_MISC_OFFSET + 0x16)
#define MISC_DIS_HPLL		(CMD_MISC_OFFSET + 0x17)

/***********************************************************************
 *                          Get State //getstate
 **********************************************************************/
#define STAT_VIDEO_VIC			(CMD_STAT_OFFSET + 0x00)
#define STAT_VIDEO_CLK			(CMD_STAT_OFFSET + 0x01)
#define STAT_AUDIO_FORMAT		(CMD_STAT_OFFSET + 0x10)
#define STAT_AUDIO_CHANNEL		(CMD_STAT_OFFSET + 0x11)
#define STAT_AUDIO_CLK_STABLE	(CMD_STAT_OFFSET + 0x12)
#define STAT_AUDIO_PACK			(CMD_STAT_OFFSET + 0x13)
#define STAT_HDR_TYPE			(CMD_STAT_OFFSET + 0x20)

#define STAT_TX_PHY				(CMD_STAT_OFFSET + 0x30)

/***********************************************************************
 *             CONFIG CONTROL //cntlconfig
 **********************************************************************/
/* Video part */
#define CONF_HDMI_DVI_MODE      (CMD_CONF_OFFSET + 0x02)
	#define HDMI_MODE           0x1
	#define DVI_MODE            0x2

/* set value as COLORSPACE_RGB444, YUV422, YUV444, YUV420 */
#define CONF_VIDEO_MUTE_OP		(CMD_CONF_OFFSET + 0x1000 + 0x04)
	#define VIDEO_NONE_OP		0x0
	#define VIDEO_MUTE			0x1
	#define VIDEO_UNMUTE		0x2
#define CONF_EMP_NUMBER			(CMD_CONF_OFFSET + 0x3000 + 0x00)
#define CONF_EMP_PHY_ADDR		(CMD_CONF_OFFSET + 0x3000 + 0x01)

#define CONFIG_CSC (CMD_CONF_OFFSET + 0x1000 + 0x05)
#define CSC_Y444_8BIT 0x1
#define CSC_Y422_12BIT 0x2
#define CSC_RGB_8BIT 0x3
#define CSC_UPDATE_AVI_CS 0x10

/* Audio part */
#define CONF_CLR_AVI_PACKET		(CMD_CONF_OFFSET + 0x04)
#define CONF_CLR_VSDB_PACKET	(CMD_CONF_OFFSET + 0x05)
#define CONF_VIDEO_MAPPING		(CMD_CONF_OFFSET + 0x06)
#define CONF_GET_HDMI_DVI_MODE	(CMD_CONF_OFFSET + 0x07)
#define CONF_CLR_DV_VS10_SIG	(CMD_CONF_OFFSET + 0x10)

#define CONF_AUDIO_MUTE_OP		(CMD_CONF_OFFSET + 0x1000 + 0x00)
	#define AUDIO_MUTE			0x1
	#define AUDIO_UNMUTE		0x2
#define CONF_CLR_AUDINFO_PACKET	(CMD_CONF_OFFSET + 0x1000 + 0x01)
#define CONF_GET_AUDIO_MUTE_ST	(CMD_CONF_OFFSET + 0x1000 + 0x02)

#define CONF_ASPECT_RATIO		(CMD_CONF_OFFSET + 0x101a)

enum avi_component_conf {
	CONF_AVI_BT2020 = (CMD_CONF_OFFSET + 0X2000 + 0x00),
	CONF_AVI_RGBYCC_INDIC,
	CONF_AVI_Q01,
	CONF_AVI_YQ01,
	CONF_CT_MODE,
	CONF_GET_AVI_BT2020,
	CONF_AVI_VIC,
	CONF_AVI_CS,
	CONF_AVI_AR,
	CONF_AVI_CT_TYPE,
};

/* CONF_AVI_BT2020 CMD*/
#define CLR_AVI_BT2020	0x0
#define SET_AVI_BT2020	0x1
/* CONF_AVI_Q01 CMD*/
#define RGB_RANGE_DEFAULT	0
#define RGB_RANGE_LIM		1
#define RGB_RANGE_FUL		2
#define RGB_RANGE_RSVD		3
/* CONF_AVI_YQ01 */
#define YCC_RANGE_LIM		0
#define YCC_RANGE_FUL		1
#define YCC_RANGE_RSVD		2
/* CN TYPE define */
enum {
	SET_CT_OFF = 0,
	SET_CT_GAME = 1,
	SET_CT_GRAPHICS = 2,
	SET_CT_PHOTO = 3,
	SET_CT_CINEMA = 4,
};

/*set packet cmd*/
#define HDMI_SOURCE_DESCRIPTION 0
#define HDMI_PACKET_VEND        1
#define HDMI_MPEG_SOURCE_INFO   2
#define HDMI_PACKET_AVI         3
#define HDMI_AUDIO_INFO         4
#define HDMI_AUDIO_CONTENT_PROTECTION   5
#define HDMI_PACKET_HBR         6
#define HDMI_PACKET_DRM		0x86

/***********************************************************************
 *             HDMITX COMMON STRUCT & API
 **********************************************************************/
struct hdmitx_hw_common {
	int (*cntlmisc)(struct hdmitx_hw_common *tx_hw,
			u32 cmd, u32 arg);
	/* Configure control */
	int (*cntlconfig)(struct hdmitx_hw_common *tx_hw,
			u32 cmd, u32 arg);
	/* In original setpacket, there are many policies, like
	 *	if ((DB[4] >> 4) == T3D_FRAME_PACKING)
	 * Need a only pure data packet to call
	 */
	void (*setdatapacket)(int type, unsigned char *DB,
			unsigned char *HB);
	/* Audio/Video/System Status */
	int (*getstate)(struct hdmitx_hw_common *tx_hw,
			u32 cmd, u32 arg);
};

int hdmitx_hw_avmute(struct hdmitx_hw_common *tx_hw,
	int muteflag);
int hdmitx_hw_set_phy(struct hdmitx_hw_common *tx_hw,
	int flag);

#endif
