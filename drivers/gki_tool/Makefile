# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

MODULE_NAME = amlogic-gkitool

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)

obj-$(CONFIG_AMLOGIC_GKI_TOOL) = $(MODULE_NAME).o
$(MODULE_NAME)-y += gki_tool.o gki_module.o
