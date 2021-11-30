# SPDX-License-Identifier: GPL-2.0-only
MODULE_NAME = amlogic-pinctrl-common
obj-$(CONFIG_AMLOGIC_COMMON_PINCTRL) = $(MODULE_NAME).o
$(MODULE_NAME)-y += pinctrl-module-init.o pinctrl-meson.o pinctrl-meson-axg-pmx.o

S4_MODULE_NAME = amlogic-pinctrl-s4
obj-$(CONFIG_AMLOGIC_PINCTRL_S4) += $(S4_MODULE_NAME).o
$(S4_MODULE_NAME)-y += pinctrl-meson-s4.o
