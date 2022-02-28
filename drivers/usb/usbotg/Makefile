# SPDX-License-Identifier: GPL-2.0

MODULE_NAME = aml_usbotg
obj-$(CONFIG_AMLOGIC_USB3PHY)      += $(MODULE_NAME).o
$(MODULE_NAME)-y += phy-aml-new-otg.o

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)