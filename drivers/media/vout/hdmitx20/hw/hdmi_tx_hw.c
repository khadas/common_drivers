// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

//#define DEBUG
#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
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
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/compiler.h>
#include <linux/arm-smccc.h>
#include <linux/rtc.h>
#include <linux/timekeeping.h>
#include <linux/gpio.h>
#include <linux/amlogic/media/vout/vinfo.h>
#include <linux/amlogic/media/vout/hdmi_tx_ext.h>
#include <linux/reset.h>
#include <linux/compiler.h>
#include "mach_reg.h"
#include "hdmi_tx_reg.h"
#include "tvenc_conf.h"
#include "common.h"
#include "hdmi_tx_hw.h"
#include "hw_clk.h"
#include <linux/arm-smccc.h>
#ifdef CONFIG_AMLOGIC_VPU
#include <linux/amlogic/media/vpu/vpu.h>
#endif
#include "checksha.h"
#include "reg_sc2.h"
#include "hdmi_tx_debug_reg.h"
#include "../hdmi_tx_module.h"
#include <linux/amlogic/clk_measure.h>
#include "../../hdmitx_common/hdmitx_compliance.h"
#include "../meson_drm_hdmitx.h"

#define HDMITX_VIC_MASK			0xff

static struct hdmitx20_hw *global_tx_hw;

struct _hdmi_clkmsr {
	int idx;
	char *name;
};

static const struct _hdmi_clkmsr hdmiclkmsr_G12A[] = {
	{6, "cts_enci_clk"},
	{8, "cts_encp_clk"},
	{9, "cts_encl_clk"},
	{36, "cts_hdmi_tx_pixel_clk"},
	{46, "cts_vpu_clk"},
	{54, "cts_vpu_clkc"},
	{68, "cts_hdcp22_esmclk"},
	{69, "cts_hdcp22_skpclk"},
	{90, "cts_hdmitx_sys_clk"},
	{96, "cts_vpu_clkb"},
	{97, "cts_vpu_clkb_tmp"},
};

static const struct _hdmi_clkmsr hdmiclkmsr_SC2[] = {
	{51, "cts_enci_clk"},
	{52, "cts_encp_clk"},
	{53, "cts_encl_clk"},
	{59, "cts_hdmi_tx_pixel_clk"},
	{61, "cts_vpu_clk"},
	{62, "cts_vpu_clkb"},
	{63, "cts_vpu_clkb_tmp"},
	{64, "cts_vpu_clkc"},
	{68, "cts_hdcp22_esmclk"},
	{69, "cts_hdcp22_skpclk"},
	{76, "hdmitx_tmds_clk"},
	{77, "cts_hdmitx_sys_clk"},
	{78, "cts_hdmitx_fe_clk"},
	{184, "vpu_osc_ring0(SVT24)"},
	{185, "vpu_osc_ring1(LVT20)"},
	{186, "vpu_osc_ring2(LVT16)"},
};

static void mode420_half_horizontal_para(void);
static void hdmi_phy_suspend(void);
static void hdmi_phy_wakeup(struct hdmitx_dev *hdev);
static void hdmitx_set_phy(struct hdmitx_dev *hdev);
static void hdmitx_set_hw(struct hdmitx_dev *hdev);
static void hdmitx_csc_config(unsigned char input_color_format,
			      unsigned char output_color_format,
			      unsigned char color_depth);
static int hdmitx_hdmi_dvi_config(struct hdmitx20_hw *tx_hw,
				  unsigned int dvi_mode);
static void hdmitx_set_avi_colorimetry(struct hdmi_format_para *para);
static int hdmitx_tmds_rxsense(void);

struct ksv_lists_ {
	unsigned char valid;
	unsigned int no;
	unsigned char lists[MAX_KSV_LISTS * 5];
};

static struct ksv_lists_ tmp_ksv_lists;

static void hdmitx_set_packet(int type, unsigned char *DB, unsigned char *HB);
static void hdmitx_disable_packet(int type);
static void hdmitx_setaudioinfoframe(unsigned char *AUD_DB,
				     unsigned char *CHAN_STAT_BUF);
static int hdmitx_set_dispmode(struct hdmitx_hw_common *tx_hw);
static int hdmitx_set_audmode(struct hdmitx_hw_common *tx_hw,
			      struct aud_para *audio_param);
static int hdmitx_setupirq(struct hdmitx_hw_common *tx_hw);
static void hdmitx_debug(struct hdmitx_hw_common *tx_hw, const char *buf);
static void hdmitx_uninit(struct hdmitx_hw_common *tx_hw);
static int hdmitx_cntl(struct hdmitx_hw_common *hdmitx_hw_common,
		unsigned int cmd, unsigned int argv);
static int hdmitx_cntl_ddc(struct hdmitx_hw_common *tx_hw,
		unsigned int cmd, unsigned long argv);
static int hdmitx_get_state(struct hdmitx_hw_common *tx_hw, unsigned int cmd,
			    unsigned int argv);
static int hdmitx_cntl_config(struct hdmitx_hw_common *hdev, unsigned int cmd,
			      unsigned int argv);
static int hdmitx_cntl_misc(struct hdmitx_hw_common *tx_hw, unsigned int cmd,
			    unsigned int  argv);
static void audio_mute_op(bool flag);

static DEFINE_MUTEX(aud_mutex);

#define EDID_RAM_ADDR_SIZE	 (8)

/* HSYNC polarity: active high */
#define HSYNC_POLARITY	 1
/* VSYNC polarity: active high */
#define VSYNC_POLARITY	 1
/* Pixel format: 0=RGB444; 1=YCbCr444; 2=Rsrv; 3=YCbCr422. */
#define TX_INPUT_COLOR_FORMAT   HDMI_COLORSPACE_YUV444
/* Pixel range: 0=16-235/240; 1=16-240; 2=1-254; 3=0-255. */
#define TX_INPUT_COLOR_RANGE	0
/* Pixel bit width: 4=24-bit; 5=30-bit; 6=36-bit; 7=48-bit. */
#define TX_COLOR_DEPTH		 COLORDEPTH_24B

static pf_callback earc_hdmitx_hpdst;

void hdmitx_earc_hpdst(pf_callback cb)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();

	earc_hdmitx_hpdst = cb;
	if (!hdev || !hdev->hdmi_init)
		return;
	if (cb && hdmitx_hpd_hw_op(HPD_READ_HPD_GPIO))
		cb(true);
}

int hdmitx_hpd_hw_op(enum hpd_op cmd)
{
	switch (global_tx_hw->chip_data->chip_type) {
#ifndef CONFIG_AMLOGIC_REMOVE_OLD
	case MESON_CPU_ID_GXBB:
		return !!hdmitx_hpd_hw_op_gxbb(cmd);
	case MESON_CPU_ID_GXTVBB:
		return !!hdmitx_hpd_hw_op_gxtvbb(cmd);
	case MESON_CPU_ID_GXL:
	case MESON_CPU_ID_GXM:
		return !!hdmitx_hpd_hw_op_gxl(cmd);
	case MESON_CPU_ID_TXLX:
#endif
	case MESON_CPU_ID_TM2:
	case MESON_CPU_ID_TM2B:
		return !!hdmitx_hpd_hw_op_txlx(cmd);
	case MESON_CPU_ID_G12A:
	case MESON_CPU_ID_G12B:
	case MESON_CPU_ID_SM1:
	case MESON_CPU_ID_SC2:
	default:
		return !!hdmitx_hpd_hw_op_g12a(cmd);
	}
	return 0;
}
EXPORT_SYMBOL(hdmitx_hpd_hw_op);

/* TODO: removed, it's not correct and not used */
int read_hpd_gpio(void)
{
	switch (global_tx_hw->chip_data->chip_type) {
#ifndef CONFIG_AMLOGIC_REMOVE_OLD
	case MESON_CPU_ID_GXBB:
		return read_hpd_gpio_gxbb();
	case MESON_CPU_ID_GXTVBB:
		return read_hpd_gpio_gxtvbb();
	case MESON_CPU_ID_GXL:
	case MESON_CPU_ID_GXM:
		return read_hpd_gpio_gxl();
	case MESON_CPU_ID_TXLX:
#endif
	case MESON_CPU_ID_G12A:
	case MESON_CPU_ID_G12B:
	case MESON_CPU_ID_SM1:
	case MESON_CPU_ID_TM2:
	case MESON_CPU_ID_TM2B:
	case MESON_CPU_ID_SC2:
	default:
		return read_hpd_gpio_txlx();
	}
	return 0;
}
EXPORT_SYMBOL(read_hpd_gpio);

int hdmitx_ddc_hw_op(enum ddc_op cmd)
{
	switch (global_tx_hw->chip_data->chip_type) {
#ifndef CONFIG_AMLOGIC_REMOVE_OLD
	case MESON_CPU_ID_GXBB:
		return hdmitx_ddc_hw_op_gxbb(cmd);
	case MESON_CPU_ID_GXTVBB:
		return hdmitx_ddc_hw_op_gxtvbb(cmd);
	case MESON_CPU_ID_GXL:
	case MESON_CPU_ID_GXM:
		return hdmitx_ddc_hw_op_gxl(cmd);
	case MESON_CPU_ID_TXLX:
		return hdmitx_ddc_hw_op_txlx(cmd);
#endif
	case MESON_CPU_ID_G12A:
	case MESON_CPU_ID_G12B:
	case MESON_CPU_ID_SM1:
	case MESON_CPU_ID_TM2:
	case MESON_CPU_ID_TM2B:
	case MESON_CPU_ID_SC2:
	default:
		return hdmitx_ddc_hw_op_g12a(cmd);
	}
	return 0;
}
EXPORT_SYMBOL(hdmitx_ddc_hw_op);

static int hdcp_topo_st = -1;
static int hdcp22_susflag;
int hdmitx_hdcp_opr(unsigned int val)
{
	struct arm_smccc_res res;
	struct hdmitx_dev *hdev = get_hdmitx_device();

	if (val == 1) { /* HDCP14_ENABLE */
		arm_smccc_smc(0x82000010, 0, 0, 0, 0, 0, 0, 0, &res);
	}
	if (val == 2) { /* HDCP14_RESULT */
		arm_smccc_smc(0x82000011, 0, 0, 0, 0, 0, 0, 0, &res);
		return (unsigned int)((res.a0) & 0xffffffff);
	}
	if (val == 0) { /* HDCP14_INIT */
		arm_smccc_smc(0x82000012, 0, 0, 0, 0, 0, 0, 0, &res);
	}
	if (val == 3) { /* HDCP14_EN_ENCRYPT */
		arm_smccc_smc(0x82000013, 0, 0, 0, 0, 0, 0, 0, &res);
	}
	if (val == 4) { /* HDCP14_OFF */
		arm_smccc_smc(0x82000014, 0, 0, 0, 0, 0, 0, 0, &res);
	}
	if (val == 5) {	/* HDCP_MUX_22 */
		arm_smccc_smc(0x82000015, 0, 0, 0, 0, 0, 0, 0, &res);
	}
	if (val == 6) {	/* HDCP_MUX_14 */
		arm_smccc_smc(0x82000016, 0, 0, 0, 0, 0, 0, 0, &res);
	}
	if (val == 7) { /* HDCP22_RESULT */
		arm_smccc_smc(0x82000017, 0, 0, 0, 0, 0, 0, 0, &res);
		return (unsigned int)((res.a0) & 0xffffffff);
	}
	if (val == 0xa) { /* HDCP14_KEY_LSTORE */
		arm_smccc_smc(0x8200001a, 0, 0, 0, 0, 0, 0, 0, &res);
		return (unsigned int)((res.a0) & 0xffffffff);
	}
	if (val == 0xb) { /* HDCP22_KEY_LSTORE */
		/* efuse ctrl hdcptx22 */
		if (hdev->tx_comm.efuse_dis_hdcp_tx22)
			return 0;
		arm_smccc_smc(0x8200001b, 0, 0, 0, 0, 0, 0, 0, &res);
		return (unsigned int)((res.a0) & 0xffffffff);
	}
	if (val == 0xc) { /* HDCP22_KEY_SET_DUK */
		arm_smccc_smc(0x8200001c, 0, 0, 0, 0, 0, 0, 0, &res);
		return (unsigned int)((res.a0) & 0xffffffff);
	}
	if (val == 0xd) { /* HDCP22_SET_TOPO */
		arm_smccc_smc(0x82000083, hdcp_topo_st, 0, 0, 0, 0, 0, 0, &res);
	}
	if (val == 0xe) { /* HDCP22_GET_TOPO */
		arm_smccc_smc(0x82000084, 0, 0, 0, 0, 0, 0, 0, &res);
		return (unsigned int)((res.a0) & 0xffffffff);
	}
	if (val == 0xf) { /* HDCP22_SET_SUSFLAG */
		arm_smccc_smc(0x8200008a, hdcp22_susflag,
			      0, 0, 0, 0, 0, 0, &res);
	}
	return -1;
}

static void config_avmute(unsigned int val)
{
	pr_debug(HW "avmute set to %d\n", val);
	switch (val) {
	case SET_AVMUTE:
		hdmitx_set_reg_bits(HDMITX_DWC_FC_GCP, 1, 1, 1);
		hdmitx_set_reg_bits(HDMITX_DWC_FC_GCP, 0, 0, 1);
		break;
	case CLR_AVMUTE:
		hdmitx_set_reg_bits(HDMITX_DWC_FC_GCP, 0, 1, 1);
		hdmitx_set_reg_bits(HDMITX_DWC_FC_GCP, 1, 0, 1);
		break;
	case OFF_AVMUTE:
	default:
		hdmitx_set_reg_bits(HDMITX_DWC_FC_GCP, 0, 1, 1);
		hdmitx_set_reg_bits(HDMITX_DWC_FC_GCP, 0, 0, 1);
		break;
	}
}

static int read_avmute(void)
{
	int val;
	int ret = 0;

	val = hdmitx_rd_reg(HDMITX_DWC_FC_GCP) & 0x3;

	switch (val) {
	case 2:
		ret = 1;
		break;
	case 1:
		ret = -1;
		break;
	case 0:
		ret = 0;
		break;
	default:
		ret = 3;
		break;
	}

	return ret;
}

/* record HDMITX current format, matched with uboot */
/* ISA_DEBUG_REG0 0x2600
 * bit[11]: Y420
 * bit[10:8]: HDMI VIC
 * bit[7:0]: CEA VIC
 */
static unsigned int hdmitx_get_isaformat(void)
{
	unsigned int ret = 0;

	switch (global_tx_hw->chip_data->chip_type) {
	case MESON_CPU_ID_SC2:
		ret = hd_read_reg(P_SYSCTRL_DEBUG_REG0);
		break;
	case MESON_CPU_ID_TXLX:
	case MESON_CPU_ID_G12A:
	case MESON_CPU_ID_G12B:
	case MESON_CPU_ID_SM1:
	case MESON_CPU_ID_TM2:
	case MESON_CPU_ID_TM2B:
		ret = hdmitx_get_format_txlx();
		break;
	case MESON_CPU_ID_GXBB:
	case MESON_CPU_ID_GXTVBB:
	case MESON_CPU_ID_GXL:
	case MESON_CPU_ID_GXM:
	default:
		ret = hd_read_reg(P_ISA_DEBUG_REG0);
		break;
	}

	return ret;
}

static void hdmitx_set_isaformat(unsigned int val)
{
	switch (global_tx_hw->chip_data->chip_type) {
	case MESON_CPU_ID_SC2:
		hd_write_reg(P_SYSCTRL_DEBUG_REG0, val);
		break;
	case MESON_CPU_ID_TXLX:
	case MESON_CPU_ID_G12A:
	case MESON_CPU_ID_G12B:
	case MESON_CPU_ID_SM1:
	case MESON_CPU_ID_TM2:
	case MESON_CPU_ID_TM2B:
		hdmitx_set_format_txlx(val);
		break;
	case MESON_CPU_ID_GXBB:
	case MESON_CPU_ID_GXTVBB:
	case MESON_CPU_ID_GXL:
	case MESON_CPU_ID_GXM:
	default:
		hd_write_reg(P_ISA_DEBUG_REG0, val);
		break;
	}
}

static int hdmitx_uboot_sc2_already_display(void)
{
	int ret = 0;

	if ((hd_read_reg(P_CLKCTRL_HDMI_CLK_CTRL) & (1 << 8)) &&
	    (hd_read_reg(P_ANACTRL_HDMIPLL_CTRL0) & (1 << 31)) &&
	    (hdmitx_get_isaformat()))
		ret = 1;
	else
		ret = 0;

	return ret;
}

/* hdmitx_get_isaformat() info may be cleared, use phy state instead
 * note that PHY_CTRL0 may be enabled for bandgap in plugin top half,
 * so should not use bangap setting bits for uboot state decision
 */
static int hdmitx_uboot_already_display(void)
{
	int ret = 0;
	int type = global_tx_hw->chip_data->chip_type;

	if (type >= MESON_CPU_ID_SC2)
		return hdmitx_uboot_sc2_already_display();

	if ((hd_read_reg(P_HHI_HDMI_CLK_CNTL) & (1 << 8)) &&
	    (hd_read_reg(P_HHI_HDMI_PLL_CNTL) & (1 << 31)) &&
	    (hdmitx_get_isaformat())) {
		HDMITX_INFO("alread display in uboot 0x%x\n",
			hdmitx_get_isaformat());
		ret = 1;
	} else {
		HDMITX_INFO("hdmitx_get_isaformat:0x%x\n",
			hdmitx_get_isaformat());
		HDMITX_INFO("P_HHI_HDMI_CLK_CNTL :0x%x\n",
			hd_read_reg(P_HHI_HDMI_CLK_CNTL));
		HDMITX_INFO("P_HHI_HDMI_PLL_CNTL :0x%x\n",
			hd_read_reg(P_HHI_HDMI_PLL_CNTL));
		ret = 0;
	}

	return ret;
}

/* reset HDMITX APB & TX */
void hdmitx_sys_reset(void)
{
	switch (global_tx_hw->chip_data->chip_type) {
	case MESON_CPU_ID_TXLX:
	case MESON_CPU_ID_G12A:
	case MESON_CPU_ID_G12B:
	case MESON_CPU_ID_SM1:
	case MESON_CPU_ID_TM2:
	case MESON_CPU_ID_TM2B:
		hdmitx_sys_reset_txlx();
		break;
	case MESON_CPU_ID_SC2:
		hdmitx_sys_reset_sc2();
		break;
	case MESON_CPU_ID_GXBB:
	case MESON_CPU_ID_GXTVBB:
	case MESON_CPU_ID_GXL:
	case MESON_CPU_ID_GXM:
	default:
		hd_set_reg_bits(P_RESET0_REGISTER, 1, 19, 1);
		hd_set_reg_bits(P_RESET2_REGISTER, 1, 15, 1);
		hd_set_reg_bits(P_RESET2_REGISTER, 1,  2, 1);
		break;
	}
}

/* for 30bits colordepth */
static void set_vmode_clk(struct hdmitx_dev *hdev)
{
	hdmitx_set_clk(hdev);
}

static void hdmi_hwp_init(struct hdmitx_dev *hdev)
{
	struct hdmitx20_hw *tx_hw = &hdev->tx_hw;
	int chip_id = tx_hw->chip_data->chip_type;

	hdmitx_set_sys_clk(tx_hw, 0xff);
	hdmitx_set_cts_hdcp22_clk(hdev);
	hdmitx_set_hdcp_pclk(hdev);
	hdmitx_set_hdmi_axi_clk(hdev);

	/* wire	wr_enable = control[3]; */
	/* wire	fifo_enable = control[2]; */
	/* assign phy_clk_en = control[1]; */
	/* Bring HDMITX MEM output of power down */
	if (chip_id >= MESON_CPU_ID_SC2)
		hd_set_reg_bits(P_PWRCTRL_MEM_PD11, 0, 8, 8);
	else
		hd_set_reg_bits(P_HHI_MEM_PD_REG0, 0, 8, 8);
#ifdef CONFIG_AMLOGIC_VPU
	/* VPU_HDMI since tm2_B: P_HHI_MEM_PD_REG4 bit[13:12] */
	if (hdev->hdmi_vpu_dev)
		vpu_dev_mem_power_on(hdev->hdmi_vpu_dev);
#endif
	/* enable CLK_TO_DIG */
	if (chip_id >= MESON_CPU_ID_SC2)
		hd_set_reg_bits(P_ANACTRL_HDMIPHY_CTRL3, 0x3, 0, 2);
	else if (chip_id == MESON_CPU_ID_TM2 ||
		 chip_id == MESON_CPU_ID_TM2B)
		hd_set_reg_bits(P_TM2_HHI_HDMI_PHY_CNTL2, 0x3, 0, 2);
	else
		hd_set_reg_bits(P_HHI_HDMI_PHY_CNTL3, 0x3, 0, 2);
	if (hdmitx_uboot_already_display() == 0) {
		/* reset HDMITX APB & TX & PHY */
		hdmitx_sys_reset();
		if (chip_id < MESON_CPU_ID_G12A) {
			/* Enable APB3 fail on error */
			hd_set_reg_bits(P_HDMITX_CTRL_PORT, 1, 15, 1);
			hd_set_reg_bits((P_HDMITX_CTRL_PORT + 0x10), 1, 15, 1);
		}
		/* Bring out of reset */
		hdmitx_wr_reg(HDMITX_TOP_SW_RESET,	0);
		usleep_range(199, 200);
		hdmitx_set_reg_bits(HDMITX_TOP_CLK_CNTL, 3, 0, 2);
		hdmitx_set_reg_bits(HDMITX_TOP_CLK_CNTL, 3, 4, 2);
		hdmitx_wr_reg(HDMITX_DWC_MC_LOCKONCLOCK, 0xff);
		hdmitx_wr_reg(HDMITX_TOP_INTR_MASKN, 0x1f);
	}
	/* enable audio default */
	hdmitx_set_reg_bits(HDMITX_TOP_CLK_CNTL, 3, 2, 2);
	hdmitx_set_reg_bits(HDMITX_DWC_FC_PACKET_TX_EN, 1, 0, 1);
	hdmitx_set_reg_bits(HDMITX_DWC_FC_PACKET_TX_EN, 1, 3, 1);
}

static void hdmi_hwi_init(struct hdmitx_dev *hdev)
{
	unsigned int data32 = 0;

	hdmitx_set_reg_bits(HDMITX_DWC_FC_INVIDCONF, 1, 7, 1);
	hdmitx_wr_reg(HDMITX_DWC_A_HDCPCFG1, 0x67);
	if (hdmitx_find_vendor_null_pkt(hdev->tx_comm.EDID_buf)) {
		hdmitx_wr_reg(HDMITX_TOP_DISABLE_NULL, 0x6);
		HDMITX_INFO("special TV, need enable NULL packet\n");
	} else {
		hdmitx_wr_reg(HDMITX_TOP_DISABLE_NULL, 0x7); /* disable NULL pkt */
	}
	hdmitx_wr_reg(HDMITX_DWC_A_HDCPCFG0, 0x13);
	hdmitx_set_reg_bits(HDMITX_DWC_HDCP22REG_CTRL, 1, 1, 1);
	/* Enable skpclk to HDCP2.2 IP */
	hdmitx_set_reg_bits(HDMITX_TOP_CLK_CNTL, 1, 7, 1);
	/* Enable esmclk to HDCP2.2 IP */
	hdmitx_set_reg_bits(HDMITX_TOP_CLK_CNTL, 1, 6, 1);
	/* Enable tmds_clk to HDCP2.2 IP */
	hdmitx_set_reg_bits(HDMITX_TOP_CLK_CNTL, 1, 5, 1);
	/* Enable axi_clk */
	hdmitx_set_reg_bits(HDMITX_TOP_CLK_CNTL, 1, 13, 1);
	/* Enable axi_async_req_en_emp & axi_async_req_en_esm */
	hdmitx_set_reg_bits(HDMITX_TOP_AXI_ASYNC_CNTL0, 1, 0, 1);
	hdmitx_set_reg_bits(HDMITX_TOP_AXI_ASYNC_CNTL0, 1, 16, 1);

	hdmitx_hpd_hw_op(HPD_INIT_DISABLE_PULLUP);
	hdmitx_hpd_hw_op(HPD_INIT_SET_FILTER);
	hdmitx_ddc_hw_op(DDC_INIT_DISABLE_PULL_UP_DN);
	hdmitx_ddc_hw_op(DDC_MUX_DDC);

/* Configure E-DDC interface */
	data32  = 0;
	data32 |= (1    << 24); /* [26:24] infilter_ddc_intern_clk_divide */
	data32 |= (0    << 16); /* [23:16] infilter_ddc_sample_clk_divide */
	data32 |= (0    << 8);  /* [10: 8] infilter_cec_intern_clk_divide */
	data32 |= (1    << 0);  /* [ 7: 0] infilter_cec_sample_clk_divide */
	hdmitx_wr_reg(HDMITX_TOP_INFILTER, data32);

	data32 = 0;
	data32 |= (0 << 6);  /* [  6] read_req_mask */
	data32 |= (0 << 2);  /* [  2] done_mask */
	hdmitx_wr_reg(HDMITX_DWC_I2CM_INT, data32);

	data32 = 0;
	data32 |= (0 << 6);  /* [  6] nack_mask */
	data32 |= (0 << 2);  /* [  2] arbitration_error_mask */
	hdmitx_wr_reg(HDMITX_DWC_I2CM_CTLINT, data32);

/* [  3] i2c_fast_mode: 0=standard mode; 1=fast mode. */
	data32 = 0;
	data32 |= (0 << 3);
	hdmitx_wr_reg(HDMITX_DWC_I2CM_DIV, data32);

	hdmitx_wr_reg(HDMITX_DWC_I2CM_SS_SCL_HCNT_1, 0);
	hdmitx_wr_reg(HDMITX_DWC_I2CM_SS_SCL_HCNT_0, 0xcf);
	hdmitx_wr_reg(HDMITX_DWC_I2CM_SS_SCL_LCNT_1, 0);
	hdmitx_wr_reg(HDMITX_DWC_I2CM_SS_SCL_LCNT_0, 0xff);
	if (global_tx_hw->base.hdcp_repeater_en == 1) {
		hdmitx_wr_reg(HDMITX_DWC_I2CM_SS_SCL_HCNT_0, 0x67);
		hdmitx_wr_reg(HDMITX_DWC_I2CM_SS_SCL_LCNT_0, 0x78);
	}
	hdmitx_wr_reg(HDMITX_DWC_I2CM_FS_SCL_HCNT_1, 0);
	hdmitx_wr_reg(HDMITX_DWC_I2CM_FS_SCL_HCNT_0, 0x0f);
	hdmitx_wr_reg(HDMITX_DWC_I2CM_FS_SCL_LCNT_1, 0);
	hdmitx_wr_reg(HDMITX_DWC_I2CM_FS_SCL_LCNT_0, 0x20);
	hdmitx_wr_reg(HDMITX_DWC_I2CM_SDA_HOLD,	0x08);

	data32 = 0;
	data32 |= (0 << 5);  /* [  5] updt_rd_vsyncpoll_en */
	data32 |= (0 << 4);  /* [  4] read_request_en  // scdc */
	data32 |= (0 << 0);  /* [  0] read_update */
	hdmitx_wr_reg(HDMITX_DWC_I2CM_SCDC_UPDATE,  data32);
}

bool hdmitx_uboot_audio_en(void)
{
	unsigned int data;

	data = hdmitx_rd_reg(HDMITX_DWC_FC_PACKET_TX_EN);
	pr_debug("%s[%d] data = 0x%x\n", __func__, __LINE__, data);
	if ((data & 1) || ((data >> 3) & 1))
		return true;
	else
		return false;
}

static int hdmitx_validate_mode(struct hdmitx_hw_common *tx_hw, u32 vic)
{
	const struct hdmi_timing *timing;

	if (!tx_hw || !global_tx_hw->chip_data) {
		return -EINVAL;
	}

	if (vic == HDMI_0_UNKNOWN) {
		return -EINVAL;
	}

	timing = hdmitx_mode_vic_to_hdmi_timing(vic);
	if (!timing) {
		return -EINVAL;
	}

	return 0;
}

static int hdmitx_calc_formatpara(struct hdmitx_hw_common *tx_hw,
	struct hdmi_format_para *para)
{
	if (!tx_hw || !para)
		return -EINVAL;

	/*update tmds clock.*/
	para->tmds_clk = hdmitx_calc_tmds_clk(para->timing.pixel_freq,
		para->cs, para->cd);

	if (para->tmds_clk > 340000) {
		para->scrambler_en = 1;
		para->tmds_clk_div40 = 1;
	} else {
		para->scrambler_en = 0;
		para->tmds_clk_div40 = 0;
	}

	return 0;
}

/* note: if need to check if global_tx_hw not NULL before use it
 * in case unexpected call into hw api before we init it.
 */
void hdmitx_meson_init(struct hdmitx_dev *hdev)
{
	global_tx_hw = &hdev->tx_hw;

	hdev->hwop.setaudioinfoframe = hdmitx_setaudioinfoframe;
	global_tx_hw->base.getstate = hdmitx_get_state;
	global_tx_hw->base.cntlconfig = hdmitx_cntl_config;
	global_tx_hw->base.cntlmisc = hdmitx_cntl_misc;
	global_tx_hw->base.validatemode = hdmitx_validate_mode;
	global_tx_hw->base.calc_format_para = hdmitx_calc_formatpara;
	global_tx_hw->base.setpacket = hdmitx_set_packet;
	global_tx_hw->base.disablepacket = hdmitx_disable_packet;
	global_tx_hw->base.cntlddc = hdmitx_cntl_ddc;
	global_tx_hw->base.cntl = hdmitx_cntl;
	global_tx_hw->base.setaudmode = hdmitx_set_audmode;
	global_tx_hw->base.uninit = hdmitx_uninit;
	global_tx_hw->base.setupirq = hdmitx_setupirq;
	global_tx_hw->base.debugfun = hdmitx_debug;
	global_tx_hw->base.setdispmode = hdmitx_set_dispmode;
	hdmi_hwp_init(hdev);
	hdmi_hwi_init(hdev);
	hdmitx_hw_cntl_misc(&global_tx_hw->base, MISC_AVMUTE_OP, CLR_AVMUTE);
	hdmitx_debugfs_init();
}

static void hdmitx_phy_bandgap_en(struct hdmitx_dev *hdev)
{
	switch (hdev->tx_hw.chip_data->chip_type) {
	case MESON_CPU_ID_TM2:
	case MESON_CPU_ID_TM2B:
		hdmitx_phy_bandgap_en_tm2();
		break;
	case MESON_CPU_ID_SM1:
		hdmitx_phy_bandgap_en_g12();
		break;
	case MESON_CPU_ID_SC2:
		hdmitx_phy_bandgap_en_sc2();
		break;
	default:
		break;
	}
}

static void hdmitx_hpd_irq_top_half_process(struct hdmitx_dev *hdev, bool hpd)
{
	if (hpd) {
		hdmitx_phy_bandgap_en(hdev);
		if (earc_hdmitx_hpdst)
			earc_hdmitx_hpdst(true);
	} else {
		if (earc_hdmitx_hpdst)
			earc_hdmitx_hpdst(false);
	}
}

static irqreturn_t intr_handler(int irq, void *dev)
{
	/* get interrupt status */
	unsigned int dat_top = hdmitx_rd_reg(HDMITX_TOP_INTR_STAT);
	unsigned int dat_dwc = hdmitx_rd_reg(HDMITX_DWC_HDCP22REG_STAT);
	struct hdmitx_dev *hdev = (struct hdmitx_dev *)dev;
	bool ret;

	/* ack INTERNAL_INTR or else we stuck with no interrupts at all */
	hdmitx_wr_reg(HDMITX_TOP_INTR_STAT_CLR, ~0);
	hdmitx_wr_reg(HDMITX_DWC_HDCP22REG_STAT, 0xff);

	HDMITX_INFO(SYS "irq %x %x\n", dat_top, dat_dwc);
	/* bit[2:1] of dat_top means HPD falling and rising */
	if ((dat_top & 0x6) && hdev->tx_hw.base.hdmitx_gpios_hpd != -EPROBE_DEFER) {
		struct timespec64 kts;
		struct rtc_time tm;

		ktime_get_real_ts64(&kts);
		rtc_time64_to_tm(kts.tv_sec, &tm);
		HDMITX_INFO("UTC+0 %ptRd %ptRt HPD %s\n", &tm, &tm,
			gpio_get_value(hdev->tx_hw.base.hdmitx_gpios_hpd) ? "HIGH" : "LOW");
	}

	if (hdev->tx_hw.base.debug_hpd_lock == 1) {
		HDMITX_INFO("HDMI hpd locked\n");
		goto next;
	}
	/* check HPD status */
	if ((dat_top & (1 << 1)) && (dat_top & (1 << 2))) {
		if (hdmitx_hpd_hw_op(HPD_READ_HPD_GPIO))
			dat_top &= ~(1 << 2);
		else
			dat_top &= ~(1 << 1);
	}
	/* HPD rising */
	if (dat_top & (1 << 1)) {
		hdmitx_hpd_irq_top_half_process(hdev, true);
		ret = queue_delayed_work(hdev->hdmi_hpd_wq,
				   &hdev->work_hpd_plugin, HZ / 2);
		if (!ret)
			HDMITX_DEBUG_HPD("HDMI plugin work is already in the queue\n");
	}
	/* HPD falling */
	if (dat_top & (1 << 2)) {
		hdmitx_hpd_irq_top_half_process(hdev, false);
		/* Cancel previous hpd work.
		 * Note that plugout work is not canceled so as to
		 * prevent plugout work is not scheduled asap in
		 * critical high cpu loading case. always do
		 * plugout work to disable output asap.
		 */
		ret = cancel_delayed_work(&hdev->work_hpd_plugin);
		if (ret)
			HDMITX_DEBUG_HPD("plugin work is pending and canceled\n");
		else
			HDMITX_DEBUG_HPD("plugin work is not pending\n");

		ret = queue_delayed_work(hdev->hdmi_hpd_wq,
			&hdev->work_hpd_plugout, 0);
		if (!ret)
			HDMITX_DEBUG_HPD("HDMI plugout work is already in the queue\n");
	}
	/* internal interrupt */
	if (dat_top & (1 << 0))
		schedule_delayed_work(&hdev->work_internal_intr, HZ / 10);

	if (dat_top & (1 << 3)) {
		unsigned int rd_nonce_mode =
			hdmitx_rd_reg(HDMITX_TOP_SKP_CNTL_STAT) & 0x1;
		HDMITX_DEBUG_HDCP("hdcp22: Nonce %s  Vld: %d\n",
				rd_nonce_mode ? "HW" : "SW",
				((hdmitx_rd_reg(HDMITX_TOP_SKP_CNTL_STAT) >> 31) & 1));
		if (!rd_nonce_mode) {
			hdmitx_wr_reg(HDMITX_TOP_NONCE_0,  0x32107654);
			hdmitx_wr_reg(HDMITX_TOP_NONCE_1,  0xba98fedc);
			hdmitx_wr_reg(HDMITX_TOP_NONCE_2,  0xcdef89ab);
			hdmitx_wr_reg(HDMITX_TOP_NONCE_3,  0x45670123);
			hdmitx_wr_reg(HDMITX_TOP_NONCE_0,  0x76543210);
			hdmitx_wr_reg(HDMITX_TOP_NONCE_1,  0xfedcba98);
			hdmitx_wr_reg(HDMITX_TOP_NONCE_2,  0x89abcdef);
			hdmitx_wr_reg(HDMITX_TOP_NONCE_3,  0x01234567);
		}
	}
	if (dat_top & (1 << 30))
		HDMITX_DEBUG_HDCP("hdcp22: reg stat: 0x%x\n", dat_dwc);

next:
	return IRQ_HANDLED;
}

static irqreturn_t vsync_intr_handler(int irq, void *dev)
{
	struct hdmitx_dev *hdev = (struct hdmitx_dev *)dev;

	if (hdev->vid_mute_op != VIDEO_NONE_OP) {
		hdmitx_hw_cntl_config(&hdev->tx_hw.base,
			CONF_VIDEO_MUTE_OP, hdev->vid_mute_op);
		hdev->vid_mute_op = VIDEO_NONE_OP;
	}

	if (hdev->tx_comm.tx_hw->tmds_phy_op == TMDS_PHY_DISABLE) {
		hdmitx_hw_cntl_misc(&hdev->tx_hw.base,
			MISC_TMDS_PHY_OP, hdev->tx_comm.tx_hw->tmds_phy_op);
		hdev->tx_comm.tx_hw->tmds_phy_op = TMDS_PHY_NONE;
	}

	return IRQ_HANDLED;
}

static unsigned long modulo(unsigned long a, unsigned long b)
{
	if (a >= b)
		return a - b;
	else
		return a;
}

static signed int to_signed(unsigned int a)
{
	if (a <= 7)
		return a;
	else
		return a - 16;
}

/*
 * mode: 1 means Progressive;  0 means interlaced
 */
static void enc_vpu_bridge_reset(int mode)
{
	unsigned int wr_clk = 0;

	wr_clk = (hd_read_reg(P_VPU_HDMI_SETTING) & 0xf00) >> 8;
	if (mode) {
		hd_write_reg(P_ENCP_VIDEO_EN, 0);
		hd_set_reg_bits(P_VPU_HDMI_SETTING, 0, 0, 2);
		hd_set_reg_bits(P_VPU_HDMI_SETTING, 0, 8, 4);
		usleep_range(1000, 1005);
		hd_set_reg_bits(P_VPU_HDMI_SETTING, wr_clk, 8, 4);
		usleep_range(1000, 1005);
		hd_set_reg_bits(P_VPU_HDMI_SETTING, 2, 0, 2);
		usleep_range(1000, 1005);
		hd_write_reg(P_ENCP_VIDEO_EN, 1);
	} else {
		hd_write_reg(P_ENCI_VIDEO_EN, 0);
		hd_set_reg_bits(P_VPU_HDMI_SETTING, 0, 0, 2);
		hd_set_reg_bits(P_VPU_HDMI_SETTING, 0, 8, 4);
		usleep_range(1000, 1005);
		hd_set_reg_bits(P_VPU_HDMI_SETTING, wr_clk, 8, 4);
		usleep_range(1000, 1005);
		hd_set_reg_bits(P_VPU_HDMI_SETTING, 1, 0, 2);
		usleep_range(1000, 1005);
		hd_write_reg(P_ENCI_VIDEO_EN, 1);
	}
}

