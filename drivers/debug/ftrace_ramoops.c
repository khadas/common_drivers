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
#include <trace/hooks/sched.h>
#include <linux/amlogic/aml_iotrace.h>
#include <linux/amlogic/gki_module.h>

static DEFINE_PER_CPU(int, en);

#define IRQ_D	1

int ramoops_io_skip = 1;
EXPORT_SYMBOL(ramoops_io_skip);
module_param(ramoops_io_skip, int, 0664);

void notrace __nocfi pstore_io_save(unsigned long reg, unsigned long val, unsigned int flag,
									unsigned long *irq_flags)
{
	int cpu;
	struct io_trace_data data;

	if (!aml_iotrace_en)
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

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "next_task:%s,pid:%d", next->comm, next->pid);

	aml_pstore_write(AML_PSTORE_TYPE_SCHED, buf, 0);

}

int ftrace_ramoops_init(void)
{
	register_trace_android_rvh_schedule(schedule_hook, NULL);

	return 0;
}
