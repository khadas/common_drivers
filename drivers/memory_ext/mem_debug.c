// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/export.h>
#include <linux/errno.h>
#include <asm/sections.h>
#include <linux/sizes.h>
#ifdef CONFIG_RISCV
#include <asm/pgtable.h>
#else
#include <asm/memory.h>
#endif
#include <linux/init.h>
#include <linux/initrd.h>
#include <linux/gfp.h>
#include <linux/memblock.h>
#include <linux/dma-mapping.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/amlogic/mem_debug.h>
#include <linux/crash_dump.h>
#ifdef CONFIG_ARM64
#include <asm/boot.h>
#endif
#include <asm/fixmap.h>
#include <linux/kasan.h>
#include <linux/seq_file.h>
#ifdef CONFIG_HIGHMEM
#include <asm/highmem.h>
#endif
#ifdef CONFIG_AMLOGIC_CMA
#include <linux/amlogic/aml_cma.h>
#endif
#ifdef CONFIG_AMLOGIC_VMAP
#include <linux/amlogic/vmap_stack.h>
#endif
#if IS_MODULE(CONFIG_AMLOGIC_MEM_DEBUG)
#include <linux/module.h>
#endif

static int pagemap_en;
unsigned long mlock_fault_size;

void dump_mem_layout(char *buf)
{
#define MLK(b, t) b, t, ((t) - (b)) >> 10
#define MLM(b, t) b, t, ((t) - (b)) >> 20
#define MLG(b, t) b, t, ((t) - (b)) >> 30
#define MLK_ROUNDUP(b, t) b, t, DIV_ROUND_UP(((t) - (b)), SZ_1K)
#ifdef CONFIG_ARM64
	int pos = 0;

	pos += sprintf(buf + pos, "Virtual kernel memory layout:\n");
#ifdef CONFIG_KASAN_GENERIC
	pos += sprintf(buf + pos, "    kasan   : 0x%16lx - 0x%16lx   (%6ld GB)\n",
		MLG(KASAN_SHADOW_START, KASAN_SHADOW_END));
#endif
	pos += sprintf(buf + pos, "    modules : 0x%16lx - 0x%16lx   (%6ld MB)\n",
		MLM(MODULES_VADDR, MODULES_END));
	pos += sprintf(buf + pos, "    vmalloc : 0x%16lx - 0x%16lx   (%6ld GB)\n",
		MLG(VMALLOC_START, VMALLOC_END));
#if IS_MODULE(CONFIG_AMLOGIC_MEM_DEBUG)
	pos += sprintf(buf + pos, "    fixed   : 0x%16lx - 0x%16lx   (%6ld KB)\n",
		MLK(FIXADDR_START, FIXADDR_TOP));
	pos += sprintf(buf + pos, "    PCI I/O : 0x%16lx - 0x%16lx   (%6ld MB)\n",
		MLM(PCI_IO_START, PCI_IO_END));
#ifdef CONFIG_SPARSEMEM_VMEMMAP
	pos += sprintf(buf + pos, "    vmemmap : 0x%16lx - 0x%16lx   (%6ld GB maximum)\n",
		MLG(VMEMMAP_START, VMEMMAP_START + VMEMMAP_SIZE));
#endif
#else
	pos += sprintf(buf + pos, "      .text : 0x%px" " - 0x%px" "   (%6ld KB) 0x%lx\n",
		MLK_ROUNDUP(_text, _etext), (unsigned long)__pa_symbol(_text));
	pos += sprintf(buf + pos, "    .rodata : 0x%px" " - 0x%px" "   (%6ld KB) 0x%lx\n",
		MLK_ROUNDUP(__start_rodata, __init_begin),
		(unsigned long)__pa_symbol(__start_rodata));
	pos += sprintf(buf + pos, "      .init : 0x%px" " - 0x%px" "   (%6ld KB) 0x%lx\n",
		MLK_ROUNDUP(__init_begin, __init_end),
		(unsigned long)__pa_symbol(__start_rodata));
	pos += sprintf(buf + pos, "      .data : 0x%px" " - 0x%px" "   (%6ld KB) 0x%lx\n",
		MLK_ROUNDUP(_sdata, _edata),
		(unsigned long)__pa_symbol(_sdata));
	pos += sprintf(buf + pos, "       .bss : 0x%px" " - 0x%px" "   (%6ld KB) 0x%lx\n",
		MLK_ROUNDUP(__bss_start, __bss_stop),
		(unsigned long)__pa_symbol(__bss_start));
	pos += sprintf(buf + pos, "    fixed   : 0x%16lx - 0x%16lx   (%6ld KB)\n",
		MLK(FIXADDR_START, FIXADDR_TOP));
	pos += sprintf(buf + pos, "    PCI I/O : 0x%16lx - 0x%16lx   (%6ld MB)\n",
		MLM(PCI_IO_START, PCI_IO_END));
#ifdef CONFIG_SPARSEMEM_VMEMMAP
	pos += sprintf(buf + pos, "    vmemmap : 0x%16lx - 0x%16lx   (%6ld GB maximum)\n",
		MLG(VMEMMAP_START, VMEMMAP_START + VMEMMAP_SIZE));
	pos += sprintf(buf + pos, "              0x%16lx - 0x%16lx   (%6ld MB actual)\n",
		MLM((unsigned long)phys_to_page(memblock_start_of_DRAM()),
		    (unsigned long)virt_to_page(high_memory)));
#endif
	pos += sprintf(buf + pos, "    memory  : 0x%16lx - 0x%16lx   (%6ld MB) 0x%lx\n",
		MLM(__phys_to_virt(memblock_start_of_DRAM()),
		    (unsigned long)high_memory), (unsigned long)memblock_start_of_DRAM());
	pos += sprintf(buf + pos, "     offset : 0x%lx\n", kaslr_offset());
#endif
#elif defined CONFIG_ARM
		sprintf(buf, "Virtual kernel memory layout:\n"
#ifdef CONFIG_KASAN
					"	 kasan	 : 0x%08lx - 0x%08lx   (%4ld MB)\n"
#endif
#ifdef CONFIG_HAVE_TCM
					"	 DTCM	 : 0x%08lx - 0x%08lx   (%4ld kB)\n"
					"	 ITCM	 : 0x%08lx - 0x%08lx   (%4ld kB)\n"
#endif
					"	 fixmap  : 0x%08lx - 0x%08lx   (%4ld kB)\n"
					"	 vmalloc : 0x%08lx - 0x%08lx   (%4ld MB)\n"
					"	 lowmem  : 0x%08lx - 0x%08lx   (%4ld MB)\n"
#ifdef CONFIG_HIGHMEM
					"	 pkmap	 : 0x%08lx - 0x%08lx   (%4ld MB)\n"
#endif
#ifdef CONFIG_MODULES
					"	 modules : 0x%08lx - 0x%08lx   (%4ld MB)\n"
#endif
					"	   .text : 0x%px" " - 0x%px" "   (%4td kB)\n"
					"	   .init : 0x%px" " - 0x%px" "   (%4td kB)\n"
					"	   .data : 0x%px" " - 0x%px" "   (%4td kB)\n"
					"		.bss : 0x%px" " - 0x%px" "   (%4td kB)\n",
#ifdef CONFIG_KASAN
					MLM(KASAN_SHADOW_START, KASAN_SHADOW_END),
#endif

#ifdef CONFIG_HAVE_TCM
					MLK(DTCM_OFFSET, (unsigned long) dtcm_end),
					MLK(ITCM_OFFSET, (unsigned long) itcm_end),
#endif
					MLK(FIXADDR_START, FIXADDR_END),
					MLM(VMALLOC_START, VMALLOC_END),
					MLM(PAGE_OFFSET, (unsigned long)high_memory),
#ifdef CONFIG_HIGHMEM
					MLM(PKMAP_BASE, (PKMAP_BASE) + (LAST_PKMAP) *
						(PAGE_SIZE)),
#endif
#ifdef CONFIG_MODULES
					MLM(MODULES_VADDR, MODULES_END),
#endif

					MLK_ROUNDUP(_text, _etext),
					MLK_ROUNDUP(__init_begin, __init_end),
					MLK_ROUNDUP(_sdata, _edata),
					MLK_ROUNDUP(__bss_start, __bss_stop));
#elif defined CONFIG_RISCV
	int pos = 0;

	pos += sprintf(buf + pos, "Virtual kernel memory layout:\n");
	pos += sprintf(buf + pos, "      fixed : 0x%16lx - 0x%16lx   (%6ld KB)\n",
		MLK(FIXADDR_START, FIXADDR_TOP));
	pos += sprintf(buf + pos, "    PCI I/O : 0x%16lx - 0x%16lx   (%6ld MB)\n",
		MLM(PCI_IO_START, PCI_IO_END));
#ifdef CONFIG_SPARSEMEM_VMEMMAP
	pos += sprintf(buf + pos, "    vmemmap : 0x%16lx - 0x%16lx   (%6ld GB maximum)\n",
		MLG(VMEMMAP_START, VMEMMAP_START + VMEMMAP_SIZE));
	pos += sprintf(buf + pos, "              0x%16lx - 0x%16lx   (%6ld MB actual)\n",
		MLM((unsigned long)phys_to_page(memblock_start_of_DRAM()),
		    (unsigned long)virt_to_page(high_memory)));
#endif
	pos += sprintf(buf + pos, "    vmalloc : 0x%16lx - 0x%16lx   (%6ld GB)\n",
		MLG(VMALLOC_START, VMALLOC_START + VMALLOC_SIZE));
	pos += sprintf(buf + pos, "     memory : 0x%16lx - 0x%16lx   (%6ld MB) 0x%lx\n",
		MLM(PAGE_OFFSET, (unsigned long)high_memory),
		(unsigned long)memblock_start_of_DRAM());
	pos += sprintf(buf + pos, "    modules : 0x%16lx - 0x%16lx   (%6ld MB)\n",
		MLM(MODULES_VADDR, MODULES_END));

	pos += sprintf(buf + pos, "     kernel : 0x%16lx - 0x%16lx   (%6ld MB)\n",
		MLM(KERNEL_LINK_ADDR, ADDRESS_SPACE_END));
	pos += sprintf(buf + pos, "      .text : 0x%px" " - 0x%px" "   (%6ld KB) 0x%lx\n",
		MLK_ROUNDUP(_text, _etext), (unsigned long)__pa_symbol(_text));
	pos += sprintf(buf + pos, "      .init : 0x%px" " - 0x%px" "   (%6ld KB) 0x%lx\n",
		MLK_ROUNDUP(__init_begin, __init_end),
		(unsigned long)__pa_symbol(__init_begin));
	pos += sprintf(buf + pos, "    .rodata : 0x%px" " - 0x%px" "   (%6ld KB) 0x%lx\n",
		MLK_ROUNDUP(__start_rodata, __end_rodata),
		(unsigned long)__pa_symbol(__start_rodata));
	pos += sprintf(buf + pos, "      .data : 0x%px" " - 0x%px" "   (%6ld KB) 0x%lx\n",
		MLK_ROUNDUP(_data, _edata),
		(unsigned long)__pa_symbol(_data));
	pos += sprintf(buf + pos, "       .bss : 0x%px" " - 0x%px" "   (%6ld KB) 0x%lx\n",
		MLK_ROUNDUP(__bss_start, __bss_stop),
		(unsigned long)__pa_symbol(__bss_start));
#endif
}

