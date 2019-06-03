/*
 * chrdev fasync() testing program
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#define __USE_GNU
#include <signal.h>

int fd;

void sigio_handler(int unused) {
	char c;
	int ret;

	ret = read(fd, &c, 1);
	if (ret < 0) {
		perror("read");
		exit(EXIT_FAILURE);
	}
	ret = write(STDOUT_FILENO, &c, 1);
	if (ret < 0) {
		perror("write");
		exit(EXIT_FAILURE);
	}
}

int main(int argc, char *argv[])
{
	sighandler_t sigh;
	long flags;
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

	/* Try to install the signal handler and the fasync stuff */
	sigh = signal(SIGIO, sigio_handler);
	if (sigh == SIG_ERR) {
		perror("signal");
		exit(EXIT_FAILURE);
	}
	ret = fcntl(fd, F_SETOWN, getpid());
	if (ret < 0) {
		perror("fcntl(..., F_SETOWN, ...)");
		exit(EXIT_FAILURE);
	}
	flags = fcntl(fd, F_GETFL);
	if (flags < 0) {
		perror("fcntl(..., F_GETFL)");
		exit(EXIT_FAILURE);
	}
	ret = fcntl(fd, F_SETFL, flags | FASYNC);
	if (flags < 0) {
		perror("fcntl(..., F_SETFL, ...)");
		exit(EXIT_FAILURE);
	}
	
	/* Now we can wait for data while waiting data from stdin */
	while (1) {
		ret = read(STDOUT_FILENO, &c, 1);
		if (ret < 0) {
			perror("read");
			exit(EXIT_FAILURE);
		}
		printf("got '%c' from stdin!\n", c);
	}

	signal(SIGIO, NULL);
	close(fd);

	return 0;
}
