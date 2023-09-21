// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

/* define DEBUG macro to enable pr_debug
 * print to log buffer
 */
//#define DEBUG
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
#include <linux/reboot.h>
#include <linux/i2c.h>
#include <linux/miscdevice.h>
#include <linux/vmalloc.h>
//#include <linux/amlogic/cpu_version.h>
#include <linux/amlogic/media/vout/vinfo.h>
#include <linux/amlogic/media/vout/vout_notify.h>
#if IS_ENABLED(CONFIG_AMLOGIC_SND_SOC)
#include <linux/amlogic/media/sound/aout_notify.h>
#endif
#include <linux/amlogic/media/vout/hdmi_tx/hdmi_info_global.h>
#include <linux/amlogic/media/vout/hdmi_tx/hdmi_tx_ddc.h>
#include <linux/amlogic/media/vout/hdmi_tx/hdmi_tx_module.h>
#include <linux/amlogic/media/vout/hdmi_tx/hdmi_config.h>
#include <linux/amlogic/media/vout/hdmi_tx_ext.h>
#include <linux/amlogic/media/registers/cpu_version.h>
#include <linux/extcon.h>
#include <linux/extcon-provider.h>
#include <linux/of_gpio.h>
#include <linux/rtc.h>
#include <linux/timekeeping.h>

#include "hw/tvenc_conf.h"
#include "hw/common.h"
#include "hw/hw_clk.h"
#include "hw/reg_ops.h"
#include "hdmi_tx_hdcp.h"
#include "meson_drm_hdmitx.h"
#include "meson_hdcp.h"

#include <linux/component.h>
#include <uapi/drm/drm_mode.h>
#include <linux/amlogic/gki_module.h>
#include <drm/amlogic/meson_drm_bind.h>
#include <hdmitx_boot_parameters.h>
#include <hdmitx_drm_hook.h>
#include <hdmitx_sysfs_common.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_common.h>

#define DEVICE_NAME "amhdmitx"
#define HDMI_TX_COUNT 32
#define HDMI_TX_POOL_NUM  6
#define HDMI_TX_RESOURCE_NUM 4
#define HDMI_TX_PWR_CTRL_NUM	6

#define to_hdmitx20_dev(x)	container_of(x, struct hdmitx_dev, tx_comm)

static struct class *hdmitx_class;
static void hdmitx_get_edid(struct hdmitx_dev *hdev);
static void hdmitx_set_drm_pkt(struct master_display_info_s *data);
static void hdmitx_set_vsif_pkt(enum eotf_type type, enum mode_type
	tunnel_mode, struct dv_vsif_para *data, bool signal_sdr);
static void hdmitx_set_hdr10plus_pkt(unsigned int flag,
				     struct hdr10plus_para *data);
static void hdmitx_set_cuva_hdr_vsif(struct cuva_hdr_vsif_para *data);
static void hdmitx_set_cuva_hdr_vs_emds(struct cuva_hdr_vs_emds_para *data);
static void hdmitx_set_emp_pkt(unsigned char *data,
			       unsigned int type,
			       unsigned int size);
static int check_fbc_special(unsigned char *edid_dat);
static void hdmitx_fmt_attr(struct hdmitx_dev *hdev);
static void edidinfo_attach_to_vinfo(struct hdmitx_dev *hdev);
static void edidinfo_detach_to_vinfo(struct hdmitx_dev *hdev);
static void update_vinfo_from_formatpara(void);
static bool is_cur_tmds_div40(struct hdmitx_dev *hdev);
static void hdmitx_resend_div40(struct hdmitx_dev *hdev);
static unsigned int hdmitx_get_frame_duration(void);
static int hdmitx_hook_drm(struct device *device);
static int hdmitx_unhook_drm(struct device *device);
const char *hdmitx_mode_get_timing_name(enum hdmi_vic vic);
const struct hdmi_timing *hdmitx_mode_match_timing_name(const char *name);

/*
 * Normally, after the HPD in or late resume, there will reading EDID, and
 * notify application to select a hdmi mode output. But during the mode
 * setting moment, there may be HPD out. It will clear the edid data, ..., etc.
 * To avoid such case, here adds the hdmimode_mutex to let the HPD in, HPD out
 * handler and mode setting sequentially.
 */
static DEFINE_MUTEX(hdmimode_mutex);

#ifdef CONFIG_AMLOGIC_VOUT_SERVE
static struct vinfo_s *hdmitx_get_current_vinfo(void *data);
#else
static struct vinfo_s *hdmitx_get_current_vinfo(void *data)
{
	return NULL;
}
#endif

#ifdef CONFIG_OF
static struct amhdmitx_data_s amhdmitx_data_g12a = {
	.chip_type = MESON_CPU_ID_G12A,
	.chip_name = "g12a",
};

static struct amhdmitx_data_s amhdmitx_data_g12b = {
	.chip_type = MESON_CPU_ID_G12B,
	.chip_name = "g12b",
};

static struct amhdmitx_data_s amhdmitx_data_sm1 = {
	.chip_type = MESON_CPU_ID_SM1,
	.chip_name = "sm1",
};

static struct amhdmitx_data_s amhdmitx_data_sc2 = {
	.chip_type = MESON_CPU_ID_SC2,
	.chip_name = "sc2",
};

static struct amhdmitx_data_s amhdmitx_data_tm2 = {
	.chip_type = MESON_CPU_ID_TM2,
	.chip_name = "tm2",
};

static const struct of_device_id meson_amhdmitx_of_match[] = {
	{
		.compatible	 = "amlogic, amhdmitx-g12a",
		.data = &amhdmitx_data_g12a,
	},
	{
		.compatible	 = "amlogic, amhdmitx-g12b",
		.data = &amhdmitx_data_g12b,
	},
	{
		.compatible	 = "amlogic, amhdmitx-sm1",
		.data = &amhdmitx_data_sm1,
	},
	{
		.compatible	 = "amlogic, amhdmitx-sc2",
		.data = &amhdmitx_data_sc2,
	},
	{
		.compatible	 = "amlogic, amhdmitx-tm2",
		.data = &amhdmitx_data_tm2,
	},
	{},
};
#else
#define meson_amhdmitx_dt_match NULL
#endif

static DEFINE_MUTEX(setclk_mutex);
static DEFINE_MUTEX(getedid_mutex);

static struct hdmitx_dev *tx20_dev;
static const struct dv_info dv_dummy;
static int log_level;
static bool hdmitx_edid_done;

/* for SONY-KD-55A8F TV, need to mute more frames
 * when switch DV(LL)->HLG
 */
static int hdr_mute_frame = 20;
static char suspend_fmt_attr[16];

struct vout_device_s hdmitx_vdev = {
	.dv_info = &dv_dummy,
	.fresh_tx_hdr_pkt = hdmitx_set_drm_pkt,
	.fresh_tx_vsif_pkt = hdmitx_set_vsif_pkt,
	.fresh_tx_hdr10plus_pkt = hdmitx_set_hdr10plus_pkt,
	.fresh_tx_cuva_hdr_vsif = hdmitx_set_cuva_hdr_vsif,
	.fresh_tx_cuva_hdr_vs_emds = hdmitx_set_cuva_hdr_vs_emds,
	.fresh_tx_emp_pkt = hdmitx_set_emp_pkt,
	.get_attr = NULL,
	.setup_attr = NULL,
	.video_mute = hdmitx20_video_mute_op,
};

struct hdmi_config_platform_data *hdmi_pdata;

static struct extcon_dev *hdmitx_extcon_hdmi;

static const unsigned int hdmi_extcon_cable[] = {
	EXTCON_DISP_HDMI,
	EXTCON_NONE,
};

static struct hdmitx_uevent hdmi_events[] = {
	{
		.type = HDMITX_HPD_EVENT,
		.env = "hdmitx_hpd=",
	},
	{
		.type = HDMITX_HDCP_EVENT,
		.env = "hdmitx_hdcp=",
	},
	{
		.type = HDMITX_CUR_ST_EVENT,
		.env = "hdmitx_current_status=",
	},
	{
		.type = HDMITX_AUDIO_EVENT,
		.env = "hdmitx_audio=",
	},
	{
		.type = HDMITX_HDCPPWR_EVENT,
		.env = "hdmitx_hdcppwr=",
	},
	{
		.type = HDMITX_HDR_EVENT,
		.env = "hdmitx_hdr=",
	},
	{
		.type = HDMITX_RXSENSE_EVENT,
		.env = "hdmitx_rxsense=",
	},
	{
		.type = HDMITX_CEDST_EVENT,
		.env = "hdmitx_cedst=",
	},
	{ /* end of hdmi_events[] */
		.type = HDMITX_NONE_EVENT,
	},
};

/* indicate plugout before systemcontrol boot  */
static bool plugout_mute_flg;

int hdmitx_set_uevent_state(enum hdmitx_event type, int state)
{
	int ret = -1;
	struct hdmitx_uevent *event;

	for (event = hdmi_events; event->type != HDMITX_NONE_EVENT; event++) {
		if (type == event->type)
			break;
	}
	if (event->type == HDMITX_NONE_EVENT)
		return ret;
	event->state = state;

	if (log_level == 0xfe)
		pr_info("[%s] event_type: %s%d\n", __func__, event->env, state);
	return 0;
}

int hdmitx_set_uevent(enum hdmitx_event type, int val)
{
	char env[MAX_UEVENT_LEN];
	struct hdmitx_uevent *event = hdmi_events;
	struct hdmitx_dev *hdev = get_hdmitx_device();
	char *envp[2];
	int ret = -1;

	for (event = hdmi_events; event->type != HDMITX_NONE_EVENT; event++) {
		if (type == event->type)
			break;
	}
	if (event->type == HDMITX_NONE_EVENT)
		return ret;
	if (event->state == val)
		return ret;

	event->state = val;
	memset(env, 0, sizeof(env));
	envp[0] = env;
	envp[1] = NULL;
	snprintf(env, MAX_UEVENT_LEN, "%s%d", event->env, val);

	ret = kobject_uevent_env(&hdev->hdtx_dev->kobj, KOBJ_CHANGE, envp);
	if (log_level == 0xfe)
		pr_info("%s %s %d\n", __func__, env, ret);
	return ret;
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
int hdr_status_pos;

static inline void hdmitx_notify_hpd(int hpd, void *p)
{
	if (hpd)
		hdmitx_event_notify(HDMITX_PLUG, p);
	else
		hdmitx_event_notify(HDMITX_UNPLUG, NULL);
}

#ifdef CONFIG_AMLOGIC_LEGACY_EARLY_SUSPEND
#include <linux/amlogic/pm.h>
static void hdmitx_early_suspend(struct early_suspend *h)
{
	struct hdmitx_dev *hdev = (struct hdmitx_dev *)h->param;
	struct hdmitx_common *tx_comm = &hdev->tx_comm;
	bool need_rst_ratio = hdmitx_find_vendor_ratio(hdev);
	unsigned int mute_us =
		hdev->debug_param.avmute_frame * hdmitx_get_frame_duration();

	/* Here remove the hpd_lock. Suppose at beginning, the hdmi cable is
	 * unconnected, and system goes to suspend, during this time, cable
	 * plugin. In this time, there will be no CEC physical address update
	 * and must wait the resume.
	 */
	mutex_lock(&hdmimode_mutex);
	/* under suspend, driver should not respond to mode setting,
	 * as it may cause logic abnormal, most importantly,
	 * it will enable hdcp and occupy DDC channel with high
	 * priority, though there's protection in system control,
	 * driver still need protection in case of old android version
	 */
	hdev->tx_comm.suspend_flag = true;
	if (hdev->cedst_policy)
		cancel_delayed_work(&hdev->work_cedst);
	hdev->ready = 0;
	hdev->tx_hw.cntlmisc(&hdev->tx_hw, MISC_SUSFLAG, 1);
	usleep_range(10000, 10010);
	hdev->tx_hw.cntlmisc(&hdev->tx_hw, MISC_AVMUTE_OP, SET_AVMUTE);
	/* delay 100ms after avmute is a empirical value,
	 * mute_us is for debug, at least 16ms for 60fps
	 */
	if (hdev->debug_param.avmute_frame > 0)
		msleep(mute_us / 1000);
	else
		msleep(100);
	pr_info(SYS "HDMITX: Early Suspend\n");
	/* if (hdev->hdcp_ctl_lvl > 0 && */
		/* hdev->hwop.am_hdmitx_hdcp_disable) */
		/* hdev->hwop.am_hdmitx_hdcp_disable(); */
	hdev->hwop.cntl(hdev, HDMITX_EARLY_SUSPEND_RESUME_CNTL,
		HDMITX_EARLY_SUSPEND);
	hdev->tx_comm.cur_VIC = HDMI_0_UNKNOWN;
	hdev->output_blank_flag = 0;
	hdev->hwop.cntlddc(hdev, DDC_HDCP_MUX_INIT, 1);
	hdev->hwop.cntlddc(hdev, DDC_HDCP_OP, HDCP14_OFF);
	hdmitx_set_drm_pkt(NULL);
	hdmitx_set_vsif_pkt(0, 0, NULL, true);
	hdmitx_set_hdr10plus_pkt(0, NULL);
	hdmitx_edid_clear(hdev);
	hdmitx_edid_done = false;
	hdmitx_edid_ram_buffer_clear(hdev);
	edidinfo_detach_to_vinfo(hdev);

	hdmitx_set_uevent(HDMITX_HDCPPWR_EVENT, HDMI_SUSPEND);
	hdmitx_set_uevent(HDMITX_AUDIO_EVENT, 0);
	hdev->tx_hw.cntlconfig(&hdev->tx_hw, CONF_CLR_AVI_PACKET, 0);
	hdev->tx_hw.cntlconfig(&hdev->tx_hw, CONF_CLR_VSDB_PACKET, 0);
	/* for huawei TV, it will display green screen pattern under
	 * 4k50/60hz y420 deep color when receive amvute. After disable
	 * phy of box, TV will continue mute and stay in still frame
	 * mode for a few frames, if it receives scdc clk ratio change
	 * during this period, it may recognize it as signal unstable
	 * instead of no signal, and keep mute pattern for several
	 * seconds. Here keep hdmi output disabled for a few frames
	 * so let TV exit its still frame mode and not show pattern
	 */
	if (need_rst_ratio) {
		usleep_range(120000, 120010);
		hdev->hwop.cntlddc(hdev, DDC_SCDC_DIV40_SCRAMB, 0);
	}
	memcpy(suspend_fmt_attr, tx_comm->fmt_attr, sizeof(tx_comm->fmt_attr));
	mutex_unlock(&hdmimode_mutex);
}

static void hdmitx_late_resume(struct early_suspend *h)
{
	const struct vinfo_s *info = hdmitx_get_current_vinfo(NULL);
	struct hdmitx_dev *hdev = (struct hdmitx_dev *)h->param;
	u8 hpd_state = 0;

	mutex_lock(&hdmimode_mutex);

	hdev->hpd_lock = 0;
	/* update status for hpd and switch/state */
	hpd_state = !!(hdev->tx_hw.cntlmisc(&hdev->tx_hw, MISC_HPD_GPI_ST, 0));
	/* for RDK userspace, after receive plug change uevent,
	 * it will check connector state before enable encoder.
	 * so should not change hpd_state other than in plug handler
	 */
	if (hdev->hdcp_ctl_lvl != 0x1)
		hdev->tx_comm.hpd_state = hpd_state;
	if (hdev->tx_comm.hpd_state)
		hdev->already_used = 1;

	pr_info("hdmitx hpd state: %d\n", hdev->tx_comm.hpd_state);

	/*force to get EDID after resume */
	if (hpd_state) {
		/*add i2c soft reset before read EDID */
		hdev->hwop.cntlddc(hdev, DDC_GLITCH_FILTER_RESET, 0);
		if (hdev->data->chip_type >= MESON_CPU_ID_G12A)
			hdev->tx_hw.cntlmisc(&hdev->tx_hw, MISC_I2C_RESET, 0);
		hdmitx_get_edid(hdev);
		hdmitx_edid_done = true;
	}
	hdmitx_notify_hpd(hpd_state,
			  hdev->tx_comm.edid_parsing ?
			  hdev->tx_comm.edid_ptr : NULL);

	/* recover attr (especially for HDR case) */
	if (info && drm_hdmitx_chk_mode_attr_sup(info->name,
	    suspend_fmt_attr))
		setup_attr(suspend_fmt_attr);
	/* force revert state to trigger uevent send */
	if (hdev->tx_comm.hpd_state) {
		hdmitx_set_uevent_state(HDMITX_HPD_EVENT, 0);
		hdmitx_set_uevent_state(HDMITX_AUDIO_EVENT, 0);
		extcon_set_state(hdmitx_extcon_hdmi, EXTCON_DISP_HDMI, 0);
	} else {
		hdmitx_set_uevent_state(HDMITX_HPD_EVENT, 1);
		hdmitx_set_uevent_state(HDMITX_AUDIO_EVENT, 1);
		extcon_set_state(hdmitx_extcon_hdmi, EXTCON_DISP_HDMI, 1);
	}
	hdev->tx_comm.suspend_flag = false;

	extcon_set_state_sync(hdmitx_extcon_hdmi, EXTCON_DISP_HDMI,
		hdev->tx_comm.hpd_state);
	hdmitx_set_uevent(HDMITX_HPD_EVENT, hdev->tx_comm.hpd_state);
	hdmitx_set_uevent(HDMITX_HDCPPWR_EVENT, HDMI_WAKEUP);
	hdmitx_set_uevent(HDMITX_AUDIO_EVENT, hdev->tx_comm.hpd_state);
	pr_info("amhdmitx: late resume module %d\n", __LINE__);
	hdev->hwop.cntl(hdev, HDMITX_EARLY_SUSPEND_RESUME_CNTL,
		HDMITX_LATE_RESUME);
	hdev->tx_hw.cntlmisc(&hdev->tx_hw, MISC_SUSFLAG, 0);
	pr_info(SYS "HDMITX: Late Resume\n");
	mutex_unlock(&hdmimode_mutex);
	/* hpd plug uevent may not be handled by rdk userspace
	 * during suspend/resume (such as no plugout uevent is
	 * triggered, and subsequent plugin event will be ignored)
	 * as a result, hdmi/hdcp may not set correctly.
	 * in such case, force restart hdmi/hdcp when resume.
	 */
	if (hdev->tx_comm.hpd_state) {
		if (hdev->hdcp_ctl_lvl == 0x1)
			hdev->hwop.am_hdmitx_set_out_mode();
	}
	/*notify to drm hdmi*/
	hdmitx_hpd_notify_unlocked(&hdev->tx_comm);
}

/* Set avmute_set signal to HDMIRX */
static int hdmitx_reboot_notifier(struct notifier_block *nb,
				  unsigned long action, void *data)
{
	struct hdmitx_dev *hdev = container_of(nb, struct hdmitx_dev, nb);
	unsigned int mute_us =
		hdev->debug_param.avmute_frame * hdmitx_get_frame_duration();

	hdev->ready = 0;
	hdev->tx_hw.cntlmisc(&hdev->tx_hw, MISC_AVMUTE_OP, SET_AVMUTE);
	if (hdev->debug_param.avmute_frame > 0)
		msleep(mute_us / 1000);
	else
		msleep(100);
	hdev->tx_hw.cntlmisc(&hdev->tx_hw, MISC_TMDS_PHY_OP, TMDS_PHY_DISABLE);
	hdev->hwop.cntl(hdev, HDMITX_EARLY_SUSPEND_RESUME_CNTL,
		HDMITX_EARLY_SUSPEND);

	return NOTIFY_OK;
}

static struct early_suspend hdmitx_early_suspend_handler = {
	.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN - 10,
	.suspend = hdmitx_early_suspend,
	.resume = hdmitx_late_resume,
};
#endif

#undef DISABLE_AUDIO

static  int  set_disp_mode(struct hdmitx_dev *hdev, const char *mode)
{
	int ret =  -1;
	enum hdmi_vic vic;
	const struct hdmi_timing *timing = 0;

	/*function for debug, only get vic and check if ip can support, skip rx cap check.*/
	timing = hdmitx_mode_match_timing_name(mode);
	if (!timing || timing->vic == HDMI_0_UNKNOWN) {
		pr_err("unknown mode %s\n", mode);
		return -EINVAL;
	}

	vic = timing->vic;
	if (hdmitx_common_validate_vic(&hdev->tx_comm, timing->vic) != 0) {
		pr_err("ip cannot support mode %s. %d\n", mode, timing->vic);
		return -EINVAL;
	}

	if (vic != HDMI_0_UNKNOWN) {
		hdev->mux_hpd_if_pin_high_flag = 1;
		if (hdev->vic_count == 0) {
			if (hdev->unplug_powerdown)
				return 0;
		}
	}

	hdev->tx_comm.cur_VIC = HDMI_0_UNKNOWN;
	ret = hdmitx_set_display(hdev, vic);
	if (ret >= 0) {
		hdev->hwop.cntl(hdev,
			HDMITX_AVMUTE_CNTL, AVMUTE_CLEAR);
		hdev->tx_comm.cur_VIC = vic;
		hdev->audio_param_update_flag = 1;
		hdev->auth_process_timer = AUTH_PROCESS_TIME;
	}

	if (hdev->tx_comm.cur_VIC == HDMI_0_UNKNOWN) {
		if (hdev->hpdmode == 2) {
			/* edid will be read again when hpd is muxed and
			 * it is high
			 */
			hdmitx_edid_clear(hdev);
			hdev->mux_hpd_if_pin_high_flag = 0;
		}
		if (hdev->hwop.cntl) {
			hdev->hwop.cntl(hdev,
				HDMITX_HWCMD_TURNOFF_HDMIHW,
				(hdev->hpdmode == 2) ? 1 : 0);
		}
	}
	return ret;
}

static void hdmitx_pre_display_init(struct hdmitx_dev *hdev)
{
	hdev->tx_comm.cur_VIC = HDMI_0_UNKNOWN;
	hdev->auth_process_timer = AUTH_PROCESS_TIME;
	hdev->hwop.cntlddc(hdev, DDC_HDCP_MUX_INIT, 1);
	hdev->hwop.cntlddc(hdev, DDC_HDCP_OP, HDCP14_OFF);
	/* msleep(10); */
	hdev->tx_hw.cntlmisc(&hdev->tx_hw, MISC_TMDS_PHY_OP,
		TMDS_PHY_DISABLE);
	hdev->tx_hw.cntlconfig(&hdev->tx_hw,
		CONF_CLR_AVI_PACKET, 0);
	hdev->tx_hw.cntlconfig(&hdev->tx_hw,
		CONF_CLR_VSDB_PACKET, 0);
}

static void hdmi_physical_size_to_vinfo(struct hdmitx_dev *hdev)
{
	unsigned int width, height;
	struct vinfo_s *info = &hdev->tx_comm.hdmitx_vinfo;

	if (info->mode == VMODE_HDMI) {
		width = hdev->tx_comm.rxcap.physical_width;
		height = hdev->tx_comm.rxcap.physical_height;
		if (width == 0 || height == 0) {
			info->screen_real_width = info->aspect_ratio_num;
			info->screen_real_height = info->aspect_ratio_den;
		} else {
			info->screen_real_width = width;
			info->screen_real_height = height;
		}
		if (hdev->log_level & VINFO_LOG)
			pr_info(SYS "update physical size: %d %d\n",
				info->screen_real_width, info->screen_real_height);
	}
}

static void hdrinfo_to_vinfo(struct hdr_info *hdrinfo, struct hdmitx_dev *hdev)
{
	memcpy(hdrinfo, &hdev->tx_comm.rxcap.hdr_info, sizeof(struct hdr_info));
	hdrinfo->colorimetry_support = hdev->tx_comm.rxcap.colorimetry_data;
}

static void rxlatency_to_vinfo(struct vinfo_s *info, struct rx_cap *rx)
{
	if (!info || !rx)
		return;
	info->rx_latency.vLatency = rx->vLatency;
	info->rx_latency.aLatency = rx->aLatency;
	info->rx_latency.i_vLatency = rx->i_vLatency;
	info->rx_latency.i_aLatency = rx->i_aLatency;
}

static void edidinfo_attach_to_vinfo(struct hdmitx_dev *hdev)
{
	struct vinfo_s *info = &hdev->tx_comm.hdmitx_vinfo;

	mutex_lock(&getedid_mutex);
	hdrinfo_to_vinfo(&info->hdr_info, hdev);
	if (hdev->tx_comm.fmt_para.cd == COLORDEPTH_24B)
		memset(&info->hdr_info, 0, sizeof(struct hdr_info));
	rxlatency_to_vinfo(info, &hdev->tx_comm.rxcap);
	hdmitx_vdev.dv_info = &hdev->tx_comm.rxcap.dv_info;
	hdmi_physical_size_to_vinfo(hdev);
	memcpy(info->hdmichecksum, hdev->tx_comm.rxcap.chksum, 10);
	mutex_unlock(&getedid_mutex);
}

static void edidinfo_detach_to_vinfo(struct hdmitx_dev *hdev)
{
	struct vinfo_s *info = &hdev->tx_comm.hdmitx_vinfo;

	memset(&info->hdr_info, 0, sizeof(info->hdr_info));
	memset(&info->rx_latency, 0, sizeof(info->rx_latency));
	hdmitx_vdev.dv_info = &dv_dummy;

	info->screen_real_width = 0;
	info->screen_real_height = 0;
}

/*disp_mode attr*/
static ssize_t disp_mode_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	pos += snprintf(buf + pos, PAGE_SIZE, "VIC:%d\n",
		hdev->tx_comm.cur_VIC);
	return pos;
}

static ssize_t disp_mode_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t count)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	set_disp_mode(hdev, buf);
	return count;
}

/* for android application data exchange / swap */
static char *tmp_swap;
static DEFINE_MUTEX(mutex_swap);

static ssize_t swap_store(struct device *dev,
			  struct device_attribute *attr,
			  const char *buf, size_t count)
{
	pr_info("%s: %s\n", __func__, buf);
	mutex_lock(&mutex_swap);

	kfree(tmp_swap);
	tmp_swap = kzalloc(count + 1, GFP_KERNEL);
	if (!tmp_swap) {
		mutex_unlock(&mutex_swap);
		return count;
	}
	memcpy(tmp_swap, buf, count);
	tmp_swap[count] = '\0'; /* padding end string */
	mutex_unlock(&mutex_swap);
	return count;
}

static ssize_t swap_show(struct device *dev,
			 struct device_attribute *attr, char *buf)
{
	int i = 0;
	int n = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);
	struct rx_cap *prxcap = &hdev->tx_comm.rxcap;
	struct hdcprp14_topo *topo14 = &hdev->topo_info->topo.topo14;

	mutex_lock(&mutex_swap);

	if (!tmp_swap ||
	    (!hdev->tx_comm.edid_parsing && !strstr(tmp_swap, "hdcp.topo"))) {
		mutex_unlock(&mutex_swap);
		return n;
	}

	/* VSD: Video Short Descriptor */
	if (strstr(tmp_swap, "edid.vsd"))
		for (i = 0; i < prxcap->vsd.len; i++)
			n += snprintf(buf + n, PAGE_SIZE - n, "%02x",
				prxcap->vsd.raw[i]);
	/* ASD: Audio Short Descriptor */
	if (strstr(tmp_swap, "edid.asd"))
		for (i = 0; i < prxcap->asd.len; i++)
			n += snprintf(buf + n, PAGE_SIZE - n, "%02x",
				prxcap->asd.raw[i]);
	/* CEC: Physical Address */
	if (strstr(tmp_swap, "edid.cec"))
		n += snprintf(buf + n, PAGE_SIZE - n, "%x%x%x%x",
			hdev->hdmi_info.vsdb_phy_addr.a,
			hdev->hdmi_info.vsdb_phy_addr.b,
			hdev->hdmi_info.vsdb_phy_addr.c,
			hdev->hdmi_info.vsdb_phy_addr.d);
	/* HDCP TOPO */
	if (strstr(tmp_swap, "hdcp.topo")) {
		char *tmp = (char *)topo14;

		pr_info("max_cascade_exceeded %d\n",
			topo14->max_cascade_exceeded);
		pr_info("depth %d\n", topo14->depth);
		pr_info("max_devs_exceeded %d\n", topo14->max_devs_exceeded);
		pr_info("device_count %d\n", topo14->device_count);
		for (i = 0; i < sizeof(struct hdcprp14_topo); i++)
			n += snprintf(buf + n, PAGE_SIZE - n, "%02x", tmp[i]);
	}

	kfree(tmp_swap);
	tmp_swap = NULL;

	mutex_unlock(&mutex_swap);
	return n;
}

static ssize_t aud_mode_show(struct device *dev,
			     struct device_attribute *attr,
			     char *buf)
{
	return 0;
}

static ssize_t aud_mode_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	/* set_disp_mode(buf); */
	struct hdmitx_audpara *audio_param =
		&hdev->cur_audio_param;
	if (strncmp(buf, "32k", 3) == 0) {
		audio_param->sample_rate = FS_32K;
	} else if (strncmp(buf, "44.1k", 5) == 0) {
		audio_param->sample_rate = FS_44K1;
	} else if (strncmp(buf, "48k", 3) == 0) {
		audio_param->sample_rate = FS_48K;
	} else {
		hdev->force_audio_flag = 0;
		return count;
	}
	audio_param->type = CT_PCM;
	audio_param->channel_num = CC_2CH;
	audio_param->sample_size = SS_16BITS;

	hdev->audio_param_update_flag = 1;
	hdev->force_audio_flag = 1;

	return count;
}

/*
 * sink_type attr
 * sink, or repeater
 */
static ssize_t sink_type_show(struct device *dev, struct device_attribute *attr,
			      char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	if (!hdev->tx_comm.hpd_state) {
		pos += snprintf(buf + pos, PAGE_SIZE, "none\n");
		return pos;
	}

	if (hdev->hdmi_info.vsdb_phy_addr.b)
		pos += snprintf(buf + pos, PAGE_SIZE, "repeater\n");
	else
		pos += snprintf(buf + pos, PAGE_SIZE, "sink\n");

	return pos;
}

/*
 * hdcp_repeater attr
 * For hdcp 22, hdcp_tx22 will write to hdcp_repeater_store
 * For hdcp 14, directly get bcaps bit
 */
static ssize_t hdcp_repeater_show(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	if (hdev->hdcp_mode == 1)
		hdev->hdcp_bcaps_repeater = hdev->hwop.cntlddc(hdev,
			DDC_HDCP14_GET_BCAPS_RP, 0);

	pos += snprintf(buf + pos, PAGE_SIZE, "%d\n",
			hdev->hdcp_bcaps_repeater);

	return pos;
}

static ssize_t hdcp_repeater_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	if (hdev->hdcp_mode == 2)
		hdev->hdcp_bcaps_repeater = (buf[0] == '1');

	return count;
}

/*
 * hdcp_topo_info attr
 * For hdcp 22, hdcp_tx22 will write to hdcp_topo_info_store
 * For hdcp 14, directly get from HW
 */

static ssize_t hdcp_topo_info_show(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);
	struct hdcprp_topo *topoinfo = hdev->topo_info;

	if (!hdev->hdcp_mode) {
		pos += snprintf(buf + pos, PAGE_SIZE, "hdcp mode: 0\n");
		return pos;
	}
	if (!topoinfo)
		return pos;

	if (hdev->hdcp_mode == 1) {
		memset(topoinfo, 0, sizeof(struct hdcprp_topo));
		hdev->hwop.cntlddc(hdev, DDC_HDCP14_GET_TOPO_INFO,
			(unsigned long)&topoinfo->topo.topo14);
	}

	pos += snprintf(buf + pos, PAGE_SIZE, "hdcp mode: %s\n",
		hdev->hdcp_mode == 1 ? "14" : "22");
	if (hdev->hdcp_mode == 2) {
		topoinfo->hdcp_ver = HDCPVER_22;
		pos += snprintf(buf + pos, PAGE_SIZE, "max_devs_exceeded: %d\n",
			topoinfo->topo.topo22.max_devs_exceeded);
		pos += snprintf(buf + pos, PAGE_SIZE,
			"max_cascade_exceeded: %d\n",
			topoinfo->topo.topo22.max_cascade_exceeded);
		pos += snprintf(buf + pos, PAGE_SIZE,
				"v2_0_repeater_down: %d\n",
			topoinfo->topo.topo22.v2_0_repeater_down);
		pos += snprintf(buf + pos, PAGE_SIZE, "v1_X_device_down: %d\n",
			topoinfo->topo.topo22.v1_X_device_down);
		pos += snprintf(buf + pos, PAGE_SIZE, "device_count: %d\n",
			topoinfo->topo.topo22.device_count);
		pos += snprintf(buf + pos, PAGE_SIZE, "depth: %d\n",
			topoinfo->topo.topo22.depth);
		return pos;
	}
	if (hdev->hdcp_mode == 1) {
		topoinfo->hdcp_ver = HDCPVER_14;
		pos += snprintf(buf + pos, PAGE_SIZE, "max_devs_exceeded: %d\n",
			topoinfo->topo.topo14.max_devs_exceeded);
		pos += snprintf(buf + pos, PAGE_SIZE,
			"max_cascade_exceeded: %d\n",
			topoinfo->topo.topo14.max_cascade_exceeded);
		pos += snprintf(buf + pos, PAGE_SIZE, "device_count: %d\n",
			topoinfo->topo.topo14.device_count);
		pos += snprintf(buf + pos, PAGE_SIZE, "depth: %d\n",
			topoinfo->topo.topo14.depth);
		return pos;
	}

	return pos;
}

