// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_common.h>
#include <hdmitx_boot_parameters.h>

int hdmitx_format_para_init(struct hdmi_format_para *para,
		enum hdmi_vic vic, u32 frac_rate_policy,
		enum hdmi_colorspace cs, enum hdmi_color_depth cd,
		enum hdmi_quantization_range cr);
const struct hdmi_timing *hdmitx_mode_match_timing_name(const char *name);

struct hdmitx_base_state *hdmitx_get_mod_state(struct hdmitx_common *tx_common,
					       enum HDMITX_MODULE type)
{
	if (type < HDMITX_MAX_MODULE)
		return tx_common->states[type];

	return NULL;
}

void hdmitx_get_init_state(struct hdmitx_common *tx_common,
					struct hdmitx_binding_state *state)
{
	struct hdmi_format_para *para = &tx_common->fmt_para;

	state->hts.vic = tx_common->cur_VIC;
	state->hts.cd = para->cd;
	state->hts.cs = para->cs;
}
EXPORT_SYMBOL(hdmitx_get_init_state);

int hdmitx_common_init(struct hdmitx_common *tx_comm, struct hdmitx_hw_common *hw_comm)
{
	struct hdmitx_boot_param *boot_param = get_hdmitx_boot_params();

	/*load tx boot params*/
	tx_comm->hdr_priority = boot_param->hdr_mask;
	memcpy(tx_comm->hdmichecksum, boot_param->edid_chksum, sizeof(tx_comm->hdmichecksum));

	memcpy(tx_comm->fmt_attr, boot_param->color_attr, sizeof(tx_comm->fmt_attr));
	memcpy(tx_comm->backup_fmt_attr, boot_param->color_attr, sizeof(tx_comm->fmt_attr));

	tx_comm->frac_rate_policy = boot_param->fraction_refreshrate;
	tx_comm->backup_frac_rate_policy = boot_param->fraction_refreshrate;
	tx_comm->config_csc_en = boot_param->config_csc;

	hdmitx_format_para_reset(&tx_comm->fmt_para);

	tx_comm->res_1080p = 0;
	tx_comm->max_refreshrate = 60;

	tx_comm->tx_hw = hw_comm;

	/*mutex init*/
	mutex_init(&tx_comm->setclk_mutex);
	return 0;
}

int hdmitx_common_destroy(struct hdmitx_common *tx_comm)
{
	return 0;
}

int hdmitx_common_validate_vic(struct hdmitx_common *tx_comm, u32 vic)
{
	const struct hdmi_timing *timing = hdmitx_mode_vic_to_hdmi_timing(vic);

	if (!timing)
		return -EINVAL;

	/*soc level filter*/
	/*filter 1080p max size.*/
	if (tx_comm->res_1080p) {
		/* if the vic equals to HDMI_UNKNOWN or VESA,
		 * then create it as over limited
		 */
		if (vic == HDMI_0_UNKNOWN || vic >= HDMITX_VESA_OFFSET)
			return -ERANGE;

		/* check the resolution is over 1920x1080 or not */
		if (timing->h_active > 1920 || timing->v_active > 1080)
			return -ERANGE;

		/* check the fresh rate is over 60hz or not */
		if (timing->v_freq > 60000)
			return -ERANGE;

		/* test current vic is over 150MHz or not */
		if (timing->pixel_freq > 150000)
			return -ERANGE;
	}
	/*filter max refreshrate.*/
	if (timing->v_freq > (tx_comm->max_refreshrate * 1000)) {
		//pr_info("validate refreshrate (%s)-(%d) fail\n", timing->name, timing->v_freq);
		return -EACCES;
	}

	/*ip level filter*/
	if (tx_comm->tx_hw->validatemode(vic) != 0)
		return -EPERM;

	return 0;
}

int hdmitx_common_validate_format_para(struct hdmitx_common *tx_comm,
	struct hdmi_format_para *para)
{
	int ret = 0;
	unsigned int calc_tmds_clk = para->tmds_clk / 1000;

	if (para->vic == HDMI_0_UNKNOWN)
		return -EPERM;

