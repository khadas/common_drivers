# SPDX-License-Identifier: GPL-2.0-only
# Amlogic pcie drivers

MODULE_NAME = amlogic_pcie_v2_host
obj-$(CONFIG_AMLOGIC_PCIE_V2_HOST) += $(MODULE_NAME).o
$(MODULE_NAME)-y = amlogic-pcie-v2-host.o
PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
CFLAGS_amlogic-pcie-v2-host.o += $(PR_FMT_DEFINE) -I $(srctree)/drivers/pci/controller/dwc
