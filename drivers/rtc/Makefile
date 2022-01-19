# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

MODULE_NAME = amlogic-rtc-virtual
obj-$(CONFIG_AMLOGIC_RTC_DRV_MESON_VRTC) = $(MODULE_NAME).o
$(MODULE_NAME)-y += rtc-meson-vrtc.o

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)
