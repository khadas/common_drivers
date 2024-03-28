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
#include <trace/hooks/mm.h>
#ifdef CONFIG_RANDOMIZE_BASE
#include <asm/module.h>
#endif
#if CONFIG_AMLOGIC_KERNEL_VERSION < 14515
#include <linux/kasan.h>
#endif

#ifndef CONFIG_AMLOGIC_PAGE_TRACE_INLINE
#define DEBUG_PAGE_TRACE	0
#else
#define DEBUG_PAGE_TRACE	0
#endif

#if IS_MODULE(CONFIG_AMLOGIC_PAGE_TRACE)
#include <linux/nodemask.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#endif

#ifdef CONFIG_NUMA
#define COMMON_CALLER_SIZE	64	/* more function names if NUMA */
#else
#define COMMON_CALLER_SIZE	64
#endif

static const char * const aml_migratetype_names[MIGRATE_TYPES] = {
	"Unmovable",
	"Movable",
	"Reclaimable",
#ifdef CONFIG_CMA
	"CMA",
#endif
	"HighAtomic",
#ifdef CONFIG_MEMORY_ISOLATION
	"Isolate",
#endif
};

#if IS_MODULE(CONFIG_AMLOGIC_PAGE_TRACE)
unsigned long *free_pages_bitmap;
unsigned long aml_text;
#else
unsigned long aml_text = (unsigned long)_text;
#endif

#ifdef CONFIG_RANDOMIZE_BASE
unsigned long aml_module_alloc_base;
#endif

/*
 * this is a driver which will hook during page alloc/free and
 * record caller of each page to a buffer. Detailed information
 * of page allocate statistics can be find in /proc/pagetrace
 *
 */
static bool merge_function = 1;
static int page_trace_filter = 64; /* not print size < page_trace_filter */
static int page_trace_filter_slab;
static struct proc_dir_entry *d_pagetrace;
#ifndef CONFIG_AMLOGIC_PAGE_TRACE_INLINE
struct page_trace *trace_buffer;
static unsigned long ptrace_size;
static unsigned int trace_step = 1;
static bool page_trace_disable __initdata;
#endif
#if defined(CONFIG_TRACEPOINTS) && \
	defined(CONFIG_ANDROID_VENDOR_HOOKS) && \
	IS_BUILTIN(CONFIG_AMLOGIC_PAGE_TRACE)
struct pagetrace_vendor_param trace_param;
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
static struct fun_symbol common_func[] = {
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
#if IS_MODULE(CONFIG_AMLOGIC_PAGE_TRACE)
	{"alloc_pages_ret_handler",	1, 0},
	{"comp_alloc_ret_handler",	1, 0},
	{"__kretprobe_trampoline_handler", 1, 0},
	{"trampoline_probe_handler",	1, 0},
	{"kretprobe_trampoline",	1, 0},
#endif
	{"module_alloc",		1, 0},
	{"load_module",			1, 0},
#ifdef CONFIG_NUMA
	{"alloc_pages_current",		1, 0},
	{"alloc_page_interleave",	1, 0},
	{"kmalloc_large_node",		1, 0},
	{"__kmalloc_node",		1, 0},
	{"alloc_pages_vma",		1, 0},
#endif
#ifdef CONFIG_SLUB	/* for some static symbols not exported in headfile */
	{"new_slab",			0, 0},
#if IS_MODULE(CONFIG_AMLOGIC_PAGE_TRACE)
	{"slab_alloc",			0, 0},
	{"___slab_alloc",		0, 0},
#endif
	{"__slab_alloc",		0, 0},
	{"allocate_slab",		0, 0},
#endif
	{}		/* tail */
};

/* ----------------------------------- */

#if IS_MODULE(CONFIG_AMLOGIC_PAGE_TRACE)
#define MAX_SYMBOL_LEN	64
static char sym_lookup_off[MAX_SYMBOL_LEN] = "kallsyms_lookup_size_offset";
int (*aml_kallsyms_lookup_size_offset)(unsigned long addr,
					unsigned long *symbolsize,
					unsigned long *offset);

/* For each probe you need to allocate a kprobe structure */
static struct kprobe kp_lookup_off = {
	.symbol_name	= sym_lookup_off,
};

static char sym_lookup_name[MAX_SYMBOL_LEN] = "kallsyms_lookup_name";
unsigned long (*aml_kallsyms_lookup_name)(const char *name);

/* For each probe you need to allocate a kprobe structure */
static struct kprobe kp_lookup_name = {
	.symbol_name	= sym_lookup_name,
};

/* ----------------------------------- */

static char func_comp_alloc[NAME_MAX] = "compaction_alloc";
static char func_alloc_pages[NAME_MAX] = "__alloc_pages";
#if CONFIG_AMLOGIC_KERNEL_VERSION >= 14515
static char func_free_prep[NAME_MAX] = "free_unref_page_prepare";
#else
static char func_free_prep[NAME_MAX] = "free_pcp_prepare";
#endif
static char func_free_ok[NAME_MAX] = "__free_pages_ok";

