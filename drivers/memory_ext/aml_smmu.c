// SPDX-License-Identifier: GPL-2.0
/*
 * IOMMU API for ARM architected SMMUv3 implementations.
 *
 * Copyright (C) 2015 ARM Limited
 *
 * Author: Will Deacon <will.deacon@arm.com>
 *
 * This driver is powered by bad coffee and bombay mix.
 */

#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <linux/dma-iommu.h>
#include <linux/err.h>
#include <linux/iommu.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_iommu.h>
#include <linux/of_platform.h>
#include <linux/pci.h>
#include <linux/pci-ats.h>
#include <linux/platform_device.h>
#include <linux/of_reserved_mem.h>

#include <linux/amba/bus.h>
#include <linux/arm-smccc.h>
#include <trace/hooks/iommu.h>
#include <linux/amlogic/tee.h>
#include <linux/dma-map-ops.h>

#include <linux/dma-mapping.h>
#include <linux/dma-direct.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/pfn.h>
#include <linux/types.h>
#include <linux/highmem.h>
#include <linux/gfp.h>
#include <asm/dma.h>
#include <linux/printk.h>

#include <linux/init.h>
#include <linux/iommu-helper.h>
#include <linux/of.h>
#include <linux/genalloc.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/set_memory.h>

#include "arm-smmu-v3.h"

struct aml_smmu_ll_queue {
	union {
		u64			val;
		struct {
			u32		prod;
			u32		cons;
		};
		struct {
			atomic_t	prod;
			atomic_t	cons;
		} atomic;
		u8			__pad[SMP_CACHE_BYTES];
	} ____cacheline_aligned_in_smp;
	u32				max_n_shift;
};

struct aml_smmu_queue {
	struct aml_smmu_ll_queue	llq;
	int				irq; /* Wired interrupt */

	__le64				*base;
	dma_addr_t			base_dma;
	u64				q_base;

	size_t				ent_dwords;

	u32 __iomem			*prod_reg;
	u32 __iomem			*cons_reg;
};

struct aml_smmu_cmdq {
	struct aml_smmu_queue		q;
	atomic_long_t			*valid_map;
	atomic_t			owner_prod;
	atomic_t			lock;
};

struct aml_smmu_evtq {
	struct aml_smmu_queue		q;
	u32				max_stalls;
};

struct aml_smmu_priq {
	struct aml_smmu_queue		q;
};

/* High-level stream table and context descriptor structures */
struct aml_smmu_strtab_l1_desc {
	u8				span;

	__le64				*l2ptr;
	dma_addr_t			l2ptr_dma;
};

struct aml_smmu_strtab_cfg {
	__le64				*strtab;
	dma_addr_t			strtab_dma;
	struct aml_smmu_strtab_l1_desc	*l1_desc;
	unsigned int			num_l1_ents;

	u64				strtab_base;
	u32				strtab_base_cfg;
};

/* An SMMUv3 instance */
struct aml_smmu_device {
	struct device			*dev;
	void __iomem			*base;

	u32				features;

	u32				options;

	struct aml_smmu_cmdq		cmdq;
	struct aml_smmu_evtq		evtq;
	struct aml_smmu_priq		priq;

	int				gerr_irq;
	int				combined_irq;

	unsigned long			ias; /* IPA */
	unsigned long			oas; /* PA */
	unsigned long			pgsize_bitmap;

#define ARM_SMMU_MAX_ASIDS		(1 << 16)
	unsigned int			asid_bits;
	DECLARE_BITMAP(asid_map, ARM_SMMU_MAX_ASIDS);

#define ARM_SMMU_MAX_VMIDS		(1 << 16)
	unsigned int			vmid_bits;
	DECLARE_BITMAP(vmid_map, ARM_SMMU_MAX_VMIDS);

	unsigned int			ssid_bits;
	unsigned int			sid_bits;

	struct aml_smmu_strtab_cfg	strtab_cfg;

	/* IOMMU core code handle */
	struct iommu_device		iommu;
};

struct aml_iommu_group {
	struct kobject kobj;
	struct kobject *devices_kobj;
	struct list_head devices;
	struct mutex mutex;
	struct blocking_notifier_head notifier;
	void *iommu_data;
	void (*iommu_data_release)(void *iommu_data);
	char *name;
	int id;
	struct iommu_domain *default_domain;
	struct iommu_domain *domain;
	struct list_head entry;
};

struct aml_iommu_group *aml_global_group;

static struct iommu_device *aml_smmu_add_device(struct device *dev)
{
	return 0;
}

static int aml_smmu_of_xlate(struct device *dev, struct of_phandle_args *args)
{
	dev->iommu_group = (struct iommu_group *)aml_global_group;
	return 0;
}

static struct iommu_group *aml_smmu_device_group(struct device *dev)
{
	struct aml_iommu_group *group;

	/*
	 * We don't support devices sharing stream IDs other than PCI RID
	 * aliases, since the necessary ID-to-device lookup becomes rather
	 * impractical given a potential sparse 32-bit stream ID space.
	 */
	/*
	 *if (dev_is_pci(dev))
	 *	group = pci_device_group(dev);
	 *else
	 *	group = generic_device_group(dev);
	 */
	group = kzalloc(sizeof(*group), GFP_KERNEL);
	if (!group)
		return ERR_PTR(-ENOMEM);

	return (struct iommu_group *)group;
}

static int aml_smmu_attach_dev(struct iommu_domain *domain, struct device *dev)
{
	return 0;
}

static struct iommu_domain *aml_smmu_domain_alloc(unsigned int type)
{
	struct arm_smmu_domain *smmu_domain;

	/*
	 * Allocate the domain and initialise some of its data structures.
	 * We can't really do anything meaningful until we've added a
	 * master.
	 */
	smmu_domain = kzalloc(sizeof(*smmu_domain), GFP_KERNEL);
	if (!smmu_domain)
		return NULL;

	return &smmu_domain->domain;
}

static void aml_smmu_release_device(struct device *dev)
{
	return;
}

static struct iommu_ops aml_smmu_ops = {
	.owner		= THIS_MODULE,
	.domain_alloc	= aml_smmu_domain_alloc,
	.probe_device	= aml_smmu_add_device,
	.device_group	= aml_smmu_device_group,
	.attach_dev	= aml_smmu_attach_dev,
	.of_xlate	= aml_smmu_of_xlate,
	.pgsize_bitmap	= -1UL, /* Restricted during device attach */
	.release_device	= aml_smmu_release_device,
};

