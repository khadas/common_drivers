// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

/*
 * The lowmemorykiller driver lets user-space specify a set of memory thresholds
 * where processes with a range of oom_score_adj values will get killed. Specify
 * the minimum oom_score_adj values in
 * /sys/module/lowmemorykiller/parameters/adj and the number of free pages in
 * /sys/module/lowmemorykiller/parameters/minfree. Both files take a comma
 * separated list of numbers in ascending order.
 *
 * For example, write "0,8" to /sys/module/lowmemorykiller/parameters/adj and
 * "1024,4096" to /sys/module/lowmemorykiller/parameters/minfree to kill
 * processes with a oom_score_adj value of 8 or higher when the free memory
 * drops below 4096 pages and kill processes with a oom_score_adj value of 0 or
 * higher when the free memory drops below 1024 pages.
 *
 * The driver considers memory used for caches to be free, but if a large
 * percentage of the cached memory is locked this can be very inaccurate
 * and processes may not get killed until the normal oom killer is triggered.
 *
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/oom.h>
#include <linux/sched.h>
#include <linux/swap.h>
#include <linux/rcupdate.h>
#include <linux/profile.h>
#include <linux/notifier.h>

#ifdef CONFIG_AMLOGIC_CMA
#include <linux/amlogic/aml_cma.h>
#endif

#define CREATE_TRACE_POINTS

#include "lowmemorykiller.h"

#ifndef CONFIG_AMLOGIC_LMK
#define PFA_LMK_WAITING		12	/* Lowmemorykiller is waiting */

