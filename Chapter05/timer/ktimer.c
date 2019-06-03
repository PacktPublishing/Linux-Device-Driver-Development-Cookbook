/*
 * Kernel timer
 */

#define pr_fmt(fmt) "%s:%s: " fmt, KBUILD_MODNAME, __func__
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/timer.h>

/*
 * Module parameter and data
 */

static int delay_ms = 1000;
module_param(delay_ms, int, S_IRUSR | S_IWUSR);
MODULE_PARM_DESC(delay_ms, "kernel timer delay is ms");

static struct ktimer_data {
	struct timer_list timer;
	long delay_jiffies;
	unsigned int data;
} ktinfo;

/*
 * The kernel timer handler
 */

static void ktimer_handler(struct timer_list *t)
{
	struct ktimer_data *info = from_timer(info, t, timer);

	pr_info("kernel timer expired at %ld (data=%d)\n",
				jiffies, info->data++);

	/* Reschedule kernel timer */
	mod_timer(&info->timer, jiffies + info->delay_jiffies);
}

/*
 * Probe/remove functions
 */

static int __init ktimer_init(void)
{
	/* Save kernel timer delay */
	ktinfo.delay_jiffies = msecs_to_jiffies(delay_ms);
	pr_info("delay is set to %dms (%ld jiffies)\n",
				delay_ms, ktinfo.delay_jiffies);

	/* Setup and start the kernel timer */
	timer_setup(&ktinfo.timer, ktimer_handler, 0);
	mod_timer(&ktinfo.timer, jiffies + ktinfo.delay_jiffies);

	pr_info("kernel timer module loaded\n");
	return 0;
}

static void __exit ktimer_exit(void)
{
	del_timer_sync(&ktinfo.timer);

	pr_info("kernel timer module unloaded\n");
}

module_init(ktimer_init);
module_exit(ktimer_exit);

MODULE_AUTHOR("Rodolfo Giometti");
MODULE_DESCRIPTION("Kernel timer");
MODULE_LICENSE("GPL");
