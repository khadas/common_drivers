# SPDX-License-Identifier: (GPL-2.0+ OR MIT)
################################
# Amlogic codec memory manager #
################################

####################################
# The order of directories matter. #
# Do not change.                   #
####################################

ccflags-y += -I$(srctree)/$(COMMON_DRIVERS_DIR)/drivers/dvb/

$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_CODEC_MM) += common/codec_mm/codec_mm.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_CODEC_MM) += common/codec_mm/codec_mm_scatter.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_CODEC_MM) += common/codec_mm/codec_mm_keeper.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_CODEC_MM) += common/codec_mm/dmabuf_manage.o

include $(COMMON_DRIVERS_DIR)/drivers/media/common/codec_mm/configs/Makefile

