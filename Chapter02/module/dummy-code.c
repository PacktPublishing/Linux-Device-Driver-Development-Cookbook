/*
 * Dummy code
 */

#define pr_fmt(fmt) "%s:%s: " fmt, KBUILD_MODNAME, __func__
#include <linux/module.h>

static int __init dummy_code_init(void)
{
	pr_info("dummy-code loaded\n");
	return 0;
}

static void __exit dummy_code_exit(void)
{
	pr_info("dummy-code unloaded\n");
}

module_init(dummy_code_init);
module_exit(dummy_code_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rodolfo Giometti");
MODULE_DESCRIPTION("Dummy code");
