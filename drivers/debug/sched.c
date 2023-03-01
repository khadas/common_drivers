// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#if defined(CONFIG_ANDROID_VENDOR_HOOKS) && defined(CONFIG_FAIR_GROUP_SCHED)

#include <linux/stacktrace.h>
#include <linux/export.h>
#include <linux/types.h>
#include <linux/smp.h>
#include <linux/irqflags.h>
#include <linux/sched.h>
#include <linux/moduleparam.h>
#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/sched/clock.h>
#include <linux/sched/debug.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/arm-smccc.h>
#include <linux/kprobes.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <sched.h>

#include <trace/hooks/sched.h>
#include <trace/events/meson_atrace.h>

static int sched_big_weight = 10; // * NICE_0_LOAD
module_param(sched_big_weight, int, 0644);

static int sched_interactive_task_util = 150;
module_param(sched_interactive_task_util, int, 0644);

static int sched_task_low_prio = 125;
module_param(sched_task_low_prio, int, 0644);

static int sched_task_high_prio = 110;
module_param(sched_task_high_prio, int, 0644);

static int sched_rt_nice_enable;
module_param(sched_rt_nice_enable, int, 0644);

static int sched_rt_nice_debug;
module_param(sched_rt_nice_debug, int, 0644);

static int sched_rt_nice_prio = 110;
module_param(sched_rt_nice_prio, int, 0644);

static unsigned long sched_rt_nice_gran = 4000000; //4ms
module_param(sched_rt_nice_gran, ulong, 0644);

static int sched_check_preempt_wakeup_enable = 1;
module_param(sched_check_preempt_wakeup_enable, int, 0644);

static int sched_check_preempt_wakeup_debug;
module_param(sched_check_preempt_wakeup_debug, int, 0644);

/* default 3ms, same with wakeup_granularity_ns(4*core smp) */
static unsigned long sched_check_preempt_wakeup_gran = 3000000;
module_param(sched_check_preempt_wakeup_gran, ulong, 0644);

static int sched_pick_next_task_enable = 1;
module_param(sched_pick_next_task_enable, int, 0644);

static int sched_pick_next_task_debug;
module_param(sched_pick_next_task_debug, int, 0644);

static int sched_pick_next_task_wait_socre = 10; //1ms+
module_param(sched_pick_next_task_wait_socre, int, 0644);

static int sched_pick_next_task_util_score = 80; //load.util_avg <= 200
module_param(sched_pick_next_task_util_score, int, 0644);

static int sched_pick_next_task_ignore_wait_prio = 120;
module_param(sched_pick_next_task_ignore_wait_prio, int, 0644);

static int sched_place_entity_enable = 1;
module_param(sched_place_entity_enable, int, 0644);

static int sched_place_entity_debug;
module_param(sched_place_entity_debug, int, 0644);

static int sched_place_entity_factor = 3;
module_param(sched_place_entity_factor, int, 0644);

static int sched_check_preempt_tick_enable = 1;
module_param(sched_check_preempt_tick_enable, int, 0644);

static int sched_check_preempt_tick_debug;
module_param(sched_check_preempt_tick_debug, int, 0644);

#ifdef CONFIG_SMP
static inline bool should_honor_rt_sync(struct rq *rq, struct task_struct *p,
					bool sync)
{
	/*
	 * If the waker is CFS, then an RT sync wakeup would preempt the waker
	 * and force it to run for a likely small time after the RT wakee is
	 * done. So, only honor RT sync wakeups from RT wakers.
	 */
	return sync && task_has_rt_policy(rq->curr) &&
		p->prio <= rq->rt.highest_prio.next &&
		rq->rt.rt_nr_running <= 2;
}
#else
static inline bool should_honor_rt_sync(struct rq *rq, struct task_struct *p,
					bool sync)
{
	return 0;
}
#endif

