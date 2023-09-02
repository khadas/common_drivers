// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

//#define DEBUG

#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/mm.h>
#include <linux/major.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <crypto/hash.h>
#include <linux/crypto.h>
#include <linux/scatterlist.h>
#include <linux/delay.h>

#include <linux/amlogic/media/vout/vinfo.h>
#include <linux/amlogic/media/vout/vout_notify.h>
#include <linux/amlogic/media/vout/hdmi_tx/hdmi_info_global.h>
#include <linux/amlogic/media/vout/hdmi_tx/hdmi_tx_module.h>
#include "hw/common.h"
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_edid.h>

#define CEA_DATA_BLOCK_COLLECTION_ADDR_1STP 0x04
#define VIDEO_TAG 0x40
#define AUDIO_TAG 0x20
#define VENDOR_TAG 0x60
#define SPEAKER_TAG 0x80

#define HDMI_EDID_BLOCK_TYPE_RESERVED	        0
#define HDMI_EDID_BLOCK_TYPE_AUDIO		1
#define HDMI_EDID_BLOCK_TYPE_VIDEO		2
#define HDMI_EDID_BLOCK_TYPE_VENDER	        3
#define HDMI_EDID_BLOCK_TYPE_SPEAKER	        4
#define HDMI_EDID_BLOCK_TYPE_VESA		5
#define HDMI_EDID_BLOCK_TYPE_RESERVED2	        6
#define HDMI_EDID_BLOCK_TYPE_EXTENDED_TAG       7

#define EXTENSION_VENDOR_SPECIFIC 0x1
#define EXTENSION_COLORMETRY_TAG 0x5
/* DRM stands for "Dynamic Range and Mastering " */
#define EXTENSION_DRM_STATIC_TAG    0x6
#define EXTENSION_DRM_DYNAMIC_TAG   0x7
   #define TYPE_1_HDR_METADATA_TYPE    0x0001
   #define TS_103_433_SPEC_TYPE        0x0002
   #define ITU_T_H265_SPEC_TYPE        0x0003
   #define TYPE_4_HDR_METADATA_TYPE    0x0004
/* Video Format Preference Data block */
#define EXTENSION_VFPDB_TAG	0xd
#define EXTENSION_Y420_VDB_TAG	0xe
#define EXTENSION_Y420_CMDB_TAG	0xf
#define EXTENSION_Y420_SCDB_TAG	0x79 /*HDMI Forum Sink Capability Data Block*/
#define EXTENSION_DOLBY_VSADB	0x11

#define EDID_DETAILED_TIMING_DES_BLOCK0_POS 0x36
#define EDID_DETAILED_TIMING_DES_BLOCK1_POS 0x48
#define EDID_DETAILED_TIMING_DES_BLOCK2_POS 0x5A
#define EDID_DETAILED_TIMING_DES_BLOCK3_POS 0x6C

/* EDID Descriptor Tag */
#define TAG_PRODUCT_SERIAL_NUMBER 0xFF
#define TAG_ALPHA_DATA_STRING 0xFE
#define TAG_RANGE_LIMITS 0xFD
#define TAG_DISPLAY_PRODUCT_NAME_STRING 0xFC /* MONITOR NAME */
#define TAG_COLOR_POINT_DATA 0xFB
#define TAG_STANDARD_TIMINGS 0xFA
#define TAG_DISPLAY_COLOR_MANAGEMENT 0xF9
#define TAG_CVT_TIMING_CODES 0xF8
#define TAG_ESTABLISHED_TIMING_III 0xF7
#define TAG_DUMMY_DES 0x10

static unsigned int hdmitx_edid_check_valid_blocks(unsigned char *buf);
static void edid_dtd_parsing(struct rx_cap *prxcap, unsigned char *data);
static void hdmitx_edid_set_default_aud(struct hdmitx_dev *hdev);
static bool hdmitx_edid_header_invalid(unsigned char *buf);
/* Base Block, Vendor/Product Information, byte[8]~[18] */
struct edid_venddat_t {
	unsigned char data[10];
};

static struct edid_venddat_t vendor_id[] = {
{ {0x41, 0x0C, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x03, 0x14} },
/* { {0x05, 0xAC, 0x30, 0x00, 0x01, 0x00, 0x00, 0x00, 0x20, 0x19} }, */
/* Add new vendor data here */
};

static void edid_parsing_id_manufacturer_name(struct rx_cap *prxcap,
					   unsigned char *data)
{
	int i;
	unsigned char uppercase[26] = { 0 };
	unsigned char brand[3];

	/* Fill array uppercase with 'A' to 'Z' */
	for (i = 0; i < 26; i++)
		uppercase[i] = 'A' + i;

	brand[0] = data[0] >> 2;
	brand[1] = ((data[0] & 0x3) << 3) + (data[1] >> 5);
	brand[2] = data[1] & 0x1f;

	if (brand[0] > 26 || brand[0] == 0 ||
	    brand[1] > 26 || brand[1] == 0 ||
	    brand[2] > 26 || brand[2] == 0)
		return;
	for (i = 0; i < 3; i++)
		prxcap->IDManufacturerName[i] = uppercase[brand[i] - 1];
}

static void edid_parsing_id_product_code(struct rx_cap *prxcap,
				      unsigned char *data)
{
	if (!data)
		return;
	prxcap->IDProductCode[0] = data[1];
	prxcap->IDProductCode[1] = data[0];
}

static void edid_parsing_id_serial_number(struct rx_cap *prxcap,
				       unsigned char *data)
{
	int i;

	if (data)
		for (i = 0; i < 4; i++)
			prxcap->IDSerialNumber[i] = data[3 - i];
}

bool LGAVIErrorTV(struct rx_cap *prxcap)
{
	if (strcmp("GSM", (char *)&prxcap->IDManufacturerName[0]) == 0 &&
	    prxcap->IDProductCode[0] == 1 &&
	    prxcap->IDProductCode[1] == 0 &&
	    prxcap->IDSerialNumber[0] == 1 &&
	    prxcap->IDSerialNumber[1] == 1 &&
	    prxcap->IDSerialNumber[2] == 1 &&
	    prxcap->IDSerialNumber[3] == 1)
		return true;
	return false;
}

/* store the idx of vesa_timing[32], which is 0 */
static void store_vesa_idx(struct rx_cap *prxcap, enum hdmi_vic vesa_timing)
{
	int i;

	for (i = 0; i < VESA_MAX_TIMING; i++) {
		if (!prxcap->vesa_timing[i]) {
			prxcap->vesa_timing[i] = vesa_timing;
			break;
		}

		if (prxcap->vesa_timing[i] == vesa_timing)
			break;
	}
}

static void store_cea_idx(struct rx_cap *prxcap, enum hdmi_vic vic)
{
	int i;
	int already = 0;

	for (i = 0; (i < VIC_MAX_NUM) && (i < prxcap->VIC_count); i++) {
		if (vic == prxcap->VIC[i]) {
			already = 1;
			break;
		}
	}
	if (!already) {
		prxcap->VIC[prxcap->VIC_count] = vic;
		prxcap->VIC_count++;
	}
}

static void store_y420_idx(struct rx_cap *prxcap, enum hdmi_vic vic)
{
	int i;
	int already = 0;

	/* Y420 is claimed in Y420VDB, y420_vic[] will list in dc_cap */
	for (i = 0; i < Y420_VIC_MAX_NUM; i++) {
		if (vic == prxcap->y420_vic[i]) {
			already = 1;
			break;
		}
	}
	if (!already) {
		for (i = 0; i < Y420_VIC_MAX_NUM; i++) {
			if (prxcap->y420_vic[i] == 0) {
				prxcap->y420_vic[i] = vic;
				break;
			}
		}
	}
}

static void edid_established_timings(struct rx_cap *prxcap, unsigned char *data)
{
	if (data[0] & (1 << 5))
		store_vesa_idx(prxcap, HDMIV_640x480p60hz);
	if (data[0] & (1 << 0))
		store_vesa_idx(prxcap, HDMIV_800x600p60hz);
	if (data[1] & (1 << 3))
		store_vesa_idx(prxcap, HDMIV_1024x768p60hz);
}

static void edid_standard_timing_iii(struct rx_cap *prxcap, unsigned char *data)
{
	if (data[0] & (1 << 0))
		store_vesa_idx(prxcap, HDMIV_1152x864p75hz);
	if (data[1] & (1 << 6))
		store_vesa_idx(prxcap, HDMIV_1280x768p60hz);
	if (data[1] & (1 << 2))
		store_vesa_idx(prxcap, HDMIV_1280x960p60hz);
	if (data[1] & (1 << 1))
		store_vesa_idx(prxcap, HDMIV_1280x1024p60hz);
	if (data[2] & (1 << 7))
		store_vesa_idx(prxcap, HDMIV_1360x768p60hz);
	if (data[2] & (1 << 1))
		store_vesa_idx(prxcap, HDMIV_1400x1050p60hz);
	if (data[3] & (1 << 5))
		store_vesa_idx(prxcap, HDMIV_1680x1050p60hz);
	if (data[3] & (1 << 2))
		store_vesa_idx(prxcap, HDMIV_1600x1200p60hz);
	if (data[4] & (1 << 0))
		store_vesa_idx(prxcap, HDMIV_1920x1200p60hz);
}

static void calc_timing(unsigned char *data, struct vesa_standard_timing *t)
{
	const struct hdmi_timing *timing = NULL;

	if (data[0] < 2 && data[1] < 2)
		return;
	t->hactive = (data[0] + 31) * 8;
	switch ((data[1] >> 6) & 0x3) {
	case 0:
		t->vactive = t->hactive * 5 / 8;
		break;
	case 1:
		t->vactive = t->hactive * 3 / 4;
		break;
	case 2:
		t->vactive = t->hactive * 4 / 5;
		break;
	case 3:
	default:
		t->vactive = t->hactive * 9 / 16;
		break;
	}
	t->vsync = (data[1] & 0x3f) + 60;
	timing = hdmitx_mode_match_vesa_timing(t);
	if (timing) {
		struct hdmitx_dev *hdev = get_hdmitx_device();
		struct rx_cap *prxcap = &hdev->tx_comm.rxcap;

		/* prefer 16x9 mode */
		if (timing->vic == HDMI_720x480i60_4x3 ||
			timing->vic == HDMI_720x480p60_4x3 ||
			timing->vic == HDMI_720x576i50_4x3 ||
			timing->vic == HDMI_720x576p50_4x3)
			t->vesa_timing = timing->vic + 1;
		else
			t->vesa_timing = timing->vic;

		if (t->vesa_timing < HDMITX_VESA_OFFSET)
			store_cea_idx(prxcap, t->vesa_timing);
		else
			store_vesa_idx(prxcap, t->vesa_timing);
	}
}

static void edid_standardtiming(struct rx_cap *prxcap, unsigned char *data,
				int max_num)
{
	int i;
	struct vesa_standard_timing timing;

	for (i = 0; i < max_num; i++) {
		memset(&timing, 0, sizeof(struct vesa_standard_timing));
		calc_timing(&data[i * 2], &timing);
	}
}

static void edid_receiverproductnameparse(struct rx_cap *prxcap,
					  unsigned char *data)
{
	int i = 0;
	/* some Display Product name end with 0x20, not 0x0a
	 */
	while ((data[i] != 0x0a) && (data[i] != 0x20) && (i < 13)) {
		prxcap->ReceiverProductName[i] = data[i];
		i++;
	}
	prxcap->ReceiverProductName[i] = '\0';
}

