#ifndef __SCHEDULER__
#define __SCHEDULER__

#include <sched.h>
#include <stdbool.h>

#define ITERATION_PER_TIMEUNIT 1000000UL // one unit, one million iterations

/* Data structures */
typedef enum ProcessStatus {
    NOT_STARTED, RUNNING, STOPPED
} ProcessStatus;

typedef enum EventType{
    TIMER_EXPIRED, CHILD_TERMINATED, TIMESLICE_OVER, PROCESS_ARRIVAL
} EventType;

typedef struct ProcessTimeRecord { // For logging
    pid_t pid;
    struct timespec start_time;
} ProcessTimeRecord;

typedef struct ProcessInfo {
    int ready_time;
    int time_needed; // Same as execution time in the problem description i.e. time needed to run the process.
    int remaining_time; // Remaining time for the process; Use in PSJF to determine the process to be run.
    pid_t pid;
    ProcessStatus status;
    char *name; // Not an array; please allocate memory before writing.
} ProcessInfo;

typedef enum scheduleStrategy {
    FIFO, RR, SJF, PSJF
} ScheduleStrategy;

/* Global variables */
extern ScheduleStrategy current_strategy;
extern int num_process;

/* Scheduler functions: should be implemented by each scheduler */
/* The scheduler will be informed that an event has happend via a function call. */

/* Each scheduler should maintain a global data structure to record which processes are being managed.
 * For example, an array of (ProcessInfo *),  a linked list of (ProcessInfo *), or a queue of (ProcessInfo *).
 * You should initialize your data structures when set_strategy() is called.*/
void set_strategy_FIFO();
void set_strategy_RR();
void set_strategy_SJF();
void set_strategy_PSJF();

/* A call to add_process() means that a new process has arrived.
 * Please use my_fork() to fork a new process, record its pid in p->pid, and update your data structure.
 * After forking, send SIGSTOP to the child immediately.
 * Its possible that multiple new processes arrive simultaneously, so don't perform a context switch. */
void add_process_FIFO(ProcessInfo *);
void add_process_RR(ProcessInfo *);
void add_process_SJF(ProcessInfo *);
void add_process_PSJF(ProcessInfo *);

/* A call to remove_process() signals that the current process has ended.
 * Please remove current process from your data structure, but don't perform a context switch. */
void remove_current_process_FIFO(void);
void remove_current_process_RR(void);
void remove_current_process_SJF(void);
void remove_current_process_PSJF(void);
/* A call to timeslice_over() signals that the current time slice has ended,
 * a RR scheduler should update its data structure. */
void timeslice_over_RR(void);

/* The event handler will notify when to context switch.
 * Perform a context switch when, and only when, this function is called.
 * Send SIGSTOP and SIGCONT to appropriate children to perform a context switch. */

void context_switch_FIFO(void);
void context_switch_RR(void);
void context_switch_SJF(void);
void context_switch_PSFJ(void);

/* The event handler may want to know if there are any more jobs in the job pool. */
bool scheduler_empty_FIFO(void);
bool scheduler_empty_RR(void);
bool scheduler_empty_SJF(void);
bool scheduler_empty_PSFJ(void);

/* The loop that should be run by children process */
static inline void run_single_unit(void) {
    volatile unsigned long i;
    for(i = 0; i < ITERATION_PER_TIMEUNIT; i++) {}
}

/* Systemcall wrapper:
 * After forking sys_log_process_start() should be called to record the start time in a ProcessTimeReocrd.
 * After a child terminiates, sys_log_process_end() should be called to write a message to dmesg. */
void sys_log_process_start(ProcessTimeRecord *p);
void sys_log_process_end(ProcessTimeRecord *p);

/* Use this function to spawn a new process instead of using fork(). */
pid_t my_fork();

#endif
