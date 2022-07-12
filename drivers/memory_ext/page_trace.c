// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/gfp.h>
#include <linux/kallsyms.h>
#include <linux/mmzone.h>
#include <linux/memblock.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/sort.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>
#include <linux/memcontrol.h>
#include <linux/mm_inline.h>
#include <linux/proc_fs.h>
#include <linux/sched/clock.h>
#include <linux/gfp.h>
#include <linux/mm.h>
#include <linux/amlogic/page_trace.h>
#ifdef CONFIG_AMLOGIC_CMA
#include <linux/amlogic/aml_cma.h>
#endif
#include <asm/stacktrace.h>
#include <asm/sections.h>

#ifndef CONFIG_AMLOGIC_PAGE_TRACE_INLINE
#define DEBUG_PAGE_TRACE	0
#else
#define DEBUG_PAGE_TRACE	0
#endif

#ifdef CONFIG_NUMA
#define COMMON_CALLER_SIZE	64	/* more function names if NUMA */
#else
#define COMMON_CALLER_SIZE	64
#endif

/*
 * this is a driver which will hook during page alloc/free and
 * record caller of each page to a buffer. Detailed information
 * of page allocate statistics can be find in /proc/pagetrace
 *
 */
static bool merge_function = 1;
static int page_trace_filter = 64; /* not print size < page_trace_filter */
static struct proc_dir_entry *d_pagetrace;
#ifndef CONFIG_AMLOGIC_PAGE_TRACE_INLINE
struct page_trace *trace_buffer;
static unsigned long ptrace_size;
static unsigned int trace_step = 1;
static bool page_trace_disable __initdata;
#endif

struct alloc_caller {
	unsigned long func_start_addr;
	unsigned long size;
};

struct fun_symbol {
	const char *name;
	int full_match;
	int matched;
};

static struct alloc_caller common_caller[COMMON_CALLER_SIZE];

/*
 * following functions are common API from page allocate, they should not
 * be record in page trace, parse for back trace should keep from these
 * functions
 */
static struct fun_symbol common_func[] __initdata = {
	{"__alloc_pages",		1, 0},
	{"kmem_cache_alloc",		1, 0},
	{"__get_free_pages",		1, 0},
	{"__kmalloc",			1, 0},
	{"cma_alloc",			1, 0},
	{"dma_alloc_from_contiguous",	1, 0},
	{"dma_alloc_contiguous",	1, 0},
	{"alloc_slab_page",		1, 0},
	{"aml_cma_alloc_post_hook",	1, 0},
#ifdef CONFIG_ARM
	{"__dma_alloc",			1, 0},
	{"arm_dma_alloc",		1, 0},
	{"__alloc_from_contiguous",	1, 0},
	{"cma_allocator_alloc",		1, 0},
#endif
	{"__kmalloc_track_caller",	1, 0},
	{"kmem_cache_alloc_trace",	1, 0},
	{"alloc_pages_exact",		1, 0},
	{"alloc_pages_exact_nid",	1, 0},
	{"get_zeroed_page",		1, 0},
	{"__vmalloc_node_range",	1, 0},
	{"__vmalloc_area_node",		1, 0},
	{"sk_prot_alloc",		1, 0},
	{"__alloc_skb",			1, 0},
	{"__vmalloc_node_flags",	0, 0},
	{"vzalloc",			1, 0},
	{"vmalloc",			1, 0},
	{"__vmalloc",			1, 0},
	{"kzalloc",			1, 0},
	{"kstrdup_const",		1, 0},
	{"kvmalloc_node",		1, 0},
	{"kmalloc_order",		1, 0},
	{"kmalloc_order_trace",		1, 0},
	{"aml_slub_alloc_large",	1, 0},
#ifdef CONFIG_NUMA
	{"alloc_pages_current",		1, 0},
	{"alloc_page_interleave",	1, 0},
	{"kmalloc_large_node",		1, 0},
	{"__kmalloc_node",		1, 0},
	{"alloc_pages_vma",		1, 0},
#endif
#ifdef CONFIG_SLUB	/* for some static symbols not exported in headfile */
	{"new_slab",			0, 0},
	{"__slab_alloc",		0, 0},
	{"allocate_slab",		0, 0},
#endif
	{}		/* tail */
};

static inline bool page_trace_invalid(struct page_trace *trace)
{
	return trace->order == IP_INVALID;
}