static void hdmi_tvenc1080i_set(struct hdmi_format_para *param)
{
	unsigned long VFIFO2VD_TO_HDMI_LATENCY = 2;
	unsigned long TOTAL_PIXELS = 0, PIXEL_REPEAT_HDMI = 0,
		PIXEL_REPEAT_VENC = 0, ACTIVE_PIXELS = 0;
	unsigned int FRONT_PORCH = 88, HSYNC_PIXELS = 0, ACTIVE_LINES = 0,
		INTERLACE_MODE = 0, TOTAL_LINES = 0, SOF_LINES = 0,
		VSYNC_LINES = 0;
	unsigned int LINES_F0 = 0, LINES_F1 = 563, BACK_PORCH = 0,
		EOF_LINES = 2, TOTAL_FRAMES = 0;

	unsigned long total_pixels_venc = 0;
	unsigned long active_pixels_venc = 0;
	unsigned long front_porch_venc = 0;
	unsigned long hsync_pixels_venc = 0;

	unsigned long de_h_begin = 0, de_h_end = 0;
	unsigned long de_v_begin_even = 0, de_v_end_even = 0,
		de_v_begin_odd = 0, de_v_end_odd = 0;
	unsigned long hs_begin = 0, hs_end = 0;
	unsigned long vs_adjust = 0;
	unsigned long vs_bline_evn = 0, vs_eline_evn = 0,
		vs_bline_odd = 0, vs_eline_odd = 0;
	unsigned long vso_begin_evn = 0, vso_begin_odd = 0;

	if (param->vic == HDMI_5_1920x1080i60_16x9) {
		INTERLACE_MODE = 1;
		PIXEL_REPEAT_VENC = 0;
		PIXEL_REPEAT_HDMI = 0;
		ACTIVE_PIXELS = (1920 * (1 + PIXEL_REPEAT_HDMI));
		ACTIVE_LINES = (1080 / (1 + INTERLACE_MODE));
		LINES_F0 = 562;
		LINES_F1 = 563;
		FRONT_PORCH = 88;
		HSYNC_PIXELS = 44;
		BACK_PORCH = 148;
		EOF_LINES = 2;
		VSYNC_LINES = 5;
		SOF_LINES = 15;
		TOTAL_FRAMES = 4;
	} else if (param->vic == HDMI_20_1920x1080i50_16x9) {
		INTERLACE_MODE = 1;
		PIXEL_REPEAT_VENC = 0;
		PIXEL_REPEAT_HDMI = 0;
		ACTIVE_PIXELS = (1920 * (1 + PIXEL_REPEAT_HDMI));
		ACTIVE_LINES = (1080 / (1 + INTERLACE_MODE));
		LINES_F0 = 562;
		LINES_F1 = 563;
		FRONT_PORCH = 528;
		HSYNC_PIXELS = 44;
		BACK_PORCH = 148;
		EOF_LINES = 2;
		VSYNC_LINES = 5;
		SOF_LINES = 15;
		TOTAL_FRAMES = 4;
	}
	TOTAL_PIXELS = FRONT_PORCH + HSYNC_PIXELS + BACK_PORCH + ACTIVE_PIXELS;
	TOTAL_LINES = LINES_F0 + LINES_F1 * INTERLACE_MODE;

	total_pixels_venc = (TOTAL_PIXELS / (1 + PIXEL_REPEAT_HDMI)) *
		(1 + PIXEL_REPEAT_VENC);
	active_pixels_venc = (ACTIVE_PIXELS / (1 + PIXEL_REPEAT_HDMI)) *
		(1 + PIXEL_REPEAT_VENC);
	front_porch_venc = (FRONT_PORCH / (1 + PIXEL_REPEAT_HDMI)) *
		(1 + PIXEL_REPEAT_VENC);
	hsync_pixels_venc =
	(HSYNC_PIXELS / (1 + PIXEL_REPEAT_HDMI)) * (1 + PIXEL_REPEAT_VENC);

	hd_write_reg(P_ENCP_VIDEO_MODE, hd_read_reg(P_ENCP_VIDEO_MODE)
		| (1 << 14));

	/* Program DE timing */
	de_h_begin = modulo(hd_read_reg(P_ENCP_VIDEO_HAVON_BEGIN) +
		VFIFO2VD_TO_HDMI_LATENCY, total_pixels_venc);
	de_h_end  = modulo(de_h_begin + active_pixels_venc, total_pixels_venc);
	hd_write_reg(P_ENCP_DE_H_BEGIN, de_h_begin);
	hd_write_reg(P_ENCP_DE_H_END, de_h_end);
	/* Program DE timing for even field */
	de_v_begin_even = hd_read_reg(P_ENCP_VIDEO_VAVON_BLINE);
	de_v_end_even  = de_v_begin_even + ACTIVE_LINES;
	hd_write_reg(P_ENCP_DE_V_BEGIN_EVEN, de_v_begin_even);
	hd_write_reg(P_ENCP_DE_V_END_EVEN,  de_v_end_even);
	/* Program DE timing for odd field if needed */
	if (INTERLACE_MODE) {
		de_v_begin_odd =
		to_signed((hd_read_reg(P_ENCP_VIDEO_OFLD_VOAV_OFST) & 0xf0)
			  >> 4) + de_v_begin_even + (TOTAL_LINES - 1) / 2;
		de_v_end_odd = de_v_begin_odd + ACTIVE_LINES;
		hd_write_reg(P_ENCP_DE_V_BEGIN_ODD, de_v_begin_odd);/* 583 */
		hd_write_reg(P_ENCP_DE_V_END_ODD, de_v_end_odd);  /* 1123 */
	}

	/* Program Hsync timing */
	if (de_h_end + front_porch_venc >= total_pixels_venc) {
		hs_begin = de_h_end + front_porch_venc - total_pixels_venc;
		vs_adjust  = 1;
	} else {
		hs_begin = de_h_end + front_porch_venc;
		vs_adjust  = 0;
	}
	hs_end = modulo(hs_begin + hsync_pixels_venc, total_pixels_venc);
	hd_write_reg(P_ENCP_DVI_HSO_BEGIN,  hs_begin);
	hd_write_reg(P_ENCP_DVI_HSO_END, hs_end);

	/* Program Vsync timing for even field */
	if (de_v_begin_even >= SOF_LINES + VSYNC_LINES + (1 - vs_adjust))
		vs_bline_evn = de_v_begin_even - SOF_LINES - VSYNC_LINES
			- (1 - vs_adjust);
	else
		vs_bline_evn = TOTAL_LINES + de_v_begin_even - SOF_LINES
			- VSYNC_LINES - (1 - vs_adjust);

	vs_eline_evn = modulo(vs_bline_evn + VSYNC_LINES, TOTAL_LINES);
	hd_write_reg(P_ENCP_DVI_VSO_BLINE_EVN, vs_bline_evn);   /* 0 */
	hd_write_reg(P_ENCP_DVI_VSO_ELINE_EVN, vs_eline_evn);   /* 5 */
	vso_begin_evn = hs_begin; /* 2 */
	hd_write_reg(P_ENCP_DVI_VSO_BEGIN_EVN, vso_begin_evn);  /* 2 */
	hd_write_reg(P_ENCP_DVI_VSO_END_EVN, vso_begin_evn);  /* 2 */
	/* Program Vsync timing for odd field if needed */
	if (INTERLACE_MODE) {
		vs_bline_odd = de_v_begin_odd - 1 - SOF_LINES - VSYNC_LINES;
		vs_eline_odd = de_v_begin_odd - 1 - SOF_LINES;
		vso_begin_odd  = modulo(hs_begin + (total_pixels_venc >> 1),
					total_pixels_venc);
		hd_write_reg(P_ENCP_DVI_VSO_BLINE_ODD, vs_bline_odd);
		hd_write_reg(P_ENCP_DVI_VSO_ELINE_ODD, vs_eline_odd);
		hd_write_reg(P_ENCP_DVI_VSO_BEGIN_ODD, vso_begin_odd);
		hd_write_reg(P_ENCP_DVI_VSO_END_ODD, vso_begin_odd);
	}

	hd_write_reg(P_VPU_HDMI_SETTING, (0 << 0) |
		(0 << 1) |
		(HSYNC_POLARITY << 2) |
		(VSYNC_POLARITY << 3) |
		(0 << 4) |
		(4 << 5) |
		(0 << 8) |
		(0 << 12)
	);
}

static void hdmi_tvenc4k2k_set(struct hdmi_format_para *param)
{
	unsigned long VFIFO2VD_TO_HDMI_LATENCY = 2;
	unsigned long TOTAL_PIXELS = 4400, PIXEL_REPEAT_HDMI = 0,
		PIXEL_REPEAT_VENC = 0, ACTIVE_PIXELS = 3840;
	unsigned int FRONT_PORCH = 1020, HSYNC_PIXELS = 0, ACTIVE_LINES = 2160,
		INTERLACE_MODE = 0U, TOTAL_LINES = 0, SOF_LINES = 0,
		VSYNC_LINES = 0;
	unsigned int LINES_F0 = 2250, LINES_F1 = 2250, BACK_PORCH = 0,
		EOF_LINES = 8, TOTAL_FRAMES = 0;

	unsigned long total_pixels_venc = 0;
	unsigned long active_pixels_venc = 0;
	unsigned long front_porch_venc = 0;
	unsigned long hsync_pixels_venc = 0;

	unsigned long de_h_begin = 0, de_h_end = 0;
	unsigned long de_v_begin_even = 0, de_v_end_even = 0;
	unsigned long hs_begin = 0, hs_end = 0;
	unsigned long vs_adjust = 0;
	unsigned long vs_adjust_420 = 0;
	unsigned long vs_bline_evn = 0, vs_eline_evn  = 0;
	unsigned long vso_begin_evn = 0;

/* Due to 444->420 line buffer latency, the active line output from
 * 444->420 conversion will be delayed by 1 line. So for 420 mode,
 * we need to delay Vsync by 1 line as well, to meet the timing
 */
	if (global_tx_hw->chip_data->chip_type > MESON_CPU_ID_TM2 &&
		is_hdmi4k_support_420(param->vic) &&
		param->cs == HDMI_COLORSPACE_YUV420)
		vs_adjust_420 = 1;

	switch (param->vic) {
	case HDMI_95_3840x2160p30_16x9:
	case HDMI_97_3840x2160p60_16x9:
		INTERLACE_MODE = 0U;
		PIXEL_REPEAT_VENC = 0;
		PIXEL_REPEAT_HDMI = 0;
		ACTIVE_PIXELS = (3840 * (1 + PIXEL_REPEAT_HDMI));
		ACTIVE_LINES = (2160 / (1 + INTERLACE_MODE));
		LINES_F0 = 2250;
		LINES_F1 = 2250;
		FRONT_PORCH = 176;
		HSYNC_PIXELS = 88;
		BACK_PORCH = 296;
		EOF_LINES = 8 + 1;
		VSYNC_LINES = 10;
		SOF_LINES = 72 + 1;
		TOTAL_FRAMES = 3;
		break;
	case HDMI_94_3840x2160p25_16x9:
	case HDMI_96_3840x2160p50_16x9:
		INTERLACE_MODE = 0U;
		PIXEL_REPEAT_VENC = 0;
		PIXEL_REPEAT_HDMI = 0;
		ACTIVE_PIXELS = (3840 * (1 + PIXEL_REPEAT_HDMI));
		ACTIVE_LINES = (2160 / (1 + INTERLACE_MODE));
		LINES_F0 = 2250;
		LINES_F1 = 2250;
		FRONT_PORCH = 1056;
		HSYNC_PIXELS = 88;
		BACK_PORCH = 296;
		EOF_LINES = 8 + 1;
		VSYNC_LINES = 10;
		SOF_LINES = 72 + 1;
		TOTAL_FRAMES = 3;
		break;
	case HDMI_93_3840x2160p24_16x9:
		INTERLACE_MODE = 0U;
		PIXEL_REPEAT_VENC = 0;
		PIXEL_REPEAT_HDMI = 0;
		ACTIVE_PIXELS = (3840 * (1 + PIXEL_REPEAT_HDMI));
		ACTIVE_LINES = (2160 / (1 + INTERLACE_MODE));
		LINES_F0 = 2250;
		LINES_F1 = 2250;
		FRONT_PORCH = 1276;
		HSYNC_PIXELS = 88;
		BACK_PORCH = 296;
		EOF_LINES = 8 + 1;
		VSYNC_LINES = 10;
		SOF_LINES = 72 + 1;
		TOTAL_FRAMES = 3;
		break;
	case HDMI_98_4096x2160p24_256x135:
		INTERLACE_MODE = 0U;
		PIXEL_REPEAT_VENC = 0;
		PIXEL_REPEAT_HDMI = 0;
		ACTIVE_PIXELS = (4096 * (1 + PIXEL_REPEAT_HDMI));
		ACTIVE_LINES = (2160 / (1 + INTERLACE_MODE));
		LINES_F0 = 2250;
		LINES_F1 = 2250;
		FRONT_PORCH = 1020;
		HSYNC_PIXELS = 88;
		BACK_PORCH = 296;
		EOF_LINES = 8 + 1;
		VSYNC_LINES = 10;
		SOF_LINES = 72 + 1;
		TOTAL_FRAMES = 3;
		break;
	case HDMI_99_4096x2160p25_256x135:
	case HDMI_101_4096x2160p50_256x135:
		INTERLACE_MODE = 0U;
		PIXEL_REPEAT_VENC = 0;
		PIXEL_REPEAT_HDMI = 0;
		ACTIVE_PIXELS = (4096 * (1 + PIXEL_REPEAT_HDMI));
		ACTIVE_LINES = (2160 / (1 + INTERLACE_MODE));
		LINES_F0 = 2250;
		LINES_F1 = 2250;
		FRONT_PORCH = 968;
		HSYNC_PIXELS = 88;
		BACK_PORCH = 128;
		EOF_LINES = 8;
		VSYNC_LINES = 10;
		SOF_LINES = 72;
		TOTAL_FRAMES = 3;
		break;
	case HDMI_100_4096x2160p30_256x135:
	case HDMI_102_4096x2160p60_256x135:
		INTERLACE_MODE = 0U;
		PIXEL_REPEAT_VENC = 0;
		PIXEL_REPEAT_HDMI = 0;
		ACTIVE_PIXELS = (4096 * (1 + PIXEL_REPEAT_HDMI));
		ACTIVE_LINES = (2160 / (1 + INTERLACE_MODE));
		LINES_F0 = 2250;
		LINES_F1 = 2250;
		FRONT_PORCH = 88;
		HSYNC_PIXELS = 88;
		BACK_PORCH = 128;
		EOF_LINES = 8;
		VSYNC_LINES = 10;
		SOF_LINES = 72;
		TOTAL_FRAMES = 3;
		break;
	default:
		HDMITX_INFO("hdmitx20: no setting for VIC = %d\n", param->vic);
		break;
	}

	TOTAL_PIXELS = FRONT_PORCH + HSYNC_PIXELS + BACK_PORCH + ACTIVE_PIXELS;
	TOTAL_LINES = LINES_F0 + LINES_F1 * INTERLACE_MODE;

	total_pixels_venc = (TOTAL_PIXELS  / (1 + PIXEL_REPEAT_HDMI)) *
		(1 + PIXEL_REPEAT_VENC);
	active_pixels_venc = (ACTIVE_PIXELS / (1 + PIXEL_REPEAT_HDMI)) *
		(1 + PIXEL_REPEAT_VENC);
	front_porch_venc = (FRONT_PORCH   / (1 + PIXEL_REPEAT_HDMI)) *
		(1 + PIXEL_REPEAT_VENC);
	hsync_pixels_venc = (HSYNC_PIXELS  / (1 + PIXEL_REPEAT_HDMI)) *
		(1 + PIXEL_REPEAT_VENC);

	de_h_begin = modulo(hd_read_reg(P_ENCP_VIDEO_HAVON_BEGIN) +
		VFIFO2VD_TO_HDMI_LATENCY, total_pixels_venc);
	de_h_end  = modulo(de_h_begin + active_pixels_venc, total_pixels_venc);
	hd_write_reg(P_ENCP_DE_H_BEGIN, de_h_begin);
	hd_write_reg(P_ENCP_DE_H_END, de_h_end);
	/* Program DE timing for even field */
	de_v_begin_even = hd_read_reg(P_ENCP_VIDEO_VAVON_BLINE);
	de_v_end_even  = modulo(de_v_begin_even + ACTIVE_LINES, TOTAL_LINES);
	hd_write_reg(P_ENCP_DE_V_BEGIN_EVEN, de_v_begin_even);
	hd_write_reg(P_ENCP_DE_V_END_EVEN,  de_v_end_even);

	/* Program Hsync timing */
	if (de_h_end + front_porch_venc >= total_pixels_venc) {
		hs_begin = de_h_end + front_porch_venc - total_pixels_venc;
		vs_adjust  = 1;
	} else {
		hs_begin = de_h_end + front_porch_venc;
		vs_adjust = 1;
	}
	hs_end = modulo(hs_begin + hsync_pixels_venc, total_pixels_venc);
	hd_write_reg(P_ENCP_DVI_HSO_BEGIN,  hs_begin);
	hd_write_reg(P_ENCP_DVI_HSO_END, hs_end);

	/* Program Vsync timing for even field */
	if (de_v_begin_even + vs_adjust_420 >=
	    SOF_LINES + VSYNC_LINES + (1 - vs_adjust))
		vs_bline_evn = de_v_begin_even + vs_adjust_420 - SOF_LINES -
			VSYNC_LINES - (1 - vs_adjust);
	else
		vs_bline_evn = TOTAL_LINES + de_v_begin_even + vs_adjust_420 -
			SOF_LINES - VSYNC_LINES - (1 - vs_adjust);
	vs_eline_evn = modulo(vs_bline_evn + VSYNC_LINES, TOTAL_LINES);
	hd_write_reg(P_ENCP_DVI_VSO_BLINE_EVN, vs_bline_evn);
	hd_write_reg(P_ENCP_DVI_VSO_ELINE_EVN, vs_eline_evn);
	vso_begin_evn = hs_begin;
	hd_write_reg(P_ENCP_DVI_VSO_BEGIN_EVN, vso_begin_evn);
	hd_write_reg(P_ENCP_DVI_VSO_END_EVN, vso_begin_evn);
	hd_write_reg(P_VPU_HDMI_SETTING, (0 << 0) |
			(0 << 1) |
			(HSYNC_POLARITY << 2) |
			(VSYNC_POLARITY << 3) |
			(0 << 4) |
			(4 << 5) |
			(0 << 8) |
			(0 << 12)
	);

	if ((is_hdmi4k_support_420(param->vic) &&
		param->cs == HDMI_COLORSPACE_YUV420) &&
	    global_tx_hw->chip_data->chip_type >= MESON_CPU_ID_TM2B) {
		hd_set_reg_bits(P_VPU_HDMI_SETTING, 0, 8, 1);
		hd_set_reg_bits(P_VPU_HDMI_SETTING, 1, 20, 1);
	}
}

static void hdmi_tvenc480i_set(struct hdmi_format_para *param)
{
	unsigned long VFIFO2VD_TO_HDMI_LATENCY = 1;
	unsigned long TOTAL_PIXELS = 0, PIXEL_REPEAT_HDMI = 0,
		PIXEL_REPEAT_VENC = 0, ACTIVE_PIXELS = 0;
	unsigned int FRONT_PORCH = 38, HSYNC_PIXELS = 124, ACTIVE_LINES = 0,
		INTERLACE_MODE = 0, TOTAL_LINES = 0, SOF_LINES = 0,
		VSYNC_LINES = 0;
	unsigned int LINES_F0 = 262, LINES_F1 = 263, BACK_PORCH = 114,
		EOF_LINES = 2, TOTAL_FRAMES = 0;

	unsigned long total_pixels_venc = 0;
	unsigned long active_pixels_venc = 0;
	unsigned long front_porch_venc = 0;
	unsigned long hsync_pixels_venc = 0;

	unsigned long de_h_begin = 0, de_h_end = 0;
	unsigned long de_v_begin_even = 0, de_v_end_even = 0,
		de_v_begin_odd = 0, de_v_end_odd = 0;
	unsigned long hs_begin = 0, hs_end = 0;
	unsigned long vs_adjust = 0;
	unsigned long vs_bline_evn = 0, vs_eline_evn = 0,
		vs_bline_odd = 0, vs_eline_odd = 0;
	unsigned long vso_begin_evn = 0, vso_begin_odd = 0;

	if (global_tx_hw->chip_data->chip_type < MESON_CPU_ID_SC2)
		hd_set_reg_bits(P_HHI_GCLK_OTHER, 1, 8, 1);

	switch (param->vic) {
	case HDMI_6_720x480i60_4x3:
	case HDMI_7_720x480i60_16x9:
	case HDMI_11_2880x480i60_16x9:
		INTERLACE_MODE = 1;
		PIXEL_REPEAT_VENC = 1;
		PIXEL_REPEAT_HDMI = 1;
		ACTIVE_PIXELS	= (720 * (1 + PIXEL_REPEAT_HDMI));
		ACTIVE_LINES = (480 / (1 + INTERLACE_MODE));
		LINES_F0 = 262;
		LINES_F1 = 263;
		FRONT_PORCH = 38;
		HSYNC_PIXELS = 124;
		BACK_PORCH = 114;
		EOF_LINES = 4;
		VSYNC_LINES = 3;
		SOF_LINES = 15;
		TOTAL_FRAMES = 4;
	break;
	case HDMI_21_720x576i50_4x3:
	case HDMI_22_720x576i50_16x9:
	case HDMI_26_2880x576i50_16x9:
		INTERLACE_MODE = 1;
		PIXEL_REPEAT_VENC = 1;
		PIXEL_REPEAT_HDMI = 1;
		ACTIVE_PIXELS	= (720 * (1 + PIXEL_REPEAT_HDMI));
		ACTIVE_LINES = (576 / (1 + INTERLACE_MODE));
		LINES_F0 = 312;
		LINES_F1 = 313;
		FRONT_PORCH = 24;
		HSYNC_PIXELS = 126;
		BACK_PORCH = 138;
		EOF_LINES = 2;
		VSYNC_LINES = 3;
		SOF_LINES = 19;
		TOTAL_FRAMES = 4;
		break;
	default:
		break;
	}

	TOTAL_PIXELS = FRONT_PORCH + HSYNC_PIXELS + BACK_PORCH + ACTIVE_PIXELS;
	TOTAL_LINES = LINES_F0 + LINES_F1 * INTERLACE_MODE;

	total_pixels_venc = (TOTAL_PIXELS  / (1 + PIXEL_REPEAT_HDMI)) *
		(1 + PIXEL_REPEAT_VENC); /* 1716 / 2 * 2 = 1716 */
	active_pixels_venc = (ACTIVE_PIXELS / (1 + PIXEL_REPEAT_HDMI)) *
		(1 + PIXEL_REPEAT_VENC);
	front_porch_venc = (FRONT_PORCH   / (1 + PIXEL_REPEAT_HDMI)) *
		(1 + PIXEL_REPEAT_VENC); /* 38   / 2 * 2 = 38 */
	hsync_pixels_venc = (HSYNC_PIXELS  / (1 + PIXEL_REPEAT_HDMI)) *
		(1 + PIXEL_REPEAT_VENC); /* 124  / 2 * 2 = 124 */

	de_h_begin = modulo(hd_read_reg(P_ENCI_VFIFO2VD_PIXEL_START) +
		VFIFO2VD_TO_HDMI_LATENCY, total_pixels_venc);
	de_h_end  = modulo(de_h_begin + active_pixels_venc, total_pixels_venc);
	hd_write_reg(P_ENCI_DE_H_BEGIN, de_h_begin);	/* 235 */
	hd_write_reg(P_ENCI_DE_H_END, de_h_end);	 /* 1675 */

	de_v_begin_even = hd_read_reg(P_ENCI_VFIFO2VD_LINE_TOP_START);
	de_v_end_even  = de_v_begin_even + ACTIVE_LINES;
	de_v_begin_odd = hd_read_reg(P_ENCI_VFIFO2VD_LINE_BOT_START);
	de_v_end_odd = de_v_begin_odd + ACTIVE_LINES;
	hd_write_reg(P_ENCI_DE_V_BEGIN_EVEN, de_v_begin_even);
	hd_write_reg(P_ENCI_DE_V_END_EVEN,  de_v_end_even);
	hd_write_reg(P_ENCI_DE_V_BEGIN_ODD, de_v_begin_odd);
	hd_write_reg(P_ENCI_DE_V_END_ODD, de_v_end_odd);

	/* Program Hsync timing */
	if (de_h_end + front_porch_venc >= total_pixels_venc) {
		hs_begin = de_h_end + front_porch_venc - total_pixels_venc;
		vs_adjust  = 1;
	} else {
		hs_begin = de_h_end + front_porch_venc;
		vs_adjust  = 0;
	}
	hs_end = modulo(hs_begin + hsync_pixels_venc, total_pixels_venc);
	hd_write_reg(P_ENCI_DVI_HSO_BEGIN,  hs_begin);  /* 1713 */
	hd_write_reg(P_ENCI_DVI_HSO_END, hs_end);	/* 121 */

	/* Program Vsync timing for even field */
	if (de_v_end_odd - 1 + EOF_LINES + vs_adjust >= LINES_F1) {
		vs_bline_evn = de_v_end_odd - 1 + EOF_LINES + vs_adjust
			- LINES_F1;
		vs_eline_evn = vs_bline_evn + VSYNC_LINES;
		hd_write_reg(P_ENCI_DVI_VSO_BLINE_EVN, vs_bline_evn);
		/* vso_bline_evn_reg_wr_cnt ++; */
		hd_write_reg(P_ENCI_DVI_VSO_ELINE_EVN, vs_eline_evn);
		/* vso_eline_evn_reg_wr_cnt ++; */
		hd_write_reg(P_ENCI_DVI_VSO_BEGIN_EVN, hs_begin);
		hd_write_reg(P_ENCI_DVI_VSO_END_EVN, hs_begin);
	} else {
		vs_bline_odd = de_v_end_odd - 1 + EOF_LINES + vs_adjust;
		hd_write_reg(P_ENCI_DVI_VSO_BLINE_ODD, vs_bline_odd);
		/* vso_bline_odd_reg_wr_cnt ++; */
		hd_write_reg(P_ENCI_DVI_VSO_BEGIN_ODD, hs_begin);
	if (vs_bline_odd + VSYNC_LINES >= LINES_F1) {
		vs_eline_evn = vs_bline_odd + VSYNC_LINES - LINES_F1;
		hd_write_reg(P_ENCI_DVI_VSO_ELINE_EVN, vs_eline_evn);
		/* vso_eline_evn_reg_wr_cnt ++; */
		hd_write_reg(P_ENCI_DVI_VSO_END_EVN, hs_begin);
	} else {
		vs_eline_odd = vs_bline_odd + VSYNC_LINES;
		hd_write_reg(P_ENCI_DVI_VSO_ELINE_ODD, vs_eline_odd);
		/* vso_eline_odd_reg_wr_cnt ++; */
		hd_write_reg(P_ENCI_DVI_VSO_END_ODD, hs_begin);
	}
	}
	/* Program Vsync timing for odd field */
	if (de_v_end_even - 1 + EOF_LINES + 1 >= LINES_F0) {
		vs_bline_odd = de_v_end_even - 1 + EOF_LINES + 1 - LINES_F0;
		vs_eline_odd = vs_bline_odd + VSYNC_LINES;
		hd_write_reg(P_ENCI_DVI_VSO_BLINE_ODD, vs_bline_odd);
		/* vso_bline_odd_reg_wr_cnt ++; */
		hd_write_reg(P_ENCI_DVI_VSO_ELINE_ODD, vs_eline_odd);
		/* vso_eline_odd_reg_wr_cnt ++; */
		vso_begin_odd  = modulo(hs_begin + (total_pixels_venc >> 1),
					total_pixels_venc);
		hd_write_reg(P_ENCI_DVI_VSO_BEGIN_ODD, vso_begin_odd);
		hd_write_reg(P_ENCI_DVI_VSO_END_ODD, vso_begin_odd);
	} else {
		vs_bline_evn = de_v_end_even - 1 + EOF_LINES + 1;
		hd_write_reg(P_ENCI_DVI_VSO_BLINE_EVN, vs_bline_evn); /* 261 */
		/* vso_bline_evn_reg_wr_cnt ++; */
		vso_begin_evn  = modulo(hs_begin + (total_pixels_venc >> 1),
					total_pixels_venc);
		hd_write_reg(P_ENCI_DVI_VSO_BEGIN_EVN, vso_begin_evn);
	if (vs_bline_evn + VSYNC_LINES >= LINES_F0) {
		vs_eline_odd = vs_bline_evn + VSYNC_LINES - LINES_F0;
		hd_write_reg(P_ENCI_DVI_VSO_ELINE_ODD, vs_eline_odd);
		/* vso_eline_odd_reg_wr_cnt ++; */
		hd_write_reg(P_ENCI_DVI_VSO_END_ODD, vso_begin_evn);
	} else {
		vs_eline_evn = vs_bline_evn + VSYNC_LINES;
		hd_write_reg(P_ENCI_DVI_VSO_ELINE_EVN, vs_eline_evn);
		/* vso_eline_evn_reg_wr_cnt ++; */
		hd_write_reg(P_ENCI_DVI_VSO_END_EVN, vso_begin_evn);
	}
	}

	hd_write_reg(P_VPU_HDMI_SETTING, (0 << 0) |
			(0 << 1) |
			(0 << 2) |
			(0 << 3) |
			(0 << 4) |
			(4 << 5) |
			(1 << 8) |
			(1 << 12)
	);
	if (param->vic == HDMI_11_2880x480i60_16x9 ||
	    param->vic == HDMI_26_2880x576i50_16x9)
		hd_set_reg_bits(P_VPU_HDMI_SETTING, 3, 12, 4);
}

static void hdmi_tvenc_vesa_set(struct hdmi_format_para *param)
{
	unsigned long VFIFO2VD_TO_HDMI_LATENCY = 2;
	unsigned long TOTAL_PIXELS = 0, PIXEL_REPEAT_HDMI = 0,
		PIXEL_REPEAT_VENC = 0, ACTIVE_PIXELS = 0;
	unsigned int FRONT_PORCH = 0, HSYNC_PIXELS = 0, ACTIVE_LINES = 0,
		INTERLACE_MODE = 0, TOTAL_LINES = 0, SOF_LINES = 0,
		VSYNC_LINES = 0;
	unsigned int LINES_F0 = 0, LINES_F1 = 0, BACK_PORCH = 0,
		EOF_LINES = 0, TOTAL_FRAMES = 0;

	unsigned long total_pixels_venc = 0;
	unsigned long active_pixels_venc = 0;
	unsigned long front_porch_venc = 0;
	unsigned long hsync_pixels_venc = 0;

	unsigned long de_h_begin = 0, de_h_end = 0;
	unsigned long de_v_begin_even = 0, de_v_end_even = 0;
	unsigned long hs_begin = 0, hs_end = 0;
	unsigned long vs_adjust = 0;
	unsigned long vs_bline_evn = 0, vs_eline_evn = 0;
	unsigned long vso_begin_evn = 0;
	const struct hdmi_timing *vtiming = NULL;

	vtiming = hdmitx_mode_vic_to_hdmi_timing(param->vic);
	if (!vtiming) {
		HDMITX_INFO("don't find Paras for VESA %d\n", param->vic);
		return;
	}

	INTERLACE_MODE = 0;
	PIXEL_REPEAT_VENC = 0;
	PIXEL_REPEAT_HDMI = 0;
	ACTIVE_PIXELS = vtiming->h_active;
	ACTIVE_LINES = vtiming->v_active;
	if (vtiming->pi_mode == 0)
		ACTIVE_LINES = ACTIVE_LINES >> 1;
	LINES_F0 = vtiming->v_total;
	LINES_F1 = vtiming->v_total;
	FRONT_PORCH = vtiming->h_front;
	HSYNC_PIXELS = vtiming->h_sync;
	BACK_PORCH = vtiming->h_back;
	EOF_LINES = vtiming->v_front;
	VSYNC_LINES = vtiming->v_sync;
	SOF_LINES = vtiming->v_back;
	TOTAL_FRAMES = 4;

	TOTAL_PIXELS = FRONT_PORCH + HSYNC_PIXELS + BACK_PORCH + ACTIVE_PIXELS;
	TOTAL_LINES = (LINES_F0 + LINES_F1 * INTERLACE_MODE);

	total_pixels_venc = (TOTAL_PIXELS  / (1 + PIXEL_REPEAT_HDMI)) *
		(1 + PIXEL_REPEAT_VENC);
	active_pixels_venc = (ACTIVE_PIXELS / (1 + PIXEL_REPEAT_HDMI)) *
		(1 + PIXEL_REPEAT_VENC);
	front_porch_venc = (FRONT_PORCH   / (1 + PIXEL_REPEAT_HDMI)) *
		(1 + PIXEL_REPEAT_VENC);
	hsync_pixels_venc = (HSYNC_PIXELS  / (1 + PIXEL_REPEAT_HDMI)) *
		(1 + PIXEL_REPEAT_VENC);

	hd_write_reg(P_ENCP_VIDEO_MODE, hd_read_reg(P_ENCP_VIDEO_MODE)
		| (1 << 14));
	/* Program DE timing */
	de_h_begin = modulo(hd_read_reg(P_ENCP_VIDEO_HAVON_BEGIN) +
		VFIFO2VD_TO_HDMI_LATENCY,  total_pixels_venc);
	de_h_end  = modulo(de_h_begin + active_pixels_venc, total_pixels_venc);
	hd_write_reg(P_ENCP_DE_H_BEGIN, de_h_begin);	/* 220 */
	hd_write_reg(P_ENCP_DE_H_END, de_h_end);	 /* 1660 */
	/* Program DE timing for even field */
	de_v_begin_even = hd_read_reg(P_ENCP_VIDEO_VAVON_BLINE);
	de_v_end_even  = de_v_begin_even + ACTIVE_LINES;
	hd_write_reg(P_ENCP_DE_V_BEGIN_EVEN, de_v_begin_even);
	hd_write_reg(P_ENCP_DE_V_END_EVEN,  de_v_end_even);	/* 522 */

	/* Program Hsync timing */
	if (de_h_end + front_porch_venc >= total_pixels_venc) {
		hs_begin = de_h_end + front_porch_venc - total_pixels_venc;
		vs_adjust  = 1;
	} else {
		hs_begin = de_h_end + front_porch_venc;
		vs_adjust  = 0;
	}
	hs_end = modulo(hs_begin + hsync_pixels_venc, total_pixels_venc);
	hd_write_reg(P_ENCP_DVI_HSO_BEGIN,  hs_begin);
	hd_write_reg(P_ENCP_DVI_HSO_END, hs_end);
	/* Program Vsync timing for even field */
	if (de_v_begin_even >= SOF_LINES + VSYNC_LINES + (1 - vs_adjust))
		vs_bline_evn = de_v_begin_even - SOF_LINES - VSYNC_LINES -
			(1 - vs_adjust);
	else
		vs_bline_evn = TOTAL_LINES + de_v_begin_even - SOF_LINES -
			VSYNC_LINES - (1 - vs_adjust);
	vs_eline_evn = modulo(vs_bline_evn + VSYNC_LINES, TOTAL_LINES);
	hd_write_reg(P_ENCP_DVI_VSO_BLINE_EVN, vs_bline_evn);   /* 5 */
	hd_write_reg(P_ENCP_DVI_VSO_ELINE_EVN, vs_eline_evn);   /* 11 */
	vso_begin_evn = hs_begin; /* 1692 */
	hd_write_reg(P_ENCP_DVI_VSO_BEGIN_EVN, vso_begin_evn);  /* 1692 */
	hd_write_reg(P_ENCP_DVI_VSO_END_EVN, vso_begin_evn);  /* 1692 */

	switch (param->vic) {
	default:
		hd_write_reg(P_VPU_HDMI_SETTING, (0 << 0) |
				(0 << 1) | /* [	1] src_sel_encp */
				(HSYNC_POLARITY << 2) |
				(VSYNC_POLARITY << 3) |
				(0 << 4) |
				(4 << 5) |
				(0 << 8) |
				(0 << 12)
		);
	}
}

