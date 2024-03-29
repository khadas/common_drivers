/*
*
* SPDX-License-Identifier: GPL-2.0
*
* Copyright (C) 2011-2018 ARM or its affiliates
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; version 2.
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
* or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
* for more details.
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*
*/

#if !defined(__IMX334_SENSOR_H__)
#define __IMX334_SENSOR_H__


/*-----------------------------------------------------------------------------
Initialization sequence - do not edit
-----------------------------------------------------------------------------*/

#include "sensor_init.h"

/* mclk:24MHz */

/*
 * bayer = BAYER_RGGB
 */
static acam_reg_t linear_1920_1084_30fps_1188Mbps_4lane_10bits[] = {
    {0x3000, 0x01, 0xff, 1}, /* standby */
    {0xFFFF, 1},

    {0x300c, 0x3b, 0xff, 1}, // bcwait_time[7:0]
    {0x300d, 0x2a, 0xff, 1}, // cpwait_time[7:0]
    {0x3018, 0x04, 0xff, 1}, // winmode[3:0]
    {0x302c, 0xfc, 0xff, 1}, // htrimming_start[11:0]
    {0x302d, 0x03, 0xff, 1},
    {0x302e, 0x80, 0xff, 1}, // hnum[11:0]
    {0x302f, 0x07, 0xff, 1},
    {0x3030, 0xca, 0xff, 1}, // vmax[19:0]
    {0x3031, 0x08, 0xff, 1},
    {0x3034, 0x4c, 0xff, 1}, // hmax[15:0]
    {0x3035, 0x04, 0xff, 1},
    {0x3050, 0x00, 0xff, 1}, // adbit[0]
    {0x3074, 0xf8, 0xff, 1}, // area3_st_adr_1[12:0]
    {0x3075, 0x04, 0xff, 1},
    {0x3076, 0x3c, 0xff, 1}, // area3_width_1[12:0]
    {0x3077, 0x04, 0xff, 1},
    {0x308e, 0xf9, 0xff, 1}, // area3_st_adr_2[12:0]
    {0x308f, 0x04, 0xff, 1},
    {0x3090, 0x3c, 0xff, 1}, // area3_width_2[12:0]
    {0x3091, 0x04, 0xff, 1},
    {0x30c6, 0x12, 0xff, 1}, // black_ofset_adr[12:0]
    {0x30ce, 0x64, 0xff, 1}, // unrd_line_max[12:0]
    {0x30d8, 0x40, 0xff, 1}, // unread_ed_adr[12:0]
    {0x30d9, 0x0e, 0xff, 1},
    {0x314c, 0xc6, 0xff, 1}, // incksel 1[8:0]
    {0x315a, 0x02, 0xff, 1}, // incksel2[1:0]
    {0x3168, 0xa0, 0xff, 1}, // incksel3[7:0]
    {0x316a, 0x7e, 0xff, 1}, // incksel4[1:0]
    {0x319d, 0x00, 0xff, 1}, // mdbit
    {0x319e, 0x01, 0xff, 1}, // sys_mode
    {0x31a1, 0x00, 0xff, 1}, // xvs_drv[1:0]
    {0x3288, 0x21, 0xff, 1},
    {0x328a, 0x02, 0xff, 1},
    {0x3308, 0x3c, 0xff, 1}, // y_out_size[12:0]
    {0x3309, 0x04, 0xff, 1},
    {0x3414, 0x05, 0xff, 1},
    {0x3416, 0x18, 0xff, 1},
    {0x341c, 0xff, 0xff, 1}, // adbit1[8:0]
    {0x341d, 0x01, 0xff, 1},
    {0x35ac, 0x0e, 0xff, 1},
    {0x3648, 0x01, 0xff, 1},
    {0x364a, 0x04, 0xff, 1},
    {0x364c, 0x04, 0xff, 1},
    {0x3678, 0x01, 0xff, 1},
    {0x367c, 0x31, 0xff, 1},
    {0x367e, 0x31, 0xff, 1},
    {0x3708, 0x02, 0xff, 1},
    {0x3714, 0x01, 0xff, 1},
    {0x3715, 0x02, 0xff, 1},
    {0x3716, 0x02, 0xff, 1},
    {0x3717, 0x02, 0xff, 1},
    {0x371c, 0x3d, 0xff, 1},
    {0x371d, 0x3f, 0xff, 1},
    {0x372c, 0x00, 0xff, 1},
    {0x372d, 0x00, 0xff, 1},
    {0x372e, 0x46, 0xff, 1},
    {0x372f, 0x00, 0xff, 1},
    {0x3730, 0x89, 0xff, 1},
    {0x3731, 0x00, 0xff, 1},
    {0x3732, 0x08, 0xff, 1},
    {0x3733, 0x01, 0xff, 1},
    {0x3734, 0xfe, 0xff, 1},
    {0x3735, 0x05, 0xff, 1},
    {0x375d, 0x00, 0xff, 1},
    {0x375e, 0x00, 0xff, 1},
    {0x375f, 0x61, 0xff, 1},
    {0x3760, 0x06, 0xff, 1},
    {0x3768, 0x1b, 0xff, 1},
    {0x3769, 0x1b, 0xff, 1},
    {0x376a, 0x1a, 0xff, 1},
    {0x376b, 0x19, 0xff, 1},
    {0x376c, 0x18, 0xff, 1},
    {0x376d, 0x14, 0xff, 1},
    {0x376e, 0x0f, 0xff, 1},
    {0x3776, 0x00, 0xff, 1},
    {0x3777, 0x00, 0xff, 1},
    {0x3778, 0x46, 0xff, 1},
    {0x3779, 0x00, 0xff, 1},
    {0x377a, 0x08, 0xff, 1},
    {0x377b, 0x01, 0xff, 1},
    {0x377c, 0x45, 0xff, 1},
    {0x377d, 0x01, 0xff, 1},
    {0x377e, 0x23, 0xff, 1},
    {0x377f, 0x02, 0xff, 1},
    {0x3780, 0xd9, 0xff, 1},
    {0x3781, 0x03, 0xff, 1},
    {0x3782, 0xf5, 0xff, 1},
    {0x3783, 0x06, 0xff, 1},
    {0x3784, 0xa5, 0xff, 1},
    {0x3788, 0x0f, 0xff, 1},
    {0x378a, 0xd9, 0xff, 1},
    {0x378b, 0x03, 0xff, 1},
    {0x378c, 0xeb, 0xff, 1},
    {0x378d, 0x05, 0xff, 1},
    {0x378e, 0x87, 0xff, 1},
    {0x378f, 0x06, 0xff, 1},
    {0x3790, 0xf5, 0xff, 1},
    {0x3792, 0x43, 0xff, 1},
    {0x3794, 0x7a, 0xff, 1},
    {0x3796, 0xa1, 0xff, 1},
    {0x3a18, 0x8f, 0xff, 1}, // tclkpost[15:0]
    {0x3a1a, 0x4f, 0xff, 1}, // tclkprepare[15:0]
    {0x3a1c, 0x47, 0xff, 1}, // tclktrail[15:0]
    {0x3a1e, 0x37, 0xff, 1}, // tclkzero[15:0]
    {0x3a20, 0x4f, 0xff, 1}, // thsprepare[15:0]
    {0x3a22, 0x87, 0xff, 1}, // thszero[15:0]
    {0x3a24, 0x4f, 0xff, 1}, // thstrail[15:0]
    {0x3a26, 0x7f, 0xff, 1}, // thsexit[15:0]
    {0x3a28, 0x3f, 0xff, 1}, // tlpx[15:0]
    {0x3e04, 0x0e, 0xff, 1},

    {0xFFFF, 1},
    {0x3002, 0x00, 0xff, 1}, /* master mode start */
    {0x0000, 0x0000, 0x0000, 0x0000},
};
static acam_reg_t linear_3840_2160_30fps_1188Mbps_4lane_10bits[] = {
    {0x3000, 0x01, 0xff, 1}, /* standby */
    {0x3002, 0x00, 0xff, 1}, /* XTMSTA */
    {0xFFFF, 1},

    {0x300c, 0x3b, 0xff, 1}, // bcwait_time[7:0]
    {0x300d, 0x2a, 0xff, 1}, // cpwait_time[7:0]
    {0x3030, 0xca, 0xff, 1}, // vmax[19:0]
    {0x3031, 0x08, 0xff, 1},
    {0x3034, 0x4c, 0xff, 1}, // hmax[15:0]
    {0x3035, 0x04, 0xff, 1},
    {0x3050, 0x00, 0xff, 1}, // adbit[0]
    {0x314c, 0xc6, 0xff, 1}, // incksel 1[8:0]
    {0x315a, 0x02, 0xff, 1}, // incksel2[1:0]
    {0x3168, 0xa0, 0xff, 1}, // incksel3[7:0]
    {0x316a, 0x7e, 0xff, 1}, // incksel4[1:0]
    {0x319d, 0x00, 0xff, 1}, // mdbit
    {0x319e, 0x01, 0xff, 1}, // sys_mode
    {0x31a1, 0x00, 0xff, 1}, // xvs_drv[1:0]
    {0x3288, 0x21, 0xff, 1},
    {0x328a, 0x02, 0xff, 1},
    {0x3414, 0x05, 0xff, 1},
    {0x3416, 0x18, 0xff, 1},
    {0x341c, 0xff, 0xff, 1}, // adbit1[8:0]
    {0x341d, 0x01, 0xff, 1},
    {0x35ac, 0x0e, 0xff, 1},
    {0x3648, 0x01, 0xff, 1},
    {0x364a, 0x04, 0xff, 1},
    {0x364c, 0x04, 0xff, 1},
    {0x3678, 0x01, 0xff, 1},
    {0x367c, 0x31, 0xff, 1},
    {0x367e, 0x31, 0xff, 1},
    {0x3708, 0x02, 0xff, 1},
    {0x3714, 0x01, 0xff, 1},
    {0x3715, 0x02, 0xff, 1},
    {0x3716, 0x02, 0xff, 1},
    {0x3717, 0x02, 0xff, 1},
    {0x371c, 0x3d, 0xff, 1},
    {0x371d, 0x3f, 0xff, 1},
    {0x372c, 0x00, 0xff, 1},
    {0x372d, 0x00, 0xff, 1},
    {0x372e, 0x46, 0xff, 1},
    {0x372f, 0x00, 0xff, 1},
    {0x3730, 0x89, 0xff, 1},
    {0x3731, 0x00, 0xff, 1},
    {0x3732, 0x08, 0xff, 1},
    {0x3733, 0x01, 0xff, 1},
    {0x3734, 0xfe, 0xff, 1},
    {0x3735, 0x05, 0xff, 1},
    {0x375d, 0x00, 0xff, 1},
    {0x375e, 0x00, 0xff, 1},
    {0x375f, 0x61, 0xff, 1},
    {0x3760, 0x06, 0xff, 1},
    {0x3768, 0x1b, 0xff, 1},
    {0x3769, 0x1b, 0xff, 1},
    {0x376a, 0x1a, 0xff, 1},
    {0x376b, 0x19, 0xff, 1},
    {0x376c, 0x18, 0xff, 1},
    {0x376d, 0x14, 0xff, 1},
    {0x376e, 0x0f, 0xff, 1},
    {0x3776, 0x00, 0xff, 1},
    {0x3777, 0x00, 0xff, 1},
    {0x3778, 0x46, 0xff, 1},
    {0x3779, 0x00, 0xff, 1},
    {0x377a, 0x08, 0xff, 1},
    {0x377b, 0x01, 0xff, 1},
    {0x377c, 0x45, 0xff, 1},
    {0x377d, 0x01, 0xff, 1},
    {0x377e, 0x23, 0xff, 1},
    {0x377f, 0x02, 0xff, 1},
    {0x3780, 0xd9, 0xff, 1},
    {0x3781, 0x03, 0xff, 1},
    {0x3782, 0xf5, 0xff, 1},
    {0x3783, 0x06, 0xff, 1},
    {0x3784, 0xa5, 0xff, 1},
    {0x3788, 0x0f, 0xff, 1},
    {0x378a, 0xd9, 0xff, 1},
    {0x378b, 0x03, 0xff, 1},
    {0x378c, 0xeb, 0xff, 1},
    {0x378d, 0x05, 0xff, 1},
    {0x378e, 0x87, 0xff, 1},
    {0x378f, 0x06, 0xff, 1},
    {0x3790, 0xf5, 0xff, 1},
    {0x3792, 0x43, 0xff, 1},
    {0x3794, 0x7a, 0xff, 1},
    {0x3796, 0xa1, 0xff, 1},
    {0x3a18, 0x8f, 0xff, 1}, // tclkpost[15:0]
    {0x3a1a, 0x4f, 0xff, 1}, // tclkprepare[15:0]
    {0x3a1c, 0x47, 0xff, 1}, // tclktrail[15:0]
    {0x3a1e, 0x37, 0xff, 1}, // tclkzero[15:0]
    {0x3a20, 0x4f, 0xff, 1}, // thsprepare[15:0]
    {0x3a22, 0x87, 0xff, 1}, // thszero[15:0]
    {0x3a24, 0x4f, 0xff, 1}, // thstrail[15:0]
    {0x3a26, 0x7f, 0xff, 1}, // thsexit[15:0]
    {0x3a28, 0x3f, 0xff, 1}, // tlpx[15:0]
    {0x3e04, 0x0e, 0xff, 1},

    {0xFFFF, 1},
    {0x3002, 0x00, 0xff, 1}, /* master mode start */
    {0x0000, 0x0000, 0x0000, 0x0000},
};

