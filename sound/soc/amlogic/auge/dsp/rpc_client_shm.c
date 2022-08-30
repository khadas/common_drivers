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
 * audio rpc shm api implementation
 *
 * Author: Wenjie Zhou <Wenjie.Zhou@amlogic.com>
 * Version:
 * - 0.1        init
 */

/* #include <string.h> */
/* #include <stdio.h> */
#include <linux/printk.h>
/* #include <stdlib.h> */
/* #include <string.h> */
/* #include <errno.h> */
#include <linux/errno.h>
/* #include <fcntl.h> */
/* #include <sys/ioctl.h> */
#include <linux/ioctl.h>
/* #include <sys/mman.h> */
#include <linux/dma-mapping.h>
#include <linux/dma-direction.h>
/* #include <unistd.h> */
/* #include <pthread.h> */
#include <asm-generic/io.h>

#include "../../../../drivers/amlogic/hifi4dsp/hifi4dsp_firmware.h"

#include "aipc_type.h"
#include "rpc_client_shm.h"
#include "rpc_client_aipc.h"
#include "hifi4dsp_api.h"

static void *g_dev;

void shm_kernel_init(void *dev)
{
	g_dev = dev;
}

struct shm_info_t {
	int		rpchdl;
	int		fd;
	void		*shm_vir_base;
	long		shm_phy_base;
	size_t		size;
	/* pthread_mutex_t mutex; */
};

struct shm_info_t g_acodec_shmpool_info = {
	.rpchdl		= -1,
	.fd		= -1,
	.shm_vir_base	= NULL,
	.shm_phy_base	= 0,
	.size		= 0,
	/* .mutex		= PTHREAD_MUTEX_INITIALIZER, */
};

static int aml_acodecmemory_init(void)
{
	int ret = 0;
	struct hifi4dsp_firmware *p;
	/* struct hifi4dsp_info_t info; */

	/* pthread_mutex_lock(&g_acodec_shmpool_info.mutex); */
	if (g_acodec_shmpool_info.fd >= 0) {
		ret = 0;
		goto tab_end;
	}

	/**
	 * g_acodec_shmpool_info.fd = open("/dev/hifi4dsp0", O_RDWR);
	 * if (g_acodec_shmpool_info.fd < 0) {
	 * pr_info("open fail:%s\n", strerror(errno));
	 * ret = -1;
	 * goto tab_end;
	 * }

	 * memset(&info, 0, sizeof(info));
	 * ret = ioctl(g_acodec_shmpool_info.fd, HIFI4DSP_GET_INFO, &info);
	 * if (ret < 0) {
	 * pr_info("ioctl invalidate cache fail:%s\n", strerror(errno));
	 * ret = -1;
	 * goto tab_end;
	 * }
	 * g_acodec_shmpool_info.shm_phy_base = info.phy_addr;
	 * g_acodec_shmpool_info.size = info.size;

	 * g_acodec_shmpool_info.shm_vir_base = mmap(NULL,
	 * g_acodec_shmpool_info.size,
	 * PROT_READ | PROT_WRITE, MAP_SHARED,
	 * g_acodec_shmpool_info.fd, 0);
	 */
	p = hifi4dsp_get_firmware(0);
	g_acodec_shmpool_info.fd = 1;
	g_acodec_shmpool_info.shm_phy_base = p->paddr;; /* alloc-ranges aml_dsp_memory */
	g_acodec_shmpool_info.size = p->size;
	g_acodec_shmpool_info.shm_vir_base = phys_to_virt(g_acodec_shmpool_info.shm_phy_base);

	pr_info("SHM fd=%d Vir=%lx Phy=0x%lx size=%zu/0x%zx\n",
		g_acodec_shmpool_info.fd, (unsigned long)g_acodec_shmpool_info.shm_vir_base,
		g_acodec_shmpool_info.shm_phy_base, g_acodec_shmpool_info.size,
		g_acodec_shmpool_info.size);
	g_acodec_shmpool_info.rpchdl = xaudio_ipc_init(0);
tab_end:
	/* pthread_mutex_unlock(&g_acodec_shmpool_info.mutex); */

	return ret;
}

