# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

MODULE_NAME = amlogic-uart
PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE) -D SKIP_LOCKUP_CHECK
obj-$(CONFIG_AMLOGIC_SERIAL_MESON) = $(MODULE_NAME).o
$(MODULE_NAME)-y += meson_uart.o

obj-$(CONFIG_AMLOGIC_SERIAL_EARLY_CONSOLE) += meson_early_console.o
