// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/scatterlist.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/dma-buf.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/genalloc.h>
#include <linux/list.h>
#include <linux/mm.h>
#include <linux/compat.h>
#include <linux/amlogic/tee.h>
#include <linux/amlogic/media/codec_mm/codec_mm.h>
#include <linux/amlogic/media/codec_mm/dmabuf_manage.h>
#include <linux/amlogic/media/codec_mm/configs.h>
#if IS_ENABLED(CONFIG_AMLOGIC_OPTEE)
#include <linux/amlogic/tee_drv.h>
#endif
#include <uapi/linux/dvb/aml_dmx_ext.h>
#ifdef CONFIG_AMLOGIC_DVB_COMPAT
#include <media/aml_demux_ext.h>
#endif
#include "dmxdev.h"
#include "demux.h"

#if IS_BUILTIN(CONFIG_AMLOGIC_MEDIA_MODULE)
#include <linux/amlogic/aml_cma.h>
#endif

static int dmabuf_manage_debug = 1;
module_param(dmabuf_manage_debug, int, 0644);

#if IS_ENABLED(CONFIG_AMLOGIC_OPTEE)
static u32 secure_heap_version = SECURE_HEAP_USER_TA_VERSION;
#else
static u32 secure_heap_version = SECURE_HEAP_DYNAMIC_ALLOC_VERSION;
#endif

static u32 secure_pool_max_block_count = 8;
static u32 secure_vdec_def_size_bytes;
static u32 secure_hd_pool_size_mb = 4;
static u32 secure_uhd_pool_size_mb = 12;
static u32 secure_fuhd_pool_size_mb = 32;
static u32 secure_vdec_config = 0x7C;

#define  DEVICE_NAME "secmem"
#define  CLASS_NAME  "dmabuf_manage"

#define CONFIG_PATH "media.dmabuf_manage"
#define CONFIG_PREFIX "media"

#define dprintk(level, fmt, arg...)						\
	do {									\
		if (dmabuf_manage_debug >= (level))						\
			pr_info(CLASS_NAME ": %s: " fmt, __func__, ## arg);\
	} while (0)

#define pr_dbg(fmt, args ...)  dprintk(6, fmt, ## args)
#define pr_error(fmt, args ...) dprintk(1, fmt, ## args)
#define pr_inf(fmt, args ...) dprintk(8, fmt, ## args)
#define pr_enter() pr_inf("enter")

#define POOL_SIZE_MB (1024 * 1024)
#define DMABUF_MANAGE_POOL_ALIGN_2N		20
#define DMABUF_MANAGE_POOL_SIZE_MB		32

#if IS_ENABLED(CONFIG_AMLOGIC_OPTEE)
#define DMABUF_MANAGE_BLOCK_MIN_SIZE_KB				(256 * 1024)

#define TA_SECMEM_V2_CMD_INIT						294
#define TA_SECMEM_V2_CMD_MEM_CREATE					251
#define TA_SECMEM_V2_CMD_MEM_FREE					258
#define TA_SECMEM_V2_CMD_CLOSE						266

#define TA_SECMEM_V2_SHM_SIZE						4096
#define TEE_CMD_PARAM_NUM							4
#define PARAM_ALIGN									32

#define SECMEM_TA_UUID UUID_INIT(0x2c1a33c0, 0x44cc, 0x11e5, \
		0xbc, 0x3b, 0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b)
#endif

struct kdmabuf_attachment {
	struct sg_table sgt;
	enum dma_data_direction dma_dir;
};

struct block_node {
	struct list_head node;
	u32 addr;
	u32 size;
};

struct secure_pool_info {
	struct list_head node;
	struct list_head block_node;
	u32 version;
	u32 id_high;
	u32 id_low;
	u32 protect_handle;
	struct gen_pool *gen_pool;
	struct codec_mm_s *mms;
	phys_addr_t pool_addr;
	u32 pool_size;
	u32 block_size;
	u32 max_frame_size;
	u32 channel_register;
#if IS_ENABLED(CONFIG_AMLOGIC_OPTEE)
	struct tee_ioctl_open_session_arg secmem_session;
	struct tee_shm *shm_pool;
#endif
};

typedef int (*decode_info)(struct dmx_demux *demux, void *info);
static long dmabuf_manage_release_channel(u32 id_high, u32 id_low);
static int dmabuf_manage_release_dmabufheap_resource(struct secure_pool_info *release_pool);

struct dmx_filter_info {
	struct list_head list;
	__u32 token;
	__u32 filter_fd;
	struct dmx_demux *demux;
	decode_info decode_info_func;
};

static struct list_head pool_list;
static struct list_head dmx_filter_list;
static int dev_no;
static struct device *dmabuf_manage_dev;
static DEFINE_MUTEX(g_secure_pool_mutex);
#if IS_ENABLED(CONFIG_AMLOGIC_OPTEE)
static struct tee_context *g_secmem_context;

enum TEE_CMD_PARAMTYPE {
	TEE_CMD_PARAMTYPE_BUF,
	TEE_CMD_PARAMTYPE_UINT32,
	TEE_CMD_PARAMTYPE_UINT64,
	TEE_CMD_PARAMTYPE_VOID,
};

struct tee_cmdparam {
	u32 type;
	union {
		struct { /* type == tee_cmdparamType_Buf */
			u32 buflen;
			u32 pbuf;
		} buf;
		u32 u32_value; /* type == TEE_CMD_PARAMTYPE_UINT32 */
	} param;
};

static int dmabuf_manage_tee_pack_u32(char *shm_data, const u32 num, u32 *poff)
{
	struct tee_cmdparam p;
	u32 off = 0;

	if (!shm_data || !poff)
		return -1;

	off = *poff;
	if (off > TA_SECMEM_V2_SHM_SIZE - sizeof(struct tee_cmdparam))
		return -1;

	p.type = TEE_CMD_PARAMTYPE_UINT32;
	p.param.u32_value = num;
	memcpy((void *)(shm_data + off), &p, sizeof(struct tee_cmdparam));
	*poff = (off + sizeof(struct tee_cmdparam) + PARAM_ALIGN) & ~(PARAM_ALIGN - 1);

	return 0;
}

static int dmabuf_manage_tee_unpack_u32(const char *shm, u32 *num, u32 *poff)
{
	struct tee_cmdparam p;
	u32 off = 0;

	if (!shm || !poff || !num)
		return -1;

	off = *poff;
	if (off > TA_SECMEM_V2_SHM_SIZE - sizeof(struct tee_cmdparam))
		return -1;

	memcpy((void *)&p, (void *)(shm + off), sizeof(struct tee_cmdparam));
	if (p.type != TEE_CMD_PARAMTYPE_UINT32) {
		pr_error("error param type %d", p.type);
		return -1;
	}

	*num = p.param.u32_value;
	*poff = (off + sizeof(struct tee_cmdparam) + PARAM_ALIGN) & ~(PARAM_ALIGN - 1);

	return 0;
}
#endif

static int dmabuf_manage_attach(struct dma_buf *dbuf, struct dma_buf_attachment *attachment)
{
	struct kdmabuf_attachment *attach;
	struct dmabuf_manage_block *block = dbuf->priv;
	struct sg_table *sgt;
	struct page *page;
	phys_addr_t phys = block->paddr;
	int ret;
	int sgnum = 1;
	struct dmabuf_dmx_sec_es_data *es = NULL;
	int len = 0;

	pr_enter();
	attach = (struct kdmabuf_attachment *)
		kzalloc(sizeof(*attach), GFP_KERNEL);
	if (!attach) {
		pr_error("kzalloc failed\n");
		goto error;
	}

	if (block->type == DMA_BUF_TYPE_DMX_ES) {
		es = (struct dmabuf_dmx_sec_es_data *)block->priv;
		if (es->data_end < es->data_start)
			sgnum = 2;
	}
	sgt = &attach->sgt;
	ret = sg_alloc_table(sgt, sgnum, GFP_KERNEL);
	if (ret) {
		pr_error("No memory for sgtable");
		goto error_alloc;
	}
	if (sgnum == 2) {
		len = PAGE_ALIGN(es->buf_end - es->data_start);
		if (block->paddr != es->data_start) {
			pr_error("Invalid buffer info %x %x",
				block->paddr, es->data_start);
			goto error_attach;
		}
		page = phys_to_page(phys);
		sg_set_page(sgt->sgl, page, len, 0);
		len = PAGE_ALIGN(es->data_end - es->buf_start);
		page = phys_to_page(es->buf_start);
		sg_set_page(sg_next(sgt->sgl), page, len, 0);
	} else {
		page = phys_to_page(phys);
		sg_set_page(sgt->sgl, page, PAGE_ALIGN(block->size), 0);
	}
	attach->dma_dir = DMA_NONE;
	attachment->priv = attach;
	return 0;
error_attach:
	sg_free_table(sgt);
error_alloc:
	kfree(attach);
error:
	return 0;
}

