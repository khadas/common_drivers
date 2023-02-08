// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/gfp.h>
#include <linux/proc_fs.h>
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
#include <linux/sched/clock.h>
#include <linux/gfp.h>
#include <linux/mm.h>
#include <linux/amlogic/slab_trace.h>
#include <linux/jhash.h>
#include <linux/slub_def.h>
#include <asm/stacktrace.h>

#define SLAB_TRACE_FLAG		(__GFP_ZERO | __GFP_NOFAIL | GFP_ATOMIC)

static int slab_trace_filter = 64; /* not print size < slab_trace_filter */
static LIST_HEAD(st_root);
static int slab_trace_en __read_mostly;
static struct kmem_cache *slab_trace_cache;
static struct slab_stack_master *stm;
static struct proc_dir_entry *d_slabtrace;

struct slab_trace_group *trace_group[KMALLOC_SHIFT_HIGH + 1] = {};

struct page_summary {
	struct rb_node entry;
	unsigned long ip;
	unsigned long cnt;
};

static int trace_cmp(const void *x1, const void *x2)
{
	struct page_summary *s1, *s2;

	s1 = (struct page_summary *)x1;
	s2 = (struct page_summary *)x2;
	return s2->cnt - s1->cnt;
}

static int __init early_slab_trace_param(char *buf)
{
	if (!buf)
		return -EINVAL;

	if (strcmp(buf, "off") == 0)
		slab_trace_en = false;
	else if (strcmp(buf, "on") == 0)
		slab_trace_en = true;

	pr_debug("slab_trace %sabled\n", slab_trace_en ? "dis" : "en");

	return 0;
}

early_param("slab_trace", early_slab_trace_param);

int save_obj_stack(unsigned long *stack, int depth)
{
#if CONFIG_AMLOGIC_KERNEL_VERSION >= 14515

#else
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
	frame.pc = (unsigned long)save_obj_stack;
#endif
	while (step < depth) {
	#ifdef CONFIG_ARM64
		ret = unwind_frame(current, &frame);
	#elif defined(CONFIG_ARM)
		ret = unwind_frame(&frame);
	#else	/* not supported */
		ret = -1;
	#endif
		if (ret < 0)
			return 0;
		if (step >= 1)	/* ignore first function */
			stack[step - 1] = frame.pc;
		step++;
	}
#endif
	return 0;
}

int __init slab_trace_init(void)
{
	struct slab_trace_group *group = NULL;
	struct kmem_cache *cache;
	int cache_size;
	char buf[64] = {0};
	int i;
	int size;

	if (!slab_trace_en)
		return -EINVAL;

	for (i = 0; i <= KMALLOC_SHIFT_HIGH; i++) {
		cache = kmalloc_caches[KMALLOC_NORMAL][i];
		if (!cache || cache->size >= PAGE_SIZE)
			continue;

		sprintf(buf, "trace_%s", cache->name);
		group = kzalloc(sizeof(*group), GFP_KERNEL);
		if (!group)
			goto nomem;

		cache_size = PAGE_SIZE * (1 << get_cache_max_order(cache));
		cache_size = (cache_size / cache->size) * sizeof(int);
		group->ip_cache = kmem_cache_create(buf, cache_size, cache_size,
						    SLAB_NOLEAKTRACE, NULL);
		if (!group->ip_cache)
			goto nomem;

		spin_lock_init(&group->lock);
		list_add(&group->list, &st_root);
		group->object_size = cache->size;
		trace_group[i] = group;
		pr_debug("%s, trace group %p for %s, %d:%d, cache_size:%d:%d\n",
			 __func__, group, cache->name,
			 cache->size, cache->object_size,
			 cache_size, get_cache_max_order(cache));
	}
	stm = kzalloc(sizeof(*stm), GFP_KERNEL);
	//stm->slab_stack_cache = KMEM_CACHE(slab_stack, SLAB_NOLEAKTRACE);
	size = sizeof(struct slab_stack);
	stm->slab_stack_cache = kmem_cache_create("slab_stack", size, size, SLAB_NOLEAKTRACE, NULL);
	spin_lock_init(&stm->stack_lock);

	//slab_trace_cache = KMEM_CACHE(slab_trace, SLAB_NOLEAKTRACE);
	size = sizeof(struct slab_trace);
	slab_trace_cache = kmem_cache_create("slab_trace", size, size, SLAB_NOLEAKTRACE, NULL);
	WARN_ON(!slab_trace_cache);
	pr_debug("%s, create slab trace cache:%p\n",
		__func__, slab_trace_cache);

	return 0;
nomem:
	pr_err("%s, failed to create trace group for %s\n",
	       __func__, buf);
	kfree(group);
	return -ENOMEM;
}

