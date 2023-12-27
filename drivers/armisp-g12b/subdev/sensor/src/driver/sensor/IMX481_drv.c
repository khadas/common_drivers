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
#include "acamera_logger.h"
#include "acamera_math.h"
#include "acamera_command_api.h"
#include "acamera_sbus_api.h"
#include "system_sensor.h"
#include "acamera_sensor_api.h"
#include "system_timer.h"
#include "acamera_firmware_config.h"
#include "sensor_bus_config.h"
#include "IMX481_seq.h"
#include "IMX481_config.h"
#include "system_am_mipi.h"
#include "system_am_adap.h"
#include "sensor_bsp_common.h"

#define AGAIN_PRECISION 12
#define DGAIN_PRECISION 8
#define NEED_CONFIG_BSP 1   //config bsp by sensor driver owner

#define VBLANK_30FPS 0x0CB6 // 3254 lines

static void start_streaming( void *ctx );
static void stop_streaming( void *ctx );

static imx481_private_t imx481_ctx;
static sensor_context_t sensor_ctx;

static sensor_mode_t supported_modes[] = {
    {
        .wdr_mode = WDR_MODE_LINEAR,
        .fps = 30 * 256,
        .resolution.width = 2328,
        .resolution.height = 1748,
        .bits = 10,
        .exposures = 1,
        .bps = 446,
        .lanes = 4,
        .num = 1,
        .bayer = BAYER_RGGB,
        .dol_type = DOL_NON,
    },
    {
        .wdr_mode = WDR_MODE_LINEAR,
        .fps = 30 * 256,
        .resolution.width = 1920,
        .resolution.height = 1080,
        .bits = 10,
        .exposures = 1,
        .bps = 446,
        .lanes = 4,
        .num = 2,
        .bayer = BAYER_RGGB,
        .dol_type = DOL_NON,
    },
    {
        .wdr_mode = WDR_MODE_LINEAR,
        .fps = 24 * 256,
        .resolution.width = 1920,
        .resolution.height = 1080,
        .bits = 10,
        .exposures = 1,
        .bps = 446,
        .lanes = 4,
        .num = 3,
        .bayer = BAYER_RGGB,
        .dol_type = DOL_NON,
    },
    {
        .wdr_mode = WDR_MODE_LINEAR,
        .fps = 30 * 256,
        .resolution.width = 1280,
        .resolution.height = 720,
        .bits = 10,
        .exposures = 1,
        .bps = 446,
        .lanes = 4,
        .num = 4,
        .bayer = BAYER_RGGB,
        .dol_type = DOL_NON,
    },
};

static int ctx_counter = 0;
static uint32_t reset_state = 0;

//-------------------------------------------------------------------------------------
#if SENSOR_BINARY_SEQUENCE
static const char p_sensor_data[] = SENSOR_IMX481_SEQUENCE_DEFAULT;
static const char p_isp_data[] = SENSOR_IMX481_ISP_CONTEXT_SEQUENCE;
#else
static const acam_reg_t **p_sensor_data = seq_table;
static const acam_reg_t **p_isp_data = isp_seq_table;

#endif
//--------------------DATA-------------------------------------------------------------


//*************************************************************************************
//********************VARIABLE SECTION START*******************************************

//--------------------RESET------------------------------------------------------------
static void sensor_hw_reset_enable( void )
{
    reset_state &= ~( 1 << ( 2 * ( ctx_counter - 1 ) ) );
    system_reset_sensor( reset_state );
    system_timer_usleep( 10000 );
}

static void sensor_hw_reset_disable( void )
{
    reset_state |= 1 << ( 2 * ( ctx_counter - 1 ) );
    system_reset_sensor( reset_state );
    system_timer_usleep( 10000 );
}


//********************VARIABLE SECTION END*********************************************
//*************************************************************************************