static void dmabuf_manage_detach(struct dma_buf *dbuf,
				struct dma_buf_attachment *attachment)
{
	struct kdmabuf_attachment *attach = attachment->priv;
	struct sg_table *sgt;

	pr_enter();
	if (!attach)
		return;
	sgt = &attach->sgt;
	sg_free_table(sgt);
	kfree(attach);
	attachment->priv = NULL;
}

static struct sg_table *dmabuf_manage_map_dma_buf(struct dma_buf_attachment *attachment,
		enum dma_data_direction dma_dir)
{
	struct kdmabuf_attachment *attach = attachment->priv;
	struct dmabuf_manage_block *block = attachment->dmabuf->priv;
	struct mutex *lock = &attachment->dmabuf->lock;
	struct sg_table *sgt;

	pr_enter();
	mutex_lock(lock);
	sgt = &attach->sgt;
	if (attach->dma_dir == dma_dir) {
		mutex_unlock(lock);
		return sgt;
	}
	sgt->sgl->dma_address = block->paddr;
#ifdef CONFIG_NEED_SG_DMA_LENGTH
	sgt->sgl->dma_length = PAGE_ALIGN(block->size);
#else
	sgt->sgl->length = PAGE_ALIGN(block->size);
#endif
	pr_dbg("nents %d, %x, %d, %d\n", sgt->nents, block->paddr,
			sg_dma_len(sgt->sgl), block->size);
	attach->dma_dir = dma_dir;
	mutex_unlock(lock);
	return sgt;
}

static void dmabuf_manage_unmap_dma_buf(struct dma_buf_attachment *attachment,
		struct sg_table *sgt,
		enum dma_data_direction dma_dir)
{
	pr_enter();
}

static void dmabuf_manage_buf_release(struct dma_buf *dbuf)
{
	struct dmabuf_manage_block *block = NULL;
	struct dmabuf_dmx_sec_es_data *es = NULL;
	struct dmx_filter_info *node = NULL;
	struct secure_vdec_channel *channel = NULL;
	struct list_head *pos = NULL, *tmp = NULL;
	int found = 0;
	struct decoder_mem_info rp_info;

	pr_enter();
	block = (struct dmabuf_manage_block *)dbuf->priv;
	if (block && block->priv && block->type == DMA_BUF_TYPE_DMX_ES) {
		es = (struct dmabuf_dmx_sec_es_data *)block->priv;
		list_for_each_safe(pos, tmp, &dmx_filter_list) {
			node = list_entry(pos, struct dmx_filter_info, list);
			if (node && node->token == es->token) {
				found = 1;
				break;
			}
		}
		if (found) {
			if (es->buf_rp == 0)
				es->buf_rp = es->data_end;
			if (es->buf_rp >= es->buf_start && es->buf_rp <= es->buf_end &&
				node->demux && node->decode_info_func) {
				rp_info.rp_phy = es->buf_rp;
				node->decode_info_func(node->demux, &rp_info);
			}
		}
	} else if (block && block->type == DMA_BUF_TYPE_DMABUF) {
		if (block->flags & DMABUF_ALLOC_FROM_CMA)
			codec_mm_free_for_dma("dmabuf", block->paddr);
	} else if (block && block->priv &&
		block->type == DMA_BUF_TYPE_SECURE_VDEC) {
		channel = (struct secure_vdec_channel *)block->priv;
		dmabuf_manage_release_channel(channel->id_high, channel->id_low);
	}

	if (block) {
		pr_dbg("dma release handle:%x\n", block->handle);
		kfree(block->priv);
		kfree(block);
	}
}

static int dmabuf_manage_mmap(struct dma_buf *dbuf, struct vm_area_struct *vma)
{
	struct dmabuf_manage_block *block;
	struct dmabuf_dmx_sec_es_data *es;
	unsigned long addr = vma->vm_start;
	int len = 0;
	int ret = -EFAULT;

	pr_enter();
	block = (struct dmabuf_manage_block *)dbuf->priv;
	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
	if (block->type == DMA_BUF_TYPE_DMX_ES) {
		es = (struct dmabuf_dmx_sec_es_data *)block->priv;
		if (es->data_end >= es->data_start) {
			len = PAGE_ALIGN(es->data_end - es->data_start);
			if (block->paddr != es->data_start) {
				pr_error("Invalid buffer info %x %x",
					block->paddr, es->data_start);
				goto error;
			}
			ret = remap_pfn_range(vma, addr,
					page_to_pfn(phys_to_page(block->paddr)),
					len, vma->vm_page_prot);
		} else {
			len = PAGE_ALIGN(es->buf_end - es->data_start);
			if (block->paddr != es->data_start) {
				pr_error("Invalid buffer info %x %x",
					block->paddr, es->data_start);
				goto error;
			}
			ret = remap_pfn_range(vma, addr,
					page_to_pfn(phys_to_page(block->paddr)),
					len, vma->vm_page_prot);
			if (ret) {
				pr_error("remap failed %d", ret);
				goto error;
			}
			addr += len;
			if (addr >= vma->vm_end)
				return ret;
			len = PAGE_ALIGN(es->data_end - es->buf_start);
			ret = remap_pfn_range(vma, addr,
					page_to_pfn(phys_to_page(es->buf_start)),
					len, vma->vm_page_prot);
		}
	} else {
		ret = remap_pfn_range(vma, vma->vm_start, page_to_pfn(phys_to_page(block->paddr)),
			block->size, vma->vm_page_prot);
	}
error:
	return ret;
}

static struct dma_buf_ops dmabuf_manage_ops = {
	.attach = dmabuf_manage_attach,
	.detach = dmabuf_manage_detach,
	.map_dma_buf = dmabuf_manage_map_dma_buf,
	.unmap_dma_buf = dmabuf_manage_unmap_dma_buf,
	.release = dmabuf_manage_buf_release,
	.mmap = dmabuf_manage_mmap
};

bool dmabuf_is_esbuf(struct dma_buf *dmabuf)
{
	return dmabuf->ops == &dmabuf_manage_ops;
}
EXPORT_SYMBOL(dmabuf_is_esbuf);

static struct dma_buf *get_dmabuf(struct dmabuf_manage_block *block,
				  unsigned long flags)
{
	struct dma_buf *dbuf;
	DEFINE_DMA_BUF_EXPORT_INFO(exp_info);

	pr_dbg("export handle:%x, paddr:%x, size:%x\n",
		block->handle, block->paddr, block->size);
	exp_info.ops = &dmabuf_manage_ops;
	exp_info.size = block->size;
	exp_info.flags = flags;
	exp_info.priv = (void *)block;
	exp_info.exp_name = "dmabuf_manage";

	dbuf = dma_buf_export(&exp_info);
	if (IS_ERR(dbuf))
		return NULL;
	return dbuf;
}

static long dmabuf_manage_export(unsigned long args)
{
	int ret;
	struct dmabuf_manage_block *block;
	struct dma_buf *dbuf;
	int fd;
	int fd_flags = O_CLOEXEC;

	pr_enter();
	block = (struct dmabuf_manage_block *)
		kzalloc(sizeof(*block), GFP_KERNEL);
	if (!block) {
		pr_error("kmalloc failed\n");
		goto error_alloc_object;
	}
	ret = copy_from_user((void *)block, (void __user *)args,
				sizeof(struct secmem_block));
	if (ret) {
		pr_error("copy_from_user failed\n");
		goto error_copy;
	}
	block->type = DMA_BUF_TYPE_SECMEM;
	dbuf = get_dmabuf(block, fd_flags);
	if (!dbuf) {
		pr_error("get_dmabuf failed\n");
		goto error_export;
	}
	fd = dma_buf_fd(dbuf, fd_flags);
	if (fd < 0) {
		pr_error("dma_buf_fd failed\n");
		goto error_fd;
	}
	return fd;
error_fd:
	dma_buf_put(dbuf);
error_export:
error_copy:
	kfree(block);
error_alloc_object:
	return -EFAULT;
}

