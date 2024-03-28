// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/compat.h>
#include <linux/efi.h>
#include <linux/elf.h>
#include <linux/export.h>
#include <linux/sched.h>
#include <linux/sched/debug.h>
#include <linux/sched/task.h>
#include <linux/sched/task_stack.h>
#include <linux/kernel.h>
#include <linux/mman.h>
#include <linux/mm.h>
#include <linux/nospec.h>
#include <linux/stddef.h>
#include <linux/sysctl.h>
#include <linux/unistd.h>
#include <linux/user.h>
#include <linux/delay.h>
#include <linux/reboot.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/cpu.h>
#include <linux/elfcore.h>
#include <linux/pm.h>
#include <linux/tick.h>
#include <linux/utsname.h>
#include <linux/uaccess.h>
#include <linux/random.h>
#include <linux/hw_breakpoint.h>
#include <linux/personality.h>
#include <linux/notifier.h>
#include <trace/events/power.h>
#include <linux/percpu.h>
#include <linux/thread_info.h>
#include <linux/prctl.h>
#include <trace/hooks/fpsimd.h>
#include <trace/hooks/mpam.h>
#include <linux/mmap_lock.h>

#include <asm/compat.h>
#ifndef CONFIG_RISCV
#include <asm/cpufeature.h>
#endif
#include <asm/cacheflush.h>
#include <asm/exec.h>
#include <asm/mmu_context.h>
#include <asm/processor.h>
#include <asm/stacktrace.h>
#include <asm/switch_to.h>
#ifndef CONFIG_RISCV
#include <asm/system_misc.h>
#endif

#include <linux/mm_inline.h>
#include <linux/amlogic/user_fault.h>
#if IS_ENABLED(CONFIG_AMLOGIC_SECMON)
#include <linux/amlogic/secmon.h>
#endif

#include "../memory_debug/ddr_tool/dmc_monitor.h"

#ifndef CONFIG_RISCV
#ifndef CONFIG_ARM64
#include <asm/ptrace.h>
#include <asm/vdso/cp15.h>
#include <linux/ratelimit.h>

#define USR_FAULT_DBG_RATELIMIT_INTERVAL	(5 * HZ)
#define USR_FAULT_DBG_RATELIMIT_BURST		3

#define user_fault_debug_ratelimited()					\
({									\
	static DEFINE_RATELIMIT_STATE(usr_fault_dgb_rs,			\
				      USR_FAULT_DBG_RATELIMIT_INTERVAL,	\
				      USR_FAULT_DBG_RATELIMIT_BURST);	\
	bool __show_ratelimited = false;				\
	if (__ratelimit(&usr_fault_dgb_rs))				\
		__show_ratelimited = true;				\
	__show_ratelimited;						\
})
#endif
#endif

static int show_kernel_data_valid(unsigned long reg)
{
	struct page *page;

	if (reg < (unsigned long)PAGE_OFFSET)
		return 0;
	else if (reg <= (unsigned long)high_memory)
		return 1;

	page = vmalloc_to_page((const void *)reg);
	if (page && pfn_valid(page_to_pfn(page)))
		return 1;

	return 0;
}

static void show_data(unsigned long addr, int nbytes, const char *name)
{
	int	i, j;
	int	nlines;
	u32	*p;
	char	buf[128] = {0};
	int	len = 0;

	/*
	 * don't attempt to dump non-kernel addresses or
	 * values that are probably just small negative numbers
	 */
#ifdef CONFIG_RISCV
	if (addr < VMALLOC_START || addr > -256UL)
#else
	if (addr < PAGE_OFFSET || addr > -256UL)
#endif
		return;
	/*
	 * Treating data in general purpose register as an address
	 * and dereferencing it is quite a dangerous behaviour,
	 * especially when it belongs to secure monotor region or
	 * ioremap region(for arm64 vmalloc region is already filtered
	 * out), which can lead to external abort on non-linefetch and
	 * can not be protected by probe_kernel_address.
	 * We need more strict filtering rules
	 */

	if (!show_kernel_data_valid((unsigned long)(addr + nbytes / 2)))
		return;

#if IS_ENABLED(CONFIG_AMLOGIC_SECMON)
	/*
	 * filter out secure monitor region
	 */
	if (within_secmon_region(addr + nbytes / 2)) {
		pr_emerg("\n%s: %#lx S\n", name, addr + nbytes / 2);
		return;
	}
#endif

	printk("\n%s: %#lx:\n", name, addr);

	/*
	 * round address down to a 32 bit boundary
	 * and always dump a multiple of 32 bytes
	 */
	p = (u32 *)(addr & ~(sizeof(u32) - 1));
	nbytes += (addr & (sizeof(u32) - 1));
	nlines = (nbytes + 31) / 32;

	for (i = 0; i < nlines; i++) {
		/*
		 * just display low 16 bits of address to keep
		 * each line of the dump < 80 characters
		 */
		len = 0;
		len += snprintf(buf + len, sizeof(buf) - len, "%04lx ", (unsigned long)p & 0xffff);
		for (j = 0; j < 8; j++) {
			u32 data = 0;

			if (get_kernel_nofault(data, p))
				len += snprintf(buf + len, sizeof(buf) - len, " ********");
			else
				len += snprintf(buf + len, sizeof(buf) - len, " %08x", data);
			++p;
		}
		pr_info("%s\n", buf);
	}
}