	/* if current status already limited to 1080p, so here also needs to
	 * limit the rx_max_tmds_clk as 150 * 1.5 = 225 to make the valid mode
	 * checking works
	 */
	if (tx_comm->res_1080p) {
		if (calc_tmds_clk > 225)
			return -ERANGE;
	}

	ret = hdmitx_edid_validate_format_para(&tx_comm->rxcap, para);

	return ret;
}

int hdmitx_common_build_format_para(struct hdmitx_common *tx_comm,
		struct hdmi_format_para *para, enum hdmi_vic vic, u32 frac_rate_policy,
		enum hdmi_colorspace cs, enum hdmi_color_depth cd, enum hdmi_quantization_range cr)
{
	int ret = 0;

	ret = hdmitx_format_para_init(para, vic, frac_rate_policy, cs, cd, cr);
	if (ret == 0)
		ret = tx_comm->tx_hw->calcformatpara(para);
	if (ret < 0)
		hdmitx_format_para_print(para);

	return ret;
}

int hdmitx_common_init_bootup_format_para(struct hdmitx_common *tx_comm,
		struct hdmi_format_para *para)
{
	int ret = 0;
	struct hdmitx_hw_common *tx_hw = tx_comm->tx_hw;

	if (tx_hw->getstate(tx_hw, STAT_TX_OUTPUT, 0)) {
		para->vic = tx_hw->getstate(tx_hw, STAT_VIDEO_VIC, 0);
		para->cs = tx_hw->getstate(tx_hw, STAT_VIDEO_CS, 0);
		para->cd = tx_hw->getstate(tx_hw, STAT_VIDEO_CD, 0);

		pr_info("%s: init uboot format para (%d,%d,%d)\n",
			__func__, para->vic, para->cs, para->cd);

		ret = hdmitx_common_build_format_para(tx_comm, para, para->vic,
			tx_comm->frac_rate_policy, para->cs, para->cd,
			HDMI_QUANTIZATION_RANGE_FULL);
		if (ret == 0) {
			pr_info("%s init ok\n", __func__);
			hdmitx_format_para_print(para);
		}

		return ret;
	} else {
		return hdmitx_format_para_reset(para);
	}
}

int hdmitx_hpd_notify_unlocked(struct hdmitx_common *tx_comm)
{
	if (tx_comm->drm_hpd_cb.callback)
		tx_comm->drm_hpd_cb.callback(tx_comm->drm_hpd_cb.data);

	return 0;
}

int hdmitx_register_hpd_cb(struct hdmitx_common *tx_comm, struct connector_hpd_cb *hpd_cb)
{
	mutex_lock(&tx_comm->setclk_mutex);
	tx_comm->drm_hpd_cb.callback = hpd_cb->callback;
	tx_comm->drm_hpd_cb.data = hpd_cb->data;
	mutex_unlock(&tx_comm->setclk_mutex);
	return 0;
}

unsigned char *hdmitx_get_raw_edid(struct hdmitx_common *tx_comm)
{
	if (tx_comm->edid_ptr)
		return tx_comm->edid_ptr;
	else
		return tx_comm->EDID_buf;
}

int hdmitx_setup_attr(struct hdmitx_common *tx_comm, const char *buf)
{
	char attr[16] = {0};
	int len = strlen(buf);

	if (len <= 16)
		memcpy(attr, buf, len);
	memcpy(tx_comm->fmt_attr, attr, sizeof(tx_comm->fmt_attr));
	return 0;
}

int hdmitx_get_attr(struct hdmitx_common *tx_comm, char attr[16])
{
	memcpy(attr, tx_comm->fmt_attr, sizeof(tx_comm->fmt_attr));
	return 0;
}

int hdmitx_get_hdrinfo(struct hdmitx_common *tx_comm, struct hdr_info *hdrinfo)
{
	struct rx_cap *prxcap = &tx_comm->rxcap;

	memcpy(hdrinfo, &prxcap->hdr_info, sizeof(struct hdr_info));
	hdrinfo->colorimetry_support = prxcap->colorimetry_data;

	return 0;
}

static unsigned char __nosavedata edid_checkvalue[4] = {0};

