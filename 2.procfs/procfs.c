#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <asm/page_types.h>

#include "procfs.h"

#define PROC_NAME		"procfs"
#define MAX_BUF_SIZE	(PAGE_SIZE * 4)

extern int vm_munmap(unsigned long start, size_t len);

static struct proc_dir_entry *proc_file;
static char kernel_buf[MAX_BUF_SIZE];	/* 内核缓冲区 */
static size_t data_size;				/* 实际数据大小 */

static char *message_buffer[1024];
static int buffer_size;

static ssize_t my_proc_read(struct file *file, char __user *buf,
								size_t count, loff_t *ppos)
{
	size_t copy_size;
	int ret;

	if (*ppos >= data_size)
		return 0;

	copy_size = min((size_t)(data_size - *ppos), count);
	ret = copy_to_user(buf, kernel_buf + *ppos, copy_size);
	if (ret) {
		pr_err("Failed to copy data to user. ret %d\n", ret);
		 return -EFAULT;
	}
	*ppos += copy_size;
	return copy_size;
}

static ssize_t my_proc_write(struct file *file, const char __user *buf,
								 size_t count, loff_t *ppos)
{
	size_t write_size;
	int ret;

	write_size = min(count, (size_t)MAX_BUF_SIZE - 1);

	memset(kernel_buf, 0, MAX_BUF_SIZE);

	ret = copy_from_user(kernel_buf, buf, write_size);
	if (ret) {
	pr_err("Failed to copy data from user. ret %d\n", ret);
	return -EFAULT;
	}

	kernel_buf[write_size] = '\0';
	data_size = write_size;
	return count;
}

static vm_fault_t page_fault_handler(struct vm_fault *vmf)
{
	struct page *page;
	unsigned long vmf_message_address;
	unsigned long address = vmf->address;
	unsigned long offset = vmf->pgoff;  // 页偏移量

	pr_info("Page Fault occurred at address: 0x%llx, offset: %lu\n",
		address, offset);

	vmf_message_address = message_buffer[offset];
	page = virt_to_page(vmf_message_address);

	vmf->page = page;
	return 0;
}

static const struct vm_operations_struct my_vm_ops = {
	.fault = page_fault_handler,
};

static int my_proc_mmap(struct file *file, struct vm_area_struct *vma)
{
	vma->vm_ops = &my_vm_ops;
	vma->vm_flags |= VM_IO | VM_DONTEXPAND;
	return 0;
}

static void print_message_buffer(void)
{
	int i, j, start_index = 0, end_index = 0;
	char *buffer;
	unsigned long phys_addr;
	for (i = 0 ; i < 1024; i++) {
		buffer = message_buffer[i];
		if (buffer == NULL)
			continue;
		phys_addr = virt_to_phys(buffer);
		for (j = 0; j < PAGE_SIZE - 1; j++)
		{
			if (*(buffer + j) == '\0' && *(buffer + j + 1) != '\0')
				start_index = j + 1;
			if (*(buffer + j) != '\0' && *(buffer + j + 1) == '\0')
			{
				end_index = j;
				pr_info("phys_addr start_index: 0x%llx, end_index: 0x%llx\ncontent: %s\n",
						phys_addr + start_index, phys_addr + end_index, buffer + start_index);
			}
		}
	}
}

static char *message_buffer_alloc(int size)
{
	int i;
	int pages = size / PAGE_SIZE;
	char message_buffers_address[1024], addr[100];

	buffer_size = size;
	for (i = 0; i < pages; i++) {
		message_buffer[i] = kmalloc(size, GFP_KERNEL | __GFP_ZERO);
		if (message_buffer[i]) {
			sprintf(addr, "page %d phys_addr", i, virt_to_phys(message_buffer[i]));
			strcat(message_buffers_address, addr);
		}
	}
	return message_buffers_address;
}

static bool message_buffer_init(char* buf)
{
	if (*buf)
		strcpy(message_buffer[0], buf);
	pr_info("messae buffer %s \n", message_buffer[0]);
	if (strcmp(message_buffer[0], buf))
		return false;
	return true;
}

static bool message_buffer_free(int size)
{
	int i, end;
	int pages = size / PAGE_SIZE;

	for (i = 0; i < 1024; i++) {
		if (message_buffer[i] != NULL)
			end = i + 1;
	}
	if (end < pages)
		return false;
	for (i = end - pages; i < end; i++) {
		kfree(message_buffer[i]);
		message_buffer[i] = NULL;
	}
	return true;
}

static void message_buffer_info(void)
{
	int i, j, percentage;
	char *buffer;
	unsigned long start_address, remain, available_address, usage;
	for (i = 0; i < 1024; i++) {
		buffer = message_buffer[i];
		if (buffer == NULL)
			continue;
		percentage = 0;
		usage = 0UL;
		available_address = 0UL;
		start_address = virt_to_phys(buffer);
		for (j = 0; j < buffer_size; j++) {
			if (*(char *)(buffer + j) != '\0')
				usage ++;
			else
				if (available_address == 0)
					available_address = virt_to_phys(buffer + j);
		}
		remain = buffer_size - usage;
		if (buffer_size)
			percentage = remain * 100 / buffer_size;
		pr_info("page %d, remain %ld, percentage %d, start_address 0x%llx, available_address 0x%llx\n",
					i, remain, percentage, start_address, available_address);
	}
	print_message_buffer();
}

