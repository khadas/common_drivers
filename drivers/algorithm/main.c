// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

//#define ALGORITHM
#include <linux/module.h>
#include <linux/amlogic/module_merge.h>
#include "main.h"

static int __init algorithm_main_init(void)
{
	pr_debug("### %s() start\n", __func__);
	call_sub_init(aml_dnlp_alg_init);
	call_sub_init(aad_alg_driver_init);
	//call_sub_init(bls_alg_init);
	call_sub_init(cabc_alg_driver_init);
	call_sub_init(color_tune_init);
	//call_sub_init(cuva_alg_driver_init);
	call_sub_init(hdr10_tmo_alg_driver_init);
	call_sub_init(pregm_alg_driver_init);
	pr_debug("### %s() end\n", __func__);
	return 0;
}

static void __exit algorithm_main_exit(void)
{
	pregm_alg_driver_exit();
	hdr10_tmo_alg_driver_exit();
	//cuva_alg_driver_exit();
	color_tune_exit();
	cabc_alg_driver_exit();
	//bls_alg_exit();
	aad_alg_driver_exit();
	aml_dnlp_alg_exit();
}

module_init(algorithm_main_init);
module_exit(algorithm_main_exit);

MODULE_LICENSE("GPL v2");
