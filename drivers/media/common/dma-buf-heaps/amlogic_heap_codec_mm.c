// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>
#include <linux/dma-heap.h>
#include <linux/err.h>
#include <linux/highmem.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/scatterlist.h>
#include <linux/sched/signal.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/amlogic/media/codec_mm/codec_mm.h>
#include <linux/amlogic/media/codec_mm/dmabuf_manage.h>
#include <linux/amlogic/media/dmabuf_heaps/amlogic_dmabuf_heap.h>

#define DMA_BUF_CODEC_MM "CODEC_MM_DMA_BUF"

struct codec_mm_heap_buffer {
	struct dma_heap *heap;
	struct list_head attachments;
	//lock for buffer access
	struct mutex lock;
	unsigned long len;
	struct sg_table sg_table;
	int vmap_cnt;
	void *vaddr;
	//struct deferred_freelist_item deferred_free;
	bool uncached;
	unsigned long heap_flags;
	void *priv;
};

struct dma_heap_attachment {
	struct device *dev;
	struct sg_table *table;
	struct list_head list;
	bool mapped;
	bool uncached;
};

static struct sg_table *dup_sg_table(struct sg_table *table)
{
	struct sg_table *new_table;
	int ret, i;
	struct scatterlist *sg, *new_sg;

	new_table = kzalloc(sizeof(*new_table), GFP_KERNEL);
	if (!new_table)
		return ERR_PTR(-ENOMEM);

	ret = sg_alloc_table(new_table, table->orig_nents, GFP_KERNEL);
	if (ret) {
		kfree(new_table);
		return ERR_PTR(-ENOMEM);
	}

	new_sg = new_table->sgl;
	for_each_sgtable_sg(table, sg, i) {
		sg_set_page(new_sg, sg_page(sg), sg->length, sg->offset);
		new_sg = sg_next(new_sg);
	}

	return new_table;
}

static int codec_mm_heap_attach(struct dma_buf *dmabuf,
			      struct dma_buf_attachment *attachment)
{
	struct codec_mm_heap_buffer *buffer = dmabuf->priv;
	struct dma_heap_attachment *a;
	struct sg_table *table;

	a = kzalloc(sizeof(*a), GFP_KERNEL);
	if (!a)
		return -ENOMEM;

	table = dup_sg_table(&buffer->sg_table);
	if (IS_ERR(table)) {
		kfree(a);
		return -ENOMEM;
	}

	a->table = table;
	a->dev = attachment->dev;
	INIT_LIST_HEAD(&a->list);
	a->mapped = false;
	a->uncached = buffer->uncached;
	attachment->priv = a;

	mutex_lock(&buffer->lock);
	list_add(&a->list, &buffer->attachments);
	mutex_unlock(&buffer->lock);

	return 0;
}

static void codec_mm_heap_detach(struct dma_buf *dmabuf,
			       struct dma_buf_attachment *attachment)
{
	struct codec_mm_heap_buffer *buffer = dmabuf->priv;
	struct dma_heap_attachment *a = attachment->priv;

	mutex_lock(&buffer->lock);
	list_del(&a->list);
	mutex_unlock(&buffer->lock);

	sg_free_table(a->table);
	kfree(a->table);
	kfree(a);
}

static struct sg_table *codec_mm_heap_map_dma_buf
					(struct dma_buf_attachment *attachment,
					enum dma_data_direction direction)
{
	struct dma_heap_attachment *a = attachment->priv;
	struct sg_table *table = a->table;
	int attr = 0;
	int ret;

	if (a->uncached)
		attr = DMA_ATTR_SKIP_CPU_SYNC;

	ret = dma_map_sgtable(attachment->dev, table, direction, attr);
	if (ret)
		return ERR_PTR(ret);

	a->mapped = true;
	return table;
}

static void codec_mm_heap_unmap_dma_buf(struct dma_buf_attachment *attachment,
				      struct sg_table *table,
				      enum dma_data_direction direction)
{
	struct dma_heap_attachment *a = attachment->priv;
	int attr = 0;

	if (a->uncached)
		attr = DMA_ATTR_SKIP_CPU_SYNC;
	a->mapped = false;
	dma_unmap_sgtable(attachment->dev, table, direction, attr);
}