static ssize_t hdcp_topo_info_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);
	struct hdcprp_topo *topoinfo = hdev->topo_info;
	int cnt;

	if (!topoinfo)
		return count;

	if (hdev->hdcp_mode == 2) {
		memset(topoinfo, 0, sizeof(struct hdcprp_topo));
		cnt = sscanf(buf, "%x %x %x %x %x %x",
			     (int *)&topoinfo->topo.topo22.max_devs_exceeded,
			     (int *)&topoinfo->topo.topo22.max_cascade_exceeded,
			     (int *)&topoinfo->topo.topo22.v2_0_repeater_down,
			     (int *)&topoinfo->topo.topo22.v1_X_device_down,
			     (int *)&topoinfo->topo.topo22.device_count,
			     (int *)&topoinfo->topo.topo22.depth);
		if (cnt < 0)
			return count;
	}

	return count;
}

static ssize_t hdcp22_type_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	pos +=
	snprintf(buf + pos, PAGE_SIZE, "%d\n", hdev->hdcp22_type);

	return pos;
}

static ssize_t hdcp22_type_store(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	int type = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	if (buf[0] == '1')
		type = 1;
	else
		type = 0;
	hdev->hdcp22_type = type;

	pr_info("hdmitx: set hdcp22 content type %d\n", type);
	hdev->hwop.cntlddc(hdev, DDC_HDCP_SET_TOPO_INFO, type);

	return count;
}

static ssize_t hdcp22_base_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	int pos = 0;

	pos += snprintf(buf + pos, PAGE_SIZE, "0x%x\n", get_hdcp22_base());

	return pos;
}

void hdmitx20_audio_mute_op(unsigned int flag)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();

	if (hdev->hdmi_init != 1)
		return;

	hdev->tx_aud_cfg = flag;
	if (flag == 0)
		hdev->tx_hw.cntlconfig(&hdev->tx_hw,
			CONF_AUDIO_MUTE_OP, AUDIO_MUTE);
	else
		hdev->tx_hw.cntlconfig(&hdev->tx_hw,
			CONF_AUDIO_MUTE_OP, AUDIO_UNMUTE);
}

void hdmitx20_video_mute_op(unsigned int flag)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();

	if (hdev->hdmi_init != 1)
		return;

	if (flag == 0) {
		/* tx_dev->tx_hw.cntlconfig(&hdmitx_device.tx_hw, */
			/* CONF_VIDEO_MUTE_OP, VIDEO_MUTE); */
		hdev->vid_mute_op = VIDEO_MUTE;
	} else {
		/* tx_dev->tx_hw.cntlconfig(&hdmitx_device.tx_hw, */
			/* CONF_VIDEO_MUTE_OP, VIDEO_UNMUTE); */
		hdev->vid_mute_op = VIDEO_UNMUTE;
	}
}

/*
 *  SDR/HDR uevent
 *  1: SDR to HDR
 *  0: HDR to SDR
 */
static void hdmitx_sdr_hdr_uevent(struct hdmitx_dev *hdev)
{
	if (hdev->hdmi_current_hdr_mode != 0) {
		/* SDR -> HDR*/
		hdmitx_set_uevent(HDMITX_HDR_EVENT, 1);
	} else if (hdev->hdmi_current_hdr_mode == 0) {
		/* HDR -> SDR*/
		hdmitx_set_uevent(HDMITX_HDR_EVENT, 0);
	}
}

static unsigned int hdmitx_get_frame_duration(void)
{
	unsigned int frame_duration;
	struct vinfo_s *vinfo = hdmitx_get_current_vinfo(NULL);

	if (!vinfo || !vinfo->sync_duration_num)
		return 0;

	frame_duration =
		1000000 * vinfo->sync_duration_den / vinfo->sync_duration_num;
	return frame_duration;
}

static int hdmitx_check_valid_aspect_ratio(enum hdmi_vic vic, int aspect_ratio)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();

	switch (vic) {
	case HDMI_2_720x480p60_4x3:
		if (hdmitx_edid_validate_mode(&hdev->tx_comm.rxcap,
			HDMI_3_720x480p60_16x9) == 0) {
			if (aspect_ratio == AR_16X9)
				return 1;
			pr_info("same aspect_ratio = %d\n", aspect_ratio);
		} else {
			pr_info("TV not support dual aspect_ratio\n");
		}
		break;
	case HDMI_3_720x480p60_16x9:
		if (hdmitx_edid_validate_mode(&hdev->tx_comm.rxcap,
			HDMI_2_720x480p60_4x3) == 0) {
			if (aspect_ratio == AR_4X3)
				return 1;
			pr_info("same aspect_ratio = %d\n", aspect_ratio);
		} else {
			pr_info("TV not support dual aspect_ratio\n");
		}
		break;
	case HDMI_17_720x576p50_4x3:
		if (hdmitx_edid_validate_mode(&hdev->tx_comm.rxcap,
			HDMI_18_720x576p50_16x9) == 0) {
			if (aspect_ratio == AR_16X9)
				return 1;
			pr_info("same aspect_ratio = %d\n", aspect_ratio);
		} else {
			pr_info("TV not support dual aspect_ratio\n");
		}
		break;
	case HDMI_18_720x576p50_16x9:
		if (hdmitx_edid_validate_mode(&hdev->tx_comm.rxcap,
			HDMI_17_720x576p50_4x3) == 0) {
			if (aspect_ratio == AR_4X3)
				return 1;
			pr_info("same aspect_ratio = %d\n", aspect_ratio);
		} else {
			pr_info("TV not support dual aspect_ratio\n");
		}
		break;
	default:
		break;
	}
	pr_info("not support vic = %d\n", vic);
	return 0;
}

int hdmitx_get_aspect_ratio(void)
{
	enum hdmi_vic vic = HDMI_0_UNKNOWN;
	int x, y;
	struct hdmitx_dev *hdev = get_hdmitx_device();

	vic = hdev->tx_hw.getstate(&hdev->tx_hw, STAT_VIDEO_VIC, 0);

	if (vic == HDMI_2_720x480p60_4x3 || vic == HDMI_17_720x576p50_4x3)
		return AR_4X3;
	if (vic == HDMI_3_720x480p60_16x9 || vic == HDMI_18_720x576p50_16x9)
		return AR_16X9;

	struct vinfo_s *info = NULL;

	info = hdmitx_get_current_vinfo(NULL);
	x = info->aspect_ratio_num;
	y = info->aspect_ratio_den;
	if (x == 4 && y == 3)
		return AR_4X3;
	if (x == 16 && y == 9)
		return AR_16X9;

	return 0;
}

struct aspect_ratio_list *hdmitx_get_support_ar_list(void)
{
	static struct aspect_ratio_list ar_list[4];
	int i = 0;
	struct hdmitx_dev *hdev = get_hdmitx_device();
	struct rx_cap *prxcap = &hdev->tx_comm.rxcap;

	memset(ar_list, 0, sizeof(ar_list));
	if (hdmitx_edid_validate_mode(prxcap, HDMI_2_720x480p60_4x3) == 0 &&
			hdmitx_edid_validate_mode(prxcap, HDMI_3_720x480p60_16x9) == 0) {
		ar_list[i].vic = HDMI_2_720x480p60_4x3;
		ar_list[i].flag = TRUE;
		ar_list[i].aspect_ratio_num = 4;
		ar_list[i].aspect_ratio_den = 3;
		i++;

		ar_list[i].vic = HDMI_3_720x480p60_16x9;
		ar_list[i].flag = TRUE;
		ar_list[i].aspect_ratio_num = 16;
		ar_list[i].aspect_ratio_den = 9;
		i++;
	}
	if (hdmitx_edid_validate_mode(prxcap, HDMI_17_720x576p50_4x3) == 0 &&
			hdmitx_edid_validate_mode(prxcap, HDMI_18_720x576p50_16x9) == 0) {
		ar_list[i].vic = HDMI_17_720x576p50_4x3;
		ar_list[i].flag = TRUE;
		ar_list[i].aspect_ratio_num = 4;
		ar_list[i].aspect_ratio_den = 3;
		i++;

		ar_list[i].vic = HDMI_18_720x576p50_16x9;
		ar_list[i].flag = TRUE;
		ar_list[i].aspect_ratio_num = 16;
		ar_list[i].aspect_ratio_den = 9;
		i++;
	}
	return &ar_list[0];
}

void hdmitx_set_aspect_ratio(int aspect_ratio)
{
	enum hdmi_vic vic = HDMI_0_UNKNOWN;
	struct hdmitx_dev *hdev = get_hdmitx_device();
	int ret;
	int aspect_ratio_vic = 0;

	if (aspect_ratio != AR_4X3 && aspect_ratio != AR_16X9) {
		pr_info("aspect ratio should be 1 or 2");
		return;
	}

	vic = hdev->tx_hw.getstate(&hdev->tx_hw, STAT_VIDEO_VIC, 0);
	ret = hdmitx_check_valid_aspect_ratio(vic, aspect_ratio);
	pr_info("%s vic = %d, ret = %d\n", __func__, vic, ret);

	if (!ret)
		return;

	switch (vic) {
	case HDMI_2_720x480p60_4x3:
		aspect_ratio_vic = (HDMI_3_720x480p60_16x9 << 2) + aspect_ratio;
		break;
	case HDMI_3_720x480p60_16x9:
		aspect_ratio_vic = (HDMI_2_720x480p60_4x3 << 2) + aspect_ratio;
		break;
	case HDMI_17_720x576p50_4x3:
		aspect_ratio_vic = (HDMI_18_720x576p50_16x9 << 2) + aspect_ratio;
		break;
	case HDMI_18_720x576p50_16x9:
		aspect_ratio_vic = (HDMI_17_720x576p50_4x3 << 2) + aspect_ratio;
		break;
	default:
		break;
	}

	hdev->tx_hw.cntlconfig(&hdev->tx_hw, CONF_ASPECT_RATIO, aspect_ratio_vic);
	hdev->aspect_ratio = aspect_ratio;
	pr_info("set new aspect ratio = %d\n", aspect_ratio);
}

static void hdr_unmute_work_func(struct work_struct *work)
{
	unsigned int mute_us;

	if (hdr_mute_frame) {
		pr_info("vid mute %d frames before play hdr/hlg video\n",
			hdr_mute_frame);
		mute_us = hdr_mute_frame * hdmitx_get_frame_duration();
		usleep_range(mute_us, mute_us + 10);
		hdmitx_video_mute_op(1);
	}
}

static void hdr_work_func(struct work_struct *work)
{
	struct hdmitx_dev *hdev =
		container_of(work, struct hdmitx_dev, work_hdr);

	if (hdev->hdr_transfer_feature == T_BT709 &&
	    hdev->hdr_color_feature == C_BT709) {
		unsigned char DRM_HB[3] = {0x87, 0x1, 26};
		unsigned char DRM_DB[26] = {0x0};

		pr_info("%s: send zero DRM\n", __func__);
		hdev->hwop.setpacket(HDMI_PACKET_DRM, DRM_DB, DRM_HB);

		msleep(1500);/*delay 1.5s*/
		/* disable DRM packets completely ONLY if hdr transfer
		 * feature and color feature still demand SDR.
		 */
		if (hdr_status_pos == 4) {
			/* zero hdr10+ VSIF being sent - disable it */
			pr_info("%s: disable hdr10+ vsif\n", __func__);
			hdev->hwop.setpacket(HDMI_PACKET_VEND, NULL, NULL);
			hdr_status_pos = 0;
		}
		if (hdev->hdr_transfer_feature == T_BT709 &&
		    hdev->hdr_color_feature == C_BT709) {
			pr_info("%s: disable DRM\n", __func__);
			hdev->hwop.setpacket(HDMI_PACKET_DRM, NULL, NULL);
			hdev->hdmi_current_hdr_mode = 0;
			hdmitx_sdr_hdr_uevent(hdev);
		} else {
			pr_info("%s: tf=%d, cf=%d\n",
				__func__,
				hdev->hdr_transfer_feature,
				hdev->hdr_color_feature);
		}
	} else {
		hdmitx_sdr_hdr_uevent(hdev);
	}
}

#define hdmi_debug() \
	do { \
		if (log_level == 0xff) \
			pr_info("%s[%d]\n", __func__, __LINE__); \
	} while (0)

/* Init DRM_DB[0] from Uboot status */
static void init_drm_db0(struct hdmitx_dev *hdev, unsigned char *dat)
{
	static int once_flag = 1;

	if (once_flag) {
		once_flag = 0;
		*dat = hdev->tx_hw.getstate(&hdev->tx_hw, STAT_HDR_TYPE, 0);
	}
}

static bool hdmitx_dv_en(void)
{
	return (hdmitx_get_cur_dv_st() & HDMI_DV_TYPE) == HDMI_DV_TYPE;
}

#define GET_LOW8BIT(a)	((a) & 0xff)
#define GET_HIGH8BIT(a)	(((a) >> 8) & 0xff)
struct master_display_info_s hsty_drm_config_data[8];
unsigned int hsty_drm_config_loc, hsty_drm_config_num;
struct master_display_info_s drm_config_data;
static void hdmitx_set_drm_pkt(struct master_display_info_s *data)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();
	struct hdr_info *hdr_info = &hdev->tx_comm.rxcap.hdr_info;
	unsigned char DRM_HB[3] = {0x87, 0x1, 26};
	static unsigned char DRM_DB[26] = {0x0};
	unsigned long flags = 0;

	hdmi_debug();
	spin_lock_irqsave(&hdev->edid_spinlock, flags);
	if (data)
		memcpy(&drm_config_data, data,
		       sizeof(struct master_display_info_s));
	else
		memset(&drm_config_data, 0,
		       sizeof(struct master_display_info_s));
	if (hsty_drm_config_loc > 7)
		hsty_drm_config_loc = 0;
	memcpy(&hsty_drm_config_data[hsty_drm_config_loc++],
	       &drm_config_data, sizeof(struct master_display_info_s));
	if (hsty_drm_config_num < 0xfffffff0)
		hsty_drm_config_num++;
	else
		hsty_drm_config_num = 8;

	init_drm_db0(hdev, &DRM_DB[0]);
	if (hdr_status_pos == 4) {
		/* zero hdr10+ VSIF being sent - disable it */
		pr_info("hdmitx_set_drm_pkt: disable hdr10+ zero vsif\n");
		hdev->hwop.setpacket(HDMI_PACKET_VEND, NULL, NULL);
		hdr_status_pos = 0;
	}

	/*
	 *hdr_color_feature: bit 23-16: color_primaries
	 *	1:bt709  0x9:bt2020
	 *hdr_transfer_feature: bit 15-8: transfer_characteristic
	 *	1:bt709 0xe:bt2020-10 0x10:smpte-st-2084 0x12:hlg(todo)
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
			pr_info("%s: tf=%d, cf=%d, colormetry=%d\n",
				__func__,
				hdev->hdr_transfer_feature,
				hdev->hdr_color_feature,
				hdev->colormetry);
		}
	} else {
		pr_info("%s: disable drm pkt\n", __func__);
	}

	hdr_status_pos = 1;
	/* if VSIF/DV or VSIF/HDR10P packet is enabled, disable it */
	if (hdmitx_dv_en()) {
		hdev->tx_hw.cntlconfig(&hdev->tx_hw, CONF_AVI_RGBYCC_INDIC,
			hdev->tx_comm.fmt_para.cs);
/* if using VSIF/DOVI, then only clear DV_VS10_SIG, else disable VSIF */
		if (hdev->tx_hw.cntlconfig(&hdev->tx_hw, CONF_CLR_DV_VS10_SIG, 0) == 0)
			hdev->hwop.setpacket(HDMI_PACKET_VEND, NULL, NULL);
	}

	/* hdr10+ content on a hdr10 sink case */
	if (hdev->hdr_transfer_feature == 0x30) {
		if (hdr_info->hdr10plus_info.ieeeoui != 0x90848B ||
		    hdr_info->hdr10plus_info.application_version != 1) {
			hdev->hdr_transfer_feature = T_SMPTE_ST_2084;
			pr_info("%s: HDR10+ not supported, treat as hdr10\n",
				__func__);
		}
	}

	if (!data || !hdev->tx_comm.rxcap.hdr_info2.hdr_support) {
		DRM_HB[1] = 0;
		DRM_HB[2] = 0;
		DRM_DB[0] = 0;
		hdev->colormetry = 0;
		hdev->hwop.setpacket(HDMI_PACKET_DRM, NULL, NULL);
		hdev->tx_hw.cntlconfig(&hdev->tx_hw,
			CONF_AVI_BT2020, hdev->colormetry);
		spin_unlock_irqrestore(&hdev->edid_spinlock, flags);
		return;
	}

	/*SDR*/
	if (hdev->hdr_transfer_feature == T_BT709 &&
		hdev->hdr_color_feature == C_BT709) {
		/* send zero drm only for HDR->SDR transition */
		if (DRM_DB[0] == 0x02 || DRM_DB[0] == 0x03) {
			pr_info("%s: HDR->SDR, DRM_DB[0]=%d\n",
				__func__, DRM_DB[0]);
			hdev->colormetry = 0;
			hdev->tx_hw.cntlconfig(&hdev->tx_hw, CONF_AVI_BT2020, 0);
			schedule_work(&hdev->work_hdr);
			DRM_DB[0] = 0;
		}
		/* back to previous cs */
		/* currently output y444,8bit or rgb,8bit, if exit playing,
		 * then switch back to 8bit mode
		 */
		if (hdev->tx_comm.fmt_para.cs == HDMI_COLORSPACE_YUV444 &&
			hdev->tx_comm.fmt_para.cd == COLORDEPTH_24B) {
			/* hdev->hwop.cntlconfig(hdev, */
					/* CONF_AVI_RGBYCC_INDIC, */
					/* COLORSPACE_YUV444); */
			hdev->tx_hw.cntlconfig(&hdev->tx_hw, CONFIG_CSC,
				CSC_Y444_8BIT | CSC_UPDATE_AVI_CS);
			pr_info("%s: switch back to cs:%d, cd:%d\n",
				__func__, hdev->tx_comm.fmt_para.cs,
				hdev->tx_comm.fmt_para.cd);
		} else if (hdev->tx_comm.fmt_para.cs == HDMI_COLORSPACE_RGB &&
			hdev->tx_comm.fmt_para.cd == COLORDEPTH_24B) {
			/* hdev->hwop.cntlconfig(hdev, */
					/* CONF_AVI_RGBYCC_INDIC, */
					/* COLORSPACE_RGB444); */
			hdev->tx_hw.cntlconfig(&hdev->tx_hw, CONFIG_CSC,
				CSC_RGB_8BIT | CSC_UPDATE_AVI_CS);
			pr_info("%s: switch back to cs:%d, cd:%d\n",
				__func__, hdev->tx_comm.fmt_para.cs,
				hdev->tx_comm.fmt_para.cd);
		}
		spin_unlock_irqrestore(&hdev->edid_spinlock, flags);
		return;
	}

	DRM_DB[1] = 0x0;
	DRM_DB[2] = GET_LOW8BIT(data->primaries[0][0]);
	DRM_DB[3] = GET_HIGH8BIT(data->primaries[0][0]);
	DRM_DB[4] = GET_LOW8BIT(data->primaries[0][1]);
	DRM_DB[5] = GET_HIGH8BIT(data->primaries[0][1]);
	DRM_DB[6] = GET_LOW8BIT(data->primaries[1][0]);
	DRM_DB[7] = GET_HIGH8BIT(data->primaries[1][0]);
	DRM_DB[8] = GET_LOW8BIT(data->primaries[1][1]);
	DRM_DB[9] = GET_HIGH8BIT(data->primaries[1][1]);
	DRM_DB[10] = GET_LOW8BIT(data->primaries[2][0]);
	DRM_DB[11] = GET_HIGH8BIT(data->primaries[2][0]);
	DRM_DB[12] = GET_LOW8BIT(data->primaries[2][1]);
	DRM_DB[13] = GET_HIGH8BIT(data->primaries[2][1]);
	DRM_DB[14] = GET_LOW8BIT(data->white_point[0]);
	DRM_DB[15] = GET_HIGH8BIT(data->white_point[0]);
	DRM_DB[16] = GET_LOW8BIT(data->white_point[1]);
	DRM_DB[17] = GET_HIGH8BIT(data->white_point[1]);
	DRM_DB[18] = GET_LOW8BIT(data->luminance[0]);
	DRM_DB[19] = GET_HIGH8BIT(data->luminance[0]);
	DRM_DB[20] = GET_LOW8BIT(data->luminance[1]);
	DRM_DB[21] = GET_HIGH8BIT(data->luminance[1]);
	DRM_DB[22] = GET_LOW8BIT(data->max_content);
	DRM_DB[23] = GET_HIGH8BIT(data->max_content);
	DRM_DB[24] = GET_LOW8BIT(data->max_frame_average);
	DRM_DB[25] = GET_HIGH8BIT(data->max_frame_average);

	/* bt2020 + gamma transfer */
	if (hdev->hdr_transfer_feature == T_BT709 &&
	    hdev->hdr_color_feature == C_BT2020) {
		if (hdev->sdr_hdr_feature == 0) {
			hdev->hwop.setpacket(HDMI_PACKET_DRM,
				NULL, NULL);
			hdev->tx_hw.cntlconfig(&hdev->tx_hw,
				CONF_AVI_BT2020, SET_AVI_BT2020);
		} else if (hdev->sdr_hdr_feature == 1) {
			memset(DRM_DB, 0, sizeof(DRM_DB));
			hdev->hwop.setpacket(HDMI_PACKET_DRM,
				DRM_DB, DRM_HB);
			hdev->tx_hw.cntlconfig(&hdev->tx_hw,
				CONF_AVI_BT2020, SET_AVI_BT2020);
		} else {
			DRM_DB[0] = 0x02; /* SMPTE ST 2084 */
			hdev->hwop.setpacket(HDMI_PACKET_DRM,
				DRM_DB, DRM_HB);
			hdev->tx_hw.cntlconfig(&hdev->tx_hw,
				CONF_AVI_BT2020, SET_AVI_BT2020);
		}
		spin_unlock_irqrestore(&hdev->edid_spinlock, flags);
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
		DRM_DB[0] = 0x02; /* SMPTE ST 2084 */
		hdev->hwop.setpacket(HDMI_PACKET_DRM,
			DRM_DB, DRM_HB);
		hdev->tx_hw.cntlconfig(&hdev->tx_hw,
			CONF_AVI_BT2020, SET_AVI_BT2020);
		break;
	case 2:
		/*non standard*/
		DRM_DB[0] = 0x02; /* no standard SMPTE ST 2084 */
		hdev->hwop.setpacket(HDMI_PACKET_DRM,
			DRM_DB, DRM_HB);
		hdev->tx_hw.cntlconfig(&hdev->tx_hw,
			CONF_AVI_BT2020, CLR_AVI_BT2020);
		break;
	case 3:
		/*HLG*/
		DRM_DB[0] = 0x03;/* HLG is 0x03 */
		hdev->hwop.setpacket(HDMI_PACKET_DRM,
			DRM_DB, DRM_HB);
		hdev->tx_hw.cntlconfig(&hdev->tx_hw,
			CONF_AVI_BT2020, SET_AVI_BT2020);
		break;
	case 0:
	default:
		/*other case*/
		hdev->hwop.setpacket(HDMI_PACKET_DRM, NULL, NULL);
		hdev->tx_hw.cntlconfig(&hdev->tx_hw,
			CONF_AVI_BT2020, CLR_AVI_BT2020);
		break;
	}

	/* if sdr/hdr mode change ,notify uevent to userspace*/
	if (hdev->hdmi_current_hdr_mode != hdev->hdmi_last_hdr_mode) {
		/* NOTE: for HDR <-> HLG, also need update last mode */
		hdev->hdmi_last_hdr_mode = hdev->hdmi_current_hdr_mode;
		if (hdr_mute_frame) {
			hdmitx_video_mute_op(0);
			pr_info("SDR->HDR enter mute\n");
			/* force unmute after specific frames,
			 * no need to check hdr status when unmute
			 */
			schedule_work(&hdev->work_hdr_unmute);
		}
		schedule_work(&hdev->work_hdr);
	}

	if (hdev->hdmi_current_hdr_mode == 1 ||
		hdev->hdmi_current_hdr_mode == 2 ||
		hdev->hdmi_current_hdr_mode == 3) {
		/* currently output y444,8bit or rgb,8bit, and EDID
		 * support Y422, then switch to y422,12bit mode
		 */
		if ((hdev->tx_comm.fmt_para.cs == HDMI_COLORSPACE_YUV444 ||
			hdev->tx_comm.fmt_para.cs == HDMI_COLORSPACE_RGB) &&
			hdev->tx_comm.fmt_para.cd == COLORDEPTH_24B &&
			(hdev->tx_comm.rxcap.native_Mode & (1 << 4))) {
			/* hdev->hwop.cntlconfig(hdev,*/
					/* CONF_AVI_RGBYCC_INDIC, */
					/* COLORSPACE_YUV422);*/
			hdev->tx_hw.cntlconfig(&hdev->tx_hw, CONFIG_CSC,
				CSC_Y422_12BIT | CSC_UPDATE_AVI_CS);
			pr_info("%s: switch to 422,12bit\n", __func__);
		}
	}
	spin_unlock_irqrestore(&hdev->edid_spinlock, flags);
}

static int calc_vinfo_from_hdmi_timing(const struct hdmi_timing *timing, struct vinfo_s *tx_vinfo)
{
	/* manually assign hdmitx_vinfo from timing */
	tx_vinfo->name = timing->sname ? timing->sname : timing->name;
	tx_vinfo->mode = VMODE_HDMI;
	tx_vinfo->frac = 0;
	if (timing->pixel_repetition_factor)
		tx_vinfo->width = timing->h_active >> 1;
	else
		tx_vinfo->width = timing->h_active;

	tx_vinfo->height  = timing->v_active;
	tx_vinfo->field_height = timing->pi_mode ? timing->v_active : (timing->v_active >> 1);

	tx_vinfo->aspect_ratio_num = timing->h_pict;
	tx_vinfo->aspect_ratio_den = timing->v_pict;

	if (timing->v_freq % 1000 == 0) {
		tx_vinfo->sync_duration_num = timing->v_freq / 1000;
		tx_vinfo->sync_duration_den = 1;
	} else {
		tx_vinfo->sync_duration_num = timing->v_freq;
		tx_vinfo->sync_duration_den = 1000;
	}

	tx_vinfo->video_clk = timing->pixel_freq;
	tx_vinfo->htotal = timing->h_total;
	tx_vinfo->vtotal = timing->v_total;
	tx_vinfo->fr_adj_type = VOUT_FR_ADJ_HDMI;
	tx_vinfo->viu_color_fmt = COLOR_FMT_YUV444;

	tx_vinfo->viu_mux = timing->pi_mode ? VIU_MUX_ENCP : VIU_MUX_ENCI;
	/* 1080i use the ENCP, not ENCI */
	if (timing->name && strstr(timing->name, "1080i"))
		tx_vinfo->viu_mux = VIU_MUX_ENCP;

	return 0;
}

static void update_vinfo_from_formatpara(void)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();
	struct vinfo_s *vinfo = &hdev->tx_comm.hdmitx_vinfo;
	struct hdmi_format_para *fmtpara = &hdev->tx_comm.fmt_para;

	/*update vinfo for out device.*/
	calc_vinfo_from_hdmi_timing(&fmtpara->timing, vinfo);
	vinfo->info_3d = NON_3D;
	if (hdev->flag_3dfp)
		vinfo->info_3d = FP_3D;
	if (hdev->flag_3dtb)
		vinfo->info_3d = TB_3D;
	if (hdev->flag_3dss)
		vinfo->info_3d = SS_3D;
	vinfo->vout_device = &hdmitx_vdev;
	/*dynamic info, always need set.*/
	vinfo->cs = fmtpara->cs;
	vinfo->cd = fmtpara->cd;
}

