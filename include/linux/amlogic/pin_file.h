/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __PIN_FILE_H__
#define __PIN_FILE_H__

struct aml_pin_file {
	spinlock_t pin_file_lock;		/* for address space */
	unsigned int start_bit;
	int cached_pages;
	struct vm_struct *root_vm;
	unsigned long *bitmap;
	struct list_head list;
	spinlock_t page_lock;		/* for cached pages */
};

/* the macro must large than enum mapping_flags in include/linux/pagemap.h */
#define AS_LOCK_MAPPING	10

struct page *aml_mlock_page_as_lock_mapping(struct vm_area_struct *vma,
	struct mm_struct *mm, struct vm_fault *vmf, unsigned long address);

void reset_page_vma_flags(struct vm_area_struct *vma, vm_flags_t flags);

int aml_is_pin_locked_file(struct page *page);

void aml_set_pin_locked_file(struct page *page);

void aml_clear_pin_locked_file(struct page *page);

extern int sysctrl_shrink_unevictable;

#endif /* __PIN_FILE_H__ */
