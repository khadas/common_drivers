# SPDX-License-Identifier: GPL-2.0-or-later

MODULE_NAME = amlogic-dvb
PR_FMT = $(subst amlogic-,,$(MODULE_NAME))
PR_FMT_DEFINE="-Dpr_fmt(fmt)= \"[$(PR_FMT)]: \" fmt"
ccflags-y += $(PR_FMT_DEFINE)

ccflags-y += -I.
ccflags-y += -I$(srctree)/drivers/media/dvb-core
ccflags-y += -I$(srctree)/include/media
ccflags-y += -I$(srctree)/$(COMMON_DRIVERS_DIR)/include

obj-$(CONFIG_AMLOGIC_DVB_CONFIG) = $(MODULE_NAME).o
$(MODULE_NAME)-y = main.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_DVB_EXTERN) += aml_demod_common.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_DVB_EXTERN) += aml_tuner.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_DVB_EXTERN) += aml_atvdemod.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_DVB_EXTERN) += aml_dtvdemod.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_DVB_EXTERN) += aml_tuner_ops.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_DVB_EXTERN) += aml_demod_ops.o
$(MODULE_NAME)-$(CONFIG_AMLOGIC_DVB_EXTERN) += aml_dvb_extern_driver.o
include $(COMMON_DRIVERS_DIR)/drivers/dvb/aucpu/Makefile
include $(COMMON_DRIVERS_DIR)/drivers/dvb/dsm/Makefile
include $(COMMON_DRIVERS_DIR)/drivers/dvb/smartcard_sc2/Makefile

obj-$(CONFIG_AMLOGIC_DVB_DMX) += demux/