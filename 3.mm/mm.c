#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mm.h>


extern struct mm_struct *get_task_mm(struct task_struct *task);
extern void mmput(struct mm_struct *mm);

unsigned long task_statm(struct mm_struct *mm,
			 unsigned long *shared, unsigned long *text,
			 unsigned long *data, unsigned long *resident)
{
	*shared = get_mm_counter(mm, MM_FILEPAGES) +
			get_mm_counter(mm, MM_SHMEMPAGES);
	*text = (PAGE_ALIGN(mm->end_code) - (mm->start_code & PAGE_MASK))
								>> PAGE_SHIFT;
	*data = mm->data_vm + mm->stack_vm;
	*resident = *shared + get_mm_counter(mm, MM_ANONPAGES);
	return mm->total_vm;
}

static void get_task_statm(struct task_struct *task) {
	struct mm_struct *mm = get_task_mm(task);

	if (mm) {
		unsigned long size;
		unsigned long resident = 0;
		unsigned long shared = 0;
		unsigned long text = 0;
		unsigned long data = 0;

		size = task_statm(mm, &shared, &text, &data, &resident);
		mmput(mm);
		
		pr_info("task pid %d name %s size %lu, resident %lu, shared %lu, text %lu, data %lu\n",
				task->pid, task->comm, size, resident, shared, text, data);

	} else {
		pr_info("task pid %d name %s no mm creat\n", task->pid, task->comm);
	}
}


static int __init mm_init(void)
{
	struct task_struct *task;

	pr_info("init\n");
	list_for_each_entry(task, &init_task.tasks, tasks) {
    	if (task->pid == 1) {
        	pr_info("Found init process: %s [PID: %d]\n", task->comm, task->pid);
			get_task_statm(task);
	    }
	}
	return 0;
}

static void __exit mm_exit(void)
{
	pr_info("exit\n");
}

module_init(mm_init);
module_exit(mm_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yqx");
MODULE_DESCRIPTION("A simple module for mm");
