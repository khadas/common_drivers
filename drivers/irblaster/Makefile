# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

MODULE_NAME = amlogic-irblaster
obj-$(CONFIG_AMLOGIC_IRBLASTER) = $(MODULE_NAME).o
$(MODULE_NAME)-y += core.o
$(MODULE_NAME)-y += sysfs.o
$(MODULE_NAME)-y += irblaster-meson.o

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)