/*************************************************/
/*
 * Enumeration for sync targets
 */
enum dma_sync_target {
	SYNC_FOR_CPU = 0,
	SYNC_FOR_DEVICE = 1,
};

#define OFFSET(val, align) ((unsigned long) \
			((val) & ((align) - 1)))

/* default to 32MB */
#define AML_IO_TLB_DEFAULT_SIZE (64UL << 20)

/*
 * Maximum allowable number of contiguous slabs to map,
 * must be a power of 2.  What is the appropriate value ?
 * The complexity of {map,unmap}_single is linearly dependent on this value.
 */

#ifdef IO_TLB_SEGSIZE
#undef IO_TLB_SEGSIZE
#endif
#define IO_TLB_SEGSIZE	2048

/*
 * log of the size of each IO TLB slab.  The number of slabs is command line
 * controllable.
 */
#define IO_TLB_SHIFT 11

void (*aml_dma_common_free_remap)(void *cpu_addr, size_t size);
int (*aml_set_memory_encrypted)(unsigned long addr, int numpages);
int (*aml_set_memory_decrypted)(unsigned long addr, int numpages);
#ifdef CONFIG_DMA_DIRECT_REMAP
void * (*aml_dma_common_contiguous_remap)(struct page *page, size_t size,
			pgprot_t prot, const void *caller);
#endif
void (*aml_arch_dma_prep_coherent)(struct page *page, size_t size);
struct page * (*aml_dma_alloc_from_contiguous)(struct device *dev, size_t count,
				       unsigned int align, bool no_warn);
void (*aml_arch_sync_dma_for_device)(phys_addr_t paddr, size_t size,
		enum dma_data_direction dir);
void (*aml_arch_sync_dma_for_cpu)(phys_addr_t paddr, size_t size,
		enum dma_data_direction dir);
void (*_aml_dma_direct_free)(struct device *dev, size_t size,
		void *cpu_addr, dma_addr_t dma_addr, unsigned long attrs);
void *(*_aml_dma_direct_alloc)(struct device *dev, size_t size,
		dma_addr_t *dma_handle, gfp_t gfp, unsigned long attrs);
int (*aml_dma_mmap_from_dev_coherent)(struct device *dev, struct vm_area_struct *vma,
			   void *vaddr, size_t size, int *ret);
#ifdef CONFIG_MMU
pgprot_t (*aml_dma_pgprot)(struct device *dev, pgprot_t prot, unsigned long attrs);
#endif

unsigned long aml_max_pfn;

unsigned long (*aml_kallsyms_lookup_name)(const char *name);

/* For each probe you need to allocate a kprobe structure */
static struct kprobe kp_lookup_name = {
	.symbol_name	= "kallsyms_lookup_name",
};

/*
 * We need to save away the original address corresponding to a mapped entry
 * for the sync operations.
 */
#define INVALID_PHYS_ADDR (~(phys_addr_t)0)
static phys_addr_t *io_tlb_orig_addr;

/*
 * Protect the above data structures in the map and unmap calls
 */
static DEFINE_SPINLOCK(io_tlb_lock);

static bool no_iotlb_memory;

/*
 * Used to do a quick range check in aml_swiotlb_tbl_unmap_single and
 * swiotlb_tbl_sync_single_*, to see if the memory was in fact allocated by this
 * API.
 */
static phys_addr_t io_tlb_start, io_tlb_end;

/*
 * The number of IO TLB blocks (in groups of 64) between io_tlb_start and
 * io_tlb_end.  This is command line adjustable via setup_io_tlb_npages.
 */
static unsigned long io_tlb_nslabs;

/*
 * The number of used IO TLB block
 */
static unsigned long io_tlb_used;

/*
 * This is a free list describing the number of free entries available from
 * each index
 */
static unsigned int *io_tlb_list;
static unsigned int io_tlb_index;

static inline bool aml_is_swiotlb_buffer(struct device *dev, phys_addr_t paddr)
{
	return paddr >= io_tlb_start && paddr < io_tlb_end;
}

/*
 * Bounce: copy the swiotlb buffer from or back to the original dma location
 */
static void swiotlb_bounce(phys_addr_t orig_addr, phys_addr_t tlb_addr,
			   size_t size, enum dma_data_direction dir)
{
	unsigned long pfn = PFN_DOWN(orig_addr);
	unsigned char *vaddr = phys_to_virt(tlb_addr);

	if (PageHighMem(pfn_to_page(pfn))) {
		/* The buffer does not have a mapping.  Map it in and copy */
		unsigned int offset = orig_addr & ~PAGE_MASK;
		char *buffer;
		unsigned int sz = 0;
		unsigned long flags;

		while (size) {
			sz = min_t(size_t, PAGE_SIZE - offset, size);

			local_irq_save(flags);
			buffer = kmap_atomic(pfn_to_page(pfn));
			if (dir == DMA_TO_DEVICE)
				memcpy(vaddr, buffer + offset, sz);
			else
				memcpy(buffer + offset, vaddr, sz);
			kunmap_atomic(buffer);
			local_irq_restore(flags);

			size -= sz;
			pfn++;
			vaddr += sz;
			offset = 0;
		}
	} else if (dir == DMA_TO_DEVICE) {
		memcpy(vaddr, phys_to_virt(orig_addr), size);
	} else {
		memcpy(phys_to_virt(orig_addr), vaddr, size);
	}
}