static int mdebug_show(struct seq_file *m, void *arg)
{
	char *buf = kmalloc(4096, GFP_KERNEL);

	if (!buf) {
		pr_err("%s failed\n", __func__);
		return -1;
	}

	/* update only once */
	dump_mem_layout(buf);
	seq_printf(m, "%s\n", buf);
	pr_info("pagemap_en:%d\n", pagemap_en);
	memset(buf, 0, 4096);
	sprintf(buf, "mlock_fault_size: %ldkb\n", mlock_fault_size * 4);
	seq_printf(m, "%s\n", buf);

	kfree(buf);

	return 0;
}

static int mdebug_open(struct inode *inode, struct file *file)
{
	return single_open(file, mdebug_show, NULL);
}

int pagemap_enabled(void)
{
	return pagemap_en;
}

#if !IS_MODULE(CONFIG_AMLOGIC_MEM_DEBUG)
/*
 * These information will auto inject to /proc/meminfo sysfs
 */
void arch_report_meminfo(struct seq_file *m)
{
#ifdef CONFIG_AMLOGIC_CMA
	seq_printf(m, "DriverCma:      %8ld kB\n",
		   get_cma_allocated() * (1 << (PAGE_SHIFT - 10)));
#endif
#ifdef CONFIG_AMLOGIC_VMAP
	vmap_report_meminfo(m);
#endif
}

