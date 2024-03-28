// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/mm.h>
#include <linux/major.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/compiler.h>
#include <linux/arm-smccc.h>
#include <linux/amlogic/media/vout/vinfo.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_common.h>

#include "hdmi_tx_module.h"
#include "hdmi_tx.h"
#include "hw/enc_clk_config.h"

/* now this interface should be not used, otherwise need
 * adjust as hdmi_vend_infoframe_rawset fistly
 */
void hdmi_vend_infoframe_set(struct hdmi_vendor_infoframe *info)
{
	u8 body[31] = {0};
	struct hdmitx_dev *hdev = get_hdmitx21_device();

	if (!info) {
		if (hdev->tx_comm.rxcap.ifdb_present)
			hdmitx_infoframe_send(HDMI_INFOFRAME_TYPE_VENDOR, NULL);
		else
			hdmitx_infoframe_send(HDMI_INFOFRAME_TYPE_VENDOR2, NULL);
		return;
	}

	hdmi_vendor_infoframe_pack(info, body, sizeof(body));
	if (hdev->tx_comm.rxcap.ifdb_present)
		hdmitx_infoframe_send(HDMI_INFOFRAME_TYPE_VENDOR, body);
	else
		hdmitx_infoframe_send(HDMI_INFOFRAME_TYPE_VENDOR2, body);
}

/* refer to DV Consumer Decoder for Source Devices
 * System Development Guide Kit version chapter 4.4.8 Game
 * content signaling:
 * 1.if DV sink device that supports ALLM with
 * InfoFrame Data Block (IFDB), HF-VSIF with ALLM_Mode = 1
 * should comes after Dolby VSIF with L11_MD_Present = 1 and
 * Content_Type[3:0] = 0x2(case B1)
 * 2.DV sink device that supports ALLM without
 * InfoFrame Data Block (IFDB), Dolby VSIF with L11_MD_Present
 * = 1 and Content_Type[3:0] = 0x2 should comes after HF-VSIF
 * with  ALLM_Mode = 1(case B2), or should only send Dolby VSIF,
 * not send HF-VSIF(case A)
 */
/* only used for DV_VSIF / HDMI1.4b_VSIF / CUVA_VSIF / HDR10+ VSIF */
void hdmi_vend_infoframe_rawset(u8 *hb, u8 *pb)
{
	u8 body[31] = {0};
	struct hdmitx_dev *hdev = get_hdmitx21_device();

	if (!hb || !pb) {
		if (!hdev->tx_comm.rxcap.ifdb_present)
			hdmitx_infoframe_send(HDMI_INFOFRAME_TYPE_VENDOR2, NULL);
		else
			hdmitx_infoframe_send(HDMI_INFOFRAME_TYPE_VENDOR, NULL);
		return;
	}

	memcpy(body, hb, 3);
	memcpy(&body[3], pb, 28);
	if (hdev->tx_comm.rxcap.ifdb_present &&
		hdev->tx_comm.rxcap.additional_vsif_num >= 1) {
		/* dolby cts case93 B1 */
		hdmitx_infoframe_send(HDMI_INFOFRAME_TYPE_VENDOR, body);
	} else if (!hdev->tx_comm.rxcap.ifdb_present) {
		/* dolby cts case92 B2 */
		hdmitx_infoframe_send(HDMI_INFOFRAME_TYPE_VENDOR2, body);
	} else {
		/* case89 ifdb_present but no additional_vsif, should not send HF-VSIF */
		hdmitx_infoframe_send(HDMI_INFOFRAME_TYPE_VENDOR2, NULL);
		hdmitx_infoframe_send(HDMI_INFOFRAME_TYPE_VENDOR, body);
	}
}

