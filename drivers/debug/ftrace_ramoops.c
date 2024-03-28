// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * drivers/amlogic/debug/debug_ftrace_ramoops.c
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
#define SKIP_LOCKUP_CHECK
#include <linux/init.h>
#include <linux/module.h>
#include <linux/irqflags.h>
#include <linux/percpu.h>
#include <linux/ftrace.h>
#include <linux/kprobes.h>
#include <linux/pm_domain.h>
#include <linux/workqueue.h>
#include <trace/hooks/sched.h>
#include <linux/amlogic/aml_iotrace.h>
#include <linux/amlogic/gki_module.h>
#include <trace/events/rwmmio.h>
#include <linux/of_address.h>
#include <linux/scs.h>

#ifdef CONFIG_SHADOW_CALL_STACK
#define SCS_MASK (~(SCS_SIZE - 1))
#endif

static DEFINE_PER_CPU(int, en);

#define IRQ_D	1
#define MAX_DETECT_REG 10

/*
 * bit0: skip vdec-core,vdec irqthread read/write
 * bit1: skip frc tasklet read/write
 * bit2: skip vsync isr read/write
 * bit3: skip amvecm isr read/write
 * bit4: skip usb isr read/write
 */

#define IO_BLACKLIST_VDEC	0x1
#define IO_BLACKLIST_FRC	0x2
#define IO_BLACKLIST_VSYNC	0x4
#define IO_BLACKLIST_AMVECM	0x8
#define IO_BLACKLIST_USB	0x10

static int ramoops_io_blacklist = IO_BLACKLIST_AMVECM;

static int ramoops_io_blacklist_setup(char *buf)
{
	if (!buf)
		return -EINVAL;

	if (kstrtoint(buf, 0, &ramoops_io_blacklist)) {
		pr_err("ramoops_io_blacklist error: %s\n", buf);
		return -EINVAL;
	}

	return 0;
}
__setup("ramoops_io_blacklist=", ramoops_io_blacklist_setup);

static unsigned int check_reg[MAX_DETECT_REG];
static unsigned int check_mask[MAX_DETECT_REG];
static unsigned int *virt_addr[MAX_DETECT_REG];
unsigned long old_val_reg[MAX_DETECT_REG];

DEFINE_PER_CPU(bool, frc_iotrace_cut);
EXPORT_PER_CPU_SYMBOL_GPL(frc_iotrace_cut);

DEFINE_PER_CPU(bool, vsync_iotrace_cut);
EXPORT_PER_CPU_SYMBOL_GPL(vsync_iotrace_cut);

DEFINE_PER_CPU(bool, amvecm_iotrace_cut);
EXPORT_PER_CPU_SYMBOL_GPL(amvecm_iotrace_cut);

DEFINE_PER_CPU(bool, usb_iotrace_cut);
EXPORT_PER_CPU_SYMBOL_GPL(usb_iotrace_cut);

#if 0
static struct delayed_work find_vdec_pid_work;
static pid_t vdec_core_pid, vdec_irqthread_pid0, vdec_irqthread_pid1;
static int retry_times;

static void find_vdec_pid(struct work_struct *work)
{
	struct task_struct *task;

	for_each_process(task) {
		if (!task->mm) {
			if (!vdec_core_pid && !strncmp(task->comm, "vdec-core", 16))
				vdec_core_pid = task->pid;
			if (!vdec_irqthread_pid0 && strstr(task->comm, "vdec-0"))
				vdec_irqthread_pid0 = task->pid;
			if (!vdec_irqthread_pid1 && strstr(task->comm, "vdec-1"))
				vdec_irqthread_pid1 = task->pid;
		}
	}

	if (!vdec_core_pid || !vdec_irqthread_pid0 || !vdec_irqthread_pid1) {
		if (retry_times++ > 3)
			//cancel_delayed_work(&find_vdec_pid_work);
		else
			queue_delayed_work(system_wq, &find_vdec_pid_work, retry_times * 10 * HZ);
	} else {
		//cancel_delayed_work(&find_vdec_pid_work);
	}
}

static bool is_vdec_pid(void)
{
	if (current->pid == vdec_core_pid || current->pid == vdec_irqthread_pid0 ||
		current->pid == vdec_irqthread_pid1)
		return true;
	else
		return false;
}
#endif

static bool is_in_frc_tasklet(void)
{
	unsigned int cpu = raw_smp_processor_id();

	return per_cpu(frc_iotrace_cut, cpu);
}

