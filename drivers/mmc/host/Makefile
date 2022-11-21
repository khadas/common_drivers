# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

ccflags-y += -I$(srctree)/drivers/mmc/core
#ccflags-y += -DDEBUG

MODULE_NAME = amlogic-mmc
obj-$(CONFIG_AMLOGIC_MMC_MESON_GX) = $(MODULE_NAME).o
$(MODULE_NAME)-y += meson-gx-mmc.o
$(MODULE_NAME)-y += mmc_dtb.o
$(MODULE_NAME)-y += mmc_key.o

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)