static void hdmi_tvenc_set(struct hdmi_format_para *param)
{
	unsigned long VFIFO2VD_TO_HDMI_LATENCY = 2;
	unsigned long TOTAL_PIXELS = 0, PIXEL_REPEAT_HDMI = 0,
		PIXEL_REPEAT_VENC = 0, ACTIVE_PIXELS = 0;
	unsigned int FRONT_PORCH = 0, HSYNC_PIXELS = 0, ACTIVE_LINES = 0,
		INTERLACE_MODE = 0U, TOTAL_LINES = 0, SOF_LINES = 0,
		VSYNC_LINES = 0;
	unsigned int LINES_F0 = 0, LINES_F1 = 0, BACK_PORCH = 0,
		EOF_LINES = 0, TOTAL_FRAMES = 0;

	unsigned long total_pixels_venc = 0;
	unsigned long active_pixels_venc = 0;
	unsigned long front_porch_venc = 0;
	unsigned long hsync_pixels_venc = 0;

	unsigned long de_h_begin = 0, de_h_end = 0;
	unsigned long de_v_begin_even = 0, de_v_end_even = 0;
	unsigned long hs_begin = 0, hs_end = 0;
	unsigned long vs_adjust = 0;
	unsigned long vs_bline_evn = 0, vs_eline_evn = 0;
	unsigned long vso_begin_evn = 0;
	const struct hdmi_timing *hdmi_encp_timing = NULL;

	if ((param->vic & HDMITX_VESA_OFFSET) == HDMITX_VESA_OFFSET) {
		/* VESA modes setting */
		hdmi_tvenc_vesa_set(param);
		return;
	}

	hdmi_encp_timing = hdmitx_mode_vic_to_hdmi_timing(param->vic);
	if (!hdmi_encp_timing) {
		HDMITX_INFO("don't find Paras for VIC : %d\n", param->vic);
		return;
	} // TODO

	switch (param->vic) {
	case HDMI_109_1280x720p48_64x27:
		INTERLACE_MODE = 0U;
		PIXEL_REPEAT_VENC = 0;
		PIXEL_REPEAT_HDMI = 0;
		ACTIVE_PIXELS	= 3840;
		ACTIVE_LINES = 1080;
		LINES_F0 = 1125;
		LINES_F1 = 1125;
		FRONT_PORCH = 176;
		HSYNC_PIXELS = 88;
		BACK_PORCH = 296;
		EOF_LINES = 4;
		VSYNC_LINES = 5;
		SOF_LINES = 36;
		TOTAL_FRAMES = 0;
		break;
	case HDMI_110_1680x720p48_64x27:
		INTERLACE_MODE = 0U;
		PIXEL_REPEAT_VENC = 0;
		PIXEL_REPEAT_HDMI = 0;
		ACTIVE_PIXELS	= 3840;
		ACTIVE_LINES = 1080;
		LINES_F0 = 1125;
		LINES_F1 = 1125;
		FRONT_PORCH = 1056;
		HSYNC_PIXELS = 88;
		BACK_PORCH = 296;
		EOF_LINES = 4;
		VSYNC_LINES = 5;
		SOF_LINES = 36;
		TOTAL_FRAMES = 0;
		break;
	case HDMI_111_1920x1080p48_16x9:
		INTERLACE_MODE = 0U;
		PIXEL_REPEAT_VENC = 0;
		PIXEL_REPEAT_HDMI = 0;
		ACTIVE_PIXELS	= 3840;
		ACTIVE_LINES = 1080;
		LINES_F0 = 562;
		LINES_F1 = 562;
		FRONT_PORCH = 176;
		HSYNC_PIXELS = 88;
		BACK_PORCH = 296;
		EOF_LINES = 2;
		VSYNC_LINES = 2;
		SOF_LINES = 18;
		TOTAL_FRAMES = 0;
		break;
	case HDMI_112_1920x1080p48_64x27:
		INTERLACE_MODE = 0U;
		PIXEL_REPEAT_VENC = 0;
		PIXEL_REPEAT_HDMI = 0;
		ACTIVE_PIXELS	= 3840;
		ACTIVE_LINES = 1080;
		LINES_F0 = 562;
		LINES_F1 = 562;
		FRONT_PORCH = 1056;
		HSYNC_PIXELS = 88;
		BACK_PORCH = 296;
		EOF_LINES = 2;
		VSYNC_LINES = 2;
		SOF_LINES = 18;
		TOTAL_FRAMES = 0;
		break;
	case HDMI_2_720x480p60_4x3:
	case HDMI_3_720x480p60_16x9:
	case HDMI_36_2880x480p60_16x9:
		INTERLACE_MODE = 0U;
		PIXEL_REPEAT_VENC = 0;
		PIXEL_REPEAT_HDMI = 0;
		ACTIVE_PIXELS	= (720 * (1 + PIXEL_REPEAT_HDMI));
		ACTIVE_LINES = (480 / (1 + INTERLACE_MODE));
		LINES_F0 = 525;
		LINES_F1 = 525;
		FRONT_PORCH = 16;
		HSYNC_PIXELS = 62;
		BACK_PORCH = 60;
		EOF_LINES = 9;
		VSYNC_LINES = 6;
		SOF_LINES = 30;
		TOTAL_FRAMES = 4;
		break;
	case HDMI_17_720x576p50_4x3:
	case HDMI_18_720x576p50_16x9:
	case HDMI_38_2880x576p50_16x9:
		INTERLACE_MODE = 0U;
		PIXEL_REPEAT_VENC = 0;
		PIXEL_REPEAT_HDMI = 0;
		ACTIVE_PIXELS	= (720 * (1 + PIXEL_REPEAT_HDMI));
		ACTIVE_LINES = (576 / (1 + INTERLACE_MODE));
		LINES_F0 = 625;
		LINES_F1 = 625;
		FRONT_PORCH = 12;
		HSYNC_PIXELS = 64;
		BACK_PORCH = 68;
		EOF_LINES = 5;
		VSYNC_LINES = 5;
		SOF_LINES = 39;
		TOTAL_FRAMES = 4;
		break;
	case HDMI_4_1280x720p60_16x9:
		INTERLACE_MODE = 0U;
		PIXEL_REPEAT_VENC = 0;
		PIXEL_REPEAT_HDMI = 0;
		ACTIVE_PIXELS	= (1280 * (1 + PIXEL_REPEAT_HDMI));
		ACTIVE_LINES = (720 / (1 + INTERLACE_MODE));
		LINES_F0 = 750;
		LINES_F1 = 750;
		FRONT_PORCH = 110;
		HSYNC_PIXELS = 40;
		BACK_PORCH = 220;
		EOF_LINES = 5;
		VSYNC_LINES = 5;
		SOF_LINES = 20;
		TOTAL_FRAMES = 4;
		break;
	case HDMI_19_1280x720p50_16x9:
		INTERLACE_MODE = 0U;
		PIXEL_REPEAT_VENC = 0;
		PIXEL_REPEAT_HDMI = 0;
		ACTIVE_PIXELS	= (1280 * (1 + PIXEL_REPEAT_HDMI));
		ACTIVE_LINES = (720 / (1 + INTERLACE_MODE));
		LINES_F0 = 750;
		LINES_F1 = 750;
		FRONT_PORCH = 440;
		HSYNC_PIXELS = 40;
		BACK_PORCH = 220;
		EOF_LINES = 5;
		VSYNC_LINES = 5;
		SOF_LINES = 20;
		TOTAL_FRAMES = 4;
		break;
	case HDMI_31_1920x1080p50_16x9:
	case HDMI_33_1920x1080p25_16x9:
		INTERLACE_MODE	= 0U;
		PIXEL_REPEAT_VENC  = 0;
		PIXEL_REPEAT_HDMI  = 0;
		ACTIVE_PIXELS = (1920 * (1 + PIXEL_REPEAT_HDMI));
		ACTIVE_LINES = (1080 / (1 + INTERLACE_MODE));
		LINES_F0 = 1125;
		LINES_F1 = 1125;
		FRONT_PORCH = 528;
		HSYNC_PIXELS = 44;
		BACK_PORCH = 148;
		EOF_LINES = 4;
		VSYNC_LINES = 5;
		SOF_LINES = 36;
		TOTAL_FRAMES = 4;
		break;
	case HDMI_32_1920x1080p24_16x9:
		INTERLACE_MODE	= 0U;
		PIXEL_REPEAT_VENC  = 0;
		PIXEL_REPEAT_HDMI  = 0;
		ACTIVE_PIXELS = (1920 * (1 + PIXEL_REPEAT_HDMI));
		ACTIVE_LINES = (1080 / (1 + INTERLACE_MODE));
		LINES_F0 = 1125;
		LINES_F1 = 1125;
		FRONT_PORCH = 638;
		HSYNC_PIXELS = 44;
		BACK_PORCH = 148;
		EOF_LINES = 4;
		VSYNC_LINES = 5;
		SOF_LINES = 36;
		TOTAL_FRAMES = 4;
		break;
	case HDMI_16_1920x1080p60_16x9:
	case HDMI_34_1920x1080p30_16x9:
	case HDMI_63_1920x1080p120_16x9:
		INTERLACE_MODE	= 0U;
		PIXEL_REPEAT_VENC  = 0;
		PIXEL_REPEAT_HDMI  = 0;
		ACTIVE_PIXELS = (1920 * (1 + PIXEL_REPEAT_HDMI));
		ACTIVE_LINES = (1080 / (1 + INTERLACE_MODE));
		LINES_F0 = 1125;
		LINES_F1 = 1125;
		FRONT_PORCH = 88;
		HSYNC_PIXELS = 44;
		BACK_PORCH = 148;
		EOF_LINES = 4;
		VSYNC_LINES = 5;
		SOF_LINES = 36;
		TOTAL_FRAMES = 4;
		break;
	case HDMI_89_2560x1080p50_64x27:
	case HDMI_90_2560x1080p60_64x27:
		INTERLACE_MODE	= 0U;
		PIXEL_REPEAT_VENC  = 0;
		PIXEL_REPEAT_HDMI  = 0;
		ACTIVE_PIXELS = hdmi_encp_timing->h_active;
		ACTIVE_LINES = hdmi_encp_timing->v_active;
		LINES_F0 = hdmi_encp_timing->v_total;
		LINES_F1 = hdmi_encp_timing->v_total;
		FRONT_PORCH = hdmi_encp_timing->h_front;
		HSYNC_PIXELS = hdmi_encp_timing->h_sync;
		BACK_PORCH = hdmi_encp_timing->h_back;
		EOF_LINES = hdmi_encp_timing->v_front;
		VSYNC_LINES = hdmi_encp_timing->v_sync;
		SOF_LINES = hdmi_encp_timing->v_back;
		TOTAL_FRAMES = 4;
		break;
	default:
		break;
	}

	TOTAL_PIXELS = FRONT_PORCH + HSYNC_PIXELS + BACK_PORCH + ACTIVE_PIXELS;
	TOTAL_LINES = (LINES_F0 + LINES_F1 * INTERLACE_MODE);

	total_pixels_venc = (TOTAL_PIXELS  / (1 + PIXEL_REPEAT_HDMI)) *
		(1 + PIXEL_REPEAT_VENC);
	active_pixels_venc = (ACTIVE_PIXELS / (1 + PIXEL_REPEAT_HDMI)) *
		(1 + PIXEL_REPEAT_VENC);
	front_porch_venc = (FRONT_PORCH   / (1 + PIXEL_REPEAT_HDMI)) *
		(1 + PIXEL_REPEAT_VENC);
	hsync_pixels_venc = (HSYNC_PIXELS  / (1 + PIXEL_REPEAT_HDMI)) *
		(1 + PIXEL_REPEAT_VENC);

	hd_write_reg(P_ENCP_VIDEO_MODE, hd_read_reg(P_ENCP_VIDEO_MODE)
		| (1 << 14));
	/* Program DE timing */
	de_h_begin = modulo(hd_read_reg(P_ENCP_VIDEO_HAVON_BEGIN) +
		VFIFO2VD_TO_HDMI_LATENCY,  total_pixels_venc);
	de_h_end  = modulo(de_h_begin + active_pixels_venc, total_pixels_venc);
	hd_write_reg(P_ENCP_DE_H_BEGIN, de_h_begin);	/* 220 */
	hd_write_reg(P_ENCP_DE_H_END, de_h_end);	 /* 1660 */
	/* Program DE timing for even field */
	de_v_begin_even = hd_read_reg(P_ENCP_VIDEO_VAVON_BLINE);
	de_v_end_even  = de_v_begin_even + ACTIVE_LINES;
	hd_write_reg(P_ENCP_DE_V_BEGIN_EVEN, de_v_begin_even);
	hd_write_reg(P_ENCP_DE_V_END_EVEN,  de_v_end_even);	/* 522 */

	/* Program Hsync timing */
	if (de_h_end + front_porch_venc >= total_pixels_venc) {
		hs_begin = de_h_end + front_porch_venc - total_pixels_venc;
		vs_adjust  = 1;
	} else {
		hs_begin = de_h_end + front_porch_venc;
		vs_adjust  = 0;
	}
	hs_end = modulo(hs_begin + hsync_pixels_venc, total_pixels_venc);
	hd_write_reg(P_ENCP_DVI_HSO_BEGIN,  hs_begin);
	hd_write_reg(P_ENCP_DVI_HSO_END, hs_end);

	/* Program Vsync timing for even field */
	if (de_v_begin_even >= SOF_LINES + VSYNC_LINES + (1 - vs_adjust))
		vs_bline_evn = de_v_begin_even - SOF_LINES - VSYNC_LINES -
			(1 - vs_adjust);
	else
		vs_bline_evn = TOTAL_LINES + de_v_begin_even - SOF_LINES -
			VSYNC_LINES - (1 - vs_adjust);
	vs_eline_evn = modulo(vs_bline_evn + VSYNC_LINES, TOTAL_LINES);
	hd_write_reg(P_ENCP_DVI_VSO_BLINE_EVN, vs_bline_evn);   /* 5 */
	hd_write_reg(P_ENCP_DVI_VSO_ELINE_EVN, vs_eline_evn);   /* 11 */
	vso_begin_evn = hs_begin; /* 1692 */
	hd_write_reg(P_ENCP_DVI_VSO_BEGIN_EVN, vso_begin_evn);  /* 1692 */
	hd_write_reg(P_ENCP_DVI_VSO_END_EVN, vso_begin_evn);  /* 1692 */

	if (param->vic == HDMI_111_1920x1080p48_16x9 ||
	    param->vic == HDMI_112_1920x1080p48_64x27)
		hd_write_reg(P_ENCP_DE_V_END_EVEN, 0x230);
	switch (param->vic) {
	case HDMI_109_1280x720p48_64x27:
	case HDMI_110_1680x720p48_64x27:
	case HDMI_111_1920x1080p48_16x9:
	case HDMI_112_1920x1080p48_64x27:
		hd_write_reg(P_VPU_HDMI_SETTING, 0x8c);
		break;
	case HDMI_6_720x480i60_4x3:
	case HDMI_7_720x480i60_16x9:
	case HDMI_21_720x576i50_4x3:
	case HDMI_22_720x576i50_16x9:
	case HDMI_11_2880x480i60_16x9:
	case HDMI_26_2880x576i50_16x9:
		hd_write_reg(P_VPU_HDMI_SETTING, (0 << 0) |
				(0 << 1) |
				(0 << 2) |
				(0 << 3) |
				(0 << 4) |
				(4 << 5) |
				(1 << 8) |
				(1 << 12)
		);
		if (param->vic == HDMI_11_2880x480i60_16x9 ||
		    param->vic == HDMI_26_2880x576i50_16x9)
			hd_set_reg_bits(P_VPU_HDMI_SETTING, 3, 12, 4);
		break;
	case HDMI_5_1920x1080i60_16x9:
	case HDMI_20_1920x1080i50_16x9:
		hd_write_reg(P_VPU_HDMI_SETTING, (0 << 0) |
				(0 << 1) |
				(HSYNC_POLARITY << 2) |
				(VSYNC_POLARITY << 3) |
				(0 << 4) |
				(0 << 5) |
				(0 << 8) |
				(0 << 12)
		);
		break;
	case HDMI_95_3840x2160p30_16x9:
	case HDMI_94_3840x2160p25_16x9:
	case HDMI_93_3840x2160p24_16x9:
	case HDMI_98_4096x2160p24_256x135:
	case HDMI_99_4096x2160p25_256x135:
	case HDMI_100_4096x2160p30_256x135:
	case HDMI_101_4096x2160p50_256x135:
	case HDMI_102_4096x2160p60_256x135:
	case HDMI_96_3840x2160p50_16x9:
	case HDMI_97_3840x2160p60_16x9:
		hd_write_reg(P_VPU_HDMI_SETTING, (0 << 0) |
			(0 << 1) |
			(HSYNC_POLARITY << 2) |
			(VSYNC_POLARITY << 3) |
			(0 << 4) |
			(4 << 5) |
			(0 << 8) |
			(0 << 12)
		);
		break;
	case HDMI_36_2880x480p60_16x9:
	case HDMI_38_2880x576p50_16x9:
	case HDMI_2_720x480p60_4x3:
	case HDMI_3_720x480p60_16x9:
	case HDMI_17_720x576p50_4x3:
	case HDMI_18_720x576p50_16x9:
		hd_write_reg(P_VPU_HDMI_SETTING, (0 << 0) |
				(0 << 1) |
				(0 << 2) |
				(0 << 3) |
				(0 << 4) |
				(4 << 5) |
				(0 << 8) |
				(0 << 12)
		);
		if (param->vic == HDMI_36_2880x480p60_16x9 ||
		    param->vic == HDMI_38_2880x576p50_16x9)
			hd_set_reg_bits(P_VPU_HDMI_SETTING, 3, 12, 4);
		break;
	case HDMI_4_1280x720p60_16x9:
	case HDMI_19_1280x720p50_16x9:
		hd_write_reg(P_VPU_HDMI_SETTING, (0 << 0) |
				(0 << 1) |
				(HSYNC_POLARITY << 2) |
				(VSYNC_POLARITY << 3) |
				(0 << 4) |
				(4 << 5) |
				(0 << 8) |
				(0 << 12)
		);
		break;
	default:
		hd_write_reg(P_VPU_HDMI_SETTING, (0 << 0) |
				(0 << 1) | /* [	1] src_sel_encp */
				(HSYNC_POLARITY << 2) |
				(VSYNC_POLARITY << 3) |
				(0 << 4) |
				(4 << 5) |
				(0 << 8) |
				(0 << 12)
		);
	}
	if (param->vic == HDMI_36_2880x480p60_16x9 ||
	    param->vic == HDMI_38_2880x576p50_16x9)
		hd_set_reg_bits(P_VPU_HDMI_SETTING, 3, 12, 4);
}

static void phy_pll_off(void)
{
	hdmi_phy_suspend();
}

/************************************
 *	hdmitx hardware level interface
 *************************************/
static void hdmitx_set_pll(struct hdmitx_dev *hdev)
{
	hdmitx_set_clk(hdev);
}

static void set_phy_by_mode(unsigned int mode)
{
	switch (global_tx_hw->chip_data->chip_type) {
	case MESON_CPU_ID_G12A:
	case MESON_CPU_ID_G12B:
		set_phy_by_mode_g12(mode);
		break;
	case MESON_CPU_ID_SM1:
		set_phy_by_mode_sm1(mode);
		break;
	case MESON_CPU_ID_SC2:
		set_phy_by_mode_sc2(mode);
		break;
	case MESON_CPU_ID_TM2:
	case MESON_CPU_ID_TM2B:
		set_phy_by_mode_tm2(mode);
		break;
#ifndef CONFIG_AMLOGIC_REMOVE_OLD
	case MESON_CPU_ID_GXBB:
	case MESON_CPU_ID_GXTVBB:
		set_phy_by_mode_gxbb(mode);
		break;
	case MESON_CPU_ID_GXL:
	case MESON_CPU_ID_GXM:
	case MESON_CPU_ID_TXL:
	case MESON_CPU_ID_TXLX:
	case MESON_CPU_ID_AXG:
	case MESON_CPU_ID_GXLX:
	case MESON_CPU_ID_TXHD:
		set_phy_by_mode_gxl(mode);
		break;
#endif
	default:
		break;
	}
}

static void hdmitx_set_phy(struct hdmitx_dev *hdev)
{
	unsigned int phy_addr = 0;
	struct hdmi_format_para *para;
	int chip_id;

	if (!hdev)
		return;

	para = &hdev->tx_comm.fmt_para;
	chip_id = hdev->tx_hw.chip_data->chip_type;

	if (chip_id >= MESON_CPU_ID_SC2)
		phy_addr = P_ANACTRL_HDMIPHY_CTRL0;
	else if (chip_id == MESON_CPU_ID_TM2 || chip_id == MESON_CPU_ID_TM2B)
		phy_addr = P_TM2_HHI_HDMI_PHY_CNTL0;
	else
		phy_addr = P_HHI_HDMI_PHY_CNTL0;
	hd_write_reg(phy_addr, 0x0);

	if (chip_id == MESON_CPU_ID_TM2 || chip_id == MESON_CPU_ID_TM2B)
		phy_addr = P_TM2_HHI_HDMI_PHY_CNTL1;
	else if (chip_id == MESON_CPU_ID_SC2)
		phy_addr = P_ANACTRL_HDMIPHY_CTRL1;
	else
		phy_addr = P_HHI_HDMI_PHY_CNTL1;

/* P_HHI_HDMI_PHY_CNTL1 bit[1]: enable clock	bit[0]: soft reset */
#define RESET_HDMI_PHY() \
do { \
	hd_set_reg_bits(phy_addr, 0xf, 0, 4); \
	mdelay(2); \
	hd_set_reg_bits(phy_addr, 0xe, 0, 4); \
	mdelay(2); \
} while (0)

	hd_set_reg_bits(phy_addr, 0x0390, 16, 16);
	hd_set_reg_bits(phy_addr, 0x1, 17, 1);
	if (chip_id >= MESON_CPU_ID_GXL)
		hd_set_reg_bits(phy_addr, 0x0, 17, 1);
	hd_set_reg_bits(phy_addr, 0x0, 0, 4);
	msleep(20);
	RESET_HDMI_PHY();
	RESET_HDMI_PHY();
	RESET_HDMI_PHY();
#undef RESET_HDMI_PHY

	switch (hdev->tx_comm.fmt_para.vic) {
	case HDMI_96_3840x2160p50_16x9:
	case HDMI_97_3840x2160p60_16x9:
	case HDMI_101_4096x2160p50_256x135:
	case HDMI_102_4096x2160p60_256x135:
		if (para->cs != HDMI_COLORSPACE_YUV420)
			set_phy_by_mode(HDMI_PHYPARA_6G);
		else
			if (para->cd == COLORDEPTH_36B)
				set_phy_by_mode(HDMI_PHYPARA_4p5G);
			else if (para->cd == COLORDEPTH_30B)
				set_phy_by_mode(HDMI_PHYPARA_3p7G);
			else
				set_phy_by_mode(HDMI_PHYPARA_3G);
		break;
	case HDMI_93_3840x2160p24_16x9:
	case HDMI_94_3840x2160p25_16x9:
	case HDMI_95_3840x2160p30_16x9:
	case HDMI_63_1920x1080p120_16x9:
	case HDMI_78_1920x1080p120_64x27:
	case HDMI_98_4096x2160p24_256x135:
	case HDMI_99_4096x2160p25_256x135:
	case HDMI_100_4096x2160p30_256x135:
		if (para->cs == HDMI_COLORSPACE_YUV422 ||
		    para->cd == COLORDEPTH_24B)
			set_phy_by_mode(HDMI_PHYPARA_3G);
		else
			if (para->cd == COLORDEPTH_36B)
				set_phy_by_mode(HDMI_PHYPARA_4p5G);
			else if (para->cd == COLORDEPTH_30B)
				set_phy_by_mode(HDMI_PHYPARA_3p7G);
			else
				set_phy_by_mode(HDMI_PHYPARA_3G);
		break;
	case HDMI_3_720x480p60_16x9:
	case HDMI_18_720x576p50_16x9:
	case HDMI_7_720x480i60_16x9:
	case HDMI_22_720x576i50_16x9:
		set_phy_by_mode(HDMI_PHYPARA_270M);
		break;
	case HDMI_16_1920x1080p60_16x9:
	case HDMI_31_1920x1080p50_16x9:
	default:
		set_phy_by_mode(HDMI_PHYPARA_DEF);
		break;
	}
}

static void set_tmds_clk_div40(unsigned int div40)
{
	HDMITX_INFO("div40: %d\n", div40);
	if (div40) {
		hdmitx_wr_reg(HDMITX_TOP_TMDS_CLK_PTTN_01, 0);
		hdmitx_wr_reg(HDMITX_TOP_TMDS_CLK_PTTN_23, 0x03ff03ff);
	} else {
		hdmitx_wr_reg(HDMITX_TOP_TMDS_CLK_PTTN_01, 0x001f001f);
		hdmitx_wr_reg(HDMITX_TOP_TMDS_CLK_PTTN_23, 0x001f001f);
	}
	hdmitx_set_reg_bits(HDMITX_DWC_FC_SCRAMBLER_CTRL, !!div40, 0, 1);
	hdmitx_wr_reg(HDMITX_TOP_TMDS_CLK_PTTN_CNTL, 0x1);
	msleep(20);
	hdmitx_wr_reg(HDMITX_TOP_TMDS_CLK_PTTN_CNTL, 0x2);
}

static void hdmitx_set_scdc(struct hdmitx_dev *hdev)
{
	struct hdmi_format_para *para = &hdev->tx_comm.fmt_para;
	u8 pref_clk_div40 = 0;

	switch (para->vic) {
	case HDMI_96_3840x2160p50_16x9:
	case HDMI_97_3840x2160p60_16x9:
	case HDMI_101_4096x2160p50_256x135:
	case HDMI_102_4096x2160p60_256x135:
		if (para->cs == HDMI_COLORSPACE_YUV420 &&
		    para->cd == COLORDEPTH_24B)
			pref_clk_div40 = 0;
		else
			pref_clk_div40 = 1;
		break;
	case HDMI_93_3840x2160p24_16x9:
	case HDMI_103_3840x2160p24_64x27:
	case HDMI_98_4096x2160p24_256x135:
	case HDMI_94_3840x2160p25_16x9:
	case HDMI_104_3840x2160p25_64x27:
	case HDMI_99_4096x2160p25_256x135:
	case HDMI_95_3840x2160p30_16x9:
	case HDMI_105_3840x2160p30_64x27:
	case HDMI_100_4096x2160p30_256x135:
		if (para->cs == HDMI_COLORSPACE_YUV422 ||
		    para->cd == COLORDEPTH_24B)
			pref_clk_div40 = 0;
		else
			pref_clk_div40 = 1;
		break;
	case HDMIV_2560x1600p60hz:
		pref_clk_div40 = 0;
		break;
	default:
		pref_clk_div40 = 0;
		break;
	}

	if (pref_clk_div40 != para->tmds_clk_div40) {
		HDMITX_ERROR("clk div40 failed!!\n");
		hdmitx_format_para_print(para, NULL);
	}

	set_tmds_clk_div40(para->tmds_clk_div40);
	scdc_config(hdev);
	hdev->pre_tmds_clk_div40 = para->tmds_clk_div40;
}

void hdmitx_set_enc_hw(struct hdmitx_dev *hdev)
{
	unsigned int data32 = 0;
	struct hdmi_format_para *para = &hdev->tx_comm.fmt_para;

	set_vmode_enc_hw(hdev);

	if (hdev->flag_3dfp) {
		hd_write_reg(P_VPU_HDMI_SETTING, 0x8c);
	} else {
		switch (para->vic) {
		case HDMI_6_720x480i60_4x3:
		case HDMI_7_720x480i60_16x9:
		case HDMI_21_720x576i50_4x3:
		case HDMI_22_720x576i50_16x9:
		case HDMI_11_2880x480i60_16x9:
		case HDMI_26_2880x576i50_16x9:
			hdmi_tvenc480i_set(para);
			break;
		case HDMI_5_1920x1080i60_16x9:
		case HDMI_20_1920x1080i50_16x9:
			hdmi_tvenc1080i_set(para);
			break;
		case HDMI_95_3840x2160p30_16x9:
		case HDMI_94_3840x2160p25_16x9:
		case HDMI_93_3840x2160p24_16x9:
		case HDMI_98_4096x2160p24_256x135:
		case HDMI_99_4096x2160p25_256x135:
		case HDMI_100_4096x2160p30_256x135:
		case HDMI_101_4096x2160p50_256x135:
		case HDMI_102_4096x2160p60_256x135:
		case HDMI_96_3840x2160p50_16x9:
		case HDMI_97_3840x2160p60_16x9:
			hdmi_tvenc4k2k_set(para);
			break;
		default:
			hdmi_tvenc_set(para);
		}
	}
	/* [ 3: 2] chroma_dnsmp. 0=use pixel 0; 1=use pixel 1; 2=use average. */
	/* [	5] hdmi_dith_md: random noise selector. */
	hd_write_reg(P_VPU_HDMI_FMT_CTRL, (((TX_INPUT_COLOR_FORMAT ==
			HDMI_COLORSPACE_YUV420) ? 2 : 0)  << 0) | (2 << 2) |
				(0 << 4) | /* [4]dith_en: disable dithering */
				(0	<< 5) |
				(0 << 6)); /* [ 9: 6] hdmi_dith10_cntl. */
	if (para->cs == HDMI_COLORSPACE_YUV420) {
		hd_set_reg_bits(P_VPU_HDMI_FMT_CTRL, 2, 0, 2);
		hd_set_reg_bits(P_VPU_HDMI_SETTING, 0, 4, 4);
		hd_set_reg_bits(P_VPU_HDMI_SETTING, 1, 8, 1);
		if ((is_hdmi4k_support_420(hdev->tx_comm.fmt_para.vic)) &&
		    hdev->tx_hw.chip_data->chip_type >= MESON_CPU_ID_TM2B)
			hd_set_reg_bits(P_VPU_HDMI_SETTING, 0, 8, 1);
	}

	if (para->cs == HDMI_COLORSPACE_YUV422) {
		hd_set_reg_bits(P_VPU_HDMI_FMT_CTRL, 1, 0, 2);
		hd_set_reg_bits(P_VPU_HDMI_SETTING, 0, 4, 4);
	}

	switch (para->vic) {
	case HDMI_6_720x480i60_4x3:
	case HDMI_7_720x480i60_16x9:
	case HDMI_21_720x576i50_4x3:
	case HDMI_22_720x576i50_16x9:
	case HDMI_11_2880x480i60_16x9:
	case HDMI_26_2880x576i50_16x9:
		data32 = (0 << 0) | /* 2b01: ENCI  2b10: ENCP */
			 (0 << 2) | /* INV_HSYNC */
			 (0 << 3) | /* INV_VSYNC */
			 (4 << 5) | /* 0 CrYCb/BGR 1 YCbCr/RGB 2 YCrCb/RBG.. */
			 (1 << 8) | /* WR_RATE */
			 (1 << 12); /* RD_RATE */
		hd_write_reg(P_VPU_HDMI_SETTING, data32);
		break;
	default:
		break;
	}

	switch (para->cd) {
	case COLORDEPTH_30B:
	case COLORDEPTH_36B:
	case COLORDEPTH_48B:
		if (hdev->tx_hw.chip_data->chip_type >= MESON_CPU_ID_GXM) {
			unsigned int hs_flag = 0;
			/* 12-10 dithering on */
			hd_set_reg_bits(P_VPU_HDMI_FMT_CTRL, 0, 4, 1);
			/* hsync/vsync not invert */
			hs_flag = (hd_read_reg(P_VPU_HDMI_SETTING) >> 2) & 0x3;
			hd_set_reg_bits(P_VPU_HDMI_SETTING, 0, 2, 2);
			/* 12-10 rounding off */
			hd_set_reg_bits(P_VPU_HDMI_FMT_CTRL, 0, 10, 1);
			/* 10-8 dithering off (2x2 old dither) */
			hd_set_reg_bits(P_VPU_HDMI_DITH_CNTL, 0, 4, 1);
			/* set hsync/vsync */
			hd_set_reg_bits(P_VPU_HDMI_DITH_CNTL, hs_flag, 2, 2);
		} else {
			hd_set_reg_bits(P_VPU_HDMI_FMT_CTRL, 0, 4, 1);
			hd_set_reg_bits(P_VPU_HDMI_FMT_CTRL, 0, 10, 1);
		}
	break;
	default:
		if (hdev->tx_hw.chip_data->chip_type >= MESON_CPU_ID_GXM) {
			/* 12-10 dithering off */
			hd_set_reg_bits(P_VPU_HDMI_FMT_CTRL, 0, 4, 1);
			/* 12-10 rounding on */
			hd_set_reg_bits(P_VPU_HDMI_FMT_CTRL, 1, 10, 1);
			/* 10-8 dithering on (2x2 old dither) */
			hd_set_reg_bits(P_VPU_HDMI_DITH_CNTL, 1, 4, 1);
			/* set hsync/vsync as default 0 */
			hd_set_reg_bits(P_VPU_HDMI_DITH_CNTL, 0, 2, 2);
		} else {
			hd_set_reg_bits(P_VPU_HDMI_FMT_CTRL, 0, 4, 1);
			hd_set_reg_bits(P_VPU_HDMI_FMT_CTRL, 1, 10, 1);
		}
	break;
	}
}

static void hdmitx_disable_venc(void)
{
	hd_write_reg(P_ENCI_VIDEO_EN, 0);
	hd_write_reg(P_ENCP_VIDEO_EN, 0);
	usleep_range(1000, 1005);
	hd_set_reg_bits(P_VPU_HDMI_SETTING, 0, 0, 2);
}

/* switch mode flow:
 * HDMITX PHY disable-> disable VENC-> vpu decouple disable->
 * HDMI HPLL setting-> config VENC-> IP configure & reset->
 * vpu decouple FIFO-> enable ->enable VENC-> HDMITX PHY enable
 */
static int hdmitx_set_dispmode(struct hdmitx_hw_common *tx_hw)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();
	struct hdmi_format_para *para;

	if (!hdev) /* disable HDMI */
		return 0;
	para = &hdev->tx_comm.fmt_para;

	hdmitx_disable_venc();

	hdmitx_set_scdc(hdev);

	hdmitx_set_pll(hdev);

	hdmitx_set_enc_hw(hdev);

	hdmitx_set_hw(hdev);
	switch (para->vic) {
	case HDMI_6_720x480i60_4x3:
	case HDMI_7_720x480i60_16x9:
	case HDMI_21_720x576i50_4x3:
	case HDMI_22_720x576i50_16x9:
	case HDMI_11_2880x480i60_16x9:
	case HDMI_26_2880x576i50_16x9:
		enc_vpu_bridge_reset(0);
		break;
	default:
		enc_vpu_bridge_reset(1);
		break;
	}
	HDMITX_DEBUG("adjust decouple fifo\n");
	/* For 3D, enable phy by SystemControl at last step */
	if (!hdev->flag_3dfp && !hdev->flag_3dtb && !hdev->flag_3dss)
		hdmitx_set_phy(hdev);
	return 0;
}

static enum hdmi_tf_type hdmitx_get_cur_hdr_st(void)
{
	enum hdmi_tf_type type = HDMI_NONE;
	unsigned int val = 0;

	if (!hdmitx_get_bit(HDMITX_DWC_FC_DATAUTO3, 6) ||
	    !hdmitx_get_bit(HDMITX_DWC_FC_PACKET_TX_EN, 7))
		return type;

	val = hdmitx_rd_reg(HDMITX_DWC_FC_DRM_PB00);
	switch (val) {
	case 0:
		type = HDMI_HDR_SDR;
		break;
	case 1:
		type = HDMI_HDR_HDR;
		break;
	case 2:
		type = HDMI_HDR_SMPTE_2084;
		break;
	case 3:
		type = HDMI_HDR_HLG;
		break;
	default:
		type = HDMI_HDR_TYPE;
		break;
	};

	return type;
}

static bool hdmitx_vsif_en(void)
{
	if (!hdmitx_get_bit(HDMITX_DWC_FC_DATAUTO0, 3) ||
	    !hdmitx_get_bit(HDMITX_DWC_FC_PACKET_TX_EN, 4))
		return 0;
	else
		return 1;
}

#define GET_IEEEOUI() \
	(hdmitx_rd_reg(HDMITX_DWC_FC_VSDIEEEID0) | \
	hdmitx_rd_reg(HDMITX_DWC_FC_VSDIEEEID1) << 8 | \
	hdmitx_rd_reg(HDMITX_DWC_FC_VSDIEEEID2) << 16)

static enum hdmi_tf_type hdmitx_get_cur_dv_st(void)
{
	enum hdmi_tf_type type = HDMI_NONE;
	unsigned int ieee_code = 0;
	unsigned int size = hdmitx_rd_reg(HDMITX_DWC_FC_VSDSIZE);
	unsigned int cs = hdmitx_rd_reg(HDMITX_DWC_FC_AVICONF0) & 0x3;
	unsigned int amdv_signal = hdmitx_rd_reg(HDMITX_DWC_FC_VSDPAYLOAD0) & 0x3;

	if (!hdmitx_vsif_en())
		return type;

	ieee_code = GET_IEEEOUI();

	if ((ieee_code == HDMI_IEEE_OUI && size == 0x18) ||
	    (ieee_code == DOVI_IEEEOUI && size == 0x1b)) {
		/* When outputting DV_LL, cs needs to be 422,
		 * Dolby_Vision_Signal (bit1) is 1,
		 * and Low_Latency (bit0) is 1
		 */
		if (cs == 0x1 && amdv_signal == 0x3)
			type = HDMI_DV_VSIF_LL;
		/* When outputting DV_STD, cs needs to be rgb,
		 * Dolby_Vision_Signal (bit1) is 1,
		 * and Low_Latency (bit0) is 0
		 */
		if (cs == 0x0 && amdv_signal == 0x2)
			type = HDMI_DV_VSIF_STD;
	}
	return type;
}

static enum hdmi_tf_type hdmitx_get_cur_hdr10p_st(void)
{
	enum hdmi_tf_type type = HDMI_NONE;
	unsigned int ieee_code = 0;

	if (!hdmitx_vsif_en())
		return type;

	ieee_code = GET_IEEEOUI();

	if (ieee_code == HDR10PLUS_IEEEOUI)
		type = HDMI_HDR10P_DV_VSIF;

	return type;
}

static void hdmitx_set_packet(int type, unsigned char *DB, unsigned char *HB)
{
	int i;
	int pkt_data_len = 0;
	unsigned int ieee_code = 0;

	switch (type) {
	case HDMI_PACKET_AVI:
		break;
	case HDMI_PACKET_VEND:
	case HDMI_INFOFRAME_TYPE_VENDOR:
	/* 21 allm will use vendor2 */
	case HDMI_INFOFRAME_TYPE_VENDOR2:
		if (!DB || !HB) {
			hdmitx_set_reg_bits(HDMITX_DWC_FC_DATAUTO0, 0, 3, 1);
			hdmitx_wr_reg(HDMITX_DWC_FC_VSDSIZE, 0x0);
			hdmitx_wr_reg(HDMITX_DWC_FC_VSDIEEEID0, 0x00);
			hdmitx_wr_reg(HDMITX_DWC_FC_VSDIEEEID1, 0x00);
			hdmitx_wr_reg(HDMITX_DWC_FC_VSDIEEEID2, 0x00);
			for (i = 0; i < 24; i++)
				hdmitx_wr_reg(HDMITX_DWC_FC_VSDPAYLOAD0 + i, 0x00);
			return;
		}
		hdmitx_wr_reg(HDMITX_DWC_FC_VSDIEEEID0, DB[0]);
		hdmitx_wr_reg(HDMITX_DWC_FC_VSDIEEEID1, DB[1]);
		hdmitx_wr_reg(HDMITX_DWC_FC_VSDIEEEID2, DB[2]);
		hdmitx_wr_reg(HDMITX_DWC_FC_VSDPAYLOAD0, DB[3]);
		hdmitx_wr_reg(HDMITX_DWC_FC_VSDSIZE, HB[2]);
		ieee_code = DB[0] | DB[1] << 8 | DB[2] << 16;
		if (ieee_code == HDMI_IEEE_OUI && DB[3] == 0x20) {
			/* set HDMI VIC */
			hdmitx_wr_reg(HDMITX_DWC_FC_AVIVID, 0);
			hdmitx_wr_reg(HDMITX_DWC_FC_VSDPAYLOAD1, DB[4]);
		}
		if (ieee_code == HDMI_IEEE_OUI && DB[3] == 0x40) {
			/* 3D VSI */
			hdmitx_wr_reg(HDMITX_DWC_FC_VSDPAYLOAD1, DB[4]);
			hdmitx_wr_reg(HDMITX_DWC_FC_VSDPAYLOAD2, DB[5]);
			if ((DB[4] >> 4) == T3D_FRAME_PACKING)
				hdmitx_wr_reg(HDMITX_DWC_FC_VSDSIZE, 5);
			else
				hdmitx_wr_reg(HDMITX_DWC_FC_VSDSIZE, 6);
		}
		if (ieee_code == DOVI_IEEEOUI && HB[2] == 0x1b) {
			/*set dolby vsif data information*/
			hdmitx_wr_reg(HDMITX_DWC_FC_VSDPAYLOAD1, DB[4]);
			hdmitx_wr_reg(HDMITX_DWC_FC_VSDPAYLOAD2, DB[5]);
			hdmitx_wr_reg(HDMITX_DWC_FC_VSDPAYLOAD3, DB[6]);
			hdmitx_wr_reg(HDMITX_DWC_FC_VSDPAYLOAD4, DB[7]);
			hdmitx_wr_reg(HDMITX_DWC_FC_VSDPAYLOAD5, DB[8]);
		}
		/*set hdr 10+ vsif data information*/
		if (ieee_code == HDR10PLUS_IEEEOUI) {
			for (i = 0; i < 23; i++)
				hdmitx_wr_reg(HDMITX_DWC_FC_VSDPAYLOAD1 + i,
					DB[4 + i]);
		}
		/* allm */
		if (ieee_code == HDMI_FORUM_IEEE_OUI)
			hdmitx_wr_reg(HDMITX_DWC_FC_VSDPAYLOAD1, DB[4]);

		/* Enable VSI packet */
		hdmitx_set_reg_bits(HDMITX_DWC_FC_DATAUTO0, 1, 3, 1);
		hdmitx_wr_reg(HDMITX_DWC_FC_DATAUTO1, 0);
		hdmitx_wr_reg(HDMITX_DWC_FC_DATAUTO2, 0x10);
		hdmitx_set_reg_bits(HDMITX_DWC_FC_PACKET_TX_EN, 1, 4, 1);
		break;
	case HDMI_PACKET_DRM:
		pkt_data_len = 26;
		if (!DB || !HB) {
			hdmitx_set_reg_bits(HDMITX_DWC_FC_DATAUTO3, 0, 6, 1);
			hdmitx_set_reg_bits(HDMITX_DWC_FC_PACKET_TX_EN,
					    0, 7, 1);
			return;
		}
		/* Ignore HB[0] */
		hdmitx_wr_reg(HDMITX_DWC_FC_DRM_HB01, HB[1]);
		hdmitx_wr_reg(HDMITX_DWC_FC_DRM_HB02, HB[2]);
		for (i = 0; i < pkt_data_len; i++)
			hdmitx_wr_reg(HDMITX_DWC_FC_DRM_PB00 + i, DB[i]);
		hdmitx_set_reg_bits(HDMITX_DWC_FC_DATAUTO3, 1, 6, 1);
		hdmitx_set_reg_bits(HDMITX_DWC_FC_PACKET_TX_EN, 1, 7, 1);
		break;
	case HDMI_AUDIO_INFO:
		pkt_data_len = 9;
		break;
	case HDMI_SOURCE_DESCRIPTION:
		pkt_data_len = 25;
		for (i = 0; i < 25; i++)
			hdmitx_wr_reg(HDMITX_DWC_FC_SPDVENDORNAME0 + i, DB[i]);
		hdmitx_set_reg_bits(HDMITX_DWC_FC_DATAUTO0, 1, 4, 1);
		hdmitx_set_reg_bits(HDMITX_DWC_FC_DATAUTO2, 0x1, 4, 4);
		hdmitx_set_reg_bits(HDMITX_DWC_FC_PACKET_TX_EN, 1, 4, 1);
		break;
	default:
		break;
	}
}

