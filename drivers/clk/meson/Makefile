# SPDX-License-Identifier: GPL-2.0-only
# Amlogic clock drivers

MODULE_NAME = amlogic-clk
obj-$(CONFIG_AMLOGIC_COMMON_CLK) = $(MODULE_NAME).o
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

C2_MODULE_NAME = amlogic-clk-soc-c2
obj-$(CONFIG_AMLOGIC_COMMON_CLK_C2) += $(C2_MODULE_NAME).o
$(C2_MODULE_NAME)-y += c2/c2.o c2/clk-pll-c2.o
PR_FMT_C2 = $(subst amlogic-,,$(C2_MODULE_NAME))
PR_FMT_DEFINE_C2="-Dpr_fmt(fmt)= \"[$(PR_FMT_C2)]: \" fmt"
CFLAGS_c2.o += $(PR_FMT_DEFINE_C2)
CFLAGS_clk-pll-c2.o += $(PR_FMT_DEFINE_C2)
