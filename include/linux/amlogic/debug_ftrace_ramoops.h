/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __DEBUG_FTRACE_RAMOOPS_H__
#define  __DEBUG_FTRACE_RAMOOPS_H__
#include <linux/pstore_ram.h>

extern unsigned int ramoops_ftrace_en;
extern int ramoops_io_en;
extern int ramoops_io_dump;
extern unsigned int dump_iomap;

#define PSTORE_FLAG_FUNC	0x1
#define PSTORE_FLAG_IO_R	0x2
#define PSTORE_FLAG_IO_W	0x3
#define PSTORE_FLAG_IO_R_END	0x4
#define PSTORE_FLAG_IO_W_END	0x5
#define PSTORE_FLAG_IO_TAG	0x6
#define PSTORE_FLAG_MASK	0xF

#define CALLER_ADDR_0 ((unsigned long)__builtin_return_address(0))

void notrace pstore_io_save(unsigned long reg, unsigned long val,
			    unsigned long parant, unsigned int flag,
			    unsigned long *irq_flag);

struct persistent_ram_zone;
void pstore_ftrace_dump_old(struct persistent_ram_zone *prz);

//#define SKIP_IO_TRACE
#if (defined CONFIG_AMLOGIC_DEBUG_FTRACE_PSTORE) && (!defined SKIP_IO_TRACE)
#define pstore_ftrace_io_wr(reg, val)	\
unsigned long irqflg;					\
pstore_io_save(reg, val, CALLER_ADDR_0, PSTORE_FLAG_IO_W, &irqflg)

#define pstore_ftrace_io_wr_end(reg, val)	\
pstore_io_save(reg, val, CALLER_ADDR_0, PSTORE_FLAG_IO_W_END, &irqflg)

#define pstore_ftrace_io_rd(reg)		\
unsigned long irqflg;					\
pstore_io_save(reg, 0, CALLER_ADDR_0, PSTORE_FLAG_IO_R, &irqflg)

#define pstore_ftrace_io_rd_end(reg)	\
pstore_io_save(reg, 0, CALLER_ADDR_0, PSTORE_FLAG_IO_R_END, &irqflg)

#define pstore_ftrace_io_tag(reg, val)  \
pstore_io_save(reg, val, CALLER_ADDR_0, PSTORE_FLAG_IO_TAG, NULL)

#define need_dump_iomap()               (ramoops_io_en | dump_iomap)
#else
#define pstore_ftrace_io_wr(reg, val)                   do {    } while (0)
#define pstore_ftrace_io_rd(reg)                        do {    } while (0)
#define need_dump_iomap()                               0
#define pstore_ftrace_io_wr_end(reg, val)               do {    } while (0)
#define pstore_ftrace_io_rd_end(reg)                    do {    } while (0)
#define pstore_ftrace_io_tag(reg, val)                  do {    } while (0)
#endif /*CONFIG_AMLOGIC_DEBUG_FTRACE_PSTORE && !SKIP_IO_TRACE */

#ifdef CONFIG_AMLOGIC_DEBUG_FTRACE_PSTORE
#define pstore_ftrace_io_copy_from(reg, cnt)	\
unsigned long irqflg;                                   \
pstore_io_save(reg, cnt, CALLER_ADDR_0, PSTORE_FLAG_IO_R, &irqflg)

#define pstore_ftrace_io_copy_from_end(reg, cnt)	\
pstore_io_save(reg, cnt, CALLER_ADDR_0, PSTORE_FLAG_IO_R_END, &irqflg)

#define pstore_ftrace_io_copy_to(reg, cnt)	\
unsigned long irqflg;                                   \
pstore_io_save(reg, 0x12340000 | cnt, CALLER_ADDR_0, PSTORE_FLAG_IO_W, &irqflg)

#define pstore_ftrace_io_copy_to_end(reg, cnt)		\
pstore_io_save(reg, 0x12340000 | cnt, CALLER_ADDR_0, PSTORE_FLAG_IO_W_END, &irqflg)

#define pstore_ftrace_io_memset(reg, cnt)	\
unsigned long irqflg;					\
pstore_io_save(reg, 0xabcd0000 | cnt, CALLER_ADDR_0, PSTORE_FLAG_IO_W, &irqflg)

#define pstore_ftrace_io_memset_end(reg, cnt)		\
pstore_io_save(reg, 0xabcd0000 | cnt, CALLER_ADDR_0, PSTORE_FLAG_IO_W_END, &irqflg)
#else
#define pstore_ftrace_io_copy_from(reg, cnt)		do {	} while (0)
#define pstore_ftrace_io_copy_from_end(reg, cnt)	do {	} while (0)
#define pstore_ftrace_io_copy_to(reg, cnt)		do {	} while (0)
#define pstore_ftrace_io_copy_to_end(reg, cnt)		do {	} while (0)
#define pstore_ftrace_io_memset(reg, cnt)		do {	} while (0)
#define pstore_ftrace_io_memset_end(reg, cnt)		do {	} while (0)
#endif /* CONFIG_AMLOGIC_DEBUG_FTRACE_PSTORE */

#endif
