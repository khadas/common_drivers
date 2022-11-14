/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __USER_FAULT_H
#define __USER_FAULT_H

#ifndef CONFIG_ARM64
void show_vmalloc_pfn(struct pt_regs *regs);
void show_debug_ratelimited(struct pt_regs *regs, unsigned int reg_en);
#endif

#ifdef CONFIG_AMLOGIC_USER_FAULT
void show_all_pfn(struct task_struct *task, struct pt_regs *regs);
void show_vma(struct mm_struct *mm, unsigned long addr);
void _dump_dmc_reg(void);
void show_user_fault_info(struct pt_regs *regs, u64 lr, u64 sp);
void show_extra_reg_data(struct pt_regs *regs);
void set_dump_dmc_func(void *f);

#else
static inline void show_all_pfn(struct task_struct *task, struct pt_regs *regs)
{
}

static inline void show_vma(struct mm_struct *mm, unsigned long addr)
{
}

static inline void _dump_dmc_reg(void)
{
}

static inline void show_user_fault_info(struct pt_regs *regs, u64 lr, u64 sp)
{
}

static inline void show_extra_reg_data(struct pt_regs *regs)
{
}

static inline void set_dump_dmc_func(void *f)
{
}
#endif /* CONFIG_AMLOGIC_USER_FAULT */

#endif
