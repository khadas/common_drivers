// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

//#define DEBUG
#include <linux/module.h>
#include <linux/amlogic/module_merge.h>
#include "meson-mmc-main.h"

static int __init mmc_main_init(void)
{
	pr_debug("### %s() start\n", __func__);
	call_sub_init(g12a_mmc_init);
	call_sub_init(meson_mmc_init);
	pr_debug("### %s() end\n", __func__);
	return 0;
}

static void __exit mmc_main_exit(void)
{
	g12a_mmc_exit();
	meson_mmc_exit();
}

module_init(mmc_main_init);
module_exit(mmc_main_exit);

MODULE_DESCRIPTION("Amlogic Meson SD/eMMC driver");
MODULE_AUTHOR("Kevin Hilman <khilman@baylibre.com>");
MODULE_LICENSE("GPL v2");
