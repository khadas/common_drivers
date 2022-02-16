# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"$(MODULE_NAME): \" fmt"
ccflags-y += $(PR_FMT_DEFINE)

ccflags-y += -I$(srctree)/mm

obj-$(CONFIG_AMLOGIC_PAGE_TRACE)        += page_trace.o
obj-$(CONFIG_AMLOGIC_SLAB_TRACE)        += slab_trace.o
obj-$(CONFIG_AMLOGIC_CMA)               += aml_cma.o
obj-$(CONFIG_AMLOGIC_MEM_DEBUG)         += mem_debug.o
