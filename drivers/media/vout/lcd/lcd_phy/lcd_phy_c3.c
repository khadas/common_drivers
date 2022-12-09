// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/init.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/amlogic/media/vout/lcd/lcd_vout.h>
#include "../lcd_reg.h"
#include "lcd_phy_config.h"

static struct lcd_phy_ctrl_s *phy_ctrl_p;

static void lcd_mipi_phy_set(struct aml_lcd_drv_s *pdrv, int status)
{
	if (status == LCD_PHY_LOCK_LANE)
		return;

	if (status) {
		lcd_ana_write(ANACTRL_MIPIDSI_CTRL0, 0xa4870008);
		lcd_ana_write(ANACTRL_MIPIDSI_CTRL1, 0x0001002e);
		lcd_ana_write(ANACTRL_MIPIDSI_CTRL2, 0x2680fc59);
	} else {
		lcd_ana_write(ANACTRL_MIPIDSI_CTRL0, 0x04070000);
		lcd_ana_write(ANACTRL_MIPIDSI_CTRL1, 0x2e);
		lcd_ana_write(ANACTRL_MIPIDSI_CTRL2, 0x2680045a);
	}
}

static struct lcd_phy_ctrl_s lcd_phy_ctrl_c3 = {
	.ctrl_bit_on = 1,
	.lane_lock = 0,
	.phy_vswing_level_to_val = NULL,
	.phy_preem_level_to_val = NULL,
	.phy_set_lvds = NULL,
	.phy_set_vx1 = NULL,
	.phy_set_mlvds = NULL,
	.phy_set_p2p = NULL,
	.phy_set_mipi = lcd_mipi_phy_set,
	.phy_set_edp = NULL,
};

struct lcd_phy_ctrl_s *lcd_phy_config_init_c3(struct aml_lcd_drv_s *pdrv)
{
	phy_ctrl_p = &lcd_phy_ctrl_c3;
	return phy_ctrl_p;
}
