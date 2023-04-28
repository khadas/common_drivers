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
#include <trace/hooks/sched.h>
#include <linux/amlogic/aml_iotrace.h>
#include <linux/amlogic/gki_module.h>

static DEFINE_PER_CPU(int, en);

#define IRQ_D	1

void notrace __nocfi pstore_io_save(unsigned long reg, unsigned long val, unsigned int flag,
									unsigned long *irq_flags)
{
	int cpu;
	struct io_trace_data data;

	if (!ramoops_io_en || !(ramoops_trace_mask & 0x1))
		return;

	if ((flag == PSTORE_FLAG_IO_R || flag == PSTORE_FLAG_IO_W) && IRQ_D)
		local_irq_save(*irq_flags);

	data.flag = flag;
	data.reg = (unsigned int)page_to_phys(vmalloc_to_page((const void *)reg)) +
				offset_in_page(reg);
	data.val = (unsigned int)val;

	switch (ramoops_io_skip) {
	case 1:
		data.ip = CALLER_ADDR1 | flag;
		data.parent_ip = CALLER_ADDR2;
		break;
	case 2:
		data.ip = CALLER_ADDR2 | flag;
		data.parent_ip = CALLER_ADDR3;
		break;
	case 3:
		data.ip = CALLER_ADDR3 | flag;
		data.parent_ip = CALLER_ADDR4;
		break;
	default:
		data.ip = CALLER_ADDR0 | flag;
		data.parent_ip = CALLER_ADDR1;
		break;
	}

	cpu = raw_smp_processor_id();
	if (unlikely(oops_in_progress) || unlikely(per_cpu(en, cpu))) {
		if ((flag == PSTORE_FLAG_IO_R || flag == PSTORE_FLAG_IO_W) && IRQ_D)
			local_irq_restore(*irq_flags);
			return;
	}

	per_cpu(en, cpu) = 1;

	aml_pstore_write(AML_PSTORE_TYPE_IO, (void *)&data, sizeof(struct io_trace_data));

	per_cpu(en, cpu) = 0;

	if ((flag == PSTORE_FLAG_IO_R_END || flag == PSTORE_FLAG_IO_W_END) &&
	    IRQ_D)
		local_irq_restore(*irq_flags);
}
EXPORT_SYMBOL(pstore_io_save);

static void schedule_hook(void *data, struct task_struct *prev, struct task_struct *next,
							struct rq *rq)
{
	char buf[100];

	if (!(ramoops_trace_mask & 0x2))
		return;

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "next_task:%s,pid:%d", next->comm, next->pid);

	aml_pstore_write(AML_PSTORE_TYPE_SCHED, buf, 0);

}

static struct kprobe clk_disable_kp = {
	.symbol_name = "clk_core_disable",
};

static struct kprobe clk_unprepare_kp = {
	.symbol_name = "clk_core_unprepare",
};

static struct kprobe power_off_kp = {
	.symbol_name = "_genpd_power_off",
};

static int clk_handler_pre(struct kprobe *p, struct pt_regs *regs)
{
	/*
	 * set clk_core_disable/clk_core_unprepare x0 to 0, do not disable clk
	 */
#if defined(CONFIG_ARM64)
	regs->regs[0] = 0;
#elif defined(CONFIG_ARM)
	regs->ARM_r0 = 0;
#endif

	return 0;
}

static int power_handler_pre(struct kprobe *p, struct pt_regs *regs)
{
	/*
	 * set _genpd_power_off (struct _genpd_power_off *)x0->power_off to 0, do not power_off
	 */
#if defined(CONFIG_ARM64)
	((struct generic_pm_domain *)regs->regs[0])->power_off = 0;
	pr_info("skip all pd power_off,%s pd will not power_off\n",
		((struct generic_pm_domain *)regs->regs[0])->name);
#elif defined(CONFIG_ARM)
	((struct generic_pm_domain *)regs->ARM_r0)->power_off = 0;
	pr_info("skip all pd power_off,%s pd will not power_off\n",
		((struct generic_pm_domain *)regs->ARM_r0)->name);
#endif
	return 0;
}

#if IS_MODULE(CONFIG_AMLOGIC_DEBUG_IOTRACE)
/*
 * struct regmap_mmio_context sync from common/drivers/base/regmap/regmap-mmio.c
 */
struct regmap_mmio_context {
	void __iomem *regs;
	unsigned int val_bytes;
	bool relaxed_mmio;

	bool attached_clk;
	struct clk *clk;

	void (*reg_write)(struct regmap_mmio_context *ctx,
			  unsigned int reg, unsigned int val);
	unsigned int (*reg_read)(struct regmap_mmio_context *ctx,
					unsigned int reg);
};

struct regmap_data {
	unsigned long reg;
	unsigned long val;
	unsigned long flag;
};

