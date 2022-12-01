// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */
// #define DEBUG
// #define USE_CMA
// #define TEST_ACCESS
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/module.h>
#include <linux/of_fdt.h>
#include <linux/libfdt_env.h>
#include <linux/of_reserved_mem.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#ifdef USE_CMA
#include <linux/dma-map-ops.h>
#include <linux/cma.h>
#endif
#include <linux/arm-smccc.h>
#include <linux/memblock.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <asm/page.h>

static void __iomem *sharemem_in_base;
static void __iomem *sharemem_out_base;
static long phy_in_base;
static long phy_out_base;
static unsigned long secmon_start_virt;
static unsigned int secmon_size;

#if IS_ENABLED(CONFIG_ARM64)
#define IN_SIZE	0x6000
#else
#define IN_SIZE	0x6000
#endif

#define OUT_SIZE 0x1000
static DEFINE_MUTEX(sharemem_mutex);
#define DEV_REGISTERED 1
#define DEV_UNREGISTERED 0

unsigned int sharemem_in_size = IN_SIZE;
unsigned int sharemem_out_size = OUT_SIZE;

static int secmon_dev_registered = DEV_UNREGISTERED;
static long get_sharemem_info(unsigned int function_id)
{
	struct arm_smccc_res res;

	arm_smccc_smc(function_id, 0, 0, 0, 0, 0, 0, 0, &res);

	return res.a0;
}

static void get_sharemem_size(unsigned int function_id)
{
	struct arm_smccc_res res;

	arm_smccc_smc(function_id, 1, 0, 0, 0, 0, 0, 0, &res);
	if (res.a0 != -1)
		sharemem_in_size =  res.a0;

	arm_smccc_smc(function_id, 2, 0, 0, 0, 0, 0, 0, &res);
	if (res.a0 != -1)
		sharemem_out_size =  res.a0;
}

#define RESERVE_MEM_SIZE	0x300000

int within_secmon_region(unsigned long addr)
{
	if (!secmon_start_virt)
		return 0;

	if (addr >= secmon_start_virt &&
	    addr <= (secmon_start_virt + secmon_size))
		return 1;

	return 0;
}
EXPORT_SYMBOL(within_secmon_region);

static int get_reserver_base_size(struct platform_device *pdev)
{
	struct device_node *mem_region;
	struct reserved_mem *rmem;

	mem_region = of_parse_phandle(pdev->dev.of_node, "memory-region", 0);
	if (!mem_region) {
		dev_warn(&pdev->dev, "no such memory-region\n");
		return -ENODEV;
	}

	rmem = of_reserved_mem_lookup(mem_region);
	if (!rmem) {
		dev_warn(&pdev->dev, "no such reserved mem of node name %s\n",
				pdev->dev.of_node->name);
		return -ENODEV;
	}

	/* Need to wait for reserved memory to be mapped */
	//if (!rmem->priv)
	//	return -EPROBE_DEFER;

	if (!rmem->base || !rmem->size) {
		dev_warn(&pdev->dev, "unexpected reserved memory\n");
		return -EINVAL;
	}

	secmon_start_virt = __phys_to_virt(rmem->base);

	//pr_info("secmon_start_virt=0x%016lx, base=0x%010lx, size=0x%010x\n",
	//	secmon_start_virt, rmem->base, rmem->size);

	return 0;
}

#ifdef TEST_ACCESS
static void test_access_secmon(void)
{
	int	i, j;
	int	nlines;
	u32	*p;

	nlines = 16;
	p = (u32 *)(secmon_start_virt + 0x200000);
	for (i = 0; i < nlines; i++) {
		/*
		 * just display low 16 bits of address to keep
		 * each line of the dump < 80 characters
		 */
		pr_info("%04lx ", (unsigned long)p & 0xffff);
		for (j = 0; j < 8; j++) {
			u32	data;

			if (get_kernel_nofault(data, p))
				pr_cont(" ********");
			else
				pr_cont(" %08x", data);
			++p;
		}
		pr_cont("\n");
	}
}
#endif

static void *ram_vmap(phys_addr_t start, size_t size)
{
	struct page **pages;
	phys_addr_t page_start;
	unsigned int page_count;
	unsigned int i;
	void *vaddr;

	page_start = start - offset_in_page(start);
	page_count = DIV_ROUND_UP(size + offset_in_page(start), PAGE_SIZE);

	pages = kmalloc_array(page_count, sizeof(struct page *), GFP_KERNEL);
	if (!pages)
		return NULL;

	for (i = 0; i < page_count; i++) {
		phys_addr_t addr = page_start + i * PAGE_SIZE;

		pages[i] = pfn_to_page(addr >> PAGE_SHIFT);
	}
	vaddr = vmap(pages, page_count, VM_MAP, PAGE_KERNEL);
	kfree(pages);

	return vaddr + offset_in_page(start);
}

static int secmon_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	unsigned int id;
	int ret = 0;
