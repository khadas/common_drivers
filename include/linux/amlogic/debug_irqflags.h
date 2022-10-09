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

extern irq_trace_fn_t irq_trace_start_hook_gki_builtin;
extern irq_trace_fn_t irq_trace_stop_hook_gki_builtin;

static inline void __nocfi irq_trace_start_glue(unsigned long flags)
{
#if defined(CONFIG_AMLOGIC_DEBUG) || (defined(CONFIG_AMLOGIC_DEBUG_MODULE) && defined(MODULE))
	/* builtin code or gki module */
	if (irq_trace_start_hook)
		irq_trace_start_hook(flags);
#else
	/* gki builtin */
	if (irq_trace_start_hook_gki_builtin)
		irq_trace_start_hook_gki_builtin(flags);
#endif
}

static inline void __nocfi irq_trace_stop_glue(unsigned long flags)
{
#if defined(CONFIG_AMLOGIC_DEBUG) || (defined(CONFIG_AMLOGIC_DEBUG_MODULE) && defined(MODULE))
	/* builtin code or gki module */
	if (irq_trace_stop_hook)
		irq_trace_stop_hook(flags);
#else
	/* gki builtin */
	if (irq_trace_stop_hook_gki_builtin)
		irq_trace_stop_hook_gki_builtin(flags);
#endif
}
#endif

#endif
