/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __HDMI_RX_DRV_H__
#define __HDMI_RX_DRV_H__

#include <linux/workqueue.h>
#include <linux/extcon-provider.h>

#include <linux/amlogic/media/registers/cpu_version.h>
#include <linux/amlogic/media/frame_provider/tvin/tvin.h>
#include <linux/amlogic/iomap.h>
#include <linux/cdev.h>
#include <linux/irqreturn.h>
#include <linux/input.h>
#include <uapi/amlogic/hdmi_rx.h>
#include "../tvin_global.h"
#include "../tvin_frontend.h"
//#include "hdmirx_repeater.h"
//#include "hdmi_rx_pktinfo.h"
#include "hdmi_rx_edid.h"
#include "hdmi_rx_drv_ext.h"

//2023.03.01
//added interlaced mode protection
//2023.03.01
//optimize the judgment condition of unnormal timing
//2023.03.17
//fix emp pkt parse err
//2023.03.18
//disabled edid response after HPD low
//hdmirx earc
//add an ioctl to update single edid
//2023.04.28
//fix HDR10+ info error
//2023.5.6
//bring up sync to 5.15
//2023.5.17
//support filmmaker mode
//fix no edid result in system crash
//fix switch fail between edid 1.4 and 2.0.
//hdmi hbr no sound
//fix txhd2 merge to trunk problem
//2023.7.10 txhd2 bring up
//2023.7.21 fix phy/pll clk band
//2023.7.25 hdmirx suspend
//2023.7.27 phy setting and control pcs reset
//2023.8.8 fix edid switch issue
//2023.8.18 fix no sound when no aif pkt
//2023.8.25 fix t3x port4 get spd info err
//2023.8.29 hdcp cts
//2023 9.8 t3x reb bring up
//2023 9.13 add gcp mute cnt when fps change
//2023.9.22 t3x 480p aud clk settings
//2023.10.07 CTS2-79 for IP2.0
//2023.10.11 increase interval time of HPD low
//2023.10.27 hdmirx device vendor and product
//2023.11.3 add bist for debug
//2023.11.09 clr gcp write&the respective av mute related filed
//2023.11.16 rm gb check when dvi input
//2023.11.29 set main_port_open when resume
//2023.12.12 t3x no open port limit when reboot
//2024.01.04 fix soundless issue for 2.0 ip
//2024.01.10 optimize eq setting for 75m~115m frequency
#define RX_VER0 "ver.2024/01/10"

/*print type*/
#define COR1_LOG	0x10000
#define	LOG_EN		0x01
#define VIDEO_LOG	0x02
#define AUDIO_LOG	0x04
#define HDCP_LOG	0x08
#define PACKET_LOG	0x10
#define EQ_LOG		0x20
#define REG_LOG		0x40
#define ECC_LOG		0x80
#define EDID_LOG	0x100
#define PHY_LOG		0x200
#define VSI_LOG		0x800
#define SIG_PROP_LOG	0x400
#define DBG_LOG		0x1000
#define IRQ_LOG		0x2000
#define COR_LOG		0x4000
#define DBG1_LOG    0x8000

#define EDID_DATA_LOG	0x20000
#define RP_LOG		0x40000
#define FRL_LOG		0x80000

#define FRAME_RATE_MIN 20
#define FRAME_RATE_MAX 300

/* fix 3d timing issue and panasonic 1080p */
/* 0323: t3x bringup*/
/* 0406 add t3x edid*/
/* t3x top sw reset */
/* t3x sw flow */
/* 2-path-emp support */
/* hdmirx set all ports hpd */
/* select new api for clk msr */
/* modify code and single dwork for t3x */
/* correct phy trim value config method */
/* merge project modifications back to trunk */
/* optimize unnormal_format logic */
/* 2023.5.12 fix silent issue, switch to FSM_HPD_LOW */
/* 2023.05.15 optimize frl_rate monitor logic */
/* 2023.5.22 modify edid delivery method */
/* 2023.5.23 optimize color bar debug logic */
/* 2023.5.30 hdmirx cts and hdcp */
/* 2023.6.8  support black pattern for AV mute */
/* I2C edid communication is stopped at 0x2 */
/* play next song no sound */
/* 2023.7.5 clear dv packet when no emp */
/* 2023.7.12 txhd2 bring up debug */
/* 2023.8.1 add ctrl of 5v wake up */
/* 2023.08.14 modify the mapped emp buffer address*/
/* 2023.8.25 fix 40M 192k 176k no sound */
/* 2023.08.28 support FRL 3G3L & 6G3L */
/* 2023.08.31 add vpp mute cnt */
/* 2023.9.14 add support for 240p */
/* 2023 09.28 add trim flow for txhd2 */
/* 2023.10.8 t3x some compatibility problem */
/* 2023.10.10 fix t3x frl audio problem */
/* 2023.10.30 fix t3x clk msr fail */
/* 2023.11.13 fix t3x irq issue */
/* 2024.01.08 support to get AVI info */
/* 2024.02.05 Fix t5d accessing illegal addresses */
#define RX_VER1 "ver.2024/02/06"

