# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

MODULE_NAME = amlogic-debug
PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)

obj-$(CONFIG_AMLOGIC_DEBUG) = $(MODULE_NAME).o
$(MODULE_NAME)-y = main.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_DEBUG_LOCKUP) += debug_lockup.o cpu_mhz.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_DEBUG_ATRACE) += meson_atrace.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_DEBUG_PRINTK) += debug_printk.o

#autotest
# obj-m += debug_auto_test.o
