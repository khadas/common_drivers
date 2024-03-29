// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 *
 * Copyright (C) 2019 Amlogic, Inc. All rights reserved.
 *
 */

/* include clk for chip: tl1 tm2 t5 t5d*/

#include <linux/init.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/amlogic/media/vout/lcd/lcd_vout.h>
#include <linux/clk.h>
#ifdef CONFIG_AMLOGIC_VPU
#include <linux/amlogic/media/vpu/vpu.h>
#endif
#include "../lcd_reg.h"
#include "../lcd_common.h"
#include "lcd_clk_config.h"
#include "lcd_clk_ctrl.h"
#include "lcd_clk_utils.h"

static unsigned int pll_ss_reg_tl1[][2] = {
	/* dep_sel,  str_m  */
	{ 0,          0}, /* 0: disable */
	{ 4,          1}, /* 1: +/-0.1% */
	{ 4,          2}, /* 2: +/-0.2% */
	{ 4,          3}, /* 3: +/-0.3% */
	{ 4,          4}, /* 4: +/-0.4% */
	{ 4,          5}, /* 5: +/-0.5% */
	{ 4,          6}, /* 6: +/-0.6% */
	{ 4,          7}, /* 7: +/-0.7% */
	{ 4,          8}, /* 8: +/-0.8% */
	{ 4,          9}, /* 9: +/-0.9% */
	{ 4,         10}, /* 10: +/-1.0% */
	{ 11,         4}, /* 11: +/-1.1% */
	{ 12,         4}, /* 12: +/-1.2% */
	{ 10,         5}, /* 13: +/-1.25% */
	{ 8,          7}, /* 14: +/-1.4% */
	{ 6,         10}, /* 15: +/-1.5% */
	{ 8,          8}, /* 16: +/-1.6% */
	{ 11,         6}, /* 17: +/-1.65% */
	{ 8,          9}, /* 18: +/-1.8% */
	{ 11,         7}, /* 19: +/-1.925% */
	{ 10,         8}, /* 20: +/-2.0% */
	{ 12,         7}, /* 21: +/-2.1% */
	{ 11,         8}, /* 22: +/-2.2% */
	{ 9,         10}, /* 23: +/-2.25% */
	{ 12,         8}, /* 24: +/-2.4% */
	{ 10,        10}, /* 25: +/-2.5% */
	{ 10,        10}, /* 26: +/-2.5% */
	{ 12,         9}, /* 27: +/-2.7% */
	{ 11,        10}, /* 28: +/-2.75% */
	{ 11,        10}, /* 29: +/-2.75% */
	{ 12,        10}, /* 30: +/-3.0% */
};

static void lcd_pll_ss_init(struct lcd_clk_config_s *cconf)
{
	if (!cconf)
		return;

	if (cconf->ss_level > 0) {
		cconf->ss_en = 1;
		cconf->ss_dep_sel = pll_ss_reg_tl1[cconf->ss_level][0];
		cconf->ss_str_m = pll_ss_reg_tl1[cconf->ss_level][1];
		cconf->ss_ppm = cconf->ss_dep_sel * cconf->ss_str_m * cconf->data->ss_dep_base;
	} else {
		cconf->ss_en = 0;
	}
}

static void lcd_pll_ss_enable(struct aml_lcd_drv_s *pdrv, int status)
{
	struct lcd_clk_config_s *cconf;
	unsigned int pll_ctrl2, flag;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	pll_ctrl2 = lcd_ana_read(HHI_TCON_PLL_CNTL2);
	pll_ctrl2 &= ~((0xf << 16) | (0xf << 28));

	if (status) {
		if (cconf->ss_level > 0)
			flag = 1;
		else
			flag = 0;
	} else {
		flag = 0;
	}

	if (flag) {
		cconf->ss_en = 1;
		pll_ctrl2 |= ((cconf->ss_dep_sel << 28) | (cconf->ss_str_m << 16));
		LCDPR("pll ss enable: level %d, %dppm\n", cconf->ss_level, cconf->ss_ppm);
	} else {
		cconf->ss_en = 0;
		LCDPR("pll ss disable\n");
	}
	lcd_ana_write(HHI_TCON_PLL_CNTL2, pll_ctrl2);
}

static void lcd_set_pll_ss_level(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;
	unsigned int level, pll_ctrl2;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	level = cconf->ss_level;
	pll_ctrl2 = lcd_ana_read(HHI_TCON_PLL_CNTL2);
	pll_ctrl2 &= ~((0xf << 16) | (0xf << 28));

	if (level > 0) {
		cconf->ss_en = 1;
		cconf->ss_dep_sel = pll_ss_reg_tl1[level][0];
		cconf->ss_str_m = pll_ss_reg_tl1[level][1];
		cconf->ss_ppm = cconf->ss_dep_sel * cconf->ss_str_m * cconf->data->ss_dep_base;
		pll_ctrl2 |= ((cconf->ss_dep_sel << 28) | (cconf->ss_str_m << 16));
		LCDPR("set pll spread spectrum: level %d, %dppm\n", level, cconf->ss_ppm);
	} else {
		cconf->ss_en = 0;
		LCDPR("set pll spread spectrum: disable\n");
	}
	lcd_ana_write(HHI_TCON_PLL_CNTL2, pll_ctrl2);
}

static void lcd_set_pll_ss_advance(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;
	unsigned int pll_ctrl2;
	unsigned int freq, mode;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	freq = cconf->ss_freq;
	mode = cconf->ss_mode;
	pll_ctrl2 = lcd_ana_read(HHI_TCON_PLL_CNTL2);
	pll_ctrl2 &= ~(0x7 << 24); /* ss_freq */
	pll_ctrl2 |= (freq << 24);
	pll_ctrl2 &= ~(0x3 << 22); /* ss_mode */
	pll_ctrl2 |= (mode << 22);
	lcd_ana_write(HHI_TCON_PLL_CNTL2, pll_ctrl2);

	LCDPR("set pll spread spectrum: freq=%d, mode=%d\n", freq, mode);
}

static void lcd_pll_frac_set(struct aml_lcd_drv_s *pdrv, unsigned int frac)
{
	unsigned int val;

	val = lcd_ana_read(HHI_TCON_PLL_CNTL1);
	lcd_ana_setb(HHI_TCON_PLL_CNTL1, frac, 0, 17);
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("%s: reg 0x%x: 0x%08x->0x%08x\n",
			__func__, HHI_TCON_PLL_CNTL1,
			val, lcd_ana_read(HHI_TCON_PLL_CNTL1));
	}
}

static void lcd_pll_m_set(struct aml_lcd_drv_s *pdrv, unsigned int m)
{
	unsigned int val;

	val = lcd_ana_read(HHI_TCON_PLL_CNTL0);
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, m, 0, 8);
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("%s: reg 0x%x: 0x%08x->0x%08x\n",
			__func__, HHI_TCON_PLL_CNTL0,
			val, lcd_ana_read(HHI_TCON_PLL_CNTL0));
	}
}

static void lcd_pll_reset(struct aml_lcd_drv_s *pdrv)
{
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 1, 29, 1);
	usleep_range(10, 11);
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 0, 29, 1);
}

