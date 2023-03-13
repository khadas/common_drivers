/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __AMLOGIC_FREE_RESERVED_H__
#define __AMLOGIC_FREE_RESERVED_H__

#include <linux/kconfig.h>

#if IS_ENABLED(CONFIG_AMLOGIC_MEMORY_DEBUG)
unsigned long aml_free_reserved_area(void *start, void *end, int poison, const char *s);
#else
static inline unsigned long aml_free_reserved_area(void *start,
			void *end, int poison, const char *s)
{
	return 0;
}
#endif

#endif
