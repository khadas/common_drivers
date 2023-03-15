// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

//#define DEBUG
#include <linux/module.h>
#include <linux/amlogic/module_merge.h>
#include "main.h"

static int __init regulator_main_init(void)
{
	pr_debug("### %s() start\n", __func__);
	call_sub_init(meson_pmic6b_regulator_init);
	pr_debug("### %s() end\n", __func__);
	return 0;
}

static void __exit regulator_main_exit(void)
{
	meson_pmic6b_regulator_exit();
}

module_init(regulator_main_init);
module_exit(regulator_main_exit);

MODULE_LICENSE("GPL v2");
