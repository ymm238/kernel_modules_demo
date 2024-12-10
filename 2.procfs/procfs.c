#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
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

static char *message_buffer;
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


	vmf_message_address = message_buffer + PAGE_SIZE * offset;
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
	int i, start_index = 0, end_index = 0;
	phys_addr_t phys_addr = virt_to_phys(message_buffer);
	for (i = 0; i < buffer_size - 1; i++)
	{
		if (*(message_buffer + i) == '\0' && *(message_buffer + i + 1) != '\0')
			start_index = i + 1;
		if (*(message_buffer + i) != '\0' && *(message_buffer + i + 1) == '\0')
		{
			end_index = i;
			pr_info(">> phys_addr start_index: 0x%llx, end_index: 0x%llx\ncontent: %s\n",
					phys_addr + start_index, phys_addr + end_index, message_buffer + start_index);
		}
	}
}

static unsigned long message_buffer_alloc(int size)
{
	if (message_buffer)
		return 0;

	message_buffer = kmalloc(size, GFP_KERNEL | __GFP_ZERO);
	buffer_size = size;
	if (message_buffer)
		return virt_to_phys(message_buffer);
	return -1;
}

static bool message_buffer_init(char* buf)
{
	if (*buf)
		strcpy(message_buffer, buf);
	else
		memset(message_buffer, 0, buffer_size);
	pr_info("messae buffer %s \n", message_buffer);
	if (strcmp(message_buffer, buf))
		return false;
	return true;
}

static bool message_buffer_free(int size)
{
	int ret;
	ret = vm_munmap(message_buffer + buffer_size - size, size);
	pr_info("free ret %d\n", ret);
	if (ret == 0) {
		buffer_size -= size;
		return true;
	}
	return false;
}

static void message_buffer_info(void)
{
	int i;
	int percentage = 0;
	unsigned long start_address, remain,
				  available_address = 0UL,
				  usage = 0UL;

	start_address = virt_to_phys(message_buffer);

	for (i = 0; i < buffer_size; i++) {
		if (*(char *)(message_buffer + i) != '\0')
			usage ++;
		else
			if (available_address == 0)
				available_address = virt_to_phys(message_buffer + i);
	}
	remain = buffer_size - usage;
	if (buffer_size)
		percentage = remain * 100 / buffer_size;
	pr_info("[message_buffer_info] remain %ld, percentage %d, start_address 0x%llx, available_address 0x%llx\n",
					remain, percentage, start_address, available_address);
	print_message_buffer();
}

static void message_buffer_insert(unsigned long offset, char *buf)
{
	strcpy(message_buffer + offset, buf);
}

static long my_proc_ioctl(struct file *file, unsigned int cmd, unsigned long __user arg)
{
	struct ioctl_data data;
	int size, ret;
	bool result;

	switch(cmd) {
		case KOS_IOC_ALLOC:
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
			ret = copy_from_user(&data, arg, sizeof(struct ioctl_data));
			if (ret) {
				pr_err("Failed to copy data from user. ret %d\n", ret);
				 return -EFAULT;
			}
			
			result = message_buffer_init(data.msg);
			memset(&data, 0, sizeof(struct ioctl_data));
			data.result = result;
			pr_info("message_buffer_init result: %s\n", result ? "success" : "failed");

			ret = copy_to_user(arg, &data, sizeof(struct ioctl_data));
			if (ret) {
				pr_err("Failed to copy data to user. ret %d\n", ret);
				return -EFAULT;
			}
			break;
		/* TODO: free一部分内存 */
		case KOS_IOC_FREE:
			ret = copy_from_user(&data, arg, sizeof(struct ioctl_data));
			if (ret) {
				pr_err("Failed to copy data from user. ret %d\n", ret);
				 return -EFAULT;
			}

			pr_info("before message_buffer_free %ld\n", ksize(message_buffer));
			size = data.value;
			result = message_buffer_free(size);
			memset(&data, 0, sizeof(struct ioctl_data));
			data.result = result;
			pr_info("after message_buffer_free %ld\n", ksize(message_buffer));
			ret = copy_to_user(arg, &data, sizeof(struct ioctl_data));
			if (ret) {
				pr_err("Failed to copy data to user. ret %d\n", ret);
				return -EFAULT;
			}
			break;
		case KOS_IOC_SHOW:
			if(message_buffer)
				pr_info("message_buffer_show physical address 0x%llx\n", virt_to_phys(message_buffer));
			else
				pr_info("message_buffer not alloc\n");
			break;
		case KOS_IOC_INFO:
			message_buffer_info();
			break;
		case KOS_IOC_INSERT:
			ret = copy_from_user(&data, arg, sizeof(struct ioctl_data));
			if (ret) {
				pr_err("Failed to copy data from user. ret %d\n", ret);
				 return -EFAULT;
			}

			message_buffer_insert(data.value, data.msg);
			break;
		case KOS_IOC_DEC:
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
