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
#include <linux/io.h>
#include <linux/amlogic/gki_module.h>
#include <linux/kprobes.h>
#include <linux/printk.h>
#include <linux/amlogic/user_fault.h>
#if IS_ENABLED(CONFIG_AMLOGIC_DEBUG_TEST)
#define KERNEL_ATRACE_TAG KERNEL_ATRACE_TAG_ALL
#include <trace/events/meson_atrace.h>
#endif
#include <trace/events/irq.h>
#include <trace/hooks/cpuidle.h>
#include <trace/hooks/dtask.h>
#include <trace/hooks/sched.h>
#include <trace/hooks/preemptirq.h>
#include <trace/hooks/gic_v3.h>
#include <trace/hooks/ftrace_dump.h>
#include <linux/time.h>
#include <linux/delay.h>
#if IS_ENABLED(CONFIG_AMLOGIC_DEBUG_IOTRACE)
#include <linux/amlogic/aml_iotrace.h>
#endif
#include <sched.h>

#include "lockup.h"

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

static unsigned long long isr_long_thr = LONG_ISR;
module_param(isr_long_thr, ullong, 0644);

static unsigned long isr_ratio_thr = 50;
module_param(isr_ratio_thr, ulong, 0644);

static unsigned long long sirq_thr = LONG_SIRQ;
module_param(sirq_thr, ullong, 0644);

static unsigned long long idle_thr = LONG_IDLE;
module_param(idle_thr, ullong, 0644);

static unsigned long long smc_thr = LONG_SMC;
module_param(smc_thr, ullong, 0644);

static int isr_check_en = 1;
module_param(isr_check_en, int, 0644);

static int sirq_check_en = 1;
module_param(sirq_check_en, int, 0644);

static int idle_check_en = 1;
module_param(idle_check_en, int, 0644);

static int smc_check_en = 1;
module_param(smc_check_en, int, 0644);

static unsigned long long irq_disable_thr = LONG_IRQDIS;
module_param(irq_disable_thr, ullong, 0644);

static int irq_check_en;
module_param(irq_check_en, int, 0644);

static int irq_check_en_setup(char *str)
{
	if (!strcmp(str, "1")) {
		irq_check_en = 1;
		return 0;
	}
	if (!strcmp(str, "0")) {
		irq_check_en = 0;
		return 0;
	}
	return 1;
}
__setup("irq_check_en=", irq_check_en_setup);

#if (defined CONFIG_ARM64) || (defined CONFIG_AMLOGIC_ARMV8_AARCH32)
#define FIQ_DEBUG_SMC_CMD	0x820000f1
#define FIQ_INIT_SMC_ARG	0x1
#define FIQ_SEND_SMC_ARG	0x2

static int fiq_check_en = 1;

static int fiq_check_en_setup(char *str)
{
	if (!strcmp(str, "1")) {
		fiq_check_en = 1;
		return 0;
	}
	if (!strcmp(str, "0")) {
		fiq_check_en = 0;
		return 0;
	}
	return 1;
}
__setup("fiq_check_en=", fiq_check_en_setup);

static int fiq_check_show_regs_en;

static int fiq_check_show_regs_en_setup(char *str)
{
	if (!strcmp(str, "1")) {
		fiq_check_show_regs_en = 1;
		return 0;
	}
	if (!strcmp(str, "0")) {
		fiq_check_show_regs_en = 0;
		return 0;
	}
	return 1;
}
__setup("fiq_check_show_regs_en=", fiq_check_show_regs_en_setup);

struct fiq_regs {
	u64 regs[31];
	u64 sp;
	u64 pc;
	u64 pstate;
};

struct fiq_smc_ret {
	unsigned long phy_addr;
	unsigned long buf_size;
	unsigned long percpu_size;
	unsigned long reserved1;
};

static void *fiq_virt_addr;
static unsigned long fiq_buf_size;
static unsigned long fiq_percpu_size;
#endif

static int initialized;

static void (*lockup_hook)(int cpu);

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
	int smc_enter_task_pid;
	char smc_enter_task_comm[TASK_COMM_LEN];
	unsigned long curr_smc_a0;
	unsigned long curr_smc_a1;
	unsigned long smc_enter_trace_entries[ENTRY];
	int smc_enter_trace_entries_nr;

	/* irq disable check */
	unsigned long long irq_disable_time;
	unsigned long irq_disable_trace_entries[ENTRY];
	int irq_disable_trace_entries_nr;
};

