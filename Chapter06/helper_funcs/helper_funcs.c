/*
 * Kernel helper functions
 */

#define pr_fmt(fmt) "%s:%s: " fmt, KBUILD_MODNAME, __func__
#include <linux/moduleparam.h>
#include <linux/module.h>
#include <linux/types.h>

static char *str = "default string";
module_param(str, charp, S_IRUSR | S_IWUSR);
MODULE_PARM_DESC(str, "a string value");

#define STR2_LEN	32

/*
 * Init & exit stuff
 */

static int __init helper_funcs_init(void)
{
	char str2[STR2_LEN];

	pr_info("str=\"%s\"\n", str);
	pr_info("str size=%ld\n", strlen(str));

	strncpy(str2, str, STR2_LEN);

	pr_info("str2=\"%s\"\n", str2);
	pr_info("str2 size=%ld\n", strlen(str2));

	return -EINVAL;
}

static void __exit helper_funcs_exit(void)
{
	pr_info("unloaded\n");
}

module_init(helper_funcs_init);
module_exit(helper_funcs_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rodolfo Giometti");
MODULE_DESCRIPTION("Kernel helper functions");
MODULE_VERSION("0.1");
