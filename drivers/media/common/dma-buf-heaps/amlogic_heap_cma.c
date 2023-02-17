// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/cma.h>
#include <linux/dma-buf.h>
#include <linux/dma-heap.h>
#include <linux/dma-map-ops.h>
#include <linux/err.h>
#include <linux/highmem.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

struct meson_cma_heap {
	struct dma_heap *heap;
	struct cma *cma;
};

struct meson_cma_heap_buffer {
	struct meson_cma_heap *heap;
	struct list_head attachments;
	struct mutex lock;//protect list operation
	unsigned long len;
	struct page *cma_pages;
	struct page **pages;
	pgoff_t pagecount;
	int vmap_cnt;
	void *vaddr;
};

struct meson_dma_heap_attachment {
	struct device *dev;
	struct sg_table table;
	struct list_head list;
	bool mapped;
};

static int meson_cma_heap_attach(struct dma_buf *dmabuf,
			   struct dma_buf_attachment *attachment)
{
	struct meson_cma_heap_buffer *buffer = dmabuf->priv;
	struct meson_dma_heap_attachment *a;
	int ret;

	a = kzalloc(sizeof(*a), GFP_KERNEL);
	if (!a)
		return -ENOMEM;

	ret = sg_alloc_table_from_pages(&a->table, buffer->pages,
					buffer->pagecount, 0,
					buffer->pagecount << PAGE_SHIFT,
					GFP_KERNEL);
	if (ret) {
		kfree(a);
		return ret;
	}

	a->dev = attachment->dev;
	INIT_LIST_HEAD(&a->list);
	a->mapped = false;

	attachment->priv = a;

	mutex_lock(&buffer->lock);
	list_add(&a->list, &buffer->attachments);
	mutex_unlock(&buffer->lock);

	return 0;
}

static void meson_cma_heap_detach(struct dma_buf *dmabuf,
			    struct dma_buf_attachment *attachment)
{
	struct meson_cma_heap_buffer *buffer = dmabuf->priv;
	struct meson_dma_heap_attachment *a = attachment->priv;

	mutex_lock(&buffer->lock);
	list_del(&a->list);
	mutex_unlock(&buffer->lock);

	sg_free_table(&a->table);
	kfree(a);
}

static struct sg_table *meson_cma_heap_map_dma_buf(struct dma_buf_attachment *attachment,
					     enum dma_data_direction direction)
{
	struct meson_dma_heap_attachment *a = attachment->priv;
	struct sg_table *table = &a->table;
	int attrs = attachment->dma_map_attrs;
	int ret;

	ret = dma_map_sgtable(attachment->dev, table, direction, attrs);
	if (ret)
		return ERR_PTR(-ENOMEM);
	a->mapped = true;
	return table;
}

static void meson_cma_heap_unmap_dma_buf(struct dma_buf_attachment *attachment,
				   struct sg_table *table,
				   enum dma_data_direction direction)
{
	struct meson_dma_heap_attachment *a = attachment->priv;
	int attrs = attachment->dma_map_attrs;

	a->mapped = false;
	dma_unmap_sgtable(attachment->dev, table, direction, attrs);
}

static int meson_cma_heap_dma_buf_begin_cpu_access(struct dma_buf *dmabuf,
					     enum dma_data_direction direction)
{
	struct meson_cma_heap_buffer *buffer = dmabuf->priv;
	struct meson_dma_heap_attachment *a;

	mutex_lock(&buffer->lock);

	if (buffer->vmap_cnt)
		invalidate_kernel_vmap_range(buffer->vaddr, buffer->len);

	list_for_each_entry(a, &buffer->attachments, list) {
		if (!a->mapped)
			continue;
		dma_sync_sgtable_for_cpu(a->dev, &a->table, direction);
	}
	mutex_unlock(&buffer->lock);

	return 0;
}

static int meson_cma_heap_dma_buf_end_cpu_access(struct dma_buf *dmabuf,
					   enum dma_data_direction direction)
{
	struct meson_cma_heap_buffer *buffer = dmabuf->priv;
	struct meson_dma_heap_attachment *a;

	mutex_lock(&buffer->lock);

	if (buffer->vmap_cnt)
		flush_kernel_vmap_range(buffer->vaddr, buffer->len);

