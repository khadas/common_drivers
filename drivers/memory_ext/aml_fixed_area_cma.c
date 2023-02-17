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
#include <linux/amlogic/aml_fix_area.h>
#include <linux/slab.h>

#include <linux/dma-mapping.h>
static struct cma *fixed_area_one, *fixed_area_two;

void *aml_dma_alloc_contiguous(size_t size, gfp_t gfp, struct page **ret_page, u32 area)
{
	size_t count;
	struct cma *cma = NULL;
	void *vaddr = NULL;

	if (area == 1)
		cma = fixed_area_one;
	else if (area == 2)
		cma = fixed_area_two;

	size = PAGE_ALIGN(size);
	count = size >> PAGE_SHIFT;

	/* CMA can be used only in the context which permits sleeping */
	if (cma && gfpflags_allow_blocking(gfp)) {
		size_t align = get_order(size);
		size_t cma_align = min_t(size_t, align, CONFIG_CMA_ALIGNMENT);

#if CONFIG_AMLOGIC_KERNEL_VERSION >= 14515
		*ret_page = cma_alloc(cma, count, cma_align, gfp);
#else
		*ret_page = cma_alloc(cma, count, cma_align, gfp & __GFP_NOWARN);
#endif
	}
	if (*ret_page)
		vaddr = page_address(*ret_page);

	return vaddr;
}
EXPORT_SYMBOL_GPL(aml_dma_alloc_contiguous);

bool aml_dma_free_contiguous(void *vaddr, const struct page *pages, size_t size, u32 area)
{
	size_t count;
	bool res = false;
	struct cma *cma = NULL;

	size = PAGE_ALIGN(size);
	count = size >> PAGE_SHIFT;

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

	ret = of_reserved_mem_device_init_by_idx(&pdev->dev, np, 0);
	if (ret) {
		dev_err(&pdev->dev, "failed to init reserved memory 1.\n");
		return -EINVAL;
	}
	fixed_area_one = dev_get_cma_area(&pdev->dev);

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
