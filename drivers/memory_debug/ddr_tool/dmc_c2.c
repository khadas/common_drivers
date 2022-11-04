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

#define DMC_SEC_STATUS		((0x00fa  << 2))
#define DMC_VIO_ADDR0		((0x00fb  << 2))
#define DMC_VIO_ADDR1		((0x00fc  << 2))
#define DMC_VIO_ADDR2		((0x00fd  << 2))
#define DMC_VIO_ADDR3		((0x00fe  << 2))

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
	int port, subport;
	unsigned long irqreg;
	unsigned long addr = 0, status = 0;
	char title[10] = "";
	char id_str[4];
	struct page *page;
	struct page_trace *trace;
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
	}

	if (irqreg & DMC_READ_VIOLATION) {
		status = dmc_prot_rw(dmc_mon->io_mem1, DMC_PROT_VIO_3, 0, DMC_READ);
		addr = dmc_prot_rw(dmc_mon->io_mem1, DMC_PROT_VIO_2, 0, DMC_READ);
	}

	if (!(status & (1 << vio_bit)))
		return;

	if (addr > mon->addr_end)
		return;
	/* ignore violation on same page/same port */
	if ((addr & PAGE_MASK) == mon->last_addr &&
		status == mon->last_status) {
		mon->same_page++;
		return;
	}
	/* ignore cma driver pages */
	page = phys_to_page(addr);
	trace = dmc_find_page_base(page);
	if (trace && trace->migrate_type == MIGRATE_CMA) {
		if (mon->debug & DMC_DEBUG_CMA)
			sprintf(title, "%s", "_CMA");
		else
			return;
	}
	if (mon->chip == DMC_TYPE_C2) {
		awuser = (status >> 16) & 0x3ff;
		port = get_c2_port(awuser);
	} else {
		port = (status >> 11) & 0x1f;
	}
	subport = (status >> 6) & 0xf;

	if ((mon->debug & DMC_DEBUG_CMA) == 0) {
		if (strstr(to_ports(port), "EMMC"))
			return;
		if (strstr(to_ports(port), "USB"))
			return;
		if (strstr(to_ports(port), "ETH"))
			return;
	}

	pr_emerg(DMC_TAG "%s, addr:%08lx, s:%08lx, ID:%s, sub:%s, c:%ld, d:%p, u:%x\n",
			title, addr, status, to_ports(port),
			to_sub_ports(port, subport, id_str),
			mon->same_page, data, awuser);
	show_violation_mem(addr);
	if (!port) /* dump stack for CPU write */
		dump_stack();

	mon->same_page   = 0;
	mon->last_addr   = addr & PAGE_MASK;
	mon->last_status = status;
}

static void c2_dmc_mon_irq(struct dmc_monitor *mon, void *data)
{
	unsigned long value;

	if (mon->chip == DMC_TYPE_C2)
		value = dmc_prot_rw(dmc_mon->io_mem1, 0 - DMC_IRQ_STS_C2, 0, DMC_READ);
	else
		value = dmc_prot_rw(dmc_mon->io_mem1, DMC_IRQ_STS, 0, DMC_READ);
	if (in_interrupt()) {
		if (value & DMC_WRITE_VIOLATION)
			check_violation(mon, data);

		/* check irq flags just after IRQ handler */
		mod_delayed_work(system_wq, &mon->work, 0);
	}
	/* clear irq */
	value &= 0x03;		/* irq flags */
	value |= 0x04;		/* en */
	dmc_prot_rw(dmc_mon->io_mem1, DMC_PROT_IRQ_CTRL, value, DMC_WRITE);
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
	// dmc_prot_rw(dmc_mon->io_mem1, DMC_PROT1_RANGE, value, DMC_WRITE);

	dmc_prot_rw(dmc_mon->io_mem1, DMC_PROT0_CTRL, mon->device, DMC_WRITE);
	dmc_prot_rw(dmc_mon->io_mem1, DMC_PROT0_CTRL1, wb << 25 | 1 << 24, DMC_WRITE);
	dmc_prot_rw(dmc_mon->io_mem1, DMC_PROT_IRQ_CTRL, 0x06, DMC_WRITE);

	// dmc_prot_rw(dmc_mon->io_mem1, DMC_PROT1_CTRL, mon->device, DMC_WRITE);
	// dmc_prot_rw(dmc_mon->io_mem1, DMC_PROT1_CTRL1, 1 << 24, DMC_WRITE);


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

struct dmc_mon_ops c2_dmc_mon_ops = {
	.handle_irq = c2_dmc_mon_irq,
	.set_monitor = c2_dmc_mon_set,
	.disable    = c2_dmc_mon_disable,
	.dump_reg   = c2_dmc_dump_reg,
};