static acam_reg_t dol_1920_1084_30fps_1188Mbps_4lane_10bits[] = {
    {0x3000, 0x01, 0xff, 1}, /* standby */
    {0xFFFF, 1},
    {0x3002, 0x00, 0xff, 1}, /* XTMSTA */

    {0x300c, 0x3b, 0xff, 1}, // bcwait_time[7:0]
    {0x300d, 0x2a, 0xff, 1}, // cpwait_time[7:0]
    {0x3018, 0x04, 0xff, 1}, // winmode[3:0]
    {0x302c, 0xfc, 0xff, 1}, // htrimming_start[11:0]
    {0x302d, 0x03, 0xff, 1},
    {0x302e, 0x80, 0xff, 1}, // hnum[11:0]
    {0x302f, 0x07, 0xff, 1},
    {0x3030, 0xe2, 0xff, 1}, // vmax[19:0]
    {0x3031, 0x04, 0xff, 1},
    {0x3034, 0xde, 0xff, 1}, // hmax[15:0]
    {0x3035, 0x03, 0xff, 1},
    {0x3048, 0x01, 0xff, 1}, // wdmode[0]
    {0x3049, 0x01, 0xff, 1}, // wdsel[1:0]
    {0x304a, 0x01, 0xff, 1}, // wd_set1[2:0]
    {0x304b, 0x02, 0xff, 1}, // wd_set2[3:0]
    {0x304c, 0x13, 0xff, 1}, // opb_size_v[5:0]
    {0x3050, 0x00, 0xff, 1}, // adbit[0]
    {0x3058, 0xb0, 0xff, 1}, // shr0[19:0]
    {0x3068, 0x9d, 0xff, 1}, // rhs1[19:0]
    {0x3074, 0xf8, 0xff, 1}, // area3_st_adr_1[12:0]
    {0x3075, 0x04, 0xff, 1},
    {0x3076, 0x3c, 0xff, 1}, // area3_width_1[12:0]
    {0x3077, 0x04, 0xff, 1},
    {0x308e, 0xf9, 0xff, 1}, // area3_st_adr_2[12:0]
    {0x308f, 0x04, 0xff, 1},
    {0x3090, 0x3c, 0xff, 1}, // area3_width_2[12:0]
    {0x3091, 0x04, 0xff, 1},
    {0x30c6, 0x12, 0xff, 1}, // black_ofset_adr[12:0]
    {0x30ce, 0x64, 0xff, 1}, // unrd_line_max[12:0]
    {0x30d8, 0x40, 0xff, 1}, // unread_ed_adr[12:0]
    {0x30d9, 0x0e, 0xff, 1}, //
    {0x314c, 0xc6, 0xff, 1}, // incksel 1[8:0]
    {0x315a, 0x02, 0xff, 1}, // incksel2[1:0]
    {0x3168, 0xa0, 0xff, 1}, // incksel3[7:0]
    {0x316a, 0x7e, 0xff, 1}, // incksel4[1:0]
    {0x319d, 0x00, 0xff, 1}, // mdbit
    {0x319e, 0x01, 0xff, 1}, // sys_mode
    {0x31a1, 0x00, 0xff, 1}, // xvs_drv[1:0]
    {0x31d7, 0x01, 0xff, 1}, // xvsmskcnt_int[1:0]
    {0x3200, 0x10, 0xff, 1}, // fgainen[0]
    {0x3288, 0x21, 0xff, 1},
    {0x328a, 0x02, 0xff, 1},
    {0x3308, 0x3c, 0xff, 1}, // y_out_size[12:0]
    {0x3309, 0x04, 0xff, 1},
    {0x3414, 0x05, 0xff, 1},
    {0x3416, 0x18, 0xff, 1},
    {0x341c, 0xff, 0xff, 1}, // adbit1[8:0]
    {0x341d, 0x01, 0xff, 1},
    {0x35ac, 0x0e, 0xff, 1},
    {0x3648, 0x01, 0xff, 1},
    {0x364a, 0x04, 0xff, 1},
    {0x364c, 0x04, 0xff, 1},
    {0x3678, 0x01, 0xff, 1},
    {0x367c, 0x31, 0xff, 1},
    {0x367e, 0x31, 0xff, 1},
    {0x3708, 0x02, 0xff, 1},
    {0x3714, 0x01, 0xff, 1},
    {0x3715, 0x02, 0xff, 1},
    {0x3716, 0x02, 0xff, 1},
    {0x3717, 0x02, 0xff, 1},
    {0x371c, 0x3d, 0xff, 1},
    {0x371d, 0x3f, 0xff, 1},
    {0x372c, 0x00, 0xff, 1},
    {0x372d, 0x00, 0xff, 1},
    {0x372e, 0x46, 0xff, 1},
    {0x372f, 0x00, 0xff, 1},
    {0x3730, 0x89, 0xff, 1},
    {0x3731, 0x00, 0xff, 1},
    {0x3732, 0x08, 0xff, 1},
    {0x3733, 0x01, 0xff, 1},
    {0x3734, 0xfe, 0xff, 1},
    {0x3735, 0x05, 0xff, 1},
    {0x375d, 0x00, 0xff, 1},
    {0x375e, 0x00, 0xff, 1},
    {0x375f, 0x61, 0xff, 1},
    {0x3760, 0x06, 0xff, 1},
    {0x3768, 0x1b, 0xff, 1},
    {0x3769, 0x1b, 0xff, 1},
    {0x376a, 0x1a, 0xff, 1},
    {0x376b, 0x19, 0xff, 1},
    {0x376c, 0x18, 0xff, 1},
    {0x376d, 0x14, 0xff, 1},
    {0x376e, 0x0f, 0xff, 1},
    {0x3776, 0x00, 0xff, 1},
    {0x3777, 0x00, 0xff, 1},
    {0x3778, 0x46, 0xff, 1},
    {0x3779, 0x00, 0xff, 1},
    {0x377a, 0x08, 0xff, 1},
    {0x377b, 0x01, 0xff, 1},
    {0x377c, 0x45, 0xff, 1},
    {0x377d, 0x01, 0xff, 1},
    {0x377e, 0x23, 0xff, 1},
    {0x377f, 0x02, 0xff, 1},
    {0x3780, 0xd9, 0xff, 1},
    {0x3781, 0x03, 0xff, 1},
    {0x3782, 0xf5, 0xff, 1},
    {0x3783, 0x06, 0xff, 1},
    {0x3784, 0xa5, 0xff, 1},
    {0x3788, 0x0f, 0xff, 1},
    {0x378a, 0xd9, 0xff, 1},
    {0x378b, 0x03, 0xff, 1},
    {0x378c, 0xeb, 0xff, 1},
    {0x378d, 0x05, 0xff, 1},
    {0x378e, 0x87, 0xff, 1},
    {0x378f, 0x06, 0xff, 1},
    {0x3790, 0xf5, 0xff, 1},
    {0x3792, 0x43, 0xff, 1},
    {0x3794, 0x7a, 0xff, 1},
    {0x3796, 0xa1, 0xff, 1},
    {0x3a18, 0x8f, 0xff, 1}, // tclkpost[15:0]
    {0x3a1a, 0x4f, 0xff, 1}, // tclkprepare[15:0]
    {0x3a1c, 0x47, 0xff, 1}, // tclktrail[15:0]
    {0x3a1e, 0x37, 0xff, 1}, // tclkzero[15:0]
    {0x3a20, 0x4f, 0xff, 1}, // thsprepare[15:0]
    {0x3a22, 0x87, 0xff, 1}, // thszero[15:0]
    {0x3a24, 0x4f, 0xff, 1}, // thstrail[15:0]
    {0x3a26, 0x7f, 0xff, 1}, // thsexit[15:0]
    {0x3a28, 0x3f, 0xff, 1}, // tlpx[15:0]
    {0x3e04, 0x0e, 0xff, 1},

    {0xFFFF, 1},
    {0x3002, 0x00, 0xff, 1}, /* master mode start */
    {0x0000, 0x0000, 0x0000, 0x0000},
};

