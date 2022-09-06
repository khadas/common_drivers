// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */
#include <linux/stacktrace.h>
#include <linux/export.h>
#include <linux/types.h>
#include <linux/smp.h>
#include <linux/irqflags.h>
#include <linux/sched.h>
#include <linux/moduleparam.h>
#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/sched/clock.h>
#include <linux/sched/debug.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/stacktrace.h>
#include <linux/arm-smccc.h>
#include <linux/kprobes.h>
#include <trace/events/irq.h>
#include <trace/hooks/cpuidle.h>
#include <trace/hooks/dtask.h>
#include <trace/hooks/sched.h>
#include <linux/time.h>
#include <linux/delay.h>
#include "../../../kernel/sched/sched.h"
#include <linux/amlogic/debug_irqflags.h>
#include <linux/amlogic/debug_ftrace_ramoops.h>

/*isr trace*/
#define ns2us			(1000)
#define ns2ms			(1000 * 1000)
#define LONG_ISR		(500 * ns2ms)
#define LONG_SIRQ		(500 * ns2ms)
#define CHK_WINDOW		(1000 * ns2ms)
#define IRQ_CNT			256
#define CCCNT_WARN		15000
/*irq disable trace*/
#define LONG_IRQDIS		(1000 * 1000000)	/*1000 ms*/
#define OUT_WIN			(500 * 1000000)		/*500 ms*/
#define LONG_IDLE		(5000000000ULL)		/*5 sec*/
#define LONG_SMC		(500 * 1000000)		/*500 ms*/
#define ENTRY			10
#define INVALID_IRQ             -1
#define INVALID_SIRQ            -1

static unsigned long isr_thr = LONG_ISR;
core_param(isr_thr, isr_thr, ulong, 0644);

static unsigned long sirq_thr = LONG_SIRQ;
core_param(sirq_thr, sirq_thr, ulong, 0644);

static unsigned long long idle_thr = LONG_IDLE;
core_param(idle_thr, idle_thr, ullong, 0644);

static unsigned long smc_thr = LONG_SMC;
core_param(smc_thr, smc_thr, ulong, 0644);

static int isr_check_en = 1;
core_param(isr_check_en, isr_check_en, int, 0644);

static int sirq_check_en = 1;
core_param(sirq_check_en, sirq_check_en, int, 0644);

static int idle_check_en = 1;
core_param(idle_check_en, idle_check_en, int, 0644);

static int smc_check_en = 1;
core_param(smc_check_en, smc_check_en, int, 0644);

static unsigned long irq_disable_thr = LONG_IRQDIS;
core_param(irq_disable_thr, irq_disable_thr, ulong, 0644);

static int irq_check_en;
core_param(irq_check_en, irq_check_en, int, 0644);

static int initialized;

struct isr_check_info {
	unsigned long long period_start_time;
	unsigned long long exec_start_time;
	unsigned long long exec_sum_time;
	unsigned int cnt;
};

struct lockup_info {
	/* isr check */
	struct isr_check_info isr_infos[IRQ_CNT];
	int curr_irq;
	struct irqaction *curr_irq_action;

	/* sirq check */
	unsigned long long sirq_enter_time;
	int curr_sirq;

	/* idle check */
	unsigned long long idle_enter_time;

	/* smc check */
	unsigned long long smc_enter_time;
	unsigned long curr_smc_a0;
	unsigned long curr_smc_a1;
	unsigned long curr_smc_a2;
	unsigned long curr_smc_a3;
	unsigned long curr_smc_a4;
	unsigned long curr_smc_a5;
	unsigned long curr_smc_a6;
	unsigned long curr_smc_a7;
	unsigned long smc_enter_trace_entries[ENTRY];
	int smc_enter_trace_entries_nr;

	/* irq disable check */
	unsigned long long irq_disable_time;
	unsigned long irq_disable_trace_entries[ENTRY];
	int irq_disable_trace_entries_nr;
};

static struct lockup_info __percpu *infos;

