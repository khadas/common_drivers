/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __MESON_MHU_SEC_H__
#define __MESON_MHU_SEC_H__

#include <linux/cdev.h>
#include <linux/mailbox_controller.h>

#define MBOX_SEC_SIZE		0x80
/* u64 compete*/
#define MBOX_COMPETE_LEN	8
int __init mbox_sec_init(void);
void __exit mbox_sec_exit(void);
#endif
