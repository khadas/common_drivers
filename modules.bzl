# SPDX-License-Identifier: GPL-2.0
load("//common:common_drivers/project/project.bzl", "ANDROID_PROJECT", "GKI_CONFIG", "UPGRADE_PROJECT")
load("//common:common_drivers/project/project.bzl", "VENDOR_MODULES_REMOVE", "VENDOR_MODULES_ADD")

OEM_PROJECT_MODULES = [

]

AMLOGIC_GKI20_MODULES = [
    "common_drivers/drivers/memory_ext/aml_cma.ko",
]

AMLOGIC_GKI10_MODULES = [

]

AMLOGIC_MODULES_ANDROID = [
    "common_drivers/drivers/tty/serial/amlogic-uart.ko",
]

AMLOGIC_COMMON_MODULES = [
    # keep sorted
    "arch/arm64/crypto/sha1-ce.ko",
    "common_drivers/drivers/aml_tee/optee/optee.ko",
    "common_drivers/drivers/aml_tee/tee.ko",
    "common_drivers/drivers/char/hw_random/amlogic-rng.ko",
    "common_drivers/drivers/clk/meson/amlogic-aoclk-g12a.ko",
    "common_drivers/drivers/clk/meson/amlogic-aoclk-soc-t5w.ko",
    "common_drivers/drivers/clk/meson/amlogic-aoclk-soc-txhd2.ko",
    "common_drivers/drivers/clk/meson/amlogic-clk.ko",
    "common_drivers/drivers/clk/meson/amlogic-clk-soc-g12a.ko",
    "common_drivers/drivers/clk/meson/amlogic-clk-soc-s4.ko",
    "common_drivers/drivers/clk/meson/amlogic-clk-soc-s5.ko",
    "common_drivers/drivers/clk/meson/amlogic-clk-soc-sc2.ko",
    "common_drivers/drivers/clk/meson/amlogic-clk-soc-t3.ko",
    "common_drivers/drivers/clk/meson/amlogic-clk-soc-t3x.ko",
    "common_drivers/drivers/clk/meson/amlogic-clk-soc-t5m.ko",
    "common_drivers/drivers/clk/meson/amlogic-clk-soc-t5w.ko",
    "common_drivers/drivers/clk/meson/amlogic-clk-soc-t7.ko",
    "common_drivers/drivers/clk/meson/amlogic-clk-soc-txhd2.ko",
    "common_drivers/drivers/cpufreq/amlogic-cpufreq.ko",
    "common_drivers/drivers/cpu_info/amlogic-cpuinfo.ko",
    "common_drivers/drivers/crypto/amlogic-crypto-dma.ko",
    "common_drivers/drivers/debug/amlogic-audio-utils.ko",
    "common_drivers/drivers/debug/amlogic-debug.ko",
    "common_drivers/drivers/debug/amlogic-debug-iotrace.ko",
    "common_drivers/drivers/drm/aml_drm.ko",
    "common_drivers/drivers/dvb/amlogic-dvb.ko",
    "common_drivers/drivers/dvb_ci/amlogic-dvb-ci.ko",
    "common_drivers/drivers/dvb/demux/amlogic-dvb-demux.ko",
    "common_drivers/drivers/efuse_unifykey/amlogic-efuse-unifykey.ko",
    "common_drivers/drivers/firmware/bl40_module.ko",
    "common_drivers/drivers/gki_tool/amlogic-gkitool.ko",
    "common_drivers/drivers/gpio/amlogic-gpio.ko",
    "common_drivers/drivers/gpio/amlogic-pinctrl-soc-g12a.ko",
    "common_drivers/drivers/gpio/amlogic-pinctrl-soc-s4.ko",
    "common_drivers/drivers/gpio/amlogic-pinctrl-soc-s5.ko",
    "common_drivers/drivers/gpio/amlogic-pinctrl-soc-sc2.ko",
    "common_drivers/drivers/gpio/amlogic-pinctrl-soc-t3.ko",
    "common_drivers/drivers/gpio/amlogic-pinctrl-soc-t3x.ko",
    "common_drivers/drivers/gpio/amlogic-pinctrl-soc-t5m.ko",
    "common_drivers/drivers/gpio/amlogic-pinctrl-soc-t5w.ko",
    "common_drivers/drivers/gpio/amlogic-pinctrl-soc-t7.ko",
    "common_drivers/drivers/hifi4dsp/amlogic-hifidsp.ko",
    "common_drivers/drivers/hwspinlock/amlogic-hwspinlock.ko",
    "common_drivers/drivers/i2c/busses/amlogic-i2c.ko",
    "common_drivers/drivers/iio/adc/amlogic-adc.ko",
    "common_drivers/drivers/input/amlogic-input.ko",
    "common_drivers/drivers/irblaster/amlogic-irblaster.ko",
    "common_drivers/drivers/jtag/amlogic-jtag.ko",
    "common_drivers/drivers/led/amlogic-led.ko",
    "common_drivers/drivers/mailbox/amlogic-mailbox.ko",
    "common_drivers/drivers/media/aml_media.ko",
    "common_drivers/drivers/media/camera/amlogic-camera.ko",
    "common_drivers/drivers/memory_debug/amlogic-memory-debug.ko",
    "common_drivers/drivers/memory_ext/page_trace.ko",
    "common_drivers/drivers/mmc/host/amlogic-mmc.ko",
    "common_drivers/drivers/net/ethernet/stmicro/stmmac/amlogic-phy-debug.ko",
    "common_drivers/drivers/net/mdio/amlogic-mdio-g12a.ko",
    "common_drivers/drivers/net/phy/amlogic-inphy.ko",
    "common_drivers/drivers/pci/controller/amlogic_pcie_v2_host.ko",
    "common_drivers/drivers/pci/controller/amlogic-pcie-v3_host.ko",
    "common_drivers/drivers/pm/amlogic-pm.ko",
    "common_drivers/drivers/power/amlogic-power.ko",
    "common_drivers/drivers/pwm/amlogic-pwm.ko",
    "common_drivers/drivers/reset/amlogic-reset.ko",
    "common_drivers/drivers/rtc/amlogic-rtc.ko",
    "common_drivers/drivers/secmon/amlogic-secmon.ko",
    "common_drivers/drivers/soc_info/amlogic-socinfo.ko",
    "common_drivers/drivers/spi/amlogic-spi.ko",
    "common_drivers/drivers/tee/amlogic-tee.ko",
    "common_drivers/drivers/thermal/amlogic-thermal.ko",
    "common_drivers/drivers/usb/amlogic-usb.ko",
    "common_drivers/drivers/usb/dwc_otg.ko",
    "common_drivers/drivers/watchdog/amlogic-watchdog.ko",
    "common_drivers/drivers/wireless/amlogic-wireless.ko",
    "common_drivers/drivers/seckey/amlogic-seckey.ko",
    "common_drivers/sound/soc/amlogic/amlogic-snd-soc.ko",
    "common_drivers/sound/soc/codecs/amlogic/amlogic-snd-codec-ad82128.ko",
    "common_drivers/sound/soc/codecs/amlogic/amlogic-snd-codec-ad82584f.ko",
    "common_drivers/sound/soc/codecs/amlogic/amlogic-snd-codec-dummy.ko",
    "common_drivers/sound/soc/codecs/amlogic/amlogic-snd-codec-pa1.ko",
    "common_drivers/sound/soc/codecs/amlogic/amlogic-snd-codec-t9015.ko",
    "common_drivers/sound/soc/codecs/amlogic/amlogic-snd-codec-tas5707.ko",
    "common_drivers/sound/soc/codecs/amlogic/amlogic-snd-codec-tas5805.ko",
    "common_drivers/sound/soc/codecs/amlogic/amlogic-snd-codec-tl1.ko",
    "drivers/dma-buf/heaps/system_heap.ko",
    "drivers/i2c/i2c-dev.ko",
    "drivers/media/dvb-core/dvb-core.ko",
    "drivers/media/v4l2-core/v4l2-async.ko",
    "drivers/media/v4l2-core/v4l2-fwnode.ko",
    "drivers/media/v4l2-core/videobuf-core.ko",
    "drivers/media/v4l2-core/videobuf-vmalloc.ko",
    "drivers/mmc/host/cqhci.ko",
    "drivers/net/ethernet/stmicro/stmmac/dwmac-dwc-qos-eth.ko",
    "drivers/net/ethernet/stmicro/stmmac/dwmac-meson8b.ko",
    "drivers/net/ethernet/stmicro/stmmac/dwmac-meson.ko",
    "drivers/net/ethernet/stmicro/stmmac/stmmac.ko",
    "drivers/net/ethernet/stmicro/stmmac/stmmac-platform.ko",
    "drivers/net/mdio/mdio-mux.ko",
    "drivers/net/pcs/pcs_xpcs.ko",
    "drivers/regulator/gpio-regulator.ko",
    "drivers/regulator/pwm-regulator.ko",
    "fs/ntfs3/ntfs3.ko",
    "net/mac80211/mac80211.ko",
    "net/wireless/cfg80211.ko",
]

