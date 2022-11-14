/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _MESON_MBOX_COMM_H_
#define _MESON_MBOX_COMM_H_
#include <linux/cdev.h>

struct mbox_cmd_t {
	u32 cmd:16;
	u32 size:9;
	u32 sync:7;
};

union mbox_stat {
	u32 set_cmd;
	struct mbox_cmd_t  mbox_cmd_t;
};
#endif
