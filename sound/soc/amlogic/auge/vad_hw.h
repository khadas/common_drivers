/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __VAD_HW_H__
#define __VAD_HW_H__
#include <linux/types.h>

#include "regs.h"
#include "iomap.h"

enum vad_int_mode {
	INT_MODE_FS,
	INT_MODE_FLAG
};

void vad_set_ram_coeff(int len, int *params);

void vad_set_de_params(int len, int *params);

void vad_set_pwd(void);

void vad_set_cep(void);

void vad_set_src(int src, bool vad_top);

void vad_set_in(void);

void vad_set_enable(bool enable, bool vad_top);

void vad_force_clk_to_oscin(bool force, bool vad_top);

void vad_irq_clr(enum vad_int_mode mode);

#endif
