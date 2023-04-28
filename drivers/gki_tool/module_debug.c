// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/amlogic/gki_module.h>

static struct kprobe kp = {
	.symbol_name = "module_memfree",
};

static int handler_pre(struct kprobe *p, struct pt_regs *regs)
{
	/*
	 * set module_memfree x0 to 0, do not free module init section
	 * it's very important for iotrace function
	 */
#if defined(CONFIG_ARM64)
	regs->regs[0] = 0;
#elif defined(CONFIG_ARM)
	regs->ARM_r0 = 0;
#endif

	return 0;
}

int module_debug_init(void)
{
	int ret;
	int i;

	for (i = 0 ; i < cpv_count; i++) {
		if (!strcmp(cpv[i].param, "ramoops_io_en") && !strcmp(cpv[i].val, "1")) {
			kp.pre_handler = handler_pre;

			ret = register_kprobe(&kp);
			if (ret < 0) {
				pr_err("register_kprobe failed, returned %d\n", ret);
				return ret;
			}
		}
	}

	return 0;
}