/* 50ms timer for hdmirx main loop (HDMI_STATE_CHECK_FREQ is 20) */

#define TIME_1MS 1000000
#define EDID_MIX_MAX_SIZE 64
#define ESM_KILL_WAIT_TIMES 250
#define pr_var(str, index) rx_pr("%5d %-30s = %#x\n", (index), #str, (str))
#define var_to_str(var) (#var)

/* hdmirx fix audio no sound */
/* clear scdc with RX_HPD_C_CTRL_AON_IVCRX */
/* collate t5m code */
/* add aspect 4:3 */
/* 2023.5.5 fix emp pkt parse error */
/* 2023.05.09 core reset when afifo overflow */
/* 2023.05.24 fix 1366*768 identify to 1360*768 */
/* 2023.8.3 phy flow and aud pkt judge */
/* 2023.08.01 add t3x poweroff */
/* 2023.08.15 support black pattern for t7~t5w */
/* 2023.08.18 fix YUV422 data lost issue */
/* 2023.8.25 gcp avmute issue */
/* 2023.08.28 fix t3x sound issue */
/* 2023 09 27 reduce phy power */
/* optimize afifo configuration */
/* 2023.11.03 disable DDR access when suspend */
/* 2023.12.1 fix trim value err when resume */
/* 2023.12.06 fix resume panic issue */
/* 2024.01.11 fix EMP DDR write out of bounds */
/* 2023.1.11 fix timing lost */
/* 2024.2.22 fix hdr flash */
#define RX_VER2 "ver.2024/2/22"

#define PFIFO_SIZE 256
#define HDCP14_KEY_SIZE 368

/* sizeof(emp_buf) / sizeof(sizeof(struct pd_infoframe_s) + 1) = 1024/32 */
#define EMP_DSF_CNT_MAX 32

//#define SPECIAL_FUNC_EN
#ifdef SPECIAL_FUNC_EN
//bit0 portA bit1 portB bit2 portC
#define EDID_DETECT_PORT  2
#else
#define EDID_DETECT_PORT  7
#endif

enum chip_id_e {
	CHIP_ID_NONE,
	CHIP_ID_GXTVBB,
	CHIP_ID_TXL,
	CHIP_ID_TXLX,
	CHIP_ID_TXHD,
	CHIP_ID_TL1,
	CHIP_ID_TM2,
	CHIP_ID_T5,
	CHIP_ID_T5D,
	CHIP_ID_T7,
	CHIP_ID_T3,
	CHIP_ID_T5W,
	CHIP_ID_T5M,
	CHIP_ID_TXHD2,
	CHIP_ID_T3X,
};

enum phy_ver_e {
	PHY_VER_ORG,
	PHY_VER_TL1,
	PHY_VER_TM2,
	PHY_VER_T5,
	PHY_VER_T7,
	PHY_VER_T3,
	PHY_VER_T5W,
	PHY_VER_T5M,
	PHY_VER_TXHD2,
	PHY_VER_T3X,
};

enum port_num_e {
	PORT_NUM_2 = 2,
	PORT_NUM_3,
	PORT_NUM_4,
};

struct meson_hdmirx_data {
	enum chip_id_e chip_id;
	enum phy_ver_e phy_ver;
	enum port_num_e port_num;
	struct ctrl *phyctrl;
};

struct hdmirx_dev_s {
	int                         index;
	dev_t                       devt;
	struct cdev                 cdev;
	struct device               *dev;
	struct tvin_parm_s          param[2]; //for main & sub port
	struct timer_list           timer;
	struct tvin_frontend_s		frontend;
	unsigned int			irq[4];
	char					irq_name[4][12];
	/* mutex for wwwreg W/R protection */
	struct mutex			rx_lock;
	struct clk *modet_clk;
	struct clk *cfg_clk;
	struct clk *acr_ref_clk;
	struct clk *audmeas_clk;
	struct clk *aud_out_clk;
	struct clk *esm_clk;
	struct clk *skp_clk;
	struct clk *meter_clk;
	struct clk *axi_clk;
	struct clk *cts_hdmirx_5m_clk;
	struct clk *cts_hdmirx_2m_clk;
	struct clk *cts_hdmirx_hdcp2x_eclk;
	const struct meson_hdmirx_data *data;
	struct input_dev *hdmirx_input_dev;
};

#define IOC_SPD_INFO  _BIT(0)
#define IOC_AUD_INFO  _BIT(1)
#define IOC_MPEGS_INFO _BIT(2)
#define IOC_AVI_INFO _BIT(3)
#define ALL_PORTS ((1 << E_PORT_NUM) - 1)

