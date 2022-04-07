# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_GDC) += gdc/src/fw_lib/acamera_gdc.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_GDC) += gdc/src/platform/system_gdc_io.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_GDC) += gdc/app/gdc_main.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_GDC) += gdc/app/gdc_module.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_GDC) += gdc/app/gdc_dmabuf.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_GDC) += gdc/app/gdc_wq.o

PWD=$(srctree)/$(COMMON_DRIVERS_DIR)/drivers/media/gdc
ccflags-y +=-I$(PWD)/app -I$(PWD)/inc
ccflags-y +=-I$(PWD)/inc/api -I$(PWD)/inc/gdc -I$(PWD)/inc/sys
ccflags-y +=-I$(PWD)/src/platform -I$(PWD)/src/fw_lib
ccflags-y += -I$(srctree)/$(COMMON_DRIVERS_DIR)/drivers/media/common/ion_dev/
ccflags-y += -I$(srctree)/drivers/staging/android/

ccflags-y += -Wno-declaration-after-statement
