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
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/of.h>
#include <linux/reset.h>
#include "lcd_smart_temp.h"

struct lcd_type_match_s {
	char *name;
	enum lcd_type_e type;
	unsigned char field_mode;
};

static struct lcd_type_match_s lcd_type_match_table[] = {
	{"ttl",      LCD_TTL,    LCD_PROGRESS},
	{"lvds",     LCD_LVDS,   LCD_PROGRESS},
	{"vbyone",   LCD_VBYONE, LCD_PROGRESS},
	{"mipi",     LCD_MIPI,   LCD_PROGRESS},
	{"bt656",    LCD_BT656,  LCD_INTERLACE},
	{"bt1120",   LCD_BT1120, LCD_INTERLACE},
	{"invalid",  LCD_TYPE_MAX, LCD_FLD_MODE_MAX}
};

static unsigned int lcd_str_to_type(const char *str)
{
	unsigned int type = LCD_TYPE_MAX;
	int i;

	for (i = 0; i < ARRAY_SIZE(lcd_type_match_table); i++) {
		if (!strcmp(str, lcd_type_match_table[i].name)) {
			type = lcd_type_match_table[i].type;
			break;
		}
	}
	return type;
}

static unsigned char lcd_type_to_field_mode(unsigned int type)
{
	unsigned char mode = LCD_FLD_MODE_MAX;
	int i;

	for (i = 0; i < ARRAY_SIZE(lcd_type_match_table); i++) {
		if (type == lcd_type_match_table[i].type) {
			mode = lcd_type_match_table[i].field_mode;
			break;
		}
	}
	return mode;
}

int lcd_smart_config_load_from_dts(struct aml_lcd_drv_s *pdrv)
{
	struct device_node *child;
	struct lcd_config_s *pconf = &pdrv->config;
	unsigned int para[10];
	const char *str;
	int ret = 0;

	if (!pdrv->dev->of_node) {
		LCDERR("dev of_node is null\n");
		return -1;
	}

	child = of_get_child_by_name(pdrv->dev->of_node, pconf->propname);
	if (!child) {
		LCDERR("failed to get %s\n", pconf->propname);
		return -1;
	}

	ret = of_property_read_string(child, "interface", &str);
	if (ret) {
		LCDERR("failed to get interface\n");
		str = "invalid";
	}
	pconf->lcd_type = lcd_str_to_type(str);
	pconf->field_mode = lcd_type_to_field_mode(pconf->lcd_type);
	LCDPR("type: %d(%s), field_mode: %d\n", pconf->lcd_type, str, pconf->field_mode);

	ret = of_property_read_u32_array(child, "basic_setting", &para[0], 7);
	if (ret) {
		LCDERR("failed to get basic_setting\n");
		return -1;
	}
	pconf->h_active = para[0];
	pconf->v_active = para[1];
	pconf->h_period = para[2];
	pconf->v_period = para[3];
	pconf->lcd_bits = para[4];
	LCDPR("h_active: %d, v_active: %d, h_period: %d, v_period: %d, lcd_bits: %d\n",
		pconf->h_active, pconf->v_active,
		pconf->h_period, pconf->v_period,
		pconf->lcd_bits);

	ret = of_property_read_u32_array(child, "lcd_timing", &para[0], 6);
	if (ret) {
		LCDERR("failed to get lcd_timing\n");
		return -1;
	}
	pconf->hsync_width = (unsigned short)(para[0]);
	pconf->hsync_bp = (unsigned short)(para[1]);
	pconf->hsync_pol = (unsigned short)(para[2]);
	pconf->vsync_width = (unsigned short)(para[3]);
	pconf->vsync_bp = (unsigned short)(para[4]);
	pconf->vsync_pol = (unsigned short)(para[5]);
	LCDPR("hsync_width: %d, hsync_bp: %d, vsync_width: %d, vsync_bp: %d\n",
		pconf->hsync_width, pconf->hsync_bp,
		pconf->vsync_width, pconf->vsync_bp);

	ret = of_property_read_u32_array(child, "clk_attr", &para[0], 4);
	if (ret) {
		LCDERR("failed to get clk_attr\n");
		pconf->lcd_clk = 60;
	} else {
		pconf->lcd_clk = para[3];
		if (pconf->lcd_clk == 0) /* avoid 0 mistake */
			pconf->lcd_clk = 60;
	}
	LCDPR("lcd_clk: %d\n", pconf->lcd_clk);

	return ret;
}

void lcd_smart_timing_init_config(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_config_s *pconf = &pdrv->config;
	unsigned short h_period, v_period, h_active, v_active;
	unsigned short hsync_bp, hsync_width, vsync_bp, vsync_width;
	unsigned short de_hstart, de_vstart;
	unsigned short hs_start, hs_end, vs_start, vs_end;
	unsigned short h_delay;

	switch (pconf->lcd_type) {
	case LCD_TTL:
		h_delay = TTL_DELAY;
		break;
	default:
		h_delay = 0;
		break;
	}
	/* use period_dft to avoid period changing offset */
	h_period = pconf->h_period;
	v_period = pconf->v_period;
	h_active = pconf->h_active;
	v_active = pconf->v_active;
	hsync_bp = pconf->hsync_bp;
	hsync_width = pconf->hsync_width;
	vsync_bp = pconf->vsync_bp;
	vsync_width = pconf->vsync_width;

	de_hstart = hsync_bp + hsync_width;
	de_vstart = vsync_bp + vsync_width;

	pconf->hstart = de_hstart - h_delay;
	pconf->hend = pconf->h_active + pconf->hstart - 1;
	pconf->vstart = de_vstart;
	pconf->vend = pconf->v_active + pconf->vstart - 1;

	pconf->de_hstart = de_hstart;
	pconf->de_hend = de_hstart + h_active;
	pconf->de_vstart = de_vstart;
	pconf->de_vend = de_vstart + v_active - 1;

	hs_start = (de_hstart + h_period - hsync_bp - hsync_width) % h_period;
	hs_end = (de_hstart + h_period - hsync_bp) % h_period;
	pconf->hs_hstart = hs_start;
	pconf->hs_hend = hs_end;
	pconf->hs_vstart = 0;
	pconf->hs_vend = v_period - 1;

	pconf->vs_hstart = (hs_start + h_period) % h_period;
	pconf->vs_hend = pconf->vs_hstart;
	vs_start = (de_vstart + v_period - vsync_bp - vsync_width) % v_period;
	vs_end = (de_vstart + v_period - vsync_bp) % v_period;
	pconf->vs_vstart = vs_start;
	pconf->vs_vend = vs_end;

	LCDPR("hstart=%d, hend=%d\n"
		"vstart=%d, vend=%d\n"
		"hs_hstart=%d, hs_hend=%d\n"
		"vs_hstart=%d, vs_hend=%d\n"
		"vs_vstart=%d, vs_vend=%d\n",
		pconf->hstart, pconf->hend,
		pconf->vstart, pconf->vend,
		pconf->hs_hstart, pconf->hs_hend,
		pconf->vs_hstart, pconf->vs_hend,
		pconf->vs_vstart, pconf->vs_vend);
}
