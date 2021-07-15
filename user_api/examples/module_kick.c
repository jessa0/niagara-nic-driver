/* 20131012
 * This is example program.
 * it generates periodic "kick" signals for Niagara card, which are
 * keeping card in ACTIVE (non-BYPASS) state
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <niagara_api.h>

// feel free to change it dynamically in /sys/modules/kick/parameters
static int card, segment;
module_param(card, int, S_IRUGO | S_IWUSR);
module_param(segment, int, S_IRUGO | S_IWUSR);

static int SystemIsGood(void)
{
// put your stuff here
	return 1;
}

static struct task_struct *thread;
static int thread_fn(void *arg)
{
	for (; !kthread_should_stop(); ) {
		NiagaraSetAttribute(card, segment, "kick", SystemIsGood());
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(HZ);
	}
	return 0;
}

static int __init kick_init(void)
{
	thread = kthread_run(thread_fn, NULL, "niagara_kick");
	if (thread == NULL) return -ENOMEM;
	return 0;
}
module_init(kick_init);

static void __exit kick_destroy(void)
{
	if (thread) kthread_stop(thread);
}
module_exit(kick_destroy);

MODULE_LICENSE("GPL");