static long dmabuf_manage_get_handle(unsigned long args)
{
	int ret;
	long res = -EFAULT;
	int fd;
	struct dma_buf *dbuf;
	struct dmabuf_manage_block *block;

	pr_enter();
	ret = copy_from_user((void *)&fd, (void __user *)args, sizeof(fd));
	if (ret) {
		pr_error("copy_from_user failed\n");
		goto error_copy;
	}
	dbuf = dma_buf_get(fd);
	if (IS_ERR(dbuf)) {
		pr_error("dma_buf_get failed\n");
		goto error_get;
	}
	block = dbuf->priv;
	res = (long)(block->handle & (0xffffffff));
	dma_buf_put(dbuf);
error_get:
error_copy:
	return res;
}

static long dmabuf_manage_get_phyaddr(unsigned long args)
{
	int ret;
	long res = -EFAULT;
	int fd;
	struct dma_buf *dbuf;
	struct dmabuf_manage_block *block;

	pr_enter();
	ret = copy_from_user((void *)&fd, (void __user *)args, sizeof(fd));
	if (ret) {
		pr_error("copy_from_user failed\n");
		goto error_copy;
	}
	dbuf = dma_buf_get(fd);
	if (IS_ERR(dbuf)) {
		pr_error("dma_buf_get failed\n");
		goto error_get;
	}
	block = dbuf->priv;
	res = (long)(block->paddr & (0xffffffff));
	dma_buf_put(dbuf);
error_get:
error_copy:
	return res;
}

static long dmabuf_manage_import(unsigned long args)
{
	int ret;
	long res = -EFAULT;
	int fd;
	struct dma_buf *dbuf;
	struct dma_buf_attachment *attach;
	struct sg_table *sgt;
	dma_addr_t paddr;

	pr_enter();
	ret = copy_from_user((void *)&fd, (void __user *)args, sizeof(fd));
	if (ret) {
		pr_error("copy_from_user failed\n");
		goto error_copy;
	}
	dbuf = dma_buf_get(fd);
	if (IS_ERR(dbuf)) {
		pr_error("dma_buf_get failed\n");
		goto error_get;
	}
	attach = dma_buf_attach(dbuf, dmabuf_manage_dev);
	if (IS_ERR(attach)) {
		pr_error("dma_buf_attach failed\n");
		goto error_attach;
	}
	sgt = dma_buf_map_attachment(attach, DMA_FROM_DEVICE);
	if (IS_ERR(sgt)) {
		pr_error("dma_buf_map_attachment failed\n");
		goto error_map;
	}
	paddr = sg_dma_address(sgt->sgl);
	res = (long)(paddr & (0xffffffff));
	dma_buf_unmap_attachment(attach, sgt, DMA_FROM_DEVICE);
error_map:
	dma_buf_detach(dbuf, attach);
error_attach:
	dma_buf_put(dbuf);
error_get:
error_copy:
	return res;
}

static long dmabuf_manage_export_dmabuf(unsigned long args)
{
	long res = -EFAULT;
	struct dmabuf_manage_buffer info;
	struct dmabuf_manage_block *block;
	struct dmabuf_dmx_sec_es_data *dmxes;
	struct dmabuf_videodec_es_data *vdecdata;
	struct dma_buf *dbuf;
	int fd = -1;
	int fd_flags = O_CLOEXEC;

	pr_enter();
	res = copy_from_user((void *)&info, (void __user *)args, sizeof(info));
	if (res) {
		pr_error("copy_from_user failed\n");
		goto error_copy;
	}
	if (info.type == DMA_BUF_TYPE_INVALID) {
		pr_error("unknown dma buf type\n");
		goto error_copy;
	}
	block = kzalloc(sizeof(*block), GFP_KERNEL);
	if (!block) {
		pr_error("kmalloc failed\n");
		goto error_copy;
	}
	switch (info.type) {
	case DMA_BUF_TYPE_SECMEM:
		fd_flags = O_CLOEXEC;
		break;
	case DMA_BUF_TYPE_DMX_ES:
		fd_flags = O_CLOEXEC;
		dmxes = kzalloc(sizeof(*dmxes), GFP_KERNEL);
		if (!dmxes) {
			pr_error("kmalloc failed\n");
			goto error_alloc_object;
		}
		memcpy(dmxes, &info.buffer.dmxes, sizeof(*dmxes));
		block->priv = dmxes;
		break;
	case DMA_BUF_TYPE_DMABUF:
		fd_flags = O_RDWR | O_CLOEXEC;
		break;
	case DMA_BUF_TYPE_VIDEODEC_ES:
		fd_flags = O_RDWR | O_CLOEXEC;
		vdecdata = kzalloc(sizeof(*vdecdata), GFP_KERNEL);
		if (!vdecdata) {
			pr_error("kmalloc failed\n");
			goto error_alloc_object;
		}
		memcpy(vdecdata, &info.buffer.vdecdata, sizeof(*vdecdata));
		block->priv = vdecdata;
		break;
	default:
		block->priv = NULL;
		break;
	}
	block->paddr = info.paddr;
	block->size = PAGE_ALIGN(info.size);
	block->handle = info.handle;
	block->type = info.type;
	dbuf = get_dmabuf(block, fd_flags);
	if (!dbuf) {
		pr_error("get_dmabuf failed\n");
		goto error_alloc_object;
	}
	fd = dma_buf_fd(dbuf, fd_flags);
	if (fd < 0) {
		pr_error("dma_buf_fd failed\n");
		goto error_fd;
	}
	return fd;
error_fd:
	dma_buf_put(dbuf);
error_alloc_object:
	kfree(block->priv);
	kfree(block);
error_copy:
	return res;
}

static long dmabuf_manage_get_dmabufinfo(unsigned long args)
{
	struct dmabuf_manage_buffer info;
	struct dmabuf_manage_block *block;
	struct dma_buf *dbuf;

	pr_enter();
	if (copy_from_user((void *)&info, (void __user *)args, sizeof(info))) {
		pr_error("copy_from_user failed\n");
		goto error;
	}
	if (info.type == DMA_BUF_TYPE_INVALID)
		goto error;
	dbuf = dma_buf_get(info.fd);
	if (IS_ERR(dbuf))
		goto error;
	if (dbuf->priv && dbuf->ops == &dmabuf_manage_ops) {
		block = dbuf->priv;
		if (block->type != info.type)
			goto error;
		info.paddr = block->paddr;
		info.size = block->size;
		info.handle = block->handle;
		switch (info.type) {
		case DMA_BUF_TYPE_DMX_ES:
			memcpy(&info.buffer.dmxes, block->priv,
				sizeof(struct dmabuf_dmx_sec_es_data));
			break;
		case DMA_BUF_TYPE_VIDEODEC_ES:
			memcpy(&info.buffer.vdecdata, block->priv,
				sizeof(struct dmabuf_videodec_es_data));
			break;
		default:
			break;
		}
		if (copy_to_user((void __user *)args, &info, sizeof(info))) {
			pr_error("error copy to use space");
			goto error_fd;
		}
	}
	dma_buf_put(dbuf);
	return 0;
error_fd:
	dma_buf_put(dbuf);
error:
	return -EFAULT;
}

static long dmabuf_manage_set_filterfd(unsigned long args)
{
	struct filter_info info;
	struct dmx_filter_info *node = NULL;
	struct list_head *pos, *tmp;
	int found = 0;
	struct dmx_demux *demux = NULL;
	decode_info decode_info_func = NULL;
#ifdef CONFIG_AMLOGIC_DVB_COMPAT
	struct fd f;
	struct dmxdev_filter *dmxdevfilter = NULL;
	struct dmxdev *dmxdev = NULL;
	struct dmx_demux_ext *demux_ext = NULL;
#endif
	pr_enter();
	if (copy_from_user((void *)&info, (void __user *)args, sizeof(info))) {
		pr_error("copy_from_user failed\n");
		goto error;
	}
	if (list_empty(&dmx_filter_list)) {
		if (info.release) {
			pr_error("No filter info setting\n");
			goto error;
		} else {
			node = kzalloc(sizeof(*node), GFP_KERNEL);
			if (!node)
				goto error;
			list_add_tail(&node->list, &dmx_filter_list);
		}
	} else {
		list_for_each_safe(pos, tmp, &dmx_filter_list) {
			node = list_entry(pos, struct dmx_filter_info, list);
			if (node->token == info.token && node->filter_fd == info.filter_fd) {
				found = 1;
				break;
			}
		}
		if (info.release) {
			if (!found) {
				pr_error("No filter info setting\n");
				goto error;
			} else {
				list_del(&node->list);
				kfree(node);
				node = NULL;
			}
		} else {
			if (!found) {
				node = kzalloc(sizeof(*node), GFP_KERNEL);
				if (!node)
					goto error;
				list_add_tail(&node->list, &dmx_filter_list);
			}
		}
	}
	if (node) {
		node->token = info.token;
		node->filter_fd = info.filter_fd;
#ifdef CONFIG_AMLOGIC_DVB_COMPAT
		f = fdget(node->filter_fd);
		if (f.file && f.file->private_data) {
			dmxdevfilter = f.file->private_data;
			if (dmxdevfilter)
				dmxdev = dmxdevfilter->dev;
		}
		if (dmxdev && dmxdev->demux) {
			demux = dmxdev->demux;
			demux_ext = container_of(demux, struct dmx_demux_ext, dmx);
			decode_info_func = demux_ext->decode_info;
		} else {
			pr_error("Invalid filter fd\n");
		}
		fdput(f);
#endif
		node->demux = demux;
		node->decode_info_func = decode_info_func;
	}
	return 0;
error:
	return -EFAULT;
}

