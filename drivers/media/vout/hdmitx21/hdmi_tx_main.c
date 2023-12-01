// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/mm.h>
#include <linux/major.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/devinfo.h>
#include <linux/pinctrl/consumer.h>
#include <linux/amlogic/clk_measure.h>
#include <linux/mutex.h>
#include <linux/cdev.h>
#include <linux/ctype.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>
#include <linux/input.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/reboot.h>
#include <linux/i2c.h>
#include <linux/miscdevice.h>
//#include <linux/amlogic/cpu_version.h>
#include <linux/amlogic/media/vout/vinfo.h>
#include <linux/amlogic/media/vout/vout_notify.h>
#if IS_ENABLED(CONFIG_AMLOGIC_SND_SOC)
#include <linux/amlogic/media/sound/aout_notify.h>
#endif
#include <linux/amlogic/media/vout/hdmi_tx_ext.h>
#include <linux/amlogic/media/vrr/vrr.h>

#include "hdmi_tx_module.h"
#include "hdmi_tx_ext.h"
#include "hdmi_tx.h"
#include "hdmi_config.h"
#include "hw/hdmi_tx_ddc.h"

#include <linux/amlogic/gki_module.h>
#include <linux/component.h>
#include <uapi/drm/drm_mode.h>
#include <drm/amlogic/meson_drm_bind.h>
#include <../../vin/tvin/tvin_global.h>
#include <../../vin/tvin/hdmirx/hdmi_rx_repeater.h>
#include <hdmitx_boot_parameters.h>
#include <hdmitx_drm_hook.h>
#include <hdmitx_sysfs_common.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_common.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_format_para.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_platform_linux.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_audio.h>

#define HDMI_TX_COUNT 32
#define HDMI_TX_POOL_NUM  6
#define HDMI_TX_RESOURCE_NUM 4
#define HDMI_TX_PWR_CTRL_NUM	6

static u8 hdmi_allm_passthough_en;
unsigned int rx_hdcp2_ver;
//static unsigned int hdcp_ctl_lvl;

#define TEE_HDCP_IOC_START _IOW('P', 0, int)
#define to_hdmitx21_dev(x)	container_of(x, struct hdmitx_dev, tx_comm)

static struct class *hdmitx_class;
static void hdmitx_set_drm_pkt(struct master_display_info_s *data);
static void hdmitx_set_sbtm_pkt(struct vtem_sbtm_st *data);
static void hdmitx_set_vsif_pkt(enum eotf_type type, enum mode_type
	tunnel_mode, struct dv_vsif_para *data, bool signal_sdr);
static void hdmitx_set_hdr10plus_pkt(u32 flag,
				     struct hdr10plus_para *data);
static void hdmitx_set_cuva_hdr_vsif(struct cuva_hdr_vsif_para *data);
static void hdmitx_set_cuva_hdr_vs_emds(struct cuva_hdr_vs_emds_para *data);
static void hdmitx_set_emp_pkt(u8 *data, u32 type, u32 size);
static void hdmi_tx_enable_ll_mode(bool enable);
static int hdmitx_hook_drm(struct device *device);
static int hdmitx_unhook_drm(struct device *device);
static void tee_comm_dev_reg(struct hdmitx_dev *hdev);
static void tee_comm_dev_unreg(struct hdmitx_dev *hdev);
const struct hdmi_timing *hdmitx_mode_match_timing_name(const char *name);

/*
 * Normally, after the HPD in or late resume, there will reading EDID, and
 * notify application to select a hdmi mode output. But during the mode
 * setting moment, there may be HPD out. It will clear the edid data, ..., etc.
 * To avoid such case, here adds the hdmimode_mutex to let the HPD in, HPD out
 * handler and mode setting sequentially.
 */
/* static DEFINE_MUTEX(hdmimode_mutex); */

#ifdef CONFIG_OF
static struct amhdmitx_data_s amhdmitx_data_t7 = {
	.chip_type = MESON_CPU_ID_T7,
	.chip_name = "t7",
};

static struct amhdmitx_data_s amhdmitx_data_s5 = {
	.chip_type = MESON_CPU_ID_S5,
	.chip_name = "s5",
};

static struct amhdmitx_data_s amhdmitx_data_s1a = {
	.chip_type = MESON_CPU_ID_S1A,
	.chip_name = "s1a",
};

static struct amhdmitx_data_s amhdmitx_data_s7 = {
	.chip_type = MESON_CPU_ID_S7,
	.chip_name = "s7",
};

static const struct of_device_id meson_amhdmitx_of_match[] = {
	{
		.compatible	 = "amlogic, amhdmitx-t7",
		.data = &amhdmitx_data_t7,
	},
	{
		.compatible	 = "amlogic, amhdmitx-s5",
		.data = &amhdmitx_data_s5,
	},
	{
		.compatible	 = "amlogic, amhdmitx-s1a",
		.data = &amhdmitx_data_s1a,
	},
	{
		.compatible	 = "amlogic, amhdmitx-s7",
		.data = &amhdmitx_data_s7,
	},
	{},
};
#else
#define meson_amhdmitx_dt_match NULL
#endif

static struct hdmitx_dev *tx21_dev;

struct hdmitx_dev *get_hdmitx21_device(void)
{
	return tx21_dev;
}
EXPORT_SYMBOL(get_hdmitx21_device);

int get_hdmitx21_init(void)
{
	struct hdmitx_dev *hdev = get_hdmitx21_device();

	if (hdev)
		return hdev->hdmi_init;
	return 0;
}

struct vsdb_phyaddr *get_hdmitx21_phy_addr(void)
{
	struct hdmitx_dev *hdev = get_hdmitx21_device();

	return &hdev->tx_comm.rxcap.vsdb_phy_addr;
}

static struct vout_device_s hdmitx_vdev = {
	.fresh_tx_hdr_pkt = hdmitx_set_drm_pkt,
	.fresh_tx_sbtm_pkt = hdmitx_set_sbtm_pkt,
	.fresh_tx_vsif_pkt = hdmitx_set_vsif_pkt,
	.fresh_tx_hdr10plus_pkt = hdmitx_set_hdr10plus_pkt,
	.fresh_tx_cuva_hdr_vsif = hdmitx_set_cuva_hdr_vsif,
	.fresh_tx_cuva_hdr_vs_emds = hdmitx_set_cuva_hdr_vs_emds,
	.fresh_tx_emp_pkt = hdmitx_set_emp_pkt,
};

int hdmitx21_set_uevent_state(enum hdmitx_event type, int state)
{
	struct hdmitx_dev *hdev = get_hdmitx21_device();

	return hdmitx_event_mgr_set_uevent_state(hdev->tx_comm.event_mgr,
				type, state);
}

static u32 is_passthrough_switch;
int hdmitx21_set_uevent(enum hdmitx_event type, int val)
{
	struct hdmitx_dev *hdev = get_hdmitx21_device();

	return hdmitx_event_mgr_send_uevent(hdev->tx_comm.event_mgr,
				type, val, false);
}

/* There are 3 callback functions for front HDR/DV/HDR10+ modules to notify
 * hdmi drivers to send out related HDMI infoframe
 * hdmitx_set_drm_pkt() is for HDR 2084 SMPTE, HLG, etc.
 * hdmitx_set_vsif_pkt() is for DV
 * hdmitx_set_hdr10plus_pkt is for HDR10+
 * Front modules may call the 2nd, and next call the 1st, and the realted flags
 * are remained the same. So, add hdr_status_pos and place it in the above 3
 * functions to record the position.
 */
static int hdr_status_pos;

static inline void hdmitx_notify_hpd_to_rx(int hpd, void *p)
{
	struct hdmitx_dev *hdev = get_hdmitx21_device();

	if (hpd)
		hdmitx_event_mgr_notify(hdev->tx_comm.event_mgr,
				HDMITX_PLUG, p);
	else
		hdmitx_event_mgr_notify(hdev->tx_comm.event_mgr,
				HDMITX_UNPLUG, NULL);
}

static void hdmitx21_clear_packets(struct hdmitx_hw_common *tx_hw_base)
{
	hdmitx_set_drm_pkt(NULL);
	hdmitx_set_vsif_pkt(EOTF_T_NULL, 0, NULL, true);
	hdmitx_set_hdr10plus_pkt(0, NULL);
	/* clear any VSIF packet left over because of vendor<->vendor2 switch */
	hdmi_vend_infoframe_rawset(NULL, NULL);
	/* stop ALLM packet by hdmitx itself
	 * DV CTS case91: clear HF-VSIF for safety
	 */
	hdmi_vend_infoframe2_rawset(NULL, NULL);
	hdmitx_hw_cntl_config(tx_hw_base, CONF_CLR_AVI_PACKET, 0);
}

static void hdmitx21_reset_hdcp(struct hdmitx_common *tx_comm)
{
	struct hdmitx_dev *hdev = to_hdmitx21_dev(tx_comm);
	struct hdcp_t *p_hdcp = (struct hdcp_t *)hdev->am_hdcp;

	hdmitx21_disable_hdcp(hdev);
	hdmitx21_rst_stream_type(p_hdcp);
	p_hdcp->saved_upstream_type = 0;
	p_hdcp->rx_update_flag = 0;
	rx_hdcp2_ver = 0;
	is_passthrough_switch = 0;
	/* clear audio/video mute flag of stream type */
	hdmitx21_video_mute_op(1, VIDEO_MUTE_PATH_2);
	hdmitx21_audio_mute_op(1, AUDIO_MUTE_PATH_3);
}

#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
/* hdmi21 specific functions disable */
static void hdmitx21_disable_21_work(void)
{
	frl_tx_stop();
	hdmitx_vrr_disable();
	/* TODO: DSC */
}
#endif

#ifdef CONFIG_AMLOGIC_LEGACY_EARLY_SUSPEND
#include <linux/amlogic/pm.h>
static void hdmitx_early_suspend(struct early_suspend *h)
{
	struct hdmitx_dev *hdev = (struct hdmitx_dev *)h->param;

	if (hdev->aon_output) {
		HDMITX_INFO("%s return, HDMI signal enabled\n", __func__);
		return;
	}

	mutex_lock(&hdev->tx_comm.hdmimode_mutex);
	/* step1: keep hdcp auth state before suspend */
	hdmitx_hw_cntl_misc(&hdev->tx_hw.base, MISC_SUSFLAG, 1);
	/* under suspend, driver should not respond to mode setting,
	 * as it may cause logic abnormal, most importantly,
	 * it will enable hdcp and occupy DDC channel with high
	 * priority, though there's protection in system control,
	 * driver still need protection in case of old android version
	 */
	hdev->tx_comm.suspend_flag = true;

	HDMITX_INFO("Early Suspend\n");
	/* step2: clear ready status/disable phy/packets/hdcp HW */
	hdmitx_common_output_disable(&hdev->tx_comm,
		true, true, true, false);
	/* TOCONFIRM: note it will disable hpll for T7/S1A */
	hdmitx_hw_cntl(&hdev->tx_hw.base, HDMITX_EARLY_SUSPEND_RESUME_CNTL,
		HDMITX_EARLY_SUSPEND);

	/* step3: SW: post uevent to system */
	hdmitx21_set_uevent(HDMITX_HDCPPWR_EVENT, HDMI_SUSPEND);
	hdmitx21_set_uevent(HDMITX_AUDIO_EVENT, 0);

	mutex_unlock(&hdev->tx_comm.hdmimode_mutex);
}

static void hdmitx_late_resume(struct early_suspend *h)
{
	struct hdmitx_dev *hdev = (struct hdmitx_dev *)h->param;

	if (hdev->aon_output) {
		HDMITX_INFO("%s return, HDMI signal already enabled\n", __func__);
		return;
	}

	mutex_lock(&hdev->tx_comm.hdmimode_mutex);
	hdmitx_hw_cntl_config(&hdev->tx_hw.base, CONF_AUDIO_MUTE_OP, AUDIO_MUTE);
	hdmitx_common_late_resume(&hdev->tx_comm);
	HDMITX_INFO("Late Resume\n");
	mutex_unlock(&hdev->tx_comm.hdmimode_mutex);

	/* notify to drm hdmi  */
	hdmitx_fire_drm_hpd_cb_unlocked(&hdev->tx_comm);
}

/* Set avmute_set signal to HDMIRX */
static int hdmitx_reboot_notifier(struct notifier_block *nb,
				  unsigned long action, void *data)
{
	struct hdmitx_dev *hdev = container_of(nb, struct hdmitx_dev, nb);

	hdev->tx_comm.ready = 0;
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
	hdmitx_vrr_disable();
#endif

	hdmitx_common_avmute_locked(&hdev->tx_comm, SET_AVMUTE, AVMUTE_PATH_HDMITX);

#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
	frl_tx_stop();
#endif
	if (hdev->tx_comm.rxsense_policy)
		cancel_delayed_work(&hdev->tx_comm.work_rxsense);
	if (hdev->tx_comm.cedst_en)
		cancel_delayed_work(&hdev->tx_comm.work_cedst);
	hdmitx21_disable_hdcp(hdev);
	hdmitx21_rst_stream_type(hdev->am_hdcp);
	hdmitx_hw_cntl_misc(&hdev->tx_hw.base, MISC_TMDS_PHY_OP, TMDS_PHY_DISABLE);
	hdmitx_hw_cntl(&hdev->tx_hw.base,
		HDMITX_EARLY_SUSPEND_RESUME_CNTL, HDMITX_EARLY_SUSPEND);

	return NOTIFY_OK;
}

static struct early_suspend hdmitx_early_suspend_handler = {
	.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN - 10,
	.suspend = hdmitx_early_suspend,
	.resume = hdmitx_late_resume,
};
#endif

static void restore_mute(void)
{
	struct hdmitx_dev *hdev = get_hdmitx21_device();
	atomic_t kref_video_mute = hdev->kref_video_mute;
	atomic_t kref_audio_mute = hdev->kref_audio_mute;

	if (!(atomic_sub_and_test(0, &kref_video_mute))) {
		HDMITX_INFO("%s: hdmitx21_video_mute_op(0, 0) call\n", __func__);
		hdmitx21_video_mute_op(0, 0);
	}
	if (!(atomic_sub_and_test(0, &kref_audio_mute))) {
		HDMITX_INFO("%s: hdmitx21_audio_mute_op(0,0) call\n", __func__);
		hdmitx21_audio_mute_op(0, 0);
	}
}

static void hdmitx_up_hdcp_timeout_handler(struct work_struct *work)
{
	struct hdmitx_dev *hdev = container_of((struct delayed_work *)work,
		struct hdmitx_dev, work_up_hdcp_timeout);

	if (hdcp_need_control_by_upstream(hdev)) {
		HDMITX_INFO("enable hdcp as wait upstream hdcp timeout\n");
		/* note: hdcp should only be started when hdmi signal ready */
		mutex_lock(&hdev->tx_comm.hdmimode_mutex);
		if (!hdev->tx_comm.ready || !hdev->tx_comm.hpd_state) {
			HDMITX_INFO("signal ready: %d, hpd_state: %d, exit hdcp\n",
				hdev->tx_comm.ready, hdev->tx_comm.hpd_state);
			mutex_unlock(&hdev->tx_comm.hdmimode_mutex);
			return;
		}
		hdmitx_hw_cntl_misc(&hdev->tx_hw.base, MISC_AVMUTE_OP, CLR_AVMUTE);
		hdmitx21_enable_hdcp(hdev);
		mutex_unlock(&hdev->tx_comm.hdmimode_mutex);
	} else {
		HDMITX_INFO("wait upstream hdcp timeout, but now not in hdmirx channel\n");
	}
}

static void hdmitx_start_hdcp_handler(struct work_struct *work)
{
	struct hdmitx_dev *hdev = container_of((struct delayed_work *)work,
		struct hdmitx_dev, work_start_hdcp);
	unsigned long timeout_sec;
	struct hdmitx_common *tx_comm = &hdev->tx_comm;

	if (hdcp_need_control_by_upstream(hdev)) {
		if (is_passthrough_switch) {
			HDMITX_INFO("enable hdcp by passthrough switch mode\n");
			/* note: hdcp should only be started when hdmi signal ready */
			mutex_lock(&hdev->tx_comm.hdmimode_mutex);
			if (!hdev->tx_comm.ready || !tx_comm->hpd_state) {
				HDMITX_INFO("signal ready: %d, hpd_state: %d, eixt hdcp\n",
					hdev->tx_comm.ready, tx_comm->hpd_state);
				is_passthrough_switch = 0;
				mutex_unlock(&hdev->tx_comm.hdmimode_mutex);
				return;
			}
			hdmitx_hw_cntl_misc(&hdev->tx_hw.base, MISC_AVMUTE_OP, CLR_AVMUTE);
			hdmitx21_enable_hdcp(hdev);
			mutex_unlock(&hdev->tx_comm.hdmimode_mutex);
		} else {
			/* for source->hdmirx->hdmitx->tv, plug on tv side */
			/* 1.for repeater CTS, only start hdcp by upstream side
			 * 2.however if upstream source side no signal output
			 * or never start hdcp auth with hdmirx(such as PC),
			 * then we add 5S timeout period, after 5S timeout,
			 * it means that no input source start hdcp auth with
			 * hdmirx, or "no signal" on hdmirx side, then hdmitx
			 * side will start hdcp auth itself.
			 * thus both 1/2 will be satisfied.
			 */
			HDMITX_INFO("hdcp should started by upstream, wait...\n");
			/* timeout period: hdcp1.4 5S, hdcp2.2 2S */
			if (rx_hdcp2_ver)
				timeout_sec = 2;
			else
				timeout_sec = hdev->up_hdcp_timeout_sec;
			schedule_delayed_work(&hdev->work_up_hdcp_timeout,
				timeout_sec * HZ);
		}
	} else {
		mutex_lock(&hdev->tx_comm.hdmimode_mutex);
		if (!hdev->tx_comm.ready || !tx_comm->hpd_state) {
			HDMITX_INFO("signal ready: %d, hpd_state: %d, eixt hdcp2\n",
				hdev->tx_comm.ready, tx_comm->hpd_state);
			is_passthrough_switch = 0;
			mutex_unlock(&hdev->tx_comm.hdmimode_mutex);
			return;
		}
		hdmitx_hw_cntl_misc(&hdev->tx_hw.base, MISC_AVMUTE_OP, CLR_AVMUTE);
		hdmitx21_enable_hdcp(hdev);
		mutex_unlock(&hdev->tx_comm.hdmimode_mutex);
	}
	/* clear after start hdcp */
	is_passthrough_switch = 0;
}

/*disp_mode attr*/
static ssize_t disp_mode_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	int pos = 0;
	int i = 0;
	struct hdmitx_dev *hdev = get_hdmitx21_device();
	struct hdmi_format_para *para = &hdev->tx_comm.fmt_para;
	struct hdmi_timing *timing = &para->timing;
	struct vinfo_s *vinfo = &hdev->tx_comm.hdmitx_vinfo;

	pos += snprintf(buf + pos, PAGE_SIZE, "cd/cs/cr: %d/%d/%d\n", para->cd,
		para->cs, para->cr);
	pos += snprintf(buf + pos, PAGE_SIZE, "scramble/tmds_clk_div40: %d/%d\n",
		para->scrambler_en, para->tmds_clk_div40);
	pos += snprintf(buf + pos, PAGE_SIZE, "tmds_clk: %d\n", para->tmds_clk);
	pos += snprintf(buf + pos, PAGE_SIZE, "vic: %d\n", timing->vic);
	pos += snprintf(buf + pos, PAGE_SIZE, "name: %s\n", timing->name);
	pos += snprintf(buf + pos, PAGE_SIZE, "enc_idx: %d\n", hdev->tx_comm.enc_idx);
	if (timing->sname)
		pos += snprintf(buf + pos, PAGE_SIZE, "sname: %s\n",
			timing->sname);
	pos += snprintf(buf + pos, PAGE_SIZE, "pi_mode: %c\n",
		timing->pi_mode ? 'P' : 'I');
	pos += snprintf(buf + pos, PAGE_SIZE, "h/v_freq: %d/%d\n",
		timing->h_freq, timing->v_freq);
	pos += snprintf(buf + pos, PAGE_SIZE, "pixel_freq: %d\n",
		timing->pixel_freq);
	pos += snprintf(buf + pos, PAGE_SIZE, "h_total: %d\n", timing->h_total);
	pos += snprintf(buf + pos, PAGE_SIZE, "h_blank: %d\n", timing->h_blank);
	pos += snprintf(buf + pos, PAGE_SIZE, "h_front: %d\n", timing->h_front);
	pos += snprintf(buf + pos, PAGE_SIZE, "h_sync: %d\n", timing->h_sync);
	pos += snprintf(buf + pos, PAGE_SIZE, "h_back: %d\n", timing->h_back);
	pos += snprintf(buf + pos, PAGE_SIZE, "h_active: %d\n", timing->h_active);
	pos += snprintf(buf + pos, PAGE_SIZE, "v_total: %d\n", timing->v_total);
	pos += snprintf(buf + pos, PAGE_SIZE, "v_blank: %d\n", timing->v_blank);
	pos += snprintf(buf + pos, PAGE_SIZE, "v_front: %d\n", timing->v_front);
	pos += snprintf(buf + pos, PAGE_SIZE, "v_sync: %d\n", timing->v_sync);
	pos += snprintf(buf + pos, PAGE_SIZE, "v_back: %d\n", timing->v_back);
	pos += snprintf(buf + pos, PAGE_SIZE, "v_active: %d\n", timing->v_active);
	pos += snprintf(buf + pos, PAGE_SIZE, "v_sync_ln: %d\n", timing->v_sync_ln);
	pos += snprintf(buf + pos, PAGE_SIZE, "h/v_pol: %d/%d\n", timing->h_pol, timing->v_pol);
	pos += snprintf(buf + pos, PAGE_SIZE, "h/v_pict: %d/%d\n", timing->h_pict, timing->v_pict);
	pos += snprintf(buf + pos, PAGE_SIZE, "h/v_pixel: %d/%d\n",
		timing->h_pixel, timing->v_pixel);
	pos += snprintf(buf + pos, PAGE_SIZE, "name: %s\n", vinfo->name);
	pos += snprintf(buf + pos, PAGE_SIZE, "mode: %d\n", vinfo->mode);
	pos += snprintf(buf + pos, PAGE_SIZE, "ext_name: %s\n", vinfo->ext_name);
	pos += snprintf(buf + pos, PAGE_SIZE, "frac: %d\n", vinfo->frac);
	pos += snprintf(buf + pos, PAGE_SIZE, "width/height: %d/%d\n", vinfo->width, vinfo->height);
	pos += snprintf(buf + pos, PAGE_SIZE, "field_height: %d\n", vinfo->field_height);
	pos += snprintf(buf + pos, PAGE_SIZE, "aspect_ratio_num/den: %d/%d\n",
		vinfo->aspect_ratio_num, vinfo->aspect_ratio_den);
	pos += snprintf(buf + pos, PAGE_SIZE, "screen_real_width/height: %d/%d\n",
		vinfo->screen_real_width, vinfo->screen_real_height);
	pos += snprintf(buf + pos, PAGE_SIZE, "sync_duration_num/den: %d/%d\n",
		vinfo->sync_duration_num, vinfo->sync_duration_den);
	pos += snprintf(buf + pos, PAGE_SIZE, "video_clk: %d\n", vinfo->video_clk);
	pos += snprintf(buf + pos, PAGE_SIZE, "h/vtotal: %d/%d\n", vinfo->htotal, vinfo->vtotal);
	pos += snprintf(buf + pos, PAGE_SIZE, "hdmichecksum:\n");
	for (i = 0; i < sizeof(vinfo->hdmichecksum); i++)
		pos += snprintf(buf + pos, PAGE_SIZE, "%02x", vinfo->hdmichecksum[i]);
	pos += snprintf(buf + pos, PAGE_SIZE, "\n");
	pos += snprintf(buf + pos, PAGE_SIZE, "info_3d: %d\n", vinfo->info_3d);
	pos += snprintf(buf + pos, PAGE_SIZE, "fr_adj_type: %d\n", vinfo->fr_adj_type);
	pos += snprintf(buf + pos, PAGE_SIZE, "viu_color_fmt: %d\n", vinfo->viu_color_fmt);
	pos += snprintf(buf + pos, PAGE_SIZE, "viu_mux: %d\n", vinfo->viu_mux);
	/* master_display_info / hdr_info / rx_latency */

	return pos;
}