static phys_addr_t aml_swiotlb_tbl_map_single(struct device *hwdev,
				   dma_addr_t tbl_dma_addr,
				   phys_addr_t orig_addr,
				   size_t mapping_size,
				   size_t alloc_size,
				   enum dma_data_direction dir,
				   unsigned long attrs)
{
	unsigned long flags;
	phys_addr_t tlb_addr;
	unsigned int nslots, stride, index, wrap;
	int i;
	unsigned long mask;
	unsigned long offset_slots;
	unsigned long max_slots;
	unsigned long tmp_io_tlb_used;

	if (no_iotlb_memory)
		panic("Can not allocate SWIOTLB buffer earlier and can't now provide you with the DMA bounce buffer");

	if (mapping_size > alloc_size) {
		dev_warn_once(hwdev, "Invalid sizes (mapping: %zd bytes, alloc: %zd bytes)",
			      mapping_size, alloc_size);
		return (phys_addr_t)DMA_MAPPING_ERROR;
	}

	mask = dma_get_seg_boundary(hwdev);

	tbl_dma_addr &= mask;

	offset_slots = ALIGN(tbl_dma_addr, 1 << IO_TLB_SHIFT) >> IO_TLB_SHIFT;

	/*
	 * Carefully handle integer overflow which can occur when mask == ~0UL.
	 */
	max_slots = mask + 1
		    ? ALIGN(mask + 1, 1 << IO_TLB_SHIFT) >> IO_TLB_SHIFT
		    : 1UL << (BITS_PER_LONG - IO_TLB_SHIFT);

	/*
	 * For mappings greater than or equal to a page, we limit the stride
	 * (and hence alignment) to a page size.
	 */
	nslots = ALIGN(alloc_size, 1 << IO_TLB_SHIFT) >> IO_TLB_SHIFT;
	if (alloc_size >= PAGE_SIZE)
		stride = (1 << (PAGE_SHIFT - IO_TLB_SHIFT));
	else
		stride = 1;

	WARN_ON(!nslots);

	/*
	 * Find suitable number of IO TLB entries size that will fit this
	 * request and allocate a buffer from that IO TLB pool.
	 */
	spin_lock_irqsave(&io_tlb_lock, flags);

	if (unlikely(nslots > io_tlb_nslabs - io_tlb_used))
		goto not_found;

	index = ALIGN(io_tlb_index, stride);
	if (index >= io_tlb_nslabs)
		index = 0;
	wrap = index;

	do {
		while (iommu_is_span_boundary(index, nslots, offset_slots,
					      max_slots)) {
			index += stride;
			if (index >= io_tlb_nslabs)
				index = 0;
			if (index == wrap)
				goto not_found;
		}

		/*
		 * If we find a slot that indicates we have 'nslots' number of
		 * contiguous buffers, we allocate the buffers from that slot
		 * and mark the entries as '0' indicating unavailable.
		 */
		if (io_tlb_list[index] >= nslots) {
			int count = 0;

			for (i = index; i < (int)(index + nslots); i++)
				io_tlb_list[i] = 0;
			for (i = index - 1; (OFFSET(i, IO_TLB_SEGSIZE) !=
				IO_TLB_SEGSIZE - 1) && io_tlb_list[i]; i--)
				io_tlb_list[i] = ++count;
			tlb_addr = io_tlb_start + (index << IO_TLB_SHIFT);

			/*
			 * Update the indices to avoid searching in the next
			 * round.
			 */
			io_tlb_index = ((index + nslots) < io_tlb_nslabs
					? (index + nslots) : 0);

			goto found;
		}
		index += stride;
		if (index >= io_tlb_nslabs)
			index = 0;
	} while (index != wrap);

not_found:
	tmp_io_tlb_used = io_tlb_used;

	spin_unlock_irqrestore(&io_tlb_lock, flags);
	if (!(attrs & DMA_ATTR_NO_WARN) && __printk_ratelimit(__func__))
		dev_warn(hwdev, "swiotlb buffer is full (sz: %zd bytes), total %lu (slots), used %lu (slots)\n",
			 alloc_size, io_tlb_nslabs, tmp_io_tlb_used);
	return (phys_addr_t)DMA_MAPPING_ERROR;
found:
	io_tlb_used += nslots;
	spin_unlock_irqrestore(&io_tlb_lock, flags);

	/*
	 * Save away the mapping from the original address to the DMA address.
	 * This is needed when we sync the memory.  Then we sync the buffer if
	 * needed.
	 */
	for (i = 0; i < nslots; i++)
		io_tlb_orig_addr[index + i] = orig_addr + (i << IO_TLB_SHIFT);
	if (!(attrs & DMA_ATTR_SKIP_CPU_SYNC) &&
	    (dir == DMA_TO_DEVICE || dir == DMA_BIDIRECTIONAL))
		swiotlb_bounce(orig_addr, tlb_addr, mapping_size, DMA_TO_DEVICE);

	return tlb_addr;
}

/*
 * Create a swiotlb mapping for the buffer at @phys, and in case of DMAing
 * to the device copy the data into it as well.
 */
static bool aml_swiotlb_map(struct device *dev, phys_addr_t *phys, dma_addr_t *dma_addr,
		size_t size, enum dma_data_direction dir, unsigned long attrs)
{
	/* Oh well, have to allocate and map a bounce buffer. */
	*phys = aml_swiotlb_tbl_map_single(dev, phys_to_dma(dev, io_tlb_start),
			*phys, size, size, dir, attrs);
	if (*phys == (phys_addr_t)DMA_MAPPING_ERROR)
		return false;

	/* Ensure that the address returned is DMA'ble */
	*dma_addr = phys_to_dma(dev, *phys);

	return true;
}

static void swiotlb_cleanup(void)
{
	io_tlb_end = 0;
	io_tlb_start = 0;
	io_tlb_nslabs = 0;
}

static void aml_swiotlb_print_info(void)
{
	unsigned long bytes = io_tlb_nslabs << IO_TLB_SHIFT;

	if (no_iotlb_memory) {
		pr_warn("No low mem\n");
		return;
	}

	pr_info("mapped [mem %#010llx-%#010llx] (%luMB)\n",
		(unsigned long long)io_tlb_start,
		(unsigned long long)io_tlb_end, bytes >> 20);
}

static int aml_swiotlb_init_with_tbl(phys_addr_t tlb, unsigned long nslabs)
{
	unsigned long i, bytes;

	bytes = nslabs << IO_TLB_SHIFT;

	io_tlb_nslabs = nslabs;
	io_tlb_start = tlb;
	io_tlb_end = io_tlb_start + bytes;

	/*
	 * Allocate and initialize the free list array.  This array is used
	 * to find contiguous free memory regions of size up to IO_TLB_SEGSIZE
	 * between io_tlb_start and io_tlb_end.
	 */
	io_tlb_list = (unsigned int *)__get_free_pages(GFP_KERNEL,
			get_order(io_tlb_nslabs * sizeof(int)));
	if (!io_tlb_list)
		goto cleanup3;

	io_tlb_orig_addr = (phys_addr_t *)
		__get_free_pages(GFP_KERNEL,
				get_order(io_tlb_nslabs * sizeof(phys_addr_t)));
	if (!io_tlb_orig_addr)
		goto cleanup4;

	for (i = 0; i < io_tlb_nslabs; i++) {
		io_tlb_list[i] = IO_TLB_SEGSIZE - OFFSET(i, IO_TLB_SEGSIZE);
		io_tlb_orig_addr[i] = INVALID_PHYS_ADDR;
	}
	io_tlb_index = 0;
	no_iotlb_memory = false;

	aml_swiotlb_print_info();

	return 0;

cleanup4:
	free_pages((unsigned long)io_tlb_list,
		get_order(io_tlb_nslabs * sizeof(int)));
	io_tlb_list = NULL;
cleanup3:
	swiotlb_cleanup();
	return -ENOMEM;
}

