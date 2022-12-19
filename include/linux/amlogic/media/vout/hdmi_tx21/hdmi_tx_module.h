/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _HDMI_TX21_MODULE_H
#define _HDMI_TX21_MODULE_H
#include "hdmi_info_global.h"
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
#include <linux/amlogic/media/vout/hdmitx_common/hdmitx_hw_common.h>

#define DEVICE_NAME "amhdmitx21"

/* HDMITX driver version */
#define HDMITX_VER "20220125"

/* chip type */
enum amhdmitx_chip_e {
	MESON_CPU_ID_T7,
	MESON_CPU_ID_MAX,
};

struct amhdmitx_data_s {
	enum amhdmitx_chip_e chip_type;
	const char *chip_name;
};

/*****************************
 *    hdmitx attr management
 ******************************/

/************************************
 *    hdmitx device structure
 *************************************/

enum hd_ctrl {
	VID_EN, VID_DIS, AUD_EN, AUD_DIS, EDID_EN, EDID_DIS, HDCP_EN, HDCP_DIS,
};

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

struct frac_rate_table {
	char *hz;
	u32 sync_num_int;
	u32 sync_den_int;
	u32 sync_num_dec;
	u32 sync_den_dec;
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

enum hdmi_hdr_transfer {
	T_UNKNOWN = 0,
	T_BT709,
	T_UNDEF,
	T_BT601,
	T_BT470M,
	T_BT470BG,
	T_SMPTE170M,
	T_SMPTE240M,
	T_LINEAR,
	T_LOG100,
	T_LOG316,
	T_IEC61966_2_4,
	T_BT1361E,
	T_IEC61966_2_1,
	T_BT2020_10,
	T_BT2020_12,
	T_SMPTE_ST_2084,
	T_SMPTE_ST_28,
	T_HLG,
};

enum hdmi_hdr_color {
	C_UNKNOWN = 0,
	C_BT709,
	C_UNDEF,
	C_BT601,
	C_BT470M,
	C_BT470BG,
	C_SMPTE170M,
	C_SMPTE240M,
	C_FILM,
	C_BT2020,
};

enum hdmitx_aspect_ratio {
	AR_UNKNOWN = 0,
	AR_4X3,
	AR_16X9,
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

struct hdmitx_dev {
	struct cdev cdev; /* The cdev structure */
	struct hdmitx_common tx_comm;
	struct hdmitx_hw_common tx_hw;
	dev_t hdmitx_id;
	struct proc_dir_entry *proc_file;
	struct task_struct *task;
	struct task_struct *task_monitor;
	struct task_struct *task_hdcp;
	struct workqueue_struct *hdmi_wq;
	struct workqueue_struct *rxsense_wq;
	struct workqueue_struct *cedst_wq;
	struct device *hdtx_dev;
	struct device *pdev; /* for pinctrl*/
	struct hdmi_format_para *para;
	struct pinctrl_state *pinctrl_i2c;
	struct pinctrl_state *pinctrl_default;
	struct amhdmitx_data_s *data;
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
	u32 div40;
	u32 lstore;
	u32 hdcp_mode;
	struct {
		int (*setdispmode)(struct hdmitx_dev *hdev);
		int (*setaudmode)(struct hdmitx_dev *hdev, struct hdmitx_audpara *audio_param);
		void (*setupirq)(struct hdmitx_dev *hdev);
		void (*debugfun)(struct hdmitx_dev *hdev, const char *buf);
		void (*debug_bist)(struct hdmitx_dev *hdev, u32 num);
		void (*uninit)(struct hdmitx_dev *hdev);
		int (*cntlpower)(struct hdmitx_dev *hdev, u32 cmd, u32 arg);
		/* edid/hdcp control */
		int (*cntlddc)(struct hdmitx_dev *hdev, u32 cmd, unsigned long arg);
		int (*cntlpacket)(struct hdmitx_dev *hdev, u32 cmd, u32 arg); /* Packet control */
		int (*cntl)(struct hdmitx_dev *hdev, u32 cmd, u32 arg); /* Other control */
	} hwop;
	struct {
		u32 enable;
		union hdmi_infoframe vend;
		union hdmi_infoframe avi;
		union hdmi_infoframe spd;
		union hdmi_infoframe aud;
		union hdmi_infoframe drm;
		union hdmi_infoframe emp;
	} infoframes;
	struct hdmi_config_platform_data config_data;
	enum hdmi_event_t hdmitx_event;
	u32 irq_hpd;
	u32 irq_vrr_vsync;
	/*EDID*/
	int vic_count;
	struct hdmitx_clk_tree_s hdmitx_clk_tree;
	/*audio*/
	struct hdmitx_audpara cur_audio_param;
	int audio_param_update_flag;
	u8 unplug_powerdown;
	unsigned short physical_addr;
	atomic_t kref_video_mute;
	atomic_t kref_audio_mute;
	/**/
	u8 hpd_event; /* 1, plugin; 2, plugout */
	u8 drm_mode_setting; /* 1, setting; 0, keeping */
	u8 rhpd_state; /* For repeater use only, no delay */
	u8 force_audio_flag;
	u8 mux_hpd_if_pin_high_flag;
	int aspect_ratio;	/* 1, 4:3; 2, 16:9 */
	struct hdmitx_info hdmi_info;
	u32 log;
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
	u32 dongle_mode:1;
	u32 cedst_en:1; /* configure in DTS */
	u32 bist_lock:1;
	u32 vend_id_hit:1;
	u32 fr_duration;
	spinlock_t edid_spinlock; /* edid hdr/dv cap lock */
	struct vpu_dev_s *hdmitx_vpu_clk_gate_dev;

