// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/xattr.h>
#include <linux/amlogic/debug_relabel.h>

//this function is only used debug, not be used in normal.
struct file *debug_filp_open(const char *filename, int flags, umode_t mode)
{
	return filp_open(filename, flags, mode);
}
EXPORT_SYMBOL(debug_filp_open);

int
debug_vfs_setxattr(struct dentry *dentry,
		const char *name, const void *value, size_t size, int flags)
{
	return vfs_setxattr(&init_user_ns, dentry, name, value, size, flags);
}
EXPORT_SYMBOL(debug_vfs_setxattr);
