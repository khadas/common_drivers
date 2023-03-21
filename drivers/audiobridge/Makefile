# SPDX-License-Identifier: (GPL-2.0+ OR MIT)
# Makefile for audiobridge

MODULE_NAME = amlogic-audiobridge


obj-$(CONFIG_AMLOGIC_AUDIO_BRIDGE) = $(MODULE_NAME).o

$(MODULE_NAME)-y += bridge_audio.o \
                    bridge_ringbuffer.o \
                    bridge_pcm_hal.o \
                    bridge_arm_pcm.o

ifeq ($(CONFIG_AMLOGIC_BRIDGE_UAC),y)
        $(MODULE_NAME)-y += bridge_uac_pcm.o
endif

ifeq ($(CONFIG_AMLOGIC_BRIDGE_DSP),y)
        $(MODULE_NAME)-y += bridge_dsp_pcm.o \
                            dsp_client_api.o \
                            bridge_dsp_card.o
endif

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)
