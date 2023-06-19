/*
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Description:
 */
#ifndef __VIDEO_FIRMWARE_HEADER_
#define __VIDEO_FIRMWARE_HEADER_

#include "../../../common/firmware/firmware_type.h"
#include <linux/amlogic/media/utils/vformat.h>
#if IS_ENABLED(CONFIG_AMLOGIC_TEE) || \
	IS_ENABLED(CONFIG_AMLOGIC_TEE_MODULE)
#include <linux/amlogic/tee.h>
#else
/* device ID used by tee_config_device_state() */
#define DMC_DEV_ID_GPU                                     1
#define DMC_DEV_ID_HEVC                                    4
#define DMC_DEV_ID_PARSER                                  7
#define DMC_DEV_ID_VPU                                     8
#define DMC_DEV_ID_VDIN                                    9
#define DMC_DEV_ID_VDEC                                    13
#define DMC_DEV_ID_HCODEC                                  14
#define DMC_DEV_ID_GE2D                                    15
#define DMC_DEV_ID_DI_PRE                                  16
#define DMC_DEV_ID_DI_POST                                 17
#define DMC_DEV_ID_GDC                                     18

bool __weak tee_enabled(void) { return false; }

int __weak tee_load_video_fw_swap(u32 index, u32 vdec, bool is_swap)
{
	return -1;
}

int __weak tee_load_video_fw(u32 index, u32 vdec)
{
	return -1;
}

int __weak tee_check_in_mem(u32 pa, u32 size)
{
	return -1;
}

int __weak tee_vp9_prob_free(u32 prob_addr)
{
	return -1;
}

int __weak tee_vp9_prob_malloc(u32 *prob_addr)
{
	return -1;
}

int __weak tee_config_device_state(int dev_id, int secure)
{
	return -1;
}

int __weak tee_vp9_prob_process(u32 cur_frame_type, u32 prev_frame_type,
		u32 prob_status, u32 prob_addr)
{
	return -1;
}
#endif

#define FW_LOAD_FORCE	(0x1)
#define FW_LOAD_TRY	(0X2)

struct firmware_s {
	char name[32];
	unsigned int len;
	char data[0];
};

extern int get_decoder_firmware_data(enum vformat_e type,
	const char *file_name, char *buf, int size);
extern int get_data_from_name(const char *name, char *buf);
extern int get_firmware_data(unsigned int format, char *buf);
extern int video_fw_reload(int mode);
extern bool fw_tee_enabled(void);

#endif