static void hdmitx_disable_packet(int type)
{
	switch (type) {
	case HDMI_PACKET_AVI:
		break;
	case HDMI_PACKET_VEND:
		/* disable VSI packet */
		hdmitx_set_reg_bits(HDMITX_DWC_FC_DATAUTO0, 0, 3, 1);
		hdmitx_wr_reg(HDMITX_DWC_FC_DATAUTO1, 0);
		hdmitx_wr_reg(HDMITX_DWC_FC_DATAUTO2, 0x10);
		hdmitx_set_reg_bits(HDMITX_DWC_FC_PACKET_TX_EN, 1, 4, 1);
		break;
	case HDMI_PACKET_DRM:
		break;
	case HDMI_AUDIO_INFO:
		break;
	case HDMI_SOURCE_DESCRIPTION:
		break;
	default:
		break;
	}
}

static void hdmitx_setaudioinfoframe(unsigned char *AUD_DB,
				     unsigned char *CHAN_STAT_BUF)
{
}

/* 60958-3 bit 27-24 */
static unsigned char aud_csb_sampfreq[FS_MAX + 1] = {
	[FS_REFER_TO_STREAM] = 0x1, /* not indicated */
	[FS_32K] = 0x3, /* FS_32K */
	[FS_44K1] = 0x0, /* FS_44K1 */
	[FS_48K] = 0x2, /* FS_48K */
	[FS_88K2] = 0x8, /* FS_88K2 */
	[FS_96K] = 0xa, /* FS_96K */
	[FS_176K4] = 0xc, /* FS_176K4 */
	[FS_192K] = 0xe, /* FS_192K */
	[FS_768K] = 0x9, /* FS_768K */
};

/* 60958-3 bit 39:36 */
static unsigned char aud_csb_ori_sampfreq[FS_MAX + 1] = {
	[FS_REFER_TO_STREAM] = 0x0, /* not indicated */
	[FS_32K] = 0xc, /* FS_32K */
	[FS_44K1] = 0xf, /* FS_44K1 */
	[FS_48K] = 0xd, /* FS_48K */
	[FS_88K2] = 0x7, /* FS_88K2 */
	[FS_96K] = 0xa, /* FS_96K */
	[FS_176K4] = 0x3, /* FS_176K4 */
	[FS_192K] = 0x1, /* FS_192K */
};

static void set_aud_chnls(struct aud_para *audio_param)
{
	int i;

	pr_debug(HW "set channel status\n");
	for (i = 0; i < 9; i++)
		/* First, set all status to 0 */
		hdmitx_wr_reg(HDMITX_DWC_FC_AUDSCHNLS0 + i, 0x00);
	/* set default 48k 2ch pcm */
	if (audio_param->type == CT_PCM &&
	    (audio_param->chs == (2 - 1))) {
		hdmitx_wr_reg(HDMITX_DWC_FC_AUDSV, 0);
		hdmitx_wr_reg(HDMITX_DWC_FC_AUDSCHNLS7, 0x02);
		hdmitx_wr_reg(HDMITX_DWC_FC_AUDSCHNLS8, 0xd2);
	} else {
		hdmitx_wr_reg(HDMITX_DWC_FC_AUDSV, 0);
	}
	switch (audio_param->type) {
	case CT_AC_3:
	case CT_DD_P:
	case CT_DST:
		hdmitx_wr_reg(HDMITX_DWC_FC_AUDSCHNLS3, 0x01); /* CSB 20 */
		hdmitx_wr_reg(HDMITX_DWC_FC_AUDSCHNLS5, 0x02); /* CSB 21 */
		break;
	default:
		hdmitx_wr_reg(HDMITX_DWC_FC_AUDSCHNLS3, 0x42);
		hdmitx_wr_reg(HDMITX_DWC_FC_AUDSCHNLS4, 0x86);
		hdmitx_wr_reg(HDMITX_DWC_FC_AUDSCHNLS5, 0x31);
		hdmitx_wr_reg(HDMITX_DWC_FC_AUDSCHNLS6, 0x75);
		break;
	}
	hdmitx_set_reg_bits(HDMITX_DWC_FC_AUDSCHNLS7,
		aud_csb_sampfreq[audio_param->rate], 0, 4); /*CSB 27:24*/
	hdmitx_set_reg_bits(HDMITX_DWC_FC_AUDSCHNLS7, 0x0, 6, 2); /*CSB 31:30*/
	hdmitx_set_reg_bits(HDMITX_DWC_FC_AUDSCHNLS7, 0x0, 4, 2); /*CSB 29:28*/
	hdmitx_set_reg_bits(HDMITX_DWC_FC_AUDSCHNLS8, 0x2, 0, 4); /*CSB 35:32*/
	hdmitx_set_reg_bits(HDMITX_DWC_FC_AUDSCHNLS8,  /* CSB 39:36 */
		aud_csb_ori_sampfreq[audio_param->rate], 4, 4);
}

#define GET_OUTCHN_NO(a)	(((a) >> 4) & 0xf)
#define GET_OUTCHN_MSK(a)	((a) & 0xf)

static void set_aud_info_pkt(struct aud_para *audio_param)
{
	u8 aud_output_i2s_ch;

	if (!audio_param)
		return;
	aud_output_i2s_ch = audio_param->aud_output_i2s_ch;
	hdmitx_set_reg_bits(HDMITX_DWC_FC_AUDICONF0, 0, 0, 4); /* CT */
	hdmitx_set_reg_bits(HDMITX_DWC_FC_AUDICONF0, audio_param->chs, 4, 3); /* CC */
	if (GET_OUTCHN_NO(aud_output_i2s_ch))
		hdmitx_set_reg_bits(HDMITX_DWC_FC_AUDICONF0,
			GET_OUTCHN_NO(aud_output_i2s_ch) - 1, 4, 3);
	hdmitx_set_reg_bits(HDMITX_DWC_FC_AUDICONF1, 0, 0, 3); /* SF */
	hdmitx_set_reg_bits(HDMITX_DWC_FC_AUDICONF1, 0, 4, 2); /* SS */
	switch (audio_param->type) {
	case CT_MAT:
	case CT_DTS_HD_MA:
		/* CC: 8ch */
		hdmitx_set_reg_bits(HDMITX_DWC_FC_AUDICONF0, 7, 4, 3);
		hdmitx_wr_reg(HDMITX_DWC_FC_AUDICONF2, 0x13);
		break;
	case CT_PCM:
		if (!aud_output_i2s_ch)
			hdmitx_set_reg_bits(HDMITX_DWC_FC_AUDICONF0, audio_param->chs, 4, 3);
		if (audio_param->chs == 0x7 && !aud_output_i2s_ch)
			hdmitx_wr_reg(HDMITX_DWC_FC_AUDICONF2, 0x13);
		else
			hdmitx_wr_reg(HDMITX_DWC_FC_AUDICONF2, 0x00);
		/* Refer to CEA861-D P90 */
		switch (GET_OUTCHN_NO(aud_output_i2s_ch)) {
		case 2:
			hdmitx_wr_reg(HDMITX_DWC_FC_AUDICONF2, 0x00);
			break;
		case 4:
			hdmitx_wr_reg(HDMITX_DWC_FC_AUDICONF2, 0x03);
			break;
		case 6:
			hdmitx_wr_reg(HDMITX_DWC_FC_AUDICONF2, 0x0b);
			break;
		case 8:
			hdmitx_wr_reg(HDMITX_DWC_FC_AUDICONF2, 0x13);
			break;
		default:
			break;
		}
		break;
	case CT_DTS:
	case CT_DTS_HD:
	default:
		/* CC: 2ch */
		hdmitx_set_reg_bits(HDMITX_DWC_FC_AUDICONF0, 1, 4, 3);
		hdmitx_wr_reg(HDMITX_DWC_FC_AUDICONF2, 0x0);
		break;
	}
	hdmitx_wr_reg(HDMITX_DWC_FC_AUDICONF3, 0);
}

static bool set_aud_acr_pkt(struct hdmitx_dev *hdev,
			    struct aud_para *audio_param)
{
	unsigned int data32;
	unsigned int aud_n_para;
	static unsigned int pre_aud_n_para;
	struct hdmi_format_para *para = &hdev->tx_comm.fmt_para;
	unsigned int char_rate = para->timing.pixel_freq;

	/* audio packetizer config */
	hdmitx_wr_reg(HDMITX_DWC_AUD_INPUTCLKFS, audio_param->aud_src_if ? 4 : 0);

	if (audio_param->type == CT_MAT || audio_param->type == CT_DTS_HD_MA)
		hdmitx_wr_reg(HDMITX_DWC_AUD_INPUTCLKFS, 2);

	if (para->cs == HDMI_COLORSPACE_YUV422)
		aud_n_para = hdmitx_hw_get_audio_n_paras(audio_param->rate,
						  COLORDEPTH_24B, char_rate);
	else if (para->cs == HDMI_COLORSPACE_YUV420)
		aud_n_para = hdmitx_hw_get_audio_n_paras(audio_param->rate,
						  para->cd, char_rate / 2);
	else
		aud_n_para = hdmitx_hw_get_audio_n_paras(audio_param->rate,
						  para->cd, char_rate);
	/* N must multiples 4 for DD+ */
	switch (audio_param->type) {
	case CT_DD_P:
		aud_n_para *= 4;
		break;
	default:
		break;
	}

	/* ACR packet configuration */
	data32 = 0;
	data32 |= (1 << 7);  /* [  7] ncts_atomic_write */
	data32 |= (0 << 0);  /* [3:0] AudN[19:16] */
	hdmitx_wr_reg(HDMITX_DWC_AUD_N3, data32);

	data32 = 0;
	data32 |= (0 << 7);  /* [7:5] N_shift */
	data32 |= (0 << 4);  /* [  4] CTS_manual */
	data32 |= (0 << 0);  /* [3:0] manual AudCTS[19:16] */
	hdmitx_wr_reg(HDMITX_DWC_AUD_CTS3, data32);

	hdmitx_wr_reg(HDMITX_DWC_AUD_CTS2, 0); /* manual AudCTS[15:8] */
	hdmitx_wr_reg(HDMITX_DWC_AUD_CTS1, 0); /* manual AudCTS[7:0] */

	data32 = 0;
	data32 |= (1 << 7);  /* [  7] ncts_atomic_write */
	data32 |= (((aud_n_para >> 16) & 0xf) << 0);  /* [3:0] AudN[19:16] */
	/* if only audio module update and previous n_para is same as current
	 * value, then skip update audio_n_para
	 */
	if (hdev->tx_comm.cur_audio_param.aud_notify_update && pre_aud_n_para == aud_n_para)
		return false;
	/* update audio_n_para */
	pre_aud_n_para = aud_n_para;
	hdmitx_wr_reg(HDMITX_DWC_AUD_N3, data32);
	hdmitx_wr_reg(HDMITX_DWC_AUD_N2,
		      (aud_n_para >> 8) & 0xff); /* AudN[15:8] */
	hdmitx_wr_reg(HDMITX_DWC_AUD_N1, aud_n_para & 0xff); /* AudN[7:0] */
	HDMITX_INFO("update audio N %d", aud_n_para);
	return true;
}

static void set_aud_fifo_rst(void)
{
	/* reset audio fifo */
	hdmitx_set_reg_bits(HDMITX_DWC_AUD_CONF0, 1, 7, 1);
	hdmitx_set_reg_bits(HDMITX_DWC_AUD_CONF0, 0, 7, 1);
	hdmitx_set_reg_bits(HDMITX_DWC_AUD_SPDIF0, 1, 7, 1);
	hdmitx_set_reg_bits(HDMITX_DWC_AUD_SPDIF0, 0, 7, 1);
	hdmitx_wr_reg(HDMITX_DWC_MC_SWRSTZREQ, 0xe7);
	/* need reset again */
	hdmitx_set_reg_bits(HDMITX_DWC_AUD_SPDIF0, 1, 7, 1);
	hdmitx_set_reg_bits(HDMITX_DWC_AUD_SPDIF0, 0, 7, 1);
	usleep_range(9, 11);
	hdmitx_wr_reg(HDMITX_DWC_AUD_N1, hdmitx_rd_reg(HDMITX_DWC_AUD_N1));
}

static void set_aud_samp_pkt(struct aud_para *audio_param)
{
	u8 aud_output_i2s_ch;

	if (!audio_param)
		return;
	aud_output_i2s_ch = audio_param->aud_output_i2s_ch;
	switch (audio_param->type) {
	case CT_MAT: /* HBR */
	case CT_DTS_HD_MA:
		hdmitx_set_reg_bits(HDMITX_DWC_AUD_SPDIF1, 1, 7, 1);
		hdmitx_set_reg_bits(HDMITX_DWC_AUD_SPDIF1, 1, 6, 1);
		hdmitx_set_reg_bits(HDMITX_DWC_AUD_SPDIF1, 24, 0, 5);
		hdmitx_set_reg_bits(HDMITX_DWC_FC_AUDSCONF, 1, 0, 1);
		break;
	case CT_PCM: /* AudSamp */
		hdmitx_set_reg_bits(HDMITX_DWC_AUD_SPDIF1, 0, 7, 1);
		hdmitx_set_reg_bits(HDMITX_DWC_AUD_SPDIF1, 0, 6, 1);
		hdmitx_set_reg_bits(HDMITX_DWC_AUD_SPDIF1, 24, 0, 5);
		if (audio_param->chs == 0x7 && !aud_output_i2s_ch)
			hdmitx_set_reg_bits(HDMITX_DWC_FC_AUDSCONF, 1, 0, 1);
		else
			hdmitx_set_reg_bits(HDMITX_DWC_FC_AUDSCONF, 0, 0, 1);
		switch (GET_OUTCHN_NO(aud_output_i2s_ch)) {
		case 2:
			hdmitx_set_reg_bits(HDMITX_DWC_FC_AUDSCONF, 0, 0, 1);
			break;
		case 4:
		case 6:
		case 8:
			hdmitx_set_reg_bits(HDMITX_DWC_FC_AUDSCONF, 1, 0, 1);
			break;
		default:
			break;
		}
		break;
	case CT_AC_3:
	case CT_DD_P:
	case CT_DTS:
	case CT_DTS_HD:
	default:
		hdmitx_set_reg_bits(HDMITX_DWC_AUD_SPDIF1, 0, 7, 1);
		hdmitx_set_reg_bits(HDMITX_DWC_AUD_SPDIF1, 0, 6, 1);
		hdmitx_set_reg_bits(HDMITX_DWC_AUD_SPDIF1, 24, 0, 5);
		hdmitx_set_reg_bits(HDMITX_DWC_FC_AUDSCONF, 0, 0, 1);
		break;
	}
}

static void audio_mute_op(bool flag)
{
	u8 aud_output_i2s_ch;
	struct hdmitx_dev *hdev = get_hdmitx_device();
	struct aud_para *tx_aud_param = &hdev->tx_comm.cur_audio_param;

	aud_output_i2s_ch = tx_aud_param->aud_output_i2s_ch;
	mutex_lock(&aud_mutex);
	if (flag == 0) {
		hdmitx_set_reg_bits(HDMITX_DWC_AUD_CONF0, 0x20, 0, 6);
		hdmitx_set_reg_bits(HDMITX_DWC_FC_PACKET_TX_EN, 0, 0, 1);
		hdmitx_set_reg_bits(HDMITX_DWC_FC_PACKET_TX_EN, 0, 3, 1);
	} else {
		set_aud_fifo_rst();
		hdmitx_set_reg_bits(HDMITX_DWC_FC_DATAUTO3, 1, 0, 1);
		if (GET_OUTCHN_MSK(aud_output_i2s_ch))
			hdmitx_set_reg_bits(HDMITX_DWC_AUD_CONF0,
				GET_OUTCHN_MSK(aud_output_i2s_ch), 0, 4);
		else
			hdmitx_set_reg_bits(HDMITX_DWC_AUD_CONF0, 0xf, 0, 4);
		hdmitx_set_reg_bits(HDMITX_DWC_AUD_CONF0, !!tx_aud_param->aud_src_if, 5, 1);
		if (tx_aud_param->fifo_rst)
			hdmitx_hw_cntl_misc(&hdev->tx_hw.base, MISC_AUDIO_RESET, 1);
		usleep_range(2000, 3000);
		hdmitx_set_reg_bits(HDMITX_DWC_FC_PACKET_TX_EN, 1, 0, 1);
		hdmitx_set_reg_bits(HDMITX_DWC_FC_PACKET_TX_EN, 1, 3, 1);
	}
	HDMITX_INFO("audio state %s\n", flag == 0 ? "AUDIO_MUTE" : "AUDIO_UNMUTE");
	mutex_unlock(&aud_mutex);
}

static bool audio_get_mute_st(void)
{
	bool st[4];

	st[0] = hdmitx_get_bit(HDMITX_TOP_CLK_CNTL, 2);
	st[1] = hdmitx_get_bit(HDMITX_TOP_CLK_CNTL, 3);
	st[2] = hdmitx_get_bit(HDMITX_DWC_FC_PACKET_TX_EN, 0);
	st[3] = hdmitx_get_bit(HDMITX_DWC_FC_PACKET_TX_EN, 3);

	if (st[0] && st[1] && st[2] && st[3])
		return true;

	return false;
}

struct aud_para hdmiaud_config_data;
struct aud_para hsty_hdmiaud_config_data[8];
unsigned int hsty_hdmiaud_config_loc, hsty_hdmiaud_config_num;
static int hdmitx_set_audmode(struct hdmitx_hw_common *tx_hw,
			      struct aud_para *audio_param)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();
	unsigned int data32;
	u8 aud_output_i2s_ch;
	int acr_update = 0;

	if (!hdev || !audio_param)
		return -1;

	pr_debug(HW "set audio\n");
	aud_output_i2s_ch = hdev->tx_comm.cur_audio_param.aud_output_i2s_ch;
	mutex_lock(&aud_mutex);
	memcpy(&hdmiaud_config_data,
		   audio_param, sizeof(struct aud_para));
	if (hsty_hdmiaud_config_loc > 7)
		hsty_hdmiaud_config_loc = 0;
	memcpy(&hsty_hdmiaud_config_data[hsty_hdmiaud_config_loc++],
	       &hdmiaud_config_data, sizeof(struct aud_para));
	if (hsty_hdmiaud_config_num < 0xfffffff0)
		hsty_hdmiaud_config_num++;
	else
		hsty_hdmiaud_config_num = 8;

/* config IP */
/* Configure audio */
	/* I2S Sampler config */
	data32 = 0;
/* [  3] fifo_empty_mask: 0=enable int; 1=mask int. */
	data32 |= (1 << 3);
/* [  2] fifo_full_mask: 0=enable int; 1=mask int. */
	data32 |= (1 << 2);
	hdmitx_wr_reg(HDMITX_DWC_AUD_INT, data32);

	data32 = 0;
/* [  4] fifo_overrun_mask: 0=enable int; 1=mask int.
 * Enable it later when audio starts.
 */
	data32 |= (1 << 4);
	hdmitx_wr_reg(HDMITX_DWC_AUD_INT1,  data32);

	data32 = 0;
	data32 |= (0 << 5);  /* [7:5] i2s_mode: 0=standard I2S mode */
	data32 |= (24 << 0);  /* [4:0] i2s_width */
	hdmitx_wr_reg(HDMITX_DWC_AUD_CONF1, data32);

	data32 = 0;
	data32 |= (0 << 1);  /* [  1] NLPCM */
	data32 |= (0 << 0);  /* [  0] HBR */
	hdmitx_wr_reg(HDMITX_DWC_AUD_CONF2, data32);

	/* spdif sampler config */
/* [  2] SPDIF fifo_full_mask: 0=enable int; 1=mask int. */
/* [  3] SPDIF fifo_empty_mask: 0=enable int; 1=mask int. */
	data32 = 0;
	data32 |= (1 << 3);
	data32 |= (1 << 2);
	hdmitx_wr_reg(HDMITX_DWC_AUD_SPDIFINT,  data32);
	/* [  4] SPDIF fifo_overrun_mask: 0=enable int; 1=mask int. */
	data32 = 0;
	data32 |= (0 << 4);
	hdmitx_wr_reg(HDMITX_DWC_AUD_SPDIFINT1, data32);

	data32 = 0;
	data32 |= (0 << 7);  /* [  7] sw_audio_fifo_rst */
	hdmitx_wr_reg(HDMITX_DWC_AUD_SPDIF0, data32);

	set_aud_info_pkt(audio_param);
	acr_update = set_aud_acr_pkt(hdev, audio_param);
	set_aud_samp_pkt(audio_param);

	set_aud_chnls(audio_param);

	if (audio_param->aud_src_if == 1) {
		/* Enable audi2s_fifo_overrun interrupt */
		hdmitx_wr_reg(HDMITX_DWC_AUD_INT1,
			      hdmitx_rd_reg(HDMITX_DWC_AUD_INT1) & (~(1 << 4)));
		/* Wait for 40 us for TX I2S decoder to settle */
		usleep_range(40, 50);
	}
	set_aud_fifo_rst();
	hdmitx_set_reg_bits(HDMITX_DWC_FC_DATAUTO3, 1, 0, 1);
	if (audio_param->aud_output_en) {
		if (GET_OUTCHN_MSK(aud_output_i2s_ch))
			hdmitx_set_reg_bits(HDMITX_DWC_AUD_CONF0,
				GET_OUTCHN_MSK(aud_output_i2s_ch), 0, 4);
		else
			hdmitx_set_reg_bits(HDMITX_DWC_AUD_CONF0, 0xf, 0, 4);
		hdmitx_set_reg_bits(HDMITX_DWC_AUD_CONF0, !!audio_param->aud_src_if, 5, 1);
	}
	if (audio_param->fifo_rst)
		hdmitx_hw_cntl_misc(tx_hw, MISC_AUDIO_RESET, 1);
	if (audio_param->aud_output_en) {
		hdmitx_wr_reg(HDMITX_DWC_AUD_N1, hdmitx_rd_reg(HDMITX_DWC_AUD_N1));
		usleep_range(2000, 3000);
		hdmitx_set_reg_bits(HDMITX_DWC_FC_PACKET_TX_EN, 1, 0, 1);
	}
	mutex_unlock(&aud_mutex);

	return 0;
}

static int hdmitx_setupirq(struct hdmitx_hw_common *tx_hw)
{
	int r;
	struct hdmitx_dev *hdev = get_hdmitx_device();

	hdmitx_wr_reg(HDMITX_TOP_INTR_STAT_CLR, 0x7);
	r = request_irq(hdev->irq_hpd, &intr_handler,
			IRQF_SHARED, "hdmitx",
			(void *)hdev);
	if (r != 0)
		HDMITX_INFO(SYS "can't request hdmitx irq\n");
	r = request_irq(hdev->irq_viu1_vsync, &vsync_intr_handler,
			IRQF_SHARED, "hdmi_vsync",
			(void *)hdev);
	if (r != 0)
		HDMITX_INFO(SYS "can't request viu1_vsync irq\n");

	return r;
}

static void hdmitx_uninit(struct hdmitx_hw_common *tx_hw)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();

	free_irq(hdev->irq_hpd, (void *)hdev);
	HDMITX_DEBUG("power off hdmi, unmux hpd\n");

	phy_pll_off();
	hdmitx_hpd_hw_op(HPD_UNMUX_HPD);
}

static void hw_reset_dbg(void)
{
	u32 val1 = hdmitx_rd_reg(HDMITX_DWC_MC_CLKDIS);
	u32 val2 = hdmitx_rd_reg(HDMITX_DWC_FC_INVIDCONF);
	u32 val3 = hdmitx_rd_reg(HDMITX_DWC_FC_VSYNCINWIDTH);

	hdmitx_wr_reg(HDMITX_DWC_MC_CLKDIS, 0xff);
	usleep_range(9, 11);
	hdmitx_wr_reg(HDMITX_DWC_MC_CLKDIS, val1);
	usleep_range(9, 11);
	hdmitx_wr_reg(HDMITX_DWC_MC_SWRSTZREQ, 0);
	usleep_range(9, 11);
	hdmitx_wr_reg(HDMITX_DWC_FC_INVIDCONF, 0);
	usleep_range(9, 11);
	hdmitx_wr_reg(HDMITX_DWC_FC_INVIDCONF, val2);
	usleep_range(9, 11);
	hdmitx_wr_reg(HDMITX_DWC_FC_VSYNCINWIDTH, val3);
}

static int hdmitx_cntl(struct hdmitx_hw_common *tx_hw,
	unsigned int cmd, unsigned int argv)
{
	struct hdmitx20_hw *tx20_hw = to_hdmitx20_hw(tx_hw);

	if (cmd == HDMITX_AVMUTE_CNTL) {
		return 0;
	} else if (cmd == HDMITX_EARLY_SUSPEND_RESUME_CNTL) {
		if (argv == HDMITX_EARLY_SUSPEND) {
			/* phy disable, not disable HPLL/VSYNC */
			hdmi_phy_suspend();
		} else if (argv == HDMITX_LATE_RESUME) {
			/* No need below, will be set at set_disp_mode_auto() */
			/* hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL, 1, 30, 1); */
			hw_reset_dbg();
			HDMITX_DEBUG("swrstzreq\n");
		}
		return 0;
	} else if (cmd == HDMITX_HWCMD_MUX_HPD_IF_PIN_HIGH) {
		/* turnon digital module if gpio is high */
		if (hdmitx_hpd_hw_op(HPD_IS_HPD_MUXED) == 0) {
			if (hdmitx_hpd_hw_op(HPD_READ_HPD_GPIO)) {
				msleep(500);
				if (hdmitx_hpd_hw_op(HPD_READ_HPD_GPIO)) {
					HDMITX_DEBUG_HPD("mux hpd\n");
					hdmitx_set_sys_clk(tx20_hw, 4);
					msleep(100);
					hdmitx_hpd_hw_op(HPD_MUX_HPD);
				}
			}
		}
	} else if (cmd == HDMITX_HWCMD_MUX_HPD) {
		hdmitx_hpd_hw_op(HPD_MUX_HPD);
/* For test only. */
	} else if (cmd == HDMITX_HWCMD_TURNOFF_HDMIHW) {
		int unmux_hpd_flag = argv;

		if (unmux_hpd_flag) {
			HDMITX_DEBUG("power off hdmi, unmux hpd\n");
			phy_pll_off();
			hdmitx_hpd_hw_op(HPD_UNMUX_HPD);
		} else {
			HDMITX_DEBUG("power off hdmi\n");
			hdmitx_set_sys_clk(tx20_hw, 6);
			phy_pll_off();
		}
	}
	return 0;
}

static void hdmitx_print_info(struct hdmitx_dev *hdev, int HDMITX_INFO_flag)
{
	HDMITX_DEBUG("------------------\nHdmitx driver version: ");
	HDMITX_DEBUG("%s\n", HDMITX_VER);
	HDMITX_DEBUG("------------------\n");
}

#define DUMP_CVREG_SECTION(_start, _end) \
do { \
	typeof(_start) start = (_start); \
	typeof(_end) end = (_end); \
	if (start > end) { \
		HDMITX_INFO("Error start = 0x%x > end = 0x%x\n", \
			((start & 0xffff) >> 2), ((end & 0xffff) >> 2)); \
		break; \
	} \
	HDMITX_INFO("Start = 0x%x[0x%x]   End = 0x%x[0x%x]\n", \
		start, ((start & 0xffff) >> 2), end, ((end & 0xffff) >> 2)); \
	for (addr = start; addr < end + 1; addr += 4) {	\
		val = hd_read_reg(addr); \
		if (val) \
			HDMITX_INFO("0x%08x[0x%04x]: 0x%08x\n", addr, \
				((addr & 0xffff) >> 2), val); \
		} \
} while (0)

#define DUMP_HDMITXREG_SECTION(_start, _end) \
do { \
	typeof(_start) start = (_start); \
	typeof(_end) end = (_end); \
	if (start > end) \
		break; \
\
	for (addr = start; addr < end + 1; addr++) { \
		val = hdmitx_rd_reg(addr); \
		if (val) \
			HDMITX_INFO("[0x%08x]: 0x%08x\n", addr, val); \
	} \
} while (0)

static void hdmitx_dump_intr(void)
{
	unsigned int addr = 0, val = 0;

	DUMP_HDMITXREG_SECTION(HDMITX_DWC_IH_FC_STAT0,
		HDMITX_DWC_IH_I2CMPHY_STAT0);
	DUMP_HDMITXREG_SECTION(HDMITX_DWC_IH_DECODE, HDMITX_DWC_IH_DECODE);
	DUMP_HDMITXREG_SECTION(HDMITX_DWC_IH_MUTE_FC_STAT0,
		HDMITX_DWC_IH_MUTE_I2CMPHY_STAT0);
	DUMP_HDMITXREG_SECTION(HDMITX_DWC_IH_MUTE, HDMITX_DWC_IH_MUTE);
}

static void mode420_half_horizontal_para(void)
{
	unsigned int hactive = 0;
	unsigned int hblank = 0;
	unsigned int hfront = 0;
	unsigned int hsync = 0;

	hactive =  hdmitx_rd_reg(HDMITX_DWC_FC_INHACTV0);
	hactive += (hdmitx_rd_reg(HDMITX_DWC_FC_INHACTV1) & 0x3f) << 8;
	hblank =  hdmitx_rd_reg(HDMITX_DWC_FC_INHBLANK0);
	hblank += (hdmitx_rd_reg(HDMITX_DWC_FC_INHBLANK1) & 0x1f) << 8;
	hfront =  hdmitx_rd_reg(HDMITX_DWC_FC_HSYNCINDELAY0);
	hfront += (hdmitx_rd_reg(HDMITX_DWC_FC_HSYNCINDELAY1) & 0x1f) << 8;
	hsync =  hdmitx_rd_reg(HDMITX_DWC_FC_HSYNCINWIDTH0);
	hsync += (hdmitx_rd_reg(HDMITX_DWC_FC_HSYNCINWIDTH1) & 0x3) << 8;

	hactive = hactive / 2;
	hblank = hblank / 2;
	hfront = hfront / 2;
	hsync = hsync / 2;

	hdmitx_wr_reg(HDMITX_DWC_FC_INHACTV0, (hactive & 0xff));
	hdmitx_wr_reg(HDMITX_DWC_FC_INHACTV1, ((hactive >> 8) & 0x3f));
	hdmitx_wr_reg(HDMITX_DWC_FC_INHBLANK0, (hblank  & 0xff));
	hdmitx_wr_reg(HDMITX_DWC_FC_INHBLANK1, ((hblank >> 8) & 0x1f));
	hdmitx_wr_reg(HDMITX_DWC_FC_HSYNCINDELAY0, (hfront & 0xff));
	hdmitx_wr_reg(HDMITX_DWC_FC_HSYNCINDELAY1, ((hfront >> 8) & 0x1f));
	hdmitx_wr_reg(HDMITX_DWC_FC_HSYNCINWIDTH0, (hsync & 0xff));
	hdmitx_wr_reg(HDMITX_DWC_FC_HSYNCINWIDTH1, ((hsync >> 8) & 0x3));
}

static void hdmitx_set_fake_vic(struct hdmitx_dev *hdev)
{
	hdev->tx_comm.fmt_para.cs = HDMI_COLORSPACE_YUV444;
	hdev->tx_comm.fmt_para.vic = HDMI_VIC_FAKE;
	set_vmode_clk(hdev);
}

int hdmitx_debug_reg_dump(struct hdmitx_dev *hdev, char *buf, int len)
{
	int i, pos = 0;
	int cnt;
	unsigned int val;
	struct hdmitx_dbgreg_s **reg;

	if (!hdev || !hdev->tx_hw.chip_data || !buf || !len)
		return pos;
	reg = hdmitx_get_dbgregs(hdev->tx_hw.chip_data->chip_type);

	if (!reg)
		return pos;

	for (; *reg; reg++) {
		if (!(*reg)->rd_reg_func || !(*reg)->get_reg_paddr ||
			!(*reg)->name || !(*reg)->reg) {
			HDMITX_ERROR("lack dump reg member\n");
			continue;
		}
		cnt = snprintf(buf + pos, len - pos, "%s\n", (*reg)->name);
		if (cnt < 0)
			return cnt;
		pos += cnt;
		for (i = 0; (*reg)->reg[i] != REGS_END; i++) {
			val = (*reg)->rd_reg_func((*reg)->reg[i]);
			cnt = snprintf(buf + pos, len - pos,
				"[0x%08x]0x%08x\n",
				(*reg)->get_reg_paddr((*reg)->reg[i]), val);
			if (cnt < 0)
				return cnt;
			pos += cnt;
		}
	}

	return pos;
}

ssize_t _show20_clkmsr(char *buf)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();
	static const struct _hdmi_clkmsr *hdmiclkmsr;
	int i = 0, len = 0, pos = 0;

	switch (hdev->tx_hw.chip_data->chip_type) {
	case MESON_CPU_ID_G12A:
	case MESON_CPU_ID_G12B:
	case MESON_CPU_ID_SM1:
		hdmiclkmsr = hdmiclkmsr_G12A;
		len = ARRAY_SIZE(hdmiclkmsr_G12A);
		break;
	case MESON_CPU_ID_SC2:
		hdmiclkmsr = hdmiclkmsr_SC2;
		len = ARRAY_SIZE(hdmiclkmsr_SC2);
		break;
	default:
		break;
	}
	for (i = 0; i < len; i++)
		pos += snprintf(buf + pos, PAGE_SIZE - pos,
			"[%d] %d %s\n", hdmiclkmsr[i].idx,
			meson_clk_measure(hdmiclkmsr[i].idx),
			hdmiclkmsr[i].name);
	return pos;
}

