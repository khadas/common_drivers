// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * drivers/amlogic/memory_ext/aml_cma.c
 *
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#include <internal.h>
#include <linux/stddef.h>
#include <linux/compiler.h>
#include <linux/mm.h>
#include <linux/kernel.h>
#include <linux/rmap.h>
#include <linux/kthread.h>
#include <linux/sched/rt.h>
#include <linux/completion.h>
#include <linux/module.h>
#include <linux/swap.h>
#include <linux/migrate.h>
#include <linux/cpu.h>
#include <linux/page-isolation.h>
#include <linux/spinlock_types.h>
#include <linux/amlogic/aml_cma.h>
#include <linux/sched/signal.h>
#include <linux/hugetlb.h>
#include <linux/cma.h>
#include <linux/proc_fs.h>
#include <linux/platform_device.h>
#include <linux/sched/clock.h>
#include <linux/oom.h>
#include <linux/of.h>
#include <linux/shrinker.h>
#include <linux/vmalloc.h>
#include <asm/system_misc.h>
#include <asm/pgtable.h>
#include <linux/page_pinner.h>
#include <trace/events/page_isolation.h>
#if IS_MODULE(CONFIG_AMLOGIC_CMA)
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/jump_label.h>
#include <linux/types.h>
#endif
#ifdef CONFIG_PAGE_OWNER
#include <linux/page_owner.h>
#endif

/* from mm/ path */
#include <internal.h>

#if IS_ENABLED(CONFIG_AMLOGIC_PAGE_TRACE)
#include <linux/amlogic/page_trace.h>
#endif /* CONFIG_AMLOGIC_PAGE_TRACE */

#if IS_ENABLED(CONFIG_AMLOGIC_USER_FAULT)
#include <linux/amlogic/user_fault.h>
#endif

#include <trace/hooks/mm.h>
#include <linux/amlogic/gki_module.h>
#include <linux/swap.h>

#define MAX_DEBUG_LEVEL		5
#define MAX_JOB_NUM		40

struct work_cma {
	struct list_head list;
	unsigned long pfn;
	unsigned long count;
	struct task_struct *host;
	int ret;
};

struct cma_pcp {
	struct completion start;
	struct completion end;
	struct task_struct *task;
	int cpu;
};

static DEFINE_MUTEX(cma_mutex);
static atomic_long_t nr_cma_allocated;
static bool can_boost;
static DEFINE_PER_CPU(struct cma_pcp, cma_pcp_thread);
static struct proc_dir_entry *dentry;
static int cma_alloc_trace;
int cma_debug_level;
static int allow_cma_tasks;
static unsigned long cma_isolated;
static struct list_head work_list;
static spinlock_t work_list_lock;		/* protect job list */

static atomic_t cma_allocate;

#ifdef CONFIG_ANDROID_VENDOR_HOOKS
static int cma_enabled;
module_param(cma_enabled, int, 0644);
#endif

#ifdef CONFIG_AMLOGIC_CMA_DIS
unsigned long ion_cma_allocated;
#endif

#if IS_MODULE(CONFIG_AMLOGIC_CMA)
struct dummy_cma {
	unsigned long   base_pfn;
	unsigned long   count;
	unsigned long   *bitmap;
	unsigned int order_per_bit; /* Order of pages represented by one bit */
	spinlock_t	lock;
#ifdef CONFIG_CMA_DEBUGFS
	struct hlist_head mem_head;
	spinlock_t mem_head_lock; /* for cma debugfs */
	struct debugfs_u32_array dfs_bitmap;
#endif
	char name[CMA_MAX_NAME];
#ifdef CONFIG_CMA_SYSFS
	/* the number of CMA page successful allocations */
	atomic64_t nr_pages_succeeded;
	/* the number of CMA page allocation failures */
	atomic64_t nr_pages_failed;
	/* kobject requires dynamic object */
	struct cma_kobject *cma_kobj;
#endif
};

static inline unsigned long cma_bitmap_maxno(struct dummy_cma *cma)
{
	return cma->count >> cma->order_per_bit;
}

#ifdef CONFIG_CMA_SYSFS
static void cma_sysfs_account_success_pages(struct dummy_cma *cma, unsigned long nr_pages)
{
	atomic64_add(nr_pages, &cma->nr_pages_succeeded);
}

static void cma_sysfs_account_fail_pages(struct dummy_cma *cma, unsigned long nr_pages)
{
	atomic64_add(nr_pages, &cma->nr_pages_failed);
}
#else
static inline void cma_sysfs_account_success_pages(struct dummy_cma *cma,
						   unsigned long nr_pages) {};
static inline void cma_sysfs_account_fail_pages(struct dummy_cma *cma,
						unsigned long nr_pages) {};
#endif

static unsigned long cma_bitmap_aligned_mask(const struct dummy_cma *cma,
					     unsigned int align_order)
{
	if (align_order <= cma->order_per_bit)
		return 0;
	return (1UL << (align_order - cma->order_per_bit)) - 1;
}

/*
 * Find the offset of the base PFN from the specified align_order.
 * The value returned is represented in order_per_bits.
 */
static unsigned long cma_bitmap_aligned_offset(const struct dummy_cma *cma,
					       unsigned int align_order)
{
	return (cma->base_pfn & ((1UL << align_order) - 1))
		>> cma->order_per_bit;
}

static unsigned long cma_bitmap_pages_to_bits(const struct dummy_cma *cma,
					      unsigned long pages)
{
	return ALIGN(pages, 1UL << cma->order_per_bit) >> cma->order_per_bit;
}

static void cma_clear_bitmap(struct dummy_cma *cma, unsigned long pfn,
			     unsigned long count)
{
	unsigned long bitmap_no, bitmap_count;
	unsigned long flags;

	bitmap_no = (pfn - cma->base_pfn) >> cma->order_per_bit;
	bitmap_count = cma_bitmap_pages_to_bits(cma, count);

	spin_lock_irqsave(&cma->lock, flags);
	bitmap_clear(cma->bitmap, bitmap_no, bitmap_count);
	spin_unlock_irqrestore(&cma->lock, flags);
}

unsigned long aml_totalcma_pages;

#ifdef CONFIG_PAGE_PINNER
static void __nocfi aml_page_pinner_failure_detect(struct page *page)
{
	/*
	 *if (!static_branch_unlikely(&page_pinner_inited))
	 *	return;
	 *
	 *if (!static_branch_unlikely(&failure_tracking))
	 *	return;
	 */

	//__page_pinner_failure_detect(page);
}
#else
static void aml_page_pinner_failure_detect(struct page *page)
{
}
#endif /* CONFIG_PAGE_PINNER */

#ifdef CONFIG_TRANSPARENT_HUGEPAGE
void (*aml_prep_huge_page)(struct page *page);
#else /* CONFIG_TRANSPARENT_HUGEPAGE */
static inline void aml_prep_huge_page(struct page *page) {}
#endif

unsigned int (*aml_reclaim_clean_pages_from_list)(struct zone *zone,
					    struct list_head *page_list);
int (*aml_isolate_pages_range)(struct compact_control *cc, unsigned long start_pfn,
							unsigned long end_pfn);
void (*aml_rmap_walk)(struct page *page, struct rmap_walk_control *rwc);
void (*aml_undo_isolate_page_range)(unsigned long start_pfn, unsigned long end_pfn,
			    unsigned int migratetype);
unsigned long (*aml_iso_free_range)(struct compact_control *cc,
			unsigned long start_pfn, unsigned long end_pfn);
void (*aml_drain_all_pages)(struct zone *zone);
void (*aml_lru_add_drain_all)(void);
#if CONFIG_AMLOGIC_KERNEL_VERSION >= 14515
int (*aml_start_isolate_page_range)(unsigned long start_pfn, unsigned long end_pfn,
			     unsigned int migratetype, int flags,
			     unsigned long *failed_pfn);
#else
int (*aml_start_isolate_page_range)(unsigned long start_pfn, unsigned long end_pfn,
			     unsigned int migratetype, int flags);
#endif

unsigned long (*aml_kallsyms_lookup_name)(const char *name);

/* For each probe you need to allocate a kprobe structure */
static struct kprobe kp_lookup_name = {
	.symbol_name	= "kallsyms_lookup_name",
};
#endif

#ifdef CONFIG_PAGE_OWNER
#if IS_MODULE(CONFIG_AMLOGIC_CMA)
void (*aml__dump_owner)(const struct page *page);
#else
static void aml__dump_owner(const struct page *page)
{
	__dump_page_owner(page);
}
#endif
#else
static void aml__dump_owner(const struct page *page)
{
}
#endif

