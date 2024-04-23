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
#include <linux/init.h>
#include <linux/device.h>
#include <linux/mm.h>
#include <linux/major.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/of_gpio.h>
#include <linux/amlogic/media/frame_provider/tvin/tvin.h>
#include <linux/amlogic/media/video_sink/video.h>
#include <linux/amlogic/media/sound/hdmi_earc.h>
#include <linux/sched/clock.h>
#include <linux/amlogic/clk_measure.h>

/* Local include */
#include "hdmi_rx_repeater.h"
#include "hdmi_rx_drv.h"
#include "hdmi_rx_hw.h"
#include "hdmi_rx_eq.h"
#include "hdmi_rx_wrapper.h"
#include "hdmi_rx_pktinfo.h"
#include "hdmi_rx_edid.h"
#include "hdmi_rx_drv_ext.h"
#include "hdmi_rx_hw_t5.h"
#include "hdmi_rx_hw_t7.h"
#include "hdmi_rx_hw_tl1.h"
#include "hdmi_rx_hw_tm2.h"
#include "hdmi_rx_hw_t5m.h"
#include "hdmi_rx_hw_t3x.h"
#include "hdmi_rx_hw_txhd2.h"

static int pow5v_max_cnt = 2;
static int pll_unlock_max;
static int pll_lock_max;
static int dwc_rst_wait_cnt_max;
static int spec_dev_wait_cnt_max;
static int sig_stable_max;
static int sig_stable_err_max;
static int err_cnt_sum_max;
static int hpd_wait_max;
static int sig_unstable_max;
static int sig_unready_max;
static int fps_unready_max;

//int rgb_quant_range;
//int yuv_quant_range;
//int it_content;
static int diff_pixel_th;
static int diff_line_th;
static int diff_frame_th;
static int aud_sr_stb_max;
//static int audio_coding_type;
//static int audio_channel_count;
int irq_err_max = 300;
int de_err_max = 30;

static int clk_unstable_max;
static int clk_stable_max;
static int unnormal_wait_max = 200;
static int wait_no_sig_max = 600;
static int fpll_stable_max = 50;
static int reset_pcs_en;
int fsm_debug;
static int ecc_err_monitor;
u32 vrr_func_en = 1;
u32 allm_func_en = 0xff;
u32 htotal_cnt;

typedef void (*pf_callback)(int earc_port, bool st);
static pf_callback earc_hdmirx_hpdst;
static cec_callback cec_hdmirx5v_update;

/* for debug */
//bool clk_debug_en;
u32 rx5v_debug_en;
u32 err_chk_en;
u32 force_vic;
int log_level = LOG_EN;
u32 dbg_cs;
u32 dbg_pkt;// = 0x81;
int color_bar_debug_en;
int color_bar_lvl;
/* used in other module */
//static int audio_sample_rate;
int reset_pcs_flag;
int reset_pcs_cnt = 10;
int port_debug_en;
int fpll_ready_max = 1;
int rx_emp_dbg_en;
int frl_lock_det_max = 10;
//for rs err test
int frl_debug_en;
int rs_err_chk;
int err_cnt = 100;
bool cts_ced_err_test;
//static int auds_rcv_sts;
//module_param(auds_rcv_sts, int, 0664);
//MODULE_PARM_DESC(auds_rcv_sts, "auds_rcv_sts");

/* to inform ESM whether the cable is connected or not */
bool hpd_to_esm;
MODULE_PARM_DESC(hpd_to_esm, "\n hpd_to_esm\n");
module_param(hpd_to_esm, bool, 0664);

int hdcp22_kill_esm;
MODULE_PARM_DESC(hdcp22_kill_esm, "\n hdcp22_kill_esm\n");
module_param(hdcp22_kill_esm, int, 0664);

bool hdcp_mode_sel;
MODULE_PARM_DESC(hdcp_mode_sel, "\n hdcp_mode_sel\n");
module_param(hdcp_mode_sel, bool, 0664);

bool esm_auth_fail_en;
MODULE_PARM_DESC(esm_auth_fail_en, "\n esm_auth_fail_en\n");
module_param(esm_auth_fail_en, bool, 0664);

/* to inform hdcp_rx22 whether there's any device connected */
u32 pwr_sts_to_esm;
static int hdcp22_capable_sts = 0xff;
bool esm_error_flag;

/*the esm reset flag for hdcp_rx22*/
bool esm_reset_flag;
MODULE_PARM_DESC(esm_reset_flag, "\n esm_reset_flag\n");
module_param(esm_reset_flag, bool, 0664);

/* to inform ESM whether the cable is connected or not */
bool video_stable_to_esm;
MODULE_PARM_DESC(video_stable_to_esm, "\n video_stable_to_esm\n");
module_param(video_stable_to_esm, bool, 0664);

bool enable_hdcp22_esm_log;
MODULE_PARM_DESC(enable_hdcp22_esm_log, "\n enable_hdcp22_esm_log\n");
module_param(enable_hdcp22_esm_log, bool, 0664);

int hdcp22_auth_sts = 0xff;
MODULE_PARM_DESC(hdcp22_auth_sts, "\n hdcp22_auth_sts\n");
module_param(hdcp22_auth_sts, int, 0664);

bool hdcp22_esm_reset2;
MODULE_PARM_DESC(hdcp22_esm_reset2, "\n hdcp22_esm_reset2\n");
module_param(hdcp22_esm_reset2, bool, 0664);

bool hdcp22_stop_auth;
module_param(hdcp22_stop_auth, bool, 0664);
MODULE_PARM_DESC(hdcp22_stop_auth, "hdcp22_stop_auth");

int hdcp14_on;
MODULE_PARM_DESC(hdcp14_on, "\n hdcp14_on\n");
module_param(hdcp14_on, int, 0664);

/*esm recovery mode for changing resolution & hdmi2.0*/
int esm_recovery_mode = ESM_REC_MODE_TMDS;
module_param(esm_recovery_mode, int, 0664);
MODULE_PARM_DESC(esm_recovery_mode, "esm_recovery_mode");

/* No need to judge  frame rate while checking timing stable,as there are
 * some out-spec sources whose framerate change a lot(e.g:59.7~60.16hz).
 * Other brands of tv can support this,we also need to support.
 */
static int stable_check_lvl;

/* If dvd source received the frequent pulses on HPD line,
 * It will sent a length of dirty audio data sometimes.it's TX's issues.
 * Compared with other brands TV, delay 1.5S to avoid this noise.
 */
static int edid_update_delay = 150;
int skip_frame_cnt = 2;
u32 hdcp22_reauth_enable = 1;
static u32 hdcp22_stop_auth_enable;
static u32 hdcp22_esm_reset2_enable;
int sm_pause;
int pre_port = 0xff;
u32 vsync_err_cnt_max = 10;
static int esd_phy_rst_max;
static int detect_time_min = 210;
static int cec_dev_info;
struct rx_info_s rx_info;
struct rx_s rx[HDMIRX_PORT_MAX];
//u8 rx_info.main_port;
static bool term_flag = 1;
/* vpp mute when signal change, used
 * in companion with vlock phase = 84
 */
u32 vpp_mute_enable = 1;
int clk_chg_cnt;
int clk_chg_max = 3;
static int edid_auto_chk_max = 4;
// 1. connected to a non-hdcp device
// for dv_cts HDMI 40-47 switch input per 2s
// test equipment does not support hdcp.
// 2. connected to a HDCP2.2 device
// 2.1 fix appletv BSOD issue.
// 2.2 for repeater. need to wait upstream's hdcp
// communication to avoid black screen twice
// 3. for hdcp1.4 cts. need to wait for hdcp start.
// waiting time cannot be reduced 1S
static int hdcp_none_wait_max = 80;
int fpll_chk_cnt;

void hdmirx_phy_var_init(void)
{
	if (rx_info.phy_ver == PHY_VER_TM2) {
		/* for tm2_b */
		rx_info.aml_phy.dfe_en = 1;
		rx_info.aml_phy.ofst_en = 1;
		rx_info.aml_phy.cdr_mode = 0;
		rx_info.aml_phy.pre_int = 1;
		rx_info.aml_phy.pre_int_en = 0;
		rx_info.aml_phy.phy_bwth = 1;
		rx_info.aml_phy.alirst_en = 0;
		rx_info.aml_phy.tap1_byp = 1;
		rx_info.aml_phy.eq_byp = 0;
		rx_info.aml_phy.long_cable = 1;
		rx_info.aml_phy.osc_mode = 0;
		rx_info.aml_phy.pll_div = 1;
		rx_info.aml_phy.sqrst_en = 0;
		rx_info.aml_phy.vga_dbg = 1;
		rx_info.aml_phy.vga_dbg_delay = 200;
		rx_info.aml_phy.eq_fix_val = 16;
		rx_info.aml_phy.cdr_fr_en = 0;
		rx_info.aml_phy.force_sqo = 0;
		rx_info.aml_phy.reset_pcs_en = 0;
	} else if (rx_info.phy_ver >= PHY_VER_T5 && rx_info.phy_ver <= PHY_VER_T5W) {
		rx_info.aml_phy.dfe_en = 1;
		rx_info.aml_phy.ofst_en = 0;
		rx_info.aml_phy.cdr_mode = 0;
		rx_info.aml_phy.pre_int = 1;
		rx_info.aml_phy.pre_int_en = 0;
		rx_info.aml_phy.phy_bwth = 1;
		rx_info.aml_phy.alirst_en = 0;
		rx_info.aml_phy.tap1_byp = 1;
		rx_info.aml_phy.eq_byp = 1;
		rx_info.aml_phy.long_cable = 1;
		rx_info.aml_phy.osc_mode = 0;
		rx_info.aml_phy.pll_div = 1;
		rx_info.aml_phy.sqrst_en = 0;
		rx_info.aml_phy.vga_dbg = 1;
		rx_info.aml_phy.vga_dbg_delay = 200;
		rx_info.aml_phy.eq_fix_val = 16;
		rx_info.aml_phy.cdr_fr_en = 100;
		rx_info.aml_phy.force_sqo = 0;
		/* add for t5 */
		rx_info.aml_phy.os_rate = 3;
		rx_info.aml_phy.cdr_mode = 1;
		rx_info.aml_phy.vga_gain = 0x1000;
		rx_info.aml_phy.eq_stg1 = 0x1f;
		rx_info.aml_phy.eq_stg2 = 0x1f;
		rx_info.aml_phy.eq_hold = 1;
		rx_info.aml_phy.dfe_hold  = 0;
		rx_info.aml_phy.eye_delay = 1000;
		rx_info.aml_phy.eq_retry = 1;
		rx_info.aml_phy.tap2_byp = 0;
		rx_info.aml_phy.long_bist_en = 0;
		rx_info.aml_phy.reset_pcs_en = 0;
	} else if (rx_info.phy_ver >= PHY_VER_T5M) {
		rx_info.aml_phy.dfe_en = 1;
		rx_info.aml_phy.ofst_en = 0;
		rx_info.aml_phy.cdr_mode = 0;
		rx_info.aml_phy.pre_int = 1;
		rx_info.aml_phy.pre_int_en = 1;
		rx_info.aml_phy.phy_bwth = 0;
		rx_info.aml_phy.alirst_en = 0;
		rx_info.aml_phy.tap1_byp = 1;
		rx_info.aml_phy.eq_byp = 1;
		rx_info.aml_phy.long_cable = 1;
		rx_info.aml_phy.osc_mode = 0;
		rx_info.aml_phy.pll_div = 1;
		rx_info.aml_phy.sqrst_en = 0;
		rx_info.aml_phy.vga_dbg = 1;
		rx_info.aml_phy.vga_dbg_delay = 200;
		rx_info.aml_phy.eq_fix_val = 16;
		rx_info.aml_phy.cdr_fr_en = 500;
		rx_info.aml_phy.force_sqo = 0;
		/* add for t5 */
		rx_info.aml_phy.os_rate = 3;
		rx_info.aml_phy.cdr_mode = 1;
		rx_info.aml_phy.vga_gain = 0x1000;
		rx_info.aml_phy.eq_stg1 = 0x1f;
		rx_info.aml_phy.eq_stg2 = 0x1f;
		rx_info.aml_phy.eq_hold = 0;
		rx_info.aml_phy.dfe_hold  = 0;
		rx_info.aml_phy.eye_delay = 50;
		rx_info.aml_phy.eq_retry = 0;
		rx_info.aml_phy.tap2_byp = 0;
		rx_info.aml_phy.long_bist_en = 0;
		rx_info.aml_phy.reset_pcs_en = 1;
		rx_info.aml_phy.enhance_dfe_en_old = 1;
		rx_info.aml_phy.enhance_dfe_en_new = 0;
		rx_info.aml_phy.cdr_retry_en = 1;
		rx_info.aml_phy.agc_enable = 0;
		rx_info.aml_phy.enhance_eq = 0;
		rx_info.aml_phy.cdr_fr_en_auto = 0;
		rx_info.aml_phy.eq_en = 1;
		rx_info.aml_phy.eye_height = 5;
		rx_info.aml_phy.hyper_gain_en = 0;
		rx_info.aml_phy.eye_height_min = 8;
		// for t3x 2.1 phy
		if (rx_info.phy_ver == PHY_VER_T3X && !rx_info.aml_phy.phy_debug_en) {
			rx_info.aml_phy_21.phy_bwth = 1;
			rx_info.aml_phy_21.vga_gain = 0x10000;
			rx_info.aml_phy_21.eq_stg1 = 0x1f;
			rx_info.aml_phy_21.eq_stg2 = 0x8;
			rx_info.aml_phy_21.eq_pole = 0x8;
			rx_info.aml_phy_21.buf_gain = 0x8;
			rx_info.aml_phy_21.cdr_fr_en = 0;//0x1f4
			rx_info.aml_phy_21.eq_hold = 0x3;
			rx_info.aml_phy_21.eq_retry = 0x1;
			rx_info.aml_phy_21.dfe_en = 0x1;
			rx_info.aml_phy_21.dfe_hold = 0;//0x3
			rx_info.aml_phy_21.vga_tune = 0x1;
			rx_info.aml_phy.phy_debug_en = 0x1;//0x1
			rx_info.aml_phy_21.pre_int = 0x1;
			rx_info.aml_phy_21.pre_int_en = 0x1;
		}
	}
}

void hdmirx_fsm_var_init(void)
{
	switch (rx_info.chip_id) {
	case CHIP_ID_TL1:
	case CHIP_ID_TM2:
		sig_stable_err_max = 5;
		sig_stable_max = 10;
		dwc_rst_wait_cnt_max = 1;
		spec_dev_wait_cnt_max = 200;
		clk_unstable_max = 50;
		esd_phy_rst_max = 16;
		pll_unlock_max = 30;
		stable_check_lvl = 0x7cf;
		pll_lock_max = 5;
		err_cnt_sum_max = 10;
		/* increase time of hpd low, to avoid some source like */
		/* MTK box/KaiboerH9 i2c communicate error */
		hpd_wait_max = 74;
		sig_unstable_max = 20;
		sig_unready_max = 0;
		diff_pixel_th = 2;
		diff_line_th = 2;
		/* (25hz-24hz)/2 = 50/100 */
		diff_frame_th = 40;
		aud_sr_stb_max = 30;
		clk_stable_max = 3;
		break;
	case CHIP_ID_T5:
	case CHIP_ID_T5D:
		sig_stable_err_max = 5;
		sig_stable_max = 10;
		dwc_rst_wait_cnt_max = 30;
		spec_dev_wait_cnt_max = 200;
		clk_unstable_max = 50;
		esd_phy_rst_max = 16;
		pll_unlock_max = 30;
		stable_check_lvl = 0x7cf;
		pll_lock_max = 5;
		err_cnt_sum_max = 10;
		hpd_wait_max = 74;
		sig_unstable_max = 20;
		sig_unready_max = 0;
		diff_pixel_th = 2;
		diff_line_th = 2;
		diff_frame_th = 40;
		aud_sr_stb_max = 30;
		clk_stable_max = 3;
		break;
	case CHIP_ID_GXTVBB:
	case CHIP_ID_TXL:
	case CHIP_ID_TXLX:
	case CHIP_ID_TXHD:
		sig_stable_err_max = 5;
		sig_stable_max = 10;
		dwc_rst_wait_cnt_max = 110;
		spec_dev_wait_cnt_max = 200;
		clk_unstable_max = 100;
		esd_phy_rst_max = 4;
		pll_unlock_max = 30;
		stable_check_lvl = 0x17cf;
		pll_lock_max = 2;
		err_cnt_sum_max = 10;
		hpd_wait_max = 74;
		sig_unstable_max = 20;
		sig_unready_max = 0;
		diff_pixel_th = 2;
		diff_line_th = 2;
		diff_frame_th = 40;
		aud_sr_stb_max = 30;
		clk_stable_max = 3;
		break;
	case CHIP_ID_T5M:
	case CHIP_ID_TXHD2:
		hbr_force_8ch = 1; //use it to enable hdr2spdif
		sig_stable_err_max = 5;
		sig_stable_max = 10;
		dwc_rst_wait_cnt_max = 30;
		spec_dev_wait_cnt_max = 200;
		clk_unstable_max = 50;
		esd_phy_rst_max = 16;
		pll_unlock_max = 30;
		//do not to check colorspace changes
		//Vdin can adapt it automatically
		stable_check_lvl = 0x7c3;
		pll_lock_max = 2;
		err_cnt_sum_max = 10;
		hpd_wait_max = 74;
		sig_unstable_max = 20;
		sig_unready_max = 0;
		/* decreased to 2 */
		/* decreased to 2 */
		diff_pixel_th = 1;
		/* decreased to 2 */
		diff_line_th = 1;
		/* (25hz-24hz)/2 = 50/100 */
		diff_frame_th = 40;
		aud_sr_stb_max = 30;
		clk_stable_max = 3;
		rx_phy_level = 5;
		fps_unready_max = 3;
		break;
	case CHIP_ID_T3X:
		hbr_force_8ch = 1; //use it to enable hdr2spdif
		sig_stable_err_max = 5;
		sig_stable_max = 10;
		fps_unready_max = 3;
		dwc_rst_wait_cnt_max = 30;
		spec_dev_wait_cnt_max = 200;
		clk_unstable_max = 50;
		esd_phy_rst_max = 4;
		pll_unlock_max = 30;
		//do not to check colorspace changes
		//Vdin can adapt it automatically
		stable_check_lvl = 0x6c3;
		pll_lock_max = 2;
		err_cnt_sum_max = 10;
		hpd_wait_max = 74;
		sig_unstable_max = 20;
		sig_unready_max = 0;
		/* decreased to 2 */
		/* decreased to 2 */
		diff_pixel_th = 1;
		/* decreased to 2 */
		diff_line_th = 1;
		/* (25hz-24hz)/2 = 50/100 */
		diff_frame_th = 40;
		aud_sr_stb_max = 30;
		clk_stable_max = 3;
		rx_phy_level = 5;
		break;
	case CHIP_ID_T7:
	case CHIP_ID_T3:
	case CHIP_ID_T5W:
	default:
		hbr_force_8ch = 1; //use it to enable hdr2spdif
		sig_stable_err_max = 5;
		sig_stable_max = 10;
		dwc_rst_wait_cnt_max = 5; //for repeater
		spec_dev_wait_cnt_max = 200;
		clk_unstable_max = 50;
		esd_phy_rst_max = 16;
		pll_unlock_max = 30;
		//do not to check colorspace changes
		//Vdin can adapt it automatically
		stable_check_lvl = 0x7c3;
		pll_lock_max = 2;
		err_cnt_sum_max = 10;
		hpd_wait_max = 74;
		sig_unstable_max = 20;
		sig_unready_max = 0;
		/* decreased to 2 */
		/* decreased to 2 */
		diff_pixel_th = 1;
		/* decreased to 2 */
		diff_line_th = 1;
		/* (25hz-24hz)/2 = 50/100 */
		diff_frame_th = 40;
		aud_sr_stb_max = 30;
		clk_stable_max = 3;
		break;
	}
}

void hdmirx_init_params(u8 port)
{
	/* for fsm var init */
	hdmirx_fsm_var_init();
	/* for phy variable */
	hdmirx_phy_var_init();
}

void rx_hpd_to_esm_handle(struct work_struct *work)
{
	if (rx_info.chip_id >= CHIP_ID_T7)
		return;
	cancel_delayed_work(&esm_dwork);
	rx_hdcp22_send_uevent(0);
	rx_pr("esm_hpd-0\n");
	msleep(80);
	rx_hdcp22_send_uevent(1);
	rx_pr("esm_hpd-1\n");
}

int cec_set_dev_info(u8 dev_idx)
{
	cec_dev_info |= 1 << dev_idx;

	if (rx_info.chip_id >= CHIP_ID_T7)
		return 0;
	if (dev_idx == 1)
		hdcp_enc_mode = 1;
	if (dev_idx == 2 && cec_dev_en)
		dev_is_apple_tv_v2 = 1;
	rx_pr("cec special dev = %x", dev_idx);
	return 0;
}
EXPORT_SYMBOL(cec_set_dev_info);

#define DEV_CNT 1
#define VENDOR_ID_LEN 3
const u8 device_vendor_id[DEV_CNT][VENDOR_ID_LEN] = {
	{0x08, 0x00, 0x46},
};

int cec_set_vendor_id(u8 *vendor_id)
{
	int i = 0;

	for (i = 0; i < DEV_CNT; i++) {
		if (!memcmp((char *)vendor_id, device_vendor_id[i], VENDOR_ID_LEN)) {
			rx[rx_info.main_port].spec_vendor_id = true;
			if (log_level & 0x1000)
				rx_pr("vendor id=%s\n", vendor_id);
		}
	}
	return 0;
}
EXPORT_SYMBOL(cec_set_vendor_id);

int register_earctx_callback(pf_callback callback)
{
	earc_hdmirx_hpdst = callback;
	return 0;
}
EXPORT_SYMBOL(register_earctx_callback);

void unregister_earctx_callback(void)
{
	earc_hdmirx_hpdst = NULL;
}
EXPORT_SYMBOL(unregister_earctx_callback);

int register_cec_callback(cec_callback callback)
{
	cec_hdmirx5v_update = callback;
	return 0;
}
EXPORT_SYMBOL(register_cec_callback);

void unregister_cec_callback(void)
{
	cec_hdmirx5v_update = NULL;
}
EXPORT_SYMBOL(unregister_cec_callback);

void rx_update_edid_callback(enum tvin_port_e tvin_port, u32 hdr_priority)
{
	u8 port;

	port = (tvin_port - TVIN_PORT_HDMI0) & 0xff;
#ifdef CONFIG_AMLOGIC_HDMITX
	tx_hdr_priority = hdr_priority;
#endif
	rx_pr(" edid update:%d\n", hdr_priority);
	hdmi_rx_top_edid_update();
	port_hpd_rst_flag |= (1 << rx_info.main_port);
	rx[port].state = FSM_HPD_LOW;
}
EXPORT_SYMBOL(rx_update_edid_callback);

static bool video_mute_enabled(u8 port)
{
	if (rx[port].state != FSM_SIG_READY || port == rx_info.sub_port)
		return false;

	if (!rx_info.main_port_open)
		return false;
	/* for debug with flicker issues, especially
	 * unplug or switch timing under game mode
	 */
	if (vpp_mute_enable)
		return true;
	else
		return false;
}

/*
 *func: irq tasklet
 *param: flag:
 *	0x01:	audio irq
 *	0x02:	packet irq
 */
void rx_tasklet_handler(unsigned long arg)
{
	struct rx_s *prx = (struct rx_s *)arg;
	u8 irq_flag = prx->irq_flag;
	u8 port = E_PORT0;

	/* prx->irq_flag = 0; /
	 * if (irq_flag & IRQ_PACKET_FLAG)
	 *	rx_pkt_handler(PKT_BUFF_SET_FIFO);
	 */
	/*pkt overflow or underflow*/
	if (irq_flag & IRQ_PACKET_ERR) {
		hdmirx_packet_fifo_rst();
		irq_flag &= ~IRQ_PACKET_ERR;
	}

	if (irq_flag & IRQ_AUD_FLAG) {
		hdmirx_audio_fifo_rst(port);
		irq_flag &= ~IRQ_AUD_FLAG;
	}
	prx->irq_flag = irq_flag;
	/*irq_flag = 0;*/
}

static int rx_dwc_irq_handler(void)
{
	u8 port = rx_info.main_port;
	int error = 0;
	/* unsigned i = 0; */
	u32 intr_hdmi = 0;
	u32 intr_md = 0;
	u32 intr_pedc = 0;
	/* u32 intr_aud_clk = 0; */
	u32 intr_aud_fifo = 0;
	u32 intr_hdcp22 = 0;
	bool vsi_handle_flag = false;
	bool drm_handle_flag = false;
	bool emp_handle_flag = false;
	u32 rx_top_intr_stat = 0;
	bool irq_need_clr = 0;

	/* clear interrupt quickly */
	intr_hdmi =
	    hdmirx_rd_dwc(DWC_HDMI_ISTS) &
	    hdmirx_rd_dwc(DWC_HDMI_IEN);
	if (intr_hdmi != 0)
		hdmirx_wr_dwc(DWC_HDMI_ICLR, intr_hdmi);

	intr_md =
	    hdmirx_rd_dwc(DWC_MD_ISTS) &
	    hdmirx_rd_dwc(DWC_MD_IEN);
	if (intr_md != 0)
		hdmirx_wr_dwc(DWC_MD_ICLR, intr_md);

	intr_pedc =
	    hdmirx_rd_dwc(DWC_PDEC_ISTS) &
	    hdmirx_rd_dwc(DWC_PDEC_IEN);
	if (intr_pedc != 0)
		hdmirx_wr_dwc(DWC_PDEC_ICLR, intr_pedc);

	intr_aud_fifo =
	    hdmirx_rd_dwc(DWC_AUD_FIFO_ISTS) &
	    hdmirx_rd_dwc(DWC_AUD_FIFO_IEN);
	if (intr_aud_fifo != 0)
		hdmirx_wr_dwc(DWC_AUD_FIFO_ICLR, intr_aud_fifo);

	if (hdcp22_on) {
		intr_hdcp22 =
			hdmirx_rd_dwc(DWC_HDMI2_ISTS) &
		    hdmirx_rd_dwc(DWC_HDMI2_IEN);
	}
	if (intr_hdcp22 != 0) {
		hdmirx_wr_dwc(DWC_HDMI2_ICLR, intr_hdcp22);
		skip_frame(skip_frame_cnt, rx_info.main_port, "dwc hdcp intr");
		rx_pr("intr=%#x\n", intr_hdcp22);
		switch (intr_hdcp22) {
		case _BIT(0):
			hdcp22_capable_sts = HDCP22_AUTH_STATE_CAPABLE;
			/* rx[port].hdcp.hdcp_version = HDCP_VER_22; */
			break;
		case _BIT(1):
			hdcp22_capable_sts = HDCP22_AUTH_STATE_NOT_CAPABLE;
			break;
		case _BIT(2):
			hdcp22_auth_sts = HDCP22_AUTH_STATE_LOST;
			break;
		case _BIT(3):
			hdcp22_auth_sts = HDCP22_AUTH_STATE_SUCCESS;
			rx[port].hdcp.hdcp_version = HDCP_VER_22;
			rx[port].hdcp.hdcp_source = true;
			break;
		case _BIT(4):
			hdcp22_auth_sts = HDCP22_AUTH_STATE_FAILED;
			break;
		case _BIT(5):
			break;
		default:
			break;
		}
	}

	if (rx_info.chip_id < CHIP_ID_TL1) {
		rx_top_intr_stat = hdmirx_rd_top(TOP_INTR_STAT, port);
		if (rx_top_intr_stat & _BIT(31))
			irq_need_clr = 1;
	}

	/* check hdmi open status before dwc isr */
	if (!rx_info.main_port_open) {
		if (irq_need_clr)
			error = 1;
		if (log_level & DBG_LOG)
			rx_pr("[isr] ignore dwc isr ---\n");
		return error;
	}

	if (intr_hdmi != 0) {
		if (rx_get_bits(intr_hdmi, AKSV_RCV) != 0) {
			rx_pr("[*aksv*\n");
			rx[port].hdcp.hdcp_version = HDCP_VER_14;
			rx[port].hdcp.hdcp_source = true;
#ifdef CONFIG_AMLOGIC_HDMITX
			if (hdmirx_repeat_support())
				rx_start_repeater_auth();
#endif
		}
	}

	if (intr_md != 0) {
		if (rx_get_bits(intr_md, md_ists_en) != 0) {
			if (log_level & 0x100)
				rx_pr("md_ists:%x\n", intr_md);
		}
	}

	if (intr_pedc != 0) {
		if (rx_get_bits(intr_pedc, DVIDET | AVI_CKS_CHG) != 0) {
			if (log_level & 0x400)
				rx_pr("[irq] AVI_CKS_CHG\n");
		}
		if (rx_get_bits(intr_pedc, AVI_RCV) != 0) {
			if (log_level & 0x400)
				rx_pr("[irq] AVI_RCV\n");
			if (rx[port].var.de_stable)
				rx[port].var.avi_rcv_cnt++;
		}
		if (rx_get_bits(intr_pedc, VSI_RCV) != 0) {
			if (log_level & 0x400)
				rx_pr("[irq] VSI_RCV\n");
			vsi_handle_flag = true;
		}
		if (rx_get_bits(intr_pedc, VSI_CKS_CHG) != 0) {
			if (log_level & 0x400)
				rx_pr("[irq] VSI_CKS_CHG\n");
		}
		if (rx_get_bits(intr_pedc, PD_FIFO_START_PASS) != 0) {
			if (log_level & VSI_LOG)
				rx_pr("[irq] FIFO START\n");
			/* rx[port].irq_flag |= IRQ_PACKET_FLAG; */
		}
		if (rx_get_bits(intr_pedc, PD_FIFO_NEW_ENTRY) != 0) {
			if (log_level & VSI_LOG)
				rx_pr("[irq] FIFO NEW_ENTRY\n");
			rx[port].irq_flag |= IRQ_PACKET_FLAG;
			/* rx_pkt_handler(PKT_BUFF_SET_FIFO); */
		}
		if (rx_get_bits(intr_pedc, _BIT(1)) != 0) {
			if (log_level & 0x200)
				rx_pr("[irq] FIFO MAX\n");
		}
		if (rx_get_bits(intr_pedc, _BIT(0)) != 0) {
			if (log_level & 0x200)
				rx_pr("[irq] FIFO MIN\n");
		}
		if (rx_get_bits(intr_pedc, GCP_AV_MUTE_CHG) != 0) {
			if (log_level & 0x100)
				rx_pr("[irq] GCP_AV_MUTE_CHG\n");
		}

		if (rx_info.chip_id >= CHIP_ID_TL1) {
			if (rx_get_bits(intr_pedc,
					_BIT(9)) != 0) {
				if (log_level & IRQ_LOG)
					rx_pr("[irq] EMP_RCV %#x\n",
					      intr_pedc);
				emp_handle_flag = true;
			} else if (rx_get_bits(intr_pedc,
					DRM_RCV_EN_TXLX) != 0) {
				if (log_level & IRQ_LOG)
					rx_pr("[irq] DRM_RCV_EN %#x\n",
					      intr_pedc);
				drm_handle_flag = true;
			}
		} else if (rx_info.chip_id == CHIP_ID_TXLX) {
			if (rx_get_bits(intr_pedc,
					DRM_RCV_EN_TXLX) != 0) {
				if (log_level & IRQ_LOG)
					rx_pr("[irq] DRM_RCV_EN %#x\n",
					      intr_pedc);
				drm_handle_flag = true;
			}
		} else {
			if (rx_get_bits(intr_pedc,
					DRM_RCV_EN) != 0) {
				if (log_level & IRQ_LOG)
					rx_pr("[irq] DRM_RCV_EN %#x\n",
					      intr_pedc);
				drm_handle_flag = true;
			}
		}
		if (rx_get_bits(intr_pedc, PD_FIFO_OVERFL) != 0) {
			if (log_level & VSI_LOG)
				rx_pr("[irq] PD_FIFO_OVERFL\n");
			rx[port].irq_flag |= IRQ_PACKET_ERR;
			hdmirx_packet_fifo_rst();
		}
		if (rx_get_bits(intr_pedc, PD_FIFO_UNDERFL) != 0) {
			if (log_level & VSI_LOG)
				rx_pr("[irq] PD_FIFO_UNDFLOW\n");
			rx[port].irq_flag |= IRQ_PACKET_ERR;
			hdmirx_packet_fifo_rst();
		}
	}

	if (intr_aud_fifo != 0) {
		if (rx_get_bits(intr_aud_fifo, OVERFL) != 0) {
			if (log_level & 0x100)
				rx_pr("[irq] OVERFL\n");
			if (rx[port].state == FSM_SIG_READY)
				hdmirx_audio_fifo_rst(port);
		}
		if (rx_get_bits(intr_aud_fifo, UNDERFL) != 0) {
			if (log_level & 0x100)
				rx_pr("[irq] UNDERFL\n");
			if (rx[port].state == FSM_SIG_READY)
				hdmirx_audio_fifo_rst(port);
		}
	}
	if (vsi_handle_flag)
		rx_pkt_handler(PKT_BUFF_SET_VSI, port);

	if (drm_handle_flag)
		rx_pkt_handler(PKT_BUFF_SET_DRM, port);

	if (emp_handle_flag)
		rx_pkt_handler(PKT_BUFF_SET_EMP, port);

	if (rx[port].irq_flag)
		tasklet_schedule(&rx_tasklet);

	if (irq_need_clr)
		error = 1;

	return error;
}

static int rx_cor3_irq_handler(void)
{
	int error = 0;
	u8 intr_2 = 0;
	u8 intr_3 = 0;
	u8 intr_5 = 0;
	u8 intr_6 = 0;
	u8 rx_intr_1 = 0;
	u8 rx_intr_4 = 0;
	u8 rx_intr_13 = 0;
	u8 rx_depack2_intr0;
	u8 rx_depack2_intr2;
	u8 rx_hdcp1x_intr0;
	u8 rx_hdcp2x_intr0;
	u8 rx_hdcp2x_intr1;
	u8 hdcp_2x_ecc_intr;
	u8 port = E_PORT3;

	hdcp_2x_ecc_intr = hdmirx_rd_cor(HDCP2X_RX_ECC_INTR, port);
	if (hdcp_2x_ecc_intr != 0)
		hdmirx_wr_cor(HDCP2X_RX_ECC_INTR, hdcp_2x_ecc_intr, port);

	intr_2 = hdmirx_rd_cor(RX_DEPACK_INTR2_DP2_IVCRX, port);
	if (intr_2 != 0)
		hdmirx_wr_cor(RX_DEPACK_INTR2_DP2_IVCRX, intr_2, port);

	intr_3 = hdmirx_rd_cor(RX_DEPACK_INTR3_DP2_IVCRX, port);
	if (intr_3 != 0)
		hdmirx_wr_cor(RX_DEPACK_INTR3_DP2_IVCRX, intr_3, port);

	//no packet received more than 4 frames
	intr_5 = hdmirx_rd_cor(RX_DEPACK_INTR5_DP2_IVCRX, port);
	if (intr_5 != 0)
		hdmirx_wr_cor(RX_DEPACK_INTR5_DP2_IVCRX, intr_5, port);

	intr_6 = hdmirx_rd_cor(RX_DEPACK_INTR6_DP2_IVCRX, port);
	if (intr_6 != 0)
		hdmirx_wr_cor(RX_DEPACK_INTR6_DP2_IVCRX, intr_6, port);

	rx_intr_1 = hdmirx_rd_cor(RX_INTR1_PWD_IVCRX, port);
	if (rx_intr_1 != 0)
		hdmirx_wr_cor(RX_INTR1_PWD_IVCRX, rx_intr_1, port);

	rx_intr_4 = hdmirx_rd_cor(RX_INTR4_PWD_IVCRX, port);
	if (rx_intr_4 != 0) {
		//aud fifo
		hdmirx_wr_cor(RX_INTR4_PWD_IVCRX, rx_intr_4, port);
	}

	rx_intr_13 = hdmirx_rd_cor(RX_INTR13_PWD_IVCRX, port);
	if (rx_intr_13 != 0)
		hdmirx_wr_cor(RX_INTR13_PWD_IVCRX, rx_intr_13, port);

	rx_depack2_intr0 = hdmirx_rd_cor(RX_DEPACK2_INTR0_DP0B_IVCRX, port);
	if (rx_depack2_intr0 != 0)
		hdmirx_wr_cor(RX_DEPACK2_INTR0_DP0B_IVCRX, rx_depack2_intr0, port);

	rx_depack2_intr2 = hdmirx_rd_cor(RX_DEPACK2_INTR2_DP0B_IVCRX, port);
	if (rx_depack2_intr2 != 0)
		hdmirx_wr_cor(RX_DEPACK2_INTR2_DP0B_IVCRX, rx_depack2_intr2, port);

	//hdcp1x
	rx_hdcp1x_intr0 = hdmirx_rd_cor(RX_HDCP1X_INTR0_HDCP1X_IVCRX, port);
	if (rx_hdcp1x_intr0 != 0)
		hdmirx_wr_cor(RX_HDCP1X_INTR0_HDCP1X_IVCRX, rx_hdcp1x_intr0, port);

	//hdcp2x
	rx_hdcp2x_intr0 = hdmirx_rd_cor(CP2PAX_INTR0_HDCP2X_IVCRX, port);
	if (rx_hdcp2x_intr0 != 0)
		hdmirx_wr_cor(CP2PAX_INTR0_HDCP2X_IVCRX, rx_hdcp2x_intr0, port);

	rx_hdcp2x_intr1 = hdmirx_rd_cor(CP2PAX_INTR1_HDCP2X_IVCRX, port);
	if (rx_hdcp2x_intr1 != 0)
		hdmirx_wr_cor(CP2PAX_INTR1_HDCP2X_IVCRX, rx_hdcp2x_intr1, port);
//-----------------------------irq on--------------------------------
	if (rx_hdcp2x_intr1) {
		if (log_level & IRQ_LOG)
			rx_pr("hdcp2-1%x\n", rx_hdcp2x_intr1);
		if (rx_get_bits(rx_hdcp2x_intr1, _BIT(1))) {
			rx_pr("type\n");
			rx[port].hdcp.stream_manage_rcvd = true;
		}
		if (rx_get_bits(rx_hdcp2x_intr1, _BIT(2))) {
			if (rx[port].hdcp.hdcp_version != HDCP_VER_22)
				skip_frame(skip_frame_cnt, port, "cor hdcp intr");
			rx[port].hdcp.hdcp_version = HDCP_VER_22;
			rx[port].tx_type = DEV_HDMI20;
			if (rx[port].hdcp.repeat && hdmirx_repeat_support())
				rx[port].hdcp.rpt_reauth_event =
				HDCP_VER_22 | HDCP_NEED_REQ_DS_AUTH;
			rx[port].hdcp.hdcp_source = true;
			rx_pr("22\n");
		}
	}
	if (rx_hdcp2x_intr0) {
		if (log_level & IRQ_LOG)
			rx_pr("hdcp2-2%x\n", rx_hdcp2x_intr0);
		if (rx_get_bits(rx_hdcp2x_intr0, _BIT(0))) {
			if (log_level & HDCP_IRQ_LOG)
				rx_pr("22 done");
			hdcp22_auth_sts = HDCP22_AUTH_STATE_SUCCESS;
		}
		if (rx_get_bits(rx_hdcp2x_intr0, _BIT(1))) {
			if (log_level & HDCP_IRQ_LOG)
				rx_pr("22-ng");
			hdcp22_auth_sts = HDCP22_AUTH_STATE_FAILED;
		}
		if (rx_get_bits(rx_hdcp2x_intr0, _BIT(3))) {
			if (log_level & HDCP_IRQ_LOG)
				rx_pr("hash-ng");
		}
	}
	if (rx_intr_13) {
		if (log_level & IRQ_LOG)
			rx_pr("hdcp2-3%x\n", rx_intr_13);
		if (rx_get_bits(rx_intr_13, _BIT(7))) {
			//auth decryption change
			if (log_level & HDCP_IRQ_LOG)
				rx_pr("22 chg\n");
		}
	}
	if (rx_intr_1) {
		if (log_level & IRQ_LOG)
			rx_pr("hdcp1-%x\n", rx_intr_1);
		if (rx_get_bits(rx_intr_1, _BIT(1))) {
			//askv
			rx[port].hdcp.hdcp_version = HDCP_VER_14;
			if (rx[port].hdcp.repeat && hdmirx_repeat_support())
				rx[port].hdcp.rpt_reauth_event =
				HDCP_VER_14 | HDCP_NEED_REQ_DS_AUTH;
			rx[port].hdcp.hdcp_source = true;
			rx_pr("14\n");
		}
		if (rx_get_bits(rx_intr_1, _BIT(0))) {
			if (rx[port].hdcp.repeat && hdmirx_repeat_support())
				rx_pr("auth done\n");
		}
	}
	if (rx_hdcp1x_intr0) {
		if (log_level & IRQ_LOG)
			rx_pr("hdcp1-0%x\n", rx_hdcp1x_intr0);
		if (rx_get_bits(rx_hdcp1x_intr0, _BIT(0))) {
			if (log_level & HDCP_IRQ_LOG)
				rx_pr("14 chg\n");
		}
	}
	if (rx_depack2_intr0) {
		if (log_level & IRQ_LOG)
			rx_pr("dp2-0-%x\n", rx_depack2_intr0);
		if (rx_get_bits(rx_depack2_intr0, _BIT(2))) {
			/* parse hdr in EMP pkt */
			if (log_level & PKT_IRQ_LOG)
				rx_pr("NEW HDR\n");
		}
		if (rx_get_bits(rx_depack2_intr0, _BIT(3))) {
			rx_emp_type[port] |= EMP_TYPE_VTEM;
			if (log_level & PKT_IRQ_LOG)
				rx_pr("VTEM\n");
		}
	}
	//default off
	if (intr_2) {
		if (log_level & IRQ_LOG)
			rx_pr("irq2-%x\n", intr_2);
		if (rx_get_bits(intr_2, INTR2_BIT1_SPD))
			rx_spd_type[port] = 1;
		if (rx_get_bits(intr_2, INTR2_BIT0_AVI))
			skip_frame(skip_frame_cnt, port, "avi-");
	}
	if (hdcp_2x_ecc_intr) {
		if (log_level & HDCP_IRQ_LOG)
			rx_pr("ecc_err-%x\n", hdcp_2x_ecc_intr);
		if (hdcp22_auth_sts == HDCP22_AUTH_STATE_SUCCESS)  {
			if (hdcp22_reauth_enable) {
				rx_hdcp_22_sent_reauth(port);
				if (rx[port].state >= FSM_SIG_STABLE)
					rx[port].state = FSM_SIG_WAIT_STABLE;
			}
		}
	}
	//new vsi
	//if (intr_3) {
		//if (log_level & IRQ_LOG)
			//rx_pr("irq3-%x\n", intr_3);
	//}
	if (rx_depack2_intr2) {
		if (log_level & IRQ_LOG)
			rx_pr("dp2-2-%x\n", rx_depack2_intr2);
		if (rx_get_bits(rx_depack2_intr2, _BIT(4)))
			rx_emp_type[port] |= EMP_TYPE_VSIF;
		if (rx_get_bits(rx_depack2_intr2, _BIT(2)))
			rx_emp_type[port] |= EMP_TYPE_HDR;
	}
	if (intr_6) {
		if (rx_get_bits(rx_depack2_intr2, _BIT(2)))
			rx_clr_gcp_avmute(port);
	}
//---------------------------irq off--------------------------------
	//if (vsif_type)
		//rx_pkt_handler(PKT_BUFF_SET_VSI);
	//if (drm_handle_flag)
		//rx_pkt_handler(PKT_BUFF_SET_DRM);
	//if (emp_handle_flag)
		//rx_pkt_handler(PKT_BUFF_SET_EMP);
	//if (rx[rx_info.main_port].irq_flag)
		//tasklet_schedule(&rx_tasklet);

	//if (irq_need_clr)
		//error = 1;
	return error;
}

static int rx_cor2_irq_handler(void)
{
	int error = 0;
	u8 intr_2 = 0;
	u8 intr_3 = 0;
	u8 intr_5 = 0;
	u8 intr_6 = 0;
	u8 rx_intr_1 = 0;
	u8 rx_intr_4 = 0;
	u8 rx_intr_13 = 0;
	u8 rx_depack2_intr0;
	u8 rx_depack2_intr2;
	u8 rx_hdcp1x_intr0;
	u8 rx_hdcp2x_intr0;
	u8 rx_hdcp2x_intr1;
	u8 hdcp_2x_ecc_intr;
	u8 port = E_PORT2;

	hdcp_2x_ecc_intr = hdmirx_rd_cor(HDCP2X_RX_ECC_INTR, port);
	if (hdcp_2x_ecc_intr != 0)
		hdmirx_wr_cor(HDCP2X_RX_ECC_INTR, hdcp_2x_ecc_intr, port);

	intr_2 = hdmirx_rd_cor(RX_DEPACK_INTR2_DP2_IVCRX, port);
	if (intr_2 != 0)
		hdmirx_wr_cor(RX_DEPACK_INTR2_DP2_IVCRX, intr_2, port);

	intr_3 = hdmirx_rd_cor(RX_DEPACK_INTR3_DP2_IVCRX, port);
	if (intr_3 != 0)
		hdmirx_wr_cor(RX_DEPACK_INTR3_DP2_IVCRX, intr_3, port);

	//no packet received more than 4 frames
	intr_5 = hdmirx_rd_cor(RX_DEPACK_INTR5_DP2_IVCRX, port);
	if (intr_5 != 0)
		hdmirx_wr_cor(RX_DEPACK_INTR5_DP2_IVCRX, intr_5, port);

	intr_6 = hdmirx_rd_cor(RX_DEPACK_INTR6_DP2_IVCRX, port);
	if (intr_6 != 0)
		hdmirx_wr_cor(RX_DEPACK_INTR6_DP2_IVCRX, intr_6, port);

	rx_intr_1 = hdmirx_rd_cor(RX_INTR1_PWD_IVCRX, port);
	if (rx_intr_1 != 0)
		hdmirx_wr_cor(RX_INTR1_PWD_IVCRX, rx_intr_1, port);

	rx_intr_4 = hdmirx_rd_cor(RX_INTR4_PWD_IVCRX, port);
	if (rx_intr_4 != 0) {
		//aud fifo
		hdmirx_wr_cor(RX_INTR4_PWD_IVCRX, rx_intr_4, port);
	}

	rx_intr_13 = hdmirx_rd_cor(RX_INTR13_PWD_IVCRX, port);
	if (rx_intr_13 != 0)
		hdmirx_wr_cor(RX_INTR13_PWD_IVCRX, rx_intr_13, port);

	rx_depack2_intr0 = hdmirx_rd_cor(RX_DEPACK2_INTR0_DP0B_IVCRX, port);
	if (rx_depack2_intr0 != 0)
		hdmirx_wr_cor(RX_DEPACK2_INTR0_DP0B_IVCRX, rx_depack2_intr0, port);

	rx_depack2_intr2 = hdmirx_rd_cor(RX_DEPACK2_INTR2_DP0B_IVCRX, port);
	if (rx_depack2_intr2 != 0)
		hdmirx_wr_cor(RX_DEPACK2_INTR2_DP0B_IVCRX, rx_depack2_intr2, port);

	//hdcp1x
	rx_hdcp1x_intr0 = hdmirx_rd_cor(RX_HDCP1X_INTR0_HDCP1X_IVCRX, port);
	if (rx_hdcp1x_intr0 != 0)
		hdmirx_wr_cor(RX_HDCP1X_INTR0_HDCP1X_IVCRX, rx_hdcp1x_intr0, port);

	//hdcp2x
	rx_hdcp2x_intr0 = hdmirx_rd_cor(CP2PAX_INTR0_HDCP2X_IVCRX, port);
	if (rx_hdcp2x_intr0 != 0)
		hdmirx_wr_cor(CP2PAX_INTR0_HDCP2X_IVCRX, rx_hdcp2x_intr0, port);

	rx_hdcp2x_intr1 = hdmirx_rd_cor(CP2PAX_INTR1_HDCP2X_IVCRX, port);
	if (rx_hdcp2x_intr1 != 0)
		hdmirx_wr_cor(CP2PAX_INTR1_HDCP2X_IVCRX, rx_hdcp2x_intr1, port);
//-----------------------------irq on--------------------------------
	if (rx_hdcp2x_intr1) {
		if (log_level & IRQ_LOG)
			rx_pr("hdcp2-1%x\n", rx_hdcp2x_intr1);
		if (rx_get_bits(rx_hdcp2x_intr1, _BIT(1))) {
			rx_pr("type\n");
			rx[port].hdcp.stream_manage_rcvd = true;
		}
		if (rx_get_bits(rx_hdcp2x_intr1, _BIT(2))) {
			if (rx[port].hdcp.hdcp_version != HDCP_VER_22)
				skip_frame(skip_frame_cnt, port, "cor hdcp intr");
			rx[port].hdcp.hdcp_version = HDCP_VER_22;
			rx[port].tx_type = DEV_HDMI20;
			if (rx[port].hdcp.repeat && hdmirx_repeat_support())
				rx[port].hdcp.rpt_reauth_event =
				HDCP_VER_22 | HDCP_NEED_REQ_DS_AUTH;
			rx[port].hdcp.hdcp_source = true;
			rx_pr("22\n");
		}
	}
	if (rx_hdcp2x_intr0) {
		if (log_level & IRQ_LOG)
			rx_pr("hdcp2-2%x\n", rx_hdcp2x_intr0);
		if (rx_get_bits(rx_hdcp2x_intr0, _BIT(0))) {
			if (log_level & HDCP_IRQ_LOG)
				rx_pr("22 done");
			hdcp22_auth_sts = HDCP22_AUTH_STATE_SUCCESS;
		}
		if (rx_get_bits(rx_hdcp2x_intr0, _BIT(1))) {
			if (log_level & HDCP_IRQ_LOG)
				rx_pr("22-ng");
			hdcp22_auth_sts = HDCP22_AUTH_STATE_FAILED;
		}
		if (rx_get_bits(rx_hdcp2x_intr0, _BIT(3))) {
			if (log_level & HDCP_IRQ_LOG)
				rx_pr("hash-ng");
		}
	}
	if (rx_intr_13) {
		if (log_level & IRQ_LOG)
			rx_pr("hdcp2-3%x\n", rx_intr_13);
		if (rx_get_bits(rx_intr_13, _BIT(7))) {
			//auth decryption change
			if (log_level & HDCP_IRQ_LOG)
				rx_pr("22 chg\n");
		}
	}
	if (rx_intr_1) {
		if (log_level & IRQ_LOG)
			rx_pr("hdcp1-%x\n", rx_intr_1);
		if (rx_get_bits(rx_intr_1, _BIT(1))) {
			//askv
			rx[port].hdcp.hdcp_version = HDCP_VER_14;
			if (rx[port].hdcp.repeat && hdmirx_repeat_support())
				rx[port].hdcp.rpt_reauth_event =
				HDCP_VER_14 | HDCP_NEED_REQ_DS_AUTH;
			rx[port].hdcp.hdcp_source = true;
			rx_pr("14\n");
		}
		if (rx_get_bits(rx_intr_1, _BIT(0))) {
			if (rx[port].hdcp.repeat && hdmirx_repeat_support())
				rx_pr("auth done\n");
		}
	}
	if (rx_hdcp1x_intr0) {
		if (log_level & IRQ_LOG)
			rx_pr("hdcp1-0%x\n", rx_hdcp1x_intr0);
		if (rx_get_bits(rx_hdcp1x_intr0, _BIT(0))) {
			if (log_level & HDCP_IRQ_LOG)
				rx_pr("14 chg\n");
		}
	}
	if (rx_depack2_intr0) {
		if (log_level & IRQ_LOG)
			rx_pr("dp2-0-%x\n", rx_depack2_intr0);
		if (rx_get_bits(rx_depack2_intr0, _BIT(2))) {
			/* parse hdr in EMP pkt */
			if (log_level & PKT_IRQ_LOG)
				rx_pr("NEW HDR\n");
		}
		if (rx_get_bits(rx_depack2_intr0, _BIT(3))) {
			rx_emp_type[port] |= EMP_TYPE_VTEM;
			if (log_level & PKT_IRQ_LOG)
				rx_pr("VTEM\n");
		}
	}
	//default off
	if (intr_2) {
		if (log_level & IRQ_LOG)
			rx_pr("irq2-%x\n", intr_2);
		if (rx_get_bits(intr_2, INTR2_BIT1_SPD))
			rx_spd_type[port] = 1;
		if (rx_get_bits(intr_2, INTR2_BIT0_AVI))
			skip_frame(skip_frame_cnt, port, "avi-");
	}
	if (hdcp_2x_ecc_intr) {
		if (log_level & HDCP_IRQ_LOG)
			rx_pr("ecc_err-%x\n", hdcp_2x_ecc_intr);
		if (hdcp22_auth_sts == HDCP22_AUTH_STATE_SUCCESS)  {
			if (hdcp22_reauth_enable) {
				rx_hdcp_22_sent_reauth(port);
				if (rx[port].state >= FSM_SIG_STABLE)
					rx[port].state = FSM_SIG_WAIT_STABLE;
			}
		}
	}
	//new vsi
	//if (intr_3) {
		//if (log_level & IRQ_LOG)
			//rx_pr("irq3-%x\n", intr_3);
	//}
	if (rx_depack2_intr2) {
		if (log_level & IRQ_LOG)
			rx_pr("dp2-2-%x\n", rx_depack2_intr2);
		if (rx_get_bits(rx_depack2_intr2, _BIT(4)))
			rx_emp_type[port] |= EMP_TYPE_VSIF;
		if (rx_get_bits(rx_depack2_intr2, _BIT(2)))
			rx_emp_type[port] |= EMP_TYPE_HDR;
	}
	if (intr_6) {
		if (rx_get_bits(rx_depack2_intr2, _BIT(2)))
			rx_clr_gcp_avmute(port);
	}
//---------------------------irq off--------------------------------
	//if (vsif_type)
		//rx_pkt_handler(PKT_BUFF_SET_VSI);
	//if (drm_handle_flag)
		//rx_pkt_handler(PKT_BUFF_SET_DRM);
	//if (emp_handle_flag)
		//rx_pkt_handler(PKT_BUFF_SET_EMP);
	//if (rx[rx_info.main_port].irq_flag)
		//tasklet_schedule(&rx_tasklet);

	//if (irq_need_clr)
		//error = 1;
	return error;
}

static int rx_cor1_irq_handler(void)
{
	int error = 0;
	u8 intr_2 = 0;
	u8 intr_3 = 0;
	u8 intr_5 = 0;
	u8 intr_6 = 0;
	u8 rx_intr_1 = 0;
	u8 rx_intr_4 = 0;
	u8 rx_intr_13 = 0;
	u8 rx_depack2_intr0;
	u8 rx_depack2_intr2;
	u8 rx_hdcp1x_intr0;
	u8 rx_hdcp2x_intr0;
	u8 rx_hdcp2x_intr1;
	u8 hdcp_2x_ecc_intr;
	u8 port = E_PORT1;

	hdcp_2x_ecc_intr = hdmirx_rd_cor(HDCP2X_RX_ECC_INTR, port);
	if (hdcp_2x_ecc_intr != 0)
		hdmirx_wr_cor(HDCP2X_RX_ECC_INTR, hdcp_2x_ecc_intr, port);

	intr_2 = hdmirx_rd_cor(RX_DEPACK_INTR2_DP2_IVCRX, port);
	if (intr_2 != 0)
		hdmirx_wr_cor(RX_DEPACK_INTR2_DP2_IVCRX, intr_2, port);

	intr_3 = hdmirx_rd_cor(RX_DEPACK_INTR3_DP2_IVCRX, port);
	if (intr_3 != 0)
		hdmirx_wr_cor(RX_DEPACK_INTR3_DP2_IVCRX, intr_3, port);

	//no packet received more than 4 frames
	intr_5 = hdmirx_rd_cor(RX_DEPACK_INTR5_DP2_IVCRX, port);
	if (intr_5 != 0)
		hdmirx_wr_cor(RX_DEPACK_INTR5_DP2_IVCRX, intr_5, port);

	intr_6 = hdmirx_rd_cor(RX_DEPACK_INTR6_DP2_IVCRX, port);
	if (intr_6 != 0)
		hdmirx_wr_cor(RX_DEPACK_INTR6_DP2_IVCRX, intr_6, port);

	rx_intr_1 = hdmirx_rd_cor(RX_INTR1_PWD_IVCRX, port);
	if (rx_intr_1 != 0)
		hdmirx_wr_cor(RX_INTR1_PWD_IVCRX, rx_intr_1, port);

	rx_intr_4 = hdmirx_rd_cor(RX_INTR4_PWD_IVCRX, port);
	if (rx_intr_4 != 0) {
		//aud fifo
		hdmirx_wr_cor(RX_INTR4_PWD_IVCRX, rx_intr_4, port);
	}

	rx_intr_13 = hdmirx_rd_cor(RX_INTR13_PWD_IVCRX, port);
	if (rx_intr_13 != 0)
		hdmirx_wr_cor(RX_INTR13_PWD_IVCRX, rx_intr_13, port);

	rx_depack2_intr0 = hdmirx_rd_cor(RX_DEPACK2_INTR0_DP0B_IVCRX, port);
	if (rx_depack2_intr0 != 0)
		hdmirx_wr_cor(RX_DEPACK2_INTR0_DP0B_IVCRX, rx_depack2_intr0, port);

	rx_depack2_intr2 = hdmirx_rd_cor(RX_DEPACK2_INTR2_DP0B_IVCRX, port);
	if (rx_depack2_intr2 != 0)
		hdmirx_wr_cor(RX_DEPACK2_INTR2_DP0B_IVCRX, rx_depack2_intr2, port);

	//hdcp1x
	rx_hdcp1x_intr0 = hdmirx_rd_cor(RX_HDCP1X_INTR0_HDCP1X_IVCRX, port);
	if (rx_hdcp1x_intr0 != 0)
		hdmirx_wr_cor(RX_HDCP1X_INTR0_HDCP1X_IVCRX, rx_hdcp1x_intr0, port);

	//hdcp2x
	rx_hdcp2x_intr0 = hdmirx_rd_cor(CP2PAX_INTR0_HDCP2X_IVCRX, port);
	if (rx_hdcp2x_intr0 != 0)
		hdmirx_wr_cor(CP2PAX_INTR0_HDCP2X_IVCRX, rx_hdcp2x_intr0, port);

	rx_hdcp2x_intr1 = hdmirx_rd_cor(CP2PAX_INTR1_HDCP2X_IVCRX, port);
	if (rx_hdcp2x_intr1 != 0)
		hdmirx_wr_cor(CP2PAX_INTR1_HDCP2X_IVCRX, rx_hdcp2x_intr1, port);
//-----------------------------irq on--------------------------------
	if (rx_hdcp2x_intr1) {
		if (log_level & IRQ_LOG)
			rx_pr("hdcp2-1%x\n", rx_hdcp2x_intr1);
		if (rx_get_bits(rx_hdcp2x_intr1, _BIT(1))) {
			rx_pr("type\n");
			rx[port].hdcp.stream_manage_rcvd = true;
		}
		if (rx_get_bits(rx_hdcp2x_intr1, _BIT(2))) {
			if (rx[port].hdcp.hdcp_version != HDCP_VER_22)
				skip_frame(skip_frame_cnt, port, "cor hdcp intr");
			rx[port].hdcp.hdcp_version = HDCP_VER_22;
			rx[port].tx_type = DEV_HDMI20;
			if (rx[port].hdcp.repeat && hdmirx_repeat_support())
				rx[port].hdcp.rpt_reauth_event =
				HDCP_VER_22 | HDCP_NEED_REQ_DS_AUTH;
			rx[port].hdcp.hdcp_source = true;
			rx_pr("22\n");
		}
	}
	if (rx_hdcp2x_intr0) {
		if (log_level & IRQ_LOG)
			rx_pr("hdcp2-2%x\n", rx_hdcp2x_intr0);
		if (rx_get_bits(rx_hdcp2x_intr0, _BIT(0))) {
			if (log_level & HDCP_IRQ_LOG)
				rx_pr("22 done");
			hdcp22_auth_sts = HDCP22_AUTH_STATE_SUCCESS;
		}
		if (rx_get_bits(rx_hdcp2x_intr0, _BIT(1))) {
			if (log_level & HDCP_IRQ_LOG)
				rx_pr("22-ng");
			hdcp22_auth_sts = HDCP22_AUTH_STATE_FAILED;
		}
		if (rx_get_bits(rx_hdcp2x_intr0, _BIT(3))) {
			if (log_level & HDCP_IRQ_LOG)
				rx_pr("hash-ng");
		}
	}
	if (rx_intr_13) {
		if (log_level & IRQ_LOG)
			rx_pr("hdcp2-3%x\n", rx_intr_13);
		if (rx_get_bits(rx_intr_13, _BIT(7))) {
			//auth decryption change
			if (log_level & HDCP_IRQ_LOG)
				rx_pr("22 chg\n");
		}
	}
	if (rx_intr_1) {
		if (log_level & IRQ_LOG)
			rx_pr("hdcp1-%x\n", rx_intr_1);
		if (rx_get_bits(rx_intr_1, _BIT(1))) {
			//askv
			rx[port].hdcp.hdcp_version = HDCP_VER_14;
			if (rx[port].hdcp.repeat && hdmirx_repeat_support())
				rx[port].hdcp.rpt_reauth_event =
				HDCP_VER_14 | HDCP_NEED_REQ_DS_AUTH;
			rx[port].hdcp.hdcp_source = true;
			rx_pr("14\n");
		}
		if (rx_get_bits(rx_intr_1, _BIT(0))) {
			if (rx[port].hdcp.repeat && hdmirx_repeat_support())
				rx_pr("auth done\n");
		}
	}
	if (rx_hdcp1x_intr0) {
		if (log_level & IRQ_LOG)
			rx_pr("hdcp1-0%x\n", rx_hdcp1x_intr0);
		if (rx_get_bits(rx_hdcp1x_intr0, _BIT(0))) {
			if (log_level & HDCP_IRQ_LOG)
				rx_pr("14 chg\n");
		}
	}
	if (rx_depack2_intr0) {
		if (log_level & IRQ_LOG)
			rx_pr("dp2-0-%x\n", rx_depack2_intr0);
		if (rx_get_bits(rx_depack2_intr0, _BIT(2))) {
			/* parse hdr in EMP pkt */
			if (log_level & PKT_IRQ_LOG)
				rx_pr("NEW HDR\n");
		}
		if (rx_get_bits(rx_depack2_intr0, _BIT(3))) {
			rx_emp_type[port] |= EMP_TYPE_VTEM;
			if (log_level & PKT_IRQ_LOG)
				rx_pr("VTEM\n");
		}
	}
	//default off
	if (intr_2) {
		if (log_level & IRQ_LOG)
			rx_pr("irq2-%x\n", intr_2);
		if (rx_get_bits(intr_2, INTR2_BIT1_SPD))
			rx_spd_type[port] = 1;
		if (rx_get_bits(intr_2, INTR2_BIT0_AVI))
			skip_frame(skip_frame_cnt, port, "avi-");
	}
	if (hdcp_2x_ecc_intr) {
		if (log_level & HDCP_IRQ_LOG)
			rx_pr("ecc_err-%x\n", hdcp_2x_ecc_intr);
		if (hdcp22_auth_sts == HDCP22_AUTH_STATE_SUCCESS)  {
			if (hdcp22_reauth_enable) {
				rx_hdcp_22_sent_reauth(port);
				if (rx[port].state >= FSM_SIG_STABLE)
					rx[port].state = FSM_SIG_WAIT_STABLE;
			}
		}
	}
	//new vsi
	//if (intr_3) {
		//if (log_level & IRQ_LOG)
			//rx_pr("irq3-%x\n", intr_3);
	//}
	if (rx_depack2_intr2) {
		if (log_level & IRQ_LOG)
			rx_pr("dp2-2-%x\n", rx_depack2_intr2);
		if (rx_get_bits(rx_depack2_intr2, _BIT(4)))
			rx_emp_type[port] |= EMP_TYPE_VSIF;
		if (rx_get_bits(rx_depack2_intr2, _BIT(2)))
			rx_emp_type[port] |= EMP_TYPE_HDR;
	}
	if (intr_6) {
		if (rx_get_bits(rx_depack2_intr2, _BIT(2)))
			rx_clr_gcp_avmute(port);
	}
//---------------------------irq off--------------------------------
	//if (vsif_type)
		//rx_pkt_handler(PKT_BUFF_SET_VSI);
	//if (drm_handle_flag)
		//rx_pkt_handler(PKT_BUFF_SET_DRM);
	//if (emp_handle_flag)
		//rx_pkt_handler(PKT_BUFF_SET_EMP);
	//if (rx[rx_info.main_port].irq_flag)
		//tasklet_schedule(&rx_tasklet);

	//if (irq_need_clr)
		//error = 1;
	return error;
}

static int rx_cor_irq_handler(void)
{
	int error = 0;
	u8 intr_2 = 0;
	u8 intr_3 = 0;
	u8 intr_5 = 0;
	u8 intr_6 = 0;
	u8 rx_intr_1 = 0;
	u8 rx_intr_4 = 0;
	u8 rx_intr_13 = 0;
	u8 rx_depack2_intr0;
	u8 rx_depack2_intr2;
	u8 rx_hdcp1x_intr0;
	u8 rx_hdcp2x_intr0;
	u8 rx_hdcp2x_intr1;
	u8 hdcp_2x_ecc_intr;
	u8 port = (rx_info.chip_id >= CHIP_ID_T3X) ? E_PORT0 : rx_info.main_port;

	/* clear interrupt quickly */
	hdcp_2x_ecc_intr = hdmirx_rd_cor(HDCP2X_RX_ECC_INTR, port);
	if (hdcp_2x_ecc_intr != 0)
		hdmirx_wr_cor(HDCP2X_RX_ECC_INTR, hdcp_2x_ecc_intr, port);

	intr_2 = hdmirx_rd_cor(RX_DEPACK_INTR2_DP2_IVCRX, port);
	if (intr_2 != 0)
		hdmirx_wr_cor(RX_DEPACK_INTR2_DP2_IVCRX, intr_2, port);

	intr_3 = hdmirx_rd_cor(RX_DEPACK_INTR3_DP2_IVCRX, port);
	if (intr_3 != 0)
		hdmirx_wr_cor(RX_DEPACK_INTR3_DP2_IVCRX, intr_3, port);

	//no packet received more than 4 frames
	intr_5 = hdmirx_rd_cor(RX_DEPACK_INTR5_DP2_IVCRX, port);
	if (intr_5 != 0)
		hdmirx_wr_cor(RX_DEPACK_INTR5_DP2_IVCRX, intr_5, port);

	intr_6 = hdmirx_rd_cor(RX_DEPACK_INTR6_DP2_IVCRX, port);
	if (intr_6 != 0)
		hdmirx_wr_cor(RX_DEPACK_INTR6_DP2_IVCRX, intr_6, port);

	rx_intr_1 = hdmirx_rd_cor(RX_INTR1_PWD_IVCRX, port);
	if (rx_intr_1 != 0)
		hdmirx_wr_cor(RX_INTR1_PWD_IVCRX, rx_intr_1, port);

	rx_intr_4 = hdmirx_rd_cor(RX_INTR4_PWD_IVCRX, port);
	if (rx_intr_4 != 0) {
		//aud fifo
		hdmirx_wr_cor(RX_INTR4_PWD_IVCRX, rx_intr_4, port);
	}

	rx_intr_13 = hdmirx_rd_cor(RX_INTR13_PWD_IVCRX, port);
	if (rx_intr_13 != 0)
		hdmirx_wr_cor(RX_INTR13_PWD_IVCRX, rx_intr_13, port);

	rx_depack2_intr0 = hdmirx_rd_cor(RX_DEPACK2_INTR0_DP0B_IVCRX, port);
	if (rx_depack2_intr0 != 0)
		hdmirx_wr_cor(RX_DEPACK2_INTR0_DP0B_IVCRX, rx_depack2_intr0, port);

	rx_depack2_intr2 = hdmirx_rd_cor(RX_DEPACK2_INTR2_DP0B_IVCRX, port);
	if (rx_depack2_intr2 != 0)
		hdmirx_wr_cor(RX_DEPACK2_INTR2_DP0B_IVCRX, rx_depack2_intr2, port);

	//hdcp1x
	rx_hdcp1x_intr0 = hdmirx_rd_cor(RX_HDCP1X_INTR0_HDCP1X_IVCRX, port);
	if (rx_hdcp1x_intr0 != 0)
		hdmirx_wr_cor(RX_HDCP1X_INTR0_HDCP1X_IVCRX, rx_hdcp1x_intr0, port);

	//hdcp2x
	rx_hdcp2x_intr0 = hdmirx_rd_cor(CP2PAX_INTR0_HDCP2X_IVCRX, port);
	if (rx_hdcp2x_intr0 != 0)
		hdmirx_wr_cor(CP2PAX_INTR0_HDCP2X_IVCRX, rx_hdcp2x_intr0, port);

	rx_hdcp2x_intr1 = hdmirx_rd_cor(CP2PAX_INTR1_HDCP2X_IVCRX, port);
	if (rx_hdcp2x_intr1 != 0)
		hdmirx_wr_cor(CP2PAX_INTR1_HDCP2X_IVCRX, rx_hdcp2x_intr1, port);
//-----------------------------irq on--------------------------------
	if (rx_hdcp2x_intr1) {
		if (log_level & IRQ_LOG)
			rx_pr("hdcp2-1%x\n", rx_hdcp2x_intr1);
		if (rx_get_bits(rx_hdcp2x_intr1, _BIT(1))) {
			rx_pr("type\n");
			rx[port].hdcp.stream_manage_rcvd = true;
		}
		if (rx_get_bits(rx_hdcp2x_intr1, _BIT(2))) {
			if (rx[port].hdcp.hdcp_version != HDCP_VER_22)
				skip_frame(skip_frame_cnt, port, "cor hdcp intr");
			rx[port].hdcp.hdcp_version = HDCP_VER_22;
			rx[port].tx_type = DEV_HDMI20;
			if (rx[port].hdcp.repeat && hdmirx_repeat_support())
				rx[port].hdcp.rpt_reauth_event =
				HDCP_VER_22 | HDCP_NEED_REQ_DS_AUTH;
			rx[port].hdcp.hdcp_source = true;
			rx_pr("22\n");
		}
	}
	if (rx_hdcp2x_intr0) {
		if (log_level & IRQ_LOG)
			rx_pr("hdcp2-2%x\n", rx_hdcp2x_intr0);
		if (rx_get_bits(rx_hdcp2x_intr0, _BIT(0))) {
			if (log_level & HDCP_IRQ_LOG)
				rx_pr("22 done");
			hdcp22_auth_sts = HDCP22_AUTH_STATE_SUCCESS;
		}
		if (rx_get_bits(rx_hdcp2x_intr0, _BIT(1))) {
			if (log_level & HDCP_IRQ_LOG)
				rx_pr("22-ng");
			hdcp22_auth_sts = HDCP22_AUTH_STATE_FAILED;
		}
		if (rx_get_bits(rx_hdcp2x_intr0, _BIT(3))) {
			if (log_level & HDCP_IRQ_LOG)
				rx_pr("hash-ng");
		}
	}
	if (rx_intr_13) {
		if (log_level & IRQ_LOG)
			rx_pr("hdcp2-3%x\n", rx_intr_13);
		if (rx_get_bits(rx_intr_13, _BIT(7))) {
			//auth decryption change
			if (log_level & HDCP_IRQ_LOG)
				rx_pr("22 chg\n");
		}
	}
	if (rx_intr_1) {
		if (log_level & IRQ_LOG)
			rx_pr("hdcp1-%x\n", rx_intr_1);
		if (rx_get_bits(rx_intr_1, _BIT(1))) {
			//askv
			rx[port].hdcp.hdcp_version = HDCP_VER_14;
			if (rx[port].hdcp.repeat && hdmirx_repeat_support())
				rx[port].hdcp.rpt_reauth_event =
				HDCP_VER_14 | HDCP_NEED_REQ_DS_AUTH;
			rx[port].hdcp.hdcp_source = true;
			rx_pr("14\n");
		}
		if (rx_get_bits(rx_intr_1, _BIT(0))) {
			if (rx[port].hdcp.repeat && hdmirx_repeat_support())
				rx_pr("auth done\n");
		}
	}
	if (rx_hdcp1x_intr0) {
		if (log_level & IRQ_LOG)
			rx_pr("hdcp1-0%x\n", rx_hdcp1x_intr0);
		if (rx_get_bits(rx_hdcp1x_intr0, _BIT(0))) {
			if (log_level & HDCP_IRQ_LOG)
				rx_pr("14 chg\n");
		}
	}
	if (rx_depack2_intr0) {
		if (log_level & IRQ_LOG)
			rx_pr("dp2-0-%x\n", rx_depack2_intr0);
		if (rx_get_bits(rx_depack2_intr0, _BIT(2))) {
			/* parse hdr in EMP pkt */
			if (log_level & PKT_IRQ_LOG)
				rx_pr("NEW HDR\n");
		}
		if (rx_get_bits(rx_depack2_intr0, _BIT(3))) {
			rx_emp_type[port] |= EMP_TYPE_VTEM;
			if (log_level & PKT_IRQ_LOG)
				rx_pr("VTEM\n");
		}
	}
	//default off
	if (intr_2) {
		if (log_level & PKT_IRQ_LOG)
			rx_pr("irq2-%x\n", intr_2);
		if (rx_get_bits(intr_2, INTR2_BIT1_SPD))
			rx_spd_type[port] = 1;
		if (rx_get_bits(intr_2, INTR2_BIT0_AVI))
			skip_frame(skip_frame_cnt, port, "avi-");
	}
	if (hdcp_2x_ecc_intr) {
		if (log_level & HDCP_IRQ_LOG)
			rx_pr("ecc_err-%x\n", hdcp_2x_ecc_intr);
		if (hdcp22_auth_sts == HDCP22_AUTH_STATE_SUCCESS)  {
			if (hdcp22_reauth_enable) {
				rx_hdcp_22_sent_reauth(port);
				if (rx[port].state >= FSM_SIG_STABLE)
					rx[port].state = FSM_SIG_WAIT_STABLE;
			}
		}
	}
	//new vsi
	//if (intr_3) {
		//if (log_level & IRQ_LOG)
			//rx_pr("irq3-%x\n", intr_3);
	//}
	if (rx_depack2_intr2) {
		if (log_level & IRQ_LOG)
			rx_pr("dp2-2-%x\n", rx_depack2_intr2);
		if (rx_get_bits(rx_depack2_intr2, _BIT(4)))
			rx_emp_type[port] |= EMP_TYPE_VSIF;
		if (rx_get_bits(rx_depack2_intr2, _BIT(2)))
			rx_emp_type[port] |= EMP_TYPE_HDR;
	}
	if (intr_6) {
		if (rx_get_bits(rx_depack2_intr2, _BIT(2)))
			rx_clr_gcp_avmute(port);
	}
//---------------------------irq off--------------------------------
	//if (vsif_type)
		//rx_pkt_handler(PKT_BUFF_SET_VSI);
	//if (drm_handle_flag)
		//rx_pkt_handler(PKT_BUFF_SET_DRM);
	//if (emp_handle_flag)
		//rx_pkt_handler(PKT_BUFF_SET_EMP);
	//if (rx[rx_info.main_port].irq_flag)
		//tasklet_schedule(&rx_tasklet);

	//if (irq_need_clr)
		//error = 1;
	return error;
}

irqreturn_t irq0_handler(int irq, void *params)
{
	unsigned long hdmirx_top_intr_stat;
	unsigned long long cur_clks, irq_duration;
	static unsigned long long last_clks;
	int error = 0;
	u8 emp_rcvd;
	u8 tmp = 0;
	bool need_check;
	u8 port = (rx_info.chip_id >= CHIP_ID_T3X) ? E_PORT0 : rx_info.main_port;

	if (params == 0) {
		rx_pr("%s: %s\n", __func__,
		      "RX IRQ invalid parameter");
		return IRQ_HANDLED;
	}

	cur_clks = sched_clock();
	irq_duration = cur_clks - last_clks;
	last_clks = cur_clks;

	if (irq_duration <= TIME_1MS)
		rx[port].irq_err_cnt++;
	else
		rx[port].irq_err_cnt = 0;
	if (rx[port].irq_err_cnt >= irq_err_max) {
		rx_pr("DE ERR\n");
		if (video_mute_enabled(port)) {
			rx_mute_vpp(rx_get_port_type(port));
			rx[port].vpp_mute = true;
			set_video_mute(HDMI_RX_MUTE_SET, true);
			rx_pr("vpp mute\n");
		}
		rx[port].irq_err_cnt = 0;
		rx_irq_en(0, port);
		hdmirx_output_en(false);
		if (rx[port].state > FSM_WAIT_CLK_STABLE)
			rx[port].state = FSM_WAIT_CLK_STABLE;
	}
reisr:hdmirx_top_intr_stat = hdmirx_rd_top(TOP_INTR_STAT, port);
	hdmirx_wr_top(TOP_INTR_STAT_CLR, hdmirx_top_intr_stat, port);

	if (rx_info.chip_id >= CHIP_ID_TL1 &&
	    rx_info.chip_id <= CHIP_ID_T5D) {
		tmp = hdmirx_top_intr_stat & 0x1;
	} else if (rx_info.chip_id >= CHIP_ID_T7) {
		tmp = hdmirx_top_intr_stat & 0x2;
	} else {
		tmp = hdmirx_top_intr_stat & (~(1 << 30));
	}
	if (tmp) {
		//if (rx_info.chip_id >= CHIP_ID_T3X) //todo
			///error = rx_cor0_handler();
		if (rx_info.chip_id >= CHIP_ID_T7)
			error = rx_cor_irq_handler();
		else
			error = rx_dwc_irq_handler();
		if (error < 0) {
			if (error != -EPERM) {
				rx_pr("%s: RX IRQ handler %d\n",
				      __func__, error);
			}
		}
	}
	/* modify interrupt flow for isr loading */
	/* top interrupt handler */
	if (rx_info.chip_id >= CHIP_ID_TL1) {//todo
		//check sqo
		if (rx_info.chip_id >= CHIP_ID_T3X) {//todo
			if (hdmirx_top_intr_stat & (1 << 20))
				need_check = true;
		} else if (rx_info.chip_id >= CHIP_ID_T7 &&
				   rx_info.chip_id <= CHIP_ID_TXHD2) {//todo
			if (hdmirx_top_intr_stat & (1 << 29))
				need_check = true;
		}
		if (need_check) {
			need_check = false;
			if (video_mute_enabled(port)) {
				rx[port].vpp_mute = true;
				rx_mute_vpp(rx_get_port_type(port));
				set_video_mute(HDMI_RX_MUTE_SET, true);
				rx[port].var.mute_cnt = 0;
				if (log_level & 0x100)
					rx_pr("vpp mute\n");
			}
			skip_frame(skip_frame_cnt, port, "irq0 sqofclk_fall");
			if (log_level & 0x100)
				rx_pr("[isr] sqofclk_fall\n");
		}
		if (hdmirx_top_intr_stat & (1 << 28)) {
			if (log_level & 0x100)
				rx_pr("[isr] sqofclk_rise\n");
		}
		if (rx_info.chip_id >= CHIP_ID_T3X) {//todo
			if (hdmirx_top_intr_stat & (1 << 21))
				need_check = true;
		} else if (rx_info.chip_id >= CHIP_ID_T7 &&
				   rx_info.chip_id <= CHIP_ID_TXHD2) {//todo
			if (hdmirx_top_intr_stat & (1 << 25))
				need_check = true;
		}
		if (need_check) {
			need_check = false;
			if (rx[port].state >= FSM_SIG_STABLE) {
				emp_rcvd = true;
				//rx_emp_field_done_irq(port);
				//rx_pkt_handler(PKT_BUFF_SET_EMP, port);
			}
			if (log_level & 0x400)
				rx_pr("[isr] emp_field_done\n");
		}
		if (rx_info.chip_id >= CHIP_ID_T3X) {//todo
			if (hdmirx_top_intr_stat & (1 << 23))
				need_check = true;
		} else if (rx_info.chip_id >= CHIP_ID_T7 &&
				   rx_info.chip_id <= CHIP_ID_TXHD2) {//todo
			if (hdmirx_top_intr_stat & (1 << 27))
				need_check = true;
		}
		if (need_check) {
			rx_pkt_initial(port);
			need_check = false;
			if (rx[port].state >= FSM_SIG_STABLE) {
				rx[port].vsync_cnt++;
				if (rx_spd_type[port]) {
					rx_pkt_handler(PKT_BUFF_SET_SPD, port);
					rx_spd_type[port] = 0;
				}
				if (rx_emp_type[port] & EMP_TYPE_VTEM) {
					rx[port].vrr_en = true;
					rx_emp_type[port] &= (~EMP_TYPE_VTEM);
				} else {
					rx[port].vrr_en = false;
				}
				if (emp_rcvd) {
					rx_emp_field_done_irq(port);
					rx_pkt_handler(PKT_BUFF_SET_EMP, port);
					rx_update_sig_info(port);
					emp_rcvd = false;
				}
				tvin_update_vdin_prop(rx_get_port_type(port));
			}
			if (log_level & 0x400)
				rx_pr("[isr] DE rise.\n");
		}
		if (hdmirx_top_intr_stat & (1 << 26)) {
			rx_emp_lastpkt_done_irq(port);
			if (log_level & 0x400)
				rx_pr("[isr] last_emp_done\n");
		}
		if (hdmirx_top_intr_stat & (1 << 24))
			if (log_level & 0x100)
				rx_pr("[isr] tmds_align_stable_chg\n");
		//if (hdmirx_top_intr_stat & (1 << 23))
			//if (log_level & 0x100)
				//rx_pr("[isr] meter_stable_chg_cable\n");
	}
	if (hdmirx_top_intr_stat & (1 << 13))
		rx_pr("[isr] auth rise\n");
	if (hdmirx_top_intr_stat & (1 << 14))
		rx_pr("[isr] auth fall\n");
	if (hdmirx_top_intr_stat & (1 << 15))
		rx_pr("[isr] enc rise\n");
	if (hdmirx_top_intr_stat & (1 << 16))
		rx_pr("[isr] enc fall\n");
	if (hdmirx_top_intr_stat & (1 << 3))
		rx_pr("[isr] 5v rise\n");
	if (hdmirx_top_intr_stat & (1 << 4))
		rx_pr("[isr] 5v fall\n");
	if (rx_info.chip_id >= CHIP_ID_T7) {
		if (hdmirx_top_intr_stat & (1 << 2))
			if (log_level & COR_LOG)
				rx_pr("[isr] phy dig\n");
		if (hdmirx_top_intr_stat & (1 << 1))
			if (log_level & COR_LOG)
				rx_pr("[isr] ctrl pwd\n");
		if (hdmirx_top_intr_stat & (1 << 0))
			if (log_level & COR_LOG)
				rx_pr("[isr] ctrl aon\n");
	}
	if (rx_info.chip_id < CHIP_ID_TL1) {
		if (error == 1)
			goto reisr;
	} else if (rx_info.chip_id >= CHIP_ID_T7) {
		//nothing
	} else {
		hdmirx_top_intr_stat = hdmirx_rd_top(TOP_INTR_STAT, port);
		hdmirx_top_intr_stat &= 0x1;
		if (hdmirx_top_intr_stat) {
			if (log_level & IRQ_LOG)
				rx_pr("\n irq_miss");
			goto reisr;
		}
	}
	return IRQ_HANDLED;
}

irqreturn_t irq1_handler(int irq, void *params)
{
	unsigned long hdmirx_top_intr_stat;
	unsigned long long cur_clks, irq_duration;
	static unsigned long long last_clks;
	int error = 0;
	u8 emp_rcvd;
	u8 tmp = 0;

	if (params == 0) {
		rx_pr("%s: %s\n", __func__,
		      "RX IRQ invalid parameter");
		return IRQ_HANDLED;
	}

	cur_clks = sched_clock();
	irq_duration = cur_clks - last_clks;
	last_clks = cur_clks;

	if (irq_duration <= TIME_1MS)
		rx[E_PORT1].irq_err_cnt++;
	else
		rx[E_PORT1].irq_err_cnt = 0;
	if (rx[E_PORT1].irq_err_cnt >= irq_err_max) {
		rx_pr("DE ERR\n");
		if (video_mute_enabled(E_PORT1)) {
			rx_mute_vpp(rx_get_port_type(E_PORT1));
			set_video_mute(HDMI_RX_MUTE_SET, true);
			rx_pr("vpp mute\n");
		}
		rx[E_PORT1].irq_err_cnt = 0;
		rx_irq_en(0, E_PORT1);
		hdmirx_output_en(false);
		if (rx[E_PORT1].state > FSM_WAIT_CLK_STABLE)
			rx[E_PORT1].state = FSM_WAIT_CLK_STABLE;
	}
	hdmirx_top_intr_stat = hdmirx_rd_top(TOP_INTR_STAT, E_PORT1);
	hdmirx_wr_top(TOP_INTR_STAT_CLR, hdmirx_top_intr_stat, E_PORT1);

	tmp = hdmirx_top_intr_stat & 0x2;
	if (tmp) {
		error = rx_cor1_irq_handler();
		if (error < 0) {
			if (error != -EPERM)
				rx_pr("cor1 irq %d\n", error);
		}
	}
	/* modify interrupt flow for isr loading */
	/* top interrupt handler */
	if (hdmirx_top_intr_stat & (1 << 20)) {
		if (video_mute_enabled(E_PORT1)) {
			rx[E_PORT1].vpp_mute = true;
			set_video_mute(HDMI_RX_MUTE_SET, true);
				rx[E_PORT1].var.mute_cnt = 0;
				if (log_level & 0x100)
					rx_pr("vpp mute\n");
			}
			skip_frame(skip_frame_cnt, E_PORT1, "irq1 sqofclk_fall");
			if (log_level & 0x100)
				rx_pr("[isr] sqofclk_fall\n");
		}
	if (hdmirx_top_intr_stat & (1 << 28)) {
		if (log_level & 0x100)
			rx_pr("[isr] sqofclk_rise\n");
	}
	if (hdmirx_top_intr_stat & (1 << 21)) {
		if (rx[E_PORT1].state >= FSM_SIG_STABLE) {
			emp_rcvd = true;
			//rx_emp_field_done_irq(E_PORT1);
			//rx_pkt_handler(PKT_BUFF_SET_EMP, E_PORT1);
		}
		if (log_level & 0x400)
			rx_pr("[isr] emp_field_done\n");
	}
	if (hdmirx_top_intr_stat & (1 << 23)) {
		rx_pkt_initial(E_PORT1);
		if (rx[E_PORT1].state >= FSM_SIG_STABLE) {
			rx[E_PORT1].vsync_cnt++;
			if (rx_spd_type[E_PORT1]) {
				rx_pkt_handler(PKT_BUFF_SET_SPD, E_PORT1);
				rx_spd_type[E_PORT1] = 0;
			}
			if (rx_emp_type[E_PORT1] & EMP_TYPE_VTEM) {
				rx[E_PORT1].vrr_en = true;
				rx_emp_type[E_PORT1] &= (~EMP_TYPE_VTEM);
			} else {
				rx[E_PORT1].vrr_en = false;
			}
			if (emp_rcvd) {
				rx_emp_field_done_irq(E_PORT1);
				rx_pkt_handler(PKT_BUFF_SET_EMP, E_PORT1);
				rx_update_sig_info(E_PORT1);
				emp_rcvd = false;
			}
			tvin_update_vdin_prop(rx_get_port_type(E_PORT1));
		}
		if (log_level & 0x400)
			rx_pr("[isr] DE rise.\n");
	}
	if (hdmirx_top_intr_stat & (1 << 26)) {
		rx_emp_lastpkt_done_irq(E_PORT1);
		if (log_level & 0x400)
			rx_pr("[isr] last_emp_done\n");
	}
	if (hdmirx_top_intr_stat & (1 << 24))
		if (log_level & 0x100)
			rx_pr("[isr] tmds_align_stable_chg\n");
	if (hdmirx_top_intr_stat & (1 << 23))
		if (log_level & 0x100)
			rx_pr("[isr] meter_stable_chg_cable\n");
	if (hdmirx_top_intr_stat & (1 << 13))
		rx_pr("[isr] auth rise\n");
	if (hdmirx_top_intr_stat & (1 << 14))
		rx_pr("[isr] auth fall\n");
	if (hdmirx_top_intr_stat & (1 << 15))
		rx_pr("[isr] enc rise\n");
	if (hdmirx_top_intr_stat & (1 << 16))
		rx_pr("[isr] enc fall\n");
	if (hdmirx_top_intr_stat & (1 << 3))
		rx_pr("[isr] 5v rise\n");
	if (hdmirx_top_intr_stat & (1 << 4))
		rx_pr("[isr] 5v fall\n");
	if (hdmirx_top_intr_stat & (1 << 2))
		if (log_level & COR_LOG)
			rx_pr("[isr] phy dig\n");
	if (hdmirx_top_intr_stat & (1 << 1))
		if (log_level & COR_LOG)
			rx_pr("[isr] ctrl pwd\n");
	if (hdmirx_top_intr_stat & (1 << 0))
		if (log_level & COR_LOG)
			rx_pr("[isr] ctrl aon\n");
	return IRQ_HANDLED;
}

irqreturn_t irq2_handler(int irq, void *params)
{
	unsigned long hdmirx_top_intr_stat;
	unsigned long long cur_clks, irq_duration;
	static unsigned long long last_clks;
	int error = 0;
	bool emp_rcvd;
	u32 tmp = 0;
	int status = 0;

	if (params == 0) {
		rx_pr("%s: %s\n", __func__,
		      "RX IRQ invalid parameter");
		return IRQ_HANDLED;
	}

	cur_clks = sched_clock();
	irq_duration = cur_clks - last_clks;
	last_clks = cur_clks;

	if (irq_duration <= TIME_1MS)
		rx[E_PORT2].irq_err_cnt++;
	else
		rx[E_PORT2].irq_err_cnt = 0;
	if (rx[E_PORT2].irq_err_cnt >= irq_err_max) {
		rx_pr("IRQ ERR\n");
		if (video_mute_enabled(E_PORT2)) {
			rx_mute_vpp(rx_get_port_type(E_PORT2));
			set_video_mute(HDMI_RX_MUTE_SET, true);
			rx_pr("vpp mute\n");
		}
		rx[E_PORT2].irq_err_cnt = 0;
		rx_irq_en(0, E_PORT2);
		hdmirx_output_en(false);
		if (rx[E_PORT2].state > FSM_FRL_FLT_READY)
			rx[E_PORT2].state = FSM_FRL_FLT_READY;
	}
	hdmirx_top_intr_stat = hdmirx_rd_top(TOP_INTR_STAT, E_PORT2);
	hdmirx_wr_top(TOP_INTR_STAT_CLR, hdmirx_top_intr_stat, E_PORT2);

	if (irq_duration <= TIME_1MS && (hdmirx_top_intr_stat & (1 << 23)))
		rx[E_PORT2].de_err_cnt++;
	else
		rx[E_PORT2].de_err_cnt = 0;
	if (rx[E_PORT2].de_err_cnt >= de_err_max) {
		rx_pr("DE ERR\n");
		if (video_mute_enabled(E_PORT2)) {
			rx_mute_vpp(rx_get_port_type(E_PORT2));
			set_video_mute(HDMI_RX_MUTE_SET, true);
			rx_pr("vpp mute\n");
		}
		rx[E_PORT2].de_err_cnt = 0;
		rx_irq_en(0, E_PORT2);
		hdmirx_output_en(false);
		if (rx[E_PORT2].state > FSM_FRL_FLT_READY)
			rx[E_PORT2].state = FSM_FRL_FLT_READY;
	}
	tmp = hdmirx_top_intr_stat & 0x2;
	if (tmp) {
		error = rx_cor2_irq_handler();
		if (error < 0) {
			if (error != -EPERM)
				rx_pr("cor1 irq %d\n", error);
		}
	}
	/* modify interrupt flow for isr loading */
	/* top interrupt handler */
	if (rx[E_PORT2].var.frl_rate == 0) {
		if (hdmirx_top_intr_stat & (1 << 20)) {
			if (video_mute_enabled(E_PORT2)) {
				rx[E_PORT2].vpp_mute = true;
				set_video_mute(HDMI_RX_MUTE_SET, true);
				rx[E_PORT2].var.mute_cnt = 0;
				if (log_level & 0x100)
					rx_pr("vpp mute\n");
			}
			skip_frame(skip_frame_cnt, E_PORT2, "irq2 sqofclk_fall");
			if (log_level & 0x100)
				rx_pr("[isr] sqofclk_fall\n");
		}
	}
	if (rx[E_PORT2].var.frl_rate) {
		if (hdmirx_top_intr_stat & (1 << 29)) {
			if (video_mute_enabled(E_PORT2)) {
				rx[E_PORT2].vpp_mute = true;
				rx_mute_vpp(rx_get_port_type(E_PORT2));
				set_video_mute(HDMI_RX_MUTE_SET, true);
				rx[E_PORT2].var.mute_cnt = 0;
				if (log_level & 0x100)
					rx_pr("vpp mute\n");
			}
			skip_frame(skip_frame_cnt, E_PORT2, "irq2 valid_m_fall");
			if (log_level & 0x100)
				rx_pr("[isr] valid_m_fall\n");
		}
	}
	if (hdmirx_top_intr_stat & (1 << 21)) {
		if (rx[E_PORT2].state >= FSM_SIG_STABLE) {
			emp_rcvd = true;
			//rx_emp_field_done_irq(E_PORT2);
			//rx_pkt_handler(PKT_BUFF_SET_EMP, E_PORT2);
		}
		if (log_level & 0x400)
			rx_pr("[isr] emp_field_done\n");
	}
	if (hdmirx_top_intr_stat & (1 << 23)) {
		rx_pkt_initial(E_PORT2);
		rx[E_PORT2].var.check_dsc_de_cnt++;
		if (rx[E_PORT2].state >= FSM_SIG_STABLE) {
			rx[E_PORT2].vsync_cnt++;
			if (rx_spd_type[E_PORT2]) {
				rx_pkt_handler(PKT_BUFF_SET_SPD, E_PORT2);
				rx_spd_type[E_PORT2] = 0;
			}
			if (rx_emp_type[E_PORT2] & EMP_TYPE_VTEM) {
				rx[E_PORT2].vrr_en = true;
				rx_emp_type[E_PORT2] &= (~EMP_TYPE_VTEM);
			} else {
				rx[E_PORT2].vrr_en = false;
			}
			if (emp_rcvd) {
				rx_emp_field_done_irq(E_PORT2);
				rx_pkt_handler(PKT_BUFF_SET_EMP, E_PORT2);
				rx_update_sig_info(E_PORT2);
				emp_rcvd = false;
			}
			tvin_update_vdin_prop(rx_get_port_type(E_PORT2));
		}
		if (log_level & 0x400)
			rx_pr("[isr] DE rise.\n");
	}
	if (hdmirx_top_intr_stat & (1 << 26)) {
		rx_emp_lastpkt_done_irq(E_PORT2);
		if (log_level & 0x400)
			rx_pr("[isr] last_emp_done\n");
	}
	if (hdmirx_top_intr_stat & (1 << 24))
		if (log_level & 0x100)
			rx_pr("[isr] tmds_align_stable_chg\n");
	//if (hdmirx_top_intr_stat & (1 << 23))
		//if (log_level & 0x100)
			//rx_pr("[isr] meter_stable_chg_cable\n");
	if (hdmirx_top_intr_stat & (1 << 13))
		rx_pr("[isr] auth rise\n");
	if (hdmirx_top_intr_stat & (1 << 14))
		rx_pr("[isr] auth fall\n");
	if (hdmirx_top_intr_stat & (1 << 15))
		rx_pr("[isr] enc rise\n");
	if (hdmirx_top_intr_stat & (1 << 16))
		rx_pr("[isr] enc fall\n");
	if (hdmirx_top_intr_stat & (1 << 3))
		rx_pr("[isr] 5v rise\n");
	if (hdmirx_top_intr_stat & (1 << 4))
		rx_pr("[isr] 5v fall\n");
	if (hdmirx_top_intr_stat & (1 << 2))
		if (log_level & COR_LOG)
			rx_pr("[isr] phy dig\n");
	if (hdmirx_top_intr_stat & (1 << 1)) {
		tmp = hdmirx_rd_cor(H21RXSB_INTR2_M42H_IVCRX, E_PORT2);
		if (tmp & 0x10) {
			hdmirx_wr_cor(H21RXSB_INTR2_M42H_IVCRX, 0xff, E_PORT2);
			//rx_pr("overlap!!! 0x152f:0x%x, 0x1563:0x%x\n", tmp,
				//hdmirx_rd_cor(H21RXSB_INTR3_M42H_IVCRX, E_PORT2));
		}
		if (rx[E_PORT2].var.frl_rate && rx[E_PORT2].state >= FSM_SIG_STABLE) {
			status = hdmirx_rd_bits_cor(VP_FDET_IRQ_STATUS_VID_IVCRX,
				MSK(4, 7), E_PORT2);
			if (status != 0) {
				clr_frl_fifo_status(E_PORT2);
				htotal_cnt++;
				if (htotal_cnt >= 10) {
					htotal_cnt = 0;
				}
			}
		}
	}
	tmp = hdmirx_rd_cor(RX_INTR9_AON_AON_IVCRX, E_PORT2);
	if (tmp & 0x4) {
		hdmirx_wr_cor(RX_INTR9_AON_AON_IVCRX, 0xff, E_PORT2);
		rx_pr("ced update!!! 0x0084:0x%x\n", tmp);
		rx_pr("ced flag 0x%x\n", hdmirx_rd_cor(SCDCS_UPD_FLAGS_SCDC_IVCRX, E_PORT2));
	}
	if (hdmirx_top_intr_stat & (1 << 0))
		if (log_level & COR_LOG)
			rx_pr("[isr] ctrl aon\n");
	return IRQ_HANDLED;
}

irqreturn_t irq3_handler(int irq, void *params)
{
	unsigned long hdmirx_top_intr_stat;
	unsigned long long cur_clks, irq_duration;
	static unsigned long long last_clks;
	int error = 0;
	bool emp_rcvd;
	u32 tmp = 0;
	int status = 0;

	if (params == 0) {
		rx_pr("%s: %s\n", __func__,
		      "RX IRQ invalid parameter");
		return IRQ_HANDLED;
	}

	cur_clks = sched_clock();
	irq_duration = cur_clks - last_clks;
	last_clks = cur_clks;

	if (irq_duration <= TIME_1MS)
		rx[E_PORT3].irq_err_cnt++;
	else
		rx[E_PORT3].irq_err_cnt = 0;
	if (rx[E_PORT3].irq_err_cnt >= irq_err_max) {
		rx_pr("IRQ ERR\n");
		if (video_mute_enabled(E_PORT3)) {
			rx_mute_vpp(rx_get_port_type(E_PORT3));
			set_video_mute(HDMI_RX_MUTE_SET, true);
			rx_pr("vpp mute\n");
		}
		rx[E_PORT3].irq_err_cnt = 0;
		rx_irq_en(0, E_PORT3);
		hdmirx_output_en(false);
		if (rx[E_PORT3].state > FSM_FRL_FLT_READY)
			rx[E_PORT3].state = FSM_FRL_FLT_READY;
	}
	hdmirx_top_intr_stat = hdmirx_rd_top(TOP_INTR_STAT, E_PORT3);
	hdmirx_wr_top(TOP_INTR_STAT_CLR, hdmirx_top_intr_stat, E_PORT3);

	if (irq_duration <= TIME_1MS && (hdmirx_top_intr_stat & (1 << 23)))
		rx[E_PORT3].de_err_cnt++;
	else
		rx[E_PORT3].de_err_cnt = 0;
	if (rx[E_PORT3].de_err_cnt >= de_err_max) {
		rx_pr("DE ERR\n");
		if (video_mute_enabled(E_PORT3)) {
			rx_mute_vpp(rx_get_port_type(E_PORT3));
			set_video_mute(HDMI_RX_MUTE_SET, true);
			rx_pr("vpp mute\n");
		}
		rx[E_PORT3].de_err_cnt = 0;
		rx_irq_en(0, E_PORT3);
		hdmirx_output_en(false);
		if (rx[E_PORT3].state > FSM_FRL_FLT_READY)
			rx[E_PORT3].state = FSM_FRL_FLT_READY;
	}
	tmp = hdmirx_top_intr_stat & 0x2;
	if (tmp) {
		error = rx_cor3_irq_handler();
		if (error < 0) {
			if (error != -EPERM)
				rx_pr("cor1 irq %d\n", error);
		}
	}
	/* modify interrupt flow for isr loading */
	/* top interrupt handler */
	if (rx[E_PORT3].var.frl_rate == 0) {
		if (hdmirx_top_intr_stat & (1 << 20)) {
			if (video_mute_enabled(E_PORT3)) {
				rx[E_PORT3].vpp_mute = true;
				set_video_mute(HDMI_RX_MUTE_SET, true);
				rx[E_PORT3].var.mute_cnt = 0;
				if (log_level & 0x100)
					rx_pr("vpp mute\n");
			}
			skip_frame(skip_frame_cnt, E_PORT3, "irq3 sqofclk_fall");
			if (log_level & 0x100)
				rx_pr("[isr] sqofclk_fall\n");
		}
	}
	if (rx[E_PORT3].var.frl_rate) {
		if (hdmirx_top_intr_stat & (1 << 29)) {
			if (video_mute_enabled(E_PORT3)) {
				rx[E_PORT3].vpp_mute = true;
				rx_mute_vpp(rx_get_port_type(E_PORT3));
				set_video_mute(HDMI_RX_MUTE_SET, true);
				rx[E_PORT3].var.mute_cnt = 0;
				if (log_level & 0x100)
					rx_pr("vpp mute\n");
			}
			skip_frame(skip_frame_cnt, E_PORT3, "irq3 valid_m_fail");
			if (log_level & 0x100)
				rx_pr("[isr] valid_m_fall\n");
		}
	}
	if (hdmirx_top_intr_stat & (1 << 21)) {
		if (rx[E_PORT3].state >= FSM_SIG_STABLE) {
			emp_rcvd = true;
			//rx_emp_field_done_irq(E_PORT3);
			//rx_pkt_handler(PKT_BUFF_SET_EMP, E_PORT3);
		}
		if (log_level & 0x400)
			rx_pr("[isr] emp_field_done\n");
	}
	if (hdmirx_top_intr_stat & (1 << 23)) {
		rx_pkt_initial(E_PORT3);
		rx[E_PORT3].var.check_dsc_de_cnt++;
		if (rx[E_PORT3].state >= FSM_SIG_STABLE) {
			rx[E_PORT3].vsync_cnt++;
			if (rx_spd_type[E_PORT3]) {
				rx_pkt_handler(PKT_BUFF_SET_SPD, E_PORT3);
				rx_spd_type[E_PORT3] = 0;
			}
			if (rx_emp_type[E_PORT3] & EMP_TYPE_VTEM) {
				rx[E_PORT3].vrr_en = true;
				rx_emp_type[E_PORT3] &= (~EMP_TYPE_VTEM);
			} else {
				rx[E_PORT3].vrr_en = false;
			}
			if (emp_rcvd) {
				rx_emp_field_done_irq(E_PORT3);
				rx_pkt_handler(PKT_BUFF_SET_EMP, E_PORT3);
				rx_update_sig_info(E_PORT3);
				emp_rcvd = false;
			}
			tvin_update_vdin_prop(rx_get_port_type(E_PORT3));
		}
		if (log_level & 0x400)
			rx_pr("[isr] DE rise.\n");
	}
	if (hdmirx_top_intr_stat & (1 << 26)) {
		rx_emp_lastpkt_done_irq(E_PORT3);
		if (log_level & 0x400)
			rx_pr("[isr] last_emp_done\n");
	}
	if (hdmirx_top_intr_stat & (1 << 24))
		if (log_level & 0x100)
			rx_pr("[isr] tmds_align_stable_chg\n");
	if (hdmirx_top_intr_stat & (1 << 23))
		if (log_level & 0x100)
			rx_pr("[isr] meter_stable_chg_cable\n");
	if (hdmirx_top_intr_stat & (1 << 13))
		rx_pr("[isr] auth rise\n");
	if (hdmirx_top_intr_stat & (1 << 14))
		rx_pr("[isr] auth fall\n");
	if (hdmirx_top_intr_stat & (1 << 15))
		rx_pr("[isr] enc rise\n");
	if (hdmirx_top_intr_stat & (1 << 16))
		rx_pr("[isr] enc fall\n");
	if (hdmirx_top_intr_stat & (1 << 3))
		rx_pr("[isr] 5v rise\n");
	if (hdmirx_top_intr_stat & (1 << 4))
		rx_pr("[isr] 5v fall\n");
	if (hdmirx_top_intr_stat & (1 << 2))
		if (log_level & COR_LOG)
			rx_pr("[isr] phy dig\n");
	if (hdmirx_top_intr_stat & (1 << 1)) {
		tmp = hdmirx_rd_cor(H21RXSB_INTR2_M42H_IVCRX, E_PORT3);
		if (tmp & 0x10) {
			hdmirx_wr_cor(H21RXSB_INTR2_M42H_IVCRX, 0xff, E_PORT3);
			//rx_pr("overlap!!! 0x152f:0x%x, 0x1563:0x%x\n", tmp,
				//hdmirx_rd_cor(H21RXSB_INTR3_M42H_IVCRX, E_PORT3));
		}
		if (rx[E_PORT3].var.frl_rate && rx[E_PORT3].state >= FSM_SIG_STABLE) {
			status = hdmirx_rd_bits_cor(VP_FDET_IRQ_STATUS_VID_IVCRX,
				MSK(4, 7), E_PORT3);
			if (status != 0) {
				clr_frl_fifo_status(E_PORT3);
				htotal_cnt++;
				if (htotal_cnt >= 10) {
					if (rx[E_PORT3].var.dbg_ve)
						rx_cor_reset_t3x(E_PORT3);
					htotal_cnt = 0;
				}
			}
		}
	}
	tmp = hdmirx_rd_cor(RX_INTR9_AON_AON_IVCRX, E_PORT3);
	if (tmp & 0x4) {
		hdmirx_wr_cor(RX_INTR9_AON_AON_IVCRX, 0xff, E_PORT3);
		rx_pr("ced update!!! 0x0084:0x%x\n", tmp);
		rx_pr("ced flag 0x%x\n", hdmirx_rd_cor(SCDCS_UPD_FLAGS_SCDC_IVCRX, E_PORT3));
	}
	if (hdmirx_top_intr_stat & (1 << 0))
		if (log_level & COR_LOG)
			rx_pr("[isr] ctrl aon\n");
	return IRQ_HANDLED;
}

static const u32 sr_tbl[][2] = {
	{32000, 3000},
	{44100, 2000},
	{48000, 2000},
	{64000, 3000},
	{88200, 4000},
	{96000, 4000},
	{176400, 5000},
	{192000, 5000},
	{0, 0}
};

static bool check_real_sr_change(u8 port)
{
	u8 i;
	bool ret = false;
	/* note: if arc is mismatch with LUT, then return 0 */
	u32 ret_sr = 0;

	if (port  == rx_info.sub_port)
		return false;
	for (i = 0; sr_tbl[i][0] != 0; i++) {
		if (abs(rx[port].aud_info.arc - sr_tbl[i][0]) < sr_tbl[i][1]) {
			ret_sr = sr_tbl[i][0];
			break;
		}
	}
	if (ret_sr != rx[port].aud_info.real_sr) {
		rx[port].aud_info.real_sr = ret_sr;
		ret = true;
		if (log_level & AUDIO_LOG)
			dump_state(RX_DUMP_AUDIO, port);
	}
	return ret;
}

static const struct freq_ref_s freq_ref[] = {
	/* interlace 420 3d hac vac index */
	/* 420mode */
	{0,	3,	0,	1920,	2160,	HDMI_2160p50_16x9_Y420},
	{0,	3,	0,	2048,	2160,	HDMI_4096p50_256x135_Y420},
	{0,	3,	0,	960,	1080,	HDMI_1080p_420},
	/* interlace */
	{1,	0,	0,	720,	240,	HDMI_480i60},
	{1,	0,	0,	1440,	240,	HDMI_1440x480i60},
	{1,	0,	0,	720,	288,	HDMI_576i50},
	{1,	0,	0,	1440,	288,	HDMI_1440x576i50},
	{1,	0,	0,	1920,	540,	HDMI_1080i50},
	{1,	0,	0,	1920,	1103,	HDMI_1080i_ALTERNATIVE},
	{1,	0,	0,	1920,	2228,	HDMI_1080i_FRAMEPACKING},
	{1,	0,	0,	360,	240,    HDMI_360x480i},
	{1,	0,	0,	360,	288,	HDMI_360x576i},
	{0,	0,	0,	360,	480,	HDMI_360x480p},
	{0,	0,	0,	360,	576,	HDMI_360x576p},
	{0,	0,	0,	1440,	240,	HDMI_1440x240p60},
	{0, 0,	0,	720,	240,	HDMI_720X240},
	{0,	0,	0,	2880,	240,	HDMI_2880x240p60},
	{0,	0,	0,	1440,	288,	HDMI_1440x288p50},
	{0,	0,	0,	2880,	288,	HDMI_2880x288p50},
	{0,	0,	0,	2560,	1080,	HDMI_2560x1080p24_64x27},
	{0,	0,	0,	3840,	1080,	HDMI_3840x1080p60},

	{0,	0,	0,	720,	480,	HDMI_480p60},
	{0,	0,	0,	1440,	480,	HDMI_1440x480p60},
	{0,	0,	0,	720,	1005,	HDMI_480p_FRAMEPACKING},

	{0,	0,	0,	720,	576,	HDMI_576p50},
	{0,	0,	0,	1440,	576,	HDMI_1440x576p50},
	{0,	0,	0,	720,	1201,	HDMI_576p_FRAMEPACKING},

	{0,	0,	0,	1280,	720,	HDMI_720p50},
	{0,	0,	0,	1280,	1470,	HDMI_720p_FRAMEPACKING},

	{1,	0,	0,	1920,	1080,	HDMI_1920_1080_INTERLACED},
	{0,	0,	0,	1920,	1080,	HDMI_1080p50},
	{0,	4,	0,	1920,	2160,	HDMI_2160p24_16x9},
	{0,	0,	0,	1920,	2160,	HDMI_1080p_ALTERNATIVE},
	{0,	0,	0,	1920,	2205,	HDMI_1080p_FRAMEPACKING},

	{1,	0,	0,	2880,	240,	HDMI_2880x480i60},
	{1,	0,	0,	1440,	240,	HDMI_2880x480i60},
	{1,	0,	0,	2880,	288,	HDMI_2880x576i50},
	{0,	0,	0,	2880,	480,	HDMI_2880x480p60},
	{0,	0,	0,	2880,	576,	HDMI_2880x576p50},
	/* vesa format*/
	{0,	0,	0,	640,	480,	HDMI_640x480p60},
	{0,	0,	0,	640,	350,	HDMI_640_350},
	{0,	0,	0,	640,	400,	HDMI_640_400},
	{0,	0,	0,	720,	400,	HDMI_720_400},
	{0,	0,	0,	800,	600,	HDMI_800_600},
	{0,	0,	0,	848,	480,	HDMI_848_480},
	{0,	0,	0,	1024,	768,	HDMI_1024_768},
	{0,	0,	0,	1280,	768,	HDMI_1280_768},
	{0,	0,	0,	1280,	800,	HDMI_1280_800},
	{0,	0,	0,	1280,	960,	HDMI_1280_960},
	{0,	0,	0,	1280,	1024,	HDMI_1280_1024},
	{0,	0,	0,	1360,	768,	HDMI_1360_768},
	{0,	0,	0,	1366,	768,	HDMI_1366_768},
	{0,	0,	0,	1440,	900,	HDMI_1440_900},
	{0,	0,	0,	1400,	1050,	HDMI_1400_1050},
	{0,	0,	0,	1600,	900,	HDMI_1600_900},
	{0,	0,	0,	1600,	1200,	HDMI_1600_1200},
	{0,	0,	0,	1680,	1050,	HDMI_1680_1050},
	{0,	0,	0,	1792,	1344,	HDMI_1792_1344},
	{0,	0,	0,	1856,	1392,	HDMI_1856_1392},
	{0,	0,	0,	1920,	1200,	HDMI_1920_1200},
	{0,	0,	0,	1920,	1440,	HDMI_1920_1440},
	{0,	0,	0,	1152,	864,	HDMI_1152_864},
	{0,	0,	0,	2048,	1152,	HDMI_2048_1152},
	{0,	0,	0,	2560,	1600,	HDMI_2560_1600},
	{0,	0,	0,	3840,	600,	HDMI_3840_600},
	{0, 0,	0,	2688,	1520,	HDMI_2688_1520},
	/* 4k2k and 4k2k dsc mode*/
	{0,	0,	0,	3840,	2160,	HDMI_2160p24_16x9},
	{0,	4,	0,	1370,	2160,	HDMI_2160p24_16x9},
	{0,	4,	0,	1130,	2160,	HDMI_2160p24_16x9},

	/* 8k and 8k dsc mode */
	{0,	0,	0,	7680,	4320,	HDMI_7680x4320},
	{0,	4,	0,	3840,	4320,	HDMI_7680x4320},
	{0,	4,	0,	3182,	4320,	HDMI_7680x4320},
	{0,	4,	0,	3142,	4320,	HDMI_7680x4320},
	{0,	4,	0,	2460,	4320,	HDMI_7680x4320},
	{0,	4,	0,	2360,	4320,	HDMI_7680x4320},
	{0,	4,	0,	2380,	4320,	HDMI_7680x4320},
	{0,	4,	0,	2600,	4320,	HDMI_7680x4320},

	{0,	0,	0,	3840,	4320,	HDMI_3840x4320},
	{0,	0,	0,	5120,	2880,	HDMI_5120x2880},
	{0,	0,	0,	2560,	2880,	HDMI_2560x2880},
	/*{0,	0,	0,	1920,	2160,	HDMI_2160p25_16x9},*/
	/*4k(4096) and dsc mode */
	{0,	0,	0,	4096,	2160,	HDMI_4096p24_256x135},
	{0,	4,	0,	2048,	2160,	HDMI_4096p24_256x135},

	{0,	0,	0,	2560,	1440,	HDMI_2560_1440},
	{0,	0,	0,	2560,	3488,	HDMI_2560_1440},
	{0,	0,	0,	2560,	2986,	HDMI_2560_1440},
	/* for AG-506 */
	{0,	0,	0,	720,	483,	HDMI_480p60},
	/* for NUC8BEK */
	{0,	0,	0,	1920,	2160,	HDMI_1920x2160p60_16x9},
	{0,	0,	0,	960,	540,	HDMI_960x540},
};

bool de_active_check(u8 port)
{
	bool ret = false;

	if (rx[port].cur.hactive <= 200 || rx[port].cur.vactive <= 200)
		ret = true;

	//if (rx[port].cur.htotal <= 200 || rx[port].cur.vtotal <= 200)
		//ret = true;

	if (rx[port].cur.hactive - rx[port].cur.vactive >= 6000)
		ret = true;
	return ret;
}

static bool fmt_vic_abnormal(u8 port)
{
	/* if format is unknown or unsupported after
	 * timing match, but TX send normal VIC, then
	 * abnormal format is detected.
	 */
	if (de_active_check(port) &&
		rx[port].pre.sw_vic == HDMI_UNSUPPORT) {
		rx_pr("de error\n");
		return true;
	}

	if (rx[port].pre.hw_vic != HDMI_UNKNOWN &&
	    rx[port].pre.sw_vic == HDMI_UNSUPPORT) {
		rx_pr("hw_vic: is not matched!\n");
		return true;
	} else if (rx[port].pre.sw_vic >= HDMI_VESA_OFFSET &&
		rx[port].pre.sw_vic < HDMI_UNSUPPORT &&
		rx[port].pre.repeat != 0) {
		/* no pixel repetition for VESA mode */
		rx_pr("repetition abnormal for vesa\n");
		return true;
	}
	return false;
}

enum fps_e get_fps_index(u8 port)
{
	enum fps_e ret = E_60HZ;

	if ((abs(rx[port].pre.frame_rate - 2400)
		< diff_frame_th))
		ret = E_24HZ;
	else if ((abs(rx[port].pre.frame_rate - 2500)
		  < diff_frame_th))
		ret = E_25HZ;
	else if ((abs(rx[port].pre.frame_rate - 3000)
		  < diff_frame_th))
		ret = E_30HZ;
	else if ((abs(rx[port].pre.frame_rate - 5000)
		  < diff_frame_th))
		ret = E_50HZ;
	else if ((abs(rx[port].pre.frame_rate - 7200)
		  < diff_frame_th))
		ret = E_72HZ;
	else if ((abs(rx[port].pre.frame_rate - 7500)
		  < diff_frame_th))
		ret = E_75HZ;
	else if ((abs(rx[port].pre.frame_rate - 10000)
		  < diff_frame_th))
		ret = E_100HZ;
	else if ((abs(rx[port].pre.frame_rate - 12000)
		  < diff_frame_th))
		ret = E_120HZ;
	else
		ret = E_60HZ;

	return ret;
}

enum tvin_sig_fmt_e hdmirx_hw_get_fmt(u8 port)
{
	enum tvin_sig_fmt_e fmt = TVIN_SIG_FMT_NULL;
	enum hdmi_vic_e vic = HDMI_UNKNOWN;

	/*
	 * if (fmt_vic_abnormal())
	 *	vic = rx[port].pre.hw_vic;
	 * else
	 */
	vic = rx[port].pre.sw_vic;
	if (force_vic)
		vic = force_vic;

	switch (vic) {
	case HDMI_640x480p60:
		fmt = TVIN_SIG_FMT_HDMI_640X480P_60HZ;
		break;
	case HDMI_640_350:
		fmt = TVIN_SIG_FMT_HDMI_640X350_85HZ;
		break;
	case HDMI_640_400:
		fmt = TVIN_SIG_FMT_HDMI_640X400_85HZ;
		break;
	case HDMI_480p60:	/*2 */
	case HDMI_480p60_16x9:	/*3 */
	case HDMI_480p120:	/* 48 */
	case HDMI_480p120_16x9:	/* 49 */
	case HDMI_480p240:	/* 56 */
	case HDMI_480p240_16x9:	/* 57 */
		fmt = TVIN_SIG_FMT_HDMI_720X480P_60HZ;
		break;
	case HDMI_1440x480p60:	/* 14 */
	case HDMI_1440x480p60_16x9:	/* 15 */
		fmt = TVIN_SIG_FMT_HDMI_1440X480P_60HZ;
		break;
	case HDMI_480p_FRAMEPACKING:
		fmt = TVIN_SIG_FMT_HDMI_720X480P_60HZ_FRAME_PACKING;
		break;
	case HDMI_848_480:
		fmt = TVIN_SIG_FMT_HDMI_848X480_60HZ;
		break;
	case HDMI_720p24:	/* 60 */
	case HDMI_720p25:	/* 61 */
	case HDMI_720p30:	/* 62 */
	case HDMI_720p50:	/* 19 */
	case HDMI_720p60:	/* 4 */
	case HDMI_720p100:	/* 41 */
	case HDMI_720p120:	/* 47 */
	case HDMI_720p24_64x27:	/* 65 */
	case HDMI_720p25_64x27:	/* 66 */
	case HDMI_720p30_64x27:	/* 67 */
	case HDMI_720p50_64x27:	/* 68 */
	case HDMI_720p60_64x27:	/* 69 */
	case HDMI_720p100_64x27:	/* 70 */
	case HDMI_720p120_64x27:	/* 71 */
		if (get_fps_index(port) == E_24HZ)
			fmt = TVIN_SIG_FMT_HDMI_1280X720P_24HZ;
		else if (get_fps_index(port) == E_25HZ)
			fmt = TVIN_SIG_FMT_HDMI_1280X720P_25HZ;
		else if (get_fps_index(port) == E_30HZ)
			fmt = TVIN_SIG_FMT_HDMI_1280X720P_30HZ;
		else if (get_fps_index(port) == E_50HZ)
			fmt = TVIN_SIG_FMT_HDMI_1280X720P_50HZ;
		else
			fmt = TVIN_SIG_FMT_HDMI_1280X720P_60HZ;
		break;
	case HDMI_720p_FRAMEPACKING:
		fmt = TVIN_SIG_FMT_HDMI_1280X720P_60HZ_FRAME_PACKING;
		break;
	case HDMI_1080i50:	/* 20 */
	case HDMI_1080i100:	/* 40 */
	case HDMI_1080i60:	/* 5 */
	case HDMI_1080i120:	/* 46 */
		if (get_fps_index(port) == E_50HZ)
			fmt = TVIN_SIG_FMT_HDMI_1920X1080I_50HZ_A;
		else
			fmt = TVIN_SIG_FMT_HDMI_1920X1080I_60HZ;
		break;
	case HDMI_1080i_FRAMEPACKING:
		fmt = TVIN_SIG_FMT_HDMI_1920X1080I_60HZ_FRAME_PACKING;
		break;
	case HDMI_1080i_ALTERNATIVE:
		fmt = TVIN_SIG_FMT_HDMI_1920X1080I_60HZ_ALTERNATIVE;
		break;
	case HDMI_480i60:	/* 6 */
	case HDMI_480i60_16x9:	/* 7 */
	case HDMI_480i120:	/* 50 */
	case HDMI_480i120_16x9:	/* 51 */
	case HDMI_480i240:	/* 58 */
	case HDMI_480i240_16x9:	/* 59 */
	case HDMI_720x480i:
		fmt = TVIN_SIG_FMT_HDMI_720X480I_60HZ;
		break;
	case HDMI_1440x480i60:
		fmt = TVIN_SIG_FMT_HDMI_1440X480I_60HZ;
		break;
	case HDMI_1080p24:	/* 32 */
	case HDMI_1080p24_64x27: /* 72 */
	case HDMI_1080p25:	/* 33 */
	case HDMI_1080p25_64x27:	/* 73 */
	case HDMI_1080p30:	/* 34 */
	case HDMI_1080p30_64x27:	/* 74 */
	case HDMI_1080p50:	/* 31 */
	case HDMI_1080p60:	/* 16 */
	case HDMI_1080p50_64x27:	/* 75 */
	case HDMI_1080p60_64x27:	/* 76 */
	case HDMI_1080p100:	/* 64 */
	case HDMI_1080p120:	/* 63 */
	case HDMI_1080p100_64x27:	/* 77 */
	case HDMI_1080p120_64x27:	/* 78 */
	case HDMI_1080p_420:
	case HDMI_1920_1080_INTERLACED:
		if (get_fps_index(port) == E_24HZ)
			fmt = TVIN_SIG_FMT_HDMI_1920X1080P_24HZ;
		else if (get_fps_index(port) == E_25HZ)
			fmt = TVIN_SIG_FMT_HDMI_1920X1080P_25HZ;
		else if (get_fps_index(port) == E_30HZ)
			fmt = TVIN_SIG_FMT_HDMI_1920X1080P_30HZ;
		else if (get_fps_index(port) == E_50HZ)
			fmt = TVIN_SIG_FMT_HDMI_1920X1080P_50HZ;
		else
			fmt = TVIN_SIG_FMT_HDMI_1920X1080P_60HZ;
		break;
	case HDMI_1080p_FRAMEPACKING:
		fmt = TVIN_SIG_FMT_HDMI_1920X1080P_24HZ_FRAME_PACKING;
		break;
	case HDMI_1080p_ALTERNATIVE:
		fmt = TVIN_SIG_FMT_HDMI_1920X1080P_24HZ_ALTERNATIVE;
		break;
	case HDMI_2560x1080p24_64x27:
	case HDMI_2560x1080p25_64x27:
	case HDMI_2560x1080p30_64x27:
	case HDMI_2560x1080p50_64x27:
	case HDMI_2560x1080p60_64x27:
	case HDMI_2560x1080p100_64x27:
	case HDMI_2560x1080p120_64x27:
		if (get_fps_index(port) == E_24HZ)
			fmt = TVIN_SIG_FMT_HDMI_2560X1080P_24HZ;
		else if (get_fps_index(port) == E_25HZ)
			fmt = TVIN_SIG_FMT_HDMI_2560X1080P_25HZ;
		else if (get_fps_index(port) == E_30HZ)
			fmt = TVIN_SIG_FMT_HDMI_2560X1080P_30HZ;
		else if (get_fps_index(port) == E_50HZ)
			fmt = TVIN_SIG_FMT_HDMI_2560X1080P_50HZ;
		else if (get_fps_index(port) == E_60HZ)
			fmt = TVIN_SIG_FMT_HDMI_2560X1080P_60HZ;
		else if (get_fps_index(port) == E_100HZ)
			fmt = TVIN_SIG_FMT_HDMI_2560X1080P_100HZ;
		else if (get_fps_index(port) == E_120HZ)
			fmt = TVIN_SIG_FMT_HDMI_2560X1080P_120HZ;
		else
			fmt = TVIN_SIG_FMT_HDMI_2560X1080P_60HZ;
		break;
	case HDMI_3840x1080p60:
		fmt = TVIN_SIG_FMT_HDMI_3840X1080P_60HZ;
		break;
	case HDMI_576p50:	/* 17 */
	case HDMI_576p50_16x9: /* 18 */
	case HDMI_576p100:	/* 42 */
	case HDMI_576p100_16x9: /* 43 */
	case HDMI_576p200:	/* 52 */
	case HDMI_576p200_16x9: /* 53 */
		fmt = TVIN_SIG_FMT_HDMI_720X576P_50HZ;
		break;
	case HDMI_1440x576p50:	/* 29 */
	case HDMI_1440x576p50_16x9:	/* 30 */
		fmt = TVIN_SIG_FMT_HDMI_1440X576P_50HZ;
		break;
	case HDMI_576p_FRAMEPACKING:
		fmt = TVIN_SIG_FMT_HDMI_720X576P_50HZ_FRAME_PACKING;
		break;
	case HDMI_576i50:	/* 21 */
	case HDMI_576i50_16x9:	/* 22 */
	case HDMI_576i100:	/* 44 */
	case HDMI_576i100_16x9:	/* 45 */
	case HDMI_576i200:	/* 54 */
	case HDMI_576i200_16x9:	/* 55 */
		fmt = TVIN_SIG_FMT_HDMI_720X576I_50HZ;
		break;
	case HDMI_1440x576i50:
		fmt = TVIN_SIG_FMT_HDMI_1440X576I_50HZ;
		break;
	case HDMI_1440x240p60:	/* 8 */
	case HDMI_1440x240p60_16x9:	/* 9 */
		fmt = TVIN_SIG_FMT_HDMI_1440X240P_60HZ;
		break;
	case HDMI_2880x240p60:	/* 12 */
	case HDMI_2880x240p60_16x9: /* 13 */
		fmt = TVIN_SIG_FMT_HDMI_2880X240P_60HZ;
		break;
	case HDMI_1440x288p50:	/* 23 */
	case HDMI_1440x288p50_16x9: /* 24 */
		fmt = TVIN_SIG_FMT_HDMI_1440X288P_50HZ;
		break;
	case HDMI_2880x288p50:	/* 27 */
	case HDMI_2880x288p50_16x9: /* 28 */
		fmt = TVIN_SIG_FMT_HDMI_2880X288P_50HZ;
		break;
	case HDMI_2880x480i60:	/* 10 */
	case HDMI_2880x480i60_16x9:	/* 11 */
		fmt = TVIN_SIG_FMT_HDMI_2880X480I_60HZ;
		break;
	case HDMI_2880x576i50:	/* 25 */
	case HDMI_2880x576i50_16x9:	/* 26 */
		fmt = TVIN_SIG_FMT_HDMI_2880X576I_50HZ;
		break;
	case HDMI_2880x480p60:	/* 35 */
	case HDMI_2880x480p60_16x9:	/* 36 */
		fmt = TVIN_SIG_FMT_HDMI_2880X480P_60HZ;
		break;
	case HDMI_2880x576p50:	/* 37 */
	case HDMI_2880x576p50_16x9: /* 38 */
		fmt = TVIN_SIG_FMT_HDMI_2880X576P_50HZ;
		break;
	case HDMI_1080i50_1250: /* 39 */
		fmt = TVIN_SIG_FMT_HDMI_1920X1080I_50HZ_B;
		break;

	/* VESA mode*/
	case HDMI_800_600:
		fmt = TVIN_SIG_FMT_HDMI_800X600_00HZ;
		break;
	case HDMI_1024_768:
		fmt = TVIN_SIG_FMT_HDMI_1024X768_00HZ;
		break;
	case HDMI_720_400:
		fmt = TVIN_SIG_FMT_HDMI_720X400_00HZ;
		break;
	case HDMI_720_350:
		fmt = TVIN_SIG_FMT_HDMI_720X350_00HZ;
		break;
	case HDMI_1280_768:
		fmt = TVIN_SIG_FMT_HDMI_1280X768_00HZ;
		break;
	case HDMI_1280_800:
		fmt = TVIN_SIG_FMT_HDMI_1280X800_00HZ;
		break;
	case HDMI_1280_960:
		fmt = TVIN_SIG_FMT_HDMI_1280X960_00HZ;
		break;
	case HDMI_1280_1024:
		fmt = TVIN_SIG_FMT_HDMI_1280X1024_00HZ;
		break;
	case HDMI_1360_768:
		fmt = TVIN_SIG_FMT_HDMI_1360X768_00HZ;
		break;
	case HDMI_1366_768:
		fmt = TVIN_SIG_FMT_HDMI_1366X768_00HZ;
		break;
	case HDMI_1600_1200:
		fmt = TVIN_SIG_FMT_HDMI_1600X1200_00HZ;
		break;
	case HDMI_1600_900:
		fmt = TVIN_SIG_FMT_HDMI_1600X900_60HZ;
		break;
	case HDMI_1792_1344:
		fmt = TVIN_SIG_FMT_HDMI_1792X1344_85HZ;
		break;
	case HDMI_1856_1392:
		fmt = TVIN_SIG_FMT_HDMI_1856X1392_00HZ;
		break;
	case HDMI_1920_1200:
		fmt = TVIN_SIG_FMT_HDMI_1920X1200_00HZ;
		break;
	case HDMI_1920_1440:
		fmt = TVIN_SIG_FMT_HDMI_1920X1440_00HZ;
		break;
	case HDMI_2048_1152:
		fmt = TVIN_SIG_FMT_HDMI_2048X1152_60HZ;
		break;
	case HDMI_1440_900:
		fmt = TVIN_SIG_FMT_HDMI_1440X900_00HZ;
		break;
	case HDMI_1400_1050:
		fmt = TVIN_SIG_FMT_HDMI_1400X1050_00HZ;
		break;
	case HDMI_1680_1050:
		fmt = TVIN_SIG_FMT_HDMI_1680X1050_00HZ;
		break;
	case HDMI_1152_864:
		fmt = TVIN_SIG_FMT_HDMI_1152X864_00HZ;
		break;
	case HDMI_3840_600:
		fmt = TVIN_SIG_FMT_HDMI_3840X600_00HZ;
		break;
	case HDMI_2560_1600:
		fmt = TVIN_SIG_FMT_HDMI_2560X1600_00HZ;
		break;
	case HDMI_2688_1520:
		fmt = TVIN_SIG_FMT_HDMI_2688X1520_00HZ;
		break;
	case HDMI_2160p24_16x9:
	case HDMI_2160p25_16x9:
	case HDMI_2160p30_16x9:
	case HDMI_2160p50_16x9:
	case HDMI_2160p60_16x9:
	case HDMI_2160p24_64x27:
	case HDMI_2160p25_64x27:
	case HDMI_2160p30_64x27:
	case HDMI_2160p50_64x27:
	case HDMI_2160p60_64x27:
	case HDMI_2160p50_16x9_Y420:
	case HDMI_2160p60_16x9_Y420:
	case HDMI_2160p50_64x27_Y420:
	case HDMI_2160p60_64x27_Y420:
		if (en_4k_timing)
			fmt = TVIN_SIG_FMT_HDMI_3840_2160_00HZ;
		else
			fmt = TVIN_SIG_FMT_NULL;
		break;
	case HDMI_4096p24_256x135:
	case HDMI_4096p25_256x135:
	case HDMI_4096p30_256x135:
	case HDMI_4096p50_256x135:
	case HDMI_4096p60_256x135:
	case HDMI_4096p50_256x135_Y420:
	case HDMI_4096p60_256x135_Y420:
		if (en_4k_timing)
			fmt = TVIN_SIG_FMT_HDMI_4096_2160_00HZ;
		else
			fmt = TVIN_SIG_FMT_NULL;
		break;
	case HDMI_7680x4320:
		fmt = TVIN_SIG_FMT_HDMI_7680X4320;
		break;
	case HDMI_3840x4320:
		fmt = TVIN_SIG_FMT_HDMI_3840X4320;
		break;
	case HDMI_5120x2880:
		fmt = TVIN_SIG_FMT_HDMI_5120X2880;
		break;
	case HDMI_2560x2880:
		fmt = TVIN_SIG_FMT_HDMI_2560X2880;
		break;
	case HDMI_2560_1440:
		fmt = TVIN_SIG_FMT_HDMI_2560X1440_00HZ;
		break;
	case HDMI_1920x2160p60_16x9:
		fmt = TVIN_SIG_FMT_HDMI_1920X2160_60HZ;
		break;
	case HDMI_960x540:
		fmt = TVIN_SIG_FMT_HDMI_960X540_60HZ;
		break;
	case HDMI_720X240:
		return TVIN_SIG_FMT_HDMI_720X240;
	case HDMI_360x480i:
		fmt = TVIN_SIG_FMT_HDMI_360X480I;
		break;
	case HDMI_360x576i:
		fmt = TVIN_SIG_FMT_HDMI_360X576I;
		break;
	case HDMI_360x480p:
		fmt = TVIN_SIG_FMT_HDMI_360X480P;
		break;
	case HDMI_360x576p:
		fmt = TVIN_SIG_FMT_HDMI_360X576P;
		break;
	default:
		break;
	}
	return fmt;
}

bool rx_is_sig_ready(u8 port)
{
	if (rx[port].state == FSM_SIG_READY || force_vic)
		return true;
	else
		return false;
}

bool rx_is_nosig(u8 port)
{
	if (force_vic)
		return false;
	return rx[port].no_signal;
}

static bool rx_is_color_space_stable(u8 port)
{
	bool ret = true;

	if (rx_info.chip_id < CHIP_ID_T7)
		return ret;

	if (rx[port].pre.colorspace != rx[port].cur.colorspace &&
		rx_chk_avi_valid(port)) {
		ret = false;
		if (log_level & VIDEO_LOG)
			rx_pr("colorspace(%d=>%d),",
			      rx[port].pre.colorspace,
			      rx[port].cur.colorspace);
	}
	return ret;
}
/*
 * check timing info
 */
static bool rx_is_timing_stable(u8 port)
{
	bool ret = true;
	u32 ch0 = 0, ch1 = 0, ch2 = 0, ch3 = 0;

	if (stable_check_lvl & TMDS_VALID_EN) {
		if (!is_tmds_valid(port)) {
			ret = false;
			if (log_level & VIDEO_LOG)
				rx_pr("TMDS invalid\n");
		}
	}
	if (stable_check_lvl & HACTIVE_EN) {
		if (abs(rx[port].cur.hactive -
			rx[port].pre.hactive) > diff_pixel_th) {
			ret = false;
			if (log_level & VIDEO_LOG)
				rx_pr("hactive(%d=>%d),",
				      rx[port].pre.hactive,
				      rx[port].cur.hactive);
		}
	}
	if (stable_check_lvl & VACTIVE_EN) {
		if (abs(rx[port].cur.vactive -
			rx[port].pre.vactive) > diff_line_th) {
			ret = false;
			if (log_level & VIDEO_LOG)
				rx_pr("vactive(%d=>%d),",
				      rx[port].pre.vactive,
				      rx[port].cur.vactive);
		}
	}
	if (stable_check_lvl & HTOTAL_EN) {
		if (abs(rx[port].cur.htotal -
			rx[port].pre.htotal) > diff_pixel_th) {
			ret = false;
			if (log_level & VIDEO_LOG)
				rx_pr("htotal(%d=>%d),",
				      rx[port].pre.htotal,
				      rx[port].cur.htotal);
		}
	}
	if (stable_check_lvl & VTOTAL_EN) {
		if (abs(rx[port].cur.vtotal -
			rx[port].pre.vtotal) > diff_line_th) {
			ret = false;
			if (log_level & VIDEO_LOG)
				rx_pr("vtotal(%d=>%d),",
				      rx[port].pre.vtotal,
				      rx[port].cur.vtotal);
		}
	}
	if (stable_check_lvl & REFRESH_RATE_EN) {
		if (abs(rx[port].pre.frame_rate -
			rx[port].cur.frame_rate)
			> diff_frame_th) {
			ret = false;
			if (log_level & VIDEO_LOG)
				rx_pr("frame_rate(%d=>%d),",
				      rx[port].pre.frame_rate,
				      rx[port].cur.frame_rate);
		}
	}
	if (stable_check_lvl & REPEAT_EN) {
		if (rx[port].pre.repeat !=
			rx[port].cur.repeat &&
		    stable_check_lvl & REPEAT_EN) {
			ret = false;
			if (log_level & VIDEO_LOG)
				rx_pr("repeat(%d=>%d),",
				      rx[port].pre.repeat,
				      rx[port].cur.repeat);
		}
	}
	if (stable_check_lvl & DVI_EN) {
		if (rx[port].pre.hw_dvi !=
			rx[port].cur.hw_dvi) {
			ret = false;
			if (log_level & VIDEO_LOG)
				rx_pr("dvi(%d=>%d),",
				      rx[port].pre.hw_dvi,
				      rx[port].cur.hw_dvi);
			}
	}
	if (stable_check_lvl & INTERLACED_EN) {
		if (rx[port].pre.interlaced !=
			rx[port].cur.interlaced) {
			ret = false;
			if (log_level & VIDEO_LOG)
				rx_pr("interlaced(%d=>%d),",
				      rx[port].pre.interlaced,
				      rx[port].cur.interlaced);
		}
	}
	if (stable_check_lvl & COLOR_DEP_EN) {
		if (rx[port].pre.colordepth !=
			rx[port].cur.colordepth) {
			ret = false;
			if (log_level & VIDEO_LOG)
				rx_pr("colordepth(%d=>%d),",
				      rx[port].pre.colordepth,
				      rx[port].cur.colordepth);
			}
	}
	if (stable_check_lvl & ERR_CNT_EN) {
		rx_get_error_cnt(&ch0, &ch1, &ch2, &ch3, port);
		if ((ch0 + ch1 + ch2) > err_cnt_sum_max) {
			if (rx[port].var.sig_stable_err_cnt++ >
				sig_stable_err_max) {
				rx_pr("warning: more err counter\n");
				rx[port].var.sig_stable_err_cnt = 0;
				/*phy setting is fail, need reset phy*/
				rx[port].var.sig_unstable_cnt = sig_unstable_max;
				rx[port].clk.cable_clk = 0;
			}
			ret = false;
		}
	}
	if (stable_check_lvl & ECC_ERR_CNT_EN) {
		if (rx[port].cur.hdcp22_state == 3)
			rx[port].ecc_err = rx_get_ecc_err(port);

		if (rx[port].ecc_err) {
			ret = false;
			if (log_level & VIDEO_LOG)
				rx_pr("ecc err\n");
		}
	}
	if (!ret && log_level & VIDEO_LOG)
		rx_pr("\n");

	if (force_vic)
		ret = true;
	return ret;
}

static bool rx_fps_is_changed(u8 port)
{
	bool ret = true;

	if (abs(rx[port].pre.frame_rate -
		rx[port].cur.frame_rate)
		> diff_frame_th) {
		ret = false;
		if (log_level & VIDEO_LOG)
			rx_pr("frame_rate(%d=>%d),",
			rx[port].pre.frame_rate,
			rx[port].cur.frame_rate);
	}
	return ret;
}

static int get_fmt_with_colordepth(u8 port, u8 colordepth)
{
	int i, size, tmp_hactive, tmp_vactive;

	tmp_hactive = rx[port].pre.hactive * 8 / colordepth;
	size = sizeof(freq_ref) / sizeof(struct freq_ref_s);
	for (i = 0; i < size; i++) {
		if (abs(tmp_hactive - freq_ref[i].hactive) > diff_pixel_th)
			continue;
		if ((abs(rx[port].pre.vactive - freq_ref[i].vactive)) > diff_line_th)
			continue;
		if (rx[port].pre.colorspace != freq_ref[i].cd420 &&
			freq_ref[i].cd420 != 0 && freq_ref[i].cd420 != 4)
			continue;
		if (freq_ref[i].interlace != rx[port].pre.interlaced &&
			freq_ref[i].interlace != 0)
			continue;
		if (freq_ref[i].cd420 != 4 && rx[port].pre.htotal < 100)
			continue;
		if (freq_ref[i].cd420 == 4 && rx[port].pre.htotal > 960)
			continue;
		//if (freq_ref[i].type_3d != rx[port].vs_info_details._3d_structure)
			//continue;
		break;
	}
	if (i == size) {
		tmp_vactive = rx[port].pre.vactive * 8 / colordepth;
		for (i = 0; i < size; i++) {
			if (abs(rx[port].pre.hactive - freq_ref[i].hactive) > diff_pixel_th)
				continue;
			if ((abs(tmp_vactive - freq_ref[i].vactive)) > diff_line_th)
				continue;
			if (rx[port].pre.colorspace != freq_ref[i].cd420 &&
				freq_ref[i].cd420 != 0)
				continue;
			if (freq_ref[i].interlace != rx[port].pre.interlaced &&
				freq_ref[i].interlace != 0)
				continue;
			break;
		}
	}
	if (i == size) {
		tmp_hactive = rx[port].pre.hactive * 8 / colordepth;
		tmp_vactive = rx[port].pre.vactive * 8 / colordepth;
		for (i = 0; i < size; i++) {
			if (abs(tmp_hactive - freq_ref[i].hactive) > diff_pixel_th)
				continue;
			if ((abs(tmp_vactive - freq_ref[i].vactive)) > diff_line_th)
				continue;
			if (rx[port].pre.colorspace != freq_ref[i].cd420 &&
				freq_ref[i].cd420 != 0)
				continue;
			if (freq_ref[i].interlace != rx[port].pre.interlaced &&
				freq_ref[i].interlace != 0)
				continue;
			break;
		}
	}
	return i;
}

static void get_timing_fmt(u8 port)
{
	int i;
	int size = sizeof(freq_ref) / sizeof(struct freq_ref_s);

	rx[port].pre.sw_vic = HDMI_UNKNOWN;
	rx[port].pre.sw_dvi = 0;
	rx[port].pre.sw_fp = 0;
	rx[port].pre.sw_alternative = 0;

	if (force_vic) {
		rx[port].pre.sw_vic = force_vic;
		rx[port].pre.sw_dvi = 0;
		return;
	}
	i = get_fmt_with_colordepth(port, 8);
	if (i == size) {
		i = get_fmt_with_colordepth(port, 10);
		if (i == size)
			i = get_fmt_with_colordepth(port, 12);
		if (i != size) {
			if (rx[port].pre.hw_dvi && rx[port].tx_type == DEV_UNKNOWN)
				rx[port].tx_type = DEV_HDMI14;
			rx[port].pre.sw_vic = freq_ref[i].vic;
			rx[port].pre.sw_dvi = rx[port].pre.hw_dvi;
		} else {
			/* if format is not matched, sw_vic will be UNSUPPORT */
			rx[port].pre.sw_vic = HDMI_UNSUPPORT;
		}
	} else {
		if (freq_ref[i].cd420 == 4)  {
			rx_pr("dsc!!!\n");
			rx[port].dsc_flag = true;
		}
		rx[port].pre.sw_vic = freq_ref[i].vic;
		rx[port].pre.sw_dvi = rx[port].pre.hw_dvi;
	}
}

static void set_fsm_state(enum fsm_states_e sts, u8 port)
{
	rx[port].state = sts;
}

static void signal_status_init(u8 port)
{
	rx[port].no_signal = false;
	rx[port].wait_no_sig_cnt = 0;
	rx[port].var.hpd_wait_cnt = 0;
	rx[port].var.pll_unlock_cnt = 0;
	rx[port].var.pll_lock_cnt = 0;
	rx[port].var.sig_unstable_cnt = 0;
	rx[port].var.sig_stable_cnt = 0;
	rx[port].var.sig_unready_cnt = 0;
	rx[port].var.clk_chg_cnt = 0;
	/*rx[port].wait_no_sig_cnt = 0;*/
	/* rx[port].no_signal = false; */
	/* audio */
	rx[port].aud_info.real_sr = 0;
	rx[port].aud_info.coding_type = 0;
	rx[port].aud_info.channel_count = 0;
	rx[port].aud_info.aud_packet_received = 0;
	rx[port].aud_sr_stable_cnt = 0;
	rx[port].aud_sr_unstable_cnt = 0;
	//rx_aud_pll_ctl(0);
	rx_set_eq_run_state(E_EQ_START, port);
	rx[port].err_rec_mode = ERR_REC_EQ_RETRY;
	rx_irq_en(0, port);
	rx_esm_reset(2);
	//set_scdc_cfg(1, 0, port);
	rx[port].state = FSM_INIT;
	rx[port].fsm_ext_state = FSM_NULL;
	/*if (hdcp22_on)*/
		/*esm_set_stable(0);*/
	rx[port].hdcp.hdcp_version = HDCP_VER_NONE;
	rx[port].hdcp.hdcp_source = false;
	rx[port].hdcp.rpt_reauth_event = HDCP_VER_NONE;
	rx[port].hdcp.stream_type = 0;
	rx[port].skip = 0;
	rx[port].var.mute_cnt = 0;
	rx[port].var.de_stable = false;
	rx[port].vs_info_details.dv_allm = 0;
	rx[port].vs_info_details.hdmi_allm = 0;
	rx[port].cur.cn_type = 0;
	rx[port].cur.it_content = 0;
	rx[port].hdcp.stream_type = 0;
	latency_info.allm_mode = 0;
	latency_info.it_content = 0;
	latency_info.cn_type = 0;
	if (rx_info.chip_id >= CHIP_ID_T3X && port >= E_PORT2) {
		rx_set_frl_train_sts(E_FRL_TRAIN_START, port);
		rx[port].var.frl_rate = FRL_OFF;
	}
#ifdef CONFIG_AMLOGIC_HDMITX
	if (rx_info.chip_id == CHIP_ID_T7)
		hdmitx_update_latency_info(&latency_info);
#endif
}

//void packet_update(u8 port)
//{
	/*rx_getaudinfo(&rx[port].aud_info);*/
	//rgb_quant_range = rx[port].cur.rgb_quant_range;
	//yuv_quant_range = rx[port].cur.yuv_quant_range;
	//it_content = rx[port].cur.it_content;
	//auds_rcv_sts = rx[port].aud_info.aud_packet_received;
	//audio_sample_rate = rx[port].aud_info.real_sr;
	//audio_coding_type = rx[port].aud_info.coding_type;
	//audio_channel_count = rx[port].aud_info.channel_count;
//}

void hdmirx_hdcp22_reauth(void)
{
	if (hdcp22_reauth_enable) {
		esm_auth_fail_en = 1;
		hdcp22_auth_sts = 0xff;
	}
}

void monitor_hdcp22_sts(void)
{
	/*if the auth lost after the success of authentication*/
	if (hdcp22_capable_sts == HDCP22_AUTH_STATE_CAPABLE &&
	    (hdcp22_auth_sts == HDCP22_AUTH_STATE_LOST ||
	    hdcp22_auth_sts == HDCP22_AUTH_STATE_FAILED)) {
		hdmirx_hdcp22_reauth();
		/*rx_pr("\n auth lost force hpd rst\n");*/
	}
}

void rx_cor_reset(u8 port)
{
	u8 rst_lvl;

	/*
	 * hdcp14 sts only be cleared by
	 * 1. hdmi swreset
	 * 2. new AKSV is received
	 */
	//todo
	if (!rx_info.aml_phy.reset_pcs_en) {
		hdmirx_wr_top(TOP_SW_RESET, 0x280, port);
		hdmirx_wr_top(TOP_SW_RESET, 0, port);
	}

	if (rx[port].hdcp.hdcp_version == HDCP_VER_NONE)
	/* dishNXT box only send set_avmute, not clear_avmute
	 * we must clear hdcp avmute status here
	 * otherwise hdcp2.2 module does not work
	 */
		rst_lvl = 2;
	else
		rst_lvl = 1;

	if (log_level & VIDEO_LOG)
		rx_pr("%s-%d.\n", __func__, rst_lvl);
	rx_sw_reset(rst_lvl, port);
	rx_esm_reset(3);
	if (rx_info.chip_id >= CHIP_ID_T3X) {
		if (port >= E_PORT2) {
			if (rx[port].var.frl_rate) {
				hdmirx_wr_cor(DPLL_CFG6_DPLL_IVCRX, 0x0, port);
				/* for 2.1 hdcp */
				hdmirx_wr_cor(CP2PA_AESCTL0_HDCP2X_IVCRX, 0xf0, port);
				hdmirx_wr_bits_top(TOP_CLK_CNTL, _BIT(14), 1, port);
			} else {
				hdmirx_wr_cor(DPLL_CFG6_DPLL_IVCRX, 0x10, port);
				hdmirx_wr_cor(RX_H21_CTRL_PWD_IVCRX, 0, port);
				hdmirx_wr_bits_top(TOP_CLK_CNTL, _BIT(14), 0, port);
				hdmirx_wr_bits_cor(RX_VP_INPUT_FORMAT_LO, _BIT(0), 1, port);
				hdmirx_wr_cor(CP2PA_AESCTL0_HDCP2X_IVCRX, 0x0, port);
			}
		}
	}
}

bool rx_hpd_keep_low(u8 port)
{
	bool ret = false;

	if (rx[port].var.downstream_hpd_flag ||
		rx[rx_info.main_port].var.edid_update_flag) {
		if (rx[port].var.hpd_wait_cnt <= hpd_wait_max * 2)
			ret = true;
	} else {
		if (rx[port].var.hpd_wait_cnt <= hpd_wait_max)
			ret = true;
	}
	return ret;
}

int rx_get_cur_hpd_sts(u8 port)
{
	int tmp;

	tmp = hdmirx_rd_top_common(TOP_HPD_PWR5V) & (1 << port);
	tmp >>= port;
	return tmp;
}

void esm_set_reset(bool reset)
{
	if (log_level & HDCP_LOG)
		rx_pr("esm set reset:%d\n", reset);

	esm_reset_flag = reset;
}

void esm_set_stable(bool stable)
{
	if (log_level & HDCP_LOG)
		rx_pr("esm set stable:%d\n", stable);
	video_stable_to_esm = stable;
}

void esm_recovery(void)
{
	if (hdcp22_stop_auth_enable)
		hdcp22_stop_auth = 1;
	if (hdcp22_esm_reset2_enable)
		hdcp22_esm_reset2 = 1;
}

bool is_unnormal_format(u32 wait_cnt, u8 port)
{
	if (rx[port].pre.sw_vic == HDMI_UNSUPPORT ||
	    rx[port].pre.sw_vic == HDMI_UNKNOWN) {
		if (wait_cnt == sig_stable_max)
			rx_pr("*unsupport*\n");
		if (unnormal_wait_max == wait_cnt) {
			dump_state(RX_DUMP_VIDEO, port);
			return false;
		} else {
			return true;
		}
	}
	if (rx[port].pre.sw_dvi == 1) {
		if (wait_cnt == sig_stable_max)
			rx_pr("*DVI*\n");
		if (unnormal_wait_max == wait_cnt) {
			dump_state(RX_DUMP_VIDEO, port);
			return false;
		} else {
			return true;
		}
	}
	if (rx[port].pre.hdcp14_state != 3 &&
	    rx[port].pre.hdcp14_state != 0 &&
	    rx[port].hdcp.hdcp_version == HDCP_VER_14) {
		if (sig_stable_max == wait_cnt)
			rx_pr("hdcp14 unfinished\n");
		if (unnormal_wait_max == wait_cnt) {
			dump_state(RX_DUMP_HDCP, port);
			return false;
		} else {
			return true;
		}
	}
	if (rx[port].hdcp.hdcp_version == HDCP_VER_NONE &&
		rx[port].hdcp.hdcp_pre_ver != HDCP_VER_NONE) {
		if ((dev_is_apple_tv_v2 && wait_cnt >= hdcp_none_wait_max * 2) ||
			(!dev_is_apple_tv_v2 && wait_cnt >= hdcp_none_wait_max)) {
			dump_state(RX_DUMP_HDCP, port);
			return false;
		} else {
			if (log_level & VIDEO_LOG)
				rx_pr("hdcp waiting\n");
			return true;
		}
	}

	if (rx[port].hdcp.hdcp_version == HDCP_VER_22 && rx[port].cur.hdcp22_state == 0) {
		if (wait_cnt == (hdcp_none_wait_max * 2))
			return false;
		rx_pr("wait hdcp22 decrypt");
		return true;
	}

	rx_pr("unnormal wait cnt = %d\n", wait_cnt - sig_stable_max);
	return false;
}

void fsm_restart(u8 port)
{
	rx_esm_reset(2);
	hdmi_rx_top_edid_update();
	hdmirx_hw_config(port);
	if (rx[port].var.edid_update_flag)
		hdmirx_clr_scdc(true, port);
	hdmirx_audio_disabled(port);
	rx[port].var.vic_check_en = false;
	rx[port].var.dvi_check_en = true;
	rx[port].fsm_ext_state = FSM_INIT;
	rx[port].clk.cable_clk = 0;
	rx[port].phy.pll_rate = 0;
	rx[port].phy.phy_bw = 0;
	rx[port].phy.pll_bw = 0;
	rx_info.aml_phy.force_sqo = 0;
	i2c_err_cnt[port] = 0;
	rx_pr("force_fsm_init\n");
}

void dump_unnormal_info(u8 port)
{
	if (rx[port].pre.colorspace != rx[port].cur.colorspace)
		rx_pr("colorspace:%d-%d\n",
		      rx[port].pre.colorspace,
		      rx[port].cur.colorspace);
	if (rx[port].pre.colordepth != rx[port].cur.colordepth)
		rx_pr("colordepth:%d-%d\n",
		      rx[port].pre.colordepth,
		      rx[port].cur.colordepth);
	if (rx[port].pre.interlaced != rx[port].cur.interlaced)
		rx_pr("interlace:%d-%d\n",
		      rx[port].pre.interlaced,
		      rx[port].cur.interlaced);
	if (rx[port].pre.htotal != rx[port].cur.htotal)
		rx_pr("htotal:%d-%d\n",
		      rx[port].pre.htotal,
		      rx[port].cur.htotal);
	if (rx[port].pre.hactive != rx[port].cur.hactive)
		rx_pr("hactive:%d-%d\n",
		      rx[port].pre.hactive,
		      rx[port].cur.hactive);
	if (rx[port].pre.vtotal != rx[port].cur.vtotal)
		rx_pr("vtotal:%d-%d\n",
		      rx[port].pre.vtotal,
		      rx[port].cur.vtotal);
	if (rx[port].pre.vactive != rx[port].cur.vactive)
		rx_pr("vactive:%d-%d\n",
		      rx[port].pre.vactive,
		      rx[port].cur.vactive);
	if (rx[port].pre.repeat != rx[port].cur.repeat)
		rx_pr("repetition:%d-%d\n",
		      rx[port].pre.repeat,
		      rx[port].cur.repeat);
	if (rx[port].pre.frame_rate != rx[port].cur.frame_rate)
		rx_pr("frame_rate:%d-%d\n",
		      rx[port].pre.frame_rate,
		      rx[port].cur.frame_rate);
	if (rx[port].pre.hw_dvi != rx[port].cur.hw_dvi)
		rx_pr("dvi:%d-%d\n,",
		      rx[port].pre.hw_dvi,
		      rx[port].cur.hw_dvi);
	if (rx[port].pre.hdcp14_state != rx[port].cur.hdcp14_state)
		rx_pr("HDCP14 state:%d-%d\n",
		      rx[port].pre.hdcp14_state,
		      rx[port].cur.hdcp14_state);
	if (rx[port].pre.hdcp22_state != rx[port].cur.hdcp22_state)
		rx_pr("HDCP22 state:%d-%d\n",
		      rx[port].pre.hdcp22_state,
		      rx[port].cur.hdcp22_state);
}

void rx_send_hpd_pulse(u8 port)
{
	/*rx_set_cur_hpd(0);*/
	/*fsm_restart();*/
	rx[port].fsm_ext_state = FSM_HPD_LOW;
}

static void set_hdcp(struct hdmi_rx_hdcp *hdcp, const unsigned char *b_key)
{
	int i, j;
	/*memset(&init_hdcp_data, 0, sizeof(struct hdmi_rx_hdcp));*/
	for (i = 0, j = 0; i < 80; i += 2, j += 7) {
		hdcp->keys[i + 1] =
		    b_key[j] | (b_key[j + 1] << 8) | (b_key[j + 2] << 16) |
		    (b_key[j + 3] << 24);
		hdcp->keys[i + 0] =
		    b_key[j + 4] | (b_key[j + 5] << 8) | (b_key[j + 6] << 16);
	}
	hdcp->bksv[1] =
	    b_key[j] | (b_key[j + 1] << 8) | (b_key[j + 2] << 16) |
	    (b_key[j + 3] << 24);
	hdcp->bksv[0] = b_key[j + 4];
}

/*
 * func: hdmirx_fill_key_buf
 */
void hdmirx_fill_key_buf(const char *buf, int size)
{
	u8 port = rx_info.main_port;

	if (buf[0] == 'k' && buf[1] == 'e' && buf[2] == 'y') {
		set_hdcp(&rx[port].hdcp, buf + 3);
	} else {
		//memcpy(key_buf, buf, size);
		//key_size = size;
		//rx_pr("HDMIRX: fill key buf, size %d\n", size);
	}
	hdcp14_on = 1;
	rx_pr("HDMIRX: fill key buf, hdcp14_on %d\n", hdcp14_on);
}

/*
 *debug functions
 */
unsigned int hdmirx_hw_dump_reg(unsigned char *buf, int size)
{
	return 0;
}

void rx_get_global_variable(const char *buf)
{
	int i = 1;

	rx_pr("index %-30s   value\n", "variable");
	pr_var(dwc_rst_wait_cnt_max, i++);
	pr_var(spec_dev_wait_cnt_max, i++);
	pr_var(sig_stable_max, i++);
	pr_var(hpd_wait_max, i++);
	pr_var(sig_unstable_max, i++);
	pr_var(sig_unready_max, i++);
	pr_var(pow5v_max_cnt, i++);
	pr_var(diff_pixel_th, i++);
	pr_var(diff_line_th, i++);
	pr_var(diff_frame_th, i++);
	pr_var(force_vic, i++);
	pr_var(aud_sr_stb_max, i++);
	pr_var(pwr_sts_to_esm, i++);
	pr_var(log_level, i++);
	pr_var(rx5v_debug_en, i++);
	pr_var(irq_err_max, i++);
	pr_var(de_err_max, i++);
	pr_var(clk_unstable_max, i++);
	pr_var(clk_stable_max, i++);
	pr_var(wait_no_sig_max, i++);
	pr_var(vrr_func_en, i++);
	pr_var(allm_func_en, i++);
#ifdef CONFIG_AMLOGIC_HDMITX
	pr_var(receive_edid_len, i++);
#endif
	pr_var(edid_auto_sel, i++);
	//pr_var(hdcp_array_len, i++);
#ifdef CONFIG_AMLOGIC_HDMITX
	pr_var(hdcp_len, i++);
	pr_var(hdcp_repeat_depth, i++);
	pr_var(up_phy_addr, i++);
#endif
	pr_var(stable_check_lvl, i++);
	pr_var(hdcp22_reauth_enable, i++);
	pr_var(esm_recovery_mode, i++);
	pr_var(unnormal_wait_max, i++);
	pr_var(en_4k_2_2k, i++);
	pr_var(ops_port, i++);
	pr_var(en_4096_2_3840, i++);
	pr_var(en_4k_timing, i++);
	pr_var(acr_mode, i++);
	pr_var(force_clk_rate, i++);
	pr_var(rx_afifo_dbg_en, i++);
	pr_var(auto_aclk_mute, i++);
	pr_var(aud_avmute_en, i++);
	pr_var(rx_ecc_err_thres, i++);
	pr_var(rx_ecc_err_frames, i++);
	pr_var(aud_mute_sel, i++);
	pr_var(md_ists_en, i++);
	pr_var(pdec_ists_en, i++);
	pr_var(packet_fifo_cfg, i++);
	pr_var(pd_fifo_start_cnt, i++);
	pr_var(hdcp22_on, i++);
	pr_var(delay_ms_cnt, i++);
	pr_var(eq_max_setting, i++);
	pr_var(eq_dbg_ch0, i++);
	pr_var(eq_dbg_ch1, i++);
	pr_var(eq_dbg_ch2, i++);
	pr_var(edid_mode, i++);
	pr_var(phy_pddq_en, i++);
	pr_var(long_cable_best_setting, i++);
	pr_var(port_map, i++);
	pr_var(skip_frame_cnt, i++);
	pr_var(vdin_drop_frame_cnt, i++);
	pr_var(atmos_edid_update_hpd_en, i++);
	pr_var(suspend_pddq_sel, i++);
	pr_var(hdcp_none_wait_max, i++);
	pr_var(pll_unlock_max, i++);
	pr_var(esd_phy_rst_max, i++);
	pr_var(detect_time_min, i++);
	pr_var(vsync_err_cnt_max, i++);
	pr_var(ignore_sscp_charerr, i++);
	pr_var(ignore_sscp_tmds, i++);
	pr_var(err_chk_en, i++);
	pr_var(find_best_eq, i++);
	pr_var(eq_try_cnt, i++);
	pr_var(pll_rst_max, i++);
	pr_var(hdcp_enc_mode, i++);
	pr_var(hbr_force_8ch, i++);
	pr_var(cdr_lock_level, i++);
	pr_var(top_intr_maskn_value, i++);
	pr_var(pll_lock_max, i++);
	pr_var(clock_lock_th, i++);
	pr_var(en_take_dtd_space, i++);
	pr_var(earc_cap_ds_update_hpd_en, i++);
	pr_var(scdc_force_en, i++);
	pr_var(ddc_dbg_en, i++);
	pr_var(dbg_port, i++);
	pr_var(hdcp_hpd_ctrl_en, i++);
	pr_var(eq_dbg_lvl, i++);
	pr_var(edid_select, i++);
	pr_var(vpp_mute_enable, i++);
	pr_var(dbg_cs, i++);
	pr_var(dbg_pkt, i++);
	pr_var(rpt_edid_selection, i++);
	pr_var(vrr_range_dynamic_update_en, i++);
	pr_var(allm_update_en, i++);
	pr_var(phy_term_lel, i++);
	pr_var(vpcore_debug, i++);
	pr_var(tuning_cnt, i++);
	pr_var(frl_scrambler_en, i++);
	pr_var(frl_sync_cnt, i++);
	pr_var(phy_rate, i++);
	pr_var(odn_reg_n_mul, i++);
	pr_var(ext_cnt, i++);
	pr_var(tr_delay0, i++);
	pr_var(tr_delay1, i++);
	pr_var(force_clk_stable, i++);
	pr_var(audio_debug, i++);
	pr_var(port_debug_en, i++);
	pr_var(fpll_sel, i++);
	pr_var(fpll_chk_lvl, i++);
	pr_var(rx_info.aml_phy.hyper_gain_en, i++);
	pr_var(edid_reset_max, i++);
	pr_var(vdin_reset_pcs_en, i++);
	pr_var(rx_5v_wake_up_en, i++);
	pr_var(hdcp_22_en, i++);
	pr_var(phy_term_lel_t3x_21, i++);
	pr_var(fpll_ready_max, i++);
	pr_var(vpp_mute_cnt, i++);
	pr_var(gcp_mute_cnt, i++);
	pr_var(fps_unready_max, i++);
	pr_var(clk_msr_param, i++);
	pr_var(frl_debug_en, i++);
	pr_var(rx_emp_dbg_en, i++);
	pr_var(fsm_debug, i++);
	pr_var(rs_err_chk, i++);
	pr_var(err_cnt, i++);
	pr_var(vga_tuning_max, i++);
	pr_var(vga_tuning_min, i++);
	pr_var(ecc_err_monitor, i++);
	pr_var(rx_phy_level, i++);
	pr_var(color_bar_debug_en, i++);
	pr_var(color_bar_lvl, i++);
	pr_var(reset_pcs_flag, i++);
	pr_var(reset_pcs_cnt, i++);
	pr_var(pll_level, i++);
	pr_var(pll_level_en, i++);
	pr_var(vsvdb_update_hpd_en, i++);
	pr_var(clk_chg_max, i++);
	pr_var(reset_pcs_en, i++);
	pr_var(cts_ced_err_test, i++);
	pr_var(cal_phy_time, i++);
	pr_var(rx[E_PORT0].var.clk_stable_cnt, i++);
	pr_var(rx[E_PORT1].var.clk_stable_cnt, i++);
	pr_var(rx[E_PORT2].var.clk_stable_cnt, i++);
	pr_var(rx[E_PORT3].var.clk_stable_cnt, i++);
	pr_var(rx[E_PORT0].var.force_pattern, i++);
	pr_var(rx[E_PORT1].var.force_pattern, i++);
	pr_var(rx[E_PORT2].var.force_pattern, i++);
	pr_var(rx[E_PORT3].var.force_pattern, i++);
	pr_var(rx[E_PORT0].var.dbg_ve, i++);
	pr_var(rx[E_PORT1].var.dbg_ve, i++);
	pr_var(rx[E_PORT2].var.dbg_ve, i++);
	pr_var(rx[E_PORT3].var.dbg_ve, i++);
	pr_var(rx[E_PORT2].var.frl_rate, i++);
	pr_var(rx[E_PORT3].var.frl_rate, i++);
	/* phy var definition */
	pr_var(rx[E_PORT0].phy.aud_div, i++);
	pr_var(rx[E_PORT1].phy.aud_div, i++);
	pr_var(rx[E_PORT2].phy.aud_div, i++);
	pr_var(rx[E_PORT3].phy.aud_div, i++);
	pr_var(rx_info.aml_phy.tapx_value, i++);
	pr_var(rx_info.aml_phy.agc_enable, i++);
	pr_var(rx_info.aml_phy.afe_value, i++);
	pr_var(rx_info.aml_phy.dfe_value, i++);
	pr_var(rx_info.aml_phy.cdr_value, i++);
	pr_var(rx_info.aml_phy.eq_value, i++);
	pr_var(rx_info.aml_phy.misc1_value, i++);
	pr_var(rx_info.aml_phy.misc2_value, i++);
	pr_var(rx_info.aml_phy.phy_debug_en, i++);
	pr_var(rx_info.aml_phy.enhance_dfe_en_old, i++);
	pr_var(rx_info.aml_phy.eye_height, i++);
	pr_var(rx_info.aml_phy.eye_height_min, i++);
	pr_var(rx_info.aml_phy.enhance_eq, i++);
	pr_var(rx_info.aml_phy.eq_en, i++);
	pr_var(rx_info.aml_phy.eq_level, i++);
	pr_var(rx_info.aml_phy.cdr_retry_en, i++);
	pr_var(rx_info.aml_phy.cdr_retry_max, i++);
	pr_var(rx_info.aml_phy.sqrst_en, i++);
	pr_var(rx_info.aml_phy.vga_dbg, i++);
	pr_var(rx_info.aml_phy.dfe_en, i++);
	pr_var(rx_info.aml_phy.ofst_en, i++);
	pr_var(rx_info.aml_phy.cdr_mode, i++);
	pr_var(rx_info.aml_phy.pre_int_en, i++);
	pr_var(rx_info.aml_phy.pre_int, i++);
	pr_var(rx_info.aml_phy.phy_bwth, i++);
	pr_var(rx_info.aml_phy.vga_dbg_delay, i++);
	pr_var(rx_info.aml_phy.alirst_en, i++);
	pr_var(rx_info.aml_phy.tap1_byp, i++);
	pr_var(rx_info.aml_phy.eq_byp, i++);
	pr_var(rx_info.aml_phy.long_cable, i++);
	pr_var(rx_info.aml_phy.osc_mode, i++);
	pr_var(rx_info.aml_phy.pll_div, i++);
	pr_var(rx_info.aml_phy.eq_fix_val, i++);
	pr_var(rx_info.aml_phy.cdr_fr_en, i++);
	pr_var(rx_info.aml_phy.force_sqo, i++);
	pr_var(rx_info.aml_phy.os_rate, i++);
	pr_var(rx_info.aml_phy.vga_gain, i++);
	pr_var(rx_info.aml_phy.eq_stg1, i++);
	pr_var(rx_info.aml_phy.eq_stg2, i++);
	pr_var(rx_info.aml_phy.dfe_hold, i++);
	pr_var(rx_info.aml_phy.eq_hold, i++);
	pr_var(rx_info.aml_phy.eye_delay, i++);
	pr_var(rx_info.aml_phy.eq_retry, i++);
	pr_var(rx_info.aml_phy.tap2_byp, i++);
	pr_var(rx_info.aml_phy.long_bist_en, i++);
	pr_var(rx_info.aml_phy.reset_pcs_en, i++);
	pr_var(rx_info.aml_phy_21.phy_bwth, i++);
	pr_var(rx_info.aml_phy_21.vga_gain, i++);
	pr_var(rx_info.aml_phy_21.eq_stg1, i++);
	pr_var(rx_info.aml_phy_21.eq_stg2, i++);
	pr_var(rx_info.aml_phy_21.eq_pole, i++);
	pr_var(rx_info.aml_phy_21.buf_gain, i++);
	pr_var(rx_info.aml_phy_21.cdr_fr_en, i++);
	pr_var(rx_info.aml_phy_21.eq_hold, i++);
	pr_var(rx_info.aml_phy_21.eq_retry, i++);
	pr_var(rx_info.aml_phy_21.dfe_en, i++);
	pr_var(rx_info.aml_phy_21.dfe_hold, i++);
	pr_var(rx_info.aml_phy_21.ofst_en, i++);
	pr_var(rx_info.aml_phy_21.vga_tune, i++);
	pr_var(rx_info.aml_phy_21.pre_int, i++);
	pr_var(rx_info.aml_phy_21.pre_int_en, i++);
	pr_var(rx[E_PORT2].var.frl_rate, i++);
	pr_var(rx[E_PORT3].var.frl_rate, i++);
	pr_var(tuning_cnt, i++);
	pr_var(frl_scrambler_en, i++);
	pr_var(frl_sync_cnt, i++);
	pr_var(phy_rate, i++);
	pr_var(odn_reg_n_mul, i++);
	pr_var(ext_cnt, i++);
	pr_var(tr_delay0, i++);
	pr_var(tr_delay1, i++);
	pr_var(force_clk_stable, i++);
	pr_var(audio_debug, i++);
	pr_var(edid_auto_debug, i++);
	pr_var(port_debug_en, i++);
	pr_var(fpll_sel, i++);
	pr_var(fpll_chk_lvl, i++);
	pr_var(rx_info.aml_phy.hyper_gain_en, i++);
	pr_var(edid_reset_max, i++);
	pr_var(vdin_reset_pcs_en, i++);
	pr_var(rx_5v_wake_up_en, i++);
	pr_var(hdcp_22_en, i++);
	pr_var(phy_term_lel_t3x_21, i++);
	pr_var(fpll_ready_max, i++);
	pr_var(vpp_mute_cnt, i++);
	pr_var(gcp_mute_cnt, i++);
	pr_var(fps_unready_max, i++);
	pr_var(clk_msr_param, i++);
	pr_var(fsm_debug, i++);
	pr_var(rs_err_chk, i++);
	pr_var(err_cnt, i++);
	pr_var(vga_tuning_max, i++);
	pr_var(vga_tuning_min, i++);
	pr_var(ecc_err_monitor, i++);
}

bool str_cmp(unsigned char *buff, unsigned char *str)
{
	if (strlen(str) == strlen(buff) &&
	    strncmp(buff, str, strlen(str)) == 0)
		return true;
	else
		return false;
}

bool comp_set_pr_var(u8 *buff, u8 *var_str, void *var, u32 val)
{
	char index_c[5] = {'\0'};

	if (str_cmp(buff, var_str) || str_cmp((buff), (index_c))) {
		memcpy(var, &val, sizeof(val));
		return true;
	}
	return false;
}

bool set_pr_var(unsigned char *buff, unsigned char *str, void *var, u32 value)
{
	return comp_set_pr_var(buff, str, var, value);
}

int rx_set_global_variable(const char *buf, int size)
{
	char tmpbuf[60];
	int i = 0;
	u32 value = 0;
	int ret = 0;
	int index = 1;

	if (buf == 0 || size == 0 || size > 60)
		return -1;

	memset(tmpbuf, 0, sizeof(tmpbuf));
	while ((buf[i]) && (buf[i] != ',') && (buf[i] != ' ') &&
	       (buf[i] != '\n') && (i < size)) {
		tmpbuf[i] = buf[i];
		i++;
	}
	/*skip the space*/
	while (++i < size) {
		if ((buf[i] != ' ') && (buf[i] != ','))
			break;
	}
	if ((buf[i] == '0') && ((buf[i + 1] == 'x') || (buf[i + 1] == 'X')))
		ret = kstrtou32(buf + i + 2, 16, &value);
	else
		ret = kstrtou32(buf + i, 10, &value);
	if (ret != 0) {
		rx_pr("No value set:%d\n", ret);
		return -2;
	}

	if (set_pr_var(tmpbuf, var_to_str(dwc_rst_wait_cnt_max), &dwc_rst_wait_cnt_max, value))
		return pr_var(dwc_rst_wait_cnt_max, index);
	if (set_pr_var(tmpbuf, var_to_str(spec_dev_wait_cnt_max), &spec_dev_wait_cnt_max, value))
		return pr_var(spec_dev_wait_cnt_max, index);
	if (set_pr_var(tmpbuf, var_to_str(sig_stable_max), &sig_stable_max, value))
		return pr_var(sig_stable_max, index);
	if (set_pr_var(tmpbuf, var_to_str(hpd_wait_max), &hpd_wait_max, value))
		return pr_var(hpd_wait_max, index);
	if (set_pr_var(tmpbuf, var_to_str(log_level), &log_level, value))
		return pr_var(log_level, index);
	if (set_pr_var(tmpbuf, var_to_str(sig_unready_max), &sig_unready_max, value))
		return pr_var(sig_unready_max, index);
	if (set_pr_var(tmpbuf, var_to_str(pow5v_max_cnt), &pow5v_max_cnt, value))
		return pr_var(pow5v_max_cnt, index);
	if (set_pr_var(tmpbuf, var_to_str(diff_pixel_th), &diff_pixel_th, value))
		return pr_var(diff_pixel_th, index);
	if (set_pr_var(tmpbuf, var_to_str(diff_line_th), &diff_line_th, value))
		return pr_var(diff_line_th, index);
	if (set_pr_var(tmpbuf, var_to_str(diff_frame_th), &diff_frame_th, value))
		return pr_var(diff_frame_th, index);
	if (set_pr_var(tmpbuf, var_to_str(force_vic), &force_vic, value))
		return pr_var(force_vic, index);
	if (set_pr_var(tmpbuf, var_to_str(aud_sr_stb_max), &aud_sr_stb_max, value))
		return pr_var(aud_sr_stb_max, index);
	if (set_pr_var(tmpbuf, var_to_str(pwr_sts_to_esm), &pwr_sts_to_esm, value))
		return pr_var(pwr_sts_to_esm, index);
	if (set_pr_var(tmpbuf, var_to_str(rx5v_debug_en), &rx5v_debug_en, value))
		return pr_var(rx5v_debug_en, index);
	if (set_pr_var(tmpbuf, var_to_str(irq_err_max), &irq_err_max, value))
		return pr_var(irq_err_max, index);
	if (set_pr_var(tmpbuf, var_to_str(de_err_max), &de_err_max, value))
		return pr_var(de_err_max, index);
	if (set_pr_var(tmpbuf, var_to_str(clk_unstable_max), &clk_unstable_max, value))
		return pr_var(clk_unstable_max, index);
	if (set_pr_var(tmpbuf, var_to_str(clk_stable_max), &clk_stable_max, value))
		return pr_var(clk_stable_max, index);
	if (set_pr_var(tmpbuf, var_to_str(wait_no_sig_max), &wait_no_sig_max, value))
		return pr_var(wait_no_sig_max, index);
	if (set_pr_var(tmpbuf, var_to_str(vrr_func_en), &vrr_func_en, value))
		return pr_var(vrr_func_en, index);
	if (set_pr_var(tmpbuf, var_to_str(allm_func_en), &allm_func_en, value))
		return pr_var(allm_func_en, index);
#ifdef CONFIG_AMLOGIC_HDMITX
	if (set_pr_var(tmpbuf, var_to_str(receive_edid_len), &receive_edid_len, value))
		return pr_var(receive_edid_len, index);
#endif
	if (set_pr_var(tmpbuf, var_to_str(edid_auto_sel), &edid_auto_sel, value))
		return pr_var(edid_auto_sel, index);
#ifdef CONFIG_AMLOGIC_HDMITX
	if (set_pr_var(tmpbuf, var_to_str(hdcp_len), &hdcp_len, value))
		return pr_var(hdcp_len, index);
	if (set_pr_var(tmpbuf, var_to_str(hdcp_repeat_depth), &hdcp_repeat_depth, value))
		return pr_var(hdcp_repeat_depth, index);
	if (set_pr_var(tmpbuf, var_to_str(up_phy_addr), &up_phy_addr, value))
		return pr_var(up_phy_addr, index);
#endif
	if (set_pr_var(tmpbuf, var_to_str(stable_check_lvl), &stable_check_lvl, value))
		return pr_var(stable_check_lvl, index);
	if (set_pr_var(tmpbuf, var_to_str(hdcp22_reauth_enable), &hdcp22_reauth_enable, value))
		return pr_var(hdcp22_reauth_enable, index);
	if (set_pr_var(tmpbuf, var_to_str(esm_recovery_mode), &esm_recovery_mode, value))
		return pr_var(esm_recovery_mode, index);
	if (set_pr_var(tmpbuf, var_to_str(unnormal_wait_max), &unnormal_wait_max, value))
		return pr_var(unnormal_wait_max, index);
	if (set_pr_var(tmpbuf, var_to_str(edid_update_delay), &edid_update_delay, value))
		return pr_var(edid_update_delay, index);
	if (set_pr_var(tmpbuf, var_to_str(en_4k_2_2k), &en_4k_2_2k, value))
		return pr_var(en_4k_2_2k, index);
	if (set_pr_var(tmpbuf, var_to_str(en_4096_2_3840), &en_4096_2_3840, value))
		return pr_var(en_4096_2_3840, index);
	if (set_pr_var(tmpbuf, var_to_str(ops_port), &ops_port, value))
		return pr_var(ops_port, index);
	if (set_pr_var(tmpbuf, var_to_str(en_4k_timing), &en_4k_timing, value))
		return pr_var(en_4k_timing, index);
	if (set_pr_var(tmpbuf, var_to_str(acr_mode), &acr_mode, value))
		return pr_var(acr_mode, index);
	if (set_pr_var(tmpbuf, var_to_str(force_clk_rate), &force_clk_rate, value))
		return pr_var(force_clk_rate, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_afifo_dbg_en),
					&rx_afifo_dbg_en, value))
		return pr_var(rx_afifo_dbg_en, index);
	if (set_pr_var(tmpbuf, var_to_str(auto_aclk_mute), &auto_aclk_mute, value))
		return pr_var(auto_aclk_mute, index);
	if (set_pr_var(tmpbuf, var_to_str(aud_avmute_en), &aud_avmute_en, value))
		return pr_var(aud_avmute_en, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_ecc_err_thres),
		&rx_ecc_err_thres, value))
		return pr_var(rx_ecc_err_thres, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_ecc_err_frames),
		&rx_ecc_err_frames, value))
		return pr_var(rx_ecc_err_frames, index);
	if (set_pr_var(tmpbuf, var_to_str(aud_mute_sel), &aud_mute_sel, value))
		return pr_var(aud_mute_sel, index);
	if (set_pr_var(tmpbuf, var_to_str(md_ists_en), &md_ists_en, value))
		return pr_var(md_ists_en, index);
	if (set_pr_var(tmpbuf, var_to_str(pdec_ists_en), &pdec_ists_en, value))
		return pr_var(pdec_ists_en, index);
	if (set_pr_var(tmpbuf, var_to_str(packet_fifo_cfg), &packet_fifo_cfg, value))
		return pr_var(packet_fifo_cfg, index);
	if (set_pr_var(tmpbuf, var_to_str(pd_fifo_start_cnt), &pd_fifo_start_cnt, value))
		return pr_var(pd_fifo_start_cnt, index);
	if (set_pr_var(tmpbuf, var_to_str(hdcp22_on), &hdcp22_on, value))
		return pr_var(hdcp22_on, index);
	if (set_pr_var(tmpbuf, var_to_str(delay_ms_cnt), &delay_ms_cnt, value))
		return pr_var(delay_ms_cnt, index);
	if (set_pr_var(tmpbuf, var_to_str(eq_max_setting), &eq_max_setting, value))
		return pr_var(eq_max_setting, index);
	if (set_pr_var(tmpbuf, var_to_str(eq_dbg_ch0), &eq_dbg_ch0, value))
		return pr_var(eq_dbg_ch0, index);
	if (set_pr_var(tmpbuf, var_to_str(eq_dbg_ch1), &eq_dbg_ch1, value))
		return pr_var(eq_dbg_ch1, index);
	if (set_pr_var(tmpbuf, var_to_str(eq_dbg_ch2), &eq_dbg_ch2, value))
		return pr_var(eq_dbg_ch2, index);
	if (set_pr_var(tmpbuf, var_to_str(edid_mode), &edid_mode, value))
		return pr_var(edid_mode, index);
	if (set_pr_var(tmpbuf, var_to_str(phy_pddq_en), &phy_pddq_en, value))
		return pr_var(phy_pddq_en, index);
	if (set_pr_var(tmpbuf, var_to_str(long_cable_best_setting),
		&long_cable_best_setting, value))
		return pr_var(long_cable_best_setting, index);
	if (set_pr_var(tmpbuf, var_to_str(port_map), &port_map, value))
		return pr_var(port_map, index);
	if (set_pr_var(tmpbuf, var_to_str(skip_frame_cnt), &skip_frame_cnt, value))
		return pr_var(skip_frame_cnt, index);
	if (set_pr_var(tmpbuf, var_to_str(vdin_drop_frame_cnt), &vdin_drop_frame_cnt, value))
		return pr_var(vdin_drop_frame_cnt, index);
	if (set_pr_var(tmpbuf, var_to_str(atmos_edid_update_hpd_en),
		&atmos_edid_update_hpd_en, value))
		return pr_var(atmos_edid_update_hpd_en, index);
	if (set_pr_var(tmpbuf, var_to_str(suspend_pddq_sel), &suspend_pddq_sel, value))
		return pr_var(suspend_pddq_sel, index);
	if (set_pr_var(tmpbuf, var_to_str(hdcp_none_wait_max), &hdcp_none_wait_max, value))
		return pr_var(hdcp_none_wait_max, index);
	if (set_pr_var(tmpbuf, var_to_str(pll_unlock_max), &pll_unlock_max, value))
		return pr_var(pll_unlock_max, index);
	if (set_pr_var(tmpbuf, var_to_str(esd_phy_rst_max), &esd_phy_rst_max, value))
		return pr_var(esd_phy_rst_max, index);
	if (set_pr_var(tmpbuf, var_to_str(detect_time_min), &detect_time_min, value))
		return pr_var(detect_time_min, index);
	if (set_pr_var(tmpbuf, var_to_str(vsync_err_cnt_max), &vsync_err_cnt_max, value))
		return pr_var(vsync_err_cnt_max, index);
	if (set_pr_var(tmpbuf, var_to_str(ignore_sscp_charerr), &ignore_sscp_charerr, value))
		return pr_var(ignore_sscp_charerr, index);
	if (set_pr_var(tmpbuf, var_to_str(ignore_sscp_tmds), &ignore_sscp_tmds, value))
		return pr_var(ignore_sscp_tmds, index);
	if (set_pr_var(tmpbuf, var_to_str(err_chk_en), &err_chk_en, value))
		return pr_var(err_chk_en, index);
	if (set_pr_var(tmpbuf, var_to_str(find_best_eq), &find_best_eq, value))
		return pr_var(find_best_eq, index);
	if (set_pr_var(tmpbuf, var_to_str(eq_try_cnt), &eq_try_cnt, value))
		return pr_var(eq_try_cnt, index);
	if (set_pr_var(tmpbuf, var_to_str(pll_rst_max), &pll_rst_max, value))
		return pr_var(pll_rst_max, index);
	if (set_pr_var(tmpbuf, var_to_str(hdcp_enc_mode), &hdcp_enc_mode, value))
		return pr_var(hdcp_enc_mode, index);
	if (set_pr_var(tmpbuf, var_to_str(hbr_force_8ch), &hbr_force_8ch, value))
		return pr_var(hbr_force_8ch, index);
	if (set_pr_var(tmpbuf, var_to_str(cdr_lock_level), &cdr_lock_level, value))
		return pr_var(cdr_lock_level, index);
	if (set_pr_var(tmpbuf, var_to_str(top_intr_maskn_value), &top_intr_maskn_value, value))
		return pr_var(top_intr_maskn_value, index);
	if (set_pr_var(tmpbuf, var_to_str(pll_lock_max), &pll_lock_max, value))
		return pr_var(pll_lock_max, index);
	if (set_pr_var(tmpbuf, var_to_str(clock_lock_th), &clock_lock_th, value))
		return pr_var(clock_lock_th, index);
	if (set_pr_var(tmpbuf, var_to_str(en_take_dtd_space), &en_take_dtd_space, value))
		return pr_var(en_take_dtd_space, index);
	if (set_pr_var(tmpbuf, var_to_str(earc_cap_ds_update_hpd_en),
		&earc_cap_ds_update_hpd_en, value))
		return pr_var(earc_cap_ds_update_hpd_en, index);
	if (set_pr_var(tmpbuf, var_to_str(scdc_force_en), &scdc_force_en, value))
		return pr_var(scdc_force_en, index);
	if (set_pr_var(tmpbuf, var_to_str(ddc_dbg_en), &ddc_dbg_en, value))
		return pr_var(ddc_dbg_en, index);
	if (set_pr_var(tmpbuf, var_to_str(dbg_port), &dbg_port, value))
		return pr_var(dbg_port, index);
	if (set_pr_var(tmpbuf, var_to_str(hdcp_hpd_ctrl_en), &hdcp_hpd_ctrl_en, value))
		return pr_var(hdcp_hpd_ctrl_en, index);
	if (set_pr_var(tmpbuf, var_to_str(eq_dbg_lvl), &eq_dbg_lvl, value))
		return pr_var(eq_dbg_lvl, index);
	if (set_pr_var(tmpbuf, var_to_str(edid_select), &edid_select, value))
		return pr_var(edid_select, index);
	if (set_pr_var(tmpbuf, var_to_str(vpp_mute_enable), &vpp_mute_enable, value))
		return pr_var(vpp_mute_enable, index);
	if (set_pr_var(tmpbuf, var_to_str(reset_pcs_en), &reset_pcs_en, value))
		return pr_var(reset_pcs_en, index);
	if (set_pr_var(tmpbuf, var_to_str(vsvdb_update_hpd_en), &vsvdb_update_hpd_en, value))
		return pr_var(vsvdb_update_hpd_en, index);
	if (set_pr_var(tmpbuf, var_to_str(clk_chg_max), &clk_chg_max, value))
		return pr_var(clk_chg_max, index);
	if (set_pr_var(tmpbuf, var_to_str(dbg_cs), &dbg_cs, value))
		return pr_var(dbg_cs, index);
	if (set_pr_var(tmpbuf, var_to_str(dbg_pkt), &dbg_pkt, value))
		return pr_var(dbg_pkt, index);
	if (set_pr_var(tmpbuf, var_to_str(rpt_edid_selection),
	    &rpt_edid_selection, value))
		return pr_var(rpt_edid_selection, index);
	if (set_pr_var(tmpbuf, var_to_str(vrr_range_dynamic_update_en),
	    &vrr_range_dynamic_update_en, value))
		return pr_var(vrr_range_dynamic_update_en, index);
	if (set_pr_var(tmpbuf, var_to_str(allm_update_en),
	    &allm_update_en, value))
		return pr_var(allm_update_en, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_phy_level),
	    &rx_phy_level, value))
		return pr_var(rx_phy_level, index);
	if (set_pr_var(tmpbuf, var_to_str(color_bar_debug_en),
	    &color_bar_debug_en, value))
		return pr_var(color_bar_debug_en, index);
	if (set_pr_var(tmpbuf, var_to_str(color_bar_lvl),
	    &color_bar_lvl, value))
		return pr_var(color_bar_lvl, index);
	if (set_pr_var(tmpbuf, var_to_str(reset_pcs_flag),
	    &reset_pcs_flag, value))
		return pr_var(reset_pcs_flag, index);
	if (set_pr_var(tmpbuf, var_to_str(reset_pcs_cnt),
	    &reset_pcs_cnt, value))
		return pr_var(reset_pcs_cnt, index);
	if (set_pr_var(tmpbuf, var_to_str(pll_level_en),
	    &pll_level_en, value))
		return pr_var(pll_level_en, index);
	if (set_pr_var(tmpbuf, var_to_str(pll_level),
	    &pll_level, value))
		return pr_var(pll_level, index);
	if (set_pr_var(tmpbuf, var_to_str(vpcore_debug),
	    &vpcore_debug, value))
		return pr_var(vpcore_debug, index);
	if (set_pr_var(tmpbuf, var_to_str(audio_debug),
	    &audio_debug, value))
		return pr_var(audio_debug, index);
	if (set_pr_var(tmpbuf, var_to_str(phy_term_lel), &phy_term_lel, value))
		return pr_var(phy_term_lel, index);
	if (set_pr_var(tmpbuf, var_to_str(tuning_cnt), &tuning_cnt, value))
		return pr_var(tuning_cnt, index);
	if (set_pr_var(tmpbuf, var_to_str(sig_unstable_max), &sig_unstable_max, value))
		return pr_var(sig_unstable_max, index);
	if (set_pr_var(tmpbuf, var_to_str(frl_scrambler_en), &frl_scrambler_en, value))
		return pr_var(frl_scrambler_en, index);
	if (set_pr_var(tmpbuf, var_to_str(frl_sync_cnt), &frl_sync_cnt, value))
		return pr_var(frl_sync_cnt, index);
	if (set_pr_var(tmpbuf, var_to_str(port_debug_en), &port_debug_en, value))
		return pr_var(port_debug_en, index);
	if (set_pr_var(tmpbuf, var_to_str(audio_debug), &audio_debug, value))
		return pr_var(audio_debug, index);
	if (set_pr_var(tmpbuf, var_to_str(phy_rate), &phy_rate, value))
		return pr_var(phy_rate, index);
	if (set_pr_var(tmpbuf, var_to_str(odn_reg_n_mul), &odn_reg_n_mul, value))
		return pr_var(odn_reg_n_mul, index);
	if (set_pr_var(tmpbuf, var_to_str(ext_cnt), &ext_cnt, value))
		return pr_var(ext_cnt, index);
	if (set_pr_var(tmpbuf, var_to_str(tr_delay0), &tr_delay0, value))
		return pr_var(tr_delay0, index);
	if (set_pr_var(tmpbuf, var_to_str(tr_delay1), &tr_delay1, value))
		return pr_var(tr_delay1, index);
	if (set_pr_var(tmpbuf, var_to_str(force_clk_stable), &force_clk_stable, value))
		return pr_var(force_clk_stable, index);
	if (set_pr_var(tmpbuf, var_to_str(fpll_sel),
		&fpll_sel, value))
		return pr_var(fpll_sel, index);
	if (set_pr_var(tmpbuf, var_to_str(fpll_chk_lvl),
		&fpll_chk_lvl, value))
		return pr_var(fpll_chk_lvl, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.hyper_gain_en),
		&rx_info.aml_phy.hyper_gain_en, value))
		return pr_var(rx_info.aml_phy.hyper_gain_en, index);
	if (set_pr_var(tmpbuf, var_to_str(edid_reset_max),
		&edid_reset_max, value))
		return pr_var(edid_reset_max, index);
	if (set_pr_var(tmpbuf, var_to_str(vdin_reset_pcs_en),
		&vdin_reset_pcs_en, value))
		return pr_var(vdin_reset_pcs_en, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_5v_wake_up_en),
		&rx_5v_wake_up_en, value))
		return pr_var(rx_5v_wake_up_en, index);
	if (set_pr_var(tmpbuf, var_to_str(hdcp_22_en),
		&hdcp_22_en, value))
		return pr_var(hdcp_22_en, index);
	if (set_pr_var(tmpbuf, var_to_str(fpll_ready_max),
		&fpll_ready_max, value))
		return pr_var(fpll_ready_max, index);
	if (set_pr_var(tmpbuf, var_to_str(phy_term_lel_t3x_21),
		&phy_term_lel_t3x_21, value))
		return pr_var(phy_term_lel_t3x_21, index);
	if (set_pr_var(tmpbuf, var_to_str(vpp_mute_cnt),
		&vpp_mute_cnt, value))
		return pr_var(vpp_mute_cnt, index);
	if (set_pr_var(tmpbuf, var_to_str(gcp_mute_cnt),
		&gcp_mute_cnt, value))
		return pr_var(gcp_mute_cnt, index);
	if (set_pr_var(tmpbuf, var_to_str(fps_unready_max),
		&fps_unready_max, value))
		return pr_var(fps_unready_max, index);
	if (set_pr_var(tmpbuf, var_to_str(clk_msr_param),
		&clk_msr_param, value))
		return pr_var(clk_msr_param, index);
	if (set_pr_var(tmpbuf, var_to_str(fpll_clk_sel),
		&fpll_clk_sel, value))
		return pr_var(fpll_clk_sel, index);
	if (set_pr_var(tmpbuf, var_to_str(frl_debug_en),
		&frl_debug_en, value))
		return pr_var(frl_debug_en, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_emp_dbg_en),
		&rx_emp_dbg_en, value))
		return pr_var(rx_emp_dbg_en, index);
	if (set_pr_var(tmpbuf, var_to_str(fsm_debug),
		&fsm_debug, value))
		return pr_var(fsm_debug, index);
	if (set_pr_var(tmpbuf, var_to_str(rs_err_chk),
		&rs_err_chk, value))
		return pr_var(rs_err_chk, index);
	if (set_pr_var(tmpbuf, var_to_str(err_cnt),
		&err_cnt, value))
		return pr_var(err_cnt, index);
	if (set_pr_var(tmpbuf, var_to_str(vga_tuning_max),
		&vga_tuning_max, value))
		return pr_var(vga_tuning_max, index);
	if (set_pr_var(tmpbuf, var_to_str(vga_tuning_min),
		&vga_tuning_min, value))
		return pr_var(vga_tuning_min, index);
	if (set_pr_var(tmpbuf, var_to_str(ecc_err_monitor),
		&ecc_err_monitor, value))
		return pr_var(ecc_err_monitor, index);
	if (set_pr_var(tmpbuf, var_to_str(cts_ced_err_test),
		&cts_ced_err_test, value))
		return pr_var(cts_ced_err_test, index);
	if (set_pr_var(tmpbuf, var_to_str(cal_phy_time),
		&cal_phy_time, value))
		return pr_var(cal_phy_time, index);
	//fsm var
	if (set_pr_var(tmpbuf, var_to_str(rx[E_PORT0].var.dbg_ve),
		&rx[E_PORT0].var.dbg_ve, value))
		return pr_var(rx[E_PORT0].var.dbg_ve, index);
	if (set_pr_var(tmpbuf, var_to_str(rx[E_PORT1].var.dbg_ve),
		&rx[E_PORT1].var.dbg_ve, value))
		return pr_var(rx[E_PORT1].var.dbg_ve, index);
	if (set_pr_var(tmpbuf, var_to_str(rx[E_PORT2].var.dbg_ve),
		&rx[E_PORT2].var.dbg_ve, value))
		return pr_var(rx[E_PORT2].var.dbg_ve, index);
	if (set_pr_var(tmpbuf, var_to_str(rx[E_PORT3].var.dbg_ve),
		&rx[E_PORT3].var.dbg_ve, value))
		return pr_var(rx[E_PORT3].var.dbg_ve, index);
	if (set_pr_var(tmpbuf, var_to_str(rx[E_PORT0].var.clk_stable_cnt),
		&rx[E_PORT0].var.clk_stable_cnt, value))
		return pr_var(rx[E_PORT0].var.clk_stable_cnt, index);
	if (set_pr_var(tmpbuf, var_to_str(rx[E_PORT1].var.clk_stable_cnt),
		&rx[E_PORT1].var.clk_stable_cnt, value))
		return pr_var(rx[E_PORT1].var.clk_stable_cnt, index);
	if (set_pr_var(tmpbuf, var_to_str(rx[E_PORT2].var.clk_stable_cnt),
		&rx[E_PORT2].var.clk_stable_cnt, value))
		return pr_var(rx[E_PORT2].var.clk_stable_cnt, index);
	if (set_pr_var(tmpbuf, var_to_str(rx[E_PORT3].var.clk_stable_cnt),
		&rx[E_PORT3].var.clk_stable_cnt, value))
		return pr_var(rx[E_PORT3].var.clk_stable_cnt, index);
	if (set_pr_var(tmpbuf, var_to_str(rx[E_PORT0].var.force_pattern),
		&rx[E_PORT0].var.force_pattern, value))
		return pr_var(rx[E_PORT0].var.force_pattern, index);
	if (set_pr_var(tmpbuf, var_to_str(rx[E_PORT1].var.force_pattern),
		&rx[E_PORT1].var.force_pattern, value))
		return pr_var(rx[E_PORT1].var.force_pattern, index);
	if (set_pr_var(tmpbuf, var_to_str(rx[E_PORT2].var.force_pattern),
		&rx[E_PORT2].var.force_pattern, value))
		return pr_var(rx[E_PORT2].var.force_pattern, index);
	if (set_pr_var(tmpbuf, var_to_str(rx[E_PORT3].var.force_pattern),
		&rx[E_PORT3].var.force_pattern, value))
		return pr_var(rx[E_PORT3].var.force_pattern, index);
	if (set_pr_var(tmpbuf, var_to_str(rx[E_PORT2].var.frl_rate),
		&rx[E_PORT2].var.frl_rate, value))
		return pr_var(rx[E_PORT2].var.frl_rate, index);
	if (set_pr_var(tmpbuf, var_to_str(rx[E_PORT3].var.frl_rate),
		&rx[E_PORT3].var.frl_rate, value))
		return pr_var(rx[E_PORT3].var.frl_rate, index);
	//phy var
	if (set_pr_var(tmpbuf, var_to_str(rx[E_PORT0].phy.aud_div),
		&rx[E_PORT0].phy.aud_div, value))
		return pr_var(rx[E_PORT0].phy.aud_div, index);
	if (set_pr_var(tmpbuf, var_to_str(rx[E_PORT1].phy.aud_div),
		&rx[E_PORT1].phy.aud_div, value))
		return pr_var(rx[E_PORT1].phy.aud_div, index);
	if (set_pr_var(tmpbuf, var_to_str(rx[E_PORT2].phy.aud_div),
		&rx[E_PORT2].phy.aud_div, value))
		return pr_var(rx[E_PORT2].phy.aud_div, index);
	if (set_pr_var(tmpbuf, var_to_str(rx[E_PORT3].phy.aud_div),
		&rx[E_PORT3].phy.aud_div, value))
		return pr_var(rx[E_PORT3].phy.aud_div, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.tapx_value),
	    &rx_info.aml_phy.tapx_value, value))
		return pr_var(rx_info.aml_phy.tapx_value, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.agc_enable),
	    &rx_info.aml_phy.agc_enable, value))
		return pr_var(rx_info.aml_phy.agc_enable, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.afe_value),
	    &rx_info.aml_phy.afe_value, value))
		return pr_var(rx_info.aml_phy.afe_value, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.dfe_value),
	    &rx_info.aml_phy.dfe_value, value))
		return pr_var(rx_info.aml_phy.dfe_value, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.cdr_value),
	    &rx_info.aml_phy.cdr_value, value))
		return pr_var(rx_info.aml_phy.cdr_value, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.eq_value),
	    &rx_info.aml_phy.eq_value, value))
		return pr_var(rx_info.aml_phy.eq_value, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.misc1_value),
	    &rx_info.aml_phy.misc1_value, value))
		return pr_var(rx_info.aml_phy.misc1_value, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.misc2_value),
	    &rx_info.aml_phy.misc2_value, value))
		return pr_var(rx_info.aml_phy.misc2_value, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.phy_debug_en),
	    &rx_info.aml_phy.phy_debug_en, value))
		return pr_var(rx_info.aml_phy.phy_debug_en, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.enhance_dfe_en_old),
	    &rx_info.aml_phy.enhance_dfe_en_old, value))
		return pr_var(rx_info.aml_phy.enhance_dfe_en_old, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.enhance_dfe_en_new),
	    &rx_info.aml_phy.enhance_dfe_en_new, value))
		return pr_var(rx_info.aml_phy.enhance_dfe_en_new, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.eye_height),
	    &rx_info.aml_phy.eye_height, value))
		return pr_var(rx_info.aml_phy.eye_height, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.eye_height_min),
	    &rx_info.aml_phy.eye_height_min, value))
		return pr_var(rx_info.aml_phy.eye_height_min, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.enhance_eq),
	    &rx_info.aml_phy.enhance_eq, value))
		return pr_var(rx_info.aml_phy.enhance_eq, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.eq_en),
	    &rx_info.aml_phy.eq_en, value))
		return pr_var(rx_info.aml_phy.eq_en, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.eq_level),
	    &rx_info.aml_phy.eq_level, value))
		return pr_var(rx_info.aml_phy.eq_level, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.cdr_retry_en),
	    &rx_info.aml_phy.cdr_retry_en, value))
		return pr_var(rx_info.aml_phy.cdr_retry_en, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.cdr_retry_max),
	    &rx_info.aml_phy.cdr_retry_max, value))
		return pr_var(rx_info.aml_phy.cdr_retry_max, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.cdr_fr_en_auto),
	    &rx_info.aml_phy.cdr_fr_en_auto, value))
		return pr_var(rx_info.aml_phy.cdr_fr_en_auto, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.sqrst_en),
		&rx_info.aml_phy.sqrst_en, value))
		return pr_var(rx_info.aml_phy.sqrst_en, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.vga_dbg),
		&rx_info.aml_phy.vga_dbg, value))
		return pr_var(rx_info.aml_phy.vga_dbg, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.dfe_en),
		&rx_info.aml_phy.dfe_en, value))
		return pr_var(rx_info.aml_phy.dfe_en, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.ofst_en),
		&rx_info.aml_phy.ofst_en, value))
		return pr_var(rx_info.aml_phy.ofst_en, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.cdr_mode),
		&rx_info.aml_phy.cdr_mode, value))
		return pr_var(rx_info.aml_phy.cdr_mode, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.pre_int_en),
		&rx_info.aml_phy.pre_int_en, value))
		return pr_var(rx_info.aml_phy.pre_int_en, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.pre_int),
		&rx_info.aml_phy.pre_int, value))
		return pr_var(rx_info.aml_phy.pre_int, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.phy_bwth),
		&rx_info.aml_phy.phy_bwth, value))
		return pr_var(rx_info.aml_phy.phy_bwth, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.vga_dbg_delay),
		&rx_info.aml_phy.vga_dbg_delay, value))
		return pr_var(rx_info.aml_phy.vga_dbg_delay, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.alirst_en),
		&rx_info.aml_phy.alirst_en, value))
		return pr_var(rx_info.aml_phy.alirst_en, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.tap1_byp),
		&rx_info.aml_phy.tap1_byp, value))
		return pr_var(rx_info.aml_phy.tap1_byp, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.eq_byp),
		&rx_info.aml_phy.eq_byp, value))
		return pr_var(rx_info.aml_phy.eq_byp, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.long_cable),
		&rx_info.aml_phy.long_cable, value))
		return pr_var(rx_info.aml_phy.long_cable, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.osc_mode),
		&rx_info.aml_phy.osc_mode, value))
		return pr_var(rx_info.aml_phy.osc_mode, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.pll_div),
		&rx_info.aml_phy.pll_div, value))
		return pr_var(rx_info.aml_phy.pll_div, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.eq_fix_val),
		&rx_info.aml_phy.eq_fix_val, value))
		return pr_var(rx_info.aml_phy.eq_fix_val, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.cdr_fr_en),
		&rx_info.aml_phy.cdr_fr_en, value))
		return pr_var(rx_info.aml_phy.cdr_fr_en, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.force_sqo),
		&rx_info.aml_phy.force_sqo, value))
		return pr_var(rx_info.aml_phy.force_sqo, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.os_rate),
		&rx_info.aml_phy.os_rate, value))
		return pr_var(rx_info.aml_phy.os_rate, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.vga_gain),
		&rx_info.aml_phy.vga_gain, value))
		return pr_var(rx_info.aml_phy.vga_gain, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.eq_stg1),
		&rx_info.aml_phy.eq_stg1, value))
		return pr_var(rx_info.aml_phy.eq_stg1, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.eq_stg2),
		&rx_info.aml_phy.eq_stg2, value))
		return pr_var(rx_info.aml_phy.eq_stg2, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.dfe_hold),
		&rx_info.aml_phy.dfe_hold, value))
		return pr_var(rx_info.aml_phy.dfe_hold, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.eq_hold),
		&rx_info.aml_phy.eq_hold, value))
		return pr_var(rx_info.aml_phy.eq_hold, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.eye_delay),
		&rx_info.aml_phy.eye_delay, value))
		return pr_var(rx_info.aml_phy.eye_delay, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.eq_retry),
		&rx_info.aml_phy.eq_retry, value))
		return pr_var(rx_info.aml_phy.eq_retry, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.tap2_byp),
		&rx_info.aml_phy.tap2_byp, value))
		return pr_var(rx_info.aml_phy.tap2_byp, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.long_bist_en),
		&rx_info.aml_phy.long_bist_en, value))
		return pr_var(rx_info.aml_phy.long_bist_en, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy.reset_pcs_en),
		&rx_info.aml_phy.reset_pcs_en, value))
		return pr_var(rx_info.aml_phy.reset_pcs_en, index);
	if (set_pr_var(tmpbuf, var_to_str(phy_term_lel), &phy_term_lel, value))
		return pr_var(phy_term_lel, index);
	if (set_pr_var(tmpbuf, var_to_str(tuning_cnt), &tuning_cnt, value))
		return pr_var(tuning_cnt, index);
	if (set_pr_var(tmpbuf, var_to_str(sig_unstable_max), &sig_unstable_max, value))
		return pr_var(sig_unstable_max, index);
	if (set_pr_var(tmpbuf, var_to_str(rx[E_PORT2].var.frl_rate),
		&rx[E_PORT2].var.frl_rate, value))
		return pr_var(rx[E_PORT2].var.frl_rate, index);
	if (set_pr_var(tmpbuf, var_to_str(rx[E_PORT3].var.frl_rate),
		&rx[E_PORT3].var.frl_rate, value))
		return pr_var(rx[E_PORT3].var.frl_rate, index);
	if (set_pr_var(tmpbuf, var_to_str(frl_scrambler_en), &frl_scrambler_en, value))
		return pr_var(frl_scrambler_en, index);
	if (set_pr_var(tmpbuf, var_to_str(frl_sync_cnt), &frl_sync_cnt, value))
		return pr_var(frl_sync_cnt, index);
	if (set_pr_var(tmpbuf, var_to_str(port_debug_en), &port_debug_en, value))
		return pr_var(port_debug_en, index);
	if (set_pr_var(tmpbuf, var_to_str(audio_debug), &audio_debug, value))
		return pr_var(audio_debug, index);
	if (set_pr_var(tmpbuf, var_to_str(edid_auto_debug), &edid_auto_debug, value))
		return pr_var(edid_auto_debug, index);
	if (set_pr_var(tmpbuf, var_to_str(phy_rate), &phy_rate, value))
		return pr_var(phy_rate, index);
	if (set_pr_var(tmpbuf, var_to_str(odn_reg_n_mul), &odn_reg_n_mul, value))
		return pr_var(odn_reg_n_mul, index);
	if (set_pr_var(tmpbuf, var_to_str(ext_cnt), &ext_cnt, value))
		return pr_var(ext_cnt, index);
	if (set_pr_var(tmpbuf, var_to_str(tr_delay0), &tr_delay0, value))
		return pr_var(tr_delay0, index);
	if (set_pr_var(tmpbuf, var_to_str(tr_delay1), &tr_delay1, value))
		return pr_var(tr_delay1, index);
	if (set_pr_var(tmpbuf, var_to_str(force_clk_stable), &force_clk_stable, value))
		return pr_var(force_clk_stable, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy_21.phy_bwth),
		&rx_info.aml_phy_21.phy_bwth, value))
		return pr_var(rx_info.aml_phy_21.phy_bwth, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy_21.vga_gain),
		&rx_info.aml_phy_21.vga_gain, value))
		return pr_var(rx_info.aml_phy_21.vga_gain, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy_21.eq_stg1),
		&rx_info.aml_phy_21.eq_stg1, value))
		return pr_var(rx_info.aml_phy_21.eq_stg1, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy_21.eq_stg2),
		&rx_info.aml_phy_21.eq_stg2, value))
		return pr_var(rx_info.aml_phy_21.eq_stg2, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy_21.eq_pole),
		&rx_info.aml_phy_21.eq_pole, value))
		return pr_var(rx_info.aml_phy_21.eq_pole, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy_21.buf_gain),
		&rx_info.aml_phy_21.buf_gain, value))
		return pr_var(rx_info.aml_phy_21.buf_gain, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy_21.cdr_fr_en),
		&rx_info.aml_phy_21.cdr_fr_en, value))
		return pr_var(rx_info.aml_phy_21.cdr_fr_en, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy_21.eq_hold),
		&rx_info.aml_phy_21.eq_hold, value))
		return pr_var(rx_info.aml_phy_21.eq_hold, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy_21.eq_retry),
		&rx_info.aml_phy_21.eq_retry, value))
		return pr_var(rx_info.aml_phy_21.eq_retry, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy_21.dfe_en),
		&rx_info.aml_phy_21.dfe_en, value))
		return pr_var(rx_info.aml_phy_21.dfe_en, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy_21.vga_tune),
		&rx_info.aml_phy_21.vga_tune, value))
		return pr_var(rx_info.aml_phy_21.vga_tune, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy_21.dfe_hold),
		&rx_info.aml_phy_21.dfe_hold, value))
		return pr_var(rx_info.aml_phy_21.dfe_hold, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy_21.ofst_en),
		&rx_info.aml_phy_21.ofst_en, value))
		return pr_var(rx_info.aml_phy_21.ofst_en, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy_21.pre_int),
		&rx_info.aml_phy_21.pre_int, value))
		return pr_var(rx_info.aml_phy_21.pre_int, index);
	if (set_pr_var(tmpbuf, var_to_str(rx_info.aml_phy_21.pre_int_en),
		&rx_info.aml_phy_21.pre_int_en, value))
		return pr_var(rx_info.aml_phy_21.pre_int_en, index);
	return 0;
}

void skip_frame(unsigned int cnt, u8 port, char *str)
{
	if (rx[port].state != FSM_SIG_READY)
		return;

	rx[port].skip = get_frame_interval_cnt(cnt, port);
	rx_pr("rx[%d].skip = %d\n", port, rx[port].skip);
	if (log_level & VIDEO_LOG)
		rx_pr("skip reason is %s\n", str);
	//do not depend on state mechine condition
	tvin_notify_vdin_skip_frame(skip_frame_cnt, rx_get_port_type(port));
}

u8 get_frame_interval_cnt(u8 cnt, u8 port)
{
	u8 ret = 2 * cnt;

	if (rx[port].cur.frame_rate < FRAME_RATE_MAX * 100 &&
		rx[port].cur.frame_rate > FRAME_RATE_MIN * 100)
		ret = (1000 * 100 / rx[port].cur.frame_rate / 10) * cnt + 1;

	return ret;
}

void wait_ddc_idle(u8 port)
{
	unsigned char i;
	/* add delays to avoid the edid communication fail */
	for (i = 0; i <= 10; i++) {
		if (!is_ddc_idle(port))
			msleep(20);
	}
}

void hdmirx_open_main_port_t3x(u8 port)
{
	if (rx_info.main_port_open && port == rx_info.main_port)
		return;
	rx_info.main_port_open = true;
	rx_info.main_port = port;
	aml_phy_switch_port(port);
	rx_pr("%s:%d\n", __func__, port);
}

/***********************
 * hdmirx_open_port
 ***********************/
void hdmirx_open_main_port(u8 port)
{
	u32 fsmst = sm_pause;

	rx_info.main_port_open = true;
	rx_info.main_port = port;
	/* stop fsm when switch port */
	if (rx[port].resume_flag) {
		rx[port].resume_flag = false;
		return;
	}
	sm_pause = 1;
	rx[port].no_signal = false;
	rx[port].wait_no_sig_cnt = 0;
	if (hdmirx_repeat_support()) {
		if (pre_port != port)
			rx[port].hdcp.stream_type = 0;
		else if (rx[port].hdcp.hdcp_version == HDCP_VER_22)
			hdmitx_reauth_request(rx[port].hdcp.stream_type |
			UPSTREAM_ACTIVE);
	}
	if (rx_sw_scramble_en())
		force_clk_rate |= 0x10;
	else
		force_clk_rate &= ~(_BIT(4));
	if (rx_special_func_en(port)) {
		rx[port].state = FSM_HPD_HIGH;
	} else if (pre_port != rx_info.main_port ||
			   (rx_get_cur_hpd_sts(rx_info.main_port) == 0)) {
		hdmirx_clr_scdc(true, port);
		rx_esm_reset(1);
		if (rx[port].state > FSM_HPD_LOW)
			rx[port].state = FSM_HPD_LOW;
		wait_ddc_idle(port);
		rx_i2c_div_init();
		rx_set_cur_hpd(0, 0, port);
		/* need reset the whole module when switch port */
		if (need_update_edid(port))
			hdmi_rx_top_edid_update();
		//hdmirx_hw_config();
	} else {
		//aml_phy_switch_port(port);
		if (rx[port].state >= FSM_SIG_STABLE)
			rx[port].state = FSM_SIG_STABLE;
		else
			rx[port].state = FSM_HPD_LOW;
	}
	rx[port].var.edid_update_flag = 0;
	rx_pkt_initial(port);
	rx[port].fsm_ext_state = FSM_NULL;
	sm_pause = fsmst;
	rx[port].pre_state = rx[port].state;
	rx_pr("%s:%d\n", __func__, port);
}

void hdmirx_open_sub_port(u8 port)
{
	if (rx_info.sub_port != port) {
		rx_info.sub_port = port;
		aml_phy_switch_port(port);
	}
	rx_pr("%s:%d\n", __func__, port);
}

//void hdmirx_open_port(u8 main_port, u8 sub_port)
//{
	//if (port_debug_en)
		//rx_info.sub_port = rx_info.main_port;

	//if (main_port < rx_info.port_num) {
		//if (rx_info.chip_id == CHIP_ID_T3X)
			//hdmirx_open_main_port_t3x(main_port);
		//else
			//hdmirx_open_main_port(main_port);
	//}
	//if (rx_is_pip_on() && (sub_port == E_PORT0 || sub_port == E_PORT1))
		//hdmirx_open_sub_port(sub_port);
	//rx_pr("open port-main:%d, sub:%d\n", main_port, sub_port);
//}

void hdmirx_close_port(u8 port)
{
	if (port == rx_info.sub_port) {
		rx_info.pip_on = false;
		rx_info.sub_port = 0xff;//todo
		if (rx[port].cur_5v_sts) {
			if (rx[port].state >=  FSM_SIG_HOLD)
				rx[port].state = FSM_SIG_HOLD;
			else if (rx[port].state >=  FSM_WAIT_CLK_STABLE)
				rx[port].state = FSM_WAIT_CLK_STABLE;
		} else {
			rx[port].state = FSM_5V_LOST;
		}

	} else if (port == rx_info.main_port) {
		if (rx[port].cur_5v_sts) {
			if (rx[port].state >= FSM_SIG_HOLD)
				rx[port].state = FSM_SIG_HOLD;
			else if (rx[port].state >=  FSM_WAIT_CLK_STABLE)
				rx[port].state = FSM_WAIT_CLK_STABLE;
		} else {
			rx[port].state = FSM_5V_LOST;
		}
		rx_info.main_port_open = false;
		rx_aud_pll_ctl(0, port);
	}
	port_hpd_rst_flag |= (1 << port);
	rx[port].vs_info_details.hdmi_allm = 0;
	rx[port].cur.cn_type = 0;
	rx[port].cur.it_content = 0;
	rx[port].vsif_fmm_flag = false;

#ifdef CONFIG_AMLOGIC_HDMITX
	if (rx_info.chip_id == CHIP_ID_T7) {
		latency_info.allm_mode = 0;
		latency_info.it_content = 0;
		latency_info.cn_type = 0;
		hdmitx_update_latency_info(&latency_info);
	}
#endif
	/* clear vpp mute, such as after unplug */
	if (vpp_mute_enable) {
		if (get_video_mute_val(HDMI_RX_MUTE_SET) &&
			port != rx_info.sub_port)// && rx[port].vpp_mute)
			set_video_mute(HDMI_RX_MUTE_SET, false);
		//rx[port].vpp_mute = false;
	}
	rx_irq_en(0, port);
	if (hdmirx_repeat_support())
		hdmitx_reauth_request(UPSTREAM_INACTIVE);
}

void rx_nosig_monitor(u8 port)
{
	if (rx[port].cur_5v_sts == 0) {
		rx[port].no_signal = true;
	} else if (rx[port].state != FSM_SIG_READY &&
		rx[port].state != FSM_SIG_HOLD) {
		if (rx[port].wait_no_sig_cnt >= wait_no_sig_max) {
			rx[port].no_signal = true;
		} else {
			rx[port].wait_no_sig_cnt++;
			if (rx[port].no_signal)
				rx[port].no_signal = false;
		}
	} else {
		rx[port].wait_no_sig_cnt = 0;
		rx[port].no_signal = false;
	}
}

static void rx_cable_clk_monitor(u8 port)
{
	static bool pre_sts[4];

	if (rx[port].cur_5v_sts == 0)
		rx[port].cableclk_stb_flg = false;
	else
		rx[port].cableclk_stb_flg =
		is_clk_stable(port);

	if (pre_sts[port] != rx[port].cableclk_stb_flg) {
		pre_sts[port] = rx[port].cableclk_stb_flg;
		if (log_level & PHY_LOG)
			rx_pr("\n port %d clk_stb_changed to = %d\n", port, pre_sts[port]);
	}
}

/* inform hdcp_rx22 the 5v sts of rx to control delay time */
void rx_5v_sts_to_esm(unsigned int pwr)
{
	if (rx_info.chip_id >= CHIP_ID_T7)
		return;
	if (hdcp22_on) {
		if (!pwr)
			pwr_sts_to_esm = true;
		else
			pwr_sts_to_esm = false;
	}
}

/* ---------------------------------------------------------- */
/* func:         port A,B,C,D  hdmitx-5v monitor & HPD control */
/* note:         G9TV portD no used */
/* ---------------------------------------------------------- */
void rx_5v_monitor(void)
{
	static u8 check_cnt;
	u8 tmp_5v;
	u8 i;
	bool tmp_arc_5v;

	if (rx5v_debug_en)
		tmp_5v = 0x0f;
	else
		tmp_5v = rx_get_hdmi5v_sts();

	if (tmp_5v != pwr_sts)
		check_cnt++;

	for (i = 0; i < rx_info.port_num; i++) {
		if (rx_info.chip_id == CHIP_ID_T3X) {
			if (rx[i].cur_5v_sts == 0) {
				if (rx_get_hpd_sts(i) == 1)
					rx_set_cur_hpd(0, 5, i);
				if (!rx_is_phy_power_off(i))
					aml_phy_power_off_t3x(i);
			}
		}
	}
	if (check_cnt >= pow5v_max_cnt) {
		check_cnt = 0;
		pwr_sts = tmp_5v;
		rx_pr("Hotplug-0x%x\n", pwr_sts);
		hotplug_wait_query();
		if (cec_hdmirx5v_update)
			cec_hdmirx5v_update(pwr_sts);
		rx_5v_sts_to_esm(pwr_sts);
		for (i = 0; i < rx_info.port_num; i++) {
			if (rx[i].cur_5v_sts != ((pwr_sts >> i) & 1)) {
				rx[i].cur_5v_sts = (pwr_sts >> i) & 1;
				if (rx_info.chip_id == CHIP_ID_T3X) {
					if (rx[i].cur_5v_sts == 0) {
						set_fsm_state(FSM_5V_LOST, i);
						//rx_set_cur_hpd(0, 5, i);
						//aml_phy_power_off_t3x(i);
						//rx_cor_reset_t3x(i);
						rx[i].tx_type = DEV_UNKNOWN;
						rx_clr_edid_type(i);
						rx_edid_reset(i);
					}
				} else {
					if (rx[i].cur_5v_sts == 0) {
						set_fsm_state(FSM_5V_LOST, i);
						rx[i].tx_type = DEV_UNKNOWN;
						rx_clr_edid_type(i);
					}
				}
				if (hdmirx_repeat_support()) {
					rx[i].hdcp.stream_type = 0;
					hdmitx_reauth_request(UPSTREAM_INACTIVE);
				}
			}
			rx[i].spec_vendor_id = false;
		}
	}
	tmp_arc_5v = (pwr_sts >> rx_info.arc_port) & 1;
	if (earc_hdmirx_hpdst && rx_info.arc_5vsts != tmp_arc_5v) {
		rx_info.arc_5vsts = tmp_arc_5v;
		earc_hdmirx_hpdst(rx_info.arc_port, rx_info.arc_5vsts);
	}
}

/*
 * function:
 * for check error counter start for tl1
 *
 */
void rx_monitor_error_cnt_start(u8 port)
{
	rx[port].phy.timestap = ktime_get_real_seconds();//get_seconds();
}

static void rx_get_ecc_err_cnt(u8 port)
{
	u32 ecc_err;

	/* enable ecc err read, bit0:enable and clear*/
	hdmirx_wr_cor(RX_ECC_CTRL_DP2_IVCRX, _BIT(0), port);
	udelay(1);
	ecc_err = hdmirx_rd_cor(RX_BCH_ERR_DP2_IVCRX, port) |
			(((u32)hdmirx_rd_cor(RX_BCH_ERR2_DP2_IVCRX, port)     & 0x7f) << 8);
	if (ecc_err)
		if (log_level & FRL_LOG)
			rx_pr("ecc_err:0x%x-0x%x-%d\n",
				hdmirx_rd_cor(RX_BCH_ERR_DP2_IVCRX, port),
				hdmirx_rd_cor(RX_BCH_ERR2_DP2_IVCRX, port),
				ecc_err);
	hdmirx_wr_cor(RX_ECC_CTRL_DP2_IVCRX, 0x0, port);
}

void rx_get_rs_err_cnt(u8 port)
{
	u32 rs_err, rs_err_po, ecc_fixed_lsb, rs_cnt;
	u32 rs_err_fixed;
	u32 data;

	rs_err = hdmirx_rd_cor(SCDCS_RS_CORR_L_SCDC_IVCRX, port) |
			(((u32)hdmirx_rd_cor(SCDCS_RS_CORR_H_SCDC_IVCRX, port)     & 0x7f) << 8);
	/* 0x1532 bit1 0->1 pulse to read */
	data = hdmirx_rd_cor(H21RXSB_CTRL1_M42H_IVCRX, port);
	hdmirx_wr_cor(H21RXSB_CTRL1_M42H_IVCRX, data & (~(_BIT(1))), port);
	udelay(1);
	hdmirx_wr_cor(H21RXSB_CTRL1_M42H_IVCRX, data | _BIT(1), port);

	/* read 0x1533 before read rs error */
	ecc_fixed_lsb = hdmirx_rd_cor(H21RXSB_ECC_CNT_FIXED_LSB_RS0_M42H_IVCRX, port);
	rs_cnt = hdmirx_rd_cor(H21RXSB_RS_CNT_LSB_M42H_IVCRX, port) |
		(((u32)hdmirx_rd_cor(H21RXSB_RS_CNT_MSB_M42H_IVCRX, port) & 0x7f) << 8);
	rs_err_po = hdmirx_rd_cor(H21RXSB_RS_ERR_CNT_PO_LSB_M42H_IVCRX, port) |
		(((u32)hdmirx_rd_cor(H21RXSB_RS_ERR_CNT_PO_MSB_M42H_IVCRX, port) & 0x7f) << 8);
	rs_err_fixed = hdmirx_rd_cor(H21RXSB_RSERRCNT_FIXED_LSB_M42H_IVCRX, port) |
		((u32)hdmirx_rd_cor(H21RXSB_RSERRCNT_FIXED_LSB1_M42H_IVCRX, port) << 8) |
		((u32)hdmirx_rd_cor(H21RXSB_RSERRCNT_FIXED_MSB_M42H_IVCRX, port) << 16);
	if (rs_err) {
		if (log_level & FRL_LOG) {
			rx_pr("rs_err:%d, rs_cnt:%d\n", rs_err, rs_cnt);
			rx_pr("rs_err_last:%d--rs_err_po(%d)-rs_err_fixed(%d)\n",
				rs_err_po - rs_err_fixed, rs_err_po, rs_err_fixed);
		}
		hdmirx_wr_bits_cor(H21RXSB_RST_CTRL_M42H_IVCRX, _BIT(0), 0x1, port);
		udelay(1);
		hdmirx_wr_bits_cor(H21RXSB_RST_CTRL_M42H_IVCRX, _BIT(0), 0x0, port);
	}
	/* 0x1532 bit0 1->0 clear cnt */
	data = hdmirx_rd_cor(H21RXSB_CTRL1_M42H_IVCRX, port) & (~(_BIT(0)));
	hdmirx_wr_cor(H21RXSB_CTRL1_M42H_IVCRX,     data, port);
	udelay(1);
	hdmirx_wr_cor(H21RXSB_CTRL1_M42H_IVCRX,     data | _BIT(0), port);
}

/*
 * function:
 *	1min error counter check for tl1 aml phy
 */
void rx_monitor_error_counter(u8 port)
{
	ulong timestap;
	u32 ch0, ch1, ch2, ch3;

	if (rx_info.chip_id < CHIP_ID_TL1)
		return;

	timestap = ktime_get_real_seconds();//get_seconds();

	if ((timestap - rx[port].phy.timestap) > 1) {
		rx[port].phy.timestap = timestap;
		rx_get_error_cnt(&ch0, &ch1, &ch2, &ch3, port);
		if (ch0 || ch1 || ch2 || ch3)
			rx_pr("err cnt:%d,%d,%d,%d\n", ch0, ch1, ch2, ch3);
		if (rs_err_chk)
			rx_get_rs_err_cnt(port);
		rx_get_ecc_err_cnt(port);
	}
}

char *fsm_st[] = {
	"FSM_5V_LOST",
	"FSM_INIT",
	"FSM_HPD_LOW",
	"FSM_HPD_HIGH",
	"FSM_COR_RST",
	"FSM_FRL_FLT_READY",
	"FSM_WAIT_SIG",
	"FLT_RX_LTS_3",
	"FLT_RX_LTS_P",
	"FSM_FRL_TRN",
	"FSM_WAIT_FRL_TRN_DONE",
	"FSM_WAIT_CLK_STABLE",
	"FSM_EQ_START",
	"FSM_WAIT_EQ_DONE",
	"FSM_PCS_RESET",
	"FSM_SIG_UNSTABLE",
	"FSM_SIG_WAIT_STABLE",
	"FSM_SIG_HOLD",
	"FSM_SIG_STABLE",
	"FSM_SIG_STABLE_TO_READY",
	"FSM_SIG_READY",
	"FSM_NULL",
};

static void hdcp22_decrypt_monitor(u8 port)
{
	if (!hdcp22_on)
		return;

	if (rx[port].cur.hdcp_type) {
		if (rx[port].last_hdcp22_state !=
			rx[port].cur.hdcp22_state) {
			if (rx[port].state == FSM_SIG_READY)
				skip_frame(skip_frame_cnt, port, "hdcp22_decrypt");
			if (log_level & VIDEO_LOG)
				rx_pr("hdcp22 decrypt chg(%d->%d)\n",
				      rx[port].last_hdcp22_state,
				      rx[port].cur.hdcp22_state);
			rx[port].last_hdcp22_state =
				rx[port].cur.hdcp22_state;
		}
	} else {
		rx[port].last_hdcp22_state = 0;
	}
}

/*
 * FUNC: rx_main_state_machine
 * signal detection main process
 */
void rx_main_state_machine(void)
{
	u8 port = rx_info.main_port;
	u8 chk_cnt;

	switch (rx[port].state) {
	case FSM_5V_LOST:
		if (rx[port].cur_5v_sts)
			rx[port].state = FSM_INIT;
		fsm_restart(port);
		break;
	case FSM_HPD_LOW:
		/* disable irq before hpd low */
		rx_irq_en(0, port);
		rx_set_cur_hpd(0, 0, port);
		rx[port].state = FSM_INIT;
		break;
	case FSM_INIT:
		if (rx[port].cur_5v_sts == 0)
			break;
		signal_status_init(port);
		rx[port].clk.cable_clk = 0;
		rx[port].state = FSM_HPD_HIGH;
		break;
	case FSM_HPD_HIGH:
		if (rx[port].cur_5v_sts == 0)
			break;
		rx[port].var.hpd_wait_cnt++;
		if (rx_get_cur_hpd_sts(port) == 0) {
			if (rx_hpd_keep_low(port))
				break;
			hdmirx_hw_config(port);
		} else if (pre_port != port) {
			hdmirx_hw_config(port);
		}
		rx[port].var.hpd_wait_cnt = 0;
		rx[port].var.clk_unstable_cnt = 0;
		rx[port].var.esd_phy_rst_cnt = 0;
		rx[port].var.downstream_hpd_flag = 0;
		rx[port].var.edid_update_flag = 0;
		pre_port = port;
		rx_set_cur_hpd(1, 0, port);
		rx[port].clk.cable_clk = 0;
		rx[port].phy.cablesel = 0;
		hdmirx_clr_scdc(false, port);
		/* rx[port].hdcp.hdcp_version = HDCP_VER_NONE; */
		rx[port].state = FSM_WAIT_CLK_STABLE;
		break;
	case FSM_WAIT_CLK_STABLE:
		if (rx[port].cur_5v_sts == 0)
			break;
		if (rx[port].cableclk_stb_flg && !rx[port].ddc_filter_en) {
			if (rx[port].var.clk_unstable_cnt != 0)
				rx_pr("wait clk cnt %d\n", rx[port].var.clk_unstable_cnt);
			if (rx[port].var.clk_stable_cnt < clk_stable_max) {
				rx[port].var.clk_stable_cnt++;
				break;
			}
			rx[port].state = FSM_EQ_START;
			rx[port].var.clk_stable_cnt = 0;
			rx[port].var.clk_unstable_cnt = 0;
			rx_pr("clk stable=%d\n", rx[port].clk.cable_clk);
			rx[port].var.de_stable = false;
			rx_irq_en(IRQ_EN_HDCP, port);
		} else {
			rx[port].var.clk_stable_cnt = 0;
			if (rx[port].var.clk_unstable_cnt < clk_unstable_max) {
				rx[port].var.clk_unstable_cnt++;
				break;
			}
			rx[port].var.clk_unstable_cnt = 0;
			if (rx[port].var.esd_phy_rst_cnt < esd_phy_rst_max &&
				!rx[port].ddc_filter_en) {
				if (edid_auto_debug &&
					rx[port].edid_type.cfg == EDID_AUTO20 &&
					rx[port].var.esd_phy_rst_cnt == edid_auto_chk_max &&
					rx[port].tx_type == DEV_UNKNOWN)
					rx[port].tx_type = DEV_HDMI14;
				hdmirx_phy_init(port);
				rx[port].clk.cable_clk = 0;
				rx[port].var.esd_phy_rst_cnt++;
			} else {
				if (!rx[port].resume_flag && port != rx_info.arc_port &&
					rx[port].ddc_filter_en) {
					rx[port].state = FSM_HPD_LOW;
					rx_i2c_err_monitor(port);
					hdmi_rx_top_edid_update();
				}
				rx[port].ddc_filter_en = false;
				rx[port].var.esd_phy_rst_cnt = 0;
				break;
			}
		}
		break;
	case FSM_EQ_START:
		if (!rx[port].cableclk_stb_flg) {
			rx[port].state = FSM_WAIT_CLK_STABLE;
			break;
		}
		rx_run_eq(port);
		rx[port].state = FSM_WAIT_EQ_DONE;
		break;
	case FSM_WAIT_EQ_DONE:
		if (!rx[port].cableclk_stb_flg) {
			rx[port].state = FSM_WAIT_CLK_STABLE;
			break;
		}
		if (rx_eq_done(port)) {
			if (rx_info.aml_phy.reset_pcs_en) {
				rx[port].state = FSM_PCS_RESET;
				break;
			}
			rx[port].state = FSM_SIG_UNSTABLE;
			rx[port].var.pll_lock_cnt = 0;
			rx[port].var.pll_unlock_cnt = 0;
			rx[port].var.clk_chg_cnt = 0;
			//esd_phy_rst_cnt = 0;
		}
		break;
	case FSM_PCS_RESET:
		reset_pcs(port);
		rx[port].state = FSM_SIG_UNSTABLE;
		rx[port].var.pll_lock_cnt = 0;
		rx[port].var.pll_unlock_cnt = 0;
		rx[port].var.clk_chg_cnt = 0;
		break;
	case FSM_SIG_UNSTABLE:
		if (!rx[port].cableclk_stb_flg) {
			rx[port].state = FSM_WAIT_CLK_STABLE;
			break;
		}
		if (is_tmds_valid(port)) {
			/* pll_unlock_cnt = 0; */
			if (++rx[port].var.pll_lock_cnt < pll_lock_max)
				break;
			rx_cor_reset(port);
			//hdmirx_output_en(true);
			//rx_irq_en(IRQ_EN_ALL, port);
			//hdmirx_top_irq_en(1, 1, port);
			rx[port].state = FSM_SIG_WAIT_STABLE;
		} else {
			rx[port].var.pll_lock_cnt = 0;
			if (rx[port].var.pll_unlock_cnt < pll_unlock_max) {
				rx[port].var.pll_unlock_cnt++;
				break;
			}
			if (rx[port].err_rec_mode == ERR_REC_EQ_RETRY) {
				rx[port].state = FSM_WAIT_CLK_STABLE;
				if (rx_sw_scramble_en() &&
					force_clk_rate > 1) {
					if (force_clk_rate & 1)
						force_clk_rate = 0x10;
					else
						force_clk_rate = 0x11;
					rx_pr("force_clk=%x\n", force_clk_rate);
				}
				if (rx[port].var.esd_phy_rst_cnt < esd_phy_rst_max) {
					rx[port].var.esd_phy_rst_cnt++;
					rx[port].phy.cablesel++;
					//rx[port].clk.cable_clk = 0;
					//hdmirx_phy_init();
				} else {
					rx[port].var.esd_phy_rst_cnt = 0;
					rx[port].err_rec_mode = ERR_REC_HPD_RST;
				}
			} else if (rx[port].err_rec_mode == ERR_REC_HPD_RST &&
				port != rx_info.arc_port && rx[port].ddc_filter_en) {
				rx_set_cur_hpd(0, 2, port);
				rx[port].clk.cable_clk = 0;
				rx[port].state = FSM_INIT;
				rx[port].err_rec_mode = ERR_REC_EQ_RETRY;
			}
			rx_set_eq_run_state(E_EQ_START, port);
		}
		break;
	case FSM_SIG_WAIT_STABLE:
		if (!rx[port].cableclk_stb_flg) {
			rx[port].state = FSM_WAIT_CLK_STABLE;
			break;
		}
		rx[port].var.dwc_rst_wait_cnt++;
		if (rx[port].var.dwc_rst_wait_cnt < dwc_rst_wait_cnt_max)
			break;
		rx[port].var.edid_update_flag = 0;
		rx[port].var.dwc_rst_wait_cnt = 0;
		rx[port].var.sig_stable_cnt = 0;
		rx[port].var.sig_unstable_cnt = 0;
		rx[port].var.sig_stable_err_cnt = 0;
		rx[port].var.clk_chg_cnt = 0;
		reset_pcs(port);
		rx_pkt_initial(port);
		rx[port].state = FSM_SIG_STABLE;
		break;
	case FSM_SIG_STABLE:
		if (!rx[port].cableclk_stb_flg) {
			rx[port].state = FSM_WAIT_CLK_STABLE;
			break;
		}
		memcpy(&rx[port].pre, &rx[port].cur, sizeof(struct rx_video_info));
		rx_get_video_info(port);
		if (rx_is_timing_stable(port)) {
			if (rx[port].var.sig_stable_cnt == sig_stable_max / 2)
				rx_irq_en(IRQ_EN_ALL, port);
			if (++rx[port].var.sig_stable_cnt >= sig_stable_max) {
				get_timing_fmt(port);
				/* timing stable, start count vsync and avi pkt */
				rx[port].var.de_stable = true;
				rx[port].var.sig_unstable_cnt = 0;
				if (is_unnormal_format(rx[port].var.sig_stable_cnt, port))
					break;
				/* if format vic is abnormal, do hw
				 * reset once to try to recover.
				 */
				if (fmt_vic_abnormal(port)) {
					if (rx[port].var.vic_check_en) {
						/* hdmi_rx_top_edid_update(); */
						hdmirx_hw_config(port);
						rx[port].state = FSM_HPD_LOW;
						rx[port].var.vic_check_en = false;
					} else {
						rx[port].state = FSM_WAIT_CLK_STABLE;
						rx_set_eq_run_state(E_EQ_START, port);
						rx[port].var.vic_check_en = true;
					}
					break;
				}
				rx[port].var.sig_unready_cnt = 0;
				/* if DVI signal is detected, then try
				 * hpd reset once to recovery, to avoid
				 * recognition to DVI of low probability
				 */
				if (rx[port].pre.sw_dvi && rx[port].var.dvi_check_en &&
					is_ddc_filter_en(port) &&
				    rx[port].hdcp.hdcp_version == HDCP_VER_NONE) {
					rx[port].state = FSM_HPD_LOW;
					rx_i2c_div_init();
					 rx[port].var.dvi_check_en = false;
					break;
				}
				rx[port].var.sig_stable_cnt = 0;
				rx[port].skip = 0;
				rx[port].var.mute_cnt = 0;
				rx[port].state = FSM_SIG_READY;
				rx[port].aud_sr_stable_cnt = 0;
				rx[port].aud_sr_unstable_cnt = 0;
				rx[port].no_signal = false;
				rx[port].ecc_err = 0;
				rx[port].var.clk_chg_cnt = 0;
				/*memset(&rx[port].aud_info, 0,*/
					/*sizeof(struct aud_info_s));*/
				/*rx_set_eq_run_state(E_EQ_PASS);*/
				hdmirx_config_video(port);
				rx_get_aud_info(&rx[port].aud_info, port);
				hdmirx_config_audio(port);
				rx_aud_pll_ctl(1, port);
				rx_afifo_store_valid(true, port);
				hdmirx_audio_fifo_rst(port);
				rx[port].hdcp.hdcp_pre_ver = rx[port].hdcp.hdcp_version;
				rx[port].stable_timestamp = rx_info.timestamp;
				rx_pr("Sig ready\n");
				dump_state(RX_DUMP_VIDEO, port);
				#ifdef K_TEST_CHK_ERR_CNT
				rx_monitor_error_cnt_start(port);
				#endif
				rx[port].var.sig_stable_err_cnt = 0;
				rx[port].var.esd_phy_rst_cnt = 0;
				//reset_pcs_flag = 1;
				rx[port].ddc_filter_en = false;
			}
		} else {
			rx_irq_en(0, port);
			rx[port].var.sig_stable_cnt = 0;
			rx[port].var.de_stable = false;
			if (rx[port].var.sig_unstable_cnt < sig_unstable_max) {
				rx[port].var.sig_unstable_cnt++;
				if (log_level & PHY_LOG)
					rx_pr("DE not stable\n");
				if (rx_phy_level & 0x1 &&
					(rx[port].var.sig_unstable_cnt >= reset_pcs_flag &&
					rx[port].var.sig_unstable_cnt <=
					reset_pcs_flag + reset_pcs_cnt)) {
					reset_pcs(port);
					//reset_pcs_flag = 0;
					if (log_level & PHY_LOG)
						rx_pr("reset pcs\n");
				}
				break;
			}
			if (rx[port].err_rec_mode == ERR_REC_EQ_RETRY) {
				rx[port].state = FSM_WAIT_CLK_STABLE;
				rx[port].phy.cablesel++;
				if (rx[port].var.esd_phy_rst_cnt++ >= esd_phy_rst_max) {
					rx[port].err_rec_mode = ERR_REC_HPD_RST;
					rx_set_eq_run_state(E_EQ_START, port);
					rx[port].var.esd_phy_rst_cnt = 0;
				}
			} else if (rx[port].err_rec_mode == ERR_REC_HPD_RST &&
				port != rx_info.arc_port && rx[port].ddc_filter_en) {
				rx_set_cur_hpd(0, 2, port);
				rx[port].clk.cable_clk = 0;
				rx[port].state = FSM_INIT;
				rx[port].err_rec_mode = ERR_REC_EQ_RETRY;
			}
		}
		break;
	case FSM_SIG_READY:
		rx_get_video_info(port);
		rx[port].err_rec_mode = ERR_REC_EQ_RETRY;
		#ifdef K_TEST_CHK_ERR_CNT
		rx_monitor_error_counter(port);
		#endif
		/* video info change */
		if (!is_tmds_valid(port)) {
			if (video_mute_enabled(port)) {
				set_video_mute(HDMI_RX_MUTE_SET, true);
				rx_mute_vpp(rx_get_port_type(port));
				rx[port].vpp_mute = true;
				rx[port].var.mute_cnt = 0;
				if (log_level & 0x100)
					rx_pr("vpp mute\n");
			}
			skip_frame(skip_frame_cnt, port, "fsm tmds skip");
			rx[port].unready_timestamp = rx_info.timestamp;
			rx_i2c_div_init();
			dump_unnormal_info(port);
			rx_pr("tmds_invalid-->unready\n");
			rx[port].var.de_stable = false;
			rx[port].var.sig_unready_cnt = 0;
			rx[port].aud_info.real_sr = 0;
			rx_aud_pll_ctl(0, port);
			rx[port].hdcp.hdcp_pre_ver = rx[port].hdcp.hdcp_version;
			/* need to clr to none, for dishNXT box */
			rx[port].hdcp.hdcp_version = HDCP_VER_NONE;
			//rx_sw_reset(2);
			rx_irq_en(0, port);
			hdmirx_output_en(false);
			rx[port].state = FSM_WAIT_CLK_STABLE;
			rx[port].var.vic_check_en = false;
			rx[port].skip = 0;
			rx[port].var.mute_cnt = 0;
			rx[port].aud_sr_stable_cnt = 0;
			rx[port].aud_sr_unstable_cnt = 0;
			rx[port].clk.cable_clk = 0;
			rx[port].hdcp.stream_type = 0;
			rx[port].var.esd_phy_rst_cnt = 0;
			rx_esm_reset(0);
			break;
		} else if (!rx_is_timing_stable(port)) {
			skip_frame(skip_frame_cnt, port, "fsm timing skip");
			if (sig_unready_max)
				chk_cnt = sig_unready_max;
			else
				chk_cnt = get_frame_interval_cnt(1, port);
			if (++rx[port].var.sig_unready_cnt >= chk_cnt) {
				/*sig_lost_lock_cnt = 0;*/
				rx[port].unready_timestamp = rx_info.timestamp;
				rx_i2c_div_init();
				dump_unnormal_info(port);
				rx_pr("timing unstable-->unready\n");
				rx[port].var.de_stable = false;
				rx[port].var.sig_unready_cnt = 0;
				rx[port].aud_info.real_sr = 0;
				rx_aud_pll_ctl(0, port);
				rx[port].hdcp.hdcp_pre_ver = rx[port].hdcp.hdcp_version;
				/* need to clr to none, for dishNXT box */
				rx[port].hdcp.hdcp_version = HDCP_VER_NONE;
				//rx_sw_reset(2);
				rx_irq_en(0, port);
				hdmirx_output_en(false);
				rx[port].state = FSM_WAIT_CLK_STABLE;
				rx[port].var.vic_check_en = false;
				rx[port].skip = 0;
				rx[port].var.mute_cnt = 0;
				rx[port].aud_sr_stable_cnt = 0;
				rx[port].aud_sr_unstable_cnt = 0;
				rx[port].clk.cable_clk = 0;
				rx[port].hdcp.stream_type = 0;
				rx[port].var.esd_phy_rst_cnt = 0;
				rx_esm_reset(0);
				break;
			}
		} else if (!rx_is_color_space_stable(port)) {
			//Color space changes, no need to do EQ training
			skip_frame(skip_frame_cnt, port, "fsm color skip");
			if (video_mute_enabled(port)) {
				set_video_mute(HDMI_RX_MUTE_SET, true);
				rx_mute_vpp(rx_get_port_type(port));
				rx[port].vpp_mute = true;
				rx[port].var.mute_cnt = 0;
				if (log_level & 0x100)
					rx_pr("vpp mute1\n");
			}
			if (sig_unready_max)
				chk_cnt = sig_unready_max;
			else
				chk_cnt = get_frame_interval_cnt(1, port);
			if (++rx[port].var.sig_unready_cnt >= chk_cnt) {
				/*sig_lost_lock_cnt = 0;*/
				rx[port].unready_timestamp = rx_info.timestamp;
				rx_i2c_div_init();
				rx_pr("colorspace changes from %d to %d\n",
					  rx[port].pre.colorspace, rx[port].cur.colorspace);
				rx[port].var.de_stable = false;
				rx[port].hdcp.hdcp_pre_ver = rx[port].hdcp.hdcp_version;
				/* need to clr to none, for dishNXT box */
				hdmirx_output_en(false);
				rx[port].state = FSM_SIG_WAIT_STABLE;
				break;
			}
		} else if (!rx_fps_is_changed(port) && gcp_mute_flag[port] &&
		!rx[port].free_sync_sts && !rx[port].vtem_info.vrr_en) {
			if (++rx[port].var.fps_unready_cnt >= fps_unready_max) {
				rx[port].gcp_mute_cnt = gcp_mute_cnt;
				rx[port].pre.frame_rate = rx[port].cur.frame_rate;
				rx[port].var.fps_unready_cnt = 0;
				gcp_mute_flag[port] = 0;
				break;
			}
		} else {
			rx[port].var.sig_unready_cnt = 0;
			if (rx[port].skip > 0) {
				rx[port].skip--;
			} else if (vpp_mute_enable) {
				if (rx[port].gcp_mute_cnt > 0) {
					rx[port].gcp_mute_cnt--;
					break;
				}
				if (rx[port].vpp_mute_cnt > 0) {
					rx[port].vpp_mute_cnt--;
					break;
				}
				/* clear vpp mute after signal stable */
				if (get_video_mute_val(HDMI_RX_MUTE_SET) &&
					port != rx_info.sub_port) {
					if (rx[port].var.mute_cnt++ <
						get_frame_interval_cnt(1, port))
						break;
					rx[port].var.mute_cnt = 0;
					rx[port].vpp_mute = false;
					set_video_mute(HDMI_RX_MUTE_SET, false);
				}
			}
		}
		if (rx[port].pre.sw_dvi == 1)
			break;
		//packet_update(port);
		rx_get_aud_info(&rx[port].aud_info, port);

		if (check_real_sr_change(port))
			rx_audio_pll_sw_update();

		if (is_aud_pll_error()) {
			rx[port].aud_sr_unstable_cnt++;
			if (rx[port].aud_sr_unstable_cnt > aud_sr_stb_max) {
				unsigned int aud_sts = rx_get_aud_pll_err_sts(port);

				if (aud_sts == E_REQUESTCLK_ERR) {
					hdmirx_phy_init(port);
					rx[port].state = FSM_WAIT_CLK_STABLE;
					/*timing sw at same FRQ*/
					rx[port].clk.cable_clk = 0;
					rx_pr("reqclk err->wait_clk\n");
				} else if (aud_sts == E_PLLRATE_CHG) {
					rx_aud_pll_ctl(1, port);
				} else if (aud_sts == E_AUDCLK_ERR) {
					rx_audio_bandgap_rst();
				} else {
					rx_acr_info_sw_update();
					rx_audio_pll_sw_update();
				}
				if (log_level & AUDIO_LOG)
					rx_pr("update audio-err:%d\n", aud_sts);
				rx[port].aud_sr_unstable_cnt = 0;
			}
		} else if (is_aud_fifo_error()) {
			rx[port].aud_sr_unstable_cnt++;
			if (rx[port].aud_sr_unstable_cnt > aud_sr_stb_max) {
				hdmirx_audio_fifo_rst(port);
				rx[port].aud_sr_unstable_cnt = 0;
			}
		} else {
			rx[port].aud_sr_unstable_cnt = 0;
		}
		break;
	default:
		break;
	}
	/* for fsm debug */
	if (rx[port].state != rx[port].pre_state) {
		if (log_level & LOG_EN)
			rx_pr("fsm_main (%s) to (%s)\n",
			      fsm_st[rx[port].pre_state],
			      fsm_st[rx[port].state]);
		rx[port].pre_state = rx[port].state;
	}
}

void rx_port0_main_state_machine(void)
{
	int chk_cnt;
	u8 port = E_PORT0;

	if ((dbg_port - 1 != port) &&
		dbg_port)
		return;

	rx_cable_clk_monitor(port);
	switch (rx[port].state) {
	case FSM_5V_LOST:
		if (rx[port].cur_5v_sts)
			rx[port].state = FSM_INIT;
		fsm_restart(port);
		break;
	case FSM_HPD_LOW:
		/* disable irq before hpd low */
		rx_irq_en(0, port);
		rx_set_cur_hpd(0, 0, port);
		rx[port].state = FSM_INIT;
		break;
	case FSM_INIT:
		signal_status_init(port);
		rx[port].clk.cable_clk = 0;
		rx[port].state = FSM_HPD_HIGH;
		break;
	case FSM_HPD_HIGH:
		if (rx[port].cur_5v_sts == 0)
			break;
		rx[port].var.hpd_wait_cnt++;
		if (rx_get_cur_hpd_sts(port) == 0) {
			if (rx_hpd_keep_low(port))
				break;
			//hdmirx_hw_config(port);//todo
		}
		rx[port].var.hpd_wait_cnt = 0;
		rx[port].var.clk_unstable_cnt = 0;
		//rx[port].var.esd_phy_rst_cnt = 0;
		rx[port].var.downstream_hpd_flag = 0;
		rx[port].var.edid_update_flag = 0;
		rx_set_cur_hpd(1, 0, port);
		rx[port].last_sw_vic = 0;
		memset(&rx[port].pre, 0, sizeof(struct rx_video_info));
		memset(&rx[port].cur, 0, sizeof(struct rx_video_info));
		rx[port].clk.cable_clk = 0;
		rx[port].phy.cablesel = 0;
		/* rx[port].hdcp.hdcp_version = HDCP_VER_NONE; */
		rx[port].state = FSM_WAIT_CLK_STABLE;
		break;
	case FSM_WAIT_CLK_STABLE:
		if (rx[port].cur_5v_sts == 0)
			break;
		if (rx[port].cableclk_stb_flg) {
			if (rx[port].var.clk_unstable_cnt != 0) {
				if (rx[port].var.clk_stable_cnt < clk_stable_max) {
					rx[port].var.clk_stable_cnt++;
					break;
				}
				rx_pr("wait clk cnt %d\n", rx[port].var.clk_unstable_cnt);
			}
			rx[port].stable_timestamp = rx_info.timestamp;
			rx[port].state = FSM_EQ_START;
			rx[port].var.clk_stable_cnt = 0;
			rx[port].var.clk_unstable_cnt = 0;
			rx_pr("clk stable=%d\n", rx[port].clk.cable_clk);
			rx_irq_en(IRQ_EN_HDCP, port);//todo
		} else {
			rx[port].var.clk_stable_cnt = 0;
			if (rx[port].var.clk_unstable_cnt < clk_unstable_max) {
				rx[port].var.clk_unstable_cnt++;
				break;
			}
			rx[port].var.clk_unstable_cnt = 0;
			hdmirx_phy_init(port);
			rx[port].clk.cable_clk = 0;
		}
		break;
	case FSM_EQ_START:
		if (!rx[port].cableclk_stb_flg) {
			rx[port].state = FSM_WAIT_CLK_STABLE;
			break;
		}
		rx_run_eq(port);
		rx[port].state = FSM_WAIT_EQ_DONE;
		break;
	case FSM_WAIT_EQ_DONE:
		if (!rx[port].cableclk_stb_flg) {
			rx[port].state = FSM_WAIT_CLK_STABLE;
			break;
		}
		if (rx_eq_done(port)) {
			rx[port].state = FSM_PCS_RESET;
			rx[port].var.pll_lock_cnt = 0;
			rx[port].var.pll_unlock_cnt = 0;
			rx[port].var.clk_chg_cnt = 0;

		}
		break;
	case FSM_PCS_RESET:
		reset_pcs(port);
		rx[port].state = FSM_SIG_UNSTABLE;
		rx[port].var.pll_lock_cnt = 0;
		rx[port].var.pll_unlock_cnt = 0;
		rx[port].var.clk_chg_cnt = 0;
		break;
	case FSM_SIG_UNSTABLE:
		if (!rx[port].cableclk_stb_flg) {
			rx[port].state = FSM_WAIT_CLK_STABLE;
			break;
		}
		if (is_tmds_valid(port)) {
			/* pll_unlock_cnt = 0; */
			if (++rx[port].var.pll_lock_cnt < pll_lock_max)
				break;
			//rx_irq_en(true, port);
			//hdmirx_top_irq_en(1, 1, port);
			rx[port].state = FSM_SIG_WAIT_STABLE;
		} else {
			rx[port].var.pll_lock_cnt = 0;
			if (rx[port].var.pll_unlock_cnt < pll_unlock_max) {
				rx[port].var.pll_unlock_cnt++;
				break;
			}
			rx[port].state = FSM_WAIT_CLK_STABLE;
		}
		break;
	case FSM_SIG_WAIT_STABLE:
		if (!rx[port].cableclk_stb_flg) {
			rx[port].state = FSM_WAIT_CLK_STABLE;
			break;
		}
		rx[port].var.dwc_rst_wait_cnt++;
		if (rx[port].var.dwc_rst_wait_cnt < dwc_rst_wait_cnt_max)
			break;
		rx[port].var.edid_update_flag = 0;
		rx[port].var.dwc_rst_wait_cnt = 0;
		rx[port].var.sig_stable_cnt = 0;
		rx[port].var.sig_unstable_cnt = 0;
		rx[port].var.sig_stable_err_cnt = 0;
		rx[port].var.clk_chg_cnt = 0;
		reset_pcs(port);
		rx_pkt_initial(port);
		rx[port].state = FSM_SIG_HOLD;
		break;
	case FSM_SIG_HOLD:    //todo
		if (!rx[port].cableclk_stb_flg) {
			rx[port].state = FSM_WAIT_CLK_STABLE;
			break;
		}
		if (port != rx_info.main_port &&
			port != rx_info.sub_port)
			break;
		rx[port].state = FSM_SIG_STABLE;
		break;
	case FSM_SIG_STABLE:
		if (!rx[port].cableclk_stb_flg) {
			rx[port].state = FSM_WAIT_CLK_STABLE;
			break;
		}
		memcpy(&rx[port].pre, &rx[port].cur, sizeof(struct rx_video_info));
		rx_get_video_info(port);
		if (rx_is_timing_stable(port)) {
			if (++rx[port].var.sig_stable_cnt >= sig_stable_max) {
				rx_irq_en(IRQ_EN_ALL, port);
				get_timing_fmt(port);
				rx[port].var.de_stable = true;
				rx[port].var.sig_unstable_cnt = 0;
				rx[port].var.sig_unready_cnt = 0;
				/* if DVI signal is detected, then try
				 * hpd reset once to recovery, to avoid
				 * recognition to DVI of low probability
				 */
				if (rx[port].pre.sw_dvi &&  rx[port].var.dvi_check_en &&
				    rx[port].hdcp.hdcp_version == HDCP_VER_NONE) {
					rx[port].state = FSM_HPD_LOW;
					rx[port].var.dvi_check_en = false;
					break;
				}
				if (rx[port].last_sw_vic != rx[port].pre.sw_vic)
					rx[port].min_time_en = true;
				else
					rx[port].min_time_en = false;
				rx[port].state = FSM_SIG_STABLE_TO_READY;
				rx[port].var.sig_stable_cnt = 0;
				rx[port].skip = 0;
				rx[port].var.mute_cnt = 0;
				rx[port].aud_sr_stable_cnt = 0;
				rx[port].aud_sr_unstable_cnt = 0;
				rx[port].no_signal = false;
				rx[port].ecc_err = 0;
				rx[port].var.clk_chg_cnt = 0;
				rx[port].var.sig_stable_err_cnt = 0;
				rx[port].var.esd_phy_rst_cnt = 0;
				rx[port].ddc_filter_en = false;
			}
		} else {
			rx[port].var.sig_stable_cnt = 0;
			rx[port].var.de_stable = false;
			if (rx[port].var.sig_unstable_cnt < sig_unstable_max) {
				rx[port].var.sig_unstable_cnt++;
				if (rx_phy_level & 0x1 &&
					(rx[port].var.sig_unstable_cnt >= reset_pcs_flag &&
					rx[port].var.sig_unstable_cnt <=
					reset_pcs_flag + reset_pcs_cnt)) {
					reset_pcs(port);
					//reset_pcs_flag = 0;
					if (log_level & PHY_LOG)
						rx_pr("reset pcs\n");
				}
				break;
			}
			rx[port].state = FSM_WAIT_CLK_STABLE;
		}
		break;
	case FSM_SIG_STABLE_TO_READY:
		if (!rx[port].cableclk_stb_flg) {
			rx[port].state = FSM_WAIT_CLK_STABLE;
			break;
		}
		memcpy(&rx[port].pre, &rx[port].cur, sizeof(struct rx_video_info));
		rx_get_video_info(port);
		if (rx_is_timing_stable(port)) {
			if (++rx[port].var.sig_stable_cnt >= sig_stable_max) {
				get_timing_fmt(port);
				if ((rx_info.timestamp - rx[port].stable_timestamp <
					detect_time_min) &&
					rx[port].min_time_en)
					break;
				rx[port].var.de_stable = true;
				rx[port].var.sig_unstable_cnt = 0;
				rx[port].var.sig_unready_cnt = 0;
				rx[port].var.sig_stable_cnt = 0;
				rx[port].skip = 0;
				rx[port].var.mute_cnt = 0;
				rx[port].last_sw_vic = rx[port].pre.sw_vic;
				rx[port].state = FSM_SIG_READY;
				rx[port].aud_sr_stable_cnt = 0;
				rx[port].aud_sr_unstable_cnt = 0;
				rx[port].no_signal = false;
				rx[port].ecc_err = 0;
				rx[port].var.clk_chg_cnt = 0;
				hdmirx_config_video(port);
				rx_get_aud_info(&rx[port].aud_info, port);
				hdmirx_config_audio(port);
				rx_aud_pll_ctl(1, port);
				hdmirx_audio_fifo_rst(port);
				rx[port].hdcp.hdcp_pre_ver = rx[port].hdcp.hdcp_version;
				rx[port].stable_timestamp = rx_info.timestamp;
				rx_pr("Sig ready\n");
				dump_state(RX_DUMP_VIDEO, port);
				rx[port].var.sig_stable_err_cnt = 0;
			}
		} else {
			rx_irq_en(0, port);
			rx[port].var.sig_stable_cnt = 0;
			rx[port].var.de_stable = false;
			rx[port].state = FSM_WAIT_CLK_STABLE;
		}
		break;
	case FSM_SIG_READY:
		rx_get_video_info(port);
		#ifdef K_TEST_CHK_ERR_CNT
		rx_monitor_error_counter(port);
		#endif
		/* video info change */
		if (!is_tmds_valid(port)) {
			if (video_mute_enabled(port)) {
				set_video_mute(HDMI_RX_MUTE_SET, true);
				rx_mute_vpp(rx_get_port_type(port));
				rx[port].vpp_mute = true;
				rx[port].var.mute_cnt = 0;
				if (log_level & 0x100)
					rx_pr("vpp mute\n");
			}
			skip_frame(skip_frame_cnt, port, "fsm0 tmds skip");
			rx[port].unready_timestamp = rx_info.timestamp;
			dump_unnormal_info(port);
			rx_pr("tmds_invalid-->unready\n");
			rx[port].var.de_stable = false;
			rx[port].var.sig_unready_cnt = 0;
			rx[port].aud_info.real_sr = 0;
			rx_aud_pll_ctl(0, port);
			rx[port].hdcp.hdcp_pre_ver = rx[port].hdcp.hdcp_version;
			/* need to clr to none, for dishNXT box */
			rx[port].hdcp.hdcp_version = HDCP_VER_NONE;
			rx_irq_en(0, port);
			hdmirx_output_en(false);
			rx[port].state = FSM_WAIT_CLK_STABLE;
			rx[port].var.vic_check_en = false;
			rx[port].skip = 0;
			rx[port].var.mute_cnt = 0;
			rx[port].aud_sr_stable_cnt = 0;
			rx[port].aud_sr_unstable_cnt = 0;
			rx[port].clk.cable_clk = 0;
			rx[port].hdcp.stream_type = 0;
			rx[port].var.esd_phy_rst_cnt = 0;
			break;
		} else if (!rx_is_timing_stable(port)) {
			skip_frame(skip_frame_cnt, port, "fsm0 timing skip");
			if (sig_unready_max)
				chk_cnt = sig_unready_max;
			else
				chk_cnt = get_frame_interval_cnt(1, port);
			if (++rx[port].var.sig_unready_cnt >= chk_cnt) {
				rx[port].unready_timestamp = rx_info.timestamp;
				dump_unnormal_info(port);
				rx_pr("timing unstable-->unready\n");
				rx[port].var.de_stable = false;
				rx[port].var.sig_unready_cnt = 0;
				rx[port].aud_info.real_sr = 0;
				rx_aud_pll_ctl(0, port);
				rx[port].hdcp.hdcp_pre_ver = rx[port].hdcp.hdcp_version;
				/* need to clr to none, for dishNXT box */
				rx[port].hdcp.hdcp_version = HDCP_VER_NONE;
				//rx_sw_reset(2, port);
				rx_irq_en(0, port);
				hdmirx_output_en(false);
				rx[port].state = FSM_WAIT_CLK_STABLE;
				rx[port].var.vic_check_en = false;
				rx[port].skip = 0;
				rx[port].var.mute_cnt = 0;
				rx[port].aud_sr_stable_cnt = 0;
				rx[port].aud_sr_unstable_cnt = 0;
				rx[port].clk.cable_clk = 0;
				rx[port].hdcp.stream_type = 0;
				rx[port].var.esd_phy_rst_cnt = 0;
				break;
			}
		} else if (!rx_is_color_space_stable(port)) {
			//Color space changes, no need to do EQ training
			skip_frame(skip_frame_cnt, port, "fsm color skip");
			if (video_mute_enabled(port)) {
				set_video_mute(HDMI_RX_MUTE_SET, true);
				rx_mute_vpp(rx_get_port_type(port));
				rx[port].vpp_mute = true;
				rx[port].var.mute_cnt = 0;
				if (log_level & 0x100)
					rx_pr("vpp mute1\n");
			}
			if (sig_unready_max)
				chk_cnt = sig_unready_max;
			else
				chk_cnt = get_frame_interval_cnt(1, port);
			if (++rx[port].var.sig_unready_cnt >= chk_cnt) {
				/*sig_lost_lock_cnt = 0;*/
				rx[port].unready_timestamp = rx_info.timestamp;
				rx_pr("colorspace changes from %d to %d\n",
					  rx[port].pre.colorspace, rx[port].cur.colorspace);
				rx[port].var.de_stable = false;
				rx[port].hdcp.hdcp_pre_ver = rx[port].hdcp.hdcp_version;
				/* need to clr to none, for dishNXT box */
				hdmirx_output_en(false);
				rx[port].state = FSM_SIG_WAIT_STABLE;
				break;
			}
		} else if (!rx_fps_is_changed(port) && gcp_mute_flag[port] &&
		!rx[port].free_sync_sts && !rx[port].vtem_info.vrr_en) {
			if (++rx[port].var.fps_unready_cnt >= fps_unready_max) {
				rx[port].gcp_mute_cnt = gcp_mute_cnt;
				rx[port].pre.frame_rate = rx[port].cur.frame_rate;
				rx[port].var.fps_unready_cnt = 0;
				gcp_mute_flag[port] = 0;
				break;
			}
		} else {
			rx[port].var.sig_unready_cnt = 0;
			if (rx[port].skip > 0) {
				rx[port].skip--;
			} else if (vpp_mute_enable) {
				if (rx[port].gcp_mute_cnt > 0) {
					rx[port].gcp_mute_cnt--;
					break;
				}
				if (rx[port].vpp_mute_cnt > 0) {
					rx[port].vpp_mute_cnt--;
					break;
				}
				/* clear vpp mute after signal stable */
				if (get_video_mute_val(HDMI_RX_MUTE_SET) &&
					port != rx_info.sub_port) {
					if (rx[port].var.mute_cnt++ <
						get_frame_interval_cnt(1, port))
						break;
					rx[port].var.mute_cnt = 0;
					rx[port].vpp_mute = false;
					set_video_mute(HDMI_RX_MUTE_SET, false);
				}
			}
		}
		if (rx[port].pre.sw_dvi == 1)
			break;
		rx_get_aud_info(&rx[port].aud_info, port);
		if (check_real_sr_change(port))
			rx_audio_pll_sw_update();
		break;
	default:
		break;
	}
	/* for fsm debug */
	if (rx[port].state != rx[port].pre_state) {
		if (!(log_level & COR1_LOG))
			rx_pr("fsm0 (%s) to (%s)\n",
			      fsm_st[rx[port].pre_state],
			      fsm_st[rx[port].state]);
		rx[port].pre_state = rx[port].state;
	}
}

void rx_port1_main_state_machine(void)
{
	int chk_cnt;
	u8 port = E_PORT1;

	if ((dbg_port - 1 != port) &&
		dbg_port)
		return;

	rx_cable_clk_monitor(port);
	switch (rx[port].state) {
	case FSM_5V_LOST:
		if (rx[port].cur_5v_sts)
			rx[port].state = FSM_INIT;
		fsm_restart(port);
		break;
	case FSM_HPD_LOW:
		rx_irq_en(0, port);
		rx_set_cur_hpd(0, 0, port);
		rx[port].state = FSM_INIT;
		break;
	case FSM_INIT:
		signal_status_init(port);
		rx[port].clk.cable_clk = 0;
		rx[port].state = FSM_HPD_HIGH;
		break;
	case FSM_HPD_HIGH:
		if (rx[port].cur_5v_sts == 0)
			break;
		rx[port].var.hpd_wait_cnt++;
		if (rx_get_cur_hpd_sts(port) == 0) {
			if (rx_hpd_keep_low(port))
				break;
			//hdmirx_hw_config(port);//todo
		}
		rx[port].var.hpd_wait_cnt = 0;
		rx[port].var.clk_unstable_cnt = 0;
		rx[port].var.downstream_hpd_flag = 0;
		rx[port].var.edid_update_flag = 0;
		rx_set_cur_hpd(1, 0, port);
		rx[port].last_sw_vic = 0;
		memset(&rx[port].pre, 0, sizeof(struct rx_video_info));
		memset(&rx[port].cur, 0, sizeof(struct rx_video_info));
		rx[port].clk.cable_clk = 0;
		rx[port].phy.cablesel = 0;
		/* rx[port].hdcp.hdcp_version = HDCP_VER_NONE; */
		rx[port].state = FSM_WAIT_CLK_STABLE;
		break;
	case FSM_WAIT_CLK_STABLE:
		if (rx[port].cur_5v_sts == 0)
			break;
		if (rx[port].cableclk_stb_flg) {
			if (rx[port].var.clk_unstable_cnt != 0) {
				if (rx[port].var.clk_stable_cnt < clk_stable_max) {
					rx[port].var.clk_stable_cnt++;
					break;
				}
				rx_pr("wait clk cnt %d\n", rx[port].var.clk_unstable_cnt);
			}
			rx[port].stable_timestamp = rx_info.timestamp;
			rx[port].state = FSM_EQ_START;
			rx[port].var.clk_stable_cnt = 0;
			rx[port].var.clk_unstable_cnt = 0;
			rx_pr("clk stable=%d\n", rx[port].clk.cable_clk);
			rx_irq_en(IRQ_EN_HDCP, port);//todo
		} else {
			rx[port].var.clk_stable_cnt = 0;
			if (rx[port].var.clk_unstable_cnt < clk_unstable_max) {
				rx[port].var.clk_unstable_cnt++;
				break;
			}
			rx[port].var.clk_unstable_cnt = 0;
			hdmirx_phy_init(port);
			rx[port].clk.cable_clk = 0;
		}
		break;
	case FSM_EQ_START:
		if (!rx[port].cableclk_stb_flg) {
			rx[port].state = FSM_WAIT_CLK_STABLE;
			break;
		}
		rx_run_eq(port);
		rx[port].state = FSM_WAIT_EQ_DONE;
		break;
	case FSM_WAIT_EQ_DONE:
		if (!rx[port].cableclk_stb_flg) {
			rx[port].state = FSM_WAIT_CLK_STABLE;
			break;
		}
		if (rx_eq_done(port)) {
			rx[port].state = FSM_PCS_RESET;
			rx[port].var.pll_lock_cnt = 0;
			rx[port].var.pll_unlock_cnt = 0;
			rx[port].var.clk_chg_cnt = 0;

		}
		break;
	case FSM_PCS_RESET:
		reset_pcs(port);
		rx[port].state = FSM_SIG_UNSTABLE;
		rx[port].var.pll_lock_cnt = 0;
		rx[port].var.pll_unlock_cnt = 0;
		rx[port].var.clk_chg_cnt = 0;
		break;
	case FSM_SIG_UNSTABLE:
		if (!rx[port].cableclk_stb_flg) {
			rx[port].state = FSM_WAIT_CLK_STABLE;
			break;
		}
		if (is_tmds_valid(port)) {
			/* pll_unlock_cnt = 0; */
			if (++rx[port].var.pll_lock_cnt < pll_lock_max)
				break;
			//rx_irq_en(true, port);
			//hdmirx_top_irq_en(1, 1, port);
			rx[port].state = FSM_SIG_WAIT_STABLE;
		} else {
			rx[port].var.pll_lock_cnt = 0;
			if (rx[port].var.pll_unlock_cnt < pll_unlock_max) {
				rx[port].var.pll_unlock_cnt++;
				break;
			}
			rx[port].state = FSM_WAIT_CLK_STABLE;
		}
		break;
	case FSM_SIG_WAIT_STABLE:
		if (!rx[port].cableclk_stb_flg) {
			rx[port].state = FSM_WAIT_CLK_STABLE;
			break;
		}
		rx[port].var.dwc_rst_wait_cnt++;
		if (rx[port].var.dwc_rst_wait_cnt < dwc_rst_wait_cnt_max)
			break;
		rx[port].var.edid_update_flag = 0;
		rx[port].var.dwc_rst_wait_cnt = 0;
		rx[port].var.sig_stable_cnt = 0;
		rx[port].var.sig_unstable_cnt = 0;
		rx[port].var.sig_stable_err_cnt = 0;
		rx[port].var.clk_chg_cnt = 0;
		reset_pcs(port);
		rx_pkt_initial(port);
		rx[port].state = FSM_SIG_HOLD;
		break;
	case FSM_SIG_HOLD:    //todo
		if (!rx[port].cableclk_stb_flg) {
			rx[port].state = FSM_WAIT_CLK_STABLE;
			break;
		}
		if (port != rx_info.main_port &&
			port != rx_info.sub_port)
			break;
		rx[port].state = FSM_SIG_STABLE;
		break;
	case FSM_SIG_STABLE:
		if (!rx[port].cableclk_stb_flg) {
			rx[port].state = FSM_WAIT_CLK_STABLE;
			break;
		}
		memcpy(&rx[port].pre, &rx[port].cur, sizeof(struct rx_video_info));
		rx_get_video_info(port);
		if (rx_is_timing_stable(port)) {
			if (++rx[port].var.sig_stable_cnt >= sig_stable_max) {
				rx_irq_en(IRQ_EN_ALL, port);
				get_timing_fmt(port);
				rx[port].var.de_stable = true;
				rx[port].var.sig_unstable_cnt = 0;
				rx[port].var.sig_unready_cnt = 0;
				/* if DVI signal is detected, then try
				 * hpd reset once to recovery, to avoid
				 * recognition to DVI of low probability
				 */
				if (rx[port].pre.sw_dvi &&  rx[port].var.dvi_check_en &&
				    rx[port].hdcp.hdcp_version == HDCP_VER_NONE) {
					rx[port].state = FSM_HPD_LOW;
					rx[port].var.dvi_check_en = false;
					break;
				}
				if (rx[port].last_sw_vic != rx[port].pre.sw_vic)
					rx[port].min_time_en = true;
				else
					rx[port].min_time_en = false;
				rx[port].state = FSM_SIG_STABLE_TO_READY;
				rx[port].var.sig_stable_cnt = 0;
				rx[port].skip = 0;
				rx[port].var.mute_cnt = 0;
				rx[port].aud_sr_stable_cnt = 0;
				rx[port].aud_sr_unstable_cnt = 0;
				rx[port].no_signal = false;
				rx[port].ecc_err = 0;
				rx[port].var.clk_chg_cnt = 0;
				rx[port].var.sig_stable_err_cnt = 0;
				rx[port].var.esd_phy_rst_cnt = 0;
				rx[port].ddc_filter_en = false;
			}
		} else {
			rx[port].var.sig_stable_cnt = 0;
			rx[port].var.de_stable = false;
			if (rx[port].var.sig_unstable_cnt < sig_unstable_max) {
				rx[port].var.sig_unstable_cnt++;
				if (rx_phy_level & 0x1 &&
					(rx[port].var.sig_unstable_cnt >= reset_pcs_flag &&
					rx[port].var.sig_unstable_cnt <=
					reset_pcs_flag + reset_pcs_cnt)) {
					reset_pcs(port);
					//reset_pcs_flag = 0;
					if (log_level & PHY_LOG)
						rx_pr("reset pcs\n");
				}
				break;
			}
			rx[port].state = FSM_WAIT_CLK_STABLE;
		}
		break;
	case FSM_SIG_STABLE_TO_READY:
		if (!rx[port].cableclk_stb_flg) {
			rx[port].state = FSM_WAIT_CLK_STABLE;
			break;
		}
		memcpy(&rx[port].pre, &rx[port].cur, sizeof(struct rx_video_info));
		rx_get_video_info(port);
		if (rx_is_timing_stable(port)) {
			if (++rx[port].var.sig_stable_cnt >= sig_stable_max) {
				get_timing_fmt(port);
				if ((rx_info.timestamp - rx[port].stable_timestamp <
					detect_time_min) &&
					rx[port].min_time_en)
					break;
				rx[port].var.de_stable = true;
				rx[port].var.sig_unstable_cnt = 0;
				rx[port].var.sig_unready_cnt = 0;
				rx[port].var.sig_stable_cnt = 0;
				rx[port].skip = 0;
				rx[port].var.mute_cnt = 0;
				rx[port].last_sw_vic = rx[port].pre.sw_vic;
				rx[port].state = FSM_SIG_READY;
				rx[port].aud_sr_stable_cnt = 0;
				rx[port].aud_sr_unstable_cnt = 0;
				rx[port].no_signal = false;
				rx[port].ecc_err = 0;
				rx[port].var.clk_chg_cnt = 0;
				hdmirx_config_video(port);
				rx_get_aud_info(&rx[port].aud_info, port);
				hdmirx_config_audio(port);
				rx_aud_pll_ctl(1, port);
				hdmirx_audio_fifo_rst(port);
				rx[port].hdcp.hdcp_pre_ver = rx[port].hdcp.hdcp_version;
				rx[port].stable_timestamp = rx_info.timestamp;
				rx_pr("Sig ready\n");
				dump_state(RX_DUMP_VIDEO, port);
				rx[port].var.sig_stable_err_cnt = 0;
			}
		} else {
			rx_irq_en(0, port);
			rx[port].var.sig_stable_cnt = 0;
			rx[port].var.de_stable = false;
			rx[port].state = FSM_WAIT_CLK_STABLE;
		}
		break;
	case FSM_SIG_READY:
		rx_get_video_info(port);
		#ifdef K_TEST_CHK_ERR_CNT
		rx_monitor_error_counter(port);
		#endif
		/* video info change */
		if (!is_tmds_valid(port)) {
			if (video_mute_enabled(port)) {
				set_video_mute(HDMI_RX_MUTE_SET, true);
				rx_mute_vpp(rx_get_port_type(port));
				rx[port].vpp_mute = true;
				rx[port].var.mute_cnt = 0;
				if (log_level & 0x100)
					rx_pr("vpp mute\n");
			}
			skip_frame(skip_frame_cnt, port, "fsm1 tmds skip");
			rx[port].unready_timestamp = rx_info.timestamp;
			dump_unnormal_info(port);
			rx_pr("tmds_invalid-->unready\n");
			rx[port].var.de_stable = false;
			rx[port].var.sig_unready_cnt = 0;
			rx[port].aud_info.real_sr = 0;
			rx_aud_pll_ctl(0, port);
			rx[port].hdcp.hdcp_pre_ver = rx[port].hdcp.hdcp_version;
			/* need to clr to none, for dishNXT box */
			rx[port].hdcp.hdcp_version = HDCP_VER_NONE;
			rx_irq_en(0, port);
			hdmirx_output_en(false);
			rx[port].state = FSM_WAIT_CLK_STABLE;
			rx[port].var.vic_check_en = false;
			rx[port].skip = 0;
			rx[port].var.mute_cnt = 0;
			rx[port].aud_sr_stable_cnt = 0;
			rx[port].aud_sr_unstable_cnt = 0;
			rx[port].clk.cable_clk = 0;
			rx[port].hdcp.stream_type = 0;
			rx[port].var.esd_phy_rst_cnt = 0;
			break;
		} else if (!rx_is_timing_stable(port)) {
			skip_frame(skip_frame_cnt, port, "fsm1 timing skip");
			if (sig_unready_max)
				chk_cnt = sig_unready_max;
			else
				chk_cnt = get_frame_interval_cnt(1, port);
			if (++rx[port].var.sig_unready_cnt >= chk_cnt) {
				rx[port].unready_timestamp = rx_info.timestamp;
				dump_unnormal_info(port);
				rx_pr("timing unstable-->unready\n");
				rx[port].var.de_stable = false;
				rx[port].var.sig_unready_cnt = 0;
				rx[port].aud_info.real_sr = 0;
				rx_aud_pll_ctl(0, port);
				rx[port].hdcp.hdcp_pre_ver = rx[port].hdcp.hdcp_version;
				/* need to clr to none, for dishNXT box */
				rx[port].hdcp.hdcp_version = HDCP_VER_NONE;
				//rx_sw_reset(2, port);
				rx_irq_en(0, port);
				hdmirx_output_en(false);
				rx[port].state = FSM_WAIT_CLK_STABLE;
				rx[port].var.vic_check_en = false;
				rx[port].skip = 0;
				rx[port].var.mute_cnt = 0;
				rx[port].aud_sr_stable_cnt = 0;
				rx[port].aud_sr_unstable_cnt = 0;
				rx[port].clk.cable_clk = 0;
				rx[port].hdcp.stream_type = 0;
				rx[port].var.esd_phy_rst_cnt = 0;
				break;
			}
		} else if (!rx_is_color_space_stable(port)) {
			//Color space changes, no need to do EQ training
			skip_frame(skip_frame_cnt, port, "fsm1 color skip");
			if (video_mute_enabled(port)) {
				set_video_mute(HDMI_RX_MUTE_SET, true);
				rx_mute_vpp(rx_get_port_type(port));
				rx[port].vpp_mute = true;
				rx[port].var.mute_cnt = 0;
				if (log_level & 0x100)
					rx_pr("vpp mute1\n");
			}
			if (sig_unready_max)
				chk_cnt = sig_unready_max;
			else
				chk_cnt = get_frame_interval_cnt(1, port);
			if (++rx[port].var.sig_unready_cnt >= chk_cnt) {
				/*sig_lost_lock_cnt = 0;*/
				rx[port].unready_timestamp = rx_info.timestamp;
				rx_pr("colorspace changes from %d to %d\n",
					  rx[port].pre.colorspace, rx[port].cur.colorspace);
				rx[port].var.de_stable = false;
				rx[port].hdcp.hdcp_pre_ver = rx[port].hdcp.hdcp_version;
				/* need to clr to none, for dishNXT box */
				hdmirx_output_en(false);
				rx[port].state = FSM_SIG_WAIT_STABLE;
				break;
			}
		} else if (!rx_fps_is_changed(port) && gcp_mute_flag[port] &&
		!rx[port].free_sync_sts && !rx[port].vtem_info.vrr_en) {
			if (++rx[port].var.fps_unready_cnt >= fps_unready_max) {
				rx[port].gcp_mute_cnt = gcp_mute_cnt;
				rx[port].pre.frame_rate = rx[port].cur.frame_rate;
				rx[port].var.fps_unready_cnt = 0;
				gcp_mute_flag[port] = 0;
				break;
			}
		} else {
			rx[port].var.sig_unready_cnt = 0;
			if (rx[port].skip > 0) {
				rx[port].skip--;
			} else if (vpp_mute_enable) {
				if (rx[port].gcp_mute_cnt > 0) {
					rx[port].gcp_mute_cnt--;
					break;
				}
				if (rx[port].vpp_mute_cnt > 0) {
					rx[port].vpp_mute_cnt--;
					break;
				}
				/* clear vpp mute after signal stable */
				if (get_video_mute_val(HDMI_RX_MUTE_SET) &&
					port != rx_info.sub_port) {
					if (rx[port].var.mute_cnt++ <
						get_frame_interval_cnt(1, port))
						break;
					rx[port].var.mute_cnt = 0;
					rx[port].vpp_mute = false;
					set_video_mute(HDMI_RX_MUTE_SET, false);
				}
			}
		}
		if (rx[port].pre.sw_dvi == 1)
			break;
		rx_get_aud_info(&rx[port].aud_info, port);
		if (check_real_sr_change(port))
			rx_audio_pll_sw_update();
		break;
	default:
		break;
	}
	/* for fsm debug */
	if (rx[port].state != rx[port].pre_state) {
		if (!(log_level & COR1_LOG))
			rx_pr("fsm1 (%s) to (%s)\n",
			      fsm_st[rx[port].pre_state],
			      fsm_st[rx[port].state]);
		rx[port].pre_state = rx[port].state;
	}
}

void rx_port2_main_state_machine(void)
{
	int chk_cnt;
	u8 port = E_PORT2;

	if (ecc_err_monitor)
		rx_read_ecc_err(port);
	frate_monitor();
	if ((dbg_port - 1 != port) &&
		dbg_port)
		return;
	rx_cable_clk_monitor(port);
	if (port != rx_info.main_port && port != rx_info.sub_port) {
		if (rx[port].state >= FSM_SIG_HOLD)
			rx[port].state = FSM_SIG_HOLD;
	}

	switch (rx[port].state) {
	case FSM_5V_LOST:
		if (rx[port].cur_5v_sts)
			rx[port].state = FSM_INIT;
		//fsm_restart(port);
		break;
	case FSM_HPD_LOW:
		/* disable irq before hpd low */
		rx_irq_en(0, port);
		rx_set_cur_hpd(0, 0, port);
		//set_scdc_cfg(1, 0, port);
		rx[port].state = FSM_INIT;
		break;
	case FSM_INIT:
		signal_status_init(port);
		rx[port].clk.cable_clk = 0;
		rx[port].state = FSM_HPD_HIGH;
		break;
	case FSM_HPD_HIGH:
		if (rx[port].cur_5v_sts == 0)
			break;
		rx[port].var.hpd_wait_cnt++;
		if (rx_get_cur_hpd_sts(port) == 0) {
			if (rx_hpd_keep_low(port))
				break;
			hdmirx_hw_config(port);
		}
		rx[port].var.hpd_wait_cnt = 0;
		rx[port].var.clk_unstable_cnt = 0;
		rx[port].var.esd_phy_rst_cnt = 0;
		rx[port].var.downstream_hpd_flag = 0;
		rx[port].var.edid_update_flag = 0;
		pre_port = port;
		rx_set_cur_hpd(1, 0, port);
		rx[port].clk.cable_clk = 0;
		rx[port].phy.cablesel = 0;
		//set_scdc_cfg(0, 1, port);
		/* rx[port].hdcp.hdcp_version = HDCP_VER_NONE; */
		if (frl_debug_en)
			rx[port].state = FSM_COR_RESET;
		else
			rx[port].state = FSM_FRL_FLT_READY;
		break;
	case FSM_COR_RESET:
		rx_cor_reset_t3x(port);
		rx[port].state = FSM_FRL_FLT_READY;
		break;
	case FSM_FRL_FLT_READY:
		hdmirx_wr_cor(DPLL_CFG6_DPLL_IVCRX, 0x0, port);
		hdmirx_frl_config(port);
		rx_lts_2_flt_ready(port);
		rx[port].state = FSM_WAIT_SIG;
		break;
	case FSM_WAIT_SIG:
		rx[port].var.frl_rate =
		hdmirx_rd_cor(SCDCS_CONFIG1_SCDC_IVCRX, port) & 0xf;
		if (rx[port].var.frl_rate == 0) {
			rx[port].state = FSM_WAIT_CLK_STABLE;
			if (log_level & FRL_LOG)
				rx_pr("Sink FRL LTS:2 - TMDS detected");
		} else {
			rx[port].state = FLT_RX_LTS_3;
		}
		break;
	case FLT_RX_LTS_3:
		rx_frl_train(port);
		rx[port].state = FSM_FRL_TRN;
		break;
	case FSM_FRL_TRN:
		if (is_frl_train_finished(port))
			rx[port].state = FLT_RX_LTS_P;
		rx[port].var.flt_update = 0;
		break;
	case FLT_RX_LTS_P:
		// sb sr not sync, set hpd low to retry
		if (!rx_lts_p_syn_detect(rx[port].var.frl_rate, port)) {
			rx[port].var.flt_update = 1;
			if (fsm_debug & 0x1) {
				rx[port].state = FSM_HPD_LOW;
				break;
			}
		}
		RX_LTS_P_FRL_START(port);
		rx[port].state = FSM_WAIT_FRL_TRN_DONE;
		rx[port].var.fpll_stable_cnt = 0;
		rx[port].var.fpll_ready_cnt = 0;
		rx[port].var.frl_lock_det_cnt = 0;
		if (s_tmds_transmission_detected(port))
			rx[port].state = FSM_WAIT_CLK_STABLE;
		else
			rx[port].state = FSM_WAIT_FRL_TRN_DONE;
		break;
	case FSM_WAIT_FRL_TRN_DONE:
		//wait timing stable for debug;
		if (rx[port].var.fpll_ready_cnt++ < fpll_ready_max)
			break;
		if (rx[port].var.flt_update) {
			hal_flt_update_set(port);
			rx[port].var.flt_update = 0;
			rx[port].state = FSM_FRL_FLT_READY;
		}
		rx[port].var.lock = hdmirx_rd_cor(SCDCS_STATUS_FLAGS0_SCDC_IVCRX, port);
		if ((rx[port].var.lock & 0x1f) != 0x1e &&
			(rx[port].var.lock & 0x1f) != 0xe) {
			if (rx[port].var.frl_lock_det_cnt++ < frl_lock_det_max)
				break;
			rx[port].var.frl_lock_det_cnt = 0;
			hdmirx_clr_scdc(true, port);
			rx[port].state = FSM_HPD_LOW;
			break;
		}
		rx[port].var.fpll_ready_cnt = 0;
		if (is_fpll_err(port)) {
			if (rx[port].var.fpll_stable_cnt++ < fpll_stable_max)
				break;
		}
		rx[port].var.fpll_stable_cnt = 0;
		if (cts_ced_err_test)
			rx_rcc_err_frl_config(port);
		rx[port].state =  FSM_SIG_UNSTABLE;
		htotal_cnt = 0;
		clr_frl_fifo_status(port);
		break;
	case FSM_WAIT_CLK_STABLE:
		if (rx[port].cur_5v_sts == 0)
			break;
		if (rx[port].cableclk_stb_flg) {
			if (rx[port].var.clk_unstable_cnt != 0) {
				if (rx[port].var.clk_stable_cnt < clk_stable_max) {
					rx[port].var.clk_stable_cnt++;
					break;
				}
				rx_pr("wait clk cnt %d\n", rx[port].var.clk_unstable_cnt);
			}
			rx[port].state = FSM_EQ_START;
			rx[port].var.clk_stable_cnt = 0;
			rx[port].var.clk_unstable_cnt = 0;
			rx_pr("clk stable=%d\n", rx[port].clk.cable_clk);
			rx[port].var.de_stable = false;
			rx_irq_en(IRQ_EN_HDCP, port);
		} else {
			rx[port].var.clk_stable_cnt = 0;
			if (rx[port].var.clk_unstable_cnt < clk_unstable_max) {
				rx[port].var.clk_unstable_cnt++;
				break;
			}
			rx[port].var.clk_unstable_cnt = 0;
			if (rx[port].var.esd_phy_rst_cnt < esd_phy_rst_max) {
				hdmirx_phy_init(port);
				rx[port].clk.cable_clk = 0;
				rx[port].var.esd_phy_rst_cnt++;
			}
		}
		break;
	case FSM_EQ_START:
		if (!rx[port].cableclk_stb_flg) {
			rx[port].state = FSM_WAIT_CLK_STABLE;
			break;
		}
		rx_run_eq(port);
		rx[port].state = FSM_WAIT_EQ_DONE;
		break;
	case FSM_WAIT_EQ_DONE:
		if (!rx[port].cableclk_stb_flg) {
			rx[port].state = FSM_WAIT_CLK_STABLE;
			break;
		}
		if (rx_eq_done(port)) {
			if (rx_info.aml_phy.reset_pcs_en) {
				rx[port].state = FSM_PCS_RESET;
				break;
			}
			rx[port].state = FSM_SIG_UNSTABLE;
			rx[port].var.pll_lock_cnt = 0;
			rx[port].var.pll_unlock_cnt = 0;
			rx[port].var.clk_chg_cnt = 0;
			//esd_phy_rst_cnt = 0;
		}
		break;
	case FSM_PCS_RESET:
		reset_pcs(port);
		rx[port].state = FSM_SIG_UNSTABLE;
		rx[port].var.pll_lock_cnt = 0;
		rx[port].var.pll_unlock_cnt = 0;
		rx[port].var.clk_chg_cnt = 0;
		rx[port].var.fpll_stable_cnt = 0;
		break;
	case FSM_SIG_UNSTABLE:
		if (!rx[port].cableclk_stb_flg && rx[port].var.frl_rate == 0) {
			rx[port].state = FSM_WAIT_CLK_STABLE;
			break;
		}
		if (is_tmds_valid(port)) {
			/* pll_unlock_cnt = 0; */
			if (++rx[port].var.pll_lock_cnt < pll_lock_max)
				break;
			rx_cor_reset(port);
			hdmirx_output_en(1);
			//rx_irq_en(true, port);
			//hdmirx_top_irq_en(1, 1, port);
			rx[port].dsc_flag = false;
			rx[port].state = FSM_SIG_WAIT_STABLE;
		} else {
			rx[port].var.pll_lock_cnt = 0;
			if (rx[port].var.pll_unlock_cnt < pll_unlock_max) {
				rx[port].var.pll_unlock_cnt++;
				break;
			}
			if (rx[port].err_rec_mode == ERR_REC_EQ_RETRY &&
				rx[port].var.frl_rate == 0) {
				rx[port].state = FSM_WAIT_CLK_STABLE;
				if (rx[port].var.esd_phy_rst_cnt++ < esd_phy_rst_max) {
					rx[port].phy.cablesel++;
					//rx[port].clk.cable_clk = 0;
					//hdmirx_phy_init(port);
				} else {
					rx[port].var.esd_phy_rst_cnt = 0;
					rx[port].err_rec_mode = ERR_REC_HPD_RST;
				}
			} else if (rx[port].err_rec_mode == ERR_REC_HPD_RST &&
				rx[port].var.frl_rate == 0 && port != rx_info.arc_port) {
				rx_set_cur_hpd(0, 2, port);
				rx[port].clk.cable_clk = 0;
				rx[port].state = FSM_INIT;
				rx[port].err_rec_mode = ERR_REC_EQ_RETRY;
			}
			rx_set_eq_run_state(E_EQ_START, port);
		}
		break;
	case FSM_SIG_WAIT_STABLE:
		if (!rx[port].cableclk_stb_flg && rx[port].var.frl_rate == 0) {
			rx[port].state = FSM_WAIT_CLK_STABLE;
			break;
		}
		rx[port].var.dwc_rst_wait_cnt++;
		if (rx[port].var.dwc_rst_wait_cnt < dwc_rst_wait_cnt_max)
			break;
		if (rx[port].var.edid_update_flag &&
		    rx[port].var.dwc_rst_wait_cnt < edid_update_delay)
			break;
		rx[port].var.edid_update_flag = 0;
		rx[port].var.dwc_rst_wait_cnt = 0;
		rx[port].var.sig_stable_cnt = 0;
		rx[port].var.sig_unstable_cnt = 0;
		rx[port].var.sig_stable_err_cnt = 0;
		rx[port].var.clk_chg_cnt = 0;
		//reset_pcs(port);
		rx_pkt_initial(port);
		rx[port].var.check_dsc_de_cnt = 0;
		rx_switch_to_self_hsync(port, false);
		rx_clr_f_det(true, port);
		rx[port].state = FSM_SIG_HOLD;
		break;
	case FSM_SIG_HOLD:    //todo
		if (port != rx_info.main_port &&
			port != rx_info.sub_port)
			break;
		rx[port].state = FSM_SIG_STABLE;
		break;
	case FSM_SIG_STABLE:
		if (!rx[port].cableclk_stb_flg && rx[port].var.frl_rate == 0) {
			rx[port].state = FSM_WAIT_CLK_STABLE;
			break;
		}
		if (fsm_debug & 0x2)
			break;
		rx_clr_f_det(false, port);
		memcpy(&rx[port].pre, &rx[port].cur, sizeof(struct rx_video_info));
		rx_get_video_info(port);
		if (rx_is_timing_stable(port)) {
			if (rx[port].var.sig_stable_cnt == sig_stable_max / 2)
				rx_irq_en(IRQ_EN_ALL, port);
			if (++rx[port].var.sig_stable_cnt >= sig_stable_max) {
				get_timing_fmt(port);
				/* timing stable, start count vsync and avi pkt */
				rx[port].var.de_stable = true;
				rx[port].var.sig_unstable_cnt = 0;
				if (is_unnormal_format(rx[port].var.sig_stable_cnt, port))
					break;
				/* if format vic is abnormal, do hw
				 * reset once to try to recover.
				 */
				if (fmt_vic_abnormal(port)) {
					if (rx[port].var.vic_check_en) {
						/* hdmi_rx_top_edid_update(); */
						hdmirx_hw_config(port);
						rx[port].state = FSM_HPD_LOW;
						rx[port].var.vic_check_en = false;
					} else {
						rx[port].state = FSM_FRL_FLT_READY;
						rx_set_eq_run_state(E_EQ_START, port);
						rx[port].var.vic_check_en = true;
					}
					break;
				}
				rx[port].var.sig_unready_cnt = 0;
				/* if DVI signal is detected, then try
				 * hpd reset once to recovery, to avoid
				 * recognition to DVI of low probability
				 */
				if (rx[port].pre.sw_dvi &&  rx[port].var.dvi_check_en &&
					is_ddc_filter_en(port) &&
				    rx[port].hdcp.hdcp_version == HDCP_VER_NONE) {
					rx[port].state = FSM_HPD_LOW;
					rx_i2c_div_init();
					 rx[port].var.dvi_check_en = false;
					break;
				}
				rx[port].var.sig_stable_cnt = 0;
				rx[port].skip = 0;
				rx[port].var.mute_cnt = 0;
				rx[port].state = FSM_SIG_READY;
				rx[port].aud_sr_stable_cnt = 0;
				rx[port].aud_sr_unstable_cnt = 0;
				rx[port].no_signal = false;
				rx[port].ecc_err = 0;
				rx[port].var.clk_chg_cnt = 0;
				/*memset(&rx[port].aud_info, 0,*/
					/*sizeof(struct aud_info_s));*/
				/*rx_set_eq_run_state(E_EQ_PASS);*/
				hdmirx_config_video(port);
				rx_get_aud_info(&rx[port].aud_info, port);
				hdmirx_config_audio(port);
				rx_aud_pll_ctl(1, port);
				hdmirx_audio_fifo_rst(port);
				rx[port].hdcp.hdcp_pre_ver = rx[port].hdcp.hdcp_version;
				rx[port].stable_timestamp = rx_info.timestamp;
				rx_pr("Sig ready\n");
				dump_state(RX_DUMP_VIDEO, port);
				#ifdef K_TEST_CHK_ERR_CNT
				//rx_monitor_error_cnt_start();
				#endif
				rx[port].var.sig_stable_err_cnt = 0;
				rx[port].var.esd_phy_rst_cnt = 0;
				//reset_pcs_flag = 1;
				rx[port].ddc_filter_en = false;
			}
		} else {
			rx_irq_en(IRQ_EN_HDCP, port);
			rx[port].var.sig_stable_cnt = 0;
			rx[port].var.de_stable = false;
			if (rx[port].var.sig_unstable_cnt < sig_unstable_max) {
				rx[port].var.sig_unstable_cnt++;
				if (log_level & PHY_LOG)
					rx_pr("DE not stable\n");
				if (!rx[port].var.frl_rate && rx_phy_level & 0x1 &&
					(rx[port].var.sig_unstable_cnt >= reset_pcs_flag &&
					rx[port].var.sig_unstable_cnt <=
					reset_pcs_flag + reset_pcs_cnt)) {
					reset_pcs(port);
					//reset_pcs_flag = 0;
					if (log_level & PHY_LOG)
						rx_pr("reset pcs\n");
				}
				break;
			}
			if (rx[port].err_rec_mode == ERR_REC_EQ_RETRY &&
				rx[port].var.frl_rate == 0) {
				rx[port].state = FSM_WAIT_CLK_STABLE;
				rx[port].phy.cablesel++;
				if (rx[port].var.esd_phy_rst_cnt++ >= esd_phy_rst_max) {
					rx[port].err_rec_mode = ERR_REC_HPD_RST;
					rx_set_eq_run_state(E_EQ_START, port);
					rx[port].var.esd_phy_rst_cnt = 0;
				}
			} else if (rx[port].err_rec_mode == ERR_REC_HPD_RST &&
				rx[port].var.frl_rate == 0 && port != rx_info.arc_port) {
				rx_set_cur_hpd(0, 2, port);
				rx[port].clk.cable_clk = 0;
				rx[port].state = FSM_INIT;
				rx[port].err_rec_mode = ERR_REC_EQ_RETRY;
			}
		}
		break;
	case FSM_SIG_READY:
		rx_get_video_info(port);
		rx[port].err_rec_mode = ERR_REC_EQ_RETRY;
		#ifdef K_TEST_CHK_ERR_CNT
		//rx_monitor_error_counter(port);
		#endif
		/* video info change */
		if (rx[port].var.frl_rate) {
			if (rx_get_clkready_sts(port) == 0 && rx_get_valid_m_sts(port))
				rx_21_fpll_cfg(rx[port].var.frl_rate, port);
		}
		if (!is_tmds_valid(port)) {
			rx[port].clk.t_clk_pre = rx[port].clk.tclk;
			if (video_mute_enabled(port)) {
				set_video_mute(HDMI_RX_MUTE_SET, true);
				rx_mute_vpp(rx_get_port_type(port));
				rx[port].vpp_mute = true;
				rx[port].var.mute_cnt = 0;
				if (log_level & 0x100)
					rx_pr("vpp mute\n");
			}
			skip_frame(skip_frame_cnt, port, "fsm2 tmds skip");
			rx[port].unready_timestamp = rx_info.timestamp;
			rx_i2c_div_init();
			dump_unnormal_info(port);
			rx_pr("tmds_invalid-->unready\n");
			rx[port].var.de_stable = false;
			rx[port].var.sig_unready_cnt = 0;
			rx[port].aud_info.real_sr = 0;
			rx_aud_pll_ctl(0, port);
			rx[port].hdcp.hdcp_pre_ver = rx[port].hdcp.hdcp_version;
			/* need to clr to none, for dishNXT box */
			rx[port].hdcp.hdcp_version = HDCP_VER_NONE;
			//rx_sw_reset(2);
			rx_irq_en(0, port);
			hdmirx_output_en(false);
			if (frl_debug_en == 2)
				rx[port].state = FSM_COR_RESET;
			else
				rx[port].state = FSM_FRL_FLT_READY;
			rx[port].var.vic_check_en = false;
			rx[port].skip = 0;
			rx[port].var.mute_cnt = 0;
			rx[port].aud_sr_stable_cnt = 0;
			rx[port].aud_sr_unstable_cnt = 0;
			rx[port].clk.cable_clk = 0;
			rx[port].hdcp.stream_type = 0;
			rx[port].var.esd_phy_rst_cnt = 0;
			break;
		} else if (!rx_is_timing_stable(port)) {
			skip_frame(skip_frame_cnt, port, "fsm1 timing skip");
			if (sig_unready_max)
				chk_cnt = sig_unready_max;
			else
				chk_cnt = get_frame_interval_cnt(1, port);
			if (++rx[port].var.sig_unready_cnt >= chk_cnt) {
				/*sig_lost_lock_cnt = 0;*/
				rx[port].unready_timestamp = rx_info.timestamp;
				rx_i2c_div_init();
				dump_unnormal_info(port);
				rx_pr("timing unstable-->unready\n");
				rx[port].var.de_stable = false;
				rx[port].var.sig_unready_cnt = 0;
				rx[port].aud_info.real_sr = 0;
				rx_aud_pll_ctl(0, port);
				rx[port].hdcp.hdcp_pre_ver = rx[port].hdcp.hdcp_version;
				/* need to clr to none, for dishNXT box */
				rx[port].hdcp.hdcp_version = HDCP_VER_NONE;
				//rx_sw_reset(2, port);
				rx_irq_en(0, port);
				hdmirx_output_en(false);
				if (frl_debug_en == 2)
					rx[port].state = FSM_COR_RESET;
				else
					rx[port].state = FSM_FRL_FLT_READY;
				rx[port].var.vic_check_en = false;
				rx[port].skip = 0;
				rx[port].var.mute_cnt = 0;
				rx[port].aud_sr_stable_cnt = 0;
				rx[port].aud_sr_unstable_cnt = 0;
				rx[port].clk.cable_clk = 0;
				rx[port].hdcp.stream_type = 0;
				rx[port].var.esd_phy_rst_cnt = 0;
				break;
			}
		} else if (!rx_is_color_space_stable(port)) {
			//Color space changes, no need to do EQ training
			skip_frame(skip_frame_cnt, port, "fsm2 color skip");
			if (sig_unready_max)
				chk_cnt = sig_unready_max;
			else
				chk_cnt = get_frame_interval_cnt(1, port);
			if (++rx[port].var.sig_unready_cnt >= chk_cnt) {
				/*sig_lost_lock_cnt = 0;*/
				rx[port].unready_timestamp = rx_info.timestamp;
				rx_i2c_div_init();
				rx_pr("colorspace changes from %d to %d\n",
					  rx[port].pre.colorspace, rx[port].cur.colorspace);
				rx[port].var.de_stable = false;
				rx[port].hdcp.hdcp_pre_ver = rx[port].hdcp.hdcp_version;
				/* need to clr to none, for dishNXT box */
				hdmirx_output_en(false);
				rx[port].state = FSM_SIG_WAIT_STABLE;
				break;
			}
		} else if (!rx_fps_is_changed(port) && gcp_mute_flag[port] &&
		!rx[port].free_sync_sts && !rx[port].vtem_info.vrr_en) {
			if (++rx[port].var.fps_unready_cnt >= fps_unready_max) {
				rx[port].gcp_mute_cnt = gcp_mute_cnt;
				rx[port].pre.frame_rate = rx[port].cur.frame_rate;
				rx[port].var.fps_unready_cnt = 0;
				gcp_mute_flag[port] = 0;
				break;
			}
		} else {
			rx[port].var.sig_unready_cnt = 0;
			if (rx[port].skip > 0) {
				rx[port].skip--;
			} else if (vpp_mute_enable) {
				if (rx[port].gcp_mute_cnt > 0) {
					rx[port].gcp_mute_cnt--;
					break;
				}
				if (rx[port].vpp_mute_cnt > 0) {
					rx[port].vpp_mute_cnt--;
					break;
				}
				/* clear vpp mute after signal stable */
				if (get_video_mute_val(HDMI_RX_MUTE_SET) &&
					port != rx_info.sub_port) {
					if (rx[port].var.mute_cnt++ <
						get_frame_interval_cnt(1, port))
						break;
					rx[port].var.mute_cnt = 0;
					rx[port].vpp_mute = false;
					set_video_mute(HDMI_RX_MUTE_SET, false);
				}
			}
		}
		if (rx[port].pre.sw_dvi == 1)
			break;
		//packet_update(port);
		rx_get_aud_info(&rx[port].aud_info, port);

		if (check_real_sr_change(port))
			rx_audio_pll_sw_update();
		if (is_aud_pll_error()) {
			rx[port].aud_sr_unstable_cnt++;
			if (rx[port].aud_sr_unstable_cnt > aud_sr_stb_max) {
				unsigned int aud_sts = rx_get_aud_pll_err_sts(port);

				if (aud_sts == E_REQUESTCLK_ERR) {
					hdmirx_phy_init(port);
					rx[port].state = FSM_WAIT_CLK_STABLE;
					/*timing sw at same FRQ*/
					rx[port].clk.cable_clk = 0;
					rx_pr("reqclk err->wait_clk\n");
				} else if (aud_sts == E_PLLRATE_CHG) {
					rx_aud_pll_ctl(1, port);
				} else if (aud_sts == E_AUDCLK_ERR) {
					rx_audio_bandgap_rst();
				} else {
					rx_acr_info_sw_update();
					rx_audio_pll_sw_update();
				}
				if (log_level & AUDIO_LOG)
					rx_pr("update audio-err:%d\n", aud_sts);
				rx[port].aud_sr_unstable_cnt = 0;
			}
		} else if (is_aud_fifo_error()) {
			rx[port].aud_sr_unstable_cnt++;
			if (rx[port].aud_sr_unstable_cnt > aud_sr_stb_max) {
				hdmirx_audio_fifo_rst(port);
				rx[port].aud_sr_unstable_cnt = 0;
			}
		} else {
			rx[port].aud_sr_unstable_cnt = 0;
		}
		break;
	default:
		break;
	}
	/* for fsm debug */
	if (rx[port].state != rx[port].pre_state) {
		rx_pr("fsm2 (%s) to (%s)\n",
			      fsm_st[rx[port].pre_state],
			      fsm_st[rx[port].state]);
		rx[port].pre_state = rx[port].state;
	}
}

void rx_port3_main_state_machine(void)
{
	int chk_cnt;
	u8 port = E_PORT3;

	if (ecc_err_monitor)
		rx_read_ecc_err(port);
	frate_monitor1();
	if ((dbg_port - 1 != port) &&
		dbg_port)
		return;
	rx_cable_clk_monitor(port);
	if (port != rx_info.main_port && port != rx_info.sub_port) {
		if (rx[port].state >= FSM_SIG_HOLD)
			rx[port].state = FSM_SIG_HOLD;
	}

	switch (rx[port].state) {
	case FSM_5V_LOST:
		if (rx[port].cur_5v_sts)
			rx[port].state = FSM_INIT;
		//fsm_restart(port);
		break;
	case FSM_HPD_LOW:
		/* disable irq before hpd low */
		rx_irq_en(0, port);
		rx_set_cur_hpd(0, 0, port);
		//set_scdc_cfg(1, 0, port);
		rx[port].state = FSM_INIT;
		break;
	case FSM_INIT:
		signal_status_init(port);
		rx[port].clk.cable_clk = 0;
		rx[port].state = FSM_HPD_HIGH;
		break;
	case FSM_HPD_HIGH:
		if (rx[port].cur_5v_sts == 0)
			break;
		rx[port].var.hpd_wait_cnt++;
		if (rx_get_cur_hpd_sts(port) == 0) {
			if (rx_hpd_keep_low(port))
				break;
			hdmirx_hw_config(port);
		}
		rx[port].var.hpd_wait_cnt = 0;
		rx[port].var.clk_unstable_cnt = 0;
		rx[port].var.esd_phy_rst_cnt = 0;
		rx[port].var.downstream_hpd_flag = 0;
		rx[port].var.edid_update_flag = 0;
		pre_port = port;
		rx_set_cur_hpd(1, 0, port);
		rx[port].clk.cable_clk = 0;
		rx[port].phy.cablesel = 0;
		//set_scdc_cfg(0, 1, port);
		/* rx[port].hdcp.hdcp_version = HDCP_VER_NONE; */
		if (frl_debug_en)
			rx[port].state = FSM_COR_RESET;
		else
			rx[port].state = FSM_FRL_FLT_READY;
		break;
	case FSM_COR_RESET:
		rx_cor_reset_t3x(port);
		rx[port].state = FSM_FRL_FLT_READY;
		break;
	case FSM_FRL_FLT_READY:
		hdmirx_wr_cor(DPLL_CFG6_DPLL_IVCRX, 0x0, port);
		hdmirx_frl_config(port);
		rx_lts_2_flt_ready(port);
		rx[port].state = FSM_WAIT_SIG;
		break;
	case FSM_WAIT_SIG:
		rx[port].var.frl_rate =
		hdmirx_rd_cor(SCDCS_CONFIG1_SCDC_IVCRX, port) & 0xf;
		if (rx[port].var.frl_rate == 0) {
			rx[port].state = FSM_WAIT_CLK_STABLE;
			if (log_level & FRL_LOG)
				rx_pr("Sink FRL LTS:2 - TMDS detected");
		} else {
			rx[port].state = FLT_RX_LTS_3;
		}
		break;
	case FLT_RX_LTS_3:
		rx_frl_train(port);
		rx[port].state = FSM_FRL_TRN;
		break;
	case FSM_FRL_TRN:
		if (is_frl_train_finished(port))
			rx[port].state = FLT_RX_LTS_P;
		rx[port].var.flt_update = 0;
		break;
	case FLT_RX_LTS_P:
		// sb sr not sync, set hpd low to retry
		if (!rx_lts_p_syn_detect(rx[port].var.frl_rate, port)) {
			rx[port].var.flt_update = 1;
			if (fsm_debug & 0x1) {
				rx[port].state = FSM_HPD_LOW;
				break;
			}
		}
		RX_LTS_P_FRL_START(port);
		rx[port].state = FSM_WAIT_FRL_TRN_DONE;
		rx[port].var.fpll_stable_cnt = 0;
		rx[port].var.fpll_ready_cnt = 0;
		rx[port].var.frl_lock_det_cnt = 0;
		if (s_tmds_transmission_detected(port))
			rx[port].state = FSM_WAIT_CLK_STABLE;
		else
			rx[port].state = FSM_WAIT_FRL_TRN_DONE;
		break;
	case FSM_WAIT_FRL_TRN_DONE:
		if (rx[port].var.fpll_ready_cnt++ < fpll_ready_max)
			break;
		if (rx[port].var.flt_update) {
			hal_flt_update_set(port);
			rx[port].var.flt_update = 0;
			rx[port].state = FSM_FRL_FLT_READY;
		}
		rx[port].var.lock = hdmirx_rd_cor(SCDCS_STATUS_FLAGS0_SCDC_IVCRX, port);
		if ((rx[port].var.lock & 0x1f) != 0x1e &&
			(rx[port].var.lock & 0x1f) != 0xe) {
			if (rx[port].var.frl_lock_det_cnt++ < frl_lock_det_max)
				break;
			rx[port].var.frl_lock_det_cnt = 0;
			hdmirx_clr_scdc(true, port);
			rx[port].state = FSM_HPD_LOW;
			break;
		}
		rx[port].var.fpll_ready_cnt = 0;
		if (is_fpll_err(port)) {
			if (rx[port].var.fpll_stable_cnt++ < fpll_stable_max)
				break;
		}
		rx[port].var.fpll_stable_cnt = 0;
		rx[port].state = FSM_SIG_UNSTABLE;
		if (cts_ced_err_test)
			rx_rcc_err_frl_config(port);
		htotal_cnt = 0;
		clr_frl_fifo_status(port);
		break;
	case FSM_WAIT_CLK_STABLE:
		if (rx[port].cur_5v_sts == 0)
			break;
		if (rx[port].cableclk_stb_flg) {
			if (rx[port].var.clk_unstable_cnt != 0) {
				if (rx[port].var.clk_stable_cnt < clk_stable_max) {
					rx[port].var.clk_stable_cnt++;
					break;
				}
				rx_pr("wait clk cnt %d\n", rx[port].var.clk_unstable_cnt);
			}
			rx[port].state = FSM_EQ_START;
			rx[port].var.clk_stable_cnt = 0;
			rx[port].var.clk_unstable_cnt = 0;
			rx_pr("clk stable=%d\n", rx[port].clk.cable_clk);
			rx[port].var.de_stable = false;
			rx_irq_en(IRQ_EN_HDCP, port);
		} else {
			rx[port].var.clk_stable_cnt = 0;
			if (rx[port].var.clk_unstable_cnt < clk_unstable_max) {
				rx[port].var.clk_unstable_cnt++;
				break;
			}
			rx[port].var.clk_unstable_cnt = 0;
			if (rx[port].var.esd_phy_rst_cnt < esd_phy_rst_max) {
				hdmirx_phy_init(port);
				rx[port].clk.cable_clk = 0;
				rx[port].var.esd_phy_rst_cnt++;
			}
		}
		break;
	case FSM_EQ_START:
		if (!rx[port].cableclk_stb_flg) {
			rx[port].state = FSM_WAIT_CLK_STABLE;
			break;
		}
		rx_run_eq(port);
		rx[port].state = FSM_WAIT_EQ_DONE;
		break;
	case FSM_WAIT_EQ_DONE:
		if (!rx[port].cableclk_stb_flg) {
			rx[port].state = FSM_WAIT_CLK_STABLE;
			break;
		}
		if (rx_eq_done(port)) {
			if (rx_info.aml_phy.reset_pcs_en) {
				rx[port].state = FSM_PCS_RESET;
				break;
			}
			rx[port].state = FSM_SIG_UNSTABLE;
			rx[port].var.pll_lock_cnt = 0;
			rx[port].var.pll_unlock_cnt = 0;
			rx[port].var.clk_chg_cnt = 0;
			//esd_phy_rst_cnt = 0;
		}
		break;
	case FSM_PCS_RESET:
		reset_pcs(port);
		rx[port].state = FSM_SIG_UNSTABLE;
		rx[port].var.pll_lock_cnt = 0;
		rx[port].var.pll_unlock_cnt = 0;
		rx[port].var.clk_chg_cnt = 0;
		rx[port].var.fpll_stable_cnt = 0;
		break;
	case FSM_SIG_UNSTABLE:
		if (!rx[port].cableclk_stb_flg && rx[port].var.frl_rate == 0) {
			rx[port].state = FSM_WAIT_CLK_STABLE;
			break;
		}
		if (is_tmds_valid(port)) {
			/* pll_unlock_cnt = 0; */
			if (++rx[port].var.pll_lock_cnt < pll_lock_max)
				break;
			rx_cor_reset(port);
			hdmirx_output_en(1);
			rx[port].dsc_flag = false;
			//rx_irq_en(true, port);
			//hdmirx_top_irq_en(1, 1, port);
			rx[port].state = FSM_SIG_WAIT_STABLE;
		} else {
			rx[port].var.pll_lock_cnt = 0;
			if (rx[port].var.pll_unlock_cnt < pll_unlock_max) {
				rx[port].var.pll_unlock_cnt++;
				break;
			}
			if (rx[port].err_rec_mode == ERR_REC_EQ_RETRY &&
				rx[port].var.frl_rate == 0) {
				rx[port].state = FSM_WAIT_CLK_STABLE;
				if (rx[port].var.esd_phy_rst_cnt++ < esd_phy_rst_max) {
					rx[port].phy.cablesel++;
					//rx[port].clk.cable_clk = 0;
					//hdmirx_phy_init(port);
				} else {
					rx[port].var.esd_phy_rst_cnt = 0;
					rx[port].err_rec_mode = ERR_REC_HPD_RST;
				}
			} else if (rx[port].err_rec_mode == ERR_REC_HPD_RST &&
				rx[port].var.frl_rate == 0 && port != rx_info.arc_port) {
				rx_set_cur_hpd(0, 2, port);
				rx[port].clk.cable_clk = 0;
				rx[port].state = FSM_INIT;
				rx[port].err_rec_mode = ERR_REC_EQ_RETRY;
			}
			rx_set_eq_run_state(E_EQ_START, port);
		}
		break;
	case FSM_SIG_WAIT_STABLE:
		if (!rx[port].cableclk_stb_flg && rx[port].var.frl_rate == 0) {
			rx[port].state = FSM_WAIT_CLK_STABLE;
			break;
		}
		rx[port].var.dwc_rst_wait_cnt++;
		if (rx[port].var.dwc_rst_wait_cnt < dwc_rst_wait_cnt_max)
			break;
		if (rx[port].var.edid_update_flag &&
		    rx[port].var.dwc_rst_wait_cnt < edid_update_delay)
			break;
		rx[port].var.edid_update_flag = 0;
		rx[port].var.dwc_rst_wait_cnt = 0;
		rx[port].var.sig_stable_cnt = 0;
		rx[port].var.sig_unstable_cnt = 0;
		rx[port].var.sig_stable_err_cnt = 0;
		rx[port].var.clk_chg_cnt = 0;
		//reset_pcs(port);
		rx_pkt_initial(port);
		rx[port].var.check_dsc_de_cnt = 0;
		rx_switch_to_self_hsync(port, false);
		rx_clr_f_det(true, port);
		rx[port].state = FSM_SIG_HOLD;
		break;
	case FSM_SIG_HOLD:    //todo
		if (port != rx_info.main_port &&
			port != rx_info.sub_port)
			break;
		rx[port].state = FSM_SIG_STABLE;
		break;
	case FSM_SIG_STABLE:
		if (!rx[port].cableclk_stb_flg && rx[port].var.frl_rate == 0) {
			rx[port].state = FSM_WAIT_CLK_STABLE;
			break;
		}
		if (fsm_debug & 0x2)
			break;
		rx_clr_f_det(false, port);
		memcpy(&rx[port].pre, &rx[port].cur, sizeof(struct rx_video_info));
		rx_get_video_info(port);
		if (rx_is_timing_stable(port)) {
			if (rx[port].var.sig_stable_cnt == sig_stable_max / 2)
				rx_irq_en(IRQ_EN_ALL, port);
			if (++rx[port].var.sig_stable_cnt >= sig_stable_max) {
				get_timing_fmt(port);
				/* timing stable, start count vsync and avi pkt */
				rx[port].var.de_stable = true;
				rx[port].var.sig_unstable_cnt = 0;
				if (is_unnormal_format(rx[port].var.sig_stable_cnt, port))
					break;
				/* if format vic is abnormal, do hw
				 * reset once to try to recover.
				 */
				if (fmt_vic_abnormal(port)) {
					if (rx[port].var.vic_check_en) {
						/* hdmi_rx_top_edid_update(); */
						hdmirx_hw_config(port);
						rx[port].state = FSM_HPD_LOW;
						rx[port].var.vic_check_en = false;
					} else {
						rx[port].state = FSM_FRL_FLT_READY;
						rx_set_eq_run_state(E_EQ_START, port);
						rx[port].var.vic_check_en = true;
					}
					break;
				}
				rx[port].var.sig_unready_cnt = 0;
				/* if DVI signal is detected, then try
				 * hpd reset once to recovery, to avoid
				 * recognition to DVI of low probability
				 */
				if (rx[port].pre.sw_dvi &&  rx[port].var.dvi_check_en &&
					is_ddc_filter_en(port) &&
				    rx[port].hdcp.hdcp_version == HDCP_VER_NONE) {
					rx[port].state = FSM_HPD_LOW;
					rx_i2c_div_init();
					 rx[port].var.dvi_check_en = false;
					break;
				}
				rx[port].var.sig_stable_cnt = 0;
				rx[port].skip = 0;
				rx[port].var.mute_cnt = 0;
				rx[port].state = FSM_SIG_READY;
				rx[port].aud_sr_stable_cnt = 0;
				rx[port].aud_sr_unstable_cnt = 0;
				rx[port].no_signal = false;
				rx[port].ecc_err = 0;
				rx[port].var.clk_chg_cnt = 0;
				/*memset(&rx[port].aud_info, 0,*/
					/*sizeof(struct aud_info_s));*/
				/*rx_set_eq_run_state(E_EQ_PASS);*/
				hdmirx_config_video(port);
				rx_get_aud_info(&rx[port].aud_info, port);
				hdmirx_config_audio(port);
				rx_aud_pll_ctl(1, port);
				hdmirx_audio_fifo_rst(port);
				rx[port].hdcp.hdcp_pre_ver = rx[port].hdcp.hdcp_version;
				rx[port].stable_timestamp = rx_info.timestamp;
				rx_pr("Sig ready\n");
				dump_state(RX_DUMP_VIDEO, port);
				#ifdef K_TEST_CHK_ERR_CNT
				//rx_monitor_error_cnt_start();
				#endif
				rx[port].var.sig_stable_err_cnt = 0;
				rx[port].var.esd_phy_rst_cnt = 0;
				//reset_pcs_flag = 1;
				rx[port].ddc_filter_en = false;
			}
		} else {
			rx_irq_en(IRQ_EN_HDCP, port);
			rx[port].var.sig_stable_cnt = 0;
			rx[port].var.de_stable = false;
			if (rx[port].var.sig_unstable_cnt < sig_unstable_max) {
				rx[port].var.sig_unstable_cnt++;
				if (log_level & PHY_LOG)
					rx_pr("DE not stable\n");
				if (!rx[port].var.frl_rate && rx_phy_level & 0x1 &&
					(rx[port].var.sig_unstable_cnt >= reset_pcs_flag &&
					rx[port].var.sig_unstable_cnt <=
					reset_pcs_flag + reset_pcs_cnt)) {
					reset_pcs(port);
					//reset_pcs_flag = 0;
					if (log_level & PHY_LOG)
						rx_pr("reset pcs\n");
				}
				break;
			}
			if (rx[port].err_rec_mode == ERR_REC_EQ_RETRY &&
				rx[port].var.frl_rate == 0) {
				rx[port].state = FSM_WAIT_CLK_STABLE;
				rx[port].phy.cablesel++;
				if (rx[port].var.esd_phy_rst_cnt++ >= esd_phy_rst_max) {
					rx[port].err_rec_mode = ERR_REC_HPD_RST;
					rx_set_eq_run_state(E_EQ_START, port);
					rx[port].var.esd_phy_rst_cnt = 0;
				}
			} else if (rx[port].err_rec_mode == ERR_REC_HPD_RST &&
				rx[port].var.frl_rate == 0 && port != rx_info.arc_port) {
				rx_set_cur_hpd(0, 2, port);
				rx[port].clk.cable_clk = 0;
				rx[port].state = FSM_INIT;
				rx[port].err_rec_mode = ERR_REC_EQ_RETRY;
			}
		}
		break;
	case FSM_SIG_READY:
		rx_get_video_info(port);
		rx[port].err_rec_mode = ERR_REC_EQ_RETRY;
		#ifdef K_TEST_CHK_ERR_CNT
		//rx_monitor_error_counter(port);
		#endif
		/* video info change */
		if (rx[port].var.frl_rate) {
			if (rx_get_clkready_sts(port) == 0 && rx_get_valid_m_sts(port))
				rx_21_fpll_cfg(rx[port].var.frl_rate, port);
		}
		if (!is_tmds_valid(port)) {
			if (video_mute_enabled(port)) {
				set_video_mute(HDMI_RX_MUTE_SET, true);
				rx_mute_vpp(rx_get_port_type(port));
				rx[port].vpp_mute = true;
				rx[port].var.mute_cnt = 0;
				if (log_level & 0x100)
					rx_pr("vpp mute\n");
			}
			skip_frame(skip_frame_cnt, port, "fsm3 tmds skip");
			rx[port].unready_timestamp = rx_info.timestamp;
			rx_i2c_div_init();
			dump_unnormal_info(port);
			rx_pr("tmds_invalid-->unready\n");
			rx[port].var.de_stable = false;
			rx[port].var.sig_unready_cnt = 0;
			rx[port].aud_info.real_sr = 0;
			rx_aud_pll_ctl(0, port);
			rx[port].hdcp.hdcp_pre_ver = rx[port].hdcp.hdcp_version;
			/* need to clr to none, for dishNXT box */
			rx[port].hdcp.hdcp_version = HDCP_VER_NONE;
			//rx_sw_reset(2);
			rx_irq_en(0, port);
			hdmirx_output_en(false);
			if (frl_debug_en == 2)
				rx[port].state = FSM_COR_RESET;
			else
				rx[port].state = FSM_FRL_FLT_READY;
			rx[port].var.vic_check_en = false;
			rx[port].skip = 0;
			rx[port].var.mute_cnt = 0;
			rx[port].aud_sr_stable_cnt = 0;
			rx[port].aud_sr_unstable_cnt = 0;
			rx[port].clk.cable_clk = 0;
			rx[port].hdcp.stream_type = 0;
			rx[port].var.esd_phy_rst_cnt = 0;
			break;
		} else if (!rx_is_timing_stable(port)) {
			skip_frame(skip_frame_cnt, port, "fsm3 timing skip");
			if (sig_unready_max)
				chk_cnt = sig_unready_max;
			else
				chk_cnt = get_frame_interval_cnt(1, port);
			if (++rx[port].var.sig_unready_cnt >= chk_cnt) {
				/*sig_lost_lock_cnt = 0;*/
				rx[port].unready_timestamp = rx_info.timestamp;
				rx_i2c_div_init();
				dump_unnormal_info(port);
				rx_pr("timing unstable-->unready\n");
				rx[port].var.de_stable = false;
				rx[port].var.sig_unready_cnt = 0;
				rx[port].aud_info.real_sr = 0;
				rx_aud_pll_ctl(0, port);
				rx[port].hdcp.hdcp_pre_ver = rx[port].hdcp.hdcp_version;
				/* need to clr to none, for dishNXT box */
				rx[port].hdcp.hdcp_version = HDCP_VER_NONE;
				//rx_sw_reset(2, port);
				rx_irq_en(0, port);
				hdmirx_output_en(false);
				if (frl_debug_en == 2)
					rx[port].state = FSM_COR_RESET;
				else
					rx[port].state = FSM_FRL_FLT_READY;
				rx[port].var.vic_check_en = false;
				rx[port].skip = 0;
				rx[port].var.mute_cnt = 0;
				rx[port].aud_sr_stable_cnt = 0;
				rx[port].aud_sr_unstable_cnt = 0;
				rx[port].clk.cable_clk = 0;
				rx[port].hdcp.stream_type = 0;
				rx[port].var.esd_phy_rst_cnt = 0;
				break;
			}
		} else if (!rx_is_color_space_stable(port)) {
			//Color space changes, no need to do EQ training
			skip_frame(skip_frame_cnt, port, "fsm3 color skip");
			if (sig_unready_max)
				chk_cnt = sig_unready_max;
			else
				chk_cnt = get_frame_interval_cnt(1, port);
			if (++rx[port].var.sig_unready_cnt >= chk_cnt) {
				/*sig_lost_lock_cnt = 0;*/
				rx[port].unready_timestamp = rx_info.timestamp;
				rx_i2c_div_init();
				rx_pr("colorspace changes from %d to %d\n",
					  rx[port].pre.colorspace, rx[port].cur.colorspace);
				rx[port].var.de_stable = false;
				rx[port].hdcp.hdcp_pre_ver = rx[port].hdcp.hdcp_version;
				/* need to clr to none, for dishNXT box */
				hdmirx_output_en(false);
				rx[port].state = FSM_SIG_WAIT_STABLE;
				break;
			}
		} else if (!rx_fps_is_changed(port) && gcp_mute_flag[port] &&
		!rx[port].free_sync_sts && !rx[port].vtem_info.vrr_en) {
			if (++rx[port].var.fps_unready_cnt >= fps_unready_max) {
				rx[port].gcp_mute_cnt = gcp_mute_cnt;
				rx[port].pre.frame_rate = rx[port].cur.frame_rate;
				rx[port].var.fps_unready_cnt = 0;
				gcp_mute_flag[port] = 0;
				break;
			}
		} else {
			rx[port].var.sig_unready_cnt = 0;
			if (rx[port].skip > 0) {
				rx[port].skip--;
			} else if (vpp_mute_enable) {
				if (rx[port].gcp_mute_cnt > 0) {
					rx[port].gcp_mute_cnt--;
					break;
				}
				if (rx[port].vpp_mute_cnt > 0) {
					rx[port].vpp_mute_cnt--;
					break;
				}
				/* clear vpp mute after signal stable */
				if (get_video_mute_val(HDMI_RX_MUTE_SET) &&
					port != rx_info.sub_port) {
					if (rx[port].var.mute_cnt++ <
						get_frame_interval_cnt(1, port))
						break;
					rx[port].var.mute_cnt = 0;
					rx[port].vpp_mute = false;
					set_video_mute(HDMI_RX_MUTE_SET, false);
				}
			}
		}
		if (rx[port].pre.sw_dvi == 1)
			break;
		//packet_update(port);
		rx_get_aud_info(&rx[port].aud_info, port);

		if (check_real_sr_change(port))
			rx_audio_pll_sw_update();
		if (is_aud_pll_error()) {
			rx[port].aud_sr_unstable_cnt++;
			if (rx[port].aud_sr_unstable_cnt > aud_sr_stb_max) {
				unsigned int aud_sts = rx_get_aud_pll_err_sts(port);

				if (aud_sts == E_REQUESTCLK_ERR) {
					hdmirx_phy_init(port);
					rx[port].state = FSM_WAIT_CLK_STABLE;
					/*timing sw at same FRQ*/
					rx[port].clk.cable_clk = 0;
					rx_pr("reqclk err->wait_clk\n");
				} else if (aud_sts == E_PLLRATE_CHG) {
					rx_aud_pll_ctl(1, port);
				} else if (aud_sts == E_AUDCLK_ERR) {
					rx_audio_bandgap_rst();
				} else {
					rx_acr_info_sw_update();
					rx_audio_pll_sw_update();
				}
				if (log_level & AUDIO_LOG)
					rx_pr("update audio-err:%d\n", aud_sts);
				rx[port].aud_sr_unstable_cnt = 0;
			}
		} else if (is_aud_fifo_error()) {
			rx[port].aud_sr_unstable_cnt++;
			if (rx[port].aud_sr_unstable_cnt > aud_sr_stb_max) {
				hdmirx_audio_fifo_rst(port);
				rx[port].aud_sr_unstable_cnt = 0;
			}
		} else {
			rx[port].aud_sr_unstable_cnt = 0;
		}
		break;
	default:
		break;
	}
	/* for fsm debug */
	if (rx[port].state != rx[port].pre_state) {
		rx_pr("fsm3 (%s) to (%s)\n",
			      fsm_st[rx[port].pre_state],
			      fsm_st[rx[port].state]);
		rx[port].pre_state = rx[port].state;
	}
}

static void show_boot_info(void)
{
	u8 i;

	for (i = 0; i < MAP_ADDR_MODULE_NUM; ++i)
		rx_pr("phy_addr = 0x%x, size = 0x%x, maped:%px\n",
			rx_reg_maps[i].phy_addr, rx_reg_maps[i].size,
			rx_reg_maps[i].p);
	for (i = 0; i < 30; ++i) {
		if (boot_info[i][0] == '\0')
			break;
		rx_pr("%s\n", boot_info[i]);
	}
	rx_pr("pkt_buffa_a=0x%p\n", rx_info.emp_buff_a.store_a);
	rx_pr("pkt_buffa_b=0x%p\n", rx_info.emp_buff_a.store_b);
	//page_address
	rx_pr("buffa_a paddr=0x%p\n",
		(void *)rx_info.emp_buff_a.p_addr_a);
	rx_pr("buffa_b paddr=0x%p\n",
		(void *)rx_info.emp_buff_a.p_addr_b);
	rx_pr("pkt_buffb_a=0x%p\n", rx_info.emp_buff_b.store_a);
	rx_pr("pkt_buffb_b=0x%p\n", rx_info.emp_buff_b.store_b);
	//page_address
	rx_pr("buffb_a paddr=0x%p\n",
		(void *)rx_info.emp_buff_b.p_addr_a);
	rx_pr("buffb_b paddr=0x%p\n",
		(void *)rx_info.emp_buff_b.p_addr_b);
}

unsigned int hdmirx_show_info(unsigned char *buf, int size, u8 port)
{
	int pos = 0;
	struct drm_infoframe_st *drmpkt;

	drmpkt = (struct drm_infoframe_st *)&rx_pkt[port].drm_info;

	pos += snprintf(buf + pos, size - pos,
			"port: %d\n", port);
	pos += snprintf(buf + pos, size - pos,
		"HDMI info\n\n");
	if (rx[port].cur.colorspace == E_COLOR_RGB)
		pos += snprintf(buf + pos, size - pos,
			"Color Space: %s\n", "0-RGB");
	else if (rx[port].cur.colorspace == E_COLOR_YUV422)
		pos += snprintf(buf + pos, size - pos,
			"Color Space: %s\n", "1-YUV422");
	else if (rx[port].cur.colorspace == E_COLOR_YUV444)
		pos += snprintf(buf + pos, size - pos,
			"Color Space: %s\n", "2-YUV444");
	else if (rx[port].cur.colorspace == E_COLOR_YUV420)
		pos += snprintf(buf + pos, size - pos,
			"Color Space: %s\n", "3-YUV420");
	pos += snprintf(buf + pos, size - pos,
		"Dvi: %d\n", rx[port].cur.hw_dvi);
	pos += snprintf(buf + pos, size - pos,
		"Interlace: %d\n", rx[port].cur.interlaced);
	pos += snprintf(buf + pos, size - pos,
		"Htotal: %d\n", rx[port].cur.htotal);
	pos += snprintf(buf + pos, size - pos,
		"Hactive: %d\n", rx[port].cur.hactive);
	pos += snprintf(buf + pos, size - pos,
		"Vtotal: %d\n", rx[port].cur.vtotal);
	pos += snprintf(buf + pos, size - pos,
		"Vactive: %d\n", rx[port].cur.vactive);
	pos += snprintf(buf + pos, size - pos,
		"Repetition: %d\n", rx[port].cur.repeat);
	pos += snprintf(buf + pos, size - pos,
		"Color Depth: %d\n", rx[port].cur.colordepth);
	pos += snprintf(buf + pos, size - pos,
		"Frame Rate: %d\n", rx[port].cur.frame_rate);
	pos += snprintf(buf + pos, size - pos,
		"Skip frame: %d\n", rx[port].skip);
	pos += snprintf(buf + pos, size - pos,
		"avmute skip: %d\n", rx[port].avmute_skip);
	pos += snprintf(buf + pos, size - pos,
		"TMDS clock: %d\n", rx[port].clk.cable_clk);
	if (rx_info.chip_id < CHIP_ID_T7)
		pos += snprintf(buf + pos, size - pos,
			"Pixel clock: %d\n", rx[port].clk.pixel_clk);
	if (drmpkt->des_u.tp1.eotf == EOTF_SDR)
		pos += snprintf(buf + pos, size - pos,
		"HDR EOTF: %s\n", "SDR");
	else if (drmpkt->des_u.tp1.eotf == EOTF_HDR)
		pos += snprintf(buf + pos, size - pos,
		"HDR EOTF: %s\n", "HDR");
	else if (drmpkt->des_u.tp1.eotf == EOTF_SMPTE_ST_2048)
		pos += snprintf(buf + pos, size - pos,
		"HDR EOTF: %s\n", "SMPTE_ST_2048");
	else if (drmpkt->des_u.tp1.eotf == EOTF_HLG)
		pos += snprintf(buf + pos, size - pos,
		"HDR EOTF: %s\n", "HLG");
	pos += snprintf(buf + pos, size - pos,
		"Dolby Vision: %d\n",
		rx[port].vs_info_details.dolby_vision_flag);

	pos += snprintf(buf + pos, size - pos,
		"\n\nAudio info\n\n");
	pos += snprintf(buf + pos, size - pos,
		"CTS: %d\n", rx[port].aud_info.cts);
	pos += snprintf(buf + pos, size - pos,
		"N: %d\n", rx[port].aud_info.n);
	pos += snprintf(buf + pos, size - pos,
		"Recovery clock: %d\n", rx[port].aud_info.arc);
	pos += snprintf(buf + pos, size - pos,
		"audio receive data: %d\n", rx[port].aud_info.aud_packet_received);
	pos += snprintf(buf + pos, size - pos,
		"Audio PLL clock: %d\n", rx[port].clk.aud_pll);
	if (rx_info.chip_id <= CHIP_ID_TXLX)
		pos += snprintf(buf + pos, size - pos,
			"mpll_div_clk: %d\n", rx[port].clk.mpll_clk);
	pos += snprintf(buf + pos, size - pos,
		"edid_select_ver: %s\n", rx[port].edid_type.cfg == EDID_V20 ? "2.0" : "1.4");

	pos += snprintf(buf + pos, size - pos,
		"\n\nHDCP info\n\n");
	pos += snprintf(buf + pos, size - pos,
		"HDCP Debug Value: 0x%x\n", hdmirx_rd_dwc(DWC_HDCP_DBG));
	pos += snprintf(buf + pos, size - pos,
		"HDCP14 state: %d\n", rx[port].cur.hdcp14_state);
	pos += snprintf(buf + pos, size - pos,
		"HDCP22 state: %d\n", rx[port].cur.hdcp22_state);

	pos += snprintf(buf + pos, size - pos,
		"\n\nCapacity info\n\n");
	pos += snprintf(buf + pos, size - pos,
		"support vrr: %d\n", rx_info.edid_cap.vrr);
	pos += snprintf(buf + pos, size - pos,
		"support allm: %d\n", rx_info.edid_cap.allm);
	pos += snprintf(buf + pos, size - pos,
		"support HF_DB: %d\n", rx_info.edid_cap.hf_db);
	pos += snprintf(buf + pos, size - pos,
		"support DV_DB: %d\n", rx_info.edid_cap.dv_db);
	pos += snprintf(buf + pos, size - pos,
		"support HDR10P_DB: %d\n", rx_info.edid_cap.hdr10p_db);
	pos += snprintf(buf + pos, size - pos,
		"support HDR_STATIC_DB: %d\n", rx_info.edid_cap.hdr_static_db);
	pos += snprintf(buf + pos, size - pos,
		"support HDR_DYNAMIC_DB: %d\n", rx_info.edid_cap.hdr_dynamic_db);
	pos += snprintf(buf + pos, size - pos,
		"support FREESYNC_DB: %d\n", rx_info.edid_cap.freesync_db);

	if (port == E_PORT0)
		pos += snprintf(buf + pos, size - pos,
			"Source Physical address: %d.0.0.0\n", 1);
	else if (port == E_PORT1)
		pos += snprintf(buf + pos, size - pos,
			"Source Physical address: %d.0.0.0\n", 3);
	else if (port == E_PORT2)
		pos += snprintf(buf + pos, size - pos,
			"Source Physical address: %d.0.0.0\n", 2);
	else if (port == E_PORT3)
		pos += snprintf(buf + pos, size - pos,
			"Source Physical address: %d.0.0.0\n", 4);
	pos += snprintf(buf + pos, size - pos,
		"HDCP1.4 secure: %d\n", rx_set_hdcp14_secure_key());
	if (rx_info.chip_id >= CHIP_ID_T7)
		return pos;
	if (hdcp22_on) {
		pos += snprintf(buf + pos, size - pos,
			"HDCP22_ON: %d\n", hdcp22_on);
		pos += snprintf(buf + pos, size - pos,
			"HDCP22 sts: 0x%x\n", rx_hdcp22_rd_reg(0x60));
		pos += snprintf(buf + pos, size - pos,
			"HDCP22_capable_sts: %d\n", hdcp22_capable_sts);
		pos += snprintf(buf + pos, size - pos,
			"sts0x8fc: 0x%x\n", hdmirx_rd_dwc(DWC_HDCP22_STATUS));
		pos += snprintf(buf + pos, size - pos,
			"sts0x81c: 0x%x\n", hdmirx_rd_dwc(DWC_HDCP22_CONTROL));
	}

	return pos;
}

static void dump_phy_status(u8 port)
{
	if (rx_info.phy_ver == PHY_VER_TM2)
		dump_aml_phy_sts_tm2();
	else if (rx_info.phy_ver == PHY_VER_T5)
		dump_aml_phy_sts_t5();
	else if (rx_info.phy_ver >= PHY_VER_T7 && rx_info.phy_ver <= PHY_VER_T5W)
		dump_aml_phy_sts_t7();
	else if (rx_info.phy_ver == PHY_VER_T5M)
		dump_aml_phy_sts_t5m();
	else if (rx_info.phy_ver == PHY_VER_TXHD2)
		dump_aml_phy_sts_txhd2();
	else if (rx_info.phy_ver == PHY_VER_T3X)
		dump_aml_phy_sts_t3x(port);
	else
		dump_aml_phy_sts_tl1();
}

static void dump_clk_status(u8 port)
{
	rx_pr("[port:%d HDMI clk info]\n", port);
	rx_pr("top cableclk=%d\n",
	      rx_get_clock(TOP_HDMI_CABLECLK, port));

	rx_pr("top tmdsclk=%d\n",
	      rx_get_clock(TOP_HDMI_TMDSCLK, port));
	rx_pr("cable clock = %d\n",
	      rx[port].clk.cable_clk);
	rx_pr("tmds clock = %d\n",
	      rx[port].clk.tmds_clk);
	rx_pr("audio clock = %d\n",
	      rx[port].clk.aud_pll);
	if (rx_info.chip_id <= CHIP_ID_TXLX)
		rx_pr("mpll clock = %d\n",
		      rx[port].clk.mpll_clk);
}

void dump_video_status(u8 port)
{
	enum edid_ver_e edid_ver =
		rx_parse_edid_ver(rx_get_cur_used_edid(port));

	rx_get_video_info(port);
	rx_pr("[HDMI info]\n");
	if (rx[port].cur.colorspace == 0)
		rx_pr("colorspace:RGB\n");
	else if (rx[port].cur.colorspace == 1)
		rx_pr("colorspace:YUV422\n");
	else if (rx[port].cur.colorspace == 2)
		rx_pr("colorspace:YUV444\n");
	else if (rx[port].cur.colorspace == 3)
		rx_pr("colorspace:YUV420\n");
	else
		rx_pr("colorspace:%d\n", rx[port].cur.colorspace);
	rx_pr("colordepth %d\n", rx[port].cur.colordepth);
	rx_pr("interlace %d\n", rx[port].cur.interlaced);
	rx_pr("htotal %d\n", rx[port].cur.htotal);
	rx_pr("hactive %d\n", rx[port].cur.hactive);
	rx_pr("vtotal %d\n", rx[port].cur.vtotal);
	rx_pr("vactive %d\n", rx[port].cur.vactive);
	rx_pr("frame_rate %d\n", rx[port].cur.frame_rate);
	rx_pr("repetition %d\n", rx[port].cur.repeat);
	rx_pr("dvi %d,", rx[port].cur.hw_dvi);
	rx_pr("sw_dvi:%d,", rx[port].pre.sw_dvi);
	rx_pr("fmt=0x%x,", hdmirx_hw_get_fmt(port));
	rx_pr("hw_vic %d,", rx[port].cur.hw_vic);
	rx_pr("sw_vic %d,", rx[port].pre.sw_vic);
	rx_pr("rx[port_idx].no_signal=%d,rx[port_idx].state=%d,",
	      rx[port].no_signal, rx[port].state);
	rx_pr("VRR en = %d\n", rx[port].vtem_info.vrr_en);
	rx_pr("skip frame=%d\n", rx[port].skip);
	rx_pr("avmute_skip:0x%x\n", rx[port].avmute_skip);
	rx_pr("ecc cnt:%d\n", rx[port].ecc_err);
	rx_pr("****pkts_info_details:*****\n");
	rx_pr("hdr10plus = %d\n", rx[port].vs_info_details.hdr10plus);
	rx_pr("hdmi_allm_mode = %d\n", rx[port].vs_info_details.hdmi_allm);
	rx_pr("dv_allm_mode = %d\n", rx[port].vs_info_details.dv_allm);
	rx_pr("itcontent = %d\n", rx[port].cur.it_content);
	rx_pr("cnt_type = %d\n", rx[port].cur.cn_type);
	rx_pr("dolby_vision = %d\n", rx[port].vs_info_details.dolby_vision_flag);
	rx_pr("dv ll = %d\n", rx[port].vs_info_details.low_latency);
	rx_pr("cuva hdr = %d\n", rx[port].vs_info_details.cuva_hdr);
	rx_pr("filmmaker = %d\n", rx[port].vs_info_details.filmmaker);
	rx_pr("iMax = %d\n", rx[port].vs_info_details.imax);
	//rx_pr("VTEM = %d\n", rx[port_idx].vrr_en);
	rx_pr("DRM = %d\n", rx_pkt_chk_attach_drm(port));
	rx_pr("freesync = %d\n-bit0 supported,bit1:enabled.bit2:active",
		rx[port].free_sync_sts);
	rx_pr("0:14; 1:20; 2:Auto20; 4:Auto14\n");
	rx_pr("edid_selected_ver: %d\n", rx[port].edid_type.cfg);
	rx_pr("edid_cur_ver: %d\n", rx[port].edid_type.edid_ver);
	rx_pr("edid_parse_ver: %s\n",  edid_ver == EDID_V20 ? "2.0" : "1.4");
	rx_pr("edid status = %d\n",
		is_valid_edid_data(rx_get_cur_used_edid(port)));
	rx_pr("tx_type: %d\n", rx[port].tx_type);
	if (log_level & EDID_LOG)
		rx_print_edid_support();
}

void dump_audio_status(u8 port)
{
	static struct aud_info_s a;
	//u32 val0, val1;

	rx_get_aud_info(&a, port);
	rx_pr("[AudioInfo]\n");
	rx_pr(" CT=%u CC=%u", a.coding_type,
	      a.channel_count);
	rx_pr(" SF=%u SS=%u", a.sample_frequency,
	      a.sample_size);
	rx_pr(" CA=%u\n", a.auds_ch_alloc);
	rx_pr("CTS=%d, N=%d,", a.cts, a.n);
	rx_pr("acr clk=%d\n", a.arc);
	rx_pr("layout=%d\n", a.auds_layout);
	rx_pr("afifo cfg=%d\n", a.afifo_cfg);
	rx_pr("audio receive data:%d\n", rx[port].aud_info.aud_packet_received);
	rx_pr("aud mute = %d", a.aud_mute_en);
	rx_pr("aud fifo = %d", rx[port].afifo_sts);
	rx_pr("overflow_cnt = %d", afifo_overflow_cnt);
	rx_pr("underflow_cnt = %d", afifo_underflow_cnt);
}

static void dump_hdcp_status(u8 port)
{
	rx_pr("HDCP version:%d\n", rx[port].hdcp.hdcp_version);
	rx_pr("HDCP14 state:%d\n",
	      rx[port].cur.hdcp14_state);
	rx_pr("HDCP22 state:%d\n",
	      rx[port].cur.hdcp22_state);
	if (rx_info.chip_id >= CHIP_ID_T7) {
		if (log_level & HDCP_LOG) {
			rx_pr("14 key loaded-%d\n", is_rx_hdcp14key_loaded_t7());
			rx_pr("22 key loaded-%d\n", is_rx_hdcp22key_loaded_t7());
			rx_pr("14 key crc-%d\n", is_rx_hdcp14key_crc_pass());
			rx_pr("22 key crc0-%d\n", is_rx_hdcp22key_crc0_pass());
			rx_pr("22 key ccrc1-%d\n", is_rx_hdcp22key_crc1_pass());
		}
	}
	if (rx_info.chip_id != CHIP_ID_T7)
		return;
	rx_pr("rpt = %d\n", hdmirx_repeat_support());
	rx_pr("downstream_plug = %d\n", rx[port].hdcp.repeat);
	rx_pr("up is hdcp%dx\n", rx[port].hdcp.hdcp_version);
	rx_pr("ds is hdcp%dx\n", rx[port].hdcp.ds_hdcp_ver);
	rx_pr("bcaps:%x\n",
		  hdmirx_rd_cor(RX_BCAPS_SET_HDCP1X_IVCRX, port));
	rx_pr("dev cnt:%x\n",
		  hdmirx_rd_cor(RX_SHD_BSTATUS1_HDCP1X_IVCRX, port));
	rx_pr("dev depth:%x\n",
		  hdmirx_rd_cor(RX_SHD_BSTATUS2_HDCP1X_IVCRX, port) & 0xf);
	rx_pr("upstream stream_type:%x\n", rx[port].hdcp.stream_type);
	//rx_pr("sha length:%x\n",
		 // hdmirx_rd_cor(RX_SHA_length1_HDCP1X_IVCRX, port));
	//rx_pr("fifo addr:%x\n",
		 // hdmirx_rd_cor(RX_KSV_SHA_start1_HDCP1X_IVCRX, port));
}

void dump_state(int enable, u8 port)
{
	rx_pr("dump port=%d\n", port);
	//rx_get_video_info(port);
	/* video info */
	if (enable == RX_DUMP_VIDEO) {
		dump_video_status(port);
	} else if (enable & RX_DUMP_ALL) {
		dump_clk_status(port);
		dump_phy_status(port);
		dump_video_status(port);
		dump_audio_status(port);
		dump_hdcp_status(port);
		dump_pktinfo_status(port);
	} else if (enable & RX_DUMP_AUDIO) {
		/* audio info */
		dump_audio_status(port);
	} else if (enable & RX_DUMP_HDCP) {
		/* hdcp info */
		dump_hdcp_status(port);
	} else if (enable & RX_DUMP_PHY) {
		/* phy info */
		dump_phy_status(port);
	} else if (enable & RX_DUMP_CLK) {
		/* clk src info */
		dump_clk_status(port);
	} else {
		dump_video_status(port);
	}
}

void rx_debug_help(void)
{
	rx_pr("*****************\n");
	rx_pr("reset0--hw_config\n");
	rx_pr("reset1--8bit phy rst\n");
	rx_pr("reset3--irq open\n");
	rx_pr("reset4--edid_update\n");
	rx_pr("reset5--esm rst\n");
	rx_pr("database--esm data addr\n");
	rx_pr("duk--dump duk\n");
	rx_pr("v - driver version\n");
	rx_pr("state0 -dump video\n");
	rx_pr("state1 -dump audio\n");
	rx_pr("state2 -dump hdcp\n");
	rx_pr("state3 -dump phy\n");
	rx_pr("state4 -dump clock\n");
	rx_pr("statex -dump all\n");
	rx_pr("port1/2/3 -port switch\n");
	rx_pr("hpd0/1 -set hpd 0:low\n");
	rx_pr("cable_status -5V sts\n");
	rx_pr("pause -pause fsm\n");
	rx_pr("reg -dump all dwc reg\n");
	rx_pr("*****************\n");
}

int hdmirx_debug(const char *buf, int size)
{
	char tmpbuf[256];
	int i = 0;
	int j = 0;
	u32 value = 0;

	char input[5][20];
	char *const delim = " ";
	char *token;
	char *cur;
	int cnt = 0;
	struct edid_info_s edid_info;
	u8 port = 0;
	u_char *pedid = NULL;
	struct emp_info_s *emp_p = NULL;

	while ((buf[i]) && (buf[i] != ',') && (buf[i] != ' ')) {
		tmpbuf[i] = buf[i];
		i++;
	}
	tmpbuf[i] = 0;

	for (cnt = 0; cnt < 5; cnt++)
		input[cnt][0] = '\0';

	cur = (char *)buf;
	cnt = 0;
	while ((token = strsep(&cur, delim)) && (cnt < 5)) {
		if (strlen((char *)token) < 20)
			strcpy(&input[cnt][0], (char *)token);
		else
			rx_pr("err input\n");
		cnt++;
	}
	if (strncmp(input[cnt - 1], "0", 1) == 0)
		port = E_PORT0;
	else if (strncmp(input[cnt - 1], "1", 1) == 0)
		port = E_PORT1;
	else if (strncmp(input[cnt - 1], "2", 1) == 0)
		port = E_PORT2;
	else if (strncmp(input[cnt - 1], "3", 1) == 0)
		port = E_PORT3;

	pedid = rx_get_cur_used_edid(port);

	if (strncmp(tmpbuf, "help", 4) == 0) {
		rx_debug_help();
	} else if (strncmp(tmpbuf, "hpd", 3) == 0) {
		rx_set_cur_hpd((tmpbuf[3] == '0' ? 0 : 1), 4, port);
	} else if (strncmp(tmpbuf, "reset", 5) == 0) {
		if (tmpbuf[5] == '0') {
			rx_pr(" hdmirx hw config\n");
			/* hdmi_rx_top_edid_update(); */
			hdmirx_hw_config(port);
		} else if (tmpbuf[5] == '1') {
			rx_pr(" hdmirx phy init 8bit\n");
			hdmirx_phy_init(port);
		} else if (tmpbuf[5] == '3') {
			rx_pr("cor reset\n");
			hdmirx_wr_top(TOP_SW_RESET, 1, port);
			udelay(1);
			hdmirx_wr_top(TOP_SW_RESET, 0, port);
		} else if (tmpbuf[5] == '4') {
			if (kstrtou32(tmpbuf + 6, 16, &value) < 0)
				return -EINVAL;
#ifdef CONFIG_AMLOGIC_HDMITX
			tx_hdr_priority = value;
#endif
			rx_pr(" edid update:%d\n", value);
			hdmi_rx_top_edid_update();
			port_hpd_rst_flag |= (1 << rx_info.main_port);
			rx_set_port_hpd(rx_info.main_port, 0);
		} else if (tmpbuf[5] == '5') {
			hdmirx_hdcp22_esm_rst();
		}
	} else if (strncmp(tmpbuf, "state", 5) == 0) {
		if (tmpbuf[5] == '0')
			dump_state(RX_DUMP_VIDEO, port);
		else if (tmpbuf[5] == '1')
			dump_state(RX_DUMP_AUDIO, port);
		else if (tmpbuf[5] == '2')
			dump_state(RX_DUMP_HDCP, port);
		else if (tmpbuf[5] == '3')
			dump_state(RX_DUMP_PHY, port);
		else if (tmpbuf[5] == '4')
			dump_state(RX_DUMP_CLK, port);
		else
			dump_state(RX_DUMP_ALL, port);
	} else if (strncmp(tmpbuf, "pause", 5) == 0) {
		if (kstrtou32(tmpbuf + 5, 10, &value) < 0)
			return -EINVAL;
		rx_pr("%s\n", value ? "pause" : "enable");
		sm_pause = value;
	} else if (strncmp(tmpbuf, "reg", 3) == 0) {
		if (tmpbuf[3] == '1')
			dump_reg_phy(port);
		else
			dump_reg(port);
	}  else if (strncmp(tmpbuf, "duk", 3) == 0) {
		rx_pr("hdcp22=%d\n", rx_sec_set_duk(hdmirx_repeat_support()));
	} else if (strncmp(tmpbuf, "edid", 4) == 0) {
		if (tmpbuf[4] == '1')
			dump_edid_reg(512);
		else
			dump_edid_reg(256);
	} else if (strncmp(tmpbuf, "load14key", 7) == 0) {
		rx_debug_loadkey(port);
	} else if (strncmp(tmpbuf, "load22key", 9) == 0) {
		rx_debug_load22key(port);
	} else if (strncmp(tmpbuf, "esm0", 4) == 0) {
		rx_hdcp22_send_uevent(0);
	} else if (strncmp(tmpbuf, "esm1", 4) == 0) {
		/*switch_set_state(&rx[rx_info.main_port].hpd_sdev, 0x01);*/
		rx_hdcp22_send_uevent(1);
	} else if (strncmp(input[0], "pktinfo", 7) == 0) {
		if (strlen(input[0]) == 8)
			rx_debug_pktinfo(input, input[0][7] - '0');
		else
			rx_debug_pktinfo(input, port);
	} else if (strncmp(tmpbuf, "parse_edid", 10) == 0) {
		//if (tmpbuf[10] == '1')
			//pedid = rx_get_cur_def_edid(rx_info.main_port);
		memset(&edid_info, 0, sizeof(struct edid_info_s));
		rx_edid_parse(pedid, &edid_info);
		rx_edid_parse_print(&edid_info);
		rx_blk_index_print(&edid_info.cea_ext_info.blk_parse_info);
	} else if (strncmp(tmpbuf, "splice_db", 9) == 0) {
		if (kstrtou32(tmpbuf + 9, 16, &value) < 0)
			return -EINVAL;
		//edid_splice_data_blk_dbg(pedid, value);
	} else if (strncmp(tmpbuf, "rm_db_tag", 9) == 0) {
		if (kstrtou32(tmpbuf + 9, 16, &value) < 0)
			return -EINVAL;
		edid_rm_db_by_tag(pedid, value);
	} else if (strncmp(tmpbuf, "rm_db_idx", 9) == 0) {
		if (kstrtou32(tmpbuf + 9, 16, &value) < 0)
			return -EINVAL;
		edid_rm_db_by_idx(pedid, value);
	} else if (tmpbuf[0] == 'w') {
		rx_debug_wr_reg(buf, tmpbuf, i, port);
	} else if (tmpbuf[0] == 'r') {
		rx_debug_rd_reg(buf, tmpbuf, port);
	} else if (tmpbuf[0] == 'v') {
		rx_pr("------------------\n");
		rx_pr("Hdmirx version0: %s\n", RX_VER0);
		rx_pr("Hdmirx version1: %s\n", RX_VER1);
		rx_pr("Hdmirx version2: %s\n", RX_VER2);
		rx_pr("------------------\n");
	} else if (strncmp(input[0], "port0", 5) == 0) {
		//hdmirx_open_port(E_PORT0, 0xff);
		rx_info.main_port = E_PORT0;
	} else if (strncmp(input[0], "port1", 5) == 0) {
		//hdmirx_open_port(E_PORT1, 0xff);
		rx_info.main_port = E_PORT1;
	} else if (strncmp(input[0], "port2", 5) == 0) {
		//hdmirx_open_port(E_PORT2,   0xff);
		rx_info.main_port = E_PORT2;
	} else if (strncmp(input[0], "port3", 5) == 0) {
		//hdmirx_open_port(E_PORT3, 0xff);
		rx_info.main_port = E_PORT3;
	} else if (strncmp(input[0], "ports", 5) == 0) {
		//hdmirx_open_port(E_PORT3, E_PORT1);
		rx_info.main_port = E_PORT3;
		rx_info.sub_port = E_PORT1;
	} else if (strncmp(input[0], "empsts", 6) == 0) {
		rx_emp_status(port);
	} else if (strncmp(input[0], "empstart", 8) == 0) {
		rx_emp_capture_stop(port);
		rx_emp_resource_allocate(hdmirx_dev);
		rx_emp_to_ddr_init(port);
	} else if (strncmp(input[0], "tmdsstart", 9) == 0) {
		rx_emp_capture_stop(port);
		rx_tmds_resource_allocate(hdmirx_dev, port);
		rx_tmds_to_ddr_init(port);
	} else if (strncmp(input[0], "empcapture", 10) == 0) {
		rx_emp_data_capture(port);
	} else if (strncmp(input[0], "tmdscapture", 11) == 0) {
		rx_tmds_data_capture(port);
	} else if (strncmp(input[0], "tmdscnt", 7) == 0) {
		if (kstrtou32(input[1], 16, &value) < 0)
			rx_pr("error input Value\n");
		rx_pr("set pkt cnt:0x%x\n", value);
		rx_info.emp_buff_a.tmds_pkt_cnt = value;
	} else if (strncmp(input[0], "phyinit", 7) == 0) {
		hdmirx_phy_init(port);
	} else if (strncmp(input[0], "phyeq", 5) == 0) {
		//aml_eq_setting();
		find_best_eq = 0x1111;
		rx[port].phy.err_sum = 0xffffff;
	} else if (strncmp(tmpbuf, "audio", 5) == 0) {
		hdmirx_audio_fifo_rst(port);
	} else if (strncmp(tmpbuf, "eqcal", 5) == 0) {
		rx_phy_rt_cal();
	} else if (strncmp(tmpbuf, "empbuf", 5) == 0) {
		emp_p = rx[port].emp_vid_idx ? &rx_info.emp_buff_b : &rx_info.emp_buff_a;
		rx_pr("cnt=%d\n", emp_p->emp_pkt_cnt);
		cnt = emp_p->emp_pkt_cnt;
		for (i = 0; i < cnt; i++) {
			memset(tmpbuf, '\0', sizeof(tmpbuf));
			for (j = 0; j < 32; j++)
				sprintf(tmpbuf + strlen(tmpbuf), "0x%02x ",
					emp_buf[rx[port].emp_vid_idx][j + i * 32]);
			rx_pr("%s\n", tmpbuf);
		}
		rx_pr("ieee=%x\n", emp_p->emp_tagid);
	} else if (strncmp(tmpbuf, "muteget", 7) == 0) {
		rx_pr("mute sts: %x\n", get_video_mute_val(HDMI_RX_MUTE_SET));
	} else if (strncmp(tmpbuf, "muteset", 7) == 0) {
		if (tmpbuf[7] == '0')
			set_video_mute(HDMI_RX_MUTE_SET, false);
		else
			set_video_mute(HDMI_RX_MUTE_SET, true);
	} else if (strncmp(tmpbuf, "bist", 4) == 0) {
		if (tmpbuf[4] == '1') {
			rx_set_color_bar(true, tmpbuf[5] - '0', port);
			//hdmirx_set_video_mute(1, port);
		} else if (tmpbuf[4] == '0') {
			rx_set_color_bar(false, tmpbuf[5] - '0', port);
			//hdmirx_set_video_mute(0, port);
		} else {
			rx_phy_short_bist(port);
		}
	} else if (strncmp(tmpbuf, "eye", 3) == 0) {
		sm_pause = 1;
		aml_eq_eye_monitor(port);
		sm_pause = 0;
	} else if (strncmp(tmpbuf, "iq", 2) == 0) {
		aml_phy_iq_skew_monitor();
	} else if (strncmp(tmpbuf, "crc", 3) == 0) {
		rx_hdcp_crc_check();
	} else if (strncmp(tmpbuf, "phyoff", 6) == 0) {
		aml_phy_power_off();
	} else if (strncmp(tmpbuf, "clkoff", 6) == 0) {
		if (tmpbuf[6] == '1')
			rx_dig_clk_en(1);
		else
			rx_dig_clk_en(0);
	} else if (strncmp(tmpbuf, "frlcfg", 6) == 0) {
		hdmirx_wr_cor(DPLL_CFG6_DPLL_IVCRX, 0x0, port);
		hdmirx_frl_config(port);
		rx_lts_2_flt_ready(port);
	}  else if (strncmp(tmpbuf, "frltrn", 6) == 0) {
		hdmi_tx_rx_frl_training_main(port);
	} else if (strncmp(tmpbuf, "cvtem", 5) == 0) {
		dump_cvtem_packet(port);
	} else if (strncmp(tmpbuf, "parse_pps", 9) == 0) {
		parse_dsc_pps_data(rx[port].cvtem_info.dsc_info, port);
		dump_dsc_pps_info(port);
	} else if (strncmp(tmpbuf, "prbs", 4) == 0) {
		rx_long_bist_t3x();
	} else if (strncmp(tmpbuf, "l_bist", 6) == 0) {
		rx_long_bist_t3x();
	} else if (strncmp(tmpbuf, "aud21", 5) == 0) {
		dump_aud21_param(E_PORT2);
	} else if (strncmp(tmpbuf, "fpll", 4) == 0) {
		rx_21_fpll_cfg(rx[port].var.frl_rate, port);
	} else if (strncmp(tmpbuf, "wake", 4) == 0) {
		hdmirx_wr_bits_top_common(TOP_EDID_RAM_OVR0_DATA, _BIT(0), 1);
	} else if (strncmp(tmpbuf, "aud_cal", 7) == 0) {
		txhd2_aud_clk_cal();
	} else if (strncmp(tmpbuf, "info", 4) == 0) {
		show_boot_info();
	} else if (strncmp(tmpbuf, "reg_map", 7) == 0) {
		for (i = 0; i < MAP_ADDR_MODULE_NUM; i++) {
			rx_pr("phy_addr = 0x%x, size = 0x%x, maped:%px\n",
				rx_reg_maps[i].phy_addr, size, rx_reg_maps[i].p);
		}
	} else if (strncmp(tmpbuf, "i2c_monitor", 10) == 0) {
		rx_i2c_monitor(6 + port, E_FUNC_SAMPLE, E_SW_TRIGGER, E_DUMP_ALL);
		rx_pr("i2c_monitor start\n");
	} else if (strncmp(tmpbuf, "i2c_dump", 7) == 0) {
		rx_pr("i2c_dump\n");
		rx_i2c_dump();
	} else if (strncmp(tmpbuf, "self_hsync", 10) == 0) {
		if (tmpbuf[10] == '1') {
			rx[port].dsc_flag = true;
			rx_switch_to_self_hsync(port, true);
		} else {
			rx[port].dsc_flag = false;
			rx_switch_to_self_hsync(port, false);
		}
	}
	return 0;
}

void rx_ext_state_monitor(u8 port)
{
	if (rx[port].fsm_ext_state != FSM_NULL) {
		rx[port].state = rx[port].fsm_ext_state;
		rx[port].fsm_ext_state = FSM_NULL;
		if (rx[port].state != rx[port].pre_state) {
			if (log_level & LOG_EN)
				rx_pr("fsm %d (%s) to (%s)\n",
				port,
				fsm_st[rx[port].pre_state],
				fsm_st[rx[port].state]);
			rx[port].pre_state = rx[port].state;
		}
	}
}

void rx_edid_update_handler(struct work_struct *dwork)
{
	struct edid_update_work_s *p = container_of(dwork, struct edid_update_work_s, work);
	u8 port;

	if (!p)
		return;
	port = p->port;
	rx_pr("port%d edid update to %d\n", port, rx[port].edid_type.edid_ver);
	hdmi_rx_top_edid_update();
	if (rx[port].tx_type != DEV_ABNORMAL_SCDC)
		rx[port].fsm_ext_state = FSM_HPD_LOW;
}

void rx_edid_monitor(void)
{
	int port;

	if (!edid_auto_sel)
		return;
	for (port = E_PORT0; port < rx_info.port_num; port++)
		edid_type_update(port);
}

//for debug only
int vm = 0xf;
void valid_m_monitor(u8 port)
{
	int val_m = 0;

	val_m = hdmirx_rd_cor(0x1525, port) & 0x1;
	if (val_m != vm) {
		vm = val_m;
		if (log_level & FRL_LOG)
			rx_pr("port-%d valid_m change to %d\n", port, val_m);
	}
}

int vm1 = 0xf;
void valid_m_monitor1(u8 port)
{
	int val_m = 0;

	val_m = hdmirx_rd_cor(0x1525, port) & 0x1;
	if (val_m != vm1) {
		vm1 = val_m;
		if (log_level & FRL_LOG)
			rx_pr("port-%d valid_m change to %d\n", port, val_m);
	}
}

static int frate_flg = 0xf;
void frate_monitor(void)
{
	u8 port = E_PORT2;
	static int lock;
	static int pre_lock;

	rx[port].var.frl_rate = hdmirx_rd_cor(SCDCS_CONFIG1_SCDC_IVCRX, port) & 0xf;
	lock = hdmirx_rd_cor(SCDCS_STATUS_FLAGS0_SCDC_IVCRX, port);
	if (lock != pre_lock) {
		rx_pr("lock = 0x%x\n", lock);
		pre_lock = lock;
	}
	if (rx[port].var.frl_rate != frate_flg) {
		frate_flg = rx[port].var.frl_rate;
		if (rx[port].var.frl_rate) {
			if (fpll_chk_lvl & 0xf) {
				cor_init(port);
			} else if (((fpll_chk_lvl >> 4) & 0xf) == 0x1) {
				hdmirx_hw_config(port);
			} else if (((fpll_chk_lvl >> 4) & 0xf) == 0x2) {
				if (rx[port].var.frl_rate) {
					//frl_debug todo
					hdmirx_wr_cor(DPLL_CFG6_DPLL_IVCRX, 0x0, port);
					hdmirx_wr_cor(H21RXSB_D2TH_M42H_IVCRX, 0x20, port);
					hdmirx_wr_bits_cor(H21RXSB_GP1_REGISTER_M42H_IVCRX,
						_BIT(3), 1, port);
					//clk ready threshold
					hdmirx_wr_cor(H21RXSB_DIFF1T_M42H_IVCRX, 0x20, port);
				} else {
					hdmirx_wr_cor(DPLL_CFG6_DPLL_IVCRX, 0x10, port);
					hdmirx_wr_cor(RX_H21_CTRL_PWD_IVCRX, 0x0, port);
				}
			}
			if (rx[port].state == FSM_SIG_READY) {
				//hdmirx_top_irq_en(0, 0, port);
				rx_irq_en(0, port);
			}
			rx[port].state = FLT_RX_LTS_3;
		} else {
			if (rx[port].state == FSM_SIG_READY) {
				//hdmirx_top_irq_en(0, 0, port);
				rx_irq_en(0, port);
			}
			if (rx[port].state > FSM_FRL_FLT_READY)
				rx[port].state = FSM_FRL_FLT_READY;
		}
		if (log_level & FRL_LOG)
			rx_pr("port-%d frate change to %d\n", port, rx[port].var.frl_rate);
	} else {
		if (rx[port].var.frl_rate) {
			if ((abs(rx[port].clk.t_clk_pre - rx[port].clk.tclk) > 10 * MHz)) {
				if (rx[port].state == FSM_SIG_READY) {
					rx[port].state = FSM_FRL_FLT_READY;
					//hdmirx_top_irq_en(0, 0, port);
					rx_irq_en(0, port);
				}
			}
		}
	}
	//rx_monitor_error_counter(port);
	if (rx[port].var.frl_rate)
		valid_m_monitor(port);
}

static int frate_flg1 = 0xf;
void frate_monitor1(void)
{
	u8 port = E_PORT3;
	static int lock;
	static int pre_lock;

	rx[port].var.frl_rate = hdmirx_rd_cor(SCDCS_CONFIG1_SCDC_IVCRX, port) & 0xf;
	lock = hdmirx_rd_cor(SCDCS_STATUS_FLAGS0_SCDC_IVCRX, port);
	if (lock != pre_lock) {
		rx_pr("lock = 0x%x\n", lock);
		pre_lock = lock;
	}
	if (rx[port].var.frl_rate != frate_flg1) {
		frate_flg1 = rx[port].var.frl_rate;
		if (rx[port].var.frl_rate) {
			if (fpll_chk_lvl & 0xf) {
				cor_init(port);
			} else if (((fpll_chk_lvl >> 4) & 0xf) == 0x1) {
				hdmirx_hw_config(port);
			} else if (((fpll_chk_lvl >> 4) & 0xf) == 0x2) {
				if (rx[port].var.frl_rate) {
					//frl_debug todo
					hdmirx_wr_cor(DPLL_CFG6_DPLL_IVCRX, 0x0, port);
					hdmirx_wr_cor(H21RXSB_D2TH_M42H_IVCRX, 0x20, port);
					hdmirx_wr_bits_cor(H21RXSB_GP1_REGISTER_M42H_IVCRX,
						_BIT(3), 1, port);
					//clk ready threshold
					hdmirx_wr_cor(H21RXSB_DIFF1T_M42H_IVCRX, 0x20, port);
				} else {
					hdmirx_wr_cor(DPLL_CFG6_DPLL_IVCRX, 0x10, port);
					hdmirx_wr_cor(RX_H21_CTRL_PWD_IVCRX, 0x0, port);
				}
			}
			if (rx[port].state == FSM_SIG_READY) {
				//hdmirx_top_irq_en(0, 0, port);
				rx_irq_en(0, port);
			}
			rx[port].state = FLT_RX_LTS_3;
		} else {
			if (rx[port].state == FSM_SIG_READY) {
				//hdmirx_top_irq_en(0, 0, port);
				rx_irq_en(0, port);
			}
			if (rx[port].state > FSM_FRL_FLT_READY)
				rx[port].state = FSM_FRL_FLT_READY;
		}
		if (log_level & FRL_LOG)
			rx_pr("port-%d frate change to %d\n", port, rx[port].var.frl_rate);
	} else {
		if (rx[port].var.frl_rate) {
			if ((abs(rx[port].clk.t_clk_pre - rx[port].clk.tclk) > 10 * MHz)) {
				if (rx[port].state == FSM_SIG_READY) {
					rx[port].state = FSM_FRL_FLT_READY;
					//hdmirx_top_irq_en(0, 0, port);
					rx_irq_en(0, port);
				}
			}
		}
	}
	//rx_monitor_error_counter(port);
	if (rx[port].var.frl_rate)
		valid_m_monitor1(port);
}

void rx_hpd_monitor(void)
{
	static u8 hpd_wait_cnt0, hpd_wait_cnt1, hpd_wait_cnt2, hpd_wait_cnt3;

	if (!hdmi_cec_en || hdmi_cec_en == 0xff)
		return;

	if (rx_info.chip_id == CHIP_ID_T3X)
		return;

	if (rx_info.main_port_open)
		port_hpd_rst_flag &= ~(1 << rx_info.main_port);

	if ((port_hpd_rst_flag & 1) && rx[E_PORT0].cur_5v_sts) {
		if (hpd_wait_cnt0++ > hpd_wait_max) {
			rx_set_port_hpd(0, 1);
			hpd_wait_cnt0 = 0;
			port_hpd_rst_flag &= 0xFE;
		}
	} else {
		hpd_wait_cnt0 = 0;
	}
	if ((port_hpd_rst_flag & 2) && rx[E_PORT1].cur_5v_sts) {
		if (hpd_wait_cnt1++ > hpd_wait_max) {
			rx_set_port_hpd(1, 1);
			hpd_wait_cnt1 = 0;
			port_hpd_rst_flag &= 0xFD;
		}
	} else {
		hpd_wait_cnt1 = 0;
	}
	if ((port_hpd_rst_flag & 4) && rx[E_PORT2].cur_5v_sts) {
		if (hpd_wait_cnt2++ > hpd_wait_max) {
			rx_set_port_hpd(2, 1);
			hpd_wait_cnt2 = 0;
			port_hpd_rst_flag &= 0xFB;
		}
	} else {
		hpd_wait_cnt2 = 0;
	}
	if ((port_hpd_rst_flag & 8) && rx[E_PORT3].cur_5v_sts) {
		if (hpd_wait_cnt3++ > hpd_wait_max) {
			rx_set_port_hpd(3, 1);
			hpd_wait_cnt3 = 0;
			port_hpd_rst_flag &= 0xF7;
		}
	} else {
		hpd_wait_cnt3 = 0;
	}
}

void hdmirx_timer_t3x(void)
{
	int port;

	if (is_fsm_ready_t3x()) {
		if (!sm_pause) {
			for (port = E_PORT0; port < E_PORT_NUM; port++) {
				if (rx_info.main_port_open)
					rx_nosig_monitor(port);
				rx_clk_rate_monitor(port);
				//t3x do not need ddc monitor
				//rx_ddc_active_monitor(port);
				rx_hdcp_monitor(port);
				//rm cor reset for t3x txhd2 t5m
				rx_afifo_monitor(port);
				// no need hdcp22_decrypt
				//hdcp22_decrypt_monitor(port);
				rx_ext_state_monitor(port);
			}
			rx_port0_main_state_machine();
			rx_port1_main_state_machine();
			rx_port2_main_state_machine();
			rx_port3_main_state_machine();
		}
		#ifdef K_TEST_CHK_ERR_CNT
		if (err_chk_en) {
			for (port = E_PORT0; port < E_PORT_NUM; port++)
				rx_monitor_error_counter(port);
		}
		#endif
	}
}

void hdmirx_timer_t3x_pre(void)
{
	if (rx_info.main_port_open || rx[rx_info.main_port].resume_flag) {
		rx_nosig_monitor(rx_info.main_port);
		rx_cable_clk_monitor(rx_info.main_port);
#ifdef CONFIG_AMLOGIC_HDMITX
		rx_check_repeat(rx_info.main_port);
#endif
		if (!(rpt_only_mode && !rx[rx_info.main_port].hdcp.repeat)) {
			if (!sm_pause) {
				rx_clk_rate_monitor(rx_info.main_port);
				rx_ddc_active_monitor(rx_info.main_port);
				rx_hdcp_monitor(rx_info.main_port);
				rx_afifo_monitor(rx_info.main_port);
				hdcp22_decrypt_monitor(rx_info.main_port);
				rx_ext_state_monitor(rx_info.main_port);
				rx_main_state_machine();
				//rx_hpd_monitor();
			}
			/* rx_pkt_check_content(rx_info.main_port); */
			#ifdef K_TEST_CHK_ERR_CNT
			if (err_chk_en) {
				rx_monitor_error_counter(rx_info.main_port);
			}
			#endif
		}
	}
}

void hdmirx_timer_handler(struct timer_list *t)
{
	struct hdmirx_dev_s *devp = from_timer(devp, t, timer);
	rx_info.timestamp++;
	if (term_flag && term_cal_en) {
		rx_phy_rt_cal();
		term_flag = 0;
	}
	rx_5v_monitor();
	rx_clkmsr_monitor();
	rx_hpd_monitor();
	rx_edid_monitor();
	rx_i2c_dbg_monitor();
	if (rx_info.chip_id == CHIP_ID_T3X)
		hdmirx_timer_t3x();
	else
		hdmirx_timer_t3x_pre();
	devp->timer.expires = jiffies + TIMER_STATE_CHECK;
	add_timer(&devp->timer);
}