static ssize_t disp_mode_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t count)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);
	int ret = 0;

	ret = set_disp_mode(&hdev->tx_comm, buf);
	if (ret < 0)
		HDMITX_INFO("%s: set mode failed\n", __func__);
	return count;
}

static DEFINE_MUTEX(aud_mute_mutex);
void hdmitx21_audio_mute_op(u32 flag, unsigned int path)
{
	static unsigned int aud_mute_path;
	struct hdmitx_dev *hdev = get_hdmitx21_device();

	mutex_lock(&aud_mute_mutex);
	if (flag == 0)
		aud_mute_path |= path;
	else
		aud_mute_path &= ~path;
	hdev->tx_comm.cur_audio_param.aud_output_en = !aud_mute_path;

	if (flag == 0) {
		HDMITX_INFO("%s: AUD_MUTE path=0x%x\n", __func__, path);
		hdmitx_hw_cntl_config(&hdev->tx_hw.base, CONF_AUDIO_MUTE_OP, AUDIO_MUTE);
	} else {
		/* unmute only if none of the paths are muted */
		if (aud_mute_path == 0) {
			HDMITX_INFO("%s: AUD_UNMUTE path=0x%x\n", __func__, path);
			hdmitx_hw_cntl_config(&hdev->tx_hw.base, CONF_AUDIO_MUTE_OP, AUDIO_UNMUTE);
		}
	}
	mutex_unlock(&aud_mute_mutex);
}

static DEFINE_MUTEX(vid_mute_mutex);
void hdmitx21_video_mute_op(u32 flag, unsigned int path)
{
	static unsigned int vid_mute_path;
	struct hdmitx_dev *hdev = get_hdmitx21_device();

	mutex_lock(&vid_mute_mutex);
	if (flag == 0)
		vid_mute_path |= path;
	else
		vid_mute_path &= ~path;

	if (flag == 0) {
		HDMITX_INFO("%s: VID_MUTE path=0x%x\n", __func__, path);
		hdmitx_hw_cntl_config(&hdev->tx_hw.base, CONF_VIDEO_MUTE_OP, VIDEO_MUTE);
	} else {
		/* unmute only if none of the paths are muted */
		if (vid_mute_path == 0) {
			HDMITX_INFO("%s: VID_UNMUTE path=0x%x\n", __func__, path);
			hdmitx_hw_cntl_config(&hdev->tx_hw.base, CONF_VIDEO_MUTE_OP, VIDEO_UNMUTE);
		}
	}
	mutex_unlock(&vid_mute_mutex);
}

/*
 *  SDR/HDR uevent
 *  1: SDR to HDR
 * 0: HDR to SDR
 */
static void hdmitx_sdr_hdr_uevent(struct hdmitx_dev *hdev)
{
	if (hdev->hdmi_current_hdr_mode != 0) {
		/* SDR -> HDR*/
		hdmitx21_set_uevent(HDMITX_HDR_EVENT, 1);
	} else if (hdev->hdmi_current_hdr_mode == 0) {
		/* HDR -> SDR*/
		hdmitx21_set_uevent(HDMITX_HDR_EVENT, 0);
	}
}

static void hdr_work_func(struct work_struct *work)
{
	struct hdmitx_dev *hdev =
		container_of(work, struct hdmitx_dev, work_hdr);
	struct hdmi_drm_infoframe *info = &hdev->infoframes.drm.drm;

	if (hdev->hdr_transfer_feature == T_BT709 &&
	    hdev->hdr_color_feature == C_BT709) {
		HDMITX_INFO("%s: send zero DRM\n", __func__);
		hdmi_drm_infoframe_init(info);
		hdmi_drm_infoframe_set(info);
		hdmi_avi_infoframe_config(CONF_AVI_BT2020, hdev->colormetry);

		msleep(1500);/*delay 1.5s*/
		/* disable DRM packets completely ONLY if hdr transfer
		 * feature and color feature still demand SDR.
		 */
		if (hdr_status_pos == 4) {
			/* zero hdr10+ VSIF being sent - disable it */
			HDMITX_INFO("%s: disable hdr10+ vsif\n", __func__);
			/* hdmi_vend_infoframe_set(NULL); */
			hdmi_vend_infoframe_rawset(NULL, NULL);
			hdr_status_pos = 0;
		}
		if (hdev->hdr_transfer_feature == T_BT709 &&
		    hdev->hdr_color_feature == C_BT709) {
			hdev->hdmi_current_hdr_mode = 0;
			hdev->hdmi_last_hdr_mode = hdev->hdmi_current_hdr_mode;
			hdmitx_sdr_hdr_uevent(hdev);
		}
	} else {
		hdmitx_sdr_hdr_uevent(hdev);
	}
}

static bool hdmitx21_hdr_en(void)
{
	struct hdmitx_dev *hdev = get_hdmitx21_device();
	struct hdmitx_hw_common *tx_hw_base = &hdev->tx_hw.base;

	return (hdmitx_hw_get_hdr_st(tx_hw_base) & HDMI_HDR_TYPE) == HDMI_HDR_TYPE;
}

static bool hdmitx21_dv_en(void)
{
	struct hdmitx_dev *hdev = get_hdmitx21_device();
	struct hdmitx_hw_common *tx_hw_base = &hdev->tx_hw.base;

	return (hdmitx_hw_get_dv_st(tx_hw_base) & HDMI_DV_TYPE) == HDMI_DV_TYPE;
}

static bool hdmitx21_hdr10p_en(void)
{
	struct hdmitx_dev *hdev = get_hdmitx21_device();
	struct hdmitx_hw_common *tx_hw_base = &hdev->tx_hw.base;

	return (hdmitx_hw_get_hdr10p_st(tx_hw_base) & HDMI_HDR10P_TYPE) == HDMI_HDR10P_TYPE;
}

#define GET_LOW8BIT(a)	((a) & 0xff)
#define GET_HIGH8BIT(a)	(((a) >> 8) & 0xff)
static void hdmitx_set_drm_pkt(struct master_display_info_s *data)
{
	struct hdmitx_dev *hdev = get_hdmitx21_device();
	struct hdr_info *hdr_info = &hdev->tx_comm.rxcap.hdr_info;
	u8 drm_hb[3] = {0x87, 0x1, 26};
	static u8 db[28] = {0x0};
	u8 *drm_db = &db[1]; /* db[0] is the checksum */
	unsigned long flags = 0;

	HDMITX_DEBUG_PACKET("%s[%d]\n", __func__, __LINE__);
	spin_lock_irqsave(&hdev->tx_comm.edid_spinlock, flags);
	/* if ready is 0, only can clear pkt */
	if (hdev->tx_comm.ready == 0 && data) {
		spin_unlock_irqrestore(&hdev->tx_comm.edid_spinlock, flags);
		return;
	}
	if (hdr_status_pos == 4) {
		/* zero hdr10+ VSIF being sent - disable it */
		HDMITX_INFO("%s: disable hdr10+ zero vsif\n", __func__);
		/* hdmi_vend_infoframe_set(NULL); */
		hdmi_vend_infoframe_rawset(NULL, NULL);
		hdr_status_pos = 0;
	}

	/*
	 *hdr_color_feature: bit 23-16: color_primaries
	 *	1:bt709 0x9:bt2020
	 *hdr_transfer_feature: bit 15-8: transfer_characteristic
	 *	1:bt709 0xe:bt2020-10 0x10:smpte-st-2084 0x12:hlg(TODO)
	 */
	if (data) {
		if ((hdev->hdr_transfer_feature !=
			((data->features >> 8) & 0xff)) ||
			(hdev->hdr_color_feature !=
			((data->features >> 16) & 0xff)) ||
			(hdev->colormetry !=
			((data->features >> 30) & 0x1))) {
			hdev->hdr_transfer_feature =
				(data->features >> 8) & 0xff;
			hdev->hdr_color_feature =
				(data->features >> 16) & 0xff;
			hdev->colormetry =
				(data->features >> 30) & 0x1;
			HDMITX_INFO("%s: tf=%d, cf=%d, colormetry=%d\n",
				__func__,
				hdev->hdr_transfer_feature,
				hdev->hdr_color_feature,
				hdev->colormetry);
		}
	} else {
		HDMITX_INFO("%s: disable drm pkt\n", __func__);
	}
	if (hdr_status_pos != 1 && hdr_status_pos != 3)
		HDMITX_INFO("%s: tf=%d, cf=%d, colormetry=%d\n",
			__func__,
			hdev->hdr_transfer_feature,
			hdev->hdr_color_feature,
			hdev->colormetry);
	hdr_status_pos = 1;
	/* if VSIF/DV or VSIF/HDR10P packet is enabled, disable it */
	if (hdmitx21_dv_en()) {
		hdmi_avi_infoframe_config(CONF_AVI_CS, hdev->tx_comm.fmt_para.cs);
/* if using VSIF/DOVI, then only clear DV_VS10_SIG, else disable VSIF */
		if (hdmitx_hw_cntl_config(&hdev->tx_hw.base, CONF_CLR_DV_VS10_SIG, 0) == 0)
			/* hdmi_vend_infoframe_set(NULL); */
			hdmi_vend_infoframe_rawset(NULL, NULL);
	}

	/* hdr10+ content on a hdr10 sink case */
	if (hdev->hdr_transfer_feature == 0x30) {
		if (hdr_info->hdr10plus_info.ieeeoui != 0x90848B ||
		    hdr_info->hdr10plus_info.application_version != 1) {
			hdev->hdr_transfer_feature = T_SMPTE_ST_2084;
			HDMITX_INFO("%s: HDR10+ not supported, treat as hdr10\n",
				__func__);
		}
	}

	if (!data || !hdev->tx_comm.rxcap.hdr_info2.hdr_support) {
		drm_hb[1] = 0;
		drm_hb[2] = 0;
		drm_db[0] = 0;
		hdmi_drm_infoframe_set(NULL);
		hdmi_avi_infoframe_config(CONF_AVI_BT2020, hdev->colormetry);
		spin_unlock_irqrestore(&hdev->tx_comm.edid_spinlock, flags);
		return;
	}

	/*SDR*/
	if (hdev->hdr_transfer_feature == T_BT709 &&
		hdev->hdr_color_feature == C_BT709) {
		/* send zero drm only for HDR->SDR transition */
		if (drm_db[0] == 0x02 || drm_db[0] == 0x03) {
			HDMITX_INFO("%s: HDR->SDR, drm_db[0]=%d\n",
				__func__, drm_db[0]);
			hdev->colormetry = 0;
			hdmi_avi_infoframe_config(CONF_AVI_BT2020, 0);
			schedule_work(&hdev->work_hdr);
			hdmitx_tracer_write_event(hdev->tx_comm.tx_tracer,
						HDMITX_HDR_MODE_SDR);
			drm_db[0] = 0;
		}
		spin_unlock_irqrestore(&hdev->tx_comm.edid_spinlock, flags);
		return;
	}

	drm_db[1] = 0x0;
	drm_db[2] = GET_LOW8BIT(data->primaries[0][0]);
	drm_db[3] = GET_HIGH8BIT(data->primaries[0][0]);
	drm_db[4] = GET_LOW8BIT(data->primaries[0][1]);
	drm_db[5] = GET_HIGH8BIT(data->primaries[0][1]);
	drm_db[6] = GET_LOW8BIT(data->primaries[1][0]);
	drm_db[7] = GET_HIGH8BIT(data->primaries[1][0]);
	drm_db[8] = GET_LOW8BIT(data->primaries[1][1]);
	drm_db[9] = GET_HIGH8BIT(data->primaries[1][1]);
	drm_db[10] = GET_LOW8BIT(data->primaries[2][0]);
	drm_db[11] = GET_HIGH8BIT(data->primaries[2][0]);
	drm_db[12] = GET_LOW8BIT(data->primaries[2][1]);
	drm_db[13] = GET_HIGH8BIT(data->primaries[2][1]);
	drm_db[14] = GET_LOW8BIT(data->white_point[0]);
	drm_db[15] = GET_HIGH8BIT(data->white_point[0]);
	drm_db[16] = GET_LOW8BIT(data->white_point[1]);
	drm_db[17] = GET_HIGH8BIT(data->white_point[1]);
	drm_db[18] = GET_LOW8BIT(data->luminance[0]);
	drm_db[19] = GET_HIGH8BIT(data->luminance[0]);
	drm_db[20] = GET_LOW8BIT(data->luminance[1]);
	drm_db[21] = GET_HIGH8BIT(data->luminance[1]);
	drm_db[22] = GET_LOW8BIT(data->max_content);
	drm_db[23] = GET_HIGH8BIT(data->max_content);
	drm_db[24] = GET_LOW8BIT(data->max_frame_average);
	drm_db[25] = GET_HIGH8BIT(data->max_frame_average);

	/* bt2020 + gamma transfer */
	if (hdev->hdr_transfer_feature == T_BT709 &&
	    hdev->hdr_color_feature == C_BT2020) {
		if (hdev->sdr_hdr_feature == 0) {
			hdmi_drm_infoframe_set(NULL);
			hdmi_avi_infoframe_config(CONF_AVI_BT2020, SET_AVI_BT2020);
		} else if (hdev->sdr_hdr_feature == 1) {
			memset(db, 0, sizeof(db));
			hdmi_drm_infoframe_rawset(drm_hb, db);
			hdmi_avi_infoframe_config(CONF_AVI_BT2020, SET_AVI_BT2020);
		} else {
			drm_db[0] = 0x02; /* SMPTE ST 2084 */
			hdmi_drm_infoframe_rawset(drm_hb, db);
			hdmi_avi_infoframe_config(CONF_AVI_BT2020, SET_AVI_BT2020);
		}
		spin_unlock_irqrestore(&hdev->tx_comm.edid_spinlock, flags);
		return;
	}

	/*must clear hdr mode*/
	hdev->hdmi_current_hdr_mode = 0;

	/* SMPTE ST 2084 and (BT2020 or NON_STANDARD) */
	if (hdev->tx_comm.rxcap.hdr_info2.hdr_support & 0x4) {
		if (hdev->hdr_transfer_feature == T_SMPTE_ST_2084 &&
		    hdev->hdr_color_feature == C_BT2020)
			hdev->hdmi_current_hdr_mode = 1;
		else if (hdev->hdr_transfer_feature == T_SMPTE_ST_2084 &&
			 hdev->hdr_color_feature != C_BT2020)
			hdev->hdmi_current_hdr_mode = 2;
	}

	/*HLG and BT2020*/
	if (hdev->tx_comm.rxcap.hdr_info2.hdr_support & 0x8) {
		if (hdev->hdr_color_feature == C_BT2020 &&
		    (hdev->hdr_transfer_feature == T_BT2020_10 ||
		     hdev->hdr_transfer_feature == T_HLG))
			hdev->hdmi_current_hdr_mode = 3;
	}
	switch (hdev->hdmi_current_hdr_mode) {
	case 1:
		/*standard HDR*/
		drm_db[0] = 0x02; /* SMPTE ST 2084 */
		hdmi_drm_infoframe_rawset(drm_hb, db);
		hdmi_avi_infoframe_config(CONF_AVI_BT2020, SET_AVI_BT2020);
		hdmitx_tracer_write_event(hdev->tx_comm.tx_tracer,
					HDMITX_HDR_MODE_SMPTE2084);
		break;
	case 2:
		/*non standard*/
		drm_db[0] = 0x02; /* no standard SMPTE ST 2084 */
		hdmi_drm_infoframe_rawset(drm_hb, db);
		hdmi_avi_infoframe_config(CONF_AVI_BT2020, CLR_AVI_BT2020);
		break;
	case 3:
		/*HLG*/
		drm_db[0] = 0x03;/* HLG is 0x03 */
		hdmi_drm_infoframe_rawset(drm_hb, db);
		hdmi_avi_infoframe_config(CONF_AVI_BT2020, SET_AVI_BT2020);
		hdmitx_tracer_write_event(hdev->tx_comm.tx_tracer,
					HDMITX_HDR_MODE_HLG);
		break;
	case 0:
	default:
		/*other case*/
		hdmi_drm_infoframe_set(NULL);
		hdmi_avi_infoframe_config(CONF_AVI_BT2020, CLR_AVI_BT2020);
		break;
	}

	/* if sdr/hdr mode change ,notify uevent to userspace*/
	if (hdev->hdmi_current_hdr_mode != hdev->hdmi_last_hdr_mode) {
		/* NOTE: for HDR <-> HLG, also need update last mode */
		hdev->hdmi_last_hdr_mode = hdev->hdmi_current_hdr_mode;
		schedule_work(&hdev->work_hdr);
	}
	spin_unlock_irqrestore(&hdev->tx_comm.edid_spinlock, flags);
}

static struct emp_packet_st pkt_sbtm;
static void hdmitx_set_sbtm_pkt(struct vtem_sbtm_st *data)
{
	struct emp_packet_st *sbtm = &pkt_sbtm;

	if (!data) {
		hdmi_emp_infoframe_set(EMP_TYPE_SBTM, NULL);
		return;
	}
	memset(sbtm, 0, sizeof(*sbtm));
	sbtm->type = EMP_TYPE_SBTM;
	hdmi_emp_frame_set_member(sbtm, CONF_HEADER_INIT, HDMI_INFOFRAME_TYPE_EMP);
	hdmi_emp_frame_set_member(sbtm, CONF_HEADER_FIRST, 1);
	hdmi_emp_frame_set_member(sbtm, CONF_HEADER_LAST, 1);
	hdmi_emp_frame_set_member(sbtm, CONF_HEADER_SEQ_INDEX, 0);
	hdmi_emp_frame_set_member(sbtm, CONF_DS_TYPE, 1);
	hdmi_emp_frame_set_member(sbtm, CONF_SYNC, 1);
	hdmi_emp_frame_set_member(sbtm, CONF_VFR, 1);
	hdmi_emp_frame_set_member(sbtm, CONF_AFR, 0);
	hdmi_emp_frame_set_member(sbtm, CONF_NEW, 0);
	hdmi_emp_frame_set_member(sbtm, CONF_END, 0);
	hdmi_emp_frame_set_member(sbtm, CONF_ORG_ID, 1);
	hdmi_emp_frame_set_member(sbtm, CONF_DATA_SET_TAG, 3);
	hdmi_emp_frame_set_member(sbtm, CONF_DATA_SET_LENGTH, 4);
	hdmi_emp_frame_set_member(sbtm, CONF_SBTM_VER, data->sbtm_ver);
	hdmi_emp_frame_set_member(sbtm, CONF_SBTM_MODE, data->sbtm_mode);
	hdmi_emp_frame_set_member(sbtm, CONF_SBTM_TYPE, data->sbtm_type);
	hdmi_emp_frame_set_member(sbtm, CONF_SBTM_GRDM_MIN, data->grdm_min);
	hdmi_emp_frame_set_member(sbtm, CONF_SBTM_GRDM_LUM, data->grdm_lum);
	hdmi_emp_frame_set_member(sbtm, CONF_SBTM_FRMPBLIMITINT, data->frmpblimitint);
	hdmi_emp_infoframe_set(EMP_TYPE_SBTM, sbtm);
}