static int codec_mm_heap_dma_buf_begin_cpu_access
						(struct dma_buf *dmabuf,
					enum dma_data_direction direction)
{
	struct codec_mm_heap_buffer *buffer = dmabuf->priv;
	struct dma_heap_attachment *a;

	mutex_lock(&buffer->lock);

	if (buffer->vmap_cnt)
		invalidate_kernel_vmap_range(buffer->vaddr, buffer->len);

	if (!buffer->uncached) {
		list_for_each_entry(a, &buffer->attachments, list) {
			if (!a->mapped)
				continue;
			dma_sync_sgtable_for_cpu(a->dev, a->table, direction);
		}
	}
	mutex_unlock(&buffer->lock);

	return 0;
}

static int codec_mm_heap_dma_buf_end_cpu_access(struct dma_buf *dmabuf,
					      enum dma_data_direction direction)
{
	struct codec_mm_heap_buffer *buffer = dmabuf->priv;
	struct dma_heap_attachment *a;

	mutex_lock(&buffer->lock);

	if (buffer->vmap_cnt)
		flush_kernel_vmap_range(buffer->vaddr, buffer->len);

	if (!buffer->uncached) {
		list_for_each_entry(a, &buffer->attachments, list) {
			if (!a->mapped)
				continue;
			dma_sync_sgtable_for_device(a->dev, a->table,
								direction);
		}
	}
	mutex_unlock(&buffer->lock);

	return 0;
}

static int codec_mm_heap_mmap(struct dma_buf *dmabuf,
						struct vm_area_struct *vma)
{
	struct codec_mm_heap_buffer *buffer = dmabuf->priv;
	struct sg_table *table = &buffer->sg_table;
	unsigned long addr = vma->vm_start;
	struct sg_page_iter piter;
	int ret;

	if (buffer->uncached)
		vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);

	for_each_sgtable_page(table, &piter, vma->vm_pgoff) {
		struct page *page = sg_page_iter_page(&piter);

		ret = remap_pfn_range(vma, addr, page_to_pfn(page), PAGE_SIZE,
				      vma->vm_page_prot);
		if (ret)
			return ret;
		addr += PAGE_SIZE;
		if (addr >= vma->vm_end)
			return 0;
	}
	return 0;
}

static void *codec_mm_heap_do_vmap(struct codec_mm_heap_buffer *buffer)
{
	struct sg_table *table = &buffer->sg_table;
	int npages = PAGE_ALIGN(buffer->len) / PAGE_SIZE;
	struct page **pages = vmalloc(sizeof(struct page *) * npages);
	struct page **tmp = pages;
	struct sg_page_iter piter;
	pgprot_t pgprot = PAGE_KERNEL;
	void *vaddr;

	if (!pages)
		return ERR_PTR(-ENOMEM);

	if (buffer->uncached)
		pgprot = pgprot_writecombine(PAGE_KERNEL);

	for_each_sgtable_page(table, &piter, 0) {
		WARN_ON(tmp - pages >= npages);
		*tmp++ = sg_page_iter_page(&piter);
	}

	vaddr = vmap(pages, npages, VM_MAP, pgprot);
	vfree(pages);

	if (!vaddr)
		return ERR_PTR(-ENOMEM);

	return vaddr;
}

static int codec_mm_heap_vmap(struct dma_buf *dmabuf, struct dma_buf_map *map)
{
	struct codec_mm_heap_buffer *buffer = dmabuf->priv;
	void *vaddr;
	int ret = 0;

	mutex_lock(&buffer->lock);
	if (buffer->vmap_cnt) {
		buffer->vmap_cnt++;
		dma_buf_map_set_vaddr(map, buffer->vaddr);
		goto out;
	}

	vaddr = codec_mm_heap_do_vmap(buffer);
	if (IS_ERR(vaddr)) {
		ret = PTR_ERR(vaddr);
		goto out;
	}

	buffer->vaddr = vaddr;
	buffer->vmap_cnt++;
	dma_buf_map_set_vaddr(map, buffer->vaddr);
out:
	mutex_unlock(&buffer->lock);

	return ret;
}

