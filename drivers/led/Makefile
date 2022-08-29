# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

TLC_MODULE_NAME = amlogic-led-tlc59116
obj-$(CONFIG_AMLOGIC_LEDS_TLC59116) += $(TLC_MODULE_NAME).o
$(TLC_MODULE_NAME)-y += leds-tlc59116.o
PR_FMT_TLC = $(subst amlogic-,,$(TLC_MODULE_NAME))
PR_FMT_DEFINE_TLC="-Dpr_fmt(fmt)= \"[$(PR_FMT_TLC)]: \" fmt"
CFLAGS_leds-tlc59116.o += $(PR_FMT_DEFINE_TLC)

AW_MODULE_NAME = amlogic-led-aw9523b
obj-$(CONFIG_AMLOGIC_LEDS_AW9523B) += $(AW_MODULE_NAME).o
$(AW_MODULE_NAME)-y += leds-aw9523b.o
PR_FMT_AW = $(subst amlogic-,,$(AW_MODULE_NAME))
PR_FMT_DEFINE_AW="-Dpr_fmt(fmt)= \"[$(PR_FMT_AW)]: \" fmt"
CFLAGS_leds-aw9523b.o += $(PR_FMT_DEFINE_AW)

STATE_MODULE_NAME = amlogic-led-state
obj-$(CONFIG_AMLOGIC_LEDS_STATE) += $(STATE_MODULE_NAME).o
$(STATE_MODULE_NAME)-y += leds_state.o
PR_FMT_STATE = $(subst amlogic-,,$(STATE_MODULE_NAME))
PR_FMT_DEFINE_STATE="-Dpr_fmt(fmt)= \"[$(PR_FMT_STATE)]: \" fmt"
CFLAGS_leds_state.o += $(PR_FMT_DEFINE_STATE)
