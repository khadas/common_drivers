/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

/**
 * shm for transferring memory between arm and hifi
 *
 * Author: Wenjie Zhou <Wenjie.Zhou@amlogic.com>
 * Version:
 * - 0.1        init
 */

#ifndef _RPC_CLIENT_SHM_H_
#define _RPC_CLIENT_SHM_H_

/* #include <stdint.h> */
#include <linux/types.h>

void shm_kernel_init(void *dev);

/**
 *  Shared Memory Handler
 */
/* kernel want physical address as unsigned long, instead of void * */
#define AML_MEM_HANDLE unsigned long

#define AML_MEM_NULL 0

/**
 * Allocate a block of shared memory
 *
 * @param[in] Shared memory size in bytes
 *
 * @return a valid shared memory handler if success, otherwise return 0
 */
AML_MEM_HANDLE aml_mem_allocate(size_t size);

/**
 * Free a block of shared memory
 *
 * @param[in] Shared memory handler
 *
 * @return
 */
void aml_mem_free(AML_MEM_HANDLE h_shm);

/**
 * Copy data from a shared memory to another
 *
 * @param[in] Handler of dest shared memory
 *
 * @param[in] Handler of source shared memory
 *
 * @param[in] shared memory size
 *
 * @return
 */
void aml_mem_transfer(AML_MEM_HANDLE h_dst, AML_MEM_HANDLE h_src, size_t size);

/**
 * Recycle all shared memory allocated by the process
 *
 * @param[in] process id
 *
 * @return
 */
void aml_mem_recycle(int pid);

/**
 * Get the virtual address of a shared memory
 *
 * @param[in] Shared memory handler
 *
 * @return virtual address if successful, otherwise return NULL
 */
void *aml_mem_getvirtaddr(AML_MEM_HANDLE h_shm);

/**
 * Get physical address of a shared memory
 *
 * @param[in] Shared memory handler
 *
 * @return physical address if successful, otherwise return 0
 */
void *aml_mem_getphyaddr(AML_MEM_HANDLE h_shm);

/**
 * Clean cache for a block of shared memory
 *
 * @param[in] Shared memory handler
 *
 * @param[in] The size need to be cleaned
 *
 * @return 0 if successful, return -1 if failed
 */
int32_t aml_mem_clean(AML_MEM_HANDLE h_shm, size_t size);

/**
 * Invalidate cache for a block of shared memory
 *
 * @param[in] Shared memory handler
 *
 * @param[in] The size need to be invalidated
 *
 * @return 0 if successful, return -1 if failed
 */
int32_t aml_mem_invalidate(AML_MEM_HANDLE h_shm, size_t size);

#endif // end _OFFLOAD_ACODEC_MP3_H_
