// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

//#define DEBUG
#include <linux/module.h>
#include <linux/amlogic/module_merge.h>
#include "main.h"

static int __init wireless_main_init(void)
{
	pr_debug("### %s() start\n", __func__);
	call_sub_init(wifi_dt_init);
	call_sub_init(bt_init);
	pr_debug("### %s() end\n", __func__);
	return 0;
}

static void __exit wireless_main_exit(void)
{
	bt_exit();
	wifi_dt_exit();
}

module_init(wireless_main_init);
module_exit(wireless_main_exit);

MODULE_LICENSE("GPL v2");
