/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __AMLOGIC_CMA_H__
#define __AMLOGIC_CMA_H__

#ifdef CONFIG_AMLOGIC_CMA
#define GFP_NO_CMA    (__GFP_NO_CMA | __GFP_WRITE)
static inline bool cma_forbidden_mask(gfp_t gfp_flags)
{
	if ((gfp_flags & GFP_NO_CMA) || !(gfp_flags & __GFP_MOVABLE))
		return true;
	return false;
}
#endif

/* the highest bit of total_migrate_scanned in struct compact_control */
#define FORBID_TO_CMA_BIT	63

#ifdef CONFIG_AMLOGIC_CMA_DIS
extern unsigned long ion_cma_allocated;
#endif
extern int cma_debug_level;

void cma_page_count_update(long size);
void aml_cma_alloc_pre_hook(int *dummy, int count, unsigned long *tick);
void aml_cma_alloc_post_hook(int *dummy, int count, struct page *page,
			     unsigned long tick, int ret);
void aml_cma_release_hook(int a, struct page *p);
struct page *get_cma_page(struct zone *zone, unsigned int order);
unsigned long compact_to_free_cma(struct zone *zone);
bool can_use_cma(gfp_t gfp_flags);
bool cma_page(struct page *page);
unsigned long get_cma_allocated(void);
unsigned long get_total_cmapages(void);
int aml_cma_alloc_range(unsigned long start, unsigned long end,
			unsigned int migrate_type, gfp_t gfp_mask);
void check_cma_isolated(unsigned long *isolate,
			unsigned long active, unsigned long inactive);
void cma_keep_high_active(struct page *page, struct list_head *high,
			  struct list_head *clean);
void update_gfp_flags(gfp_t *gfp);
void check_water_mark(long free_pages, unsigned long mark);
struct compact_control;
void check_page_to_cma(struct compact_control *cc, struct page *page);
struct page *get_compact_page(struct page *migratepage,
			      struct compact_control *cc);

void aml_cma_free(unsigned long pfn, unsigned int nr_pages, int update);

void show_page(struct page *page);

struct page *compaction_cma_alloc(struct page *migratepage,
				  unsigned long data,
				  int **result);

/* for cma area serror */
int cma_mmu_op(struct page *page, int count, bool set);
int setup_cma_full_pagemap(unsigned long pfn, unsigned long count);

/* check page in cma allocating process */
int in_cma_allocating(struct page *page);

#define cma_debug(l, p, format, args...)	\
	{								\
		if ((l) < cma_debug_level) {				\
			show_page(p);					\
			pr_info("%s,%d " format, __func__, __LINE__, ##args); \
		}							\
	}

#endif /* __AMLOGIC_CMA_H__ */