static bool is_in_vsync_isr(void)
{
	unsigned int cpu = raw_smp_processor_id();

	return per_cpu(vsync_iotrace_cut, cpu);
}

static bool is_in_amvecm_isr(void)
{
	unsigned int cpu = raw_smp_processor_id();

	return per_cpu(amvecm_iotrace_cut, cpu);
}

static bool is_in_usb_isr(void)
{
	unsigned int cpu = raw_smp_processor_id();

	return per_cpu(usb_iotrace_cut, cpu);
}

static int reg_check_panic;
static bool reg_check_flag;

static int reg_check_panic_setup(char *buf)
{
	if (!buf)
		return -EINVAL;

	if (kstrtoint(buf, 0, &reg_check_panic)) {
		pr_err("reg_check_panic error: %s\n", buf);
		return -EINVAL;
	}

	return 0;
}
__setup("reg_check_panic=", reg_check_panic_setup);

void reg_check_init(void)
{
	int i;
	unsigned int *virt_tmp[MAX_DETECT_REG] = {NULL};

	memcpy(virt_tmp, virt_addr, sizeof(virt_addr));

	for (i = 0; i < MAX_DETECT_REG; i++)
		rcu_assign_pointer(virt_addr[i], NULL);

	synchronize_rcu();

	for (i = 0; i < MAX_DETECT_REG; i++) {
		if (virt_tmp[i])
			iounmap(virt_tmp[i]);
		else
			break;
	}

	for (i = 0; i < MAX_DETECT_REG; i++) {
		if (check_reg[i]) {
			virt_addr[i] = (unsigned int *)ioremap(check_reg[i], sizeof(unsigned long));
			if (!virt_addr[i]) {
				pr_err("Unable to map reg 0x%x\n", check_reg[i]);
				return;
			}
			pr_info("reg 0x%x has been mapped to 0x%px\n", check_reg[i], virt_addr[i]);
		} else {
			break;
		}
	}
}

void reg_check_func(void)
{
	unsigned int val;
	unsigned long tmp;
	unsigned int i = 0;
	unsigned int *tmp_addr;

	rcu_read_lock();
	while (i < MAX_DETECT_REG && virt_addr[i]) {
		tmp_addr = rcu_dereference(virt_addr[i]);
		if (old_val_reg[i] != -1) {
			val = *tmp_addr;
			if ((val & check_mask[i]) != (old_val_reg[i] & check_mask[i])) {
				tmp = old_val_reg[i];
				old_val_reg[i] = val;
				pr_err("phys_addr:0x%x new_val=0x%x old_val=0x%lx\n",
					check_reg[i], val, tmp);
				if (!reg_check_panic)
					dump_stack();
				else
					panic("reg_check_panic");
			}
		} else {
			old_val_reg[i] = *tmp_addr;
		}
		i++;
	}
	rcu_read_unlock();
}

static int check_reg_setup(char *ptr)
{
	char *str_entry;
	char *str = (char *)ptr;
	unsigned int tmp;
	unsigned int i = 0, ret;

	do {
		str_entry = strsep(&str, ",");
		if (str_entry) {
			ret = kstrtou32(str_entry, 16, &tmp);
			if (ret)
				return -1;
			pr_info("check_reg: 0x%x\n", tmp);
			check_reg[i] = tmp;
			old_val_reg[i++] = -1;
		}
	} while (str_entry && i < MAX_DETECT_REG);

	reg_check_flag = true;

	return 0;
}

__setup("check_reg=", check_reg_setup);

static int check_mask_setup(char *ptr)
{
	char *str_entry;
	char *str = (char *)ptr;
	unsigned int tmp;
	unsigned int i = 0, ret;

	do {
		str_entry = strsep(&str, ",");
		if (str_entry) {
			ret = kstrtou32(str_entry, 16, &tmp);
			if (ret)
				return -1;
			pr_info("check_mask: 0x%x\n", tmp);
			check_mask[i++] = tmp;
		}
	} while (str_entry && i < MAX_DETECT_REG);

	return 0;
}

__setup("check_mask=", check_mask_setup);

#define REG_MAX_NUM	5
static struct resource gic_mem[REG_MAX_NUM];

static char *gic_compatible[] = {
	"arm,gic-400",
	"arm,arm11mp-gic",
	"arm,arm1176jzf-devchip-gic",
	"arm,cortex-a15-gic",
	"arm,cortex-a9-gic",
	"arm,cortex-a7-gic",
	"arm,p1390"
};

