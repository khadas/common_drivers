// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

//#define DEBUG
#include <linux/module.h>
#include <linux/amlogic/module_merge.h>
#include "main.h"

#if IS_ENABLED(CONFIG_AMLOGIC_DEBUG)
#include <linux/amlogic/gki_module.h>
#endif

static int __init pm_main_init(void)
{
	pr_debug("### %s() start\n", __func__);
	call_sub_init(pm_init);
	call_sub_init(reboot_init);
	pr_debug("### %s() end\n", __func__);

	return 0;
}

static void __exit pm_main_exit(void)
{
	reboot_exit();
	pm_exit();
}

module_init(pm_main_init);
module_exit(pm_main_exit);
MODULE_LICENSE("GPL v2");
