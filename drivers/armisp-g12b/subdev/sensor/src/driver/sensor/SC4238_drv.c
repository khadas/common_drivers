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

#include <linux/delay.h>
#include "acamera_types.h"
#include "system_spi.h"
#include "system_sensor.h"
#include "sensor_bus_config.h"
#include "acamera_command_api.h"
#include "acamera_sbus_api.h"
#include "acamera_sensor_api.h"
#include "system_timer.h"
#include "sensor_init.h"
#include "SC4238_seq.h"
#include "SC4238_config.h"
#include "acamera_math.h"
#include "system_am_mipi.h"
#include "system_am_adap.h"
#include "sensor_bsp_common.h"

#define AGAIN_PRECISION 10
#define NEED_CONFIG_BSP 1   //config bsp by sensor driver owner

static void start_streaming( void *ctx );
static void stop_streaming( void *ctx );

static sensor_context_t sensor_ctx;

static sensor_mode_t supported_modes[] = {
    {
        .wdr_mode = WDR_MODE_LINEAR,
        .fps = 30 * 256,
        .resolution.width = 2688,
        .resolution.height = 1520,
        .bits = 10,
        .exposures = 1,
        .lanes = 2,
        .bps = 675,
        .bayer = BAYER_BGGR,
        .dol_type = DOL_NON,
        .num = SENSOR_SC4238_SEQUENCE_2688_1520_30FPS_10BIT_2LANE,
    },
    {
        .wdr_mode = WDR_MODE_FS_LIN,
        .fps = 30 * 256,
        .resolution.width = 2688,
        .resolution.height = 1520,
        .bits = 10,
        .exposures = 2,
        .lanes = 4,
        .bps = 675,
        .bayer = BAYER_BGGR,
        .dol_type = DOL_VC,
        .num = SENSOR_SC4238_SEQUENCE_2688_1520_30FPS_10BIT_4LANE_WDR,
    },
};

//-------------------------------------------------------------------------------------
#if SENSOR_BINARY_SEQUENCE
static const char p_sensor_data[] = SENSOR_SC4238_SEQUENCE_DEFAULT;
static const char p_isp_data[] = SENSOR_SC4238_ISP_CONTEXT_SEQUENCE;
#else
static const acam_reg_t **p_sensor_data = sc4238_seq_table;
static const acam_reg_t **p_isp_data = isp_seq_table;

#endif
//--------------------RESET------------------------------------------------------------
static void sensor_hw_reset_enable( void )
{
    system_reset_sensor( 0 );
}

static void sensor_hw_reset_disable( void )
{
    system_reset_sensor( 3 );
}

static uint16_t again_index[256] =
{
    1024, 1040, 1056, 1072, 1088, 1104, 1120, 1136, 1152, 1168, 1184, 1200, 1216, 1232, 1248, 1264, 1280, 1296, 1312, 1328, 1344, 1360, 1376, 1392, 1408, 1424, 1440, 1456, 1472, 1488, 1504, 1520,
    1536, 1552, 1568, 1584, 1600, 1616, 1632, 1648, 1664, 1680, 1696, 1712, 1728, 1744, 1760, 1776, 1792, 1808, 1824, 1840, 1856, 1872, 1888, 1904, 1920, 1936, 1952, 1968, 1984, 2000, 2016, 2032,
    2048, 2080, 2112, 2144, 2176, 2208, 2240, 2272, 2304, 2336, 2368, 2400, 2432, 2464, 2496, 2528, 2560, 2592, 2624, 2656, 2688, 2720, 2752, 2784, 2816, 2848, 2880, 2912, 2944, 2976, 3008, 3040,
    3072, 3104, 3136, 3168, 3200, 3232, 3264, 3296, 3328, 3360, 3392, 3424, 3456, 3488, 3520, 3552, 3584, 3616, 3648, 3680, 3712, 3744, 3776, 3808, 3840, 3872, 3904, 3936, 3968, 4000, 4032, 4064,
    4096, 4160, 4224, 4288, 4352, 4416, 4480, 4544, 4608, 4672, 4736, 4800, 4864, 4928, 4992, 5056, 5120, 5184, 5248, 5312, 5376, 5440, 5504, 5568, 5632, 5696, 5760, 5824, 5888, 5952, 6016, 6080,
    6144, 6208, 6272, 6336, 6400, 6464, 6528, 6592, 6656, 6720, 6784, 6848, 6912, 6976, 7040, 7104, 7168, 7232, 7296, 7360, 7424, 7488, 7552, 7616, 7680, 7744, 7808, 7872, 7936, 8000, 8064, 8128,
    8192, 8320, 8448, 8576, 8704, 8832, 8960, 9088, 9216, 9344, 9472, 9600, 9728, 9856, 9984, 10112, 10240, 10368, 10496, 10624, 10752, 10880, 11008, 11136, 11264, 11392, 11520, 11648, 11776, 11904, 12032,
    12160, 12288, 12416, 12544, 12672, 12800, 12928, 13056, 13184, 13312, 13440, 13568, 13696, 13824, 13952, 14080, 14208, 14336, 14464, 14592, 14720, 14848, 14976, 15104, 15232, 15360, 15488, 15616, 15744, 15872, 16000, 16128, 16256
};