#ifndef CONFIG_AMLOGIC_PAGE_TRACE_INLINE
static int __init early_page_trace_param(char *buf)
{
	if (!buf)
		return -EINVAL;

	if (strcmp(buf, "off") == 0)
		page_trace_disable = true;
	else if (strcmp(buf, "on") == 0)
		page_trace_disable = false;

	pr_info("page_trace %sabled\n", page_trace_disable ? "dis" : "en");

	return 0;
}

early_param("page_trace", early_page_trace_param);

static int early_page_trace_step(char *buf)
{
	if (!buf)
		return -EINVAL;

	if (!kstrtouint(buf, 10, &trace_step))
		pr_info("page trace_step:%d\n", trace_step);

	return 0;
}

early_param("page_trace_step", early_page_trace_step);
#endif

#if DEBUG_PAGE_TRACE
static inline bool range_ok(struct page_trace *trace)
{
	unsigned long offset;

	offset = (trace->ret_ip << 2);
	if (trace->module_flag) {
		if (offset >= MODULES_END)
			return false;
	} else {
		if (offset >= ((unsigned long)_end - (unsigned long)_text))
			return false;
	}
	return true;
}

static bool check_trace_valid(struct page_trace *trace)
{
	unsigned long offset;

	if (trace->order == IP_INVALID)
		return true;

	if (trace->order >= 10 ||
	    trace->migrate_type >= MIGRATE_TYPES ||
	    !range_ok(trace)) {
		offset = (unsigned long)((trace - trace_buffer));
		pr_err("bad trace:%p, %x, pfn:%lx, ip:%ps\n", trace,
			*((unsigned int *)trace),
			offset / sizeof(struct page_trace),
			(void *)_RET_IP_);
		return false;
	}
	return true;
}
#endif /* DEBUG_PAGE_TRACE */

#ifdef CONFIG_AMLOGIC_PAGE_TRACE_INLINE
static void push_ip(struct page_trace *base, struct page_trace *ip)
{
	*base = *ip;
}
#else
static void push_ip(struct page_trace *base, struct page_trace *ip)
{
	int i;
	unsigned long end;

	for (i = trace_step - 1; i > 0; i--)
		base[i] = base[i - 1];

	/* debug check */
#if DEBUG_PAGE_TRACE
	check_trace_valid(base);
#endif
	end = (((unsigned long)trace_buffer) + ptrace_size);
	WARN_ON((unsigned long)(base + trace_step - 1) >= end);

	base[0] = *ip;
}
#endif /* CONFIG_AMLOGIC_PAGE_TRACE_INLINE */

static inline int is_module_addr(unsigned long ip)
{
	if (ip >= MODULES_VADDR && ip < MODULES_END)
		return 1;
	return 0;
}

/*
 * set up information for common caller in memory allocate API
 */
static int __init setup_common_caller(unsigned long kaddr)
{
	unsigned long size, offset;
	int i = 0, ret;

	for (i = 0; i < COMMON_CALLER_SIZE; i++) {
		/* find a empty caller */
		if (!common_caller[i].func_start_addr)
			break;
	}
	if (i >= COMMON_CALLER_SIZE) {
		pr_err("%s, out of memory\n", __func__);
		return -1;
	}

	ret = kallsyms_lookup_size_offset(kaddr, &size, &offset);
	if (ret) {
		common_caller[i].func_start_addr = kaddr;
		common_caller[i].size = size;
		pr_debug("setup %d caller:%lx + %lx, %ps\n",
			i, kaddr, size, (void *)kaddr);
		return 0;
	}

	pr_err("can't find symbol %ps\n", (void *)kaddr);
	return -1;
}

static void __init dump_common_caller(void)
{
	int i;

	for (i = 0; i < COMMON_CALLER_SIZE; i++) {
		if (common_caller[i].func_start_addr)
			pr_debug("%2d, addr:%lx + %4lx, %ps\n", i,
				common_caller[i].func_start_addr,
				common_caller[i].size,
				(void *)common_caller[i].func_start_addr);
		else
			break;
	}
}

static int __init sym_cmp(const void *x1, const void *x2)
{
	struct alloc_caller *p1, *p2;

	p1 = (struct alloc_caller *)x1;
	p2 = (struct alloc_caller *)x2;

	/* desending order */
	return p1->func_start_addr < p2->func_start_addr ? 1 : -1;
}