AML_MEM_HANDLE aml_mem_allocate(size_t size)
{
	struct acodec_shm_alloc_st arg;

	arg.h_shm = 0;
	arg.size = size;
	arg.pid = 0; /* TODO: pid as owner of SHM getpid(); */
	if (g_acodec_shmpool_info.rpchdl < 0) {
		if (aml_acodecmemory_init()) {
			pr_info("Initialize audio codec shm pool fail\n");
			return AML_MEM_NULL;
		}
	}
	xaipc(g_acodec_shmpool_info.rpchdl, MBX_CMD_SHM_ALLOC, &arg, sizeof(arg));
	/* pr_info("alloc h_shm=%llx uint32_t %x h=%x\n", */
	/* arg.h_shm, (uint32_t)arg.h_shm, (uint32_t)arg.h_shm); */
	return (uint32_t)arg.h_shm;
}

void aml_mem_free(AML_MEM_HANDLE h_shm)
{
	struct acodec_shm_free_st arg;

	arg.h_shm = (aml_mem_handle_rpc)(int32_t)h_shm;
	xaipc(g_acodec_shmpool_info.rpchdl, MBX_CMD_SHM_FREE, &arg, sizeof(arg));
}

void aml_mem_transfer(AML_MEM_HANDLE h_dst, AML_MEM_HANDLE h_src, size_t size)
{
	struct acodec_shm_transfer_st arg;

	arg.h_src = (aml_mem_handle_rpc)(int32_t)h_src;
	arg.h_dst = (aml_mem_handle_rpc)(int32_t)h_dst;
	arg.size = size;
	xaipc(g_acodec_shmpool_info.rpchdl, MBX_CMD_SHM_TRANSFER, &arg, sizeof(arg));
}

void aml_mem_recycle(int pid)
{
	struct acodec_shm_recycle_st arg;

	arg.pid = pid;
	xaipc(g_acodec_shmpool_info.rpchdl, MBX_CMD_SHM_RECYCLE, &arg, sizeof(arg));
}

void *aml_mem_getvirtaddr(AML_MEM_HANDLE h_shm)
{
	void *p_vir = NULL;

	if (g_acodec_shmpool_info.rpchdl < 0) {
		if (aml_acodecmemory_init()) {
			pr_info("Initialize audio codec shm pool fail\n");
			return NULL;
		}
	}
	/** disable range checking, due to hifi4dsp_get_firmware's wrong size */
	/* ((long)h_shm < g_acodec_shmpool_info.shm_phy_base + g_acodec_shmpool_info.size)) */
	if (((long)h_shm >= g_acodec_shmpool_info.shm_phy_base) && true) {
		p_vir = (void *)(((long)h_shm - g_acodec_shmpool_info.shm_phy_base) +
				 (long)g_acodec_shmpool_info.shm_vir_base);
		return p_vir;
	}
	pr_info("h_shm=%lx is NOT in range phy=%lx size=%zu/0x%zx\n",
		h_shm, g_acodec_shmpool_info.shm_phy_base,
		g_acodec_shmpool_info.size, g_acodec_shmpool_info.size);
	return NULL;
}

void *aml_mem_getphyaddr(AML_MEM_HANDLE h_shm)
{
	return (void *)h_shm;
}

int32_t aml_mem_clean(AML_MEM_HANDLE phy, size_t size)
{
	int ret = 0;
	struct hifi4_shm_info_t info;

	info.addr = (long)phy;
	info.size = size;

	if (g_acodec_shmpool_info.rpchdl < 0) {
		if (aml_acodecmemory_init()) {
			pr_info("Initialize audio codec shm pool fail\n");
			return -1;
		}
	}
	/* ret = ioctl(g_acodec_shmpool_info.fd, HIFI4DSP_SHM_CLEAN, &info); */
	hifi4dsp_shm_clean(0, phy, size);
	if (ret < 0) {
		pr_info("ioctl clean cache fail\n");
		return -1;
	}
	return 0;
}

int32_t aml_mem_invalidate(AML_MEM_HANDLE phy, size_t size)
{
	int ret = 0;
	struct hifi4_shm_info_t info;

	info.addr = (long)phy;
	info.size = size;

	if (g_acodec_shmpool_info.rpchdl < 0) {
		if (aml_acodecmemory_init()) {
			pr_info("Initialize audio codec shm pool fail\n");
			return -1;
		}
	}
	/* ret = ioctl(g_acodec_shmpool_info.fd, HIFI4DSP_SHM_INV, &info); */
	hifi4dsp_shm_invalidate(0, phy, size);
	if (ret < 0) {
		pr_info("ioctl invalidate cache fail\n");
		return -1;
	}
	return 0;
}