	unsigned int hdcp_ctl_lvl;

	/*DRM related*/
	struct connector_hdcp_cb drm_hdcp_cb;

	struct miscdevice hdcp_comm_device;
	u8 def_stream_type;
	u8 tv_usage;
	bool systemcontrol_on;
};

/***********************************************************************
 *             DDC CONTROL //cntlddc
 **********************************************************************/
#define DDC_RESET_EDID          (CMD_DDC_OFFSET + 0x00)
#define DDC_PIN_MUX_OP          (CMD_DDC_OFFSET + 0x08)
#define PIN_MUX             0x1
#define PIN_UNMUX           0x2
#define DDC_EDID_READ_DATA      (CMD_DDC_OFFSET + 0x0a)
#define DDC_IS_EDID_DATA_READY  (CMD_DDC_OFFSET + 0x0b)
#define DDC_EDID_GET_DATA       (CMD_DDC_OFFSET + 0x0c)
#define DDC_EDID_CLEAR_RAM      (CMD_DDC_OFFSET + 0x0d)
#define DDC_GLITCH_FILTER_RESET	(CMD_DDC_OFFSET + 0x11)
#define DDC_SCDC_DIV40_SCRAMB	(CMD_DDC_OFFSET + 0x20)

struct hdmitx_dev *get_hdmitx21_device(void);

/***********************************************************************
 *    hdmitx protocol level interface
 **********************************************************************/
enum hdmi_vic hdmitx21_edid_vic_tab_map_vic(const char *disp_mode);
int hdmitx21_edid_parse(struct hdmitx_dev *hdev);
int check21_dvi_hdmi_edid_valid(u8 *buf);
enum hdmi_vic hdmitx21_edid_get_VIC(struct hdmitx_dev *hdev,
				  const char *disp_mode,
				  char force_flag);

int hdmitx21_edid_dump(struct hdmitx_dev *hdev, char *buffer,
		     int buffer_len);
bool hdmitx21_edid_check_valid_mode(struct hdmitx_dev *hdev,
				  struct hdmi_format_para *para);
const char *hdmitx21_edid_vic_to_string(enum hdmi_vic vic);
void hdmitx21_edid_clear(struct hdmitx_dev *hdev);
void hdmitx21_edid_ram_buffer_clear(struct hdmitx_dev *hdev);
void hdmitx21_edid_buf_compare_print(struct hdmitx_dev *hdev);
void hdmitx21_dither_config(struct hdmitx_dev *hdev);

int hdmitx21_construct_vsif(struct hdmitx_common *tx_comm,
	enum vsif_type type, int on, void *param);

/* if vic is 93 ~ 95, or 98 (HDMI14 4K), return 1 */
bool _is_hdmi14_4k(enum hdmi_vic vic);
/* if vic is 96, 97, 101, 102, 106, 107, 4k 50/60hz, return 1 */
bool _is_y420_vic(enum hdmi_vic vic);

/*
 * HDMI Repeater TX I/F
 * RX downstream Information from rptx to rprx
 */
/* send part raw edid from TX to RX */
void rx_repeat_hpd_state(bool st);
/* prevent compile error in no HDMIRX case */
void __attribute__((weak))rx_repeat_hpd_state(bool st)
{
}

void hdmi21_vframe_write_reg(u32 value);
void rx_edid_physical_addr(u8 a, u8 b,
			   u8 c, u8 d);
void __attribute__((weak))rx_edid_physical_addr(u8 a,
						u8 b,
						u8 c,
						u8 d)
{
}

int rx_set_hdr_lumi(u8 *data, int len);
int __attribute__((weak))rx_set_hdr_lumi(u8 *data, int len)
{
	return 0;
}

void rx_set_repeater_support(bool enable);
void __attribute__((weak))rx_set_repeater_support(bool enable)
{
}

void rx_set_receiver_edid(u8 *data, int len);
void __attribute__((weak))rx_set_receiver_edid(u8 *data, int len)
{
}

void rx_set_receive_hdcp(u8 *data, int len, int depth,
			 bool max_cascade, bool max_devs);
void __attribute__((weak))rx_set_receive_hdcp(u8 *data, int len,
					      int depth, bool max_cascade,
					      bool max_devs)
{
}

int hdmitx21_set_display(struct hdmitx_dev *hdev,
		       enum hdmi_vic videocode);

int hdmi21_set_3d(struct hdmitx_dev *hdev, int type,
		u32 param);

int hdmitx21_set_audio(struct hdmitx_dev *hdev,
		     struct hdmitx_audpara *audio_param);

/* for notify to cec */
#define HDMITX_PLUG			1
#define HDMITX_UNPLUG			2
#define HDMITX_PHY_ADDR_VALID		3
#define HDMITX_KSVLIST	4

#define HDMI_SUSPEND    0
#define HDMI_WAKEUP     1

enum hdmitx_event {
	HDMITX_NONE_EVENT = 0,
	HDMITX_HPD_EVENT,
	HDMITX_HDCP_EVENT,
	HDMITX_AUDIO_EVENT,
	HDMITX_HDCPPWR_EVENT,
	HDMITX_HDR_EVENT,
	HDMITX_RXSENSE_EVENT,
	HDMITX_CEDST_EVENT,
};

#define MAX_UEVENT_LEN 64
struct hdmitx_uevent {
	const enum hdmitx_event type;
	int state;
	const char *env;
};

int hdmitx21_set_uevent(enum hdmitx_event type, int val);

void hdmi_set_audio_para(int para);
int get21_cur_vout_index(void);
void phy_hpll_off(void);
int get21_hpd_state(void);
void hdmitx21_event_notify(unsigned long state, void *arg);
void hdmitx21_hdcp_status(int hdmi_authenticated);

/***********************************************************************
 *    hdmitx hardware level interface
 ***********************************************************************/
void hdmitx21_meson_init(struct hdmitx_dev *hdev);

/*
 * HDMITX HPD HW related operations
 */
enum hpd_op {
	HPD_INIT_DISABLE_PULLUP,
	HPD_INIT_SET_FILTER,
	HPD_IS_HPD_MUXED,
	HPD_MUX_HPD,
	HPD_UNMUX_HPD,
	HPD_READ_HPD_GPIO,
};

int hdmitx21_hpd_hw_op(enum hpd_op cmd);
/*
 * HDMITX DDC HW related operations
 */
enum ddc_op {
	DDC_INIT_DISABLE_PULL_UP_DN,
	DDC_MUX_DDC,
	DDC_UNMUX_DDC,
};

int hdmitx21_ddc_hw_op(enum ddc_op cmd);

#define HDMITX_HWCMD_MUX_HPD_IF_PIN_HIGH       0x3
#define HDMITX_HWCMD_TURNOFF_HDMIHW           0x4
#define HDMITX_HWCMD_MUX_HPD                0x5
#define HDMITX_HWCMD_PLL_MODE                0x6
#define HDMITX_HWCMD_TURN_ON_PRBS           0x7
#define HDMITX_FORCE_480P_CLK                0x8
#define HDMITX_GET_AUTHENTICATE_STATE        0xa
#define HDMITX_SW_INTERNAL_HPD_TRIG          0xb
#define HDMITX_HWCMD_OSD_ENABLE              0xf

#define HDMITX_IP_INTR_MASN_RST              0x12
#define HDMITX_EARLY_SUSPEND_RESUME_CNTL     0x14
#define HDMITX_EARLY_SUSPEND             0x1
#define HDMITX_LATE_RESUME               0x2
/* Refer to HDMI_OTHER_CTRL0 in hdmi_tx_reg.h */
#define HDMITX_IP_SW_RST                     0x15
#define TX_CREG_SW_RST      BIT(5)
#define TX_SYS_SW_RST       BIT(4)
#define CEC_CREG_SW_RST     BIT(3)
#define CEC_SYS_SW_RST      BIT(2)
#define HDMITX_AVMUTE_CNTL                   0x19
#define AVMUTE_SET          0   /* set AVMUTE to 1 */
#define AVMUTE_CLEAR        1   /* set AVunMUTE to 1 */
#define AVMUTE_OFF          2   /* set both AVMUTE and AVunMUTE to 0 */
#define HDMITX_CBUS_RST                      0x1A
#define HDMITX_INTR_MASKN_CNTL               0x1B
#define INTR_MASKN_ENABLE   0
#define INTR_MASKN_DISABLE  1
#define INTR_CLEAR          2

void hdmi_tx_edid_proc(u8 *edid);

void vsem_init_cfg(struct hdmitx_dev *hdev);

enum hdmi_tf_type hdmitx21_get_cur_hdr_st(void);
enum hdmi_tf_type hdmitx21_get_cur_dv_st(void);
enum hdmi_tf_type hdmitx21_get_cur_hdr10p_st(void);
bool hdmitx21_hdr_en(void);
bool hdmitx21_dv_en(void);
bool hdmitx21_hdr10p_en(void);
u32 aud_sr_idx_to_val(enum hdmi_audio_fs e_sr_idx);
bool hdmitx21_uboot_already_display(void);
#endif