static void aml_select_rt_nice(void *data, struct task_struct *p,
				int prev_cpu, int sd_flag,
				int wake_flags, int *new_cpu)
{
	int test = 0;
	struct rq *rq;
	struct task_struct *curr;
	int this_cpu;
	struct rq *this_cpu_rq;
	unsigned long rtime = 0;
	int lowest_prio_cpu = -1;
	int lowest_prio = -1;
	int tmp_cpu;
	bool sync = !!(wake_flags & WF_SYNC);

	if (!sched_rt_nice_enable)
		return;

	rcu_read_lock();
	rq = cpu_rq(prev_cpu);
	/* coverity[overrun-local] prev_cpu is safe */
	curr = READ_ONCE(rq->curr);
	this_cpu = smp_processor_id();
	this_cpu_rq = cpu_rq(this_cpu);

	if (should_honor_rt_sync(this_cpu_rq, p, sync) &&
	    cpumask_test_cpu(this_cpu, p->cpus_ptr)) {
		*new_cpu = this_cpu;
		goto out_unlock;
	}

	if (!curr)
		goto out_unlock;

	if (task_may_not_preempt(curr, prev_cpu) || rt_task(curr)) {
		test = 1;
	} else if (curr->prio <= sched_rt_nice_prio) {
#ifdef CONFIG_FAIR_GROUP_SCHED
		if (curr->se.depth == 1 &&
		    curr->se.parent->my_q->tg->shares < sched_big_weight * NICE_0_LOAD)
			goto out_unlock;
#endif

		//high prio normal interactive task
		if (curr->se.avg.util_avg >= sched_interactive_task_util)
			goto out_unlock;

		update_rq_clock(rq);

		rtime = curr->se.sum_exec_runtime - curr->se.prev_sum_exec_runtime;
		rtime += (rq_clock_task(rq) - curr->se.exec_start);
		if (rtime >= sched_rt_nice_gran)
			goto out_unlock;

		test = 1;
	}

	if (!test)
		goto out_unlock;

	for_each_cpu(tmp_cpu, p->cpus_ptr) {
		/* coverity[overrun-local] for_each_cpu() is safe */
		struct task_struct *task = READ_ONCE(cpu_rq(tmp_cpu)->curr);

		if (task && task->pid == 0) {
			if (sched_rt_nice_debug)
				aml_trace_printk("wake:%s/%d curr:%s/%d prio=%d util=%lu rtime=%lu idle_cpu:%d\n",
					     p->comm, p->pid, curr->comm, curr->pid,
					     curr->prio, curr->se.avg.util_avg, rtime,
					     tmp_cpu);

			*new_cpu = tmp_cpu;
			goto out_unlock;
		}

#ifdef CONFIG_FAIR_GROUP_SCHED
		if (task && task->se.depth == 1 &&
		    task->se.parent->my_q->tg->shares < sched_big_weight * NICE_0_LOAD) {
			if (sched_rt_nice_debug)
				aml_trace_printk("wake:%s/%d curr:%s/%d prio=%d util=%lu rtime=%lu low_share_group_cpu:%d\n",
					     p->comm, p->pid, curr->comm, curr->pid,
					     curr->prio, curr->se.avg.util_avg, rtime,
					     tmp_cpu);

			*new_cpu = tmp_cpu;
			goto out_unlock;
		}
#endif

		if (task && task->prio > lowest_prio) {
			lowest_prio = task->prio;
			lowest_prio_cpu = tmp_cpu;
		}
	}

	if (lowest_prio_cpu != -1) {
		if (sched_rt_nice_debug)
			aml_trace_printk("wake:%s/%d curr:%s/%d prio=%d util=%lu rtime=%lu lowest_prio_cpu:%d\n",
				     p->comm, p->pid, curr->comm, curr->pid,
				     curr->prio, curr->se.avg.util_avg, rtime,
				     lowest_prio_cpu);
		*new_cpu = lowest_prio_cpu;
	}

out_unlock:
	rcu_read_unlock();
}

