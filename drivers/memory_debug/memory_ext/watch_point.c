// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * drivers/amlogic/memory_ext/watch_point.c
 *
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/platform_device.h>
#include <linux/amlogic/iomap.h>
#include <linux/of.h>
#include <linux/of_fdt.h>

#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/compat.h>
#include <linux/random.h>
#include <linux/ptrace.h>
#include <linux/hw_breakpoint.h>
#include <linux/perf_event.h>
#include <linux/cpu.h>
#include <linux/smp.h>
#include <linux/amlogic/watch_point.h>
#include <linux/sched/debug.h>
#include <asm/hw_breakpoint.h>

struct aml_watch_points {
	struct perf_event * __percpu *wp_event[MAX_WATCH_POINTS];
	perf_overflow_handler_t handler[MAX_WATCH_POINTS];
	int num_watch_points;
	struct work_struct replace_work;
	spinlock_t lock;
};

struct aml_watch_points *awp;

#ifdef CONFIG_ARM64
#define READ_WB_REG_CASE(OFF, N, REG, VAL)		\
	do {						\
		case (OFF + N):				\
			AARCH64_DBG_READ(N, REG, VAL);	\
			break;				\
	} while (0)

#define GEN_READ_WB_REG_CASES(OFF, REG, VAL)		\
	do {						\
		READ_WB_REG_CASE(OFF,  0, REG, VAL);	\
		READ_WB_REG_CASE(OFF,  1, REG, VAL);	\
		READ_WB_REG_CASE(OFF,  2, REG, VAL);	\
		READ_WB_REG_CASE(OFF,  3, REG, VAL);	\
		READ_WB_REG_CASE(OFF,  4, REG, VAL);	\
		READ_WB_REG_CASE(OFF,  5, REG, VAL);	\
		READ_WB_REG_CASE(OFF,  6, REG, VAL);	\
		READ_WB_REG_CASE(OFF,  7, REG, VAL);	\
		READ_WB_REG_CASE(OFF,  8, REG, VAL);	\
		READ_WB_REG_CASE(OFF,  9, REG, VAL);	\
		READ_WB_REG_CASE(OFF, 10, REG, VAL);	\
		READ_WB_REG_CASE(OFF, 11, REG, VAL);	\
		READ_WB_REG_CASE(OFF, 12, REG, VAL);	\
		READ_WB_REG_CASE(OFF, 13, REG, VAL);	\
		READ_WB_REG_CASE(OFF, 14, REG, VAL);	\
		READ_WB_REG_CASE(OFF, 15, REG, VAL);	\
	} while (0)

static u64 read_wb_reg(int reg, int n)
{
	u64 val = 0;

	switch (reg + n) {
	GEN_READ_WB_REG_CASES(AARCH64_DBG_REG_BVR, AARCH64_DBG_REG_NAME_BVR, val);
	GEN_READ_WB_REG_CASES(AARCH64_DBG_REG_BCR, AARCH64_DBG_REG_NAME_BCR, val);
	GEN_READ_WB_REG_CASES(AARCH64_DBG_REG_WVR, AARCH64_DBG_REG_NAME_WVR, val);
	GEN_READ_WB_REG_CASES(AARCH64_DBG_REG_WCR, AARCH64_DBG_REG_NAME_WCR, val);
	default:
		pr_warn("attempt to read unknown breakpoint register %d\n", n);
	}

	return val;
}

