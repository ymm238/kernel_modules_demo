#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#define MY_IOC_MAGIC            'k'
#define MY_IOC_SHOW_INFO        _IO(MY_IOC_MAGIC, 0)

#define BUFFER_SIZE		1024
char *buffer = "testabc\n";
char buffer1[BUFFER_SIZE];
char *mapped_buf;
int main()
{
	const char *file_path = "/proc/procfs";
	int fd = open(file_path, O_RDWR);
	write(fd, buffer, BUFFER_SIZE);
	read(fd, buffer1, BUFFER_SIZE);
	printf("/proc/procfs context: %s\n", buffer1);

	mapped_buf = mmap(NULL, BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	
	if (mapped_buf == MAP_FAILED) {
		perror("mapped failed");
		close(fd);
		return -1;
	}
	mapped_buf[0] = 'a';
	int result = ioctl(fd, MY_IOC_SHOW_INFO, NULL);
	if(result < 0) {
		perror("ioctl failed");
		close(fd);
		return -1;
	}
	
	close(fd);
	return 0;
}
