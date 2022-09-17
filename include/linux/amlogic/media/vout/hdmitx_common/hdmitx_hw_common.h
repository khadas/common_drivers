/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __HDMITX_HW_COMMON_H
#define __HDMITX_HW_COMMON_H

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

struct hdmitx_hw_common {
	int (*cntlmisc)(struct hdmitx_hw_common *tx_hw,
			unsigned int cmd, unsigned int arg);
};

int hdmitx_hw_avmute(struct hdmitx_hw_common *tx_hw,
	int muteflag);
int hdmitx_hw_set_phy(struct hdmitx_hw_common *tx_hw,
	int flag);

#endif
