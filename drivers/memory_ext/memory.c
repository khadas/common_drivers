// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/stddef.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/swap.h>
#include <linux/interrupt.h>
#include <linux/pagemap.h>
#include <linux/jiffies.h>
#include <linux/memblock.h>
#include <linux/compiler.h>
#include <linux/kernel.h>
#include <linux/kasan.h>
#include <linux/module.h>
#include <linux/suspend.h>
#include <linux/pagevec.h>
#include <linux/blkdev.h>
#include <linux/slab.h>
#include <linux/amlogic/aml_cma.h>
#include <../../../mm/internal.h>
#include <linux/kmemleak.h>
#if IS_BUILTIN(CONFIG_AMLOGIC_PAGE_TRACE)
#include <linux/amlogic/page_trace.h>
#include <linux/kasan.h>
#endif

void should_wakeup_kswap(gfp_t gfp_mask, int order,
				       struct alloc_context *ac)
{
	unsigned long free_pages;
	struct zoneref *z = ac->preferred_zoneref;
	struct zone *zone;
	unsigned long high_wm;

	/*
	 * 1, if flag not allow reclaim
	 * 2, if with aotimic, we still need enable pre-wake up of
	 *    kswap to avoid large amount memory request fail in very
	 *    short time
	 */
	if (!(gfp_mask & __GFP_RECLAIM) && !(gfp_mask & __GFP_ATOMIC))
		return;

	for_next_zone_zonelist_nodemask(zone, z, ac->highest_zoneidx,
					ac->nodemask) {
		free_pages = zone_page_state(zone, NR_FREE_PAGES);
	#ifdef CONFIG_AMLOGIC_CMA
		if (!can_use_cma(gfp_mask))
			free_pages -= zone_page_state(zone, NR_FREE_CMA_PAGES);
	#endif /* CONFIG_AMLOGIC_CMA */
		/*
		 * wake up kswapd before get pages from buddy, this help to
		 * fast reclaim process and can avoid memory become too low
		 * some times
		 */
		high_wm = high_wmark_pages(zone);
		if (gfp_mask & __GFP_HIGH) /* 1.5x if __GFP_HIGH */
			high_wm = ((high_wm * 3) / 2);
		if (free_pages <= high_wm)
			wakeup_kswapd(zone, gfp_mask, order, ac->highest_zoneidx);
	}
}
EXPORT_SYMBOL(should_wakeup_kswap);

void adjust_redzone_end(const void *ptr, size_t size, unsigned long *p_end)
{
	if (PageOwnerPriv1(virt_to_page(ptr))) { /* end of this page was freed */
		*p_end = (unsigned long)ptr + PAGE_ALIGN(size);
	}
}

void *aml_slub_alloc_large(size_t size, gfp_t flags, int order)
{
	struct page *page, *p;

	flags &= ~__GFP_COMP;
	page = alloc_pages(flags, order);
	if (page) {
		unsigned long used_pages = PAGE_ALIGN(size) / PAGE_SIZE;
		unsigned long total_pages = 1 << order;
		unsigned long saved = 0;
		unsigned long fun = 0;
		int i;

		/* record how many pages in first page*/
		__SetPageHead(page);
		SetPageOwnerPriv1(page);	/* special flag */

	#if IS_BUILTIN(CONFIG_AMLOGIC_PAGE_TRACE)
		fun = get_page_trace(page);
	#endif

		for (i = 1; i < used_pages; i++) {
			p = page + i;
			set_compound_head(p, page);
		#if IS_BUILTIN(CONFIG_AMLOGIC_PAGE_TRACE)
			set_page_trace(page, 0, flags, (void *)fun);
		#endif
		}
		page->index = used_pages;
		split_page(page, order);
		p = page + used_pages;
		while (used_pages < total_pages) {
			__free_pages(p, 0);
			used_pages++;
			p++;
			saved++;
		}
		pr_debug("%s, page:%p, all:%5ld, size:%5ld, save:%5ld, f:%ps\n",
			__func__, page_address(page), total_pages * PAGE_SIZE,
			(long)size, saved * PAGE_SIZE, (void *)fun);
		return page;
	}
	return NULL;
}

static void aml_slub_free_large(struct page *page, const void *obj)
{
	unsigned int nr_pages, i;

	if (page) {
		__ClearPageHead(page);
		ClearPageOwnerPriv1(page);
		nr_pages = page->index;
		pr_debug("%s, page:%p, pages:%d, obj:%p\n",
			__func__, page_address(page), nr_pages, obj);
		for (i = 0; i < nr_pages; i++)  {
			__free_pages(page, 0);
			page++;
		}
	}
}

int aml_free_nonslab_page(struct page *page, void *object)
{
	unsigned int nr_pages;
	unsigned int order = compound_order(page);

	if (page->index)
		nr_pages = page->index;
	else
		nr_pages = 1 << order;
	VM_BUG_ON_PAGE(!PageCompound(page), page);
#ifdef CONFIG_DEBUG_KMEMLEAK
	kmemleak_free(object);
#endif
	kasan_kfree_large(object);
	mod_lruvec_page_state(page, NR_SLAB_UNRECLAIMABLE_B, -(nr_pages * PAGE_SIZE));
	if (unlikely(PageOwnerPriv1(page))) {
		aml_slub_free_large(page, object);
		return 1;
	}

	return 0;
}