	list_for_each_entry(a, &buffer->attachments, list) {
		if (!a->mapped)
			continue;
		dma_sync_sgtable_for_device(a->dev, &a->table, direction);
	}
	mutex_unlock(&buffer->lock);

	return 0;
}

static vm_fault_t meson_cma_heap_vm_fault(struct vm_fault *vmf)
{
	struct vm_area_struct *vma = vmf->vma;
	struct meson_cma_heap_buffer *buffer = vma->vm_private_data;

	if (vmf->pgoff > buffer->pagecount)
		return VM_FAULT_SIGBUS;

	vmf->page = buffer->pages[vmf->pgoff];
	get_page(vmf->page);

	return 0;
}

static const struct vm_operations_struct dma_heap_vm_ops = {
	.fault = meson_cma_heap_vm_fault,
};

static int meson_cma_heap_mmap(struct dma_buf *dmabuf, struct vm_area_struct *vma)
{
	struct meson_cma_heap_buffer *buffer = dmabuf->priv;

	if ((vma->vm_flags & (VM_SHARED | VM_MAYSHARE)) == 0)
		return -EINVAL;

	vma->vm_ops = &dma_heap_vm_ops;
	vma->vm_private_data = buffer;

	return 0;
}

static void *meson_cma_heap_do_vmap(struct meson_cma_heap_buffer *buffer)
{
	void *vaddr;

	vaddr = vmap(buffer->pages, buffer->pagecount, VM_MAP, PAGE_KERNEL);
	if (!vaddr)
		return ERR_PTR(-ENOMEM);

	return vaddr;
}