/*
 * dump a block of user memory from around the given address
 */
static void show_user_data(unsigned long addr, int nbytes, const char *name)
{
	int	i, j;
	int	nlines;
	u32	*p;
	char	buf[128] = {0};
	int	len = 0;

	if (!access_ok((void *)addr, nbytes))
		return;

	pr_info("\n%s: %#lx:\n", name, addr);

	/*
	 * round address down to a 32 bit boundary
	 * and always dump a multiple of 32 bytes
	 */
	p = (u32 *)(addr & ~(sizeof(u32) - 1));
	nbytes += (addr & (sizeof(u32) - 1));
	nlines = (nbytes + 31) / 32;

	for (i = 0; i < nlines; i++) {
		/*
		 * just display low 16 bits of address to keep
		 * each line of the dump < 80 characters
		 */
		len = 0;
		len += snprintf(buf + len, sizeof(buf) - len,
						"%04lx ", (unsigned long)p & 0xffff);
		for (j = 0; j < 8; j++) {
			int bad;
			unsigned char data[4];

			bad = __copy_from_user_inatomic(data, (const void *)p, sizeof(u32));
			if (bad)
				len += snprintf(buf + len, sizeof(buf) - len, " ********");
			else
				len += snprintf(buf + len, sizeof(buf) - len,
								" %08x", *(u32 *)data);
			++p;
		}
		pr_info("%s\n", buf);
	}
}

#ifdef CONFIG_ARM64
static void show_pfn(unsigned long reg, char *s)
{
	struct page *page;

	if (reg < (unsigned long)PAGE_OFFSET) {
		pr_info("%s : %016lx  U\n", s, reg);
	} else if (reg <= (unsigned long)high_memory) {
		pr_info("%s : %016lx, PFN:%5lx L\n", s, reg, virt_to_pfn(reg));
	} else {
		page = vmalloc_to_page((const void *)reg);
		if (page)
			pr_info("%s : %016lx, PFN:%5lx V\n", s, reg, page_to_pfn(page));
		else
			pr_info("%s : %016lx, PFN:***** V\n", s, reg);
	}
}

static void show_regs_pfn(struct pt_regs *regs)
{
	int i;
	struct page *page;

	for (i = 0; i < 31; i++) {
		if (regs->regs[i] < (unsigned long)PAGE_OFFSET) {
			continue;
		} else if (regs->regs[i] <= (unsigned long)high_memory) {
#if IS_MODULE(CONFIG_AMLOGIC_PAGE_TRACE)
			page = virt_to_page((void *)regs->regs[i]);
			if (page)
				pr_info("R%-2d : %016llx, PFN:%5lx L, f: %ps\n",
					i, regs->regs[i], virt_to_pfn((void *)regs->regs[i]),
					(void *)dmc_get_page_trace(page));
			else
				pr_info("R%-2d : %016llx, PFN:%5lx L\n",
					i, regs->regs[i], virt_to_pfn((void *)regs->regs[i]));
		} else {
			page = vmalloc_to_page((void *)regs->regs[i]);
			if (page)
				pr_info("R%-2d : %016llx, PFN:%5lx V, f: %ps\n",
					i, regs->regs[i], page_to_pfn(page),
					(void *)dmc_get_page_trace(page));
			else
				pr_info("R%-2d : %016llx, PFN:***** V\n", i, regs->regs[i]);
		}
#else
			pr_info("R%-2d : %016llx, PFN:%5lx L\n",
				i, regs->regs[i], virt_to_pfn((void *)regs->regs[i]));
		} else {
			page = vmalloc_to_page((void *)regs->regs[i]);
			if (page)
				pr_info("R%-2d : %016llx, PFN:%5lx V\n",
					i, regs->regs[i], page_to_pfn(page));
			else
				pr_info("R%-2d : %016llx, PFN:***** V\n", i, regs->regs[i]);
		}
#endif
	}
}

