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
#if defined(CONFIG_TRACEPOINTS) && defined(CONFIG_ANDROID_VENDOR_HOOKS)
#include <trace/hooks/iommu.h>
#endif

/* from mm/ path */
#include <internal.h>

#ifdef CONFIG_AMLOGIC_PAGE_TRACE
#include <linux/amlogic/page_trace.h>
#endif /* CONFIG_AMLOGIC_PAGE_TRACE */

#ifdef CONFIG_AMLOGIC_USER_FAULT
#include <linux/amlogic/user_fault.h>
#endif

#define MAX_DEBUG_LEVEL		5

struct work_cma {
	struct list_head list;
	unsigned long pfn;
	unsigned long count;
	struct task_struct *host;
	int ret;
};

struct cma_pcp {
	struct list_head list;
	struct completion start;
	struct completion end;
	struct task_struct *task;
	spinlock_t  list_lock;		/* protect job list */
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

static atomic_t cma_allocate;

#ifdef CONFIG_AMLOGIC_CMA_DIS
unsigned long ion_cma_allocated;
#endif
/*
 * We insert a none-mapping vm area to vmalloc space
 * and dynamic adjust it's size according nr_cma_allocated.
 * Just in order to let all driver allocated cma size counted
 * into KernelUsed item for dumpsys meminfo command on Android
 * layer
 */

static int cma_alloc_ref(void)
{
	return atomic_read(&cma_allocate);
}

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

#ifdef CONFIG_AMLOGIC_PAGE_TRACE
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
		set_page_trace(page, 0, __GFP_NO_CMA, (void *)fun);
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
#if defined(CONFIG_TRACEPOINTS) && defined(CONFIG_ANDROID_VENDOR_HOOKS) && defined(CONFIG_ARM64)
	trace_android_vh_iommu_iovad_free_iova((struct iova_domain *)mte_sync_tags,
			0, (size_t)&init_mm);
#endif
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
#ifdef CONFIG_AMLOGIC_PAGE_TRACE
	update_cma_page_trace(page, count);
#endif /* CONFIG_AMLOGIC_PAGE_TRACE */
}

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

