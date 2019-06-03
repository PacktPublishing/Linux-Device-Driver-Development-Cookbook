/*
 * chrdev mmap() testing program
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

int main(int argc, char *argv[])
{
	int fd;
	void *addr;
	long len;
	char *ptr;
	int i;
	int ret;

	if (argc < 2) {
		fprintf(stderr, "usage: %s <dev> <size> [<c>]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	len = atol(argv[2]);

	ret = open(argv[1], O_RDWR);
	if (ret < 0) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	printf("file %s opened\n", argv[1]);
	fd = ret;

	/* Try to remap file into memory */
	addr = mmap(NULL, len, PROT_READ | PROT_WRITE,
			MAP_FILE | MAP_SHARED, fd, 0);
	if (addr == MAP_FAILED) {
		perror("mmap");
		exit(EXIT_FAILURE);
	}
	printf("got address=%p and len=%ld\n", addr, len);

	/* Do a (partial) dump of the file as it was a buffer of bytes */
	printf("---\n");
	ptr = (char *) addr;
	for (i = 0; i < len; i++)
		printf("%c", ptr[i]);

	/* Just in case, we modify the first byte by writing char <c> */
	if (argc == 4) {
		ptr[0] = argv[3][0];
		printf("---\nFirst character changed to '%c'\n", ptr[0]);
	}

	munmap(addr, len);
	close(fd);

	return 0;
}