static struct vsif_debug_save vsif_debug_info;
static void hdmitx_set_vsif_pkt(enum eotf_type type,
				enum mode_type tunnel_mode,
				struct dv_vsif_para *data,
				bool signal_sdr)
{
	struct hdmitx_dev *hdev = get_hdmitx21_device();
	struct dv_vsif_para para = {0};
	u8 ven_hb[3] = {0x81, 0x01};
	u8 db1[28] = {0x00};
	u8 *ven_db1 = &db1[1];
	u8 db2[28] = {0x00};
	u8 *ven_db2 = &db2[1];
	u8 len = 0;
	u32 vic = hdev->tx_comm.fmt_para.vic;
	u32 hdmi_vic_4k_flag = 0;
	static enum eotf_type ltype = EOTF_T_NULL;
	static u8 ltmode = -1;
	enum hdmi_tf_type hdr_type = HDMI_NONE;
	unsigned long flags = 0;
	struct hdmi_format_para *fmt_para = &hdev->tx_comm.fmt_para;

	HDMITX_DEBUG_PACKET("%s[%d]\n", __func__, __LINE__);
	spin_lock_irqsave(&hdev->tx_comm.edid_spinlock, flags);
	if (hdev->bist_lock) {
		spin_unlock_irqrestore(&hdev->tx_comm.edid_spinlock, flags);
		return;
	}

	if (!data)
		memcpy(&vsif_debug_info.data, &para,
		       sizeof(struct dv_vsif_para));
	else
		memcpy(&vsif_debug_info.data, data,
		       sizeof(struct dv_vsif_para));
	vsif_debug_info.type = type;
	vsif_debug_info.tunnel_mode = tunnel_mode;
	vsif_debug_info.signal_sdr = signal_sdr;
	/* if ready is 0, only can clear pkt */
	if (hdev->tx_comm.ready == 0 && type != EOTF_T_NULL) {
		ltype = EOTF_T_NULL;
		ltmode = -1;
		spin_unlock_irqrestore(&hdev->tx_comm.edid_spinlock, flags);
		return;
	}
	if (hdev->tx_comm.rxcap.dv_info.ieeeoui != DV_IEEE_OUI) {
		if (type == 0 && !data && signal_sdr)
			HDMITX_INFO("TV not support DV, clr dv_vsif\n");
	}

	if (hdev->hdmi_current_eotf_type != type ||
		hdev->hdmi_current_tunnel_mode != tunnel_mode ||
		hdev->hdmi_current_signal_sdr != signal_sdr) {
		hdev->hdmi_current_eotf_type = type;
		hdev->hdmi_current_tunnel_mode = tunnel_mode;
		hdev->hdmi_current_signal_sdr = signal_sdr;
		HDMITX_INFO("%s: type=%d, tunnel_mode=%d, signal_sdr=%d\n",
			__func__, type, tunnel_mode, signal_sdr);
	}
	if (hdr_status_pos != 2)
		HDMITX_INFO("%s: type = %d\n", __func__, type);
	hdr_status_pos = 2;

	/* if DRM/HDR packet is enabled, disable it */
	hdr_type = hdmitx_hw_get_hdr_st(&hdev->tx_hw.base);
	if (hdr_type != HDMI_NONE) {
		hdev->hdr_transfer_feature = T_BT709;
		hdev->hdr_color_feature = C_BT709;
		hdev->colormetry = 0;
		hdmi_avi_infoframe_config(CONF_AVI_BT2020, hdev->colormetry);
		hdmi_drm_infoframe_set(NULL);
	}

	hdev->hdmi_current_eotf_type = type;
	hdev->hdmi_current_tunnel_mode = tunnel_mode;
	/*ver0 and ver1_15 and ver1_12bit with ll= 0 use hdmi 1.4b VSIF*/
	if (hdev->tx_comm.rxcap.dv_info.ver == 0 ||
	    (hdev->tx_comm.rxcap.dv_info.ver == 1 &&
	    hdev->tx_comm.rxcap.dv_info.length == 0xE) ||
	    (hdev->tx_comm.rxcap.dv_info.ver == 1 &&
	    hdev->tx_comm.rxcap.dv_info.length == 0xB &&
	    hdev->tx_comm.rxcap.dv_info.low_latency == 0)) {
		if (vic == HDMI_95_3840x2160p30_16x9 ||
		    vic == HDMI_94_3840x2160p25_16x9 ||
		    vic == HDMI_93_3840x2160p24_16x9 ||
		    vic == HDMI_98_4096x2160p24_256x135)
			hdmi_vic_4k_flag = 1;

		switch (type) {
		case EOTF_T_DOLBYVISION:
			len = 0x18;
			hdev->dv_src_feature = 1;
			break;
		case EOTF_T_HDR10:
		case EOTF_T_SDR:
		case EOTF_T_NULL:
		default:
			len = 0x05;
			hdev->dv_src_feature = 0;
			break;
		}

		ven_hb[2] = len;
		ven_db1[0] = 0x03;
		ven_db1[1] = 0x0c;
		ven_db1[2] = 0x00;
		ven_db1[3] = 0x00;

		if (hdmi_vic_4k_flag) {
			ven_db1[3] = 0x20;
			if (vic == HDMI_95_3840x2160p30_16x9)
				ven_db1[4] = 0x1;
			else if (vic == HDMI_94_3840x2160p25_16x9)
				ven_db1[4] = 0x2;
			else if (vic == HDMI_93_3840x2160p24_16x9)
				ven_db1[4] = 0x3;
			else/*vic == HDMI_98_4096x2160p24_256x135*/
				ven_db1[4] = 0x4;
		}
		if (type == EOTF_T_DV_AHEAD) {
			hdmi_vend_infoframe_rawset(ven_hb, db1);
			spin_unlock_irqrestore(&hdev->tx_comm.edid_spinlock, flags);
			return;
		}
		if (type == EOTF_T_DOLBYVISION) {
			/* disable forced gaming in this mode because if we are
			 * here with forced gaming on, it means TV is not DV LL
			 * capable
			 */
			if (hdev->ll_user_set_mode == HDMI_LL_MODE_ENABLE &&
			   (hdev->tx_comm.allm_mode == 1 || hdev->tx_comm.ct_mode == 1)) {
				HDMITX_INFO("DV H14b VSIF, disable forced game mode\n");
				hdmi_tx_enable_ll_mode(false);
			}
			/*first disable drm package*/
			hdmi_drm_infoframe_set(NULL);
			hdmi_vend_infoframe_rawset(ven_hb, db1);
			/* Dolby Vision Source System-on-Chip Platform Kit Version 2.6:
			 * 4.4.1 Expected AVI-IF for Dolby Vision output, need BT2020 for DV
			 */
			hdmi_avi_infoframe_config(CONF_AVI_BT2020, SET_AVI_BT2020);/*BT2020*/
			if (tunnel_mode == RGB_8BIT) {
				hdmi_avi_infoframe_config(CONF_AVI_CS, HDMI_COLORSPACE_RGB);
				hdmi_avi_infoframe_config(CONF_AVI_Q01, RGB_RANGE_FUL);
				hdmitx_tracer_write_event(hdev->tx_comm.tx_tracer,
							HDMITX_HDR_MODE_DV_STD);
			} else {
				hdmi_avi_infoframe_config(CONF_AVI_CS, HDMI_COLORSPACE_YUV422);
				hdmi_avi_infoframe_config(CONF_AVI_YQ01, YCC_RANGE_FUL);
				hdmitx_tracer_write_event(hdev->tx_comm.tx_tracer,
							HDMITX_HDR_MODE_DV_LL);
			}
		} else {
			if (hdmi_vic_4k_flag)
				hdmi_vend_infoframe_rawset(ven_hb, db1);
			else
				/* hdmi_vend_infoframe_set(NULL); */
				hdmi_vend_infoframe_rawset(NULL, NULL);
			if (signal_sdr) {
				HDMITX_INFO("DV H14b VSIF, switching signal to SDR\n");
				hdmi_avi_infoframe_config(CONF_AVI_CS, fmt_para->cs);
				hdmi_avi_infoframe_config(CONF_AVI_Q01, RGB_RANGE_LIM);
				hdmi_avi_infoframe_config(CONF_AVI_YQ01, YCC_RANGE_LIM);
				hdmi_avi_infoframe_config(CONF_AVI_BT2020, CLR_AVI_BT2020);/*BT709*/
				/* re-enable forced game mode if selected by the user */
				if (hdev->ll_user_set_mode == HDMI_LL_MODE_ENABLE) {
					HDMITX_INFO("DV H14b VSIF OFF,re-enable force game mode\n");
					hdmi_tx_enable_ll_mode(true);
				}
				hdmitx_tracer_write_event(hdev->tx_comm.tx_tracer,
							HDMITX_HDR_MODE_SDR);
			}
		}
	}
	/*ver1_12  with low_latency = 1 and ver2 use Dolby VSIF*/
	if (hdev->tx_comm.rxcap.dv_info.ver == 2 ||
	    (hdev->tx_comm.rxcap.dv_info.ver == 1 &&
	     hdev->tx_comm.rxcap.dv_info.length == 0xB &&
	     hdev->tx_comm.rxcap.dv_info.low_latency == 1) ||
	     type == EOTF_T_LL_MODE) {
		if (!data)
			data = &para;
		len = 0x1b;

		switch (type) {
		case EOTF_T_DOLBYVISION:
		case EOTF_T_LL_MODE:
			hdev->dv_src_feature = 1;
			break;
		case EOTF_T_HDR10:
		case EOTF_T_SDR:
		case EOTF_T_NULL:
		default:
			hdev->dv_src_feature = 0;
			break;
		}
		ven_hb[2] = len;
		ven_db2[0] = 0x46;
		ven_db2[1] = 0xd0;
		ven_db2[2] = 0x00;
		if (data->ver2_l11_flag == 1) {
			ven_db2[3] = data->vers.ver2_l11.low_latency |
				     data->vers.ver2_l11.dobly_vision_signal << 1 |
				     data->vers.ver2_l11.src_dm_version << 5;
			ven_db2[4] = data->vers.ver2_l11.eff_tmax_PQ_hi
				     | data->vers.ver2_l11.auxiliary_MD_present << 6
				     | data->vers.ver2_l11.backlt_ctrl_MD_present << 7
				     | 0x20; /*L11_MD_Present*/
			ven_db2[5] = data->vers.ver2_l11.eff_tmax_PQ_low;
			ven_db2[6] = data->vers.ver2_l11.auxiliary_runmode;
			ven_db2[7] = data->vers.ver2_l11.auxiliary_runversion;
			ven_db2[8] = data->vers.ver2_l11.auxiliary_debug0;
			ven_db2[9] = (data->vers.ver2_l11.content_type)
				| (data->vers.ver2_l11.content_sub_type << 4);
			ven_db2[10] = (data->vers.ver2_l11.intended_white_point)
				| (data->vers.ver2_l11.crf << 4);
			ven_db2[11] = data->vers.ver2_l11.l11_byte2;
			ven_db2[12] = data->vers.ver2_l11.l11_byte3;
		} else {
			ven_db2[3] = (data->vers.ver2.low_latency) |
				(data->vers.ver2.dobly_vision_signal << 1) |
				(data->vers.ver2.src_dm_version << 5);
			ven_db2[4] = (data->vers.ver2.eff_tmax_PQ_hi)
				| (data->vers.ver2.auxiliary_MD_present << 6)
				| (data->vers.ver2.backlt_ctrl_MD_present << 7);
			ven_db2[5] = data->vers.ver2.eff_tmax_PQ_low;
			ven_db2[6] = data->vers.ver2.auxiliary_runmode;
			ven_db2[7] = data->vers.ver2.auxiliary_runversion;
			ven_db2[8] = data->vers.ver2.auxiliary_debug0;
		}
		if (type == EOTF_T_DV_AHEAD) {
			hdmi_vend_infoframe_rawset(ven_hb, db2);
			spin_unlock_irqrestore(&hdev->tx_comm.edid_spinlock, flags);
			return;
		}
		/*Dolby Vision standard case*/
		if (type == EOTF_T_DOLBYVISION) {
			/* disable forced gaming in this mode because if we are
			 * here with forced gaming on, it means TV is not DV LL
			 * capable
			 */
			if (hdev->ll_user_set_mode == HDMI_LL_MODE_ENABLE &&
			   (hdev->tx_comm.allm_mode == 1 || hdev->tx_comm.ct_mode == 1)) {
				HDMITX_INFO("Dolby VSIF, disable forced game mode\n");
				hdmi_tx_enable_ll_mode(false);
			}
			/*first disable drm package*/
			hdmi_drm_infoframe_set(NULL);
			hdmi_vend_infoframe_rawset(ven_hb, db2);
			/* Dolby Vision Source System-on-Chip Platform Kit Version 2.6:
			 * 4.4.1 Expected AVI-IF for Dolby Vision output, need BT2020 for DV
			 */
			hdmi_avi_infoframe_config(CONF_AVI_BT2020, SET_AVI_BT2020);/*BT.2020*/
			if (tunnel_mode == RGB_8BIT) {/*RGB444*/
				hdmi_avi_infoframe_config(CONF_AVI_CS, HDMI_COLORSPACE_RGB);
				hdmi_avi_infoframe_config(CONF_AVI_Q01, RGB_RANGE_FUL);
				hdmitx_tracer_write_event(hdev->tx_comm.tx_tracer,
							HDMITX_HDR_MODE_DV_STD);
			} else {/*YUV422*/
				hdmi_avi_infoframe_config(CONF_AVI_CS, HDMI_COLORSPACE_YUV422);
				hdmi_avi_infoframe_config(CONF_AVI_YQ01, YCC_RANGE_FUL);
				hdmitx_tracer_write_event(hdev->tx_comm.tx_tracer,
							HDMITX_HDR_MODE_DV_LL);
			}
		}
		/*Dolby Vision low-latency case*/
		else if  (type == EOTF_T_LL_MODE) {
			/* make sure forced game mode is enabled as there could be DV std
			 * to DV LL transition during uboot to kernel transition because
			 * of game mode forced enabled by user.
			 */
			if (hdev->ll_user_set_mode == HDMI_LL_MODE_ENABLE &&
			    hdev->tx_comm.allm_mode == 0 && hdev->tx_comm.ct_mode == 0) {
				pr_debug("hdmitx: Dolby LL VSIF, enable forced game mode\n");
				hdmi_tx_enable_ll_mode(true);
			}
			/*first disable drm package*/
			hdmi_drm_infoframe_set(NULL);
			hdmi_vend_infoframe_rawset(ven_hb, db2);
			/* Dolby vision HDMI Signaling Case25,
			 * UCD323 not declare bt2020 colorimetry,
			 * need to forcely send BT.2020
			 */
			hdmi_avi_infoframe_config(CONF_AVI_BT2020, SET_AVI_BT2020);
			if (tunnel_mode == RGB_10_12BIT) {/*10/12bit RGB444*/
				hdmi_avi_infoframe_config(CONF_AVI_CS, HDMI_COLORSPACE_RGB);
				hdmi_avi_infoframe_config(CONF_AVI_Q01, RGB_RANGE_LIM);
			} else if (tunnel_mode == YUV444_10_12BIT) {
				/*10/12bit YUV444*/
				hdmi_avi_infoframe_config(CONF_AVI_CS, HDMI_COLORSPACE_YUV444);
				hdmi_avi_infoframe_config(CONF_AVI_YQ01, YCC_RANGE_LIM);
			} else {/*YUV422*/
				hdmi_avi_infoframe_config(CONF_AVI_CS, HDMI_COLORSPACE_YUV422);
				hdmi_avi_infoframe_config(CONF_AVI_YQ01, YCC_RANGE_LIM);
			}
			hdmitx_tracer_write_event(hdev->tx_comm.tx_tracer,
						HDMITX_HDR_MODE_DV_LL);
		} else { /*SDR case*/
			HDMITX_INFO("Dolby VSIF, ven_db2[3]) = %d\n", ven_db2[3]);
			hdmi_vend_infoframe_rawset(ven_hb, db2);
			if (signal_sdr) {
				HDMITX_INFO("Dolby VSIF, switching signal to SDR\n");
				HDMITX_INFO("vic:%d, cd:%d, cs:%d, cr:%d\n",
					fmt_para->timing.vic, fmt_para->cd,
					fmt_para->cs, fmt_para->cr);
				hdmi_avi_infoframe_config(CONF_AVI_CS, fmt_para->cs);
				hdmi_avi_infoframe_config(CONF_AVI_Q01, RGB_RANGE_DEFAULT);
				hdmi_avi_infoframe_config(CONF_AVI_YQ01, YCC_RANGE_LIM);
				hdmi_avi_infoframe_config(CONF_AVI_BT2020, CLR_AVI_BT2020);/*BT709*/
				hdmitx_tracer_write_event(hdev->tx_comm.tx_tracer,
							HDMITX_HDR_MODE_SDR);
				/* re-enable forced game mode if selected by the user */
				if (hdev->ll_user_set_mode == HDMI_LL_MODE_ENABLE) {
					HDMITX_INFO("DV VSIF disabled,re-enable force game mode\n");
					hdmi_tx_enable_ll_mode(true);
				}
			}
		}
	}
	hdmitx21_dither_config(hdev);
	spin_unlock_irqrestore(&hdev->tx_comm.edid_spinlock, flags);
}

static void hdmitx_set_hdr10plus_pkt(u32 flag,
	struct hdr10plus_para *data)
{
	struct hdmitx_dev *hdev = get_hdmitx21_device();
	u8 ven_hb[3] = {0x81, 0x01, 0x1b};
	u8 db[28] = {0x00};
	u8 *ven_db = &db[1];

	HDMITX_DEBUG_PACKET("%s[%d]\n", __func__, __LINE__);
	if (hdev->bist_lock)
		return;
	/* if ready is 0, only can clear pkt */
	if (hdev->tx_comm.ready == 0 && data)
		return;
	if (flag == HDR10_PLUS_ZERO_VSIF) {
		/* needed during hdr10+ to sdr transition */
		HDMITX_INFO("%s: zero vsif\n", __func__);
		hdmi_vend_infoframe_rawset(ven_hb, db);
		hdmi_avi_infoframe_config(CONF_AVI_BT2020, CLR_AVI_BT2020);
		hdev->hdr10plus_feature = 0;
		hdr_status_pos = 4;
		/* When hdr10plus mode ends, clear hdr10plus_event flag */
		hdmitx_tracer_clean_hdr10plus_event(hdev->tx_comm.tx_tracer,
					HDMITX_HDR_MODE_HDR10PLUS);
		hdmitx_tracer_write_event(hdev->tx_comm.tx_tracer,
					HDMITX_HDR_MODE_SDR);
		return;
	}

	if (!data || !flag) {
		HDMITX_INFO("%s: null vsif\n", __func__);
		/* hdmi_vend_infoframe_set(NULL); */
		hdmi_vend_infoframe_rawset(NULL, NULL);
		hdmi_avi_infoframe_config(CONF_AVI_BT2020, CLR_AVI_BT2020);
		hdev->hdr10plus_feature = 0;
		/* When hdr10plus mode ends, clear hdr10plus_event flag */
		hdmitx_tracer_clean_hdr10plus_event(hdev->tx_comm.tx_tracer,
					HDMITX_HDR_MODE_HDR10PLUS);
		return;
	}

	if (hdev->hdr10plus_feature != 1)
		HDMITX_INFO("%s: flag = %d\n", __func__, flag);
	hdev->hdr10plus_feature = 1;
	hdr_status_pos = 3;
	ven_db[0] = 0x8b;
	ven_db[1] = 0x84;
	ven_db[2] = 0x90;

	ven_db[3] = ((data->application_version & 0x3) << 6) |
		 ((data->targeted_max_lum & 0x1f) << 1);
	ven_db[4] = data->average_maxrgb;
	ven_db[5] = data->distribution_values[0];
	ven_db[6] = data->distribution_values[1];
	ven_db[7] = data->distribution_values[2];
	ven_db[8] = data->distribution_values[3];
	ven_db[9] = data->distribution_values[4];
	ven_db[10] = data->distribution_values[5];
	ven_db[11] = data->distribution_values[6];
	ven_db[12] = data->distribution_values[7];
	ven_db[13] = data->distribution_values[8];
	ven_db[14] = ((data->num_bezier_curve_anchors & 0xf) << 4) |
		((data->knee_point_x >> 6) & 0xf);
	ven_db[15] = ((data->knee_point_x & 0x3f) << 2) |
		((data->knee_point_y >> 8) & 0x3);
	ven_db[16] = data->knee_point_y  & 0xff;
	ven_db[17] = data->bezier_curve_anchors[0];
	ven_db[18] = data->bezier_curve_anchors[1];
	ven_db[19] = data->bezier_curve_anchors[2];
	ven_db[20] = data->bezier_curve_anchors[3];
	ven_db[21] = data->bezier_curve_anchors[4];
	ven_db[22] = data->bezier_curve_anchors[5];
	ven_db[23] = data->bezier_curve_anchors[6];
	ven_db[24] = data->bezier_curve_anchors[7];
	ven_db[25] = data->bezier_curve_anchors[8];
	ven_db[26] = ((data->graphics_overlay_flag & 0x1) << 7) |
		((data->no_delay_flag & 0x1) << 6);

	hdmi_vend_infoframe_rawset(ven_hb, db);
	hdmi_avi_infoframe_config(CONF_AVI_BT2020, SET_AVI_BT2020);
	hdmitx_tracer_write_event(hdev->tx_comm.tx_tracer,
				HDMITX_HDR_MODE_HDR10PLUS);
}

static void hdmitx_set_cuva_hdr_vsif(struct cuva_hdr_vsif_para *data)
{
	unsigned long flags = 0;
	struct hdmitx_dev *hdev = get_hdmitx21_device();
	unsigned char ven_hb[3] = {0x81, 0x01, 0x1b};
	unsigned char db[28] = {0x00};
	unsigned char *ven_db = &db[1];

	spin_lock_irqsave(&hdev->tx_comm.edid_spinlock, flags);
	if (!data) {
		hdmi_vend_infoframe_rawset(NULL, NULL);
		spin_unlock_irqrestore(&hdev->tx_comm.edid_spinlock, flags);
		return;
	}
	ven_db[0] = GET_OUI_BYTE0(CUVA_IEEEOUI);
	ven_db[1] = GET_OUI_BYTE1(CUVA_IEEEOUI);
	ven_db[2] = GET_OUI_BYTE2(CUVA_IEEEOUI);
	ven_db[3] = data->system_start_code;
	ven_db[4] = (data->version_code & 0xf) << 4;
	hdmi_vend_infoframe_rawset(ven_hb, db);
	spin_unlock_irqrestore(&hdev->tx_comm.edid_spinlock, flags);
	hdmitx_tracer_write_event(hdev->tx_comm.tx_tracer,
				HDMITX_HDR_MODE_CUVA);
}

static void hdmitx_set_cuva_hdr_vs_emds(struct cuva_hdr_vs_emds_para *data)
{
	struct hdmi_packet_t vs_emds[3];
	unsigned long flags;
	struct hdmitx_dev *hdev = get_hdmitx21_device();
	int max_size;

	memset(vs_emds, 0, sizeof(vs_emds));
	spin_lock_irqsave(&hdev->tx_comm.edid_spinlock, flags);
	if (!data) {
		hdmitx_dhdr_send(NULL, 0);
		spin_unlock_irqrestore(&hdev->tx_comm.edid_spinlock, flags);
		return;
	}

	hdr_status_pos = 4;
	max_size = sizeof(struct hdmi_packet_t) * 3;
	vs_emds[0].hb[0] = 0x7f;
	vs_emds[0].hb[1] = 1 << 7;
	vs_emds[0].hb[2] = 0; /* Sequence_Index */
	vs_emds[0].pb[0] = (1 << 7) | (1 << 4) | (1 << 2) | (1 << 1);
	vs_emds[0].pb[1] = 0; /* rsvd */
	vs_emds[0].pb[2] = 0; /* Organization_ID */
	vs_emds[0].pb[3] = 0; /* Data_Set_Tag_MSB */
	vs_emds[0].pb[4] = 2; /* Data_Set_Tag_LSB */
	vs_emds[0].pb[5] = 0; /* Data_Set_Length_MSB */
	vs_emds[0].pb[6] = 0x38; /* Data_Set_Length_LSB */
	vs_emds[0].pb[7] = GET_OUI_BYTE0(CUVA_IEEEOUI);
	vs_emds[0].pb[8] = GET_OUI_BYTE1(CUVA_IEEEOUI);
	vs_emds[0].pb[9] = GET_OUI_BYTE2(CUVA_IEEEOUI);
	vs_emds[0].pb[10] = data->system_start_code;
	vs_emds[0].pb[11] = ((data->version_code & 0xf) << 4) |
			     ((data->min_maxrgb_pq >> 8) & 0xf);
	vs_emds[0].pb[12] = data->min_maxrgb_pq & 0xff;
	vs_emds[0].pb[13] = (data->avg_maxrgb_pq >> 8) & 0xf;
	vs_emds[0].pb[14] = data->avg_maxrgb_pq & 0xff;
	vs_emds[0].pb[15] = (data->var_maxrgb_pq >> 8) & 0xf;
	vs_emds[0].pb[16] = data->var_maxrgb_pq & 0xff;
	vs_emds[0].pb[17] = (data->max_maxrgb_pq >> 8) & 0xf;
	vs_emds[0].pb[18] = data->max_maxrgb_pq & 0xff;
	vs_emds[0].pb[19] = (data->targeted_max_lum_pq >> 8) & 0xf;
	vs_emds[0].pb[20] = data->targeted_max_lum_pq & 0xff;
	vs_emds[0].pb[21] = ((data->transfer_character & 1) << 7) |
			     ((data->base_enable_flag & 0x1) << 6) |
			     ((data->base_param_m_p >> 8) & 0x3f);
	vs_emds[0].pb[22] = data->base_param_m_p & 0xff;
	vs_emds[0].pb[23] = data->base_param_m_m & 0x3f;
	vs_emds[0].pb[24] = (data->base_param_m_a >> 8) & 0x3;
	vs_emds[0].pb[25] = data->base_param_m_a & 0xff;
	vs_emds[0].pb[26] = (data->base_param_m_b >> 8) & 0x3;
	vs_emds[0].pb[27] = data->base_param_m_b & 0xff;
	vs_emds[1].hb[0] = 0x7f;
	vs_emds[1].hb[1] = 0;
	vs_emds[1].hb[2] = 1; /* Sequence_Index */
	vs_emds[1].pb[0] = data->base_param_m_n & 0x3f;
	vs_emds[1].pb[1] = (((data->base_param_k[0] & 3) << 4) |
			   ((data->base_param_k[1] & 3) << 2) |
			   ((data->base_param_k[2] & 3) << 0));
	vs_emds[1].pb[2] = data->base_param_delta_enable_mode & 0x7;
	vs_emds[1].pb[3] = data->base_param_enable_delta & 0x7f;
	vs_emds[1].pb[4] = (((data->_3spline_enable_num & 0x3) << 3) |
			    ((data->_3spline_enable_flag & 1)  << 2) |
			    (data->_3spline_data[0].th_enable_mode & 0x3));
	vs_emds[1].pb[5] = data->_3spline_data[0].th_enable_mb;
	vs_emds[1].pb[6] = (data->_3spline_data[0].th_enable >> 8) & 0xf;
	vs_emds[1].pb[7] = data->_3spline_data[0].th_enable & 0xff;
	vs_emds[1].pb[8] =
		(data->_3spline_data[0].th_enable_delta[0] >> 8) & 0x3;
	vs_emds[1].pb[9] = data->_3spline_data[0].th_enable_delta[0] & 0xff;
	vs_emds[1].pb[10] =
		(data->_3spline_data[0].th_enable_delta[1] >> 8) & 0x3;
	vs_emds[1].pb[11] = data->_3spline_data[0].th_enable_delta[1] & 0xff;
	vs_emds[1].pb[12] = data->_3spline_data[0].enable_strength;
	vs_emds[1].pb[13] = data->_3spline_data[1].th_enable_mode & 0x3;
	vs_emds[1].pb[14] = data->_3spline_data[1].th_enable_mb;
	vs_emds[1].pb[15] = (data->_3spline_data[1].th_enable >> 8) & 0xf;
	vs_emds[1].pb[16] = data->_3spline_data[1].th_enable & 0xff;
	vs_emds[1].pb[17] =
		(data->_3spline_data[1].th_enable_delta[0] >> 8) & 0x3;
	vs_emds[1].pb[18] = data->_3spline_data[1].th_enable_delta[0] & 0xff;
	vs_emds[1].pb[19] =
		(data->_3spline_data[1].th_enable_delta[1] >> 8) & 0x3;
	vs_emds[1].pb[20] = data->_3spline_data[1].th_enable_delta[1] & 0xff;
	vs_emds[1].pb[21] = data->_3spline_data[1].enable_strength;
	vs_emds[1].pb[22] = data->color_saturation_num;
	vs_emds[1].pb[23] = data->color_saturation_gain[0];
	vs_emds[1].pb[24] = data->color_saturation_gain[1];
	vs_emds[1].pb[25] = data->color_saturation_gain[2];
	vs_emds[1].pb[26] = data->color_saturation_gain[3];
	vs_emds[1].pb[27] = data->color_saturation_gain[4];
	vs_emds[2].hb[0] = 0x7f;
	vs_emds[2].hb[1] = (1 << 6);
	vs_emds[2].hb[2] = 2; /* Sequence_Index */
	vs_emds[2].pb[0] = data->color_saturation_gain[5];
	vs_emds[2].pb[1] = data->color_saturation_gain[6];
	vs_emds[2].pb[2] = data->color_saturation_gain[7];
	vs_emds[2].pb[3] = data->graphic_src_display_value;
	vs_emds[2].pb[4] = 0; /* Reserved */
	vs_emds[2].pb[5] = data->max_display_mastering_lum >> 8;
	vs_emds[2].pb[6] = data->max_display_mastering_lum & 0xff;

	hdmitx_dhdr_send((u8 *)&vs_emds, max_size);
	spin_unlock_irqrestore(&hdev->tx_comm.edid_spinlock, flags);
	hdmitx_tracer_write_event(hdev->tx_comm.tx_tracer,
				HDMITX_HDR_MODE_CUVA);
}

