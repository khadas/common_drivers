// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/init.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/compat.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/clk.h>
#include <linux/of_device.h>
#include <linux/of.h>
#include <linux/reset.h>
#include <linux/amlogic/media/vout/vout_notify.h>
#include "lcd_smart_temp.h"
#include "lcd_reg_temp.h"

#define LCD_CDEV_NAME  "lcd"

struct lcd_cdev_s {
	dev_t           devno;
	struct class    *class;
};

static struct lcd_cdev_s *lcd_cdev;

struct aml_lcd_drv_s lcd_driver = {
	.index = 0,
	.status = 0,
	.lcd_pxp = 0,

	.config = {
		.lcd_type = LCD_TTL,
		.lcd_bits = 8,
		.h_active = 1920,
		.v_active = 1080,
		.h_period = 2200,
		.v_period = 1125,

		.lcd_clk = 148500000,
		.frame_rate = 60,
	},
};

static void lcd_clk_gate_switch(int flag)
{
	//
}

static void lcd_vout_main_clk(void)
{
	//main clk: 333M
	lcd_clk_setb(CLKCTRL_VOUTENC_CLK_CTRL, 0, 8, 1);
	lcd_clk_setb(CLKCTRL_VOUTENC_CLK_CTRL, 1, 25, 3);
	lcd_clk_setb(CLKCTRL_VOUTENC_CLK_CTRL, 1, 16, 7);

	lcd_clk_setb(CLKCTRL_VOUTENC_CLK_CTRL, 1, 8, 1);
}

static void lcd_smart_set_gp1_pll(struct aml_lcd_drv_s *pdrv)
{
	//none for pxp
}

static void lcd_smart_set_clk(struct aml_lcd_drv_s *pdrv)
{
	unsigned int sel, div;

	sel = pdrv->clk_config.clk_sel;
	div = pdrv->clk_config.clk_div;

	lcd_clk_setb(CLKCTRL_VOUTENC_CLK_CTRL, 0, 24, 1);
	lcd_clk_setb(CLKCTRL_VOUTENC_CLK_CTRL, sel, 25, 3);
	lcd_clk_setb(CLKCTRL_VOUTENC_CLK_CTRL, div, 16, 7);

	lcd_clk_setb(CLKCTRL_VOUTENC_CLK_CTRL, 1, 24, 1);
}

static void lcd_smart_disable_clk(struct aml_lcd_drv_s *pdrv)
{
	lcd_clk_setb(CLKCTRL_VOUTENC_CLK_CTRL, 0, 24, 1);
}

static unsigned int lcd_dth_lut[16] = {
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x82412814, 0x48122481, 0x18242184, 0x18242841,
	0x9653ca56, 0x6a3ca635, 0x6ca9c35a, 0xca935635,
	0x7dbedeb7, 0xde7dbed7, 0xeb7dbed7, 0xdbe77edb
};

