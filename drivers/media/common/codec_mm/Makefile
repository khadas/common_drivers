# SPDX-License-Identifier: (GPL-2.0+ OR MIT)
################################
# Amlogic codec memory manager #
################################

####################################
# The order of directories matter. #
# Do not change.                   #
####################################

$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_CODEC_MM) += common/codec_mm/codec_mm.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_CODEC_MM) += common/codec_mm/codec_mm_scatter.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_CODEC_MM) += common/codec_mm/codec_mm_keeper.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_CODEC_MM) += common/codec_mm/secmem.o

include $(COMMON_DRIVERS_DIR)/drivers/media/common/codec_mm/configs/Makefile
CFLAGS_secmem.o += -Wframe-larger-than=${CONFIG_FRAME_WARN}
