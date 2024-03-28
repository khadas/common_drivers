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

#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_common.h>
#include "hdmi_tx_module.h"
#include "hw/common.h"

/* Old definitions */
#define TYPE_AVI_INFOFRAMES       0x82
#define AVI_INFOFRAMES_VERSION    0x02
#define AVI_INFOFRAMES_LENGTH     0x0D

static void hdmitx_set_spd_info(struct hdmitx_dev *hdmitx_device);
static void hdmi_set_vend_spec_infofram(struct hdmitx_dev *hdev,
					enum hdmi_vic videocode);

static void hdmi_tx_construct_avi_packet(struct hdmi_format_para *video_param,
					 char *AVI_DB)
{
	// TODO
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

int hdmitx_set_display(struct hdmitx_dev *hdev, enum hdmi_vic videocode)
{
	struct hdmi_format_para *para = &hdev->tx_comm.fmt_para;
	struct hdmitx_hw_common *tx_hw_base = &hdev->tx_hw.base;
	int i, ret = -1;
	unsigned char AVI_DB[32];
	unsigned char AVI_HB[32];

	AVI_HB[0] = TYPE_AVI_INFOFRAMES;
	AVI_HB[1] = AVI_INFOFRAMES_VERSION;
	AVI_HB[2] = AVI_INFOFRAMES_LENGTH;
	for (i = 0; i < 32; i++)
		AVI_DB[i] = 0;

	HDMITX_DEBUG_VIDEO("%s set VIC = %d\n", __func__, videocode);

	if (para) {
		if (videocode >= HDMITX_VESA_OFFSET && para->cs != HDMI_COLORSPACE_RGB) {
			HDMITX_ERROR("VESA only support RGB format\n");
			para->cs = HDMI_COLORSPACE_RGB;
			para->cd = COLORDEPTH_24B;
		}

		if (hdev->tx_hw.base.setdispmode(&hdev->tx_hw.base) >= 0) {
			/* HDMI CT 7-33 DVI Sink, no HDMI VSDB nor any
			 * other VSDB, No GB or DI expected
			 * TMDS_MODE[hdmi_config]
			 * 0: DVI Mode	   1: HDMI Mode
			 */
			if (is_dvi_device(&hdev->tx_comm.rxcap)) {
				HDMITX_INFO("Sink is DVI device\n");
				hdmitx_hw_cntl_config(tx_hw_base,
					CONF_HDMI_DVI_MODE, DVI_MODE);
			} else {
				HDMITX_INFO("Sink is HDMI device\n");
				hdmitx_hw_cntl_config(tx_hw_base,
					CONF_HDMI_DVI_MODE, HDMI_MODE);
			}
			hdmi_tx_construct_avi_packet(para, (char *)AVI_DB);

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
				hdmitx_common_setup_vsif_packet(&hdev->tx_comm, VT_ALLM, 1, NULL);
				hdmitx_hw_cntl_config(tx_hw_base, CONF_CT_MODE,
					SET_CT_OFF);
			} else {
				hdmitx_hw_cntl_config(tx_hw_base, CONF_CT_MODE,
					hdev->tx_comm.ct_mode);
			}
			ret = 0;
		}
	}
	hdmitx_set_spd_info(hdev);

	return ret;
}

/* TODO: merge in hdmitx_common_setup_vsif_packet() */
static void hdmi_set_vend_spec_infofram(struct hdmitx_dev *hdev,
					enum hdmi_vic videocode)
{
	int i;
	unsigned char VEN_DB[6];
	unsigned char VEN_HB[3];
	struct hdmitx_hw_common *tx_hw_base = &hdev->tx_hw.base;

	VEN_HB[0] = 0x81;
	VEN_HB[1] = 0x01;
	VEN_HB[2] = 0x5;

	if (videocode == 0) {	   /* For non-4kx2k mode setting */
		hdmitx_hw_set_packet(tx_hw_base, HDMI_PACKET_VEND, NULL, VEN_HB);
		return;
	}

