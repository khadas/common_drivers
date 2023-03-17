/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef HDMITX_BOOT_PARAMETERS
#define HDMITX_BOOT_PARAMETERS

#include <linux/types.h>
#include <linux/kernel.h>

#define INIT_FLAG_NOT_LOAD 0x80

struct hdmitx_boot_param {
	char	edid_chksum[11];
	char	color_attr[16];
	u32		fraction_refreshrate;
	u32		hdr_mask;
	u8		init_state;
	bool	config_csc;
};

struct hdmitx_boot_param *get_hdmitx_boot_params(void);

#endif