/*
 * This function must under protect of lock
 */
static struct slab_trace *find_slab_trace(struct slab_trace_group *group,
					  unsigned long addr)
{
	struct rb_node *rb;
	struct slab_trace *trace;

	rb = group->root.rb_node;
	while (rb) {
		trace = rb_entry(rb, struct slab_trace, entry);
		if (addr >= trace->s_addr && addr < trace->e_addr)
			return trace;
		if (addr < trace->s_addr)
			rb = trace->entry.rb_left;
		if (addr >= trace->e_addr)
			rb = trace->entry.rb_right;
	}
	return NULL;
}

/*
 * Conversion table for small slabs sizes / 8 to the index in the
 * kmalloc array. This is necessary for slabs < 192 since we have non power
 * of two cache sizes there. The size of larger slabs can be determined using
 * fls.
 */
static u8 size_index[24] __ro_after_init = {
	3,	/* 8 */
	4,	/* 16 */
	5,	/* 24 */
	5,	/* 32 */
	6,	/* 40 */
	6,	/* 48 */
	6,	/* 56 */
	6,	/* 64 */
	1,	/* 72 */
	1,	/* 80 */
	1,	/* 88 */
	1,	/* 96 */
	7,	/* 104 */
	7,	/* 112 */
	7,	/* 120 */
	7,	/* 128 */
	2,	/* 136 */
	2,	/* 144 */
	2,	/* 152 */
	2,	/* 160 */
	2,	/* 168 */
	2,	/* 176 */
	2,	/* 184 */
	2	/* 192 */
};

static inline unsigned int size_index_elem(unsigned int bytes)
{
	return (bytes - 1) / 8;
}

/*
 * Find the kmem_cache structure that serves a given size of
 * allocation
 */
static int get_kmem_cache_by_size(size_t size)
{
	int index;

	if (size <= 192) {
		if (!size)
			return -1;

		index = size_index[size_index_elem(size)];
	} else {
		if (WARN_ON_ONCE(size > KMALLOC_MAX_CACHE_SIZE))
			return -1;
		index = fls(size - 1);
	}

	return index;
}

static int is_trace_func(const char *name)
{
	int ret = 0;

	if (!strncmp(name, "kmalloc-", 8))
		ret = 1;

	return ret;
}

int slab_trace_add_page(struct page *page, unsigned int order,
			struct kmem_cache *s, gfp_t flag)
{
	struct rb_node **link, *parent = NULL;
	struct slab_trace *trace = NULL, *tmp;
	struct slab_trace_group *group;
	void *buf = NULL;
	unsigned long addr, flags;
	int obj_cnt;
	int s_index;

	if (!slab_trace_en || !page || !s || !is_trace_func(s->name))
		return -EINVAL;

	s_index = get_kmem_cache_by_size(s->size);
	if (s_index < 0 || s_index > KMALLOC_SHIFT_HIGH)
		return -EINVAL;
	group = trace_group[s_index];
	if (!group)
		return -EINVAL;

	trace = kmem_cache_alloc(slab_trace_cache, SLAB_TRACE_FLAG);
	if (!trace)
		goto nomem;

	obj_cnt = PAGE_SIZE * (1 << order) / s->size;
	buf = kmem_cache_alloc(group->ip_cache, SLAB_TRACE_FLAG);
	if (!buf)
		goto nomem;

	addr = (unsigned long)page_address(page);
	trace->s_addr = addr;
	trace->e_addr = addr + PAGE_SIZE * (1 << order);
	trace->object_count = obj_cnt;
	trace->object_ip = buf;
	/*
	 * insert it to rb_tree;
	 */
	spin_lock_irqsave(&group->lock, flags);
	link = &group->root.rb_node;
	while (*link) {
		parent = *link;
		tmp = rb_entry(parent, struct slab_trace, entry);
		if (addr < tmp->s_addr)
			link = &tmp->entry.rb_left;
		else if (addr >= tmp->e_addr)
			link = &tmp->entry.rb_right;
	}
	rb_link_node(&trace->entry, parent, link);
	rb_insert_color(&trace->entry, &group->root);
	group->trace_count++;
	spin_unlock_irqrestore(&group->lock, flags);
	pr_debug("%s, add:%lx-%lx, buf:%p, trace:%p to group:%p, %ld, obj:%d\n",
		 s->name, addr, trace->e_addr,
		 buf, trace, group, group->trace_count, obj_cnt);
	return 0;

nomem:
	pr_err("%s, failed to trace obj %p for %s, trace:%p\n", __func__,
	       page_address(page), s->name, trace);
	kfree(trace);
	return -ENOMEM;
}

