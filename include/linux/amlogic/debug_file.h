/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __AMLOGIC_DEBUG_FILE_H
#define __AMLOGIC_DEBUG_FILE_H

#include <linux/kfifo.h>
#include <linux/list.h>
#include <linux/fcntl.h>

#define DEBUG_FILE_NAME_LEN 200
struct debug_file_param {
	int id;
	int status;
	int flags;
	unsigned int mode;
	char filename[DEBUG_FILE_NAME_LEN];
};

struct debug_file {
	struct delayed_work release_work;
	struct kfifo kfifo_buf;
	struct list_head list;
	struct mutex mutex; /* */
	struct proc_dir_entry *proc_file;
	struct debug_file_param param;
	wait_queue_head_t wq;
};

struct debug_file *debug_file_open(const char *filename, int flags, umode_t mode);
ssize_t debug_file_read(struct debug_file *filp, void *buf, size_t count);
ssize_t debug_file_write(struct debug_file *filp, const void *buf, size_t count);
int debug_file_close(struct debug_file *filp);

#endif
