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
#include <linux/spinlock.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/mm.h>
#include <linux/major.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/amlogic/media/frame_provider/tvin/tvin.h>
#include <linux/amlogic/media/vout/vdac_dev.h>
#include <linux/amlogic/media/video_sink/video.h>
#include <linux/arm-smccc.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/highmem.h>
#include <linux/amlogic/clk_measure.h>

/* Local include */
#include "hdmi_rx_eq.h"
#include "hdmi_rx_repeater.h"
#include "hdmi_rx_drv.h"
#include "hdmi_rx_hw.h"
#include "hdmi_rx_edid.h"
#include "hdmi_rx_wrapper.h"
#include "hdmi_rx_pktinfo.h"
#include "hdmi_rx_hw_t5m.h"
#include "hdmi_rx_hw_t3x.h"
#include "hdmi_rx_hw_txhd2.h"
#include "hdmi_rx_hw_t5.h"
#include "hdmi_rx_hw_t7.h"
#include "hdmi_rx_hw_tl1.h"
#include "hdmi_rx_hw_tm2.h"

/*------------------------marco define------------------------------*/
#define SCRAMBLE_SEL 1
#define HYST_HDMI_TO_DVI 5
/* must = 0, other agilent source fail */
#define HYST_DVI_TO_HDMI 0
#define GCP_GLOB_AVMUTE_EN 1 /* ag506 must clear this bit */
#define EDID_CLK_DIV 9 /* sys clk/(9+1) = 20M */
#define HDCP_KEY_WR_TRIES		(5)

/*------------------------variable define------------------------------*/
static DEFINE_SPINLOCK(reg_rw_lock);
/* should enable fast switching, since some devices in non-current port */
/* will suspend because of RxSense = 0, such as xiaomi-mtk box */
static bool phy_fast_switching;
static bool phy_fsm_enhancement = true;
/*u32 last_clk_rate;*/
static u32 modet_clk = 24000;
int hdcp_enc_mode;
/* top_irq_en bit[16:13] hdcp_sts */
/* bit27 DE rise */
int top_intr_maskn_value;
u32 afifo_overflow_cnt;
u32 afifo_underflow_cnt;
int rx_afifo_dbg_en;
bool hdcp_enable = 1;
int acr_mode;
int auto_aclk_mute = 2;
int aud_avmute_en = 1;
int aud_mute_sel = 2;
int force_clk_rate = 1;
u32 rx_ecc_err_thres = 100;
u32 rx_ecc_err_frames = 5;
int md_ists_en = VIDEO_MODE;
int pdec_ists_en;/* = AVI_CKS_CHG | DVIDET | DRM_CKS_CHG | DRM_RCV_EN;*/
u32 packet_fifo_cfg;
int pd_fifo_start_cnt = 0x8;
/* Controls equalizer reference voltage. */
int hdcp22_on;
int audio_debug = 1;
int edid_auto_debug;
int vpcore1_select = 1;
int clk_msr_param = 200;
int fpll_clk_sel;
MODULE_PARM_DESC(hdcp22_on, "\n hdcp22_on\n");
module_param(hdcp22_on, int, 0664);

/* 0: previous hdcp_rx22 ,1: new hdcp_rx22 */
int rx22_ver;
MODULE_PARM_DESC(rx22_ver, "\n rx22_ver\n");
module_param(rx22_ver, int, 0664);

MODULE_PARM_DESC(force_clk_rate, "\n force_clk_rate\n");
module_param(force_clk_rate, int, 0664);

/* test for HBR CTS, audio module can set it to force 8ch */
int hbr_force_8ch;
/*
 * hdcp14_key_mode:hdcp1.4 key handle method select
 * NORMAL_MODE:systemcontrol path
 * SECURE_MODE:secure OS path
 */
int hdcp14_key_mode = NORMAL_MODE;
int ignore_sscp_charerr = 1;
int ignore_sscp_tmds = 1;
int find_best_eq;
int eq_try_cnt = 20;
int pll_rst_max = 5;
/* cdr lock threshold */
int cdr_lock_level;
u32 term_cal_val;
bool term_cal_en;
int clock_lock_th = 2;
int scdc_force_en = 1;
/* for hdcp_hpd debug, disable by default */
u32 hdcp_hpd_ctrl_en;
int eq_dbg_lvl;
u32 phy_trim_val;
/* bit'4: tdr enable
 * bit [3:0]: tdr level control
 */
int phy_term_lel;
bool phy_tdr_en;
int kill_esm_fail;
int dbg_port;
/* emp buffer */
char emp_buf[2][1024];
char pre_emp_buf[2][1024];
int i2c_err_cnt[4];
u32 ddc_dbg_en;
int dual_port_en;
int force_clk_stable;//t3x frl todo

bool earc_hpd_low_flag;
/*------------------------variable define end------------------------------*/

static int check_regmap_flag(u32 addr)
{
	return 1;
}

/*
 * hdmirx_rd_dwc - Read data from HDMI RX CTRL
 * @addr: register address
 *
 * return data read value
 */
u32 hdmirx_rd_dwc(u32 addr)
{
	ulong flags;
	int data;
	unsigned long dev_offset = 0x10;

	if (!rx_get_dig_clk_en_sts())
		return 0;
	if (rx_info.chip_id >= CHIP_ID_TL1) {
		spin_lock_irqsave(&reg_rw_lock, flags);
		data = rd_reg(MAP_ADDR_MODULE_TOP,
			      addr + rx_reg_maps[MAP_ADDR_MODULE_TOP].phy_addr);
		spin_unlock_irqrestore(&reg_rw_lock, flags);
	} else {
		spin_lock_irqsave(&reg_rw_lock, flags);
		wr_reg(MAP_ADDR_MODULE_TOP,
		       hdmirx_addr_port | dev_offset, addr);
		data = rd_reg(MAP_ADDR_MODULE_TOP,
			      hdmirx_data_port | dev_offset);
		spin_unlock_irqrestore(&reg_rw_lock, flags);
	}
	return data;
}

/*
 * hdmirx_rd_bits_dwc - read specified bits of HDMI RX CTRL reg
 * @addr: register address
 * @mask: bits mask
 *
 * return masked bits of register value
 */
u32 hdmirx_rd_bits_dwc(u32 addr, u32 mask)
{
	return rx_get_bits(hdmirx_rd_dwc(addr), mask);
}

/*
 * hdmirx_wr_dwc - Write data to HDMI RX CTRL
 * @addr: register address
 * @data: new register value
 */
void hdmirx_wr_dwc(u32 addr, u32 data)
{
	ulong flags;
	u32 dev_offset = 0x10;

	if (!rx_get_dig_clk_en_sts())
		return;
	if (rx_info.chip_id >= CHIP_ID_TL1) {
		spin_lock_irqsave(&reg_rw_lock, flags);
		wr_reg(MAP_ADDR_MODULE_TOP,
		       addr + rx_reg_maps[MAP_ADDR_MODULE_TOP].phy_addr, data);
		spin_unlock_irqrestore(&reg_rw_lock, flags);
	} else {
		spin_lock_irqsave(&reg_rw_lock, flags);
		wr_reg(MAP_ADDR_MODULE_TOP,
		       hdmirx_addr_port | dev_offset, addr);
		wr_reg(MAP_ADDR_MODULE_TOP,
		       hdmirx_data_port | dev_offset, data);
		spin_unlock_irqrestore(&reg_rw_lock, flags);
	}
}

/*
 * hdmirx_wr_bits_dwc - write specified bits of HDMI RX CTRL reg
 * @addr: register address
 * @mask: bits mask
 * @value: new register value
 */
void hdmirx_wr_bits_dwc(u32 addr,
			u32 mask,
			u32 value)
{
	hdmirx_wr_dwc(addr, rx_set_bits(hdmirx_rd_dwc(addr), mask, value));
}

/*
 * hdmirx_rd_phy - Read data from HDMI RX phy
 * @addr: register address
 *
 * return data read value
 */
u32 hdmirx_rd_phy(u32 reg_address)
{
	int cnt = 0;

	/* hdmirx_wr_dwc(DWC_I2CM_PHYG3_SLAVE, 0x39); */
	hdmirx_wr_dwc(DWC_I2CM_PHYG3_ADDRESS, reg_address);
	hdmirx_wr_dwc(DWC_I2CM_PHYG3_OPERATION, 0x02);
	do {
		if ((cnt % 10) == 0) {
			/* wait i2cmpdone */
			if (hdmirx_rd_dwc(DWC_HDMI_ISTS) & (1 << 28)) {
				hdmirx_wr_dwc(DWC_HDMI_ICLR, 1 << 28);
				break;
			}
		}
		cnt++;
		if (cnt > 50000 && (log_level & VIDEO_LOG)) {
			rx_pr("[HDMIRX err]: %s(%x,%x) timeout\n",
			      __func__, 0x39, reg_address);
			break;
		}
	} while (1);

	return (u32)(hdmirx_rd_dwc(DWC_I2CM_PHYG3_DATAI));
}

/*
 * hdmirx_rd_bits_phy - read specified bits of HDMI RX phy reg
 * @addr: register address
 * @mask: bits mask
 *
 * return masked bits of register value
 */
u32 hdmirx_rd_bits_phy(u32 addr, u32 mask)
{
	return rx_get_bits(hdmirx_rd_phy(addr), mask);
}

/*
 * hdmirx_wr_phy - Write data to HDMI RX phy
 * @addr: register address
 * @data: new register value
 *
 * return 0 on write succeed, return -1 otherwise.
 */
u32 hdmirx_wr_phy(u32 reg_address, u32 data)
{
	int error = 0;
	int cnt = 0;

	/* hdmirx_wr_dwc(DWC_I2CM_PHYG3_SLAVE, 0x39); */
	hdmirx_wr_dwc(DWC_I2CM_PHYG3_ADDRESS, reg_address);
	hdmirx_wr_dwc(DWC_I2CM_PHYG3_DATAO, data);
	hdmirx_wr_dwc(DWC_I2CM_PHYG3_OPERATION, 0x01);

	do {
		/* wait i2cmpdone */
		if ((cnt % 10) == 0) {
			if (hdmirx_rd_dwc(DWC_HDMI_ISTS) & (1 << 28)) {
				hdmirx_wr_dwc(DWC_HDMI_ICLR, 1 << 28);
				break;
			}
		}
		cnt++;
		if (cnt > 50000) {
			error = -1;
			rx_pr("[err-%s]:(%x,%x)timeout\n",
			      __func__, reg_address, data);
			break;
		}
	} while (1);
	return error;
}

/*
 * hdmirx_wr_bits_phy - write specified bits of HDMI RX phy reg
 * @addr: register address
 * @mask: bits mask
 * @value: new register value
 *
 * return 0 on write succeed, return -1 otherwise.
 */
int hdmirx_wr_bits_phy(u16 addr, u32 mask, u32 value)
{
	return hdmirx_wr_phy(addr, rx_set_bits(hdmirx_rd_phy(addr),
			     mask, value));
}

/*
 * hdmirx_rd_top - read hdmirx top reg
 * @addr: register address
 *
 * return data read value
 */
u32 hdmirx_rd_top(u32 addr, u8 port)
{
	ulong flags;
	int data;
	u32 dev_offset = 0;

	//if (dbg_port)
		//port = dbg_port - 1;

	if (rx_info.chip_id >= CHIP_ID_TL1) {
		spin_lock_irqsave(&reg_rw_lock, flags);
		dev_offset = rx_reg_maps[MAP_ADDR_MODULE_TOP].phy_addr;
		if (rx_info.chip_id >= CHIP_ID_T3X) {
			if (addr >= TOP_EDID_ADDR_S &&
				addr <= TOP_EDID_PORT4_ADDR_E)
				dev_offset += TOP_EDID_OFFSET;
			else
				dev_offset += TOP_SINGLE_OFFSET_T3X +
					TOP_SINGLE_REG_RANGE * port;
		} else if (rx_info.chip_id < CHIP_ID_T7) {
			dev_offset += TOP_COMMON_OFFSET;
		}
		if (addr >= TOP_EDID_ADDR_S && addr <= TOP_EDID_PORT4_ADDR_E)
			data = rd_reg_b(MAP_ADDR_MODULE_TOP, dev_offset + addr);
		else
			data = rd_reg(MAP_ADDR_MODULE_TOP, dev_offset + (addr << 2));
		spin_unlock_irqrestore(&reg_rw_lock, flags);
	} else {
		spin_lock_irqsave(&reg_rw_lock, flags);
		wr_reg(MAP_ADDR_MODULE_TOP,
		       hdmirx_addr_port | dev_offset, addr);
		data = rd_reg(MAP_ADDR_MODULE_TOP,
			      hdmirx_data_port | dev_offset);
		spin_unlock_irqrestore(&reg_rw_lock, flags);
	}
	return data;
}

u32 hdmirx_rd_top_common(u32 addr)
{
	ulong flags;
	int data;
	u32 dev_offset = 0;

	if (addr >= TOP_EDID_ADDR_S &&
		addr <= TOP_EDID_PORT4_ADDR_E)
		rx_pr("err_%d\n", __LINE__);

	if (rx_info.chip_id >= CHIP_ID_TL1) {
		spin_lock_irqsave(&reg_rw_lock, flags);
		dev_offset = rx_reg_maps[MAP_ADDR_MODULE_TOP].phy_addr;
		if (rx_info.chip_id >= CHIP_ID_T3X)
			dev_offset += TOP_COMMON_OFFSET_T3X;
		else if (rx_info.chip_id < CHIP_ID_T7)
			dev_offset += TOP_COMMON_OFFSET;
		data = rd_reg(MAP_ADDR_MODULE_TOP, dev_offset + (addr << 2));
		spin_unlock_irqrestore(&reg_rw_lock, flags);
	} else {
		spin_lock_irqsave(&reg_rw_lock, flags);
		wr_reg(MAP_ADDR_MODULE_TOP,
		       hdmirx_addr_port | dev_offset, addr);
		data = rd_reg(MAP_ADDR_MODULE_TOP,
			      hdmirx_data_port | dev_offset);
		spin_unlock_irqrestore(&reg_rw_lock, flags);
	}
	return data;
}

u32 hdmirx_rd_top_common_1(u32 addr)
{
	ulong flags;
	int data;
	u32 dev_offset = 0;

	if (addr >= TOP_EDID_ADDR_S &&
		addr <= TOP_EDID_PORT4_ADDR_E)
		rx_pr("err_%d\n", __LINE__);

	if (rx_info.chip_id >= CHIP_ID_TL1) {
		spin_lock_irqsave(&reg_rw_lock, flags);
		dev_offset = rx_reg_maps[MAP_ADDR_MODULE_TOP].phy_addr;
		if (rx_info.chip_id >= CHIP_ID_T3X)
			dev_offset += TOP_COMMON_OFFSET_T3X + 0x400;
		else if (rx_info.chip_id < CHIP_ID_T7)
			dev_offset += TOP_COMMON_OFFSET;
		data = rd_reg(MAP_ADDR_MODULE_TOP, dev_offset + (addr << 2));
		spin_unlock_irqrestore(&reg_rw_lock, flags);
	} else {
		spin_lock_irqsave(&reg_rw_lock, flags);
		wr_reg(MAP_ADDR_MODULE_TOP,
		       hdmirx_addr_port | dev_offset, addr);
		data = rd_reg(MAP_ADDR_MODULE_TOP,
			      hdmirx_data_port | dev_offset);
		spin_unlock_irqrestore(&reg_rw_lock, flags);
	}
	return data;
}

/*
 * hdmirx_rd_bits_top - read specified bits of hdmirx top reg
 * @addr: register address
 * @mask: bits mask
 *
 * return masked bits of register value
 */
u32 hdmirx_rd_bits_top(u16 addr, u32 mask, u8 port)
{
	return rx_get_bits(hdmirx_rd_top(addr, port), mask);
}

u32 hdmirx_rd_bits_top_common(u16 addr, u32 mask)
{
	return rx_get_bits(hdmirx_rd_top_common(addr), mask);
}

u32 hdmirx_rd_bits_top_common_1(u16 addr, u32 mask)
{
	return rx_get_bits(hdmirx_rd_top_common_1(addr), mask);
}

/*
 * hdmirx_wr_top - Write data to hdmirx top reg
 * @addr: register address
 * @data: new register value
 */
void hdmirx_wr_top(u32 addr, u32 data, u8 port)
{
	ulong flags;
	unsigned long dev_offset = 0;

	//if (dbg_port)
		//port = dbg_port - 1;

	if (rx_info.chip_id >= CHIP_ID_TL1) {
		dev_offset = rx_reg_maps[MAP_ADDR_MODULE_TOP].phy_addr;
		spin_lock_irqsave(&reg_rw_lock, flags);
		if (rx_info.chip_id >= CHIP_ID_T3X) {
			if (addr >= TOP_EDID_ADDR_S &&
				addr <= TOP_EDID_PORT4_ADDR_E)
				dev_offset += TOP_EDID_OFFSET;
			else
				dev_offset += TOP_SINGLE_OFFSET_T3X +
					TOP_SINGLE_REG_RANGE * port;
		} else if (rx_info.chip_id < CHIP_ID_T7) {
			dev_offset += TOP_COMMON_OFFSET;
		}
		if (addr >= TOP_EDID_ADDR_S && addr <= TOP_EDID_PORT4_ADDR_E)
			wr_reg_b(MAP_ADDR_MODULE_TOP, dev_offset + addr, data);
		else
			wr_reg(MAP_ADDR_MODULE_TOP, dev_offset + (addr << 2), data);
		spin_unlock_irqrestore(&reg_rw_lock, flags);
	} else {
		spin_lock_irqsave(&reg_rw_lock, flags);
		wr_reg(MAP_ADDR_MODULE_TOP, hdmirx_addr_port | dev_offset, addr);
		wr_reg(MAP_ADDR_MODULE_TOP, hdmirx_data_port | dev_offset, data);
		spin_unlock_irqrestore(&reg_rw_lock, flags);
	}
}

void hdmirx_wr_top_common(u32 addr, u32 data)
{
	ulong flags;
	unsigned long dev_offset = 0;

	if (addr >= TOP_EDID_ADDR_S &&
		addr <= TOP_EDID_PORT4_ADDR_E)
		rx_pr("err_%d\n", __LINE__);

	if (rx_info.chip_id >= CHIP_ID_TL1) {
		dev_offset = rx_reg_maps[MAP_ADDR_MODULE_TOP].phy_addr;
		spin_lock_irqsave(&reg_rw_lock, flags);
		if (rx_info.chip_id >= CHIP_ID_T3X)
			dev_offset += TOP_COMMON_OFFSET_T3X;
		else if (rx_info.chip_id < CHIP_ID_T7)
			dev_offset += TOP_COMMON_OFFSET;
		wr_reg(MAP_ADDR_MODULE_TOP, dev_offset + (addr << 2), data);
		spin_unlock_irqrestore(&reg_rw_lock, flags);
	} else {
		spin_lock_irqsave(&reg_rw_lock, flags);
		wr_reg(MAP_ADDR_MODULE_TOP, hdmirx_addr_port | dev_offset, addr);
		wr_reg(MAP_ADDR_MODULE_TOP, hdmirx_data_port | dev_offset, data);
		spin_unlock_irqrestore(&reg_rw_lock, flags);
	}
}

void hdmirx_wr_top_common_1(u32 addr, u32 data)
{
	ulong flags;
	unsigned long dev_offset = 0;

	if (rx_info.chip_id < CHIP_ID_T3X)
		return;
	if (addr >= TOP_EDID_ADDR_S &&
		addr <= TOP_EDID_PORT4_ADDR_E)
		rx_pr("err_%d\n", __LINE__);

	if (rx_info.chip_id >= CHIP_ID_TL1) {
		dev_offset = rx_reg_maps[MAP_ADDR_MODULE_TOP].phy_addr;
		spin_lock_irqsave(&reg_rw_lock, flags);
		if (rx_info.chip_id >= CHIP_ID_T3X)
			dev_offset += TOP_COMMON_OFFSET_T3X + 0x400;
		else if (rx_info.chip_id < CHIP_ID_T7)
			dev_offset += TOP_COMMON_OFFSET;
		wr_reg(MAP_ADDR_MODULE_TOP, dev_offset + (addr << 2), data);
		spin_unlock_irqrestore(&reg_rw_lock, flags);
	} else {
		spin_lock_irqsave(&reg_rw_lock, flags);
		wr_reg(MAP_ADDR_MODULE_TOP, hdmirx_addr_port | dev_offset, addr);
		wr_reg(MAP_ADDR_MODULE_TOP, hdmirx_data_port | dev_offset, data);
		spin_unlock_irqrestore(&reg_rw_lock, flags);
	}
}

/*
 * hdmirx_wr_bits_top - write specified bits of hdmirx top reg
 * @addr: register address
 * @mask: bits mask
 * @value: new register value
 */
void hdmirx_wr_bits_top(u32 addr,
			u32 mask,
			u32 value, u8 port)
{
	hdmirx_wr_top(addr, rx_set_bits(hdmirx_rd_top(addr, port), mask, value), port);
}

void hdmirx_wr_bits_top_common(u32 addr,
			u32 mask,
			u32 value)
{
	hdmirx_wr_top_common(addr, rx_set_bits(hdmirx_rd_top_common(addr), mask, value));
}

void hdmirx_wr_bits_top_common_1(u32 addr,
			u32 mask,
			u32 value)
{
	hdmirx_wr_top_common_1(addr, rx_set_bits(hdmirx_rd_top_common_1(addr), mask, value));
}

//EDID W/R
//T3X base addr == single 0x8000
//Other chips base addr == common
void hdmirx_wr_edid(unsigned int addr, unsigned int data)
{
	ulong flags;
	unsigned long dev_offset = 0;

	if (rx_info.chip_id >= CHIP_ID_TL1) {
		spin_lock_irqsave(&reg_rw_lock, flags);
		dev_offset = rx_reg_maps[MAP_ADDR_MODULE_TOP].phy_addr;
		if (rx_info.chip_id >= CHIP_ID_T3X)
			dev_offset += TOP_EDID_OFFSET;
		else if (rx_info.chip_id < CHIP_ID_T7)
			dev_offset += TOP_COMMON_OFFSET;
		wr_reg_b(MAP_ADDR_MODULE_TOP, dev_offset + addr, data);
		spin_unlock_irqrestore(&reg_rw_lock, flags);
	} else {
		spin_lock_irqsave(&reg_rw_lock, flags);
		wr_reg(MAP_ADDR_MODULE_TOP, hdmirx_addr_port | dev_offset, addr);
		wr_reg(MAP_ADDR_MODULE_TOP, hdmirx_data_port | dev_offset, data);
		spin_unlock_irqrestore(&reg_rw_lock, flags);
	}
}

unsigned int hdmirx_rd_edid(unsigned int addr)
{
	ulong flags;
	int data;
	unsigned int dev_offset = 0;

	if (rx_info.chip_id >= CHIP_ID_TL1) {
		spin_lock_irqsave(&reg_rw_lock, flags);
		dev_offset = rx_reg_maps[MAP_ADDR_MODULE_TOP].phy_addr;
		if (rx_info.chip_id >= CHIP_ID_T3X)
			dev_offset += TOP_EDID_OFFSET;
		else if (rx_info.chip_id < CHIP_ID_T7)
			dev_offset += TOP_COMMON_OFFSET;
		data = rd_reg_b(MAP_ADDR_MODULE_TOP, dev_offset + addr);
		spin_unlock_irqrestore(&reg_rw_lock, flags);
	} else {
		spin_lock_irqsave(&reg_rw_lock, flags);
		wr_reg(MAP_ADDR_MODULE_TOP,
		       hdmirx_addr_port | dev_offset, addr);
		data = rd_reg(MAP_ADDR_MODULE_TOP,
			      hdmirx_data_port | dev_offset);
		spin_unlock_irqrestore(&reg_rw_lock, flags);
	}
	return data;
}

/*
 * hdmirx_rd_amlphy - read hdmirx amlphy reg
 * @addr: register address
 *
 * return data read value
 */
u32 hdmirx_rd_amlphy(u32 addr)
{
	ulong flags;
	int data;
	u32 dev_offset = 0;
	u32 base_ofst = 0;

	if (rx_info.chip_id >= CHIP_ID_T3X)
		return 0;
	else if (rx_info.chip_id >= CHIP_ID_T7)
		base_ofst = TOP_AMLPHY_BASE_OFFSET_T7;
	else
		base_ofst = TOP_AMLPHY_BASE_OFFSET_T5;
	spin_lock_irqsave(&reg_rw_lock, flags);
	dev_offset = base_ofst +
		rx_reg_maps[MAP_ADDR_MODULE_TOP].phy_addr;
	data = rd_reg(MAP_ADDR_MODULE_TOP, dev_offset + addr);
	spin_unlock_irqrestore(&reg_rw_lock, flags);
	return data;
}

u32 hdmirx_rd_amlphy_t3x(u32 addr, u8 port)
{
	ulong flags;
	int data;
	u32 dev_offset = 0;
	u32 base_ofst = 0;

	if (dbg_port)
		port = dbg_port - 1;

	if (rx_info.chip_id >= CHIP_ID_T3X)
		base_ofst = TOP_AMLPHY_BASE_OFFSET_T3X + T3X_PHY_OFFSET * port;

	spin_lock_irqsave(&reg_rw_lock, flags);
	dev_offset = base_ofst +
		rx_reg_maps[MAP_ADDR_MODULE_TOP].phy_addr;
	data = rd_reg(MAP_ADDR_MODULE_TOP, dev_offset + addr);
	spin_unlock_irqrestore(&reg_rw_lock, flags);
	return data;
}

u32 hdmirx_rd_bits_amlphy_t3x(u16 addr, u32 mask, u8 port)
{
	return rx_get_bits(hdmirx_rd_amlphy_t3x(addr, port), mask);
}

/*
 * hdmirx_rd_bits_amlphy - read specified bits of hdmirx amlphy reg
 * @addr: register address
 * @mask: bits mask
 *
 * return masked bits of register value
 */
u32 hdmirx_rd_bits_amlphy(u16 addr, u32 mask)
{
	return rx_get_bits(hdmirx_rd_amlphy(addr), mask);
}

/*
 * hdmirx_wr_amlphy - Write data to hdmirx amlphy reg
 * @addr: register address
 * @data: new register value
 */
void hdmirx_wr_amlphy(u32 addr, u32 data)
{
	ulong flags;
	unsigned long dev_offset = 0;
	u32 base_ofst = 0;
	if (rx_info.chip_id >= CHIP_ID_T3X)
		return;
	else if (rx_info.chip_id >= CHIP_ID_T7)
		base_ofst = TOP_AMLPHY_BASE_OFFSET_T7;
	else
		base_ofst = TOP_AMLPHY_BASE_OFFSET_T5;
	spin_lock_irqsave(&reg_rw_lock, flags);
	dev_offset = base_ofst +
		rx_reg_maps[MAP_ADDR_MODULE_TOP].phy_addr;
		wr_reg(MAP_ADDR_MODULE_TOP, dev_offset + addr, data);
	spin_unlock_irqrestore(&reg_rw_lock, flags);
}

void hdmirx_wr_amlphy_t3x(u32 addr, u32 data, u8 port)
{
	ulong flags;
	unsigned long dev_offset = 0;
	u32 base_ofst = 0;

	if (dbg_port)
		port = dbg_port - 1;

	if (rx_info.chip_id >= CHIP_ID_T3X)
		base_ofst = TOP_AMLPHY_BASE_OFFSET_T3X + T3X_PHY_OFFSET * port;
	spin_lock_irqsave(&reg_rw_lock, flags);
	dev_offset = base_ofst +
		rx_reg_maps[MAP_ADDR_MODULE_TOP].phy_addr;
		wr_reg(MAP_ADDR_MODULE_TOP, dev_offset + addr, data);
	spin_unlock_irqrestore(&reg_rw_lock, flags);
}

//void hdmirx_wr_phy(u32 addr, u32 data, u8 port)
//{
	//if (rx_info.chip_id >= CHIP_ID_T3X)
		//hdmirx_wr_amlphy_t3x(addr, data, port);
	//else if (rx_info.chip_id > CHIP_ID_TXLX)
		//hdmirx_wr_amlphy(addr, data);
	//else
		//hdmirx_wr_phy_txlx();
//}

/*
 * hdmirx_wr_bits_amlphy - write specified bits of hdmirx amlphy reg
 * @addr: register address
 * @mask: bits mask
 * @value: new register value
 */
void hdmirx_wr_bits_amlphy(u32 addr,
			   u32 mask,
			   u32 value)
{
	hdmirx_wr_amlphy(addr, rx_set_bits(hdmirx_rd_amlphy(addr), mask, value));
}

void hdmirx_wr_bits_amlphy_t3x(u32 addr, u32 mask, u32 value, u8 port)
{
	hdmirx_wr_amlphy_t3x(addr, rx_set_bits(hdmirx_rd_amlphy_t3x(addr, port),
		mask, value), port);
}

/* for T7 */
u8 hdmirx_rd_cor(u32 addr, u8 port)
{
	ulong flags;
	u8 data;
	u32 dev_offset = 0;
	bool need_wr_twice = false;

	if (dbg_port)
		port = dbg_port - 1;

	/* addr bit[8:15] is 0x1d or 0x1e need write twice */
	need_wr_twice = ((((addr >> 8) & 0xff) == 0x1d) ||
		(((addr >> 8) & 0xff) == 0x1e));
	if (rx_info.chip_id == CHIP_ID_TXHD2)
		dev_offset = rx_reg_maps[MAP_ADDR_MODULE_COR].phy_addr;
	else
		dev_offset = rx_reg_maps[MAP_ADDR_MODULE_TOP].phy_addr;

	if (rx_info.chip_id >= CHIP_ID_T3X) {
		dev_offset = rx_reg_maps[MAP_ADDR_MODULE_TOP].phy_addr;
		if ((addr >= 0x1800 && addr <= 0x1bff) ||
			(addr >= 0x1400 && addr <= 0x14ff)) {
			;//todo rd vp_core & audio
			if (rx_info.main_port == port)
				dev_offset -= 0x1000;
		} else {
			dev_offset += TOP_COR_BASE_OFFSET_T3X + TOP_COR_REG_RANGE * port;
		}
	} else {
		if (rx_info.chip_id != CHIP_ID_TXHD2)
			dev_offset += TOP_COR_BASE_OFFSET_T7;
	}
	spin_lock_irqsave(&reg_rw_lock, flags);
	if (rx_info.chip_id == CHIP_ID_TXHD2)
		data = rd_reg_b(MAP_ADDR_MODULE_COR, addr + dev_offset);
	else
		data = rd_reg_b(MAP_ADDR_MODULE_TOP, addr + dev_offset);
	if (need_wr_twice) {
		if (rx_info.chip_id == CHIP_ID_TXHD2)
			data = rd_reg_b(MAP_ADDR_MODULE_COR, addr + dev_offset);
		else
			data = rd_reg_b(MAP_ADDR_MODULE_TOP, addr + dev_offset);
	}
	spin_unlock_irqrestore(&reg_rw_lock, flags);

	return data;
}

u8 hdmirx_rd_bits_cor(u32 addr, u32 mask, u8 port)
{
	return rx_get_bits(hdmirx_rd_cor(addr, port), mask);
}

void hdmirx_wr_cor(u32 addr, u8 data, u8 port)
{
	ulong flags;
	u32 dev_offset = 0;
	bool need_wr_twice = false;

	if (dbg_port)
		port = dbg_port - 1;
	/* addr bit[8:15] is 0x1d or 0x1e need write twice */
	need_wr_twice = ((((addr >> 8) & 0xff) == 0x1d) ||
		(((addr >> 8) & 0xff) == 0x1e));
	if (rx_info.chip_id == CHIP_ID_TXHD2)
		dev_offset = rx_reg_maps[MAP_ADDR_MODULE_COR].phy_addr;
	else
		dev_offset = rx_reg_maps[MAP_ADDR_MODULE_TOP].phy_addr;
	if (rx_info.chip_id >= CHIP_ID_T3X) {
		if ((addr >= 0x1800 && addr <= 0x1bff) ||
			(addr >= 0x1400 && addr <= 0x14ff) ||
			addr == 0x1c48 || addr == 0x1c65 || addr == 0x1c4d) {
			;//todo rd vp_core & audio
			if (rx_info.main_port == port)
				dev_offset -= 0x1000;
		} else {
			dev_offset += TOP_COR_BASE_OFFSET_T3X + TOP_COR_REG_RANGE * port;
		}
	} else {
		if (rx_info.chip_id != CHIP_ID_TXHD2)
			dev_offset += TOP_COR_BASE_OFFSET_T7;
	}
	spin_lock_irqsave(&reg_rw_lock, flags);
	if (rx_info.chip_id == CHIP_ID_TXHD2)
		wr_reg_b(MAP_ADDR_MODULE_COR,
		addr + dev_offset, data);
	else
		wr_reg_b(MAP_ADDR_MODULE_TOP,
		addr + dev_offset, data);
	if (need_wr_twice) {
		if (rx_info.chip_id == CHIP_ID_TXHD2)
			wr_reg_b(MAP_ADDR_MODULE_COR,
				addr + dev_offset, data);
		else
			wr_reg_b(MAP_ADDR_MODULE_TOP,
				addr + dev_offset, data);
	}
	spin_unlock_irqrestore(&reg_rw_lock, flags);
}

void hdmirx_wr_bits_cor(u32 addr, u32 mask, u8 value, u8 port)
{
	hdmirx_wr_cor(addr, rx_set_bits(hdmirx_rd_cor(addr, port), mask, value), port);
}

bool hdmirx_flt_update_cleared_wait(u32 addr, u8 port)
{
	unsigned long timeout = jiffies + HZ / 10;

	if (log_level & FRL_LOG)
		rx_pr("before flt cor = 0x%x\n", hdmirx_rd_cor(addr, port));
	while (time_before(jiffies, timeout)) {
		if (hdmirx_rd_bits_cor(addr, _BIT(5), port) == 0)
			break;
	}
	if (log_level & FRL_LOG)
		rx_pr("after flt cor = 0x%x\n", hdmirx_rd_cor(addr, port));
	if (hdmirx_rd_bits_cor(addr, _BIT(5), port) == 0)
		return true;
	return false;
}

bool hdmirx_poll_cor(u32 addr, u8 exp_data, u8 mask, u32 max_try, u8 port)
{
	u8 rd_data;
	u32 cnt = 0;
	u8 done = 0;

	rd_data = hdmirx_rd_cor(addr, port);
	while (((cnt < max_try) || (max_try == 0)) && (done != 1)) {
		if ((rd_data | mask) == (exp_data | mask)) {
			done = 1;
		} else {
			cnt++;
			rd_data = hdmirx_rd_cor(addr, port);
		}
		udelay(5);
	}
	if (done == 0)
		rx_pr("hdmirx_poll_COR access time-out!\n");
	return done;
}

u32 rd_reg_clk_ctl(u32 offset)
{
	u32 ret;
	unsigned long flags;
	u32 addr;

	spin_lock_irqsave(&reg_rw_lock, flags);
	addr = offset + rx_reg_maps[MAP_ADDR_MODULE_CLK_CTRL].phy_addr;
	ret = rd_reg(MAP_ADDR_MODULE_CLK_CTRL, addr);
	spin_unlock_irqrestore(&reg_rw_lock, flags);
	return ret;
}

u32 hdmirx_rd_bits_clk_ctl(u32 addr, u32 mask)
{
	return rx_get_bits(rd_reg_clk_ctl(addr), mask);
}

void wr_reg_clk_ctl(u32 offset, u32 val)
{
	unsigned long flags;
	u32 addr;

	spin_lock_irqsave(&reg_rw_lock, flags);
	addr = offset + rx_reg_maps[MAP_ADDR_MODULE_CLK_CTRL].phy_addr;
	wr_reg(MAP_ADDR_MODULE_CLK_CTRL, addr, val);
	spin_unlock_irqrestore(&reg_rw_lock, flags);
}

void hdmirx_wr_bits_clk_ctl(u32 addr, u32 mask, u32 value)
{
	wr_reg_clk_ctl(addr, rx_set_bits(rd_reg_clk_ctl(addr), mask, value));
}

/* For analog modules register rd */
u32 rd_reg_ana_ctl(u32 offset)
{
	u32 ret;
	unsigned long flags;
	u32 addr;

	spin_lock_irqsave(&reg_rw_lock, flags);
	addr = offset + rx_reg_maps[MAP_ADDR_MODULE_ANA_CTRL].phy_addr;
	ret = rd_reg(MAP_ADDR_MODULE_ANA_CTRL, addr);
	spin_unlock_irqrestore(&reg_rw_lock, flags);
	return ret;
}

/* For analog modules register wr */
void wr_reg_ana_ctl(u32 offset, u32 val)
{
	unsigned long flags;
	u32 addr;

	spin_lock_irqsave(&reg_rw_lock, flags);
	addr = offset + rx_reg_maps[MAP_ADDR_MODULE_ANA_CTRL].phy_addr;
	wr_reg(MAP_ADDR_MODULE_ANA_CTRL, addr, val);
	spin_unlock_irqrestore(&reg_rw_lock, flags);
}

void wr_bits_reg_ana_ctl(u32 addr, u32 mask, u32 value)
{
	wr_reg_ana_ctl(addr, rx_set_bits(rd_reg_ana_ctl(addr), mask, value));
}

/*
 * rd_reg_hhi
 * @offset: offset address of hhi physical addr
 *
 * returns u32 bytes read from the addr
 */
u32 rd_reg_hhi(u32 offset)
{
	u32 ret;
	unsigned long flags;
	u32 addr;

	spin_lock_irqsave(&reg_rw_lock, flags);
	addr = offset + rx_reg_maps[MAP_ADDR_MODULE_HIU].phy_addr;
	ret = rd_reg(MAP_ADDR_MODULE_HIU, addr);
	spin_unlock_irqrestore(&reg_rw_lock, flags);
	return ret;
}

/*
 * rd_reg_hhi_bits - read specified bits of HHI reg
 * @addr: register address
 * @mask: bits mask
 *
 * return masked bits of register value
 */
u32 rd_reg_hhi_bits(u32 offset, u32 mask)
{
	return rx_get_bits(rd_reg_hhi(offset), mask);
}

/*
 * wr_reg_hhi
 * @offset: offset address of hhi physical addr
 * @val: value being written
 */
void wr_reg_hhi(u32 offset, u32 val)
{
	unsigned long flags;
	u32 addr;

	spin_lock_irqsave(&reg_rw_lock, flags);
	addr = offset + rx_reg_maps[MAP_ADDR_MODULE_HIU].phy_addr;
	wr_reg(MAP_ADDR_MODULE_HIU, addr, val);
	spin_unlock_irqrestore(&reg_rw_lock, flags);
}

/*
 * wr_reg_hhi_bits
 * @offset: offset address of hhi physical addr
 * @mask: modify bits mask
 * @val: value being written
 */
void wr_reg_hhi_bits(u32 offset, u32 mask, u32 val)
{
	wr_reg_hhi(offset, rx_set_bits(rd_reg_hhi(offset), mask, val));
}

/*
 * rd_reg - register read
 * @module: module index of the reg_map table
 * @reg_addr: offset address of specified phy addr
 *
 * returns u32 bytes read from the addr
 */
u32 rd_reg(enum map_addr_module_e module,
		    u32 reg_addr)
{
	u32 val = 0;

	if (module < MAP_ADDR_MODULE_NUM && check_regmap_flag(reg_addr))
		val = readl(rx_reg_maps[module].p +
			    (reg_addr - rx_reg_maps[module].phy_addr));
	else
		rx_pr("rd reg %x error,md %d\n", reg_addr, module);
	return val;
}

/*
 * wr_reg - register write
 * @module: module index of the reg_map table
 * @reg_addr: offset address of specified phy addr
 * @val: value being written
 */
void wr_reg(enum map_addr_module_e module,
	    u32 reg_addr, u32 val)
{
	if (module < MAP_ADDR_MODULE_NUM && check_regmap_flag(reg_addr))
		writel(val, rx_reg_maps[module].p +
		       (reg_addr - rx_reg_maps[module].phy_addr));
	else
		rx_pr("wr reg %x err\n", reg_addr);
}

/*
 * rd_reg_b - register read byte mode
 * @module: module index of the reg_map table
 * @reg_addr: offset address of specified phy addr
 *
 * returns unsigned char bytes read from the addr
 */
unsigned char rd_reg_b(enum map_addr_module_e module,
		       u32 reg_addr)
{
	unsigned char val = 0;

	if (module < MAP_ADDR_MODULE_NUM && check_regmap_flag(reg_addr))
		val = readb(rx_reg_maps[module].p +
			    (reg_addr - rx_reg_maps[module].phy_addr));
	else
		rx_pr("rd reg %x error,md %d\n", reg_addr, module);
	return val;
}

/*
 * wr_reg_b - register write byte mode
 * @module: module index of the reg_map table
 * @reg_addr: offset address of specified phy addr
 * @val: value being written
 */
void wr_reg_b(enum map_addr_module_e module,
	      u32 reg_addr, unsigned char val)
{
	if (module < MAP_ADDR_MODULE_NUM && check_regmap_flag(reg_addr))
		writeb(val, rx_reg_maps[module].p +
		       (reg_addr - rx_reg_maps[module].phy_addr));
	else
		rx_pr("wr reg %x err\n", reg_addr);
}

/*
 * rx_hdcp22_wr_only
 */
void rx_hdcp22_wr_only(u32 addr, u32 data)
{
	unsigned long flags;

	spin_lock_irqsave(&reg_rw_lock, flags);
	wr_reg(MAP_ADDR_MODULE_HDMIRX_CAPB3,
	       rx_reg_maps[MAP_ADDR_MODULE_HDMIRX_CAPB3].phy_addr | addr,
	data);
	spin_unlock_irqrestore(&reg_rw_lock, flags);
}

u32 rx_hdcp22_rd(u32 addr)
{
	u32 data;
	unsigned long flags;

	spin_lock_irqsave(&reg_rw_lock, flags);
	data = rd_reg(MAP_ADDR_MODULE_HDMIRX_CAPB3,
		      rx_reg_maps[MAP_ADDR_MODULE_HDMIRX_CAPB3].phy_addr | addr);
	spin_unlock_irqrestore(&reg_rw_lock, flags);
	return data;
}

void rx_hdcp22_rd_check(u32 addr,
			u32 exp_data,
			u32 mask)
{
	u32 rd_data;

	rd_data = rx_hdcp22_rd(addr);
	if ((rd_data | mask) != (exp_data | mask))
		rx_pr("addr=0x%02x rd_data=0x%08x\n", addr, rd_data);
}

void rx_hdcp22_wr(u32 addr, u32 data)
{
	rx_hdcp22_wr_only(addr, data);
	rx_hdcp22_rd_check(addr, data, 0);
}

/*
 * rx_hdcp22_rd_reg - hdcp2.2 reg write
 * @addr: register address
 * @value: new register value
 */
void rx_hdcp22_wr_reg(u32 addr, u32 data)
{
	rx_sec_reg_write((u32 *)(unsigned long)
		(rx_reg_maps[MAP_ADDR_MODULE_HDMIRX_CAPB3].phy_addr + addr),
		data);
}

/*
 * rx_hdcp22_rd_reg - hdcp2.2 reg read
 * @addr: register address
 */
u32 rx_hdcp22_rd_reg(u32 addr)
{
	return (u32)rx_sec_reg_read((u32 *)(unsigned long)
		(rx_reg_maps[MAP_ADDR_MODULE_HDMIRX_CAPB3].phy_addr + addr));
}

/*
 * rx_hdcp22_rd_reg_bits - hdcp2.2 reg masked bits read
 * @addr: register address
 * @mask: bits mask
 */
u32 rx_hdcp22_rd_reg_bits(u32 addr, u32 mask)
{
	return rx_get_bits(rx_hdcp22_rd_reg(addr), mask);
}

/*
 * rx_hdcp22_wr_reg_bits - hdcp2.2 reg masked bits write
 * @addr: register address
 * @mask: bits mask
 * @value: new register value
 */
void rx_hdcp22_wr_reg_bits(u32 addr,
			   u32 mask,
			   u32 value)
{
	rx_hdcp22_wr_reg(addr, rx_set_bits(rx_hdcp22_rd_reg(addr),
					   mask, value));
}

/*
 * hdcp22_wr_top - hdcp2.2 top reg write
 * @addr: register address
 * @data: new register value
 */
void rx_hdcp22_wr_top(u32 addr, u32 data)
{
	sec_top_write((u32 *)(unsigned long)addr, data);
}

/*
 * hdcp22_rd_top - hdcp2.2 top reg read
 * @addr: register address
 */
u32 rx_hdcp22_rd_top(u32 addr)
{
	if (rx_info.chip_id >= CHIP_ID_T7)
		return 0;
	return (u32)sec_top_read((u32 *)(unsigned long)addr);
}

/*
 * sec_top_write - secure top write
 */
void sec_top_write(u32 *addr, u32 value)
{
	struct arm_smccc_res res;

	if (rx_info.chip_id >= CHIP_ID_T7)
		arm_smccc_smc(HDMIRX_WR_SEC_TOP_NEW, (unsigned long)(uintptr_t)addr,
		      value, 0, 0, 0, 0, 0, &res);
	else
		arm_smccc_smc(HDMIRX_WR_SEC_TOP, (unsigned long)(uintptr_t)addr,
		      value, 0, 0, 0, 0, 0, &res);
}

/*
 * sec_top_read - secure top read
 */
u32 sec_top_read(u32 *addr)
{
	struct arm_smccc_res res;

	if (rx_info.chip_id >= CHIP_ID_T7)
		return 0;
	else
		arm_smccc_smc(HDMIRX_RD_SEC_TOP, (unsigned long)(uintptr_t)addr,
			      0, 0, 0, 0, 0, 0, &res);

	return (u32)((res.a0) & 0xffffffff);
}

/*
 * rx_sec_reg_write - secure region write
 */
void rx_sec_reg_write(u32 *addr, u32 value)
{
	struct arm_smccc_res res;

	arm_smccc_smc(HDCP22_RX_ESM_WRITE, (unsigned long)(uintptr_t)addr,
		      value, 0, 0, 0, 0, 0, &res);
}

/*
 * rx_sec_reg_read - secure region read
 */
u32 rx_sec_reg_read(u32 *addr)
{
	struct arm_smccc_res res;

	arm_smccc_smc(HDCP22_RX_ESM_READ, (unsigned long)(uintptr_t)addr,
		      0, 0, 0, 0, 0, 0, &res);

	return (u32)((res.a0) & 0xffffffff);
}

/*
 * rx_sec_set_duk
 */
u32 rx_sec_set_duk(bool repeater)
{
	struct arm_smccc_res res;

	if (repeater)
		arm_smccc_smc(HDCP22_RP_SET_DUK_KEY, 0, 0, 0, 0, 0, 0, 0, &res);
	else
		arm_smccc_smc(HDCP22_RX_SET_DUK_KEY, 0, 0, 0, 0, 0, 0, 0, &res);

	return (u32)((res.a0) & 0xffffffff);
}

/*
 * rx_set_hdcp14_secure_key
 */
u32 rx_set_hdcp14_secure_key(void)
{
	struct arm_smccc_res res;

	/* 0x8200002d is the SMC cmd defined in BL31,this CMD
	 * will call set hdcp1.4 key function
	 */
	arm_smccc_smc(HDCP14_RX_SETKEY, 0, 0, 0, 0, 0, 0, 0, &res);

	return (u32)((res.a0) & 0xffffffff);
}

/*
 * rx_smc_cmd_handler: communicate with bl31
 */
u32 rx_smc_cmd_handler(u32 index, u32 value)
{
	struct arm_smccc_res res;

	arm_smccc_smc(HDMI_RX_SMC_CMD, index,
				value, 0, 0, 0, 0, 0, &res);
	return (u32)((res.a0) & 0xffffffff);
}

void hdmirx_phy_pddq_tl1_tm2(u32 enable, u32 term_val)
{
	wr_reg_hhi_bits(HHI_HDMIRX_PHY_MISC_CNTL2,
			_BIT(1), !enable);
	/* set rxsense */
	if (enable)
		wr_reg_hhi_bits(HHI_HDMIRX_PHY_MISC_CNTL0,
				MSK(3, 0), 0);
	else
		wr_reg_hhi_bits(HHI_HDMIRX_PHY_MISC_CNTL0,
				MSK(3, 0), term_val);
}

void hdmirx_phy_pddq_t5(u32 enable, u32 term_val)
{
	hdmirx_wr_bits_amlphy(T5_HHI_RX_PHY_MISC_CNTL2,
			      _BIT(1), !enable);
	/* set rxsense */
	if (enable)
		hdmirx_wr_bits_amlphy(T5_HHI_RX_PHY_MISC_CNTL0,
				      MSK(3, 0), 0);
	else
		hdmirx_wr_bits_amlphy(T5_HHI_RX_PHY_MISC_CNTL0,
				      MSK(3, 0), term_val);
}

void hdmirx_phy_pddq_snps(u32 enable)
{
	hdmirx_wr_bits_dwc(DWC_SNPS_PHYG3_CTRL, MSK(1, 1), enable);
}

/*
 * hdmirx_phy_pddq - phy pddq config
 * @enable: enable phy pddq up
 */
void hdmirx_phy_pddq(u32 enable)
{
	u32 term_value = hdmirx_rd_top_common(TOP_HPD_PWR5V) & 0x7;

	if (rx_info.chip_id >= CHIP_ID_TL1 &&
	    rx_info.chip_id <= CHIP_ID_TM2) {
		hdmirx_phy_pddq_tl1_tm2(enable, term_value);
	} else if (rx_info.chip_id >= CHIP_ID_T5) {
		hdmirx_phy_pddq_t5(enable, term_value);
	} else {
		hdmirx_wr_bits_dwc(DWC_SNPS_PHYG3_CTRL, MSK(1, 1), enable);
	}
}

/*
 * hdmirx_wr_ctl_port
 */
void hdmirx_wr_ctl_port(u32 offset, u32 data)
{
	unsigned long flags;

	if (rx_info.chip_id < CHIP_ID_TL1) {
		spin_lock_irqsave(&reg_rw_lock, flags);
		wr_reg(MAP_ADDR_MODULE_TOP, hdmirx_ctrl_port + offset, data);
		spin_unlock_irqrestore(&reg_rw_lock, flags);
	}
}

/*
 * hdmirx_top_sw_reset
 */
void hdmirx_top_sw_reset(void)
{
	ulong flags;
	unsigned long dev_offset = 0;

	spin_lock_irqsave(&reg_rw_lock, flags);
	if (rx_info.chip_id >= CHIP_ID_TL1 &&
	    rx_info.chip_id <= CHIP_ID_T5D) {
		dev_offset = TOP_COMMON_OFFSET +
			rx_reg_maps[MAP_ADDR_MODULE_TOP].phy_addr;
		wr_reg(MAP_ADDR_MODULE_TOP, dev_offset + TOP_SW_RESET, 1);
		udelay(1);
		wr_reg(MAP_ADDR_MODULE_TOP, dev_offset + TOP_SW_RESET, 0);
	} else if (rx_info.chip_id >= CHIP_ID_T7) {
		//dev_offset = rx_reg_maps[MAP_ADDR_MODULE_TOP].phy_addr;
		//wr_reg(MAP_ADDR_MODULE_TOP, dev_offset + TOP_SW_RESET, 1);
		//udelay(1);
		//wr_reg(MAP_ADDR_MODULE_TOP, dev_offset + TOP_SW_RESET, 0);
	} else {
		wr_reg(MAP_ADDR_MODULE_TOP,
		       hdmirx_addr_port | dev_offset, TOP_SW_RESET);
		wr_reg(MAP_ADDR_MODULE_TOP,
		       hdmirx_data_port | dev_offset, 1);
		udelay(1);
		wr_reg(MAP_ADDR_MODULE_TOP,
		       hdmirx_addr_port | dev_offset, TOP_SW_RESET);
		wr_reg(MAP_ADDR_MODULE_TOP,
		       hdmirx_data_port | dev_offset, 0);
	}
	spin_unlock_irqrestore(&reg_rw_lock, flags);
}

/*
 * rx_irq_en - hdmirx controller irq config
 * @enable - irq set or clear
 */
void rx_set_irq_20(u8 enable)
{
	u32 data32 = 0;
	if (enable == IRQ_EN_ALL) {
		if (rx_info.chip_id >= CHIP_ID_TL1) {
			data32 |= 1 << 31; /* DRC_CKS_CHG */
			data32 |= 1 << 30; /* DRC_RCV */
			data32 |= 0 << 29; /* AUD_TYPE_CHG */
			data32 |= 0 << 28; /* DVI_DET */
			data32 |= 0 << 27; /* VSI_CKS_CHG */
			data32 |= 0 << 26; /* GMD_CKS_CHG */
			data32 |= 0 << 25; /* AIF_CKS_CHG */
			data32 |= 1 << 24; /* AVI_CKS_CHG */
			data32 |= 0 << 23; /* ACR_N_CHG */
			data32 |= 0 << 22; /* ACR_CTS_CHG */
			data32 |= 1 << 21; /* GCP_AV_MUTE_CHG */
			data32 |= 0 << 20; /* GMD_RCV */
			data32 |= 0 << 19; /* AIF_RCV */
			data32 |= 1 << 18; /* AVI_RCV */
			data32 |= 0 << 17; /* ACR_RCV */
			data32 |= 0 << 16; /* GCP_RCV */
			data32 |= 0 << 15; /* VSI_RCV */
			data32 |= 0 << 14; /* AMP_RCV */
			data32 |= 0 << 13; /* AMP_CHG */
			data32 |= 1 << 9; /* EMP_RCV*/
			data32 |= 0 << 8; /* PD_FIFO_NEW_ENTRY */
			data32 |= 0 << 4; /* PD_FIFO_OVERFL */
			data32 |= 0 << 3; /* PD_FIFO_UNDERFL */
			data32 |= 0 << 2; /* PD_FIFO_TH_START_PASS */
			data32 |= 0 << 1; /* PD_FIFO_TH_MAX_PASS */
			data32 |= 0 << 0; /* PD_FIFO_TH_MIN_PASS */
			data32 |= pdec_ists_en;
		} else if (rx_info.chip_id == CHIP_ID_TXLX) {
			data32 |= 1 << 31; /* DRC_CKS_CHG */
			data32 |= 1 << 30; /* DRC_RCV */
			data32 |= 0 << 29; /* AUD_TYPE_CHG */
			data32 |= 0 << 28; /* DVI_DET */
			data32 |= 1 << 27; /* VSI_CKS_CHG */
			data32 |= 0 << 26; /* GMD_CKS_CHG */
			data32 |= 0 << 25; /* AIF_CKS_CHG */
			data32 |= 1 << 24; /* AVI_CKS_CHG */
			data32 |= 0 << 23; /* ACR_N_CHG */
			data32 |= 0 << 22; /* ACR_CTS_CHG */
			data32 |= 1 << 21; /* GCP_AV_MUTE_CHG */
			data32 |= 0 << 20; /* GMD_RCV */
			data32 |= 0 << 19; /* AIF_RCV */
			data32 |= 0 << 18; /* AVI_RCV */
			data32 |= 0 << 17; /* ACR_RCV */
			data32 |= 0 << 16; /* GCP_RCV */
			data32 |= 1 << 15; /* VSI_RCV */
			data32 |= 0 << 14; /* AMP_RCV */
			data32 |= 0 << 13; /* AMP_CHG */
			data32 |= 0 << 8; /* PD_FIFO_NEW_ENTRY */
			data32 |= 0 << 4; /* PD_FIFO_OVERFL */
			data32 |= 0 << 3; /* PD_FIFO_UNDERFL */
			data32 |= 0 << 2; /* PD_FIFO_TH_START_PASS */
			data32 |= 0 << 1; /* PD_FIFO_TH_MAX_PASS */
			data32 |= 0 << 0; /* PD_FIFO_TH_MIN_PASS */
			data32 |= pdec_ists_en;
		} else if (rx_info.chip_id == CHIP_ID_TXHD) {
			/* data32 |= 1 << 31;  DRC_CKS_CHG */
			/* data32 |= 1 << 30; DRC_RCV */
			data32 |= 0 << 29; /* AUD_TYPE_CHG */
			data32 |= 0 << 28; /* DVI_DET */
			data32 |= 1 << 27; /* VSI_CKS_CHG */
			data32 |= 0 << 26; /* GMD_CKS_CHG */
			data32 |= 0 << 25; /* AIF_CKS_CHG */
			data32 |= 1 << 24; /* AVI_CKS_CHG */
			data32 |= 0 << 23; /* ACR_N_CHG */
			data32 |= 0 << 22; /* ACR_CTS_CHG */
			data32 |= 1 << 21; /* GCP_AV_MUTE_CHG */
			data32 |= 0 << 20; /* GMD_RCV */
			data32 |= 0 << 19; /* AIF_RCV */
			data32 |= 0 << 18; /* AVI_RCV */
			data32 |= 0 << 17; /* ACR_RCV */
			data32 |= 0 << 16; /* GCP_RCV */
			data32 |= 1 << 15; /* VSI_RCV */
			/* data32 |= 0 << 14;  AMP_RCV */
			/* data32 |= 0 << 13;  AMP_CHG */
			data32 |= 0 << 8; /* PD_FIFO_NEW_ENTRY */
			data32 |= 0 << 4; /* PD_FIFO_OVERFL */
			data32 |= 0 << 3; /* PD_FIFO_UNDERFL */
			data32 |= 0 << 2; /* PD_FIFO_TH_START_PASS */
			data32 |= 0 << 1; /* PD_FIFO_TH_MAX_PASS */
			data32 |= 0 << 0; /* PD_FIFO_TH_MIN_PASS */
			data32 |= pdec_ists_en;
		} else { /* TXL and previous Chip */
			data32 = 0;
			data32 |= 0 << 29; /* AUD_TYPE_CHG */
			data32 |= 0 << 28; /* DVI_DET */
			data32 |= 1 << 27; /* VSI_CKS_CHG */
			data32 |= 0 << 26; /* GMD_CKS_CHG */
			data32 |= 0 << 25; /* AIF_CKS_CHG */
			data32 |= 1 << 24; /* AVI_CKS_CHG */
			data32 |= 0 << 23; /* ACR_N_CHG */
			data32 |= 0 << 22; /* ACR_CTS_CHG */
			data32 |= 1 << 21; /* GCP_AV_MUTE_CHG */
			data32 |= 0 << 20; /* GMD_RCV */
			data32 |= 0 << 19; /* AIF_RCV */
			data32 |= 0 << 18; /* AVI_RCV */
			data32 |= 0 << 17; /* ACR_RCV */
			data32 |= 0 << 16; /* GCP_RCV */
			data32 |= 0 << 15; /* VSI_RCV */
			data32 |= 0 << 14; /* AMP_RCV */
			data32 |= 0 << 13; /* AMP_CHG */
			/* diff */
			data32 |= 1 << 10; /* DRC_CKS_CHG */
			data32 |= 1 << 9; /* DRC_RCV */
			/* diff */
			data32 |= 0 << 8; /* PD_FIFO_NEW_ENTRY */
			data32 |= 0 << 4; /* PD_FIFO_OVERFL */
			data32 |= 0 << 3; /* PD_FIFO_UNDERFL */
			data32 |= 0 << 2; /* PD_FIFO_TH_START_PASS */
			data32 |= 0 << 1; /* PD_FIFO_TH_MAX_PASS */
			data32 |= 0 << 0; /* PD_FIFO_TH_MIN_PASS */
			data32 |= pdec_ists_en;
		}
		hdmirx_wr_dwc(DWC_PDEC_IEN_SET, data32);
		hdmirx_wr_dwc(DWC_AUD_FIFO_IEN_SET, OVERFL | UNDERFL);
		if (hdcp22_on)
			hdmirx_wr_dwc(DWC_HDMI2_IEN_SET, 0x1f);
		/* hdcp1.4 */
		hdmirx_wr_dwc(DWC_HDMI_IEN_SET, AKSV_RCV);
		/* clear status */
		hdmirx_wr_dwc(DWC_PDEC_ICLR, ~0);
		hdmirx_wr_dwc(DWC_AUD_CEC_ICLR, ~0);
		hdmirx_wr_dwc(DWC_AUD_FIFO_ICLR, ~0);
		hdmirx_wr_dwc(DWC_MD_ICLR, ~0);
	} else if (enable == IRQ_EN_HDCP) {
		if (hdcp22_on)
			hdmirx_wr_dwc(DWC_HDMI2_IEN_SET, 0x1f);
		/* hdcp1.4 */
		hdmirx_wr_dwc(DWC_HDMI_IEN_SET, AKSV_RCV);
	} else {
		/* clear enable */
		hdmirx_wr_dwc(DWC_HDMI2_IEN_CLR, ~0);
		hdmirx_wr_dwc(DWC_HDMI_IEN_CLR, ~0);
		hdmirx_wr_dwc(DWC_PDEC_IEN_CLR, ~0);
		hdmirx_wr_dwc(DWC_AUD_CEC_IEN_CLR, ~0);
		hdmirx_wr_dwc(DWC_AUD_FIFO_IEN_CLR, ~0);
		hdmirx_wr_dwc(DWC_MD_IEN_CLR, ~0);
		/* clear status */
		hdmirx_wr_dwc(DWC_HDMI2_ICLR, ~0);
		hdmirx_wr_dwc(DWC_HDMI_ICLR, ~0);
		hdmirx_wr_dwc(DWC_PDEC_ICLR, ~0);
		hdmirx_wr_dwc(DWC_AUD_CEC_ICLR, ~0);
		hdmirx_wr_dwc(DWC_AUD_FIFO_ICLR, ~0);
		hdmirx_wr_dwc(DWC_MD_ICLR, ~0);
	}
}

//enable = 0 off
//enable = 1 all en
//enable = 2 hdcp en only
void rx_set_irq_21(u8 enable, u8 port)
{
	u8 data8;
	u8 val_hdcp;
	u8 val_all;

	val_hdcp = enable ? 1 : 0;
	val_all = (enable == IRQ_EN_ALL) ? 1 : 0;
	//HDCP
	data8 = 0; //HDCP2.X
	data8 |= val_hdcp << 2; //ake init rcvd
	data8 |= val_all << 1; //stream_manage_rcvd for repeater
	//hdmirx_wr_cor(CP2PAX_INTR1_MASK_HDCP2X_IVCRX, 0x6, port);
	hdmirx_wr_cor(CP2PAX_INTR1_MASK_HDCP2X_IVCRX, data8, port);

	data8 = 0;//HDCP1.4
	data8 |= val_all << 1; //auth done
	data8 |= val_hdcp << 0; //auth start
	//hdmirx_wr_cor(RX_INTR1_MASK_PWD_IVCRX, 0x03, port);
	hdmirx_wr_cor(RX_INTR1_MASK_PWD_IVCRX, data8, port);

	data8 = 0;
	data8 |= val_all << 0; /* 1.4 encrypted sts changed en */
	hdmirx_wr_cor(RX_HDCP1X_INTR0_MASK_HDCP1X_IVCRX, data8, port);

	data8 = 0;//HDCP2.x
	data8 |= val_all << 3; //hash fail
	data8 |= val_all << 1; //auth fail
	data8 |= val_all << 0; //auth done
	hdmirx_wr_cor(CP2PAX_INTR0_MASK_HDCP2X_IVCRX, data8, port);

	data8 = 0;
	data8 |= val_all << 7; /* 2.3 encrypted sts changed en */
	hdmirx_wr_cor(RX_INTR13_MASK_PWD_IVCRX, data8, port);

	//HDCP 2X_RX_ECC
	data8 |= 0 << 0;//ecc
	hdmirx_wr_cor(HDCP2X_RX_ECC_INTR_MASK, data8, port);

	data8 = 0;
	data8 |= 0 << 4; /* intr_new_unrec en */
	data8 |= 0 << 2; /* intr_new_aud */
	data8 |= val_all << 1; /* intr_spd */
	data8 |= val_all << 0; /* intr_avi */
	hdmirx_wr_cor(RX_DEPACK_INTR2_MASK_DP2_IVCRX, data8, port);

	data8 = 0;
	data8 |= 0 << 4; /* intr_cea_repeat_hf_vsi en */
	data8 |= 0 << 3; /* intr_cea_new_hf_vsi en */
	data8 |= 0 << 2; /* intr_cea_new_vsi */
	hdmirx_wr_cor(RX_DEPACK_INTR3_MASK_DP2_IVCRX, data8, port);

	data8 = 0;
	data8 |= 0 << 4; /* end of VSIF EMP data received */
	data8 |= 0 << 3;
	data8 |= 0 << 2;
	hdmirx_wr_cor(RX_DEPACK2_INTR2_MASK_DP0B_IVCRX, data8, port);

	data8 = 0;
	data8 |= 0 << 2; /* gcp did not arrived 4 frames */
	hdmirx_wr_cor(RX_DEPACK_INTR6_MASK_DP2_IVCRX, data8, port);

	data8 = 0;
	hdmirx_wr_cor(RX_DEPACK2_INTR0_MASK_DP0B_IVCRX, data8, port);//emp

	data8 = 0;
	data8 |= val_all << 6; //hdcp1.x group en
	data8 |= val_all << 2; //hdcp2.x group en
	data8 |= val_all << 0; //depac group en
	hdmirx_wr_cor(RX_GRP_INTR1_MASK_PWD_IVCRX, data8, port);

	hdmirx_wr_cor(RX_INTR2_MASK_PWD_IVCRX, 0x00, port);
	hdmirx_wr_cor(RX_INTR3_MASK_PWD_IVCRX, 0x00, port);
	hdmirx_wr_cor(RX_INTR4_MASK_PWD_IVCRX, 0x00, port);
	hdmirx_wr_cor(RX_INTR5_MASK_PWD_IVCRX, 0x00, port);
	hdmirx_wr_cor(RX_INTR6_MASK_PWD_IVCRX, 0x00, port);
	hdmirx_wr_cor(RX_INTR7_MASK_PWD_IVCRX, 0x00, port);
	hdmirx_wr_cor(RX_INTR8_MASK_PWD_IVCRX, 0x00, port);
	hdmirx_wr_cor(RX_INTR9_MASK_PWD_IVCRX, 0x00, port);
	hdmirx_wr_cor(RX_DEPACK_INTR4_MASK_DP2_IVCRX, 0x00, port);//3d audio

	if (!enable) {
		/* clear status */
		hdmirx_wr_cor(RX_HDCP1X_INTR0_HDCP1X_IVCRX, 0xff, port);
		hdmirx_wr_cor(CP2PAX_INTR1_HDCP2X_IVCRX, 0xff, port);
		hdmirx_wr_cor(RX_DEPACK_INTR2_DP2_IVCRX, 0xff, port);
		hdmirx_wr_cor(RX_DEPACK_INTR3_DP2_IVCRX, 0xff, port);
		hdmirx_wr_cor(RX_GRP_INTR1_STAT_PWD_IVCRX, 0xff, port);
		hdmirx_wr_cor(RX_INTR1_PWD_IVCRX, 0xff, port);
		hdmirx_wr_cor(RX_INTR2_PWD_IVCRX, 0xff, port);
		hdmirx_wr_cor(RX_INTR3_PWD_IVCRX, 0xff, port);
		hdmirx_wr_cor(RX_INTR4_PWD_IVCRX, 0xff, port);
		hdmirx_wr_cor(RX_INTR5_PWD_IVCRX, 0xff, port);
		hdmirx_wr_cor(RX_INTR6_PWD_IVCRX, 0xff, port);
		hdmirx_wr_cor(RX_INTR7_PWD_IVCRX, 0xff, port);
		hdmirx_wr_cor(RX_INTR8_PWD_IVCRX, 0xff, port);
		hdmirx_wr_cor(RX_INTR9_PWD_IVCRX, 0xff, port);
		hdmirx_wr_cor(RX_DEPACK2_INTR2_DP0B_IVCRX, 0xff, port);
		hdmirx_wr_cor(RX_DEPACK_INTR6_DP2_IVCRX, 0xff, port);
		hdmirx_wr_cor(HDCP2X_RX_ECC_INTR, 0xff, port);
	}
}

/*
 * rx_irq_en - hdmirx controller irq config
 * @enable - irq set or clear
 */
void rx_irq_en(u8 enable, u8 port)
{
	switch (rx_info.chip_id) {
	case CHIP_ID_T3X:
	case CHIP_ID_T5M:
	case CHIP_ID_TXHD2:
	case CHIP_ID_T3:
	case CHIP_ID_T7:
	case CHIP_ID_T5W:
		hdmirx_top_irq_en(enable, port);
		rx_set_irq_21(enable, port);
	break;
	case CHIP_ID_T5:
	case CHIP_ID_T5D:
	case CHIP_ID_TM2:
	case CHIP_ID_TL1:
	default:
		hdmirx_top_irq_en(enable, port);
		rx_set_irq_20(enable);
	break;
	}
}

/*
 * hdmirx_irq_hdcp_enable - hdcp irq enable
 */
void hdmirx_top_irq_en(u8 en, u8 port)
{
	u32 data32;

	if (rx_info.chip_id >= CHIP_ID_T3X) {//todo
		data32  = 0;
		data32 |= (0    << 30); // [   30] aud_chg;
		data32 |= (1    << 29); // [   29] hdmirx_sqofclk_fall;
		data32 |= (0    << 28); // [   28] hdmirx_sqofclk_rise;
		data32 |= (0	<< 26); // [   26] last_emp_done;
		data32 |= (1	<< 23); // [   23] de_rise_del_irq;
		data32 |= (1	<< 21); // [   21] emp_field_done;
		data32 |= (1    << 20); // [   23] hdmirx_sqofclk_fall;
		data32 |= (0    << 19); // [   19] edid_addr2_intr
		data32 |= (0    << 18); // [   18] edid_addr1_intr
		data32 |= (0    << 17); // [   17] edid_addr0_intr
		data32 |= (0    << 16); // [   16] hdcp_enc_state_fall
		data32 |= (0    << 15); // [   15] hdcp_enc_state_rise
		data32 |= (0    << 14); // [   14] hdcp_auth_start_fall
		data32 |= (0    << 13); // [   13] clk1618 chg
		data32 |= (0    << 12); // [   12] meter_stable_chg_hdmi
		data32 |= (0    << 11); // [   11] vid_colour_depth_chg
		data32 |= (0    << 10); // [   10] vid_fmt_chg
		data32 |= (0    << 9);  // [    9] tmds21clk chg
		data32 |= (0x0  << 4);  // [ 8: 6] hdmirx_5v_fall
		data32 |= (0x0  << 3);  // [ 5: 3] hdmirx_5v_rise
		// [    2] sherman_phy_intr: phy digital interrupt
		data32 |= (0    << 2);
		// [    1] pwd_sherman_intr: controller pwd interrupt
		data32 |= (1    << 1);
		// [    0] aon_sherman_intr: controller aon interrupt
		data32 |= (0    << 0);
		top_intr_maskn_value = data32;
	} else if (rx_info.chip_id >= CHIP_ID_T7 &&
		rx_info.chip_id <= CHIP_ID_TXHD2) {
		data32  = 0;
		data32 |= (0    << 30); // [   30] aud_chg;
		data32 |= (1    << 29); // [   29] hdmirx_sqofclk_fall;
		data32 |= (0    << 28); // [   28] hdmirx_sqofclk_rise;
		data32 |= (1	<< 27); // [   27] de_rise_del_irq;
		data32 |= (0    << 26); // [   26] last_emp_done;
		data32 |= (1	<< 25); // [   25] emp_field_done;
		data32 |= (0    << 23); // [   23] meter_stable_chg_cable;
		data32 |= (0    << 19); // [   19] edid_addr2_intr
		data32 |= (0    << 18); // [   18] edid_addr1_intr
		data32 |= (0    << 17); // [   17] edid_addr0_intr
		data32 |= (0    << 16); // [   16] hdcp_enc_state_fall
		data32 |= (0    << 15); // [   15] hdcp_enc_state_rise
		data32 |= (0    << 14); // [   14] hdcp_auth_start_fall
		data32 |= (0    << 13); // [   13] hdcp_auth_start_rise
		data32 |= (0    << 12); // [   12] meter_stable_chg_hdmi
		data32 |= (0    << 11); // [   11] vid_colour_depth_chg
		data32 |= (0    << 10); // [   10] vid_fmt_chg
		data32 |= (0x0  << 6);  // [ 8: 6] hdmirx_5v_fall
		data32 |= (0x0  << 3);  // [ 5: 3] hdmirx_5v_rise
		// [    2] sherman_phy_intr: phy digital interrupt
		data32 |= (0    << 2);
		// [    1] pwd_sherman_intr: controller pwd interrupt
		data32 |= (1    << 1);
		// [    0] aon_sherman_intr: controller aon interrupt
		data32 |= (0    << 0);
		top_intr_maskn_value = data32;
	} else {
		data32 = 0;
		//hdmirx_sqofclk_fall
		data32 |= (1 << 29);
		//de_rise_irq: DE rise edge.
		data32 |= (1 << 27);
		//RX Controller IP interrupt.
		data32 |= (1 << 0);
		top_intr_maskn_value = data32;
	}
	if (en == IRQ_EN_ALL) {
		if (rx_info.chip_id <= CHIP_ID_TL1)
			top_intr_maskn_value |= 0x1e0000;

		//hdmirx_wr_top(TOP_INTR_MASKN, top_intr_maskn_value, port);
	} else if (en == IRQ_EN_HDCP) {
		top_intr_maskn_value = 1 << 1;
	} else {
		top_intr_maskn_value = 0;
	}
	hdmirx_wr_top(TOP_INTR_MASKN, top_intr_maskn_value, port);
}

/*
 * rx_get_aud_info - get aduio info
 */
void rx_get_aud_info(struct aud_info_s *audio_info, u8 port)
{
	u32 tmp;
	struct packet_info_s *prx = &rx_pkt[port];
	struct aud_infoframe_st *pkt =
		(struct aud_infoframe_st *)&prx->aud_pktinfo;

	if (port == rx_info.sub_port)
		return;
	/* refer to hdmi spec. CT = 0 */
	audio_info->coding_type = 0;
	/* refer to hdmi spec. SS = 0 */
	audio_info->sample_size = 0;
	/* refer to hdmi spec. SF = 0*/
	audio_info->sample_frequency = 0;
	if (rx_info.chip_id >= CHIP_ID_T7) {
		pkt->pkttype = PKT_TYPE_INFOFRAME_AUD;
		pkt->version = hdmirx_rd_cor(AUDRX_VERS_DP2_IVCRX, port);
		pkt->length = hdmirx_rd_cor(AUDRX_LENGTH_DP2_IVCRX, port);
		pkt->checksum = hdmirx_rd_cor(AUDRX_CHSUM_DP2_IVCRX, port);
		pkt->rsd = 0;

		/*get AudioInfo */
		pkt->coding_type = hdmirx_rd_bits_cor(AUDRX_DBYTE1_DP2_IVCRX, MSK(4, 4), port);
		pkt->ch_count = hdmirx_rd_bits_cor(AUDRX_DBYTE1_DP2_IVCRX, MSK(3, 0), port);
		pkt->sample_frq = hdmirx_rd_bits_cor(AUDRX_DBYTE2_DP2_IVCRX, MSK(3, 2), port);
		pkt->sample_size = hdmirx_rd_bits_cor(AUDRX_DBYTE2_DP2_IVCRX, MSK(2, 0), port);
		pkt->fromat = hdmirx_rd_cor(AUDRX_DBYTE3_DP2_IVCRX, port);
		pkt->ca = hdmirx_rd_cor(AUDRX_DBYTE4_DP2_IVCRX, port);
		pkt->down_mix = hdmirx_rd_bits_cor(AUDRX_DBYTE5_DP2_IVCRX, MSK(7, 6), port);
		pkt->level_shift_value =
			hdmirx_rd_bits_cor(AUDRX_DBYTE5_DP2_IVCRX, MSK(4, 3), port);
		pkt->lfep = hdmirx_rd_bits_cor(AUDRX_DBYTE5_DP2_IVCRX, MSK(2, 0), port);

		audio_info->channel_count = pkt->ch_count;
		audio_info->auds_ch_alloc = pkt->ca;
		audio_info->aud_hbr_rcv =
			(hdmirx_rd_cor(RX_AUDP_STAT_DP2_IVCRX, port) >> 6) & 1;
		audio_info->auds_layout =
			hdmirx_rd_bits_cor(RX_AUDP_STAT_DP2_IVCRX, MSK(2, 3), port);
		tmp = (hdmirx_rd_cor(RX_ACR_DBYTE4_DP2_IVCRX, port) & 0x0f) << 16;
		tmp += hdmirx_rd_cor(RX_ACR_DBYTE5_DP2_IVCRX, port) << 8;
		tmp += hdmirx_rd_cor(RX_ACR_DBYTE6_DP2_IVCRX, port);
		audio_info->n = tmp;
		tmp = (hdmirx_rd_cor(RX_ACR_DBYTE1_DP2_IVCRX, port) & 0x0f) << 16;
		tmp += hdmirx_rd_cor(RX_ACR_DBYTE2_DP2_IVCRX, port) << 8;
		tmp += hdmirx_rd_cor(RX_ACR_DBYTE3_DP2_IVCRX, port);
		audio_info->cts = tmp;
		if (rx_info.chip_id == CHIP_ID_T7 || rx_info.chip_id == CHIP_ID_T3X) {
			if (audio_info->aud_hbr_rcv) {
				audio_info->aud_packet_received = 8;
			} else {
				if (pkt->length == 10) { //aif length is 10
					audio_info->aud_packet_received = 1;
				} else if (audio_info->n) {
					audio_info->aud_packet_received = 1;
				} else {
					audio_info->aud_packet_received = 0;
				}
			}
		} else {
			audio_info->aud_packet_received =
				hdmirx_rd_top(TOP_MISC_STAT0, port) >> 16 & 0xff;
		}
		audio_info->ch_sts[0] = hdmirx_rd_cor(RX_CHST1_AUD_IVCRX, port);
		audio_info->ch_sts[1] = hdmirx_rd_cor(RX_CHST2_AUD_IVCRX, port);
		audio_info->ch_sts[2] = hdmirx_rd_cor(RX_CHST3a_AUD_IVCRX, port);
		audio_info->ch_sts[3] = hdmirx_rd_cor(RX_CHST4_AUD_IVCRX, port);
		audio_info->ch_sts[4] = hdmirx_rd_cor(RX_CHST5_AUD_IVCRX, port);
		audio_info->ch_sts[5] = hdmirx_rd_cor(RX_CHST6_AUD_IVCRX, port);
		audio_info->ch_sts[6] = hdmirx_rd_cor(RX_CHST7_AUD_IVCRX, port);
	} else {
		audio_info->channel_count =
			hdmirx_rd_bits_dwc(DWC_PDEC_AIF_PB0, CHANNEL_COUNT);
		audio_info->coding_extension =
			hdmirx_rd_bits_dwc(DWC_PDEC_AIF_PB0, AIF_DATA_BYTE_3);
		audio_info->auds_ch_alloc =
			hdmirx_rd_bits_dwc(DWC_PDEC_AIF_PB0, CH_SPEAK_ALLOC);
		audio_info->auds_layout =
			hdmirx_rd_bits_dwc(DWC_PDEC_STS, PD_AUD_LAYOUT);
		audio_info->aud_hbr_rcv =
			hdmirx_rd_dwc(DWC_PDEC_AUD_STS) & AUDS_HBR_RCV;
		audio_info->aud_packet_received =
				hdmirx_rd_dwc(DWC_PDEC_AUD_STS);
		audio_info->aud_mute_en =
			(hdmirx_rd_bits_dwc(DWC_PDEC_STS, PD_GCP_MUTE_EN) == 0)
			? false : true;
		audio_info->cts = hdmirx_rd_dwc(DWC_PDEC_ACR_CTS);
		audio_info->n = hdmirx_rd_dwc(DWC_PDEC_ACR_N);
		audio_info->afifo_cfg = rx_get_afifo_cfg();
	}
	if (audio_info->cts != 0) {
		if (rx[port].var.frl_rate == 0) {
			audio_info->arc =
				(rx[rx_info.main_port].clk.tmds_clk / audio_info->cts) *
				audio_info->n / 128;
		} else {
			audio_info->arc =
				(rx[rx_info.main_port].clk.fpll_clk / audio_info->cts) *
				audio_info->n / 128;
		}
	} else {
		audio_info->arc = 0;
	}
	audio_info->aud_clk = rx[rx_info.main_port].clk.aud_pll;
}

/*
 * rx_get_audio_status - interface for audio module
 */
void rx_get_audio_status(struct rx_audio_stat_s *aud_sts)
{
	u8 port = rx_info.main_port;
	enum tvin_sig_fmt_e fmt = hdmirx_hw_get_fmt(port);

	if (rx_info.chip_id == CHIP_ID_NONE)
		return;
	if (rx[port].state == FSM_SIG_READY &&
	    fmt != TVIN_SIG_FMT_NULL &&
	    rx[port].avmute_skip == 0) {
		if (rx_info.chip_id < CHIP_ID_T7) {
			aud_sts->aud_alloc = rx[port].aud_info.auds_ch_alloc;
			aud_sts->aud_sr = rx[port].aud_info.real_sr;
			aud_sts->aud_channel_cnt = rx[port].aud_info.channel_count;
			aud_sts->aud_type = rx[port].aud_info.coding_type;
			aud_sts->afifo_thres_pass =
				((hdmirx_rd_dwc(DWC_AUD_FIFO_STS) &
				 THS_PASS_STS) == 0) ? false : true;
			aud_sts->aud_rcv_packet =
				rx[port].aud_info.aud_packet_received;
			aud_sts->aud_stb_flag =
				aud_sts->afifo_thres_pass &&
				!rx[port].aud_info.aud_mute_en;
		} else {
			if ((rx[port].afifo_sts & 3) == 0)
				aud_sts->aud_stb_flag = true;
			aud_sts->aud_alloc = rx[port].aud_info.auds_ch_alloc;
			aud_sts->aud_rcv_packet =
				rx[port].aud_info.aud_packet_received;
			aud_sts->aud_channel_cnt = rx[port].aud_info.channel_count;
			aud_sts->aud_type = rx[port].aud_info.coding_type;
			aud_sts->aud_sr = rx[port].aud_info.real_sr;
			memcpy(aud_sts->ch_sts, &rx[port].aud_info.ch_sts, 7);
		}
	} else {
		memset(aud_sts, 0, sizeof(struct rx_audio_stat_s));
	}
}
EXPORT_SYMBOL(rx_get_audio_status);

/*
 * rx_get_audio_status - interface for audio module
 */

int rx_set_audio_param(u32 param)
{
	if (rx_info.chip_id == CHIP_ID_NONE)
		return 0;
	if (rx_info.chip_id < CHIP_ID_T7) {
		hbr_force_8ch = param & 1;
	} else if (rx_info.chip_id < CHIP_ID_T3X) {
		rx_set_aud_output_t7(param);
	} else {
		rx_set_aud_output_t3x(param, E_PORT0);
		rx_set_aud_output_t3x(param, E_PORT1);
		rx_set_aud_output_t3x(param, E_PORT2);
		rx_set_aud_output_t3x(param, E_PORT3);
	}
	return 1;
}
EXPORT_SYMBOL(rx_set_audio_param);

/*
 * rx_get_hdmi5v_sts - get current pwr5v status on all ports
 */
u32 rx_get_hdmi5v_sts(void)
{
	return (hdmirx_rd_top_common(TOP_HPD_PWR5V) >> 20) &
		MSK(rx_info.port_num, 0);
}
EXPORT_SYMBOL(rx_get_hdmi5v_sts);

/*
 * rx_get_hpd_sts - get current hpd status on all ports
 */
u32 rx_get_hpd_sts(u8 port)
{
	return (hdmirx_rd_top_common(TOP_HPD_PWR5V) >> port) & 0x1;
}

/*
 * rx_get_scdc_clkrate_sts - get tmds clk ratio
 */
u32 rx_get_scdc_clkrate_sts(u8 port)
{
	u32 clk_rate = 0;

	if (rx_info.chip_id == CHIP_ID_TXHD ||
	    rx_info.chip_id == CHIP_ID_T5D)
		clk_rate = 0;
	else if (rx_info.chip_id >= CHIP_ID_T7)
		clk_rate = (hdmirx_rd_cor(SCDCS_TMDS_CONFIG_SCDC_IVCRX, port) >> 1) & 1;
	else
		clk_rate = (hdmirx_rd_dwc(DWC_SCDC_REGS0) >> 17) & 1;

	if (force_clk_rate & 0x10) {
		clk_rate = force_clk_rate & 1;
		if (clk_rate)
			hdmirx_wr_cor(HDMI2_MODE_CTRL_AON_IVCRX, 0x33, port);
		else
			hdmirx_wr_cor(HDMI2_MODE_CTRL_AON_IVCRX, 0x11, port);
	}

	return clk_rate;
}

/*
 * rx_get_pll_lock_sts - tmds pll lock indication
 * return true if tmds pll locked, false otherwise.
 */
u32 rx_get_pll_lock_sts(void)
{
	return hdmirx_rd_dwc(DWC_HDMI_PLL_LCK_STS) & 1;
}

/*
 * rx_get_aud_pll_lock_sts - audio pll lock indication
 * no use
 */
bool rx_get_aud_pll_lock_sts(void)
{
	/* if ((hdmirx_rd_dwc(DWC_AUD_PLL_CTRL) & (1 << 31)) == 0) */
	if ((rd_reg_hhi(HHI_AUD_PLL_CNTL_I) & (1 << 31)) == 0)
		return false;
	else
		return true;
}

bool is_clk_stable(u8 port)
{
	bool flag = false;

	port = (rx_info.chip_id >= CHIP_ID_T3X) ? port : rx_info.main_port;

	//t3x frl todo
	if (rx[port].var.frl_rate)
		return true;
	switch (rx_info.chip_id) {
	case CHIP_ID_TXHD:
	case CHIP_ID_TXL:
	case CHIP_ID_TXLX:
		flag = hdmirx_rd_phy(PHY_MAINFSM_STATUS1) & 0x100;
	break;
	case CHIP_ID_TL1:
	case CHIP_ID_TM2:
	case CHIP_ID_T5:
	case CHIP_ID_T5D:
	case CHIP_ID_T7:
	case CHIP_ID_T3:
	case CHIP_ID_T5W:
	case CHIP_ID_T5M:
	case CHIP_ID_TXHD2:
	case CHIP_ID_T3X:
	default:
		if (rx[port].clk.cable_clk > TMDS_CLK_MIN * KHz &&
			rx[port].clk.cable_clk < TMDS_CLK_MAX * KHz &&
			abs(rx[port].clk.cable_clk - rx[port].clk.cable_clk_pre) < 5 * MHz)
			flag = true;
	break;
	}
	return flag;
}

void rx_afifo_store_valid(bool en, u8 port)
{
	if (rx_info.chip_id >= CHIP_ID_T7)
		return;

	if (!en) {
		hdmirx_wr_bits_dwc(DWC_AUD_FIFO_CTRL,
						   AFIF_SUBPACKETS, 0);
		rx[port].aud_info.afifo_cfg = false;
		if (log_level & AUDIO_LOG)
			rx_pr("afifo store all\n");
	} else {
		hdmirx_wr_bits_dwc(DWC_AUD_FIFO_CTRL,
						   AFIF_SUBPACKETS, 1);
		rx[port].aud_info.afifo_cfg = true;
		if (log_level & AUDIO_LOG)
			rx_pr("afifo store valid\n");
	}
}

bool rx_get_afifo_cfg(void)
{
	if (rx_info.chip_id >= CHIP_ID_T7)
		return true;

	if (hdmirx_rd_bits_dwc(DWC_AUD_FIFO_CTRL, AFIF_SUBPACKETS))
		return true;
	else
		return false;
}

void hdmirx_audio_disabled(u8 port)
{
	if (rx_info.chip_id >= CHIP_ID_T7)
		hdmirx_wr_bits_cor(RX_PWD_SRST_PWD_IVCRX, _BIT(1), 1, port);
	else
		hdmirx_wr_bits_dwc(DWC_AUD_FIFO_CTRL, AFIF_INIT, 1);

	if (log_level & AUDIO_LOG)
		rx_pr("Aml %s\n", __func__);
}

/*
 * hdmirx_audio_fifo_rst - reset afifo
 */
u32 hdmirx_audio_fifo_rst(u8 port)
{
	int error = 0;

	if (port == rx_info.sub_port)
		return 0;
	if (rx_info.chip_id >= CHIP_ID_T7) {
		if (rx_info.chip_id >= CHIP_ID_T5M) {
			hdmirx_wr_cor(RX_AUDIO_FIFO_RST, 0xff, port);
			udelay(1);
			hdmirx_wr_cor(RX_AUDIO_FIFO_RST, 0x0, port);
		}
		hdmirx_wr_bits_cor(RX_PWD_SRST_PWD_IVCRX, _BIT(1), 1, port);
		udelay(1);
		hdmirx_wr_bits_cor(RX_PWD_SRST_PWD_IVCRX, _BIT(1), 0, port);
	}  else {
		hdmirx_wr_dwc(DWC_DMI_SW_RST, 0x10);
		hdmirx_wr_bits_dwc(DWC_AUD_FIFO_CTRL, AFIF_INIT, 1);
		//udelay(20);
		hdmirx_wr_bits_dwc(DWC_AUD_FIFO_CTRL, AFIF_INIT, 0);
	}
	if (log_level & AUDIO_LOG)
		rx_pr("%s\n", __func__);
	return error;
}

/*
 * hdmirx_control_clk_range
 */
int hdmirx_control_clk_range(unsigned long min, unsigned long max)
{
	int error = 0;
	u32 eval_time = 0;
	unsigned long ref_clk;

	ref_clk = modet_clk;
	eval_time = (ref_clk * 4095) / 158000;
	min = (min * eval_time) / ref_clk;
	max = (max * eval_time) / ref_clk;
	hdmirx_wr_bits_dwc(DWC_HDMI_CKM_F, MINFREQ, min);
	hdmirx_wr_bits_dwc(DWC_HDMI_CKM_F, CKM_MAXFREQ, max);
	return error;
}

/*
 * hdmirx_clr_scdc
 * en: for chip_id >= T7, clear and recovery are done together
 *	0: recover scdc; 1: clear scdc
 */
void hdmirx_clr_scdc(bool en, u8 port)
{
	switch (rx_info.chip_id) {
	case CHIP_ID_TXHD:
	case CHIP_ID_T5D:
		break;
	case CHIP_ID_GXTVBB:
	case CHIP_ID_TXL:
	case CHIP_ID_TXLX:
	case CHIP_ID_TL1:
	case CHIP_ID_TM2:
	case CHIP_ID_T5:
		if (en)
			hdmirx_wr_dwc(DWC_SCDC_CONFIG, 0x2);
		else
			hdmirx_wr_dwc(DWC_SCDC_CONFIG, 0x1);
		break;
	case CHIP_ID_T7:
	case CHIP_ID_T3:
	case CHIP_ID_T5W:
	case CHIP_ID_T5M:
	case CHIP_ID_TXHD2:
	default:
		if (en)
			rx_clr_scdc(port);
		break;
	}
}

int packet_init_t5(void)
{
	int error = 0;
	int data32 = 0;

	data32 |= 1 << 12; /* emp_err_filter, tl1*/
	data32 |= 1 << 11;
	data32 |= 1 << 9; /* amp_err_filter */
	data32 |= 1 << 8; /* isrc_err_filter */
	data32 |= 1 << 7; /* gmd_err_filter */
	data32 |= 1 << 6; /* aif_err_filter */
	data32 |= 1 << 5; /* avi_err_filter */
	data32 |= 1 << 4; /* vsi_err_filter */
	data32 |= 1 << 3; /* gcp_err_filter */
	data32 |= 1 << 2; /* acrp_err_filter */
	data32 |= 1 << 1; /* ph_err_filter */
	data32 |= 0 << 0; /* checksum_err_filter */
	hdmirx_wr_dwc(DWC_PDEC_ERR_FILTER, data32);

	data32 = hdmirx_rd_dwc(DWC_PDEC_CTRL);
	data32 |= 1 << 31;	/* PFIFO_STORE_FILTER_EN */
	data32 |= 0 << 30;  /* Enable packet FIFO store EMP pkt*/
	data32 |= 0 << 22;
	data32 |= 1 << 4;	/* PD_FIFO_WE */
	data32 |= 0 << 1;	/* emp pkt rev int,0:last 1:every */
	data32 |= 1 << 0;	/* PDEC_BCH_EN */
	data32 &= (~GCP_GLOB_AVMUTE);
	data32 |= GCP_GLOB_AVMUTE_EN << 15;
	data32 |= packet_fifo_cfg;
	hdmirx_wr_dwc(DWC_PDEC_CTRL, data32);

	data32 = 0;
	data32 |= pd_fifo_start_cnt << 20;	/* PD_start */
	data32 |= 640 << 10;	/* PD_max */
	data32 |= 8 << 0;		/* PD_min */
	hdmirx_wr_dwc(DWC_PDEC_FIFO_CFG, data32);

	return error;
}

int packet_init_t7(u8 port)
{
	u8 data8 = 0;

	/* vsif id check en */
	//hdmirx_wr_cor(VSI_CTRL2_DP3_IVCRX, 1, port);
	/* vsif pkt id cfg, default is 000c03 */
	//hdmirx_wr_cor(VSIF_ID1_DP3_IVCRX, 0xd8, port);
	//hdmirx_wr_cor(VSIF_ID2_DP3_IVCRX, 0x5d, port);
	//hdmirx_wr_cor(VSIF_ID3_DP3_IVCRX, 0xc4, port);
	//hdmirx_wr_cor(VSIF_ID4_DP3_IVCRX, 0, port);

	/* hf-vsif id check en */
	//hdmirx_wr_bits_cor(HF_VSIF_CTRL_DP3_IVCRX, _BIT(3), 1, port);
	/* hf-vsif set to get dv, default is 0xc45dd8 */
	//hdmirx_wr_cor(HF_VSIF_ID1_DP3_IVCRX, 0x46, port);
	//hdmirx_wr_cor(HF_VSIF_ID2_DP3_IVCRX, 0xd0, port);
	//hdmirx_wr_cor(HF_VSIF_ID3_DP3_IVCRX, 0x00, port);

	//data8 = 0;
	//data8 |= 1 << 7; /* enable clr vsif pkt */
	//data8 |= 1 << 5; /* enable comparison first 3 bytes IEEE */
	//data8 |= 4 << 2; /* clr register if 4 frames no pkt */
	///hdmirx_wr_cor(VSI_CTRL1_DP3_IVCRX, data8, port);
	//hdmirx_wr_cor(VSI_CTRL3_DP3_IVCRX, 1, port);
	/* aif to store hdr10+ */
	//hdmirx_wr_cor(VSI_ID1_DP3_IVCRX, 0x8b, port);
	//hdmirx_wr_cor(VSI_ID2_DP3_IVCRX, 0x84, port);
	//hdmirx_wr_cor(VSI_ID3_DP3_IVCRX, 0x90, port);

	/* use unrec to store hf-vsif */
	//hdmirx_wr_cor(RX_UNREC_CTRL_DP2_IVCRX, 1, port);
	//hdmirx_wr_cor(RX_UNREC_DEC_DP2_IVCRX, PKT_TYPE_INFOFRAME_VSI, port);

	/* get data 0x11c0-11de */

	data8 = 0;
	data8 |= 0 << 7; /* use AIF to VSI */
	data8 |= 1 << 6; /* irq is set for any VSIF */
	data8 |= 0 << 5; /* irq is set for any ACP */
	data8 |= 1 << 4; /* irq is set for any UN-REC */
	data8 |= 0 << 3; /* irq is set for any MPEG */
	data8 |= 1 << 2; /* irq is set for any AUD */
	data8 |= 1 << 1; /* irq is set for any SPD */
	data8 |= 0 << 0; /* irq is set for any AVI */
	hdmirx_wr_cor(RX_INT_IF_CTRL_DP2_IVCRX, data8, port);

	data8 = 0;
	data8 |= 0 << 7; /* rsvd */
	data8 |= 0 << 6; /* rsvd */
	data8 |= 0 << 5; /* rsvd */
	data8 |= 0 << 4; /* rsvd */
	data8 |= 0 << 3; /* irq is set for any ACR */
	data8 |= 0 << 2; /* irq is set for any GCP */
	data8 |= 0 << 1; /* irq is set for any ISC2 */
	data8 |= 0 << 0; /* irq is set for any ISC1 */
	hdmirx_wr_cor(RX_INT_IF_CTRL2_DP2_IVCRX, data8, port);

	/* auto clr pkt if cable-unplugged/sync lost */
	data8 = 0;
	data8 |= 1 << 7; /*  */
	data8 |= 1 << 6; /*  */
	data8 |= 1 << 5; /*  */
	data8 |= 1 << 4; /*  */
	data8 |= 1 << 3; /*  */
	data8 |= 1 << 2; /*  */
	data8 |= 1 << 1; /*  */
	data8 |= 1 << 0; /*  */
	hdmirx_wr_cor(RX_AUTO_CLR_PKT1_DP2_IVCRX, data8, port);

	/* auto clr pkt if cable-unplugged/sync lost */
	data8 = 0;
	data8 |= 1 << 7; /*  */
	data8 |= 1 << 6; /*  */
	data8 |= 1 << 5; /*  */
	data8 |= 1 << 4; /*  */
	data8 |= 1 << 3; /*  */
	data8 |= 1 << 2; /*  */
	data8 |= 1 << 1; /*  */
	data8 |= 1 << 0; /*  */
	hdmirx_wr_cor(RX_AUTO_CLR_PKT2_DP2_IVCRX, data8, port);

	/* auto clr pkt if did not get update */
	data8 = 0;
	data8 |= 1 << 1; /* meta data */
	data8 |= 1 << 0; /* GCP */
	hdmirx_wr_cor(IF_CTRL2_DP3_IVCRX, data8, port);

	return 0;
}

/*
 * packet_init - packet receiving config
 */
int packet_init(u8 port)
{
	if (rx_info.chip_id >= CHIP_ID_T7)
		packet_init_t7(port);
	else
		packet_init_t5();
	return 0;
}

/*
 * pd_fifo_irq_ctl
 */
void pd_fifo_irq_ctl(bool en)
{
	int i = hdmirx_rd_dwc(DWC_PDEC_IEN);

	if (en == 0)
		hdmirx_wr_bits_dwc(DWC_PDEC_IEN_CLR, _BIT(2), 1);
	else
		hdmirx_wr_dwc(DWC_PDEC_IEN_SET, _BIT(2) | i);
}

/*
 * hdmirx_packet_fifo_rst - reset packet fifo
 */
u32 hdmirx_packet_fifo_rst(void)
{
	int error = 0;

	hdmirx_wr_bits_dwc(DWC_PDEC_CTRL,
			   PD_FIFO_FILL_INFO_CLR | PD_FIFO_CLR, ~0);
	hdmirx_wr_bits_dwc(DWC_PDEC_CTRL,
			   PD_FIFO_FILL_INFO_CLR | PD_FIFO_CLR,  0);
	return error;
}

void rx_set_suspend_edid_clk(bool en)
{
	if (en) {
		hdmirx_wr_bits_top_common(TOP_EDID_GEN_CNTL,
				   MSK(7, 0), 1);
	} else {
		hdmirx_wr_bits_top_common(TOP_EDID_GEN_CNTL,
				   MSK(7, 0), EDID_CLK_DIV);
	}
}

void rx_clr_gcp_avmute(u8 port)
{
	if (rx_info.chip_id < CHIP_ID_T7)
		return;

	hdmirx_wr_bits_cor(DEC_AV_MUTE_DP2_IVCRX, _BIT(5), 1, port);
}

bool rx_is_need_edid_reset(u8 port)
{
	bool ret = false;
	unsigned int sts;
	unsigned int ddc_sts;
	unsigned int ddc_offset;

	sts = hdmirx_rd_top(TOP_EDID_GEN_STAT, port);
	ddc_sts = (sts >> 20) & 0x1f;
	ddc_offset = sts & 0x1ff;
	if (ddc_offset != 0 && ddc_offset != 0xff)
		ret = true;
	return ret;
}

void rx_edid_module_reset(void)
{
	if (rx_info.chip_id == CHIP_ID_T3X) {
		hdmirx_wr_top_common(TOP_SW_RESET, 0x1000);
		udelay(1);
		hdmirx_wr_top_common(TOP_SW_RESET, 0);
	} else {
		hdmirx_wr_top_common(TOP_SW_RESET, 0x2);
		udelay(1);
		hdmirx_wr_top_common(TOP_SW_RESET, 0);
	}
}

void rx_i2c_div_init(void)
{
	int data32 = 0;

	data32 |= (0xf	<< 13); /* bit[16:13] */
	if (rx_info.chip_id >= CHIP_ID_T5M)
		data32 |= 0x0f << 12;
	data32 |= 0	<< 11;
	data32 |= 0	<< 10;
	data32 |= 0	<< 9;
	data32 |= 0 << 8;
	data32 |= EDID_CLK_DIV << 0;
	hdmirx_wr_top_common(TOP_EDID_GEN_CNTL, data32);
}

void rx_i2c_hdcp_cfg(void)
{
	int data32 = 0;

	data32 = 0;
	/* SDA filter internal clk div */
	data32 |= 1 << 29;
	/* SDA sampling clk div */
	data32 |= 1 << 16;
	/* SCL filter internal clk div */
	data32 |= 1 << 13;
	/* SCL sampling clk div */
	data32 |= 1 << 0;
	hdmirx_wr_top_common(TOP_INFILTER_HDCP, data32);
}

void rx_i2c_edid_cfg_with_port(u8 port_id, bool en)
{
	int data32 = 0;
	u8 port;

	data32 = 0;
	/* SDA filter internal clk div */
	data32 |= 1 << 29;
	/* SDA sampling clk div */
	data32 |= 1 << 16;
	/* SCL filter internal clk div */
	data32 |= 1 << 13;
	/* SCL sampling clk div */
	data32 |= 1 << 0;
	if (!en) {
		data32 = 0xffffffff;
		if (rx_info.chip_id >= CHIP_ID_T3X) {
			hdmirx_wr_top(TOP_INFILTER_I2C0, data32, port_id);
		} else {
			if (port_id == 0)
				hdmirx_wr_top_common(TOP_INFILTER_I2C0, data32);
			else if (port_id == 1)
				hdmirx_wr_top_common(TOP_INFILTER_I2C1, data32);
			else if (port_id == 2)
				hdmirx_wr_top_common(TOP_INFILTER_I2C2, data32);
			else if (port_id == 3)
				hdmirx_wr_top_common(TOP_INFILTER_I2C3, data32);
		}
	} else {
		if (rx_info.chip_id >= CHIP_ID_T3X) {
			for (port = 0; port < 4; port++)
				hdmirx_wr_top(TOP_INFILTER_I2C0, data32, port);
		} else {
			hdmirx_wr_top_common(TOP_INFILTER_I2C0, data32);
			hdmirx_wr_top_common(TOP_INFILTER_I2C1, data32);
			hdmirx_wr_top_common(TOP_INFILTER_I2C2, data32);
			hdmirx_wr_top_common(TOP_INFILTER_I2C3, data32);
		}
	}
}

/*
 * TOP_init - hdmirx top initialization
 */
void top_common_init(void)
{
	u32 data32 = 0;
	u8 i = 0;

	data32 = 0;
	/* bit4: hpd override, bit5: hpd reverse */
	data32 |= 1 << 4;
	if (rx_info.chip_id == CHIP_ID_GXTVBB)
		data32 |= 0 << 5;
	else
		data32 |= 1 << 5;
	/* pull down all the hpd */
	hdmirx_wr_top_common(TOP_HPD_PWR5V, data32);
	data32 = 0;
	if (rx_info.chip_id >= CHIP_ID_T7 &&
		rx_info.chip_id < CHIP_ID_T3X) {
		/*420to444_en*/
		data32 |= 1	<< 21;
		/*422to444_en*/
		data32 |= 1	<< 20;
	}
	/* conversion mode of 422 to 444 */
	data32 |= 0	<< 19;
	/* pixel_repeat_ovr 0=auto  1 only for T7!!! */
	if (rx_info.chip_id == CHIP_ID_T7)
		data32 |= 1 << 7;
	/* !!!!dolby vision 422 to 444 ctl bit */
	data32 |= 0	<< 0;
	hdmirx_wr_top_common(TOP_VID_CNTL, data32);//to do

	if (rx_info.chip_id != CHIP_ID_TXHD &&
		rx_info.chip_id != CHIP_ID_T5D) {
		data32 = 0;
		data32 |= 0	<< 20;
		data32 |= 0	<< 8;
		data32 |= 0x0a	<< 0;
		hdmirx_wr_top_common(TOP_VID_CNTL2, data32);//to do
	}

	data32 = 0;
	data32 |= 7	<< 13;
	data32 |= 0	<< 12;
	data32 |= 1	<< 11;
	data32 |= 0	<< 10;
	data32 |= 0	<< 9;
	data32 |= 1	<< 8;
	data32 |= 1	<< 6;
	data32 |= 3	<< 4;
	data32 |= 0	<< 3;
	data32 |= acr_mode  << 2;
	data32 |= acr_mode  << 1;
	data32 |= acr_mode  << 0;
	if (rx_info.chip_id != CHIP_ID_T3X)
		hdmirx_wr_top_common(TOP_ACR_CNTL_STAT, data32);
	else
		hdmirx_wr_top_common_1(TOP_ACR_CNTL_STAT, data32);

	if (rx_info.chip_id >= CHIP_ID_TL1) {
		data32 = 0;
		data32 |= 0	<< 2;/*meas_mode*/
		data32 |= 1	<< 1;/*enable*/
		data32 |= 1	<< 0;/*reset*/
		if (acr_mode)
			data32 |= 2 << 16;/*aud pll*/
		else
			data32 |= 500 << 16;/*acr*/
		hdmirx_wr_top_common(TOP_AUDMEAS_CTRL, data32);//to do
		hdmirx_wr_top_common(TOP_AUDMEAS_CYCLES_M1, 65535);//to do
		/*start messure*/
		hdmirx_wr_top_common(TOP_AUDMEAS_CTRL, data32 & (~0x1));//to do
	}

	if (rx_info.chip_id < CHIP_ID_T3X)
		return;

	for (i = 0; i < 4; i++) {
		/* reset and select data port */
		data32 = hdmirx_rd_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_MISC2, i);
		data32 &= (~(0xf << 24));
		data32 |= ((1 << i) << 24);
		hdmirx_wr_amlphy_t3x(T3X_HDMIRX20PHY_DCHA_MISC2, data32, i);
		//hdmirx_wr_bits_top_common(TOP_PORT_SEL, MSK(4, 0), (1 << port));
	}
}

static int top_init(u8 port)
{
	int err = 0;
	int data32 = 0;

	if (rx_info.chip_id >= CHIP_ID_T7) {
		rx_hdcp22_wr_top(TOP_SECURE_MODE,  0x1f);
		/* Filter 100ns glitch */
		hdmirx_wr_top(TOP_AUD_PLL_LOCK_FILTER, 32, port);
		data32  = 0;
		data32 |= (1 << 1);// [1:0]  sel
		hdmirx_wr_top(TOP_PHYIF_CNTL0, data32, port);
	}
	rx_i2c_div_init();
	rx_i2c_hdcp_cfg();
	rx_i2c_edid_cfg_with_port(0xf, true);

	data32 = 0;
	if (rx_info.chip_id >= CHIP_ID_TL1) {
		/* n_cts_auto_mode: */
		/*	0-every ACR packet */
		/*	1-on N or CTS value change */
		data32 |= 1 << 4;
	}
	/* delay cycles before n/cts update pulse */
	data32 |= 7 << 0;
	if (rx_info.chip_id >= CHIP_ID_TL1)
		hdmirx_wr_top(TOP_TL1_ACR_CNTL2, data32, port);// to do
	else
		hdmirx_wr_top(TOP_ACR_CNTL2, data32, port);//change

	if (rx_info.chip_id >= CHIP_ID_TL1) {
		/* Configure channel switch */
		data32  = 0;
		data32 |= (3 << 24);
		data32 |= (0 << 4); /* [  4]  valid_always*/
		data32 |= (7 << 0); /* [3:0]  decoup_thresh*/
		if (rx_info.chip_id >= CHIP_ID_T3X)
			hdmirx_wr_top(TOP_CHAN_SWITCH_1_T3X, data32, port);
		else
			hdmirx_wr_top(TOP_CHAN_SWITCH_1, data32, port);
		data32  = 0;
		data32 |= (2 << 28); /* [29:28]      source_2 */
		data32 |= (1 << 26); /* [27:26]      source_1 */
		data32 |= (0 << 24); /* [25:24]      source_0 */
		hdmirx_wr_top(TOP_CHAN_SWITCH_0, data32, port);

		/* Configure TMDS align T7 unused */
		if (rx_info.chip_id <= CHIP_ID_T7) {
			data32	= 0;
			hdmirx_wr_top(TOP_TMDS_ALIGN_CNTL0, data32, port);
			data32	= 0;
			hdmirx_wr_top(TOP_TMDS_ALIGN_CNTL1, data32, port);
		}

		/* Enable channel output */
		data32 = hdmirx_rd_top(TOP_CHAN_SWITCH_0, port);
		hdmirx_wr_top(TOP_CHAN_SWITCH_0, data32 | (1 << 0), port);

		/* configure cable clock measure */
		data32 = 0;
		data32 |= (1 << 28); /* [31:28] meas_tolerance */
		data32 |= (8192 << 0); /* [23: 0] ref_cycles */
		hdmirx_wr_top(TOP_METER_CABLE_CNTL, data32, port);//change to do
	}

	/* configure hdmi clock measure */
	data32 = 0;
	data32 |= (1 << 28); /* [31:28] meas_tolerance */
	data32 |= (8192 << 0); /* [23: 0] ref_cycles */
	hdmirx_wr_top(TOP_METER_HDMI_CNTL, data32, port);

	/* bit'15 hbr_spdif_en: 1-spdif, 0- hbr */
	if (rx_info.chip_id >= CHIP_ID_T7)
		hdmirx_wr_bits_top(TOP_CLK_CNTL, _BIT(15), 0, port);
	return err;
}

/*
 * DWC_init - DWC controller initialization
 */
static int DWC_init(void)
{
	int err = 0;
	unsigned long   data32;
	u32 eval_time = 0;

	eval_time = (modet_clk * 4095) / 158000;
	/* enable all */
	hdmirx_wr_dwc(DWC_HDMI_OVR_CTRL, ~0);
	/* recover to default value.*/
	/* remain code for some time.*/
	/* if no side effect then remove it */
	/*hdmirx_wr_bits_dwc(DWC_HDMI_SYNC_CTRL,*/
	/*	VS_POL_ADJ_MODE, VS_POL_ADJ_AUTO);*/
	/*hdmirx_wr_bits_dwc(DWC_HDMI_SYNC_CTRL,*/
	/*	HS_POL_ADJ_MODE, HS_POL_ADJ_AUTO);*/

	hdmirx_wr_bits_dwc(DWC_HDMI_CKM_EVLTM, EVAL_TIME, eval_time);
	hdmirx_control_clk_range(TMDS_CLK_MIN, TMDS_CLK_MAX);

	/* hdmirx_wr_bits_dwc(DWC_SNPS_PHYG3_CTRL,*/
		/*((1 << 2) - 1) << 2, port); */

	data32 = 0;
	data32 |= 0     << 20;
	data32 |= 1     << 19;
	data32 |= 5     << 16;  /* [18:16]  valid_mode */
	data32 |= 0     << 12;  /* [13:12]  ctrl_filt_sens */
	data32 |= 3     << 10;  /* [11:10]  vs_filt_sens */
	data32 |= 0     << 8;   /* [9:8]    hs_filt_sens */
	data32 |= 2     << 6;   /* [7:6]    de_measure_mode */
	data32 |= 0     << 5;   /* [5]      de_regen */
	data32 |= 3     << 3;   /* [4:3]    de_filter_sens */
	hdmirx_wr_dwc(DWC_HDMI_ERROR_PROTECT, data32);

	data32 = 0;
	data32 |= 0     << 8;   /* [10:8]   hact_pix_ith */
	data32 |= 0     << 5;   /* [5]      hact_pix_src */
	data32 |= 1     << 4;   /* [4]      htot_pix_src */
	hdmirx_wr_dwc(DWC_MD_HCTRL1, data32);

	data32 = 0;
	data32 |= 1     << 12;  /* [14:12]  hs_clk_ith */
	data32 |= 7     << 8;   /* [10:8]   htot32_clk_ith */
	data32 |= 1     << 5;   /* [5]      vs_act_time */
	data32 |= 3     << 3;   /* [4:3]    hs_act_time */
	/* bit[1:0] default setting should be 2 */
	data32 |= 2     << 0;   /* [1:0]    h_start_pos */
	hdmirx_wr_dwc(DWC_MD_HCTRL2, data32);

	data32 = 0;
	data32 |= 1	<< 4;   /* [4]      v_offs_lin_mode */
	data32 |= 1	<< 1;   /* [1]      v_edge */
	data32 |= 0	<< 0;   /* [0]      v_mode */
	hdmirx_wr_dwc(DWC_MD_VCTRL, data32);

	data32  = 0;
	data32 |= 1 << 10;  /* [11:10]  vofs_lin_ith */
	data32 |= 3 << 8;   /* [9:8]    vact_lin_ith */
	data32 |= 0 << 6;   /* [7:6]    vtot_lin_ith */
	data32 |= 7 << 3;   /* [5:3]    vs_clk_ith */
	data32 |= 2 << 0;   /* [2:0]    vtot_clk_ith */
	hdmirx_wr_dwc(DWC_MD_VTH, data32);

	data32  = 0;
	data32 |= 1 << 2;   /* [2]      fafielddet_en */
	data32 |= 0 << 0;   /* [1:0]    field_pol_mode */
	hdmirx_wr_dwc(DWC_MD_IL_POL, data32);

	data32  = 0;
	data32 |= 0	<< 1;
	data32 |= 1	<< 0;
	hdmirx_wr_dwc(DWC_HDMI_RESMPL_CTRL, data32);

	data32	= 0;
	data32 |= (hdmirx_rd_dwc(DWC_HDMI_MODE_RECOVER) & 0xf8000000);
	data32 |= (0	<< 24);
	data32 |= (0	<< 18);
	data32 |= (HYST_HDMI_TO_DVI	<< 13);
	data32 |= (HYST_DVI_TO_HDMI	<< 8);
	data32 |= (0	<< 6);
	data32 |= (0	<< 4);
	/* EESS_OESS */
	/* 0: new auto mode,check on HDMI mode or 1.1 features en */
	/* 1: force OESS */
	/* 2: force EESS */
	/* 3: auto mode,check CTL[3:0]=d9/d8 during WOO */
	data32 |= (hdcp_enc_mode	<< 2);
	data32 |= (0	<< 0);
	hdmirx_wr_dwc(DWC_HDMI_MODE_RECOVER, data32);

	data32 = hdmirx_rd_dwc(DWC_HDCP_CTRL);
	/* 0: Original behaviour */
	/* 1: Balance path delay between non-HDCP and HDCP */
	data32 |= 1 << 27; /* none & hdcp */
	/* 0: Original behaviour */
	/* 1: Balance path delay between HDCP14 and HDCP22. */
	data32 |= 1 << 26; /* 1.4 & 2.2 */
	hdmirx_wr_dwc(DWC_HDCP_CTRL, data32);

	return err;
}

void rx_hdcp14_set_normal_key(const struct hdmi_rx_hdcp *hdcp)
{
	u32 i = 0;
	u32 k = 0;
	int error = 0;

	for (i = 0; i < HDCP_KEYS_SIZE; i += 2) {
		for (k = 0; k < HDCP_KEY_WR_TRIES; k++) {
			if (hdmirx_rd_bits_dwc(DWC_HDCP_STS,
					       HDCP_KEY_WR_OK_STS) != 0) {
				break;
			}
		}
		if (k < HDCP_KEY_WR_TRIES) {
			hdmirx_wr_dwc(DWC_HDCP_KEY1, hdcp->keys[i + 0]);
			hdmirx_wr_dwc(DWC_HDCP_KEY0, hdcp->keys[i + 1]);
		} else {
			error = -EAGAIN;
			break;
		}
	}
	hdmirx_wr_dwc(DWC_HDCP_BKSV1, hdcp->bksv[0]);
	hdmirx_wr_dwc(DWC_HDCP_BKSV0, hdcp->bksv[1]);
}

/*
 * hdmi_rx_ctrl_hdcp_config - config hdcp1.4 keys
 */
void rx_hdcp14_config(const struct hdmi_rx_hdcp *hdcp)
{
	u32 data32 = 0;

	/* I2C_SPIKE_SUPPR */
	data32 |= 1 << 16;
	/* FAST_I2C */
	data32 |= 0 << 12;
	/* ONE_DOT_ONE */
	data32 |= 0 << 9;
	/* FAST_REAUTH */
	data32 |= 0 << 8;
	/* DDC_ADDR */
	data32 |= 0x3a << 1;
	hdmirx_wr_dwc(DWC_HDCP_SETTINGS, data32);
	/* hdmirx_wr_bits_dwc(DWC_HDCP_SETTINGS, HDCP_FAST_MODE, 0); */
	/* Enable hdcp bcaps bit(bit7). In hdcp1.4 spec: Use of
	 * this bit is reserved, hdcp Receivers not capable of
	 * supporting HDMI must clear this bit to 0. For YAMAHA
	 * RX-V377 amplifier, enable this bit is needed, in case
	 * the amplifier won't do hdcp1.4 interaction occasionally.
	 */
	hdmirx_wr_bits_dwc(DWC_HDCP_SETTINGS, HDCP_BCAPS, 1);
	hdmirx_wr_bits_dwc(DWC_HDCP_CTRL, ENCRIPTION_ENABLE, 0);
	/* hdmirx_wr_bits_dwc(ctx, DWC_HDCP_CTRL, KEY_DECRYPT_ENABLE, 1); */
	hdmirx_wr_bits_dwc(DWC_HDCP_CTRL, KEY_DECRYPT_ENABLE, 0);
	hdmirx_wr_dwc(DWC_HDCP_SEED, hdcp->seed);
	if (hdcp14_key_mode == SECURE_MODE || hdcp14_on) {
		rx_set_hdcp14_secure_key();
		rx_pr("hdcp1.4 secure mode\n");
	} else {
		rx_hdcp14_set_normal_key(&rx[rx_info.main_port].hdcp);
		rx_pr("hdcp1.4 normal mode\n");
	}
	if (rx_info.chip_id != CHIP_ID_TXHD &&
		rx_info.chip_id != CHIP_ID_T5D) {
		hdmirx_wr_bits_dwc(DWC_HDCP_RPT_CTRL,
				   REPEATER, hdcp->repeat ? 1 : 0);
		/* nothing attached downstream */
		hdmirx_wr_dwc(DWC_HDCP_RPT_BSTATUS, 0);
	}
	hdmirx_wr_bits_dwc(DWC_HDCP_CTRL, ENCRIPTION_ENABLE, 1);
}

/* rst cdr to clr tmds_valid */
bool rx_clr_tmds_valid(u8 port)
{
	bool ret = false;

	if (rx_info.chip_id == CHIP_ID_T3X)
		return ret;
	if (rx[port].state >= FSM_SIG_STABLE) {
		rx[port].state = FSM_WAIT_CLK_STABLE;
		if (vpp_mute_enable && port != rx_info.sub_port) {
			rx_mute_vpp(rx_get_port_type(port));
			rx[port].vpp_mute = true;
			set_video_mute(HDMI_RX_MUTE_SET, true);
			rx_pr("vpp mute\n");
		}
		hdmirx_output_en(false);
		rx_irq_en(0, port);
		rx_aud_pll_ctl(0, port);
		if (log_level & VIDEO_LOG)
			rx_pr("%s!\n", __func__);
	}
	if (rx[port].state < FSM_SIG_READY)
		return ret;
	if (rx_info.phy_ver == PHY_VER_TL1) {
		wr_reg_hhi_bits(HHI_HDMIRX_PHY_MISC_CNTL0, MSK(3, 7), 0);
		if (log_level & VIDEO_LOG)
			rx_pr("%s!\n", __func__);
		ret = true;
	} else if (rx_info.phy_ver == PHY_VER_TM2) {
		if (rx_info.aml_phy.force_sqo) {
			wr_reg_hhi_bits(HHI_HDMIRX_PHY_DCHD_CNTL0, _BIT(25), 0);
			udelay(5);
			wr_reg_hhi_bits(HHI_HDMIRX_PHY_DCHD_CNTL0, _BIT(25), 1);
			if (log_level & VIDEO_LOG)
				rx_pr("low amplitude %s!\n", __func__);
			ret = true;
		}
	} else if (rx_info.phy_ver >= PHY_VER_T5) {
		hdmirx_wr_bits_amlphy(T5_HHI_RX_PHY_DCHD_CNTL0, T5_CDR_RST, 0);
		ret = true;
		if (log_level & VIDEO_LOG)
			rx_pr("%s!\n", __func__);
	}
	return ret;
}

void rx_set_term_value_pre(unsigned char port, bool value)
{
	u32 data32;

	if (rx_info.chip_id >= CHIP_ID_TL1) {
		data32 = rd_reg_hhi(HHI_HDMIRX_PHY_MISC_CNTL0);
		if (port < E_PORT3) {
			if (value) {
				data32 |= (1 << port);
			} else {
				/* rst cdr to clr tmds_valid */
				data32 &= ~(MSK(3, 7));
				data32 &= ~(1 << port);
			}
			wr_reg_hhi(HHI_HDMIRX_PHY_MISC_CNTL0, data32);
		} else if (port == ALL_PORTS) {
			if (value) {
				data32 |= 0x7;
			} else {
				/* rst cdr to clr tmds_valid */
				data32 &= 0xfffffc78;
				data32 |= (MSK(3, 7));
				/* data32 &= 0xfffffff8; */
			}
			wr_reg_hhi(HHI_HDMIRX_PHY_MISC_CNTL0, data32);
		}
	} else {
		if (port < rx_info.port_num) {
			if (value)
				hdmirx_wr_bits_phy(PHY_MAIN_FSM_OVERRIDE1,
						   _BIT(port + 4), 1);
			else
				hdmirx_wr_bits_phy(PHY_MAIN_FSM_OVERRIDE1,
						   _BIT(port + 4), 0);
		} else if (port == ALL_PORTS) {
			if (value)
				hdmirx_wr_bits_phy(PHY_MAIN_FSM_OVERRIDE1,
						   PHY_TERM_OV_VALUE, 0xF);
			else
				hdmirx_wr_bits_phy(PHY_MAIN_FSM_OVERRIDE1,
						   PHY_TERM_OV_VALUE, 0);
		}
	}
}

void rx_set_term_value_t5(unsigned char port, bool value)
{
	u32 data32;

	data32 = hdmirx_rd_amlphy(T5_HHI_RX_PHY_MISC_CNTL0);
	if (port < E_PORT3) {
		if (value) {
			data32 |= (1 << port);
		} else {
			/* rst cdr to clr tmds_valid */
			data32 &= ~(MSK(3, 7));
			data32 &= ~(1 << port);
		}
		hdmirx_wr_amlphy(T5_HHI_RX_PHY_MISC_CNTL0, data32);
	} else if (port == ALL_PORTS) {
		if (value) {
			data32 |= 0x7;
		} else {
			/* rst cdr to clr tmds_valid */
			data32 &= 0xfffffc78;
			data32 |= (MSK(3, 7));
			/* data32 &= 0xfffffff8; */
		}
		hdmirx_wr_amlphy(T5_HHI_RX_PHY_MISC_CNTL0, data32);
	}
}

void rx_set_term_value_t5m(unsigned char port, bool value)
{
	u32 data32;

	data32 = hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHA_MISC2);
	if (port < rx_info.port_num) {
		if (value) {
			data32 |= (1 << (28 + port));
		} else {
			/* rst cdr to clr tmds_valid */
			//data32 &= ~(MSK(3, 7));
			data32 &= ~(1 << (28 + port));
		}
		hdmirx_wr_amlphy(T5M_HDMIRX20PHY_DCHA_MISC2, data32);
	} else if (port == ALL_PORTS) {
		if (value) {
			data32 |= 1 << 28;
			data32 |= 1 << 29;
			data32 |= 1 << 30;
			data32 |= 1 << 31;
		} else {
			/* rst cdr to clr tmds_valid */
			data32 &= 0xfffffff;
			//data32 |= (MSK(3, 7));
			/* data32 &= 0xfffffff8; */
		}
		hdmirx_wr_amlphy(T5M_HDMIRX20PHY_DCHA_MISC2, data32);
	}
}

void rx_set_term_value(unsigned char port, bool value)
{
	if (rx_info.chip_id == CHIP_ID_T3X)
		rx_set_term_value_t3x(port, value);
	else if (rx_info.chip_id == CHIP_ID_T5M || rx_info.chip_id == CHIP_ID_TXHD2)
		rx_set_term_value_t5m(port, value);
	else if (rx_info.chip_id >= CHIP_ID_T5 && rx_info.chip_id <= CHIP_ID_T5W)
		rx_set_term_value_t5(port, value);
	else
		rx_set_term_value_pre(port, value);
}

void rx_clr_scdc(u8 port)
{
	if (rx_info.chip_id < CHIP_ID_T7)
		return;

	scdc_dwork.port = port;
	queue_work(scdc_wq, &scdc_dwork.work_wq);
}

void scdc_dwork_handler(struct work_struct *work)
{
	struct work_data *dd = container_of(work, struct work_data, work_wq);

	if (rx_info.chip_id < CHIP_ID_T7)
		return;
	hdmirx_wr_cor(RX_HPD_C_CTRL_AON_IVCRX, 0x0, dd->port);
	mdelay(2);
	hdmirx_wr_cor(RX_HPD_C_CTRL_AON_IVCRX, 0x1, dd->port);
}

int rx_set_port_hpd(u8 port_id, bool val)
{
	if (port_id < rx_info.port_num) {
		if (val) {
			hdmirx_wr_bits_top_common(TOP_HPD_PWR5V, _BIT(port_id), 1);
			rx_i2c_edid_cfg_with_port(0xf, true);
			rx_set_term_value(port_id, 1);
		} else {
			rx_i2c_edid_cfg_with_port(port_id, false);
			hdmirx_wr_bits_top_common(TOP_HPD_PWR5V, _BIT(port_id), 0);
			rx_set_term_value(port_id, 0);
		}
	} else if (port_id == ALL_PORTS) {
		if (val) {
			rx_i2c_edid_cfg_with_port(0xf, true);
			hdmirx_wr_bits_top_common(TOP_HPD_PWR5V, MSK(4, 0), 0xF);
			rx_set_term_value(port_id, 1);
		} else {
			hdmirx_wr_bits_top_common(TOP_HPD_PWR5V, MSK(4, 0), 0x0);
			rx_set_term_value(port_id, 0);
		}
	} else {
		return -1;
	}
	if (log_level & LOG_EN)
		rx_pr("%s, port:%d, val:%d\n", __func__, port_id, val);
	return 0;
}

/* add param to differentiate repeater/main state machine/etc
 * 0: main loop; 2: workaround; 3: repeater flow; 4: special use
 */
void rx_set_cur_hpd(u8 val, u8 func, u8 port)
{
	rx_pr("func-%d\n", func);
	if (val == 0) {
		if (rx_is_need_edid_reset(port))
			rx_edid_module_reset();
	}
	rx_set_port_hpd(port, val);
	port_hpd_rst_flag |= (1 << port);
}

/*
 * rx_force_hpd_config - force config hpd level on all ports
 * @hpd_level: hpd level
 */
void rx_force_hpd_cfg(u8 hpd_level)
{
	u32 hpd_value;

	if (hpd_level) {
		hpd_value = 0xF;
		rx_set_port_hpd(ALL_PORTS, hpd_value);
	} else {
		rx_set_port_hpd(ALL_PORTS, 0);
	}
}

/*
 * rx_force_rxsense_cfg_pre - force config rxsense level on all ports
 * for the chips before t5
 * @level: rxsense level
 */
void rx_force_rxsense_cfg_pre(u8 level)
{
	u32 term_ovr_value;
	u32 data32;

	if (rx_info.chip_id >= CHIP_ID_TL1) {
		/* enable terminal connect */
		data32 = rd_reg_hhi(HHI_HDMIRX_PHY_MISC_CNTL0);
		if (level) {
			term_ovr_value = 0x7;
			data32 |= term_ovr_value;
		} else {
			data32 &= 0xfffffff8;
		}
		wr_reg_hhi(HHI_HDMIRX_PHY_MISC_CNTL0, data32);
	} else {
		if (level) {
			term_ovr_value = 0xF;
			hdmirx_wr_bits_phy(PHY_MAIN_FSM_OVERRIDE1,
					   PHY_TERM_OV_VALUE, term_ovr_value);
		} else {
			hdmirx_wr_bits_phy(PHY_MAIN_FSM_OVERRIDE1,
					   PHY_TERM_OV_VALUE, 0x0);
		}
	}
}

void rx_force_rxsense_cfg_t5(u8 level)
{
	u32 term_ovr_value;
	u32 data32;

	/* enable terminal connect */
	data32 = hdmirx_rd_amlphy(T5_HHI_RX_PHY_MISC_CNTL0);
	if (level) {
		term_ovr_value = 0x7;
		data32 |= term_ovr_value;
	} else {
		data32 &= 0xfffffff8;
	}
	hdmirx_wr_amlphy(T5_HHI_RX_PHY_MISC_CNTL0, data32);
}

void rx_force_rxsense_cfg(u8 level)
{
	if (rx_info.chip_id >= CHIP_ID_T5)
		rx_force_rxsense_cfg_t5(level);
	else
		rx_force_rxsense_cfg_pre(level);
}

/*
 * rx_force_hpd_rxsense_cfg - force config
 * hpd & rxsense level on all ports
 * @level: hpd & rxsense level
 */
void rx_force_hpd_rxsense_cfg(u8 level)
{
	rx_force_hpd_cfg(level);
	rx_force_rxsense_cfg(level);
	if (log_level & LOG_EN)
		rx_pr("hpd & rxsense force val:%d\n", level);
}

/*
 * control_reset - hdmirx controller reset
 */
void control_reset(void)
{
	unsigned long data32;

	/* Enable functional modules */
	data32  = 0;
	data32 |= 1 << 5;   /* [5]      cec_enable */
	data32 |= 1 << 4;   /* [4]      aud_enable */
	data32 |= 1 << 3;   /* [3]      bus_enable */
	data32 |= 1 << 2;   /* [2]      hdmi_enable */
	data32 |= 1 << 1;   /* [1]      modet_enable */
	data32 |= 1 << 0;   /* [0]      cfg_enable */
	hdmirx_wr_dwc(DWC_DMI_DISABLE_IF, data32);
	mdelay(1);
	hdmirx_wr_dwc(DWC_DMI_SW_RST,	0x0000001F);
}

void rx_dig_clk_en(bool en)
{
	switch (rx_info.chip_id) {
	case CHIP_ID_TL1:
		rx_dig_clk_en_tl1(en);
		break;
	case CHIP_ID_TM2:
		rx_dig_clk_en_tm2(en);
		break;
	case CHIP_ID_T5:
	case CHIP_ID_T5D:
		rx_dig_clk_en_t5(en);
		break;
	case CHIP_ID_T7:
	case CHIP_ID_T3:
	case CHIP_ID_T5W:
		rx_dig_clk_en_t7(en);
		break;
	case CHIP_ID_T5M:
		rx_dig_clk_en_t5m(en);
		break;
	case CHIP_ID_T3X:
		rx_dig_clk_en_t3x(en);
		break;
	case CHIP_ID_TXHD2:
		rx_dig_clk_en_txhd2(en);
		break;
	default:
		break;
	}
}

bool rx_get_dig_clk_en_sts(void)
{
	int ret;

	if (rx_info.chip_id >= CHIP_ID_T7)
		return true;
	if (rx_info.chip_id >= CHIP_ID_T5)
		ret = hdmirx_rd_bits_clk_ctl(HHI_HDMIRX_CLK_CNTL, CFG_CLK_EN);
	else
		ret = rd_reg_hhi_bits(HHI_HDMIRX_CLK_CNTL, CFG_CLK_EN);
	return ret;
}

void rx_esm_tmds_clk_en(bool en)
{
	if (rx_info.chip_id >= CHIP_ID_T7)
		return;
	hdmirx_wr_bits_top(TOP_CLK_CNTL, HDCP22_TMDSCLK_EN, en, rx_info.main_port);
	if (hdcp22_on && hdcp_hpd_ctrl_en)
		hdmirx_hdcp22_hpd(en);
	if (log_level & HDCP_LOG)
		rx_pr("%s:%d\n", __func__, en);
}

/*
 * hdcp22_clk_en - clock gating for hdcp2.2
 * @en: enable or disable clock
 */
void hdcp22_clk_en(bool en)
{
	u32 data32;
	u8 port = rx_info.main_port;

	if (rx_info.chip_id >= CHIP_ID_T7)
		return;
	if (en) {
		if (rx_info.chip_id >= CHIP_ID_T5)
			data32 = rd_reg_clk_ctl(HHI_HDCP22_CLK_CNTL);
		else
			data32 = rd_reg_hhi(HHI_HDCP22_CLK_CNTL);
		/* [26:25] select cts_oscin_clk=24 MHz */
		data32 |= 0 << 25;
		/* [   24] Enable gated clock */
		data32 |= 1 << 24;
		data32 |= 0 << 16;
		/* [10: 9] fclk_div7=2000/7=285.71 MHz */
		data32 |= 0 << 9;
		/* [    8] clk_en. Enable gated clock */
		data32 |= 1 << 8;
		/* [ 6: 0] Divide by 1. = 285.71/1 = 285.71 MHz */
		data32 |= 0 << 0;
		if (rx_info.chip_id >= CHIP_ID_T5)
			wr_reg_clk_ctl(HHI_HDCP22_CLK_CNTL, data32);
		else
			wr_reg_hhi(HHI_HDCP22_CLK_CNTL, data32);
		/* axi clk config*/
		if (rx_info.chip_id >= CHIP_ID_T5)
			data32 = rd_reg_clk_ctl(HHI_AXI_CLK_CNTL);
		else
			data32 = rd_reg_hhi(HHI_AXI_CLK_CNTL);
		/* [    8] clk_en. Enable gated clock */
		data32 |= 1 << 8;
		if (rx_info.chip_id >= CHIP_ID_T5)
			wr_reg_clk_ctl(HHI_AXI_CLK_CNTL, data32);
		else
			wr_reg_hhi(HHI_AXI_CLK_CNTL, data32);

		if (rx_info.chip_id >= CHIP_ID_TL1)
			/* TL1:esm related clk bit9-11 */
			hdmirx_wr_bits_top(TOP_CLK_CNTL, MSK(3, 9), 0x7, port);
		else
			/* TL1:esm related clk bit3-5 */
			hdmirx_wr_bits_top(TOP_CLK_CNTL, MSK(3, 3), 0x7, port);

		if (rx_info.chip_id >= CHIP_ID_TM2)
			/* Enable axi_clk,for tm2 */
			/* AXI arbiter is moved outside of hdmitx. */
			/* There is an AXI arbiter in the chip's EE domain */
			/* for arbitrating AXI requests from HDMI TX and rx[rx_info.main_port].*/
			hdmirx_wr_bits_top(TOP_CLK_CNTL, MSK(1, 12), 0x1, port);
	} else {
		if (rx_info.chip_id >= CHIP_ID_T5) {
			wr_reg_clk_ctl(HHI_HDCP22_CLK_CNTL, 0);
			wr_reg_clk_ctl(HHI_AXI_CLK_CNTL, 0);
		} else {
			wr_reg_hhi(HHI_HDCP22_CLK_CNTL, 0);
			wr_reg_hhi(HHI_AXI_CLK_CNTL, 0);
		}
		if (rx_info.chip_id >= CHIP_ID_TL1)
			/* TL1:esm related clk bit9-11 */
			hdmirx_wr_bits_top(TOP_CLK_CNTL, MSK(3, 9), 0x0, port);
		else
			/* TXLX:esm related clk bit3-5 */
			hdmirx_wr_bits_top(TOP_CLK_CNTL, MSK(3, 3), 0x0, port);
	}
}

/*
 * hdmirx_hdcp22_esm_rst - software reset esm
 */
void hdmirx_hdcp22_esm_rst(void)
{
	u8 port = rx_info.main_port;

	if (rx_info.chip_id >= CHIP_ID_T7)
		return;

	rx_pr("before kill\n");
	rx_kill_esm();
	mdelay(5);
	if (hdcp22_kill_esm == 3 || !rx22_ver) {
		rx_pr("before rst:\n");
		/* For TL1,the sw_reset_hdcp22 bit is top reg 0x0,bit'12 */
		if (rx_info.chip_id >= CHIP_ID_TL1)
			hdmirx_wr_top(TOP_SW_RESET, 0x1000, port);
		else
			/* For txlx and previous chips,the sw_reset_hdcp22 is bit'8 */
			hdmirx_wr_top(TOP_SW_RESET, 0x100, port);
		rx_pr("before releas\n");
		mdelay(1);
		hdmirx_wr_top(TOP_SW_RESET, 0x0, port);
	} else {
		rx_pr("do not kill\n");
	}
	rx_pr("esm rst\n");
}

/*
 * hdmirx_hdcp22_init - hdcp2.2 initialization
 */
void rx_is_hdcp22_support(void)
{
	int temp;

	if (rx_info.chip_id >= CHIP_ID_T7)
		return;
	temp = rx_sec_set_duk(hdmirx_repeat_support());
	if (temp > 0) {
		rx_hdcp22_wr_top(TOP_SKP_CNTL_STAT, 7);
		hdcp22_on = 1;
		if (temp == 2)
			rx_pr("2.2 test key!!!\n");
	} else {
		hdcp22_on = 0;
	}
	rx_pr("hdcp22 == %d\n", hdcp22_on);
}

/*
 * kill esm may not executed in rx22
 * kill esm in driver when 2.2 off
 * refer to ESM_Kill->esm_hostlib_mb_cmd
 */

void rx_kill_esm(void)
{
	rx_hdcp22_wr_reg(0x28, 9);
}

/*
 * hdmirx_hdcp22_hpd - set hpd level for hdcp2.2
 * @value: whether to set hpd high
 */
void hdmirx_hdcp22_hpd(bool value)
{
	if (rx_info.chip_id >= CHIP_ID_T7)
		return;
	unsigned long data32 = hdmirx_rd_dwc(DWC_HDCP22_CONTROL);

	if (value)
		data32 |= 0x1000;
	else
		data32 &= (~0x1000);
	hdmirx_wr_dwc(DWC_HDCP22_CONTROL, data32);
}

/*
 * hdcp_22_off
 */
void hdcp_22_off(void)
{
	if (rx_info.chip_id >= CHIP_ID_T7) {
		return;
	} else {
		/* note: can't pull down hpd before enter suspend */
		/* it will stop cec wake up func if EE domain still working */
		/* rx_set_cur_hpd(0); */
		hpd_to_esm = 0;
		hdmirx_wr_dwc(DWC_HDCP22_CONTROL, 0x0);
	//if (hdcp22_kill_esm == 0)
	hdmirx_hdcp22_esm_rst();
	//else
		//hdcp22_kill_esm = 0;
	hdcp22_clk_en(0);
	}
	rx_pr("hdcp22 off\n");
}

/*
 * hdcp_22_on
 */
void hdcp_22_on(void)
{
	if (rx_info.chip_id >= CHIP_ID_T7) {
		//TODO..
	} else {
		hdcp22_kill_esm = 0;
		/* switch_set_state(&rx[rx_info.main_port].hpd_sdev, 0x0); */
		rx_hdcp22_send_uevent(0);
		hdcp22_clk_en(1);
		hdmirx_wr_dwc(DWC_HDCP22_CONTROL, 0x1000);
		/* rx_hdcp22_wr_top(TOP_SKP_CNTL_STAT, 0x1); */
		/* hdmirx_hw_config(); */
		/* switch_set_state(&rx[rx_info.main_port].hpd_sdev, 0x1); */
		rx_hdcp22_send_uevent(1);
		hpd_to_esm = 1;
		/* don't need to delay 900ms to wait sysctl start hdcp_rx22,*/
		/*sysctl is userspace it wakes up later than driver */
		/* mdelay(900); */
		/* rx_set_cur_hpd(1); */
	}
	rx_pr("hdcp22 on\n");
}

/*
 * clk_init - clock initialization
 * config clock for hdmirx module
 */
void clk_init_cor(void)
{
	switch (rx_info.chip_id) {
	case CHIP_ID_T3:
	case CHIP_ID_T7:
	case CHIP_ID_T5W:
		clk_init_cor_t7();
		break;
	case CHIP_ID_T5M:
		clk_init_cor_t5m();
		break;
	case CHIP_ID_T3X:
		clk_init_cor_t3x();
		break;
	case CHIP_ID_TXHD2:
		clk_init_cor_txhd2();
		break;
	default:
		rx_pr("%s err\n", __func__);
		break;
	}
}

void clk_init_dwc(void)
{
	u32 data32;
	u8 port = rx_info.main_port;

	/* DWC clock enable */
	/* Turn on clk_hdmirx_pclk, also = sysclk */
	wr_reg_hhi(HHI_GCLK_MPEG0,
		   rd_reg_hhi(HHI_GCLK_MPEG0) | (1 << 21));

	hdmirx_wr_ctl_port(0, 0x93ff);
	hdmirx_wr_ctl_port(0x10, 0x93ff);

	if (rx_info.chip_id == CHIP_ID_TXLX ||
	    rx_info.chip_id == CHIP_ID_TXHD ||
	    rx_info.chip_id >= CHIP_ID_TL1) {
		data32  = 0;
		data32 |= (0 << 15);
		data32 |= (1 << 14);
		data32 |= (0 << 5);
		data32 |= (0 << 4);
		data32 |= (0 << 0);
		if (rx_info.chip_id >= CHIP_ID_T5) {
			wr_reg_clk_ctl(HHI_AUD_PLL_CLK_OUT_CNTL, data32);
			data32 |= (1 << 4);
			wr_reg_clk_ctl(HHI_AUD_PLL_CLK_OUT_CNTL, data32);
		} else {
			wr_reg_hhi(HHI_AUD_PLL_CLK_OUT_CNTL, data32);
			data32 |= (1 << 4);
			wr_reg_hhi(HHI_AUD_PLL_CLK_OUT_CNTL, data32);
		}
	}
	data32 = hdmirx_rd_top(TOP_CLK_CNTL, port);
	data32 |= 0 << 31;  /* [31]     disable clk_gating */
	data32 |= 1 << 17;  /* [17]     aud_fifo_rd_en */
	data32 |= 1 << 16;  /* [16]     pkt_fifo_rd_en */
	if (rx_info.chip_id >= CHIP_ID_TL1) {
		data32 |= 0 << 8;   /* [8]      tmds_ch2_clk_inv */
		data32 |= 0 << 7;   /* [7]      tmds_ch1_clk_inv */
		data32 |= 0 << 6;   /* [6]      tmds_ch0_clk_inv */
		data32 |= 0 << 5;   /* [5]      pll4x_cfg */
		data32 |= 0 << 4;   /* [4]      force_pll4x */
		data32 |= 0 << 3;   /* [3]      phy_clk_inv: 1-invert */
	} else {
		data32 |= 1 << 2;   /* [2]      hdmirx_cec_clk_en */
		data32 |= 0 << 1;   /* [1]      bus_clk_inv */
		data32 |= 0 << 0;   /* [0]      hdmi_clk_inv */
	}
	hdmirx_wr_top(TOP_CLK_CNTL, data32, port);    /* DEFAULT: {32'h0} */
}

void clk_init(void)
{
	if (rx_info.chip_id >= CHIP_ID_T7)
		clk_init_cor();
	else
		clk_init_dwc();
}

/*
 * hdmirx_20_init - hdmi2.0 config
 */
void hdmirx_20_init(void)
{
	unsigned long data32;
	bool scdc_en =
		scdc_force_en ? 1 : get_edid_selection(rx_info.main_port);

	data32 = 0;
	data32 |= 1	<< 12; /* [12]     vid_data_checken */
	data32 |= 1	<< 11; /* [11]     data_island_checken */
	data32 |= 1	<< 10; /* [10]     gb_checken */
	data32 |= 1	<< 9;  /* [9]      preamb_checken */
	data32 |= 1	<< 8;  /* [8]      ctrl_checken */
	data32 |= scdc_en	<< 4;  /* [4]      scdc_enable */
	/* To support some TX that sends out SSCP even when not scrambling:
	 * 0: Original behaviour
	 * 1: During TMDS character error detection, treat SSCP character
	 *	as normal TMDS character.
	 * Note: If scramble is turned on, this bit will not take effect,
	 *	revert to original IP behaviour.
	 */
	data32 |= ignore_sscp_charerr << 3; /* [3]ignore sscp character err */
	/* To support some TX that sends out SSCP even when not scrambling:
	 * 0: Original behaviour
	 * 1: During TMDS decoding, treat SSCP character
	 * as normal TMDS character
	 * Note: If scramble is turned on, this bit will not take effect,
	 * revert to original IP behaviour.
	 */
	data32 |= ignore_sscp_tmds << 2;  /* [2]	   ignore sscp tmds */
	data32 |= SCRAMBLE_SEL	<< 0;  /* [1:0]    scramble_sel */
	hdmirx_wr_dwc(DWC_HDMI20_CONTROL,    data32);

	data32  = 0;
	data32 |= 1	<< 24; /* [25:24]  i2c_spike_suppr */
	data32 |= 0	<< 20; /* [20]     i2c_timeout_en */
	data32 |= 0	<< 0;  /* [19:0]   i2c_timeout_cnt */
	hdmirx_wr_dwc(DWC_SCDC_I2C_CONFIG,    data32);

	data32  = 0;
	data32 |= 1    << 1;  /* [1]      hpd_low */
	data32 |= 0    << 0;  /* [0]      power_provided */
	hdmirx_wr_dwc(DWC_SCDC_CONFIG,   data32);

	data32  = 0;
	data32 |= 0 << 8;  /* [31:8]   manufacture_oui */
	data32 |= 1	<< 0;  /* [7:0]    sink_version */
	hdmirx_wr_dwc(DWC_SCDC_WRDATA0,	data32);

	data32  = 0;
	data32 |= 10	<< 20; /* [29:20]  chlock_max_err */
	data32 |= 24000	<< 0;  /* [15:0]   mili_sec_timer_limit */
	hdmirx_wr_dwc(DWC_CHLOCK_CONFIG, data32);

	/* hdcp2.2 ctl */
	if (hdcp22_on) {
		/* set hdcp_hpd high later */
		if (hdcp_hpd_ctrl_en)
			hdmirx_wr_dwc(DWC_HDCP22_CONTROL, 0);
		else
			hdmirx_wr_dwc(DWC_HDCP22_CONTROL, 0x1000);
	} else {
		hdmirx_wr_dwc(DWC_HDCP22_CONTROL, 2);
	}
}

/*
 * hdmirx_audio_init - audio initialization
 */
int hdmirx_audio_init(void)
{
	/* 0=I2S 2-channel; 1=I2S 4 x 2-channel. */
	int err = 0;
	unsigned long data32 = 0;

	/*
	 *recover to default value, bit[27:24]
	 *set aud_pll_lock filter
	 *data32  = 0;
	 *data32 |= 0 << 28;
	 *data32 |= 0 << 24;
	 *hdmirx_wr_dwc(DWC_AUD_PLL_CTRL, data32);
	 */

	/* AFIFO depth 1536word.*/
	/*increase start threshold to middle position */
	data32  = 0;
	data32 |= 160 << 18; /* start */
	data32 |= 200	<< 9; /* max */
	data32 |= 8	<< 0; /* min */
	hdmirx_wr_dwc(DWC_AUD_FIFO_TH, data32);

	data32  = 0;
	data32 |= 1	<< 16;
	data32 |= 0	<< 0;
	hdmirx_wr_dwc(DWC_AUD_FIFO_CTRL, data32);

	data32  = 0;
	data32 |= 0	<< 8;
	data32 |= 0	<< 7;
	data32 |= 0 << 2;
	data32 |= 1	<< 0;
	hdmirx_wr_dwc(DWC_AUD_CHEXTR_CTRL, data32);

	data32 = 0;
	/* [22:21]	a port_sh_dw_ctrl */
	data32 |= 3	<< 21;
	/* [20:19]  auto_aclk_mute */
	data32 |= auto_aclk_mute	<< 19;
	/* [16:10]  aud_mute_speed */
	data32 |= 1	<< 10;
	/* [7]      aud_avmute_en */
	data32 |= aud_avmute_en	<< 7;
	/* [6:5]    aud_mute_sel */
	data32 |= aud_mute_sel	<< 5;
	/* [4:3]    aud_mute_mode */
	data32 |= 1	<< 3;
	/* [2:1]    aud_t_tone_fs_sel */
	data32 |= 0	<< 1;
	/* [0]      testtone_en */
	data32 |= 0	<< 0;
	hdmirx_wr_dwc(DWC_AUD_MUTE_CTRL, data32);

	/* recover to default value.*/
	/*remain code for some time.*/
	/*if no side effect then remove it */
	/*
	 *data32 = 0;
	 *data32 |= 0	<< 16;
	 *data32 |= 0	<< 12;
	 *data32 |= 0	<< 4;
	 *data32 |= 0	<< 1;
	 *data32 |= 0	<< 0;
	 *hdmirx_wr_dwc(DWC_AUD_PAO_CTRL,   data32);
	 */

	/* recover to default value.*/
	/*remain code for some time.*/
	/*if no side effect then remove it */
	/*
	 *data32  = 0;
	 *data32 |= 0	<< 8;
	 *hdmirx_wr_dwc(DWC_PDEC_AIF_CTRL,  data32);
	 */

	data32  = 0;
	/* [4:2]    deltacts_irqtrig */
	data32 |= 0 << 2;
	/* [1:0]    cts_n_meas_mode */
	data32 |= 0 << 0;
	/* DEFAULT: {27'd0, 3'd0, 2'd1} */
	hdmirx_wr_dwc(DWC_PDEC_ACRM_CTRL, data32);

	/* unsupport HBR serial mode. invalid bit */
	if (rx_info.chip_id < CHIP_ID_TM2)
		hdmirx_wr_bits_dwc(DWC_AUD_CTRL, DWC_AUD_HBR_ENABLE, 1);

	/* SAO cfg, disable I2S output, no use */
	data32 = 0;
	data32 |= 1	<< 10;
	data32 |= 0	<< 9;
	data32 |= 0x0f	<< 5;
	data32 |= 0	<< 1;
	data32 |= 1	<< 0;
	hdmirx_wr_dwc(DWC_AUD_SAO_CTRL, data32);

	data32  = 0;
	data32 |= 1	<< 6;
	data32 |= 0xf	<< 2;
	hdmirx_wr_dwc(DWC_PDEC_ASP_CTRL, data32);

	return err;
}

/*
 * snps phy g3 initial
 */
void snps_phyg3_init(void)
{
	//u8 port = rx_info.main_port;

	u32 data32;
	u32 term_value =
		hdmirx_rd_top_common(TOP_HPD_PWR5V);

	data32 = 0;
	data32 |= 1 << 6;
	data32 |= 1 << 4;
	data32 |= rx_info.main_port << 2;
	data32 |= 1 << 1;
	data32 |= 1 << 0;
	hdmirx_wr_dwc(DWC_SNPS_PHYG3_CTRL, data32);
	usleep_range(1000, 1010);

	data32	= 0;
	data32 |= 1 << 6;
	data32 |= 1 << 4;
	data32 |= rx_info.main_port << 2;
	data32 |= 1 << 1;
	data32 |= 0 << 0;
	hdmirx_wr_dwc(DWC_SNPS_PHYG3_CTRL, data32);

	data32 = 0;
	data32 |= 6 << 10;
	data32 |= 1 << 9;
	data32 |= ((24000 * 4) / 1000);
	hdmirx_wr_phy(PHY_CMU_CONFIG, data32);

	hdmirx_wr_phy(PHY_VOLTAGE_LEVEL, 0x1ea);

	data32 = 0;
	data32 |= 0	<< 15;
	data32 |= 0	<< 13;
	data32 |= 0	<< 12;
	data32 |= phy_fast_switching << 11;
	data32 |= 0	<< 10;
	data32 |= phy_fsm_enhancement << 9;
	data32 |= 0	<< 8;
	data32 |= 0	<< 7;
	data32 |= 0 << 5;
	data32 |= 0	<< 3;
	data32 |= 0 << 2;
	data32 |= 0 << 0;
	hdmirx_wr_phy(PHY_SYSTEM_CONFIG, data32);

	hdmirx_wr_phy(MPLL_PARAMETERS2,	0x1c94);
	hdmirx_wr_phy(MPLL_PARAMETERS3,	0x3713);
	/*default 0x24da , EQ optimizing for kaiboer box */
	hdmirx_wr_phy(MPLL_PARAMETERS4,	0x24dc);
	hdmirx_wr_phy(MPLL_PARAMETERS5,	0x5492);
	hdmirx_wr_phy(MPLL_PARAMETERS6,	0x4b0d);
	hdmirx_wr_phy(MPLL_PARAMETERS7,	0x4760);
	hdmirx_wr_phy(MPLL_PARAMETERS8,	0x008c);
	hdmirx_wr_phy(MPLL_PARAMETERS9,	0x0010);
	hdmirx_wr_phy(MPLL_PARAMETERS10, 0x2d20);
	hdmirx_wr_phy(MPLL_PARAMETERS11, 0x2e31);
	hdmirx_wr_phy(MPLL_PARAMETERS12, 0x4b64);
	hdmirx_wr_phy(MPLL_PARAMETERS13, 0x2493);
	hdmirx_wr_phy(MPLL_PARAMETERS14, 0x676d);
	hdmirx_wr_phy(MPLL_PARAMETERS15, 0x23e0);
	hdmirx_wr_phy(MPLL_PARAMETERS16, 0x001b);
	hdmirx_wr_phy(MPLL_PARAMETERS17, 0x2218);
	hdmirx_wr_phy(MPLL_PARAMETERS18, 0x1b25);
	hdmirx_wr_phy(MPLL_PARAMETERS19, 0x2492);
	hdmirx_wr_phy(MPLL_PARAMETERS20, 0x48ea);
	hdmirx_wr_phy(MPLL_PARAMETERS21, 0x0011);
	hdmirx_wr_phy(MPLL_PARAMETERS22, 0x04d2);
	hdmirx_wr_phy(MPLL_PARAMETERS23, 0x0414);

	/* Configuring I2C to work in fastmode */
	hdmirx_wr_dwc(DWC_I2CM_PHYG3_MODE,	 0x1);
	/* disable overload protect for Philips DVD */
	/* NOTE!!!!! don't remove below setting */
	hdmirx_wr_phy(OVL_PROT_CTRL, 0xa);

	/* clear clk_rate cfg */
	hdmirx_wr_bits_phy(PHY_CDR_CTRL_CNT, CLK_RATE_BIT, 0);
	/*last_clk_rate = 0;*/
	rx[rx_info.main_port].phy.clk_rate = 0;
	/* enable all ports's termination */
	data32 = 0;
	data32 |= 1 << 8;
	data32 |= ((term_value & 0xF) << 4);
	hdmirx_wr_phy(PHY_MAIN_FSM_OVERRIDE1, data32);

	data32 = 0;
	data32 |= 1 << 6;
	data32 |= 1 << 4;
	data32 |= rx_info.main_port << 2;
	data32 |= 0 << 1;
	data32 |= 0 << 0;
	hdmirx_wr_dwc(DWC_SNPS_PHYG3_CTRL, data32);
}

void rx_run_eq(u8 port)
{
	if (rx_info.chip_id < CHIP_ID_TL1)
		rx_eq_algorithm();
	else
		hdmirx_phy_init(port);
}

bool rx_eq_done(u8 port)
{
	bool ret = true;

	if (rx_get_eq_run_state(port) == E_EQ_START)
		ret = false;
	return ret;
}

void aml_phy_offset_cal(void)
{
	switch (rx_info.phy_ver) {
	case PHY_VER_T5:
		aml_phy_offset_cal_t5();
		break;
	case PHY_VER_T7:
	case PHY_VER_T3:
	case PHY_VER_T5W:
		aml_phy_offset_cal_t7();
	break;
	case PHY_VER_T5M:
		aml_phy_offset_cal_t5m();
		break;
	case PHY_VER_T3X:
		aml_phy_offset_cal_t3x();
		break;
	case PHY_VER_TXHD2:
		aml_phy_offset_cal_txhd2();
		break;
	default:
		break;
	}
}

/*
 * rx_clkrate_monitor - clock ratio monitor
 * detect SCDC tmds clk ratio changes and
 * update phy setting
 */
bool rx_clk_rate_monitor(u8 port)
{
	u32 clk_rate, phy_band, pll_band;
	bool changed = false;
	int i;
	int error = 0;

	clk_rate = rx_get_scdc_clkrate_sts(port);
	/* should rm squelch judgement for low-amplitude issue */
	/* otherwise,sw can not detect the low-amplitude signal */
	/* if (rx[rx_info.main_port].state < FSM_WAIT_CLK_STABLE) */
		/*return changed;*/
	/*if (is_clk_stable()) { */
	pll_band = aml_phy_pll_band(rx[port].clk.cable_clk, clk_rate);
	phy_band = aml_cable_clk_band(rx[port].clk.cable_clk, clk_rate);
	if (rx[port].phy.pll_bw != pll_band ||
	    rx[port].phy.phy_bw != phy_band) {
		rx[port].phy.cablesel = 0;
		rx[port].phy.phy_bw = phy_band;
		rx[port].phy.pll_bw = pll_band;
		changed = true;
	}

	if (clk_rate != rx[port].phy.clk_rate) {
		changed = true;
		if (log_level & VIDEO_LOG)
			rx_pr("clk_rate:%d, last_clk_rate: %d\n",
					clk_rate, rx[port].phy.clk_rate);
		rx[port].phy.clk_rate = clk_rate;
	}
	if (changed) {
		rx[port].cableclk_stb_flg = false;
		i2c_err_cnt[port] = 0;
		if (rx_info.chip_id < CHIP_ID_TL1) {
			for (i = 0; i < 3; i++) {
				error = hdmirx_wr_bits_phy(PHY_CDR_CTRL_CNT,
							   CLK_RATE_BIT, clk_rate);
				if (error == 0)
					break;
			}
		} else {
			hdmirx_phy_init(port);
		}
	}
	return changed;
}

static void hdmirx_cor_reset(void)
{
	ulong flags;
	unsigned long dev_offset = 0;

	if (rx_info.chip_id < CHIP_ID_T7 || rx_info.chip_id >= CHIP_ID_T3X)
		return;
	spin_lock_irqsave(&reg_rw_lock, flags);
	dev_offset = rx_reg_maps[MAP_ADDR_MODULE_TOP].phy_addr;
	wr_reg(MAP_ADDR_MODULE_TOP, dev_offset + TOP_SW_RESET, 1);
	udelay(1);
	wr_reg(MAP_ADDR_MODULE_TOP, dev_offset + TOP_SW_RESET, 0);
	spin_unlock_irqrestore(&reg_rw_lock, flags);
}

void rx_afifo_monitor(u8 port)
{
	if (rx[port].state != FSM_SIG_READY || port == rx_info.sub_port)
		return;

	if (rx_info.chip_id < CHIP_ID_T7) {
		if (rx[port].aud_info.auds_layout || rx[port].aud_info.aud_hbr_rcv) {
			if (rx[port].aud_info.afifo_cfg) {
				dump_audio_status(port);
				rx_afifo_store_valid(false, port);
				hdmirx_audio_fifo_rst(port);
			}
		} else {
			if (!rx[port].aud_info.afifo_cfg) {
				dump_audio_status(port);
				rx_afifo_store_valid(true, port);
				hdmirx_audio_fifo_rst(port);
			}
		}
		return;
	}

	if (rx_afifo_dbg_en) {
		afifo_overflow_cnt = 0;
		afifo_underflow_cnt = 0;
		return;
	}
	rx[port].afifo_sts = hdmirx_rd_cor(RX_INTR4_PWD_IVCRX, port) & 3;
	hdmirx_wr_cor(RX_INTR4_PWD_IVCRX, 3, port);
	if (rx[port].afifo_sts & 2) {
		afifo_overflow_cnt++;
		hdmirx_audio_fifo_rst(port);
		if (log_level & AUDIO_LOG)
			rx_pr("overflow\n");
	} else if (rx[port].afifo_sts & 1) {
		afifo_underflow_cnt++;
		hdmirx_audio_fifo_rst(port);
		if (log_level & AUDIO_LOG)
			rx_pr("underflow\n");
	} else {
		if (afifo_overflow_cnt)
			afifo_overflow_cnt--;
		if (afifo_underflow_cnt)
			afifo_underflow_cnt--;
	}
	if (rx_info.chip_id < CHIP_ID_T5M) {
		if (afifo_overflow_cnt > 600) {
			afifo_overflow_cnt = 0;
			hdmirx_output_en(false);
			hdmirx_cor_reset();
			//hdmirx_hbr2spdif(0);
			//rx_set_cur_hpd(0, 5);
			//rx[rx_info.main_port].state = FSM_5V_LOST;
			rx_pr("!!force reset\n");
		}
	}
	//if (afifo_underflow_cnt) {
		//afifo_underflow_cnt = 0;
		//rx_aud_fifo_rst();
		//rx_pr("!!pll rst\n");
	//}
}

void rx_hdcp_monitor(u8 port)
{
	static u8 sts1, sts2, sts3;
	u8 tmp;

	if (rx_info.chip_id < CHIP_ID_T7)
		return;
	if (rx[port].hdcp.hdcp_version == HDCP_VER_NONE)
		return;
	if (rx[port].state < FSM_SIG_STABLE)
		return;

	rx_get_ecc_info(port);
	if (rx[port].ecc_err &&
		rx[port].ecc_pkt_cnt == rx[port].ecc_err) {
		if (log_level & VIDEO_LOG)
			rx_pr("ecc:%d-%d\n", rx[port].ecc_err,
				  rx[port].ecc_pkt_cnt);
		skip_frame(1, port, "hdcp ecc err");
		rx[port].ecc_err_frames_cnt++;
	} else {
		rx[port].ecc_err_frames_cnt = 0;
	}
	if (rx[port].ecc_err_frames_cnt >= rx_ecc_err_frames) {
		if (rx[port].hdcp.hdcp_version == HDCP_VER_22)
			rx_hdcp_22_sent_reauth(port);
		else if (rx[port].hdcp.hdcp_version == HDCP_VER_14)
			rx_hdcp_14_sent_reauth(port);
		rx_pr("reauth-err:%d\n", rx[port].ecc_err);
		rx[port].hdcp.hdcp_version = HDCP_VER_NONE;
		rx[port].state = FSM_SIG_WAIT_STABLE;
		rx[port].ecc_err = 0;
		rx[port].ecc_err_frames_cnt = 0;
	}
	//hdcp14 status
	tmp = hdmirx_rd_cor(RX_HDCP_STAT_HDCP1X_IVCRX, port);
	if (tmp == 2 || tmp == 0x0a) {
		rx_pr("hdcp1sts %x->%x\n", sts1, tmp);
		sts1 = tmp;
	}
	tmp = hdmirx_rd_cor(CP2PAX_AUTH_STAT_HDCP2X_IVCRX, port);
	if (tmp != sts2 && (log_level & HDCP_LOG)) {
		rx_pr("hdcp2sts %x->%x\n", sts2, tmp);
		sts2 = tmp;
	}
	tmp = hdmirx_rd_cor(CP2PAX_STATE_HDCP2X_IVCRX, port);
	if (tmp != sts3 && (log_level & HDCP_LOG)) {
		rx_pr("hdcp2sts3 %x->%x\n", sts3, tmp);
		sts3 = tmp;
	}
}

bool rx_special_func_en(u8 port)
{
	bool ret = false;

	if (rx_info.chip_id <= CHIP_ID_T7)
		return ret;

#ifdef CVT_DEF_FIXED_HPD_PORT
	if (port == E_PORT0 && ((CVT_DEF_FIXED_HPD_PORT & (1 << E_PORT0)) != 0))
		ret = true;

	if (rx_info.boot_flag && port == E_PORT0) {
		if (hdmirx_rd_cor(SCDCS_TMDS_CONFIG_SCDC_IVCRX) & 2)
			ret = true;
		//no hdcp
		rx_pr("pc port first boot\n");
	}
#endif

	return ret;
}

bool rx_sw_scramble_en(void)
{
	bool ret = false;

	if (rx_info.main_port == ops_port) {
		ret = true;
		rx_pr("ops port in\n");
	}

	return ret;
}

/*
 * rx_hdcp_init - hdcp1.4 init and enable
 */
void rx_hdcp_init(void)
{
	if (hdcp_enable)
		rx_hdcp14_config(&rx[rx_info.main_port].hdcp);
	else
		hdmirx_wr_bits_dwc(DWC_HDCP_CTRL, ENCRIPTION_ENABLE, 0);
}

/* need reset bandgap when
 * aud_clk=0 & req_clk!=0
 * according to analog team's request
 */
void rx_audio_bandgap_rst(void)
{
	if (rx_info.chip_id >= CHIP_ID_TL1) {
		wr_reg_hhi_bits(HHI_VDAC_CNTL0, _BIT(10), 1);
		udelay(10);
		wr_reg_hhi_bits(HHI_VDAC_CNTL0, _BIT(10), 0);
	} else {
		wr_reg_hhi_bits(HHI_VDAC_CNTL0_TXLX, _BIT(13), 1);
		udelay(10);
		wr_reg_hhi_bits(HHI_VDAC_CNTL0_TXLX, _BIT(13), 0);
	}
	if (log_level & AUDIO_LOG)
		rx_pr("%s\n", __func__);
}

void rx_audio_bandgap_en(void)
{
	if (rx_info.chip_id >= CHIP_ID_TL1) {
		/* for tl1/tm2 0:bg on   1: bg off */
		wr_reg_hhi_bits(HHI_VDAC_CNTL1, _BIT(7), 0);

		wr_reg_hhi_bits(HHI_VDAC_CNTL0, _BIT(10), 1);
		udelay(10);
		wr_reg_hhi_bits(HHI_VDAC_CNTL0, _BIT(10), 0);
	} else {
		/* for txlx/txl... 1:bg on   0: bg off */
		wr_reg_hhi_bits(HHI_VDAC_CNTL0_TXLX, _BIT(9), 1);

		wr_reg_hhi_bits(HHI_VDAC_CNTL0_TXLX, _BIT(13), 1);
		udelay(10);
		wr_reg_hhi_bits(HHI_VDAC_CNTL0_TXLX, _BIT(13), 0);
	}
	if (log_level & AUDIO_LOG)
		rx_pr("%s\n", __func__);
}

void rx_sw_reset(int level, u8 port)
{
	unsigned long data32 = 0;

	if (rx_info.chip_id >= CHIP_ID_T7) {
		rx_sw_reset_t7(level, port);
	} else {
		if (level == 1) {
			data32 |= 0 << 7;	/* [7]vid_enable */
			data32 |= 0 << 5;	/* [5]cec_enable */
			data32 |= 0 << 4;	/* [4]aud_enable */
			data32 |= 0 << 3;	/* [3]bus_enable */
			data32 |= 0 << 2;	/* [2]hdmi_enable */
			data32 |= 1 << 1;	/* [1]modet_enable */
			data32 |= 0 << 0;	/* [0]cfg_enable */

		} else if (level == 2) {
			data32 |= 0 << 7;	/* [7]vid_enable */
			data32 |= 0 << 5;	/* [5]cec_enable */
			data32 |= 1 << 4;	/* [4]aud_enable */
			data32 |= 0 << 3;	/* [3]bus_enable */
			data32 |= 1 << 2;	/* [2]hdmi_enable */
			data32 |= 1 << 1;	/* [1]modet_enable */
			data32 |= 0 << 0;	/* [0]cfg_enable */
		}
		hdmirx_wr_dwc(DWC_DMI_SW_RST, data32);
		hdmirx_audio_fifo_rst(port);
		hdmirx_packet_fifo_rst();
	}
}

void rx_esm_reset(int level)
{
	if (rx_info.chip_id >= CHIP_ID_T7 || !hdcp22_on)
		return;
	if (level == 0) {//for state machine
		esm_set_stable(false);
		if (esm_recovery_mode
			== ESM_REC_MODE_RESET)
			esm_set_reset(true);
		/* for some hdcp2.2 devices which
		 * don't retry 2.2 interaction
		 * continuously and don't response
		 * to re-auth, such as chroma 2403,
		 * esm needs to be on work even
		 * before tmds is valid so that to
		 * not miss 2.2 interaction
		 */
		/* else */
			/* rx_esm_tmds_clk_en(false); */
	} else if (level == 1) {//for open port
		esm_set_stable(false);
		esm_set_reset(true);
		if (esm_recovery_mode == ESM_REC_MODE_TMDS)
			rx_esm_tmds_clk_en(false);
	} else if (level == 2) {//for fsm_restart&signal_status_init
		if (esm_recovery_mode == ESM_REC_MODE_TMDS)
			rx_esm_tmds_clk_en(false);
		esm_set_stable(false);
	} else if (level == 3) {//for dwc_reset
		if (rx[rx_info.main_port].hdcp.hdcp_version != HDCP_VER_14) {
			if (esm_recovery_mode == ESM_REC_MODE_TMDS)
				rx_esm_tmds_clk_en(true);
			esm_set_stable(true);
			if (rx[rx_info.main_port].hdcp.hdcp_version == HDCP_VER_22)
				hdmirx_hdcp22_reauth();
		}
	}
}

void cor_init(u8 port)
{
	u8 data8;
	u32 data32;

	//--------AON REG------
	data8  = 0;
	data8 |= (0 << 4);// [4]   reg_sw_rst_auto
	data8 |= (1 << 0);// [0]   reg_sw_rst
	hdmirx_wr_cor(RX_AON_SRST, data8, port);//register address: 0x0005

	data8  = 0;
	data8 |= (0 << 5);// [5]  reg_scramble_on_ovr
	data8 |= (1 << 4);// [4]  reg_hdmi2_on_ovr
	data8 |= (0 << 2);// [3:2]rsvd
	data8 |= (0 << 1);// [1]  reg_scramble_on_val
	data8 |= (1 << 0);// [0]  reg_hdmi2_on_val
	hdmirx_wr_cor(RX_HDMI2_MODE_CTRL, data8, port);//register address: 0x0040

	data8  = 0;
	data8 |= (0 << 3);// [3] reg_soft_intr_en
	data8 |= (0 << 1);// [1] reg_intr_polarity (default is 1)
	hdmirx_wr_cor(RX_INT_CTRL, data8, port);//register address: 0x0079

	//-------PWD REG-------
	data8  = 0;
	data8 |= (0 << 7);// [7]  reg_mhl3ce_sel_rx
	data8 |= (0 << 6);// [6]  rsvd
	data8 |= (0 << 5);// [5]  reg_vdi_rx_dig_bypass
	data8 |= (0 << 4);// [4]  reg_tmds_mode_inv
	data8 |= (0 << 3);// [3]  reg_bypass_rx2tx_dsc_video
	data8 |= (0 << 2);// [2]  rsvd
	data8 |= (0 << 1);// [1]  rsvd
	data8 |= (0 << 0);// [0]  reg_core_iso_en  TMDS core isolation enable
	hdmirx_wr_cor(RX_PWD_CTRL, data8, port);//register address: 0x1001

	data8  = 0;
	data8 |= (0 << 3);// [4:3] reg_dsc_bypass_align
	data8 |= (0 << 2);// [2]   reg_hv_sync_cntrl
	data8 |= (1 << 1);// [1]   reg_sync_pol
	data8 |= (1 << 0);// [0]   reg_pd_all   0:power down; 1: normal operation
	hdmirx_wr_cor(RX_SYS_CTRL1, data8, port);//register address: 0x1007

	data8  = 0;
	data8 |= (1 << 4);// [5:4]	reg_di_ch2_sel
	data8 |= (0 << 2);// [3:2]	reg_di_ch1_sel
	data8 |= (2 << 0);// [1:0]	reg_di_ch0_sel
	hdmirx_wr_cor(RX_SYS_TMDS_CH_MAP, data8, port);//register address: 0x100E

	data8 = 0;
	data8 |= (0 << 7);//rsvd
	data8 |= (1 << 6);//reg_phy_di_dff_en : enable for dff latching data coming from TMDS phy
	data8 |= (0 << 5);//reg_di_ch2_invt
	data8 |= (1 << 4);//reg_di_ch1_invt
	data8 |= (0 << 3);//reg_di_ch0_invt
	data8 |= (0 << 2);//reg_di_ch2_bsi
	data8 |= (1 << 1);//reg_di_ch1_bsi
	data8 |= (0 << 0);//reg_di_ch0_bsi
	hdmirx_wr_cor(RX_SYS_TMDS_D_IR, data8, port);//register address: 0x100F

	/* deep color clock source */
	data8 = 0;
	data8 |= (0 << 6);//[7:6] reg_pp_status
	data8 |= (1 << 5);//[5]   reg_offset_coen
	data8 |= (0 << 4);//[4]   reg_dc_ctl_ow  //!!!!
	data8 |= (0 << 0);//[3:0] reg_dc_ctl  deep-color clock from the TMDS RX core
	hdmirx_wr_cor(RX_TMDS_CCTRL2, data8, port);//register address: 0x1013

	data8  = 0;
	data8 |= (1 << 7);//reg_tst_x_clk 1:Crystal oscillator clock muxed to test output pin
	data8 |= (0 << 6);//reg_tst_ckdt 1:CKDT muxed to test output pin
	data8 |= (0 << 5);//reg_invert_tclk
	hdmirx_wr_cor(RX_TEST_STAT, data8, port);//register address: 0x103b (0x80)

	data8  = 0;
	data8 |= (0 << 3);//[5:3] divides the vpc out clock
	//[2:0] divides the vpc core clock:
	//0: divide by 1; 1: divide by 2; 3: divide by 4; 7: divide by 8
	data8 |= (1 << 0);
	hdmirx_wr_cor(RX_PWD0_CLK_DIV_0, data8, port) ;//register address: 0x10c1

	data8 = 0;
	data8 |= (0 << 7);// [  7] cbcr_order
	data8 |= (0 << 6);// [  6] yc_demux_polarity
	data8 |= (0 << 5);// [  5] yc_demux_enable
	hdmirx_wr_cor(RX_VP_INPUT_FORMAT_LO, data8, port);

	data8  = 0;
	data8 |= (0 << 3);// [  3] mux_cb_or_cr
	data8 |= (0 << 2);// [  2] mux_420_enable
	data8 |= (0 << 0);// [1:0] input_pixel_rate
	hdmirx_wr_cor(RX_VP_INPUT_FORMAT_HI, data8, port);

	//===hdcp 1.4 needed
	hdmirx_wr_cor(RX_SW_HDMI_MODE_PWD_IVCRX, 0x04, port);//register address: 0x1022

	//[0] reg_ext_mclk_en; 1 select external mclk; 0: select internal dacr mclk
	hdmirx_wr_cor(EXT_MCLK_SEL_PWD_IVCRX, 0x01, port);//register address: 0x10c6

	//DEPACK
	data8  = 0;
	data8 |= (1 << 4);//[4] reg_wait4_two_avi_pkts  default is 1, but need two packet
	data8 |= (0 << 0);//[0] reg_all_if_clr_en
	hdmirx_wr_cor(VSI_CTRL4_DP3_IVCRX, data8, port);   //register_address: 0x120f

	// ------PHY CLK/RST-----
	hdmirx_wr_cor(PWD0_CLK_EN_1_PHYCK_IVCRX, 0xff, port);//register address: 0x20a3
	hdmirx_wr_cor(PWD0_CLK_EN_2_PHYCK_IVCRX, 0x3f, port);//register address: 0x20a4
	hdmirx_wr_cor(PWD0_CLK_EN_3_PHYCK_IVCRX, 0x01, port);//register address: 0x20a5

	// for t3x 2.1 port
	hdmirx_wr_cor(PWD0_CLK_EN_4_PHYCK_IVCRX, 0x04, port);//register address: 0x20a6

	hdmirx_wr_cor(RX_AON_SRST_AON_IVCRX, 0x00, port);//reset
	hdmirx_wr_cor(RX_PWD_INT_CTRL, 0x00, port);//[1] reg_intr_polarity, default = 1
	//-------------------
	//  vp core config
	//-------------------
	//invecase Rx 422 output directly
	//mux 422 12bit*2 in to 8bit*3 out
	data8  = 0;
	data8 |= (0	   << 3);// de_polarity
	data8 |= (0	   << 2);// rsvd
	data8 |= (0	   << 1);// hsync_polarity
	data8 |= (0	   << 0);// vsync_polarity
	hdmirx_wr_cor(VP_OUTPUT_SYNC_CFG_VID_IVCRX, data8, port);//register address: 0x1842

	data32 = 0;
	//data32 |= (((rx_color_format==HDMI_COLOR_FORMAT_422)?3:2)   << 9);
	data32 |= (2 << 9);
	// [11: 9] select_cr: 0=ch1(Y); 1=ch0(Cb); 2=ch2(Cr); 3={ch2 8-b,ch0 4-b}(422).
	//data32 |= (((rx_color_format==HDMI_COLOR_FORMAT_422)?3:1)   << 6);
	data32 |= (1 << 6);
	// [ 8: 6] select_cb: 0=ch1(Y); 1=ch0(Cb); 2=ch2(Cr); 3={ch2 8-b,ch0 4-b}(422).
	//data32 |= (((rx_color_format==HDMI_COLOR_FORMAT_422)?3:0)   << 3);
	data32 |= (0 << 3);
	// [ 5: 3] select_y : 0=ch1(Y); 1=ch0(Cb); 2=ch2(Cr); 3={ch1 8-b,ch0 4-b}(422).
	data32 |= (0 << 2);// [    2] reverse_cr
	data32 |= (0 << 1);// [    1] reverse_cb
	data32 |= (0 << 0);// [    0] reverse_y
	hdmirx_wr_cor(VP_INPUT_MAPPING_VID_IVCRX, data32 & 0xff, port);
	hdmirx_wr_cor(VP_INPUT_MAPPING_VID_IVCRX + 1, (data32 >> 8) & 0xff, port);

	//------------------
	// audio I2S config
	//------------------
	data8 = 0;
	data8 |= (5 << 4);//reg_vres_xclk_diff
	data8 |= (0 << 0);//reg_vid_xlckpclk_en
	hdmirx_wr_cor(VID_XPCLK_EN_AUD_IVCRX, data8, port);//register address: 0x1468 (0x50)

	data8 = 0xc; //[5:0] reg_post_val_sw
	hdmirx_wr_cor(RX_POST_SVAL_AUD_IVCRX, data8, port);//register address: 0x1411 (0x0c)

	data8 = 0;
	data8 |= (1 << 6);//[7:6] reg_fm_val_sw 0:128*fs; 1: 256*fs; 2:384*fs; 3: 512*fs
	data8 |= (3 << 0);//[5:0] reg_fs_val_sw
	hdmirx_wr_cor(RX_FREQ_SVAL_AUD_IVCRX, data8, port);//register address: 0x1402 (0x43)

	hdmirx_wr_cor(RX_UPLL_SVAL_AUD_IVCRX, 0x00, port);//UPLL_SVAL

	hdmirx_wr_cor(RX_AVG_WINDOW_AUD_IVCRX, 0xff, port);//AVG_WINDOW

	data8 |= (0 << 7); //[7] cts_dropped_auto_en
	data8 |= (0 << 6); //[6] post_hw_sw_sel
	data8 |= (0 << 5); //[5] upll_hw_sw_sel
	data8 |= (0 << 4); //[4] cts_hw_sw_sel: 0=hw; 1=sw.
	data8 |= (0 << 3); //[3] n_hw_sw_sel: 0=hw; 1=sw.
	data8 |= (0 << 2); //[2] cts_reused_auto_en
	data8 |= (0 << 1); //[1] fs_hw_sw_sel: 0=hw; 1=sw.
	data8 |= (0 << 0); //[0] acr_init_wp
	hdmirx_wr_cor(RX_ACR_CTRL1_AUD_IVCRX, data8, port);//register address: 0x1400 (0x7a)

	data8 = 0;
	data8 |= (0 << 6);//[7:6] r_hdmi_aud_sample_f_extn
	data8 |= (0 << 4);//[4]   reg_fs_filter_en
	data8 |= (0 << 0);//[3:0] rhdmi_aud_sample_f
	hdmirx_wr_cor(RX_TCLK_FS_AUD_IVCRX, data8, port);	//register address: 0x1417 (0x0)

	data8 = 0;
	data8 |= (1 << 3);//[6:3] reg_cts_thresh
	data8 |= (1 << 2);//[2] reg_mclk_loopback
	data8 |= (0 << 1);//[1] reg_log_win_ena
	data8 |= (0 << 0);//[0] reg_post_div2_ena
	hdmirx_wr_cor(RX_ACR_CTRL3_AUD_IVCRX, data8, port);//register address: 0x1418 (0xc)

	data8 = 0;
	data8 |= (1 << 7);//[7]  reg_sd3_en
	data8 |= (1 << 6);//[6]  reg_sd2_en
	data8 |= (1 << 5);//[5]  reg_sd1_en
	data8 |= (1 << 4);//[4]  reg_sd0_en
	data8 |= (0 << 3);//[3]  reg_mclk_en
	data8 |= (1 << 2);//[2]  reg_mute_flag
	data8 |= (0 << 1);//[1]  reg_vucp
	data8 |= (0 << 0);//[0]  reg_pcm
	hdmirx_wr_cor(RX_I2S_CTRL2_AUD_IVCRX, data8, port);

	data8 = 0;
	data8 |= (3 << 6);//[7:6] reg_sd3_map
	data8 |= (2 << 4);//[5:4] reg_sd2_map
	data8 |= (1 << 2);//[3:2] reg_sd1_map
	//[1:0] reg_sd0_map : 0 from FIFO#1; 1 from FIFO#2; 2:from FIFO#3; 3: from FIFO#3
	data8 |= (0 << 0);
	hdmirx_wr_cor(RX_I2S_MAP_AUD_IVCRX, data8, port);//register address: 0x1428 (0xe4)

	hdmirx_wr_cor(RX_I2S_CTRL1_AUD_IVCRX, 0x40, port);//I2S_CTRL1
	hdmirx_wr_cor(RX_SW_OW_AUD_IVCRX, 0x00, port);//SW_OW

	hdmirx_wr_cor(RX_AUDO_MUTE_AUD_IVCRX, 0x00, port);//AUDO_MODE
	hdmirx_wr_cor(RX_OW_15_8_AUD_IVCRX, 0x00, port);//OW_15_8
	hdmirx_wr_cor(AAC_MCLK_SEL_AUD_IVCRX, 0x80, port);//MCLK_SEL [5:4]=>00:128*Fs;01:256*Fs

	data8 = 0;
	data8 |= (0 << 7);//[7] enable overwrite length ralated cbit
	data8 |= (0 << 6);//[6] enable sine wave
	data8 |= (1 << 5);//[5] enable hardware mute
	data8 |= (1 << 4);//[4] pass spdif error
	data8 |= (1 << 3);//[3] pass aud error
	data8 |= (1 << 2);//[2] reg_i2s_mode
	data8 |= (1 << 1);//[1] reg_spdif_mode
	data8 |= (1 << 0);//[0] reg_spidf_en
	hdmirx_wr_cor(RX_AUDRX_CTRL_AUD_IVCRX, data8, port);//AUDRX_CTRL

	data8 = 0;
	data8 |= (1 << 1);//[1] dont_clr_sys_intr
	hdmirx_wr_cor(AEC4_CTRL_AUD_IVCRX, data8, port); //AEC4 CTRL

	data8 = 0;
	data8 |= (1 << 7);//ctl acr en
	data8 |= (1 << 6);//aac exp sel
	data8 |= (0 << 5);//[5] reg_aac_out_off_en
	data8 |= (0 << 2);//[2] reserved
	data8 |= (1 << 1);//[1] aac hw auto unmute enable
	data8 |= (1 << 0);//[0] aac hw auto mute enable
	hdmirx_wr_cor(AEC0_CTRL_AUD_IVCRX, data8, port); //AEC0 CTRL

	data8 = 0;
	data8 |= (0 << 7);//[7] H resolution change
	data8 |= (0 << 6);//[6] polarity change
	data8 |= (0 << 5);//[5] change of interlaced
	data8 |= (0 << 4);//[4] change of the FS
	data8 |= (0 << 3);//[3] CTS reused
	data8 |= (0 << 2);//[2] audio fifo overrun
	data8 |= (0 << 1);//[1] audio fifo underrun
	data8 |= (0 << 0);//[0] hdmi mode
	hdmirx_wr_cor(RX_AEC_EN2_AUD_IVCRX, data8, port);//RX_AEC_EN2

	data8 = 0;
	//if(rx_hbr_sel_i2s_spdif == 1){        //hbr_i2s
	//data8 |= (0	      << 4);}
	//else if(rx_hbr_sel_i2s_spdif == 2){   //hbr_spdif
	//data8 |= (0	      << 4);}
	//else{				       //not hbr
	data8 |= (1 << 4);
	//}
	hdmirx_wr_cor(RX_3D_SW_OW2_AUD_IVCRX, data8, port);//duplicate

	hdmirx_wr_cor(RX_PWD_SRST_PWD_IVCRX, 0x1a, port);//SRST = 1
	/* BIT0 AUTO RST AUD FIFO when fifo err */
	hdmirx_wr_cor(RX_PWD_SRST_PWD_IVCRX, 0x01, port);//SRST = 0

	/* TDM cfg */
	hdmirx_wr_cor(RX_TDM_CTRL1_AUD_IVCRX, 0x00, port);
	hdmirx_wr_cor(RX_TDM_CTRL2_AUD_IVCRX, 0x10, port);

	//clr gcp wr; disable hw avmute for [T7,T5M)
	hdmirx_wr_cor(DEC_AV_MUTE_DP2_IVCRX, 0x20, port);
	// hdcp 2x ECC detection enable  mode 3
	hdmirx_wr_cor(HDCP2X_RX_ECC_CTRL, 3, port);
	hdmirx_wr_cor(HDCP2X_RX_ECC_CONS_ERR_THR, 50, port);
	hdmirx_wr_cor(HDCP2X_RX_ECC_FRM_ERR_THR_0, 0x1, port);
	hdmirx_wr_cor(HDCP2X_RX_ECC_FRM_ERR_THR_1, 0x0, port);
	//hdmirx_wr_cor(HDCP2X_RX_ECC_GVN_FRM_ERR_THR_2, 0xff, port);
	//hdmirx_wr_cor(HDCP2X_RX_GVN_FRM, 30, port);

	//DPLL
	if (rx[port].var.frl_rate) {
		//frl_debug todo
		hdmirx_wr_cor(DPLL_CFG6_DPLL_IVCRX, 0x0, port);
		hdmirx_wr_cor(H21RXSB_D2TH_M42H_IVCRX, 0x20, port);
		hdmirx_wr_bits_cor(H21RXSB_GP1_REGISTER_M42H_IVCRX, _BIT(3), 1, port);
		//clk ready threshold
		hdmirx_wr_cor(H21RXSB_DIFF1T_M42H_IVCRX, 0x20, port);
	} else {
		hdmirx_wr_cor(DPLL_CFG6_DPLL_IVCRX, 0x10, port);
		hdmirx_wr_cor(RX_H21_CTRL_PWD_IVCRX, 0x0, port);
	}
	hdmirx_wr_cor(DPLL_HDMI2_DPLL_IVCRX, 0, port);

	hdmirx_wr_cor(HDMI2_MODE_CTRL_AON_IVCRX, 0x11, port);
}

void hdmirx_hbr2spdif(u8 val, u8 port)
{
	/* if (rx_info.chip_id < CHIP_ID_T7) */
	return;

	if (hdmirx_rd_cor(RX_AUDP_STAT_DP2_IVCRX, port) & _BIT(6))
		hdmirx_wr_bits_top(TOP_CLK_CNTL, _BIT(15), val, port);
	else
		hdmirx_wr_bits_top(TOP_CLK_CNTL, _BIT(15), 0, port);
}

void hdmirx_output_en(bool en)
{
	if (rx_info.chip_id < CHIP_ID_T7)
		return;
	if (en)
		hdmirx_wr_top_common_1(TOP_OVID_OVERRIDE0, 0);
	else
		hdmirx_wr_top_common_1(TOP_OVID_OVERRIDE0, 0x80000000);
}

void hdmirx_hw_config(u8 port)
{
	rx_pr("%s port:%d\n", __func__, port);
	hdmirx_top_sw_reset();
	//rx_i2c_div_init();
	hdmirx_output_en(false);
	if (rx_info.chip_id >= CHIP_ID_T3X) {
		top_init(port);
		cor_init(port);
	} else if (rx_info.chip_id >= CHIP_ID_T7) {
		cor_init(port);
	} else {
		control_reset();
		rx_hdcp_init();
		hdmirx_audio_init();
		//packet_init();
		if (rx_info.chip_id != CHIP_ID_TXHD)
			hdmirx_20_init();
		DWC_init();
	}
	if (rx_info.chip_id >= CHIP_ID_T7)
		hdcp_init_t7(port);
	packet_init(port);
	if (rx_info.chip_id >= CHIP_ID_TL1)
		aml_phy_switch_port(port);
	hdmirx_phy_init(port);
	pre_port = 0xff;
}

/*
 * hdmirx_hw_probe - hdmirx top/controller/phy init
 */
void hdmirx_hw_probe(void)
{
	u8 port = rx_info.main_port;
	u8 i = 0;

	if (rx_info.chip_id < CHIP_ID_T7)
		hdmirx_wr_top(TOP_MEM_PD, 0, port);
	if (rx_info.chip_id == CHIP_ID_T3X) {
		rx_pwrcntl_mem_pd_cfg();
		rx_cor_reset_t3x(E_PORT0);
		rx_cor_reset_t3x(E_PORT1);
		rx_cor_reset_t3x(E_PORT2);
		rx_cor_reset_t3x(E_PORT3);
	}
	hdmirx_top_irq_en(0, 0);
	if (rx_info.chip_id == CHIP_ID_T3X) {
		hdmirx_top_irq_en(0, 1);
		hdmirx_top_irq_en(0, 2);
		hdmirx_top_irq_en(0, 3);
	}
	hdmirx_wr_top(TOP_SW_RESET, 0, 0);
	if (rx_info.chip_id == CHIP_ID_T3X) {
		hdmirx_wr_top(TOP_SW_RESET, 0, 1);
		hdmirx_wr_top(TOP_SW_RESET, 0, 2);
		hdmirx_wr_top(TOP_SW_RESET, 0, 3);
	}
	clk_init();
	top_common_init();
	hdmirx_top_sw_reset();
	if (rx_info.chip_id >= CHIP_ID_T3X) {
		for (i = 0; i < 4; i++) {
			cor_init(i); //todo
			top_init(i);
			packet_init(i);
		}
	} else if (rx_info.chip_id >= CHIP_ID_T7) {
		top_init(0);
		cor_init(0);
	} else {
		top_init(0);
		control_reset();
		DWC_init();
		hdcp22_clk_en(1);
		hdmirx_audio_init();
		//packet_init();
		if (rx_info.chip_id != CHIP_ID_TXHD)
			hdmirx_20_init();
		}
	rx_emp_to_ddr_init(port);
	if (rx_info.chip_id >= CHIP_ID_T3X)
		rx_emp1_to_ddr_init(port);
	hdmi_rx_top_edid_update();
	if (rx_info.chip_id >= CHIP_ID_TL1)
		aml_phy_switch_port(port);

	/* for t5,offset_cal also did some phy & pll init operation*/
	/* dont need to do phy init again */
	if (rx_info.phy_ver >= PHY_VER_T5)
		aml_phy_offset_cal();
	else
		hdmirx_phy_init(port);
	if (rx_info.chip_id >= CHIP_ID_T3X) {
		hdcp_init_t7(E_PORT0);
		hdcp_init_t7(E_PORT1);
		hdcp_init_t7(E_PORT2);
		hdcp_init_t7(E_PORT3);
	} else if (rx_info.chip_id >= CHIP_ID_T7) {
		hdcp_init_t7(port);
	}
	if (rx_info.chip_id >= CHIP_ID_T3X)
		hdmirx_wr_top_common(TOP_PORT_SEL, 0x100);
	else
		hdmirx_wr_top_common(TOP_PORT_SEL, 0x10);
	hdmirx_wr_top(TOP_INTR_STAT_CLR, ~0, port);//to do
}

/*
 * rx_audio_pll_sw_update
 * Sent an update pulse to audio pll module.
 * Indicate the ACR info is changed.
 */
void rx_audio_pll_sw_update(void)
{
	//if (rx_info.chip_id >= CHIP_ID_T3X)
	if (rx_info.chip_id != CHIP_ID_T3X)
		hdmirx_wr_bits_top_common(TOP_ACR_CNTL_STAT, _BIT(11), 1);
	else
		hdmirx_wr_bits_top_common_1(TOP_ACR_CNTL_STAT, _BIT(11), 1);
	//else
		//hdmirx_wr_bits_top(TOP_ACR_CNTL_STAT, _BIT(11), 1);
}

/*
 * func: rx_acr_info_update
 * refresh aud_pll by manual N/CTS changing
 */
void rx_acr_info_sw_update(void)
{
	if (rx_info.chip_id >= CHIP_ID_T7)
		return;

	hdmirx_wr_dwc(DWC_AUD_CLK_CTRL, 0x10);
	udelay(100);
	hdmirx_wr_dwc(DWC_AUD_CLK_CTRL, 0x0);
}

/*
 * is_afifo_error - audio fifo unnormal detection
 * check if afifo block or not
 * bit4: indicate FIFO is overflow
 * bit3: indicate FIFO is underflow
 * bit2: start threshold pass
 * bit1: wr point above max threshold
 * bit0: wr point below mix threshold
 *
 * return true if afifo under/over flow, false otherwise.
 */
bool is_aud_fifo_error(void)
{
	bool ret = false;

	if (rx_info.chip_id >= CHIP_ID_T7)
		return ret;

	if ((hdmirx_rd_dwc(DWC_AUD_FIFO_STS) &
		(OVERFL_STS | UNDERFL_STS)) &&
		rx[rx_info.main_port].aud_info.aud_packet_received) {
		ret = true;
		if (log_level & DBG_LOG)
			rx_pr("afifo err\n");
	}
	return ret;
}

/*
 * is_aud_pll_error - audio clock range detection
 * normal mode: aud_pll = aud_sample_rate * 128
 * HBR: aud_pll = aud_sample_rate * 128 * 4
 *
 * return true if audio clock is in range, false otherwise.
 */
bool is_aud_pll_error(void)
{
	bool ret = true;
	u32 clk;
	u32 aud_128fs;
	u32 aud_512fs;

	if (rx_info.chip_id >= CHIP_ID_T7)
		return false;

	clk = rx[rx_info.main_port].aud_info.aud_clk;
	aud_128fs = rx[rx_info.main_port].aud_info.real_sr * 128;
	aud_512fs = rx[rx_info.main_port].aud_info.real_sr * 512;
	if (rx[rx_info.main_port].aud_info.real_sr == 0)
		return false;
	if (abs(clk - aud_128fs) < AUD_PLL_THRESHOLD ||
	    abs(clk - aud_512fs) < AUD_PLL_THRESHOLD)
		ret = false;
	if ((ret) && (log_level & AUDIO_LOG))
		rx_pr("clk:%d,128fs:%d,512fs:%d,\n", clk, aud_128fs, aud_512fs);
	return ret;
}

/*
 * rx_aud_pll_ctl - audio pll config
 */
void rx_aud_pll_ctl(bool en, u8 port)
{
	int tmp = 0;
	/*u32 od, od2;*/

	if (rx_is_pip_on() && port == rx_info.sub_port) {
		rx_pr("%s sub_port mute\n", __func__);
		return;
	}
	if (rx_info.chip_id >= CHIP_ID_TL1) {
		if (rx_info.chip_id == CHIP_ID_T7) {
			if (en) {
				tmp = rd_reg_clk_ctl(RX_CLK_CTRL2);
				tmp |= (1 << 8);// [    8] clk_en for cts_hdmirx_aud_pll_clk
				wr_reg_clk_ctl(RX_CLK_CTRL2, tmp);
				/* AUD_CLK=N/CTS*TMDS_CLK */
				hdmirx_wr_amlphy(HHI_AUD_PLL_CNTL_T7, 0x40001540);
				/* use mpll */
				tmp = 0;
				tmp |= 2 << 2; /* 0:tmds_clk 1:ref_clk 2:mpll_clk */
				hdmirx_wr_amlphy(HHI_AUD_PLL_CNTL2_T7, tmp);
				/* cntl3 2:0 000=1*cts 001=2*cts 010=4*cts 011=8*cts */
				hdmirx_wr_amlphy(HHI_AUD_PLL_CNTL3_T7,
					rx[port].phy.aud_div);
				if (log_level & AUDIO_LOG)
					rx_pr("aud div=%d\n", rd_reg_hhi(HHI_AUD_PLL_CNTL3));
				hdmirx_wr_amlphy(HHI_AUD_PLL_CNTL_T7, 0x60001540);
				if (log_level & AUDIO_LOG)
					rx_pr("audio pll lock:0x%x\n",
						  hdmirx_rd_amlphy(HHI_AUD_PLL_CNTL_I_T7));
				rx_audio_pll_sw_update();
				hdmirx_audio_fifo_rst(port);
			} else {
				/* disable pll, into reset mode */
				hdmirx_audio_disabled(port);
				hdmirx_wr_amlphy(HHI_AUD_PLL_CNTL_T7, 0x0);
				tmp = rd_reg_clk_ctl(RX_CLK_CTRL2);
				tmp &= ~(1 << 8);// [    8] clk_en for cts_hdmirx_aud_pll_clk
				wr_reg_clk_ctl(RX_CLK_CTRL2, tmp);
			}
		} else if (rx_info.chip_id == CHIP_ID_T3 ||
			rx_info.chip_id == CHIP_ID_T5M) {
			if (en) {
				tmp = rd_reg_clk_ctl(RX_CLK_CTRL2);
				tmp |= (1 << 8);// [    8] clk_en for cts_hdmirx_aud_pll_clk
				wr_reg_clk_ctl(RX_CLK_CTRL2, tmp);
				/* AUD_CLK=N/CTS*TMDS_CLK */
				wr_reg_ana_ctl(ANACTL_AUD_PLL_CNTL, 0x40001540);
				/* use mpll */
				tmp = 0;
				tmp |= 2 << 2; /* 0:tmds_clk 1:ref_clk 2:mpll_clk */
				wr_reg_ana_ctl(ANACTL_AUD_PLL_CNTL2, tmp);
				/* cntl3 2:0 000=1*cts 001=2*cts 010=4*cts 011=8*cts */
				wr_reg_ana_ctl(ANACTL_AUD_PLL_CNTL3,
					rx[port].phy.aud_div);
				if (log_level & AUDIO_LOG)
					rx_pr("aud div=%d\n",
						rd_reg_ana_ctl(ANACTL_AUD_PLL_CNTL3));
				wr_reg_ana_ctl(ANACTL_AUD_PLL_CNTL, 0x60001540);
				if (log_level & AUDIO_LOG)
					/* t3 audio pll lock bit: top reg acr_cntl_stat bit'31 */
					rx_pr("audio pll lock:0x%x\n",
						  (hdmirx_rd_top_common(TOP_ACR_CNTL_STAT) >> 31));
				rx_audio_pll_sw_update();
				hdmirx_audio_fifo_rst(port);
			} else {
				/* disable pll, into reset mode */
				hdmirx_audio_disabled(port);
				wr_reg_ana_ctl(ANACTL_AUD_PLL_CNTL, 0x0);
				tmp = rd_reg_clk_ctl(RX_CLK_CTRL2);
				tmp &= ~(1 << 8);// [    8] clk_en for cts_hdmirx_aud_pll_clk
				wr_reg_clk_ctl(RX_CLK_CTRL2, tmp);
			}
		} else if (rx_info.chip_id == CHIP_ID_T5W) {
			if (en) {
				tmp = rd_reg_clk_ctl(RX_CLK_CTRL2_T5W);
				/* [    8] clk_en for cts_hdmirx_aud_pll_clk */
				tmp |= (1 << 8);
				wr_reg_clk_ctl(RX_CLK_CTRL2_T5W, tmp);
				/* AUD_CLK=N/CTS*TMDS_CLK */
				wr_reg_hhi(HHI_AUD_PLL_CNTL, 0x40001540);
				/* use mpll */
				tmp = 0;
				/* 0:tmds_clk 1:ref_clk 2:mpll_clk */
				tmp |= 2 << 2;
				wr_reg_hhi(HHI_AUD_PLL_CNTL2, tmp);
				/* cntl3 2:0 0=1*cts 1=2*cts */
				/* 010=4*cts 011=8*cts */
				wr_reg_hhi(HHI_AUD_PLL_CNTL3, rx[port].phy.aud_div);
				if (log_level & AUDIO_LOG)
					rx_pr("aud div=%d\n",
					rd_reg_hhi(ANACTL_AUD_PLL_CNTL3));
				wr_reg_hhi(HHI_AUD_PLL_CNTL, 0x60001540);
				if (log_level & AUDIO_LOG) {
					/* pll lock bit:*/
					/*top reg acr_cntl_stat bit'31 */
					tmp = hdmirx_rd_top_common(TOP_ACR_CNTL_STAT);
					rx_pr("apll lock:0x%x\n", (tmp >> 31));
				}
				rx_audio_pll_sw_update();
				hdmirx_audio_fifo_rst(port);
			} else {
				/* disable pll, into reset mode */
				hdmirx_audio_disabled(port);
				wr_reg_hhi(HHI_AUD_PLL_CNTL, 0x0);
				tmp = rd_reg_clk_ctl(RX_CLK_CTRL2_T5W);
				/* [    8] clk_en for cts_hdmirx_aud_pll_clk */
				tmp &= ~(1 << 8);
				wr_reg_clk_ctl(RX_CLK_CTRL2_T5W, tmp);
			}
		} else if (rx_info.chip_id == CHIP_ID_TXHD2) {
			if (en) {
				tmp = rd_reg_hhi(HHI_VDAC_CNTL0);
				tmp |= (1 << 7);
				tmp |= (1 << 11);
				wr_reg_hhi(HHI_VDAC_CNTL0, tmp);
				tmp = rd_reg_hhi(TXHD2_PWR_CTL);
				tmp |= (1 << 1);
				wr_reg_hhi(TXHD2_PWR_CTL, tmp);

				tmp = rd_reg_clk_ctl(CLK_MUX_TXHD2);
				/* [    8] clk_en for cts_hdmirx_aud_pll_clk */
				tmp |= (1 << 8);
				wr_reg_clk_ctl(CLK_MUX_TXHD2, tmp);

				/* AUD_CLK=N/CTS*TMDS_CLK */
				wr_reg_hhi(HHI_AUD_PLL_CNTL, 0x60001540);
				/* use mpll */
				tmp = 0;
				/* 2:tmds_clk 1:ref_clk 0:mpll_clk */
				if (rx[port].phy.pll_bw == 0)
					tmp = 0xc80;
				else if (rx[port].phy.pll_bw == 1)
					tmp = 0x80;
				else
					tmp = 0x88;
				wr_reg_hhi(HHI_AUD_PLL_CNTL2, tmp);
				/* cntl3 2:0 0=1*cts 1=2*cts */
				/* 010=4*cts 011=8*cts */
				wr_reg_hhi(HHI_AUD_PLL_CNTL3, rx[port].phy.aud_div);
				if (log_level & AUDIO_LOG)
					rx_pr("aud div=%d\n",
					rd_reg_hhi(HHI_AUD_PLL_CNTL3));
				wr_reg_hhi(HHI_AUD_PLL_CNTL, 0x40005540);
				if (log_level & AUDIO_LOG) {
					/* pll lock bit:*/
					/*top reg acr_cntl_stat bit'31 */
					tmp = hdmirx_rd_top_common(TOP_ACR_CNTL_STAT);
					rx_pr("apll lock:0x%x\n", (tmp >> 31));
				}
				rx_audio_pll_sw_update();
				hdmirx_audio_fifo_rst(port);
			} else {
				hdmirx_audio_disabled(port);
				tmp = rd_reg_clk_ctl(CLK_MUX_TXHD2);
				/* [    8] clk_en for cts_hdmirx_aud_pll_clk */
				tmp &= ~(1 << 8);
				wr_reg_clk_ctl(CLK_MUX_TXHD2, tmp);
			}
		} else if (rx_info.chip_id >= CHIP_ID_T3X) {
			if (en) {
				if (port == rx_info.main_port && vpcore1_select) {//to do
					/* switch to core1 no sound */
					tmp = rd_reg_clk_ctl(RX_CLK_CTRL2);
					tmp |= (1 << 24);
					tmp &= ~(1 << 25);
					wr_reg_clk_ctl(RX_CLK_CTRL2, tmp);
					wr_reg_clk_ctl(T3X_CLKCTRL_AUD21_PLL_CTRL0, 0x40001540);
					/* 0:tmds_clk 1:ref_clk 2:mpll_clk */
					wr_reg_clk_ctl(T3X_CLKCTRL_AUD21_PLL_CTRL1,
					rx[port].phy.aud_div_1);
					wr_reg_clk_ctl(T3X_CLKCTRL_AUD21_PLL_CTRL3,
						rx[port].phy.aud_div);
					if (rx[port].var.frl_rate) {
						audio_setting_for_aud21(rx[port].var.frl_rate,
						port);
					} else {
						hdmirx_wr_bits_amlphy_t3x(T3X_HDMIRX21PHY_DCHA_PI,
							MSK(2, 12), 0x0, port);
						hdmirx_wr_bits_clk_ctl(T3X_CLKCTRL_AUD21_PLL_CTRL0,
							_BIT(13), 0);
						hdmirx_wr_bits_clk_ctl(T3X_CLKCTRL_AUD21_PLL_CTRL2,
							_BIT(19), 0);
					}
					//wr_reg_clk_ctl(T3X_CLKCTRL_AUD21_PLL_CTRL0, 0x6000d540);
					hdmirx_wr_bits_clk_ctl(T3X_CLKCTRL_AUD21_PLL_CTRL0,
						_BIT(14), 1);
					hdmirx_wr_bits_clk_ctl(T3X_CLKCTRL_AUD21_PLL_CTRL0,
						_BIT(29), 1);
					if (rx[port].var.frl_rate) {
						tmp = hdmirx_rd_amlphy_t3x(T3X_HDMIRX21PLL_CTRL2,
							port);
						hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL2,
							(tmp | (1 << 30)), port);
					} else {
						hdmirx_wr_amlphy_t3x(T3X_HDMIRX21PLL_CTRL2,
							0, port);
					}
					rx_audio_pll_sw_update();
					hdmirx_audio_fifo_rst(port);
					rx_pr("21 audio cfg\n");
				} else if (!vpcore1_select) {
					tmp = hdmirx_rd_top_common(HDMIRX_TOP_FSW_CNTL);
					tmp |= _BIT(8 + port * 2);
					hdmirx_wr_top_common(HDMIRX_TOP_FSW_CNTL, tmp);
					tmp = rd_reg_clk_ctl(RX_CLK_CTRL2);
					tmp |= (1 << 8);// [    8] clk_en for cts_hdmirx_aud_pll_clk
					wr_reg_clk_ctl(RX_CLK_CTRL2, tmp);
					/* AUD_CLK=N/CTS*TMDS_CLK */
					wr_reg_ana_ctl(ANACTL_AUD_PLL_CNTL, 0x40001540);
					/* use mpll */
					tmp = 0;
					tmp |= 2 << 2; /* 0:tmds_clk 1:ref_clk 2:mpll_clk */
					wr_reg_ana_ctl(ANACTL_AUD_PLL_CNTL2, tmp);
					/* cntl3 2:0 000=1*cts 001=2*cts 010=4*cts 011=8*cts */
					wr_reg_ana_ctl(ANACTL_AUD_PLL_CNTL3,
						rx[port].phy.aud_div);
					if (log_level & AUDIO_LOG)
						rx_pr("aud div=%d\n",
							rd_reg_ana_ctl(ANACTL_AUD_PLL_CNTL3));
					wr_reg_ana_ctl(ANACTL_AUD_PLL_CNTL, 0x60001540);
					tmp = hdmirx_rd_top_common(TOP_ACR_CNTL_STAT) >> 31;
					if (log_level & AUDIO_LOG)
						rx_pr("audio pll lock:0x%x\n", tmp);
					rx_audio_pll_sw_update();
					hdmirx_audio_fifo_rst(port);
				}
			} else {
				/* disable pll, into reset mode */
				hdmirx_audio_disabled(port);
				wr_reg_ana_ctl(ANACTL_AUD_PLL_CNTL, 0x0);
				tmp = rd_reg_clk_ctl(RX_CLK_CTRL2);
				tmp &= ~(1 << 8);// [    8] clk_en for cts_hdmirx_aud_pll_clk
				tmp &= ~(1 << 24);
				wr_reg_clk_ctl(RX_CLK_CTRL2, tmp);
			}
		} else {
			if (en) {
				/* AUD_CLK=N/CTS*TMDS_CLK */
				wr_reg_hhi(HHI_AUD_PLL_CNTL, 0x40001540);
				/* use mpll */
				tmp = 0;
				tmp |= 2 << 2; /* 0:tmds_clk 1:ref_clk 2:mpll_clk */
				wr_reg_hhi(HHI_AUD_PLL_CNTL2, tmp);
				/* cntl3 2:0 000=1*cts 001=2*cts 010=4*cts 011=8*cts */
				wr_reg_hhi(HHI_AUD_PLL_CNTL3, rx[port].phy.aud_div);

				if (log_level & AUDIO_LOG)
					rx_pr("aud div=%d\n", rd_reg_hhi(HHI_AUD_PLL_CNTL3));
				wr_reg_hhi(HHI_AUD_PLL_CNTL, 0x60001540);
				if (log_level & AUDIO_LOG)
					rx_pr("audio pll lock:0x%x\n",
					      rd_reg_hhi(HHI_AUD_PLL_CNTL_I));
				/*rx_audio_pll_sw_update();*/
			} else {
				/* disable pll, into reset mode */
				hdmirx_audio_disabled(port);
				wr_reg_hhi(HHI_AUD_PLL_CNTL, 0x0);
			}
		}
	} else {
		if (en) {
			rx_audio_bandgap_en();
			tmp = hdmirx_rd_phy(PHY_MAINFSM_STATUS1);
			wr_reg_hhi(HHI_AUD_PLL_CNTL, 0x20000000);
			/* audio pll div depends on input freq */
			wr_reg_hhi(HHI_AUD_PLL_CNTL6, (tmp >> 9 & 3) << 28);
			/* audio pll div fixed to N/CTS as below*/
			/* wr_reg_hhi(HHI_AUD_PLL_CNTL6, 0x40000000); */
			wr_reg_hhi(HHI_AUD_PLL_CNTL5, 0x0000002e);
			wr_reg_hhi(HHI_AUD_PLL_CNTL4, 0x30000000);
			wr_reg_hhi(HHI_AUD_PLL_CNTL3, 0x00000000);
			wr_reg_hhi(HHI_AUD_PLL_CNTL, 0x40000000);
			wr_reg_hhi(HHI_ADC_PLL_CNTL4, 0x805);
			rx_audio_pll_sw_update();
			hdmirx_audio_fifo_rst(port);
			/*External_Mute(0);*/
		} else {
			wr_reg_hhi(HHI_AUD_PLL_CNTL, 0x20000000);
		}
	}
}

unsigned char rx_get_hdcp14_sts(void)
{
	return (unsigned char)((hdmirx_rd_dwc(DWC_HDCP_STS) >> 8) & 3);
}

/*
 * rx_get_video_info - get current avi info
 */
bool rx_get_dvi_mode(u8 port)
{
	u32 ret;

	if (rx_info.chip_id >= CHIP_ID_T7)
		ret = hdmirx_rd_cor(RX_AUDP_STAT_DP2_IVCRX, port) & 1;
	else
		ret = !hdmirx_rd_bits_dwc(DWC_PDEC_STS, DVIDET);
	if (ret)
		return false;
	else
		return true;
}

u8 rx_get_hdcp_type(u8 port)
{
	u32 tmp;
	u8 data_dec, data_auth;

	if (rx_info.chip_id >= CHIP_ID_T7) {
		//
		//get from irq_handler
		//
		data_auth = hdmirx_rd_cor(CP2PAX_AUTH_STAT_HDCP2X_IVCRX, port);
		data_dec = hdmirx_rd_cor(RX_HDCP_STATUS_PWD_IVCRX, port);
		rx[port].cur.hdcp14_state =
			(hdmirx_rd_cor(RX_HDCP_STAT_HDCP1X_IVCRX, port) >> 4) & 3;
		rx[port].cur.hdcp22_state =
			((data_dec & 1) << 1) | (data_auth & 1);
		//if (rx[rx_info.main_port].cur.hdcp22_state & 3 &&
		//rx[rx_info.main_port].cur.hdcp14_state != 3)
			//rx[rx_info.main_port].hdcp.hdcp_version = HDCP_VER_22;
		//else if (rx[rx_info.main_port].cur.hdcp14_state == 3 &&
		//rx[rx_info.main_port].cur.hdcp22_state != 3)
			//rx[rx_info.main_port].hdcp.hdcp_version = HDCP_VER_14;
		//else
			//rx[rx_info.main_port].hdcp.hdcp_version = HDCP_VER_NONE;
	} else {
		if (hdcp22_on) {
			tmp = hdmirx_rd_dwc(DWC_HDCP22_STATUS);
			rx[port].cur.hdcp_type = (tmp >> 4) & 1;
			rx[port].cur.hdcp22_state = tmp & 1;
		}
		if (!rx[port].cur.hdcp_type)
			rx[port].cur.hdcp14_state =
				(hdmirx_rd_dwc(DWC_HDCP_STS) >> 8) & 3;
	}
	return 1;
}

/*
 * rx_get_hdcp_auth_sts
 */
int rx_get_hdcp_auth_sts(u8 port)
{
	bool ret = 0;
	int hdcp22_status;

	if (rx[port].state < FSM_SIG_READY)
		return ret;

	if (rx_info.chip_id <= CHIP_ID_T5D)
		hdcp22_status = (rx[port].cur.hdcp22_state == 3) ? 1 : 0;
	else
		hdcp22_status = rx[port].cur.hdcp22_state & 1;

	if ((rx[port].hdcp.hdcp_version ==
		HDCP_VER_14 && rx[port].cur.hdcp14_state == 3) ||
		(rx[port].hdcp.hdcp_version == HDCP_VER_22 && hdcp22_status))
		ret = 1;
	return ret;
}

void rx_get_avi_params(u8 port)
{
	u8 data8, data8_lo, data8_up;

	if (rx_info.chip_id >= CHIP_ID_T7) {
		/*byte 1*/
		data8 = hdmirx_rd_cor(AVIRX_DBYTE1_DP2_IVCRX, port);
		/*byte1:bit[7:5]*/
		rx[port].cur.colorspace = (data8 >> 5) & 0x07;
		/*byte 1 bit'4*/
		rx[port].cur.active_valid = (data8 >> 4) & 0x01;
		/*byte1:bit[3:2]*/
		rx[port].cur.bar_valid = (data8 >> 2) & 0x03;

		/*byte1:bit[1:0]*/
		rx[port].cur.scan_info = data8 & 0x3;

		/*byte 2*/
		data8 = hdmirx_rd_cor(AVIRX_DBYTE2_DP2_IVCRX, port);
		/*byte1:bit[7:6]*/
		rx[port].cur.colorimetry = (data8 >> 6) & 0x3;
		/*byte1:bit[5:4]*/
		rx[port].cur.picture_ratio = (data8 >> 4) & 0x3;
		/*byte1:bit[3:0]*/
		rx[port].cur.active_ratio = data8 & 0xf;

		/*byte 3*/
		data8 = hdmirx_rd_cor(AVIRX_DBYTE3_DP2_IVCRX, port);
		/* byte3 bit'7 */
		rx[port].cur.it_content = (data8 >> 7) & 0x1;
		/* byte3 bit[6:4] */
		rx[port].cur.ext_colorimetry = (data8 >> 4) & 0x7;
		/* byte3 bit[3:2]*/
		rx[port].cur.rgb_quant_range = (data8 >> 2) & 0x3;
		/* byte3 bit[1:0]*/
		rx[port].cur.n_uniform_scale = data8 & 0x3;

		/*byte 4*/
		rx[port].cur.hw_vic = hdmirx_rd_cor(AVIRX_DBYTE4_DP2_IVCRX, port);

		/*byte 5*/
		data8 = hdmirx_rd_cor(AVIRX_DBYTE5_DP2_IVCRX, port);
		/*byte5:bit[7:6]*/
		rx[port].cur.yuv_quant_range = (data8 >> 6) & 0x3;
		/*byte5:bit[5:4]*/
		rx[port].cur.cn_type = (data8 >> 4) & 0x3;
		/*byte5:bit[3:0]*/
		rx[port].cur.repeat = data8 & 0xf;

		/* byte [9:6]*/
		if (rx[port].cur.bar_valid == 3) {
			data8_lo = hdmirx_rd_cor(AVIRX_DBYTE6_DP2_IVCRX, port);
			data8_up = hdmirx_rd_cor(AVIRX_DBYTE7_DP2_IVCRX, port);
			rx[port].cur.bar_end_top = (data8_lo | (data8_up << 8));
			data8_lo = hdmirx_rd_cor(AVIRX_DBYTE8_DP2_IVCRX, port);
			data8_up = hdmirx_rd_cor(AVIRX_DBYTE9_DP2_IVCRX, port);
			rx[port].cur.bar_start_bottom = (data8_lo | (data8_up << 8));
			data8_lo = hdmirx_rd_cor(AVIRX_DBYTE10_DP2_IVCRX, port);
			data8_up = hdmirx_rd_cor(AVIRX_DBYTE11_DP2_IVCRX, port);
			rx[port].cur.bar_end_left = (data8_lo | (data8_up << 8));
			data8_lo = hdmirx_rd_cor(AVIRX_DBYTE12_DP2_IVCRX, port);
			data8_up = hdmirx_rd_cor(AVIRX_DBYTE13_DP2_IVCRX, port);
			rx[port].cur.bar_start_right = (data8_lo | (data8_up << 8));
		}
	} else {
		rx[port].cur.hw_vic =
			hdmirx_rd_bits_dwc(DWC_PDEC_AVI_PB, VID_IDENT_CODE);
		rx[port].cur.cn_type =
			hdmirx_rd_bits_dwc(DWC_PDEC_AVI_HB, CONTENT_TYPE);
		rx[port].cur.repeat =
			hdmirx_rd_bits_dwc(DWC_PDEC_AVI_HB, PIX_REP_FACTOR);
		rx[port].cur.colorspace =
			hdmirx_rd_bits_dwc(DWC_PDEC_AVI_PB, VIDEO_FORMAT);
		rx[port].cur.it_content =
			hdmirx_rd_bits_dwc(DWC_PDEC_AVI_PB, IT_CONTENT);
		rx[port].cur.rgb_quant_range =
			hdmirx_rd_bits_dwc(DWC_PDEC_AVI_PB, RGB_QUANT_RANGE);
		rx[port].cur.yuv_quant_range =
			hdmirx_rd_bits_dwc(DWC_PDEC_AVI_HB, YUV_QUANT_RANGE);
		rx[port].cur.scan_info =
			hdmirx_rd_bits_dwc(DWC_PDEC_AVI_PB, SCAN_INFO);
		rx[port].cur.n_uniform_scale =
			hdmirx_rd_bits_dwc(DWC_PDEC_AVI_PB, NON_UNIF_SCALE);
		rx[port].cur.ext_colorimetry =
			hdmirx_rd_bits_dwc(DWC_PDEC_AVI_PB, EXT_COLORIMETRY);
		rx[port].cur.active_ratio =
			hdmirx_rd_bits_dwc(DWC_PDEC_AVI_PB, ACT_ASPECT_RATIO);
		rx[port].cur.active_valid =
			hdmirx_rd_bits_dwc(DWC_PDEC_AVI_PB, ACT_INFO_PRESENT);
		rx[port].cur.bar_valid =
			hdmirx_rd_bits_dwc(DWC_PDEC_AVI_PB, BAR_INFO_VALID);
		if (rx[port].cur.bar_valid == 3) {
			rx[port].cur.bar_end_top =
				hdmirx_rd_bits_dwc(DWC_PDEC_AVI_TBB, LIN_END_TOP_BAR);
			rx[port].cur.bar_start_bottom =
				hdmirx_rd_bits_dwc(DWC_PDEC_AVI_TBB, LIN_ST_BOT_BAR);
			rx[port].cur.bar_end_left =
				hdmirx_rd_bits_dwc(DWC_PDEC_AVI_LRB, PIX_END_LEF_BAR);
			rx[port].cur.bar_start_right =
				hdmirx_rd_bits_dwc(DWC_PDEC_AVI_LRB, PIX_ST_RIG_BAR);
		}
		rx[port].cur.colorimetry =
			hdmirx_rd_bits_dwc(DWC_PDEC_AVI_PB, COLORIMETRY);
		rx[port].cur.picture_ratio =
			hdmirx_rd_bits_dwc(DWC_PDEC_AVI_PB, PIC_ASPECT_RATIO);
	}
}

void rx_get_colordepth(u8 port)
{
	u8 tmp;

	if (rx_info.chip_id >= CHIP_ID_T7)
		tmp = hdmirx_rd_cor(COR_VININ_STS, port);
	else
		tmp = hdmirx_rd_bits_dwc(DWC_HDMI_STS, DCM_CURRENT_MODE);
	switch (tmp) {
	case DCM_CURRENT_MODE_48b:
		rx[port].cur.colordepth = E_COLORDEPTH_16;
		break;
	case DCM_CURRENT_MODE_36b:
		rx[port].cur.colordepth = E_COLORDEPTH_12;
		break;
	case DCM_CURRENT_MODE_30b:
		rx[port].cur.colordepth = E_COLORDEPTH_10;
		break;
	default:
		rx[port].cur.colordepth = E_COLORDEPTH_8;
		break;
	}
}

void rx_get_framerate(u8 port)
{
	u32 tmp;

	if (rx_info.chip_id >= CHIP_ID_T7) {
		tmp = (hdmirx_rd_cor(COR_FRAME_RATE_HI, port) << 16) |
			(hdmirx_rd_cor(COR_FRAME_RATE_MI, port) << 8) |
			(hdmirx_rd_cor(COR_FRAME_RATE_LO, port));
		if (tmp == 0)
			rx[port].cur.frame_rate = 0;
		else
			rx[port].cur.frame_rate =
				rx[port].clk.p_clk / (tmp / 100);
	} else {
		tmp = hdmirx_rd_bits_dwc(DWC_MD_VTC, VTOT_CLK);
		if (tmp == 0)
			rx[port].cur.frame_rate = 0;
		else
			rx[port].cur.frame_rate = (modet_clk * 100000) / tmp;
	}
}

void rx_get_interlaced(u8 port)
{
	u8 tmp;

	if (rx_info.chip_id >= CHIP_ID_T7)
		tmp = hdmirx_rd_cor(COR_FDET_STS, port) & _BIT(2);
	else
		tmp = hdmirx_rd_bits_dwc(DWC_MD_STS, ILACE);

	if (tmp)
		rx[port].cur.interlaced = true;
	else
		rx[port].cur.interlaced = false;
}

void rx_get_de_sts(u8 port)
{
	u32 tmp;

	if (!rx[port].cur.colordepth)
		rx[port].cur.colordepth = 8;

	if (rx_info.chip_id >= CHIP_ID_T7) {
		tmp = hdmirx_rd_cor(COR_PIXEL_CNT_LO, port) |
			(hdmirx_rd_cor(COR_PIXEL_CNT_HI, port) << 8);
		rx[port].cur.hactive = tmp;
		rx[port].cur.vactive = hdmirx_rd_cor(COR_LINE_CNT_LO, port) |
			(hdmirx_rd_cor(COR_LINE_CNT_HI, port) << 8);
		rx[port].cur.htotal = (hdmirx_rd_cor(COR_HSYNC_LOW_COUNT_LO, port) |
			(hdmirx_rd_cor(COR_HSYNC_LOW_COUNT_HI, port) << 8)) +
			(hdmirx_rd_cor(COR_HSYNC_HIGH_COUNT_LO, port) |
			(hdmirx_rd_cor(COR_HSYNC_HIGH_COUNT_HI, port) << 8));
		rx[port].cur.vtotal = (hdmirx_rd_cor(COR_VSYNC_LOW_COUNT_LO, port) |
			(hdmirx_rd_cor(COR_VSYNC_LOW_COUNT_HI, port) << 8)) +
			(hdmirx_rd_cor(COR_VSYNC_HIGH_COUNT_LO, port) |
			(hdmirx_rd_cor(COR_VSYNC_HIGH_COUNT_HI, port) << 8));
		if (rx[port].cur.repeat) {
			rx[port].cur.hactive =
				rx[port].cur.hactive /
					(rx[port].cur.repeat + 1);
			rx[port].cur.htotal = rx[port].cur.htotal /
				(rx[port].cur.repeat + 1);
		}
	} else {
		rx[port].cur.vactive = hdmirx_rd_bits_dwc(DWC_MD_VAL, VACT_LIN);
		rx[port].cur.vtotal = hdmirx_rd_bits_dwc(DWC_MD_VTL, VTOT_LIN);
		rx[port].cur.hactive = hdmirx_rd_bits_dwc(DWC_MD_HACT_PX, HACT_PIX);
		rx[port].cur.htotal = hdmirx_rd_bits_dwc(DWC_MD_HT1, HTOT_PIX);
		rx[port].cur.hactive = rx[port].cur.hactive * 8 / rx[port].cur.colordepth;
		rx[port].cur.htotal = rx[port].cur.htotal * 8 / rx[port].cur.colordepth;
		if (rx[port].cur.repeat) {
			rx[port].cur.hactive = rx[port].cur.hactive /
				(rx[port].cur.repeat + 1);
			rx[port].cur.htotal = rx[port].cur.htotal /
				(rx[port].cur.repeat + 1);
		}
	}
}

void rx_get_ecc_info(u8 port)
{
	if (rx_info.chip_id < CHIP_ID_T7)
		return;

	rx[port].ecc_err = rx_get_ecc_err(port);
	rx[port].ecc_pkt_cnt = rx_get_ecc_pkt_cnt(port);
}

static void rx_fmt_override(bool override_en, u8 port)
{
	if (rx_info.chip_id == CHIP_ID_T3X && port == rx_info.main_port) {
		if (override_en) {
			hdmirx_wr_bits_top_common_1(TOP_VID_CNTL, VID_FMT_OVERRIDE, 1);
			hdmirx_wr_bits_top_common_1(TOP_VID_CNTL, VID_FMT_VAL,
				rx[port].cur.colorspace);
		} else {
			hdmirx_wr_bits_top_common_1(TOP_VID_CNTL, VID_FMT_OVERRIDE, 0);
		}
	} else {
		if (override_en) {
			hdmirx_wr_bits_top_common(TOP_VID_CNTL, VID_FMT_OVERRIDE, 1);
			hdmirx_wr_bits_top_common(TOP_VID_CNTL, VID_FMT_VAL,
				rx[port].cur.colorspace);
		} else {
			hdmirx_wr_bits_top_common(TOP_VID_CNTL, VID_FMT_OVERRIDE, 0);
		}
	}
}

/*
 * rx_get_video_info - get current avi info
 */
void rx_get_video_info(u8 port)
{
	/* DVI mode */
	rx[port].cur.hw_dvi = rx_get_dvi_mode(port);
	// new ip rm gb check when dvi input
	// avmute depend on vsync with gb check
	// rm gb check to make vsync valid when dvi input
	if (rx_info.chip_id >= CHIP_ID_T7) {
		if (rx[port].cur.hw_dvi)
			hdmirx_wr_cor(RX_PREAMBLE_CRIT_PWD_IVCRX, 0x0, port);
		else
			hdmirx_wr_cor(RX_PREAMBLE_CRIT_PWD_IVCRX, 0x6, port);
	}
	/* HDCP sts*/
	rx_get_hdcp_type(port);
	/* AVI parameters */
	rx_get_avi_params(port);
	/* frame rate */
	rx_get_framerate(port);
	/* deep color mode */
	rx_get_colordepth(port);
	/* pixel clock */
	//rx[rx_info.main_port].cur.pixel_clk =
	//rx[rx_info.main_port].clk.pixel_clk / rx[rx_info.main_port].cur.colordepth * 8;
	/* image parameters */
	rx_get_de_sts(port);
	/* interlace */
	rx_get_interlaced(port);
	rx_get_dev_type(port);
}

void hdmirx_set_vp_mapping(enum colorspace_e cs, u8 port)
{
	u32 data32 = 0;

	if (rx_info.chip_id < CHIP_ID_T7)
		return;

	if (rx_info.chip_id == CHIP_ID_T3X) {
		data32 = 0;
		if (cs == E_COLOR_YUV422) {
			data32 |= 3 << 9;
			data32 |= 3 << 6;
			data32 |= 3 << 3;
		} else {
			data32 |= 2 << 9;
			data32 |= 1 << 6;
			data32 |= 0 << 3;
		}
		hdmirx_wr_cor(VP_INPUT_MAPPING_VID_IVCRX, data32 & 0xff, port);
		hdmirx_wr_cor(VP_INPUT_MAPPING_VID_IVCRX + 1, (data32 >> 8) & 0xff, port);
		if (port == rx_info.main_port) {
			data32 = hdmirx_rd_top_common_1(TOP_VID_CNTL);
			data32 &= (~(0x7 << 24));
			data32 |= 2 << 24;
			hdmirx_wr_top_common_1(TOP_VID_CNTL, data32);//to do
		}
		if (port == rx_info.sub_port) {
			data32 = hdmirx_rd_top_common(TOP_VID_CNTL);
			data32 &= (~(0x7 << 24));
			data32 |= 2 << 24;
			hdmirx_wr_top_common(TOP_VID_CNTL, data32);
		}
		return;
	}

	switch (cs) {
	case E_COLOR_YUV422:
		data32 |= 3 << 9;
		data32 |= 3 << 6;
		data32 |= 3 << 3;
		hdmirx_wr_cor(VP_INPUT_MAPPING_VID_IVCRX, data32 & 0xff, port);
		hdmirx_wr_cor(VP_INPUT_MAPPING_VID_IVCRX + 1, (data32 >> 8) & 0xff, port);
		data32 = hdmirx_rd_top_common(TOP_VID_CNTL);//to do
		data32 &= (~(0x7 << 24));
		data32 |= 1 << 24;
		if (rx_info.chip_id != CHIP_ID_T3X)
			hdmirx_wr_top_common(TOP_VID_CNTL, data32);//to do
		break;
	case E_COLOR_YUV420:
	case E_COLOR_RGB:
		data32 |= 2 << 9;
		data32 |= 1 << 6;
		data32 |= 0 << 3;
		hdmirx_wr_cor(VP_INPUT_MAPPING_VID_IVCRX, data32 & 0xff, port);
		hdmirx_wr_cor(VP_INPUT_MAPPING_VID_IVCRX + 1, (data32 >> 8) & 0xff, port);
		data32 = hdmirx_rd_top_common(TOP_VID_CNTL);//to do
		data32 &= (~(0x7 << 24));
		data32 |= 0 << 24;
		if (rx_info.chip_id != CHIP_ID_T3X)
			hdmirx_wr_top_common(TOP_VID_CNTL, data32);//to do
		break;
	case E_COLOR_YUV444:
	default:
		data32 |= 2 << 9;
		data32 |= 1 << 6;
		data32 |= 0 << 3;
		hdmirx_wr_cor(VP_INPUT_MAPPING_VID_IVCRX, data32 & 0xff, port);
		hdmirx_wr_cor(VP_INPUT_MAPPING_VID_IVCRX + 1, (data32 >> 8) & 0xff, port);
		data32 = hdmirx_rd_top_common(TOP_VID_CNTL);//to do
		data32 &= (~(0x7 << 24));
		data32 |= 2 << 24;
		if (rx_info.chip_id != CHIP_ID_T3X)
			hdmirx_wr_top_common(TOP_VID_CNTL, data32);//to do
	break;
	}
}

/*
 * hdmirx_set_video_mute - video mute
 * @mute: mute enable or disable
 */
void hdmirx_set_video_mute(bool mute, u8 port)
{
	static bool pre_mute_flag;

	if (port != rx_info.main_port && port != rx_info.sub_port)
		return;
	/* bluescreen cfg */
	if (rx_info.chip_id == CHIP_ID_T3X) {
		// t3x to do now only mute main port
		if (rx[port].pre.colorspace == E_COLOR_RGB) {
			if (port == rx_info.main_port) {
				hdmirx_wr_top_common_1(TOP_OVID_OVERRIDE0, 0x0);
				hdmirx_wr_bits_top_common_1(TOP_OVID_OVERRIDE0, _BIT(30), mute);
			}
			if (port == rx_info.sub_port) {
				hdmirx_wr_top_common(TOP_OVID_OVERRIDE0, 0x0);
				hdmirx_wr_bits_top_common(TOP_OVID_OVERRIDE0, _BIT(30), mute);
			}
		} else {
			if (port == rx_info.main_port) {
				hdmirx_wr_top_common_1(TOP_OVID_OVERRIDE0, 0x80200);
				/* FRL mode */
				hdmirx_wr_top_common_1(TOP_OVID_OVERRIDE2, 0x80200);
				hdmirx_wr_bits_top_common_1(TOP_OVID_OVERRIDE0, _BIT(30), mute);
			}
			if (port == rx_info.sub_port) {
				hdmirx_wr_top_common(TOP_OVID_OVERRIDE0, 0x80200);
				hdmirx_wr_bits_top_common(TOP_OVID_OVERRIDE0, _BIT(30), mute);
			}
		}
	} else if (rx_info.chip_id >= CHIP_ID_T5M) {
		if (rx[port].pre.colorspace == E_COLOR_RGB)
			hdmirx_wr_top_common(TOP_OVID_OVERRIDE0, 0x0);
		else
			hdmirx_wr_top_common(TOP_OVID_OVERRIDE0, 0x80200);
		hdmirx_wr_bits_top_common(TOP_OVID_OVERRIDE0, _BIT(30), mute);
	} else if (rx_info.chip_id >= CHIP_ID_T7 && rx_info.chip_id < CHIP_ID_T5M) {
		if (mute && (rx_pkt_chk_attach_drm(port) ||
			rx[port].vs_info_details.dolby_vision_flag != DV_NULL))
			return;
		if (mute != pre_mute_flag) {
			vdin_set_black_pattern(mute);
			pre_mute_flag = mute;
		}
	} else {
		if (rx[port].pre.colorspace == E_COLOR_RGB) {
			hdmirx_wr_bits_dwc(DWC_HDMI_VM_CFG_CH2, MSK(16, 0), 0x00);
			hdmirx_wr_bits_dwc(DWC_HDMI_VM_CFG_CH_0_1, MSK(16, 0), 0x00);
		} else if (rx[port].pre.colorspace == E_COLOR_YUV420) {
			hdmirx_wr_bits_dwc(DWC_HDMI_VM_CFG_CH2, MSK(16, 0), 0x1000);
			hdmirx_wr_bits_dwc(DWC_HDMI_VM_CFG_CH_0_1, MSK(16, 0), 0x8000);
		} else {
			hdmirx_wr_bits_dwc(DWC_HDMI_VM_CFG_CH2, MSK(16, 0), 0x8000);
			hdmirx_wr_bits_dwc(DWC_HDMI_VM_CFG_CH_0_1, MSK(16, 0), 0x8000);
		}
		hdmirx_wr_bits_dwc(DWC_HDMI_VM_CFG_CH2, _BIT(16), mute);
	}
}

void set_dv_ll_mode(bool en, u8 port)
{
	if (rx_info.chip_id >= CHIP_ID_T3X ||
		rx_info.chip_id <= CHIP_ID_TL1)
		return;

	if (en) {
		hdmirx_wr_bits_top(TOP_VID_CNTL, _BIT(17), 1, port);
		hdmirx_wr_bits_top(TOP_VID_CNTL, _BIT(19), 1, port);
	} else {
		hdmirx_wr_bits_top(TOP_VID_CNTL, _BIT(17), 0, port);
		hdmirx_wr_bits_top(TOP_VID_CNTL, _BIT(19), 0, port);
	}
}

/*
 * hdmirx_config_video - video mute config
 */
void hdmirx_config_video(u8 port)
{
	u32 temp = 0;
	u32 top_vid_fmt;
	u8 data8;
	u8 pixel_rpt_cnt;
	int reg_clk_vp_core_div, reg_clk_vp_out_div;

	if (dbg_cs & 0x10)
		temp = dbg_cs & 0x0f;
	else
		temp = rx[port].pre.colorspace;

	hdmirx_set_video_mute(0, port);
	set_dv_ll_mode(false, port);
	hdmirx_output_en(true);
	rx_irq_en(IRQ_EN_ALL, port);//pcs rst needed

	if (rx_info.chip_id < CHIP_ID_T7)
		return;

	hdmirx_set_vp_mapping(temp, port);

	if (rx_info.chip_id == CHIP_ID_T7) {
		/* repetition config */
		switch (rx[port].cur.repeat) {
		case 1:
			reg_clk_vp_core_div = 3;
			reg_clk_vp_out_div = 1;
			pixel_rpt_cnt = 1;
		break;
		case 3:
			reg_clk_vp_core_div = 7;
			reg_clk_vp_out_div = 3;
			pixel_rpt_cnt = 2;
		break;
		case 7: //todo
		default:
			reg_clk_vp_core_div = 1;
			reg_clk_vp_out_div = 0;
			pixel_rpt_cnt = 0;
		break;
		}
		data8 = hdmirx_rd_cor(RX_PWD0_CLK_DIV_0, port);
		data8 &= (~0x3f);
		//[5:3] divides the vpc out clock
		data8 |= (reg_clk_vp_out_div << 3);//[5:3] divides the vpc out clock
		//[2:0] divides the vpc core clock:
		//0: divide by 1; 1: divide by 2; 3: divide by 4; 7: divide by 8
		data8 |= (reg_clk_vp_core_div << 0);
		hdmirx_wr_cor(RX_PWD0_CLK_DIV_0, data8, port) ;//register address: 0x10c1

		data8 = hdmirx_rd_cor(RX_VP_INPUT_FORMAT_HI, port);
		data8 &= (~0x7);
		data8 |= ((pixel_rpt_cnt & 0x3) << 0);
		hdmirx_wr_cor(RX_VP_INPUT_FORMAT_HI, data8, port);
	}
	if (rx_info.chip_id >= CHIP_ID_T3) {
		if (rx[port].pre.sw_vic >= HDMI_VESA_OFFSET ||
			rx[port].pre.sw_vic == HDMI_640x480p60 ||
			rx[port].pre.repeat == 0 ||
			rx[port].pre.sw_dvi)
			/* for T7, bit7 must be written as 1 in order to de-repeat */
			hdmirx_wr_bits_top(TOP_VID_CNTL, _BIT(7), 1, port);
		else//use auto de-repeat
			hdmirx_wr_bits_top(TOP_VID_CNTL, _BIT(7), 0, port);
	}
	if (rx_info.chip_id >= CHIP_ID_T7) {
		if (rx_info.chip_id == CHIP_ID_T3X && port == rx_info.main_port)
			top_vid_fmt = hdmirx_rd_bits_top_common_1(TOP_VID_STAT, TOP_VID_FMT);
		else
			top_vid_fmt = hdmirx_rd_bits_top_common(TOP_VID_STAT, TOP_VID_FMT);
		if (top_vid_fmt != rx[port].cur.colorspace)
			rx_fmt_override(true, port);
		else
			rx_fmt_override(false, port);
	}
	if (rx_info.chip_id >= CHIP_ID_T3X && port == rx_info.main_port) {
		rx[port].emp_vid_idx = 1;
		rx[port].emp_info = &rx_info.emp_buff_b;
	} else {
		rx[port].emp_vid_idx = 0;
		rx[port].emp_info = &rx_info.emp_buff_a;
	}
	rx_sw_reset_t7(2, port);
	//frl_debug
	if (rx_info.chip_id >= CHIP_ID_T3X && rx[port].var.frl_rate) {
		/* 2ppc */
		hdmirx_wr_bits_cor(RX_PWD0_CLK_DIV_0, _BIT(0), 0, port);
	} else {
		/* 1ppc */
		hdmirx_wr_bits_cor(RX_PWD0_CLK_DIV_0, _BIT(0), 1, port);
	}

	//dig clk low, use analog clk for 4k120 yuv420 12.
	if (rx_is_switch_to_analog_clk(port))
		rx_switch_to_analog_clk(port);

	if (rx[port].dsc_flag)
		rx_switch_to_self_hsync(port, true);
	if (port == rx_info.main_port) {
		if (rx[port].cur.vactive >= 2100 || rx[port].cur.hactive >= 3800)
			hdmirx_wr_bits_top_common_1(TOP_VID_CNTL2, _BIT(31), 1);
		else
			hdmirx_wr_bits_top_common_1(TOP_VID_CNTL2, _BIT(31), 0);
	}
}

/*
 * hdmirx_config_audio - audio channel map
 */
void hdmirx_config_audio(u8 port)
{
	if (port == rx_info.sub_port)
		return;
	if (rx_info.chip_id >= CHIP_ID_T7) {
		/* set MCLK for I2S/SPDIF */
		hdmirx_wr_cor(AAC_MCLK_SEL_AUD_IVCRX, 0x80, port);
		hdmirx_hbr2spdif(1, port);
	}
}

/*
 * rx_get_clock: git clock from hdmi top
 * tl1: have hdmi, cable clock
 * other: have hdmi clock
 */
int rx_get_clock(enum measure_clk_top_e clk_src, u8 port)  //todo
{
	int clock = -1;
	u32 tmp_data = 0;
	u32 meas_cycles = 0;
	u64 tmp_data2 = 0;
	u64 aud_clk = 0;

	if (clk_src == TOP_HDMI_TMDSCLK) {
		tmp_data = hdmirx_rd_top(TOP_METER_HDMI_STAT, port);
		if (tmp_data & 0x80000000) {
			meas_cycles = tmp_data & 0xffffff;
			clock = (2930 * meas_cycles);
		}
	} else if (clk_src == TOP_HDMI_CABLECLK) {
		if (rx_info.chip_id >= CHIP_ID_TL1)
			tmp_data = hdmirx_rd_top(TOP_METER_CABLE_STAT, port);
		if (tmp_data & 0x80000000) {
			meas_cycles = tmp_data & 0xffffff;
			clock = (2930 * meas_cycles);
		}
	} else if (clk_src == TOP_HDMI_AUDIOCLK) {
		if (rx_info.chip_id >= CHIP_ID_TL1) {
			/*get audio clk*/
			tmp_data = hdmirx_rd_top_common(TOP_AUDMEAS_REF_CYCLES_STAT0);//to do
			tmp_data2 = hdmirx_rd_top_common(TOP_AUDMEAS_REF_CYCLES_STAT1);//to do
			aud_clk = ((tmp_data2 & 0xffff) << 32) | tmp_data;
			if (tmp_data2 & (0x1 << 17))
				aud_clk = div_u64((24000 * 65536), div_u64((aud_clk + 1), 1000));
			else
				rx_pr("audio clk measure fail\n");
		}
		clock = aud_clk;
	} else {
		tmp_data = 0;
	}

	/*reset hdmi,cable clk meter*/
	hdmirx_wr_top(TOP_SW_RESET, 0x60, port);
	hdmirx_wr_top(TOP_SW_RESET, 0x0, port);
	return clock;
}

void rx_clkmsr_monitor(void)
{
	rx[E_PORT0].clk.cable_clk_pre = rx[E_PORT0].clk.cable_clk;
	rx[E_PORT1].clk.cable_clk_pre = rx[E_PORT1].clk.cable_clk;
	rx[E_PORT2].clk.cable_clk_pre = rx[E_PORT2].clk.cable_clk;
	rx[E_PORT3].clk.cable_clk_pre = rx[E_PORT3].clk.cable_clk;
	if ((rx_info.main_port_open || rx_info.chip_id == CHIP_ID_T3X) && rx_get_hdmi5v_sts())
		schedule_work(&clkmsr_dwork);
}

void rx_clkmsr_handler(struct work_struct *work)
{
	int aud_pll;
	int p_clk;
	u8 port = 0;

	switch (rx_info.chip_id) {
	case CHIP_ID_TXL:
		rx[E_PORT0].clk.aud_pll = meson_clk_measure_with_precision(24, 32);
		rx[E_PORT0].clk.pixel_clk = meson_clk_measure_with_precision(29, 32);
		rx[E_PORT0].clk.tmds_clk = meson_clk_measure_with_precision(25, 32);
		rx[E_PORT0].clk.tmds_clk = hdmirx_rd_dwc(DWC_HDMI_CKM_RESULT) & 0xffff;
		rx[E_PORT0].clk.tmds_clk = rx[E_PORT0].clk.tmds_clk * 158000 / 4095 * 1000;
		for (port = E_PORT1; port < rx_info.port_num; port++) {
			rx[port].clk.aud_pll = rx[E_PORT0].clk.aud_pll;
			rx[port].clk.pixel_clk = rx[E_PORT0].clk.pixel_clk;
			rx[port].clk.tmds_clk = rx[E_PORT0].clk.tmds_clk;
		}
		break;
	case CHIP_ID_T5W:
	case CHIP_ID_TXHD2:
		rx[E_PORT0].clk.cable_clk = meson_clk_measure_with_precision(30, 32);
		rx[E_PORT0].clk.tmds_clk = meson_clk_measure_with_precision(63, 32);
		rx[E_PORT0].clk.aud_pll = meson_clk_measure_with_precision(74, 32);
		/* renamed to clk81_hdmirx_pclk,id=7 */
		rx[E_PORT0].clk.p_clk = meson_clk_measure_with_precision(7, 32);
		for (port = E_PORT1; port < rx_info.port_num; port++) {
			rx[port].clk.cable_clk = rx[E_PORT0].clk.cable_clk;
			rx[port].clk.tmds_clk = rx[E_PORT0].clk.tmds_clk;
			rx[port].clk.aud_pll = rx[E_PORT0].clk.aud_pll;
			/* renamed to clk81_hdmirx_pclk,id=7 */
			rx[port].clk.p_clk = rx[E_PORT0].clk.p_clk;
		}
		break;
	case CHIP_ID_T5D:
		rx[E_PORT0].clk.cable_clk = meson_clk_measure_with_precision(30, 32);
		rx[E_PORT0].clk.tmds_clk = meson_clk_measure_with_precision(63, 32);
		rx[E_PORT0].clk.aud_pll = meson_clk_measure_with_precision(74, 32);
		rx[E_PORT0].clk.pixel_clk = meson_clk_measure_with_precision(29, 32);
		for (port = E_PORT1; port < rx_info.port_num; port++) {
			rx[port].clk.cable_clk = rx[E_PORT0].clk.cable_clk;
			rx[port].clk.tmds_clk = rx[E_PORT0].clk.tmds_clk;
			rx[port].clk.aud_pll = rx[E_PORT0].clk.aud_pll;
			rx[port].clk.pixel_clk = rx[E_PORT0].clk.pixel_clk;
		}
		break;
	case CHIP_ID_T7:
	case CHIP_ID_T3:
	case CHIP_ID_T5M:
		/* to decrease cpu loading of clk_msr work queue */
		/* 64: clk_msr resample time 32us,previous setting is 640us */
		rx[E_PORT0].clk.cable_clk = meson_clk_measure_with_precision(44, 32);
		rx[E_PORT0].clk.tmds_clk = meson_clk_measure_with_precision(43, 32);
		rx[E_PORT0].clk.aud_pll = meson_clk_measure_with_precision(104, 32);
		rx[E_PORT0].clk.p_clk = meson_clk_measure_with_precision(0, 32);
		for (port = E_PORT1; port < rx_info.port_num; port++) {
			rx[port].clk.cable_clk = rx[E_PORT0].clk.cable_clk;
			rx[port].clk.tmds_clk = rx[E_PORT0].clk.tmds_clk;
			rx[port].clk.aud_pll = rx[E_PORT0].clk.aud_pll;
			rx[port].clk.p_clk = rx[E_PORT0].clk.p_clk;
		}
		break;
	case CHIP_ID_T5:
	case CHIP_ID_TM2:
	case CHIP_ID_TL1:
		rx[E_PORT0].clk.cable_clk = meson_clk_measure_with_precision(30, 32);
		rx[E_PORT0].clk.tmds_clk = meson_clk_measure_with_precision(63, 32);
		rx[E_PORT0].clk.aud_pll = meson_clk_measure_with_precision(104, 32);
		rx[E_PORT0].clk.pixel_clk = meson_clk_measure_with_precision(29, 32);
		for (port = E_PORT1; port < rx_info.port_num; port++) {
			rx[port].clk.cable_clk = rx[E_PORT0].clk.cable_clk;
			rx[port].clk.tmds_clk = rx[E_PORT0].clk.tmds_clk;
			rx[port].clk.aud_pll = rx[E_PORT0].clk.aud_pll;
			rx[port].clk.pixel_clk = rx[E_PORT0].clk.pixel_clk;
		}
		break;
	case CHIP_ID_T3X:
		aud_pll = meson_clk_measure_with_precision(147, 32);
		p_clk = meson_clk_measure_with_precision(0, 32);
		if (rx[E_PORT0].cur_5v_sts) {
			rx[E_PORT0].clk.cable_clk =
				meson_clk_measure_with_precision(42, 32);
			rx[E_PORT0].clk.tmds_clk = meson_clk_measure_with_precision(47, 32);
			rx[E_PORT0].clk.aud_pll = aud_pll;
			rx[E_PORT0].clk.p_clk = p_clk;
		}
			//Port-B
		if (rx[E_PORT1].cur_5v_sts) {
			rx[E_PORT1].clk.cable_clk =
				meson_clk_measure_with_precision(43, 32);
			rx[E_PORT1].clk.tmds_clk = meson_clk_measure_with_precision(48, 32);
			rx[E_PORT1].clk.aud_pll = aud_pll;
			rx[E_PORT1].clk.p_clk = p_clk;
		}
			//Port-C
		if (rx[E_PORT2].cur_5v_sts) {
			rx[E_PORT2].clk.t_clk_pre = rx[E_PORT2].clk.tclk;
			rx[E_PORT2].clk.aud_pll = aud_pll;
			if (!rx[E_PORT2].var.frl_rate) {
				rx[E_PORT2].clk.cable_clk =
					meson_clk_measure_with_precision(40, 32);
				rx[E_PORT2].clk.tmds_clk = meson_clk_measure(45);
				rx[E_PORT2].clk.p_clk = p_clk;
			} else {
				rx[E_PORT2].clk.fpll_clk = fpll_clk_sel ?
					meson_clk_measure(9) :
					meson_clk_measure_with_precision(9, clk_msr_param);
				rx[E_PORT2].clk.p_clk = p_clk;
				rx[E_PORT2].clk.tclk = meson_clk_measure(49);
			}
		}
			//Port-D
		if (rx[E_PORT3].cur_5v_sts) {
			rx[E_PORT3].clk.t_clk_pre = rx[E_PORT3].clk.tclk;
			rx[E_PORT3].clk.aud_pll = aud_pll;
			if (!rx[E_PORT3].var.frl_rate) {
				rx[E_PORT3].clk.cable_clk =
					meson_clk_measure_with_precision(41, 32);
				rx[E_PORT3].clk.tmds_clk =
					meson_clk_measure_with_precision(46, 32);
				rx[E_PORT3].clk.p_clk = p_clk;
			} else {
				rx[E_PORT3].clk.fpll_clk = fpll_clk_sel ?
					meson_clk_measure(11) :
					meson_clk_measure_with_precision(11, clk_msr_param);
				rx[E_PORT3].clk.p_clk = p_clk;
				rx[E_PORT3].clk.tclk = meson_clk_measure(50);
			}
		}
		break;
	default:
		break;
	}
	//if (rx[rx_info.main_port].state == FSM_SIG_READY)
			//rx[rx_info.main_port].clk.mpll_clk =
				//meson_clk_measure_with_precision(27, 32);
}

bool is_earc_hpd_low(void)
{
	return earc_hpd_low_flag;
}

void rx_earc_hpd_handler(struct work_struct *work)
{
	earc_hpd_low_flag = true;
	usleep_range(30000, 40000);
	cancel_delayed_work(&eq_dwork);
	skip_frame(2, rx_info.main_port, "earc skip");
	rx_pr("earc call hpd\n");
	rx_set_port_hpd(rx_info.arc_port, 0);
	usleep_range(600000, 650000);
	rx_set_port_hpd(rx_info.arc_port, 1);
	earc_hpd_low_flag = false;
}

static const u32 wr_only_register[] = {
0x0c, 0x3c, 0x60, 0x64, 0x68, 0x6c, 0x70, 0x74, 0x78, 0x7c, 0x8c, 0xa0,
0xac, 0xc8, 0xd8, 0xdc, 0x184, 0x188, 0x18c, 0x190, 0x194, 0x198, 0x19c,
0x1a0, 0x1a4, 0x1a8, 0x1ac, 0x1b0, 0x1b4, 0x1b8, 0x1bc, 0x1c0, 0x1c4,
0x1c8, 0x1cc, 0x1d0, 0x1d4, 0x1d8, 0x1dc, 0x1e0, 0x1e4, 0x1e8, 0x1ec,
0x1f0, 0x1f4, 0x1f8, 0x1fc, 0x204, 0x20c, 0x210, 0x214, 0x218, 0x21c,
0x220, 0x224, 0x228, 0x22c, 0x230, 0x234, 0x238, 0x268, 0x26c, 0x270,
0x274, 0x278, 0x290, 0x294, 0x298, 0x29c, 0x2a8, 0x2ac, 0x2b0, 0x2b4,
0x2b8, 0x2bc, 0x2d4, 0x2dc, 0x2e8, 0x2ec, 0x2f0, 0x2f4, 0x2f8, 0x2fc,
0x314, 0x318, 0x328, 0x32c, 0x348, 0x34c, 0x350, 0x354, 0x358, 0x35c,
0x384, 0x388, 0x38c, 0x398, 0x39c, 0x3d8, 0x3dc, 0x400, 0x404, 0x408,
0x40c, 0x410, 0x414, 0x418, 0x810, 0x814, 0x818, 0x830, 0x834, 0x838,
0x83c, 0x854, 0x858, 0x85c, 0xf60, 0xf64, 0xf70, 0xf74, 0xf78, 0xf7c,
0xf88, 0xf8c, 0xf90, 0xf94, 0xfa0, 0xfa4, 0xfa8, 0xfac, 0xfb8, 0xfbc,
0xfc0, 0xfc4, 0xfd0, 0xfd4, 0xfd8, 0xfdc, 0xfe8, 0xfec, 0xff0, 0x1f04,
0x1f0c, 0x1f10, 0x1f24, 0x1f28, 0x1f2c, 0x1f30, 0x1f34, 0x1f38, 0x1f3c
};

bool is_wr_only_reg(u32 addr)
{
	int i;

	if (rx_info.chip_id >= CHIP_ID_T7)
		return false;

	for (i = 0; i < sizeof(wr_only_register) / sizeof(u32); i++) {
		if (addr == wr_only_register[i])
			return true;
	}
	return false;
}

void rx_debug_load22key(u8 port)
{
	int ret = 0;
	int wait_kill_done_cnt = 0;

	if (rx_info.chip_id >= CHIP_ID_T7) {
		rx[port].fsm_ext_state = FSM_HPD_LOW;
	} else {
		ret = rx_sec_set_duk(hdmirx_repeat_support());
		rx_pr("22 = %d\n", ret);
		if (ret) {
			rx_pr("load 2.2 key\n");
			sm_pause = 1;
			rx_set_cur_hpd(0, 4, port);
			hdcp22_on = 1;
			hdcp22_kill_esm = 1;
			while (wait_kill_done_cnt++ < 10) {
				if (!hdcp22_kill_esm)
					break;
				msleep(20);
			}
			hdcp22_kill_esm = 0;
			rx_hdcp22_send_uevent(0);
			hdmirx_wr_dwc(DWC_HDCP22_CONTROL, 0x0);
			/* if key_a is already exist on platform,*/
			/*need to set valid bit to 0 before burning key_b,*/
			/*otherwise,key_b will not be activated*/
			rx_hdcp22_wr_top(TOP_SKP_CNTL_STAT, 0x1);
			hdmirx_hdcp22_esm_rst();
			mdelay(110);
			rx_is_hdcp22_support();
			hdmirx_wr_dwc(DWC_HDCP22_CONTROL, 0x1000);
			/* rx_hdcp22_wr_top(TOP_SKP_CNTL_STAT, 0x1); */
			hdcp22_clk_en(1);
			rx_hdcp22_send_uevent(1);
			mdelay(100);
			hdmi_rx_top_edid_update();
			hdmirx_hw_config(port);
			hpd_to_esm = 1;
			/* mdelay(900); */
			rx_set_cur_hpd(1, 4, port);
			sm_pause = 0;
		}
	}
	rx_pr("%s\n", __func__);
}

void rx_debug_loadkey(u8 port)
{
	rx_pr("load hdcp key\n");
	hdmi_rx_top_edid_update();
	hdmirx_hw_config(port);
	pre_port = 0xfe;
}

void print_reg(uint start_addr, uint end_addr)
{
	int i;
	int k = 0;
	unsigned char buff[512] = {0};
	u8 port = rx_info.main_port;

	if (end_addr < start_addr)
		return;

	for (i = start_addr; i <= end_addr; i += sizeof(uint)) {
		if (rx_info.chip_id >= CHIP_ID_T7) {
			if ((i - start_addr) % (sizeof(uint) * 4) == 0)
				k += sprintf(buff + k, "[0x%-4x] ", i);
			k += sprintf(buff + k, "0x%02x,   ", hdmirx_rd_cor(i, port));
			k += sprintf(buff + k, "0x%02x,   ", hdmirx_rd_cor(i + 1, port));
			k += sprintf(buff + k, "0x%02x,   ", hdmirx_rd_cor(i + 2, port));
			k += sprintf(buff + k, "0x%02x,   ", hdmirx_rd_cor(i + 3, port));
		} else {
			if ((i - start_addr) % (sizeof(uint) * 4) == 0)
				k += sprintf(buff + k, "[0x%-4x] ", i);
			if (!is_wr_only_reg(i))
				k += sprintf(buff + k, "0x%x,   ", hdmirx_rd_dwc(i));
			else
				k += sprintf(buff + k, "xxxx,   ");
		}
		if ((i - start_addr) % (sizeof(uint) * 4) == sizeof(uint) * 3) {
			if (rx_info.chip_id >= CHIP_ID_T7) {
				pr_info("%s", buff);
				k = 0;
			} else {
				pr_info("%s", buff);
				k = 0;
				rx_pr(" ");
			}
		}
	}

	if ((end_addr - start_addr + sizeof(uint)) % (sizeof(uint) * 4) != 0)
		rx_pr(" ");
}

void dump_reg(u8 port)
{
	int i = 0;
	int k = 0;
	unsigned char buff[512] = {0};

	rx_pr("\n*** dump port: %d ***\n", port);
	rx_pr("\n***Top registers***\n");
	pr_info("[addr ]  addr + 0x0, addr + 0x1, addr + 0x2, addr + 0x3\n");
	for (i = 0; i <= 0x84;) {
		pr_cont("[0x%-3x]", i);
		pr_cont("0x%-8x,0x%-8x,0x%-8x,0x%-8x\n",
				hdmirx_rd_top(i, port),
				hdmirx_rd_top(i + 1, port),
				hdmirx_rd_top(i + 2, port),
				hdmirx_rd_top(i + 3, port));
		i = i + 4;
	}

	if (rx_info.chip_id == CHIP_ID_T3X) {
		rx_pr("\n***Top common registers***\n");
		pr_info("[addr ]  addr + 0x0, addr + 0x1, addr + 0x2, addr + 0x3\n");
		for (i = 0; i <= 0xff; ) {
			k += sprintf(buff + k, "[0x%-3x]", i);
			k += sprintf(buff + k, "  0x%-8x,0x%-8x,0x%-8x,0x%-8x\n",
					hdmirx_rd_top_common(i),
					hdmirx_rd_top_common(i + 1),
					hdmirx_rd_top_common(i + 2),
					hdmirx_rd_top_common(i + 3));
			pr_info("%s ", buff);
			k = 0;
			i += 4;
		}
		k = 0;
		rx_pr("\n***Top common vid registers***\n");
		for (i = 0xc; i <= 0xf; i++) {
			k += sprintf(buff + k, "vid0 0x%x value: 0x%x\n",
				i, hdmirx_rd_top_common(i));
			k += sprintf(buff + k, "vid1 0x%x value: 0x%x\n",
				i, hdmirx_rd_top_common_1(i));
		}
		pr_info("%s", buff);
		k = 0;
		for (i = 0x10; i <= 0x13; i++) {
			k += sprintf(buff + k, "vid0 0x%x value: 0x%x\n",
				i, hdmirx_rd_top_common(i));
			k += sprintf(buff + k, "vid1 0x%x value: 0x%x\n",
				i, hdmirx_rd_top_common_1(i));
		}
		pr_info("%s", buff);
		k = 0;
		for (i = 0x46; i <= 0x4b; i++) {
			k += sprintf(buff + k, "vid0 0x%x value: 0x%x\n",
				i, hdmirx_rd_top_common(i));
			k += sprintf(buff + k, "vid1 0x%x value: 0x%x\n",
				i, hdmirx_rd_top_common_1(i));
		}
		pr_info("%s", buff);
	}
	if (rx_info.chip_id < CHIP_ID_TL1) {
		rx_pr("\n***PHY registers***\n");
		pr_cont("[addr ]  addr + 0x0,");
		pr_cont("addr + 0x1,addr + 0x2,");
		rx_pr("addr + 0x3\n");
		for (i = 0; i <= 0x9a;) {
			pr_cont("[0x%-3x]", i);
			pr_cont("0x%-8x,0x%-8x,0x%-8x,0x%-8x\n",
				  hdmirx_rd_phy(i),
			      hdmirx_rd_phy(i + 1),
			      hdmirx_rd_phy(i + 2),
			      hdmirx_rd_phy(i + 3));
			i = i + 4;
		}
	} else if (rx_info.chip_id >= CHIP_ID_TL1) {
		/* dump phy register */
		dump_reg_phy(port);
	}

	if (rx_info.chip_id < CHIP_ID_T7) {
		rx_pr("\n**Controller registers**\n");
		pr_cont("[addr ]  addr + 0x0,");
		pr_cont("addr + 0x4,  addr + 0x8,");
		rx_pr("addr + 0xc\n");
		print_reg(0, 0xfc);
		print_reg(0x140, 0x3ac);
		print_reg(0x3c0, 0x418);
		print_reg(0x480, 0x4bc);
		print_reg(0x600, 0x610);
		print_reg(0x800, 0x87c);
		print_reg(0x8e0, 0x8e0);
		print_reg(0x8fc, 0x8fc);
		print_reg(0xf60, 0xffc);
	} else {
		print_reg(0x0, 0xfe);
		print_reg(0x300, 0x3ff);
		print_reg(0x1001, 0x1f78);
	}
}

void dump_reg_phy(u8 port)
{
	if (rx_info.phy_ver == PHY_VER_T5)
		dump_reg_phy_t5();
	else if (rx_info.phy_ver >= PHY_VER_T7 && rx_info.phy_ver <= PHY_VER_T5W)
		dump_reg_phy_t7();
	else if (rx_info.phy_ver == PHY_VER_T5M)
		dump_reg_phy_t5m();
	else if (rx_info.phy_ver == PHY_VER_TXHD2)
		dump_reg_phy_txhd2();
	else if (rx_info.phy_ver == PHY_VER_T3X)
		dump_reg_phy_t3x(port);
	else
		dump_reg_phy_tl1_tm2();
}

void dump_edid_reg(u32 size)
{
	int i = 0;
	int j = 0;

	rx_pr("********************************\n");
	if (rx_info.chip_id < CHIP_ID_TL1) {
		for (i = 0; i < 16; i++) {
			pr_cont("[%2d] ", i);
			for (j = 0; j < 16; j++) {
				pr_cont("0x%02x, ",
				     hdmirx_rd_edid(TOP_EDID_ADDR +
						    (i * 16 + j)));
			}
			rx_pr(" ");
		}
	} else if (rx_info.chip_id == CHIP_ID_TL1) {
		for (i = 0; i < 16; i++) {
			pr_cont("[%2d] ", i);
			for (j = 0; j < 16; j++) {
				pr_cont("0x%02x, ",
				      hdmirx_rd_edid(TOP_EDID_ADDR_S +
						   (i * 16 + j)));
			}
			rx_pr(" ");
		}
	} else {
		rx_pr("Port-A\n");
		for (i = 0; i * 64 < size; i++) {
			for (j = 0; j < 64; j++)
				pr_cont("%02x",
						hdmirx_rd_edid(TOP_EDID_ADDR_S + (i * 64 + j)));
			rx_pr("");
		}
		rx_pr("Port-B\n");
		for (i = 0; i * 64 < size; i++) {
			for (j = 0; j < 64; j++)
				pr_cont("%02x",
						hdmirx_rd_edid(TOP_EDID_PORT2_ADDR_S +
									 (i * 64 + j)));
			rx_pr("");
		}
		rx_pr("Port-C\n");
		for (i = 0; i * 64 < size; i++) {
			for (j = 0; j < 64; j++)
				pr_cont("%02x",
						hdmirx_rd_edid(TOP_EDID_PORT3_ADDR_S +
									  (i * 64 + j)));
			rx_pr("");
		}
		if (rx_info.chip_id >= CHIP_ID_T5M) {
			rx_pr("Port-D\n");
			for (i = 0; i * 64 < size; i++) {
				for (j = 0; j < 64; j++)
					pr_cont("%02x",
							hdmirx_rd_edid(TOP_EDID_PORT4_ADDR_S +
										  (i * 64 + j)));
				rx_pr("");
			}
		}
	}
}

int rx_debug_wr_reg(const char *buf, char *tmpbuf, int i, u8 port)
{
	u32 adr = 0;
	u32 value = 0;

	if (kstrtou32(tmpbuf + 3, 16, &adr) < 0)
		return -EINVAL;
	rx_pr("adr = %#x\n", adr);
	if (kstrtou32(buf + i + 1, 16, &value) < 0)
		return -EINVAL;
	rx_pr("value = %#x\n", value);
	if (tmpbuf[1] == 'h') {
		if (buf[2] == 't') {
			hdmirx_wr_top(adr, value, port);
			rx_pr("write %x to TOP [%x]\n", value, adr);
		} else if (tmpbuf[2] == 'T') {
			hdmirx_wr_top_common(adr, value);
			rx_pr("write %x to TOP_common [%x]\n", value, adr);
		} else if (tmpbuf[2] == 'Y') {
			hdmirx_wr_top_common_1(adr, value);
			rx_pr("write %x to TOP_common [%x]\n", value, adr);
		} else if (buf[2] == 'd') {
			if (rx_info.chip_id >= CHIP_ID_T7)
				hdmirx_wr_cor(adr, value, port);
			else
				hdmirx_wr_dwc(adr, value);
			rx_pr("write %x to DWC [%x]\n", value, adr);
		} else if (buf[2] == 'p') {
			hdmirx_wr_phy(adr, value);
			rx_pr("write %x to PHY [%x]\n", value, adr);
		} else if (buf[2] == 'u') {
			wr_reg_hhi(adr, value);
			rx_pr("write %x to hiu [%x]\n", value, adr);
		} else if (buf[2] == 's') {
			rx_hdcp22_wr_top(adr, value);
			rx_pr("write %x to sec-top [%x]\n", value, adr);
		} else if (buf[2] == 'h') {
			rx_hdcp22_wr_reg(adr, value);
			rx_pr("write %x to esm [%x]\n", value, adr);
		} else if (buf[2] == 'a') {
			hdmirx_wr_amlphy(adr, value);
			rx_pr("write %x to amlphy [%x]\n", value, adr);
		} else if (buf[2] == 'A') {
			hdmirx_wr_amlphy_t3x(adr, value, port);
			rx_pr("write %x to port%d [%x]\n", value, port, adr);
		} else if (buf[2] == 'c') {
			wr_reg_clk_ctl(adr, value);
			rx_pr("write %x to [%x]\n", value, adr);
		}
	}
	return 0;
}

int rx_debug_rd_reg(const char *buf, char *tmpbuf, u8 port)
{
	u32 adr = 0;
	u32 value = 0;

	if (tmpbuf[1] == 'h') {
		if (kstrtou32(tmpbuf + 3, 16, &adr) < 0)
			return -EINVAL;
		if (tmpbuf[2] == 't') {
			value = hdmirx_rd_top(adr, port);
			rx_pr("TOP [%x]=%x\n", adr, value);
		} else if (tmpbuf[2] == 'T') {
			value = hdmirx_rd_top_common(adr);
			rx_pr("TOP_common [%x]=%x\n", adr, value);
		} else if (tmpbuf[2] == 'Y') {
			value = hdmirx_rd_top_common_1(adr);
			rx_pr("TOP_common [%x]=%x\n", adr, value);
		} else if (tmpbuf[2] == 'd') {
			if (rx_info.chip_id >= CHIP_ID_T7)
				value = hdmirx_rd_cor(adr, port);
			else
				value = hdmirx_rd_dwc(adr);
			rx_pr("DWC [%x]=%x\n", adr, value);
		} else if (tmpbuf[2] == 'p') {
			value = hdmirx_rd_phy(adr);
			rx_pr("PHY [%x]=%x\n", adr, value);
		} else if (tmpbuf[2] == 'u') {
			value = rd_reg_hhi(adr);
			rx_pr("HIU [%x]=%x\n", adr, value);
		} else if (tmpbuf[2] == 's') {
			value = rx_hdcp22_rd_top(adr);
			rx_pr("SEC-TOP [%x]=%x\n", adr, value);
		} else if (tmpbuf[2] == 'h') {
			value = rx_hdcp22_rd_reg(adr);
			rx_pr("esm [%x]=%x\n", adr, value);
		} else if (tmpbuf[2] == 'a') {
			value = hdmirx_rd_amlphy(adr);
			rx_pr("amlphy [%x]=%x\n", adr, value);
		} else if (buf[2] == 'A') {
			value = hdmirx_rd_amlphy_t3x(adr, port);
			rx_pr("port%d, amlphy [%x]=%x\n", port, adr, value);
		} else if (buf[2] == 'c') {
			value = rd_reg_clk_ctl(adr);
			rx_pr("amlphy [%x]=%x\n", adr, value);
		}
	}
	return 0;
}

int rx_get_aud_pll_err_sts(u8 port)
{
	int ret = E_AUDPLL_OK;
	u32 req_clk = 0;
	u32 aud_clk = 0;//rx[rx_info.main_port].aud_info.aud_clk;
	u32 phy_pll_rate = 0;//(hdmirx_rd_phy(PHY_MAINFSM_STATUS1) >> 9) & 0x3;
	u32 aud_pll_cntl = 0;//(rd_reg_hhi(HHI_AUD_PLL_CNTL6) >> 28) & 0x3;

	if (rx_info.chip_id >= CHIP_ID_TL1)
		return ret;

	req_clk = rx[port].clk.mpll_clk;
	aud_clk = rx[port].aud_info.aud_clk;
	phy_pll_rate = (hdmirx_rd_phy(PHY_MAINFSM_STATUS1) >> 9) & 0x3;
	aud_pll_cntl = (rd_reg_hhi(HHI_AUD_PLL_CNTL6) >> 28) & 0x3;

	if (req_clk > PHY_REQUEST_CLK_MAX ||
	    req_clk < PHY_REQUEST_CLK_MIN) {
		ret = E_REQUESTCLK_ERR;
		if (log_level & AUDIO_LOG)
			rx_pr("request clk err:%d\n", req_clk);
	} else if (phy_pll_rate != aud_pll_cntl) {
		ret = E_PLLRATE_CHG;
		if (log_level & AUDIO_LOG)
			rx_pr("pll rate chg,phy=%d,pll=%d\n",
			      phy_pll_rate, aud_pll_cntl);
	} else if (aud_clk == 0) {
		ret = E_AUDCLK_ERR;
		if (log_level & AUDIO_LOG)
			rx_pr("aud_clk=0\n");
	}

	return ret;
}

u32 aml_cable_clk_band(u32 cable_clk, u32 clk_rate)
{
	u32 bw;
	u32 cab_clk = cable_clk;

	if (rx_info.chip_id < CHIP_ID_TL1)
		return PHY_BW_2;

	/* rx_pr("cable clk=%d, clk_rate=%d\n", cable_clk, clk_rate); */
	/* 1:40 */
	if (clk_rate)
		cab_clk = cable_clk << 2;

	/* 1:10 */
	if (rx_info.chip_id >= CHIP_ID_T5M) {
		if (cab_clk < (37 * MHz))
			bw = PHY_BW_0;
		else if (cab_clk < (75 * MHz))
			bw = PHY_BW_1;
		else if (cab_clk < (115 * MHz))
			bw = PHY_BW_2;
		else if (cab_clk < (150 * MHz))
			bw = PHY_BW_3;
		else if (cab_clk < (300 * MHz))
			bw = PHY_BW_4;
		else if (cab_clk < (525 * MHz))
			bw = PHY_BW_5;
		else if (cab_clk < (600 * MHz))
			bw = PHY_BW_6;
		else
			bw = PHY_BW_3;
	} else {
		if (cab_clk < (45 * MHz))
			bw = PHY_BW_0;
		else if (cab_clk < (77 * MHz))
			bw = PHY_BW_1;
		else if (cab_clk < (155 * MHz))
			bw = PHY_BW_2;
		else if (cab_clk < (340 * MHz))
			bw = PHY_BW_3;
		else if (cab_clk < (525 * MHz))
			bw = PHY_BW_4;
		else if (cab_clk < (600 * MHz))
			bw = PHY_BW_5;
		else
			bw = PHY_BW_2;
	}
	return bw;
}

u32 aml_phy_pll_band(u32 cable_clk, u32 clk_rate)
{
	u32 bw;
	u32 cab_clk = cable_clk;

	if (clk_rate)
		cab_clk = cable_clk << 2;

	/* 1:10 */
	if (rx_info.chip_id >= CHIP_ID_T5M) {
		if (cab_clk < (37 * MHz))
			bw = PLL_BW_0;
		else if (cab_clk < (75 * MHz))
			bw = PLL_BW_1;
		else if (cab_clk < (150 * MHz))
			bw = PLL_BW_2;
		else if (cab_clk < (300 * MHz))
			bw = PLL_BW_3;
		else if (cab_clk < (600 * MHz))
			bw = PLL_BW_4;
		else
			bw = PLL_BW_2;
	} else {
		if (cab_clk < (35 * MHz))
			bw = PLL_BW_0;
		//CVT 1280X720 is 77M
		else if (cab_clk < (75 * MHz))
			bw = PLL_BW_1;
		else if (cab_clk < (155 * MHz))
			bw = PLL_BW_2;
		else if (cab_clk < (340 * MHz))
			bw = PLL_BW_3;
		else if (cab_clk < (600 * MHz))
			bw = PLL_BW_4;
		else
			bw = PLL_BW_2;
	}
	return bw;
}

void aml_phy_switch_port(u8 port)
{
	if (rx_info.chip_id == CHIP_ID_TL1)
		aml_phy_switch_port_tl1();
	else if (rx_info.chip_id == CHIP_ID_TM2)
		aml_phy_switch_port_tm2();
	else if (rx_info.chip_id >= CHIP_ID_T5 &&
		rx_info.chip_id <= CHIP_ID_T5D)
		aml_phy_switch_port_t5();
	else if (rx_info.chip_id >= CHIP_ID_T7 && rx_info.chip_id <= CHIP_ID_T5W)
		aml_phy_switch_port_t7();
	else if (rx_info.chip_id == CHIP_ID_T5M)
		aml_phy_switch_port_t5m();
	else if (rx_info.chip_id == CHIP_ID_TXHD2)
		aml_phy_switch_port_txhd2();
	else if (rx_info.chip_id == CHIP_ID_T3X)
		aml_phy_switch_port_t3x(port);
}

bool is_ft_trim_done(void)
{
	int ret = phy_trim_val & 0x1;

	rx_pr("ft trim=%d\n", ret);
	return ret;
}

/*T5 todo:*/
void aml_phy_get_trim_val_tl1_tm2(void)
{
	phy_trim_val = def_trim_value;
	dts_debug_flag = (phy_term_lel >> 4) & 0x1;
	rlevel = phy_term_lel & 0xf;
	if (rlevel > 11)
		rlevel = 10;
	phy_tdr_en = dts_debug_flag;
}

void aml_phy_get_trim_val(void)
{
	if (rx_info.chip_id >= CHIP_ID_TL1 &&
		rx_info.chip_id <= CHIP_ID_TM2)
		aml_phy_get_trim_val_tl1_tm2();
	else if (rx_info.chip_id >= CHIP_ID_T5 &&
		rx_info.chip_id <= CHIP_ID_T5D)
		aml_phy_get_trim_val_t5();
	else if (rx_info.chip_id >= CHIP_ID_T7 &&
		rx_info.chip_id <= CHIP_ID_T5W)
		aml_phy_get_trim_val_t7();
	else if (rx_info.chip_id == CHIP_ID_T5M)
		aml_phy_get_trim_val_t5m();
	else if (rx_info.chip_id == CHIP_ID_T3X)
		aml_phy_get_trim_val_t3x();
	else if (rx_info.chip_id == CHIP_ID_TXHD2)
		aml_phy_get_trim_val_txhd2();
}

void rx_get_best_eq_setting(u8 port)
{
	u32 ch0, ch1, ch2, ch3;
	static u32 err_sum;
	static u32 time_cnt;
	static u32 array_cnt;

	if (rx_info.chip_id < CHIP_ID_TL1 || !find_best_eq)
		return;
	if (find_best_eq >= 0x7777 || array_cnt >= 255) {
		rx_pr("eq traversal completed.\n");
		rx_pr("best eq value:%d\n", array_cnt);
		if (array_cnt) {
			do  {
				rx_pr("%x:\n", rx[port].phy.eq_data[array_cnt]);
			} while (array_cnt--);
		} else {
			rx_pr("%x:\n", rx[port].phy.eq_data[array_cnt]);
		}
		find_best_eq = 0;
		array_cnt = 0;
		return;
	}
	if (time_cnt == 0) {
		hdmirx_phy_init(port);
		udelay(1);
		wr_reg_hhi_bits(HHI_HDMIRX_PHY_DCHD_CNTL1, MSK(16, 4), find_best_eq);
		udelay(2);
		wr_reg_hhi_bits(HHI_HDMIRX_PHY_DCHD_CNTL1, _BIT(22), 1);
		rx_pr("set eq:%x\n", find_best_eq);
		err_sum = 0;
		do {
			find_best_eq++;
		} while (((find_best_eq & 0xf) > 7) ||
				(((find_best_eq >> 4) & 0xf) > 7) ||
				(((find_best_eq >> 8) & 0xf) > 7) ||
				(((find_best_eq >> 12) & 0xf) > 7));
	}
	time_cnt++;
	if (time_cnt > 2) {
		if (!is_tmds_valid(port))
			return;
	}
	if (time_cnt > 4) {
		rx_get_error_cnt(&ch0, &ch1, &ch2, &ch3, port);
		err_sum += (ch0 + ch1 + ch2);
	}
	if (time_cnt > eq_try_cnt) {
		time_cnt = 0;
		if (err_sum < rx[port].phy.err_sum) {
			rx[port].phy.err_sum = err_sum;
			rx_pr("err_sum = %d\n", err_sum);
			array_cnt = 0;
			rx[port].phy.eq_data[array_cnt] = find_best_eq;
		} else if ((err_sum == rx[port].phy.err_sum) ||
			(err_sum == 0)) {
			rx[port].phy.err_sum = err_sum;
			array_cnt++;
			rx_pr("array = %x\n", array_cnt);
			rx[port].phy.eq_data[array_cnt] = find_best_eq;
		}
	}
}

bool is_tmds_clk_stable(u8 port)
{
	bool ret = true;
	u32 cable_clk;

	if (rx[port].phy.clk_rate)
		cable_clk = rx[port].clk.cable_clk * 4;
	else
		cable_clk = rx[port].clk.cable_clk;

	if (abs(cable_clk - rx[port].clk.tmds_clk) > clock_lock_th * MHz) {
		if (log_level & VIDEO_LOG)
			rx_pr("cable_clk=%d,tmdsclk=%d,\n",
			      cable_clk / MHz, rx[port].clk.tmds_clk / MHz);
		ret = false;
	} else {
		ret = true;
	}
	return ret;
}

void aml_phy_init_handler_port0(struct work_struct *work)
{
	if (rx_info.phy_ver == PHY_VER_TL1)
		aml_phy_init_tl1();
	else if (rx_info.phy_ver == PHY_VER_TM2)
		aml_phy_init_tm2();
	else if (rx_info.phy_ver == PHY_VER_T5)
		aml_phy_init_t5();
	else if (rx_info.phy_ver >= PHY_VER_T7 && rx_info.phy_ver <= PHY_VER_T5W)
		aml_phy_init_t7();
	else if (rx_info.phy_ver == PHY_VER_T5M)
		aml_phy_init_t5m();
	else if (rx_info.phy_ver == PHY_VER_T3X)
		aml_phy_init_t3x(E_PORT0);
	else if (rx_info.phy_ver == PHY_VER_TXHD2)
		aml_phy_init_txhd2();
	eq_sts[E_PORT0] = E_EQ_FINISH;
}

void aml_phy_init_handler_port1(struct work_struct *work)
{
	if (rx_info.phy_ver == PHY_VER_TL1)
		aml_phy_init_tl1();
	else if (rx_info.phy_ver == PHY_VER_TM2)
		aml_phy_init_tm2();
	else if (rx_info.phy_ver == PHY_VER_T5)
		aml_phy_init_t5();
	else if (rx_info.phy_ver >= PHY_VER_T7 && rx_info.phy_ver <= PHY_VER_T5W)
		aml_phy_init_t7();
	else if (rx_info.phy_ver == PHY_VER_T5M)
		aml_phy_init_t5m();
	else if (rx_info.phy_ver == PHY_VER_T3X)
		aml_phy_init_t3x(E_PORT1);
	else if (rx_info.phy_ver == PHY_VER_TXHD2)
		aml_phy_init_txhd2();
	eq_sts[E_PORT1] = E_EQ_FINISH;
}

void aml_phy_init_handler_port2(struct work_struct *work)
{
	if (rx_info.phy_ver == PHY_VER_TL1)
		aml_phy_init_tl1();
	else if (rx_info.phy_ver == PHY_VER_TM2)
		aml_phy_init_tm2();
	else if (rx_info.phy_ver == PHY_VER_T5)
		aml_phy_init_t5();
	else if (rx_info.phy_ver >= PHY_VER_T7 && rx_info.phy_ver <= PHY_VER_T5W)
		aml_phy_init_t7();
	else if (rx_info.phy_ver == PHY_VER_T5M)
		aml_phy_init_t5m();
	else if (rx_info.phy_ver == PHY_VER_T3X)
		aml_phy_init_t3x(E_PORT2);
	else if (rx_info.phy_ver == PHY_VER_TXHD2)
		aml_phy_init_txhd2();
	eq_sts[E_PORT2] = E_EQ_FINISH;
}

void aml_phy_init_handler_port3(struct work_struct *work)
{
	if (rx_info.phy_ver == PHY_VER_TL1)
		aml_phy_init_tl1();
	else if (rx_info.phy_ver == PHY_VER_TM2)
		aml_phy_init_tm2();
	else if (rx_info.phy_ver == PHY_VER_T5)
		aml_phy_init_t5();
	else if (rx_info.phy_ver >= PHY_VER_T7 && rx_info.phy_ver <= PHY_VER_T5W)
		aml_phy_init_t7();
	else if (rx_info.phy_ver == PHY_VER_T5M)
		aml_phy_init_t5m();
	else if (rx_info.phy_ver == PHY_VER_T3X)
		aml_phy_init_t3x(E_PORT3);
	else if (rx_info.phy_ver == PHY_VER_TXHD2)
		aml_phy_init_txhd2();
	eq_sts[E_PORT3] = E_EQ_FINISH;
}

void aml_phy_init(u8 port)
{
	if (port == E_PORT0)
		schedule_work(&aml_phy_dwork_port0);
	else if (port == E_PORT1)
		schedule_work(&aml_phy_dwork_port1);
	else if (port == E_PORT2)
		schedule_work(&aml_phy_dwork_port2);
	else
		schedule_work(&aml_phy_dwork_port3);
	eq_sts[port] = E_EQ_START;
}

/*
 * hdmirx_phy_init - hdmirx phy initialization
 */
void hdmirx_phy_init(u8 port)
{
	if (rx_info.chip_id >= CHIP_ID_TL1)
		aml_phy_init(port);
	else
		snps_phyg3_init();
}

void rx_phy_short_bist(u8 port)
{
	if (rx_info.phy_ver == PHY_VER_TL1)
		aml_phy_short_bist_tl1();
	else if (rx_info.phy_ver == PHY_VER_TM2)
		aml_phy_short_bist_tm2();
	else if (rx_info.phy_ver == PHY_VER_T5)
		aml_phy_short_bist_t5();
	else if (rx_info.phy_ver >= PHY_VER_T7 && rx_info.phy_ver <= PHY_VER_T5W)
		aml_phy_short_bist_t7();
	else if (rx_info.phy_ver == PHY_VER_T5M)
		aml_phy_short_bist_t5m();
	else if (rx_info.phy_ver == PHY_VER_T3X)
		aml_phy_short_bist_t3x(port);
	else if (rx_info.phy_ver == PHY_VER_TXHD2)
		aml_phy_short_bist_txhd2();
}

u32 aml_phy_pll_lock_tm2(void)
{
	if (rd_reg_hhi(TM2_HHI_HDMIRX_APLL_CNTL0) & 0x80000000)
		return true;
	else
		return false;
}

u32 aml_phy_pll_lock_t5(void)
{
	if (hdmirx_rd_amlphy(T5_HHI_RX_APLL_CNTL0) & 0x80000000)
		return true;
	else
		return false;
}

u32 aml_phy_pll_lock_t3x(u8 port)
{
	if (hdmirx_rd_amlphy_t3x(T5_HHI_RX_APLL_CNTL0, port) & 0x80000000)
		return true;
	else
		return false;
}

u32 aml_phy_pll_lock(u8 port)
{
	if (rx_info.chip_id >= CHIP_ID_T3X)
		return aml_phy_pll_lock_t3x(port);
	else if (rx_info.chip_id >= CHIP_ID_T5)
		return aml_phy_pll_lock_t5();
	else
		return aml_phy_pll_lock_tm2();
}

bool is_tmds_valid(u8 port)
{
	if (force_vic)
		return true;

	if (!rx[port].cableclk_stb_flg)
		return false;

	if (rx_info.chip_id >= CHIP_ID_TL1)
		return (aml_phy_tmds_valid(port) == 1) ? true : false;
	else
		return (rx_get_pll_lock_sts() == 1) ? true : false;
}

u32 aml_phy_tmds_valid(u8 port)
{
	if (rx_info.phy_ver == PHY_VER_TL1)
		return aml_get_tmds_valid_tl1();
	else if (rx_info.phy_ver == PHY_VER_TM2)
		return aml_get_tmds_valid_tm2();
	else if (rx_info.phy_ver == PHY_VER_T5)
		return aml_get_tmds_valid_t5();
	else if (rx_info.phy_ver >= PHY_VER_T7 && rx_info.phy_ver <= PHY_VER_T5W)
		return aml_get_tmds_valid_t7();
	else if (rx_info.phy_ver == PHY_VER_T5M)
		return aml_get_tmds_valid_t5m();
	else if (rx_info.phy_ver == PHY_VER_T3X)
		return aml_get_tmds_valid_t3x(port);
	else if (rx_info.phy_ver == PHY_VER_TXHD2)
		return aml_get_tmds_valid_txhd2();
	else
		return false;
}

void rx_phy_rxsense_pulse(u32 t1, u32 t2, bool en)
{
	/* set rxsense pulse */
	hdmirx_phy_pddq(!en);
	mdelay(t1);
	hdmirx_phy_pddq(en);
	mdelay(t2);
}

void aml_phy_power_off(void)
{
	switch (rx_info.phy_ver) {
	case PHY_VER_TL1:
		aml_phy_power_off_tl1();
		break;
	case PHY_VER_TM2:
		aml_phy_power_off_tm2();
		break;
	case PHY_VER_T5:
		aml_phy_power_off_t5();
		break;
	case PHY_VER_T7:
	case PHY_VER_T3:
	case PHY_VER_T5W:
		aml_phy_power_off_t7();
		break;
	case PHY_VER_T5M:
		aml_phy_power_off_t5m();
		break;
	case PHY_VER_TXHD2:
		aml_phy_power_off_txhd2();
		break;
	case PHY_VER_T3X:
		aml_phy_power_off_t3x(rx_info.port_num);
		break;
	default:
		rx_pr("rx not poweroff\n");
		break;
	}
	if (log_level & VIDEO_LOG)
		rx_pr("%s\n", __func__);
}

void rx_phy_power_on(u32 onoff)
{
	if (onoff)
		hdmirx_phy_pddq(0);
	else
		hdmirx_phy_pddq(1);
	if (rx_info.chip_id >= CHIP_ID_TL1) {
		/*the enable of these regs are in phy init*/
		if (onoff == 0)
			aml_phy_power_off();
	}
}

bool rx_is_phy_power_off(u8 port)
{
	return rx_is_power_off_t3x(port);
}

void aml_phy_iq_skew_monitor(void)
{
	if (rx_info.phy_ver == PHY_VER_T5)
		aml_phy_iq_skew_monitor_t5();
	else if (rx_info.phy_ver >= PHY_VER_T7 && rx_info.phy_ver <= PHY_VER_T5W)
		aml_phy_iq_skew_monitor_t7();
	else if (rx_info.phy_ver == PHY_VER_T5M)
		aml_phy_iq_skew_monitor_t5m();
	else if (rx_info.phy_ver == PHY_VER_T3X)
		aml_phy_iq_skew_monitor_t3x();
	else if (rx_info.phy_ver == PHY_VER_TXHD2)
		aml_phy_iq_skew_monitor_txhd2();
}

void aml_eq_eye_monitor(u8 port)
{
	if (rx_info.phy_ver == PHY_VER_T5)
		aml_eq_eye_monitor_t5();
	else if (rx_info.phy_ver >= PHY_VER_T7 && rx_info.phy_ver <= PHY_VER_T5W)
		aml_eq_eye_monitor_t7();
	else if (rx_info.phy_ver == PHY_VER_T5M)
		aml_eq_eye_monitor_t5m();
	else if (rx_info.phy_ver == PHY_VER_T3X)
		aml_eq_eye_monitor_t3x(port);
	else if (rx_info.phy_ver == PHY_VER_TXHD2)
		aml_eq_eye_monitor_txhd2(0);
}

/* resume-enable:true, suspend-enable:false */
void rx_emp_hw_enable(bool enable)
{
	u32 data;

	data = hdmirx_rd_top_common(TOP_EMP_CNTL_1);
	if (enable) {
		if (rx_info.chip_id == CHIP_ID_T3X)
			hdmirx_wr_top_common(TOP_EMP1_CNTL_1, data | _BIT(0));
		hdmirx_wr_top_common(TOP_EMP_CNTL_1, data | _BIT(0));
	} else {
		if (rx_info.chip_id == CHIP_ID_T3X)
			hdmirx_wr_top_common(TOP_EMP1_CNTL_1, data & ~(_BIT(0)));
		hdmirx_wr_top_common(TOP_EMP_CNTL_1, data & ~(_BIT(0)));
	}
}

void rx_emp_to_ddr_init(u8 port)
{
	u32 data32;

	if (rx_info.chip_id < CHIP_ID_TL1)
		return;

	if (rx_info.emp_buff_a.pg_addr) {
		rx_pr("%s\n", __func__);
		/*disable field done and last pkt interrupt*/
		data32 = hdmirx_rd_top(TOP_INTR_MASKN, port);
		if (rx_info.chip_id >= CHIP_ID_T3X) {
			data32 &= ~(1 << 21);
			data32 &= ~(1 << 22);
		} else {
			data32 &= ~(1 << 25);
			data32 &= ~(1 << 26);
		}
		hdmirx_wr_top(TOP_INTR_MASKN, data32, port);

		if (rx_info.emp_buff_a.p_addr_a) {
			/* emp int enable */
			/* config ddr buffer */
			if (rx_info.chip_id <= CHIP_ID_T5D) {
				hdmirx_wr_top_common(TOP_EMP_DDR_START_A,
					rx_info.emp_buff_a.p_addr_a);
				hdmirx_wr_top_common(TOP_EMP_DDR_START_B,
					rx_info.emp_buff_a.p_addr_b);
			} else {
				hdmirx_wr_top_common(TOP_EMP_DDR_START_A,
					rx_info.emp_buff_a.p_addr_a >> 2);
				hdmirx_wr_top_common(TOP_EMP_DDR_START_B,
					rx_info.emp_buff_a.p_addr_b >> 2);
			}
		}
		/* enable store EMP pkt type */
		data32 = 0;
		if (disable_hdr)
			data32 |= 0 << 22; /* ddr_store_drm */
		else
			data32 |= 1 << 22;/* ddr_store_drm */
		/* ddr_store_aif */
		//if (rx_info.chip_id == CHIP_ID_T7)
			//data32 |= 1 << 19;
		//else
			//data32 |= 0 << 19;
		data32 |= 0 << 19;//aif
		data32 |= 0 << 18;/* ddr_store_spd */
		data32 |= 1 << 16;/* ddr_store_vsi */
		data32 |= 1 << 15;/* ddr_store_emp */
		data32 |= 0 << 12;/* ddr_store_amp */
		data32 |= 0 << 8;/* ddr_store_hbr */
		data32 |= 0 << 1;/* ddr_store_auds */
		hdmirx_wr_top_common(TOP_EMP_DDR_FILTER, data32);
		/* max pkt count */
		hdmirx_wr_top_common(TOP_EMP_CNTMAX, EMP_BUFF_MAX_PKT_CNT);

		data32 = 0;
		data32 |= 0xf << 16;/*[23:16] hs_beat_rate=0xf */
		data32 |= 0x0 << 14;/*[14] buffer_info_mode=0 */
		data32 |= 0x1 << 13;/*[13] reset_on_de=1 */
		data32 |= 0x1 << 12;/*[12] burst_end_on_last_emp=1 */
		data32 |= 0x3ff << 2;/*[11:2] de_rise_delay=0 */
		data32 |= 0x0 << 0;/*[1:0] Endian = 0 */
		hdmirx_wr_top_common(TOP_EMP_CNTL_0, data32);

		data32 = 0;
		data32 |= 0 << 1; /*ddr_mode[1] 0: emp 1: tmds*/
		data32 |= 1 << 0; /*ddr_en[0] 1:enable*/
		hdmirx_wr_top_common(TOP_EMP_CNTL_1, data32);
		/* emp int enable TOP_INTR_MASKN*/
		/* emp field end done at DE rist bit[25]*/
		/* emp last EMP pkt recv done bit[26]*/
		/* disable emp irq */
		//top_intr_maskn_value |= _BIT(25);
		//hdmirx_wr_top(TOP_INTR_MASKN, top_intr_maskn_value);
	}

	rx_info.emp_buff_a.ready = NULL;
	rx_info.emp_buff_a.irq_cnt = 0;
	rx_info.emp_buff_a.emp_pkt_cnt = 0;
	rx_info.emp_buff_a.tmds_pkt_cnt = 720 * 480;
}

void rx_emp1_to_ddr_init(u8 port)
{
	u32 data32;

	if (rx_info.chip_id < CHIP_ID_TL1)
		return;

	if (rx_info.emp_buff_b.pg_addr) {
		/*disable field done and last pkt interrupt*/
		data32 = hdmirx_rd_top(TOP_INTR_MASKN, port);
		if (rx_info.chip_id >= CHIP_ID_T3X) {
			data32 &= ~(1 << 21);
			data32 &= ~(1 << 22);
		} else {
			data32 &= ~(1 << 25);
			data32 &= ~(1 << 26);
		}
		hdmirx_wr_top(TOP_INTR_MASKN, data32, port);

		if (rx_info.emp_buff_b.p_addr_a) {
			/* emp int enable */
			/* config ddr buffer */
			hdmirx_wr_top_common(TOP_EMP1_DDR_START_A,
				rx_info.emp_buff_b.p_addr_a >> 2);
			hdmirx_wr_top_common(TOP_EMP1_DDR_START_B,
				rx_info.emp_buff_b.p_addr_b >> 2);
		}
		/* enable store EMP pkt type */
		data32 = 0;
		if (disable_hdr)
			data32 |= 0 << 22; /* ddr_store_drm */
		else
			data32 |= 1 << 22;/* ddr_store_drm */
		/* ddr_store_aif */
		if (rx_info.chip_id == CHIP_ID_T7)
			data32 |= 1 << 19;
		else
			data32 |= 0 << 19;
		data32 |= 0 << 18;/* ddr_store_spd */
		data32 |= 1 << 16;/* ddr_store_vsi */
		data32 |= 1 << 15;/* ddr_store_emp */
		data32 |= 0 << 12;/* ddr_store_amp */
		data32 |= 0 << 8;/* ddr_store_hbr */
		data32 |= 0 << 1;/* ddr_store_auds */
		hdmirx_wr_top_common(TOP_EMP1_DDR_FILTER, data32);
		/* max pkt count */
		hdmirx_wr_top_common(TOP_EMP1_CNTMAX, EMP_BUFF_MAX_PKT_CNT);

		data32 = 0;
		data32 |= 0xf << 16;/*[23:16] hs_beat_rate=0xf */
		data32 |= 0x0 << 14;/*[14] buffer_info_mode=0 */
		data32 |= 0x1 << 13;/*[13] reset_on_de=1 */
		data32 |= 0x1 << 12;/*[12] burst_end_on_last_emp=1 */
		data32 |= 0x3ff << 2;/*[11:2] de_rise_delay=0 */
		data32 |= 0x0 << 0;/*[1:0] Endian = 0 */
		hdmirx_wr_top_common(TOP_EMP1_CNTL_0, data32);

		data32 = 0;
		data32 |= 0 << 1; /*ddr_mode[1] 0: emp 1: tmds*/
		data32 |= 1 << 0; /*ddr_en[0] 1:enable*/
		hdmirx_wr_top_common(TOP_EMP1_CNTL_1, data32);
		/* emp int enable TOP_INTR_MASKN*/
		/* emp field end done at DE rist bit[25]*/
		/* emp last EMP pkt recv done bit[26]*/
		/* disable emp irq */
		//top_intr_maskn_value |= _BIT(25);
		//hdmirx_wr_top(TOP_INTR_MASKN, top_intr_maskn_value);
	}

	rx_info.emp_buff_b.ready = NULL;
	rx_info.emp_buff_b.irq_cnt = 0;
	rx_info.emp_buff_b.emp_pkt_cnt = 0;
	rx_info.emp_buff_b.tmds_pkt_cnt = 720 * 480;
}

void rx_emp_field_done_irq(u8 port)
{
	phys_addr_t p_addr;
	u32 recv_pkt_cnt, recv_byte_cnt, recv_pagenum;
	u32 emp_pkt_cnt = 0;
	unsigned char *src_addr;
	unsigned char *dst_addr;
	u32 i, j, k;
	u32 data_cnt = 0;
	struct page *cur_start_pg_addr;
	struct emp_info_s *emp_buf_p = NULL;

	if (rx[port].emp_vid_idx == 0) {
		/*emp data start physical address*/
		p_addr = rx_info.chip_id <= CHIP_ID_T5D ?
			(u64)hdmirx_rd_top_common(TOP_EMP_DDR_PTR_S_BUF) :
			(u64)hdmirx_rd_top_common(TOP_EMP_DDR_PTR_S_BUF) << 2;
		/*buffer number*/
		recv_pkt_cnt = hdmirx_rd_top_common(TOP_EMP_RCV_CNT_BUF);
		emp_buf_p = &rx_info.emp_buff_a;
	} else if (rx[port].emp_vid_idx == 1) {
		p_addr = (u64)hdmirx_rd_top_common(TOP_EMP1_DDR_PTR_S_BUF) << 2;
		recv_pkt_cnt = hdmirx_rd_top_common(TOP_EMP1_RCV_CNT_BUF);
		emp_buf_p = &rx_info.emp_buff_b;
	}
	if (!emp_buf_p) {
		rx_pr("emp buff NULL\n");
		return;
	}
	recv_byte_cnt = recv_pkt_cnt * 32;
	if (recv_byte_cnt > (EMP_BUFFER_SIZE >> 2))
		recv_byte_cnt = EMP_BUFFER_SIZE >> 2;
	//if (log_level & PACKET_LOG)
		//rx_pr("recv_byte_cnt=0x%x\n", recv_byte_cnt);
	recv_pagenum = (recv_byte_cnt >> PAGE_SHIFT) + 1;

	if (emp_buf_p->irq_cnt & 0x1) {
		dst_addr = emp_buf_p->store_b;
		if (!dst_addr)
			return;
	} else {
		dst_addr = emp_buf_p->store_a;
		if (!dst_addr)
			return;
	}

	if (recv_pkt_cnt >= EMP_BUFF_MAX_PKT_CNT) {
		recv_pkt_cnt = EMP_BUFF_MAX_PKT_CNT - 1;
		if (log_level & PACKET_LOG)
			rx_pr("pkt cnt err:%d\n", recv_pkt_cnt);
	}
	if (!rx[port].emp_pkt_rev)
		rx[port].emp_pkt_rev = true;
	for (i = 0; i < recv_pagenum;) {
		/*one page 4k*/
		cur_start_pg_addr = phys_to_page(p_addr + i * PAGE_SIZE);
		if (p_addr == emp_buf_p->p_addr_a)
			src_addr = kmap_atomic(cur_start_pg_addr);
		else
			src_addr = kmap_atomic(cur_start_pg_addr) + (emp_buf_p->p_addr_b -
				emp_buf_p->p_addr_a) % PAGE_SIZE;
		if (!src_addr)
			return;
		dma_sync_single_for_cpu(hdmirx_dev, (p_addr + i * PAGE_SIZE),
					PAGE_SIZE, DMA_TO_DEVICE);
		for (j = 0; j < recv_byte_cnt;) {
			//if (src_addr[j] == 0x7f) {
			emp_pkt_cnt++;
			/*32 bytes per emp pkt*/
			for (k = 0; k < 32; k++) {
				dst_addr[data_cnt] = src_addr[j + k];
				data_cnt++;
			}
			//}
			j += 32;
		}
		/*release*/
		/*__kunmap_atomic(src_addr);*/
		kunmap_atomic(src_addr);
		i++;
	}
	if (emp_pkt_cnt * 32 > 1024) {
		if (log_level & 0x400)
			rx_pr("emp buffer overflow!!\n");
	} else {
		/*ready address*/
		emp_buf_p->ready = dst_addr;
		/*ready pkt cnt*/
		emp_buf_p->emp_pkt_cnt = emp_pkt_cnt;
		for (i = 0; i < emp_buf_p->emp_pkt_cnt; i++)
			memcpy((char *)(emp_buf[rx[port].emp_vid_idx] + 31 * i),
				   (char *)(dst_addr + 32 * i), 31);
		/*emp field dont irq counter*/
		emp_buf_p->irq_cnt++;
	}
}

void rx_emp_status(u8 port)
{
	struct emp_info_s *emp_info_p = rx_get_emp_info(port);

	if (!emp_info_p) {
		rx_pr("%s emp info NULL\n", __func__);
		return;
	}
	rx_pr("p_addr_a=0x%p\n", (void *)emp_info_p->p_addr_a);
	rx_pr("p_addr_b=0x%p\n", (void *)emp_info_p->p_addr_b);
	rx_pr("store_a=0x%p\n", emp_info_p->store_a);
	rx_pr("store_b=0x%p\n", emp_info_p->store_b);
	rx_pr("irq cnt =0x%x\n", (u32)emp_info_p->irq_cnt);
	rx_pr("ready=0x%p\n", emp_info_p->ready);
	rx_pr("dump_mode =0x%x\n", emp_info_p->dump_mode);
	rx_pr("recv emp pkt cnt=0x%x\n", emp_info_p->emp_pkt_cnt);
	rx_pr("recv tmds pkt cnt=0x%x\n", emp_info_p->tmds_pkt_cnt);
}

void rx_tmds_to_ddr_init(u8 port)
{
	u32 data, data2;
	u32 i = 0;

	if (rx_info.chip_id < CHIP_ID_T7 || !rx[port].emp_info)
		return;

	if (rx[port].emp_info->pg_addr) {
		rx_pr("%s\n", __func__);
		/*disable field done and last pkt interrupt*/
		if (rx_info.chip_id == CHIP_ID_T3X) {
			top_intr_maskn_value &= ~(1 << 21);
			top_intr_maskn_value &= ~(1 << 22);
		} else {
			top_intr_maskn_value &= ~(1 << 25);
			top_intr_maskn_value &= ~(1 << 26);
		}
		hdmirx_wr_top(TOP_INTR_MASKN, top_intr_maskn_value, port);

		/* disable emp rev */
		data = hdmirx_rd_top_common(TOP_EMP_CNTL_1);
		data &= ~0x1;
		hdmirx_wr_top_common(TOP_EMP_CNTL_1, data);
		/* wait until emp finish */
		data2 = hdmirx_rd_top_common(TOP_EMP_STAT_0) & 0x7fffffff;
		data = hdmirx_rd_top_common(TOP_EMP_STAT_1);
		while (data2 || data) {
			mdelay(1);
			data2 = hdmirx_rd_top_common(TOP_EMP_STAT_0) & 0x7fffffff;
			data = hdmirx_rd_top_common(TOP_EMP_STAT_1);
			if (i++ > 100) {
				rx_pr("warning: wait emp timeout\n");
				break;
			}
		}
		if (rx[port].emp_info->p_addr_a) {
			/* config ddr buffer */
			hdmirx_wr_top_common(TOP_EMP_DDR_START_A,
			rx[port].emp_info->p_addr_a >> 2);
			hdmirx_wr_top_common(TOP_EMP_DDR_START_B,
				rx[port].emp_info->p_addr_a >> 2);
			rx_pr("cfg hw addr=0x%p\n",
				(void *)rx[port].emp_info->p_addr_a);
		}

		/* max pkt count to avoid buffer overflow */
		/* one pixel 4bytes */
		data = ((rx[port].emp_info->tmds_pkt_cnt / 8) * 8) - 1;
		hdmirx_wr_top_common(TOP_EMP_CNTMAX, data);
		rx_pr("pkt max cnt limit=0x%x\n", data);

		data = 0;
		data |= 0x0 << 16;/*[23:16] hs_beat_rate=0xf */
		data |= 0x1 << 14;/*[14] buffer_info_mode=0 */
		data |= 0x1 << 13;/*[13] reset_on_de=1 */
		data |= 0x0 << 12;/*[12] burst_end_on_last_emp=1 */
		data |= 0x0 << 2;/*[11:2] de_rise_delay=0 */
		data |= 0x0 << 0;/*[1:0] Endian = 0 */
		hdmirx_wr_top_common(TOP_EMP_CNTL_0, data);

		/* working mode: tmds data to ddr enable */
		data = hdmirx_rd_top(TOP_EMP_CNTL_1, port);
		data |= 0x1 << 1;/*ddr_mode[1] 0: emp 1: tmds*/
		hdmirx_wr_top_common(TOP_EMP_CNTL_1, data);

		/* emp int enable TOP_INTR_MASKN*/
		/* emp field end done at DE rist bit[25]*/
		/* emp last EMP pkt recv done bit[26]*/
		top_intr_maskn_value |= _BIT(26);
		hdmirx_wr_top_common(TOP_INTR_MASKN, top_intr_maskn_value);

		/*start record*/
		data |= 0x1;	/*ddr_en[0] 1:enable*/
		hdmirx_wr_top_common(TOP_EMP_CNTL_1, data);
	}
}

void rx_emp_lastpkt_done_irq(u8 port)
{
	u32 data;

	/* disable record */
	data = hdmirx_rd_top_common(TOP_EMP_CNTL_1);//to do
	data &= ~0x1;	/*ddr_en[0] 1:enable*/
	hdmirx_wr_top_common(TOP_EMP_CNTL_1, data);//to do

	/*need capture data*/

	rx_pr(">> lastpkt_done_irq >\n");
}

void rx_emp_capture_stop(u8 port)
{
	u32 i = 0, data, data2;

	/*disable field done and last pkt interrupt*/
	if (rx_info.chip_id == CHIP_ID_T3X) {
		top_intr_maskn_value &= ~(1 << 21);
		top_intr_maskn_value &= ~(1 << 22);
	} else {
		top_intr_maskn_value &= ~(1 << 25);
		top_intr_maskn_value &= ~(1 << 26);
	}
	hdmirx_wr_top(TOP_INTR_MASKN, top_intr_maskn_value, port);

	/* disable emp rev */
	data = hdmirx_rd_top_common(TOP_EMP_CNTL_1);//to do
	data &= ~0x1;
	hdmirx_wr_top_common(TOP_EMP_CNTL_1, data);//to do
	/* wait until emp finish */
	data2 = hdmirx_rd_top_common(TOP_EMP_STAT_0) & 0x7fffffff;
	data = hdmirx_rd_top_common(TOP_EMP_STAT_1);
	while (data2 || data) {
		mdelay(1);
		data2 = hdmirx_rd_top_common(TOP_EMP_STAT_0) & 0x7fffffff;
		data = hdmirx_rd_top_common(TOP_EMP_STAT_1);
		if (i++ > 100) {
			rx_pr("warning: wait emp timeout\n");
			break;
		}
	}
	rx_pr("emp capture stop\n");
}

/*
 * get hdmi data error counter
 * for tl1
 * return:
 * ch0 , ch1 , ch2 error counter value
 */
void rx_get_error_cnt(u32 *ch0, u32 *ch1, u32 *ch2, u32 *ch3, u8 port)
{
	u32 val;

	if (rx_info.chip_id == CHIP_ID_T7) {
		/* t7 top 0x41/0x42 can not shadow IP's periodical error counter */
		/* use cor register to get err cnt,t3 fix it */
		hdmirx_wr_bits_cor(DPLL_CTRL0_DPLL_IVCRX, MSK(3, 0), 0x0, port);
		*ch0 = hdmirx_rd_cor(SCDCS_CED0_L_SCDC_IVCRX, port) |
			(((u32)hdmirx_rd_cor(SCDCS_CED0_H_SCDC_IVCRX, port) & 0x7f) << 8);
		*ch1 = hdmirx_rd_cor(SCDCS_CED1_L_SCDC_IVCRX, port) |
			(((u32)hdmirx_rd_cor(SCDCS_CED1_H_SCDC_IVCRX, port) & 0x7f) << 8);
		*ch2 = hdmirx_rd_cor(SCDCS_CED2_L_SCDC_IVCRX, port) |
			(((u32)hdmirx_rd_cor(SCDCS_CED2_H_SCDC_IVCRX, port) & 0x7f) << 8);
		*ch3 = 0;
		udelay(1);
		hdmirx_wr_bits_cor(DPLL_CTRL0_DPLL_IVCRX, MSK(3, 0), 0x7, port);
	} else {
		if (rx[port].var.frl_rate) {
			val = hdmirx_rd_top(TOP_LANE01_ERRCNT, port);
			*ch0 = val & 0x7fff;
			*ch1 = (val >> 16) & 0x7fff;
			val = hdmirx_rd_top(TOP_LANE23_ERRCNT, port);
			*ch2 = val & 0x7fff;
			*ch3 = (val >> 16) & 0x7fff;
		} else {
			val = hdmirx_rd_top(TOP_CHAN01_ERRCNT, port);
			*ch0 = val & 0xffff;
			*ch1 = (val >> 16) & 0xffff;
			val = hdmirx_rd_top(TOP_CHAN2_ERRCNT, port);
			*ch2 = val & 0xffff;
			*ch3 = 0;
		}
	}
}

/*
 * get hdmi audio N CTS
 * for tl1
 * return:
 * audio ACR N
 * audio ACR CTS
 */
void rx_get_audio_N_CTS(u32 *N, u32 *CTS, u8 port)
{
	*N = hdmirx_rd_top_common(TOP_ACR_N_STAT);//
	*CTS = hdmirx_rd_top_common(TOP_ACR_CTS_STAT);//
	//to do
}

u8 rx_get_avmute_sts(u8 port)
{
	u8 ret = 0;

	if (rx_info.chip_id >= CHIP_ID_T7) {
		if (hdmirx_rd_cor(RX_GCP_DBYTE0_DP3_IVCRX, port) & 1)
			ret = 1;
	} else {
		if (hdmirx_rd_dwc(DWC_PDEC_GCP_AVMUTE) & 0x02)
			ret = 1;
	}
	return ret;
}
/* termination calibration */
void rx_phy_rt_cal(void)
{
	int i = 0, j = 0;
	u32 x_val[100][2];
	u32 temp;
	int val_cnt = 1;

	for (; i < 100; i++) {
		wr_reg_hhi_bits(HHI_HDMIRX_PHY_MISC_CNTL0, MISCI_COMMON_RST, 0);
		wr_reg_hhi_bits(HHI_HDMIRX_PHY_MISC_CNTL0, MISCI_COMMON_RST, 1);
		udelay(1);
		temp = (rd_reg_hhi(HHI_HDMIRX_PHY_MISC_STAT) >> 1) & 0x3ff;
		if (i == 0) {
			x_val[0][0] = temp;
			x_val[0][1] = 1;
		}

		for (; j < i; j++) {
			if (temp == x_val[j][0]) {
				x_val[j][1]	+= 1;
				goto todo;
			}
		}
todo:
		if (j == (val_cnt + 1)) {
			x_val[j][0] = temp;
			x_val[j][1] = 1;
			val_cnt++;
			rx_pr("new\n");
		}

		if (x_val[j][1] == 10) {
			term_cal_val = (~((x_val[j][0]) << 1)) & 0x3ff;
			rx_pr("tdr cal val=0x%x", term_cal_val);
			return;
		}
		j = 0;
	}
}

/*
 * for Nvidia PC long detection time issue
 */
void rx_i2c_err_monitor(u8 port)
{
	int data32 = 0;

	if (!(rx[port].ddc_filter_en || is_ddc_filter_en(port)))
		return;

	i2c_err_cnt[port]++;
	data32 = hdmirx_rd_top_common(TOP_EDID_GEN_CNTL);
	if ((i2c_err_cnt[port] % 3) != 1)
		data32 = ((data32 & (~0xff)) | 0x9);
	else
		data32 = ((data32 & (~0xff)) | 0x4f);
	hdmirx_wr_top_common(TOP_EDID_GEN_CNTL,  data32);
	if (log_level & EDID_LOG)
		rx_pr("data32: 0x%x,\n", data32);
}

bool is_ddc_filter_en(u8 port)
{
	bool ret = false;
	int data32 = 0;

	data32 = hdmirx_rd_top_common(TOP_EDID_GEN_CNTL);
	if ((data32 & 0xff) > 0x10)
		ret = true;

	return ret;
}

bool rx_need_ddc_monitor(void)
{
	bool ret = true;

	if (ddc_dbg_en)
		ret = false;

	if (rx_info.chip_id > CHIP_ID_T5W || (is_meson_t7_cpu() && is_meson_rev_c()))
		ret = false;

	return ret;
}

/*
 * FUNC: rx_ddc_active_monitor
 * ddc active monitor
 */
void rx_ddc_active_monitor(u8 port)
{
	u32 temp = 0;

	if (!rx_need_ddc_monitor())
		return;

	if (rx[port].state != FSM_WAIT_CLK_STABLE)
		return;

	if (!((1 << port) & EDID_DETECT_PORT))
		return;

	//if (rx[rx_info.main_port].ddc_filter_en)
		//return;

	switch (port) {
	case E_PORT0:
		temp = hdmirx_rd_top(TOP_EDID_GEN_STAT, E_PORT0);
		break;
	case E_PORT1:
		temp = hdmirx_rd_top(TOP_EDID_GEN_STAT_B, E_PORT1);
		break;
	case E_PORT2:
		temp = hdmirx_rd_top(TOP_EDID_GEN_STAT_C, E_PORT2);
		break;
	case E_PORT3:
		temp = hdmirx_rd_top(TOP_EDID_GEN_STAT_D, E_PORT3);
		break;
	default:
		break;
	}

	temp = temp & 0xff;
	/*0x0a, 0x15 for hengyi ops-pc. refer to 88378
	 *0x6 for X86 GTX1050Ti 4GB
	 *0x14 for special spliter. refer to 72949
	 *0x13 for 8268 refer to 73940
	 *fix edid filter setting
	 */
	if (temp < 0x3f &&
		temp != 0x1 &&
		temp != 0x3 &&
		temp != 0x6 &&
		temp != 0x8 &&
		temp != 0xa &&
		temp != 0xc &&
		temp != 0x13 &&
		temp != 0x14 &&
		temp != 0x15 &&
		temp) {
		rx[port].ddc_filter_en = true;
		if (log_level & EDID_LOG)
			rx_pr("port: %d, edid_status: 0x%x,\n", port, temp);
	} else {
		if ((log_level & EDID_LOG) && rx[port].ddc_filter_en)
			rx_pr("port: %d, edid_status: 0x%x,\n", port, temp);
		rx[port].ddc_filter_en = false;
	}
}

void rx_set_color_bar(bool en, u32 lvl, u8 port)
{
	int data32;

	if (rx_info.chip_id == CHIP_ID_T3X) {
		if (en) {
			hdmirx_wr_top_common_1(TOP_VID_CNTL2, 0);
			data32 = 0;
			data32 |= (1 << 2);
			data32 |= (0 << 1);
			data32 |= (0 << 0);
			hdmirx_wr_top_common_1(HDMIRX_TOP_PXL_BIST, data32);

			data32 = 0;
			data32 |= (1 << 1);
			hdmirx_wr_cor(BIST_CTRL_PBIST_IVCRX, data32, port);

			data32 = 0;
			data32 |= (1 << 4);//start bist
			data32 |= (1 << 3);//run continuously
			data32 |= (1 << 2);// stpg set
			data32 |= (0 << 1);//bist reset
			data32 |= (1 << 0);//bist enable
			hdmirx_wr_cor(BIST_CTRL_PBIST_IVCRX, data32, port);

			data32 = 0;
			data32 |= (1 << 5);//stpg use external or pbist timing
			data32 |= (0 << 4);//stpg inv vsync or not inv
			data32 |= (0 << 3);//stpg inv hsync or not inv
			data32 |= (0 << 2);//PGEN out or STPG out
			data32 |= (0 << 1);//inv vsync or not inv
			data32 |= (0 << 0);//inv hsync or not inv
			hdmirx_wr_cor(BIST_CTRL2_PBIST_IVCRX, data32, port);

			data32 = 0;
			data32 |= (lvl << 4);//reg_stpg_sel
			data32 |= (0 << 3);//de follow tmds_de for stpg
			data32 |= (5 << 0);//resample mode pixel repeat
			hdmirx_wr_cor(BIST_VIDEO_MODE_PBIST_IVCRX, data32, port);
		} else {
			data32 = 0;
			data32 |= (0 << 2);
			data32 |= (0 << 1);
			data32 |= (0 << 0);
			hdmirx_wr_top_common_1(HDMIRX_TOP_PXL_BIST, data32);
			data32 = 0;
			data32 |= 0	<< 20;
			data32 |= 0	<< 8;
			data32 |= 0x0a	<< 0;
			hdmirx_wr_top_common(TOP_VID_CNTL2, data32);//to do
			if (port == rx_info.main_port) {
				if (rx[port].cur.vactive >= 2100 || rx[port].cur.hactive >= 3800)
					hdmirx_wr_bits_top_common_1(TOP_VID_CNTL2, _BIT(31), 1);
				else
					hdmirx_wr_bits_top_common_1(TOP_VID_CNTL2, _BIT(31), 0);
			}
		}
	} else {
		data32 = 0;
		data32 |= (en << 3);//reg_px1_bist2vpcout
		data32 |= (0 << 2);//reg_px1_bist2vpcin_en
		data32 |= (0 << 1);//reg_px1_bist_in_rpt_px1
		data32 |= (0 << 0);//reg_px1_bist_in_vpc_sel
		hdmirx_wr_cor(PXL_BIST_CTRL_PWD_IVCRX, data32, port);
		data32 = 0;
		data32 |= (1 << 1);//bist reset
		hdmirx_wr_cor(BIST_CTRL_PBIST_IVCRX, data32, port);
		data32 = 0;
		data32 |= (1 << 4);//start bist
		data32 |= (1 << 3);//run continuously
		data32 |= (1 << 2);// stpg set
		data32 |= (0 << 1);//bist reset
		data32 |= (1 << 0);//bist enable
		hdmirx_wr_cor(BIST_CTRL_PBIST_IVCRX, data32, port);
		data32 = 0;
		data32 |= (1 << 5);//stpg use external or pbist timing
		data32 |= (0 << 4);//stpg inv vsync or not inv
		data32 |= (0 << 3);//stpg inv hsync or not inv
		data32 |= (0 << 2);//PGEN out or STPG out
		data32 |= (0 << 1);//inv vsync or not inv
		data32 |= (0 << 0);//inv hsync or not inv
		hdmirx_wr_cor(BIST_CTRL2_PBIST_IVCRX, data32, port);
		data32 = 0;
		data32 |= (lvl << 4);//reg_stpg_sel
		data32 |= (0 << 3);//de follow tmds_de for stpg
		data32 |= (5 << 0);//resample mode pixel repeat
		hdmirx_wr_cor(BIST_VIDEO_MODE_PBIST_IVCRX, data32, port);
	}
}

u32 rx_get_ecc_pkt_cnt(u8 port)
{
	u32 pkt_cnt;

	pkt_cnt = hdmirx_rd_cor(RX_PKT_CNT_DP2_IVCRX, port) |
			(hdmirx_rd_cor(RX_PKT_CNT2_DP2_IVCRX, port) << 8);

	return pkt_cnt;
}

u32 rx_get_ecc_err(u8 port)
{
	u32 err_cnt;

	hdmirx_wr_cor(RX_ECC_CTRL_DP2_IVCRX, 3, port);
	err_cnt = hdmirx_rd_cor(RX_HDCP_ERR_DP2_IVCRX, port) |
			(hdmirx_rd_cor(RX_HDCP_ERR2_DP2_IVCRX, port) << 8);

	return err_cnt;
}

void rx_hdcp_22_sent_reauth(u8 port)
{
	hdmirx_wr_bits_cor(CP2PAX_CTRL_0_HDCP2X_IVCRX, _BIT(7), 0, port);
	hdmirx_wr_bits_cor(CP2PAX_CTRL_0_HDCP2X_IVCRX, _BIT(7), 0, port);
	//hdmirx_wr_cor(RX_ECC_CTRL_DP2_IVCRX, 3, port);
	hdmirx_wr_bits_cor(CP2PAX_CTRL_0_HDCP2X_IVCRX, _BIT(7), 1, port);
}

void rx_hdcp_14_sent_reauth(u8 port)
{
	hdmirx_wr_cor(RX_HDCP_DEBUG_HDCP1X_IVCRX, 0x80, port);
}

void rx_check_ecc_error(u8 port)
{
	u32 ecc_pkt_cnt;

	rx[port].ecc_err = rx_get_ecc_err(port);
	ecc_pkt_cnt = rx_get_ecc_pkt_cnt(port);
	if (log_level & ECC_LOG)
		rx_pr("ecc:%d-%d\n",
			  rx[port].ecc_err,
			  ecc_pkt_cnt);
	if (rx[port].ecc_err && ecc_pkt_cnt) {
		rx[port].ecc_err_frames_cnt++;
		if (rx[port].ecc_err_frames_cnt % 20 == 0)
			rx_pr("ecc:%d\n", rx[port].ecc_err);
		if (rx[port].ecc_err == ecc_pkt_cnt)
			skip_frame(2, port, "ecc err");
	} else {
		rx[port].ecc_err_frames_cnt = 0;
	}
}

int is_rx_hdcp14key_loaded_t7(void)
{
	return rx_smc_cmd_handler(HDCP14_RX_QUERY, 0);
}

int is_rx_hdcp22key_loaded_t7(void)
{
	return rx_smc_cmd_handler(HDCP22_RX_QUERY, 0);
}

int is_rx_hdcp14key_crc_pass(void)
{
	return rx_smc_cmd_handler(HDCP14_CRC_STS, 0);
}

int is_rx_hdcp22key_crc0_pass(void)
{
	return rx_smc_cmd_handler(HDCP22_CRC0_STS, 0);
}

int is_rx_hdcp22key_crc1_pass(void)
{
	return rx_smc_cmd_handler(HDCP22_CRC1_STS, 0);
}

void rx_hdcp_crc_check(void)
{
	rx_smc_cmd_handler(HDCP_CRC_CHK, 0);
}

void reset_pcs(u8 port)
{
	hdmirx_wr_top(TOP_SW_RESET, 0x2080, port);
	hdmirx_wr_top(TOP_SW_RESET, 0, port);
}

int aml_phy_get_def_trim_value(void)
{
	// t3x to do
	if (rx_info.chip_id >= CHIP_ID_TL1 &&
		rx_info.chip_id <= CHIP_ID_TM2)
		return rd_reg_hhi(HHI_HDMIRX_PHY_MISC_CNTL1);
	else if (rx_info.chip_id >= CHIP_ID_T5 &&
		rx_info.chip_id <= CHIP_ID_T5D)
		return hdmirx_rd_amlphy(T5_HHI_RX_PHY_MISC_CNTL1);
	else if (rx_info.chip_id >= CHIP_ID_T7 &&
		rx_info.chip_id <= CHIP_ID_T5W)
		return hdmirx_rd_amlphy(T7_HHI_RX_PHY_MISC_CNTL1);
	else if (rx_info.chip_id == CHIP_ID_T5M)
		return hdmirx_rd_amlphy(T5M_HDMIRX20PHY_DCHA_MISC1);
	else if (rx_info.chip_id == CHIP_ID_TXHD2)
		return hdmirx_rd_amlphy(TXHD2_HDMIRX20PHY_DCHA_MISC1);
	else
		return 0;
}

