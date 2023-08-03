# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

# Makefile for the Post Process Manager device #
#ifeq ($(TARGET_BUILD_VARIANT),userdebug)
#ccflags-y += -D DEBUG_SUPPORT
#ccflags-y += -DDEBUG
#else
#ccflags-y += -DDEBUG
#endif

ccflags-y += -I.

#CFLAGS_dim.o := -I$(src)
ifdef CONFIG_AMLOGIC_ZAPPER_CUT
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_DEINTERLACE) += di_mini/deinterlace.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_DEINTERLACE) += di_mini/deinterlace_hw.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_DEINTERLACE) += di_mini/deinterlace_dbg.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_DEINTERLACE) += di_mini/nr_downscale.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_DEINTERLACE) += di_mini/di_pps.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_DEINTERLACE) += di_mini/di_vframe.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_DEINTERLACE) += di_mini/di_prc.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_DEINTERLACE) += di_mini/di_pre.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_DEINTERLACE) += di_mini/di_post.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_DEINTERLACE) += di_mini/di_reg_tab.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_DEINTERLACE) += di_mini/di_dbg.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_DEINTERLACE) += di_mini/di_que.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_DEINTERLACE) += di_mini/di_sys.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_DEINTERLACE) += di_mini/di_task.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_DEINTERLACE) += di_mini/di_api.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_DEINTERLACE) += di_mini/di_que_buf.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_DEINTERLACE) += di_mini/di_mmu_box.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_DEINTERLACE) += di_mini/di_mem_scatter.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_DEINTERLACE) += di_mini/di_plink_api.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_DEINTERLACE) += di_mini/di_interface.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_DEINTERLACE) += di_mini/di_hw_v2.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_DEINTERLACE) += di_mini/di_hw_v3.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_DEINTERLACE) += di_mini/di_afbc_v3.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_DEINTERLACE) += di_mini/di_afbc_dbg.o

ccflags-y += -I$(COMMON_DRIVERS_DIR)/drivers/media/common/rdma/
ccflags-y += -I$(src)/di_mini/
ccflags-y += -I$(src)/di_mini/sc2
endif
ccflags-y += -I$(COMMON_DRIVERS_DIR)/drivers/media/di_local/
ccflags-y += -DCOMMON_DRIVERS_DIR=$(COMMON_DRIVERS_DIR)
