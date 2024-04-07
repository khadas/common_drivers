// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/mm.h>
#include <linux/major.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/cdev.h>

/* Local include */
#include "hdmi_rx_repeater.h"
#include "hdmi_rx_pktinfo.h"
#include "hdmi_rx_drv.h"
#include "hdmi_rx_hw.h"
#include "hdmi_rx_wrapper.h"

/*this parameter moved from wrapper.c */
/*
 * bool hdr_enable = true;
 */
static struct rxpkt_st rxpktsts[E_PORT_NUM];
struct packet_info_s rx_pkt[E_PORT_NUM];
u32 rx_vsif_type[E_PORT_NUM];
u32 rx_emp_type[E_PORT_NUM];
u32 rx_spd_type[E_PORT_NUM];
u32 next_tfr[14] = {0, 23976, 24000, 25000, 29970, 30000, 47952, 48000, 50000, 59940, 60000,
	100000, 119880, 120000};

static struct pkt_type_reg_map_st pkt_maping[] = {
	/*infoframe pkt*/
	{PKT_TYPE_INFOFRAME_VSI,	PFIFO_VS_EN},
	{PKT_TYPE_INFOFRAME_AVI,	PFIFO_AVI_EN},
	{PKT_TYPE_INFOFRAME_SPD,	PFIFO_SPD_EN},
	{PKT_TYPE_INFOFRAME_AUD,	PFIFO_AUD_EN},
	{PKT_TYPE_INFOFRAME_MPEGSRC,	PFIFO_MPEGS_EN},
	{PKT_TYPE_INFOFRAME_NVBI,	PFIFO_NTSCVBI_EN},
	{PKT_TYPE_INFOFRAME_DRM,	PFIFO_DRM_EN},
	/*other pkt*/
	{PKT_TYPE_ACR,				PFIFO_ACR_EN},
	{PKT_TYPE_GCP,				PFIFO_GCP_EN},
	{PKT_TYPE_ACP,				PFIFO_ACP_EN},
	{PKT_TYPE_ISRC1,			PFIFO_ISRC1_EN},
	{PKT_TYPE_ISRC2,			PFIFO_ISRC2_EN},
	{PKT_TYPE_GAMUT_META,		PFIFO_GMT_EN},
	{PKT_TYPE_AUD_META,			PFIFO_AMP_EN},
	{PKT_TYPE_EMP,				PFIFO_EMP_EN},

	/*end of the table*/
	{K_FLAG_TAB_END,			K_FLAG_TAB_END},
};

struct st_pkt_test_buff *pkt_testbuff;

void rx_pkt_status(u8 port)
{
	//rx_pr("packet_fifo_cfg=0x%x\n", packet_fifo_cfg);
	/*rx_pr("pdec_ists_en=0x%x\n\n", pdec_ists_en);*/
	rx_pr("fifo_Int_cnt=%d\n", rxpktsts[port].fifo_int_cnt);
	rx_pr("fifo_pkt_num=%d\n", rxpktsts[port].fifo_pkt_num);
	rx_pr("pkt_cnt_avi=%d\n", rxpktsts[port].pkt_cnt_avi);
	rx_pr("pkt_cnt_vsi=%d\n", rxpktsts[port].pkt_cnt_vsi);
	rx_pr("pkt_cnt_drm=%d\n", rxpktsts[port].pkt_cnt_drm);
	rx_pr("pkt_cnt_spd=%d\n", rxpktsts[port].pkt_cnt_spd);
	rx_pr("pkt_cnt_audif=%d\n", rxpktsts[port].pkt_cnt_audif);
	rx_pr("pkt_cnt_mpeg=%d\n", rxpktsts[port].pkt_cnt_mpeg);
	rx_pr("pkt_cnt_nvbi=%d\n", rxpktsts[port].pkt_cnt_nvbi);
	rx_pr("pkt_cnt_acr=%d\n", rxpktsts[port].pkt_cnt_acr);
	rx_pr("pkt_cnt_gcp=%d\n", rxpktsts[port].pkt_cnt_gcp);
	rx_pr("pkt_cnt_acp=%d\n", rxpktsts[port].pkt_cnt_acp);
	rx_pr("pkt_cnt_isrc1=%d\n", rxpktsts[port].pkt_cnt_isrc1);
	rx_pr("pkt_cnt_isrc2=%d\n", rxpktsts[port].pkt_cnt_isrc2);
	rx_pr("pkt_cnt_gameta=%d\n", rxpktsts[port].pkt_cnt_gameta);
	rx_pr("pkt_cnt_amp=%d\n", rxpktsts[port].pkt_cnt_amp);
	rx_pr("pkt_cnt_emp=%d\n", rxpktsts[port].pkt_cnt_emp);
	rx_pr("pkt_cnt_vsi_ex=%d\n", rxpktsts[port].pkt_cnt_vsi_ex);
	rx_pr("pkt_cnt_drm_ex=%d\n", rxpktsts[port].pkt_cnt_drm_ex);
	rx_pr("pkt_cnt_gmd_ex=%d\n", rxpktsts[port].pkt_cnt_gmd_ex);
	rx_pr("pkt_cnt_aif_ex=%d\n", rxpktsts[port].pkt_cnt_aif_ex);
	rx_pr("pkt_cnt_avi_ex=%d\n", rxpktsts[port].pkt_cnt_avi_ex);
	rx_pr("pkt_cnt_acr_ex=%d\n", rxpktsts[port].pkt_cnt_acr_ex);
	rx_pr("pkt_cnt_gcp_ex=%d\n", rxpktsts[port].pkt_cnt_gcp_ex);
	rx_pr("pkt_cnt_amp_ex=%d\n", rxpktsts[port].pkt_cnt_amp_ex);
	rx_pr("pkt_cnt_nvbi_ex=%d\n", rxpktsts[port].pkt_cnt_nvbi_ex);
	rx_pr("pkt_cnt_emp_ex=%d\n", rxpktsts[port].pkt_cnt_emp_ex);
	rx_pr("pkt_chk_flg=%d\n", rxpktsts[port].pkt_chk_flg);

}

void rx_pkt_debug(void)
{
	u32 data32;
	u8 i;

	for (i = 0; i < rx_info.port_num; i++)
		memset(&rxpktsts[i], 0, sizeof(struct rxpkt_st));

	data32 = hdmirx_rd_dwc(DWC_PDEC_CTRL);
	data32 |= (rx_pkt_type_mapping(PKT_TYPE_INFOFRAME_VSI));
	data32 |= (rx_pkt_type_mapping(PKT_TYPE_INFOFRAME_AVI));
	data32 |= (rx_pkt_type_mapping(PKT_TYPE_INFOFRAME_SPD));
	data32 |= (rx_pkt_type_mapping(PKT_TYPE_INFOFRAME_AUD));
	data32 |= (rx_pkt_type_mapping(PKT_TYPE_INFOFRAME_VSI));
	data32 |= (rx_pkt_type_mapping(PKT_TYPE_INFOFRAME_MPEGSRC));
	if (rx_info.chip_id != CHIP_ID_TXHD &&
		rx_info.chip_id != CHIP_ID_T5D) {
		data32 |= (rx_pkt_type_mapping(PKT_TYPE_INFOFRAME_NVBI));
		data32 |= (rx_pkt_type_mapping(PKT_TYPE_INFOFRAME_DRM));
		data32 |= (rx_pkt_type_mapping(PKT_TYPE_AUD_META));
	}
	data32 |= (rx_pkt_type_mapping(PKT_TYPE_ACR));
	data32 |= (rx_pkt_type_mapping(PKT_TYPE_GCP));
	data32 |= (rx_pkt_type_mapping(PKT_TYPE_ACP));
	data32 |= (rx_pkt_type_mapping(PKT_TYPE_ISRC1));
	data32 |= (rx_pkt_type_mapping(PKT_TYPE_ISRC2));
	data32 |= (rx_pkt_type_mapping(PKT_TYPE_GAMUT_META));
	if (rx_info.chip_id >= CHIP_ID_TL1)
		data32 |= (rx_pkt_type_mapping(PKT_TYPE_EMP));

	hdmirx_wr_dwc(DWC_PDEC_CTRL, data32);
	rx_pr("enable fifo\n");
}

/*
 * hdmi rx packet module debug interface, if system not
 * enable pkt fifo module, you need first fun "debugfifo" cmd
 * to enable hw de-code pkt. and then rum "status", you will
 * see the pkt_xxx_cnt value is increasing. means have received
 * pkt info.
 * @param  input [are group of string]
 *  cmd style: pktinfo [param1] [param2]
 *  cmd ex:pktinfo dump 0x82
 *  ->means dump avi infofram pkt content
 * @return [no]
 */
void rx_debug_pktinfo(char input[][20], u8 port)
{
	u32 sts = 0;
	u32 enable = 0;
	u32 res = 0;

	if (strncmp(input[1], "debugfifo", 9) == 0) {
		/*open all pkt interrupt source for debug*/
		rx_pkt_debug();
		enable |= (PD_FIFO_START_PASS | PD_FIFO_OVERFL);
		pdec_ists_en |= enable;
		rx_pr("pdec_ists_en=0x%x\n", pdec_ists_en);
		rx_irq_en(1, port);
	} else if (strncmp(input[1], "debugext", 8) == 0) {
		if (rx_info.chip_id == CHIP_ID_TXLX ||
		    rx_info.chip_id == CHIP_ID_TXHD)
			enable |= _BIT(30);/* DRC_RCV*/
		else
			enable |= _BIT(9);/* DRC_RCV*/
		if (rx_info.chip_id >= CHIP_ID_TL1)
			enable |= _BIT(9);/* EMP_RCV*/
		enable |= _BIT(20);/* GMD_RCV */
		enable |= _BIT(19);/* AIF_RCV */
		enable |= _BIT(18);/* AVI_RCV */
		enable |= _BIT(17);/* ACR_RCV */
		enable |= _BIT(16);/* GCP_RCV */
		enable |= _BIT(15);/* VSI_RCV CHG*/
		enable |= _BIT(14);/* AMP_RCV*/
		pdec_ists_en |= enable;
		rx_pr("pdec_ists_en=0x%x\n", pdec_ists_en);
		rx_irq_en(1, port);
	} else if (strncmp(input[1], "status", 6) == 0) {
		rx_pkt_status(port);
	} else if (strncmp(input[1], "dump", 7) == 0) {
		/*check input type*/
		if (kstrtou32(input[2], 16, &res) < 0)
			rx_pr("error input:fmt is 0xValue\n");
		rx_pkt_dump(res, port);
	} else if (strncmp(input[1], "irqdisable", 10) == 0) {
		if (strncmp(input[2], "fifo", 4) == 0) {
			sts = (PD_FIFO_START_PASS | PD_FIFO_OVERFL);
		} else if (strncmp(input[2], "drm", 3) == 0) {
			if (rx_info.chip_id == CHIP_ID_TXLX ||
			    rx_info.chip_id == CHIP_ID_TXHD)
				sts = _BIT(30);
			else
				sts = _BIT(9);
		} else if (strncmp(input[2], "gmd", 3) == 0) {
			sts = _BIT(20);
		} else if (strncmp(input[2], "aif", 3) == 0) {
			sts = _BIT(19);
		} else if (strncmp(input[2], "avi", 3) == 0) {
			sts = _BIT(18);
		} else if (strncmp(input[2], "acr", 3) == 0) {
			sts = _BIT(17);
		} else if (strncmp(input[2], "gcp", 3) == 0) {
			sts = GCP_RCV;
		} else if (strncmp(input[2], "vsi", 3) == 0) {
			sts = VSI_RCV;
		} else if (strncmp(input[2], "amp", 3) == 0) {
			sts = _BIT(14);
		} else if (strncmp(input[2], "emp", 3) == 0) {
			if (rx_info.chip_id >= CHIP_ID_TL1)
				sts = _BIT(9);
			else
				rx_pr("no emp function\n");
		}
		pdec_ists_en &= ~sts;
		rx_pr("pdec_ists_en=0x%x\n", pdec_ists_en);
		/*disable irq*/
		hdmirx_wr_dwc(DWC_PDEC_IEN_CLR, sts);
	} else if (strncmp(input[1], "irqenable", 9) == 0) {
		sts = hdmirx_rd_dwc(DWC_PDEC_IEN);
		if (strncmp(input[2], "fifo", 4) == 0) {
			enable |= (PD_FIFO_START_PASS | PD_FIFO_OVERFL);
		} else if (strncmp(input[2], "drm", 3) == 0) {
			if (rx_info.chip_id == CHIP_ID_TXLX ||
			    rx_info.chip_id == CHIP_ID_TXHD)
				enable |= _BIT(30);
			else
				enable |= _BIT(9);
		} else if (strncmp(input[2], "gmd", 3) == 0) {
			enable |= _BIT(20);
		} else if (strncmp(input[2], "aif", 3) == 0) {
			enable |= _BIT(19);
		} else if (strncmp(input[2], "avi", 3) == 0) {
			enable |= _BIT(18);
		} else if (strncmp(input[2], "acr", 3) == 0) {
			enable |= _BIT(17);
		} else if (strncmp(input[2], "gcp", 3) == 0) {
			enable |= GCP_RCV;
		} else if (strncmp(input[2], "vsi", 3) == 0) {
			enable |= VSI_RCV;
		} else if (strncmp(input[2], "amp", 3) == 0) {
			enable |= _BIT(14);
		} else if (strncmp(input[2], "emp", 3) == 0) {
			if (rx_info.chip_id >= CHIP_ID_TL1)
				enable |= _BIT(9);
			else
				rx_pr("no emp function\n");
		}
		pdec_ists_en = enable | sts;
		rx_pr("pdec_ists_en=0x%x\n", pdec_ists_en);
		/*open irq*/
		hdmirx_wr_dwc(DWC_PDEC_IEN_SET, pdec_ists_en);
		/*hdmirx_irq_open()*/
	} else if (strncmp(input[1], "fifopkten", 9) == 0) {
		/*check input*/
		if (kstrtou32(input[2], 16, &res) < 0)
			return;
		rx_pr("pkt ctl disable:0x%x", res);
		/*check pkt enable ctl bit*/
		sts = rx_pkt_type_mapping((u32)res);
		if (sts == 0)
			return;

		packet_fifo_cfg |= sts;
		/* not work immediately ?? maybe int is not open*/
		enable = hdmirx_rd_dwc(DWC_PDEC_CTRL);
		enable |= sts;
		hdmirx_wr_dwc(DWC_PDEC_CTRL, enable);
	} else if (strncmp(input[1], "fifopktdis", 10) == 0) {
		/*check input*/
		if (kstrtou32(input[2], 16, &res) < 0)
			return;
		rx_pr("pkt ctl disable:0x%x", res);
		/*check pkt enable ctl bit*/
		sts = rx_pkt_type_mapping((u32)res);
		if (sts == 0)
			return;

		packet_fifo_cfg &= (~sts);
		/* not work immediately ?? maybe int is not open*/
		enable = hdmirx_rd_dwc(DWC_PDEC_CTRL);
		enable &= (~sts);
		hdmirx_wr_dwc(DWC_PDEC_CTRL, enable);
	} else if (strncmp(input[1], "contentchk", 10) == 0) {
		/*check input*/
		if (kstrtou32(input[2], 16, &res) < 0) {
			rx_pr("error input:fmt is 0xXX\n");
			return;
		}
		rx_pkt_content_chk_en(res, port);
	}
}

static void rx_pktdump_raw(void *pdata)
{
	u8 i;
	union infoframe_u *pktdata = pdata;

	rx_pr(">---raw data detail------>\n");
	rx_pr("HB0:0x%x\n", pktdata->raw_infoframe.pkttype);
	rx_pr("HB1:0x%x\n", pktdata->raw_infoframe.version);
	rx_pr("HB2:0x%x\n", pktdata->raw_infoframe.length);
	if (pktdata->raw_infoframe.pkttype != 0x7f)
		rx_pr("RSD:0x%x\n", pktdata->raw_infoframe.rsd);

	for (i = 0; i < 28; i++)
		rx_pr("PB%d:0x%x\n", i, pktdata->raw_infoframe.PB[i]);
	rx_pr(">------------------>end\n");
}

static void rx_pktdump_vsi(void *pdata)
{
	struct vsi_infoframe_st *pktdata = pdata;
	u32 i;

	if (!pktdata->ieee)
		return;

	rx_pr(">---vsi infoframe detail -------->\n");
	rx_pr("type: 0x%x\n", pktdata->pkttype);
	rx_pr("ver: %d\n", pktdata->ver_st.version);
	rx_pr("chgbit: %d\n", pktdata->ver_st.chgbit);
	rx_pr("length: %d\n", pktdata->length);
	rx_pr("ieee: 0x%x\n", pktdata->ieee);
	for (i = 0; i < 6; i++)
		rx_pr("payload %d : 0x%x\n", i,
		      pktdata->sbpkt.payload.data[i]);
	rx_pr(">------------------>end\n");
}

