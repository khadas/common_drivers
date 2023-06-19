// SPDX-License-Identifier: GPL-2.0
/****************************************************************************
 *
 *    The MIT License (MIT)
 *
 *    COPYRIGHT (C) 2014 VERISILICON ALL RIGHTS RESERVED
 *
 *    Permission is hereby granted, free of charge, to any person obtaining a
 *    copy of this software and associated documentation files (the "Software"),
 *    to deal in the Software without restriction, including without limitation
 *    the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *    and/or sell copies of the Software, and to permit persons to whom the
 *    Software is furnished to do so, subject to the following conditions:
 *
 *    The above copyright notice and this permission notice shall be included in
 *    all copies or substantial portions of the Software.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *    DEALINGS IN THE SOFTWARE.
 *
 *****************************************************************************
 *
 *    The GPL License (GPL)
 *
 *    COPYRIGHT (C) 2014 VERISILICON ALL RIGHTS RESERVED
 *
 *    This program is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU General Public License
 *    as published by the Free Software Foundation; either version 2
 *    of the License, or (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software Foundation,
 *    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *****************************************************************************
 *
 *    Note: This software is released under dual MIT and GPL licenses. A
 *    recipient may use this file under the terms of either the MIT license or
 *    GPL License. If you wish to use only one license not the other, you can
 *    indicate your decision by deleting one of the above license notices in your
 *    version of this file.
 *
 *****************************************************************************
 */

#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

#include <linux/version.h>
/* Our header */
#include "memalloc.h"

#include <linux/device.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/dma-mapping.h>
#include <linux/of_platform.h>
#include <linux/of_reserved_mem.h>
#include <linux/moduleparam.h>
#include <linux/io.h>
#include <linux/kthread.h>
#include <linux/amlogic/media/codec_mm/codec_mm.h>
#include <linux/compat.h>

#ifndef HLINA_START_ADDRESS
#define HLINA_START_ADDRESS 0x02000000
#endif

#ifndef HLINA_SIZE
#define HLINA_SIZE 96
#endif

#ifndef HLINA_TRANSL_OFFSET
#define HLINA_TRANSL_OFFSET 0x0
#endif

/* the size of chunk in MEMALLOC_DYNAMIC */
#define CHUNK_SIZE (PAGE_SIZE * 4)

static struct attribute *vencmem_class_attrs[] = {
	NULL
};

ATTRIBUTE_GROUPS(vencmem_class);
#define CLASS_NAME "vencmem"
#define DEVICE_NAME "memalloc"

static struct class vencmem_class = {
	.name = CLASS_NAME,
	.class_groups = vencmem_class_groups,
};

static struct device*  device;
/* memory size in MBs for MEMALLOC_DYNAMIC */
static unsigned int alloc_size = 96;
static unsigned long alloc_base = HLINA_START_ADDRESS;

/* user space SW will subtract HLINA_TRANSL_OFFSET from the bus address
 * and decoder HW will use the result as the address translated base
 * address. The SW needs the original host memory bus address for memory
 * mapping to virtual address.
 */
static unsigned long addr_transl = HLINA_TRANSL_OFFSET;

static int memalloc_major; /* dynamic */

/* module_param(name, type, perm) */
module_param(alloc_size, uint, 0);
module_param(alloc_base, ulong, 0);
module_param(addr_transl, ulong, 0);

static DEFINE_SPINLOCK(mem_lock);

typedef struct hlinc {
    unsigned long bus_address;
    u16 chunks_reserved;
    const struct file *filp; /* Client that allocated this chunk */
} hlina_chunk;

static hlina_chunk *hlina_chunks;
static size_t chunks;

static int AllocMemory(unsigned int *busaddr, unsigned int size, const struct file *filp);
static int FreeMemory(unsigned int busaddr, const struct file *filp);
static void ResetMems(void);

static int venc_mem_mmap(struct file *filp, struct vm_area_struct *vma)
{
    int ret = 0;
    unsigned long size = (unsigned long)(vma->vm_end - vma->vm_start);

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    ret = remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, size, vma->vm_page_prot);

    return ret;
}

