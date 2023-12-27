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

//-------------------------------------------------------------------------------------
//STRUCTURE:
//  VARIABLE SECTION:
//        CONTROLS - dependence from preprocessor
//        DATA     - modulation
//        RESET     - reset function
//        MIPI     - mipi settings
//        FLASH     - flash support
//  CONSTANT SECTION
//        DRIVER
//-------------------------------------------------------------------------------------

#include <linux/delay.h>
#include "acamera_types.h"
#include "sensor_init.h"
#include "acamera_math.h"
#include "system_sensor.h"
#include "acamera_command_api.h"
#include "acamera_sbus_api.h"
#include "acamera_sensor_api.h"
#include "system_timer.h"
#include "acamera_firmware_config.h"
#include "sensor_bus_config.h"
#include "virtcam_config.h"
#include "virtcam_seq.h"
#include "system_am_mipi.h"
#include "system_am_adap.h"
#include "sensor_bsp_common.h"

#define AGAIN_PRECISION 7
#define NEED_CONFIG_BSP 1   //config bsp by sensor driver owner

#define FS_LIN_1080P 1

static void start_streaming( void *ctx );
static void stop_streaming( void *ctx );

static sensor_context_t sensor_ctx;
static exp_offset_t win_offset;

static sensor_mode_t supported_modes[] = {
    {
        .wdr_mode = WDR_MODE_LINEAR,
        .fps = 30 * 256,
        .resolution.width = 1920,
        .resolution.height = 1080,
        .bits = 10,
        .exposures = 1,
        .lanes = 4,
        .bps = 800,
        .bayer = BAYER_BGGR,
        .dol_type = DOL_NON,
        .num = 2,
    },
#if PLATFORM_C305X
    {
        .wdr_mode = WDR_MODE_LINEAR,
        .fps = 30 * 256,
        .resolution.width = 2560,
        .resolution.height = 1440,
        .bits = 10,
        .exposures = 1,
        .lanes = 4,
        .bps = 800,
        .bayer = BAYER_BGGR,
        .dol_type = DOL_NON,
        .num = 4,
    },
    {
        .wdr_mode = WDR_MODE_FS_LIN,
        .fps = 30 * 256,
        .resolution.width = 2560,
        .resolution.height = 1440,
        .bits = 10,
        .exposures = 2,
        .lanes = 4,
        .bps = 1440,
        .bayer = BAYER_BGGR,
        .dol_type = DOL_VC,
        .num = 10,
    },
#else
    {
        .wdr_mode = WDR_MODE_LINEAR,
        .fps = 30 * 256,
        .resolution.width = 3840,
        .resolution.height = 2160,
        .bits = 10,
        .exposures = 1,
        .lanes = 4,
        .bps = 800,
        .bayer = BAYER_BGGR,
        .dol_type = DOL_NON,
        .num = 4,
    },
    {
        .wdr_mode = WDR_MODE_FS_LIN,
        .fps = 30 * 256,
        .resolution.width = 3840,
        .resolution.height = 2160,
        .bits = 10,
        .exposures = 2,
        .lanes = 4,
        .bps = 1440,
        .bayer = BAYER_BGGR,
        .dol_type = DOL_VC,
        .num = 10,
    },
#endif
    {
        .wdr_mode = WDR_MODE_FS_LIN,
        .fps = 30 * 256,
        .resolution.width = 1920,
        .resolution.height = 1080,
        .bits = 10,
        .exposures = 2,
        .lanes = 4,
        .bps = 960,
        .bayer = BAYER_BGGR,
        .dol_type = DOL_VC,
        .num = 13,
    }
};

