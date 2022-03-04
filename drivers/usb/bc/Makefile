# SPDX-License-Identifier: GPL-2.0

MODULE_NAME = amlogic_usb_bc
obj-$(CONFIG_AMLOGIC_USB_SUPPORT)      += $(MODULE_NAME).o
$(MODULE_NAME)-y += amlogic-bc.o

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)