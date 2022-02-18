# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

MODULE_NAME = amlogic-gkitool
obj-$(CONFIG_AMLOGIC_GKI_TOOL) = $(MODULE_NAME).o
$(MODULE_NAME)-y += gki_tool.o gki_module.o gki_config.o

ccflags-y += "-DGKI_CONFIG=\"$(srctree)/$(COMMON_DRIVERS_DIR)/arch/arm64/configs/amlogic_gki.fragment\""

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)
