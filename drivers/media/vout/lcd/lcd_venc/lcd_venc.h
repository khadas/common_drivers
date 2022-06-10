/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __AML_LCD_VENC_H__
#define __AML_LCD_VENC_H__
#include <linux/amlogic/media/vout/lcd/lcd_vout.h>

#define LCD_WAIT_VSYNC_TIMEOUT    50000

struct lcd_venc_op_s {
	void (*wait_vsync)(struct aml_lcd_drv_s *pdrv);
	void (*gamma_test_en)(struct aml_lcd_drv_s *pdrv, int flag);
	void (*venc_set_timing)(struct aml_lcd_drv_s *pdrv);
	void (*venc_set)(struct aml_lcd_drv_s *pdrv);
	void (*venc_change)(struct aml_lcd_drv_s *pdrv);
	void (*venc_enable)(struct aml_lcd_drv_s *pdrv, int flag);
	int (*get_venc_init_config)(struct aml_lcd_drv_s *pdrv);
};

extern struct lcd_venc_op_s lcd_venc_op_dft;
extern struct lcd_venc_op_s lcd_venc_op_t7;
extern struct lcd_venc_op_s lcd_venc_op_c3;

void lcd_wait_vsync(struct aml_lcd_drv_s *pdrv);
void lcd_gamma_debug_test_en(struct aml_lcd_drv_s *pdrv, int flag);
void lcd_set_venc_timing(struct aml_lcd_drv_s *pdrv);
void lcd_set_venc(struct aml_lcd_drv_s *pdrv);
void lcd_venc_change(struct aml_lcd_drv_s *pdrv);
void lcd_venc_enable(struct aml_lcd_drv_s *pdrv, int flag);
int lcd_get_venc_init_config(struct aml_lcd_drv_s *pdrv);

#endif

