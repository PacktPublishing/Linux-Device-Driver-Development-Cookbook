/*
 * Completion
 */

#define pr_fmt(fmt) "%s:%s: " fmt, KBUILD_MODNAME, __func__
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/timer.h>
#include <linux/completion.h>

/*
 * Module parameter and data
 */

static int delay_ms = 5000;
module_param(delay_ms, int, S_IRUSR | S_IWUSR);
MODULE_PARM_DESC(delay_ms, "kernel timer delay is ms");

static struct ktimer_data {
	struct completion done;
	struct timer_list timer;
	long delay_jiffies;
	unsigned int data;
} cinfo;

/*
 * The kernel timer handler
 */

static void ktimer_handler(struct timer_list *t)
{
	struct ktimer_data *info = from_timer(info, t, timer);

	pr_info("kernel timer expired at %ld (data=%d)\n",
				jiffies, info->data++);

	/* Signal that job is done */
	complete(&info->done);
}

/*
 * Probe/remove functions
 */

static int __init completion_init(void)
{
	/* Save kernel timer delay */
	cinfo.delay_jiffies = msecs_to_jiffies(delay_ms);
	pr_info("delay is set to %dms (%ld jiffies)\n",
				delay_ms, cinfo.delay_jiffies);

	/* Init the wait queue */
	init_completion(&cinfo.done);

	/* Setup and start the kernel timer */
	timer_setup(&cinfo.timer, ktimer_handler, 0);
	mod_timer(&cinfo.timer, jiffies + cinfo.delay_jiffies);

	/* Wait for completition... */
	wait_for_completion(&cinfo.done);

	pr_info("job done\n");

	return 0;
}

static void __exit completion_exit(void)
{
	del_timer_sync(&cinfo.timer);

	pr_info("module unloaded\n");
}

module_init(completion_init);
module_exit(completion_exit);

MODULE_AUTHOR("Rodolfo Giometti");
MODULE_DESCRIPTION("Completion");
MODULE_LICENSE("GPL");