int slab_trace_remove_page(struct page *page, unsigned int order, struct kmem_cache *s)
{
	struct slab_trace *trace = NULL;
	struct slab_trace_group *group;
	unsigned long addr, flags;
	int s_index;

	if (!slab_trace_en || !page || !s || !is_trace_func(s->name))
		return -EINVAL;

	s_index = get_kmem_cache_by_size(s->size);
	if (s_index < 0 || s_index > KMALLOC_SHIFT_HIGH)
		return -EINVAL;
	group = trace_group[s_index];
	if (!group)
		return -EINVAL;

	addr  = (unsigned long)page_address(page);
	spin_lock_irqsave(&group->lock, flags);
	trace = find_slab_trace(group, addr);
	if (!trace) {
		spin_unlock_irqrestore(&group->lock, flags);
		return 0;
	}
	rb_erase(&trace->entry, &group->root);
	group->trace_count--;
	spin_unlock_irqrestore(&group->lock, flags);
	WARN_ON((addr + PAGE_SIZE * (1 << order)) != trace->e_addr);
	pr_debug("%s, rm: %lx-%lx, buf:%p, trace:%p to group:%p, %ld\n",
		 s->name, addr, trace->e_addr,
		 trace->object_ip, trace, group, group->trace_count);
	kfree(trace->object_ip);
	kfree(trace);
	return 0;
}

static int record_stack(unsigned int hash, unsigned long *stack)
{
	struct rb_node **link, *parent = NULL;
	struct slab_stack *tmp, *new;
	unsigned long flags;

	/* No matched hash, we need create another one */
	new = kmem_cache_alloc(stm->slab_stack_cache, SLAB_TRACE_FLAG);
	if (!new)
		return -ENOMEM;

	spin_lock_irqsave(&stm->stack_lock, flags);
	link = &stm->stack_root.rb_node;
	while (*link) {
		parent = *link;
		tmp = rb_entry(parent, struct slab_stack, entry);
		if (hash == tmp->hash) {
			tmp->use_cnt++;
			/* hash match, but we need check stack same? */
			if (memcmp(stack, tmp->stack, sizeof(tmp->stack))) {
				int i;

				pr_err("%s stack hash conflict:%x\n",
				       __func__, hash);
				for (i = 0; i < SLAB_STACK_DEP; i++) {
					pr_err("%16lx %16lx, %ps, %ps\n",
					       tmp->stack[i], stack[i],
					       (void *)tmp->stack[i],
					       (void *)stack[i]);
				}
			}
			spin_unlock_irqrestore(&stm->stack_lock, flags);
			kfree(new);
			return 0;
		} else if (hash < tmp->hash) {
			link = &tmp->entry.rb_left;
		} else {
			link = &tmp->entry.rb_right;
		}
	}
	/* add to stack tree */
	new->hash    = hash;
	new->use_cnt = 1;
	memcpy(new->stack, stack, sizeof(new->stack));
	rb_link_node(&new->entry, parent, link);
	rb_insert_color(&new->entry, &stm->stack_root);
	stm->stack_cnt++;
	spin_unlock_irqrestore(&stm->stack_lock, flags);

	return 0;
}

static struct slab_stack *get_hash_stack(unsigned int hash)
{
	struct rb_node *rb;
	struct slab_stack *stack;

	rb = stm->stack_root.rb_node;
	while (rb) {
		stack = rb_entry(rb, struct slab_stack, entry);
		if (hash == stack->hash)
			return stack;

		if (hash < stack->hash)
			rb = stack->entry.rb_left;
		if (hash > stack->hash)
			rb = stack->entry.rb_right;
	}
	return NULL;
}

int slab_trace_mark_object(void *object, unsigned long ip,
			   struct kmem_cache *s)
{
	struct slab_trace *trace = NULL;
	struct slab_trace_group *group;
	unsigned long addr, flags, index;
	unsigned long stack[SLAB_STACK_DEP] = {0};
	unsigned int hash, len;
	int s_index;

	if (!slab_trace_en || !object || !s || !is_trace_func(s->name))
		return -EINVAL;

