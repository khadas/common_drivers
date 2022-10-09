// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */
#include <linux/module.h>
#include "main.h"

static int __init debug_main_init(void)
{
#ifdef CONFIG_AMLOGIC_DEBUG_LOCKUP
	debug_lockup_init();
	cpu_mhz_init();
#endif
#ifdef CONFIG_AMLOGIC_DEBUG_ATRACE
	meson_atrace_init();
#endif

	return 0;
}

static void __exit debug_main_exit(void)
{
}

module_init(debug_main_init);
module_exit(debug_main_exit);

MODULE_LICENSE("GPL v2");
