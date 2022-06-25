# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

obj-y    += common/audiodsp/
obj-y    += common/amaudio/
obj-y    += common/audioinfo/

KBUILD_CFLAGS += -Wlarger-than=32768
# KBUILD_CFLAGS += -Wstack-usage=1856

MODULE_NAME = amlogic-snd-soc

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)

obj-$(CONFIG_AMLOGIC_SND_SOC) += $(MODULE_NAME).o
$(MODULE_NAME)-y += sound_init.o

$(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/clks/clk-sm1.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/clks/clk-a1.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/clks/clk-sc2.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/clks/clk-c3.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/clks/audio_clks.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/audio_controller.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/audio_io.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/iomap.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/pinctrl/pctrl-audio.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/pinctrl/pctrl-audio-sm1.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/card_utils.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/pdm.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/pdm_hw.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/ddr_mngr.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/aud_sram.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/tdm.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/tdm_hw.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/spdif.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/spdif_hw.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/loopback_hw.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/audio_utils.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/sharebuffer.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/loopback.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/locker.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/locker_hw.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/resample_hw.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/resample.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/effects.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/effects_hw.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/effects_v2.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/effects_hw_v2.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/vad.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/vad_hw.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/vad_dev.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/aud_sram.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/pcpd_monitor.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/pwrdet_hw.o
# $(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/wwe.o
# $(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/pwrdet.o
# $(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/pwrdet_hw.o
# $(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/sharebuffer.o
# $(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/extn.o
# $(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/frhdmirx_hw.o
# $(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/earc.o
# $(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/earc_hw.o
# $(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/dsp/rpc_dev.o
# $(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/dsp/rpc_client_aipc.o
# $(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/dsp/rpc_client_shm.o
# $(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/dsp/rpc_client_vsp.o

# $(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/dsp/aml_wakeup_api.o
# $(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/dsp/aml_flatbuf_api.o
# $(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/dsp/hifi4rpc_awe.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += auge/card.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += common/iec_info.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += common/misc.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += common/audio_uevent.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_SND_SOC_AUGE) += common/iomapres.o