struct vsif_debug_save vsif_debug_info;
struct vsif_debug_save hsty_vsif_config_data[8];
unsigned int hsty_vsif_config_loc, hsty_vsif_config_num;
static void hdmitx_set_vsif_pkt(enum eotf_type type,
				enum mode_type tunnel_mode,
				struct dv_vsif_para *data,
				bool signal_sdr)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();
	struct hdmitx_common *tx_comm = &hdev->tx_comm;
	struct dv_vsif_para para = {0};
	unsigned char VEN_HB[3] = {0x81, 0x01};
	unsigned char VEN_DB1[24] = {0x00};
	unsigned char VEN_DB2[27] = {0x00};
	unsigned char len = 0;
	unsigned int vic = hdev->tx_comm.cur_VIC;
	unsigned int hdmi_vic_4k_flag = 0;
	static enum eotf_type ltype = EOTF_T_NULL;
	static u8 ltmode = -1;
	enum hdmi_tf_type hdr_type = HDMI_NONE;
	unsigned long flags = 0;

	hdmi_debug();
	spin_lock_irqsave(&hdev->edid_spinlock, flags);
	if (hdev->bist_lock) {
		spin_unlock_irqrestore(&hdev->edid_spinlock, flags);
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

	if (hsty_vsif_config_loc > 7)
		hsty_vsif_config_loc = 0;
	memcpy(&hsty_vsif_config_data[hsty_vsif_config_loc++],
	       &vsif_debug_info, sizeof(struct vsif_debug_save));
	if (hsty_vsif_config_num < 0xfffffff0)
		hsty_vsif_config_num++;
	else
		hsty_vsif_config_num = 8;

	if (hdev->ready == 0) {
		ltype = EOTF_T_NULL;
		ltmode = -1;
		spin_unlock_irqrestore(&hdev->edid_spinlock, flags);
		return;
	}
	if (hdev->tx_comm.rxcap.dv_info.ieeeoui != DV_IEEE_OUI) {
		if (type == 0 && !data && signal_sdr)
			pr_info("TV not support DV, clr dv_vsif\n");
	}

	if (hdev->data->chip_type < MESON_CPU_ID_GXL) {
		pr_info("hdmitx: not support DolbyVision\n");
		spin_unlock_irqrestore(&hdev->edid_spinlock, flags);
		return;
	}

	if (hdev->hdmi_current_eotf_type != type ||
		hdev->hdmi_current_tunnel_mode != tunnel_mode ||
		hdev->hdmi_current_signal_sdr != signal_sdr) {
		hdev->hdmi_current_eotf_type = type;
		hdev->hdmi_current_tunnel_mode = tunnel_mode;
		hdev->hdmi_current_signal_sdr = signal_sdr;
		pr_info("%s: type=%d, tunnel_mode=%d, signal_sdr=%d\n",
			__func__, type, tunnel_mode, signal_sdr);
	}
	hdr_status_pos = 2;

	/* if DRM/HDR packet is enabled, disable it */
	hdr_type = hdmitx_get_cur_hdr_st();
	if (hdr_type != HDMI_NONE && hdr_type != HDMI_HDR_SDR) {
		hdev->hdr_transfer_feature = T_BT709;
		hdev->hdr_color_feature = C_BT709;
		hdev->colormetry = 0;
		hdev->tx_hw.cntlconfig(&hdev->tx_hw, CONF_AVI_BT2020, hdev->colormetry);
		schedule_work(&hdev->work_hdr);
	}

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

		VEN_HB[2] = len;
		VEN_DB1[0] = 0x03;
		VEN_DB1[1] = 0x0c;
		VEN_DB1[2] = 0x00;
		VEN_DB1[3] = 0x00;

		if (hdmi_vic_4k_flag) {
			VEN_DB1[3] = 0x20;
			if (vic == HDMI_95_3840x2160p30_16x9)
				VEN_DB1[4] = 0x1;
			else if (vic == HDMI_94_3840x2160p25_16x9)
				VEN_DB1[4] = 0x2;
			else if (vic == HDMI_93_3840x2160p24_16x9)
				VEN_DB1[4] = 0x3;
			else/*vic == HDMI_98_4096x2160p24_256x135*/
				VEN_DB1[4] = 0x4;
		}
		if (type == EOTF_T_DV_AHEAD) {
			hdev->hwop.setpacket(HDMI_PACKET_VEND, VEN_DB1, VEN_HB);
			spin_unlock_irqrestore(&hdev->edid_spinlock, flags);
			return;
		}
		if (type == EOTF_T_DOLBYVISION) {
			/*first disable drm package*/
			hdev->hwop.setpacket(HDMI_PACKET_DRM,
				NULL, NULL);
			hdev->hwop.setpacket(HDMI_PACKET_VEND, VEN_DB1, VEN_HB);
			/* Dolby Vision Source System-on-Chip Platform Kit Version 2.6:
			 * 4.4.1 Expected AVI-IF for Dolby Vision output, need BT2020 for DV
			 */
			hdev->tx_hw.cntlconfig(&hdev->tx_hw, CONF_AVI_BT2020,
				SET_AVI_BT2020);/*BT.2020*/
			if (tunnel_mode == RGB_8BIT) {
				hdev->tx_hw.cntlconfig(&hdev->tx_hw,
					CONF_AVI_RGBYCC_INDIC,
					HDMI_COLORSPACE_RGB);
				hdev->tx_hw.cntlconfig(&hdev->tx_hw, CONF_AVI_Q01,
					RGB_RANGE_FUL);
				/* to test, if needed */
				/* hdev->hwop.cntlconfig(hdev, CONFIG_CSC, CSC_Y444_8BIT); */
				/* if (log_level == 0xfd) */
					/* pr_info("hdmitx: Dolby H14b VSIF, */
					/* switch to y444 csc\n"); */
			} else {
				hdev->tx_hw.cntlconfig(&hdev->tx_hw,
					CONF_AVI_RGBYCC_INDIC,
					HDMI_COLORSPACE_YUV422);
				hdev->tx_hw.cntlconfig(&hdev->tx_hw, CONF_AVI_YQ01,
					YCC_RANGE_FUL);
			}
		} else {
			if (hdmi_vic_4k_flag)
				hdev->hwop.setpacket(HDMI_PACKET_VEND,
						     VEN_DB1, VEN_HB);
			else
				hdev->hwop.setpacket(HDMI_PACKET_VEND,
						     NULL, NULL);
			if (signal_sdr) {
				pr_info("hdmitx: H14b VSIF, switching signal to SDR\n");
				hdev->tx_hw.cntlconfig(&hdev->tx_hw,
					CONF_AVI_RGBYCC_INDIC, tx_comm->fmt_para.cs);
				hdev->tx_hw.cntlconfig(&hdev->tx_hw,
					CONF_AVI_Q01, RGB_RANGE_LIM);
				hdev->tx_hw.cntlconfig(&hdev->tx_hw,
					CONF_AVI_YQ01, YCC_RANGE_LIM);
				hdev->tx_hw.cntlconfig(&hdev->tx_hw, CONF_AVI_BT2020,
					CLR_AVI_BT2020);/*BT709*/
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
		VEN_HB[2] = len;
		VEN_DB2[0] = 0x46;
		VEN_DB2[1] = 0xd0;
		VEN_DB2[2] = 0x00;
		if (data->ver2_l11_flag == 1) {
			VEN_DB2[3] = data->vers.ver2_l11.low_latency |
				     data->vers.ver2_l11.dobly_vision_signal << 1 |
				     data->vers.ver2_l11.src_dm_version << 5;
			VEN_DB2[4] = data->vers.ver2_l11.eff_tmax_PQ_hi
				     | data->vers.ver2_l11.auxiliary_MD_present << 6
				     | data->vers.ver2_l11.backlt_ctrl_MD_present << 7
				     | 0x20; /*L11_MD_Present*/
			VEN_DB2[5] = data->vers.ver2_l11.eff_tmax_PQ_low;
			VEN_DB2[6] = data->vers.ver2_l11.auxiliary_runmode;
			VEN_DB2[7] = data->vers.ver2_l11.auxiliary_runversion;
			VEN_DB2[8] = data->vers.ver2_l11.auxiliary_debug0;
			VEN_DB2[9] = (data->vers.ver2_l11.content_type)
				| (data->vers.ver2_l11.content_sub_type << 4);
			VEN_DB2[10] = (data->vers.ver2_l11.intended_white_point)
				| (data->vers.ver2_l11.crf << 4);
			VEN_DB2[11] = data->vers.ver2_l11.l11_byte2;
			VEN_DB2[12] = data->vers.ver2_l11.l11_byte3;
		} else {
			VEN_DB2[3] = (data->vers.ver2.low_latency) |
				(data->vers.ver2.dobly_vision_signal << 1) |
				(data->vers.ver2.src_dm_version << 5);
			VEN_DB2[4] = (data->vers.ver2.eff_tmax_PQ_hi)
				| (data->vers.ver2.auxiliary_MD_present << 6)
				| (data->vers.ver2.backlt_ctrl_MD_present << 7);
			VEN_DB2[5] = data->vers.ver2.eff_tmax_PQ_low;
			VEN_DB2[6] = data->vers.ver2.auxiliary_runmode;
			VEN_DB2[7] = data->vers.ver2.auxiliary_runversion;
			VEN_DB2[8] = data->vers.ver2.auxiliary_debug0;
		}
		if (type == EOTF_T_DV_AHEAD) {
			hdev->hwop.setpacket(HDMI_PACKET_VEND, VEN_DB2, VEN_HB);
			spin_unlock_irqrestore(&hdev->edid_spinlock, flags);
			return;
		}
		/*Dolby Vision standard case*/
		if (type == EOTF_T_DOLBYVISION) {
			/*first disable drm package*/
			hdev->hwop.setpacket(HDMI_PACKET_DRM,
				NULL, NULL);
			hdev->hwop.setpacket(HDMI_PACKET_VEND, VEN_DB2, VEN_HB);
			/* Dolby Vision Source System-on-Chip Platform Kit Version 2.6:
			 * 4.4.1 Expected AVI-IF for Dolby Vision output, need BT2020 for DV
			 */
			hdev->tx_hw.cntlconfig(&hdev->tx_hw, CONF_AVI_BT2020,
				SET_AVI_BT2020);/*BT.2020*/
			if (tunnel_mode == RGB_8BIT) {/*RGB444*/
				hdev->tx_hw.cntlconfig(&hdev->tx_hw,
					CONF_AVI_RGBYCC_INDIC,
					HDMI_COLORSPACE_RGB);
				hdev->tx_hw.cntlconfig(&hdev->tx_hw, CONF_AVI_Q01,
					RGB_RANGE_FUL);
				/* to test, if needed */
				/* hdev->hwop.cntlconfig(hdev, CONFIG_CSC, CSC_Y444_8BIT); */
				/* if (log_level == 0xfd) */
					/* pr_info("hdmitx: Dolby STD, switch to y444 csc\n"); */
			} else {/*YUV422*/
				hdev->tx_hw.cntlconfig(&hdev->tx_hw,
					CONF_AVI_RGBYCC_INDIC,
					HDMI_COLORSPACE_YUV422);
				hdev->tx_hw.cntlconfig(&hdev->tx_hw, CONF_AVI_YQ01,
					YCC_RANGE_FUL);
			}
		}
		/*Dolby Vision low-latency case*/
		else if  (type == EOTF_T_LL_MODE) {
			/*first disable drm package*/
			hdev->hwop.setpacket(HDMI_PACKET_DRM,
				NULL, NULL);
			hdev->hwop.setpacket(HDMI_PACKET_VEND, VEN_DB2, VEN_HB);
			/* Dolby vision HDMI Signaling Case25,
			 * UCD323 not declare bt2020 colorimetry,
			 * need to forcely send BT.2020
			 */
			hdev->tx_hw.cntlconfig(&hdev->tx_hw, CONF_AVI_BT2020,
				SET_AVI_BT2020);/*BT2020*/
			if (tunnel_mode == RGB_10_12BIT) {/*10/12bit RGB444*/
				hdev->tx_hw.cntlconfig(&hdev->tx_hw,
					CONF_AVI_RGBYCC_INDIC,
					HDMI_COLORSPACE_RGB);
				hdev->tx_hw.cntlconfig(&hdev->tx_hw, CONF_AVI_Q01,
					RGB_RANGE_LIM);
			} else if (tunnel_mode == YUV444_10_12BIT) {
				/*10/12bit YUV444*/
				hdev->tx_hw.cntlconfig(&hdev->tx_hw,
					CONF_AVI_RGBYCC_INDIC,
					HDMI_COLORSPACE_YUV444);
				hdev->tx_hw.cntlconfig(&hdev->tx_hw, CONF_AVI_YQ01,
					YCC_RANGE_LIM);
			} else {/*YUV422*/
				hdev->tx_hw.cntlconfig(&hdev->tx_hw,
					CONF_AVI_RGBYCC_INDIC,
					HDMI_COLORSPACE_YUV422);
				hdev->tx_hw.cntlconfig(&hdev->tx_hw, CONF_AVI_YQ01,
					YCC_RANGE_LIM);
			}
		} else { /*SDR case*/
			pr_info("hdmitx: Dolby VSIF, VEN_DB2[3]) = %d\n",
				VEN_DB2[3]);
			hdev->hwop.setpacket(HDMI_PACKET_VEND, VEN_DB2, VEN_HB);
			if (signal_sdr) {
				pr_info("hdmitx: Dolby VSIF, switching signal to SDR\n");
				pr_info("vic:%d, cd:%d, cs:%d, cr:%d\n",
					tx_comm->fmt_para.timing.vic, tx_comm->fmt_para.cd,
					tx_comm->fmt_para.cs, tx_comm->fmt_para.cr);
				hdev->tx_hw.cntlconfig(&hdev->tx_hw,
					CONF_AVI_RGBYCC_INDIC, tx_comm->fmt_para.cs);
				hdev->tx_hw.cntlconfig(&hdev->tx_hw,
					CONF_AVI_Q01, RGB_RANGE_DEFAULT);
				hdev->tx_hw.cntlconfig(&hdev->tx_hw,
					CONF_AVI_YQ01, YCC_RANGE_LIM);
				hdev->tx_hw.cntlconfig(&hdev->tx_hw, CONF_AVI_BT2020,
					CLR_AVI_BT2020);/*BT709*/
			}
		}
	}
	spin_unlock_irqrestore(&hdev->edid_spinlock, flags);
}

struct hdr10plus_para hdr10p_config_data;
struct hdr10plus_para hsty_hdr10p_config_data[8];
unsigned int hsty_hdr10p_config_loc, hsty_hdr10p_config_num;
static void hdmitx_set_hdr10plus_pkt(unsigned int flag,
	struct hdr10plus_para *data)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();
	unsigned char VEN_HB[3] = {0x81, 0x01, 0x1b};
	unsigned char VEN_DB[27] = {0x00};

	hdmi_debug();
	if (hdev->bist_lock)
		return;
	if (data)
		memcpy(&hdr10p_config_data, data,
		       sizeof(struct hdr10plus_para));
	else
		memset(&hdr10p_config_data, 0,
		       sizeof(struct hdr10plus_para));
	if (hsty_hdr10p_config_loc > 7)
		hsty_hdr10p_config_loc = 0;
	memcpy(&hsty_hdr10p_config_data[hsty_hdr10p_config_loc++],
	       &hdr10p_config_data, sizeof(struct hdr10plus_para));
	if (hsty_hdr10p_config_num < 0xfffffff0)
		hsty_hdr10p_config_num++;
	else
		hsty_hdr10p_config_num = 8;

	if (flag == HDR10_PLUS_ZERO_VSIF) {
		/* needed during hdr10+ to sdr transition */
		pr_info("hdmitx_set_hdr10plus_pkt: zero vsif\n");
		hdev->hwop.setpacket(HDMI_PACKET_VEND, VEN_DB, VEN_HB);
		hdev->tx_hw.cntlconfig(&hdev->tx_hw, CONF_AVI_BT2020,
			CLR_AVI_BT2020);
		hdev->hdr10plus_feature = 0;
		hdr_status_pos = 4;
		return;
	}

	if (!data || !flag) {
		pr_info("hdmitx_set_hdr10plus_pkt: null vsif\n");
		hdev->hwop.setpacket(HDMI_PACKET_VEND, NULL, NULL);
		hdev->tx_hw.cntlconfig(&hdev->tx_hw, CONF_AVI_BT2020,
			CLR_AVI_BT2020);
		hdev->hdr10plus_feature = 0;
		return;
	}

	if (hdev->hdr10plus_feature != 1)
		pr_info("hdmitx_set_hdr10plus_pkt: flag = %d\n", flag);
	hdev->hdr10plus_feature = 1;
	hdr_status_pos = 3;
	VEN_DB[0] = 0x8b;
	VEN_DB[1] = 0x84;
	VEN_DB[2] = 0x90;

	VEN_DB[3] = ((data->application_version & 0x3) << 6) |
		 ((data->targeted_max_lum & 0x1f) << 1);
	VEN_DB[4] = data->average_maxrgb;
	VEN_DB[5] = data->distribution_values[0];
	VEN_DB[6] = data->distribution_values[1];
	VEN_DB[7] = data->distribution_values[2];
	VEN_DB[8] = data->distribution_values[3];
	VEN_DB[9] = data->distribution_values[4];
	VEN_DB[10] = data->distribution_values[5];
	VEN_DB[11] = data->distribution_values[6];
	VEN_DB[12] = data->distribution_values[7];
	VEN_DB[13] = data->distribution_values[8];
	VEN_DB[14] = ((data->num_bezier_curve_anchors & 0xf) << 4) |
		((data->knee_point_x >> 6) & 0xf);
	VEN_DB[15] = ((data->knee_point_x & 0x3f) << 2) |
		((data->knee_point_y >> 8) & 0x3);
	VEN_DB[16] = data->knee_point_y  & 0xff;
	VEN_DB[17] = data->bezier_curve_anchors[0];
	VEN_DB[18] = data->bezier_curve_anchors[1];
	VEN_DB[19] = data->bezier_curve_anchors[2];
	VEN_DB[20] = data->bezier_curve_anchors[3];
	VEN_DB[21] = data->bezier_curve_anchors[4];
	VEN_DB[22] = data->bezier_curve_anchors[5];
	VEN_DB[23] = data->bezier_curve_anchors[6];
	VEN_DB[24] = data->bezier_curve_anchors[7];
	VEN_DB[25] = data->bezier_curve_anchors[8];
	VEN_DB[26] = ((data->graphics_overlay_flag & 0x1) << 7) |
		((data->no_delay_flag & 0x1) << 6);

	hdev->hwop.setpacket(HDMI_PACKET_VEND, VEN_DB, VEN_HB);
	hdev->tx_hw.cntlconfig(&hdev->tx_hw, CONF_AVI_BT2020,
			SET_AVI_BT2020);
}

static void hdmitx_set_cuva_hdr_vsif(struct cuva_hdr_vsif_para *data)
{
	unsigned long flags = 0;
	struct hdmitx_dev *hdev = get_hdmitx_device();
	unsigned char ven_hb[3] = {0x81, 0x01, 0x1b};
	unsigned char ven_db[27] = {0x00};
	const struct cuva_info *cuva = &hdev->tx_comm.rxcap.hdr_info.cuva_info;

	spin_lock_irqsave(&hdev->edid_spinlock, flags);
	if (cuva->ieeeoui != CUVA_IEEEOUI) {
		spin_unlock_irqrestore(&hdev->edid_spinlock, flags);
		return;
	}
	if (!data) {
		hdev->hwop.setpacket(HDMI_PACKET_VEND, NULL, NULL);
		spin_unlock_irqrestore(&hdev->edid_spinlock, flags);
		return;
	}
	ven_db[0] = GET_OUI_BYTE0(CUVA_IEEEOUI);
	ven_db[1] = GET_OUI_BYTE1(CUVA_IEEEOUI);
	ven_db[2] = GET_OUI_BYTE2(CUVA_IEEEOUI);
	ven_db[3] = data->system_start_code;
	ven_db[4] = (data->version_code & 0xf) << 4;
	hdev->hwop.setpacket(HDMI_PACKET_VEND, ven_db, ven_hb);
	spin_unlock_irqrestore(&hdev->edid_spinlock, flags);
}

struct hdmi_packet_t {
	u8 hb[3];
	u8 pb[28];
	u8 no_used; /* padding to 32 bytes */
};

static void hdmitx_set_cuva_hdr_vs_emds(struct cuva_hdr_vs_emds_para *data)
{
	struct hdmi_packet_t vs_emds[3];
	unsigned long flags;
	struct hdmitx_dev *hdev = get_hdmitx_device();
	static unsigned char *virt_ptr;
	static unsigned char *virt_ptr_align32bit;
	unsigned long phys_ptr;

	memset(vs_emds, 0, sizeof(vs_emds));
	spin_lock_irqsave(&hdev->edid_spinlock, flags);
	if (!data) {
		hdev->tx_hw.cntlconfig(&hdev->tx_hw, CONF_EMP_NUMBER, 0);
		spin_unlock_irqrestore(&hdev->edid_spinlock, flags);
		return;
	}

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

	if (!virt_ptr) { /* init virt_ptr and virt_ptr_align32bit */
		virt_ptr = kzalloc((sizeof(vs_emds) + 0x1f), GFP_KERNEL);
		virt_ptr_align32bit = (unsigned char *)
			((((unsigned long)virt_ptr) + 0x1f) & (~0x1f));
	}
	memcpy(virt_ptr_align32bit, vs_emds, sizeof(vs_emds));
	phys_ptr = virt_to_phys(virt_ptr_align32bit);

	pr_info("emp_pkt phys_ptr: %lx\n", phys_ptr);

	hdev->tx_hw.cntlconfig(&hdev->tx_hw, CONF_EMP_NUMBER,
			      sizeof(vs_emds) / (sizeof(struct hdmi_packet_t)));
	hdev->tx_hw.cntlconfig(&hdev->tx_hw, CONF_EMP_PHY_ADDR, phys_ptr);
	spin_unlock_irqrestore(&hdev->edid_spinlock, flags);
}

#define  EMP_FIRST 0x80
#define  EMP_LAST 0x40
struct emp_debug_save emp_config_data;
static void hdmitx_set_emp_pkt(unsigned char *data, unsigned int type,
			       unsigned int size)
{
	unsigned int number;
	unsigned int remainder;
	unsigned char *virt_ptr;
	unsigned char *virt_ptr_align32bit;
	unsigned long phys_ptr;
	unsigned int i;
	struct hdmitx_dev *hdev = get_hdmitx_device();
	unsigned int ds_type = 0;
	unsigned char AFR = 0;
	unsigned char VFR = 0;
	unsigned char sync = 0;
	unsigned char  new = 0;
	unsigned char  end = 0;
	unsigned int organization_id = 0;
	unsigned int data_set_tag = 0;
	unsigned int data_set_length = 0;

	hdmi_debug();

	if (!data) {
		pr_info("the data is null\n");
		return;
	}

	emp_config_data.type = type;
	emp_config_data.size = size;
	if (size <= 128)
		memcpy(emp_config_data.data, data, size);
	else
		memcpy(emp_config_data.data, data, 128);

	if (hdev->data->chip_type < MESON_CPU_ID_G12A) {
		pr_info("this chip doesn't support emp function\n");
		return;
	}
	if (size <= 21) {
		number = 1;
		remainder = size;
	} else {
		number = ((size - 21) / 28) + 2;
		remainder = (size - 21) % 28;
	}

	virt_ptr = kzalloc(sizeof(unsigned char) * (number + 0x1f),
			   GFP_KERNEL);
	if (!virt_ptr)
		return;
	pr_info("emp_pkt virt_ptr: %p\n", virt_ptr);
	virt_ptr_align32bit = (unsigned char *)
		((((unsigned long)virt_ptr) + 0x1f) & (~0x1f));
	pr_info("emp_pkt virt_ptr_align32bit: %p\n", virt_ptr_align32bit);

	memset(virt_ptr_align32bit, 0, sizeof(unsigned char) * (number + 0x1f));

	switch (type) {
	case VENDOR_SPECIFIC_EM_DATA:
		break;
	case COMPRESS_VIDEO_TRAMSPORT:
		break;
	case HDR_DYNAMIC_METADATA:
			ds_type = 1;
			sync = 1;
			VFR = 1;
			AFR = 0;
			new = 0x1; /*todo*/
			end = 0x1; /*todo*/
			organization_id = 2;
		break;
	case VIDEO_TIMING_EXTENDED:
		break;
	default:
		break;
	}

	for (i = 0; i < number; i++) {
		/*HB[0]-[2]*/
		virt_ptr_align32bit[i * 32 + 0] = 0x7F;
		if (i == 0)
			virt_ptr_align32bit[i * 32 + 1] |=  EMP_FIRST;
		if (i == number)
			virt_ptr_align32bit[i * 32 + 1] |= EMP_LAST;
		virt_ptr_align32bit[i * 32 + 2] = number;
		/*PB[0]-[6]*/
		if (i == 0) {
			virt_ptr_align32bit[3] = (new << 7) | (end << 6) |
				(ds_type << 4) | (AFR << 3) |
				(VFR << 2) | (sync << 1);
			virt_ptr_align32bit[4] = 0;/*Rsvd*/
			virt_ptr_align32bit[5] = organization_id;
			virt_ptr_align32bit[6] = (data_set_tag >> 8) & 0xFF;
			virt_ptr_align32bit[7] = data_set_tag & 0xFF;
			virt_ptr_align32bit[8] = (data_set_length >> 8)
				& 0xFF;
			virt_ptr_align32bit[9] = data_set_length & 0xFF;
		}
		if (number == 1) {
			memcpy(&virt_ptr_align32bit[10], &data[0],
			       sizeof(unsigned char) * remainder);
		} else {
			if (i == 0) {
			/*MD: first package need PB[7]-[27]*/
				memcpy(&virt_ptr_align32bit[10], &data[0],
				       sizeof(unsigned char) * 21);
			} else if (i != number) {
			/*MD: following package need PB[0]-[27]*/
				memcpy(&virt_ptr_align32bit[i * 32 + 10],
				       &data[(i - 1) * 28 + 21],
				       sizeof(unsigned char) * 28);
			} else {
			/*MD: the last package need PB[0] to end */
				memcpy(&virt_ptr_align32bit[0],
				       &data[(i - 1) * 28 + 21],
				       sizeof(unsigned char) * remainder);
			}
		}
			/*PB[28]*/
		virt_ptr_align32bit[i * 32 + 31] = 0;
	}

	phys_ptr = virt_to_phys(virt_ptr_align32bit);
	pr_info("emp_pkt phys_ptr: %lx\n", phys_ptr);

	hdev->tx_hw.cntlconfig(&hdev->tx_hw, CONF_EMP_NUMBER, number);
	hdev->tx_hw.cntlconfig(&hdev->tx_hw, CONF_EMP_PHY_ADDR, phys_ptr);
}

/*config attr*/
static ssize_t config_show(struct device *dev,
			   struct device_attribute *attr, char *buf)
{
	int pos = 0;
	unsigned char *conf;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);
	struct hdmitx_audpara *audio_param = &hdev->cur_audio_param;

	pos += snprintf(buf + pos, PAGE_SIZE, "cur_VIC: %d\n", hdev->tx_comm.cur_VIC);
	if (hdev->cur_video_param)
		pos += snprintf(buf + pos, PAGE_SIZE,
			"cur_video_param->VIC=%d\n",
			hdev->cur_video_param->VIC);

	switch (hdev->tx_comm.fmt_para.cd) {
	case COLORDEPTH_24B:
		conf = "8bit";
		break;
	case COLORDEPTH_30B:
		conf = "10bit";
		break;
	case COLORDEPTH_36B:
		conf = "12bit";
		break;
	case COLORDEPTH_48B:
		conf = "16bit";
		break;
	default:
		conf = "reserved";
	}
	pos += snprintf(buf + pos, PAGE_SIZE, "colordepth: %s\n",
			conf);
	switch (hdev->tx_comm.fmt_para.cs) {
	case HDMI_COLORSPACE_RGB:
		conf = "RGB";
		break;
	case HDMI_COLORSPACE_YUV422:
		conf = "422";
		break;
	case HDMI_COLORSPACE_YUV444:
		conf = "444";
		break;
	case HDMI_COLORSPACE_YUV420:
		conf = "420";
		break;
	default:
		conf = "reserved";
	}
	pos += snprintf(buf + pos, PAGE_SIZE, "colorspace: %s\n",
			conf);

	switch (hdev->tx_aud_cfg) {
	case 0:
		conf = "off";
		break;
	case 1:
		conf = "on";
		break;
	case 2:
		conf = "auto";
		break;
	default:
		conf = "none";
	}
	pos += snprintf(buf + pos, PAGE_SIZE, "audio config: %s\n", conf);

	switch (hdev->hdmi_audio_off_flag) {
	case 0:
		conf = "on";
		break;
	case 1:
		conf = "off";
		break;
	default:
		conf = "none";
	}
	pos += snprintf(buf + pos, PAGE_SIZE, "audio on/off: %s\n", conf);

	switch (audio_param->aud_src_if) {
	case AUD_SRC_IF_SPDIF:
		conf = "SPDIF";
		break;
	case AUD_SRC_IF_I2S:
		conf = "I2S";
		break;
	case AUD_SRC_IF_TDM:
		conf = "TDM";
		break;
	default:
		conf = "none";
	}
	pos += snprintf(buf + pos, PAGE_SIZE, "audio source: %s\n", conf);

	switch (hdev->cur_audio_param.type) {
	case CT_REFER_TO_STREAM:
		conf = "refer to stream header";
		break;
	case CT_PCM:
		conf = "L-PCM";
		break;
	case CT_AC_3:
		conf = "AC-3";
		break;
	case CT_MPEG1:
		conf = "MPEG1";
		break;
	case CT_MP3:
		conf = "MP3";
		break;
	case CT_MPEG2:
		conf = "MPEG2";
		break;
	case CT_AAC:
		conf = "AAC";
		break;
	case CT_DTS:
		conf = "DTS";
		break;
	case CT_ATRAC:
		conf = "ATRAC";
		break;
	case CT_ONE_BIT_AUDIO:
		conf = "One Bit Audio";
		break;
	case CT_DD_P:
		conf = "Dobly Digital+";
		break;
	case CT_DTS_HD:
		conf = "DTS_HD";
		break;
	case CT_MAT:
		conf = "MAT";
		break;
	case CT_DST:
		conf = "DST";
		break;
	case CT_WMA:
		conf = "WMA";
		break;
	default:
		conf = "MAX";
	}
	pos += snprintf(buf + pos, PAGE_SIZE, "audio type: %s\n", conf);

	switch (hdev->cur_audio_param.channel_num) {
	case CC_REFER_TO_STREAM:
		conf = "refer to stream header";
		break;
	case CC_2CH:
		conf = "2 channels";
		break;
	case CC_3CH:
		conf = "3 channels";
		break;
	case CC_4CH:
		conf = "4 channels";
		break;
	case CC_5CH:
		conf = "5 channels";
		break;
	case CC_6CH:
		conf = "6 channels";
		break;
	case CC_7CH:
		conf = "7 channels";
		break;
	case CC_8CH:
		conf = "8 channels";
		break;
	default:
		conf = "MAX";
	}
	pos += snprintf(buf + pos, PAGE_SIZE, "audio channel num: %s\n", conf);

	switch (hdev->cur_audio_param.sample_rate) {
	case FS_REFER_TO_STREAM:
		conf = "refer to stream header";
		break;
	case FS_32K:
		conf = "32kHz";
		break;
	case FS_44K1:
		conf = "44.1kHz";
		break;
	case FS_48K:
		conf = "48kHz";
		break;
	case FS_88K2:
		conf = "88.2kHz";
		break;
	case FS_96K:
		conf = "96kHz";
		break;
	case FS_176K4:
		conf = "176.4kHz";
		break;
	case FS_192K:
		conf = "192kHz";
		break;
	case FS_768K:
		conf = "768kHz";
		break;
	default:
		conf = "MAX";
	}
	pos += snprintf(buf + pos, PAGE_SIZE, "audio sample rate: %s\n", conf);

	switch (hdev->cur_audio_param.sample_size) {
	case SS_REFER_TO_STREAM:
		conf = "refer to stream header";
		break;
	case SS_16BITS:
		conf = "16bit";
		break;
	case SS_20BITS:
		conf = "20bit";
		break;
	case SS_24BITS:
		conf = "24bit";
		break;
	default:
		conf = "MAX";
	}
	pos += snprintf(buf + pos, PAGE_SIZE, "audio sample size: %s\n", conf);

	if (hdev->flag_3dfp)
		conf = "FramePacking";
	else if (hdev->flag_3dss)
		conf = "SidebySide";
	else if (hdev->flag_3dtb)
		conf = "TopButtom";
	else
		conf = "off";
	pos += snprintf(buf + pos, PAGE_SIZE, "3D config: %s\n", conf);
	return pos;
}

static ssize_t config_store(struct device *dev,
			    struct device_attribute *attr,
			    const char *buf, size_t count)
{
	int ret = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);
	struct master_display_info_s data = {0};
	struct hdr10plus_para hdr_data = {0x1, 0x2, 0x3};
	struct dv_vsif_para vsif_para = {0};

	pr_info("hdmitx: config: %s\n", buf);

	if (strncmp(buf, "unplug_powerdown", 16) == 0) {
		if (buf[16] == '0')
			hdev->unplug_powerdown = 0;
		else
			hdev->unplug_powerdown = 1;
	} else if (strncmp(buf, "3d", 2) == 0) {
		/* Second, set 3D parameters */
		if (strncmp(buf + 2, "tb", 2) == 0) {
			hdev->flag_3dtb = 1;
			hdev->flag_3dss = 0;
			hdev->flag_3dfp = 0;
			hdmi_set_3d(hdev, T3D_TAB, 0);
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
			hdmi_set_3d(hdev, T3D_SBS_HALF,
				    sub_sample_mode);
		} else if (strncmp(buf + 2, "fp", 2) == 0) {
			hdev->flag_3dtb = 0;
			hdev->flag_3dss = 0;
			hdev->flag_3dfp = 1;
			hdmi_set_3d(hdev, T3D_FRAME_PACKING, 0);
		} else if (strncmp(buf + 2, "off", 3) == 0) {
			hdev->flag_3dfp = 0;
			hdev->flag_3dtb = 0;
			hdev->flag_3dss = 0;
			hdmi_set_3d(hdev, T3D_DISABLE, 0);
		}
	} else if (strncmp(buf, "sdr", 3) == 0) {
		data.features = 0x00010100;
		hdmitx_set_drm_pkt(&data);
	} else if (strncmp(buf, "hdr", 3) == 0) {
		data.features = 0x00091000;
		hdmitx_set_drm_pkt(&data);
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
		} else if (buf[4] == '4') {
			/* DV LL */
			vsif_para.ver = 0x1;
			vsif_para.length = 0x1b;
			vsif_para.ver2_l11_flag = 0;
			vsif_para.vers.ver2.low_latency = 1;
			vsif_para.vers.ver2.dobly_vision_signal = 1;
			hdmitx_set_vsif_pkt(4, 0, &vsif_para, false);
		} else if (buf[4] == '0') {
			/* exit DV to SDR */
			hdmitx_set_vsif_pkt(0, 0, NULL, true);
		}
	} else if (strncmp(buf, "emp", 3) == 0) {
		if (hdev->data->chip_type >= MESON_CPU_ID_G12A)
			hdmitx_set_emp_pkt(NULL, 1, 1);
	} else if (strncmp(buf, "hdr10+", 6) == 0) {
		hdmitx_set_hdr10plus_pkt(1, &hdr_data);
	}
	return count;
}

void hdmitx20_ext_set_audio_output(int enable)
{
	pr_info("%s[%d] enable = %d\n", __func__, __LINE__, enable);
	hdmitx_audio_mute_op(enable);
}

int hdmitx20_ext_get_audio_status(void)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();
	int val;
	static int val_st;

	val = !!(hdev->tx_hw.cntlconfig(&hdev->tx_hw, CONF_GET_AUDIO_MUTE_ST, 0));
	if (val_st != val) {
		val_st = val;
		pr_info("%s[%d] val = %d\n", __func__, __LINE__, val);
	}
	return val;
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
		if (atomic_read(&kref_video_mute) == 1)
			hdmitx_video_mute_op(0);
	}
	if (buf[0] == '0') {
		if (!(atomic_sub_and_test(0, &kref_video_mute))) {
			atomic_dec(&kref_video_mute);
			if (atomic_sub_and_test(0, &kref_video_mute))
				hdmitx_video_mute_op(1);
		}
	}

	hdev->kref_video_mute = kref_video_mute;

	return count;
}

static ssize_t debug_store(struct device *dev,
			   struct device_attribute *attr,
			   const char *buf, size_t count)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	hdev->hwop.debugfun(hdev, buf);
	return count;
}

static void _show_pcm_ch(struct rx_cap *prxcap, int i,
			 int *ppos, char *buf)
{
	const char * const aud_sample_size[] = {"ReferToStreamHeader",
		"16", "20", "24", NULL};
	int j = 0;

	for (j = 0; j < 3; j++) {
		if (prxcap->RxAudioCap[i].cc3 & (1 << j))
			*ppos += snprintf(buf + *ppos, PAGE_SIZE, "%s/",
				aud_sample_size[j + 1]);
	}
	*ppos += snprintf(buf + *ppos - 1, PAGE_SIZE, " bit\n") - 1;
}

