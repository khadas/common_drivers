// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>
#include <linux/dma-heap.h>
#include <linux/err.h>
#include <linux/genalloc.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_reserved_mem.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/amlogic/tee.h>

struct meson_secure_heap {
	struct gen_pool *pool;
	phys_addr_t base;
};

struct meson_secure_heap *meson_secure_heap;

struct secure_heap_buffer {
	struct dma_heap *heap;
	struct list_head attachments;
	struct mutex lock;//protect list operation
	unsigned long len;
	struct sg_table sg_table;
	int vmap_cnt;
	void *vaddr;
	bool uncached;
	struct dma_buf *buf;
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

static int meson_secure_heap_attach(struct dma_buf *dmabuf,
				struct dma_buf_attachment *attachment)
{
	struct secure_heap_buffer *buffer = dmabuf->priv;
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

static void meson_secure_heap_detach(struct dma_buf *dmabuf,
				struct dma_buf_attachment *attachment)
{
	struct secure_heap_buffer *buffer = dmabuf->priv;
	struct dma_heap_attachment *a = attachment->priv;

	mutex_lock(&buffer->lock);
	list_del(&a->list);
	mutex_unlock(&buffer->lock);

	sg_free_table(a->table);
	kfree(a->table);
	kfree(a);
}

static struct sg_table *meson_secure_heap_map_dma_buf
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

static void meson_secure_heap_unmap_dma_buf(struct dma_buf_attachment *attachment,
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

static int meson_secure_heap_dma_buf_begin_cpu_access
						(struct dma_buf *dmabuf,
					enum dma_data_direction direction)
{
	return 0;
}

static int meson_secure_heap_dma_buf_end_cpu_access(struct dma_buf *dmabuf,
						enum dma_data_direction direction)
{
	return 0;
}

static int meson_secure_heap_mmap(struct dma_buf *dmabuf,
						struct vm_area_struct *vma)
{
	return 0;
}

static int meson_secure_heap_vmap(struct dma_buf *dmabuf, struct dma_buf_map *vaddr)
{
	return 0;
}

static void meson_secure_heap_vunmap(struct dma_buf *dmabuf, struct dma_buf_map *vaddr)
{}

static void meson_secure_heap_dma_buf_release(struct dma_buf *dmabuf)
{
	struct secure_heap_buffer *buffer = dmabuf->priv;
	struct sg_table *table = &buffer->sg_table;
	phys_addr_t paddr = PFN_PHYS(page_to_pfn(sg_page(table->sgl)));

	if (!buffer)
		return;
	gen_pool_free(meson_secure_heap->pool, paddr, buffer->len);
	sg_free_table(table);

	kfree(buffer);
}

static const struct dma_buf_ops meson_secure_heap_buf_ops = {
	.attach = meson_secure_heap_attach,
	.detach = meson_secure_heap_detach,
	.map_dma_buf = meson_secure_heap_map_dma_buf,
	.unmap_dma_buf = meson_secure_heap_unmap_dma_buf,
	.begin_cpu_access = meson_secure_heap_dma_buf_begin_cpu_access,
	.end_cpu_access = meson_secure_heap_dma_buf_end_cpu_access,
	.mmap = meson_secure_heap_mmap,
	.vmap = meson_secure_heap_vmap,
	.vunmap = meson_secure_heap_vunmap,
	.release = meson_secure_heap_dma_buf_release,
};

static struct dma_buf *meson_secure_heap_allocate(struct dma_heap *heap,
						unsigned long len,
						unsigned long fd_flags,
						unsigned long heap_flags,
						bool uncached)
{
	struct secure_heap_buffer *buffer;
	DEFINE_DMA_BUF_EXPORT_INFO(exp_info);
	struct dma_buf *dmabuf;
	struct sg_table *table;
	unsigned long paddr = 0;
	int ret = -ENOMEM;

	buffer = kzalloc(sizeof(*buffer), GFP_KERNEL);
	if (!buffer)
		return ERR_PTR(-ENOMEM);

	INIT_LIST_HEAD(&buffer->attachments);
	mutex_init(&buffer->lock);
	buffer->heap = heap;
	buffer->len = len;
	buffer->uncached = uncached;

	table = &buffer->sg_table;
	if (sg_alloc_table(table, 1, GFP_KERNEL))
		goto free_buffer;

	paddr = gen_pool_alloc(meson_secure_heap->pool, len);
	if (!paddr)
		goto free_tables;
	sg_set_page(table->sgl, pfn_to_page(PFN_DOWN(paddr)), len, 0);

	/* create the dmabuf */
	exp_info.exp_name = dma_heap_get_name(heap);
	exp_info.ops = &meson_secure_heap_buf_ops;
	exp_info.size = buffer->len;
	exp_info.flags = fd_flags;
	exp_info.priv = buffer;
	dmabuf = dma_buf_export(&exp_info);
	if (IS_ERR(dmabuf)) {
		ret = PTR_ERR(dmabuf);

		goto free_tables;
	}
	buffer->buf = dmabuf;

	return dmabuf;

free_tables:
	sg_free_table(table);
free_buffer:
	kfree(buffer);
	return ERR_PTR(ret);
}

static struct dma_buf *meson_secure_uncached_heap_allocate
						(struct dma_heap *heap,
						unsigned long len,
						unsigned long fd_flags,
						unsigned long heap_flags)
{
	return meson_secure_heap_allocate(heap, len, fd_flags, heap_flags, true);
}

static struct dma_buf *meson_secure_uncached_heap_not_initialized
						(struct dma_heap *heap,
						unsigned long len,
						unsigned long fd_flags,
						unsigned long heap_flags)
{
	return ERR_PTR(-EBUSY);
}

static struct dma_heap_ops meson_secure_heap_ops = {
	.allocate = meson_secure_uncached_heap_not_initialized,
};

int __init amlogic_heap_secure_dma_buf_init(void)
{
	struct dma_heap_export_info exp_info;
	struct device_node *target = NULL;
	struct reserved_mem *mem = NULL;
	struct dma_heap *secure_heap;
	unsigned int secure_heap_handle;
	int err;

	target = of_find_compatible_node(target, NULL, "amlogic, heap-secure-mem");
	if (target) {
		mem = of_reserved_mem_lookup(target);
	} else {
		pr_err("%s: heap-secure not added in dts.\n", __func__);
		return -ENOMEM;
	}

	meson_secure_heap = kzalloc(sizeof(*meson_secure_heap), GFP_KERNEL);
	if (!meson_secure_heap)
		return -ENOMEM;
	meson_secure_heap->pool = gen_pool_create(PAGE_SHIFT, -1);
	if (!meson_secure_heap->pool) {
		kfree(meson_secure_heap);
		return -ENOMEM;
	}
	meson_secure_heap->base = mem->base;
	gen_pool_add(meson_secure_heap->pool, meson_secure_heap->base, mem->size, -1);

	exp_info.name = target->name;
	exp_info.ops = &meson_secure_heap_ops;
	exp_info.priv = NULL;

	secure_heap = dma_heap_add(&exp_info);
	if (IS_ERR(secure_heap)) {
		int ret = PTR_ERR(secure_heap);

		kfree(secure_heap);
		return ret;
	}
	err = tee_protect_mem_by_type(TEE_MEM_TYPE_GPU,
				      (u32)mem->base,
				      (u32)mem->size,
				      &secure_heap_handle);
	if (err)
		pr_err("%s: tee protect gpu mem fail!\n", __func__);
	else
		pr_info("tee protect gpu mem done.\n");

	dma_coerce_mask_and_coherent(dma_heap_get_dev(secure_heap),
		DMA_BIT_MASK(64));
	mb(); /* make sure we only set allocate after dma_mask is set */
	meson_secure_heap_ops.allocate = meson_secure_uncached_heap_allocate;

	pr_info("dmaheap: find %s\n", exp_info.name);
	pr_info("%s: success add %s, size=%pa, paddr=%pa\n",
		 __func__, exp_info.name, &mem->size, &mem->base);
	return 0;
}

MODULE_LICENSE("GPL v2");