static void aml_check_preempt_wakeup(void *data, struct rq *rq, struct task_struct *p, bool *preempt, bool *nopreempt,
				     int wake_flags, struct sched_entity *se, struct sched_entity *pse,
				     int next_buddy_marked, unsigned int granularity)
{
	struct task_struct *curr = rq->curr;
	unsigned long delta_exec = curr->se.sum_exec_runtime - curr->se.prev_sum_exec_runtime;
	int cpu = cpu_of(rq);

	if (!sched_check_preempt_wakeup_enable)
		return;

#ifdef CONFIG_FAIR_GROUP_SCHED
	if (p->se.depth == 1 &&
	    p->se.parent->my_q->tg->shares < sched_big_weight * NICE_0_LOAD) {
		if (sched_check_preempt_wakeup_debug)
			aml_trace_printk("ignore:%d low-share group:%s share=%lu\n",
				     cpu, p->sched_task_group->css.cgroup->kn->name,
				     p->se.parent->my_q->tg->shares);
		 *nopreempt = 1;
		return;
	}

	if (curr->se.depth == 1 &&
	    curr->se.parent->my_q->tg->shares < sched_big_weight * NICE_0_LOAD) {
		if (sched_check_preempt_wakeup_debug)
			aml_trace_printk("resched:%d current low-share group:%s share=%lu\n",
				     cpu, curr->sched_task_group->css.cgroup->kn->name,
				     curr->se.parent->my_q->tg->shares);
		 *preempt = 1;
		return;
	}
#endif

	if (p->prio >= sched_task_low_prio) {
		if (sched_check_preempt_wakeup_debug)
			aml_trace_printk("ignore:%d low-prio task: prio=%d\n", cpu, p->prio);
		 *nopreempt = 1;
		return;
	}

	if (curr->prio >= sched_task_low_prio) {
		if (sched_check_preempt_wakeup_debug)
			aml_trace_printk("resched:%d low-prio current task: prio=%d\n", cpu, p->prio);
		 *preempt = 1;
		return;
	}

	if (curr->prio <= sched_task_high_prio && curr->se.avg.util_avg < sched_interactive_task_util &&
	    delta_exec <= sched_check_preempt_wakeup_gran) {
		if (sched_check_preempt_wakeup_debug)
			aml_trace_printk("ignore:%d current interactive min_gran: delta_exec=%lu\n", cpu, delta_exec);
		 *nopreempt = 1;
		return;
	}

	if (p->prio <= sched_task_high_prio && p->se.avg.util_avg < sched_interactive_task_util) {
		if (sched_check_preempt_wakeup_debug)
			aml_trace_printk("resched:%d new interactive\n", cpu);
		 *preempt = 1;
		return;
	}
}

void set_next_entity(struct cfs_rq *cfs_rq, struct sched_entity *se);

#define __node_2_se(node) \
	rb_entry((node), struct sched_entity, run_node)

static struct sched_entity *___pick_first_entity(struct cfs_rq *cfs_rq)
{
	struct rb_node *left = rb_first_cached(&cfs_rq->tasks_timeline);

	if (!left)
		return NULL;

	return __node_2_se(left);
}

static struct sched_entity *__pick_next_entity(struct sched_entity *se)
{
	struct rb_node *next = rb_next(&se->run_node);

	if (!next)
		return NULL;

	return __node_2_se(next);
}

static inline struct sched_entity *parent_entity(struct sched_entity *se)
{
		return se->parent;
}

static int task_interactive_score(struct task_struct *p, unsigned long weight, int ignore_wait)
{
	int score, weight_score, prio_score, wait_score, util_score;
	unsigned long delta;

	wait_score = 0;

	if (weight < sched_big_weight * NICE_0_LOAD ||
	    p->prio > sched_task_high_prio ||
	    p->se.avg.util_avg >= sched_interactive_task_util)
		return 0;

	weight_score = (weight / NICE_0_LOAD - 10) * 5;  //share 10240 = 0, 20480 = 50, 40960+ = 100;
	if (weight_score > 100)
		weight_score = 100;

	prio_score = (sched_task_high_prio - p->prio) * 10;

	if (!ignore_wait && sched_place_entity_enable) {
		delta = rq_clock(rq_of(p->se.cfs_rq)) - p->android_kabi_reserved1;
		delta = delta >> 20;
		wait_score = delta * 10; //wait 1ms = 10, 10ms = 100, 20ms = 200;

		if (wait_score < sched_pick_next_task_wait_socre)
			return 0;
	}

	util_score = sched_interactive_task_util - p->se.avg.util_avg;

	score = weight_score + prio_score + wait_score + util_score;
	if (sched_pick_next_task_debug)
		aml_trace_printk("interactive_task: %s/%d score:%d/%d,%d,%d,%d, wait:%llu util=%lu\n",
			     p->comm, p->pid, score, weight_score, prio_score, wait_score, util_score,
			     p->android_kabi_reserved1, p->se.avg.util_avg);

	return score;
}

