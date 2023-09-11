// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

//#define DEBUG
#include <linux/module.h>
#include <linux/amlogic/module_merge.h>
#include "main.h"

int bypass_power_off;

static int bypass_power_off_setup(char *buf)
{
	if (!buf)
		return -EINVAL;

	if (kstrtoint(buf, 0, &bypass_power_off)) {
		pr_err("bypass_power_off error: %s\n", buf);
		return -EINVAL;
	}

	return 0;
}
__setup("bypass_power_off=", bypass_power_off_setup);

static int __init domain_main_init(void)
{
	pr_debug("### %s() start\n", __func__);
	call_sub_init(sec_power_domain_init);
	call_sub_init(power_ee_domain_init);
	call_sub_init(meson_pmic6b_bat_init);
	pr_debug("### %s() end\n", __func__);

	return 0;
}

static void __exit domain_main_exit(void)
{
	sec_power_domain_exit();
	power_ee_domain_exit();
	meson_pmic6b_bat_exit();
}

module_init(domain_main_init);
module_exit(domain_main_exit);
MODULE_LICENSE("GPL v2");
