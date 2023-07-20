// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
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
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/irqreturn.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/highmem.h>

#include <linux/cpu.h>
#include <linux/smp.h>
#include <linux/kallsyms.h>
#include <linux/of_irq.h>
#include <linux/interrupt.h>
#include <linux/amlogic/page_trace.h>
#include "ddr_port.h"
#include "dmc_monitor.h"

#define DMC_PROT0_RANGE		((0x0030  << 2))
#define DMC_PROT0_CTRL		((0x0031  << 2))
#define DMC_PROT0_CTRL1		((0x0032  << 2))

#define DMC_PROT1_RANGE		((0x0033  << 2))
#define DMC_PROT1_CTRL		((0x0034  << 2))
#define DMC_PROT1_CTRL1		((0x0035  << 2))

#define DMC_PROT_VIO_0		((0x0036  << 2))
#define DMC_PROT_VIO_1		((0x0037  << 2))

#define DMC_PROT_VIO_2		((0x0038  << 2))
#define DMC_PROT_VIO_3		((0x0039  << 2))

#define DMC_PROT_IRQ_CTRL	((0x003a  << 2))
#define DMC_IRQ_STS		((0x003b  << 2))

#define DMC_IRQ_STS_C2		((0x0030  << 2))

#define DMC_SEC_STATUS		((0x009a  << 2))
#define DMC_VIO_ADDR0		((0x009b  << 2))
#define DMC_VIO_ADDR1		((0x009c  << 2))
#define DMC_VIO_ADDR2		((0x009d  << 2))
#define DMC_VIO_ADDR3		((0x009e  << 2))

static size_t c2_dmc_dump_reg(char *buf)
{
	size_t sz = 0, i;
	unsigned long val;

	for (i = 0; i < 2; i++) {
		val = dmc_prot_rw(dmc_mon->io_mem1, DMC_PROT0_RANGE + (i * 12), 0, DMC_READ);
		sz += sprintf(buf + sz, "DMC_PROT%zu_RANGE:%lx\n", i, val);
		val = dmc_prot_rw(dmc_mon->io_mem1, DMC_PROT0_CTRL + (i * 12), 0, DMC_READ);
		sz += sprintf(buf + sz, "DMC_PROT%zu_CTRL:%lx\n", i, val);
		val = dmc_prot_rw(dmc_mon->io_mem1, DMC_PROT0_CTRL1 + (i * 12), 0, DMC_READ);
		sz += sprintf(buf + sz, "DMC_PROT%zu_CTRL1:%lx\n", i, val);
	}
	for (i = 0; i < 4; i++) {
		val = dmc_prot_rw(dmc_mon->io_mem1, DMC_PROT_VIO_0 + (i << 2), 0, DMC_READ);
		sz += sprintf(buf + sz, "DMC_PROT_VIO_%zu:%lx\n", i, val);
	}
	val = dmc_prot_rw(dmc_mon->io_mem1, DMC_PROT_IRQ_CTRL, 0, DMC_READ);
	sz += sprintf(buf + sz, "DMC_PROT_IRQ_CTRL:%lx\n", val);
	val = dmc_prot_rw(dmc_mon->io_mem1, 0 - DMC_IRQ_STS_C2, 0, DMC_READ);
	sz += sprintf(buf + sz, "DMC_IRQ_STS:%lx\n", val);

	return sz;
}

static int get_c2_port(unsigned int awuser)
{
	awuser >>= 2;
	if (awuser >= 128)
		return ((awuser >> 4) & 0x7) + 16;
	if (awuser <= 5)
		return awuser - 1;
	if (awuser <= 10)
		return awuser - 3;
	return awuser - 4;
}

static void check_violation(struct dmc_monitor *mon, void *data)
{
	char rw = 'n';
	int port, subport;
	unsigned long irqreg;
	unsigned long addr = 0, status = 0;
	char title[10];
	unsigned int vio_bit = 19;
	unsigned int awuser = 0;

	switch (mon->chip) {
	case DMC_TYPE_TM2:
		vio_bit = 19;
		break;
	case DMC_TYPE_C2:
		vio_bit = 30;
		break;
	default:
		break;
	}

	if (mon->chip == DMC_TYPE_C2)
		irqreg = dmc_prot_rw(dmc_mon->io_mem1, 0 - DMC_IRQ_STS_C2, 0, DMC_READ);
	else
		irqreg = dmc_prot_rw(dmc_mon->io_mem1, DMC_IRQ_STS, 0, DMC_READ);

	if (irqreg & DMC_WRITE_VIOLATION) {
		status = dmc_prot_rw(dmc_mon->io_mem1, DMC_PROT_VIO_1, 0, DMC_READ);
		addr = dmc_prot_rw(dmc_mon->io_mem1, DMC_PROT_VIO_0, 0, DMC_READ);
		rw = 'w';
	} else if (irqreg & DMC_READ_VIOLATION) {
		status = dmc_prot_rw(dmc_mon->io_mem1, DMC_PROT_VIO_3, 0, DMC_READ);
		addr = dmc_prot_rw(dmc_mon->io_mem1, DMC_PROT_VIO_2, 0, DMC_READ);
		rw = 'r';
	}

	/* clear irq */
	if (dmc_mon->debug & DMC_DEBUG_SUSPEND)
		irqreg &= ~0x04;
	else
		irqreg |= 0x04;		/* en */
	dmc_prot_rw(dmc_mon->io_mem1, DMC_PROT_IRQ_CTRL, irqreg, DMC_WRITE);

	if (!(status & (0x3 << vio_bit)))
		return;

	if (addr > mon->addr_end)
		return;

	if (mon->chip == DMC_TYPE_C2) {
		awuser = (status >> 16) & 0x3ff;
		port = get_c2_port(awuser);
	} else {
		port = (status >> 11) & 0x1f;
	}
	subport = (status >> 6) & 0xf;

	if (dmc_violation_ignore(title, addr, status | (0x3 << vio_bit), port, subport, rw))
		return;

#if IS_ENABLED(CONFIG_EVENT_TRACING)
	if (mon->debug & DMC_DEBUG_TRACE) {
		show_violation_mem_trace_event(addr, status, port, subport, rw);
		return;
	}
#endif
	show_violation_mem_printk(title, addr, status, port, subport, rw);
}

