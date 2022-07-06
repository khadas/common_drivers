// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/clk.h>
#include <linux/console.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/serial.h>
#include <linux/serial_core.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/pinctrl/consumer.h>
#include <linux/trace_events.h>
#include <trace/hooks/printk.h>
#include <linux/amlogic/debug_printk.h>

static int task_name_len = 8;
module_param(task_name_len, int, 0664);
MODULE_PARM_DESC(task_name_len, "\n limit print task_name length\n");

static bool print_cnt_bool;
module_param(print_cnt_bool, bool, 0664);
MODULE_PARM_DESC(print_cnt_bool, "\n statistical log printing\n");

struct printk_info_t {
	u8	print_cnt;
	u8	cpu_id;
	u8	irqflags;
	char	task_name[TASK_COMM_LEN];
	char	format_buf[50];
	u16	format_len;
	int	preempt_count;
};

static struct printk_info_t printk_info;
static size_t print_irq(u8 pc, u8 irq_flags, char *buf)
{
	char hardsoft_irq;
	char need_resched;
	char irqs_off;
	int hardirq;
	int softirq;
	int nmi;

	nmi = irq_flags & TRACE_FLAG_NMI;
	hardirq = irq_flags & TRACE_FLAG_HARDIRQ;
	softirq = irq_flags & TRACE_FLAG_SOFTIRQ;

	irqs_off =
		(irq_flags & TRACE_FLAG_IRQS_OFF) ? 'd' :
		(irq_flags & TRACE_FLAG_IRQS_NOSUPPORT) ? 'X' :
		'.';

	switch (irq_flags & (TRACE_FLAG_NEED_RESCHED |
				TRACE_FLAG_PREEMPT_RESCHED)) {
	case TRACE_FLAG_NEED_RESCHED | TRACE_FLAG_PREEMPT_RESCHED:
		need_resched = 'N';
		break;
	case TRACE_FLAG_NEED_RESCHED:
		need_resched = 'n';
		break;
	case TRACE_FLAG_PREEMPT_RESCHED:
		need_resched = 'p';
		break;
	default:
		need_resched = '.';
		break;
	}

	hardsoft_irq =
		(nmi && hardirq)     ? 'Z' :
		nmi                  ? 'z' :
		(hardirq && softirq) ? 'H' :
		hardirq              ? 'h' :
		softirq              ? 's' :
				       '.';

	if (pc)
		return sprintf(buf, "[%c%c%c%x]", irqs_off, need_resched, hardsoft_irq, pc);
	else
		return sprintf(buf, "[%c%c%c%c]", irqs_off, need_resched, hardsoft_irq, '.');
}

static size_t print_format_info(struct printk_info_t *info, char *buf)
{
	size_t len = 0;

	len = sprintf(buf, "[@%d %.*s]", info->cpu_id, task_name_len, info->task_name);
	len += print_irq(info->preempt_count, info->irqflags, buf + len);
	if (print_cnt_bool)
		len += sprintf(buf + len, "[%02d]", info->print_cnt);
	buf[len++] = ' ';
	buf[len] = '\0';
	return len;
}

static void printk_get_info(unsigned long irq_flags)
{
	printk_info.print_cnt++;
	printk_info.print_cnt %= 100;
	printk_info.cpu_id = smp_processor_id();
	get_task_comm(printk_info.task_name, current);
	printk_info.preempt_count = preempt_count();
	printk_info.irqflags =
		(irqs_disabled_flags(irq_flags) ? TRACE_FLAG_IRQS_OFF : 0) |
		((printk_info.preempt_count & NMI_MASK) ? TRACE_FLAG_NMI     : 0) |
		((printk_info.preempt_count & HARDIRQ_MASK) ? TRACE_FLAG_HARDIRQ : 0) |
		((printk_info.preempt_count & SOFTIRQ_OFFSET) ? TRACE_FLAG_SOFTIRQ : 0) |
		(tif_need_resched() ? TRACE_FLAG_NEED_RESCHED : 0) |
		(test_preempt_need_resched() ? TRACE_FLAG_PREEMPT_RESCHED : 0);
}

void debug_printk_modify_len(u16 *reserve_size, unsigned long irqflags, unsigned int max_line)
{
	size_t info_len = 0;

	printk_get_info(irqflags);
	info_len = print_format_info(&printk_info, printk_info.format_buf);
	*reserve_size += info_len;
	if (*reserve_size > max_line)
		*reserve_size = max_line;
	printk_info.format_len = info_len;
}
EXPORT_SYMBOL(debug_printk_modify_len);

void debug_printk_insert_info(char *text_buf, u16 *text_len)
{
	memmove(text_buf + printk_info.format_len, text_buf, *text_len);
	memcpy(text_buf, printk_info.format_buf, printk_info.format_len);
	*text_len += printk_info.format_len;
}
EXPORT_SYMBOL(debug_printk_insert_info);
