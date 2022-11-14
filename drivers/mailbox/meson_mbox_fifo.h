/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __MESON_MBOX_FIFO_H__
#define __MESON_MBOX_FIFO_H__

#include <linux/cdev.h>

#define PAYLOAD_OFFSET(chan)	(0x80 * (chan))
#define CTL_OFFSET(chan)	((chan) * 0x4)

#define IRQ_REV_BIT(mbox)	(1ULL << ((mbox) * 2))
#define IRQ_SENDACK_BIT(mbox)	(1ULL << ((mbox) * 2 + 1))

#define IRQ_MASK_OFFSET(x)	(0x00 + ((x) << 2))
#define IRQ_TYPE_OFFSET(x)	(0x10 + ((x) << 2))
#define IRQ_CLR_OFFSET(x)	(0x20 + ((x) << 2))
#define IRQ_STS_OFFSET(x)	(0x30 + ((x) << 2))

#define MBOX_FIFO_SIZE		0x80
#define MHUIRQ_MAXNUM_DEF	32
#define MBOX_IRQSHIFT		32
#define MBOX_IRQMASK		0xffffffff

int __init mbox_fifo_init(void);
void mbox_fifo_exit(void);
#endif
