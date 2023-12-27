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
#include "IMX224_seq.h"
#include "IMX224_config.h"
#include "acamera_math.h"
#include "system_am_mipi.h"
#include "system_am_adap.h"
#include "sensor_bsp_common.h"

#define NEED_CONFIG_BSP 1

// As per IMX224 datasheet
// Max gain is 72db
// Analog gain is 0 - 30 dB
// Digital gain is applied beyond 30 dB to 72dB
// Gain set in register as (gainIndB * 10)
#define AGAIN_MAX_DB 0x12C // (30 * 10)
#define DGAIN_MAX_DB 0x1A4 // (42 * 10)

static void start_streaming( void *ctx );
static void stop_streaming( void *ctx );

static imx224_private_t imx224_ctx;
static sensor_context_t sensor_ctx;

static sensor_mode_t supported_modes[] = {
    {
        .wdr_mode = WDR_MODE_LINEAR,
        .fps = 30 * 256,
        .resolution.width = 1280,
        .resolution.height = 960,
        .bits = 12,
        .exposures = 1,
        .bps = 297,
        .lanes = 2,
        .num = SENSOR_IMX224_SEQUENCE_QVGA_30FPS_12BIT_2LANES,
        .bayer = BAYER_RGGB,
        .dol_type = DOL_NON,
    },
    {
        .wdr_mode = WDR_MODE_LINEAR,
        .fps = 60 * 256,
        .resolution.width = 1280,
        .resolution.height = 960,
        .bits = 12,
        .exposures = 1,
        .bps = 594,
        .lanes = 2,
        .num = SENSOR_IMX224_SEQUENCE_QVGA_60FPS_12BIT_2LANES,
        .bayer = BAYER_RGGB,
        .dol_type = DOL_NON,
    },
    {
        .wdr_mode = WDR_MODE_FS_LIN,
        .fps = 30 * 256,
        .resolution.width = 1280,
        .resolution.height = 960,
        .bits = 12,
        .exposures = 2,
        .bps = 594,
        .lanes = 2,
        .num = SENSOR_IMX224_SEQUENCE_QVGA_30FPS_12BIT_2LANES_WDR,
        .bayer = BAYER_RGGB,
        .dol_type = DOL_LINEINFO,
    },
    {
        .wdr_mode = WDR_MODE_LINEAR,
        .fps = 30 * 256,
        .resolution.width = 1280,
        .resolution.height = 720,
        .bits = 10,
        .exposures = 1,
        .bps = 594,
        .lanes = 1,
        .num = SENSOR_IMX224_SEQUENCE_720P_30FPS_10BIT_1LANE,
        .bayer = BAYER_RGGB,
        .dol_type = DOL_NON,
    },
};

//-------------------------------------------------------------------------------------
#if SENSOR_BINARY_SEQUENCE
static const char p_sensor_data[] = SENSOR_IMX224_SEQUENCE_DEFAULT;
static const char p_isp_data[] = SENSOR_IMX224_ISP_CONTEXT_SEQ;
#else
static const acam_reg_t **p_sensor_data = imx224_seq_table;
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

//-------------------------------------------------------------------------------------
static int32_t sensor_alloc_analog_gain( void *ctx, int32_t gain )
{
    sensor_context_t *p_ctx = ctx;

    // gain(register) = gain(dB) * 10
    // Input gain is in Log2 format of linear gain with LOG2_GAIN_SHIFT
    // Gain(dB) = (20 * Log10(linear_gain)) = 20 * Log2(linear_gain) * Log10(2)
    //          = 20 * input_gain * 0.3
    // As per specs, analog gain register setting is calculated as
    // gain(register) = 20 * input_gain * 0.3 * 10
    uint16_t again = ( gain * 20 * 3) >> LOG2_GAIN_SHIFT;

    if ( again > p_ctx->again_limit ) again = p_ctx->again_limit;

    if ( p_ctx->again[0] != again ) {
        p_ctx->gain_cnt = p_ctx->again_delay + 1;
        p_ctx->again[0] = again;
    }

    return ( ( (int32_t)again ) << LOG2_GAIN_SHIFT ) / 60;
}