/*
 * We insert a none-mapping vm area to vmalloc space
 * and dynamic adjust it's size according nr_cma_allocated.
 * Just in order to let all driver allocated cma size counted
 * into KernelUsed item for dumpsys meminfo command on Android
 * layer
 */

static void get_cma_alloc_ref(void)
{
	atomic_inc(&cma_allocate);
}

static void put_cma_alloc_ref(void)
{
	atomic_dec(&cma_allocate);
}

unsigned long get_cma_allocated(void)
{
	return atomic_long_read(&nr_cma_allocated);
}
EXPORT_SYMBOL(get_cma_allocated);

#if IS_BUILTIN(CONFIG_AMLOGIC_CMA)
static int cma_alloc_ref(void)
{
	return atomic_read(&cma_allocate);
}

static bool cma_first_wm_low __read_mostly;

static int __init early_cma_first_wm_low_param(char *buf)
{
	if (!buf)
		return -EINVAL;

	if (strcmp(buf, "off") == 0)
		cma_first_wm_low = false;
	else if (strcmp(buf, "on") == 0)
		cma_first_wm_low = true;

	pr_info("cma_first_wm_low %sabled\n", cma_first_wm_low ? "en" : "dis");

	return 0;
}

early_param("cma_first_wm_low", early_cma_first_wm_low_param);

void check_cma_isolated(unsigned long *isolate,
			unsigned long active, unsigned long inactive)
{
	long tmp;
	unsigned long raw = *isolate;

	tmp = *isolate - cma_isolated;
	if (tmp < 0)
		*isolate = 0;
	else
		*isolate = tmp;

	WARN_ONCE(*isolate > (inactive + active) / 2,
		  "isolated:%ld, cma:%ld, inactive:%ld, active:%ld\n",
		  raw, cma_isolated, inactive, active);
}

bool can_use_cma(gfp_t gfp_flags)
{
#ifdef CONFIG_ANDROID_VENDOR_HOOKS
	if (!cma_enabled)
		return false;
#endif

	if (unlikely(!cma_first_wm_low))
		return false;

	if (cma_forbidden_mask(gfp_flags))
		return false;

	if (cma_alloc_ref())
		return false;

	if (task_nice(current) > 0)
		return false;

	return true;
}
EXPORT_SYMBOL(can_use_cma);

void update_gfp_flags(gfp_t *gfp)
{
	/*
	 * There are 2 bit flags in gfp:
	 * __GFP_CMA: indicate to get CMA page from buddy system
	 * __GFP_NO_CMA: indicate not get CMA page from buddy system
	 *
	 * Kernel only allow ZRAM/ANON pages use cma, but cma pool usually
	 * very large, if file cache can't use cma(movable) then it will
	 * cause system hung. So we should use cma with all movable page but
	 * filter some special case which may cause CMA allocation fail by
	 * __GFP_NO_CMA.
	 */
	if (can_use_cma(*gfp))
		*gfp |=  __GFP_CMA;
	else
		*gfp &= ~__GFP_CMA;
}

#define ACTIVE_MIGRATE		3
#define INACTIVE_MIGRATE	(ACTIVE_MIGRATE * 4)
static int filecache_need_migrate(struct page *page)
{
	if (PageActive(page) && page_mapcount(page) >= ACTIVE_MIGRATE)
		return 1;

	if (!PageActive(page) && page_mapcount(page) >= INACTIVE_MIGRATE)
		return 1;

	if (PageUnevictable(page))
		return 0;

	return 0;
}

void cma_keep_high_active(struct page *page, struct list_head *high,
			  struct list_head *clean)
{
	if (filecache_need_migrate(page)) {
		/*
		 * leaving pages with high map count to migrate
		 * instead of reclaimed. This can help to avoid
		 * file cache jolt if reclaim large cma size
		 */
		list_move(&page->lru, high);
	} else {
		ClearPageActive(page);
		list_move(&page->lru, clean);
	}
}

bool cma_page(struct page *page)
{
	int migrate_type = 0;

	if (!page)
		return false;
	migrate_type = get_pageblock_migratetype(page);
	if (is_migrate_cma(migrate_type) ||
	    is_migrate_isolate(migrate_type)) {
		return true;
	}
	return false;
}
EXPORT_SYMBOL(cma_page);
#endif

#if (IS_MODULE(CONFIG_AMLOGIC_PAGE_TRACE) && IS_MODULE(CONFIG_AMLOGIC_CMA)) || \
	(IS_BUILTIN(CONFIG_AMLOGIC_PAGE_TRACE) && IS_BUILTIN(CONFIG_AMLOGIC_CMA))
static void update_cma_page_trace(struct page *page, unsigned long cnt)
{
	long i;
	unsigned long fun;

	if (!page)
		return;

	fun = find_back_trace();
	if (cma_alloc_trace)
		pr_info("c a p:%lx, c:%ld, f:%ps\n",
			page_to_pfn(page), cnt, (void *)fun);
	for (i = 0; i < cnt; i++) {
	#if IS_BUILTIN(CONFIG_AMLOGIC_CMA)
		set_page_trace(page, 0, __GFP_NO_CMA, (void *)fun);
	#else
		set_page_trace(page, 0, 0, (void *)fun);
	#endif
		page++;
	}
}
#endif /* CONFIG_AMLOGIC_PAGE_TRACE */

void aml_cma_alloc_pre_hook(int *dummy, int count, unsigned long *tick)
{
	get_cma_alloc_ref();

	/* temporary increase task priority if allocate many pages */
	*dummy = task_nice(current);
	*tick  = sched_clock();
	if (count >= (pageblock_nr_pages / 2))
		set_user_nice(current, -18);
}

void aml_cma_alloc_post_hook(int *dummy, int count, struct page *page,
			     unsigned long tick, int ret)
{
	put_cma_alloc_ref();
	if (page)
		atomic_long_add(count, &nr_cma_allocated);

	if (count >= (pageblock_nr_pages / 2))
		set_user_nice(current, *dummy);
	cma_debug(0, NULL, "return page:%lx, tick:%16ld, ret:%d\n",
		  page ? page_to_pfn(page) : 0, (unsigned long)sched_clock() - tick, ret);
#if (IS_MODULE(CONFIG_AMLOGIC_PAGE_TRACE) && IS_MODULE(CONFIG_AMLOGIC_CMA)) || \
	(IS_BUILTIN(CONFIG_AMLOGIC_PAGE_TRACE) && IS_BUILTIN(CONFIG_AMLOGIC_CMA))
	update_cma_page_trace(page, count);
#endif /* CONFIG_AMLOGIC_PAGE_TRACE */
}

#if IS_BUILTIN(CONFIG_AMLOGIC_CMA)
void check_water_mark(long free_pages, unsigned long mark)
{
	/* already set */
	if (likely(cma_first_wm_low))
		return;

	/* cma first after system boot, dont care the watermark when:
	 * 1) total ram is smaller than 1G or
	 * 2) cma pages is more than 30% of totalram  (total - reserved)
	 */
	if ((totalcma_pages * 10 > 3 * totalram_pages() ||
	     totalram_pages() < 262144) && system_state == SYSTEM_RUNNING) {
		cma_first_wm_low = true;
		pr_info("Now can use cma1, free:%ld, wm:%ld, cma:%ld, total:%ld\n",
			free_pages, mark, totalcma_pages, totalram_pages());
	}
	if (free_pages <= mark && free_pages > 0) {
		/* do not using cma until water mark is low */
		cma_first_wm_low = true;
		pr_info("Now can use cma, free:%ld, wm:%ld\n",
			free_pages, mark);
	}
}

#ifdef CONFIG_ARM64
static int clear_cma_pagemap(unsigned long pfn, unsigned long count)
{
	pgd_t *pgd;
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd;
	unsigned long addr, end;
	struct mm_struct *mm;

	addr = (unsigned long)pfn_to_kaddr(pfn);
	end  = addr + count * PAGE_SIZE;
	mm = &init_mm;
	for (; addr < end; addr += PMD_SIZE) {
		pgd = pgd_offset(mm, addr);
		if (pgd_none(*pgd) || pgd_bad(*pgd))
			break;

		p4d = p4d_offset(pgd, addr);
		if (p4d_none(*p4d) || p4d_bad(*p4d))
			break;

		pud = pud_offset(p4d, addr);
		if (pud_none(*pud) || pud_bad(*pud))
			break;

		pmd = pmd_offset(pud, addr);
		if (pmd_none(*pmd))
			break;

		pr_debug("%s, addr:%lx, pgd:%p %llx, pmd:%p %llx\n",
			 __func__, addr, pgd,
			 pgd_val(*pgd), pmd, pmd_val(*pmd));
		pmd_clear(pmd);
	}

	return 0;
}
#endif