//*************************************************************************************
//********************CONSTANT SECTION START*******************************************
static int32_t sensor_alloc_analog_gain( void *ctx, int32_t gain )
{
    sensor_context_t *p_ctx = ctx;
    imx481_private_t *p_imx481 = (imx481_private_t *)p_ctx->sdrv;

    uint32_t again = acamera_math_exp2( gain, LOG2_GAIN_SHIFT, AGAIN_PRECISION );
    uint32_t a_gain;

    if ( again > p_ctx->again_limit ) again = p_ctx->again_limit;

    a_gain = 256 - ( ( 256 << AGAIN_PRECISION ) / again );

    p_imx481->again_change = 1;
    p_ctx->again[0] = a_gain;

    return acamera_log2_fixed_to_fixed( again, AGAIN_PRECISION, LOG2_GAIN_SHIFT );
}

static int32_t sensor_alloc_digital_gain( void *ctx, int32_t gain )
{
    sensor_context_t *p_ctx = ctx;
    imx481_private_t *p_imx481 = (imx481_private_t *)p_ctx->sdrv;

    uint32_t dgain = acamera_math_exp2( gain, LOG2_GAIN_SHIFT, DGAIN_PRECISION );
    uint32_t d_gain = dgain;

    if ( dgain > p_ctx->dgain_limit ) dgain = p_ctx->dgain_limit;

    p_imx481->dgain_change = 1;
    p_ctx->dgain[0] = d_gain;

    return acamera_log2_fixed_to_fixed( dgain, DGAIN_PRECISION, LOG2_GAIN_SHIFT );
}

static void sensor_alloc_integration_time( void *ctx, uint16_t *int_time, uint16_t *int_time_M, uint16_t *int_time_L )
{
    sensor_context_t *p_ctx = ctx;
    imx481_private_t *p_imx481 = (imx481_private_t *)p_ctx->sdrv;

    if ( *int_time > p_ctx->max_S ) *int_time = p_ctx->max_S;
    if ( *int_time < 1 ) *int_time = 1;


    if ( p_ctx->int_time_S != *int_time ) {
        p_imx481->change_flg = 1;
        p_ctx->int_time_S = *int_time;
    }
}

static int32_t sensor_ir_cut_set( void *ctx, int32_t ir_cut_state )
{
    return 0;
}

static void sensor_update( void *ctx )
{
    sensor_context_t *p_ctx = ctx;
    imx481_private_t *p_imx481 = (imx481_private_t *)p_ctx->sdrv;
    acamera_sbus_ptr_t p_sbus = &p_ctx->sbus;

    if ( p_imx481->change_flg || p_imx481->again_change || p_imx481->dgain_change ) {
        acamera_sbus_write_u8( p_sbus, 0x104, 0x01 );
        if ( p_imx481->again_change ) {
            // ---------- Analog Gain -------------
            acamera_sbus_write_u8( p_sbus, 0x205, ( uint8_t )( p_ctx->again[p_ctx->again_delay] ) );
            p_imx481->again_change--;
        }

        if ( p_imx481->dgain_change ) {
            // --------- Digital Gain -------------
            acamera_sbus_write_u8( p_sbus, 0x20e, ( uint8_t )( p_ctx->dgain[p_ctx->dgain_delay] >> 8 ) );
            acamera_sbus_write_u8( p_sbus, 0x20f, (uint8_t)p_ctx->dgain[p_ctx->dgain_delay] & 0xFF );
            p_imx481->dgain_change--;
        }

        if ( p_imx481->change_flg ) {
            // -------- Integration Time ----------
            acamera_sbus_write_u8( p_sbus, 0x202, ( uint8_t )( p_ctx->int_time_S >> 8 ) );
            acamera_sbus_write_u8( p_sbus, 0x203, ( uint8_t )( p_ctx->int_time_S & 0xFF ) );
            p_imx481->change_flg = 0;
        }

        acamera_sbus_write_u8( p_sbus, 0x104, 0x00 );
    }
    p_ctx->again[1] = p_ctx->again[0];
    p_ctx->dgain[1] = p_ctx->dgain[0];
}

