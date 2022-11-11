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
#include <linux/time.h>
#include <linux/delay.h>
#include <sched.h>

#define KERNEL_ATRACE_TAG KERNEL_ATRACE_TAG_ALL
#include <trace/events/meson_atrace.h>

#include "lockup.h"

static unsigned int load_hrtimer_sleepus;
module_param(load_hrtimer_sleepus, int, 0644);

static unsigned int load_hrtimer_delayms;
module_param(load_hrtimer_delayms, int, 0644);
static int load_hrtimer_print;
static struct hrtimer load_hrtimer;

static enum hrtimer_restart do_load_hrtimer(struct hrtimer *timer)
{
	if (load_hrtimer_print)
		pr_info("++ %s() load_hrtimer_delayms=%d\n", __func__, load_hrtimer_delayms);

	mdelay(load_hrtimer_delayms);

	if (load_hrtimer_print)
		pr_info("-- %s()\n", __func__);

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

void load_hrtimer_test(u64 sleepus, u64 delayus, int cpu, int print)
{
	load_hrtimer_sleepus = sleepus;
	load_hrtimer_delayms = delayus;
	load_hrtimer_print = print;

	pr_emerg("%s: (%px) sleepus=%d delayms=%d cpu=%d print=%d\n",
		__func__,
		&load_hrtimer,
		load_hrtimer_sleepus,
		load_hrtimer_delayms,
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

	if (load_timer_print)
		pr_info("-- %s()\n", __func__);

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
	load_hrtimer_test(0, 600, 3, 1);
	msleep(2000);
	pr_emerg("--- %s() end\n", __func__);
}

void isr_ratio_test(void)
{
	pr_emerg("+++ %s() start\n", __func__);
	load_hrtimer_test(20, 0, 3, 0);
	msleep(3000);
	load_hrtimer_sleepus = 1000000;
	pr_emerg("--- %s() end\n", __func__);
}

void sirq_long_test(void)
{
	pr_emerg("+++ %s() start\n", __func__);
	load_timer_test(0, 800000, 3, 0);
	msleep(2000);
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

static int debug_test_init(void)
{
	pr_emerg("++++++++++++++++++++++++ %s() start\n", __func__);

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

	pr_emerg("------------------------ %s() end\n", __func__);

	return 0;
}

static void debug_test_exit(void)
{
}

module_init(debug_test_init);
module_exit(debug_test_exit);

MODULE_LICENSE("GPL v2");