static int __init match_common_caller(void *data, const char *name,
				       struct module *module,
				       unsigned long addr)
{
	int i, ret;
	struct fun_symbol *s;

	if (module)
		return -1;

	if (!strcmp(name, "_etext"))	/* end of text */
		return -1;

	for (i = 0; i < ARRAY_SIZE(common_func); i++) {
		s = &common_func[i];
		if (!s->name)
			break;		/* end */
		if (s->full_match && !s->matched) {
			if (!strcmp(name, s->name)) {	/* strict match */
				ret = setup_common_caller(addr);
				s->matched = 1;
				break;
			}
		} else if (!s->full_match) {
			if (strstr(name, s->name)) {	/* contians */
				setup_common_caller(addr);
				break;
			}
		}
	}
	return 0;
}

/* kallsyms_sym_address/kallsyms_expand_symbol/kallsyms_for_each_symbol are
 * copied from kallsyms.c
 */
static unsigned long __init kallsyms_sym_addr(int idx)
{
	if (!IS_ENABLED(CONFIG_KALLSYMS_BASE_RELATIVE))
		return kallsyms_addresses[idx];

	/* values are unsigned offsets if --absolute-percpu is not in effect */
	if (!IS_ENABLED(CONFIG_KALLSYMS_ABSOLUTE_PERCPU))
		return kallsyms_relative_base + (u32)kallsyms_offsets[idx];

	/* ...otherwise, positive offsets are absolute values */
	if (kallsyms_offsets[idx] >= 0)
		return kallsyms_offsets[idx];

	/* ...and negative offsets are relative to kallsyms_relative_base - 1 */
	return kallsyms_relative_base - 1 - kallsyms_offsets[idx];
}

static unsigned int __init kallsyms_expand_sym(unsigned int off,
					char *result, size_t maxlen)
{
	int len, skipped_first = 0;
	const char *tptr;
	const u8 *data;

	/* Get the compressed symbol length from the first symbol byte. */
	data = &kallsyms_names[off];
	len = *data;
	data++;

	/*
	 * Update the offset to return the offset for the next symbol on
	 * the compressed stream.
	 */
	off += len + 1;

	/*
	 * For every byte on the compressed symbol data, copy the table
	 * entry for that byte.
	 */
	while (len) {
		tptr = &kallsyms_token_table[kallsyms_token_index[*data]];
		data++;
		len--;

		while (*tptr) {
			if (skipped_first) {
				if (maxlen <= 1)
					goto tail;
				*result = *tptr;
				result++;
				maxlen--;
			} else {
				skipped_first = 1;
			}
			tptr++;
		}
	}

tail:
	if (maxlen)
		*result = '\0';

	/* Return to offset to the next symbol. */
	return off;
}

static int __init kallsyms_for_each_symbol(int (*fn)(void *, const char *,
						     struct module *,
						     unsigned long),
					   void *data)
{
	char namebuf[KSYM_NAME_LEN];
	unsigned long i;
	unsigned int off;
	int ret;

	for (i = 0, off = 0; i < kallsyms_num_syms; i++) {
		off = kallsyms_expand_sym(off, namebuf, ARRAY_SIZE(namebuf));
		ret = fn(data, namebuf, NULL, kallsyms_sym_addr(i));
		if (ret != 0)
			return ret;
	}
	return 0;
}

static void __init find_static_common_symbol(void)
{
	memset(common_caller, 0, sizeof(common_caller));
	kallsyms_for_each_symbol(match_common_caller, NULL);
	sort(common_caller, COMMON_CALLER_SIZE, sizeof(struct alloc_caller),
		sym_cmp, NULL);
	dump_common_caller();
}

static int is_common_caller(struct alloc_caller *caller, unsigned long pc)
{
	int ret = 0;
	int low = 0, high = COMMON_CALLER_SIZE - 1, mid;
	unsigned long add_l, add_h;

	while (1) {
		mid = (high + low) / 2;
		add_l = caller[mid].func_start_addr;
		add_h = caller[mid].func_start_addr + caller[mid].size;
		if (pc >= add_l && pc < add_h) {
			ret = 1;
			break;
		}

		if (low >= high)	/* still not match */
			break;

		if (pc < add_l)		/* caller is desending order */
			low = mid + 1;
		else
			high = mid - 1;

		/* fix range */
		if (high < 0)
			high = 0;
		if (low > (COMMON_CALLER_SIZE - 1))
			low = COMMON_CALLER_SIZE - 1;
	}
	return ret;
}