static long memalloc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    MemallocParams memparams;
    unsigned long busaddr;

    //spin_lock(&mem_lock);

    switch (cmd) {
    case MEMALLOC_IOCGMEMBASE:
        __put_user(alloc_base, (unsigned long __user *)arg);
        break;
    case MEMALLOC_IOCHARDRESET:
        ResetMems();
        break;
    case MEMALLOC_IOCXGETBUFFER:
        ret = copy_from_user(&memparams, (MemallocParams __user *)arg, sizeof(MemallocParams));
        if (ret)
            break;

        ret = AllocMemory(&memparams.bus_address, memparams.size, filp);

        memparams.translation_offset = addr_transl;

        ret |= copy_to_user((MemallocParams __user *)arg, &memparams, sizeof(MemallocParams));

        break;
    case MEMALLOC_IOCSFREEBUFFER:
        __get_user(busaddr, (unsigned long __user *)arg);
        ret = FreeMemory(busaddr, filp);
        break;
    default:
        break;

    }

    //spin_unlock(&mem_lock);

    return ret ? -EFAULT : 0;
}

static int memalloc_open(struct inode *inode, struct file *filp)
{
    PDEBUG("dev opened\n");
    return 0;
}

static int memalloc_release(struct inode *inode, struct file *filp)
{
    int i = 0;

    for (i = 0; i < chunks; i++) {
        spin_lock(&mem_lock);
        if (hlina_chunks[i].filp == filp) {
            pr_warn("memalloc: Found unfreed memory at release time!\n");

            hlina_chunks[i].filp = NULL;
            hlina_chunks[i].chunks_reserved = 0;
        }
        spin_unlock(&mem_lock);
    }
    PDEBUG("dev closed\n");
    return 0;
}

#ifdef CONFIG_COMPAT
static long memalloc_compat_ioctl(struct file *filp,
	unsigned int cmd, unsigned long args)
{
	unsigned long ret;

	args = (unsigned long)compat_ptr(args);
	ret = memalloc_ioctl(filp, cmd, args);

	return ret;
}
#endif

static dma_addr_t paddr = 0;
static void *vaddr = NULL;
static void memalloc_cleanup(struct platform_device *pf_dev)
{
    if (hlina_chunks)
        vfree(hlina_chunks);

    dma_free_coherent(&pf_dev->dev, alloc_size * SZ_1M, vaddr, paddr);
    unregister_chrdev(memalloc_major, "memalloc");

    if (device)
		device_destroy(&vencmem_class, MKDEV(memalloc_major, 0));

	class_destroy(&vencmem_class);
    PDEBUG("module removed\n");
}
/* VFS methods */
static struct file_operations memalloc_fops = {.owner = THIS_MODULE,
                                               .open = memalloc_open,
                                               .release = memalloc_release,
                                               .unlocked_ioctl = memalloc_ioctl,
                                               .mmap = venc_mem_mmap,
#ifdef CONFIG_COMPAT
											   .compat_ioctl = memalloc_compat_ioctl,
#endif
};

static int memalloc_init(struct platform_device *pf_dev)
{
    int result;
    int ret = 0;

    PDEBUG("module init\n");

    pr_info("memalloc: Linear Memory Allocator\n");

    pr_info("============== memalloc_init this is probe func\n");

    ret = of_reserved_mem_device_init(&pf_dev->dev);
    if (ret) {
        pr_info("reserve memory init fail:%d\n", ret);
        return ret;
    }

    vaddr = dma_alloc_coherent(&pf_dev->dev, alloc_size*SZ_1M, &paddr, GFP_KERNEL);
    //vaddr = codec_mm_vmap(paddr, 32 * SZ_1M);
    pr_info("------- vaddr: %px, paddr: %llx\n", vaddr, paddr);

    alloc_base = paddr;
    pr_info("memalloc: alloc_size = 0x%x,Linear memory base = %px\n", alloc_size,
            (void *)alloc_base);

    chunks = (alloc_size * 1024 * 1024) / CHUNK_SIZE;

    pr_info("memalloc: Total size %d MB; %d chunks of size %lu\n", alloc_size, (int)chunks,
            CHUNK_SIZE);

    hlina_chunks = vmalloc(chunks * sizeof(hlina_chunk));
    if (!hlina_chunks) {
        pr_err("memalloc: cannot allocate hlina_chunks\n");
        result = -ENOMEM;
        goto err;
    }

    result = register_chrdev(memalloc_major, "memalloc", &memalloc_fops);
    if (result < 0) {
        PDEBUG("memalloc: unable to get major %d\n", memalloc_major);
        goto err;
    } else if (result != 0) { /* this is for dynamic major */
        memalloc_major = result;
    }

    ret = class_register(&vencmem_class);
	if (ret < 0) {
        pr_info("hantro: error create venc class!");
		return ret;
	}

    device = device_create(&vencmem_class, NULL, MKDEV(memalloc_major, 0), NULL, DEVICE_NAME);
    if (IS_ERR(device)) {
        class_destroy(&vencmem_class);
        unregister_chrdev(memalloc_major, DEVICE_NAME);
        ret = PTR_ERR(device);
		goto err;
    }

    ResetMems();

    return 0;

err:
    if (hlina_chunks)
        vfree(hlina_chunks);

    return result;
}

