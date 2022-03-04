# SPDX-License-Identifier: GPL-2.0

USB2_MODULE_NAME = amlogic_usb2_c2_phy
obj-$(CONFIG_AMLOGIC_USB_SUPPORT)      += $(USB2_MODULE_NAME).o
$(USB2_MODULE_NAME)-y += phy-aml-new-usb2-c2.o

PR_FMT_USB2 = $(subst amlogic-,,$(S4_MODULE_NAME))
PR_FMT_DEFINE_USB2="-Dpr_fmt(fmt)= \"[$(PR_FMT_USB2)]: \" fmt"
CFLAGS_phy-aml-new-usb2-c2.o += $(PR_FMT_DEFINE_USB2)

USB3_MODULE_NAME = amlogic_usb3_c2_phy
obj-$(CONFIG_AMLOGIC_USB_SUPPORT)      += $(USB3_MODULE_NAME).o
$(USB3_MODULE_NAME)-y += phy-aml-new-usb3-c2.o

PR_FMT_USB3 = $(subst amlogic-,,$(S4_MODULE_NAME))
PR_FMT_DEFINE_USB3="-Dpr_fmt(fmt)= \"[$(PR_FMT_USB3)]: \" fmt"
CFLAGS_phy-aml-new-usb3-c2.o += $(PR_FMT_DEFINE_USB3)