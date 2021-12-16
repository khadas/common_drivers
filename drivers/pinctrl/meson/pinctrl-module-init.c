// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifdef MODULE

#include <linux/platform_device.h>
#include <linux/module.h>

static int __init pinctrl_module_init(void)
{
	return 0;
}

static void __exit pinctrl_module_exit(void)
{
}

module_init(pinctrl_module_init);
module_exit(pinctrl_module_exit);

MODULE_LICENSE("GPL v2");
#endif
