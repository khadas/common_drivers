# SPDX-License-Identifier: (GPL-2.0+ OR MIT)
#
# Makefile for amlogic uvm
#

ccflags-y += -Idrivers/staging/android/ -I$(COMMON_DRIVERS_DIR)/drivers/

$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_UVM_ALLOCATOR)		+= common/uvm/meson_uvm_allocator.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_UVM_CORE)			+= common/uvm/meson_uvm_core.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_UVM_CORE)			+= common/uvm/meson_uvm_nn_processor.o
$(MEDIA_MODULE_NAME)-$(CONFIG_AMLOGIC_UVM_CORE)			+= common/uvm/meson_uvm_aipq_processor.o
