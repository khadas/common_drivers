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

#include <asm/compat.h>
#include <asm/cpufeature.h>
#include <asm/cacheflush.h>
#include <asm/exec.h>
#include <asm/mmu_context.h>
#include <asm/processor.h>
#include <asm/stacktrace.h>
#include <asm/switch_to.h>
#include <asm/system_misc.h>

#include <linux/mm_inline.h>
#include <linux/amlogic/user_fault.h>
#ifdef CONFIG_AMLOGIC_SECMON
#include <linux/amlogic/secmon.h>
#endif

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

static void show_data(unsigned long addr, int nbytes, const char *name)
{
	int	i, j;
	int	nlines;
	u32	*p;

	/*
	 * don't attempt to dump non-kernel addresses or
	 * values that are probably just small negative numbers
	 */
	if (addr < PAGE_OFFSET || addr > -256UL)
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

#ifdef CONFIG_AMLOGIC_SECMON
	/*
	 * filter out secure monitor region
	 */
	if (addr <= (unsigned long)high_memory)
		if (within_secmon_region(addr)) {
			printk("\n%s: %#lx S\n", name, addr);
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
		printk("%04lx ", (unsigned long)p & 0xffff);
		for (j = 0; j < 8; j++) {
			u32 data = 0;

			if (get_kernel_nofault(data, p)) {
				pr_cont(" ********");
			} else {
				pr_cont(" %08x", data);
			}
			++p;
		}
		pr_cont("\n");
	}
}

/*
 * dump a block of user memory from around the given address
 */
#ifdef CONFIG_ARM64
static void show_user_data(unsigned long addr, int nbytes, const char *name)
{
	int	i, j;
	int	nlines;
	u32	*p;

	if (!access_ok((void *)addr, nbytes))
		return;

#ifdef CONFIG_AMLOGIC_SECMON
	/*
	 * filter out secure monitor region
	 */
	if (within_secmon_region(addr)) {
		pr_info("\n%s: %#lx S\n", name, addr);
		return;
	}
#endif

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
		pr_info("%04lx ", (unsigned long)p & 0xffff);
		for (j = 0; j < 8; j++) {
			u32	data;
			int bad;

			bad = __get_user(data, p);
			if (bad)
				pr_cont(" ********");
			else
				pr_cont(" %08x", data);
			++p;
		}
		pr_cont("\n");
	}
}

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
	}
}

#else
static void show_user_data(unsigned long addr, int nbytes, const char *name)
{
	int	i, j;
	int	nlines;
	u32	*p;

	if (!access_ok((void *)addr, nbytes))
		return;

#ifdef CONFIG_AMLOGIC_MODIFY
	/*
	 * Treating data in general purpose register as an address
	 * and dereferencing it is quite a dangerous behaviour,
	 * especially when it is an address belonging to secure
	 * region or ioremap region, which can lead to external
	 * abort on non-linefetch and can not be protected by
	 * probe_kernel_address.
	 * We need more strict filtering rules
	 */

#ifdef CONFIG_AMLOGIC_SEC
	/*
	 * filter out secure monitor region
	 */
	if (addr <= (unsigned long)high_memory)
		if (within_secmon_region(addr)) {
			pr_info("\n%s: %#lx S\n", name, addr);
			return;
		}
#endif

	/*
	 * filter out ioremap region
	 */
	if (addr >= VMALLOC_START && addr <= VMALLOC_END)
		if (!pfn_valid(vmalloc_to_pfn((void *)addr))) {
			pr_info("\n%s: %#lx V\n", name, addr);
			return;
		}
#endif

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
		pr_info("%04lx ", (unsigned long)p & 0xffff);
		for (j = 0; j < 8; j++) {
			u32 data;
			int bad;

			bad = __get_user(data, p);
			if (bad) {
				if (j != 7)
					pr_cont(" ********");
				else
					pr_cont(" ********\n");
			} else {
				if (j != 7)
					pr_cont(" %08x", data);
				else
					pr_cont(" %08x\n", data);
			}
			++p;
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
#else
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
#endif

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

		tid = vma_is_stack_for_current(vma);

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

		if (vma_anon_name(vma))
			pr_info("[anon]");
	}

done:
	if (name)
		pr_info("%s", name);
	pr_info("\n");
}

static long get_user_pfn(struct mm_struct *mm, unsigned long addr)
{
	long pfn = -1;
	pgd_t *pgd;

	if (!mm || addr >= VMALLOC_START)
		mm = &init_mm;

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

	if (compat_user_mode(regs))
		top = 15;
	else
		top = 31;
	pr_info("reg              value       pfn  ");
	pr_cont("reg              value       pfn\n");
	for (i = 0; i < top; i++) {
		pfn1 = get_user_pfn(task->mm, regs->regs[i]);
		if (pfn1 >= 0)
			sprintf(s1, "%8lx", pfn1);
		else
			sprintf(s1, "--------");
		if (i % 2 == 1)
			pr_cont("r%-2d:  %016llx  %s\n", i, regs->regs[i], s1);
		else
			pr_info("r%-2d:  %016llx  %s  ", i, regs->regs[i], s1);
	}
	pr_cont("\n");
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
}
#else
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

	top = 16;
	pr_info("reg     value       pfn   ");
	pr_cont("reg     value       pfn\n");
	for (i = 0; i < top; i++) {
		pfn1 = get_user_pfn(task->mm, regs->uregs[i]);
		if (pfn1 >= 0)
			sprintf(s1, "%8lx", pfn1);
		else
			sprintf(s1, "--------");
		if (i % 2 == 0)
			pr_info("%s:  %08lx  %s  ", regidx_to_name[i],
				regs->uregs[i], s1);
		else
			pr_cont("%s:  %08lx  %s\n", regidx_to_name[i],
				regs->uregs[i], s1);
	}
}

void show_debug_ratelimited(struct pt_regs *regs, unsigned int reg_en)
{
	if (user_fault_debug_ratelimited()) {
		show_all_pfn(current, regs);

		if (reg_en)
			show_regs(regs);
	}
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
	if (user_mode(regs)) {
		show_vma(current->mm, instruction_pointer(regs));
		show_vma(current->mm, lr);
		show_vma(current->mm, read_sysreg(far_el1));
	}
	show_pfn(instruction_pointer(regs), "PC");
	show_pfn(sp, "SP");
	show_pfn(read_sysreg(far_el1), "FAR");
	show_regs_pfn(regs);
#else
	if (user_mode(regs)) {
		show_vma(current->mm, instruction_pointer(regs));
		show_vma(current->mm, regs->ARM_lr);
	}
#endif
}

void show_extra_reg_data(struct pt_regs *regs)
{
	if (!user_mode(regs))
		show_extra_register_data(regs, 128);
	else
		show_user_extra_register_data(regs, 128);
	printk("\n");
}
