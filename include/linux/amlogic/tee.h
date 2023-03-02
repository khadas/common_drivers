/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __TEE_H__
#define __TEE_H__

/* memory align used by tee_protect_mem_by_type() */
#define TEE_MEM_ALIGN_SIZE                                 0x10000

/* memory type used by tee_protect_mem_by_type() */
#define TEE_MEM_TYPE_STREAM_INPUT                          0x4
#define TEE_MEM_TYPE_STREAM_OUTPUT                         0x5
#define TEE_MEM_TYPE_GPU                                   0x6
#define TEE_MEM_TYPE_VDIN                                  0x7
#define TEE_MEM_TYPE_HCODEC                                0x8
#define TEE_MEM_TYPE_GE2D                                  0x9
#define TEE_MEM_TYPE_DEMUX                                 0xa
#define TEE_MEM_TYPE_TCON                                  0xb
#define TEE_MEM_TYPE_PCIE                                  0xc
#define TEE_MEM_TYPE_FRC                                   0xd
#define TEE_MEM_TYPE_KERNEL                                0xe
#define TEE_MEM_TYPE_INVALID                               0xff

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

/* TEE error codes */
#define TEEC_SUCCESS                                       0x00000000

bool tee_enabled(void);

int is_secload_get(void);

int tee_load_video_fw(u32 index, u32 vdec);

int tee_load_video_fw_swap(u32 index, u32 vdec, bool is_swap);

int tee_config_device_state(int dev_id, int secure);

void tee_demux_config_pipeline(int tsn_in, int tsn_out);

int tee_demux_config_pad(int reg, int val);

int tee_read_reg_bits(u32 reg, u32 *val, u32 offset, u32 length);

int tee_write_reg_bits(u32 reg, u32 val, u32 offset, u32 length);

u32 tee_protect_mem(u32 type, u32 level,
		u32 start, u32 size, u32 *handle);

void tee_unprotect_mem(u32 handle);

int tee_check_in_mem(u32 pa, u32 size);

int tee_check_out_mem(u32 pa, u32 size);

int tee_vp9_prob_process(u32 cur_frame_type, u32 prev_frame_type,
		u32 prob_status, u32 prob_addr);

int tee_vp9_prob_malloc(u32 *prob_addr);

int tee_vp9_prob_free(u32 prob_addr);

/* All of the following apis will be deprecated */
u32 tee_protect_tvp_mem(u32 start, u32 size,
			u32 *handle);

void tee_unprotect_tvp_mem(u32 handle);

u32 tee_protect_mem_by_type(u32 type,
		u32 start, u32 size,
		u32 *handle);

#endif /* __TEE_H__ */