static uint16_t again_table[256] =
{
    0x0340, 0x0341, 0x0342, 0x0343, 0x0344, 0x0345, 0x0346, 0x0347, 0x0348, 0x0349, 0x034A, 0x034B, 0x034C, 0x034D, 0x034E, 0x034F, 0x0350, 0x0351, 0x0352, 0x0353, 0x0354, 0x0355, 0x0356, 0x0357, 0x0358, 0x0359, 0x035A, 0x035B, 0x035C, 0x035D, 0x035E, 0x035F,
    0x0360, 0x0361, 0x0362, 0x0363, 0x0364, 0x0365, 0x0366, 0x0367, 0x0368, 0x0369, 0x036A, 0x036B, 0x036C, 0x036D, 0x036E, 0x036F, 0x0370, 0x0371, 0x0372, 0x0373, 0x0374, 0x0375, 0x0376, 0x0377, 0x0378, 0x0379, 0x037A, 0x037B, 0x037C, 0x037D, 0x037E, 0x037F,
    0x0740, 0x0741, 0x0742, 0x0743, 0x0744, 0x0745, 0x0746, 0x0747, 0x0748, 0x0749, 0x074A, 0x074B, 0x074C, 0x074D, 0x074E, 0x074F, 0x0750, 0x0751, 0x0752, 0x0753, 0x0754, 0x0755, 0x0756, 0x0757, 0x0758, 0x0759, 0x075A, 0x075B, 0x075C, 0x075D, 0x075E, 0x075F,
    0x0760, 0x0761, 0x0762, 0x0763, 0x0764, 0x0765, 0x0766, 0x0767, 0x0768, 0x0769, 0x076A, 0x076B, 0x076C, 0x076D, 0x076E, 0x076F, 0x0770, 0x0771, 0x0772, 0x0773, 0x0774, 0x0775, 0x0776, 0x0777, 0x0778, 0x0779, 0x077A, 0x077B, 0x077C, 0x077D, 0x077E, 0x077F,
    0x0f40, 0x0f41, 0x0f42, 0x0f43, 0x0f44, 0x0f45, 0x0f46, 0x0f47, 0x0f48, 0x0f49, 0x0f4A, 0x0f4B, 0x0f4C, 0x0f4D, 0x0f4E, 0x0f4F, 0x0f50, 0x0f51, 0x0f52, 0x0f53, 0x0f54, 0x0f55, 0x0f56, 0x0f57, 0x0f58, 0x0f59, 0x0f5A, 0x0f5B, 0x0f5C, 0x0f5D, 0x0f5E, 0x0f5F,
    0x0f60, 0x0f61, 0x0f62, 0x0f63, 0x0f64, 0x0f65, 0x0f66, 0x0f67, 0x0f68, 0x0f69, 0x0f6A, 0x0f6B, 0x0f6C, 0x0f6D, 0x0f6E, 0x0f6F, 0x0f70, 0x0f71, 0x0f72, 0x0f73, 0x0f74, 0x0f75, 0x0f76, 0x0f77, 0x0f78, 0x0f79, 0x0f7A, 0x0f7B, 0x0f7C, 0x0f7D, 0x0f7E, 0x0f7F,
    0x1f40, 0x1f41, 0x1f42, 0x1f43, 0x1f44, 0x1f45, 0x1f46, 0x1f47, 0x1f48, 0x1f49, 0x1f4A, 0x1f4B, 0x1f4C, 0x1f4D, 0x1f4E, 0x1f4F, 0x1f50, 0x1f51, 0x1f52, 0x1f53, 0x1f54, 0x1f55, 0x1f56, 0x1f57, 0x1f58, 0x1f59, 0x1f5A, 0x1f5B, 0x1f5C, 0x1f5D, 0x1f5E, 0x1f5F,
    0x1f60, 0x1f61, 0x1f62, 0x1f63, 0x1f64, 0x1f65, 0x1f66, 0x1f67, 0x1f68, 0x1f69, 0x1f6A, 0x1f6B, 0x1f6C, 0x1f6D, 0x1f6E, 0x1f6F, 0x1f70, 0x1f71, 0x1f72, 0x1f73, 0x1f74, 0x1f75, 0x1f76, 0x1f77, 0x1f78, 0x1f79, 0x1f7A, 0x1f7B, 0x1f7C, 0x1f7D, 0x1f7E, 0x1f7F
};

