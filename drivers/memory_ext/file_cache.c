// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/gfp.h>
#include <linux/proc_fs.h>
#include <linux/mmzone.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/sort.h>
#include <linux/seq_file.h>
#include <linux/list.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>
#include <linux/kernel_stat.h>
#include <linux/memcontrol.h>
#include <linux/mm_inline.h>
#include <linux/sched/clock.h>
#include <linux/gfp.h>
#include <linux/mm.h>
#include <linux/jhash.h>
#include <asm/stacktrace.h>
#include <linux/amlogic/file_cache.h>
#include <linux/tick.h>

static int file_cache_filter = 64; /* not print size < file_cache_filter, kb */

static struct proc_dir_entry *d_filecache;
static int record_fct(struct page *page, struct file_cache_trace *fct,
		      int *used, struct rb_root *root, int mc,
		      int active)
{
	struct address_space *mapping;
	struct file_cache_trace *tmp;
	struct rb_node **link, *parent = NULL;

	mapping = page_mapping(page);
	if (!mapping)
		return -1;

	link  = &root->rb_node;
	while (*link) {
		parent = *link;
		tmp = rb_entry(parent, struct file_cache_trace, entry);
		if (mapping == tmp->mapping) { /* match */
			tmp->count++;
			tmp->mapcnt += mc;
			if (active)
				tmp->active_count++;
			else
				tmp->inactive_count++;
			return 0;
		} else if (mapping < tmp->mapping) {
			link = &tmp->entry.rb_left;
		} else {
			link = &tmp->entry.rb_right;
		}
	}
	/* not found, get a new page summary */
	if (*used >= MAX_FCT) {
		pr_err("file out of range\n");
		return -ERANGE;
	}
	tmp = &fct[*used];
	*used = (*used) + 1;
	tmp->mapping = mapping;
	tmp->count++;
	tmp->mapcnt += mc;
	tmp->off = page_to_pgoff(page);
	if (active)
		tmp->active_count++;
	else
		tmp->inactive_count++;
	rb_link_node(&tmp->entry, parent, link);
	rb_insert_color(&tmp->entry, root);
	return 0;
}

struct filecache_stat {
	unsigned int total;
	unsigned int nomap[3];		/* include active/inactive */
	unsigned int files;
};

static void update_file_cache(struct filecache_stat *fs,
			      struct file_cache_trace *fct)
{
	struct lruvec *lruvec;
	pg_data_t *pgdat, *tmp;
	struct page *page, *next;
	struct list_head *list;
	struct rb_root fct_root = RB_ROOT;
	unsigned int t = 0, in = 0, an = 0;
	int r, mc, lru = 0, a = 0;
	struct mem_cgroup *root = NULL, *memcg;

	for_each_online_pgdat(pgdat) {
		memcg = mem_cgroup_iter(root, NULL, NULL);
		do {
			lruvec = mem_cgroup_lruvec(memcg, pgdat);
			tmp = lruvec_pgdat(lruvec);

			for_each_lru(lru) {
				/* only count for filecache */
				if (!is_file_lru(lru) &&
				   lru != LRU_UNEVICTABLE)
					continue;

				if (lru == LRU_ACTIVE_FILE)
					a = 1;
				else
					a = 0;

				list = &lruvec->lists[lru];
				spin_lock_irq(&lruvec->lru_lock);
				list_for_each_entry_safe(page, next,
							 list, lru) {
					if (!page_is_file_lru(page))
						continue;

					t++;
					mc = page_mapcount(page);
					if (mc <= 0) {
						if (a)
							an++;
						else
							in++;
						continue;
					}
					r = record_fct(page, fct, &fs->files,
						       &fct_root, mc, a);
					/* some data may lost */
					if (r == -ERANGE) {
						spin_unlock_irq(&lruvec->lru_lock);
						goto out;
					}
					if (r) {
						if (a)
							an++;
						else
							in++;
					}
				}
				spin_unlock_irq(&lruvec->lru_lock);
			}
		} while ((memcg =  mem_cgroup_iter(root, memcg, NULL)));
	}
out:	/* update final statistics */
	fs->total    = t;
	fs->nomap[0] = an + in;
	fs->nomap[1] = in;
	fs->nomap[2] = an;
}

static int fcmp(const void *x1, const void *x2)
{
	struct file_cache_trace *s1, *s2;

	s1 = (struct file_cache_trace *)x1;
	s2 = (struct file_cache_trace *)x2;
	return s2->count - s1->count;
}

static char *parse_fct_name(struct file_cache_trace *fct, char *buf)
{
	struct address_space *mapping = fct->mapping;
	pgoff_t pgoff;
	struct vm_area_struct *vma;
	struct file *file;

	pgoff = fct->off;
	vma = vma_interval_tree_iter_first(&mapping->i_mmap, pgoff, pgoff);
	if (!vma) {
		pr_err("%s, can't find vma for mapping:%p\n",
		       __func__, mapping);
		return NULL;
	}
	memset(buf, 0, 256);
	file = vma->vm_file;
	if (file) {
		char *p = d_path(&file->f_path, buf, 256);

		if (!IS_ERR(p))
			mangle_path(buf, p, "\n");
		else
			return NULL;
	}
	if (mapping->flags & (1 << 6))
		strncat(buf, " [pin]", 255);

	return buf;
}

