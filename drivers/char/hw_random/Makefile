# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

MODULE_NAME = amlogic-rng
obj-$(CONFIG_AMLOGIC_HW_RANDOM_MESON) = $(MODULE_NAME).o
$(MODULE_NAME)-y += meson-rng.o

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)