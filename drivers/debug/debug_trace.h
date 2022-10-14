/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#undef TRACE_SYSTEM
#define TRACE_SYSTEM amlogic_debug

#if !defined(_TRACE_AMLOGIC_DEBUG_H) || defined(TRACE_HEADER_MULTI_READ)
#include <linux/tracepoint.h>

DECLARE_TRACE(inject_irq_hooks,
	      TP_PROTO(int dummy),
	      TP_ARGS(dummy));

#ifdef CONFIG_AMLOGIC_HARDLOCKUP_DETECTOR
DECLARE_TRACE(inject_pr_lockup_info,
	      TP_PROTO(int dummy),
	      TP_ARGS(dummy));
#endif

#ifdef CONFIG_AMLOGIC_DEBUG_FTRACE_PSTORE
DECLARE_TRACE(inject_pstore_io_save,
	      TP_PROTO(int dummy),
	      TP_ARGS(dummy));
#endif

#endif /* _TRACE_AMLOGIC_DEBUG_H */

/* This part must be outside protection */
#include <trace/define_trace.h>

