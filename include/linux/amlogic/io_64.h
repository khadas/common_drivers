/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __AMLOGIC_ASM_IO_64_H
#define __AMLOGIC_ASM_IO_64_H

#include <linux/amlogic/debug_ftrace_ramoops.h>
/*
 * Generic IO read/write.  These perform native-endian accesses.
 */
#define __raw_writeb __raw_writeb
static inline void __raw_writeb(u8 val, volatile void __iomem *addr)
{
	pstore_ftrace_io_wr((unsigned long)addr, (unsigned long)val);
	asm volatile("strb %w0, [%1]" : : "rZ" (val), "r" (addr));
	pstore_ftrace_io_wr_end((unsigned long)addr, (unsigned long)val);
}

#define __raw_writew __raw_writew
static inline void __raw_writew(u16 val, volatile void __iomem *addr)
{
	pstore_ftrace_io_wr((unsigned long)addr, (unsigned long)val);
	asm volatile("strh %w0, [%1]" : : "rZ" (val), "r" (addr));
	pstore_ftrace_io_wr_end((unsigned long)addr, (unsigned long)val);
}

#define __raw_writel __raw_writel
static __always_inline void __raw_writel(u32 val, volatile void __iomem *addr)
{
	pstore_ftrace_io_wr((unsigned long)addr, (unsigned long)val);
	asm volatile("str %w0, [%1]" : : "rZ" (val), "r" (addr));
	pstore_ftrace_io_wr_end((unsigned long)addr, (unsigned long)val);
}

#define __raw_writeq __raw_writeq
static inline void __raw_writeq(u64 val, volatile void __iomem *addr)
{
	pstore_ftrace_io_wr((unsigned long)addr, (unsigned long)val);
	asm volatile("str %x0, [%1]" : : "rZ" (val), "r" (addr));
	pstore_ftrace_io_wr_end((unsigned long)addr, (unsigned long)val);
}

#define __raw_readb __raw_readb
static inline u8 __raw_readb(const volatile void __iomem *addr)
{
	u8 val;

	pstore_ftrace_io_rd((unsigned long)addr);
	asm volatile(ALTERNATIVE("ldrb %w0, [%1]",
				 "ldarb %w0, [%1]",
				 ARM64_WORKAROUND_DEVICE_LOAD_ACQUIRE)
		     : "=r" (val) : "r" (addr));
	pstore_ftrace_io_rd_end((unsigned long)addr);
	return val;
}

#define __raw_readw __raw_readw
static inline u16 __raw_readw(const volatile void __iomem *addr)
{
	u16 val;

	pstore_ftrace_io_rd((unsigned long)addr);
	asm volatile(ALTERNATIVE("ldrh %w0, [%1]",
				 "ldarh %w0, [%1]",
				 ARM64_WORKAROUND_DEVICE_LOAD_ACQUIRE)
		     : "=r" (val) : "r" (addr));
	pstore_ftrace_io_rd_end((unsigned long)addr);
	return val;
}

#define __raw_readl __raw_readl
static __always_inline u32 __raw_readl(const volatile void __iomem *addr)
{
	u32 val;

	pstore_ftrace_io_rd((unsigned long)addr);
	asm volatile(ALTERNATIVE("ldr %w0, [%1]",
				 "ldar %w0, [%1]",
				 ARM64_WORKAROUND_DEVICE_LOAD_ACQUIRE)
		     : "=r" (val) : "r" (addr));
	pstore_ftrace_io_rd_end((unsigned long)addr);
	return val;
}

#define __raw_readq __raw_readq
static inline u64 __raw_readq(const volatile void __iomem *addr)
{
	u64 val;

	pstore_ftrace_io_rd((unsigned long)addr);
	asm volatile(ALTERNATIVE("ldr %0, [%1]",
				 "ldar %0, [%1]",
				 ARM64_WORKAROUND_DEVICE_LOAD_ACQUIRE)
		     : "=r" (val) : "r" (addr));
	pstore_ftrace_io_rd_end((unsigned long)addr);
	return val;
}

#endif	/* __AMLOGIC_ASM_IO_H */