static void rx_pktdump_avi(void *pdata)
{
	struct avi_infoframe_st *pktdata = pdata;

	rx_pr(">---avi infoframe detail -------->\n");
	rx_pr("type: 0x%x\n", pktdata->pkttype);
	rx_pr("ver: %d\n", pktdata->version);
	rx_pr("length: %d\n", pktdata->length);

	if (pktdata->version == 1) {
		/*ver 1*/
		rx_pr("scaninfo S: 0x%x\n", pktdata->cont.v1.scaninfo);
		rx_pr("barinfo B : 0x%x\n", pktdata->cont.v1.barinfo);
		rx_pr("activeinfo A: 0x%x\n", pktdata->cont.v1.activeinfo);
		rx_pr("colorimetry Y: 0x%x\n",
		      pktdata->cont.v1.colorindicator);
		rx_pr("fmt_ration R: 0x%x\n", pktdata->cont.v1.fmt_ration);
		rx_pr("pic_ration M: 0x%x\n", pktdata->cont.v1.pic_ration);
		rx_pr("colorimetry C: 0x%x\n", pktdata->cont.v1.colorimetry);
		rx_pr("colorimetry SC: 0x%x\n", pktdata->cont.v1.pic_scaling);
	} else {
		/*ver 2/3/4 */
		rx_pr("scaninfo S: 0x%x\n", pktdata->cont.v4.scaninfo);
		rx_pr("barinfo B: 0x%x\n", pktdata->cont.v4.barinfo);
		rx_pr("activeinfo A: 0x%x\n", pktdata->cont.v4.activeinfo);
		rx_pr("colorimetry Y: 0x%x\n",
		      pktdata->cont.v4.colorindicator);
		rx_pr("fmt_ration R: 0x%x\n", pktdata->cont.v4.fmt_ration);
		rx_pr("pic_ration M: 0x%x\n", pktdata->cont.v4.pic_ration);
		rx_pr("colorimetry C: 0x%x\n", pktdata->cont.v4.colorimetry);
		rx_pr("pic_scaling SC: 0x%x\n", pktdata->cont.v4.pic_scaling);
		rx_pr("qt_range Q: 0x%x\n", pktdata->cont.v4.qt_range);
		rx_pr("ext_color EC : 0x%x\n", pktdata->cont.v4.ext_color);
		rx_pr("it_content ITC: 0x%x\n", pktdata->cont.v4.it_content);
		rx_pr("vic: 0x%x\n", pktdata->cont.v4.vic);
		rx_pr("pix_repeat PR: 0x%x\n",
		      pktdata->cont.v4.pix_repeat);
		rx_pr("content_type CN: 0x%x\n",
		      pktdata->cont.v4.content_type);
		rx_pr("ycc_range YQ: 0x%x\n",
		      pktdata->cont.v4.ycc_range);
	}
	rx_pr("line_end_topbar: 0x%x\n",
	      pktdata->line_num_end_topbar);
	rx_pr("line_start_btmbar: 0x%x\n",
	      pktdata->line_num_start_btmbar);
	rx_pr("pix_num_left_bar: 0x%x\n",
	      pktdata->pix_num_left_bar);
	rx_pr("pix_num_right_bar: 0x%x\n",
	      pktdata->pix_num_right_bar);
	rx_pr("additional_colorimetry: 0x%x\n", pktdata->additional_colorimetry);
	rx_pr(">------------------>end\n");
}

const struct spec_dev_table_s spd_white_list[SPEC_DEV_CNT] = {
	/* dev_type, spd_info */
	/* ps5 */
	{0x1, {0x53, 0x43, 0x45, 0x49, 0x0, 0x0, 0x0, 0x0, 0x50, 0x53, 0x35, 0x0, 0x0,
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x8}},
	/* xbox one*/
	{0x1, {0x4d, 0x53, 0x46, 0x54, 0x0, 0x0, 0x0, 0x0, 0x58, 0x62, 0x6f, 0x78,
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x8}},
	/* ps */
	{0x1, {0x53, 0x43, 0x45, 0x49, 0x0, 0x0, 0x0, 0x0, 0x50, 0x53, 0x34, 0x0,
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x8}},
	/* xbox one series*/
	{0x1, {0x4d, 0x53, 0x46, 0x54, 0x0, 0x0, 0x0, 0x0, 0x58, 0x62, 0x6f, 0x78,
		0x20, 0x4f, 0x6e, 0x65, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x8}},
	/* Panasonic BP UB820 */
	{0x2, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x50, 0x61, 0x6e, 0x61, 0x73, 0x6f,
		0x6e, 0x69, 0x63, 0x20, 0x42, 0x44, 0x0, 0x0, 0x0, 0x0, 0xa}}
	/*
	 * test dev,for debug only
	 * {0x41, 0x6d, 0x6c, 0x6f, 0x67, 0x69, 0x63, 0x0, 0x4d, 0x42, 0x6f, 0x78,
	 *	0x20, 0x4d, 0x65, 0x73, 0x6f, 0x6e, 0x20, 0x52, 0x65, 0x66, 0x0, 0x0, 0x1}
	 */
};

enum spec_dev_e rx_get_dev_type(u8 port)
{
	struct spd_infoframe_st *spdpkt;
	int i;

	//SPD
	spdpkt = (struct spd_infoframe_st *)&rx_pkt[port].spd_info;
	for (i = 0; i < SPEC_DEV_CNT; i++) {
		if (!memcmp((char *)spdpkt + 4, spd_white_list[i].spd_info, WHITE_LIST_SIZE)) {
			if (log_level & 0x1000)
				rx_pr("white dev=%d\n", i);
			rx[port].tx_type = spd_white_list[i].dev_type;
			return i;
		}
	}
	return i;
}

bool rx_is_xbox_dev(u8 port)
{
	u8 dev = rx_get_dev_type(port);

	if (dev == SPEC_DEV_XBOX || dev == SPEC_DEV_XBOX_SERIES)
		return true;
	else
		return false;
}

static void rx_pktdump_spd(void *pdata)
{
	struct spd_infoframe_st *pktdata = pdata;

	rx_pr(">---spd infoframe detail ---->\n");
	rx_pr("type: 0x%x\n", pktdata->pkttype);
	rx_pr("ver: %d\n", pktdata->version);
	rx_pr("length: %d\n", pktdata->length);
	rx_pr(">------------------>end\n");
}

static void rx_pktdump_aud(void *pdata)
{
	struct aud_infoframe_st *pktdata = pdata;

	rx_pr(">---audio infoframe detail ---->\n");

	rx_pr("type: 0x%x\n", pktdata->pkttype);
	rx_pr("ver: %d\n", pktdata->version);
	rx_pr("length: %d\n", pktdata->length);

	rx_pr("ch_count(CC):0x%x\n", pktdata->ch_count);
	rx_pr("coding_type(CT):0x%x\n", pktdata->coding_type);
	rx_pr("sample_size(SS):0x%x\n", pktdata->sample_size);
	rx_pr("sample_frq(SF):0x%x\n", pktdata->sample_frq);

	rx_pr("fromat(PB3):0x%x\n", pktdata->fromat);
	rx_pr("mul_ch(CA):0x%x\n", pktdata->ca);
	rx_pr("lfep(BL):0x%x\n", pktdata->lfep);
	rx_pr("level_shift_value(LSV):0x%x\n", pktdata->level_shift_value);
	rx_pr("down_mix(DM_INH):0x%x\n", pktdata->down_mix);
	rx_pr(">------------------>end\n");
}

static void rx_pktdump_drm(void *pdata)
{
	struct drm_infoframe_st *pktdata = pdata;
	union infoframe_u pktraw;

	memset(&pktraw, 0, sizeof(union infoframe_u));

	rx_pr(">---drm infoframe detail -------->\n");
	rx_pr("type: 0x%x\n", pktdata->pkttype);
	rx_pr("ver: %x\n", pktdata->version);
	rx_pr("length: %x\n", pktdata->length);

	rx_pr("b1 eotf: 0x%x\n", pktdata->des_u.tp1.eotf);
	rx_pr("b2 meta des id: 0x%x\n", pktdata->des_u.tp1.meta_des_id);

	rx_pr("dis_pri_x0: %x\n", pktdata->des_u.tp1.dis_pri_x0);
	rx_pr("dis_pri_y0: %x\n", pktdata->des_u.tp1.dis_pri_y0);
	rx_pr("dis_pri_x1: %x\n", pktdata->des_u.tp1.dis_pri_x1);
	rx_pr("dis_pri_y1: %x\n", pktdata->des_u.tp1.dis_pri_y1);
	rx_pr("dis_pri_x2: %x\n", pktdata->des_u.tp1.dis_pri_x2);
	rx_pr("dis_pri_y2: %x\n", pktdata->des_u.tp1.dis_pri_y2);
	rx_pr("white_points_x: %x\n", pktdata->des_u.tp1.white_points_x);
	rx_pr("white_points_y: %x\n", pktdata->des_u.tp1.white_points_y);
	rx_pr("max_dislum: %x\n", pktdata->des_u.tp1.max_dislum);
	rx_pr("min_dislum: %x\n", pktdata->des_u.tp1.min_dislum);
	rx_pr("max_light_lvl: %x\n", pktdata->des_u.tp1.max_light_lvl);
	rx_pr("max_fa_light_lvl: %x\n", pktdata->des_u.tp1.max_fa_light_lvl);
	rx_pr(">------------------>end\n");
}

static void rx_pktdump_acr(void *pdata)
{
	struct acr_pkt_st *pktdata = pdata;
	u32 CTS;
	u32 N;

	rx_pr(">---audio clock regeneration detail ---->\n");

	rx_pr("type: 0x%0x\n", pktdata->pkttype);

	CTS = (pktdata->sbpkt1.SB1_CTS_H << 16) |
			(pktdata->sbpkt1.SB2_CTS_M << 8) |
			pktdata->sbpkt1.SB3_CTS_L;

	N = (pktdata->sbpkt1.SB4_N_H << 16) |
			(pktdata->sbpkt1.SB5_N_M << 8) |
			pktdata->sbpkt1.SB6_N_L;
	rx_pr("sbpkt1 CTS: %d\n", CTS);
	rx_pr("sbpkt1 N : %d\n", N);
	rx_pr(">------------------>end\n");
}

static void rx_pktdump_emp(u8 port)
{
	struct emp_info_s *emp_info_p = rx_get_emp_info(port);
	u8 str[256];
	u8 pkt_cnt = 0;
	u8 i, j;
	u8 *pkt = NULL;
	u8 temp;

	if (!emp_info_p)
		return;
	pkt_cnt = emp_info_p->emp_pkt_cnt;
	pkt = emp_buf[rx[port].emp_vid_idx];
	if (pkt_cnt == 0)
		return;
	for (i = 0; i < pkt_cnt; i++) {
		memset(str, '\0', 256);
		for (j = 0; j < 32; j++)
			sprintf(str + strlen(str), "0x%x ", pkt[j]);
		rx_pr("raw_data:%s\n", str);
		if (!(log_level & PACKET_LOG) || pkt[0] != 0x7f)
			continue;
		rx_pr("\npkttype=0x%x\n", pkt[i * 31]);
		temp = pkt[i * 31 + 1];
		rx_pr("first=0x%x\n", (temp & _BIT(7)) >> 7);
		rx_pr("last=0x%x\n", (temp & _BIT(6)) >> 6);
		rx_pr("sequence_idx=0x%x\n", pkt[i * 31 + 2]);
		temp = pkt[i * 31 + 3];
		rx_pr("new=0x%x\n", (temp & _BIT(7)) >> 7);
		rx_pr("end=0x%x\n", (temp & _BIT(6)) >> 6);
		rx_pr("ds_type=0x%x\n", (temp & MSK(2, 4)) >> 4);
		rx_pr("afr=0x%x\n", (temp & _BIT(3)) >> 3);
		rx_pr("vfr=0x%x\n", (temp & _BIT(2)) >> 2);
		rx_pr("sync=0x%x\n", (temp & _BIT(1)) >> 1);
		rx_pr("or_id=0x%x\n", pkt[i * 31 + 5]);
		rx_pr("tag=0x%x\n", (pkt[i * 31 + 6] << 8) | pkt[i * 31 + 7]);
		rx_pr("length=0x%x\n", (pkt[i * 31 + 8] << 8) | pkt[i * 31 + 9]);
	}
}

static void rx_dump_aud_sample_pkt(u8 port)
{
	u32 i, j, tmp = 0;
	char str[256];
	struct emp_info_s *emp_p = NULL;

	if (rx[port].emp_vid_idx) {
		tmp = hdmirx_rd_top_common(TOP_EMP1_DDR_FILTER);
		hdmirx_wr_top_common(TOP_EMP1_DDR_FILTER, 1 << 1); //only audio sample pkt
		emp_p = &rx_info.emp_buff_b;
	} else {
		tmp = hdmirx_rd_top_common(TOP_EMP_DDR_FILTER);
		hdmirx_wr_top_common(TOP_EMP_DDR_FILTER, 1 << 1); //only audio sample pkt
		emp_p = &rx_info.emp_buff_a;
	}
	while (audio_debug) {
		for (i = 0; i < emp_p->emp_pkt_cnt; i++) {
			memset(str, '\0', sizeof(str));
			for (j = 0; j < 31; j++)
				sprintf(str + strlen(str), "0x%02x ",
					emp_buf[rx[port].emp_vid_idx][j + i * 31]);
			rx_pr("pkt[%2d]: %s\n", i, str);
		}
	}

	/* recover emp filter */
	hdmirx_wr_top_common(TOP_EMP_DDR_FILTER, tmp);
}

void rx_pkt_dump(enum pkt_type_e typeid, u8 port)
{
	struct packet_info_s *prx = &rx_pkt[port];
	union infoframe_u pktdata;
	int i;

	rx_pr("dump cmd:0x%x\n", typeid);

	/*mutex_lock(&pktbuff_lock);*/
	memset(&pktdata, 0, sizeof(pktdata));
	switch (typeid) {
	/*infoframe pkt*/
	case PKT_TYPE_INFOFRAME_VSI:
		for  (i = 0; i < VSI_TYPE_MAX; i++)
			rx_pktdump_vsi(&prx->multi_vs_info[i]);
		break;
	case PKT_TYPE_INFOFRAME_AVI:
		rx_pktdump_raw(&prx->avi_info);
		rx_pktdump_avi(&prx->avi_info);
		rx_pr("-------->ex register set >>\n");
		rx_pkt_get_avi_ex(&pktdata, port);
		rx_pktdump_avi(&pktdata);
		break;
	case PKT_TYPE_INFOFRAME_SPD:
		rx_pktdump_raw(&prx->spd_info);
		rx_pktdump_spd(&prx->spd_info);
		break;
	case PKT_TYPE_INFOFRAME_AUD:
		rx_pktdump_raw(&prx->aud_pktinfo);
		rx_pktdump_aud(&prx->aud_pktinfo);
		rx_pr("-------->ex register set >>\n");
		rx_pkt_get_audif_ex(&pktdata);
		rx_pktdump_raw(&pktdata);
		break;
	case PKT_TYPE_INFOFRAME_MPEGSRC:
		rx_pktdump_raw(&prx->mpegs_info);
		/*rx_pktdump_mpeg(&prx->mpegs_info);*/
		break;
	case PKT_TYPE_INFOFRAME_NVBI:
		rx_pktdump_raw(&prx->ntscvbi_info);
		/*rx_pktdump_ntscvbi(&prx->ntscvbi_info);*/
		rx_pr("-------->ex register set >>\n");
		rx_pkt_get_ntscvbi_ex(&pktdata);
		rx_pktdump_raw(&pktdata);
		break;
	case PKT_TYPE_INFOFRAME_DRM:
		rx_pktdump_raw(&prx->drm_info);
		rx_pktdump_drm(&prx->drm_info);
		rx_pr("-------->ex register set >>\n");
		rx_pkt_get_drm_ex(&pktdata);
		rx_pktdump_drm(&pktdata);
		break;
	/*other pkt*/
	case PKT_TYPE_ACR:
		rx_pktdump_raw(&prx->acr_info);
		rx_pktdump_acr(&prx->acr_info);
		rx_pr("-------->ex register set >>\n");
		rx_pkt_get_acr_ex(&pktdata);
		rx_pktdump_acr(&pktdata);
		break;
	case PKT_TYPE_GCP:
		rx_pktdump_raw(&prx->gcp_info);
		rx_pr("-------->ex register set >>\n");
		rx_pkt_get_gcp_ex(&pktdata);
		rx_pktdump_raw(&pktdata);
		break;
	case PKT_TYPE_ACP:
		rx_pktdump_raw(&prx->acp_info);
		break;
	case PKT_TYPE_ISRC1:
		rx_pktdump_raw(&prx->isrc1_info);
		break;
	case PKT_TYPE_ISRC2:
		rx_pktdump_raw(&prx->isrc2_info);
		break;
	case PKT_TYPE_GAMUT_META:
		rx_pktdump_raw(&prx->gameta_info);
		rx_pr("-------->ex register set >>\n");
		rx_pkt_get_gmd_ex(&pktdata);
		rx_pktdump_raw(&pktdata);
		break;
	case PKT_TYPE_AUD_META:
		rx_pktdump_raw(&prx->amp_info);
		rx_pr("-------->ex register set >>\n");
		rx_pkt_get_amp_ex(&pktdata);
		rx_pktdump_raw(&pktdata);
		break;
	case PKT_TYPE_EMP:
		rx_pktdump_emp(port);
		break;
	case PKT_TYPE_AUD_SAMPLE:
		rx_dump_aud_sample_pkt(port);
		break;
	default:
		rx_pr("warning: not support\n");
		rx_pr("vsi->0x81:Vendor-Specific infoframe\n");
		rx_pr("avi->0x82:Auxiliary video infoframe\n");
		rx_pr("spd->0x83:Source Product Description infoframe\n");
		rx_pr("aud->0x84:Audio infoframe\n");
		rx_pr("mpeg->0x85:MPEG infoframe\n");
		rx_pr("nvbi->0x86:NTSC VBI infoframe\n");
		rx_pr("drm->0x87:DRM infoframe\n");
		rx_pr("acr->0x01:audio clk regeneration\n");
		rx_pr("gcp->0x03\n");
		rx_pr("acp->0x04\n");
		rx_pr("isrc1->0x05\n");
		rx_pr("isrc2->0x06\n");
		rx_pr("gmd->0x0a\n");
		rx_pr("amp->0x0d\n");
		rx_pr("emp->0x7f:EMP\n");
		break;
	}
	/*mutex_unlock(&pktbuff_lock);*/
}