int setup_cma_full_pagemap(unsigned long pfn, unsigned long count)
{
#ifdef CONFIG_ARM
	/*
	 * arm already create level 3 mmu mapping for lowmem cma.
	 * And if high mem cma, there is no mapping. So nothing to
	 * do for arch arm.
	 */
	return 0;
#elif defined(CONFIG_ARM64)
	struct vm_area_struct vma = {};
	unsigned long addr, size;
	int ret;

	clear_cma_pagemap(pfn, count);
	addr = (unsigned long)pfn_to_kaddr(pfn);
	size = count * PAGE_SIZE;
	vma.vm_mm    = &init_mm;
	vma.vm_start = addr;
	vma.vm_end   = addr + size;
	vma.vm_page_prot = PAGE_KERNEL;
	ret = remap_pfn_range(&vma, addr, pfn,
			      size, vma.vm_page_prot);
	if (ret < 0)
		pr_info("%s, remap pte failed:%d, cma:%lx\n",
			__func__, ret, pfn);
	return 0;
#else
	#error "NOT supported ARCH"
#endif
}
EXPORT_SYMBOL(setup_cma_full_pagemap);

int cma_mmu_op(struct page *page, int count, bool set)
{
	pgd_t *pgd;
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;
	unsigned long addr, end;
	struct mm_struct *mm;
	struct cma *cma;

	if (!page || PageHighMem(page))
		return -EINVAL;

	/* TODO: owner must make sure this cma pool have called
	 * setup_cma_full_pagemap before call this function
	 */
	if (!cma_page(page)) {
		pr_debug("%s, page:%lx is not cma or no clear-map, cma:%px\n",
			 __func__, page_to_pfn(page), cma);
		return -EINVAL;
	}

	addr = (unsigned long)page_address(page);
	end  = addr + count * PAGE_SIZE;
	mm = &init_mm;
	for (; addr < end; addr += PAGE_SIZE) {
		pgd = pgd_offset(mm, addr);
		if (pgd_none(*pgd) || pgd_bad(*pgd))
			break;

		p4d = p4d_offset(pgd, addr);
		if (p4d_none(*p4d) || p4d_bad(*p4d))
			break;

		pud = pud_offset(p4d, addr);
		if (pud_none(*pud) || pud_bad(*pud))
			break;

		pmd = pmd_offset(pud, addr);
		if (pmd_none(*pmd))
			break;

		pte = pte_offset_map(pmd, addr);
		if (set)
			set_pte_at(mm, addr, pte, mk_pte(page, PAGE_KERNEL));
		else
			pte_clear(mm, addr, pte);
		pte_unmap(pte);
	#ifdef CONFIG_ARM
		pr_debug("%s, add:%lx, pgd:%p %x, pmd:%p %x, pte:%p %x\n",
			 __func__, addr, pgd, (int)pgd_val(*pgd),
			 pmd, (int)pmd_val(*pmd), pte, (int)pte_val(*pte));
	#elif defined(CONFIG_ARM64)
		pr_debug("%s, add:%lx, pgd:%p %llx, pmd:%p %llx, pte:%p %llx\n",
			 __func__, addr, pgd, pgd_val(*pgd),
			 pmd, pmd_val(*pmd), pte, pte_val(*pte));
	#endif
		page++;
	}
	return 0;
}

void check_page_to_cma(struct compact_control *cc,
		       struct address_space *mapping,
		       struct page *page)
{
	/* no need check once it is true */
	if (test_bit(FORBID_TO_CMA_BIT, &cc->total_migrate_scanned))
		return;

	if ((unsigned long)mapping & PAGE_MAPPING_ANON)
		mapping = NULL;

#ifdef CONFIG_KSM
	if ((((unsigned long)page->mapping & PAGE_MAPPING_FLAGS) == PAGE_MAPPING_KSM) &&
	    !PageSlab(page))
		__set_bit(FORBID_TO_CMA_BIT, &cc->total_migrate_scanned);
#endif

	if (mapping && cma_forbidden_mask(mapping_gfp_mask(mapping)))
		__set_bit(FORBID_TO_CMA_BIT, &cc->total_migrate_scanned);
}

static int can_migrate_to_cma(struct page *page)
{
	struct address_space *mapping;

	mapping = page_mapping(page);
	if ((unsigned long)mapping & PAGE_MAPPING_ANON)
		mapping = NULL;

	if (PageKsm(page) && !PageSlab(page))
		return 0;

	if (mapping && cma_forbidden_mask(mapping_gfp_mask(mapping)))
		return 0;

	return 1;
}

struct page *get_compact_page(struct page *migratepage,
				struct compact_control *cc)
{
	int can_to_cma, find = 0;
	struct page *page, *next;

	can_to_cma = can_migrate_to_cma(migratepage);
	if (!can_to_cma) {
		list_for_each_entry_safe(page, next, &cc->freepages, lru) {
			if (!cma_page(page)) {
				list_del(&page->lru);
				cc->nr_freepages--;
				find = 1;
				break;
			}
		}
		if (!find)
			return NULL;
	} else {
		page = list_entry(cc->freepages.next, struct page, lru);
		list_del(&page->lru);
		cc->nr_freepages--;
	}
	return page;
}
#endif

/* cma alloc/free interface */

static unsigned long pfn_max_align_down(unsigned long pfn)
{
	return pfn & ~(max_t(unsigned long, MAX_ORDER_NR_PAGES,
			     pageblock_nr_pages) - 1);
}

static unsigned long aml_pfn_max_align_up(unsigned long pfn)
{
	return ALIGN(pfn, max_t(unsigned long, MAX_ORDER_NR_PAGES,
				pageblock_nr_pages));
}

static struct page *get_migrate_page(struct page *page, unsigned long private)
{
	struct migration_target_control *mtc;
	gfp_t gfp_mask;
	unsigned int order = 0;
	struct page *new_page = NULL;
	int nid;
	int zidx;

	mtc = (struct migration_target_control *)private;
#if IS_BUILTIN(CONFIG_AMLOGIC_CMA)
	gfp_mask = mtc->gfp_mask | __GFP_NO_CMA;
#else
	gfp_mask = mtc->gfp_mask;
#endif
	nid = mtc->nid;
	if (nid == NUMA_NO_NODE)
		nid = page_to_nid(page);

	/*
	 * TODO: allocate a destination hugepage from a nearest neighbor node,
	 * accordance with memory policy of the user process if possible. For
	 * now as a simple work-around, we use the next node for destination.
	 */
	if (PageHuge(page)) {
		struct hstate *h = page_hstate(compound_head(page));

		gfp_mask |= htlb_modify_alloc_mask(h, gfp_mask);
		new_page = alloc_huge_page_nodemask(h,
					       page_to_nid(page),
					       0, gfp_mask);
	#if (IS_MODULE(CONFIG_AMLOGIC_PAGE_TRACE) && IS_MODULE(CONFIG_AMLOGIC_CMA)) || \
		(IS_BUILTIN(CONFIG_AMLOGIC_PAGE_TRACE) && IS_BUILTIN(CONFIG_AMLOGIC_CMA))
	#ifdef CONFIG_HUGETLB_PAGE
		replace_page_trace(new_page, page);
	#endif
	#endif
		return new_page;
	}

	if (PageTransHuge(page)) {
		/*
		 * clear __GFP_RECLAIM to make the migration callback
		 * consistent with regular THP allocations.
		 */
		gfp_mask &= ~__GFP_RECLAIM;
		gfp_mask |= GFP_TRANSHUGE;
		order = HPAGE_PMD_ORDER;
	}
	zidx = zone_idx(page_zone(page));
	if (is_highmem_idx(zidx) || zidx == ZONE_MOVABLE)
		gfp_mask |= __GFP_HIGHMEM;

	new_page = __alloc_pages(gfp_mask, order, nid, mtc->nmask);

