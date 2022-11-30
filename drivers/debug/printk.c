// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/clk.h>
#include <linux/console.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/serial.h>
#include <linux/serial_core.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/pinctrl/consumer.h>
#include <linux/trace_events.h>
#include <trace/hooks/printk.h>
#include <linux/irqflags.h>

static bool print_task_name_bool;
/* notice: this param only used by build module */
module_param(print_task_name_bool, bool, 0664);
MODULE_PARM_DESC(print_task_name_bool, "\n statistical log printing\n");

void printk_caller_id(void *data, u32 *caller_id)
{
	unsigned long irqflags;
	unsigned long preempt_value = preempt_count();
	u8 cpu = raw_smp_processor_id();
	u8 irq_trace;

	local_save_flags(irqflags);
	irq_trace =
		(irqs_disabled_flags(irqflags) ? TRACE_FLAG_IRQS_OFF : 0) |
		((preempt_value & NMI_MASK) ? TRACE_FLAG_NMI     : 0) |
		((preempt_value & HARDIRQ_MASK) ? TRACE_FLAG_HARDIRQ : 0) |
		((preempt_value & SOFTIRQ_OFFSET) ? TRACE_FLAG_SOFTIRQ : 0) |
		(tif_need_resched() ? TRACE_FLAG_NEED_RESCHED : 0) |
		(test_preempt_need_resched() ? TRACE_FLAG_PREEMPT_RESCHED : 0);

	*caller_id = (cpu << 28 | irq_trace << 20 | current->pid);
}

void printk_caller(void *data, char *caller, size_t size, u32 id, int *ret)
{
	struct task_struct *task = NULL;
	char task_name[TASK_COMM_LEN] = "";
	u8 cpu = id >> 28;
	u8 irq_trace = (id >> 20) & 0xff;
	pid_t pid = id & 0xfffff;

	char hardsoft_irq;
	char irqs_off;
	int hardirq;
	int softirq;
	int nmi;

	nmi = irq_trace & TRACE_FLAG_NMI;
	hardirq = irq_trace & TRACE_FLAG_HARDIRQ;
	softirq = irq_trace & TRACE_FLAG_SOFTIRQ;

	irqs_off =
		(irq_trace & TRACE_FLAG_IRQS_OFF) ? 'd' :
		(irq_trace & TRACE_FLAG_IRQS_NOSUPPORT) ? 'X' :
		'.';

	hardsoft_irq =
		(nmi && hardirq)     ? 'Z' :
		nmi                  ? 'z' :
		(hardirq && softirq) ? 'H' :
		hardirq              ? 'h' :
		softirq              ? 's' :
				       '.';

	if (print_task_name_bool) {
		rcu_read_lock();
		task = find_task_by_vpid(pid);
		rcu_read_unlock();
		if (task)
			__get_task_comm(task_name, TASK_COMM_LEN, task);
	} else {
		sprintf(task_name, "T%u", pid);
	}

	*ret = snprintf(caller, size, "%d %-6.6s %c%c",
				cpu,
				task_name,
				irqs_off, hardsoft_irq);
}

#if defined(CONFIG_TRACEPOINTS) && defined(CONFIG_ANDROID_VENDOR_HOOKS)
int printk_vendor_hook_init(void)
{
	int ret;

	ret = register_trace_android_vh_printk_caller_id(printk_caller_id, NULL);
	if (ret)
		pr_err("register_trace_android_vh_printk_caller_id fail ret=%d\n", ret);

	ret = register_trace_android_vh_printk_caller(printk_caller, NULL);
	if (ret)
		pr_err("register_trace_android_vh_printk_caller fail ret=%d\n", ret);

	return ret;
}

void printk_vendor_hook_exit(void)
{
	int ret;

	ret = unregister_trace_android_vh_printk_caller_id(printk_caller_id, NULL);
	if (ret)
		pr_err("unregister_trace_android_vh_printk_caller_id fail ret=%d\n", ret);

	ret = unregister_trace_android_vh_printk_caller(printk_caller, NULL);
	if (ret)
		pr_err("unregister_trace_android_vh_printk_caller fail ret=%d\n", ret);
}
#else
int printk_vendor_hook_init(void)
{
	return 0;
}

void printk_vendor_hook_exit(void)
{
}

void trace_android_vh_printk_caller_id(u32 *caller_id)
{
	printk_caller_id(NULL, caller_id);
}
EXPORT_SYMBOL(trace_android_vh_printk_caller_id);

void trace_android_vh_printk_caller(char *caller, size_t size, u32 id, int *ret)
{
	printk_caller(NULL, caller, size, id, ret);
}
EXPORT_SYMBOL(trace_android_vh_printk_caller);

#endif /* END CONFIG_ANDROID_VENDOR_HOOKS */

#if IS_BUILTIN(CONFIG_AMLOGIC_DEBUG_PRINTK)
early_initcall(printk_vendor_hook_init);
module_exit(printk_vendor_hook_exit);
#endif