enum colorspace_e {
	E_COLOR_RGB,
	E_COLOR_YUV422,
	E_COLOR_YUV444,
	E_COLOR_YUV420,
};

enum colorrange_rgb_e {
	E_RGB_RANGE_DEFAULT,
	E_RGB_RANGE_LIMIT,
	E_RGB_RANGE_FULL,
};

enum colorrange_ycc_e {
	E_YCC_RANGE_LIMIT,
	E_YCC_RANGE_FULL,
	E_YCC_RANGE_RSVD,
};

enum colordepth_e {
	E_COLORDEPTH_8 = 8,
	E_COLORDEPTH_10 = 10,
	E_COLORDEPTH_12 = 12,
	E_COLORDEPTH_16 = 16,
};

enum port_sts_e {
	E_PORT0,
	E_PORT1,
	E_PORT2,
	E_PORT3,
	E_PORT_NUM,
	E_5V_LOST = 0xfd,
	E_INIT = 0xff,
};

/* flag: need to request downstream re-auth */
#define HDCP_NEED_REQ_DS_AUTH 0x10
#define HDCP_VER_MASK 0xf

enum hdcp_version_e {
	HDCP_VER_NONE,
	HDCP_VER_14,
	HDCP_VER_22,
};

enum hdmirx_port_e {
	HDMIRX_PORT0,
	HDMIRX_PORT1,
	HDMIRX_PORT2,
	HDMIRX_PORT3,
};

enum map_addr_module_e {
	MAP_ADDR_MODULE_CBUS,
	MAP_ADDR_MODULE_HIU,
	MAP_ADDR_MODULE_HDMIRX_CAPB3,
	MAP_ADDR_MODULE_SEC_AHB,
	MAP_ADDR_MODULE_SEC_AHB2,
	MAP_ADDR_MODULE_APB4,
	MAP_ADDR_MODULE_TOP,
	MAP_ADDR_MODULE_CLK_CTRL,
	MAP_ADDR_MODULE_ANA_CTRL,
	MAP_ADDR_MODULE_COR,
	MAP_ADDR_MODULE_NUM
};

struct rx_var_param {
	int pll_unlock_cnt;
	int pll_unlock_max;
	int pll_lock_cnt;
	int pll_lock_max;
	int dwc_rst_wait_cnt;
	int dwc_rst_wait_cnt_max;
	int sig_stable_cnt;
	int sig_stable_max;
	int sig_stable_err_cnt;
	int sig_stable_err_max;
	int err_cnt_sum_max;
	int fpll_ready_cnt;
	//bool clk_debug_en;
	int hpd_wait_cnt;
	int special_wait_max;
	/* increase time of hpd low, to avoid some source like */
	/* MTK box/KaiboerH9 i2c communicate error */
	int hpd_wait_max;
	int sig_unstable_cnt;
	int sig_unstable_max;
	bool vic_check_en;
	bool dvi_check_en;
	int sig_unready_cnt;
	int sig_unready_max;
	int fps_unready_cnt;
	int fps_unready_max;
	int diff_pixel_th;
	int diff_line_th;
	int diff_frame_th;
	u32 force_vic;
	u32 err_chk_en;
	int aud_sr_stb_max;
	int log_level;
	bool rx5v_debug_en;
	int clk_unstable_cnt;
	int clk_unstable_max;
	int clk_stable_cnt;
	int clk_stable_max;
	int unnormal_wait_max;
	int wait_no_sig_max;
	/* No need to judge frame rate while checking timing stable,as there are
	 * some out-spec sources whose framerate change a lot(e.g:59.7~60.16hz).
	 * Other brands of tv can support this,we also need to support.
	 */
	int stable_check_lvl;
	/* If dvd source received the frequent pulses on HPD line,
	 * It will sent a length of dirty audio data sometimes.it's TX's issues.
	 * Compared with other brands TV, delay 1.5S to avoid this noise.
	 */
	int edid_update_delay;
	int skip_frame_cnt;
	u32 hdcp22_reauth_enable;
	unsigned int edid_update_flag;
	unsigned int downstream_hpd_flag;
	u32 hdcp22_stop_auth_enable;
	u32 hdcp22_esm_reset2_enable;
	int sm_pause;
	int pre_port;
	/* waiting time cannot be reduced */
	/* it will cause hdcp1.4 cts fail */
	int hdcp_none_wait_max;
	int esd_phy_rst_cnt;
	int esd_phy_rst_max;
	int cec_dev_info;
	bool term_flag;
	int clk_chg_cnt;
	int clk_chg_max;
	/* vpp mute when signal change, used
	 * in companion with vlock phase = 84
	 */
	u32 vpp_mute_enable;
	/* mute delay cnt before vpp unmute */
	int mute_cnt;
	u8 dbg_ve;
	/* after DE stable, start DE count */
	bool de_stable;
	u32 de_cnt;
	u8 avi_chk_frames;
	u32 avi_rcv_cnt;
	bool force_pattern;
	int frl_rate;
	int fpll_stable_cnt;
};

