# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

MODULE_NAME = amlogic-socinfo
obj-$(CONFIG_AMLOGIC_SOC_INFO) = $(MODULE_NAME).o
$(MODULE_NAME)-y += socdata.o
$(MODULE_NAME)-y += nocsdata.o
$(MODULE_NAME)-y += authregion.o

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)
