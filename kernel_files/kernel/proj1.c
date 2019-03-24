// System calls for project 1
#include <linux/linkage.h>
#include <linux/syscalls.h>
#include <linux/ktime.h>
#include <linux/types.h> // for pid_t
#include <linux/uaccess.h> // user space memory manipulation
#include <linux/compiler.h> // for __user

/*
 * Since the user mode scheduler needs to maintain process information anyway,
 * I think it's better to let schedular save process start time, too.
 * This may lead to extra copying, if we were to implement that safely.
 *
 * Maybe we can write a version which saves the start time in kernel space.
 * That requires extra bookeeping, which is certainly not without overhead.
 * */


/* system call implementation */

static inline void _print_process_info(pid_t pid, 
		const struct timespec *start_time,
		const struct timespec *stop_time)
{ 
    printk("[Project1] %d %ld.%ld %ld.%ld", pid, 
            start_time->tv_sec, start_time->tv_nsec, 
            stop_time->tv_sec, stop_time->tv_nsec
            );
}

/*
 * In practice kernel code doesn't access user space pointer directly.  
 * There's no guarantee that the pointer really points to user space,
 * and in some architectures it's impossible to dereference user space pointers.
 * As a result, the kernel should access user pointer using copy_{from|to}_user(). 
 * */
 
static inline long _start_process_safe(pid_t unused, 
		struct timespec __user *start_time_ptr)
{
    struct timespec current_time;
    int bytes_not_copied;
    getnstimeofday(&current_time);
    bytes_not_copied = copy_to_user(start_time_ptr, &current_time, sizeof(struct timespec));
    if (bytes_not_copied != 0){
        printk("[Project1] %d bytes can't be copied to user pointer %px!"
			, bytes_not_copied, start_time_ptr);
	return -EFAULT;
    }
    return 0;
}

static inline long _stop_process_safe(pid_t pid, 
		const struct timespec __user *start_time_ptr)
{
    struct timespec start_time, stop_time;
    int bytes_not_copied = copy_from_user(&start_time, start_time_ptr, sizeof(struct timespec));
    if (bytes_not_copied != 0){
        printk("[Project1] %d bytes can't be read from user pointer %px!"
			, bytes_not_copied, start_time_ptr);
        return -EFAULT;
    }
    _print_process_info(pid, &start_time, &stop_time);
    return 0;
}

/* In x86 it's possible to access user space directly. */

static inline long _start_process_unsafe(pid_t unused, 
		struct timespec __user *start_time_ptr)
{
    getnstimeofday(start_time_ptr);
    return 0;
}
static inline long _stop_process_unsafe(pid_t pid, const struct timespec __user *start_time_ptr)
{
    struct timespec stop_time;
    getnstimeofday(&stop_time);
    _print_process_info(pid, start_time_ptr, &stop_time);
    return 0;
}

/* The actual definition of the system calls. */

SYSCALL_DEFINE2(proj1_start_process, 
		pid_t, unused, /* so the interfaces are unifrom */
                struct timespec __user *, start_time_ptr)
{
    /* Since the only callers are written by us,
     * I believe it's okay in this project that we access user space directly. 
     */
    return _start_process_unsafe(unused, start_time_ptr);
}

SYSCALL_DEFINE2(proj1_stop_process,
		pid_t, pid,
		const struct timespec __user *, start_time_ptr)
{
    return _stop_process_unsafe(pid, start_time_ptr);
}
