/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __AMLOGIC_DEBUG_MAIN_H
#define __AMLOGIC_DEBUG_MAIN_H

int debug_lockup_init(void);
int cpu_mhz_init(void);

#ifdef CONFIG_AMLOGIC_DEBUG_ATRACE
int meson_atrace_init(void);
#else
static inline int meson_atrace_init(void)
{
	return 0;
}
#endif

int debug_file_init(void);
void debug_file_exit(void);

#endif
