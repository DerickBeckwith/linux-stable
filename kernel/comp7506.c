#include <linux/syscalls.h>
#include <linux/printk.h>

SYSCALL_DEFINE0(comp7506)
{
	pid_t current_pid = current->pid;
	printk("Processs %i made the Spring 2014, Auburn University COMP7506 system call.\n",
			current_pid);
	return 0;
}