/* Cycle through the buffers we have, give the first free one */
static int AllocMemory(unsigned int *busaddr, unsigned int size, const struct file *filp)
{
    int i = 0;
    int j = 0;
    unsigned int skip_chunks = 0;

    /* calculate how many chunks we need; round up to chunk boundary */
    unsigned int alloc_chunks = (size + CHUNK_SIZE - 1) / CHUNK_SIZE;

    *busaddr = 0;

    /* run through the chunk table */
    for (i = 0; i < chunks;) {
        skip_chunks = 0;
        /* if this chunk is available */
        if (!hlina_chunks[i].chunks_reserved) {
            /* check that there is enough memory left */
            if (i + alloc_chunks > chunks)
                break;

            /* check that there is enough consecutive chunks available */
            for (j = i; j < i + alloc_chunks; j++) {
                if (hlina_chunks[j].chunks_reserved) {
                    skip_chunks = 1;
                    /* skip the used chunks */
                    i = j + hlina_chunks[j].chunks_reserved;
                    break;
                }
            }

            /* if enough free memory found */
            if (!skip_chunks) {
                *busaddr = hlina_chunks[i].bus_address;
                hlina_chunks[i].filp = filp;
                hlina_chunks[i].chunks_reserved = alloc_chunks;
                break;
            }
        } else {
            /* skip the used chunks */
            i += hlina_chunks[i].chunks_reserved;
        }
    }

    if (*busaddr == 0) {
        pr_info("memalloc: Allocation FAILED: size = %d\n", size);
        return -EFAULT;
    } else {
        PDEBUG("MEMALLOC OK: size: %d, reserved: %ld\n", size, alloc_chunks * CHUNK_SIZE);
    }

    return 0;
}

/* Free a buffer based on bus address */
static int FreeMemory(unsigned int busaddr, const struct file *filp)
{
    int i = 0;

    for (i = 0; i < chunks; i++) {
        /* user space SW has stored the translated bus address, add addr_transl to
		 * translate back to our address space
		 */
        if (hlina_chunks[i].bus_address == busaddr + addr_transl) {
            if (hlina_chunks[i].filp == filp) {
                hlina_chunks[i].filp = NULL;
                hlina_chunks[i].chunks_reserved = 0;
            } else {
                pr_warn("memalloc: Owner mismatch while freeing memory!\n");
            }
            break;
        }
    }
    return 0;
}

/* Reset "used" status */
static void ResetMems(void)
{
    int i = 0;
    unsigned long ba = alloc_base;

    for (i = 0; i < chunks; i++) {
        hlina_chunks[i].bus_address = ba;
        hlina_chunks[i].filp = NULL;
        hlina_chunks[i].chunks_reserved = 0;

        ba += CHUNK_SIZE;
    }
}

static int encmem_vce_probe(struct platform_device *pf_dev)
{
    pr_info("encmem_vce_probe\n");
    memalloc_init(pf_dev);
    return 0;
}

static int encmem_vce_remove(struct platform_device *pf_dev)
{
    pr_info("encmem_vce_remove:\n");
    memalloc_cleanup(pf_dev);
    return 0;
}

static const struct of_device_id amlogic_venc_mem_match[] = {{
                                                                 .compatible = "encmem_rev",
                                                             },
                                                             {}};

static struct platform_driver venc_mem_driver = {.probe = encmem_vce_probe,
                                                 .remove = encmem_vce_remove,
                                                 .driver = {
                                                     .name = "encmem_rev",
                                                     .owner = THIS_MODULE,
                                                     .of_match_table = amlogic_venc_mem_match,
                                                 }};

int __init enc_memallc_init(void)
{
    pr_info("enc_mem_init: enc_mem_init\n");
    platform_driver_register(&venc_mem_driver);
    return 0;
}

void __exit enc_memallc_exit(void)
{
    pr_info("enc_mem_init: enc_mem_exit\n");
    platform_driver_unregister(&venc_mem_driver);
}

module_init(enc_memallc_init);
module_exit(enc_memallc_exit);

/* module description */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amlogic Inc.");
MODULE_DESCRIPTION("Linear RAM allocation");