	if (new_page && PageTransHuge(new_page))
#if IS_BUILTIN(CONFIG_AMLOGIC_CMA)
		prep_transhuge_page(new_page);
#else
		aml_prep_huge_page(new_page);
#endif

#if (IS_MODULE(CONFIG_AMLOGIC_PAGE_TRACE) && IS_MODULE(CONFIG_AMLOGIC_CMA)) || \
	(IS_BUILTIN(CONFIG_AMLOGIC_PAGE_TRACE) && IS_BUILTIN(CONFIG_AMLOGIC_CMA))
	replace_page_trace(new_page, page);
#endif
	return new_page;
}

#if defined(CONFIG_DYNAMIC_DEBUG) || \
	(defined(CONFIG_DYNAMIC_DEBUG_CORE) && defined(DYNAMIC_DEBUG_MODULE))
/* Usage: See admin-guide/dynamic-debug-howto.rst */
static void alloc_contig_dump_pages(struct list_head *page_list)
{
	DEFINE_DYNAMIC_DEBUG_METADATA(descriptor, "migrate failure");

	if (DYNAMIC_DEBUG_BRANCH(descriptor)) {
		struct page *page;

		dump_stack();
		list_for_each_entry(page, page_list, lru)
			dump_page(page, "migration failure");
	}
}
#else
static inline void alloc_contig_dump_pages(struct list_head *page_list)
{
}
#endif

/* [start, end) must belong to a single zone. */
static int __nocfi aml_alloc_contig_migrate_range(struct compact_control *cc,
					  unsigned long start,
					  unsigned long end, bool boost,
					  struct task_struct *host)
{
	/* This function is based on compact_zone() from compaction.c. */
	unsigned long nr_reclaimed;
	unsigned long pfn = start;
	unsigned int tries = 0;
	int ret = 0;
	struct migration_target_control mtc = {
		.nid = zone_to_nid(cc->zone),
		.gfp_mask = GFP_USER | __GFP_MOVABLE | __GFP_RETRY_MAYFAIL,
	};

	while (pfn < end || !list_empty(&cc->migratepages)) {
		if (fatal_signal_pending(host)) {
			ret = -EINTR;
			break;
		}

		if (list_empty(&cc->migratepages)) {
			cc->nr_migratepages = 0;
		#if IS_BUILTIN(CONFIG_AMLOGIC_CMA)
			ret = isolate_migratepages_range(cc, pfn, end);
		#else
			ret = aml_isolate_pages_range(cc, pfn, end);
		#endif
			if (ret && ret != -EAGAIN) {
				cma_debug(1, NULL, " iso migrate page fail, ret:%d\n",
					ret);
				break;
			}
			pfn = cc->migrate_pfn;
			tries = 0;
		} else if (++tries == 5) {
			ret = -EBUSY;
			break;
		}

	#if IS_BUILTIN(CONFIG_AMLOGIC_CMA)
		nr_reclaimed = reclaim_clean_pages_from_list(cc->zone,
							     &cc->migratepages);
	#else
		nr_reclaimed = aml_reclaim_clean_pages_from_list(cc->zone,
							     &cc->migratepages);
	#endif
		cc->nr_migratepages -= nr_reclaimed;

		ret = migrate_pages(&cc->migratepages, get_migrate_page,
			NULL, (unsigned long)&mtc, cc->mode, MR_CONTIG_RANGE, NULL);

		/*
		 * On -ENOMEM, migrate_pages() bails out right away. It is pointless
		 * to retry again over this error, so do the same here.
		 */
		if (ret == -ENOMEM)
			break;
	}

	if (ret < 0) {
		if (ret == -EBUSY) {
			struct page *page;

			alloc_contig_dump_pages(&cc->migratepages);
			list_for_each_entry(page, &cc->migratepages, lru) {
				/* The page will be freed by putback_movable_pages soon */
				if (page_count(page) == 1)
					continue;
			#if IS_BUILTIN(CONFIG_AMLOGIC_CMA)
				page_pinner_failure_detect(page);
			#else
				aml_page_pinner_failure_detect(page);
			#endif
			}
		}
		putback_movable_pages(&cc->migratepages);
		return ret;
	}
	return 0;
}

static int __nocfi cma_boost_work_func(void *cma_data)
{
	struct cma_pcp *c_work;
	struct work_cma *job;
	unsigned long pfn, end;
	int ret = -1;
	int this_cpu;
	struct compact_control cc = {
		.nr_migratepages = 0,
		.order = -1,
		.mode = MIGRATE_SYNC,
		.ignore_skip_hint = true,
		.no_set_skip_hint = true,
		.gfp_mask = GFP_KERNEL,
		.alloc_contig = true,
	};

	c_work  = (struct cma_pcp *)cma_data;
	for (;;) {
		ret = wait_for_completion_interruptible(&c_work->start);
		if (ret < 0) {
			pr_err("%s wait for task %d is %d\n",
			       __func__, c_work->cpu, ret);
			continue;
		}
		this_cpu = get_cpu();
		put_cpu();
		if (this_cpu != c_work->cpu) {
			pr_err("%s, cpu %d is not work cpu:%d\n",
			       __func__, this_cpu, c_work->cpu);
		}
again:
		spin_lock(&work_list_lock);
		if (list_empty(&work_list)) {
			/* NO job todo ? */
			spin_unlock(&work_list_lock);
			cma_debug(1, NULL, "%s,%d, list empty\n", __func__, __LINE__);
			goto next;
		}
		job = list_first_entry(&work_list, struct work_cma, list);
		list_del(&job->list);
		spin_unlock(&work_list_lock);

		INIT_LIST_HEAD(&cc.migratepages);
		pfn      = job->pfn;
		cc.zone  = page_zone(pfn_to_page(pfn));
		end      = pfn + job->count;
		ret      = aml_alloc_contig_migrate_range(&cc, pfn, end,
							  1, job->host);
		job->ret = ret;
		if (!ret) {
			goto again;
		} else if (ret == -EBUSY) {
			spin_lock(&work_list_lock);
			job->ret = 0;
			list_add(&job->list, &work_list);
			spin_unlock(&work_list_lock);
			cma_debug(1, pfn_to_page(pfn), "contig migrate ebusy\n");
			goto again;
		} else {
			pr_err("cma alloc contig failed, ret:%d\n", ret);
		}
next:
		complete(&c_work->end);
		if (kthread_should_stop()) {
			pr_err("%s task exit\n", __func__);
			break;
		}
	}
	return 0;
}

#ifdef CONFIG_ANDROID_VENDOR_HOOKS
static int cma_enabled_setup(char *str)
{
	if (kstrtoint(str, 0, &cma_enabled)) {
		pr_err("cma_enabled: bad arg:%s\n", str);
		return -EINVAL;
	}

	return 0;
}
__setup("cma_enabled=", cma_enabled_setup);

static int cma_enabled_swap_ratio = 70;
module_param(cma_enabled_swap_ratio, int, 0644);

static int cma_enabled_swap_ratio_setup(char *str)
{
	if (kstrtoint(str, 0, &cma_enabled_swap_ratio)) {
		pr_err("cma_enabled_swap_ratio: bad arg:%s\n", str);
		return -EINVAL;
	}

	return 0;
}
__setup("cma_enabled_swap_ratio=", cma_enabled_swap_ratio_setup);

static struct delayed_work cma_enabled_work;

static void cma_enabled_work_fn(struct work_struct *work)
{
	struct sysinfo i;

	if (cma_enabled)
		return;

	si_swapinfo(&i);

	if (i.totalswap && i.freeswap * 100 < i.totalswap * cma_enabled_swap_ratio) {
		pr_info("swap free low(MB):%lu/%lu, ratio=%d, enable cma now!\n",
				i.freeswap * 4 / 1024,
				i.totalswap * 4 / 1024,
				cma_enabled_swap_ratio);
		cma_enabled = 1;
	} else {
		schedule_delayed_work(&cma_enabled_work, HZ / 10);
	}
}

#if CONFIG_AMLOGIC_KERNEL_VERSION >= 14515
static void __maybe_unused delay_enable_cma_hook(void *data,
		gfp_t gfp_mask, unsigned int *alloc_flags, bool *bypass)
{
	if (!cma_enabled)
		*bypass = 1;
}
#else
static void __maybe_unused delay_enable_cma_hook(void *data,
		gfp_t gfp_mask, unsigned int *alloc_flags)
{
	if (!cma_enabled)
		*alloc_flags &= ~ALLOC_CMA;
}
#endif

