/*
 * Kernel lists
 */

#define pr_fmt(fmt) "%s:%s: " fmt, KBUILD_MODNAME, __func__
#include <linux/module.h>
#include <linux/list.h>

static LIST_HEAD(data_list);

struct l_struct {
	int data;
	struct list_head list;
};

/*
 * Local functions
 */

static void add_ordered_entry(struct l_struct *new)
{
	struct list_head *ptr;
	struct l_struct *entry;

	list_for_each(ptr, &data_list) {
		entry = list_entry(ptr, struct l_struct, list);
		if (entry->data < new->data) {
			list_add_tail(&new->list, ptr);
			return;
		}
	}
	list_add_tail(&new->list, &data_list);
}

static void del_entry(int data)
{
	struct list_head *ptr;
	struct l_struct *entry;

	list_for_each(ptr, &data_list) {
		entry = list_entry(ptr, struct l_struct, list);
		if (entry->data == data) {
			list_del(ptr);
			return;
		}
	}
}

static void print_entries(void)
{
	struct l_struct *entry;

	list_for_each_entry(entry, &data_list, list)
		pr_info("data=%d\n", entry->data);
}

/*
 * Init & exit stuff
 */

static int __init list_init(void)
{
	struct l_struct e1 = {
		.data = 5
	};
	struct l_struct e2 = {
		.data = 1
	};
	struct l_struct e3 = {
		.data = 7
	};

	pr_info("add e1...\n");
	add_ordered_entry(&e1);
	print_entries();

	pr_info("add e2, e3...\n");
	add_ordered_entry(&e2);
	add_ordered_entry(&e3);
	print_entries();

	pr_info("del data=5...\n");
	del_entry(5);
	print_entries();

	return -EINVAL;
}

static void __exit list_exit(void)
{
	pr_info("unloaded\n");
}

module_init(list_init);
module_exit(list_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rodolfo Giometti");
MODULE_DESCRIPTION("Kernel lists");
MODULE_VERSION("0.1");