/**/
static ssize_t aud_cap_show(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	int i, pos = 0, j;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);
	static const char * const aud_ct[] =  {
		"ReferToStreamHeader", "PCM", "AC-3", "MPEG1", "MP3",
		"MPEG2", "AAC", "DTS", "ATRAC",	"OneBitAudio",
		"Dolby_Digital+", "DTS-HD", "MAT", "DST", "WMA_Pro",
		"Reserved", NULL};
	static const char * const aud_sampling_frequency[] = {
		"ReferToStreamHeader", "32", "44.1", "48", "88.2", "96",
		"176.4", "192", NULL};
	struct rx_cap *prxcap = &hdev->tx_comm.rxcap;
	struct dolby_vsadb_cap *cap = &prxcap->dolby_vsadb_cap;

	pos += snprintf(buf + pos, PAGE_SIZE,
		"CodingType MaxChannels SamplingFreq SampleSize\n");
	for (i = 0; i < prxcap->AUD_count; i++) {
		if (prxcap->RxAudioCap[i].audio_format_code == CT_CXT) {
			if ((prxcap->RxAudioCap[i].cc3 >> 3) == 0xb) {
				pos += snprintf(buf + pos, PAGE_SIZE, "MPEG-H, 8ch, ");
				for (j = 0; j < 7; j++) {
					if (prxcap->RxAudioCap[i].freq_cc & (1 << j))
						pos += snprintf(buf + pos, PAGE_SIZE, "%s/",
							aud_sampling_frequency[j + 1]);
				}
				pos += snprintf(buf + pos - 1, PAGE_SIZE, " kHz\n");
			}
			continue;
		}
		pos += snprintf(buf + pos, PAGE_SIZE, "%s",
			aud_ct[prxcap->RxAudioCap[i].audio_format_code]);
		if (prxcap->RxAudioCap[i].audio_format_code == CT_DD_P &&
		    (prxcap->RxAudioCap[i].cc3 & 1))
			pos += snprintf(buf + pos, PAGE_SIZE, "/ATMOS");
		if (prxcap->RxAudioCap[i].audio_format_code != CT_CXT)
			pos += snprintf(buf + pos, PAGE_SIZE, ", %d ch, ",
				prxcap->RxAudioCap[i].channel_num_max + 1);
		for (j = 0; j < 7; j++) {
			if (prxcap->RxAudioCap[i].freq_cc & (1 << j))
				pos += snprintf(buf + pos, PAGE_SIZE, "%s/",
					aud_sampling_frequency[j + 1]);
		}
		pos += snprintf(buf + pos - 1, PAGE_SIZE, " kHz, ") - 1;
		switch (prxcap->RxAudioCap[i].audio_format_code) {
		case CT_PCM:
			_show_pcm_ch(prxcap, i, &pos, buf);
			break;
		case CT_AC_3:
		case CT_MPEG1:
		case CT_MP3:
		case CT_MPEG2:
		case CT_AAC:
		case CT_DTS:
		case CT_ATRAC:
		case CT_ONE_BIT_AUDIO:
			pos += snprintf(buf + pos, PAGE_SIZE,
				"MaxBitRate %dkHz\n",
				prxcap->RxAudioCap[i].cc3 * 8);
			break;
		case CT_DD_P:
		case CT_DTS_HD:
		case CT_MAT:
		case CT_DST:
			pos += snprintf(buf + pos, PAGE_SIZE, "DepValue 0x%x\n",
				prxcap->RxAudioCap[i].cc3);
			break;
		case CT_WMA:
		default:
			break;
		}
	}

	if (cap->ieeeoui == DOVI_IEEEOUI) {
		/*
		 *Dolby Vendor Specific:
		 *  headphone_playback_only:0,
		 *  center_speaker:1,
		 *  surround_speaker:1,
		 *  height_speaker:1,
		 *  Ver:1.0,
		 *  MAT_PCM_48kHz_only:1,
		 *  e61146d0007001,
		 */
		pos += snprintf(buf + pos, PAGE_SIZE,
				"Dolby Vendor Specific:\n");
		if (cap->dolby_vsadb_ver == 0)
			pos += snprintf(buf + pos, PAGE_SIZE, "  Ver:1.0,\n");
		else
			pos += snprintf(buf + pos, PAGE_SIZE,
				"  Ver:Reversed,\n");
		pos += snprintf(buf + pos, PAGE_SIZE,
			"  center_speaker:%d,\n", cap->spk_center);
		pos += snprintf(buf + pos, PAGE_SIZE,
			"  surround_speaker:%d,\n", cap->spk_surround);
		pos += snprintf(buf + pos, PAGE_SIZE,
			"  height_speaker:%d,\n", cap->spk_height);
		pos += snprintf(buf + pos, PAGE_SIZE,
			"  headphone_playback_only:%d,\n", cap->headphone_only);
		pos += snprintf(buf + pos, PAGE_SIZE,
			"  MAT_PCM_48kHz_only:%d,\n", cap->mat_48k_pcm_only);

		pos += snprintf(buf + pos, PAGE_SIZE, "  ");
		for (i = 0; i < 7; i++)
			pos += snprintf(buf + pos, PAGE_SIZE, "%02x",
				cap->rawdata[i]);
		pos += snprintf(buf + pos, PAGE_SIZE, ",\n");
	}
	return pos;
}

/**/
static ssize_t hdmi_hdr_status_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

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

	/* default is SDR */
	pos += snprintf(buf + pos, PAGE_SIZE, "SDR");

	return pos;
}

static int hdmi_hdr_status_to_drm(void)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();

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

/**/
static ssize_t dc_cap_show(struct device *dev,
			   struct device_attribute *attr, char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);
	struct rx_cap *prxcap = &hdev->tx_comm.rxcap;
	const struct dv_info *dv = &hdev->tx_comm.rxcap.dv_info;
	const struct dv_info *dv2 = &hdev->tx_comm.rxcap.dv_info2;
	int i;

	if (prxcap->dc_36bit_420)
		pos += snprintf(buf + pos, PAGE_SIZE, "420,12bit\n");
	if (prxcap->dc_30bit_420)
		pos += snprintf(buf + pos, PAGE_SIZE, "420,10bit\n");

	for (i = 0; i < Y420_VIC_MAX_NUM; i++) {
		if (prxcap->y420_vic[i]) {
			pos += snprintf(buf + pos, PAGE_SIZE,
				"420,8bit\n");
			break;
		}
	}

	if (prxcap->native_Mode & (1 << 5)) {
		if (prxcap->dc_y444) {
			if (prxcap->dc_36bit || dv->sup_10b_12b_444 == 0x2 ||
			    dv2->sup_10b_12b_444 == 0x2)
				if (!hdev->vend_id_hit)
					pos += snprintf(buf + pos, PAGE_SIZE, "444,12bit\n");
			if (prxcap->dc_30bit || dv->sup_10b_12b_444 == 0x1 ||
			    dv2->sup_10b_12b_444 == 0x1) {
				if (!hdev->vend_id_hit)
					pos += snprintf(buf + pos, PAGE_SIZE, "444,10bit\n");
			}
		}
		pos += snprintf(buf + pos, PAGE_SIZE, "444,8bit\n");
	}
	/* y422, not check dc */
	if (prxcap->native_Mode & (1 << 4)) {
		pos += snprintf(buf + pos, PAGE_SIZE, "422,12bit\n");
		pos += snprintf(buf + pos, PAGE_SIZE, "422,10bit\n");
		pos += snprintf(buf + pos, PAGE_SIZE, "422,8bit\n");
	}

	if (prxcap->dc_36bit || dv->sup_10b_12b_444 == 0x2 ||
	    dv2->sup_10b_12b_444 == 0x2)
		if (!hdev->vend_id_hit)
			pos += snprintf(buf + pos, PAGE_SIZE, "rgb,12bit\n");
	if (prxcap->dc_30bit || dv->sup_10b_12b_444 == 0x1 ||
	    dv2->sup_10b_12b_444 == 0x1)
		if (!hdev->vend_id_hit)
			pos += snprintf(buf + pos, PAGE_SIZE, "rgb,10bit\n");
	pos += snprintf(buf + pos, PAGE_SIZE, "rgb,8bit\n");
	return pos;
}

static ssize_t allm_cap_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);
	struct rx_cap *prxcap = &hdev->tx_comm.rxcap;

	pos += snprintf(buf + pos, PAGE_SIZE, "%d\n\r", prxcap->allm);
	return pos;
}

static ssize_t allm_mode_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	pos += snprintf(buf + pos, PAGE_SIZE, "%d\n\r", hdev->tx_comm.allm_mode);

	return pos;
}

static inline int com_str(const char *buf, const char *str)
{
	return strncmp(buf, str, strlen(str)) == 0;
}

static ssize_t allm_mode_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf,
			       size_t count)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	pr_info("hdmitx: store allm_mode as %s\n", buf);

	if (com_str(buf, "0")) {
		// disable ALLM
		hdev->tx_comm.allm_mode = 0;
		hdmitx_construct_vsif(&hdev->tx_comm, VT_ALLM, 0, NULL);
		hdmitx_construct_vsif(&hdev->tx_comm, VT_HDMI14_4K, 1, NULL);
	}
	if (com_str(buf, "1")) {
		hdev->tx_comm.allm_mode = 1;
		hdmitx_construct_vsif(&hdev->tx_comm, VT_ALLM, 1, NULL);
		hdev->tx_hw.cntlconfig(&hdev->tx_hw, CONF_CT_MODE, SET_CT_OFF);
	}
	if (com_str(buf, "-1")) {
		if (hdev->tx_comm.allm_mode == 1) {
			hdev->tx_comm.allm_mode = 0;
			hdev->hwop.disablepacket(HDMI_PACKET_VEND);
		}
	}
	return count;
}

static void drm_set_allm_mode(int mode)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();

	if (mode == 0) {
		hdev->tx_comm.allm_mode = 0;
		hdmitx_construct_vsif(&hdev->tx_comm, VT_ALLM, 0, NULL);
		hdmitx_construct_vsif(&hdev->tx_comm, VT_HDMI14_4K, 1, NULL);
	}
	if (mode == 1) {
		hdev->tx_comm.allm_mode = 1;
		hdmitx_construct_vsif(&hdev->tx_comm, VT_ALLM, 1, NULL);
		hdev->tx_hw.cntlconfig(&hdev->tx_hw, CONF_CT_MODE, SET_CT_OFF);
	}
	if (mode == 2) {
		if (hdev->tx_comm.allm_mode == 1) {
			hdev->tx_comm.allm_mode = 0;
			hdev->hwop.disablepacket(HDMI_PACKET_VEND);
		}
	}
}

bool dv_support(void)
{
	int ret;
	struct hdmitx_dev *hdev = get_hdmitx_device();
	const struct dv_info *dv = &hdev->tx_comm.rxcap.dv_info;
	struct hdmitx_common *tx_comm = &hdev->tx_comm;

	ret = (dv->ieeeoui != DV_IEEE_OUI || tx_comm->hdr_priority);
	return ret;
}
EXPORT_SYMBOL(dv_support);


static ssize_t aud_ch_show(struct device *dev,
			   struct device_attribute *attr,
			   char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	pos += snprintf(buf + pos, PAGE_SIZE,
		"hdmi_channel = %d ch\n",
		hdev->hdmi_ch ? hdev->hdmi_ch + 1 : 0);
	return pos;
}

static ssize_t aud_ch_store(struct device *dev,
			    struct device_attribute *attr,
			    const char *buf, size_t count)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	if (strncmp(buf, "6ch", 3) == 0)
		hdev->hdmi_ch = 5;
	else if (strncmp(buf, "8ch", 3) == 0)
		hdev->hdmi_ch = 7;
	else if (strncmp(buf, "2ch", 3) == 0)
		hdev->hdmi_ch = 1;
	else
		return count;

	hdev->audio_param_update_flag = 1;
	hdev->force_audio_flag = 1;

	return count;
}

/*
 * 0: clear vic
 */
static ssize_t vic_store(struct device *dev,
			 struct device_attribute *attr,
			 const char *buf, size_t count)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	if (strncmp(buf, "0", 1) == 0) {
		hdev->tx_hw.cntlconfig(&hdev->tx_hw, CONF_CLR_AVI_PACKET, 0);
		hdev->tx_hw.cntlconfig(&hdev->tx_hw, CONF_CLR_VSDB_PACKET, 0);
	}

	return count;
}

static ssize_t vic_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);
	enum hdmi_vic vic = HDMI_0_UNKNOWN;
	int pos = 0;

	vic = hdev->tx_hw.getstate(&hdev->tx_hw, STAT_VIDEO_VIC, 0);
	pos += snprintf(buf + pos, PAGE_SIZE, "%d\n", vic);

	return pos;
}

static ssize_t avmute_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ret = 0;
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	ret = hdev->tx_hw.cntlmisc(&hdev->tx_hw, MISC_READ_AVMUTE_OP, 0);
	pos += snprintf(buf + pos, PAGE_SIZE, "%d", ret);

	return pos;
}

/*
 *  1: set avmute
 * -1: clear avmute
 *  0: off avmute
 */
static ssize_t avmute_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int cmd = OFF_AVMUTE;
	static int mask0;
	static int mask1;
	static DEFINE_MUTEX(avmute_mutex);
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	unsigned int mute_us =
		hdev->debug_param.avmute_frame * hdmitx_get_frame_duration();

	pr_info("%s %s\n", __func__, buf);
	mutex_lock(&avmute_mutex);
	if (strncmp(buf, "-1", 2) == 0) {
		cmd = CLR_AVMUTE;
		mask0 = -1;
	} else if (strncmp(buf, "0", 1) == 0) {
		cmd = OFF_AVMUTE;
		mask0 = 0;
	} else if (strncmp(buf, "1", 1) == 0) {
		cmd = SET_AVMUTE;
		mask0 = 1;
	}
	if (strncmp(buf, "r-1", 3) == 0) {
		cmd = CLR_AVMUTE;
		mask1 = -1;
	} else if (strncmp(buf, "r0", 2) == 0) {
		cmd = OFF_AVMUTE;
		mask1 = 0;
	} else if (strncmp(buf, "r1", 2) == 0) {
		cmd = SET_AVMUTE;
		mask1 = 1;
	}
	if (mask0 == 1 || mask1 == 1)
		cmd = SET_AVMUTE;
	else if ((mask0 == -1) && (mask1 == -1))
		cmd = CLR_AVMUTE;

	hdev->tx_hw.cntlmisc(&hdev->tx_hw, MISC_AVMUTE_OP, cmd);

	if (cmd == SET_AVMUTE && hdev->debug_param.avmute_frame > 0)
		msleep(mute_us / 1000);

	mutex_unlock(&avmute_mutex);

	return count;
}

static ssize_t rxsense_policy_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	int val = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	if (isdigit(buf[0])) {
		val = buf[0] - '0';
		pr_info(SYS "hdmitx: set rxsense_policy as %d\n", val);
		if (val == 0 || val == 1)
			hdev->rxsense_policy = val;
		else
			pr_info(SYS "only accept as 0 or 1\n");
	}
	if (hdev->rxsense_policy)
		queue_delayed_work(hdev->rxsense_wq,
				   &hdev->work_rxsense, 0);
	else
		cancel_delayed_work(&hdev->work_rxsense);

	return count;
}

static ssize_t rxsense_policy_show(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	pos += snprintf(buf + pos, PAGE_SIZE, "%d\n",
		hdev->rxsense_policy);

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
		pr_info("hdmitx: set cedst_policy as %d\n", val);
		if (val == 0 || val == 1 || val == 2) {
			hdev->cedst_policy = val;
			if (val == 1) { /* Auto mode, depends on Rx */
				/* check RX scdc_present */
				if (hdev->tx_comm.rxcap.scdc_present)
					hdev->cedst_policy = 1;
				else
					hdev->cedst_policy = 0;
			}
			if (val == 2) /* Force mode */
				hdev->cedst_policy = 1;
			/* assgin cedst_en from dts or here */
			hdev->cedst_en = hdev->cedst_policy;
		} else {
			pr_info("only accept as 0, 1(auto), or 2(force)\n");
		}
	}
	if (hdev->cedst_policy)
		queue_delayed_work(hdev->cedst_wq, &hdev->work_cedst, 0);
	else
		cancel_delayed_work(&hdev->work_cedst);

	return count;
}

static ssize_t cedst_policy_show(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	pos += snprintf(buf + pos, PAGE_SIZE, "%d\n",
		hdev->cedst_policy);

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
		pr_info(SYS "set sspll : %d\n", val);
		if (val == 0 || val == 1)
			hdev->sspll = val;
		else
			pr_info(SYS "sspll only accept as 0 or 1\n");
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
	int val = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	if (strncmp(buf, "0", 1) == 0)
		val = 0;
	if (strncmp(buf, "1", 1) == 0)
		val = 1;
	if (strncmp(buf, "-1", 2) == 0)
		val = -1;
	pr_info(SYS "set hdcp_type_policy as %d\n", val);
	hdev->hdcp_type_policy = val;

	return count;
}

static ssize_t hdcp_type_policy_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	pos += snprintf(buf + pos, PAGE_SIZE, "%d\n",
		hdev->hdcp_type_policy);

	return pos;
}

static ssize_t hdcp_clkdis_store(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	hdev->tx_hw.cntlmisc(&hdev->tx_hw, MISC_HDCP_CLKDIS,
		buf[0] == '1' ? 1 : 0);
	return count;
}

static ssize_t hdcp_clkdis_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	return 0;
}

static ssize_t hdcp_pwr_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	if (buf[0] == '1') {
		hdev->hdcp_tst_sig = 1;
		pr_debug(SYS "set hdcp_pwr 1\n");
	}

	return count;
}

static ssize_t hdcp_pwr_show(struct device *dev,
			     struct device_attribute *attr,
			     char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	if (hdev->hdcp_tst_sig == 1) {
		pos += snprintf(buf + pos, PAGE_SIZE, "%d\n",
			hdev->hdcp_tst_sig);
		hdev->hdcp_tst_sig = 0;
		pr_debug(SYS "restore hdcp_pwr 0\n");
	}

	return pos;
}

static ssize_t hdcp_byp_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	pr_info(SYS "%s...\n", __func__);

	hdev->tx_hw.cntlmisc(&hdev->tx_hw, MISC_HDCP_CLKDIS,
		buf[0] == '1' ? 1 : 0);

	return count;
}

static ssize_t hdcp_lstore_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	/* if current TX is RP-TX, then return lstore as 00 */
	/* hdcp_lstore is used under only TX */
	if (hdev->repeater_tx == 1) {
		pos += snprintf(buf + pos, PAGE_SIZE, "00\n");
		return pos;
	}

	if (hdev->lstore < 0x10) {
		hdev->lstore = 0;
		if (hdev->hwop.cntlddc(hdev, DDC_HDCP_14_LSTORE, 0))
			hdev->lstore += 1;
		else
			hdmitx_current_status(HDMITX_HDCP_AUTH_NO_14_KEYS_ERROR);
		if (hdev->hwop.cntlddc(hdev,
			DDC_HDCP_22_LSTORE, 0))
			hdev->lstore += 2;
		else
			hdmitx_current_status(HDMITX_HDCP_AUTH_NO_22_KEYS_ERROR);
	}
	if ((hdev->lstore & 0x3) == 0x3) {
		pos += snprintf(buf + pos, PAGE_SIZE, "14+22\n");
	} else {
		if (hdev->lstore & 0x1)
			pos += snprintf(buf + pos, PAGE_SIZE, "14\n");
		if (hdev->lstore & 0x2)
			pos += snprintf(buf + pos, PAGE_SIZE, "22\n");
		if ((hdev->lstore & 0xf) == 0)
			pos += snprintf(buf + pos, PAGE_SIZE, "00\n");
	}
	return pos;
}

static ssize_t hdcp_lstore_store(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	pr_info("hdcp: set lstore as %s\n", buf);
	if (strncmp(buf, "-1", 2) == 0)
		hdev->lstore = 0x0;
	if (strncmp(buf, "0", 1) == 0)
		hdev->lstore = 0x10;
	if (strncmp(buf, "11", 2) == 0)
		hdev->lstore = 0x11;
	if (strncmp(buf, "12", 2) == 0)
		hdev->lstore = 0x12;
	if (strncmp(buf, "13", 2) == 0)
		hdev->lstore = 0x13;

	return count;
}

static int rptxlstore;
static ssize_t hdcp_rptxlstore_show(struct device *dev,
				    struct device_attribute *attr,
				    char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	/* if current TX is not RP-TX, then return rptxlstore as 00 */
	/* hdcp_rptxlstore is used under only RP-TX */
	if (hdev->repeater_tx == 0) {
		pos += snprintf(buf + pos, PAGE_SIZE, "00\n");
		return pos;
	}

	if (rptxlstore < 0x10) {
		rptxlstore = 0;
		if (hdev->hwop.cntlddc(hdev,
					       DDC_HDCP_14_LSTORE,
					       0))
			rptxlstore += 1;
		if (hdev->hwop.cntlddc(hdev,
					       DDC_HDCP_22_LSTORE,
					       0))
			rptxlstore += 2;
	}
	if (rptxlstore & 0x1)
		pos += snprintf(buf + pos, PAGE_SIZE, "14\n");
	if (rptxlstore & 0x2)
		pos += snprintf(buf + pos, PAGE_SIZE, "22\n");
	if ((rptxlstore & 0xf) == 0)
		pos += snprintf(buf + pos, PAGE_SIZE, "00\n");
	return pos;
}

static ssize_t hdcp_rptxlstore_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	pr_info("hdcp: set lstore as %s\n", buf);
	if (strncmp(buf, "0", 1) == 0)
		rptxlstore = 0x10;
	if (strncmp(buf, "11", 2) == 0)
		rptxlstore = 0x11;
	if (strncmp(buf, "12", 2) == 0)
		rptxlstore = 0x12;
	if (strncmp(buf, "13", 2) == 0)
		rptxlstore = 0x13;

	return count;
}

static ssize_t div40_show(struct device *dev,
			  struct device_attribute *attr, char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	pos += snprintf(buf + pos, PAGE_SIZE, "%d\n", hdev->div40);

	return pos;
}

/* echo 1 > div40, force send 1:40 tmds bit clk ratio
 * echo 0 > div40, send 1:10 tmds bit clk ratio if scdc_present
 * echo 2 > div40, force send 1:10 tmds bit clk ratio
 */
static ssize_t div40_store(struct device *dev,
			   struct device_attribute *attr,
			   const char *buf, size_t count)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	hdev->hwop.cntlddc(hdev, DDC_SCDC_DIV40_SCRAMB, buf[0] - '0');

	return count;
}

static ssize_t hdcp_mode_show(struct device *dev,
			      struct device_attribute *attr,
			      char *buf)
{
	int pos = 0;
	unsigned int hdcp_ret = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	switch (hdev->hdcp_mode) {
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
	if (hdev->hdcp_ctl_lvl > 0 &&
	    hdev->hdcp_mode > 0) {
		hdcp_ret = hdev->hwop.cntlddc(hdev,
						      DDC_HDCP_GET_AUTH, 0);
		if (hdcp_ret == 1)
			pos += snprintf(buf + pos, PAGE_SIZE, ": succeed\n");
		else
			pos += snprintf(buf + pos, PAGE_SIZE, ": fail\n");
	}

	return pos;
}

static ssize_t hdcp_mode_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t count)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);
	enum hdmi_vic vic =
		hdev->tx_hw.getstate(&hdev->tx_hw, STAT_VIDEO_VIC, 0);

	if (hdev->tx_hw.cntlmisc(&hdev->tx_hw, MISC_TMDS_RXSENSE, 0) == 0)
		hdmitx_current_status(HDMITX_HDCP_DEVICE_NOT_READY_ERROR);
	/* there's risk:
	 * hdcp2.2 start auth-->enter early suspend, stop hdcp-->
	 * hdcp2.2 auth fail & timeout-->fall back to hdcp1.4, so
	 * hdcp running even no hdmi output-->resume, read EDID.
	 * EDID may read fail as hdcp may also access DDC simultaneously.
	 */
	mutex_lock(&hdmimode_mutex);
	if (!hdev->ready) {
		pr_info("hdmi signal not ready, should not set hdcp mode %s\n", buf);
		mutex_unlock(&hdmimode_mutex);
		return count;
	}
	pr_info(SYS "hdcp: set mode as %s\n", buf);
	hdev->hwop.cntlddc(hdev, DDC_HDCP_MUX_INIT, 1);
	hdev->hwop.cntlddc(hdev, DDC_HDCP_GET_AUTH, 0);
	if (strncmp(buf, "0", 1) == 0) {
		hdev->hdcp_mode = 0;
		hdev->hwop.cntlddc(hdev,
			DDC_HDCP_OP, HDCP14_OFF);
		hdmitx_hdcp_do_work(hdev);
		hdmitx_current_status(HDMITX_HDCP_NOT_ENABLED);
	}
	if (strncmp(buf, "1", 1) == 0) {
		char bksv[5] = {0};

		hdev->hwop.cntlddc(hdev, DDC_HDCP_GET_BKSV, (unsigned long)bksv);
		if (!hdcp_ksv_valid(bksv))
			hdmitx_current_status(HDMITX_HDCP_AUTH_READ_BKSV_ERROR);
		if (vic == HDMI_17_720x576p50_4x3 || vic == HDMI_18_720x576p50_16x9)
			usleep_range(500000, 500010);
		hdev->hdcp_mode = 1;
		hdmitx_hdcp_do_work(hdev);
		hdev->hwop.cntlddc(hdev,
			DDC_HDCP_OP, HDCP14_ON);
		hdmitx_current_status(HDMITX_HDCP_HDCP_1_ENABLED);
	}
	if (strncmp(buf, "2", 1) == 0) {
		hdev->hdcp_mode = 2;
		hdmitx_hdcp_do_work(hdev);
		hdev->hwop.cntlddc(hdev,
			DDC_HDCP_MUX_INIT, 2);
		hdmitx_current_status(HDMITX_HDCP_HDCP_2_ENABLED);
	}
	mutex_unlock(&hdmimode_mutex);

	return count;
}

static bool hdcp_sticky_mode;
static ssize_t hdcp_stickmode_show(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	int pos = 0;

	pos += snprintf(buf + pos, PAGE_SIZE, "%d\n", hdcp_sticky_mode);

	return pos;
}

static ssize_t hdcp_stickmode_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	if (buf[0] == '0')
		hdcp_sticky_mode = 0;
	if (buf[0] == '1')
		hdcp_sticky_mode = 1;

	return count;
}

static unsigned char hdcp_sticky_step;
static ssize_t hdcp_stickstep_show(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	int pos = 0;

	pos += snprintf(buf + pos, PAGE_SIZE, "%x\n", hdcp_sticky_step);
	if (hdcp_sticky_step)
		hdcp_sticky_step = 0;

	return pos;
}

static ssize_t hdcp_stickstep_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	if (isdigit(buf[0]))
		hdcp_sticky_step = buf[0] - '0';

	return count;
}

/* Indicate whether a rptx under repeater */
static ssize_t hdmi_repeater_tx_show(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	pos += snprintf(buf + pos, PAGE_SIZE, "%d\n",
		!!hdev->repeater_tx);

	return pos;
}

#include <linux/amlogic/media/vout/hdmi_tx/hdmi_rptx.h>

void direct_hdcptx14_opr(enum rptx_hdcp14_cmd cmd, void *args)
{
	int rst;
	struct hdmitx_dev *hdev = get_hdmitx_device();

	if (hdev->hdmi_init != 1)
		return;

	pr_info("%s[%d] cmd: %d\n", __func__, __LINE__, cmd);
	switch (cmd) {
	case RPTX_HDCP14_OFF:
		hdev->hdcp_mode = 0;
		hdev->hwop.cntlddc(hdev, DDC_HDCP_OP, HDCP14_OFF);
		break;
	case RPTX_HDCP14_ON:
		hdev->hdcp_mode = 1;
		hdev->hwop.cntlddc(hdev, DDC_HDCP_OP, HDCP14_ON);
		break;
	case RPTX_HDCP14_GET_AUTHST:
		rst = hdev->hwop.cntlddc(hdev, DDC_HDCP_GET_AUTH, 0);
		*(int *)args = rst;
		break;
	}
}
EXPORT_SYMBOL(direct_hdcptx14_opr);

static ssize_t hdcp_ctrl_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t count)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	if (hdev->hwop.cntlddc(hdev, DDC_HDCP_14_LSTORE, 0) == 0)
		return count;

	/* for repeater */
	if (hdev->repeater_tx) {
		if (hdev->log_level & HDCP_LOG)
			pr_info("hdmitx20: %s\n", buf);
		if (strncmp(buf, "rstop", 5) == 0) {
			if (strncmp(buf + 5, "14", 2) == 0)
				hdev->hwop.cntlddc(hdev, DDC_HDCP_OP,
					HDCP14_OFF);
			if (strncmp(buf + 5, "22", 2) == 0)
				hdev->hwop.cntlddc(hdev, DDC_HDCP_OP,
					HDCP22_OFF);
			hdev->hdcp_mode = 0;
			hdmitx_hdcp_do_work(hdev);
		}
		return count;
	}
	/* for non repeater */
	if (strncmp(buf, "stop", 4) == 0) {
		if (hdev->log_level & HDCP_LOG)
			pr_info("hdmitx20: %s\n", buf);
		if (strncmp(buf + 4, "14", 2) == 0)
			hdev->hwop.cntlddc(hdev, DDC_HDCP_OP, HDCP14_OFF);
		if (strncmp(buf + 4, "22", 2) == 0)
			hdev->hwop.cntlddc(hdev, DDC_HDCP_OP, HDCP22_OFF);
		hdev->hdcp_mode = 0;
		hdmitx_hdcp_do_work(hdev);
	}

	return count;
}

static ssize_t hdcp_ctrl_show(struct device *dev,
			      struct device_attribute *attr,
			      char *buf)
{
	return 0;
}

static ssize_t hdcp_ksv_info_show(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	int pos = 0, i;
	char bksv_buf[5];
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	hdev->hwop.cntlddc(hdev, DDC_HDCP_GET_BKSV,
		(unsigned long)bksv_buf);

	pos += snprintf(buf + pos, PAGE_SIZE, "HDCP14 BKSV: ");
	for (i = 0; i < 5; i++) {
		pos += snprintf(buf + pos, PAGE_SIZE, "%02x",
			bksv_buf[i]);
	}
	pos += snprintf(buf + pos, PAGE_SIZE, "  %s\n",
		hdcp_ksv_valid(bksv_buf) ? "Valid" : "Invalid");

	return pos;
}

static ssize_t hdmitx_cur_status_show(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	if (!kfifo_initialized(hdev->log_kfifo))
		return pos;
	if (kfifo_is_empty(hdev->log_kfifo))
		return pos;

	pos = kfifo_out(hdev->log_kfifo, buf, PAGE_SIZE);

	return pos;
}

/* Special FBC check */
static int check_fbc_special(unsigned char *edid_dat)
{
	if (edid_dat[250] == 0xfb && edid_dat[251] == 0x0c)
		return 1;
	else
		return 0;
}

static ssize_t hdcp_ver_show(struct device *dev,
			     struct device_attribute *attr,
			     char *buf)
{
	int pos = 0;
	u32 ver = 0U;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	if (check_fbc_special(&hdev->tx_comm.EDID_buf[0]) ||
	    check_fbc_special(&hdev->tx_comm.EDID_buf1[0])) {
		pos += snprintf(buf + pos, PAGE_SIZE, "00\n\r");
		return pos;
	}

	/* if TX don't have HDCP22 key, skip RX hdcp22 ver */
	if (hdev->hwop.cntlddc(hdev,
				       DDC_HDCP_22_LSTORE, 0) == 0)
		goto next;

	/* Detect RX support HDCP22 */
	mutex_lock(&getedid_mutex);
	ver = hdcp_rd_hdcp22_ver();
	mutex_unlock(&getedid_mutex);
	if (ver) {
		pos += snprintf(buf + pos, PAGE_SIZE, "22\n\r");
		pos += snprintf(buf + pos, PAGE_SIZE, "14\n\r");
		return pos;
	}
next:	/* Detect RX support HDCP14 */
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

	sense = hdev->tx_hw.cntlmisc(&hdev->tx_hw, MISC_TMDS_RXSENSE, 0);

	pos += snprintf(buf + pos, PAGE_SIZE, "%d", sense);
	return pos;
}

static ssize_t hdmi_used_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	pos += snprintf(buf + pos, PAGE_SIZE, "%d",
		hdev->already_used);
	return pos;
}
static ssize_t fake_plug_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d", hdev->tx_comm.hpd_state);
}

static ssize_t fake_plug_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t count)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	pr_info("hdmitx: fake plug %s\n", buf);

	if (strncmp(buf, "1", 1) == 0)
		hdev->tx_comm.hpd_state = 1;

	if (strncmp(buf, "0", 1) == 0)
		hdev->tx_comm.hpd_state = 0;

	/*notify to drm hdmi*/
	hdmitx_hpd_notify_unlocked(&hdev->tx_comm);
	extcon_set_state_sync(hdmitx_extcon_hdmi, EXTCON_DISP_HDMI,
		hdev->tx_comm.hpd_state);
	hdmitx_set_uevent(HDMITX_HPD_EVENT, hdev->tx_comm.hpd_state);

	return count;
}

static ssize_t rhpd_state_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int st;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	st = hdev->tx_hw.cntlmisc(&hdev->tx_hw, MISC_HPD_GPI_ST, 0);

	return snprintf(buf, PAGE_SIZE, "%d", hdev->rhpd_state);
}

static ssize_t max_exceed_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d", hdev->hdcp_max_exceed_state);
}

