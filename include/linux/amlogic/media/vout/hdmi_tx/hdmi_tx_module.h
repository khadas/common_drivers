/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _HDMI_TX_MODULE_H
#define _HDMI_TX_MODULE_H
#include "hdmi_info_global.h"
#include "hdmi_config.h"
#include "hdmi_hdcp.h"
#include <linux/wait.h>
#include <linux/clk.h>
#include <linux/cdev.h>
#include <linux/clk-provider.h>
#include <linux/kfifo.h>
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

enum hdmitx_event_log_bits {
	HDMITX_HPD_PLUGOUT                      = BIT(0),
	HDMITX_HPD_PLUGIN                       = BIT(1),
	/* EDID states */
	HDMITX_EDID_HDMI_DEVICE                 = BIT(2),
	HDMITX_EDID_DVI_DEVICE                  = BIT(3),
	/* HDCP states */
	HDMITX_HDCP_AUTH_SUCCESS                = BIT(4),
	HDMITX_HDCP_AUTH_FAILURE                = BIT(5),
	HDMITX_HDCP_HDCP_1_ENABLED              = BIT(6),
	HDMITX_HDCP_HDCP_2_ENABLED              = BIT(7),
	HDMITX_HDCP_NOT_ENABLED                 = BIT(8),

	/* EDID errors */
	HDMITX_EDID_HEAD_ERROR                  = BIT(16),
	HDMITX_EDID_CHECKSUM_ERROR              = BIT(17),
	HDMITX_EDID_I2C_ERROR                   = BIT(18),
	/* HDCP errors */
	HDMITX_HDCP_DEVICE_NOT_READY_ERROR      = BIT(19),
	HDMITX_HDCP_AUTH_NO_14_KEYS_ERROR       = BIT(20),
	HDMITX_HDCP_AUTH_NO_22_KEYS_ERROR       = BIT(21),
	HDMITX_HDCP_AUTH_READ_BKSV_ERROR        = BIT(22),
	HDMITX_HDCP_AUTH_VI_MISMATCH_ERROR      = BIT(23),
	HDMITX_HDCP_AUTH_TOPOLOGY_ERROR         = BIT(24),
	HDMITX_HDCP_AUTH_R0_MISMATCH_ERROR      = BIT(25),
	HDMITX_HDCP_AUTH_REPEATER_DELAY_ERROR   = BIT(26),
	HDMITX_HDCP_I2C_ERROR                   = BIT(27),

	HDMITX_HDMI_ERROR_MASK	=	GENMASK(31, 16),
};

/************************************
 *    hdmitx device structure
 *************************************/

struct hdr_dynamic_struct {
	unsigned int type;
	unsigned int hd_len;/*hdr_dynamic_length*/
	unsigned char support_flags;
	unsigned char optional_fields[20];
};

struct cts_conftab {
	unsigned int fixed_n;
	unsigned int tmds_clk;
	unsigned int fixed_cts;
};

struct vic_attrmap {
	enum hdmi_vic VIC;
	unsigned int tmds_clk;
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
	unsigned int val:20;
	unsigned int stable:1;
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
	struct clk *hdcp22_tx_skp;
	struct clk *hdcp22_tx_esm;
	struct clk *cts_hdmi_axi_clk;
	struct clk *venci_top_gate;
	struct clk *venci_0_gate;
	struct clk *venci_1_gate;
};

#define HDMI_LOG_SIZE (BIT(12)) /* 4k */
struct st_debug_param {
	unsigned int avmute_frame;
};