struct rx_aml_phy {
	int dfe_en;
	int ofst_en;
	int cdr_mode;
	int pre_int;
	int pre_int_en;
	int phy_bwth;
	int alirst_en;
	int tap1_byp;
	int eq_byp;
	int long_cable;
	int osc_mode;
	int pll_div;
	bool sqrst_en;
	int vga_dbg;
	int vga_dbg_delay;
	u8 eq_fix_val;
	int cdr_fr_en;
	int vga_tune;
	u32 force_sqo;
	/* add for t5 */
	int os_rate;
	u32 vga_gain;
	u32 eq_stg1;
	u32 eq_stg2;
	u8 eq_pole;
	u32 dfe_hold;
	u32 eq_hold;
	int eye_delay;
	u32 eq_retry;
	u32 tap2_byp;
	u32 long_bist_en;
	int reset_pcs_en;
	/* add for t5m */
	int eq_en;
	int tapx_value;
	int agc_enable;
	u32 afe_value;
	u32 dfe_value;
	u32 cdr_value;
	u32 eq_value;
	u32 misc2_value;
	u32 misc1_value;
	int phy_debug_en;
	int enhance_dfe_en_old;
	int enhance_dfe_en_new;
	int eye_height;
	int enhance_eq;
	int eq_level;
	int cdr_retry_en;
	int cdr_retry_max;
	int cdr_fr_en_auto;
	int hyper_gain_en;
	int eye_height_min;
	bool phy_power_off_en;
	int buf_gain;
};

struct rx_aml_phy_21 {
	int phy_bwth;
	u32 vga_gain;
	u32 eq_stg1;
	u32 eq_stg2;
	int cdr_fr_en;
	u32 eq_hold;
	u32 eq_retry;
	u32 dfe_en;
	int dfe_hold;
};

enum scan_mode_e {
	E_UNKNOWN_SCAN,
	E_OVERSCAN,
	E_UNDERSCAN,
	E_FUTURE,
};

/**
 * @short HDMI RX controller video parameters
 *
 * For Auxiliary Video InfoFrame (AVI) details see HDMI 1.4a section 8.2.2
 */
struct rx_video_info {
	/** DVI detection status: DVI (true) or HDMI (false) */
	bool hw_dvi;

	u8 hdcp_type;
	/** bit'0:auth start  bit'1:enc state(0:not encrypted 1:encrypted) **/
	u8 hdcp14_state;
	/** 1:decrypted 0:encrypted **/
	u8 hdcp22_state;
	/** Deep color mode: 8, 10, 12 or 16 [bits per pixel] */
	u8 colordepth;
	/** Pixel clock frequency [kHz] */
	u32 pixel_clk;
	/** Refresh rate [0.01Hz] */
	u32 frame_rate;
	/** Interlaced */
	bool interlaced;
	/** Vertical offset */
	u32 voffset;
	/** Vertical active */
	u32 vactive;
	/** Vertical total */
	u32 vtotal;
	/** Horizontal offset */
	u32 hoffset;
	/** Horizontal active */
	u32 hactive;
	/** Horizontal total */
	u32 htotal;
	/** AVI Y1-0, video format */
	u8 colorspace;
	/** AVI VIC6-0, video identification code */
	enum hdmi_vic_e hw_vic;
	/** AVI PR3-0, pixel repetition factor */
	u8 repeat;
	/* for sw info */
	enum hdmi_vic_e sw_vic;
	u8 sw_dvi;
	unsigned int it_content;
	enum tvin_cn_type_e cn_type;
	/** AVI Q1-0, RGB quantization range */
	unsigned int rgb_quant_range;
	/** AVI Q1-0, YUV quantization range */
	u8 yuv_quant_range;
	u8 active_valid;
	u8 active_ratio;
	u8 bar_valid;
	u8 scan_info;
	u8 colorimetry;
	u8 picture_ratio;
	u8 ext_colorimetry;
	u8 n_uniform_scale;
	u32 bar_end_top;
	u32 bar_start_bottom;
	u32 bar_end_left;
	u32 bar_start_right;
	bool sw_fp;
	bool sw_alternative;
};

/** Receiver key selection size - 40 bits */
#define HDCP_BKSV_SIZE	(2 *  1)
/** Encrypted keys size - 40 bits x 40 keys */
#define HDCP_KEYS_SIZE	(2 * 40)

