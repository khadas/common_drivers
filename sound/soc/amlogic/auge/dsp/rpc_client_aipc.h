/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

/**
 * HiFi IPC API
 *
 * Author: Wenjie Zhou <Wenjie.Zhou@amlogic.com>
 * Version:
 * - 0.1        init
 */

#ifndef _RPC_CLIENT_APIC_H_
#define _RPC_CLIENT_APIC_H_

#ifdef __cplusplus
extern "C" {
#endif

int xaudio_ipc_init(int id);
void xaudio_ipc_deinit(int handle);
int xaipc(int handle, unsigned int cmd, void *buf, size_t size);
void show_hex(s8 *p, size_t n, char *label);

#ifdef __cplusplus
}
#endif

#endif
