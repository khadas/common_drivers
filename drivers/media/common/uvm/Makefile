# SPDX-License-Identifier: (GPL-2.0+ OR MIT)
#
# Makefile for amlogic uvm
#

ccflags-y += -Idrivers/staging/android/ -I$(COMMON_DRIVERS_DIR)/drivers/
ccflags-y += -I$(srctree)/$(COMMON_DRIVERS_DIR)/drivers/media/common/ion_dev/

$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_UVM_ALLOCATOR)		+= common/uvm/meson_uvm_allocator.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_UVM_CORE)			+= common/uvm/meson_uvm_core.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_UVM_CORE)			+= common/uvm/meson_uvm_nn_processor.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_UVM_CORE)			+= common/uvm/meson_uvm_aipq_processor.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_UVM_CORE)			+= common/uvm/amlogic_fbc_hook.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_UVM_CORE)			+= common/uvm/meson_uvm_buffer_info.o