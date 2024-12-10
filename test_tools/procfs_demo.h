#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_BUF_SIZE			(4096 * 4)
typedef struct ioctl_data {
	int				value;
	bool			result;
	unsigned long 	addr;
	char			msg[1024];
};


#define KOS_IOC_MAGIC			'k'

#define KOS_IOC_ALLOC		_IOWR(KOS_IOC_MAGIC, 0, struct ioctl_data)
#define KOS_IOC_INIT		_IOWR(KOS_IOC_MAGIC, 1, struct ioctl_data)
#define KOS_IOC_FREE		_IOWR(KOS_IOC_MAGIC, 2, struct ioctl_data)
#define KOS_IOC_SHOW		_IO(KOS_IOC_MAGIC, 3)
#define KOS_IOC_INFO		_IO(KOS_IOC_MAGIC, 4)
#define KOS_IOC_INSERT		_IOW(KOS_IOC_MAGIC, 5, struct ioctl_data)
#define KOS_IOC_DEC			_IOW(KOS_IOC_MAGIC, 6, struct ioctl_data)
