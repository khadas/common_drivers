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
#include <trace/hooks/mm.h>

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

static inline unsigned long vma_start_pgoff(struct vm_area_struct *v)
{
	return v->vm_pgoff;
}

static inline unsigned long vma_last_pgoff(struct vm_area_struct *v)
{
	return v->vm_pgoff + vma_pages(v) - 1;
}

/*
 * Iterate over intervals intersecting [start;last]
 *
 * Note that a node's interval intersects [start;last] iff:
 *   Cond1: vma_start_pgoff(node) <= last
 * and
 *   Cond2: start <= vma_last_pgoff(node)
 */

static struct vm_area_struct *
aml_vma_subtree_search(struct vm_area_struct *node, unsigned long start, unsigned long last)
{
	while (true) {
		/*
		 * Loop invariant: start <= node->shared.rb_subtree_last
		 * (Cond2 is satisfied by one of the subtree nodes)
		 */
		if (node->shared.rb.rb_left) {
			struct vm_area_struct *left = rb_entry(node->shared.rb.rb_left,
						  struct vm_area_struct, shared.rb);
			if (start <= left->shared.rb_subtree_last) {
				/*
				 * Some nodes in left subtree satisfy Cond2.
				 * Iterate to find the leftmost such node N.
				 * If it also satisfies Cond1, that's the
				 * match we are looking for. Otherwise, there
				 * is no matching interval as nodes to the
				 * right of N can't satisfy Cond1 either.
				 */
				node = left;
				continue;
			}
		}
		if (vma_start_pgoff(node) <= last) {		/* Cond1 */
			if (start <= vma_last_pgoff(node))	/* Cond2 */
				return node;	/* node is leftmost match */
			if (node->shared.rb.rb_right) {
				node = rb_entry(node->shared.rb.rb_right,
						struct vm_area_struct, shared.rb);
				if (start <= node->shared.rb_subtree_last)
					continue;
			}
		}
		return NULL;	/* No match */
	}
}

static struct vm_area_struct *
aml_vma_iter_first(struct rb_root_cached *root,
			unsigned long start, unsigned long last)
{
	struct vm_area_struct *node, *leftmost;

	if (!root->rb_root.rb_node)
		return NULL;

	/*
	 * Fastpath range intersection/overlap between A: [a0, a1] and
	 * B: [b0, b1] is given by:
	 *
	 *         a0 <= b1 && b0 <= a1
	 *
	 *  ... where A holds the lock range and B holds the smallest
	 * 'start' and largest 'last' in the tree. For the later, we
	 * rely on the root node, which by augmented interval tree
	 * property, holds the largest value in its last-in-subtree.
	 * This allows mitigating some of the tree walk overhead for
	 * non-intersecting ranges, maintained and consulted in O(1).
	 */
	node = rb_entry(root->rb_root.rb_node, struct vm_area_struct, shared.rb);
	if (node->shared.rb_subtree_last < start)
		return NULL;

	leftmost = rb_entry(root->rb_leftmost, struct vm_area_struct, shared.rb);
	if (vma_start_pgoff(leftmost) > last)
		return NULL;

	return aml_vma_subtree_search(node, start, last);
}

#ifdef CONFIG_MEMCG
struct mem_cgroup *aml_root_mem_cgroup;

#if defined(CONFIG_TRACEPOINTS) && defined(CONFIG_ANDROID_VENDOR_HOOKS)
void get_root_memcg_hook(void *data, struct mem_cgroup *memcg)
{
	if (!aml_root_mem_cgroup)
		aml_root_mem_cgroup = memcg;
}
#endif

static struct mem_cgroup *aml_mem_cgroup_iter(struct mem_cgroup *root,
				   struct mem_cgroup *prev,
				   struct mem_cgroup_reclaim_cookie *reclaim)
{
	struct cgroup_subsys_state *css = NULL;
	struct mem_cgroup *memcg = NULL;
	struct mem_cgroup *pos = NULL;

	if (mem_cgroup_disabled())
		return NULL;

	if (!root)
		root = aml_root_mem_cgroup;

	if (prev && !reclaim)
		pos = prev;

	rcu_read_lock();

	if (pos)
		css = &pos->css;

	for (;;) {
		css = css_next_descendant_pre(css, &root->css);
		if (!css) {
			/*
			 * Reclaimers share the hierarchy walk, and a
			 * new one might jump in right at the end of
			 * the hierarchy - make sure they see at least
			 * one group and restart from the beginning.
			 */
			if (!prev)
				continue;
			break;
		}

		/*
		 * Verify the css and acquire a reference.  The root
		 * is provided by the caller, so we know it's alive
		 * and kicking, and don't take an extra reference.
		 */
		memcg = mem_cgroup_from_css(css);

		if (css == &root->css)
			break;

		if (css_tryget(css))
			break;

		memcg = NULL;
	}

