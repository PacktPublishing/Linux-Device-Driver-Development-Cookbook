/*
 * chrdev
 */

#define pr_fmt(fmt) "%s:%s: " fmt, KBUILD_MODNAME, __func__
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/mman.h>


#include "chrdev.h"

/*
 * Local variables
 */

static dev_t chrdev_devt;
static struct class *chrdev_class;

struct chrdev_device chrdev_array[MAX_DEVICES];

/*
 * Methods
 */

static int chrdev_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct chrdev_device *chrdev = filp->private_data;
	size_t size = vma->vm_end - vma->vm_start;
	phys_addr_t offset = (phys_addr_t) vma->vm_pgoff << PAGE_SHIFT;
	unsigned long pfn;

	/* Does it even fit in phys_addr_t? */
	if (offset >> PAGE_SHIFT != vma->vm_pgoff)
		return -EINVAL;

	/* We cannot mmap too big areas */
	if ((offset > BUF_LEN) || (size > BUF_LEN - offset))
		return -EINVAL;

	/* Get the physical address belong the virtual kernel address */
	pfn = virt_to_phys(chrdev->buf) >> PAGE_SHIFT;

	dev_info(chrdev->dev, "mmap vma=%lx pfn=%lx size=%lx",
			vma->vm_start, pfn, size);

	/* Remap-pfn-range will mark the range VM_IO */
	if (remap_pfn_range(vma, vma->vm_start,
			    pfn, size,
			    vma->vm_page_prot))
		return -EAGAIN;

	return 0;
}

static long chrdev_ioctl(struct file *filp,
			unsigned int cmd, unsigned long arg)
{
	struct chrdev_device *chrdev = filp->private_data;
	struct chrdev_info info;
	void __user *uarg = (void __user *) arg;
	int __user *iuarg = (int __user *) arg;
	int ret;

	/* Get some command information */
	if (_IOC_TYPE(cmd) != CHRDEV_IOCTL_BASE) {
		dev_err(chrdev->dev, "command %x is not for us!\n", cmd);
		return -EINVAL;
	}
	dev_info(chrdev->dev, "cmd nr=%d size=%d dir=%x\n",
			_IOC_NR(cmd), _IOC_SIZE(cmd), _IOC_DIR(cmd));

	switch (cmd) {
	case CHRDEV_IOC_GETINFO:
		dev_info(chrdev->dev, "CHRDEV_IOC_GETINFO\n");

		strncpy(info.label, chrdev->label, NAME_LEN);
		info.read_only = chrdev->read_only;

		ret = copy_to_user(uarg, &info, sizeof(struct chrdev_info));
		if (ret)
			return -EFAULT;

		break;

	case WDIOC_SET_RDONLY:
		dev_info(chrdev->dev, "WDIOC_SET_RDONLY\n");

		ret = get_user(chrdev->read_only, iuarg);
		if (ret)
			return -EFAULT;

		break;

	default:
		return -ENOIOCTLCMD;
	}

	return 0;
}

static loff_t chrdev_llseek(struct file *filp, loff_t offset, int whence)
{
	struct chrdev_device *chrdev = filp->private_data;
	loff_t newppos;

	dev_info(chrdev->dev, "should move *ppos=%lld by whence %d off=%lld\n",
				filp->f_pos, whence, offset);

	switch (whence) {
	case SEEK_SET:
		newppos = offset;
		break;

	case SEEK_CUR:
		newppos = filp->f_pos + offset;
		break;

	case SEEK_END:
		newppos = BUF_LEN + offset;
		break;

	default:
		return -EINVAL;
	}

	if ((newppos < 0) || (newppos >= BUF_LEN))
		return -EINVAL;

	filp->f_pos = newppos;
	dev_info(chrdev->dev, "return *ppos=%lld\n", filp->f_pos);

	return newppos;
}

static ssize_t chrdev_read(struct file *filp,
			   char __user *buf, size_t count, loff_t *ppos)
{
	struct chrdev_device *chrdev = filp->private_data;
	int ret;

	dev_info(chrdev->dev, "should read %ld bytes (*ppos=%lld)\n",
				count, *ppos);

	/* Check for end-of-buffer */
	if (*ppos + count >= BUF_LEN)
		count = BUF_LEN - *ppos;

	/* Return data to the user space */
	ret = copy_to_user(buf, chrdev->buf + *ppos, count);
	if (ret < 0)
		return -EFAULT;

	*ppos += count;
	dev_info(chrdev->dev, "return %ld bytes (*ppos=%lld)\n", count, *ppos);

	return count;
}

static ssize_t chrdev_write(struct file *filp,
			    const char __user *buf, size_t count, loff_t *ppos)
{
	struct chrdev_device *chrdev = filp->private_data;
	int ret;

	dev_info(chrdev->dev, "should write %ld bytes (*ppos=%lld)\n",
				count, *ppos);

	if (chrdev->read_only)
		return -EINVAL;

	/* Check for end-of-buffer */
	if (*ppos + count >= BUF_LEN)
		count = BUF_LEN - *ppos;

	/* Get data from the user space */
	ret = copy_from_user(chrdev->buf + *ppos, buf, count);
	if (ret < 0)
		return -EFAULT;

	*ppos += count;
	dev_info(chrdev->dev, "got %ld bytes (*ppos=%lld)\n", count, *ppos);

	return count;
}