/*emp buffer config*/
#define DUMP_MODE_EMP	0
#define DUMP_MODE_TMDS	1
#define TMDS_BUFFER_SIZE	0x2000000 /*32M*/
#define EMP_BUFFER_SIZE		0x1000	/*4k*/
#define EMP_BUFF_MAX_PKT_CNT	32
#define TMDS_DATA_BUFFER_SIZE	0x200000

struct rx_edid_auto_mode {
	enum edid_ver_e edid_ver; //cur edid_ver
	enum edid_ver_e cfg; //cfg from ui
	bool need_update;
};

/**
 * @short HDMI RX controller HDCP configuration
 */
struct hdmi_rx_hdcp {
	/*hdcp auth state*/
	enum repeater_state_e state;
	/** Repeater mode else receiver only */
	bool repeat;
	bool hdcp14_ready;
	bool cascade_exceed;
	bool dev_exceed;
	unsigned char ds_hdcp_ver;
	bool stream_manage_rcvd;
	u32 topo_updated;
	u8 rpt_reauth_event;
	/*downstream depth*/
	unsigned char depth;
	/*downstream count*/
	u32 count;
	/** Key description seed */
	u32 seed;
	u32 delay;/*according to the timer,5s*/
	/**
	 * Receiver key selection
	 * @note 0: high order, 1: low order
	 */
	u32 bksv[HDCP_BKSV_SIZE];
	/**
	 * Encrypted keys
	 * @note 0: high order, 1: low order
	 */
	u32 keys[HDCP_KEYS_SIZE];
	enum hdcp_version_e hdcp_version;/* 0 no hdcp;1 hdcp14;2 hdcp22 */
	/* add for dv cts */
	enum hdcp_version_e hdcp_pre_ver;
	u8 stream_type;
	bool hdcp_source;
	unsigned char hdcp22_exception;/*esm exception code,reg addr :0x60*/
};

static const unsigned int rx22_ext[] = {
	EXTCON_DISP_HDMI,
	EXTCON_NONE,
};

struct vsi_info_s {
	unsigned int identifier;
	unsigned char vd_fmt;
	unsigned char _3d_structure;
	unsigned char _3d_ext_data;
	u8 dolby_vision_flag;
	bool low_latency;
	bool backlt_md_bit;
	unsigned int eff_tmax_pq;
	bool dv_allm;
	bool hdmi_allm;
	bool hdr10plus;
	bool cuva_hdr;
	bool filmmaker;
	bool imax;
	u8 ccbpc;
	u8 vsi_state; // bit0-6: 4K3D/VSI21/HDR10+/DV10/DV15/CUVA/filmmaker/imax
	u8 emp_pkt_cnt;
	u8 timeout;
	u8 max_frl_rate;
	u8 sys_start_code;
	u8 cuva_version_code;
};

//===============emp start
struct vtem_info_s {
	u8 vrr_en;
	u8 m_const;
	u8 qms_en;
	u8 fva_factor_m1;
	u8 base_vfront;
	u8 rb;
	u16 base_framerate;
};

struct sbtm_info_s {
	u8 flag;
	u8 sbtm_ver;
	u8 sbtm_mode;
	u8 sbtm_type;
	u8 grdm_min;
	u8 grdm_lum;
	u16 frm_pb_limit_int;
};

struct cuva_emds_s {
	bool flag;
	unsigned char *emds_addr;
	u8 cuva_emds_size;
};

struct dv_info_s {
	bool flag;
	u8 dv_addr[1024];
	u8 dv_pkt_cnt;
};

struct emp_dsf_st {
	int pkt_cnt;
	u8 *pkt_addr;
};

//================emp end

#define CHANNEL_STATUS_SIZE   24

struct aud_info_s {
    /* info frame*/
	/*
	 *unsigned char cc;
	 *unsigned char ct;
	 *unsigned char ss;
	 *unsigned char sf;
	 */

	int coding_type;
	int channel_count;
	int sample_frequency;
	int sample_size;
	int coding_extension;
	int auds_ch_alloc;
	int auds_layout;

	/*
	 *int down_mix_inhibit;
	 *int level_shift_value;
	 */
	int aud_hbr_rcv;
	int aud_packet_received;
	/* aud mute by gcp_avmute or aud_spflat mute */
	bool aud_mute_en;
	bool afifo_cfg;
	/* channel status */
	unsigned char channel_status[CHANNEL_STATUS_SIZE];
	unsigned char channel_status_bak[CHANNEL_STATUS_SIZE];
    /**/
	unsigned int cts;
	unsigned int n;
	unsigned int arc;
	/**/
	int real_channel_num;
	int real_sample_size;
	int real_sr;
	u32 aud_clk;
	u8 ch_sts[7];
};