/* ----------------------------------------------------------- */
void edid_parseceatiming(struct rx_cap *prxcap,
	unsigned char *buff)
{
	int i;
	unsigned char *dtd_base = buff;

	for (i = 0; i < 4; i++) {
		edid_dtd_parsing(prxcap, dtd_base);
		dtd_base += 0x12;
	}
}

static struct vsdb_phyaddr vsdb_local = {0};
int get_vsdb_phy_addr(struct vsdb_phyaddr *vsdb)
{
	vsdb = &vsdb_local;
	return vsdb->valid;
}

static void set_vsdb_phy_addr(struct hdmitx_dev *hdev,
			      struct vsdb_phyaddr *vsdb,
			      unsigned char *edid_offset)
{
	int phy_addr;

	vsdb->a = (edid_offset[0] >> 4) & 0xf;
	vsdb->b = (edid_offset[0] >> 0) & 0xf;
	vsdb->c = (edid_offset[1] >> 4) & 0xf;
	vsdb->d = (edid_offset[1] >> 0) & 0xf;
	vsdb_local = *vsdb;
	vsdb->valid = 1;

	phy_addr = ((vsdb->a & 0xf) << 12) |
		   ((vsdb->b & 0xf) <<  8) |
		   ((vsdb->c & 0xf) <<  4) |
		   ((vsdb->d & 0xf) <<  0);
	hdev->physical_addr = phy_addr;
}

static void set_vsdb_dc_cap(struct rx_cap *prxcap)
{
	prxcap->dc_y444 = !!(prxcap->ColorDeepSupport & (1 << 3));
	prxcap->dc_30bit = !!(prxcap->ColorDeepSupport & (1 << 4));
	prxcap->dc_36bit = !!(prxcap->ColorDeepSupport & (1 << 5));
	prxcap->dc_48bit = !!(prxcap->ColorDeepSupport & (1 << 6));
}

static void set_vsdb_dc_420_cap(struct rx_cap *prxcap,
				unsigned char *edid_offset)
{
	prxcap->dc_30bit_420 = !!(edid_offset[6] & (1 << 0));
	prxcap->dc_36bit_420 = !!(edid_offset[6] & (1 << 1));
	prxcap->dc_48bit_420 = !!(edid_offset[6] & (1 << 2));
}

/* Special FBC check */
static int check_fbc_special(unsigned char *edid_dat)
{
	if (edid_dat[250] == 0xfb && edid_dat[251] == 0x0c)
		return 1;
	else
		return 0;
}

static void _edid_parsingvendspec(struct dv_info *dv,
				  struct hdr10_plus_info *hdr10_plus,
				  struct cuva_info *cuva,
				  unsigned char *buf)
{
	unsigned char *dat = buf;
	unsigned char *cuva_dat = buf;
	unsigned char pos = 0;
	unsigned int ieeeoui = 0;
	u8 length = 0;

	length = dat[pos] & 0x1f;
	pos++;

	if (dat[pos] != 1) {
		pr_err("hdmitx: edid: parsing fail %s[%d]\n", __func__,
			__LINE__);
		return;
	}

	pos++;
	ieeeoui = dat[pos++];
	ieeeoui += dat[pos++] << 8;
	ieeeoui += dat[pos++] << 16;
	pr_debug("Edid_ParsingVendSpec:ieeeoui=0x%x,len=%u\n", ieeeoui, length);

/*HDR10+ use vsvdb*/
	if (ieeeoui == HDR10_PLUS_IEEE_OUI) {
		memset(hdr10_plus, 0, sizeof(struct hdr10_plus_info));
		hdr10_plus->length = length;
		hdr10_plus->ieeeoui = ieeeoui;
		hdr10_plus->application_version = dat[pos] & 0x3;
		pos++;
		return;
	}
	if (ieeeoui == CUVA_IEEEOUI) {
		memcpy(cuva->rawdata, cuva_dat, 15); /* 15, fixed length */
		cuva->length = cuva_dat[0] & 0x1f;
		cuva->ieeeoui = cuva_dat[2] |
				(cuva_dat[3] << 8) |
				(cuva_dat[4] << 16);
		cuva->system_start_code = cuva_dat[5];
		cuva->version_code = cuva_dat[6] >> 4;
		cuva->display_max_lum = cuva_dat[7] |
					(cuva_dat[8] << 8) |
					(cuva_dat[9] << 16) |
					(cuva_dat[10] << 24);
		cuva->display_min_lum = cuva_dat[11] | (cuva_dat[12] << 8);
		cuva->rx_mode_sup = (cuva_dat[13] >> 6) & 0x1;
		cuva->monitor_mode_sup = (cuva_dat[13] >> 7) & 0x1;
		return;
	}

	if (ieeeoui == DV_IEEE_OUI) {
		/* it is a Dovi block*/
		memset(dv, 0, sizeof(struct dv_info));
		dv->block_flag = CORRECT;
		dv->length = length;
		memcpy(dv->rawdata, dat, dv->length + 1);
		dv->ieeeoui = ieeeoui;
		dv->ver = (dat[pos] >> 5) & 0x7;
		if (dv->ver > 2) {
			dv->block_flag = ERROR_VER;
			return;
		}
		/* Refer to DV 2.9 Page 27 */
		if (dv->ver == 0) {
			if (dv->length == 0x19) {
				dv->sup_yuv422_12bit = dat[pos] & 0x1;
				dv->sup_2160p60hz = (dat[pos] >> 1) & 0x1;
				dv->sup_global_dimming = (dat[pos] >> 2) & 0x1;
				pos++;
				dv->Rx =
					(dat[pos + 1] << 4) | (dat[pos] >> 4);
				dv->Ry =
					(dat[pos + 2] << 4) | (dat[pos] & 0xf);
				pos += 3;
				dv->Gx =
					(dat[pos + 1] << 4) | (dat[pos] >> 4);
				dv->Gy =
					(dat[pos + 2] << 4) | (dat[pos] & 0xf);
				pos += 3;
				dv->Bx =
					(dat[pos + 1] << 4) | (dat[pos] >> 4);
				dv->By =
					(dat[pos + 2] << 4) | (dat[pos] & 0xf);
				pos += 3;
				dv->Wx =
					(dat[pos + 1] << 4) | (dat[pos] >> 4);
				dv->Wy =
					(dat[pos + 2] << 4) | (dat[pos] & 0xf);
				pos += 3;
				dv->tminPQ =
					(dat[pos + 1] << 4) | (dat[pos] >> 4);
				dv->tmaxPQ =
					(dat[pos + 2] << 4) | (dat[pos] & 0xf);
				pos += 3;
				dv->dm_major_ver = dat[pos] >> 4;
				dv->dm_minor_ver = dat[pos] & 0xf;
				pos++;
				pr_debug("v0 VSVDB: len=%d, sup_2160p60hz=%d\n",
					dv->length, dv->sup_2160p60hz);
			} else {
				dv->block_flag = ERROR_LENGTH;
			}
		}

		if (dv->ver == 1) {
			if (dv->length == 0x0B) {/* Refer to DV 2.9 Page 33 */
				dv->dm_version = (dat[pos] >> 2) & 0x7;
				dv->sup_yuv422_12bit = dat[pos] & 0x1;
				dv->sup_2160p60hz = (dat[pos] >> 1) & 0x1;
				pos++;
				dv->sup_global_dimming = dat[pos] & 0x1;
				dv->tmax_lum = dat[pos] >> 1;
				pos++;
				dv->colorimetry = dat[pos] & 0x1;
				dv->tmin_lum = dat[pos] >> 1;
				pos++;
				dv->low_latency = dat[pos] & 0x3;
				dv->Bx = 0x20 | ((dat[pos] >> 5) & 0x7);
				dv->By = 0x08 | ((dat[pos] >> 2) & 0x7);
				pos++;
				dv->Gx = 0x00 | (dat[pos] >> 1);
				dv->Ry = 0x40 | ((dat[pos] & 0x1) |
					((dat[pos + 1] & 0x1) << 1) |
					((dat[pos + 2] & 0x3) << 2));
				pos++;
				dv->Gy = 0x80 | (dat[pos] >> 1);
				pos++;
				dv->Rx = 0xA0 | (dat[pos] >> 3);
				pos++;
				pr_debug("v1 VSVDB: len=%d, sup_2160p60hz=%d, low_latency=%d\n",
					dv->length, dv->sup_2160p60hz, dv->low_latency);
			} else if (dv->length == 0x0E) {
				dv->dm_version = (dat[pos] >> 2) & 0x7;
				dv->sup_yuv422_12bit = dat[pos] & 0x1;
				dv->sup_2160p60hz = (dat[pos] >> 1) & 0x1;
				pos++;
				dv->sup_global_dimming = dat[pos] & 0x1;
				dv->tmax_lum = dat[pos] >> 1;
				pos++;
				dv->colorimetry = dat[pos] & 0x1;
				dv->tmin_lum = dat[pos] >> 1;
				pos += 2; /* byte8 is reserved as 0 */
				dv->Rx = dat[pos++];
				dv->Ry = dat[pos++];
				dv->Gx = dat[pos++];
				dv->Gy = dat[pos++];
				dv->Bx = dat[pos++];
				dv->By = dat[pos++];
				pr_debug("v1 VSVDB: len=%d, sup_2160p60hz=%d\n",
					dv->length, dv->sup_2160p60hz);
			} else {
				dv->block_flag = ERROR_LENGTH;
			}
		}
		if (dv->ver == 2) {
			/* v2 VSVDB length could be greater than 0xB
			 * and should not be treated as unrecognized
			 * block. Instead, we should parse it as a regular
			 * v2 VSVDB using just the remaining 11 bytes here
			 */
			if (dv->length >= 0x0B) {
				dv->sup_2160p60hz = 0x1;/*default*/
				dv->dm_version = (dat[pos] >> 2) & 0x7;
				dv->sup_yuv422_12bit = dat[pos] & 0x1;
				dv->sup_backlight_control = (dat[pos] >> 1) & 0x1;
				pos++;
				dv->sup_global_dimming = (dat[pos] >> 2) & 0x1;
				dv->backlt_min_luma = dat[pos] & 0x3;
				dv->tminPQ = dat[pos] >> 3;
				pos++;
				dv->Interface = dat[pos] & 0x3;
				dv->tmaxPQ = dat[pos] >> 3;
				pos++;
				dv->sup_10b_12b_444 = ((dat[pos] & 0x1) << 1) |
					(dat[pos + 1] & 0x1);
				dv->Gx = 0x00 | (dat[pos] >> 1);
				pos++;
				dv->Gy = 0x80 | (dat[pos] >> 1);
				pos++;
				dv->Rx = 0xA0 | (dat[pos] >> 3);
				dv->Bx = 0x20 | (dat[pos] & 0x7);
				pos++;
				dv->Ry = 0x40  | (dat[pos] >> 3);
				dv->By = 0x08  | (dat[pos] & 0x7);
				pos++;
				pr_debug("v2 VSVDB: len=%d, sup_2160p60hz=%d, Interface=%d\n",
					dv->length, dv->sup_2160p60hz, dv->Interface);
			} else {
				dv->block_flag = ERROR_LENGTH;
			}
		}

		if (pos > (dv->length + 1))
			pr_debug("hdmitx: edid: maybe invalid dv%d data\n", dv->ver);
		return;
	}
	/* future: other new VSVDB add here: */
}

