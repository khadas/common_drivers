# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

MODULE_NAME = amlogic-led

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)

obj-$(CONFIG_AMLOGIC_LED) = $(MODULE_NAME).o
$(MODULE_NAME)-y = main.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_LEDS_TLC59116) += leds-tlc59116.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_LEDS_AW9523B) += leds-aw9523b.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_LEDS_STATE) += leds_state.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_LEDS_DCON)  += meson_led_unipolar_ctrl.o