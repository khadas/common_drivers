# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

MODULE_NAME = amlogic-i2c
obj-$(CONFIG_AMLOGIC_I2C_MESON) = $(MODULE_NAME).o
$(MODULE_NAME)-y += i2c-meson.o

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)