/*
 * tlb_addr is the physical address of the bounce buffer to unmap.
 */
static void aml_swiotlb_tbl_unmap_single(struct device *hwdev, phys_addr_t tlb_addr,
			      size_t mapping_size, size_t alloc_size,
			      enum dma_data_direction dir, unsigned long attrs)
{
	unsigned long flags;
	int i, count, nslots = ALIGN(alloc_size, 1 << IO_TLB_SHIFT) >> IO_TLB_SHIFT;
	int index = (tlb_addr - io_tlb_start) >> IO_TLB_SHIFT;
	phys_addr_t orig_addr = io_tlb_orig_addr[index];

	/*
	 * First, sync the memory before unmapping the entry
	 */
	if (orig_addr != INVALID_PHYS_ADDR &&
		!(attrs & DMA_ATTR_SKIP_CPU_SYNC) &&
		(dir == DMA_FROM_DEVICE || dir == DMA_BIDIRECTIONAL))
		swiotlb_bounce(orig_addr, tlb_addr, mapping_size, DMA_FROM_DEVICE);

	/*
	 * Return the buffer to the free list by setting the corresponding
	 * entries to indicate the number of contiguous entries available.
	 * While returning the entries to the free list, we merge the entries
	 * with slots below and above the pool being returned.
	 */
	spin_lock_irqsave(&io_tlb_lock, flags);
	{
		count = ((index + nslots) < ALIGN(index + 1, IO_TLB_SEGSIZE) ?
			 io_tlb_list[index + nslots] : 0);
		/*
		 * Step 1: return the slots to the free list, merging the
		 * slots with superceeding slots
		 */
		for (i = index + nslots - 1; i >= index; i--) {
			io_tlb_list[i] = ++count;
			io_tlb_orig_addr[i] = INVALID_PHYS_ADDR;
		}
		/*
		 * Step 2: merge the returned slots with the preceding slots,
		 * if available (non zero)
		 */
		for (i = index - 1; (OFFSET(i, IO_TLB_SEGSIZE) !=
			IO_TLB_SEGSIZE - 1) && io_tlb_list[i]; i--)
			io_tlb_list[i] = ++count;

		io_tlb_used -= nslots;
	}
	spin_unlock_irqrestore(&io_tlb_lock, flags);
}

static void swiotlb_tbl_sync_single(struct device *hwdev, phys_addr_t tlb_addr,
			     size_t size, enum dma_data_direction dir,
			     enum dma_sync_target target)
{
	int index = (tlb_addr - io_tlb_start) >> IO_TLB_SHIFT;
	phys_addr_t orig_addr = io_tlb_orig_addr[index];

	if (orig_addr == INVALID_PHYS_ADDR)
		return;
	orig_addr += (unsigned long)tlb_addr & ((1 << IO_TLB_SHIFT) - 1);

	switch (target) {
	case SYNC_FOR_CPU:
		if (likely(dir == DMA_FROM_DEVICE || dir == DMA_BIDIRECTIONAL))
			swiotlb_bounce(orig_addr, tlb_addr,
				       size, DMA_FROM_DEVICE);
		else
			WARN_ON(dir != DMA_TO_DEVICE);
		break;
	case SYNC_FOR_DEVICE:
		if (likely(dir == DMA_TO_DEVICE || dir == DMA_BIDIRECTIONAL))
			swiotlb_bounce(orig_addr, tlb_addr,
				       size, DMA_TO_DEVICE);
		else
			WARN_ON(dir != DMA_FROM_DEVICE);
		break;
	default:
		BUG();
	}
}

static struct device *aml_dma_dev;
/*
 * Statically reserve bounce buffer space and initialize bounce buffer data
 * structures for the software IO TLB used to implement the DMA API.
 */
static void __nocfi pcie_swiotlb_init(struct device *dma_dev)
{
	size_t default_size = AML_IO_TLB_DEFAULT_SIZE;
	unsigned char *vstart;
	unsigned long bytes;
	dma_addr_t paddr = 0;

	if (!io_tlb_nslabs) {
		io_tlb_nslabs = (default_size >> IO_TLB_SHIFT);
		io_tlb_nslabs = ALIGN(io_tlb_nslabs, IO_TLB_SEGSIZE);
	}

	bytes = io_tlb_nslabs << IO_TLB_SHIFT;

	aml_dma_dev = dma_dev;
	/* Get IO TLB memory from the low pages */
	vstart = dma_alloc_coherent(dma_dev, PAGE_ALIGN(bytes), &paddr, GFP_KERNEL);
	if (vstart && !aml_swiotlb_init_with_tbl(paddr, io_tlb_nslabs))
		return;

	pr_warn("Cannot allocate buffer");
	no_iotlb_memory = true;
}

/* ----------------- atomic pool --------------------- */

static struct gen_pool *aml_atomic_pool;

/* Size can be defined by the coherent_pool command line */
static size_t atomic_pool_size;

