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

#include <asm/irq_regs.h>
#include <linux/perf_event.h>

static DEFINE_PER_CPU(bool, hard_watchdog_warn);
static DEFINE_PER_CPU(bool, watchdog_nmi_touch);
static cpumask_t __read_mostly watchdog_cpus;
static unsigned long __maybe_unused hardlockup_allcpu_dumped;

static int hardlockup_panic = 1;
core_param(hardlockup_panic, hardlockup_panic, int, 0644);

DECLARE_PER_CPU(unsigned long, hrtimer_interrupts);
DECLARE_PER_CPU(unsigned long, hrtimer_interrupts_saved);

extern void (*pr_lockup_info_hook)(int lockup_cpu);

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

	if (per_cpu(hrtimer_interrupts_saved, cpu) == hrint)
		return 1;

	per_cpu(hrtimer_interrupts_saved, cpu) = hrint;
	return 0;
}

void __nocfi watchdog_check_hardlockup_other_cpu(void)
{
	unsigned int next_cpu;
	/*
	 * Test for hardlockups every 3 samples.  The sample period is
	 *  watchdog_thresh * 2 / 5, so 3 samples gets us back to slightly over
	 *  watchdog_thresh (over by 20%).
	 */
	if (__this_cpu_read(hrtimer_interrupts) % 3 != 0)
		return;

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
		pr_lockup_info_hook(next_cpu);

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

int watchdog_nmi_enable(unsigned int cpu)
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

void watchdog_nmi_disable(unsigned int cpu)
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