/* hdr_priority = 2, hdr_cap mask dv_info and hdr_info
 * hdr_priority = 1, hdr_cap mask dv_info
 */
static void edid_parsingvendspec(struct hdmitx_dev *hdev,
				 struct rx_cap *prxcap,
				 unsigned char *buf)
{
	struct dv_info *dv = &prxcap->dv_info;
	struct dv_info *dv2 = &prxcap->dv_info2;
	struct hdr10_plus_info *hdr10_plus = &prxcap->hdr_info.hdr10plus_info;
	struct hdr10_plus_info *hdr10_plus2 = &prxcap->hdr_info2.hdr10plus_info;
	struct cuva_info *cuva = &prxcap->hdr_info.cuva_info;
	struct cuva_info *cuva2 = &prxcap->hdr_info2.cuva_info;

	unsigned char pos = 0;
	unsigned int ieeeoui = 0;

	pos++;

	if (buf[pos] != 1) {
		pr_info("hdmitx: edid: parsing fail %s[%d]\n", __func__,
			__LINE__);
		return;
	}

	pos++;
	ieeeoui = buf[pos++];
	ieeeoui += buf[pos++] << 8;
	ieeeoui += buf[pos++] << 16;

	if ((hdev->tx_comm.hdr_priority == 1 && ieeeoui == DV_IEEE_OUI) ||
		hdev->tx_comm.hdr_priority == 2) {
		_edid_parsingvendspec(dv2, hdr10_plus2, cuva2, buf);
		return;
	}

	_edid_parsingvendspec(dv, hdr10_plus, cuva, buf);
	_edid_parsingvendspec(dv2, hdr10_plus2, cuva2, buf);
}

static bool Y420VicRight(unsigned int vic)
{
	bool rtn_val;

	rtn_val = false;
	if (vic == HDMI_3840x2160p60_64x27 ||
	    vic == HDMI_3840x2160p50_64x27 ||
	    vic == HDMI_4096x2160p60_256x135 ||
	    vic == HDMI_4096x2160p50_256x135 ||
	    vic == HDMI_3840x2160p60_16x9 ||
	    vic == HDMI_3840x2160p50_16x9)
		rtn_val = true;
	return rtn_val;
}

/* ----------------------------------------------------------- */
static int edid_parsingy420vdb(struct rx_cap *prxcap,
			       unsigned char *buf)
{
	unsigned char tag = 0, ext_tag = 0, data_end = 0;
	unsigned int pos = 0;

	tag = (buf[pos] >> 5) & 0x7;
	data_end = (buf[pos] & 0x1f) + 1;
	pos++;
	ext_tag = buf[pos];

	if (tag != 0x7 || ext_tag != 0xe)
		goto INVALID_Y420VDB;

	pos++;
	while (pos < data_end) {
		if (prxcap->VIC_count < VIC_MAX_NUM) {
			if (Y420VicRight(buf[pos])) {
				store_cea_idx(prxcap, buf[pos]);
				store_y420_idx(prxcap, buf[pos]);
			}
		}
		pos++;
	}

	return 0;

INVALID_Y420VDB:
	pr_err("[%s] it's not a valid y420vdb!\n", __func__);
	return -1;
}

static int _edid_parsedrmsb(struct hdr_info *info,
			   unsigned char *buf)
{
	unsigned char tag = 0, ext_tag = 0, data_end = 0;
	unsigned int pos = 0;

	tag = (buf[pos] >> 5) & 0x7;
	data_end = (buf[pos] & 0x1f);
	memset(info->rawdata, 0, 7);
	memcpy(info->rawdata, buf, data_end + 1);
	pos++;
	ext_tag = buf[pos];
	if (tag != HDMI_EDID_BLOCK_TYPE_EXTENDED_TAG ||
	    ext_tag != EXTENSION_DRM_STATIC_TAG)
		goto INVALID_DRM_STATIC;
	pos++;
	info->hdr_support = buf[pos];
	pos++;
	info->static_metadata_type1 = buf[pos];
	pos++;
	if (data_end == 3)
		return 0;
	if (data_end == 4) {
		info->lumi_max = buf[pos];
		return 0;
	}
	if (data_end == 5) {
		info->lumi_max = buf[pos];
		info->lumi_avg = buf[pos + 1];
		return 0;
	}
	if (data_end == 6) {
		info->lumi_max = buf[pos];
		info->lumi_avg = buf[pos + 1];
		info->lumi_min = buf[pos + 2];
		return 0;
	}
	return 0;
INVALID_DRM_STATIC:
	pr_err("[%s] it's not a valid DRM STATIC BLOCK\n", __func__);
	return -1;
}

static int edid_parsedrmsb(struct rx_cap *prxcap,
			   unsigned char *buf)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();
	struct hdr_info *hdr = &prxcap->hdr_info;
	struct hdr_info *hdr2 = &prxcap->hdr_info2;

	if (hdev->tx_comm.hdr_priority == 2) {
		_edid_parsedrmsb(hdr2, buf);
		return 0;
	}
	_edid_parsedrmsb(hdr, buf);
	_edid_parsedrmsb(hdr2, buf);
	return 1;
}

static int _edid_parsedrmdb(struct hdr_info *info,
			   unsigned char *buf)
{
	unsigned char tag = 0, ext_tag = 0, data_end = 0;
	unsigned int pos = 0;
	unsigned int type;
	unsigned int type_length;
	unsigned int i;
	unsigned int num;

	tag = (buf[pos] >> 5) & 0x7;
	data_end = (buf[pos] & 0x1f);
	pos++;
	ext_tag = buf[pos];
	if (tag != HDMI_EDID_BLOCK_TYPE_EXTENDED_TAG ||
	    ext_tag != EXTENSION_DRM_DYNAMIC_TAG)
		goto INVALID_DRM_DYNAMIC;
	pos++;
	data_end--;/*extended tag code byte doesn't need*/

	while (data_end) {
		type_length = buf[pos];
		pos++;
		type = (buf[pos + 1] << 8) | buf[pos];
		pos += 2;
		switch (type) {
		case TS_103_433_SPEC_TYPE:
			num = 1;
			break;
		case ITU_T_H265_SPEC_TYPE:
			num = 2;
			break;
		case TYPE_4_HDR_METADATA_TYPE:
			num = 3;
			break;
		case TYPE_1_HDR_METADATA_TYPE:
		default:
			num = 0;
			break;
		}
		info->dynamic_info[num].of_len = type_length;
		info->dynamic_info[num].type = type;
		info->dynamic_info[num].support_flags = buf[pos];
		pos++;
		for (i = 0; i < type_length - 3; i++) {
			info->dynamic_info[num].optional_fields[i] =
			buf[pos];
			pos++;
		}
		data_end = data_end - (type_length + 1);
	}

	return 0;
INVALID_DRM_DYNAMIC:
	pr_err("[%s] it's not a valid DRM DYNAMIC BLOCK\n", __func__);
	return -1;
}

static int edid_parsedrmdb(struct rx_cap *prxcap,
			   unsigned char *buf)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();
	struct hdr_info *hdr = &prxcap->hdr_info;
	struct hdr_info *hdr2 = &prxcap->hdr_info2;

	if (hdev->tx_comm.hdr_priority == 2) {
		_edid_parsedrmdb(hdr2, buf);
		return 0;
	}
	_edid_parsedrmdb(hdr, buf);
	_edid_parsedrmdb(hdr2, buf);
	return 1;
}

static int edid_parsingvfpdb(struct rx_cap *prxcap, unsigned char *buf)
{
	unsigned int len = buf[0] & 0x1f;
	enum hdmi_vic svr = HDMI_UNKNOWN;

	if (buf[1] != EXTENSION_VFPDB_TAG)
		return 0;
	if (len < 2)
		return 0;

	svr = buf[2];
	if ((svr >= 1 && svr <= 127) ||
	    (svr >= 193 && svr <= 253)) {
		prxcap->flag_vfpdb = 1;
		prxcap->preferred_mode = svr;
		pr_debug("preferred mode 0 srv %d\n", prxcap->preferred_mode);
		return 1;
	}
	if (svr >= 129 && svr <= 144) {
		prxcap->flag_vfpdb = 1;
		prxcap->preferred_mode = prxcap->dtd[svr - 129].vic;
		pr_debug("preferred mode 0 dtd %d\n", prxcap->preferred_mode);
		return 1;
	}
	return 0;
}

/* ----------------------------------------------------------- */
static int edid_parsingy420cmdb(struct hdmitx_dev *hdev,
				unsigned char *buf)
{
	unsigned char tag = 0, ext_tag = 0, length = 0, data_end = 0;
	unsigned int pos = 0, i = 0;
	struct hdmitx_info *info = &hdev->hdmi_info;

	tag = (buf[pos] >> 5) & 0x7;
	length = buf[pos] & 0x1f;
	data_end = length + 1;
	pos++;
	ext_tag = buf[pos];

	if (tag != 0x7 || ext_tag != 0xf)
		goto INVALID_Y420CMDB;

	if (length == 1) {
		info->y420_all_vic = 1;
		return 0;
	}

	info->bitmap_length = 0;
	info->bitmap_valid = 0;
	memset(info->y420cmdb_bitmap, 0x00, Y420CMDB_MAX);

	pos++;
	if (pos < data_end) {
		info->bitmap_length = data_end - pos;
		info->bitmap_valid = 1;
	}
	while (pos < data_end) {
		if (i < Y420CMDB_MAX)
			info->y420cmdb_bitmap[i] = buf[pos];
		pos++;
		i++;
	}

	return 0;

INVALID_Y420CMDB:
	pr_err("[%s] it's not a valid y420cmdb!\n", __func__);
	return -1;
}

static void edid_parsingdolbyvsadb(struct hdmitx_dev *hdev,
				unsigned char *buf)
{
	unsigned char length = 0;
	unsigned char pos = 0;
	unsigned int ieeeoui;
	struct dolby_vsadb_cap *cap = &hdev->tx_comm.rxcap.dolby_vsadb_cap;

	memset(cap->rawdata, 0, sizeof(cap->rawdata));
	memcpy(cap->rawdata, buf, 7); /* fixed 7 bytes */

	pos = 0;
	length = buf[pos] & 0x1f;
	if (length != 0x06)
		pr_debug("%s[%d]: the length is %d, should be 6 bytes\n",
			__func__, __LINE__, length);

	cap->length = length;
	pos += 2;
	ieeeoui = buf[pos] + (buf[pos + 1] << 8) + (buf[pos + 2] << 16);
	if (ieeeoui != DOVI_IEEEOUI)
		pr_debug("%s[%d]: the ieeeoui is 0x%x, should be 0x%x\n",
			__func__, __LINE__, ieeeoui, DOVI_IEEEOUI);
	cap->ieeeoui = ieeeoui;

	pos += 3;
	cap->dolby_vsadb_ver = buf[pos] & 0x7;
	if (cap->dolby_vsadb_ver)
		pr_debug("%s[%d]: the version is 0x%x, should be 0x0\n",
			__func__, __LINE__, cap->dolby_vsadb_ver);

	cap->spk_center = (buf[pos] >> 4) & 1;
	cap->spk_surround = (buf[pos] >> 5) & 1;
	cap->spk_height = (buf[pos] >> 6) & 1;
	cap->headphone_only = (buf[pos] >> 7) & 1;

	pos++;
	cap->mat_48k_pcm_only = (buf[pos] >> 0) & 1;
}