static int __nocfi aml_atomic_pool_expand(struct device *dev, struct gen_pool *pool,
			      size_t pool_size, gfp_t gfp)
{
	unsigned int order;
	struct page *page = NULL;
	void *addr;
	int ret = -ENOMEM;

	/* Cannot allocate larger than MAX_ORDER-1 */
	order = min(get_order(pool_size), MAX_ORDER - 1);

	page = aml_dma_alloc_from_contiguous(dev, 1 << order,
						order, false);
	if (!page)
		goto out;

	aml_arch_dma_prep_coherent(page, pool_size);

#ifdef CONFIG_DMA_DIRECT_REMAP
	addr = aml_dma_common_contiguous_remap(page, pool_size,
					   pgprot_dmacoherent(PAGE_KERNEL),
					   __builtin_return_address(0));
	if (!addr)
		goto out;
#else
	addr = page_to_virt(page);
#endif
	/*
	 * Memory in the atomic DMA pools must be unencrypted, the pools do not
	 * shrink so no re-encryption occurs in dma_direct_free().
	 */
	ret = aml_set_memory_decrypted((unsigned long)page_to_virt(page),
				   1 << order);
	if (ret)
		goto remove_mapping;
	ret = gen_pool_add_virt(pool, (unsigned long)addr, page_to_phys(page),
				pool_size, NUMA_NO_NODE);
	if (ret)
		goto encrypt_mapping;

	return 0;

encrypt_mapping:
	ret = aml_set_memory_encrypted((unsigned long)page_to_virt(page),
				   1 << order);
	if (WARN_ON_ONCE(ret)) {
		/* Decrypt succeeded but encrypt failed, purposely leak */
		goto out;
	}
remove_mapping:
#ifdef CONFIG_DMA_DIRECT_REMAP
	aml_dma_common_free_remap(addr, pool_size);
#endif
out:
	return ret;
}

static struct gen_pool __nocfi *__dma_atomic_pool_init(struct device *dev,
						size_t pool_size, gfp_t gfp)
{
	struct gen_pool *pool;
	int ret;

	pool = gen_pool_create(PAGE_SHIFT, NUMA_NO_NODE);
	if (!pool)
		return NULL;

	gen_pool_set_algo(pool, gen_pool_first_fit_order_align, NULL);

	ret = aml_atomic_pool_expand(dev, pool, pool_size, gfp);
	if (ret) {
		gen_pool_destroy(pool);
		pr_err("aml DMA: failed to allocate %zu KiB %pGg pool for atomic allocation\n",
		       pool_size >> 10, &gfp);
		return NULL;
	}

	pr_info("aml DMA: preallocated %zu KiB %pGg pool for atomic allocations\n",
		gen_pool_size(pool) >> 10, &gfp);
	return pool;
}

static int __nocfi aml_dma_atomic_pool_init(struct device *dev)
{
	int ret = 0;

	/*
	 * If coherent_pool was not used on the command line, default the pool
	 * sizes to 128KB per 1GB of memory, min 128KB, max MAX_ORDER-1.
	 */
	if (!atomic_pool_size) {
		unsigned long pages = totalram_pages() / (SZ_1G / SZ_128K);

		pages = min_t(unsigned long, pages, MAX_ORDER_NR_PAGES);
		atomic_pool_size = max_t(size_t, pages << PAGE_SHIFT, SZ_128K);
	}

	aml_atomic_pool = __dma_atomic_pool_init(dev, atomic_pool_size,
						    GFP_KERNEL);
	if (!aml_atomic_pool)
		ret = -ENOMEM;

	return ret;
}

static void *aml_dma_alloc_from_pool(size_t size, struct page **ret_page, gfp_t flags)
{
	unsigned long val;
	void *ptr = NULL;

	if (!aml_atomic_pool) {
		WARN(1, "coherent pool not initialised!\n");
		return NULL;
	}

	val = gen_pool_alloc(aml_atomic_pool, size);
	if (val) {
		phys_addr_t phys = gen_pool_virt_to_phys(aml_atomic_pool, val);

		*ret_page = pfn_to_page(__phys_to_pfn(phys));
		ptr = (void *)val;
		memset(ptr, 0, size);
	}

	return ptr;
}

static bool aml_dma_in_atomic_pool(void *start, size_t size)
{
	if (unlikely(!aml_atomic_pool))
		return false;

	return gen_pool_has_addr(aml_atomic_pool, (unsigned long)start, size);
}

static bool aml_dma_free_from_pool(void *start, size_t size)
{
	if (!aml_dma_in_atomic_pool(start, size))
		return false;
	gen_pool_free(aml_atomic_pool, (unsigned long)start, size);
	return true;
}

/* ----------------- atomic pool end --------------------- */

static void __nocfi *aml_dma_direct_alloc(struct device *dev, size_t size,
		dma_addr_t *dma_handle, gfp_t gfp, unsigned long attrs)
{
	struct device_node *of_node = dev->of_node;
	int count;
	void *ret;
	struct page *page = NULL;

	count = of_property_count_elems_of_size(of_node, "memory-region", sizeof(u32));
	if (count <= 0 && aml_dma_dev)
		dev = aml_dma_dev;

	if (!gfpflags_allow_blocking(gfp)) {
		size = PAGE_ALIGN(size);
		ret = aml_dma_alloc_from_pool(size, &page, gfp);
		if (!ret)
			return NULL;

		*dma_handle = phys_to_dma(dev, page_to_phys(page));
		return ret;
	}

	return _aml_dma_direct_alloc(dev, size, dma_handle, gfp, attrs);
}

static void __nocfi aml_dma_direct_free(struct device *dev, size_t size,
		void *cpu_addr, dma_addr_t dma_addr, unsigned long attrs)
{
	struct device_node *of_node = dev->of_node;
	int count;

	count = of_property_count_elems_of_size(of_node, "memory-region", sizeof(u32));
	if (count <= 0 && aml_dma_dev)
		dev = aml_dma_dev;

	if (!aml_dma_free_from_pool(cpu_addr, PAGE_ALIGN(size)))
		return _aml_dma_direct_free(dev, size, cpu_addr, dma_addr, attrs);
}

static dma_addr_t __nocfi aml_dma_map_page(struct device *dev, struct page *page,
		unsigned long offset, size_t size, enum dma_data_direction dir,
		unsigned long attrs)
{
	phys_addr_t phys = page_to_phys(page) + offset;
	dma_addr_t dma_addr = phys_to_dma(dev, phys);

	if (!aml_swiotlb_map(dev, &phys, &dma_addr, size, dir, attrs))
		return DMA_MAPPING_ERROR;

	if (!dev_is_dma_coherent(dev) && !(attrs & DMA_ATTR_SKIP_CPU_SYNC))
		aml_arch_sync_dma_for_device(phys, size, dir);
	return dma_addr;
}

static void __nocfi aml_dma_sync_single_for_cpu(struct device *dev,
		dma_addr_t addr, size_t size, enum dma_data_direction dir)
{
	phys_addr_t paddr = dma_to_phys(dev, addr);

	if (!dev_is_dma_coherent(dev)) {
		aml_arch_sync_dma_for_cpu(paddr, size, dir);
		arch_sync_dma_for_cpu_all();
	}

	if (unlikely(aml_is_swiotlb_buffer(dev, paddr)))
		swiotlb_tbl_sync_single(dev, paddr, size, dir, SYNC_FOR_CPU);

	if (dir == DMA_FROM_DEVICE)
		arch_dma_mark_clean(paddr, size);
}

