# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

MODULE_NAME = amlogic-spi
obj-$(CONFIG_AMLOGIC_SPI) = $(MODULE_NAME).o
$(MODULE_NAME)-y += module-init.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SPI_MESON_SPICC) += spi-meson-spicc.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SPI_MESON_SPIFC) += spi-meson-spifc.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SPI_MESON_SPICC_SLAVE) += spi-meson-spicc-slave.o vmem.o