static int dmabuf_manage_alloc_dmabuf(unsigned long args)
{
	int res = -EFAULT;
	struct dmabuf_manage_buffer info;
	int fd_flags = O_RDWR | O_CLOEXEC;
	struct dmabuf_manage_block *block;
	struct dma_buf *dbuf;

	pr_enter();
	res = copy_from_user((void *)&info, (void __user *)args, sizeof(info));
	if (res)
		goto error_copy;
	if (info.size <= 0 || info.size % 4096 != 0) {
		pr_error("Invalid size isn't 4K align %d", info.size);
		goto error_copy;
	}
	block = kzalloc(sizeof(*block), GFP_KERNEL);
	if (!block) {
		pr_error("kmalloc failed\n");
		goto error_copy;
	}
	block->paddr = codec_mm_alloc_for_dma("dmabuf", info.size / PAGE_SIZE,
		PAGE_SHIFT, CODEC_MM_FLAGS_DMA);
	if (block->paddr <= 0)
		goto error_alloc_object;
	block->size = PAGE_ALIGN(info.size);
	block->handle = info.handle;
	block->type = info.type;
	block->flags |= DMABUF_ALLOC_FROM_CMA;
	dbuf = get_dmabuf(block, fd_flags);
	if (!dbuf) {
		pr_error("get_dmabuf failed\n");
		goto error_alloc;
	}
	res = dma_buf_fd(dbuf, fd_flags);
	if (res < 0) {
		pr_error("dma_buf_fd failed\n");
		goto error_fd;
	}
	return res;
error_fd:
	dma_buf_put(dbuf);
error_alloc:
	codec_mm_free_for_dma("dmabuf", block->paddr);
error_alloc_object:
	kfree(block);
error_copy:
	return -EFAULT;
}

unsigned int dmabuf_manage_get_type(struct dma_buf *dbuf)
{
	int ret = DMA_BUF_TYPE_INVALID;
	struct dmabuf_manage_block *block;

	if (!dbuf) {
		pr_dbg("acquire dma_buf failed");
		goto error;
	}
	if (dbuf->priv && dbuf->ops == &dmabuf_manage_ops) {
		block = (struct dmabuf_manage_block *)dbuf->priv;
		if (block)
			ret = block->type;
	}
error:
	return ret;
}
EXPORT_SYMBOL(dmabuf_manage_get_type);

void *dmabuf_manage_get_info(struct dma_buf *dbuf, unsigned int type)
{
	void *buf = NULL;
	struct dmabuf_manage_block *block;

	if (type == DMA_BUF_TYPE_INVALID)
		goto error;
	if (!dbuf) {
		pr_dbg("acquire dma_buf failed");
		goto error;
	}
	if (dbuf->priv && dbuf->ops == &dmabuf_manage_ops) {
		block = (struct dmabuf_manage_block *)dbuf->priv;
		switch (block->type) {
		case DMA_BUF_TYPE_SECMEM:
			buf = block;
			break;
		case DMA_BUF_TYPE_DMX_ES:
			buf = block->priv;
			break;
		case DMA_BUF_TYPE_DMABUF:
			buf = block;
			break;
		case DMA_BUF_TYPE_VIDEODEC_ES:
			buf = block->priv;
			break;
		default:
			break;
		}
	}
error:
	return buf;
}
EXPORT_SYMBOL(dmabuf_manage_get_info);

static struct secure_pool_info *dmabuf_manage_get_secure_vdec_pool(u32 id_high,
	u32 id_low)
{
	struct list_head *pos = NULL;
	struct list_head *tmp = NULL;
	struct secure_pool_info *pool = NULL;

	if (!list_empty(&pool_list)) {
		list_for_each_safe(pos, tmp, &pool_list) {
			pool = list_entry(pos, struct secure_pool_info, node);
			if (pool && pool->id_high == id_high && pool->id_low == id_low)
				return pool;
		}
	}

	return NULL;
}

static int dmabuf_manage_secure_pool_get_block_count(struct secure_pool_info *pool)
{
	int count = 0;
	struct list_head *pos = NULL;
	struct list_head *tmp = NULL;

	if (pool && !list_empty(&pool->block_node)) {
		list_for_each_safe(pos, tmp, &pool->block_node)
			count++;
	}

	return count;
}

static int dmabuf_manage_secure_pool_get_block_free_slot(struct secure_pool_info *pool,
	u32 frame_size)
{
	int slot = 0;
	int index = 0;
	phys_addr_t *paddr = NULL;
	int alignsize = PAGE_ALIGN(frame_size);

	if (pool && pool->gen_pool) {
		if (pool->max_frame_size < alignsize)
			pool->max_frame_size = alignsize;

		paddr = kcalloc(secure_pool_max_block_count, sizeof(phys_addr_t), GFP_KERNEL);
		if (!paddr)
			return slot;

		for (index = 0; index < secure_pool_max_block_count; index++, slot++) {
			paddr[index] = gen_pool_alloc(pool->gen_pool, alignsize);
			if (paddr[index] <= 0)
				break;
		}

		for (index = 0; index < slot; index++)
			gen_pool_free(pool->gen_pool, paddr[index], alignsize);

		kfree(paddr);
	}

	return slot;
}

static inline u32 secure_pool_align_up2n(u32 size, u32 alg2n)
{
	return ((size + (1 << alg2n) - 1) & (~((1 << alg2n) - 1)));
}

