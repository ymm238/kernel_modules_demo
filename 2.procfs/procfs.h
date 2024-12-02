#include <uapi/asm-generic/ioctl.h>
#define MY_IOC_MAGIC			'k'

#define MY_IOC_SHOW_INFO		_IO(MY_IOC_MAGIC, 0)
