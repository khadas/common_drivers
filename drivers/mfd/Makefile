# SPDX-License-Identifier: (GPL-2.0+ OR MIT)
#
# Makefile for multifunction miscellaneous devices
#

MODULE_NAME = amlogic-mfd

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)

obj-$(CONFIG_AMLOGIC_MFD)		+= $(MODULE_NAME).o
$(MODULE_NAME)-y += main.o

$(MODULE_NAME)-$(CONFIG_AMLOGIC_MFD_PMIC6B) += meson_pmic6b.o
