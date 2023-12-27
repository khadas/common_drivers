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

#if !defined(__VIRTCAM_SENSOR_H__)
#define __VIRTCAM_SENSOR_H__


/*-----------------------------------------------------------------------------
Initialization sequence - do not edit
-----------------------------------------------------------------------------*/

#include "sensor_init.h"

static acam_reg_t settings_context_virtcam[] = {
    //stop sequence - address is 0x0000
    { 0x18ED8, 0x01, 0xFF, 1 },
    { 0x0000, 0x0000, 0x0000, 0x0000 }
};

static const acam_reg_t *isp_seq_table[] = {
    settings_context_virtcam,
};


#define SENSOR__VIRTCAM_SEQUENCE_DEFAULT seq_table
#define SENSOR__VIRTCAM_ISP_SEQUENCE_DEFAULT seq_table


#define SENSOR_VIRTCAM_SEQUENCE_DEFAULT_INIT    0
#define SENSOR_VIRTCAM_SEQUENCE_DEFAULT_PREVIEW    1
#define SENSOR_VIRTCAM_SEQUENCE_SECOND_PREVIEW    2
#define SENSOR_VIRTCAM_SEQUENCE_THIRD_PREVIEW    3
#define SENSOR_VIRTCAM_SEQUENCE_S2_PREVIEW    4
#define SENSOR_VIRTCAM_SEQUENCE_S4_PREVIEW    5

#define SENSOR_VIRTCAM_ISP_CONTEXT_SEQ   0


#endif /* __VIRTCAM_SENSOR_H__ */