//-------------------------------------------------------------------------------------
static int32_t sensor_alloc_analog_gain( void *ctx, int32_t gain )
{
    sensor_context_t *p_ctx = ctx;
    uint16_t again_sc = 1024;
    uint16_t i;
    uint32_t again = acamera_math_exp2( gain, LOG2_GAIN_SHIFT, AGAIN_PRECISION );

    if ( again >= again_index[255]) {
        again = again_index[255];
        again_sc = again_table[255];
    }
    else {
        for (i = 1; i < 256; i++)
        {
            if (again < again_index[i]) {
                again = again_index[i - 1];
                again_sc = again_table[i - 1];
                break;
            }
        }
    }

    if ( p_ctx->again[0] != again_sc ) {
        p_ctx->gain_cnt = p_ctx->again_delay + 1;
        p_ctx->again[0] = again_sc;
    }

    return acamera_log2_fixed_to_fixed( again, AGAIN_PRECISION, LOG2_GAIN_SHIFT );
}

static int32_t sensor_alloc_digital_gain( void *ctx, int32_t gain )
{
    return 0;
}

static void sensor_alloc_integration_time( void *ctx, uint16_t *int_time_S, uint16_t *int_time_M, uint16_t *int_time_L )
{
    sensor_context_t *p_ctx = ctx;

    switch ( p_ctx->wdr_mode ) {
    case WDR_MODE_LINEAR: // Normal mode
        if ( *int_time_S > p_ctx->param.integration_time_max ) *int_time_S = p_ctx->param.integration_time_max;
        if ( *int_time_S < p_ctx->param.integration_time_min ) *int_time_S = p_ctx->param.integration_time_min;

        if ( p_ctx->int_time_S != *int_time_S ) {
            p_ctx->int_cnt = 2;
            p_ctx->int_time_S = *int_time_S;
        }
        break;
    case WDR_MODE_FS_LIN: // DOL2 Frames
        if ( *int_time_L < p_ctx->param.integration_time_min ) *int_time_L = p_ctx->param.integration_time_min;
        if ( *int_time_S < p_ctx->param.integration_time_min ) *int_time_S = p_ctx->param.integration_time_min;
        if ( *int_time_L > ( p_ctx->max_L ) ) *int_time_L = p_ctx->max_L;
        if ( *int_time_S > ( p_ctx->max_S ) ) *int_time_S = p_ctx->max_S;

        if ( p_ctx->int_time_S != *int_time_S || p_ctx->int_time_L != *int_time_L ) {
            p_ctx->int_cnt = 2;
            p_ctx->int_time_S = *int_time_S;
            p_ctx->int_time_L = *int_time_L;
        }
        break;
    }
}

