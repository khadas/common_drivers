/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __SLAB_TRACE_H__
#define __SLAB_TRACE_H__

#include <asm/memory.h>
#include <asm/stacktrace.h>
#include <asm/sections.h>

/*
 * @entry:         rb tree for quick search/insert/delete
 * @s_addr:        start address for this slab object
 * @e_addr:        end address for this slab object
 * @object_count:  how many objects in this slab obj
 * @object_ip: a   array stores ip for each slab object
 */
struct slab_trace {
	struct rb_node entry;
	unsigned long s_addr;
	unsigned long e_addr;
	unsigned int object_count;
	unsigned int *object_ip;
};

/*
 * @trace_count:    how many slab_trace object we have used
 * @total_obj_size: total object size according obj size
 * @lock:           protection for rb tree update
 * @list:           link to root list
 * @root:           root for rb tree
 */
struct slab_trace_group {
	unsigned long trace_count;
	unsigned long total_obj_size;
	unsigned int  object_size;
	spinlock_t   lock;		/* protection for rb tree update */
	struct list_head list;
	struct kmem_cache *ip_cache;
	struct rb_root root;
};

#define SLAB_STACK_DEP		7
/*
 * @hash: hash value for stack
 * @entry: rb tree for quick search
 * @stack: stack for object
 */
struct slab_stack {
	unsigned int hash;
	unsigned int use_cnt;
	struct rb_node entry;
	unsigned long stack[SLAB_STACK_DEP];
};

struct slab_stack_master {
	int stack_cnt;
	spinlock_t stack_lock;		/* protection for rb tree update */
	struct kmem_cache *slab_stack_cache;
	struct rb_root stack_root;
};

int slab_trace_init(void);
int slab_trace_add_page(struct page *page, unsigned int order,
			struct kmem_cache *s, gfp_t flags);
int slab_trace_remove_page(struct page *page, unsigned int order, struct kmem_cache *s);
int slab_trace_mark_object(void *object, unsigned long ip,
			   struct kmem_cache *s);
int slab_trace_remove_object(void *object, struct kmem_cache *s);
int get_cache_max_order(struct kmem_cache *s);
#endif /* __SLAB_TRACE_H__ */