struct phy_sts {
	u32 aud_div;
	u32 pll_rate;
	u32 clk_rate;
	u32 phy_bw;
	u32 pll_bw;
	u32 cablesel;
	ulong timestap;
	u32 err_sum;
	u32 eq_data[256];
	u32 aud_div_1;
};

struct clk_msr {
	u32 cable_clk;
	u32 cable_clk_pre;
	u32 tmds_clk;
	u32 pixel_clk;
	u32 mpll_clk;
	u32 aud_pll;
	u32 adu_div;
	u32 esm_clk;
	u32 p_clk;
	u32 tclk;
	u32 t_clk_pre;
	u32 fpll_clk;
};

struct emp_info_s {
	unsigned int dump_mode;
	struct page *pg_addr;
	phys_addr_t p_addr_a;
	phys_addr_t p_addr_b;
	/*void __iomem *v_addr_a;*/
	/*void __iomem *v_addr_b;*/
	void __iomem *store_a;
	void __iomem *store_b;
	void __iomem *ready;
	unsigned long irq_cnt;
	unsigned int emp_pkt_cnt;
	unsigned int pre_emp_pkt_cnt;
	unsigned int tmds_pkt_cnt;
	bool end;
	u8 ogi_id;
	unsigned int emp_tagid;
	u8 emp_content_type;
	u8 data[128];
	u8 data_ver;
};

struct spkts_rcvd_sts {
	u32 pkt_vsi_rcvd:1;
	u32 pkt_drm_rcvd:1;
	u32 pkt_spd_rcvd:1;
	u32 pkt_vtem_rcvd:1;
	u32 rsvd:28;
};

struct edid_capacity {
	bool vrr;
	bool allm;
	bool hf_db;
	bool dv_db;
	bool hdr10p_db;
	bool hdr_static_db;
	bool hdr_dynamic_db;
	bool freesync_db;
};

enum e_colorimetry {
	E_NULL = 0,
	E_SMPTE_ST_170,
	E_BT_709,
	E_EXTENDED_VALID = 3,
	E_XVYCC_601 = 3,
	E_XVYCC_709,
	E_SYCC_601,
	E_OPYCC_601,
	E_OP_RGB,
	E_BT_2020_YCC,
	E_BI_2020_RGBORYCC,
	E_SMPTE_ST_2113_P3D65RGB = 10,
	E_SMPTE_ST_2113_P3DCIRGB,
	E_BT_2100,
};

enum hdmirx_event {
	HDMIRX_NONE_EVENT = 0,
};

#define MAX_UEVENT_LEN 64
struct hdmirx_uevent {
	const enum hdmirx_event type;
	int state;
	const char *env;
};

int hdmirx_set_uevent(enum hdmirx_event type, int val);

struct rx_info_s {
	struct hdmirx_dev_s *hdmirx_dev;
	enum chip_id_e chip_id;
	enum phy_ver_e phy_ver;
	u8 port_num;
	u8 main_port;
	u8 sub_port;
	u8 vp_cor0_port;
	u8 vp_cor1_port;
	bool boot_flag;
	bool main_port_open;
	bool sub_port_open;
	bool pip_on;
	u8 vrr_min;
	u8 vrr_max;
	u32 arc_port;
	bool arc_5vsts;
	struct rx_aml_phy aml_phy;
	struct rx_aml_phy aml_phy_21;
	struct emp_info_s emp_buff_a; //for vid0
	struct emp_info_s emp_buff_b; //for vid1
	struct edid_capacity edid_cap;
	bool suspend_flag;
};

