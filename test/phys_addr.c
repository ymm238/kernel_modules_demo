#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <asm/io.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yqx");
MODULE_DESCRIPTION("A simple Linux kernel module.");
MODULE_VERSION("1.0");

void *p;

static int __init my_module_init(void) {
    p = kmalloc(1024*1024, GFP_KERNEL);
    if (!p)
	    return -ENOMEM;
    printk(KERN_INFO "virt 0x%p\n", p);
    printk(KERN_INFO "virt_to_phys 0x%llx\n", virt_to_phys(p));
    printk(KERN_INFO "virt_to_phys + 4k 0x%llx\n", virt_to_phys((void *)((char *)p + 4 * 1024)));
    memset(p, 0, 1024*1024);

    printk(KERN_INFO "Hello, World! My module is loaded.\n");
    return 0;
}

static void __exit my_module_exit(void) {
    if (p)
	    kfree(p);
    printk(KERN_INFO "Goodbye, World! My module is unloaded.\n");
}

module_init(my_module_init);
module_exit(my_module_exit);
