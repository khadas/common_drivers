# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

ccflags-y += -DDVB_CORE_ORI=1

ccflags-y += -DAML_DEMOD_SUPPORT_DTMB
ccflags-y += -DAML_DEMOD_SUPPORT_ATSC
ccflags-y += -DAML_DEMOD_SUPPORT_J83B
ccflags-y += -DAML_DEMOD_SUPPORT_ISDBT
ccflags-y += -DAML_DEMOD_SUPPORT_DVBT
ccflags-y += -DAML_DEMOD_SUPPORT_DVBS
ccflags-y += -DAML_DEMOD_SUPPORT_DVBC

$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_DTV_DEMOD) += dtv_demod/demod_func.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_DTV_DEMOD) += dtv_demod/i2c_func.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_DTV_DEMOD) += dtv_demod/tuner_func.o

ifneq ($(findstring AML_DEMOD_SUPPORT_ATSC, $(ccflags-y)),)
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_DTV_DEMOD) += dtv_demod/atsc/atsc_func.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_DTV_DEMOD) += dtv_demod/atsc/atsc_frontend.o
ccflags-y += -I$(srctree)/$(COMMON_DRIVERS_DIR)/drivers/media/dtv_demod/atsc
endif

ifneq ($(findstring AML_DEMOD_SUPPORT_DVBC, $(ccflags-y)),)
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_DTV_DEMOD) += dtv_demod/dvbc/dvbc_func.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_DTV_DEMOD) += dtv_demod/dvbc/dvbc_v2.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_DTV_DEMOD) += dtv_demod/dvbc/dvbc_v3.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_DTV_DEMOD) += dtv_demod/dvbc/dvbc_frontend.o
ccflags-y += -I$(srctree)/$(COMMON_DRIVERS_DIR)/drivers/media/dtv_demod/dvbc
endif

ifneq ($(findstring AML_DEMOD_SUPPORT_DVBT, $(ccflags-y)),)
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_DTV_DEMOD) += dtv_demod/dvbt/dvbt_v2.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_DTV_DEMOD) += dtv_demod/dvbt/dvbt_frontend.o
ccflags-y += -I$(srctree)/$(COMMON_DRIVERS_DIR)/drivers/media/dtv_demod/dvbt
endif

ifneq ($(findstring AML_DEMOD_SUPPORT_DTMB, $(ccflags-y)),)
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_DTV_DEMOD) += dtv_demod/dtmb/dtmb_func.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_DTV_DEMOD) += dtv_demod/dtmb/dtmb_frontend.o
ccflags-y += -I$(srctree)/$(COMMON_DRIVERS_DIR)/drivers/media/dtv_demod/dtmb
endif

$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_DTV_DEMOD) += dtv_demod/amlfrontend.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_DTV_DEMOD) += dtv_demod/demod_dbg.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_DTV_DEMOD) += dtv_demod/aml_demod.o

ifneq ($(findstring AML_DEMOD_SUPPORT_DVBS, $(ccflags-y)),)
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_DTV_DEMOD) += dtv_demod/dvbs/dvbs.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_DTV_DEMOD) += dtv_demod/dvbs/dvbs_frontend.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_DTV_DEMOD) += dtv_demod/dvbs/dvbs_diseqc.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_DTV_DEMOD) += dtv_demod/dvbs/dvbs_singlecable.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_DTV_DEMOD) += dtv_demod/lnb_controller/gpio/gpio_lnbc.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_DTV_DEMOD) += dtv_demod/lnb_controller/wt20_1811/wt20_1811.o
ccflags-y += -I$(srctree)/$(COMMON_DRIVERS_DIR)/drivers/media/dtv_demod/dvbs
ccflags-y += -I$(srctree)/$(COMMON_DRIVERS_DIR)/drivers/media/dtv_demod/lnb_controller
ccflags-y += -I$(srctree)/$(COMMON_DRIVERS_DIR)/drivers/media/dtv_demod/lnb_controller/wt20_1811
ccflags-y += -I$(srctree)/$(COMMON_DRIVERS_DIR)/drivers/media/dtv_demod/lnb_controller/gpio
endif

ifneq ($(findstring AML_DEMOD_SUPPORT_ISDBT, $(ccflags-y)),)
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_DTV_DEMOD) += dtv_demod/isdbt/isdbt_func.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_DTV_DEMOD) += dtv_demod/isdbt/isdbt_frontend.o
ccflags-y += -I$(srctree)/$(COMMON_DRIVERS_DIR)/drivers/media/dtv_demod/isdbt
endif

ifneq ($(findstring AML_DEMOD_SUPPORT_J83B, $(ccflags-y)),)
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_DTV_DEMOD) += dtv_demod/j83b/j83b_func.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_DTV_DEMOD) += dtv_demod/j83b/j83b_frontend.o
ccflags-y += -I$(srctree)/$(COMMON_DRIVERS_DIR)/drivers/media/dtv_demod/j83b
endif

ccflags-y += -I.
ccflags-y += -I$(srctree)/drivers/media/dvb-core
ccflags-y += -I$(srctree)/drivers/iio/adc
ccflags-y += -I$(srctree)/include/media
ccflags-y += -I$(srctree)/$(COMMON_DRIVERS_DIR)/drivers/media/dtv_demod/include

