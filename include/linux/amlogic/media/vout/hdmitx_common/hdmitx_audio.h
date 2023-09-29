/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __HDMITX_AUDIO_H
#define __HDMITX_AUDIO_H

u32 hdmitx_hw_get_audio_n_paras(enum hdmi_audio_fs fs, enum hdmi_color_depth cd, u32 tmds_clk);

/* log_buf = null, will print with printk. or will write to log_buf.
 * return is the log len.
 */
int hdmitx_audio_para_print(struct aud_para *audio_para, char *log_buf);

#endif