void check_page_to_cma(struct compact_control *cc, struct page *page)
{
	struct address_space *mapping;

	/* no need check once it is true */
	if (test_bit(FORBID_TO_CMA_BIT, &cc->total_migrate_scanned))
		return;

	mapping = page_mapping(page);
	if ((unsigned long)mapping & PAGE_MAPPING_ANON)
		mapping = NULL;

	if (PageKsm(page) && !PageSlab(page))
		__set_bit(FORBID_TO_CMA_BIT, &cc->total_migrate_scanned);

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

/* cma alloc/free interface */

static unsigned long pfn_max_align_down(unsigned long pfn)
{
	return pfn & ~(max_t(unsigned long, MAX_ORDER_NR_PAGES,
			     pageblock_nr_pages) - 1);
}

#if CONFIG_AMLOGIC_KERNEL_VERSION < 14515
static unsigned long pfn_max_align_up(unsigned long pfn)
{
	return ALIGN(pfn, max_t(unsigned long, MAX_ORDER_NR_PAGES,
				pageblock_nr_pages));
}
#endif

static struct page *get_migrate_page(struct page *page, unsigned long private)
{
	struct migration_target_control *mtc;
	gfp_t gfp_mask;
	unsigned int order = 0;
	struct page *new_page = NULL;
	int nid;
	int zidx;

	mtc = (struct migration_target_control *)private;
	gfp_mask = mtc->gfp_mask | __GFP_NO_CMA;
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
	#ifdef CONFIG_AMLOGIC_PAGE_TRACE
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
		prep_transhuge_page(new_page);

#ifdef CONFIG_AMLOGIC_PAGE_TRACE
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
static int aml_alloc_contig_migrate_range(struct compact_control *cc,
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

	lru_cache_disable();
	while (pfn < end || !list_empty(&cc->migratepages)) {
		if (fatal_signal_pending(host)) {
			ret = -EINTR;
			break;
		}

		if (list_empty(&cc->migratepages)) {
			cc->nr_migratepages = 0;
			ret = isolate_migratepages_range(cc, pfn, end);
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

		nr_reclaimed = reclaim_clean_pages_from_list(cc->zone,
							     &cc->migratepages);
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

	lru_cache_enable();
	if (ret < 0) {
		if (ret == -EBUSY) {
			struct page *page;

			alloc_contig_dump_pages(&cc->migratepages);
			list_for_each_entry(page, &cc->migratepages, lru) {
				/* The page will be freed by putback_movable_pages soon */
				if (page_count(page) == 1)
					continue;
				page_pinner_failure_detect(page);
			}
		}
		putback_movable_pages(&cc->migratepages);
		return ret;
	}
	return 0;
}

static int cma_boost_work_func(void *cma_data)
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
		spin_lock(&c_work->list_lock);
		if (list_empty(&c_work->list)) {
			/* NO job todo ? */
			pr_err("%s,%d, list empty\n", __func__, __LINE__);
			spin_unlock(&c_work->list_lock);
			goto next;
		}
		job = list_first_entry(&c_work->list, struct work_cma, list);
		list_del(&job->list);
		spin_unlock(&c_work->list_lock);

		INIT_LIST_HEAD(&cc.migratepages);
		lru_add_drain();
		pfn      = job->pfn;
		cc.zone  = page_zone(pfn_to_page(pfn));
		end      = pfn + job->count;
		ret      = aml_alloc_contig_migrate_range(&cc, pfn, end,
							  1, job->host);
		job->ret = ret;
		if (ret)
			cma_debug(1, NULL, "failed, ret:%d\n", ret);
next:
		complete(&c_work->end);
		if (kthread_should_stop()) {
			pr_err("%s task exit\n", __func__);
			break;
		}
	}
	return 0;
}

static int __init init_cma_boost_task(void)
{
	int cpu;
	struct task_struct *task;
	struct cma_pcp *work;
	char task_name[20] = {};

	for_each_possible_cpu(cpu) {
		memset(task_name, 0, sizeof(task_name));
		sprintf(task_name, "cma_task%d", cpu);
		work = &per_cpu(cma_pcp_thread, cpu);
		init_completion(&work->start);
		init_completion(&work->end);
		INIT_LIST_HEAD(&work->list);
		spin_lock_init(&work->list_lock);
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
	return 0;
}
module_init(init_cma_boost_task);

int cma_alloc_contig_boost(unsigned long start_pfn, unsigned long count)
{
	struct cpumask has_work;
	int cpu, cpus, i = 0, ret = 0, ebusy = 0, einv = 0;
	atomic_t ok;
	unsigned long delta;
	unsigned long cnt;
	unsigned long flags;
	struct cma_pcp *work;
	struct work_cma job[NR_CPUS] = {};

	cpumask_clear(&has_work);

	if (allow_cma_tasks)
		cpus = allow_cma_tasks;
	else
		cpus = num_online_cpus() - 1;
	cnt   = count;
	delta = count / cpus;
	atomic_set(&ok, 0);
	local_irq_save(flags);
	for_each_online_cpu(cpu) {
		work = &per_cpu(cma_pcp_thread, cpu);
		spin_lock(&work->list_lock);
		INIT_LIST_HEAD(&job[cpu].list);
		job[cpu].pfn   = start_pfn + i * delta;
		job[cpu].count = delta;
		job[cpu].ret   = -1;
		job[cpu].host  = current;
		if (i == cpus - 1)
			job[cpu].count = count - i * delta;
		cpumask_set_cpu(cpu, &has_work);
		list_add(&job[cpu].list, &work->list);
		spin_unlock(&work->list_lock);
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

static int __aml_check_pageblock_isolate(unsigned long pfn,
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
			break;
		}
	}

	return pfn;
}

static inline struct page *
check_page_valid(unsigned long pfn, unsigned long nr_pages)
{
	int i;

	for (i = 0; i < nr_pages; i++) {
		struct page *page;

		page = pfn_to_online_page(pfn + i);
		if (!page)
			continue;
		return page;
	}
	return NULL;
}

int aml_check_pages_isolated(unsigned long start_pfn, unsigned long end_pfn,
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
	trace_test_pages_isolated(start_pfn, end_pfn, pfn);
	if (pfn < end_pfn)
		page_pinner_failure_detect(pfn_to_page(pfn));

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

int aml_cma_alloc_range(unsigned long start, unsigned long end,
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
	};
	INIT_LIST_HEAD(&cc.migratepages);

	mutex_lock(&cma_mutex);
	cma_debug(0, NULL, " range [%lx-%lx]\n", start, end);
#if CONFIG_AMLOGIC_KERNEL_VERSION >= 14515
	ret = start_isolate_page_range(pfn_max_align_down(start),
				       pfn_max_align_up(end), migrate_type,
				       0, &failed_pfn);
#else
	ret = start_isolate_page_range(pfn_max_align_down(start),
				       pfn_max_align_up(end), migrate_type, 0);
#endif
	if (ret < 0) {
		cma_debug(1, NULL, "ret:%d\n", ret);
		return ret;
	}

	cur_alloc_start = start;
	cur_alloc_end = end;
	cma_isolated += (pfn_max_align_up(end) - pfn_max_align_down(start));
try_again:
	lru_add_drain();
	drain_all_pages(cc.zone);
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
	outer_end = isolate_freepages_range(&cc, outer_start, end);
	if (!outer_end) {
		ret = -EBUSY;
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
	undo_isolate_page_range(pfn_max_align_down(start),
				pfn_max_align_up(end), migrate_type);
	cma_isolated -= (pfn_max_align_up(end) - pfn_max_align_down(start));
	cur_alloc_start = 0;
	cur_alloc_end = 0;
	mutex_unlock(&cma_mutex);

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
	#ifdef CONFIG_AMLOGIC_PAGE_TRACE
		if (cma_alloc_trace)
			pr_info("c f p:%lx, c:%d, f:%ps\n",
				pfn, count, (void *)find_back_trace());
	#endif /* CONFIG_AMLOGIC_PAGE_TRACE */
		atomic_long_sub(orig_nr_pages, &nr_cma_allocated);
	}
}

static bool cma_vma_show(struct page *page, struct vm_area_struct *vma,
			 unsigned long addr, void *arg)
{
#ifdef CONFIG_AMLOGIC_USER_FAULT
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
	rmap_walk(page, &rwc);
}

void show_page(struct page *page)
{
	unsigned long trace = 0;
	unsigned long map_flag = -1UL;

	if (!page)
		return;
#ifdef CONFIG_AMLOGIC_PAGE_TRACE
	trace = get_page_trace(page);
#endif
	if (page->mapping && !((unsigned long)page->mapping & 0x3))
		map_flag = page->mapping->flags;
	pr_info("page:%lx, map:%lx, mf:%lx, pf:%lx, m:%d, c:%d, o:%lx, pt:%lx, f:%ps\n",
		page_to_pfn(page), (unsigned long)page->mapping, map_flag,
		page->flags & 0xffffffff,
		page_mapcount(page), page_count(page), page->private, page->index,
		(void *)trace);
	if (cma_debug_level > 4 && !irqs_disabled())
		rmap_walk_vma(page);
}

static int cma_debug_show(struct seq_file *m, void *arg)
{
	seq_printf(m, "level=%d, alloc trace:%d, allow task:%d\n",
		   cma_debug_level, cma_alloc_trace, allow_cma_tasks);
	seq_printf(m, "driver used:%lu isolated:%d total:%lu\n",
		   get_cma_allocated(), 0, totalcma_pages);
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

	buf = kmalloc(count, GFP_KERNEL);
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
		goto exit;
	}

	if (kstrtoint(buf, 10, &arg))
		goto exit;

	if (arg > MAX_DEBUG_LEVEL)
		goto exit;

	ok = 1;
	cma_debug_level = arg;
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