AMLOGIC_UPGRADE_COMMON_MODULES = [
    # keep sorted
    "arch/arm64/crypto/sha1-ce.ko",
    "common_drivers/drivers/crypto/amlogic-crypto-dma.ko",
    "common_drivers/drivers/debug/amlogic-audio-utils.ko",
    "common_drivers/drivers/drm/aml_drm.ko",
    "common_drivers/drivers/dvb_ci/amlogic-dvb-ci.ko",
    "common_drivers/drivers/dvb/demux/amlogic-dvb-demux.ko",
    "common_drivers/drivers/firmware/bl40_module.ko",
    "common_drivers/drivers/hifi4dsp/amlogic-hifidsp.ko",
    "common_drivers/drivers/hwspinlock/amlogic-hwspinlock.ko",
    "common_drivers/drivers/iio/adc/amlogic-adc.ko",
    "common_drivers/drivers/irblaster/amlogic-irblaster.ko",
    "common_drivers/drivers/jtag/amlogic-jtag.ko",
    "common_drivers/drivers/led/amlogic-led.ko",
    "common_drivers/drivers/media/camera/amlogic-camera.ko",
    "common_drivers/drivers/net/mdio/amlogic-mdio-g12a.ko",
    "common_drivers/drivers/net/phy/amlogic-inphy.ko",
    "common_drivers/drivers/pci/controller/amlogic_pcie_v2_host.ko",
    "common_drivers/drivers/pci/controller/amlogic-pcie-v3_host.ko",
    "common_drivers/drivers/rtc/amlogic-rtc.ko",
    "common_drivers/drivers/soc_info/amlogic-socinfo.ko",
    "common_drivers/drivers/thermal/amlogic-thermal.ko",
    "common_drivers/drivers/usb/amlogic-usb.ko",
    "common_drivers/drivers/usb/dwc_otg.ko",
    "common_drivers/drivers/wireless/amlogic-wireless.ko",
    "common_drivers/drivers/seckey/amlogic-seckey.ko",
    "common_drivers/drivers/net/ethernet/stmicro/stmmac/amlogic-phy-debug.ko",
    "common_drivers/sound/soc/amlogic/amlogic-snd-soc.ko",
    "common_drivers/sound/soc/codecs/amlogic/amlogic-snd-codec-ad82128.ko",
    "common_drivers/sound/soc/codecs/amlogic/amlogic-snd-codec-ad82584f.ko",
    "common_drivers/sound/soc/codecs/amlogic/amlogic-snd-codec-dummy.ko",
    "common_drivers/sound/soc/codecs/amlogic/amlogic-snd-codec-pa1.ko",
    "common_drivers/sound/soc/codecs/amlogic/amlogic-snd-codec-t9015.ko",
    "common_drivers/sound/soc/codecs/amlogic/amlogic-snd-codec-tas5707.ko",
    "common_drivers/sound/soc/codecs/amlogic/amlogic-snd-codec-tas5805.ko",
    "common_drivers/sound/soc/codecs/amlogic/amlogic-snd-codec-tl1.ko",
    "drivers/i2c/i2c-dev.ko",
    "drivers/media/v4l2-core/v4l2-async.ko",
    "drivers/media/v4l2-core/v4l2-fwnode.ko",
    "drivers/net/ethernet/stmicro/stmmac/dwmac-dwc-qos-eth.ko",
    "drivers/net/ethernet/stmicro/stmmac/dwmac-meson8b.ko",
    "drivers/net/ethernet/stmicro/stmmac/dwmac-meson.ko",
    "drivers/net/ethernet/stmicro/stmmac/stmmac.ko",
    "drivers/net/ethernet/stmicro/stmmac/stmmac-platform.ko",
    "drivers/net/mdio/mdio-mux.ko",
    "drivers/net/pcs/pcs_xpcs.ko",
    "drivers/regulator/gpio-regulator.ko",
    "fs/ntfs3/ntfs3.ko",
    "net/mac80211/mac80211.ko",
    "net/wireless/cfg80211.ko",
]

