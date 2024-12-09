#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <stdbool.h>

#include "procfs_demo.h"

int main()
{
	int ret;
	struct ioctl_data data;

	const char *file_path = "/proc/procfs";
	int fd = open(file_path, O_RDWR);
	
	char *buffer = "test abc\n";
	char* message_buffer;

	/* 申请内存 */
	data.value = MAX_BUF_SIZE;
	ret = ioctl(fd,  KOS_IOC_ALLOC, &data);
	printf("alloc phys addr: 0x%lx\n", data.addr);

	message_buffer = mmap(NULL, MAX_BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (message_buffer == MAP_FAILED) {
		perror("mapped failed");
		close(fd);
		return -1;
	}

	close(fd);
	return 0;
}
