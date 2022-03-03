// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/stddef.h>
#include <linux/amlogic/scpi_protocol.h>

#include "rpc_dev.h"

#define DSPA	0

/** wrapper layer for mailbox at linux kernel */

static void *g_dev;

void RPC_kernel_init(void *dev)
{
	g_dev = dev;
}

int RPC_init(const char *path, int flags, int mode)
{
	return 0;
}

int RPC_deinit(int handle)
{
	return 0;
}

int RPC_invoke(int handle, int cmd, void *data, unsigned int len)
{
	return mbox_message_send_dsp_sync(g_dev, cmd, data, len, DSPA);
}

void show_xxd(void *n, unsigned long bytes, char *label)
{
	u32 *p = n;
	unsigned long i;

	pr_info("p=%lx bytes=%lu label=%s\n", (unsigned long)p, bytes, label);
	for (i = 0; i != bytes / sizeof(u32); i++) {
		pr_info("%08x\n", p[i]);
		if ((i + 1) % 4 == 0)
			pr_info("\n");
	}
	pr_info("\n");
}