static int edid_y420cmdb_fill_all_vic(struct hdmitx_dev *hdmitx_device)
{
	struct rx_cap *rxcap = &hdmitx_device->tx_comm.rxcap;
	struct hdmitx_info *info = &hdmitx_device->hdmi_info;
	unsigned int count = rxcap->VIC_count;
	unsigned int a, b;

	if (info->y420_all_vic != 1)
		return 1;

	a = count / 8;
	a = (a >= Y420CMDB_MAX) ? Y420CMDB_MAX : a;
	b = count % 8;

	if (a > 0)
		memset(&info->y420cmdb_bitmap[0], 0xff, a);

	if (b != 0 && a < Y420CMDB_MAX)
		info->y420cmdb_bitmap[a] = (1 << b) - 1;

	info->bitmap_length = (b == 0) ? a : (a + 1);
	info->bitmap_valid = (info->bitmap_length != 0) ? 1 : 0;

	return 0;
}

static int edid_y420cmdb_postprocess(struct hdmitx_dev *hdmitx_device)
{
	unsigned int i = 0, j = 0, valid = 0;
	struct rx_cap *rxcap = &hdmitx_device->tx_comm.rxcap;
	struct hdmitx_info *info = &hdmitx_device->hdmi_info;
	unsigned char *p = NULL;
	u32 vic;

	if (info->y420_all_vic == 1)
		edid_y420cmdb_fill_all_vic(hdmitx_device);

	if (info->bitmap_valid == 0)
		goto PROCESS_END;

	for (i = 0; i < info->bitmap_length; i++) {
		p = &info->y420cmdb_bitmap[i];
		for (j = 0; j < 8; j++) {
			valid = ((*p >> j) & 0x1);
			vic = rxcap->VIC[i * 8 + j];
			if (valid != 0 &&
			    Y420VicRight(rxcap->VIC[i * 8 + j])) {
				store_cea_idx(rxcap, vic);
				store_y420_idx(rxcap, vic);
			}
		}
	}

PROCESS_END:
	return 0;
}

static void edid_y420cmdb_reset(struct hdmitx_info *info)
{
	info->bitmap_valid = 0;
	info->bitmap_length = 0;
	info->y420_all_vic = 0;
	memset(info->y420cmdb_bitmap, 0x00, Y420CMDB_MAX);
}

/* ----------------------------------------------------------- */
static void hdmitx_3d_update(struct rx_cap *prxcap)
{
	int j = 0;

	for (j = 0; j < 16; j++) {
		if ((prxcap->threeD_MASK_15_0 >> j) & 0x1) {
			/* frame packing */
			if (prxcap->threeD_Structure_ALL_15_0
				& (1 << 0))
				prxcap->support_3d_format[prxcap->VIC[j]]
				.frame_packing = 1;
			/* top and bottom */
			if (prxcap->threeD_Structure_ALL_15_0
				& (1 << 6))
				prxcap->support_3d_format[prxcap->VIC[j]]
				.top_and_bottom = 1;
			/* top and bottom */
			if (prxcap->threeD_Structure_ALL_15_0
				& (1 << 8))
				prxcap->support_3d_format[prxcap->VIC[j]]
				.side_by_side = 1;
		}
	}
}

/* parse Sink 3D information */
static int hdmitx_edid_3d_parse(struct rx_cap *prxcap, unsigned char *dat,
				unsigned int size)
{
	int j = 0;
	unsigned int base = 0;
	unsigned int pos = base + 1;
	unsigned int index = 0;

	if (dat[base] & (1 << 7))
		pos += 2;
	if (dat[base] & (1 << 6))
		pos += 2;
	if (dat[base] & (1 << 5)) {
		prxcap->threeD_present = dat[pos] >> 7;
		prxcap->threeD_Multi_present = (dat[pos] >> 5) & 0x3;
		pos += 1;
		prxcap->hdmi_vic_LEN = dat[pos] >> 5;
		prxcap->HDMI_3D_LEN = dat[pos] & 0x1f;
		pos += prxcap->hdmi_vic_LEN + 1;

		if (prxcap->threeD_Multi_present == 0x01) {
			prxcap->threeD_Structure_ALL_15_0 =
				(dat[pos] << 8) + dat[pos + 1];
			prxcap->threeD_MASK_15_0 = 0;
			pos += 2;
		}
		if (prxcap->threeD_Multi_present == 0x02) {
			prxcap->threeD_Structure_ALL_15_0 =
				(dat[pos] << 8) + dat[pos + 1];
			pos += 2;
			prxcap->threeD_MASK_15_0 =
			(dat[pos] << 8) + dat[pos + 1];
			pos += 2;
		}
	}
	while (pos < size) {
		if ((dat[pos] & 0xf) < 0x8) {
			/* frame packing */
			if ((dat[pos] & 0xf) == T3D_FRAME_PACKING)
				prxcap->support_3d_format[prxcap->VIC[((dat[pos]
					& 0xf0) >> 4)]].frame_packing = 1;
			/* top and bottom */
			if ((dat[pos] & 0xf) == T3D_TAB)
				prxcap->support_3d_format[prxcap->VIC[((dat[pos]
					& 0xf0) >> 4)]].top_and_bottom = 1;
			pos += 1;
		} else {
			/* SidebySide */
			if ((dat[pos] & 0xf) == T3D_SBS_HALF &&
			    (dat[pos + 1] >> 4) < 0xb) {
				index = (dat[pos] & 0xf0) >> 4;
				prxcap->support_3d_format[prxcap->VIC[index]]
				.side_by_side = 1;
			}
			pos += 2;
		}
	}
	if (prxcap->threeD_MASK_15_0 == 0) {
		for (j = 0; (j < 16) && (j < prxcap->VIC_count); j++) {
			prxcap->support_3d_format[prxcap->VIC[j]]
			.frame_packing = 1;
			prxcap->support_3d_format[prxcap->VIC[j]]
			.top_and_bottom = 1;
			prxcap->support_3d_format[prxcap->VIC[j]]
			.side_by_side = 1;
		}
	} else {
		hdmitx_3d_update(prxcap);
	}
	return 1;
}

/* parse Sink 4k2k information */
static void hdmitx_edid_4k2k_parse(struct rx_cap *prxcap, unsigned char *dat,
				   unsigned int size)
{
	if (size > 4 || size == 0) {
		pr_info(EDID
			"4k2k in edid out of range, SIZE = %d\n",
			size);
		return;
	}
	while (size--) {
		if (*dat == 1)
			prxcap->VIC[prxcap->VIC_count] = HDMI_3840x2160p30_16x9;
		else if (*dat == 2)
			prxcap->VIC[prxcap->VIC_count] = HDMI_3840x2160p25_16x9;
		else if (*dat == 3)
			prxcap->VIC[prxcap->VIC_count] = HDMI_3840x2160p24_16x9;
		else if (*dat == 4)
			prxcap->VIC[prxcap->VIC_count] = HDMI_4096x2160p24_256x135;
		else
			;
		dat++;
		prxcap->VIC_count++;
	}
}

static void get_latency(struct rx_cap *prxcap, unsigned char *val)
{
	if (val[0] == 0)
		prxcap->vLatency = LATENCY_INVALID_UNKNOWN;
	else if (val[0] == 0xFF)
		prxcap->vLatency = LATENCY_NOT_SUPPORT;
	else
		prxcap->vLatency = (val[0] - 1) * 2;

	if (val[1] == 0)
		prxcap->aLatency = LATENCY_INVALID_UNKNOWN;
	else if (val[1] == 0xFF)
		prxcap->aLatency = LATENCY_NOT_SUPPORT;
	else
		prxcap->aLatency = (val[1] - 1) * 2;
}

static void get_ilatency(struct rx_cap *prxcap, unsigned char *val)
{
	if (val[0] == 0)
		prxcap->i_vLatency = LATENCY_INVALID_UNKNOWN;
	else if (val[0] == 0xFF)
		prxcap->i_vLatency = LATENCY_NOT_SUPPORT;
	else
		prxcap->i_vLatency = val[0] * 2 - 1;

	if (val[1] == 0)
		prxcap->i_aLatency = LATENCY_INVALID_UNKNOWN;
	else if (val[1] == 0xFF)
		prxcap->i_aLatency = LATENCY_NOT_SUPPORT;
	else
		prxcap->i_aLatency = val[1] * 2 - 1;
}

static void hdmitx_edid_parse_hdmi14(struct rx_cap *prxcap,
				     unsigned char offset,
				     unsigned char *block_buf,
				     unsigned char count)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();
	struct hdmitx_info *info = &hdev->hdmi_info;
	int idx = 0, tmp = 0;

	prxcap->ieeeoui = HDMI_IEEE_OUI;
	set_vsdb_phy_addr(hdev, &info->vsdb_phy_addr, &block_buf[offset + 3]);
	if ((check_fbc_special(&hdev->tx_comm.edid_ptr[0])) ||
	    (check_fbc_special(&hdev->tx_comm.edid_ptr[0])))
		rx_edid_physical_addr(0, 0, 0, 0);
	else
		rx_edid_physical_addr(info->vsdb_phy_addr.a,
				      info->vsdb_phy_addr.b,
				      info->vsdb_phy_addr.c,
				      info->vsdb_phy_addr.d);

	prxcap->ColorDeepSupport =
	(count > 5) ? block_buf[offset + 5] : 0;
	set_vsdb_dc_cap(prxcap);
	prxcap->Max_TMDS_Clock1 =
		(count > 6) ? block_buf[offset + 6] : 0;
	if (count > 7) {
		tmp = block_buf[offset + 7];
		idx = offset + 8;
		if (tmp & (1 << 6)) {
			unsigned char val[2];

			val[0] = block_buf[idx];
			val[1] = block_buf[idx + 1];
			get_latency(prxcap, val);
			idx += 2;
		}
		if (tmp & (1 << 7)) {
			unsigned char val[2];

			val[0] = block_buf[idx];
			val[1] = block_buf[idx + 1];
			get_ilatency(prxcap, val);
			idx += 2;
		}
		prxcap->cnc0 = (tmp >> 0) & 1;
		prxcap->cnc1 = (tmp >> 1) & 1;
		prxcap->cnc2 = (tmp >> 2) & 1;
		prxcap->cnc3 = (tmp >> 3) & 1;
		if (tmp & (1 << 5)) {
			idx += 1;
			/* valid 4k */
			if (block_buf[idx] & 0xe0) {
				hdmitx_edid_4k2k_parse(prxcap,
						       &block_buf[idx + 1],
				block_buf[idx] >> 5);
			}
			/* valid 3D */
			if (block_buf[idx - 1] & 0xe0) {
				hdmitx_edid_3d_parse(prxcap,
						     &block_buf[offset + 7],
				count - 7);
			}
		}
	}
}