//SBTM PKT test
void hdmitx21_send_sbtm_pkt(void)
{
	u8 hb[3] = {0x0};
	u8 pb[28] = {0x0};

	hb[0] = 0x7f; //header[7:0] packet type; EMP packet = 0x7f
	hb[1] = 0xc0; //header[15:8]; [7]:first; [6]:last
	hb[2] = 0x00; //sequence_index;

	pb[0] = 0x96; //[7]:new; [6]:end; [5:4]:DS_type; [3]:AFR; [2]:VFR;
	pb[1] = 0x00; //reserved
	pb[2] = 0x01; //Organization_ID
	pb[3] = 0x00; //data_set_tag(msb)
	pb[4] = 0x03; //data_set_tag(lsb)
	pb[5] = 0x00; //data_set_length(msb) when ID>0, then length = 0
	pb[6] = 0x00; //data_set_length(lsb)

	pb[7] = 0x11;
	pb[8] = 0x12;
	pb[9] = 0x13;
	pb[10] = 0x14;
	pb[11] = 0x15;
	pb[12] = 0x16;
	pb[13] = 0x17;
	pb[14] = 0x18;
	pb[15] = 0x19;
	pb[16] = 0x1a;
	pb[17] = 0x1b;
	pb[18] = 0x1c;
	pb[19] = 0x1d;
	pb[20] = 0x1e;
	pb[21] = 0x1f;
	pb[22] = 0x20;
	pb[23] = 0x21;
	pb[24] = 0x22;
	pb[25] = 0x23;
	pb[26] = 0x24;
	pb[27] = 0x25;

	hdmi_sbtm_infoframe_rawset(hb, pb);
}

/* reserved,  left blank here, move to hdmi_tx_vrr.c file */
static void hdmitx_set_emp_pkt(u8 *data, u32 type, u32 size)
{
}

static ssize_t config_store(struct device *dev,
			    struct device_attribute *attr,
			    const char *buf, size_t count)
{
	int ret = 0;
	struct master_display_info_s data = {0};
	struct hdr10plus_para hdr_data = {0x1, 0x2, 0x3};
	struct cuva_hdr_vs_emds_para cuva_data = {0x1, 0x2, 0x3};
	unsigned char pb[28] = {0x46, 0xD0, 0x00, 0x00, 0x00, 0x00, 0x46, 0xD0,
	0x00, 0x10, 0x21, 0xaa, 0x9b, 0x96, 0x19, 0xfc, 0x19, 0x75, 0xd5, 0x78,
	0x10, 0x21, 0xaa, 0x9b, 0x96, 0x19, 0xfc, 0x19};
	unsigned char hb[3] = {0x01, 0x02, 0x03};
	struct dv_vsif_para vsif_para = {0};
	/* unsigned int mute_us = 0; */
	/* unsigned int mute_frames = 0; */
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	HDMITX_INFO("config: %s\n", buf);

	if (strncmp(buf, "info", 4) == 0) {
		HDMITX_INFO("%x %x %x %x %x %x\n",
			hdmitx_hw_get_hdr_st(&hdev->tx_hw.base),
			hdmitx_hw_get_dv_st(&hdev->tx_hw.base),
			hdmitx_hw_get_hdr10p_st(&hdev->tx_hw.base),
			hdmitx21_hdr_en(),
			hdmitx21_dv_en(),
			hdmitx21_hdr10p_en()
			);
	} else if (strncmp(buf, "3d", 2) == 0) {
		/* Second, set 3D parameters */
		if (strncmp(buf + 2, "tb", 2) == 0) {
			hdev->flag_3dtb = 1;
			hdev->flag_3dss = 0;
			hdev->flag_3dfp = 0;
			hdmi21_set_3d(hdev, T3D_TAB, 0);
		} else if ((strncmp(buf + 2, "lr", 2) == 0) ||
			(strncmp(buf + 2, "ss", 2) == 0)) {
			unsigned long sub_sample_mode = 0;

			hdev->flag_3dtb = 0;
			hdev->flag_3dss = 1;
			hdev->flag_3dfp = 0;
			if (buf[2])
				ret = kstrtoul(buf + 2, 10,
					       &sub_sample_mode);
			/* side by side */
			hdmi21_set_3d(hdev, T3D_SBS_HALF,
				    sub_sample_mode);
		} else if (strncmp(buf + 2, "fp", 2) == 0) {
			hdev->flag_3dtb = 0;
			hdev->flag_3dss = 0;
			hdev->flag_3dfp = 1;
			hdmi21_set_3d(hdev, T3D_FRAME_PACKING, 0);
		} else if (strncmp(buf + 2, "off", 3) == 0) {
			hdev->flag_3dfp = 0;
			hdev->flag_3dtb = 0;
			hdev->flag_3dss = 0;
			hdmi21_set_3d(hdev, T3D_DISABLE, 0);
		}
	} else if (strncmp(buf, "sdr_hdr_dov", 11) == 0) {
		/* firstly stay at SDR state, then send hdr->dv packet to
		 * emulate SDR->HDR->DV switch, DRM-TX-47
		 */
		/* step1: SDR-->HDR */
		data.features = 0x00091000;
		hdmitx_set_drm_pkt(&data);
		/* mute_us = mute_frames * hdmitx_get_frame_duration(); */
		/* usleep_range(mute_us, mute_us + 10); */
		/* step2: HDR->DV_LL */
		vsif_para.ver = 0x1;
		vsif_para.length = 0x1b;
		vsif_para.ver2_l11_flag = 0;
		vsif_para.vers.ver2.low_latency = 1;
		vsif_para.vers.ver2.dobly_vision_signal = 1;
		hdmitx_set_vsif_pkt(4, 0, &vsif_para, false);
	} else if (strncmp(buf, "sdr", 3) == 0) {
		data.features = 0x00010100;
		hdmitx_set_drm_pkt(&data);
	} else if (strncmp(buf, "hdr", 3) == 0) {
		data.features = 0x00091000;
		hdmitx_set_drm_pkt(&data);
	} else if (strncmp(buf, "sbtm", 4) == 0) {
		struct vtem_sbtm_st sbtm = {
			.sbtm_ver = 0x2,
			.sbtm_mode = 0x3,
			.sbtm_type = 0x1,
			.grdm_min = 0x1,
			.grdm_lum = 2,
			.frmpblimitint = 0xdcba, /* MD2/3 */
		};
		hdmitx_set_sbtm_pkt(&sbtm);
	} else if (strncmp(buf, "hlg", 3) == 0) {
		data.features = 0x00091200;
		hdmitx_set_drm_pkt(&data);
	} else if (strncmp(buf, "vsif", 4) == 0) {
		if (buf[4] == '1' && buf[5] == '1') {
			/* DV STD */
			vsif_para.ver = 0x1;
			vsif_para.length = 0x1b;
			vsif_para.ver2_l11_flag = 0;
			vsif_para.vers.ver2.low_latency = 0;
			vsif_para.vers.ver2.dobly_vision_signal = 1;
			hdmitx_set_vsif_pkt(1, 1, &vsif_para, false);
		} else if (buf[4] == '1' && buf[5] == '0') {
			/* DV STD packet, but dolby_vision_signal bit cleared */
			vsif_para.ver = 0x1;
			vsif_para.length = 0x1b;
			vsif_para.ver2_l11_flag = 0;
			vsif_para.vers.ver2.low_latency = 0;
			vsif_para.vers.ver2.dobly_vision_signal = 0;
			hdmitx_set_vsif_pkt(1, 1, &vsif_para, false);
		} else if (buf[4] == '4' && buf[5] == '1') {
			/* DV LL */
			vsif_para.ver = 0x1;
			vsif_para.length = 0x1b;
			vsif_para.ver2_l11_flag = 0;
			vsif_para.vers.ver2.low_latency = 1;
			vsif_para.vers.ver2.dobly_vision_signal = 1;
			hdmitx_set_vsif_pkt(4, 0, &vsif_para, false);
		}  else if (buf[4] == '4' && buf[5] == '0') {
			/* DV LL packet, but dolby_vision_signal bit cleared */
			vsif_para.ver = 0x1;
			vsif_para.length = 0x1b;
			vsif_para.ver2_l11_flag = 0;
			vsif_para.vers.ver2.low_latency = 1;
			vsif_para.vers.ver2.dobly_vision_signal = 0;
			hdmitx_set_vsif_pkt(4, 0, &vsif_para, false);
		} else if (buf[4] == '0') {
			/* exit DV to SDR */
			hdmitx_set_vsif_pkt(0, 0, NULL, true);
		}
	} else if (strncmp(buf, "emp", 3) == 0) {
		hdmitx_set_emp_pkt(NULL, 1, 1);
	} else if (strncmp(buf, "hdr10+", 6) == 0) {
		hdmitx_set_hdr10plus_pkt(1, &hdr_data);
	} else if (strncmp(buf, "cuva", 4) == 0) {
		hdmitx_set_cuva_hdr_vs_emds(&cuva_data);
	} else if (strncmp(buf, "w_dhdr", 6) == 0) {
		hdmitx21_write_dhdr_sram();
	} else if (strncmp(buf, "r_dhdr", 6) == 0) {
		hdmitx21_read_dhdr_sram();
	} else if (strncmp(buf, "t_avi", 5) == 0) {
		hdmi_avi_infoframe_rawset(hb, pb);
	} else if (strncmp(buf, "t_audio", 7) == 0) {
		hdmi_audio_infoframe_rawset(hb, pb);
	} else if (strncmp(buf, "t_sbtm", 6) == 0) {
		hdmitx21_send_sbtm_pkt();
	}
	return count;
}

static void hdmitx21_ext_set_audio_output(bool enable)
{
	hdmitx21_audio_mute_op(enable, AUDIO_MUTE_PATH_1);
	HDMITX_INFO("%s enable:%d\n", __func__, enable);
}

static int hdmitx21_ext_get_audio_status(void)
{
	struct hdmitx_dev *hdev = get_hdmitx21_device();

	return !!hdev->tx_comm.cur_audio_param.aud_output_en;
}

static ssize_t aud_mute_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	pos += snprintf(buf + pos, PAGE_SIZE, "%d\n",
		atomic_read(&hdev->kref_audio_mute));
	return pos;
}

static ssize_t aud_mute_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);
	atomic_t kref_audio_mute = hdev->kref_audio_mute;

	if (buf[0] == '1') {
		atomic_inc(&kref_audio_mute);
		hdmitx21_audio_mute_op(0, AUDIO_MUTE_PATH_2);
	}
	if (buf[0] == '0') {
		if (!(atomic_sub_and_test(0, &kref_audio_mute)))
			atomic_dec(&kref_audio_mute);
		hdmitx21_audio_mute_op(1, AUDIO_MUTE_PATH_2);
	}
	hdev->kref_audio_mute = kref_audio_mute;

	return count;
}

static ssize_t vid_mute_show(struct device *dev,
			     struct device_attribute *attr,
			     char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	pos += snprintf(buf + pos, PAGE_SIZE, "%d\n",
		atomic_read(&hdev->kref_video_mute));
	return pos;
}

static ssize_t vid_mute_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);
	atomic_t kref_video_mute = hdev->kref_video_mute;

	if (buf[0] == '1') {
		atomic_inc(&kref_video_mute);
		hdmitx21_video_mute_op(0, VIDEO_MUTE_PATH_1);
	}
	if (buf[0] == '0') {
		if (!(atomic_sub_and_test(0, &kref_video_mute))) {
			atomic_dec(&kref_video_mute);
		}
		hdmitx21_video_mute_op(1, VIDEO_MUTE_PATH_1);
	}

	hdev->kref_video_mute = kref_video_mute;

	return count;
}

/**/
static ssize_t lipsync_cap_show(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);
	struct rx_cap *prxcap = &hdev->tx_comm.rxcap;

	pos += snprintf(buf + pos, PAGE_SIZE, "Lipsync(in ms)\n");
	pos += snprintf(buf + pos, PAGE_SIZE, "%d, %d\n",
				prxcap->vLatency, prxcap->aLatency);
	return pos;
}

/**/
static ssize_t hdmi_hdr_status_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	int pos = 0;
	enum hdmi_tf_type type = HDMI_NONE;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	/* pos = 4 */
	if (hdr_status_pos == 4) {
		pos += snprintf(buf + pos, PAGE_SIZE, "HDR10-GAMMA_CUVA");
		return pos;
	}

	/* pos = 3 */
	if (hdr_status_pos == 3 || hdev->hdr10plus_feature) {
		pos += snprintf(buf + pos, PAGE_SIZE, "HDR10Plus-VSIF");
		return pos;
	}

	/* pos = 2 */
	if (hdr_status_pos == 2) {
		if (hdev->hdmi_current_eotf_type == EOTF_T_DOLBYVISION) {
			pos += snprintf(buf + pos, PAGE_SIZE,
				"DolbyVision-Std");
			return pos;
		}
		if (hdev->hdmi_current_eotf_type == EOTF_T_LL_MODE) {
			pos += snprintf(buf + pos, PAGE_SIZE,
				"DolbyVision-Lowlatency");
			return pos;
		}
	}

	/* pos = 1 */
	if (hdr_status_pos == 1) {
		if (hdev->hdr_transfer_feature == T_SMPTE_ST_2084) {
			if (hdev->hdr_color_feature == C_BT2020) {
				pos += snprintf(buf + pos, PAGE_SIZE,
					"HDR10-GAMMA_ST2084");
				return pos;
			}
			pos += snprintf(buf + pos, PAGE_SIZE, "HDR10-others");
			return pos;
		}
		if (hdev->hdr_color_feature == C_BT2020 &&
		    (hdev->hdr_transfer_feature == T_BT2020_10 ||
		     hdev->hdr_transfer_feature == T_HLG)) {
			pos += snprintf(buf + pos, PAGE_SIZE,
				"HDR10-GAMMA_HLG");
			return pos;
		}
	}

	type = hdmitx_hw_get_state(&hdev->tx_hw.base, STAT_TX_HDR10P, 0);
	if (type) {
		if (type == HDMI_HDR10P_DV_VSIF) {
			pos += snprintf(buf + pos, PAGE_SIZE, "HDR10Plus-VSIF");
			return pos;
		}
	}
	type = hdmitx_hw_get_state(&hdev->tx_hw.base, STAT_TX_DV, 0);
	if (type) {
		if (type == HDMI_DV_VSIF_STD) {
			pos += snprintf(buf + pos, PAGE_SIZE, "DolbyVision-Std");
			return pos;
		} else if (type == HDMI_DV_VSIF_LL) {
			pos += snprintf(buf + pos, PAGE_SIZE, "DolbyVision-Lowlatency");
			return pos;
		}
	}
	type = hdmitx_hw_get_state(&hdev->tx_hw.base, STAT_TX_HDR, 0);
	if (type) {
		if (type == HDMI_HDR_SMPTE_2084) {
			pos += snprintf(buf + pos, PAGE_SIZE, "HDR10-GAMMA_ST2084");
			return pos;
		} else if (type == HDMI_HDR_HLG) {
			pos += snprintf(buf + pos, PAGE_SIZE, "HDR10-GAMMA_HLG");
			return pos;
		} else if (type == HDMI_HDR_HDR) {
			pos += snprintf(buf + pos, PAGE_SIZE, "HDR10-others");
			return pos;
		}
	}
	/* default is SDR */
	pos += snprintf(buf + pos, PAGE_SIZE, "SDR");

	return pos;
}

static int hdmi_hdr_status_to_drm(void)
{
	struct hdmitx_dev *hdev = get_hdmitx21_device();

	/* pos = 3 */
	if (hdr_status_pos == 3 || hdev->hdr10plus_feature)
		return HDR10PLUS_VSIF;

	/* pos = 2 */
	if (hdr_status_pos == 2) {
		if (hdev->hdmi_current_eotf_type == EOTF_T_DOLBYVISION)
			return dolbyvision_std;

		if (hdev->hdmi_current_eotf_type == EOTF_T_LL_MODE)
			return dolbyvision_lowlatency;
	}

	/* pos = 1 */
	if (hdr_status_pos == 1) {
		if (hdev->hdr_transfer_feature == T_SMPTE_ST_2084) {
			if (hdev->hdr_color_feature == C_BT2020)
				return HDR10_GAMMA_ST2084;
			else
				return HDR10_others;
		}
		if (hdev->hdr_color_feature == C_BT2020 &&
		    (hdev->hdr_transfer_feature == T_BT2020_10 ||
		     hdev->hdr_transfer_feature == T_HLG))
			return HDR10_GAMMA_HLG;
	}

	/* default is SDR */
	return SDR;
}

static inline int com_str(const char *buf, const char *str)
{
	return strncmp(buf, str, strlen(str)) == 0;
}

static void hdmi_tx_enable_ll_mode(bool enable)
{
	struct hdmitx_dev *hdev = get_hdmitx21_device();
	struct hdmitx_common *tx_comm = &hdev->tx_comm;

	if (enable) {
		if (tx_comm->rxcap.allm) {
			/* DV CTS case89 requirement: if IFDB with no
			 * additional VSIF support in EDID, then only
			 * send DV-VSIF, not send HF-VSIF
			 */
			if (hdmitx21_dv_en() &&
				(tx_comm->rxcap.ifdb_present &&
					tx_comm->rxcap.additional_vsif_num < 1)) {
				HDMITX_INFO("%s: can't send HF-VSIF, ifdb: %d, vsif_num: %d\n",
					__func__, tx_comm->rxcap.ifdb_present,
					tx_comm->rxcap.additional_vsif_num);
				return;
			}
			tx_comm->allm_mode = 1;
			HDMITX_INFO("%s: enabling ALLM, enable:%d, allm:%d, cnc3:%d\n",
				__func__, enable, tx_comm->rxcap.allm, tx_comm->rxcap.cnc3);
			hdmitx_common_setup_vsif_packet(tx_comm, VT_ALLM, 1, NULL);
			tx_comm->ct_mode = 0;
			hdev->it_content = 0;
			hdmitx_hw_cntl_config(&hdev->tx_hw.base, CONF_CT_MODE, SET_CT_OFF);
		} else if (tx_comm->rxcap.cnc3) {
		    /* disable ALLM first if enabled*/
			if (tx_comm->allm_mode == 1) {
				tx_comm->allm_mode = 0;
				HDMITX_INFO("%s: first dis-ALLM, enable:%d, allm:%d, cnc3:%d\n",
					__func__, enable, tx_comm->rxcap.allm, tx_comm->rxcap.cnc3);
				hdmitx_common_setup_vsif_packet(tx_comm, VT_ALLM, 0, NULL);
				if (hdmitx_edid_get_hdmi14_4k_vic(tx_comm->fmt_para.vic) > 0)
					hdmitx_common_setup_vsif_packet(tx_comm,
						VT_HDMI14_4K, 1, NULL);
				/* if not hdmi1.4 4k, need to sent > 4 frames and shorter than 1S
				 * HF-VSIF with allm_mode = 0, and then disable HF-VSIF according
				 * 10.2.1 HF-VSIF Transitions in hdmi2.1a. TODO:
				 */
			}
			tx_comm->ct_mode = 1;
			hdev->it_content = 1;
			HDMITX_INFO("%s: enabling GAME Mode, enable:%d, allm:%d, cnc3:%d\n",
				__func__, enable, tx_comm->rxcap.allm, tx_comm->rxcap.cnc3);
			hdmitx_hw_cntl_config(&hdev->tx_hw.base, CONF_CT_MODE,
				SET_CT_GAME | IT_CONTENT << 4);
		} else {
			/* for safety, clear ALLM/HDMI1.X GAME if enabled */
			/* disable ALLM */
			if (tx_comm->allm_mode == 1) {
				tx_comm->allm_mode = 0;
				HDMITX_INFO("%s: disabling ALLM, enable:%d, allm:%d, cnc3:%d\n",
					__func__, enable, tx_comm->rxcap.allm, tx_comm->rxcap.cnc3);
				hdmitx_common_setup_vsif_packet(tx_comm,
					VT_ALLM, 0, NULL);
				if (hdmitx_edid_get_hdmi14_4k_vic(tx_comm->fmt_para.vic) > 0 &&
					!hdmitx21_dv_en() &&
					!hdmitx21_hdr10p_en())
					hdmitx_common_setup_vsif_packet(tx_comm,
						VT_HDMI14_4K, 1, NULL);
			}
			/* clear content type */
			if (tx_comm->ct_mode == 1) {
				tx_comm->ct_mode = 0;
				hdev->it_content = 0;
				HDMITX_INFO("%s:disabling GAME Mode, enable:%d, allm:%d, cnc3:%d\n",
					__func__, enable, tx_comm->rxcap.allm, tx_comm->rxcap.cnc3);
				hdmitx_hw_cntl_config(&hdev->tx_hw.base, CONF_CT_MODE, SET_CT_OFF);
			}
		}
	} else {
		/* disable ALLM */
		if (tx_comm->allm_mode == 1) {
			tx_comm->allm_mode = 0;
			HDMITX_INFO("%s: disabling ALLM, enable:%d, allm:%d, cnc3:%d\n",
				__func__, enable, tx_comm->rxcap.allm, tx_comm->rxcap.cnc3);
			hdmitx_common_setup_vsif_packet(tx_comm, VT_ALLM, 0, NULL);
			if (hdmitx_edid_get_hdmi14_4k_vic(tx_comm->fmt_para.vic) > 0 &&
				!hdmitx21_dv_en() &&
				!hdmitx21_hdr10p_en())
				hdmitx_common_setup_vsif_packet(tx_comm, VT_HDMI14_4K, 1, NULL);
		}
		/* clear content type */
		if (tx_comm->ct_mode == 1) {
			tx_comm->ct_mode = 0;
			hdev->it_content = 0;
			HDMITX_INFO("%s: disabling GAME Mode, enable:%d, allm:%d, cnc3:%d\n",
				__func__, enable, tx_comm->rxcap.allm, tx_comm->rxcap.cnc3);
			hdmitx_hw_cntl_config(&hdev->tx_hw.base, CONF_CT_MODE, SET_CT_OFF);
		}
	}
}

