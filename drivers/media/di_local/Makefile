# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

# Makefile for the Post Process Manager device #
#ifeq ($(TARGET_BUILD_VARIANT),userdebug)
#ccflags-y += -D DEBUG_SUPPORT
#ccflags-y += -DDEBUG
#else
#ccflags-y += -DDEBUG
#endif

#CFLAGS_di_local.o := -I$(src)
ccflags-y += -I$(src)/di_local/
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_DEINTERLACE) += di_local/di_local.o

#ccflags-y += -I$(COMMON_DRIVERS_DIR)/drivers/media/deinterlace/
