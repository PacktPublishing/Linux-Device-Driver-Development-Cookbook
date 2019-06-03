/*
 * Kernel memory allocation
 */

#define pr_fmt(fmt) "%s:%s: " fmt, KBUILD_MODNAME, __func__
#include <linux/moduleparam.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>

static long size = 4;
module_param(size, long, S_IRUSR | S_IWUSR);
MODULE_PARM_DESC(size, "memory size in Kbytes");

/*
 * Init & exit stuff
 */

static int __init mem_alloc_init(void)
{
	void *ptr;

	pr_info("size=%ldkbytes\n", size);

	ptr = kmalloc(size << 10, GFP_KERNEL);
	pr_info("kmalloc(..., GFP_KERNEL) =%px\n", ptr);
	kfree(ptr);

	ptr = kmalloc(size << 10, GFP_ATOMIC);
	pr_info("kmalloc(..., GFP_ATOMIC) =%px\n", ptr);
	kfree(ptr);

	ptr = vmalloc(size << 10);
	pr_info("vmalloc(...)             =%px\n", ptr);
	vfree(ptr);

	ptr = kvmalloc(size << 10, GFP_KERNEL);
	pr_info("kvmalloc(..., GFP_KERNEL)=%px\n", ptr);
	kvfree(ptr);

	ptr = kvmalloc(size << 10, GFP_ATOMIC);
	pr_info("kvmalloc(..., GFP_ATOMIC)=%px\n", ptr);
	kvfree(ptr);

	return -EINVAL;
}

static void __exit mem_alloc_exit(void)
{
	pr_info("unloaded\n");
}

module_init(mem_alloc_init);
module_exit(mem_alloc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rodolfo Giometti");
MODULE_DESCRIPTION("Kernel memory allocation");
MODULE_VERSION("0.1");
