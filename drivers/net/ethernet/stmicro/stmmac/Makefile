PDX-License-Identifier: (GPL-2.0+ OR MIT)

MODULE_NAME = amlogic-phy-debug
PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)
obj-$(CONFIG_AMLOGIC_MDIO_G12A) = $(MODULE_NAME).o
$(MODULE_NAME)-y += aml_phy_debug.o