static void c2_dmc_mon_irq(struct dmc_monitor *mon, void *data)
{
	check_violation(mon, data);

}

static int c2_dmc_mon_set(struct dmc_monitor *mon)
{
	unsigned long value, end;
	unsigned int wb;

	/* aligned to 64KB */
	wb = mon->addr_start & 0x01;
	end = ALIGN(mon->addr_end, DMC_ADDR_SIZE);
	value = (mon->addr_start >> 16) | ((end >> 16) << 16);
	dmc_prot_rw(dmc_mon->io_mem1, DMC_PROT0_RANGE, value, DMC_WRITE);

	dmc_prot_rw(dmc_mon->io_mem1, DMC_PROT0_CTRL, mon->device, DMC_WRITE);

	value = wb << 25;
	if (dmc_mon->debug & DMC_DEBUG_WRITE)
		value |= (1 << 24);
	/* if set, will be crash when read access */
	if (dmc_mon->debug & DMC_DEBUG_READ)
		value |= (1 << 26);
	dmc_prot_rw(dmc_mon->io_mem1, DMC_PROT0_CTRL1, value, DMC_WRITE);

	if (dmc_mon->debug & DMC_DEBUG_SUSPEND)
		value = 0X3;
	else
		value = 0X7;
	dmc_prot_rw(dmc_mon->io_mem1, DMC_PROT_IRQ_CTRL, value, DMC_WRITE);

	pr_emerg("range:%08lx - %08lx, device:%llx\n",
		 mon->addr_start, mon->addr_end, mon->device);
	return 0;
}

void c2_dmc_mon_disable(struct dmc_monitor *mon)
{
	dmc_prot_rw(dmc_mon->io_mem1, DMC_PROT0_RANGE, 0, DMC_WRITE);
	dmc_prot_rw(dmc_mon->io_mem1, DMC_PROT0_CTRL, 0, DMC_WRITE);
	dmc_prot_rw(dmc_mon->io_mem1, DMC_PROT0_CTRL1, 0, DMC_WRITE);
	dmc_prot_rw(dmc_mon->io_mem1, DMC_PROT_IRQ_CTRL, 0, DMC_WRITE);
	mon->device     = 0;
	mon->addr_start = 0;
	mon->addr_end   = 0;
}

static int c2_reg_analysis(char *input, char *output)
{
	unsigned long status, vio_reg0, vio_reg1, vio_reg2, vio_reg3;
	int port, subport, count = 0;
	unsigned long addr;
	char rw = 'n';

	if (sscanf(input, "%lx %lx %lx %lx %lx",
			 &status, &vio_reg0, &vio_reg1,
			 &vio_reg2, &vio_reg3) != 5) {
		pr_emerg("%s parma input error, buf:%s\n", __func__, input);
		return 0;
	}

	if (status & 0x1) { /* read */
		addr = vio_reg2;
		port = get_c2_port((vio_reg3 >> 16) & 0x3ff);
		subport = (vio_reg3 >> 6) & 0xf;
		rw = 'r';

		count += sprintf(output + count, "DMC READ:");
		count += sprintf(output + count, "addr=%09lx port=%s sub=%s\n",
				 addr, to_ports(port), to_sub_ports_name(port, subport, rw));
	}

	if (status & 0x2) { /* write */
		addr = vio_reg0;
		port = get_c2_port((vio_reg1 >> 16) & 0x3ff);
		subport = (vio_reg1 >> 6) & 0xf;
		rw = 'w';
		count += sprintf(output + count, "DMC WRITE:");
		count += sprintf(output + count, "addr=%09lx port=%s sub=%s\n",
				 addr, to_ports(port), to_sub_ports_name(port, subport, rw));
	}
	return count;
}

static int c2_dmc_reg_control(char *input, char control, char *output)
{
	int count = 0, i;
	unsigned long val;

	switch (control) {
	case 'a':	/* analysis sec vio reg */
		count = c2_reg_analysis(input, output);
		break;
	case 'c':	/* clear sec statue reg */
		dmc_prot_rw(NULL, 0x1000 + DMC_SEC_STATUS, 0x3, DMC_WRITE);
		break;
	case 'd':	/* dump sec vio reg */
		count += sprintf(output + count, "DMC SEC INFO:\n");
		val = dmc_prot_rw(NULL, 0x1000 + DMC_SEC_STATUS, 0, DMC_READ);
		count += sprintf(output + count, "DMC_SEC_STATUS:%lx\n", val);
		for (i = 0; i < 4; i++) {
			val = dmc_prot_rw(NULL, 0x1000 + DMC_VIO_ADDR0 + (i << 2), 0, DMC_READ);
			count += sprintf(output + count, "DMC_VIO_ADDR%d:%lx\n", i, val);
		}
		break;
	default:
		break;
	}

	return count;
}

struct dmc_mon_ops c2_dmc_mon_ops = {
	.handle_irq = c2_dmc_mon_irq,
	.set_monitor = c2_dmc_mon_set,
	.disable    = c2_dmc_mon_disable,
	.dump_reg   = c2_dmc_dump_reg,
	.reg_control   = c2_dmc_reg_control,
};