static acam_reg_t dol_3840_2160_30fps_1440Mbps_4lane_10bits[] = {
    {0x3000, 0x01, 0xff, 1}, /* standby */
    {0xFFFF, 1},
    {0x3002, 0x00, 0xff, 1}, /* XTMSTA */

    {0x300c, 0x3b, 0xff, 1}, // bcwait_time[7:0]
    {0x300d, 0x2a, 0xff, 1}, // cpwait_time[7:0]
    {0x3030, 0xce, 0xff, 1}, // vmax[19:0]
    {0x3031, 0x08, 0xff, 1},
    {0x3034, 0xbc, 0xff, 1}, // hmax[15:0]
    {0x3035, 0x01, 0xff, 1},
    {0x3048, 0x01, 0xff, 1}, // wdmode[0]
    {0x3049, 0x01, 0xff, 1}, // wdsel[1:0]
    {0x304a, 0x01, 0xff, 1}, // wd_set1[2:0]
    {0x304b, 0x02, 0xff, 1}, // wd_set2[3:0]
    {0x304c, 0x13, 0xff, 1}, // opb_size_v[5:0]
    {0x3050, 0x00, 0xff, 1}, // adbit[0]
    {0x3058, 0x1e, 0xff, 1}, // shr0[19:0]
    {0x3059, 0x01, 0xff, 1},
    {0x3068, 0x11, 0xff, 1}, // rhs1[19:0]
    {0x3069, 0x01, 0xff, 1},
    {0x314c, 0xf0, 0xff, 1}, // incksel 1[8:0]
    {0x315a, 0x02, 0xff, 1}, // incksel2[1:0]
    {0x3168, 0x82, 0xff, 1}, // incksel3[7:0]
    {0x316a, 0x7e, 0xff, 1}, // incksel4[1:0]
    {0x319d, 0x00, 0xff, 1}, // mdbit
    {0x31a1, 0x00, 0xff, 1}, // xvs_drv[1:0]
    {0x31d7, 0x01, 0xff, 1}, // xvsmskcnt_int[1:0]
    {0x3200, 0x10, 0xff, 1}, // fgainen[0]
    {0x3288, 0x21, 0xff, 1},
    {0x328a, 0x02, 0xff, 1},
    {0x3414, 0x05, 0xff, 1},
    {0x3416, 0x18, 0xff, 1},
    {0x341c, 0xff, 0xff, 1}, // adbit1[8:0]
    {0x341d, 0x01, 0xff, 1},
    {0x35ac, 0x0e, 0xff, 1},
    {0x3648, 0x01, 0xff, 1},
    {0x364a, 0x04, 0xff, 1},
    {0x364c, 0x04, 0xff, 1},
    {0x3678, 0x01, 0xff, 1},
    {0x367c, 0x31, 0xff, 1},
    {0x367e, 0x31, 0xff, 1},
    {0x3708, 0x02, 0xff, 1},
    {0x3714, 0x01, 0xff, 1},
    {0x3715, 0x02, 0xff, 1},
    {0x3716, 0x02, 0xff, 1},
    {0x3717, 0x02, 0xff, 1},
    {0x371c, 0x3d, 0xff, 1},
    {0x371d, 0x3f, 0xff, 1},
    {0x372c, 0x00, 0xff, 1},
    {0x372d, 0x00, 0xff, 1},
    {0x372e, 0x46, 0xff, 1},
    {0x372f, 0x00, 0xff, 1},
    {0x3730, 0x89, 0xff, 1},
    {0x3731, 0x00, 0xff, 1},
    {0x3732, 0x08, 0xff, 1},
    {0x3733, 0x01, 0xff, 1},
    {0x3734, 0xfe, 0xff, 1},
    {0x3735, 0x05, 0xff, 1},
    {0x375d, 0x00, 0xff, 1},
    {0x375e, 0x00, 0xff, 1},
    {0x375f, 0x61, 0xff, 1},
    {0x3760, 0x06, 0xff, 1},
    {0x3768, 0x1b, 0xff, 1},
    {0x3769, 0x1b, 0xff, 1},
    {0x376a, 0x1a, 0xff, 1},
    {0x376b, 0x19, 0xff, 1},
    {0x376c, 0x18, 0xff, 1},
    {0x376d, 0x14, 0xff, 1},
    {0x376e, 0x0f, 0xff, 1},
    {0x3776, 0x00, 0xff, 1},
    {0x3777, 0x00, 0xff, 1},
    {0x3778, 0x46, 0xff, 1},
    {0x3779, 0x00, 0xff, 1},
    {0x377a, 0x08, 0xff, 1},
    {0x377b, 0x01, 0xff, 1},
    {0x377c, 0x45, 0xff, 1},
    {0x377d, 0x01, 0xff, 1},
    {0x377e, 0x23, 0xff, 1},
    {0x377f, 0x02, 0xff, 1},
    {0x3780, 0xd9, 0xff, 1},
    {0x3781, 0x03, 0xff, 1},
    {0x3782, 0xf5, 0xff, 1},
    {0x3783, 0x06, 0xff, 1},
    {0x3784, 0xa5, 0xff, 1},
    {0x3788, 0x0f, 0xff, 1},
    {0x378a, 0xd9, 0xff, 1},
    {0x378b, 0x03, 0xff, 1},
    {0x378c, 0xeb, 0xff, 1},
    {0x378d, 0x05, 0xff, 1},
    {0x378e, 0x87, 0xff, 1},
    {0x378f, 0x06, 0xff, 1},
    {0x3790, 0xf5, 0xff, 1},
    {0x3792, 0x43, 0xff, 1},
    {0x3794, 0x7a, 0xff, 1},
    {0x3796, 0xa1, 0xff, 1},
    {0x3a04, 0x00, 0xff, 1},
    {0x3a05, 0x06, 0xff, 1},
    {0x3a18, 0x9f, 0xff, 1}, // tclkpost[15:0]
    {0x3a1a, 0x57, 0xff, 1}, // tclkprepare[15:0]
    {0x3a1c, 0x57, 0xff, 1}, // tclktrail[15:0]
    {0x3a1e, 0x87, 0xff, 1}, // tclkzero[15:0]
    {0x3a20, 0x5f, 0xff, 1}, // thsprepare[15:0]
    {0x3a22, 0xa7, 0xff, 1}, // thszero[15:0]
    {0x3a24, 0x5f, 0xff, 1}, // thstrail[15:0]
    {0x3a26, 0x97, 0xff, 1}, // thsexit[15:0]
    {0x3a28, 0x4f, 0xff, 1}, // tlpx[15:0]
    {0x3e04, 0x0e, 0xff, 1},

    {0xFFFF, 1},
    {0x3002, 0x00, 0xff, 1}, /* master mode start */
    {0x0000, 0x0000, 0x0000, 0x0000},
};