static void lcd_smart_set_vout(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_config_s *pconf = &pdrv->config;
	unsigned int hsize, vsize;
	unsigned int reoder, timgen_mode, serial_rate, field_mode;
	unsigned int total_hsize, total_vsize;
	unsigned int hs_pix_bgn, hs_pix_end, vs_lne_bgn_e, vs_lne_end_e;
	unsigned int vs_pix_bgn_e, vs_pix_end_e, de_lne_bgn_e, de_lne_end_e;
	unsigned int de_px_bgn_e, de_px_end_e, bot_bgn_lne, top_bgn_lne;
	unsigned int vs_lne_bgn_o, vs_lne_end_o, vs_pix_bgn_o, vs_pix_end_o;
	unsigned int de_lne_bgn_o, de_lne_end_o;
	int i;

	hsize = pconf->h_active;
	vsize = pconf->v_active;
	reoder = 36;
	//1<<0:MIPI_TX, 1<<1:LCDs8,  1<<2:BT1120,  1<<3:BT656, 1<<4:lcds6,
	//1<<5:lcdp6, 1<<6:lcdp8, 1<<7:lcd565, 1<<8:lcds9(6+3,3+6), 1<<9:lcds8(5+3,3+5)
	timgen_mode = (1 << 6);
	//0:pix/1cylce    1:pix/2cycle  2:pix/3cycle
	serial_rate = 0;
	//0:progress 1:interlace
	field_mode = pconf->field_mode;

	total_hsize  = pconf->h_period;
	total_vsize  = pconf->v_period;

	hs_pix_bgn   = pconf->hs_hstart;
	hs_pix_end   = pconf->hs_hend;
	vs_lne_bgn_e = pconf->vs_vstart;
	vs_lne_end_e = pconf->vs_vend;
	vs_pix_bgn_e = pconf->vs_hstart; //0;
	vs_pix_end_e = pconf->vs_hend; //9;

	de_lne_bgn_e = pconf->vstart;
	de_lne_end_e = pconf->vend;
	de_px_bgn_e  = pconf->hstart;
	de_px_end_e  = pconf->hend;

	bot_bgn_lne  =    0;
	top_bgn_lne  =    0;
	vs_lne_bgn_o =    0;
	vs_lne_end_o =    0;
	vs_pix_bgn_o =    0;
	vs_pix_end_o =    0;
	de_lne_bgn_o =    0;
	de_lne_end_o =    0;

	lcd_vcbus_setb(VPU_VOUT_CORE_CTRL, 0, 0, 1); //disable venc_en
	lcd_vcbus_setb(VPU_VOUT_DETH_CTRL, 0, 5, 1); //10bit to 9bit
	lcd_vcbus_setb(VPU_VOUT_DETH_CTRL, hsize, 6, 13);
	lcd_vcbus_setb(VPU_VOUT_DETH_CTRL, vsize, 19, 13);
	lcd_vcbus_setb(VPU_VOUT_INT_CTRL, 1, 14, 1); //dth_en

	if (reoder != 36)
		lcd_vcbus_setb(VPU_VOUT_CORE_CTRL, reoder, 4, 6);
	lcd_vcbus_setb(VPU_VOUT_CORE_CTRL, timgen_mode, 16, 10);
	lcd_vcbus_setb(VPU_VOUT_CORE_CTRL, serial_rate, 2, 2);

	//if (bt1120 || bt656) {
	//	lcd_vcbus_setb(VPU_VOUT_BT_CTRL, yc_switch, 0, 1);
	//	//0:cb first   1:cr first
	//	lcd_vcbus_setb(VPU_VOUT_BT_CTRL, cr_fst, 1, 1);
	//	//0:left, 1:right, 2:average
	//	lcd_vcbus_setb(VPU_VOUT_BT_CTRL, mode_422, 2, 2);
	//}

	lcd_vcbus_write(VPU_VOUT_DTH_ADDR, 0);
	for (i = 0; i < 32; i++)
		lcd_vcbus_write(VPU_VOUT_DTH_DATA, lcd_dth_lut[i % 16]);

	lcd_vcbus_setb(VPU_VOUT_CORE_CTRL,    field_mode,  1, 1);
	lcd_vcbus_setb(VPU_VOUT_MAX_SIZE,     total_hsize, 16, 13);
	lcd_vcbus_setb(VPU_VOUT_MAX_SIZE,     total_vsize,  0, 13);
	lcd_vcbus_setb(VPU_VOUT_FLD_BGN_LINE, bot_bgn_lne, 16, 13);
	lcd_vcbus_setb(VPU_VOUT_FLD_BGN_LINE, top_bgn_lne,  0, 13);

	lcd_vcbus_setb(VPU_VOUT_HS_POS,      hs_pix_bgn, 16, 13);
	lcd_vcbus_setb(VPU_VOUT_HS_POS,      hs_pix_end,  0, 13);
	lcd_vcbus_setb(VPU_VOUT_VSLN_E_POS,  vs_lne_bgn_e, 16, 13);
	lcd_vcbus_setb(VPU_VOUT_VSLN_E_POS,  vs_lne_end_e,  0, 13);
	lcd_vcbus_setb(VPU_VOUT_VSPX_E_POS,  vs_pix_bgn_e, 16, 13);
	lcd_vcbus_setb(VPU_VOUT_VSPX_E_POS,  vs_pix_end_e,  0, 13);
	lcd_vcbus_setb(VPU_VOUT_VSLN_O_POS,  vs_lne_bgn_o, 16, 13);
	lcd_vcbus_setb(VPU_VOUT_VSLN_O_POS,  vs_lne_end_o,  0, 13);
	lcd_vcbus_setb(VPU_VOUT_VSPX_O_POS,  vs_pix_bgn_o, 16, 13);
	lcd_vcbus_setb(VPU_VOUT_VSPX_O_POS,  vs_pix_end_o,  0, 13);

	lcd_vcbus_setb(VPU_VOUT_DELN_E_POS,  de_lne_bgn_e, 16, 13);
	lcd_vcbus_setb(VPU_VOUT_DELN_E_POS,  de_lne_end_e,  0, 13);
	lcd_vcbus_setb(VPU_VOUT_DELN_O_POS,  de_lne_bgn_o, 16, 13);
	lcd_vcbus_setb(VPU_VOUT_DELN_O_POS,  de_lne_end_o,  0, 13);
	lcd_vcbus_setb(VPU_VOUT_DE_PX_EN,    de_px_bgn_e, 16, 13);
	lcd_vcbus_setb(VPU_VOUT_DE_PX_EN,    de_px_end_e,  0, 13);

	lcd_vcbus_setb(VPU_VOUT_CORE_CTRL, 1, 0, 1); //venc_en
}

