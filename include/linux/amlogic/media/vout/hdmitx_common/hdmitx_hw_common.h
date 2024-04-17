/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __HDMITX_HW_COMMON_H
#define __HDMITX_HW_COMMON_H

#include <linux/types.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_mode.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_types.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_format_para.h>

/*hw cntl cmd define, abstract from hdmi_tx_module.h*/
#define CMD_DDC_OFFSET          (0x10 << 24)
#define CMD_STATUS_OFFSET       (0x11 << 24)
#define CMD_PACKET_OFFSET       (0x12 << 24)
#define CMD_MISC_OFFSET         (0x13 << 24)
#define CMD_CONF_OFFSET         (0x14 << 24)
#define CMD_STAT_OFFSET         (0x15 << 24)

/***********************************************************************
 *             DDC CONTROL //cntlddc
 **********************************************************************/
#define DDC_RESET_EDID          (CMD_DDC_OFFSET + 0x00)
#define DDC_RESET_HDCP          (CMD_DDC_OFFSET + 0x01)
#define DDC_HDCP_OP             (CMD_DDC_OFFSET + 0x02)
	#define HDCP14_ON	0x1
	#define HDCP14_OFF	0x2
	#define HDCP22_ON	0x3
	#define HDCP22_OFF	0x4
#define DDC_IS_HDCP_ON          (CMD_DDC_OFFSET + 0x04)
#define DDC_HDCP_GET_AKSV       (CMD_DDC_OFFSET + 0x05)
#define DDC_HDCP_GET_BKSV       (CMD_DDC_OFFSET + 0x06)
#define DDC_HDCP_GET_AUTH       (CMD_DDC_OFFSET + 0x07)
#define DDC_PIN_MUX_OP          (CMD_DDC_OFFSET + 0x08)
	#define PIN_MUX             0x1
	#define PIN_UNMUX           0x2
#define DDC_EDID_READ_DATA      (CMD_DDC_OFFSET + 0x0a)
#define DDC_IS_EDID_DATA_READY  (CMD_DDC_OFFSET + 0x0b)
#define DDC_EDID_CLEAR_RAM      (CMD_DDC_OFFSET + 0x0d)
#define DDC_HDCP_MUX_INIT       (CMD_DDC_OFFSET + 0x0e)
#define DDC_HDCP_14_LSTORE      (CMD_DDC_OFFSET + 0x0f)
#define DDC_HDCP_22_LSTORE      (CMD_DDC_OFFSET + 0x10)

#define DDC_GLITCH_FILTER_RESET (CMD_DDC_OFFSET + 0x11)
#define DDC_SCDC_DIV40_SCRAMB   (CMD_DDC_OFFSET + 0x20)
#define DDC_HDCP14_GET_BCAPS_RP (CMD_DDC_OFFSET + 0x30)
#define DDC_HDCP14_GET_TOPO_INFO (CMD_DDC_OFFSET + 0x31)
#define DDC_HDCP_SET_TOPO_INFO  (CMD_DDC_OFFSET + 0x32)
#define DDC_HDCP14_SAVE_OBS     (CMD_DDC_OFFSET + 0x40)

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
	#define TMDS_PHY_NONE       0x0
	#define TMDS_PHY_ENABLE     0x1
	#define TMDS_PHY_DISABLE    0x2

