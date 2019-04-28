#ifndef __SCHEDULER__
#define __SCHEDULER__

#include <sched.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include <signal.h>

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
    int arrival_time;
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

/* Scheduler functions: should be implemented by each scheduler */
/* The scheduler will be informed that an event has happend via a function call. */

/* Each scheduler should maintain a global data structure to record which processes are being managed.
 * For example, an array of (ProcessInfo *),  a linked list of (ProcessInfo *), or a queue of (ProcessInfo *).
 * You should initialize your data structures when set_strategy() is called.*/
void set_strategy_FIFO(int num_process);
void set_strategy_RR(int num_process);
void set_strategy_SJF(int num_process);
void set_strategy_PSJF(int num_process);

/* A call to add_process() means that a new process has arrived.  Please update your data structure.
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
 * Use suspend_process and resume_process to perform a context switch. */

void context_switch_FIFO(void);
void context_switch_RR(void);
void context_switch_SJF(void);
void context_switch_PSJF(void);

/* The event handler may want to know if there are any more jobs in the job pool. */
bool scheduler_empty_FIFO(void);
bool scheduler_empty_RR(void);
bool scheduler_empty_SJF(void);
bool scheduler_empty_PSJF(void);

/* The loop that should be run by children process */
static inline void run_single_unit(void) {
    volatile unsigned long i;
    for(i = 0; i < ITERATION_PER_TIMEUNIT; i++) {}
}

typedef struct Heap {
    ProcessInfo **pq;
    int heap_size;
} Heap;

inline void set_priority(pid_t pid, int priority)
{
    struct sched_param kernel_sched_param;
    kernel_sched_param.sched_priority = priority;
    int res = sched_setscheduler(pid, SCHED_FIFO, &kernel_sched_param);
    if (res != 0){
        perror("Can't set scheduler!");
        exit(res);
    }
}

static inline void suspend_process(pid_t pid)
{
    set_priority(pid, sched_get_priority_min(SCHED_FIFO));
    // kill(pid, SIGSTOP);
}
static inline void resume_process(pid_t pid)
{
    set_priority(pid, sched_get_priority_max(SCHED_FIFO)-1);
    // kill(pid, SIGCONT);
}

void heap_insert(Heap *,ProcessInfo *p);
void heap_init(Heap* p, int max_size);
ProcessInfo *heap_top(Heap *);
void heap_pop(Heap *);
int heap_size(Heap *);
bool heap_empty(Heap *);
#endif