static int meson_cma_heap_vmap(struct dma_buf *dmabuf, struct dma_buf_map *map)
{
	struct meson_cma_heap_buffer *buffer = dmabuf->priv;
	void *vaddr;
	int ret = 0;

	mutex_lock(&buffer->lock);
	if (buffer->vmap_cnt) {
		buffer->vmap_cnt++;
		dma_buf_map_set_vaddr(map, buffer->vaddr);
		goto out;
	}

	vaddr = meson_cma_heap_do_vmap(buffer);
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

static void meson_cma_heap_vunmap(struct dma_buf *dmabuf, struct dma_buf_map *map)
{
	struct meson_cma_heap_buffer *buffer = dmabuf->priv;

	mutex_lock(&buffer->lock);
	if (!--buffer->vmap_cnt) {
		vunmap(buffer->vaddr);
		buffer->vaddr = NULL;
	}
	mutex_unlock(&buffer->lock);
	dma_buf_map_clear(map);
}

static void meson_cma_heap_dma_buf_release(struct dma_buf *dmabuf)
{
	struct meson_cma_heap_buffer *buffer = dmabuf->priv;
	struct meson_cma_heap *meson_cma_heap = buffer->heap;

	if (buffer->vmap_cnt > 0) {
		WARN(1, "%s: buffer still mapped in the kernel\n", __func__);
		vunmap(buffer->vaddr);
		buffer->vaddr = NULL;
	}

	/* free page list */
	kfree(buffer->pages);
	/* release memory */
	cma_release(meson_cma_heap->cma, buffer->cma_pages, buffer->pagecount);
	kfree(buffer);
}

static const struct dma_buf_ops meson_cma_heap_buf_ops = {
	.attach = meson_cma_heap_attach,
	.detach = meson_cma_heap_detach,
	.map_dma_buf = meson_cma_heap_map_dma_buf,
	.unmap_dma_buf = meson_cma_heap_unmap_dma_buf,
	.begin_cpu_access = meson_cma_heap_dma_buf_begin_cpu_access,
	.end_cpu_access = meson_cma_heap_dma_buf_end_cpu_access,
	.mmap = meson_cma_heap_mmap,
	.vmap = meson_cma_heap_vmap,
	.vunmap = meson_cma_heap_vunmap,
	.release = meson_cma_heap_dma_buf_release,
};

static struct dma_buf *meson_cma_heap_allocate(struct dma_heap *heap,
					 unsigned long len,
					 unsigned long fd_flags,
					 unsigned long heap_flags)
{
	struct meson_cma_heap *meson_cma_heap = dma_heap_get_drvdata(heap);
	struct meson_cma_heap_buffer *buffer;
	DEFINE_DMA_BUF_EXPORT_INFO(exp_info);
	size_t size = PAGE_ALIGN(len);
	pgoff_t pagecount = size >> PAGE_SHIFT;
	unsigned long align = get_order(size);
	struct page *cma_pages;
	struct dma_buf *dmabuf;
	int ret = -ENOMEM;
	pgoff_t pg;

	buffer = kzalloc(sizeof(*buffer), GFP_KERNEL);
	if (!buffer)
		return ERR_PTR(-ENOMEM);

	INIT_LIST_HEAD(&buffer->attachments);
	mutex_init(&buffer->lock);
	buffer->len = size;

	if (align > CONFIG_CMA_ALIGNMENT)
		align = CONFIG_CMA_ALIGNMENT;

#if CONFIG_AMLOGIC_KERNEL_VERSION >= 14515
	cma_pages = cma_alloc(meson_cma_heap->cma, pagecount, align, GFP_KERNEL);
#else
	cma_pages = cma_alloc(meson_cma_heap->cma, pagecount, align, false);
#endif
	if (!cma_pages)
		goto free_buffer;

	/* Clear the cma pages */
	if (PageHighMem(cma_pages)) {
		unsigned long nr_clear_pages = pagecount;
		struct page *page = cma_pages;

		while (nr_clear_pages > 0) {
			void *vaddr = kmap_atomic(page);

			memset(vaddr, 0, PAGE_SIZE);
			kunmap_atomic(vaddr);
			/*
			 * Avoid wasting time zeroing memory if the process
			 * has been killed by SIGKILL
			 */
			if (fatal_signal_pending(current))
				goto free_cma;
			page++;
			nr_clear_pages--;
		}
	} else {
		memset(page_address(cma_pages), 0, size);
	}

	buffer->pages = kmalloc_array(pagecount, sizeof(*buffer->pages), GFP_KERNEL);
	if (!buffer->pages) {
		ret = -ENOMEM;
		goto free_cma;
	}

	for (pg = 0; pg < pagecount; pg++)
		buffer->pages[pg] = &cma_pages[pg];

	buffer->cma_pages = cma_pages;
	buffer->heap = meson_cma_heap;
	buffer->pagecount = pagecount;

	/* create the dmabuf */
	exp_info.exp_name = dma_heap_get_name(heap);
	exp_info.ops = &meson_cma_heap_buf_ops;
	exp_info.size = buffer->len;
	exp_info.flags = fd_flags;
	exp_info.priv = buffer;
	dmabuf = dma_buf_export(&exp_info);
	if (IS_ERR(dmabuf)) {
		ret = PTR_ERR(dmabuf);
		goto free_pages;
	}
	return dmabuf;

free_pages:
	kfree(buffer->pages);
free_cma:
	cma_release(meson_cma_heap->cma, cma_pages, pagecount);
free_buffer:
	kfree(buffer);

	return ERR_PTR(ret);
}

static const struct dma_heap_ops meson_cma_heap_ops = {
	.allocate = meson_cma_heap_allocate,
};

static int __add_meson_cma_heap(struct cma *cma, void *data)
{
	struct meson_cma_heap *meson_cma_heap;
	struct dma_heap_export_info exp_info;

	meson_cma_heap = kzalloc(sizeof(*meson_cma_heap), GFP_KERNEL);
	if (!meson_cma_heap)
		return -ENOMEM;
	meson_cma_heap->cma = cma;

	exp_info.name = cma_get_name(cma);
	exp_info.ops = &meson_cma_heap_ops;
	exp_info.priv = meson_cma_heap;

	meson_cma_heap->heap = dma_heap_add(&exp_info);
	if (IS_ERR(meson_cma_heap->heap)) {
		int ret = PTR_ERR(meson_cma_heap->heap);

		kfree(meson_cma_heap);
		return ret;
	}

	return 0;
}

static int meson_cma_scan(struct cma *cma, void *data)
{
	const char *cma_name = cma_get_name(cma);

	if (strstr(cma_name, "heap-gfx") ||
		strstr(cma_name, "heap-fb") ||
		strstr(cma_name, "heap-secure")) {
		__add_meson_cma_heap(cma, NULL);
		pr_info("dmaheap: find %s\n", cma_name);
	}
	return 0;
}

int add_meson_cma_heap(void)
{
	cma_for_each_area(meson_cma_scan, NULL);
	return 0;
}

MODULE_LICENSE("GPL v2");
