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
#include "OV2718_seq.h"
#include "OV2718_config.h"
#include "system_am_mipi.h"
#include "system_am_adap.h"
#include "sensor_bsp_common.h"

#define AGAIN_PRECISION 10
#define NEED_CONFIG_BSP 1   //config bsp by sensor driver owner

#define FS_LIN_1080P 1
static void start_streaming( void *ctx );
static void stop_streaming( void *ctx );

static sensor_context_t sensor_ctx;

static int count = 0;

static sensor_mode_t supported_modes[] = {
    {
        .wdr_mode = WDR_MODE_LINEAR,
        .fps = 30 * 256,
        .resolution.width = 1920,
        .resolution.height = 1080,
        .bits = 12,
        .exposures = 1,
        .lanes = 4,
        .bps = 960,
        .bayer = BAYER_GBRG,
        .dol_type = DOL_NON,
        .num = 0,
    },
    {
        .wdr_mode = WDR_MODE_LINEAR,
        .fps = 25 * 256,
        .resolution.width = 1920,
        .resolution.height = 1080,
        .bits = 12,
        .exposures = 1,
        .lanes = 4,
        .bps = 960,
        .bayer = BAYER_GBRG,
        .dol_type = DOL_NON,
        .num = 0,
    },
    {
        .wdr_mode = WDR_MODE_FS_LIN,
        .fps = 30 * 256,
        .resolution.width = 1920,
        .resolution.height = 1080,
        .bits = 12,
        .exposures = 2,
        .lanes = 4,
        .bps = 960,
        .bayer = BAYER_GBRG,
        .dol_type = DOL_VC,
        .num = 4,
    },
    {
        .wdr_mode = WDR_MODE_FS_LIN,
        .fps = 25 * 256,
        .resolution.width = 1920,
        .resolution.height = 1080,
        .bits = 12,
        .exposures = 2,
        .lanes = 4,
        .bps = 960,
        .bayer = BAYER_GBRG,
        .dol_type = DOL_VC,
        .num = 4,
    },
};

#if SENSOR_BINARY_SEQUENCE
static const char p_sensor_data[] = SENSOR__OV2718_SEQUENCE_DEFAULT;
static const char p_isp_data[] = SENSOR__OV2718_ISP_SEQUENCE_DEFAULT;
#else
static const acam_reg_t **p_sensor_data = seq_table;
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
        if ( *int_time_S > p_ctx->param.integration_time_max ) *int_time_S = p_ctx->param.integration_time_max;
        if ( *int_time_S < p_ctx->param.integration_time_min ) *int_time_S = p_ctx->param.integration_time_min;
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
        //if ( *int_time_L > p_ctx->max_L ) *int_time_L = p_ctx->max_L;

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

    LOG( LOG_ERR, "ir_cut_state = %d", ir_cut_state);
    LOG( LOG_INFO, "entry ir cut" );

    //ir_cut_GPIOZ_11, 0: open ir cut, 1: colse ir cut, 2: no operation

   if (sensor_bp->ir_gname[0] <= 0) {
       pr_err("get gpio id fail\n");
       return 0;
    }

    if (ir_cut_state == 1) {
        ret = pwr_ir_cut_enable(sensor_bp, sensor_bp->ir_gname[0], 0);
        if (ret < 0 )
            pr_err("set power fail\n");
    } else if(ir_cut_state == 0) {
        ret = pwr_ir_cut_enable(sensor_bp, sensor_bp->ir_gname[0], 1);
        if (ret < 0 )
            pr_err("set power fail\n");
    }

    LOG( LOG_INFO, "exit ir cut" );

    return 0;
}

