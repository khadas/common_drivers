# SPDX-License-Identifier: GPL-2.0

MODULE_NAME = amlogic_usb_crg
obj-$(CONFIG_AMLOGIC_CRG)      += $(MODULE_NAME).o
$(MODULE_NAME)-y += crg.o

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)