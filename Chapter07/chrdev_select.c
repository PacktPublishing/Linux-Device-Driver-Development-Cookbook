/*
 * chrdev select() testing program
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>

int main(int argc, char *argv[])
{
	int fd;
	fd_set read_fds;
	char c;
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

	while (1) {
		/* Set up reading file descriptors */
		FD_ZERO(&read_fds);
		FD_SET(STDIN_FILENO, &read_fds);
		FD_SET(fd, &read_fds);

		/* Wait for any data fro our device or stdin */
		ret = select(FD_SETSIZE, &read_fds, NULL, NULL, NULL);
		if (ret < 0) {
			perror("select");
			exit(EXIT_FAILURE);
		}

		if (FD_ISSET(STDIN_FILENO, &read_fds)) {
			ret = read(STDIN_FILENO, &c, 1);
			if (ret < 0) {
				perror("read(STDIN, ...)");
				exit(EXIT_FAILURE);
			}
			printf("got '%c' from stdin!\n", c);
		}
		if (FD_ISSET(fd, &read_fds)) {
			ret = read(fd, &c, 1);
			if (ret < 0) {
				perror("read(fd, ...)");
				exit(EXIT_FAILURE);
			}
			printf("got '%c' from device!\n", c);
		}
	}

	close(fd);

	return 0;
}
