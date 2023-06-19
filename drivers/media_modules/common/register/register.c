/*
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Description:
 */
#include <linux/io.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/of_address.h>
#include <linux/io.h>
#include <linux/regmap.h>
#include <linux/device.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include "../chips/decoder_cpu_ver_info.h"
#include "register.h"

static void __iomem *reg_base[MAX_REG_BUS];
struct bus_reg_desc *reg_desc[MAX_REG_BUS];

#define CODEC_REG_READ_DEBUG  0x01
#define CODEC_REG_WRITE_DEBUG 0x02
#define CODEC_REG_MAP_DEBUG   0x08

static u32 register_debug;
module_param(register_debug, uint, 0664);

void registers_offset_config(struct bus_reg_desc *offset_from, s32 val, u32 size)
{
	u32 i;

	for (i = 0; i < size; i++) {
		offset_from[i].reg_compat_offset = val;
	}
}
EXPORT_SYMBOL(registers_offset_config);


void s5_mm_registers_compat(struct bus_reg_desc *desc, MM_BUS_ENUM bs)
{
	if (bs == DOS_BUS) {
		printk("s5 dos register compat\n");
		registers_offset_config(&desc[HEVC_ASSIST_AMR1_INT0],
			-(0x0025 - 0x0015),
			(HEVC_ASSIST_MBX_SSEL - HEVC_ASSIST_AMR1_INT0 + 1));

		registers_offset_config(&desc[HEVC_ASSIST_TIMER0_LO],
			-(0x0060 - 0x0036),
			(HEVC_ASSIST_DMA_INT_MSK2 - HEVC_ASSIST_TIMER0_LO + 1));

		registers_offset_config(&desc[HEVC_ASSIST_MBOX0_IRQ_REG],
			-(0x0070 - 0x0040),
			(HEVC_ASSIST_AXI_STATUS2_LO - HEVC_ASSIST_MBOX0_IRQ_REG + 1));

		registers_offset_config(&desc[HEVC_ASSIST_SCRATCH_0],
			-(0x00c0 - 0x00b0),
			(HEVC_ASSIST_SCRATCH_N - HEVC_ASSIST_SCRATCH_0) + 1);

		registers_offset_config(&desc[AV1D_IPP_DIR_CFG], -(0x0490 - 0x0419), 1);
	}
}

void t3_mm_registers_compat(struct bus_reg_desc *desc, MM_BUS_ENUM bs)
{
	registers_offset_config(&desc[AV1D_IPP_DIR_CFG], -(0x0490 - 0x0419), 1);
}


//###############################################################################
ulong dos_reg_compat_convert(ulong addr)
{
		s32 reg_compat_offset;
		struct bus_reg_desc *dos_desc = reg_desc[DOS_BUS];

		if (addr & NEW_REG_CHECK_MASK) {
			addr &= (~NEW_REG_CHECK_MASK);
			reg_compat_offset = 0;
		} else {
			reg_compat_offset = dos_desc[addr].reg_compat_offset;
		}

		return (addr + reg_compat_offset);
	}
EXPORT_SYMBOL(dos_reg_compat_convert);

void write_dos_reg(ulong addr, int val)
{
	void __iomem * reg_adr;
	s32 reg_compat_offset;
	struct bus_reg_desc *dos_desc = reg_desc[DOS_BUS];

	if (addr & NEW_REG_CHECK_MASK) {
		addr &= (~NEW_REG_CHECK_MASK);
		reg_compat_offset = 0;
	} else {
		reg_compat_offset = dos_desc[addr].reg_compat_offset;
	}

	if ((reg_compat_offset + addr) < 0) {
		pr_err("write dos reg out of range, name %s, addr %lx, offset %d\n",
			dos_desc[addr].reg_name, addr, reg_compat_offset);
		return;
	}

	reg_adr = reg_base[DOS_BUS] + ((reg_compat_offset + addr) << 2);

	if (register_debug) {
		if (register_debug & CODEC_REG_WRITE_DEBUG) {
			pr_info("write_reg(%lx, %x)\n", (reg_compat_offset + addr), val);
		}
		if (register_debug & CODEC_REG_MAP_DEBUG) {
			pr_info("%s %px, name %s, addr %lx, offset %d\n",
				__func__, reg_adr, dos_desc[addr].reg_name, addr, reg_compat_offset);
		}
	}

	writel(val, reg_adr);
}
EXPORT_SYMBOL(write_dos_reg);