#elif defined CONFIG_RISCV
static void show_pfn(unsigned long reg, const char *s)
{
	struct page *page;

	if (reg < (unsigned long)VMALLOC_START) {
		pr_info("%s : %016lx  U\n", s, reg);
	} else if (reg <= (unsigned long)PAGE_OFFSET) {
		page = vmalloc_to_page((const void *)reg);
		if (page)
			pr_info("%s : %016lx, PFN:%5lx V\n", s, reg, page_to_pfn(page));
		else
			pr_info("%s : %016lx, PFN:***** V\n", s, reg);
	} else if (reg <= (unsigned long)high_memory) {
		pr_info("%s : %016lx, PFN:%5lx L\n", s, reg, virt_to_pfn(reg));
	} else if (reg <= KERNEL_LINK_ADDR) {
	} else if (reg <= kernel_map.virt_addr + kernel_map.size) {
		pr_info("%s : %016lx, PFN:%5lx L\n", s, reg, virt_to_pfn(reg));
	}
}

static const char * const regs_name[] = {
	"epc", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0", "s1",
	"a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "s2", "s3",
	"s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "t3", "t4",
	"t5", "t6"
};

static void show_regs_pfn(struct pt_regs *regs)
{
	int i;
	struct page *page;
	unsigned long *reg_array = (unsigned long *)regs;

	for (i = 0; i < 32; i++) {
		if (reg_array[i] < (unsigned long)VMALLOC_START) {
			continue;
		} else if (reg_array[i] <= (unsigned long)PAGE_OFFSET) {
			page = vmalloc_to_page((void *)reg_array[i]);
			if (page)
				pr_info("%-3s : %016lx, PFN:%5lx V\n",
					regs_name[i], reg_array[i], page_to_pfn(page));
			else
				pr_info("%-3s : %016lx, PFN:***** V\n",
					regs_name[i], reg_array[i]);
		} else if (reg_array[i] <= (unsigned long)high_memory) {
			pr_info("%-3s : %016lx, PFN:%5lx L\n",
				regs_name[i], reg_array[i], virt_to_pfn((void *)reg_array[i]));
		} else if (reg_array[i] <= KERNEL_LINK_ADDR) {
		} else if (reg_array[i] <= kernel_map.virt_addr + kernel_map.size) {
			pr_info("%-3s : %016lx, PFN:%5lx L\n",
				regs_name[i], reg_array[i], virt_to_pfn((void *)reg_array[i]));
		}
	}
}
#endif /* CONFIG_ARM64 */

#ifdef CONFIG_ARM64
static void show_extra_register_data(struct pt_regs *regs, int nbytes)
{
	unsigned int i;

	show_data(regs->pc - nbytes, nbytes * 2, "PC");
	show_data(regs->regs[30] - nbytes, nbytes * 2, "LR");
	show_data(regs->sp - nbytes, nbytes * 2, "SP");
	show_data(read_sysreg(far_el1), nbytes * 2, "FAR");
	for (i = 0; i < 30; i++) {
		char name[4];

		snprintf(name, sizeof(name), "X%u", i);
		show_data(regs->regs[i] - nbytes, nbytes * 2, name);
	}
}

