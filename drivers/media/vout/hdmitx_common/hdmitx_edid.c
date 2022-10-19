// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_edid.h>
#include <linux/amlogic/media/vout/hdmi_tx_ext.h>

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

/* check the first edid block */
int _check_base_structure(unsigned char *buf)
{
	unsigned int i = 0;

	/* check block 0 first 8 bytes */
	if (buf[0] != 0 && buf[7] != 0)
		return 0;

	for (i = 1; i < 7; i++) {
		if (buf[i] != 0xff)
			return 0;
	}

	if (_check_edid_blk_chksum(buf) == 0)
		return 0;

	return 1;
}

/* check the checksum for each sub block */
int _check_edid_blk_chksum(unsigned char *block)
{
	unsigned int chksum = 0;
	unsigned int i = 0;

	for (chksum = 0, i = 0; i < 0x80; i++)
		chksum += block[i];
	if ((chksum & 0xff) != 0)
		return 0;
	else
		return 1;
}

/*
 * check the EDID validity
 * base structure: header, checksum
 * extension: the first non-zero byte, checksum
 */
int check_dvi_hdmi_edid_valid(unsigned char *buf)
{
	int i;
	int blk_cnt = buf[0x7e] + 1;

	/* limit blk_cnt to EDID_MAX_BLOCK  */
	if (blk_cnt > EDID_MAX_BLOCK)
		blk_cnt = EDID_MAX_BLOCK;

	/* check block 0 */
	if (_check_base_structure(&buf[0]) == 0)
		return 0;

	if (blk_cnt == 1)
		return 1;

	/* check extension block 1 and more */
	for (i = 1; i < blk_cnt; i++) {
		if (buf[i * 0x80] == 0)
			return 0;
		if (_check_edid_blk_chksum(&buf[i * 0x80]) == 0)
			return 0;
	}

	return 1;
}

/* return 0 means valid */
int hdmitx_edid_validate(u8 *rawedid)
{
	unsigned int hdmi_ver = hdmitx_drv_ver();

	/* notify EDID NG to systemcontrol */
	/* todo: hdmi21 for tv_ts */
	if (!rawedid)
		return -EINVAL;
	if (hdmi_ver == 0) {
		return -EINVAL;
	} else if (hdmi_ver == 20) {
		if (check_dvi_hdmi_edid_valid(rawedid))
			return 0;
		else
			return -EINVAL;
	} else if (hdmi_ver == 21) {
		if (hdmitx_edid_is_all_zeros(rawedid))
			return -EINVAL;
		else if ((rawedid[0x7e] > 3) &&
			hdmitx_edid_header_invalid(rawedid))
			return -EINVAL;
		/* may extend NG case here */
	}
	return 0;
}

/*index is hdmi 1.4 vic in vsif, value is hdmi2.0 vic*/
static const u32 hdmi14_4k_vics[] = {
/* 0 - dummy*/
	0,
/* 1 - 3840x2160@30Hz */
	95,
/* 2 - 3840x2160@25Hz */
	94,
/* 3 - 3840x2160@24Hz */
	93,
/* 4 - 4096x2160@24Hz (SMPTE) */
	98,
};

u32 hdmitx_edid_get_hdmi14_4k_vic(u32 vic)
{
	bool ret = 0;
	int i;

	for (i = 0; i < ARRAY_SIZE(hdmi14_4k_vics); i++) {
		if (vic == hdmi14_4k_vics[i]) {
			ret = i;
			break;
		}
	}

	return ret;
}
