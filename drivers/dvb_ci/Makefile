# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

MODULE_NAME = amlogic-dvb-ci
obj-$(CONFIG_AMLOGIC_DVB_CI) +=$(MODULE_NAME).o

$(MODULE_NAME)-y = $(amlci-objs) $(cimcu-objs)

amlci-objs = aml_pcmcia.o  aml_ci.o  aml_ci_bus.o
cimcu-objs = cimcu/dvb_ca_en50221_cimcu.o cimax/dvb_ringbuffer.o

ccflags-y += -I$(srctree)/
ccflags-y += -I$(srctree)/include
ccflags-y += -I$(srctree)/drivers/gpio
ccflags-y += -I$(srctree)/drivers/media/dvb-core
ccflags-y += -I$(srctree)/include/media

PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)
