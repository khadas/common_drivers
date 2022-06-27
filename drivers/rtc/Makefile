# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

VRTC_MODULE_NAME = amlogic-rtc-virtual
obj-$(CONFIG_AMLOGIC_RTC_DRV_MESON_VRTC) += $(VRTC_MODULE_NAME).o
$(VRTC_MODULE_NAME)-y += rtc-meson-vrtc.o
PR_FMT_VRTC = $(subst amlogic-,,$(VRTC_MODULE_NAME))
PR_FMT_DEFINE_VRTC="-Dpr_fmt(fmt)= \"[$(PR_FMT_VRTC)]: \" fmt"
CFLAGS_rtc-meson-vrtc.o += $(PR_FMT_DEFINE_VRTC)

RTC_MODULE_NAME = amlogic-rtc
obj-$(CONFIG_AMLOGIC_MESON_RTC) += $(RTC_MODULE_NAME).o
$(RTC_MODULE_NAME)-y += meson-rtc.o
PR_FMT_RTC = $(subst amlogic-,,$(RTC_MODULE_NAME))
PR_FMT_DEFINE_RTC="-Dpr_fmt(fmt)= \"[$(PR_FMT_RTC)]: \" fmt"
CFLAGS_meson-rtc.o += $(PR_FMT_DEFINE_RTC)
