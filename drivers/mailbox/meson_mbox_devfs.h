/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __MESON_MBOX_DEVFS_H__
#define __MESON_MBOX_DEVFS_H__

#define MBOX_USER_SIZE		0x80

int __init mbox_devfs_init(void);
void __exit mbox_devfs_exit(void);

#endif
