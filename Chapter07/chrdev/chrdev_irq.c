/*
 * chrdev
 */

#define pr_fmt(fmt) "%s:%s: " fmt, KBUILD_MODNAME, __func__
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/hrtimer.h>
#include <linux/poll.h>

#include "chrdev_irq.h"

/*
 * Module parameter
 */

static int delay_ns = 1000000000;
module_param(delay_ns, int, S_IRUSR | S_IWUSR);
MODULE_PARM_DESC(delay_ns, "kernel timer delay is ns");

/*
 * Local variables
 */

static dev_t chrdev_devt;
static struct class *chrdev_class;

struct chrdev_device chrdev_array[MAX_DEVICES];

/*
 * Dummy function to generate data
 */

static char get_new_char(void)
{
	static char d = 'A' - 1;

	if (++d == ('Z' + 1))
		d = 'A';

	return d;
}

/*
 * Circular buffer management functions
 */

static inline bool cbuf_is_empty(size_t head, size_t tail, size_t len)
{
	return head == tail;
}

static inline bool cbuf_is_full(size_t head, size_t tail, size_t len)
{
	head = (head + 1) % len;
	return head == tail;
}

static inline size_t cbuf_count_to_end(size_t head, size_t tail, size_t len)
{
	if (head >= tail)
		return head - tail;
	else
		return len - tail + head;
}

static inline size_t cbuf_space_to_end(size_t head, size_t tail, size_t len)
{
	if (head >= tail)
		return len - head + tail - 1;
	else
		return tail - head - 1;
}

static inline void cbuf_pointer_move(size_t *ptr, size_t n, size_t len)
{
	*ptr = (*ptr + n) % len;
}

/*
 * (simulation of) IRQ handler
 */

static enum hrtimer_restart chrdev_timer_handler(struct hrtimer *ptr)
{
	struct chrdev_device *chrdev = container_of(ptr,
					struct chrdev_device, timer);

	/* Grab the lock */
	spin_lock(&chrdev->lock);

	/* Now we should check if we have some space to
	 * save incoming data, otherwise they must be dropped...
	 */
	if (!cbuf_is_full(chrdev->head, chrdev->tail, BUF_LEN)) {
		chrdev->buf[chrdev->head] = get_new_char();

		cbuf_pointer_move(&chrdev->head, 1, BUF_LEN);
	}

	/* Release the lock */
	spin_unlock(&chrdev->lock);

	/* Wake up any possible sleeping process */
	wake_up_interruptible(&chrdev->queue);
	kill_fasync(&chrdev->fasync_queue, SIGIO, POLL_IN);

	/* Now forward the expiration time and ask to be rescheduled */
	hrtimer_forward_now(&chrdev->timer, ns_to_ktime(delay_ns));
	return HRTIMER_RESTART;
}

/*
 * Methods
 */

static int chrdev_fasync(int fd, struct file *filp, int on)
{
	struct chrdev_device *chrdev = filp->private_data;

        return fasync_helper(fd, filp, on, &chrdev->fasync_queue);
}

static __poll_t chrdev_poll(struct file *filp, poll_table *wait)
{
	struct chrdev_device *chrdev = filp->private_data;
	__poll_t mask = 0;

	poll_wait(filp, &chrdev->queue, wait);

	/* Grab the mutex */
	mutex_lock(&chrdev->mux);

	if (!cbuf_is_empty(chrdev->head, chrdev->tail, BUF_LEN))
		mask |= EPOLLIN | EPOLLRDNORM;

	/* Release the mutex */
	mutex_unlock(&chrdev->mux);

	return mask;
}

static ssize_t chrdev_read(struct file *filp,
               char __user *buf, size_t count, loff_t *ppos)
{
	struct chrdev_device *chrdev = filp->private_data;
	unsigned long flags;
	char tmp[256];
	size_t n;
	int ret;

	dev_info(chrdev->dev, "should read %ld bytes\n", count);

	/* Grab the mutex */
	mutex_lock(&chrdev->mux);

	/* Check for some data into read buffer */
	if (filp->f_flags & O_NONBLOCK) {
		if (cbuf_is_empty(chrdev->head, chrdev->tail, BUF_LEN)) {
			ret = -EAGAIN;
			goto unlock;
		}
	} else if (wait_event_interruptible(chrdev->queue,
		!cbuf_is_empty(chrdev->head, chrdev->tail, BUF_LEN))) {
		count = -ERESTARTSYS;
		goto unlock;
	}

	/* Grab the lock */
	spin_lock_irqsave(&chrdev->lock, flags);

	/* Get data from the circular buffer */
	n = cbuf_count_to_end(chrdev->head, chrdev->tail, BUF_LEN);
	count = min(count, n);
	memcpy(tmp, &chrdev->buf[chrdev->tail], count);

	/* Release the lock */
	spin_unlock_irqrestore(&chrdev->lock, flags);

	/* Return data to the user space */
	ret = copy_to_user(buf, tmp, count);
	if (ret < 0) {
		ret = -EFAULT;
		goto unlock;
	}

	/* Now we can safely move the tail pointer */
	cbuf_pointer_move(&chrdev->tail, count, BUF_LEN);
	dev_info(chrdev->dev, "return %ld bytes\n", count);

unlock:
	/* Release the mutex */
	mutex_unlock(&chrdev->mux);

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
	.fasync		= chrdev_fasync,
	.poll		= chrdev_poll,
	.llseek		= no_llseek,
	.read		= chrdev_read,
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
	mutex_init(&chrdev->mux);
	spin_lock_init(&chrdev->lock);
	init_waitqueue_head(&chrdev->queue);
	chrdev->head = chrdev->tail = 0;
	chrdev->fasync_queue = NULL;

	/* Setup and start the hires timer */
	hrtimer_init(&chrdev->timer, CLOCK_MONOTONIC,
				HRTIMER_MODE_REL | HRTIMER_MODE_SOFT);
	chrdev->timer.function = chrdev_timer_handler;
	hrtimer_start(&chrdev->timer, ns_to_ktime(delay_ns),
				HRTIMER_MODE_REL | HRTIMER_MODE_SOFT);


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

	/* Stop the timer */
	hrtimer_cancel(&chrdev->timer);

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