#define MISC_VIID_IS_USING      (CMD_MISC_OFFSET + 0x05)
#define MISC_CONF_MODE420       (CMD_MISC_OFFSET + 0x06)
#define MISC_TMDS_CLK_DIV40     (CMD_MISC_OFFSET + 0x07)
#define MISC_COMP_HPLL          (CMD_MISC_OFFSET + 0x08)
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
#define MISC_HPLL_FAKE          (CMD_MISC_OFFSET + 0x0c)
#define MISC_ESM_RESET          (CMD_MISC_OFFSET + 0x0d)
#define MISC_HDCP_CLKDIS        (CMD_MISC_OFFSET + 0x0e)
#define MISC_TMDS_RXSENSE       (CMD_MISC_OFFSET + 0x0f)
/* MISC_I2C_REACTIVE is merged into MISC_I2C_RESET */
/* #define MISC_I2C_REACTIVE       (CMD_MISC_OFFSET + 0x10) */
#define MISC_I2C_RESET          (CMD_MISC_OFFSET + 0x11)
#define MISC_READ_AVMUTE_OP     (CMD_MISC_OFFSET + 0x12)
#define MISC_TMDS_CEDST         (CMD_MISC_OFFSET + 0x13)
#define MISC_TRIGGER_HPD        (CMD_MISC_OFFSET + 0X14)
#define MISC_SUSFLAG            (CMD_MISC_OFFSET + 0X15)
#define MISC_AUDIO_RESET        (CMD_MISC_OFFSET + 0x16)
#define MISC_DIS_HPLL           (CMD_MISC_OFFSET + 0x17)
#define MISC_AUDIO_ACR_CTRL     (CMD_MISC_OFFSET + 0x18)
#define MISC_GET_FRL_MODE       (CMD_MISC_OFFSET + 0x19)
#define MISC_AUDIO_PREPARE	(CMD_MISC_OFFSET + 0x1a)
#define MISC_ESMCLK_CTRL        (CMD_MISC_OFFSET + 0x1b)
#define MISC_CLK_DIV_RST        (CMD_MISC_OFFSET + 0x20)
#define MISC_HPD_IRQ_TOP_HALF   (CMD_MISC_OFFSET + 0x21)
#define MISC_HDMI_CLKS_CTRL		(CMD_MISC_OFFSET + 0X22)

/***********************************************************************
 *                          Get State //getstate
 **********************************************************************/
#define STAT_VIDEO_VIC			(CMD_STAT_OFFSET + 0x00)
#define STAT_VIDEO_CLK			(CMD_STAT_OFFSET + 0x01)
#define STAT_VIDEO_CS			(CMD_STAT_OFFSET + 0x02)
#define STAT_VIDEO_CD			(CMD_STAT_OFFSET + 0x03)
#define STAT_AUDIO_FORMAT		(CMD_STAT_OFFSET + 0x10)
#define STAT_AUDIO_CHANNEL		(CMD_STAT_OFFSET + 0x11)
#define STAT_AUDIO_CLK_STABLE	(CMD_STAT_OFFSET + 0x12)
#define STAT_AUDIO_PACK			(CMD_STAT_OFFSET + 0x13)
#define STAT_HDR_TYPE			(CMD_STAT_OFFSET + 0x20)
#define STAT_TX_HDR				(CMD_STAT_OFFSET + 0x21) /*hdmitx_get_cur_hdr_st*/
#define STAT_TX_DV				(CMD_STAT_OFFSET + 0x22) /*hdmitx_get_cur_dv_st*/
#define STAT_TX_HDR10P			(CMD_STAT_OFFSET + 0x23) /*hdmitx_get_cur_hdr10p_st*/
#define STAT_TX_PHY				(CMD_STAT_OFFSET + 0x30)
#define STAT_TX_OUTPUT			(CMD_STAT_OFFSET + 0x31) /*if hdmitx have output*/
#define STAT_TX_DSC_EN (CMD_STAT_OFFSET + 0x32) /* if hdmitx have enable dsc */

/***********************************************************************
 *             CONFIG CONTROL //cntlconfig
 **********************************************************************/
/* Video part */
#define CONF_HDMI_DVI_MODE      (CMD_CONF_OFFSET + 0x02)
	#define HDMI_MODE           0x1
	#define DVI_MODE            0x2

#define CONF_VIDEO_MUTE_OP      (CMD_CONF_OFFSET + 0x1000 + 0x04)
	#define VIDEO_NONE_OP       0x0
	#define VIDEO_MUTE          0x1
	#define VIDEO_UNMUTE        0x2
