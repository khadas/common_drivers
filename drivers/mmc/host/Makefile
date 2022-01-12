# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

ccflags-y += -I$(srctree)/drivers/mmc/core

MODULE_NAME = amlogic-mmc
obj-$(CONFIG_AMLOGIC_MMC_MESON_GX) = $(MODULE_NAME).o
$(MODULE_NAME)-y += meson-gx-mmc.o
