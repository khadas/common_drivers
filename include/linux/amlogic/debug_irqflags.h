/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __AMLOGIC_DEBUG_IRQFLAGS_H
#define __AMLOGIC_DEBUG_IRQFLAGS_H

typedef	void (*irq_trace_fn_t)(unsigned long flags);

extern irq_trace_fn_t irq_trace_start_hook;
extern irq_trace_fn_t irq_trace_stop_hook;

void irq_trace_start_gki_builtin(unsigned long flags);
void irq_trace_stop_gki_builtin(unsigned long flags);

static inline void __nocfi irq_trace_start_glue(unsigned long flags)
{
#if defined(CONFIG_AMLOGIC_DEBUG) || (defined(CONFIG_AMLOGIC_DEBUG_MODULE) && defined(MODULE))
	/* builtin mode or gki mode build with module */
	if (irq_trace_start_hook)
		irq_trace_start_hook(flags);
#elif defined(CONFIG_AMLOGIC_BGKI_DEBUG_MISC)
	/* gki mode build with builtin */
	irq_trace_start_gki_builtin(flags);
#endif
}

static inline void __nocfi irq_trace_stop_glue(unsigned long flags)
{
#if defined(CONFIG_AMLOGIC_DEBUG) || (defined(CONFIG_AMLOGIC_DEBUG_MODULE) && defined(MODULE))
	/* builtin mode or gki mode build with module */
	if (irq_trace_stop_hook)
		irq_trace_stop_hook(flags);
#elif defined(CONFIG_AMLOGIC_BGKI_DEBUG_MISC)
	/* gki mode build with builtin */
	irq_trace_stop_gki_builtin(flags);
#endif
}

void set_lockup_hook(void (*lockup_hook)(int cpu));

#endif
