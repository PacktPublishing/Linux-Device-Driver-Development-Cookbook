/*
 * Chrdev ioctl() include file
 */

#include <linux/ioctl.h>
#include <linux/types.h>

#define CHRDEV_IOCTL_BASE	'C'
#define CHRDEV_NAME_LEN		32

struct chrdev_info {
	char label[CHRDEV_NAME_LEN];
	int read_only;
};

/*
 * The ioctl() commands
 */

#define CHRDEV_IOC_GETINFO	_IOR(CHRDEV_IOCTL_BASE, 0, struct chrdev_info)
#define WDIOC_SET_RDONLY	_IOW(CHRDEV_IOCTL_BASE, 1, int)