static int dmabuf_manage_secure_genpool_init(u32 id_high, u32 id_low, u32 block_size,
	u32 *pool_addr, u32 *pool_size, u32 version)
{
	int res = 1;
	struct secure_pool_info *pool = NULL;

	if (!pool_addr || !pool_size)
		goto error;

	pool = kzalloc(sizeof(*pool), GFP_KERNEL);
	if (!pool)
		goto error;

	pool->version = version;
	pool->id_high = id_high;
	pool->id_low = id_low;
	pool->block_size = block_size;

	if (version == SECURE_HEAP_DEFAULT_VERSION) {
		pool->pool_size = PAGE_ALIGN(block_size * secure_pool_max_block_count);
	} else {
		if (block_size <= 2 * POOL_SIZE_MB)
			pool->pool_size = secure_hd_pool_size_mb * POOL_SIZE_MB;
		else if (block_size <= 6 * POOL_SIZE_MB)
			pool->pool_size = secure_uhd_pool_size_mb * POOL_SIZE_MB;
		else
			pool->pool_size = secure_fuhd_pool_size_mb * POOL_SIZE_MB;
	}

	pool->pool_size = secure_pool_align_up2n(pool->pool_size,
		DMABUF_MANAGE_POOL_ALIGN_2N);
	if (pool->pool_size > DMABUF_MANAGE_POOL_SIZE_MB * POOL_SIZE_MB)
		pool->pool_size = DMABUF_MANAGE_POOL_SIZE_MB * POOL_SIZE_MB;

	pool->mms = codec_mm_alloc("dma-secure-buf", pool->pool_size,
		DMABUF_MANAGE_POOL_ALIGN_2N, CODEC_MM_FLAGS_DMA);
	if (!pool->mms || !pool->mms->phy_addr)
		goto error_alloc;

	pool->pool_addr = pool->mms->phy_addr;
#if IS_BUILTIN(CONFIG_AMLOGIC_MEDIA_MODULE)
	cma_mmu_op(pool->mms->mem_handle, pool->mms->page_count, 0);
#endif
	res = tee_protect_mem_by_type(TEE_MEM_TYPE_STREAM_INPUT,
		(u32)pool->pool_addr, pool->pool_size, &pool->protect_handle);
	if (res) {
		pr_error("Protect vdec fail %x %d %d %x %x\n", res, id_high, id_low,
			(u32)pool->pool_addr, pool->pool_size);
		goto error_protect;
	}

	res = tee_check_in_mem((u32)pool->pool_addr, (u32)pool->pool_size);
	if (res) {
		pr_error("CheckIn vdec fail %x %d %d %x %x\n", res, id_high, id_low,
			(u32)pool->pool_addr, pool->pool_size);
		goto error_checkin;
	}

	pool->gen_pool = gen_pool_create(PAGE_SHIFT, -1);
	if (!pool->gen_pool) {
		pr_error("Create gen pool fail for %d %d %x %x\n", id_high, id_low,
			(u32)pool->pool_addr, pool->pool_size);
		goto error_create_pool;
	}

	res = gen_pool_add(pool->gen_pool, pool->pool_addr, pool->pool_size, -1);
	if (res) {
		pr_error("Add gen pool fail %d for %d %d %x %x\n", res, id_high, id_low,
			(u32)pool->pool_addr, pool->pool_size);
		goto error_add_pool;
	}

	*pool_addr = pool->pool_addr;
	*pool_size = pool->pool_size;
	INIT_LIST_HEAD(&pool->block_node);
	list_add_tail(&pool->node, &pool_list);
	return 0;
error_add_pool:
	gen_pool_destroy(pool->gen_pool);
error_create_pool:
	res = tee_check_out_mem(pool->pool_addr,  pool->pool_size);
	if (res)
		pr_error("Check Out %x %x fail\n", (u32)pool->pool_addr, pool->pool_size);
error_checkin:
	tee_unprotect_mem(pool->protect_handle);
error_protect:
#if IS_BUILTIN(CONFIG_AMLOGIC_MEDIA_MODULE)
	cma_mmu_op(pool->mms->mem_handle, pool->mms->page_count, 1);
#endif
	res = codec_mm_free_for_dma("dma-secure-buf", pool->pool_addr);
	if (res)
		pr_error("Free %x %x fail\n", (u32)pool->pool_addr, pool->pool_size);
error_alloc:
	kfree(pool);
error:
	return 1;
}

#if IS_ENABLED(CONFIG_AMLOGIC_OPTEE)
static int dmabuf_manage_secmem_ctx_match(struct tee_ioctl_version_data *ver,
	const void *data)
{
	return (ver && ver->impl_id == TEE_IMPL_ID_OPTEE);
}

static int dmabuf_manage_secure_v2_init(struct secure_pool_info *pool, u32 flags)
{
	int res = 0;
	u32 in_len = 0;
	u32 config = 0;
	u32 tvp_flag = flags & 0x0F;
	u32 decoder_flag = (flags >> 4) & 0x0F;
	u32 vd_index = (flags >> 9) & 0x0F;
	char *shm_data = NULL;
	struct tee_param param[TEE_CMD_PARAM_NUM] = { 0 };
	struct tee_ioctl_invoke_arg inv_arg = { 0 };

	if (!pool || !g_secmem_context)
		return -1;

	if (!pool->shm_pool)
		return -1;

	config = secure_vdec_config;
	if ((decoder_flag & 0x3) == 0x3)
		config |= 0x400000;

	if ((decoder_flag & 0x8) == 0x8)
		config |= 0x2;

	if (tvp_flag == 0x2)
		config |= 0x100;

	config |= (vd_index << 11);
	config |= 0x10000;

	shm_data = tee_shm_get_va(pool->shm_pool, 0);
	if (IS_ERR(shm_data)) {
		pr_error("%s tee_shm_get_va failed\n", __func__);
		res = -EBUSY;
		goto error;
	}

	res = dmabuf_manage_tee_pack_u32(shm_data, TA_SECMEM_V2_CMD_INIT, &in_len);
	if (res) {
		pr_error("%s pass tee parameter failed\n", __func__);
		res = -EFAULT;
		goto error;
	}

	res = dmabuf_manage_tee_pack_u32(shm_data, 1, &in_len);
	if (res) {
		pr_error("%s pass tee parameter failed\n", __func__);
		res = -EFAULT;
		goto error;
	}

	res = dmabuf_manage_tee_pack_u32(shm_data, config, &in_len);
	if (res) {
		pr_error("%s pass tee parameter failed\n", __func__);
		res = -EFAULT;
		goto error;
	}

	res = dmabuf_manage_tee_pack_u32(shm_data, 0, &in_len);
	if (res) {
		pr_error("%s pass tee parameter failed\n", __func__);
		res = -EFAULT;
		goto error;
	}

	res = dmabuf_manage_tee_pack_u32(shm_data, 0, &in_len);
	if (res) {
		pr_error("%s pass tee parameter failed\n", __func__);
		res = -EFAULT;
		goto error;
	}

	res = dmabuf_manage_tee_pack_u32(shm_data, pool->version, &in_len);
	if (res) {
		pr_error("%s pass tee parameter failed\n", __func__);
		res = -EFAULT;
		goto error;
	}

	res = dmabuf_manage_tee_pack_u32(shm_data, pool->id_high, &in_len);
	if (res) {
		pr_error("%s pass tee parameter failed\n", __func__);
		res = -EFAULT;
		goto error;
	}

	res = dmabuf_manage_tee_pack_u32(shm_data, pool->id_low, &in_len);
	if (res) {
		pr_error("%s pass tee parameter failed\n", __func__);
		res = -EFAULT;
		goto error;
	}

	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INOUT;
	param[0].u.memref.shm = pool->shm_pool;
	param[0].u.memref.size = in_len;
	param[0].u.memref.shm_offs = 0;
	param[3].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;

	inv_arg.func = TA_SECMEM_V2_CMD_INIT;
	inv_arg.session = pool->secmem_session.session;
	inv_arg.num_params = TEE_CMD_PARAM_NUM;
	res = tee_client_invoke_func(g_secmem_context, &inv_arg, param);
	if (res < 0 || inv_arg.ret != TEEC_SUCCESS) {
		pr_error("%s invoke func failed, res %d, res 0x%x,origin = 0x%x\n",
			__func__, res, inv_arg.ret, inv_arg.ret_origin);
		res = inv_arg.ret;
	}

error:
	return res;
}

static int dmabuf_manage_secmem_pool_init(u32 id_high, u32 id_low, u32 block_size,
	u32 *pool_addr, u32 *pool_size, u32 version)
{
	int res = 1;
	struct secure_pool_info *pool = NULL;
	uuid_t uuid = SECMEM_TA_UUID;
	unsigned int tvp_set = 0;
	unsigned int codec_flags = 0;
	unsigned int vd_index = 1;

	if (!pool_addr || !pool_size || version != SECURE_HEAP_USER_TA_VERSION)
		goto error;

	pool = kzalloc(sizeof(*pool), GFP_KERNEL);
	if (!pool)
		goto error;

	pool->version = version;
	pool->id_high = id_high;
	pool->id_low = id_low;
	pool->block_size = block_size;
	if (block_size <= DMABUF_MANAGE_BLOCK_MIN_SIZE_KB) {
		codec_flags = 0x3;
		codec_flags |= 0x8;
	} else if (block_size <= 2 * POOL_SIZE_MB) {
		tvp_set = 1;
		pool->pool_size = secure_hd_pool_size_mb * POOL_SIZE_MB;
	} else if (block_size <= 6 * POOL_SIZE_MB) {
		tvp_set = 2;
		pool->pool_size = secure_uhd_pool_size_mb * POOL_SIZE_MB;
	} else {
		tvp_set = 2;
		pool->pool_size = secure_uhd_pool_size_mb * POOL_SIZE_MB;
	}

	pool->pool_addr = 0;

	if (!g_secmem_context) {
		g_secmem_context = tee_client_open_context(NULL,
			dmabuf_manage_secmem_ctx_match, NULL, NULL);
		if (IS_ERR(g_secmem_context)) {
			pr_error("%s open context failed\n", __func__);
			goto error_alloc;
		}
	}

	memcpy(pool->secmem_session.uuid, uuid.b, TEE_IOCTL_UUID_LEN);
	pool->secmem_session.clnt_login = TEE_IOCTL_LOGIN_PUBLIC;
	pool->secmem_session.num_params = 0;

	res = tee_client_open_session(g_secmem_context, &pool->secmem_session, NULL);
	if (res < 0 || pool->secmem_session.ret != TEEC_SUCCESS) {
		pr_error("%s open session ret %d, res 0x%x, origin 0x%x\n",
			__func__, res, pool->secmem_session.ret, pool->secmem_session.ret_origin);
		res = pool->secmem_session.ret;
		goto error_alloc;
	}

	pool->shm_pool = tee_shm_alloc_kernel_buf(g_secmem_context,
		TA_SECMEM_V2_SHM_SIZE);
	if (IS_ERR(pool->shm_pool)) {
		pr_error("%s tee_shm_alloc failed\n", __func__);
		res = -ENOMEM;
		goto error_alloc_shm;
	}

	res = dmabuf_manage_secure_v2_init(pool,
		tvp_set | (codec_flags << 4) | (vd_index << 9));
	if (res) {
		pr_error("%s secure v2 init failed\n", __func__);
		goto error_open_session;
	}

	*pool_addr = pool->pool_addr;
	*pool_size = pool->pool_size;

	INIT_LIST_HEAD(&pool->block_node);
	list_add_tail(&pool->node, &pool_list);
	return 0;
error_open_session:
	tee_shm_free(pool->shm_pool);
error_alloc_shm:
	tee_client_close_session(g_secmem_context, pool->secmem_session.session);
error_alloc:
	kfree(pool);
error:
	return res;
}

