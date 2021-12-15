# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

ccflags-y += -I$(srctree)/drivers/regulator
ccflags-y += -I$(srctree)/drivers/opp

MODULE_NAME = amlogic-cpufreq
obj-$(CONFIG_AMLOGIC_MESON_CPUFREQ) = $(MODULE_NAME).o
$(MODULE_NAME)-y += meson-cpufreq.o
