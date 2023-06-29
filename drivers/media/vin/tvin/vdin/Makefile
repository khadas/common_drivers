# SPDX-License-Identifier: (GPL-2.0+ OR MIT)
#
# Makefile for Vdin.
#
#ifeq ($(TARGET_BUILD_VARIANT),userdebug)
#ccflags-y += -DDEBUG_SUPPORT
#else
#ccflags-y += -DDEBUG
#endif

$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_VDIN) += vin/tvin/vdin/vdin_vf.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_VDIN) += vin/tvin/vdin/vdin_debug.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_VDIN) += vin/tvin/vdin/vdin_drv.o
ifndef CONFIG_AMLOGIC_ZAPPER_CUT
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_VDIN) += vin/tvin/vdin/vdin_afbce_t3x.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_VDIN) += vin/tvin/vdin/vdin_ctl_t3x.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_VDIN) += vin/tvin/vdin/vdin_ctl_s5.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_VDIN) += vin/tvin/vdin/vdin_afbce.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_VDIN) += vin/tvin/vdin/vdin_mmu_box.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_VDIN) += vin/tvin/vdin/vdin_mem_scatter.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_VDIN) += vin/tvin/vdin/vdin_v4l2.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_VDIN) += vin/tvin/vdin/vdin_v4l2_dbg.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_VDIN) += vin/tvin/vdin/vdin_v4l2_if.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_VDIN) += vin/tvin/vdin/vdin_dv.o
endif
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_VDIN) += vin/tvin/vdin/vdin_ctl.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_VDIN) += vin/tvin/vdin/vdin_screen.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_VDIN) += vin/tvin/vdin/vdin_sm.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_VDIN) += vin/tvin/vdin/vdin_canvas.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_MEDIA_VDIN) += vin/tvin/vdin/vdin_hw.o