u32 rx_pkt_type_mapping(enum pkt_type_e pkt_type)
{
	struct pkt_type_reg_map_st *ptab = pkt_maping;
	u32 i = 0;
	u32 rt = 0;

	while (ptab[i].pkt_type != K_FLAG_TAB_END) {
		if (ptab[i].pkt_type == pkt_type)
			rt = ptab[i].reg_bit;
		i++;
	}
	return rt;
}

void rx_pkt_initial(u8 port)
{
	int i = port;
	int j = 0;
	struct emp_info_s *emp_info_p = rx_get_emp_info(port);

	rx[i].vs_info_details.vsi_state = E_VSI_NULL;
	//vsi info
	rx[i].vs_info_details._3d_structure = 0;
	rx[i].vs_info_details._3d_ext_data = 0;
	rx[i].threed_info.meta_data_flag = false;
	rx[i].vs_info_details.low_latency = false;
	rx[i].vs_info_details.backlt_md_bit = false;
	rx[i].vs_info_details.dv_allm = false;
	rx[i].vs_info_details.hdmi_allm = false;
	rx[i].vs_info_details.dolby_vision_flag = DV_NULL;
	rx[i].vs_info_details.hdr10plus = false;
	rx[i].vs_info_details.cuva_hdr = false;
	rx[i].vs_info_details.filmmaker = false;
	rx[i].vs_info_details.imax = false;
	//emp info
	rx[i].sbtm_info.flag = false;
	rx[i].vtem_info.vrr_en = false;
	rx[i].emp_dv_info.flag = false;
	rx[i].emp_cuva_info.flag = false;
	rx_pkt_clr_attach_drm(port);

	if (!emp_info_p) {
		rx_pr("%s emp info null\n", __func__);
		return;
	}
	emp_info_p->emp_pkt_cnt = 0;
	memset(&rxpktsts[i], 0, sizeof(struct rxpkt_st));
	while (j < VSI_TYPE_MAX)
		memset(&rx_pkt[i].multi_vs_info[j++], 0, sizeof(struct pd_infoframe_s));
	memset(&rx_pkt[i].avi_info, 0, sizeof(struct pd_infoframe_s));
	memset(&rx_pkt[i].spd_info, 0, sizeof(struct pd_infoframe_s));
	//memset(&rx_pkt[i].aud_pktinfo, 0, sizeof(struct pd_infoframe_s));
	memset(&rx_pkt[i].mpegs_info, 0, sizeof(struct pd_infoframe_s));
	memset(&rx_pkt[i].ntscvbi_info, 0, sizeof(struct pd_infoframe_s));
	memset(&rx_pkt[i].drm_info, 0, sizeof(struct pd_infoframe_s));
	memset(&rx_pkt[i].emp_info, 0, sizeof(struct pd_infoframe_s));
	memset(&rx_pkt[i].acr_info, 0, sizeof(struct pd_infoframe_s));
	memset(&rx_pkt[i].gcp_info, 0, sizeof(struct pd_infoframe_s));
	memset(&rx_pkt[i].acp_info, 0, sizeof(struct pd_infoframe_s));
	memset(&rx_pkt[i].isrc1_info, 0, sizeof(struct pd_infoframe_s));
	memset(&rx_pkt[i].isrc2_info, 0, sizeof(struct pd_infoframe_s));
	memset(&rx_pkt[i].gameta_info, 0, sizeof(struct pd_infoframe_s));
	memset(&rx_pkt[i].amp_info, 0, sizeof(struct pd_infoframe_s));
}

/*please ignore checksum byte*/
void rx_pkt_get_audif_ex(void *pktinfo)
{
	struct aud_infoframe_st *pkt = pktinfo;

	if (!pktinfo) {
		rx_pr("pkinfo null\n");
		return;
	}

	/*memset(pkt, 0, sizeof(struct aud_infoframe_st));*/

	pkt->pkttype = PKT_TYPE_INFOFRAME_AUD;
	pkt->version =
		hdmirx_rd_bits_dwc(DWC_PDEC_AIF_PB0, MSK(8, 0));
	pkt->length =
		hdmirx_rd_bits_dwc(DWC_PDEC_AIF_PB0, MSK(5, 8));
	pkt->checksum =
		hdmirx_rd_bits_dwc(DWC_PDEC_AIF_PB0, MSK(8, 16));
	pkt->rsd = 0;

	/*get AudioInfo */
	pkt->coding_type =
		hdmirx_rd_bits_dwc(DWC_PDEC_AIF_PB0, CODING_TYPE);
	pkt->ch_count =
		hdmirx_rd_bits_dwc(DWC_PDEC_AIF_PB0, CHANNEL_COUNT);
	pkt->sample_frq =
		hdmirx_rd_bits_dwc(DWC_PDEC_AIF_PB0, SAMPLE_FREQ);
	pkt->sample_size =
		hdmirx_rd_bits_dwc(DWC_PDEC_AIF_PB0, SAMPLE_SIZE);
	pkt->fromat =
		hdmirx_rd_bits_dwc(DWC_PDEC_AIF_PB0, AIF_DATA_BYTE_3);
	pkt->ca =
		hdmirx_rd_bits_dwc(DWC_PDEC_AIF_PB0, CH_SPEAK_ALLOC);
	pkt->down_mix =
		hdmirx_rd_bits_dwc(DWC_PDEC_AIF_PB1, DWNMIX_INHIBIT);
	pkt->level_shift_value =
		hdmirx_rd_bits_dwc(DWC_PDEC_AIF_PB1, LEVEL_SHIFT_VAL);
	pkt->lfep =
		hdmirx_rd_bits_dwc(DWC_PDEC_AIF_PB1, MSK(2, 8));
}

void rx_pkt_get_acr_ex(void *pktinfo)
{
	struct acr_pkt_st *pkt = pktinfo;
	u32 N, CTS;

	if (!pktinfo) {
		rx_pr("pkinfo null\n");
		return;
	}

	/*memset(pkt, 0, sizeof(struct acr_pkt_st));*/

	pkt->pkttype = PKT_TYPE_ACR;
	pkt->zero0 = 0x0;
	pkt->zero1 = 0x0;

	CTS = hdmirx_rd_dwc(DWC_PDEC_ACR_CTS);
	N = hdmirx_rd_dwc(DWC_PDEC_ACR_N);

	pkt->sbpkt1.SB1_CTS_H = ((CTS >> 16) & 0xf);
	pkt->sbpkt1.SB2_CTS_M = ((CTS >> 8) & 0xff);
	pkt->sbpkt1.SB3_CTS_L = (CTS & 0xff);
	pkt->sbpkt1.SB4_N_H = ((N >> 16) & 0xf);
	pkt->sbpkt1.SB5_N_M = ((N >> 8) & 0xff);
	pkt->sbpkt1.SB6_N_L = (N & 0xff);

	pkt->sbpkt2.SB1_CTS_H = ((CTS >> 16) & 0xf);
	pkt->sbpkt2.SB2_CTS_M = ((CTS >> 8) & 0xff);
	pkt->sbpkt2.SB3_CTS_L = (CTS & 0xff);
	pkt->sbpkt2.SB4_N_H = ((N >> 16) & 0xf);
	pkt->sbpkt2.SB5_N_M = ((N >> 8) & 0xff);
	pkt->sbpkt2.SB6_N_L = (N & 0xff);

	pkt->sbpkt3.SB1_CTS_H = ((CTS >> 16) & 0xf);
	pkt->sbpkt3.SB2_CTS_M = ((CTS >> 8) & 0xff);
	pkt->sbpkt3.SB3_CTS_L = (CTS & 0xff);
	pkt->sbpkt3.SB4_N_H = ((N >> 16) & 0xf);
	pkt->sbpkt3.SB5_N_M = ((N >> 8) & 0xff);
	pkt->sbpkt3.SB6_N_L = (N & 0xff);

	pkt->sbpkt4.SB1_CTS_H = ((CTS >> 16) & 0xf);
	pkt->sbpkt4.SB2_CTS_M = ((CTS >> 8) & 0xff);
	pkt->sbpkt4.SB3_CTS_L = (CTS & 0xff);
	pkt->sbpkt4.SB4_N_H = ((N >> 16) & 0xf);
	pkt->sbpkt4.SB5_N_M = ((N >> 8) & 0xff);
	pkt->sbpkt4.SB6_N_L = (N & 0xff);
}

/*please ignore checksum byte*/
void rx_pkt_get_avi_ex(void *pktinfo, u8 port)
{
	struct avi_infoframe_st *pkt = pktinfo;
	u8 data;

	if (!pktinfo || rx_info.chip_id < CHIP_ID_T7) {
		rx_pr("pkinfo null\n");
		return;
	}

	pkt->pkttype = PKT_TYPE_INFOFRAME_AVI;
	pkt->version = hdmirx_rd_cor(AVIRX_VERS_DP2_IVCRX, port);
	pkt->length = hdmirx_rd_cor(AVIRX_LENGTH_DP2_IVCRX, port);
	pkt->checksum = hdmirx_rd_cor(AVIRX_CHSUM_DP2_IVCRX, port);
	/* AVI parameters */
	data = hdmirx_rd_cor(AVIRX_DBYTE1_DP2_IVCRX, port);
	pkt->cont.v4.scaninfo = data & 0x3;
	pkt->cont.v4.barinfo = (data >> 2) & 0x3;
	pkt->cont.v4.activeinfo = (data >> 4) & 0x1;
	pkt->cont.v4.colorindicator = (data >> 5) & 0x7;

	data = hdmirx_rd_cor(AVIRX_DBYTE2_DP2_IVCRX, port);
	pkt->cont.v4.fmt_ration = data & 0xf;
	pkt->cont.v4.pic_ration = (data >> 4) & 0x3;
	pkt->cont.v4.colorimetry = (data >> 6) & 0x3;

	data = hdmirx_rd_cor(AVIRX_DBYTE3_DP2_IVCRX, port);
	pkt->cont.v4.pic_scaling = data & 0x3;
	pkt->cont.v4.qt_range = (data >> 2) & 0x3;
	pkt->cont.v4.ext_color = (data >> 4) & 0x7;
	pkt->cont.v4.it_content = (data >> 7) & 0x1;
	pkt->cont.v4.vic = hdmirx_rd_cor(AVIRX_DBYTE4_DP2_IVCRX, port);

	data = hdmirx_rd_cor(AVIRX_DBYTE5_DP2_IVCRX, port);
	pkt->cont.v4.pix_repeat = data & 0xf;
	pkt->cont.v4.content_type = (data >> 4) & 0x3;
	pkt->cont.v4.ycc_range = (data >> 6) & 0x3;

	pkt->line_num_end_topbar = hdmirx_rd_cor(AVIRX_DBYTE6_DP2_IVCRX, port) |
		(u16)hdmirx_rd_cor(AVIRX_DBYTE7_DP2_IVCRX, port) << 8;
	pkt->line_num_start_btmbar = hdmirx_rd_cor(AVIRX_DBYTE8_DP2_IVCRX, port) |
		(u16)hdmirx_rd_cor(AVIRX_DBYTE9_DP2_IVCRX, port) << 8;
	pkt->pix_num_left_bar = hdmirx_rd_cor(AVIRX_DBYTE10_DP2_IVCRX, port) |
		(u16)hdmirx_rd_cor(AVIRX_DBYTE11_DP2_IVCRX, port) << 8;
	pkt->pix_num_right_bar = hdmirx_rd_cor(AVIRX_DBYTE12_DP2_IVCRX, port) |
		(u16)hdmirx_rd_cor(AVIRX_DBYTE13_DP2_IVCRX, port) << 8;
	pkt->additional_colorimetry = hdmirx_rd_cor(AVIRX_DBYTE14_DP2_IVCRX, port);
}

void rx_pkt_get_spd_ex(void *pktinfo, u8 port)
{
	struct spd_infoframe_st *pkt = pktinfo;
	int i = 0;

	if (!pktinfo) {
		rx_pr("pkinfo null\n");
		return;
	}

	pkt->pkttype = PKT_TYPE_INFOFRAME_SPD;
	pkt->pkttype = hdmirx_rd_cor(SPDRX_TYPE_DP2_IVCRX, port);
	if (pkt->pkttype != PKT_TYPE_INFOFRAME_SPD)
		;//rx_pr("wrong SPD\n");
	pkt->version = hdmirx_rd_cor(SPDRX_VERS_DP2_IVCRX, port);
	pkt->length = hdmirx_rd_cor(SPDRX_LENGTH_DP2_IVCRX, port);
	pkt->checksum = hdmirx_rd_cor(SPDRX_CHSUM_DP2_IVCRX, port);
	for (i = 0; i < 27; i++)
		pkt->des_u.data[i] = hdmirx_rd_cor(SPDRX_DBYTE1_DP2_IVCRX + i, port);
}

