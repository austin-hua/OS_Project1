#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <assert.h>
#include <errno.h>

#include <unistd.h>
#include <sys/wait.h>
#include <sched.h>

#define PROCESS_NAME_MAX 100
#define UNIT 1000000UL // one unit, one million iterations

/* data structures */
typedef enum ProcessStatus { 
    NOT_STARTED, RUNNING, STOPPED 
} ProcessStatus;

typedef struct ProcessTimeRecord { // for logging
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

/* Scheduler functions: should be implemented by each scheduler */
/* The scheduler will be informed that an event has happend via a function call. */
/* Please use SIGSTOP and SIGCONT to perform a context switch. */ 

/* Each scheduler should maintain a global data structure to record which processes are being managed.
 * For example, an array of (ProcessInfo *),  a linked list of (ProcessInfo *), or a queue of (ProcessInfo *). 
 * You should initialize your data structures when set_strategy() is called.*/
static  void set_strategy(ScheduleStrategy);

/* A call to add_process() means that a new process has arrived.
 * Please use my_fork() to fork a new process, and record its pid in p->pid.
 * Be mindful that you may have to perform a context switch, or send a SIGSTOP to the newly forked process. */
static  void add_process(ProcessInfo *p); 

/* A call to remove_process() signals that the current process has ended. 
 * Please remove current process from your data structure, and context sitch to an appropriate child. */
static  void remove_current_process(void); // called when current process ends 

/* A call to switch_process() signals that the current time slice has ended, 
    and a RR scheduler should perform a context switch. */
static  void switch_process(void); // for RR. Please use sigstop/sigcont.

/* The event handler may want to know if there are any more jobs in the job pool. */
static bool scheduler_empty(void);

/* The loop that should be run by children process */
static  void run_single_unit(void) {
    volatile unsigned long i;
    for(i = 0; i < UNIT; i++) {}
}

/* for control kernel scheduler */
static  void set_my_priority(int priority);
static  void set_parent_priority(void);
static  void set_child_priority(void);

pid_t my_fork()
{
    pid_t fork_res = fork();
    if (fork_res == 0){
        set_child_priority();
    }
    return fork_res;
}

/* systemcall wrapper */
static void sys_log_process_start(ProcessTimeRecord *p)
{
    // process start time is logged at user space for performance reasons.
    syscall(335, p->pid, &p->start_time);
}

static void sys_log_process_end(ProcessTimeRecord *p)
{
    syscall(336, p->pid, &p->start_time);
}

/* IO fnts */
static void read_process_info();
ScheduleStrategy str_to_strategy(char strat[]);

/* global variables */
ProcessInfo *all_process_info;
int num_process; // number of processes s
ScheduleStrategy current_strategy;

// functions that are only useful to the main() or the event handler:
struct timespec timespec_multiply(struct timespec, int);
struct timespec timespec_devide(struct timespec, int);
struct timespec measure_time_unit(void);

int timeunits_until_next_arrival(void);
ProcessInfo *get_next_arrvied_process(void);
bool arrival_queue_empty(void);

int main(void)
{
    char strat[4];
    scanf("%s", strat);

    ScheduleStrategy S = str_to_strategy(strat);

    read_process_info(); 

    set_strategy(S);

    /*for time retrieval when process begins execution*/
    //timespec_get(&P[i].time_record, TIME_UTC);
}

/* IO fnts */
ScheduleStrategy str_to_strategy(char strat[]) {
    if(!strcmp(strat, "RR")) return RR;
    if(!strcmp(strat, "FIFO")) return FIFO;
    if(!strcmp(strat, "SJF")) return SJF;
    if(!strcmp(strat, "PSJF")) return PSJF;
    assert(0);
}

static void read_single_entry(ProcessInfo *p)
{
    char *process_name = (char *)malloc(sizeof(char) * PROCESS_NAME_MAX);
    scanf("%s", process_name);
    p->name = process_name;
    scanf("%d%d", &p->ready_time, &p->time_needed);
    p->remaining_time = p->time_needed;
    p->status = NOT_STARTED;
}

void read_process_info(void)
{ 
    scanf("%d", &num_process);

    all_process_info =  (ProcessInfo *) malloc(num_process * sizeof(ProcessInfo));
    for(int i = 0; i < num_process; i++) {
	    read_single_entry(&all_process_info[i]);
    }

    return;
}

/* for controlling kernel scheduling */
static  void set_my_priority(int priority)
{
    struct sched_param scheduler_param;
    scheduler_param.sched_priority = priority;
    int res = sched_setscheduler(0 /* this process */, SCHED_FIFO, &scheduler_param);
    if (res != 0){
        perror("Can't set scheduler!");
        exit(res);
    }
}

static  void set_parent_priority(void)
{
    int priority = sched_get_priority_max(SCHED_FIFO);
    set_my_priority(priority);
}

static  void set_child_priority(void)
{
    int priority = sched_get_priority_max(SCHED_FIFO);
    priority -= 1;
    set_my_priority(priority);
}

/* The following functions are for testing */
void priority_test(void)
{
    /* Tests whether this process has priority over all other processes on this machine.
     * Expected behavior:
     * If run as root, the program should freeze for several seconds.
     * Otherwise, the program prints an error message. */
    set_parent_priority();
    for(int i = 0; i < 1000; i++){
        run_single_unit();
    }
}

bool parent_is_terminated(void)
{
    // The parent terminated so the child is adopted by init (whose pid is 1)
    return getppid() == 1;
}

void fork_test(void)
{
    /* Tests if the children runs only if the parent has done all its stuff.
     * Expected behavior:
     * If run as root, the program should freeze for several seconds,
     * then prints "Success"
     */
    set_parent_priority();
    pid_t pid = my_fork();
    if (pid != 0) { /* parent, who should have priority */
        for(int i = 0; i < 1000; i++){
            run_single_unit();
        }
    } else { /* Child, who should run after the parent terminates. */
        if (parent_is_terminated()){
            printf("Success\n");
        } else {
            printf("Error: the child runs before the parent terminates.\n");
        }
    }
}

void fork_priority_test(void)
{
    /* Tests if the parent has priority over its children,
     * even if the parent tries to give up its time slice.
     * Expected behavior:
     * If run as root, the program should freeze for several seconds,
     * then prints "Success"
     */
    set_parent_priority();
    pid_t pid = my_fork();
    if (pid != 0) {
        sched_yield();
        for(int i = 0; i < 1000; i++){
            run_single_unit();
        }
    } else {
        if (parent_is_terminated()){
            printf("Success\n");
        } else {
            printf("Error: the child runs before the parent terminates.\n");
        }
    }
}

void fork_block_test(void)
{
    /* Tests if the parent is blocked, then the children can gets its priority.
     * Expected behavior: If run as root, then a message is printed
     * before the program freezes for 10 seconds.
     */
    set_parent_priority();
    pid_t pid = my_fork();
    if (pid != 0) {
        sleep(10);
    } else {
        if(parent_is_terminated()){
            printf("Error: The child runs after the parent terminates");
        } else {
            printf("Success\n");
        }
    }
}

void sigalrmtest(int unused)
{
}

void fork_signal_test(void)
{
    /* Tests if the parent can receive signal even when a child is running 
     * Expected behavior: The program freezes for 1 second, prints an "awoken by a signal" message,
     * then keeps freezing for a few seconds, then prints a message.
     */

    set_parent_priority();
    signal(SIGALRM, sigalrmtest);
    // block sigalrm
    sigset_t old_mask, sigalrm_mask;
    sigemptyset(&sigalrm_mask);
    sigaddset(&sigalrm_mask, SIGALRM);
    sigprocmask(SIG_BLOCK, &sigalrm_mask, &old_mask);
    alarm(1);
    pid_t pid = my_fork();
    if (pid != 0){
        // unblock siglarm
        sigsuspend(&old_mask);
        printf("The parent is awoken by a signal!\n");
        wait(NULL);
    } else {
        for(volatile unsigned long  i = 0; i < 500; i++){
            run_single_unit();
        }
        printf("IF the parent has been awoken by a signal, then success, else error.\n");
    }
}
