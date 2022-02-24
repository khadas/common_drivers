# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

ccflags-y += -I$(srctree)/drivers/watchdog

MODULE_NAME = amlogic-watchdog
obj-$(CONFIG_AMLOGIC_MESON_GXBB_WATCHDOG) = $(MODULE_NAME).o
$(MODULE_NAME)-y += meson_gxbb_wdt.o
PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"$(PR_FMT): \" fmt"
ccflags_meson_gxbb_wdt.o += $(PR_FMT_DEFINE)

MODULE_NAME_SMC = amlogic-smc-watchdog
obj-$(CONFIG_AMLOGIC_ARM_SMC_WATCHDOG) = $(MODULE_NAME_SMC).o
$(MODULE_NAME_SMC)-y += arm_smc_wdt.o
PR_FMT_SMC = $(subst amlogic-,,$(MODULE_NAME_SMC))
PR_FMT_DEFINE_SMC="-Dpr_fmt(fmt)= \"$(PR_FMT_SMC): \" fmt"
ccflags_arm_smc_wdt.o += $(PR_FMT_DEFINE_SMC)
