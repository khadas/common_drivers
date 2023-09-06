/*
*
* SPDX-License-Identifier: GPL-2.0
*
* Copyright (C) 2018 Amlogic or its affiliates
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

#ifndef __SYSTEM_AM_ADAP_H__
#define __SYSTEM_AM_ADAP_H__

#include <linux/module.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include "acamera_firmware_config.h"

#if PLATFORM_G12B
#define FRONTEND_BASE               0x00004800
#define FRONTEND1_BASE              0x00004C00
#define RD_BASE                     0x00005000
#define PIXEL_BASE                  0x00005000
#define ALIGN_BASE                  0x00005000
#define MISC_BASE                   0x00005000
#elif PLATFORM_C308X || PLATFORM_C305X
#define FRONTEND_BASE               0x00002800
#define FRONTEND1_BASE              0x00002C00
#define RD_BASE                     0x00003000
#define PIXEL_BASE                  0x00003000
#define ALIGN_BASE                  0x00003000
#define MISC_BASE                   0x00003000
#endif

#define FTE1_OFFSET                 0x00000400
#define CSI2_CLK_RESET				0x00
#define CSI2_GEN_CTRL0				0x04
#define CSI2_GEN_CTRL1				0x08
#define CSI2_X_START_END_ISP		0x0C
#define CSI2_Y_START_END_ISP		0x10
#define CSI2_X_START_END_MEM		0x14
#define CSI2_Y_START_END_MEM		0x18
#define CSI2_VC_MODE				0x1C

#if PLATFORM_G12B
#define CSI2_VC_MODE2_MATCH_MASK_L	0x20
#define CSI2_VC_MODE2_MATCH_MASK_H	0x24
#define CSI2_VC_MODE2_MATCH_TO_VC_L	0x28
#define CSI2_VC_MODE2_MATCH_TO_VC_H	0x2C
#define CSI2_VC_MODE2_MATCH_TO_IGNORE_L	0x30
#define CSI2_VC_MODE2_MATCH_TO_IGNORE_H	0x34

#elif PLATFORM_C308X || PLATFORM_C305X
#define CSI2_VC_MODE2_MATCH_MASK_A_L	0x20
#define CSI2_VC_MODE2_MATCH_MASK_A_H	0x24
#define CSI2_VC_MODE2_MATCH_A_L	0x28
#define CSI2_VC_MODE2_MATCH_A_H	0x2C
#define CSI2_VC_MODE2_MATCH_B_L	0x30
#define CSI2_VC_MODE2_MATCH_B_H	0x34
#endif

#define CSI2_DDR_START_PIX			0x38
#define CSI2_DDR_START_PIX_ALT		0x3C
#define CSI2_DDR_STRIDE_PIX			0x40
#define CSI2_DDR_START_OTHER		0x44
#define CSI2_DDR_START_OTHER_ALT	0x48
#define CSI2_DDR_MAX_BYTES_OTHER	0x4C
#define CSI2_INTERRUPT_CTRL_STAT	0x50

#define CSI2_VC_MODE2_MATCH_MASK_B_L 0x54
#define CSI2_VC_MODE2_MATCH_MASK_B_H 0x58
#define CSI2_DDR_LOOP_LINES_PIX      0x5c

#define CSI2_GEN_STAT0				0x80
#define CSI2_ERR_STAT0				0x84
#define CSI2_PIC_SIZE_STAT			0x88
#define CSI2_DDR_WPTR_STAT_PIX		0x8C
#define CSI2_DDR_WPTR_STAT_OTHER	0x90
#define CSI2_STAT_MEM_0				0x94
#define CSI2_STAT_MEM_1				0x98

#define CSI2_STAT_GEN_SHORT_08		0xA0
#define CSI2_STAT_GEN_SHORT_09		0xA4
#define CSI2_STAT_GEN_SHORT_0A		0xA8
#define CSI2_STAT_GEN_SHORT_0B		0xAC
#define CSI2_STAT_GEN_SHORT_0C		0xB0
#define CSI2_STAT_GEN_SHORT_0D		0xB4
#define CSI2_STAT_GEN_SHORT_0E		0xB8
#define CSI2_STAT_GEN_SHORT_0F		0xBC


#define MIPI_ADAPT_DDR_RD0_CNTL0    0x00
#define MIPI_ADAPT_DDR_RD0_CNTL1    0x04
#define MIPI_ADAPT_DDR_RD0_CNTL2    0x08
#define MIPI_ADAPT_DDR_RD0_CNTL3    0x0C
#define MIPI_ADAPT_DDR_RD0_CNTL4    0x10
#define MIPI_ADAPT_DDR_RD0_ST0		0x14
#define MIPI_ADAPT_DDR_RD0_ST1		0x18
#define MIPI_ADAPT_DDR_RD0_ST2		0x1c
#define MIPI_ADAPT_DDR_RD0_CNTL5    0x20
#define MIPI_ADAPT_DDR_RD0_CNTL6    0x24
#define MIPI_ADAPT_DDR_RD1_CNTL0    0x40
#define MIPI_ADAPT_DDR_RD1_CNTL1    0x44
#define MIPI_ADAPT_DDR_RD1_CNTL2    0x48
#define MIPI_ADAPT_DDR_RD1_CNTL3    0x4C
#define MIPI_ADAPT_DDR_RD1_CNTL4    0x50
#define MIPI_ADAPT_DDR_RD1_ST0		0x54
#define MIPI_ADAPT_DDR_RD1_ST1		0x58
#define MIPI_ADAPT_DDR_RD1_ST2		0x5c
#define MIPI_ADAPT_DDR_RD1_CNTL5    0x60
#define MIPI_ADAPT_DDR_RD1_CNTL6    0x64

#define MIPI_ADAPT_PIXEL0_CNTL0     0x80
#define MIPI_ADAPT_PIXEL0_CNTL1     0x84
#define MIPI_ADAPT_PIXEL1_CNTL0     0x88
#define MIPI_ADAPT_PIXEL1_CNTL1     0x8C
#define MIPI_ADAPT_PIXEL0_ST0       0xA8
#define MIPI_ADAPT_PIXEL0_ST1       0xAC
#define MIPI_ADAPT_PIXEL1_ST0       0xB0
#define MIPI_ADAPT_PIXEL1_ST1       0xB4

#define MIPI_ADAPT_ALIG_CNTL0       0xC0
#define MIPI_ADAPT_ALIG_CNTL1       0xC4
#define MIPI_ADAPT_ALIG_CNTL2       0xC8
#define MIPI_ADAPT_ALIG_CNTL3       0xCC
#define MIPI_ADAPT_ALIG_CNTL4       0xD0
#define MIPI_ADAPT_ALIG_CNTL5       0xD4
#define MIPI_ADAPT_ALIG_CNTL6       0xD8
#define MIPI_ADAPT_ALIG_CNTL7       0xDC
#define MIPI_ADAPT_ALIG_CNTL8       0xE0
#define MIPI_ADAPT_ALIG_CNTL9       0xE4
#define MIPI_ADAPT_ALIG_ST0         0xE8
#define MIPI_ADAPT_ALIG_ST1         0xEC

#define MIPI_OTHER_CNTL0           0x100
#define MIPI_ADAPT_IRQ_MASK0       0x180
#define MIPI_ADAPT_IRQ_PENDING0    0x184
#define MIPI_ADAPT_AXI_CTRL0 (0x18 + 0x800)
#define DDR_RD0_LBUF_STATUS        0x140
#define DDR_RD1_LBUF_STATUS        0x144

//Interrupt Bit
#define FRONT0_WR_DONE             19
#define READ0_RD_DONE              13
#define FRONT1_WR_DONE             24
#define READ1_RD_DONE              11
#define ALIGN_FRAME_END            8


//Camera Flag Bit
#define CAM_LAST                   26
#define CAM_CURRENT                24
#define CAM_NEXT                   22
#define CAM_NEXT_NEXT              20

typedef enum {
	FRONTEND_IO,
	RD_IO,
	PIXEL_IO,
	ALIGN_IO,
	MISC_IO,
} adap_io_type_t;

typedef enum {
	DDR_MODE,
	DIR_MODE,
	DOL_MODE,
	DCAM_MODE,
} adap_mode_t;

typedef enum {
	PATH0,
	PATH1,
} adap_path_t;

typedef struct {
	uint32_t width;
	uint32_t height;
} adap_img_t;

typedef enum {
	AM_RAW6 = 1,
	AM_RAW7,
	AM_RAW8,
	AM_RAW10,
	AM_RAW12,
	AM_RAW14,
} img_fmt_t;

typedef enum {
	DOL_NON = 0,
	DOL_VC,
	DOL_LINEINFO,
} dol_type_t;

typedef enum {
	FTE_DONE = 0,
	FTE0_DONE,
	FTE1_DONE,
} dol_state_t;

typedef enum {
	ADAP0_PATH = 0,
	ADAP1_PATH,
	ADAP1_ALT0_PATH,
} adap_chan_t;

typedef enum {
    CAM0_ACT = 0,
    CAM1_ACT,
    CAMS_MAX,
} cam_num_t;

typedef enum {
    FRAME_READY,
    FRAME_NOREADY,
} frame_status_t;

typedef enum {
	CAM_DIS,
	CAM_EN,
	DUAL_CAM_EN,
} cam_mode_t;

typedef struct exp_offset {
	int long_offset;
	int short_offset;
	int offset_x;
} exp_offset_t;

struct am_adap {
	struct device_node *of_node;
	struct platform_device *p_dev;
	struct resource reg;
	void __iomem *base_addr;
	int f_end_irq;
	int rd_irq;
	unsigned int adap_buf_size;
	int f_fifo;
	int f_adap;

	uint32_t write_frame_ptr;
	uint32_t read_frame_ptr;

	int frame_state;
	struct task_struct *kadap_stream;
	wait_queue_head_t frame_wq;
    spinlock_t reg_lock;
};

struct am_adap_info {
	adap_path_t path;
	adap_mode_t mode;
	adap_img_t img;
	img_fmt_t fmt;
	dol_type_t type;
	exp_offset_t offset;
	uint8_t alt0_path;
	uint32_t align_width;
};


int am_adap_parse_dt(struct device_node *node);
void am_adap_deinit_parse_dt(void);
int am_adap_init(uint8_t channel);
int am_adap_start(uint8_t channel, uint8_t dcam);
int am_adap_reset(uint8_t channel);
int am_adap_deinit(uint8_t channel);
void am_adap_set_info(struct am_adap_info *info);
int get_fte1_flag(void);
int am_adap_get_depth(uint8_t channel);
extern int32_t system_timer_usleep( uint32_t usec );
extern int camera_notify( uint notification, void *arg);

#endif