void rx_pkt_get_vsi_ex(void *pktinfo, u8 port)
{
	struct vsi_infoframe_st *pkt = pktinfo;
	u32 st0;
	u32 st1;
	u8 tmp, i;
	u32 tmp32;

	if (!pktinfo) {
		rx_pr("pkinfo null\n");
		return;
	}
	if (rx_info.chip_id >= CHIP_ID_T7) {
		if (log_level & IRQ_LOG)
			rx_pr("type = %x\n", rx_vsif_type[port]);
		/* must use if_else to decide priority */
		if (rx_vsif_type[port] & VSIF_TYPE_DV15) {
			pkt->pkttype = PKT_TYPE_INFOFRAME_VSI;
			pkt->length = hdmirx_rd_cor(HF_VSIRX_LENGTH_DP3_IVCRX, port) & 0x1f;
			pkt->ieee = IEEE_DV15;
			tmp = hdmirx_rd_cor(HF_VSIRX_VERS_DP3_IVCRX, port);
			pkt->ver_st.version = tmp & 0x7f;
			pkt->ver_st.chgbit = tmp >> 7 & 1;
			pkt->checksum = hdmirx_rd_cor(HF_VSIRX_DBYTE0_DP3_IVCRX, port);
			tmp = hdmirx_rd_cor(HF_VSIRX_DBYTE4_DP3_IVCRX, port);
			pkt->sbpkt.vsi_dobv15.ll = tmp & 1;
			if (log_level & VSI_LOG)
				rx_pr("LL=%d\n", pkt->sbpkt.vsi_dobv15.ll);
			pkt->sbpkt.vsi_dobv15.dv_vs10_sig_type = tmp >> 1 & 0x0f;
			pkt->sbpkt.vsi_dobv15.source_dm_ver = tmp >> 5 & 7;
			tmp = hdmirx_rd_cor(HF_VSIRX_DBYTE5_DP3_IVCRX, port);
			pkt->sbpkt.vsi_dobv15.tmax_pq_hi = tmp & 0x0f;
			pkt->sbpkt.vsi_dobv15.aux_md = tmp >> 6 & 1;
			pkt->sbpkt.vsi_dobv15.bklt_md = tmp >> 7 & 1;
			tmp = hdmirx_rd_cor(HF_VSIRX_DBYTE6_DP3_IVCRX, port);
			pkt->sbpkt.vsi_dobv15.tmax_pq_lo = tmp;
			tmp = hdmirx_rd_cor(HF_VSIRX_DBYTE7_DP3_IVCRX, port);
			pkt->sbpkt.vsi_dobv15.aux_run_mode = tmp;
			tmp = hdmirx_rd_cor(HF_VSIRX_DBYTE8_DP3_IVCRX, port);
			pkt->sbpkt.vsi_dobv15.aux_run_ver = tmp;
			tmp = hdmirx_rd_cor(HF_VSIRX_DBYTE9_DP3_IVCRX, port);
			pkt->sbpkt.vsi_dobv15.aux_debug = tmp;
			tmp = hdmirx_rd_cor(HF_VSIRX_DBYTE10_DP3_IVCRX, port);
			pkt->sbpkt.vsi_dobv15.content_type = tmp;
			for (i = 0; i < 17; i++) {
				tmp = hdmirx_rd_cor(HF_VSIRX_DBYTE11_DP3_IVCRX + i, port);
				pkt->sbpkt.vsi_dobv15.data[i] = tmp;
			}
			if (rx_vsif_type[port] & VSIF_TYPE_HDMI21) {
				tmp = hdmirx_rd_cor(VSIRX_DBYTE5_DP3_IVCRX, port);
				if ((tmp >> 1) & 1)
					pkt->ieee = IEEE_DV_PLUS_ALLM;
			}
			tmp32 = hdmirx_rd_cor(HF_VSIRX_DBYTE4_DP3_IVCRX, port);
			tmp32 |= hdmirx_rd_cor(HF_VSIRX_DBYTE5_DP3_IVCRX, port) << 8;
			tmp32 |= hdmirx_rd_cor(HF_VSIRX_DBYTE6_DP3_IVCRX, port) << 16;
			tmp32 |= hdmirx_rd_cor(HF_VSIRX_DBYTE7_DP3_IVCRX, port) << 24;
			pkt->sbpkt.payload.data[0] = tmp32;

			tmp32 = hdmirx_rd_cor(HF_VSIRX_DBYTE8_DP3_IVCRX, port);
			tmp32 |= hdmirx_rd_cor(HF_VSIRX_DBYTE9_DP3_IVCRX, port) << 8;
			tmp32 |= hdmirx_rd_cor(HF_VSIRX_DBYTE10_DP3_IVCRX, port) << 16;
			tmp32 |= hdmirx_rd_cor(HF_VSIRX_DBYTE11_DP3_IVCRX, port) << 24;
			pkt->sbpkt.payload.data[1] = tmp32;
		} else if (rx_vsif_type[port] & VSIF_TYPE_HDR10P) {
			pkt->pkttype = PKT_TYPE_INFOFRAME_VSI;
			pkt->length = hdmirx_rd_cor(AUDRX_TYPE_DP2_IVCRX, port) & 0x1f;
			pkt->ieee = IEEE_HDR10PLUS;
			if (rx_vsif_type[port] & VSIF_TYPE_HDMI21) {
				tmp = hdmirx_rd_cor(VSIRX_DBYTE5_DP3_IVCRX, port);
				if ((tmp >> 1) & 1)
					pkt->ieee = IEEE_HDR10P_PLUS_ALLM;
			}
			for (i = 0; i < 24; i++) {
				tmp = hdmirx_rd_cor(AUDRX_DBYTE4_DP2_IVCRX + i, port);
				pkt->sbpkt.vsi_st.data[i] = tmp;
			}
		} else if (rx_vsif_type[port] & VSIF_TYPE_HDMI21) {
			pkt->pkttype = PKT_TYPE_INFOFRAME_VSI;
			pkt->ieee = IEEE_VSI21;
			pkt->sbpkt.vsi_st_21.ver = 1;
			tmp = hdmirx_rd_cor(VSIRX_DBYTE5_DP3_IVCRX, port);
			pkt->sbpkt.vsi_st_21.valid_3d = tmp & 1;
			pkt->sbpkt.vsi_st_21.allm_mode = (tmp >> 1) & 1;
			pkt->sbpkt.vsi_st_21.ccbpc = (tmp >> 4) & 0x0f;
			for (i = 0; i < 21; i++) {
				tmp = hdmirx_rd_cor(VSIRX_DBYTE6_DP3_IVCRX + i, port);
				pkt->sbpkt.vsi_st_21.data[i] = tmp;
			}
		} else if (rx_vsif_type[port] & VSIF_TYPE_HDMI14) {
			pkt->pkttype = PKT_TYPE_INFOFRAME_VSI;
			pkt->length = hdmirx_rd_cor(RX_UNREC_BYTE3_DP2_IVCRX, port) & 0x1f;
			pkt->ver_st.version = 1;//hdmirx_rd_cor(VSIRX_VERS_DP3_IVCRX, port);
			pkt->ieee = IEEE_VSI14;
			for (i = 0; i < 24; i++) {
				tmp = hdmirx_rd_cor(RX_UNREC_BYTE8_DP2_IVCRX + i, port);
				pkt->sbpkt.vsi_st.data[i] = tmp;
			}
		}
	} else if (rx_info.chip_id != CHIP_ID_TXHD) {
		st0 = hdmirx_rd_dwc(DWC_PDEC_VSI_ST0);
		st1 = hdmirx_rd_dwc(DWC_PDEC_VSI_ST1);
		pkt->pkttype = PKT_TYPE_INFOFRAME_VSI;
		pkt->length = st1 & 0x1f;
		pkt->checksum = (st0 >> 24) & 0xff;
		pkt->ieee = st0 & 0xffffff;
		pkt->ver_st.version = 1;
		pkt->ver_st.chgbit = 0;
		pkt->sbpkt.payload.data[0] =
			hdmirx_rd_dwc(DWC_PDEC_VSI_PLAYLOAD0);
		pkt->sbpkt.payload.data[1] =
			hdmirx_rd_dwc(DWC_PDEC_VSI_PLAYLOAD1);
		pkt->sbpkt.payload.data[2] =
			hdmirx_rd_dwc(DWC_PDEC_VSI_PLAYLOAD2);
		pkt->sbpkt.payload.data[3] =
			hdmirx_rd_dwc(DWC_PDEC_VSI_PLAYLOAD3);
		pkt->sbpkt.payload.data[4] =
			hdmirx_rd_dwc(DWC_PDEC_VSI_PLAYLOAD4);
		pkt->sbpkt.payload.data[5] =
			hdmirx_rd_dwc(DWC_PDEC_VSI_PLAYLOAD5);
	}
}

/*return 32 byte data , data struct see register spec*/
void rx_pkt_get_amp_ex(void *pktinfo)
{
	struct pd_infoframe_s *pkt = pktinfo;
	u32 HB;

	if (!pktinfo) {
		rx_pr("pkinfo null\n");
		return;
	}

	/*memset(pkt, 0, sizeof(struct pd_infoframe_s));*/
	if (rx_info.chip_id != CHIP_ID_TXHD) {
		HB = hdmirx_rd_dwc(DWC_PDEC_AMP_HB);
		pkt->HB = (HB << 8) | PKT_TYPE_AUD_META;
		pkt->PB0 = hdmirx_rd_dwc(DWC_PDEC_AMP_PB0);
		pkt->PB1 = hdmirx_rd_dwc(DWC_PDEC_AMP_PB1);
		pkt->PB2 = hdmirx_rd_dwc(DWC_PDEC_AMP_PB2);
		pkt->PB3 = hdmirx_rd_dwc(DWC_PDEC_AMP_PB3);
		pkt->PB4 = hdmirx_rd_dwc(DWC_PDEC_AMP_PB4);
		pkt->PB5 = hdmirx_rd_dwc(DWC_PDEC_AMP_PB5);
		pkt->PB6 = hdmirx_rd_dwc(DWC_PDEC_AMP_PB6);
	}
}

void rx_pkt_get_gmd_ex(void *pktinfo)
{
	struct gamutmeta_pkt_st *pkt = pktinfo;
	u32 HB, PB;

	if (!pktinfo) {
		rx_pr("pkinfo null\n");
		return;
	}

	/*memset(pkt, 0, sizeof(struct gamutmeta_pkt_st));*/

	pkt->pkttype = PKT_TYPE_GAMUT_META;

	HB = hdmirx_rd_dwc(DWC_PDEC_GMD_HB);
	PB = hdmirx_rd_dwc(DWC_PDEC_GMD_PB);

	/*3:0*/
	pkt->affect_seq_num = (HB & 0xf);
	/*6:4*/
	pkt->gbd_profile = ((HB >> 4) & 0x7);
	/*7*/
	pkt->next_field = ((HB >> 7) & 0x1);
	/*11:8*/
	pkt->cur_seq_num = ((HB >> 8) & 0xf);
	/*13:12*/
	pkt->pkt_seq = ((HB >> 12) & 0x3);
	/*15*/
	pkt->no_cmt_gbd = ((HB >> 15) & 0x1);

	pkt->sbpkt.p1_profile.gbd_length_l = (PB & 0xff);
	pkt->sbpkt.p1_profile.gbd_length_h = ((PB >> 8) & 0xff);
	pkt->sbpkt.p1_profile.checksum = ((PB >> 16) & 0xff);
}

void rx_pkt_get_gcp_ex(void *pktinfo)
{
	struct gcp_pkt_st *pkt = pktinfo;
	u32 gcpavmute;

	if (!pktinfo) {
		rx_pr("pkinfo null\n");
		return;
	}

	/*memset(pkt, 0, sizeof(struct gcp_pkt_st));*/

	gcpavmute = hdmirx_rd_dwc(DWC_PDEC_GCP_AVMUTE);

	pkt->pkttype = PKT_TYPE_GCP;
	pkt->sbpkt.clr_avmute = (gcpavmute & 0x01);
	pkt->sbpkt.set_avmute = ((gcpavmute >> 1) & 0x01);
	pkt->sbpkt.colordepth = ((gcpavmute >> 4) & 0xf);
	pkt->sbpkt.pixel_pkg_phase = ((gcpavmute >> 8) & 0xf);
	pkt->sbpkt.def_phase = ((gcpavmute >> 12) & 0x01);
}

void rx_pkt_get_drm_ex(void *pktinfo)
{
	struct drm_infoframe_st *drmpkt = pktinfo;

	if (!pktinfo) {
		rx_pr("pkinfo null\n");
		return;
	}

	drmpkt->pkttype = PKT_TYPE_INFOFRAME_DRM;
	if (rx_info.chip_id != CHIP_ID_TXHD) {
		drmpkt->length = (hdmirx_rd_dwc(DWC_PDEC_DRM_HB) >> 8);
		drmpkt->version = hdmirx_rd_dwc(DWC_PDEC_DRM_HB);

		drmpkt->des_u.payload[0] =
			hdmirx_rd_dwc(DWC_PDEC_DRM_PAYLOAD0);
		drmpkt->des_u.payload[1] =
			hdmirx_rd_dwc(DWC_PDEC_DRM_PAYLOAD1);
		drmpkt->des_u.payload[2] =
			hdmirx_rd_dwc(DWC_PDEC_DRM_PAYLOAD2);
		drmpkt->des_u.payload[3] =
			hdmirx_rd_dwc(DWC_PDEC_DRM_PAYLOAD3);
		drmpkt->des_u.payload[4] =
			hdmirx_rd_dwc(DWC_PDEC_DRM_PAYLOAD4);
		drmpkt->des_u.payload[5] =
			hdmirx_rd_dwc(DWC_PDEC_DRM_PAYLOAD5);
		drmpkt->des_u.payload[6] =
			hdmirx_rd_dwc(DWC_PDEC_DRM_PAYLOAD6);
	}
}

/*return 32 byte data , data struct see register spec*/
void rx_pkt_get_ntscvbi_ex(void *pktinfo)
{
	struct pd_infoframe_s *pkt = pktinfo;

	if (!pktinfo) {
		rx_pr("pkinfo null\n");
		return;
	}

	if (rx_info.chip_id != CHIP_ID_TXHD &&
		rx_info.chip_id != CHIP_ID_T5D) {
		/*byte 0 , 1*/
		pkt->HB = hdmirx_rd_dwc(DWC_PDEC_NTSCVBI_HB);
		pkt->PB0 = hdmirx_rd_dwc(DWC_PDEC_NTSCVBI_PB0);
		pkt->PB1 = hdmirx_rd_dwc(DWC_PDEC_NTSCVBI_PB1);
		pkt->PB2 = hdmirx_rd_dwc(DWC_PDEC_NTSCVBI_PB2);
		pkt->PB3 = hdmirx_rd_dwc(DWC_PDEC_NTSCVBI_PB3);
		pkt->PB4 = hdmirx_rd_dwc(DWC_PDEC_NTSCVBI_PB4);
		pkt->PB5 = hdmirx_rd_dwc(DWC_PDEC_NTSCVBI_PB5);
		pkt->PB6 = hdmirx_rd_dwc(DWC_PDEC_NTSCVBI_PB6);
	}
}

//todo
u32 rx_pkt_chk_attach_vsi(u8 port)
{
	if (rxpktsts[port].pkt_attach_vsi)
		return 1;
	else
		return 0;
}

//todo
void rx_pkt_clr_attach_vsi(u8 port)
{
	u8 i = 0;

	rxpktsts[port].pkt_attach_vsi = 0;
	while (i < VSI_TYPE_MAX)
		memset(&rx_pkt[port].multi_vs_info[i++], 0, sizeof(struct pd_infoframe_s));
}

u32 rx_pkt_chk_updated_spd(u8 port)
{
	if (rxpktsts[port].pkt_spd_updated)
		return 1;
	else
		return 0;
}

void rx_pkt_clr_updated_spd(u8 port)
{
	rxpktsts[port].pkt_spd_updated = 0;
}

u32 rx_pkt_chk_attach_drm(u8 port)
{
	if (rxpktsts[port].pkt_attach_drm)
		return 1;
	else
		return 0;
}

void rx_pkt_clr_attach_drm(u8 port)
{
	rxpktsts[port].pkt_attach_drm = 0;
}

void rx_check_pkt_flag(u8 port)
{
	if (!(rxpktsts[port].pkt_op_flag & PKT_OP_VSI))
		rx_pkt_clr_attach_vsi(port);
	//other pkts, todo
}

u32 rx_pkt_chk_busy_vsi(u8 port)
{
	if (rxpktsts[port].pkt_op_flag & PKT_OP_VSI)
		return 1;
	else
		return 0;
}

u32 rx_pkt_chk_busy_drm(u8 port)
{
	if (rxpktsts[port].pkt_op_flag & PKT_OP_DRM)
		return 1;
	else
		return 0;
}

bool rx_chk_avi_valid(u8 port)
{
	u32 chk = 0;
	u32 i;

	for (i = AVIRX_TYPE_DP2_IVCRX; i <= AVIRX_DBYTE15_DP2_IVCRX; i++)
		chk += hdmirx_rd_cor(i, port);

	if (chk & 0xff)
		return false;
	else
		return true;
}

/*  version2.86 ieee-0x00d046, length 0x1B
 *	pb4 bit[0]: Low_latency
 *	pb4 bit[1]: Dolby_vision_signal
 *	pb5 bit[7]: Backlt_Ctrl_MD_Present
 *	pb5 bit[3:0] | pb6 bit[7:0]: Eff_tmax_PQ
 *
 *	version2.6 ieee-0x000c03,
 *	start: length 0x18
 *	stop: 0x05,0x04
 */