static int xtochar(u8 value, u8 *checksum)
{
	if (((value  >> 4) & 0xf) <= 9)
		checksum[0] = ((value  >> 4) & 0xf) + '0';
	else
		checksum[0] = ((value  >> 4) & 0xf) - 10 + 'a';

	if ((value & 0xf) <= 9)
		checksum[1] = (value & 0xf) + '0';
	else
		checksum[1] = (value & 0xf) - 10 + 'a';

	return 0;
}

int hdmitx_update_edid_chksum(u8 *buf, u32 block_cnt, struct rx_cap *rxcap)
{
	u32 i, length, max;

	if (!buf)
		return -EINVAL;

	length = sizeof(edid_checkvalue);
	memset(edid_checkvalue, 0x00, length);

	max = (block_cnt > length) ? length : block_cnt;

	for (i = 0; i < max; i++)
		edid_checkvalue[i] = *(buf + (i + 1) * 128 - 1);

	rxcap->chksum[0] = '0';
	rxcap->chksum[1] = 'x';

	for (i = 0; i < 4; i++)
		xtochar(edid_checkvalue[i], &rxcap->chksum[2 * i + 2]);

	return 0;
}

static enum hdmi_vic get_alternate_ar_vic(enum hdmi_vic vic)
{
	int i = 0;
	struct {
		u32 mode_16x9_vic;
		u32 mode_4x3_vic;
	} vic_pairs[] = {
		{HDMI_7_720x480i60_16x9, HDMI_6_720x480i60_4x3},
		{HDMI_3_720x480p60_16x9, HDMI_2_720x480p60_4x3},
		{HDMI_22_720x576i50_16x9, HDMI_21_720x576i50_4x3},
		{HDMI_18_720x576p50_16x9, HDMI_17_720x576p50_4x3},
	};

	for (i = 0; i < ARRAY_SIZE(vic_pairs); i++) {
		if (vic_pairs[i].mode_16x9_vic == vic)
			return vic_pairs[i].mode_4x3_vic;
		if (vic_pairs[i].mode_4x3_vic == vic)
			return vic_pairs[i].mode_16x9_vic;
	}

	return HDMI_0_UNKNOWN;
}

int hdmitx_common_check_valid_para_of_vic(struct hdmitx_common *tx_comm, enum hdmi_vic vic)
{
	struct hdmi_format_para tst_para;
	enum hdmi_color_depth cd; /* cd8, cd10 or cd12 */
	enum hdmi_colorspace cs; /* 0/1/2/3: rgb/422/444/420 */
	const enum hdmi_quantization_range cr = HDMI_QUANTIZATION_RANGE_FULL; /*default to full*/
	int i = 0;

	if (vic == HDMI_0_UNKNOWN)
		return -EINVAL;

	if (vic == HDMI_96_3840x2160p50_16x9 ||
		vic == HDMI_97_3840x2160p60_16x9 ||
		vic == HDMI_101_4096x2160p50_256x135 ||
		vic == HDMI_102_4096x2160p60_256x135) {
		cs = HDMI_COLORSPACE_YUV420;
		cd = COLORDEPTH_24B;
		if (hdmitx_common_build_format_para(tx_comm,
			&tst_para, vic, false, cs, cd, cr) == 0 &&
			hdmitx_common_validate_format_para(tx_comm, &tst_para) == 0) {
			return 0;
		}
	}

	struct {
		enum hdmi_colorspace cs;
		enum hdmi_color_depth cd;
	} test_color_attr[] = {
		{HDMI_COLORSPACE_RGB, COLORDEPTH_24B},
		{HDMI_COLORSPACE_YUV444, COLORDEPTH_24B},
		{HDMI_COLORSPACE_YUV422, COLORDEPTH_36B},
	};

	for (i = 0; i < ARRAY_SIZE(test_color_attr); i++) {
		cs = test_color_attr[i].cs;
		cd = test_color_attr[i].cd;
		hdmitx_format_para_reset(&tst_para);
		if (hdmitx_common_build_format_para(tx_comm,
			&tst_para, vic, false, cs, cd, cr) == 0 &&
			hdmitx_common_validate_format_para(tx_comm, &tst_para) == 0) {
			return 0;
		}
	}

	return -EPERM;
}