unsigned long unpack_ip(struct page_trace *trace)
{
	unsigned long text;

	if (trace->order == IP_INVALID)
		return 0;

	if (trace->module_flag)
		text = MODULES_VADDR;
	else
		text = (unsigned long)_text;
	return text + ((trace->ret_ip) << 2);
}
EXPORT_SYMBOL(unpack_ip);

unsigned long find_back_trace(void)
{
	struct stackframe frame;
	int ret, step = 0;

#ifdef CONFIG_ARM64
	frame.fp = (unsigned long)__builtin_frame_address(0);
	frame.pc = _RET_IP_;
#ifdef CONFIG_FUNCTION_GRAPH_TRACER
	frame.graph = 0;
#endif
	bitmap_zero(frame.stacks_done, __NR_STACK_TYPES);
	frame.prev_fp = 0;
	frame.prev_type = STACK_TYPE_UNKNOWN;
#else
	frame.fp = (unsigned long)__builtin_frame_address(0);
	frame.sp = current_stack_pointer;
	frame.lr = (unsigned long)__builtin_return_address(0);
	frame.pc = (unsigned long)find_back_trace;
#endif
	while (1) {
	#ifdef CONFIG_ARM64
		ret = unwind_frame(current, &frame);
	#elif defined(CONFIG_ARM)
		ret = unwind_frame(&frame);
	#else	/* not supported */
		ret = -1;
	#endif
		if (ret < 0)
			return 0;
		step++;
		if (!is_common_caller(common_caller, frame.pc) && step > 1)
			return frame.pc;
	}
	pr_info("can't get pc\n");
	dump_stack();
	return 0;
}

#ifdef CONFIG_AMLOGIC_PAGE_TRACE_INLINE
struct page_trace *find_page_base(struct page *page)
{
	struct page_trace *trace;

	trace = (struct page_trace *)&page->trace;
	return trace;
}
#else
struct page_trace *find_page_base(struct page *page)
{
	unsigned long pfn, zone_offset = 0, offset;
	struct zone *zone;
	struct page_trace *p;

	if (!trace_buffer)
		return NULL;

	pfn = page_to_pfn(page);
	for_each_populated_zone(zone) {
		/* pfn is in this zone */
		if (pfn <= zone_end_pfn(zone) &&
		    pfn >= zone->zone_start_pfn) {
			offset = pfn - zone->zone_start_pfn;
			p = trace_buffer;
			return p + ((offset + zone_offset) * trace_step);
		}
		/* next zone */
		zone_offset += zone->spanned_pages;
	}
	return NULL;
}
#endif
EXPORT_SYMBOL(find_page_base);

unsigned long get_page_trace(struct page *page)
{
	struct page_trace *trace;

	trace = find_page_base(page);
	if (trace)
		return unpack_ip(trace);

	return 0;
}
EXPORT_SYMBOL(get_page_trace);

#ifndef CONFIG_AMLOGIC_PAGE_TRACE_INLINE
static void __init set_init_page_trace(struct page *page, unsigned int order, gfp_t flag)
{
	unsigned long text, ip;
	struct page_trace trace = {}, *base;

	if (page && trace_buffer) {
		ip = (unsigned long)set_page_trace;
		text = (unsigned long)_text;

		trace.ret_ip = (ip - text) >> 2;
		trace.migrate_type = gfp_migratetype(flag);
		trace.order = order;
		base = find_page_base(page);
		push_ip(base, &trace);
	}
}
#endif

unsigned int pack_ip(unsigned long ip, unsigned int order, gfp_t flag)
{
	unsigned long text;
	struct page_trace trace = {};

	text = (unsigned long)_text;
	if (ip >= (unsigned long)_text) {
		text = (unsigned long)_text;
	} else if (is_module_addr(ip)) {
		text = MODULES_VADDR;
		trace.module_flag = 1;
	}

	trace.ret_ip = (ip - text) >> 2;
#ifdef CONFIG_AMLOGIC_CMA
	if (flag == __GFP_NO_CMA)
		trace.migrate_type = MIGRATE_CMA;
	else
		trace.migrate_type = gfp_migratetype(flag);
#else
	trace.migrate_type = gfp_migratetype(flag);
#endif /* CONFIG_AMLOGIC_CMA */
	trace.order = order;
#if DEBUG_PAGE_TRACE
	pr_debug("%s, base:%p, page:%lx, _ip:%x, o:%d, f:%x, ip:%lx\n",
		 __func__, base, page_to_pfn(page),
		 (*((unsigned int *)&trace)), order,
		 flag, ip);
#endif
	return *((unsigned int *)&trace);
}
EXPORT_SYMBOL(pack_ip);