static int32_t sensor_alloc_digital_gain( void *ctx, int32_t gain )
{
    sensor_context_t *p_ctx = ctx;

    // gain(register) = gain(dB) * 10
    // Input gain is in Log2 format of linear gain with LOG2_GAIN_SHIFT
    // Gain(dB) = (20 * Log10(linear_gain)) = 20 * Log2(linear_gain) * Log10(2)
    //          = 20 * input_gain * 0.3
    // As per specs, analog gain register setting is calculated as
    // gain(register) = 20 * input_gain * 0.3 * 10
    uint16_t dgain = ( gain * 20 * 3) >> LOG2_GAIN_SHIFT;

    if ( dgain > p_ctx->dgain_limit ) dgain = p_ctx->dgain_limit;

    if ( p_ctx->dgain[0] != dgain ) {
        p_ctx->dgain_change = 1;
        p_ctx->dgain[0] = dgain;
    }

    return ( ( (int32_t)dgain ) << LOG2_GAIN_SHIFT ) / 60;
}

static void sensor_alloc_integration_time( void *ctx, uint16_t *int_time_S, uint16_t *int_time_M, uint16_t *int_time_L )
{
    sensor_context_t *p_ctx = ctx;
    imx224_private_t *p_imx224 = (imx224_private_t *)p_ctx->sdrv;

    switch ( p_ctx->wdr_mode ) {
    case WDR_MODE_LINEAR: // Normal mode
        if ( *int_time_S > p_ctx->vmax - 2 ) *int_time_S = p_ctx->vmax - 2;
        if ( *int_time_S < 1 ) *int_time_S = 1;
        // Integration time is calculated using following formula
        // Integration time = 1 frame period - (SHS1 + 1) lines
        uint16_t shs1 = p_ctx->vmax - *int_time_S - 1;;
        if ( p_imx224->shs1 != shs1 ) {
            p_ctx->int_cnt = 2;
            p_imx224->shs1 = shs1;
            p_ctx->int_time_S = *int_time_S;
        }
        break;
    case WDR_MODE_FS_LIN: // DOL2 Frames
        if ( *int_time_S < 2 ) *int_time_S = 2;
        if ( *int_time_S > p_ctx->max_S ) *int_time_S = p_ctx->max_S;
        if ( *int_time_L < 2 ) *int_time_L = 2;
        if ( *int_time_L > p_ctx->max_L ) *int_time_L = p_ctx->max_L;

        if ( p_ctx->int_time_S != *int_time_S || p_ctx->int_time_L != *int_time_L ) {
            p_ctx->int_cnt = 2;

            p_ctx->int_time_S = *int_time_S;
            p_ctx->int_time_L = *int_time_L;

            p_imx224->shs2 = p_ctx->frame - *int_time_L - 1;
            p_imx224->shs1 = p_imx224->rhs1 - *int_time_S - 1;
        }

        break;
    }
}

static int32_t sensor_ir_cut_set( void *ctx, int32_t ir_cut_state )
{
    return 0;
}

