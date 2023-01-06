// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

//#define DEBUG
#include <linux/module.h>
#include <linux/amlogic/module_merge.h>
#include "main.h"

static int __init domain_main_init(void)
{
	pr_debug("### %s() start\n", __func__);
	call_sub_init(sec_power_domain_init);
	call_sub_init(power_ee_domain_init);
	pr_debug("### %s() end\n", __func__);

	return 0;
}

static void __exit domain_main_exit(void)
{
	sec_power_domain_exit();
	power_ee_domain_exit();
}

module_init(domain_main_init);
module_exit(domain_main_exit);
MODULE_LICENSE("GPL v2");