static struct sched_entity *__aml_pick_next_task(struct cfs_rq *cfs_rq, unsigned long weight, int *score, int ignore_wait)
{
	struct sched_entity *se, *ret;
	int max_score = 0;
	int tmp_score;

	*score = 0;
	ret = NULL;

	se = ___pick_first_entity(cfs_rq);

	while (se) {
		if (!entity_is_task(se))
			WARN(1, "not support 2+ level cgroups");

		tmp_score = task_interactive_score(task_of(se), weight, ignore_wait);
		if (tmp_score > max_score) {
			ret = se;
			max_score = tmp_score;
			*score = max_score;
		}

		se = __pick_next_entity(se);
	}

	return ret;
}

static void aml_pick_next_task(void *data, struct rq *rq, struct task_struct **p_new, struct sched_entity **se_new,
			       bool *repick, bool simple, struct task_struct *prev)
{
	struct sched_entity *ret, *p;
	struct sched_entity *se;
	int score, max_score;
	struct task_struct *aml_p = NULL;
	struct sched_entity *aml_se = NULL;
	struct task_struct *curr = rq->curr;
	int ignore_wait = 0;

	if (!sched_pick_next_task_enable)
		return;

	ret = NULL;
	max_score = 0;

	//if current task is big-group interactive task, select it again
	if (!simple && prev->on_rq && prev->se.depth == 1 && prev->se.parent->my_q->tg->shares >= sched_big_weight * NICE_0_LOAD &&
	    task_interactive_score(prev, prev->se.parent->my_q->tg->shares, 1)) {
		if (sched_pick_next_task_debug)
			aml_trace_printk("try_again:%s/%d -> %s/%d\n", (*p_new)->comm, (*p_new)->pid, prev->comm, prev->pid);

		*p_new = prev;
		return;
	}

	if (task_has_dl_policy(curr) || task_has_rt_policy(curr)) {
		ignore_wait = 1;
	} else if (fair_policy(curr->policy)) {
		if ((curr->se.depth == 1 && curr->se.parent->my_q->tg->shares < sched_big_weight * NICE_0_LOAD) ||
		    curr->prio >= sched_pick_next_task_ignore_wait_prio)
			ignore_wait = 1;
	}

	se = ___pick_first_entity(&rq->cfs);

	while (se) {
		if (!entity_is_task(se) && se->my_q->tg->shares >= sched_big_weight * NICE_0_LOAD) {
			p = __aml_pick_next_task(group_cfs_rq(se), se->my_q->tg->shares, &score, ignore_wait);
			if (p && score > max_score) {
				ret = p;
				max_score = score;
			}
		}

		se = __pick_next_entity(se);
	}

	if (!ret)
		return;

	if (simple) {
		aml_se = ret;
		aml_p = task_of(aml_se);

		*p_new = aml_p;

		if (sched_pick_next_task_debug)
			aml_trace_printk("select_simple: %s/%d\n", aml_p->comm, aml_p->pid);

		while (aml_se) {
			set_next_entity(cfs_rq_of(aml_se), aml_se);
			aml_se = parent_entity(aml_se);
		}

		*repick = 1;
	} else {
		aml_se = ret;
		aml_p = task_of(aml_se);

		if (sched_pick_next_task_debug)
			aml_trace_printk("select: %s/%d\n", aml_p->comm, aml_p->pid);

		*p_new = aml_p;
		*se_new = aml_se;
	}
}

static inline u64 max_vruntime(u64 max_vruntime, u64 vruntime)
{
	s64 delta = (s64)(vruntime - max_vruntime);

	if (delta > 0)
		max_vruntime = vruntime;

	return max_vruntime;
}