static void message_buffer_insert(unsigned long offset, char *buf)
{
	strcpy(message_buffer[0] + offset, buf);
	print_message_buffer();
}

static void message_buffer_dec(int size)
{
	int i, dec_count = 0;
	for (i = 0; i < buffer_size; i++) {
		if (*(message_buffer[0] + i) == '\0') {
			*(message_buffer[0] + i) = '1';
			dec_count ++;
			if (dec_count == size)
				break;
		}
	}
	print_message_buffer();
}

static long my_proc_ioctl(struct file *file, unsigned int cmd, unsigned long __user arg)
{
	struct ioctl_data data;
	int size, ret;
	bool result;

	switch(cmd) {
		case KOS_IOC_ALLOC:
			pr_info("================ KOS_IOC_ALLOC ================");
			ret = copy_from_user(&data, arg, sizeof(struct ioctl_data));
			if (ret) {
				pr_err("Failed to copy data from user. ret %d\n", ret);
			return -EFAULT;
			}

			size = data.value;
			memset(&data, 0, sizeof(struct ioctl_data));
			data.addr = message_buffer_alloc(size);
			pr_info("phys addr 0x%lx\n", data.addr);

			ret = copy_to_user(arg, &data, sizeof(struct ioctl_data));
			if (ret) {
				pr_err("Failed to copy data to user. ret %d\n", ret);
				return -EFAULT;
			}
			break;
		case KOS_IOC_INIT:
			pr_info("================ KOS_IOC_INIT =================");
			ret = copy_from_user(&data, arg, sizeof(struct ioctl_data));
			if (ret) {
				pr_err("Failed to copy data from user. ret %d\n", ret);
				 return -EFAULT;
			}
			
			result = message_buffer_init(data.msg);
			memset(&data, 0, sizeof(struct ioctl_data));
			data.result = result;
			pr_info("result: %s\n", result ? "success" : "failed");

			ret = copy_to_user(arg, &data, sizeof(struct ioctl_data));
			if (ret) {
				pr_err("Failed to copy data to user. ret %d\n", ret);
				return -EFAULT;
			}
			break;
		/* TODO: free一部分内存 */
		case KOS_IOC_FREE:
			pr_info("================ KOS_IOC_FREE =================");
			ret = copy_from_user(&data, arg, sizeof(struct ioctl_data));
			if (ret) {
				pr_err("Failed to copy data from user. ret %d\n", ret);
				 return -EFAULT;
			}

			size = data.value;
			result = message_buffer_free(size);
			memset(&data, 0, sizeof(struct ioctl_data));
			data.result = result;
			ret = copy_to_user(arg, &data, sizeof(struct ioctl_data));
			if (ret) {
				pr_err("Failed to copy data to user. ret %d\n", ret);
				return -EFAULT;
			}
			break;
		case KOS_IOC_SHOW:
			pr_info("================ KOS_IOC_SHOW =================");
			print_message_buffer();
			break;
		case KOS_IOC_INFO:
			pr_info("================ KOS_IOC_INFO =================");
			message_buffer_info();
			break;
		case KOS_IOC_INSERT:
			pr_info("================ KOS_IOC_INSERT ===============");
			ret = copy_from_user(&data, arg, sizeof(struct ioctl_data));
			if (ret) {
				pr_err("Failed to copy data from user. ret %d\n", ret);
				 return -EFAULT;
			}

			message_buffer_insert(data.value, data.msg);
			break;
		case KOS_IOC_DEC:
			pr_info("================ KOS_IOC_DEC ==================");
			ret = copy_from_user(&data, arg, sizeof(struct ioctl_data));
			if (ret) {
				pr_err("Failed to copy data from user. ret %d\n", ret);
				 return -EFAULT;
			}

			size = data.value;
			message_buffer_dec(size);
			memset(&data, 0 , sizeof(struct ioctl_data));
			break;
		default:
			break;
	}
	return 0;
}

static const struct proc_ops proc_ops = {
	.proc_read = my_proc_read,
	.proc_write = my_proc_write,
	.proc_mmap = my_proc_mmap,
	.proc_ioctl = my_proc_ioctl,
};

static int __init my_proc_init(void)
{
	proc_file = proc_create(PROC_NAME, 0666, NULL, &proc_ops);
	if (!proc_file) {
		pr_err("Failed to create proc file\n");
	return -ENOMEM;
	}

	strscpy(kernel_buf, "Hello from kernel!", MAX_BUF_SIZE);
	data_size = strlen(kernel_buf);

	pr_info("Proc file '%s' created\n", PROC_NAME);
	return 0;
}

static void __exit my_proc_exit(void)
{
	proc_remove(proc_file);
//	if (message_buffer)
//		kfree(message_buffer);
}

module_init(my_proc_init);
module_exit(my_proc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yqx");
MODULE_DESCRIPTION("A simple procfs read/write template using traditional methods");