AMLOGIC_UPGRADE_P_MODULES = AMLOGIC_UPGRADE_COMMON_MODULES + [

]

AMLOGIC_UPGRADE_R_MODULES = AMLOGIC_UPGRADE_COMMON_MODULES + [

]

AMLOGIC_UPGRADE_S_MODULES = AMLOGIC_UPGRADE_COMMON_MODULES + [

]

AMLOGIC_UPGRADE_U_MODULES = AMLOGIC_UPGRADE_COMMON_MODULES + [

]

UPGRADE_MODULES_REMOVE_R = [
    "common_drivers/drivers/tty/serial/amlogic-uart.ko",
]

UPGRADE_MODULES_REMOVE_S = [
    "common_drivers/drivers/tty/serial/amlogic-uart.ko",
]

UPGRADE_MODULES_REMOVE_P = [
    "common_drivers/drivers/tty/serial/amlogic-uart.ko",
    "common_drivers/drivers/drm/aml_drm.ko",
    "common_drivers/drivers/hwspinlock/amlogic-hwspinlock.ko",
    "common_drivers/drivers/iio/adc/amlogic-adc.ko",
    "common_drivers/drivers/irblaster/amlogic-irblaster.ko",
    "common_drivers/drivers/media/camera/amlogic-camera.ko",
    "common_drivers/drivers/net/mdio/amlogic-mdio-g12a.ko",
    "common_drivers/drivers/net/phy/amlogic-inphy.ko",
    "common_drivers/drivers/thermal/amlogic-thermal.ko",
    "common_drivers/drivers/usb/amlogic-usb.ko",
    "common_drivers/drivers/net/ethernet/stmicro/stmmac/amlogic-phy-debug.ko",
    "drivers/net/ethernet/stmicro/stmmac/dwmac-dwc-qos-eth.ko",
    "drivers/net/ethernet/stmicro/stmmac/dwmac-meson8b.ko",
    "drivers/net/ethernet/stmicro/stmmac/dwmac-meson.ko",
    "drivers/net/ethernet/stmicro/stmmac/stmmac.ko",
    "drivers/net/ethernet/stmicro/stmmac/stmmac-platform.ko",
    "drivers/net/mdio/mdio-mux.ko",
    "drivers/net/pcs/pcs_xpcs.ko",
    "drivers/net/mii.ko",
]

