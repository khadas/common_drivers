/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __PAGE_TRACE_H__
#define __PAGE_TRACE_H__

#include <asm/memory.h>
#include <asm/stacktrace.h>
#include <asm/sections.h>
#include <linux/page-flags.h>

/*
 * bit map lay out for _ret_ip table
 *
 *  31   28 27 26  24 23             0
 * +------+---+----------------------+
 * |      |   |      |               |
 * +------+---+----------------------+
 *     |    |     |         |
 *     |    |     |         +-------- offset of ip in base address
 *     |    |     +------------------ MIGRATE_TYPE
 *     |    +------------------------ base address select
 *     |                              0: ip base is in kernel address
 *     |                              1: ip base is in module address
 *     +----------------------------- allocate order
 *
 * Note:
 *  offset in ip address is Logical shift right by 2 bits,
 *  because kernel code are always compiled by ARM instruction
 *  set, so pc is aligned by 2. There are 24 bytes used for store
 *  offset in kernel/module, plus these 2 shift bits, You must
 *  make sure your kernel image size is not larger than 2^26 = 64MB
 */
#define IP_ORDER_MASK		(0xf0000000)
#define IP_MODULE_BIT		BIT(27)
#define IP_MIGRATE_MASK		(0x07000000)
#define IP_RANGE_MASK		(0x00ffffff)
 /* max order usually should not be 15 */
#define IP_INVALID		(0xf)

struct page;

/* this struct should not larger than 32 bit */
struct page_trace {
	union {
		struct {
			unsigned int ret_ip       :24;
			unsigned int migrate_type : 3;
			unsigned int module_flag  : 1;
			unsigned int order        : 4;
		};
		unsigned int ip_data;
	};
};

#if defined(CONFIG_TRACEPOINTS) && \
	defined(CONFIG_ANDROID_VENDOR_HOOKS) && \
	IS_BUILTIN(CONFIG_AMLOGIC_PAGE_TRACE)
struct pagetrace_vendor_param {
	struct page_trace *trace_buf;
	unsigned int trace_step;
	unsigned long text;
	unsigned long ip;
};
#endif

#if IS_ENABLED(CONFIG_AMLOGIC_PAGE_TRACE)
u64 get_iow_time(u64 *cpu);
unsigned long unpack_ip(struct page_trace *trace);
unsigned int pack_ip(unsigned long ip, unsigned int order, gfp_t flag);
void set_page_trace(struct page *page, unsigned int order,
		    gfp_t gfp_flags, void *func);
void replace_page_trace(struct page *new, struct page *old);
void reset_page_trace(struct page *page, unsigned int order);
void page_trace_mem_init(void);
struct page_trace *find_page_base(struct page *page);
unsigned long find_back_trace(void);
unsigned long get_page_trace(struct page *page);
int save_obj_stack(unsigned long *stack, int depth);
#else
static inline u64 get_iow_time(u64 *cpu)
{
	return 0;
}
static inline unsigned long unpack_ip(struct page_trace *trace)
{
	return 0;
}

static inline void set_page_trace(struct page *page, unsigned int order, gfp_t gfp_flags)
{
}

static inline void reset_page_trace(struct page *page, unsigned int order)
{
}

static inline void page_trace_mem_init(void)
{
}

static inline struct page_trace *find_page_base(struct page *page)
{
	return NULL;
}

static inline unsigned long find_back_trace(void)
{
	return 0;
}

static inline unsigned long get_page_trace(struct page *page)
{
	return 0;
}

static inline int slab_trace_init(void)
{
	return 0;
}

static inline int slab_trace_add_page(struct page *page, unsigned int order,
				      struct kmem_cache *s, gfp_t flags)
{
	return 0;
}

static inline int slab_trace_remove_page(struct page *page, unsigned int order,
					 struct kmem_cache *s)
{
	return 0;
}

static inline int slab_trace_mark_object(void *object, unsigned long ip,
					 struct kmem_cache *s)
{
	return 0;
}

static inline int slab_trace_remove_object(void *object, struct kmem_cache *s)
{
	return 0;
}

static inline int get_cache_max_order(struct kmem_cache *s)
{
	return 0;
}

static inline int save_obj_stack(unsigned long *stack, int depth)
{
	return 0;
}
#endif

#ifdef CONFIG_KALLSYMS
extern const unsigned long kallsyms_addresses[] __weak;
extern const int kallsyms_offsets[] __weak;
extern const u8 kallsyms_names[] __weak;
extern const unsigned long kallsyms_num_syms __weak __section(".rodata");
extern const unsigned long kallsyms_relative_base __weak __section(".rodata");
extern const u8 kallsyms_token_table[] __weak;
extern const u16 kallsyms_token_index[] __weak;
#endif /* CONFIG_KALLSYMS */

#endif /* __PAGE_TRACE_H__ */
