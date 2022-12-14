// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

//#define DEBUG
#include <linux/module.h>
#include <linux/amlogic/module_merge.h>
#include "main.h"

static int __init led_main_init(void)
{
	pr_debug("### %s() start\n", __func__);
	call_sub_init(led_tlc59116_init);
	call_sub_init(led_aw9523_init);
	call_sub_init(led_state_init);
	call_sub_init(led_unipolar_ctrl_init);
	pr_debug("### %s() end\n", __func__);
	return 0;
}

static void __exit led_main_exit(void)
{
	led_state_exit();
	led_aw9523_init();
	led_tlc59116_init();
	led_unipolar_ctrl_exit();
}

module_init(led_main_init);
module_exit(led_main_exit);

MODULE_LICENSE("GPL v2");
