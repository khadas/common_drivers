/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _HDMI_TX_MODULE_H
#define _HDMI_TX_MODULE_H
#include "hdmi_config.h"
#include "hdmi_hdcp.h"
#include <linux/wait.h>
#include <linux/clk.h>
#include <linux/cdev.h>
#include <linux/clk-provider.h>
#include <linux/device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/amlogic/media/vout/vout_notify.h>
#ifdef CONFIG_AMLOGIC_VPU
#include <linux/amlogic/media/vpu/vpu.h>
#endif
#include <linux/spinlock.h>
#include <drm/amlogic/meson_connector_dev.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_common.h>
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_hw_common.h>
#include <linux/amlogic/media/vout/hdmi_tx/hdmi_tx_hw.h>
#include <linux/amlogic/media/vout/hdmi_tx_repeater.h>

#define DEVICE_NAME "amhdmitx"

/* HDMITX driver version */
#define HDMITX_VER "20210902"

/* log_level */
#define LOG_EN 0x01
#define VIDEO_LOG 0x02
#define AUDIO_LOG 0x04
#define HDCP_LOG 0x08
/* for dv/hdr... */
#define PACKET_LOG 0x10
#define EDID_LOG 0x20
#define PHY_LOG 0x40
#define REG_LOG 0x80
#define SCDC_LOG 0x100
#define VINFO_LOG 0x200
extern int hdmitx_log_level;

/************************************
 *    hdmitx device structure
 *************************************/

enum hdmi_event_t {
	HDMI_TX_NONE = 0,
	HDMI_TX_HPD_PLUGIN = 1,
	HDMI_TX_HPD_PLUGOUT = 2,
	HDMI_TX_INTERNAL_INTR = 4,
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

struct drm_hdmitx_hdcp_cb {
	void (*callback)(void *data, int auth);
	void *data;
};

struct hdmitx_clk_tree_s {
	/* hdmitx clk tree */
	struct clk *hdmi_clk_vapb;
	struct clk *hdmi_clk_vpu;
	struct clk *hdcp22_tx_skp;
	struct clk *hdcp22_tx_esm;
	struct clk *cts_hdmi_axi_clk;
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
	struct device *hdtx_dev;
	struct device *pdev; /* for pinctrl*/

	struct hdmitx_common tx_comm;
	struct hdmitx20_hw tx_hw;

	struct task_struct *task_hdcp;
	struct workqueue_struct *hdmi_wq;
	struct workqueue_struct *rxsense_wq;
	struct workqueue_struct *cedst_wq;
	struct pinctrl_state *pinctrl_i2c;
	struct pinctrl_state *pinctrl_default;
	struct delayed_work work_hpd_plugin;
	struct delayed_work work_hpd_plugout;
	struct delayed_work work_rxsense;
	struct delayed_work work_internal_intr;
	struct delayed_work work_cedst;
	struct work_struct work_hdr;
	struct work_struct work_hdr_unmute;
	struct delayed_work work_do_hdcp;
	int hdmi_init;
	int ready;	/* 1, hdmi stable output, others are 0 */

	/*hdcp */
	struct timer_list hdcp_timer;
	/* -1, no hdcp; 0, NULL; 1, 1.4; 2, 2.2 */
	int hdcp_mode;
	/* in board dts file, here can add
	 * &amhdmitx {
	 *     hdcp_type_policy = <1>;
	 * };
	 * 0 is default for NTS 0->1, 1 is fixed as 1, and 2 is fixed as 0
	 */
	/* -1, fixed 0; 0, NTS 0->1; 1, fixed 1 */
	int hdcp_type_policy;
	int hdcp_bcaps_repeater;
	int hdcp_hpd_stick;	/* 1 not init & reset at plugout */
	int hdcp_tst_sig;
	unsigned int lstore;
	unsigned int hdcp_ctl_lvl;
	unsigned char hdcp_max_exceed_state;
	unsigned int hdcp_max_exceed_cnt;
	bool hdcp22_type;
	struct hdcprp_topo *topo_info;
	struct drm_hdmitx_hdcp_cb drm_hdcp_cb;
	/*hdcp end*/

	struct {
		int (*setdispmode)(struct hdmitx_dev *hdmitx_device);
		int (*setaudmode)(struct hdmitx_dev *hdmitx_device,
				  struct aud_para *audio_param);
		void (*setupirq)(struct hdmitx_dev *hdmitx_device);
		int (*cntl)(struct hdmitx_dev *hdmitx_device, unsigned int cmd,
			    unsigned int arg); /* Other control */
		void (*am_hdmitx_set_hdcp_mode)(unsigned int user_type);
		void (*am_hdmitx_set_hdmi_mode)(void);
		void (*am_hdmitx_set_out_mode)(void);
		void (*am_hdmitx_hdcp_disable)(void);
		void (*am_hdmitx_hdcp_enable)(void);
		void (*am_hdmitx_hdcp_disconnect)(void);
		void (*setaudioinfoframe)(unsigned char *AUD_DB,
					  unsigned char *CHAN_STAT_BUF);
	} hwop;
	struct hdmi_config_platform_data config_data;
	enum hdmi_event_t hdmitx_event;
	/*audio*/
	/* if switching from 48k pcm to 48k DD, the ACR/N parameter is same,
	 * so there is no need to update ACR/N. but for mode change, different
	 * sample rate, need to update ACR/N.
	 */
	struct aud_para cur_audio_param;
	int audio_param_update_flag;
	unsigned char hpd_event; /* 1, plugin; 2, plugout */
	unsigned char rhpd_state; /* For repeater use only, no delay */
	unsigned char force_audio_flag;
	unsigned char mux_hpd_if_pin_high_flag;

