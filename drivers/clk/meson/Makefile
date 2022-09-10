# SPDX-License-Identifier: GPL-2.0-only
# Amlogic clock drivers

MODULE_NAME = amlogic-clk
obj-$(CONFIG_AMLOGIC_COMMON_CLK) = $(MODULE_NAME).o

ccflags-y += -I$(srctree)/$(COMMON_DRIVERS_DIR)/drivers/clk/meson

$(MODULE_NAME)-y = clk-common-init.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_COMMON_CLK_MESON_AO_CLKC) += meson-aoclk.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_COMMON_CLK_MESON_CPU_DYNDIV) += clk-cpu-dyndiv.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_COMMON_CLK_MESON_DUALDIV) += clk-dualdiv.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_COMMON_CLK_MESON_EE_CLKC) += meson-eeclk.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_COMMON_CLK_MESON_MPLL) += clk-mpll.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_COMMON_CLK_MESON_PHASE) += clk-phase.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_COMMON_CLK_MESON_PLL) += clk-pll.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_COMMON_CLK_MESON_REGMAP) += clk-regmap.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_COMMON_CLK_MESON_SCLK_DIV) += sclk-div.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_COMMON_CLK_MESON_VID_PLL_DIV) += vid-pll-div.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_MESON_CLK_MEASURE) += meson-clk-measure.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_CLK_DEBUG) += meson-clk-debug.o
PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
CFLAGS_meson-aoclk.o += $(PR_FMT_DEFINE)
CFLAGS_clk-cpu-dyndiv.o += $(PR_FMT_DEFINE)
CFLAGS_clk-dualdiv.o += $(PR_FMT_DEFINE)
CFLAGS_meson-eeclk.o += $(PR_FMT_DEFINE)
CFLAGS_clk-mpll.o += $(PR_FMT_DEFINE)
CFLAGS_clk-phase.o += $(PR_FMT_DEFINE)
CFLAGS_clk-pll.o += $(PR_FMT_DEFINE)
CFLAGS_clk-regmap.o += $(PR_FMT_DEFINE)
CFLAGS_vid-pll-div.o += $(PR_FMT_DEFINE)
CFLAGS_meson-clk-measure.o += $(PR_FMT_DEFINE)
CFLAGS_meson-clk-debug.o += $(PR_FMT_DEFINE)

S4_MODULE_NAME = amlogic-clk-soc-s4
obj-$(CONFIG_AMLOGIC_COMMON_CLK_S4) += $(S4_MODULE_NAME).o
$(S4_MODULE_NAME)-y = s4.o
PR_FMT_S4 = $(subst amlogic-,,$(S4_MODULE_NAME))
PR_FMT_DEFINE_S4="-Dpr_fmt(fmt)= \"[$(PR_FMT_S4)]: \" fmt"
CFLAGS_s4.o += $(PR_FMT_DEFINE_S4)

SC2_MODULE_NAME = amlogic-clk-soc-sc2
obj-$(CONFIG_AMLOGIC_COMMON_CLK_SC2) += $(SC2_MODULE_NAME).o
$(SC2_MODULE_NAME)-y = sc2.o
PR_FMT_SC2 = $(subst amlogic-,,$(SC2_MODULE_NAME))
PR_FMT_DEFINE_SC2="-Dpr_fmt(fmt)= \"[$(PR_FMT_SC2)]: \" fmt"
CFLAGS_sc2.o += $(PR_FMT_DEFINE_SC2)

C2_MODULE_NAME = amlogic-clk-soc-c2
obj-$(CONFIG_AMLOGIC_COMMON_CLK_C2) += $(C2_MODULE_NAME).o
$(C2_MODULE_NAME)-y += c2.o
PR_FMT_C2 = $(subst amlogic-,,$(C2_MODULE_NAME))
PR_FMT_DEFINE_C2="-Dpr_fmt(fmt)= \"[$(PR_FMT_C2)]: \" fmt"
CFLAGS_c2.o += $(PR_FMT_DEFINE_C2)

C3_MODULE_NAME = amlogic-clk-soc-c3
obj-$(CONFIG_AMLOGIC_COMMON_CLK_C3) += $(C3_MODULE_NAME).o
$(C3_MODULE_NAME)-y += c3.o
PR_FMT_C3 = $(subst amlogic-,,$(C3_MODULE_NAME))
PR_FMT_DEFINE_C3="-Dpr_fmt(fmt)= \"[$(PR_FMT_C3)]: \" fmt"
CFLAGS_c3.o += $(PR_FMT_DEFINE_C3)

A1_MODULE_NAME = amlogic-clk-soc-a1
obj-$(CONFIG_AMLOGIC_COMMON_CLK_A1) += $(A1_MODULE_NAME).o
$(A1_MODULE_NAME)-y += a1.o
PR_FMT_A1 = $(subst amlogic-,,$(A1_MODULE_NAME))
PR_FMT_DEFINE_A1="-Dpr_fmt(fmt)= \"[$(PR_FMT_A1)]: \" fmt"
CFLAGS_a1.o += $(PR_FMT_DEFINE_A1)

T3_MODULE_NAME = amlogic-clk-soc-t3
obj-$(CONFIG_AMLOGIC_COMMON_CLK_T3) += $(T3_MODULE_NAME).o
$(T3_MODULE_NAME)-y += t3.o
PR_FMT_T3 = $(subst amlogic-,,$(T3_MODULE_NAME))
PR_FMT_DEFINE_T3="-Dpr_fmt(fmt)= \"[$(PR_FMT_T3)]: \" fmt"
CFLAGS_t3.o += $(PR_FMT_DEFINE_T3)

T7_MODULE_NAME = amlogic-clk-soc-t7
obj-$(CONFIG_AMLOGIC_COMMON_CLK_T7) += $(T7_MODULE_NAME).o
$(T7_MODULE_NAME)-y += t7.o
PR_FMT_T7 = $(subst amlogic-,,$(T7_MODULE_NAME))
PR_FMT_DEFINE_T7="-Dpr_fmt(fmt)= \"[$(PR_FMT_T7)]: \" fmt"
CFLAGS_t7.o += $(PR_FMT_DEFINE_T7)

T5M_MODULE_NAME = amlogic-clk-soc-t5m
obj-$(CONFIG_AMLOGIC_COMMON_CLK_T5M) += $(T5M_MODULE_NAME).o
$(T5M_MODULE_NAME)-y += t5m.o
PR_FMT_T5M = $(subst amlogic-,,$(T5M_MODULE_NAME))
PR_FMT_DEFINE_T5M="-Dpr_fmt(fmt)= \"[$(PR_FMT_T5M)]: \" fmt"
CFLAGS_t5m.o += $(PR_FMT_DEFINE_T5M)