static int32_t sensor_ir_cut_set( void *ctx, int32_t ir_cut_state )
{
    sensor_context_t *t_ctx = ctx;
    int ret;
    sensor_bringup_t* sensor_bp = t_ctx->sbp;

    if ( sensor_bp->ir_gname[0] <= 0 ) {
        pr_err("get gpio id fail: %d\n", ir_cut_state);
        return 0;
    }

    if (ir_cut_state == 1) {
        ret = pwr_ir_cut_enable(sensor_bp, sensor_bp->ir_gname[0], 1);
        if (ret < 0 )
            pr_err("set power fail\n");
    } else if (ir_cut_state == 0) {
        ret = pwr_ir_cut_enable(sensor_bp, sensor_bp->ir_gname[0], 0);
        if (ret < 0 )
            pr_err("set power fail\n");
    } else {
        if (gpio_is_valid(sensor_bp->ir_gname[0]))
            devm_gpio_free(sensor_bp->dev, sensor_bp->ir_gname[0]);
    }

    mdelay(500);

    LOG( LOG_INFO, "exit ir cut:%d", ir_cut_state );

    return 0;
}

static void sensor_update( void *ctx )
{
    sensor_context_t *p_ctx = ctx;
    acamera_sbus_ptr_t p_sbus = &p_ctx->sbus;

    if ( p_ctx->int_cnt || p_ctx->gain_cnt ) {
        // ---------- Analog Gain -------------
        if ( p_ctx->gain_cnt ) {
            switch ( p_ctx->wdr_mode ) {
            case WDR_MODE_LINEAR:
                acamera_sbus_write_u8( p_sbus, 0x3e08, (p_ctx->again[p_ctx->again_delay]>> 8 ) & 0x1F );
                acamera_sbus_write_u8( p_sbus, 0x3e09, (p_ctx->again[p_ctx->again_delay]>> 0 ) & 0xFF );
                break;
            case WDR_MODE_FS_LIN:
                acamera_sbus_write_u8( p_sbus, 0x3e08, (p_ctx->again[p_ctx->again_delay]>> 8 ) & 0x1F );
                acamera_sbus_write_u8( p_sbus, 0x3e09, (p_ctx->again[p_ctx->again_delay]>> 0 ) & 0xFF );

                acamera_sbus_write_u8( p_sbus, 0x3e12, ( p_ctx->again[p_ctx->again_delay] >> 8 ) & 0x1F );
                acamera_sbus_write_u8( p_sbus, 0x3e13, ( p_ctx->again[p_ctx->again_delay] >> 0 ) & 0xFF );
                break;
            }
            p_ctx->gain_cnt--;
        }

        // -------- Integration Time ----------
        if ( p_ctx->int_cnt ) {
            switch ( p_ctx->wdr_mode ) {
            case WDR_MODE_LINEAR:
                acamera_sbus_write_u8( p_sbus, 0x3e00, ( p_ctx->int_time_S >> 12 ) & 0x0F );
                acamera_sbus_write_u8( p_sbus, 0x3e01, ( p_ctx->int_time_S >> 4 ) & 0xFF );
                acamera_sbus_write_u8( p_sbus, 0x3e02,( ( p_ctx->int_time_S >> 0 ) & 0x0F ) << 4);
                break;
            case WDR_MODE_FS_LIN:
                // SHS1
                acamera_sbus_write_u8( p_sbus, 0x3e00, ( p_ctx->int_time_L >> 12 ) & 0x0F );
                acamera_sbus_write_u8( p_sbus, 0x3e01, ( p_ctx->int_time_L >> 4 ) & 0xFF );
                acamera_sbus_write_u8( p_sbus, 0x3e02,( ( p_ctx->int_time_L >> 0 ) & 0x0F ) << 4);
                // SHS2
                acamera_sbus_write_u8( p_sbus, 0x3e04, ( p_ctx->int_time_S >> 4 ) & 0xFF );
                acamera_sbus_write_u8( p_sbus, 0x3e05,( ( p_ctx->int_time_S >> 0 ) & 0x0F ) << 4);
                break;
            }
            p_ctx->int_cnt--;
        }
    }

    p_ctx->again[3] = p_ctx->again[2];
    p_ctx->again[2] = p_ctx->again[1];
    p_ctx->again[1] = p_ctx->again[0];
}