static void lcd_set_pll(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;
	unsigned int pll_ctrl, pll_ctrl1;
	unsigned int tcon_div_sel;
	int ret, cnt = 0;

	if (lcd_debug_print_flag & LCD_DBG_PR_ADV2)
		LCDPR("lcd clk: set_pll_tl1\n");
	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	tcon_div_sel = cconf->pll_tcon_div_sel;
	pll_ctrl = ((0x3 << 17) | /* gate ctrl */
		(tcon_div[tcon_div_sel][2] << 16) |
		(cconf->pll_n << LCD_PLL_N_TL1) |
		(cconf->pll_m << LCD_PLL_M_TL1) |
		(cconf->pll_od3_sel << LCD_PLL_OD3_TL1) |
		(cconf->pll_od2_sel << LCD_PLL_OD2_TL1) |
		(cconf->pll_od1_sel << LCD_PLL_OD1_TL1));
	pll_ctrl1 = (1 << 28) |
		(tcon_div[tcon_div_sel][0] << 22) |
		(tcon_div[tcon_div_sel][1] << 21) |
		((1 << 20) | /* sdm_en */
		(cconf->pll_frac << 0));

set_pll_retry_tl1:
	lcd_ana_write(HHI_TCON_PLL_CNTL0, pll_ctrl);
	udelay(10);
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 1, LCD_PLL_RST_TL1, 1);
	udelay(10);
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 1, LCD_PLL_EN_TL1, 1);
	udelay(10);
	lcd_ana_write(HHI_TCON_PLL_CNTL1, pll_ctrl1);
	udelay(10);
	lcd_ana_write(HHI_TCON_PLL_CNTL2, 0x0000110c);
	udelay(10);
	lcd_ana_write(HHI_TCON_PLL_CNTL3, 0x10051400);
	udelay(10);
	lcd_ana_setb(HHI_TCON_PLL_CNTL4, 0x0100c0, 0, 24);
	udelay(10);
	lcd_ana_setb(HHI_TCON_PLL_CNTL4, 0x8300c0, 0, 24);
	udelay(10);
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 1, 26, 1);
	udelay(10);
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 0, LCD_PLL_RST_TL1, 1);
	udelay(10);
	lcd_ana_write(HHI_TCON_PLL_CNTL2, 0x0000300c);

	ret = lcd_pll_wait_lock(HHI_TCON_PLL_CNTL0, LCD_PLL_LOCK_TL1);
	if (ret) {
		if (cnt++ < PLL_RETRY_MAX)
			goto set_pll_retry_tl1;
		LCDERR("hpll lock failed\n");
	} else {
		udelay(100);
		lcd_ana_setb(HHI_TCON_PLL_CNTL2, 1, 5, 1);
	}

	if (cconf->ss_level > 0) {
		lcd_set_pll_ss_level(pdrv);
		lcd_set_pll_ss_advance(pdrv);
	}
}

static void lcd_clk_set(struct aml_lcd_drv_s *pdrv)
{
	lcd_clk_setb(HHI_VIID_CLK_CNTL, 0, VCLK2_EN, 1);
	lcd_set_pll(pdrv);
	lcd_set_vid_pll_div_dft(pdrv);
}

static void lcd_pll_ss_init_txhd2(struct lcd_clk_config_s *cconf)
{
	int ret;

	if (!cconf)
		return;

	if (cconf->ss_level > 0) {
		ret = lcd_pll_ss_level_generate(cconf);
		if (ret == 0)
			cconf->ss_en = 1;
	} else {
		cconf->ss_en = 0;
	}
}

static void lcd_pll_ss_enable_txhd2(struct aml_lcd_drv_s *pdrv, int status)
{
	struct lcd_clk_config_s *cconf;
	unsigned int pll_ctrl2, flag;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	pll_ctrl2 = lcd_ana_read(HHI_TCON_PLL_CNTL2);
	pll_ctrl2 &= ~((0xf << 4) | (0xf << 12));

	if (status) {
		if (cconf->ss_level > 0)
			flag = 1;
		else
			flag = 0;
	} else {
		flag = 0;
	}
	if (flag) {
		cconf->ss_en = 1;
		pll_ctrl2 |= ((cconf->ss_dep_sel << 4) | (cconf->ss_str_m << 12));
		LCDPR("[%d]: pll ss enable: level %d, %dppm\n",
			pdrv->index, cconf->ss_level, cconf->ss_ppm);
	} else {
		cconf->ss_en = 0;
		LCDPR("[%d]: pll ss disable\n", pdrv->index);
	}
	lcd_ana_write(HHI_TCON_PLL_CNTL2, pll_ctrl2);
}

static void lcd_set_pll_ss_level_txhd2(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;
	unsigned int pll_ctrl2;
	int ret;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	pll_ctrl2 = lcd_ana_read(HHI_TCON_PLL_CNTL2);
	pll_ctrl2 &= ~((0xf << 4) | (0xf << 12));

	if (cconf->ss_level > 0) {
		ret = lcd_pll_ss_level_generate(cconf);
		if (ret == 0) {
			cconf->ss_en = 1;
			pll_ctrl2 |= ((cconf->ss_dep_sel << 4) | (cconf->ss_str_m << 12));
			LCDPR("[%d]: set pll spread spectrum: level %d, %dppm\n",
				pdrv->index, cconf->ss_level, cconf->ss_ppm);
		}
	} else {
		cconf->ss_en = 0;
		LCDPR("[%d]: set pll spread spectrum: disable\n", pdrv->index);
	}
	lcd_ana_write(HHI_TCON_PLL_CNTL2, pll_ctrl2);
}

static void lcd_set_pll_ss_advance_txhd2(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;
	unsigned int pll_ctrl2;
	unsigned int freq, mode;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	freq = cconf->ss_freq;
	mode = cconf->ss_mode;
	pll_ctrl2 = lcd_ana_read(HHI_TCON_PLL_CNTL2);
	pll_ctrl2 &= ~(0x7 << 20); /* ss_freq */
	pll_ctrl2 |= (freq << 20);
	pll_ctrl2 &= ~(0x3 << 0); /* ss_mode */
	pll_ctrl2 |= (mode << 0);
	lcd_ana_write(HHI_TCON_PLL_CNTL2, pll_ctrl2);

	LCDPR("set pll spread spectrum: freq=%d, mode=%d\n", freq, mode);
}

static void lcd_pll_m_set_txhd2(struct aml_lcd_drv_s *pdrv, unsigned int m)
{
	unsigned int val;

	val = lcd_ana_read(HHI_TCON_PLL_CNTL0);
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, m, 0, 9);
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("%s: reg 0x%x: 0x%08x->0x%08x\n",
			__func__, HHI_TCON_PLL_CNTL0,
			val, lcd_ana_read(HHI_TCON_PLL_CNTL0));
	}
}

static void lcd_pll_reset_txhd2(struct aml_lcd_drv_s *pdrv)
{
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 0, 29, 1);
	usleep_range(10, 11);
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 1, 29, 1);
}

static void lcd_set_pll_txhd2(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;
	unsigned int pll_ctrl, pll_ctrl1;
	int ret, cnt = 0;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	pll_ctrl =
		(cconf->pll_n << 10) |
		(cconf->pll_m << 0) |
		(cconf->pll_od1_sel << 16) |
		(cconf->pll_od2_sel << 18) |
		(cconf->pll_od3_sel << 20) |
		(3 << 24);

	pll_ctrl1 = cconf->pll_frac;

	lcd_ana_write(HHI_TCON_PLL_CNTL0, 1 << 29);
	lcd_ana_write(HHI_TCON_PLL_CNTL0, pll_ctrl);
	lcd_ana_write(HHI_TCON_PLL_CNTL1, pll_ctrl1);
set_pll_retry_txhd2:
	lcd_ana_write(HHI_TCON_PLL_CNTL2, 0x01000000);
	lcd_ana_write(HHI_TCON_PLL_CNTL3, 0x00258000);
	lcd_ana_write(HHI_TCON_PLL_CNTL4, 0x05501000);
	lcd_ana_write(HHI_TCON_PLL_CNTL5, 0x00150500);
	lcd_ana_write(HHI_TCON_PLL_CNTL6, 0x50450000);
	udelay(10);
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 1, 28, 1);
	udelay(10);
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 1, 29, 1);
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 0, 24, 2);
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 1, 23, 1);
	udelay(10);
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 1, 15, 1);
	lcd_ana_write(HHI_TCON_PLL_CNTL6, 0x50440000);

	ret = lcd_pll_wait_lock(HHI_TCON_PLL_STS, 31);
	if (ret) {
		if (cnt++ < PLL_RETRY_MAX)
			goto set_pll_retry_txhd2;
		LCDERR("hpll lock failed\n");
	}

	if (cconf->ss_level > 0) {
		lcd_set_pll_ss_level_txhd2(pdrv);
		lcd_set_pll_ss_advance_txhd2(pdrv);
	}
}

