/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __AMLOGIC_DEBUG_PRINTK_H
#define __AMLOGIC_DEBUG_PRINTK_H

void trace_android_vh_printk_caller_id(u32 *caller_id);
void trace_android_vh_printk_caller(char *caller, size_t size, u32 id, int *ret);
#endif
