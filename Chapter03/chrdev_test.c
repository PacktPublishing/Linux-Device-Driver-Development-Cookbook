/*
 * chrdev testing program
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void dump(char *str, char *buf, int size)
{
	int i;

	if (size <= 0)
		return;

	printf("%s", str);
	for (i = 0; i < size; i++)
		printf("%02x ", buf[i]);
	printf("\n");
}

int main(int argc, char *argv[])
{
	int fd;
	char buf[] = "DUMMY DATA";
	int n, c;
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

	for (c = 0; c < sizeof(buf); c += n) {
		ret = write(fd, buf + c, sizeof(buf) - c);
		if (ret < 0) {
			perror("write");
			exit(EXIT_FAILURE);
		}
		n = ret;

		printf("wrote %d bytes into file %s\n", n, argv[1]);
		dump("data written are: ", buf + c, n);
	}

	for (c = 0; c < sizeof(buf); c += n) {
		ret = read(fd, buf, sizeof(buf));
		if (ret == 0) {
			printf("read EOF\n");
			break;
		} else if (ret < 0) {
			perror("read");
			exit(EXIT_FAILURE);
		}
		n = ret;

		printf("read %d bytes from file %s\n", n, argv[1]);
		dump("data read are: ", buf, n);
	}

	close(fd);

	return 0;
}
