/*
 * Module with parameters
 */

#define pr_fmt(fmt) "%s:%s: " fmt, KBUILD_MODNAME, __func__
#include <linux/moduleparam.h>
#include <linux/module.h>

static int var = 0x3f;
module_param(var, int, S_IRUSR | S_IWUSR);
MODULE_PARM_DESC(var, "an integer value");

static char *str = "default string";
module_param(str, charp, S_IRUSR | S_IWUSR);
MODULE_PARM_DESC(str, "a string value");

#define ARR_SIZE		8
static int arr[ARR_SIZE];
static int arr_count;
module_param_array(arr, int, &arr_count,  S_IRUSR | S_IWUSR);
MODULE_PARM_DESC(arr, "an array of " __stringify(ARR_SIZE) " values");

/*
 * Init & exit stuff
 */

static int __init module_par_init(void)
{
	int i;

	pr_info("loaded\n");
	pr_info("var = 0x%02x\n", var);
	pr_info("str = \"%s\"\n", str);
	pr_info("arr = ");
	for (i = 0; i < ARR_SIZE; i++)
		pr_cont("%d ", arr[i]);
	pr_cont("\n");

	return 0;
}

static void __exit module_par_exit(void)
{
	pr_info("unloaded\n");
}

module_init(module_par_init);
module_exit(module_par_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rodolfo Giometti");
MODULE_DESCRIPTION("Module with parameters");
MODULE_VERSION("0.1");