struct hdmitx_dev {
	struct cdev cdev; /* The cdev structure */
	struct hdmitx_common tx_comm;
	struct hdmitx20_hw tx_hw;
	dev_t hdmitx_id;
	struct proc_dir_entry *proc_file;
	struct task_struct *task;
	struct task_struct *task_monitor;
	struct task_struct *task_hdcp;
	struct notifier_block nb;
	struct workqueue_struct *hdmi_wq;
	struct workqueue_struct *rxsense_wq;
	struct workqueue_struct *cedst_wq;
	struct device *hdtx_dev;
	struct device *pdev; /* for pinctrl*/
	struct pinctrl_state *pinctrl_i2c;
	struct pinctrl_state *pinctrl_default;
	struct kfifo *log_kfifo;
	const char *log_str;
	int previous_error_event;
	struct delayed_work work_hpd_plugin;
	struct delayed_work work_hpd_plugout;
	struct delayed_work work_aud_hpd_plug;
	struct delayed_work work_rxsense;
	struct delayed_work work_internal_intr;
	struct delayed_work work_cedst;
	struct work_struct work_hdr;
	struct work_struct work_hdr_unmute;
	struct delayed_work work_do_hdcp;
	struct delayed_work work_do_event_logs;
#ifdef CONFIG_AML_HDMI_TX_14
	struct delayed_work cec_work;
#endif
	struct timer_list hdcp_timer;
	int hdmitx_gpios_hpd;
	int hdmitx_gpios_scl;
	int hdmitx_gpios_sda;
	int hdmi_init;
	int hpdmode;
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
	int ready;	/* 1, hdmi stable output, others are 0 */
	int hdcp_hpd_stick;	/* 1 not init & reset at plugout */
	int hdcp_tst_sig;
	bool pre_tmds_clk_div40;
	unsigned int lstore;
	struct {
		void (*setaudioinfoframe)(unsigned char *AUD_DB,
					  unsigned char *CHAN_STAT_BUF);
		int (*setdispmode)(struct hdmitx_dev *hdmitx_device);
		int (*setaudmode)(struct hdmitx_dev *hdmitx_device,
				  struct hdmitx_audpara *audio_param);
		void (*setupirq)(struct hdmitx_dev *hdmitx_device);
		void (*debugfun)(struct hdmitx_dev *hdmitx_device,
				 const char *buf);
		void (*debug_bist)(struct hdmitx_dev *hdmitx_device,
				   unsigned int num);
		void (*uninit)(struct hdmitx_dev *hdmitx_device);
		int (*cntlpower)(struct hdmitx_dev *hdmitx_device,
				 unsigned int cmd, unsigned int arg);
		/* edid/hdcp control */
		int (*cntlddc)(struct hdmitx_dev *hdmitx_device,
			       unsigned int cmd, unsigned long arg);
		int (*cntlpacket)(struct hdmitx_dev *hdmitx_device,
				  unsigned int cmd,
				  unsigned int arg); /* Packet control */
		int (*cntl)(struct hdmitx_dev *hdmitx_device, unsigned int cmd,
			    unsigned int arg); /* Other control */
		void (*am_hdmitx_set_hdcp_mode)(unsigned int user_type);
		void (*am_hdmitx_set_hdmi_mode)(void);
		void (*am_hdmitx_set_out_mode)(void);
		void (*am_hdmitx_hdcp_disable)(void);
		void (*am_hdmitx_hdcp_enable)(void);
		void (*am_hdmitx_hdcp_disconnect)(void);
	} hwop;
	struct hdmi_config_platform_data config_data;
	enum hdmi_event_t hdmitx_event;
	unsigned int irq_hpd;
	unsigned int irq_viu1_vsync;
	/*EDID*/
	struct hdmitx_vidpara *cur_video_param;
	int vic_count;
	struct hdmitx_clk_tree_s hdmitx_clk_tree;
	/*audio*/
	struct hdmitx_audpara cur_audio_param;
	int audio_param_update_flag;
	unsigned char unplug_powerdown;
	unsigned short physical_addr;
	atomic_t kref_video_mute;
	atomic_t kref_audio_mute;
	/**/
	unsigned char hpd_event; /* 1, plugin; 2, plugout */
	unsigned char rhpd_state; /* For repeater use only, no delay */
	unsigned char hdcp_max_exceed_state;
	unsigned int hdcp_max_exceed_cnt;
	unsigned char force_audio_flag;
	unsigned char mux_hpd_if_pin_high_flag;
	int auth_process_timer;
	int aspect_ratio;	/* 1, 4:3; 2, 16:9 */
	struct hdmitx_info hdmi_info;
	unsigned int log;
	unsigned int tx_aud_cfg; /* 0, off; 1, on */
	unsigned int hpd_lock;
	/* 0: RGB444  1: Y444  2: Y422  3: Y420 */
	/* 4: 24bit  5: 30bit  6: 36bit  7: 48bit */
	/* if equals to 1, means current video & audio output are blank */
	unsigned int output_blank_flag;
	unsigned int audio_notify_flag;
	unsigned int audio_step;
	bool hdcp22_type;
	unsigned int repeater_tx;
	struct hdcprp_topo *topo_info;
	unsigned int rxsense_policy;
	unsigned int cedst_policy;
	struct ced_cnt ced_cnt;
	struct scdc_locked_st chlocked_st;
	unsigned int sspll;
	unsigned int hdmi_rext; /* Rext resistor */
	/* if HDMI plugin even once time, then set 1 */
	/* if never hdmi plugin, then keep as 0 */
	unsigned int already_used;
	/* configure for I2S: 8ch in, 2ch out */
	/* 0: default setting  1:ch0/1  2:ch2/3  3:ch4/5  4:ch6/7 */
	unsigned int aud_output_ch;
	unsigned int hdmi_ch;
	/* if set to 1, then HDMI will output no audio */
	/* In KTV case, HDMI output Picture only, and Audio is driven by other
	 * sources.
	 */
	unsigned char hdmi_audio_off_flag;
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
	/* if switching from 48k pcm to 48k DD, the ACR/N parameter is same,
	 * so there is no need to update ACR/N. but for mode change, different
	 * sample rate, need to update ACR/N.
	 */
	bool aud_notify_update;
	unsigned int flag_3dfp:1;
	unsigned int flag_3dtb:1;
	unsigned int flag_3dss:1;
	unsigned int cedst_en:1; /* configure in DTS */
	unsigned int bist_lock:1;
	unsigned int vend_id_hit:1;
	struct vpu_dev_s *hdmitx_vpu_clk_gate_dev;

