/* SPDX-License-Identifier: GPL-2.0 */
#if !defined(_TRACE_DMC_MONITOR_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_DMC_MONITOR_H

#include <linux/tracepoint.h>

#undef TRACE_SYSTEM
#define TRACE_SYSTEM dmc_monitor

#include <asm/memory.h>
struct page;
char *to_ports(int id);
char *to_sub_ports_name(int mid, int sid, char rw);
unsigned long read_violation_mem(unsigned long addr, char rw);
unsigned long dmc_get_page_trace(struct page *page);

TRACE_EVENT(dmc_violation,

	TP_PROTO(unsigned long addr, unsigned long status, int port, int sub_port, char rw),

	TP_ARGS(addr, status, port, sub_port, rw),

	TP_STRUCT__entry(
		__field(unsigned long, addr)
		__field(unsigned long, status)
		__field(int, port)
		__field(int, sub_port)
		__field(char, rw)
		__field(int, bd)
		__field(int, sb)
		__field(int, lru)
		__field(unsigned long, page_trace)
	),

	TP_fast_assign(
		__entry->addr = addr;
		__entry->status = status;
		__entry->port = port;
		__entry->sub_port = sub_port;
		__entry->rw = rw;
		__entry->bd = PageBuddy(phys_to_page(__entry->addr));
		__entry->sb = PageSlab(phys_to_page(__entry->addr));
		__entry->lru = PageLRU(phys_to_page(__entry->addr));
		__entry->page_trace = dmc_get_page_trace(phys_to_page(__entry->addr));
	),

	TP_printk("addr=%09lx val=%016lx s=%08lx port=%s sub=%s rw:%c bd:%d sb:%d lru:%d a:%ps",
		  __entry->addr,
		  read_violation_mem(__entry->addr, __entry->rw),
		  __entry->status,
		  to_ports(__entry->port),
		  to_sub_ports_name(__entry->port, __entry->sub_port, __entry->rw),
		  __entry->rw,
		  __entry->bd,
		  __entry->sb,
		  __entry->lru,
		  (void *)__entry->page_trace)
);

#endif /*  _TRACE_DMC_MONITOR_H */

#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH ../../drivers/memory_debug/ddr_tool/
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE dmc_trace

/* This part must be outside protection */
#include <trace/define_trace.h>