void rx_get_vsi_info(u8 port)
{
	struct vsi_infoframe_st *pkt;
	unsigned int tmp;
	static u32 num;
	struct emp_info_s *emp_info_p = rx_get_emp_info(port);

	if (!emp_info_p) {
		rx_pr("%s emp info null\n", __func__);
		return;
	}
	rx[port].vs_info_details.emp_pkt_cnt = emp_info_p->emp_pkt_cnt;

	int i  = 0;

	while (i < VSI_TYPE_MAX) {
		pkt = (struct vsi_infoframe_st *)&rx_pkt[port].multi_vs_info[i];
		if ((log_level & PACKET_LOG) && pkt->ieee && rx[port].new_emp_pkt)
			rx_pr("ieee[%d/%d]-%x\n", i, VSI_TYPE_MAX, pkt->ieee);
		if (pkt->ieee == IEEE_DV15) {
			/* dolbyvision 1.5 */
			if (pkt->length != E_PKT_LENGTH_27) {
				if (log_level & PACKET_LOG)
					rx_pr("vsi dv15 length err\n");
				//dv vsif
				rx[port].vs_info_details.dolby_vision_flag = DV_VSIF;
				i++;
				continue;
			}
			tmp = pkt->sbpkt.payload.data[0] & _BIT(1);
			if (tmp) {
				rx[port].vs_info_details.vsi_state |= E_VSI_DV15;
				rx[port].vs_info_details.dolby_vision_flag = DV_VSIF;
				tmp = pkt->sbpkt.payload.data[0] & _BIT(0);
				rx[port].vs_info_details.low_latency =
					tmp ? true : false;
				tmp = pkt->sbpkt.payload.data[0] >> 15 & 0x01;
				rx[port].vs_info_details.backlt_md_bit =
					tmp ? true : false;
				if (tmp) {
					tmp = (pkt->sbpkt.payload.data[0] >> 16 & 0xff) |
					      (pkt->sbpkt.payload.data[0] & 0xf00);
					rx[port].vs_info_details.eff_tmax_pq = tmp;
				}
				tmp = (pkt->sbpkt.payload.data[1] >> 16 & 0xf);
				if (tmp == 2)
					rx[port].vs_info_details.dv_allm = true;
			}
		} else if (pkt->ieee == IEEE_HDR10PLUS) {
			/* HDR10+ */
			if (pkt->length != E_PKT_LENGTH_27 ||
			    pkt->pkttype != 0x01 ||
			    pkt->ver_st.version != 0x01 ||
			    ((pkt->sbpkt.payload.data[1] >> 6) & 0x03) != 0x01)
				if (log_level & PACKET_LOG)
					rx_pr("vsi hdr10+ length err\n");
			/* consider hdr10+ is true when IEEE matched */
			rx[port].vs_info_details.vsi_state |= E_VSI_HDR10PLUS;
			rx[port].vs_info_details.hdr10plus = true;
		} else if (pkt->ieee == IEEE_CUVAHDR) {
			if (pkt->length != E_PKT_LENGTH_27) {
				if (log_level & PACKET_LOG)
					rx_pr("cuva hdr length err\n");
			}
			tmp = pkt->sbpkt.payload.data[0] & 0xff;
			rx[port].vs_info_details.sys_start_code = tmp;
			tmp = (pkt->sbpkt.payload.data[1] >> 4) & 0xf;
			rx[port].vs_info_details.cuva_version_code = tmp;
			pkt->sbpkt.vsi_cuva_hdr.transfer_char = 0;
			pkt->sbpkt.vsi_cuva_hdr.monitor_mode_enable = 1;
			rx[port].vs_info_details.vsi_state |= E_VSI_CUVAHDR;
			rx[port].vs_info_details.cuva_hdr = true;
		} else if (pkt->ieee == IEEE_FILMMAKER) {
			if (pkt->length != E_PKT_LENGTH_5 ||
				pkt->pkttype != 0x01 ||
				pkt->ver_st.version != 0x01 ||
				pkt->sbpkt.payload.data[0] != 0x01 ||
				pkt->sbpkt.payload.data[1] != 0x00)
				if (log_level & PACKET_LOG)
					rx_pr("vsi filmmaker pkt err\n");
			rx[port].vs_info_details.vsi_state |= E_VSI_FILMMAKER;
			rx[port].vs_info_details.filmmaker = true;
		} else if (pkt->ieee == IEEE_IMAX) {
			if (pkt->length != E_PKT_LENGTH_5 ||
				pkt->ver_st.version != 0x01 ||
				pkt->sbpkt.vsi_st.data[0] != 0x01 ||
				pkt->sbpkt.payload.data[1] != 0x01)
				if (log_level & PACKET_LOG)
					rx_pr("vsi imax pkt err\n");
			rx[port].vs_info_details.vsi_state |= E_VSI_IMAX;
			rx[port].vs_info_details.imax = true;
		}

		if (pkt->ieee == IEEE_VSI14) {
			/* dolbyvision1.0 */
			if (pkt->length == E_PKT_LENGTH_24) {
				/* DV10: PB4-0x00/0x20, PB5-0/1/2/3/4 */
				if ((pkt->sbpkt.vsi_dobv10.vdfmt == 0x00 ||
					pkt->sbpkt.vsi_dobv10.vdfmt == 0x20) &&
					pkt->sbpkt.vsi_dobv10.hdmi_vic <= 0x4) {
					if ((pkt->sbpkt.payload.data[0] & 0xffff) == 0)
						pkt->sbpkt.payload.data[0] = 0xffff;
					rx[port].vs_info_details.dolby_vision_flag = DV_VSIF;
					rx[port].vs_info_details.vsi_state |= E_VSI_DV10;
					if (log_level & PACKET_LOG)
						rx_pr("IEEE_VSI14 DV10\n");
				}
			} else if ((pkt->length == E_PKT_LENGTH_5) &&
				(pkt->sbpkt.payload.data[0] & 0xffff)) {
				rx[port].vs_info_details.dolby_vision_flag = DV_NULL;
			} else if ((pkt->length == E_PKT_LENGTH_4) &&
				   ((pkt->sbpkt.payload.data[0] & 0xff) == 0)) {
				rx[port].vs_info_details.dolby_vision_flag = DV_NULL;
			} else {
				if (pkt->sbpkt.vsi_3dext.vdfmt == VSI_FORMAT_3D_FORMAT) {
					rx[port].vs_info_details.vd_fmt = VSI_FORMAT_3D_FORMAT;
					rx[port].vs_info_details._3d_structure =
						pkt->sbpkt.vsi_3dext.threed_st;
					rx[port].vs_info_details._3d_ext_data =
						pkt->sbpkt.vsi_3dext.threed_ex;
					rx[port].threed_info.meta_data_flag =
						pkt->sbpkt.vsi_3dext.threed_meta_pre;
					rx[port].threed_info.meta_data_type =
						pkt->sbpkt.vsi_3dext.threed_meta_type;
					rx[port].threed_info.meta_data_length =
						pkt->sbpkt.vsi_3dext.threed_meta_length;
					memcpy(rx[port].threed_info.meta_data,
						pkt->sbpkt.vsi_3dext.threed_meta_data,
						sizeof(rx[port].threed_info.meta_data));
					if (log_level & VSI_LOG)
						rx_pr("3d:%d, 3d_ext:%d, mete_data:%d\n",
							pkt->sbpkt.vsi_3dext.threed_st,
							pkt->sbpkt.vsi_3dext.threed_ex,
							pkt->sbpkt.vsi_3dext.threed_meta_pre);
				}
				rx[port].vs_info_details.vsi_state |= E_VSI_4K3D;
				rx[port].vs_info_details.dolby_vision_flag = DV_NULL;
			}
		} else if (pkt->ieee == IEEE_VSI21) {
			/* hdmi2.1 */
			tmp = pkt->sbpkt.payload.data[0] & _BIT(9);
			rx[port].vs_info_details.vsi_state |= E_VSI_VSI21;
			rx[port].vs_info_details.hdmi_allm = tmp ? true : false;
		}
		i++;
	}
	if (rx[port].vs_info_details.emp_pkt_cnt &&
	    emp_info_p->emp_tagid == IEEE_DV15) {
		//pkt->ieee = rx[rx_info.main_port].emp_buff.emp_tagid;
		rx[port].vs_info_details.dolby_vision_flag = DV_EMP;
		rx[port].vs_info_details.vsi_state |= E_VSI_DV15;
		pkt->sbpkt.vsi_dobv15.dv_vs10_sig_type = 1;
		pkt->sbpkt.vsi_dobv15.ll =
			(emp_info_p->data_ver & 1) ? 1 : 0;
		pkt->sbpkt.vsi_dobv15.bklt_md = 0;
		pkt->sbpkt.vsi_dobv15.aux_md = 0;
		pkt->sbpkt.vsi_dobv15.content_type =
			emp_info_p->emp_content_type;
		if (pkt->sbpkt.vsi_dobv15.content_type == 2)
			rx[port].vs_info_details.dv_allm = true;
		if (log_level & PACKET_LOG)
			rx_pr("dv_emp-allm-%d\n", rx[port].vs_info_details.dv_allm);
	} else {
		if (num == rxpktsts[port].pkt_cnt_vsi)
			pkt->ieee = 0;
		num = rxpktsts[port].pkt_cnt_vsi;
	}
	emp_info_p->emp_tagid = 0;
	/* pkt->ieee = 0; */
	/* memset(&rx_pkt.vs_info, 0, sizeof(struct pd_infoframe_s)); */
	rxpktsts[port].pkt_op_flag &= ~PKT_OP_VSI;
}

void rx_pkt_buffclear(enum pkt_type_e pkt_type, u8 port)
{
	struct packet_info_s *prx = &rx_pkt[port];
	void *pktinfo;
	int i = 0;

	if (pkt_type == PKT_TYPE_INFOFRAME_VSI) {
		while (i < VSI_TYPE_MAX)
			memset(&prx->multi_vs_info[i++], 0, sizeof(struct pd_infoframe_s));
		return;
	} else if (pkt_type == PKT_TYPE_INFOFRAME_AVI) {
		pktinfo = &prx->avi_info;
	} else if (pkt_type == PKT_TYPE_INFOFRAME_SPD) {
		pktinfo = &prx->spd_info;
	} else if (pkt_type == PKT_TYPE_INFOFRAME_AUD) {
		pktinfo = &prx->aud_pktinfo;
	} else if (pkt_type == PKT_TYPE_INFOFRAME_MPEGSRC) {
		pktinfo = &prx->mpegs_info;
	} else if (pkt_type == PKT_TYPE_INFOFRAME_NVBI) {
		pktinfo = &prx->ntscvbi_info;
	} else if (pkt_type == PKT_TYPE_INFOFRAME_DRM) {
		pktinfo = &prx->drm_info;
	} else if (pkt_type == PKT_TYPE_EMP) {
		pktinfo = &prx->emp_info;
	} else if (pkt_type == PKT_TYPE_ACR) {
		pktinfo = &prx->acr_info;
	} else if (pkt_type == PKT_TYPE_GCP) {
		pktinfo = &prx->gcp_info;
	} else if (pkt_type == PKT_TYPE_ACP) {
		pktinfo = &prx->acp_info;
	} else if (pkt_type == PKT_TYPE_ISRC1) {
		pktinfo = &prx->isrc1_info;
	} else if (pkt_type == PKT_TYPE_ISRC2) {
		pktinfo = &prx->isrc2_info;
	} else if (pkt_type == PKT_TYPE_GAMUT_META) {
		pktinfo = &prx->gameta_info;
	} else if (pkt_type == PKT_TYPE_AUD_META) {
		pktinfo = &prx->amp_info;
	} else {
		rx_pr("err type:0x%x\n", pkt_type);
		return;
	}
	memset(pktinfo, 0, sizeof(struct pd_infoframe_s));
}

void rx_pkt_content_chk_en(u32 enable, u8 port)
{
	rx_pr("content chk:%d\n", enable);
	if (enable) {
		if (!pkt_testbuff)
			pkt_testbuff = kmalloc(sizeof(*pkt_testbuff),
					       GFP_KERNEL);
		if (pkt_testbuff) {
			memset(pkt_testbuff, 0,
			       sizeof(*pkt_testbuff));
			rxpktsts[port].pkt_chk_flg = true;
		} else {
			rx_pr("kmalloc fail for pkt chk\n");
		}
	} else {
		if (rxpktsts[port].pkt_chk_flg)
			kfree(pkt_testbuff);
		rxpktsts[port].pkt_chk_flg = false;
		pkt_testbuff = NULL;
	}
}

void rx_pkt_check_content(u8 port)
{
	struct packet_info_s *cur_pkt = &rx_pkt[port];
	struct st_pkt_test_buff *pre_pkt;
	struct rxpkt_st *pktsts;
	static u32 acr_pkt_cnt;
	static u32 ex_acr_pkt_cnt;
	struct pd_infoframe_s pktdata;

	pre_pkt = pkt_testbuff;
	pktsts = &rxpktsts[port];

	if (!pre_pkt)
		return;

	if (rxpktsts[port].pkt_chk_flg) {
		/*compare vsi*/
		if (pktsts->pkt_cnt_vsi) {
			if (memcmp((char *)&pre_pkt->vs_info,
				   (char *)&cur_pkt->vs_info,
				   sizeof(struct pd_infoframe_s)) != 0) {
				rx_pr(" vsi chg\n");
				/*dump pre data*/
				rx_pktdump_raw(&pre_pkt->vs_info);
				/*dump current data*/
				rx_pktdump_raw(&cur_pkt->vs_info);
				/*save current*/
				memcpy(&pre_pkt->vs_info,
				       &cur_pkt->vs_info,
				       sizeof(struct pd_infoframe_s));
			}
		}

		/*compare avi*/
		if (pktsts->pkt_cnt_avi) {
			if (memcmp((char *)&pre_pkt->avi_info,
				   (char *)&cur_pkt->avi_info,
				   sizeof(struct pd_infoframe_s)) != 0) {
				rx_pr(" avi chg\n");
				rx_pktdump_raw(&pre_pkt->avi_info);
				rx_pktdump_raw(&cur_pkt->avi_info);
				memcpy(&pre_pkt->avi_info,
				       &cur_pkt->avi_info,
				       sizeof(struct pd_infoframe_s));
			}
		}

		/*compare spd*/
		if (pktsts->pkt_cnt_spd) {
			if (memcmp((char *)&pre_pkt->spd_info,
				   (char *)&cur_pkt->spd_info,
				   sizeof(struct pd_infoframe_s)) != 0) {
				rx_pr(" spd chg\n");
				rx_pktdump_raw(&pre_pkt->spd_info);
				rx_pktdump_raw(&cur_pkt->spd_info);
				memcpy(&pre_pkt->spd_info,
				       &cur_pkt->spd_info,
				       sizeof(struct pd_infoframe_s));
			}
		}
		/*compare aud_pktinfo*/
		if (pktsts->pkt_cnt_audif) {
			if (memcmp((char *)&pre_pkt->aud_pktinfo,
				   (char *)&cur_pkt->aud_pktinfo,
				   sizeof(struct pd_infoframe_s)) != 0) {
				rx_pr(" audif chg\n");
				rx_pktdump_raw(&pre_pkt->aud_pktinfo);
				rx_pktdump_raw(&cur_pkt->aud_pktinfo);
				memcpy(&pre_pkt->aud_pktinfo,
				       &cur_pkt->aud_pktinfo,
				       sizeof(struct pd_infoframe_s));
			}
		}
		/*compare mpegs_info*/
		if (pktsts->pkt_cnt_mpeg) {
			if (memcmp((char *)&pre_pkt->mpegs_info,
				   (char *)&cur_pkt->mpegs_info,
				   sizeof(struct pd_infoframe_s)) != 0) {
				rx_pktdump_raw(&pre_pkt->mpegs_info);
				rx_pktdump_raw(&cur_pkt->mpegs_info);
				memcpy(&pre_pkt->mpegs_info,
				       &cur_pkt->mpegs_info,
				       sizeof(struct pd_infoframe_s));
			}
		}
		/*compare ntscvbi_info*/
		if (pktsts->pkt_cnt_nvbi) {
			if (memcmp((char *)&pre_pkt->ntscvbi_info,
				   (char *)&cur_pkt->ntscvbi_info,
				   sizeof(struct pd_infoframe_s)) != 0) {
				rx_pr(" nvbi chg\n");
				rx_pktdump_raw(&pre_pkt->ntscvbi_info);
				rx_pktdump_raw(&cur_pkt->ntscvbi_info);
				memcpy(&pre_pkt->ntscvbi_info,
				       &cur_pkt->ntscvbi_info,
				       sizeof(struct pd_infoframe_s));
			}
		}
		/*compare drm_info*/
		if (pktsts->pkt_cnt_drm) {
			if (memcmp((char *)&pre_pkt->drm_info,
				   (char *)&cur_pkt->drm_info,
				   sizeof(struct pd_infoframe_s)) != 0) {
				rx_pr(" drm chg\n");
				rx_pktdump_raw(&pre_pkt->drm_info);
				rx_pktdump_raw(&cur_pkt->drm_info);
				memcpy(&pre_pkt->drm_info,
				       &cur_pkt->drm_info,
				       sizeof(struct pd_infoframe_s));
			}
		}
		/*compare acr_info*/
		if (pktsts->pkt_cnt_acr) {
			if (memcmp((char *)&pre_pkt->acr_info,
				   (char *)&cur_pkt->acr_info,
				   sizeof(struct pd_infoframe_s)) != 0) {
				if (acr_pkt_cnt++ > 100) {
					acr_pkt_cnt = 0;
					rx_pr(" acr chg\n");
					rx_pktdump_acr(&cur_pkt->acr_info);
				}
				/*save current*/
				memcpy(&pre_pkt->acr_info,
				       &cur_pkt->acr_info,
				       sizeof(struct pd_infoframe_s));
			}
		}
		/*compare gcp_info*/
		if (pktsts->pkt_cnt_gcp) {
			if (memcmp((char *)&pre_pkt->gcp_info,
				   (char *)&cur_pkt->gcp_info,
				   sizeof(struct pd_infoframe_s)) != 0) {
				rx_pr(" gcp chg\n");
				/*dump pre data*/
				rx_pktdump_raw(&pre_pkt->gcp_info);
				/*dump current data*/
				rx_pktdump_raw(&cur_pkt->gcp_info);
				/*save current*/
				memcpy(&pre_pkt->gcp_info,
				       &cur_pkt->gcp_info,
				       sizeof(struct pd_infoframe_s));
			}
		}
		/*compare acp_info*/
		if (pktsts->pkt_cnt_acp) {
			if (memcmp((char *)&pre_pkt->acp_info,
				   (char *)&cur_pkt->acp_info,
				   sizeof(struct pd_infoframe_s)) != 0) {
				rx_pr(" acp chg\n");
				/*dump pre data*/
				rx_pktdump_raw(&pre_pkt->acp_info);
				/*dump current data*/
				rx_pktdump_raw(&cur_pkt->acp_info);
				/*save current*/
				memcpy(&pre_pkt->acp_info,
				       &cur_pkt->acp_info,
				       sizeof(struct pd_infoframe_s));
			}
		}
		/*compare isrc1_info*/
		if (pktsts->pkt_cnt_isrc1) {
			if (memcmp((char *)&pre_pkt->isrc1_info,
				   (char *)&cur_pkt->isrc1_info,
				   sizeof(struct pd_infoframe_s)) != 0) {
				rx_pr(" isrc2 chg\n");
				/*dump pre data*/
				rx_pktdump_raw(&pre_pkt->isrc1_info);
				/*dump current data*/
				rx_pktdump_raw(&cur_pkt->isrc1_info);
				/*save current*/
				memcpy(&pre_pkt->isrc1_info,
				       &cur_pkt->isrc1_info,
				       sizeof(struct pd_infoframe_s));
			}
		}
		/*compare isrc2_info*/
		if (pktsts->pkt_cnt_isrc2) {
			if (memcmp((char *)&pre_pkt->isrc2_info,
				   (char *)&cur_pkt->isrc2_info,
				   sizeof(struct pd_infoframe_s)) != 0) {
				rx_pr(" isrc1 chg\n");
				/*dump pre data*/
				rx_pktdump_raw(&pre_pkt->isrc2_info);
				/*dump current data*/
				rx_pktdump_raw(&cur_pkt->isrc2_info);
				/*save current*/
				memcpy(&pre_pkt->isrc2_info,
				       &cur_pkt->isrc2_info,
				       sizeof(struct pd_infoframe_s));
			}
		}
		/*compare gameta_info*/
		if (pktsts->pkt_cnt_gameta) {
			if (memcmp((char *)&pre_pkt->gameta_info,
				   (char *)&cur_pkt->gameta_info,
				   sizeof(struct pd_infoframe_s)) != 0) {
				rx_pr(" gmd chg\n");
				/*dump pre data*/
				rx_pktdump_raw(&pre_pkt->gameta_info);
				/*dump current data*/
				rx_pktdump_raw(&cur_pkt->gameta_info);
				/*save current*/
				memcpy(&pre_pkt->gameta_info,
				       &cur_pkt->gameta_info,
				       sizeof(struct pd_infoframe_s));
			}
		}
		/*compare amp_info*/
		if (pktsts->pkt_cnt_amp) {
			if (memcmp((char *)&pre_pkt->amp_info,
				   (char *)&cur_pkt->amp_info,
				   sizeof(struct pd_infoframe_s)) != 0) {
				rx_pr(" amp chg\n");
				/*dump pre data*/
				rx_pktdump_raw(&pre_pkt->amp_info);
				/*dump current data*/
				rx_pktdump_raw(&cur_pkt->amp_info);
				/*save current*/
				memcpy(&pre_pkt->amp_info,
				       &cur_pkt->amp_info,
				       sizeof(struct pd_infoframe_s));
			}
		}

		rx_pkt_get_audif_ex(&pktdata);
		if (memcmp((char *)&pre_pkt->ex_audif,
			   (char *)&pktdata,
			   sizeof(struct pd_infoframe_s)) != 0) {
			rx_pr(" ex_audif chg\n");
			memcpy(&pre_pkt->ex_audif, &pktdata,
			       sizeof(struct pd_infoframe_s));
		}

		rx_pkt_get_acr_ex(&pktdata);
		if (memcmp((char *)&pre_pkt->ex_acr,
			   (char *)&pktdata,
			   sizeof(struct pd_infoframe_s)) != 0) {
			if (ex_acr_pkt_cnt++ > 100) {
				ex_acr_pkt_cnt = 0;
				rx_pr(" ex_acr chg\n");
			}
			memcpy(&pre_pkt->ex_acr, &pktdata,
			       sizeof(struct pd_infoframe_s));
		}

		rx_pkt_get_avi_ex(&pktdata, port);
		if (memcmp((char *)&pre_pkt->ex_avi,
			   (char *)&pktdata,
			   sizeof(struct pd_infoframe_s)) != 0) {
			rx_pr(" ex_avi chg\n");
			memcpy(&pre_pkt->ex_avi, &pktdata,
			       sizeof(struct pd_infoframe_s));
		}

		rx_pkt_get_vsi_ex(&pktdata, port);
		if (memcmp((char *)&pre_pkt->ex_vsi,
			   (char *)&pktdata,
			   sizeof(struct pd_infoframe_s)) != 0) {
			rx_pr(" ex_vsi chg\n");
			memcpy(&pre_pkt->ex_vsi, &pktdata,
			       sizeof(struct pd_infoframe_s));
		}

		rx_pkt_get_amp_ex(&pktdata);
		if (memcmp((char *)&pre_pkt->ex_amp,
			   (char *)&pktdata,
			   sizeof(struct pd_infoframe_s)) != 0) {
			rx_pr(" ex_amp chg\n");
			memcpy(&pre_pkt->ex_amp, &pktdata,
			       sizeof(struct pd_infoframe_s));
		}

		rx_pkt_get_gmd_ex(&pktdata);
		if (memcmp((char *)&pre_pkt->ex_gmd,
			   (char *)&pktdata,
			   sizeof(struct pd_infoframe_s)) != 0) {
			rx_pr(" ex_gmd chg\n");
			memcpy(&pre_pkt->ex_gmd, &pktdata,
			       sizeof(struct pd_infoframe_s));
		}

		rx_pkt_get_gcp_ex(&pktdata);
		if (memcmp((char *)&pre_pkt->ex_gcp,
			   (char *)&pktdata,
			   sizeof(struct pd_infoframe_s)) != 0) {
			rx_pr(" ex_gcp chg\n");
			memcpy(&pre_pkt->ex_gcp, &pktdata,
			       sizeof(struct pd_infoframe_s));
		}

		rx_pkt_get_drm_ex(&pktdata);
		if (memcmp((char *)&pre_pkt->ex_drm,
			   (char *)&pktdata,
			   sizeof(struct pd_infoframe_s)) != 0) {
			rx_pr(" ex_drm chg\n");
			memcpy(&pre_pkt->ex_drm, &pktdata,
			       sizeof(struct pd_infoframe_s));
		}

		rx_pkt_get_ntscvbi_ex(&pktdata);
		if (memcmp((char *)&pre_pkt->ex_nvbi,
			   (char *)&pktdata,
			   sizeof(struct pd_infoframe_s)) != 0) {
			rx_pr(" ex_nvbi chg\n");
			memcpy(&pre_pkt->ex_nvbi, &pktdata,
			       sizeof(struct pd_infoframe_s));
		}
	}
}

