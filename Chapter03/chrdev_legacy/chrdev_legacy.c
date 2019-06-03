/*
 * chrdev legacy
 */

#define pr_fmt(fmt) "%s:%s: " fmt, KBUILD_MODNAME, __func__
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>

/* Device major umber */
static int major;

/*
 * Methods
 */

static ssize_t chrdev_read(struct file *filp,
			   char __user *buf, size_t count, loff_t *ppos)
{
	pr_info("return EOF\n");

	return 0;
}

static ssize_t chrdev_write(struct file *filp,
			    const char __user *buf, size_t count, loff_t *ppos)
{
	pr_info("got %ld bytes\n", count);

	*ppos += count;	
	return count;
}

static int chrdev_open(struct inode *inode, struct file *filp)
{
	pr_info("chrdev opened\n");

	return 0;
}

static int chrdev_release(struct inode *inode, struct file *filp)
{
	pr_info("chrdev released\n");

	return 0;
}

static struct file_operations chrdev_fops = {
	.owner		= THIS_MODULE,
	.read		= chrdev_read,
	.write		= chrdev_write,
	.open		= chrdev_open,
	.release	= chrdev_release
};

/*
 * Module stuff
 */

static int __init chrdev_init(void)
{
	int ret;

	ret = register_chrdev(0, "chrdev", &chrdev_fops);
	if (ret < 0) {
		pr_err("unable to register char device! Error %d\n", ret);
		return ret;
	}
	major = ret;
	pr_info("got major %d\n", major);

	return 0;
}

static void __exit chrdev_exit(void)
{
	unregister_chrdev(major, "chrdev");
}

module_init(chrdev_init);
module_exit(chrdev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rodolfo Giometti");
MODULE_DESCRIPTION("chrdev legacy");