static void isr_in_hook(void *data, int irq, struct irqaction *action)
{
	struct lockup_info *info;
	struct isr_check_info *isr_info;
	int cpu;
	unsigned long long now;

	if (irq >= IRQ_CNT || !isr_check_en)
		return;

	cpu = smp_processor_id();

	info = per_cpu_ptr(infos, cpu);
	info->curr_irq = irq;
	info->curr_irq_action = action;

	isr_info = &info->isr_infos[irq];
	now = sched_clock();
	isr_info->exec_start_time = now;

	if (now >= CHK_WINDOW + isr_info->period_start_time) {
		isr_info->period_start_time = now;
		isr_info->exec_sum_time = 0;
		isr_info->cnt = 0;
	}
}

static void isr_out_hook(void *data, int irq, struct irqaction *action, int ret)
{
	struct lockup_info *info;
	struct isr_check_info *isr_info;
	int cpu;
	unsigned long long now, delta, this_period_time;

	if (irq >= IRQ_CNT || !isr_check_en)
		return;

	cpu = smp_processor_id();

	info = per_cpu_ptr(infos, cpu);
	info->curr_irq = INVALID_IRQ;

	isr_info = &info->isr_infos[irq];
	if (!isr_info->exec_start_time)
		return;

	now = sched_clock();
	delta = now - isr_info->exec_start_time;
	isr_info->exec_start_time = 0;

	isr_info->exec_sum_time += delta;
	isr_info->cnt++;

	if (delta > isr_thr)
		pr_err("ISR_Long___ERR. irq:%d/%s action=%ps exec_time:%llums\n",
		       irq, action->name, action->handler, div_u64(delta, ns2ms));

	this_period_time = now - isr_info->period_start_time;
	if (this_period_time < CHK_WINDOW)
		return;

	if (isr_info->exec_sum_time * 100 >= 50 * this_period_time ||
	    isr_info->cnt > CCCNT_WARN) {
		pr_err("IRQRatio___ERR.irq:%d/%s action=%ps ratio:%llu\n",
		       irq, action->name, action->handler,
		       div_u64(isr_info->exec_sum_time * 100, this_period_time));

		pr_err("period_time:%llums isr_sum_time:%llums, cnt:%d, last_exec_time:%lluus\n",
		       div_u64(this_period_time, ns2ms),
		       div_u64(isr_info->exec_sum_time, ns2ms),
		       isr_info->cnt,
		       div_u64(delta, ns2us));
	}

	isr_info->period_start_time = now;
	isr_info->exec_sum_time = 0;
	isr_info->cnt = 0;
}

static void softirq_in_hook(void *data, unsigned int vec_nr)
{
	int cpu;
	struct lockup_info *info;

	if (!sirq_check_en)
		return;

	cpu = smp_processor_id();
	info = per_cpu_ptr(infos, cpu);

	info->curr_sirq = vec_nr;
	info->sirq_enter_time = sched_clock();
}

static void softirq_out_hook(void *data, unsigned int vec_nr)
{
	int cpu;
	unsigned long long delta;
	struct lockup_info *info;

	if (!sirq_check_en)
		return;

	cpu = smp_processor_id();
	info = per_cpu_ptr(infos, cpu);

	info->curr_sirq = INVALID_SIRQ;
	if (!info->sirq_enter_time)
		return;

	delta = sched_clock() - info->sirq_enter_time;
	if (delta > sirq_thr)
		pr_err("SIRQLong___ERR. sirq:%d exec_time:%llu ms\n",
		       vec_nr, div_u64(delta, ns2ms));

	info->sirq_enter_time = 0;
}

static void idle_in_hook(void *data, int *state, struct cpuidle_device *dev)
{
	int cpu;
	struct lockup_info *info;

	if (!idle_check_en)
		return;

	cpu = smp_processor_id();
	info = per_cpu_ptr(infos, cpu);
	info->idle_enter_time = sched_clock();
}

