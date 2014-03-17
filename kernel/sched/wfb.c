/*
 * WFB Scheduling Class - implementation copied from the real-time scheduler
 * MAR 16 2014 Derick Beckwith
 */
#ifdef CONFIG_SCHED_WFB

/*
 * Adding/removing a task to/from a priority array:
 */
static void
enqueue_task_wfb(struct rq *rq, struct task_struct *p, int flags)
{
	struct sched_rt_entity *rt_se = &p->rt;

	if (flags & ENQUEUE_WAKEUP)
		rt_se->timeout = 0;

	enqueue_rt_entity(rt_se, flags & ENQUEUE_HEAD);

	if (!task_current(rq, p) && p->nr_cpus_allowed > 1)
		enqueue_pushable_task(rq, p);

	inc_nr_running(rq);
}

static void dequeue_task_wfb(struct rq *rq, struct task_struct *p, int flags)
{
	struct sched_rt_entity *rt_se = &p->rt;

	update_curr_rt(rq);
	dequeue_rt_entity(rt_se);

	dequeue_pushable_task(rq, p);

	dec_nr_running(rq);
}

static void yield_task_wfb(struct rq *rq)
{
	requeue_task_rt(rq, rq->curr, 0);
}

static int
select_task_rq_wfb(struct task_struct *p, int cpu, int sd_flag, int flags)
{
	struct task_struct *curr;
	struct rq *rq;

	if (p->nr_cpus_allowed == 1)
		goto out;

	/* For anything but wake ups, just return the task_cpu */
	if (sd_flag != SD_BALANCE_WAKE && sd_flag != SD_BALANCE_FORK)
		goto out;

	rq = cpu_rq(cpu);

	rcu_read_lock();
	curr = ACCESS_ONCE(rq->curr); /* unlocked access */

	/*
	 * If the current task on @p's runqueue is an RT task, then
	 * try to see if we can wake this RT task up on another
	 * runqueue. Otherwise simply start this RT task
	 * on its current runqueue.
	 *
	 * We want to avoid overloading runqueues. If the woken
	 * task is a higher priority, then it will stay on this CPU
	 * and the lower prio task should be moved to another CPU.
	 * Even though this will probably make the lower prio task
	 * lose its cache, we do not want to bounce a higher task
	 * around just because it gave up its CPU, perhaps for a
	 * lock?
	 *
	 * For equal prio tasks, we just let the scheduler sort it out.
	 *
	 * Otherwise, just let it ride on the affined RQ and the
	 * post-schedule router will push the preempted task away
	 *
	 * This test is optimistic, if we get it wrong the load-balancer
	 * will have to sort it out.
	 */
	if (curr && unlikely(rt_task(curr)) &&
	    (curr->nr_cpus_allowed < 2 ||
	     curr->prio <= p->prio)) {
		int target = find_lowest_rq(p);

		if (target != -1)
			cpu = target;
	}
	rcu_read_unlock();

out:
	return cpu;
}

/*
 * Preempt the current task with a newly woken task if needed:
 */
static void check_preempt_curr_wfb(struct rq *rq, struct task_struct *p, int flags)
{
	if (p->prio < rq->curr->prio) {
		resched_task(rq->curr);
		return;
	}

#ifdef CONFIG_SMP
	/*
	 * If:
	 *
	 * - the newly woken task is of equal priority to the current task
	 * - the newly woken task is non-migratable while current is migratable
	 * - current will be preempted on the next reschedule
	 *
	 * we should check to see if current can readily move to a different
	 * cpu.  If so, we will reschedule to allow the push logic to try
	 * to move current somewhere else, making room for our non-migratable
	 * task.
	 */
	if (p->prio == rq->curr->prio && !test_tsk_need_resched(rq->curr))
		check_preempt_equal_prio(rq, p);
#endif
}

static struct task_struct *pick_next_task_wfb(struct rq *rq)
{
	struct task_struct *p = _pick_next_task_rt(rq);

	/* The running task is never eligible for pushing */
	if (p)
		dequeue_pushable_task(rq, p);

#ifdef CONFIG_SMP
	/*
	 * We detect this state here so that we can avoid taking the RQ
	 * lock again later if there is no need to push
	 */
	rq->post_schedule = has_pushable_tasks(rq);
#endif