/* for decoder/hwc or sysctl to control the low latency mode,
 * as they don't care if sink support ALLM OR HDMI1.X game mode
 * so need hdmitx driver to device to send ALLM OR HDMI1.X game
 * mode according to capability of EDID
 */
static ssize_t ll_mode_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	if (hdev->tx_comm.rxcap.allm) {
		if (hdev->tx_comm.allm_mode == 1)
			pos += snprintf(buf + pos, PAGE_SIZE, "HDMI2.1_ALLM_ENABLED\n\r");
		else
			pos += snprintf(buf + pos, PAGE_SIZE, "HDMI2.1_ALLM_DISABLED\n\r");
	}
	if (hdev->tx_comm.rxcap.cnc3) {
		if (hdev->tx_comm.ct_mode == 1)
			pos += snprintf(buf + pos, PAGE_SIZE, "HDMI1.x_GAME_MODE_ENABLED\n\r");
		else
			pos += snprintf(buf + pos, PAGE_SIZE, "HDMI1.x_GAME_MODE_DISABLED\n\r");
	}

	if (!hdev->tx_comm.rxcap.allm && !hdev->tx_comm.rxcap.cnc3)
		pos += snprintf(buf + pos, PAGE_SIZE, "HDMI_LATENCY_MODE_UNKNOWN\n\r");
	return pos;
}

/* 1.echo 1 to enable ALLM OR HDMI1.X game mode
 * if sink support ALLM, then output ALLM mode;
 * else if support HDMI1.X game mode, then output
 * HDMI1.X game mode; else, do nothing
 * 2.echo 0 to disable ALLM and HDMI1.X game mode
 */
static ssize_t ll_mode_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf,
			       size_t count)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);
	hdev->ll_enabled_in_auto_mode = com_str(buf, "1");

	HDMITX_INFO("store ll_enabled_in_auto_mode: %d, ll_user_set_mode:%d\n",
		hdev->ll_enabled_in_auto_mode, hdev->ll_user_set_mode);

	if (hdev->ll_user_set_mode == HDMI_LL_MODE_AUTO) {
		HDMITX_INFO("store ll_mode as %s, calling hdmi_tx_enable_ll_mode()\n", buf);
		hdmi_tx_enable_ll_mode(hdev->ll_enabled_in_auto_mode);
	} else {
		HDMITX_INFO("ll mode is forced on/off: %d\n", hdev->ll_user_set_mode);
	}

	return count;
}

/* for user to force enable/disable low-latency modes
 */
static ssize_t ll_user_mode_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	switch (hdev->ll_user_set_mode) {
	case HDMI_LL_MODE_ENABLE:
		pos += snprintf(buf + pos, PAGE_SIZE, "HDMI_LL_MODE_ENABLE\n\r");
		break;
	case HDMI_LL_MODE_DISABLE:
		pos += snprintf(buf + pos, PAGE_SIZE, "HDMI_LL_MODE_DISABLE\n\r");
		break;
	case HDMI_LL_MODE_AUTO:
	default:
		pos += snprintf(buf + pos, PAGE_SIZE, "HDMI_LL_MODE_AUTO\n\r");
		break;
	}
	return pos;
}

/* 1.echo enable to enable ALLM OR HDMI1.X game mode
 * if sink support ALLM, then output ALLM mode;
 * else if support HDMI1.X game mode, then output
 * HDMI1.X game mode; else, do nothing
 * 2.echo disable to disable ALLM and HDMI1.X game mode
 * 3.echo auto to enable/disable low-latency mode per
 * content type
 */
static ssize_t ll_user_mode_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf,
			       size_t count)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	HDMITX_INFO("store ll_user_set_mode as %s\n", buf);
	if (com_str(buf, "enable")) {
		hdev->ll_user_set_mode = HDMI_LL_MODE_ENABLE;
		hdmi_tx_enable_ll_mode(true);
	} else if (com_str(buf, "disable")) {
		hdev->ll_user_set_mode = HDMI_LL_MODE_DISABLE;
		hdmi_tx_enable_ll_mode(false);
	} else {
		hdev->ll_user_set_mode = HDMI_LL_MODE_AUTO;
		hdmi_tx_enable_ll_mode(hdev->ll_enabled_in_auto_mode);
	}
	return count;
}

/* for game console-> hdmirx -> hdmitx -> TV
 * interface for hdmirx module
 * ret: false if not update, true if updated
 */
bool hdmitx_update_latency_info(struct tvin_latency_s *latency_info)
{
	struct hdmitx_dev *hdev = get_hdmitx21_device();
	struct hdmitx_common *tx_comm = &hdev->tx_comm;
	bool it_content = false;
	/* when switch between hdmirx source(ALLM) and hdmitx home(non-ALLM),
	 * the ALLM/1.4 Game will change, need to mute before change
	 */
	bool video_mute = false;

	if (!hdmi_allm_passthough_en)
		return false;
	if (!latency_info)
		return false;

	HDMITX_INFO("%s: ll_enabled_in_auto_mode: %d, ll_user_set_mode:%d\n",
		__func__, hdev->ll_enabled_in_auto_mode, hdev->ll_user_set_mode);

	if (hdev->ll_user_set_mode != HDMI_LL_MODE_AUTO) {
		HDMITX_INFO("%s:non-auto mode,return, allm_mode: %d, it_content: %d, cn_type: %d\n",
			__func__,
			latency_info->allm_mode,
			latency_info->it_content,
			latency_info->cn_type);
		return false;
	}
	HDMITX_INFO("%s: allm_mode: %d, it_content: %d, cn_type: %d\n",
		__func__, latency_info->allm_mode, latency_info->it_content, latency_info->cn_type);
	if (tx_comm->allm_mode == latency_info->allm_mode &&
		hdev->it_content == latency_info->it_content &&
		tx_comm->ct_mode == latency_info->cn_type) {
		HDMITX_INFO("latency_info not changed, exit\n");
		return false;
	}
	/* refer to allm_mode_store() */
	if (latency_info->allm_mode) {
		if (tx_comm->rxcap.allm) {
			//if (hdmitx21_dv_en() &&
				//(hdev->rxcap.ifdb_present &&
				//hdev->rxcap.additional_vsif_num < 1)) {
				//HDMITX_INFO("%s: DV enabled, but ifdb_present: %d,
				//additional_vsif_num: %d\n",
				//__func__, hdev->rxcap.ifdb_present,
				//hdev->rxcap.additional_vsif_num);
				//return false;
			//}
			if (!get_rx_active_sts()) {
				video_mute = true;
				//hdmitx21_video_mute_op(0, VIDEO_MUTE_PATH_4);
				hdmitx_hw_cntl_config(&hdev->tx_hw.base, CONF_VIDEO_MUTE_OP,
					VIDEO_MUTE);
			}
			tx_comm->allm_mode = 1;
			HDMITX_INFO("%s: enabling ALLM\n", __func__);
			hdmitx_common_setup_vsif_packet(tx_comm, VT_ALLM, 1, NULL);
			tx_comm->ct_mode = 0;
			hdev->it_content = 0;
			hdmitx_hw_cntl_config(&hdev->tx_hw.base, CONF_CT_MODE, SET_CT_OFF);
		}
	} else {
		if (!get_rx_active_sts()) {
			video_mute = true;
			//hdmitx21_video_mute_op(0, VIDEO_MUTE_PATH_4);
			hdmitx_hw_cntl_config(&hdev->tx_hw.base, CONF_VIDEO_MUTE_OP, VIDEO_MUTE);
		}
		/* disable ALLM firstly */
		if (tx_comm->allm_mode == 1) {
			tx_comm->allm_mode = 0;
			HDMITX_INFO("%s: disabling ALLM before enable/disable game mode\n",
			__func__);
			hdmitx_common_setup_vsif_packet(tx_comm, VT_ALLM, 0, NULL);
			if (hdmitx_edid_get_hdmi14_4k_vic(tx_comm->fmt_para.vic) > 0 &&
				!hdmitx21_dv_en() &&
				!hdmitx21_hdr10p_en())
				hdmitx_common_setup_vsif_packet(tx_comm, VT_HDMI14_4K, 1, NULL);
		}
		hdev->it_content = latency_info->it_content;
		it_content = hdev->it_content;
		if (tx_comm->rxcap.cnc3 && latency_info->cn_type == GAME) {
			tx_comm->ct_mode = 1;
			HDMITX_INFO("%s: enabling GAME mode\n", __func__);
			hdmitx_hw_cntl_config(&hdev->tx_hw.base, CONF_CT_MODE,
				SET_CT_GAME | it_content << 4);
		} else if (tx_comm->rxcap.cnc0 && latency_info->cn_type == GRAPHICS &&
		    latency_info->it_content == 1) {
			tx_comm->ct_mode = 2;
			HDMITX_INFO("%s: enabling GRAPHICS mode\n", __func__);
			hdmitx_hw_cntl_config(&hdev->tx_hw.base, CONF_CT_MODE,
				SET_CT_GRAPHICS | it_content << 4);
		} else if (tx_comm->rxcap.cnc1 && latency_info->cn_type == PHOTO) {
			tx_comm->ct_mode = 3;
			HDMITX_INFO("%s: enabling PHOTO mode\n", __func__);
			hdmitx_hw_cntl_config(&hdev->tx_hw.base, CONF_CT_MODE,
				SET_CT_PHOTO | it_content << 4);
		} else if (tx_comm->rxcap.cnc2 && latency_info->cn_type == CINEMA) {
			tx_comm->ct_mode = 4;
			HDMITX_INFO("%s: enabling CINEMA mode\n", __func__);
			hdmitx_hw_cntl_config(&hdev->tx_hw.base, CONF_CT_MODE,
				SET_CT_CINEMA | it_content << 4);
		} else {
			tx_comm->ct_mode = 0;
			HDMITX_INFO("%s: No GAME or CT mode\n", __func__);
			hdmitx_hw_cntl_config(&hdev->tx_hw.base, CONF_CT_MODE,
				SET_CT_OFF | it_content << 4);
		}
	}
	return true;
}
EXPORT_SYMBOL(hdmitx_update_latency_info);

#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
static ssize_t vrr_cap_show(struct device *dev,
			    struct device_attribute *attr,
			    char *buf)
{
	return _vrr_cap_show(dev, attr, buf);
}

static ssize_t vrr_mode_show(struct device *dev,
			    struct device_attribute *attr,
			    char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	switch (hdev->vrr_mode) {
	case T_VRR_GAME:
		pos += snprintf(buf + pos, PAGE_SIZE, "%s\n", "game-vrr");
		break;
	case T_VRR_QMS:
		pos += snprintf(buf + pos, PAGE_SIZE, "%s\n", "qms-vrr");
		break;
	default:
		pos += snprintf(buf + pos, PAGE_SIZE, "%s\n", "none");
		break;
	}
	return pos;
}

/*
 * vrr_mode: 0: default value, no vrr
 *           1/game-vrr: Game VRR
 *           2/qms-vrr:  Quick Media Switch VRR
 */
static ssize_t vrr_mode_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	int val = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	pr_info("set vrr_mode as %s\n", buf);
	if (isdigit(buf[0])) {
		val = buf[0] - '0';
		if (val == 0 || val == 1 || val == 2)
			hdev->vrr_mode = val;
		else
			pr_info("only accept as 0, 1 or 2\n");

		return count;
	}
	if (strncmp(buf, "game-vrr", 8) == 0)
		hdev->vrr_mode = T_VRR_GAME;
	else if (strncmp(buf, "qms-vrr", 7) == 0)
		hdev->vrr_mode = T_VRR_QMS;
	else
		hdev->vrr_mode = T_VRR_NONE;

	return count;
}
#endif

static ssize_t rxsense_policy_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	int val = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	if (isdigit(buf[0])) {
		val = buf[0] - '0';
		HDMITX_INFO("set rxsense_policy as %d\n", val);
		if (val == 0 || val == 1)
			hdev->tx_comm.rxsense_policy = val;
		else
			HDMITX_INFO("only accept as 0 or 1\n");
	}
	if (hdev->tx_comm.rxsense_policy)
		queue_delayed_work(hdev->tx_comm.rxsense_wq,
				   &hdev->tx_comm.work_rxsense, 0);
	else
		cancel_delayed_work(&hdev->tx_comm.work_rxsense);

	return count;
}

static ssize_t rxsense_policy_show(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	pos += snprintf(buf + pos, PAGE_SIZE, "%d\n",
		hdev->tx_comm.rxsense_policy);

	return pos;
}

/* cedst_policy: 0, no CED feature
 *	       1, auto mode, depends on RX scdc_present
 *	       2, forced CED feature
 */
static ssize_t cedst_policy_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf,
				  size_t count)
{
	int val = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	if (isdigit(buf[0])) {
		val = buf[0] - '0';
		HDMITX_INFO("set cedst_policy as %d\n", val);
		if (val == 0 || val == 1 || val == 2) {
			hdev->tx_comm.cedst_policy = val;
			if (val == 1) { /* Auto mode, depends on Rx */
				/* check RX scdc_present */
				if (hdev->tx_comm.rxcap.scdc_present)
					hdev->tx_comm.cedst_en = 1;
				else
					hdev->tx_comm.cedst_en = 0;
			} else if (val == 2) {
				hdev->tx_comm.cedst_en = 1;
			}
		} else {
			HDMITX_INFO("only accept as 0, 1(auto), or 2(force)\n");
		}
	}
	if (hdev->tx_comm.cedst_en)
		queue_delayed_work(hdev->tx_comm.cedst_wq, &hdev->tx_comm.work_cedst, 0);
	else
		cancel_delayed_work(&hdev->tx_comm.work_cedst);

	return count;
}

static ssize_t cedst_policy_show(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	pos += snprintf(buf + pos, PAGE_SIZE, "%d\n",
		hdev->tx_comm.cedst_policy);

	return pos;
}

static ssize_t cedst_count_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);
	struct ced_cnt *ced = &hdev->ced_cnt;
	struct scdc_locked_st *ch_st = &hdev->chlocked_st;

	if (!ch_st->clock_detected)
		pos += snprintf(buf + pos, PAGE_SIZE, "clock undetected\n");
	if (!ch_st->ch0_locked)
		pos += snprintf(buf + pos, PAGE_SIZE, "CH0 unlocked\n");
	if (!ch_st->ch1_locked)
		pos += snprintf(buf + pos, PAGE_SIZE, "CH1 unlocked\n");
	if (!ch_st->ch2_locked)
		pos += snprintf(buf + pos, PAGE_SIZE, "CH2 unlocked\n");
	if (ced->ch0_valid && ced->ch0_cnt)
		pos += snprintf(buf + pos, PAGE_SIZE, "CH0 ErrCnt 0x%x\n",
			ced->ch0_cnt);
	if (ced->ch1_valid && ced->ch1_cnt)
		pos += snprintf(buf + pos, PAGE_SIZE, "CH1 ErrCnt 0x%x\n",
			ced->ch1_cnt);
	if (ced->ch2_valid && ced->ch2_cnt)
		pos += snprintf(buf + pos, PAGE_SIZE, "CH2 ErrCnt 0x%x\n",
			ced->ch2_cnt);
	memset(ced, 0, sizeof(*ced));

	return pos;
}

static ssize_t sspll_store(struct device *dev,
			   struct device_attribute *attr,
			   const char *buf,
			   size_t count)
{
	int val = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	if (isdigit(buf[0])) {
		val = buf[0] - '0';
		HDMITX_INFO("set sspll : %d\n", val);
		if (val == 0 || val == 1)
			hdev->sspll = val;
		else
			HDMITX_INFO("sspll only accept as 0 or 1\n");
	}

	return count;
}

static ssize_t sspll_show(struct device *dev,
			  struct device_attribute *attr,
			  char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	pos += snprintf(buf + pos, PAGE_SIZE, "%d\n",
		hdev->sspll);

	return pos;
}

static ssize_t hdcp_type_policy_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t hdcp_type_policy_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int pos = 0;

	return pos;
}

static ssize_t hdcp_lstore_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);
	int lstore = hdev->lstore;

	if (lstore < 0x10) {
		lstore = 0;
		if (get_hdcp2_lstore())
			lstore |= BIT(1);
		if (get_hdcp1_lstore())
			lstore |= BIT(0);
	}
	if ((lstore & 0x3) == 0x3) {
		pos += snprintf(buf + pos, PAGE_SIZE, "22+14\n");
	} else {
		if (lstore & 0x1)
			pos += snprintf(buf + pos, PAGE_SIZE, "14\n");
		if (lstore & 0x2)
			pos += snprintf(buf + pos, PAGE_SIZE, "22\n");
	}
	return pos;
}

static ssize_t hdcp_lstore_store(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	/* debug usage for key store check
	 * echo value > hdcp_lstore. value can be
	 * -1: automatically check stored key when enable hdcp
	 * 0: same as no hdcp key stored
	 * 11: only hdcp1.x key stored
	 * 12: only hdcp2.x key stored
	 * 13: both hdcp1.x and hdcp2.x key stored
	 */
	HDMITX_INFO("hdcp: set lstore as %s\n", buf);
	if (strncmp(buf, "-1", 2) == 0)
		hdev->lstore = 0x0;
	if (strncmp(buf, "0", 1) == 0 ||
		strncmp(buf, "10", 2) == 0)
		hdev->lstore = 0x10;
	if (strncmp(buf, "11", 2) == 0)
		hdev->lstore = 0x11;
	if (strncmp(buf, "12", 2) == 0)
		hdev->lstore = 0x12;
	if (strncmp(buf, "13", 2) == 0)
		hdev->lstore = 0x13;

	return count;
}

static ssize_t hdcp_mode_show(struct device *dev,
			      struct device_attribute *attr,
			      char *buf)
{
	int pos = 0;
	unsigned int hdcp_ret = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);
	u32 hdcp_mode = hdmitx21_get_hdcp_mode();

	switch (hdcp_mode) {
	case 1:
		pos += snprintf(buf + pos, PAGE_SIZE, "14");
		break;
	case 2:
		pos += snprintf(buf + pos, PAGE_SIZE, "22");
		break;
	default:
		pos += snprintf(buf + pos, PAGE_SIZE, "off");
		break;
	}

	if (hdev->tx_comm.hdcp_ctl_lvl > 0 && hdcp_mode > 0) {
		if (hdcp_mode == 1)
			hdcp_ret = get_hdcp1_result();
		else if (hdcp_mode == 2)
			hdcp_ret = get_hdcp2_result();
		else
			hdcp_ret = 0;
		if (hdcp_ret == 1)
			pos += snprintf(buf + pos, PAGE_SIZE, ": succeed\n");
		else
			pos += snprintf(buf + pos, PAGE_SIZE, ": fail\n");
	}

	return pos;
}

/* note: below store is just for debug, no mutex in it */
static ssize_t hdcp_mode_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t count)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	if (strncmp(buf, "f1", 2) == 0) {
		if (hdev->tx_comm.efuse_dis_hdcp_tx14) {
			HDMITX_ERROR("warning, efuse disable hdcptx14\n");
			return count;
		}
		hdev->tx_comm.hdcp_mode = 0x1;
		hdcp_mode_set(1);
	}
	if (strncmp(buf, "f2", 2) == 0) {
		if (hdev->tx_comm.efuse_dis_hdcp_tx22) {
			HDMITX_ERROR("warning, efuse disable hdcptx22\n");
			return count;
		}
		hdev->tx_comm.hdcp_mode = 0x2;
		hdcp_mode_set(2);
	}
	if (buf[0] == '0') {
		hdev->tx_comm.hdcp_mode = 0x00;
		hdcp_mode_set(0);
	}

	return count;
}

static ssize_t def_stream_type_show(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	pos += snprintf(buf + pos, PAGE_SIZE, "%d\n",
		hdev->def_stream_type);

	return pos;
}

static ssize_t def_stream_type_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf,
				      size_t count)
{
	u8 val = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	if (isdigit(buf[0])) {
		val = buf[0] - '0';
		HDMITX_INFO("set def_stream_type as %d\n", val);
		if (val == 0 || val == 1)
			hdev->def_stream_type = val;
		else
			HDMITX_INFO("only accept as 0 or 1\n");
	}

	return count;
}

static ssize_t propagate_stream_type_show(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);
	struct hdcp_t *p_hdcp = (struct hdcp_t *)hdev->am_hdcp;

	if (p_hdcp->ds_repeater && p_hdcp->hdcp_type == HDCP_VER_HDCP2X) {
		pos += snprintf(buf + pos, PAGE_SIZE, "%d\n",
			p_hdcp->csm_message.streamid_type & 0xFF);
	} else {
		HDMITX_INFO("no stream type, as ds_repeater: %d, hdcp_type: %d\n",
			p_hdcp->ds_repeater, p_hdcp->hdcp_type);
	}
	return pos;
}

static ssize_t cont_smng_method_show(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);
	struct hdcp_t *p_hdcp = (struct hdcp_t *)hdev->am_hdcp;

	pos += snprintf(buf + pos, PAGE_SIZE, "%d\n",
		p_hdcp->cont_smng_method);

	return pos;
}

/* content stream management update method:
 * when upstream side received new content stream
 * management message, there're two method to
 * update content stream type that propagated
 * to downstream hdcp2.3 repeater:
 * 0(default): only send content stream management
 * message with new stream type to downstream
 * 1: init new re-auth with downstream
 */
static ssize_t cont_smng_method_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf,
				      size_t count)
{
	u8 val = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);
	struct hdcp_t *p_hdcp = (struct hdcp_t *)hdev->am_hdcp;

	if (isdigit(buf[0])) {
		val = buf[0] - '0';
		HDMITX_INFO("set cont_smng_method as %d\n", val);
		if (val == 0 || val == 1)
			p_hdcp->cont_smng_method = val;
		else
			HDMITX_INFO("only accept as 0 or 1\n");
	}

	return count;
}

static ssize_t is_passthrough_switch_show(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	int pos = 0;

	pos += snprintf(buf + pos, PAGE_SIZE, "%d\n", is_passthrough_switch);

	return pos;
}

static ssize_t is_passthrough_switch_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf,
				      size_t count)
{
	u8 val = 0;

	if (isdigit(buf[0])) {
		val = buf[0] - '0';
		HDMITX_INFO("set is_passthrough_switch as %d\n", val);
		if (val == 0 || val == 1)
			is_passthrough_switch = val;
		else
			HDMITX_INFO("only accept as 0 or 1\n");
	}

	return count;
}

/* is hdcp cts test equipment */
static ssize_t is_hdcp_cts_te_show(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	pos += snprintf(buf + pos, PAGE_SIZE, "%d\n",
		hdmitx_edid_only_support_sd(&hdev->tx_comm.rxcap));

	return pos;
}

static ssize_t frl_rate_show(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	pos += snprintf(buf + pos, PAGE_SIZE, "%d\n", hdev->frl_rate);
	switch (hdev->frl_rate) {
	case FRL_3G3L:
		pos += snprintf(buf + pos, PAGE_SIZE, "FRL_3G3L\n");
		break;
	case FRL_6G3L:
		pos += snprintf(buf + pos, PAGE_SIZE, "FRL_6G3L\n");
		break;
	case FRL_6G4L:
		pos += snprintf(buf + pos, PAGE_SIZE, "FRL_6G4L\n");
		break;
	case FRL_8G4L:
		pos += snprintf(buf + pos, PAGE_SIZE, "FRL_8G4L\n");
		break;
	case FRL_10G4L:
		pos += snprintf(buf + pos, PAGE_SIZE, "FRL_10G4L\n");
		break;
	case FRL_12G4L:
		pos += snprintf(buf + pos, PAGE_SIZE, "FRL_12G4L\n");
		break;
	default:
		break;
	}

	return pos;
}