void set_page_trace(struct page *page, unsigned int order, gfp_t flag, void *func)
{
	unsigned long ip;
	struct page_trace *base;
	unsigned int val, i;

	if (!page)
		return;
#ifndef CONFIG_AMLOGIC_PAGE_TRACE_INLINE
	if (!trace_buffer)
		return;
#endif
	if (!func)
		ip = find_back_trace();
	else
		ip = (unsigned long)func;

	if (!ip) {
		pr_debug("can't find backtrace for page:%lx\n",
			 page_to_pfn(page));
		return;
	}
	val = pack_ip(ip, order, flag);
	base = find_page_base(page);
	push_ip(base, (struct page_trace *)&val);
	if (order) {
		/* in order to easy get trace for high order alloc */
		val = pack_ip(ip, 0, flag);
		for (i = 1; i < (1 << order); i++) {
		#ifdef CONFIG_AMLOGIC_PAGE_TRACE_INLINE
			base = find_page_base(++page);
		#else
			base += (trace_step);
		#endif
			push_ip(base, (struct page_trace *)&val);
		}
	}
}
EXPORT_SYMBOL(set_page_trace);

#ifdef CONFIG_AMLOGIC_PAGE_TRACE_INLINE
void reset_page_trace(struct page *page, unsigned int order)
{
	struct page_trace *base;
	struct page *p;
	int i, cnt;

	if (page) {
		cnt = 1 << order;
		p = page;
		for (i = 0; i < cnt; i++) {
			base = find_page_base(p);
			base->order = IP_INVALID;
			p++;
		}
	}
}
#else
void reset_page_trace(struct page *page, unsigned int order)
{
	struct page_trace *base;
	int i, cnt;
#if DEBUG_PAGE_TRACE
	unsigned long end;
#endif

	if (page && trace_buffer) {
		base = find_page_base(page);
		cnt = 1 << order;
	#if DEBUG_PAGE_TRACE
		check_trace_valid(base);
		end = ((unsigned long)trace_buffer + ptrace_size);
		WARN_ON((unsigned long)(base + cnt * trace_step - 1) >= end);
	#endif
		for (i = 0; i < cnt; i++) {
			base->order = IP_INVALID;
			base += (trace_step);
		}
	}
}
#endif
EXPORT_SYMBOL(reset_page_trace);

void replace_page_trace(struct page *new, struct page *old)
{
	struct page_trace *old_trace, *new_trace;

	if (!new || !old)
		return;

	old_trace  = find_page_base(old);
	new_trace  = find_page_base(new);
	*new_trace = *old_trace;
}

#ifndef CONFIG_AMLOGIC_PAGE_TRACE_INLINE
/*
 * move page out of buddy and make sure they are not malloced by
 * other module
 *
 * Note:
 * before call this functions, memory for page trace buffer are
 * freed to buddy.
 */
static int __init page_trace_pre_work(unsigned long size)
{
	unsigned int order = get_order(size);
	unsigned long addr;
	unsigned long used, end;
	struct page *page;

	if (order >= MAX_ORDER) {
		pr_err("trace buffer size %lx is too large\n", size);
		return -1;
	}
	addr = __get_free_pages(GFP_KERNEL, order);
	if (!addr)
		return -ENOMEM;

	trace_buffer = (struct page_trace *)addr;
	end  = addr + (PAGE_SIZE << order);
	used = addr + size;
	pr_info("%s, trace buffer:%p, size:%lx, used:%lx, end:%lx\n",
		__func__, trace_buffer, size, used, end);
	memset((void *)addr, 0, size);

	for (; addr < end; addr += PAGE_SIZE) {
		if (addr < used) {
			page = virt_to_page((void *)addr);
			set_init_page_trace(page, 0, GFP_KERNEL);
		#if DEBUG_PAGE_TRACE
			pr_info("%s, trace page:%p, %lx\n",
				__func__, page, page_to_pfn(page));
		#endif
		} else {
			free_page(addr);
		#if DEBUG_PAGE_TRACE
			pr_info("%s, free page:%lx, %lx\n", __func__,
				addr, page_to_pfn(virt_to_page(addr)));
		#endif
		}
	}
	return 0;
}
#endif

