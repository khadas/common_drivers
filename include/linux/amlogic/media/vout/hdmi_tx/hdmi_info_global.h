/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _HDMI_INFO_GLOBAL_H
#define _HDMI_INFO_GLOBAL_H

#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_types.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_common.h>
#include "../hdmi_tx_ext.h"

/* -----------------------HDMI TX---------------------------------- */

struct hdmitx_vidpara {
	unsigned int VIC;
	enum hdmi_colorspace color_prefer;
	enum hdmi_colorspace color;
	enum hdmi_color_depth color_depth;
	enum hdmi_barinfo bar_info;
	enum hdmi_pixel_repeat repeat_time;
	enum hdmi_aspect_ratio aspect_ratio;
	enum hdmi_colourimetry cc;
	enum hdmi_scan ss;
	enum hdmi_scaling sc;
};

struct hdmitx_audpara {
	enum hdmi_audio_type type;
	enum hdmi_audio_chnnum channel_num;
	enum hdmi_audio_fs sample_rate;
	enum hdmi_audio_sampsize sample_size;
	enum hdmi_audio_source_if aud_src_if;
};

struct hdmitx_audinfo {
	/* !< Signal decoding type -- TvAudioType */
	enum hdmi_audio_type type;
	enum hdmi_audio_format format;
	/* !< active audio channels bit mask. */
	enum hdmi_audio_chnnum channels;
	enum hdmi_audio_fs fs; /* !< Signal sample rate in Hz */
	enum hdmi_audio_sampsize ss;
};

#define Y420CMDB_MAX	32
struct hdmitx_info {
	struct hdmi_rx_audioinfo audio_info;
	/* -----------------Source Physical Address--------------- */
	struct vsdb_phyaddr vsdb_phy_addr;

	/* ------------------------------------------------------- */
	/* for total = 32*8 = 256 VICs */
	/* for Y420CMDB bitmap */
	unsigned char bitmap_valid;
	unsigned char bitmap_length;
	unsigned char y420_all_vic;
	unsigned char y420cmdb_bitmap[Y420CMDB_MAX];
	/* ------------------------------------------------------- */
};

#endif  /* _HDMI_RX_GLOBAL_H */