static void delay_enable_cma_init(void)
{
#if CONFIG_AMLOGIC_KERNEL_VERSION >= 14515
	register_trace_android_vh_calc_alloc_flags(delay_enable_cma_hook, NULL);
#else

	register_trace_android_vh_alloc_flags_cma_adjust(delay_enable_cma_hook, NULL);
#endif

	INIT_DELAYED_WORK(&cma_enabled_work, cma_enabled_work_fn);
	schedule_delayed_work(&cma_enabled_work, HZ);
}
#endif

static int __init init_cma_boost_task(void)
{
	int cpu;
	struct task_struct *task;
	struct cma_pcp *work;
	char task_name[20] = {};
	INIT_LIST_HEAD(&work_list);
	spin_lock_init(&work_list_lock);

	for_each_possible_cpu(cpu) {
		memset(task_name, 0, sizeof(task_name));
		sprintf(task_name, "cma_task%d", cpu);
		work = &per_cpu(cma_pcp_thread, cpu);
		init_completion(&work->start);
		init_completion(&work->end);
		work->cpu = cpu;
		task = kthread_create(cma_boost_work_func, work, task_name);
		if (!IS_ERR(task)) {
			kthread_bind(task, cpu);
			set_user_nice(task, -17);
			work->task = task;
			pr_debug("create cma task%p, for cpu %d\n", task, cpu);
			wake_up_process(task);
		} else {
			can_boost = 0;
			pr_err("create task for cpu %d fail:%p\n", cpu, task);
			return -1;
		}
	}
	can_boost = 1;

#ifdef CONFIG_ANDROID_VENDOR_HOOKS
	delay_enable_cma_init();
#endif

	return 0;
}

#if IS_BUILTIN(CONFIG_AMLOGIC_CMA)
module_init(init_cma_boost_task);
#endif

int cma_alloc_contig_boost(unsigned long start_pfn, unsigned long count)
{
	struct cpumask has_work;
	int cpu, cpus, i = 0, ret = 0, ebusy = 0, einv = 0;
	atomic_t ok;
	unsigned long delta;
	unsigned long cnt;
	unsigned long flags;
	struct cma_pcp *work;
	struct work_cma job[MAX_JOB_NUM] = {};

	cpumask_clear(&has_work);

	if (allow_cma_tasks)
		cpus = allow_cma_tasks;
	else
		cpus = num_online_cpus() - 1;

	/* cnt   = count; */
	if (count < pageblock_nr_pages) {
		delta = count / cpus;
		cnt = cpus;
	} else if (count <= pageblock_nr_pages * 10) {
		delta = 256;
		cnt = count / delta;
	} else if (count <= pageblock_nr_pages * 20) {
		delta = 512;
		cnt = count / delta;
	} else {
		delta = count / MAX_JOB_NUM;
		cnt = MAX_JOB_NUM;
	}
	if (cnt > MAX_JOB_NUM) {
		pr_err("cnt too large: %ld, delta: %ld, count: %ld\n", cnt, delta, count);
		return -ENOMEM;
	}
	spin_lock(&work_list_lock);
	if (!list_empty(&work_list))
		INIT_LIST_HEAD(&work_list);
	for (i = 0; i < cnt; i++) {
		INIT_LIST_HEAD(&job[i].list);
		job[i].pfn   = start_pfn + i * delta;
		job[i].count = delta;
		job[i].ret   = 0;
		job[i].host  = current;
		if (i == cnt - 1)
			job[i].count = count - i * delta;
		list_add(&job[i].list, &work_list);
	}
	spin_unlock(&work_list_lock);
	i = 0;

	atomic_set(&ok, 0);
	local_irq_save(flags);
	for_each_online_cpu(cpu) {
		work = &per_cpu(cma_pcp_thread, cpu);
		cpumask_set_cpu(cpu, &has_work);
		complete(&work->start);
		i++;
		if (i == cpus) {
			cma_debug(1, NULL, "sched work to %d cpu\n", i);
			break;
		}
	}
	local_irq_restore(flags);

	for_each_cpu(cpu, &has_work) {
		work = &per_cpu(cma_pcp_thread, cpu);
		wait_for_completion(&work->end);
		if (job[cpu].ret) {
			if (job[cpu].ret != -EBUSY)
				einv++;
			else
				ebusy++;
		}
	}

	if (einv)
		ret = -EINVAL;
	else if (ebusy)
		ret = -EBUSY;
	else
		ret = 0;

	if (ret < 0 && ret != -EBUSY) {
		pr_err("%s, failed, ret:%d, ok:%d\n",
		       __func__, ret, atomic_read(&ok));
	}

	return ret;
}

static int __nocfi __aml_check_pageblock_isolate(unsigned long pfn,
					 unsigned long end_pfn,
					 int flags)
{
	struct page *page;

	while (pfn < end_pfn) {
		page = pfn_to_page(pfn);
		if (PageBuddy(page)) {
			/*
			 * If the page is on a free list, it has to be on
			 * the correct MIGRATE_ISOLATE freelist. There is no
			 * simple way to verify that as VM_BUG_ON(), though.
			 */
			pfn += 1 << buddy_order(page);
		} else if ((flags & MEMORY_OFFLINE) && PageHWPoison(page)) {
			/* A HWPoisoned page cannot be also PageBuddy */
			pfn++;
		} else if ((flags & MEMORY_OFFLINE) && PageOffline(page) &&
			 !page_count(page)) {
			/*
			 * The responsible driver agreed to skip PageOffline()
			 * pages when offlining memory by dropping its
			 * reference in MEM_GOING_OFFLINE.
			 */
			pfn++;
		} else {
			cma_debug(1, page, " isolate failed\n");
			aml__dump_owner(page);
			break;
		}
	}

	return pfn;
}

#if IS_MODULE(CONFIG_AMLOGIC_CMA)
#define aml_pfn_to_online_page(pfn)			\
({						\
	struct page *___page = NULL;		\
	if (pfn_valid(pfn))			\
		___page = pfn_to_page(pfn);	\
	___page;				\
 })
#endif

static inline struct page * __nocfi
check_page_valid(unsigned long pfn, unsigned long nr_pages)
{
	int i;

	for (i = 0; i < nr_pages; i++) {
		struct page *page;

	#if IS_BUILTIN(CONFIG_AMLOGIC_CMA)
		page = pfn_to_online_page(pfn + i);
	#else
		page = aml_pfn_to_online_page(pfn + i);
	#endif
		if (!page)
			continue;
		return page;
	}
	return NULL;
}

int __nocfi aml_check_pages_isolated(unsigned long start_pfn, unsigned long end_pfn,
			     int isol_flags)
{
	unsigned long pfn, flags;
	struct page *page;
	struct zone *zone;
	int ret;

	/*
	 * Note: pageblock_nr_pages != MAX_ORDER. Then, chunks of free pages
	 * are not aligned to pageblock_nr_pages.
	 * Then we just check migratetype first.
	 */
	for (pfn = start_pfn; pfn < end_pfn; pfn += pageblock_nr_pages) {
		page = check_page_valid(pfn, pageblock_nr_pages);
		if (page && !is_migrate_isolate_page(page))
			break;
	}
	page = check_page_valid(start_pfn, end_pfn - start_pfn);
	if (pfn < end_pfn || !page) {
		ret = -EBUSY;
		goto out;
	}
	/* Check all pages are free or marked as ISOLATED */
	zone = page_zone(page);
	spin_lock_irqsave(&zone->lock, flags);
	pfn = __aml_check_pageblock_isolate(start_pfn, end_pfn, isol_flags);
	spin_unlock_irqrestore(&zone->lock, flags);

	ret = pfn < end_pfn ? -EBUSY : 0;

out:
#if IS_BUILTIN(CONFIG_AMLOGIC_CMA)
	trace_test_pages_isolated(start_pfn, end_pfn, pfn);
#endif
	if (pfn < end_pfn)
	#if IS_BUILTIN(CONFIG_AMLOGIC_CMA)
		page_pinner_failure_detect(pfn_to_page(pfn));
	#else
		aml_page_pinner_failure_detect(pfn_to_page(pfn));
	#endif

	return ret;
}

static unsigned long cur_alloc_start;
static unsigned long cur_alloc_end;

int in_cma_allocating(struct page *page)
{
	unsigned long pfn;

	if (!page)
		return 0;
	pfn = page_to_pfn(page);
	if (pfn >= cur_alloc_start && pfn <= cur_alloc_end)
		return 1;

	return 0;
}