/* per-instance private data */
struct kretp_data {
	struct page *page;
	gfp_t gfp;
	unsigned int order;
	unsigned long count;
};

/* Here we use the entry_handler to timestamp function entry */
static int comp_alloc_entry_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	struct kretp_data *data;

	data = (struct kretp_data *)ri->data;
	data->page = (struct page *)regs->regs[0];
	return 0;
}

NOKPROBE_SYMBOL(comp_alloc_entry_handler);

static int alloc_pages_entry_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	struct kretp_data *data;

	data = (struct kretp_data *)ri->data;
	data->gfp = (gfp_t)regs->regs[0];
	data->order = (unsigned int)regs->regs[1];
	return 0;
}

NOKPROBE_SYMBOL(alloc_pages_entry_handler);

static int free_prep_entry_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	struct kretp_data *data;

	data = (struct kretp_data *)ri->data;
	data->page = (struct page *)regs->regs[0];
#if CONFIG_AMLOGIC_KERNEL_VERSION >= 14515
	data->order = (unsigned int)regs->regs[2];
#else
	data->order = (unsigned int)regs->regs[1];
#endif
	return 0;
}

NOKPROBE_SYMBOL(free_prep_entry_handler);

static int free_ok_entry_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	struct kretp_data *data;

	data = (struct kretp_data *)ri->data;
	data->page = (struct page *)regs->regs[0];
	data->order = (unsigned int)regs->regs[1];
	return 0;
}

NOKPROBE_SYMBOL(free_ok_entry_handler);

/*
 * Return-probe handler: Log the return value and duration. Duration may turn
 * out to be zero consistently, depending upon the granularity of time
 * accounting on the platform.
 */
static int comp_alloc_ret_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
//	unsigned long retval = regs_return_value(regs);
	struct page *ret_page = (struct page *)regs_return_value(regs);
	struct kretp_data *data = (struct kretp_data *)ri->data;

	replace_page_trace(ret_page, data->page);

	return 0;
}

NOKPROBE_SYMBOL(comp_alloc_ret_handler);

static int alloc_pages_ret_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
//	unsigned long retval = regs_return_value(regs);
	struct page *ret_page = (struct page *)regs_return_value(regs);
	struct kretp_data *data = (struct kretp_data *)ri->data;

	set_page_trace(ret_page, data->order, data->gfp, NULL);

	return 0;
}

NOKPROBE_SYMBOL(alloc_pages_ret_handler);

static int free_prep_ret_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	bool retval = (bool)regs_return_value(regs);
	struct kretp_data *data = (struct kretp_data *)ri->data;

	if (retval)
		reset_page_trace(data->page, data->order);

	return 0;
}

NOKPROBE_SYMBOL(free_prep_ret_handler);

static int free_ok_ret_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	struct kretp_data *data = (struct kretp_data *)ri->data;

	if (PageBuddy(data->page))
		reset_page_trace(data->page, data->order);

	return 0;
}

NOKPROBE_SYMBOL(free_ok_ret_handler);

static struct kretprobe comp_alloc_kretprobe = {
	.handler		= comp_alloc_ret_handler,
	.entry_handler		= comp_alloc_entry_handler,
	.data_size		= sizeof(struct kretp_data),
	/* Probe up to 20 instances concurrently. */
	.maxactive		= 20,
};

static struct kretprobe alloc_pages_kretprobe = {
	.handler		= alloc_pages_ret_handler,
	.entry_handler		= alloc_pages_entry_handler,
	.data_size		= sizeof(struct kretp_data),
	/* Probe up to 20 instances concurrently. */
	.maxactive		= 20,
};

static struct kretprobe free_prep_kretprobe = {
	.handler		= free_prep_ret_handler,
	.entry_handler		= free_prep_entry_handler,
	.data_size		= sizeof(struct kretp_data),
	/* Probe up to 20 instances concurrently. */
	.maxactive		= 20,
};

static struct kretprobe free_ok_kretprobe = {
	.handler		= free_ok_ret_handler,
	.entry_handler		= free_ok_entry_handler,
	.data_size		= sizeof(struct kretp_data),
	/* Probe up to 20 instances concurrently. */
	.maxactive		= 20,
};
#endif

/* ----------------------------------- */

static inline bool page_trace_invalid(struct page_trace *trace)
{
	return trace->order == IP_INVALID;
}

#if !defined(CONFIG_AMLOGIC_PAGE_TRACE_INLINE) && IS_BUILTIN(CONFIG_AMLOGIC_PAGE_TRACE)
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
		if (offset >= ((unsigned long)_end - aml_text))
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
#ifdef CONFIG_RANDOMIZE_BASE
	u64 module_alloc_end = aml_module_alloc_base + MODULES_VSIZE;

	if (ip >= aml_module_alloc_base && ip < module_alloc_end)
#else
	if (ip >= MODULES_VADDR && ip < MODULES_END)
#endif
		return 1;
	return 0;
}

/*
 * set up information for common caller in memory allocate API
 */
static int __nocfi setup_common_caller(unsigned long kaddr)
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