static void codec_mm_heap_vunmap(struct dma_buf *dmabuf, struct dma_buf_map *map)
{
	struct codec_mm_heap_buffer *buffer = dmabuf->priv;

	mutex_lock(&buffer->lock);
	if (!--buffer->vmap_cnt) {
		vunmap(buffer->vaddr);
		buffer->vaddr = NULL;
	}
	mutex_unlock(&buffer->lock);
	dma_buf_map_clear(map);
}

static int codec_mm_heap_zero_buffer(struct codec_mm_heap_buffer *buffer)
{
	struct sg_table *sgt = &buffer->sg_table;
	struct sg_page_iter piter;
	struct page *p;
	void *vaddr;
	int ret = 0;

	for_each_sgtable_page(sgt, &piter, 0) {
		p = sg_page_iter_page(&piter);
		vaddr = kmap_atomic(p);
		memset(vaddr, 0, PAGE_SIZE);
		kunmap_atomic(vaddr);
	}

	return ret;
}

static void codec_mm_heap_dma_buf_release(struct dma_buf *dmabuf)
{
	struct codec_mm_heap_buffer *buffer = dmabuf->priv;
	struct sg_table *table;
	phys_addr_t paddr = 0;
	unsigned long heap_flags = buffer->heap_flags;

	table = &buffer->sg_table;
	if (!(heap_flags & DMABUF_FLAG_EXTEND_PROTECTED))
		codec_mm_heap_zero_buffer(buffer);

	paddr = PFN_PHYS(page_to_pfn(sg_page(table->sgl)));
	if (codec_mm_free_for_dma(DMA_BUF_CODEC_MM, paddr))
		pr_err("codec_mm free error, please fix it");

	sg_free_table(table);
	kfree(buffer);
}

static const struct dma_buf_ops codec_mm_heap_buf_ops = {
	.attach = codec_mm_heap_attach,
	.detach = codec_mm_heap_detach,
	.map_dma_buf = codec_mm_heap_map_dma_buf,
	.unmap_dma_buf = codec_mm_heap_unmap_dma_buf,
	.begin_cpu_access = codec_mm_heap_dma_buf_begin_cpu_access,
	.end_cpu_access = codec_mm_heap_dma_buf_end_cpu_access,
	.mmap = codec_mm_heap_mmap,
	.vmap = codec_mm_heap_vmap,
	.vunmap = codec_mm_heap_vunmap,
	.release = codec_mm_heap_dma_buf_release,
};

static struct dma_buf *codec_mm_heap_do_allocate(struct dma_heap *heap,
					       unsigned long len,
					       unsigned long fd_flags,
					       unsigned long heap_flags,
					       bool uncached)
{
	struct codec_mm_heap_buffer *buffer;
	DEFINE_DMA_BUF_EXPORT_INFO(exp_info);
	struct dma_buf *dmabuf;
	struct sg_table *table;
	unsigned long paddr = 0;
	int ret = -ENOMEM;
	int memflags = CODEC_MM_FLAGS_DMA;

	if (heap_flags & DMABUF_FLAG_EXTEND_PROTECTED)
		memflags = CODEC_MM_FLAGS_TVP;

	buffer = kzalloc(sizeof(*buffer), GFP_KERNEL);
	if (!buffer)
		return ERR_PTR(-ENOMEM);

	INIT_LIST_HEAD(&buffer->attachments);
	mutex_init(&buffer->lock);
	buffer->heap = heap;
	buffer->len = len;
	buffer->uncached = uncached;
	buffer->heap_flags = heap_flags;

	table = &buffer->sg_table;
	if (sg_alloc_table(table, 1, GFP_KERNEL))
		goto free_buffer;

	paddr = codec_mm_alloc_for_dma(DMA_BUF_CODEC_MM,
					PAGE_ALIGN(len) / PAGE_SIZE,
					0,
					memflags);
	if (!paddr)
		goto free_tables;

	sg_set_page(table->sgl, pfn_to_page(PFN_DOWN(paddr)), len, 0);
	/* create the dmabuf */
	exp_info.exp_name = dma_heap_get_name(heap);
	exp_info.ops = &codec_mm_heap_buf_ops;
	exp_info.size = buffer->len;
	exp_info.flags = fd_flags;
	exp_info.priv = buffer;
	dmabuf = dma_buf_export(&exp_info);
	if (IS_ERR(dmabuf)) {
		ret = PTR_ERR(dmabuf);
		goto free_tables;
	}
	/*
	 * For uncached buffers, we need to initially flush cpu cache, since
	 * the __GFP_ZERO on the allocation means the zeroing was done by the
	 * cpu and thus it is likely cached. Map (and implicitly flush) and
	 * unmap it now so we don't get corruption later on.
	 */
	if (buffer->uncached && (!(heap_flags & DMABUF_FLAG_EXTEND_PROTECTED))) {
		dma_map_sgtable(dma_heap_get_dev(heap), table,
						DMA_BIDIRECTIONAL, 0);
		dma_unmap_sgtable(dma_heap_get_dev(heap), table,
						DMA_BIDIRECTIONAL, 0);
	}

	return dmabuf;
free_tables:
	if (paddr)
		codec_mm_free_for_dma(DMA_BUF_CODEC_MM, paddr);
	sg_free_table(table);
free_buffer:
	kfree(buffer);
	pr_err("Allocate dmabuf %lx %lx %lx failed", len, fd_flags, heap_flags);
	return ERR_PTR(ret);
}

