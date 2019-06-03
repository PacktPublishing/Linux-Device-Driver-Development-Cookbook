/*
 * Chrdev include file
 */

#include <linux/cdev.h>
#include "chrdev_ioctl.h"

#define MAX_DEVICES	8
#define NAME_LEN	CHRDEV_NAME_LEN
#define BUF_LEN		PAGE_SIZE

/*
 * Chrdev basic structs
 */

/* Main struct */
struct chrdev_device {
	char label[NAME_LEN];
	unsigned int busy : 1;
	char *buf;
	int read_only;

	unsigned int id;
	struct module *owner;
	struct cdev cdev;
	struct device *dev;
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
