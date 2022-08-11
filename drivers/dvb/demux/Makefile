# SPDX-License-Identifier: GPL-2.0-or-later
#
#Makefile for the dvb dmx drivers
#

MODULE_NAME = amlogic-dvb-demux
obj-$(CONFIG_AMLOGIC_DVB_DMX) = $(MODULE_NAME).o

#PR_FMT = $(subst amlogic-,,$(MODULE_NAME)) 
#PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
# ccflags-y += $(PR_FMT_DEFINE)

#ccflags-y += -I$(srctree)/drivers/media/dvb-core -I$(srctree)/drivers/gpio -I$(srctree)/include -fno-tree-scev-cprop
#ccflags-y += -I$(srctree)/drivers/media/dvb-core -fno-tree-scev-cprop
ccflags-y += -I$(srctree)/include
ccflags-y += -I$(srctree)/include/media
ccflags-y += -I$(srctree)/include/uapi
ccflags-y += -I$(srctree)/$(COMMON_DRIVERS_DIR)/include/linux/amlogic
ccflags-y += -I$(srctree)/$(COMMON_DRIVERS_DIR)/include/uapi
ccflags-y += -I$(srctree)/$(COMMON_DRIVERS_DIR)/drivers/dvb

#ccflags-y += -I.
#ccflags-y += -DCOMMON_DRIVERS_DIR=$(COMMON_DRIVERS_DIR)

$(MODULE_NAME)-y += aml_dvb.o
$(MODULE_NAME)-y += aml_dsc.o
$(MODULE_NAME)-y += aml_dmx.o
$(MODULE_NAME)-y += aml_key.o
#$(MODULE_NAME)-y += dmxdev.o
#$(MODULE_NAME)-y += ringbuffer.o

$(MODULE_NAME)-y += sw_demux/swdmx_cb_list.o
$(MODULE_NAME)-y += sw_demux/swdmx_crc32.o
$(MODULE_NAME)-y += sw_demux/swdmx_demux.o
$(MODULE_NAME)-y += sw_demux/swdmx_ts_parser.o

$(MODULE_NAME)-y += sc2_demux/sc2_control.o
$(MODULE_NAME)-y += sc2_demux/mem_desc.o
$(MODULE_NAME)-y += sc2_demux/ts_output.o
$(MODULE_NAME)-y += sc2_demux/ts_input.o
$(MODULE_NAME)-y += sc2_demux/frontend.o
$(MODULE_NAME)-y += sc2_demux/dvb_reg.o