static uint32_t sensor_vmax_fps( void *ctx, uint32_t framerate )
{
    sensor_context_t *p_ctx = ctx;
    acamera_sbus_ptr_t p_sbus = &p_ctx->sbus;
    uint32_t rhs1;

    if ( framerate == 0 )
        return p_ctx->vmax_fps;

    if (framerate > p_ctx->s_fps )
        return 0;

    sensor_param_t *param = &p_ctx->param;
    uint32_t vmax = ( (( p_ctx->s_fps * 1000) / framerate ) * p_ctx->vmax ) / 1000;
    acamera_sbus_write_u8( p_sbus, 0x320f, vmax & 0xFF );
    acamera_sbus_write_u8( p_sbus, 0x320e, vmax >> 8 );

    if ( p_ctx->wdr_mode == WDR_MODE_LINEAR ) {
        p_ctx->max_L = 2 * vmax - 10 ;
        param->integration_time_min = 3;
        param->integration_time_limit = p_ctx->max_L;
        param->integration_time_max = p_ctx->max_L;
    } else {
        rhs1 = ((uint32_t)acamera_sbus_read_u8(p_sbus,0x3e23) << 8) | acamera_sbus_read_u8(p_sbus, 0x3e24);
        p_ctx->max_S = 2 * (rhs1 - 8);
        p_ctx->max_L = 16 * p_ctx->max_S;
        param->integration_time_min = 5;
        param->integration_time_limit = p_ctx->max_L;
        param->integration_time_max = p_ctx->max_L;
        param->integration_time_long_max = p_ctx->max_L;
        param->lines_per_second = param->lines_per_second >> 1;
        p_ctx->frame = vmax << 1;
    }

    p_ctx->vmax_adjust = vmax;
    p_ctx->vmax_fps = framerate;

    LOG(LOG_INFO,"framerate:%d, vmax:%d, p_ctx->max_L:%d, param->integration_time_long_max:%d",
        framerate, vmax, p_ctx->max_L, param->integration_time_long_max);

    return 0;
}

static uint16_t sensor_get_id( void *ctx )
{
    /* return that sensor id register does not exist */

    sensor_context_t *p_ctx = ctx;
    uint16_t sensor_id = 0;

    sensor_id |= acamera_sbus_read_u8(&p_ctx->sbus, 0x3107) << 8;
    sensor_id |= acamera_sbus_read_u8(&p_ctx->sbus, 0x3108);

    if (sensor_id != SENSOR_CHIP_ID) {
        LOG(LOG_CRIT, "%s: Failed to read sensor id\n", __func__);
        return 0xFFFF;
    }

    LOG(LOG_CRIT, "%s: success to read sensor id: 0x%x\n", __func__, sensor_id);

    return sensor_id;
}