static void show_user_extra_register_data(struct pt_regs *regs, int nbytes)
{
	unsigned int i, top_reg;
	u64 sp, lr;

	if (compat_user_mode(regs)) {
		lr = regs->compat_lr;
		sp = regs->compat_sp;
		top_reg = 13;
	} else {
		lr = regs->regs[30];
		sp = regs->sp;
		top_reg = 29;
	}

	show_user_data(regs->pc - nbytes, nbytes * 2, "PC");
	show_user_data(lr - nbytes, nbytes * 2, "LR");
	show_user_data(sp - nbytes, nbytes * 2, "SP");
	show_user_data(read_sysreg(far_el1), nbytes * 2, "FAR");
	for (i = 0; i < top_reg; i++) {
		char name[4];

		snprintf(name, sizeof(name), "r%u", i);
		show_user_data(regs->regs[i] - nbytes, nbytes * 2, name);
	}
}
#elif defined CONFIG_ARM
static void show_extra_register_data(struct pt_regs *regs, int nbytes)
{
	show_data(regs->ARM_pc - nbytes, nbytes * 2, "PC");
	show_data(regs->ARM_lr - nbytes, nbytes * 2, "LR");
	show_data(regs->ARM_sp - nbytes, nbytes * 2, "SP");
	show_data(regs->ARM_ip - nbytes, nbytes * 2, "IP");
	show_data(regs->ARM_fp - nbytes, nbytes * 2, "FP");
	show_data(regs->ARM_r0 - nbytes, nbytes * 2, "R0");
	show_data(regs->ARM_r1 - nbytes, nbytes * 2, "R1");
	show_data(regs->ARM_r2 - nbytes, nbytes * 2, "R2");
	show_data(regs->ARM_r3 - nbytes, nbytes * 2, "R3");
	show_data(regs->ARM_r4 - nbytes, nbytes * 2, "R4");
	show_data(regs->ARM_r5 - nbytes, nbytes * 2, "R5");
	show_data(regs->ARM_r6 - nbytes, nbytes * 2, "R6");
	show_data(regs->ARM_r7 - nbytes, nbytes * 2, "R7");
	show_data(regs->ARM_r8 - nbytes, nbytes * 2, "R8");
	show_data(regs->ARM_r9 - nbytes, nbytes * 2, "R9");
	show_data(regs->ARM_r10 - nbytes, nbytes * 2, "R10");
}

static void show_user_extra_register_data(struct pt_regs *regs, int nbytes)
{
	show_user_data(regs->ARM_pc - nbytes, nbytes * 2, "PC");
	show_user_data(regs->ARM_lr - nbytes, nbytes * 2, "LR");
	show_user_data(regs->ARM_sp - nbytes, nbytes * 2, "SP");
	show_user_data(regs->ARM_ip - nbytes, nbytes * 2, "IP");
	show_user_data(regs->ARM_fp - nbytes, nbytes * 2, "FP");
	show_user_data(regs->ARM_r0 - nbytes, nbytes * 2, "R0");
	show_user_data(regs->ARM_r1 - nbytes, nbytes * 2, "R1");
	show_user_data(regs->ARM_r2 - nbytes, nbytes * 2, "R2");
	show_user_data(regs->ARM_r3 - nbytes, nbytes * 2, "R3");
	show_user_data(regs->ARM_r4 - nbytes, nbytes * 2, "R4");
	show_user_data(regs->ARM_r5 - nbytes, nbytes * 2, "R5");
	show_user_data(regs->ARM_r6 - nbytes, nbytes * 2, "R6");
	show_user_data(regs->ARM_r7 - nbytes, nbytes * 2, "R7");
	show_user_data(regs->ARM_r8 - nbytes, nbytes * 2, "R8");
	show_user_data(regs->ARM_r9 - nbytes, nbytes * 2, "R9");
	show_user_data(regs->ARM_r10 - nbytes, nbytes * 2, "R10");
}

void show_vmalloc_pfn(struct pt_regs *regs)
{
	int i;
	struct page *page;

	for (i = 0; i < 16; i++) {
		if (is_vmalloc_or_module_addr((void *)regs->uregs[i])) {
			page = vmalloc_to_page((void *)regs->uregs[i]);
			if (!page)
				continue;
			pr_info("R%-2d : %08lx, PFN:%5lx\n",
				i, regs->uregs[i], page_to_pfn(page));
		}
	}
}
#elif defined CONFIG_RISCV
static void show_extra_register_data(struct pt_regs *regs, int nbytes)
{
	int i;
	unsigned long *reg_array = (unsigned long *)regs;

	show_data(regs->badaddr - nbytes, nbytes * 2, "badaddr");

	for (i = 0; i < 32; i++)
		show_data(reg_array[i] - nbytes, nbytes * 2, regs_name[i]);
}