static int gic_reg_num;

static void aml_ramoops_filter_dt(void)
{
	struct device_node *node_gic = NULL;
	int i, ret = 0;

	for (i = 0; i < ARRAY_SIZE(gic_compatible); i++) {
		node_gic = of_find_compatible_node(NULL, NULL, gic_compatible[i]);
		if (node_gic)
			break;
	}
	if (node_gic) {
		for (i = 0; i < REG_MAX_NUM; i++) {
			ret = of_address_to_resource(node_gic, i, &gic_mem[i]);
			if (ret)
				break;
		}
		gic_reg_num = i;
	}
}

static int is_filter_reg(unsigned int reg)
{
	int i;

	/* filter the gic register. */
	for (i = 0; i < gic_reg_num; i++)
		if (reg >= gic_mem[i].start && reg <= gic_mem[i].end)
			return 1;

	return 0;
}

#ifdef CONFIG_SHADOW_CALL_STACK
unsigned long get_prev_lr_val(unsigned long lr, unsigned long offset)
{
	unsigned long lr_mask = lr & SCS_MASK;

	return ((lr - offset) & SCS_MASK) == lr_mask ? *(u64 *)(lr - offset) : 0x0;
}
EXPORT_SYMBOL(get_prev_lr_val);
#endif

void __nocfi pstore_io_save(unsigned long reg, unsigned long val, unsigned int flag,
									unsigned long *irq_flags)
{
	struct iotrace_record rec = {
		.type = flag,
	};

#ifdef CONFIG_SHADOW_CALL_STACK
	unsigned long lr;
#endif
	int cpu = raw_smp_processor_id();

	if (!ramoops_ftrace_en || !(ramoops_trace_mask & TRACE_MASK_IO))
		return;

#if 0
	if (ramoops_io_blacklist) {
		if ((ramoops_io_blacklist & IO_BLACKLIST_VDEC && is_vdec_pid() && in_task()) ||
			(ramoops_io_blacklist & IO_BLACKLIST_FRC && (is_in_frc_tasklet() &&
				!in_irq())) ||
			(ramoops_io_blacklist & IO_BLACKLIST_VSYNC && is_in_vsync_isr()) ||
			(ramoops_io_blacklist & IO_BLACKLIST_AMVECM && is_in_amvecm_isr()) ||
			(ramoops_io_blacklist & IO_BLACKLIST_USB && is_in_usb_isr()))
			return;
	}
#else
	if (ramoops_io_blacklist) {
		if ((ramoops_io_blacklist & IO_BLACKLIST_FRC && (is_in_frc_tasklet() &&
				!in_irq())) ||
			(ramoops_io_blacklist & IO_BLACKLIST_VSYNC && is_in_vsync_isr()) ||
			(ramoops_io_blacklist & IO_BLACKLIST_AMVECM && is_in_amvecm_isr()) ||
			(ramoops_io_blacklist & IO_BLACKLIST_USB && is_in_usb_isr()))
			return;
	}
#endif

#ifdef CONFIG_SHADOW_CALL_STACK
	/* get lr from scs */
	asm volatile("mov %0, x18\n"
		: "=&r" (lr));
#endif

	if ((flag == RECORD_TYPE_IO_R || flag == RECORD_TYPE_IO_W) && IRQ_D)
		local_irq_save(*irq_flags);

	rec.reg = (unsigned int)page_to_phys(vmalloc_to_page((const void *)reg)) +
				offset_in_page(reg);
	rec.reg_val = (unsigned int)val;

	if (reg_check_flag && flag == RECORD_TYPE_IO_W_END)
		reg_check_func();

#ifdef CONFIG_SHADOW_CALL_STACK
	if (flag == RECORD_TYPE_IO_W || flag == RECORD_TYPE_IO_R) {
		rec.ip = get_prev_lr_val(lr, (ramoops_io_skip + 4) * sizeof(unsigned long));
		rec.parent_ip = get_prev_lr_val(lr, (ramoops_io_skip + 5) * sizeof(unsigned long));
	} else {
		rec.ip = get_prev_lr_val(lr, (ramoops_io_skip + 8) * sizeof(unsigned long));
		rec.parent_ip = get_prev_lr_val(lr, (ramoops_io_skip + 9) * sizeof(unsigned long));
	}
#else
	if (flag == RECORD_TYPE_IO_W || flag == RECORD_TYPE_IO_R) {
		switch (ramoops_io_skip) {
		case 0:
			rec.ip = CALLER_ADDR2;
			rec.parent_ip = CALLER_ADDR3;
			break;
		case 1:
			rec.ip = CALLER_ADDR3;
			rec.parent_ip = CALLER_ADDR4;
			break;
		case 2:
			rec.ip = CALLER_ADDR4;
			rec.parent_ip = CALLER_ADDR5;
			break;
		default:
			rec.ip = CALLER_ADDR0;
			rec.parent_ip = CALLER_ADDR1;
			break;
		}
	} else {
		rec.ip = 0;
		rec.parent_ip = 0;
	}
#endif

	cpu = raw_smp_processor_id();
	if (unlikely(oops_in_progress) || unlikely(per_cpu(en, cpu))) {
		if ((flag == RECORD_TYPE_IO_R || flag == RECORD_TYPE_IO_W) && IRQ_D)
			local_irq_restore(*irq_flags);
		return;
	}

	per_cpu(en, cpu) = 1;

	if (!is_filter_reg(rec.reg))
		aml_pstore_write(AML_PSTORE_TYPE_IO, (void *)&rec, irqs_disabled_flags(*irq_flags),
					flag);

	per_cpu(en, cpu) = 0;

	if ((flag == RECORD_TYPE_IO_R_END || flag == RECORD_TYPE_IO_W_END) &&
	    IRQ_D)
		local_irq_restore(*irq_flags);

}
EXPORT_SYMBOL(pstore_io_save);