static uint16_t sensor_get_id( void *ctx )
{
    sensor_context_t *p_ctx = ctx;
    uint32_t sensor_id = 0;

    sensor_id |= acamera_sbus_read_u8(&p_ctx->sbus, 0x0016) << 8;
    sensor_id |= acamera_sbus_read_u8(&p_ctx->sbus, 0x0017);

    if (sensor_id != SENSOR_CHIP_ID) {
        LOG(LOG_ERR, "%s: Failed to read sensor id\n", __func__);
        return 0xFFFF;
    }

    return sensor_id;
}

static void sensor_set_mode( void *ctx, uint8_t mode )
{
    sensor_context_t *p_ctx = ctx;
    sensor_param_t *param = &p_ctx->param;
    acamera_sbus_ptr_t p_sbus = &p_ctx->sbus;
    uint8_t setting_num = param->modes_table[mode].num;

    sensor_hw_reset_enable();
    system_timer_usleep( 10000 );
    sensor_hw_reset_disable();
    system_timer_usleep( 10000 );

    p_ctx->again[0] = 0;
    p_ctx->int_time_S = 0;

    if (sensor_get_id(ctx) != SENSOR_CHIP_ID) {
        LOG(LOG_INFO, "%s: check sensor failed\n", __func__);
        return;
    }

    switch ( param->modes_table[mode].wdr_mode ) {
    case WDR_MODE_LINEAR:
        sensor_load_sequence( p_sbus, p_ctx->seq_width, p_sensor_data, setting_num);
        p_ctx->again_delay = 0;
        p_ctx->dgain_delay = 0;
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

    param->total.width = ( acamera_sbus_read_u8( p_sbus, 0x342 ) << 8 ) | acamera_sbus_read_u8( p_sbus, 0x343 );
    param->total.height = ( acamera_sbus_read_u8( p_sbus, 0x340 ) << 8 ) | acamera_sbus_read_u8( p_sbus, 0x341 );
    p_ctx->max_S = VBLANK_30FPS;

    param->pixels_per_line = param->total.width;
    param->lines_per_second = 288000000 / param->total.width;
    param->integration_time_min = 1;
    param->integration_time_limit = p_ctx->max_S;
    param->integration_time_max = p_ctx->max_S;
    param->integration_time_long_max = p_ctx->max_S;
    param->mode = mode;
    param->bayer = param->modes_table[mode].bayer;
    p_ctx->wdr_mode = param->modes_table[mode].wdr_mode;

    sensor_set_iface(&param->modes_table[mode], p_ctx->win_offset);

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
    uint32_t val = acamera_sbus_read_u8( p_sbus, address );
    return val;
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
    acamera_sbus_write_u8(p_sbus, 0x0100, 0x00);
    LOG(LOG_ERR, "%s: Stream Off\n", __func__);
}

static void start_streaming( void *ctx )
{
    sensor_context_t *p_ctx = ctx;
    acamera_sbus_ptr_t p_sbus = &p_ctx->sbus;
    p_ctx->streaming_flg = 1;
    acamera_sbus_write_u8(p_sbus, 0x0100, 0x01);
    LOG(LOG_ERR, "%s: Stream On\n", __func__);
}

static void sensor_test_pattern( void *ctx, uint8_t mode )
{
}

void sensor_deinit_imx481( void *ctx )
{
    sensor_context_t *t_ctx = ctx;

    reset_sensor_bus_counter();

    sensor_iface_disable();

    acamera_sbus_deinit(&t_ctx->sbus, sbus_i2c);

    if (t_ctx != NULL && t_ctx->sbp != NULL)
        clk_am_disable(t_ctx->sbp);
}

static sensor_context_t *sensor_global_parameter(void* sbp)
{
    // Local sensor data structure
    int ret;
    sensor_bringup_t* sensor_bp = (sensor_bringup_t*) sbp;

#if NEED_CONFIG_BSP
    ret = pwr_am_enable(sensor_bp, "power-enable", 1);
    if (ret < 0 )
        pr_err("set power fail\n");
    udelay(30);
#endif

#if PLATFORM_G12B
    ret = clk_am_enable(sensor_bp, "g12a_24m");
    if (ret < 0 )
        pr_err("set mclk fail\n");
#elif PLATFORM_C305X
    ret = gp_pl_am_enable(sensor_bp, "mclk_0", 24000000);
    if (ret < 0 )
        pr_info("set mclk fail\n");
#endif

#if NEED_CONFIG_BSP
    reset_am_enable(sensor_bp,"reset", 1);
    if (ret < 0 )
        pr_err("set reset fail\n");
#endif
    sensor_ctx.sbp = sbp;
    sensor_ctx.sdrv = &imx481_ctx;

    sensor_ctx.sbus.mask = SBUS_MASK_ADDR_16BITS |
                        SBUS_MASK_SAMPLE_8BITS |SBUS_MASK_ADDR_SWAP_BYTES;
    sensor_ctx.sbus.control = I2C_CONTROL_MASK;
    sensor_ctx.sbus.bus = 1;//get_next_sensor_bus_address();
    sensor_ctx.sbus.device = SENSOR_DEV_ADDRESS;
    acamera_sbus_init(&sensor_ctx.sbus, sbus_i2c);

    sensor_ctx.address = SENSOR_DEV_ADDRESS;
    sensor_ctx.seq_width = 1;
    sensor_ctx.streaming_flg = 0;
    imx481_ctx.again_old = 0;
    imx481_ctx.change_flg = 0;
    sensor_ctx.again_limit = 8 << AGAIN_PRECISION;
    sensor_ctx.dgain_limit = 15 << DGAIN_PRECISION;

    sensor_ctx.param.again_accuracy = 1 << LOG2_GAIN_SHIFT;
    sensor_ctx.param.sensor_exp_number = SENSOR_EXP_NUMBER;
    sensor_ctx.param.again_log2_max = 3 << LOG2_GAIN_SHIFT;
    sensor_ctx.param.dgain_log2_max = 3 << LOG2_GAIN_SHIFT;
    sensor_ctx.param.integration_time_apply_delay = 2;
    sensor_ctx.param.isp_exposure_channel_delay = 0;
    sensor_ctx.param.modes_table = supported_modes;
    sensor_ctx.param.modes_num = array_size_s( supported_modes );
    sensor_ctx.param.mode = 0;
    sensor_ctx.param.sensor_ctx = &sensor_ctx;
    sensor_ctx.param.isp_context_seq.sequence = p_isp_data;
    sensor_ctx.param.isp_context_seq.seq_num= SENSOR_IMX481_CONTEXT_SEQ;
    sensor_ctx.param.isp_context_seq.seq_table_max = array_size_s( isp_seq_table );

    memset(&sensor_ctx.win_offset, 0, sizeof(sensor_ctx.win_offset));

    return &sensor_ctx;
}

//--------------------Initialization------------------------------------------------------------
void sensor_init_imx481( void **ctx, sensor_control_t *ctrl, void* sbp)
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

    LOG(LOG_ERR, "%s: Success subdev init\n", __func__);
}

int sensor_detect_imx481( void* sbp)
{
    int ret = 0;
    sensor_bringup_t* sensor_bp = (sensor_bringup_t*) sbp;
    sensor_ctx.sbp = sbp;

#if PLATFORM_G12B
    ret = clk_am_enable(sensor_bp, "g12a_24m");
    if (ret < 0 )
        pr_err("set mclk fail\n");
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
        pr_info("sensor_detect_imx481:%d\n", ret);

    acamera_sbus_deinit(&sensor_ctx.sbus,  sbus_i2c);
    reset_am_disable(sensor_bp);
    return ret;
}

//*************************************************************************************
