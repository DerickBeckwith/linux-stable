/*
 * 17 APR 2014 Derick Beckwith Lab2
 * Add a new system call named comp7506.
 */

#include <linux/syscalls.h>
#include <linux/printk.h>

SYSCALL_DEFINE0(comp7506)
{
	/* Get the currently running process's process id. */
	pid_t current_pid = current->pid;

	/* Get the current time. */
	struct timeval t;
	struct tm time_stamp;
	int timezone_offset = sys_tz.tz_minuteswest * -60;
	do_gettimeofday(&t);
	time_to_tm(t.tv_sec, timezone_offset, &time_stamp);

	/* Print to the kernel log. */
	printk("Processs %d made the Spring 2014, "
			"Auburn University COMP7506 system call at "
			"%d:%d:%d:%ld\n",
			current_pid,
			time_stamp.tm_hour,
			time_stamp.tm_min,
			time_stamp.tm_sec,
			t.tv_usec);

	return 0;
}