#ifdef CONFIG_ANDROID_VENDOR_HOOKS
static void schedule_hook(void *data, struct task_struct *prev, struct task_struct *next,
							struct rq *rq)
{
	struct iotrace_record rec = {
		.type = RECORD_TYPE_SCHED_SWITCH,
		.curr_pid = prev->pid,
		.next_pid = next->pid,
	};

	if (!ramoops_ftrace_en || !(ramoops_trace_mask & TRACE_MASK_SCHED))
		return;

	strscpy(rec.curr_comm, prev->comm, sizeof(rec.curr_comm));
	strscpy(rec.next_comm, next->comm, sizeof(rec.next_comm));

	aml_pstore_write(AML_PSTORE_TYPE_SCHED, &rec, irqs_disabled(), 0);
}

static DEFINE_PER_CPU(unsigned long, irqflag);

static void __nocfi rwmmio_write_hook(void *data, unsigned long caller_addr,
		u64 val, u8 width, volatile void __iomem *addr)
{
	unsigned int cpu = get_cpu();

	pstore_ftrace_io_wr((unsigned long)addr, val, per_cpu(irqflag, cpu));
}

static void __nocfi rwmmio_post_write_hook(void *data, unsigned long caller_addr,
		u64 val, u8 width, volatile void __iomem *addr)
{
	unsigned int cpu = raw_smp_processor_id();

	pstore_ftrace_io_wr_end((unsigned long)addr, val, per_cpu(irqflag, cpu));
	put_cpu();
}

static void __nocfi rwmmio_read_hook(void *data, unsigned long caller_addr,
		u8 width, const volatile void __iomem *addr)
{
	unsigned int cpu = get_cpu();

	pstore_ftrace_io_rd((unsigned long)addr, per_cpu(irqflag, cpu));
}

static void __nocfi rwmmio_post_read_hook(void *data, unsigned long caller_addr,
		u64 val, u8 width, const volatile void __iomem *addr)
{
	unsigned int cpu = raw_smp_processor_id();

	pstore_ftrace_io_rd_end((unsigned long)addr, val, per_cpu(irqflag, cpu));
	put_cpu();
}
#endif

int ftrace_ramoops_init(void)
{
	if (reg_check_flag)
		reg_check_init();

	aml_ramoops_filter_dt();

#ifdef CONFIG_ANDROID_VENDOR_HOOKS
	register_trace_android_rvh_schedule(schedule_hook, NULL);

	register_trace_rwmmio_write(rwmmio_write_hook, NULL);
	register_trace_rwmmio_post_write(rwmmio_post_write_hook, NULL);

	register_trace_rwmmio_read(rwmmio_read_hook, NULL);
	register_trace_rwmmio_post_read(rwmmio_post_read_hook, NULL);
#endif
#if 0
	INIT_DELAYED_WORK(&find_vdec_pid_work, find_vdec_pid);
	queue_delayed_work(system_wq, &find_vdec_pid_work, 10 * HZ);
#endif
	return 0;
}