	s_index = get_kmem_cache_by_size(s->size);
	if (s_index < 0 || s_index > KMALLOC_SHIFT_HIGH)
		return -EINVAL;
	group = trace_group[s_index];
	if (!group)
		return -EINVAL;

	addr  = (unsigned long)object;
	spin_lock_irqsave(&group->lock, flags);
	trace = find_slab_trace(group, addr);
	spin_unlock_irqrestore(&group->lock, flags);
	if (!trace)
		return -ENODEV;

	len  = sizeof(stack);
	len /= sizeof(int);
	group->total_obj_size += s->size;
	index = (addr - trace->s_addr) / s->size;
	WARN_ON(index >= trace->object_count);
	if (save_obj_stack(stack, SLAB_STACK_DEP))
		return -EINVAL;
	hash = jhash2((unsigned int *)stack, len, 0x9747b28c);
	record_stack(hash, stack);
	trace->object_ip[index] = hash;
	pr_debug("%s, mk object:%p,%lx, idx:%ld, trace:%p, group:%p,%ld, %ps\n",
		 s->name, object, trace->s_addr, index,
		 trace, group, group->total_obj_size, (void *)ip);
	return 0;
}

int slab_trace_remove_object(void *object, struct kmem_cache *s)
{
	struct slab_trace *trace = NULL;
	struct slab_trace_group *group;
	unsigned long addr, flags, index;
	unsigned int hash, need_free = 0;
	struct slab_stack *ss;
	int s_index;

	if (!slab_trace_en || !object || !s || !is_trace_func(s->name))
		return -EINVAL;

	s_index = get_kmem_cache_by_size(s->size);
	if (s_index < 0 || s_index > KMALLOC_SHIFT_HIGH)
		return -EINVAL;
	group = trace_group[s_index];
	if (!group)
		return -EINVAL;

	addr  = (unsigned long)object;
	spin_lock_irqsave(&group->lock, flags);
	trace = find_slab_trace(group, addr);
	spin_unlock_irqrestore(&group->lock, flags);
	if (!trace)
		return -EINVAL;

	group->total_obj_size -= s->size;
	index = (addr - trace->s_addr) / s->size;
	WARN_ON(index >= trace->object_count);

	/* remove hashed stack */
	hash = trace->object_ip[index];
	spin_lock_irqsave(&stm->stack_lock, flags);
	ss = get_hash_stack(hash);
	if (ss) {
		ss->use_cnt--;
		if (!ss->use_cnt) {
			rb_erase(&ss->entry, &stm->stack_root);
			stm->stack_cnt--;
			need_free = 1;
		}
	}
	spin_unlock_irqrestore(&stm->stack_lock, flags);
	trace->object_ip[index] = 0;
	if (need_free)
		kfree(ss);
	pr_debug("%s, rm object: %p, %lx, idx:%ld, trace:%p, group:%p, %ld\n",
		 s->name, object, trace->s_addr, index,
		 trace, group, group->total_obj_size);
	return 0;
}

/*
 * functions to summary slab trace
 */
#define SLAB_TRACE_SHOW_CNT		1024

static int find_slab_hash(unsigned int hash, struct page_summary *sum,
			  int range, int *funcs, int size, struct rb_root *root)
{
	struct rb_node **link, *parent = NULL;
	struct page_summary *tmp;

	link  = &root->rb_node;
	while (*link) {
		parent = *link;
		tmp = rb_entry(parent, struct page_summary, entry);
		if (hash == tmp->ip) { /* match */
			tmp->cnt += size;
			return 0;
		} else if (hash < tmp->ip) {
			link = &tmp->entry.rb_left;
		} else {
			link = &tmp->entry.rb_right;
		}
	}
	/* not found, get a new page summary */
	if (*funcs >= range)
		return -ERANGE;
	tmp       = &sum[*funcs];
	tmp->ip   = hash;
	tmp->cnt += size;
	*funcs    = *funcs + 1;
	rb_link_node(&tmp->entry, parent, link);
	rb_insert_color(&tmp->entry, root);
	return 0;
}

