/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _HDMI_TX21_MODULE_H
#define _HDMI_TX21_MODULE_H
#include "hdmi_config.h"
#include "hdmi_hdcp.h"
#include <linux/wait.h>
#include <linux/clk.h>
#include <linux/cdev.h>
#include <linux/clk-provider.h>
#include <linux/device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/amlogic/media/vout/vout_notify.h>
#include <linux/amlogic/media/vpu/vpu.h>
#include <linux/amlogic/media/vrr/vrr.h>
#include <drm/amlogic/meson_connector_dev.h>
#include <linux/miscdevice.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_common.h>
#include <linux/amlogic/media/vout/hdmi_tx_repeater.h>
#include "hw/hdmi_tx_hw.h"
#include "../hdmitx_common/hdmitx_log.h"
#ifdef CONFIG_AMLOGIC_DSC
#include <linux/amlogic/media/vout/dsc.h>
#endif

#define DEVICE_NAME "amhdmitx21"

/* HDMITX driver version */
#define HDMITX_VER "20220125"

/************************************
 *    hdmitx device structure
 *************************************/

struct hdr_dynamic_struct {
	u32 type;
	u32 hd_len;/*hdr_dynamic_length*/
	u8 support_flags;
	u8 optional_fields[20];
};

struct cts_conftab {
	u32 fixed_n;
	u32 tmds_clk;
	u32 fixed_cts;
};

struct vic_attrmap {
	enum hdmi_vic VIC;
	u32 tmds_clk;
};

struct hdmi_phy_t {
	unsigned long reg;
	unsigned long val_sleep;
	unsigned long val_save;
};

struct audcts_log {
	u32 val:20;
	u32 stable:1;
};

struct ced_cnt {
	bool ch0_valid;
	u16 ch0_cnt:15;
	bool ch1_valid;
	u16 ch1_cnt:15;
	bool ch2_valid;
	u16 ch2_cnt:15;
	u8 chksum;
	bool ch3_valid;
	u16 ch3_cnt:15;
	bool rs_c_valid;
	u16 rs_c_cnt:15;
};

struct scdc_locked_st {
	u8 clock_detected:1;
	u8 ch0_locked:1;
	u8 ch1_locked:1;
	u8 ch2_locked:1;
	u8 ch3_locked:1;
};

struct aspect_ratio_list {
	enum hdmi_vic vic;
	int flag;
	char aspect_ratio_num;
	char aspect_ratio_den;
};

struct hdmitx_clk_tree_s {
	/* hdmitx clk tree */
	struct clk *hdmi_clk_vapb;
	struct clk *hdmi_clk_vpu;
	struct clk *venci_top_gate;
	struct clk *venci_0_gate;
	struct clk *venci_1_gate;
};

struct drm_hdmitx_hdcp {
	int hdcp_auth_result;
	int hdcp_fail_cnt;
	/* hdcp result callback */
	struct connector_hdcp_cb drm_hdcp_cb;
	/* drm hdcp debug function. */
	int test_hdcp_mode;
	void (*test_hdcp_disable)(void);
	void (*test_hdcp_enable)(int hdcp_mode);
	void (*test_hdcp_disconnect)(void);
};

