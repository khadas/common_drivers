/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __HDMI_TX20_HW_H
#define __HDMI_TX20_HW_H

#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_hw_common.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_audio.h>

/* chip type */
enum amhdmitx_chip_e {
	MESON_CPU_ID_M8B = 0,
	MESON_CPU_ID_GXBB,
	MESON_CPU_ID_GXTVBB,
	MESON_CPU_ID_GXL,
	MESON_CPU_ID_GXM,
	MESON_CPU_ID_TXL,
	MESON_CPU_ID_TXLX,
	MESON_CPU_ID_AXG,
	MESON_CPU_ID_GXLX,
	MESON_CPU_ID_TXHD,
	MESON_CPU_ID_G12A,
	MESON_CPU_ID_G12B,
	MESON_CPU_ID_SM1,
	MESON_CPU_ID_TM2,
	MESON_CPU_ID_TM2B,
	MESON_CPU_ID_SC2,
	MESON_CPU_ID_MAX,
};

struct amhdmitx_data_s {
	enum amhdmitx_chip_e chip_type;
	const char *chip_name;
};

struct hdmitx20_hw {
	struct hdmitx_hw_common base;
	struct amhdmitx_data_s *chip_data;
	u32 dongle_mode:1;
	u32 repeater_mode:1;

	/*for debug*/
	u32 debug_hpd_lock;
};

#define to_hdmitx20_hw(x)	container_of(x, struct hdmitx20_hw, base)

struct hdmitx20_hw *get_hdmitx20_hw_instance(void);

unsigned int hdmitx_rd_reg(unsigned int addr);

/*
 * HDMITX HPD HW related operations
 */
enum hpd_op {
	HPD_INIT_DISABLE_PULLUP,
	HPD_INIT_SET_FILTER,
	HPD_IS_HPD_MUXED,
	HPD_MUX_HPD,
	HPD_UNMUX_HPD,
	HPD_READ_HPD_GPIO,
};

int hdmitx_hpd_hw_op(enum hpd_op cmd);

#endif