static void sensor_set_mode( void *ctx, uint8_t mode )
{
    sensor_context_t *p_ctx = ctx;
    sensor_param_t *param = &p_ctx->param;
    acamera_sbus_ptr_t p_sbus = &p_ctx->sbus;
    uint8_t setting_num = param->modes_table[mode].num;
    uint32_t rhs1;

    sensor_hw_reset_enable();
    system_timer_usleep( 10000 );
    sensor_hw_reset_disable();
    system_timer_usleep( 10000 );

    p_ctx->again[0] = 0;
    p_ctx->int_time_S = 0;
    p_ctx->int_time_L = 0;

    if (sensor_get_id(ctx) != SENSOR_CHIP_ID) {
        LOG(LOG_INFO, "%s: check sensor failed\n", __func__);
        return;
    }

    switch ( param->modes_table[mode].wdr_mode ) {
    case WDR_MODE_LINEAR:
        sensor_load_sequence( p_sbus, p_ctx->seq_width, p_sensor_data, setting_num);
        p_ctx->again_delay = 0;
        param->integration_time_apply_delay = 2;
        param->isp_exposure_channel_delay = 0;
        break;
    case WDR_MODE_FS_LIN:
        p_ctx->again_delay = 0;
        param->integration_time_apply_delay = 2;
        param->isp_exposure_channel_delay = 0;
        sensor_load_sequence( p_sbus, p_ctx->seq_width, p_sensor_data, setting_num);
        break;
    default:
        LOG(LOG_ERR, "Invalid wdr mode. Returning!");
        return;
    }

    if (param->modes_table[mode].exposures == 1 ) {
        p_ctx->max_L = 2 * p_ctx->vmax - 10;
    } else if (param->modes_table[mode].exposures == 2)  {
        rhs1 = ((uint32_t)acamera_sbus_read_u8(p_sbus, 0x3e23) <<8 ) | acamera_sbus_read_u8(p_sbus, 0x3e24);
        p_ctx->max_S = 2 * (rhs1 - 8);
        p_ctx->max_L = 16 * p_ctx->max_S;
    }

    param->active.width = param->modes_table[mode].resolution.width;
    param->active.height = param->modes_table[mode].resolution.height;

    param->total.width = (( (uint16_t)acamera_sbus_read_u8(p_sbus, 0x320c ) << 8 ) | acamera_sbus_read_u8(p_sbus, 0x320d));
    param->total.height = ((uint32_t)acamera_sbus_read_u8(p_sbus, 0x320e) << 8) | acamera_sbus_read_u8(p_sbus, 0x320f);

    p_ctx->s_fps = param->modes_table[mode].fps >> 8;
    p_ctx->pixel_clock = param->total.width * param->active.height * p_ctx->s_fps;
    p_ctx->vmax = param->total.height;

    param->lines_per_second = p_ctx->pixel_clock / param->total.width;
    param->pixels_per_line = param->total.width;
    if ( param->modes_table[mode].wdr_mode == WDR_MODE_LINEAR ) {
        param->integration_time_min = 3;
        param->integration_time_limit = p_ctx->max_L;
        param->integration_time_max = p_ctx->max_L;
    } else {
        param->integration_time_min = 5;
        param->integration_time_limit = p_ctx->max_L;
        param->integration_time_max = p_ctx->max_L;
        if ( param->modes_table[mode].exposures == 2 ) {
            param->integration_time_long_max = p_ctx->max_L; //(p_ctx->vmax << 1 ) - 256;
            param->lines_per_second = param->lines_per_second >> 1;
            p_ctx->frame = p_ctx->vmax << 1;
        } else {
            param->integration_time_long_max = ( p_ctx->vmax << 2 ) - 256;
            param->lines_per_second = param->lines_per_second >> 2;
            p_ctx->frame = p_ctx->vmax << 2;
        }
    }

    param->sensor_exp_number = param->modes_table[mode].exposures;
    param->mode = mode;
    param->bayer = param->modes_table[mode].bayer;
    p_ctx->wdr_mode = param->modes_table[mode].wdr_mode;
    p_ctx->vmax_adjust = p_ctx->vmax;
    p_ctx->vmax_fps = p_ctx->s_fps;

    //sensor_set_iface(&param->modes_table[mode], p_ctx->win_offset, p_ctx);

    LOG( LOG_CRIT, "Mode %d, Setting num: %d, RES:%dx%d\n", mode, setting_num,
                (int)param->active.width, (int)param->active.height );
}

static const sensor_param_t *sensor_get_parameters( void *ctx )
{
    sensor_context_t *p_ctx = ctx;
    return (const sensor_param_t *)&p_ctx->param;
}

static void sensor_disable_isp( void *ctx )
{
}

static uint32_t read_register( void *ctx, uint32_t address )
{
    sensor_context_t *p_ctx = ctx;
    acamera_sbus_ptr_t p_sbus = &p_ctx->sbus;
    return acamera_sbus_read_u8( p_sbus, address );
}

static void write_register( void *ctx, uint32_t address, uint32_t data )
{
    sensor_context_t *p_ctx = ctx;
    acamera_sbus_ptr_t p_sbus = &p_ctx->sbus;
    acamera_sbus_write_u8( p_sbus, address, data );
}

static void stop_streaming( void *ctx )
{
    sensor_context_t *p_ctx = ctx;
    acamera_sbus_ptr_t p_sbus = &p_ctx->sbus;
    p_ctx->streaming_flg = 0;

    acamera_sbus_write_u8( p_sbus, 0x0100, 0x00 );

    reset_sensor_bus_counter();
    sensor_iface_disable(p_ctx);
    //am_adap_deinit();
    //am_mipi_deinit();
}