static void lcd_smart_disable_vout(struct aml_lcd_drv_s *pdrv)
{
	//
}

static void lcd_smart_pinmux_set(struct aml_lcd_drv_s *pdrv)
{
	//
}

static void lcd_smart_pinmux_clr(struct aml_lcd_drv_s *pdrv)
{
	//
}

static int lcd_driver_enable(struct aml_lcd_drv_s *pdrv)
{
	LCDPR("lcd driver enable\n");

	lcd_smart_set_gp1_pll(pdrv);
	if (pdrv->lcd_pxp) {
		pdrv->clk_config.clk_sel = 7;
		pdrv->clk_config.clk_div = 1;
	} else {
		pdrv->clk_config.clk_sel = 0;
		pdrv->clk_config.clk_div = 7;
	}
	lcd_smart_set_clk(pdrv);
	lcd_smart_set_vout(pdrv);
	lcd_smart_pinmux_set(pdrv);

	pdrv->status = 1;

	LCDPR("%s: finished\n", __func__);
	return 0;
}

static void lcd_driver_disable(struct aml_lcd_drv_s *pdrv)
{
	lcd_smart_pinmux_clr(pdrv);
	lcd_smart_disable_vout(pdrv);
	lcd_smart_disable_clk(pdrv);

	pdrv->status = 0;

	LCDPR("%s: finished\n", __func__);
}

static int lcd_config_probe(struct aml_lcd_drv_s *pdrv)
{
	const struct device_node *np;
	unsigned int val;
	int ret = 0;

	if (!pdrv->dev->of_node) {
		LCDERR("dev of_node is null\n");
		return -1;
	}
	np = pdrv->dev->of_node;

	ret = of_property_read_u32(np, "pxp", &val);
	if (ret) {
		pdrv->lcd_pxp = 0;
	} else {
		pdrv->lcd_pxp = (unsigned char)val;
		LCDPR("find lcd_pxp: %d\n", pdrv->lcd_pxp);
	}

	snprintf(pdrv->config.propname, 24, "lcd_0");
	lcd_smart_config_load_from_dts(pdrv);
	lcd_smart_timing_init_config(pdrv);

	return 0;
}

/* ************************************************************* */
/* lcd debug                                                     */
/* ************************************************************* */
#define LCD_REG_DBG_VC_BUS          0
#define LCD_REG_DBG_ANA_BUS         1
#define LCD_REG_DBG_CLK_BUS         2
#define LCD_REG_DBG_PERIPHS_BUS     3
#define LCD_REG_DBG_MIPIHOST_BUS    4
#define LCD_REG_DBG_MIPIPHY_BUS     5
#define LCD_REG_DBG_RST_BUS         10
#define LCD_REG_DBG_MAX_BUS         0xff

static void lcd_debug_reg_write(struct aml_lcd_drv_s *pdrv, unsigned int reg,
				unsigned int data, unsigned int bus)
{
	switch (bus) {
	case LCD_REG_DBG_VC_BUS:
		lcd_vcbus_write(reg, data);
		pr_info("write vcbus [0x%04x] = 0x%08x, readback 0x%08x\n",
			reg, data, lcd_vcbus_read(reg));
		break;
	case LCD_REG_DBG_ANA_BUS:
		lcd_ana_write(reg, data);
		pr_info("write ana [0x%04x] = 0x%08x, readback 0x%08x\n",
			reg, data, lcd_ana_read(reg));
		break;
	case LCD_REG_DBG_CLK_BUS:
		lcd_clk_write(reg, data);
		pr_info("write clk [0x%04x] = 0x%08x, readback 0x%08x\n",
			reg, data, lcd_clk_read(reg));
		break;
	case LCD_REG_DBG_PERIPHS_BUS:
		lcd_periphs_write(reg, data);
		pr_info("write periphs [0x%04x] = 0x%08x, readback 0x%08x\n",
			reg, data, lcd_periphs_read(reg));
		break;
	case LCD_REG_DBG_MIPIHOST_BUS:
		dsi_host_write(reg, data);
		pr_info("write mipi_dsi_host [0x%04x] = 0x%08x, readback 0x%08x\n",
			reg, data, dsi_host_read(reg));
		break;
	case LCD_REG_DBG_MIPIPHY_BUS:
		dsi_phy_write(reg, data);
		pr_info("write mipi_dsi_phy [0x%04x] = 0x%08x, readback 0x%08x\n",
			reg, data, dsi_phy_read(reg));
		break;
	case LCD_REG_DBG_RST_BUS:
		lcd_reset_write(reg, data);
		pr_info("write rst [0x%04x] = 0x%08x, readback 0x%08x\n",
			reg, data, lcd_reset_read(reg));
		break;
	default:
		break;
	}
}

