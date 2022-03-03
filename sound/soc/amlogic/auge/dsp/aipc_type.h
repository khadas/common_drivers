/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

/**
 * data type for audio rpc
 *
 * Author: Wenjie Zhou <Wenjie.Zhou@amlogic.com>
 * Version:
 * - 0.1        init
 */

#ifndef _AIPC_TYPE_H_
#define _AIPC_TYPE_H_

#include <linux/types.h>
#include <asm-generic/int-ll64.h>
/* for s32, u32 */
/* #include <stdint.h> */
#include "ipc_cmd_type.h"
#include "rpc_dev.h"

/*rpc dummy*/
struct t_aml_dummy_rpc {
	s32   task_id;
	u32  task_sleep_ms;
	s32   func_code;
	s32   input_param;
	s32   output_param;
} __packed;

/*Voice Signal Processing*/
/* typedef xpointer aml_vsp_rpc_hdl_t; */
/* typedef uint64_t xsize_t; */
#define aml_vsp_rpc_hdl_t xpointer
#define xsize_t uint64_t
#define VSP_ID_MAX 32
struct vsp_init_st {
	char vsp_id[VSP_ID_MAX];
	aml_vsp_rpc_hdl_t hdl;
	u32 ret;
	xpointer param;
	xsize_t param_size;
	s32 ops_idx;
} __packed;

struct vsp_deinit_st {
	s32 ops_idx;
	aml_vsp_rpc_hdl_t hdl;
} __packed;

struct vsp_open_st {
	s32 ops_idx;
	aml_vsp_rpc_hdl_t hdl;
	u32 ret;
} __packed;

struct vsp_close_st {
	s32 ops_idx;
	aml_vsp_rpc_hdl_t hdl;
	u32 ret;
} __packed;

struct vsp_setparam_st {
	s32 ops_idx;
	aml_vsp_rpc_hdl_t hdl;
	s32 param_id;
	xpointer param;
	xsize_t param_size;
	u32 ret;
} __packed;

struct vsp_getparam_st {
	s32 ops_idx;
	aml_vsp_rpc_hdl_t hdl;
	s32 param_id;
	xpointer param;
	xsize_t param_size;
	u32 ret;
} __packed;

struct vsp_process_st {
	s32 ops_idx;
	aml_vsp_rpc_hdl_t hdl;
	u32 ret;
	xpointer input_buf;
	xsize_t input_size;
	xpointer output_buf;
	xsize_t output_size;
	u32 acc;
} __packed;

/*hifi codec shared memory*/
/* typedef xpointer aml_mem_handle_rpc; */
#define aml_mem_handle_rpc xpointer
struct acodec_shm_alloc_st {
	aml_mem_handle_rpc h_shm;
	xsize_t size;
	s32 pid;
} __packed;

struct acodec_shm_free_st {
	aml_mem_handle_rpc h_shm;
} __packed;

struct acodec_shm_transfer_st {
	aml_mem_handle_rpc h_dst;
	aml_mem_handle_rpc h_src;
	xsize_t size;
} __packed;

struct acodec_shm_recycle_st {
	s32 pid;
} __packed;

/*hifi flat buffer*/
/* typedef xpointer tAmlFlatBufHdlRpc; */
#define t_aml_flatbuf_hdl_rpc xpointer
#define BUF_STR_ID_MAX 32

struct aml_flatbuf_reset_st {
	t_aml_flatbuf_hdl_rpc h_fbuf;
	u32 clear;
} __packed;

struct aml_flatbuf_create_st {
	t_aml_flatbuf_hdl_rpc h_fbuf;
	aml_mem_handle_rpc h_inter_buf;
	char buf_id[BUF_STR_ID_MAX];
	s32 flags;
	xsize_t size;
} __packed;

struct aml_flatbuf_destroy_st {
	t_aml_flatbuf_hdl_rpc h_fbuf;
} __packed;

struct aml_flatbuf_write_st {
	t_aml_flatbuf_hdl_rpc h_fbuf;
	xpointer mem;
	xsize_t size;
	u32 ms;
} __packed;

struct aml_flatbuf_read_st {
	t_aml_flatbuf_hdl_rpc h_fbuf;
	xpointer mem;
	xsize_t size;
	u32 ms;
} __packed;

struct aml_flatbuf_size_st {
	t_aml_flatbuf_hdl_rpc h_fbuf;
	xsize_t size;
} __packed;

struct shm_st {
	xpointer out_ptr; /* returned pointer */
	xsize_t size;     /* memory's size */
	xpointer ptr;
} __packed;

#endif