struct rx_s {
	/** HDMI RX received signal changed */
	u32 skip;
	/*avmute*/
	u32 avmute_skip;
	bool vpp_mute;
	bool cableclk_stb_flg;
	u8 irq_flag;
	/** HDMI RX controller HDCP configuration */
	struct hdmi_rx_hdcp hdcp;
	/*report hpd status to app*/
	/* wrapper */
	unsigned int state;
	unsigned int fsm_ext_state;
	unsigned int pre_state;
	/* recovery mode */
	unsigned char err_rec_mode;
	unsigned char pre_5v_sts;
	unsigned char cur_5v_sts;
	bool no_signal;
	unsigned char err_cnt;
	u16 wait_no_sig_cnt;
	int aud_sr_stable_cnt;
	int aud_sr_unstable_cnt;
	unsigned long timestamp;
	unsigned long stable_timestamp;
	unsigned long unready_timestamp;
	/* info */
	struct rx_video_info pre;
	struct rx_video_info cur;
	struct aud_info_s aud_info;
	struct vsi_info_s vs_info_details;
	struct tvin_3d_meta_data_s threed_info;
	struct tvin_hdr_info_s hdr_info;
	struct vtem_info_s vtem_info;
	struct sbtm_info_s sbtm_info;
	struct cuva_emds_s emp_cuva_info;
	bool vsif_fmm_flag;
	struct dv_info_s emp_dv_info;
	u8 emp_vid_idx;
	struct emp_info_s *emp_info;
	u8 emp_dsf_cnt;
	bool emp_pkt_rev;
	bool new_emp_pkt;
	struct emp_dsf_st emp_dsf_info[EMP_DSF_CNT_MAX];
	unsigned char edid_mix_buf[EDID_MIX_MAX_SIZE];
	unsigned int pwr_sts;
	/* for debug */
	/*struct pd_infoframe_s dbg_info;*/
	struct phy_sts phy;
	struct clk_msr clk;
	enum edid_ver_e edid_ver;
	u8 tx_type;
	bool arc_5vsts;
	u32 vsync_cnt;
	bool vrr_en;
	u8 free_sync_sts;
	u8 afifo_sts;
	u8 vpp_mute_cnt;
	u8 gcp_mute_cnt;
	u32 ecc_err;
	u32 ecc_pkt_cnt;
	u32 ecc_err_frames_cnt;
	bool ddc_filter_en;
	struct rx_var_param var;
	u8 last_hdcp22_state;
	struct rx_aml_phy aml_phy;
	struct rx_aml_phy aml_phy_21;
	//struct spkts_rcvd_sts pkts_sts;
	struct rx_edid_auto_mode edid_type;
	bool resume_flag;
	bool spec_vendor_id;
};

struct reg_map {
	unsigned int phy_addr;
	unsigned int size;
	void __iomem *p;
	int flag;
};

struct phy_port_data {
	int port_idx;
	struct work_struct work;
	struct workqueue_struct *aml_phy_wq;
};

struct work_data {
	struct work_struct work_wq;
	u8 port;
};

#define WHITE_LIST_SIZE 25
enum spec_dev_e {
	/* following devices need to switch to edid2.0 */
	SPEC_DEV_PS5,
	SPEC_DEV_XBOX,
	SPEC_DEV_PS,
	SPEC_DEV_XBOX_SERIES,
	/* following devices need to get SPD earlier */
	SPEC_DEV_PANASONIC,
	SPEC_DEV_CNT
};

enum spec_dev_type_e {
	DEV_UNKNOWN = 0x0,
	DEV_HDMI20 = 0x1,
	SPD_GET_EARLIER = 0x2,
	DEV_HDMI14 = 0x4,
	DEV_ABNORMAL_SCDC = 0x8
};

struct spec_dev_table_s {
	enum spec_dev_type_e dev_type;
	u8 spd_info[WHITE_LIST_SIZE];
};

struct edid_update_work_s {
	struct work_struct work;
	u8 port;
};

/* system */
extern struct delayed_work	eq_dwork;
extern struct workqueue_struct	*eq_wq;
extern struct work_data     scdc_dwork;
extern struct workqueue_struct *scdc_wq;
extern struct delayed_work	esm_dwork;
extern struct workqueue_struct	*esm_wq;
extern struct delayed_work	repeater_dwork;
//extern struct work_struct	aml_phy_dwork;
//extern struct workqueue_struct	*aml_phy_wq;
extern struct work_struct	aml_phy_dwork_port0;
extern struct workqueue_struct	*aml_phy_wq_port0;
extern struct work_struct	aml_phy_dwork_port1;
extern struct workqueue_struct	*aml_phy_wq_port1;
extern struct work_struct	aml_phy_dwork_port2;
extern struct workqueue_struct	*aml_phy_wq_port2;
extern struct work_struct	aml_phy_dwork_port3;
extern struct workqueue_struct	*aml_phy_wq_port3;
extern struct work_struct     clkmsr_dwork;
extern struct workqueue_struct *clkmsr_wq;
extern struct work_struct     earc_hpd_dwork;
extern struct workqueue_struct *earc_hpd_wq;
extern struct workqueue_struct	*repeater_wq;
extern struct work_struct     frl_train_dwork;
extern struct workqueue_struct *frl_train_wq;
extern struct work_struct     frl_train_1_dwork;
extern struct workqueue_struct *frl_train_1_wq;
extern struct edid_update_work_s edid_update_dwork;
extern struct workqueue_struct *edid_update_wq;

extern wait_queue_head_t tx_wait_queue;

extern struct tasklet_struct rx_tasklet;
extern struct device *hdmirx_dev;
extern struct rx_s rx[4];
extern struct rx_info_s rx_info;
extern char boot_info[30][128];
//extern struct phy_port_data aml_phy_dwork;
//extern u8 port_idx;

extern struct tvin_latency_s latency_info;
extern struct reg_map rx_reg_maps[MAP_ADDR_MODULE_NUM];
extern bool downstream_repeat_support;
extern int vrr_range_dynamic_update_en;
extern int allm_update_en;

