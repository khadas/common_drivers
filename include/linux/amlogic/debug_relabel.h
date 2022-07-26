/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __AMLOGIC_DEBUG_RELABEL_H
#define __AMLOGIC_DEBUG_RELABEL_H

struct file *debug_filp_open(const char *filename, int flags, umode_t mode);
int debug_vfs_setxattr(struct dentry *dentry,
		const char *name, const void *value, size_t size, int flags);
#endif