static void idle_out_hook(void *data, int state, struct cpuidle_device *dev)
{
	int cpu;
	unsigned long long delta;
	struct lockup_info *info;

	if (!idle_check_en)
		return;

	cpu = smp_processor_id();
	info = per_cpu_ptr(infos, cpu);

	if (!info->idle_enter_time)
		return;

	delta = sched_clock() - info->idle_enter_time;
	if (delta > idle_thr)
		pr_err("IDLELong___ERR. state:%d idle_time:%llu ms\n",
		       state, div_u64(delta, ns2ms));

	info->idle_enter_time = 0;
}

static unsigned long smcid_skip_list[] = {
	0x84000001, /* suspend A32*/
	0xC4000001, /* suspend A64*/
	0x84000002, /* cpu off */
	0x84000008, /* system off */
	0x84000009, /* system reset */
	0x8400000E, /* system suspend A32 */
	0xC400000E, /* system suspend A64 */
};

static bool notrace is_noret_smcid(unsigned long smcid)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(smcid_skip_list); i++) {
		if (smcid == smcid_skip_list[i])
			return true;
	}

	return false;
}

static void smc_in_hook(unsigned long smcid, unsigned long val, bool noret)
{
	int cpu;
	struct lockup_info *info;

	if (noret) {
		pstore_ftrace_io_smc_noret_in(smcid, val);
		return;
	}

	pstore_ftrace_io_smc_in(smcid, val);

	if (!initialized || !smc_check_en)
		return;

	cpu = smp_processor_id();
	info = per_cpu_ptr(infos, cpu);

	info->smc_enter_time = sched_clock();
	info->curr_smc_a0 = smcid;
	info->curr_smc_a1 = val;

	memset(info->smc_enter_trace_entries, 0, sizeof(info->smc_enter_trace_entries));
	info->smc_enter_trace_entries_nr = stack_trace_save(info->smc_enter_trace_entries, ENTRY, 0);
}

static void smc_out_hook(unsigned long smcid, unsigned long val)
{
	int cpu;
	unsigned long rem_nsec;
	unsigned long long ts, delta;
	struct lockup_info *info;

	pstore_ftrace_io_smc_out(smcid, val);

	if (!initialized || !smc_check_en)
		return;

	cpu = smp_processor_id();
	info = per_cpu_ptr(infos, cpu);

	if (!info->smc_enter_time)
		return;

	delta = sched_clock() - info->smc_enter_time;
	if (delta > smc_thr) {
		ts = info->smc_enter_time;
		rem_nsec = do_div(ts, 1000000000);

		pr_err("SMCLong___ERR. smc_time:%llu ms(%lx %lx), entered at: %llu.%06lu\n",
		       div_u64(delta, ns2ms),
		       info->curr_smc_a0,
		       info->curr_smc_a1,
		       ts, rem_nsec / 1000);

		stack_trace_print(info->smc_enter_trace_entries, info->smc_enter_trace_entries_nr, 0);
		dump_stack();
	}

	info->smc_enter_time = 0;

}

static void irq_trace_start(unsigned long flags)
{
	int cpu, softirq;
	struct lockup_info *info;

	if (!irq_check_en || oops_in_progress)
		return;

	if (arch_irqs_disabled_flags(flags))
		return;

	cpu = smp_processor_id();
	info = per_cpu_ptr(infos, cpu);

	softirq = task_thread_info(current)->preempt_count & SOFTIRQ_MASK;
	if ((!current->pid && !softirq) ||
	    info->idle_enter_time ||
	    cpu_is_offline(cpu) ||
	    (softirq_count() && info->sirq_enter_time))
		return;

	info->irq_disable_time = sched_clock();

	memset(info->irq_disable_trace_entries, 0, sizeof(info->irq_disable_trace_entries));
	info->irq_disable_trace_entries_nr = stack_trace_save(info->irq_disable_trace_entries, ENTRY, 0);
}

