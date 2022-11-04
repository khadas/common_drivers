// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

//#define DEBUG
#include <linux/module.h>
#include <linux/amlogic/module_merge.h>
#include "main.h"

static int __init memory_main_init(void)
{
	pr_debug("### %s() start\n", __func__);
	call_sub_init(filecache_module_init);
	call_sub_init(aml_watch_pint_init);
	call_sub_init(aml_reg_init);
	call_sub_init(ddr_tool_init);
	pr_debug("### %s() end\n", __func__);

	return 0;
}

static void __exit memory_main_exit(void)
{
	ddr_tool_exit();
	aml_reg_exit();
	aml_watch_point_uninit();
	filecache_module_exit();
}

module_init(memory_main_init);
module_exit(memory_main_exit);
MODULE_LICENSE("GPL v2");
