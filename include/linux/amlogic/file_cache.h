/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __FILE_CACHE_H__
#define __FILE_CACHE_H__

#include <asm/memory.h>
#include <asm/stacktrace.h>
#include <asm/sections.h>

#define MAX_FCT		2048
struct file_cache_trace {
	unsigned int count;
	unsigned int active_count;
	unsigned int inactive_count;
	unsigned int lock_count;
	unsigned int mapcnt;
	unsigned long off;		/* for find out vma */
	struct address_space *mapping;
	struct rb_node entry;
};
#endif /* __FILE_CACHE_H__ */
