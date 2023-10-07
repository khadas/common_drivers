// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/spinlock.h>
#include <linux/rtc.h>
#include <linux/timekeeping.h>
#include <linux/amlogic/media/vout/hdmi_tx_ext.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_common.h>
#include <hdmitx_boot_parameters.h>

int hdmitx_format_para_init(struct hdmi_format_para *para,
		enum hdmi_vic vic, u32 frac_rate_policy,
		enum hdmi_colorspace cs, enum hdmi_color_depth cd,
		enum hdmi_quantization_range cr);
const struct hdmi_timing *hdmitx_mode_match_timing_name(const char *name);

void hdmitx_get_init_state(struct hdmitx_common *tx_common,
			   struct hdmitx_common_state *state)
{
	struct hdmi_format_para *para = &tx_common->fmt_para;

	memcpy(&state->para, para, sizeof(*para));
}
EXPORT_SYMBOL(hdmitx_get_init_state);

int hdmitx_common_init(struct hdmitx_common *tx_comm, struct hdmitx_hw_common *hw_comm)
{
	struct hdmitx_boot_param *boot_param = get_hdmitx_boot_params();

	/*load tx boot params*/
	tx_comm->hdr_priority = boot_param->hdr_mask;
	/* rxcap.hdmichecksum save the edid checksum of kernel */
	/* memcpy(tx_comm->rxcap.hdmichecksum, boot_param->edid_chksum, */
		/* sizeof(tx_comm->rxcap.hdmichecksum)); */
	memcpy(tx_comm->fmt_attr, boot_param->color_attr, sizeof(tx_comm->fmt_attr));

	tx_comm->frac_rate_policy = boot_param->fraction_refreshrate;
	tx_comm->config_csc_en = boot_param->config_csc;
	tx_comm->res_1080p = 0;
	tx_comm->max_refreshrate = 60;

	tx_comm->tx_hw = hw_comm;
	tx_comm->repeater_mode = 0;

	tx_comm->rxcap.physical_addr = 0xffff;
	tx_comm->debug_param.avmute_frame = 0;

	hdmitx_format_para_reset(&tx_comm->fmt_para);

	/*mutex init*/
	mutex_init(&tx_comm->hdmimode_mutex);
	return 0;
}

int hdmitx_common_attch_platform_data(struct hdmitx_common *tx_comm,
	enum HDMITX_PLATFORM_API_TYPE type, void *plt_data)
{
	switch (type) {
	case HDMITX_PLATFORM_TRACER:
		tx_comm->tx_tracer = (struct hdmitx_tracer *)plt_data;
		break;
	case HDMITX_PLATFORM_UEVENT:
		tx_comm->event_mgr = (struct hdmitx_event_mgr *)plt_data;
		break;
	default:
		pr_err("%s unknown platform api %d\n", __func__, type);
		break;
	};

	return 0;
}

int hdmitx_common_trace_event(struct hdmitx_common *tx_comm,
	enum hdmitx_event_log_bits event)
{
	static int cnt;

	hdmitx_tracer_write_event(tx_comm->tx_tracer, event);
	return hdmitx_event_mgr_send_uevent(tx_comm->event_mgr,
				HDMITX_CUR_ST_EVENT, ++cnt);
}

int hdmitx_common_destroy(struct hdmitx_common *tx_comm)
{
	if (tx_comm->tx_tracer)
		hdmitx_tracer_destroy(tx_comm->tx_tracer);
	if (tx_comm->event_mgr)
		hdmitx_event_mgr_destroy(tx_comm->event_mgr);
	return 0;
}

bool soc_resolution_limited(const struct hdmi_timing *timing, u32 res_v)
{
	if (timing->v_active > res_v)
		return 0;
	return 1;
}

bool soc_freshrate_limited(const struct hdmi_timing *timing, u32 vsync)
{
	if (!timing)
		return 0;

	if (timing->v_freq / 1000 > vsync)
		return 0;
	return 1;
}