static int update_slab_trace(struct seq_file *m, struct slab_trace_group *group,
			     struct page_summary *sum, unsigned long *tick,
			     int remain)
{
	struct rb_node *rb;
	struct slab_trace *trace;
	unsigned long flags, time;
	int i, r, funcs = 0;
	struct rb_root root = RB_ROOT;

	/* This may lock long time */
	time = sched_clock();
	spin_lock_irqsave(&group->lock, flags);
	for (rb = rb_first(&group->root); rb; rb = rb_next(rb)) {
		trace = rb_entry(rb, struct slab_trace, entry);
		for (i = 0; i < trace->object_count; i++) {
			if (!trace->object_ip[i])
				continue;

			r = find_slab_hash(trace->object_ip[i], sum, remain,
					   &funcs, group->object_size, &root);
			if (r) {
				pr_err("slab trace cout is not enough\n");
				spin_unlock_irqrestore(&group->lock, flags);
				return -ERANGE;
			}
		}
	}
	spin_unlock_irqrestore(&group->lock, flags);
	*tick = sched_clock() - time;
	return funcs;
}

static void show_slab_trace(struct seq_file *m, struct page_summary *p,
			    int count)
{
	int i, j;
	unsigned long total = 0, flags;
	struct slab_stack *stack;

	seq_printf(m, "%s          %s, %s\n",
		   "size(bytes)", "kaddr", "function");
	seq_puts(m, "------------------------------\n");

	sort(p, count, sizeof(*p), trace_cmp, NULL);

	for (i = 0; i < count; i++) {
		if (!p[i].cnt)	/* may be empty after merge */
			continue;

		total += p[i].cnt;
		if (p[i].cnt >= slab_trace_filter) {
			spin_lock_irqsave(&stm->stack_lock, flags);
			stack = get_hash_stack(p[i].ip);
			spin_unlock_irqrestore(&stm->stack_lock, flags);
			if (!stack)
				continue;

			seq_printf(m, "%8ld, %16lx, %ps\n",
				   p[i].cnt, stack->stack[0],
				   (void *)stack->stack[0]);
			for (j = 1; j < SLAB_STACK_DEP; j++) {
				seq_printf(m, "%8s  %16lx, %ps\n",
					   " ", stack->stack[j],
					   (void *)stack->stack[j]);
			}
		}
	}
	seq_printf(m, "total kmalloc slabs:%6ld, %9ld kB\n",
		   total, total >> 10);
	seq_puts(m, "------------------------------\n");
}

static int slabtrace_show(struct seq_file *m, void *arg)
{
	struct page_summary *sum, *p;
	int ret = 0, remain, alloc;
	struct slab_trace_group *group;
	unsigned long ticks, group_time = 0, funcs = 0;

	alloc = stm->stack_cnt + 200;
	sum   = vzalloc(sizeof(*sum) * alloc);
	if (!sum)
		return -ENOMEM;
	m->private = sum;

	/* update only once */
	seq_puts(m, "==============================\n");
	p      = sum;
	remain = alloc;
	ticks  = sched_clock();
	list_for_each_entry(group, &st_root, list) {
		ret = update_slab_trace(m, group, p, &group_time, remain);
		seq_printf(m, "%s-%4d, trace:%8ld, total:%10ld, time:%12ld, f:%d\n",
			   "slab kmalloc", group->object_size,
			   group->trace_count, group->total_obj_size,
			   group_time, ret);
		if (ret < 0) {
			seq_printf(m, "Error %d in slab %d\n",
				   ret, group->object_size);
			return -ERANGE;
		}
		funcs  += ret;
		p      += ret;
		remain -= ret;
	}
	seq_puts(m, "------------------------------\n");
	show_slab_trace(m, sum, funcs);
	ticks = sched_clock() - ticks;

	seq_printf(m, "SHOW_CNT:%d, tick:%ld ns, funs:%ld\n",
		   stm->stack_cnt, ticks, funcs);
	seq_puts(m, "==============================\n");

	vfree(sum);

	return 0;
}

static int slabtrace_open(struct inode *inode, struct file *file)
{
	return single_open(file, slabtrace_show, NULL);
}

static const struct proc_ops slabtrace_proc_ops = {
	.proc_open	= slabtrace_open,
	.proc_read	= seq_read,
	.proc_lseek	= seq_lseek,
	.proc_release	= single_release,
};

static int __init slab_trace_module_init(void)
{
	if (slab_trace_en)
		d_slabtrace = proc_create("slabtrace", 0444,
					  NULL, &slabtrace_proc_ops);
	if (IS_ERR_OR_NULL(d_slabtrace)) {
		pr_err("%s, create slabtrace failed\n", __func__);
		return -1;
	}

	return 0;
}

static void __exit slab_trace_module_exit(void)
{
	if (d_slabtrace)
		proc_remove(d_slabtrace);
}
module_init(slab_trace_module_init);
module_exit(slab_trace_module_exit);
