# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

MODULE_NAME = amlogic-adc
obj-$(CONFIG_AMLOGIC_MESON_SARADC) = $(MODULE_NAME).o
$(MODULE_NAME)-y += meson_saradc.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_IIO_KFIFO_BUF) += kfifo_buf.o
