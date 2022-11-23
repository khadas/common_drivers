/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __MEM_DEBUG_H
#define __MEM_DEBUG_H

void dump_mem_layout(char *buf);
void dump_mem_layout_boot_phase(void);

extern unsigned long mlock_fault_size;
#endif