static void sensor_update( void *ctx )
{
    sensor_context_t *p_ctx = ctx;
    imx224_private_t *p_imx224 = (imx224_private_t *)p_ctx->sdrv;
    acamera_sbus_ptr_t p_sbus = &p_ctx->sbus;

    if ( p_ctx->int_cnt || p_ctx->gain_cnt ) {
        // ---------- Start Changes -------------
        acamera_sbus_write_u8( p_sbus, 0x3001, 1 );

        // ---------- Analog & Digital Gain -------------
        // IMX224 uses same register to update analog and digital gain.
        // After maximum analog gain reached reached, any additional gain
        // applied is treated as digital gain
        // gain (0-72dB) = analog gain (0-30dB) + digital gain (0 - 42dB)
        if ( p_ctx->gain_cnt || p_ctx->dgain_change) {
            uint16_t reg_gain = 0;
            if ( p_ctx->gain_cnt ) {
              reg_gain = p_ctx->again[p_ctx->again_delay];
              p_ctx->gain_cnt--;
            }
            if ( p_ctx->dgain_change ) {
              reg_gain = p_ctx->again[p_ctx->again_delay] + p_ctx->dgain[0];
              p_ctx->dgain_change = 0;
            }
            acamera_sbus_write_u8( p_sbus, 0x3015, (reg_gain >> 8) & 0xFF);
            acamera_sbus_write_u8( p_sbus, 0x3014, reg_gain & 0xFF);
        }

        // -------- Integration Time ----------
        if ( p_ctx->int_cnt ) {
            p_ctx->int_cnt--;
            switch ( p_ctx->wdr_mode ) {
            case WDR_MODE_LINEAR:
                acamera_sbus_write_u8( p_sbus, 0x3022, ( p_imx224->shs1 >> 16 ) & 0x0F );
                acamera_sbus_write_u8( p_sbus, 0x3021, ( p_imx224->shs1 >> 8 ) & 0xFF );
                acamera_sbus_write_u8( p_sbus, 0x3020, p_imx224->shs1 & 0xFF );
                break;
            case WDR_MODE_FS_LIN:
                acamera_sbus_write_u8( p_sbus, 0x3022, ( p_imx224->shs1 >> 16 ) & 0x0F );
                acamera_sbus_write_u8( p_sbus, 0x3021, ( p_imx224->shs1 >> 8 ) & 0xFF );
                acamera_sbus_write_u8( p_sbus, 0x3020, p_imx224->shs1 & 0xFF );
                acamera_sbus_write_u8( p_sbus, 0x3025, ( p_imx224->shs2 >> 16 ) & 0x0F );
                acamera_sbus_write_u8( p_sbus, 0x3024, ( p_imx224->shs2 >> 8 ) & 0xFF );
                acamera_sbus_write_u8( p_sbus, 0x3023, p_imx224->shs2 & 0xFF );
                break;
            }
        }

        // ---------- End Changes -------------
        acamera_sbus_write_u8( p_sbus, 0x3001, 0 );
    }

    p_ctx->again[3] = p_ctx->again[2];
    p_ctx->again[2] = p_ctx->again[1];
    p_ctx->again[1] = p_ctx->again[0];
}

static uint16_t sensor_get_id( void *ctx )
{
    sensor_context_t *p_ctx = ctx;
    uint16_t sensor_id = 0;

    sensor_id |= acamera_sbus_read_u8(&p_ctx->sbus, 0x339a) << 8;
    sensor_id |= acamera_sbus_read_u8(&p_ctx->sbus, 0x3399);

    if (sensor_id != SENSOR_CHIP_ID) {
        LOG(LOG_ERR, "%s: Failed to read sensor id\n", __func__);
        return 0xFFFF;
    }

    LOG(LOG_INFO, "%s: success to read sensor %x\n", __func__, sensor_id);
    return sensor_id;
}