static void dmabuf_manage_destroy_secmem_pool(struct secure_pool_info *pool)
{
	int res = 0;
	u32 in_len = 0;
	char *shm_data = NULL;
	struct tee_param param[TEE_CMD_PARAM_NUM] = { 0 };
	struct tee_ioctl_invoke_arg inv_arg = { 0 };

	if (!pool)
		return;

	if (list_empty(&pool->block_node)) {
		if (pool->shm_pool) {
			shm_data = tee_shm_get_va(pool->shm_pool, 0);
			if (IS_ERR(shm_data)) {
				pr_error("%s tee_shm_get_va failed\n", __func__);
				return;
			}

			res = dmabuf_manage_tee_pack_u32(shm_data, TA_SECMEM_V2_CMD_CLOSE, &in_len);
			if (res) {
				pr_error("%s pass tee parameter failed\n", __func__);
				return;
			}

			param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INOUT;
			param[0].u.memref.shm = pool->shm_pool;
			param[0].u.memref.size = in_len;
			param[0].u.memref.shm_offs = 0;
			param[3].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;

			inv_arg.func = TA_SECMEM_V2_CMD_CLOSE;
			inv_arg.session = pool->secmem_session.session;
			inv_arg.num_params = TEE_CMD_PARAM_NUM;
			res = tee_client_invoke_func(g_secmem_context, &inv_arg, param);
			if (res < 0 || inv_arg.ret != TEEC_SUCCESS) {
				pr_error("%s invoke func failed, res %d, res 0x%x, origin = 0x%x\n",
					__func__, res, inv_arg.ret, inv_arg.ret_origin);
				res = inv_arg.ret;
				return;
			}
			tee_shm_free(pool->shm_pool);
			res = tee_client_close_session(g_secmem_context,
				pool->secmem_session.session);
			if (res)
				pr_error("close secmem session error 0x%x", res);
		}

		list_del(&pool->node);
		kfree(pool);
	}
}
#endif

static int dmabuf_manage_secure_pool_init(u32 id_high, u32 id_low, u32 block_size,
	u32 *pool_addr, u32 *pool_size, u32 version)
{
#if IS_ENABLED(CONFIG_AMLOGIC_OPTEE)
	int ret = 0;

	if (version < SECURE_HEAP_USER_TA_VERSION)
		ret = dmabuf_manage_secure_genpool_init(id_high, id_low, block_size,
			pool_addr, pool_size, version);
	else
		ret = dmabuf_manage_secmem_pool_init(id_high, id_low, block_size,
			pool_addr, pool_size, version);
	return ret;
#else
	return dmabuf_manage_secure_genpool_init(id_high, id_low, block_size,
			pool_addr, pool_size, version);
#endif
}

int dmabuf_manage_secure_pool_create(u32 id_high, u32 id_low, u32 block_size,
	u32 *pool_addr, u32 *pool_size, u32 version)
{
	int res = 0;
	struct secure_pool_info *pool = NULL;

	if (!pool_addr || !pool_size)
		return 1;

	mutex_lock(&g_secure_pool_mutex);
	pool = dmabuf_manage_get_secure_vdec_pool(id_high, id_low);

	if (pool) {
		*pool_addr = pool->pool_addr;
		*pool_size = pool->pool_size;
	} else {
		res = dmabuf_manage_secure_pool_init(id_high, id_low,
			block_size, pool_addr, pool_size, version);
	}

	mutex_unlock(&g_secure_pool_mutex);
	return res;
}

static void dmabuf_manage_destroy_secure_gen_pool(struct secure_pool_info *pool)
{
	int res = 0;

	if (!pool)
		return;

	if (!pool->channel_register && list_empty(&pool->block_node)) {
		gen_pool_destroy(pool->gen_pool);

		res = tee_check_out_mem(pool->pool_addr,  pool->pool_size);
		if (res)
			pr_error("Check Out %x %x fail\n", (u32)pool->pool_addr, pool->pool_size);

		tee_unprotect_mem(pool->protect_handle);
#if IS_BUILTIN(CONFIG_AMLOGIC_MEDIA_MODULE)
		cma_mmu_op(pool->mms->mem_handle, pool->mms->page_count, 1);
#endif

		res = codec_mm_free_for_dma("dma-secure-buf", pool->pool_addr);
		if (res)
			pr_error("Free %x %x fail\n", (u32)pool->pool_addr, pool->pool_size);

		list_del(&pool->node);
		kfree(pool);
	}
}

static long dmabuf_manage_register_channel(unsigned long args)
{
	struct secure_vdec_channel *channel = NULL;
	struct secure_pool_info *pool = NULL;
	int ret = 0;
	int fd = -1;
	int fd_flags = O_CLOEXEC;
	struct dmabuf_manage_block *block = NULL;
	struct dma_buf *dbuf = NULL;

	mutex_lock(&g_secure_pool_mutex);
	pr_enter();

	channel = (struct secure_vdec_channel *)
		kzalloc(sizeof(*channel), GFP_KERNEL);
	if (!channel) {
		pr_error("kmalloc channel failed\n");
		goto error_alloc_channel;
	}

	ret = copy_from_user((void *)channel, (void __user *)args, sizeof(*channel));
	if (ret) {
		pr_error("copy channel information failed\n");
		goto error_copy;
	}

	pool = dmabuf_manage_get_secure_vdec_pool(channel->id_high, channel->id_low);
	if (pool) {
		pool->channel_register = 1;
		block = (struct dmabuf_manage_block *)
			kzalloc(sizeof(*block), GFP_KERNEL);
		if (!block) {
			pr_error("kmalloc failed\n");
			goto error_alloc_object;
		}

		block->handle = pool->protect_handle;
		block->paddr = pool->pool_addr;
		block->size = pool->block_size;
		block->type = DMA_BUF_TYPE_SECURE_VDEC;
		block->priv = (void *)channel;

		dbuf = get_dmabuf(block, fd_flags);
		if (!dbuf) {
			pr_error("get_dmabuf failed\n");
			goto error_export;
		}

		fd = dma_buf_fd(dbuf, fd_flags);
		if (fd < 0) {
			pr_error("dma_buf_fd failed\n");
			goto error_fd;
		}
	} else {
		goto error_copy;
	}

	mutex_unlock(&g_secure_pool_mutex);
	return fd;
error_fd:
	dma_buf_put(dbuf);
error_export:
	kfree(block);
error_alloc_object:
error_copy:
	kfree(channel);
error_alloc_channel:
	mutex_unlock(&g_secure_pool_mutex);
	return -EFAULT;
}

static long dmabuf_manage_release_channel(u32 id_high, u32 id_low)
{
	struct secure_pool_info *pool = NULL;
	pr_enter();

	mutex_lock(&g_secure_pool_mutex);

	pool = dmabuf_manage_get_secure_vdec_pool(id_high, id_low);
	if (pool) {
		pool->channel_register = 0;
		if (secure_heap_version == SECURE_HEAP_USER_TA_VERSION)
			dmabuf_manage_release_dmabufheap_resource(pool);
		else
			dmabuf_manage_destroy_secure_gen_pool(pool);
	}

	mutex_unlock(&g_secure_pool_mutex);
	return 0;
}

