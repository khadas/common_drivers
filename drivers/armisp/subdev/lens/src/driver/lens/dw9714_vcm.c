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
#include <linux/i2c.h>
#include <linux/module.h>

#include "acamera_lens_api.h"
#include "system_sensor.h"
#include "acamera_logger.h"
#include "acamera_sbus_api.h"
#include "dw9714_vcm.h"

#define DW9714_PACKET_BYTES  2

#define STEP_PERIOD_81_US   0x0
#define STEP_PERIOD_162_US  0x1
#define STEP_PERIOD_324_US  0x2
#define STEP_PERIOD_648_US  0x3


#define PER_STEP_CODES_0    (0x0<<2)
#define PER_STEP_NO_SRC     PER_STEP_CODES_0
#define PER_STEP_CODES_1    (0x1<<2)
#define PER_STEP_CODES_2    (0x2<<2)
#define PER_STEP_CODES_3    (0x3<<2)


typedef struct dw9714_vcm_context_t {
    struct i2c_client *client;

    uint16_t pos;
    uint16_t prev_pos;
    uint16_t move_pos;

    uint32_t time;

    lens_param_t param;
    int inited;
} dw9714_vcm_context_t;

static struct dw9714_vcm_context_t  g_dw9714_ctx =
{
    .client = 0,
    .inited = 0
};

static int dw9714_i2c_write(uint8_t *data, int bytes)
{
    if (g_dw9714_ctx.client) {
        int msg_count = 0;
        int i = 0;
        int rc = -1;
        uint16_t saddr = g_dw9714_ctx.client->addr;

        struct i2c_msg msgs[] = {
            {
                .addr  = saddr,
                .flags = 0, // write
                .len   = bytes,
                .buf   = data,
            }
        };

        msg_count = sizeof(msgs) / sizeof(msgs[0]);

        for (i = 0; i < 5; i++) {
            rc = i2c_transfer(g_dw9714_ctx.client->adapter, msgs, msg_count);
            if (rc == msg_count) {
                break;
            }
        }

        if (rc < 0) {
            pr_err("%s:failed to write reg data: rc %d, saddr 0x%x\n", __func__,
                        rc, saddr);
            return rc;
        }

        return 0;

    } else {
        pr_err("%s: dw9714 i2c not probed\n", __func__);
        return -1;
    }
}

static int dw9714_i2c_read(uint8_t *data, int bytes)
{
    if (g_dw9714_ctx.client) {
        int msg_count = 0;
        int i = 0;
        int rc = -1;
        uint16_t saddr = g_dw9714_ctx.client->addr;

        struct i2c_msg msgs[] = {
            {
                .addr  = saddr,
                .flags = 0,
                .len   = 0,
                .buf   = 0,
            },
            {
                .addr  = saddr,
                .flags = I2C_M_RD,
                .len   = bytes,
                .buf   = data,
            },
        };

        msg_count = sizeof(msgs) / sizeof(msgs[0]);

        for (i = 0; i < 5; i++) {
            rc = i2c_transfer(g_dw9714_ctx.client->adapter, msgs, msg_count);
            if (rc == msg_count) {
                break;
            }
        }

        if (rc < 0) {
            pr_err("%s:failed to read reg data: rc %d, saddr 0x%x\n", __func__,
                        rc, saddr);
            return rc;
        }

        return 0;

    } else {
        pr_err("%s: dw9714 i2c not probed\n", __func__);
        return -1;
    }

}

static void dw9714_vcm_move( void *ctx, uint16_t position )
{
    dw9714_vcm_context_t *p_ctx = ctx;
    uint16_t pos;
    uint8_t data[DW9714_PACKET_BYTES];

    p_ctx->move_pos = position;
    p_ctx->param.next_pos = position;

    pos = ( position ) / (p_ctx->param.min_step);
    pr_err("%s  position %d / min_step  %d = pos %d",
            __func__, position, p_ctx->param.min_step,  pos );

    pos = pos & 0x3FF ; // 10 bits data.

    if (0 != dw9714_i2c_read(data, DW9714_PACKET_BYTES))
    {
        LOG( LOG_CRIT, " fail " );
    }

    data[0] = (pos >> 4) & 0b00111111;// PD & FLAG bits must be low. D9 - D4
    data[1] = ( (pos & 0x0f) << 4 ) + data[1] & 0x0f; // KEEP s[3:0]

    if (0 != dw9714_i2c_write(data, DW9714_PACKET_BYTES))
    {
        LOG( LOG_CRIT, " fail " );
    }

    p_ctx->prev_pos = position;
}

