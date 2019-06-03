/*
 * Chrdev include file
 */

#include <linux/cdev.h>
#include <linux/circ_buf.h>
#include <linux/mutex.h>
#include <linux/hrtimer.h>
#include <linux/spinlock.h>

#define MAX_DEVICES	8
#define NAME_LEN	32
#define BUF_LEN		PAGE_SIZE

/*
 * Chrdev basic structs
 */

/* Main struct */
struct chrdev_device {
	char label[NAME_LEN];
	unsigned int busy : 1;
	char *buf;
	size_t head, tail;
	int read_only;

	unsigned int id;
	struct module *owner;
	struct cdev cdev;
	struct device *dev;

	struct mutex mux;
	struct spinlock lock;
	struct wait_queue_head queue;
	struct hrtimer timer;
	struct fasync_struct *fasync_queue;
};

/*
 * Exported functions
 */

#define to_class_dev(obj) container_of((obj), struct class_device, kobj)
#define to_chrdev_device(obj) container_of((obj), struct chrdev_device, class)

extern int chrdev_device_register(const char *label, unsigned int id,
				unsigned int read_only,
				struct module *owner, struct device *parent);
extern int chrdev_device_unregister(const char *label, unsigned int id);
