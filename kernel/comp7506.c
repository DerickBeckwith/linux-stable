#include <linux/syscalls.h>
#include <linux/printk.h>

SYSCALL_DEFINE1(comp7506, int pid)
{
	printk("Processs %i made the Spring 2014, Auburn University COMP7500 system call.\n", pid);
	return 0;
}