//todo
bool is_new_visf_pkt_rcv(union infoframe_u *pktdata, u8 port)
{
	struct vsi_infoframe_st *pkt;
	enum vsi_state_e new_pkt, old_pkt;
	bool allm_sts = false;

	pkt = (struct vsi_infoframe_st *)&rx_pkt[port].vs_info;
	/* old vsif pkt */
	if (pkt->ieee == IEEE_DV15)
		old_pkt = E_VSI_DV15;
	else if (pkt->ieee == IEEE_VSI21)
		old_pkt = E_VSI_VSI21;
	else if (pkt->ieee == IEEE_HDR10PLUS)
		old_pkt = E_VSI_HDR10PLUS;
	else if ((pkt->ieee == IEEE_VSI14) &&
		 (pkt->length == E_PKT_LENGTH_24))
		old_pkt = E_VSI_DV10;
	else
		old_pkt = E_VSI_4K3D;

	//====================for dv5.0=====================
	if (old_pkt == E_VSI_VSI21) {
		if (pkt->sbpkt.payload.data[0] & _BIT(9))
			allm_sts = true;
	}
	//====================for dv5.0 end=================
	pkt = (struct vsi_infoframe_st *)(pktdata);
	/* new vsif */
	if (pkt->ieee == IEEE_DV15)
		new_pkt = E_VSI_DV15;
	else if (pkt->ieee == IEEE_VSI21)
		new_pkt = E_VSI_VSI21;
	else if (pkt->ieee == IEEE_HDR10PLUS)
		new_pkt = E_VSI_HDR10PLUS;
	else if ((pkt->ieee == IEEE_VSI14) &&
		 (pkt->length == E_PKT_LENGTH_24))
		new_pkt = E_VSI_DV10;
	else
		new_pkt = E_VSI_4K3D;
	//====================for dv5.0=====================
	if (new_pkt == E_VSI_VSI21) {
		if (pkt->sbpkt.payload.data[0] & _BIT(9))
			allm_sts = true;
	}
	if (allm_sts) {
		if (old_pkt == E_VSI_VSI21 && new_pkt == E_VSI_DV15) {
			pkt->ieee = IEEE_DV_PLUS_ALLM;
		} else if (old_pkt == E_VSI_DV15 && new_pkt == E_VSI_VSI21) {
			pkt = (struct vsi_infoframe_st *)&rx_pkt[port].vs_info;
			pkt->ieee = IEEE_DV_PLUS_ALLM;
		}
	}
	//====================for dv5.0 end=================
	if (new_pkt > old_pkt)
		return true;

	return false;
}

int rx_pkt_fifodecode(struct packet_info_s *prx,
		      union infoframe_u *pktdata,
		      struct rxpkt_st *pktsts, u8 port)
{
	switch (pktdata->raw_infoframe.pkttype) {
	case PKT_TYPE_INFOFRAME_VSI:
		if (log_level & PACKET_LOG && rx[port].new_emp_pkt)
			rx_pr("ieee_type:0x%x\n", pktdata->vsi_infoframe.ieee);
		pktsts->pkt_op_flag |= PKT_OP_VSI;
		switch (pktdata->vsi_infoframe.ieee) {
		case IEEE_DV15:
			memcpy(&prx->multi_vs_info[DV15], pktdata,
				sizeof(struct pd_infoframe_s));
			rx[port].vs_info_details.vsi_state |= E_VSI_DV15;
			break;
		case IEEE_HDR10PLUS:
			memcpy(&prx->multi_vs_info[HDR10PLUS], pktdata,
				sizeof(struct pd_infoframe_s));
			rx[port].vs_info_details.vsi_state |= E_VSI_HDR10PLUS;
			break;
		case IEEE_CUVAHDR:
			memcpy(&prx->multi_vs_info[CUVAHDR], pktdata,
				sizeof(struct pd_infoframe_s));
			rx[port].vs_info_details.vsi_state |= E_VSI_CUVAHDR;
			break;
		case IEEE_FILMMAKER:
			memcpy(&prx->multi_vs_info[FILMMAKER], pktdata,
				sizeof(struct pd_infoframe_s));
			rx[port].vs_info_details.vsi_state |= E_VSI_FILMMAKER;
			break;
		case IEEE_IMAX:
			memcpy(&prx->multi_vs_info[IMAX], pktdata,
				sizeof(struct pd_infoframe_s));
			rx[port].vs_info_details.vsi_state |= E_VSI_IMAX;
			break;
		case IEEE_VSI21:
			memcpy(&prx->multi_vs_info[VSI21], pktdata,
				sizeof(struct pd_infoframe_s));
			rx[port].vs_info_details.vsi_state |= E_VSI_VSI21;
			break;
		case IEEE_VSI14:
			memcpy(&prx->multi_vs_info[VSI14], pktdata,
				sizeof(struct pd_infoframe_s));
			rx[port].vs_info_details.vsi_state |= E_VSI_4K3D;
			break;
		default:
			break;
		}
		break;
	case PKT_TYPE_INFOFRAME_AVI:
		pktsts->pkt_cnt_avi++;
		pktsts->pkt_op_flag |= PKT_OP_AVI;
		/*memset(&prx->avi_info, 0, sizeof(struct pd_infoframe_s));*/
		memcpy(&prx->avi_info, pktdata, sizeof(struct pd_infoframe_s));
		pktsts->pkt_op_flag &= ~PKT_OP_AVI;
		break;
	case PKT_TYPE_INFOFRAME_SPD:
		pktsts->pkt_cnt_spd++;
		pktsts->pkt_op_flag |= PKT_OP_SPD;
		/*memset(&prx->spd_info, 0, sizeof(struct pd_infoframe_s));*/
		memcpy(&prx->spd_info, pktdata,
		       sizeof(struct spd_infoframe_st));
		pktsts->pkt_op_flag &= ~PKT_OP_SPD;
		break;
	case PKT_TYPE_INFOFRAME_AUD:
		pktsts->pkt_cnt_audif++;
		pktsts->pkt_op_flag |= PKT_OP_AIF;
		/*memset(&prx->aud_pktinfo, 0, sizeof(struct pd_infoframe_s));*/
		memcpy(&prx->aud_pktinfo,
		       pktdata, sizeof(struct pd_infoframe_s));
		pktsts->pkt_op_flag &= ~PKT_OP_AIF;
		break;
	case PKT_TYPE_INFOFRAME_MPEGSRC:
		pktsts->pkt_cnt_mpeg++;
		pktsts->pkt_op_flag |= PKT_OP_MPEGS;
		/*memset(&prx->mpegs_info, 0, sizeof(struct pd_infoframe_s));*/
		memcpy(&prx->mpegs_info, pktdata,
		       sizeof(struct pd_infoframe_s));
		pktsts->pkt_op_flag &= ~PKT_OP_MPEGS;
		break;
	case PKT_TYPE_INFOFRAME_NVBI:
		pktsts->pkt_cnt_nvbi++;
		pktsts->pkt_op_flag |= PKT_OP_NVBI;
		/* memset(&prx->ntscvbi_info, 0, */
			/* sizeof(struct pd_infoframe_s)); */
		memcpy(&prx->ntscvbi_info, pktdata,
		       sizeof(struct pd_infoframe_s));
		pktsts->pkt_op_flag &= ~PKT_OP_NVBI;
		break;
	case PKT_TYPE_INFOFRAME_DRM:
		pktsts->pkt_attach_drm++;
		pktsts->pkt_cnt_drm++;
		pktsts->pkt_op_flag |= PKT_OP_DRM;
		/*memset(&prx->drm_info, 0, sizeof(struct pd_infoframe_s));*/
		memcpy(&prx->drm_info, pktdata, sizeof(struct pd_infoframe_s));
		pktsts->pkt_op_flag &= ~PKT_OP_DRM;
		break;
	case PKT_TYPE_ACR:
		pktsts->pkt_cnt_acr++;
		pktsts->pkt_op_flag |= PKT_OP_ACR;
		/*memset(&prx->acr_info, 0, sizeof(struct pd_infoframe_s));*/
		memcpy(&prx->acr_info,
		       pktdata, sizeof(struct pd_infoframe_s));
		pktsts->pkt_op_flag &= ~PKT_OP_ACR;
		break;
	case PKT_TYPE_GCP:
		pktsts->pkt_cnt_gcp++;
		pktsts->pkt_op_flag |= PKT_OP_GCP;
		/*memset(&prx->gcp_info, 0, sizeof(struct pd_infoframe_s));*/
		memcpy(&prx->gcp_info,
		       pktdata, sizeof(struct pd_infoframe_s));
		pktsts->pkt_op_flag &= ~PKT_OP_GCP;
		break;
	case PKT_TYPE_ACP:
		pktsts->pkt_cnt_acp++;
		pktsts->pkt_op_flag |= PKT_OP_ACP;
		/*memset(&prx->acp_info, 0, sizeof(struct pd_infoframe_s));*/
		memcpy(&prx->acp_info,
		       pktdata, sizeof(struct pd_infoframe_s));
		pktsts->pkt_op_flag &= ~PKT_OP_ACP;
		break;
	case PKT_TYPE_ISRC1:
		pktsts->pkt_cnt_isrc1++;
		pktsts->pkt_op_flag |= PKT_OP_ISRC1;
		/*memset(&prx->isrc1_info, 0, sizeof(struct pd_infoframe_s));*/
		memcpy(&prx->isrc1_info,
		       pktdata, sizeof(struct pd_infoframe_s));
		pktsts->pkt_op_flag &= ~PKT_OP_ISRC1;
		break;
	case PKT_TYPE_ISRC2:
		pktsts->pkt_cnt_isrc2++;
		pktsts->pkt_op_flag |= PKT_OP_ISRC2;
		/*memset(&prx->isrc2_info, 0, sizeof(struct pd_infoframe_s));*/
		memcpy(&prx->isrc2_info,
		       pktdata, sizeof(struct pd_infoframe_s));
		pktsts->pkt_op_flag &= ~PKT_OP_ISRC2;
		break;
	case PKT_TYPE_GAMUT_META:
		pktsts->pkt_cnt_gameta++;
		pktsts->pkt_op_flag |= PKT_OP_GMD;
		/*memset(&prx->gameta_info, 0, sizeof(struct pd_infoframe_s));*/
		memcpy(&prx->gameta_info,
		       pktdata, sizeof(struct pd_infoframe_s));
		pktsts->pkt_op_flag &= ~PKT_OP_GMD;
		break;
	case PKT_TYPE_AUD_META:
		pktsts->pkt_cnt_amp++;
		pktsts->pkt_op_flag |= PKT_OP_AMP;
		/*memset(&prx->amp_info, 0, sizeof(struct pd_infoframe_s));*/
		memcpy(&prx->amp_info,
		       pktdata, sizeof(struct pd_infoframe_s));
		pktsts->pkt_op_flag &= ~PKT_OP_AMP;
		break;
	case PKT_TYPE_EMP:
		pktsts->pkt_cnt_emp++;
		pktsts->pkt_op_flag |= PKT_OP_EMP;
		memcpy(&prx->emp_info,
		       pktdata, sizeof(struct pd_infoframe_s));
		pktsts->pkt_op_flag &= ~PKT_OP_EMP;
		break;

	default:
		break;
	}
	return 0;
}

static void rx_parse_dsf(unsigned char *src_addr, u8 port)
{
	bool first, last;
	u8 sequence_index;
	u8 i;

	first = *(src_addr + 1) >> 7;
	last = (*(src_addr + 1) >> 6) & 0x1;
	sequence_index = *(src_addr + 2);
	if (log_level & PACKET_LOG && rx[port].new_emp_pkt)
		rx_pr("dst_addr:0x%x, dst_addr+1:0x%x, first:%d, last:%d\n",
			*src_addr, *(src_addr + 1), first, last);
	if (first)
		rx[port].emp_dsf_info[rx[port].emp_dsf_cnt].pkt_addr =
		src_addr;
	if (last) {
		rx[port].emp_dsf_info[rx[port].emp_dsf_cnt].pkt_cnt =
			sequence_index + 1;
		rx[port].emp_dsf_cnt++;
	}
	if (rx[port].emp_dsf_cnt >= EMP_DSF_CNT_MAX) {
		rx[port].emp_dsf_cnt = 0;
		for (i = 0; i < EMP_DSF_CNT_MAX; i++)
			memset(&rx[port].emp_dsf_info[i], 0, sizeof(struct emp_dsf_st));
	}
}

