# SPDX-License-Identifier: (GPL-2.0+ OR MIT)
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_FB) += osd/osd_hw.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_FB) += osd/osd_fb.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_FB) += osd/osd_debug.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_FB) += osd/osd_backup.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_FB) += osd/osd_logo.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_FB) += osd/osd_io.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_FB) += osd/osd_antiflicker.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_FB) += osd/osd_clone.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_FB) += osd/osd_drm.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_FB) += osd/osd_rdma.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_FB) += osd/osd_virtual.o
$(MEDIA_MODULE_NAME)-$(CONFIG_INSTABOOT) += osd/osd_progressbar.o

#ifdef CONFIG_SYNC_FILE
#$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_FB) += osd/osd_sw_sync.o
#endif

ccflags-y += -I$(srctree)/$(COMMON_DRIVERS_DIR)/drivers/media/common/ion_dev/
ccflags-y += -I$(srctree)/drivers/staging/android/
