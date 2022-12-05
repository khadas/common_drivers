# SPDX-License-Identifier: GPL-2.0-or-later
#
#Makefile for the dvb dmx drivers
#

MODULE_NAME_DEMUX = amlogic-dvb-demux
obj-$(CONFIG_AMLOGIC_DVB_DMX) = $(MODULE_NAME_DEMUX).o

#PR_FMT_DEMUX = $(subst amlogic-,,$(MODULE_NAME_DEMUX))
#PR_FMT__DEMUX_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT_DEMUX)]: \" fmt"
# ccflags-y += $(PR_FMT__DEMUX_DEFINE)

ccflags-y += -I$(srctree)/include
ccflags-y += -I$(srctree)/include/media
ccflags-y += -I$(srctree)/include/uapi
ccflags-y += -I$(srctree)/$(COMMON_DRIVERS_DIR)/include/linux/amlogic
ccflags-y += -I$(srctree)/$(COMMON_DRIVERS_DIR)/include/uapi
ccflags-y += -I$(srctree)/$(COMMON_DRIVERS_DIR)/drivers/dvb

$(MODULE_NAME_DEMUX)-y += aml_dvb.o
$(MODULE_NAME_DEMUX)-y += aml_dsc.o
$(MODULE_NAME_DEMUX)-y += aml_dmx.o
$(MODULE_NAME_DEMUX)-y += aml_key.o
#$(MODULE_NAME_DEMUX)-y += dmxdev.o
#$(MODULE_NAME_DEMUX)-y += ringbuffer.o

$(MODULE_NAME_DEMUX)-y += sw_demux/swdmx_cb_list.o
$(MODULE_NAME_DEMUX)-y += sw_demux/swdmx_crc32.o
$(MODULE_NAME_DEMUX)-y += sw_demux/swdmx_demux.o
$(MODULE_NAME_DEMUX)-y += sw_demux/swdmx_ts_parser.o

$(MODULE_NAME_DEMUX)-y += sc2_demux/sc2_control.o
$(MODULE_NAME_DEMUX)-y += sc2_demux/mem_desc.o
$(MODULE_NAME_DEMUX)-y += sc2_demux/ts_output.o
$(MODULE_NAME_DEMUX)-y += sc2_demux/ts_input.o
$(MODULE_NAME_DEMUX)-y += sc2_demux/frontend.o
$(MODULE_NAME_DEMUX)-y += sc2_demux/dvb_reg.o
