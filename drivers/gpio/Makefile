# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

MODULE_NAME = amlogic-gpio

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)

ccflags-y += -I$(srctree)/drivers/gpio

obj-$(CONFIG_AMLOGIC_GPIO) = $(MODULE_NAME).o
$(MODULE_NAME)-y = main.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_GPIOLIB) += gpiolib.o

include $(COMMON_DRIVERS_DIR)/drivers/gpio/irqchip/Makefile
include $(COMMON_DRIVERS_DIR)/drivers/gpio/pinctrl/Makefile