static void aml_place_entity(void *data, struct cfs_rq *cfs_rq, struct sched_entity *se,
			     int initial, u64 *vruntime)
{
	u64 vruntime_new = cfs_rq->min_vruntime;
	unsigned long thresh;

	if (!sched_place_entity_enable)
		return;

	if (initial)
		return;

	if (sched_place_entity_factor) {
		thresh = sysctl_sched_latency / sched_place_entity_factor;
		vruntime_new -= thresh;

		se->vruntime = max_vruntime(se->vruntime, vruntime_new);

		if (sched_place_entity_debug && entity_is_task(se))
			aml_trace_printk("cpu:%d task:%s/%d(%s) vrutime:%llu(%llu->%llu)\n",
				      cpu_of(rq_of(cfs_rq)),
				      task_of(se)->comm, task_of(se)->pid,
				      task_of(se)->sched_task_group->css.cgroup->kn->name,
				      se->vruntime, cfs_rq->min_vruntime, vruntime_new);
	}

	//task_struct.android_kabi_reserved1: last wakeup time
	if (entity_is_task(se))
		task_of(se)->android_kabi_reserved1 = rq_clock(rq_of(cfs_rq));
}

static void aml_check_preempt_tick(void *data, struct task_struct *p, unsigned long *ideal_runtime,
				   bool *skip_preempt, unsigned long delta_exec, struct cfs_rq *cfs_rq,
				   struct sched_entity *curr, unsigned int granularity)
{
	struct sched_entity *se, *se_long_wait;
	int score;

	if (!sched_check_preempt_tick_enable)
		return;

	if (p->prio >= sched_task_low_prio) {
		if (sched_check_preempt_tick_debug)
			aml_trace_printk("resched: low-prio task_prio=%d\n", p->prio);

		*ideal_runtime = 0; //resched
		return;
	}

	if (p->se.depth == 1 &&
	    p->se.parent->my_q->tg->shares < sched_big_weight * NICE_0_LOAD) {
		if (sched_check_preempt_tick_debug)
			aml_trace_printk("resched: low-share group:%s share=%lu\n",
				     p->sched_task_group->css.cgroup->kn->name,
				     p->se.parent->my_q->tg->shares);
		*ideal_runtime = 0; //resched
		return;
	}

	if (p->prio <= sched_task_high_prio && p->se.avg.util_avg < sched_interactive_task_util &&
	    delta_exec <= sched_check_preempt_wakeup_gran) {
		if (sched_check_preempt_wakeup_debug)
			aml_trace_printk("ignore: current interactive min_gran: delta_exec=%lu\n", delta_exec);
		 *skip_preempt = 1;
		return;
	}

	//if any long_wait big group task exsit
	se = ___pick_first_entity(cfs_rq);

	while (se) {
		if (!entity_is_task(se) && se->my_q->tg->shares >= sched_big_weight * NICE_0_LOAD) {
			se_long_wait  = __aml_pick_next_task(group_cfs_rq(se), se->my_q->tg->shares, &score, 0);
			if (se_long_wait) {
				if (sched_check_preempt_tick_debug)
					aml_trace_printk("resched long_wait task:%s/%d score=%d\n",
						     task_of(se_long_wait)->comm,
						     task_of(se_long_wait)->pid,
						     score);

				*ideal_runtime = 0; //resched
				return;
			}
		}
		se = __pick_next_entity(se);
	}
}

int aml_sched_init(void)
{
	register_trace_android_rvh_select_task_rq_rt(aml_select_rt_nice, NULL);
	register_trace_android_rvh_check_preempt_wakeup(aml_check_preempt_wakeup, NULL);
	register_trace_android_rvh_replace_next_task_fair(aml_pick_next_task, NULL);
	register_trace_android_rvh_place_entity(aml_place_entity, NULL);
	register_trace_android_rvh_check_preempt_tick(aml_check_preempt_tick, NULL);
	return 0;
}
#else
int aml_sched_init(void)
{
	return 0;
}
#endif
