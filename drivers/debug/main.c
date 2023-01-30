// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

//#define DEBUG
#include <linux/module.h>
#include <linux/amlogic/module_merge.h>
#include "main.h"

static int __init debug_main_init(void)
{
	pr_debug("### %s() start\n", __func__);
#if IS_MODULE(CONFIG_AMLOGIC_DEBUG_PRINTK)
	call_sub_init(printk_vendor_hook_init);
#endif
	call_sub_init(debug_lockup_init);
	call_sub_init(aml_debug_init);
	call_sub_init(cpu_mhz_init);
	call_sub_init(meson_atrace_init);
	call_sub_init(debug_file_init);
	call_sub_init(gki_config_init);
	call_sub_init(aml_hld_init);
	call_sub_init(aml_sched_init);
	pr_debug("### %s() end\n", __func__);
	return 0;
}

static void __exit debug_main_exit(void)
{
	debug_file_exit();
#if IS_MODULE(CONFIG_AMLOGIC_DEBUG_PRINTK)
	printk_vendor_hook_exit();
#endif
}

module_init(debug_main_init);
module_exit(debug_main_exit);

MODULE_LICENSE("GPL v2");