	return p;
}

static void put_prev_task_wfb(struct rq *rq, struct task_struct *p)
{
	update_curr_rt(rq);

	/*
	 * The previous task needs to be made eligible for pushing
	 * if it is still active
	 */
	if (on_rt_rq(&p->rt) && p->nr_cpus_allowed > 1)
		enqueue_pushable_task(rq, p);
}

static void pre_schedule_wfb(struct rq *rq, struct task_struct *prev)
{
	/* Try to pull RT tasks here if we lower this rq's prio */
	if (rq->rt.highest_prio.curr > prev->prio)
		pull_rt_task(rq);
}

static void post_schedule_wfb(struct rq *rq)
{
	push_rt_tasks(rq);
}

/*
 * If we are not running and we are not going to reschedule soon, we should
 * try to push tasks away now
 */
static void task_woken_wfb(struct rq *rq, struct task_struct *p)
{
	if (!task_running(rq, p) &&
	    !test_tsk_need_resched(rq->curr) &&
	    has_pushable_tasks(rq) &&
	    p->nr_cpus_allowed > 1 &&
	    (dl_task(rq->curr) || rt_task(rq->curr)) &&
	    (rq->curr->nr_cpus_allowed < 2 ||
	     rq->curr->prio <= p->prio))
		push_rt_tasks(rq);
}

static void set_cpus_allowed_wfb(struct task_struct *p,
				const struct cpumask *new_mask)
{
	struct rq *rq;
	int weight;

	BUG_ON(!rt_task(p));

	if (!p->on_rq)
		return;

	weight = cpumask_weight(new_mask);

	/*
	 * Only update if the process changes its state from whether it
	 * can migrate or not.
	 */
	if ((p->nr_cpus_allowed > 1) == (weight > 1))
		return;

	rq = task_rq(p);

	/*
	 * The process used to be able to migrate OR it can now migrate
	 */
	if (weight <= 1) {
		if (!task_current(rq, p))
			dequeue_pushable_task(rq, p);
		BUG_ON(!rq->rt.rt_nr_migratory);
		rq->rt.rt_nr_migratory--;
	} else {
		if (!task_current(rq, p))
			enqueue_pushable_task(rq, p);
		rq->rt.rt_nr_migratory++;
	}

	update_rt_migration(&rq->rt);
}

/* Assumes rq->lock is held */
static void rq_online_wfb(struct rq *rq)
{
	if (rq->rt.overloaded)
		rt_set_overload(rq);

	__enable_runtime(rq);

	cpupri_set(&rq->rd->cpupri, rq->cpu, rq->rt.highest_prio.curr);
}

/* Assumes rq->lock is held */
static void rq_offline_wfb(struct rq *rq)
{
	if (rq->rt.overloaded)
		rt_clear_overload(rq);

	__disable_runtime(rq);

	cpupri_set(&rq->rd->cpupri, rq->cpu, CPUPRI_INVALID);
}

/*
 * When switch from the rt queue, we bring ourselves to a position
 * that we might want to pull RT tasks from other runqueues.
 */
static void switched_from_wfb(struct rq *rq, struct task_struct *p)
{
	/*
	 * If there are other RT tasks then we will reschedule
	 * and the scheduling of the other RT tasks will handle
	 * the balancing. But if we are the last RT task
	 * we may need to handle the pulling of RT tasks
	 * now.
	 */
	if (!p->on_rq || rq->rt.rt_nr_running)
		return;

	if (pull_rt_task(rq))
		resched_task(rq->curr);
}

/*
 * When switching a task to RT, we may overload the runqueue
 * with RT tasks. In this case we try to push them off to
 * other runqueues.
 */
static void switched_to_wfb(struct rq *rq, struct task_struct *p)
{
	int check_resched = 1;

	/*
	 * If we are already running, then there's nothing
	 * that needs to be done. But if we are not running
	 * we may need to preempt the current running task.
	 * If that current running task is also an RT task
	 * then see if we can move to another run queue.
	 */
	if (p->on_rq && rq->curr != p) {
#ifdef CONFIG_SMP
		if (rq->rt.overloaded && push_rt_task(rq) &&
		    /* Don't resched if we changed runqueues */
		    rq != task_rq(p))
			check_resched = 0;
#endif /* CONFIG_SMP */
		if (check_resched && p->prio < rq->curr->prio)
			resched_task(rq->curr);
	}
}