static void lcd_debug_reg_read(struct aml_lcd_drv_s *pdrv,
			       unsigned int reg, unsigned int bus)
{
	switch (bus) {
	case LCD_REG_DBG_VC_BUS:
		pr_info("read vcbus [0x%04x] = 0x%08x\n",
			reg, lcd_vcbus_read(reg));
		break;
	case LCD_REG_DBG_ANA_BUS:
		pr_info("read ana [0x%04x] = 0x%08x\n",
			reg, lcd_ana_read(reg));
		break;
	case LCD_REG_DBG_CLK_BUS:
		pr_info("read clk [0x%04x] = 0x%08x\n",
			reg, lcd_clk_read(reg));
		break;
	case LCD_REG_DBG_PERIPHS_BUS:
		pr_info("read periphs [0x%04x] = 0x%08x\n",
			reg, lcd_periphs_read(reg));
		break;
	case LCD_REG_DBG_MIPIHOST_BUS:
		pr_info("read mipi_dsi_host [0x%04x] = 0x%08x\n",
			reg, dsi_host_read(reg));
		break;
	case LCD_REG_DBG_MIPIPHY_BUS:
		pr_info("read mipi_dsi_phy [0x%04x] = 0x%08x\n",
			reg, dsi_phy_read(reg));
		break;
	case LCD_REG_DBG_RST_BUS:
		pr_info("read rst [0x%04x] = 0x%08x\n",
			reg, lcd_reset_read(reg));
		break;
	default:
		break;
	}
}

static void lcd_debug_reg_dump(struct aml_lcd_drv_s *pdrv, unsigned int reg,
			       unsigned int num, unsigned int bus)
{
	int i;

	switch (bus) {
	case LCD_REG_DBG_VC_BUS:
		pr_info("dump vcbus regs:\n");
		for (i = 0; i < num; i++) {
			pr_info("[0x%04x] = 0x%08x\n",
				(reg + i), lcd_vcbus_read(reg + i));
		}
		break;
	case LCD_REG_DBG_ANA_BUS:
		pr_info("dump ana regs:\n");
		for (i = 0; i < num; i++) {
			pr_info("[0x%04x] = 0x%08x\n",
				(reg + i), lcd_ana_read(reg + i));
		}
		break;
	case LCD_REG_DBG_CLK_BUS:
		pr_info("dump clk regs:\n");
		for (i = 0; i < num; i++) {
			pr_info("[0x%04x] = 0x%08x\n",
				(reg + i), lcd_clk_read(reg + i));
		}
		break;
	case LCD_REG_DBG_PERIPHS_BUS:
		pr_info("dump periphs-bus regs:\n");
		for (i = 0; i < num; i++) {
			pr_info("[0x%04x] = 0x%08x\n",
				(reg + i), lcd_periphs_read(reg + i));
		}
		break;
	case LCD_REG_DBG_MIPIHOST_BUS:
		pr_info("dump mipi_dsi_host regs:\n");
		for (i = 0; i < num; i++) {
			pr_info("[0x%04x] = 0x%08x\n",
				(reg + i), dsi_host_read(reg + i));
		}
		break;
	case LCD_REG_DBG_MIPIPHY_BUS:
		pr_info("dump mipi_dsi_phy regs:\n");
		for (i = 0; i < num; i++) {
			pr_info("[0x%04x] = 0x%08x\n",
				(reg + i), dsi_phy_read(reg + i));
		}
		break;
	case LCD_REG_DBG_RST_BUS:
		pr_info("dump rst regs:\n");
		for (i = 0; i < num; i++) {
			pr_info("[0x%04x] = 0x%08x\n",
				(reg + i), lcd_reset_read(reg + i));
		}
		break;
	default:
		break;
	}
}