/* force_vsvdb */
/*  0: no force, use TV's */
/*  1~n: use preset vsvdb 0~n-1 */
/*  255: use current vsvdb_data */
/*       update by module param vsvdb_data */
static unsigned int force_vsvdb;
static unsigned int vsvdb_size = 12;
static unsigned char vsvdb_data[32] = {
	0xeb, 0x01, 0x46, 0xd0, 0x00, 0x45, 0x0b, 0x90,
	0x86, 0x60, 0x76, 0x8f, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

#define PRESET_VSVDB_COUNT 4
static unsigned char tv_vsvdb[PRESET_VSVDB_COUNT][32] = {
	/* source-led */
	{
		0xeb, 0x01, 0x46, 0xd0,
		0x00, 0x45, 0x0b, 0x90,
		0x86, 0x60, 0x76, 0x8f
	},
	/* sink-led */
	{
		0xee, 0x01, 0x46, 0xd0,
		0x00, 0x24, 0x0f, 0x8b,
		0xa8, 0x53, 0x4b, 0x9d,
		0x27, 0x0b, 0x00
	},
	/* sink-led & source-led */
	{
		0xeb, 0x01, 0x46, 0xd0,
		0x00, 0x44, 0x4f, 0x42,
		0x8c, 0x46, 0x56, 0x8e
	},
	/* hdr10+ */
	{
		0xe5, 0x01, 0x8b, 0x84,
		0x90, 0x01
	}
};

module_param(force_vsvdb, uint, 0664);
MODULE_PARM_DESC(force_vsvdb, "\n force_vsvdb\n");
module_param_array(vsvdb_data, byte, &vsvdb_size, 0664);
MODULE_PARM_DESC(vsvdb_data, "\n vsvdb data\n");

/* force_hdr */
/*  0: no force, use TV's */
/*  1~n: use preset drm 0~n-1 */
/*  255: use current drm_data */
/*       update by module param drm_data */
static unsigned int force_hdr;
static unsigned int drm_size = 4;
static unsigned char drm_data[8] = {
	0xe3, 0x06, 0x0d, 0x01, 0x00, 0x00, 0x00, 0x00
};

#define PRESET_DRM_COUNT 3
static unsigned char tv_drm[PRESET_DRM_COUNT][32] = {
	/* hdr10 + hlg */
	{
		0xe3, 0x06, 0x0d, 0x01
	},
	/* hdr10 */
	{
		0xe3, 0x06, 0x05, 0x01
	},
	/* hlg */
	{
		0xe3, 0x06, 0x09, 0x01
	}
};

module_param(force_hdr, uint, 0664);
MODULE_PARM_DESC(force_hdr, "\n force_drm\n");
module_param_array(drm_data, byte, &drm_size, 0664);
MODULE_PARM_DESC(drm_data, "\n drm data\n");

static void hdmitx_parse_sink_capability(struct rx_cap *prxcap,
	unsigned char offset, unsigned char *block_buf,
	unsigned char count)
{
	prxcap->hf_ieeeoui = HF_IEEEOUI;
	prxcap->Max_TMDS_Clock2 = block_buf[offset + 4];
	prxcap->scdc_present =
		!!(block_buf[offset + 5] & (1 << 7));
	prxcap->scdc_rr_capable =
		!!(block_buf[offset + 5] & (1 << 6));
	prxcap->lte_340mcsc_scramble =
		!!(block_buf[offset + 5] & (1 << 3));
	set_vsdb_dc_420_cap(prxcap, &block_buf[offset]);
	if (count > 7) {
		unsigned char b7 = block_buf[offset + 7];

		prxcap->allm = !!(b7 & (1 << 1));
	}
}

static int hdmitx_edid_cta_block_parse(struct hdmitx_dev *hdev,
				   unsigned char *block_buf)
{
	unsigned char offset, end;
	unsigned char count;
	unsigned char tag;
	int i, tmp, idx;
	unsigned char *vfpdb_offset = NULL;
	struct rx_cap *prxcap = &hdev->tx_comm.rxcap;
	unsigned int aud_flag = 0;

	/* CEA-861 implementations are required to use Tag = 0x02
	 * for the CEA Extension Tag and Sources should ignore
	 * Tags that are not understood. but for Samsung LA32D400E1
	 * its extension tag is 0x0 while other bytes normal,
	 * so continue parse as other sources do
	 */
	if (block_buf[0] == 0x0) {
		pr_info(EDID "unknown Extension Tag detected, continue\n");
	} else if (block_buf[0] != 0x02) {
		pr_info("skip the block of tag: 0x%02x%02x", block_buf[0], block_buf[1]);
		return -1; /* not a CEA BLOCK. */
	}
	end = block_buf[2]; /* CEA description. */
	prxcap->native_Mode = block_buf[1] >= 2 ? block_buf[3] : 0;
	prxcap->number_of_dtd += block_buf[1] >= 2 ? (block_buf[3] & 0xf) : 0;

	/*prxcap->native_VIC = 0xff;*/
	/*prxcap->AUD_count = 0;*/

	edid_y420cmdb_reset(&hdev->hdmi_info);
	if (end > 126)
		return 0;

	if (block_buf[1] <= 2) {
		/* skip below for loop */
		goto next;
	}
	/* this loop should be parsing when revision number is larger than 2 */
	for (offset = 4 ; offset < end ; ) {
		tag = block_buf[offset] >> 5;
		count = block_buf[offset] & 0x1f;
		switch (tag) {
		case HDMI_EDID_BLOCK_TYPE_AUDIO:
			aud_flag = 1;
			tmp = count / 3;
			idx = prxcap->AUD_count;
			prxcap->AUD_count += tmp;
			offset++;
			for (i = 0 ; i < tmp; i++) {
				prxcap->RxAudioCap[idx + i].audio_format_code =
					(block_buf[offset + i * 3] >> 3) & 0xf;
				prxcap->RxAudioCap[idx + i].channel_num_max =
					block_buf[offset + i * 3] & 0x7;
				prxcap->RxAudioCap[idx + i].freq_cc =
					block_buf[offset + i * 3 + 1] & 0x7f;
				prxcap->RxAudioCap[idx + i].cc3 =
					block_buf[offset + i * 3 + 2];
			}
			offset += count;
			break;

		case HDMI_EDID_BLOCK_TYPE_VIDEO:
			offset++;
			for (i = 0 ; i < count ; i++) {
				unsigned char VIC;
				/* 7.5.1 Video Data Block Table 58
				 * and CTA-861-G page101: only 1~64
				 * maybe Native Video Format. and
				 * need to take care hdmi2.1 VIC:
				 * 193~253
				 */
				VIC = block_buf[offset + i];
				if (VIC >= 129 && VIC <= 192) {
					VIC &= (~0x80);
					prxcap->native_vic = VIC;
				}
				prxcap->VIC[prxcap->VIC_count] = VIC;
				prxcap->VIC_count++;
			}
			offset += count;
			break;

		case HDMI_EDID_BLOCK_TYPE_VENDER:
			offset++;
			if (block_buf[offset] == 0x03 &&
			    block_buf[offset + 1] == 0x0c &&
			    block_buf[offset + 2] == 0x00) {
				hdmitx_edid_parse_hdmi14(prxcap, offset,
							 block_buf, count);
			} else if ((block_buf[offset] == 0xd8) &&
				(block_buf[offset + 1] == 0x5d) &&
				(block_buf[offset + 2] == 0xc4))
				hdmitx_parse_sink_capability(prxcap,
					offset, block_buf, count);
			offset += count; /* ignore the remind. */
			break;

		case HDMI_EDID_BLOCK_TYPE_SPEAKER:
			offset++;
			prxcap->RxSpeakerAllocation = block_buf[offset];
			offset += count;
			break;

		case HDMI_EDID_BLOCK_TYPE_VESA:
			offset++;
			offset += count;
			break;

		case HDMI_EDID_BLOCK_TYPE_EXTENDED_TAG:
			{
				unsigned char ext_tag = 0;

				ext_tag = block_buf[offset + 1];
				switch (ext_tag) {
				case EXTENSION_VENDOR_SPECIFIC:
					edid_parsingvendspec(hdev, prxcap,
							     &block_buf[offset]);
					break;
				case EXTENSION_COLORMETRY_TAG:
					prxcap->colorimetry_data =
						block_buf[offset + 2];
					prxcap->colorimetry_data2 =
						block_buf[offset + 2];
					break;
				case EXTENSION_DRM_STATIC_TAG:
					edid_parsedrmsb(prxcap,
							&block_buf[offset]);
					rx_set_hdr_lumi(&block_buf[offset],
							(block_buf[offset] &
							 0x1f) + 1);
					break;
				case EXTENSION_DRM_DYNAMIC_TAG:
					edid_parsedrmdb(prxcap,
							&block_buf[offset]);
					break;
				case EXTENSION_VFPDB_TAG:
/* Just record VFPDB offset address, call Edid_ParsingVFPDB() after DTD
 * parsing, in case that
 * SVR >=129 and SVR <=144, Interpret as the Kth DTD in the EDID,
 * where K = SVR – 128 (for K=1 to 16)
 */
					vfpdb_offset = &block_buf[offset];
					break;
				case EXTENSION_Y420_VDB_TAG:
					edid_parsingy420vdb(prxcap,
							    &block_buf[offset]);
					break;
				case EXTENSION_Y420_CMDB_TAG:
					edid_parsingy420cmdb(hdev,
							     &block_buf[offset]);
					break;
				case EXTENSION_Y420_SCDB_TAG:
					hdmitx_parse_sink_capability(prxcap,
						offset + 1, block_buf, count);
					break;
				case EXTENSION_DOLBY_VSADB:
					edid_parsingdolbyvsadb(hdev,
							     &block_buf[offset]);
					break;
				default:
					break;
				}
			}
			offset += count + 1;
			break;

		case HDMI_EDID_BLOCK_TYPE_RESERVED:
			offset++;
			offset += count;
			break;

		case HDMI_EDID_BLOCK_TYPE_RESERVED2:
			offset++;
			offset += count;
			break;

		default:
			break;
		}
	}
next:
	if (force_vsvdb) {
		if (force_vsvdb <= PRESET_VSVDB_COUNT) {
			vsvdb_size = (tv_vsvdb[force_vsvdb - 1][0] & 0x1f) + 1;
			memcpy(vsvdb_data, tv_vsvdb[force_vsvdb - 1],
				vsvdb_size);
		}
		edid_parsingvendspec(hdev, prxcap, vsvdb_data);
	}
	if (force_hdr) {
		if (force_hdr <= PRESET_DRM_COUNT) {
			drm_size = (tv_drm[force_hdr - 1][0] & 0x1f) + 1;
			memcpy(drm_data, tv_drm[force_hdr - 1],
				drm_size);
		}
		edid_parsedrmsb(prxcap, drm_data);
	}

	if (aud_flag == 0)
		hdmitx_edid_set_default_aud(hdev);

	edid_y420cmdb_postprocess(hdev);
	hdev->vic_count = prxcap->VIC_count;

	/* dtds in extended blocks */
	i = 0;
	offset = block_buf[2] + i * 18;
	for ( ; (offset + 18) < 0x7f; i++) {
		edid_dtd_parsing(prxcap, &block_buf[offset]);
		offset += 18;
	}

	if (vfpdb_offset)
		edid_parsingvfpdb(prxcap, vfpdb_offset);

	return 0;
}

static void hdmitx_edid_set_default_aud(struct hdmitx_dev *hdev)
{
	struct rx_cap *prxcap = &hdev->tx_comm.rxcap;

	/* if AUD_count not equal to 0, no need default value */
	if (prxcap->AUD_count)
		return;

	prxcap->AUD_count = 1;
	prxcap->RxAudioCap[0].audio_format_code = 1; /* PCM */
	prxcap->RxAudioCap[0].channel_num_max = 1; /* 2ch */
	prxcap->RxAudioCap[0].freq_cc = 7; /* 32/44.1/48 kHz */
	prxcap->RxAudioCap[0].cc3 = 1; /* 16bit */
}

/* add default VICs for all zeroes case */
static void hdmitx_edid_set_default_vic(struct hdmitx_dev *hdmitx_device)
{
	struct rx_cap *prxcap = &hdmitx_device->tx_comm.rxcap;

	prxcap->VIC_count = 0x4;
	prxcap->VIC[0] = HDMI_720x480p60_16x9;
	prxcap->VIC[1] = HDMI_1280x720p60_16x9;
	prxcap->VIC[2] = HDMI_1920x1080i60_16x9;
	prxcap->VIC[3] = HDMI_1920x1080p60_16x9;
	prxcap->native_vic = HDMI_720x480p60_16x9;
	hdmitx_device->vic_count = prxcap->VIC_count;
	pr_debug(EDID "set default vic\n");
}

#define PRINT_HASH(hash)

static int edid_hash_calc(unsigned char *hash, const char *data,
			  unsigned int len)
{
	return 1;
}

static int hdmitx_edid_search_IEEEOUI(char *buf)
{
	int i;

	for (i = 0; i < 0x180 - 2; i++) {
		if (buf[i] == 0x03 && buf[i + 1] == 0x0c &&
		    buf[i + 2] == 0x00)
			return 1;
	}
	return 0;
}

/* check EDID strictly */
static int edid_check_valid(unsigned char *buf)
{
	unsigned int chksum = 0;
	unsigned int i = 0;

	/* check block 0 first 8 bytes */
	if (buf[0] != 0 && buf[7] != 0)
		return 0;
	for (i = 1; i < 7; i++) {
		if (buf[i] != 0xff)
			return 0;
	}

	/* check block 0 checksum */
	for (chksum = 0, i = 0; i < 0x80; i++)
		chksum += buf[i];

	if ((chksum & 0xff) != 0)
		return 0;

	/* check Extension flag at block 0 */
	/* for DVI: there may be >= 0 cta block,
	 * so it's normal to have only basic block
	 */
	if (buf[0x7e] == 0)
		return 1;

	/* check block 1 extension tag */
	if (!(buf[0x80] == 0x2 || buf[0x80] == 0xf0))
		return 0;

	/* check block 1 checksum */
	for (chksum = 0, i = 0x80; i < 0x100; i++)
		chksum += buf[i];

	if ((chksum & 0xff) != 0)
		return 0;

	return 1;
}

int check_hdmi_edid_sub_block_valid(unsigned char *buf, int blk_no)
{
	if (!buf)
		return 0;

	/* check block 0 */
	if (blk_no == 0)
		return _check_base_structure(buf);

	/* check block n */
	if (buf[0] == 0)
		return 0;
	return _check_edid_blk_chksum(buf);
}

static void edid_manufacture_date_parse(struct rx_cap *prxcap,
				      unsigned char *data)
{
	if (!data)
		return;

	/* week:
	 *	0: not specified
	 *	0x1~0x36: valid week
	 *	0x37~0xfe: reserved
	 *	0xff: model year is specified
	 */
	if (data[0] == 0 || (data[0] >= 0x37 && data[0] <= 0xfe))
		prxcap->manufacture_week = 0;
	else
		prxcap->manufacture_week = data[0];

	/* year:
	 *	0x0~0xf: reserved
	 *	0x10~0xff: year of manufacture,
	 *		or model year(if specified by week=0xff)
	 */
	prxcap->manufacture_year =
		(data[1] <= 0xf) ? 0 : data[1];
}

static void edid_version_parse(struct rx_cap *prxcap,
			      unsigned char *data)
{
	if (!data)
		return;

	/*
	 *	0x1: edid version 1
	 *	0x0,0x2~0xff: reserved
	 */
	prxcap->edid_version = (data[0] == 0x1) ? 1 : 0;

	/*
	 *	0x0~0x4: revision number
	 *	0x5~0xff: reserved
	 */
	prxcap->edid_revision = (data[1] < 0x5) ? data[1] : 0;
}

static void Edid_Physical_Size_Parse(struct rx_cap *prxcap,
				   unsigned char *data)
{
	if (data[0] != 0 && data[1] != 0) {
		/* Here the unit is cm, transfer to mm */
		prxcap->physical_width  = data[0] * 10;
		prxcap->physical_height = data[1] * 10;
	}
}

/* if edid block 0 are all zeros, then consider RX as HDMI device */
static int edid_zero_data(unsigned char *buf)
{
	int sum = 0;
	int i = 0;

	for (i = 0; i < 128; i++)
		sum += buf[i];

	if (sum == 0)
		return 1;
	else
		return 0;
}

static void dump_dtd_info(struct dtd *t)
{
	if (0) {
		pr_debug(EDID "%s[%d]\n", __func__, __LINE__);
		pr_debug(EDID "pixel_clock: %d\n", t->pixel_clock);
		pr_debug(EDID "h_active: %d\n", t->h_active);
		pr_debug(EDID "v_active: %d\n", t->v_active);
		pr_debug(EDID "v_blank: %d\n", t->v_blank);
		pr_debug(EDID "h_sync_offset: %d\n", t->h_sync_offset);
		pr_debug(EDID "h_sync: %d\n", t->h_sync);
		pr_debug(EDID "v_sync_offset: %d\n", t->v_sync_offset);
		pr_debug(EDID "v_sync: %d\n", t->v_sync);
	}
}

static void edid_dtd_parsing(struct rx_cap *prxcap, unsigned char *data)
{
	const struct hdmi_timing *timing = NULL;
	struct dtd *t = &prxcap->dtd[prxcap->dtd_idx];

	/* if data[0-2] are zeroes, no need parse, and skip*/
	if (data[0] == 0 && data[1] == 0 && data[2] == 0)
		return;
	memset(t, 0, sizeof(struct dtd));
	t->pixel_clock = data[0] + (data[1] << 8);
	t->h_active = (((data[4] >> 4) & 0xf) << 8) + data[2];
	t->h_blank = ((data[4] & 0xf) << 8) + data[3];
	t->v_active = (((data[7] >> 4) & 0xf) << 8) + data[5];
	t->v_blank = ((data[7] & 0xf) << 8) + data[6];
	t->h_sync_offset = (((data[11] >> 6) & 0x3) << 8) + data[8];
	t->h_sync = (((data[11] >> 4) & 0x3) << 8) + data[9];
	t->v_sync_offset = (((data[11] >> 2) & 0x3) << 4) +
		((data[10] >> 4) & 0xf);
	t->v_sync = (((data[11] >> 0) & 0x3) << 4) + ((data[10] >> 0) & 0xf);
	t->h_image_size = (((data[14] >> 4) & 0xf) << 8) + data[12];
	t->v_image_size = ((data[14] & 0xf) << 8) + data[13];
	t->flags = data[17];
/*
 * Special handling of 1080i60hz, 1080i50hz
 */
	if (t->pixel_clock == 7425 && t->h_active == 1920 &&
	    t->v_active == 1080) {
		t->v_active = t->v_active / 2;
		t->v_blank = t->v_blank / 2;
	}
/*
 * Special handling of 480i60hz, 576i50hz
 */
	if ((((t->flags >> 1) & 0x3) == 0) && t->h_active == 1440) {
		if (t->pixel_clock == 2700) /* 576i50hz */
			goto next;
		if ((t->pixel_clock - 2700) < 10) /* 480i60hz */
			t->pixel_clock = 2702;
next:
		t->v_active = t->v_active / 2;
		t->v_blank = t->v_blank / 2;
	}
/*
 * call hdmitx_mode_match_dtd_timing() to check t is matched with VIC
 */
	timing = hdmitx_mode_match_dtd_timing(t);
	if (timing->vic != HDMI_0_UNKNOWN) {
		/* diff 4x3 and 16x9 mode */
		if (timing->vic == HDMI_720x480i60_4x3 ||
			timing->vic == HDMI_720x480p60_4x3 ||
			timing->vic == HDMI_720x576i50_4x3 ||
			timing->vic == HDMI_720x576p50_4x3) {
			if (abs(t->v_image_size * 100 / t->h_image_size - 3 * 100 / 4) <= 2)
				t->vic = timing->vic;
			else
				t->vic = timing->vic + 1;
		} else {
			t->vic = timing->vic;
		}
		prxcap->preferred_mode = prxcap->dtd[0].vic; /* Select dtd0 */
		pr_debug(EDID "get dtd%d vic: %d\n",
			prxcap->dtd_idx, t->vic);
		prxcap->dtd_idx++;
		if (t->vic < HDMITX_VESA_OFFSET)
			store_cea_idx(prxcap, t->vic);
		else
			store_vesa_idx(prxcap, t->vic);
	} else {
		dump_dtd_info(t);
	}
}

static void edid_check_pcm_declare(struct rx_cap *prxcap)
{
	int idx_pcm = 0;
	int i;

	if (!prxcap->AUD_count)
		return;

	/* Try to find more than 1 PCMs, RxAudioCap[0] is always basic audio */
	for (i = 1; i < prxcap->AUD_count; i++) {
		if (prxcap->RxAudioCap[i].audio_format_code ==
			prxcap->RxAudioCap[0].audio_format_code) {
			idx_pcm = i;
			break;
		}
	}

	/* Remove basic audio */
	if (idx_pcm) {
		for (i = 0; i < prxcap->AUD_count - 1; i++)
			memcpy(&prxcap->RxAudioCap[i],
			       &prxcap->RxAudioCap[i + 1],
			       sizeof(struct rx_audiocap));
		/* Clear the last audio declaration */
		memset(&prxcap->RxAudioCap[i], 0, sizeof(struct rx_audiocap));
		prxcap->AUD_count--;
	}
}

static bool is_4k60_supported(struct rx_cap *prxcap)
{
	int i = 0;

	if (!prxcap)
		return false;

	for (i = 0; (i < prxcap->VIC_count) && (i < VIC_MAX_NUM); i++) {
		if (((prxcap->VIC[i] & 0xff) == HDMI_3840x2160p50_16x9) ||
		    ((prxcap->VIC[i] & 0xff) == HDMI_3840x2160p60_16x9)) {
			return true;
		}
	}
	return false;
}

static void edid_descriptor_pmt(struct rx_cap *prxcap,
				struct vesa_standard_timing *t,
				unsigned char *data)
{
	const struct hdmi_timing *timing = NULL;

	t->tmds_clk = data[0] + (data[1] << 8);
	t->hactive = data[2] + (((data[4] >> 4) & 0xf) << 8);
	t->hblank = data[3] + ((data[4] & 0xf) << 8);
	t->vactive = data[5] + (((data[7] >> 4) & 0xf) << 8);
	t->vblank = data[6] + ((data[7] & 0xf) << 8);

	timing = hdmitx_mode_match_vesa_timing(t);
	if (timing && (timing->vic < (HDMI_3840x2160p60_64x27 + 1))) {
		prxcap->native_vic = timing->vic;
		pr_debug("hdmitx: get PMT vic: %d\n", timing->vic);
	}
	if (timing && timing->vic >= HDMITX_VESA_OFFSET)
		store_vesa_idx(prxcap, timing->vic);
}

static void edid_descriptor_pmt2(struct rx_cap *prxcap,
				 struct vesa_standard_timing *t,
				 unsigned char *data)
{
	const struct hdmi_timing *timing = NULL;

	t->tmds_clk = data[0] + (data[1] << 8);
	t->hactive = data[2] + (((data[4] >> 4) & 0xf) << 8);
	t->hblank = data[3] + ((data[4] & 0xf) << 8);
	t->vactive = data[5] + (((data[7] >> 4) & 0xf) << 8);
	t->vblank = data[6] + ((data[7] & 0xf) << 8);

	timing = hdmitx_mode_match_vesa_timing(t);
	if (timing && timing->vic >= HDMITX_VESA_OFFSET)
		store_vesa_idx(prxcap, timing->vic);
}

static void edid_cvt_timing_3bytes(struct rx_cap *prxcap,
				   struct vesa_standard_timing *t,
				   const unsigned char *data)
{
	const struct hdmi_timing *timing = NULL;

	t->hactive = ((data[0] + (((data[1] >> 4) & 0xf) << 8)) + 1) * 2;
	switch ((data[1] >> 2) & 0x3) {
	case 0:
		t->vactive = t->hactive * 3 / 4;
		break;
	case 1:
		t->vactive = t->hactive * 9 / 16;
		break;
	case 2:
		t->vactive = t->hactive * 5 / 8;
		break;
	case 3:
	default:
		t->vactive = t->hactive * 3 / 5;
		break;
	}
	switch ((data[2] >> 5) & 0x3) {
	case 0:
		t->vsync = 50;
		break;
	case 1:
		t->vsync = 60;
		break;
	case 2:
		t->vsync = 75;
		break;
	case 3:
	default:
		t->vsync = 85;
		break;
	}
	timing = hdmitx_mode_match_vesa_timing(t);
	if (timing->vic != HDMI_0_UNKNOWN)
		t->vesa_timing = timing->vic;
}

static void edid_cvt_timing(struct rx_cap *prxcap, unsigned char *data)
{
	int i;
	struct vesa_standard_timing t;

	for (i = 0; i < 4; i++) {
		memset(&t, 0, sizeof(struct vesa_standard_timing));
		edid_cvt_timing_3bytes(prxcap, &t, &data[i * 3]);
		if (t.vesa_timing)
			store_vesa_idx(prxcap, t.vesa_timing);
	}
}

static void check_dv_truly_support(struct hdmitx_dev *hdev, struct dv_info *dv)
{
	struct rx_cap *prxcap = &hdev->tx_comm.rxcap;
	unsigned int max_tmds_clk = 0;

	if (dv->ieeeoui == DV_IEEE_OUI && dv->ver <= 2) {
		/* check max tmds rate to determine if 4k60 DV can truly be
		 * supported.
		 */
		if (prxcap->Max_TMDS_Clock2) {
			max_tmds_clk = prxcap->Max_TMDS_Clock2 * 5;
		} else {
			/* Default min is 74.25 / 5 */
			if (prxcap->Max_TMDS_Clock1 < 0xf)
				prxcap->Max_TMDS_Clock1 = 0x1e;
			max_tmds_clk = prxcap->Max_TMDS_Clock1 * 5;
		}
		if (dv->ver == 0)
			dv->sup_2160p60hz = dv->sup_2160p60hz &&
						(max_tmds_clk >= 594);

		if (dv->ver == 1 && dv->length == 0xB) {
			if (dv->low_latency == 0x00) {
				/*standard mode */
				dv->sup_2160p60hz = dv->sup_2160p60hz &&
							(max_tmds_clk >= 594);
			} else if (dv->low_latency == 0x01) {
				/* both standard and LL are supported. 4k60 LL
				 * DV support should/can be determined using
				 * video formats supported inthe E-EDID as flag
				 * sup_2160p60hz might not be set.
				 */
				if ((dv->sup_2160p60hz ||
				     is_4k60_supported(prxcap)) &&
				    max_tmds_clk >= 594)
					dv->sup_2160p60hz = 1;
				else
					dv->sup_2160p60hz = 0;
			}
		}

		if (dv->ver == 1 && dv->length == 0xE)
			dv->sup_2160p60hz = dv->sup_2160p60hz &&
						(max_tmds_clk >= 594);

		if (dv->ver == 2) {
			/* 4k60 DV support should be determined using video
			 * formats supported in the EEDID as flag sup_2160p60hz
			 * is not applicable for VSVDB V2.
			 */
			if (is_4k60_supported(prxcap) && max_tmds_clk >= 594)
				dv->sup_2160p60hz = 1;
			else
				dv->sup_2160p60hz = 0;
		}
	}
}

int hdmitx_find_philips(struct hdmitx_dev *hdev)
{
	int j;
	int length = sizeof(vendor_id) / sizeof(struct edid_venddat_t);

	for (j = 0; j < length; j++) {
		if (memcmp(&hdev->tx_comm.EDID_buf[8], &vendor_id[j],
			sizeof(struct edid_venddat_t)) == 0)
			return 1;
	}
	return 0;
}

/*
 * if the EDID is invalid, then set the fallback mode
 * Resolution & RefreshRate:
 *   1920x1080p60hz 16:9
 *   1280x720p60hz 16:9 (default)
 *   720x480p 16:9
 * ColorSpace: RGB
 * ColorDepth: 8bit
 */
static void edid_set_fallback_mode(struct hdmitx_dev *hdev)
{
	struct rx_cap *prxcap;
	struct vsdb_phyaddr *phyaddr;

	if (!hdev)
		return;

	phyaddr = &hdev->hdmi_info.vsdb_phy_addr;

	/* EDID extended blk chk error, set the 720p60, rgb,8bit */
	prxcap = &hdev->tx_comm.rxcap;
	prxcap->ieeeoui = HDMI_IEEE_OUI;

	/* set the default cec physical address as 0xffff */
	phyaddr->a = 0xf;
	phyaddr->b = 0xf;
	phyaddr->c = 0xf;
	phyaddr->d = 0xf;
	phyaddr->valid = 0;
	hdev->physical_addr = 0xffff;

	prxcap->Max_TMDS_Clock1 = 0x1e; /* 150MHZ / 5 */
	prxcap->native_Mode = 0; /* only RGB */
	prxcap->dc_y444 = 0; /* only 8bit */
	prxcap->VIC_count = 0x3;
	prxcap->VIC[0] = HDMI_1920x1080p60_16x9;
	prxcap->VIC[1] = HDMI_1280x720p60_16x9;
	prxcap->VIC[2] = HDMI_720x480p60_16x9;
	prxcap->native_vic = HDMI_1280x720p60_16x9;
	hdev->vic_count = prxcap->VIC_count;
	pr_info(EDID "set default vic 720p60hz\n");

	hdmitx_edid_set_default_aud(hdev);
}

static void _edid_parse_base_structure(struct hdmitx_dev *hdmitx_device,
	unsigned char *EDID_buf)
{
	unsigned char checksum;
	unsigned char zero_numbers;
	unsigned char cta_block_count;
	int i;
	struct rx_cap *prxcap = &hdmitx_device->tx_comm.rxcap;

	edid_parsing_id_manufacturer_name(prxcap, &EDID_buf[8]);
	edid_parsing_id_product_code(prxcap, &EDID_buf[0x0A]);
	edid_parsing_id_serial_number(prxcap, &EDID_buf[0x0C]);

	edid_established_timings(prxcap, &EDID_buf[0x23]);

	edid_manufacture_date_parse(prxcap, &EDID_buf[16]);

	edid_version_parse(prxcap, &EDID_buf[18]);

	Edid_Physical_Size_Parse(prxcap, &EDID_buf[21]);
	prxcap->blk0_chksum = EDID_buf[0x7F];

	cta_block_count = EDID_buf[0x7E];

	if (cta_block_count == 0) {
		pr_info(EDID "EDID BlockCount=0\n");
		/* DVI case judgement: only contains one block and
		 * checksum valid
		 */
		checksum = 0;
		zero_numbers = 0;
		for (i = 0; i < 128; i++) {
			checksum += EDID_buf[i];
			if (EDID_buf[i] == 0)
				zero_numbers++;
		}
		pr_info(EDID "edid blk0 checksum:%d ext_flag:%d\n",
			checksum, EDID_buf[0x7e]);
		if ((checksum & 0xff) == 0)
			prxcap->ieeeoui = 0;
		else
			prxcap->ieeeoui = HDMI_IEEE_OUI;
		if (zero_numbers > 120)
			prxcap->ieeeoui = HDMI_IEEE_OUI;
		hdmitx_edid_set_default_vic(hdmitx_device);
	}
}

int hdmitx_edid_parse(struct hdmitx_dev *hdmitx_device)
{
	unsigned char cta_block_count;
	unsigned char *EDID_buf;
	int i;
	int idx[4];
	struct rx_cap *prxcap = &hdmitx_device->tx_comm.rxcap;
	struct dv_info *dv = &hdmitx_device->tx_comm.rxcap.dv_info;

	if (check_dvi_hdmi_edid_valid(hdmitx_device->tx_comm.EDID_buf)) {
		EDID_buf = hdmitx_device->tx_comm.EDID_buf;
		hdmitx_device->tx_comm.edid_parsing = 1;
		memcpy(hdmitx_device->tx_comm.EDID_buf1, hdmitx_device->tx_comm.EDID_buf,
		       EDID_MAX_BLOCK * 128);
	} else {
		EDID_buf = hdmitx_device->tx_comm.EDID_buf1;
	}
	if (check_dvi_hdmi_edid_valid(hdmitx_device->tx_comm.EDID_buf1))
		hdmitx_device->tx_comm.edid_parsing = 1;
	prxcap->head_err = hdmitx_edid_header_invalid(&EDID_buf[0]);
	if (prxcap->head_err)
		hdmitx_current_status(HDMITX_EDID_HEAD_ERROR);
	prxcap->chksum_err = !edid_check_valid(&EDID_buf[0]);
	if (prxcap->chksum_err)
		hdmitx_current_status(HDMITX_EDID_CHECKSUM_ERROR);

	hdmitx_device->tx_comm.edid_ptr = EDID_buf;
	pr_debug(EDID "EDID Parser:\n");
	/* Calculate the EDID hash for special use */
	memset(hdmitx_device->tx_comm.EDID_hash, 0,
	       ARRAY_SIZE(hdmitx_device->tx_comm.EDID_hash));
	edid_hash_calc(hdmitx_device->tx_comm.EDID_hash, hdmitx_device->tx_comm.EDID_buf, 256);

	if (check_dvi_hdmi_edid_valid(EDID_buf) == 0) {
		edid_set_fallback_mode(hdmitx_device);
		pr_info("set fallback mode\n");
		return 0;
	}
	if (_check_base_structure(EDID_buf))
		_edid_parse_base_structure(hdmitx_device, EDID_buf);

	cta_block_count = EDID_buf[0x7E];
	/* HF-EEODB */
	if (cta_block_count == 1 && EDID_buf[128 + 4] == 0xe2 &&
		EDID_buf[128 + 5] == 0x78)
		cta_block_count = EDID_buf[128 + 6];
	/* limit cta_block_count to EDID_MAX_BLOCK - 1 */
	if (cta_block_count > EDID_MAX_BLOCK - 1)
		cta_block_count = EDID_MAX_BLOCK - 1;
	for (i = 1; i <= cta_block_count; i++) {
		if (EDID_buf[i * 0x80] == 0x02)
			hdmitx_edid_cta_block_parse(hdmitx_device, &EDID_buf[i * 0x80]);
	}

	/* EDID parsing complete - check if 4k60/50 DV can be truly supported */
	dv = &prxcap->dv_info;
	check_dv_truly_support(hdmitx_device, dv);
	dv = &prxcap->dv_info2;
	check_dv_truly_support(hdmitx_device, dv);
	edid_check_pcm_declare(&hdmitx_device->tx_comm.rxcap);
	/* move parts that may contain cea timing parse behind
	 * VDB parse, so that to not affect VDB index which
	 * will be used in Y420CMDB map
	 */
	edid_standardtiming(&hdmitx_device->tx_comm.rxcap, &EDID_buf[0x26], 8);
	edid_parseceatiming(&hdmitx_device->tx_comm.rxcap, &EDID_buf[0x36]);
/*
 * Because DTDs are not able to represent some Video Formats, which can be
 * represented as SVDs and might be preferred by Sinks, the first DTD in the
 * base EDID data structure and the first SVD in the first CEA Extension can
 * differ. When the first DTD and SVD do not match and the total number of
 * DTDs defining Native Video Formats in the whole EDID is zero, the first
 * SVD shall take precedence.
 */
	if (!prxcap->flag_vfpdb &&
	    prxcap->preferred_mode != prxcap->VIC[0] &&
	    prxcap->number_of_dtd == 0) {
		pr_debug(EDID "change preferred_mode from %d to %d\n",
			prxcap->preferred_mode,	prxcap->VIC[0]);
		prxcap->preferred_mode = prxcap->VIC[0];
	}

	idx[0] = EDID_DETAILED_TIMING_DES_BLOCK0_POS;
	idx[1] = EDID_DETAILED_TIMING_DES_BLOCK1_POS;
	idx[2] = EDID_DETAILED_TIMING_DES_BLOCK2_POS;
	idx[3] = EDID_DETAILED_TIMING_DES_BLOCK3_POS;
	for (i = 0; i < 4; i++) {
		if ((EDID_buf[idx[i]]) && (EDID_buf[idx[i] + 1])) {
			struct vesa_standard_timing t;

			memset(&t, 0, sizeof(struct vesa_standard_timing));
			if (i == 0)
				edid_descriptor_pmt(prxcap, &t,
						    &EDID_buf[idx[i]]);
			if (i == 1)
				edid_descriptor_pmt2(prxcap, &t,
						     &EDID_buf[idx[i]]);
			continue;
		}
		switch (EDID_buf[idx[i] + 3]) {
		case TAG_STANDARD_TIMINGS:
			edid_standardtiming(prxcap, &EDID_buf[idx[i] + 5], 6);
			break;
		case TAG_CVT_TIMING_CODES:
			edid_cvt_timing(prxcap, &EDID_buf[idx[i] + 6]);
			break;
		case TAG_ESTABLISHED_TIMING_III:
			edid_standard_timing_iii(prxcap, &EDID_buf[idx[i] + 6]);
			break;
		case TAG_RANGE_LIMITS:
			break;
		case TAG_DISPLAY_PRODUCT_NAME_STRING:
			edid_receiverproductnameparse(prxcap,
						      &EDID_buf[idx[i] + 5]);
			break;
		default:
			break;
		}
	}

	if (hdmitx_edid_search_IEEEOUI(&EDID_buf[128])) {
		prxcap->ieeeoui = HDMI_IEEE_OUI;
		pr_debug(EDID "find IEEEOUT\n");
	} else {
		prxcap->ieeeoui = 0x0;
		pr_debug(EDID "not find IEEEOUT\n");
	}

	/* strictly DVI device judgement */
	/* valid EDID & no audio tag & no IEEEOUI */
	if (edid_check_valid(&EDID_buf[0]) &&
		!hdmitx_edid_search_IEEEOUI(&EDID_buf[128])) {
		prxcap->ieeeoui = 0x0;
		pr_debug(EDID "sink is DVI device\n");
	} else {
		prxcap->ieeeoui = HDMI_IEEE_OUI;
	}
	if (edid_zero_data(EDID_buf))
		prxcap->ieeeoui = HDMI_IEEE_OUI;

	if (!prxcap->AUD_count && !prxcap->ieeeoui)
		hdmitx_edid_set_default_aud(hdmitx_device);

	hdmitx_update_edid_chksum(EDID_buf, cta_block_count + 1, prxcap);

	if (!hdmitx_edid_check_valid_blocks(&EDID_buf[0])) {
		prxcap->ieeeoui = HDMI_IEEE_OUI;
		pr_info(EDID "Invalid edid, consider RX as HDMI device\n");
	}
	dv = &prxcap->dv_info;
	/* if sup_2160p60hz of dv or dv2 is true, check the MAX_TMDS*/
	if (dv->sup_2160p60hz) {
		if (prxcap->Max_TMDS_Clock2 * 5 < 590) {
			dv->sup_2160p60hz = 0;
			pr_debug(EDID "clear sup_2160p60hz\n");
		}
	}
	dv = &prxcap->dv_info2;
	if (dv->sup_2160p60hz) {
		if (prxcap->Max_TMDS_Clock2 * 5 < 590) {
			dv->sup_2160p60hz = 0;
			pr_debug(EDID "clear sup_2160p60hz\n");
		}
	}
	hdmitx_device->vend_id_hit = hdmitx_find_philips(hdmitx_device);
	/* For some receivers, they don't claim the screen size
	 * and re-calculate it from the h/v image size from dtd
	 * the unit of screen size is cm, but the unit of image size is mm
	 */
	if (prxcap->physical_width == 0 || prxcap->physical_height == 0) {
		struct dtd *t = &prxcap->dtd[0];

		prxcap->physical_width  = t->h_image_size;
		prxcap->physical_height = t->v_image_size;
	}

	/* if edid are all zeroes, or no VIC, set default vic */
	if (edid_zero_data(EDID_buf) || prxcap->VIC_count == 0)
		hdmitx_edid_set_default_vic(hdmitx_device);
	if (prxcap->ieeeoui == HDMI_IEEE_OUI) {
		hdmitx_current_status(HDMITX_EDID_HDMI_DEVICE);
	} else {
		hdmitx_device->physical_addr = 0xffff;
		hdmitx_current_status(HDMITX_EDID_DVI_DEVICE);
	}
	return 0;
}

/* Clear HDMI Hardware Module EDID RAM and EDID Buffer */
void hdmitx_edid_ram_buffer_clear(struct hdmitx_dev *hdmitx_device)
{
	unsigned int i = 0;

	/* Clear HDMI Hardware Module EDID RAM */
	hdmitx_device->hwop.cntlddc(hdmitx_device, DDC_EDID_CLEAR_RAM, 0);

	/* Clear EDID Buffer */
	for (i = 0; i < EDID_MAX_BLOCK * 128; i++)
		hdmitx_device->tx_comm.EDID_buf[i] = 0;
	for (i = 0; i < EDID_MAX_BLOCK * 128; i++)
		hdmitx_device->tx_comm.EDID_buf1[i] = 0;
}

/* Clear the Parse result of HDMI Sink's EDID. */
void hdmitx_edid_clear(struct hdmitx_dev *hdmitx_device)
{
	char tmp[2] = {0};
	struct rx_cap *prxcap = &hdmitx_device->tx_comm.rxcap;

	memset(prxcap, 0, sizeof(struct rx_cap));

	/* Note: in most cases, we think that rx is tv and the default
	 * IEEEOUI is HDMI Identifier
	 */
	prxcap->ieeeoui = HDMI_IEEE_OUI;

	hdmitx_device->vic_count = 0;
	memset(&hdmitx_device->tx_comm.EDID_hash[0], 0,
	       sizeof(hdmitx_device->tx_comm.EDID_hash));
	hdmitx_device->tx_comm.edid_parsing = 0;
	hdmitx_edid_set_default_aud(hdmitx_device);
	rx_set_hdr_lumi(&tmp[0], 2);
	rx_set_receiver_edid(&tmp[0], 2);
	phy_addr_clear(&hdmitx_device->hdmi_info.vsdb_phy_addr);
}

#undef pr_fmt
#define pr_fmt(fmt) "" fmt
/*
 * print one block data of edid
 */
#define TMP_EDID_BUF_SIZE	(256 + 8)
static void hdmitx_edid_blk_print(unsigned char *blk, unsigned int blk_idx)
{
	unsigned int i, pos;
	unsigned char *tmp_buf = NULL;

	tmp_buf = kmalloc(TMP_EDID_BUF_SIZE, GFP_KERNEL);
	if (!tmp_buf)
		return;

	memset(tmp_buf, 0, TMP_EDID_BUF_SIZE);
	pr_info("hdmitx: edid: blk%d raw data\n", blk_idx);
	for (i = 0, pos = 0; i < 128; i++) {
		pos += sprintf(tmp_buf + pos, "%02x", blk[i]);
		if (((i + 1) & 0x3f) == 0)    /* print 64 bytes a line */
			pos += sprintf(tmp_buf + pos, "\n");
	}
	pr_info("%s", tmp_buf);
	kfree(tmp_buf);
}

#undef pr_fmt
#define pr_fmt(fmt) "hdmitx: " fmt

/*
 * check EDID buf contains valid block numbers
 */
static unsigned int hdmitx_edid_check_valid_blocks(unsigned char *buf)
{
	unsigned int valid_blk_no = 0;
	unsigned int i = 0, j = 0;
	unsigned int tmp_chksum = 0;

	for (j = 0; j < EDID_MAX_BLOCK; j++) {
		for (i = 0; i < 128; i++)
			tmp_chksum += buf[i + j * 128];
		if (tmp_chksum != 0) {
			valid_blk_no++;
			if ((tmp_chksum & 0xff) == 0)
				pr_debug(EDID "check sum valid\n");
			else
				pr_warn(EDID "check sum invalid\n");
		}
		tmp_chksum = 0;
	}
	return valid_blk_no;
}

/*
 * suppose DDC read EDID two times successfully,
 * then compare EDID_buf and EDID_buf1.
 * if same, just print out EDID_buf raw data, else print out 2 buffers
 */
void hdmitx_edid_buf_compare_print(struct hdmitx_dev *hdmitx_device)
{
	unsigned int i = 0;
	unsigned int err_no = 0;
	unsigned char *buf0 = hdmitx_device->tx_comm.EDID_buf;
	unsigned char *buf1 = hdmitx_device->tx_comm.EDID_buf1;
	unsigned int valid_blk_no = 0;
	unsigned int blk_idx = 0;

	for (i = 0; i < EDID_MAX_BLOCK * 128; i++) {
		if (buf0[i] != buf1[i])
			err_no++;
	}

	if (err_no == 0) {
		/* calculate valid edid block numbers */
		valid_blk_no = hdmitx_edid_check_valid_blocks(buf0);

		if (valid_blk_no == 0) {
			pr_debug(EDID "raw data are all zeroes\n");
		} else {
			for (blk_idx = 0; blk_idx < valid_blk_no; blk_idx++)
				hdmitx_edid_blk_print(&buf0[blk_idx * 128],
						      blk_idx);
		}
	} else {
		pr_warn(EDID "%d errors between two reading\n", err_no);
		valid_blk_no = hdmitx_edid_check_valid_blocks(buf0);
		for (blk_idx = 0; blk_idx < valid_blk_no; blk_idx++)
			hdmitx_edid_blk_print(&buf0[blk_idx * 128], blk_idx);

		valid_blk_no = hdmitx_edid_check_valid_blocks(buf1);
		for (blk_idx = 0; blk_idx < valid_blk_no; blk_idx++)
			hdmitx_edid_blk_print(&buf1[blk_idx * 128], blk_idx);
	}
}

static bool hdmitx_edid_header_invalid(unsigned char *buf)
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