static void hdmitx_debug(struct hdmitx_hw_common *tx_hw, const char *buf)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();
	char tmpbuf[128];
	int i = 0;
	int ret;
	unsigned long adr = 0;
	unsigned long value = 0;
	struct hdmi_format_para *para = &hdev->tx_comm.fmt_para;

	while ((buf[i]) && (buf[i] != ',') && (buf[i] != ' ')) {
		tmpbuf[i] = buf[i];
		i++;
	}
	tmpbuf[i] = 0;

	if (strncmp(tmpbuf, "testhpll", 8) == 0) {
		ret = kstrtoul(tmpbuf + 8, 10, &value);
		hdev->tx_comm.fmt_para.vic = value;
		set_vmode_clk(hdev);
		return;
	} else if (strncmp(tmpbuf, "testedid", 8) == 0) {
		hdmitx_hw_cntl_ddc(&hdev->tx_hw.base, DDC_RESET_EDID, 0);
		hdmitx_hw_cntl_ddc(&hdev->tx_hw.base, DDC_EDID_READ_DATA, 0);
		return;
	} else if (strncmp(tmpbuf, "i2c_reactive", 12) == 0) {
		hdmitx_hw_cntl_misc(&hdev->tx_hw.base, MISC_I2C_RESET, 0);
		return;
	} else if (strncmp(tmpbuf, "edid_check", 10) == 0) {
		if (strncmp(tmpbuf + 10, "=0", 2) == 0)
			hdev->tx_comm.rxcap.edid_check = 0;
		else if (strncmp(tmpbuf + 10, "=1", 2) == 0)
			hdev->tx_comm.rxcap.edid_check = 1;
		else if (strncmp(tmpbuf + 10, "=2", 2) == 0)
			hdev->tx_comm.rxcap.edid_check = 2;
		else if (strncmp(tmpbuf + 10, "=3", 2) == 0)
			hdev->tx_comm.rxcap.edid_check = 3;
		HDMITX_INFO("edid_check = %d\n", hdev->tx_comm.rxcap.edid_check);
		return;
	} else if (strncmp(tmpbuf, "bist", 4) == 0) {
		if (strncmp(tmpbuf + 4, "off", 3) == 0) {
			hdev->bist_lock = 0;
			hd_set_reg_bits(P_ENCP_VIDEO_MODE_ADV, 1, 3, 1);
			hd_write_reg(P_VENC_VIDEO_TST_EN, 0);
			return;
		}
		hdev->bist_lock = 1;
		hdmitx_wr_reg(HDMITX_DWC_FC_VSDSIZE, 0x05);
		hdmitx_hw_cntl_config(&hdev->tx_hw.base, CONF_AVI_RGBYCC_INDIC, para->cs);
		if (hdev->tx_hw.chip_data->chip_type < MESON_CPU_ID_SC2)
			hd_set_reg_bits(P_HHI_GCLK_OTHER, 1, 3, 1);
		hd_set_reg_bits(P_ENCP_VIDEO_MODE_ADV, 0, 3, 1);
		hd_write_reg(P_VENC_VIDEO_TST_EN, 1);
		if (strncmp(tmpbuf + 4, "line", 4) == 0) {
			hd_write_reg(P_VENC_VIDEO_TST_MDSEL, 2);
			return;
		}
		if (strncmp(tmpbuf + 4, "dot", 3) == 0) {
			hd_write_reg(P_VENC_VIDEO_TST_MDSEL, 3);
			return;
		}
		if (strncmp(tmpbuf + 4, "start", 5) == 0) {
			ret = kstrtoul(tmpbuf + 9, 10, &value);
			hd_write_reg(P_VENC_VIDEO_TST_CLRBAR_STRT, value);
			return;
		}
		if (strncmp(tmpbuf + 4, "shift", 5) == 0) {
			ret = kstrtoul(tmpbuf + 9, 10, &value);
			hd_write_reg(P_VENC_VIDEO_TST_VDCNT_STSET, value);
			return;
		}
		hd_write_reg(P_VENC_VIDEO_TST_MDSEL, 1);
		value = 1920;
		ret = kstrtoul(tmpbuf + 4, 10, &value);
		hd_write_reg(P_VENC_VIDEO_TST_CLRBAR_WIDTH, value / 8);
		return;
	} else if (strncmp(tmpbuf, "testaudio", 9) == 0) {
		hdmitx_set_audmode(&hdev->tx_hw.base, NULL);
	} else if (strncmp(tmpbuf, "dumpintr", 8) == 0) {
		hdmitx_dump_intr();
	} else if (strncmp(tmpbuf, "testhdcp", 8) == 0) {
		int i;

		i = tmpbuf[8] - '0';
		if (i == 2)
			HDMITX_INFO("hdcp rslt = %d", hdmitx_hdcp_opr(2));
		if (i == 1)
			hdmitx_hw_cntl_ddc(&hdev->tx_hw.base, DDC_HDCP_OP, HDCP14_ON);
		return;
	} else if (strncmp(tmpbuf, "chkfmt", 6) == 0) {
		hdmitx_mode_print_all_mode_table();
		return;
	} else if (strncmp(tmpbuf, "ss", 2) == 0) {
		HDMITX_INFO("hdev->hpd_state: 0x%x\n", hdev->tx_comm.hpd_state);
		HDMITX_INFO("hdev->tx_comm.fmt_para.vic: 0x%x\n", hdev->tx_comm.fmt_para.vic);
	} else if (strncmp(tmpbuf, "hpd_lock", 8) == 0) {
		if (tmpbuf[8] == '1') {
			hdev->tx_hw.base.debug_hpd_lock = 1;
			HDMITX_INFO("lock hpd\n");
		} else {
			hdev->tx_hw.base.debug_hpd_lock = 0;
			HDMITX_INFO("unlock hpd\n");
		}
		return;
	} else if (strncmp(tmpbuf, "hpd_stick", 9) == 0) {
		if (tmpbuf[9] == '1')
			hdev->hdcp_hpd_stick = 1;
		else
			hdev->hdcp_hpd_stick = 0;
		HDMITX_DEBUG_HPD("%sstick hpd\n",
			(hdev->hdcp_hpd_stick) ? "" : "un");
	} else if (strncmp(tmpbuf, "topo", 4) == 0) {
		HDMITX_INFO("topo: %d\n", hdmitx_hdcp_opr(0xe));
		return;
	} else if (strncmp(tmpbuf, "dumpcecreg", 10) == 0) {
		unsigned char cec_val = 0;
		unsigned int cec_adr = 0;
		/* HDMI CEC Regs address range:0xc000~0xc01c;0xc080~0xc094 */
		for (cec_adr = 0xc000; cec_adr < 0xc01d; cec_adr++) {
			cec_val = hdmitx_rd_reg(cec_adr);
			HDMITX_INFO(CEC "HDMI CEC Regs[0x%x]: 0x%x\n",
				cec_adr, cec_val);
		}
		for (cec_adr = 0xc080; cec_adr < 0xc095; cec_adr++) {
			cec_val = hdmitx_rd_reg(cec_adr);
			HDMITX_INFO(CEC "HDMI CEC Regs[0x%x]: 0x%x\n",
				cec_adr, cec_val);
		}
		return;
	} else if (strncmp(tmpbuf, "dumpcbusreg", 11) == 0) {
		unsigned int i, val;

		for (i = 0; i < 0x3000; i++) {
			val = hd_read_reg(CBUS_REG_ADDR(i));
			if (val)
				HDMITX_INFO("CBUS[0x%x]: 0x%x\n", i, val);
		}
		return;
	} else if (strncmp(tmpbuf, "dumpvcbusreg", 12) == 0) {
		unsigned int i, val;

		for (i = 0; i < 0x3000; i++) {
			val = hd_read_reg(VCBUS_REG_ADDR(i));
			if (val)
				HDMITX_INFO("VCBUS[0x%x]: 0x%x\n", i, val);
		}
		return;
	} else if (strncmp(tmpbuf, "cfgreg", 6) == 0) {
		ret = kstrtoul(tmpbuf + 6, 16, &adr);
		ret = kstrtoul(buf + i + 1, 16, &value);
		return;
	} else if ((tmpbuf[0] == 'v') &&
			   (strncmp(tmpbuf, "vsif_info", 9) != 0)) {
		hdmitx_print_info(hdev, 1);
		return;
	} else if (tmpbuf[0] == 'w') {
		unsigned long read_back = 0;

		ret = kstrtoul(tmpbuf + 2, 16, &adr);
		ret = kstrtoul(buf + i + 1, 16, &value);
		if (buf[1] == 'h') {
			hdmitx_wr_reg((unsigned int)adr, (unsigned int)value);
			read_back = hdmitx_rd_reg(adr);
		}
		HDMITX_INFO("write %lx to %s reg[%lx]\n", value, "HDMI", adr);
		/* read back in order to check writing is OK or NG. */
		HDMITX_INFO("Read Back %s reg[%lx]=%lx\n", "HDMI",
			adr, read_back);
	} else if (tmpbuf[0] == 'r') {
		ret = kstrtoul(tmpbuf + 2, 16, &adr);
		if (buf[1] == 'h')
			value = hdmitx_rd_reg(adr);
		HDMITX_INFO("%s reg[%lx]=%lx\n", "HDMI", adr, value);
	} else if (strncmp(tmpbuf, "prbs", 4) == 0) {
		unsigned int phy_cntl1;
		unsigned int phy_cntl4;
		unsigned int phy_status;

		switch (hdev->tx_hw.chip_data->chip_type) {
		case MESON_CPU_ID_SC2:
			phy_cntl1 = P_ANACTRL_HDMIPHY_CTRL1;
			phy_cntl4 = P_ANACTRL_HDMIPHY_CTRL4;
			phy_status = P_ANACTRL_HDMIPHY_STS;
			break;
		case MESON_CPU_ID_TM2:
		case MESON_CPU_ID_TM2B:
			phy_cntl1 = P_TM2_HHI_HDMI_PHY_CNTL1;
			phy_cntl4 = P_TM2_HHI_HDMI_PHY_CNTL4;
			phy_status = P_TM2_HHI_HDMI_PHY_STATUS;
			break;
		case MESON_CPU_ID_G12A:
		case MESON_CPU_ID_G12B:
		case MESON_CPU_ID_SM1:
		default:
			phy_cntl1 = P_HHI_HDMI_PHY_CNTL1;
			phy_cntl4 = P_HHI_HDMI_PHY_CNTL4;
			phy_status = P_HHI_HDMI_PHY_STATUS;
			break;
		}
		/* test prbs */
		for (i = 0; i < 4; i++) {
			hd_write_reg(phy_cntl1, 0x0390000f);
			hd_write_reg(phy_cntl1, 0x0390000e);
			hd_write_reg(phy_cntl1, 0x03904002);
			hd_write_reg(phy_cntl4, 0x0001efff
				| (i << 20));
			hd_write_reg(phy_cntl1, 0xef904002);
			mdelay(10);
			if (i > 0)
				HDMITX_INFO("prbs D[%d]:%x\n", i - 1,
					hd_read_reg(phy_status));
			else
				HDMITX_INFO("prbs clk :%x\n",
					hd_read_reg(phy_status));
		}
	} else if (strncmp(tmpbuf, "stop_vsif", 9) == 0) {
		hdmitx_disable_packet(HDMI_PACKET_VEND);
	} else if (strncmp(tmpbuf, "hdcp_mode", 9) == 0) {
		ret = kstrtoul(tmpbuf + 9, 16, &value);
		if (value <= 3 &&
		    hdev->hwop.am_hdmitx_set_hdcp_mode)
			hdev->hwop.am_hdmitx_set_hdcp_mode(value);
	} else if (strncmp(tmpbuf, "drm_set_hdmi", 12) == 0) {
		if (hdev->hwop.am_hdmitx_set_hdmi_mode)
			hdev->hwop.am_hdmitx_set_hdmi_mode();
	} else if (strncmp(tmpbuf, "drm_set_out", 11) == 0) {
		if (hdev->hwop.am_hdmitx_set_out_mode)
			hdev->hwop.am_hdmitx_set_out_mode();
	} else if (strncmp(tmpbuf, "drm_hdcp_op", 11) == 0) {
		ret = kstrtoul(tmpbuf + 11, 16, &value);
		if (value == 0 && hdev->hwop.am_hdmitx_hdcp_disable)
			hdev->hwop.am_hdmitx_hdcp_disable();
		else if (value == 1 && hdev->hwop.am_hdmitx_hdcp_enable)
			hdev->hwop.am_hdmitx_hdcp_enable();
		else if (value == 2 && hdev->hwop.am_hdmitx_hdcp_disconnect)
			hdev->hwop.am_hdmitx_hdcp_disconnect();
	} else if (strncmp(tmpbuf, "avmute_frame", 12) == 0) {
		ret = kstrtoul(tmpbuf + 12, 10, &value);
		hdev->tx_comm.debug_param.avmute_frame = value;
		HDMITX_INFO("avmute_frame = %lu\n", value);
	} else if (strncmp(tmpbuf, "csc_en", 6) == 0) {
		ret = kstrtoul(tmpbuf + 6, 0, &value);
		/* 0: no change
		 * 1: force switch color space converter to 444,8bit
		 * 2: force switch color space converter to 422,12bit
		 * 3: force switch color space converter to rgb,8bit
		 */
		if (value != 0 && value != 1 && value != 2 && value != 3) {
			HDMITX_ERROR("set csc in 0 ~ 3\n");
		} else {
			HDMITX_INFO("set csc_en as %lu\n", value);

			hdmitx_hw_cntl_config(&hdev->tx_hw.base, CONFIG_CSC,
				value | CSC_UPDATE_AVI_CS);
		}
	} else if (strncmp(tmpbuf, "config_csc_en", 13) == 0) {
		ret = kstrtoul(tmpbuf + 13, 0, &value);
		HDMITX_INFO("config_csc_en to %lu\n", value);

		if (value == 0)
			hdev->tx_comm.config_csc_en = false;
		else if (value == 1)
			hdev->tx_comm.config_csc_en = true;
	} else if (strncmp(tmpbuf, "set_div40", 9) == 0) {
		/* echo 1 > div40, force send 1:40 tmds bit clk ratio
		 * echo 0 > div40, send 1:10 tmds bit clk ratio if scdc_present
		 * echo 2 > div40, force send 1:10 tmds bit clk ratio
		 */
		ret = kstrtoul(tmpbuf + 9, 0, &value);
		if (value != 0 && value != 1 && value != 2) {
			HDMITX_ERROR("set div40 value in 0 ~ 2\n");
		} else {
			HDMITX_INFO("set div40 to %lu\n", value);
			hdmitx_hw_cntl_ddc(&hdev->tx_hw.base,
				DDC_SCDC_DIV40_SCRAMB, value);
		}
	} else if (strncmp(tmpbuf, "drm_hdcp_ver", 12) == 0) {
		HDMITX_INFO("test drm_hdcp_ver: %d\n", meson_hdcp_get_rx_cap());
	}
}

