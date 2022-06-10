// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/init.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/notifier.h>
#include <linux/reset.h>
#include <linux/amlogic/media/vout/lcd/lcd_vout.h>
#include <linux/amlogic/media/vout/lcd/lcd_notify.h>
#include "../lcd_common.h"
#include "lcd_venc.h"

static struct lcd_venc_op_s *lcd_venc_op;

void lcd_wait_vsync(struct aml_lcd_drv_s *pdrv)
{
	if (!lcd_venc_op || !lcd_venc_op->wait_vsync)
		return;

	lcd_venc_op->wait_vsync(pdrv);
}

void lcd_gamma_debug_test_en(struct aml_lcd_drv_s *pdrv, int flag)
{
	if (!lcd_venc_op || !lcd_venc_op->gamma_test_en)
		return;

	lcd_venc_op->gamma_test_en(pdrv, flag);
}

void lcd_set_venc_timing(struct aml_lcd_drv_s *pdrv)
{
	if (!lcd_venc_op || !lcd_venc_op->venc_set_timing)
		return;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);
	lcd_venc_op->venc_set_timing(pdrv);
}

void lcd_set_venc(struct aml_lcd_drv_s *pdrv)
{
	if (!lcd_venc_op || !lcd_venc_op->venc_set)
		return;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);
	lcd_venc_op->venc_set(pdrv);
}

void lcd_venc_change(struct aml_lcd_drv_s *pdrv)
{
	if (!lcd_venc_op || !lcd_venc_op->venc_change)
		return;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);
	lcd_venc_op->venc_change(pdrv);
}

void lcd_venc_enable(struct aml_lcd_drv_s *pdrv, int flag)
{
	if (!lcd_venc_op || !lcd_venc_op->venc_enable)
		return;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);
	lcd_venc_op->venc_enable(pdrv, flag);
}

int lcd_get_venc_init_config(struct aml_lcd_drv_s *pdrv)
{
	int ret = 0;

	if (!lcd_venc_op || !lcd_venc_op->get_venc_init_config)
		return -1;

	ret = lcd_venc_op->get_venc_init_config(pdrv);
	return ret;
}

int lcd_venc_probe(struct aml_lcd_drv_s *pdrv)
{
	switch (pdrv->data->chip_type) {
	case LCD_CHIP_T7:
	case LCD_CHIP_T3:
	case LCD_CHIP_T5W:
		lcd_venc_op = &lcd_venc_op_t7;
		break;
	case LCD_CHIP_C3:
		lcd_venc_op = &lcd_venc_op_c3;
		break;
	default:
		lcd_venc_op = &lcd_venc_op_dft;
		break;
	}

	if (!lcd_venc_op) {
		LCDERR("[%d]: %s: failed\n", pdrv->index, __func__);
		return -1;
	}

	return 0;
}