	if (hdev->tx_comm.rxcap.dv_info.block_flag == CORRECT ||
	    hdev->dv_src_feature == 1) {	   /* For dolby */
		return;
	}

	for (i = 0; i < 0x6; i++)
		VEN_DB[i] = 0;
	VEN_DB[0] = GET_OUI_BYTE0(HDMI_IEEE_OUI);
	VEN_DB[1] = GET_OUI_BYTE1(HDMI_IEEE_OUI);
	VEN_DB[2] = GET_OUI_BYTE2(HDMI_IEEE_OUI);
	VEN_DB[3] = 0x00;    /* 4k x 2k  Spec P156 */

	if (videocode == HDMI_95_3840x2160p30_16x9) {
		VEN_DB[3] = 0x20;
		VEN_DB[4] = 0x1;
	} else if (videocode == HDMI_94_3840x2160p25_16x9) {
		VEN_DB[3] = 0x20;
		VEN_DB[4] = 0x2;
	} else if (videocode == HDMI_93_3840x2160p24_16x9) {
		VEN_DB[3] = 0x20;
		VEN_DB[4] = 0x3;
	} else if (videocode == HDMI_98_4096x2160p24_256x135) {
		VEN_DB[3] = 0x20;
		VEN_DB[4] = 0x4;
	} else {
		;
	}
	hdmitx_hw_set_packet(tx_hw_base, HDMI_PACKET_VEND, VEN_DB, VEN_HB);
}

int hdmi_set_3d(struct hdmitx_dev *hdev, int type, unsigned int param)
{
	int i;
	unsigned char VEN_DB[6];
	unsigned char VEN_HB[3];
	struct hdmitx_hw_common *tx_hw_base = &hdev->tx_hw.base;

	VEN_HB[0] = 0x81;
	VEN_HB[1] = 0x01;
	VEN_HB[2] = 0x6;
	if (type == T3D_DISABLE) {
		hdmitx_hw_set_packet(tx_hw_base, HDMI_PACKET_VEND, NULL, VEN_HB);
	} else {
		for (i = 0; i < 0x6; i++)
			VEN_DB[i] = 0;
		VEN_DB[0] = GET_OUI_BYTE0(HDMI_IEEE_OUI);
		VEN_DB[1] = GET_OUI_BYTE1(HDMI_IEEE_OUI);
		VEN_DB[2] = GET_OUI_BYTE2(HDMI_IEEE_OUI);
		VEN_DB[3] = 0x40;
		VEN_DB[4] = type << 4;
		VEN_DB[5] = param << 4;
		hdmitx_hw_set_packet(tx_hw_base, HDMI_PACKET_VEND, VEN_DB, VEN_HB);
	}
	return 0;
}

/* Set Source Product Descriptor InfoFrame
 */
static void hdmitx_set_spd_info(struct hdmitx_dev *hdev)
{
	unsigned char SPD_DB[25] = {0x00};
	unsigned char SPD_HB[3] = {0x83, 0x1, 0x19};
	unsigned int len = 0;
	struct vendor_info_data *vend_data;
	struct hdmitx_hw_common *tx_hw_base = &hdev->tx_hw.base;

	if (hdev->config_data.vend_data) {
		vend_data = hdev->config_data.vend_data;
	} else {
		HDMITX_DEBUG_VIDEO("packet: can\'t get vendor data\n");
		return;
	}
	if (vend_data->vendor_name) {
		len = strlen(vend_data->vendor_name);
		strncpy(&SPD_DB[0], vend_data->vendor_name,
			(len > 8) ? 8 : len);
	}
	if (vend_data->product_desc) {
		len = strlen(vend_data->product_desc);
		strncpy(&SPD_DB[8], vend_data->product_desc,
			(len > 16) ? 16 : len);
	}
	SPD_DB[24] = 0x1;
	hdmitx_hw_set_packet(tx_hw_base, HDMI_SOURCE_DESCRIPTION, SPD_DB, SPD_HB);
}