static ssize_t frl_rate_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf,
				      size_t count)
{
	u8 val = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	/* if rx don't support FRL, return */
	if (!hdev->tx_comm.rxcap.max_frl_rate)
		HDMITX_INFO("rx not support FRL\n");

	/* forced FRL rate setting */
	if (buf[0] == 'f' && isdigit(buf[1])) {
		val = buf[1] - '0';
		if (val > FRL_12G4L) {
			HDMITX_INFO("set frl_rate in 0 ~ 6\n");
			return count;
		}
		hdev->manual_frl_rate = val;
		HDMITX_INFO("set tx frl_rate as %d\n", val);
	}
	if (hdev->manual_frl_rate > hdev->tx_comm.rxcap.max_frl_rate)
		HDMITX_INFO("larger than rx max_frl_rate %d\n",
				hdev->tx_comm.rxcap.max_frl_rate);

	return count;
}

static ssize_t dsc_en_show(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	pos += snprintf(buf + pos, PAGE_SIZE, "%d\n", hdev->dsc_en);

	return pos;
}

static ssize_t dsc_en_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf,
				      size_t count)
{
	u8 val = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	if (isdigit(buf[0])) {
		val = buf[0] - '0';
		if (val != 0 && val != 1) {
			HDMITX_INFO("set dsc_en in 0 ~ 1\n");
			return count;
		}
		hdev->dsc_en = val;
		HDMITX_INFO("set dsc_en as %d\n", val);
	}

	return count;
}

static ssize_t clkmsr_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	return _show21_clkmsr(buf);
}

static ssize_t hdcp_ver_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);
	struct hdmitx_common *tx_comm = &hdev->tx_comm;

	if (!tx_comm->hpd_state) {
		HDMITX_INFO("%s: hpd low, just return 14\n", __func__);
		pos += snprintf(buf + pos, PAGE_SIZE, "14\n\r");
		return pos;
	}
	if (rx_hdcp2_ver) {
		pos += snprintf(buf + pos, PAGE_SIZE, "22\n\r");
	} else {
		//on hotplug case
		/* note that, when do hdcp repeater1.4 CTS,
		 * hdcp port access will affect item 3a-02 Irregular
		 * procedure: (First part of authentication) HDCP port
		 * access. Refer to hdcp1.4 cts spec: "If DUT does
		 * not read an HDCP register past 4 seconds after
		 * the previous attempt, then FAIL". after read hdcp
		 * version soon after plugin(access failed as TE
		 * not ack), our hdmitx side should keep retrying
		 * in 4S. but source TE start hdcp auth with
		 * hdmirx side too late(more than 4S), as hdcp auth
		 * of hdmitx side is started by hdmirx side, it will
		 * time out the access of hdcp port of 4 second.
		 * so for repeater CTS, should not read hdcp version
		 * whenever you want. Here add protect to only read
		 * hdcp version when currently not in hdmirx channel.
		 * special customer want to read downstream hdcp version
		 * after hotplug, and only sen 4K output when EDID
		 * support 4K && support hdcp2.2. so add is_4k_sink()
		 * decision, it won't affect hdcp repeater CTS.
		 */
		if (hdcp_need_control_by_upstream(hdev)) {
			HDMITX_INFO("%s: currently should not read hdcp version\n", __func__);
		} else if (hdmitx21_get_hdcp_mode() == 0) {
			if (get_hdcp2_lstore() && is_rx_hdcp2ver()) {
				pos += snprintf(buf + pos, PAGE_SIZE, "22\n\r");
				rx_hdcp2_ver = 1;
			}
			HDMITX_INFO("%s: hdev->tx_comm.hdcp_mode: 0, rx_hdcp2_ver = %d\n",
				__func__, rx_hdcp2_ver);
		}
	}
	/* Here, must assume RX support HDCP14, otherwise affect 1A-03 */
	pos += snprintf(buf + pos, PAGE_SIZE, "14\n\r");

	return pos;
}

static ssize_t rxsense_state_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	int pos = 0;
	int sense;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	sense = hdmitx_hw_cntl_misc(&hdev->tx_hw.base, MISC_TMDS_RXSENSE, 0);

	pos += snprintf(buf + pos, PAGE_SIZE, "%d", sense);
	return pos;
}

static ssize_t ready_show(struct device *dev,
			  struct device_attribute *attr, char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	pos += snprintf(buf + pos, PAGE_SIZE, "%d\r\n",
		hdev->tx_comm.ready);
	return pos;
}

static ssize_t ready_store(struct device *dev,
			   struct device_attribute *attr,
			   const char *buf, size_t count)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	if (strncmp(buf, "0", 1) == 0)
		hdev->tx_comm.ready = 0;
	if (strncmp(buf, "1", 1) == 0)
		hdev->tx_comm.ready = 1;
	return count;
}

static ssize_t aon_output_show(struct device *dev,
			  struct device_attribute *attr, char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	pos += snprintf(buf + pos, PAGE_SIZE, "%d\r\n",
		hdev->aon_output);
	return pos;
}

static ssize_t aon_output_store(struct device *dev,
			   struct device_attribute *attr,
			   const char *buf, size_t count)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	if (strncmp(buf, "0", 1) == 0)
		hdev->aon_output = 0;
	if (strncmp(buf, "1", 1) == 0)
		hdev->aon_output = 1;
	return count;
}

static ssize_t hdr_priority_mode_show(struct device *dev,
			  struct device_attribute *attr, char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	pos += snprintf(buf + pos, PAGE_SIZE, "%d\r\n",
		hdev->tx_comm.hdr_priority);

	return pos;
}

/* hide or enable HDR capabilities.
 * 0 : No HDR capabilities are hidden
 * 1 : DV Capabilities are hidden
 * 2 : All HDR capabilities are hidden
 */
static ssize_t hdr_priority_mode_store(struct device *dev,
			   struct device_attribute *attr,
			   const char *buf, size_t count)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);
	struct hdmitx_common *tx_comm = &hdev->tx_comm;
	unsigned int val = 0;
	struct vinfo_s *info = NULL;

	HDMITX_INFO("%s[%d] buf:%s hdr_priority:0x%x\n", __func__, __LINE__, buf,
		hdev->tx_comm.hdr_priority);
	if ((strncmp("0", buf, 1) == 0) || (strncmp("1", buf, 1) == 0) ||
	    (strncmp("2", buf, 1) == 0)) {
		val = buf[0] - '0';
	}

	if (val == tx_comm->hdr_priority)
		return count;
	info = hdmitx_get_current_vinfo(NULL);
	if (!info)
		return count;
	mutex_lock(&hdev->tx_comm.hdmimode_mutex);
	tx_comm->hdr_priority = val;
	/* hdmitx21_event_notify(HDMITX_HDR_PRIORITY, &hdev->hdr_priority); */
	/* force trigger plugin event
	 * hdmitx21_set_uevent_state(HDMITX_HPD_EVENT, 0);
	 * hdmitx21_set_uevent(HDMITX_HPD_EVENT, 1);
	 */
	mutex_unlock(&hdev->tx_comm.hdmimode_mutex);
	return count;
}

/* hdcp fail event method 1: add hdcp fail uevent filter
 * below need_filter_hdcp_off and filter_hdcp_off_period
 * sysfs node are for this filter
 */
static ssize_t need_filter_hdcp_off_show(struct device *dev,
			  struct device_attribute *attr, char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	pos += snprintf(buf + pos, PAGE_SIZE, "%d\r\n",
		hdev->need_filter_hdcp_off);

	return pos;
}

/* if need to filter hdcp fail uevent, systemcontrol
 * write 1 to this node. for example:
 * when start player game movie, it switch from DV STD to DV LL,
 * before switch mode, write 1 to this node. it means that it
 * won't send hdcp fail uevent if hdcp fail but retry auth
 * pass during filter_hdcp_off_period seconds.
 * note: need_filter_hdcp_off is self cleared after filter
 * period expired
 */
static ssize_t need_filter_hdcp_off_store(struct device *dev,
			   struct device_attribute *attr,
			   const char *buf, size_t count)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);
	unsigned int val = 0;

	if ((strncmp("0", buf, 1) == 0) || (strncmp("1", buf, 1) == 0))
		val = buf[0] - '0';

	hdev->need_filter_hdcp_off = val;
	HDMITX_INFO("set need_filter_hdcp_off: %d\n", val);

	return count;
}

/* if hdcp fail but retry auth pass during this period(unit: second),
 * then won't sent hdcp fail uevent. the default is 6 second
 */
static ssize_t filter_hdcp_off_period_show(struct device *dev,
			  struct device_attribute *attr, char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	pos += snprintf(buf + pos, PAGE_SIZE, "%d\r\n",
		hdev->filter_hdcp_off_period);

	return pos;
}

/* example: write 5 to filter 5 second */
static ssize_t filter_hdcp_off_period_store(struct device *dev,
			   struct device_attribute *attr,
			   const char *buf, size_t count)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);
	unsigned long filter_second = 0;

	HDMITX_INFO("set hdcp fail filter_second: %s\n", buf);
	if (kstrtoul(buf, 10, &filter_second) == 0)
		hdev->filter_hdcp_off_period = filter_second;

	return count;
}

/* hdcp fail event method 2: don't stop-restart hdcp auth */
/* when start play game movie, it will switch from
 * DV STD to DV LL, hdcp will stop and restart after
 * mode setting done.
 * now provide an option: when DV STD <-> DV LL
 * switch caused by game movie, systemcontrol write 1
 * to not_restart_hdcp node before do mode switch, it
 * means that this colorspace(mode) switch will only
 * switch mode, but won't stop->restart hdcp action to
 * prevent hdcp fail uevent sent to app.
 * note: 1.not sure if it will always work on different TV.
 * 2.after mode switch done, not_restart_hdcp will be self cleared
 */
static ssize_t not_restart_hdcp_show(struct device *dev,
			  struct device_attribute *attr, char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	pos += snprintf(buf + pos, PAGE_SIZE, "%d\r\n",
		hdev->not_restart_hdcp);

	return pos;
}

static ssize_t not_restart_hdcp_store(struct device *dev,
			   struct device_attribute *attr,
			   const char *buf, size_t count)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);
	unsigned int val = 0;

	if ((strncmp("0", buf, 1) == 0) || (strncmp("1", buf, 1) == 0))
		val = buf[0] - '0';

	hdev->not_restart_hdcp = val;
	HDMITX_INFO("set not_restart_hdcp: %d\n", val);

	return count;
}

static DEVICE_ATTR_RW(disp_mode);
static DEVICE_ATTR_RW(vid_mute);
static DEVICE_ATTR_WO(config);
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
static DEVICE_ATTR_RO(vrr_cap);
static DEVICE_ATTR_RW(vrr_mode);
#endif
static DEVICE_ATTR_RW(aud_mute);
static DEVICE_ATTR_RO(lipsync_cap);
static DEVICE_ATTR_RO(hdmi_hdr_status);
static DEVICE_ATTR_RW(ll_mode);
static DEVICE_ATTR_RW(ll_user_mode);
static DEVICE_ATTR_RW(sspll);
static DEVICE_ATTR_RW(rxsense_policy);
static DEVICE_ATTR_RW(cedst_policy);
static DEVICE_ATTR_RO(cedst_count);
static DEVICE_ATTR_RW(hdcp_mode);
static DEVICE_ATTR_RO(hdcp_ver);
static DEVICE_ATTR_RW(hdcp_type_policy);
static DEVICE_ATTR_RW(hdcp_lstore);
static DEVICE_ATTR_RO(rxsense_state);
static DEVICE_ATTR_RW(ready);
static DEVICE_ATTR_RW(def_stream_type);
static DEVICE_ATTR_RW(aon_output);
static DEVICE_ATTR_RO(propagate_stream_type);
static DEVICE_ATTR_RW(cont_smng_method);
static DEVICE_ATTR_RW(hdr_priority_mode);
static DEVICE_ATTR_RW(is_passthrough_switch);
static DEVICE_ATTR_RO(is_hdcp_cts_te);
static DEVICE_ATTR_RW(need_filter_hdcp_off);
static DEVICE_ATTR_RW(filter_hdcp_off_period);
static DEVICE_ATTR_RW(not_restart_hdcp);
static DEVICE_ATTR_RW(frl_rate);
static DEVICE_ATTR_RW(dsc_en);
static DEVICE_ATTR_RO(clkmsr);

static int hdmitx21_pre_enable_mode(struct hdmitx_common *tx_comm, struct hdmi_format_para *para)
{
	struct hdmitx_dev *hdev = to_hdmitx21_dev(tx_comm);

	if (tx_comm->rxcap.max_frl_rate) {
		hdev->frl_rate = hdmitx_select_frl_rate(hdev->dsc_en, para->vic,
			para->cs, para->cd);
		if (hdev->frl_rate > tx_comm->tx_hw->txcap.tx_max_frl_rate)
			HDMITX_INFO("Current frl_rate %d is larger than tx_max_frl_rate %d\n",
				hdev->frl_rate, tx_comm->tx_hw->txcap.tx_max_frl_rate);
	}
	/* if manual_frl_rate is true, set to force frl_rate */
	if (hdev->manual_frl_rate)
		hdev->frl_rate = hdev->manual_frl_rate;

	return 0;
}

static int hdmitx21_enable_mode(struct hdmitx_common *tx_comm, struct hdmi_format_para *para)
{
	int ret;
	struct hdmitx_dev *hdev = to_hdmitx21_dev(tx_comm);

	ret = hdmitx21_set_display(hdev, para->vic);

	if (ret >= 0)
		restore_mute();
	hdmitx21_set_audio(hdev, &hdev->tx_comm.cur_audio_param);

	return 0;
}

static int hdmitx21_post_enable_mode(struct hdmitx_common *tx_comm, struct hdmi_format_para *para)
{
	struct hdmitx_dev *hdev = to_hdmitx21_dev(tx_comm);
	struct vinfo_s *vinfo = NULL;

	/* wait for TV detect signal stable,
	 * otherwise hdcp may easily auth fail
	 */
	if (hdev->not_restart_hdcp) {
		/* self clear */
		hdev->not_restart_hdcp = 0;
		HDMITX_INFO("special mode switch, not start hdcp\n");
	} else {
		schedule_delayed_work(&hdev->work_start_hdcp, HZ / 4);
	}

	vinfo = get_current_vinfo();
	if (vinfo) {
		vinfo->cur_enc_ppc = 1;
		if (hdmitx_hw_cntl_misc(&hdev->tx_hw.base, MISC_IS_FRL_MODE, 0))
			vinfo->cur_enc_ppc = 4;
		HDMITX_INFO("vinfo: set cur_enc_ppc as %d\n", vinfo->cur_enc_ppc);
	}

	return 0;
}

static int hdmitx21_disable_mode(struct hdmitx_common *tx_comm, struct hdmi_format_para *para)
{
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
	struct hdmitx_dev *hdev = get_hdmitx21_device();

	hdmitx_unregister_vrr(hdev);
#endif
	return 0;
}

static int hdmitx21_init_uboot_mode(enum vmode_e mode)
{
	struct vinfo_s *vinfo = NULL;
	struct hdmitx_dev *hdev = get_hdmitx21_device();
	struct hdmitx_common *tx_comm = &hdev->tx_comm;

#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
	hdmitx_register_vrr(hdev);
#endif
	if (!(mode & VMODE_INIT_BIT_MASK)) {
		HDMITX_ERROR("warning, echo /sys/class/display/mode is disabled\n");
	} else {
		HDMITX_INFO("already display in uboot\n");
		mutex_lock(&hdev->tx_comm.hdmimode_mutex);
		hdev->tx_comm.ready = 1;
		/* if hdmitx already output under uboot,
		 * here get the current parameters of
		 * output mode, which may be used later
		 */
		if (tx_comm->rxcap.max_frl_rate) {
			hdev->frl_rate = hdmitx_select_frl_rate(hdev->dsc_en,
				tx_comm->fmt_para.vic, hdev->tx_comm.fmt_para.cs,
				hdev->tx_comm.fmt_para.cd);
			if (hdev->frl_rate > tx_comm->tx_hw->txcap.tx_max_frl_rate)
				HDMITX_INFO("Current frl_rate %d larger than tx_max_frl_rate %d\n",
					hdev->frl_rate, tx_comm->tx_hw->txcap.tx_max_frl_rate);
		}
		edidinfo_attach_to_vinfo(&hdev->tx_comm);
		update_vinfo_from_formatpara(&hdev->tx_comm);
		vinfo = get_current_vinfo();
		if (vinfo) {
			vinfo->cur_enc_ppc = 1;
			if (hdmitx_hw_cntl_misc(&hdev->tx_hw.base, MISC_IS_FRL_MODE, 0))
				vinfo->cur_enc_ppc = 4;
			HDMITX_INFO("vinfo: set cur_enc_ppc as %d\n", vinfo->cur_enc_ppc);
		}
		mutex_unlock(&hdev->tx_comm.hdmimode_mutex);
		/* started after output setting done */
		if (hdev->tx_comm.cedst_en) {
			cancel_delayed_work(&hdev->tx_comm.work_cedst);
			queue_delayed_work(hdev->tx_comm.cedst_wq, &hdev->tx_comm.work_cedst, 0);
		}
	}
	return 0;
}

static struct hdmitx_ctrl_ops tx21_ctrl_ops = {
	.pre_enable_mode = hdmitx21_pre_enable_mode,
	.enable_mode = hdmitx21_enable_mode,
	.post_enable_mode = hdmitx21_post_enable_mode,
	.disable_mode = hdmitx21_disable_mode,
	.init_uboot_mode = hdmitx21_init_uboot_mode,
	.reset_hdcp = hdmitx21_reset_hdcp,
	.clear_pkt = hdmitx21_clear_packets,
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
	.disable_21_work = hdmitx21_disable_21_work,
#endif
};

static bool drm_hdmitx_get_vrr_cap(void)
{
	struct hdmitx_dev *hdev = get_hdmitx21_device();

	if (hdev->tx_comm.rxcap.qms) {
		HDMITX_INFO("%s support vrr\n", __func__);
		return true;
	}

	HDMITX_INFO("%s not support vrr\n", __func__);
	return false;
}

static bool is_rx_supported_vic(enum hdmi_vic brr_vic)
{
	int i;
	struct hdmitx_dev *hdev = get_hdmitx21_device();
	struct rx_cap *prxcap = &hdev->tx_comm.rxcap;

	for (i = 0; i < prxcap->VIC_count; i++) {
		if (brr_vic == prxcap->VIC[i])
			return 1;
	}

	return 0;
}

static void add_vic_to_group(enum hdmi_vic vic, struct drm_vrr_mode_group *group)
{
	const struct hdmi_timing *timing;

	timing = hdmitx_mode_vic_to_hdmi_timing(vic);
	if (!timing)
		return;
	group->brr_vic = vic;
	group->width = timing->h_active;
	group->height = timing->v_active;
	group->vrr_min = 24; /* fixed value */
	group->vrr_max = timing->v_freq / 1000;
}

static int drm_hdmitx_get_vrr_mode_group(struct drm_vrr_mode_group *group, int max_group)
{
	int i = 0, j = 0;
	const struct hdmi_timing *timing;
	struct hdmitx_dev *hdev = get_hdmitx21_device();
	enum hdmi_vic brr_vic[] = {
		HDMI_16_1920x1080p60_16x9,
		HDMI_63_1920x1080p120_16x9,
		HDMI_4_1280x720p60_16x9,
		HDMI_47_1280x720p120_16x9,
		HDMI_97_3840x2160p60_16x9,
		HDMI_102_4096x2160p60_256x135,
	};

	if (!group || max_group == 0)
		return 0;
	/* check RX VRR capabilities */
	if (!drm_hdmitx_get_vrr_cap())
		return 0;
	for (i = 0, j = 0; i < ARRAY_SIZE(brr_vic); i++) {
		timing = hdmitx_mode_vic_to_hdmi_timing(brr_vic[i]);
		if (!timing)
			continue;
		/* check both TX and RX support current vic */
		if ((hdmitx_common_validate_vic(&hdev->tx_comm, brr_vic[i]) >= 0) &&
			is_rx_supported_vic(brr_vic[i])) {
			add_vic_to_group(brr_vic[i], group + j);
			j++;
		}
	}

	return j;
}

#if IS_ENABLED(CONFIG_AMLOGIC_SND_SOC)

static int hdmitx_notify_callback_a(struct notifier_block *block,
				    unsigned long cmd, void *para);
static struct notifier_block hdmitx_notifier_nb_a = {
	.notifier_call	= hdmitx_notify_callback_a,
};

static int hdmitx_notify_callback_a(struct notifier_block *block,
				    unsigned long cmd, void *para)
{
	struct hdmitx_dev *hdev = get_hdmitx21_device();

	hdmitx_audio_notify_callback(&hdev->tx_comm, &hdev->tx_hw.base, block, cmd, para);
	return 0;
}
#endif

static void hdmitx_rxsense_process(struct work_struct *work)
{
	int sense;
	struct hdmitx_common *tx_comm = container_of((struct delayed_work *)work,
		struct hdmitx_common, work_rxsense);
	struct hdmitx_dev *hdev = container_of(tx_comm, struct hdmitx_dev, tx_comm);

	sense = hdmitx_hw_cntl_misc(&hdev->tx_hw.base, MISC_TMDS_RXSENSE, 0);
	hdmitx21_set_uevent(HDMITX_RXSENSE_EVENT, sense);
	queue_delayed_work(tx_comm->rxsense_wq, &tx_comm->work_rxsense, HZ);
}

static void hdmitx_cedst_process(struct work_struct *work)
{
	int ced;
	struct hdmitx_common *tx_comm = container_of((struct delayed_work *)work,
		struct hdmitx_common, work_cedst);
	struct hdmitx_dev *hdev = to_hdmitx21_dev(tx_comm);

	ced = hdmitx_hw_cntl_misc(&hdev->tx_hw.base, MISC_TMDS_CEDST, 0);
	/* firstly send as 0, then real ced, A trigger signal */
	hdmitx21_set_uevent(HDMITX_CEDST_EVENT, 0);
	hdmitx21_set_uevent(HDMITX_CEDST_EVENT, ced);
	queue_delayed_work(tx_comm->cedst_wq, &tx_comm->work_cedst, HZ);
}

static void hdmitx_process_plugin(struct hdmitx_dev *hdev, bool set_audio)
{
	struct vinfo_s *info = NULL;

	/* step1: SW: EDID read/parse, notify client modules */
	hdmitx_plugin_common_work(&hdev->tx_comm);

	/* TODO: need remove/optimised, keep it temporarily */
	if (set_audio) {
		info = hdmitx_get_current_vinfo(NULL);
		if (info && info->mode == VMODE_HDMI)
			hdmitx21_set_audio(hdev, &hdev->tx_comm.cur_audio_param);
	}

	/* step2: SW: notify client modules and update uevent state */
	hdmitx_common_notify_hpd_status(&hdev->tx_comm, false);
}

/* action which is done in lock, it copy the flow of plugin handler.
 * only set audio if it's already enable in uboot, only check edid
 * if hdmitx output is enabled under uboot.
 * uboot_output_state is indicated in ready flag, can be replaced by
 * HW state later
 */
static void hdmitx_bootup_plugin_handler(struct hdmitx_dev *hdev)
{
	/* if current mode is TMDS/nonFRL, then resend_div40 */
	if (hdmitx_hw_cntl_misc(&hdev->tx_hw.base, MISC_IS_FRL_MODE, 0) == FRL_NONE) {
		if (hdev->tx_comm.fmt_para.tmds_clk_div40)
			hdmitx_hw_cntl_ddc(&hdev->tx_hw.base, DDC_SCDC_DIV40_SCRAMB, 1);
	}
	hdmitx_process_plugin(hdev, hdev->tx_comm.ready);
}

