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
#ifdef CONFIG_AMLOGIC_DEBUG_TEST
#define KERNEL_ATRACE_TAG KERNEL_ATRACE_TAG_ALL
#include <trace/events/meson_atrace.h>
#endif
#include <trace/events/irq.h>
#include <trace/hooks/cpuidle.h>
#include <trace/hooks/dtask.h>
#include <trace/hooks/sched.h>
#include <trace/hooks/preemptirq.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <sched.h>
#include "trace.h"

#define CREATE_TRACE_POINTS
DEFINE_TRACE(inject_irq_hooks,
	     TP_PROTO(int dummy),
	     TP_ARGS(dummy));

#ifdef CONFIG_AMLOGIC_HARDLOCKUP_DETECTOR
DEFINE_TRACE(inject_pr_lockup_info,
	     TP_PROTO(int dummy),
	     TP_ARGS(dummy));
#endif

#ifdef CONFIG_AMLOGIC_DEBUG_FTRACE_PSTORE
DEFINE_TRACE(inject_pstore_io_save,
	     TP_PROTO(int dummy),
	     TP_ARGS(dummy));
#endif

/*isr trace*/
#define ns2us			(1000)
#define ns2ms			(1000 * 1000)
#define LONG_ISR		(500 * ns2ms)
#define LONG_SIRQ		(500 * ns2ms)
#define CHK_WINDOW		(1000 * ns2ms)
#define IRQ_CNT			256
#define CCCNT_WARN		15000
/*irq disable trace*/
#define LONG_IRQDIS		(500 * 1000000)	        /* 500 ms*/
#define OUT_WIN			(500 * 1000000)		/* 500 ms*/
#define LONG_IDLE		(5000000000ULL)		/* 5 sec*/
#define LONG_SMC		(500 * 1000000)		/* 500 ms*/
#define ENTRY			10
#define INVALID_IRQ	     -1
#define INVALID_SIRQ	    -1

static unsigned long isr_long_thr = LONG_ISR;
module_param(isr_long_thr, ulong, 0644);

static unsigned long isr_ratio_thr = 50;
module_param(isr_ratio_thr, ulong, 0644);

static unsigned long sirq_thr = LONG_SIRQ;
module_param(sirq_thr, ulong, 0644);

static unsigned long long idle_thr = LONG_IDLE;
module_param(idle_thr, ullong, 0644);

static unsigned long smc_thr = LONG_SMC;
module_param(smc_thr, ulong, 0644);

static int isr_check_en = 1;
module_param(isr_check_en, int, 0644);

static int sirq_check_en = 1;
module_param(sirq_check_en, int, 0644);

static int idle_check_en = 1;
module_param(idle_check_en, int, 0644);

static int smc_check_en = 1;
module_param(smc_check_en, int, 0644);

static unsigned long irq_disable_thr = LONG_IRQDIS;
module_param(irq_disable_thr, ulong, 0644);

static int irq_check_en;
module_param(irq_check_en, int, 0644);

static int initialized;

irq_trace_fn_t irq_trace_start_hook;
EXPORT_SYMBOL(irq_trace_start_hook);

