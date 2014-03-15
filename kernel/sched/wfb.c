#ifdef CONFIG_SCHED_WFB

/*
 * Starting with a simple, 1 runq per cpu scheduler.  Don't care
 * about fairness for right now.  Just get it up and running to 
 * verify that we have the interface correct
 */

static void enqueue_task_wfb(struct rq *rq, struct task_struct *p, int wakeup, bool head)
{
}

static void dequeue_task_wfb(struct rq *rq, struct task_struct *p, int sleep)
{
}

static void yield_task_wfb(struct rq *rq)
{
}

static void check_preempt_curr_wfb(struct rq *rq, struct task_struct *p, int flags)
{
}

static struct task_struct *pick_next_task_wfb(struct rq *rq)
{
}

static void put_prev_task_wfb(struct rq *rq, struct task_struct *p)
{
}

#ifdef CONFIG_SMP
static int select_task_rq_wfb(struct task_struct *p, int sd_flag, int flags)
{
}
static void pre_schedule_wfb(struct rq *rq, struct task_struct *prev)
{
}

static void post_schedule_wfb(struct rq *rq)
{
}

static void task_woken_wfb(struct rq *rq, struct task_struct *p)
{
}

static void task_waking_wfb(struct rq *this_rq, struct task_struct *task)
{
}
static void set_cpus_allowed_wfb(struct task_struct *p,
               const struct cpumask *new_mask)
{
}
/* Assumes rq->lock is held */
static void rq_online_wfb(struct rq *rq)
{
}

/* Assumes rq->lock is held */
static void rq_offline_wfb(struct rq *rq)
{
}
#endif /* COMFIG_SMP */

static void set_curr_task_wfb(struct rq *rq)
{
}


static void task_tick_wfb(struct rq *rq, struct task_struct *p, int queued)
{
} 

static void task_fork_wfb(struct task_struct *p)
{
}
static void switched_from_wfb(struct rq *rq, struct task_struct *p,
              int running)
{
}
static void switched_to_wfb(struct rq *this_rq, struct task_struct *task,
               int running)
{
}
static void prio_changed_wfb(struct rq *rq, struct task_struct *p,
               int oldprio, int running)
{
}
static unsigned int get_rr_interval_wfb(struct rq *rq, struct task_struct *task)
{
}



static const struct sched_class wfb_sched_class = {
   .next           = &fair_sched_class,
   .enqueue_task       = enqueue_task_wfb,
   .dequeue_task       = dequeue_task_wfb,
   .yield_task     = yield_task_wfb,

   .check_preempt_curr = check_preempt_curr_wfb,

   .pick_next_task     = pick_next_task_wfb,
   .put_prev_task      = put_prev_task_wfb,

#ifdef CONFIG_SMP
   .select_task_rq     = select_task_rq_wfb,

   .pre_schedule       = pre_schedule_wfb,
   .post_schedule      = post_schedule_wfb,

   .task_waking            = task_waking_wfb,
   .task_woken     = task_woken_wfb,

   .set_cpus_allowed       = set_cpus_allowed_wfb,

   .rq_online              = rq_online_wfb,
   .rq_offline             = rq_offline_wfb,
#endif

   .set_curr_task          = set_curr_task_wfb,
   .task_tick      = task_tick_wfb,
   .task_fork              = task_fork_wfb,

   .switched_from          = switched_from_wfb,
   .switched_to        = switched_to_wfb,

   .prio_changed       = prio_changed_wfb,

   .get_rr_interval    = get_rr_interval_wfb,
#ifdef CONFIG_FAIR_GROUP_SCHED
   .moved_group            = NULL
#endif
};

#endif /* CONFIG_SCHED_WFB */