static void lcd_set_vid_pll_div_txhd2(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;
	unsigned int shift_val, shift_sel;
	int i;

	if (!pdrv)
		return;
	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	lcd_clk_setb(HHI_VIID_CLK_CNTL, 0, 19, 1);
	udelay(5);

	/* Disable the div output clock */
	lcd_combo_dphy_setb(pdrv, COMBO_DPHY_VID_PLL0_DIV_TXHD2, 0, 19, 1);
	lcd_combo_dphy_setb(pdrv, COMBO_DPHY_VID_PLL0_DIV_TXHD2, 0, 15, 1);

	i = 0;
	while (lcd_clk_div_table[i][0] != CLK_DIV_SEL_MAX) {
		if (cconf->div_sel == lcd_clk_div_table[i][0])
			break;
		i++;
	}
	if (lcd_clk_div_table[i][0] == CLK_DIV_SEL_MAX)
		LCDERR("invalid clk divider\n");
	shift_val = lcd_clk_div_table[i][1];
	shift_sel = lcd_clk_div_table[i][2];

	if (shift_val == 0xffff) { /* if divide by 1 */
		lcd_combo_dphy_setb(pdrv, COMBO_DPHY_VID_PLL0_DIV_TXHD2, 1, 18, 1);
	} else {
		lcd_combo_dphy_setb(pdrv, COMBO_DPHY_VID_PLL0_DIV_TXHD2, 0, 16, 2);
		lcd_combo_dphy_setb(pdrv, COMBO_DPHY_VID_PLL0_DIV_TXHD2, 0, 15, 1);
		lcd_combo_dphy_setb(pdrv, COMBO_DPHY_VID_PLL0_DIV_TXHD2, 0, 0, 14);
		lcd_combo_dphy_setb(pdrv, COMBO_DPHY_VID_PLL0_DIV_TXHD2, shift_sel, 16, 2);
		lcd_combo_dphy_setb(pdrv, COMBO_DPHY_VID_PLL0_DIV_TXHD2, 1, 15, 1);
		lcd_combo_dphy_setb(pdrv, COMBO_DPHY_VID_PLL0_DIV_TXHD2, shift_val, 0, 14);
		lcd_combo_dphy_setb(pdrv, COMBO_DPHY_VID_PLL0_DIV_TXHD2, 0, 15, 1);
	}
	/* Enable the final output clock */
	lcd_combo_dphy_setb(pdrv, COMBO_DPHY_VID_PLL0_DIV_TXHD2, 1, 19, 1);
}

static void lcd_set_dsi_phy_clk(struct aml_lcd_drv_s *pdrv)
{
	if (lcd_debug_print_flag & LCD_DBG_PR_ADV2)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);
	lcd_clk_setb(HHI_MIPIDSI_PHY_CLK_CNTL, 0, 0, 7);
	lcd_clk_setb(HHI_MIPIDSI_PHY_CLK_CNTL, 0, 12, 3);
	lcd_clk_setb(HHI_MIPIDSI_PHY_CLK_CNTL, 1, 8, 1);
}

static void lcd_clk_set_txhd2(struct aml_lcd_drv_s *pdrv)
{
	lcd_clk_setb(HHI_VIID_CLK_CNTL, 0, VCLK2_EN, 1);
	lcd_set_pll_txhd2(pdrv);
	lcd_set_vid_pll_div_txhd2(pdrv);

	if (pdrv->config.basic.lcd_type == LCD_MIPI) {
		// lcd_set_dsi_meas_clk(pdrv->index);
		lcd_set_dsi_phy_clk(pdrv);
	}
}

static void lcd_set_vclk_crt(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;

	if (lcd_debug_print_flag & LCD_DBG_PR_ADV2)
		LCDPR("lcd clk: set_vclk_crt\n");
	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	if (pdrv->lcd_pxp) {
		/* setup the XD divider value */
		lcd_clk_setb(HHI_VIID_CLK_DIV, cconf->xd, VCLK2_XD, 8);
		udelay(5);

		/* select vid_pll_clk */
		lcd_clk_setb(HHI_VIID_CLK_CNTL, 7, VCLK2_CLK_IN_SEL, 3);
	} else {
		/* setup the XD divider value */
		lcd_clk_setb(HHI_VIID_CLK_DIV, (cconf->xd - 1), VCLK2_XD, 8);
		udelay(5);

		/* select vid_pll_clk */
		lcd_clk_setb(HHI_VIID_CLK_CNTL, cconf->data->vclk_sel, VCLK2_CLK_IN_SEL, 3);
	}
	lcd_clk_setb(HHI_VIID_CLK_CNTL, 1, VCLK2_EN, 1);
	udelay(2);

	/* [15:12] encl_clk_sel, select vclk2_div1 */
	lcd_clk_setb(HHI_VIID_CLK_DIV, 8, ENCL_CLK_SEL, 4);
	/* release vclk2_div_reset and enable vclk2_div */
	lcd_clk_setb(HHI_VIID_CLK_DIV, 1, VCLK2_XD_EN, 2);
	udelay(5);

	lcd_clk_setb(HHI_VIID_CLK_CNTL, 1, VCLK2_DIV1_EN, 1);
	lcd_clk_setb(HHI_VIID_CLK_CNTL, 1, VCLK2_SOFT_RST, 1);
	usleep_range(10, 11);
	lcd_clk_setb(HHI_VIID_CLK_CNTL, 0, VCLK2_SOFT_RST, 1);
	udelay(5);

	/* enable CTS_ENCL clk gate */
	lcd_clk_setb(HHI_VID_CLK_CNTL2, 1, ENCL_GATE_VCLK, 1);
}

static void lcd_clk_disable(struct aml_lcd_drv_s *pdrv)
{
	lcd_clk_setb(HHI_VID_CLK_CNTL2, 0, 3, 1);

	/* close vclk2_div gate: 0x104b[4:0] */
	lcd_clk_setb(HHI_VIID_CLK_CNTL, 0, 0, 5);
	lcd_clk_setb(HHI_VIID_CLK_CNTL, 0, 19, 1);

	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 0, 28, 1);  //disable
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 1, 29, 1);  //reset
}

static void lcd_clk_disable_txhd2(struct aml_lcd_drv_s *pdrv)
{
	lcd_clk_setb(HHI_VID_CLK_CNTL2, 0, 3, 1);

	/* close vclk2_div gate: 0x104b[4:0] */
	lcd_clk_setb(HHI_VIID_CLK_CNTL, 0, 0, 5);
	lcd_clk_setb(HHI_VIID_CLK_CNTL, 0, 19, 1);

	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 0, 28, 1);  //disable
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 0, 29, 1);  //resetn
}

static void lcd_set_tcon_clk_tl1(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;
	struct lcd_config_s *pconf = &pdrv->config;
	unsigned int freq, val;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("lcd clk: set_tcon_clk_tl1\n");
	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	switch (pconf->basic.lcd_type) {
	case LCD_MLVDS:
		val = pconf->control.mlvds_cfg.clk_phase & 0xfff;
		lcd_ana_setb(HHI_TCON_PLL_CNTL1, (val & 0xf), 24, 4);
		lcd_ana_setb(HHI_TCON_PLL_CNTL4, ((val >> 4) & 0xf), 28, 4);
		lcd_ana_setb(HHI_TCON_PLL_CNTL4, ((val >> 8) & 0xf), 24, 4);

		/* tcon_clk */
		if (pconf->timing.lcd_clk >= 100000000) /* 25M */
			freq = 25000000;
		else /* 12.5M */
			freq = 12500000;
		if (!IS_ERR_OR_NULL(cconf->clktree.tcon_clk)) {
			clk_set_rate(cconf->clktree.tcon_clk, freq);
			clk_prepare_enable(cconf->clktree.tcon_clk);
		}
		break;
	case LCD_P2P:
		if (!IS_ERR_OR_NULL(cconf->clktree.tcon_clk)) {
			clk_set_rate(cconf->clktree.tcon_clk, 50000000);
			clk_prepare_enable(cconf->clktree.tcon_clk);
		}
		break;
	default:
		break;
	}
}

static void lcd_set_tcon_clk_t5(struct aml_lcd_drv_s *pdrv)
{
	if (pdrv->config.basic.lcd_type != LCD_MLVDS &&
	    pdrv->config.basic.lcd_type != LCD_P2P)
		return;

	lcd_set_tcon_clk_tl1(pdrv);

	lcd_tcon_global_reset(pdrv);
}