static void show_user_extra_register_data(struct pt_regs *regs, int nbytes)
{
	int i;
	unsigned long *reg_array = (unsigned long *)regs;

	show_user_data(regs->badaddr - nbytes, nbytes * 2, "badaddr");

	for (i = 0; i < 32; i++)
		show_user_data(reg_array[i] - nbytes, nbytes * 2, regs_name[i]);
}
#endif

#if IS_MODULE(CONFIG_AMLOGIC_USER_FAULT)
__weak const char *arch_vma_name(struct vm_area_struct *vma)
{
	return NULL;
}
#endif

/* Check if the vma is being used as a stack by this task */
static int aml_vma_is_stack_for_current(struct vm_area_struct *vma)
{
	struct task_struct * __maybe_unused t = current;

	return (vma->vm_start <= KSTK_ESP(t) && vma->vm_end >= KSTK_ESP(t));
}

static struct anon_vma_name *aml_anon_vma_name(struct vm_area_struct *vma)
{
	mmap_assert_locked(vma->vm_mm);

	if (vma->vm_file)
		return NULL;

	return vma->anon_name;
}

void show_vma(struct mm_struct *mm, unsigned long addr)
{
	struct vm_area_struct *vma;
	struct file *file;
	vm_flags_t flags;
	unsigned long ino = 0;
	unsigned long long pgoff = 0;
	unsigned long start, end;
	dev_t dev = 0;
	const char *name = NULL;

	vma = find_vma(mm, addr);
	if (!vma) {
		pr_info("can't find vma for %lx\n", addr);
		return;
	}

	file = vma->vm_file;
	flags = vma->vm_flags;
	if (file) {
		struct inode *inode = file_inode(vma->vm_file);

		dev = inode->i_sb->s_dev;
		ino = inode->i_ino;
		pgoff = ((loff_t)vma->vm_pgoff) << PAGE_SHIFT;
	}

	/* We don't show the stack guard page in /proc/maps */
	start = vma->vm_start;
	end = vma->vm_end;

	pr_info("vma for %lx:\n", addr);
	pr_info("%08lx-%08lx %c%c%c%c %08llx %02x:%02x %lu ",
		start,
		end,
		flags & VM_READ ? 'r' : '-',
		flags & VM_WRITE ? 'w' : '-',
		flags & VM_EXEC ? 'x' : '-',
		flags & VM_MAYSHARE ? 's' : 'p',
		pgoff,
		MAJOR(dev), MINOR(dev), ino);

	/*
	 * Print the dentry name for named mappings, and a
	 * special [heap] marker for the heap:
	 */
	if (file) {
		char file_name[256] = {};
		char *p = d_path(&file->f_path, file_name, 256);

		if (!IS_ERR(p)) {
			mangle_path(file_name, p, "\n");
			pr_info("%s", p);
		} else {
			pr_info(" get file path failed\n");
		}
		goto done;
	}

	name = arch_vma_name(vma);
	if (!name) {
		pid_t tid;

		if (!mm) {
			name = "[vdso]";
			goto done;
		}

		if (vma->vm_start <= mm->brk &&
		    vma->vm_end >= mm->start_brk) {
			name = "[heap]";
			goto done;
		}

		tid = aml_vma_is_stack_for_current(vma);

		if (tid != 0) {
			/*
			 * Thread stack in /proc/PID/task/TID/maps or
			 * the main process stack.
			 */
			if ((vma->vm_start <= mm->start_stack &&
			    vma->vm_end >= mm->start_stack)) {
				name = "[stack]";
			} else {
				/* Thread stack in /proc/PID/maps */
				pr_info("[stack:%d]", tid);
			}
			goto done;
		}

		if (aml_anon_vma_name(vma))
			pr_info("[anon]");
	}

done:
	if (name)
		pr_info("%s", name);
	pr_info("\n");
}

#if IS_MODULE(CONFIG_AMLOGIC_USER_FAULT) && IS_ENABLED(CONFIG_KALLSYMS_ALL)
struct mm_struct *aml_init_mm;

unsigned long (*aml_syms_lookup)(const char *name);

/* For each probe you need to allocate a kprobe structure */
static struct kprobe kp_lookup_name = {
	.symbol_name	= "kallsyms_lookup_name",
};
#endif

