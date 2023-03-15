// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

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

#include <linux/amlogic/media/vout/hdmi_tx21/hdmi_info_global.h>
#include <linux/amlogic/media/vout/hdmi_tx21/hdmi_tx_module.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_dev_common.h>

#include "hw/common.h"

#define to_hdmitx21_dev(x)	container_of(x, struct hdmitx_dev, tx_comm)

static void hdmitx_set_spd_info(struct hdmitx_dev *hdmitx_device);
static void hdmi_set_vend_spec_infofram(struct hdmitx_dev *hdev,
					enum hdmi_vic videocode);

static void construct_avi_packet(struct hdmitx_dev *hdev)
{
	struct hdmi_avi_infoframe *info = &hdev->infoframes.avi.avi;
	struct hdmi_format_para *para = hdev->para;

	hdmi_avi_infoframe_init(info);

	info->colorspace = para->cs;
	info->scan_mode = HDMI_SCAN_MODE_NONE;
	if (para->timing.v_active <= 576)
		info->colorimetry = HDMI_COLORIMETRY_ITU_601;
	else
		info->colorimetry = HDMI_COLORIMETRY_ITU_709;
	info->picture_aspect = HDMI_PICTURE_ASPECT_16_9;
	info->active_aspect = HDMI_ACTIVE_ASPECT_PICTURE;
	info->itc = 0;
	info->extended_colorimetry = HDMI_EXTENDED_COLORIMETRY_XV_YCC_601;
	info->quantization_range = HDMI_QUANTIZATION_RANGE_LIMITED;
	info->nups = HDMI_NUPS_UNKNOWN;
	info->video_code = para->timing.vic;
	if (para->timing.vic == HDMI_95_3840x2160p30_16x9 ||
	    para->timing.vic == HDMI_94_3840x2160p25_16x9 ||
	    para->timing.vic == HDMI_93_3840x2160p24_16x9 ||
	    para->timing.vic == HDMI_98_4096x2160p24_256x135)
		/*HDMI Spec V1.4b P151*/
		info->video_code = 0;
	info->ycc_quantization_range = HDMI_YCC_QUANTIZATION_RANGE_LIMITED;
	info->content_type = HDMI_CONTENT_TYPE_GRAPHICS;
	info->pixel_repeat = 0;
	if (para->timing.pi_mode == 0) { /* interlaced modes */
		if (para->timing.h_active == 1440)
			info->pixel_repeat = 1;
		if (para->timing.h_active == 2880)
			info->pixel_repeat = 3;
	}
	info->top_bar = 0;
	info->bottom_bar = 0;
	info->left_bar = 0;
	info->right_bar = 0;
	hdmi_avi_infoframe_set(info);
}

/************************************
 *	hdmitx protocol level interface
 *************************************/

/*
 * HDMI Identifier = HDMI_IEEE_OUI 0x000c03
 * If not, treated as a DVI Device
 */
static int is_dvi_device(struct rx_cap *prxcap)
{
	if (prxcap->ieeeoui != HDMI_IEEE_OUI)
		return 1;
	else
		return 0;
}

int hdmitx21_set_display(struct hdmitx_dev *hdev, enum hdmi_vic videocode)
{
	struct hdmi_format_para *param = NULL;
	enum hdmi_vic vic;
	int ret = -1;

	vic = hdev->tx_hw.getstate(&hdev->tx_hw, STAT_VIDEO_VIC, 0);
	if (hdev->vend_id_hit)
		pr_info(VID "special tv detected\n");
	pr_info(VID "already init VIC = %d  Now VIC = %d\n",
		vic, videocode);
	if (vic != HDMI_0_UNKNOWN && vic == videocode)
		hdev->tx_comm.cur_VIC = vic;

	param = hdev->para;
	if (!param) {
		pr_info("%s[%d]\n", __func__, __LINE__);
		return ret;
	}

	if (param->cs == HDMI_COLORSPACE_YUV444)
		if (!(hdev->tx_comm.rxcap.native_Mode & (1 << 5))) {
			param->cs = HDMI_COLORSPACE_YUV422;
			pr_info("change cs from 444 to 422\n");
		}
	if (param->cs == HDMI_COLORSPACE_YUV422)
		if (!(hdev->tx_comm.rxcap.native_Mode & (1 << 4))) {
			param->cs = HDMI_COLORSPACE_RGB;
			pr_info("change cs from 422 to rgb\n");
		}
	/* For Y420 modes */
	switch (videocode) {
	case HDMI_96_3840x2160p50_16x9:
	case HDMI_97_3840x2160p60_16x9:
	case HDMI_101_4096x2160p50_256x135:
	case HDMI_102_4096x2160p60_256x135:
		//param->cs = HDMI_COLORSPACE_YUV420; /* TODO */
		break;
	default:
		break;
	}

	if (param->cs == HDMI_COLORSPACE_RGB)
		pr_info(VID "rx edid only support RGB format\n");

	if (videocode >= HDMITX_VESA_OFFSET) {
		hdev->para->cs = HDMI_COLORSPACE_RGB;
		hdev->para->cd = COLORDEPTH_24B;
		pr_info("hdmitx: VESA only support RGB format\n");
	}

	if (hdev->hwop.setdispmode(hdev) >= 0) {
		construct_avi_packet(hdev);

		/* HDMI CT 7-33 DVI Sink, no HDMI VSDB nor any
		 * other VSDB, No GB or DI expected
		 * TMDS_MODE[hdmi_config]
		 * 0: DVI Mode	   1: HDMI Mode
		 */
		if (is_dvi_device(&hdev->tx_comm.rxcap)) {
			pr_info(VID "Sink is DVI device\n");
			hdev->tx_hw.cntlconfig(&hdev->tx_hw,
				CONF_HDMI_DVI_MODE, DVI_MODE);
		} else {
			pr_info(VID "Sink is HDMI device\n");
			hdev->tx_hw.cntlconfig(&hdev->tx_hw,
				CONF_HDMI_DVI_MODE, HDMI_MODE);
		}
		if (videocode == HDMI_95_3840x2160p30_16x9 ||
		    videocode == HDMI_94_3840x2160p25_16x9 ||
		    videocode == HDMI_93_3840x2160p24_16x9 ||
		    videocode == HDMI_98_4096x2160p24_256x135)
			hdmi_set_vend_spec_infofram(hdev, videocode);
		else if ((!hdev->flag_3dfp) && (!hdev->flag_3dtb) &&
			 (!hdev->flag_3dss))
			hdmi_set_vend_spec_infofram(hdev, 0);
		else
			;

		if (hdev->tx_comm.allm_mode) {
			hdmitx21_construct_vsif(&hdev->tx_comm, VT_ALLM, 1, NULL);
			hdev->tx_hw.cntlconfig(&hdev->tx_hw, CONF_CT_MODE,
				SET_CT_OFF);
		} else {
			hdev->tx_hw.cntlconfig(&hdev->tx_hw, CONF_CT_MODE,
				hdev->tx_comm.ct_mode);
		}
		ret = 0;
	}
	hdmitx_set_spd_info(hdev);

	return ret;
}