static ssize_t lcd_debug_reg_store(struct device *dev, struct device_attribute *attr,
				   const char *buf, size_t count)
{
	struct aml_lcd_drv_s *pdrv = dev_get_drvdata(dev);
	unsigned int bus = LCD_REG_DBG_MAX_BUS;
	unsigned int reg32 = 0, data32 = 0;
	int ret = 0;

	switch (buf[0]) {
	case 'w':
		if (buf[1] == 'v') { /* vcbus */
			ret = sscanf(buf, "wv %x %x", &reg32, &data32);
			bus = LCD_REG_DBG_VC_BUS;
		} else if (buf[1] == 'a') { /* ana */
			ret = sscanf(buf, "wa %x %x", &reg32, &data32);
			bus = LCD_REG_DBG_ANA_BUS;
		} else if (buf[1] == 'c') { /* clk */
			ret = sscanf(buf, "wc %x %x", &reg32, &data32);
			bus = LCD_REG_DBG_CLK_BUS;
		} else if (buf[1] == 'p') { /* periphs */
			ret = sscanf(buf, "wp %x %x", &reg32, &data32);
			bus = LCD_REG_DBG_PERIPHS_BUS;
		} else if (buf[1] == 'm') {
			if (buf[2] == 'h') { /* mipi host */
				ret = sscanf(buf, "wmh %x %x", &reg32, &data32);
				bus = LCD_REG_DBG_MIPIHOST_BUS;
			} else if (buf[2] == 'p') { /* mipi phy */
				ret = sscanf(buf, "wmp %x %x", &reg32, &data32);
				bus = LCD_REG_DBG_MIPIPHY_BUS;
			}
		} else if (buf[1] == 'r') { /* rst */
			ret = sscanf(buf, "wr %x %x", &reg32, &data32);
			bus = LCD_REG_DBG_RST_BUS;
		}
		if (ret == 2) {
			lcd_debug_reg_write(pdrv, reg32, data32, bus);
		} else {
			pr_info("invalid data\n");
			return -EINVAL;
		}
		break;
	case 'r':
		if (buf[1] == 'v') { /* vcbus */
			ret = sscanf(buf, "rv %x", &reg32);
			bus = LCD_REG_DBG_VC_BUS;
		} else if (buf[1] == 'a') { /* ana */
			ret = sscanf(buf, "ra %x", &reg32);
			bus = LCD_REG_DBG_ANA_BUS;
		} else if (buf[1] == 'c') { /* clk */
			ret = sscanf(buf, "rc %x", &reg32);
			bus = LCD_REG_DBG_CLK_BUS;
		} else if (buf[1] == 'p') { /* periphs */
			ret = sscanf(buf, "rp %x", &reg32);
			bus = LCD_REG_DBG_PERIPHS_BUS;
		} else if (buf[1] == 'm') {
			if (buf[2] == 'h') { /* mipi host */
				ret = sscanf(buf, "rmh %x", &reg32);
				bus = LCD_REG_DBG_MIPIHOST_BUS;
			} else if (buf[2] == 'p') { /* mipi phy */
				ret = sscanf(buf, "rmp %x", &reg32);
				bus = LCD_REG_DBG_MIPIPHY_BUS;
			}
		} else if (buf[1] == 'r') { /* rst */
			ret = sscanf(buf, "rr %x", &reg32);
			bus = LCD_REG_DBG_RST_BUS;
		}
		if (ret == 1) {
			lcd_debug_reg_read(pdrv, reg32, bus);
		} else {
			pr_info("invalid data\n");
			return -EINVAL;
		}
		break;
	case 'd':
		if (buf[1] == 'v') { /* vcbus */
			ret = sscanf(buf, "dv %x %d", &reg32, &data32);
			bus = LCD_REG_DBG_VC_BUS;
		} else if (buf[1] == 'a') { /* ana */
			ret = sscanf(buf, "da %x %d", &reg32, &data32);
			bus = LCD_REG_DBG_ANA_BUS;
		} else if (buf[1] == 'c') { /* clk */
			ret = sscanf(buf, "dc %x %d", &reg32, &data32);
			bus = LCD_REG_DBG_CLK_BUS;
		} else if (buf[1] == 'p') { /* periphs */
			ret = sscanf(buf, "dp %x %d", &reg32, &data32);
			bus = LCD_REG_DBG_PERIPHS_BUS;
		} else if (buf[1] == 'm') {
			if (buf[2] == 'h') { /* mipi host */
				ret = sscanf(buf, "dmh %x %d", &reg32, &data32);
				bus = LCD_REG_DBG_MIPIHOST_BUS;
			} else if (buf[2] == 'p') { /* mipi phy */
				ret = sscanf(buf, "dmp %x %d", &reg32, &data32);
				bus = LCD_REG_DBG_MIPIPHY_BUS;
			}
		} else if (buf[1] == 'r') { /* rst */
			ret = sscanf(buf, "dr %x %x", &reg32, &data32);
			bus = LCD_REG_DBG_RST_BUS;
		}
		if (ret == 2) {
			lcd_debug_reg_dump(pdrv, reg32, data32, bus);
		} else {
			pr_info("invalid data\n");
			return -EINVAL;
		}
		break;
	default:
		pr_info("wrong command\n");
		break;
	}

	return count;
}

