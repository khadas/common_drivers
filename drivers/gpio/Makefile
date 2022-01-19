# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

ccflags-y += -I$(srctree)/drivers/gpio

MODULE_NAME = amlogic-gpiolib
obj-$(CONFIG_AMLOGIC_GPIOLIB) = $(MODULE_NAME).o
$(MODULE_NAME)-y += gpiolib.o

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)
