// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifdef MODULE

#include <linux/platform_device.h>
#include <linux/module.h>
#include "clk-common-init.h"

static int __init clk_module_init(void)
{
	clk_measure_init();

	clk_debug_init();

	return 0;
}

static void __exit clk_module_exit(void)
{
}

module_init(clk_module_init);
module_exit(clk_module_exit);

MODULE_LICENSE("GPL v2");
#endif
