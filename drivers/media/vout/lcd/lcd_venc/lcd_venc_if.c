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

static struct lcd_venc_op_s lcd_venc_op = {
	.init_flag = 0,
	.wait_vsync = NULL,
	.gamma_test_en = NULL,
	.venc_debug_test = NULL,
	.venc_set_timing = NULL,
	.venc_set = NULL,
	.venc_change = NULL,
	.venc_enable = NULL,
	.mute_set = NULL,
	.get_venc_init_config = NULL,
};

void lcd_wait_vsync(struct aml_lcd_drv_s *pdrv)
{
	if (!lcd_venc_op.wait_vsync)
		return;

	lcd_venc_op.wait_vsync(pdrv);
}

void lcd_gamma_debug_test_en(struct aml_lcd_drv_s *pdrv, int flag)
{
	if (!lcd_venc_op.gamma_test_en)
		return;

	lcd_venc_op.gamma_test_en(pdrv, flag);
}

static void lcd_test_pattern_check(struct work_struct *work)
{
	struct aml_lcd_drv_s *pdrv;

	pdrv = container_of(work, struct aml_lcd_drv_s, test_check_work);
	aml_lcd_notifier_call_chain(LCD_EVENT_TEST_PATTERN, (void *)pdrv);
}

void lcd_debug_test(struct aml_lcd_drv_s *pdrv, unsigned int num)
{
	int ret;

	if (!lcd_venc_op.venc_debug_test) {
		LCDERR("[%d]: %s: invalid\n", pdrv->index, __func__);
		return;
	}

	ret = lcd_venc_op.venc_debug_test(pdrv, num);
	if (ret) {
		LCDERR("[%d]: %s: %d not support\n", pdrv->index, __func__, num);
		return;
	}

	if (num == 0)
		LCDPR("[%d]: disable test pattern\n", pdrv->index);
}

static void lcd_screen_restore(struct aml_lcd_drv_s *pdrv)
{
	unsigned int num;
	int ret;

	num = pdrv->test_state;
	if (!lcd_venc_op.venc_debug_test)
		return;

	ret = lcd_venc_op.venc_debug_test(pdrv, num);
	if (ret) {
		LCDERR("[%d]: %s: test %d not support\n",
			pdrv->index, __func__, num);
	}
}

static void lcd_screen_black(struct aml_lcd_drv_s *pdrv)
{
	if (!lcd_venc_op.mute_set) {
		LCDERR("[%d]: %s: invalid\n", pdrv->index, __func__);
		return;
	}

	lcd_venc_op.mute_set(pdrv, 1);
}

void lcd_set_venc_timing(struct aml_lcd_drv_s *pdrv)
{
	if (!lcd_venc_op.venc_set_timing)
		return;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);
	lcd_venc_op.venc_set_timing(pdrv);
}

void lcd_set_venc(struct aml_lcd_drv_s *pdrv)
{
	if (!lcd_venc_op.venc_set) {
		LCDERR("[%d]: %s: invalid\n", pdrv->index, __func__);
		return;
	}

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);
	lcd_venc_op.venc_set(pdrv);
}

void lcd_venc_change(struct aml_lcd_drv_s *pdrv)
{
	if (!lcd_venc_op.venc_change) {
		LCDERR("[%d]: %s: invalid\n", pdrv->index, __func__);
		return;
	}

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);
	lcd_venc_op.venc_change(pdrv);
}

void lcd_venc_enable(struct aml_lcd_drv_s *pdrv, int flag)
{
	if (!lcd_venc_op.venc_enable) {
		LCDERR("[%d]: %s: invalid\n", pdrv->index, __func__);
		return;
	}

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s: %d\n", pdrv->index, __func__, flag);
	lcd_venc_op.venc_enable(pdrv, flag);
}

void lcd_mute_set(struct aml_lcd_drv_s *pdrv,  unsigned char flag)
{
	if (!lcd_venc_op.mute_set) {
		LCDERR("[%d]: %s: invalid\n", pdrv->index, __func__);
		return;
	}

	lcd_venc_op.mute_set(pdrv, flag);
	LCDPR("[%d]: %s: %d\n", pdrv->index, __func__, flag);
}

int lcd_get_venc_init_config(struct aml_lcd_drv_s *pdrv)
{
	int ret = 0;

	if (!lcd_venc_op.get_venc_init_config) {
		LCDERR("[%d]: %s: invalid\n", pdrv->index, __func__);
		return 0;
	}

	ret = lcd_venc_op.get_venc_init_config(pdrv);
	return ret;
}

int lcd_venc_probe(struct aml_lcd_drv_s *pdrv)
{
	int ret;

	if (lcd_venc_op.init_flag)
		return 0;

	switch (pdrv->data->chip_type) {
	case LCD_CHIP_T7:
	case LCD_CHIP_T3:
	case LCD_CHIP_T5W:
		ret = lcd_venc_op_init_t7(pdrv, &lcd_venc_op);
		break;
	case LCD_CHIP_C3:
		ret = lcd_venc_op_init_c3(pdrv, &lcd_venc_op);
		break;
	default:
		ret = lcd_venc_op_init_dft(pdrv, &lcd_venc_op);
		break;
	}
	if (ret) {
		LCDERR("[%d]: %s: failed\n", pdrv->index, __func__);
		return -1;
	}

	lcd_venc_op.init_flag = 1;
	pdrv->lcd_screen_restore = lcd_screen_restore;
	pdrv->lcd_screen_black = lcd_screen_black;
	INIT_WORK(&pdrv->test_check_work, lcd_test_pattern_check);

	return 0;
}
