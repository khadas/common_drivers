# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

MODULE_NAME = amlogic-gkitool
obj-$(CONFIG_AMLOGIC_GKI_TOOL) = $(MODULE_NAME).o
$(MODULE_NAME)-y += gki_tool.o gki_module.o gki_config.o

$(obj)/gki_config.o: $(obj)/.gki_config
$(obj)/.gki_config: FORCE
	cp $(srctree)/../common_drivers/arch/arm64/configs/amlogic_gki.fragment ../common_drivers/drivers/gki_tool/.gki_config
