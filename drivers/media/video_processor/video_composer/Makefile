# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

KBUILD_CFLAGS += -Wframe-larger-than=3072
asflags-y += -mfloat-abi=softfp -mfpu=neon
ccflags-y += -Idrivers/staging/android/
ccflags-y += -I$(srctree)/$(COMMON_DRIVERS_DIR)/drivers/media/common/ion_dev/
ccflags-y += -I$(srctree)/$(COMMON_DRIVERS_DIR)/drivers/media/frc/

$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_VIDEO_COMPOSER) += video_processor/video_composer/video_composer.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_VIDEO_COMPOSER) += video_processor/video_composer/vframe_ge2d_composer.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_VIDEO_COMPOSER) += video_processor/video_composer/videodisplay.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_VIDEO_COMPOSER) += video_processor/video_composer/vframe_dewarp_composer.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_VIDEO_COMPOSER) += video_processor/video_composer/vframe_vicp_composer.o