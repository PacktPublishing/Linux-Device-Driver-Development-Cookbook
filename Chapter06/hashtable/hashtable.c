/*
 * Kernel hash tables
 */

#define pr_fmt(fmt) "%s:%s: " fmt, KBUILD_MODNAME, __func__
#include <linux/module.h>
#include <linux/hashtable.h>

static DEFINE_HASHTABLE(data_hash, 1);

struct h_struct {
	int data;
	struct hlist_node node;
};

static int hash_func(int data)
{
	return data % 2;
}

/*
 * Local functions
 */

static void add_node(struct h_struct *new)
{
	int key = hash_func(new->data);

	hash_add(data_hash, &new->node, key);
}

static void del_node(int data)
{
	int key = hash_func(data);
	struct h_struct *entry;

	hash_for_each_possible(data_hash, entry, node, key) {
		if (entry->data == data) {
			hash_del(&entry->node);
			return;
		}
	}
}

static void print_nodes(void)
{
	int key;
	struct h_struct *entry;

	hash_for_each(data_hash, key, entry, node)
		pr_info("data=%d\n", entry->data);
}

/*
 * Init & exit stuff
 */

static int __init hashtable_init(void)
{
	struct h_struct e1 = {
		.data = 5
	};
	struct h_struct e2 = {
		.data = 2
	};
	struct h_struct e3 = {
		.data = 7
	};

	pr_info("add e1...\n");
	add_node(&e1);
	print_nodes();

	pr_info("add e2, e3...\n");
	add_node(&e2);
	add_node(&e3);
	print_nodes();

	pr_info("del data=5\n");
	del_node(5);
	print_nodes();

	return -EINVAL;
}

static void __exit hashtable_exit(void)
{
	pr_info("unloaded\n");
}

module_init(hashtable_init);
module_exit(hashtable_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rodolfo Giometti");
MODULE_DESCRIPTION("Kernel hash tables");
MODULE_VERSION("0.1");