static int regmap_read_entry_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	struct regmap_data *data;
#if defined(CONFIG_ARM64)
	unsigned long reg = (unsigned long)
			(((struct regmap_mmio_context *)regs->regs[0])->regs + regs->regs[1]);
#elif defined(CONFIG_ARM)
	unsigned long reg = (unsigned long)
			(((struct regmap_mmio_context *)regs->ARM_r0)->regs + regs->ARM_r1);
#endif
	/*
	 * #define pstore_ftrace_io_rd(reg)		\
	 * unsigned long irqflg;					\
	 * pstore_io_save(reg, 0, PSTORE_FLAG_IO_R, &irqflg)
	 */
	pstore_ftrace_io_rd(reg);

	data = (struct regmap_data *)ri->data;
	(*data).reg = reg;
	(*data).flag = irqflg;

	return 0;
}

static int regmap_read_ret_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	struct regmap_data data = *(struct regmap_data *)ri->data;
	unsigned long irqflg = data.flag;

	/*
	 * #define pstore_ftrace_io_rd_end(reg)	\
	 * pstore_io_save(reg, 0, PSTORE_FLAG_IO_R_END, &irqflg)
	 */
	pstore_ftrace_io_rd_end(data.reg);

	return 0;
}

static struct kretprobe regmap_mmio_read_krp = {
	.handler = regmap_read_ret_handler,
	.entry_handler = regmap_read_entry_handler,
	.data_size = sizeof(struct regmap_data),
};

static int regmap_write_entry_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	struct regmap_data *data;
#if defined(CONFIG_ARM64)
	unsigned long reg = (unsigned long)
			(((struct regmap_mmio_context *)regs->regs[0])->regs + regs->regs[1]);
	unsigned long val = (unsigned long)regs->regs[2];
#elif defined(CONFIG_ARM)
	unsigned long reg = (unsigned long)
			(((struct regmap_mmio_context *)regs->ARM_r0)->regs + regs->ARM_r1);
	unsigned long val = (unsigned long)regs->ARM_r2;
#endif
	/*
	 * #define pstore_ftrace_io_wr(reg, val)	\
	 * unsigned long irqflg;					\
	 * pstore_io_save(reg, val, PSTORE_FLAG_IO_W, &irqflg)
	 */
	pstore_ftrace_io_wr(reg, val);

	data = (struct regmap_data *)ri->data;
	(*data).reg = reg;
	(*data).val = val;
	(*data).flag = irqflg;

	return 0;
}

static int regmap_write_ret_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	struct regmap_data data = *(struct regmap_data *)ri->data;
	unsigned long irqflg = data.flag;

	/*
	 * #define pstore_ftrace_io_wr_end(reg, val)	\
	 * pstore_io_save(reg, val, PSTORE_FLAG_IO_W_END, &irqflg)
	 */
	pstore_ftrace_io_wr_end(data.reg, data.val);

	return 0;
}

static struct kretprobe regmap_mmio_write_krp = {
	.handler = regmap_write_ret_handler,
	.entry_handler = regmap_write_entry_handler,
	.data_size = sizeof(struct regmap_data),
};
#endif /* CONFIG_AMLOGIC_DEBUG_IOTRACE */

int ftrace_ramoops_init(void)
{
	int ret;

	if (!ramoops_io_en)
		return 0;

#ifdef CONFIG_ANDROID_VENDOR_HOOKS
	register_trace_android_rvh_schedule(schedule_hook, NULL);
#endif

#if IS_MODULE(CONFIG_AMLOGIC_DEBUG_IOTRACE)
	regmap_mmio_read_krp.kp.symbol_name = "regmap_mmio_read";
	ret = register_kretprobe(&regmap_mmio_read_krp);
	if (ret < 0) {
		pr_err("register kretprobe 'regmap_mmio_read' failed, returned %d\n", ret);
		return ret;
	}

	regmap_mmio_write_krp.kp.symbol_name = "regmap_mmio_write";
	ret = register_kretprobe(&regmap_mmio_write_krp);
	if (ret < 0) {
		pr_err("register kretprobe 'regmap_mmio_write' failed, returned %d\n", ret);
		return ret;
	}
#endif

	if (meson_clk_debug) {
		clk_disable_kp.pre_handler = clk_handler_pre;
		ret = register_kprobe(&clk_disable_kp);
		if (ret < 0) {
			pr_err("register 'clk_core_disable' failed, returned %d\n", ret);
			return ret;
		}

		clk_unprepare_kp.pre_handler = clk_handler_pre;
		ret = register_kprobe(&clk_unprepare_kp);
		if (ret < 0) {
			pr_err("register 'clk_core_unprepare' failed, returned %d\n", ret);
			return ret;
		}
	}

	if (meson_pd_debug) {
		power_off_kp.pre_handler = power_handler_pre;
		ret = register_kprobe(&power_off_kp);
		if (ret < 0) {
			pr_err("register '_genpd_power_off' failed, returned %d\n", ret);
			return ret;
		}
	}

	return 0;
}
