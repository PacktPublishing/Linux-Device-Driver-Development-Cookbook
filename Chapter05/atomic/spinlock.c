/*
 * Mutex
 */

#define pr_fmt(fmt) "%s:%s: " fmt, KBUILD_MODNAME, __func__
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/timer.h>
#include <linux/spinlock.h>

/*
 * Module parameter and data
 */

static int delay_ms = 1000;
module_param(delay_ms, int, S_IRUSR | S_IWUSR);
MODULE_PARM_DESC(delay_ms, "kernel timer delay is ms");

static struct ktimer_data {
	struct spinlock lock;
	struct timer_list timer;
	long delay_jiffies;
	int data;
} sinfo;

/*
 * The kernel timer handler
 */

static void ktimer_handler(struct timer_list *t)
{
	struct ktimer_data *info = from_timer(info, t, timer);

	pr_info("kernel timer expired at %ld (data=%d)\n",
				jiffies, info->data);
	spin_lock(&sinfo.lock);
		info->data++;
	spin_unlock(&info->lock);

	/* Reschedule kernel timer */
	mod_timer(&info->timer, jiffies + info->delay_jiffies);
}

/*
 * Probe/remove functions
 */

static int __init ktimer_init(void)
{
	/* Save kernel timer delay */
	sinfo.delay_jiffies = msecs_to_jiffies(delay_ms);
	pr_info("delay is set to %dms (%ld jiffies)\n",
				delay_ms, sinfo.delay_jiffies);

	/* Init the spinlock */
	spin_lock_init(&sinfo.lock);

	/* Setup and start the kernel timer */
	timer_setup(&sinfo.timer, ktimer_handler, 0);
	mod_timer(&sinfo.timer, jiffies);

	spin_lock(&sinfo.lock);
	sinfo.data++;
	spin_unlock(&sinfo.lock);

	pr_info("spinlock module loaded\n");
	return 0;
}

static void __exit ktimer_exit(void)
{
	del_timer_sync(&sinfo.timer);

	pr_info("spinlock module unloaded\n");
}

module_init(ktimer_init);
module_exit(ktimer_exit);

MODULE_AUTHOR("Rodolfo Giometti");
MODULE_DESCRIPTION("Mutex");
MODULE_LICENSE("GPL");
