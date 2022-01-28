/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __AMLOGIC_DEBUG_IRQFLAGS_H
#define __AMLOGIC_DEBUG_IRQFLAGS_H

#ifdef CONFIG_AMLOGIC_DEBUG_LOCKUP
typedef	void (*irq_trace_fn_t)(unsigned long flags);

extern irq_trace_fn_t irq_trace_start_hook;
extern irq_trace_fn_t irq_trace_stop_hook;

static inline void __nocfi irq_trace_start_glue(unsigned long flags)
{
	if (irq_trace_start_hook)
		irq_trace_start_hook(flags);
}

static inline void __nocfi irq_trace_stop_glue(unsigned long flags)
{
	if (irq_trace_stop_hook)
		irq_trace_stop_hook(flags);
}
#endif

#endif