	rcu_read_unlock();
	if (prev && prev != root)
		css_put(&prev->css);

	return memcg;
}

static inline struct lruvec *aml_mem_cgroup_lruvec(struct mem_cgroup *memcg,
					       struct pglist_data *pgdat)
{
	struct mem_cgroup_per_node *mz;
	struct lruvec *lruvec;

	if (mem_cgroup_disabled()) {
		lruvec = &pgdat->__lruvec;
		goto out;
	}

	if (!memcg)
		memcg = aml_root_mem_cgroup;

	mz = memcg->nodeinfo[pgdat->node_id];
	lruvec = &mz->lruvec;
out:
	/*
	 * Since a node can be onlined after the mem_cgroup was created,
	 * we have to be prepared to initialize lruvec->pgdat here;
	 * and if offlined then back onlined, we need to reinitialize it.
	 */
	if (unlikely(lruvec->pgdat != pgdat))
		lruvec->pgdat = pgdat;
	return lruvec;
}
#endif

struct pglist_data *aml_first_online_pgdat(void)
{
	return NODE_DATA(first_online_node);
}

struct pglist_data *aml_next_online_pgdat(struct pglist_data *pgdat)
{
	int nid = next_online_node(pgdat->node_id);

	/*
	 * this code copy from next_online_pgdat().
	 */
	// coverity[DEADCODE]
	if (nid == MAX_NUMNODES)
		return NULL;
	return NODE_DATA(nid);
}

static void statistic_filecache_info(struct filecache_stat *fs,
		struct file_cache_trace *fct, struct lruvec *lruvec)
{
	unsigned int t = 0, in = 0, an = 0;
	int r, mc, lru = 0, a = 0;
	struct list_head *list;
	struct page *page, *next;
	struct rb_root fct_root = RB_ROOT;

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
out:	/* update final statistics */
	fs->total    += t;
	fs->nomap[0] += an + in;
	fs->nomap[1] += in;
	fs->nomap[2] += an;
}

static void update_file_cache(struct filecache_stat *fs,
		struct file_cache_trace *fct)
{
	struct lruvec *lruvec;
	pg_data_t *pgdat, *tmp;
	struct mem_cgroup *root = NULL, *memcg;

	/* for_each_online_pgdat(pgdat) { */
	for (pgdat = aml_first_online_pgdat(); pgdat; pgdat = aml_next_online_pgdat(pgdat)) {
#ifdef CONFIG_MEMCG
		if (!aml_root_mem_cgroup)
			return;
		memcg = aml_mem_cgroup_iter(root, NULL, NULL);
#else
		memcg = mem_cgroup_iter(root, NULL, NULL);
#endif
		do {
#ifdef CONFIG_MEMCG
			lruvec = aml_mem_cgroup_lruvec(memcg, pgdat);
#else
			lruvec = mem_cgroup_lruvec(memcg, pgdat);
#endif
			tmp = lruvec_pgdat(lruvec);

			statistic_filecache_info(fs, fct, lruvec);
#ifdef CONFIG_MEMCG
		} while ((memcg =  aml_mem_cgroup_iter(root, memcg, NULL)));
#else
		} while ((memcg =  mem_cgroup_iter(root, memcg, NULL)));
#endif
	}
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
	vma = aml_vma_iter_first(&mapping->i_mmap, pgoff, pgoff);
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
static u64 aml_get_idle_time(struct kernel_cpustat *kcs, int cpu)
{
	u64 idle, idle_usecs = -1ULL;

	if (cpu_online(cpu))
		idle_usecs = get_cpu_idle_time_us(cpu, NULL);

	if (idle_usecs == -1ULL)
		/* !NO_HZ or cpu offline so we can rely on cpustat.idle */
		idle = kcs->cpustat[CPUTIME_IDLE];
	else
		idle = idle_usecs * NSEC_PER_USEC;

	return idle;
}

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
		idle += aml_get_idle_time(kcs, i);
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

int __init filecache_module_init(void)
{
	d_filecache = proc_create("filecache", 0444,
				  NULL, &filecache_ops);
	if (IS_ERR_OR_NULL(d_filecache)) {
		pr_err("%s, create filecache failed\n", __func__);
		return -1;
	}

#if defined(CONFIG_TRACEPOINTS) && defined(CONFIG_ANDROID_VENDOR_HOOKS) && defined(CONFIG_MEMCG)
	register_trace_android_vh_mem_cgroup_alloc(get_root_memcg_hook, NULL);
#endif
	return 0;
}

void __exit filecache_module_exit(void)
{
	if (d_filecache)
		proc_remove(d_filecache);
#if defined(CONFIG_TRACEPOINTS) && defined(CONFIG_ANDROID_VENDOR_HOOKS) && defined(CONFIG_MEMCG)
	unregister_trace_android_vh_mem_cgroup_alloc(get_root_memcg_hook, NULL);
#endif
}

