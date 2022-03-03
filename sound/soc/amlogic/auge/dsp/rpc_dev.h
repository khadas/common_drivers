/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _RPC_DEV_H_
#define _RPC_DEV_H_

#include <uapi/asm-generic/fcntl.h>
/* for flags of RPC_init */

void RPC_kernel_init(void *dev);
int RPC_init(const char *path, int flags, int mode);
int RPC_deinit(int handle);
int RPC_invoke(int handle, int cmd, void *data, unsigned int len);

void show_xxd(void *p, unsigned long bytes, char *label);

#endif
