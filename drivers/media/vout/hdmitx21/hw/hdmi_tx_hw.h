/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __HDMI_TX21_HW_H
#define __HDMI_TX21_HW_H

#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_hw_common.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_audio.h>

/* chip type */
enum amhdmitx_chip_e {
	MESON_CPU_ID_T7 = 0,
	MESON_CPU_ID_S1A,
	MESON_CPU_ID_S5,
	MESON_CPU_ID_MAX,
};

struct amhdmitx_data_s {
	enum amhdmitx_chip_e chip_type;
	const char *chip_name;
};

struct hdmitx21_hw {
	struct hdmitx_hw_common base;
	struct amhdmitx_data_s *chip_data;
	u8 tx_max_frl_rate; /* configure in dts file */
	unsigned int dongle_mode:1;
	u32 repeater_mode:1;
	u32 enc_idx;
	struct hdmitx_infoframe *infoframes;
};

#define to_hdmitx21_hw(x)	container_of(x, struct hdmitx21_hw, base)

#endif