struct emp_info_s *rx_get_emp_info(u8 port)
{
	if (rx[port].emp_vid_idx == 0)
		return &rx_info.emp_buff_a;
	else if (rx[port].emp_vid_idx == 1)
		return &rx_info.emp_buff_b;
	else
		return NULL;
}

bool is_emp_buf_change(u8 port)
{
	struct emp_info_s *emp_info_p = rx_get_emp_info(port);

	if (!emp_info_p) {
		rx_pr("%s emp info NULL\n", __func__);
		return false;
	}
	if (rx_emp_dbg_en)
		return true;
	if (emp_info_p->pre_emp_pkt_cnt != emp_info_p->emp_pkt_cnt)
		return true;
	else if (memcmp((u8 *)emp_buf[rx[port].emp_vid_idx],
		(u8 *)pre_emp_buf[rx[port].emp_vid_idx],
		emp_info_p->emp_pkt_cnt * 32) != 0)
		return true;
	else
		return false;
}

int rx_pkt_handler(enum pkt_decode_type pkt_int_src, u8 port)
{
	//u32 i = 0;
	u32 j = 0;
	u32 pkt_num = 0;
	bool find_emp_header = false;
	u32 *pkt_cnt = NULL;
	u8 try_cnt = 3;
	union infoframe_u *pktdata;
	struct packet_info_s *prx = &rx_pkt[port];
	struct emp_info_s *emp_info_p = NULL;
	u8 vid_idx;
	u32 *fifo_buf_p = NULL;

	/*u32 t1, t2;*/
	rxpktsts[port].dv_pkt_num = 0;
	pkt_cnt = &rxpktsts[port].fifo_pkt_num;
	*pkt_cnt = 0;
	if (rx[port].emp_vid_idx == 0)
		fifo_buf_p = pd_fifo_buf_a;
	else if (rx[port].emp_vid_idx == 1)
		fifo_buf_p = pd_fifo_buf_b;
	if (pkt_int_src == PKT_BUFF_SET_FIFO) {
		/*from pkt fifo*/
		if (!fifo_buf_p)
			return 0;
		memset(fifo_buf_p, 0, PFIFO_SIZE * sizeof(u32));
		memset(&prx->vs_info, 0, sizeof(struct pd_infoframe_s));
		/*t1 = sched_clock();*/
		/*for recode interrupt cnt*/
		/* rxpktsts.fifo_int_cnt++; */
		pkt_num = hdmirx_rd_dwc(DWC_PDEC_FIFO_STS1);
		//if (log_level & 0x8000)
			//rx_pr("pkt=%d\n", pkt_num);
		while (pkt_num >= K_ONEPKT_BUFF_SIZE) {
			(*pkt_cnt)++;
			/*read one pkt from fifo*/
			for (j = 0; j < K_ONEPKT_BUFF_SIZE; j++) {
				/*8 word per packet size*/
				fifo_buf_p[j] =
					hdmirx_rd_dwc(DWC_PDEC_FIFO_DATA);
			}
			if (log_level & PACKET_LOG)
				rx_pr("PD[%d]=%x\n",  *pkt_cnt, fifo_buf_p[0]);
			pktdata = (union infoframe_u *)fifo_buf_p;
			rx_pkt_fifodecode(prx, pktdata, &rxpktsts[port], port);
			rx[port].irq_flag &= ~IRQ_PACKET_FLAG;
			if (try_cnt != 0)
				try_cnt--;
			if (try_cnt == 0)
				return 0;

			pkt_num = hdmirx_rd_dwc(DWC_PDEC_FIFO_STS1);
		}
	} else if (pkt_int_src == PKT_BUFF_SET_VSI) {
		rxpktsts[port].pkt_attach_vsi++;
		rxpktsts[port].pkt_op_flag |= PKT_OP_VSI;
		rx_pkt_get_vsi_ex(&prx->vs_info, port);
		rxpktsts[port].pkt_op_flag &= ~PKT_OP_VSI;
		rxpktsts[port].pkt_cnt_vsi_ex++;
		rxpktsts[port].pkt_cnt_vsi++;
		if (rxpktsts[port].pkt_cnt_vsi == 0xffffffff)
			rxpktsts[port].pkt_cnt_vsi = 0;
	} else if (pkt_int_src == PKT_BUFF_SET_DRM) {
		rxpktsts[port].pkt_attach_drm++;
		rxpktsts[port].pkt_op_flag |= PKT_OP_DRM;
		rx_pkt_get_drm_ex(&prx->drm_info);
		rxpktsts[port].pkt_op_flag &= ~PKT_OP_DRM;
		rxpktsts[port].pkt_cnt_drm_ex++;
	} else if (pkt_int_src == PKT_BUFF_SET_GMD) {
		rxpktsts[port].pkt_op_flag |= PKT_OP_GMD;
		rx_pkt_get_gmd_ex(&prx->gameta_info);
		rxpktsts[port].pkt_op_flag &= ~PKT_OP_GMD;
		rxpktsts[port].pkt_cnt_gmd_ex++;
	} else if (pkt_int_src == PKT_BUFF_SET_AIF) {
		rxpktsts[port].pkt_op_flag |= PKT_OP_AIF;
		rx_pkt_get_audif_ex(&prx->aud_pktinfo);
		rxpktsts[port].pkt_op_flag &= ~PKT_OP_AIF;
		rxpktsts[port].pkt_cnt_aif_ex++;
	} else if (pkt_int_src == PKT_BUFF_SET_AVI) {
		rxpktsts[port].pkt_op_flag |= PKT_OP_AVI;
		rx_pkt_get_avi_ex(&prx->avi_info, port);
		rxpktsts[port].pkt_op_flag &= ~PKT_OP_AVI;
		rxpktsts[port].pkt_cnt_avi_ex++;
	} else if (pkt_int_src == PKT_BUFF_SET_ACR) {
		rxpktsts[port].pkt_op_flag |= PKT_OP_ACR;
		rx_pkt_get_acr_ex(&prx->acr_info);
		rxpktsts[port].pkt_op_flag &= ~PKT_OP_ACR;
		rxpktsts[port].pkt_cnt_acr_ex++;
	} else if (pkt_int_src == PKT_BUFF_SET_GCP) {
		rxpktsts[port].pkt_op_flag |= PKT_OP_GCP;
		rx_pkt_get_gcp_ex(&prx->gcp_info);
		rxpktsts[port].pkt_op_flag &= ~PKT_OP_GCP;
		rxpktsts[port].pkt_cnt_gcp_ex++;
	} else if (pkt_int_src == PKT_BUFF_SET_AMP) {
		rxpktsts[port].pkt_op_flag |= PKT_OP_AMP;
		rx_pkt_get_amp_ex(&prx->amp_info);
		rxpktsts[port].pkt_op_flag &= ~PKT_OP_AMP;
		rxpktsts[port].pkt_cnt_amp_ex++;
	} else if (pkt_int_src == PKT_BUFF_SET_NVBI) {
		rxpktsts[port].pkt_op_flag |= PKT_OP_NVBI;
		rx_pkt_get_ntscvbi_ex(&prx->ntscvbi_info);
		rxpktsts[port].pkt_op_flag &= ~PKT_OP_NVBI;
		rxpktsts[port].pkt_cnt_nvbi_ex++;
	} else if (pkt_int_src == PKT_BUFF_SET_EMP) {
		if (rx[port].emp_vid_idx == 0) {
			emp_info_p = &rx_info.emp_buff_a;
			vid_idx = 0;
		} else if (rx[port].emp_vid_idx == 1) {
			emp_info_p = &rx_info.emp_buff_b;
			vid_idx = 1;
		}
		/*from pkt fifo*/
		if (!fifo_buf_p || !emp_info_p)
			return 0;
		if (is_emp_buf_change(port)) {
			rx[port].new_emp_pkt = true;
			memcpy(pre_emp_buf[vid_idx], emp_buf[vid_idx],
				emp_info_p->emp_pkt_cnt * 32);
			emp_info_p->pre_emp_pkt_cnt = emp_info_p->emp_pkt_cnt;
		} else {
			rx[port].new_emp_pkt = false;
		}
		memset(fifo_buf_p, 0, PFIFO_SIZE * sizeof(u32));
		memset(&prx->emp_info, 0, sizeof(struct pd_infoframe_s));
		pkt_num = emp_info_p->emp_pkt_cnt;
		if (log_level & PACKET_LOG && rx[port].new_emp_pkt)
			rx_pr("vid_idx:0x%x, emp pkt=%d\n", vid_idx, pkt_num);
		rx[port].vs_info_details.vsi_state = 0x0;
		rx_pkt_buffclear(PKT_TYPE_INFOFRAME_VSI, port);
		while (pkt_num) {
			pkt_num--;
			/*read one pkt from fifo*/
			if (rx_info.chip_id >= CHIP_ID_T7) {
				memcpy((char *)fifo_buf_p + (*pkt_cnt) * 32,
					emp_buf[vid_idx] + (*pkt_cnt) * 31, 3);
				memcpy((char *)fifo_buf_p + 4 + (*pkt_cnt) * 32,
					emp_buf[vid_idx] + (*pkt_cnt) * 31 + 3,
					28);
			} else {
				memcpy((char *)fifo_buf_p + (*pkt_cnt) * 32,
					emp_buf[vid_idx] + (*pkt_cnt) * 32, 3);
				memcpy((char *)fifo_buf_p + 4 + (*pkt_cnt) * 32,
					emp_buf[vid_idx] + (*pkt_cnt) * 32 + 3,
					28);
			}
			if (log_level & PACKET_LOG && rx[port].new_emp_pkt)
				rx_pr("PD[0/%d]=0x%x, PD[1/%d]=0x%x\n",
					pkt_num, fifo_buf_p[(*pkt_cnt) * 8],
					pkt_num, fifo_buf_p[1 + (*pkt_cnt) * 8]);
			pktdata = (union infoframe_u *)&fifo_buf_p[(*pkt_cnt) * 8];
			rx_pkt_fifodecode(prx, pktdata, &rxpktsts[port], port);
			if ((fifo_buf_p[(*pkt_cnt) * 8] & 0xff) == PKT_TYPE_EMP)
				rx_parse_dsf((u8 *)&fifo_buf_p[(*pkt_cnt) * 8], port);
			if ((fifo_buf_p[(*pkt_cnt) * 8] & 0xff) == PKT_TYPE_EMP &&
				!find_emp_header) {
				find_emp_header = true;
				emp_info_p->ogi_id =
					(fifo_buf_p[1 + (*pkt_cnt) * 8] >> 16) & 0xff;
				j = (((fifo_buf_p[2 + (*pkt_cnt) * 8] >> 24) & 0xff) +
					((fifo_buf_p[3 + (*pkt_cnt) * 8] & 0xff) << 8) +
				(((fifo_buf_p[3 + (*pkt_cnt) * 8] >> 8) & 0xff) << 16));
				emp_info_p->emp_tagid = j;
				emp_info_p->data_ver =
					(fifo_buf_p[3 + (*pkt_cnt) * 8] >> 16) & 0xff;
				emp_info_p->emp_content_type =
					(fifo_buf_p[5 + (*pkt_cnt) * 8] >> 24) & 0x0f;
			}
			rx[port].irq_flag &= ~IRQ_PACKET_FLAG;
			(*pkt_cnt)++;
		}
	} else if (pkt_int_src == PKT_BUFF_SET_SPD) {
		rxpktsts[port].pkt_op_flag |= PKT_OP_SPD;
		rx_pkt_get_spd_ex(&prx->spd_info, port);
		rxpktsts[port].pkt_op_flag &= ~PKT_OP_SPD;
		rxpktsts[port].pkt_cnt_spd++;
		rxpktsts[port].pkt_spd_updated = 1;
	}

	/*t2 = sched_clock();*/
	/*
	 * timerbuff[timerbuff_idx] = pkt_num;
	 * if (timerbuff_idx++ >= 50)
	 *	timerbuff_idx = 0;
	 */
	return 0;
}

int rx_check_emp_type(struct emp_pkt_st *pkt)
{
	u32 u_ieee;
	int emp_type = -1;
	u8 new, end, ds_type, vfr, afr, sync, temp;
	u8 *src = (u8 *)pkt;

	if (!pkt)
		return emp_type;
	u_ieee = pkt->cnt.md[0] + (pkt->cnt.md[1] << 8) + (pkt->cnt.md[2] << 16);
	temp = src[4];
	new = (temp & _BIT(7)) >> 7;
	end = (temp & _BIT(6)) >> 6;
	ds_type = (temp & MSK(2, 4)) >> 4;
	afr = (temp & _BIT(3)) >> 3;
	vfr = (temp & _BIT(2)) >> 2;
	sync = (temp & _BIT(1)) >> 1;
	if (log_level == 0x121) {
		rx_pr("---emp dsf params---\n");
		rx_pr("pkttype = 0x%x", pkt->pkttype);
		rx_pr("ds_type=0x%x, sync=0x%x, vfr=0x%x, afr=0x%x\n",
			ds_type, sync, vfr, afr);
		rx_pr("org_id = 0x%x", pkt->cnt.organization_id);
		rx_pr("data_tag = 0x%x", pkt->cnt.data_set_tag_lo);
		rx_pr("length = 0x%x", pkt->cnt.data_set_length_lo);
		if (!pkt->cnt.organization_id)
			rx_pr("ieee = 0x%x", u_ieee);
		rx_pr("md[0] = 0x%x", pkt->cnt.md[0]);
		rx_pr("md[1] = 0x%x", pkt->cnt.md[1]);
		rx_pr("md[2] = 0x%x", pkt->cnt.md[2]);
		rx_pr("md[3] = 0x%x", pkt->cnt.md[3]);
	}

	if (pkt->cnt.organization_id == 0) {
		if (pkt->cnt.data_set_tag_lo == 2 &&
			u_ieee == IEEE_CUVAHDR) //cuva
			emp_type = EMP_CUVA;
		else if (u_ieee == IEEE_DV15) //dv
			emp_type = EMP_AMDV;
	} else if (ds_type == 0 &&
		vfr == 1 &&
		afr == 0 &&
		pkt->cnt.organization_id == 1 &&
		pkt->cnt.data_set_tag_lo == 1) {
		emp_type = sync ? EMP_VTEM_CLASS1 : EMP_VTEM_CLASS0;//vtem
	} else if (pkt->cnt.organization_id == 1 &&
		pkt->cnt.data_set_tag_lo == 3 &&
		ds_type == 1 &&
		sync == 1 &&
		vfr == 1 &&
		afr == 0) {
		emp_type = EMP_SBTM;//sbtm
	} else if (pkt->cnt.organization_id == 1 &&
		pkt->cnt.data_set_tag_lo == 2 &&
		ds_type == 0 &&
		sync == 1 &&
		vfr == 1 &&
		afr == 0 &&
		pkt->cnt.data_set_length_lo == 136) {
		emp_type = EMP_CVTEM;//cvtem
	}
	if (log_level == 0x121)
		rx_pr("\nemp_type = 0x%x\n", emp_type);

	return emp_type;
}

void dump_cvtem_packet(u8 port)
{
	int i, j;
	unsigned char buff[1024] = {0};
	int k = 0;

	for (i = 0; i < 6; i++) {
		for (j = 0; j < 32; j++)
			k += sprintf(buff + k, "0x%x ", rx[port].cvtem_info.dsc_info[i * 32 + j]);
		pr_info("%s", buff);
		k = 0;
	}
}