static void __nocfi aml_dma_unmap_page(struct device *dev, dma_addr_t addr,
		size_t size, enum dma_data_direction dir, unsigned long attrs)
{
	phys_addr_t phys = dma_to_phys(dev, addr);

	if (!(attrs & DMA_ATTR_SKIP_CPU_SYNC))
		aml_dma_sync_single_for_cpu(dev, addr, size, dir);

	if (unlikely(aml_is_swiotlb_buffer(dev, phys)))
		aml_swiotlb_tbl_unmap_single(dev, phys, size, size, dir,
					 attrs | DMA_ATTR_SKIP_CPU_SYNC);
}

static void __nocfi aml_dma_sync_single_for_device(struct device *dev,
		dma_addr_t addr, size_t size, enum dma_data_direction dir)
{
	phys_addr_t paddr = dma_to_phys(dev, addr);

	if (unlikely(aml_is_swiotlb_buffer(dev, paddr)))
		swiotlb_tbl_sync_single(dev, paddr, size, dir, SYNC_FOR_DEVICE);

	if (!dev_is_dma_coherent(dev))
		aml_arch_sync_dma_for_device(paddr, size, dir);
}

static void __nocfi aml_dma_unmap_sg(struct device *dev, struct scatterlist *sgl,
		int nents, enum dma_data_direction dir, unsigned long attrs)
{
	struct scatterlist *sg;
	int i;

	for_each_sg(sgl, sg, nents, i)
		aml_dma_unmap_page(dev, sg->dma_address, sg_dma_len(sg), dir,
			     attrs);
}

static int __nocfi aml_dma_map_sg(struct device *dev, struct scatterlist *sgl, int nents,
		enum dma_data_direction dir, unsigned long attrs)
{
	int i;
	struct scatterlist *sg;

	for_each_sg(sgl, sg, nents, i) {
		sg->dma_address = aml_dma_map_page(dev, sg_page(sg),
				sg->offset, sg->length, dir, attrs);
		if (sg->dma_address == DMA_MAPPING_ERROR)
			goto out_unmap;
		sg_dma_len(sg) = sg->length;
	}

	return nents;

out_unmap:
	aml_dma_unmap_sg(dev, sgl, i, dir, attrs | DMA_ATTR_SKIP_CPU_SYNC);
	return 0;
}

static void __nocfi aml_dma_sync_sg_for_cpu(struct device *dev,
		struct scatterlist *sgl, int nents, enum dma_data_direction dir)
{
	struct scatterlist *sg;
	int i;

	for_each_sg(sgl, sg, nents, i) {
		phys_addr_t paddr = dma_to_phys(dev, sg_dma_address(sg));

		if (!dev_is_dma_coherent(dev))
			aml_arch_sync_dma_for_cpu(paddr, sg->length, dir);

		if (unlikely(aml_is_swiotlb_buffer(dev, paddr)))
			swiotlb_tbl_sync_single(dev, paddr, sg->length,
						    dir, SYNC_FOR_CPU);

		if (dir == DMA_FROM_DEVICE)
			arch_dma_mark_clean(paddr, sg->length);
	}

	if (!dev_is_dma_coherent(dev))
		arch_sync_dma_for_cpu_all();
}

static void __nocfi aml_dma_sync_sg_for_device(struct device *dev,
		struct scatterlist *sgl, int nents, enum dma_data_direction dir)
{
	struct scatterlist *sg;
	int i;

	for_each_sg(sgl, sg, nents, i) {
		phys_addr_t paddr = dma_to_phys(dev, sg_dma_address(sg));

		if (unlikely(aml_is_swiotlb_buffer(dev, paddr)))
			swiotlb_tbl_sync_single(dev, paddr, sg->length,
						       dir, SYNC_FOR_DEVICE);

		if (!dev_is_dma_coherent(dev))
			aml_arch_sync_dma_for_device(paddr, sg->length,
					dir);
	}
}

static struct page *aml_dma_common_vaddr_to_page(void *cpu_addr)
{
	if (is_vmalloc_addr(cpu_addr))
		return vmalloc_to_page(cpu_addr);
	return virt_to_page(cpu_addr);
}

/*
 * Create scatter-list for the already allocated DMA buffer.
 */
static int aml_dma_common_get_sgtable(struct device *dev, struct sg_table *sgt,
		 void *cpu_addr, dma_addr_t dma_addr, size_t size,
		 unsigned long attrs)
{
	struct page *page = aml_dma_common_vaddr_to_page(cpu_addr);
	int ret;

	ret = sg_alloc_table(sgt, 1, GFP_KERNEL);
	if (!ret)
		sg_set_page(sgt->sgl, page, PAGE_ALIGN(size), 0);
	return ret;
}

/*
 * Create userspace mapping for the DMA-coherent memory.
 */
static int __nocfi aml_dma_common_mmap(struct device *dev, struct vm_area_struct *vma,
		void *cpu_addr, dma_addr_t dma_addr, size_t size,
		unsigned long attrs)
{
#ifdef CONFIG_MMU
	unsigned long user_count = vma_pages(vma);
	unsigned long count = PAGE_ALIGN(size) >> PAGE_SHIFT;
	unsigned long off = vma->vm_pgoff;
	struct page *page = aml_dma_common_vaddr_to_page(cpu_addr);
	int ret = -ENXIO;

	vma->vm_page_prot = aml_dma_pgprot(dev, vma->vm_page_prot, attrs);

	if (aml_dma_mmap_from_dev_coherent(dev, vma, cpu_addr, size, &ret))
		return ret;

	if (off >= count || user_count > count - off)
		return -ENXIO;

	return remap_pfn_range(vma, vma->vm_start,
			page_to_pfn(page) + vma->vm_pgoff,
			user_count << PAGE_SHIFT, vma->vm_page_prot);
#else
	return -ENXIO;
#endif /* CONFIG_MMU */
}

