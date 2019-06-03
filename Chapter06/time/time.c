/*
 * Kernel timing functions
 */

#define pr_fmt(fmt) "%s:%s: " fmt, KBUILD_MODNAME, __func__
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/timekeeping.h>

#define print_time(str, code)			\
	do {					\
		u64 t0, t1;			\
		t0 = ktime_get_real_ns();	\
		code;				\
		t1 = ktime_get_real_ns();	\
		pr_info(str " -> %lluns\n", t1 - t0); \
	} while (0)

/*
 * Init & exit stuff
 */

static int __init time_init(void)
{
	pr_info("*delay() functions:\n");
	print_time("10ns", ndelay(10));
	print_time("10000ns", udelay(10));
	print_time("10000000ns", mdelay(10));

	pr_info("*sleep() functions:\n");
	print_time("10000ns", usleep_range(10, 10));
	print_time("10000000ns", msleep(10));
	print_time("10000000ns", msleep_interruptible(10));
	print_time("10000000000ns", ssleep(10));

	return -EINVAL;
}

static void __exit time_exit(void)
{
	pr_info("unloaded\n");
}

module_init(time_init);
module_exit(time_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rodolfo Giometti");
MODULE_DESCRIPTION("Kernel timing functions");
MODULE_VERSION("0.1");