static void hdmi_set_vend_spec_infofram(struct hdmitx_dev *hdev,
					enum hdmi_vic videocode)
{
	int i;
	u8 db[28];
	u8 *ven_db = &db[1];
	u8 ven_hb[3];

	ven_hb[0] = 0x81;
	ven_hb[1] = 0x01;
	ven_hb[2] = 0x5;

	if (videocode == 0) {	   /* For non-4kx2k mode setting */
		hdmi_vend_infoframe_set(NULL);
		return;
	}

	if (hdev->tx_comm.rxcap.dv_info.block_flag == CORRECT ||
	    hdev->dv_src_feature == 1) {	   /* For dolby */
		return;
	}

	for (i = 0; i < 0x6; i++)
		ven_db[i] = 0;
	ven_db[0] = GET_OUI_BYTE0(HDMI_IEEE_OUI);
	ven_db[1] = GET_OUI_BYTE1(HDMI_IEEE_OUI);
	ven_db[2] = GET_OUI_BYTE2(HDMI_IEEE_OUI);
	ven_db[3] = 0x00;    /* 4k x 2k  Spec P156 */

	if (videocode == HDMI_95_3840x2160p30_16x9) {
		ven_db[3] = 0x20;
		ven_db[4] = 0x1;
	} else if (videocode == HDMI_94_3840x2160p25_16x9) {
		ven_db[3] = 0x20;
		ven_db[4] = 0x2;
	} else if (videocode == HDMI_93_3840x2160p24_16x9) {
		ven_db[3] = 0x20;
		ven_db[4] = 0x3;
	} else if (videocode == HDMI_98_4096x2160p24_256x135) {
		ven_db[3] = 0x20;
		ven_db[4] = 0x4;
	} else {
		;
	}
	hdmi_vend_infoframe_rawset(ven_hb, db);
}

int hdmi21_set_3d(struct hdmitx_dev *hdev, int type, u32 param)
{
	int i;
	u8 db[28];
	u8 *ven_db = &db[1];
	u8 ven_hb[3];
	struct hdmi_vendor_infoframe *info;

	info = &hdev->infoframes.vend.vendor.hdmi;

	ven_hb[0] = 0x81;
	ven_hb[1] = 0x01;
	ven_hb[2] = 0x6;
	if (type == T3D_DISABLE) {
		hdmi_vend_infoframe_rawset(ven_hb, db);
	} else {
		for (i = 0; i < 0x6; i++)
			ven_db[i] = 0;
		ven_db[0] = GET_OUI_BYTE0(HDMI_IEEE_OUI);
		ven_db[1] = GET_OUI_BYTE1(HDMI_IEEE_OUI);
		ven_db[2] = GET_OUI_BYTE2(HDMI_IEEE_OUI);
		ven_db[3] = 0x40;
		ven_db[4] = type << 4;
		ven_db[5] = param << 4;
		hdmi_vend_infoframe_rawset(ven_hb, db);
	}
	return 0;
}

/* Set Source Product Descriptor InfoFrame
 */
static void hdmitx_set_spd_info(struct hdmitx_dev *hdev)
{
	u8 spd_db[28] = {0x00};
	u32 len = 0;
	struct vendor_info_data *vend_data;

	if (hdev->config_data.vend_data) {
		vend_data = hdev->config_data.vend_data;
	} else {
		pr_info(VID "packet: can\'t get vendor data\n");
		return;
	}
	if (vend_data->vendor_name) {
		len = strlen(vend_data->vendor_name);
		strncpy(&spd_db[0], vend_data->vendor_name,
			(len > 8) ? 8 : len);
	}
	if (vend_data->product_desc) {
		len = strlen(vend_data->product_desc);
		strncpy(&spd_db[8], vend_data->product_desc,
			(len > 16) ? 16 : len);
	}
	spd_db[24] = 0x1;
	// TODO hdev->hwop.setinfoframe(HDMI_INFOFRAME_TYPE_SPD, SPD_HB);
}

int hdmitx21_construct_vsif(struct hdmitx_common *tx_comm,
	enum vsif_type type, int on, void *param)
{
	struct hdmitx_dev *hdev = to_hdmitx21_dev(tx_comm);

	return hdmitx_dev_setup_vsif_packet(tx_comm, &hdev->tx_hw, type, on, param);
}
