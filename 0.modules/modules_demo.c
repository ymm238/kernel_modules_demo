#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

static int debug_enable = 0;
module_param(debug_enable, int, 0644);
MODULE_PARM_DESC(debug_enable, "The debug level.");

static int __init my_module_init(void)
{
    pr_info("The current debug_enable param is %d\n", debug_enable);
	pr_info("pr_xxx: This is a info-level log.\n");
	pr_warn("pr_xxx: This is a warning-level log.\n");
	pr_err("pr_xxx: This is a error-level log.\n");
	pr_alert("pr_xxx: This is a alert-level log.\n");
    printk(KERN_INFO "printk: This is a info-level log.\n");
    printk(KERN_WARNING "printk: This is a warning-level log.\n");
    printk(KERN_ERR "printk: This is a error-level log.\n");
    printk(KERN_ALERT "printk: This is a alert-level log.\n");

    return 0;
}

static void __exit my_module_exit(void)
{
    pr_info("Module exit\n");
}

module_init(my_module_init);
module_exit(my_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yqx");
MODULE_DESCRIPTION("Modules init and exit.");
MODULE_VERSION("1.0");