static ssize_t lcd_debug_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct aml_lcd_drv_s *pdrv = dev_get_drvdata(dev);

	return sprintf(buf, "lcd_status: %d\n", pdrv->status);
}

static ssize_t lcd_debug_enable_store(struct device *dev, struct device_attribute *attr,
				      const char *buf, size_t count)
{
	struct aml_lcd_drv_s *pdrv = dev_get_drvdata(dev);
	int ret = 0;
	unsigned int temp = 1;

	ret = kstrtouint(buf, 10, &temp);
	if (ret) {
		LCDERR("invalid data\n");
		return -EINVAL;
	}
	if (temp)
		lcd_driver_enable(pdrv);
	else
		lcd_driver_disable(pdrv);

	return count;
}

static ssize_t lcd_debug_test_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct aml_lcd_drv_s *pdrv = dev_get_drvdata(dev);

	return sprintf(buf, "test pattern: %d\n", pdrv->test_state);
}

static ssize_t lcd_debug_test_store(struct device *dev, struct device_attribute *attr,
				    const char *buf, size_t count)
{
	unsigned int temp = 0;
	int ret = 0;

	ret = kstrtouint(buf, 10, &temp);
	if (ret) {
		pr_info("invalid data\n");
		return -EINVAL;
	}

	LCDPR("please use vpp_blend_dummy data for test pattern\n");

	return count;
}

static struct device_attribute lcd_debug_attrs[] = {
	__ATTR(reg,       0200, NULL, lcd_debug_reg_store),
	__ATTR(enable,    0644, lcd_debug_enable_show, lcd_debug_enable_store),
	__ATTR(test,      0644, lcd_debug_test_show, lcd_debug_test_store)
};

static int lcd_debug_file_creat(struct aml_lcd_drv_s *pdrv)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(lcd_debug_attrs); i++) {
		if (device_create_file(pdrv->dev, &lcd_debug_attrs[i])) {
			LCDERR("create lcd debug attribute %s fail\n",
			       lcd_debug_attrs[i].attr.name);
		}
	}

	return 0;
}

static int lcd_debug_file_remove(struct aml_lcd_drv_s *pdrv)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(lcd_debug_attrs); i++)
		device_remove_file(pdrv->dev, &lcd_debug_attrs[i]);

	return 0;
}

/* ************************************************************* */
/* lcd ioctl                                                     */
/* ************************************************************* */
static int lcd_io_open(struct inode *inode, struct file *file)
{
	struct aml_lcd_drv_s *pdrv;

	pdrv = container_of(inode->i_cdev, struct aml_lcd_drv_s, cdev);
	file->private_data = pdrv;

	return 0;
}

static int lcd_io_release(struct inode *inode, struct file *file)
{
	struct aml_lcd_drv_s *pdrv;

	if (!file->private_data)
		return 0;

	pdrv = (struct aml_lcd_drv_s *)file->private_data;
	file->private_data = NULL;
	return 0;
}

static long lcd_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return 0;
}

#ifdef CONFIG_COMPAT
static long lcd_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	unsigned long ret;

	arg = (unsigned long)compat_ptr(arg);
	ret = lcd_ioctl(file, cmd, arg);
	return ret;
}
#endif

static const struct file_operations lcd_fops = {
	.owner          = THIS_MODULE,
	.open           = lcd_io_open,
	.release        = lcd_io_release,
	.unlocked_ioctl = lcd_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl   = lcd_compat_ioctl,
#endif
};

static int lcd_cdev_add(struct aml_lcd_drv_s *pdrv, struct device *parent)
{
	dev_t devno;
	int ret = 0;

	if (!pdrv) {
		LCDERR("%s: pdrv is null\n", __func__);
		return -1;
	}
	if (!lcd_cdev) {
		ret = 1;
		goto lcd_cdev_add_failed;
	}

	devno = MKDEV(MAJOR(lcd_cdev->devno), 0);

	cdev_init(&pdrv->cdev, &lcd_fops);
	pdrv->cdev.owner = THIS_MODULE;
	ret = cdev_add(&pdrv->cdev, devno, 1);
	if (ret) {
		ret = 2;
		goto lcd_cdev_add_failed;
	}

	pdrv->dev = device_create(lcd_cdev->class, parent, devno, NULL, "lcd0");
	if (IS_ERR_OR_NULL(pdrv->dev)) {
		ret = 3;
		goto lcd_cdev_add_failed1;
	}

	dev_set_drvdata(pdrv->dev, pdrv);
	pdrv->dev->of_node = parent->of_node;

	LCDPR("%s OK\n", __func__);
	return 0;

lcd_cdev_add_failed1:
	cdev_del(&pdrv->cdev);
lcd_cdev_add_failed:
	LCDERR("%s: failed: %d\n", __func__, ret);
	return -1;
}

