#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>

#define BUFFER_SIZE		1024 * 1024

static void *kptr;
static void *vptr;

/*
 * kmalloc GFP_KERNEL标志申请的内存位于直接映射区域
 * virt_addr = phys_addr + PAGE_OFFSE
 * 可使用virt_to_phys、__pa等接口获取内存物理地址
 */
static int kmalloc_memory_address(void)
{
	kptr = kmalloc(BUFFER_SIZE, GFP_KERNEL | __GFP_ZERO);
    if (!kptr)
	    return -ENOMEM;
    printk(KERN_INFO "kptr virt: 0x%p\n", kptr);
    printk(KERN_INFO "kptr virt_to_phys: 0x%llx\n", virt_to_phys(kptr));
	printk(KERN_INFO "kptr phys_to_virt(virt_to_phys(kptr)): 0x%p\n", phys_to_virt(virt_to_phys(kptr)));
	printk(KERN_INFO "x86 64 PAGE_OFFSET: 0x%lx\n", PAGE_OFFSET);
	printk(KERN_INFO "kptr __pa 0x%lx\n", __pa(kptr));
	printk(KERN_INFO "kptr virt - PAGE-OFFSET 0x%lx\n", (unsigned long)kptr - PAGE_OFFSET);
	printk(KERN_INFO "kptr +4k virt_to_phys 0x%llx\n", virt_to_phys((void *)((char *)kptr + 4 * 1024)));
	if(kptr) {
		kfree(kptr);
		kptr = NULL;	
	}
	return 0;
}

/*
 * vmalloc申请的内存位于非连续的物理内存区域
 * 该内存不存在直接映射关系
 * vmalloc_to_page API根据页表获取*page后再获取其物理地址
 */
static int vmalloc_memory_address(void)
{
	vptr = vmalloc(BUFFER_SIZE);
	if(!vptr)
		return -ENOMEM;
	printk(KERN_INFO "vptr virt: 0x%p\n", vptr);
	printk(KERN_INFO "vptr virt->page->phys 0x%llx\n", page_to_phys(vmalloc_to_page(vptr)) + ((unsigned long)vptr & ~PAGE_MASK));
	printk(KERN_INFO "vptr +4k virt->page->phys 0x%llx\n", page_to_phys(vmalloc_to_page(vptr + 4 *1024)) + ((unsigned long)vptr & ~PAGE_MASK));

	if(vptr) {
		vfree(vptr);
		vptr = NULL;
	}
	return 0;

}

static int __init my_module_init(void)
{
	kmalloc_memory_address();
	vmalloc_memory_address();
    return 0;
}

static void __exit my_module_exit(void)
{
    if (kptr)
	    kfree(kptr);
	if (vptr)
		vfree(vptr);
    printk(KERN_INFO "memory_addr module exit.\n");
}

module_init(my_module_init);
module_exit(my_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yqx");
MODULE_DESCRIPTION("A simple Linux kernel module about memory address.");
MODULE_VERSION("1.0");