static void irq_trace_stop(unsigned long flags)
{
	int cpu, softirq;
	struct lockup_info *info;
	unsigned long long ts, delta;
	unsigned long rem_nsec;

	if (!irq_check_en || oops_in_progress)
		return;

	if (arch_irqs_disabled_flags(flags))
		return;

	cpu = smp_processor_id();
	info = per_cpu_ptr(infos, cpu);

	if (!info->irq_disable_time || !arch_irqs_disabled_flags(arch_local_save_flags()))
		return;

	softirq =  task_thread_info(current)->preempt_count & SOFTIRQ_MASK;
	delta = sched_clock() - info->irq_disable_time;

	if (delta > irq_disable_thr &&
	    !(!current->pid && !softirq) &&
	    !(softirq_count() && info->sirq_enter_time)) {
		ts = info->irq_disable_time;
		rem_nsec = do_div(ts, 1000000000);
		pr_err("\n\nDisIRQ___ERR:%llums, disabled at: %llu.%06lu\n",
		       div_u64(delta, ns2ms), ts, rem_nsec / 1000);

		stack_trace_print(info->irq_disable_trace_entries, info->irq_disable_trace_entries_nr, 0);
		dump_stack();
	}

	info->irq_disable_time = 0;
}

irq_trace_fn_t irq_trace_start_hook;
EXPORT_SYMBOL(irq_trace_start_hook);

irq_trace_fn_t irq_trace_stop_hook;
EXPORT_SYMBOL(irq_trace_stop_hook);

static void sched_show_task_hook(void *data, struct task_struct *p)
{
	unsigned long long ts;
	unsigned long rem_nsec;

	ts = p->se.exec_start;
	rem_nsec = do_div(ts, 1000000000);

	pr_info("task:%s/%d on_cpu=%d prio=%d exec_start=%llu.%06lu sum_exec_runtime=%llums load_avg=%lu runnable_avg=%lu util_avg=%lu\n",
		p->comm, p->pid,
		p->on_cpu,
		p->prio,
		ts, rem_nsec / 1000,
		div_u64(p->se.sum_exec_runtime, ns2ms),
		p->se.avg.load_avg,
		p->se.avg.runnable_avg,
		p->se.avg.util_avg);
}

void pr_lockup_info(int lock_cpu)
{
	int cpu;
	unsigned long flags;
	struct lockup_info *info;
	unsigned long long delta, ts;
	unsigned long rem_nsec;

	local_irq_save(flags);
	irq_check_en = 0;
	isr_check_en = 0;
	sirq_check_en = 0;
	idle_check_en = 0;
	smc_check_en = 0;

	console_loglevel = 7;

	pr_err("\n");
	pr_err("\n");
	pr_err("pr_lockup_info: lock_cpu=[%d]  START -------------------------\n", lock_cpu);
	for_each_online_cpu(cpu) {
		pr_err("\n");
		pr_err("### cpu[%d]:\n", cpu);
		info = per_cpu_ptr(infos, cpu);

		if (info->curr_irq != INVALID_IRQ) {
			ts = info->isr_infos[info->curr_irq].exec_start_time;
			rem_nsec = do_div(ts, 1000000000);

			pr_err("curr_irq:%d action=%s/%ps exec_start_time=%llu.%06lu\n",
			       info->curr_irq,
			       info->curr_irq_action->name,
			       info->curr_irq_action->handler,
			       ts, rem_nsec / 1000);
		}

		if (info->curr_sirq != INVALID_SIRQ) {
			ts = info->sirq_enter_time;
			rem_nsec = do_div(ts, 1000000000);

			pr_err("curr_sirq:%d sirq_enter_time=%llu.%06lu\n",
				info->curr_sirq,
				ts, rem_nsec / 1000);
		}

		if (info->idle_enter_time) {
			ts = info->idle_enter_time;
			rem_nsec = do_div(ts, 1000000000);

			pr_err("in idle, idle_enter_time=%llu.%06lu\n", ts, rem_nsec / 1000);
		}

		if (info->smc_enter_time && !info->idle_enter_time) {
			ts = info->smc_enter_time;
			rem_nsec = do_div(ts, 1000000000);

			pr_err("in smc, smc_enter_time=%llu.%06lu (%lx %lx %lx %lx %lx %lx %lx %lx)\n",
			       ts, rem_nsec / 1000,
			       info->curr_smc_a0,
			       info->curr_smc_a1,
			       info->curr_smc_a2,
			       info->curr_smc_a3,
			       info->curr_smc_a4,
			       info->curr_smc_a5,
			       info->curr_smc_a6,
			       info->curr_smc_a7);

			stack_trace_print(info->smc_enter_trace_entries, info->smc_enter_trace_entries_nr, 0);
		}

		if (info->irq_disable_time) {
			delta = sched_clock() - info->irq_disable_time;
			ts = info->irq_disable_time;
			rem_nsec = do_div(ts, 1000000000);

			pr_err("in irq, disabled at: %llu.%06lu for %llums\n",
			       ts, rem_nsec / 1000, div_u64(delta, ns2ms));

			stack_trace_print(info->irq_disable_trace_entries, info->irq_disable_trace_entries_nr, 0);
		}

		dump_cpu_task(cpu);
	}

	pr_err("pr_lockup_info: lock_cpu=[%d] END ------------------------\n", lock_cpu);

	local_irq_restore(flags);
}
EXPORT_SYMBOL(pr_lockup_info);

