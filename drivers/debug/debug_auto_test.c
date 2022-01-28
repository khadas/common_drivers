// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */
#include <linux/module.h>
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
#include <linux/delay.h>

static struct hrtimer isr_check_hrtimer;
static unsigned long isr_check_hrtimer_delayus;
static unsigned long isr_check_hrtimer_sleepus;

static enum hrtimer_restart isr_check_hrtimer_handler(struct hrtimer *timer)
{
	if (isr_check_hrtimer_delayus)
		udelay(isr_check_hrtimer_delayus);

	if (isr_check_hrtimer_sleepus) {
		hrtimer_forward(timer, ktime_get(), ktime_set(0, isr_check_hrtimer_sleepus*1000));
		return HRTIMER_RESTART;
	}

	return HRTIMER_NORESTART;
}

void isr_check_test(void)
{
	hrtimer_init(&isr_check_hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	isr_check_hrtimer.function = isr_check_hrtimer_handler;

	pr_info("---- isr_check_test() hrtimer delay 600ms start\n");
	isr_check_hrtimer_delayus = 600000;
	isr_check_hrtimer_sleepus = 0;
	hrtimer_start(&isr_check_hrtimer, ktime_set(1, 0), HRTIMER_MODE_REL);
	msleep(5000);
	hrtimer_cancel(&isr_check_hrtimer);

	pr_info("---- isr_check_test() hrtimer 5us sleep loop start\n");
	isr_check_hrtimer_delayus = 0;
	isr_check_hrtimer_sleepus = 5;
	hrtimer_start(&isr_check_hrtimer, ktime_set(1, 0), HRTIMER_MODE_REL);
	msleep(5000);
	hrtimer_cancel(&isr_check_hrtimer);
}

static int sirq_timer_delayus;
static struct timer_list sirq_timer;

static void sirq_timer_handler(struct timer_list *timer)
{
	udelay(sirq_timer_delayus);
}

void sirq_check_test(void)
{
	pr_info("---- sirq_check_test() delay 600ms start\n");
	sirq_timer_delayus = 600000;
	timer_setup(&sirq_timer, sirq_timer_handler, 0);
	mod_timer(&sirq_timer, jiffies + HZ);
	msleep(5000);
}

static int idle_test_delayus = 6000000;
static int idle_test_handler(struct kprobe *p, struct pt_regs *regs)
{
	if (smp_processor_id() == 0 && idle_test_delayus) {
		udelay(idle_test_delayus);
		idle_test_delayus = 0;
	}

	return 0;
}

static struct kprobe idle_kp = {
	.symbol_name = "psci_enter_idle_state",
	.pre_handler = idle_test_handler,
};

void idle_check_test(void)
{
	pr_info("----- idle_check_test() idle delay 5s start\n");
	register_kprobe(&idle_kp);
	msleep(6000);
}

static int smc_test_delayms = 6000;
static int smc_test_pid;
static int smc_test_handler(struct kprobe *p, struct pt_regs *regs)
{
	if (smc_test_delayms && current->pid == smc_test_pid) {
		mdelay(smc_test_delayms);
		smc_test_delayms = 0;
	}

	return 0;
}

static struct kprobe smc_kp = {
	.symbol_name = "__arm_smccc_smc",
	.pre_handler = smc_test_handler,
};

void smc_check_test(void)
{
	struct arm_smccc_res res;
	pr_info("---- smc_check_test() smc delay 6s start\n");

	register_kprobe(&smc_kp);

	smc_test_pid = current->pid;
	arm_smccc_smc(0x82000040, 0, 0, 0, 0, 0, 0, 0, &res); //jtag on
}

void irq_disable_test(void)
{
	spinlock_t lock;
	unsigned long flags;

	spin_lock_init(&lock);

	pr_info("---- irq_disable_test() lock 8s start\n");
	spin_lock_irq(&lock);
	mdelay(1000);
	local_irq_save(flags);
	mdelay(2000);
	local_irq_restore(flags);
	mdelay(5000);
	spin_unlock_irq(&lock);
	mdelay(5000);
	local_irq_enable();
}

#define KERNEL_ATRACE_TAG KERNEL_ATRACE_TAG_CPUFREQ
#include <trace/events/meson_atrace.h>

static struct timer_list atrace_timer;

static void atrace_timer_handler(struct timer_list *timer)
{
	static int i;
	ATRACE_COUNTER("atrace_test", i++);
	mod_timer(timer, jiffies + HZ);
}

void meson_atrace_test(void)
{
	pr_info("---- meson_atrace_test() KERNEL_ATRACE_TAG_CPUFREQ start\n");
	timer_setup(&atrace_timer, atrace_timer_handler, 0);
	mod_timer(&atrace_timer, jiffies + HZ);
}

static int __init debug_test_init(void)
{
	isr_check_test();
	sirq_check_test();
	idle_check_test();
	smc_check_test();
	irq_disable_test();

	meson_atrace_test();

	return 0;
}

static void __exit debug_test_exit(void)
{
}

module_init(debug_test_init);
module_exit(debug_test_exit);

MODULE_LICENSE("GPL v2");
