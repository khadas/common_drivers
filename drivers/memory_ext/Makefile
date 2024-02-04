# SPDX-License-Identifier: (GPL-2.0+ OR MIT)

ccflags-y += -I$(srctree)/mm -I$(srctree)/drivers/iommu/arm/arm-smmu-v3
KBUILD_CFLAGS += -Wframe-larger-than=2068

obj-$(CONFIG_AMLOGIC_PAGE_TRACE)        += page_trace.o
obj-$(CONFIG_AMLOGIC_SLAB_TRACE)        += slab_trace.o
obj-$(CONFIG_AMLOGIC_CMA)               += aml_cma.o
obj-$(CONFIG_AMLOGIC_VMAP)              += vmap_stack.o
obj-$(CONFIG_AMLOGIC_MEM_DEBUG)         += mem_debug.o
obj-$(CONFIG_AMLOGIC_MEMORY_EXTEND)	+= memory.o
obj-$(CONFIG_AMLOGIC_USER_FAULT)	+= user_fault.o
obj-$(CONFIG_AMLOGIC_PIN_LOCKED_FILE)	+= pin_file.o
obj-$(CONFIG_AMLOGIC_PCIE_DMA_OPS)	+= dma_pcie_mapping.o
obj-$(CONFIG_AMLOGIC_FAKE_SMMU)		+= aml_smmu.o
obj-$(CONFIG_AMLOGIC_MIX_ZRAM)		+= mix_zram.o