/* Determine number of WRP registers available. */
static int aml_get_num_wrps(void)
{
	u64 dfr0 = read_sanitised_ftr_reg(SYS_ID_AA64DFR0_EL1);
#ifdef CONFIG_AMLOGIC_VMAP
#if CONFIG_AMLOGIC_KERNEL_VERSION >= 14515
	return (1 +
		cpuid_feature_extract_unsigned_field(dfr0,
						ID_AA64DFR0_EL1_WRPs_SHIFT)) - 2;
#else
	return (1 +
		cpuid_feature_extract_unsigned_field(dfr0,
						ID_AA64DFR0_WRPS_SHIFT)) - 2;
#endif
#else
#if CONFIG_AMLOGIC_KERNEL_VERSION >= 14515
	return 1 +
		cpuid_feature_extract_unsigned_field(dfr0,
						ID_AA64DFR0_EL1_WRPs_SHIFT);
#else
	return 1 +
		cpuid_feature_extract_unsigned_field(dfr0,
						ID_AA64DFR0_WRPS_SHIFT);
#endif
#endif
}
#else
#define READ_WB_REG_CASE(OP2, M, VAL)				\
	do {							\
		case ((OP2 << 4) + M):				\
			ARM_DBG_READ(c0, c ## M, OP2, VAL);	\
			break;					\
	} while (0)

#define GEN_READ_WB_REG_CASES(OP2, VAL)			\
	do {						\
		READ_WB_REG_CASE(OP2, 0, VAL);		\
		READ_WB_REG_CASE(OP2, 1, VAL);		\
		READ_WB_REG_CASE(OP2, 2, VAL);		\
		READ_WB_REG_CASE(OP2, 3, VAL);		\
		READ_WB_REG_CASE(OP2, 4, VAL);		\
		READ_WB_REG_CASE(OP2, 5, VAL);		\
		READ_WB_REG_CASE(OP2, 6, VAL);		\
		READ_WB_REG_CASE(OP2, 7, VAL);		\
		READ_WB_REG_CASE(OP2, 8, VAL);		\
		READ_WB_REG_CASE(OP2, 9, VAL);		\
		READ_WB_REG_CASE(OP2, 10, VAL);		\
		READ_WB_REG_CASE(OP2, 11, VAL);		\
		READ_WB_REG_CASE(OP2, 12, VAL);		\
		READ_WB_REG_CASE(OP2, 13, VAL);		\
		READ_WB_REG_CASE(OP2, 14, VAL);		\
		READ_WB_REG_CASE(OP2, 15, VAL);		\
	} while (0)

static u32 read_wb_reg(int n)
{
	u32 val = 0;

	switch (n) {
	GEN_READ_WB_REG_CASES(ARM_OP2_BVR, val);
	GEN_READ_WB_REG_CASES(ARM_OP2_BCR, val);
	GEN_READ_WB_REG_CASES(ARM_OP2_WVR, val);
	GEN_READ_WB_REG_CASES(ARM_OP2_WCR, val);
	default:
		pr_warn("attempt to read from register %d\n", n);
	}

	return val;
}

/* Determine debug architecture. */
static u8 get_debug_arch(void)
{
	u32 didr;

	/* Do we implement the extended CPUID interface? */
	if (((read_cpuid_id() >> 16) & 0xf) != 0xf) {
		pr_warn_once("CPUID not support.\n");
		return ARM_DEBUG_ARCH_V6;
	}

	ARM_DBG_READ(c0, c0, 0, didr);
	return (didr >> 16) & 0xf;
}

static int debug_arch_supported(void)
{
	u8 arch = get_debug_arch();

	/* We don't support the memory-mapped interface. */
	return (arch >= ARM_DEBUG_ARCH_V6 && arch <= ARM_DEBUG_ARCH_V7_ECP14) ||
		arch >= ARM_DEBUG_ARCH_V7_1;
}

/* Determine number of WRP registers available. */
static int get_num_wrp_resources(void)
{
	u32 didr;

	ARM_DBG_READ(c0, c0, 0, didr);
	return ((didr >> 28) & 0xf) + 1;
}

/* Determine number of usable WRPs available. */
static int aml_get_num_wrps(void)
{
	/*
	 * On debug architectures prior to 7.1, when a watchpoint fires, the
	 * only way to work out which watchpoint it was is by disassembling
	 * the faulting instruction and working out the address of the memory
	 * access.
	 *
	 * Furthermore, we can only do this if the watchpoint was precise
	 * since imprecise watchpoints prevent us from calculating register
	 * based addresses.
	 *
	 * Providing we have more than 1 breakpoint register, we only report
	 * a single watchpoint register for the time being. This way, we always
	 * know which watchpoint fired. In the future we can either add a
	 * disassembler and address generation emulator, or we can insert a
	 * check to see if the DFAR is set on watchpoint exception entry
	 * [the ARM ARM states that the DFAR is UNKNOWN, but experience shows
	 * that it is set on some implementations].
	 */
	if (get_debug_arch() < ARM_DEBUG_ARCH_V7_1)
		return 1;

	return get_num_wrp_resources();
}
#endif

static int aml_hw_breakpoint_slots(int type)
{
#ifdef CONFIG_ARM
	if (!debug_arch_supported())
		return 0;
#endif

	/*
	 * We can be called early, so don't rely on
	 * our static variables being initialised.
	 */
	switch (type) {
	case TYPE_DATA:
		return aml_get_num_wrps();
	default:
		pr_warn("unknown slot type: %d\n", type);
		return 0;
	}
}

static void get_cpu_wb_reg(void *info)
{
	unsigned long *p, r;

	p = (unsigned long *)info;
	r = *p;

#ifdef CONFIG_ARM64
	if (r < AARCH64_DBG_REG_WCR)
		*p = read_wb_reg(AARCH64_DBG_REG_WVR, r - AARCH64_DBG_REG_WVR);
	else
		*p = read_wb_reg(AARCH64_DBG_REG_WCR, r - AARCH64_DBG_REG_WCR);
#else
	*p = read_wb_reg(r);
#endif
}

#ifdef CONFIG_ARM
static struct perf_event **wp_flag(struct perf_event **event, int set)
{
	unsigned long tmp;

	tmp = (unsigned long)event;

	if (set)
		tmp |= 0x01;
	else
		tmp &= ~0x01;
	return (struct perf_event **)tmp;
}

static void wp_del(void *data)
{
	struct perf_event *bp;

	bp = (struct perf_event *)data;
	bp->pmu->del(bp, PERF_EF_UPDATE);
	pr_info("del for wp:%lx, wp:%p\n",
		(unsigned long)bp->attr.bp_addr, bp);
}

static void wp_add(void *data)
{
	struct perf_event *bp;

	bp = (struct perf_event *)data;
	bp->pmu->add(bp, PERF_EF_START);
	pr_info("add for wp:%lx, wp:%p\n",
		(unsigned long)bp->attr.bp_addr, bp);
}
#endif

static int dump_watch_point_reg(char *buf)
{
	int i, cpu = 0;
	unsigned long addr, wvr, wcr;
	int len, type, size = 0;
	struct perf_event *bp;

	size += sprintf(buf + size,
			"idx,            addr, type, len, event, handler\n");
	for (i = 0; i < awp->num_watch_points; i++) {
		if (awp->wp_event[i]) {
			bp = get_cpu_var(*awp->wp_event[i]);
			addr = bp->attr.bp_addr;
			len  = bp->attr.bp_len;
			type = bp->attr.bp_type;
			put_cpu_var(*awp->wp_event[i]);
		} else {
			addr = 0;
			len  = 0;
			type = 0;
		}
		size += sprintf(buf + size, "%2d, %16lx,   %x,   %x, %p, %ps\n",
				i, addr, type, len, awp->wp_event[i],
				awp->handler[i]);
	}
	for_each_online_cpu(cpu) {
		size += sprintf(buf + size, "CPU:%d\n", cpu);
		for (i = 0; i < awp->num_watch_points; i++) {
		#ifdef CONFIG_ARM64
			wvr = AARCH64_DBG_REG_WVR + i;
		#else
			wvr = ARM_BASE_WVR + i;
		#endif
			smp_call_function_single(cpu, get_cpu_wb_reg, &wvr, 1);

		#ifdef CONFIG_ARM64
			wcr = AARCH64_DBG_REG_WCR + i;
		#else
			wcr = ARM_BASE_WCR + i;
		#endif
			smp_call_function_single(cpu, get_cpu_wb_reg, &wcr, 1);
			size += sprintf(buf + size, "  WVR:%16lx WCR:%16lx\n",
					wvr, wcr);
		}
	}
	return size;
}

static void wp_replace_back(struct work_struct *data)
{
	int i, cpu;
	struct perf_event *bp;

	for (i = 0; i < awp->num_watch_points; i++) {
		if (!awp->wp_event[i])
			continue;
	#ifdef CONFIG_ARM64
		cpus_read_lock();
		for_each_online_cpu(cpu) {
			bp = per_cpu(*awp->wp_event[i], cpu);
		#ifdef CONFIG_AMLOGIC_DIS
			if (is_default_overflow_handler(bp)) {
				bp->overflow_handler = awp->handler[i];
				pr_info("replace handler for wp:%lx\n",
					(unsigned long)bp->attr.bp_addr);
			}
		#endif
		}
		cpus_read_unlock();
	#else
		if (!(((unsigned long)awp->wp_event[i]) & 0x01))
			continue;

		awp->wp_event[i] = wp_flag(awp->wp_event[i], 0);
		cpus_read_lock();
		for_each_online_cpu(cpu) {
			bp = per_cpu(*awp->wp_event[i], cpu);
			smp_call_function_single(cpu, wp_add, bp, 1);
		}
		cpus_read_unlock();
	#endif
	}
}

static void aml_default_hbp_handler(struct perf_event *bp,
				    struct perf_sample_data *data,
				    struct pt_regs *regs)
{
#ifdef CONFIG_ARM64
	pr_info("watch addr %llx triggered, pc:%ps, lr:%ps\n",
		bp->attr.bp_addr, (void *)regs->pc,
		(void *)regs->compat_lr_fiq);
#ifdef CONFIG_AMLOGIC_DIS
	bp->overflow_handler = perf_event_output_forward;
#endif
	show_regs(regs);
	dump_stack();
#else
	struct perf_event * __percpu *event = NULL;
	int i, cpu;

	pr_info("watch addr %llx triggered, pc:%ps, lr:%ps\n",
		bp->attr.bp_addr, (void *)regs->ARM_pc,
		(void *)regs->ARM_lr);

	for (i = 0; i < awp->num_watch_points; i++) {
		if (!awp->wp_event[i])
			continue;
		for_each_online_cpu(cpu) {
			if (bp == per_cpu(*awp->wp_event[i], cpu)) {
				event = awp->wp_event[i];
				break;
			}
		}
		if (event) {
			for_each_online_cpu(cpu) {
				bp = per_cpu(*event, cpu);
				smp_call_function_single(cpu, wp_del, bp, 1);
			}
			break;
		}
	}
	if (event)
		awp->wp_event[i] = wp_flag(awp->wp_event[i], 1);
	//show_regs(regs);
	dump_stack();
#endif
//	schedule_work_on(smp_processor_id(), &awp->replace_work);
}

/* register a watch pointer */
int aml_watch_point_register(unsigned long addr,
			     unsigned int len,
			     unsigned int type,
			     perf_overflow_handler_t handle)
{
	int i;
	struct perf_event_attr attr;
	struct perf_event * __percpu *event;

	if (!awp)
		return -ENOMEM;

	/* parameter check */
	if (len > HW_BREAKPOINT_LEN_8 || len < HW_BREAKPOINT_LEN_1) {
		pr_err("bad input len:%d\n", len);
		return -EINVAL;
	}
	if (type & ~(HW_BREAKPOINT_W | HW_BREAKPOINT_R)) {
		pr_err("bad input type:%d\n", type);
		return -EINVAL;
	}

	/* check if all watch points are used */
	spin_lock(&awp->lock);
	for (i = 0; i < awp->num_watch_points; i++) {
		if (!awp->wp_event[i]) {
			awp->wp_event[i]++;
			break;
		}
	}
	if (i == awp->num_watch_points) {
		spin_unlock(&awp->lock);
		pr_err("%s, watch point is all used\n", __func__);
		return -ENODEV;
	}
	spin_unlock(&awp->lock);

	hw_breakpoint_init(&attr);
	attr.bp_addr = addr;
	attr.bp_len  = len;
	attr.bp_type = type;
	if (!handle)
		handle = aml_default_hbp_handler;

	event = register_wide_hw_breakpoint(&attr, handle, NULL);
	spin_lock(&awp->lock);
	if (IS_ERR_OR_NULL(event)) {
		awp->wp_event[i] = NULL;
		awp->handler[i]  = NULL;
	} else {
		awp->wp_event[i] = event;
		awp->handler[i]  = handle;
	}
	spin_unlock(&awp->lock);

	pr_info("watch point[%d], addr:%lx, len:%d, type:%x, event:%p\n",
		i, addr, len, type, awp->wp_event[i]);
	return awp->wp_event[i] ? 0 : -EINVAL;
}
EXPORT_SYMBOL(aml_watch_point_register);

/* remove watch point according given address */
void aml_watch_point_remove(unsigned long addr)
{
	int i;
	struct perf_event *bp;
	struct perf_event * __percpu *event = NULL;

	if (!awp)
		return;

	spin_lock(&awp->lock);
	for (i = 0; i < awp->num_watch_points; i++) {
		if (awp->wp_event[i]) {
			bp = get_cpu_var(*awp->wp_event[i]);
			if (bp->attr.bp_addr == addr) {
				event = awp->wp_event[i];
				awp->wp_event[i] = NULL;
				awp->handler[i] = NULL;
				put_cpu_var(*awp->wp_event[i]);
				break;
			}
			put_cpu_var(*awp->wp_event[i]);
		}
	}
	spin_unlock(&awp->lock);
	if (event)
		unregister_wide_hw_breakpoint(event);
}
EXPORT_SYMBOL(aml_watch_point_remove);

static ssize_t watch_addr_show(struct class *cla,
	struct class_attribute *attr, char *buf)
{
	return dump_watch_point_reg(buf);
}

static ssize_t watch_addr_store(struct class *cla,
	struct class_attribute *attr, const char *buf, size_t count)
{
	unsigned long addr;
	u32 len = HW_BREAKPOINT_LEN_8;
	u32 type = HW_BREAKPOINT_W;
	int ret;

	ret = sscanf(buf, "%lx %x %x", &addr, &len, &type);
	if (ret < 1)
		return -EINVAL;

	ret = aml_watch_point_register(addr, len, type, NULL);

	return count;
}
static CLASS_ATTR_RW(watch_addr);

static ssize_t num_watch_points_show(struct class *cla,
	struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", awp->num_watch_points);
}
static CLASS_ATTR_RO(num_watch_points);

/*
 * force clear a watch point
 */
static void aml_watch_point_clear(int idx)
{
	struct perf_event * __percpu *event = NULL;

	if (idx >= awp->num_watch_points)
		return;

	spin_lock(&awp->lock);
	if (awp->wp_event[idx]) {
		event = awp->wp_event[idx];
		awp->wp_event[idx] = NULL;
		awp->handler[idx] = NULL;
	}
	spin_unlock(&awp->lock);
	if (event)
		unregister_wide_hw_breakpoint(event);
}

static ssize_t clear_store(struct class *cla,
	struct class_attribute *attr, const char *buf, size_t count)
{
	int i;
	int idx = -1;

	if (kstrtoint(buf, 10, &idx))
		return count;

	if (idx >= awp->num_watch_points) {
		pr_err("input index %d out of range:[0 - %d]\n",
			idx, awp->num_watch_points);
		return -EINVAL;
	}

	/* negative value means clear all watch point */
	if (idx < 0) {
		for (i = 0; i < awp->num_watch_points; i++)
			aml_watch_point_clear(i);
	} else {
		aml_watch_point_clear(idx);
	}

	return count;
}
static CLASS_ATTR_WO(clear);

unsigned long test;
EXPORT_SYMBOL(test);
static ssize_t test_show(struct class *cla,
	struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "addr=%lx, value=%lx\n", (unsigned long)&test, test);
}

