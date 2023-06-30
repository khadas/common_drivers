# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

obj-y					+= tty/
obj-$(CONFIG_AMLOGIC_GKI_TOOL)		+= gki_tool/
obj-y					+= debug/
obj-$(CONFIG_AMLOGIC_MEMORY_DEBUG)	+= memory_debug/
obj-$(CONFIG_AMLOGIC_SECMON)		+= secmon/
obj-y					+= clk/
obj-$(CONFIG_AMLOGIC_GPIO)		+= gpio/
obj-$(CONFIG_MAILBOX)			+= mailbox/
obj-y					+= pwm/
obj-y					+= power/
obj-$(CONFIG_AMLOGIC_CPU_INFO)		+= cpu_info/
obj-$(CONFIG_CPU_FREQ)			+= cpufreq/
obj-$(CONFIG_RESET_CONTROLLER)		+= reset/
obj-$(CONFIG_AMLOGIC_EFUSE_UNIFYKEY)	+= efuse_unifykey/
obj-y					+= mmc/
obj-y					+= i2c/
obj-$(CONFIG_SPI)			+= spi/
obj-y					+= char/
obj-$(CONFIG_AMLOGIC_TEE)		+= tee/
obj-$(CONFIG_AML_TEE)			+= aml_tee/
obj-$(CONFIG_AMLOGIC_TRUSTED_KEYS)	+= aml_trusted_keys/
obj-$(CONFIG_AMLOGIC_GX_SUSPEND)	+= pm/
obj-$(CONFIG_INPUT)			+= input/
obj-$(CONFIG_WATCHDOG)			+= watchdog/
obj-y					+= dvb/
obj-y					+= dvb_ci/
obj-$(CONFIG_AMLOGIC_MEDIA_ENABLE)	+= media/
obj-$(CONFIG_IIO)			+= iio/
obj-$(CONFIG_THERMAL)			+= thermal/
obj-$(CONFIG_AMLOGIC_USB)		+= usb/
obj-$(CONFIG_AMLOGIC_IRBLASTER)		+= irblaster/
obj-$(CONFIG_AMLOGIC_DRM)		+= drm/
obj-$(CONFIG_AMLOGIC_CRYPTO_DMA)	+= crypto/
obj-$(CONFIG_RTC_LIB)			+= rtc/
obj-y					+= pci/
obj-$(CONFIG_AMLOGIC_SOC_INFO)		+= soc_info/
obj-$(CONFIG_AMLOGIC_HIFI4DSP)		+= hifi4dsp/
obj-$(CONFIG_AMLOGIC_WIRELESS)		+= wireless/
obj-$(CONFIG_AMLOGIC_LED)		+= led/
obj-$(CONFIG_AMLOGIC_INPHY)		+= net/
obj-$(CONFIG_AMLOGIC_MTD_COMMON)	+= mtd/common/
obj-$(CONFIG_AMLOGIC_MTD_SPI_NAND)	+= mtd/spinand/
obj-$(CONFIG_AMLOGIC_MTD_NAND)		+= mtd/nand/
obj-$(CONFIG_AMLOGIC_FREERTOS)		+= freertos/
obj-$(CONFIG_AMLOGIC_JTAG_MESON)	+= jtag/
obj-y					+= memory_ext/
obj-$(CONFIG_AMLOGIC_HWSPINLOCK)	+= hwspinlock/
obj-y					+= android/
obj-$(CONFIG_AMLOGIC_YAFFS_FS)		+= yaffs2/
obj-$(CONFIG_AMLOGIC_MFD_PMIC6B)	+= mfd/
obj-$(CONFIG_AMLOGIC_REGULATOR_PMIC6B)	+= regulator/
obj-$(CONFIG_AMLOGIC_AUDIO_BRIDGE)	+= audiobridge/
obj-$(CONFIG_AMLOGIC_MEDIA_ALGORITHM)	+= algorithm/
obj-y        				+= dvb/
obj-$(CONFIG_AMLOGIC_DRM)		+= drm/
obj-y              			+= pci/
obj-$(CONFIG_AMLOGIC_LED)		+= led/
obj-y              			+= firmware/
obj-$(CONFIG_AMLOGIC_SOC_TIMESTAMP)	+= timestamp/
obj-$(CONFIG_AMLOGIC_SECKEY)		+= seckey/

KBUILD_CFLAGS_MODULE += $(GKI_EXT_KERNEL_MODULE_PREDEFINE)
all:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) modules $(GKI_EXT_KERNEL_MODULE_CONFIG)

modules_install:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) modules_install

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) clean
