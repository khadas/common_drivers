// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/version.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/reboot.h>
#include <linux/memblock.h>
#include <linux/vmalloc.h>
#include <linux/arm-smccc.h>
#include <linux/memcontrol.h>
#include <linux/kthread.h>
#include <linux/mm.h>
#include <linux/rmap.h>
#include <linux/spinlock.h>
#include <internal.h>
#include <linux/amlogic/pin_file.h>

static struct aml_pin_file *aml_pin;

extern void lru_add_drain(void);

struct page *aml_mlock_page_as_lock_mapping(struct vm_area_struct *vma,
	struct mm_struct *mm, struct vm_fault *vmf, unsigned long address)
{
	struct address_space *mapping = NULL;

	/* Android lock it but not access it */
	if (vma->vm_file && !(vma->vm_flags & VM_LOCKED))
		mapping = vma->vm_file->f_mapping;
	if (mapping && test_bit(AS_LOCK_MAPPING, &mapping->flags)) {
		struct page *page;
		spinlock_t *ptl; /* page table lock */
		pte_t *ptep, pte;
		int need_restore = 0;

		ptep = pte_offset_map_lock(mm, vmf->pmd, address, &ptl);
		pte  = *ptep;
		page = vm_normal_page(vmf->vma, address, pte);
		if (page && !PageMlocked(page)) {
			if (page->mapping && trylock_page(page)) {
				pr_debug("fault on locked, new pte:%llx, addr:%lx, page:%lx, %lx, mapping:%px f:%lx\n",
					(unsigned long long)pte_val(pte),
					address, page_to_pfn(page),
					page->flags,
					mapping, mapping->flags);
				lru_add_drain();  /* push cached pages to LRU */
				mlock_vma_page(page);
				/* Set this flag to avoid shrink again */
				/* SetPageCmaAllocating(page); */
				aml_set_pin_locked_file(page);
				unlock_page(page);
				need_restore = 1;
			}
		}
		pte_unmap_unlock(ptep, ptl);
		if (need_restore)
			return page;
	}

	return NULL;
}

void reset_page_vma_flags(struct vm_area_struct *vma, vm_flags_t flags)
{
	if (vma->vm_file && vma->vm_file->f_mapping) {
		struct inode *host;

		host = vma->vm_file->f_mapping->host;
		if ((flags & (VM_LOCKED | VM_LOCKONFAULT))) {
			set_bit(AS_LOCK_MAPPING,
					&vma->vm_file->f_mapping->flags);
			atomic_inc(&host->i_count);
		} else if (test_bit(AS_LOCK_MAPPING,
					&vma->vm_file->f_mapping->flags)){
			atomic_dec(&host->i_count);
			clear_bit(AS_LOCK_MAPPING,
					&vma->vm_file->f_mapping->flags);
		}
		pr_debug("%s lock mapping:%px, f:%lx\n",
				flags & (VM_LOCKED | VM_LOCKONFAULT) ?
				"Mark" : "Clear",
				vma->vm_file->f_mapping,
				vma->vm_file->f_mapping->flags);
	}
}

int sysctrl_shrink_unevictable = 1;
static struct ctl_table shrink_unevictable[] = {
	{
		.procname	= "shrink_unevictable",
		.data		= &sysctrl_shrink_unevictable,
		.maxlen		= sizeof(sysctrl_shrink_unevictable),
		.mode		= 0644,
		.proc_handler	= proc_dointvec_minmax,
		.extra1		= SYSCTL_ZERO,
		.extra2		= SYSCTL_ONE,
		},
	{ }
};

static int __init aml_gup_init(void)
{
	register_sysctl("vm", shrink_unevictable);
	return 0;
}
fs_initcall(aml_gup_init);

int aml_is_pin_locked_file(struct page *page)
{
	unsigned long pfn;

	pfn = page_to_pfn(page);
	return test_bit(pfn, aml_pin->bitmap);
}

void aml_set_pin_locked_file(struct page *page)
{
	unsigned long flags;
	unsigned long pfn;

	pfn = page_to_pfn(page);
	spin_lock_irqsave(&aml_pin->pin_file_lock, flags);
	bitmap_set(aml_pin->bitmap, pfn, 1);
	spin_unlock_irqrestore(&aml_pin->pin_file_lock, flags);
}

void aml_clear_pin_locked_file(struct page *page)
{
	unsigned long flags;
	unsigned long pfn;

	pfn = page_to_pfn(page);
	spin_lock_irqsave(&aml_pin->pin_file_lock, flags);
	bitmap_clear(aml_pin->bitmap, pfn, 1);
	spin_unlock_irqrestore(&aml_pin->pin_file_lock, flags);
}

/* borrow the poking_init function, which comes from init/main.c */
void __init poking_init(void)
{
	struct zone *zone;
	unsigned long total_pages = 0;

	for_each_populated_zone(zone)
		total_pages += zone->spanned_pages;

	aml_pin = kzalloc(sizeof(*aml_pin), GFP_KERNEL);
	if (!aml_pin)
		return;

	aml_pin->bitmap = kzalloc(total_pages / 8, GFP_KERNEL);
	if (!aml_pin->bitmap) {
		kfree(aml_pin);
		return;
	}

	spin_lock_init(&aml_pin->pin_file_lock);
}

