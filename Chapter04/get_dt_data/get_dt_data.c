/*
 * Module to inspect device tree from the kernel
 */

#define pr_fmt(fmt) "%s: " fmt, KBUILD_MODNAME
#include <linux/moduleparam.h>
#include <linux/module.h>
#include <linux/of.h>

#define PATH_DEFAULT	"/"
static char *path = PATH_DEFAULT;
module_param(path, charp, S_IRUSR | S_IWUSR);
MODULE_PARM_DESC(path, "a device tree pathname " \
			"(default is \"" PATH_DEFAULT "\")");

/*
 * Local functions
 */

static void print_property_u32(struct device_node *node, const char *name)
{
	u32 val32;
	if (of_property_read_u32(node, name, &val32) == 0)
		pr_info(" \%s = %d\n", name, val32);
}

static void print_property_string(struct device_node *node, const char *name)
{
	const char *str;
	if (of_property_read_string(node, name, &str) == 0)
		pr_info(" \%s = %s\n", name, str);
}

static void print_main_prop(struct device_node *node)
{
	pr_info("+ node = %s\n", node->full_name);
	print_property_u32(node, "#address-cells");
	print_property_u32(node, "#size-cells");
	print_property_u32(node, "reg");
	print_property_string(node, "name");
	print_property_string(node, "compatible");
	print_property_string(node, "status");
}

/*
 * Init & exit stuff
 */

static int __init get_dt_data_init(void)
{
	struct device_node *node, *child;
	struct property *prop;

	pr_info("path = \"%s\"\n", path);

	/* Find node by its pathname */
	node = of_find_node_by_path(path);
	if (!node) {
		pr_err("failed to find device-tree node \"%s\"\n", path);
		return -ENODEV;
	}
	pr_info("device-tree node found!\n");

	pr_info("now getting main properties...\n");
	print_main_prop(node);

	pr_info("now move through all properties...\n");
	for_each_property_of_node(node, prop)
		pr_info("-> %s\n", prop->name);

	/* Move through node's children... */
	pr_info("Now move through children...\n");
	for_each_child_of_node(node, child)
		print_main_prop(child);

	/* Force module unloading... */
	return -EINVAL;
}

static void __exit get_dt_data_exit(void)
{
	/* nop */
}

module_init(get_dt_data_init);
module_exit(get_dt_data_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rodolfo Giometti");
MODULE_DESCRIPTION("Module to inspect device tree from the kernel");
MODULE_VERSION("0.1");