static void lcd_cdev_remove(struct aml_lcd_drv_s *pdrv)
{
	dev_t devno;

	if (!lcd_cdev || !pdrv)
		return;

	devno = MKDEV(MAJOR(lcd_cdev->devno), pdrv->index);
	device_destroy(lcd_cdev->class, devno);
	cdev_del(&pdrv->cdev);
}

static int lcd_global_init_once(void)
{
	int ret;

	lcd_cdev = kzalloc(sizeof(*lcd_cdev), GFP_KERNEL);
	if (!lcd_cdev)
		return -1;

	ret = alloc_chrdev_region(&lcd_cdev->devno, 0, 2, LCD_CDEV_NAME);
	if (ret) {
		ret = 1;
		goto lcd_cdev_init_once_err;
	}

	lcd_cdev->class = class_create(THIS_MODULE, "aml_lcd");
	if (IS_ERR_OR_NULL(lcd_cdev->class)) {
		ret = 2;
		goto lcd_cdev_init_once_err_1;
	}

	return 0;

lcd_cdev_init_once_err_1:
	unregister_chrdev_region(lcd_cdev->devno, 2);
lcd_cdev_init_once_err:
	kfree(lcd_cdev);
	lcd_cdev = NULL;
	LCDERR("%s: failed: %d\n", __func__, ret);
	return -1;
}

static void lcd_global_remove_once(void)
{
	if (!lcd_cdev)
		return;

	class_destroy(lcd_cdev->class);
	unregister_chrdev_region(lcd_cdev->devno, 2);
	kfree(lcd_cdev);
	lcd_cdev = NULL;
}

/* ************************************************************* */
/* lcd ioctl                                                     */
/* ************************************************************* */
static struct vinfo_s lcd_vinfo = {
	.name              = "panel",
	.mode              = VMODE_LCD,
	.frac              = 0,
	.width             = 1920,
	.height            = 1080,
	.field_height      = 1080,
	.aspect_ratio_num  = 16,
	.aspect_ratio_den  = 9,
	.sync_duration_num = 60,
	.sync_duration_den = 1,
	.std_duration      = 60,
	.video_clk         = 148500000,
	.htotal            = 2200,
	.vtotal            = 1125,
	.hsw               = 44,
	.hbp               = 148,
	.hfp               = 88,
	.vsw               = 5,
	.vbp               = 30,
	.vfp               = 10,
	.fr_adj_type       = VOUT_FR_ADJ_NONE,
	.viu_color_fmt     = COLOR_FMT_RGB444,
	.viu_mux           = VIU_MUX_ENCL,
	.vout_device       = NULL,
};

static struct vinfo_s *lcd_get_current_info(void *data)
{
	return &lcd_vinfo;
}

static int lcd_check_same_vmodeattr(char *mode, void *data)
{
	return 1;
}

static int lcd_vmode_is_supported(enum vmode_e mode, void *data)
{
	mode &= VMODE_MODE_BIT_MASK;

	if (mode == VMODE_LCD)
		return true;
	return false;
}

static enum vmode_e lcd_validate_vmode(char *mode, unsigned int frac, void *data)
{
	if (!mode)
		return VMODE_MAX;
	if (frac)
		return VMODE_MAX;

	if ((strcmp(mode, "panel")) == 0)
		return VMODE_LCD;

	return VMODE_MAX;
}

static int lcd_set_current_vmode(enum vmode_e mode, void *data)
{
	struct aml_lcd_drv_s *pdrv = (struct aml_lcd_drv_s *)data;
	int ret = 0;

	if (!pdrv)
		return -1;

	if (VMODE_LCD != (mode & VMODE_MODE_BIT_MASK))
		return -1;

	if (mode & VMODE_INIT_BIT_MASK)
		lcd_clk_gate_switch(1);
	else
		lcd_driver_enable(pdrv);

	return ret;
}

static int lcd_vout_disable(enum vmode_e cur_vmod, void *data)
{
	struct aml_lcd_drv_s *pdrv = (struct aml_lcd_drv_s *)data;

	if (!pdrv)
		return -1;

	lcd_driver_disable(pdrv);
	LCDPR("%s finished\n", __func__);

	return 0;
}

static int lcd_vout_get_disp_cap(char *buf, void *data)
{
	int ret = 0;

	ret += sprintf(buf + ret, "%s\n", "panel");
	return ret;
}