void rx_tasklet_handler(unsigned long arg);
void skip_frame(unsigned int cnt, u8 port, char *str);

/* reg */

/* packets */
extern unsigned int packet_fifo_cfg;
extern unsigned int *pd_fifo_buf_a;
extern unsigned int *pd_fifo_buf_b;

/* hotplug */
extern unsigned int pwr_sts;
extern int pre_port;
void hotplug_wait_query(void);
void rx_send_hpd_pulse(u8 port);

/* irq */
void rx_irq_en(bool enable, u8 port);
irqreturn_t irq_handler(int irq, void *params);
irqreturn_t irq0_handler(int irq, void *params);
irqreturn_t irq1_handler(int irq, void *params);
irqreturn_t irq2_handler(int irq, void *params);
irqreturn_t irq3_handler(int irq, void *params);

void cecb_irq_handle(void);

/* user interface */
//extern int it_content;
//extern int rgb_quant_range;
//extern int yuv_quant_range;
extern int en_4k_timing;
extern int cec_dev_en;
extern bool dev_is_apple_tv_v2;
extern u32 en_4096_2_3840;
extern int en_4k_2_2k;
extern u32 ops_port;
extern int hdmi_cec_en;
extern int vdin_drop_frame_cnt;
extern int rpt_edid_selection;
extern int rpt_only_mode;
extern u32 vrr_func_en;
extern u32 allm_func_en;
/* debug */
extern bool hdcp_enable;
extern int log_level;
extern int sm_pause;
extern int suspend_pddq_sel;
extern int disable_port_num;
extern int disable_port_en;
extern bool video_stable_to_esm;
extern u32 pwr_sts_to_esm;
extern bool enable_hdcp22_esm_log;
extern bool esm_reset_flag;
extern bool esm_auth_fail_en;
extern bool esm_error_flag;
extern bool hdcp22_stop_auth;
extern bool hdcp22_esm_reset2;
extern int esm_recovery_mode;
extern u32 dbg_pkt;
extern int disable_hdr;
extern int rx_phy_level;
extern int vdin_reset_pcs_en;
extern int rx_5v_wake_up_en;
extern char edid_cur[EDID_SIZE];
extern int vpp_mute_cnt;
extern int gcp_mute_cnt;
extern int gcp_mute_flag[4];
extern int def_trim_value;
extern int edid_auto_sel;
#ifdef CONFIG_AMLOGIC_MEDIA_VRR
extern struct notifier_block vrr_notify;
#endif
#ifdef CONFIG_AMLOGIC_HDMITX
extern struct notifier_block tx_notify;
void hdmitx_update_latency_info(struct tvin_latency_s *latency_info);
#endif
void __weak hdmitx_update_latency_info(struct tvin_latency_s *latency_info)
{
}

u8 rx_get_port_type(u8 port);
bool rx_is_pip_on(void);
int rx_set_global_variable(const char *buf, int size);
void rx_get_global_variable(const char *buf);
int rx_pr(const char *fmt, ...);
unsigned int hdmirx_hw_dump_reg(unsigned char *buf, int size);
unsigned int hdmirx_show_info(unsigned char *buf, int size, u8 port);
unsigned int hdmirx_show_info_t3x(unsigned char *buf, int size);
bool is_aud_fifo_error(void);
bool is_aud_pll_error(void);
int hdmirx_debug(const char *buf, int size);
void dump_reg(u8 port);
void dump_edid_reg(u32 size);
void rx_debug_loadkey(u8 port);
void rx_debug_load22key(u8 port);
int rx_debug_wr_reg(const char *buf, char *tmpbuf, int i, u8 port);
int rx_debug_rd_reg(const char *buf, char *tmpbuf, u8 port);
void rx_update_sig_info(u8 port);
int aml_vrr_atomic_notifier_register(struct notifier_block *nb);
/* repeater */
bool hdmirx_repeat_support(void);

/* edid-hdcp14 */
extern unsigned int downstream_hpd_flag;
void hdmirx_fill_edid_buf(const char *buf, int size);
void hdmirx_fill_edid_with_port_buf(const char *buf, int size);
void hdmirx_fill_key_buf(const char *buf, int size);
extern int rx_audio_block_len;
extern u8 rx_audio_block[MAX_AUDIO_BLK_LEN];

/* for other modules */
void rx_is_hdcp22_support(void);
int rx_hdcp22_send_uevent(int val);

/* for cec set tx_type */
void register_cec_rx_notify(cec_spd_callback callback);

//#define RX_VER0 "ver.2021/06/21"
//1. added colorspace detection
//2. add afifo detection
//3. use top de-repeat

//#define RX_VER0 "ver.2021/07/06"
//waiting i2c idle before pull dow HPD

// fix vsif irq issue
//#define RX_VER0 "ver.2021/07/13"

#endif
