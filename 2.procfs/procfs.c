#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include "procfs.h"

#define PROC_NAME       "procfs"
#define MAX_BUF_SIZE    4096

/* 全局变量 */
static struct proc_dir_entry *proc_file;
static char kernel_buf[MAX_BUF_SIZE];    /* 内核缓冲区 */
static size_t data_size;                 /* 实际数据大小 */

static void* new_addr;
/* read 实现 */
static ssize_t my_proc_read(struct file *file, char __user *buf,
                           size_t count, loff_t *ppos)
{
    size_t copy_size;
    int ret;

    /* 检查是否已读取完毕 */
    if (*ppos >= data_size)
        return 0;

    /* 计算本次要复制的大小 */
    copy_size = min((size_t)(data_size - *ppos), count);

    /* 将数据拷贝到用户空间 */
    ret = copy_to_user(buf, kernel_buf + *ppos, copy_size);
    if (ret) {
        pr_err("Failed to copy data to user. ret %d\n", ret);
        return -EFAULT;
    }

    /* 更新文件位置 */
    *ppos += copy_size;

    return copy_size;
}

/* write 实现 */
static ssize_t my_proc_write(struct file *file, const char __user *buf,
                            size_t count, loff_t *ppos)
{
    size_t write_size;
    int ret;

    /* 防止缓冲区溢出 */
    write_size = min(count, (size_t)MAX_BUF_SIZE - 1);

    /* 清空原有数据 */
    memset(kernel_buf, 0, MAX_BUF_SIZE);

    /* 从用户空间复制数据 */
    ret = copy_from_user(kernel_buf, buf, write_size);
    if (ret) {
        pr_err("Failed to copy data from user. ret %d\n", ret);
        return -EFAULT;
    }

    /* 确保字符串以 null 结尾 */
    kernel_buf[write_size] = '\0';
    
    /* 更新数据大小 */
    data_size = write_size;

    /* 返回实际写入的字节数 */
    return count;
}

static vm_fault_t page_fault_handler(struct vm_fault *vmf)
{
	struct page *page;
    unsigned long address = vmf->address;
    unsigned long offset = vmf->pgoff;  // 页偏移量

    // 打印调试信息
    printk(KERN_INFO "Page Fault occurred at address: 0x%lx, offset: %lu\n", 
           address, offset);

    // 确保页面偏移在允许范围内
    if (offset >= 2) {
        printk(KERN_ERR "Page offset out of range\n");
        return VM_FAULT_SIGBUS;
    }

    // 分配物理页
    page = alloc_page(GFP_KERNEL);
    if (!page) {
        printk(KERN_ERR "Failed to allocate page\n");
        return VM_FAULT_OOM;
    }

    // 清零页面
    clear_page(page_address(page));

    // 在第一个页面写入一些测试数据
    if (offset == 0) {
        new_addr = page_address(page);
        sprintf(new_addr, "First Page: Offset %lu\n", offset);
    }

    // 在第二个页面写入不同的数据
    if (offset == 1) {
        new_addr = page_address(page);
        sprintf(new_addr, "Second Page: Offset %lu\n", offset);
    }

    // 直接设置页错误的页面
    vmf->page = page;

    return 0;
}

static const struct vm_operations_struct my_vm_ops = {
    .fault = page_fault_handler,
};

static int my_proc_mmap(struct file *file, struct vm_area_struct *vma)
{
    vma->vm_ops = &my_vm_ops;
    
    // 使用 VM_RESERVED 或更新的标志
    vma->vm_flags |= VM_IO | VM_DONTEXPAND;

    return 0;
}
/* ioctl结构体 */
static long my_proc_ioctl(struct file *file, unsigned int cmd, unsigned long __arg)
{
	switch(cmd) {
		case MY_IOC_SHOW_INFO:
			// pr_info("MY_IOC_SHOW_INFO %c\n", ((char *)new_addr)[0]);
			pr_info("MY_IOC_SHOW_INFO %s\n", (char *)new_addr);
			break;
		default:
			break;
	}
	return 0;
}

/* 文件操作结构体 */
static const struct proc_ops proc_ops = {
    .proc_read = my_proc_read,
    .proc_write = my_proc_write,
	.proc_mmap = my_proc_mmap,
	.proc_ioctl = my_proc_ioctl,
};

/* 模块初始化函数 */
static int __init my_proc_init(void)
{
    /* 创建 proc 文件 */
    proc_file = proc_create(PROC_NAME, 0666, NULL, &proc_ops);
    if (!proc_file) {
        pr_err("Failed to create proc file\n");
        return -ENOMEM;
    }

    /* 初始化数据 */
    strscpy(kernel_buf, "Hello from kernel!", MAX_BUF_SIZE);
    data_size = strlen(kernel_buf);

    pr_info("Proc file '%s' created\n", PROC_NAME);
    return 0;
}

/* 模块清理函数 */
static void __exit my_proc_exit(void)
{
    /* 移除 proc 文件 */
    proc_remove(proc_file);
	pr_info("Proc file '%s' removed\n", PROC_NAME);
}

module_init(my_proc_init);
module_exit(my_proc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple procfs read/write template using traditional methods");