int hdmitx_common_parse_vic_in_edid(struct hdmitx_common *tx_comm, const char *mode)
{
	const struct hdmi_timing *timing;
	enum hdmi_vic vic = HDMI_0_UNKNOWN;
	enum hdmi_vic alternate_vic = HDMI_0_UNKNOWN;

	/*parse by name to find default mode*/
	timing = hdmitx_mode_match_timing_name(mode);
	if (!timing || timing->vic == HDMI_0_UNKNOWN)
		return HDMI_0_UNKNOWN;

	vic = timing->vic;
	/* for compatibility: 480p/576p
	 * 480p/576p use same short name in hdmitx_timing table, so when match name, will return
	 * 4x3 mode fist. But user prefer 16x9 first, so try 16x9 first;
	 */
	alternate_vic = get_alternate_ar_vic(vic);
	if (alternate_vic != HDMI_0_UNKNOWN) {
		//pr_info("get alternate vic %d->%d\n", vic, alternate_vic);
		vic = alternate_vic;
	}

	/*check if vic supported by rx*/
	if (hdmitx_edid_validate_mode(&tx_comm->rxcap, vic) == 0)
		return vic;

	/* for compatibility: 480p/576p, will get 0 if there is no alternate vic;*/
	alternate_vic = get_alternate_ar_vic(vic);
	if (alternate_vic != vic) {
		//pr_info("get alternate vic %d->%d\n", vic, alternate_vic);
		vic = alternate_vic;
	}

	if (vic != HDMI_0_UNKNOWN && hdmitx_edid_validate_mode(&tx_comm->rxcap, vic) != 0)
		vic = HDMI_0_UNKNOWN;

	/* Dont call hdmitx_common_check_valid_para_of_vic anymore.
	 * This function used to parse user passed mode name which should already
	 * checked by hdmitx_common_check_valid_para_of_vic().
	 */

	if (vic == HDMI_0_UNKNOWN)
		pr_err("%s: parse mode %s vic %d\n", __func__, mode, vic);

	return vic;
}

/********************************Debug function***********************************/
int hdmitx_load_edid_file(char *path)
{
	/*todo: sync function <load_edid_data>.*/
	return 0;
}

int hdmitx_save_edid_file(unsigned char *rawedid, char *path)
{
#ifdef CONFIG_AMLOGIC_ENABLE_MEDIA_FILE
	struct file *filp = NULL;
	loff_t pos = 0;
	char line[128] = {0};
	u32 i = 0, j = 0, k = 0, size = 0, block_cnt = 0;
	u32 index = 0, tmp = 0;

	filp = filp_open(path, O_RDWR | O_CREAT, 0666);
	if (IS_ERR(filp)) {
		pr_info("[%s] failed to open/create file: |%s|\n",
			__func__, path);
		goto PROCESS_END;
	}

	block_cnt = rawedid[0x7e] + 1;
	if (rawedid[0x7e] != 0 &&
		rawedid[128 + 4] == 0xe2 &&
		rawedid[128 + 5] == 0x78)
		block_cnt = rawedid[128 + 6] + 1;

	/* dump as txt file*/
	for (i = 0; i < block_cnt; i++) {
		for (j = 0; j < 8; j++) {
			for (k = 0; k < 16; k++) {
				index = i * 128 + j * 16 + k;
				tmp = rawedid[index];
				snprintf((char *)&line[k * 6], 7,
					 "0x%02x, ",
					 tmp);
			}
			line[16 * 6 - 1] = '\n';
			line[16 * 6] = 0x0;
			pos = (i * 8 + j) * 16 * 6;
		}
	}

	pr_info("[%s] write %d bytes to file %s\n", __func__, size, path);

	vfs_fsync(filp, 0);
	filp_close(filp, NULL);

PROCESS_END:
#else
	pr_err("Not support write file.\n");
#endif
	return 0;
}