static int chrdev_open(struct inode *inode, struct file *filp)
{
	struct chrdev_device *chrdev = container_of(inode->i_cdev,
						struct chrdev_device, cdev);
	filp->private_data = chrdev;
	kobject_get(&chrdev->dev->kobj);

	dev_info(chrdev->dev, "chrdev (id=%d) opened\n", chrdev->id);

	return 0;
}

static int chrdev_release(struct inode *inode, struct file *filp)
{
	struct chrdev_device *chrdev = container_of(inode->i_cdev,
						struct chrdev_device, cdev);
	kobject_put(&chrdev->dev->kobj);
	filp->private_data = NULL;

	dev_info(chrdev->dev, "chrdev (id=%d) released\n", chrdev->id);

	return 0;
}

static const struct file_operations chrdev_fops = {
	.owner		= THIS_MODULE,
	.mmap		= chrdev_mmap,
	.unlocked_ioctl	= chrdev_ioctl,
	.llseek		= chrdev_llseek,
	.read		= chrdev_read,
	.write		= chrdev_write,
	.open		= chrdev_open,
	.release	= chrdev_release
};

/*
 * Exported functions
 */

int chrdev_device_register(const char *label, unsigned int id,
				unsigned int read_only,
				struct module *owner, struct device *parent)
{
	struct chrdev_device *chrdev;
	dev_t devt;
	int ret;

	/* First check if we are allocating a valid device... */
	if (id >= MAX_DEVICES) {
		pr_err("invalid id %d\n", id);
		return -EINVAL;
	}
	chrdev = &chrdev_array[id];

	/* ... then check if we have not busy id */
	if (chrdev->busy) {
		pr_err("id %d\n is busy", id);
		return -EBUSY;
	}

	/* First try to allocate memory for internal buffer */
	chrdev->buf = kzalloc(BUF_LEN, GFP_KERNEL);
	if (!chrdev->buf) {
		dev_err(chrdev->dev, "cannot allocate memory buffer!\n");
		return -ENOMEM;
	}

	/* Create the device and initialize its data */
	cdev_init(&chrdev->cdev, &chrdev_fops);
	chrdev->cdev.owner = owner;

	devt = MKDEV(MAJOR(chrdev_devt), id);
	ret = cdev_add(&chrdev->cdev, devt, 1);
	if (ret) {
		pr_err("failed to add char device %s at %d:%d\n",
				label, MAJOR(chrdev_devt), id);
		goto kfree_buf;
	}

	chrdev->dev = device_create(chrdev_class, parent, devt, chrdev,
				   "%s@%d", label, id);
	if (IS_ERR(chrdev->dev)) {
		pr_err("unable to create device %s\n", label);
		ret = PTR_ERR(chrdev->dev);
		goto del_cdev;
	}
	dev_set_drvdata(chrdev->dev, chrdev);

	/* Init the chrdev data */
	chrdev->id = id;
	chrdev->read_only = read_only;
	chrdev->busy = 1;
	strncpy(chrdev->label, label, NAME_LEN);

	dev_info(chrdev->dev, "chrdev %s with id %d added\n", label, id);

	return 0;

del_cdev:
	cdev_del(&chrdev->cdev);
kfree_buf:
	kfree(chrdev->buf);

	return ret;
}
EXPORT_SYMBOL(chrdev_device_register);

int chrdev_device_unregister(const char *label, unsigned int id)
{
	struct chrdev_device *chrdev;

	/* First check if we are deallocating a valid device... */
	if (id >= MAX_DEVICES) {
		pr_err("invalid id %d\n", id);
		return -EINVAL;
	}
	chrdev = &chrdev_array[id];

	/* ... then check if device is actualy allocated */
	if (!chrdev->busy || strcmp(chrdev->label, label)) {
		pr_err("id %d is not busy or label %s is not known\n",
						id, label);
		return -EINVAL;
	}

	/* Deinit the chrdev data */
	chrdev->id = 0;
	chrdev->busy = 0;

	dev_info(chrdev->dev, "chrdev %s with id %d removed\n", label, id);

	/* Free allocated memory */
	kfree(chrdev->buf);

	/* Dealocate the device */
	device_destroy(chrdev_class, chrdev->dev->devt);
	cdev_del(&chrdev->cdev);

	return 0;
}
EXPORT_SYMBOL(chrdev_device_unregister);

/*
 * Module stuff
 */

static int __init chrdev_init(void)
{
	int ret;

	/* Create the new class for the chrdev devices */
	chrdev_class = class_create(THIS_MODULE, "chrdev");
	if (!chrdev_class) {
		pr_err("chrdev: failed to allocate class\n");
		return -ENOMEM;
	}

	/* Allocate a region for character devices */
	ret = alloc_chrdev_region(&chrdev_devt, 0, MAX_DEVICES, "chrdev");
	if (ret < 0) {
		pr_err("failed to allocate char device region\n");
		goto remove_class;
	}

	pr_info("got major %d\n", MAJOR(chrdev_devt));

	return 0;

remove_class:
	class_destroy(chrdev_class);

	return ret;
}

static void __exit chrdev_exit(void)
{
	unregister_chrdev_region(chrdev_devt, MAX_DEVICES);
	class_destroy(chrdev_class);
}

module_init(chrdev_init);
module_exit(chrdev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rodolfo Giometti");
MODULE_DESCRIPTION("chardev");
