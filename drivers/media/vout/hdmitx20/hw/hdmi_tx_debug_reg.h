/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __HDMI_TX_DEBUG_REG_H_
#define __HDMI_TX_DEBUG_REG_H_

#include "hdmi_tx_hw.h"
#include "hdmi_tx_reg.h"
#include "mach_reg.h"
#include "reg_sc2.h"

#define REGS_END 0xffffffff

struct hdmitx_dbgreg_s {
	unsigned int (*rd_reg_func)(unsigned int add);
	unsigned int (*get_reg_paddr)(unsigned int add);
	char *name;
	unsigned int *reg;
};

struct hdmitx_dbgreg_s **hdmitx_get_dbgregs(enum amhdmitx_chip_e type);

#endif