static void start_streaming( void *ctx )
{
    sensor_context_t *p_ctx = ctx;
    acamera_sbus_ptr_t p_sbus = &p_ctx->sbus;
    sensor_param_t *param = &p_ctx->param;
    sensor_set_iface(&param->modes_table[param->mode], p_ctx->win_offset, p_ctx);
    p_ctx->streaming_flg = 1;
    acamera_sbus_write_u8( p_sbus, 0x0100, 0x01 );
}

static void sensor_test_pattern( void *ctx, uint8_t mode )
{

}

#if PLATFORM_C308X
static uint32_t write1_reg(unsigned long addr, uint32_t val)
{
    void __iomem *io_addr;
    io_addr = ioremap_nocache(addr, 8);
    if (io_addr == NULL) {
        LOG(LOG_ERR, "%s: Failed to ioremap addr\n", __func__);
        return -1;
    }
    __raw_writel(val, io_addr);
    iounmap(io_addr);
    return 0;
}
#endif

void sensor_deinit_sc4238( void *ctx )
{
    sensor_context_t *t_ctx = ctx;

    reset_sensor_bus_counter();
    am_adap_deinit(0);
    am_mipi_deinit();

    acamera_sbus_deinit(&t_ctx->sbus, sbus_i2c);

    if (t_ctx != NULL && t_ctx->sbp != NULL)
        clk_am_disable(t_ctx->sbp);
}

static sensor_context_t *sensor_global_parameter(void* sbp)
{
    // Local sensor data structure
    int ret;
    sensor_bringup_t* sensor_bp = (sensor_bringup_t*) sbp;

    sensor_ctx.sbp = sbp;

#if PLATFORM_G12B
#if NEED_CONFIG_BSP
    ret = pwr_am_enable(sensor_bp, "power-enable", 0);
    if (ret < 0 )
        pr_err("set power fail\n");
    udelay(30);
#endif

    ret = clk_am_enable(sensor_bp, "24m");
    if (ret < 0 )
        pr_err("set mclk fail\n");
#elif PLATFORM_C308X
    ret = pwr_am_enable(sensor_bp, "power-enable", 0);
    if (ret < 0 )
        pr_err("set power fail\n");
    mdelay(50);
    ret = clk_am_enable(sensor_bp, "g12a_24m");
    if (ret < 0 )
        pr_err("set mclk fail\n");
    write1_reg(0xfe000428, 0x11400400);
#elif PLATFORM_C305X
    ret = gp_pl_am_enable(sensor_bp, "mclk_0", 24000000);
    if (ret < 0 )
        pr_info("set mclk fail\n");
#endif
    udelay(30);

#if NEED_CONFIG_BSP
    ret = reset_am_enable(sensor_bp,"reset", 1);
    if (ret < 0 )
        pr_info("set reset fail\n");
#endif

    sensor_ctx.sbus.mask =
        SBUS_MASK_SAMPLE_8BITS | SBUS_MASK_ADDR_16BITS |
        SBUS_MASK_ADDR_SWAP_BYTES;
    sensor_ctx.sbus.control = 0;
    sensor_ctx.sbus.bus = 0;
    sensor_ctx.sbus.device = SENSOR_DEV_ADDRESS;
    acamera_sbus_init( &sensor_ctx.sbus, sbus_i2c );

    // Initial local parameters
    sensor_ctx.address = SENSOR_DEV_ADDRESS;
    sensor_ctx.seq_width = 1;
    sensor_ctx.streaming_flg = 0;
    sensor_ctx.again[0] = 0;
    sensor_ctx.again[1] = 0;
    sensor_ctx.again[2] = 0;
    sensor_ctx.again[3] = 0;
    sensor_ctx.again_limit = 15872;
    sensor_ctx.pixel_clock = 135000000;
    sensor_ctx.s_fps = 30;
    sensor_ctx.vmax = 1550;
    sensor_ctx.vmax_fps = sensor_ctx.s_fps;
    sensor_ctx.vmax_adjust = sensor_ctx.vmax;

    sensor_ctx.param.again_accuracy = 1 << LOG2_GAIN_SHIFT;
    sensor_ctx.param.sensor_exp_number = 1;
    sensor_ctx.param.again_log2_max = 16 << LOG2_GAIN_SHIFT;
    sensor_ctx.param.dgain_log2_max = 0;
    sensor_ctx.param.integration_time_apply_delay = 2;
    sensor_ctx.param.isp_exposure_channel_delay = 0;
    sensor_ctx.param.modes_table = supported_modes;
    sensor_ctx.param.modes_num = array_size_s( supported_modes );
    sensor_ctx.param.sensor_ctx = &sensor_ctx;
    sensor_ctx.param.isp_context_seq.sequence = p_isp_data;
    sensor_ctx.param.isp_context_seq.seq_num= SENSOR_SC4238_CONTEXT_SEQ;
    sensor_ctx.param.isp_context_seq.seq_table_max = array_size_s( isp_seq_table );
    sensor_ctx.cam_isp_path = CAM0_ACT;
    sensor_ctx.dcam_mode = 0;

    memset(&sensor_ctx.win_offset, 0, sizeof(sensor_ctx.win_offset));

    return &sensor_ctx;
}