#if SENSOR_BINARY_SEQUENCE
static const char p_sensor_data[] = SENSOR__VIRTCAM_SEQUENCE_DEFAULT;
static const char p_isp_data[] = SENSOR__VIRTCAM_ISP_SEQUENCE_DEFAULT;
#else
static const acam_reg_t **p_isp_data = isp_seq_table;
#endif
//-------------------------------------------------------------------------------------
static int32_t sensor_alloc_analog_gain( void *ctx, int32_t gain )
{
    sensor_context_t *p_ctx = ctx;
    uint32_t again = acamera_math_exp2( gain, LOG2_GAIN_SHIFT, AGAIN_PRECISION );

    if ( again > p_ctx->again_limit ) again = p_ctx->again_limit;

    if ( p_ctx->again[0] != again ) {
        p_ctx->gain_cnt = p_ctx->again_delay + 1;
        p_ctx->again[0] = again;
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
        if ( *int_time_S > p_ctx->vmax - 8 ) *int_time_S = p_ctx->vmax - 8;
        if ( *int_time_S < 8 ) *int_time_S = 8;
        if ( p_ctx->int_time_S != *int_time_S ) {
            p_ctx->int_cnt = 2;
            p_ctx->int_time_S = *int_time_S;
        }
        break;
    case WDR_MODE_FS_LIN: // DOL2 Frames
        if ( *int_time_S < 8 ) *int_time_S = 8;
        if ( *int_time_S > p_ctx->max_S ) *int_time_S = p_ctx->max_S;
        if ( *int_time_L < 8 ) *int_time_L = 8;
        if ( *int_time_L > ( p_ctx->vmax - *int_time_S ) ) *int_time_L = p_ctx->vmax - *int_time_S;

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
    LOG( LOG_INFO, "exit ir cut" );
    return 0;
}

static void sensor_update( void *ctx )
{
    sensor_context_t *p_ctx = ctx;
    return;
    if ( p_ctx->int_cnt || p_ctx->gain_cnt ) {
        // ---------- Start Changes -------------
        //acamera_sbus_write_u8( p_sbus, 0x0201, 1 );

        // ---------- Analog Gain -------------
        if ( p_ctx->gain_cnt ) {
            switch ( p_ctx->wdr_mode ) {
            case WDR_MODE_LINEAR:
                break;
            case WDR_MODE_FS_LIN:
                break;
            }
            p_ctx->gain_cnt--;
        }

        // -------- Integration Time ----------
        if ( p_ctx->int_cnt ) {
            switch ( p_ctx->wdr_mode ) {
            case WDR_MODE_LINEAR:
                break;
            case WDR_MODE_FS_LIN:
                break;
            }
           p_ctx->int_cnt--;
        }

        // ---------- End Changes -------------
        //acamera_sbus_write_u8( p_sbus, 0x0201, 0 );
    }

    p_ctx->again[3] = p_ctx->again[2];
    p_ctx->again[2] = p_ctx->again[1];
    p_ctx->again[1] = p_ctx->again[0];
}

static uint16_t sensor_get_id( void *ctx )
{
    /* return that sensor id register does not exist */
    return 0;
}

static void sensor_set_mode( void *ctx, uint8_t mode )
{
    sensor_context_t *p_ctx = ctx;
    sensor_param_t *param = &p_ctx->param;
    uint8_t setting_num = param->modes_table[mode].num;

    switch ( param->modes_table[mode].wdr_mode ) {
    case WDR_MODE_LINEAR:
        p_ctx->s_fps = param->modes_table[mode].fps;
        p_ctx->again_delay = 0;
        param->integration_time_apply_delay = 2;
        param->isp_exposure_channel_delay = 0;
        break;
    case WDR_MODE_FS_LIN:
        p_ctx->again_delay = 0;
        param->integration_time_apply_delay = 2;
        param->isp_exposure_channel_delay = 0;

        break;
    default:
        return;
        break;
    }

    param->active.width = param->modes_table[mode].resolution.width;
    param->active.height = param->modes_table[mode].resolution.height;

    if ( param->active.width == 1920 ) {
            p_ctx->s_fps = 30;
            p_ctx->vmax = 2771; // VTS *30/25 -8
            p_ctx->max_S = 170;
            p_ctx->max_L = p_ctx->vmax - p_ctx->max_S;
    } else {
            p_ctx->s_fps = 30;
            p_ctx->vmax = 2768; // VTS-8
            p_ctx->max_S = 170;
            p_ctx->max_L = p_ctx->vmax - p_ctx->max_S;
    }

    /* calculate pixel clock */
    uint32_t VTS = 0;
    uint32_t HTS = 0;
    uint32_t FPS = 0;
    VTS = 3840; // VMAX
    HTS = 2160; //HMAX
    FPS = p_ctx->s_fps;
    p_ctx->pixel_clock = HTS * VTS * FPS;
    param->total.width = 3840;
    param->lines_per_second = p_ctx->pixel_clock / param->total.width;
    param->total.height = (uint16_t)p_ctx->vmax;

    param->pixels_per_line = param->total.width;
    param->integration_time_min = 8;
    if ( param->modes_table[mode].wdr_mode == WDR_MODE_LINEAR ) {
        param->integration_time_limit = p_ctx->vmax;
        param->integration_time_max = p_ctx->vmax;
    } else {
        param->integration_time_limit = p_ctx->max_S;
        param->integration_time_max = p_ctx->max_S;
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
    p_ctx->wdr_mode = param->modes_table[mode].wdr_mode;
    param->bayer = param->modes_table[mode].bayer;

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
    return 0;
}

static void write_register( void *ctx, uint32_t address, uint32_t data )
{
}

static void stop_streaming( void *ctx )
{
    sensor_context_t *p_ctx = ctx;
    p_ctx->streaming_flg = 0;

    reset_sensor_bus_counter();
    sensor_iface_disable();
}

static void start_streaming( void *ctx )
{
    sensor_context_t *p_ctx = ctx;
    sensor_param_t *param = &p_ctx->param;
    adapt_set_virtcam();
    sensor_set_iface(&param->modes_table[param->mode], win_offset);
    p_ctx->streaming_flg = 1;
}

static void sensor_test_pattern( void *ctx, uint8_t mode )
{
}

void sensor_deinit_virtcam( void *ctx )
{
    sensor_context_t *t_ctx = ctx;

    reset_sensor_bus_counter();
    acamera_sbus_deinit(&t_ctx->sbus,  sbus_i2c);
}

static sensor_context_t *sensor_global_parameter(void* sbp)
{
    // Local sensor data structure
    sensor_ctx.sbp = sbp;

    sensor_ctx.sbus.mask = SBUS_MASK_ADDR_16BITS |
           SBUS_MASK_SAMPLE_8BITS |SBUS_MASK_ADDR_SWAP_BYTES;
    sensor_ctx.sbus.control = 0;
    sensor_ctx.sbus.bus = 1;
    sensor_ctx.sbus.device = 0x6c;
    acamera_sbus_init(&sensor_ctx.sbus, sbus_i2c);

    sensor_get_id(&sensor_ctx);

    sensor_ctx.address = 0x6c;
    sensor_ctx.seq_width = 1;
    sensor_ctx.streaming_flg = 0;
    sensor_ctx.again[0] = 0;
    sensor_ctx.again[1] = 0;
    sensor_ctx.again[2] = 0;
    sensor_ctx.again[3] = 0;
    sensor_ctx.again_limit = 2047;
    sensor_ctx.pixel_clock = 184000000;

    sensor_ctx.param.again_accuracy = 1 << LOG2_GAIN_SHIFT;
    sensor_ctx.param.sensor_exp_number = 1;
    sensor_ctx.param.again_log2_max = acamera_log2_fixed_to_fixed( sensor_ctx.again_limit, AGAIN_PRECISION, LOG2_GAIN_SHIFT );
    sensor_ctx.param.dgain_log2_max = 0;
    sensor_ctx.param.integration_time_apply_delay = 2;
    sensor_ctx.param.isp_exposure_channel_delay = 0;
    sensor_ctx.param.modes_table = supported_modes;
    sensor_ctx.param.modes_num = array_size_s( supported_modes );
    sensor_ctx.param.mode = 0;
    sensor_ctx.param.sensor_ctx = &sensor_ctx;
    sensor_ctx.param.isp_context_seq.sequence = p_isp_data;
    sensor_ctx.param.isp_context_seq.seq_num = SENSOR_VIRTCAM_ISP_CONTEXT_SEQ;
    sensor_ctx.param.isp_context_seq.seq_table_max = array_size_s( isp_seq_table );

    memset(&win_offset, 0, sizeof(win_offset));

    return &sensor_ctx;
}

//--------------------Initialization------------------------------------------------------------
void sensor_init_virtcam( void **ctx, sensor_control_t *ctrl, void *sbp )
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

    LOG(LOG_ERR, "%s: Success subdev init\n", __func__);
}

int sensor_detect_virtcam( void* sbp)
{
    int ret = 0;
    sensor_ctx.sbp = sbp;

    sensor_ctx.sbus.mask = SBUS_MASK_SAMPLE_8BITS | SBUS_MASK_ADDR_16BITS | SBUS_MASK_ADDR_SWAP_BYTES;
    sensor_ctx.sbus.control = 0;
    sensor_ctx.sbus.bus = 0;
    sensor_ctx.sbus.device = 0x6c;
    acamera_sbus_init( &sensor_ctx.sbus, sbus_i2c );

    pr_info("sensor_detect_vcam:%d\n", ret);

    acamera_sbus_deinit(&sensor_ctx.sbus,  sbus_i2c);
    return ret;
}
//*************************************************************************************
