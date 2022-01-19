# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

# Makefile for hifi4dsp

MODULE_NAME = amlogic-hifidsp
obj-$(CONFIG_AMLOGIC_HIFI4DSP)	= $(MODULE_NAME).o
$(MODULE_NAME)-y		+= hifi4dsp_module.o \
				   hifi4dsp_firmware.o \
				   hifi4dsp_dsp.o \
				   dsp_top.o \
				   hifi4dsp_syslog.o

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)
