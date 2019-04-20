#include <linux/module.h>
#include <linux/kernel.h>

/*int init_module(void) {
    printk(KERN_INFO "\n");
    return 0;
}*/


void printProcess(ProcessInfo p) {

/* dmesg output
TAG PID ST FT
TAG - the tag to identify the message of this project.
PID - the process ID of this process
ST - the start time of this process in the format seconds.nanoseconds.
FT - the finish time of this process in the format seconds.nanoseconds.

ex: [Project1] 4007 1394530429.657899070 1394530430.004979285

ProcessInfo contents
    int ready_time;
    int time_needed; // Same as execution time in the problem description i.e. time needed to run the process.
    int remaining_time; // Remaining time for the process; Use in PSJF to determine the process to be run.
    pid_t pid;
    ProcessStatus status;
    char *name;
*/

      
  printf(KERN_INFO "\n";
}

/*void cleanup_module(void) {
    printk(KERN_INFO "");
}*/