static void rt_throttle_func(void *data, int cpu, u64 clock, ktime_t rt_period, u64 rt_runtime,
		s64 rt_period_timer_expires)
{
	u64 exec_runtime;
	u64 rt_time;
	struct rq *rq = cpu_rq(cpu);
	struct rt_rq *rt_rq = &rq->rt;

	exec_runtime = rq->curr->se.sum_exec_runtime;
	rt_time = rt_rq->rt_time;
	do_div(exec_runtime, 1000000);
	do_div(rt_time, 1000000);
	pr_warn("RT throttling on cpu:%d rt_time:%llums, curr:%s/%d prio:%d sum_runtime:%llums\n",
		cpu, rt_time, rq->curr->comm, rq->curr->pid,
		rq->curr->prio, exec_runtime);
}

void __arm_smccc_smc_glue(unsigned long a0, unsigned long a1,
			  unsigned long a2, unsigned long a3, unsigned long a4,
			  unsigned long a5, unsigned long a6, unsigned long a7,
			  struct arm_smccc_res *res, struct arm_smccc_quirk *quirk)
{
	smc_in_hook(a0, a1, is_noret_smcid(a0));
	__arm_smccc_smc(a0, a1, a2, a3, a4, a5, a6, a7, res, quirk);
	smc_out_hook(a0, a1);
}
EXPORT_SYMBOL(__arm_smccc_smc_glue);

int debug_lockup_init(void)
{
	int cpu;
	struct lockup_info *info;

	infos = alloc_percpu(struct lockup_info);
	if (!infos) {
		pr_err("alloc percpu infos failed\n");
		return 1;
	}

	for_each_possible_cpu(cpu) {
		info = per_cpu_ptr(infos, cpu);
		memset(info, 0, sizeof(*info));
		info->curr_irq = INVALID_IRQ;
		info->curr_sirq = INVALID_SIRQ;
	}

	register_trace_irq_handler_entry(isr_in_hook, NULL);
	register_trace_irq_handler_exit(isr_out_hook, NULL);

	register_trace_softirq_entry(softirq_in_hook, NULL);
	register_trace_softirq_exit(softirq_out_hook, NULL);

	register_trace_android_vh_cpu_idle_enter(idle_in_hook, NULL);
	register_trace_android_vh_cpu_idle_exit(idle_out_hook, NULL);

	register_trace_android_vh_sched_show_task(sched_show_task_hook, NULL);

	register_trace_android_vh_dump_throttled_rt_tasks(rt_throttle_func, NULL);

	irq_trace_start_hook = irq_trace_start;
	irq_trace_stop_hook = irq_trace_stop;

	initialized = 1;

	return 0;
}