int __nocfi aml_cma_alloc_range(unsigned long start, unsigned long end,
			unsigned int migrate_type, gfp_t gfp_mask)
{
	unsigned long outer_start, outer_end;
	int ret = 0, order;
	int try_times = 0;
	int boost_ok = 0;
#if CONFIG_AMLOGIC_KERNEL_VERSION >= 14515
	unsigned long failed_pfn;
#endif

	struct compact_control cc = {
		.nr_migratepages = 0,
		.order = -1,
		.zone = page_zone(pfn_to_page(start)),
		.mode = MIGRATE_SYNC,
		.ignore_skip_hint = true,
		.no_set_skip_hint = true,
		.gfp_mask = current_gfp_context(gfp_mask),
		.alloc_contig = true,
		.contended = false,
	};
	INIT_LIST_HEAD(&cc.migratepages);

	cma_debug(0, NULL, " range [%lx-%lx]\n", start, end);
#if IS_BUILTIN(CONFIG_AMLOGIC_CMA)
	mutex_lock(&cma_mutex);
#if CONFIG_AMLOGIC_KERNEL_VERSION >= 14515
	ret = start_isolate_page_range(pfn_max_align_down(start),
				       aml_pfn_max_align_up(end), migrate_type,
				       0, &failed_pfn);
#else
	ret = start_isolate_page_range(pfn_max_align_down(start),
				       aml_pfn_max_align_up(end), migrate_type, 0);
#endif
#else
#if CONFIG_AMLOGIC_KERNEL_VERSION >= 14515
	ret = aml_start_isolate_page_range(pfn_max_align_down(start),
				       aml_pfn_max_align_up(end), migrate_type,
				       0, &failed_pfn);
#else
	ret = aml_start_isolate_page_range(pfn_max_align_down(start),
				       aml_pfn_max_align_up(end), migrate_type, 0);
#endif
#endif
	if (ret < 0) {
		cma_debug(1, NULL, "ret:%d\n", ret);
		return ret;
	}

	cur_alloc_start = start;
	cur_alloc_end = end;
	cma_isolated += (aml_pfn_max_align_up(end) - pfn_max_align_down(start));
try_again:
#if IS_BUILTIN(CONFIG_AMLOGIC_CMA)
	lru_add_drain_all();
	drain_all_pages(cc.zone);
#else
	aml_lru_add_drain_all();
	aml_drain_all_pages(cc.zone);
#endif
	/*
	 * try to use more cpu to do this job when alloc count is large
	 */
	cpus_read_lock();
	if ((num_online_cpus() > 1) && can_boost &&
	    ((end - start) >= pageblock_nr_pages / 2)) {
		ret = cma_alloc_contig_boost(start, end - start);
		boost_ok = !ret ? 1 : 0;
	} else {
		ret = aml_alloc_contig_migrate_range(&cc, start,
						     end, 0, current);
	}
	cpus_read_unlock();

	if (ret && ret != -EBUSY) {
		cma_debug(1, NULL, "ret:%d\n", ret);
		goto done;
	}

	ret = 0;
	order = 0;
	outer_start = start;
	while (!PageBuddy(pfn_to_page(outer_start))) {
		if (++order >= MAX_ORDER) {
			outer_start = start;
			break;
		}
		outer_start &= ~0UL << order;
	}

	if (outer_start != start) {
		order = buddy_order(pfn_to_page(outer_start));

		/*
		 * outer_start page could be small order buddy page and
		 * it doesn't include start page. Adjust outer_start
		 * in this case to report failed page properly
		 * on tracepoint in test_pages_isolated()
		 */
		if (outer_start + (1UL << order) <= start)
			outer_start = start;
	}

	/* Make sure the range is really isolated. */
	if (aml_check_pages_isolated(outer_start, end, false)) {
		cma_debug(1, NULL, "check page isolate(%lx, %lx) failed\n",
			  outer_start, end);
		try_times++;
		if (try_times < 10)
			goto try_again;
		ret = -EBUSY;
		goto done;
	}

	/* Grab isolated pages from freelists. */
#if IS_BUILTIN(CONFIG_AMLOGIC_CMA)
	outer_end = isolate_freepages_range(&cc, outer_start, end);
#else
	outer_end = aml_iso_free_range(&cc, outer_start, end);
#endif
	if (!outer_end) {
		if (cc.contended) {
			ret = -EINTR;
			pr_info("cma_alloc [%lx-%lx] aborted\n", start, end);
		} else {
			ret = -EBUSY;
		}
		cma_debug(1, NULL, "iso free range(%lx, %lx) failed\n",
			  outer_start, end);
		goto done;
	}

	/* Free head and tail (if any) */
	if (start != outer_start)
		aml_cma_free(outer_start, start - outer_start, 0);
	if (end != outer_end)
		aml_cma_free(end, outer_end - end, 0);

done:
#if IS_BUILTIN(CONFIG_AMLOGIC_CMA)
	undo_isolate_page_range(pfn_max_align_down(start),
				aml_pfn_max_align_up(end), migrate_type);
#else
	aml_undo_isolate_page_range(pfn_max_align_down(start),
				aml_pfn_max_align_up(end), migrate_type);
#endif
	cma_isolated -= (aml_pfn_max_align_up(end) - pfn_max_align_down(start));
	cur_alloc_start = 0;
	cur_alloc_end = 0;
#if IS_BUILTIN(CONFIG_AMLOGIC_CMA)
	mutex_unlock(&cma_mutex);
#endif

	return ret;
}

static int __aml_cma_free_check(struct page *page, int order, unsigned int *cnt)
{
	int i;
	int ref = 0;

	/*
	 * clear ref count, head page should avoid this operation.
	 * ref count of head page will be cleared when __free_pages
	 * is called.
	 */
	for (i = 1; i < (1 << order); i++) {
		if (!put_page_testzero(page + i))
			ref++;
	}
	if (ref) {
		pr_info("%s, %d pages are still in use\n", __func__, ref);
		*cnt += ref;
		return -1;
	}
	return 0;
}

static int aml_cma_get_page_order(unsigned long pfn)
{
	int i, mask = 1;

	for (i = 0; i < (MAX_ORDER - 1); i++) {
		if (pfn & (mask << i))
			break;
	}

	return i;
}

void aml_cma_free(unsigned long pfn, unsigned int nr_pages, int update)
{
	unsigned int count = 0;
	struct page *page;
	int free_order, start_order = 0;
	int batch;
	unsigned int orig_nr_pages = nr_pages;
#if (IS_MODULE(CONFIG_AMLOGIC_PAGE_TRACE) && IS_MODULE(CONFIG_AMLOGIC_CMA)) || \
	(IS_BUILTIN(CONFIG_AMLOGIC_PAGE_TRACE) && IS_BUILTIN(CONFIG_AMLOGIC_CMA))
	unsigned long orig_pfn = pfn;
#endif

	while (nr_pages) {
		page = pfn_to_page(pfn);
		free_order = aml_cma_get_page_order(pfn);
		if (nr_pages >= (1 << free_order)) {
			start_order = free_order;
		} else {
			/* remain pages is not enough */
			start_order = 0;
			while (nr_pages >= (1 << start_order))
				start_order++;
			start_order--;
		}
		batch = (1 << start_order);
		if (__aml_cma_free_check(page, start_order, &count))
			break;
		__free_pages(page, start_order);
		pr_debug("pages:%4d, free:%2d, start:%2d, batch:%4d, pfn:%lx\n",
			 nr_pages, free_order,
			 start_order, batch, pfn);
		nr_pages -= batch;
		pfn += batch;
	}
	WARN(count != 0, "%d pages are still in use!\n", count);
	if (update) {
	#if (IS_MODULE(CONFIG_AMLOGIC_PAGE_TRACE) && IS_MODULE(CONFIG_AMLOGIC_CMA)) || \
		(IS_BUILTIN(CONFIG_AMLOGIC_PAGE_TRACE) && IS_BUILTIN(CONFIG_AMLOGIC_CMA))
		if (cma_alloc_trace)
			pr_info("c f p:%lx, c:%d, f:%ps\n",
				orig_pfn, orig_nr_pages, (void *)find_back_trace());
	#endif /* CONFIG_AMLOGIC_PAGE_TRACE */
		atomic_long_sub(orig_nr_pages, &nr_cma_allocated);
	}
}

static bool cma_vma_show(struct page *page, struct vm_area_struct *vma,
			 unsigned long addr, void *arg)
{
#if defined(CONFIG_AMLOGIC_USER_FAULT) && IS_BUILTIN(CONFIG_AMLOGIC_CMA)
	struct mm_struct *mm = vma->vm_mm;

	show_vma(mm, addr);
#endif
	return false; /* keep loop */
}

