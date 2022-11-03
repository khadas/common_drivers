# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

MODULE_NAME = amlogic-rtc

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)

obj-$(CONFIG_AMLOGIC_RTC) = $(MODULE_NAME).o
$(MODULE_NAME)-y = main.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_RTC_DRV_MESON_VRTC) += rtc-meson-vrtc.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_MESON_RTC) += meson-rtc.o