static void sensor_set_mode( void *ctx, uint8_t mode )
{
    sensor_context_t *p_ctx = ctx;
    imx224_private_t *p_imx224 = (imx224_private_t *)p_ctx->sdrv;
    sensor_param_t *param = &p_ctx->param;
    acamera_sbus_ptr_t p_sbus = &p_ctx->sbus;
    uint8_t setting_num = param->modes_table[mode].num;

    sensor_hw_reset_enable();
    system_timer_usleep( 10000 );
    sensor_hw_reset_disable();
    system_timer_usleep( 10000 );

    if (sensor_get_id(ctx) != SENSOR_CHIP_ID) {
        LOG(LOG_INFO, "%s: check sensor failed\n", __func__);
        return;
    }

    LOG(LOG_INFO, "Loading sensor sequence for: mode: %d num%d\n", mode, setting_num);
    switch ( param->modes_table[mode].wdr_mode ) {
    case WDR_MODE_LINEAR:
        sensor_load_sequence( p_sbus, p_ctx->seq_width, p_sensor_data, setting_num );
        p_ctx->s_fps = param->modes_table[mode].fps;
        p_ctx->again_delay = 0;
        param->integration_time_apply_delay = 2;
        param->isp_exposure_channel_delay = 0;
        break;
    case WDR_MODE_FS_LIN:
        sensor_load_sequence( p_sbus, p_ctx->seq_width, p_sensor_data, setting_num );
        p_ctx->s_fps = param->modes_table[mode].fps;
        p_ctx->again_delay = 0;
        param->integration_time_apply_delay = 2;
        param->isp_exposure_channel_delay = 0;
        p_ctx->s_fps = 60;
        break;
    default:
        LOG(LOG_ERR, "Invalid sensor mode. Returning!");
        return;
    }

    uint8_t r = ( acamera_sbus_read_u8( p_sbus, 0x3007 ) >> 4 );
    switch ( r ) {
    case 0: // HD QVGA
        // LEF exposure time = FSC - (SHS2 + 1) = VMAX * 2 - (SHS2 + 1)
        p_ctx->max_L = 2189;
        // SEF exposure time = RHS1 - (SHS1 + 1)
        p_ctx->max_S = 189;
        // RHS1 <= FSC - BRL x 2 - 21
        p_imx224->rhs1 = 193;
        break;
    case 1:
        // HD 720p
        p_ctx->max_L = 1489;
        p_ctx->max_S = 5;
        p_imx224->rhs1 = 9;
        break;
    default:
        // 4- Window cropping from 1080p, Other- Prohibited
        break;
    }

    if (param->modes_table[mode].exposures > 1) {
        acamera_sbus_write_u8( p_sbus, 0x302E, ( p_imx224->rhs1 >> 16 ) & 0x0F );
        acamera_sbus_write_u8( p_sbus, 0x302D, ( p_imx224->rhs1 >> 8 ) & 0xFF );
        acamera_sbus_write_u8( p_sbus, 0x302C, p_imx224->rhs1 & 0xFF );
    }

    param->active.width = param->modes_table[mode].resolution.width;
    param->active.height = param->modes_table[mode].resolution.height;

    param->total.width = ( (uint16_t)acamera_sbus_read_u8( p_sbus, 0x301D ) << 8 ) | acamera_sbus_read_u8( p_sbus, 0x301C );
    param->total.height = ((uint32_t)acamera_sbus_read_u8( p_sbus, 0x3019 ) << 8 ) | acamera_sbus_read_u8( p_sbus, 0x3018 );

    p_ctx->s_fps = param->modes_table[mode].fps >> 8;
    p_ctx->pixel_clock = param->total.width * param->active.height * p_ctx->s_fps;
    p_ctx->vmax = param->total.height;

    param->lines_per_second = p_ctx->pixel_clock / param->total.width;
    param->pixels_per_line = param->total.width;
    param->integration_time_min = SENSOR_MIN_INTEGRATION_TIME;
    if ( param->modes_table[mode].wdr_mode == WDR_MODE_LINEAR ) {
        param->integration_time_limit = SENSOR_MAX_INTEGRATION_TIME_LIMIT;
        param->integration_time_max = p_ctx->vmax - 2;
    } else {
        param->integration_time_limit = 60;
        param->integration_time_max = 60;
        if ( param->modes_table[mode].exposures == 2 ) {
            param->integration_time_long_max = ( p_ctx->vmax << 1 ) - 256;
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

    sensor_set_iface(&param->modes_table[mode], p_ctx->win_offset);

    LOG( LOG_INFO, "Output resolution from sensor: %dx%d wdr_mode: %d", param->active.width, param->active.height, p_ctx->wdr_mode ); // LOG_NOTICE Causes errors in some projects
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
    acamera_sbus_write_u8( p_sbus, 0x3000, 0x01 );

    reset_sensor_bus_counter();
    sensor_iface_disable();
}

static void start_streaming( void *ctx )
{
    sensor_context_t *p_ctx = ctx;
    acamera_sbus_ptr_t p_sbus = &p_ctx->sbus;
    sensor_param_t *param = &p_ctx->param;
    sensor_set_iface(&param->modes_table[param->mode], p_ctx->win_offset);
    p_ctx->streaming_flg = 1;
    acamera_sbus_write_u8( p_sbus, 0x3000, 0x00 );
}

static void sensor_test_pattern( void *ctx, uint8_t mode )
{
	return;
}

void sensor_deinit_imx224( void *ctx )
{
    sensor_context_t *t_ctx = ctx;
    reset_sensor_bus_counter();

    acamera_sbus_deinit(&t_ctx->sbus,  sbus_i2c);
    if (t_ctx != NULL && t_ctx->sbp != NULL)
        clk_am_disable(t_ctx->sbp);
}

static sensor_context_t *sensor_global_parameter(void* sbp)
{
    // Local sensor data structure
    int ret;

    sensor_bringup_t* sensor_bp = (sensor_bringup_t*) sbp;

    ret = pwr_am_enable(sensor_bp, "power-enable", 0);
    if (ret < 0 )
        pr_err("set power fail\n");
    udelay(30);

    ret = reset_am_enable(sensor_bp,"reset", 1);
    if (ret < 0 )
        pr_err("set reset fail\n");

    sensor_ctx.sbp = sbp;
    sensor_ctx.sdrv = &imx224_ctx;

    sensor_ctx.sbus.mask = SBUS_MASK_SAMPLE_8BITS | SBUS_MASK_ADDR_16BITS | SBUS_MASK_ADDR_SWAP_BYTES;
    sensor_ctx.sbus.control = 0;
    sensor_ctx.sbus.bus = 1;
    sensor_ctx.sbus.device = SENSOR_DEV_ADDRESS;
    acamera_sbus_init( &sensor_ctx.sbus, sbus_i2c );

    // Initial local parameters
    sensor_ctx.address = SENSOR_DEV_ADDRESS;
    sensor_ctx.seq_width = 1;
    sensor_ctx.streaming_flg = 0;
    memset(sensor_ctx.again, 0, sizeof(uint16_t) * 4);
    memset(sensor_ctx.dgain, 0, sizeof(uint16_t) * 4);
    sensor_ctx.dgain_change = 0;
    sensor_ctx.again_limit = AGAIN_MAX_DB;
    sensor_ctx.dgain_limit = DGAIN_MAX_DB;
    sensor_ctx.pixel_clock = 148500000;

    sensor_ctx.param.again_accuracy = 1 << LOG2_GAIN_SHIFT;
    sensor_ctx.param.sensor_exp_number = 1;
    sensor_ctx.param.again_log2_max = AGAIN_MAX_DB << LOG2_GAIN_SHIFT;
    sensor_ctx.param.dgain_log2_max = DGAIN_MAX_DB << LOG2_GAIN_SHIFT;
    sensor_ctx.param.integration_time_apply_delay = 2;
    sensor_ctx.param.isp_exposure_channel_delay = 0;
    sensor_ctx.param.modes_table = supported_modes;
    sensor_ctx.param.modes_num = array_size_s( supported_modes );
    sensor_ctx.param.sensor_ctx = &sensor_ctx;
    sensor_ctx.param.isp_context_seq.sequence = p_isp_data;
    sensor_ctx.param.isp_context_seq.seq_num= SENSOR_IMX224_CONTEXT_SEQ;

    memset(&sensor_ctx.win_offset, 0, sizeof(sensor_ctx.win_offset));
    sensor_ctx.win_offset.long_offset = 8;
    sensor_ctx.win_offset.short_offset = 8;

    return &sensor_ctx;
}

//--------------------Initialization------------------------------------------------------------
void sensor_init_imx224( void **ctx, sensor_control_t *ctrl, void *sbp)
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

    // Reset sensor during initialization
    sensor_hw_reset_enable();
    system_timer_usleep( 1000 ); // reset at least 1 ms
    sensor_hw_reset_disable();
    system_timer_usleep( 1000 );
}

int sensor_detect_imx224( void* sbp)
{
    int ret = 0;

    sensor_ctx.sbus.mask = SBUS_MASK_SAMPLE_8BITS | SBUS_MASK_ADDR_16BITS | SBUS_MASK_ADDR_SWAP_BYTES;
    sensor_ctx.sbus.control = 0;
    sensor_ctx.sbus.bus = 0;
    sensor_ctx.sbus.device = SENSOR_DEV_ADDRESS;
    acamera_sbus_init( &sensor_ctx.sbus, sbus_i2c );

    ret = 0;
    if (sensor_get_id(&sensor_ctx) == 0xFFFF)
        ret = -1;
    else
        pr_info("sensor_detect_imx224:%d\n", sensor_get_id(&sensor_ctx));

    acamera_sbus_deinit(&sensor_ctx.sbus,  sbus_i2c);

    return ret;
}

//********************CONSTANT SECTION END*********************************************
//*************************************************************************************