UPGRADE_MODULES_REMOVE_U = [
    "common_drivers/drivers/tty/serial/amlogic-uart.ko",
]

AMLOGIC_GKIX_MODULES = AMLOGIC_GKI20_MODULES if GKI_CONFIG == "gki_20" else AMLOGIC_GKI10_MODULES
COMMON_MODULES = AMLOGIC_UPGRADE_R_MODULES if UPGRADE_PROJECT == "r" or UPGRADE_PROJECT == "R" else \
		 AMLOGIC_UPGRADE_S_MODULES if UPGRADE_PROJECT == "s" or UPGRADE_PROJECT == "S" else \
		 AMLOGIC_UPGRADE_P_MODULES if UPGRADE_PROJECT == "p" or UPGRADE_PROJECT == "P" else \
		 AMLOGIC_UPGRADE_U_MODULES if UPGRADE_PROJECT == "u" or UPGRADE_PROJECT == "U" else \
		 AMLOGIC_COMMON_MODULES
ALL_MODULES = COMMON_MODULES + AMLOGIC_GKIX_MODULES + VENDOR_MODULES_ADD + AMLOGIC_MODULES_ANDROID if ANDROID_PROJECT else \
	      COMMON_MODULES + AMLOGIC_GKIX_MODULES + VENDOR_MODULES_ADD

ALL_MODULES_REMOVE = \
	VENDOR_MODULES_REMOVE + UPGRADE_MODULES_REMOVE_R if UPGRADE_PROJECT == "r" or UPGRADE_PROJECT == "R" else \
	VENDOR_MODULES_REMOVE + UPGRADE_MODULES_REMOVE_S if UPGRADE_PROJECT == "s" or UPGRADE_PROJECT == "S" else \
	VENDOR_MODULES_REMOVE + UPGRADE_MODULES_REMOVE_P if UPGRADE_PROJECT == "p" or UPGRADE_PROJECT == "P" else \
	VENDOR_MODULES_REMOVE + UPGRADE_MODULES_REMOVE_U if UPGRADE_PROJECT == "u" or UPGRADE_PROJECT == "U" else \
	VENDOR_MODULES_REMOVE
remove_modules_items = {module: None for module in depset(ALL_MODULES_REMOVE).to_list()}

AMLOGIC_MODULES = [module for module in depset(ALL_MODULES).to_list() if module not in remove_modules_items]
