// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

/*
 * Copyright (C) 2014-2018 Amlogic, Inc. All rights reserved.
 *
 * All information contained herein is Amlogic confidential.
 *
 * This software is provided to you pursuant to Software License Agreement
 * (SLA) with Amlogic Inc ("Amlogic"). This software may be used
 * only in accordance with the terms of this agreement.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification is strictly prohibited without prior written permission from
 * Amlogic.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * offload voice signal processing api
 *
 * Author: Wenjie Zhou <Wenjie.Zhou@amlogic.com>
 * Version:
 * - 0.1        init
 */

/* #include <string.h> */
#include <linux/string.h>
/* #include <stdlib.h> */
#include <linux/slab.h>
/* #include <stdio.h> */
#include <linux/printk.h>
#include "rpc_client_aipc.h"
#include "rpc_client_vsp.h"
#include "aipc_type.h"

struct aml_vsp_ctx_t {
	aml_vsp_rpc_hdl_t	vspsrvhdl;
	int		aipchdl;
	s32		opsidx;
};

AML_VSP_HANDLE aml_vsp_init(const char *vsp_id, void *param, size_t param_size)
{
	struct vsp_init_st arg;
	struct aml_vsp_ctx_t *p_aml_vsp_ctx = kmalloc(sizeof(*p_aml_vsp_ctx),
						      GFP_KERNEL);
	/* pr_info("ctx=%lx\n", (unsigned long)p_aml_vsp_ctx); */

	memset(p_aml_vsp_ctx, 0, sizeof(struct aml_vsp_ctx_t));
	p_aml_vsp_ctx->aipchdl = xaudio_ipc_init(0);
	memset(&arg, 0, sizeof(arg));
	memcpy(arg.vsp_id, vsp_id, strlen(vsp_id) + 1);
	arg.param = (xpointer)(unsigned long)param;
	arg.param_size = param_size;
	xaipc(p_aml_vsp_ctx->aipchdl, MBX_CODEC_VSP_API_INIT, &arg, sizeof(arg));
	p_aml_vsp_ctx->vspsrvhdl = arg.hdl;
	p_aml_vsp_ctx->opsidx = arg.ops_idx;
	/* pr_info("ctx=%lx hdl=%lx optidx=%d\n", */
	/* (unsigned long)p_aml_vsp_ctx, (unsigned long)arg.hdl, arg.ops_idx); */
	return (void *)p_aml_vsp_ctx;
}

void aml_vsp_deinit(AML_VSP_HANDLE h_vsp)
{
	struct vsp_deinit_st arg;
	struct aml_vsp_ctx_t *p_aml_vsp_ctx = (struct aml_vsp_ctx_t *)h_vsp;

	memset(&arg, 0, sizeof(arg));
	arg.hdl = (aml_vsp_rpc_hdl_t)p_aml_vsp_ctx->vspsrvhdl;
	arg.ops_idx = p_aml_vsp_ctx->opsidx;
	xaipc(p_aml_vsp_ctx->aipchdl, MBX_CODEC_VSP_API_DEINIT, &arg, sizeof(arg));
	xaudio_ipc_deinit(p_aml_vsp_ctx->aipchdl);
	kfree(p_aml_vsp_ctx);
}

int  aml_vsp_open(AML_VSP_HANDLE h_vsp)
{
	struct vsp_open_st arg;
	struct aml_vsp_ctx_t *p_aml_vsp_ctx = (struct aml_vsp_ctx_t *)h_vsp;

	memset(&arg, 0, sizeof(arg));
	arg.hdl = (aml_vsp_rpc_hdl_t)p_aml_vsp_ctx->vspsrvhdl;
	arg.ops_idx = p_aml_vsp_ctx->opsidx;
	xaipc(p_aml_vsp_ctx->aipchdl, MBX_CODEC_VSP_API_OPEN, &arg, sizeof(arg));
	return arg.ret;
}

int  aml_vsp_close(AML_VSP_HANDLE h_vsp)
{
	struct vsp_close_st arg;
	struct aml_vsp_ctx_t *p_aml_vsp_ctx = (struct aml_vsp_ctx_t *)h_vsp;

	memset(&arg, 0, sizeof(arg));
	arg.hdl = (aml_vsp_rpc_hdl_t)p_aml_vsp_ctx->vspsrvhdl;
	arg.ops_idx = p_aml_vsp_ctx->opsidx;
	xaipc(p_aml_vsp_ctx->aipchdl, MBX_CODEC_VSP_API_CLOSE, &arg, sizeof(arg));
	return arg.ret;
}

int  aml_vsp_setparam(AML_VSP_HANDLE h_vsp, s32 param_id, void *param, size_t param_size)
{
	struct vsp_setparam_st arg;
	struct aml_vsp_ctx_t *p_aml_vsp_ctx = (struct aml_vsp_ctx_t *)h_vsp;

	memset(&arg, 0, sizeof(arg));
	arg.hdl = (aml_vsp_rpc_hdl_t)p_aml_vsp_ctx->vspsrvhdl;
	arg.ops_idx = p_aml_vsp_ctx->opsidx;
	arg.param_id = param_id;
	arg.param = (xpointer)(unsigned long)param;
	arg.param_size = param_size;
	xaipc(p_aml_vsp_ctx->aipchdl, MBX_CODEC_VSP_API_SETPARAM, &arg, sizeof(arg));
	return arg.ret;
}

int  aml_vsp_getparam(AML_VSP_HANDLE h_vsp, s32 param_id, void *param, size_t param_size)
{
	struct vsp_getparam_st arg;
	struct aml_vsp_ctx_t *p_aml_vsp_ctx = (struct aml_vsp_ctx_t *)h_vsp;

	memset(&arg, 0, sizeof(arg));
	arg.hdl = (aml_vsp_rpc_hdl_t)p_aml_vsp_ctx->vspsrvhdl;
	arg.ops_idx = p_aml_vsp_ctx->opsidx;
	arg.param_id = param_id;
	arg.param = (xpointer)(unsigned long)param;
	arg.param_size = param_size;
	xaipc(p_aml_vsp_ctx->aipchdl, MBX_CODEC_VSP_API_GETPARAM, &arg, sizeof(arg));
	return arg.ret;
}

static u32 acc;

int  aml_vsp_process(AML_VSP_HANDLE h_vsp, void *input_buf, size_t input_size,
		     void *output_buf, size_t *output_size)
{
	struct vsp_process_st arg;
	struct aml_vsp_ctx_t *p_aml_vsp_ctx = (struct aml_vsp_ctx_t *)h_vsp;

	memset(&arg, 0, sizeof(arg));
	arg.hdl = (aml_vsp_rpc_hdl_t)p_aml_vsp_ctx->vspsrvhdl;
	arg.ops_idx = p_aml_vsp_ctx->opsidx;
	arg.input_buf = (xpointer)(unsigned long)input_buf;
	arg.input_size = input_size;
	arg.output_buf = (xpointer)(unsigned long)output_buf;
	arg.output_size = *output_size;
	arg.acc = acc;
	acc++;
	xaipc(p_aml_vsp_ctx->aipchdl, MBX_CODEC_VSP_API_PROCESS, &arg, sizeof(arg));
	*output_size = arg.output_size;
	return arg.ret;
}
