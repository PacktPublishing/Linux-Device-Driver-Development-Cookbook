/*
 * High resolution timer
 */

#define pr_fmt(fmt) "%s:%s: " fmt, KBUILD_MODNAME, __func__
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/hrtimer.h>

/*
 * Module parameter and data
 */

static int delay_ns = 1000000000;
module_param(delay_ns, int, S_IRUSR | S_IWUSR);
MODULE_PARM_DESC(delay_ns, "kernel timer delay is ns");

static struct hires_timer_data {
	struct hrtimer timer;
	unsigned int data;
} hires_tinfo;

/*
 * The kernel timer handler
 */

static enum hrtimer_restart hires_timer_handler(struct hrtimer *ptr)
{
	struct hires_timer_data *info = container_of(ptr,
					struct hires_timer_data, timer);

	pr_info("kernel timer expired at %ld (data=%d)\n",
				jiffies, info->data++);

	return HRTIMER_RESTART;
}

/*
 * Probe/remove functions
 */

static int __init hires_timer_init(void)
{
	/* Set up hires timer delay */

	pr_info("delay is set to %dns\n", delay_ns);

	/* Setup and start the hires timer */
	hires_tinfo.timer.function = hires_timer_handler;
	hrtimer_start(&hires_tinfo.timer, ns_to_ktime(delay_ns),
				HRTIMER_MODE_REL | HRTIMER_MODE_SOFT);

	pr_info("hires timer module loaded\n");
	return 0;
}

static void __exit hires_timer_exit(void)
{
	hrtimer_cancel(&hires_tinfo.timer);

	pr_info("hires timer module unloaded\n");
}

module_init(hires_timer_init);
module_exit(hires_timer_exit);

MODULE_AUTHOR("Rodolfo Giometti");
MODULE_DESCRIPTION("High resolution timer");
MODULE_LICENSE("GPL");