static long __nocfi get_user_pfn(struct mm_struct *mm, unsigned long addr)
{
	long pfn = -1;
	pgd_t *pgd;

#if IS_MODULE(CONFIG_AMLOGIC_USER_FAULT) && IS_ENABLED(CONFIG_KALLSYMS_ALL)
	if (!mm || addr >= VMALLOC_START)
		mm = aml_init_mm;
	if (!mm)
		return pfn;
#elif IS_MODULE(CONFIG_AMLOGIC_USER_FAULT) && !IS_ENABLED(CONFIG_KALLSYMS_ALL)
	return pfn;
#else
	if (!mm || addr >= VMALLOC_START)
		mm = &init_mm;
#endif

	pgd = pgd_offset(mm, addr);

	do {
		p4d_t *p4d;
		pud_t *pud;
		pmd_t *pmd;
		pte_t *pte;

		if (pgd_none(*pgd) || pgd_bad(*pgd))
			break;

		p4d = p4d_offset(pgd, addr);
		if (p4d_none(*p4d) || p4d_bad(*p4d))
			break;

		pud = pud_offset(p4d, addr);
		if (pud_none(*pud) || pud_bad(*pud))
			break;

		pmd = pmd_offset(pud, addr);
		if (pmd_none(*pmd) || pmd_bad(*pmd))
			break;

		pte = pte_offset_map(pmd, addr);
		pfn = pte_pfn(*pte);
		pte_unmap(pte);
	} while (0);

	return pfn;
}

#ifdef CONFIG_ARM64
void show_all_pfn(struct task_struct *task, struct pt_regs *regs)
{
	int i;
	long pfn1, far;
	char s1[10];
	int top;
	char buf[128] = {0};
	int len = 0;

	if (compat_user_mode(regs))
		top = 15;
	else
		top = 31;

	len += snprintf(buf + len, sizeof(buf) - len, "reg              value       pfn  ");
	len += snprintf(buf + len, sizeof(buf) - len, "reg              value       pfn");
	pr_info("%s\n", buf);

	len = 0;
	memset(buf, 0, sizeof(buf));

	for (i = 0; i < top; i++) {
		pfn1 = get_user_pfn(task->mm, regs->regs[i]);
		if (pfn1 >= 0)
			sprintf(s1, "%8lx", pfn1);
		else
			sprintf(s1, "--------");
		if (i % 2 == 1) {
			len += snprintf(buf + len, sizeof(buf) - len,
					"r%-2d:  %016llx  %s", i, regs->regs[i], s1);
			pr_info("%s\n", buf);
			len = 0;
		} else {
			len += snprintf(buf + len, sizeof(buf) - len,
					"r%-2d:  %016llx  %s  ", i, regs->regs[i], s1);
		}
	}

	if (len > 0)
		pr_info("%s\n", buf);

	pfn1 = get_user_pfn(task->mm, regs->pc);
	if (pfn1 >= 0)
		sprintf(s1, "%8lx", pfn1);
	else
		sprintf(s1, "--------");
	pr_info("pc :  %016llx  %s\n", regs->pc, s1);
	pfn1 = get_user_pfn(task->mm, regs->sp);
	if (pfn1 >= 0)
		sprintf(s1, "%8lx", pfn1);
	else
		sprintf(s1, "--------");
	pr_info("sp :  %016llx  %s\n", regs->sp, s1);

	far = read_sysreg(far_el1);
	pfn1 = get_user_pfn(task->mm, far);
	if (pfn1 >= 0)
		sprintf(s1, "%8lx", pfn1);
	else
		sprintf(s1, "--------");
	pr_info("unused :  %016lx  %s\n", far, s1);
#if IS_BUILTIN(CONFIG_AMLOGIC_USER_FAULT)
	pr_info("offset :  %016lx\n", kaslr_offset());
#endif
}
#elif defined CONFIG_ARM
static unsigned char *regidx_to_name[] = {
	"r0 ", "r1 ", "r2 ", "r3 ",
	"r4 ", "r5 ", "r6 ", "r7 ",
	"r8 ", "r9 ", "r10",
	"fp ", "ip ", "sp ", "lr ",
	"pc "
};

