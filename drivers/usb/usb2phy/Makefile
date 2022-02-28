# SPDX-License-Identifier: GPL-2.0

MODULE_NAME = amlogic_usb2_phy
obj-$(CONFIG_AMLOGIC_USBPHY)      += $(MODULE_NAME).o
$(MODULE_NAME)-y += phy-aml-new-usb-v2.o
$(MODULE_NAME)-y += phy-aml-new-usb2-v2.o

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)