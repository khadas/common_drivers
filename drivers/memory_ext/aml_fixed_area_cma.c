// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/io.h>
#include <linux/of_address.h>
#include <linux/of_reserved_mem.h>
#include <linux/dma-map-ops.h>
#include <linux/cma.h>
#include <linux/genalloc.h>
//#include <linux/dma-noncoherent.h>
#include <linux/amlogic/aml_fix_area.h>
//#include <amlogic/gki_module.h>
#include <linux/slab.h>

#include <linux/dma-mapping.h>
static struct cma *fixed_area_one, *fixed_area_two;
static struct gen_pool *area_one_pool, *area_two_pool;

/*
 * Remaps an allocated contiguous region into another vm_area.
 * Cannot be used in non-sleeping contexts
 */
static void *aml_dma_common_contiguous_remap(struct page *page, size_t size,
			pgprot_t prot, const void *caller)
{
	int count = PAGE_ALIGN(size) >> PAGE_SHIFT;
	struct page **pages;
	void *vaddr;
	int i;

	pages = kmalloc_array(count, sizeof(struct page *), GFP_KERNEL);
	if (!pages)
		return NULL;
	for (i = 0; i < count; i++)
		pages[i] = nth_page(page, i);
	vaddr = vmap(pages, count, VM_DMA_COHERENT, prot);
	kfree(pages);

	return vaddr;
}

/*
 * Unmaps a range previously mapped by dma_common_*_remap
 */
static void aml_dma_common_free_remap(void *cpu_addr, size_t size)
{
	vunmap(cpu_addr);
}

static int aml_atomic_pool_init(struct page *page, size_t atomic_pool_size, u32 area)
{
	void *addr;
	int ret;
	struct gen_pool *atomic_pool;

	if (!page)
		goto out;

	//arch_dma_prep_coherent(page, atomic_pool_size);

	if (area == 1) {
		area_one_pool = gen_pool_create(L1_CACHE_BYTES, -1);
		if (!area_one_pool)
			goto out;
		atomic_pool = area_one_pool;
	} else if (area == 2) {
		area_two_pool = gen_pool_create(L1_CACHE_BYTES, -1);
		if (!area_two_pool)
			goto out;
		atomic_pool = area_two_pool;
	} else {
		goto out;
	}

	addr = aml_dma_common_contiguous_remap(page, atomic_pool_size,
					   pgprot_dmacoherent(PAGE_KERNEL),
					   __builtin_return_address(0));
	if (!addr)
		goto destroy_genpool;

	ret = gen_pool_add_virt(atomic_pool, (unsigned long)addr,
				page_to_phys(page), atomic_pool_size, -1);
	if (ret)
		goto remove_mapping;
	gen_pool_set_algo(atomic_pool, gen_pool_first_fit_order_align, NULL);

	pr_info("preallocated %zu KiB pool for atomic allocations\n",
		atomic_pool_size / 1024);
	return 0;

remove_mapping:
	aml_dma_common_free_remap(addr, atomic_pool_size);
destroy_genpool:
	gen_pool_destroy(atomic_pool);
	atomic_pool = NULL;
	aml_dma_free_contiguous(addr, page, atomic_pool_size, area);
out:
	pr_err("failed to allocate %zu KiB pool for atomic allocation\n",
		atomic_pool_size / 1024);
	return -ENOMEM;
}

static bool addr_in_atomic_pool(void *start, size_t size, u32 area)
{
	struct gen_pool *atomic_pool;

	if (area == 1)
		atomic_pool = area_one_pool;
	else if (area == 2)
		atomic_pool = area_two_pool;
	else
		return false;

	if (unlikely(!atomic_pool))
		return false;

	return gen_pool_has_addr(atomic_pool, (unsigned long)start, size);
}

static void *aml_alloc_from_pool(size_t size, struct page **ret_page, gfp_t gfp, u32 area)
{
	unsigned long val;
	void *ptr = NULL;
	struct gen_pool *atomic_pool;

	if (area == 1)
		atomic_pool = area_one_pool;
	else if (area == 2)
		atomic_pool = area_two_pool;
	else
		return NULL;

	if (!atomic_pool) {
		WARN(1, "aml atomic pool not initialised!\n");
		return NULL;
	}

	val = gen_pool_alloc(atomic_pool, size);
	if (val) {
		phys_addr_t phys = gen_pool_virt_to_phys(atomic_pool, val);
		*ret_page = pfn_to_page(__phys_to_pfn(phys));
		ptr = (void *)val;
		if (gfp & __GFP_ZERO)
			memset(ptr, 0, size);
	}

	return ptr;
}

