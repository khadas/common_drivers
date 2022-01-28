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

static struct hrtimer cpu_mhz_timer;
static int cpu_mhz_period_ms;

#define FIVE(m)         {m; m; m; m; m; }
#define TEN(m)          {FIVE(m) FIVE(m)}
#define FIFTY(m)        {TEN(m) TEN(m) TEN(m) TEN(m) TEN(m)}
#define HUNDRED(m)      {FIFTY(m) FIFTY(m)}
#define FIVE_HUNDRED(m) {HUNDRED(m) HUNDRED(m) HUNDRED(m) HUNDRED(m) HUNDRED(m)}
#define THOUSAND(m)     {FIVE_HUNDRED(m) FIVE_HUNDRED(m)}
#if defined CONFIG_ARM
#define ONE_LOOP asm("add r0, r0, #1\n":::"r0", "memory")
#elif defined CONFIG_ARM64
#define ONE_LOOP asm("add x0, x0, #1\n":::"x0", "memory")
#endif

#define TIMENS_1M_LOOPS_OF_1G_CPU 1005000  //about 1ms
int cpu_speed_test(void)
{
	int i, mhz;
	unsigned long long start, end;
	unsigned int delta;
	unsigned long flags;

	local_irq_save(flags);

	/* do 1M loops */
	start = sched_clock();
	for (i = 0; i < 1000; i++)
		THOUSAND(ONE_LOOP);

	end = sched_clock();

	local_irq_restore(flags);

	if (end - start > UINT_MAX) {
		WARN(1, "%s() consume %llu ns\n", __func__, end - start);
		return 0;
	}

	delta = (unsigned int)(end - start);
	mhz = TIMENS_1M_LOOPS_OF_1G_CPU * 1000 / delta;

	pr_debug("%s() delta_us=%u mhz=%d\n", __func__, delta / 1000, mhz);

	return mhz;
}

static enum hrtimer_restart test_mhz_hrtimer_func(struct hrtimer *timer)
{
	int mhz;
	ktime_t now, interval;

	if (!cpu_mhz_period_ms) {
		pr_info("stop test_mhz hrtimer\n");
		return HRTIMER_NORESTART;
	}

	/* don't too frequently */
	if (cpu_mhz_period_ms < 1000)
		cpu_mhz_period_ms = 1000;

	mhz = cpu_speed_test();
	pr_info("cpu_mhz=%d\n", mhz);

	now = ktime_get();
	interval = ktime_set(cpu_mhz_period_ms / 1000,
			     cpu_mhz_period_ms % 1000 * 1000000);
	hrtimer_forward(timer, now, interval);
	return HRTIMER_RESTART;
}

static int cpu_mhz_period_ms_set(const char *buffer, const struct kernel_param *kp)
{
	int ret;
	int period_ms = 0;

	pr_info("cpu_mhz_period_ms_set() buffer=%s\n", buffer);

	ret = kstrtoint(buffer, 0, &period_ms);
	if (ret)
		return -1;

	if (!period_ms)
		period_ms = 0;
	else if (period_ms < 1000)
		period_ms = 1000;

	*(int *)kp->arg = period_ms; //cpu_mhz_period_ms

	if (cpu_mhz_period_ms)
		pr_info("set cpu_mhz_period_ms=%d\n", cpu_mhz_period_ms);
	else
		pr_info("set cpu_mhz_period_ms zero, disable!\n");

	hrtimer_cancel(&cpu_mhz_timer);

	if (cpu_mhz_period_ms)
		hrtimer_start(&cpu_mhz_timer, ktime_set(1, 0), HRTIMER_MODE_REL);

	return 0;
}

static int cpu_mhz_period_ms_get(char *buffer, const struct kernel_param *kp)
{
	int period_ms = *(int *)kp->arg;

	return sprintf(buffer, "%d\n", period_ms);
}

static const struct kernel_param_ops cpu_mhz_period_ms_ops = {
	.set	= cpu_mhz_period_ms_set,
	.get	= cpu_mhz_period_ms_get,
};

module_param_cb(cpu_mhz_period_ms, &cpu_mhz_period_ms_ops, &cpu_mhz_period_ms, 0644);

static int cpu_mhz_get(char *buffer, const struct kernel_param *kp)
{
	int mhz;

	mhz = cpu_speed_test();
	return sprintf(buffer, "%d\n", mhz);
}

static const struct kernel_param_ops cpu_mhz_ops = {
	.get	= cpu_mhz_get,
};

module_param_cb(cpu_mhz, &cpu_mhz_ops, NULL, 0644);

int cpu_mhz_init(void)
{
	hrtimer_init(&cpu_mhz_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	cpu_mhz_timer.function = test_mhz_hrtimer_func;

	if (cpu_mhz_period_ms)
		hrtimer_start(&cpu_mhz_timer, ktime_set(1, 0), HRTIMER_MODE_REL);

	return 0;
}
