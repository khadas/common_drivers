// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/io.h>
#include <linux/amlogic/media/registers/register_map.h>
#include <linux/amlogic/media/registers/regs/ao_regs.h>
#include <linux/amlogic/power_domain.h>
#include "vicp_reg.h"
#include "vicp_log.h"

#define VICPBUS_REG_ADDR(reg) ((reg) << 2)

static int check_map_flag(void)
{
	int ret = 0;

	if (vicp_reg_map) {
		ret = 1;
	} else {
		vicp_print(VICP_ERROR, "%s: reg map failed\n", __func__);
		ret = 0;
	}

	return ret;
}

u32 vicp_reg_read(u32 reg)
{
	u32 addr = 0;
	u32 val = 0;

	addr = VICPBUS_REG_ADDR(reg);
	if (check_map_flag())
		val = readl(vicp_reg_map + addr);

	return val;
}

void vicp_reg_write(u32 reg, u32 val)
{
	u32 addr = 0;

	addr = VICPBUS_REG_ADDR(reg);
	if (check_map_flag())
		writel(val, vicp_reg_map + addr);
}

u32 vicp_vcbus_read(u32 reg)
{
	u32 val = 0;

#ifdef CONFIG_AMLOGIC_IOMAP
	val = aml_read_vcbus(reg);
#endif
	return val;
};

void vicp_vcbus_write(u32 reg, u32 val)
{
#ifdef CONFIG_AMLOGIC_IOMAP
	aml_write_vcbus(reg, val);
#endif
}

u32 vicp_reg_get_bits(u32 reg, const u32 start, const u32 len)
{
	u32 val;

	val = (vicp_reg_read(reg) >> (start)) & ((1L << (len)) - 1);
	return val;
}

void vicp_reg_set_bits(u32 reg, const u32 value, const u32 start, const u32 len)
{
	u32 set_val = (vicp_reg_read(reg) & ~(((1L << len) - 1) << start)) |
			((value & ((1L << len) - 1)) << start);
	vicp_reg_write(reg, set_val);
}

void vicp_reg_write_addr(u64 addr, u64 data)
{
	writel(data, ((void __iomem *)(phys_to_virt(addr))));
}

u64 vicp_reg_read_addr(u64 addr)
{
	u32 val;

	val = readl((void __iomem *)(phys_to_virt(addr)));

	return (val & 0xffffffff);
}
