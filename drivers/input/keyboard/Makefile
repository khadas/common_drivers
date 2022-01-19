# SPDX-License-Identifier: GPL-2.0-only

MODULE_NAME_GPIO_KEY = amlogic-input-gpiokey
obj-$(CONFIG_AMLOGIC_GPIO_KEY)  = $(MODULE_NAME_GPIO_KEY).o
$(MODULE_NAME_GPIO_KEY)-y = gpio_keypad.o
PR_FMT_GPIOKEY = $(subst amlogic-,,$(MODULE_NAME_GPIO_KEY))
PR_FMT_DEFINE_GPIOKEY="-Dpr_fmt(fmt)= \"[$(PR_FMT_GPIOKEY)]: \" fmt"
CFLAGS_gpio_keypad.o += $(PR_FMT_DEFINE_GPIOKEY)

MODULE_NAME_ADC_KEY = amlogic-input-adckey
obj-$(CONFIG_AMLOGIC_ADC_KEYPADS)  += $(MODULE_NAME_ADC_KEY).o
$(MODULE_NAME_ADC_KEY)-y = adc_keypad.o
PR_FMT_ADCKEY = $(subst amlogic-,,$(MODULE_NAME_ADC_KEY))
PR_FMT_DEFINE_ADCKEY="-Dpr_fmt(fmt)= \"[$(PR_FMT_ADCKEY)]: \" fmt"
CFLAGS_adc_keypad.o += $(PR_FMT_DEFINE_ADCKEY)
