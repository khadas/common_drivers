// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_dev_common.h>

int hdmitx_dev_setup_vsif_packet(struct hdmitx_common *tx_comm,
	struct hdmitx_hw_common *tx_hw, enum vsif_type type, int on, void *param)
{
	u8 hb[3] = {0x81, 0x1, 0};
	u8 len = 0; /* hb[2] = len */
	u8 vsif_db[28] = {0}; /* to be fulfilled */
	u8 *pb = &vsif_db[1]; /* to be fulfilled */
	u32 ieeeoui = 0;
	u32 vic = 0;

	if (type >= VT_MAX)
		return -EINVAL;

	switch (type) {
	case VT_DEFAULT:
		break;
	case VT_HDMI14_4K:
		ieeeoui = HDMI_IEEE_OUI;
		len = 5;
		vic = hdmitx_edid_get_hdmi14_4k_vic(tx_comm->cur_VIC);
		if (vic > 0) {
			pb[4] = vic & 0xf;
			pb[3] = 0x20;
			tx_hw->cntlconfig(tx_hw, CONF_AVI_VIC, 0);
		} else {
			pr_info("skip vsif for non-4k mode.\n");
			return -EINVAL;
		}
		break;
	case VT_ALLM:
		ieeeoui = HDMI_FORUM_IEEE_OUI;
		len = 5;
		pb[3] = 0x1; /* Fixed value */
		if (on) {
			pb[4] |= 1 << 1; /* set bit1, ALLM_MODE */
			/*reset vic which may be reset by VT_HDMI14_4K.*/
			if (hdmitx_edid_get_hdmi14_4k_vic(tx_comm->cur_VIC) > 0)
				tx_hw->cntlconfig(tx_hw, CONF_AVI_VIC, tx_comm->cur_VIC);
		} else {
			pb[4] &= ~(1 << 1); /* clear bit1, ALLM_MODE */
			/* still send out HS_VSIF, no set AVI.VIC = 0 */
		}
		break;
	default:
		break;
	}

	hb[2] = len;
	pb[0] = GET_OUI_BYTE0(ieeeoui);
	pb[1] = GET_OUI_BYTE1(ieeeoui);
	pb[2] = GET_OUI_BYTE2(ieeeoui);

	tx_hw->setdatapacket(HDMI_PACKET_VEND, pb, hb);
	return 1;
}

