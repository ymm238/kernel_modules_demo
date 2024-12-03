#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define MY_IOC_MAGIC            'k'
#define MY_IOC_SHOW_INFO        _IO(MY_IOC_MAGIC, 0)

#define BUFFER_SIZE		4096
int main()
{
	char *buffer = "testabc\n";
	const char *file_path = "/proc/procfs";
	char buffer1[BUFFER_SIZE];
	char *mapped_buf;
	int result;
	int fd = open(file_path, O_RDWR);
	write(fd, buffer, BUFFER_SIZE);
	read(fd, buffer1, BUFFER_SIZE);
	printf("/proc/procfs context: %s\n", buffer1);

	mapped_buf = mmap(NULL, 2 * BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	
	if (mapped_buf == MAP_FAILED) {
		perror("mapped failed");
		close(fd);
		return -1;
	}

	printf("first page content: %s\n", mapped_buf);
	strncpy(mapped_buf, "first page modified", 20);
	
	result = ioctl(fd, MY_IOC_SHOW_INFO, NULL);
	if(result < 0) {
		perror("ioctl failed");
		close(fd);
		return -1;
	}
	
	printf("second page content: %s\n", mapped_buf + BUFFER_SIZE);
	strncpy(mapped_buf + BUFFER_SIZE, "second page modified", 20);
    
	result = ioctl(fd, MY_IOC_SHOW_INFO, NULL);
    if(result < 0) {
        perror("ioctl failed");
        close(fd);
        return -1;
    }
    close(fd);
	return 0;
}