static acam_reg_t settings_context_imx334[] = {
    //stop sequence - address is 0x0000
    { 0x0000, 0x0000, 0x0000, 0x0000 }
};
static const acam_reg_t *imx334_seq_table[] = {
    linear_1920_1084_30fps_1188Mbps_4lane_10bits,
    linear_3840_2160_30fps_1188Mbps_4lane_10bits,
    dol_1920_1084_30fps_1188Mbps_4lane_10bits,
    dol_3840_2160_30fps_1440Mbps_4lane_10bits,
};

static const acam_reg_t *isp_seq_table[] = {
    settings_context_imx334,
};

#define SENSOR_IMX334_SEQUENCE_1080P_30FPS_10BIT_4LANE        0
#define SENSOR_IMX334_SEQUENCE_4K_30FPS_10BIT_4LANE        1
#define SENSOR_IMX334_SEQUENCE_1080P_30FPS_10BIT_4LANE_WDR    2
#define SENSOR_IMX334_SEQUENCE_4K_30FPS_10BIT_4LANE_WDR    3
#define SENSOR_IMX334_SEQUENCE_DEFAULT_TEST_PATTERN    5

#define SENSOR_IMX334_ISP_CONTEXT_SEQ 0
#endif /* __IMX334_SENSOR_H__ */
