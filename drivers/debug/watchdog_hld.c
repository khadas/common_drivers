// SPDX-License-Identifier: GPL-2.0
/*
 * Detect hard lockups on a system
 *
 * started by Don Zickus, Copyright (C) 2010 Red Hat, Inc.
 *
 * Note: Most of this code is borrowed heavily from the original softlockup
 * detector, so thanks to Ingo for the initial implementation.
 * Some chunks also taken from the old x86-specific nmi watchdog code, thanks
 * to those contributors as well.
 */

#include <linux/nmi.h>
#include <linux/atomic.h>
#include <linux/module.h>
#include <linux/sched/debug.h>
#include <linux/smpboot.h>
#include <asm/irq_regs.h>
#include <linux/perf_event.h>

#include "lockup.h"

static DEFINE_PER_CPU(bool, hard_watchdog_warn);
static DEFINE_PER_CPU(bool, watchdog_nmi_touch);
static cpumask_t __read_mostly watchdog_cpus;

static ulong __read_mostly sample_period;
module_param(sample_period, ulong, 0644);

static int hardlockup_thresh = 10; /* seconds */
module_param(hardlockup_thresh, int, 0644);

static int hardlockup_panic = 1;
module_param(hardlockup_panic, int, 0644);

static DEFINE_PER_CPU(unsigned long, hrtimer_interrupts);
static DEFINE_PER_CPU(unsigned long, hrtimer_interrupts_saved);
static DEFINE_PER_CPU(unsigned long, hrtimer_interrupts_lock_cnt);
static DEFINE_PER_CPU(struct hrtimer, watchdog_hrtimer);

static void set_sample_period(void)
{
	sample_period = 1 * (u64)NSEC_PER_SEC;
}

static unsigned int watchdog_next_cpu(unsigned int cpu)
{
	cpumask_t cpus = watchdog_cpus;
	unsigned int next_cpu;

	next_cpu = cpumask_next(cpu, &cpus);
	if (next_cpu >= nr_cpu_ids)
		next_cpu = cpumask_first(&cpus);

	if (next_cpu == cpu)
		return nr_cpu_ids;

	return next_cpu;
}

static int is_hardlockup_other_cpu(unsigned int cpu)
{
	unsigned long hrint = per_cpu(hrtimer_interrupts, cpu);
	unsigned long lock_cnt = per_cpu(hrtimer_interrupts_lock_cnt, cpu);

	if (hrint ==  per_cpu(hrtimer_interrupts_saved, cpu)) {
		per_cpu(hrtimer_interrupts_lock_cnt, cpu) = ++lock_cnt;
		if (lock_cnt > hardlockup_thresh)
			return 1;
	} else {
		per_cpu(hrtimer_interrupts_lock_cnt, cpu) = 0;
	}

	per_cpu(hrtimer_interrupts_saved, cpu) = hrint;
	return 0;
}

static void watchdog_check_hardlockup_other_cpu(void)
{
	unsigned int next_cpu;

	/* check for a hardlockup on the next cpu */
	next_cpu = watchdog_next_cpu(smp_processor_id());
	if (next_cpu >= nr_cpu_ids)
		return;

	/*mem barrier*/
	smp_rmb();

	if (per_cpu(watchdog_nmi_touch, next_cpu) == true) {
		per_cpu(watchdog_nmi_touch, next_cpu) = false;
		return;
	}

	if (is_hardlockup_other_cpu(next_cpu)) {
		/* only warn once */
		if (per_cpu(hard_watchdog_warn, next_cpu) == true)
			return;

		pr_lockup_info(next_cpu);

		if (hardlockup_panic)
			panic("Watchdog detected hard LOCKUP on cpu %u",
			      next_cpu);
		else
			WARN(1, "Watchdog detected hard LOCKUP on cpu %u",
			     next_cpu);

		per_cpu(hard_watchdog_warn, next_cpu) = true;
	} else {
		per_cpu(hard_watchdog_warn, next_cpu) = false;
	}
}