#ifdef arch_idle_time
static u64 get_iowait_time(struct kernel_cpustat *kcs, int cpu)
{
	u64 iowait;

	iowait = kcs->cpustat[CPUTIME_IOWAIT];
	if (cpu_online(cpu) && nr_iowait_cpu(cpu))
		iowait += arch_idle_time(cpu);
	return iowait;
}
#else
static u64 get_iowait_time(struct kernel_cpustat *kcs, int cpu)
{
	u64 iowait, iowait_usecs = -1ULL;

	if (cpu_online(cpu))
		iowait_usecs = get_cpu_iowait_time_us(cpu, NULL);

	if (iowait_usecs == -1ULL)
		/* !NO_HZ or cpu offline so we can rely on cpustat.iowait */
		iowait = kcs->cpustat[CPUTIME_IOWAIT];
	else
		iowait = iowait_usecs * NSEC_PER_USEC;

	return iowait;
}
#endif

static u64 get_iow_time(u64 *cpu)
{
	u64 user, nice, system, idle, iowait, irq, softirq, steal;
	u64 guest, guest_nice;
	int i;

	user       = 0;
	nice       = 0;
	system     = 0;
	idle       = 0;
	iowait     = 0;
	irq        = 0;
	softirq    = 0;
	steal      = 0;
	guest      = 0;
	guest_nice = 0;

	for_each_possible_cpu(i) {
		struct kernel_cpustat *kcs = &kcpustat_cpu(i);

		user += kcpustat_cpu(i).cpustat[CPUTIME_USER];
		nice += kcpustat_cpu(i).cpustat[CPUTIME_NICE];
		system += kcpustat_cpu(i).cpustat[CPUTIME_SYSTEM];
		idle += get_idle_time(kcs, i);
		iowait += get_iowait_time(kcs, i);
		irq += kcpustat_cpu(i).cpustat[CPUTIME_IRQ];
		softirq += kcpustat_cpu(i).cpustat[CPUTIME_SOFTIRQ];
		steal += kcpustat_cpu(i).cpustat[CPUTIME_STEAL];
		guest += kcpustat_cpu(i).cpustat[CPUTIME_GUEST];
		guest_nice += kcpustat_cpu(i).cpustat[CPUTIME_GUEST_NICE];
	}
	*cpu = user + nice + system + idle + iowait + irq + softirq + steal +
	       guest + guest_nice;
	return iowait;
}

#define K(x)		((unsigned long)(x) << (PAGE_SHIFT - 10))
static int filecache_show(struct seq_file *m, void *arg)
{
	int i;
	unsigned long tick = 0;
	unsigned long long now;
	u64 iow = 0, cputime = 0;
	char fname[256];
	struct file_cache_trace *fct;
	struct filecache_stat fs = {0};
	unsigned int small_files = 0, small_fcache = 0;
	unsigned int small_active = 0, small_inactive = 0;

	fct  = vzalloc(sizeof(*fct) * MAX_FCT);
	if (!fct)
		return -ENOMEM;

	tick = sched_clock();
	update_file_cache(&fs, fct);
	now  = sched_clock();
	tick = now - tick;

	sort(fct, fs.files, sizeof(struct file_cache_trace), fcmp, NULL);
	seq_puts(m, "------------------------------\n");
	seq_puts(m, "count(KB), active, inactive,     mc,   lc, amc, file name\n");
	for (i = 0; i < fs.files; i++) {
		if (K(fct[i].count) < file_cache_filter) {
			small_files++;
			small_fcache   += fct[i].count;
			small_active   += fct[i].active_count;
			small_inactive += fct[i].inactive_count;
			continue;
		}
		seq_printf(m, "   %6lu, %6lu,   %6lu, %6u, %4ld, %3u, %s\n",
			   K(fct[i].count), K(fct[i].active_count),
			   K(fct[i].inactive_count), fct[i].mapcnt,
			   K(fct[i].lock_count),
			   fct[i].mapcnt / fct[i].count,
			   parse_fct_name(&fct[i], fname));
	}
	iow = get_iow_time(&cputime);
	seq_printf(m, "small files:%u, cache:%lu [%lu/%lu] KB, time:%ld\n",
		   small_files, K(small_fcache),
		   K(small_inactive), K(small_active), tick);
	seq_printf(m, "total:%lu KB, nomap[I/A]:%lu [%lu/%lu] KB, files:%u\n",
		   K(fs.total), K(fs.nomap[0]),
		   K(fs.nomap[1]), K(fs.nomap[2]),
		   fs.files);
	seq_printf(m, "ktime:%12lld, iow:%12lld, cputime:%12lld\n",
		   now, iow, cputime);
	seq_puts(m, "------------------------------\n");
	vfree(fct);
	return 0;
}

static int filecache_open(struct inode *inode, struct file *file)
{
	return single_open(file, filecache_show, NULL);
}

static const struct proc_ops filecache_ops = {
	.proc_open		= filecache_open,
	.proc_read		= seq_read,
	.proc_lseek		= seq_lseek,
	.proc_release		= single_release,
};

static int __init filecache_module_init(void)
{
	d_filecache = proc_create("filecache", 0444,
				  NULL, &filecache_ops);
	if (IS_ERR_OR_NULL(d_filecache)) {
		pr_err("%s, create filecache failed\n", __func__);
		return -1;
	}

	return 0;
}

static void __exit filecache_module_exit(void)
{
	if (d_filecache)
		proc_remove(d_filecache);
}
module_init(filecache_module_init);
module_exit(filecache_module_exit);
