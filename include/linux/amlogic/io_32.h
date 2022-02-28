/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __AMLOGIC_ASM_IO_32_H
#define __AMLOGIC_ASM_IO_32_H

#include <linux/amlogic/debug_ftrace_ramoops.h>

#define __raw_writew __raw_writew
static inline void __raw_writew(u16 val, volatile void __iomem *addr)
{
	pstore_ftrace_io_wr((unsigned long)addr, (unsigned long)val);
	asm volatile("strh %1, %0"
		     : : "Q" (*(volatile u16 __force *)addr), "r" (val));
	pstore_ftrace_io_wr_end((unsigned long)addr, (unsigned long)val);
}

#define __raw_readw __raw_readw
static inline u16 __raw_readw(const volatile void __iomem *addr)
{
	u16 val;

	pstore_ftrace_io_rd((unsigned long)addr);
	asm volatile("ldrh %0, %1"
		     : "=r" (val)
		     : "Q" (*(volatile u16 __force *)addr));
	pstore_ftrace_io_rd_end((unsigned long)addr);
	return val;
}

#define __raw_writeb __raw_writeb
static inline void __raw_writeb(u8 val, volatile void __iomem *addr)
{
	pstore_ftrace_io_wr((unsigned long)addr, (unsigned long)val);
	asm volatile("strb %1, %0"
		     : : "Qo" (*(volatile u8 __force *)addr), "r" (val));
	pstore_ftrace_io_wr_end((unsigned long)addr, (unsigned long)val);
}

#define __raw_writel __raw_writel
static inline void __raw_writel(u32 val, volatile void __iomem *addr)
{
	pstore_ftrace_io_wr((unsigned long)addr, (unsigned long)val);
	asm volatile("str %1, %0"
		     : : "Qo" (*(volatile u32 __force *)addr), "r" (val));
	pstore_ftrace_io_wr_end((unsigned long)addr, (unsigned long)val);
}

#define __raw_readb __raw_readb
static inline u8 __raw_readb(const volatile void __iomem *addr)
{
	u8 val;

	pstore_ftrace_io_rd((unsigned long)addr);
	asm volatile("ldrb %0, %1"
		     : "=r" (val)
		     : "Qo" (*(volatile u8 __force *)addr));
	pstore_ftrace_io_rd_end((unsigned long)addr);
	return val;
}

#define __raw_readl __raw_readl
static inline u32 __raw_readl(const volatile void __iomem *addr)
{
	u32 val;

	pstore_ftrace_io_rd((unsigned long)addr);
	asm volatile("ldr %0, %1"
		     : "=r" (val)
		     : "Qo" (*(volatile u32 __force *)addr));
	pstore_ftrace_io_rd_end((unsigned long)addr);
	return val;
}

static inline void memset_io(volatile void __iomem *dst, unsigned c,
	size_t count)
{
	extern void mmioset(void *, unsigned int, size_t);
	pstore_ftrace_io_memset((unsigned long)dst, (unsigned long)count);
	mmioset((void __force *)dst, c, count);
	pstore_ftrace_io_memset_end((unsigned long)dst, (unsigned long)count);
}

#define memset_io(dst, c, count) memset_io(dst, c, count)

static inline void memcpy_fromio(void *to, const volatile void __iomem *from,
	size_t count)
{
	extern void mmiocpy(void *, const void *, size_t);
	pstore_ftrace_io_copy_from((unsigned long)to, (unsigned long)count);
	mmiocpy(to, (const void __force *)from, count);
	pstore_ftrace_io_copy_from_end((unsigned long)to, (unsigned long)count);
}

#define memcpy_fromio(to, from, count) memcpy_fromio(to, from, count)

static inline void memcpy_toio(volatile void __iomem *to, const void *from,
	size_t count)
{
	extern void mmiocpy(void *, const void *, size_t);
	pstore_ftrace_io_copy_to((unsigned long)to, (unsigned long)count);
	mmiocpy((void __force *)to, from, count);
	pstore_ftrace_io_copy_to_end((unsigned long)to, (unsigned long)count);
}

#define memcpy_toio(to, from, count) memcpy_toio(to, from, count)
#endif /* __AMLOGIC_ASM_IO_32_H */
