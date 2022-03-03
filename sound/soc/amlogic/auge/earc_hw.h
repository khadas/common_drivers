/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __EARC_HW_H__
#define __EARC_HW_H__

#include "regs.h"
#include "iomap.h"

void earcrx_cmdc_init(void);
void earcrx_dmac_init(void);
void earc_arc_init(void);
void earc_rx_enable(bool enable);
#endif