void rmap_walk_vma(struct page *page)
{
	struct rmap_walk_control rwc = {
		.rmap_one = cma_vma_show,
	};

	pr_info("%s, show map for page:%lx,f:%lx, m:%px, p:%d\n",
		__func__, page_to_pfn(page), page->flags,
		page->mapping, page_count(page));
	if (!page_mapping(page))
		return;
#if IS_BUILTIN(CONFIG_AMLOGIC_CMA)
	rmap_walk(page, &rwc);
#else
	aml_rmap_walk(page, &rwc);
#endif
}

void __nocfi show_page(struct page *page)
{
	unsigned long trace = 0;
	unsigned long map_flag = -1UL;

	if (!page)
		return;
#if (IS_MODULE(CONFIG_AMLOGIC_PAGE_TRACE) && IS_MODULE(CONFIG_AMLOGIC_CMA)) || \
	(IS_BUILTIN(CONFIG_AMLOGIC_PAGE_TRACE) && IS_BUILTIN(CONFIG_AMLOGIC_CMA))
	trace = get_page_trace(page);
#endif
	if (page->mapping && !((unsigned long)page->mapping & 0x3))
		map_flag = page->mapping->flags;
	pr_info("page:%lx, map:%lx, mf:%lx, pf:%lx, m:%d, c:%d, o:%lx, pt:%lx, f:%ps\n",
		page_to_pfn(page), (unsigned long)page->mapping, map_flag,
		page->flags & 0xffffffff,
		page_mapcount(page), page_count(page), page->private, page->index,
		(void *)trace);
	aml__dump_owner(page);
	if (cma_debug_level > 4 && !irqs_disabled())
		rmap_walk_vma(page);
}

static int cma_debug_show(struct seq_file *m, void *arg)
{
	seq_printf(m, "level=%d, alloc trace:%d, allow task:%d\n",
		   cma_debug_level, cma_alloc_trace, allow_cma_tasks);
#if IS_BUILTIN(CONFIG_AMLOGIC_CMA)
	seq_printf(m, "driver used:%lu isolated:%d total:%lu\n",
		   get_cma_allocated(), 0, totalcma_pages);
#else
	seq_printf(m, "driver used:%lu isolated:%d total:%lu\n",
		   get_cma_allocated(), 0, aml_totalcma_pages);
#endif
	return 0;
}