struct hdmitx_dev {
	struct cdev cdev; /* The cdev structure */
	dev_t hdmitx_id;
	struct hdmitx_common tx_comm;
	struct hdmitx21_hw tx_hw;
	struct task_struct *task;
	struct task_struct *task_hdcp;
	/* dedicated for hpd */
	struct workqueue_struct *hdmi_hpd_wq;
	/* dedicated for intr */
	struct workqueue_struct *hdmi_intr_wq;
	struct device *hdtx_dev;
	struct device *pdev; /* for pinctrl*/
	struct pinctrl_state *pinctrl_i2c;
	struct pinctrl_state *pinctrl_default;
	struct notifier_block nb;
	struct delayed_work work_hpd_plugin;
	struct delayed_work work_hpd_plugout;
	struct delayed_work work_internal_intr;
	struct work_struct work_hdr;
	struct work_struct work_hdr_unmute;
	struct delayed_work work_start_hdcp;
	struct vrr_device_s hdmitx_vrr_dev;
	void *am_hdcp;
	bool dw_hdcp22_cap;
	int hdmi_init;
	int hpdmode;
	bool pre_tmds_clk_div40;
	u32 lstore;
	struct hdmitx_infoframe infoframes;
	struct hdmi_config_platform_data config_data;
	enum hdmi_event_t hdmitx_event;
	u32 irq_hpd;
	u32 irq_vrr_vsync;
	/*EDID*/
	struct hdmitx_clk_tree_s hdmitx_clk_tree;
	/*audio*/
	struct aud_para cur_audio_param;
	atomic_t kref_video_mute;
	atomic_t kref_audio_mute;
	/**/
	u8 hpd_event; /* 1, plugin; 2, plugout */
	int aspect_ratio;	/* 1, 4:3; 2, 16:9 */
	u8 manual_frl_rate; /* for manual setting */
	u8 frl_rate; /* for mode setting */
	u8 dsc_en;
#ifdef CONFIG_AMLOGIC_DSC
	/* pps data and clk info from dsc module */
	struct dsc_offer_tx_data dsc_data;
#endif
	/* 0: RGB444  1: Y444  2: Y422  3: Y420 */
	/* 4: 24bit  5: 30bit  6: 36bit  7: 48bit */
	/* if equals to 1, means current video & audio output are blank */
	enum vrr_type vrr_mode; /* 1: GAME-VRR, 2: QMS-VRR,  0: default no-VRR */
	struct ced_cnt ced_cnt;
	struct scdc_locked_st chlocked_st;
	enum hdmi_ll_mode ll_user_set_mode; /* ll setting: 0/AUTOMATIC, 1/Always OFF, 2/ALWAYS ON */
	bool ll_enabled_in_auto_mode; /* ll_mode enabled in auto or not */
	u32 sspll;
	/* configure for I2S: 8ch in, 2ch out */
	/* 0: default setting  1:ch0/1  2:ch2/3  3:ch4/5  4:ch6/7 */
	enum hdmi_hdr_transfer hdr_transfer_feature;
	enum hdmi_hdr_color hdr_color_feature;
	/* 0: sdr 1:standard HDR 2:non standard 3:HLG*/
	u32 colormetry;
	u32 dv_src_feature;
	u32 sdr_hdr_feature;
	u32 hdr10plus_feature;
	enum eotf_type hdmi_current_eotf_type;
	enum mode_type hdmi_current_tunnel_mode;
	bool hdmi_current_signal_sdr;
	u32 flag_3dfp:1;
	u32 flag_3dtb:1;
	u32 flag_3dss:1;
	u32 pxp_mode:1;
	u32 aon_output:1; /* always output in bl30 */
	u32 bist_lock:1;
	u32 fr_duration;
	struct vpu_dev_s *hdmitx_vpu_clk_gate_dev;

	/*DRM related*/
	struct drm_hdmitx_hdcp drm_hdcp;

	struct miscdevice hdcp_comm_device;
	u8 def_stream_type;
	u32 arc_rx_en;
	bool need_filter_hdcp_off;
	u32 filter_hdcp_off_period;
	bool not_restart_hdcp;
	unsigned long up_hdcp_timeout_sec;
	struct delayed_work work_up_hdcp_timeout;
};

struct hdmitx_dev *get_hdmitx21_device(void);

/***********************************************************************
 *    hdmitx protocol level interface
 **********************************************************************/
void hdmitx21_dither_config(struct hdmitx_dev *hdev);

int hdmitx21_set_display(struct hdmitx_dev *hdev,
		       enum hdmi_vic videocode);

void hdmi21_vframe_write_reg(u32 value);

int hdmi21_set_3d(struct hdmitx_dev *hdev, int type,
		u32 param);

int hdmitx21_set_audio(struct hdmitx_dev *hdev,
		     struct aud_para *audio_param);

int get21_hpd_state(void);
void hdmitx21_hdcp_status(int hdmi_authenticated);
struct hdmi_format_para *hdmitx21_get_vesa_paras(struct vesa_standard_timing *t);

/***********************************************************************
 *    hdmitx hardware level interface
 ***********************************************************************/
void hdmitx21_meson_init(struct hdmitx_dev *hdev);

u32 aud_sr_idx_to_val(enum hdmi_audio_fs e_sr_idx);
#endif
