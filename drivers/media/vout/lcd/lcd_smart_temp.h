/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _INC_LCD_TEMP_H
#define _INC_LCD_TEMP_H
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/workqueue.h>
#include <linux/of.h>
#include <linux/amlogic/aml_gpio_consumer.h>
#include <linux/pinctrl/consumer.h>
#include <linux/amlogic/iomap.h>
#include <linux/amlogic/media/vout/vout_notify.h>

/* **********************************
 * debug print define
 * **********************************
 */
#define LCDPR(fmt, args...)     pr_info("lcd: " fmt "", ## args)
#define LCDERR(fmt, args...)    pr_err("lcd: error: " fmt "", ## args)

/* **********************************
 * global control define
 * **********************************
 */
#define TTL_DELAY                   13

enum lcd_type_e {
	LCD_TTL = 0,
	LCD_LVDS,
	LCD_VBYONE,
	LCD_MIPI,
	LCD_BT656,
	LCD_BT1120,
	LCD_TYPE_MAX,
};

enum lcd_field_mode_e {
	LCD_PROGRESS = 0,
	LCD_INTERLACE,
	LCD_FLD_MODE_MAX,
};

struct lcd_config_s {
	char propname[24];

	enum lcd_type_e lcd_type;
	unsigned char field_mode;  //0=progress, 1=interlace
	unsigned char lcd_bits;

	unsigned short h_active;    /* Horizontal display area */
	unsigned short v_active;    /* Vertical display area */
	unsigned short h_period;    /* Horizontal total period time */
	unsigned short v_period;    /* Vertical total period time */

	unsigned int lcd_clk;   /* pixel clock(unit: Hz) */
	unsigned int bit_rate; /* Hz */
	unsigned int frame_rate;

	unsigned int hstart;
	unsigned int hend;
	unsigned int vstart;
	unsigned int vend;

	unsigned short hsync_width;
	unsigned short hsync_bp;
	unsigned short hsync_pol;
	unsigned short vsync_width;
	unsigned short vsync_bp;
	unsigned short vsync_pol;

	unsigned short de_hstart;
	unsigned short de_hend;
	unsigned short de_vstart;
	unsigned short de_vend;

	unsigned short hs_hstart;
	unsigned short hs_hend;
	unsigned short hs_vstart;
	unsigned short hs_vend;

	unsigned short vs_hstart;
	unsigned short vs_hend;
	unsigned short vs_vstart;
	unsigned short vs_vend;
};

struct lcd_clk_config_s {
	unsigned int clk_sel;
	unsigned int clk_div;
};

struct lcd_reg_map_s {
	struct resource res;
	unsigned int base_addr;
	unsigned int size;
	void __iomem *p;
	char flag;
};

struct aml_lcd_drv_s {
	unsigned int index;
	unsigned int status;
	unsigned int test_state;
	unsigned char lcd_pxp;

	struct cdev cdev;
	struct device *dev;
	struct device_node *of_node;
	struct lcd_reg_map_s *reg_map;

	struct lcd_config_s config;
	struct lcd_clk_config_s clk_config;

	struct vout_server_s *vout_server;
};

int lcd_smart_config_load_from_dts(struct aml_lcd_drv_s *pdrv);
void lcd_smart_timing_init_config(struct aml_lcd_drv_s *pdrv);

#endif