static ssize_t cma_debug_write(struct file *file, const char __user *buffer,
			       size_t count, loff_t *ppos)
{
	int arg = 0;
	int ok = 0;
	int cpu;
	struct cma_pcp *work;
	char *buf;

	buf = kzalloc(count + 1, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	if (copy_from_user(buf, buffer, count))
		goto exit;

	if (!strncmp(buf, "cma_task=", 9)) {	/* option for 'cma_task=' */
		if (sscanf(buf, "cma_task=%d", &arg) < 0)
			goto exit;
		if (arg <= num_online_cpus() && arg >= 1) {
			ok = 1;
			allow_cma_tasks = arg;
			pr_info("set allow_cma_tasks to %d\n", allow_cma_tasks);
		}
		goto exit;
	}

	if (!strncmp(buf, "cma_prio=", 9)) {	/* option for 'cma_prio=' */
		if (sscanf(buf, "cma_prio=%d", &arg) < 0)
			goto exit;
		if (arg >= MIN_NICE && arg < MAX_NICE) {
			for_each_possible_cpu(cpu) {
				work = &per_cpu(cma_pcp_thread, cpu);
				set_user_nice(work->task, arg);
			}
			ok = 1;
			pr_info("renice cma task to %d\n", arg);
		}
		goto exit;
	}

	if (!strncmp(buf, "cma_trace=", 9)) {	/* option for 'cma_trace=' */
		if (sscanf(buf, "cma_trace=%d", &arg) < 0)
			goto exit;

		cma_alloc_trace = arg ? 1 : 0;
		ok = 1;
		goto exit;
	}

	if (!kstrtoint(buf, 10, &arg)) {
		if (arg > MAX_DEBUG_LEVEL)
			goto exit;

		ok = 1;
		cma_debug_level = arg;
		goto exit;
	}
exit:
	kfree(buf);
	if (ok)
		return count;
	else
		return -EINVAL;
}

static int cma_debug_open(struct inode *inode, struct file *file)
{
	return single_open(file, cma_debug_show, NULL);
}

static const struct proc_ops cma_dbg_file_ops = {
	.proc_open	= cma_debug_open,
	.proc_read	= seq_read,
	.proc_lseek	= seq_lseek,
	.proc_write	= cma_debug_write,
	.proc_release	= single_release,
};

#if IS_BUILTIN(CONFIG_AMLOGIC_CMA)
static int __init aml_cma_init(void)
{
	atomic_set(&cma_allocate, 0);
	atomic_long_set(&nr_cma_allocated, 0);

	dentry = proc_create("cma_debug", 0644, NULL, &cma_dbg_file_ops);
	if (IS_ERR_OR_NULL(dentry)) {
		pr_err("%s, create sysfs failed\n", __func__);
		return -1;
	}
	return 0;
}
arch_initcall(aml_cma_init);
#else
/**
 * cma_alloc() - allocate pages from contiguous area
 * @cma:   Contiguous memory region for which the allocation is performed.
 * @count: Requested number of pages.
 * @align: Requested alignment of pages (in PAGE_SIZE order).
 * @no_warn: Avoid printing message about failed allocation
 *
 * This function allocates part of contiguous memory on specific
 * contiguous memory area.
 */
struct page *aml_cma_alloc(struct dummy_cma *cma, unsigned long count,
		       unsigned int align, bool no_warn)
{
	unsigned long mask, offset;
	unsigned long pfn = -1;
	unsigned long start = 0;
	unsigned long bitmap_maxno, bitmap_no, bitmap_count;
	unsigned long i;
	struct page *page = NULL;
	int ret = -ENOMEM;
	int num_attempts = 0;
	int max_retries = 5;
	int dummy;
	unsigned long tick = 0;
	unsigned long long in_tick, timeout;

	in_tick = sched_clock();

	if (!cma || !cma->count || !cma->bitmap)
		goto out;

	cma_debug(0, NULL, "(cma %p, count %lu, align %d)\n",
		  (void *)cma, count, align);
	in_tick = sched_clock();
	timeout = 2ULL * 1000000 * (1 + ((count * PAGE_SIZE) >> 20));

	if (!count)
		goto out;

	//trace_cma_alloc_start(cma->name, count, align);

	mask = cma_bitmap_aligned_mask(cma, align);
	offset = cma_bitmap_aligned_offset(cma, align);
	bitmap_maxno = cma_bitmap_maxno(cma);
	bitmap_count = cma_bitmap_pages_to_bits(cma, count);

	if (bitmap_count > bitmap_maxno)
		goto out;

	aml_cma_alloc_pre_hook(&dummy, count, &tick);
	//trace_android_vh_cma_alloc_retry(cma->name, &max_retries);
	for (;;) {
		spin_lock_irq(&cma->lock);
		bitmap_no = bitmap_find_next_zero_area_off(cma->bitmap,
				bitmap_maxno, start, bitmap_count, mask,
				offset);
		if (bitmap_no >= bitmap_maxno) {
			if ((num_attempts < max_retries) && (ret == -EBUSY)) {
				spin_unlock_irq(&cma->lock);

				if (fatal_signal_pending(current)) {
					ret = -EINTR;
					break;
				}

				/*
				 * Page may be momentarily pinned by some other
				 * process which has been scheduled out, e.g.
				 * in exit path, during unmap call, or process
				 * fork and so cannot be freed there. Sleep
				 * for 100ms and retry the allocation.
				 */
				start = 0;
				ret = -ENOMEM;
				schedule_timeout_killable(msecs_to_jiffies(100));
				num_attempts++;
				continue;
			} else {
				spin_unlock_irq(&cma->lock);
				break;
			}
		}
		bitmap_set(cma->bitmap, bitmap_no, bitmap_count);
		/*
		 * It's safe to drop the lock here. We've marked this region for
		 * our exclusive use. If the migration fails we will take the
		 * lock again and unmark it.
		 */
		spin_unlock_irq(&cma->lock);

		pfn = cma->base_pfn + (bitmap_no << cma->order_per_bit);
		mutex_lock(&cma_mutex);
		ret = aml_cma_alloc_range(pfn, pfn + count, MIGRATE_CMA,
				     GFP_KERNEL | (no_warn ? __GFP_NOWARN : 0));
		mutex_unlock(&cma_mutex);
		if (ret == 0) {
			page = pfn_to_page(pfn);
			break;
		}

		cma_clear_bitmap(cma, pfn, count);
		if (ret != -EBUSY)
			break;

		cma_debug(0, NULL, "memory range at %p is busy, retrying\n",
				pfn_to_page(pfn));

		//trace_cma_alloc_busy_retry(cma->name, pfn, pfn_to_page(pfn),
		//			   count, align);
		/* try again with a bit different memory target */
		//start = bitmap_no + mask + 1;
		/*
		 * CMA allocation time out, for example:
		 * 1. set isolation failed.
		 * 2. refcout and mapcount mismatch.
		 * may blocked on some pages, relax CPU and try later.
		 */
		if ((sched_clock() - in_tick) >= timeout)
			usleep_range(1000, 2000);
	}

	//trace_cma_alloc_finish(cma->name, pfn, page, count, align);

	/*
	 * CMA can allocate multiple page blocks, which results in different
	 * blocks being marked with different tags. Reset the tags to ignore
	 * those page blocks.
	 */
	if (page) {
		for (i = 0; i < count; i++)
			page_kasan_tag_reset(page + i);
	}

	if (ret && !no_warn) {
		pr_err_ratelimited("%s: %s: alloc failed, req-size: %lu pages, ret: %d\n",
				   __func__, cma->name, count, ret);
		//cma_debug_show_areas(cma);
	}

	pr_debug("%s(): returned %p\n", __func__, page);
out:
	if (page) {
		count_vm_event(CMA_ALLOC_SUCCESS);
		cma_sysfs_account_success_pages(cma, count);
	} else {
		count_vm_event(CMA_ALLOC_FAIL);
		if (cma)
			cma_sysfs_account_fail_pages(cma, count);
	}
	aml_cma_alloc_post_hook(&dummy, count, page, tick, ret);

	return page;
}

/**
 * cma_release() - release allocated pages
 * @cma:   Contiguous memory region for which the allocation is performed.
 * @pages: Allocated pages.
 * @count: Number of allocated pages.
 *
 * This function releases memory allocated by cma_alloc().
 * It returns false when provided pages do not belong to contiguous area and
 * true otherwise.
 */
bool aml_cma_release(struct dummy_cma *cma, const struct page *pages,
		 unsigned long count)
{
	unsigned long pfn;

	if (!cma || !pages)
		return false;

	pr_debug("%s(page %p, count %lu)\n", __func__, (void *)pages, count);

	pfn = page_to_pfn(pages);

	if (pfn < cma->base_pfn || pfn >= cma->base_pfn + cma->count)
		return false;

	VM_BUG_ON(pfn + count > cma->base_pfn + cma->count);

	aml_cma_free(pfn, count, 1);
	cma_clear_bitmap(cma, pfn, count);
	//trace_cma_release(cma->name, pfn, pages, count);

	return true;
}

static int __nocfi __kprobes cma_alloc_pre_handler(struct kprobe *p, struct pt_regs *regs)
{
	//restore to origin context
	instruction_pointer_set(regs, (unsigned long)aml_cma_alloc);

	//no need continue do single-step
	return 1;
}

struct kprobe kp_cma_alloc = {
	.symbol_name  = "cma_alloc",
	.pre_handler = cma_alloc_pre_handler,
};

static int __nocfi __kprobes cma_release_pre_handler(struct kprobe *p, struct pt_regs *regs)
{
	//restore to origin context
	instruction_pointer_set(regs, (unsigned long)aml_cma_release);

	//no need continue do single-step
	return 1;
}

struct kprobe kp_cma_release = {
	.symbol_name  = "cma_release",
	.pre_handler = cma_release_pre_handler,
};

static void *get_symbol_addr(const char *symbol_name)
{
	struct kprobe kp = {
		.symbol_name = symbol_name,
	};
	int ret;

	ret = register_kprobe(&kp);
	if (ret < 0) {
		pr_err("register_kprobe:%s failed, returned %d\n", symbol_name, ret);
		return NULL;
	}
	pr_debug("symbol_name:%s addr=%px\n", symbol_name, kp.addr);
	unregister_kprobe(&kp);

	return kp.addr;
}

static void aml_bypass_cma_page(void *data,
		struct compact_control *cc, struct page *page, bool *bypass)
{
	int migrate_type = get_pageblock_migratetype(page);

	if (is_migrate_cma(migrate_type) ||
	    is_migrate_isolate(migrate_type))
		*bypass = 1;
}

static int __nocfi common_symbol_init(void *data)
{
	int ret;

	ret = register_kprobe(&kp_lookup_name);
	if (ret < 0) {
		pr_err("register_kprobe failed, returned %d\n", ret);
		return ret;
	}
	pr_debug("kprobe lookup offset at %px\n", kp_lookup_name.addr);

	aml_kallsyms_lookup_name = (unsigned long (*)(const char *name))kp_lookup_name.addr;

	aml_totalcma_pages = *(unsigned long *)aml_kallsyms_lookup_name("totalcma_pages");
	aml_prep_huge_page = (void (*)(struct page *page))get_symbol_addr("prep_transhuge_page");
	aml_reclaim_clean_pages_from_list = (unsigned int (*)(struct zone *zone,
		struct list_head *page_list))get_symbol_addr("reclaim_clean_pages_from_list");
	aml_isolate_pages_range = (int (*)(struct compact_control *cc, unsigned long start_pfn,
			unsigned long end_pfn))get_symbol_addr("isolate_migratepages_range");
	aml_rmap_walk = (void (*)(struct page *page,
			struct rmap_walk_control *rwc))get_symbol_addr("rmap_walk");
	aml_undo_isolate_page_range = (void (*)(unsigned long start_pfn, unsigned long end_pfn,
		unsigned int migratetype))get_symbol_addr("undo_isolate_page_range");
	aml_iso_free_range = (unsigned long (*)(struct compact_control *cc, unsigned long start_pfn,
		unsigned long end_pfn))get_symbol_addr("isolate_freepages_range");
	aml_drain_all_pages = (void (*)(struct zone *zone))get_symbol_addr("drain_all_pages");
	aml_lru_add_drain_all = (void (*)(void))get_symbol_addr("lru_add_drain_all");
#if CONFIG_AMLOGIC_KERNEL_VERSION >= 14515
	aml_start_isolate_page_range = (int (*)(unsigned long start_pfn, unsigned long end_pfn,
			unsigned int migratetype, int flags,
			unsigned long *failed_pfn))get_symbol_addr("start_isolate_page_range");
#else
	aml_start_isolate_page_range = (int (*)(unsigned long start_pfn, unsigned long end_pfn,
		unsigned int migratetype, int flags))get_symbol_addr("start_isolate_page_range");
#endif

#ifdef CONFIG_PAGE_OWNER
	aml__dump_owner = (void (*)(const struct page *page))get_symbol_addr("__dump_page_owner");
#endif
	ret = register_kprobe(&kp_cma_alloc);
	if (ret < 0) {
		pr_err("register_kprobe:%s failed, returned %d\n",
		       kp_cma_alloc.symbol_name, ret);
		return 1;
	}

	ret = register_kprobe(&kp_cma_release);
	if (ret < 0) {
		pr_err("register_kprobe:%s failed, returned %d\n",
		       kp_cma_release.symbol_name, ret);
		return 1;
	}

	ret = register_trace_android_vh_isolate_freepages(aml_bypass_cma_page, NULL);
	if (ret)
		pr_err("register_trace_android_vh_isolate_freepages fail ret=%d\n", ret);

	return 0;
}

static int __init aml_cma_module_init(void)
{
	atomic_set(&cma_allocate, 0);
	atomic_long_set(&nr_cma_allocated, 0);

	dentry = proc_create("cma_debug", 0644, NULL, &cma_dbg_file_ops);
	if (IS_ERR_OR_NULL(dentry)) {
		pr_err("%s, create sysfs failed\n", __func__);
		return -1;
	}

	init_cma_boost_task();
	kthread_run(common_symbol_init, NULL, "AML_CMA_TASK");

	return 0;
}

static void __exit aml_cma_module_exit(void)
{
}

module_init(aml_cma_module_init);
module_exit(aml_cma_module_exit);
MODULE_LICENSE("GPL v2");
MODULE_IMPORT_NS(MINIDUMP);
#endif

