// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/mm.h>
#include <linux/reboot.h>
#include <linux/sysrq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <trace/hooks/gic_v3.h>

#include "lockup.h"

void irq_trace_start_gki_builtin(unsigned long flags)
{
	trace_android_rvh_gic_v3_set_affinity((void *)DEBUG_HOOK_MAGIC,
					      (void *)DEBUG_HOOK_IRQ_START,
					      (void *)flags,
					      0, 0, 0, 0);
}

void irq_trace_stop_gki_builtin(unsigned long flags)
{
	trace_android_rvh_gic_v3_set_affinity((void *)DEBUG_HOOK_MAGIC,
					      (void *)DEBUG_HOOK_IRQ_STOP,
					      (void *)flags,
					      0, 0, 0, 0);
}