irq_trace_fn_t irq_trace_stop_hook;
EXPORT_SYMBOL(irq_trace_stop_hook);

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

	if (delta > isr_long_thr)
		pr_err("ISR_Long___ERR. irq:%d/%s action=%ps exec_time:%llums\n",
		       irq, action->name, action->handler, div_u64(delta, ns2ms));

	this_period_time = now - isr_info->period_start_time;
	if (this_period_time < CHK_WINDOW)
		return;

	if (isr_info->exec_sum_time * 100 >= isr_ratio_thr * this_period_time ||
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

void softirq_in_hook(void *data, unsigned int vec_nr)
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

void softirq_out_hook(void *data, unsigned int vec_nr)
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

#ifdef CONFIG_AMLOGIC_DEBUG_TEST
static int idle_long_debug;
#endif

static void idle_in_hook(void *data, int *state, struct cpuidle_device *dev)
{
	int cpu;
	struct lockup_info *info;

	if (!idle_check_en)
		return;

	cpu = smp_processor_id();
	info = per_cpu_ptr(infos, cpu);
	info->idle_enter_time = sched_clock();

#ifdef CONFIG_AMLOGIC_DEBUG_TEST
	if (idle_long_debug) {
		idle_long_debug = 0;
		mdelay(5000);
	}
#endif
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

	if (noret)
		return;

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

#ifdef CONFIG_AMLOGIC_DEBUG_TEST
static int smc_long_debug;
#endif

void __arm_smccc_smc_glue(unsigned long a0, unsigned long a1,
			unsigned long a2, unsigned long a3, unsigned long a4,
			unsigned long a5, unsigned long a6, unsigned long a7,
			struct arm_smccc_res *res, struct arm_smccc_quirk *quirk)
{
	int not_in_idle = current->pid != 0;

#ifdef	CONFIG_AMLOGIC_DEBUG_TEST
	if (smc_long_debug) {
		smc_in_hook(a0, a1, is_noret_smcid(a0));
		smc_long_debug = 0;
		mdelay(1000);
		smc_out_hook(a0, a1);

		return;
	}
#endif
	if (not_in_idle)
		preempt_disable_notrace();

	smc_in_hook(a0, a1, is_noret_smcid(a0));
	__arm_smccc_smc(a0, a1, a2, a3, a4, a5, a6, a7, res, quirk);
	smc_out_hook(a0, a1);

	if (not_in_idle)
		preempt_enable_notrace();
}
EXPORT_SYMBOL(__arm_smccc_smc_glue);

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

void __dump_cpu_task(int cpu)
{
	pr_info("Task dump for CPU %d:\n", cpu);
	sched_show_task(cpu_curr(cpu));
}
void pr_lockup_info(int lock_cpu)
{
	int cpu;
	unsigned long flags;
	struct lockup_info *info;
	unsigned long long delta, ts;
	unsigned long rem_nsec;

	local_irq_save(flags);
	console_loglevel = 7;

	pr_err("\n");
	pr_err("\n");
	pr_err("%s: lock_cpu=[%d] irq_check_en=%d -------- START --------\n",
	       __func__, lock_cpu, irq_check_en);
	irq_check_en = 0;
	isr_check_en = 0;
	sirq_check_en = 0;
	idle_check_en = 0;
	smc_check_en = 0;

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

		__dump_cpu_task(cpu);
	}

	pr_err("%s: lock_cpu=[%d] --------- END --------\n", __func__, lock_cpu);

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

void debug_trace_container(void)
{
	trace_inject_irq_hooks(0);
#ifdef CONFIG_AMLOGIC_HARDLOCKUP_DETECTOR
	trace_inject_pr_lockup_info(0);
#endif
#ifdef CONFIG_AMLOGIC_DEBUG_FTRACE_PSTORE
	trace_inject_pstore_io_save(0);
#endif
}

static void irq_hooks_probe(void *data, int dummy)
{
}

void inject_irq_hooks(void)
{
	irq_trace_fn_t hooks[2];

	hooks[0] = irq_trace_start;
	hooks[1] = irq_trace_stop;

	register_trace_inject_irq_hooks(irq_hooks_probe, &hooks);
}

#ifdef CONFIG_AMLOGIC_HARDLOCKUP_DETECTOR
static void pr_lockup_info_probe(void *data, int dummy)
{
}

void inject_pr_lockup_info(void)
{
	register_trace_inject_pr_lockup_info(pr_lockup_info_probe, pr_lockup_info);
}
#endif

#ifdef CONFIG_AMLOGIC_DEBUG_FTRACE_PSTORE
static void (*pstore_io_save_hook)(unsigned long reg, unsigned long val,
				    unsigned long parent, unsigned int flag,
				    unsigned long *irq_flag);

void notrace pstore_io_save(unsigned long reg, unsigned long val,
			    unsigned long parent, unsigned int flag,
			    unsigned long *irq_flag)
{
	if (pstore_io_save_hook)
		pstore_io_save_hook(reg, val, parent, flag, irq_flag);
}
EXPORT_SYMBOL(pstore_io_save);

static void pstore_io_save_probe(void *data, int dummy)
{
}

void inject_pstore_io_save(void)
{
	register_trace_inject_pstore_io_save(pstore_io_save_probe, &pstore_io_save_hook);
}
#endif

#ifdef CONFIG_AMLOGIC_DEBUG_TEST
extern void lockup_test(void);
#endif

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

//	register_trace_softirq_entry(softirq_in_hook, NULL);
//	register_trace_softirq_exit(softirq_out_hook, NULL);

	register_trace_android_vh_cpu_idle_enter(idle_in_hook, NULL);
	register_trace_android_vh_cpu_idle_exit(idle_out_hook, NULL);

	register_trace_android_vh_sched_show_task(sched_show_task_hook, NULL);

	register_trace_android_vh_dump_throttled_rt_tasks(rt_throttle_func, NULL);

	irq_trace_start_hook = irq_trace_start;
	irq_trace_stop_hook = irq_trace_stop;
	inject_irq_hooks();

#ifdef CONFIG_AMLOGIC_HARDLOCKUP_DETECTOR
	inject_pr_lockup_info();
#endif

#ifdef CONFIG_AMLOGIC_DEBUG_FTRACE_PSTORE
	inject_pstore_io_save();
#endif

	/* CONFIG_IRQSOFF_TRACER is not enabled, can't use below function */
	//register_trace_android_rvh_irqs_disable(irq_trace_start, NULL);
	//register_trace_android_rvh_irqs_enable(irq_trace_stop, NULL);

	initialized = 1;

#ifdef CONFIG_AMLOGIC_DEBUG_TEST
	lockup_test();
#endif
	return 0;
}

#ifdef CONFIG_AMLOGIC_DEBUG_TEST
static int load_hrtimer_sleepus;
module_param(load_hrtimer_sleepus, int, 0644);
static int load_hrtimer_delayus;
module_param(load_hrtimer_delayus, int, 0644);
static int load_hrtimer_print;
static struct hrtimer load_hrtimer;

static enum hrtimer_restart do_load_hrtimer(struct hrtimer *timer)
{
	if (load_hrtimer_print)
		pr_info("do_loader_timer()\n");

	udelay(load_hrtimer_delayus);

	if (load_hrtimer_sleepus) {
		hrtimer_forward(timer, ktime_get(), ktime_set(0, load_hrtimer_sleepus * 1000));
		return HRTIMER_RESTART;
	}

	return HRTIMER_NORESTART;
}

static void load_hrtimer_start(void *info)
{
	hrtimer_start(&load_hrtimer, ktime_set(0, load_hrtimer_sleepus * 1000), HRTIMER_MODE_REL);
}

void load_hrtimer_test(int sleepus, int delayus, int cpu, int print)
{
	load_hrtimer_sleepus = sleepus;
	load_hrtimer_delayus = delayus;
	load_hrtimer_print = print;

	pr_emerg("%s: (%px) sleepus=%d delayus=%d cpu=%d print=%d\n",
		__func__,
		&load_hrtimer,
		load_hrtimer_sleepus,
		load_hrtimer_delayus,
		cpu,
		print);

	hrtimer_init(&load_hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	load_hrtimer.function = do_load_hrtimer;

	smp_call_function_single(cpu, load_hrtimer_start, NULL, 1);
}

static int load_timer_sleepms;
static int load_timer_delayus;
static int load_timer_print;
static struct timer_list load_timer;

static void do_load_timer(struct timer_list *timer)
{
	if (load_timer_print)
		pr_info("++ %s()\n", __func__);

	udelay(load_timer_delayus);

	if (load_timer_sleepms)
		mod_timer(timer, jiffies + msecs_to_jiffies(load_timer_sleepms));
}

void load_timer_start(void *info)
{
	mod_timer(&load_timer, jiffies + msecs_to_jiffies(load_timer_sleepms));
}

void load_timer_test(int sleepms, int delayus, int cpu, int print)
{
	load_timer_sleepms = sleepms;
	load_timer_delayus = delayus;
	load_timer_print = print;

	pr_emerg("%s(%px) sleepms=%d delayus=%d cpu=%d print=%d\n",
		 __func__,
		 &load_timer,
		 load_timer_sleepms,
		 load_timer_delayus,
		 cpu,
		 print);

	timer_setup(&load_timer, do_load_timer, 0);
	smp_call_function_single(cpu, load_timer_start, NULL, 1);
}

void isr_long_test(void)
{
	pr_emerg("+++ %s() start\n", __func__);
	load_hrtimer_test(0, 800000, 3, 0);
	msleep(1500);
	pr_emerg("--- %s() end\n", __func__);
}

void isr_ratio_test(void)
{
	pr_emerg("+++ %s() start\n", __func__);
	load_hrtimer_test(50, 0, 3, 0);
	msleep(2000);
	load_hrtimer_sleepus = 1000000;
	pr_emerg("--- %s() end\n", __func__);
}

void sirq_long_test(void)
{
	pr_emerg("+++ %s() start\n", __func__);
	load_timer_test(0, 800000, 3, 0);
	msleep(1500);
	pr_emerg("--- %s() end\n", __func__);
}

static void idle_long_test(void)
{
	pr_emerg("+++ %s() start\n", __func__);
	idle_long_debug = 1;
	msleep(10000);
	pr_emerg("--- %s() end\n", __func__);
}

static void irq_disable_test1(void)
{
	pr_emerg("+++ %s() start\n", __func__);

	local_irq_disable();
	mdelay(1000);
	local_irq_disable();
	mdelay(1000);
	local_irq_enable();
	mdelay(1000);
	local_irq_enable();

	pr_emerg("--- %s() end\n", __func__);
}

static void irq_disable_test2(void)
{
	unsigned long flags, flags2;

	pr_emerg("+++ %s() start\n", __func__);

	local_irq_save(flags);
	mdelay(1000);
	local_irq_save(flags2);
	mdelay(1000);
	local_irq_restore(flags2);
	mdelay(1000);
	local_irq_restore(flags);

	pr_emerg("--- %s() end\n", __func__);
}

static DEFINE_SPINLOCK(test_lock);
static void irq_disable_test3(void)
{
	unsigned long flags;

	spin_lock_init(&test_lock);
	pr_emerg("+++ %s() start\n", __func__);

	spin_lock_irqsave(&test_lock, flags);
	mdelay(1000);
	spin_unlock_irqrestore(&test_lock, flags);

	pr_emerg("--- %s() end\n", __func__);
}

static int trace_test_thread(void *data)
{
	int i;

	pr_emerg("+++ %s() start\n", __func__);
	for (i = 0; ; i++) {
		ATRACE_COUNTER("atrace_test", i);
		msleep(1000);
	}
	pr_emerg("--- %s() end\n", __func__);
}

static void atrace_test(void)
{
	pr_emerg("+++ %s() start, please confirm with ftrace cmds\n", __func__);
	kthread_run(trace_test_thread, NULL, "atrace_test");
	pr_emerg("--- %s() end\n", __func__);
}

static void smc_long_test(void)
{
	struct arm_smccc_res res;

	pr_emerg("+++ %s() start\n", __func__);
	smc_long_debug = 1;
	arm_smccc_smc(0x1234, 1, 2, 3, 4, 5, 6, 7, &res);
	msleep(2000);
	pr_emerg("--- %s() end\n", __func__);
}

static int rt_throttle_debug;
static int rt_throttle_thread(void *data)
{
	int ret;

	struct sched_attr attr = {
		.size		= sizeof(struct sched_attr),
		.sched_policy	= SCHED_FIFO,
		.sched_nice	= 0,
		.sched_priority	= 50,
	};

	pr_emerg("==== %s() start\n", __func__);

	ret = sched_setattr_nocheck(current, &attr);
	if (ret) {
		pr_warn("%s: failed to set SCHED_FIFO\n", __func__);
		return ret;
	}

	while (true) {
		if (rt_throttle_debug)
			mdelay(1000);
		else
			msleep(1000);
	}

	return 0;
}

static void rt_throttle_test(void)
{
	pr_emerg("+++ %s() start\n", __func__);
	rt_throttle_debug = 1;
	kthread_run(rt_throttle_thread, NULL, "rt_throttle_test");
	msleep(10000);
	rt_throttle_debug = 0;
	pr_emerg("--- %s() end\n", __func__);
}

void lockup_test(void)
{
	pr_emerg("++++++++++++++++++++++++ %s() start\n", __func__);

	irq_check_en = 1;

	isr_long_test();
	isr_ratio_test();
	sirq_long_test();
	idle_long_test();
	irq_disable_test1();
	irq_disable_test2();
	irq_disable_test3();

	smc_long_test();
	rt_throttle_test();

	atrace_test();

	pr_emerg("------------------------ %s() start\n", __func__);
}
#endif