static struct dma_buf *codec_mm_heap_allocate
						(struct dma_heap *heap,
						unsigned long len,
						unsigned long fd_flags,
						unsigned long heap_flags)
{
	if (!strcmp(dma_heap_get_name(heap), CODECMM_SECURE_HEAP_NAME))
		heap_flags |= DMABUF_FLAG_EXTEND_PROTECTED;
	else if (!strcmp(dma_heap_get_name(heap), CODECMM_CACHED_HEAP_NAME))
		heap_flags |= DMABUF_FLAG_EXTEND_CACHED;

	return codec_mm_heap_do_allocate(heap, len, fd_flags, heap_flags,
		!(heap_flags & DMABUF_FLAG_EXTEND_CACHED));
}

/* Dummy function to be used until we can call coerce_mask_and_coherent */
static struct dma_buf *codec_mm_heap_not_initialized
						(struct dma_heap *heap,
						unsigned long len,
						unsigned long fd_flags,
						unsigned long heap_flags)
{
	return ERR_PTR(-EBUSY);
}

static struct dma_heap_ops codec_mm_heap_ops = {
	.allocate = codec_mm_heap_not_initialized,
};

int __init amlogic_codec_mm_dma_buf_init(void)
{
	struct dma_heap_export_info exp_info;
	struct dma_heap *codec_mm_heap;

	exp_info.name = CODECMM_HEAP_NAME;
	exp_info.ops = &codec_mm_heap_ops;
	exp_info.priv = NULL;
	codec_mm_heap = dma_heap_add(&exp_info);
	if (IS_ERR(codec_mm_heap))
		return PTR_ERR(codec_mm_heap);
	dma_coerce_mask_and_coherent(dma_heap_get_dev(codec_mm_heap),
		DMA_BIT_MASK(64));
	mb(); /* make sure we only set allocate after dma_mask is set */

	exp_info.name = CODECMM_SECURE_HEAP_NAME;
	exp_info.ops = &codec_mm_heap_ops;
	exp_info.priv = NULL;
	codec_mm_heap = dma_heap_add(&exp_info);
	if (IS_ERR(codec_mm_heap))
		return PTR_ERR(codec_mm_heap);
	dma_coerce_mask_and_coherent(dma_heap_get_dev(codec_mm_heap),
		DMA_BIT_MASK(64));
	mb(); /* make sure we only set allocate after dma_mask is set */

	exp_info.name = CODECMM_CACHED_HEAP_NAME;
	exp_info.ops = &codec_mm_heap_ops;
	exp_info.priv = NULL;
	codec_mm_heap = dma_heap_add(&exp_info);
	if (IS_ERR(codec_mm_heap))
		return PTR_ERR(codec_mm_heap);
	dma_coerce_mask_and_coherent(dma_heap_get_dev(codec_mm_heap),
		DMA_BIT_MASK(64));
	mb(); /* make sure we only set allocate after dma_mask is set */

	codec_mm_heap_ops.allocate = codec_mm_heap_allocate;

	pr_info("codecmm dmaheap:enter %s\n", __func__);
	return 0;
}

MODULE_LICENSE("GPL v2");
