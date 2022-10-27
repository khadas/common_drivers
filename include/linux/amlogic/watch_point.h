/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __AML_WATCH_POINT_H__
#define __AML_WATCH_POINT_H__

#include <uapi/linux/elf.h>
#include <uapi/linux/hw_breakpoint.h>
#include <linux/perf_event.h>

#define MAX_WATCH_POINTS	16

#ifdef CONFIG_HAVE_HW_BREAKPOINT
extern int aml_watch_point_register(unsigned long addr,
				    unsigned int len,
				    unsigned int type,
				    perf_overflow_handler_t handle);

extern void aml_watch_point_remove(unsigned long addr);
#else
static inline int aml_watch_point_register(unsigned long addr,
					   unsigned int len,
					   unsigned int type,
					   perf_overflow_handler_t handle)
{
	return -1;
}

static inline void aml_watch_point_remove(unsigned long addr)
{
}
#endif
#endif