static void dmabuf_manage_dump_secure_pool(struct secure_pool_info *pool)
{
	char buf[1024] = { 0 };
	u32 size = 1024;
	u32 s = 0;
	u32 tsize = 0;
	char *pbuf = buf;
	u32 block_count = 0;
	struct list_head *pos = NULL;
	struct list_head *tmp = NULL;
	struct block_node *block = NULL;

	if (pool) {
		s = snprintf(pbuf, size - tsize,
			"Pool Ver %d %d Id %d %d Addr %x %x %x Block %x %x\n",
			pool->version, pool->channel_register,
			pool->id_high, pool->id_low, pool->protect_handle,
			(u32)pool->pool_addr, pool->pool_size, pool->block_size,
			pool->max_frame_size);
		tsize += s;
		pbuf += s;

		if (!list_empty(&pool->block_node)) {
			list_for_each_safe(pos, tmp, &pool->block_node) {
				block = list_entry(pos, struct block_node, node);
				if (block) {
					s = snprintf(pbuf, size - tsize,
						"Block %x addr %x size %x\n", block_count,
						block->addr, block->size);
					tsize += s;
					pbuf += s;
					block_count++;
				}
			}
		}

		pr_error("%s", buf);
	}
}

int dmabuf_manage_secure_pool_status(u32 id_high, u32 id_low, u32 frame_size,
	u32 *block_count, u32 *block_free_slot, u32 version)
{
	struct secure_pool_info *pool = NULL;

	if (!block_count || !block_free_slot || version == SECURE_HEAP_USER_TA_VERSION)
		return 0;

	mutex_lock(&g_secure_pool_mutex);
	*block_count = 0;
	*block_free_slot = 0;

	pool = dmabuf_manage_get_secure_vdec_pool(id_high, id_low);
	if (pool) {
		*block_count = dmabuf_manage_secure_pool_get_block_count(pool);
		*block_free_slot =
			dmabuf_manage_secure_pool_get_block_free_slot(pool, frame_size);
	}

	mutex_unlock(&g_secure_pool_mutex);
	return 0;
}

#if IS_ENABLED(CONFIG_AMLOGIC_OPTEE)
static int dmabuf_manage_secure_v2_block_alloc(struct secure_pool_info *pool, u32 size, u32 *handle)
{
	int res = 0;
	u32 in_len = 0;
	u32 out_off = 0;
	char *shm_data = NULL;
	struct tee_param param[TEE_CMD_PARAM_NUM] = { 0 };
	struct tee_ioctl_invoke_arg inv_arg = { 0 };

	if (!pool || !g_secmem_context || !handle)
		return -1;

	if (!pool->shm_pool)
		return -1;

	shm_data = tee_shm_get_va(pool->shm_pool, 0);
	if (IS_ERR(shm_data)) {
		pr_error("%s tee_shm_get_va failed\n", __func__);
		res = -EBUSY;
		goto error;
	}

	res = dmabuf_manage_tee_pack_u32(shm_data, TA_SECMEM_V2_CMD_MEM_CREATE, &in_len);
	if (res) {
		pr_error("%s pass tee parameter failed\n", __func__);
		res = -EFAULT;
		goto error;
	}

	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INOUT;
	param[0].u.memref.shm = pool->shm_pool;
	param[0].u.memref.size = in_len;
	param[0].u.memref.shm_offs = 0;
	param[3].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;

	inv_arg.func = TA_SECMEM_V2_CMD_MEM_CREATE;
	inv_arg.session = pool->secmem_session.session;
	inv_arg.num_params = TEE_CMD_PARAM_NUM;
	res = tee_client_invoke_func(g_secmem_context, &inv_arg, param);
	if (res < 0 || inv_arg.ret != TEEC_SUCCESS) {
		pr_error("%s invoke func failed, res %d, res 0x%x,origin = 0x%x\n",
			__func__, res, inv_arg.ret, inv_arg.ret_origin);
		res = inv_arg.ret;
		goto error;
	}

	res = dmabuf_manage_tee_unpack_u32(shm_data, handle, &out_off);

error:
	return res;
}

static int dmabuf_manage_secure_v2_block_free(struct secure_pool_info *pool, u32 handle)
{
	int res = 0;
	u32 in_len = 0;
	char *shm_data = NULL;
	struct tee_param param[TEE_CMD_PARAM_NUM] = { 0 };
	struct tee_ioctl_invoke_arg inv_arg = { 0 };

	if (!pool || !g_secmem_context)
		return -1;

	if (!pool->shm_pool)
		return -1;

	shm_data = tee_shm_get_va(pool->shm_pool, 0);
	if (IS_ERR(shm_data)) {
		pr_error("%s tee_shm_get_va failed\n", __func__);
		res = -EBUSY;
		goto error;
	}

	res = dmabuf_manage_tee_pack_u32(shm_data, TA_SECMEM_V2_CMD_MEM_FREE, &in_len);
	if (res) {
		pr_error("%s pass tee parameter failed\n", __func__);
		res = -EFAULT;
		goto error;
	}

	res = dmabuf_manage_tee_pack_u32(shm_data, handle, &in_len);
	if (res) {
		pr_error("%s pass tee parameter failed\n", __func__);
		res = -EFAULT;
		goto error;
	}

	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INOUT;
	param[0].u.memref.shm = pool->shm_pool;
	param[0].u.memref.size = in_len;
	param[0].u.memref.shm_offs = 0;
	param[3].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;

	inv_arg.func = TA_SECMEM_V2_CMD_MEM_FREE;
	inv_arg.session = pool->secmem_session.session;
	inv_arg.num_params = TEE_CMD_PARAM_NUM;
	res = tee_client_invoke_func(g_secmem_context, &inv_arg, param);
	if (res < 0 || inv_arg.ret != TEEC_SUCCESS) {
		pr_error("%s invoke func failed, res %d, res 0x%x, origin = 0x%x\n",
			 __func__, res, inv_arg.ret, inv_arg.ret_origin);
		res = inv_arg.ret;
	}

error:
	return res;
}
#endif

static int dmabuf_manage_release_dmabufheap_resource(struct secure_pool_info *pool)
{
#if IS_ENABLED(CONFIG_AMLOGIC_OPTEE)
	struct block_node *block = NULL;
	struct list_head *pos = NULL, *tmp = NULL;

	if (pool) {
		if (!list_empty(&pool->block_node)) {
			list_for_each_safe(pos, tmp, &pool->block_node) {
				block = list_entry(pos, struct block_node, node);
				if (block && block->addr) {
					dmabuf_manage_secure_v2_block_free(pool, block->addr);
					list_del(&block->node);
					kfree(block);
				}
			}
		}

		dmabuf_manage_destroy_secmem_pool(pool);
	}
#endif

	return 0;
}

phys_addr_t dmabuf_manage_secure_block_alloc(u32 id_high, u32 id_low, u32 size,
	u32 version)
{
	struct secure_pool_info *pool = NULL;
	phys_addr_t addr = 0;
	struct block_node *block = NULL;

	mutex_lock(&g_secure_pool_mutex);
	pool = dmabuf_manage_get_secure_vdec_pool(id_high, id_low);
	if (pool) {
#if IS_ENABLED(CONFIG_AMLOGIC_OPTEE)
		if (version == SECURE_HEAP_USER_TA_VERSION)
			dmabuf_manage_secure_v2_block_alloc(pool, size, (u32 *)&addr);
		else
			addr = gen_pool_alloc(pool->gen_pool, size);
#else
		if (version != SECURE_HEAP_USER_TA_VERSION)
			addr = gen_pool_alloc(pool->gen_pool, size);
#endif

		if (addr) {
			block = kzalloc(sizeof(*block), GFP_KERNEL);
			if (block) {
				block->addr = addr;
				block->size = size;
				list_add_tail(&block->node, &pool->block_node);
			} else {
				pr_error("No Memory for secure block\n");
			}
		} else {
			dmabuf_manage_dump_secure_pool(pool);
		}
	} else {
		pr_error("Can't found pool for %d %d\n", id_high, id_low);
	}

	mutex_unlock(&g_secure_pool_mutex);
	return addr;
}