#define TASK_LMK_TEST(name, func)					\
	static inline bool task_##func(struct task_struct *p)		\
	{ return test_bit(PFA_##name, &p->atomic_flags); }

#define TASK_LMK_SET(name, func)					\
	static inline void task_set_##func(struct task_struct *p)	\
	{ set_bit(PFA_##name, &p->atomic_flags); }

TASK_LMK_TEST(LMK_WAITING, lmk_waiting)
TASK_LMK_SET(LMK_WAITING, lmk_waiting)
#endif

static u32 lowmem_debug_level = 1;
static short lowmem_adj[6] = {
	0,
	1,
	6,
	12,
};

static int lowmem_adj_size = 4;
static int lowmem_minfree[6] = {
	3 * 512,	/* 6MB */
	2 * 1024,	/* 8MB */
	4 * 1024,	/* 16MB */
	16 * 1024,	/* 64MB */
};

static int lowmem_minfree_size = 4;

static unsigned long lowmem_deathpending_timeout;

#define lowmem_print(level, x...)			\
	do {						\
		if (lowmem_debug_level >= (level))	\
			pr_info(x);			\
	} while (0)

static unsigned long lowmem_count(struct shrinker *s,
				  struct shrink_control *sc)
{
	return global_node_page_state(NR_ACTIVE_ANON) +
		global_node_page_state(NR_ACTIVE_FILE) +
		global_node_page_state(NR_INACTIVE_ANON) +
		global_node_page_state(NR_INACTIVE_FILE);
}

#ifdef CONFIG_AMLOGIC_MEMORY_EXTEND
static unsigned long forgeround_jiffes;
static void show_task_adj(void)
{
#define SHOW_PRIFIX	"score_adj:%5d, rss:%5lu"
	struct task_struct *tsk;
	int tasksize;

	/* avoid print too many */
	if (time_after(forgeround_jiffes, jiffies))
		return;

	forgeround_jiffes = jiffies + HZ * 5;
	show_mem(0, NULL);
	lowmem_print(1, "Foreground task killed, show all Candidates\n");
	for_each_process(tsk) {
		struct task_struct *p;
		short oom_score_adj;

		if (tsk->flags & PF_KTHREAD)
			continue;
		p = find_lock_task_mm(tsk);
		if (!p)
			continue;
		oom_score_adj = p->signal->oom_score_adj;
		tasksize = get_mm_rss(p->mm);
		task_unlock(p);
	#ifdef CONFIG_ZRAM
		lowmem_print(1, SHOW_PRIFIX ", rswap:%5lu, task:%5d, %s\n",
			     oom_score_adj, get_mm_rss(p->mm),
			     get_mm_counter(p->mm, MM_SWAPENTS),
			     p->pid, p->comm);
	#else
		lowmem_print(1, SHOW_PRIFIX ", task:%5d, %s\n",
			     oom_score_adj, get_mm_rss(p->mm),
			     p->pid, p->comm);
	#endif /* CONFIG_ZRAM */
	}
}
#endif /* CONFIG_AMLOGIC_MEMORY_EXTEND */

static unsigned long lowmem_scan(struct shrinker *s, struct shrink_control *sc)
{
	struct task_struct *tsk;
	struct task_struct *selected = NULL;
	unsigned long rem = 0;
	int tasksize;
	int i;
	short min_score_adj = OOM_SCORE_ADJ_MAX + 1;
	int minfree = 0;
	int selected_tasksize = 0;
	short selected_oom_score_adj;
#ifdef CONFIG_AMLOGIC_CMA
	int free_cma   = 0;
	int file_cma   = 0;
	int cma_forbid = 0;
#endif
	int array_size = ARRAY_SIZE(lowmem_adj);
	int other_free = global_zone_page_state(NR_FREE_PAGES);
	int other_file = global_node_page_state(NR_FILE_PAGES) -
				global_node_page_state(NR_SHMEM) -
				global_node_page_state(NR_UNEVICTABLE) -
				total_swapcache_pages();

	if (time_before_eq(jiffies, lowmem_deathpending_timeout))
		return 0;

#ifdef CONFIG_AMLOGIC_CMA
	if (cma_forbidden_mask(sc->gfp_mask) && !current_is_kswapd()) {
		free_cma    = global_zone_page_state(NR_FREE_CMA_PAGES);
		other_free -= free_cma;
#ifdef CONFIG_AMLOGIC_LMK
		file_cma    = global_zone_page_state(NR_INACTIVE_FILE_CMA) +
			      global_zone_page_state(NR_ACTIVE_FILE_CMA);
		other_file -= file_cma;
#endif
		cma_forbid  = 1;
	}
#endif /* CONFIG_AMLOGIC_CMA */

	if (lowmem_adj_size < array_size)
		array_size = lowmem_adj_size;
	if (lowmem_minfree_size < array_size)
		array_size = lowmem_minfree_size;
	for (i = 0; i < array_size; i++) {
		minfree = lowmem_minfree[i];
		if (other_free < minfree && other_file < minfree) {
			min_score_adj = lowmem_adj[i];
			break;
		}
	}

	if (min_score_adj == OOM_SCORE_ADJ_MAX + 1) {
		lowmem_print(5, "lowmem scan %lu, %x, return 0\n",
			     sc->nr_to_scan, sc->gfp_mask);
		return 0;
	}

	lowmem_print(3, "lowmem scan %lu, %x, ofree %d %d, ma %hd\n",
		     sc->nr_to_scan, sc->gfp_mask, other_free,
		     other_file, min_score_adj);

	selected_oom_score_adj = min_score_adj;

	rcu_read_lock();
	for_each_process(tsk) {
		struct task_struct *p;
		short oom_score_adj;

		if (tsk->flags & PF_KTHREAD)
			continue;

		p = find_lock_task_mm(tsk);
		if (!p)
			continue;

		if (task_lmk_waiting(p)) {
			task_unlock(p);
			rcu_read_unlock();
			return 0;
		}
		oom_score_adj = p->signal->oom_score_adj;
		if (oom_score_adj < min_score_adj) {
			task_unlock(p);
			continue;
		}
		tasksize = get_mm_rss(p->mm);
		task_unlock(p);
		if (tasksize <= 0)
			continue;
		if (selected) {
			if (oom_score_adj < selected_oom_score_adj)
				continue;
			if (oom_score_adj == selected_oom_score_adj &&
			    tasksize <= selected_tasksize)
				continue;
		}
		selected = p;
		selected_tasksize = tasksize;
		selected_oom_score_adj = oom_score_adj;
		lowmem_print(2, "select '%s' (%d), adj %hd, size %d, to kill\n",
			     p->comm, p->pid, oom_score_adj, tasksize);
	}
	if (selected) {
		long cache_size = other_file * (long)(PAGE_SIZE / 1024);
		long cache_limit = minfree * (long)(PAGE_SIZE / 1024);
		long free = other_free * (long)(PAGE_SIZE / 1024);

		task_lock(selected);
		send_sig(SIGKILL, selected, 0);
		if (selected->mm)
			task_set_lmk_waiting(selected);
		task_unlock(selected);
		trace_lowmemory_kill(selected, cache_size, cache_limit, free);
		lowmem_print(1, "Killing '%s' (%d) (tgid %d), adj %hd,\n"
				 "   to free %ldkB on behalf of '%s' (%d) because\n"
				 "   cache %ldkB is below limit %ldkB for oom_score_adj %hd\n"
				 "   Free memory is %ldkB above reserved\n",
			     selected->comm, selected->pid, selected->tgid,
			     selected_oom_score_adj,
			     selected_tasksize * (long)(PAGE_SIZE / 1024),
			     current->comm, current->pid,
			     cache_size, cache_limit,
			     min_score_adj,
			     free);
	#ifdef CONFIG_AMLOGIC_CMA
		if (cma_forbid) {
			/* kill quickly if can't use cma */
			lowmem_deathpending_timeout = jiffies + HZ / 2;
			pr_info("   Free cma:%ldkB, file cma:%ldkB\n",
				free_cma * (long)(PAGE_SIZE / 1024),
				file_cma * (long)(PAGE_SIZE / 1024));
		} else {
			lowmem_deathpending_timeout = jiffies + HZ;
		}
	#else
		lowmem_deathpending_timeout = jiffies + HZ;
	#endif /* CONFIG_AMLOGIC_CMA */
		rem += selected_tasksize;
	#ifdef CONFIG_AMLOGIC_MEMORY_EXTEND
		if (!selected_oom_score_adj) /* forgeround task killed */
			show_task_adj();
	#endif /* CONFIG_AMLOGIC_MEMORY_EXTEND */
	}

	lowmem_print(4, "lowmem scan %lu, %x, return %lu\n",
		     sc->nr_to_scan, sc->gfp_mask, rem);
	rcu_read_unlock();
	return rem;
}

static struct shrinker lowmem_shrinker = {
	.scan_objects = lowmem_scan,
	.count_objects = lowmem_count,
	.seeks = DEFAULT_SEEKS * 16
};

static int __init lowmem_init(void)
{
	register_shrinker(&lowmem_shrinker);
	return 0;
}
device_initcall(lowmem_init);

#ifdef CONFIG_ANDROID_LOW_MEMORY_KILLER_AUTODETECT_OOM_ADJ_VALUES
static short lowmem_oom_adj_to_oom_score_adj(short oom_adj)
{
	if (oom_adj == OOM_ADJUST_MAX)
		return OOM_SCORE_ADJ_MAX;
	else
		return (oom_adj * OOM_SCORE_ADJ_MAX) / -OOM_DISABLE;
}

static void lowmem_autodetect_oom_adj_values(void)
{
	int i;
	short oom_adj;
	short oom_score_adj;
	int array_size = ARRAY_SIZE(lowmem_adj);

	if (lowmem_adj_size < array_size)
		array_size = lowmem_adj_size;

	if (array_size <= 0)
		return;

	oom_adj = lowmem_adj[array_size - 1];
	if (oom_adj > OOM_ADJUST_MAX)
		return;

	oom_score_adj = lowmem_oom_adj_to_oom_score_adj(oom_adj);
	if (oom_score_adj <= OOM_ADJUST_MAX)
		return;

	lowmem_print(1, "lowmem_shrink: convert oom_adj to oom_score_adj:\n");
	for (i = 0; i < array_size; i++) {
		oom_adj = lowmem_adj[i];
		oom_score_adj = lowmem_oom_adj_to_oom_score_adj(oom_adj);
		lowmem_adj[i] = oom_score_adj;
		lowmem_print(1, "oom_adj %d => oom_score_adj %d\n",
			     oom_adj, oom_score_adj);
	}
}

static int lowmem_adj_array_set(const char *val, const struct kernel_param *kp)
{
	int ret;

	ret = param_array_ops.set(val, kp);

	/* HACK: Autodetect oom_adj values in lowmem_adj array */
	lowmem_autodetect_oom_adj_values();

	return ret;
}

static int lowmem_adj_array_get(char *buffer, const struct kernel_param *kp)
{
	return param_array_ops.get(buffer, kp);
}

static void lowmem_adj_array_free(void *arg)
{
	param_array_ops.free(arg);
}

static struct kernel_param_ops lowmem_adj_array_ops = {
	.set = lowmem_adj_array_set,
	.get = lowmem_adj_array_get,
	.free = lowmem_adj_array_free,
};

static const struct kparam_array __param_arr_adj = {
	.max = ARRAY_SIZE(lowmem_adj),
	.num = &lowmem_adj_size,
	.ops = &param_ops_short,
	.elemsize = sizeof(lowmem_adj[0]),
	.elem = lowmem_adj,
};
#endif

/*
 * not really modular, but the easiest way to keep compat with existing
 * bootargs behaviour is to continue using module_param here.
 */
module_param_named(cost, lowmem_shrinker.seeks, int, 0644);
#ifdef CONFIG_ANDROID_LOW_MEMORY_KILLER_AUTODETECT_OOM_ADJ_VALUES
module_param_cb(adj, &lowmem_adj_array_ops,
		.arr = &__param_arr_adj,
		0644);
__MODULE_PARM_TYPE(adj, "array of short");
#else
module_param_array_named(adj, lowmem_adj, short, &lowmem_adj_size, 0644);
#endif
module_param_array_named(minfree, lowmem_minfree, uint, &lowmem_minfree_size,
			 0644);
module_param_named(debug_level, lowmem_debug_level, uint, 0644);