//--------------------Initialization------------------------------------------------------------
void sensor_init_sc4238( void **ctx, sensor_control_t *ctrl, void* sbp)
{
    *ctx = sensor_global_parameter(sbp);

    ctrl->alloc_analog_gain = sensor_alloc_analog_gain;
    ctrl->alloc_digital_gain = sensor_alloc_digital_gain;
    ctrl->alloc_integration_time = sensor_alloc_integration_time;
    ctrl->ir_cut_set= sensor_ir_cut_set;
    ctrl->sensor_update = sensor_update;
    ctrl->set_mode = sensor_set_mode;
    ctrl->get_id = sensor_get_id;
    ctrl->get_parameters = sensor_get_parameters;
    ctrl->disable_sensor_isp = sensor_disable_isp;
    ctrl->read_sensor_register = read_register;
    ctrl->write_sensor_register = write_register;
    ctrl->start_streaming = start_streaming;
    ctrl->stop_streaming = stop_streaming;
    ctrl->sensor_test_pattern = sensor_test_pattern;
    ctrl->vmax_fps = sensor_vmax_fps;

    // Reset sensor during initialization
    sensor_hw_reset_enable();
    system_timer_usleep( 1000 ); // reset at least 1 ms
    sensor_hw_reset_disable();
    system_timer_usleep( 1000 );

    LOG(LOG_ERR, "%s: Success subdev init\n", __func__);
}

int sensor_detect_sc4238( void* sbp)
{
    int ret = 0;
    sensor_ctx.sbp = sbp;
    sensor_bringup_t* sensor_bp = (sensor_bringup_t*) sbp;
#if PLATFORM_G12B
    ret = clk_am_enable(sensor_bp, "24m");
    if (ret < 0 )
        pr_err("set mclk fail\n");
#elif PLATFORM_C308X
    write1_reg(0xfe000428, 0x11400400);
#elif PLATFORM_C305X
    ret = gp_pl_am_enable(sensor_bp, "mclk_0", 24000000);
    if (ret < 0 )
        pr_info("set mclk fail\n");
#endif

#if NEED_CONFIG_BSP
    ret = reset_am_enable(sensor_bp,"reset", 1);
    if (ret < 0 )
        pr_info("set reset fail\n");
#endif

    sensor_ctx.sbus.mask = SBUS_MASK_SAMPLE_8BITS | SBUS_MASK_ADDR_16BITS | SBUS_MASK_ADDR_SWAP_BYTES;
    sensor_ctx.sbus.control = 0;
    sensor_ctx.sbus.bus = 0;
    sensor_ctx.sbus.device = SENSOR_DEV_ADDRESS;
    acamera_sbus_init( &sensor_ctx.sbus, sbus_i2c );

    ret = 0;
    if (sensor_get_id(&sensor_ctx) == 0xFFFF)
        ret = -1;
    else
        pr_info("sensor_detect_sc4238h:%d\n", ret);

    acamera_sbus_deinit(&sensor_ctx.sbus,  sbus_i2c);
    reset_am_disable(sensor_bp);
    return ret;
}

//*************************************************************************************