static void lcd_set_clk_phase_txhd2(unsigned int phase_value)
{
	// set clock phase value
	lcd_ana_setb(HHI_TCON_PLL_CNTL1, phase_value, 20, 12);

	// set clock phase load sequence
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 1, 25, 1);
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 0, 23, 1);
	udelay(10);
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 0, 25, 1);
	udelay(10);
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 1, 23, 1);
	udelay(10);
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 1, 25, 1);
	udelay(10);
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 0, 25, 1);
}

static void lcd_set_tcon_clk_txhd2(struct aml_lcd_drv_s *pdrv)
{
	unsigned int val = 0;
	struct lcd_config_s *pconf = &pdrv->config;

	if (pdrv->config.basic.lcd_type != LCD_MLVDS)
		return;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);

	switch (pconf->basic.lcd_type) {
	case LCD_MLVDS:
		val = pconf->control.mlvds_cfg.clk_phase & 0xfff;
		lcd_set_clk_phase_txhd2(val);

		/* tcon_clk */
		if (pconf->timing.lcd_clk >= 100000000) /* 25M */
			lcd_clk_write(HHI_TCON_CLK_CNTL, (1 << 7) | (1 << 6) | (0xf << 0));
		else /* 12.5M */
			lcd_clk_write(HHI_TCON_CLK_CNTL, (1 << 7) | (1 << 6) | (0x1f << 0));
		break;
	default:
		break;
	}

	/* global reset tcon */
	lcd_tcon_global_reset(pdrv);
}

static void lcd_clktree_probe_tl1(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;
	struct clk *temp_clk;
	int ret;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;
	cconf->clktree.clk_gate_state = 0;

	cconf->clktree.encl_top_gate = devm_clk_get(pdrv->dev, "encl_top_gate");
	if (IS_ERR_OR_NULL(cconf->clktree.encl_top_gate))
		LCDERR("%s: get encl_top_gate error\n", __func__);

	cconf->clktree.encl_int_gate = devm_clk_get(pdrv->dev, "encl_int_gate");
	if (IS_ERR_OR_NULL(cconf->clktree.encl_int_gate))
		LCDERR("%s: get encl_int_gate error\n", __func__);

	cconf->clktree.tcon_gate = devm_clk_get(pdrv->dev, "tcon_gate");
	if (IS_ERR_OR_NULL(cconf->clktree.tcon_gate))
		LCDERR("%s: get tcon_gate error\n", __func__);

	temp_clk = devm_clk_get(pdrv->dev, "fclk_div5");
	if (IS_ERR_OR_NULL(temp_clk)) {
		LCDERR("%s: clk fclk_div5\n", __func__);
		return;
	}
	cconf->clktree.tcon_clk = devm_clk_get(pdrv->dev, "clk_tcon");
	if (IS_ERR_OR_NULL(cconf->clktree.tcon_clk)) {
		LCDERR("%s: clk clk_tcon\n", __func__);
	} else {
		ret = clk_set_parent(cconf->clktree.tcon_clk, temp_clk);
		if (ret)
			LCDERR("%s: clk clk_tcon set_parent error\n", __func__);
	}

	LCDPR("lcd_clktree_probe\n");
}

static void lcd_clktree_remove_tl1(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("lcd_clktree_remove\n");
	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	if (!IS_ERR_OR_NULL(cconf->clktree.encl_top_gate))
		devm_clk_put(pdrv->dev, cconf->clktree.encl_top_gate);
	if (!IS_ERR_OR_NULL(cconf->clktree.encl_int_gate))
		devm_clk_put(pdrv->dev, cconf->clktree.encl_int_gate);
	if (!IS_ERR_OR_NULL(cconf->clktree.tcon_clk))
		devm_clk_put(pdrv->dev, cconf->clktree.tcon_clk);
	if (IS_ERR_OR_NULL(cconf->clktree.tcon_gate))
		devm_clk_put(pdrv->dev, cconf->clktree.tcon_gate);
}

static void lcd_prbs_set_pll_vx1(struct aml_lcd_drv_s *pdrv)
{
	int cnt = 0, ret;

lcd_prbs_retry_pll_vx1_tl1:
	lcd_ana_write(HHI_TCON_PLL_CNTL0, 0x000f04f7);
	usleep_range(10, 12);
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 1, LCD_PLL_RST_TL1, 1);
	usleep_range(10, 12);
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 1, LCD_PLL_EN_TL1, 1);
	usleep_range(10, 12);
	lcd_ana_write(HHI_TCON_PLL_CNTL1, 0x10110000);
	usleep_range(10, 12);
	lcd_ana_write(HHI_TCON_PLL_CNTL2, 0x00001108);
	usleep_range(10, 12);
	lcd_ana_write(HHI_TCON_PLL_CNTL3, 0x10051400);
	usleep_range(10, 12);
	lcd_ana_write(HHI_TCON_PLL_CNTL4, 0x010100c0);
	usleep_range(10, 12);
	lcd_ana_write(HHI_TCON_PLL_CNTL4, 0x038300c0);
	usleep_range(10, 12);
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 1, 26, 1);
	usleep_range(10, 12);
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 0, LCD_PLL_RST_TL1, 1);
	usleep_range(10, 12);
	lcd_ana_write(HHI_TCON_PLL_CNTL2, 0x00003008);
	usleep_range(10, 12);
	lcd_ana_write(HHI_TCON_PLL_CNTL2, 0x00003028);
	usleep_range(10, 12);

	ret = lcd_pll_wait_lock(HHI_TCON_PLL_CNTL0, LCD_PLL_LOCK_TL1);
	if (ret) {
		if (cnt++ < PLL_RETRY_MAX)
			goto lcd_prbs_retry_pll_vx1_tl1;
		LCDERR("pll lock failed\n");
	}

	/* pll_div */
	lcd_clk_setb(HHI_VIID_CLK_CNTL, 0, VCLK2_EN, 1);
	usleep_range(5, 10);

	/* Disable the div output clock */
	lcd_ana_setb(HHI_VID_PLL_CLK_DIV, 0, 19, 1);
	lcd_ana_setb(HHI_VID_PLL_CLK_DIV, 0, 15, 1);

	lcd_ana_setb(HHI_VID_PLL_CLK_DIV, 0, 18, 1);
	lcd_ana_setb(HHI_VID_PLL_CLK_DIV, 0, 16, 2);
	lcd_ana_setb(HHI_VID_PLL_CLK_DIV, 0, 15, 1);
	lcd_ana_setb(HHI_VID_PLL_CLK_DIV, 0, 0, 14);

	lcd_ana_setb(HHI_VID_PLL_CLK_DIV, 2, 16, 2);
	lcd_ana_setb(HHI_VID_PLL_CLK_DIV, 1, 15, 1);
	lcd_ana_setb(HHI_VID_PLL_CLK_DIV, 0x739c, 0, 15);
	lcd_ana_setb(HHI_VID_PLL_CLK_DIV, 0, 15, 1);

	/* Enable the final output clock */
	lcd_ana_setb(HHI_VID_PLL_CLK_DIV, 1, 19, 1);
}