#ifdef USE_CMA
	struct page *page;
#endif

	if (!of_property_read_u32(np, "in_base_func", &id))
		phy_in_base = get_sharemem_info(id);

	if (!of_property_read_u32(np, "out_base_func", &id))
		phy_out_base = get_sharemem_info(id);

	if (!of_property_read_u32(np, "inout_size_func", &id))
		get_sharemem_size(id);

	if (!of_property_read_bool (pdev->dev.of_node, "no-memory")) {
		if (of_property_read_u32(np, "reserve_mem_size", &secmon_size)) {
			pr_err("can't get reserve_mem_size, use default value\n");
			secmon_size = RESERVE_MEM_SIZE;
		} else {
			pr_info("reserve_mem_size:0x%x\n", secmon_size);
		}

		get_reserver_base_size(pdev);
#ifdef USE_CMA
		ret = of_reserved_mem_device_init(&pdev->dev);
		if (ret) {
			pr_err("reserve memory init fail:%d\n", ret);
			return ret;
		}

		page = dma_alloc_from_contiguous(&pdev->dev, secmon_size >> PAGE_SHIFT, 0, 0);
		if (!page) {
			pr_err("alloc page failed, ret:%p\n", page);
			return -ENOMEM;
		}
		pr_debug("get page:%p, %lx\n", page, page_to_pfn(page));
		secmon_start_virt = (unsigned long)page_to_virt(page);
#endif

#ifdef TEST_ACCESS
		test_access_secmon();
#endif
	}

	sharemem_in_base = ram_vmap(phy_in_base, sharemem_in_size);
	if (!sharemem_in_base) {
		pr_err("secmon share mem in buffer remap fail!\n");
		return -ENOMEM;
	}
	sharemem_out_base = ram_vmap(phy_out_base, sharemem_out_size);
	if (!sharemem_out_base) {
		pr_err("secmon share mem out buffer remap fail!\n");
		return -ENOMEM;
	}
	secmon_dev_registered = DEV_REGISTERED;
	pr_info("share in base: 0x%lx, share out base: 0x%lx\n",
		(long)sharemem_in_base, (long)sharemem_out_base);
	pr_info("phy_in_base: 0x%lx, phy_out_base: 0x%lx\n",
		phy_in_base, phy_out_base);

	return ret;
}

#ifdef USE_CMA
void __init secmon_clear_cma_mmu(void)
{
	struct device_node *np;
	unsigned int clear[2] = {};

	np = of_find_node_by_name(NULL, "secmon");
	if (!np)
		return;

	if (of_property_read_u32_array(np, "clear_range", clear, 2))
		pr_info("can't fine clear_range\n");
	else
		pr_info("clear_range:%x %x\n", clear[0], clear[1]);

	if (clear[0]) {
		struct page *page = phys_to_page(clear[0]);
		int cnt = clear[1] / PAGE_SIZE;

		cma_mmu_op(page, cnt, 0);	//cma_mmu_op() implemented in kernel
	}
}
#endif

static const struct of_device_id secmon_dt_match[] = {
	{ .compatible = "amlogic, secmon" },
	{ /* sentinel */ },
};

static  struct platform_driver secmon_platform_driver = {
	.probe		= secmon_probe,
	.driver		= {
		.owner		= THIS_MODULE,
		.name		= "secmon",
		.of_match_table	= secmon_dt_match,
	},
};

int __init meson_secmon_init(void)
{
	int ret;

	ret = platform_driver_register(&secmon_platform_driver);
	WARN((secmon_dev_registered != DEV_REGISTERED),
	     "ERROR: secmon device must be enable!!!\n");
	return ret;
}

void meson_sm_mutex_lock(void)
{
	mutex_lock(&sharemem_mutex);
}
EXPORT_SYMBOL(meson_sm_mutex_lock);

void meson_sm_mutex_unlock(void)
{
	mutex_unlock(&sharemem_mutex);
}
EXPORT_SYMBOL(meson_sm_mutex_unlock);

void __iomem *get_meson_sm_input_base(void)
{
	return sharemem_in_base;
}
EXPORT_SYMBOL(get_meson_sm_input_base);

void __iomem *get_meson_sm_output_base(void)
{
	return sharemem_out_base;
}
EXPORT_SYMBOL(get_meson_sm_output_base);

long get_secmon_phy_input_base(void)
{
	return phy_in_base;
}
EXPORT_SYMBOL(get_secmon_phy_input_base);

long get_secmon_phy_output_base(void)
{
	return phy_out_base;
}
EXPORT_SYMBOL(get_secmon_phy_output_base);

unsigned int get_secmon_sharemem_in_size(void)
{
	return sharemem_in_size;
}
EXPORT_SYMBOL(get_secmon_sharemem_in_size);

unsigned int get_secmon_sharemem_out_size(void)
{
	return sharemem_out_size;
}
EXPORT_SYMBOL(get_secmon_sharemem_out_size);