static dma_addr_t aml_dma_direct_map_resource(struct device *dev, phys_addr_t paddr,
		size_t size, enum dma_data_direction dir, unsigned long attrs)
{
	dma_addr_t dma_addr = paddr;

	if (unlikely(!dma_capable(dev, dma_addr, size, false))) {
		dev_err_once(dev,
			     "DMA addr %pad+%zu overflow (mask %llx, bus limit %llx).\n",
			     &dma_addr, size, *dev->dma_mask, dev->bus_dma_limit);
		WARN_ON_ONCE(1);
		return DMA_MAPPING_ERROR;
	}

	return dma_addr;
}

static int aml_dma_direct_supported(struct device *dev, u64 mask)
{
	u64 min_mask = (aml_max_pfn - 1) << PAGE_SHIFT;

	/*
	 * Because 32-bit DMA masks are so common we expect every architecture
	 * to be able to satisfy them - either by not supporting more physical
	 * memory, or by providing a ZONE_DMA32.  If neither is the case, the
	 * architecture needs to use an IOMMU instead of the direct mapping.
	 */
	if (mask >= DMA_BIT_MASK(32))
		return 1;

	/*
	 * This check needs to be against the actual bit mask value, so use
	 * phys_to_dma_unencrypted() here so that the SME encryption mask isn't
	 * part of the check.
	 */
	if (IS_ENABLED(CONFIG_ZONE_DMA))
		min_mask = min_t(u64, min_mask, DMA_BIT_MASK(zone_dma_bits));
	return mask >= phys_to_dma_unencrypted(dev, min_mask);
}

static inline dma_addr_t aml_phys_to_dma_direct(struct device *dev,
		phys_addr_t phys)
{
	if (force_dma_unencrypted(dev))
		return phys_to_dma_unencrypted(dev, phys);
	return phys_to_dma(dev, phys);
}

static u64 __nocfi aml_dma_direct_get_required_mask(struct device *dev)
{
	phys_addr_t phys = (phys_addr_t)(aml_max_pfn - 1) << PAGE_SHIFT;
	u64 max_dma = aml_phys_to_dma_direct(dev, phys);

	return (1ULL << (fls64(max_dma) - 1)) * 2 - 1;
}

static const struct dma_map_ops aml_pcie_dma_ops = {
	.alloc			= aml_dma_direct_alloc,
	.free			= aml_dma_direct_free,
	.mmap			= aml_dma_common_mmap,
	.get_sgtable		= aml_dma_common_get_sgtable,
	.map_page		= aml_dma_map_page,
	.unmap_page		= aml_dma_unmap_page,
	.map_sg			= aml_dma_map_sg,
	.unmap_sg		= aml_dma_unmap_sg,
	.map_resource		= aml_dma_direct_map_resource,
	.sync_single_for_cpu	= aml_dma_sync_single_for_cpu,
	.sync_single_for_device	= aml_dma_sync_single_for_device,
	.sync_sg_for_cpu	= aml_dma_sync_sg_for_cpu,
	.sync_sg_for_device	= aml_dma_sync_sg_for_device,
	.dma_supported		= aml_dma_direct_supported,
	.get_required_mask	= aml_dma_direct_get_required_mask,
};

/*************************************************/

static inline int aml_smmu_device_acpi_probe(struct platform_device *pdev,
					     struct aml_smmu_device *smmu)
{
	return -ENODEV;
}

static int aml_smmu_device_dt_probe(struct platform_device *pdev,
				    struct aml_smmu_device *smmu)
{
	struct device *dev = &pdev->dev;
	u32 cells;
	int ret = -EINVAL;

	if (of_property_read_u32(dev->of_node, "#iommu-cells", &cells))
		dev_err(dev, "missing #iommu-cells property\n");
	else if (cells != 1)
		dev_err(dev, "invalid #iommu-cells value (%d)\n", cells);
	else
		ret = 0;

	return ret;
}

static int aml_smmu_set_bus_ops(struct iommu_ops *ops)
{
	if (platform_bus_type.iommu_ops != ops) {
		/*
		 *err = bus_set_iommu(&platform_bus_type, ops);
		 */
		if (!ops)
			return 0;

		if (platform_bus_type.iommu_ops)
			return -EBUSY;

		platform_bus_type.iommu_ops = ops;
	}

	return 0;
}

void set_dma_ops_hook(void *data, struct device *dev, u64 dma_base, u64 size)
{
	set_dma_ops(dev, &aml_pcie_dma_ops);
}

#define AML_TEE_SMC_FAST_CALL_VAL(func_num) \
	ARM_SMCCC_CALL_VAL(ARM_SMCCC_FAST_CALL, ARM_SMCCC_SMC_32, \
			ARM_SMCCC_OWNER_TRUSTED_OS, (func_num))

#define AML_TEE_SMC_PROTECT_MEM_BY_TYPE AML_TEE_SMC_FAST_CALL_VAL(0xE023)

static u32 aml_tee_protect_mem_by_type(u32 type,
		u32 start, u32 size,
		u32 *handle)
{
	struct arm_smccc_res res;

	if (!handle)
		return 0xFFFF0006;

	arm_smccc_smc(AML_TEE_SMC_PROTECT_MEM_BY_TYPE,
			type, start, size, 0, 0, 0, 0, &res);

	*handle = res.a1;

	return res.a0;
}

static void *get_symbol_addr(const char *symbol_name)
{
	struct kprobe kp = {
		.symbol_name    = symbol_name,
	};
	int ret;

	ret = register_kprobe(&kp);
	if (ret < 0) {
		pr_err("register_kprobe:%s failed, returned %d\n", symbol_name, ret);
		return NULL;
	}
	pr_debug("symbol_name:%s addr=%px\n", symbol_name, kp.addr);
	unregister_kprobe(&kp);

	return kp.addr;
}

