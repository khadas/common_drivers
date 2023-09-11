// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */
#include <linux/platform_device.h>
#include <linux/module.h>
#include "clk-common-init.h"
#include <linux/amlogic/gki_module.h>

int bypass_clk_disable;

static int bypass_clk_disable_setup(char *buf)
{
	if (!buf)
		return -EINVAL;

	if (kstrtoint(buf, 0, &bypass_clk_disable)) {
		pr_err("bypass_clk_disable error: %s\n", buf);
		return -EINVAL;
	}

	return 0;
}
__setup("bypass_clk_disable=", bypass_clk_disable_setup);

static int __init clk_module_init(void)
{
#ifdef MODULE
	clk_measure_init();

	clk_debug_init();
#endif
	return 0;
}

static void __exit clk_module_exit(void)
{
}

module_init(clk_module_init);
module_exit(clk_module_exit);

MODULE_LICENSE("GPL v2");