static void lcd_prbs_set_pll_lvds(struct aml_lcd_drv_s *pdrv)
{
	int cnt = 0, ret;

lcd_prbs_retry_pll_lvds_tl1:
	lcd_ana_write(HHI_TCON_PLL_CNTL0, 0x008e049f);
	usleep_range(10, 12);
	lcd_ana_write(HHI_TCON_PLL_CNTL0, 0x208e049f);
	usleep_range(10, 12);
	lcd_ana_write(HHI_TCON_PLL_CNTL0, 0x3006049f);
	usleep_range(10, 12);
	lcd_ana_write(HHI_TCON_PLL_CNTL1, 0x10000000);
	usleep_range(10, 12);
	lcd_ana_write(HHI_TCON_PLL_CNTL2, 0x00001102);
	usleep_range(10, 12);
	lcd_ana_write(HHI_TCON_PLL_CNTL3, 0x10051400);
	usleep_range(10, 12);
	lcd_ana_write(HHI_TCON_PLL_CNTL4, 0x010100c0);
	usleep_range(10, 12);
	lcd_ana_write(HHI_TCON_PLL_CNTL4, 0x038300c0);
	usleep_range(10, 12);
	lcd_ana_write(HHI_TCON_PLL_CNTL0, 0x348e049f);
	usleep_range(10, 12);
	lcd_ana_write(HHI_TCON_PLL_CNTL0, 0x148e049f);
	usleep_range(10, 12);
	lcd_ana_write(HHI_TCON_PLL_CNTL2, 0x00003002);
	usleep_range(10, 12);
	lcd_ana_write(HHI_TCON_PLL_CNTL2, 0x00003022);
	usleep_range(10, 12);

	ret = lcd_pll_wait_lock(HHI_TCON_PLL_CNTL0, LCD_PLL_LOCK_TL1);
	if (ret) {
		if (cnt++ < PLL_RETRY_MAX)
			goto lcd_prbs_retry_pll_lvds_tl1;
		LCDERR("pll lock failed\n");
	}

	/* pll_div */
	// lcd_clk_setb(HHI_VIID_CLK_CNTL, 0, VCLK2_EN, 1);
	// usleep_range(5, 10);

	/* Disable the div output clock */
	lcd_ana_setb(HHI_VID_PLL_CLK_DIV, 0, 19, 1);
	lcd_ana_setb(HHI_VID_PLL_CLK_DIV, 0, 15, 1);

	lcd_ana_setb(HHI_VID_PLL_CLK_DIV, 0, 18, 1);
	lcd_ana_setb(HHI_VID_PLL_CLK_DIV, 0, 16, 2);
	lcd_ana_setb(HHI_VID_PLL_CLK_DIV, 0, 15, 1);
	lcd_ana_setb(HHI_VID_PLL_CLK_DIV, 0, 0, 14);

	lcd_ana_setb(HHI_VID_PLL_CLK_DIV, 1, 16, 2);
	lcd_ana_setb(HHI_VID_PLL_CLK_DIV, 1, 15, 1);
	lcd_ana_setb(HHI_VID_PLL_CLK_DIV, 0x3c78, 0, 15);
	lcd_ana_setb(HHI_VID_PLL_CLK_DIV, 0, 15, 1);

	/* Enable the final output clock */
	lcd_ana_setb(HHI_VID_PLL_CLK_DIV, 1, 19, 1);
}

static void lcd_prbs_set_pll_lvds_txhd2(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;
	int cnt = 0, ret;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	//3840 / 8 / 10 = 48M
	//3840 / 4 / 4.67 = 137M
	lcd_ana_write(HHI_TCON_PLL_CNTL0, 1 << 29);
	lcd_ana_write(HHI_TCON_PLL_CNTL0, 0x031484a0);
	lcd_ana_write(HHI_TCON_PLL_CNTL1, 0x00008000);
lcd_prbs_retry_pll_lvds_txhd2:
	lcd_ana_write(HHI_TCON_PLL_CNTL2, 0x01000000);
	lcd_ana_write(HHI_TCON_PLL_CNTL3, 0x00258000);
	lcd_ana_write(HHI_TCON_PLL_CNTL4, 0x05501000);
	lcd_ana_write(HHI_TCON_PLL_CNTL5, 0x00150500);
	lcd_ana_write(HHI_TCON_PLL_CNTL6, 0x50450000);
	usleep_range(5, 10);
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 1, 28, 1);
	usleep_range(5, 10);
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 1, 29, 1);
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 0, 24, 2);
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 1, 23, 1);
	usleep_range(5, 10);
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 1, 15, 1);
	lcd_ana_write(HHI_TCON_PLL_CNTL6, 0x50440000);

	ret = lcd_pll_wait_lock(HHI_TCON_PLL_STS, 31);
	if (ret) {
		if (cnt++ < PLL_RETRY_MAX)
			goto lcd_prbs_retry_pll_lvds_txhd2;
		LCDERR("[%d]: hpll lock failed\n", pdrv->index);
	}

	/* pll_div */
	lcd_clk_setb(HHI_VIID_CLK_CNTL, 0, VCLK2_EN, 1);
	usleep_range(5, 10);

	/* Disable the div output clock */
	lcd_combo_dphy_setb(pdrv, COMBO_DPHY_VID_PLL0_DIV, 0, 19, 1);
	lcd_combo_dphy_setb(pdrv, COMBO_DPHY_VID_PLL0_DIV, 0, 15, 1);

	lcd_combo_dphy_setb(pdrv, COMBO_DPHY_VID_PLL0_DIV, 0, 18, 1);
	lcd_combo_dphy_setb(pdrv, COMBO_DPHY_VID_PLL0_DIV, 0, 16, 2);
	lcd_combo_dphy_setb(pdrv, COMBO_DPHY_VID_PLL0_DIV, 0, 15, 1);
	lcd_combo_dphy_setb(pdrv, COMBO_DPHY_VID_PLL0_DIV, 0, 0, 14);

	lcd_combo_dphy_setb(pdrv, COMBO_DPHY_VID_PLL0_DIV, 1, 16, 2);
	lcd_combo_dphy_setb(pdrv, COMBO_DPHY_VID_PLL0_DIV, 1, 15, 1);
	lcd_combo_dphy_setb(pdrv, COMBO_DPHY_VID_PLL0_DIV, 0x0ccc, 0, 15);
	lcd_combo_dphy_setb(pdrv, COMBO_DPHY_VID_PLL0_DIV, 0, 15, 1);

	/* Enable the final output clock */
	lcd_combo_dphy_setb(pdrv, COMBO_DPHY_VID_PLL0_DIV, 1, 19, 1);
}

void lcd_prbs_config_clk(struct aml_lcd_drv_s *pdrv, unsigned int lcd_prbs_mode)
{
	if (pdrv->data->chip_type == LCD_CHIP_TXHD2) {
		lcd_clk_setb(HHI_VIID_CLK_CNTL, 0, VCLK2_EN, 1);
		lcd_prbs_set_pll_lvds_txhd2(pdrv);
	} else if (lcd_prbs_mode == LCD_PRBS_MODE_VX1) {
		lcd_clk_setb(HHI_VIID_CLK_CNTL, 0, VCLK2_EN, 1);
		lcd_prbs_set_pll_vx1(pdrv);
	} else if (lcd_prbs_mode == LCD_PRBS_MODE_LVDS) {
		lcd_clk_setb(HHI_VIID_CLK_CNTL, 0, VCLK2_EN, 1);
		lcd_prbs_set_pll_lvds(pdrv);
	} else {
		LCDERR("%s: unsupport lcd_prbs_mode %d\n", __func__, lcd_prbs_mode);
		return;
	}

	lcd_clk_setb(HHI_VIID_CLK_DIV, 0, VCLK2_XD, 8);
	usleep_range(5, 10);

	/* select vid_pll_clk */
	lcd_clk_setb(HHI_VIID_CLK_CNTL, 0, VCLK2_CLK_IN_SEL, 3);
	lcd_clk_setb(HHI_VIID_CLK_CNTL, 1, VCLK2_EN, 1);
	usleep_range(5, 10);

	/* [15:12] encl_clk_sel, select vclk2_div1 */
	lcd_clk_setb(HHI_VIID_CLK_DIV, 8, ENCL_CLK_SEL, 4);
	/* release vclk2_div_reset and enable vclk2_div */
	lcd_clk_setb(HHI_VIID_CLK_DIV, 1, VCLK2_XD_EN, 2);
	usleep_range(5, 10);

	lcd_clk_setb(HHI_VIID_CLK_CNTL, 1, VCLK2_DIV1_EN, 1);
	lcd_clk_setb(HHI_VIID_CLK_CNTL, 1, VCLK2_SOFT_RST, 1);
	usleep_range(10, 12);
	lcd_clk_setb(HHI_VIID_CLK_CNTL, 0, VCLK2_SOFT_RST, 1);
	usleep_range(5, 10);

	/* enable CTS_ENCL clk gate */
	lcd_clk_setb(HHI_VID_CLK_CNTL2, 1, ENCL_GATE_VCLK, 1);

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("%s ok\n", __func__);
}

