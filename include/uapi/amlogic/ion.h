/* SPDX-License-Identifier: LGPL-2.1+ WITH Linux-syscall-note */
/*
 * Copyright (C) 2020 Amlogic, Inc. All rights reserved.
 *
 */

#ifndef _UAPI_LINUX_ION_H
#define _UAPI_LINUX_ION_H

#include <linux/ioctl.h>
#include <linux/types.h>

/**
 * enum ion_heap_types - list of all possible types of heaps
 * @ION_HEAP_TYPE_SYSTEM:	 memory allocated via vmalloc
 * @ION_HEAP_TYPE_SYSTEM_CONTIG: memory allocated via kmalloc
 * @ION_HEAP_TYPE_CARVEOUT:	 memory allocated from a prereserved
 *				 carveout heap, allocations are physically
 *				 contiguous
 * @ION_HEAP_TYPE_DMA:		 memory allocated via DMA API
 * @ION_NUM_HEAPS:		 helper for iterating over heaps, a bit mask
 *				 is used to identify the heaps, so only 32
 *				 total heap types are supported
 */
enum ion_heap_type {
	ION_HEAP_TYPE_SYSTEM = 0,
	ION_HEAP_TYPE_SYSTEM_CONTIG = 1,
	ION_HEAP_TYPE_CARVEOUT = 2,
	ION_HEAP_TYPE_CHUNK = 3,
	ION_HEAP_TYPE_DMA = 4,
	/* reserved range for future standard heap types */
	ION_HEAP_TYPE_CUSTOM = 16,
	ION_HEAP_TYPE_MAX = 31,
};

enum ion_heap_id {
	ION_HEAP_SYSTEM = (1 << ION_HEAP_TYPE_SYSTEM),
	ION_HEAP_SYSTEM_CONTIG = (ION_HEAP_SYSTEM << 1),
	ION_HEAP_CARVEOUT_START = (ION_HEAP_SYSTEM_CONTIG << 1),
	ION_HEAP_CARVEOUT_END = (ION_HEAP_CARVEOUT_START << 4),
	ION_HEAP_CHUNK = (ION_HEAP_CARVEOUT_END << 1),
	ION_HEAP_DMA_START = (ION_HEAP_CHUNK << 1),
	ION_HEAP_DMA_END = (ION_HEAP_DMA_START << 7),
	ION_HEAP_CUSTOM_START = (ION_HEAP_DMA_END << 1),
	ION_HEAP_CUSTOM_END = (ION_HEAP_CUSTOM_START << 15),
};

#define ION_NUM_MAX_HEAPS	(32)
#define ION_NUM_HEAP_IDS		(sizeof(unsigned int) * 8)

/**
 * allocation flags - the lower 16 bits are used by core ion, the upper 16
 * bits are reserved for use by the heaps themselves.
 */

/*
 * mappings of this buffer should be cached, ion will do cache maintenance
 * when the buffer is mapped for dma
 */
#define ION_FLAG_CACHED 1

/**
 * DOC: Ion Userspace API
 *
 * create a client by opening /dev/ion
 * most operations handled via following ioctls
 *
 */

/**
 * struct ion_allocation_data - metadata passed from userspace for allocations
 * @len:		size of the allocation
 * @heap_id_mask:	mask of heap ids to allocate from
 * @flags:		flags passed to heap
 * @handle:		pointer that will be populated with a cookie to use to
 *			refer to this allocation
 *
 * Provided by userspace as an argument to the ioctl
 */
struct ion_allocation_data {
	__u64 len;
	__u32 heap_id_mask;
	__u32 flags;
	__u32 fd;
	__u32 unused;
};

#define MAX_HEAP_NAME			32

/**
 * struct ion_heap_data - data about a heap
 * @name - first 32 characters of the heap name
 * @type - heap type
 * @heap_id - heap id for the heap
 */
struct ion_heap_data {
	char name[MAX_HEAP_NAME];
	__u32 type;
	__u32 heap_id;
	__u32 reserved0;
	__u32 reserved1;
	__u32 reserved2;
};

/**
 * struct ion_heap_query - collection of data about all heaps
 * @cnt - total number of heaps to be copied
 * @heaps - buffer to copy heap data
 */
struct ion_heap_query {
	__u32 cnt; /* Total number of heaps to be copied */
	__u32 reserved0; /* align to 64bits */
	__u64 heaps; /* buffer to be populated */
	__u32 reserved1;
	__u32 reserved2;
};

#define ION_IOC_MAGIC		'I'

/**
 * DOC: ION_IOC_ALLOC - allocate memory
 *
 * Takes an ion_allocation_data struct and returns it with the handle field
 * populated with the opaque handle for the allocation.
 */
#define ION_IOC_ALLOC		_IOWR(ION_IOC_MAGIC, 0, \
				      struct ion_allocation_data)

/**
 * DOC: ION_IOC_HEAP_QUERY - information about available heaps
 *
 * Takes an ion_heap_query structure and populates information about
 * available Ion heaps.
 */
#define ION_IOC_HEAP_QUERY     _IOWR(ION_IOC_MAGIC, 8, \
					struct ion_heap_query)

#define ION_IOC_ABI_VERSION    _IOR(ION_IOC_MAGIC, 9, \
							__u32)
#endif /* _UAPI_LINUX_ION_H */