/*--------------------------sysfs node -------------------------------*/
#define LARGE	1024
#define SMALL	256

/* caller for unmovalbe are max */
#define MT_UNMOVABLE_IDX	0                            /* 0,UNMOVABLE   */
#define MT_MOVABLE_IDX		(MT_UNMOVABLE_IDX   + LARGE) /* 1,MOVABLE     */
#define MT_RECLAIMABLE_IDX	(MT_MOVABLE_IDX     + SMALL) /* 2,RECLAIMABLE */
#define MT_HIGHATOMIC_IDX	(MT_RECLAIMABLE_IDX + SMALL) /* 3,HIGHATOMIC  */
#define MT_CMA_IDX		(MT_HIGHATOMIC_IDX  + SMALL) /* 4,CMA         */
#define MT_ISOLATE_IDX		(MT_CMA_IDX         + SMALL) /* 5,ISOLATE     */

#define SHOW_CNT		(MT_ISOLATE_IDX)

static int mt_offset[] = {
	MT_UNMOVABLE_IDX,
	MT_MOVABLE_IDX,
	MT_RECLAIMABLE_IDX,
	MT_HIGHATOMIC_IDX,
	MT_CMA_IDX,
	MT_ISOLATE_IDX,
	MT_ISOLATE_IDX + SMALL
};

struct page_summary {
	struct rb_node entry;
	unsigned long ip;
	unsigned long cnt;
};

struct pagetrace_summary {
	struct page_summary *sum;
	unsigned long ticks;
	int mt_cnt[MIGRATE_TYPES];
};

static unsigned long find_ip_base(unsigned long ip)
{
	unsigned long size, offset;
	int ret;

	ret = kallsyms_lookup_size_offset(ip, &size, &offset);
	if (ret)	/* find */
		return ip - offset;
	else		/* not find */
		return ip;
}

static int find_page_ip(struct page_trace *trace, struct page_summary *sum,
			int range, int *mt_cnt, struct rb_root *root)
{
	unsigned long ip;
	struct rb_node **link, *parent = NULL;
	struct page_summary *tmp;

	ip = unpack_ip(trace);
	if (!ip || !trace->ip_data)	/* invalid ip */
		return 0;

	link  = &root->rb_node;
	while (*link) {
		parent = *link;
		tmp = rb_entry(parent, struct page_summary, entry);
		if (ip == tmp->ip) { /* match */
			tmp->cnt++;
			return 0;
		} else if (ip < tmp->ip) {
			link = &tmp->entry.rb_left;
		} else {
			link = &tmp->entry.rb_right;
		}
	}
	/* not found, get a new page summary */
	if (mt_cnt[trace->migrate_type] >= range)
		return -ERANGE;
	tmp       = &sum[mt_cnt[trace->migrate_type]];
	tmp->ip   = ip;
	tmp->cnt++;
	mt_cnt[trace->migrate_type]++;
	rb_link_node(&tmp->entry, parent, link);
	rb_insert_color(&tmp->entry, root);
	return 0;
}

#define K(x)		((unsigned long)(x) << (PAGE_SHIFT - 10))
static int trace_cmp(const void *x1, const void *x2)
{
	struct page_summary *s1, *s2;

	s1 = (struct page_summary *)x1;
	s2 = (struct page_summary *)x2;
	return s2->cnt - s1->cnt;
}

static void show_page_trace(struct seq_file *m, struct zone *zone,
			    struct page_summary *sum, int *mt_cnt)
{
	int i, j;
	struct page_summary *p;
	unsigned long total_mt, total_used = 0;

	seq_printf(m, "%s            %s, %s\n",
		   "count(KB)", "kaddr", "function");
	seq_puts(m, "------------------------------\n");
	for (j = 0; j < MIGRATE_TYPES; j++) {
		if (!mt_cnt[j])	/* this migrate type is empty */
			continue;

		p = sum + mt_offset[j];
		sort(p, mt_cnt[j], sizeof(*p), trace_cmp, NULL);

		total_mt = 0;
		for (i = 0; i < mt_cnt[j]; i++) {
			if (!p[i].cnt)	/* may be empty after merge */
				continue;

			if (K(p[i].cnt) >= page_trace_filter) {
				seq_printf(m, "%8ld, %16lx, %ps\n",
					   K(p[i].cnt), p[i].ip,
					   (void *)p[i].ip);
			}
			total_mt += p[i].cnt;
		}
		if (!total_mt)
			continue;
		seq_puts(m, "------------------------------\n");
		seq_printf(m, "total pages:%6ld, %9ld kB, type:%s\n",
			   total_mt, K(total_mt), migratetype_names[j]);
		seq_puts(m, "------------------------------\n");
		total_used += total_mt;
	}
	seq_printf(m, "Zone %8s, managed:%6ld KB, free:%6ld kB, used:%6ld KB\n",
		   zone->name, K(atomic_long_read(&zone->managed_pages)),
		   K(zone_page_state(zone, NR_FREE_PAGES)), K(total_used));
	seq_puts(m, "------------------------------\n");
}

