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
#include <linux/amlogic/media/vout/hdmi_tx21/hdmi_tx_hw.h>
#include <linux/amlogic/media/vout/hdmi_tx_repeater.h>

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

enum hdmi_event_t {
	HDMI_TX_NONE = 0,
	HDMI_TX_HPD_PLUGIN = 1,
	HDMI_TX_HPD_PLUGOUT = 2,
	HDMI_TX_INTERNAL_INTR = 4,
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
};

struct scdc_locked_st {
	u8 clock_detected:1;
	u8 ch0_locked:1;
	u8 ch1_locked:1;
	u8 ch2_locked:1;
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

struct st_debug_param {
	unsigned int avmute_frame;
};

struct hdmitx_dev {
	struct cdev cdev; /* The cdev structure */
	dev_t hdmitx_id;
	struct hdmitx_common tx_comm;
	struct hdmitx21_hw tx_hw;
	struct proc_dir_entry *proc_file;
	//struct task_struct *task_hdmist_check;
	struct task_struct *task;
	struct task_struct *task_monitor;
	struct task_struct *task_hdcp;
	struct workqueue_struct *hdmi_wq;
	struct workqueue_struct *rxsense_wq;
	struct workqueue_struct *cedst_wq;
	struct device *hdtx_dev;
	struct device *pdev; /* for pinctrl*/
	struct pinctrl_state *pinctrl_i2c;
	struct pinctrl_state *pinctrl_default;
	struct notifier_block nb;
	struct delayed_work work_hpd_plugin;
	struct delayed_work work_hpd_plugout;
	struct delayed_work work_aud_hpd_plug;
	struct delayed_work work_rxsense;
	struct delayed_work work_internal_intr;
	struct delayed_work work_cedst;
	struct work_struct work_hdr;
	struct delayed_work work_start_hdcp;
	struct vrr_device_s hdmitx_vrr_dev;
	void *am_hdcp;
#ifdef CONFIG_AML_HDMI_TX_14
	struct delayed_work cec_work;
#endif
	int hdmi_init;
	int hpdmode;
	int ready;	/* 1, hdmi stable output, others are 0 */
	bool pre_tmds_clk_div40;
	u32 lstore;
	u32 hdcp_mode;
	struct {
		int (*setdispmode)(struct hdmitx_dev *hdev);
		int (*setaudmode)(struct hdmitx_dev *hdev, struct aud_para *audio_param);
		void (*setupirq)(struct hdmitx_dev *hdev);
		void (*debugfun)(struct hdmitx_dev *hdev, const char *buf);
		void (*debug_bist)(struct hdmitx_dev *hdmitx_device, unsigned int num);
		void (*uninit)(struct hdmitx_dev *hdev);
		int (*cntlpower)(struct hdmitx_dev *hdev, u32 cmd, u32 arg);
		/* edid/hdcp control */
		int (*cntlddc)(struct hdmitx_dev *hdev, u32 cmd, unsigned long arg);
		/* Audio/Video/System Status */
		int (*cntlpacket)(struct hdmitx_dev *hdev, u32 cmd, u32 arg); /* Packet control */
		/* Configure control */
		int (*cntl)(struct hdmitx_dev *hdev, u32 cmd, u32 arg); /* Other control */
	} hwop;
	struct hdmitx_infoframe infoframes;
	struct hdmi_config_platform_data config_data;
	enum hdmi_event_t hdmitx_event;
	u32 irq_hpd;
	u32 irq_vrr_vsync;
	/*EDID*/
	int vic_count;
	struct hdmitx_clk_tree_s hdmitx_clk_tree;
	/*audio*/
	struct aud_para cur_audio_param;
	int audio_param_update_flag;
	u8 unplug_powerdown;
	atomic_t kref_video_mute;
	atomic_t kref_audio_mute;
	/**/
	u8 hpd_event; /* 1, plugin; 2, plugout */
	u8 rhpd_state; /* For repeater use only, no delay */
	u8 force_audio_flag;
	u8 mux_hpd_if_pin_high_flag;
	int aspect_ratio;	/* 1, 4:3; 2, 16:9 */
	u8 manual_frl_rate; /* for manual setting */
	u8 frl_rate; /* for mode setting */
	u8 dsc_en;
	u32 tx_aud_cfg; /* 0, off; 1, on */
	u32 hpd_lock;
	/* 0: RGB444  1: Y444  2: Y422  3: Y420 */
	/* 4: 24bit  5: 30bit  6: 36bit  7: 48bit */
	/* if equals to 1, means current video & audio output are blank */
	u32 output_blank_flag;
	u32 audio_notify_flag;
	u32 audio_step;
	u32 repeater_tx;
	u32 rxsense_policy;
	u32 cedst_policy;
	u32 enc_idx;
	u32 vrr_type; /* 1: GAME-VRR, 2: QMS-VRR */
	struct ced_cnt ced_cnt;
	struct scdc_locked_st chlocked_st;
	enum hdmi_ll_mode ll_user_set_mode; /* ll setting: 0/AUTOMATIC, 1/Always OFF, 2/ALWAYS ON */
	bool ll_enabled_in_auto_mode; /* ll_mode enabled in auto or not */
	bool it_content;
	u32 sspll;
	/* if HDMI plugin even once time, then set 1 */
	/* if never hdmi plugin, then keep as 0 */
	u32 already_used;
	/* configure for I2S: 8ch in, 2ch out */
	/* 0: default setting  1:ch0/1  2:ch2/3  3:ch4/5  4:ch6/7 */
	u32 aud_output_ch;
	u32 hdmi_ch;
	u32 tx_aud_src; /* 0: SPDIF  1: I2S */
/* if set to 1, then HDMI will output no audio */
/* In KTV case, HDMI output Picture only, and Audio is driven by other
 * sources.
 */
	u8 hdmi_audio_off_flag;
	enum hdmi_hdr_transfer hdr_transfer_feature;
	enum hdmi_hdr_color hdr_color_feature;
	/* 0: sdr 1:standard HDR 2:non standard 3:HLG*/
	u32 colormetry;
	u32 hdmi_last_hdr_mode;
	u32 hdmi_current_hdr_mode;
	u32 dv_src_feature;
	u32 sdr_hdr_feature;
	u32 hdr10plus_feature;
	enum eotf_type hdmi_current_eotf_type;
	enum mode_type hdmi_current_tunnel_mode;
	u32 flag_3dfp:1;
	u32 flag_3dtb:1;
	u32 flag_3dss:1;
	u32 pxp_mode:1;
	u32 cedst_en:1; /* configure in DTS */
	u32 aon_output:1; /* always output in bl30 */
	u32 bist_lock:1;
	u32 vend_id_hit:1;
	u32 fr_duration;
	spinlock_t edid_spinlock; /* edid hdr/dv cap lock */
	struct vpu_dev_s *hdmitx_vpu_clk_gate_dev;

