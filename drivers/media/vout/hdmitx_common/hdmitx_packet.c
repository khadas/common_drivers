// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_common.h>
#include "hdmitx_log.h"

int hdmitx_common_setup_vsif_packet(struct hdmitx_common *tx_comm,
	enum vsif_type type, int on, void *param)
{
	u8 hb[3] = {0x81, 0x1, 0};
	u8 len = 0; /* hb[2] = len */
	u8 vsif_db[28] = {0}; /* to be fulfilled */
	/* transmission usage, excluding checksum */
	u8 *db = &vsif_db[1];
	u32 ieeeoui = 0;
	u32 vic = 0;
	struct hdmitx_hw_common *tx_hw = tx_comm->tx_hw;

	if (type >= VT_MAX)
		return -EINVAL;

	switch (type) {
	case VT_DEFAULT:
		break;
	case VT_HDMI14_4K:
		ieeeoui = HDMI_IEEE_OUI;
		len = 5;
		vic = hdmitx_edid_get_hdmi14_4k_vic(tx_comm->fmt_para.vic);
		if (vic > 0) {
			hb[2] = len;
			db[0] = GET_OUI_BYTE0(ieeeoui);
			db[1] = GET_OUI_BYTE1(ieeeoui);
			db[2] = GET_OUI_BYTE2(ieeeoui);
			db[4] = vic & 0xf;
			db[3] = 0x20;
			hdmitx_hw_cntl_config(tx_hw, CONF_AVI_VIC, 0);
			hdmitx_hw_set_packet(tx_hw, HDMI_INFOFRAME_TYPE_VENDOR, db, hb);
		} else {
			HDMITX_INFO("skip vsif for non-4k mode.\n");
			return -EINVAL;
		}
		break;
	case VT_ALLM:
		ieeeoui = HDMI_FORUM_IEEE_OUI;
		len = 5;
		db[3] = 0x1; /* Fixed value */
		if (on) {
			hb[2] = len;
			db[0] = GET_OUI_BYTE0(ieeeoui);
			db[1] = GET_OUI_BYTE1(ieeeoui);
			db[2] = GET_OUI_BYTE2(ieeeoui);
			db[4] |= 1 << 1; /* set bit1, ALLM_MODE */
			/*reset vic which may be reset by VT_HDMI14_4K.*/
			if (hdmitx_edid_get_hdmi14_4k_vic(tx_comm->fmt_para.vic) > 0)
				hdmitx_hw_cntl_config(tx_hw, CONF_AVI_VIC, tx_comm->fmt_para.vic);
			hdmitx_hw_set_packet(tx_hw, HDMI_INFOFRAME_TYPE_VENDOR2, db, hb);
		} else {
			db[4] &= ~(1 << 1); /* clear bit1, ALLM_MODE */
			/* 1.When the Source stops transmitting the HF-VSIF,
			 * the Sink shall interpret this as an indication
			 * that transmission of features described in this
			 * Infoframe has stopped
			 * 2.When a Source is utilizing the HF-VSIF for ALLM
			 * signaling only and indicates the Sink should
			 * revert from low-latency Mode to its previous mode,
			 * the Source should send ALLM Mode = 0 to quickly
			 * and reliably request the change. If a Source
			 * indicates ALLM Mode = 0 in this manner, the
			 * Source should transmit an HF-VSIF with ALLM Mode = 0
			 * for at least 4 frames but for not more than 1 second.
			 */
			/* hdmi_vend_infoframe2_rawset(hb, vsif_db); */
			/* wait for 4frames ~ 1S, then stop send HF-VSIF */
			hdmitx_hw_set_packet(tx_hw, HDMI_INFOFRAME_TYPE_VENDOR2, NULL, NULL);
		}
		break;
	default:
		break;
	}

	return 1;
}