static struct lockup_info __percpu *infos;

static void __maybe_unused isr_in_hook(void *data, int irq, struct irqaction *action)
{
	struct lockup_info *info;
	struct isr_check_info *isr_info;
	int cpu;
	unsigned long long now;
#if IS_ENABLED(CONFIG_AMLOGIC_DEBUG_IOTRACE)
	char buf[20];
#endif

	if (irq >= IRQ_CNT || !isr_check_en)
		return;

#if IS_ENABLED(CONFIG_AMLOGIC_DEBUG_IOTRACE)
	if (ramoops_io_en && (ramoops_trace_mask & 0x4)) {
		memset(buf, 0, sizeof(buf));
		snprintf(buf, 20, "isr-in irq:%d", irq);
		aml_pstore_write(AML_PSTORE_TYPE_IRQ, buf, 0);
	}
#endif

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

static void __maybe_unused isr_out_hook(void *data, int irq, struct irqaction *action, int ret)
{
	struct lockup_info *info;
	struct isr_check_info *isr_info;
	int cpu;
	unsigned long long now, delta, this_period_time;
#if IS_ENABLED(CONFIG_AMLOGIC_DEBUG_IOTRACE)
	char buf[20];
#endif

	if (irq >= IRQ_CNT || !isr_check_en)
		return;

	cpu = smp_processor_id();

	info = per_cpu_ptr(infos, cpu);
	info->curr_irq = INVALID_IRQ;

	isr_info = &info->isr_infos[irq];
	if (!isr_info->exec_start_time)
		return;

#if IS_ENABLED(CONFIG_AMLOGIC_DEBUG_IOTRACE)
	if (ramoops_io_en && (ramoops_trace_mask & 0x4)) {
		memset(buf, 0, sizeof(buf));
		snprintf(buf, 20, "isr-out irq:%d", irq);
		aml_pstore_write(AML_PSTORE_TYPE_IRQ, buf, 0);
	}
#endif

	now = sched_clock();
	delta = now - isr_info->exec_start_time;
	isr_info->exec_start_time = 0;

	isr_info->exec_sum_time += delta;
	isr_info->cnt++;

	if (delta > isr_long_thr)
		pr_err("ISR_Long___ERR. irq:%d/%s action=%ps exec_time:%lluus\n",
		       irq, action->name, action->handler, div_u64(delta, ns2us));

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

static void __maybe_unused softirq_in_hook(void *data, unsigned int vec_nr)
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

static void __maybe_unused softirq_out_hook(void *data, unsigned int vec_nr)
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

#if IS_ENABLED(CONFIG_AMLOGIC_DEBUG_TEST)
int idle_long_debug;
EXPORT_SYMBOL(idle_long_debug);
#endif

static void __maybe_unused idle_in_hook(void *data, int *state, struct cpuidle_device *dev)
{
	int cpu;
	struct lockup_info *info;

	if (!idle_check_en)
		return;

	cpu = smp_processor_id();
	info = per_cpu_ptr(infos, cpu);
	info->idle_enter_time = sched_clock();

#if IS_ENABLED(CONFIG_AMLOGIC_DEBUG_TEST)
	if (idle_long_debug) {
		idle_long_debug = 0;
		mdelay(5000);
	}
#endif
}

static void __maybe_unused idle_out_hook(void *data, int state, struct cpuidle_device *dev)
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
#if IS_ENABLED(CONFIG_AMLOGIC_DEBUG_IOTRACE)
	char buf[50];

	if (ramoops_io_en && (ramoops_trace_mask & 0x8)) {
		memset(buf, 0, sizeof(buf));
		snprintf(buf, 50, "smc-in smcid:%lx, val:%lx, noret:%d", smcid, val, noret);
		aml_pstore_write(AML_PSTORE_TYPE_SMC, buf, 0);
	}
#endif

	if (noret)
		return;

	if (!initialized || !smc_check_en)
		return;

	cpu = smp_processor_id();
	info = per_cpu_ptr(infos, cpu);

	info->smc_enter_time = sched_clock();
	info->smc_enter_task_pid = current->pid;
	memcpy(info->smc_enter_task_comm, current->comm, TASK_COMM_LEN);
	info->curr_smc_a0 = smcid;
	info->curr_smc_a1 = val;

	if (irq_check_en) {
		memset(info->smc_enter_trace_entries, 0, sizeof(info->smc_enter_trace_entries));
		info->smc_enter_trace_entries_nr = stack_trace_save(info->smc_enter_trace_entries, ENTRY, 0);
	}
}

static void smc_out_hook(unsigned long smcid, unsigned long val)
{
	int cpu;
	struct lockup_info *info;
#if IS_ENABLED(CONFIG_AMLOGIC_DEBUG_IOTRACE)
	char buf[50];

	if (ramoops_io_en && (ramoops_trace_mask & 0x8)) {
		memset(buf, 0, sizeof(buf));
		snprintf(buf, 50, "smc-out smcid:%lx, val:%lx", smcid, val);
		aml_pstore_write(AML_PSTORE_TYPE_SMC, buf, 0);
	}
#endif

	if (!initialized || !smc_check_en)
		return;

	cpu = smp_processor_id();
	info = per_cpu_ptr(infos, cpu);

	info->smc_enter_time = 0;

}

#if IS_ENABLED(CONFIG_AMLOGIC_DEBUG_TEST)
int smc_long_debug;
EXPORT_SYMBOL(smc_long_debug);
#endif

void __arm_smccc_smc_glue(unsigned long a0, unsigned long a1,
			unsigned long a2, unsigned long a3, unsigned long a4,
			unsigned long a5, unsigned long a6, unsigned long a7,
			struct arm_smccc_res *res, struct arm_smccc_quirk *quirk)
{
	int not_in_idle = current->pid != 0;

#if IS_ENABLED(CONFIG_AMLOGIC_DEBUG_TEST)
	if (smc_long_debug) {
		smc_in_hook(a0, a1, is_noret_smcid(a0));
		local_irq_disable();
		smc_long_debug = 0;
		mdelay(30000);
		local_irq_enable();
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

static void __maybe_unused irq_trace_start(unsigned long flags)
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

static void __maybe_unused irq_trace_stop(unsigned long flags)
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

static void __maybe_unused sched_show_task_hook(void *data, struct task_struct *p)
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

static void __dump_cpu_task(int cpu)
{
	pr_info("Task dump for CPU %d:\n", cpu);
	sched_show_task(cpu_curr(cpu));
}

void set_lockup_hook(void (*func)(int cpu))
{
	if (lockup_hook) {
		pr_warn("lockup_hook try set to:%pS, but already set:%pS\n",
			lockup_hook,
			func);
		return;
	}

	lockup_hook = func;
	pr_info("lockup_hook set to:%pS\n", lockup_hook);
}
EXPORT_SYMBOL(set_lockup_hook);

#ifdef CONFIG_AMLOGIC_ARMV8_AARCH32
bool is_vmalloc_addr(const void *x)
{
	unsigned long addr = (unsigned long)kasan_reset_tag(x);

	return addr >= VMALLOC_START && addr < VMALLOC_END;
}

int is_vmalloc_or_module_addr(const void *x)
{
#if defined(CONFIG_MODULES) && defined(MODULES_VADDR)
	unsigned long addr = (unsigned long)kasan_reset_tag(x);

	if (addr >= MODULES_VADDR && addr < MODULES_END)
		return 1;
#endif
	return is_vmalloc_addr(x);
}

void show_vmalloc_pfn(struct pt_regs *regs)
{
	int i;
	struct page *page;

	for (i = 0; i < 16; i++) {
		if (is_vmalloc_or_module_addr((void *)regs->uregs[i])) {
			page = vmalloc_to_page((void *)regs->uregs[i]);
			if (!page)
				continue;
			pr_info("R%-2d : %08lx, PFN:%5lx\n",
				i, regs->uregs[i], page_to_pfn(page));
		}
	}
}

static int show_data_valid(unsigned long reg)
{
	struct page *page;

	if (reg < (unsigned long)PAGE_OFFSET)
		return 0;
	else if (reg <= (unsigned long)high_memory)
		return 1;

	page = vmalloc_to_page((const void *)reg);
	if (page && pfn_valid(page_to_pfn(page)))
		return 1;

	return 0;
}

static void show_data(unsigned long addr, int nbytes, const char *name)
{
	int	i, j;
	int	nlines;
	u32	*p;

	/*
	 * don't attempt to dump non-kernel addresses or
	 * values that are probably just small negative numbers
	 */
	if (addr < PAGE_OFFSET || addr > -256UL)
		return;
	/*
	 * Treating data in general purpose register as an address
	 * and dereferencing it is quite a dangerous behaviour,
	 * especially when it belongs to secure monotor region or
	 * ioremap region(for arm64 vmalloc region is already filtered
	 * out), which can lead to external abort on non-linefetch and
	 * can not be protected by probe_kernel_address.
	 * We need more strict filtering rules
	 */

#ifdef CONFIG_AMLOGIC_SECMON
	/*
	 * filter out secure monitor region
	 */
	if (addr <= (unsigned long)high_memory)
		if (within_secmon_region(addr)) {
			pr_info("\n%s: %#lx S\n", name, addr);
			return;
		}
#endif

	pr_info("\n%s: %#lx:\n", name, addr);

	if (!show_data_valid((unsigned long)(addr + nbytes / 2)))
		return;
	/*
	 * round address down to a 32 bit boundary
	 * and always dump a multiple of 32 bytes
	 */
	p = (u32 *)(addr & ~(sizeof(u32) - 1));
	nbytes += (addr & (sizeof(u32) - 1));
	nlines = (nbytes + 31) / 32;

	for (i = 0; i < nlines; i++) {
		/*
		 * just display low 16 bits of address to keep
		 * each line of the dump < 80 characters
		 */
		pr_cont("%04lx ", (unsigned long)p & 0xffff);
		for (j = 0; j < 8; j++) {
			u32 data = 0;

			if (get_kernel_nofault(data, p))
				pr_cont(" ********");
			else
				pr_cont(" %08x", data);
			++p;
		}
		pr_cont("\n");
	}
}

static void show_user_data(unsigned long addr, int nbytes, const char *name)
{
	int	i, j;
	int	nlines;
	u32	*p;

	if (!access_ok((void *)addr, nbytes))
		return;

#ifdef CONFIG_AMLOGIC_MODIFY
	/*
	 * Treating data in general purpose register as an address
	 * and dereferencing it is quite a dangerous behaviour,
	 * especially when it is an address belonging to secure
	 * region or ioremap region, which can lead to external
	 * abort on non-linefetch and can not be protected by
	 * probe_kernel_address.
	 * We need more strict filtering rules
	 */

#ifdef CONFIG_AMLOGIC_SEC
	/*
	 * filter out secure monitor region
	 */
	if (addr <= (unsigned long)high_memory)
		if (within_secmon_region(addr)) {
			pr_info("\n%s: %#lx S\n", name, addr);
			return;
		}
#endif

	/*
	 * filter out ioremap region
	 */
	if (addr >= VMALLOC_START && addr <= VMALLOC_END)
		if (!pfn_valid(vmalloc_to_pfn((void *)addr))) {
			pr_info("\n%s: %#lx V\n", name, addr);
			return;
		}
#endif

	pr_info("\n%s: %#lx:\n", name, addr);

	/*
	 * round address down to a 32 bit boundary
	 * and always dump a multiple of 32 bytes
	 */
	p = (u32 *)(addr & ~(sizeof(u32) - 1));
	nbytes += (addr & (sizeof(u32) - 1));
	nlines = (nbytes + 31) / 32;

	for (i = 0; i < nlines; i++) {
		/*
		 * just display low 16 bits of address to keep
		 * each line of the dump < 80 characters
		 */
		pr_info("%04lx ", (unsigned long)p & 0xffff);
		for (j = 0; j < 8; j++) {
			u32 data;
			int bad;

			bad = __get_user(data, p);
			if (bad) {
				if (j != 7)
					pr_cont(" ********");
				else
					pr_cont(" ********\n");
			} else {
				if (j != 7)
					pr_cont(" %08x", data);
				else
					pr_cont(" %08x\n", data);
			}
			++p;
		}
	}
}

static void show_extra_register_data(struct pt_regs *regs, int nbytes)
{
	show_data(regs->ARM_pc - nbytes, nbytes * 2, "PC");
	show_data(regs->ARM_lr - nbytes, nbytes * 2, "LR");
	show_data(regs->ARM_sp - nbytes, nbytes * 2, "SP");
	show_data(regs->ARM_ip - nbytes, nbytes * 2, "IP");
	show_data(regs->ARM_fp - nbytes, nbytes * 2, "FP");
	show_data(regs->ARM_r0 - nbytes, nbytes * 2, "R0");
	show_data(regs->ARM_r1 - nbytes, nbytes * 2, "R1");
	show_data(regs->ARM_r2 - nbytes, nbytes * 2, "R2");
	show_data(regs->ARM_r3 - nbytes, nbytes * 2, "R3");
	show_data(regs->ARM_r4 - nbytes, nbytes * 2, "R4");
	show_data(regs->ARM_r5 - nbytes, nbytes * 2, "R5");
	show_data(regs->ARM_r6 - nbytes, nbytes * 2, "R6");
	show_data(regs->ARM_r7 - nbytes, nbytes * 2, "R7");
	show_data(regs->ARM_r8 - nbytes, nbytes * 2, "R8");
	show_data(regs->ARM_r9 - nbytes, nbytes * 2, "R9");
	show_data(regs->ARM_r10 - nbytes, nbytes * 2, "R10");
}

static void show_user_extra_register_data(struct pt_regs *regs, int nbytes)
{
	show_user_data(regs->ARM_pc - nbytes, nbytes * 2, "PC");
	show_user_data(regs->ARM_lr - nbytes, nbytes * 2, "LR");
	show_user_data(regs->ARM_sp - nbytes, nbytes * 2, "SP");
	show_user_data(regs->ARM_ip - nbytes, nbytes * 2, "IP");
	show_user_data(regs->ARM_fp - nbytes, nbytes * 2, "FP");
	show_user_data(regs->ARM_r0 - nbytes, nbytes * 2, "R0");
	show_user_data(regs->ARM_r1 - nbytes, nbytes * 2, "R1");
	show_user_data(regs->ARM_r2 - nbytes, nbytes * 2, "R2");
	show_user_data(regs->ARM_r3 - nbytes, nbytes * 2, "R3");
	show_user_data(regs->ARM_r4 - nbytes, nbytes * 2, "R4");
	show_user_data(regs->ARM_r5 - nbytes, nbytes * 2, "R5");
	show_user_data(regs->ARM_r6 - nbytes, nbytes * 2, "R6");
	show_user_data(regs->ARM_r7 - nbytes, nbytes * 2, "R7");
	show_user_data(regs->ARM_r8 - nbytes, nbytes * 2, "R8");
	show_user_data(regs->ARM_r9 - nbytes, nbytes * 2, "R9");
	show_user_data(regs->ARM_r10 - nbytes, nbytes * 2, "R10");
}

void show_extra_reg_data(struct pt_regs *regs)
{
	if (!user_mode(regs))
		show_extra_register_data(regs, 128);
	else
		show_user_extra_register_data(regs, 128);
	pr_info("\n");
}

void show_regs_32(struct pt_regs *regs)
{
	pr_info("PC is at %pS\n", (void *)instruction_pointer(regs));
	pr_info("LR is at %pS\n", (void *)regs->ARM_lr);
	pr_info("pc : [<%08lx>]    lr : [<%08lx>]    psr: %08lx\n",
	       regs->ARM_pc, regs->ARM_lr, regs->ARM_cpsr);
	pr_info("sp : %08lx  ip : %08lx  fp : %08lx\n",
	       regs->ARM_sp, regs->ARM_ip, regs->ARM_fp);
	pr_info("r10: %08lx  r9 : %08lx  r8 : %08lx\n",
		regs->ARM_r10, regs->ARM_r9,
		regs->ARM_r8);
	pr_info("r7 : %08lx  r6 : %08lx  r5 : %08lx  r4 : %08lx\n",
		regs->ARM_r7, regs->ARM_r6,
		regs->ARM_r5, regs->ARM_r4);
	pr_info("r3 : %08lx  r2 : %08lx  r1 : %08lx  r0 : %08lx\n",
		regs->ARM_r3, regs->ARM_r2,
		regs->ARM_r1, regs->ARM_r0);

	show_vmalloc_pfn(regs);
	show_extra_reg_data(regs);
}
#endif

#if (defined CONFIG_ARM64) || (defined CONFIG_AMLOGIC_ARMV8_AARCH32)
void fiq_debug_entry(void)
{
	int cpu;
	struct fiq_regs *pregs  = NULL;
	struct fiq_regs fiq_regs;
	struct pt_regs regs;

	cpu = get_cpu();
	put_cpu();

	pregs = (struct fiq_regs *)(fiq_virt_addr +
						cpu * fiq_percpu_size);
	memcpy(&fiq_regs, pregs, sizeof(struct fiq_regs));

#ifdef CONFIG_ARM64
	/* sp_el0 */
	memcpy(&regs.regs[0], &fiq_regs.regs[0], 31 * sizeof(uint64_t));
	regs.pc = fiq_regs.pc;
	regs.pstate = fiq_regs.pstate;
#elif CONFIG_AMLOGIC_ARMV8_AARCH32
	regs.ARM_cpsr = (unsigned long)fiq_regs.pstate;
	regs.ARM_pc = (unsigned long)fiq_regs.pc;
	regs.ARM_sp = (unsigned long)fiq_regs.regs[19];
	regs.ARM_lr = (unsigned long)fiq_regs.regs[18];
	regs.ARM_ip = (unsigned long)fiq_regs.regs[12];
	regs.ARM_fp = (unsigned long)fiq_regs.regs[11];
	regs.ARM_r10 = (unsigned long)fiq_regs.regs[10];
	regs.ARM_r9 = (unsigned long)fiq_regs.regs[9];
	regs.ARM_r8 = (unsigned long)fiq_regs.regs[8];
	regs.ARM_r7 = (unsigned long)fiq_regs.regs[7];
	regs.ARM_r6 = (unsigned long)fiq_regs.regs[6];
	regs.ARM_r5 = (unsigned long)fiq_regs.regs[5];
	regs.ARM_r4 = (unsigned long)fiq_regs.regs[4];
	regs.ARM_r3 = (unsigned long)fiq_regs.regs[3];
	regs.ARM_r2 = (unsigned long)fiq_regs.regs[2];
	regs.ARM_r1 = (unsigned long)fiq_regs.regs[1];
	regs.ARM_r0 = (unsigned long)fiq_regs.regs[0];
#endif

	pr_err("\n\n--------fiq_dump CPU%d--------\n\n", cpu);
	if (fiq_check_show_regs_en) {
#ifdef CONFIG_ARM64
		show_regs(&regs);
#elif CONFIG_AMLOGIC_ARMV8_AARCH32
		show_regs_32(&regs);
#endif
	}
	dump_stack();
	pr_info("\n");
	while (1)
		;
}
#endif

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

			pr_err("in smc, smc_enter_time=%llu.%06lu (%lx %lx) task:%d/%s\n",
			       ts, rem_nsec / 1000, info->curr_smc_a0, info->curr_smc_a1,
			       info->smc_enter_task_pid, info->smc_enter_task_comm);

			if (irq_check_en)
				stack_trace_print(info->smc_enter_trace_entries,
						  info->smc_enter_trace_entries_nr, 0);
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

		if (lockup_hook)
			lockup_hook(cpu);
	}

	pr_err("%s: lock_cpu=[%d] --------- END --------\n", __func__, lock_cpu);

	local_irq_restore(flags);

#if (defined CONFIG_ARM64) || (defined CONFIG_AMLOGIC_ARMV8_AARCH32)
	if (fiq_virt_addr && fiq_check_en) {
		struct arm_smccc_res res;

		arm_smccc_smc(FIQ_DEBUG_SMC_CMD, FIQ_SEND_SMC_ARG, lock_cpu, 0, 0, 0, 0, 0, &res);
		mdelay(1000);
	}
#endif
}
EXPORT_SYMBOL(pr_lockup_info);

static void __maybe_unused
rt_throttle_func(void *data, int cpu, u64 clock, ktime_t rt_period, u64 rt_runtime,
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

static void __maybe_unused
debug_hook_func(void *data, struct irq_data *magic, const struct cpumask *arg1,
			    u64 *arg2, bool force, void __iomem *base,
			    void __iomem *rbase, u64 redist_stride)
{
	if ((unsigned long)magic != DEBUG_HOOK_MAGIC)
		return;

	switch ((enum debug_hook_type)arg1) {
	case DEBUG_HOOK_IRQ_START:
		irq_trace_start((unsigned long)arg2);
		break;
	case DEBUG_HOOK_IRQ_STOP:
		irq_trace_stop((unsigned long)arg2);
		break;
	default:
		pr_err("bad debug_hook_type:%d\n", (enum debug_hook_type)arg1);
		break;
	}
}

//todo after submit abi:__traceiter_android_vh_ftrace_format_check
/*
static void ftrace_format_check_hook(void *data, bool *ftrace_check)
{
	*ftrace_check = 0;
}
*/

#if (defined CONFIG_ARM64) || (defined CONFIG_AMLOGIC_ARMV8_AARCH32)
static void fiq_debug_addr_init(void)
{
	union {
		struct arm_smccc_res smccc;
		struct fiq_smc_ret result;
	} res;
	phys_addr_t fiq_phy_addr = 0;

	if (!fiq_check_en)
		return;

	memset(&res, 0, sizeof(res));
	arm_smccc_smc(FIQ_DEBUG_SMC_CMD, FIQ_INIT_SMC_ARG,
		(unsigned long)fiq_debug_entry, 0, 0, 0, 0, 0, &res.smccc);
	fiq_phy_addr = (phys_addr_t)res.result.phy_addr;
	fiq_buf_size = res.result.buf_size;
	fiq_percpu_size = res.result.percpu_size;

	/* Current ATF version does not support FIQ DEBUG */
	if (fiq_phy_addr == 0 || fiq_phy_addr == 0xFFFFFFFF) {
		pr_err("invalid fiq_phy_addr\n");
		return;
	}

	fiq_virt_addr = ioremap_cache(fiq_phy_addr,
					fiq_buf_size);
	if (!fiq_virt_addr) {
		pr_err("failed to map fiq_virt_addr space\n");
		return;
	}

	pr_info("fiq_phy_addr:%lx, fiq_buf_size: %lx, fiq_virt_addr:%lx\n",
		(unsigned long)fiq_phy_addr, fiq_buf_size,
		(unsigned long)fiq_virt_addr);

	if (fiq_virt_addr)
		memset(fiq_virt_addr, 0, fiq_buf_size);
}
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
#ifdef CONFIG_ANDROID_VENDOR_HOOKS
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

#ifndef CONFIG_FUNCTION_GRAPH_TRACER
	register_trace_android_rvh_gic_v3_set_affinity(debug_hook_func, NULL);
#endif

	/* CONFIG_IRQSOFF_TRACER is not enabled, can't use below function */
	//register_trace_android_rvh_irqs_disable(irq_trace_start, NULL);
	//register_trace_android_rvh_irqs_enable(irq_trace_stop, NULL);

	//todo after submit abi:__traceiter_android_vh_ftrace_format_check
	//register_trace_android_vh_ftrace_format_check(ftrace_format_check_hook, NULL);
#endif
	initialized = 1;

#if IS_ENABLED(CONFIG_AMLOGIC_DEBUG_TEST)
	irq_check_en = 1;
#endif

#if (defined CONFIG_ARM64) || (defined CONFIG_AMLOGIC_ARMV8_AARCH32)
	fiq_debug_addr_init();
#endif

	return 0;
}

static ssize_t write_sysrq_trigger(struct file *file, const char __user *buf,
				size_t count, loff_t *ppos)
{
	if (count) {
		char c;

		if (get_user(c, buf))
			return -EFAULT;
		if (c == 'x') {
			local_irq_disable();
			pr_emerg("trigger hardlockup\n");
			while (1)
				;
		}
	}
	return count;
}

static const struct file_operations sysrq_trigger_debug_ops = {
	.write		= write_sysrq_trigger,
	.llseek		= noop_llseek,
};

int aml_debug_init(void)
{
	static struct dentry *debug_lockup;

	debug_lockup = debugfs_create_dir("aml_debug", NULL);
	if (IS_ERR_OR_NULL(debug_lockup)) {
		pr_warn("failed to create aml_debug\n");
		debug_lockup = NULL;
		return -ENOMEM;
	}
	debugfs_create_file("sysrq-trigger", S_IFREG | 0664,
			    debug_lockup, NULL, &sysrq_trigger_debug_ops);
	return 0;
}
