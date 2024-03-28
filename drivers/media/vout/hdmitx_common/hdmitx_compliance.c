// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/string.h>

/* Base Block, Vendor/Product Information, byte[8]~[17] */
struct edid_venddat_t {
	unsigned char data[10];
};

static struct edid_venddat_t vendor_6g[] = {
	/* SAMSUNG UA55KS7300JXXZ */
	{ {0x4c, 0x2d, 0x3b, 0x0d, 0x00, 0x06, 0x00, 0x01, 0x01, 0x1a} }
	/* Add new vendor data here */
};

static struct edid_venddat_t vendor_ratio[] = {
	/* Mi L55M2-AA */
	{ {0x61, 0xA4, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x19} }
	/* Add new vendor data here */
};

static struct edid_venddat_t vendor_null_pkt[] = {
	/* changhong LT32876 */
	{ {0x0d, 0x04, 0x21, 0x90, 0x01, 0x00, 0x00, 0x00, 0x20, 0x12} }
	/* Add new vendor data here */
};

static struct edid_venddat_t vendor_phy_delay[] = {
	/* WHALEY WTV55K1J */
	{ {0x5e, 0x96, 0x11, 0x55, 0x01, 0x00, 0x00, 0x00, 0x10, 0x1a} }
	/* Add new vendor data here */
};

static struct edid_venddat_t hdr_delay_id[] = {
	/* HUAWEI */
	{ {0x22, 0xf6, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x1d} }
	/* Add new vendor data here */
};

/* HDMIPLL_CTRL3/4 under 4k50/60hz 6G mode should use the setting
 * witch is used under 4k59.94hz, specailly for SAMSUNG UA55KS7300JXXZ
 * flash screen/no signal issue on SM1/SC2
 */
bool hdmitx_find_vendor_6g(unsigned char *edid_buf)
{
	int i;

	if (!edid_buf)
		return false;
	for (i = 0; i < ARRAY_SIZE(vendor_6g); i++) {
		if (memcmp(&edid_buf[8], vendor_6g[i].data,
			   sizeof(vendor_6g[i].data)) == 0)
			return true;
	}
	return false;
}

/* need to forcely change clk ratio for such TV when suspend/resume box */
bool hdmitx_find_vendor_ratio(unsigned char *edid_buf)
{
	int i;

	if (!edid_buf)
		return false;
	for (i = 0; i < ARRAY_SIZE(vendor_ratio); i++) {
		if (memcmp(&edid_buf[8], vendor_ratio[i].data,
			   sizeof(vendor_ratio[i].data)) == 0)
			return true;
	}
	return false;
}

/* need to forcely enable null packet for such TV */
bool hdmitx_find_vendor_null_pkt(unsigned char *edid_buf)
{
	int i;

	if (!edid_buf)
		return false;
	for (i = 0; i < ARRAY_SIZE(vendor_null_pkt); i++) {
		if (memcmp(&edid_buf[8], vendor_null_pkt[i].data,
		    sizeof(vendor_null_pkt[i].data)) == 0)
			return true;
	}
	return false;
}

/* need add delay after disable phy */
bool hdmitx_find_vendor_phy_delay(unsigned char *edid_buf)
{
	int i;

	if (!edid_buf)
		return false;
	for (i = 0; i < ARRAY_SIZE(vendor_phy_delay); i++) {
		if (memcmp(&edid_buf[8], vendor_phy_delay[i].data,
		    sizeof(vendor_phy_delay[i].data)) == 0)
			return true;
	}
	return false;
}

/* On Huawei TVs, HDR will cause a flickering screen,
 * and HDR PKT needs to be moved behind VSYNC on S7 or later SOCs
 */
bool hdmitx_find_hdr_pkt_delay_to_vsync(unsigned char *edid_buf)
{
	int i;

	if (!edid_buf)
		return false;
	for (i = 0; i < ARRAY_SIZE(hdr_delay_id); i++) {
		if (memcmp(&edid_buf[8], hdr_delay_id[i].data,
			sizeof(hdr_delay_id[i].data)) == 0)
			return true;
	}
	return false;
}