/* watchdog kicker functions */
static enum hrtimer_restart watchdog_timer_fn(struct hrtimer *hrtimer)
{
	/* kick the hardlockup detector */
	__this_cpu_inc(hrtimer_interrupts);

	/* test for hardlockups on the next cpu */
	watchdog_check_hardlockup_other_cpu();

	/* .. and repeat */
	hrtimer_forward_now(hrtimer, ns_to_ktime(sample_period));

	return HRTIMER_RESTART;
}

static int aml_watchdog_nmi_enable(unsigned int cpu)
{
	/*
	 * The new cpu will be marked online before the first hrtimer interrupt
	 * runs on it.  If another cpu tests for a hardlockup on the new cpu
	 * before it has run its first hrtimer, it will get a false positive.
	 * Touch the watchdog on the new cpu to delay the first check for at
	 * least 3 sampling periods to guarantee one hrtimer has run on the new
	 * cpu.
	 */
	per_cpu(watchdog_nmi_touch, cpu) = true;
	/*mem barrier*/
	smp_wmb();
	cpumask_set_cpu(cpu, &watchdog_cpus);
	return 0;
}

static void aml_watchdog_nmi_disable(unsigned int cpu)
{
	unsigned int next_cpu = watchdog_next_cpu(cpu);

	/*
	 * Offlining this cpu will cause the cpu before this one to start
	 * checking the one after this one.  If this cpu just finished checking
	 * the next cpu and updating hrtimer_interrupts_saved, and then the
	 * previous cpu checks it within one sample period, it will trigger a
	 * false positive.  Touch the watchdog on the next cpu to prevent it.
	 */
	if (next_cpu < nr_cpu_ids)
		per_cpu(watchdog_nmi_touch, next_cpu) = true;
	/*mem barrier*/
	smp_wmb();
	cpumask_clear_cpu(cpu, &watchdog_cpus);
}

static void watchdog_enable(unsigned int cpu)
{
	struct hrtimer *hrtimer = raw_cpu_ptr(&watchdog_hrtimer);

	/* kick off the timer for the hardlockup detector */
	hrtimer_init(hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hrtimer->function = watchdog_timer_fn;

	/* Enable the perf event */
	aml_watchdog_nmi_enable(cpu);

	/* done here because hrtimer_start can only pin to smp_processor_id() */
	hrtimer_start(hrtimer, ns_to_ktime(sample_period),
		      HRTIMER_MODE_REL_PINNED);
}

static void watchdog_disable(unsigned int cpu)
{
	struct hrtimer *hrtimer = raw_cpu_ptr(&watchdog_hrtimer);

	hrtimer_cancel(hrtimer);
	/* disable the perf event */
	aml_watchdog_nmi_disable(cpu);
}

static int hld_should_run(unsigned int cpu)
{
	return 0;
}

static void hld_run(unsigned int cpu)
{
}

static void hld_cleanup(unsigned int cpu, bool online)
{
	watchdog_disable(cpu);
}

static void hld_setup(unsigned int cpu)
{
	watchdog_enable(cpu);
}

static void hld_park(unsigned int cpu)
{
	watchdog_disable(cpu);
}

static void hld_unpark(unsigned int cpu)
{
	watchdog_enable(cpu);
}

DEFINE_PER_CPU(struct task_struct *, aml_hld);

static struct smp_hotplug_thread hld_threads = {
	.store			= &aml_hld,
	.thread_should_run	= hld_should_run,
	.thread_fn		= hld_run,
	.thread_comm		= "aml_hld/%u",
	.setup			= hld_setup,
	.cleanup		= hld_cleanup,
	.park			= hld_park,
	.unpark			= hld_unpark,
};

int aml_hld_init(void)
{
	int ret;

	set_sample_period();

	ret = smpboot_register_percpu_thread(&hld_threads);
	if (ret)
		pr_err("create hld_threads failed\n");

	return ret;
}