/* only used for HF-VSIF */
void hdmi_vend_infoframe2_rawset(u8 *hb, u8 *pb)
{
	u8 body[31] = {0};
	struct hdmitx_dev *hdev = get_hdmitx21_device();

	if (!hb || !pb) {
		if (!hdev->tx_comm.rxcap.ifdb_present)
			hdmitx_infoframe_send(HDMI_INFOFRAME_TYPE_VENDOR, NULL);
		else
			hdmitx_infoframe_send(HDMI_INFOFRAME_TYPE_VENDOR2, NULL);
		return;
	}

	memcpy(body, hb, 3);
	memcpy(&body[3], pb, 28);
	if (hdev->tx_comm.rxcap.ifdb_present &&
		hdev->tx_comm.rxcap.additional_vsif_num >= 1) {
		/* dolby cts case93 B1 */
		hdmitx_infoframe_send(HDMI_INFOFRAME_TYPE_VENDOR2, body);
	} else if (!hdev->tx_comm.rxcap.ifdb_present) {
		/* dolby cts case92 B2 */
		hdmitx_infoframe_send(HDMI_INFOFRAME_TYPE_VENDOR, body);
	} else {
		/* case89 ifdb_present but no additional_vsif, currently
		 * no DV-VSIF enabled, then send HF-VSIF
		 */
		hdmitx_infoframe_send(HDMI_INFOFRAME_TYPE_VENDOR2, body);
	}
}

/* refer DV HDMI 1.4b/2.0/2.1 Transmission
 * 1.DV source device signals the video-timing
 * format by using the CTA VICs in its AVI InfoFrame
 * 2.DV source must not simultaneously transmit
 * a DV and any form of H14b VSIF, even for the case
 * of 4Kp24/25/30
 */
/* only used for DV_VSIF / CUVA VSIF / HDMI1.4b_VSIF / HDR10+ VSIF */
int hdmi_vend_infoframe_get(struct hdmitx_dev *hdev, u8 *body)
{
	int ret;

	if (!hdev || !body)
		return 0;

	if (hdev->tx_comm.rxcap.ifdb_present && hdev->tx_comm.rxcap.additional_vsif_num >= 1) {
		/* dolby cts case93 B1 */
		ret = hdmitx_infoframe_rawget(HDMI_INFOFRAME_TYPE_VENDOR, body);
	} else if (!hdev->tx_comm.rxcap.ifdb_present) {
		/* dolby cts case92 B2 */
		ret = hdmitx_infoframe_rawget(HDMI_INFOFRAME_TYPE_VENDOR2, body);
	} else {
		/* case89 */
		ret = hdmitx_infoframe_rawget(HDMI_INFOFRAME_TYPE_VENDOR, body);
	}
	return ret;
}

/******************sync from upstream start: drivers/video/hdmi.c*************************/
#define HDMI_INFOFRAME_HEADER_SIZE  4
#define HDMI_AVI_INFOFRAME_SIZE    13
#define HDMI_SPD_INFOFRAME_SIZE    25
#define HDMI_AUDIO_INFOFRAME_SIZE  10
#define HDMI_DRM_INFOFRAME_SIZE    26

