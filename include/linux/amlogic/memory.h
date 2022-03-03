/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _INC_AML_MM_H_
#define _INC_AML_MM_H_
void should_wakeup_kswap(gfp_t gfp_mask, int order,
				       struct alloc_context *ac);
void adjust_redzone_end(const void *ptr, size_t size, unsigned long *p_end);
void *aml_slub_alloc_large(size_t size, gfp_t flags, int order);
int aml_free_nonslab_page(struct page *page, void *object);
#endif
