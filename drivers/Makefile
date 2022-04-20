# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

obj-y					+= tty/
obj-y					+= clk/
obj-$(CONFIG_PINCTRL)			+= pinctrl/
obj-y					+= irqchip/
obj-$(CONFIG_AMLOGIC_SECMON)		+= secmon/
obj-y					+= power/
obj-$(CONFIG_AMLOGIC_CPU_INFO)		+= cpu_info/
obj-$(CONFIG_AMLOGIC_GX_SUSPEND)	+= pm/
obj-$(CONFIG_GPIOLIB)			+= gpio/
obj-$(CONFIG_INPUT)			+= input/
obj-y					+= i2c/
obj-$(CONFIG_SPI)			+= spi/
obj-$(CONFIG_AMLOGIC_TEE)		+= tee/
obj-$(CONFIG_AMLOGIC_GKI_TOOL)		+= gki_tool/
obj-y					+= pwm/
obj-$(CONFIG_CPU_FREQ)			+= cpufreq/
obj-$(CONFIG_AMLOGIC_EFUSE_UNIFYKEY)	+= efuse_unifykey/
obj-$(CONFIG_MAILBOX)			+= mailbox/
obj-$(CONFIG_AMLOGIC_MEMORY_EXTEND)	+= memory_ext/
obj-$(CONFIG_AMLOGIC_JTAG_MESON)	+= jtag/
obj-$(CONFIG_WATCHDOG)			+= watchdog/
obj-$(CONFIG_AMLOGIC_REG_ACCESS)	+= reg_access/
obj-$(CONFIG_AMLOGIC_MEDIA_ENABLE)	+= media/
obj-$(CONFIG_IIO)			+= iio/
obj-y					+= char/
obj-$(CONFIG_AMLOGIC_SOC_INFO)		+= soc_info/
obj-$(CONFIG_RESET_CONTROLLER)		+= reset/
obj-$(CONFIG_AMLOGIC_DDR_TOOL)		+= ddr_tool/
obj-$(CONFIG_RTC_LIB)			+= rtc/
obj-$(CONFIG_AMLOGIC_IRBLASTER)		+= irblaster/
obj-$(CONFIG_THERMAL)			+= thermal/
obj-y					+= mmc/
obj-$(CONFIG_AMLOGIC_GX_REBOOT)		+= reboot/
obj-$(CONFIG_AMLOGIC_HIFI4DSP)		+= hifi4dsp/
obj-y					+= debug/
obj-$(CONFIG_AMLOGIC_FREERTOS)		+= freertos/
obj-$(CONFIG_AMLOGIC_USB)               += usb/
obj-$(CONFIG_AMLOGIC_CRYPTO_DMA)	+= crypto/
obj-$(CONFIG_AMLOGIC_DOLBY_FW) 		+= dolby_fw/
obj-y					+= ../sound/
obj-$(CONFIG_AMLOGIC_WIFI)		+= wifi/
obj-$(CONFIG_AMLOGIC_BLUETOOTH)		+= bluetooth/

KBUILD_CFLAGS_MODULE += $(GKI_EXT_KERNEL_MODULE_PREDEFINE)

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) modules $(GKI_EXT_KERNEL_MODULE_CONFIG)

modules_install:
	$(MAKE) M=$(M) -C $(KERNEL_SRC) modules_install

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) clean