int read_dos_reg(ulong addr)
{
	void __iomem * reg_adr;
	int value;
	struct bus_reg_desc *dos_desc = reg_desc[DOS_BUS];
	s32 reg_compat_offset;

	if (addr & NEW_REG_CHECK_MASK) {
		addr &= (~NEW_REG_CHECK_MASK);
		reg_compat_offset = 0;
	} else {
		reg_compat_offset = dos_desc[addr].reg_compat_offset;
	}

	if ((reg_compat_offset + addr) < 0) {
		pr_err("read dos reg out of range, name %s, addr %lx, offset %d\n",
			dos_desc[addr].reg_name, addr, reg_compat_offset);
		return -ENXIO;
	}

	reg_adr = reg_base[DOS_BUS] + ((reg_compat_offset + addr) << 2);

	value = readl(reg_adr);

	if (register_debug) {
		if (register_debug & CODEC_REG_READ_DEBUG) {
			pr_info("read_reg(%lx) = %x\n", (reg_compat_offset + addr), value);
		}
		if (register_debug & CODEC_REG_MAP_DEBUG) {
			pr_info("%s %px, name %s, addr %lx, offset %d\n",
				__func__, reg_adr, dos_desc[addr].reg_name, addr, reg_compat_offset);
		}
	}
	return value;
}
EXPORT_SYMBOL(read_dos_reg);

int read_dos_reg_comp(ulong addr)
{
	if (is_support_new_dos_dev())
		return read_dos_reg(addr);
	else
		return aml_read_dosbus((uint)addr);
}
EXPORT_SYMBOL(read_dos_reg_comp);

void write_dos_reg_comp(ulong addr, int val)
{
	if (is_support_new_dos_dev())
		write_dos_reg(addr, val);
	else
		aml_write_dosbus((uint)addr, (uint)val);
}
EXPORT_SYMBOL(write_dos_reg_comp);


void dos_reg_write_bits(unsigned int reg, u32 val, int start, int len)
{
	u32 to_val = read_dos_reg_comp(reg);
	u32 mask = (((1L << (len)) - 1) << (start));

	to_val &= ~mask;
	to_val |= (val << start) & mask;
	write_dos_reg_comp(reg, to_val);
}
EXPORT_SYMBOL(dos_reg_write_bits);

int dos_register_probe(struct platform_device *pdev, reg_compat_func reg_compat_fn)
{
	u32 i;
	struct resource res;
	u32 res_size;

	if (pdev == NULL) {
		pr_info("no dev found, dos_register can not map\n");
		return -ENODEV;
	}

	for (i = 0; i < MAX_REG_BUS; i++) {
		if (of_address_to_resource(pdev->dev.of_node, i, &res)) {
			pr_err("of_address_to_resource failed\n");
			return -EINVAL;
		}

		res_size = resource_size(&res);
		reg_base[i] = ioremap(res.start, res_size);

		pr_info("%s, res start %llx, end %llx, iomap: %px\n",
			__func__, (unsigned long long)res.start,
			(unsigned long long)res.end, reg_base[i]);

		reg_desc[i] = (struct bus_reg_desc *)kzalloc(res_size *
			sizeof(struct bus_reg_desc), GFP_KERNEL);
		if (!reg_desc[i])
			pr_err("Warn: dos regs offset table alloc failed\n");

		if (reg_compat_fn)
			reg_compat_fn(reg_desc[i], i);
	}

	return 0;
}
EXPORT_SYMBOL(dos_register_probe);


