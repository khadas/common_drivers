# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

MODULE_NAME = amlogic-reset
obj-$(CONFIG_AMLOGIC_RESET_MESON) = $(MODULE_NAME).o
$(MODULE_NAME)-y += main.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_RESET_MESON) += reset-meson.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_DOS_RESET_MESON) += reset-meson-dos.o

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)