/*
 * Priority of the task has changed. This may cause
 * us to initiate a push or pull.
 */
static void
prio_changed_wfb(struct rq *rq, struct task_struct *p, int oldprio)
{
	if (!p->on_rq)
		return;

	if (rq->curr == p) {
#ifdef CONFIG_SMP
		/*
		 * If our priority decreases while running, we
		 * may need to pull tasks to this runqueue.
		 */
		if (oldprio < p->prio)
			pull_rt_task(rq);
		/*
		 * If there's a higher priority task waiting to run
		 * then reschedule. Note, the above pull_rt_task
		 * can release the rq lock and p could migrate.
		 * Only reschedule if p is still on the same runqueue.
		 */
		if (p->prio > rq->rt.highest_prio.curr && rq->curr == p)
			resched_task(p);
#else
		/* For UP simply resched on drop of prio */
		if (oldprio < p->prio)
			resched_task(p);
#endif /* CONFIG_SMP */
	} else {
		/*
		 * This task is not running, but if it is
		 * greater than the current running task
		 * then reschedule.
		 */
		if (p->prio < rq->curr->prio)
			resched_task(rq->curr);
	}
}

static void task_tick_wfb(struct rq *rq, struct task_struct *p, int queued)
{
	struct sched_rt_entity *rt_se = &p->rt;

	update_curr_rt(rq);

	watchdog(rq, p);

	/*
	 * RR tasks need a special form of timeslice management.
	 * FIFO tasks have no timeslices.
	 */
	if (p->policy != SCHED_RR)
		return;

	if (--p->rt.time_slice)
		return;

	p->rt.time_slice = sched_rr_timeslice;

	/*
	 * Requeue to the end of queue if we (and all of our ancestors) are not
	 * the only element on the queue
	 */
	for_each_sched_rt_entity(rt_se) {
		if (rt_se->run_list.prev != rt_se->run_list.next) {
			requeue_task_rt(rq, p, 0);
			set_tsk_need_resched(p);
			return;
		}
	}
}

static void set_curr_task_wfb(struct rq *rq)
{
	struct task_struct *p = rq->curr;

	p->se.exec_start = rq_clock_task(rq);

	/* The running task is never eligible for pushing */
	dequeue_pushable_task(rq, p);
}

static unsigned int get_rr_interval_wfb(struct rq *rq, struct task_struct *task)
{
	/*
	 * Time slice is 0 for SCHED_FIFO tasks
	 */
	if (task->policy == SCHED_RR)
		return sched_rr_timeslice;
	else
		return 0;
}

const struct sched_class wfb_sched_class = {
	.next			= &fair_sched_class,
	.enqueue_task		= enqueue_task_wfb,
	.dequeue_task		= dequeue_task_wfb,
	.yield_task		= yield_task_wfb,

	.check_preempt_curr	= check_preempt_curr_wfb,

	.pick_next_task		= pick_next_task_wfb,
	.put_prev_task		= put_prev_task_wfb,

#ifdef CONFIG_SMP
	.select_task_rq		= select_task_rq_wfb,

	.set_cpus_allowed       = set_cpus_allowed_wfb,
	.rq_online              = rq_online_wfb,
	.rq_offline             = rq_offline_wfb,
	.pre_schedule		= pre_schedule_wfb,
	.post_schedule		= post_schedule_wfb,
	.task_woken		= task_woken_wfb,
	.switched_from		= switched_from_wfb,
#endif

	.set_curr_task          = set_curr_task_wfb,
	.task_tick		= task_tick_wfb,

	.get_rr_interval	= get_rr_interval_wfb,

	.prio_changed		= prio_changed_wfb,
	.switched_to		= switched_to_wfb,
#ifdef CONFIG_FAIR_GROUP_SCHED
    .moved_group            = NULL
#endif
};

#endif /* CONFIG_SCHED_WFB */
