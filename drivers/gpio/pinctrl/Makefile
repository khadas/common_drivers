# SPDX-License-Identifier: GPL-2.0-only

ccflags-y += -I$(srctree)/drivers

$(MODULE_NAME)-$(CONFIG_AMLOGIC_PINCTRL_MESON) += pinctrl/pinctrl-meson.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_PINCTRL_MESON) += pinctrl/pinctrl-meson-axg-pmx.o

S4_MODULE_NAME = amlogic-pinctrl-soc-s4
obj-$(CONFIG_AMLOGIC_PINCTRL_MESON_S4) += $(S4_MODULE_NAME).o
$(S4_MODULE_NAME)-y += pinctrl/pinctrl-meson-s4.o
PR_FMT_S4 = $(subst amlogic-,,$(S4_MODULE_NAME))
PR_FMT_DEFINE_S4="-Dpr_fmt(fmt)= \"[$(PR_FMT_S4)]: \" fmt"
CFLAGS_pinctrl-meson-s4.o += $(PR_FMT_DEFINE_S4)

C2_MODULE_NAME = amlogic-pinctrl-soc-c2
obj-$(CONFIG_AMLOGIC_PINCTRL_MESON_C2) += $(C2_MODULE_NAME).o
$(C2_MODULE_NAME)-y += pinctrl/pinctrl-meson-c2.o
PR_FMT_C2 = $(subst amlogic-,,$(C2_MODULE_NAME))
PR_FMT_DEFINE_C2="-Dpr_fmt(fmt)= \"[$(PR_FMT_C2)]: \" fmt"
CFLAGS_pinctrl-meson-c2.o += $(PR_FMT_DEFINE_C2)

C3_MODULE_NAME = amlogic-pinctrl-soc-c3
obj-$(CONFIG_AMLOGIC_PINCTRL_MESON_C3) += $(C3_MODULE_NAME).o
$(C3_MODULE_NAME)-y += pinctrl/pinctrl-meson-c3.o
PR_FMT_C3 = $(subst amlogic-,,$(C3_MODULE_NAME))
PR_FMT_DEFINE_C3="-Dpr_fmt(fmt)= \"[$(PR_FMT_C3)]: \" fmt"
CFLAGS_pinctrl-meson-c3.o += $(PR_FMT_DEFINE_C3)

A1_MODULE_NAME = amlogic-pinctrl-soc-a1
obj-$(CONFIG_AMLOGIC_PINCTRL_MESON_A1) += $(A1_MODULE_NAME).o
$(A1_MODULE_NAME)-y += pinctrl/pinctrl-meson-a1.o
PR_FMT_A1 = $(subst amlogic-,,$(A1_MODULE_NAME))
PR_FMT_DEFINE_A1="-Dpr_fmt(fmt)= \"[$(PR_FMT_A1)]: \" fmt"
CFLAGS_pinctrl-meson-a1.o += $(PR_FMT_DEFINE_A1)

SC2_MODULE_NAME = amlogic-pinctrl-soc-sc2
obj-$(CONFIG_AMLOGIC_PINCTRL_MESON_SC2) += $(SC2_MODULE_NAME).o
$(SC2_MODULE_NAME)-y += pinctrl/pinctrl-meson-sc2.o
PR_FMT_SC2 = $(subst amlogic-,,$(SC2_MODULE_NAME))
PR_FMT_DEFINE_SC2="-Dpr_fmt(fmt)= \"[$(PR_FMT_SC2)]: \" fmt"
CFLAGS_pinctrl-meson-sc2.o += $(PR_FMT_DEFINE_SC2)

T3_MODULE_NAME = amlogic-pinctrl-soc-t3
obj-$(CONFIG_AMLOGIC_PINCTRL_MESON_T3) += $(T3_MODULE_NAME).o
$(T3_MODULE_NAME)-y += pinctrl/pinctrl-meson-t3.o
PR_FMT_T3 = $(subst amlogic-,,$(T3_MODULE_NAME))
PR_FMT_DEFINE_T3="-Dpr_fmt(fmt)= \"[$(PR_FMT_T3)]: \" fmt"
CFLAGS_pinctrl-meson-t3.o += $(PR_FMT_DEFINE_T3)

T7_MODULE_NAME = amlogic-pinctrl-soc-t7
obj-$(CONFIG_AMLOGIC_PINCTRL_MESON_T7) += $(T7_MODULE_NAME).o
$(T7_MODULE_NAME)-y += pinctrl/pinctrl-meson-t7.o
PR_FMT_T7 = $(subst amlogic-,,$(T7_MODULE_NAME))
PR_FMT_DEFINE_T7="-Dpr_fmt(fmt)= \"[$(PR_FMT_T7)]: \" fmt"
CFLAGS_pinctrl-meson-t7.o += $(PR_FMT_DEFINE_T7)

T5M_MODULE_NAME = amlogic-pinctrl-soc-t5m
obj-$(CONFIG_AMLOGIC_PINCTRL_MESON_T5M) += $(T5M_MODULE_NAME).o
$(T5M_MODULE_NAME)-y += pinctrl/pinctrl-meson-t5m.o
PR_FMT_T5M = $(subst amlogic-,,$(T5M_MODULE_NAME))
PR_FMT_DEFINE_T5M="-Dpr_fmt(fmt)= \"[$(PR_FMT_T5M)]: \" fmt"
CFLAGS_pinctrl-meson-t5m.o += $(PR_FMT_DEFINE_T5M)
