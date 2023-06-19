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
#ifndef _DOS_REGISTER_H_
#define _DOS_REGISTER_H_

#include <linux/platform_device.h>
#include "../../include/regs/dos_registers.h"
#include <linux/amlogic/media/utils/vdec_reg.h>


typedef enum {
	DOS_BUS,
	MAX_REG_BUS
} MM_BUS_ENUM;

struct bus_reg_desc {
	char const *reg_name;
	s32 reg_compat_offset;
};


#define WRITE_VREG(addr, val) write_dos_reg_comp(addr, val)

#define READ_VREG(addr) read_dos_reg_comp(addr)


int read_dos_reg(ulong addr);
void write_dos_reg(ulong addr, int val);
void dos_reg_write_bits(unsigned int reg, u32 val, int start, int len);

int read_dos_reg_comp(ulong addr);
void write_dos_reg_comp(ulong addr, int val);


#define WRITE_VREG_BITS(r, val, start, len) dos_reg_write_bits(r, val, start, len)
#define CLEAR_VREG_MASK(r, mask)   write_dos_reg_comp(r, read_dos_reg_comp(r) & ~(mask))
#define SET_VREG_MASK(r, mask)     write_dos_reg_comp(r, read_dos_reg_comp(r) | (mask))


#define READ_HREG(r) read_dos_reg_comp((r) | 0x1000)
#define WRITE_HREG(r, val) write_dos_reg_comp((r) | 0x1000, val)
#define WRITE_HREG_BITS(r, val, start, len) \
	dos_reg_write_bits((r) | 0x1000, val, start, len)
//#define SET_HREG_MASK(r, mask) codec_set_dosbus_mask((r) | 0x1000, mask)
//#define CLEAR_HREG_MASK(r, mask) codec_clear_dosbus_mask((r) | 0x1000, mask)

//##############################################################

typedef void (*reg_compat_func)(struct bus_reg_desc *, MM_BUS_ENUM bus);

void t3_mm_registers_compat(struct bus_reg_desc *desc, MM_BUS_ENUM bs);
void s5_mm_registers_compat(struct bus_reg_desc *desc, MM_BUS_ENUM bs);


ulong dos_reg_compat_convert(ulong addr);

void write_dos_reg(ulong addr, int val);

int read_dos_reg(ulong addr);


int dos_register_probe(struct platform_device *pdev, reg_compat_func reg_compat_fn);


#endif
