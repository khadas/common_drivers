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
#include <linux/amlogic/vmap_stack.h>
#include <linux/highmem.h>
#include <linux/delay.h>
#include <linux/sched/task_stack.h>
#ifdef CONFIG_KASAN
#include <linux/kasan.h>
#endif
#include <asm/tlbflush.h>
#include <asm/stacktrace.h>
#include <internal.h>

#define DEBUG_VMAP							0
#define MODULE_NAME						"amlogic-vmap"

#define D(format, args...)					\
	{ if (DEBUG_VMAP)						\
		pr_info("%s " format, __func__, ##args);	\
	}

#define E(format, args...)	pr_err("%s " format, __func__, ##args)

static atomic_t vmap_stack_size;
static atomic_t vmap_fault_count;
static atomic_t vmap_pre_handle_count;
static struct aml_vmap *avmap;
static atomic_t vmap_cache_flag;

#ifdef CONFIG_ARM64
DEFINE_PER_CPU(unsigned long [THREAD_SIZE / sizeof(long)], vmap_stack)
	__aligned(PAGE_SIZE);

bool on_vmap_stack(unsigned long sp,  struct stack_info *info)
{
	unsigned long low = (unsigned long)raw_cpu_ptr(vmap_stack);
	unsigned long high = low + THREAD_SIZE;

	if (sp < low || sp >= high)
		return false;

	if (info) {
		info->low = low;
		info->high = high;
		info->type = STACK_TYPE_VMAP;
	}

	return true;
}

void dump_backtrace_entry_vmap(unsigned long ip, unsigned long fp,
			       unsigned long low, const char *loglvl)
{
	unsigned long fp_size = 0;
	unsigned long high;

	high = low + THREAD_SIZE;

	/*
	 * Since the target process may be rescheduled again,
	 * we have to add necessary validation checking for fp.
	 * The checking condition is borrowed from unwind_frame
	 */
	if (on_irq_stack(fp, sizeof(long), NULL) ||
	    (fp >= low && fp <= high)) {
		fp_size = *((unsigned long *)fp) - fp;
		/* fp cross IRQ or vmap stack */
		if (fp_size >= THREAD_SIZE)
			fp_size = 0;
	}
	pr_info("[%016lx+%4ld][<%016lx>] %s %pSb\n",
		fp, fp_size, (unsigned long)ip, loglvl, (void *)ip);
}
#else
static unsigned long irq_stack1[(THREAD_SIZE / sizeof(long))]
				__aligned(THREAD_SIZE);
void *irq_stack[NR_CPUS] = {
	irq_stack1,	/* only assign 1st irq stack ,other need alloc */
};

static unsigned long vmap_stack1[(THREAD_SIZE / sizeof(long))]
				__aligned(THREAD_SIZE);

static void *vmap_stack[NR_CPUS] = {
	vmap_stack1,	/* only assign 1st vmap stack ,other need alloc */
};
#endif

void update_vmap_stack(int diff)
{
	atomic_add(diff, &vmap_stack_size);
}
EXPORT_SYMBOL(update_vmap_stack);

int get_vmap_stack_size(void)
{
	return atomic_read(&vmap_stack_size);
}
EXPORT_SYMBOL(get_vmap_stack_size);

#ifdef CONFIG_ARM
void notrace __setup_vmap_stack(unsigned long cpu)
{
	void *stack;
#define VMAP_MASK		(GFP_ATOMIC | __GFP_ZERO)
#ifdef CONFIG_THUMB2_KERNEL
#define TAG	"r"
#else
#define TAG	"I"
#endif
	stack = vmap_stack[cpu];
	if (!stack) {
		stack = (void *)__get_free_pages(VMAP_MASK, THREAD_SIZE_ORDER);
		WARN_ON(!stack);
		vmap_stack[cpu] = stack;
		irq_stack[cpu] = (void *)__get_free_pages(VMAP_MASK,
							  THREAD_SIZE_ORDER);
		WARN_ON(!irq_stack[cpu]);
	}

	pr_debug("cpu %ld, vmap stack:[%lx-%lx]\n",
		cpu, (unsigned long)stack,
		(unsigned long)stack + THREAD_START_SP);
	pr_debug("cpu %ld, irq  stack:[%lx-%lx]\n",
		cpu, (unsigned long)irq_stack[cpu],
		(unsigned long)irq_stack[cpu] + THREAD_START_SP);
	stack += THREAD_SIZE;
	stack -= sizeof(struct thread_info);
	/*
	 * reserve 24 byte for r0, lr, spsr, sp_svc and 8 bytes gap
	 */
	stack -= (24);
	asm volatile (
		"msr	cpsr_c, %1	\n"
		"mov	sp, %0		\n"
		"msr	cpsr_c, %2	\n"
		:
		: "r" (stack),
		  TAG(PSR_F_BIT | PSR_I_BIT | ABT_MODE),
		  TAG(PSR_F_BIT | PSR_I_BIT | SVC_MODE)
		: "memory", "cc"
	);
}

int on_vmap_irq_stack(unsigned long sp, int cpu)
{
	unsigned long sp_irq;

	sp_irq = (unsigned long)irq_stack[cpu];
	if ((sp & ~(THREAD_SIZE - 1)) == (sp_irq & ~(THREAD_SIZE - 1)))
		return 1;
	return 0;
}

unsigned long notrace irq_stack_entry(unsigned long sp)
{
	int cpu = raw_smp_processor_id();

	if (!on_vmap_irq_stack(sp, cpu)) {
		unsigned long sp_irq = (unsigned long)irq_stack[cpu];
		void *src, *dst;

		/*
		 * copy some data to irq stack
		 */
		src = current_thread_info();
		dst = (void *)(sp_irq + THREAD_INFO_OFFSET);
		memcpy(dst, src, offsetof(struct thread_info, cpu_context));
		sp_irq = (unsigned long)dst - 8;
		/*
		 * save start addr of the interrupted task's context
		 * used to back trace stack call from irq
		 * Note: sp_irq must be aligned to 16 bytes
		 */
		*((unsigned long *)sp_irq) = sp;
		return sp_irq;
	}
	return sp;
}

unsigned long notrace pmd_check(unsigned long addr, unsigned long far)
{
	unsigned int index;
	pgd_t *pgd, *pgd_k;
	p4d_t *p4d, *p4d_k;
	pud_t *pud, *pud_k;
	pmd_t *pmd, *pmd_k;

	if (far < TASK_SIZE)
		return addr;

	index = pgd_index(far);

	pgd   = cpu_get_pgd() + index;
	pgd_k = init_mm.pgd + index;

	p4d = p4d_offset(pgd, addr);
	p4d_k = p4d_offset(pgd_k, addr);

	if (p4d_none(*p4d_k))
		goto bad_area;
	if (!p4d_present(*p4d))
		set_p4d(p4d, *p4d_k);

	pud   = pud_offset(p4d, far);
	pud_k = pud_offset(p4d_k, far);

	if (pud_none(*pud_k))
		goto bad_area;
	if (!pud_present(*pud))
		set_pud(pud, *pud_k);

	pmd   = pmd_offset(pud, far);
	pmd_k = pmd_offset(pud_k, far);

#ifdef CONFIG_ARM_LPAE
	/*
	 * Only one hardware entry per PMD with LPAE.
	 */
	index = 0;
#else
	/*
	 * On ARM one Linux PGD entry contains two hardware entries (see page
	 * tables layout in pgtable.h). We normally guarantee that we always
	 * fill both L1 entries. But create_mapping() doesn't follow the rule.
	 * It can create individual L1 entries, so here we have to call
	 * pmd_none() check for the entry really corresponded to address, not
	 * for the first of pair.
	 */
	index = (far >> SECTION_SHIFT) & 1;
#endif
	if (pmd_none(pmd_k[index]))
		goto bad_area;

	copy_pmd(pmd, pmd_k);
bad_area:
	return addr;
}

void __init fixup_init_thread_union(void)
{
	void *p;

	p = (void *)((unsigned long)&init_thread_union + THREAD_INFO_OFFSET);
	memcpy(p, &init_thread_union, THREAD_INFO_SIZE);
	memset(&init_thread_union, 0, THREAD_INFO_SIZE);
}

static void copy_pgd(void)
{
	unsigned long index;
	pgd_t *pgd_c = NULL, *pgd_k, *pgd_i;
	unsigned long size;

	/*
	 * sync pgd of current task and idmap_pgd from init mm
	 */
	index = pgd_index(TASK_SIZE);
	pgd_c = cpu_get_pgd() + index;
	pgd_i = idmap_pgd     + index;
	pgd_k = init_mm.pgd   + index;
	size  = (PTRS_PER_PGD - USER_PTRS_PER_PGD) * sizeof(pgd_t);
	pr_debug("pgd:%p, pgd_k:%p, pdg_i:%p\n",
		 pgd_c, pgd_k, pgd_i);
	memcpy(pgd_c, pgd_k, size);
	memcpy(pgd_i, pgd_k, size);
}

unsigned long save_suspend_context(unsigned int *ptr)
{
	unsigned long ret;

	if (likely(is_vmap_addr((unsigned long)ptr))) {
		struct page *page = vmalloc_to_page(ptr);
		unsigned long offset;

		offset = (unsigned long)ptr & (PAGE_SIZE - 1);
		ret = (page_to_phys(page) + offset);
		pr_debug("%s, ptr:%p, page:%lx, save_ptr:%lx\n",
			 __func__, ptr, page_to_pfn(page), ret);
		copy_pgd();
	} else {
		ret = virt_to_phys(ptr);
	}
	return ret;
}
#endif /* CONFIG_ARM */

int is_vmap_addr(unsigned long addr)
{
	unsigned long start, end;

	start = (unsigned long)avmap->root_vm->addr;
	end   = (unsigned long)avmap->root_vm->addr + avmap->root_vm->size;
	if (addr >= start && addr < end)
		return 1;
	else
		return 0;
}

static struct page *get_vmap_cached_page(int *remain)
{
	unsigned long flags;
	struct page *page;

	spin_lock_irqsave(&avmap->page_lock, flags);
	if (unlikely(!avmap->cached_pages)) {
		spin_unlock_irqrestore(&avmap->page_lock, flags);
		return NULL;
	}
	page = list_first_entry(&avmap->list, struct page, lru);
	list_del(&page->lru);
	avmap->cached_pages--;
	*remain = avmap->cached_pages;
	spin_unlock_irqrestore(&avmap->page_lock, flags);

	return page;
}

struct page *check_pte_exist(unsigned long addr)
{
	struct mm_struct *mm;
	pgd_t *pgd;
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;

	mm = &init_mm;

	pgd = pgd_offset(mm, addr);

	if (pgd_none(*pgd) || pgd_bad(*pgd))
		return NULL;

	p4d = p4d_offset(pgd, addr);
	if (p4d_none(*p4d) || p4d_bad(*p4d))
		return NULL;

	pud = pud_offset(p4d, addr);
	if (pud_none(*pud) || pud_bad(*pud))
		return NULL;

	pmd = pmd_offset(pud, addr);
	if (pmd_none(*pmd) || pmd_bad(*pmd))
		return NULL;

	pte = pte_offset_kernel(pmd, addr);
	if (pte_none(*pte))
		return NULL;
#ifdef CONFIG_ARM64
	return pte_page(*pte);
#elif defined(CONFIG_ARM)
	return pte_page(pte_val(*pte));
#else
	return NULL;	/* not supported */
#endif
}

static int vmap_mmu_set(struct page *page, unsigned long addr, int set)
{
	pgd_t *pgd = NULL;
	p4d_t *p4d = NULL;
	pud_t *pud = NULL;
	pmd_t *pmd = NULL;
	pte_t *pte = NULL;

	pgd = pgd_offset_k(addr);
	p4d = p4d_alloc(&init_mm, pgd, addr);
	if (!p4d)
		goto nomem;

	pud = pud_alloc(&init_mm, p4d, addr);
	if (!pud)
		goto nomem;

	if (pud_none(*pud)) {
		pmd = pmd_alloc(&init_mm, pud, addr);
		if (!pmd)
			goto nomem;
	}

	pmd = pmd_offset(pud, addr);
	if (pmd_none(*pmd)) {
		pte = pte_alloc_kernel(pmd, addr);
		if (!pte)
			goto nomem;
	}

	pte = pte_offset_kernel(pmd, addr);
	if (set)
		set_pte_at(&init_mm, addr, pte, mk_pte(page, PAGE_KERNEL));
	else
		pte_clear(&init_mm, addr, pte);
	flush_tlb_kernel_range(addr, addr + PAGE_SIZE);
#ifdef CONFIG_ARM64
	D("add:%lx, pgd:%px %llx, pmd:%px %llx, pte:%px %llx\n",
		addr, pgd, pgd_val(*pgd), pmd, pmd_val(*pmd),
		pte, pte_val(*pte));
#elif defined(CONFIG_ARM)
	D("add:%lx, pgd:%px %x, pmd:%px %x, pte:%px %x\n",
		addr, pgd, (unsigned int)pgd_val(*pgd),
		pmd, (unsigned int)pmd_val(*pmd),
		pte, pte_val(*pte));
#endif
	return 0;
nomem:
	E("allocation page table failed, G:%px, U:%px, M:%px, T:%px",
		pgd, pud, pmd, pte);
	return -ENOMEM;
}

static int stack_floor_page(unsigned long addr)
{
	unsigned long pos;

	pos = addr & (THREAD_SIZE - 1);
	/*
	 * stack address must align to THREAD_SIZE
	 */
	if (THREAD_SIZE_ORDER > 1)
		return pos < PAGE_SIZE;
	else
		return pos < (PAGE_SIZE / 4);
}

static int check_addr_up_flow(unsigned long addr)
{
	/*
	 * It's the first page of 4 contiguous virtual address
	 * rage(aligned to THREAD_SIZE) but next page of this
	 * addr is not mapped
	 */
	if (stack_floor_page(addr) && !check_pte_exist(addr + PAGE_SIZE))
		return 1;
	return 0;
}

static void dump_backtrace_entry(unsigned long ip, unsigned long fp,
				 unsigned long sp)
{
	unsigned long fp_size = 0;

#ifdef CONFIG_ARM64
	if (fp >= VMALLOC_START) {
		fp_size = *((unsigned long *)fp) - fp;
		/* fp cross IRQ or vmap stack */
		if (fp_size >= THREAD_SIZE)
			fp_size = 0;
	}
	pr_info("[%016lx+%4ld][<%px>] %pS\n",
		fp, fp_size, (void *)ip, (void *)ip);
#elif defined(CONFIG_ARM)
	if (fp >= TASK_SIZE) {
		fp_size = fp - sp + 4;
		/* fp cross IRQ or vmap stack */
		if (fp_size >= THREAD_SIZE)
			fp_size = 0;
	}
	pr_info("[%08lx+%4ld][<%px>] %pS\n",
		fp, fp_size, (void *)ip, (void *)ip);
#endif
}

static noinline void show_fault_stack(unsigned long addr, struct pt_regs *regs)
{
	struct stackframe frame;
	unsigned long sp;

#ifdef CONFIG_ARM64
	frame.fp = regs->regs[29];
	frame.prev_fp = addr;
	frame.pc = (unsigned long)regs->regs[30];
	sp = addr;
	frame.prev_type = STACK_TYPE_UNKNOWN;
#elif defined(CONFIG_ARM)
	frame.fp = regs->ARM_fp;
	frame.sp = regs->ARM_sp;
	frame.lr = regs->ARM_lr;
	frame.pc = (unsigned long)regs->uregs[15];
	sp = regs->ARM_sp;
#endif

	pr_info("Addr:%lx, Call trace:\n", addr);
#ifdef CONFIG_ARM64
	pr_info("[%016lx+%4ld][<%px>] %pS\n",
		addr, frame.fp - addr, (void *)regs->pc, (void *)regs->pc);
#elif defined(CONFIG_ARM)
	pr_info("[%08lx+%4ld][<%px>] %pS\n",
		addr, frame.fp - addr, (void *)regs->uregs[15],
		(void *)regs->uregs[15]);
#endif
	while (1) {
		int ret;

	#ifdef CONFIG_ARM64
		dump_backtrace_entry(frame.pc, frame.fp, sp);
		ret = unwind_frame(current, &frame);
	#elif defined(CONFIG_ARM)
		dump_backtrace_entry(frame.pc, frame.fp, frame.sp);
		ret = unwind_frame(&frame);
	#endif
		if (ret < 0)
			break;
	}
}

static void check_sp_fault_again(struct pt_regs *regs)
{
	unsigned long sp = 0, addr;
	struct page *page;
	int cache = 0;

#ifdef CONFIG_ARM
	sp = regs->ARM_sp;
#elif defined(CONFIG_ARM64)
	sp = regs->sp;
#endif
	addr = sp - sizeof(*regs);

	/*
	 * When we handle vmap stack fault, we are in pre-allcated
	 * per-cpu vmap stack. But if sp is near bottom of a page and we
	 * return to normal handler, sp may down grow to another page
	 * to cause a vmap fault again. So we need map next page for
	 * stack before page-fault happen.
	 *
	 * But we need check sp is really in vmap stack range.
	 */
	if (!is_vmap_addr(addr)) /* addr may in linear mapping */
		return;

	if (sp && ((addr & PAGE_MASK) != (sp & PAGE_MASK))) {
		/*
		 * will fault when we copy back context, so handle
		 * it first
		 */
		D("fault again, sp:%lx, addr:%lx\n", sp, addr);
		page = get_vmap_cached_page(&cache);
		WARN_ON(!page);
		vmap_mmu_set(page, addr, 1);
		update_vmap_stack(1);
	#ifndef CONFIG_KASAN
		if (THREAD_SIZE_ORDER > 1 && stack_floor_page(addr)) {
			E("task:%d %s, stack near overflow, addr:%lx\n",
				current->pid, current->comm, addr);
			show_fault_stack(addr, regs);
		}
	#endif

		/* cache is not enough */
		if (cache <= (VMAP_CACHE_PAGE / 2))
			atomic_inc(&vmap_cache_flag);

		D("map page:%5lx for addr:%lx\n", page_to_pfn(page), addr);
		atomic_inc(&vmap_pre_handle_count);
	#if DEBUG_VMAP
		show_fault_stack(addr, regs);
	#endif
	}
}

/*
 * IRQ should *NEVER* been opened in this handler
 */
int __handle_vmap_fault(unsigned long addr, unsigned int esr,
		      struct pt_regs *regs)
{
	struct page *page;
	int cache = 0;

	if (!is_vmap_addr(addr)) {
		check_sp_fault_again(regs);
		return -EINVAL;
	}

	D("addr:%lx, esr:%x, task:%5d %s\n",
		addr, esr, current->pid, current->comm);
#ifdef CONFIG_ARM64
	D("pc:%ps, %llx, lr:%ps, %llx, sp:%llx, %lx\n",
		(void *)regs->pc, regs->pc,
		(void *)regs->regs[30], regs->regs[30], regs->sp,
		current_stack_pointer);
#elif defined(CONFIG_ARM)
	D("pc:%ps, %lx, lr:%ps, %lx, sp:%lx, %lx\n",
		(void *)regs->uregs[15], regs->uregs[15],
		(void *)regs->uregs[14], regs->uregs[14], regs->uregs[13],
		current_stack_pointer);
#endif

	if (check_addr_up_flow(addr)) {
		E("address %lx out of range\n", addr);
	#ifdef CONFIG_ARM64
		E("PC is:%llx, %ps, LR is:%llx %ps\n",
			regs->pc, (void *)regs->pc,
			regs->regs[30], (void *)regs->regs[30]);
	#elif defined(CONFIG_ARM)
		E("PC is:%lx, %ps, LR is:%lx %ps\n",
			regs->uregs[15], (void *)regs->uregs[15],
			regs->uregs[14], (void *)regs->uregs[14]);
	#endif
		E("task:%d %s, stack:%px, %lx\n",
			current->pid, current->comm, current->stack,
			current_stack_pointer);
		show_fault_stack(addr, regs);
		check_sp_fault_again(regs);
		return -ERANGE;
	}

#ifdef CONFIG_ARM
	page = check_pte_exist(addr);
	if (page) {
		D("task:%d %s, page:%lx mapped for addr:%lx\n",
		  current->pid, current->comm, page_to_pfn(page), addr);
		check_sp_fault_again(regs);
		return -EINVAL;
	}
#endif

	/*
	 * allocate a new page for vmap
	 */
	page = get_vmap_cached_page(&cache);
	WARN_ON(!page);
	vmap_mmu_set(page, addr, 1);
	update_vmap_stack(1);
	if (THREAD_SIZE_ORDER > 1 && stack_floor_page(addr)) {
		E("task:%d %s, stack near overflow, addr:%lx\n",
			current->pid, current->comm, addr);
		show_fault_stack(addr, regs);
	}

	/* cache is not enough */
	if (cache <= (VMAP_CACHE_PAGE / 2))
		atomic_inc(&vmap_cache_flag);

	atomic_inc(&vmap_fault_count);
	D("map page:%5lx for addr:%lx\n", page_to_pfn(page), addr);
#if DEBUG_VMAP
	show_fault_stack(addr, regs);
#endif
	return 0;
}

struct pt_regs *handle_vmap_fault(unsigned long addr, unsigned int esr,
				  struct pt_regs *regs)
{
	int ret;

	ret = __handle_vmap_fault(addr, esr, regs);
	if (!ret) {
	#ifdef CONFIG_ARM64
		__vmap_exit(regs);
	#endif
		return NULL;
	}
#ifdef CONFIG_ARM64
	return switch_vmap_context(regs);
#else
	return (struct pt_regs *)-1UL;
#endif
}
EXPORT_SYMBOL(handle_vmap_fault);

/* APIs for external usage */
void vmap_report_meminfo(struct seq_file *m)
{
	unsigned long kb = 1 << (PAGE_SHIFT - 10);
	unsigned long tmp1, tmp2, tmp3;

	tmp1 = kb * atomic_read(&vmap_stack_size);
	tmp2 = kb * atomic_read(&vmap_fault_count);
	tmp3 = kb * atomic_read(&vmap_pre_handle_count);

	seq_printf(m, "VmapStack:      %8ld kB\n", tmp1);
	seq_printf(m, "VmapFault:      %8ld kB\n", tmp2);
	seq_printf(m, "VmapPfault:     %8ld kB\n", tmp3);
}

/* FOR debug */
static unsigned long vmap_debug_jiff;

void aml_account_task_stack(struct task_struct *tsk, int account)
{
	unsigned long stack = (unsigned long)task_stack_page(tsk);
	struct page *first_page;

	if (unlikely(!is_vmap_addr(stack))) {
		/* stack get from kmalloc */
		first_page = virt_to_page((void *)stack);
		mod_lruvec_page_state(first_page, NR_KERNEL_STACK_KB,
				      THREAD_SIZE / 1024 * account);

		update_vmap_stack(account * (THREAD_SIZE / PAGE_SIZE));
		return;
	}
	stack += STACK_TOP_PAGE_OFF;
	first_page = vmalloc_to_page((void *)stack);
	mod_lruvec_page_state(first_page, NR_KERNEL_STACK_KB,
			      THREAD_SIZE / 1024 * account);
	if (time_after(jiffies, vmap_debug_jiff + HZ * 5)) {
		int ratio, rem;

		vmap_debug_jiff = jiffies;
		ratio = ((get_vmap_stack_size() << (PAGE_SHIFT - 10)) * 10000) /
			global_node_page_state(NR_KERNEL_STACK_KB);
		rem   = ratio % 100;
		D("STACK:%ld KB, vmap:%d KB, cached:%d KB, rate:%2d.%02d%%\n",
			global_node_page_state(NR_KERNEL_STACK_KB),
			get_vmap_stack_size() << (PAGE_SHIFT - 10),
			avmap->cached_pages << (PAGE_SHIFT - 10),
			ratio / 100, rem);
	}
}

#if defined(CONFIG_KASAN_GENERIC) || defined(CONFIG_KASAN_SW_TAGS)
DEFINE_MUTEX(stack_shadow_lock);	/* For kasan */
static void check_and_map_stack_shadow(unsigned long addr)
{
	unsigned long shadow;
	struct page *page, *pages[2] = {};
	int ret;

	shadow = (unsigned long)kasan_mem_to_shadow((void *)addr);
	page   = check_pte_exist(shadow);
	if (page) {
		WARN(page_address(page) == (void *)kasan_early_shadow_page,
		     "bad pte, page:%px, %lx, addr:%lx\n",
		     page_address(page), page_to_pfn(page), addr);
		return;
	}
	shadow = shadow & PAGE_MASK;
	page   = alloc_page(GFP_KERNEL | __GFP_HIGHMEM |
			    __GFP_ZERO | __GFP_HIGH);
	if (!page) {
		WARN(!page,
		     "alloc page for addr:%lx, shadow:%lx fail\n",
		     addr, shadow);
		return;
	}
	pages[0] = page;
	ret = vmap_pages_range_noflush(shadow, shadow + PAGE_SIZE, PAGE_KERNEL, pages, PAGE_SHIFT);
	if (ret < 0) {
		pr_err("%s, map shadow:%lx failed:%d\n", __func__, shadow, ret);
		__free_page(page);
	}
}
#endif

void *aml_stack_alloc(int node, struct task_struct *tsk)
{
	unsigned long bitmap_no, raw_start;
	struct page *page;
	unsigned long addr, map_addr, flags;

	spin_lock_irqsave(&avmap->vmap_lock, flags);
	raw_start = avmap->start_bit;
	bitmap_no = find_next_zero_bit(avmap->bitmap, MAX_TASKS,
				       avmap->start_bit);
	avmap->start_bit = bitmap_no + 1; /* next idle address space */
	if (bitmap_no >= MAX_TASKS) {
		spin_unlock_irqrestore(&avmap->vmap_lock, flags);
		/*
		 * if vmap address space is full, we still need to try
		 * to get stack from kmalloc
		 */
		addr = __get_free_pages(THREAD_SIZE_ORDER, GFP_KERNEL);
		E("BITMAP FULL, kmalloc task stack:%lx\n", addr);
		return (void *)addr;
	}
	bitmap_set(avmap->bitmap, bitmap_no, 1);
	spin_unlock_irqrestore(&avmap->vmap_lock, flags);

	page = alloc_page(THREADINFO_GFP | __GFP_ZERO | __GFP_HIGHMEM);
	if (!page) {
		spin_lock_irqsave(&avmap->vmap_lock, flags);
		bitmap_clear(avmap->bitmap, bitmap_no, 1);
		spin_unlock_irqrestore(&avmap->vmap_lock, flags);
		E("allocation page failed\n");
		return NULL;
	}
	/*
	 * map first page only
	 */
	addr = (unsigned long)avmap->root_vm->addr + THREAD_SIZE * bitmap_no;
	map_addr = addr + STACK_TOP_PAGE_OFF;
	vmap_mmu_set(page, map_addr, 1);
	update_vmap_stack(1);
#if defined(CONFIG_KASAN_GENERIC) || defined(CONFIG_KASAN_SW_TAGS)
	/* 2 thread stack can be a single shadow page, we need use lock */
	mutex_lock(&stack_shadow_lock);
	check_and_map_stack_shadow(addr);
	mutex_unlock(&stack_shadow_lock);
#endif

	D("bit idx:%5ld, start:%5ld, addr:%lx, page:%lx\n",
		bitmap_no, raw_start, addr, page_to_pfn(page));

	return (void *)addr;
}

void aml_stack_free(struct task_struct *tsk)
{
	unsigned long stack = (unsigned long)tsk->stack;
	unsigned long addr, bitmap_no;
	struct page *page;
	unsigned long flags;

	if (unlikely(!is_vmap_addr(stack))) {
		/* stack get from kmalloc */
		free_pages(stack, THREAD_SIZE_ORDER);
		return;
	}

	addr = stack + STACK_TOP_PAGE_OFF;
	for (; addr >= stack; addr -= PAGE_SIZE) {
		page = vmalloc_to_page((const void *)addr);
		if (!page)
			break;
	#ifdef CONFIG_KASAN
		kasan_unpoison_range((void *)addr, PAGE_SIZE);
	#endif
		vmap_mmu_set(page, addr, 0);
		/* supplement for stack page cache first */
		spin_lock_irqsave(&avmap->page_lock, flags);
		if (avmap->cached_pages < VMAP_CACHE_PAGE) {
			list_add_tail(&page->lru, &avmap->list);
			avmap->cached_pages++;
			spin_unlock_irqrestore(&avmap->page_lock, flags);
			clear_highpage(page);	/* clear for next use */
		} else {
			spin_unlock_irqrestore(&avmap->page_lock, flags);
			__free_page(page);
		}
		update_vmap_stack(-1);
	}
	bitmap_no = (stack - (unsigned long)avmap->root_vm->addr) / THREAD_SIZE;
	spin_lock_irqsave(&avmap->vmap_lock, flags);
	bitmap_clear(avmap->bitmap, bitmap_no, 1);
	if (bitmap_no < avmap->start_bit)
		avmap->start_bit = bitmap_no;
	spin_unlock_irqrestore(&avmap->vmap_lock, flags);
}

#if DEBUG_VMAP
static void check_stack_depth(void *p, int cnt)
{
	unsigned char stack_buf[32];
	unsigned long stack_end, cur_stack;

	stack_end = (unsigned long)p & ~(THREAD_SIZE - 1);
	cur_stack = (unsigned long)stack_buf & (THREAD_SIZE - 1);
	if (cur_stack < 1024) {
		pr_info("%s, %3d, p:%px, stack_buf:%px, cur_stack:%lx, end:%lx, quit!!!\n",
			__func__, cnt, p, stack_buf, cur_stack, stack_end);
		return;
	}
	pr_info("%s, %3d, p:%px, stack_buf:%px, cur_stack:%lx, end:%lx\n",
		__func__, cnt, p, stack_buf, cur_stack, stack_end);
	memcpy(stack_buf, p, sizeof(stack_buf));
	check_stack_depth(stack_buf, cnt + 1);
}

static void stack_test(void)
{
	unsigned char buf[32] = {};

	check_stack_depth(buf, 0);
}
#endif

/*
 * page cache maintain task for vmap
 */
static int vmap_task(void *data)
{
	struct page *page;
	struct list_head head;
	int i, cnt;
	unsigned long flags;
	struct aml_vmap *v = (struct aml_vmap *)data;

#if DEBUG_VMAP
	stack_test();
#endif
	set_user_nice(current, -19);
	while (1) {
		if (kthread_should_stop())
			break;

		if (!atomic_read(&vmap_cache_flag)) {
			msleep(20);
			continue;
		}
		spin_lock_irqsave(&v->page_lock, flags);
		cnt = v->cached_pages;
		spin_unlock_irqrestore(&v->page_lock, flags);
		if (cnt >= VMAP_CACHE_PAGE) {
			D("cache full cnt:%d\n", cnt);
			continue;
		}

		INIT_LIST_HEAD(&head);
		for (i = 0; i < VMAP_CACHE_PAGE - cnt; i++) {
			page = alloc_page(GFP_KERNEL | __GFP_HIGHMEM |
					  __GFP_ZERO | __GFP_HIGH);
			if (!page) {
				E("get page failed, allocated:%d, cnt:%d\n",
				  i, cnt);
				break;
			}
			list_add(&page->lru, &head);
		}
		spin_lock_irqsave(&v->page_lock, flags);
		list_splice(&head, &v->list);
		v->cached_pages += i;
		spin_unlock_irqrestore(&v->page_lock, flags);
		atomic_set(&vmap_cache_flag, 0);
		E("add %d pages, cnt:%d\n", i, cnt);
	}
	return 0;
}

int __init start_thread_work(void)
{
	kthread_run(vmap_task, avmap, "vmap_thread");
	return 0;
}
arch_initcall(start_thread_work);

void __init thread_stack_cache_init(void)
{
	int i;
	struct page *page;

	page = alloc_pages(GFP_KERNEL | __GFP_HIGHMEM, VMAP_CACHE_PAGE_ORDER);
	if (!page)
		return;

	avmap = kzalloc(sizeof(*avmap), GFP_KERNEL);
	if (!avmap) {
		__free_pages(page, VMAP_CACHE_PAGE_ORDER);
		return;
	}

	avmap->bitmap = kzalloc(MAX_TASKS / 8, GFP_KERNEL);
	if (!avmap->bitmap) {
		__free_pages(page, VMAP_CACHE_PAGE_ORDER);
		kfree(avmap);
		return;
	}
	pr_info("%s, vmap:%px, bitmap:%px, cache page:%lx\n",
		__func__, avmap, avmap->bitmap, page_to_pfn(page));
	avmap->root_vm = __get_vm_area_node(VM_STACK_AREA_SIZE,
					    MAX_ORDER_NR_PAGES * PAGE_SIZE,
					    PAGE_SHIFT,
					    0, VMAP_ADDR_START, VMAP_ADDR_END,
					    NUMA_NO_NODE, GFP_KERNEL,
					    __builtin_return_address(0));
	WARN(!avmap->root_vm, "alloc vmap area %x failed\n", VM_STACK_AREA_SIZE);
	if (!avmap->root_vm) {
		__free_pages(page, VMAP_CACHE_PAGE_ORDER);
		kfree(avmap->bitmap);
		kfree(avmap);
		return;
	}
	pr_info("%s, allocation vm area:%px, addr:%px, size:%lx\n", __func__,
		avmap->root_vm, avmap->root_vm->addr,
		avmap->root_vm->size);

	INIT_LIST_HEAD(&avmap->list);
	spin_lock_init(&avmap->page_lock);
	spin_lock_init(&avmap->vmap_lock);

	for (i = 0; i < VMAP_CACHE_PAGE; i++) {
		list_add(&page->lru, &avmap->list);
		page++;
	}
	avmap->cached_pages = VMAP_CACHE_PAGE;

#ifdef CONFIG_ARM64
	for_each_possible_cpu(i) {
		unsigned long addr;

		addr = (unsigned long)per_cpu_ptr(vmap_stack, i);
		pr_info("cpu %d, vmap_stack:[%lx-%lx]\n",
			i, addr, addr + THREAD_START_SP);
		addr = (unsigned long)per_cpu_ptr(irq_stack, i);
		pr_info("cpu %d, irq_stack: [%lx-%lx]\n",
			i, addr, addr + THREAD_START_SP);
	}
#endif
}