static void lcd_smart_vout_server_init(struct aml_lcd_drv_s *pdrv)
{
	struct vout_server_s *vserver;

	vserver = kzalloc(sizeof(*vserver), GFP_KERNEL);
	if (!vserver)
		return;
	vserver->name = kzalloc(32, GFP_KERNEL);
	if (!vserver->name) {
		kfree(vserver);
		return;
	}
	pdrv->vout_server = vserver;

	sprintf(vserver->name, "lcd_vout_server");
	vserver->op.get_vinfo = lcd_get_current_info;
	vserver->op.set_vmode = lcd_set_current_vmode;
	vserver->op.validate_vmode = lcd_validate_vmode;
	vserver->op.check_same_vmodeattr = lcd_check_same_vmodeattr;
	vserver->op.vmode_is_supported = lcd_vmode_is_supported;
	vserver->op.disable = lcd_vout_disable;
	vserver->op.set_state = NULL;
	vserver->op.clr_state = NULL;
	vserver->op.get_state = NULL;
	vserver->op.get_disp_cap = lcd_vout_get_disp_cap;
	vserver->op.set_vframe_rate_hint = NULL;
	vserver->op.get_vframe_rate_hint = NULL;
	vserver->op.set_bist = NULL;
	vserver->op.vout_suspend = NULL;
	vserver->op.vout_resume = NULL;
	vserver->data = (void *)pdrv;

	vout_register_server(pdrv->vout_server);
}

static void lcd_smart_vout_server_remove(struct aml_lcd_drv_s *pdrv)
{
	vout_unregister_server(pdrv->vout_server);
}

static const struct of_device_id lcd_dt_match_table[] = {
	{
		.compatible = "amlogic, lcd-c3",
	},
	{}
};

static int lcd_probe(struct platform_device *pdev)
{
	struct aml_lcd_drv_s *pdrv;
	int ret = 0;

	LCDPR("driver probe\n");

	/* set drvdata */
	pdrv = &lcd_driver;
	//pdrv->of_node = pdev->dev.of_node;
	platform_set_drvdata(pdev, pdrv);

	ret = lcd_ioremap(pdev);
	if (ret)
		goto lcd_probe_err_1;

	lcd_vout_main_clk();

	lcd_global_init_once();
	ret = lcd_cdev_add(pdrv, &pdev->dev);
	if (ret)
		goto lcd_probe_err_2;

	lcd_config_probe(pdrv);
	lcd_debug_file_creat(pdrv);
	lcd_smart_vout_server_init(pdrv);

	LCDPR("%s ok\n", __func__);

	lcd_driver_enable(pdrv);

	return 0;

lcd_probe_err_2:
	lcd_cdev_remove(pdrv);
	lcd_global_remove_once();
lcd_probe_err_1:
	/* free drvdata */
	platform_set_drvdata(pdev, NULL);
	LCDPR("%s failed\n", __func__);
	return ret;
}

static int lcd_remove(struct platform_device *pdev)
{
	struct aml_lcd_drv_s *pdrv = platform_get_drvdata(pdev);

	if (!pdrv)
		return 0;

	lcd_smart_vout_server_remove(pdrv);
	lcd_debug_file_remove(pdrv);
	lcd_cdev_remove(pdrv);
	lcd_global_remove_once();
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static int lcd_resume(struct platform_device *pdev)
{
	struct aml_lcd_drv_s *pdrv = platform_get_drvdata(pdev);

	if (!pdrv)
		return 0;

	LCDPR("lcd resume\n");

	lcd_driver_enable(pdrv);

	return 0;
}

static int lcd_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct aml_lcd_drv_s *pdrv = platform_get_drvdata(pdev);

	if (!pdrv)
		return 0;

	LCDPR("lcd suspend\n");

	lcd_driver_disable(pdrv);
	return 0;
}

static void lcd_shutdown(struct platform_device *pdev)
{
	struct aml_lcd_drv_s *pdrv = platform_get_drvdata(pdev);

	if (!pdrv)
		return;

	LCDPR("lcd shutdown\n");

	lcd_driver_disable(pdrv);
}

static struct platform_driver lcd_platform_driver = {
	.probe = lcd_probe,
	.remove = lcd_remove,
	.suspend = lcd_suspend,
	.resume = lcd_resume,
	.shutdown = lcd_shutdown,
	.driver = {
		.name = "mesonlcd",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(lcd_dt_match_table),
#endif
	},
};

int __init lcd_init_temp(void)
{
	if (platform_driver_register(&lcd_platform_driver)) {
		LCDERR("failed to register lcd driver module\n");
		return -ENODEV;
	}

	return 0;
}

void __exit lcd_exit_temp(void)
{
	platform_driver_unregister(&lcd_platform_driver);
}
