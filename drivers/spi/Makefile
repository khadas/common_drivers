# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

MODULE_NAME = amlogic-spi
obj-$(CONFIG_AMLOGIC_SPI) = $(MODULE_NAME).o
$(MODULE_NAME)-y += module-init.o spi-meson-mem.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SPI_MESON_SPICC) += spi-meson-spicc.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SPI_MESON_SPIFC) += spi-meson-spifc.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SPI_MESON_SPIFC_V2) += spi-meson-spifc-v2.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SPI_MESON_SPICC_SLAVE) += spi-meson-spicc-slave.o vmem.o

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)