	bool systemcontrol_on;
	unsigned char vid_mute_op;
	unsigned int hdcp_ctl_lvl;
	spinlock_t edid_spinlock; /* edid hdr/dv cap lock */
	unsigned int log_level;

	/*DRM related*/
	struct drm_hdmitx_hdcp_cb drm_hdcp_cb;

#ifdef CONFIG_AMLOGIC_VPU
	struct vpu_dev_s *encp_vpu_dev;
	struct vpu_dev_s *enci_vpu_dev;
	struct vpu_dev_s *hdmi_vpu_dev;
#endif
	struct st_debug_param debug_param;
};

/* reduce a little time, previous setting is 4000/10 */
#define AUTH_PROCESS_TIME   (1000 / 100)

/***********************************************************************
 *    hdmitx protocol level interface
 **********************************************************************/
int hdmitx_edid_parse(struct hdmitx_dev *hdmitx_device);
int check_dvi_hdmi_edid_valid(unsigned char *buf);
void hdmitx_edid_clear(struct hdmitx_dev *hdmitx_device);
void hdmitx_edid_ram_buffer_clear(struct hdmitx_dev *hdmitx_device);
void hdmitx_edid_buf_compare_print(struct hdmitx_dev *hdmitx_device);
void hdmitx_current_status(enum hdmitx_event_log_bits event);

extern struct hdmitx_audpara hdmiaud_config_data;
extern struct hdmitx_audpara hsty_hdmiaud_config_data[8];
extern unsigned int hsty_hdmiaud_config_loc, hsty_hdmiaud_config_num;

int hdmitx_set_display(struct hdmitx_dev *hdmitx_device,
		       enum hdmi_vic videocode);

int hdmi_set_3d(struct hdmitx_dev *hdmitx_device, int type,
		unsigned int param);

int hdmitx_set_audio(struct hdmitx_dev *hdmitx_device,
		     struct hdmitx_audpara *audio_param);

#define HDMI_SUSPEND	0
#define HDMI_WAKEUP	1

#define MAX_UEVENT_LEN 64
struct hdmitx_uevent {
	const enum hdmitx_event type;
	int state;
	const char *env;
};

struct hdmitx_dev *get_hdmitx_device(void);
/* for hdmitx internal usage */
void hdmitx_hdcp_status(int hdmi_authenticated);
void hdmitx_event_notify(unsigned long state, void *arg);

void hdmitx_hdcp_do_work(struct hdmitx_dev *hdev);

/***********************************************************************
 *    hdmitx hardware level interface
 ***********************************************************************/
void hdmitx_meson_init(struct hdmitx_dev *hdmitx_device);
unsigned int get_hdcp22_base(void);
void hdmitx20_video_mute_op(unsigned int flag);

bool LGAVIErrorTV(struct rx_cap *prxcap);
bool hdmitx_find_vendor_6g(struct hdmitx_dev *hdev);
bool hdmitx_find_vendor_ratio(struct hdmitx_dev *hdev);
bool hdmitx_find_vendor_null_pkt(struct hdmitx_dev *hdev);
int hdmitx_set_uevent_state(enum hdmitx_event type, int state);
int hdmitx_set_uevent(enum hdmitx_event type, int val);
struct extcon_dev *get_hdmitx_extcon_hdmi(void);
#endif