static void sensor_update( void *ctx )
{
    sensor_context_t *p_ctx = ctx;
    acamera_sbus_ptr_t p_sbus = &p_ctx->sbus;
    //uint8_t u8Reg0x30bb = acamera_sbus_read_u8(&p_ctx->sbus, 0x30bb);
    uint16_t total_gain = p_ctx->again[p_ctx->again_delay];
    uint16_t again = 0;
    int16_t LCG_dgain_h = 0;
    uint16_t LCG_dgain_l = 0;

    if ( p_ctx->int_cnt || p_ctx->gain_cnt ) {
        // ---------- Start Changes -------------

        // ---------- Analog Gain -------------
        if ( p_ctx->gain_cnt ) {
#if 1
        if (p_ctx->wdr_mode == WDR_MODE_FS_LIN) {
            total_gain = total_gain - 0x100;
            if (total_gain < 0x400 * 2) { //<1x -- 2x
                again = 0;
                LCG_dgain_h = ( total_gain >> 8) & 0xFF;
                LCG_dgain_l = ( total_gain >> 0) & 0xFF;
            } else if ((total_gain > 0x400 * 2) && (total_gain < 0x400 * 4)) { //<2x -- 4x
                again = 0x14;
                LCG_dgain_h = ((total_gain/2) >> 8) & 0xFF;
                LCG_dgain_l = ((total_gain/2) >> 0) & 0xFF;
            } else if ( (total_gain > 0x400 * 4) && (total_gain < 0x400 * 8)) { //<4x -- 8x
                again = 0x28;
                LCG_dgain_h = ((total_gain/4) >> 8) & 0xFF;
                LCG_dgain_l = ((total_gain/4) >> 0) & 0xFF;
            } else if (total_gain > 0x400 * 8) { //8x
                again = 0x3c;
                LCG_dgain_h = ((total_gain/8) >> 8) & 0xFF;
                LCG_dgain_l = ((total_gain/8) >> 0) & 0xFF;
            }
        } else {
            again = acamera_sbus_read_u8(&p_ctx->sbus, 0x30bb);
            if (total_gain < 0x400 * 2) { //<1x -- 2x
                again &= 0xFC;
                LCG_dgain_h = (total_gain >> 8) & 0xFF;
                LCG_dgain_l = (total_gain >> 0) & 0xFF;
            }
            else if ( (total_gain > 0x400 * 2) && (total_gain < 0x400 * 4)) { //<2x -- 4x
                again |= 0x01;
                LCG_dgain_h = ((total_gain/2) >> 8) & 0xFF;
                LCG_dgain_l = ((total_gain/2) >> 0) & 0xFF;
            }
            else if ( (total_gain > 0x400 * 4) && (total_gain < 0x400 * 8)) { //<4x -- 8x
                again |= 0x02;
                LCG_dgain_h = ((total_gain/4) >> 8) & 0xFF;
                LCG_dgain_l = ((total_gain/4) >> 0) & 0xFF;
            }
            else if (total_gain > 0x400 * 8) { //8x
                again |= 0x03;
                LCG_dgain_h = ((total_gain/8) >> 8) & 0xFF;
                LCG_dgain_l = ((total_gain/8) >> 0) & 0xFF;
            }
        }
#endif
            if (count ++ == 30) {
                pr_info("gain:%x,%x,%x,%x", again, total_gain,LCG_dgain_h, LCG_dgain_l);
                count = 0;
            }
            acamera_sbus_write_u8( p_sbus, 0x30bb, again);
            acamera_sbus_write_u8( p_sbus, 0x315a, LCG_dgain_h);
            acamera_sbus_write_u8( p_sbus, 0x315b, LCG_dgain_h);
            acamera_sbus_write_u8( p_sbus, 0x315c, LCG_dgain_h);
            acamera_sbus_write_u8( p_sbus, 0x315d, LCG_dgain_l);
            acamera_sbus_write_u8( p_sbus, 0x315e, LCG_dgain_h);
            acamera_sbus_write_u8( p_sbus, 0x315f, LCG_dgain_l);
            p_ctx->gain_cnt--;
        }

        // -------- Integration Time ----------
        if ( p_ctx->int_cnt ) {
            switch ( p_ctx->wdr_mode ) {
            case WDR_MODE_LINEAR:
                acamera_sbus_write_u8( p_sbus, 0x30b6, ( p_ctx->int_time_S >> 8 ) & 0xFF );
                acamera_sbus_write_u8( p_sbus, 0x30b7, ( p_ctx->int_time_S >> 0 ) & 0xFF );
                break;
            case WDR_MODE_FS_LIN:

                // SHS1
                acamera_sbus_write_u8( p_sbus, 0x30b6, ( p_ctx->int_time_L >> 8 ) & 0xFF );
                acamera_sbus_write_u8( p_sbus, 0x30b7, ( p_ctx->int_time_L >> 0 ) & 0xFF );

                // SHS2
                acamera_sbus_write_u8( p_sbus, 0x30b8, ( p_ctx->int_time_S >> 8 ) & 0xFF );
                acamera_sbus_write_u8( p_sbus, 0x30b9, ( p_ctx->int_time_S >> 0 ) & 0xFF );
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

    sensor_context_t *p_ctx = ctx;
    uint32_t sensor_id = 0;

    sensor_id |= acamera_sbus_read_u8(&p_ctx->sbus, 0x300a) << 8;
    sensor_id |= acamera_sbus_read_u8(&p_ctx->sbus, 0x300b);

    if (sensor_id != SENSOR_CHIP_ID) {
        LOG(LOG_CRIT, "%s: Failed to read sensor ov2718 id: %x\n", __func__, sensor_id);
        return 0xFFFF;
    }

    LOG(LOG_CRIT, "%s: success to read sensor ov2718: %x\n", __func__, sensor_id);
    return SENSOR_CHIP_ID;
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

    if (sensor_get_id(ctx) != SENSOR_CHIP_ID) {
        LOG(LOG_INFO, "%s: check sensor failed\n", __func__);
        return;
    }

    switch ( param->modes_table[mode].wdr_mode ) {
    case WDR_MODE_LINEAR:
        sensor_load_sequence( p_sbus, p_ctx->seq_width, p_sensor_data, setting_num);
        sensor_load_sequence( p_sbus, p_ctx->seq_width, p_sensor_data, setting_num + 1);
        p_ctx->s_fps = param->modes_table[mode].fps;
        p_ctx->again_delay = 0;
        param->integration_time_apply_delay = 2;
        param->isp_exposure_channel_delay = 0;
        break;
    case WDR_MODE_FS_LIN:
        p_ctx->again_delay = 0;
        param->integration_time_apply_delay = 2;
        param->isp_exposure_channel_delay = 0;

        if ( param->modes_table[mode].exposures == 2 ) {
            sensor_load_sequence( p_sbus, p_ctx->seq_width, p_sensor_data, setting_num);
            sensor_load_sequence( p_sbus, p_ctx->seq_width, p_sensor_data, setting_num + 1);
        } else {

        }
        break;
    default:
        return;
        break;
    }

    if ((param->modes_table[mode].exposures == 1) && (param->modes_table[mode].fps == 25 * 256)) {
        acamera_sbus_write_u8( p_sbus, 0x30b2, 0x05 ); //0x547 = 1351
        acamera_sbus_write_u8( p_sbus, 0x30b3, 0x47 );
        p_ctx->s_fps = 25;
        p_ctx->vmax = 1351; // VTS *30/25
    } else if ((param->modes_table[mode].exposures == 2) && (param->modes_table[mode].fps == 30 * 256)) {
        p_ctx->s_fps = 30;
        p_ctx->vmax = (((uint32_t)acamera_sbus_read_u8(p_sbus,0x30b2)<<8)|acamera_sbus_read_u8(p_sbus,0x30b3));
        p_ctx->max_S = 120;//p_ctx->vmax - 2;
        p_ctx->max_L = p_ctx->vmax - p_ctx->max_S;
    } else if ((param->modes_table[mode].exposures == 2) && (param->modes_table[mode].fps == 25 * 256)) {
        p_ctx->s_fps = 30;
        p_ctx->vmax = 1351 ;
        acamera_sbus_write_u8( p_sbus, 0x30b2, 0x05 ); //0x547 = 1351
        acamera_sbus_write_u8( p_sbus, 0x30b3, 0x47 );
        p_ctx->max_S = p_ctx->vmax - 2;
        p_ctx->max_L = p_ctx->max_S * 8;
    } else {
        p_ctx->vmax = (((uint32_t)acamera_sbus_read_u8(p_sbus,0x30b2)<<8)|acamera_sbus_read_u8(p_sbus,0x30b3));
    }
    param->active.width = param->modes_table[mode].resolution.width;
    param->active.height = param->modes_table[mode].resolution.height;

    param->total.width = ( (uint16_t)acamera_sbus_read_u8( p_sbus, 0x30b0 ) << 8 ) | acamera_sbus_read_u8( p_sbus, 0x30b1 );
    param->lines_per_second = p_ctx->pixel_clock / param->total.width;
    param->total.height = (uint16_t)p_ctx->vmax;

    param->pixels_per_line = param->total.width;
    param->integration_time_min = 2;
    if ( param->modes_table[mode].wdr_mode == WDR_MODE_LINEAR ) {
        param->integration_time_limit = p_ctx->vmax - 2;
        param->integration_time_max = p_ctx->vmax - 2;
    } else {
        param->integration_time_limit = p_ctx->max_S;
        param->integration_time_max = p_ctx->max_S;
        if ( param->modes_table[mode].exposures == 2 ) {
            param->integration_time_long_max = p_ctx->max_L;
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

    acamera_sbus_write_u8(p_sbus, 0x0100, 0x00);

    reset_sensor_bus_counter();
    sensor_iface_disable(p_ctx);
}

static void start_streaming( void *ctx )
{
    sensor_context_t *p_ctx = ctx;
    acamera_sbus_ptr_t p_sbus = &p_ctx->sbus;
    sensor_param_t *param = &p_ctx->param;
    sensor_set_iface(&param->modes_table[param->mode], p_ctx->win_offset, p_ctx);
    p_ctx->streaming_flg = 1;
    acamera_sbus_write_u8(p_sbus, 0x0100, 0x01);
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

void sensor_deinit_ov2718( void *ctx )
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
        pr_err("set reset fail\n");
#endif

    sensor_ctx.sbp = sbp;

    sensor_ctx.sbus.mask = SBUS_MASK_ADDR_16BITS |
           SBUS_MASK_SAMPLE_8BITS |SBUS_MASK_ADDR_SWAP_BYTES;
    sensor_ctx.sbus.control = I2C_CONTROL_MASK;
    sensor_ctx.sbus.bus = 0;
    sensor_ctx.sbus.device = SENSOR_DEV_ADDRESS;
    acamera_sbus_init(&sensor_ctx.sbus, sbus_i2c);

    sensor_ctx.address = SENSOR_DEV_ADDRESS;
    sensor_ctx.seq_width = 1;
    sensor_ctx.streaming_flg = 0;
    sensor_ctx.again[0] = 0;
    sensor_ctx.again[1] = 0;
    sensor_ctx.again[2] = 0;
    sensor_ctx.again[3] = 0;
    sensor_ctx.again_limit = 256*256;
    sensor_ctx.pixel_clock = 120000000;

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
    sensor_ctx.param.isp_context_seq.seq_num = SENSOR_OV2718_ISP_CONTEXT_SEQ;
    sensor_ctx.param.isp_context_seq.seq_table_max = array_size_s( isp_seq_table );
    sensor_ctx.cam_isp_path = CAM0_ACT;
    sensor_ctx.dcam_mode = 0;

    memset(&sensor_ctx.win_offset, 0, sizeof(sensor_ctx.win_offset));

    return &sensor_ctx;
}

//--------------------Initialization------------------------------------------------------------
void sensor_init_ov2718( void **ctx, sensor_control_t *ctrl, void *sbp )
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
#include <linux/delay.h>

int sensor_detect_ov2718( void* sbp)
{
    int ret = 0;
    sensor_ctx.sbp = sbp;
    sensor_bringup_t* sensor_bp = (sensor_bringup_t*) sbp;

#if NEED_CONFIG_BSP
    ret = pwr_am_enable(sensor_bp,"pwdn", 1);
    if (ret < 0 )
        pr_err("set reset fail\n");
    system_timer_usleep(20000);

    ret = reset_am_enable(sensor_bp,"reset", 1);
    if (ret < 0 )
        pr_err("set reset fail\n");
#endif
     system_timer_usleep(1000);

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

    sensor_ctx.sbus.mask = SBUS_MASK_SAMPLE_8BITS | SBUS_MASK_ADDR_16BITS | SBUS_MASK_ADDR_SWAP_BYTES;
    sensor_ctx.sbus.control = 0;
    sensor_ctx.sbus.bus = 0;
    sensor_ctx.sbus.device = SENSOR_DEV_ADDRESS;
    acamera_sbus_init( &sensor_ctx.sbus, sbus_i2c );

    ret = 0;
    if (sensor_get_id(&sensor_ctx) == 0xFFFF)
        ret = -1;
    else
        pr_info("sensor_detect_imx334 id:%d\n", sensor_get_id(&sensor_ctx));

    acamera_sbus_deinit(&sensor_ctx.sbus,  sbus_i2c);

#if PLATFORM_G12B
    clk_am_disable(sensor_bp);
#endif
    reset_am_disable(sensor_bp);
    return ret;
}
//*************************************************************************************
