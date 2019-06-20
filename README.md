# OSProject1
Project One - [Group 11] 2019 台大作業系統

b06902101	b06902098	b05902099	b06902051	b05902132	b06902086

Problem description: http://rswiki.csie.org/dokuwiki/courses:107_2:project_1

Kernel files can be compiled along with kernel v5.0.3 - v5.0.6.
Kernel v5.0.6 could be downloaded at https://www.kernel.org/pub/linux/kernel/v5.x/linux-5.0.6.tar.xz. 
For other versions, use kernel/proj1.c directly, but other files may need to be merged manually.

It's easy to compile the kernel, but the installation process may differ by your distribution.  
A good instruction for Archlinux is at: https://wiki.archlinux.org/index.php/Kernel/Traditional_compilation 
For Ubuntu 16, see the link at details section here: http://rswiki.csie.org/dokuwiki/courses:104_2:homework_1

### Architecture
**OS_PJ1_Test** -> Files for test data<br>

**kernel_files** -> contains the system calls, we used things like fork and built-in Linux commands to create and manage parent and child processes<br>

**Gitignore** -> this file doesn’t do anything<br>

**FIFO.c**-> basically, FIFO’s algorithm is what the OS runs natively. That means we basically didn’t have to do any code in order for this one to work, other than counting current processes. Each process would run in the order received and terminate when finished.<br>
    Just like all the other scheduling algorithms, there is a count process to keep track of the number of processes. Normally we are using set_strategy to allocate our data structures into the heap (for C) but in this case, it's not needed because the order doesn't matter. The process pool ordering in main.c will suffice. Naturally, in FIFO, context switching is non-existent. The only use we have for keeping track of the number of processes in FIFO is to tell if the scheduler is empty. Resume_process is a function used to set the newly added process to maximum priority. We just use built in kernel functions to determine priority like sched_get_priority_max, and priorities in FIFO range from 1 to 99.<br>

**LICENSE**-> MIT<br>

**Makefile**
	-GCC (compiler collection)<br>
	-wall enables displaying error messages<br>
	-wextra for coding format (?)<br>
	LDFlags used on first compilation to determine flags<br>
	main file has dependencies of the files: main.o, FIFO.o, RR.o, SJF.o, PSJF.o, and heap.o meaning that if any of these output files change, the main file will be updated<br>

**main.c** -> Reads the input, manages timer and watches over events (children termination, new process arrival, time slice ending), then asking other files to handle them.<br>

**PSJF.c** -> uses a heap to determine the smallest jobs<br>
    heap is a natural structure for this because it's efficient and allows for easy swapping of the next smallest remaining time process as active process, keeping the next shortest ones sorted<br>
    The current time variable keeps track of the current time unit, to help figuring out remaining_time of each process.<br>

**RR.c** -> The advance function will give a rotating index of the job we want to access. This uses an array and remembers two j. A previous job being removed makes previous_active_id equal to -1. The basic thing to remember is that all the functions make sure the index of the active process is rotating circularly in the priority queue, until there are no more jobs to set to active.<br>

**SJF.c** -> The basic thing to remember is, there's no preemption in SJF, so we'll run a job until it's complete. After that, once context switching is called, we'll just set the active job to the top of the minimum heap which will naturally be the next shortest job. Since we pop that off the heap as soon as it's set to the active job, all we have to do to remove a currently processing job is set that variable to null.<br>

**error_test.sh** -> for testing the scheduler, parses any errors into error.txt<br>

**heap.c** -> contains all the functions needed for our priority queue, top, pop, size, empty are intuitive. It also has parent, left child, and right child accessor functions, upheap, downheap, and insert, and everything you'd expect a heap to have. The heap is, of course, sorted according to the remaining time of the jobs in the pool.<br>


**monopolize_cpu.sh** -> shell command 'echo -1 > /proc/sys/kernel/sched_rt_runtime_us' to guarantee only our processes get cpu time in realtime scheduling policy.<br>

**scheduler.h** -> defines some fundamental stuff that needs to be shared among the scheduler classes and main. 
