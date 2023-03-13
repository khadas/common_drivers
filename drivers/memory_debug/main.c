// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

//#define DEBUG
#include <linux/module.h>
#include <linux/device.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/moduleparam.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/mm.h>
#include <linux/kasan.h>
#include <linux/amlogic/module_merge.h>
#include "main.h"
#include <linux/amlogic/aml_free_reserved.h>

unsigned long aml_free_reserved_area(void *start, void *end, int poison, const char *s)
{
	void *pos;
	unsigned long pages = 0;

	start = (void *)PAGE_ALIGN((unsigned long)start);
	end = (void *)((unsigned long)end & PAGE_MASK);
	for (pos = start; pos < end; pos += PAGE_SIZE, pages++) {
		struct page *page = virt_to_page(pos);
		void *direct_map_addr;

		/*
		 * 'direct_map_addr' might be different from 'pos'
		 * because some architectures' virt_to_page()
		 * work with aliases.  Getting the direct map
		 * address ensures that we get a _writeable_
		 * alias for the memset().
		 */
		direct_map_addr = page_address(page);
		/*
		 * Perform a kasan-unchecked memset() since this memory
		 * has not been initialized.
		 */
		direct_map_addr = kasan_reset_tag(direct_map_addr);
		if ((unsigned int)poison <= 0xFF)
			memset(direct_map_addr, poison, PAGE_SIZE);

		free_reserved_page(page);
	}

	if (pages && s)
		pr_info("Freeing %s memory: %ldK\n",
			s, pages << (PAGE_SHIFT - 10));

	return pages;
}
EXPORT_SYMBOL(aml_free_reserved_area);

static int __init memory_main_init(void)
{
	pr_debug("### %s() start\n", __func__);
	call_sub_init(filecache_module_init);
	call_sub_init(aml_watch_pint_init);
	call_sub_init(aml_reg_init);
	call_sub_init(ddr_tool_init);
	call_sub_init(ramdump_init);
	pr_debug("### %s() end\n", __func__);

	return 0;
}

static void __exit memory_main_exit(void)
{
	ramdump_uninit();
	ddr_tool_exit();
	aml_reg_exit();
	aml_watch_point_uninit();
	filecache_module_exit();
}

module_init(memory_main_init);
module_exit(memory_main_exit);
MODULE_LICENSE("GPL v2");
