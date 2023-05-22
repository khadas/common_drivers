// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

//#define DEBUG
#include <linux/module.h>
#include <linux/amlogic/module_merge.h>
#include "main.h"

static int __init aml_seckey_main_init(void)
{
	pr_debug("### %s() start\n", __func__);
	call_sub_init(aml_seckey_kt_init);
	call_sub_init(aml_seckey_kl_init);
	pr_debug("### %s() end\n", __func__);
	return 0;
}

static void __exit aml_seckey_main_exit(void)
{
	aml_seckey_kl_exit();
	aml_seckey_kt_exit();
}

module_init(aml_seckey_main_init);
module_exit(aml_seckey_main_exit);

MODULE_LICENSE("GPL v2");