	unsigned int hdcp_ctl_lvl;

	/*DRM related*/
	int drm_hdmitx_id;
	struct connector_hdcp_cb drm_hdcp_cb;

	struct miscdevice hdcp_comm_device;
	u8 def_stream_type;
	u8 tv_usage;
	bool systemcontrol_on;
	u32 arc_rx_en;
	bool need_filter_hdcp_off;
	u32 filter_hdcp_off_period;
	bool not_restart_hdcp;
	/* mutex for mode setting, note hdcp should also
	 * mutex with hdmi mode setting
	 */
	struct mutex hdmimode_mutex;
	unsigned long up_hdcp_timeout_sec;
	struct delayed_work work_up_hdcp_timeout;
	struct st_debug_param debug_param;
};

struct hdmitx_dev *get_hdmitx21_device(void);

/***********************************************************************
 *    hdmitx protocol level interface
 **********************************************************************/
void hdmitx21_dither_config(struct hdmitx_dev *hdev);

/* set vic to AVI.VIC */
void hdmitx21_set_avi_vic(enum hdmi_vic vic);

int hdmitx21_set_display(struct hdmitx_dev *hdev,
		       enum hdmi_vic videocode);

void hdmi21_vframe_write_reg(u32 value);

int hdmi21_set_3d(struct hdmitx_dev *hdev, int type,
		u32 param);

int hdmitx21_set_audio(struct hdmitx_dev *hdev,
		     struct aud_para *audio_param);

#define HDMI_SUSPEND    0
#define HDMI_WAKEUP     1

#define MAX_UEVENT_LEN 64
struct hdmitx_uevent {
	const enum hdmitx_event type;
	int state;
	const char *env;
};

int hdmitx21_set_uevent(enum hdmitx_event type, int val);
int hdmitx21_set_uevent_state(enum hdmitx_event type, int state);

int get21_hpd_state(void);
void hdmitx21_event_notify(unsigned long state, void *arg);
void hdmitx21_hdcp_status(int hdmi_authenticated);
struct hdmi_format_para *hdmitx21_get_vesa_paras(struct vesa_standard_timing *t);

/***********************************************************************
 *    hdmitx hardware level interface
 ***********************************************************************/
void hdmitx21_meson_init(struct hdmitx_dev *hdev);

u32 aud_sr_idx_to_val(enum hdmi_audio_fs e_sr_idx);
#endif