static void lcd_clk_prbs_test(struct aml_lcd_drv_s *pdrv, unsigned int ms, unsigned int mode_flag)
{
	struct lcd_clk_config_s *cconf = get_lcd_clk_config(pdrv);
	unsigned int lcd_prbs_mode, lcd_prbs_cnt;
	unsigned int reg0, reg1;
	unsigned int val1, val2, timeout;
	unsigned int clk_err_cnt = 0;
	int i, j, ret;

	if (!cconf)
		return;

	reg0 = HHI_LVDS_TX_PHY_CNTL0;
	reg1 = HHI_LVDS_TX_PHY_CNTL1;

	ms = (ms > 180000) ? 180000 : ms;
	timeout = ms / 5;

	for (i = 0; i < LCD_PRBS_MODE_MAX; i++) {
		if ((mode_flag & (1 << i)) == 0)
			continue;

		lcd_ana_write(reg0, 0);
		lcd_ana_write(reg1, 0);

		lcd_prbs_cnt = 0;
		clk_err_cnt = 0;
		lcd_prbs_mode = (1 << i);
		if (lcd_prbs_mode == LCD_PRBS_MODE_LVDS) {
			lcd_encl_clk_check_std = 136000000;
			lcd_fifo_clk_check_std = 48000000;
		} else if (lcd_prbs_mode == LCD_PRBS_MODE_VX1) {
			lcd_encl_clk_check_std = 594000000;
			lcd_fifo_clk_check_std = 297000000;
		}

		lcd_prbs_config_clk(pdrv, lcd_prbs_mode);
		msleep(20);

		lcd_ana_write(reg0, 0x000000c0);
		lcd_ana_setb(reg0, 0xfff, 16, 12);
		lcd_ana_setb(reg0, 1, 2, 1);
		lcd_ana_write(reg1, 0x41000000);
		lcd_ana_setb(reg1, 1, 31, 1);

		lcd_ana_write(reg0, 0xfff20c4);
		lcd_ana_setb(reg0, 1, 12, 1);
		val1 = lcd_ana_getb(reg1, 12, 12);

		while (lcd_prbs_flag) {
			if (ms > 1) { /* when s=1, means always run */
				if (lcd_prbs_cnt++ >= timeout)
					break;
			}
			usleep_range(5000, 5001);
			ret = 1;
			for (j = 0; j < 5; j++) {
				val2 = lcd_ana_getb(reg1, 12, 12);
				if (val2 != val1) {
					ret = 0;
					break;
				}
			}
			if (ret) {
				LCDERR("prbs check error 1, val:0x%03x, cnt:%d\n",
				       val2, lcd_prbs_cnt);
				goto lcd_prbs_test_err;
			}
			val1 = val2;
			if (lcd_ana_getb(reg1, 0, 12)) {
				LCDERR("prbs check error 2, cnt:%d\n",
				       lcd_prbs_cnt);
				goto lcd_prbs_test_err;
			}

			if (lcd_prbs_clk_check(lcd_encl_clk_check_std, 9,
					       lcd_fifo_clk_check_std, 129,
					       lcd_prbs_cnt))
				clk_err_cnt++;
			else
				clk_err_cnt = 0;
			if (clk_err_cnt >= 10) {
				LCDERR("prbs check error 3(clkmsr), cnt: %d\n",
				       lcd_prbs_cnt);
				goto lcd_prbs_test_err;
			}
		}

		lcd_ana_write(reg0, 0);
		lcd_ana_write(reg1, 0);

		if (lcd_prbs_mode == LCD_PRBS_MODE_LVDS) {
			lcd_prbs_performed |= LCD_PRBS_MODE_LVDS;
			lcd_prbs_err &= ~(LCD_PRBS_MODE_LVDS);
			LCDPR("lvds prbs check ok\n");
		} else if (lcd_prbs_mode == LCD_PRBS_MODE_VX1) {
			lcd_prbs_performed |= LCD_PRBS_MODE_VX1;
			lcd_prbs_err &= ~(LCD_PRBS_MODE_VX1);
			LCDPR("vx1 prbs check ok\n");
		} else {
			LCDPR("prbs check: unsupport mode\n");
		}
		continue;

lcd_prbs_test_err:
		if (lcd_prbs_mode == LCD_PRBS_MODE_LVDS) {
			lcd_prbs_performed |= LCD_PRBS_MODE_LVDS;
			lcd_prbs_err |= LCD_PRBS_MODE_LVDS;
		} else if (lcd_prbs_mode == LCD_PRBS_MODE_VX1) {
			lcd_prbs_performed |= LCD_PRBS_MODE_VX1;
			lcd_prbs_err |= LCD_PRBS_MODE_VX1;
		}
	}

	lcd_prbs_flag = 0;
}

static void lcd_clk_prbs_test_txhd2(struct aml_lcd_drv_s *pdrv,
				 unsigned int ms, unsigned int mode_flag)
{
	struct lcd_clk_config_s *cconf = get_lcd_clk_config(pdrv);
	unsigned int combo_dphy_ctrl0, combo_dphy_ctrl1, bit_width;
	int encl_msr_id, fifo_msr_id;
	unsigned int lcd_prbs_cnt;
	unsigned int val1, val2, timeout;
	unsigned int clk_err_cnt = 0;
	int j, ret;

	if (!cconf)
		return;
	if (!(mode_flag & LCD_PRBS_MODE_LVDS)) {
		LCDPR("%s: not support\n", __func__);
		goto lcd_prbs_test_err_txhd2;
	}

	//bit[15:0]: reg_hi_edp_lvds_tx_phy0_cntl0
	combo_dphy_ctrl0 = COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL0_TXHD2;
	//bit[31:24]: reg_hi_edp_lvds_tx_phy0_cntl1
	//bit[19:0]: ro_hi_edp_lvds_tx_phy0_cntl1_o
	combo_dphy_ctrl1 = COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL1_TXHD2;
	bit_width = 10;

	encl_msr_id = cconf->data->enc_clk_msr_id;
	fifo_msr_id = cconf->data->fifo_clk_msr_id;

	timeout = (ms > 1000) ? 1000 : ms;

	lcd_combo_dphy_write(pdrv, combo_dphy_ctrl0, 0);
	lcd_combo_dphy_write(pdrv, combo_dphy_ctrl1, 0);

	lcd_prbs_cnt = 0;
	clk_err_cnt = 0;
	LCDPR("[%d]: lcd_prbs_mode: 0x%lx\n", pdrv->index, LCD_PRBS_MODE_LVDS);
	lcd_encl_clk_check_std = 136000000;
	lcd_fifo_clk_check_std = 48000000;

	lcd_prbs_config_clk(pdrv, LCD_PRBS_MODE_LVDS);
	usleep_range(500, 510);

	/* set fifo_clk_sel: div 10 */
	// COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL0[7:6]: Fifo_clk_sel
	lcd_combo_dphy_write(pdrv, combo_dphy_ctrl0, (3 << 6));
	/* set cntl_ser_en:  10-channel */
	lcd_combo_dphy_setb(pdrv, combo_dphy_ctrl0, 0x3ff, 16, 10);
	lcd_combo_dphy_setb(pdrv, combo_dphy_ctrl0, 1, 2, 1);
	/* decoupling fifo enable, gated clock enable */
	lcd_combo_dphy_write(pdrv, combo_dphy_ctrl1, (1 << 30) | (1 << 24));
	/* decoupling fifo write enable after fifo enable */
	lcd_combo_dphy_setb(pdrv, combo_dphy_ctrl1, 1, 31, 1);

	/* cntl_prbs_en & cntl_prbs_err_en*/
	lcd_combo_dphy_setb(pdrv, combo_dphy_ctrl0, 1, 13, 1);
	lcd_combo_dphy_setb(pdrv, combo_dphy_ctrl0, 1, 12, 1);

	while (lcd_prbs_flag) {
		if (lcd_prbs_cnt++ >= timeout)
			break;
		ret = 1;
		val1 = lcd_combo_dphy_getb(pdrv, combo_dphy_ctrl1, bit_width, bit_width);
		usleep_range(1000, 1001);

		for (j = 0; j < 20; j++) {
			val2 = lcd_combo_dphy_getb(pdrv, combo_dphy_ctrl1, bit_width, bit_width);
			usleep_range(5, 10);
			if (val2 != val1) {
				ret = 0;
				break;
			}
		}
		if (ret) {
			LCDERR("[%d]: prbs error 1, val:0x%03x, cnt:%d\n",
					pdrv->index, val2, lcd_prbs_cnt);
			goto lcd_prbs_test_err_txhd2;
		}
		if (lcd_combo_dphy_getb(pdrv, combo_dphy_ctrl1, 0, bit_width)) {
			LCDERR("[%d]: prbs error 2, cnt:%d\n", pdrv->index, lcd_prbs_cnt);
			goto lcd_prbs_test_err_txhd2;
		}

		if (lcd_prbs_clk_check(lcd_encl_clk_check_std, encl_msr_id,
					lcd_fifo_clk_check_std, fifo_msr_id, lcd_prbs_cnt))
			clk_err_cnt++;
		else
			clk_err_cnt = 0;
		if (clk_err_cnt >= 10) {
			LCDERR("[%d]: prbs error 3(clkmsr), cnt:%d\n", pdrv->index, lcd_prbs_cnt);
			goto lcd_prbs_test_err_txhd2;
		}
	}

	lcd_combo_dphy_write(pdrv, combo_dphy_ctrl0, 0);
	lcd_combo_dphy_write(pdrv, combo_dphy_ctrl1, 0);

	lcd_prbs_performed = LCD_PRBS_MODE_LVDS;
	lcd_prbs_err = 0;
	lcd_prbs_flag = 0;
	LCDPR("[%d]: lvds prbs check ok\n", pdrv->index);
	return;

lcd_prbs_test_err_txhd2:
	lcd_prbs_performed = LCD_PRBS_MODE_LVDS;
	lcd_prbs_err = LCD_PRBS_MODE_LVDS;
	lcd_prbs_flag = 0;
}

