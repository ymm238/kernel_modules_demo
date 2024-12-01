#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include "procfs.h"

#define PROC_NAME       "procfs"
#define MAX_BUF_SIZE    4 * 1024

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
    struct vm_area_struct *vma = vmf->vma;
    unsigned long address = vmf->address;
    void *new_addr;
    struct page *page;

    // 分配页对齐的内存
    new_addr = kmalloc(MAX_BUF_SIZE, GFP_KERNEL);
    if (!new_addr)
        return VM_FAULT_OOM;

    // 获取页面并增加引用计数
    page = virt_to_page(new_addr);
    get_page(page);

    return vm_insert_page(vma, address, page);
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
    if(new_addr)
		kfree(new_addr);
	pr_info("Proc file '%s' removed\n", PROC_NAME);
}

module_init(my_proc_init);
module_exit(my_proc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple procfs read/write template using traditional methods");
