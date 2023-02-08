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

/* chip type */
enum amhdmitx_chip_e {
	MESON_CPU_ID_M8B = 0,
	MESON_CPU_ID_GXBB,
	MESON_CPU_ID_GXTVBB,
	MESON_CPU_ID_GXL,
	MESON_CPU_ID_GXM,
	MESON_CPU_ID_TXL,
	MESON_CPU_ID_TXLX,
	MESON_CPU_ID_AXG,
	MESON_CPU_ID_GXLX,
	MESON_CPU_ID_TXHD,
	MESON_CPU_ID_G12A,
	MESON_CPU_ID_G12B,
	MESON_CPU_ID_SM1,
	MESON_CPU_ID_TM2,
	MESON_CPU_ID_TM2B,
	MESON_CPU_ID_SC2,
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

struct drm_hdmitx_hdcp_cb {
	void (*callback)(void *data, int auth);
	void *data;
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
	AR_UNKNOWM = 0,
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
	struct hdmitx_common tx_comm;
	struct hdmitx_hw_common tx_hw;
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
	struct vinfo_s *vinfo;
	struct delayed_work work_hpd_plugin;
	struct delayed_work work_hpd_plugout;
	struct delayed_work work_aud_hpd_plug;
	struct delayed_work work_rxsense;
	struct delayed_work work_internal_intr;
	struct delayed_work work_cedst;
	struct work_struct work_hdr;
	struct delayed_work work_do_hdcp;
#ifdef CONFIG_AML_HDMI_TX_14
	struct delayed_work cec_work;
#endif
	struct timer_list hdcp_timer;
	struct amhdmitx_data_s *data;
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
	unsigned int div40;
	unsigned int lstore;
	struct {
		void (*setpacket)(int type, unsigned char *DB,
				  unsigned char *HB);
		void (*disablepacket)(int type);
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
	struct {
		unsigned int hdcp14_en;
		unsigned int hdcp14_rslt;
	} hdcpop;
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
	unsigned char drm_mode_setting; /* 1, setting; 0, keeping */
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
	/* For some un-well-known TVs, no edid at all */
	unsigned int tv_no_edid;
	unsigned int hpd_lock;
	struct hdmi_format_para *para;
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
	unsigned int flag_3dfp:1;
	unsigned int flag_3dtb:1;
	unsigned int flag_3dss:1;
	unsigned int dongle_mode:1;
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
	bool suspend_flag;
};


/***********************************************************************
 *             DDC CONTROL //cntlddc
 **********************************************************************/
#define DDC_RESET_EDID          (CMD_DDC_OFFSET + 0x00)
#define DDC_RESET_HDCP          (CMD_DDC_OFFSET + 0x01)
#define DDC_HDCP_OP             (CMD_DDC_OFFSET + 0x02)
	#define HDCP14_ON	0x1
	#define HDCP14_OFF	0x2
	#define HDCP22_ON	0x3
	#define HDCP22_OFF	0x4
#define DDC_IS_HDCP_ON          (CMD_DDC_OFFSET + 0x04)
#define DDC_HDCP_GET_AKSV       (CMD_DDC_OFFSET + 0x05)
#define DDC_HDCP_GET_BKSV       (CMD_DDC_OFFSET + 0x06)
#define DDC_HDCP_GET_AUTH       (CMD_DDC_OFFSET + 0x07)
#define DDC_PIN_MUX_OP          (CMD_DDC_OFFSET + 0x08)
#define PIN_MUX             0x1
#define PIN_UNMUX           0x2
#define DDC_EDID_READ_DATA      (CMD_DDC_OFFSET + 0x0a)
#define DDC_IS_EDID_DATA_READY  (CMD_DDC_OFFSET + 0x0b)
#define DDC_EDID_GET_DATA       (CMD_DDC_OFFSET + 0x0c)
#define DDC_EDID_CLEAR_RAM      (CMD_DDC_OFFSET + 0x0d)
#define DDC_HDCP_MUX_INIT	(CMD_DDC_OFFSET + 0x0e)
#define DDC_HDCP_14_LSTORE	(CMD_DDC_OFFSET + 0x0f)
#define DDC_HDCP_22_LSTORE	(CMD_DDC_OFFSET + 0x10)
#define DDC_GLITCH_FILTER_RESET	(CMD_DDC_OFFSET + 0x11)
#define DDC_SCDC_DIV40_SCRAMB	(CMD_DDC_OFFSET + 0x20)
#define DDC_HDCP14_GET_BCAPS_RP	(CMD_DDC_OFFSET + 0x30)
#define DDC_HDCP14_GET_TOPO_INFO (CMD_DDC_OFFSET + 0x31)
#define DDC_HDCP_SET_TOPO_INFO (CMD_DDC_OFFSET + 0x32)
#define DDC_HDCP14_SAVE_OBS	(CMD_DDC_OFFSET + 0x40)

/* HDMI LOG */
#define HDMI_LOG_HDCP           BIT(0)

#define HDMI_PROCESS_DELAY  msleep(10)
/* reduce a little time, previous setting is 4000/10 */
#define AUTH_PROCESS_TIME   (1000 / 100)

/***********************************************************************
 *    hdmitx protocol level interface
 **********************************************************************/
enum hdmi_vic hdmitx_edid_vic_tab_map_vic(const char *disp_mode);
int hdmitx_edid_parse(struct hdmitx_dev *hdmitx_device);
int check_dvi_hdmi_edid_valid(unsigned char *buf);
enum hdmi_vic hdmitx_edid_get_VIC(struct hdmitx_dev *hdmitx_device,
				  const char *disp_mode,
				  char force_flag);
int hdmitx_edid_VIC_support(enum hdmi_vic vic);

bool hdmitx_edid_check_valid_mode(struct hdmitx_dev *hdev,
				  struct hdmi_format_para *para);
const char *hdmitx_edid_vic_tab_map_string(enum hdmi_vic vic);
const char *hdmitx_edid_vic_to_string(enum hdmi_vic vic);
void hdmitx_edid_clear(struct hdmitx_dev *hdmitx_device);
void hdmitx_edid_ram_buffer_clear(struct hdmitx_dev *hdmitx_device);
void hdmitx_edid_buf_compare_print(struct hdmitx_dev *hdmitx_device);
const char *hdmitx_edid_get_native_VIC(struct hdmitx_dev *hdmitx_device);

extern struct hdmitx_audpara hdmiaud_config_data;
extern struct hdmitx_audpara hsty_hdmiaud_config_data[8];
extern unsigned int hsty_hdmiaud_config_loc, hsty_hdmiaud_config_num;

int hdmitx_construct_vsif(struct hdmitx_common *tx_comm,
	enum vsif_type type, int on, void *param);

/* if vic is 93 ~ 95, or 98 (HDMI14 4K), return 1 */
bool is_hdmi14_4k(enum hdmi_vic vic);

/* if 4k is Y420, return 1 */
bool is_hdmi4k_420(enum hdmi_vic vic);

/* the hdmitx output limits to 1080p */
bool hdmitx_limited_1080p(void);
/* test current vic is over limited or not */
bool is_vic_over_limited_1080p(enum hdmi_vic vic);

/*
 * HDMI Repeater TX I/F
 * RX downstream Information from rptx to rprx
 */
/* send part raw edid from TX to RX */
void rx_repeat_hpd_state(unsigned int st);
/* prevent compile error in no HDMIRX case */
void __attribute__((weak))rx_repeat_hpd_state(unsigned int st)
{
}

void rx_edid_physical_addr(unsigned char a, unsigned char b,
			   unsigned char c, unsigned char d);
void __attribute__((weak))rx_edid_physical_addr(unsigned char a,
						unsigned char b,
						unsigned char c,
						unsigned char d)
{
}

int rx_set_hdr_lumi(unsigned char *data, int len);
int __attribute__((weak))rx_set_hdr_lumi(unsigned char *data, int len)
{
	return 0;
}

void rx_set_repeater_support(bool enable);
void __attribute__((weak))rx_set_repeater_support(bool enable)
{
}

void rx_set_receiver_edid(unsigned char *data, int len);
void __attribute__((weak))rx_set_receiver_edid(unsigned char *data, int len)
{
}

void rx_set_receive_hdcp(unsigned char *data, int len, int depth,
			 bool max_cascade, bool max_devs);
void __attribute__((weak))rx_set_receive_hdcp(unsigned char *data, int len,
					      int depth, bool max_cascade,
					      bool max_devs)
{
}

int hdmitx_set_display(struct hdmitx_dev *hdmitx_device,
		       enum hdmi_vic videocode);

int hdmi_set_3d(struct hdmitx_dev *hdmitx_device, int type,
		unsigned int param);

int hdmitx_set_audio(struct hdmitx_dev *hdmitx_device,
		     struct hdmitx_audpara *audio_param);

/* for notify to cec/hdmirx */
#define HDMITX_PLUG			1
#define HDMITX_UNPLUG			2
#define HDMITX_PHY_ADDR_VALID		3

#define HDMI_SUSPEND	0
#define HDMI_WAKEUP	1

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

struct hdmitx_dev *get_hdmitx_device(void);
/* for hdmitx internal usage */
void hdmitx_hdcp_status(int hdmi_authenticated);
void hdmitx_event_notify(unsigned long state, void *arg);
void setup20_attr(const char *buf);
void get20_attr(char attr[16]);
void hdmitx20_video_mute_op(unsigned int flag);

void hdmi_set_audio_para(int para);
int get_cur_vout_index(void);
void phy_pll_off(void);
void hdmitx_hdcp_do_work(struct hdmitx_dev *hdev);

/***********************************************************************
 *    hdmitx hardware level interface
 ***********************************************************************/
void hdmitx_meson_init(struct hdmitx_dev *hdmitx_device);
unsigned int get_hdcp22_base(void);

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

int hdmitx_hpd_hw_op(enum hpd_op cmd);
/*
 * HDMITX DDC HW related operations
 */
enum ddc_op {
	DDC_INIT_DISABLE_PULL_UP_DN,
	DDC_MUX_DDC,
	DDC_UNMUX_DDC,
};

int hdmitx_ddc_hw_op(enum ddc_op cmd);

#define HDMITX_HWCMD_MUX_HPD_IF_PIN_HIGH       0x3
#define HDMITX_HWCMD_TURNOFF_HDMIHW           0x4
#define HDMITX_HWCMD_MUX_HPD                0x5
#define HDMITX_HWCMD_PLL_MODE                0x6
#define HDMITX_HWCMD_TURN_ON_PRBS           0x7
#define HDMITX_FORCE_480P_CLK                0x8
#define HDMITX_GET_AUTHENTICATE_STATE        0xa
#define HDMITX_SW_INTERNAL_HPD_TRIG          0xb
#define HDMITX_HWCMD_OSD_ENABLE              0xf

#define HDMITX_HDCP_MONITOR                  0x11
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

#define HDMI_HDCP_DELAYTIME_AFTER_DISPLAY    20      /* unit: ms */

#define HDMITX_HDCP_MONITOR_BUF_SIZE         1024
struct Hdcp_Sub {
	char *hdcp_sub_name;
	unsigned int hdcp_sub_addr_start;
	unsigned int hdcp_sub_len;
};

void hdmi_tx_edid_proc(unsigned char *edid);

unsigned int hd_read_reg(unsigned int addr);
void hd_write_reg(unsigned int addr, unsigned int val);
void hd_set_reg_bits(unsigned int addr, unsigned int value,
		     unsigned int offset, unsigned int len);
void hdmitx_wr_reg(unsigned int addr, unsigned int data);
void hdmitx_poll_reg(unsigned int addr, unsigned int val,
		     unsigned long timeout);
void hdmitx_set_reg_bits(unsigned int addr, unsigned int value,
			 unsigned int offset, unsigned int len);
unsigned int hdmitx_rd_reg(unsigned int addr);
unsigned int hdmitx_rd_check_reg(unsigned int addr, unsigned int exp_data,
				 unsigned int mask);
bool hdmitx_get_bit(unsigned int addr, unsigned int bit_nr);
void vsem_init_cfg(struct hdmitx_dev *hdev);

enum hdmi_tf_type hdmitx_get_cur_hdr_st(void);
enum hdmi_tf_type hdmitx_get_cur_dv_st(void);
enum hdmi_tf_type hdmitx_get_cur_hdr10p_st(void);
bool hdmitx_hdr_en(void);
bool hdmitx_dv_en(void);
bool hdmitx_hdr10p_en(void);
bool LGAVIErrorTV(struct rx_cap *prxcap);
bool hdmitx_find_vendor_6g(struct hdmitx_dev *hdev);
bool hdmitx_find_vendor_ratio(struct hdmitx_dev *hdev);
bool hdmitx_find_vendor_null_pkt(struct hdmitx_dev *hdev);
int hdmitx_set_uevent_state(enum hdmitx_event type, int state);
int hdmitx_set_uevent(enum hdmitx_event type, int val);
struct extcon_dev *get_hdmitx_extcon_hdmi(void);

int hdmitx_uboot_already_display(int type);
#endif
