/*
 * Mutex
 */

#define pr_fmt(fmt) "%s:%s: " fmt, KBUILD_MODNAME, __func__
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/timer.h>
#include <linux/mutex.h>

/*
 * Module parameter and data
 */

static int delay_ms = 1000;
module_param(delay_ms, int, S_IRUSR | S_IWUSR);
MODULE_PARM_DESC(delay_ms, "kernel timer delay is ms");

static struct ktimer_data {
	struct mutex lock;
	struct timer_list timer;
	long delay_jiffies;
	int data;
} minfo;

/*
 * The kernel timer handler
 */

static void ktimer_handler(struct timer_list *t)
{
	struct ktimer_data *info = from_timer(info, t, timer);
	int ret;

	pr_info("kernel timer expired at %ld (data=%d)\n",
				jiffies, info->data);
	ret = mutex_trylock(&info->lock);
	if (ret) {
		info->data++;
		mutex_unlock(&info->lock);
	} else
		pr_err("cannot get the lock!\n");

	/* Reschedule kernel timer */
	mod_timer(&info->timer, jiffies + info->delay_jiffies);
}

/*
 * Probe/remove functions
 */

static int __init ktimer_init(void)
{
	/* Save kernel timer delay */
	minfo.delay_jiffies = msecs_to_jiffies(delay_ms);
	pr_info("delay is set to %dms (%ld jiffies)\n",
				delay_ms, minfo.delay_jiffies);

	/* Init the mutex */
	mutex_init(&minfo.lock);

	/* Setup and start the kernel timer */
	timer_setup(&minfo.timer, ktimer_handler, 0);
	mod_timer(&minfo.timer, jiffies);

	mutex_lock(&minfo.lock);
	minfo.data++;
	mutex_unlock(&minfo.lock);

	pr_info("mutex module loaded\n");
	return 0;
}

static void __exit ktimer_exit(void)
{
	del_timer_sync(&minfo.timer);

	pr_info("mutex module unloaded\n");
}

module_init(ktimer_init);
module_exit(ktimer_exit);

MODULE_AUTHOR("Rodolfo Giometti");
MODULE_DESCRIPTION("Mutex");
MODULE_LICENSE("GPL");