static int __nocfi aml_smmu_symbol_init(void *data)
{
	int ret;
	resource_size_t ioaddr;
	struct aml_smmu_device *smmu;
	struct platform_device *pdev = (struct platform_device *)data;
	struct device *dev = &pdev->dev;
	struct reserved_mem *rmem = NULL;
	struct device_node *mem_node;
	u32 handle;

	smmu = devm_kzalloc(dev, sizeof(*smmu), GFP_KERNEL);
	if (!smmu) {
		dev_err(dev, "failed to allocate amlogic smmu\n");
		return -ENOMEM;
	}

	smmu->dev = dev;

	aml_dma_common_free_remap = (void (*)(void *cpu_addr,
				size_t size))get_symbol_addr("dma_common_free_remap");
	aml_set_memory_encrypted = (int (*)(unsigned long addr,
				int numpages))get_symbol_addr("set_memory_encrypted");
	aml_set_memory_decrypted = (int (*)(unsigned long addr,
				int numpages))get_symbol_addr("set_memory_decrypted");
#ifdef CONFIG_DMA_DIRECT_REMAP
	aml_dma_common_contiguous_remap = (void * (*)(struct page *page, size_t size,
		pgprot_t prot, const void *caller))get_symbol_addr("dma_common_contiguous_remap");
#endif
	aml_arch_dma_prep_coherent = (void (*)(struct page *page,
				size_t size))get_symbol_addr("arch_dma_prep_coherent");
	aml_dma_alloc_from_contiguous = (struct page * (*)(struct device *dev, size_t count,
		unsigned int align, bool no_warn))get_symbol_addr("dma_alloc_from_contiguous");
	aml_arch_sync_dma_for_device = (void (*)(phys_addr_t paddr, size_t size,
		enum dma_data_direction dir))get_symbol_addr("arch_sync_dma_for_device");
	aml_arch_sync_dma_for_cpu = (void (*)(phys_addr_t paddr, size_t size,
		enum dma_data_direction dir))get_symbol_addr("arch_sync_dma_for_cpu");
	_aml_dma_direct_free = (void (*)(struct device *dev, size_t size, void *cpu_addr,
		dma_addr_t dma_addr, unsigned long attrs))get_symbol_addr("dma_direct_free");
	_aml_dma_direct_alloc = (void *(*)(struct device *dev, size_t size, dma_addr_t *dma_handle,
		gfp_t gfp, unsigned long attrs))get_symbol_addr("dma_direct_alloc");
	aml_dma_mmap_from_dev_coherent = (int (*)(struct device *dev, struct vm_area_struct *vma,
		void *vaddr, size_t size, int *ret))get_symbol_addr("dma_mmap_from_dev_coherent");
#ifdef CONFIG_MMU
	aml_dma_pgprot = (pgprot_t (*)(struct device *dev, pgprot_t prot,
				unsigned long attrs))get_symbol_addr("dma_pgprot");
#endif
	ret = register_kprobe(&kp_lookup_name);
	if (ret < 0) {
		pr_err("register_kprobe failed, returned %d\n", ret);
		return ret;
	}
	pr_debug("kprobe lookup offset at %px\n", kp_lookup_name.addr);

	aml_kallsyms_lookup_name = (unsigned long (*)(const char *name))kp_lookup_name.addr;

	aml_max_pfn = *(unsigned long *)aml_kallsyms_lookup_name("max_pfn");

	/* Record our private device structure */
	platform_set_drvdata(pdev, smmu);

	ret = iommu_device_sysfs_add(&smmu->iommu, dev, NULL,
				     "smmu3.%pa", &ioaddr);
	if (ret)
		return ret;

	ret = iommu_device_register(&smmu->iommu, &aml_smmu_ops, dev);
	if (ret) {
		dev_err(dev, "Failed to register iommu\n");
		return ret;
	}

	ret = of_reserved_mem_device_init(dev);
	if (ret) {
		dev_err(dev, "reserve memory init fail:%d\n", ret);
		return ret;
	}

	mem_node = of_parse_phandle(dev->of_node, "memory-region", 0);
	if (!mem_node) {
		dev_err(dev, "parse memory region failed.\n");
		return -1;
	}
	rmem = of_reserved_mem_lookup(mem_node);
	of_node_put(mem_node);
	if (rmem) {
		dev_info(dev, "tee protect memory: %lu MiB at 0x%lx\n",
			(unsigned long)rmem->size / SZ_1M, (unsigned long)rmem->base);
		ret = aml_tee_protect_mem_by_type(TEE_MEM_TYPE_PCIE,
				rmem->base, rmem->size, &handle);
		if (ret) {
			dev_err(dev, "pcie tee mem protect fail: 0x%x\n", ret);
			return -1;
		}
	} else {
		dev_err(dev, "Can't get reserve memory region\n");
		return -1;
	}

	pcie_swiotlb_init(dev);
	aml_dma_atomic_pool_init(dev);

#ifdef CONFIG_ANDROID_VENDOR_HOOKS
	register_trace_android_rvh_iommu_setup_dma_ops(set_dma_ops_hook, NULL);
#endif

	aml_global_group = kzalloc(sizeof(*aml_global_group), GFP_KERNEL);
	if (!aml_global_group)
		return -1;

	return aml_smmu_set_bus_ops(&aml_smmu_ops);
}

static int __nocfi aml_smmu_device_probe(struct platform_device *pdev)
{
	int ret;
	struct device *dev = &pdev->dev;

	if (dev->of_node) {
		ret = aml_smmu_device_dt_probe(pdev, NULL);
		if (ret == -EINVAL)
			return ret;
	} else {
		ret = aml_smmu_device_acpi_probe(pdev, NULL);
		if (ret == -ENODEV)
			return ret;
	}

	kthread_run(aml_smmu_symbol_init, (void *)pdev, "AML_CMA_TASK");

	return 0;
}

static int aml_smmu_device_remove(struct platform_device *pdev)
{
	struct aml_smmu_device *smmu = platform_get_drvdata(pdev);

	aml_smmu_set_bus_ops(NULL);
	iommu_device_unregister(&smmu->iommu);
	iommu_device_sysfs_remove(&smmu->iommu);

	return 0;
}

static void aml_smmu_device_shutdown(struct platform_device *pdev)
{
	aml_smmu_device_remove(pdev);
}

static const struct of_device_id aml_smmu_of_match[] = {
	{ .compatible = "amlogic,smmu", },
	{ },
};
MODULE_DEVICE_TABLE(of, aml_smmu_of_match);

static struct platform_driver aml_smmu_driver = {
	.driver	= {
		.name			= "aml_smmu",
		.of_match_table		= of_match_ptr(aml_smmu_of_match),
		.suppress_bind_attrs	= true,
	},
	.probe	= aml_smmu_device_probe,
	.remove	= aml_smmu_device_remove,
	.shutdown = aml_smmu_device_shutdown,
};

//module_platform_driver(aml_smmu_driver);
static int __init aml_smmu_init(void)
{
	return platform_driver_register(&aml_smmu_driver);
}
core_initcall(aml_smmu_init);

MODULE_LICENSE("GPL v2");