static void hdmitx_hpd_plugin_irq_handler(struct work_struct *work)
{
	/* struct vinfo_s *info = NULL; */
	struct hdmitx_dev *hdev = container_of((struct delayed_work *)work,
		struct hdmitx_dev, work_hpd_plugin);

	mutex_lock(&hdev->tx_comm.hdmimode_mutex);

	/* this may happen when just queue plugin work,
	 * but plugout event happen at this time. no need
	 * to continue plugin work.
	 */
	if (hdmitx_hw_cntl_misc(&hdev->tx_hw.base, MISC_HPD_GPI_ST, 0) == 0) {
		HDMITX_INFO(SYS "plug out event come when plugin handle, abort plugin handle\n");
		mutex_unlock(&hdev->tx_comm.hdmimode_mutex);
		return;
	}
	/* only happen in such case:
	 * hpd high when suspend->plugout->plugin->late resume, the
	 * last plugin/resume flow sequence is unknown, will do
	 * plugin handler only once
	 */
	if (hdev->tx_comm.last_hpd_handle_done_stat == HDMI_TX_HPD_PLUGIN) {
		HDMITX_INFO(SYS "warning: continuous plugin, should not happen!\n");
		mutex_unlock(&hdev->tx_comm.hdmimode_mutex);
		return;
	}
	HDMITX_INFO(SYS "plugin\n");
	hdmitx_process_plugin(hdev, false);

	mutex_unlock(&hdev->tx_comm.hdmimode_mutex);

	/*notify to drm hdmi*/
	hdmitx_fire_drm_hpd_cb_unlocked(&hdev->tx_comm);
}

/* common work for plugout flow, which should be done in lock */
static void hdmitx_process_plugout(struct hdmitx_dev *hdev)
{
	hdmitx_plugout_common_work(&hdev->tx_comm);
	hdmitx_hw_cntl_ddc(&hdev->tx_hw.base, DDC_HDCP_SET_TOPO_INFO, 0);
	/* Reset the ll_enabled_in_auto_mode flag used for auto mode
	 * status. If we are in auto mode, gaming signal should be enabled
	 * when the request arrives again from the input device or playback
	 * and not on hotplug.
	 */
	hdev->ll_enabled_in_auto_mode = false;
	/* SW: notify event to user space and other modules */
	hdmitx_common_notify_hpd_status(&hdev->tx_comm, false);
}

static void hdmitx_bootup_plugout_handler(struct hdmitx_dev *hdev)
{
	hdmitx_process_plugout(hdev);
}

static void hdmitx_hpd_plugout_irq_handler(struct work_struct *work)
{
	struct hdmitx_dev *hdev = container_of((struct delayed_work *)work,
		struct hdmitx_dev, work_hpd_plugout);

	mutex_lock(&hdev->tx_comm.hdmimode_mutex);
	if (hdev->tx_comm.last_hpd_handle_done_stat == HDMI_TX_HPD_PLUGOUT) {
		HDMITX_INFO(SYS "continuous plugout handler, ignore\n");
		mutex_unlock(&hdev->tx_comm.hdmimode_mutex);
		return;
	}
	hdmitx_process_plugout(hdev);
	mutex_unlock(&hdev->tx_comm.hdmimode_mutex);

	/* notify to drm hdmi */
	hdmitx_fire_drm_hpd_cb_unlocked(&hdev->tx_comm);
}

int get21_hpd_state(void)
{
	int ret;
	struct hdmitx_dev *hdev = get_hdmitx21_device();

	mutex_lock(&hdev->tx_comm.hdmimode_mutex);
	ret = hdev->tx_comm.hpd_state;
	mutex_unlock(&hdev->tx_comm.hdmimode_mutex);

	return ret;
}

/*****************************
 *	hdmitx driver file_operations
 *
 ******************************/
static int amhdmitx_open(struct inode *node, struct file *file)
{
	struct hdmitx_dev *hdmitx_in_devp;

	/* Get the per-device structure that contains this cdev */
	hdmitx_in_devp = container_of(node->i_cdev, struct hdmitx_dev, cdev);
	file->private_data = hdmitx_in_devp;

	return 0;
}

static int amhdmitx_release(struct inode *node, struct file *file)
{
	return 0;
}

static const struct file_operations amhdmitx_fops = {
	.owner	= THIS_MODULE,
	.open	 = amhdmitx_open,
	.release = amhdmitx_release,
};

static int get_dt_vend_init_data(struct device_node *np,
				 struct vendor_info_data *vend)
{
	int ret;

	ret = of_property_read_string(np, "vendor_name",
				      (const char **)&vend->vendor_name);
	if (ret)
		HDMITX_INFO("not find vendor name\n");

	ret = of_property_read_u32(np, "vendor_id", &vend->vendor_id);
	if (ret)
		HDMITX_INFO("not find vendor id\n");

	ret = of_property_read_string(np, "product_desc",
				      (const char **)&vend->product_desc);
	if (ret)
		HDMITX_INFO("not find product desc\n");
	return 0;
}

/* for notify to cec */
int hdmitx21_event_notifier_regist(struct notifier_block *nb)
{
	int ret = 0;
	struct hdmitx_dev *hdev = get_hdmitx21_device();

	if (!nb)
		return ret;

	ret = hdmitx_event_mgr_notifier_register(hdev->tx_comm.event_mgr,
		(struct hdmitx_notifier_client *)nb);

	/* update status when register */
	if (!ret && nb->notifier_call) {
		if (hdev->tx_comm.hdmi_repeater == 1)
			hdmitx_notify_hpd_to_rx(hdev->tx_comm.hpd_state,
				  hdev->tx_comm.rxcap.edid_parsing ?
				  hdev->tx_comm.EDID_buf : NULL);
		if (hdev->tx_comm.rxcap.physical_addr != 0xffff) {
			if (hdev->tx_comm.hdmi_repeater == 1)
				hdmitx_event_mgr_notify(hdev->tx_comm.event_mgr,
						HDMITX_PHY_ADDR_VALID,
						&hdev->tx_comm.rxcap.physical_addr);
		}
	}

	return ret;
}

int hdmitx21_event_notifier_unregist(struct notifier_block *nb)
{
	struct hdmitx_dev *hdev = get_hdmitx21_device();

	return hdmitx_event_mgr_notifier_unregister(hdev->tx_comm.event_mgr,
		(struct hdmitx_notifier_client *)nb);
}

void hdmitx21_hdcp_status(int hdmi_authenticated)
{
	hdmitx21_set_uevent(HDMITX_HDCP_EVENT, hdmi_authenticated);
}

static int amhdmitx21_device_init(struct hdmitx_dev *hdev)
{
	if (!hdev)
		return 1;

	HDMITX_INFO("Ver: %s\n", HDMITX_VER);

	hdev->hdtx_dev = NULL;

	hdev->tx_comm.rxcap.physical_addr = 0xffff;
	hdev->hdmi_last_hdr_mode = 0;
	hdev->hdmi_current_hdr_mode = 0;

	/* hdr/vsif packet status init, no need to get actual status,
	 * force to print function callback for confirmation.
	 */
	hdev->hdr_transfer_feature = T_UNKNOWN;
	hdev->hdr_color_feature = C_UNKNOWN;
	hdev->colormetry = 0;
	hdev->hdmi_current_eotf_type = EOTF_T_NULL;
	hdev->hdmi_current_tunnel_mode = 0;
	hdev->hdmi_current_signal_sdr = true;

	hdev->tx_comm.ready = 0;
	hdev->tx_comm.rxsense_policy = 0; /* no RxSense by default */
	/* enable or disable HDMITX SSPLL, enable by default */
	hdev->sspll = 1;
	/*
	 * 0, do not unmux hpd when off or unplug ;
	 * 1, unmux hpd when unplug;
	 * 2, unmux hpd when unplug  or off;
	 */
	hdev->hpdmode = 1;

	hdev->flag_3dfp = 0;
	hdev->flag_3dss = 0;
	hdev->flag_3dtb = 0;
	hdev->def_stream_type = DEFAULT_STREAM_TYPE;

	/* default audio configure is on */
	hdev->tx_comm.cur_audio_param.aud_output_en = 1;
	hdev->need_filter_hdcp_off = false;
	/* default 6S */
	hdev->filter_hdcp_off_period = 6;
	hdev->not_restart_hdcp = false;
	/* wait for upstream start hdcp auth 5S */
	hdev->up_hdcp_timeout_sec = 5;
	hdev->tx_comm.ctrl_ops = &tx21_ctrl_ops;
	hdev->tx_comm.vdev = &hdmitx_vdev;
	set_dummy_dv_info(&hdmitx_vdev);
	/* ll mode init values */
	hdev->ll_enabled_in_auto_mode = false;
	hdev->ll_user_set_mode = HDMI_LL_MODE_AUTO;

	return 0;
}

static int amhdmitx_get_dt_info(struct platform_device *pdev, struct hdmitx_dev *hdev)
{
	int ret = 0;
	struct pinctrl *pin;
	//const char *pin_name;
	//const struct of_device_id *of_id;

#ifdef CONFIG_OF
	int val;
	phandle phandler;
	struct device_node *init_data;
	const struct of_device_id *match;
#endif
	u32 refreshrate_limit = 0;
	struct hdmitx_hw_common *tx_hw_base;

	match = of_match_device(meson_amhdmitx_of_match, &pdev->dev);
	if (!match) {
		HDMITX_INFO("unable to get matched device\n");
		return -1;
	}
	HDMITX_INFO("get matched device\n");
	//hdev->tx_hw.chip_data = match->data;

	/* pinmux set */
	if (pdev->dev.of_node) {
		pin = devm_pinctrl_get(&pdev->dev);
		if (!pin) {
			HDMITX_INFO("get pin control fail\n");
			return -1;
		}

		hdev->pinctrl_default = pinctrl_lookup_state(pin, "hdmitx_hpd");
		pinctrl_select_state(pin, hdev->pinctrl_default);

		hdev->pinctrl_i2c = pinctrl_lookup_state(pin, "hdmitx_ddc");
		pinctrl_select_state(pin, hdev->pinctrl_i2c);
		/* rx_pr("hdmirx: pinmux:%p, name:%s\n", */
		/* pin, pin_name); */
		HDMITX_INFO("get pin control\n");

		/* rx_pr("hdmirx: pinmux:%p, name:%s\n", */
		/* pin, pin_name); */
	} else {
		HDMITX_INFO("node null\n");
	}

	tx_hw_base = &hdev->tx_hw.base;
	tx_hw_base->hdmitx_gpios_hpd = of_get_named_gpio_flags(pdev->dev.of_node,
		"hdmitx-gpios-hpd", 0, NULL);
	if (tx_hw_base->hdmitx_gpios_hpd == -EPROBE_DEFER)
		HDMITX_ERROR("get hdmitx-gpios-hpd error\n");
	tx_hw_base->hdmitx_gpios_scl = of_get_named_gpio_flags(pdev->dev.of_node,
		"hdmitx-gpios-scl", 0, NULL);
	if (tx_hw_base->hdmitx_gpios_scl == -EPROBE_DEFER)
		HDMITX_ERROR("get hdmitx-gpios-scl error\n");
	tx_hw_base->hdmitx_gpios_sda = of_get_named_gpio_flags(pdev->dev.of_node,
		"hdmitx-gpios-sda", 0, NULL);
	if (tx_hw_base->hdmitx_gpios_sda == -EPROBE_DEFER)
		HDMITX_ERROR("get hdmitx-gpios-sda error\n");

#ifdef CONFIG_OF
	if (pdev->dev.of_node) {
		int dongle_mode = 0;
		int pxp_mode = 0;

		memset(&hdev->config_data, 0,
		       sizeof(struct hdmi_config_platform_data));
		/* Get chip type and name information */
		match = of_match_device(meson_amhdmitx_of_match, &pdev->dev);

		if (!match) {
			HDMITX_INFO("%s: no match table\n", __func__);
			return -1;
		}
		hdev->tx_hw.chip_data = (struct amhdmitx_data_s *)match->data;

		HDMITX_INFO("chip_type:%d chip_name:%s\n",
			hdev->tx_hw.chip_data->chip_type,
			hdev->tx_hw.chip_data->chip_name);

		/* Get pxp_mode information */
		ret = of_property_read_u32(pdev->dev.of_node, "pxp_mode",
					   &pxp_mode);
		hdev->pxp_mode = pxp_mode;
		if (!ret)
			HDMITX_INFO("hdev->pxp_mode: %d\n", hdev->pxp_mode);

		/* Get dongle_mode information */
		ret = of_property_read_u32(pdev->dev.of_node, "dongle_mode",
					   &dongle_mode);
		hdev->tx_hw.dongle_mode = !!dongle_mode;
		if (!ret)
			HDMITX_INFO("hdev->dongle_mode: %d\n", hdev->tx_hw.dongle_mode);
		/* Get res_1080p information */
		ret = of_property_read_u32(pdev->dev.of_node, "res_1080p",
			&hdev->tx_comm.res_1080p);

		ret = of_property_read_u32(pdev->dev.of_node, "max_refreshrate",
					   &refreshrate_limit);
		if (ret == 0 && refreshrate_limit > 0)
			hdev->tx_comm.max_refreshrate = refreshrate_limit;

		/* Get repeater_tx information */
		ret = of_property_read_u32(pdev->dev.of_node,
					   "repeater_tx", &val);
		if (!ret)
			hdev->tx_hw.base.hdcp_repeater_en = val;
		else
			hdev->tx_hw.base.hdcp_repeater_en = 0;

		/* Get repeater_tx information */
		ret = of_property_read_u32(pdev->dev.of_node,
					   "hdmi_repeater", &val);
		if (!ret)
			hdev->tx_comm.hdmi_repeater = val;
		else
			hdev->tx_comm.hdmi_repeater = 1;
		/* if it's not hdmi repeater, then should not support hdcp repeater */
		if (hdev->tx_comm.hdmi_repeater == 0)
			hdev->tx_hw.base.hdcp_repeater_en = 0;

		ret = of_property_read_u32(pdev->dev.of_node,
					   "cedst_en", &val);
		if (!ret)
			hdev->tx_comm.cedst_en = !!val;
		hdev->tx_comm.tx_hw->txcap.tx_max_frl_rate = FRL_NONE; /* default */
		ret = of_property_read_u32(pdev->dev.of_node, "tx_max_frl_rate", &val);
		if (!ret) {
			if (val > FRL_12G4L || val == FRL_NONE)
				HDMITX_INFO("wrong tx_max_frl_rate %d\n", val);
			else
				hdev->tx_comm.tx_hw->txcap.tx_max_frl_rate = val;
		}
		ret = of_property_read_u32(pdev->dev.of_node,
					   "hdcp_type_policy", &val);
		if (!ret) {
			if (val == 2)
				;
			if (val == 1)
				;
		}
		ret = of_property_read_u32(pdev->dev.of_node,
					   "enc_idx", &val);
		hdev->tx_comm.enc_idx = 0; /* default 0 */
		if (!ret) {
			if (val == 2)
				hdev->tx_comm.enc_idx = 2;
		}

		/* hdcp ctrl 0:sysctrl, 1: drv, 2: linux app */
		ret = of_property_read_u32(pdev->dev.of_node,
			   "hdcp_ctl_lvl", &hdev->tx_comm.hdcp_ctl_lvl);
		HDMITX_INFO("hdcp_ctl_lvl[%d-%d]\n", hdev->tx_comm.hdcp_ctl_lvl, ret);

		if (ret)
			hdev->tx_comm.hdcp_ctl_lvl = 0;

		/* Get vendor information */
		ret = of_property_read_u32(pdev->dev.of_node,
					   "vend-data", &val);
		if (ret)
			HDMITX_INFO("not find match init-data\n");
		if (ret == 0) {
			phandler = val;
			init_data = of_find_node_by_phandle(phandler);
			if (!init_data)
				HDMITX_INFO("not find device node\n");
			hdev->config_data.vend_data =
			kzalloc(sizeof(struct vendor_info_data), GFP_KERNEL);
			if (!(hdev->config_data.vend_data))
				HDMITX_INFO("not allocate memory\n");
			ret = get_dt_vend_init_data
			(init_data, hdev->config_data.vend_data);
			if (ret)
				HDMITX_INFO("not find vend_init_data\n");
		}
		/* Get power control */
		ret = of_property_read_u32(pdev->dev.of_node,
					   "pwr-ctrl", &val);
		if (ret)
			HDMITX_INFO("not find match pwr-ctl\n");
		if (ret == 0) {
			phandler = val;
			init_data = of_find_node_by_phandle(phandler);
			if (!init_data)
				HDMITX_INFO("not find device node\n");
			hdev->config_data.pwr_ctl = kzalloc((sizeof(struct hdmi_pwr_ctl)) *
				HDMI_TX_PWR_CTRL_NUM, GFP_KERNEL);
			if (!hdev->config_data.pwr_ctl)
				HDMITX_INFO("can not get pwr_ctl mem\n");
			else
				memset(hdev->config_data.pwr_ctl, 0, sizeof(struct hdmi_pwr_ctl));
			if (ret)
				HDMITX_INFO("not find pwr_ctl\n");
		}

		/* Get reg information */
		ret = hdmitx21_init_reg_map(pdev);
		if (ret < 0)
			HDMITX_ERROR("ERROR: hdmitx io_remap fail!\n");
	}

#else
		hdmi_pdata = pdev->dev.platform_data;
		if (!hdmi_pdata) {
			HDMITX_INFO("not get platform data\n");
			r = -ENOENT;
		} else {
			HDMITX_INFO("get hdmi platform data\n");
		}
#endif
	hdev->irq_hpd = platform_get_irq_byname(pdev, "hdmitx_hpd");
	if (hdev->irq_hpd == -ENXIO) {
		HDMITX_ERROR("%s: ERROR: hdmitx hpd irq No not found\n",
		       __func__);
			return -ENXIO;
	}
	HDMITX_INFO("hpd irq = %d\n", hdev->irq_hpd);
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
	tx_vrr_params_init();
	hdev->irq_vrr_vsync = platform_get_irq_byname(pdev, "vrr_vsync");
	if (hdev->irq_vrr_vsync == -ENXIO) {
		HDMITX_ERROR("%s: ERROR: hdmitx vrr_vsync irq No not found\n",
		       __func__);
			return -ENXIO;
	}
	HDMITX_INFO("vrr vsync irq = %d\n", hdev->irq_vrr_vsync);
#endif
	ret = of_property_read_u32(pdev->dev.of_node, "arc_rx_en", &val);
	if (!ret)
		hdev->arc_rx_en = val;
	else
		hdev->arc_rx_en = 0;
	return ret;
}

/*
 * amhdmitx_clktree_probe
 * get clktree info from dts
 */
static void amhdmitx_clktree_probe(struct device *hdmitx_dev, struct hdmitx_dev *hdev)
{
	struct clk *hdmi_clk_vapb, *hdmi_clk_vpu;
	struct clk *venci_top_gate, *venci_0_gate, *venci_1_gate;

	hdmi_clk_vapb = devm_clk_get(hdmitx_dev, "hdmi_vapb_clk");
	if (IS_ERR(hdmi_clk_vapb)) {
		pr_warn("vapb_clk failed to probe\n");
	} else {
		hdev->hdmitx_clk_tree.hdmi_clk_vapb = hdmi_clk_vapb;
		clk_prepare_enable(hdev->hdmitx_clk_tree.hdmi_clk_vapb);
	}

	hdmi_clk_vpu = devm_clk_get(hdmitx_dev, "hdmi_vpu_clk");
	if (IS_ERR(hdmi_clk_vpu)) {
		pr_warn("vpu_clk failed to probe\n");
	} else {
		hdev->hdmitx_clk_tree.hdmi_clk_vpu = hdmi_clk_vpu;
		clk_prepare_enable(hdev->hdmitx_clk_tree.hdmi_clk_vpu);
	}

	venci_top_gate = devm_clk_get(hdmitx_dev, "venci_top_gate");
	if (IS_ERR(venci_top_gate))
		pr_warn("venci_top_gate failed to probe\n");
	else
		hdev->hdmitx_clk_tree.venci_top_gate = venci_top_gate;

	venci_0_gate = devm_clk_get(hdmitx_dev, "venci_0_gate");
	if (IS_ERR(venci_0_gate))
		pr_warn("venci_0_gate failed to probe\n");
	else
		hdev->hdmitx_clk_tree.venci_0_gate = venci_0_gate;

	venci_1_gate = devm_clk_get(hdmitx_dev, "venci_1_gate");
	if (IS_ERR(venci_1_gate))
		pr_warn("venci_1_gate failed to probe\n");
	else
		hdev->hdmitx_clk_tree.venci_1_gate = venci_1_gate;
}

void amhdmitx21_vpu_dev_register(struct hdmitx_dev *hdev)
{
	hdev->hdmitx_vpu_clk_gate_dev =
	vpu_dev_register(VPU_VENCI, DEVICE_NAME);
}

static void amhdmitx_infoframe_init(struct hdmitx_dev *hdev)
{
	int ret = 0;

	ret = hdmi_vendor_infoframe_init(&hdev->infoframes.vend.vendor.hdmi);
	if (!ret)
		HDMITX_INFO("%s[%d] init vendor infoframe failed %d\n", __func__, __LINE__, ret);
	hdmi_avi_infoframe_init(&hdev->infoframes.avi.avi);

	// TODO, panic
	// hdmi_spd_infoframe_init(&hdev->infoframes.spd.spd,
	//	hdev->config_data.vend_data->vendor_name,
	//	hdev->config_data.vend_data->product_desc);
	hdmi_audio_infoframe_init(&hdev->infoframes.aud.audio);
	hdmi_drm_infoframe_init(&hdev->infoframes.drm.drm);
}

static int hdmitx21_status_check(void *data)
{
	int clk[3];
	int idx[3];
	struct hdmitx_dev *hdev = get_hdmitx21_device();

	if (hdev->tx_hw.chip_data && hdev->tx_hw.chip_data->chip_type != MESON_CPU_ID_S5)
		return 0;

	/* for S5, here need check the clk index 89 & 16 */
	idx[0] = 89; /* htx_tmds20_clk */
	idx[1] = 16; /* vid_pll0_clk */
	idx[2] = 92; /* cts_htx_tmds_clk */

	while (1) {
		msleep_interruptible(1000);
		if (!hdev->tx_comm.ready)
			continue;
		/* skip FRL mode */
		if (hdmitx_hw_cntl_misc(&hdev->tx_hw.base, MISC_IS_FRL_MODE, 0))
			continue;
		clk[0] = meson_clk_measure(idx[0]);
		clk[1] = meson_clk_measure(idx[1]);
		if (clk[0] && clk[1])
			continue;

		if (!clk[0]) {
			pr_debug("the clock[%d] is %d\n", idx[0], clk[0]);
			hdmitx_hw_cntl_misc(&hdev->tx_hw.base, MISC_CLK_DIV_RST, idx[0]);
			pr_debug("reset the clock div for %d\n", idx[0]);
			HDMITX_INFO("the clock[%d] is %d\n", idx[0], meson_clk_measure(idx[0]));
		}
		if (!clk[1]) {
			pr_debug("the clock[%d] is %d\n", idx[1], clk[1]);
			hdmitx_hw_cntl_misc(&hdev->tx_hw.base, MISC_CLK_DIV_RST, idx[1]);
			pr_debug("reset the clock div for %d\n", idx[1]);
			HDMITX_INFO("the clock[%d] is %d\n", idx[1], meson_clk_measure(idx[1]));
		}
		/* resend the SCDC/DIV40 config */
		if (!clk[0] || !clk[1]) {
			clk[2] = meson_clk_measure(idx[2]);
			if (clk[2] >= 340000000)
				hdmitx_hw_cntl_ddc(&hdev->tx_hw.base,
					DDC_SCDC_DIV40_SCRAMB, 1);
			else
				hdmitx_hw_cntl_ddc(&hdev->tx_hw.base,
					DDC_SCDC_DIV40_SCRAMB, 0);
		}
	}
	return 0;
}

