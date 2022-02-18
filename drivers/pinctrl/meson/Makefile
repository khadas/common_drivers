# SPDX-License-Identifier: GPL-2.0-only

ccflags-y += -I$(srctree)/drivers

MODULE_NAME = amlogic-pinctrl
obj-$(CONFIG_AMLOGIC_PINCTRL_MESON) = $(MODULE_NAME).o
$(MODULE_NAME)-y += pinctrl-module-init.o pinctrl-meson.o pinctrl-meson-axg-pmx.o
PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
CFLAGS_pinctrl-module-init.o += $(PR_FMT_DEFINE)
CFLAGS_pinctrl-meson.o += $(PR_FMT_DEFINE)
CFLAGS_pinctrl-meson-axg-pmx.o += $(PR_FMT_DEFINE)

S4_MODULE_NAME = amlogic-pinctrl-soc-s4
obj-$(CONFIG_AMLOGIC_PINCTRL_MESON_S4) += $(S4_MODULE_NAME).o
$(S4_MODULE_NAME)-y += pinctrl-meson-s4.o
PR_FMT_S4 = $(subst amlogic-,,$(S4_MODULE_NAME))
PR_FMT_DEFINE_S4="-Dpr_fmt(fmt)= \"[$(PR_FMT_S4)]: \" fmt"
CFLAGS_pinctrl-meson-s4.o += $(PR_FMT_DEFINE_S4)

C2_MODULE_NAME = amlogic-pinctrl-soc-c2
obj-$(CONFIG_AMLOGIC_PINCTRL_MESON_C2) += $(C2_MODULE_NAME).o
$(C2_MODULE_NAME)-y += pinctrl-meson-c2.o
PR_FMT_C2 = $(subst amlogic-,,$(C2_MODULE_NAME))
PR_FMT_DEFINE_C2="-Dpr_fmt(fmt)= \"[$(PR_FMT_C2)]: \" fmt"
CFLAGS_pinctrl-meson-c2.o += $(PR_FMT_DEFINE_C2)
