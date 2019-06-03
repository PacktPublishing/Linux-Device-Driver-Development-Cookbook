/*
 * chrdev ioctl() testing program
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "chrdev_ioctl.h"

int main(int argc, char *argv[])
{
	int fd;
	struct chrdev_info info;
	int read_only;
	int ret;

	if (argc < 2) {
		fprintf(stderr, "usage: %s <dev>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	ret = open(argv[1], O_RDWR);
	if (ret < 0) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	printf("file %s opened\n", argv[1]);
	fd = ret;

	/* Try reading device info */
	ret = ioctl(fd, CHRDEV_IOC_GETINFO, &info);
        if (ret < 0) {
                perror("ioctl(CHRDEV_IOC_GETINFO)");
                exit(EXIT_FAILURE);
        }
	printf("got label=%s and read_only=%d\n", info.label, info.read_only);

	/* Try toggling the device reading mode */
	read_only = !info.read_only;
	ret = ioctl(fd, WDIOC_SET_RDONLY, &read_only);
        if (ret < 0) {
                perror("ioctl(WDIOC_SET_RDONLY)");
                exit(EXIT_FAILURE);
        }
	printf("device has now read_only=%d\n", read_only);

	close(fd);

	return 0;
}
