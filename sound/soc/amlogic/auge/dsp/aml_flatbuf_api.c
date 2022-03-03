// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

/**
 * flat buffer api
 *
 * Author: Wenjie Zhou <Wenjie.Zhou@amlogic.com>
 * Version:
 * - 0.1        init
 */

/* #include <stdio.h> */
#include <linux/printk.h>
/* #include <string.h> */
/* #include <stdlib.h> */
#include <linux/slab.h>

#include "aipc_type.h"
#include "rpc_client_aipc.h"
#include "rpc_client_shm.h"
#include "aml_flatbuf_api.h"

struct FLATBUFS {
	int aipchdl;
	t_aml_flatbuf_hdl_rpc h_fbuf;
	AML_MEM_HANDLE h_shm;
};

void aml_flatbuf_reset(AML_FLATBUF_HANDLE h_fbuf, bool clear)
{
	struct aml_flatbuf_reset_st arg;
	struct FLATBUFS *p_fbuf;

	p_fbuf = (struct FLATBUFS *)h_fbuf;
	arg.h_fbuf = (t_aml_flatbuf_hdl_rpc)p_fbuf->h_fbuf;
	arg.clear = clear ? 1 : 0;
	xaipc(p_fbuf->aipchdl, MBX_CMD_FLATBUF_RESET, &arg, sizeof(arg));
}

AML_FLATBUF_HANDLE aml_flatbuf_create(const char *buf_id, int flags,
				      struct flatbuffer_config *config)
{
	int rpc_ret = 0;
	struct aml_flatbuf_create_st arg;
	struct FLATBUFS *p_fbuf;
	int id;

	if (config->phy_ch != FLATBUF_CH_ARM2DSPA &&
	    config->phy_ch != FLATBUF_CH_ARM2DSPB) {
		pr_info("Not supported physical channel:%d\n", config->phy_ch);
		return NULL;
	}
	p_fbuf = kmalloc(sizeof(*p_fbuf), GFP_KERNEL);
	if (!p_fbuf)
		goto recycle_flatbuf_context;
	memset(p_fbuf, 0, sizeof(struct FLATBUFS));
	strncpy(arg.buf_id, buf_id, BUF_STR_ID_MAX - 1);
	arg.flags = flags;
	arg.size = config->size;
	id = (config->phy_ch == FLATBUF_CH_ARM2DSPA) ? 0 : 1;
	p_fbuf->aipchdl = xaudio_ipc_init(id);
	if (p_fbuf->aipchdl < 0) {
		pr_info("Failed to init rpc handle\n");
		goto recycle_flatbuf_context;
	}
	rpc_ret = xaipc(p_fbuf->aipchdl, MBX_CMD_FLATBUF_CREATE, &arg, sizeof(arg));
	if (rpc_ret < 0) {
		pr_info("Failed to invoke MBX_CMD_FLATBUF_CREATE r=%d buf_id=%s flags=%s\n",
			rpc_ret, buf_id, (flags & FLATBUF_FLAG_RD) ? "RD" : "WR");
		goto recycle_flatbuf_context;
	}
	p_fbuf->h_fbuf = arg.h_fbuf;
	p_fbuf->h_shm = (AML_MEM_HANDLE)arg.h_inter_buf;
	return p_fbuf;
recycle_flatbuf_context:
	if (p_fbuf) {
		if (p_fbuf->aipchdl >= 0)
			xaudio_ipc_deinit(p_fbuf->aipchdl);
		kfree(p_fbuf);
	}
	return NULL;
}

void aml_flatbuf_destroy(AML_FLATBUF_HANDLE h_fbuf)
{
	struct aml_flatbuf_destroy_st arg;
	struct FLATBUFS *p_fbuf;

	p_fbuf = (struct FLATBUFS *)h_fbuf;

	arg.h_fbuf = (t_aml_flatbuf_hdl_rpc)p_fbuf->h_fbuf;
	xaipc(p_fbuf->aipchdl, MBX_CMD_FLATBUF_DESTROY, &arg, sizeof(arg));
	xaudio_ipc_deinit(p_fbuf->aipchdl);
	kfree(p_fbuf);
}

int aml_flatbuf_read(AML_FLATBUF_HANDLE h_fbuf, void *buf, size_t size, int ms_timeout)
{
	int rpc_ret = 0;
	struct aml_flatbuf_read_st arg;
	struct FLATBUFS *p_fbuf;

	p_fbuf = (struct FLATBUFS *)h_fbuf;
	arg.h_fbuf = (t_aml_flatbuf_hdl_rpc)p_fbuf->h_fbuf;
	arg.mem = CAST_PTR64(aml_mem_getphyaddr(p_fbuf->h_shm));
	arg.size = size;
	arg.ms = (uint32_t)ms_timeout;
	rpc_ret = xaipc(p_fbuf->aipchdl, MBX_CMD_FLATBUF_READ, &arg, sizeof(arg));
	if (rpc_ret < 0) {
		pr_info("Failed to invoke MBX_CMD_FLATBUF_READ\n");
		return -1;
	}
	aml_mem_invalidate(p_fbuf->h_shm, arg.size);
	memcpy(buf, aml_mem_getvirtaddr(p_fbuf->h_shm), arg.size);
	return arg.size;
}

int aml_flatbuf_write(AML_FLATBUF_HANDLE h_fbuf, const void *buf, size_t size, int ms_timeout)
{
	int rpc_ret = 0;
	struct aml_flatbuf_write_st arg;
	struct FLATBUFS *p_fbuf;

	p_fbuf = (struct FLATBUFS *)h_fbuf;
	arg.h_fbuf = (t_aml_flatbuf_hdl_rpc)p_fbuf->h_fbuf;
	arg.mem = CAST_PTR64(aml_mem_getphyaddr(p_fbuf->h_shm));
	arg.size = size;
	arg.ms = (uint32_t)ms_timeout;
	memcpy(aml_mem_getvirtaddr(p_fbuf->h_shm), buf, arg.size);
	aml_mem_clean(p_fbuf->h_shm, arg.size);
	rpc_ret = xaipc(p_fbuf->aipchdl, MBX_CMD_FLATBUF_WRITE, &arg, sizeof(arg));
	if (rpc_ret < 0) {
		pr_info("Failed to invoke MBX_CMD_FLATBUF_WRITE\n");
		return -1;
	}
	return arg.size;
}

size_t aml_flatbuf_getfullness(AML_FLATBUF_HANDLE h_fbuf)
{
	struct aml_flatbuf_size_st arg;
	struct FLATBUFS *p_fbuf;

	p_fbuf = (struct FLATBUFS *)h_fbuf;
	arg.h_fbuf = (t_aml_flatbuf_hdl_rpc)p_fbuf->h_fbuf;
	xaipc(p_fbuf->aipchdl, MBX_CMD_FLATBUF_GETFULLNESS, &arg, sizeof(arg));

	return arg.size;
}

size_t aml_flatbuf_getspace(AML_FLATBUF_HANDLE h_fbuf)
{
	struct aml_flatbuf_size_st arg;
	struct FLATBUFS *p_fbuf;

	p_fbuf = (struct FLATBUFS *)h_fbuf;
	arg.h_fbuf = p_fbuf->h_fbuf;
	xaipc(p_fbuf->aipchdl, MBX_CMD_FLATBUF_GETSPACE, &arg, sizeof(arg));

	return arg.size;
}