int hdmitx_print_sink_cap(struct hdmitx_common *tx_comm,
		char *buffer, int buffer_len)
{
	int i, pos = 0;
	struct rx_cap *prxcap = &tx_comm->rxcap;

	pos += snprintf(buffer + pos, buffer_len - pos,
		"Rx Manufacturer Name: %s\n", prxcap->IDManufacturerName);
	pos += snprintf(buffer + pos, buffer_len - pos,
		"Rx Product Code: %02x%02x\n",
		prxcap->IDProductCode[0],
		prxcap->IDProductCode[1]);
	pos += snprintf(buffer + pos, buffer_len - pos,
		"Rx Serial Number: %02x%02x%02x%02x\n",
		prxcap->IDSerialNumber[0],
		prxcap->IDSerialNumber[1],
		prxcap->IDSerialNumber[2],
		prxcap->IDSerialNumber[3]);
	pos += snprintf(buffer + pos, buffer_len - pos,
		"Rx Product Name: %s\n", prxcap->ReceiverProductName);

	pos += snprintf(buffer + pos, buffer_len - pos,
		"Manufacture Week: %d\n", prxcap->manufacture_week);
	pos += snprintf(buffer + pos, buffer_len - pos,
		"Manufacture Year: %d\n", prxcap->manufacture_year + 1990);

	pos += snprintf(buffer + pos, buffer_len - pos,
		"Physical size(mm): %d x %d\n",
		prxcap->physical_width, prxcap->physical_height);

	pos += snprintf(buffer + pos, buffer_len - pos,
		"EDID Version: %d.%d\n",
		prxcap->edid_version, prxcap->edid_revision);

	pos += snprintf(buffer + pos, buffer_len - pos,
		"EDID block number: 0x%x\n", tx_comm->EDID_buf[0x7e]);
	pos += snprintf(buffer + pos, buffer_len - pos,
		"blk0 chksum: 0x%02x\n", prxcap->blk0_chksum);

/*
 *	pos += snprintf(buffer + pos, buffer_len - pos,
 *		"Source Physical Address[a.b.c.d]: %x.%x.%x.%x\n",
 *		hdmitx_device->hdmi_info.vsdb_phy_addr.a,
 *		hdmitx_device->hdmi_info.vsdb_phy_addr.b,
 *		hdmitx_device->hdmi_info.vsdb_phy_addr.c,
 *		hdmitx_device->hdmi_info.vsdb_phy_addr.d);
 */
	// TODO native_vic2
	pos += snprintf(buffer + pos, buffer_len - pos,
		"native Mode %x, VIC (native %d):\n",
		prxcap->native_Mode, prxcap->native_vic);

	pos += snprintf(buffer + pos, buffer_len - pos,
		"ColorDeepSupport %x\n", prxcap->ColorDeepSupport);

	for (i = 0; i < prxcap->VIC_count ; i++) {
		pos += snprintf(buffer + pos, buffer_len - pos, "%d ",
		prxcap->VIC[i]);
	}
	pos += snprintf(buffer + pos, buffer_len - pos, "\n");
	pos += snprintf(buffer + pos, buffer_len - pos,
		"Audio {format, channel, freq, cce}\n");
	for (i = 0; i < prxcap->AUD_count; i++) {
		pos += snprintf(buffer + pos, buffer_len - pos,
			"{%d, %d, %x, %x}\n",
			prxcap->RxAudioCap[i].audio_format_code,
			prxcap->RxAudioCap[i].channel_num_max,
			prxcap->RxAudioCap[i].freq_cc,
			prxcap->RxAudioCap[i].cc3);
	}
	pos += snprintf(buffer + pos, buffer_len - pos,
		"Speaker Allocation: %x\n", prxcap->RxSpeakerAllocation);
	pos += snprintf(buffer + pos, buffer_len - pos,
		"Vendor: 0x%x ( %s device)\n",
		prxcap->ieeeoui, (prxcap->ieeeoui) ? "HDMI" : "DVI");

	pos += snprintf(buffer + pos, buffer_len - pos,
		"MaxTMDSClock1 %d MHz\n", prxcap->Max_TMDS_Clock1 * 5);

	if (prxcap->hf_ieeeoui) {
		pos +=
		snprintf(buffer + pos,
			 buffer_len - pos, "Vendor2: 0x%x\n",
			prxcap->hf_ieeeoui);
		pos += snprintf(buffer + pos, buffer_len - pos,
			"MaxTMDSClock2 %d MHz\n", prxcap->Max_TMDS_Clock2 * 5);
	}

	if (prxcap->allm)
		pos += snprintf(buffer + pos, buffer_len - pos, "ALLM: %x\n",
				prxcap->allm);

	pos += snprintf(buffer + pos, buffer_len - pos, "vLatency: ");
	if (prxcap->vLatency == LATENCY_INVALID_UNKNOWN)
		pos += snprintf(buffer + pos, buffer_len - pos,
				" Invalid/Unknown\n");
	else if (prxcap->vLatency == LATENCY_NOT_SUPPORT)
		pos += snprintf(buffer + pos, buffer_len - pos,
			" UnSupported\n");
	else
		pos += snprintf(buffer + pos, buffer_len - pos,
			" %d\n", prxcap->vLatency);

	pos += snprintf(buffer + pos, buffer_len - pos, "aLatency: ");
	if (prxcap->aLatency == LATENCY_INVALID_UNKNOWN)
		pos += snprintf(buffer + pos, buffer_len - pos,
				" Invalid/Unknown\n");
	else if (prxcap->aLatency == LATENCY_NOT_SUPPORT)
		pos += snprintf(buffer + pos, buffer_len - pos,
			" UnSupported\n");
	else
		pos += snprintf(buffer + pos, buffer_len - pos, " %d\n",
			prxcap->aLatency);

	pos += snprintf(buffer + pos, buffer_len - pos, "i_vLatency: ");
	if (prxcap->i_vLatency == LATENCY_INVALID_UNKNOWN)
		pos += snprintf(buffer + pos, buffer_len - pos,
				" Invalid/Unknown\n");
	else if (prxcap->i_vLatency == LATENCY_NOT_SUPPORT)
		pos += snprintf(buffer + pos, buffer_len - pos,
			" UnSupported\n");
	else
		pos += snprintf(buffer + pos, buffer_len - pos, " %d\n",
			prxcap->i_vLatency);

	pos += snprintf(buffer + pos, buffer_len - pos, "i_aLatency: ");
	if (prxcap->i_aLatency == LATENCY_INVALID_UNKNOWN)
		pos += snprintf(buffer + pos, buffer_len - pos,
				" Invalid/Unknown\n");
	else if (prxcap->i_aLatency == LATENCY_NOT_SUPPORT)
		pos += snprintf(buffer + pos, buffer_len - pos,
			" UnSupported\n");
	else
		pos += snprintf(buffer + pos, buffer_len - pos, " %d\n",
			prxcap->i_aLatency);

	if (prxcap->colorimetry_data)
		pos += snprintf(buffer + pos, buffer_len - pos,
			"ColorMetry: 0x%x\n", prxcap->colorimetry_data);
	pos += snprintf(buffer + pos, buffer_len - pos, "SCDC: %x\n",
		prxcap->scdc_present);
	pos += snprintf(buffer + pos, buffer_len - pos, "RR_Cap: %x\n",
		prxcap->scdc_rr_capable);
	pos +=
	snprintf(buffer + pos, buffer_len - pos, "LTE_340M_Scramble: %x\n",
		 prxcap->lte_340mcsc_scramble);

	if (prxcap->dv_info.ieeeoui == DOVI_IEEEOUI)
		pos += snprintf(buffer + pos, buffer_len - pos,
			"  DolbyVision%d", prxcap->dv_info.ver);

	if (prxcap->hdr_info2.hdr_support)
		pos += snprintf(buffer + pos, buffer_len - pos, "  HDR/%d",
			prxcap->hdr_info2.hdr_support);
	if (prxcap->dc_y444 || prxcap->dc_30bit || prxcap->dc_30bit_420)
		pos += snprintf(buffer + pos, buffer_len - pos, "  DeepColor");
	pos += snprintf(buffer + pos, buffer_len - pos, "\n");

	/* for checkvalue which maybe used by application to adjust
	 * whether edid is changed
	 */
	pos += snprintf(buffer + pos, buffer_len - pos,
			"checkvalue: 0x%02x%02x%02x%02x\n",
			edid_checkvalue[0],
			edid_checkvalue[1],
			edid_checkvalue[2],
			edid_checkvalue[3]);

	return pos;
}