#if IS_MODULE(CONFIG_AMLOGIC_PAGE_TRACE)
	ret = aml_kallsyms_lookup_size_offset(kaddr, &size, &offset);
#else
	ret = kallsyms_lookup_size_offset(kaddr, &size, &offset);
#endif
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

static void dump_common_caller(void)
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

static int  sym_cmp(const void *x1, const void *x2)
{
	struct alloc_caller *p1, *p2;

	p1 = (struct alloc_caller *)x1;
	p2 = (struct alloc_caller *)x2;

	/* descending order */
	return p1->func_start_addr < p2->func_start_addr ? 1 : -1;
}

#if IS_BUILTIN(CONFIG_AMLOGIC_PAGE_TRACE)
static int match_common_caller(void *data, const char *name,
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
			if (strstr(name, s->name)) {	/* contains */
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
static unsigned long kallsyms_sym_addr(int idx)
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

static unsigned int kallsyms_expand_sym(unsigned int off,
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

static int kallsyms_for_each_symbol(int (*fn)(void *, const char *,
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
#else
static int __nocfi match_common_caller(void)
{
	int i;
	struct fun_symbol *s;
	unsigned long addr;

	for (i = 0; i < ARRAY_SIZE(common_func); i++) {
		s = &common_func[i];
		if (!s->name)
			break;		/* end */
		addr = aml_kallsyms_lookup_name(s->name);
		if (addr)
			setup_common_caller(addr);
	}
	return 0;
}
#endif

static int find_static_common_symbol(void *data)
{
	memset(common_caller, 0, sizeof(common_caller));
#if IS_BUILTIN(CONFIG_AMLOGIC_PAGE_TRACE)
	kallsyms_for_each_symbol(match_common_caller, NULL);
#else
	match_common_caller();
#endif
	sort(common_caller, COMMON_CALLER_SIZE, sizeof(struct alloc_caller),
		sym_cmp, NULL);
	dump_common_caller();

	return 0;
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

		if (pc < add_l)		/* caller is descending order */
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
#ifdef CONFIG_RANDOMIZE_BASE
		text = aml_module_alloc_base;
#else
		text = MODULES_VADDR;
#endif
	else
		text = aml_text;
	return text + ((trace->ret_ip) << 2);
}

#if IS_MODULE(CONFIG_AMLOGIC_PAGE_TRACE)
EXPORT_SYMBOL(unpack_ip);
#endif

#ifdef CONFIG_ARM64
/*
 * We can only safely access per-cpu stacks from current in a non-preemptible
 * context.
 */
static bool aml_on_accessible_stack(const struct task_struct *tsk,
				unsigned long sp, unsigned long size,
				struct stack_info *info)
{
	if (info)
		info->type = STACK_TYPE_UNKNOWN;

	if (on_task_stack(tsk, sp, size, info))
		return true;
	if (tsk != current || preemptible())
		return false;
#if !IS_MODULE(CONFIG_AMLOGIC_PAGE_TRACE)
	if (on_irq_stack(sp, size, info))
		return true;
	if (on_overflow_stack(sp, size, info))
		return true;
#endif
	if (on_sdei_stack(sp, size, info))
		return true;

	return false;
}

#if CONFIG_AMLOGIC_KERNEL_VERSION >= 14515
/*
 * Unwind from one frame record (A) to the next frame record (B).
 *
 * We terminate early if the location of B indicates a malformed chain of frame
 * records (e.g. a cycle), determined based on the location and fp value of A
 * and the location (but not the fp value) of B.
 */
static int notrace unwind_next(struct unwind_state *state)
{
	struct task_struct *tsk = state->task;
	unsigned long fp = state->fp;
	struct stack_info info;
	int err;

	/* Final frame; nothing to unwind */
	if (fp == (unsigned long)task_pt_regs(tsk)->stackframe)
		return -ENOENT;

	err = unwind_next_common(state, &info, aml_on_accessible_stack, NULL);
	if (err)
		return err;

	state->pc = ptrauth_strip_insn_pac(state->pc);

#if defined(CONFIG_FUNCTION_GRAPH_TRACER) && IS_BUILTIN(CONFIG_AMLOGIC_PAGE_TRACE)
	if (tsk->ret_stack &&
		(state->pc == (unsigned long)return_to_handler)) {
		unsigned long orig_pc;
		/*
		 * This is a case where function graph tracer has
		 * modified a return address (LR) in a stack frame
		 * to hook a function return.
		 * So replace it to an original value.
		 */
		orig_pc = ftrace_graph_ret_addr(tsk, NULL, state->pc,
						(void *)state->fp);
		if (WARN_ON_ONCE(state->pc == orig_pc))
			return -EINVAL;
		state->pc = orig_pc;
	}
#endif /* CONFIG_FUNCTION_GRAPH_TRACER */

	return 0;
}
#else
/*
 * Unwind from one frame record (A) to the next frame record (B).
 *
 * We terminate early if the location of B indicates a malformed chain of frame
 * records (e.g. a cycle), determined based on the location and fp value of A
 * and the location (but not the fp value) of B.
 */
static int notrace aml_unwind_frame(struct task_struct *tsk, struct stackframe *frame)
{
	unsigned long fp = frame->fp;
	struct stack_info info;

	if (!tsk)
		tsk = current;

	/* Final frame; nothing to unwind */
	if (fp == (unsigned long)task_pt_regs(tsk)->stackframe)
		return -ENOENT;

	if (fp & 0x7)
		return -EINVAL;

	if (!aml_on_accessible_stack(tsk, fp, 16, &info))
		return -EINVAL;

	if (test_bit(info.type, frame->stacks_done))
		return -EINVAL;

	/*
	 * As stacks grow downward, any valid record on the same stack must be
	 * at a strictly higher address than the prior record.
	 *
	 * Stacks can nest in several valid orders, e.g.
	 *
	 * TASK -> IRQ -> OVERFLOW -> SDEI_NORMAL
	 * TASK -> SDEI_NORMAL -> SDEI_CRITICAL -> OVERFLOW
	 *
	 * ... but the nesting itself is strict. Once we transition from one
	 * stack to another, it's never valid to unwind back to that first
	 * stack.
	 */
	if (info.type == frame->prev_type) {
		if (fp <= frame->prev_fp)
			return -EINVAL;
	} else {
		set_bit(frame->prev_type, frame->stacks_done);
	}

	/*
	 * Record this frame record's values and location. The prev_fp and
	 * prev_type are only meaningful to the next unwind_frame() invocation.
	 */
	frame->fp = READ_ONCE_NOCHECK(*(unsigned long *)(fp));
	frame->pc = READ_ONCE_NOCHECK(*(unsigned long *)(fp + 8));
	frame->prev_fp = fp;
	frame->prev_type = info.type;

#if defined(CONFIG_FUNCTION_GRAPH_TRACER) && IS_BUILTIN(CONFIG_AMLOGIC_PAGE_TRACE)
	if (tsk->ret_stack &&
		(ptrauth_strip_insn_pac(frame->pc) == (unsigned long)return_to_handler)) {
		struct ftrace_ret_stack *ret_stack;
		/*
		 * This is a case where function graph tracer has
		 * modified a return address (LR) in a stack frame
		 * to hook a function return.
		 * So replace it to an original value.
		 */
		ret_stack = ftrace_graph_get_ret_stack(tsk, frame->graph++);
		if (WARN_ON_ONCE(!ret_stack))
			return -EINVAL;
		frame->pc = ret_stack->ret;
	}
#endif /* CONFIG_FUNCTION_GRAPH_TRACER */

	frame->pc = ptrauth_strip_insn_pac(frame->pc);

	return 0;
}
#endif
#endif

unsigned long find_back_trace(void)
{
#if (CONFIG_AMLOGIC_KERNEL_VERSION >= 14515) && defined(CONFIG_ARM64)
	struct unwind_state frame;
#else
	struct stackframe frame;
#endif
	int ret, step = 0;

#ifdef CONFIG_ARM64
	frame.fp = (unsigned long)__builtin_frame_address(0);
	frame.pc = _RET_IP_;
	bitmap_zero(frame.stacks_done, __NR_STACK_TYPES);
	frame.prev_fp = 0;
	frame.prev_type = STACK_TYPE_UNKNOWN;
#if CONFIG_AMLOGIC_KERNEL_VERSION >= 14515
	frame.task = current;
#else
#if defined(CONFIG_FUNCTION_GRAPH_TRACER) && IS_BUILTIN(CONFIG_AMLOGIC_PAGE_TRACE)
	frame.graph = 0;
#endif
#endif
#else
	frame.fp = (unsigned long)__builtin_frame_address(0);
	frame.sp = current_stack_pointer;
	frame.lr = (unsigned long)__builtin_return_address(0);
	frame.pc = (unsigned long)find_back_trace;
#endif
	while (1) {
	#ifdef CONFIG_ARM64
	#if CONFIG_AMLOGIC_KERNEL_VERSION >= 14515
		ret = unwind_next(&frame);
	#else
		ret = aml_unwind_frame(current, &frame);
	#endif
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

#if IS_MODULE(CONFIG_AMLOGIC_PAGE_TRACE)
EXPORT_SYMBOL(find_back_trace);
#endif

static struct pglist_data *aml_first_online_pgdat(void)
{
	return NODE_DATA(first_online_node);
}

static struct pglist_data *aml_next_online_pgdat(struct pglist_data *pgdat)
{
	int nid = next_online_node(pgdat->node_id);

	if (nid == MAX_NUMNODES)
		return NULL;
	/*
	 * This code is copy from upstream, please ignore the problem.
	 */
	/* coverity[dead_error_line:SUPPRESS] */
	return NODE_DATA(nid);
}

/*
 * next_zone - helper magic for for_each_zone()
 */
static struct zone *aml_next_zone(struct zone *zone)
{
	pg_data_t *pgdat = zone->zone_pgdat;

	if (zone < pgdat->node_zones + MAX_NR_ZONES - 1) {
		zone++;
	} else {
		pgdat = aml_next_online_pgdat(pgdat);
		if (pgdat)
			zone = pgdat->node_zones;
		else
			zone = NULL;
	}
	return zone;
}

#define aml_for_each_populated_zone(zone)			\
	for (zone = (aml_first_online_pgdat())->node_zones;	\
	     zone;						\
	     zone = aml_next_zone(zone))			\
		if (!populated_zone(zone))			\
			; /* do nothing */			\
		else

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
	aml_for_each_populated_zone(zone) {
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

#if IS_MODULE(CONFIG_AMLOGIC_PAGE_TRACE)
EXPORT_SYMBOL(find_page_base);
#endif
#endif

unsigned long get_page_trace(struct page *page)
{
	struct page_trace *trace;

	trace = find_page_base(page);
	if (trace)
		return unpack_ip(trace);

	return 0;
}

#if IS_MODULE(CONFIG_AMLOGIC_PAGE_TRACE)
EXPORT_SYMBOL(get_page_trace);
#endif

/* Convert GFP flags to their corresponding migrate type */
#define AML_GFP_MOVABLE_MASK (__GFP_RECLAIMABLE | __GFP_MOVABLE)
#define AML_GFP_MOVABLE_SHIFT 3

static int aml_gfp_migratetype(const gfp_t gfp_flags)
{
	VM_WARN_ON((gfp_flags & AML_GFP_MOVABLE_MASK) == AML_GFP_MOVABLE_MASK);
	BUILD_BUG_ON((1UL << AML_GFP_MOVABLE_SHIFT) != ___GFP_MOVABLE);
	BUILD_BUG_ON((___GFP_MOVABLE >> AML_GFP_MOVABLE_SHIFT) != MIGRATE_MOVABLE);

	/* Group based on mobility */
	return (gfp_flags & AML_GFP_MOVABLE_MASK) >> AML_GFP_MOVABLE_SHIFT;
}

#ifndef CONFIG_AMLOGIC_PAGE_TRACE_INLINE
static void __init set_init_page_trace(struct page *page, unsigned int order, gfp_t flag)
{
	unsigned long text;
	unsigned long ip;
	struct page_trace trace = {}, *base;

	if (page && trace_buffer) {
		ip = (unsigned long)set_page_trace;
		text = aml_text;

		trace.ret_ip = (ip - text) >> 2;
		trace.migrate_type = aml_gfp_migratetype(flag);
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

	text = aml_text;
	if (ip >= aml_text) {
		text = aml_text;
	} else if (is_module_addr(ip)) {
#ifdef CONFIG_RANDOMIZE_BASE
		text = aml_module_alloc_base;
#else
		text = MODULES_VADDR;
#endif
		trace.module_flag = 1;
	}

	trace.ret_ip = (ip - text) >> 2;
#ifdef CONFIG_AMLOGIC_CMA
	if (flag == __GFP_NO_CMA)
		trace.migrate_type = MIGRATE_CMA;
	else
		trace.migrate_type = aml_gfp_migratetype(flag);
#else
	trace.migrate_type = aml_gfp_migratetype(flag);
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

void set_page_trace(struct page *page, unsigned int order, gfp_t flag, void *func)
{
	unsigned long ip;
	struct page_trace *base;
	unsigned int val = 0, i;

	if (!page)
		return;
#ifndef CONFIG_AMLOGIC_PAGE_TRACE_INLINE
	if (!trace_buffer)
		return;
#ifdef CONFIG_RANDOMIZE_BASE
		ip = aml_module_alloc_base;
#else
		ip = MODULES_VADDR;
#endif
#if defined(CONFIG_TRACEPOINTS) && \
	defined(CONFIG_ANDROID_VENDOR_HOOKS) && \
	IS_BUILTIN(CONFIG_AMLOGIC_PAGE_TRACE)
	trace_param.trace_buf = trace_buffer;
	trace_param.text = (unsigned long)_text;
	trace_param.trace_step = trace_step;
	trace_param.ip = ip;
	trace_android_vh_cma_drain_all_pages_bypass(1024, (bool *)&trace_param);
#endif
#endif
#if defined(CONFIG_MEMCG) && IS_BUILTIN(CONFIG_AMLOGIC_PAGE_TRACE)
	trace_android_vh_mem_cgroup_alloc(root_mem_cgroup);
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

#if IS_MODULE(CONFIG_AMLOGIC_PAGE_TRACE)
EXPORT_SYMBOL(set_page_trace);
#endif

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

void replace_page_trace(struct page *new, struct page *old)
{
	struct page_trace *old_trace, *new_trace;

	if (!new || !old)
		return;

	old_trace  = find_page_base(old);
	new_trace  = find_page_base(new);
	*new_trace = *old_trace;
}

#if IS_MODULE(CONFIG_AMLOGIC_PAGE_TRACE)
EXPORT_SYMBOL(replace_page_trace);
#endif

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
	struct page *page;
	void *paddr;
	void *pend;
	unsigned long maddr;

#ifdef CONFIG_RANDOMIZE_BASE
	maddr = aml_module_alloc_base;
#else
	maddr = MODULES_VADDR;
#endif

#if IS_BUILTIN(CONFIG_AMLOGIC_PAGE_TRACE)
	paddr = memblock_alloc_low(size, PAGE_SIZE);
	if (!paddr) {
		panic("%s: Failed to allocate %lu bytes.\n", __func__, size);
		return -ENOMEM;
	}
#else
	paddr = vzalloc(size);
	if (!paddr)
		return -ENOMEM;
#endif

	trace_buffer = (struct page_trace *)paddr;
	pend = paddr + size;
	pr_info("%s, trace buffer:%px, size:%lx, end:%px, module: %lx\n",
			__func__, trace_buffer, size, pend, maddr);
	memset(paddr, 0, size);

	for (; paddr < pend; paddr += PAGE_SIZE) {
#if IS_BUILTIN(CONFIG_AMLOGIC_PAGE_TRACE)
		page = virt_to_page(paddr);
#else
		page = vmalloc_to_page(paddr);
#endif
		set_init_page_trace(page, 0, GFP_KERNEL);
	#if DEBUG_PAGE_TRACE
		pr_info("%s, trace page:%p, %lx\n",
				__func__, page, page_to_pfn(page));
	#endif
	}
	return 0;
}
#endif

/*--------------------------sysfs node -------------------------------*/
#define LARGE	1024
#define SMALL	256

/* caller for unmovable are max */
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
	int filter_slab[MIGRATE_TYPES];
};

static unsigned long __nocfi find_ip_base(unsigned long ip)
{
	unsigned long size, offset;
	int ret;

#if IS_MODULE(CONFIG_AMLOGIC_PAGE_TRACE)
	ret = aml_kallsyms_lookup_size_offset(ip, &size, &offset);
#else
	ret = kallsyms_lookup_size_offset(ip, &size, &offset);
#endif
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
			    struct pagetrace_summary *pt_sum)
{
	int i, j;
	struct page_summary *p;
	unsigned long total_mt, total_used = 0;
	struct page_summary *sum = pt_sum->sum;
	int *mt_cnt = pt_sum->mt_cnt;

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
			   total_mt, K(total_mt), aml_migratetype_names[j]);
		if (page_trace_filter_slab)
			seq_printf(m, "filter_slab pages:%6d, %9ld kB\n",
				pt_sum->filter_slab[j], K(pt_sum->filter_slab[j]));
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
			     struct pagetrace_summary *pt_sum)
{
	unsigned long pfn;
	unsigned long start_pfn = zone->zone_start_pfn;
	unsigned long end_pfn = start_pfn + zone->present_pages;
	int    ret = 0, mt = 0;
	struct page_trace *trace;
	struct page_summary *p;
	struct rb_root root[MIGRATE_TYPES];
	struct page_summary *sum = pt_sum->sum;
	int *mt_cnt = pt_sum->mt_cnt;

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

		if (page_trace_filter_slab && PageSlab(page)) {
			pt_sum->filter_slab[mt]++;
			continue;
		}

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
	aml_for_each_populated_zone(zone) {
		memset(sum->sum, 0, size);
		memset(sum->mt_cnt, 0, sizeof(int) * MIGRATE_TYPES);
		ret = update_page_trace(m, zone, sum);
		if (ret) {
			seq_printf(m, "Error %d in zone %8s\n",
				   ret, zone->name);
			continue;
		}
		show_page_trace(m, zone, sum);
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

	if (!strncmp(buf, "filter_slab=", 12)) {	/* option for 'filter_slab=' */
		if (sscanf(buf, "filter_slab=%ld", &arg) < 0) {
			kfree(buf);
			return -EINVAL;
		}
		page_trace_filter_slab = arg;
		pr_info("set filter_slab to %d\n", page_trace_filter_slab);
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

#if IS_MODULE(CONFIG_AMLOGIC_PAGE_TRACE)
struct page_summary *dump_sum;

static char sym_dump_header[32] = "dump_header";

static struct kprobe kp_dump_header = {
	.symbol_name	= sym_dump_header,
};

static void show_page_trace2(struct zone *zone,
		struct pagetrace_summary *pt_sum)
{
	int i, j;
	struct page_summary *p;
	unsigned long total_mt, total_used = 0;
	struct page_summary *sum = pt_sum->sum;
	int *mt_cnt = pt_sum->mt_cnt;

	pr_info("%s            %s, %s\n",
			"count(KB)", "kaddr", "function");
	pr_info("------------------------------\n");
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
				pr_info("%8ld, %16lx, %ps\n",
						K(p[i].cnt), p[i].ip,
						(void *)p[i].ip);
			}
			total_mt += p[i].cnt;
		}
		if (!total_mt)
			continue;
		pr_info("------------------------------\n");
		pr_info("total pages:%6ld, %9ld kB, type:%s\n",
				total_mt, K(total_mt), aml_migratetype_names[j]);
		if (page_trace_filter_slab)
			pr_info("filter_slab pages:%6d, %9ld kB\n",
					pt_sum->filter_slab[j], K(pt_sum->filter_slab[j]));
		pr_info("------------------------------\n");
		total_used += total_mt;
	}
	pr_info("Zone %8s, managed:%6ld KB, free:%6ld kB, used:%6ld KB\n",
			zone->name, K(atomic_long_read(&zone->managed_pages)),
			K(zone_page_state(zone, NR_FREE_PAGES)), K(total_used));
	pr_info("------------------------------\n");
}

static void __kprobes dump_header_handler_post(struct kprobe *p,
				struct pt_regs *regs, unsigned long flags)
{
	struct zone *zone;
	int ret, size = sizeof(struct page_summary) * SHOW_CNT;
	struct pagetrace_summary *sum;

#ifndef CONFIG_AMLOGIC_PAGE_TRACE_INLINE
	if (!trace_buffer) {
		pr_info("page trace not enabled\n");
		return;
	}
#endif

	sum = kzalloc(sizeof(*sum), GFP_KERNEL);
	if (!sum)
		return;

	sum->sum = dump_sum;
	if (!sum->sum) {
		kfree(sum);
		return;
	}

	/* update only once */
	pr_info("==============================\n");
	sum->ticks = sched_clock();
	aml_for_each_populated_zone(zone) {
		memset(sum->sum, 0, size);
		memset(sum->mt_cnt, 0, sizeof(int) * MIGRATE_TYPES);
		ret = update_page_trace(NULL, zone, sum);
		if (ret) {
			pr_info("Error %d in zone %8s\n",
					ret, zone->name);
			continue;
		}
		show_page_trace2(zone, sum);
	}
	sum->ticks = sched_clock() - sum->ticks;

	pr_info("SHOW_CNT:%d, buffer size:%d, tick:%ld ns\n",
			SHOW_CNT, size, sum->ticks);
	pr_info("==============================\n");

	kfree(sum);
}
#endif

static int __init page_trace_module_init(void)
{
#if IS_MODULE(CONFIG_AMLOGIC_PAGE_TRACE)
	int ret;
	int size = sizeof(struct page_summary) * SHOW_CNT;
#endif

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

#if IS_MODULE(CONFIG_AMLOGIC_PAGE_TRACE)
	ret = register_kprobe(&kp_lookup_off);
	if (ret < 0) {
		pr_err("register_kprobe failed, returned %d\n", ret);
		return ret;
	}
	pr_debug("kprobe lookup offset at %px\n", kp_lookup_off.addr);

	aml_kallsyms_lookup_size_offset = (int (*)(unsigned long addr,
				unsigned long *symbolsize,
				unsigned long *offset))kp_lookup_off.addr;

	ret = register_kprobe(&kp_lookup_name);
	if (ret < 0) {
		pr_err("register_kprobe failed, returned %d\n", ret);
		return ret;
	}
	pr_debug("kprobe lookup offset at %px\n", kp_lookup_name.addr);

	aml_kallsyms_lookup_name = (unsigned long (*)(const char *name))kp_lookup_name.addr;

#ifdef CONFIG_RANDOMIZE_BASE
	aml_module_alloc_base = *(unsigned long *)aml_kallsyms_lookup_name("module_alloc_base");
#endif
	aml_text = aml_kallsyms_lookup_name("_text");
	page_trace_mem_init();

	dump_sum = vzalloc(size);
	kp_dump_header.post_handler = dump_header_handler_post;
	ret = register_kprobe(&kp_dump_header);
	if (ret < 0) {
		pr_err("register_kprobe failed, returned %d\n", ret);
		return ret;
	}
#endif

#if IS_BUILTIN(CONFIG_AMLOGIC_PAGE_TRACE) && defined(CONFIG_RANDOMIZE_BASE)
	aml_module_alloc_base = module_alloc_base;
#endif

#ifndef CONFIG_AMLOGIC_PAGE_TRACE_INLINE
	if (!trace_buffer)
		return -ENOMEM;
#endif

#if IS_MODULE(CONFIG_AMLOGIC_PAGE_TRACE)
	comp_alloc_kretprobe.kp.symbol_name = func_comp_alloc;
	ret = register_kretprobe(&comp_alloc_kretprobe);
	if (ret < 0) {
		pr_err("register_kretprobe failed, returned %d\n", ret);
		return ret;
	}
	pr_debug("Planted return probe at %s: %px\n",
			comp_alloc_kretprobe.kp.symbol_name, comp_alloc_kretprobe.kp.addr);

	alloc_pages_kretprobe.kp.symbol_name = func_alloc_pages;
	ret = register_kretprobe(&alloc_pages_kretprobe);
	if (ret < 0) {
		pr_err("register_kretprobe failed, returned %d\n", ret);
		return ret;
	}
	pr_debug("Planted return probe at %s: %px\n",
			alloc_pages_kretprobe.kp.symbol_name, alloc_pages_kretprobe.kp.addr);

	free_prep_kretprobe.kp.symbol_name = func_free_prep;
	ret = register_kretprobe(&free_prep_kretprobe);
	if (ret < 0) {
		pr_err("register_kretprobe failed, returned %d\n", ret);
		return ret;
	}
	pr_debug("Planted return probe at %s: %px\n",
			free_prep_kretprobe.kp.symbol_name, free_prep_kretprobe.kp.addr);

	free_ok_kretprobe.kp.symbol_name = func_free_ok;
	ret = register_kretprobe(&free_ok_kretprobe);
	if (ret < 0) {
		pr_err("register_kretprobe failed, returned %d\n", ret);
		return ret;
	}
	pr_debug("Planted return probe at %s: %px\n",
			free_ok_kretprobe.kp.symbol_name, free_ok_kretprobe.kp.addr);
#endif

	return 0;
}

static void __exit page_trace_module_exit(void)
{
	if (d_pagetrace)
		proc_remove(d_pagetrace);
#if IS_MODULE(CONFIG_AMLOGIC_PAGE_TRACE)
	unregister_kprobe(&kp_lookup_off);
	pr_debug("kprobe at %p unregistered\n", kp_lookup_off.addr);

	unregister_kprobe(&kp_lookup_name);
	pr_debug("kprobe at %p unregistered\n", kp_lookup_name.addr);

	unregister_kprobe(&kp_dump_header);
	pr_debug("kprobe at %p unregistered\n", kp_dump_header.addr);

	unregister_kretprobe(&comp_alloc_kretprobe);
	pr_debug("kretprobe at %p unregistered\n", comp_alloc_kretprobe.kp.addr);

	unregister_kretprobe(&alloc_pages_kretprobe);
	pr_debug("kretprobe at %p unregistered\n", alloc_pages_kretprobe.kp.addr);

	unregister_kretprobe(&free_prep_kretprobe);
	pr_debug("kretprobe at %p unregistered\n", free_prep_kretprobe.kp.addr);

	unregister_kretprobe(&free_ok_kretprobe);
	pr_debug("kretprobe at %p unregistered\n", free_ok_kretprobe.kp.addr);

	if (!trace_buffer)
		vfree(trace_buffer);

	if (!dump_sum)
		vfree(dump_sum);
#endif
}
module_init(page_trace_module_init);
module_exit(page_trace_module_exit);

#if IS_BUILTIN(CONFIG_AMLOGIC_PAGE_TRACE)
void __init page_trace_mem_init(void)
{
#ifndef CONFIG_AMLOGIC_PAGE_TRACE_INLINE
	struct zone *zone;
	unsigned long total_page = 0;
#endif

	find_static_common_symbol(NULL);
#ifdef CONFIG_KASAN	/* open multi_shot for kasan */
#if CONFIG_AMLOGIC_KERNEL_VERSION < 14515
	kasan_save_enable_multi_shot();
#endif
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

	aml_for_each_populated_zone(zone) {
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
#else
static void mark_free_pages(struct zone *zone)
{
	unsigned long pfn;
	unsigned long flags;
	unsigned int order, t;
	struct page *page;
	unsigned int count = 0;

	if (zone_is_empty(zone))
		return;

	spin_lock_irqsave(&zone->lock, flags);
	for_each_migratetype_order(order, t) {
		list_for_each_entry(page,
				&zone->free_area[order].free_list[t], lru) {
			pfn = page_to_pfn(page);
			bitmap_set(free_pages_bitmap, pfn, (1UL << order));
			count += (1UL << order);
		}
	}
	spin_unlock_irqrestore(&zone->lock, flags);
	pr_debug("free pages: %dkb\n", count * 4);
}

static int statistic_before_insmod_mem(void)
{
	struct zone *zone;
	struct page *page;
	unsigned long start_pfn, end_pfn, pfn;
	unsigned int count = 0;

	aml_for_each_populated_zone(zone) {
		mark_free_pages(zone);

		start_pfn = zone->zone_start_pfn;
		end_pfn = start_pfn + zone->present_pages;
		for (pfn = start_pfn; pfn < end_pfn; pfn++) {
#ifdef CONFIG_ARM64
			if (!pfn_is_map_memory(pfn))
#else
			if (!pfn_valid(pfn))
#endif
				continue;

			page = pfn_to_page(pfn);
			if (!test_bit(pfn, free_pages_bitmap)) {
				set_init_page_trace(page, 0, GFP_KERNEL);
				count++;
			}
		}
	}
	pr_debug("before insmod pagetrace used size: %dKb\n", count * 4);

	kfree(free_pages_bitmap);

	return 0;
}

void __init page_trace_mem_init(void)
{
	struct zone *zone;
	unsigned long total_page = 0;

	if (page_trace_disable)
		return;

	/* find_static_common_symbol(); */
	kthread_run(find_static_common_symbol, NULL, "PAGETRACE_TASK");

	aml_for_each_populated_zone(zone) {
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

	free_pages_bitmap = kzalloc(total_page / 8, GFP_KERNEL);
	if (!free_pages_bitmap) {
		vfree(trace_buffer);
		return;
	}
	statistic_before_insmod_mem();
}
#endif

MODULE_AUTHOR("Amlogic pagetrace owner");
MODULE_DESCRIPTION("Amlogic page trace driver for memory debug");
MODULE_LICENSE("GPL v2");
