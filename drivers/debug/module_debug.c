// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/delay.h>
#include <linux/of.h>

#define MAX_SYMBOL_LEN    64
static char symbol[MAX_SYMBOL_LEN] = "module_memfree";

int aml_iotrace_en;
EXPORT_SYMBOL(aml_iotrace_en);

/* For each probe you need to allocate a kprobe structure */
static struct kprobe kp = {
	.symbol_name = symbol,
};

/* kprobe pre_handler: called just before the probed instruction is executed */
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
	struct device_node *node;
	const char *status;

	node = of_find_node_by_path("/reserved-memory/linux,iotrace");
	if (!node)
		return -EINVAL;

	ret = of_property_read_string(node, "status", &status);
	if (ret)
		return -EINVAL;

	if (strcmp(status, "okay"))
		return 0;

	aml_iotrace_en = 1;

	kp.pre_handler = handler_pre;

	ret = register_kprobe(&kp);
	if (ret < 0) {
		pr_err("register_kprobe failed, returned %d\n", ret);
		return ret;
	}

	pr_info("kprobe at %s\n", symbol);
	return 0;
}
