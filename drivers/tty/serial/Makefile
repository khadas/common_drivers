# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

obj-$(CONFIG_AMLOGIC_SERIAL_MESON) += meson_uart.o

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)