int hdmitx_pkt_dump(struct hdmitx_dev *hdmitx_device, char *buf, int len)
{
	unsigned int reg_val;
	unsigned int reg_addr;
	unsigned char *conf;
	int pos = 0;

	//GCP PKT
	pos += snprintf(buf + pos, len - pos, "hdmitx gcp reg config\n");
	reg_addr = HDMITX_DWC_FC_GCP;
	reg_val = hdmitx_rd_reg(reg_addr);
	pos += snprintf(buf + pos, len - pos, "GCP.clear_avmute: %d\n", reg_val & 0x1);
	pos += snprintf(buf + pos, len - pos, "GCP.set_avmute: %d\n", (reg_val & 0x2) >> 1);
	pos += snprintf(buf + pos, len - pos, "GCP.default_phase: %d\n", (reg_val & 0x4) >> 2);

	reg_addr = HDMITX_DWC_VP_STATUS;
	reg_val = hdmitx_rd_reg(reg_addr);
	pos += snprintf(buf + pos, len - pos, "GCP.packing_phase: %d\n", reg_val & 0xf);

	reg_addr = HDMITX_DWC_VP_PR_CD;
	reg_val = hdmitx_rd_reg(reg_addr);

	switch ((reg_val & 0xf0) >> 4) {
	case 0:
	case 4:
		conf = "24bit";
		break;
	case 5:
		conf = "30bit";
		break;
	case 6:
		conf = "36bit";
		break;
	case 7:
		conf = "48bit";
		break;
	default:
		conf = "reserved";
	}
	pos += snprintf(buf + pos, len - pos, "GCP.color_depth: %s\n", conf);

	reg_addr = HDMITX_DWC_VP_REMAP;
	reg_val = hdmitx_rd_reg(reg_addr);
	switch (reg_val & 0x3) {
	case 0:
		conf = "16bit";
		break;
	case 1:
		conf = "20bit";
		break;
	case 2:
		conf = "24bit";
		break;
	default:
		conf = "reserved";
	}
	pos += snprintf(buf + pos, len - pos, "YCC 422 size: %s\n", conf);

	reg_addr = HDMITX_DWC_VP_CONF;
	reg_val = hdmitx_rd_reg(reg_addr);

	switch (reg_val & 0x3) {
	case 0:
		conf = "pixel_packing";
		break;
	case 1:
		conf = "YCC 422";
		break;
	case 2:
	case 3:
		conf = "8bit bypass";
	}
	pos += snprintf(buf + pos, len - pos, "output selector: %s\n", conf);
	pos += snprintf(buf + pos, len - pos, "bypass select: %d\n", (reg_val & 0x4) >> 2);
	pos += snprintf(buf + pos, len - pos, "YCC 422 enable: %d\n", (reg_val & 0x8) >> 3);
	pos += snprintf(buf + pos, len - pos, "pixel repeater enable: %d\n", (reg_val & 0x10) >> 4);
	pos += snprintf(buf + pos, len - pos, "pixel packing enable: %d\n", (reg_val & 0x20) >> 5);
	pos += snprintf(buf + pos, len - pos, "bypass enable: %d\n", (reg_val & 0x40) >> 6);

	reg_addr = HDMITX_DWC_FC_DATAUTO3;
	reg_val = hdmitx_rd_reg(reg_addr);

	switch ((reg_val & 0x4) >> 2) {
	case 0:
		conf = "RDRB";
		break;
	case 1:
		conf = "auto";
	}
	pos += snprintf(buf + pos, len - pos, "GCP.mode : %s\n", conf);

	reg_addr = HDMITX_DWC_FC_PACKET_TX_EN;
	reg_val = hdmitx_rd_reg(reg_addr);

	switch ((reg_val & 0x2) >> 1) {
	case 0:
		conf = "disable";
		break;
	case 1:
		conf = "enable";
	}
	pos += snprintf(buf + pos, len - pos, "GCP.enable : %s\n\n", conf);

	//AVI PKT
	pos += snprintf(buf + pos, PAGE_SIZE, "hdmitx avi info reg config\n");

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
	pos += snprintf(buf + pos, PAGE_SIZE, "AVI.colorspace: %s\n", conf);

	switch ((reg_val & 0x40) >> 6) {
	case 0:
		conf = "disable";
		break;
	case 1:
		conf = "enable";
	}
	pos += snprintf(buf + pos, PAGE_SIZE, "AVI.active_aspect: %s\n", conf);

	switch ((reg_val & 0x0c) >> 2) {
	case 0:
		conf = "disable";
		break;
	case 1:
		conf = "vert bar";
		break;
	case 2:
		conf = "horiz bar";
		break;
	case 3:
		conf = "vert and horiz bar";
	}
	pos += snprintf(buf + pos, PAGE_SIZE, "AVI.bar: %s\n", conf);

	switch ((reg_val & 0x30) >> 4) {
	case 0:
		conf = "disable";
		break;
	case 1:
		conf = "overscan";
		break;
	case 2:
		conf = "underscan";
		break;
	default:
		conf = "disable";
	}
	pos += snprintf(buf + pos, PAGE_SIZE, "AVI.scan: %s\n", conf);

	reg_addr = HDMITX_DWC_FC_AVICONF1;
	reg_val = hdmitx_rd_reg(reg_addr);

	switch ((reg_val & 0xc0) >> 6) {
	case 0:
		conf = "disable";
		break;
	case 1:
		conf = "BT.601";
		break;
	case 2:
		conf = "BT.709";
		break;
	case 3:
		conf = "Extended";
	}
	pos += snprintf(buf + pos, PAGE_SIZE, "AVI.colorimetry: %s\n", conf);

	switch ((reg_val & 0x30) >> 4) {
	case 0:
		conf = "disable";
		break;
	case 1:
		conf = "4:3";
		break;
	case 2:
		conf = "16:9";
		break;
	default:
		conf = "disable";
	}
	pos += snprintf(buf + pos, PAGE_SIZE, "AVI.picture_aspect: %s\n", conf);

	switch (reg_val & 0xf) {
	case 8:
		conf = "Same as picture_aspect";
		break;
	case 9:
		conf = "4:3";
		break;
	case 10:
		conf = "16:9";
		break;
	case 11:
		conf = "14:9";
		break;
	default:
		conf = "Same as picture_aspect";
	}
	pos += snprintf(buf + pos, PAGE_SIZE, "AVI.active_aspect: %s\n", conf);

	reg_addr = HDMITX_DWC_FC_AVICONF2;
	reg_val = hdmitx_rd_reg(reg_addr);

	switch ((reg_val & 0x80) >> 7) {
	case 0:
		conf = "disable";
		break;
	case 1:
		conf = "enable";
	}
	pos += snprintf(buf + pos, PAGE_SIZE, "AVI.itc: %s\n", conf);

	switch ((reg_val & 0x70) >> 4) {
	case 0:
		conf = "xvYCC601";
		break;
	case 1:
		conf = "xvYCC709";
		break;
	case 2:
		conf = "sYCC601";
		break;
	case 3:
		conf = "Adobe_YCC601";
		break;
	case 4:
		conf = "Adobe_RGB";
		break;
	case 5:
	case 6:
		conf = "BT.2020";
		break;
	default:
		conf = "xvYCC601";
	}
	pos += snprintf(buf + pos, PAGE_SIZE, "AVI.extended_colorimetry: %s\n", conf);

	switch ((reg_val & 0xc) >> 2) {
	case 0:
		conf = "default";
		break;
	case 1:
		conf = "limited";
		break;
	case 2:
		conf = "full";
		break;
	default:
		conf = "default";
	}
	pos += snprintf(buf + pos, PAGE_SIZE, "AVI.quantization_range: %s\n", conf);

	switch (reg_val & 0x3) {
	case 0:
		conf = "unknown";
		break;
	case 1:
		conf = "horiz";
		break;
	case 2:
		conf = "vert";
		break;
	case 3:
		conf = "horiz and vert";
	}
	pos += snprintf(buf + pos, PAGE_SIZE, "AVI.nups: %s\n", conf);

	reg_addr = HDMITX_DWC_FC_AVIVID;
	reg_val = hdmitx_rd_reg(reg_addr);
	pos += snprintf(buf + pos, PAGE_SIZE, "AVI.video_code: %d\n", reg_val);

	reg_addr = HDMITX_DWC_FC_AVICONF3;
	reg_val = hdmitx_rd_reg(reg_addr);

	switch ((reg_val & 0xc) >> 2) {
	case 0:
	default:
		conf = "limited";
		break;
	case 1:
		conf = "full";
	}
	pos += snprintf(buf + pos, PAGE_SIZE, "AVI.ycc_quantization_range: %s\n", conf);

	switch (reg_val & 0x3) {
	case 0:
		conf = "graphics";
		break;
	case 1:
		conf = "photo";
		break;
	case 2:
		conf = "cinema";
		break;
	case 3:
		conf = "game";
	}
	pos += snprintf(buf + pos, PAGE_SIZE, "AVI.content_type: %s\n", conf);

	reg_addr = HDMITX_DWC_FC_PRCONF;
	reg_val = hdmitx_rd_reg(reg_addr);

	switch ((reg_val & 0xf0) >> 4) {
	case 0:
	case 1:
	default:
		conf = "no";
		break;
	case 2:
		conf = "2 times";
	}
	pos += snprintf(buf + pos, PAGE_SIZE, "AVI.pixel_repetition: %s\n", conf);

	reg_addr = HDMITX_DWC_FC_DATAUTO3;
	reg_val = hdmitx_rd_reg(reg_addr);

	switch ((reg_val & 0x8) >> 3) {
	case 0:
		conf = "RDRB";
		break;
	case 1:
		conf = "auto";
	}
	pos += snprintf(buf + pos, PAGE_SIZE, "AVI.mode : %s\n", conf);

	reg_addr = HDMITX_DWC_FC_RDRB6;
	reg_val = hdmitx_rd_reg(reg_addr);
	pos += snprintf(buf + pos, PAGE_SIZE, "AVI.rdrb_interpolation : %d\n", reg_val & 0xf);
	reg_addr = HDMITX_DWC_FC_RDRB7;
	reg_val = hdmitx_rd_reg(reg_addr);
	pos += snprintf(buf + pos, PAGE_SIZE, "AVI.rdrb_perframe : %d\n", (reg_val & 0xf0) >> 4);
	pos += snprintf(buf + pos, PAGE_SIZE, "AVI.rdrb_linespace : %d\n", reg_val & 0xf);
	reg_addr = HDMITX_DWC_FC_PACKET_TX_EN;
	reg_val = hdmitx_rd_reg(reg_addr);

	switch ((reg_val & 0x4) >> 2) {
	case 0:
		conf = "disable";
		break;
	case 1:
		conf = "enable";
	}
	pos += snprintf(buf + pos, PAGE_SIZE, "AVI.enable : %s\n\n", conf);

	//ACR PKT
	pos += snprintf(buf + pos, len - pos, "hdmitx audio acr info reg config\n");
	reg_addr = HDMITX_DWC_AUD_INPUTCLKFS;
	reg_val = hdmitx_rd_reg(reg_addr);

	switch (reg_val & 0x7) {
	case 0:
		conf = "128XFs";
		break;
	case 1:
		conf = "512XFs";
		break;
	case 4:
		conf = "64XFs";
		break;
	default:
		conf = "reserved";
	}
	pos += snprintf(buf + pos, len - pos, "ACR.ifsfactor: %s\n", conf);

	reg_addr = HDMITX_DWC_AUD_N1;
	reg_val = hdmitx_rd_reg(reg_addr);
	pos += snprintf(buf + pos, len - pos, "ACR.N[7:0]: 0x%x\n", reg_val);

	reg_addr = HDMITX_DWC_AUD_N2;
	reg_val = hdmitx_rd_reg(reg_addr);
	pos += snprintf(buf + pos, len - pos, "ACR.N[15:8]: 0x%x\n", reg_val);

	reg_addr = HDMITX_DWC_AUD_N3;
	reg_val = hdmitx_rd_reg(reg_addr);
	pos += snprintf(buf + pos, len - pos, "ACR.ncts_atomic_write: %d\n", (reg_val & 0x80) >> 7);
	pos += snprintf(buf + pos, len - pos, "ACR.ncts_atomic_write: %d\n", (reg_val & 0x80) >> 7);

	reg_addr = HDMITX_DWC_AUD_CTS1;
	reg_val = hdmitx_rd_reg(reg_addr);
	pos += snprintf(buf + pos, len - pos, "ACR.CTS[7:0]: 0x%x\n", reg_val);

	reg_addr = HDMITX_DWC_AUD_CTS2;
	reg_val = hdmitx_rd_reg(reg_addr);
	pos += snprintf(buf + pos, len - pos, "ACR.CTS[15:8]: 0x%x\n", reg_val);

	reg_addr = HDMITX_DWC_AUD_CTS3;
	reg_val = hdmitx_rd_reg(reg_addr);
	pos += snprintf(buf + pos, len - pos, "ACR.CTS[19:16]: 0x%x\n", reg_val & 0xf);
	pos += snprintf(buf + pos, len - pos, "ACR.CTS_manual: %d\n", (reg_val & 0x10) >> 4);

	switch ((reg_val & 0xe0) >> 5) {
	case 0:
		conf = "1";
		break;
	case 1:
		conf = "16";
		break;
	case 2:
		conf = "32";
		break;
	case 3:
		conf = "64";
		break;
	case 4:
		conf = "128";
		break;
	case 5:
		conf = "256";
		break;
	default:
		conf = "128";
	}
	pos += snprintf(buf + pos, len - pos, "ACR.N_shift: %s\n", conf);
	pos += snprintf(buf + pos, len - pos, "actual N = audN[19:0]/N_shift\n");
	reg_addr = HDMITX_DWC_FC_DATAUTO3;
	reg_val = hdmitx_rd_reg(reg_addr);

	switch (reg_val & 0x1) {
	case 0:
		conf = "RDRB";
		break;
	case 1:
		conf = "auto";
	}
	pos += snprintf(buf + pos, len - pos, "ACR.mode : %s\n", conf);

	reg_addr = HDMITX_DWC_FC_PACKET_TX_EN;
	reg_val = hdmitx_rd_reg(reg_addr);

	switch (reg_val & 0x1) {
	case 0:
		conf = "disable";
		break;
	case 1:
		conf = "enable";
	}
	pos += snprintf(buf + pos, len - pos, "ACR.enable : %s\n\n", conf);

	//DRM PKT
	reg_addr = HDMITX_DWC_FC_DRM_HB01;
	reg_val = hdmitx_rd_reg(reg_addr);
	pos += snprintf(buf + pos, len - pos, "DRM.version: %d\n", reg_addr);
	reg_addr = HDMITX_DWC_FC_DRM_HB02;
	reg_val = hdmitx_rd_reg(reg_addr);
	pos += snprintf(buf + pos, len - pos, "DRM.size: %d\n", reg_addr);

	reg_addr = HDMITX_DWC_FC_DRM_PB00;
	reg_val = hdmitx_rd_reg(reg_addr);

	switch (reg_val) {
	case 0:
		conf = "sdr";
		break;
	case 1:
		conf = "hdr";
		break;
	case 2:
		conf = "ST 2084";
		break;
	case 3:
		conf = "HLG";
		break;
	default:
		conf = "sdr";
	}
	pos += snprintf(buf + pos, len - pos, "DRM.eotf: %s\n", conf);

	reg_addr = HDMITX_DWC_FC_DRM_PB01;
	reg_val = hdmitx_rd_reg(reg_addr);

	switch (reg_val) {
	case 0:
		conf = "static metadata";
		break;
	default:
		conf = "reserved";
	}
	pos += snprintf(buf + pos, len - pos, "DRM.metadata_id: %s\n", conf);
	for (reg_addr = HDMITX_DWC_FC_DRM_PB02;
		reg_addr <= HDMITX_DWC_FC_DRM_PB26; reg_addr++) {
		reg_val = hdmitx_rd_reg(reg_addr);
		pos += snprintf(buf + pos, len - pos, "[0x%x]: 0x%x\n", reg_addr, reg_val);
	}
	reg_addr = HDMITX_DWC_FC_DATAUTO3;
	reg_val = hdmitx_rd_reg(reg_addr);

	switch ((reg_val & 0x40) >> 6) {
	case 0:
		conf = "RDRB";
		break;
	case 1:
	default:
		conf = "auto";
	}
	pos += snprintf(buf + pos, len - pos, "DRM.mode : %s\n", conf);

	reg_addr = HDMITX_DWC_FC_PACKET_TX_EN;
	reg_val = hdmitx_rd_reg(reg_addr);

	switch ((reg_val & 0x80) >> 7) {
	case 0:
		conf = "disable";
		break;
	case 1:
	default:
		conf = "enable";
	}
	pos += snprintf(buf + pos, len - pos, "DRM.enable : %s\n\n", conf);

	//VSIF PKT
	pos += snprintf(buf + pos, len - pos, "vsif info config\n");

	reg_addr = HDMITX_DWC_FC_VSDSIZE;
	reg_val = hdmitx_rd_reg(reg_addr);
	pos += snprintf(buf + pos, len - pos, "VSIF.size: %d\n", reg_val);

	reg_addr = HDMITX_DWC_FC_VSDIEEEID0;
	reg_val = hdmitx_rd_reg(reg_addr);
	pos += snprintf(buf + pos, len - pos, "VSDIEEEID0: 0x%x\n", reg_val);

	reg_addr = HDMITX_DWC_FC_VSDIEEEID1;
	reg_val = hdmitx_rd_reg(reg_addr);
	pos += snprintf(buf + pos, len - pos, "VSDIEEEID1: 0x%x\n", reg_val);

	reg_addr = HDMITX_DWC_FC_VSDIEEEID2;
	reg_val = hdmitx_rd_reg(reg_addr);
	pos += snprintf(buf + pos, len - pos, "VSDIEEEID2: 0x%x\n", reg_val);

	for (reg_addr = HDMITX_DWC_FC_VSDPAYLOAD0;
		reg_addr <= HDMITX_DWC_FC_VSDPAYLOAD23; reg_addr++) {
		reg_val = hdmitx_rd_reg(reg_addr);
		pos += snprintf(buf + pos, len - pos, "[0x%x]: 0x%x\n", reg_addr, reg_val);
	}

	reg_addr = HDMITX_DWC_FC_DATAUTO0;
	reg_val = hdmitx_rd_reg(reg_addr);

	switch ((reg_val & 0x8) >> 3) {
	case 0:
		conf = "manual";
		break;
	case 1:
	default:
		conf = "RDRB";
	}
	pos += snprintf(buf + pos, len - pos, "VSIF.mode : %s\n", conf);

	reg_addr = HDMITX_DWC_FC_DATAUTO1;
	reg_val = hdmitx_rd_reg(reg_addr);
	pos += snprintf(buf + pos, len - pos, "VSIF.rdrb_interpolation : %d\n", reg_val & 0xf);
	reg_addr = HDMITX_DWC_FC_DATAUTO2;
	reg_val = hdmitx_rd_reg(reg_addr);
	pos += snprintf(buf + pos, len - pos, "VSIF.rdrb_perframe : %d\n", (reg_val & 0xf0) >> 4);
	pos += snprintf(buf + pos, len - pos, "VSIF.rdrb_linespace : %d\n", reg_val & 0xf);

	reg_addr = HDMITX_DWC_FC_PACKET_TX_EN;
	reg_val = hdmitx_rd_reg(reg_addr);

	switch ((reg_val & 0x10) >> 4) {
	case 0:
		conf = "disable";
		break;
	case 1:
	default:
		conf = "enable";
	}
	pos += snprintf(buf + pos, len - pos, "VSIF.enable : %s\n\n", conf);

	//AUDIO PKT
	pos += snprintf(buf + pos, len - pos, "hdmitx audio info reg config\n");

	reg_addr = HDMITX_DWC_FC_AUDICONF0;
	reg_val = hdmitx_rd_reg(reg_addr);

	switch (reg_val & 0xf) {
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
	pos += snprintf(buf + pos, len - pos, "AUDI.coding_type: %s\n", conf);

	switch ((reg_val & 0x70) >> 4) {
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
	case CC_REFER_TO_STREAM:
	default:
		conf = "refer to stream header";
		break;
	}
	pos += snprintf(buf + pos, len - pos, "AUDI.channel_count: %s\n", conf);

	reg_addr = HDMITX_DWC_FC_AUDICONF1;
	reg_val = hdmitx_rd_reg(reg_addr);

	switch (reg_val & 0x7) {
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
	}
	pos += snprintf(buf + pos, len - pos, "AUDI.sample_frequency: %s\n", conf);

	switch ((reg_val & 0x30) >> 4) {
	case SS_16BITS:
		conf = "16bit";
		break;
	case SS_20BITS:
		conf = "20bit";
		break;
	case SS_24BITS:
		conf = "24bit";
		break;
	case SS_REFER_TO_STREAM:
	default:
		conf = "refer to stream header";
		break;
	}
	pos += snprintf(buf + pos, len - pos, "AUDI.sample_size: %s\n", conf);

	reg_addr = HDMITX_DWC_FC_AUDICONF2;
	reg_val = hdmitx_rd_reg(reg_addr);
	pos += snprintf(buf + pos, len - pos, "AUDI.channel_allocation: %d\n", reg_val);

	reg_addr = HDMITX_DWC_FC_AUDICONF3;
	reg_val = hdmitx_rd_reg(reg_addr);
	pos += snprintf(buf + pos, len - pos, "AUDI.level_shift_value: %d\n", reg_val & 0xf);
	pos += snprintf(buf + pos, len - pos, "AUDI.down_mix_enable: %d\n", (reg_val & 0x10) >> 4);
	pos += snprintf(buf + pos, len - pos,
		"AUDI.LFE_playback_info: %d\n", (reg_val & 0x60) >> 5);

	reg_addr = HDMITX_DWC_FC_DATAUTO3;
	reg_val = hdmitx_rd_reg(reg_addr);

	switch ((reg_val & 0x2) >> 1) {
	case 0:
		conf = "RDRB";
		break;
	case 1:
		conf = "auto";
	}
	pos += snprintf(buf + pos, len - pos, "AUDI.mode : %s\n", conf);

	reg_addr = HDMITX_DWC_FC_PACKET_TX_EN;
	reg_val = hdmitx_rd_reg(reg_addr);

	switch ((reg_val & 0x8) >> 3) {
	case 0:
		conf = "disable";
		break;
	case 1:
		conf = "enable";
	}
	pos += snprintf(buf + pos, len - pos, "AUDI.enable : %s\n\n", conf);

	//AUDIO SAMPLE
	pos += snprintf(buf + pos, len - pos, "hdmitx audio sample reg config\n");

	reg_addr = HDMITX_DWC_AUD_CONF0;
	reg_val = hdmitx_rd_reg(reg_addr);

	switch ((reg_val & 0x20) >> 5) {
	case 0:
	default:
		conf = "SPDIF/GPA";
		break;
	case 1:
		conf = "I2S";
	}
	pos += snprintf(buf + pos, len - pos, "i2s_select : %s\n", conf);
	pos += snprintf(buf + pos, len - pos, "I2S_in_en: %d\n", reg_val & 0xf);

	reg_addr = HDMITX_DWC_AUD_CONF1;
	reg_val = hdmitx_rd_reg(reg_addr);
	pos += snprintf(buf + pos, len - pos, "I2S_width: %d bit\n", reg_val & 0x1f);

	switch ((reg_val & 0xe0) >> 5) {
	case 0:
		conf = "standard";
		break;
	case 1:
		conf = "Right-justified";
		break;
	case 2:
		conf = "Left-justified";
		break;
	case 3:
		conf = "Burst 1 mode";
		break;
	case 4:
		conf = "Burst 2 mode";
		break;
	default:
		conf = "standard";
	}
	pos += snprintf(buf + pos, len - pos, "I2S_mode: %s\n", conf);

	reg_addr = HDMITX_DWC_AUD_CONF2;
	reg_val = hdmitx_rd_reg(reg_addr);
	pos += snprintf(buf + pos, len - pos, "HBR mode enable: %d\n", reg_val & 0x1);
	pos += snprintf(buf + pos, len - pos, "NLPCM mode enable: %d\n", (reg_val & 0x2) >> 1);

	reg_addr = HDMITX_DWC_AUD_SPDIF1;
	reg_val = hdmitx_rd_reg(reg_addr);
	pos += snprintf(buf + pos, len - pos, "SPDIF_width: %d bit\n", reg_val & 0x1f);
	pos += snprintf(buf + pos, len - pos, "SPDIF_HBR_MODE: %d\n", (reg_val & 0x40) >> 6);
	pos += snprintf(buf + pos, len - pos, "SPDIF_NLPCM_MODE: %d\n", (reg_val & 0x80) >> 7);

	reg_addr = HDMITX_DWC_FC_AUDSCONF;
	reg_val = hdmitx_rd_reg(reg_addr);
	pos += snprintf(buf + pos, len - pos, "layout : %d\n", reg_val & 0x1);
	pos += snprintf(buf + pos, len - pos, "sample flat: %d\n", (reg_val & 0xf0) >> 4);

	reg_addr = HDMITX_DWC_FC_AUDSSTAT;
	reg_val = hdmitx_rd_reg(reg_addr);
	pos += snprintf(buf + pos, len - pos, "sample present : %d\n", reg_val & 0xf);

	reg_addr = HDMITX_DWC_FC_AUDSV;
	reg_val = hdmitx_rd_reg(reg_addr);
	pos += snprintf(buf + pos, len - pos, "audio sample validity flag\n");
	pos += snprintf(buf + pos, len - pos, "channel 0, Left : %d\n", reg_val & 0x1);
	pos += snprintf(buf + pos, len - pos, "channel 1, Left : %d\n", (reg_val & 0x2) >> 1);
	pos += snprintf(buf + pos, len - pos, "channel 2, Left : %d\n", (reg_val & 0x4) >> 2);
	pos += snprintf(buf + pos, len - pos, "channel 3, Left : %d\n", (reg_val & 0x8) >> 3);
	pos += snprintf(buf + pos, len - pos, "channel 0, Right : %d\n", (reg_val & 0x10) >> 4);
	pos += snprintf(buf + pos, len - pos, "channel 1, Right : %d\n", (reg_val & 0x20) >> 5);
	pos += snprintf(buf + pos, len - pos, "channel 2, Right : %d\n", (reg_val & 0x40) >> 6);
	pos += snprintf(buf + pos, len - pos, "channel 3, Right : %d\n", (reg_val & 0x80) >> 7);

	reg_addr = HDMITX_DWC_FC_AUDSU;
	reg_val = hdmitx_rd_reg(reg_addr);
	pos += snprintf(buf + pos, len - pos, "audio sample user flag\n");
	pos += snprintf(buf + pos, len - pos, "channel 0, Left : %d\n", reg_val & 0x1);
	pos += snprintf(buf + pos, len - pos, "channel 1, Left : %d\n", (reg_val & 0x2) >> 1);
	pos += snprintf(buf + pos, len - pos, "channel 2, Left : %d\n", (reg_val & 0x4) >> 2);
	pos += snprintf(buf + pos, len - pos, "channel 3, Left : %d\n", (reg_val & 0x8) >> 3);
	pos += snprintf(buf + pos, len - pos, "channel 0, Right : %d\n", (reg_val & 0x10) >> 4);
	pos += snprintf(buf + pos, len - pos, "channel 1, Right : %d\n", (reg_val & 0x20) >> 5);
	pos += snprintf(buf + pos, len - pos, "channel 2, Right : %d\n", (reg_val & 0x40) >> 6);
	pos += snprintf(buf + pos, len - pos, "channel 3, Right : %d\n\n", (reg_val & 0x80) >> 7);

	//AUDIO CHANNEL
	pos += snprintf(buf + pos, len - pos, "hdmitx audio channel status reg config\n");

	reg_addr = HDMITX_DWC_FC_AUDSCHNLS0;
	reg_val = hdmitx_rd_reg(reg_addr);
	pos += snprintf(buf + pos, len - pos, "iec_copyright: %d\n", reg_val & 0x1);
	pos += snprintf(buf + pos, len - pos, "iec_cgmsa: %d\n", (reg_val & 0x30) >> 4);

	reg_addr = HDMITX_DWC_FC_AUDSCHNLS1;
	reg_val = hdmitx_rd_reg(reg_addr);
	pos += snprintf(buf + pos, len - pos, "iec_categorycode: %d\n", reg_val);

	reg_addr = HDMITX_DWC_FC_AUDSCHNLS2;
	reg_val = hdmitx_rd_reg(reg_addr);
	pos += snprintf(buf + pos, len - pos, "iec_sourcenumber: %d\n", reg_val & 0xf);
	pos += snprintf(buf + pos, len - pos, "iec_pcmaudiomode: %d\n", (reg_val & 0x30) >> 4);

	reg_addr = HDMITX_DWC_FC_AUDSCHNLS3;
	reg_val = hdmitx_rd_reg(reg_addr);
	pos += snprintf(buf + pos, len - pos, "iec_channelnumcr0: %d\n", reg_val & 0xf);
	pos += snprintf(buf + pos, len - pos, "iec_channelnumcr1: %d\n", (reg_val & 0xf0) >> 4);

	reg_addr = HDMITX_DWC_FC_AUDSCHNLS4;
	reg_val = hdmitx_rd_reg(reg_addr);
	pos += snprintf(buf + pos, len - pos, "iec_channelnumcr2: %d\n", reg_val & 0xf);
	pos += snprintf(buf + pos, len - pos, "iec_channelnumcr3: %d\n", (reg_val & 0xf0) >> 4);

	reg_addr = HDMITX_DWC_FC_AUDSCHNLS5;
	reg_val = hdmitx_rd_reg(reg_addr);
	pos += snprintf(buf + pos, len - pos, "iec_channelnumcl0: %d\n", reg_val & 0xf);
	pos += snprintf(buf + pos, len - pos, "iec_channelnumcl1: %d\n", (reg_val & 0xf0) >> 4);

	reg_addr = HDMITX_DWC_FC_AUDSCHNLS6;
	reg_val = hdmitx_rd_reg(reg_addr);
	pos += snprintf(buf + pos, len - pos, "iec_channelnumcl2: %d\n", reg_val & 0xf);
	pos += snprintf(buf + pos, len - pos, "iec_channelnumcl3: %d\n", (reg_val & 0xf0) >> 4);

	reg_addr = HDMITX_DWC_FC_AUDSCHNLS7;
	reg_val = hdmitx_rd_reg(reg_addr);

	switch (reg_val & 0xf) {
	case 0:
		conf = "44.1kHz";
		break;
	case 1:
		conf = "not indicated";
		break;
	case 2:
		conf = "48kHz";
		break;
	case 3:
		conf = "32kHz";
		break;
	case 8:
		conf = "88.2kHz";
		break;
	case 9:
		conf = "768kHz";
		break;
	case 10:
		conf = "96kHz";
		break;
	case 12:
		conf = "176.4kHz";
		break;
	case 14:
		conf = "192kHz";
		break;
	default:
		conf = "not indicated";
	}
	pos += snprintf(buf + pos, len - pos, "iec_sampfreq: %s\n", conf);
	pos += snprintf(buf + pos, len - pos, "iec_clk: %d\n", (reg_val & 0x30) >> 4);
	pos += snprintf(buf + pos, len - pos, "iec_sampfreq_ext: %d\n", (reg_val & 0xc0) >> 6);

	reg_addr = HDMITX_DWC_FC_AUDSCHNLS8;
	reg_val = hdmitx_rd_reg(reg_addr);

	switch (reg_val & 0xf) {
	case 0:
	case 1:
		conf = "not indicated";
		break;
	case 2:
		conf = "16bit";
		break;
	case 4:
		conf = "18bit";
		break;
	case 8:
		conf = "19bit";
		break;
	case 10:
		conf = "20bit";
		break;
	case 12:
		conf = "17bit";
		break;
	case 3:
		conf = "20bit";
		break;
	case 5:
		conf = "22bit";
		break;
	case 9:
		conf = "23bit";
		break;
	case 11:
		conf = "24bit";
		break;
	case 13:
		conf = "21bit";
		break;
	default:
		conf = "not indicated";
	}
	pos += snprintf(buf + pos, len - pos, "iec_worldlength: %s\n", conf);

	switch ((reg_val & 0xf0) >> 4) {
	case 0:
		conf = "not indicated";
		break;
	case 1:
		conf = "192kHz";
		break;
	case 3:
		conf = "176.4kHz";
		break;
	case 5:
		conf = "96kHz";
		break;
	case 7:
		conf = "88.2kHz";
		break;
	case 13:
		conf = "48kHz";
		break;
	case 15:
		conf = "44.1kHz";
		break;
	default:
		conf = "not indicated";
	}
	pos += snprintf(buf + pos, len - pos, "iec_origsamplefreq: %s\n", conf);
	return pos;
}

static int get_hdcp_depth(void)
{
	int ret;

	hdmitx_set_reg_bits(HDMITX_DWC_A_KSVMEMCTRL, 1, 0, 1);
	hdmitx_poll_reg(HDMITX_DWC_A_KSVMEMCTRL, (1 << 1), 2 * HZ);
	ret = hdmitx_rd_reg(HDMITX_DWC_HDCP_BSTATUS_1) & 0x7;
	hdmitx_set_reg_bits(HDMITX_DWC_A_KSVMEMCTRL, 0, 0, 1);

	return ret;
}

static bool get_hdcp_max_cascade(void)
{
	bool ret;

	hdmitx_set_reg_bits(HDMITX_DWC_A_KSVMEMCTRL, 1, 0, 1);
	hdmitx_poll_reg(HDMITX_DWC_A_KSVMEMCTRL, (1 << 1), 2 * HZ);
	ret = (hdmitx_rd_reg(HDMITX_DWC_HDCP_BSTATUS_1) >> 3) & 0x1;
	hdmitx_set_reg_bits(HDMITX_DWC_A_KSVMEMCTRL, 0, 0, 1);

	return ret;
}

static bool get_hdcp_max_devs(void)
{
	bool ret;

	hdmitx_set_reg_bits(HDMITX_DWC_A_KSVMEMCTRL, 1, 0, 1);
	hdmitx_poll_reg(HDMITX_DWC_A_KSVMEMCTRL, (1 << 1), 2 * HZ);
	ret = (hdmitx_rd_reg(HDMITX_DWC_HDCP_BSTATUS_0) >> 7) & 0x1;
	hdmitx_set_reg_bits(HDMITX_DWC_A_KSVMEMCTRL, 0, 0, 1);

	return ret;
}

static int get_hdcp_device_count(void)
{
	int ret;

	hdmitx_set_reg_bits(HDMITX_DWC_A_KSVMEMCTRL, 1, 0, 1);
	hdmitx_poll_reg(HDMITX_DWC_A_KSVMEMCTRL, (1 << 1), 2 * HZ);
	ret = hdmitx_rd_reg(HDMITX_DWC_HDCP_BSTATUS_0) & 0x7f;
	hdmitx_set_reg_bits(HDMITX_DWC_A_KSVMEMCTRL, 0, 0, 1);

	return ret;
}

static void get_hdcp_bstatus(int *ret1, int *ret2)
{
	hdmitx_set_reg_bits(HDMITX_DWC_A_KSVMEMCTRL, 1, 0, 1);
	hdmitx_poll_reg(HDMITX_DWC_A_KSVMEMCTRL, (1 << 1), 2 * HZ);
	*ret1 = hdmitx_rd_reg(HDMITX_DWC_HDCP_BSTATUS_0);
	*ret2 = hdmitx_rd_reg(HDMITX_DWC_HDCP_BSTATUS_1);
	hdmitx_set_reg_bits(HDMITX_DWC_A_KSVMEMCTRL, 0, 0, 1);
}

static void hdcp_ksv_store(struct hdcprp_topo *topo,
			   unsigned char *dat, int no)
{
	int i;
	int pos;

	if (!topo)
		return;
	if (topo->hdcp_ver != 1)
		return;
	/* must check ksv num to prevent leak */
	if (topo->topo.topo14.device_count >= MAX_KSV_LISTS)
		return;

	pos = topo->topo.topo14.device_count * 5;
	for (i = 0; (i < no) && (i < MAX_KSV_LISTS * 5); i++)
		topo->topo.topo14.ksv_list[pos++] = dat[i];
	topo->topo.topo14.device_count += no / 5;
}

static u8 *hdcp_mksvlistbuf;
static int ksv_sha_matched;
static void hdcp_ksv_sha1_calc(struct hdmitx_dev *hdev)
{
	size_t list = 0;
	size_t size = 0;
	size_t i = 0;
	int valid = HDCP_NULL;
	char temp[MAX_KSV_LISTS * 5];
	int j = 0;

	/* 0x165e: Page 95 */
	memset(&tmp_ksv_lists, 0, sizeof(tmp_ksv_lists));
	memset(&temp[0], 0, sizeof(temp));
	hdcp_mksvlistbuf = kmalloc(0x1660, GFP_ATOMIC);
	if (hdcp_mksvlistbuf) {
		/* KSV_SIZE; */
		list = hdmitx_rd_reg(HDMITX_DWC_HDCP_BSTATUS_0) & KSV_MASK;
		if (list <= MAX_KSV_LISTS) {
			size = (list * KSV_SIZE) + HDCP_HEAD + SHA_MAX_SIZE;
			for (i = 0; i < size; i++) {
				if (i < HDCP_HEAD) { /* BSTATUS & M0 */
					hdcp_mksvlistbuf[(list * KSV_SIZE) + i] =
						hdmitx_rd_reg(HDMITX_DWC_HDCP_BSTATUS_0 + i);
				} else if (i < (HDCP_HEAD +
					(list * KSV_SIZE))) {
					/* KSV list */
					hdcp_mksvlistbuf[i - HDCP_HEAD] =
						hdmitx_rd_reg(HDMITX_DWC_HDCP_BSTATUS_0 + i);
					tmp_ksv_lists.lists[tmp_ksv_lists.no++] =
						hdcp_mksvlistbuf[i - HDCP_HEAD];
					temp[j++] =
						hdcp_mksvlistbuf[i - HDCP_HEAD];
				} else { /* SHA */
					hdcp_mksvlistbuf[i] =
						hdmitx_rd_reg(HDMITX_DWC_HDCP_BSTATUS_0 + i);
				}
			}
			if (calc_hdcp_ksv_valid(hdcp_mksvlistbuf, size) == TRUE) {
				valid = HDCP_KSVLIST_VALID;
			} else {
				valid = HDCP_KSVLIST_INVALID;
				hdmitx_current_status(HDMITX_HDCP_AUTH_VI_MISMATCH_ERROR);
			}
			ksv_sha_matched = valid;
		}
		hdmitx_set_reg_bits(HDMITX_DWC_A_KSVMEMCTRL, 0, 0, 1);
		hdmitx_set_reg_bits(HDMITX_DWC_A_KSVMEMCTRL,
				    (valid == HDCP_KSVLIST_VALID) ? 0 : 1,
				    3, 1);
		if (valid == HDCP_KSVLIST_VALID) {
			tmp_ksv_lists.valid = 1;
			for (i = 0; (i < j) &&
			     (tmp_ksv_lists.no < MAX_KSV_LISTS * 5); i++)
				tmp_ksv_lists.lists[tmp_ksv_lists.no++] =
					temp[i];
		}
		hdmitx_set_reg_bits(HDMITX_DWC_A_KSVMEMCTRL, 1, 2, 1);
		hdmitx_set_reg_bits(HDMITX_DWC_A_KSVMEMCTRL, 0, 2, 1);
		kfree(hdcp_mksvlistbuf);
	} else {
		HDMITX_INFO("hdcptx14: KSV List memory not valid\n");
	}
}

static int max_exceed = 200;
MODULE_PARM_DESC(max_exceed, "\nmax_exceed\n");
module_param(max_exceed, int, 0664);

static void hdcptx_events_handle(struct timer_list *t)
{
	struct hdmitx_dev *hdev = from_timer(hdev, t, hdcp_timer);
	unsigned char ksv[5] = {0};
	int i;
	unsigned int bcaps_6_rp;
	static unsigned int bcaps_5_ksvfifoready;
	static unsigned int st_flag = -1;
	static unsigned int hdcpobs3_1;
	unsigned int hdcpobs3_2;
	struct hdcprp14_topo *topo14 = &hdev->topo_info->topo.topo14;
	int bstatus0 = 0;
	int bstatus1 = 0;

	if (hdev->tx_comm.rxsense_policy && !hdmitx_tmds_rxsense())
		return;
	if (hdev->hdcp_max_exceed_cnt == 0) {
		hdcpobs3_1 = 0;
		bcaps_5_ksvfifoready = 0;
	}

	hdcpobs3_2 = hdmitx_rd_reg(HDMITX_DWC_A_HDCPOBS3);
	if (hdcpobs3_1 != hdcpobs3_2)
		hdcpobs3_1 = hdcpobs3_2;

	bcaps_6_rp = !!(hdcpobs3_1 & (1 << 6));
	bcaps_5_ksvfifoready = !!(hdcpobs3_1 & (1 << 5));

	if (bcaps_6_rp && bcaps_5_ksvfifoready &&
	    hdev->hdcp_max_exceed_cnt == 0)
		hdev->hdcp_max_exceed_cnt++;
	if (hdev->hdcp_max_exceed_cnt)
		hdev->hdcp_max_exceed_cnt++;
	if (bcaps_6_rp && bcaps_5_ksvfifoready) {
		if (hdev->hdcp_max_exceed_cnt > max_exceed &&
		    !ksv_sha_matched) {
			topo14->max_devs_exceeded = 1;
			topo14->max_cascade_exceeded = 1;
			hdev->hdcp_max_exceed_state = 1;
			hdmitx_current_status(HDMITX_HDCP_AUTH_TOPOLOGY_ERROR);
		}
	}

	if (st_flag != hdmitx_rd_reg(HDMITX_DWC_A_APIINTSTAT)) {
		st_flag = hdmitx_rd_reg(HDMITX_DWC_A_APIINTSTAT);
		HDMITX_INFO("hdcp14: instat: 0x%x\n", st_flag);
	}

	if (st_flag & (1 << 7)) {
		hdmitx_wr_reg(HDMITX_DWC_A_APIINTCLR, 1 << 7);
		hdmitx_hdcp_opr(3);
		if (bcaps_6_rp)
			get_hdcp_bstatus(&bstatus0, &bstatus1);
		for (i = 0; i < 5; i++)
			ksv[i] = (unsigned char)
				hdmitx_rd_reg(HDMITX_DWC_HDCPREG_BKSV0 + i);
		/* if downstream is only RX */
		if (global_tx_hw->base.hdcp_repeater_en == 1 && hdev->topo_info) {
			hdcp_ksv_store(hdev->topo_info, ksv, 5);
			if (tmp_ksv_lists.valid) {
				int cnt = get_hdcp_device_count();
				int devs = get_hdcp_max_devs();
				int cascade = get_hdcp_max_cascade();
				int depth = get_hdcp_depth();

				hdcp_ksv_store(hdev->topo_info,
					       tmp_ksv_lists.lists,
					       tmp_ksv_lists.no);
				if (cnt >= 127) {
					topo14->device_count = 127;
					topo14->max_devs_exceeded = 1;
				} else {
					topo14->device_count = cnt + 1;
					topo14->max_devs_exceeded = devs;
				}

				if (depth >= 7) {
					topo14->depth = 7;
					topo14->max_cascade_exceeded = 1;
				} else {
					topo14->depth = depth + 1;
					topo14->max_cascade_exceeded = cascade;
				}
			} else {
				topo14->device_count = 1;
				topo14->max_devs_exceeded = 0;
				topo14->max_cascade_exceeded = 0;
				topo14->depth = 1;
			}
		}
	}
	if (st_flag & (1 << 6)) {
		struct hdcp_obs_val obs_cur;

		hdmitx_hw_cntl_ddc(&hdev->tx_hw.base,
			DDC_HDCP14_SAVE_OBS, (unsigned long)&obs_cur);
		if (obs_cur.intstat & (3 << 3))
			hdmitx_current_status(HDMITX_HDCP_I2C_ERROR);
		if (((obs_cur.obs0 >> 4) == 3) && (((obs_cur.obs0 >> 1) & 0x7) == 0))
			hdmitx_current_status(HDMITX_HDCP_AUTH_R0_MISMATCH_ERROR);
		if (((obs_cur.obs0 >> 4) == 9) && (((obs_cur.obs0 >> 1) & 0x7) == 2))
			hdmitx_current_status(HDMITX_HDCP_AUTH_VI_MISMATCH_ERROR);
		if (((obs_cur.obs0 >> 4) == 8) && (((obs_cur.obs0 >> 1) & 0x7) == 1))
			hdmitx_current_status(HDMITX_HDCP_AUTH_REPEATER_DELAY_ERROR);
	}
	if (st_flag & (1 << 4))
		hdmitx_current_status(HDMITX_HDCP_I2C_ERROR);

	if (st_flag & (1 << 1)) {
		hdmitx_wr_reg(HDMITX_DWC_A_APIINTCLR, (1 << 1));
		hdmitx_wr_reg(HDMITX_DWC_A_KSVMEMCTRL, 0x1);
		hdmitx_poll_reg(HDMITX_DWC_A_KSVMEMCTRL, (1 << 1), 2 * HZ);
		if (hdmitx_rd_reg(HDMITX_DWC_A_KSVMEMCTRL) & (1 << 1)) {
			hdcp_ksv_sha1_calc(hdev);
		} else {
			HDMITX_INFO("hdcptx14: KSV List memory access denied\n");
			return;
		}
		hdmitx_wr_reg(HDMITX_DWC_A_KSVMEMCTRL, 0x4);
	}
	mod_timer(&hdev->hdcp_timer, jiffies + HZ / 100);
}

static void hdcp_start_timer(struct hdmitx_dev *hdev)
{
	static int init_flag;

	if (!init_flag) {
		init_flag = 1;
		timer_setup(&hdev->hdcp_timer, hdcptx_events_handle, 0);
		hdev->hdcp_timer.expires = jiffies + HZ / 100;
		add_timer(&hdev->hdcp_timer);
		return;
	}
	hdev->hdcp_timer.expires = jiffies + HZ / 100;
	mod_timer(&hdev->hdcp_timer, jiffies + HZ / 100);
}

static void set_pkf_duk_nonce(void)
{
	static int nonce_mode = 1; /* 1: use HW nonce   0: use SW nonce */

	/* Configure duk/pkf */
	hdmitx_hdcp_opr(0xc);
	if (nonce_mode == 1) {
		hdmitx_wr_reg(HDMITX_TOP_SKP_CNTL_STAT, 0xf);
	} else {
		hdmitx_wr_reg(HDMITX_TOP_SKP_CNTL_STAT, 0xe);
/* Configure nonce[127:0].
 * MSB must be written the last to assert nonce_vld signal.
 */
		hdmitx_wr_reg(HDMITX_TOP_NONCE_0,  0x32107654);
		hdmitx_wr_reg(HDMITX_TOP_NONCE_1,  0xba98fedc);
		hdmitx_wr_reg(HDMITX_TOP_NONCE_2,  0xcdef89ab);
		hdmitx_wr_reg(HDMITX_TOP_NONCE_3,  0x45670123);
		hdmitx_wr_reg(HDMITX_TOP_NONCE_0,  0x76543210);
		hdmitx_wr_reg(HDMITX_TOP_NONCE_1,  0xfedcba98);
		hdmitx_wr_reg(HDMITX_TOP_NONCE_2,  0x89abcdef);
		hdmitx_wr_reg(HDMITX_TOP_NONCE_3,  0x01234567);
	}
	usleep_range(9, 11);
}

static void check_read_ksv_list_st(void)
{
	int cnt = 0;

	for (cnt = 0; cnt < 5; cnt++) {
		if (((hdmitx_rd_reg(HDMITX_DWC_A_HDCPOBS1) & 0x7) == 5) ||
		    ((hdmitx_rd_reg(HDMITX_DWC_A_HDCPOBS1) & 0x7) == 6))
			msleep(20);
		else
			return;
	}
	HDMITX_INFO("hdcp14: FSM: A9 read ksv list\n");
}

static int hdmitx_cntl_ddc(struct hdmitx_hw_common *tx_hw,
	unsigned int cmd, unsigned long argv)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();
	struct hdcp_obs_val *obs;
	int i = 0;
	int ret = 0;
	unsigned char *tmp_char = NULL;
	struct hdcprp14_topo *topo14 = NULL;
	unsigned int val;
	unsigned char tmp[5];

	if ((cmd & CMD_DDC_OFFSET) != CMD_DDC_OFFSET) {
		HDMITX_ERROR(HW "ddc: invalid cmd 0x%x\n", cmd);
		return -1;
	}

	switch (cmd) {
	case DDC_HDCP14_SAVE_OBS:
		obs = (struct hdcp_obs_val *)argv;
		ret = 0;
		tmp[0] = hdmitx_rd_reg(HDMITX_DWC_A_HDCPOBS0) & 0xff;
		tmp[1] = hdmitx_rd_reg(HDMITX_DWC_A_HDCPOBS1) & 0xff;
		tmp[2] = hdmitx_rd_reg(HDMITX_DWC_A_HDCPOBS2) & 0xff;
		tmp[3] = hdmitx_rd_reg(HDMITX_DWC_A_HDCPOBS3) & 0xff;
		tmp[4] = hdmitx_rd_reg(HDMITX_DWC_A_APIINTSTAT) & 0xff;
		/* if current status is not equal last obs, then return 1 */
		if (obs->obs0 != tmp[0]) {
			obs->obs0 = tmp[0];
			ret |= (1 << 0);
		}
		if (obs->obs1 != tmp[1]) {
			obs->obs1 = tmp[1];
			ret |= (1 << 1);
		}
		if (obs->obs2 != tmp[2]) {
			obs->obs2 = tmp[2];
			ret |= (1 << 2);
		}
		if (obs->obs3 != tmp[3]) {
			obs->obs3 = tmp[3];
			ret |= (1 << 3);
		}
		if (obs->intstat != tmp[4]) {
			obs->intstat = tmp[4];
			ret |= (1 << 4);
		}
		return ret;
	case DDC_RESET_EDID:
		hdmitx_wr_reg(HDMITX_DWC_I2CM_SOFTRSTZ, 0);
		break;
	case DDC_EDID_READ_DATA:
		hdmitx_read_edid(hdev->tx_comm.EDID_buf);
		break;
	case DDC_GLITCH_FILTER_RESET:
		hdmitx_set_reg_bits(HDMITX_TOP_SW_RESET, 1, 6, 1);
		/*keep resetting DDC for some time*/
		usleep_range(1000, 2000);
		hdmitx_set_reg_bits(HDMITX_TOP_SW_RESET, 0, 6, 1);
		/*wait recover for resetting DDC*/
		usleep_range(1000, 2000);
		break;
	case DDC_PIN_MUX_OP:
		if (argv == PIN_MUX)
			hdmitx_ddc_hw_op(DDC_MUX_DDC);
		if (argv == PIN_UNMUX)
			hdmitx_ddc_hw_op(DDC_UNMUX_DDC);
		break;
	case DDC_RESET_HDCP:
		hdmitx_set_reg_bits(HDMITX_TOP_SW_RESET, 1, 5, 1);
		usleep_range(9, 11);
		hdmitx_set_reg_bits(HDMITX_TOP_SW_RESET, 0, 5, 1);
		usleep_range(9, 11);
		break;
	case DDC_HDCP_MUX_INIT:
		if (argv == 2) {
			if (hdev->tx_hw.chip_data->chip_type >= MESON_CPU_ID_SC2)
				hd_write_reg(P_CLKCTRL_HDCP22_CLK_CTRL,
					     0x01000100);
			else
				hd_write_reg(P_HHI_HDCP22_CLK_CNTL, 0x01000100);
			hdmitx_ddc_hw_op(DDC_MUX_DDC);
			if (hdev->tx_hw.chip_data->chip_type >= MESON_CPU_ID_SC2)
				hdmitx_set_reg_bits(HDMITX_DWC_MC_CLKDIS_SC2,
						    1, 6, 1);
			else
				hdmitx_set_reg_bits(HDMITX_DWC_MC_CLKDIS,
						    1, 6, 1);
			usleep_range(4, 6);
			hdmitx_set_reg_bits(HDMITX_DWC_HDCP22REG_CTRL, 3, 1, 2);
			hdmitx_set_reg_bits(HDMITX_TOP_SW_RESET, 1, 5, 1);
			usleep_range(9, 11);
			hdmitx_set_reg_bits(HDMITX_TOP_SW_RESET, 0, 5, 1);
			usleep_range(9, 11);
			hdmitx_wr_reg(HDMITX_DWC_HDCP22REG_MASK, 0);
			hdmitx_wr_reg(HDMITX_DWC_HDCP22REG_MUTE, 0);
			set_pkf_duk_nonce();
		} else if (argv == 1) {
			hdmitx_hdcp_opr(6);
		} else if (argv == 3) {
			if (hdev->tx_hw.chip_data->chip_type >= MESON_CPU_ID_SC2)
				hd_write_reg(P_CLKCTRL_HDCP22_CLK_CTRL,
					     0x01000100);
			else
				hd_write_reg(P_HHI_HDCP22_CLK_CNTL, 0x01000100);
			hdmitx_ddc_hw_op(DDC_MUX_DDC);
			if (hdev->tx_hw.chip_data->chip_type >= MESON_CPU_ID_SC2)
				hdmitx_set_reg_bits(HDMITX_DWC_MC_CLKDIS_SC2,
						    1, 6, 1);
			else
				hdmitx_set_reg_bits(HDMITX_DWC_MC_CLKDIS,
						    1, 6, 1);
			udelay(5);
			hdmitx_set_reg_bits(HDMITX_DWC_HDCP22REG_CTRL, 3, 1, 2);
		}
		break;
	case DDC_HDCP_OP:
		hdev->hdcp_max_exceed_state = 0;
		hdev->hdcp_max_exceed_cnt = 0;
		ksv_sha_matched = 0;
		memset(&tmp_ksv_lists, 0, sizeof(tmp_ksv_lists));
		del_timer(&hdev->hdcp_timer);
		if (hdev->topo_info)
			memset(hdev->topo_info, 0, sizeof(*hdev->topo_info));

		if (argv == HDCP14_ON) {
			check_read_ksv_list_st();
			if (hdev->topo_info)
				hdev->topo_info->hdcp_ver = HDCPVER_14;
			hdmitx_ddc_hw_op(DDC_MUX_DDC);
			hdmitx_set_reg_bits(HDMITX_TOP_SKP_CNTL_STAT, 0, 3, 1);
			hdmitx_set_reg_bits(HDMITX_TOP_CLK_CNTL, 1, 31, 1);
			hdmitx_hdcp_opr(6);
			hdmitx_hdcp_opr(1);
			hdcp_start_timer(hdev);
		}
		if (argv == HDCP14_OFF)
			hdmitx_hdcp_opr(4);
		if (argv == HDCP22_ON) {
			if (hdev->topo_info)
				hdev->topo_info->hdcp_ver = 2;
			hdmitx_ddc_hw_op(DDC_MUX_DDC);
			hdmitx_hdcp_opr(5);
			/* wait for start hdcp22app */
		}
		if (argv == HDCP22_OFF)
			hdmitx_hdcp_opr(6);
		break;
	case DDC_IS_HDCP_ON:
		/* argv = !!((hdmitx_rd_reg(TX_HDCP_MODE)) & (1 << 7)); */
		break;
	case DDC_HDCP_GET_BKSV:
		tmp_char = (unsigned char *)argv;
		for (i = 0; i < 5; i++) {
			tmp_char[i] = (unsigned char)
				hdmitx_rd_reg(HDMITX_DWC_HDCPREG_BKSV0 + 4 - i);
		}
		break;
	case DDC_HDCP_GET_AUTH:
		if (hdev->tx_comm.hdcp_mode == 1)
			return hdmitx_hdcp_opr(2);
		if (hdev->tx_comm.hdcp_mode == 2)
			return hdmitx_hdcp_opr(7);
		else
			return 0;
		break;
	case DDC_HDCP_14_LSTORE:
		return hdmitx_hdcp_opr(0xa);
	case DDC_HDCP_22_LSTORE:
		return hdmitx_hdcp_opr(0xb);
	case DDC_HDCP14_GET_TOPO_INFO:
		topo14 = (struct hdcprp14_topo *)argv;
		/* if rx is not repeater, directly return */
		if (!(hdmitx_rd_reg(HDMITX_DWC_A_HDCPOBS3) & (1 << 6)))
			return 0;
		hdmitx_set_reg_bits(HDMITX_DWC_A_KSVMEMCTRL, 1, 0, 1);
		hdmitx_poll_reg(HDMITX_DWC_A_KSVMEMCTRL, (1 << 1), 2 * HZ);
		val = hdmitx_rd_reg(HDMITX_DWC_HDCP_BSTATUS_0);
		topo14->device_count = val & 0x7f;
		topo14->max_devs_exceeded = !!(val & 0x80);
		val = hdmitx_rd_reg(HDMITX_DWC_HDCP_BSTATUS_1);
		topo14->depth = val & 0x7;
		topo14->max_cascade_exceeded = !!(val & 0x8);
		hdmitx_set_reg_bits(HDMITX_DWC_A_KSVMEMCTRL, 0, 0, 1);
		return 1;
	case DDC_HDCP_SET_TOPO_INFO:
		if (hdcp_topo_st != argv) {
			hdcp_topo_st = argv;
			hdmitx_hdcp_opr(0xd);
		}
		break;
	case DDC_SCDC_DIV40_SCRAMB:
		/* from hdmi2.1/2.0 spec chapter 10.4, prior to accessing
		 * the SCDC, source devices shall verify that the attached
		 * sink Device incorporates a valid HF-VSDB in the E-EDID
		 * in which the SCDC Present bit is set (=1). Source
		 * devices shall not attempt to access the SCDC unless the
		 * SCDC Present bit is set (=1).
		 * For some special TV(bug#164688), it support 6G 4k60hz,
		 * but not declare scdc_present in EDID, so still force to
		 * send 1:40 tmds bit clk ratio when output >3.4Gbps signal
		 * to cover such non-standard TV.
		 */
		if (argv == 1) {
			scdc_wr_sink(TMDS_CFG, 0x3); /* TMDS 1/40 & Scramble */
			scdc_wr_sink(TMDS_CFG, 0x3); /* TMDS 1/40 & Scramble */
			hdmitx_wr_reg(HDMITX_DWC_FC_SCRAMBLER_CTRL, 1);
		} else if (argv == 0) {
			if (hdev->tx_comm.rxcap.scdc_present ||
				hdev->pre_tmds_clk_div40) {
				scdc_wr_sink(TMDS_CFG, 0x0); /* TMDS 1/10 & Scramble */
				scdc_wr_sink(TMDS_CFG, 0x0); /* TMDS 1/10 & Scramble */
				hdmitx_wr_reg(HDMITX_DWC_FC_SCRAMBLER_CTRL, 0);
			} else {
				HDMITX_INFO("ERR: SCDC not present, should not send 1:10\n");
			}
		} else {
			/* force send 1:10 tmds bit clk ratio, for echo 2 > div40 */
			scdc_wr_sink(TMDS_CFG, 0x0); /* TMDS 1/10 & Scramble */
			scdc_wr_sink(TMDS_CFG, 0x0); /* TMDS 1/10 & Scramble */
			hdmitx_wr_reg(HDMITX_DWC_FC_SCRAMBLER_CTRL, 0);
		}
		hdev->pre_tmds_clk_div40 = (argv == 1);
		break;
	case DDC_HDCP14_GET_BCAPS_RP:
		return !!(hdmitx_rd_reg(HDMITX_DWC_A_HDCPOBS3) & (1 << 6));
	default:
		break;
	}
	return 1;
}

static int hdmitx_hdmi_dvi_config(struct hdmitx20_hw *tx_hw,
		unsigned int dvi_mode)
{
	if (dvi_mode == 1) {
		hdmitx_csc_config(TX_INPUT_COLOR_FORMAT,
				  HDMI_COLORSPACE_RGB, TX_COLOR_DEPTH);

		/* set dvi flag */
		hdmitx_set_reg_bits(HDMITX_DWC_FC_INVIDCONF, 0, 3, 1);

	} else {
		/* set hdmi flag */
		hdmitx_set_reg_bits(HDMITX_DWC_FC_INVIDCONF, 1, 3, 1);
	}

	return 0;
}

static int hdmitx_get_hdmi_dvi_config(struct hdmitx_dev *hdev)
{
	int value = hdmitx_rd_reg(HDMITX_DWC_FC_INVIDCONF) & 0x8;

	return (value == 0) ? DVI_MODE : HDMI_MODE;
}

/* 1: negative, 0: positive */
static unsigned int is_sync_polarity_negative(unsigned int vic)
{
	unsigned int ret = 0;

	if (vic >= 1 && vic <= 3)
		ret = 1;
	else if (vic >= 6 && vic <= 15)
		ret = 1;
	else if (vic >= 17 && vic <= 18)
		ret = 1;
	else if (vic >= 21 && vic <= 30)
		ret = 1;
	else if (vic >= 35 && vic <= 38)
		ret = 1;
	else if (vic >= 42 && vic <= 45)
		ret = 1;
	else if (vic >= 48 && vic <= 59)
		ret = 1;
	else
		ret = 0;
	return ret;
}

static void hdmitx_dith_ctrl(struct hdmitx_dev *hdev)
{
	unsigned int hs_flag = 0;

	switch (hdev->tx_comm.fmt_para.cd) {
	case COLORDEPTH_30B:
	case COLORDEPTH_36B:
	case COLORDEPTH_48B:
		if (hdev->tx_hw.chip_data->chip_type >= MESON_CPU_ID_GXM) {
			/* 12-10 dithering on */
			hd_set_reg_bits(P_VPU_HDMI_FMT_CTRL, 0, 4, 1);
			/* hsync/vsync not invert */
			/* hs_flag = (hd_read_reg(P_VPU_HDMI_SETTING) >> 2) & 0x3; */
			/* force to config sync pol of VPU_HDMI_SETTING
			 * and VPU_HDMI_DITH_CNTL
			 */
			if (is_sync_polarity_negative(hdev->tx_comm.fmt_para.vic))
				hs_flag = 0x0;
			else
				hs_flag = 0x3;
			hd_set_reg_bits(P_VPU_HDMI_SETTING, 0, 2, 2);
			/* 12-10 rounding off */
			hd_set_reg_bits(P_VPU_HDMI_FMT_CTRL, 0, 10, 1);
			/* 10-8 dithering off (2x2 old dither) */
			hd_set_reg_bits(P_VPU_HDMI_DITH_CNTL, 0, 4, 1);
			/* set hsync/vsync */
			hd_set_reg_bits(P_VPU_HDMI_DITH_CNTL, hs_flag, 2, 2);
		} else {
			hd_set_reg_bits(P_VPU_HDMI_FMT_CTRL, 0, 4, 1);
			hd_set_reg_bits(P_VPU_HDMI_FMT_CTRL, 0, 10, 1);
		}
		break;
	default:
		if (hdev->tx_hw.chip_data->chip_type >= MESON_CPU_ID_GXM) {
			/* 12-10 dithering off */
			hd_set_reg_bits(P_VPU_HDMI_FMT_CTRL, 0, 4, 1);
			/* 12-10 rounding on */
			hd_set_reg_bits(P_VPU_HDMI_FMT_CTRL, 1, 10, 1);
			/* 10-8 dithering on (2x2 old dither) */
			hd_set_reg_bits(P_VPU_HDMI_DITH_CNTL, 1, 4, 1);
			/* set hsync/vsync as default 0 */
			hd_set_reg_bits(P_VPU_HDMI_DITH_CNTL, 0, 2, 2);
		} else {
			hd_set_reg_bits(P_VPU_HDMI_FMT_CTRL, 0, 4, 1);
			hd_set_reg_bits(P_VPU_HDMI_FMT_CTRL, 1, 10, 1);
		}
		break;
	}
}

static void hdmitx_in_vid_map(enum hdmi_vic vic,
	unsigned char color_depth,
	unsigned char input_color_format,
	unsigned char output_color_format)
{
	unsigned long data32;
	unsigned char vid_map;

	/* Configure video */
	if (input_color_format == HDMI_COLORSPACE_RGB) {
		if (color_depth == COLORDEPTH_24B)
			vid_map = 0x01;
		else if (color_depth == COLORDEPTH_30B)
			vid_map = 0x03;
		else if (color_depth == COLORDEPTH_36B)
			vid_map = 0x05;
		else
			vid_map = 0x07;
	} else if (((input_color_format == HDMI_COLORSPACE_YUV444) ||
		(input_color_format == HDMI_COLORSPACE_YUV420)) &&
		(output_color_format != HDMI_COLORSPACE_YUV422)) {
		if (color_depth == COLORDEPTH_24B)
			vid_map = 0x09;
		else if (color_depth == COLORDEPTH_30B)
			vid_map = 0x0b;
		else if (color_depth == COLORDEPTH_36B)
			vid_map = 0x0d;
		else
			vid_map = 0x0f;
	} else {
		if (color_depth == COLORDEPTH_24B)
			vid_map = 0x16;
		else if (color_depth == COLORDEPTH_30B)
			vid_map = 0x14;
		else
			vid_map = 0x12;
	}

	switch (vic) {
	case HDMI_6_720x480i60_4x3:
	case HDMI_7_720x480i60_16x9:
	case HDMI_10_2880x480i60_4x3:
	case HDMI_11_2880x480i60_16x9:
	case HDMI_21_720x576i50_4x3:
	case HDMI_22_720x576i50_16x9:
	case HDMI_25_2880x576i50_4x3:
	case HDMI_26_2880x576i50_16x9:
	case HDMI_44_720x576i100_4x3:
	case HDMI_45_720x576i100_16x9:
	case HDMI_50_720x480i120_4x3:
	case HDMI_51_720x480i120_16x9:
	case HDMI_54_720x576i200_4x3:
	case HDMI_55_720x576i200_16x9:
	case HDMI_58_720x480i240_4x3:
	case HDMI_59_720x480i240_16x9:
		if (output_color_format == HDMI_COLORSPACE_YUV422) {
			if (color_depth == COLORDEPTH_24B)
				vid_map = 0x09;
			if (color_depth == COLORDEPTH_30B)
				vid_map = 0x0b;
			if (color_depth == COLORDEPTH_36B)
				vid_map = 0x0d;
		}
		break;
	default:
		break;
	}

	data32	= 0;
	data32 |= (0 << 7);
	data32 |= (vid_map << 0);
	hdmitx_wr_reg(HDMITX_DWC_TX_INVID0, data32);
}

static void hdmitx_vp_conf(unsigned char color_depth, unsigned char output_color_format)
{
	u32 data32	= 0;
	u32 tmp = 0;

	data32 |= (((color_depth == COLORDEPTH_30B) ? 1 :
		(color_depth == COLORDEPTH_36B) ? 2 : 0) << 0);
	hdmitx_wr_reg(HDMITX_DWC_VP_REMAP, data32);
	if (output_color_format == HDMI_COLORSPACE_YUV422) {
		switch (color_depth) {
		case COLORDEPTH_36B:
			tmp = 2;
			break;
		case COLORDEPTH_30B:
			tmp = 1;
			break;
		case COLORDEPTH_24B:
			tmp = 0;
			break;
		}
	}
	/* [1:0] ycc422_size */
	hdmitx_set_reg_bits(HDMITX_DWC_VP_REMAP, tmp, 0, 2);

	/* Video Packet configuration */
	data32	= 0;
	data32 |= ((((output_color_format != HDMI_COLORSPACE_YUV422) &&
		 (color_depth == COLORDEPTH_24B)) ? 1 : 0) << 6);
	data32 |= ((((output_color_format == HDMI_COLORSPACE_YUV422) ||
		 (color_depth == COLORDEPTH_24B)) ? 0 : 1) << 5);
	data32 |= (0 << 4);
	data32 |= (((output_color_format == HDMI_COLORSPACE_YUV422) ? 1 : 0)
		<< 3);
	data32 |= (1 << 2);
	data32 |= (((output_color_format == HDMI_COLORSPACE_YUV422) ? 1 :
		(color_depth == COLORDEPTH_24B) ? 2 : 0) << 0);
	hdmitx_wr_reg(HDMITX_DWC_VP_CONF, data32);
}