static ssize_t hdmi_init_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	pos += snprintf(buf + pos, PAGE_SIZE, "%d\n\r",
			hdev->hdmi_init);
	return pos;
}

static ssize_t ready_show(struct device *dev,
			  struct device_attribute *attr, char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	pos += snprintf(buf + pos, PAGE_SIZE, "%d\r\n",
		hdev->ready);
	return pos;
}

static ssize_t ready_store(struct device *dev,
			   struct device_attribute *attr,
			   const char *buf, size_t count)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	if (strncmp(buf, "0", 1) == 0)
		hdev->ready = 0;
	if (strncmp(buf, "1", 1) == 0)
		hdev->ready = 1;
	return count;
}

static ssize_t support_3d_show(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	pos += snprintf(buf + pos, PAGE_SIZE, "%d\n",
			hdev->tx_comm.rxcap.threeD_present);
	return pos;
}

static ssize_t sysctrl_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	pos += snprintf(buf + pos, PAGE_SIZE, "%d\r\n",
		hdev->systemcontrol_on);
	return pos;
}

static ssize_t sysctrl_enable_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	if (strncmp(buf, "0", 1) == 0)
		hdev->systemcontrol_on = false;
	if (strncmp(buf, "1", 1) == 0)
		hdev->systemcontrol_on = true;
	return count;
}

static ssize_t hdcp_ctl_lvl_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	pos += snprintf(buf + pos, PAGE_SIZE, "%d\r\n",
		hdev->hdcp_ctl_lvl);
	return pos;
}

static ssize_t hdcp_ctl_lvl_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long ctl_lvl = 0xf;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	pr_info("set hdcp_ctl_lvl: %s\n", buf);
	if (kstrtoul(buf, 10, &ctl_lvl) == 0) {
		if (ctl_lvl <= 2)
			hdev->hdcp_ctl_lvl = ctl_lvl;
	}
	return count;
}

static ssize_t hdmitx_drm_flag_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int pos = 0;
	int flag = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	/* notify hdcp_tx22: use flow of drm */
	if (hdev->hdcp_ctl_lvl > 0)
		flag = 1;
	else
		flag = 0;

	pos += snprintf(buf + pos, PAGE_SIZE, "%d", flag);
	return pos;
}

static ssize_t hdr_mute_frame_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int pos = 0;

	pos += snprintf(buf + pos, PAGE_SIZE, "%d\r\n", hdr_mute_frame);
	return pos;
}

static ssize_t hdr_mute_frame_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long mute_frame = 0;

	pr_info("set hdr_mute_frame: %s\n", buf);
	if (kstrtoul(buf, 10, &mute_frame) == 0)
		hdr_mute_frame = mute_frame;
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
	unsigned int val = 0;
	struct vinfo_s *info = NULL;

	if ((strncmp("0", buf, 1) == 0) || (strncmp("1", buf, 1) == 0) ||
	    (strncmp("2", buf, 1) == 0)) {
		val = buf[0] - '0';
	}

	if (val == hdev->tx_comm.hdr_priority)
		return count;
	info = hdmitx_get_current_vinfo(NULL);
	if (!info)
		return count;
	mutex_lock(&hdmimode_mutex);
	hdev->tx_comm.hdr_priority = val;
	if (hdev->tx_comm.hdr_priority == 1) {
		//clear dv support
		memset(&hdev->tx_comm.rxcap.dv_info, 0x00, sizeof(struct dv_info));
		hdmitx_vdev.dv_info = &dv_dummy;
		//restore hdr support
		memcpy(&hdev->tx_comm.rxcap.hdr_info,
			&hdev->tx_comm.rxcap.hdr_info2, sizeof(struct hdr_info));
		//restore BT2020 support
		hdev->tx_comm.rxcap.colorimetry_data = hdev->tx_comm.rxcap.colorimetry_data2;
		hdrinfo_to_vinfo(&info->hdr_info, hdev);
	} else if (hdev->tx_comm.hdr_priority == 2) {
		//clear dv support
		memset(&hdev->tx_comm.rxcap.dv_info, 0x00, sizeof(struct dv_info));
		hdmitx_vdev.dv_info = &dv_dummy;
		//clear hdr support
		memset(&hdev->tx_comm.rxcap.hdr_info, 0x00, sizeof(struct hdr_info));
		//clear BT2020 support
		hdev->tx_comm.rxcap.colorimetry_data = hdev->tx_comm.rxcap.colorimetry_data2 & 0x1F;
		memset(&info->hdr_info, 0, sizeof(struct hdr_info));
	} else {
		//restore dv support
		memcpy(&hdev->tx_comm.rxcap.dv_info,
			&hdev->tx_comm.rxcap.dv_info2, sizeof(struct dv_info));
		//restore hdr support
		memcpy(&hdev->tx_comm.rxcap.hdr_info, &hdev->tx_comm.rxcap.hdr_info2,
			sizeof(struct hdr_info));
		//restore BT2020 support
		hdev->tx_comm.rxcap.colorimetry_data = hdev->tx_comm.rxcap.colorimetry_data2;
		edidinfo_attach_to_vinfo(hdev);
	}
	/* force trigger plugin event
	 * hdmitx_set_uevent_state(HDMITX_HPD_EVENT, 0);
	 * hdmitx_set_uevent(HDMITX_HPD_EVENT, 1);
	 */
	mutex_unlock(&hdmimode_mutex);
	return count;
}

static ssize_t log_level_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int pos = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	pos += snprintf(buf + pos, PAGE_SIZE, "%x\r\n", hdev->log_level);
	return pos;
}

static ssize_t log_level_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long log_level = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	pr_info("set log_level: %s\n", buf);
	if (kstrtoul(buf, 16, &log_level) == 0)
		hdev->log_level = log_level;
	return count;
}

/* 0: no change
 * 1: force switch color space converter to 444,8bit
 * 2: force switch color space converter to 422,12bit
 * 3: force switch color space converter to rgb,8bit
 */
static ssize_t csc_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	u8 val = 0;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	if (isdigit(buf[0])) {
		val = buf[0] - '0';
		if (val != 0 && val != 1 && val != 2 && val != 3) {
			pr_info("set csc in 0 ~ 3\n");
			return count;
		}
		pr_info("set csc_en as %d\n", val);
	}

	hdev->tx_hw.cntlconfig(&hdev->tx_hw, CONFIG_CSC, val | CSC_UPDATE_AVI_CS);
	return count;
}

static ssize_t config_csc_en_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	if (strncmp(buf, "0", 1) == 0)
		hdev->tx_comm.config_csc_en = false;
	if (strncmp(buf, "1", 1) == 0)
		hdev->tx_comm.config_csc_en = true;
	pr_info("set config_csc_en %d\n", hdev->tx_comm.config_csc_en);
	return count;
}

static ssize_t dump_debug_reg_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	return hdmitx_debug_reg_dump(hdev, buf, PAGE_SIZE);
}

static ssize_t hdmitx_pkt_dump_show(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	return hdmitx_pkt_dump(hdev, buf, PAGE_SIZE);
}

#undef pr_fmt
#define pr_fmt(fmt) "" fmt
static ssize_t hdmitx_basic_config_show(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	int pos = 0;
	unsigned int reg_val, vsd_ieee_id[3];
	unsigned int reg_addr;
	unsigned char *conf;
	unsigned char *emp_data;
	unsigned int size;
	u32 ver = 0U;
	unsigned char *tmp;
	unsigned int hdcp_ret = 0;
	unsigned int colormetry;
	unsigned int hcnt, vcnt;
	enum hdmi_vic vic;
	enum hdmi_hdr_transfer hdr_transfer_feature;
	enum hdmi_hdr_color hdr_color_feature;
	struct dv_vsif_para *data;
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);
	struct hdmitx_audpara *audio_param = &hdev->cur_audio_param;

	pos += snprintf(buf + pos, PAGE_SIZE, "************\n");
	pos += snprintf(buf + pos, PAGE_SIZE, "hdmi_config_info\n");
	pos += snprintf(buf + pos, PAGE_SIZE, "************\n");

	pos += snprintf(buf + pos, PAGE_SIZE, "display_mode in:%s\n",
		get_vout_mode_internal());
	vic = hdev->tx_hw.getstate(&hdev->tx_hw, STAT_VIDEO_VIC, 0);
	pos += snprintf(buf + pos, PAGE_SIZE, "display_mode out:%s\n",
		hdmitx_mode_get_timing_name(vic));

	if (!memcmp(hdev->tx_comm.fmt_attr, "default,", 7)) {
		memset(hdev->tx_comm.fmt_attr, 0, sizeof(hdev->tx_comm.fmt_attr));
		hdmitx_fmt_attr(hdev);
	}
	pos += snprintf(buf + pos, PAGE_SIZE, "attr in:%s\n\r", hdev->tx_comm.fmt_attr);
	pos += snprintf(buf + pos, PAGE_SIZE, "attr out:");
	reg_addr = HDMITX_DWC_FC_AVICONF0;
	reg_val = hdmitx_rd_reg(reg_addr);
	switch (reg_val & 0x3) {
	case 0:
		conf = "RGB";
		break;
	case 1:
		conf = "422";
		break;
	case 2:
		conf = "444";
		break;
	case 3:
		conf = "420";
	}
	pos += snprintf(buf + pos, PAGE_SIZE, "%s,", conf);

	reg_addr = HDMITX_DWC_VP_PR_CD;
	reg_val = hdmitx_rd_reg(reg_addr);

	switch ((reg_val & 0xf0) >> 4) {
	case 0:
	case 4:
		conf = "8bit";
		break;
	case 5:
		conf = "10bit";
		break;
	case 6:
		conf = "12bit";
		break;
	case 7:
		conf = "16bit";
		break;
	default:
		conf = "reserved";
	}
	pos += snprintf(buf + pos, PAGE_SIZE, "%s\n", conf);

	pos += snprintf(buf + pos, PAGE_SIZE, "hdr_status in:");
	if (hdr_status_pos == 3 || hdev->hdr10plus_feature) {
		pos += snprintf(buf + pos, PAGE_SIZE, "HDR10Plus-VSIF");
	} else if (hdr_status_pos == 2) {
		if (hdev->hdmi_current_eotf_type == EOTF_T_DOLBYVISION)
			pos += snprintf(buf + pos, PAGE_SIZE,
				"DolbyVision-Std");
		else if (hdev->hdmi_current_eotf_type == EOTF_T_LL_MODE)
			pos += snprintf(buf + pos, PAGE_SIZE,
				"DolbyVision-Lowlatency");
		else
			pos += snprintf(buf + pos, PAGE_SIZE, "SDR");
	} else if (hdr_status_pos == 1) {
		if (hdev->hdr_transfer_feature == T_SMPTE_ST_2084)
			if (hdev->hdr_color_feature == C_BT2020)
				pos += snprintf(buf + pos, PAGE_SIZE,
					"HDR10-GAMMA_ST2084");
			else
				pos += snprintf(buf + pos, PAGE_SIZE, "HDR10-others");
		else if (hdev->hdr_color_feature == C_BT2020 &&
		    (hdev->hdr_transfer_feature == T_BT2020_10 ||
		     hdev->hdr_transfer_feature == T_HLG))
			pos += snprintf(buf + pos, PAGE_SIZE, "HDR10-GAMMA_HLG");
		else
			pos += snprintf(buf + pos, PAGE_SIZE, "SDR");
	} else {
		pos += snprintf(buf + pos, PAGE_SIZE, "SDR");
	}
	pos += snprintf(buf + pos, PAGE_SIZE, "\n");

	pos += snprintf(buf + pos, PAGE_SIZE, "hdr_status out:");
	if (hdr_status_pos == 2) {
		reg_addr = HDMITX_DWC_FC_VSDIEEEID0;
		reg_val = hdmitx_rd_reg(reg_addr);
		vsd_ieee_id[0] = reg_val;
		reg_addr = HDMITX_DWC_FC_VSDIEEEID1;
		reg_val = hdmitx_rd_reg(reg_addr);
		vsd_ieee_id[1] = reg_val;
		reg_addr = HDMITX_DWC_FC_VSDIEEEID2;
		reg_val = hdmitx_rd_reg(reg_addr);
		vsd_ieee_id[2] = reg_val;

		/*hdmi 1.4b VSIF only Support DolbyVision-Std*/
		if (vsd_ieee_id[0] == 0x03 && vsd_ieee_id[1] == 0x0C &&
		    vsd_ieee_id[2] == 0x00) {
			pos += snprintf(buf + pos, PAGE_SIZE,
					"DolbyVision-Std_hdmi 1.4b VSIF");
		} else if ((vsd_ieee_id[0] == 0x46) &&
			   (vsd_ieee_id[1] == 0xD0) &&
			   (vsd_ieee_id[2] == 0x00)) {
			reg_addr = HDMITX_DWC_FC_AVICONF0;
			reg_val = hdmitx_rd_reg(reg_addr);

			if ((reg_val & 0x3) == 0) {
				/*RGB*/
				reg_addr = HDMITX_DWC_FC_AVICONF2;
				reg_val = hdmitx_rd_reg(reg_addr);
				if (((reg_val & 0xc) >> 2) == 2)/*FULL*/
					pos += snprintf(buf + pos, PAGE_SIZE,
									"DolbyVision-Std");
				else/*LIM*/
					pos += snprintf(buf + pos, PAGE_SIZE,
									"DolbyVision-Lowlatency");
			} else if ((reg_val & 0x3) == 1) {
				/*422*/
				reg_addr = HDMITX_DWC_FC_AVICONF3;
				reg_val = hdmitx_rd_reg(reg_addr);

				if (((reg_val & 0xc) >> 2) == 0)/*LIM*/
					pos += snprintf(buf + pos, PAGE_SIZE,
									"DolbyVision-Lowlatency");
				else/*FULL*/
					pos += snprintf(buf + pos, PAGE_SIZE,
									"DolbyVision-Std");
			} else if ((reg_val & 0x3) == 2) {
		/*444 only one probability: DolbyVision-Lowlatency*/
				pos += snprintf(buf + pos, PAGE_SIZE,
						"DolbyVision-Lowlatency");
			}
		} else {
			pos += snprintf(buf + pos, PAGE_SIZE, "SDR");
		}
	} else {
		reg_addr = HDMITX_DWC_FC_DRM_PB00;
		reg_val = hdmitx_rd_reg(reg_addr);

		switch (reg_val) {
		case 0:
			conf = "SDR";
			break;
		case 1:
			conf = "HDR10-others";
			break;
		case 2:
			conf = "HDR10-GAMMA_ST2084";
			break;
		case 3:
			conf = "HDR10-GAMMA_HLG";
			break;
		default:
			conf = "SDR";
		}
		pos += snprintf(buf + pos, PAGE_SIZE, "%s\n", conf);
	}
	pos += snprintf(buf + pos, PAGE_SIZE, "\n");

	pos += snprintf(buf + pos, PAGE_SIZE, "******config******\n");
	pos += snprintf(buf + pos, PAGE_SIZE, "cur_VIC: %d\n", hdev->tx_comm.cur_VIC);
	if (hdev->cur_video_param)
		pos += snprintf(buf + pos, PAGE_SIZE,
			"cur_video_param->VIC=%d\n",
			hdev->cur_video_param->VIC);
	if (hdev->tx_comm.fmt_para.timing.vic != HDMI_0_UNKNOWN) {
		switch (hdev->tx_comm.fmt_para.cd) {
		case COLORDEPTH_24B:
			conf = "8bit";
			break;
		case COLORDEPTH_30B:
			conf = "10bit";
			break;
		case COLORDEPTH_36B:
			conf = "12bit";
			break;
		case COLORDEPTH_48B:
			conf = "16bit";
			break;
		default:
			conf = "reserved";
		}
		pos += snprintf(buf + pos, PAGE_SIZE, "colordepth: %s\n",
				conf);
		switch (hdev->tx_comm.fmt_para.cs) {
		case HDMI_COLORSPACE_RGB:
			conf = "RGB";
			break;
		case HDMI_COLORSPACE_YUV422:
			conf = "422";
			break;
		case HDMI_COLORSPACE_YUV444:
			conf = "444";
			break;
		case HDMI_COLORSPACE_YUV420:
			conf = "420";
			break;
		default:
			conf = "reserved";
		}
		pos += snprintf(buf + pos, PAGE_SIZE, "colorspace: %s\n",
				conf);
	}

	switch (hdev->tx_aud_cfg) {
	case 0:
		conf = "off";
		break;
	case 1:
		conf = "on";
		break;
	case 2:
		conf = "auto";
		break;
	default:
		conf = "none";
	}
	pos += snprintf(buf + pos, PAGE_SIZE, "audio config: %s\n", conf);

	switch (hdev->hdmi_audio_off_flag) {
	case 0:
		conf = "on";
		break;
	case 1:
		conf = "off";
		break;
	default:
		conf = "none";
	}
	pos += snprintf(buf + pos, PAGE_SIZE, "audio on/off: %s\n", conf);

	switch (audio_param->aud_src_if) {
	case AUD_SRC_IF_SPDIF:
		conf = "SPDIF";
		break;
	case AUD_SRC_IF_I2S:
		conf = "I2S";
		break;
	case AUD_SRC_IF_TDM:
		conf = "TDM";
		break;
	default:
		conf = "none";
	}
	pos += snprintf(buf + pos, PAGE_SIZE, "audio source: %s\n", conf);

	switch (hdev->cur_audio_param.type) {
	case CT_REFER_TO_STREAM:
		conf = "refer to stream header";
		break;
	case CT_PCM:
		conf = "L-PCM";
		break;
	case CT_AC_3:
		conf = "AC-3";
		break;
	case CT_MPEG1:
		conf = "MPEG1";
		break;
	case CT_MP3:
		conf = "MP3";
		break;
	case CT_MPEG2:
		conf = "MPEG2";
		break;
	case CT_AAC:
		conf = "AAC";
		break;
	case CT_DTS:
		conf = "DTS";
		break;
	case CT_ATRAC:
		conf = "ATRAC";
		break;
	case CT_ONE_BIT_AUDIO:
		conf = "One Bit Audio";
		break;
	case CT_DD_P:
		conf = "Dobly Digital+";
		break;
	case CT_DTS_HD:
		conf = "DTS_HD";
		break;
	case CT_MAT:
		conf = "MAT";
		break;
	case CT_DST:
		conf = "DST";
		break;
	case CT_WMA:
		conf = "WMA";
		break;
	default:
		conf = "MAX";
	}
	pos += snprintf(buf + pos, PAGE_SIZE, "audio type: %s\n", conf);

	switch (hdev->cur_audio_param.channel_num) {
	case CC_REFER_TO_STREAM:
		conf = "refer to stream header";
		break;
	case CC_2CH:
		conf = "2 channels";
		break;
	case CC_3CH:
		conf = "3 channels";
		break;
	case CC_4CH:
		conf = "4 channels";
		break;
	case CC_5CH:
		conf = "5 channels";
		break;
	case CC_6CH:
		conf = "6 channels";
		break;
	case CC_7CH:
		conf = "7 channels";
		break;
	case CC_8CH:
		conf = "8 channels";
		break;
	default:
		conf = "MAX";
	}
	pos += snprintf(buf + pos, PAGE_SIZE, "audio channel num: %s\n", conf);

	switch (hdev->cur_audio_param.sample_rate) {
	case FS_REFER_TO_STREAM:
		conf = "refer to stream header";
		break;
	case FS_32K:
		conf = "32kHz";
		break;
	case FS_44K1:
		conf = "44.1kHz";
		break;
	case FS_48K:
		conf = "48kHz";
		break;
	case FS_88K2:
		conf = "88.2kHz";
		break;
	case FS_96K:
		conf = "96kHz";
		break;
	case FS_176K4:
		conf = "176.4kHz";
		break;
	case FS_192K:
		conf = "192kHz";
		break;
	case FS_768K:
		conf = "768kHz";
		break;
	default:
		conf = "MAX";
	}
	pos += snprintf(buf + pos, PAGE_SIZE, "audio sample rate: %s\n", conf);

	switch (hdev->cur_audio_param.sample_size) {
	case SS_REFER_TO_STREAM:
		conf = "refer to stream header";
		break;
	case SS_16BITS:
		conf = "16bit";
		break;
	case SS_20BITS:
		conf = "20bit";
		break;
	case SS_24BITS:
		conf = "24bit";
		break;
	default:
		conf = "MAX";
	}
	pos += snprintf(buf + pos, PAGE_SIZE, "audio sample size: %s\n", conf);

	if (hdev->flag_3dfp)
		conf = "FramePacking";
	else if (hdev->flag_3dss)
		conf = "SidebySide";
	else if (hdev->flag_3dtb)
		conf = "TopButtom";
	else
		conf = "off";
	pos += snprintf(buf + pos, PAGE_SIZE, "3D config: %s\n", conf);
	pos += snprintf(buf + pos, PAGE_SIZE, "\n");

	pos += snprintf(buf + pos, PAGE_SIZE, "******hdcp******\n");
	pos += snprintf(buf + pos, PAGE_SIZE, "hdcp mode:");
	switch (hdev->hdcp_mode) {
	case 1:
		pos += snprintf(buf + pos, PAGE_SIZE, "14\n");
		break;
	case 2:
		pos += snprintf(buf + pos, PAGE_SIZE, "22\n");
		break;
	default:
		pos += snprintf(buf + pos, PAGE_SIZE, "off\n");
		break;
	}
	if (hdev->hdcp_ctl_lvl > 0 &&
	    hdev->hdcp_mode > 0) {
		hdcp_ret = hdev->hwop.cntlddc(hdev,
						      DDC_HDCP_GET_AUTH, 0);
		if (hdcp_ret == 1)
			pos += snprintf(buf + pos, PAGE_SIZE, ": succeed\n");
		else
			pos += snprintf(buf + pos, PAGE_SIZE, ": fail\n");
	}

	pos += snprintf(buf + pos, PAGE_SIZE, "hdcp_lstore:");
	if (hdev->repeater_tx == 1) {
		pos += snprintf(buf + pos, PAGE_SIZE, "00\n");
	} else {
		if (hdev->lstore < 0x10) {
			hdev->lstore = 0;
			if (hdev->hwop.cntlddc(hdev, DDC_HDCP_14_LSTORE, 0))
				hdev->lstore += 1;
			if (hdev->hwop.cntlddc(hdev,
				DDC_HDCP_22_LSTORE, 0))
				hdev->lstore += 2;
		}
		if ((hdev->lstore & 0x3) == 0x3) {
			pos += snprintf(buf + pos, PAGE_SIZE, "14+22\n");
		} else {
			if (hdev->lstore & 0x1)
				pos += snprintf(buf + pos, PAGE_SIZE, "14\n");
			if (hdev->lstore & 0x2)
				pos += snprintf(buf + pos, PAGE_SIZE, "22\n");
			if ((hdev->lstore & 0xf) == 0)
				pos += snprintf(buf + pos, PAGE_SIZE, "00\n");
		}
	}
	pos += snprintf(buf + pos, PAGE_SIZE, "hdcp_ver:");
	if (check_fbc_special(&hdev->tx_comm.EDID_buf[0]) ||
	    check_fbc_special(&hdev->tx_comm.EDID_buf1[0])) {
		pos += snprintf(buf + pos, PAGE_SIZE, "00\n\r");
	} else {
		if (hdev->hwop.cntlddc(hdev,
					       DDC_HDCP_22_LSTORE, 0) == 0)
			goto next;

			/* Detect RX support HDCP22 */
		mutex_lock(&getedid_mutex);
		ver = hdcp_rd_hdcp22_ver();
		mutex_unlock(&getedid_mutex);
		if (ver) {
			pos += snprintf(buf + pos, PAGE_SIZE, "22\n");
			pos += snprintf(buf + pos, PAGE_SIZE, "14\n");
		}
next:/* Detect RX support HDCP14 */
		/* Here, must assume RX support HDCP14, otherwise affect 1A-03 */
		pos += snprintf(buf + pos, PAGE_SIZE, "14\n");
	}

	pos += snprintf(buf + pos, PAGE_SIZE, "******scdc******\n");
	pos += snprintf(buf + pos, PAGE_SIZE, "div40:");
	pos += snprintf(buf + pos, PAGE_SIZE, "%d\n", hdev->div40);

	pos += snprintf(buf + pos, PAGE_SIZE, "******hdmi_pll******\n");
	pos += snprintf(buf + pos, PAGE_SIZE, "sspll:");
	pos += snprintf(buf + pos, PAGE_SIZE, "%d\n", hdev->sspll);
	pos += snprintf(buf + pos, PAGE_SIZE, "\n");

	pos += snprintf(buf + pos, PAGE_SIZE, "******dv_vsif_info******\n");
	data = &vsif_debug_info.data;
	pos += snprintf(buf + pos, PAGE_SIZE, "type: %u, tunnel: %u, sigsdr: %u\n",
		vsif_debug_info.type,
		vsif_debug_info.tunnel_mode,
		vsif_debug_info.signal_sdr);
	pos += snprintf(buf + pos, PAGE_SIZE, "dv_vsif_para:\n");
	pos += snprintf(buf + pos, PAGE_SIZE, "ver: %u len: %u\n",
		data->ver, data->length);
	pos += snprintf(buf + pos, PAGE_SIZE, "ll: %u dvsig: %u\n",
		data->vers.ver2.low_latency,
		data->vers.ver2.dobly_vision_signal);
	pos += snprintf(buf + pos, PAGE_SIZE, "bcMD: %u axMD: %u\n",
		data->vers.ver2.backlt_ctrl_MD_present,
		data->vers.ver2.auxiliary_MD_present);
	pos += snprintf(buf + pos, PAGE_SIZE, "PQhi: %u PQlow: %u\n",
		data->vers.ver2.eff_tmax_PQ_hi,
		data->vers.ver2.eff_tmax_PQ_low);
	pos += snprintf(buf + pos, PAGE_SIZE, "axrm: %u, axrv: %u, ",
		data->vers.ver2.auxiliary_runmode,
		data->vers.ver2.auxiliary_runversion);
	pos += snprintf(buf + pos, PAGE_SIZE, "axdbg: %u\n",
		data->vers.ver2.auxiliary_debug0);
	pos += snprintf(buf + pos, PAGE_SIZE, "\n");

	pos += snprintf(buf + pos, PAGE_SIZE, "***drm_config_data***\n");
	hdr_transfer_feature = (drm_config_data.features >> 8) & 0xff;
	hdr_color_feature = (drm_config_data.features >> 16) & 0xff;
	colormetry = (drm_config_data.features >> 30) & 0x1;
	pos += snprintf(buf + pos, PAGE_SIZE, "tf=%u, cf=%u, colormetry=%u\n",
		hdr_transfer_feature, hdr_color_feature,
		colormetry);
	pos += snprintf(buf + pos, PAGE_SIZE, "primaries:\n");
	for (vcnt = 0; vcnt < 3; vcnt++) {
		for (hcnt = 0; hcnt < 2; hcnt++)
			pos += snprintf(buf + pos, PAGE_SIZE, "%u, ",
			drm_config_data.primaries[vcnt][hcnt]);
		pos += snprintf(buf + pos, PAGE_SIZE, "\n");
	}
	pos += snprintf(buf + pos, PAGE_SIZE, "white_point: ");
	for (hcnt = 0; hcnt < 2; hcnt++)
		pos += snprintf(buf + pos, PAGE_SIZE, "%u, ",
		drm_config_data.white_point[hcnt]);
	pos += snprintf(buf + pos, PAGE_SIZE, "\n");
	pos += snprintf(buf + pos, PAGE_SIZE, "luminance: ");
	for (hcnt = 0; hcnt < 2; hcnt++)
		pos += snprintf(buf + pos, PAGE_SIZE, "%u, ", drm_config_data.luminance[hcnt]);
	pos += snprintf(buf + pos, PAGE_SIZE, "\n");
	pos += snprintf(buf + pos, PAGE_SIZE, "max_content: %u, ", drm_config_data.max_content);
	pos += snprintf(buf + pos, PAGE_SIZE, "max_frame_average: %u\n",
		drm_config_data.max_frame_average);
	pos += snprintf(buf + pos, PAGE_SIZE, "\n");
	pos += snprintf(buf + pos, PAGE_SIZE, "***hdr10p_config_data***\n");
	pos += snprintf(buf + pos, PAGE_SIZE, "appver: %u, tlum: %u, avgrgb: %u\n",
		hdr10p_config_data.application_version,
		hdr10p_config_data.targeted_max_lum,
		hdr10p_config_data.average_maxrgb);
	tmp = hdr10p_config_data.distribution_values;
	pos += snprintf(buf + pos, PAGE_SIZE, "distribution_values:\n");
	for (vcnt = 0; vcnt < 3; vcnt++) {
		for (hcnt = 0; hcnt < 3; hcnt++)
			pos += snprintf(buf + pos, PAGE_SIZE, "%u, ", tmp[vcnt * 3 + hcnt]);
		pos += snprintf(buf + pos, PAGE_SIZE, "\n");
	}
	pos += snprintf(buf + pos, PAGE_SIZE, "nbca: %u, knpx: %u, knpy: %u\n",
		hdr10p_config_data.num_bezier_curve_anchors,
		hdr10p_config_data.knee_point_x,
		hdr10p_config_data.knee_point_y);
	tmp = hdr10p_config_data.bezier_curve_anchors;
	pos += snprintf(buf + pos, PAGE_SIZE, "bezier_curve_anchors:\n");
	for (vcnt = 0; vcnt < 3; vcnt++) {
		for (hcnt = 0; hcnt < 3; hcnt++)
			pos += snprintf(buf + pos, PAGE_SIZE, "%u, ", tmp[vcnt * 3 + hcnt]);
		pos += snprintf(buf + pos, PAGE_SIZE, "\n");
	}
	pos += snprintf(buf + pos, PAGE_SIZE, "gof: %u, ndf: %u\n",
		hdr10p_config_data.graphics_overlay_flag,
		hdr10p_config_data.no_delay_flag);
	pos += snprintf(buf + pos, PAGE_SIZE, "\n");

	pos += snprintf(buf + pos, PAGE_SIZE, "***hdmiaud_config_data***\n");
		pos += snprintf(buf + pos, PAGE_SIZE,
			"type: %u, chnum: %u, samrate: %u, samsize: %u\n",
			hdmiaud_config_data.type,
			hdmiaud_config_data.channel_num,
			hdmiaud_config_data.sample_rate,
			hdmiaud_config_data.sample_size);
	emp_data = emp_config_data.data;
	pos += snprintf(buf + pos, PAGE_SIZE, "\n");
	pos += snprintf(buf + pos, PAGE_SIZE, "***emp_config_data***\n");
	pos += snprintf(buf + pos, PAGE_SIZE, "type: %u, size: %u\n",
		emp_config_data.type,
		emp_config_data.size);
	pos += snprintf(buf + pos, PAGE_SIZE, "data:\n");

	size = emp_config_data.size;
	for (vcnt = 0; vcnt < 8; vcnt++) {
		for (hcnt = 0; hcnt < 16; hcnt++) {
			if (vcnt * 16 + hcnt >= size)
				break;
			pos += snprintf(buf + pos, PAGE_SIZE, "%u, ", emp_data[vcnt * 16 + hcnt]);
		}
		if (vcnt * 16 + hcnt < size)
			pos += snprintf(buf + pos, PAGE_SIZE, "\n");
		else
			break;
	}
	pos += snprintf(buf + pos, PAGE_SIZE, "\n");
	return pos;
}

static ssize_t hdmi_config_info_show(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	return hdmitx_basic_config_show(dev, attr, buf);
}

#undef pr_fmt
#define pr_fmt(fmt) "hdmitx: " fmt
static ssize_t hdmirx_info_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int pos = 0;

	pr_info("************hdmirx_info************\n\n");

/*
	pos = hpd_state_show(dev, attr, buf);
	buf[pos] = '\0';
	pr_info("******hpd_edid_parsing******\n");
	pr_info("hpd:%s\t", buf);

	pos = edid_parsing_show(dev, attr, buf);
	buf[pos] = '\0';
	pr_info("edid_parsing:%s\n", buf);
	*/

/*
	pos = edid_show(dev, attr, buf);
	buf[pos] = '\0';
	pr_info("******edid******\n");
	pr_info("%s\n", buf);
*/
	pos = dc_cap_show(dev, attr, buf);
	buf[pos] = '\0';
	pr_info("******dc_cap******\n%s\n", buf);

/*
	pos = disp_cap_show(dev, attr, buf);
	buf[pos] = '\0';
	pr_info("******disp_cap******\n%s\n", buf);

	pos = dv_cap_show(dev, attr, buf);
	buf[pos] = '\0';
	pr_info("******dv_cap******\n%s\n", buf);

	pos = hdr_cap_show(dev, attr, buf);
	buf[pos] = '\0';
	pr_info("******hdr_cap******\n%s\n", buf);
*/

	pos = sink_type_show(dev, attr, buf);
	buf[pos] = '\0';
	pr_info("******sink_type******\n%s\n", buf);

	pos = aud_cap_show(dev, attr, buf);
	buf[pos] = '\0';
	pr_info("******aud_cap******\n%s\n", buf);

	pos = aud_ch_show(dev, attr, buf);
	buf[pos] = '\0';
	pr_info("******aud_ch******\n%s\n", buf);