static int _merge_same_function(struct page_summary *p, int range)
{
	int j, k, real_used;

	/* first, replace all ip to entry of each function */
	for (j = 0; j < range; j++) {
		if (!p[j].ip)	/* Not used */
			break;
		p[j].ip = find_ip_base(p[j].ip);
	}

	real_used = j;
	/* second, loop and merge same ip */
	for (j = 0; j < real_used; j++) {
		for (k = j + 1; k < real_used; k++) {
			if (p[k].ip != (-1ul) &&
			    p[k].ip == p[j].ip) {
				p[j].cnt += p[k].cnt;
				p[k].ip  = (-1ul);
				p[k].cnt = 0;
			}
		}
	}
	return real_used;
}

static void merge_same_function(struct page_summary *sum, int *mt_cnt)
{
	int i, range;
	struct page_summary *p;

	for (i = 0; i < MIGRATE_TYPES; i++) {
		range = mt_cnt[i];
		p = sum + mt_offset[i];
		_merge_same_function(p, range);
	}
}

static int update_page_trace(struct seq_file *m, struct zone *zone,
			     struct page_summary *sum, int *mt_cnt)
{
	unsigned long pfn;
	unsigned long start_pfn = zone->zone_start_pfn;
	unsigned long end_pfn = zone_end_pfn(zone);
	int    ret = 0, mt;
	struct page_trace *trace;
	struct page_summary *p;
	struct rb_root root[MIGRATE_TYPES];

	memset(root, 0, sizeof(root));
	/* loop whole zone */
	for (pfn = start_pfn; pfn < end_pfn; pfn++) {
		struct page *page;

#ifdef CONFIG_ARM64
		if (!pfn_is_map_memory(pfn))
#else
		if (!pfn_valid(pfn))
#endif
			continue;

		page = pfn_to_page(pfn);

		trace = find_page_base(page);
	#if DEBUG_PAGE_TRACE
		check_trace_valid(trace);
	#endif
		if (page_trace_invalid(trace)) /* free pages */
			continue;

		if (!(*(unsigned int *)trace)) /* empty */
			continue;

		mt = trace->migrate_type;
		p  = sum + mt_offset[mt];
		ret = find_page_ip(trace, p, mt_offset[mt + 1] - mt_offset[mt],
				   mt_cnt, &root[mt]);
		if (ret) {
			pr_err("mt type:%d, out of range:%d\n",
			       mt, mt_offset[mt + 1] - mt_offset[mt]);
			break;
		}
	}
	if (merge_function)
		merge_same_function(sum, mt_cnt);
	return ret;
}

static int pagetrace_show(struct seq_file *m, void *arg)
{
	struct zone *zone;
	int ret, size = sizeof(struct page_summary) * SHOW_CNT;
	struct pagetrace_summary *sum;

#ifndef CONFIG_AMLOGIC_PAGE_TRACE_INLINE
	if (!trace_buffer) {
		seq_puts(m, "page trace not enabled\n");
		return 0;
	}
#endif
	sum = kzalloc(sizeof(*sum), GFP_KERNEL);
	if (!sum)
		return -ENOMEM;

	m->private = sum;
	sum->sum = vzalloc(size);
	if (!sum->sum) {
		kfree(sum);
		m->private = NULL;
		return -ENOMEM;
	}

	/* update only once */
	seq_puts(m, "==============================\n");
	sum->ticks = sched_clock();
	for_each_populated_zone(zone) {
		memset(sum->sum, 0, size);
		memset(sum->mt_cnt, 0, sizeof(int) * MIGRATE_TYPES);
		ret = update_page_trace(m, zone, sum->sum, sum->mt_cnt);
		if (ret) {
			seq_printf(m, "Error %d in zone %8s\n",
				   ret, zone->name);
			continue;
		}
		show_page_trace(m, zone, sum->sum, sum->mt_cnt);
	}
	sum->ticks = sched_clock() - sum->ticks;

	seq_printf(m, "SHOW_CNT:%d, buffer size:%d, tick:%ld ns\n",
		   SHOW_CNT, size, sum->ticks);
	seq_puts(m, "==============================\n");

	vfree(sum->sum);
	kfree(sum);

	return 0;
}

