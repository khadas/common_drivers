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

G12A_MODULE_NAME = amlogic-pinctrl-soc-g12a
obj-$(CONFIG_AMLOGIC_PINCTRL_MESON_G12A) += $(G12A_MODULE_NAME).o
$(G12A_MODULE_NAME)-y += pinctrl/pinctrl-meson-g12a.o
PR_FMT_G12A = $(subst amlogic-,,$(G12A_MODULE_NAME))
PR_FMT_DEFINE_G12A="-Dpr_fmt(fmt)= \"[$(PR_FMT_G12A)]: \" fmt"
CFLAGS_pinctrl-meson-g12a.o += $(PR_FMT_DEFINE_G12A)

S5_MODULE_NAME = amlogic-pinctrl-soc-s5
obj-$(CONFIG_AMLOGIC_PINCTRL_MESON_S5) += $(S5_MODULE_NAME).o
$(S5_MODULE_NAME)-y += pinctrl/pinctrl-meson-s5.o
PR_FMT_S5 = $(subst amlogic-,,$(S5_MODULE_NAME))
PR_FMT_DEFINE_S5="-Dpr_fmt(fmt)= \"[$(PR_FMT_S5)]: \" fmt"
CFLAGS_pinctrl-meson-s5.o += $(PR_FMT_DEFINE_S5)

T5W_MODULE_NAME = amlogic-pinctrl-soc-t5w
obj-$(CONFIG_AMLOGIC_PINCTRL_MESON_T5W) += $(T5W_MODULE_NAME).o
$(T5W_MODULE_NAME)-y += pinctrl/pinctrl-meson-t5w.o
PR_FMT_T5W = $(subst amlogic-,,$(T5W_MODULE_NAME))
PR_FMT_DEFINE_T5W="-Dpr_fmt(fmt)= \"[$(PR_FMT_T5W)]: \" fmt"
CFLAGS_pinctrl-meson-t5w.o += $(PR_FMT_DEFINE_T5W)

T3X_MODULE_NAME = amlogic-pinctrl-soc-t3x
obj-$(CONFIG_AMLOGIC_PINCTRL_MESON_T3X) += $(T3X_MODULE_NAME).o
$(T3X_MODULE_NAME)-y += pinctrl/pinctrl-meson-t3x.o
PR_FMT_T3X = $(subst amlogic-,,$(T3X_MODULE_NAME))
PR_FMT_DEFINE_T3X="-Dpr_fmt(fmt)= \"[$(PR_FMT_T3X)]: \" fmt"
CFLAGS_pinctrl-meson-t3x.o += $(PR_FMT_DEFINE_T3X)

TXHD2_MODULE_NAME = amlogic-pinctrl-soc-txhd2
obj-$(CONFIG_AMLOGIC_PINCTRL_MESON_TXHD2) += $(TXHD2_MODULE_NAME).o
$(TXHD2_MODULE_NAME)-y += pinctrl/pinctrl-meson-txhd2.o
PR_FMT_TXHD2 = $(subst amlogic-,,$(TXHD2_MODULE_NAME))
PR_FMT_DEFINE_TXHD2="-Dpr_fmt(fmt)= \"[$(PR_FMT_TXHD2)]: \" fmt"
CFLAGS_pinctrl-meson-txhd2.o += $(PR_FMT_DEFINE_TXHD2)

C1_MODULE_NAME = amlogic-pinctrl-soc-c1
obj-$(CONFIG_AMLOGIC_PINCTRL_MESON_C1) += $(C1_MODULE_NAME).o
$(C1_MODULE_NAME)-y += pinctrl/pinctrl-meson-c1.o
PR_FMT_C1 = $(subst amlogic-,,$(C1_MODULE_NAME))
PR_FMT_DEFINE_C1="-Dpr_fmt(fmt)= \"[$(PR_FMT_C1)]: \" fmt"
CFLAGS_pinctrl-meson-c1.o += $(PR_FMT_DEFINE_C1)