static int amhdmitx_probe(struct platform_device *pdev)
{
	int r, ret = 0;
	struct device *device = &pdev->dev;
	struct device *dev;

	struct hdmitx_dev *hdev;
	struct hdmitx_common *tx_comm;
	struct hdmitx_tracer *tx_tracer;
	struct hdmitx_event_mgr *tx_event_mgr;
	bool hpd_state;

	pr_debug("amhdmitx_probe_start\n");

	hdev = devm_kzalloc(device, sizeof(*hdev), GFP_KERNEL);
	if (!hdev)
		return -ENOMEM;

	tx21_dev = hdev;
	dev_set_drvdata(device, hdev);
	tx_comm = &hdev->tx_comm;

	amhdmitx21_device_init(hdev);
	amhdmitx_infoframe_init(hdev);

	/*init txcommon*/
	hdmitx_common_init(tx_comm, &hdev->tx_hw.base);

	ret = amhdmitx_get_dt_info(pdev, hdev);
	/* if (ret) */
		/* return ret; */

	amhdmitx_clktree_probe(&pdev->dev, hdev);
	if (0) /* TODO */
		amhdmitx21_vpu_dev_register(hdev);

	r = alloc_chrdev_region(&hdev->hdmitx_id, 0, HDMI_TX_COUNT,
				DEVICE_NAME);
	cdev_init(&hdev->cdev, &amhdmitx_fops);
	hdev->cdev.owner = THIS_MODULE;
	r = cdev_add(&hdev->cdev, hdev->hdmitx_id, HDMI_TX_COUNT);

	hdmitx_class = class_create(THIS_MODULE, "amhdmitx");
	if (IS_ERR(hdmitx_class)) {
		unregister_chrdev_region(hdev->hdmitx_id, HDMI_TX_COUNT);
		return -1;
	}

	dev = device_create(hdmitx_class, NULL, hdev->hdmitx_id, hdev,
			    "amhdmitx%d", 0); /* kernel>=2.6.27 */

	if (!dev) {
		HDMITX_INFO("device_create create error\n");
		class_destroy(hdmitx_class);
		r = -EEXIST;
		return r;
	}
	hdev->hdtx_dev = dev;
	ret = device_create_file(dev, &dev_attr_disp_mode);
	ret = device_create_file(dev, &dev_attr_vid_mute);
	ret = device_create_file(dev, &dev_attr_config);
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
	ret = device_create_file(dev, &dev_attr_vrr_cap);
	ret = device_create_file(dev, &dev_attr_vrr_mode);
#endif
	ret = device_create_file(dev, &dev_attr_lipsync_cap);
	ret = device_create_file(dev, &dev_attr_hdmi_hdr_status);
	ret = device_create_file(dev, &dev_attr_aud_mute);
	ret = device_create_file(dev, &dev_attr_sspll);
	ret = device_create_file(dev, &dev_attr_rxsense_policy);
	ret = device_create_file(dev, &dev_attr_rxsense_state);
	ret = device_create_file(dev, &dev_attr_cedst_policy);
	ret = device_create_file(dev, &dev_attr_cedst_count);
	ret = device_create_file(dev, &dev_attr_hdcp_mode);
	ret = device_create_file(dev, &dev_attr_hdcp_ver);
	ret = device_create_file(dev, &dev_attr_hdcp_type_policy);
	ret = device_create_file(dev, &dev_attr_hdcp_lstore);
	ret = device_create_file(dev, &dev_attr_ready);
	ret = device_create_file(dev, &dev_attr_ll_mode);
	ret = device_create_file(dev, &dev_attr_ll_user_mode);
	ret = device_create_file(dev, &dev_attr_aon_output);
	ret = device_create_file(dev, &dev_attr_def_stream_type);
	ret = device_create_file(dev, &dev_attr_propagate_stream_type);
	ret = device_create_file(dev, &dev_attr_cont_smng_method);
	ret = device_create_file(dev, &dev_attr_frl_rate);
	ret = device_create_file(dev, &dev_attr_dsc_en);
	ret = device_create_file(dev, &dev_attr_hdr_priority_mode);
	ret = device_create_file(dev, &dev_attr_is_passthrough_switch);
	ret = device_create_file(dev, &dev_attr_is_hdcp_cts_te);
	ret = device_create_file(dev, &dev_attr_need_filter_hdcp_off);
	ret = device_create_file(dev, &dev_attr_filter_hdcp_off_period);
	ret = device_create_file(dev, &dev_attr_not_restart_hdcp);
	ret = device_create_file(dev, &dev_attr_clkmsr);

	/*platform related functions*/
	tx_event_mgr = hdmitx_event_mgr_create(pdev, hdev->hdtx_dev);
	hdmitx_common_attch_platform_data(tx_comm,
		HDMITX_PLATFORM_UEVENT, tx_event_mgr);
	tx_tracer = hdmitx_tracer_create(tx_event_mgr);
	hdmitx_common_attch_platform_data(tx_comm,
		HDMITX_PLATFORM_TRACER, tx_tracer);
	hdmitx_audio_register_ctrl_callback(tx_tracer, hdmitx21_ext_set_audio_output,
		hdmitx21_ext_get_audio_status);
	hdmitx_edid_init(tx_comm);
#ifdef CONFIG_AMLOGIC_LEGACY_EARLY_SUSPEND
	hdmitx_early_suspend_handler.param = hdev;
	register_early_suspend(&hdmitx_early_suspend_handler);
#endif
	hdev->nb.notifier_call = hdmitx_reboot_notifier;
	register_reboot_notifier(&hdev->nb);

	/* init HW */
	hdmitx21_meson_init(hdev);
	/*load fmt para from hw info.*/
	hdmitx_common_init_bootup_format_para(tx_comm, &tx_comm->fmt_para);
	if (tx_comm->fmt_para.vic != HDMI_0_UNKNOWN)
		hdev->tx_comm.ready = 1;

	/* update fmt_attr string from fmt_para*/
	hdmitx_format_para_rebuild_fmtattr_str(&hdev->tx_comm.fmt_para,
		hdev->tx_comm.fmt_attr, sizeof(hdev->tx_comm.fmt_attr));
	/* TODO: load init hdr state for HW info */
	/* hdmitx_hdr_state_init(hdev); */

	hdmitx_vout_init(tx_comm, &hdev->tx_hw.base);

#if IS_ENABLED(CONFIG_AMLOGIC_SND_SOC)
	if (!hdev->pxp_mode && hdmitx21_uboot_audio_en()) {
		struct aud_para *audpara = &hdev->tx_comm.cur_audio_param;

		audpara->rate = FS_48K;
		audpara->type = CT_PCM;
		audpara->size = SS_16BITS;
		audpara->chs = 2 - 1;
	}
	/* TODO: to confirm: default audio clock is ON */
	hdmitx21_audio_mute_op(1, 0);
	if (!hdev->pxp_mode)
		aout_register_client(&hdmitx_notifier_nb_a);
#endif

	/* get efuse ctrl state */
	get_hdmi_efuse(tx_comm);
	spin_lock_init(&hdev->tx_comm.edid_spinlock);
	INIT_WORK(&hdev->work_hdr, hdr_work_func);
	hdev->hdmi_hpd_wq = alloc_ordered_workqueue(DEVICE_NAME,
		WQ_HIGHPRI | __WQ_LEGACY | WQ_MEM_RECLAIM);
	INIT_DELAYED_WORK(&hdev->work_hpd_plugin, hdmitx_hpd_plugin_irq_handler);
	INIT_DELAYED_WORK(&hdev->work_hpd_plugout, hdmitx_hpd_plugout_irq_handler);
	/* hdcp related work scheduled in system workqueue */
	INIT_DELAYED_WORK(&hdev->work_start_hdcp, hdmitx_start_hdcp_handler);
	INIT_DELAYED_WORK(&hdev->work_up_hdcp_timeout, hdmitx_up_hdcp_timeout_handler);
	/* interrupt handler need to be scheduled in high priority */
	hdev->hdmi_intr_wq = alloc_workqueue("hdmitx_intr_wq", WQ_HIGHPRI | WQ_CPU_INTENSIVE, 0);
	INIT_DELAYED_WORK(&hdev->work_internal_intr, hdmitx_top_intr_handler);

	/* for rx sense feature */
	hdev->tx_comm.rxsense_wq = alloc_workqueue("hdmitx_rxsense",
					   WQ_SYSFS | WQ_FREEZABLE, 0);
	INIT_DELAYED_WORK(&hdev->tx_comm.work_rxsense, hdmitx_rxsense_process);
	/* for cedst feature */
	hdev->tx_comm.cedst_wq = alloc_workqueue("hdmitx_cedst",
					 WQ_SYSFS | WQ_FREEZABLE, 0);
	INIT_DELAYED_WORK(&hdev->tx_comm.work_cedst, hdmitx_cedst_process);

	hdmitx21_hdcp_init();
	/* bind drm before hdmi event */
	hdmitx_hook_drm(&pdev->dev);

	/* init power_uevent state */
	hdmitx21_set_uevent(HDMITX_HDCPPWR_EVENT, HDMI_WAKEUP);
	/* reset EDID/vinfo */
	if (!hdev->tx_comm.forced_edid) {
		hdmitx_edid_buffer_clear(hdev->tx_comm.EDID_buf, sizeof(hdev->tx_comm.EDID_buf));
		hdmitx_edid_rxcap_clear(&hdev->tx_comm.rxcap);
	}
	/* hpd process of bootup stage */
	mutex_lock(&hdev->tx_comm.hdmimode_mutex);
	/* enable irq firstly before any hpd handler to prevent missing irq. */
	hdmitx_setupirqs(hdev);

	/* actions in top half of plug intr, do it before enable irq */
	hpd_state = !!hdmitx_hw_cntl_misc(&hdev->tx_hw.base,
		MISC_HPD_GPI_ST, 0);
	hdmitx_hw_cntl_misc(&hdev->tx_hw.base, MISC_HPD_IRQ_TOP_HALF, hpd_state);
	/* actions in bottom half of plug intr */
	if (hpd_state)
		hdmitx_bootup_plugin_handler(hdev);
	else
		hdmitx_bootup_plugout_handler(hdev);
	/* after unlock, now can take actions of bottom half of hpd irq */
	mutex_unlock(&hdev->tx_comm.hdmimode_mutex);
	/* notify to drm hdmi */
	hdmitx_fire_drm_hpd_cb_unlocked(&hdev->tx_comm);

	/* create misc device for communication with TEE: hdcp key load ready notify */
	tee_comm_dev_reg(hdev);

	hdev->task = kthread_run(hdmitx21_status_check, (void *)hdev,
				      "kthread_hdmist_check");

	pr_debug("%s end\n", __func__);
	/*everything is ready, create sysfs here.*/
	hdmitx_sysfs_common_create(dev, &hdev->tx_comm, &hdev->tx_hw.base);
	hdev->hdmi_init = 1;

	return r;
}

static int amhdmitx_remove(struct platform_device *pdev)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(&pdev->dev);
	struct device *dev = hdev->hdtx_dev;

	/*remove sysfs before uninit/*/
	hdmitx_sysfs_common_destroy(dev);

	tee_comm_dev_unreg(hdev);
	/*unbind from drm.*/
	hdmitx_unhook_drm(&pdev->dev);
	hdmitx_edid_uninit();

	cancel_work_sync(&hdev->work_hdr);
	/* cancel_work_sync(&hdev->work_hdr_unmute); */
	hdmitx21_hdcp_exit();
	cancel_delayed_work(&hdev->work_internal_intr);
	destroy_workqueue(hdev->hdmi_intr_wq);
	cancel_delayed_work(&hdev->work_hpd_plugout);
	cancel_delayed_work(&hdev->work_hpd_plugin);
	destroy_workqueue(hdev->hdmi_hpd_wq);
	cancel_delayed_work(&hdev->tx_comm.work_rxsense);
	destroy_workqueue(hdev->tx_comm.rxsense_wq);
	cancel_delayed_work(&hdev->tx_comm.work_cedst);
	destroy_workqueue(hdev->tx_comm.cedst_wq);

	if (hdev->tx_hw.base.uninit)
		hdev->tx_hw.base.uninit(&hdev->tx_hw.base);
	hdev->hpd_event = 0xff;
	kthread_stop(hdev->task);
	hdmitx_vout_uninit();

#if IS_ENABLED(CONFIG_AMLOGIC_SND_SOC)
	aout_unregister_client(&hdmitx_notifier_nb_a);
#endif

	/* Remove the cdev */
	device_remove_file(dev, &dev_attr_disp_mode);
	device_remove_file(dev, &dev_attr_vid_mute);
	device_remove_file(dev, &dev_attr_config);
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
	device_remove_file(dev, &dev_attr_vrr_cap);
	device_remove_file(dev, &dev_attr_vrr_mode);
	device_remove_file(dev, &dev_attr_vrr_cap);
#endif
	device_remove_file(dev, &dev_attr_ll_mode);
	device_remove_file(dev, &dev_attr_ll_user_mode);
	device_remove_file(dev, &dev_attr_ready);
	device_remove_file(dev, &dev_attr_aud_mute);
	device_remove_file(dev, &dev_attr_sspll);
	device_remove_file(dev, &dev_attr_rxsense_policy);
	device_remove_file(dev, &dev_attr_rxsense_state);
	device_remove_file(dev, &dev_attr_cedst_policy);
	device_remove_file(dev, &dev_attr_cedst_count);
	device_remove_file(dev, &dev_attr_hdcp_type_policy);
	device_remove_file(dev, &dev_attr_hdmi_hdr_status);
	device_remove_file(dev, &dev_attr_hdcp_ver);
	device_remove_file(dev, &dev_attr_def_stream_type);
	device_remove_file(dev, &dev_attr_aon_output);
	device_remove_file(dev, &dev_attr_propagate_stream_type);
	device_remove_file(dev, &dev_attr_cont_smng_method);
	device_remove_file(dev, &dev_attr_frl_rate);
	device_remove_file(dev, &dev_attr_dsc_en);
	device_remove_file(dev, &dev_attr_hdr_priority_mode);
	device_remove_file(dev, &dev_attr_is_passthrough_switch);
	device_remove_file(dev, &dev_attr_is_hdcp_cts_te);
	device_remove_file(dev, &dev_attr_need_filter_hdcp_off);
	device_remove_file(dev, &dev_attr_filter_hdcp_off_period);
	device_remove_file(dev, &dev_attr_not_restart_hdcp);
	device_remove_file(dev, &dev_attr_clkmsr);
	cdev_del(&hdev->cdev);

	device_destroy(hdmitx_class, hdev->hdmitx_id);

	class_destroy(hdmitx_class);

	unregister_chrdev_region(hdev->hdmitx_id, HDMI_TX_COUNT);
	hdmitx_common_destroy(&hdev->tx_comm);
	return 0;
}

static void amhdmitx_shutdown(struct platform_device *pdev)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(&pdev->dev);

	if (hdev->aon_output) {
		hdmitx21_disable_hdcp(hdev);
		return;
	}
}

#ifdef CONFIG_PM
static int amhdmitx_suspend(struct platform_device *pdev,
			    pm_message_t state)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(&pdev->dev);

	/* if HPD is high before suspend, and there were hpd
	 * plugout -> in event happened in deep suspend stage,
	 * now resume and stay in early resume stage, still
	 * need to respond to plugin irq and read/update EDID.
	 * so clear last_hpd_handle_done_stat for re-enter
	 * plugin handle. Note there may be re-enter plugout/in
	 * handler under suspend
	 */
	hdev->tx_comm.last_hpd_handle_done_stat = HDMI_TX_NONE;
	HDMITX_INFO("amhdmitx: suspend\n");
	return 0;
}

static int amhdmitx_resume(struct platform_device *pdev)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(&pdev->dev);
	struct hdmitx_common *tx_comm = &hdev->tx_comm;
	struct hdmitx_hw_common *tx_hw_base = tx_comm->tx_hw;

	HDMITX_INFO("amhdmitx: resume\n");
	mutex_lock(&tx_comm->hdmimode_mutex);
	/* need to update EDID in case TV changed during suspend */
	tx_comm->hpd_state = !!(hdmitx_hw_cntl_misc(tx_hw_base, MISC_HPD_GPI_ST, 0));
	if (tx_comm->hpd_state)
		hdmitx_plugin_common_work(tx_comm);
	else
		hdmitx_plugout_common_work(tx_comm);
	hdmitx_common_notify_hpd_status(tx_comm, false);
	mutex_unlock(&tx_comm->hdmimode_mutex);

	/* notify to drm hdmi */
	hdmitx_fire_drm_hpd_cb_unlocked(tx_comm);

	return 0;
}

static int amhdmitx_pm_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);

	HDMITX_DEBUG("%s suspend\n", __func__);
	return amhdmitx_suspend(pdev, PMSG_SUSPEND);
}

static int amhdmitx_pm_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);

	HDMITX_DEBUG("%s resume\n", __func__);
	return amhdmitx_resume(pdev);
}

const struct dev_pm_ops hdmitx21_pm = {
	.suspend	= amhdmitx_pm_suspend,
	.resume		= amhdmitx_pm_resume,
};
#endif

static struct platform_driver amhdmitx_driver = {
	.probe	 = amhdmitx_probe,
	.remove	 = amhdmitx_remove,
#ifdef CONFIG_PM
	.suspend	= amhdmitx_suspend,
	.resume	 = amhdmitx_resume,
#endif
	.shutdown = amhdmitx_shutdown,
	.driver	 = {
		.name = DEVICE_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(meson_amhdmitx_of_match),
#ifdef CONFIG_PM
		.pm = &hdmitx21_pm,
#endif
	}
};

int  __init amhdmitx21_init(void)
{
	struct hdmitx_boot_param *param = get_hdmitx_boot_params();

	if (param->init_state & INIT_FLAG_NOT_LOAD)
		return 0;

	return platform_driver_register(&amhdmitx_driver);
}

void __exit amhdmitx21_exit(void)
{
	pr_info("%s...\n", __func__);
	// TODO stop hdcp
	platform_driver_unregister(&amhdmitx_driver);
}

//MODULE_DESCRIPTION("AMLOGIC HDMI TX driver");
//MODULE_LICENSE("GPL");
//MODULE_VERSION("1.0.0");

/*************DRM connector API**************/
/*hdcp functions*/
static void drm_hdmitx_hdcp_init(void)
{
}

static void drm_hdmitx_hdcp_exit(void)
{
}

static void drm_hdmitx_hdcp_enable(int hdcp_type)
{
	struct hdmitx_dev *hdev = get_hdmitx21_device();

	switch (hdcp_type) {
	case HDCP_NULL:
		HDMITX_ERROR("%s enabled HDCP_NULL\n", __func__);
		break;
	case HDCP_MODE14:
		hdev->tx_comm.hdcp_mode = 0x1;
		hdcp_mode_set(1);
		break;
	case HDCP_MODE22:
		hdev->tx_comm.hdcp_mode = 0x2;
		hdcp_mode_set(2);
		break;
	default:
		HDMITX_ERROR("%s unknown hdcp %d\n", __func__, hdcp_type);
		break;
	};
}

static void drm_hdmitx_hdcp_disable(void)
{
	struct hdmitx_dev *hdev = get_hdmitx21_device();

	hdmitx21_reset_hdcp(&hdev->tx_comm);
}

static void drm_hdmitx_hdcp_disconnect(void)
{
	drm_hdmitx_hdcp_disable();
}

static unsigned int drm_hdmitx_get_tx_hdcp_cap(void)
{
	int lstore = 0;

	if (get_hdcp2_lstore())
		lstore |= HDCP_MODE22;
	if (get_hdcp1_lstore())
		lstore |= HDCP_MODE14;

	HDMITX_INFO("%s tx hdcp [%d]\n", __func__, lstore);
	return lstore;
}

static unsigned int drm_hdmitx_get_rx_hdcp_cap(void)
{
	unsigned int rxhdcp = 0;
	struct hdmitx_dev *hdev = get_hdmitx21_device();

	/* if TX don't have HDCP22 key, skip RX hdcp22 ver */
	/* note that during hdcp1.4 authentication, read hdcp version
	 * of connected TV set(capable of hdcp2.2) may cause TV
	 * switch its hdcp mode, and flash screen. should not
	 * read hdcp version of sink during hdcp1.4 authentication.
	 * if hdcp1.4 authentication currently, force return hdcp1.4
	 */
	if (hdev->tx_comm.hdcp_mode == 0x1) {
		rxhdcp = HDCP_MODE14;
	} else if (get_hdcp2_lstore() && is_rx_hdcp2ver()) {
		rx_hdcp2_ver = 1;
		rxhdcp = HDCP_MODE22 | HDCP_MODE14;
	} else {
		rx_hdcp2_ver = 0;
		rxhdcp = HDCP_MODE14;
	}

	HDMITX_INFO("%s rx hdcp [%d]\n", __func__, rxhdcp);
	return rxhdcp;
}

static void drm_hdmitx_register_hdcp_notify(struct connector_hdcp_cb *cb)
{
	struct hdmitx_dev *hdev = get_hdmitx21_device();

	if (hdev->drm_hdcp_cb.hdcp_notify)
		HDMITX_ERROR("Register hdcp notify again!?\n");

	hdev->drm_hdcp_cb.hdcp_notify = cb->hdcp_notify;
	hdev->drm_hdcp_cb.data = cb->data;
}

static struct meson_hdmitx_dev drm_hdmitx_instance = {
	.get_hdmi_hdr_status = hdmi_hdr_status_to_drm,

	/*hdcp apis*/
	.hdcp_init = drm_hdmitx_hdcp_init,
	.hdcp_exit = drm_hdmitx_hdcp_exit,
	.hdcp_enable = drm_hdmitx_hdcp_enable,
	.hdcp_disable = drm_hdmitx_hdcp_disable,
	.hdcp_disconnect = drm_hdmitx_hdcp_disconnect,
	.get_tx_hdcp_cap = drm_hdmitx_get_tx_hdcp_cap,
	.get_rx_hdcp_cap = drm_hdmitx_get_rx_hdcp_cap,
	.register_hdcp_notify = drm_hdmitx_register_hdcp_notify,
	.get_vrr_cap = drm_hdmitx_get_vrr_cap,
	.get_vrr_mode_group = drm_hdmitx_get_vrr_mode_group,
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
	.set_vframe_rate_hint = hdmitx_set_fr_hint,
#endif
};

int hdmitx_hook_drm(struct device *device)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(device);

	return hdmitx_bind_meson_drm(device,
		&hdev->tx_comm,
		&hdev->tx_hw.base,
		&drm_hdmitx_instance);
}

int hdmitx_unhook_drm(struct device *device)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(device);

	return hdmitx_unbind_meson_drm(device,
		&hdev->tx_comm,
		&hdev->tx_hw.base,
		&drm_hdmitx_instance);
}

/*************DRM connector API end**************/

/****** tee_hdcp key related start ******/
static long hdcp_comm_ioctl(struct file *file,
	unsigned int cmd,
	unsigned long arg)
{
	int rtn_val;
	struct hdmitx_dev *hdev = get_hdmitx21_device();

	switch (cmd) {
	case TEE_HDCP_IOC_START:
		/* notify by TEE, hdcp key ready */
		rtn_val = 0;
		HDMITX_INFO("tee load hdcp key ready\n");
		if (hdev->tx_comm.hpd_state == 1 &&
			hdev->tx_comm.ready &&
			hdmitx21_get_hdcp_mode() == 0) {
			HDMITX_INFO("hdmi ready but hdcp not enabled, enable now\n");
			if (hdcp_need_control_by_upstream(hdev))
				HDMITX_INFO("currently hdcp should started by upstream\n");
			else
				hdmitx21_enable_hdcp(hdev);
		}
		break;
	default:
		rtn_val = -EPERM;
		break;
	}
	return rtn_val;
}

static const struct file_operations hdcp_comm_file_operations = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = hdcp_comm_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = hdcp_comm_ioctl,
#endif
};

static void tee_comm_dev_reg(struct hdmitx_dev *hdev)
{
	int ret;

	hdev->hdcp_comm_device.minor = MISC_DYNAMIC_MINOR;
	hdev->hdcp_comm_device.name = "tee_comm_hdcp";
	hdev->hdcp_comm_device.fops = &hdcp_comm_file_operations;

	ret = misc_register(&hdev->hdcp_comm_device);
	if (ret < 0)
		HDMITX_ERROR("%s misc_register fail\n", __func__);
}

static void tee_comm_dev_unreg(struct hdmitx_dev *hdev)
{
	misc_deregister(&hdev->hdcp_comm_device);
}

/****** tee_hdcp key related end ******/

