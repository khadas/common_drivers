/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __AMLOGIC_FIXED_AREA_H__
#define __AMLOGIC_FIXED_AREA_H__

#include <linux/mm.h>

void *aml_dma_alloc_contiguous(size_t size, gfp_t gfp, struct page **ret_page, u32 area);

bool aml_dma_free_contiguous(void *vaddr, const struct page *pages, size_t size, u32 area);

#endif
