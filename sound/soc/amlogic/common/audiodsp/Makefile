# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

# Makefile for osd and tv

MODULE_NAME = amlogic_audiodsp

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)

obj-$(CONFIG_AMLOGIC_AUDIO_DSP)    +=$(MODULE_NAME).o
$(MODULE_NAME)-y += audiodsp_module.o

