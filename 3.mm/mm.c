#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/sysinfo.h>
#include <uapi/asm-generic/mman-common.h>
#include <asm/uaccess_64.h>
#include <linux/mmap_lock.h>
#include <linux/blkdev.h>

extern struct mm_struct *get_task_mm(struct task_struct *task);
extern void mmput(struct mm_struct *mm);
extern void si_meminfo(struct sysinfo *val);

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

static unsigned long long get_total_pages(void)
{
	struct sysinfo si;
	si_meminfo(&si);
	
	return si.totalram;
}

static unsigned long get_task_statm(struct task_struct *task)
{
	unsigned long size;
	unsigned long resident = 0; // 物理pages
	unsigned long shared = 0;
	unsigned long text = 0;
	unsigned long data = 0;
	struct mm_struct *mm = get_task_mm(task);

	if (mm) {

		size = task_statm(mm, &shared, &text, &data, &resident);
		mmput(mm);
		/* 	
		pr_info("========= task pid %d name %s =========\n"
				"虚拟物理地址空间 	total size %lu,\n"
				"驻留的物理内存		resident size %lu,\n"
				"共享内存			shared %lu,\n"
				"代码段			text %lu,\n"
				"数据段			data %lu\n",
				task->pid, task->comm, size, resident, shared, text, data);
		*/

	} else {
		pr_info("task pid %d name %s no mm creat\n", task->pid, task->comm);
	}
	return resident;
}

// 遍历并修改内存建议
int advise_process_vm_pageout(struct task_struct *task)
{
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	unsigned long start, end;
 	int ret;

	mm = task->mm;
	if (!mm) {
		pr_err("No mm_struct found for PID %d\n", task->pid);
 		return -EINVAL;
	}

	// 遍历进程的虚拟内存区域 (VMA)
	unsigned long index = 0;
 	mt_for_each(&mm->mm_mt, vma, index, ULONG_MAX) {
		start = vma->vm_start;
 		end = vma->vm_end;

 		pr_info("VMA: [0x%lx - 0x%lx] Size: %lu bytes\n", start, end, end - start);
		
		// TODO: 
		// 当前do_madvise采用手动EXPORT_SYMBOL方式导出符号，
		// 对于未导出符号的函数，采用kallsyms_lookup_name。
		ret = do_madvise(mm, start, end, MADV_PAGEOUT);
 		if (ret) {
			 pr_err("do_madvise failed for VMA [0x%lx - 0x%lx] with error %d\n", start, end, ret);
 		} else {
			 pr_info("MADV_PAGEOUT applied for VMA [0x%lx - 0x%lx]\n", start, end);
 		}
	}

	return 0;
}

static int __init mm_init(void)
{
	struct task_struct *task;
	unsigned long long total_pages;
	unsigned long task_pages;

	total_pages = get_total_pages();
	pr_info("total pages: %llu\n", total_pages);
	
	list_for_each_entry(task, &init_task.tasks, tasks) {
		task_pages = get_task_statm(task);
		// SSE register return with SSE disabled
		// if ((double)task_pages / total_pages > 0.5)
		// if (task_pages * 2 > total_pages)
		if (strcmp(task->comm, "mm_demo") == 0) {
			pr_info("task->pid %d, task->comm %s pages should be placed in the swap space\n",
					task->pid, task->comm);
			advise_process_vm_pageout(task);
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
