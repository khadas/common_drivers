/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _DVB_REG_H_
#define _DVB_REG_H_

#include <linux/amlogic/iomap.h>
#include <linux/amlogic/cpu_version.h>
#include <linux/platform_device.h>

/* S1A move the demux configuration register to the REE side */
#define CFG_DEMUX_OFFSET					0x320

void aml_write_self(unsigned int reg, unsigned int val);
int aml_read_self(unsigned int reg);
int init_demux_addr(struct platform_device *pdev);

void aml_sys_write(unsigned int reg, unsigned int val);
int aml_sys_read(unsigned int reg);

#define WRITE_CBUS_REG(_r, _v)   aml_write_self((_r), _v)
#define READ_CBUS_REG(_r)        aml_read_self((_r))

#define WRITE_SYS_REG(_r, _v)   aml_sys_write((_r), _v)
#define READ_SYS_REG(_r)        aml_sys_read((_r))
#endif
