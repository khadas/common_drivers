// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

/*
 * Copyright (C) 2019 Amlogic, Inc. All rights reserved.
 *
 * All information contained herein is Amlogic confidential.
 *
 * This software is provided to you pursuant to Software License
 * Agreement (SLA) with Amlogic Inc ("Amlogic"). This software may be
 * used only in accordance with the terms of this agreement.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification is strictly prohibited without prior written permission
 * from Amlogic.
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
 *
 */

/* #include <unistd.h> */
/* #include <sys/types.h> */
/* #include <sys/stat.h> */
/* #include <fcntl.h> */
#include <linux/ioctl.h>
/* #include <stdio.h> */
#include <linux/printk.h>
/* #include <string.h> */
#include "rpc_dev.h"

int xaudio_ipc_init(int id)
{
	if (id == 0)
		return RPC_init("/dev/dsp_dev", O_RDWR, 0);
	else if (id == 1)
		return RPC_init("/dev/dspb_dev", O_RDWR, 0);
	pr_info("Invalid rpc device:%d\n", id);
	return -1;
}

void xaudio_ipc_deinit(int handle)
{
	RPC_deinit(handle);
}

#define CHUNK 4
#define GROUP 16
void show_hex(s8 *p, size_t n, char *label)
{
	size_t i, m;

	if (!label)
		label = "unknown";
	/** use %lx/%px to pring unmodified address */
	pr_info("label=%s buf=%lx size=%zu/0x%zx\n",
		label, (unsigned long)p, n, n);
	m = n / CHUNK + ((n % CHUNK) != 0);
	for (i = 0; i != m; i++) {
		s8 *q;

		q = p + i * CHUNK;
		pr_info("%lx: %02X %02X %02X %02X\n",
			(unsigned long)q, q[0], q[1], q[2], q[3]);
	}
}

static size_t cnt;
int xaipc(int handle, unsigned int cmd, void *buf, size_t size)
{
	int r;
	/* showHex(buf, size, "enter"); */
	/* pr_info("cnt=%d cmd=%x buf=%lx size=%zu invoke\n", */
	/* cnt, cmd, (unsigned long)buf, size); */
	r = RPC_invoke(handle, cmd, buf, size);
	/* pr_info("cnt=%d cmd=%x buf=%lx size=%zu END\n", */
	/* cnt, cmd, (unsigned long)buf, size); */
	cnt++;
	/* showHex(buf, size, "leave"); */
	return r;
}
