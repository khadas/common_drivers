// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

//#define DEBUG
#include <linux/module.h>
#include <linux/amlogic/module_merge.h>
#include "main.h"

static int __init dvb_main_init(void)
{
	pr_debug("### %s() start\n", __func__);
	call_sub_init(aml_dvb_extern_init);
	call_sub_init(aucpu_init);
	call_sub_init(dsm_init);
	call_sub_init(smc_sc2_mod_init);
	pr_debug("### %s() end\n", __func__);
	return 0;
}

static void __exit dvb_main_exit(void)
{
	smc_sc2_mod_exit();
	dsm_exit();
	aucpu_exit();
	aml_dvb_extern_exit();
}
module_init(dvb_main_init);
module_exit(dvb_main_exit);
MODULE_LICENSE("GPL v2");