void show_all_pfn(struct task_struct *task, struct pt_regs *regs)
{
	int i;
	long pfn1;
	char s1[10];
	int top;
	char buf[128] = {0};
	int len = 0;

	top = 16;
	len += snprintf(buf + len, sizeof(buf) - len, "reg     value       pfn   ");
	len += snprintf(buf + len, sizeof(buf) - len, "reg     value       pfn");
	pr_info("%s\n", buf);

	len = 0;
	memset(buf, 0, sizeof(buf));

	for (i = 0; i < top; i++) {
		pfn1 = get_user_pfn(task->mm, regs->uregs[i]);
		if (pfn1 >= 0)
			sprintf(s1, "%8lx", pfn1);
		else
			sprintf(s1, "--------");
		if (i % 2 == 0) {
			len += snprintf(buf + len, sizeof(buf) - len, "%s:  %08lx  %s  ",
				       regidx_to_name[i], regs->uregs[i], s1);
		} else {
			len += snprintf(buf + len, sizeof(buf) - len, "%s:  %08lx  %s",
				       regidx_to_name[i], regs->uregs[i], s1);

			pr_info("%s\n", buf);
			len = 0;
		}
	}

	if (len > 0)
		pr_info("%s\n", buf);
}

void show_debug_ratelimited(struct pt_regs *regs, unsigned int reg_en)
{
	if (user_fault_debug_ratelimited()) {
		show_all_pfn(current, regs);

		if (reg_en)
			show_regs(regs);
	}
}
#elif defined CONFIG_RISCV
void show_all_pfn(struct task_struct *task, struct pt_regs *regs)
{
	int i;
	long pfn1;
	char s1[10];
	int top = 32;
	unsigned long *reg_array = (unsigned long *)regs;

	pr_info("reg              value       pfn  ");
	pr_cont("reg              value       pfn\n");
	for (i = 0; i < top; i++) {
		pfn1 = get_user_pfn(task->mm, reg_array[i]);
		if (pfn1 >= 0)
			sprintf(s1, "%8lx", pfn1);
		else
			sprintf(s1, "--------");
		if (i % 2 == 1)
			pr_cont("%-3s:  %016lx  %s\n", regs_name[i], reg_array[i], s1);
		else
			pr_info("%-3s:  %016lx  %s  ", regs_name[i], reg_array[i], s1);
	}
	pr_cont("\n");
	pfn1 = get_user_pfn(task->mm, regs->epc);
	if (pfn1 >= 0)
		sprintf(s1, "%8lx", pfn1);
	else
		sprintf(s1, "--------");
	pr_info("pc :  %016lx  %s\n", regs->epc, s1);
	pfn1 = get_user_pfn(task->mm, regs->sp);
	if (pfn1 >= 0)
		sprintf(s1, "%8lx", pfn1);
	else
		sprintf(s1, "--------");
	pr_info("sp :  %016lx  %s\n", regs->sp, s1);

	pfn1 = get_user_pfn(task->mm, regs->badaddr);
	if (pfn1 >= 0)
		sprintf(s1, "%8lx", pfn1);
	else
		sprintf(s1, "--------");
	pr_info("unused :  %016lx  %s\n", regs->badaddr, s1);
//	pr_info("offset :  %016lx\n", kaslr_offset());
}

#endif

static int (*dmc_cb)(char *);
void set_dump_dmc_func(void *f)
{
	dmc_cb =  (void *)f;
}
EXPORT_SYMBOL(set_dump_dmc_func);

void _dump_dmc_reg(void)
{
	char buf[1024] = {0};

	if (!dmc_cb)
		return;
	dmc_cb(buf);
	pr_crit("%s\n", buf);
}

void show_user_fault_info(struct pt_regs *regs, u64 lr, u64 sp)
{
#ifdef CONFIG_ARM64
	if (!user_mode(regs) && !oops_in_progress)
		return;

	if (user_mode(regs)) {
		show_vma(current->mm, instruction_pointer(regs));
		show_vma(current->mm, lr);
		show_vma(current->mm, read_sysreg(far_el1));
		show_all_pfn(current, regs);
	} else {
		show_pfn(instruction_pointer(regs), "PC");
		show_pfn(sp, "SP");
		show_pfn(read_sysreg(far_el1), "FAR");
		show_regs_pfn(regs);
	}
#elif defined CONFIG_ARM
	if (user_mode(regs)) {
		show_vma(current->mm, instruction_pointer(regs));
		show_vma(current->mm, regs->ARM_lr);
	}
#elif defined CONFIG_RISCV
	if (user_mode(regs)) {
		show_vma(current->mm, instruction_pointer(regs));
		show_vma(current->mm, lr);
		show_vma(current->mm, regs->badaddr);
	}

	show_pfn(regs->badaddr, "badaddr");
	show_regs_pfn(regs);
#endif
}