static uint8_t dw9714_vcm_is_moving( void *ctx )
{
    uint8_t data[DW9714_PACKET_BYTES];
    uint8_t ret = 0;

    if (0 == dw9714_i2c_read(data, DW9714_PACKET_BYTES)) {
        ret = (data[0] >> 6 ) & 0x01; // FLAG bit
    } else {
        LOG( LOG_ERR, " fail " );
    }
    LOG( LOG_INFO, "ret %d", ret);
    return ret;
}

static void dw9714_vcm_write_register( void *ctx, uint32_t address, uint32_t data )
{
    LOG( LOG_INFO, "%s", __func__ );
}

static uint32_t dw9714_vcm_read_register( void *ctx, uint32_t address )
{
    LOG( LOG_INFO, "%s", __func__ );
    return 0;
}

static const lens_param_t *dw9714_vcm_get_parameters( void *ctx )
{
    dw9714_vcm_context_t *p_ctx = ctx;
    //LOG( LOG_INFO, "%s", __func__ );
    return (const lens_param_t *)&p_ctx->param;
}

uint8_t lens_dw9714_test( uint32_t lens_bus )
{
    if (lens_bus == -1)
        return -1;
    return 1;
}


//===============dw9714 i2c driver begin======================
static int dw9714_probe(struct i2c_client *client)
{
    int ret = 0;
    uint8_t data[DW9714_PACKET_BYTES];

    pr_err("%s in ", __func__);
    g_dw9714_ctx.client = client;

    if (0 == dw9714_i2c_read(data, DW9714_PACKET_BYTES)) {
        LOG( LOG_INFO, "%s success, byte 0 & 1 0x%x  0x%x ", __func__, data[0], data[1]);
    } else {
        LOG( LOG_ERR, " read fail ");
        return -1;
    }

    // initial value.
    data[0] = 0X00;
    data[1] = PER_STEP_CODES_2 | STEP_PERIOD_81_US;
    if (0 != dw9714_i2c_write(data, DW9714_PACKET_BYTES) ) {
        LOG( LOG_ERR, " write fail ");
    }

    if (0 == dw9714_i2c_read(data, DW9714_PACKET_BYTES)) {
        LOG( LOG_INFO, "%s after initial value, read back byte 0 & 1 0x%x  0x%x ", __func__, data[0], data[1]);
    }

    return ret;
}


static int dw9714_remove(struct i2c_client *client)
{
    return 0;
}


static const struct i2c_device_id dw9714_id[] = {
    {"dw9714", 0},
    {}
};

static const struct of_device_id dw9714_dt_ids[] = {
    { .compatible = "dw, dw9714, t7" },
    { /* sentinel */ }
};


static struct i2c_driver dw9714_i2c_driver = {
    .driver = {
        .name  = "dw9714",
        .of_match_table = dw9714_dt_ids,
    },
    .id_table = dw9714_id,
    .probe_new = dw9714_probe,
    .remove   = dw9714_remove,
};



//===============dw9714 i2c driver end======================


void lens_dw9714_deinit( void *ctx )
{
    if (g_dw9714_ctx.inited)
    {
        i2c_del_driver(&dw9714_i2c_driver);
        g_dw9714_ctx.inited = 0;
    }
}

void lens_dw9714_init( void **ctx, lens_control_t *ctrl, uint32_t lens_bus )
{
    int ret = 0;
    *ctx = &g_dw9714_ctx;
    ctrl->is_moving           = dw9714_vcm_is_moving;
    ctrl->move                = dw9714_vcm_move;
    ctrl->write_lens_register = dw9714_vcm_write_register;
    ctrl->read_lens_register  = dw9714_vcm_read_register;
    ctrl->get_parameters      = dw9714_vcm_get_parameters;

    g_dw9714_ctx.prev_pos = 0;

    memset( &g_dw9714_ctx.param, 0, sizeof( lens_param_t ) );

    g_dw9714_ctx.param.min_step = 64;
    g_dw9714_ctx.param.lens_type = LENS_VCM_DRIVER_DW9714;

    if (0 == g_dw9714_ctx.inited)
    {
        ret = i2c_add_driver(&dw9714_i2c_driver);
        if (ret != 0) {
            pr_err("%s:failed to add dw9714 i2c driver\n", __func__);
        } else {
            pr_err("%s:success to add dw9714 i2c driver\n", __func__);
            g_dw9714_ctx.inited = 1;
        }
    } else {
        pr_err("%s: dw9714 i2c driver has been registered\n", __func__);
    }
}


