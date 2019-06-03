/*
 * Wait Queue
 */

#define pr_fmt(fmt) "%s:%s: " fmt, KBUILD_MODNAME, __func__
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/timer.h>
#include <linux/wait.h>

/*
 * Module parameter and data
 */

static int delay_ms = 1000;
module_param(delay_ms, int, S_IRUSR | S_IWUSR);
MODULE_PARM_DESC(delay_ms, "kernel timer delay is ms");

static struct ktimer_data {
	struct wait_queue_head waitq;
	struct timer_list timer;
	long delay_jiffies;
	unsigned int data;
} wqinfo;

/*
 * The kernel timer handler
 */

static void ktimer_handler(struct timer_list *t)
{
	struct ktimer_data *info = from_timer(info, t, timer);

	pr_info("kernel timer expired at %ld (data=%d)\n",
				jiffies, info->data++);

	/* Wake up all sleeping processes */
	wake_up_interruptible(&info->waitq);

	/* Reschedule kernel timer */
	mod_timer(&info->timer, jiffies + info->delay_jiffies);
}

/*
 * Probe/remove functions
 */

static int __init waitqueue_init(void)
{
	int ret;

	/* Save kernel timer delay */
	wqinfo.delay_jiffies = msecs_to_jiffies(delay_ms);
	pr_info("delay is set to %dms (%ld jiffies)\n",
				delay_ms, wqinfo.delay_jiffies);

	/* Init the wait queue */
	init_waitqueue_head(&wqinfo.waitq);

	/* Setup and start the kernel timer */
	timer_setup(&wqinfo.timer, ktimer_handler, 0);
	mod_timer(&wqinfo.timer, jiffies + wqinfo.delay_jiffies);

	/* Wait for the wake up event... */
	ret = wait_event_interruptible(wqinfo.waitq, wqinfo.data > 5);
	if (ret < 0)
		goto exit;

	pr_info("got event data > 5\n");

	return 0;

exit:
	if (ret == -ERESTARTSYS)
		pr_info("interrupted by signal!\n");
	else
		pr_err("unable to wait for event\n");

	del_timer_sync(&wqinfo.timer);

	return ret;
}

static void __exit waitqueue_exit(void)
{
	del_timer_sync(&wqinfo.timer);

	pr_info("module unloaded\n");
}

module_init(waitqueue_init);
module_exit(waitqueue_exit);

MODULE_AUTHOR("Rodolfo Giometti");
MODULE_DESCRIPTION("Wait queue");
MODULE_LICENSE("GPL");