/*
	pos = rawedid_show(dev, attr, buf);
	buf[pos] = '\0';
	pr_info("******rawedid******\n%s\n", buf);
*/
	memset(buf, 0, PAGE_SIZE);
	return 0;
}

void print_hsty_drm_config_data(void)
{
	unsigned int hdr_transfer_feature;
	unsigned int hdr_color_feature;
	struct master_display_info_s *drmcfg;
	unsigned int colormetry;
	unsigned int hcnt, vcnt;
	unsigned int arr_cnt, pr_loc;
	unsigned int print_num;

	pr_loc = hsty_drm_config_loc - 1;
	if (hsty_drm_config_num > 8)
		print_num = 8;
	else
		print_num = hsty_drm_config_num;
	pr_info("******drm_config_data have trans %d times******\n",
		hsty_drm_config_num);
	for (arr_cnt = 0; arr_cnt < print_num; arr_cnt++) {
		pr_info("***hsty_drm_config_data[%u]***\n", arr_cnt);
		drmcfg = &hsty_drm_config_data[pr_loc];
		hdr_transfer_feature = (drmcfg->features >> 8) & 0xff;
		hdr_color_feature = (drmcfg->features >> 16) & 0xff;
		colormetry = (drmcfg->features >> 30) & 0x1;
		pr_info("tf=%u, cf=%u, colormetry=%u\n",
			hdr_transfer_feature, hdr_color_feature,
			colormetry);

		pr_info("primaries:\n");
		for (vcnt = 0; vcnt < 3; vcnt++) {
			for (hcnt = 0; hcnt < 2; hcnt++)
				pr_info("%u, ", drmcfg->primaries[vcnt][hcnt]);
			pr_info("\n");
		}

		pr_info("white_point: ");
		for (hcnt = 0; hcnt < 2; hcnt++)
			pr_info("%u, ", drmcfg->white_point[hcnt]);
		pr_info("\n");

		pr_info("luminance: ");
		for (hcnt = 0; hcnt < 2; hcnt++)
			pr_info("%u, ", drmcfg->luminance[hcnt]);
		pr_info("\n");

		pr_info("max_content: %u, ", drmcfg->max_content);
		pr_info("max_frame_average: %u\n", drmcfg->max_frame_average);

		pr_loc = pr_loc > 0 ? pr_loc - 1 : 7;
	}
}

void print_hsty_vsif_config_data(void)
{
	struct dv_vsif_para *data;
	unsigned int arr_cnt, pr_loc;
	unsigned int print_num;

	pr_loc = hsty_vsif_config_loc - 1;
	if (hsty_vsif_config_num > 8)
		print_num = 8;
	else
		print_num = hsty_vsif_config_num;
	pr_info("******vsif_config_data have trans %d times******\n",
		hsty_vsif_config_num);
	for (arr_cnt = 0; arr_cnt < print_num; arr_cnt++) {
		pr_info("***hsty_vsif_config_data[%u]***\n", arr_cnt);
		data = &hsty_vsif_config_data[pr_loc].data;
		pr_info("***vsif_config_data***\n");
		pr_info("type: %u, tunnel: %u, sigsdr: %u\n",
			hsty_vsif_config_data[pr_loc].type,
			hsty_vsif_config_data[pr_loc].tunnel_mode,
			hsty_vsif_config_data[pr_loc].signal_sdr);
		pr_info("dv_vsif_para:\n");
		pr_info("ver: %u len: %u\n",
			data->ver, data->length);
		pr_info("ll: %u dvsig: %u\n",
			data->vers.ver2.low_latency,
			data->vers.ver2.dobly_vision_signal);
		pr_info("bcMD: %u axMD: %u\n",
			data->vers.ver2.backlt_ctrl_MD_present,
			data->vers.ver2.auxiliary_MD_present);
		pr_info("PQhi: %u PQlow: %u\n",
			data->vers.ver2.eff_tmax_PQ_hi,
			data->vers.ver2.eff_tmax_PQ_low);
		pr_info("axrm: %u, axrv: %u, ",
			data->vers.ver2.auxiliary_runmode,
			data->vers.ver2.auxiliary_runversion);
		pr_info("axdbg: %u\n",
			data->vers.ver2.auxiliary_debug0);
		pr_loc = pr_loc > 0 ? pr_loc - 1 : 7;
	}
}

void print_hsty_hdr10p_config_data(void)
{
	struct hdr10plus_para *data;
	unsigned int arr_cnt, pr_loc;
	unsigned int hcnt, vcnt;
	unsigned char *tmp;
	unsigned int print_num;

	pr_loc = hsty_hdr10p_config_loc - 1;
	if (hsty_hdr10p_config_num > 8)
		print_num = 8;
	else
		print_num = hsty_hdr10p_config_num;
	pr_info("******hdr10p_config_data have trans %d times******\n",
		hsty_hdr10p_config_num);
	for (arr_cnt = 0; arr_cnt < print_num; arr_cnt++) {
		pr_info("***hsty_hdr10p_config_data[%u]***\n", arr_cnt);
		data = &hsty_hdr10p_config_data[pr_loc];
		pr_info("appver: %u, tlum: %u, avgrgb: %u\n",
			data->application_version,
			data->targeted_max_lum,
			data->average_maxrgb);
		tmp = data->distribution_values;
		pr_info("distribution_values:\n");
		for (vcnt = 0; vcnt < 3; vcnt++) {
			for (hcnt = 0; hcnt < 3; hcnt++)
				pr_info("%u, ", tmp[vcnt * 3 + hcnt]);
			pr_info("\n");
		}
		pr_info("nbca: %u, knpx: %u, knpy: %u\n",
			data->num_bezier_curve_anchors,
			data->knee_point_x,
			data->knee_point_y);
		tmp = data->bezier_curve_anchors;
		pr_info("bezier_curve_anchors:\n");
		for (vcnt = 0; vcnt < 3; vcnt++) {
			for (hcnt = 0; hcnt < 3; hcnt++)
				pr_info("%u, ", tmp[vcnt * 3 + hcnt]);
			pr_info("\n");
		}
		pr_info("gof: %u, ndf: %u\n",
			data->graphics_overlay_flag,
			data->no_delay_flag);
		pr_loc = pr_loc > 0 ? pr_loc - 1 : 7;
	}
}

void print_hsty_hdmiaud_config_data(void)
{
	struct hdmitx_audpara *data;
	unsigned int arr_cnt, pr_loc;
	unsigned int print_num;

	pr_loc = hsty_hdmiaud_config_loc - 1;
	if (hsty_hdmiaud_config_num > 8)
		print_num = 8;
	else
		print_num = hsty_hdmiaud_config_num;
	pr_info("******hdmitx_audpara have trans %d times******\n",
		hsty_hdmiaud_config_num);
	for (arr_cnt = 0; arr_cnt < print_num; arr_cnt++) {
		pr_info("***hsty_hdmiaud_config_data[%u]***\n", arr_cnt);
		data = &hsty_hdmiaud_config_data[pr_loc];
		pr_info("type: %u, chnum: %u, samrate: %u, samsize: %u\n",
			data->type,	data->channel_num,
			data->sample_rate, data->sample_size);
		pr_loc = pr_loc > 0 ? pr_loc - 1 : 7;
	}
}
static ssize_t hdmi_hsty_config_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	print_hsty_drm_config_data();
	print_hsty_vsif_config_data();
	print_hsty_hdr10p_config_data();
	print_hsty_hdmiaud_config_data();
	memset(buf, 0, PAGE_SIZE);
	return 0;
}

static ssize_t hdcp22_top_reset_store(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(dev);

	mutex_lock(&hdmimode_mutex);
	/* should not reset hdcp2.2 after hdcp2.2 auth start */
	if (hdev->ready) {
		mutex_unlock(&hdmimode_mutex);
		return count;
	}
	pr_info("reset hdcp2.2 module after exit hdcp2.2 auth\n");
	hdev->tx_hw.cntlmisc(&hdev->tx_hw, MISC_HDCP_CLKDIS, 1);
	hdev->hwop.cntlddc(hdev, DDC_RESET_HDCP, 0);
	mutex_unlock(&hdmimode_mutex);
	return count;
}

static DEVICE_ATTR_RW(disp_mode);
static DEVICE_ATTR_RW(aud_mode);
static DEVICE_ATTR_RW(vid_mute);
static DEVICE_ATTR_RO(sink_type);
static DEVICE_ATTR_RW(config);
static DEVICE_ATTR_WO(debug);
static DEVICE_ATTR_RO(aud_cap);
static DEVICE_ATTR_RO(hdmi_hdr_status);
static DEVICE_ATTR_RO(dc_cap);
static DEVICE_ATTR_RO(allm_cap);
static DEVICE_ATTR_RW(allm_mode);
static DEVICE_ATTR_RW(aud_ch);
static DEVICE_ATTR_RW(swap);
static DEVICE_ATTR_RW(vic);
static DEVICE_ATTR_RW(avmute);
static DEVICE_ATTR_RW(sspll);
static DEVICE_ATTR_RW(rxsense_policy);
static DEVICE_ATTR_RW(cedst_policy);
static DEVICE_ATTR_RO(cedst_count);
static DEVICE_ATTR_RW(hdcp_clkdis);
static DEVICE_ATTR_RW(hdcp_pwr);
static DEVICE_ATTR_WO(hdcp_byp);
static DEVICE_ATTR_RW(hdcp_mode);
static DEVICE_ATTR_RW(hdcp_type_policy);
static DEVICE_ATTR_RW(hdcp_lstore);
static DEVICE_ATTR_RW(hdcp_rptxlstore);
static DEVICE_ATTR_RW(hdcp_repeater);
static DEVICE_ATTR_RW(hdcp_topo_info);
static DEVICE_ATTR_RW(hdcp22_type);
static DEVICE_ATTR_RW(hdcp_stickmode);
static DEVICE_ATTR_RW(hdcp_stickstep);
static DEVICE_ATTR_RO(hdmi_repeater_tx);
static DEVICE_ATTR_RO(hdcp22_base);
static DEVICE_ATTR_RW(div40);
static DEVICE_ATTR_RW(hdcp_ctrl);
static DEVICE_ATTR_RO(hdmitx_cur_status);
static DEVICE_ATTR_RO(hdcp_ksv_info);
static DEVICE_ATTR_RO(hdcp_ver);
static DEVICE_ATTR_RO(hdmi_used);
static DEVICE_ATTR_RO(rhpd_state);
static DEVICE_ATTR_RO(rxsense_state);
static DEVICE_ATTR_RO(max_exceed);
static DEVICE_ATTR_RW(fake_plug);
static DEVICE_ATTR_RO(hdmi_init);
static DEVICE_ATTR_RW(ready);
static DEVICE_ATTR_RO(support_3d);
static DEVICE_ATTR_RO(hdmirx_info);
static DEVICE_ATTR_RO(hdmi_hsty_config);
static DEVICE_ATTR_RW(sysctrl_enable);
static DEVICE_ATTR_RW(hdcp_ctl_lvl);
static DEVICE_ATTR_RO(hdmitx_drm_flag);
static DEVICE_ATTR_RW(hdr_mute_frame);
static DEVICE_ATTR_RW(log_level);
static DEVICE_ATTR_WO(csc);
static DEVICE_ATTR_WO(config_csc_en);
static DEVICE_ATTR_RO(hdmitx_basic_config);
static DEVICE_ATTR_RO(hdmi_config_info);
static DEVICE_ATTR_RO(hdmitx_pkt_dump);
static DEVICE_ATTR_RO(dump_debug_reg);
static DEVICE_ATTR_RW(hdr_priority_mode);
static DEVICE_ATTR_WO(hdcp22_top_reset);

static int hdmitx20_pre_enable_mode(struct hdmitx_common *tx_comm, struct hdmi_format_para *para)
{
	struct hdmitx_dev *hdev = to_hdmitx20_dev(tx_comm);
	struct hdmi_format_para *dev_para = &tx_comm->fmt_para;

	hdev->ready = 0;
	//TODO format para will be moved
	memcpy(dev_para, para, sizeof(struct hdmi_format_para));

	hdmitx_pre_display_init(hdev);
	return 0;
}

static int hdmitx20_enable_mode(struct hdmitx_common *tx_comm, struct hdmi_format_para *para)
{
	int ret;
	struct hdmitx_dev *hdev = to_hdmitx20_dev(tx_comm);

	/* if vic is HDMI_UNKNOWN, hdmitx_set_display will disable HDMI */
	tx_comm->cur_VIC = HDMI_0_UNKNOWN;
	ret = hdmitx_set_display(hdev, para->vic);

	if (ret >= 0) {
		hdev->hwop.cntl(hdev, HDMITX_AVMUTE_CNTL, AVMUTE_CLEAR);
		hdev->tx_comm.cur_VIC = para->vic;
		hdev->audio_param_update_flag = 1;
		hdev->auth_process_timer = AUTH_PROCESS_TIME;
	}
	if (hdev->tx_comm.cur_VIC == HDMI_0_UNKNOWN) {
		if (hdev->hpdmode == 2) {
			/* edid will be read again when hpd is muxed
			 * and it is high
			 */
			hdmitx_edid_clear(hdev);
			hdev->mux_hpd_if_pin_high_flag = 0;
		}
		/* If current display is NOT panel, needn't TURNOFF_HDMIHW */
		if (strncmp(para->name, "panel", 5) == 0) {
			hdev->hwop.cntl(hdev, HDMITX_HWCMD_TURNOFF_HDMIHW,
				(hdev->hpdmode == 2) ? 1 : 0);
		}
	}
	hdmitx_set_audio(hdev, &hdev->cur_audio_param);

	return 0;
}

static int hdmitx20_post_enable_mode(struct hdmitx_common *tx_comm, struct hdmi_format_para *para)
{
	struct hdmitx_dev *hdev = to_hdmitx20_dev(tx_comm);

	if (hdev->cedst_policy) {
		cancel_delayed_work(&hdev->work_cedst);
		queue_delayed_work(hdev->cedst_wq, &hdev->work_cedst, 0);
	}
	hdev->output_blank_flag = 1;
	hdev->ready = 1;
	edidinfo_attach_to_vinfo(hdev);
	update_vinfo_from_formatpara();
	return 0;
}

static int hdmitx20_disable_mode(struct hdmitx_common *tx_comm, struct hdmi_format_para *para)
{
	return 0;
}

static struct hdmitx_ctrl_ops tx20_ctrl_ops = {
	.pre_enable_mode = hdmitx20_pre_enable_mode,
	.enable_mode = hdmitx20_enable_mode,
	.post_enable_mode = hdmitx20_post_enable_mode,
	.disable_mode = hdmitx20_disable_mode,
};

#ifdef CONFIG_AMLOGIC_VOUT_SERVE
static struct vinfo_s *hdmitx_get_current_vinfo(void *data)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();

	return &hdev->tx_comm.hdmitx_vinfo;
}

static int hdmitx_set_current_vmode(enum vmode_e mode, void *data)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();

	pr_info("%s[%d]\n", __func__, __LINE__);

	if (!(mode & VMODE_INIT_BIT_MASK)) {
		pr_err("warning, echo /sys/class/display/mode is disabled\n");
	} else {
		pr_info("alread display in uboot\n");
		edidinfo_attach_to_vinfo(hdev);
		update_vinfo_from_formatpara();
		/* Should be started at end of output */
		if (hdev->cedst_policy) {
			cancel_delayed_work(&hdev->work_cedst);
			queue_delayed_work(hdev->cedst_wq, &hdev->work_cedst, 0);
		}
	}
	return 0;
}

static enum vmode_e hdmitx_validate_vmode(char *mode, unsigned int frac,
					  void *data)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();
	struct vinfo_s *vinfo = &hdev->tx_comm.hdmitx_vinfo;
	const struct hdmi_timing *timing = 0;

	/* vout validate vmode only used to confirm the mode is
	 * supported by this server. And dont check with edid,
	 * maybe we dont have edid when this function called.
	 */
	timing = hdmitx_mode_match_timing_name(mode);
	if (hdmitx_common_validate_vic(&hdev->tx_comm, timing->vic) == 0) {
		/*should save mode name to vinfo, will be used in set_vmode*/
		calc_vinfo_from_hdmi_timing(timing, vinfo);
		return VMODE_HDMI;
	}

	pr_err("%s validate %s fail\n", __func__, mode);
	return VMODE_MAX;
}

static int hdmitx_vmode_is_supported(enum vmode_e mode, void *data)
{
	if ((mode & VMODE_MODE_BIT_MASK) == VMODE_HDMI)
		return true;
	else
		return false;
}

static int hdmitx_module_disable(enum vmode_e cur_vmod, void *data)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();

	hdev->tx_hw.cntlconfig(&hdev->tx_hw, CONF_CLR_AVI_PACKET, 0);
	hdev->tx_hw.cntlconfig(&hdev->tx_hw, CONF_CLR_VSDB_PACKET, 0);
	hdev->tx_hw.cntlmisc(&hdev->tx_hw, MISC_TMDS_PHY_OP, TMDS_PHY_DISABLE);

	hdmitx_format_para_reset(&hdev->tx_comm.fmt_para);

	hdmitx_validate_vmode("null", 0, NULL);
	if (hdev->cedst_policy)
		cancel_delayed_work(&hdev->work_cedst);
	if (hdev->rxsense_policy)
		queue_delayed_work(hdev->rxsense_wq, &hdev->work_rxsense, 0);

	return 0;
}

static int hdmitx_vout_state;
static int hdmitx_vout_set_state(int index, void *data)
{
	hdmitx_vout_state |= (1 << index);
	return 0;
}

static int hdmitx_vout_clr_state(int index, void *data)
{
	hdmitx_vout_state &= ~(1 << index);
	return 0;
}

static int hdmitx_vout_get_state(void *data)
{
	return hdmitx_vout_state;
}

/* if cs/cd/frac_rate is changed, then return 0 */
static int hdmitx_check_same_vmodeattr(char *name, void *data)
{
	pr_info("not support anymore\n");
	return 0;
}

static int hdmitx_vout_get_disp_cap(char *buf, void *data)
{
	int pos = 0;

	pos += snprintf(buf + pos, PAGE_SIZE, "check disp_cap sysfs node in hdmitx.\n");
	return pos;
}

static void hdmitx_set_bist(unsigned int num, void *data)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();

	if (hdev->hwop.debug_bist)
		hdev->hwop.debug_bist(hdev, num);
}

static struct vout_server_s hdmitx_vout_server = {
	.name = "hdmitx_vout_server",
	.op = {
		.get_vinfo = hdmitx_get_current_vinfo,
		.set_vmode = hdmitx_set_current_vmode,
		.validate_vmode = hdmitx_validate_vmode,
		.check_same_vmodeattr = hdmitx_check_same_vmodeattr,
		.vmode_is_supported = hdmitx_vmode_is_supported,
		.disable = hdmitx_module_disable,
		.set_state = hdmitx_vout_set_state,
		.clr_state = hdmitx_vout_clr_state,
		.get_state = hdmitx_vout_get_state,
		.get_disp_cap = hdmitx_vout_get_disp_cap,
		.set_vframe_rate_hint = NULL,
		.get_vframe_rate_hint = NULL,
		.set_bist = hdmitx_set_bist,
#ifdef CONFIG_PM
		.vout_suspend = NULL,
		.vout_resume = NULL,
#endif
	},
	.data = NULL,
};
#endif

#ifdef CONFIG_AMLOGIC_VOUT2_SERVE
static struct vout_server_s hdmitx_vout2_server = {
	.name = "hdmitx_vout2_server",
	.op = {
		.get_vinfo = hdmitx_get_current_vinfo,
		.set_vmode = hdmitx_set_current_vmode,
		.validate_vmode = hdmitx_validate_vmode,
		.check_same_vmodeattr = hdmitx_check_same_vmodeattr,
		.vmode_is_supported = hdmitx_vmode_is_supported,
		.disable = hdmitx_module_disable,
		.set_state = hdmitx_vout_set_state,
		.clr_state = hdmitx_vout_clr_state,
		.get_state = hdmitx_vout_get_state,
		.get_disp_cap = hdmitx_vout_get_disp_cap,
		.set_vframe_rate_hint = NULL,
		.get_vframe_rate_hint = NULL,
		.set_bist = hdmitx_set_bist,
#ifdef CONFIG_PM
		.vout_suspend = NULL,
		.vout_resume = NULL,
#endif
	},
	.data = NULL,
};
#endif

#if IS_ENABLED(CONFIG_AMLOGIC_SND_SOC)

#include <linux/soundcard.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/initval.h>
#include <sound/control.h>

static struct rate_map_fs map_fs[] = {
	{0,	  FS_REFER_TO_STREAM},
	{32000,  FS_32K},
	{44100,  FS_44K1},
	{48000,  FS_48K},
	{88200,  FS_88K2},
	{96000,  FS_96K},
	{176400, FS_176K4},
	{192000, FS_192K},
};

static enum hdmi_audio_fs aud_samp_rate_map(unsigned int rate)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(map_fs); i++) {
		if (map_fs[i].rate == rate)
			return map_fs[i].fs;
	}
	pr_info(AUD "get FS_MAX\n");
	return FS_MAX;
}

static unsigned char *aud_type_string[] = {
	"CT_REFER_TO_STREAM",
	"CT_PCM",
	"CT_AC_3",
	"CT_MPEG1",
	"CT_MP3",
	"CT_MPEG2",
	"CT_AAC",
	"CT_DTS",
	"CT_ATRAC",
	"CT_ONE_BIT_AUDIO",
	"CT_DOLBY_D",
	"CT_DTS_HD",
	"CT_MAT",
	"CT_DST",
	"CT_WMA",
	"CT_MAX",
};

static struct size_map aud_size_map_ss[] = {
	{0,	 SS_REFER_TO_STREAM},
	{16,	SS_16BITS},
	{20,	SS_20BITS},
	{24,	SS_24BITS},
	{32,	SS_MAX},
};

static enum hdmi_audio_sampsize aud_size_map(unsigned int bits)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(aud_size_map_ss); i++) {
		if (bits == aud_size_map_ss[i].sample_bits)
			return aud_size_map_ss[i].ss;
	}
	pr_info(AUD "get SS_MAX\n");
	return SS_MAX;
}

static bool hdmitx_set_i2s_mask(char ch_num, char ch_msk)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();
	static unsigned int update_flag = -1;

	pr_debug("%s[%d] ch_num %d ch_msk %d\n", __func__, __LINE__, ch_num, ch_msk);
	if (!(ch_num == 2 || ch_num == 4 ||
	      ch_num == 6 || ch_num == 8)) {
		pr_info("err chn setting, must be 2, 4, 6 or 8, Rst as def\n");
		hdev->aud_output_ch = 0;
		if (update_flag != hdev->aud_output_ch) {
			update_flag = hdev->aud_output_ch;
			hdev->hdmi_ch = 0;
		}
		return 0;
	}
	if (ch_msk == 0) {
		pr_info("err chn msk, must larger than 0\n");
		return 0;
	}
	hdev->aud_output_ch = (ch_num << 4) + ch_msk;
	if (update_flag != hdev->aud_output_ch) {
		update_flag = hdev->aud_output_ch;
		hdev->hdmi_ch = 0;
	}
	return 1;
}

static int hdmitx_notify_callback_a(struct notifier_block *block,
				    unsigned long cmd, void *para);
static struct notifier_block hdmitx_notifier_nb_a = {
	.notifier_call	= hdmitx_notify_callback_a,
};

static int hdmitx_notify_callback_a(struct notifier_block *block,
				    unsigned long cmd, void *para)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();
	struct aud_para *aud_param = (struct aud_para *)para;
	struct hdmitx_audpara *audio_param = &hdev->cur_audio_param;
	enum hdmi_audio_fs n_rate = aud_samp_rate_map(aud_param->rate);
	enum hdmi_audio_sampsize n_size = aud_size_map(aud_param->size);

	if (aud_param->prepare) {
		aud_param->prepare = 0;
		hdev->tx_hw.cntlmisc(&hdev->tx_hw, MISC_AUDIO_ACR_CTRL, 0);
		hdev->tx_hw.cntlmisc(&hdev->tx_hw, MISC_AUDIO_PREPARE, 0);
		audio_param->type = CT_PREPARE;
		pr_info("audio prepare\n");
		return 0;
	}
	hdev->audio_param_update_flag = 0;
	hdev->audio_notify_flag = 0;
	if (hdmitx_set_i2s_mask(aud_param->chs, aud_param->i2s_ch_mask))
		hdev->audio_param_update_flag = 1;
	pr_info("type:%lu rate:%d size:%d chs:%d fifo_rst:%d aud_src_if:%d\n",
		cmd, n_rate, n_size, aud_param->chs, aud_param->fifo_rst,
		aud_param->aud_src_if);
	if (aud_param->aud_src_if == AUD_SRC_IF_SPDIF) {
		pr_debug("%s[%d] aud_src_if is %d, aud_output_ch is 0x%x, reset aud_output_ch as 0\n",
			__func__, __LINE__, aud_param->aud_src_if, hdev->aud_output_ch);
		hdev->aud_output_ch = 0;
	} else {
		if (hdev->aud_output_ch == 0) {
			hdev->aud_output_ch = (2 << 4) + 1;
			pr_debug("%s[%d] aud_src_if is %d, set default aud_output_ch 0x%x\n",
				__func__, __LINE__, aud_param->aud_src_if, hdev->aud_output_ch);
		}
	}
	if (audio_param->sample_rate != n_rate) {
		/* if the audio sample rate or type changes, stop ACR firstly */
		audio_param->sample_rate = n_rate;
		hdev->audio_param_update_flag = 1;
	}

	if (audio_param->type != cmd) {
		/* if the audio sample rate or type changes, stop ACR firstly */
		audio_param->type = cmd;
		pr_info(AUD "aout notify format %s\n",
			aud_type_string[audio_param->type & 0xff]);
		hdev->audio_param_update_flag = 1;
	}

	if (audio_param->sample_size != n_size) {
		audio_param->sample_size = n_size;
		hdev->audio_param_update_flag = 1;
	}

	if (audio_param->channel_num != (aud_param->chs - 1)) {
		int chnum = aud_param->chs;
		int lane_cnt = chnum / 2;
		int lane_mask = (1 << lane_cnt) - 1;

		pr_info(AUD "aout notify channel num: %d\n", chnum);
		audio_param->channel_num = chnum - 1;
		if (cmd == CT_PCM && chnum > 2)
			hdev->aud_output_ch = chnum << 4 | lane_mask;
		else
			hdev->aud_output_ch = 0;
		hdev->audio_param_update_flag = 1;
	}

	if (audio_param->aud_src_if != aud_param->aud_src_if) {
		pr_info("cur aud_src_if %d, new aud_src_if: %d\n",
			audio_param->aud_src_if, aud_param->aud_src_if);
		audio_param->aud_src_if = aud_param->aud_src_if;
		hdev->audio_param_update_flag = 1;
	}

	if (hdev->audio_param_update_flag == 0)
		;
	else
		hdev->audio_notify_flag = 1;

	if ((!(hdev->hdmi_audio_off_flag)) &&
	    hdev->audio_param_update_flag) {
		/* plug-in & update audio param */
		if (hdev->tx_comm.hpd_state == 1) {
			hdev->aud_notify_update = 1;
			hdmitx_set_audio(hdev, &hdev->cur_audio_param);
			hdev->aud_notify_update = 0;
			if (hdev->audio_notify_flag == 1 ||
			    hdev->audio_step == 1) {
				hdev->audio_notify_flag = 0;
				hdev->audio_step = 0;
			}
			hdev->audio_param_update_flag = 0;
			pr_info(AUD "set audio param\n");
		}
	}
	if (aud_param->fifo_rst)
		hdev->tx_hw.cntlmisc(&hdev->tx_hw, MISC_AUDIO_RESET, 1);

	return 0;
}

#endif

static void hdmitx_get_edid(struct hdmitx_dev *hdev)
{
	unsigned long flags = 0;
	struct hdmitx_common *tx_comm = &hdev->tx_comm;

	mutex_lock(&getedid_mutex);
	/* TODO hdmitx_edid_ram_buffer_clear(hdev); */
	hdev->hwop.cntlddc(hdev, DDC_RESET_EDID, 0);
	hdev->hwop.cntlddc(hdev, DDC_PIN_MUX_OP, PIN_MUX);
	/* start reading edid first time */
	hdev->hwop.cntlddc(hdev, DDC_EDID_READ_DATA, 0);
	hdev->hwop.cntlddc(hdev, DDC_EDID_GET_DATA, 0);
	if (hdmitx_edid_is_all_zeros(hdev->tx_comm.EDID_buf)) {
		hdev->hwop.cntlddc(hdev, DDC_GLITCH_FILTER_RESET, 0);
		hdev->hwop.cntlddc(hdev, DDC_EDID_READ_DATA, 0);
		hdev->hwop.cntlddc(hdev, DDC_EDID_GET_DATA, 0);
	}
	/* If EDID is not correct at first time, then retry */
	if (!check_dvi_hdmi_edid_valid(hdev->tx_comm.EDID_buf)) {
		struct timespec64 kts;
		struct rtc_time tm;

		msleep(20);
		ktime_get_real_ts64(&kts);
		rtc_time64_to_tm(kts.tv_sec, &tm);
		if (hdev->hdmitx_gpios_scl != -EPROBE_DEFER)
			pr_info("UTC+0 %ptRd %ptRt DDC SCL %s\n", &tm, &tm,
			gpio_get_value(hdev->hdmitx_gpios_scl) ? "HIGH" : "LOW");
		if (hdev->hdmitx_gpios_sda != -EPROBE_DEFER)
			pr_info("UTC+0 %ptRd %ptRt DDC SDA %s\n", &tm, &tm,
			gpio_get_value(hdev->hdmitx_gpios_sda) ? "HIGH" : "LOW");
		msleep(80);

		/* start reading edid second time */
		hdev->hwop.cntlddc(hdev, DDC_EDID_READ_DATA, 0);
		hdev->hwop.cntlddc(hdev, DDC_EDID_GET_DATA, 1);
		if (hdmitx_edid_is_all_zeros(hdev->tx_comm.EDID_buf1)) {
			hdev->hwop.cntlddc(hdev, DDC_GLITCH_FILTER_RESET, 0);
			hdev->hwop.cntlddc(hdev, DDC_EDID_READ_DATA, 0);
			hdev->hwop.cntlddc(hdev, DDC_EDID_GET_DATA, 1);
		}
	}
	spin_lock_irqsave(&hdev->edid_spinlock, flags);
	hdmitx_edid_clear(hdev);
	hdmitx_edid_parse(hdev);
	if (tx_comm->hdr_priority == 1) { /* clear dv_info */
		struct dv_info *dv = &hdev->tx_comm.rxcap.dv_info;

		memset(dv, 0, sizeof(struct dv_info));
		pr_info("clear dv_info\n");
	}
	if (tx_comm->hdr_priority == 2) { /* clear dv_info/hdr_info */
		struct dv_info *dv = &hdev->tx_comm.rxcap.dv_info;
		struct hdr_info *hdr = &hdev->tx_comm.rxcap.hdr_info;

		memset(dv, 0, sizeof(struct dv_info));
		memset(hdr, 0, sizeof(struct hdr_info));
		pr_info("clear dv_info/hdr_info\n");
	}
	spin_unlock_irqrestore(&hdev->edid_spinlock, flags);
	hdmitx_event_notify(HDMITX_PHY_ADDR_VALID, &hdev->physical_addr);
	hdmitx_edid_buf_compare_print(hdev);

	mutex_unlock(&getedid_mutex);
}

static void hdmitx_rxsense_process(struct work_struct *work)
{
	int sense;
	struct hdmitx_dev *hdev = container_of((struct delayed_work *)work,
		struct hdmitx_dev, work_rxsense);

	sense = hdev->tx_hw.cntlmisc(&hdev->tx_hw, MISC_TMDS_RXSENSE, 0);
	hdmitx_set_uevent(HDMITX_RXSENSE_EVENT, sense);
	queue_delayed_work(hdev->rxsense_wq, &hdev->work_rxsense, HZ);
}

static void hdmitx_cedst_process(struct work_struct *work)
{
	int ced;
	struct hdmitx_dev *hdev = container_of((struct delayed_work *)work,
		struct hdmitx_dev, work_cedst);

	ced = hdev->tx_hw.cntlmisc(&hdev->tx_hw, MISC_TMDS_CEDST, 0);
	/* firstly send as 0, then real ced, A trigger signal */
	hdmitx_set_uevent(HDMITX_CEDST_EVENT, 0);
	hdmitx_set_uevent(HDMITX_CEDST_EVENT, ced);
	queue_delayed_work(hdev->cedst_wq, &hdev->work_cedst, HZ);
	queue_delayed_work(hdev->cedst_wq, &hdev->work_cedst, HZ);
}

