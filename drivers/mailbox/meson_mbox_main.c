// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/of.h>
#include <linux/ctype.h>
#include <linux/kallsyms.h>
#include "meson_mbox_fifo.h"
#include "meson_mbox_pl.h"
#include "meson_mbox_sec.h"
#include "meson_mbox_devfs.h"

static int __init mailbox_init(void)
{
	mbox_fifo_init();
	mbox_pl_v0_init();
	mbox_pl_v1_init();
	mbox_pl_v2_init();
	mbox_sec_init();
	mbox_devfs_init();

	return 0;
}

static void __exit mailbox_exit(void)
{
	mbox_devfs_exit();
	mbox_sec_exit();
	mbox_pl_v2_exit();
	mbox_pl_v1_exit();
	mbox_pl_v0_exit();
	mbox_fifo_exit();
}

module_init(mailbox_init);
module_exit(mailbox_exit);

MODULE_DESCRIPTION("Amlogic MHU driver");
MODULE_LICENSE("GPL v2");

