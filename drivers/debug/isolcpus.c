// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#if ((defined CONFIG_ARM64) || defined(CONFIG_ARM)) && defined(CONFIG_KPROBES)
#include <linux/irqflags.h>
#include <linux/moduleparam.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/cpumask.h>
#include <linux/cpuset.h>
#include <linux/sched/isolation.h>
#include <linux/amlogic/gki_module.h>

static int have_isolcpus;
static int have_aml_isolcpus;
static int have_isolcpus_speedup_boot;
static struct cpumask aml_house_keeping_mask;
static int isolcpus_speedup_boot;

#ifdef CONFIG_ARM64
#define REG_0 regs->regs[0]
#define REG_LR regs->regs[30]
#endif

#ifdef CONFIG_ARM
#define REG_0 regs->ARM_r0
#define REG_LR regs->ARM_lr
#endif

static int __nocfi __kprobes
housekeeping_cpumask_pre_handler(struct kprobe *p, struct pt_regs *regs)
{
	enum hk_flags flags = REG_0;

	if (flags == HK_FLAG_DOMAIN) {
		if (isolcpus_speedup_boot)
			REG_0 = (unsigned long)cpu_possible_mask;
		else
			REG_0 = (unsigned long)&aml_house_keeping_mask;

		//skip origin function
		instruction_pointer_set(regs, REG_LR);

		//no single-step
		return 1;
	}

	//continue with origin function
	return 0;
}

static struct kprobe kp_housekeeping_cpumask = {
	.symbol_name = "housekeeping_cpumask",
	.pre_handler = housekeeping_cpumask_pre_handler,
};

static int isolcpus_setup(char *str)
{
	have_isolcpus = 1;

	return 0;
}
__setup("isolcpus=", isolcpus_setup);

static int aml_isolcpus_setup(char *str)
{
	struct cpumask isolcpus_mask;

	have_aml_isolcpus = 1;
	if (cpulist_parse(str, &isolcpus_mask) < 0) {
		pr_err("aml_isolcpus: bad arg:%s\n", str);
		return -EINVAL;
	}

	cpumask_andnot(&aml_house_keeping_mask, cpu_possible_mask, &isolcpus_mask);

	//pr_info("aml_isolcpus_setup() mask=%lx\n", *(unsigned long*)&aml_house_keeping_mask);
	return 0;
}
__setup("aml_isolcpus=", aml_isolcpus_setup);

static int isolcpus_speedup_boot_set(const char *val, const struct kernel_param *kp)
{
	int n = 0, ret;

	ret = kstrtoint(val, 10, &n);
	if (ret != 0) {
		pr_err("bad isolcpus_speedup_boot args:%s\n", val);
		return -EINVAL;
	}
	param_set_int(val, kp);

	//pr_info("isolcpus_speedup_boot=%d\n", isolcpus_speedup_boot);

	rebuild_sched_domains();

	return 0;
}

static const struct kernel_param_ops isolcpus_speedup_boot_ops = {
	.set = isolcpus_speedup_boot_set,
	.get = param_get_int,
};
module_param_cb(isolcpus_speedup_boot, &isolcpus_speedup_boot_ops, &isolcpus_speedup_boot, 0644);

static int isolcpus_speedup_boot_setup(char *str)
{
	have_isolcpus_speedup_boot = 1;

	if (kstrtoint(str, 0, &isolcpus_speedup_boot)) {
		pr_err("isolcpus_speedup_boot bad arg:%s\n", str);
		return -EINVAL;
	}

	return 0;
}
__setup("isolcpus_speedup_boot=", isolcpus_speedup_boot_setup);

int aml_isolcpus_init(void)
{
	int ret;

	if (have_isolcpus && (have_aml_isolcpus || have_isolcpus_speedup_boot)) {
		WARN(1, "Please use aml_isolcpus instead of isolcpus bootargs!\n");
		return 0;
	}

	if (!have_aml_isolcpus)
		return 0;

	ret = register_kprobe(&kp_housekeeping_cpumask);
	if (ret) {
		pr_err("register_kprobe: kp_housekeeping_cpumask failed:%d\n", ret);
		return 1;
	}

	//if need do really isolcpus, so rebuild domains
	if (!isolcpus_speedup_boot)
		rebuild_sched_domains();

	return 0;
}
#else
int aml_isolcpus_init(void)
{
	return 0;
}
#endif