/*only for first time plugout */
bool is_tv_changed(void)
{
	char invalidchecksum[11] = {
		'i', 'n', 'v', 'a', 'l', 'i', 'd', 'c', 'r', 'c', '\0'
	};
	char emptychecksum[11] = {0};
	bool ret = false;
	struct hdmitx_dev *hdev = get_hdmitx_device();
	struct hdmitx_common *tx_comm = &hdev->tx_comm;

	if (memcmp(tx_comm->hdmichecksum, hdev->tx_comm.rxcap.chksum, 10) &&
		memcmp(emptychecksum, hdev->tx_comm.rxcap.chksum, 10) &&
		memcmp(invalidchecksum, tx_comm->hdmichecksum, 10)) {
		ret = true;
		pr_info("hdmi crc is diff between uboot and kernel\n");
	}

	return ret;
}

static void hdmitx_hpd_plugin_handler(struct work_struct *work)
{
	char bksv_buf[5];
	struct vinfo_s *info = NULL;
	struct hdmitx_dev *hdev = container_of((struct delayed_work *)work,
		struct hdmitx_dev, work_hpd_plugin);

	mutex_lock(&setclk_mutex);
	mutex_lock(&hdmimode_mutex);
	hdev->already_used = 1;
	if (!(hdev->hdmitx_event & (HDMI_TX_HPD_PLUGIN))) {
		mutex_unlock(&hdmimode_mutex);
		mutex_unlock(&setclk_mutex);
		return;
	}
	/* clear plugin event asap, as there may be
	 * very short low pulse of HPD during edid reading
	 * and cause EDID abnormal, after this low hpd pulse,
	 * will read edid again, and notify system to update.
	 */
	hdev->hdmitx_event &= ~HDMI_TX_HPD_PLUGIN;

	if (hdev->rxsense_policy) {
		cancel_delayed_work(&hdev->work_rxsense);
		queue_delayed_work(hdev->rxsense_wq, &hdev->work_rxsense, 0);
	}
	hdev->previous_error_event = 0;
	pr_info(SYS "plugin\n");
	hdmitx_current_status(HDMITX_HPD_PLUGIN);
	/* there maybe such case:
	 * hpd rising & hpd level high (0.6S > HZ/2)-->
	 * plugin handler-->hpd falling & hpd level low(0.05S)-->
	 * continue plugin handler, EDID read normal,
	 * post plugin uevent-->
	 * plugout handler(may be filtered and skipped):
	 * stop hdcp/clear edid, post plugout uevent-->
	 * system plugin handle: set hdmi mode/hdcp auth-->
	 * system plugout handle: set non-hdmi mode(but hdcp is still running)-->
	 * hpd rising & keep level high-->plugin handler, EDID read abnormal
	 * as hdcp auth is running and may access DDC when reading EDID.
	 * so need to disable hdcp auth before EDID reading
	 */
	if (hdev->hdcp_mode != 0 && !hdev->ready) {
		pr_info("hdcp: %d should not be enabled before signal ready\n",
			hdev->hdcp_mode);
		drm_hdmitx_hdcp_disable(hdev->hdcp_mode);
	}
	/* there's such case: plugin irq->hdmitx resume + read EDID +
	 * resume uevent->mode setting + hdcp auth->plugin handler read
	 * EDID, now EDID already read done and hdcp already started,
	 * not read EDID again.
	 */
	if (!hdmitx_edid_done) {
		if (hdev->data->chip_type >= MESON_CPU_ID_G12A)
			hdev->tx_hw.cntlmisc(&hdev->tx_hw, MISC_I2C_RESET, 0);
		hdmitx_get_edid(hdev);
		hdmitx_edid_done = true;
	}
	/* start reading E-EDID */
	if (hdev->repeater_tx)
		rx_repeat_hpd_state(1);
	hdev->cedst_policy = hdev->cedst_en & hdev->tx_comm.rxcap.scdc_present;
	if (hdev->tx_comm.rxcap.ieeeoui != HDMI_IEEE_OUI)
		hdev->tx_hw.cntlconfig(&hdev->tx_hw,
			CONF_HDMI_DVI_MODE, DVI_MODE);
	else
		hdev->tx_hw.cntlconfig(&hdev->tx_hw,
			CONF_HDMI_DVI_MODE, HDMI_MODE);
	mutex_lock(&getedid_mutex);
	if (hdev->data->chip_type < MESON_CPU_ID_G12A)
		hdev->tx_hw.cntlmisc(&hdev->tx_hw, MISC_I2C_REACTIVE, 0);
	mutex_unlock(&getedid_mutex);
	if (hdev->repeater_tx) {
		if (check_fbc_special(&hdev->tx_comm.EDID_buf[0]) ||
		    check_fbc_special(&hdev->tx_comm.EDID_buf1[0]))
			rx_set_repeater_support(0);
		else
			rx_set_repeater_support(1);
		hdev->hwop.cntlddc(hdev, DDC_HDCP_GET_BKSV,
			(unsigned long)bksv_buf);
		rx_set_receive_hdcp(bksv_buf, 1, 1, 0, 0);
	}

	info = hdmitx_get_current_vinfo(NULL);
	if (info && info->mode == VMODE_HDMI)
		hdmitx_set_audio(hdev, &hdev->cur_audio_param);
	if (plugout_mute_flg) {
		/* 1.TV not changed: just clear avmute and continue output
		 * 2.if TV changed:
		 * keep avmute (will be cleared by systemcontrol);
		 * clear pkt, packets need to be cleared, otherwise,
		 * if plugout from DV/HDR TV, and plugin to non-DV/HDR
		 * TV, packets may not be cleared. pkt sending will
		 * be callbacked later after vinfo attached.
		 */
		if (is_cur_tmds_div40(hdev))
			hdmitx_resend_div40(hdev);
		if (!is_tv_changed()) {
			hdev->tx_hw.cntlmisc(&hdev->tx_hw, MISC_AVMUTE_OP,
					    CLR_AVMUTE);
		} else {
			/* keep avmute & clear pkt */
			hdmitx_set_drm_pkt(NULL);
			hdmitx_set_vsif_pkt(0, 0, NULL, true);
			hdmitx_set_hdr10plus_pkt(0, NULL);
		}
		edidinfo_attach_to_vinfo(hdev);
		plugout_mute_flg = false;
	}
	hdev->tx_comm.hpd_state = 1;
	hdmitx_notify_hpd(hdev->tx_comm.hpd_state,
			  hdev->tx_comm.edid_parsing ?
			  hdev->tx_comm.edid_ptr : NULL);
	/* under early suspend, only update uevent state, not
	 * post to system, in case 1.old android system will
	 * set hdmi mode, 2.audio server and audio_hal will
	 * start run, increase power consumption
	 */
	if (hdev->tx_comm.suspend_flag) {
		hdmitx_set_uevent_state(HDMITX_HPD_EVENT, 1);
		extcon_set_state(hdmitx_extcon_hdmi, EXTCON_DISP_HDMI, 1);
		hdmitx_set_uevent_state(HDMITX_AUDIO_EVENT, 1);
	} else {
		hdmitx_set_uevent(HDMITX_HPD_EVENT, 1);
		extcon_set_state_sync(hdmitx_extcon_hdmi, EXTCON_DISP_HDMI, 1);
		hdmitx_set_uevent(HDMITX_AUDIO_EVENT, 1);
	}
	/* audio uevent is used for android to
	 * register hdmi audio device, it should
	 * sync with hdmi hpd state.
	 * 1.when bootup, android will get from hpd_state
	 * 2.when hotplug or suspend/resume, sync audio
	 * uevent with hpd uevent
	 */
	mutex_unlock(&hdmimode_mutex);
	mutex_unlock(&setclk_mutex);

	/*notify to drm hdmi*/
	if (!hdev->tx_comm.suspend_flag)
		hdmitx_hpd_notify_unlocked(&hdev->tx_comm);
}

static void hdmitx_aud_hpd_plug_handler(struct work_struct *work)
{
	int st;
	struct hdmitx_dev *hdev = container_of((struct delayed_work *)work,
		struct hdmitx_dev, work_aud_hpd_plug);

	st = hdev->tx_hw.cntlmisc(&hdev->tx_hw, MISC_HPD_GPI_ST, 0);
	pr_info("%s state:%d\n", __func__, st);
	hdmitx_set_uevent(HDMITX_AUDIO_EVENT, st);
}

static void hdmitx_hpd_plugout_handler(struct work_struct *work)
{
	struct hdmitx_dev *hdev = container_of((struct delayed_work *)work,
		struct hdmitx_dev, work_hpd_plugout);

	mutex_lock(&setclk_mutex);
	mutex_lock(&hdmimode_mutex);
	/* there's such case: hpd rising & hpd level high (0.6S > HZ/2)-->
	 * plugin handler-->hpd falling & hpd level low(0.2S)-->
	 * continue plugin handler, but EDID read abnormal as hpd fall,
	 * post plugin uevent-->
	 * hpd rising & keep level high-->plugin handler, EDID read normal,
	 * post plugin uevent, but as plugout event is not handled,
	 * the second plugin event will be posted fail. system may use
	 * the abnormal EDID read during the first plugin handler.
	 * so hpd plugout event should always be handled, and no need filter.
	 */
	/* if (!(hdev->hdmitx_event & (HDMI_TX_HPD_PLUGOUT))) { */
		/* mutex_unlock(&hdmimode_mutex); */
		/* mutex_unlock(&setclk_mutex); */
		/* return; */
	/* } */
	hdev->hdcp_mode = 0;
	hdev->hdcp_bcaps_repeater = 0;
	hdev->hwop.cntlddc(hdev, DDC_HDCP_MUX_INIT, 1);
	hdev->hwop.cntlddc(hdev, DDC_HDCP_OP, HDCP14_OFF);
	if (hdev->cedst_policy)
		cancel_delayed_work(&hdev->work_cedst);
	pr_info(SYS "plugout\n");
	hdmitx_current_status(HDMITX_HPD_PLUGOUT);
	hdev->previous_error_event = 0;
	/* when plugout before systemcontrol boot, setavmute
	 * but keep output not changed, and wait for plugin
	 * NOTE: TV maybe changed(such as DV <-> non-DV)
	 */
	if (!hdev->systemcontrol_on &&
		hdev->tx_hw.getstate(&hdev->tx_hw, STAT_TX_OUTPUT, 0)) {
		plugout_mute_flg = true;
		edidinfo_detach_to_vinfo(hdev);
		hdev->hdmitx_event &= ~HDMI_TX_HPD_PLUGOUT;
		rx_edid_physical_addr(0, 0, 0, 0);
		hdmitx_edid_clear(hdev);
		hdmitx_edid_ram_buffer_clear(hdev);
		hdmitx_edid_done = false;
		hdev->tx_comm.hpd_state = 0;
		hdmitx_notify_hpd(hdev->tx_comm.hpd_state, NULL);
		hdev->tx_hw.cntlmisc(&hdev->tx_hw, MISC_AVMUTE_OP, SET_AVMUTE);
		extcon_set_state_sync(hdmitx_extcon_hdmi, EXTCON_DISP_HDMI, 0);
		hdmitx_set_uevent(HDMITX_HPD_EVENT, 0);
		hdmitx_set_uevent(HDMITX_AUDIO_EVENT, 0);
		mutex_unlock(&hdmimode_mutex);
		mutex_unlock(&setclk_mutex);

		/*notify to drm hdmi*/
		hdmitx_hpd_notify_unlocked(&hdev->tx_comm);
		return;
	}
	/*after plugout, DV mode can't be supported*/
	hdmitx_set_drm_pkt(NULL);
	hdmitx_set_vsif_pkt(0, 0, NULL, true);
	hdmitx_set_hdr10plus_pkt(0, NULL);
	hdev->ready = 0;
	if (hdev->repeater_tx)
		rx_repeat_hpd_state(0);
	hdev->tx_hw.cntlconfig(&hdev->tx_hw, CONF_CLR_AVI_PACKET, 0);
	hdev->hwop.cntlddc(hdev, DDC_HDCP_MUX_INIT, 1);
	hdev->hwop.cntlddc(hdev, DDC_HDCP_OP, HDCP14_OFF);
	hdev->hwop.cntlddc(hdev, DDC_HDCP_SET_TOPO_INFO, 0);
	hdev->tx_hw.cntlmisc(&hdev->tx_hw, MISC_TMDS_PHY_OP, TMDS_PHY_DISABLE);
	hdev->hdmitx_event &= ~HDMI_TX_HPD_PLUGOUT;
	hdev->tx_hw.cntlmisc(&hdev->tx_hw, MISC_ESM_RESET, 0);
	edidinfo_detach_to_vinfo(hdev);
	rx_edid_physical_addr(0, 0, 0, 0);
	hdmitx_edid_clear(hdev);
	hdmitx_edid_ram_buffer_clear(hdev);
	hdmitx_edid_done = false;
	hdev->tx_comm.hpd_state = 0;
	hdmitx_notify_hpd(hdev->tx_comm.hpd_state, NULL);

	/* under early suspend, only update uevent state, not
	 * post to system
	 */
	if (hdev->tx_comm.suspend_flag) {
		hdmitx_set_uevent_state(HDMITX_HPD_EVENT, 0);
		hdmitx_set_uevent_state(HDMITX_AUDIO_EVENT, 0);
		extcon_set_state(hdmitx_extcon_hdmi, EXTCON_DISP_HDMI, 0);
	} else {
		hdmitx_set_uevent(HDMITX_HPD_EVENT, 0);
		hdmitx_set_uevent(HDMITX_AUDIO_EVENT, 0);
		extcon_set_state_sync(hdmitx_extcon_hdmi, EXTCON_DISP_HDMI, 0);
	}
	mutex_unlock(&hdmimode_mutex);
	mutex_unlock(&setclk_mutex);

	/*notify to drm hdmi*/
	hdmitx_hpd_notify_unlocked(&hdev->tx_comm);
}

static void hdmitx_internal_intr_handler(struct work_struct *work)
{
	struct hdmitx_dev *hdev = container_of((struct delayed_work *)work,
		struct hdmitx_dev, work_internal_intr);

	if (hdev->log_level & REG_LOG)
		hdev->hwop.debugfun(hdev, "dumpintr");
}

int get20_hpd_state(void)
{
	int ret;
	struct hdmitx_dev *hdev = get_hdmitx_device();

	mutex_lock(&setclk_mutex);
	ret = hdev->tx_comm.hpd_state;
	mutex_unlock(&setclk_mutex);

	return ret;
}

/******************************
 *  hdmitx kernel task
 *******************************/
int tv_audio_support(int type, struct rx_cap *prxcap)
{
	int i, audio_check = 0;

	for (i = 0; i < prxcap->AUD_count; i++) {
		if (prxcap->RxAudioCap[i].audio_format_code == type)
			audio_check = 1;
	}
	return audio_check;
}

static bool is_cur_tmds_div40(struct hdmitx_dev *hdev)
{
	unsigned int act_clk = 0;

	if (!hdev || hdev->tx_comm.fmt_para.vic == HDMI_0_UNKNOWN) {
		pr_err("display is not ready\n");
		return 0;
	}

	act_clk = hdev->tx_comm.fmt_para.tmds_clk / 1000;
	pr_info("hdmitx: %s original clock %d\n", __func__, act_clk);
	if (act_clk > 340)
		return 1;

	return 0;
}

static void hdmitx_resend_div40(struct hdmitx_dev *hdev)
{
	hdev->hwop.cntlddc(hdev, DDC_SCDC_DIV40_SCRAMB, 1);
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
	.release  = amhdmitx_release,
};

struct hdmitx_dev *get_hdmitx_device(void)
{
	return tx20_dev;
}
EXPORT_SYMBOL(get_hdmitx_device);

static int get_hdmitx_hdcp_ctl_lvl_to_drm(void)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();

	pr_debug("%s hdmitx20_%d\n", __func__, hdev->hdcp_ctl_lvl);
	return hdev->hdcp_ctl_lvl;
}

int get_hdmitx20_init(void)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();

	if (hdev)
		return hdev->hdmi_init;
	return 0;
}

struct vsdb_phyaddr *get_hdmitx20_phy_addr(void)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();

	return &hdev->hdmi_info.vsdb_phy_addr;
}

static int get_dt_vend_init_data(struct device_node *np,
				 struct vendor_info_data *vend)
{
	int ret;

	ret = of_property_read_string(np, "vendor_name",
				      (const char **)&vend->vendor_name);
	if (ret)
		pr_info(SYS "not find vendor name\n");

	ret = of_property_read_u32(np, "vendor_id", &vend->vendor_id);
	if (ret)
		pr_info(SYS "not find vendor id\n");

	ret = of_property_read_string(np, "product_desc",
				      (const char **)&vend->product_desc);
	if (ret)
		pr_info(SYS "not find product desc\n");
	return 0;
}

static void hdmitx_fmt_attr(struct hdmitx_dev *hdev)
{
	struct hdmitx_common *tx_comm = &hdev->tx_comm;
	struct hdmi_format_para *para = &tx_comm->fmt_para;

	if (strlen(tx_comm->fmt_attr) >= 8) {
		pr_debug(SYS "fmt_attr %s\n", tx_comm->fmt_attr);
		return;
	}
	if (para->cd == COLORDEPTH_RESERVED &&
	    para->cs == HDMI_COLORSPACE_RESERVED6) {
		strcpy(tx_comm->fmt_attr, "default");
	} else {
		memset(tx_comm->fmt_attr, 0, sizeof(tx_comm->fmt_attr));
		switch (para->cs) {
		case HDMI_COLORSPACE_RGB:
			memcpy(tx_comm->fmt_attr, "rgb,", 5);
			break;
		case HDMI_COLORSPACE_YUV422:
			memcpy(tx_comm->fmt_attr, "422,", 5);
			break;
		case HDMI_COLORSPACE_YUV444:
			memcpy(tx_comm->fmt_attr, "444,", 5);
			break;
		case HDMI_COLORSPACE_YUV420:
			memcpy(tx_comm->fmt_attr, "420,", 5);
			break;
		default:
			break;
		}
		switch (para->cd) {
		case COLORDEPTH_24B:
			strcat(tx_comm->fmt_attr, "8bit");
			break;
		case COLORDEPTH_30B:
			strcat(tx_comm->fmt_attr, "10bit");
			break;
		case COLORDEPTH_36B:
			strcat(tx_comm->fmt_attr, "12bit");
			break;
		case COLORDEPTH_48B:
			strcat(tx_comm->fmt_attr, "16bit");
			break;
		default:
			break;
		}
	}
	pr_debug(SYS "fmt_attr %s\n", tx_comm->fmt_attr);
}

/* for notify to cec */
static BLOCKING_NOTIFIER_HEAD(hdmitx_event_notify_list);
int hdmitx20_event_notifier_regist(struct notifier_block *nb)
{
	int ret = 0;
	struct hdmitx_dev *hdev = get_hdmitx_device();

	if (hdev->hdmi_init != 1)
		return 1;
	if (!nb)
		return ret;

	ret = blocking_notifier_chain_register(&hdmitx_event_notify_list, nb);
	/* update status when register */
	if (!ret && nb->notifier_call) {
		hdmitx_notify_hpd(hdev->tx_comm.hpd_state,
				  hdev->tx_comm.edid_parsing ?
				  hdev->tx_comm.edid_ptr : NULL);
		if (hdev->physical_addr != 0xffff)
			hdmitx_event_notify(HDMITX_PHY_ADDR_VALID,
					    &hdev->physical_addr);
	}

	return ret;
}

int hdmitx20_event_notifier_unregist(struct notifier_block *nb)
{
	int ret;
	struct hdmitx_dev *hdev = get_hdmitx_device();

	if (hdev->hdmi_init != 1)
		return 1;

	ret = blocking_notifier_chain_unregister(&hdmitx_event_notify_list, nb);

	return ret;
}

void hdmitx_event_notify(unsigned long state, void *arg)
{
	blocking_notifier_call_chain(&hdmitx_event_notify_list, state, arg);
}

void hdmitx_hdcp_status(int hdmi_authenticated)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();

	hdmitx_set_uevent(HDMITX_HDCP_EVENT, hdmi_authenticated);
	if (hdev->drm_hdcp_cb.callback)
		hdev->drm_hdcp_cb.callback(hdev->drm_hdcp_cb.data,
			hdmi_authenticated);
}

static void hdmitx_log_kfifo_in(const char *log)
{
	int ret;
	struct hdmitx_dev *hdev = get_hdmitx_device();

	if (!kfifo_initialized(hdev->log_kfifo))
		return;

	if (kfifo_is_full(hdev->log_kfifo))
		return;

	ret = kfifo_in(hdev->log_kfifo, log, strlen(log));
}

static void hdmitx_logevents_handler(struct work_struct *work)
{
	static u32 cnt;
	struct hdmitx_dev *hdev = get_hdmitx_device();

	hdmitx_log_kfifo_in(hdev->log_str);
	hdmitx_set_uevent(HDMITX_CUR_ST_EVENT, ++cnt);
}

static const char *hdmitx_event_log_str(enum hdmitx_event_log_bits event)
{
	switch (event) {
	case HDMITX_HPD_PLUGOUT:
		return "hdmitx_hpd_plugout\n";
	case HDMITX_HPD_PLUGIN:
		return "hdmitx_hpd_plugin\n";
	case HDMITX_EDID_HDMI_DEVICE:
		return "hdmitx_edid_hdmi_device\n";
	case HDMITX_EDID_DVI_DEVICE:
		return "hdmitx_edid_dvi_device\n";
	case HDMITX_EDID_HEAD_ERROR:
		return "hdmitx_edid_head_error\n";
	case HDMITX_EDID_CHECKSUM_ERROR:
		return "hdmitx_edid_checksum_error\n";
	case HDMITX_EDID_I2C_ERROR:
		return "hdmitx_edid_i2c_error\n";
	case HDMITX_HDCP_AUTH_SUCCESS:
		return "hdmitx_hdcp_auth_success\n";
	case HDMITX_HDCP_AUTH_FAILURE:
		return "hdmitx_hdcp_auth_failure\n";
	case HDMITX_HDCP_HDCP_1_ENABLED:
		return "hdmitx_hdcp_hdcp1_enabled\n";
	case HDMITX_HDCP_HDCP_2_ENABLED:
		return "hdmitx_hdcp_hdcp2_enabled\n";
	case HDMITX_HDCP_NOT_ENABLED:
		return "hdmitx_hdcp_not_enabled\n";
	case HDMITX_HDCP_DEVICE_NOT_READY_ERROR:
		return "hdmitx_hdcp_device_not_ready_error\n";
	case HDMITX_HDCP_AUTH_NO_14_KEYS_ERROR:
		return "hdmitx_hdcp_auth_no_14_keys_error\n";
	case HDMITX_HDCP_AUTH_NO_22_KEYS_ERROR:
		return "hdmitx_hdcp_auth_no_22_keys_error\n";
	case HDMITX_HDCP_AUTH_READ_BKSV_ERROR:
		return "hdmitx_hdcp_auth_read_bksv_error\n";
	case HDMITX_HDCP_AUTH_VI_MISMATCH_ERROR:
		return "hdmitx_hdcp_auth_vi_mismatch_error\n";
	case HDMITX_HDCP_AUTH_TOPOLOGY_ERROR:
		return "hdmitx_hdcp_auth_topology_error\n";
	case HDMITX_HDCP_AUTH_R0_MISMATCH_ERROR:
		return "hdmitx_hdcp_auth_r0_mismatch_error\n";
	case HDMITX_HDCP_AUTH_REPEATER_DELAY_ERROR:
		return "hdmitx_hdcp_auth_repeater_delay_error\n";
	case HDMITX_HDCP_I2C_ERROR:
		return "hdmitx_hdcp_i2c_error\n";
	default:
		return "Unknown event\n";
	}
}

void hdmitx_current_status(enum hdmitx_event_log_bits event)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();

	if (event & HDMITX_HDMI_ERROR_MASK) {
		if (event & hdev->previous_error_event) {
			// Found, skip duplicate logging.
			// For example, UEvent spamming of HDCP support (b/220687552).
			return;
		}
		pr_info(SYS "Record HDMI error: %s\n", hdmitx_event_log_str(event));
		hdev->previous_error_event |= event;
	}

	hdev->log_str = hdmitx_event_log_str(event);
	queue_delayed_work(hdev->hdmi_wq, &hdev->work_do_event_logs, 1);
}

/* for compliance with p/q/r/s/t, need both extcon and hdmi_event
 * there're mixed android/kernel combination, P/Q
 * only listen to extcon; while R/S only listen to uevent
 * t need listen to extcon(only hdmi hpd) and uevent
 */
static int hdmitx_extcon_register(struct platform_device *pdev, struct device *dev)
{
	int ret;

	/*hdmitx extcon hdmi*/
	hdmitx_extcon_hdmi = devm_extcon_dev_allocate(&pdev->dev, hdmi_extcon_cable);
	if (IS_ERR(hdmitx_extcon_hdmi)) {
		pr_info("%s[%d] hdmitx_extcon_hdmi allocated failed\n", __func__, __LINE__);
		if (PTR_ERR(hdmitx_extcon_hdmi) != -ENODEV)
			return PTR_ERR(hdmitx_extcon_hdmi);
		hdmitx_extcon_hdmi = NULL;
	}
	ret = devm_extcon_dev_register(&pdev->dev, hdmitx_extcon_hdmi);
	if (ret < 0)
		pr_info("%s[%d] hdmitx_extcon_hdmi register failed\n", __func__, __LINE__);

	return 0;
}

struct extcon_dev *get_hdmitx_extcon_hdmi(void)
{
	return hdmitx_extcon_hdmi;
}

static void hdmitx_hdr_state_init(struct hdmitx_dev *hdev)
{
	enum hdmi_tf_type hdr_type = HDMI_NONE;
	unsigned int colorimetry = 0;
	unsigned int hdr_mode = 0;

	hdr_type = hdmitx_get_cur_hdr_st();
	colorimetry = hdev->tx_hw.cntlconfig(&hdev->tx_hw, CONF_GET_AVI_BT2020, 0);
	/* 1:standard HDR, 2:non-standard, 3:HLG, 0:other */
	if (hdr_type == HDMI_HDR_SMPTE_2084) {
		if (colorimetry == 1)
			hdr_mode = 1;
		else
			hdr_mode = 2;
	} else if (hdr_type == HDMI_HDR_HLG) {
		if (colorimetry == 1)
			hdr_mode = 3;
	} else {
		hdr_mode = 0;
	}

	hdev->hdmi_last_hdr_mode = hdr_mode;
	hdev->hdmi_current_hdr_mode = hdr_mode;
}

static int amhdmitx_device_init(struct hdmitx_dev *hdmi_dev)
{
	if (!hdmi_dev)
		return 1;

	/* there's common_driver commit id in boot up log */
	pr_debug(SYS "Ver: %s\n", HDMITX_VER);

	hdmi_dev->hdtx_dev = NULL;

	hdmi_dev->physical_addr = 0xffff;

	hdmi_dev->hdmi_last_hdr_mode = 0;
	hdmi_dev->hdmi_current_hdr_mode = 0;
	/* hdr/vsif packet status init, no need to get actual status,
	 * force to print function callback for confirmation.
	 */
	hdmi_dev->hdr_transfer_feature = T_UNKNOWN;
	hdmi_dev->hdr_color_feature = C_UNKNOWN;
	hdmi_dev->colormetry = 0;
	hdmi_dev->hdmi_current_eotf_type = EOTF_T_NULL;
	hdmi_dev->hdmi_current_tunnel_mode = 0;
	hdmi_dev->hdmi_current_signal_sdr = true;
	hdmi_dev->unplug_powerdown = 0;
	hdmi_dev->vic_count = 0;
	hdmi_dev->auth_process_timer = 0;
	hdmi_dev->force_audio_flag = 0;
	hdmi_dev->hdcp_mode = 0;
	hdmi_dev->ready = 0;
	hdmi_dev->systemcontrol_on = 0;
	hdmi_dev->rxsense_policy = 0; /* no RxSense by default */
	/* enable or disable HDMITX SSPLL, enable by default */
	hdmi_dev->sspll = 1;
	/*
	 * 0, do not unmux hpd when off or unplug ;
	 * 1, unmux hpd when unplug;
	 * 2, unmux hpd when unplug  or off;
	 */
	hdmi_dev->hpdmode = 1;

	hdmi_dev->flag_3dfp = 0;
	hdmi_dev->flag_3dss = 0;
	hdmi_dev->flag_3dtb = 0;

	hdmi_dev->mux_hpd_if_pin_high_flag = 1;

	hdmi_dev->audio_param_update_flag = 0;
	/* 1: 2ch */
	hdmi_dev->hdmi_ch = 1;
	/* default audio configure is on */
	hdmi_dev->tx_aud_cfg = 1;
	hdmi_dev->topo_info =
		kmalloc(sizeof(struct hdcprp_topo), GFP_KERNEL);
	if (!hdmi_dev->topo_info)
		pr_info("failed to alloc hdcp topo info\n");
	memset(&hdmi_dev->hdmi_info, 0, sizeof(struct hdmitx_info));
	hdmi_dev->vid_mute_op = VIDEO_NONE_OP;
	hdmi_dev->log_level = LOG_EN;
	/* init debug param */
	hdmi_dev->debug_param.avmute_frame = 0;
	hdmi_dev->tx_comm.ctrl_ops = &tx20_ctrl_ops;
	return 0;
}

static int amhdmitx_get_dt_info(struct platform_device *pdev, struct hdmitx_dev *hdev)
{
	int ret = 0;

#ifdef CONFIG_OF
	int val;
	phandle phandle;
	struct device_node *init_data;
	const struct of_device_id *match;
#endif
	u32 refreshrate_limit = 0;

	/* HDMITX pinctrl config for hdp and ddc*/
	if (pdev->dev.pins) {
		hdev->pdev = &pdev->dev;

		hdev->pinctrl_default =
			pinctrl_lookup_state(pdev->dev.pins->p, "default");
		if (IS_ERR(hdev->pinctrl_default))
			pr_err(SYS "no default of pinctrl state\n");

		hdev->pinctrl_i2c =
			pinctrl_lookup_state(pdev->dev.pins->p, "hdmitx_i2c");
		if (IS_ERR(hdev->pinctrl_i2c))
			pr_debug(SYS "no hdmitx_i2c of pinctrl state\n");

		pinctrl_select_state(pdev->dev.pins->p,
				     hdev->pinctrl_default);
	}

	hdev->hdmitx_gpios_hpd = of_get_named_gpio_flags(pdev->dev.of_node,
		"hdmitx-gpios-hpd", 0, NULL);
	if (hdev->hdmitx_gpios_hpd == -EPROBE_DEFER)
		pr_err("get hdmitx-gpios-hpd error\n");
	hdev->hdmitx_gpios_scl = of_get_named_gpio_flags(pdev->dev.of_node,
		"hdmitx-gpios-scl", 0, NULL);
	if (hdev->hdmitx_gpios_scl == -EPROBE_DEFER)
		pr_err("get hdmitx-gpios-scl error\n");
	hdev->hdmitx_gpios_sda = of_get_named_gpio_flags(pdev->dev.of_node,
		"hdmitx-gpios-sda", 0, NULL);
	if (hdev->hdmitx_gpios_sda == -EPROBE_DEFER)
		pr_err("get hdmitx-gpios-sda error\n");

#ifdef CONFIG_OF
	if (pdev->dev.of_node) {
		int dongle_mode = 0;

		memset(&hdev->config_data, 0,
		       sizeof(struct hdmi_config_platform_data));
		/* Get chip type and name information */
		match = of_match_device(meson_amhdmitx_of_match, &pdev->dev);

		if (!match) {
			pr_info("%s: no match table\n", __func__);
			return -1;
		}
		hdev->data = (struct amhdmitx_data_s *)match->data;
		if (hdev->data->chip_type == MESON_CPU_ID_TM2 ||
			hdev->data->chip_type == MESON_CPU_ID_TM2B) {
			/* diff revA/B of TM2 chip */
			if (is_meson_rev_b()) {
				hdev->data->chip_type = MESON_CPU_ID_TM2B;
				hdev->data->chip_name = "tm2b";
			} else {
				hdev->data->chip_type = MESON_CPU_ID_TM2;
				hdev->data->chip_name = "tm2";
			}
		}
		pr_debug(SYS "chip_type:%d chip_name:%s\n",
			hdev->data->chip_type,
			hdev->data->chip_name);

		/* Get hdmi_rext information */
		ret = of_property_read_u32(pdev->dev.of_node, "hdmi_rext", &val);
		hdev->hdmi_rext = val;
		if (!ret)
			pr_info(SYS "hdmi_rext: %d\n", val);

		/* Get dongle_mode information */
		ret = of_property_read_u32(pdev->dev.of_node, "dongle_mode",
					   &dongle_mode);
		hdev->dongle_mode = !!dongle_mode;
		if (!ret)
			pr_info(SYS "hdmitx_device.dongle_mode: %d\n",
				hdev->dongle_mode);
		/* Get res_1080p information */
		ret = of_property_read_u32(pdev->dev.of_node, "res_1080p",
					   &hdev->tx_comm.res_1080p);
		hdev->tx_comm.res_1080p = !!hdev->tx_comm.res_1080p;

		ret = of_property_read_u32(pdev->dev.of_node, "max_refreshrate",
					   &refreshrate_limit);
		if (ret == 0 && refreshrate_limit > 0)
			hdev->tx_comm.max_refreshrate = refreshrate_limit;

		/* Get repeater_tx information */
		ret = of_property_read_u32(pdev->dev.of_node,
					   "repeater_tx", &val);
		if (!ret)
			hdev->repeater_tx = val;
		if (hdev->repeater_tx == 1)
			hdev->topo_info =
			kzalloc(sizeof(*hdev->topo_info), GFP_KERNEL);

		ret = of_property_read_u32(pdev->dev.of_node,
					   "cedst_en", &val);
		if (!ret)
			hdev->cedst_en = !!val;

		ret = of_property_read_u32(pdev->dev.of_node,
					   "hdcp_type_policy", &val);
		if (!ret) {
			hdev->hdcp_type_policy = 0;
			if (val == 2)
				hdev->hdcp_type_policy = -1;
			if (val == 1)
				hdev->hdcp_type_policy = 1;
		}

		/* Get vendor information */
		ret = of_property_read_u32(pdev->dev.of_node,
					   "vend-data", &val);
		if (ret)
			pr_info(SYS "not find match init-data\n");
		if (ret == 0) {
			phandle = val;
			init_data = of_find_node_by_phandle(phandle);
			if (!init_data)
				pr_info(SYS "not find device node\n");
			hdev->config_data.vend_data =
			kzalloc(sizeof(struct vendor_info_data), GFP_KERNEL);
			if (!(hdev->config_data.vend_data))
				pr_info(SYS "not allocate memory\n");
			ret = get_dt_vend_init_data
			(init_data, hdev->config_data.vend_data);
			if (ret)
				pr_info(SYS "not find vend_init_data\n");
		}
		/* Get power control */
		ret = of_property_read_u32(pdev->dev.of_node,
					   "pwr-ctrl", &val);
		if (ret)
			pr_debug(SYS "not find match pwr-ctl\n");
		if (ret == 0) {
			phandle = val;
			init_data = of_find_node_by_phandle(phandle);
			if (!init_data)
				pr_debug(SYS "not find device node\n");
			hdev->config_data.pwr_ctl =
			kzalloc((sizeof(struct hdmi_pwr_ctl)) *
			HDMI_TX_PWR_CTRL_NUM, GFP_KERNEL);
			if (!hdev->config_data.pwr_ctl)
				pr_info(SYS "can not get pwr_ctl mem\n");
			memset(hdev->config_data.pwr_ctl, 0,
			       sizeof(struct hdmi_pwr_ctl));
			if (ret)
				pr_debug(SYS "not find pwr_ctl\n");
		}
		/* hdcp ctrl 0:sysctrl, 1: drv, 2: linux app */
		ret = of_property_read_u32(pdev->dev.of_node, "hdcp_ctl_lvl",
					   &hdev->hdcp_ctl_lvl);
		if (ret)
			hdev->hdcp_ctl_lvl = 0;
		if (hdev->hdcp_ctl_lvl > 0)
			hdev->systemcontrol_on = true;

		/* Get reg information */
		ret = hdmitx_init_reg_map(pdev);
	}

#else
	hdmi_pdata = pdev->dev.platform_data;
	if (!hdmi_pdata) {
		pr_info(SYS "not get platform data\n");
		r = -ENOENT;
	} else {
		pr_info(SYS "get hdmi platform data\n");
	}
#endif
	hdev->irq_hpd = platform_get_irq_byname(pdev, "hdmitx_hpd");
	if (hdev->irq_hpd == -ENXIO) {
		pr_err("%s: ERROR: hdmitx hpd irq No not found\n",
		       __func__);
			return -ENXIO;
	}
	pr_debug(SYS "hpd irq = %d\n", hdev->irq_hpd);

	hdev->irq_viu1_vsync =
		platform_get_irq_byname(pdev, "viu1_vsync");
	if (hdev->irq_viu1_vsync == -ENXIO) {
		pr_err("%s: ERROR: viu1_vsync irq No not found\n",
		       __func__);
		return -ENXIO;
	}
	pr_debug(SYS "viu1_vsync irq = %d\n", hdev->irq_viu1_vsync);

	return ret;
}

