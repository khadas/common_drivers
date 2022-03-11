/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __AMLOGIC_DEBUG_PRINTK_H
#define __AMLOGIC_DEBUG_PRINTK_H

void debug_printk_modify_len(u16 *reserve_size, unsigned long irqflags, unsigned int max_line);
void debug_printk_insert_info(char *text_buf, u16 *text_len);

#endif