int dmabuf_manage_secure_block_free(u32 id_high, u32 id_low, u32 release,
	phys_addr_t addr, u32 size, u32 version)
{
	struct secure_pool_info *pool = NULL;
	struct list_head *pos = NULL;
	struct list_head *tmp = NULL;
	struct block_node *block = NULL;

	mutex_lock(&g_secure_pool_mutex);
	pool = dmabuf_manage_get_secure_vdec_pool(id_high, id_low);

	if (pool) {
		if (addr && size > 0) {
#if IS_ENABLED(CONFIG_AMLOGIC_OPTEE)
			if (version == SECURE_HEAP_USER_TA_VERSION)
				dmabuf_manage_secure_v2_block_free(pool, addr);
			else
				gen_pool_free(pool->gen_pool, addr, size);
#else
			if (version != SECURE_HEAP_USER_TA_VERSION)
				gen_pool_free(pool->gen_pool, addr, size);
#endif

			if (!list_empty(&pool->block_node)) {
				list_for_each_safe(pos, tmp, &pool->block_node) {
					block = list_entry(pos, struct block_node, node);
					if (block && block->addr == addr && block->size == size) {
						list_del(&block->node);
						kfree(block);
					}
				}
			}
		}

		if (release) {
#if IS_ENABLED(CONFIG_AMLOGIC_OPTEE)
			if (version == SECURE_HEAP_USER_TA_VERSION)
				dmabuf_manage_destroy_secmem_pool(pool);
			else
				dmabuf_manage_destroy_secure_gen_pool(pool);
#else
			if (version != SECURE_HEAP_USER_TA_VERSION)
				dmabuf_manage_destroy_secure_gen_pool(pool);
#endif
		}
	}

	mutex_unlock(&g_secure_pool_mutex);
	return 0;
}

int dmabuf_manage_get_secure_heap_version(void)
{
	return secure_heap_version;
}

static int dmabuf_manage_open(struct inode *inodep, struct file *filep)
{
	pr_enter();
	return 0;
}

static ssize_t dmabuf_manage_read(struct file *filep, char *buffer,
			   size_t len, loff_t *offset)
{
	pr_enter();
	return 0;
}

static ssize_t dmabuf_manage_write(struct file *filep, const char *buffer,
				size_t len, loff_t *offset)
{
	pr_enter();
	return 0;
}

static int dmabuf_manage_release(struct inode *inodep, struct file *filep)
{
	pr_enter();
	return 0;
}

static long dmabuf_manage_ioctl(struct file *filep, unsigned int cmd,
			 unsigned long args)
{
	long ret = -EINVAL;

	pr_inf("cmd %x\n", cmd);
	switch (cmd) {
	case DMABUF_MANAGE_EXPORT_DMA:
		ret = dmabuf_manage_export(args);
		break;
	case DMABUF_MANAGE_GET_HANDLE:
		ret = dmabuf_manage_get_handle(args);
		break;
	case DMABUF_MANAGE_GET_PHYADDR:
		ret = dmabuf_manage_get_phyaddr(args);
		break;
	case DMABUF_MANAGE_IMPORT_DMA:
		ret = dmabuf_manage_import(args);
		break;
	case DMABUF_MANAGE_REGISTER_CHANNEL:
		ret = dmabuf_manage_register_channel(args);
		break;
	case DMABUF_MANAGE_VERSION:
		ret = AML_DMA_BUF_MANAGER_VERSION;
		break;
	case DMABUF_MANAGE_EXPORT_DMABUF:
		ret = dmabuf_manage_export_dmabuf(args);
		break;
	case DMABUF_MANAGE_GET_DMABUFINFO:
		ret = dmabuf_manage_get_dmabufinfo(args);
		break;
	case DMABUF_MANAGE_SET_FILTERFD:
		ret = dmabuf_manage_set_filterfd(args);
		break;
	case DMABUF_MANAGE_ALLOCDMABUF:
		ret = dmabuf_manage_alloc_dmabuf(args);
		break;
	default:
		break;
	}
	return ret;
}

#ifdef CONFIG_COMPAT
static long dmabuf_manage_compat_ioctl(struct file *filep, unsigned int cmd,
				unsigned long args)
{
	unsigned long ret;

	args = (unsigned long)compat_ptr(args);
	ret = dmabuf_manage_ioctl(filep, cmd, args);
	return ret;
}
#endif

const struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = dmabuf_manage_open,
	.read = dmabuf_manage_read,
	.write = dmabuf_manage_write,
	.release = dmabuf_manage_release,
	.unlocked_ioctl = dmabuf_manage_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = dmabuf_manage_compat_ioctl,
#endif
};

static ssize_t dmabuf_manage_dump_show(struct class *class,
				  struct class_attribute *attr, char *buf)
{
	struct list_head *pos = NULL;
	struct list_head *tmp = NULL;
	struct secure_pool_info *pool = NULL;

	mutex_lock(&g_secure_pool_mutex);

	if (!list_empty(&pool_list) && dmabuf_manage_debug >= 6) {
		list_for_each_safe(pos, tmp, &pool_list) {
			pool = list_entry(pos, struct secure_pool_info, node);
				dmabuf_manage_dump_secure_pool(pool);
		}
	}

	mutex_unlock(&g_secure_pool_mutex);
	return 0;
}

static ssize_t dmabuf_manage_config_show(struct class *class,
	struct class_attribute *attr, char *buf)
{
	ssize_t ret;

	ret = configs_list_path_nodes(CONFIG_PATH, buf, PAGE_SIZE,
					  LIST_MODE_NODE_CMDVAL_ALL);
	return ret;
}

static ssize_t dmabuf_manage_config_store(struct class *class,
			struct class_attribute *attr,
			const char *buf, size_t size)
{
	int ret;

	ret = configs_set_prefix_path_valonpath(CONFIG_PREFIX, buf);
	if (ret < 0)
		pr_err("set config failed %s\n", buf);

	return size;
}

static struct mconfig dmabuf_manage_configs[] = {
	MC_PI32("secure_vdec_def_size_bytes", &secure_vdec_def_size_bytes),
	MC_PI32("secure_hd_pool_size_mb", &secure_hd_pool_size_mb),
	MC_PI32("secure_uhd_pool_size_mb", &secure_uhd_pool_size_mb),
	MC_PI32("secure_fuhd_pool_size_mb", &secure_fuhd_pool_size_mb),
	MC_PI32("secure_pool_max_block_count", &secure_pool_max_block_count),
	MC_PI32("secure_vdec_config", &secure_vdec_config),
	MC_PI32("secure_heap_version", &secure_heap_version)
};

static CLASS_ATTR_RO(dmabuf_manage_dump);
static CLASS_ATTR_RW(dmabuf_manage_config);

static struct attribute *dmabuf_manage_class_attrs[] = {
	&class_attr_dmabuf_manage_dump.attr,
	&class_attr_dmabuf_manage_config.attr,
	NULL
};

ATTRIBUTE_GROUPS(dmabuf_manage_class);

static struct class dmabuf_manage_class = {
	.name = CLASS_NAME,
	.class_groups = dmabuf_manage_class_groups,
};

int __init dmabuf_manage_init(void)
{
	int ret;

	ret = register_chrdev(0, DEVICE_NAME, &fops);
	if (ret < 0) {
		pr_error("register_chrdev failed\n");
		goto error_register;
	}
	dev_no = ret;
	ret = class_register(&dmabuf_manage_class);
	if (ret < 0) {
		pr_error("class_register failed\n");
		goto error_class;
	}
	dmabuf_manage_dev = device_create(&dmabuf_manage_class,
				   NULL, MKDEV(dev_no, 0),
				   NULL, DEVICE_NAME);
	if (IS_ERR(dmabuf_manage_dev)) {
		pr_error("device_create failed\n");
		ret = PTR_ERR(dmabuf_manage_dev);
		goto error_create;
	}
	REG_PATH_CONFIGS(CONFIG_PATH, dmabuf_manage_configs);
	INIT_LIST_HEAD(&dmx_filter_list);
	INIT_LIST_HEAD(&pool_list);
	pr_dbg("init done\n");
	return 0;
error_create:
	class_unregister(&dmabuf_manage_class);
error_class:
	unregister_chrdev(dev_no, DEVICE_NAME);
error_register:
	return ret;
}

void __exit dmabuf_manage_exit(void)
{
	device_destroy(&dmabuf_manage_class, MKDEV(dev_no, 0));
	class_unregister(&dmabuf_manage_class);
	class_destroy(&dmabuf_manage_class);
	unregister_chrdev(dev_no, DEVICE_NAME);
	pr_dbg("exit done\n");
}

MODULE_LICENSE("GPL");