#define CONF_EMP_NUMBER         (CMD_CONF_OFFSET + 0x3000 + 0x00)
#define CONF_EMP_PHY_ADDR       (CMD_CONF_OFFSET + 0x3000 + 0x01)

/* set value as COLORSPACE_RGB444, YUV422, YUV444, YUV420 */
#define CONFIG_CSC              (CMD_CONF_OFFSET + 0x1000 + 0x05)
	#define CSC_Y444_8BIT       0x1
	#define CSC_Y422_12BIT      0x2
	#define CSC_RGB_8BIT        0x3
	#define CSC_UPDATE_AVI_CS   0x10

/* Audio part */
#define CONF_CLR_AVI_PACKET     (CMD_CONF_OFFSET + 0x04)
#define CONF_VIDEO_MAPPING      (CMD_CONF_OFFSET + 0x06)
#define CONF_GET_HDMI_DVI_MODE  (CMD_CONF_OFFSET + 0x07)
#define CONF_CLR_DV_VS10_SIG    (CMD_CONF_OFFSET + 0x10)

#define CONF_AUDIO_MUTE_OP      (CMD_CONF_OFFSET + 0x1000 + 0x00)
	#define AUDIO_MUTE          0x1
	#define AUDIO_UNMUTE        0x2
#define CONF_CLR_AUDINFO_PACKET (CMD_CONF_OFFSET + 0x1000 + 0x01)
#define CONF_GET_AUDIO_MUTE_ST  (CMD_CONF_OFFSET + 0x1000 + 0x02)

#define CONF_ASPECT_RATIO       (CMD_CONF_OFFSET + 0x101a)
#define CONF_HW_INIT			(CMD_CONF_OFFSET + 0x101b)

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

#define IT_CONTENT		1

enum hdmi_ll_mode {
	HDMI_LL_MODE_AUTO = 0,
	HDMI_LL_MODE_DISABLE,
	HDMI_LL_MODE_ENABLE,
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

#define HDMITX_HWCMD_MUX_HPD_IF_PIN_HIGH       0x3
#define HDMITX_HWCMD_TURNOFF_HDMIHW           0x4
#define HDMITX_HWCMD_MUX_HPD                0x5
#define HDMITX_HWCMD_PLL_MODE                0x6
#define HDMITX_HWCMD_TURN_ON_PRBS           0x7
#define HDMITX_FORCE_480P_CLK                0x8
#define HDMITX_GET_AUTHENTICATE_STATE        0xa
#define HDMITX_SW_INTERNAL_HPD_TRIG          0xb
#define HDMITX_HWCMD_OSD_ENABLE              0xf

#define HDMITX_EARLY_SUSPEND_RESUME_CNTL     0x14
	#define HDMITX_EARLY_SUSPEND             0x1
	#define HDMITX_LATE_RESUME               0x2

/* Refer to HDMI_OTHER_CTRL0 in hdmi_tx_reg.h */
#define HDMITX_AVMUTE_CNTL                   0x19
	#define AVMUTE_SET          0   /* set AVMUTE to 1 */
	#define AVMUTE_CLEAR        1   /* set AVunMUTE to 1 */
	#define AVMUTE_OFF          2   /* set both AVMUTE and AVunMUTE to 0 */
#define HDMITX_CBUS_RST                      0x1A
#define HDMITX_INTR_MASKN_CNTL               0x1B
	#define INTR_MASKN_ENABLE   0
	#define INTR_MASKN_DISABLE  1
	#define INTR_CLEAR          2

/***********************************************************************
 *             HDMITX COMMON STRUCT & API
 **********************************************************************/
struct tx_cap {
	/* configure in dts file */
	enum frl_rate_enum  tx_max_frl_rate;
	/* default 600Mhz, if res_1080p, then 225Mhz */
	u32 tx_max_tmds_clk;
	/* soc support DSC or not */
	bool dsc_capable;
	/* DSC enable policy, refer to dsc_policy sysfs node */
	u8 dsc_policy;
};

struct hdmitx_hw_common {
	/*uninit when destroy*/
	void (*uninit)(struct hdmitx_hw_common *tx_hw);