static void hdmitx_config_avi_cs(unsigned char output_color_format)
{
	unsigned char rgb_ycc_indicator;

	/* set rgb_ycc indicator */
	switch (output_color_format) {
	case HDMI_COLORSPACE_RGB:
		rgb_ycc_indicator = 0x0;
		break;
	case HDMI_COLORSPACE_YUV422:
		rgb_ycc_indicator = 0x1;
		break;
	case HDMI_COLORSPACE_YUV420:
		rgb_ycc_indicator = 0x3;
		break;
	case HDMI_COLORSPACE_YUV444:
	default:
		rgb_ycc_indicator = 0x2;
		break;
	}
	hdmitx_set_reg_bits(HDMITX_DWC_FC_AVICONF0,
			    ((rgb_ycc_indicator & 0x4) >> 2), 7, 1);
	hdmitx_set_reg_bits(HDMITX_DWC_FC_AVICONF0,
			    (rgb_ycc_indicator & 0x3), 0, 2);
}

static void hdmitx_pure_csc_config(unsigned char input_color_format,
			      unsigned char output_color_format,
			      unsigned char color_depth)
{
	unsigned char conv_en;
	unsigned char csc_scale;
	unsigned long csc_coeff_a1, csc_coeff_a2, csc_coeff_a3, csc_coeff_a4;
	unsigned long csc_coeff_b1, csc_coeff_b2, csc_coeff_b3, csc_coeff_b4;
	unsigned long csc_coeff_c1, csc_coeff_c2, csc_coeff_c3, csc_coeff_c4;
	unsigned long data32;

	conv_en = (((input_color_format  == HDMI_COLORSPACE_RGB) ||
		(output_color_format == HDMI_COLORSPACE_RGB)) &&
		(input_color_format  != output_color_format)) ? 1 : 0;

	if (conv_en) {
		if (output_color_format == HDMI_COLORSPACE_RGB) {
			csc_coeff_a1 = 0x2000;
			csc_coeff_a2 = 0x6926;
			csc_coeff_a3 = 0x74fd;
			csc_coeff_a4 = (color_depth == COLORDEPTH_24B) ?
				0x010e :
			(color_depth == COLORDEPTH_30B) ? 0x043b :
			(color_depth == COLORDEPTH_36B) ? 0x10ee :
			(color_depth == COLORDEPTH_48B) ? 0x10ee : 0x010e;
		csc_coeff_b1 = 0x2000;
		csc_coeff_b2 = 0x2cdd;
		csc_coeff_b3 = 0x0000;
		csc_coeff_b4 = (color_depth == COLORDEPTH_24B) ? 0x7e9a :
			(color_depth == COLORDEPTH_30B) ? 0x7a65 :
			(color_depth == COLORDEPTH_36B) ? 0x6992 :
			(color_depth == COLORDEPTH_48B) ? 0x6992 : 0x7e9a;
		csc_coeff_c1 = 0x2000;
		csc_coeff_c2 = 0x0000;
		csc_coeff_c3 = 0x38b4;
		csc_coeff_c4 = (color_depth == COLORDEPTH_24B) ? 0x7e3b :
			(color_depth == COLORDEPTH_30B) ? 0x78ea :
			(color_depth == COLORDEPTH_36B) ? 0x63a6 :
			(color_depth == COLORDEPTH_48B) ? 0x63a6 : 0x7e3b;
		csc_scale = 1;
	} else { /* input_color_format == COLORSPACE_RGB444 */
		csc_coeff_a1 = 0x2591;
		csc_coeff_a2 = 0x1322;
		csc_coeff_a3 = 0x074b;
		csc_coeff_a4 = 0x0000;
		csc_coeff_b1 = 0x6535;
		csc_coeff_b2 = 0x2000;
		csc_coeff_b3 = 0x7acc;
		csc_coeff_b4 = (color_depth == COLORDEPTH_24B) ? 0x0200 :
			(color_depth == COLORDEPTH_30B) ? 0x0800 :
			(color_depth == COLORDEPTH_36B) ? 0x2000 :
			(color_depth == COLORDEPTH_48B) ? 0x2000 : 0x0200;
		csc_coeff_c1 = 0x6acd;
		csc_coeff_c2 = 0x7534;
		csc_coeff_c3 = 0x2000;
		csc_coeff_c4 = (color_depth == COLORDEPTH_24B) ? 0x0200 :
			(color_depth == COLORDEPTH_30B) ? 0x0800 :
			(color_depth == COLORDEPTH_36B) ? 0x2000 :
			(color_depth == COLORDEPTH_48B) ? 0x2000 : 0x0200;
		csc_scale = 0;
	}
	} else {
		csc_coeff_a1 = 0x2000;
		csc_coeff_a2 = 0x0000;
		csc_coeff_a3 = 0x0000;
		csc_coeff_a4 = 0x0000;
		csc_coeff_b1 = 0x0000;
		csc_coeff_b2 = 0x2000;
		csc_coeff_b3 = 0x0000;
		csc_coeff_b4 = 0x0000;
		csc_coeff_c1 = 0x0000;
		csc_coeff_c2 = 0x0000;
		csc_coeff_c3 = 0x2000;
		csc_coeff_c4 = 0x0000;
		csc_scale = 1;
	}

	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_A1_MSB, (csc_coeff_a1 >> 8) & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_A1_LSB, csc_coeff_a1 & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_A2_MSB, (csc_coeff_a2 >> 8) & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_A2_LSB, csc_coeff_a2 & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_A3_MSB, (csc_coeff_a3 >> 8) & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_A3_LSB, csc_coeff_a3 & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_A4_MSB, (csc_coeff_a4 >> 8) & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_A4_LSB, csc_coeff_a4 & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_B1_MSB, (csc_coeff_b1 >> 8) & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_B1_LSB, csc_coeff_b1 & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_B2_MSB, (csc_coeff_b2 >> 8) & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_B2_LSB, csc_coeff_b2 & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_B3_MSB, (csc_coeff_b3 >> 8) & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_B3_LSB, csc_coeff_b3 & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_B4_MSB, (csc_coeff_b4 >> 8) & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_B4_LSB, csc_coeff_b4 & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_C1_MSB, (csc_coeff_c1 >> 8) & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_C1_LSB, csc_coeff_c1 & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_C2_MSB, (csc_coeff_c2 >> 8) & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_C2_LSB, csc_coeff_c2 & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_C3_MSB, (csc_coeff_c3 >> 8) & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_C3_LSB, csc_coeff_c3 & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_C4_MSB, (csc_coeff_c4 >> 8) & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_C4_LSB, csc_coeff_c4 & 0xff);

	data32 = 0;
	data32 |= (color_depth  << 4);  /* [7:4] csc_color_depth */
	data32 |= (csc_scale << 0);  /* [1:0] cscscale */
	hdmitx_wr_reg(HDMITX_DWC_CSC_SCALE, data32);

	/* set csc in video path */
	hdmitx_wr_reg(HDMITX_DWC_MC_FLOWCTRL, (conv_en == 1) ? 0x1 : 0x0);
}

static int hdmitx_cntl_config(struct hdmitx_hw_common *tx_hw, unsigned int cmd,
			      unsigned int argv)
{
	int ret = 0;
	unsigned int ieee_code = 0;
	struct hdmitx_dev *hdev = get_hdmitx_device();
	struct hdmitx20_hw *tx20_hw = to_hdmitx20_hw(tx_hw);

	if ((cmd & CMD_CONF_OFFSET) != CMD_CONF_OFFSET) {
		HDMITX_ERROR(HW "config: invalid cmd 0x%x\n", cmd);
		return -1;
	}

	switch (cmd) {
	case CONF_HDMI_DVI_MODE:
		hdmitx_hdmi_dvi_config(tx20_hw, (argv == DVI_MODE) ? 1 : 0);
		break;
	case CONF_GET_HDMI_DVI_MODE:
		ret = hdmitx_get_hdmi_dvi_config(hdev);
		break;
	case CONF_AUDIO_MUTE_OP:
		audio_mute_op(argv == AUDIO_MUTE ? 0 : 1);
		break;
	case CONF_GET_AUDIO_MUTE_ST:
		return audio_get_mute_st();
	case CONF_VIDEO_MUTE_OP:
		if (argv == VIDEO_MUTE) {
			if (hdev->tx_hw.chip_data->chip_type < MESON_CPU_ID_SC2)
				hd_set_reg_bits(P_HHI_GCLK_OTHER, 1, 3, 1);
			hd_set_reg_bits(P_ENCP_VIDEO_MODE_ADV, 0, 3, 1);
			hd_write_reg(P_VENC_VIDEO_TST_EN, 1);
			hd_write_reg(P_VENC_VIDEO_TST_MDSEL, 0);
			/*_Y/CB/CR, 10bits Unsigned/Signed/Signed */
			hd_write_reg(P_VENC_VIDEO_TST_Y, 0x0);
			hd_write_reg(P_VENC_VIDEO_TST_CB, 0x200);
			hd_write_reg(P_VENC_VIDEO_TST_CR, 0x200);
		}
		if (argv == VIDEO_UNMUTE) {
			hd_set_reg_bits(P_ENCP_VIDEO_MODE_ADV, 1, 3, 1);
			hd_write_reg(P_VENC_VIDEO_TST_EN, 0);
		}
		break;
	case CONF_CLR_AVI_PACKET:
		hdmitx_wr_reg(HDMITX_DWC_FC_AVIVID, 0);
		if (hdmitx_rd_reg(HDMITX_DWC_FC_VSDPAYLOAD0) == 0x20)
			hdmitx_wr_reg(HDMITX_DWC_FC_VSDPAYLOAD1, 0);
		hdmitx_set_isaformat(0);
		break;
	case CONF_CLR_AUDINFO_PACKET:
		break;
	case CONF_ASPECT_RATIO:
		HDMITX_INFO("%s argv = %d\n", __func__, argv);
		hdmitx_set_reg_bits(HDMITX_DWC_FC_AVICONF1, argv & 0x3, 4, 2);	//aspect_ratio
		hdmitx_set_reg_bits(HDMITX_DWC_FC_AVIVID, argv >> 2, 0, 7);	//vic
		break;
	case CONF_AVI_VIC:
		hdmitx_wr_reg(HDMITX_DWC_FC_AVIVID, argv);
		break;
	case CONF_AVI_BT2020:
		if (argv == SET_AVI_BT2020) {
			hdmitx_set_reg_bits(HDMITX_DWC_FC_AVICONF1, 3, 6, 2);
			hdmitx_set_reg_bits(HDMITX_DWC_FC_AVICONF2, 6, 4, 3);
		}
		if (argv == CLR_AVI_BT2020)
			hdmitx_set_avi_colorimetry(&hdev->tx_comm.fmt_para);
		break;
	case CONF_GET_AVI_BT2020:
		if (((hdmitx_rd_reg(HDMITX_DWC_FC_AVICONF1) & 0xC0) == 0xC0) &&
		    ((hdmitx_rd_reg(HDMITX_DWC_FC_AVICONF2) & 0x70) == 0x60))
			ret = HDMI_EXTENDED_COLORIMETRY_BT2020;
		else
			ret = 0;
		break;
	case CONF_CLR_DV_VS10_SIG:
/* if current is DV/VSIF.DOVI, next will switch to HDR, need set
 * Dolby_Vision_VS10_Signal_Type as 0
 */
		ieee_code = GET_IEEEOUI();
		if (ieee_code == DOVI_IEEEOUI) {
			hdmitx_set_reg_bits(HDMITX_DWC_FC_VSDPAYLOAD0, 0, 1, 1);
			return 1;
		} else {
			return 0;
		}
		break;
	case CONF_AVI_RGBYCC_INDIC:
		hdmitx_set_reg_bits(HDMITX_DWC_FC_AVICONF0, argv, 0, 2);
		hdmitx_set_reg_bits(HDMITX_DWC_FC_AVICONF0, 0, 7, 1);
		break;
	case CONF_AVI_Q01:
		hdmitx_set_reg_bits(HDMITX_DWC_FC_AVICONF2, argv, 2, 2);
		break;
	case CONF_AVI_YQ01:
		hdmitx_set_reg_bits(HDMITX_DWC_FC_AVICONF3, argv, 2, 2);
		break;
	case CONF_CT_MODE:
		if ((argv & 0xF) == SET_CT_OFF) {
			hdmitx_set_reg_bits(HDMITX_DWC_FC_AVICONF2, 0, 7, 1);
			hdmitx_set_reg_bits(HDMITX_DWC_FC_AVICONF3, 0, 0, 2);
		} else if ((argv & 0xF) == SET_CT_GAME) {
			hdmitx_set_reg_bits(HDMITX_DWC_FC_AVICONF2, 1, 7, 1);
			hdmitx_set_reg_bits(HDMITX_DWC_FC_AVICONF3, 3, 0, 2);
		} else if ((argv & 0xF) == SET_CT_GRAPHICS) {
			hdmitx_set_reg_bits(HDMITX_DWC_FC_AVICONF2, 1, 7, 1);
			hdmitx_set_reg_bits(HDMITX_DWC_FC_AVICONF3, 0, 0, 2);
		} else if ((argv & 0xF) == SET_CT_PHOTO) {
			hdmitx_set_reg_bits(HDMITX_DWC_FC_AVICONF2, 1, 7, 1);
			hdmitx_set_reg_bits(HDMITX_DWC_FC_AVICONF3, 1, 0, 2);
		} else if ((argv & 0xF) == SET_CT_CINEMA) {
			hdmitx_set_reg_bits(HDMITX_DWC_FC_AVICONF2, 1, 7, 1);
			hdmitx_set_reg_bits(HDMITX_DWC_FC_AVICONF3, 2, 0, 2);
		}
		break;
	case CONF_EMP_NUMBER:
		hdmitx_set_reg_bits(HDMITX_TOP_EMP_CNTL0, argv, 16, 16);
		if (argv > 0)/*enable*/
			hdmitx_set_reg_bits(HDMITX_TOP_EMP_CNTL0, 1, 0, 1);
		else
			hdmitx_set_reg_bits(HDMITX_TOP_EMP_CNTL0, 0, 0, 1);
		break;
	case CONF_EMP_PHY_ADDR:
		if (hdmitx_rd_check_reg(HDMITX_TOP_EMP_STAT0, 0, 0x7fffffff))
			hdmitx_set_reg_bits(HDMITX_TOP_EMP_STAT0, 1, 31, 1);
		if (hdmitx_rd_check_reg(HDMITX_TOP_EMP_STAT0, 0, 0xbfffffff))
			hdmitx_set_reg_bits(HDMITX_TOP_EMP_STAT0, 1, 30, 1);
		hdmitx_wr_reg(HDMITX_TOP_EMP_MEMADDR_START, argv);/*phys_ptr*/
		hdmitx_set_reg_bits(HDMITX_TOP_EMP_CNTL1, 1, 17, 1); /*little*/
		hdmitx_set_reg_bits(HDMITX_TOP_EMP_CNTL1, 120, 0, 16);
		break;
	case CONFIG_CSC_EN:
		if (argv == CSC_ENABLE)
			hdev->tx_comm.config_csc_en = true;
		if (argv == CSC_DISABLE)
			hdev->tx_comm.config_csc_en = false;
		break;
	case CONFIG_CSC:
		if (!hdev->tx_comm.config_csc_en)
			break;
		/* Y422,12bit to Y444,8bit */
		if ((argv & 0xF) == CSC_Y444_8BIT) {
			/* 1.vpu->encp */
			hd_set_reg_bits(P_VPU_HDMI_SETTING, 4, 5, 3);
			if (is_sync_polarity_negative(hdev->tx_comm.fmt_para.vic))
				hd_set_reg_bits(P_VPU_HDMI_SETTING, 0x0, 2, 2);
			else
				hd_set_reg_bits(P_VPU_HDMI_SETTING, 0x3, 2, 2);

			/* 2.1 encp no conversion HDMI format */
			hd_set_reg_bits(P_VPU_HDMI_FMT_CTRL, 0, 0, 2);
			/* 2.2dithering control */
			hdmitx_dith_ctrl(hdev);

			/* 3.vid input remap */
			hdmitx_in_vid_map(hdev->tx_comm.fmt_para.vic, COLORDEPTH_24B,
				HDMI_COLORSPACE_YUV444, HDMI_COLORSPACE_YUV444);

			/* 4.Video Packet YCC color remapping/configure */
			hdmitx_vp_conf(COLORDEPTH_24B, HDMI_COLORSPACE_YUV444);

			/* 5.whether update AVI colorspace */
			if (argv & CSC_UPDATE_AVI_CS)
				hdmitx_config_avi_cs(HDMI_COLORSPACE_YUV444);

			/* 6.update csc and output avi cs */
			hdmitx_pure_csc_config(HDMI_COLORSPACE_YUV444, HDMI_COLORSPACE_YUV444,
				COLORDEPTH_24B);
		} else if ((argv & 0xF) == CSC_Y422_12BIT) {
			/* 1.vpu->encp */
			hd_set_reg_bits(P_VPU_HDMI_SETTING, 0, 5, 3);
			/* sync pol is configured in hdmitx_dith_ctrl() */

			/* 2.1 encp no conversion HDMI format */
			hd_set_reg_bits(P_VPU_HDMI_FMT_CTRL, 1, 0, 2);
			/* 2.2dithering control */
			hdmitx_dith_ctrl(hdev);

			/* 3.vid input remap */
			hdmitx_in_vid_map(hdev->tx_comm.fmt_para.vic, COLORDEPTH_36B,
				HDMI_COLORSPACE_YUV444, HDMI_COLORSPACE_YUV422);

			/* 4.Video Packet YCC color remapping/configure */
			hdmitx_vp_conf(COLORDEPTH_36B, HDMI_COLORSPACE_YUV422);

			/* 5.whether update AVI colorspace */
			if (argv & CSC_UPDATE_AVI_CS)
				hdmitx_config_avi_cs(HDMI_COLORSPACE_YUV422);

			/* 6.update csc and output avi cs */
			hdmitx_pure_csc_config(HDMI_COLORSPACE_YUV444, HDMI_COLORSPACE_YUV422,
				COLORDEPTH_36B);
		} else if ((argv & 0xF) == CSC_RGB_8BIT) {
			/* 1.vpu->encp */
			hd_set_reg_bits(P_VPU_HDMI_SETTING, 4, 5, 3);
			if (is_sync_polarity_negative(hdev->tx_comm.fmt_para.vic))
				hd_set_reg_bits(P_VPU_HDMI_SETTING, 0x0, 2, 2);
			else
				hd_set_reg_bits(P_VPU_HDMI_SETTING, 0x3, 2, 2);

			/* 2.1 encp no conversion HDMI format */
			hd_set_reg_bits(P_VPU_HDMI_FMT_CTRL, 0, 0, 2);
			/* 2.2 dithering control */
			hdmitx_dith_ctrl(hdev);

			/* 3.vid input remap */
			hdmitx_in_vid_map(hdev->tx_comm.fmt_para.vic, COLORDEPTH_24B,
			HDMI_COLORSPACE_YUV444, HDMI_COLORSPACE_RGB);

			/* 4.Video Packet YCC color remapping/configure */
			hdmitx_vp_conf(COLORDEPTH_24B, HDMI_COLORSPACE_RGB);

			/* 5.whether update AVI colorspace */
			if (argv & CSC_UPDATE_AVI_CS)
				hdmitx_config_avi_cs(HDMI_COLORSPACE_RGB);

			/* 6.update csc and output avi cs */
			hdmitx_pure_csc_config(HDMI_COLORSPACE_YUV444, HDMI_COLORSPACE_RGB,
				COLORDEPTH_24B);
		} else {
			HDMITX_INFO("csc not support/implemented yet\n");
		}
		break;
	default:
		break;
	}

	return ret;
}

#define RXSEN_LOW_CNT 5
static int hdmitx_tmds_rxsense(void)
{
	unsigned int curr0, curr3;
	int ret = 0;
	static int filter_value;
	static int count;
	struct hdmitx_dev *hdev = get_hdmitx_device();

	switch (hdev->tx_hw.chip_data->chip_type) {
	case MESON_CPU_ID_G12A:
	case MESON_CPU_ID_G12B:
		hd_set_reg_bits(P_HHI_HDMI_PHY_CNTL0, 1, 16, 1);
		hd_set_reg_bits(P_HHI_HDMI_PHY_CNTL3, 1, 23, 1);
		hd_set_reg_bits(P_HHI_HDMI_PHY_CNTL3, 0, 24, 1);
		hd_set_reg_bits(P_HHI_HDMI_PHY_CNTL3, 3, 20, 3);
		ret = hd_read_reg(P_HHI_HDMI_PHY_CNTL2) & 0x1;
		break;
	case MESON_CPU_ID_SC2:
		hd_set_reg_bits(P_ANACTRL_HDMIPHY_CTRL0, 1, 16, 1);
		hd_set_reg_bits(P_ANACTRL_HDMIPHY_CTRL3, 1, 23, 1);
		hd_set_reg_bits(P_ANACTRL_HDMIPHY_CTRL3, 0, 24, 1);
		hd_set_reg_bits(P_ANACTRL_HDMIPHY_CTRL3, 3, 20, 3);
		ret = hd_read_reg(P_ANACTRL_HDMIPHY_CTRL2) & 0x1;
		break;
	case MESON_CPU_ID_GXBB:
		curr0 = hd_read_reg(P_HHI_HDMI_PHY_CNTL0);
		curr3 = hd_read_reg(P_HHI_HDMI_PHY_CNTL3);
		if (curr0 == 0)
			hd_write_reg(P_HHI_HDMI_PHY_CNTL0, 0x33632122);
		hd_set_reg_bits(P_HHI_HDMI_PHY_CNTL3, 0x9a, 16, 8);
		ret = hd_read_reg(P_HHI_HDMI_PHY_CNTL2) & 0x1;
		hd_write_reg(P_HHI_HDMI_PHY_CNTL3, curr3);
		if (curr0 == 0)
			hd_write_reg(P_HHI_HDMI_PHY_CNTL0, 0);
		break;
	case MESON_CPU_ID_GXL:
	case MESON_CPU_ID_GXM:
	default:
		curr0 = hd_read_reg(P_HHI_HDMI_PHY_CNTL0);
		curr3 = hd_read_reg(P_HHI_HDMI_PHY_CNTL3);
		if (curr0 == 0) {
			hd_write_reg(P_HHI_HDMI_PHY_CNTL0, 0x333d3282);
			hd_write_reg(P_HHI_HDMI_PHY_CNTL3, 0x2136315b);
			hd_set_reg_bits(P_HHI_HDMI_PHY_CNTL3, 0x1, 20, 1);
		}
		ret = hd_read_reg(P_HHI_HDMI_PHY_CNTL2) & 0x1;
		hd_write_reg(P_HHI_HDMI_PHY_CNTL0, curr0);
		hd_write_reg(P_HHI_HDMI_PHY_CNTL3, curr3);
		break;
	}
	if (!(hdmitx_hw_cntl_misc(&hdev->tx_hw.base, MISC_HPD_GPI_ST, 0)))
		return 0;
	if (ret == 0) {
		count++;
	} else {
		filter_value = 1;
		count = 0;
	}
	if (count >= RXSEN_LOW_CNT)
		filter_value = 0;
	return filter_value;
}

/*Check from SCDC Status_Flags_0/1 */
/* 0 means TMDS ok */
static int hdmitx_tmds_cedst(struct hdmitx_dev *hdev)
{
	return scdc_status_flags(hdev);
}

static int hdmitx_cntl_misc(struct hdmitx_hw_common *tx_hw, unsigned int cmd,
			    unsigned int argv)
{
	static int st;
	unsigned int pll_cntl = P_HHI_HDMI_PLL_CNTL;
	u8 rx_ver;
	struct hdmitx_dev *hdev = get_hdmitx_device();
	int chip_id = hdev->tx_hw.chip_data->chip_type;

	if ((cmd & CMD_MISC_OFFSET) != CMD_MISC_OFFSET) {
		HDMITX_ERROR(HW "misc: w: invalid cmd 0x%x\n", cmd);
		return -1;
	}

	switch (cmd) {
	case MISC_HPD_MUX_OP:
		if (argv == PIN_MUX)
			argv = HPD_MUX_HPD;
		else
			argv = HPD_UNMUX_HPD;
		return hdmitx_hpd_hw_op(argv);
	case MISC_HPD_GPI_ST:
		return hdmitx_hpd_hw_op(HPD_READ_HPD_GPIO);
	case MISC_TRIGGER_HPD:
		if (argv == 1)
			hdmitx_wr_reg(HDMITX_TOP_INTR_STAT, 1 << 1);
		else
			hdmitx_wr_reg(HDMITX_TOP_INTR_STAT, 1 << 2);
		return 0;
	case MISC_HPLL_FAKE:
		hdmitx_set_fake_vic(hdev);
		break;
	case MISC_TMDS_PHY_OP:
		if (argv == TMDS_PHY_ENABLE)
			hdmi_phy_wakeup(hdev);  /* TODO */
		if (argv == TMDS_PHY_DISABLE)
			hdmi_phy_suspend();
		break;
	case MISC_TMDS_RXSENSE:
		return hdmitx_tmds_rxsense();
	case MISC_TMDS_CEDST:
		return hdmitx_tmds_cedst(hdev);
	case MISC_ESM_RESET:
		if (hdev->hdcp_hpd_stick == 1) {
			HDMITX_INFO("hdcp: stick mode\n");
			break;
		}
		hdmitx_hdcp_opr(6);
		break;
	case MISC_VIID_IS_USING:
		break;
	case MISC_TMDS_CLK_DIV40:
		set_tmds_clk_div40(argv);
		break;
	case MISC_AVMUTE_OP:
		config_avmute(argv);
		break;
	case MISC_READ_AVMUTE_OP:
		return read_avmute();
	case MISC_HDCP_CLKDIS:
		if (st != !!argv) {
			st = !!argv;
			HDMITX_INFO("set hdcp clkdis: %d\n", !!argv);
		}
		if (chip_id >= MESON_CPU_ID_SC2)
			hdmitx_set_reg_bits(HDMITX_DWC_MC_CLKDIS_SC2,
					    !!argv, 6, 1);
		else
			hdmitx_set_reg_bits(HDMITX_DWC_MC_CLKDIS, !!argv, 6, 1);
		break;
	case MISC_I2C_RESET:
		if (chip_id >= MESON_CPU_ID_G12A) {
			hdmitx_set_reg_bits(HDMITX_TOP_SW_RESET, 1, 9, 1);
			usleep_range(1000, 2000);
			hdmitx_set_reg_bits(HDMITX_TOP_SW_RESET, 0, 9, 1);
			usleep_range(1000, 2000);
			hdmi_hwi_init(hdev);
		} else {
			hdmitx_hdcp_opr(4);
			hdmitx_set_reg_bits(HDMITX_DWC_A_HDCPCFG1, 0, 0, 1);
			hdmitx_set_reg_bits(HDMITX_DWC_HDCP22REG_CTRL, 0, 2, 1);
			hdmitx_wr_reg(HDMITX_DWC_I2CM_SS_SCL_HCNT_1, 0xff);
			hdmitx_wr_reg(HDMITX_DWC_I2CM_SS_SCL_HCNT_0, 0xf6);
			scdc_rd_sink(SINK_VER, &rx_ver);
			hdmi_hwi_init(hdev);
			mdelay(5);
		}
		break;
	case MISC_SUSFLAG:
		if (argv == 1) {
			hdcp22_susflag = 1;
			hdmitx_hdcp_opr(0xf);
		}
		if (argv == 0) {
			hdcp22_susflag = 0;
			hdmitx_hdcp_opr(0xf);
		}
		break;
	case MISC_AUDIO_RESET:
		 /* bit3: i2s_rst bit4: spdif_rst */
		hdmitx_wr_reg(HDMITX_DWC_MC_SWRSTZREQ, 0xe7);
		hdmitx_set_reg_bits(HDMITX_DWC_AUD_SPDIF0, 1, 7, 1);
		hdmitx_wr_reg(HDMITX_DWC_AUD_N1,
			      hdmitx_rd_reg(HDMITX_DWC_AUD_N1));
		udelay(1);
		hdmitx_wr_reg(HDMITX_DWC_MC_SWRSTZREQ, 0xe7);
		hdmitx_set_reg_bits(HDMITX_DWC_AUD_SPDIF0, 1, 7, 1);
		hdmitx_wr_reg(HDMITX_DWC_AUD_N1,
			      hdmitx_rd_reg(HDMITX_DWC_AUD_N1));
		udelay(1);
		break;
	case MISC_AUDIO_ACR_CTRL:
		if (argv == 0)
			hdmitx_set_reg_bits(HDMITX_DWC_FC_PACKET_TX_EN, 0, 0, 1);
		if (argv == 1)
			hdmitx_set_reg_bits(HDMITX_DWC_FC_PACKET_TX_EN, 1, 0, 1);
		break;
	case MISC_ESMCLK_CTRL:
		hdmitx_set_reg_bits(HDMITX_TOP_CLK_CNTL, !!argv, 6, 1);
		if (hdev->tx_hw.chip_data->chip_type >= MESON_CPU_ID_SC2)
			hd_set_reg_bits(P_CLKCTRL_HDCP22_CLK_CTRL, !!argv, 8, 1);
		else
			hd_set_reg_bits(P_HHI_HDCP22_CLK_CNTL, !!argv, 8, 1);
		break;
	case MISC_AUDIO_PREPARE:
		hdmitx_set_reg_bits(HDMITX_DWC_AUD_CONF0, 0, 0, 4);
		hdmitx_set_reg_bits(HDMITX_DWC_AUD_CONF0, 1, 5, 1);
		break;
	case MISC_DIS_HPLL:
		/* RESET set as 1, delay 50us, Enable set as 0 */
		/* G12A reset/enable bit position is different */
		if (chip_id >= MESON_CPU_ID_SC2)
			pll_cntl = P_ANACTRL_HDMIPLL_CTRL0;
		if (chip_id >= MESON_CPU_ID_G12A) {
			hd_set_reg_bits(pll_cntl, 1, 29, 1);
			usleep_range(49, 51);
			hd_set_reg_bits(pll_cntl, 0, 28, 1);
		} else {
			hd_set_reg_bits(pll_cntl, 1, 28, 1);
			usleep_range(49, 51);
			hd_set_reg_bits(pll_cntl, 0, 30, 1);
		}
		break;
	case MISC_HPD_IRQ_TOP_HALF:
		hdmitx_hpd_irq_top_half_process(hdev, !!argv);
		break;
	default:
		break;
	}
	return 1;
}

static enum hdmi_vic get_vic_from_pkt(void)
{
	enum hdmi_vic vic = HDMI_0_UNKNOWN;

	vic = hdmitx_rd_reg(HDMITX_DWC_FC_AVIVID);
	if (vic == HDMI_0_UNKNOWN) {
		vic = (enum hdmi_vic)hdmitx_rd_reg(HDMITX_DWC_FC_VSDPAYLOAD1);
		if (vic == 1)
			vic = HDMI_95_3840x2160p30_16x9;
		else if (vic == 2)
			vic = HDMI_94_3840x2160p25_16x9;
		else if (vic == 3)
			vic = HDMI_93_3840x2160p24_16x9;
		else if (vic == 4)
			vic = HDMI_98_4096x2160p24_256x135;
		else
			vic = hdmitx_get_isaformat();
	}

	return vic;
}

static enum hdmi_colorspace get_cs_from_pkt(void)
{
	/* Get uboot output color space from AVI */
	switch (hdmitx_rd_reg(HDMITX_DWC_FC_AVICONF0) & 0x3) {
	case 1:
		return HDMI_COLORSPACE_YUV422;
	case 2:
		return HDMI_COLORSPACE_YUV444;
	case 3:
		return HDMI_COLORSPACE_YUV420;
	default:
		return HDMI_COLORSPACE_RGB;
	}
}

static enum hdmi_color_depth get_cd_from_pkt(void)
{
	enum hdmi_colorspace cs = get_cs_from_pkt();
	enum hdmi_color_depth cd = COLORDEPTH_RESERVED;

	/* If color space is not 422, then get depth from VP_PR_CD */
	if (cs != HDMI_COLORSPACE_YUV422) {
		switch ((hdmitx_rd_reg(HDMITX_DWC_VP_PR_CD) >> 4) &
			0xf) {
		case 5:
			cd = COLORDEPTH_30B;
			break;
		case 6:
			cd = COLORDEPTH_36B;
			break;
		case 7:
			cd = COLORDEPTH_48B;
			break;
		case 0:
		case 4:
		default:
			cd = COLORDEPTH_24B;
			break;
		}
	} else {
		/* If colorspace is 422, then get depth from VP_REMAP */
		switch (hdmitx_rd_reg(HDMITX_DWC_VP_REMAP) & 0x3) {
		case 1:
			cd = COLORDEPTH_30B;
			break;
		case 2:
			cd = COLORDEPTH_36B;
			break;
		case 0:
		default:
			cd = COLORDEPTH_24B;
			break;
		}
	}

	return cd;
}

static int read_phy_status(struct hdmitx_hw_common *tx_hw)
{
	int phy_val = 0;
	struct hdmitx20_hw *tx20_hw = to_hdmitx20_hw(tx_hw);

	switch (tx20_hw->chip_data->chip_type) {
	case MESON_CPU_ID_SC2:
		phy_val = !!(hd_read_reg(P_ANACTRL_HDMIPHY_CTRL0) & 0xffff);
		break;
	case MESON_CPU_ID_TM2:
	case MESON_CPU_ID_TM2B:
		phy_val = !!(hd_read_reg(P_TM2_HHI_HDMI_PHY_CNTL0) & 0xffff);
		break;
	default:
		phy_val = !!(hd_read_reg(P_HHI_HDMI_PHY_CNTL0) & 0xffff);
		break;
	}
	return phy_val;
}

static int hdmitx_get_state(struct hdmitx_hw_common *tx_hw, unsigned int cmd,
			    unsigned int argv)
{
	if ((cmd & CMD_STAT_OFFSET) != CMD_STAT_OFFSET) {
		HDMITX_ERROR(HW "state: invalid cmd 0x%x\n", cmd);
		return -1;
	}

	switch (cmd) {
	case STAT_VIDEO_VIC:
		return (int)get_vic_from_pkt();
	case STAT_VIDEO_CS:
		return (int)get_cs_from_pkt();
	case STAT_VIDEO_CD:
		return (int)get_cd_from_pkt();
	case STAT_VIDEO_CLK:
		break;
	case STAT_HDR_TYPE:
		return hdmitx_rd_reg(HDMITX_DWC_FC_DRM_PB00) & 0xff;
	case STAT_TX_PHY:
		return read_phy_status(tx_hw);
	case STAT_TX_OUTPUT:
		return hdmitx_uboot_already_display();
	case STAT_TX_HDR:
		return hdmitx_get_cur_hdr_st();
	case STAT_TX_DV:
		return hdmitx_get_cur_dv_st();
	case STAT_TX_HDR10P:
		return hdmitx_get_cur_hdr10p_st();
	default:
		break;
	}
	return 0;
}

static void hdmi_phy_suspend(void)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();
	unsigned int phy_cntl0;
	unsigned int phy_cntl3;
	unsigned int phy_cntl5;

	switch (hdev->tx_hw.chip_data->chip_type) {
	case MESON_CPU_ID_SC2:
		phy_cntl0 = P_ANACTRL_HDMIPHY_CTRL0;
		phy_cntl3 = P_ANACTRL_HDMIPHY_CTRL3;
		phy_cntl5 = P_ANACTRL_HDMIPHY_CTRL5;
		break;
	case MESON_CPU_ID_TM2:
	case MESON_CPU_ID_TM2B:
		phy_cntl0 = P_TM2_HHI_HDMI_PHY_CNTL0;
		phy_cntl3 = P_TM2_HHI_HDMI_PHY_CNTL3;
		phy_cntl5 = P_TM2_HHI_HDMI_PHY_CNTL5;
		break;
	default:
		phy_cntl0 = P_HHI_HDMI_PHY_CNTL0;
		phy_cntl3 = P_HHI_HDMI_PHY_CNTL3;
		phy_cntl5 = P_HHI_HDMI_PHY_CNTL5;
		break;
	}
	hd_write_reg(phy_cntl0, 0x0);
	/* keep PHY_CNTL3 bit[1:0] as 0b11,
	 * otherwise may cause HDCP22 boot failed
	 */
	hd_write_reg(phy_cntl3, 0x3);
	hd_write_reg(phy_cntl5, 0x800);
}

static void hdmi_phy_wakeup(struct hdmitx_dev *hdev)
{
	hdmitx_set_phy(hdev);
}

void hdmitx_set_avi_colorimetry(struct hdmi_format_para *para)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();

	if (!para)
		return;

	/* set Colorimetry in AVIInfo */
	switch (para->vic) {
	case HDMI_2_720x480p60_4x3:
	case HDMI_3_720x480p60_16x9:
	case HDMI_6_720x480i60_4x3:
	case HDMI_7_720x480i60_16x9:
	case HDMI_8_720x240p60_4x3:
	case HDMI_9_720x240p60_16x9:
	case HDMI_10_2880x480i60_4x3:
	case HDMI_11_2880x480i60_16x9:
	case HDMI_12_2880x240p60_4x3:
	case HDMI_13_2880x240p60_16x9:
	case HDMI_14_1440x480p60_4x3:
	case HDMI_15_1440x480p60_16x9:
	case HDMI_17_720x576p50_4x3:
	case HDMI_18_720x576p50_16x9:
	case HDMI_21_720x576i50_4x3:
	case HDMI_22_720x576i50_16x9:
	case HDMI_23_720x288p_4x3:
	case HDMI_24_720x288p_16x9:
	case HDMI_25_2880x576i50_4x3:
	case HDMI_26_2880x576i50_16x9:
	case HDMI_27_2880x288p50_4x3:
	case HDMI_28_2880x288p50_16x9:
	case HDMI_29_1440x576p_4x3:
	case HDMI_30_1440x576p_16x9:
	case HDMI_35_2880x480p60_4x3:
	case HDMI_36_2880x480p60_16x9:
	case HDMI_37_2880x576p50_4x3:
	case HDMI_38_2880x576p50_16x9:
	case HDMI_42_720x576p100_4x3:
	case HDMI_43_720x576p100_16x9:
	case HDMI_44_720x576i100_4x3:
	case HDMI_45_720x576i100_16x9:
	case HDMI_48_720x480p120_4x3:
	case HDMI_49_720x480p120_16x9:
	case HDMI_50_720x480i120_4x3:
	case HDMI_51_720x480i120_16x9:
	case HDMI_52_720x576p200_4x3:
	case HDMI_53_720x576p200_16x9:
	case HDMI_54_720x576i200_4x3:
	case HDMI_55_720x576i200_16x9:
	case HDMI_56_720x480p240_4x3:
	case HDMI_57_720x480p240_16x9:
	case HDMI_58_720x480i240_4x3:
	case HDMI_59_720x480i240_16x9:
		/* C1C0 601 */
		hdmitx_set_reg_bits(HDMITX_DWC_FC_AVICONF1, 1, 6, 2);
		hdmitx_set_reg_bits(HDMITX_DWC_FC_AVICONF2, 0, 4, 3);
		break;
	default:
		if (hdev->colormetry) {
			hdmitx_set_reg_bits(HDMITX_DWC_FC_AVICONF1, 3, 6, 2);
			hdmitx_set_reg_bits(HDMITX_DWC_FC_AVICONF2, 6, 4, 3);
		} else {
			/* C1C0 709 */
			hdmitx_set_reg_bits(HDMITX_DWC_FC_AVICONF1, 2, 6, 2);
			hdmitx_set_reg_bits(HDMITX_DWC_FC_AVICONF2, 0, 4, 3);
		}
		break;
	}
}

