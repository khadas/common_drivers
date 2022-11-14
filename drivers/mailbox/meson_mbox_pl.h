/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __MESON_MBOX_PL_H__
#define __MESON_MBOX_PL_H__

int __init mbox_pl_v0_init(void);
void mbox_pl_v0_exit(void);
int __init mbox_pl_v1_init(void);
void mbox_pl_v1_exit(void);
int __init mbox_pl_v2_init(void);
void mbox_pl_v2_exit(void);
#endif