int hdmitx_common_validate_vic(struct hdmitx_common *tx_comm, u32 vic)
{
	const struct hdmi_timing *timing = hdmitx_mode_vic_to_hdmi_timing(vic);

	if (!timing)
		return -EINVAL;

	/*soc level filter*/
	/*filter 1080p max size.*/
	if (tx_comm->res_1080p) {
		/* if the vic equals to HDMI_0_UNKNOWN or VESA,
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
	if (hdmitx_hw_validate_mode(tx_comm->tx_hw, vic) != 0)
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
EXPORT_SYMBOL(hdmitx_common_validate_format_para);

int hdmitx_common_build_format_para(struct hdmitx_common *tx_comm,
		struct hdmi_format_para *para, enum hdmi_vic vic, u32 frac_rate_policy,
		enum hdmi_colorspace cs, enum hdmi_color_depth cd, enum hdmi_quantization_range cr)
{
	int ret = 0;

	ret = hdmitx_format_para_init(para, vic, frac_rate_policy, cs, cd, cr);
	if (ret == 0)
		ret = hdmitx_hw_calc_format_para(tx_comm->tx_hw, para);
	if (ret < 0)
		hdmitx_format_para_print(para, NULL);

	return ret;
}
EXPORT_SYMBOL(hdmitx_common_build_format_para);

int hdmitx_common_init_bootup_format_para(struct hdmitx_common *tx_comm,
		struct hdmi_format_para *para)
{
	int ret = 0;
	struct hdmitx_hw_common *tx_hw = tx_comm->tx_hw;

	if (hdmitx_hw_get_state(tx_hw, STAT_TX_OUTPUT, 0)) {
		para->vic = hdmitx_hw_get_state(tx_hw, STAT_VIDEO_VIC, 0);
		para->cs = hdmitx_hw_get_state(tx_hw, STAT_VIDEO_CS, 0);
		para->cd = hdmitx_hw_get_state(tx_hw, STAT_VIDEO_CD, 0);

		ret = hdmitx_common_build_format_para(tx_comm, para, para->vic,
			tx_comm->frac_rate_policy, para->cs, para->cd,
			HDMI_QUANTIZATION_RANGE_FULL);
		if (ret == 0) {
			pr_info("%s init ok\n", __func__);
			hdmitx_format_para_print(para, NULL);
		} else {
			pr_info("%s: init uboot format para fail (%d,%d,%d)\n",
				__func__, para->vic, para->cs, para->cd);
		}

		return ret;
	} else {
		pr_info("%s hdmi is not enabled\n", __func__);
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
	tx_comm->drm_hpd_cb.callback = hpd_cb->callback;
	tx_comm->drm_hpd_cb.data = hpd_cb->data;
	return 0;
}
EXPORT_SYMBOL(hdmitx_register_hpd_cb);

/* TODO: no mutex */
int hdmitx_get_hpd_state(struct hdmitx_common *tx_comm)
{
	return tx_comm->hpd_state;
}
EXPORT_SYMBOL(hdmitx_get_hpd_state);

/* TODO: no mutex */
unsigned char *hdmitx_get_raw_edid(struct hdmitx_common *tx_comm)
{
	return tx_comm->EDID_buf;
}
EXPORT_SYMBOL(hdmitx_get_raw_edid);

int hdmitx_setup_attr(struct hdmitx_common *tx_comm, const char *buf)
{
	char attr[16] = {0};
	int len = strlen(buf);

	if (len <= 16)
		memcpy(attr, buf, len);
	memcpy(tx_comm->fmt_attr, attr, sizeof(tx_comm->fmt_attr));
	return 0;
}
EXPORT_SYMBOL(hdmitx_setup_attr);

int hdmitx_get_attr(struct hdmitx_common *tx_comm, char attr[16])
{
	memcpy(attr, tx_comm->fmt_attr, sizeof(tx_comm->fmt_attr));
	return 0;
}
EXPORT_SYMBOL(hdmitx_get_attr);

int hdmitx_get_hdrinfo(struct hdmitx_common *tx_comm, struct hdr_info *hdrinfo)
{
	struct rx_cap *prxcap = &tx_comm->rxcap;

	memcpy(hdrinfo, &prxcap->hdr_info, sizeof(struct hdr_info));
	hdrinfo->colorimetry_support = prxcap->colorimetry_data;

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
EXPORT_SYMBOL(hdmitx_common_check_valid_para_of_vic);

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
EXPORT_SYMBOL(hdmitx_common_parse_vic_in_edid);

int hdmitx_common_notify_hpd_status(struct hdmitx_common *tx_comm)
{
	if (!tx_comm->suspend_flag) {
		/*notify to userspace by uevent*/
		hdmitx_event_mgr_send_uevent(tx_comm->event_mgr,
					HDMITX_HPD_EVENT, tx_comm->hpd_state);
		hdmitx_event_mgr_send_uevent(tx_comm->event_mgr,
					HDMITX_AUDIO_EVENT, tx_comm->hpd_state);
	} else {
		/* under early suspend, only update uevent state, not
		 * post to system, in case 1.old android system will
		 * set hdmi mode, 2.audio server and audio_hal will
		 * start run, increase power consumption
		 */
		hdmitx_event_mgr_set_uevent_state(tx_comm->event_mgr,
			HDMITX_HPD_EVENT, tx_comm->hpd_state);
		hdmitx_event_mgr_set_uevent_state(tx_comm->event_mgr,
			HDMITX_AUDIO_EVENT, tx_comm->hpd_state);
	}

	/*notify to other driver module:cec/rx TODO: need lock for EDID_buf
	 * note should not be used under TV product
	 */
	if (tx_comm->tv_usage == 0) {
		if (tx_comm->hpd_state)
			hdmitx_event_mgr_notify(tx_comm->event_mgr, HDMITX_PLUG,
				tx_comm->rxcap.edid_parsing ? tx_comm->EDID_buf : NULL);
		else
			hdmitx_event_mgr_notify(tx_comm->event_mgr, HDMITX_UNPLUG, NULL);
	}
	return 0;
}

int hdmitx_common_set_allm_mode(struct hdmitx_common *tx_comm, int mode)
{
	struct hdmitx_hw_common *tx_hw_base = tx_comm->tx_hw;

	if (mode == 0) {
		tx_comm->allm_mode = 0;
		hdmitx_common_setup_vsif_packet(tx_comm, VT_ALLM, 0, NULL);
		hdmitx_common_setup_vsif_packet(tx_comm, VT_HDMI14_4K, 1, NULL);
	}
	if (mode == 1) {
		tx_comm->allm_mode = 1;
		hdmitx_common_setup_vsif_packet(tx_comm, VT_ALLM, 1, NULL);
		hdmitx_hw_cntl_config(tx_hw_base, CONF_CT_MODE, SET_CT_OFF);
	}

	if (mode == -1) {
		if (tx_comm->allm_mode == 1) {
			tx_comm->allm_mode = 0;
			hdmitx_hw_disable_packet(tx_hw_base, HDMI_PACKET_VEND);
		}
	}
	return 0;
}
EXPORT_SYMBOL(hdmitx_common_set_allm_mode);

static unsigned int get_frame_duration(struct vinfo_s *vinfo)
{
	unsigned int frame_duration;

	if (!vinfo || !vinfo->sync_duration_num)
		return 0;

	frame_duration =
		1000000 * vinfo->sync_duration_den / vinfo->sync_duration_num;
	return frame_duration;
}

int hdmitx_common_avmute_locked(struct hdmitx_common *tx_comm,
	int mute_flag, int mute_path_hint)
{
	static DEFINE_MUTEX(avmute_mutex);
	static unsigned int global_avmute_mask;
	unsigned int mute_us =
		tx_comm->debug_param.avmute_frame * get_frame_duration(&tx_comm->hdmitx_vinfo);

	mutex_lock(&avmute_mutex);

	if (mute_flag == SET_AVMUTE) {
		global_avmute_mask |= mute_path_hint;
		pr_info("%s: AVMUTE path=0x%x\n", __func__, mute_path_hint);
		hdmitx_hw_cntl_misc(tx_comm->tx_hw, MISC_AVMUTE_OP, SET_AVMUTE);
	} else if (mute_flag == CLR_AVMUTE) {
		global_avmute_mask &= ~mute_path_hint;
		/* unmute only if none of the paths are muted */
		if (global_avmute_mask == 0) {
			pr_info("%s: AV UNMUTE path=0x%x\n", __func__, mute_path_hint);
			hdmitx_hw_cntl_misc(tx_comm->tx_hw, MISC_AVMUTE_OP, CLR_AVMUTE);
		}
	} else if (mute_flag == OFF_AVMUTE) {
		hdmitx_hw_cntl_misc(tx_comm->tx_hw, MISC_AVMUTE_OP, OFF_AVMUTE);
	}
	if (mute_flag == SET_AVMUTE) {
		if (tx_comm->debug_param.avmute_frame > 0)
			msleep(mute_us / 1000);
		else if (mute_path_hint == AVMUTE_PATH_HDMITX)
			msleep(100);
	}

	mutex_unlock(&avmute_mutex);

	return 0;
}
EXPORT_SYMBOL(hdmitx_common_avmute_locked);

/********************************Debug function***********************************/
int hdmitx_load_edid_file(char *path)
{
	/*TODO: sync function <load_edid_data>.*/
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
	if (rawedid[0x7e] && rawedid[128 + 4] == EXTENSION_EEODB_EXT_TAG &&
		rawedid[128 + 5] == EXTENSION_EEODB_EXT_CODE)
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
	const struct rx_cap *prxcap = &tx_comm->rxcap;

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
		pos += snprintf(buffer + pos, buffer_len - pos, "Vendor2: 0x%x\n",
			prxcap->hf_ieeeoui);
		pos += snprintf(buffer + pos, buffer_len - pos, "MaxTMDSClock2 %d MHz\n",
			prxcap->Max_TMDS_Clock2 * 5);
	}
	if (prxcap->max_frl_rate)
		pos += snprintf(buffer + pos, buffer_len - pos, "MaxFRLRate: %d\n",
			prxcap->max_frl_rate);

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
			"checkvalue: %s\n", prxcap->hdmichecksum);

	return pos;
}

void hdmitx_get_edid(struct hdmitx_common *tx_comm, struct hdmitx_hw_common *tx_hw_base)
{
	unsigned long flags = 0;

	hdmitx_edid_buffer_clear(tx_comm->EDID_buf, sizeof(tx_comm->EDID_buf));
	hdmitx_hw_cntl_ddc(tx_hw_base, DDC_RESET_EDID, 0);
	hdmitx_hw_cntl_ddc(tx_hw_base, DDC_PIN_MUX_OP, PIN_MUX);
	/* start reading edid first time */
	hdmitx_hw_cntl_ddc(tx_hw_base, DDC_EDID_READ_DATA, 0);
	if (hdmitx_edid_is_all_zeros(tx_comm->EDID_buf)) {
		hdmitx_hw_cntl_ddc(tx_hw_base, DDC_GLITCH_FILTER_RESET, 0);
		hdmitx_hw_cntl_ddc(tx_hw_base, DDC_EDID_READ_DATA, 0);
	}
	/* If EDID is not correct at first time, then retry */
	if (!check_dvi_hdmi_edid_valid(tx_comm->EDID_buf)) {
		struct timespec64 kts;
		struct rtc_time tm;

		msleep(20);
		ktime_get_real_ts64(&kts);
		rtc_time64_to_tm(kts.tv_sec, &tm);
		if (tx_comm->hdmitx_gpios_scl != -EPROBE_DEFER)
			pr_info("UTC+0 %ptRd %ptRt DDC SCL %s\n", &tm, &tm,
			gpio_get_value(tx_comm->hdmitx_gpios_scl) ? "HIGH" : "LOW");
		if (tx_comm->hdmitx_gpios_sda != -EPROBE_DEFER)
			pr_info("UTC+0 %ptRd %ptRt DDC SDA %s\n", &tm, &tm,
			gpio_get_value(tx_comm->hdmitx_gpios_sda) ? "HIGH" : "LOW");
		msleep(80);

		/* start reading edid second time */
		hdmitx_hw_cntl_ddc(tx_hw_base, DDC_EDID_READ_DATA, 0);
		if (hdmitx_edid_is_all_zeros(tx_comm->EDID_buf)) {
			hdmitx_hw_cntl_ddc(tx_hw_base, DDC_GLITCH_FILTER_RESET, 0);
			hdmitx_hw_cntl_ddc(tx_hw_base, DDC_EDID_READ_DATA, 0);
		}
	}
	spin_lock_irqsave(&tx_comm->edid_spinlock, flags);
	hdmitx_edid_rxcap_clear(&tx_comm->rxcap);
	hdmitx_edid_parse(&tx_comm->rxcap, tx_comm->EDID_buf);

	if (tx_comm->hdr_priority == 1) { /* clear dv_info */
		struct dv_info *dv = &tx_comm->rxcap.dv_info;

		memset(dv, 0, sizeof(struct dv_info));
		pr_info("clear dv_info\n");
	}
	if (tx_comm->hdr_priority == 2) { /* clear dv_info/hdr_info */
		struct dv_info *dv = &tx_comm->rxcap.dv_info;
		struct hdr_info *hdr = &tx_comm->rxcap.hdr_info;

		memset(dv, 0, sizeof(struct dv_info));
		memset(hdr, 0, sizeof(struct hdr_info));
		pr_info("clear dv_info/hdr_info\n");
	}
	spin_unlock_irqrestore(&tx_comm->edid_spinlock, flags);
	hdmitx_event_mgr_notify(tx_comm->event_mgr,
		HDMITX_PHY_ADDR_VALID, &tx_comm->rxcap.physical_addr);
	hdmitx_edid_print(tx_comm->EDID_buf);
}

/*only for first time plugout */
bool is_tv_changed(char *cur_edid_chksum, char *boot_param_edid_chksum)
{
	char invalidchecksum[11] = {
		'i', 'n', 'v', 'a', 'l', 'i', 'd', 'c', 'r', 'c', '\0'
	};
	char emptychecksum[11] = {0};
	bool ret = false;

	if (!cur_edid_chksum || !boot_param_edid_chksum)
		return ret;

	if (memcmp(boot_param_edid_chksum, cur_edid_chksum, 10) &&
		memcmp(emptychecksum, cur_edid_chksum, 10) &&
		memcmp(invalidchecksum, boot_param_edid_chksum, 10)) {
		ret = true;
		pr_info("hdmi crc is diff between uboot and kernel\n");
	}

	return ret;
}