void parse_dsc_pps_data(u8 *buff, u8 port)
{
	int i;

	rx[port].dsc_pps_data.dsc_version_major = (buff[0] >> 4) & 0xf;
	rx[port].dsc_pps_data.dsc_version_minor = buff[0] & 0xf;
	rx[port].dsc_pps_data.pps_identifier = buff[1];
	rx[port].dsc_pps_data.bits_per_component = (buff[3] >> 4) & 0xf;
	rx[port].dsc_pps_data.line_buf_depth = buff[3] & 0xf;
	rx[port].dsc_pps_data.block_pred_enable = (buff[4] & 0x20) >> 5;
	rx[port].dsc_pps_data.convert_rgb = (buff[4] & 0x10) >> 4;
	rx[port].dsc_pps_data.simple_422 = buff[4] & 0x8;
	rx[port].dsc_pps_data.vbr_enable = buff[4] & 0x4;
	rx[port].dsc_pps_data.bits_per_pixel = ((buff[4] & 0x3) << 8) | buff[5];
	rx[port].dsc_pps_data.pic_height = (buff[6] << 8) | buff[7];
	rx[port].dsc_pps_data.pic_width = (buff[8] << 8) | buff[9];
	rx[port].dsc_pps_data.slice_height = (buff[10] << 8) | buff[11];
	rx[port].dsc_pps_data.slice_width = (buff[12] << 8) | buff[13];
	rx[port].dsc_pps_data.chunk_size = (buff[14] << 8) | buff[15];
	rx[port].dsc_pps_data.initial_xmit_delay = ((buff[16] & 0x3) << 8) | buff[17];
	rx[port].dsc_pps_data.initial_dec_delay = (buff[18] << 8) | buff[19];
	rx[port].dsc_pps_data.initial_scale_value = buff[21] & 0x3f;
	rx[port].dsc_pps_data.scale_increment_interval = (buff[22] << 8) | buff[23];
	rx[port].dsc_pps_data.scale_decrement_interval = ((buff[24] & 0xf) << 8) | buff[25];
	rx[port].dsc_pps_data.first_line_bpg_offset = buff[27] & 0x1f;
	rx[port].dsc_pps_data.nfl_bpg_offset = (buff[28] << 8) | buff[29];
	rx[port].dsc_pps_data.slice_bpg_offset = (buff[30] << 8) | buff[31];
	rx[port].dsc_pps_data.initial_offset = (buff[32] << 8) | buff[33];
	rx[port].dsc_pps_data.final_offset = (buff[34] << 8) | buff[35];
	rx[port].dsc_pps_data.flatness_min_qp = buff[36] & 0x1f;
	rx[port].dsc_pps_data.flatness_max_qp = buff[37] & 0x1f;
	rx[port].dsc_pps_data.rc_parameter_set.rc_model_size = (buff[38] << 8) | buff[39];
	rx[port].dsc_pps_data.rc_parameter_set.rc_edge_factor = buff[40] & 0xf;
	rx[port].dsc_pps_data.rc_parameter_set.rc_quant_incr_limit0 = buff[41] & 0x1f;
	rx[port].dsc_pps_data.rc_parameter_set.rc_quant_incr_limit1 = buff[42] & 0x1f;
	rx[port].dsc_pps_data.rc_parameter_set.rc_tgt_offset_hi = (buff[43] >> 4) & 0xf;
	rx[port].dsc_pps_data.rc_parameter_set.rc_tgt_offset_lo = buff[43] & 0xf;
	memcpy(rx[port].dsc_pps_data.rc_parameter_set.rc_buf_thresh, buff + 44, 14);
	for (i = 0; i < 15; i++) {
		rx[port].dsc_pps_data.rc_parameter_set.rc_range_parameters[i].range_min_qp =
			(buff[58 + 2 * i] & 0xf8) >> 3;
		rx[port].dsc_pps_data.rc_parameter_set.rc_range_parameters[i].range_max_qp =
			(buff[59 + 2 * i] >> 6) | (((buff[58 + 2 * i] & 0x7)) << 2);
		rx[port].dsc_pps_data.rc_parameter_set.rc_range_parameters[i].range_bpg_offset =
			buff[59 + 2 * i] & 0x3f;
	}
	rx[port].dsc_pps_data.native_420 = (buff[88] & 0x2) >> 1;
	rx[port].dsc_pps_data.native_422 = buff[88] & 0x1;
	rx[port].dsc_pps_data.second_line_bpg_offset = buff[89] & 0x1f;
	rx[port].dsc_pps_data.nsl_bpg_offset = (buff[90] << 8) | buff[91];
	rx[port].dsc_pps_data.second_line_offset_adj = (buff[92] << 8) | buff[93];
}

void dump_dsc_pps_info(u8 port)
{
	int i;
	char buf[512] = {0};
	int k = 0;

	rx_pr("------Picture Parameter Set Start------\n");
	pr_info("dsc_version_major:%d\n", rx[port].dsc_pps_data.dsc_version_major);
	pr_info("dsc_version_minor:%d\n", rx[port].dsc_pps_data.dsc_version_minor);
	rx_pr("pps_identifier:%d\n", rx[port].dsc_pps_data.pps_identifier);
	rx_pr("bits_per_component:%d\n", rx[port].dsc_pps_data.bits_per_component);
	rx_pr("line_buf_depth:%d\n", rx[port].dsc_pps_data.line_buf_depth);
	rx_pr("block_pred_enable:%d\n", rx[port].dsc_pps_data.block_pred_enable);
	rx_pr("convert_rgb:%d\n",  rx[port].dsc_pps_data.convert_rgb);
	rx_pr("simple_422:%d\n", rx[port].dsc_pps_data.simple_422);
	rx_pr("vbr_enable:%d\n", rx[port].dsc_pps_data.vbr_enable);
	rx_pr("bits_per_pixel:%d\n", rx[port].dsc_pps_data.bits_per_pixel);
	rx_pr("pic_height:%d\n", rx[port].dsc_pps_data.pic_height);
	rx_pr("pic_width:%d\n", rx[port].dsc_pps_data.pic_width);
	rx_pr("slice_height:%d\n", rx[port].dsc_pps_data.slice_height);
	rx_pr("slice_width:%d\n", rx[port].dsc_pps_data.slice_width);
	rx_pr("chunk_size:%d\n", rx[port].dsc_pps_data.chunk_size);
	rx_pr("initial_xmit_delay:%d\n", rx[port].dsc_pps_data.initial_xmit_delay);
	rx_pr("initial_dec_delay:%d\n", rx[port].dsc_pps_data.initial_dec_delay);
	rx_pr("initial_scale_value:%d\n", rx[port].dsc_pps_data.initial_scale_value);
	rx_pr("scale_increment_interval:%d\n", rx[port].dsc_pps_data.scale_increment_interval);
	rx_pr("scale_decrement_interval:%d\n", rx[port].dsc_pps_data.scale_decrement_interval);
	rx_pr("first_line_bpg_offset:%d\n", rx[port].dsc_pps_data.first_line_bpg_offset);
	rx_pr("nfl_bpg_offset:%d\n", rx[port].dsc_pps_data.nfl_bpg_offset);
	rx_pr("slice_bpg_offset:%d\n", rx[port].dsc_pps_data.slice_bpg_offset);
	rx_pr("initial_offset:%d\n", rx[port].dsc_pps_data.initial_offset);
	rx_pr("final_offset:%d\n", rx[port].dsc_pps_data.final_offset);
	rx_pr("flatness_min_qp:%d\n", rx[port].dsc_pps_data.flatness_min_qp);
	rx_pr("flatness_max_qp:%d\n", rx[port].dsc_pps_data.flatness_max_qp);
	rx_pr("rc_model_size:%d\n", rx[port].dsc_pps_data.rc_parameter_set.rc_model_size);
	rx_pr("rc_edge_factor:%d\n", rx[port].dsc_pps_data.rc_parameter_set.rc_edge_factor);
	rx_pr("rc_quant_incr_limit0:%d\n",
		rx[port].dsc_pps_data.rc_parameter_set.rc_quant_incr_limit0);
	rx_pr("rc_quant_incr_limit1:%d\n",
		rx[port].dsc_pps_data.rc_parameter_set.rc_quant_incr_limit1);
	rx_pr("rc_tgt_offset_hi:%d\n", rx[port].dsc_pps_data.rc_parameter_set.rc_tgt_offset_hi);
	rx_pr("rc_tgt_offset_lo:%d\n", rx[port].dsc_pps_data.rc_parameter_set.rc_tgt_offset_lo);
	rx_pr("*******rc_buf_thresh*******\n");
	for (i = 0; i < 14; i++)
		k += sprintf(buf + k, "%d ",
		rx[port].dsc_pps_data.rc_parameter_set.rc_buf_thresh[i]);
	pr_info("%s", buf);
	k = 0;
	rx_pr("*******rc_range_parameters*******\n");
	rx_pr("*******range_min_qp*******\n");
	for (i = 0; i < 15; i++)
		k += sprintf(buf + k, "%d ",
		rx[port].dsc_pps_data.rc_parameter_set.rc_range_parameters[i].range_min_qp);
	pr_info("%s", buf);
	k = 0;
	rx_pr("*******range_max_qp*******\n");
	for (i = 0; i < 15; i++)
		k += sprintf(buf + k, "%d ",
		rx[port].dsc_pps_data.rc_parameter_set.rc_range_parameters[i].range_max_qp);
	pr_info("%s", buf);
	k = 0;
	rx_pr("*******range_bpg_offset*******\n");
	for (i = 0; i < 15; i++)
		k += sprintf(buf + k, "%d ",
		rx[port].dsc_pps_data.rc_parameter_set.rc_range_parameters[i].range_bpg_offset);
	pr_info("%s", buf);
	k = 0;
	rx_pr("native_420:%d\n", rx[port].dsc_pps_data.native_420);
	rx_pr("native_422:%d\n", rx[port].dsc_pps_data.native_422);
	rx_pr("second_line_bpg_offset:%d\n", rx[port].dsc_pps_data.second_line_bpg_offset);
	rx_pr("nsl_bpg_offset:%d\n", rx[port].dsc_pps_data.nsl_bpg_offset);
	rx_pr("second_line_offset_adj:%d\n", rx[port].dsc_pps_data.second_line_offset_adj);
	rx_pr("------Picture Parameter Set End------\n");
}

void rx_get_em_info(u8 port)
{
	u8 i, tmp;
	int emp_type = -1;
	struct emp_pkt_st *pkt;
	static int qms_en = -1;
	static int m_const = -1;

	rx[port].sbtm_info.flag = false;
	rx[port].emp_dv_info.flag = false;
	rx[port].emp_cuva_info.flag = false;
	memset(&rx[port].vtem_info, 0, sizeof(rx[port].vtem_info));
	if (rx_info.chip_id < CHIP_ID_T7 || !rx[port].emp_pkt_rev) {
		if (log_level == 0x121)
			rx_pr("rx[%d].emp_pkt_rev = %d\n",
			port, rx[port].emp_pkt_rev);
		return;
	}

	if (log_level == 0x121 && rx[port].emp_dsf_cnt)
		rx_pr("emp_dsf_cnt:0x%x\n", rx[port].emp_dsf_cnt);
	for (i = 0; i < rx[port].emp_dsf_cnt; i++) {
		pkt = (struct emp_pkt_st *)rx[port].emp_dsf_info[i].pkt_addr;
		emp_type = rx_check_emp_type(pkt);

		switch (emp_type) {
		case EMP_VTEM_CLASS0:
			/* spec2.1a table 10-36 gaming-vrr & FVA*/
			tmp = pkt->cnt.md[0];
			rx[port].vtem_info.vrr_en = tmp & 1;
			rx[port].vtem_info.fva_factor_m1 = (tmp >> 4) & 0xf;
			tmp = pkt->cnt.md[1];
			rx[port].vtem_info.base_vfront = tmp;
			rx[port].vtem_info.base_framerate = (pkt->cnt.md[2] & 0x3) |
				(pkt->cnt.md[3] << 8);
			pkt->pkttype = 0;
			break;
		case EMP_VTEM_CLASS1:
			/* spec2.1a table 10-37 */
			tmp = pkt->cnt.md[0];
			rx[port].vtem_info.m_const = (tmp >> 1) & 1;
			rx[port].vtem_info.qms_en = (tmp >> 2) & 1;
			/* TODO: gaming-vrr/qms-vrr */
			//rx[port].vtem_info.vrr_en = rx[port].vtem_info.qms_en;
			tmp = pkt->cnt.md[1];
			rx[port].vtem_info.base_vfront = tmp;
			tmp = pkt->cnt.md[2];
			rx[port].vtem_info.next_tfr = next_tfr[tmp >> 3];
			rx[port].vtem_info.base_framerate = ((tmp & 0x3) << 8) | pkt->cnt.md[3];
			if (qms_en != rx[port].vtem_info.qms_en ||
				m_const != rx[port].vtem_info.m_const) {
				rx_pr("qms_en:%d, m_const:%d, next_tfr:%d\n",
					rx[port].vtem_info.qms_en, rx[port].vtem_info.m_const,
					rx[port].vtem_info.next_tfr);
				qms_en = rx[port].vtem_info.qms_en;
				m_const = rx[port].vtem_info.m_const;
			}
			pkt->pkttype = 0;
			break;
		case EMP_SBTM:
			tmp = pkt->cnt.md[0];
			rx[port].sbtm_info.sbtm_ver = tmp & 0x0f;
			tmp = pkt->cnt.md[1];
			rx[port].sbtm_info.sbtm_mode = tmp & 3;
			rx[port].sbtm_info.sbtm_type = (tmp >> 2) & 3;
			rx[port].sbtm_info.grdm_min = (tmp >> 4) & 3;
			rx[port].sbtm_info.grdm_lum = (tmp >> 6) & 3;
			tmp = pkt->cnt.md[2];
			rx[port].sbtm_info.frm_pb_limit_int = tmp & 0x1f;
			rx[port].sbtm_info.frm_pb_limit_int <<= 8;
			tmp = pkt->cnt.md[3];
			rx[port].sbtm_info.frm_pb_limit_int |= tmp;
			pkt->pkttype = 0;
			rx[port].sbtm_info.flag = true;
			break;
		case EMP_AMDV:
			rx[port].emp_dv_info.flag = true;
			rx[port].emp_dv_info.dv_pkt_cnt = rx[port].emp_dsf_info[i].pkt_cnt;
			if (rx[port].emp_dv_info.dv_pkt_cnt > 32) {
				if (log_level & LOG_EN)
					rx_pr("%s dv_pkt_cnt(%d) too large set to 32\n", __func__,
						rx[port].emp_dv_info.dv_pkt_cnt);
				rx[port].emp_dv_info.dv_pkt_cnt = 32;
			}
			memcpy(rx[port].emp_dv_info.dv_addr, (u8 *)pkt,
				rx[port].emp_dv_info.dv_pkt_cnt * 32);
			break;
		case EMP_CUVA:
			rx[port].emp_cuva_info.flag = true;
			rx[port].emp_cuva_info.emds_addr = (u8 *)pkt;
			rx[port].emp_cuva_info.cuva_emds_size =
				rx[port].emp_dsf_info[i].pkt_cnt * 32;
			break;
		case EMP_CVTEM:
			// to do
			rx[port].cvtem_info.dsc_flag = true;
			rx[port].cvtem_info.dsc_pkt_cnt = rx[port].emp_dsf_info[i].pkt_cnt;
			if (rx[port].cvtem_info.dsc_pkt_cnt > 6) {
				if (log_level & LOG_EN)
					rx_pr("%s cvtem_pkt_cnt(%d) too large set to 6\n", __func__,
						rx[port].cvtem_info.dsc_pkt_cnt);
				rx[port].cvtem_info.dsc_pkt_cnt = 6;
			}
			memcpy(rx[port].cvtem_info.dsc_info, (u8 *)pkt + 11, 21);
			memcpy(rx[port].cvtem_info.dsc_info + 21, (u8 *)pkt + 36, 28);
			memcpy(rx[port].cvtem_info.dsc_info + 49, (u8 *)pkt + 68, 28);
			memcpy(rx[port].cvtem_info.dsc_info + 77, (u8 *)pkt + 100, 28);
			memcpy(rx[port].cvtem_info.dsc_info + 105, (u8 *)pkt + 132, 23);
			memcpy(rx[port].cvtem_info.dsc_info + 128, (u8 *)pkt + 159, 8);
			if (log_level & 0x400)
				dump_cvtem_packet(port);
			parse_dsc_pps_data(rx[port].cvtem_info.dsc_info, port);
			break;
		default:
			memset(&rx[port].vtem_info, 0, sizeof(struct vtem_info_s));
			memset(&rx[port].sbtm_info, 0, sizeof(struct sbtm_info_s));
			break;
		};
	}
	rx[port].emp_pkt_rev = false;
	rx[port].emp_dsf_cnt = 0;
	for (i = 0; i < EMP_DSF_CNT_MAX; i++)
		memset(&rx[port].emp_dsf_info[i], 0, sizeof(struct emp_dsf_st));
	if (log_level == 0x121)
		log_level = LOG_EN;
}

void rx_get_aif_info(u8 port)
{
	struct aud_infoframe_st *pkt;

	pkt = (struct aud_infoframe_st *)&rx_pkt[port].aud_pktinfo;
	rx[port].aud_info.channel_count = pkt->ch_count;
	/*rx[port].aud_info.coding_type = pkt->coding_type;*/
	rx[port].aud_info.auds_ch_alloc = pkt->ca;
	/*if (log_level & PACKET_LOG) {*/
	/*	rx_pr("cc=%x\n", pkt->ch_count);*/
	/*	rx_pr("ct=%x\n", pkt->coding_type);*/
	/*	rx_pr("ca=%x\n", pkt->ca);*/
	/*}*/
}

void dump_pktinfo_status(u8 port)
{
	rx_pr("vsi pkt:%d\n", rxpktsts[port].pkt_cnt_vsi);
	rx_pr("spd pkt:%d\n", rxpktsts[port].pkt_cnt_spd);
	rx_pr("emp pkt:%d\n", rxpktsts[port].pkt_cnt_emp);
	rx_pr("drm pkt:%d\n", rxpktsts[port].pkt_cnt_drm);
}

void rx_get_freesync_info(u8 port)
{
	struct spd_infoframe_st *pkt;

	pkt = (struct spd_infoframe_st *)&rx_pkt[port].spd_info;

	if (log_level & PACKET_LOG)
		rx_pr("ieee-%x\n", pkt->des_u.freesync.ieee);

	if (pkt->des_u.freesync.ieee == IEEE_FREESYNC) {
		rx[port].free_sync_sts = pkt->des_u.freesync.supported +
			(pkt->des_u.freesync.enabled << 1) +
			(pkt->des_u.freesync.active  << 2);
	} else {
		rx[port].free_sync_sts = 0;
	}
}

