# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

MODULE_NAME = amlogic-pm

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)

obj-$(CONFIG_AMLOGIC_PM) = $(MODULE_NAME).o
$(MODULE_NAME)-y += main.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_GX_SUSPEND) += gx_pm.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_GX_REBOOT) += reboot.o
