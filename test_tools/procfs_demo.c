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
	ioctl(fd,  KOS_IOC_ALLOC, &data);
	printf("alloc phys addr: 0x%lx\n", data.addr);

	/* 初始化内存 */
	strcpy(data.msg, "This is the first test buffer.\n");
	ioctl(fd, KOS_IOC_INIT, &data);
	printf("init buffer result, %s\n", data.result ? "success" : "failed");
	
	/* 部分释放 */
	data.value = 4096;
	ioctl(fd, KOS_IOC_FREE, &data);
	printf("free buffer result, %s\n", data.result ? "success" : "failed");

	/* 显示物理地址及内容 */
	ioctl(fd, KOS_IOC_SHOW);

	/* 显示可用容量 可用容量比例 起始物理地址 可用起始物理地址 */
	ioctl(fd, KOS_IOC_INFO);

	/* 指定偏移处插入指定数据 */
	data.value = 0x40;
	strcpy(data.msg, "This is the second buffer.\n");
	ioctl(fd, KOS_IOC_INSERT, &data);

	/* 减小可用内存容量 */
	data.value = 0x40;
	ioctl(fd, KOS_IOC_DEC, &data);


	message_buffer = mmap(NULL, MAX_BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (message_buffer == MAP_FAILED) {
		perror("mapped failed");
		close(fd);
		return -1;
	}
	
	printf("message buffer %s\n", message_buffer);
	printf("message buffer + 0x40 %s\n", message_buffer + 0x40);

	munmap(message_buffer, MAX_BUF_SIZE);
	close(fd);
	return 0;
}
