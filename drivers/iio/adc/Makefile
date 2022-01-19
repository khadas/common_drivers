# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

MODULE_NAME = amlogic-adc
obj-$(CONFIG_AMLOGIC_MESON_SARADC) = $(MODULE_NAME).o
$(MODULE_NAME)-y += meson_saradc.o meson_saradc_c2.o meson_saradc_m8.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_IIO_KFIFO_BUF) += kfifo_buf.o

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)