void show_extra_reg_data(struct pt_regs *regs)
{
#ifdef CONFIG_ARM64
	if (!user_mode(regs) && !oops_in_progress)
		return;
#endif

	if (!user_mode(regs))
		show_extra_register_data(regs, 128);
	else
		show_user_extra_register_data(regs, 128);
	printk("\n");
}

#if IS_MODULE(CONFIG_AMLOGIC_USER_FAULT) && IS_ENABLED(CONFIG_KALLSYMS_ALL)
static struct kprobe kp_show_regs = {
	.symbol_name	= "__show_regs",
};

static struct kprobe kp_bad_el0_sync = {
	.symbol_name	= "bad_el0_sync",
};

static int unhandled_signal_entry_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
#if defined(CONFIG_ARM64)
	*ri->data = regs->regs[1];
#elif defined(CONFIG_ARM)
	*ri->data = regs->ARM_r1;
#endif

	return 0;
}

static int unhandled_signal_ret_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	int sig = *ri->data;

	if (sig == SIGILL || sig == SIGBUS || sig == SIGSEGV) {
#if defined(CONFIG_ARM64)
		regs->regs[0] = 1;
#elif defined(CONFIG_ARM)
		regs->ARM_r0 = 1;
#endif
	}

	return 0;
}

static struct kretprobe unhandled_signal_krp = {
	.handler = unhandled_signal_ret_handler,
	.entry_handler = unhandled_signal_entry_handler,
	.data_size = sizeof(int),
};

static void __kprobes show_regs_handler_post(struct kprobe *p,
				struct pt_regs *param_regs, unsigned long flags)
{
	struct pt_regs *regs = (struct pt_regs *)param_regs->regs[0];
	u64 lr, sp;

	if (compat_user_mode(regs)) {
		lr = regs->compat_lr;
		sp = regs->compat_sp;
	} else {
		lr = regs->regs[30];
		sp = regs->sp;
	}

	show_user_fault_info(regs, lr, sp);
	show_extra_reg_data(regs);
}

static void __kprobes bad_el0_sync_handler_post(struct kprobe *p,
				struct pt_regs *param_regs, unsigned long flags)
{
	struct pt_regs *regs = (struct pt_regs *)param_regs->regs[0];

	show_all_pfn(current, regs);
}

static int __nocfi user_fault_register_kprobe(void *data)
{
	int ret;

	ret = register_kprobe(&kp_lookup_name);
	if (ret < 0) {
		pr_err("register_kprobe failed, returned %d\n", ret);
		return -1;
	}
	pr_debug("kprobe lookup offset at %px\n", kp_lookup_name.addr);

	aml_syms_lookup = (unsigned long (*)(const char *name))kp_lookup_name.addr;

	aml_init_mm = (struct mm_struct *)aml_syms_lookup("init_mm");
	pr_debug("aml_init_mm: %px\n", aml_init_mm);

	kp_show_regs.post_handler = show_regs_handler_post;
	ret = register_kprobe(&kp_show_regs);
	if (ret < 0) {
		pr_err("register_kprobe %s failed, returned %d\n",
			kp_show_regs.symbol_name, ret);
		return -1;
	}

	kp_bad_el0_sync.post_handler = bad_el0_sync_handler_post;
	ret = register_kprobe(&kp_bad_el0_sync);
	if (ret < 0) {
		pr_err("register_kprobe %s failed, returned %d\n",
			kp_bad_el0_sync.symbol_name, ret);
		return -1;
	}

	unhandled_signal_krp.kp.symbol_name = "unhandled_signal";
	ret = register_kretprobe(&unhandled_signal_krp);
	if (ret < 0) {
		pr_err("register kretprobe 'unhandled_signal' failed, returned %d\n", ret);
		return ret;
	}

	return 0;
}

static int __init user_fault_module_init(void)
{
	kthread_run(user_fault_register_kprobe, NULL, "AML_USER_FAULT");

	return 0;
}

static void __exit user_fault_module_exit(void)
{
	unregister_kprobe(&kp_lookup_name);
	unregister_kprobe(&kp_show_regs);
	unregister_kprobe(&kp_bad_el0_sync);
}

module_init(user_fault_module_init);
module_exit(user_fault_module_exit);
#endif
MODULE_LICENSE("GPL v2");