/*
 * color_depth: Pixel bit width: 4=24-bit; 5=30-bit; 6=36-bit; 7=48-bit.
 * input_color_format: 0=RGB444; 1=YCbCr422; 2=YCbCr444; 3=YCbCr420.
 * input_color_range: 0=limited; 1=full.
 * output_color_format: 0=RGB444; 1=YCbCr422; 2=YCbCr444; 3=YCbCr420
 */
static void config_hdmi20_tx(enum hdmi_vic vic,
			     struct hdmitx_dev *hdev,
			     unsigned char color_depth,
			     unsigned char input_color_format,
			     unsigned char output_color_format)
{
	struct hdmi_format_para *para = &hdev->tx_comm.fmt_para;
	struct hdmi_timing *t = &para->timing;
	unsigned long   data32;
	unsigned char   vid_map;
	unsigned char   csc_en;
	unsigned char   default_phase = 0;
	unsigned int tmp = 0;
	unsigned short v_active = t->v_active;
	struct hdmitx20_hw *tx_hw = &hdev->tx_hw;

	if (t->pi_mode == 0)
		v_active = v_active >> 1;

	/* Enable hdmitx_sys_clk */
	hdmitx_set_cts_sys_clk(tx_hw);

	/* Enable clk81_hdmitx_pclk */
	hdmitx_set_top_pclk(tx_hw);

	/* Bring out of reset */
	hdmitx_wr_reg(HDMITX_TOP_SW_RESET,  0);

	/* Enable internal pixclk, tmds_clk, spdif_clk, i2s_clk, cecclk */
	hdmitx_set_reg_bits(HDMITX_TOP_CLK_CNTL, 3, 0, 2);
	hdmitx_set_reg_bits(HDMITX_TOP_CLK_CNTL, 3, 4, 2);
	hdmitx_wr_reg(HDMITX_DWC_MC_LOCKONCLOCK, 0xff);

/* But keep spdif_clk and i2s_clk disable
 * until later enable by test.c
 */
	data32  = 0;
	hdmitx_wr_reg(HDMITX_DWC_MC_CLKDIS, data32);

	/* Enable normal output to PHY */
	data32  = 0;
	data32 |= (1 << 12);
	data32 |= (0 << 8);
	data32 |= (0 << 0);
	hdmitx_wr_reg(HDMITX_TOP_BIST_CNTL, data32);

	/* Configure video */
	if (input_color_format == HDMI_COLORSPACE_RGB) {
		if (color_depth == COLORDEPTH_24B)
			vid_map = 0x01;
		else if (color_depth == COLORDEPTH_30B)
			vid_map = 0x03;
		else if (color_depth == COLORDEPTH_36B)
			vid_map = 0x05;
		else
			vid_map = 0x07;
	} else if (((input_color_format == HDMI_COLORSPACE_YUV444) ||
		(input_color_format == HDMI_COLORSPACE_YUV420)) &&
		(output_color_format != HDMI_COLORSPACE_YUV422)) {
		if (color_depth == COLORDEPTH_24B)
			vid_map = 0x09;
		else if (color_depth == COLORDEPTH_30B)
			vid_map = 0x0b;
		else if (color_depth == COLORDEPTH_36B)
			vid_map = 0x0d;
		else
			vid_map = 0x0f;
	} else {
		if (color_depth == COLORDEPTH_24B)
			vid_map = 0x16;
		else if (color_depth == COLORDEPTH_30B)
			vid_map = 0x14;
		else
			vid_map = 0x12;
	}

	switch (para->vic) {
	case HDMI_6_720x480i60_4x3:
	case HDMI_7_720x480i60_16x9:
	case HDMI_10_2880x480i60_4x3:
	case HDMI_11_2880x480i60_16x9:
	case HDMI_21_720x576i50_4x3:
	case HDMI_22_720x576i50_16x9:
	case HDMI_25_2880x576i50_4x3:
	case HDMI_26_2880x576i50_16x9:
	case HDMI_44_720x576i100_4x3:
	case HDMI_45_720x576i100_16x9:
	case HDMI_50_720x480i120_4x3:
	case HDMI_51_720x480i120_16x9:
	case HDMI_54_720x576i200_4x3:
	case HDMI_55_720x576i200_16x9:
	case HDMI_58_720x480i240_4x3:
	case HDMI_59_720x480i240_16x9:
		if (output_color_format == HDMI_COLORSPACE_YUV422) {
			if (color_depth == COLORDEPTH_24B)
				vid_map = 0x09;
			if (color_depth == COLORDEPTH_30B)
				vid_map = 0x0b;
			if (color_depth == COLORDEPTH_36B)
				vid_map = 0x0d;
		}
		break;
	default:
		break;
	}

	data32  = 0;
	data32 |= (0 << 7);
	data32 |= (vid_map << 0);
	hdmitx_wr_reg(HDMITX_DWC_TX_INVID0, data32);

	data32  = 0;
	data32 |= (0 << 2);
	data32 |= (0 << 1);
	data32 |= (0 << 0);
	hdmitx_wr_reg(HDMITX_DWC_TX_INSTUFFING, data32);
	hdmitx_wr_reg(HDMITX_DWC_TX_GYDATA0, 0x00);
	hdmitx_wr_reg(HDMITX_DWC_TX_GYDATA1, 0x00);
	hdmitx_wr_reg(HDMITX_DWC_TX_RCRDATA0, 0x00);
	hdmitx_wr_reg(HDMITX_DWC_TX_RCRDATA1, 0x00);
	hdmitx_wr_reg(HDMITX_DWC_TX_BCBDATA0, 0x00);
	hdmitx_wr_reg(HDMITX_DWC_TX_BCBDATA1, 0x00);

	/* Configure Color Space Converter */
	csc_en  = (input_color_format != output_color_format) ? 1 : 0;

	data32  = 0;
	data32 |= (csc_en   << 0);
	hdmitx_wr_reg(HDMITX_DWC_MC_FLOWCTRL, data32);

	data32  = 0;
	data32 |= ((((input_color_format == HDMI_COLORSPACE_YUV422) &&
		(output_color_format != HDMI_COLORSPACE_YUV422)) ? 2 : 0) << 4);
	hdmitx_wr_reg(HDMITX_DWC_CSC_CFG, data32);
	hdmitx_csc_config(input_color_format, output_color_format, color_depth);

	/* Configure video packetizer */

	/* Video Packet color depth and pixel repetition */
	data32  = 0;
	data32 |= (((output_color_format == HDMI_COLORSPACE_YUV422) ?
		COLORDEPTH_24B : color_depth) << 4);
	data32 |= (0 << 0);
	/* HDMI1.4 CTS7-19, CD of GCP for Y422 should be 0 */
	if ((data32 & 0xf0) == 0x40)
		data32 &= ~(0xf << 4);
	hdmitx_wr_reg(HDMITX_DWC_VP_PR_CD, data32);

	/* Video Packet Stuffing */
	data32  = 0;
	data32 |= (default_phase << 5);
	data32 |= (0 << 2);
	data32 |= (0 << 1);
	data32 |= (0 << 0);
	hdmitx_wr_reg(HDMITX_DWC_VP_STUFF,  data32);

	/* Video Packet YCC color remapping */
	data32  = 0;
	data32 |= (((color_depth == COLORDEPTH_30B) ? 1 :
		(color_depth == COLORDEPTH_36B) ? 2 : 0) << 0);
	hdmitx_wr_reg(HDMITX_DWC_VP_REMAP, data32);
	if (output_color_format == HDMI_COLORSPACE_YUV422) {
		switch (color_depth) {
		case COLORDEPTH_36B:
			tmp = 2;
			break;
		case COLORDEPTH_30B:
			tmp = 1;
			break;
		case COLORDEPTH_24B:
			tmp = 0;
			break;
		}
	}
	/* [1:0] ycc422_size */
	hdmitx_set_reg_bits(HDMITX_DWC_VP_REMAP, tmp, 0, 2);

	/* Video Packet configuration */
	data32  = 0;
	data32 |= ((((output_color_format != HDMI_COLORSPACE_YUV422) &&
		 (color_depth == COLORDEPTH_24B)) ? 1 : 0) << 6);
	data32 |= ((((output_color_format == HDMI_COLORSPACE_YUV422) ||
		 (color_depth == COLORDEPTH_24B)) ? 0 : 1) << 5);
	data32 |= (0 << 4);
	data32 |= (((output_color_format == HDMI_COLORSPACE_YUV422) ? 1 : 0)
		<< 3);
	data32 |= (1 << 2);
	data32 |= (((output_color_format == HDMI_COLORSPACE_YUV422) ? 1 :
		(color_depth == COLORDEPTH_24B) ? 2 : 0) << 0);
	hdmitx_wr_reg(HDMITX_DWC_VP_CONF, data32);

	data32  = 0;
	data32 |= (1 << 7);
	data32 |= (1 << 6);
	data32 |= (1 << 5);
	data32 |= (1 << 4);
	data32 |= (1 << 3);
	data32 |= (1 << 2);
	data32 |= (1 << 1);
	data32 |= (1 << 0);
	hdmitx_wr_reg(HDMITX_DWC_VP_MASK, data32);

	/* Configure audio */
	/* I2S Sampler config */

	data32  = 0;
	data32 |= (1 << 3);
	data32 |= (1 << 2);
	hdmitx_wr_reg(HDMITX_DWC_AUD_INT, data32);

	data32  = 0;
	data32 |= (1 << 4);
	hdmitx_wr_reg(HDMITX_DWC_AUD_INT1,  data32);

	hdmitx_wr_reg(HDMITX_DWC_FC_MULTISTREAM_CTRL, 0);

	data32  = 0;
	data32 |= (0 << 5);
	data32 |= (24   << 0);
	hdmitx_wr_reg(HDMITX_DWC_AUD_CONF1, data32);

	data32  = 0;
	data32 |= (0 << 1);
	data32 |= (0 << 0);
	hdmitx_wr_reg(HDMITX_DWC_AUD_CONF2, data32);

	/* spdif sampler config */

	data32  = 0;
	data32 |= (1 << 3);
	data32 |= (1 << 2);
	hdmitx_wr_reg(HDMITX_DWC_AUD_SPDIFINT,  data32);

	data32  = 0;
	data32 |= (0 << 4);
	hdmitx_wr_reg(HDMITX_DWC_AUD_SPDIFINT1, data32);

	data32  = 0;
	data32 |= (0 << 7);
	hdmitx_wr_reg(HDMITX_DWC_AUD_SPDIF0,	data32);

	data32  = 0;
	data32 |= (0 << 7);
	data32 |= (0 << 6);
	data32 |= (24 << 0);
	hdmitx_wr_reg(HDMITX_DWC_AUD_SPDIF1,	data32);

	/* Frame Composer configuration */

	/* Video definitions, as per output video(for packet gen/scheduling) */

	data32  = 0;
	data32 |= (1 << 7);
	data32 |= t->v_pol << 6;
	data32 |= t->h_pol << 5;
	data32 |= (1 << 4);
	data32 |= (1 << 3);
	data32 |= (!(t->pi_mode) << 1);
	data32 |= (!(t->pi_mode) << 0);
	hdmitx_wr_reg(HDMITX_DWC_FC_INVIDCONF,  data32);

	data32  = t->h_active & 0xff;
	hdmitx_wr_reg(HDMITX_DWC_FC_INHACTV0,   data32);
	data32  = (t->h_active >> 8) & 0x3f;
	hdmitx_wr_reg(HDMITX_DWC_FC_INHACTV1,   data32);

	data32  = t->h_blank & 0xff;
	hdmitx_wr_reg(HDMITX_DWC_FC_INHBLANK0,  data32);
	data32  = (t->h_blank >> 8) & 0x1f;
	hdmitx_wr_reg(HDMITX_DWC_FC_INHBLANK1,  data32);

	if (hdev->flag_3dfp) {
		data32 = v_active * 2 + t->v_blank;
		hdmitx_wr_reg(HDMITX_DWC_FC_INVACTV0, data32 & 0xff);
		hdmitx_wr_reg(HDMITX_DWC_FC_INVACTV1, (data32 >> 8) & 0x1f);
	} else {
		data32 = v_active & 0xff;
		hdmitx_wr_reg(HDMITX_DWC_FC_INVACTV0, data32);
		data32 = (v_active >> 8) & 0x1f;
		hdmitx_wr_reg(HDMITX_DWC_FC_INVACTV1, data32);
	}
	data32  = t->v_blank & 0xff;
	hdmitx_wr_reg(HDMITX_DWC_FC_INVBLANK,   data32);

	data32  = t->h_front & 0xff;
	hdmitx_wr_reg(HDMITX_DWC_FC_HSYNCINDELAY0,  data32);
	data32  = (t->h_front >> 8) & 0x1f;
	hdmitx_wr_reg(HDMITX_DWC_FC_HSYNCINDELAY1,  data32);

	data32  = t->h_sync & 0xff;
	hdmitx_wr_reg(HDMITX_DWC_FC_HSYNCINWIDTH0,  data32);
	data32  = (t->h_sync >> 8) & 0x3;
	hdmitx_wr_reg(HDMITX_DWC_FC_HSYNCINWIDTH1,  data32);

	data32  = t->v_front & 0xff;
	hdmitx_wr_reg(HDMITX_DWC_FC_VSYNCINDELAY,   data32);

	data32  = t->v_sync & 0x3f;
	hdmitx_wr_reg(HDMITX_DWC_FC_VSYNCINWIDTH,   data32);

	if (para->cs == HDMI_COLORSPACE_YUV420)
		mode420_half_horizontal_para();

	/* control period duration (typ 12 tmds periods) */
	hdmitx_wr_reg(HDMITX_DWC_FC_CTRLDUR,	12);
	/* extended control period duration (typ 32 tmds periods) */
	hdmitx_wr_reg(HDMITX_DWC_FC_EXCTRLDUR,  32);
	/* max interval betwen extended control period duration (typ 50) */
	hdmitx_wr_reg(HDMITX_DWC_FC_EXCTRLSPAC, 1);
	/* preamble filler */
	hdmitx_wr_reg(HDMITX_DWC_FC_CH0PREAM, 0x0b);
	hdmitx_wr_reg(HDMITX_DWC_FC_CH1PREAM, 0x16);
	hdmitx_wr_reg(HDMITX_DWC_FC_CH2PREAM, 0x21);

	/* write GCP packet configuration */
	data32  = 0;
	data32 |= (default_phase << 2);
	data32 |= (0 << 1);
	data32 |= (0 << 0);
	if (!global_tx_hw->base.hdcp_repeater_en)
		hdmitx_wr_reg(HDMITX_DWC_FC_GCP, data32);

	/* write AVI Infoframe packet configuration */
	data32  = 0;
	data32 |= (((output_color_format >> 2) & 0x1) << 7);
	data32 |= (1 << 6);
	/* underscan */
	data32 |= (2 << 4);
	data32 |= (0 << 2);
	data32 |= (0x2 << 0);    /* FIXED YCBCR 444 */
	hdmitx_wr_reg(HDMITX_DWC_FC_AVICONF0, data32);
	switch (output_color_format) {
	case HDMI_COLORSPACE_RGB:
		tmp = 0;
		break;
	case HDMI_COLORSPACE_YUV422:
		tmp = 1;
		break;
	case HDMI_COLORSPACE_YUV420:
		tmp = 3;
		break;
	case HDMI_COLORSPACE_YUV444:
	default:
		tmp = 2;
		break;
	}
	hdmitx_set_reg_bits(HDMITX_DWC_FC_AVICONF0, tmp, 0, 2);

	hdmitx_wr_reg(HDMITX_DWC_FC_AVICONF1, 0x8);
	hdmitx_wr_reg(HDMITX_DWC_FC_AVICONF2, 0);
	/* refer to 861-H, Page 83, A Source shall always explicitly signal
	 * the RGB Quantization Range as either Limited Range (Q=1)or Full
	 * Range (Q=2).
	 */
	if (output_color_format == HDMI_COLORSPACE_RGB)
		hdmitx_set_reg_bits(HDMITX_DWC_FC_AVICONF2, 1, 2, 2);

	/* set Aspect Ratio in AVIInfo */
	switch (para->vic) {
	case HDMI_2_720x480p60_4x3:
	case HDMI_6_720x480i60_4x3:
	case HDMI_8_720x240p60_4x3:
	case HDMI_10_2880x480i60_4x3:
	case HDMI_12_2880x240p60_4x3:
	case HDMI_14_1440x480p60_4x3:
	case HDMI_17_720x576p50_4x3:
	case HDMI_21_720x576i50_4x3:
	case HDMI_23_720x288p_4x3:
	case HDMI_25_2880x576i50_4x3:
	case HDMI_27_2880x288p50_4x3:
	case HDMI_29_1440x576p_4x3:
	case HDMI_35_2880x480p60_4x3:
	case HDMI_37_2880x576p50_4x3:
	case HDMI_42_720x576p100_4x3:
	case HDMI_44_720x576i100_4x3:
	case HDMI_48_720x480p120_4x3:
	case HDMI_50_720x480i120_4x3:
	case HDMI_52_720x576p200_4x3:
	case HDMI_54_720x576i200_4x3:
	case HDMI_56_720x480p240_4x3:
	case HDMI_58_720x480i240_4x3:
		/* Picture Aspect Ratio M1/M0 4:3 */
		hdmitx_set_reg_bits(HDMITX_DWC_FC_AVICONF1, 0x1, 4, 2);
		break;
	default:
		/* Picture Aspect Ratio M1/M0 16:9 */
		hdmitx_set_reg_bits(HDMITX_DWC_FC_AVICONF1, 0x2, 4, 2);
		break;
	}
	/* Active Format Aspect Ratio R3~R0 Same as picture aspect ratio */
	hdmitx_set_reg_bits(HDMITX_DWC_FC_AVICONF1, 0x8, 0, 4);

	hdmitx_set_avi_colorimetry(para);

	data32  = 0;
	data32 |= (((0 == HDMI_QUANTIZATION_RANGE_FULL) ? 1 : 0) << 2);
	data32 |= (0 << 0);
	hdmitx_wr_reg(HDMITX_DWC_FC_AVICONF3,   data32);

	hdmitx_wr_reg(HDMITX_DWC_FC_AVIVID, vic & HDMITX_VIC_MASK);

	/* For VESA modes, set VIC as 0 */
	if (para->vic >= HDMITX_VESA_OFFSET) {
		hdmitx_wr_reg(HDMITX_DWC_FC_AVIVID, 0);
		hdmitx_set_isaformat(para->vic);
	}

	/* write Audio Infoframe packet configuration */

	data32  = 0;
	data32 |= (1 << 4);
	data32 |= (0 << 0);
	hdmitx_wr_reg(HDMITX_DWC_FC_AUDICONF0,  data32);

	data32  = 0;
	data32 |= (3 << 4);
	data32 |= (0 << 0);
	hdmitx_wr_reg(HDMITX_DWC_FC_AUDICONF1, data32);

	hdmitx_wr_reg(HDMITX_DWC_FC_AUDICONF2, 0x00);

	data32  = 0;
	data32 |= (1 << 5);
	data32 |= (0 << 4);
	data32 |= (0 << 0);
	hdmitx_wr_reg(HDMITX_DWC_FC_AUDICONF3,  data32);

	/* write audio packet configuration */
	data32  = 0;
	data32 |= (0 << 4);
	data32 |= (0 << 0);
	hdmitx_wr_reg(HDMITX_DWC_FC_AUDSCONF, data32);

/* the audio setting bellow are only used for I2S audio IEC60958-3 frame
 * insertion
 */
	data32  = 0;
	data32 |= (0 << 7);
	data32 |= (0 << 6);
	data32 |= (0 << 5);
	data32 |= (1 << 4);
	data32 |= (0 << 3);
	data32 |= (0 << 2);
	data32 |= (0 << 1);
	data32 |= (1 << 0);
	hdmitx_wr_reg(HDMITX_DWC_FC_AUDSV,  data32);

	hdmitx_wr_reg(HDMITX_DWC_FC_AUDSU,  0);

	hdmitx_wr_reg(HDMITX_DWC_FC_AUDSCHNLS0, 0x01);
	hdmitx_wr_reg(HDMITX_DWC_FC_AUDSCHNLS1, 0x23);
	hdmitx_wr_reg(HDMITX_DWC_FC_AUDSCHNLS2, 0x45);
	hdmitx_wr_reg(HDMITX_DWC_FC_AUDSCHNLS3, 0x67);
	hdmitx_wr_reg(HDMITX_DWC_FC_AUDSCHNLS4, 0x89);
	hdmitx_wr_reg(HDMITX_DWC_FC_AUDSCHNLS5, 0xab);
	hdmitx_wr_reg(HDMITX_DWC_FC_AUDSCHNLS6, 0xcd);
	hdmitx_wr_reg(HDMITX_DWC_FC_AUDSCHNLS7, 0x2f);
	hdmitx_wr_reg(HDMITX_DWC_FC_AUDSCHNLS8, 0xf0);

	/* packet queue priority (auto mode) */
	hdmitx_wr_reg(HDMITX_DWC_FC_CTRLQHIGH,  15);
	hdmitx_wr_reg(HDMITX_DWC_FC_CTRLQLOW, 3);

	/* packet scheduler configuration for SPD, VSD, ISRC1/2, ACP. */
	hdmitx_set_reg_bits(HDMITX_DWC_FC_DATAUTO0, 0, 0, 3);
	hdmitx_set_reg_bits(HDMITX_DWC_FC_DATAUTO0, 0, 4, 4);
	hdmitx_wr_reg(HDMITX_DWC_FC_DATAUTO1, 0);
	hdmitx_wr_reg(HDMITX_DWC_FC_DATMAN, 0);

	/* packet scheduler configuration for AVI, GCP, AUDI, ACR. */
	hdmitx_set_reg_bits(HDMITX_DWC_FC_DATAUTO3, 0xe, 0, 6);

	hdmitx_set_reg_bits(HDMITX_DWC_FC_DATAUTO3, 0, 6, 1);
	hdmitx_set_reg_bits(HDMITX_DWC_FC_PACKET_TX_EN, 0, 7, 1);

	/* If RX  support 3D, then enable 3D send out */
	if (hdev->flag_3dfp || hdev->flag_3dtb || hdev->flag_3dss) {
		hdmitx_set_reg_bits(HDMITX_DWC_FC_DATAUTO0, 1, 3, 1);
		hdmitx_set_reg_bits(HDMITX_DWC_FC_PACKET_TX_EN, 1, 4, 1);
	} else {
	  /* after changing mode, dv will call vsif function again*/
		hdmitx_set_reg_bits(HDMITX_DWC_FC_DATAUTO0, 0, 3, 1);
		hdmitx_set_reg_bits(HDMITX_DWC_FC_PACKET_TX_EN, 0, 4, 1);
	}

	hdmitx_wr_reg(HDMITX_DWC_FC_RDRB0,  0);
	hdmitx_wr_reg(HDMITX_DWC_FC_RDRB1,  0);
	hdmitx_wr_reg(HDMITX_DWC_FC_RDRB2,  0);
	hdmitx_wr_reg(HDMITX_DWC_FC_RDRB3,  0);
	hdmitx_wr_reg(HDMITX_DWC_FC_RDRB4,  0);
	hdmitx_wr_reg(HDMITX_DWC_FC_RDRB5,  0);
	hdmitx_wr_reg(HDMITX_DWC_FC_RDRB6, 0x0);
	hdmitx_wr_reg(HDMITX_DWC_FC_RDRB7, 0x0);
	hdmitx_wr_reg(HDMITX_DWC_FC_RDRB8,  0);
	hdmitx_wr_reg(HDMITX_DWC_FC_RDRB9,  0);
	hdmitx_wr_reg(HDMITX_DWC_FC_RDRB10, 0);
	hdmitx_wr_reg(HDMITX_DWC_FC_RDRB11, 0);

	/* Packet transmission enable */
	hdmitx_set_reg_bits(HDMITX_DWC_FC_PACKET_TX_EN, 1, 1, 1);
	hdmitx_set_reg_bits(HDMITX_DWC_FC_PACKET_TX_EN, 1, 2, 1);

	/* For 3D video */
	data32  = 0;
	data32 |= (0 << 1);
	data32 |= (0 << 0);
	hdmitx_wr_reg(HDMITX_DWC_FC_ACTSPC_HDLR_CFG, data32);

	data32  = v_active & 0xff;
	hdmitx_wr_reg(HDMITX_DWC_FC_INVACT_2D_0,	data32);
	data32  = (v_active >> 8) & 0xf;
	hdmitx_wr_reg(HDMITX_DWC_FC_INVACT_2D_1,	data32);

	/* Do not enable these interrupt below, we can check them at RX side. */
	data32  = 0;
	data32 |= (1 << 7);
	data32 |= (1 << 6);
	data32 |= (1 << 5);
	data32 |= (1 << 2);
	data32 |= (1 << 1);
	data32 |= (1 << 0);
	hdmitx_wr_reg(HDMITX_DWC_FC_MASK0,  data32);

	data32  = 0;
	data32 |= (1 << 7);
	data32 |= (1 << 6);
	data32 |= (1 << 5);
	data32 |= (1 << 4);
	data32 |= (1 << 3);
	data32 |= (1 << 1);
	data32 |= (1 << 0);
	hdmitx_wr_reg(HDMITX_DWC_FC_MASK1,  data32);

	data32  = 0;
	data32 |= (1 << 2);
	data32 |= (1 << 1);
	data32 |= (1 << 0);
	hdmitx_wr_reg(HDMITX_DWC_FC_MASK2,  data32);

	/* Pixel repetition ratio the input and output video */
	data32  = 0;
	data32 |= ((t->pixel_repetition_factor + 1) << 4);
	data32 |= (t->pixel_repetition_factor << 0);
	hdmitx_wr_reg(HDMITX_DWC_FC_PRCONF, data32);

	/* Configure HDCP */
	data32  = 0;
	data32 |= (0 << 7);
	data32 |= (0 << 6);
	data32 |= (0 << 4);
	data32 |= (0 << 3);
	data32 |= (0 << 2);
	data32 |= (0 << 1);
	data32 |= (1 << 0);
	hdmitx_wr_reg(HDMITX_DWC_A_APIINTMSK, data32);

	data32  = 0;
	data32 |= (0 << 5);
	data32 |= (1 << 4);
	/* Set hsync/vsync polarity to solve the
	 * problem of timing continuing change
	 */
	if (t->h_pol)
		data32 |= (1 << 1);
	if (t->v_pol)
		data32 |= (1 << 3);
	hdmitx_wr_reg(HDMITX_DWC_A_VIDPOLCFG,   data32);

	hdmitx_wr_reg(HDMITX_DWC_A_OESSWCFG,    0x40);
	hdmitx_hdcp_opr(0);
	/* Interrupts */
	/* Clear interrupts */
	hdmitx_wr_reg(HDMITX_DWC_IH_FC_STAT0,  0xff);
	hdmitx_wr_reg(HDMITX_DWC_IH_FC_STAT1,  0xff);
	hdmitx_wr_reg(HDMITX_DWC_IH_FC_STAT2,  0xff);
	hdmitx_wr_reg(HDMITX_DWC_IH_AS_STAT0,  0xff);
	hdmitx_wr_reg(HDMITX_DWC_IH_PHY_STAT0, 0xff);
	hdmitx_wr_reg(HDMITX_DWC_IH_I2CM_STAT0,	0xff);
	hdmitx_wr_reg(HDMITX_DWC_IH_CEC_STAT0, 0xff);
	hdmitx_wr_reg(HDMITX_DWC_IH_VP_STAT0,  0xff);
	hdmitx_wr_reg(HDMITX_DWC_IH_I2CMPHY_STAT0, 0xff);
	hdmitx_wr_reg(HDMITX_DWC_A_APIINTCLR,  0xff);
	hdmitx_wr_reg(HDMITX_DWC_HDCP22REG_STAT, 0xff);

	hdmitx_wr_reg(HDMITX_TOP_INTR_STAT_CLR,	0x0000001f);

	/* Selectively enable/mute interrupt sources */
	data32  = 0;
	data32 |= (1 << 7);
	data32 |= (1 << 6);
	data32 |= (1 << 5);
	data32 |= (1 << 4);
	data32 |= (1 << 3);
	data32 |= (1 << 2);
	data32 |= (1 << 1);
	data32 |= (1 << 0);
	hdmitx_wr_reg(HDMITX_DWC_IH_MUTE_FC_STAT0,  data32);

	data32  = 0;
	data32 |= (1 << 7);
	data32 |= (1 << 6);
	data32 |= (1 << 5);
	data32 |= (1 << 4);
	data32 |= (1 << 3);
	data32 |= (1 << 2);
	data32 |= (1 << 1);
	data32 |= (1 << 0);
	hdmitx_wr_reg(HDMITX_DWC_IH_MUTE_FC_STAT1,  data32);

	data32  = 0;
	data32 |= (1 << 1);
	data32 |= (1 << 0);
	hdmitx_wr_reg(HDMITX_DWC_IH_MUTE_FC_STAT2,  data32);

	data32  = 0;
	data32 |= (0 << 4);
	data32 |= (0 << 3);
	data32 |= (1 << 2);
	data32 |= (1 << 1);
	data32 |= (1 << 0);
	hdmitx_wr_reg(HDMITX_DWC_IH_MUTE_AS_STAT0,  data32);

	hdmitx_wr_reg(HDMITX_DWC_IH_MUTE_PHY_STAT0, 0x3f);

	data32  = 0;
	data32 |= (0 << 2);
	data32 |= (1 << 1);
	data32 |= (0 << 0);
	hdmitx_wr_reg(HDMITX_DWC_IH_MUTE_I2CM_STAT0, data32);

	data32  = 0;
	data32 |= (0 << 6);
	data32 |= (0 << 5);
	data32 |= (0 << 4);
	data32 |= (0 << 3);
	data32 |= (0 << 2);
	data32 |= (0 << 1);
	data32 |= (0 << 0);
	hdmitx_wr_reg(HDMITX_DWC_IH_MUTE_CEC_STAT0, data32);

	hdmitx_wr_reg(HDMITX_DWC_IH_MUTE_VP_STAT0,  0xff);

	hdmitx_wr_reg(HDMITX_DWC_IH_MUTE_I2CMPHY_STAT0, 0x03);

	data32  = 0;
	data32 |= (0 << 1);
	data32 |= (0 << 0);
	hdmitx_wr_reg(HDMITX_DWC_IH_MUTE, data32);

	data32  = 0;
	data32 |= (1 << 4);
	data32 |= (1 << 3);
	data32 |= (1 << 2);
	data32 |= (1 << 1);
	data32 |= (1 << 0);
	hdmitx_wr_reg(HDMITX_TOP_INTR_MASKN, data32);

	/* Reset pulse */
	hdmitx_rd_check_reg(HDMITX_DWC_MC_LOCKONCLOCK, 0xff, 0x9f);

	hdmitx_wr_reg(HDMITX_DWC_MC_CLKDIS, 0xdf);
	hdmitx_wr_reg(HDMITX_DWC_MC_SWRSTZREQ, 0);
	mdelay(10);

	data32  = 0;
	data32 |= (1 << 7);
	data32 |= (1 << 6);
	data32 |= (1 << 4);
	data32 |= (1 << 3);
	data32 |= (1 << 2);
	data32 |= (0 << 1);
	data32 |= (1 << 0);
	hdmitx_wr_reg(HDMITX_DWC_MC_SWRSTZREQ, data32);
	hdmitx_wr_reg(HDMITX_DWC_FC_VSYNCINWIDTH,
		      hdmitx_rd_reg(HDMITX_DWC_FC_VSYNCINWIDTH));

	hdmitx_wr_reg(HDMITX_DWC_MC_CLKDIS, 0);
	/* hd_write_reg(P_ENCP_VIDEO_EN, 1); */ /* enable it finally */
} /* config_hdmi20_tx */

static void hdmitx_csc_config(unsigned char input_color_format,
			      unsigned char output_color_format,
			      unsigned char color_depth)
{
	unsigned char conv_en;
	unsigned char csc_scale;
	unsigned char rgb_ycc_indicator;
	unsigned long csc_coeff_a1, csc_coeff_a2, csc_coeff_a3, csc_coeff_a4;
	unsigned long csc_coeff_b1, csc_coeff_b2, csc_coeff_b3, csc_coeff_b4;
	unsigned long csc_coeff_c1, csc_coeff_c2, csc_coeff_c3, csc_coeff_c4;
	unsigned long data32;

	conv_en = (((input_color_format  == HDMI_COLORSPACE_RGB) ||
		(output_color_format == HDMI_COLORSPACE_RGB)) &&
		(input_color_format  != output_color_format)) ? 1 : 0;

	if (conv_en) {
		if (output_color_format == HDMI_COLORSPACE_RGB) {
			csc_coeff_a1 = 0x2000;
			csc_coeff_a2 = 0x6926;
			csc_coeff_a3 = 0x74fd;
			csc_coeff_a4 = (color_depth == COLORDEPTH_24B) ?
				0x010e :
			(color_depth == COLORDEPTH_30B) ? 0x043b :
			(color_depth == COLORDEPTH_36B) ? 0x10ee :
			(color_depth == COLORDEPTH_48B) ? 0x10ee : 0x010e;
		csc_coeff_b1 = 0x2000;
		csc_coeff_b2 = 0x2cdd;
		csc_coeff_b3 = 0x0000;
		csc_coeff_b4 = (color_depth == COLORDEPTH_24B) ? 0x7e9a :
			(color_depth == COLORDEPTH_30B) ? 0x7a65 :
			(color_depth == COLORDEPTH_36B) ? 0x6992 :
			(color_depth == COLORDEPTH_48B) ? 0x6992 : 0x7e9a;
		csc_coeff_c1 = 0x2000;
		csc_coeff_c2 = 0x0000;
		csc_coeff_c3 = 0x38b4;
		csc_coeff_c4 = (color_depth == COLORDEPTH_24B) ? 0x7e3b :
			(color_depth == COLORDEPTH_30B) ? 0x78ea :
			(color_depth == COLORDEPTH_36B) ? 0x63a6 :
			(color_depth == COLORDEPTH_48B) ? 0x63a6 : 0x7e3b;
		csc_scale = 1;
	} else { /* input_color_format == HDMI_COLORSPACE_RGB */
		csc_coeff_a1 = 0x2591;
		csc_coeff_a2 = 0x1322;
		csc_coeff_a3 = 0x074b;
		csc_coeff_a4 = 0x0000;
		csc_coeff_b1 = 0x6535;
		csc_coeff_b2 = 0x2000;
		csc_coeff_b3 = 0x7acc;
		csc_coeff_b4 = (color_depth == COLORDEPTH_24B) ? 0x0200 :
			(color_depth == COLORDEPTH_30B) ? 0x0800 :
			(color_depth == COLORDEPTH_36B) ? 0x2000 :
			(color_depth == COLORDEPTH_48B) ? 0x2000 : 0x0200;
		csc_coeff_c1 = 0x6acd;
		csc_coeff_c2 = 0x7534;
		csc_coeff_c3 = 0x2000;
		csc_coeff_c4 = (color_depth == COLORDEPTH_24B) ? 0x0200 :
			(color_depth == COLORDEPTH_30B) ? 0x0800 :
			(color_depth == COLORDEPTH_36B) ? 0x2000 :
			(color_depth == COLORDEPTH_48B) ? 0x2000 : 0x0200;
		csc_scale = 0;
	}
	} else {
		csc_coeff_a1 = 0x2000;
		csc_coeff_a2 = 0x0000;
		csc_coeff_a3 = 0x0000;
		csc_coeff_a4 = 0x0000;
		csc_coeff_b1 = 0x0000;
		csc_coeff_b2 = 0x2000;
		csc_coeff_b3 = 0x0000;
		csc_coeff_b4 = 0x0000;
		csc_coeff_c1 = 0x0000;
		csc_coeff_c2 = 0x0000;
		csc_coeff_c3 = 0x2000;
		csc_coeff_c4 = 0x0000;
		csc_scale = 1;
	}

	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_A1_MSB, (csc_coeff_a1 >> 8) & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_A1_LSB, csc_coeff_a1 & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_A2_MSB, (csc_coeff_a2 >> 8) & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_A2_LSB, csc_coeff_a2 & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_A3_MSB, (csc_coeff_a3 >> 8) & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_A3_LSB, csc_coeff_a3 & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_A4_MSB, (csc_coeff_a4 >> 8) & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_A4_LSB, csc_coeff_a4 & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_B1_MSB, (csc_coeff_b1 >> 8) & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_B1_LSB, csc_coeff_b1 & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_B2_MSB, (csc_coeff_b2 >> 8) & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_B2_LSB, csc_coeff_b2 & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_B3_MSB, (csc_coeff_b3 >> 8) & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_B3_LSB, csc_coeff_b3 & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_B4_MSB, (csc_coeff_b4 >> 8) & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_B4_LSB, csc_coeff_b4 & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_C1_MSB, (csc_coeff_c1 >> 8) & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_C1_LSB, csc_coeff_c1 & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_C2_MSB, (csc_coeff_c2 >> 8) & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_C2_LSB, csc_coeff_c2 & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_C3_MSB, (csc_coeff_c3 >> 8) & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_C3_LSB, csc_coeff_c3 & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_C4_MSB, (csc_coeff_c4 >> 8) & 0xff);
	hdmitx_wr_reg(HDMITX_DWC_CSC_COEF_C4_LSB, csc_coeff_c4 & 0xff);

	data32 = 0;
	data32 |= (color_depth  << 4);  /* [7:4] csc_color_depth */
	data32 |= (csc_scale << 0);  /* [1:0] cscscale */
	hdmitx_wr_reg(HDMITX_DWC_CSC_SCALE, data32);

	/* set csc in video path */
	hdmitx_wr_reg(HDMITX_DWC_MC_FLOWCTRL, (conv_en == 1) ? 0x1 : 0x0);

	/* set rgb_ycc indicator */
	switch (output_color_format) {
	case HDMI_COLORSPACE_RGB:
		rgb_ycc_indicator = 0x0;
		break;
	case HDMI_COLORSPACE_YUV422:
		rgb_ycc_indicator = 0x1;
		break;
	case HDMI_COLORSPACE_YUV420:
		rgb_ycc_indicator = 0x3;
		break;
	case HDMI_COLORSPACE_YUV444:
	default:
		rgb_ycc_indicator = 0x2;
		break;
	}
	hdmitx_set_reg_bits(HDMITX_DWC_FC_AVICONF0,
			    ((rgb_ycc_indicator & 0x4) >> 2), 7, 1);
	hdmitx_set_reg_bits(HDMITX_DWC_FC_AVICONF0,
			    (rgb_ycc_indicator & 0x3), 0, 2);
}   /* hdmitx_csc_config */

static void hdmitx_set_hw(struct hdmitx_dev *hdev)
{
	const struct hdmi_format_para *para = &hdev->tx_comm.fmt_para;

	HDMITX_INFO(" config hdmitx IP vic = %d cd:%d cs: %d\n",
		para->vic, para->cd, para->cs);

	config_hdmi20_tx(para->vic, hdev,
			para->cd,
			TX_INPUT_COLOR_FORMAT,
			para->cs);
}

struct hdmitx20_hw *get_hdmitx20_hw_instance(void)
{
	return global_tx_hw;
}

bool is_hdmi4k_support_420(enum hdmi_vic vic)
{
	if (vic == HDMI_102_4096x2160p60_256x135 ||
		vic == HDMI_101_4096x2160p50_256x135 ||
		vic == HDMI_97_3840x2160p60_16x9 ||
		vic == HDMI_96_3840x2160p50_16x9)
		return true;

	return false;
}

