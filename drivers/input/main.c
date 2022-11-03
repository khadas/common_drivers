// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

//#define DEBUG
#include <linux/module.h>
#include <linux/amlogic/module_merge.h>
#include "main.h"

static int __init input_main_init(void)
{
	pr_debug("### %s() start\n", __func__);
	call_sub_init(meson_gpio_kp_init);
	call_sub_init(meson_adc_kp_init);
	call_sub_init(meson_ir_driver_init);
	pr_debug("### %s() end\n", __func__);
	return 0;
}

static void __exit input_main_exit(void)
{
	meson_ir_driver_exit();
	meson_adc_kp_exit();
	meson_gpio_kp_exit();
}

module_init(input_main_init);
module_exit(input_main_exit);

MODULE_LICENSE("GPL v2");