static int early_pagemap(char *buf)
{
	if (!buf)
		return -EINVAL;

	if (!strncmp(buf, "eng", 3) ||
	    !strncmp(buf, "userdebug", 9))
		pagemap_en = 1;

	pr_info("%s pagemap for %s build\n",
		pagemap_en ? "enable" : "disable", buf);

	return 0;
}
__setup("buildvariant=", early_pagemap);
#endif

static ssize_t mdebug_write(struct file *file, const char __user *buffer,
			    size_t count, loff_t *ppos)
{
	char *buf;
	unsigned long arg = 0;

	buf = kmalloc(count, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	if (copy_from_user(buf, buffer, count)) {
		kfree(buf);
		return -EINVAL;
	}

	if (!strncmp(buf, "pagemap_en=", 6)) {	/* option for 'pagemap_en=' */
		if (sscanf(buf, "pagemap_en=%ld", &arg) < 0) {
			kfree(buf);
			return -EINVAL;
		}
		pagemap_en = arg ? 1 : 0;
		pr_info("set pagemap_en to %d\n", pagemap_en);
	}

	kfree(buf);

	return count;
}

static const struct proc_ops mdebug_ops = {
	.proc_open	= mdebug_open,
	.proc_read	= seq_read,
	.proc_write	= mdebug_write,
	.proc_lseek	= seq_lseek,
	.proc_release	= single_release,
};

static size_t str_end_char(const char *s, size_t count, char c)
{
	size_t len = count, offset = count;

	while (count--) {
		if (*s == c || *s++ == '\0') {
			offset = len - count;
			break;
		}
	}

	return offset;
}

void dump_mem_layout_boot_phase(void)
{
	char *buf = (void *)__get_free_page(GFP_KERNEL);
	int i = 0, len = 0, offset = 0;

	if (!buf) {
		pr_err("%s alloc buffer failed\n", __func__);
	} else {
		dump_mem_layout(buf);
		len = strlen(buf);

		while (i < len) {
			offset = str_end_char(buf + i, 512, '\n');
			pr_crit("%.*s", offset, buf + i);
			i += offset;
		}

		free_page((unsigned long)buf);
	}
}

static int __init memory_debug_init(void)
{
	struct proc_dir_entry *d_mdebug;

	d_mdebug = proc_create("mem_debug", 0444, NULL, &mdebug_ops);
	if (IS_ERR_OR_NULL(d_mdebug)) {
		pr_err("%s, create proc failed\n", __func__);
		return -1;
	}

	return 0;
}

#if IS_MODULE(CONFIG_AMLOGIC_MEM_DEBUG)
static int __init mem_debug_module_init(void)
{
	memory_debug_init();
	dump_mem_layout_boot_phase();

	return 0;
}

static void __exit mem_debug_module_exit(void)
{
}

module_init(mem_debug_module_init);
module_exit(mem_debug_module_exit);
MODULE_LICENSE("GPL v2");
#else
rootfs_initcall(memory_debug_init);
#endif