#ifndef CONFIG_AMLOGIC_REMOVE_OLD
static struct lcd_clk_data_s lcd_clk_data_tl1 = {
	.pll_od_fb = 0,
	.pll_m_max = 511,
	.pll_m_min = 2,
	.pll_n_max = 1,
	.pll_n_min = 1,
	.pll_frac_range = (1 << 17),
	.pll_frac_sign_bit = 18,
	.pll_od_sel_max = 3,
	.pll_ref_fmax = 25000000,
	.pll_ref_fmin = 5000000,
	.pll_vco_fmax = 6024000000ULL,
	.pll_vco_fmin = 3384000000ULL,
	.pll_out_fmax = 3100000000ULL,
	.pll_out_fmin = 211500000,
	.div_in_fmax = 3100000000ULL,
	.div_out_fmax = 750000000,
	.xd_out_fmax = 750000000,
	.od_cnt = 3,
	.have_tcon_div = 1,
	.have_pll_div = 1,
	.phy_clk_location = 0,

	.vclk_sel = 0,
	.enc_clk_msr_id = 9,
	.fifo_clk_msr_id = 129,
	.tcon_clk_msr_id = 128,

	.ss_support = 1,
	.ss_level_max = 30,
	.ss_freq_max = 6,
	.ss_mode_max = 2,
	.ss_dep_base = 500, //ppm
	.ss_dep_sel_max = 12,
	.ss_str_m_max = 10,

	.clk_parameter_init = NULL,
	.clk_generate_parameter = lcd_clk_generate_dft,
	.pll_frac_generate = lcd_pll_frac_generate_dft,
	.set_ss_level = lcd_set_pll_ss_level,
	.set_ss_advance = lcd_set_pll_ss_advance,
	.clk_ss_enable = lcd_pll_ss_enable,
	.clk_ss_init = lcd_pll_ss_init,
	.pll_frac_set = lcd_pll_frac_set,
	.pll_m_set = lcd_pll_m_set,
	.pll_reset = lcd_pll_reset,
	.clk_set = lcd_clk_set,
	.vclk_crt_set = lcd_set_vclk_crt,
	.clk_disable = lcd_clk_disable,
	.clk_gate_switch = lcd_clk_gate_switch_dft,
	.clk_gate_optional_switch = lcd_clk_gate_optional_switch_dft,
	.clktree_set = lcd_set_tcon_clk_tl1,
	.clktree_probe = lcd_clktree_probe_tl1,
	.clktree_remove = lcd_clktree_remove_tl1,
	.clk_config_init_print = lcd_clk_config_init_print_dft,
	.clk_config_print = lcd_clk_config_print_dft,
	.prbs_test = lcd_clk_prbs_test,
};
#endif

static struct lcd_clk_data_s lcd_clk_data_tm2 = {
	.pll_od_fb = 0,
	.pll_m_max = 511,
	.pll_m_min = 2,
	.pll_n_max = 1,
	.pll_n_min = 1,
	.pll_frac_range = (1 << 17),
	.pll_frac_sign_bit = 18,
	.pll_od_sel_max = 3,
	.pll_ref_fmax = 25000000,
	.pll_ref_fmin = 5000000,
	.pll_vco_fmax = 6000000000ULL,
	.pll_vco_fmin = 3000000000ULL,
	.pll_out_fmax = 3100000000ULL,
	.pll_out_fmin = 187500000,
	.div_in_fmax = 3100000000ULL,
	.div_out_fmax = 750000000,
	.xd_out_fmax = 750000000,
	.od_cnt = 3,
	.have_tcon_div = 1,
	.have_pll_div = 1,
	.phy_clk_location = 0,

	.vclk_sel = 0,
	.enc_clk_msr_id = 9,
	.fifo_clk_msr_id = 129,
	.tcon_clk_msr_id = 128,

	.ss_support = 1,
	.ss_level_max = 30,
	.ss_freq_max = 6,
	.ss_mode_max = 2,
	.ss_dep_base = 500, //ppm
	.ss_dep_sel_max = 12,
	.ss_str_m_max = 10,

	.clk_parameter_init = NULL,
	.clk_generate_parameter = lcd_clk_generate_dft,
	.pll_frac_generate = lcd_pll_frac_generate_dft,
	.set_ss_level = lcd_set_pll_ss_level,
	.set_ss_advance = lcd_set_pll_ss_advance,
	.clk_ss_enable = lcd_pll_ss_enable,
	.clk_ss_init = lcd_pll_ss_init,
	.pll_frac_set = lcd_pll_frac_set,
	.pll_m_set = lcd_pll_m_set,
	.pll_reset = lcd_pll_reset,
	.clk_set = lcd_clk_set,
	.vclk_crt_set = lcd_set_vclk_crt,
	.clk_disable = lcd_clk_disable,
	.clk_gate_switch = lcd_clk_gate_switch_dft,
	.clk_gate_optional_switch = lcd_clk_gate_optional_switch_dft,
	.clktree_set = lcd_set_tcon_clk_tl1,
	.clktree_probe = lcd_clktree_probe_tl1,
	.clktree_remove = lcd_clktree_remove_tl1,
	.clk_config_init_print = lcd_clk_config_init_print_dft,
	.clk_config_print = lcd_clk_config_print_dft,
	.prbs_test = NULL,
};

static struct lcd_clk_data_s lcd_clk_data_t5 = {
	.pll_od_fb = 0,
	.pll_m_max = 511,
	.pll_m_min = 2,
	.pll_n_max = 1,
	.pll_n_min = 1,
	.pll_frac_range = (1 << 17),
	.pll_frac_sign_bit = 18,
	.pll_od_sel_max = 3,
	.pll_ref_fmax = 25000000,
	.pll_ref_fmin = 5000000,
	.pll_vco_fmax = 6000000000ULL,
	.pll_vco_fmin = 3000000000ULL,
	.pll_out_fmax = 3100000000ULL,
	.pll_out_fmin = 187500000,
	.div_in_fmax = 3100000000ULL,
	.div_out_fmax = 750000000,
	.xd_out_fmax = 750000000,
	.od_cnt = 3,
	.have_tcon_div = 1,
	.have_pll_div = 1,
	.phy_clk_location = 0,

	.vclk_sel = 0,
	.enc_clk_msr_id = 9,
	.fifo_clk_msr_id = 129,
	.tcon_clk_msr_id = 128,

	.ss_support = 1,
	.ss_level_max = 30,
	.ss_freq_max = 6,
	.ss_mode_max = 2,
	.ss_dep_base = 500, //ppm
	.ss_dep_sel_max = 12,
	.ss_str_m_max = 10,

