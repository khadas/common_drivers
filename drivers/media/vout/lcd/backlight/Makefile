# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_BACKLIGHT) += vout/lcd/backlight/lcd_bl.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_BACKLIGHT) += vout/lcd/backlight/bl_pwm.o

#ifdef CONFIG_AMLOGIC_BACKLIGHT
include $(COMMON_DRIVERS_DIR)/drivers/media/vout/lcd/backlight/bl_extern/Makefile
include $(COMMON_DRIVERS_DIR)/drivers/media/vout/lcd/backlight/bl_ldim/Makefile
#endif
