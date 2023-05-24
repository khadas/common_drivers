// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/irqflags.h>
#include <linux/moduleparam.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/stop_machine.h>
#ifdef CONFIG_ARM64
#include <asm/daifflags.h>
#endif

static DEFINE_PER_CPU(unsigned long, kprobe_busy_flags);

static int kprobe_busy_disable_irq;
module_param(kprobe_busy_disable_irq, int, 0644);

static int __nocfi __kprobes kprobe_busy_begin_pre_handler(struct kprobe *p, struct pt_regs *regs)
{
	if (kprobe_busy_disable_irq) {
#ifdef CONFIG_ARM64
		//save and disable irq
		__this_cpu_write(kprobe_busy_flags, regs->pstate & DAIF_MASK);
		regs->pstate |= DAIF_MASK;
		//directly return to lr (eg: kretprobe_trampoline_handler)
		instruction_pointer_set(regs, regs->regs[30]);
#endif
#ifdef CONFIG_ARM
		//save and disable irq
		__this_cpu_write(kprobe_busy_flags, regs->ARM_cpsr & PSR_I_BIT);
		regs->ARM_cpsr |= PSR_I_BIT;
		//directly return to lr (eg: kretprobe_trampoline_handler)
		instruction_pointer_set(regs, regs->ARM_lr);
#endif

		//no need continue do single-step
		return 1;
	}

	//if kprobe_busy_disable_irq not enable, continue origin function
	return 0;
}

static struct kprobe kp_kprobe_busy_begin = {
	.symbol_name = "kprobe_busy_begin",
	.pre_handler = kprobe_busy_begin_pre_handler,
};

static int __nocfi __kprobes kprobe_busy_end_pre_handler(struct kprobe *p, struct pt_regs *regs)
{
	if (kprobe_busy_disable_irq) {
		unsigned long flags = __this_cpu_read(kprobe_busy_flags);

#ifdef CONFIG_ARM64
		//restore irq
		regs->pstate &= ~DAIF_MASK;
		regs->pstate |= flags;
		//directly return to lr (eg: kretprobe_trampoline_handler)
		instruction_pointer_set(regs, regs->regs[30]);
#endif
#ifdef CONFIG_ARM
		//restore irq
		regs->ARM_cpsr &= ~PSR_I_BIT;
		regs->ARM_cpsr |= flags;
		//directly return to lr (eg: kretprobe_trampoline_handler)
		instruction_pointer_set(regs, regs->ARM_lr);
#endif
		//no need continue do single-step
		return 1;
	}

	//if kprobe_busy_disable_irq not enable, continue origin function
	return 0;
}

static struct kprobe kp_kprobe_busy_end = {
	.symbol_name = "kprobe_busy_end",
	.pre_handler = kprobe_busy_end_pre_handler,
};

int aml_kprobes_init(void)
{
	int ret;

	ret = register_kprobe(&kp_kprobe_busy_begin);
	if (ret) {
		pr_err("register_kprobe: kp_kprobe_busy_begin failed:%d\n", ret);
		return 1;
	}

	ret = register_kprobe(&kp_kprobe_busy_end);
	if (ret) {
		pr_err("register_kprobe: kp_kprobe_busy_end failed:%d\n", ret);
		unregister_kprobe(&kp_kprobe_busy_begin);
		return 1;
	}

	kprobe_busy_disable_irq	= 1;

	return 0;
}