/*
 * amhdmitx_clktree_probe
 * get clktree info from dts
 */
static void amhdmitx_clktree_probe(struct device *hdmitx_dev, struct hdmitx_dev *hdev)
{
	struct clk *hdmi_clk_vapb, *hdmi_clk_vpu;
	struct clk *hdcp22_tx_skp, *hdcp22_tx_esm;
	struct clk *venci_top_gate, *venci_0_gate, *venci_1_gate;
	struct clk *cts_hdmi_axi_clk;

	hdmi_clk_vapb = devm_clk_get(hdmitx_dev, "hdmi_vapb_clk");
	if (IS_ERR(hdmi_clk_vapb)) {
		pr_debug(SYS "vapb_clk failed to probe\n");
	} else {
		hdev->hdmitx_clk_tree.hdmi_clk_vapb = hdmi_clk_vapb;
		clk_prepare_enable(hdev->hdmitx_clk_tree.hdmi_clk_vapb);
	}

	hdmi_clk_vpu = devm_clk_get(hdmitx_dev, "hdmi_vpu_clk");
	if (IS_ERR(hdmi_clk_vpu)) {
		pr_debug(SYS "vpu_clk failed to probe\n");
	} else {
		hdev->hdmitx_clk_tree.hdmi_clk_vpu = hdmi_clk_vpu;
		clk_prepare_enable(hdev->hdmitx_clk_tree.hdmi_clk_vpu);
	}

	hdcp22_tx_skp = devm_clk_get(hdmitx_dev, "hdcp22_tx_skp");
	if (IS_ERR(hdcp22_tx_skp))
		pr_debug(SYS "hdcp22_tx_skp failed to probe\n");
	else
		hdev->hdmitx_clk_tree.hdcp22_tx_skp = hdcp22_tx_skp;

	hdcp22_tx_esm = devm_clk_get(hdmitx_dev, "hdcp22_tx_esm");
	if (IS_ERR(hdcp22_tx_esm))
		pr_debug(SYS "hdcp22_tx_esm failed to probe\n");
	else
		hdev->hdmitx_clk_tree.hdcp22_tx_esm = hdcp22_tx_esm;

	venci_top_gate = devm_clk_get(hdmitx_dev, "venci_top_gate");
	if (IS_ERR(venci_top_gate))
		pr_debug(SYS "venci_top_gate failed to probe\n");
	else
		hdev->hdmitx_clk_tree.venci_top_gate = venci_top_gate;

	venci_0_gate = devm_clk_get(hdmitx_dev, "venci_0_gate");
	if (IS_ERR(venci_0_gate))
		pr_debug(SYS "venci_0_gate failed to probe\n");
	else
		hdev->hdmitx_clk_tree.venci_0_gate = venci_0_gate;

	venci_1_gate = devm_clk_get(hdmitx_dev, "venci_1_gate");
	if (IS_ERR(venci_1_gate))
		pr_debug(SYS "venci_0_gate failed to probe\n");
	else
		hdev->hdmitx_clk_tree.venci_1_gate = venci_1_gate;

	cts_hdmi_axi_clk = devm_clk_get(hdmitx_dev, "cts_hdmi_axi_clk");
	if (IS_ERR(cts_hdmi_axi_clk))
		pr_warn("get cts_hdmi_axi_clk err\n");
	else
		hdev->hdmitx_clk_tree.cts_hdmi_axi_clk = cts_hdmi_axi_clk;
}

void amhdmitx_vpu_dev_register(struct hdmitx_dev *hdev)
{
	hdev->hdmitx_vpu_clk_gate_dev =
	vpu_dev_register(VPU_VENCI, DEVICE_NAME);
}

static int amhdmitx_probe(struct platform_device *pdev)
{
	int r, ret = 0;
	struct device *device = &pdev->dev;
	struct device *dev;
	static struct kfifo kfifo_log;
	struct hdmitx_dev *hdev;
	struct hdmitx_common *tx_comm;

	pr_debug(SYS "%s start\n", __func__);

	hdev = devm_kzalloc(device, sizeof(*hdev), GFP_KERNEL);
	if (!hdev)
		return -ENOMEM;

	tx20_dev = hdev;
	dev_set_drvdata(device, hdev);
	tx_comm = &hdev->tx_comm;
	amhdmitx_device_init(hdev);
	/*init txcommon*/
	hdmitx_common_init(tx_comm, &hdev->tx_hw);

	ret = amhdmitx_get_dt_info(pdev, hdev);
	if (ret)
		return ret;

	amhdmitx_clktree_probe(device, hdev);

	amhdmitx_vpu_dev_register(hdev);

	r = alloc_chrdev_region(&hdev->hdmitx_id, 0, HDMI_TX_COUNT,
				DEVICE_NAME);
	cdev_init(&hdev->cdev, &amhdmitx_fops);
	hdev->cdev.owner = THIS_MODULE;
	r = cdev_add(&hdev->cdev, hdev->hdmitx_id, HDMI_TX_COUNT);

	hdmitx_class = class_create(THIS_MODULE, DEVICE_NAME);
	if (IS_ERR(hdmitx_class)) {
		unregister_chrdev_region(hdev->hdmitx_id, HDMI_TX_COUNT);
		return -1;
	}

	dev = device_create(hdmitx_class, NULL, hdev->hdmitx_id, hdev,
			    "amhdmitx%d", MINOR(hdev->hdmitx_id)); /* kernel>=2.6.27 */

	if (!dev) {
		pr_info(SYS "device_create create error\n");
		class_destroy(hdmitx_class);
		r = -EEXIST;
		return r;
	}
	hdev->hdtx_dev = dev;
	ret = device_create_file(dev, &dev_attr_disp_mode);
	ret = device_create_file(dev, &dev_attr_aud_mode);
	ret = device_create_file(dev, &dev_attr_vid_mute);
	ret = device_create_file(dev, &dev_attr_sink_type);
	ret = device_create_file(dev, &dev_attr_config);
	ret = device_create_file(dev, &dev_attr_debug);
	ret = device_create_file(dev, &dev_attr_aud_cap);
	ret = device_create_file(dev, &dev_attr_hdmi_hdr_status);
	ret = device_create_file(dev, &dev_attr_aud_ch);
	ret = device_create_file(dev, &dev_attr_swap);
	ret = device_create_file(dev, &dev_attr_vic);
	ret = device_create_file(dev, &dev_attr_avmute);
	ret = device_create_file(dev, &dev_attr_sspll);
	ret = device_create_file(dev, &dev_attr_rxsense_policy);
	ret = device_create_file(dev, &dev_attr_rxsense_state);
	ret = device_create_file(dev, &dev_attr_cedst_policy);
	ret = device_create_file(dev, &dev_attr_cedst_count);
	ret = device_create_file(dev, &dev_attr_hdcp_clkdis);
	ret = device_create_file(dev, &dev_attr_hdcp_pwr);
	ret = device_create_file(dev, &dev_attr_hdcp_ksv_info);
	ret = device_create_file(dev, &dev_attr_hdmitx_cur_status);
	ret = device_create_file(dev, &dev_attr_hdcp_ver);
	ret = device_create_file(dev, &dev_attr_hdcp_byp);
	ret = device_create_file(dev, &dev_attr_hdcp_mode);
	ret = device_create_file(dev, &dev_attr_hdcp_type_policy);
	ret = device_create_file(dev, &dev_attr_hdcp_repeater);
	ret = device_create_file(dev, &dev_attr_hdcp_topo_info);
	ret = device_create_file(dev, &dev_attr_hdcp22_type);
	ret = device_create_file(dev, &dev_attr_hdcp_stickmode);
	ret = device_create_file(dev, &dev_attr_hdcp_stickstep);
	ret = device_create_file(dev, &dev_attr_hdmi_repeater_tx);
	ret = device_create_file(dev, &dev_attr_hdcp22_base);
	ret = device_create_file(dev, &dev_attr_hdcp_lstore);
	ret = device_create_file(dev, &dev_attr_hdcp_rptxlstore);
	ret = device_create_file(dev, &dev_attr_div40);
	ret = device_create_file(dev, &dev_attr_hdcp_ctrl);
	ret = device_create_file(dev, &dev_attr_hdmi_used);
	ret = device_create_file(dev, &dev_attr_rhpd_state);
	ret = device_create_file(dev, &dev_attr_max_exceed);
	ret = device_create_file(dev, &dev_attr_fake_plug);
	ret = device_create_file(dev, &dev_attr_hdmi_init);
	ret = device_create_file(dev, &dev_attr_ready);
	ret = device_create_file(dev, &dev_attr_support_3d);
	ret = device_create_file(dev, &dev_attr_dc_cap);
	ret = device_create_file(dev, &dev_attr_allm_cap);
	ret = device_create_file(dev, &dev_attr_allm_mode);
	ret = device_create_file(dev, &dev_attr_hdmirx_info);
	ret = device_create_file(dev, &dev_attr_hdmi_hsty_config);
	ret = device_create_file(dev, &dev_attr_sysctrl_enable);
	ret = device_create_file(dev, &dev_attr_hdcp_ctl_lvl);
	ret = device_create_file(dev, &dev_attr_hdmitx_drm_flag);
	ret = device_create_file(dev, &dev_attr_hdr_mute_frame);
	ret = device_create_file(dev, &dev_attr_log_level);
	ret = device_create_file(dev, &dev_attr_csc);
	ret = device_create_file(dev, &dev_attr_config_csc_en);
	ret = device_create_file(dev, &dev_attr_hdmitx_basic_config);
	ret = device_create_file(dev, &dev_attr_hdmi_config_info);
	ret = device_create_file(dev, &dev_attr_hdmitx_pkt_dump);
	ret = device_create_file(dev, &dev_attr_dump_debug_reg);
	ret = device_create_file(dev, &dev_attr_hdr_priority_mode);
	ret = device_create_file(dev, &dev_attr_hdcp22_top_reset);

#ifdef CONFIG_AMLOGIC_VPU
	hdev->encp_vpu_dev = vpu_dev_register(VPU_VENCP, DEVICE_NAME);
	hdev->enci_vpu_dev = vpu_dev_register(VPU_VENCI, DEVICE_NAME);
	/* vpu gate/mem ctrl for hdmitx, since TM2B */
	hdev->hdmi_vpu_dev = vpu_dev_register(VPU_HDMI, DEVICE_NAME);
#endif

#ifdef CONFIG_AMLOGIC_LEGACY_EARLY_SUSPEND
	hdmitx_early_suspend_handler.param = hdev;
	register_early_suspend(&hdmitx_early_suspend_handler);
#endif
	hdev->nb.notifier_call = hdmitx_reboot_notifier;
	register_reboot_notifier(&hdev->nb);
	hdev->log_kfifo = &kfifo_log;
	ret = kfifo_alloc(hdev->log_kfifo, HDMI_LOG_SIZE, GFP_KERNEL);
	if (ret)
		pr_info("hdmitx: alloc hdmi_log_kfifo failed\n");
	/*init hw*/
	hdmitx_meson_init(hdev);
	/*load fmt para from hw info.*/
	hdmitx_common_init_bootup_format_para(tx_comm, &tx_comm->fmt_para);
	if (tx_comm->fmt_para.vic != HDMI_0_UNKNOWN) {
		tx_comm->cur_VIC = tx_comm->fmt_para.vic;
		hdev->ready = 1;
	}
	/* update fmt_attr */
	hdmitx_fmt_attr(hdev);

	/* When init hdmi, clear the hdmitx module edid ram and edid buffer. */
	hdmitx_edid_clear(hdev);
	hdmitx_edid_ram_buffer_clear(hdev);

	hdmitx_hdr_state_init(hdev);
#ifdef CONFIG_AMLOGIC_VOUT_SERVE
	vout_register_server(&hdmitx_vout_server);
#endif
#ifdef CONFIG_AMLOGIC_VOUT2_SERVE
	vout2_register_server(&hdmitx_vout2_server);
#endif
#if IS_ENABLED(CONFIG_AMLOGIC_SND_SOC)
	if (hdmitx_uboot_audio_en()) {
		struct hdmitx_audpara *audpara = &hdev->cur_audio_param;

		audpara->sample_rate = FS_48K;
		audpara->type = CT_PCM;
		audpara->sample_size = SS_16BITS;
		audpara->channel_num = 2 - 1;
	}
	hdmitx20_audio_mute_op(1); /* default audio clock is ON */
	aout_register_client(&hdmitx_notifier_nb_a);
#endif
	spin_lock_init(&hdev->edid_spinlock);
	hdmitx_extcon_register(pdev, dev);

	hdev->tx_comm.hpd_state = !!hdev->tx_hw.cntlmisc(&hdev->tx_hw, MISC_HPD_GPI_ST, 0);
	hdmitx_notify_hpd(hdev->tx_comm.hpd_state, NULL);

	hdmitx_set_uevent(HDMITX_HDCPPWR_EVENT, HDMI_WAKEUP);
	INIT_WORK(&hdev->work_hdr, hdr_work_func);
	INIT_WORK(&hdev->work_hdr_unmute, hdr_unmute_work_func);
	hdev->hdmi_wq = alloc_workqueue(DEVICE_NAME,
					WQ_HIGHPRI | WQ_CPU_INTENSIVE, 0);
	INIT_DELAYED_WORK(&hdev->work_hpd_plugin, hdmitx_hpd_plugin_handler);
	INIT_DELAYED_WORK(&hdev->work_hpd_plugout, hdmitx_hpd_plugout_handler);
	INIT_DELAYED_WORK(&hdev->work_aud_hpd_plug,
			  hdmitx_aud_hpd_plug_handler);
	INIT_DELAYED_WORK(&hdev->work_internal_intr,
			  hdmitx_internal_intr_handler);
	INIT_DELAYED_WORK(&hdev->work_do_event_logs, hdmitx_logevents_handler);

	/* for rx sense feature */
	hdev->rxsense_wq = alloc_workqueue("hdmitx_rxsense",
					   WQ_SYSFS | WQ_FREEZABLE, 0);
	INIT_DELAYED_WORK(&hdev->work_rxsense, hdmitx_rxsense_process);
	/* for cedst feature */
	hdev->cedst_wq = alloc_workqueue("hdmitx_cedst",
					 WQ_SYSFS | WQ_FREEZABLE, 0);
	INIT_DELAYED_WORK(&hdev->work_cedst, hdmitx_cedst_process);

	hdev->tx_aud_cfg = 1; /* default audio configure is on */

	hdev->hwop.setupirq(hdev);

	if (hdev->tx_comm.hpd_state) {
		/* need to get edid before vout probe */
		hdev->already_used = 1;
		hdmitx_get_edid(hdev);
		edidinfo_attach_to_vinfo(hdev);
	}
	/* Trigger HDMITX IRQ*/
	if (hdev->tx_hw.cntlmisc(&hdev->tx_hw, MISC_HPD_GPI_ST, 0)) {
		/* When bootup mbox and TV simultaneously,
		 * TV may not handle SCDC/DIV40
		 */
		if (is_cur_tmds_div40(hdev))
			hdmitx_resend_div40(hdev);
		hdev->tx_hw.cntlmisc(&hdev->tx_hw,
			MISC_TRIGGER_HPD, 1);
		hdev->already_used = 1;
	} else {
		/* may plugout during uboot finish--kernel start,
		 * treat it as normal hotplug out, for > 3.4G case
		 */
		hdev->tx_hw.cntlmisc(&hdev->tx_hw,
			MISC_TRIGGER_HPD, 0);
	}

	hdev->hdmi_init = 1;

	hdmitx_hdcp_init(hdev);

	pr_debug(SYS "%s end\n", __func__);

	hdmitx_hook_drm(&pdev->dev);
	/*everything is ready, create sysfs here.*/
	hdmitx_sysfs_common_create(dev, &hdev->tx_comm, &hdev->tx_hw);

	return r;
}

static int amhdmitx_remove(struct platform_device *pdev)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(&pdev->dev);
	struct device *dev = hdev->hdtx_dev;

	/*remove sysfs before uninit/*/
	hdmitx_sysfs_common_destroy(dev);

	/*unbind from drm.*/
	hdmitx_unhook_drm(&pdev->dev);

	cancel_work_sync(&hdev->work_hdr);
	cancel_work_sync(&hdev->work_hdr_unmute);
	if (hdev->hwop.uninit)
		hdev->hwop.uninit(hdev);
	hdev->hpd_event = 0xff;
	kthread_stop(hdev->task);
#ifdef CONFIG_AMLOGIC_VOUT_SERVE
	vout_unregister_server(&hdmitx_vout_server);
#endif
#ifdef CONFIG_AMLOGIC_VOUT2_SERVE
	vout2_unregister_server(&hdmitx_vout2_server);
#endif

#if IS_ENABLED(CONFIG_AMLOGIC_SND_SOC)
	aout_unregister_client(&hdmitx_notifier_nb_a);
#endif

	/* Remove the cdev */
	device_remove_file(dev, &dev_attr_disp_mode);
	device_remove_file(dev, &dev_attr_aud_mode);
	device_remove_file(dev, &dev_attr_vid_mute);
	device_remove_file(dev, &dev_attr_sink_type);
	device_remove_file(dev, &dev_attr_config);
	device_remove_file(dev, &dev_attr_debug);
	device_remove_file(dev, &dev_attr_dc_cap);
	device_remove_file(dev, &dev_attr_allm_cap);
	device_remove_file(dev, &dev_attr_allm_mode);
	device_remove_file(dev, &dev_attr_hdmi_used);
	device_remove_file(dev, &dev_attr_fake_plug);
	device_remove_file(dev, &dev_attr_rhpd_state);
	device_remove_file(dev, &dev_attr_max_exceed);
	device_remove_file(dev, &dev_attr_hdmi_init);
	device_remove_file(dev, &dev_attr_ready);
	device_remove_file(dev, &dev_attr_support_3d);
	device_remove_file(dev, &dev_attr_vic);
	device_remove_file(dev, &dev_attr_avmute);
	device_remove_file(dev, &dev_attr_sspll);
	device_remove_file(dev, &dev_attr_hdmitx_cur_status);
	device_remove_file(dev, &dev_attr_rxsense_policy);
	device_remove_file(dev, &dev_attr_rxsense_state);
	device_remove_file(dev, &dev_attr_cedst_policy);
	device_remove_file(dev, &dev_attr_cedst_count);
	device_remove_file(dev, &dev_attr_hdcp_pwr);
	device_remove_file(dev, &dev_attr_div40);
	device_remove_file(dev, &dev_attr_hdcp_repeater);
	device_remove_file(dev, &dev_attr_hdcp_topo_info);
	device_remove_file(dev, &dev_attr_hdcp_type_policy);
	device_remove_file(dev, &dev_attr_hdcp22_type);
	device_remove_file(dev, &dev_attr_hdcp_stickmode);
	device_remove_file(dev, &dev_attr_hdcp_stickstep);
	device_remove_file(dev, &dev_attr_hdmi_repeater_tx);
	device_remove_file(dev, &dev_attr_hdcp22_base);
	device_remove_file(dev, &dev_attr_swap);
	device_remove_file(dev, &dev_attr_hdmi_hdr_status);
	device_remove_file(dev, &dev_attr_hdmirx_info);
	device_remove_file(dev, &dev_attr_hdmi_hsty_config);
	device_remove_file(dev, &dev_attr_sysctrl_enable);
	device_remove_file(dev, &dev_attr_hdcp_ctl_lvl);
	device_remove_file(dev, &dev_attr_hdmitx_drm_flag);
	device_remove_file(dev, &dev_attr_hdr_mute_frame);
	device_remove_file(dev, &dev_attr_log_level);
	device_remove_file(dev, &dev_attr_csc);
	device_remove_file(dev, &dev_attr_config_csc_en);
	device_remove_file(dev, &dev_attr_hdmitx_basic_config);
	device_remove_file(dev, &dev_attr_hdmi_config_info);
	device_remove_file(dev, &dev_attr_hdmitx_pkt_dump);
	device_remove_file(dev, &dev_attr_dump_debug_reg);
	device_remove_file(dev, &dev_attr_hdr_priority_mode);
	device_remove_file(dev, &dev_attr_hdcp22_top_reset);

	cdev_del(&hdev->cdev);

	device_destroy(hdmitx_class, hdev->hdmitx_id);

	class_destroy(hdmitx_class);

	unregister_chrdev_region(hdev->hdmitx_id, HDMI_TX_COUNT);

	hdmitx_common_destroy(&hdev->tx_comm);
	return 0;
}

static void _amhdmitx_suspend(struct hdmitx_dev *hdev)
{
	/* drm tx22 enters AUTH_STOP, don't do hdcp22 IP reset */
	if (hdev->hdcp_ctl_lvl > 0)
		return;
	hdev->tx_hw.cntlmisc(&hdev->tx_hw, MISC_DIS_HPLL, 0);
	hdev->hwop.cntlddc(hdev,
		DDC_RESET_HDCP, 0);
	pr_info("amhdmitx: suspend and reset hdcp\n");
}

#ifdef CONFIG_PM
static int amhdmitx_suspend(struct platform_device *pdev,
			    pm_message_t state)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(&pdev->dev);

	_amhdmitx_suspend(hdev);
	return 0;
}

static int amhdmitx_resume(struct platform_device *pdev)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(&pdev->dev);

	/* may resume after start hdcp22, i2c
	 * reactive will force mux to hdcp14
	 */
	if (hdev->hdcp_ctl_lvl > 0)
		return 0;

	pr_info("amhdmitx: I2C_REACTIVE\n");
	hdev->tx_hw.cntlmisc(&hdev->tx_hw, MISC_I2C_REACTIVE, 0);

	return 0;
}
#endif

static void amhdmitx_shutdown(struct platform_device *pdev)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(&pdev->dev);

	_amhdmitx_suspend(hdev);
}

static struct platform_driver amhdmitx_driver = {
	.probe	  = amhdmitx_probe,
	.remove	 = amhdmitx_remove,
#ifdef CONFIG_PM
	.suspend	= amhdmitx_suspend,
	.resume	 = amhdmitx_resume,
#endif
	.shutdown = amhdmitx_shutdown,
	.driver	 = {
		.name   = DEVICE_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(meson_amhdmitx_of_match),
	}
};

int  __init amhdmitx_init(void)
{
	struct hdmitx_boot_param *param = get_hdmitx_boot_params();

	if (param->init_state & INIT_FLAG_NOT_LOAD)
		return 0;

	return platform_driver_register(&amhdmitx_driver);
}

void __exit amhdmitx_exit(void)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();

	pr_info(SYS "%s...\n", __func__);
	cancel_delayed_work_sync(&hdev->work_do_hdcp);
	kthread_stop(hdev->task_hdcp);
	platform_driver_unregister(&amhdmitx_driver);
}

//MODULE_DESCRIPTION("AMLOGIC HDMI TX driver");
//MODULE_LICENSE("GPL");
//MODULE_VERSION("1.0.0");

MODULE_PARM_DESC(log_level, "\n log_level\n");
module_param(log_level, int, 0644);

/*************DRM connector API**************/
int drm_hdmitx_register_hdcp_cb(struct drm_hdmitx_hdcp_cb *hdcp_cb)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();

	mutex_lock(&setclk_mutex);
	hdev->drm_hdcp_cb.callback = hdcp_cb->callback;
	hdev->drm_hdcp_cb.data = hdcp_cb->data;
	mutex_unlock(&setclk_mutex);
	return 0;
}

/* bit[1]: hdcp22, bit[0]: hdcp14 */
unsigned int drm_hdmitx_get_hdcp_cap(void)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();

	if (hdev->hdmi_init != 1)
		return 0;
	if (hdev->lstore < 0x10) {
		hdev->lstore = 0;
		if (hdev->hwop.cntlddc(hdev, DDC_HDCP_14_LSTORE, 0))
			hdev->lstore += 1;
		if (hdev->hwop.cntlddc(hdev,
			DDC_HDCP_22_LSTORE, 0))
			hdev->lstore += 2;
	}
	return hdev->lstore & 0x3;
}

/* bit[1]: hdcp22, bit[0]: hdcp14 */
unsigned int drm_get_rx_hdcp_cap(void)
{
	unsigned int ver = 0x0;
	struct hdmitx_dev *hdev = get_hdmitx_device();

	if (hdev->hdmi_init != 1)
		return 0;

	/* note that during hdcp1.4 authentication, read hdcp version
	 * of connected TV set(capable of hdcp2.2) may cause TV
	 * switch its hdcp mode, and flash screen. should not
	 * read hdcp version of sink during hdcp1.4 authentication.
	 * if hdcp1.4 authentication currently, force return hdcp1.4
	 */
	if (hdev->hdcp_mode == 1)
		return 0x1;
	/* if TX don't have HDCP22 key, skip RX hdcp22 ver */
	if (hdev->hwop.cntlddc(hdev,
		DDC_HDCP_22_LSTORE, 0) == 0 || !hdcp_tx22_daemon_ready())
		return 0x1;

	/* Detect RX support HDCP22 */
	mutex_lock(&getedid_mutex);
	ver = hdcp_rd_hdcp22_ver();
	mutex_unlock(&getedid_mutex);
	/* Here, must assume RX support HDCP14, otherwise affect 1A-03 */
	if (ver)
		return 0x3;
	else
		return 0x1;
}

/* after TEE hdcp key valid, do hdcp22 init before tx22 start */
void drm_hdmitx_hdcp22_init(void)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();

	if (hdev->hdmi_init != 1)
		return;
	hdmitx_hdcp_do_work(hdev);
	hdev->hwop.cntlddc(hdev,
		DDC_HDCP_MUX_INIT, 2);
}

/* echo 1/2 > hdcp_mode */
int drm_hdmitx_hdcp_enable(unsigned int content_type)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();
	enum hdmi_vic vic = HDMI_0_UNKNOWN;

	if (hdev->hdmi_init != 1)
		return 1;
	vic = hdev->tx_hw.getstate(&hdev->tx_hw, STAT_VIDEO_VIC, 0);

	hdev->hwop.cntlddc(hdev, DDC_HDCP_GET_AUTH, 0);

	if (content_type == 1) {
		hdev->hwop.cntlddc(hdev, DDC_HDCP_MUX_INIT, 1);
		if (vic == HDMI_17_720x576p50_4x3 || vic == HDMI_18_720x576p50_16x9)
			usleep_range(500000, 500010);
		hdev->hdcp_mode = 1;
		hdmitx_hdcp_do_work(hdev);
		hdev->hwop.cntlddc(hdev,
			DDC_HDCP_OP, HDCP14_ON);
	} else if (content_type == 2) {
		hdev->hdcp_mode = 2;
		hdmitx_hdcp_do_work(hdev);
		/* for drm hdcp_tx22, esm init only once
		 * don't do HDCP22 IP reset after init done!
		 */
		hdev->hwop.cntlddc(hdev,
			DDC_HDCP_MUX_INIT, 3);
	}

	return 0;
}

/* echo -1 > hdcp_mode;echo stop14/22 > hdcp_ctrl */
int drm_hdmitx_hdcp_disable(unsigned int content_type)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();

	if (hdev->hdmi_init != 1)
		return 1;

	hdev->hwop.cntlddc(hdev, DDC_HDCP_MUX_INIT, 1);
	hdev->hwop.cntlddc(hdev, DDC_HDCP_GET_AUTH, 0);

	if (content_type == 1) {
		hdev->hwop.cntlddc(hdev,
			DDC_HDCP_OP, HDCP14_OFF);
	} else if (content_type == 2) {
		hdev->hwop.cntlddc(hdev,
			DDC_HDCP_OP, HDCP22_OFF);
	}

	hdev->hdcp_mode = 0;
	hdmitx_hdcp_do_work(hdev);
	hdmitx_current_status(HDMITX_HDCP_NOT_ENABLED);

	return 0;
}

int drm_get_hdcp_auth_sts(void)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();

	if (hdev->hdmi_init != 1)
		return 0;

	return hdev->hwop.cntlddc(hdev, DDC_HDCP_GET_AUTH, 0);
}

static struct meson_hdmitx_dev drm_hdmitx_instance = {
	.get_hdmi_hdr_status = hdmi_hdr_status_to_drm,
	.drm_set_allm_mode = drm_set_allm_mode,

	/*hdcp apis*/
	.hdcp_init = meson_hdcp_init,
	.hdcp_exit = meson_hdcp_exit,
	.hdcp_enable = meson_hdcp_enable,
	.hdcp_disable = meson_hdcp_disable,
	.hdcp_disconnect = meson_hdcp_disconnect,
	.get_tx_hdcp_cap = drm_hdmitx_get_hdcp_cap,
	.get_rx_hdcp_cap = drm_get_rx_hdcp_cap,
	.register_hdcp_notify = meson_hdcp_reg_result_notify,
	.get_hdcp_ctl_lvl = get_hdmitx_hdcp_ctl_lvl_to_drm,
};

int hdmitx_hook_drm(struct device *device)
{
	struct hdmitx_dev *hdev;

	hdev = dev_get_drvdata(device);
	return hdmitx_bind_meson_drm(device,
		&hdev->tx_comm,
		&hdev->tx_hw,
		&drm_hdmitx_instance);
}

int hdmitx_unhook_drm(struct device *device)
{
	struct hdmitx_dev *hdev = dev_get_drvdata(device);

	return hdmitx_unbind_meson_drm(device,
		&hdev->tx_comm,
		&hdev->tx_hw,
		&drm_hdmitx_instance);
}

/*************DRM connector API end**************/
