# SPDX-License-Identifier: GPL-2.0-only
# Amlogic pcie drivers

MODULE_NAME = amlogic_pcie_v2_host
obj-$(CONFIG_AMLOGIC_PCIE_V2_HOST) += $(MODULE_NAME).o
$(MODULE_NAME)-y = amlogic-pcie-v2-host.o
PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
CFLAGS_amlogic-pcie-v2-host.o += $(PR_FMT_DEFINE) -I $(srctree)/drivers/pci/controller/dwc

MODULE_NAME_V3 = amlogic-pcie-v3_host
obj-$(CONFIG_AMLOGIC_PCIE_V3_HOST) += $(MODULE_NAME_V3).o
$(MODULE_NAME_V3)-y = amlogic-pcie-v3-host.o amlogic-pcie-v3-comm.o
PR_FMT_V3 = $(subst amlogic-,,$(MODULE_NAME_V3))
PR_FMT_DEFINE_V3="-Dpr_fmt(fmt)= \"[$(PR_FMT_V3)]: \" fmt"
CFLAGS_amlogic-pcie-v3-host.o += $(PR_FMT_DEFINE_V3) -I $(srctree)/drivers/pci/
CFLAGS_amlogic-pcie-v3-comm.o += $(PR_FMT_DEFINE_V3)
