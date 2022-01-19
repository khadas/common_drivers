# SPDX-License-Identifier: GPL-2.0

MODULE_NAME = amlogic-irqchip
obj-$(CONFIG_AMLOGIC_MESON_IRQ_GPIO) = $(MODULE_NAME).o
$(MODULE_NAME)-y += irq-meson-gpio.o

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)