static ssize_t test_store(struct class *cla,
	struct class_attribute *attr, const char *buf, size_t count)
{
	int ret;

	ret = kstrtoul(buf, 16, &test);
	if (ret < 0)
		return -EINVAL;

	return count;
}
static CLASS_ATTR_RW(test);

static struct attribute *watch_point_class_attrs[] = {
	&class_attr_watch_addr.attr,
	&class_attr_num_watch_points.attr,
	&class_attr_clear.attr,
	&class_attr_test.attr,
	NULL,
};
ATTRIBUTE_GROUPS(watch_point_class);

static struct class watch_point_class = {
	.name		= "watch_point",
	.owner		= THIS_MODULE,
	.class_groups	= watch_point_class_groups,
};

/*
 *    aml_watch_point_probe only executes before the init process starts
 * to run, so add __ref to indicate it is okay to call __init function
 * hook_debug_fault_code
 */
static int __init aml_watch_point_probe(struct platform_device *pdev)
{
	int r;

	r = aml_hw_breakpoint_slots(TYPE_DATA);
	pr_info("%s, in, wp:%d\n", __func__, r);
	if (!r)
		return -ENODEV;

	awp = devm_kzalloc(&pdev->dev, sizeof(*awp), GFP_KERNEL);
	if (!awp)
		return -ENOMEM;

	awp->num_watch_points = r;
	r = class_register(&watch_point_class);
	if (r) {
		pr_err("regist watch_point_class failed\n");
		return -EINVAL;
	}
	INIT_WORK(&awp->replace_work, wp_replace_back);

	return 0;
}

static int aml_watch_point_drv_remove(struct platform_device *pdev)
{
	class_unregister(&watch_point_class);
	return 0;
}

static struct platform_driver aml_watch_point_driver = {
	.driver = {
		.name  = "aml_watch_point",
		.owner = THIS_MODULE,
	},
	.remove = aml_watch_point_drv_remove,
};

int __init aml_watch_pint_init(void)
{
	struct platform_device *pdev;
	int ret;

	pdev = platform_device_alloc("aml_watch_point", 0);
	if (!pdev) {
		pr_err("alloc pdev aml_watch_point failed\n");
		return -EINVAL;
	}
	ret = platform_device_add(pdev);
	if (ret) {
		pr_err("regist pdev failed, ret:%d\n", ret);
		platform_device_del(pdev);
		return ret;
	}
	ret = platform_driver_probe(&aml_watch_point_driver,
				    aml_watch_point_probe);
	if (ret)
		platform_device_del(pdev);
	return ret;
}

void __exit aml_watch_point_uninit(void)
{
	platform_driver_unregister(&aml_watch_point_driver);
}