#define HDMI_INFOFRAME_SIZE(type)	\
	(HDMI_INFOFRAME_HEADER_SIZE + HDMI_ ## type ## _INFOFRAME_SIZE)

/* from upstream */
static u8 hdmi_infoframe_checksum(const u8 *ptr, size_t size)
{
	u8 csum = 0;
	size_t i;

	/* compute checksum */
	for (i = 0; i < size; i++)
		csum += ptr[i];

	return 256 - csum;
}

/* from upstream */
static void hdmi_infoframe_set_checksum(void *buffer, size_t size)
{
	u8 *ptr = buffer;

	ptr[3] = hdmi_infoframe_checksum(buffer, size);
}

/* re-construct from upstream */
static int hdmi_avi_infoframe_check_only_renew(const struct hdmi_avi_infoframe *frame)
{
	/* refer to CTA-861-H Page 69 */
	if (frame->type != HDMI_INFOFRAME_TYPE_AVI ||
	    frame->version < 2 ||
	    frame->version > 4 ||
	    frame->length != HDMI_AVI_INFOFRAME_SIZE)
		return -EINVAL;

	if (frame->picture_aspect > HDMI_PICTURE_ASPECT_16_9)
		return -EINVAL;

	return 0;
}

/* re-construct from upstream
 * hdmi_avi_infoframe_check() - check a HDMI AVI infoframe
 * @frame: HDMI AVI infoframe
 *
 * Validates that the infoframe is consistent and updates derived fields
 * (eg. length) based on other fields.
 *
 * Returns 0 on success or a negative error code on failure.
 */
static int hdmi_avi_infoframe_check_renew(struct hdmi_avi_infoframe *frame)
{
	return hdmi_avi_infoframe_check_only_renew(frame);
}

/* re-construct from upstream
 * hdmi_avi_infoframe_pack_only() - write HDMI AVI infoframe to binary buffer
 * @frame: HDMI AVI infoframe
 * @buffer: destination buffer
 * @size: size of buffer
 *
 * Packs the information contained in the @frame structure into a binary
 * representation that can be written into the corresponding controller
 * registers. Also computes the checksum as required by section 5.3.5 of
 * the HDMI 1.4 specification.
 *
 * Returns the number of bytes packed into the binary buffer or a negative
 * error code on failure.
 */
static ssize_t hdmi_avi_infoframe_pack_only_renew(const struct hdmi_avi_infoframe *frame,
				     void *buffer, size_t size)
{
	u8 *ptr = buffer;
	size_t length;
	int ret;

	ret = hdmi_avi_infoframe_check_only_renew(frame);
	if (ret)
		return ret;

	length = HDMI_INFOFRAME_HEADER_SIZE + frame->length;

	if (size < length)
		return -ENOSPC;

	memset(buffer, 0, size);

	ptr[0] = frame->type;
	ptr[1] = frame->version;
	ptr[2] = frame->length;
	ptr[3] = 0; /* checksum */

	/* start infoframe payload */
	ptr += HDMI_INFOFRAME_HEADER_SIZE;

	ptr[0] = ((frame->colorspace & 0x3) << 5) | (frame->scan_mode & 0x3);

	/*
	 * Data byte 1, bit 4 has to be set if we provide the active format
	 * aspect ratio
	 */
	if (frame->active_aspect & 0xf)
		ptr[0] |= BIT(4);

	/* Bit 3 and 2 indicate if we transmit horizontal/vertical bar data */
	if (frame->top_bar || frame->bottom_bar)
		ptr[0] |= BIT(3);

	if (frame->left_bar || frame->right_bar)
		ptr[0] |= BIT(2);

	ptr[1] = ((frame->colorimetry & 0x3) << 6) |
		 ((frame->picture_aspect & 0x3) << 4) |
		 (frame->active_aspect & 0xf);

	ptr[2] = ((frame->extended_colorimetry & 0x7) << 4) |
		 ((frame->quantization_range & 0x3) << 2) |
		 (frame->nups & 0x3);

	if (frame->itc)
		ptr[2] |= BIT(7);

	ptr[3] = frame->video_code;

	ptr[4] = ((frame->ycc_quantization_range & 0x3) << 6) |
		 ((frame->content_type & 0x3) << 4) |
		 (frame->pixel_repeat & 0xf);

	ptr[5] = frame->top_bar & 0xff;
	ptr[6] = (frame->top_bar >> 8) & 0xff;
	ptr[7] = frame->bottom_bar & 0xff;
	ptr[8] = (frame->bottom_bar >> 8) & 0xff;
	ptr[9] = frame->left_bar & 0xff;
	ptr[10] = (frame->left_bar >> 8) & 0xff;
	ptr[11] = frame->right_bar & 0xff;
	ptr[12] = (frame->right_bar >> 8) & 0xff;

	hdmi_infoframe_set_checksum(buffer, length);

	return length;
}

/* re-construct from upstream
 * hdmi_avi_infoframe_pack() - check a HDMI AVI infoframe,
 *                             and write it to binary buffer
 * @frame: HDMI AVI infoframe
 * @buffer: destination buffer
 * @size: size of buffer
 *
 * Validates that the infoframe is consistent and updates derived fields
 * (eg. length) based on other fields, after which it packs the information
 * contained in the @frame structure into a binary representation that
 * can be written into the corresponding controller registers. This function
 * also computes the checksum as required by section 5.3.5 of the HDMI 1.4
 * specification.
 *
 * Returns the number of bytes packed into the binary buffer or a negative
 * error code on failure.
 */
static ssize_t hdmi_avi_infoframe_pack_renew(struct hdmi_avi_infoframe *frame,
				void *buffer, size_t size)
{
	int ret;

	ret = hdmi_avi_infoframe_check_renew(frame);
	if (ret)
		return ret;

	return hdmi_avi_infoframe_pack_only_renew(frame, buffer, size);
}

/* re-construct from upstream
 * hdmi_avi_infoframe_unpack() - unpack binary buffer to a HDMI AVI infoframe
 * @frame: HDMI AVI infoframe
 * @buffer: source buffer
 * @size: size of buffer
 *
 * Unpacks the information contained in binary @buffer into a structured
 * @frame of the HDMI Auxiliary Video (AVI) information frame.
 * Also verifies the checksum as required by section 5.3.5 of the HDMI 1.4
 * specification.
 *
 * Returns 0 on success or a negative error code on failure.
 */
int hdmi_avi_infoframe_unpack_renew(struct hdmi_avi_infoframe *frame,
				     const void *buffer, size_t size)
{
	const u8 *ptr = buffer;

	if (size < HDMI_INFOFRAME_SIZE(AVI))
		return -EINVAL;

	if (ptr[0] != HDMI_INFOFRAME_TYPE_AVI ||
	    ptr[1] < 2 ||
	    ptr[1] > 4 ||

	    ptr[2] != HDMI_AVI_INFOFRAME_SIZE)
		return -EINVAL;

	if (hdmi_infoframe_checksum(buffer, HDMI_INFOFRAME_SIZE(AVI)) != 0)
		return -EINVAL;

	hdmi_avi_infoframe_init(frame);

	ptr += HDMI_INFOFRAME_HEADER_SIZE;

	frame->colorspace = (ptr[0] >> 5) & 0x3;
	if (ptr[0] & 0x10)
		frame->active_aspect = ptr[1] & 0xf;
	if (ptr[0] & 0x8) {
		frame->top_bar = (ptr[6] << 8) | ptr[5];
		frame->bottom_bar = (ptr[8] << 8) | ptr[7];
	}
	if (ptr[0] & 0x4) {
		frame->left_bar = (ptr[10] << 8) | ptr[9];
		frame->right_bar = (ptr[12] << 8) | ptr[11];
	}
	frame->scan_mode = ptr[0] & 0x3;

	frame->colorimetry = (ptr[1] >> 6) & 0x3;
	frame->picture_aspect = (ptr[1] >> 4) & 0x3;
	frame->active_aspect = ptr[1] & 0xf;

	frame->itc = ptr[2] & 0x80 ? true : false;
	frame->extended_colorimetry = (ptr[2] >> 4) & 0x7;
	frame->quantization_range = (ptr[2] >> 2) & 0x3;
	frame->nups = ptr[2] & 0x3;

	frame->video_code = ptr[3];
	/* refer to CTA-861-H Page 69 */
	if (frame->video_code >= 128)
		frame->version = 0x3;
	frame->ycc_quantization_range = (ptr[4] >> 6) & 0x3;
	frame->content_type = (ptr[4] >> 4) & 0x3;

	frame->pixel_repeat = ptr[4] & 0xf;

	return 0;
}

/******************sync from upstream end*************************/

void hdmi_avi_infoframe_set(struct hdmi_avi_infoframe *info)
{
	u8 body[31] = {0};

	if (!info) {
		hdmitx_infoframe_send(HDMI_INFOFRAME_TYPE_AVI, NULL);
		return;
	}

	/* refer to CTA-861-H Page 69 */
	if (info->video_code >= 128)
		info->version = 0x3;
	else
		info->version = 0x2;
	hdmi_avi_infoframe_pack_renew(info, body, sizeof(body));
	hdmitx_infoframe_send(HDMI_INFOFRAME_TYPE_AVI, body);
}

void hdmi_avi_infoframe_rawset(u8 *hb, u8 *pb)
{
	u8 body[31] = {0};

	if (!hb || !pb) {
		hdmitx_infoframe_send(HDMI_INFOFRAME_TYPE_AVI, NULL);
		return;
	}

	memcpy(body, hb, 3);
	memcpy(&body[3], pb, 28);
	hdmitx_infoframe_send(HDMI_INFOFRAME_TYPE_AVI, body);
}

int hdmi_avi_infoframe_get(u8 *body)
{
	int ret;

	if (!body)
		return 0;
	ret = hdmitx_infoframe_rawget(HDMI_INFOFRAME_TYPE_AVI, body);
	return ret;
}

void hdmi_avi_infoframe_config(enum avi_component_conf conf, u8 val)
{
	struct hdmitx_dev *hdev = get_hdmitx21_device();
	struct hdmi_avi_infoframe *info = &hdev->infoframes.avi.avi;
	struct hdmi_format_para *para = &hdev->tx_comm.fmt_para;

	switch (conf) {
	case CONF_AVI_CS:
		info->colorspace = val;
		break;
	case CONF_AVI_BT2020:
		if (val == SET_AVI_BT2020) {
			info->colorimetry = HDMI_COLORIMETRY_EXTENDED;
			info->extended_colorimetry = HDMI_EXTENDED_COLORIMETRY_BT2020;
		}
		if (val == CLR_AVI_BT2020) {
			if (para->timing.v_total <= 576) {/* SD formats */
				info->colorimetry = HDMI_COLORIMETRY_ITU_601;
				info->extended_colorimetry = 0;
			} else {
				if (hdev->colormetry) {
					info->colorimetry = HDMI_COLORIMETRY_EXTENDED;
					info->extended_colorimetry =
						HDMI_EXTENDED_COLORIMETRY_BT2020;
				} else {
					info->colorimetry = HDMI_COLORIMETRY_ITU_709;
					info->extended_colorimetry = 0;
				}
			}
		}
		break;
	case CONF_AVI_Q01:
		info->quantization_range = val;
		break;
	case CONF_AVI_YQ01:
		info->ycc_quantization_range = val;
		break;
	case CONF_AVI_VIC:
		info->video_code = val;
		break;
	case CONF_AVI_AR:
		info->picture_aspect = val;
		break;
	case CONF_AVI_CT_TYPE:
		info->itc = (val >> 4) & 0x1;
		val = val & 0xf;
		if (val == SET_CT_OFF)
			info->content_type = 0;
		else if (val == SET_CT_GRAPHICS)
			info->content_type = 0;
		else if (val == SET_CT_PHOTO)
			info->content_type = 1;
		else if (val == SET_CT_CINEMA)
			info->content_type = 2;
		else if (val == SET_CT_GAME)
			info->content_type = 3;
		break;
	default:
		break;
	}
	hdmi_avi_infoframe_set(info);
}

/* EMP packets is different as other packets
 * no checksum, the successive packets in a video frame...
 */
void hdmi_emp_infoframe_set(enum emp_type type, struct emp_packet_st *info)
{
	u8 body[31] = {0};
	u8 *md;
	u16 pkt_type;

	pkt_type = (HDMI_INFOFRAME_TYPE_EMP << 8) | type;
	if (!info) {
		hdmitx_infoframe_send(pkt_type, NULL);
		return;
	}

	/* head body, 3bytes */
	body[0] = info->header.header;
	body[1] = info->header.first << 7 | info->header.last << 6;
	body[2] = info->header.seq_idx;
	/* packet body, 28bytes */
	body[3] = info->body.emp0.new << 7 |
		info->body.emp0.end << 6 |
		info->body.emp0.ds_type << 4 |
		info->body.emp0.afr << 3 |
		info->body.emp0.vfr << 2 |
		info->body.emp0.sync << 1;
	body[4] = 0; /* RSVD */
	body[5] = info->body.emp0.org_id;
	body[6] = info->body.emp0.ds_tag >> 8 & 0xff;
	body[7] = info->body.emp0.ds_tag & 0xff;
	body[8] = info->body.emp0.ds_length >> 8 & 0xff;
	body[9] = info->body.emp0.ds_length & 0xff;
	md = &body[10];
	switch (info->type) {
	case EMP_TYPE_VRR_GAME:
		md[0] = info->body.emp0.md.game_md.fva_factor_m1 << 4 |
			info->body.emp0.md.game_md.vrr_en << 0;
		md[1] = info->body.emp0.md.game_md.base_vfront;
		md[2] = info->body.emp0.md.game_md.brr_rate >> 8 & 3;
		md[3] = info->body.emp0.md.game_md.brr_rate & 0xff;
		break;
	case EMP_TYPE_VRR_QMS:
		md[0] = info->body.emp0.md.qms_md.qms_en << 2 |
			info->body.emp0.md.qms_md.m_const << 1;
		md[1] = info->body.emp0.md.qms_md.base_vfront;
		md[2] = info->body.emp0.md.qms_md.next_tfr << 3 |
			(info->body.emp0.md.qms_md.brr_rate >> 8 & 3);
		md[3] = info->body.emp0.md.qms_md.brr_rate & 0xff;
		break;
	case EMP_TYPE_SBTM:
		md[0] = info->body.emp0.md.sbtm_md.sbtm_ver << 0;
		md[1] = info->body.emp0.md.sbtm_md.sbtm_mode << 0 |
			info->body.emp0.md.sbtm_md.sbtm_type << 2 |
			info->body.emp0.md.sbtm_md.grdm_min << 4 |
			info->body.emp0.md.sbtm_md.grdm_lum << 6;
		md[2] = (info->body.emp0.md.sbtm_md.frmpblimitint >> 8) & 0x3f;
		md[3] = info->body.emp0.md.sbtm_md.frmpblimitint & 0xff;
		break;
	default:
		break;
	}
	hdmitx_infoframe_send(pkt_type, body);
}

/* this is only configuring the EMP frame body, not send by HW
 * then call hdmi_emp_infoframe_set to send out
 */
void hdmi_emp_frame_set_member(struct emp_packet_st *info,
	enum emp_component_conf conf, u32 val)
{
	if (!info)
		return;

	switch (conf) {
	case CONF_HEADER_INIT:
		info->header.header = HDMI_INFOFRAME_TYPE_EMP; /* fixed value */
		break;
	case CONF_HEADER_LAST:
		info->header.last = !!val;
		break;
	case CONF_HEADER_FIRST:
		info->header.first = !!val;
		break;
	case CONF_HEADER_SEQ_INDEX:
		info->header.seq_idx = val;
		break;
	case CONF_SYNC:
		info->body.emp0.sync = !!val;
		break;
	case CONF_VFR:
		info->body.emp0.vfr = !!val;
		break;
	case CONF_AFR:
		info->body.emp0.afr = !!val;
		break;
	case CONF_DS_TYPE:
		info->body.emp0.ds_type = val & 3;
		break;
	case CONF_END:
		info->body.emp0.end = !!val;
		break;
	case CONF_NEW:
		info->body.emp0.new = !!val;
		break;
	case CONF_ORG_ID:
		info->body.emp0.org_id = val;
		break;
	case CONF_DATA_SET_TAG:
		info->body.emp0.ds_tag = val;
		break;
	case CONF_DATA_SET_LENGTH:
		info->body.emp0.ds_length = val;
		break;
	case CONF_VRR_EN:
		info->body.emp0.md.game_md.vrr_en = !!val;
		break;
	case CONF_FACTOR_M1:
		info->body.emp0.md.game_md.fva_factor_m1 = val;
		break;
	case CONF_QMS_EN:
		info->body.emp0.md.qms_md.qms_en = !!val;
		break;
	case CONF_M_CONST:
		info->body.emp0.md.qms_md.m_const = !!val;
		break;
	case CONF_BASE_VFRONT:
		info->body.emp0.md.qms_md.base_vfront = val;
		break;
	case CONF_NEXT_TFR:
		info->body.emp0.md.qms_md.next_tfr = val;
		break;
	case CONF_BASE_REFRESH_RATE:
		info->body.emp0.md.qms_md.brr_rate = val & 0x3ff;
		break;
	case CONF_SBTM_VER:
		info->body.emp0.md.sbtm_md.sbtm_ver = val & 0xf;
		break;
	case CONF_SBTM_MODE:
		info->body.emp0.md.sbtm_md.sbtm_mode = val & 0x3;
		break;
	case CONF_SBTM_TYPE:
		info->body.emp0.md.sbtm_md.sbtm_type = val & 0x3;
		break;
	case CONF_SBTM_GRDM_MIN:
		info->body.emp0.md.sbtm_md.grdm_min = val & 0x3;
		break;
	case CONF_SBTM_GRDM_LUM:
		info->body.emp0.md.sbtm_md.grdm_lum = val & 0x3;
		break;
	case CONF_SBTM_FRMPBLIMITINT:
		info->body.emp0.md.sbtm_md.frmpblimitint = val & 0x3fff;
		break;
	default:
		break;
	}
}

void hdmi_spd_infoframe_set(struct hdmi_spd_infoframe *info)
{
	u8 body[31] = {0};

	if (!info) {
		hdmitx_infoframe_send(HDMI_INFOFRAME_TYPE_SPD, NULL);
		return;
	}

	hdmi_spd_infoframe_pack(info, body, sizeof(body));
	hdmitx_infoframe_send(HDMI_INFOFRAME_TYPE_SPD, body);
}

void hdmi_audio_infoframe_set(struct hdmi_audio_infoframe *info)
{
	u8 body[31] = {0};

	if (!info) {
		hdmitx_infoframe_send(HDMI_INFOFRAME_TYPE_AUDIO, NULL);
		return;
	}

	hdmi_audio_infoframe_pack(info, body, sizeof(body));
	hdmitx_infoframe_send(HDMI_INFOFRAME_TYPE_AUDIO, body);
}

void hdmi_audio_infoframe_rawset(u8 *hb, u8 *pb)
{
	u8 body[31] = {0};

	if (!hb || !pb) {
		hdmitx_infoframe_send(HDMI_INFOFRAME_TYPE_AUDIO, NULL);
		return;
	}

	memcpy(body, hb, 3);
	memcpy(&body[3], pb, 28);
	hdmitx_infoframe_send(HDMI_INFOFRAME_TYPE_AUDIO, body);
}

void hdmi_drm_infoframe_set(struct hdmi_drm_infoframe *info)
{
	u8 body[31] = {0};

	if (!info) {
		hdmitx_infoframe_send(HDMI_INFOFRAME_TYPE_DRM, NULL);
		return;
	}

	hdmi_drm_infoframe_pack(info, body, sizeof(body));
	hdmitx_infoframe_send(HDMI_INFOFRAME_TYPE_DRM, body);
}

void hdmi_drm_infoframe_rawset(u8 *hb, u8 *pb)
{
	u8 body[31] = {0};

	if (!hb || !pb) {
		hdmitx_infoframe_send(HDMI_INFOFRAME_TYPE_DRM, NULL);
		return;
	}

	memcpy(body, hb, 3);
	memcpy(&body[3], pb, 28);
	hdmitx_infoframe_send(HDMI_INFOFRAME_TYPE_DRM, body);
}

/* for only 8bit */
void hdmi_gcppkt_manual_set(bool en)
{
	u8 body[31] = {0};

	body[0] = HDMI_PACKET_TYPE_GCP;
	if (en)
		hdmitx_infoframe_send(HDMI_PACKET_TYPE_GCP, body);
	else
		hdmitx_infoframe_send(HDMI_PACKET_TYPE_GCP, NULL);
}

void hdmi_sbtm_infoframe_rawset(u8 *hb, u8 *pb)
{
	u8 body[31] = {0};

	if (!hb || !pb) {
		hdmitx_infoframe_send(HDMI_INFOFRAME_TYPE_SBTM, NULL);
		return;
	}

	memcpy(body, hb, 3);
	memcpy(&body[3], pb, 28);
	hdmitx_infoframe_send(HDMI_INFOFRAME_TYPE_SBTM, body);
}