	int (*setupirq)(struct hdmitx_hw_common *tx_hw);

	int (*cntlmisc)(struct hdmitx_hw_common *tx_hw, u32 cmd, u32 arg);
	/* Configure control */
	int (*cntlconfig)(struct hdmitx_hw_common *tx_hw, u32 cmd, u32 arg);
	/* Control ddc for hdcp/edid functions */
	int (*cntlddc)(struct hdmitx_hw_common *tx_hw,
		unsigned int cmd, unsigned long arg);
	/* Other control */
	int (*cntl)(struct hdmitx_hw_common *tx_hw, unsigned int cmd, unsigned int arg);

	/* In original setpacket, there are many policies, like
	 *	if ((DB[4] >> 4) == T3D_FRAME_PACKING)
	 * Need a only pure data packet to call
	 */
	void (*setpacket)(int type, unsigned char *DB, unsigned char *HB);
	void (*disablepacket)(int type);

	/* Audio/Video/System Status */
	int (*getstate)(struct hdmitx_hw_common *tx_hw, u32 cmd, u32 arg);

	/*validate if vic is supported by hw ip/phy*/
	int (*validatemode)(struct hdmitx_hw_common *tx_hw, u32 vic);
	/*calc formatpara hw info config*/
	int (*calc_format_para)(struct hdmitx_hw_common *tx_hw, struct hdmi_format_para *para);

	int (*setaudmode)(struct hdmitx_hw_common *tx_hw, struct aud_para *audio_param);

	/*debug function*/
	void (*debugfun)(struct hdmitx_hw_common *tx_hw, const char *cmd_str);
	int (*setdispmode)(struct hdmitx_hw_common *tx_hw);
	u8 debug_hpd_lock;

	/* GPIO hpd/scl/sda members*/
	int hdmitx_gpios_hpd;
	int hdmitx_gpios_scl;
	int hdmitx_gpios_sda;

	/* hdcp repeater enable, such as on T7 platform */
	u32 hdcp_repeater_en:1;
	/* soc/hdmitx driver capability */
	struct tx_cap hdmi_tx_cap;
	/* phy state */
	unsigned char tmds_phy_op;

	/* save the lastest plug_in time from interrupt*/
	u64 hw_sequence_id;
};

int hdmitx_hw_cntl_config(struct hdmitx_hw_common *tx_hw,
	u32 cmd, u32 arg);
int hdmitx_hw_cntl_misc(struct hdmitx_hw_common *tx_hw,
	u32 cmd, u32 arg);
int hdmitx_hw_cntl_ddc(struct hdmitx_hw_common *tx_hw,
	unsigned int cmd, unsigned long arg);
int hdmitx_hw_cntl(struct hdmitx_hw_common *tx_hw,
	unsigned int cmd, unsigned long arg);
int hdmitx_hw_get_state(struct hdmitx_hw_common *tx_hw,
	u32 cmd, u32 arg);
int hdmitx_hw_validate_mode(struct hdmitx_hw_common *tx_hw,
	u32 vic);
int hdmitx_hw_calc_format_para(struct hdmitx_hw_common *tx_hw,
	struct hdmi_format_para *para);
int hdmitx_hw_set_packet(struct hdmitx_hw_common *tx_hw,
	int type, unsigned char *DB, unsigned char *HB);
int hdmitx_hw_disable_packet(struct hdmitx_hw_common *tx_hw,
	int type);
int hdmitx_hw_set_phy(struct hdmitx_hw_common *tx_hw,
	int flag);

enum hdmi_tf_type hdmitx_hw_get_hdr_st(struct hdmitx_hw_common *tx_hw);
enum hdmi_tf_type hdmitx_hw_get_dv_st(struct hdmitx_hw_common *tx_hw);
enum hdmi_tf_type hdmitx_hw_get_hdr10p_st(struct hdmitx_hw_common *tx_hw);

/*utils functions shared for hdmitx hw module.*/

#endif