static int pagetrace_open(struct inode *inode, struct file *file)
{
	return single_open(file, pagetrace_show, NULL);
}

static ssize_t pagetrace_write(struct file *file, const char __user *buffer,
			       size_t count, loff_t *ppos)
{
	char *buf;
	unsigned long arg = 0;

	buf = kmalloc(count, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	if (copy_from_user(buf, buffer, count)) {
		kfree(buf);
		return -EINVAL;
	}

	if (!strncmp(buf, "merge=", 6)) {	/* option for 'merge=' */
		if (sscanf(buf, "merge=%ld", &arg) < 0) {
			kfree(buf);
			return -EINVAL;
		}
		merge_function = arg ? 1 : 0;
		pr_info("set merge_function to %d\n", merge_function);
	}

	if (!strncmp(buf, "filter=", 7)) {	/* option for 'filter=' */
		if (sscanf(buf, "filter=%ld", &arg) < 0) {
			kfree(buf);
			return -EINVAL;
		}
		page_trace_filter = arg;
		pr_info("set filter to %d KB\n", page_trace_filter);
	}

	kfree(buf);

	return count;
}

static const struct proc_ops pagetrace_proc_ops = {
	.proc_open	= pagetrace_open,
	.proc_read	= seq_read,
	.proc_lseek	= seq_lseek,
	.proc_write	= pagetrace_write,
	.proc_release	= single_release,
};

static int __init page_trace_module_init(void)
{
#ifdef CONFIG_AMLOGIC_PAGE_TRACE_INLINE
	d_pagetrace = proc_create("pagetrace", 0444,
				  NULL, &pagetrace_proc_ops);
#else
	if (!page_trace_disable)
		d_pagetrace = proc_create("pagetrace", 0444,
					  NULL, &pagetrace_proc_ops);
#endif
	if (IS_ERR_OR_NULL(d_pagetrace)) {
		pr_err("%s, create pagetrace failed\n", __func__);
		return -1;
	}

#ifndef CONFIG_AMLOGIC_PAGE_TRACE_INLINE
	if (!trace_buffer)
		return -ENOMEM;
#endif

	return 0;
}

static void __exit page_trace_module_exit(void)
{
	if (d_pagetrace)
		proc_remove(d_pagetrace);
}
module_init(page_trace_module_init);
module_exit(page_trace_module_exit);

void __init page_trace_mem_init(void)
{
#ifndef CONFIG_AMLOGIC_PAGE_TRACE_INLINE
	struct zone *zone;
	unsigned long total_page = 0;
#endif

	find_static_common_symbol();
#ifdef CONFIG_KASAN	/* open multi_shot for kasan */
	kasan_save_enable_multi_shot();
#endif
#ifdef CONFIG_AMLOGIC_PAGE_TRACE_INLINE
	/*
	 * if this compiler error occurs, that means there are over 32 page
	 * flags, you should disable AMLOGIC_PAGE_TRACE or reduce some page
	 * flags.
	 */
	BUILD_BUG_ON((__NR_PAGEFLAGS + ZONES_WIDTH    +
		      NODES_WIDTH    + SECTIONS_WIDTH +
		      LAST_CPUPID_SHIFT) > 32);
#else
	if (page_trace_disable)
		return;

	for_each_populated_zone(zone) {
		total_page += zone->spanned_pages;
		pr_info("zone:%s, spaned pages:%ld, total:%ld\n",
			zone->name, zone->spanned_pages, total_page);
	}
	ptrace_size = total_page * sizeof(struct page_trace) * trace_step;
	ptrace_size = PAGE_ALIGN(ptrace_size);
	if (page_trace_pre_work(ptrace_size)) {
		trace_buffer = NULL;
		ptrace_size = 0;
		pr_err("%s reserve memory failed\n", __func__);
		return;
	}
#endif
}

MODULE_AUTHOR("Amlogic pagetrace owner");
MODULE_DESCRIPTION("Amlogic page trace driver for memory debug");
MODULE_LICENSE("GPL v2");
