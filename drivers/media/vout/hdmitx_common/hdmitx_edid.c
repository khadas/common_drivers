// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/errno.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_edid.h>

static bool hdmitx_edid_header_invalid(u8 *buf)
{
	bool base_blk_invalid = false;
	bool ext_blk_invalid = false;
	bool ret = false;
	int i = 0;

	if (buf[0] != 0 || buf[7] != 0) {
		base_blk_invalid = true;
	} else {
		for (i = 1; i < 7; i++) {
			if (buf[i] != 0xff) {
				base_blk_invalid = true;
				break;
			}
		}
	}
	/* judge header strictly, only if both header invalid */
	if (buf[0x7e] > 0) {
		if (buf[0x80] != 0x2 && buf[0x80] != 0xf0)
			ext_blk_invalid = true;
		ret = base_blk_invalid && ext_blk_invalid;
	} else {
		ret = base_blk_invalid;
	}

	return ret;
}

bool hdmitx_edid_is_all_zeros(u8 *rawedid)
{
	unsigned int i = 0, j = 0;
	unsigned int chksum = 0;

	for (j = 0; j < EDID_MAX_BLOCK; j++) {
		chksum = 0;
		for (i = 0; i < 128; i++)
			chksum += rawedid[i + j * 128];
		if (chksum != 0)
			return false;
	}
	return true;
}

int hdmitx_edid_validate(u8 *rawedid)
{
	if (!rawedid)
		return -EINVAL;
	/* notify EDID NG to systemcontrol */
	if (hdmitx_edid_is_all_zeros(rawedid))
		return -EINVAL;
	else if ((rawedid[0x7e] > 3) &&
		hdmitx_edid_header_invalid(rawedid))
		return -EINVAL;
	/* may extend NG case here */

	return 0;
}