	.clk_parameter_init = NULL,
	.clk_generate_parameter = lcd_clk_generate_dft,
	.pll_frac_generate = lcd_pll_frac_generate_dft,
	.set_ss_level = lcd_set_pll_ss_level,
	.set_ss_advance = lcd_set_pll_ss_advance,
	.clk_ss_enable = lcd_pll_ss_enable,
	.clk_ss_init = lcd_pll_ss_init,
	.pll_frac_set = lcd_pll_frac_set,
	.pll_m_set = lcd_pll_m_set,
	.pll_reset = lcd_pll_reset,
	.clk_set = lcd_clk_set,
	.vclk_crt_set = lcd_set_vclk_crt,
	.clk_disable = lcd_clk_disable,
	.clk_gate_switch = lcd_clk_gate_switch_dft,
	.clk_gate_optional_switch = lcd_clk_gate_optional_switch_dft,
	.clktree_set = lcd_set_tcon_clk_t5,
	.clktree_probe = lcd_clktree_probe_tl1,
	.clktree_remove = lcd_clktree_remove_tl1,
	.clk_config_init_print = lcd_clk_config_init_print_dft,
	.clk_config_print = lcd_clk_config_print_dft,
	.prbs_test = lcd_clk_prbs_test,
};

static struct lcd_clk_data_s lcd_clk_data_t5d = {
	.pll_od_fb = 0,
	.pll_m_max = 511,
	.pll_m_min = 2,
	.pll_n_max = 1,
	.pll_n_min = 1,
	.pll_frac_range = (1 << 17),
	.pll_frac_sign_bit = 18,
	.pll_od_sel_max = 3,
	.pll_ref_fmax = 25000000,
	.pll_ref_fmin = 5000000,
	.pll_vco_fmax = 6000000000ULL,
	.pll_vco_fmin = 3000000000ULL,
	.pll_out_fmax = 3100000000ULL,
	.pll_out_fmin = 187500000,
	.div_in_fmax = 3100000000ULL,
	.div_out_fmax = 750000000,
	.xd_out_fmax = 400000000,
	.od_cnt = 3,
	.have_tcon_div = 1,
	.have_pll_div = 1,
	.phy_clk_location = 0,

	.vclk_sel = 0,
	.enc_clk_msr_id = 9,
	.fifo_clk_msr_id = 129,
	.tcon_clk_msr_id = 128,

	.ss_support = 1,
	.ss_level_max = 30,
	.ss_freq_max = 6,
	.ss_mode_max = 2,
	.ss_dep_base = 500, //ppm
	.ss_dep_sel_max = 12,
	.ss_str_m_max = 10,

	.clk_parameter_init = NULL,
	.clk_generate_parameter = lcd_clk_generate_dft,
	.pll_frac_generate = lcd_pll_frac_generate_dft,
	.set_ss_level = lcd_set_pll_ss_level,
	.set_ss_advance = lcd_set_pll_ss_advance,
	.clk_ss_enable = lcd_pll_ss_enable,
	.clk_ss_init = lcd_pll_ss_init,
	.pll_frac_set = lcd_pll_frac_set,
	.pll_m_set = lcd_pll_m_set,
	.pll_reset = lcd_pll_reset,
	.clk_set = lcd_clk_set,
	.vclk_crt_set = lcd_set_vclk_crt,
	.clk_disable = lcd_clk_disable,
	.clk_gate_switch = lcd_clk_gate_switch_dft,
	.clk_gate_optional_switch = lcd_clk_gate_optional_switch_dft,
	.clktree_set = lcd_set_tcon_clk_t5,
	.clktree_probe = lcd_clktree_probe_tl1,
	.clktree_remove = lcd_clktree_remove_tl1,
	.clk_config_init_print = lcd_clk_config_init_print_dft,
	.clk_config_print = lcd_clk_config_print_dft,
	.prbs_test = NULL,
};

static struct lcd_clk_data_s lcd_clk_data_txhd2 = {
	.pll_od_fb = 0,
	.pll_m_max = 511,
	.pll_m_min = 2,
	.pll_n_max = 1,
	.pll_n_min = 1,
	.pll_frac_range = (1 << 17),
	.pll_frac_sign_bit = 18,
	.pll_od_sel_max = 3,
	.pll_ref_fmax = 25000000,
	.pll_ref_fmin = 5000000,
	.pll_vco_fmax = 6000000000ULL,
	.pll_vco_fmin = 3000000000ULL,
	.pll_out_fmax = 3100000000ULL,
	.pll_out_fmin = 187500000,
	.div_in_fmax = 3100000000ULL,
	.div_out_fmax = 750000000,
	.xd_out_fmax = 400000000,
	.od_cnt = 3,
	.have_tcon_div = 1,
	.have_pll_div = 1,
	.phy_clk_location = 0,

	.ss_support = 1,
	.ss_level_max = 60,
	.ss_freq_max = 6,
	.ss_mode_max = 2,
	.ss_dep_base = 500, //ppm
	.ss_dep_sel_max = 12,
	.ss_str_m_max = 10,

	.vclk_sel = 0,
	.enc_clk_msr_id = 9,
	.fifo_clk_msr_id = 129,
	.tcon_clk_msr_id = 128,

	.clk_parameter_init = NULL,
	.clk_generate_parameter = lcd_clk_generate_dft,
	.pll_frac_generate = lcd_pll_frac_generate_dft,
	.set_ss_level = lcd_set_pll_ss_level_txhd2,
	.set_ss_advance = lcd_set_pll_ss_advance_txhd2,
	.clk_ss_enable = lcd_pll_ss_enable_txhd2,
	.clk_ss_init = lcd_pll_ss_init_txhd2,
	.pll_frac_set = lcd_pll_frac_set,
	.pll_m_set = lcd_pll_m_set_txhd2,
	.pll_reset = lcd_pll_reset_txhd2,
	.clk_set = lcd_clk_set_txhd2,
	.vclk_crt_set = lcd_set_vclk_crt,
	.clk_disable = lcd_clk_disable_txhd2,
	.clk_gate_switch = lcd_clk_gate_switch_dft,
	.clk_gate_optional_switch = lcd_clk_gate_optional_switch_dft,
	.clktree_set = lcd_set_tcon_clk_txhd2,
	.clktree_probe = lcd_clktree_probe_tl1,
	.clktree_remove = lcd_clktree_remove_tl1,
	.clk_config_init_print = lcd_clk_config_init_print_dft,
	.clk_config_print = lcd_clk_config_print_dft,
	.prbs_test = lcd_clk_prbs_test_txhd2,
};

void lcd_clk_config_chip_init_tl1(struct aml_lcd_drv_s *pdrv, struct lcd_clk_config_s *cconf)
{
	cconf->data = &lcd_clk_data_tl1;
	cconf->pll_od_fb = lcd_clk_data_tl1.pll_od_fb;
	cconf->clk_path_change = NULL;
}

void lcd_clk_config_chip_init_tm2(struct aml_lcd_drv_s *pdrv, struct lcd_clk_config_s *cconf)
{
	cconf->data = &lcd_clk_data_tm2;
	cconf->pll_od_fb = lcd_clk_data_tm2.pll_od_fb;
	cconf->clk_path_change = NULL;
}

void lcd_clk_config_chip_init_t5(struct aml_lcd_drv_s *pdrv, struct lcd_clk_config_s *cconf)
{
	cconf->data = &lcd_clk_data_t5;
	cconf->pll_od_fb = lcd_clk_data_t5.pll_od_fb;
	cconf->clk_path_change = NULL;
}

void lcd_clk_config_chip_init_t5d(struct aml_lcd_drv_s *pdrv, struct lcd_clk_config_s *cconf)
{
	cconf->data = &lcd_clk_data_t5d;
	cconf->pll_od_fb = lcd_clk_data_t5d.pll_od_fb;
	cconf->clk_path_change = NULL;
}

void lcd_clk_config_chip_init_txhd2(struct aml_lcd_drv_s *pdrv, struct lcd_clk_config_s *cconf)
{
	cconf->data = &lcd_clk_data_txhd2;
	cconf->pll_od_fb = lcd_clk_data_txhd2.pll_od_fb;
	cconf->clk_path_change = NULL;
}