	/*audio*/
	int auth_process_timer;
	unsigned int tx_aud_cfg; /* 0, off; 1, on */
	/* 0: RGB444  1: Y444  2: Y422  3: Y420 */
	/* 4: 24bit  5: 30bit  6: 36bit  7: 48bit */
	/* if equals to 1, means current video & audio output are blank */
	unsigned int rxsense_policy;
	unsigned int cedst_policy;
	struct ced_cnt ced_cnt;
	struct scdc_locked_st chlocked_st;
	unsigned int sspll;
	/* configure for I2S: 8ch in, 2ch out */
	/* 0: default setting  1:ch0/1  2:ch2/3  3:ch4/5  4:ch6/7 */
	unsigned int aud_output_ch;
	unsigned int hdmi_ch;
	/* if set to 1, then HDMI will output no audio */
	/* In KTV case, HDMI output Picture only, and Audio is driven by other
	 * sources.
	 */
	unsigned char hdmi_audio_off_flag;
	bool aud_notify_update;
	/*audio end*/

	/*hdr/dv*/
	enum hdmi_hdr_transfer hdr_transfer_feature;
	enum hdmi_hdr_color hdr_color_feature;
	/* 0: sdr 1:standard HDR 2:non standard 3:HLG*/
	unsigned int colormetry;
	unsigned int hdmi_last_hdr_mode;
	unsigned int hdmi_current_hdr_mode;
	unsigned int dv_src_feature;
	unsigned int sdr_hdr_feature;
	unsigned int hdr10plus_feature;
	enum eotf_type hdmi_current_eotf_type;
	enum mode_type hdmi_current_tunnel_mode;
	bool hdmi_current_signal_sdr;
	/*hdr/dv end*/

	unsigned int flag_3dfp:1;
	unsigned int flag_3dtb:1;
	unsigned int flag_3dss:1;
	unsigned int cedst_en:1; /* configure in DTS */
	unsigned int bist_lock:1;
	unsigned int vend_id_hit:1;

	bool systemcontrol_on;
	unsigned char vid_mute_op;
	atomic_t kref_video_mute;
	spinlock_t edid_spinlock; /* edid hdr/dv cap lock */

	/*hw members*/
	int hdmitx_gpios_hpd;
	int hdmitx_gpios_scl;
	int hdmitx_gpios_sda;
	unsigned int hdmi_rext; /* Rext resistor */
	struct hdmitx_clk_tree_s hdmitx_clk_tree;
	bool pre_tmds_clk_div40;
	/*hw members end*/

	/*Platform related.*/
	struct notifier_block reboot_nb;

	unsigned int irq_hpd;
	unsigned int irq_viu1_vsync;
#ifdef CONFIG_AMLOGIC_VPU
	struct vpu_dev_s *encp_vpu_dev;
	struct vpu_dev_s *enci_vpu_dev;
	struct vpu_dev_s *hdmi_vpu_dev;
	struct vpu_dev_s *hdmitx_vpu_clk_gate_dev;
#endif
	/*Platform related end.*/

	struct st_debug_param debug_param;
};

/* reduce a little time, previous setting is 4000/10 */
#define AUTH_PROCESS_TIME   (1000 / 100)

/***********************************************************************
 *    hdmitx protocol level interface
 **********************************************************************/
void hdmitx_current_status(enum hdmitx_event_log_bits event);

extern struct aud_para hdmiaud_config_data;
extern struct aud_para hsty_hdmiaud_config_data[8];
extern unsigned int hsty_hdmiaud_config_loc, hsty_hdmiaud_config_num;

int hdmitx_set_display(struct hdmitx_dev *hdmitx_device,
		       enum hdmi_vic videocode);

int hdmi_set_3d(struct hdmitx_dev *hdmitx_device, int type,
		unsigned int param);

int hdmitx_set_audio(struct hdmitx_dev *hdmitx_device,
		     struct aud_para *audio_param);

struct hdmitx_dev *get_hdmitx_device(void);
/* for hdmitx internal usage */
void hdmitx_hdcp_status(int hdmi_authenticated);
void hdmitx_hdcp_do_work(struct hdmitx_dev *hdev);

/***********************************************************************
 *    hdmitx hardware level interface
 ***********************************************************************/
void hdmitx_meson_init(struct hdmitx_dev *hdmitx_device);
unsigned int get_hdcp22_base(void);
void hdmitx20_video_mute_op(unsigned int flag);
bool hdmitx_find_vendor_6g(struct hdmitx_dev *hdev);
bool hdmitx_find_vendor_ratio(struct hdmitx_dev *hdev);
bool hdmitx_find_vendor_null_pkt(struct hdmitx_dev *hdev);
#endif