static bool aml_free_from_pool(void *start, size_t size, u32 area)
{
	struct gen_pool *atomic_pool;

	if (area == 1)
		atomic_pool = area_one_pool;
	else if (area == 2)
		atomic_pool = area_two_pool;
	else
		return false;

	if (!addr_in_atomic_pool(start, size, area))
		return false;
	gen_pool_free(atomic_pool, (unsigned long)start, size);
	return true;
}

void *aml_dma_alloc_contiguous(size_t size, gfp_t gfp, struct page **ret_page, u32 area)
{
	size_t count;
	struct cma *cma = NULL;
	void *ret;

	if (area == 1)
		cma = fixed_area_one;
	else if (area == 2)
		cma = fixed_area_two;

	size = PAGE_ALIGN(size);
	count = size >> PAGE_SHIFT;

	if (!gfpflags_allow_blocking(gfp)) {
		ret = aml_alloc_from_pool(size, ret_page, gfp, area);
		if (!ret)
			return NULL;
		goto done;
	}
	/* CMA can be used only in the context which permits sleeping */
	if (cma && gfpflags_allow_blocking(gfp)) {
		size_t align = get_order(size);
		size_t cma_align = min_t(size_t, align, CONFIG_CMA_ALIGNMENT);

		*ret_page = cma_alloc(cma, count, cma_align, gfp & __GFP_NOWARN);
	}
	//arch_dma_prep_coherent(*ret_page, size);
	ret = aml_dma_common_contiguous_remap(*ret_page, size,
				pgprot_dmacoherent(PAGE_KERNEL),
				__builtin_return_address(0));
	if (!ret) {
		cma_release(cma, *ret_page, count);
		return ret;
	}
	if (gfp & __GFP_ZERO)
		memset(ret, 0, size);
done:
	return ret;
}
EXPORT_SYMBOL_GPL(aml_dma_alloc_contiguous);

bool aml_dma_free_contiguous(void *vaddr, const struct page *pages, size_t size, u32 area)
{
	size_t count;
	bool res = false;
	struct cma *cma = NULL;

	size = PAGE_ALIGN(size);
	count = size >> PAGE_SHIFT;

	if (vaddr && aml_free_from_pool(vaddr, size, area))
		return true;

	if (area == 1)
		cma = fixed_area_one;
	else if (area == 2)
		cma = fixed_area_two;

	if (cma)
		res = cma_release(cma, pages, count);

	return res;
}
EXPORT_SYMBOL_GPL(aml_dma_free_contiguous);

static int fixed_area_cma_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	int ret;
	u64 pool_size;
	struct page *pool;
	void *vaddr;

	ret = of_reserved_mem_device_init_by_idx(&pdev->dev, np, 0);
	if (ret) {
		dev_err(&pdev->dev, "failed to init reserved memory 1.\n");
		return -EINVAL;
	}
	fixed_area_one = dev_get_cma_area(&pdev->dev);

	ret = of_property_read_u64(np, "pool_size", &pool_size);
	if (ret < 0) {
		dev_err(&pdev->dev, "invalid pool size\n");
		return -EINVAL;
	}

	vaddr = aml_dma_alloc_contiguous(pool_size, GFP_KERNEL, &pool, 1);
	if (vaddr)
		aml_atomic_pool_init(pool, (size_t)pool_size, 1);
	pr_info("%s success.\n", __func__);

	return 0;
}

static const struct of_device_id fixed_area_cma_dt_match[] = {
	{ .compatible = "amlogic, fixed_area_cma" },
	{ /* sentinel */ }
};

static  struct platform_driver fixed_area_cma_platform_driver = {
	.probe		= fixed_area_cma_probe,
	.driver		= {
		.owner		= THIS_MODULE,
		.name		= "fixed_area_cma",
		.of_match_table	= fixed_area_cma_dt_match,
	},
};

static int __init meson_fixed_area_cma_init(void)
{
	return  platform_driver_register(&fixed_area_cma_platform_driver);
}
core_initcall(meson_fixed_area_cma_init);

MODULE_LICENSE("GPL v2");
