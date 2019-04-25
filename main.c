#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <assert.h>
#include <errno.h>
#include <stdint.h>

#include <unistd.h>
#include <sys/wait.h>
#include <sched.h>

#define PROCESS_NAME_MAX 100
#define UNIT 1000000UL // one unit, one million iterations
#define BILLION 1000000000L
#define UNIT_MEASURE_REPEAT 1000
#define RR_TIMES_OF_UNIT 500
#define CLOCKID CLOCK_MONOTONIC_RAW

/* Data structures */
typedef enum ProcessStatus {
    NOT_STARTED, RUNNING, STOPPED
} ProcessStatus;

typedef enum EventType{
    TIMER_EXPIRED, CHILD_TERMINATED
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

typedef struct TimerInfo {
    timer_t timer_id;
    int process_arrival;
    int timeslice_over;
    struct timespec time_unit;
    struct timespec arrival_remaining;
    struct timespec timeslice_remaining;
}TimerInfo;

/* Global variables */
ScheduleStrategy current_strategy;
ProcessInfo *all_process_info;
ProcessInfo *next_process;
int num_process; // Number of processes s
volatile sig_atomic_t event_type;

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

/* For control kernel scheduler */
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

/* Systemcall wrapper */
static void sys_log_process_start(ProcessTimeRecord *p);
static void sys_log_process_end(ProcessTimeRecord *p);

/* IO fnts */
static void read_process_info();
ScheduleStrategy str_to_strategy(char strat[]);

/* Qsort compare fnt */
int sort_by_ready_time(const void *p1, const void *p2);

/* Block some signals */
sigset_t block_some_signals(void);

/* Costumize signal handlers */
void signal_handler(int signo)
{
    if(signo == SIGCHLD)
        event_type = CHILD_TERMINATED;
    else if(signo == SIGALRM)
        event_type = TIMER_EXPIRED;
}

void costumize_signal_handlers(void)
{
    struct sigaction sig_act;
    sig_act.sa_flags = 0;
    sigemptyset(&sig_act.sa_mask);
    sig_act.sa_handler = signal_handler;
    sigaction(SIGALRM, &sig_act, NULL);
    sigaction(SIGCHLD, &sig_act, NULL);
}

/* Functions that are only useful to the main() or the event handler */
struct timespec timespec_multiply(struct timespec, int);
struct timespec timespec_divide(struct timespec, int);
struct timespec timespec_subtract(struct timespec, struct timespec);
struct timespec measure_time_unit(void);

int timeunits_until_next_arrival(void);
ProcessInfo *get_next_arrived_process(void);
bool arrival_queue_empty(void);

void init_arrival_remaining(TimerInfo *ti)
{
    ti->process_arrival = 0;
    ti->arrival_remaining = timespec_multiply(ti->time_unit, timeunits_until_next_arrival());
}
void init_timeslice_remaining(TimerInfo *ti)
{
    ti->timeslice_over = 0;
    if(current_strategy != RR) {
        return;
    }
    ti->timeslice_remaining = timespec_multiply(ti->time_unit, RR_TIMES_OF_UNIT);
}
struct timespec shorter_remaining(TimerInfo *ti)
{
    if(current_strategy == RR) {
        struct timespec dif = timespec_subtract(ti->arrival_remaining,ti->timeslice_remaining);
        if(dif.tv_sec >= 0 && dif.tv_nsec >= 0)
            return ti->timeslice_remaining;
        else
            return ti->arrival_remaining;
    } else {
        return ti->arrival_remaining;
    }
}
void set_timer(TimerInfo *ti) {
    struct timespec min = shorter_remaining(ti);
    struct itimerspec its;
    its.it_interval = its.it_value = min;
    int ret = timer_settime(ti->timer_id, 0, &its, NULL);
    if(ret == -1) {
        printf("timer_settime error!!!\n");
        exit(ret);
    }
}

void create_timer_and_init_timespec(TimerInfo *ti)
{
    // Create the timer
    struct sigevent sev;
    
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGALRM;
    if(timer_create(CLOCKID, &sev, &ti->timer_id) == -1) {
        printf("timer_create error!!!\n");
        exit(0);
    }

    // Init arrival_remaining and timeslice_remaining
    init_arrival_remaining(ti);
    init_timeslice_remaining(ti);

    // Set the timer
    set_timer(ti);
}

void check_timespec(TimerInfo *ti)
{
    if(is_zero(ti->timeslice_remaining))
        ti->timeslice_over = 1;
    else if(is_zero(ti->arrival_remaining))
        ti->process_arrival = 1;
    else {
        printf("check_timespec fnt error!!!\n");
        exit(0);
    }
}

void update_timespec_and_set_timer(TimerInfo *ti)
{
    // Update timespec
    struct timespec min = shorter_remaining(ti);
    update_arrival_remaining(min,ti);
    update_timeslice_remaining(min,ti);

    // Set the timer
    set_timer(ti);
}

int main(void)
{
    char strat[4];
    scanf("%s", strat);
    current_strategy = str_to_strategy(strat);
    set_strategy(current_strategy);

    read_process_info();
    qsort(all_process_info, num_process, sizeof(ProcessInfo), sort_by_ready_time);
    
    /* Signal handling */
    sigset_t oldset = block_some_signals();
    costumize_signal_handlers();

    /* Create the timer */
    TimerInfo timer_info;
    timer_info.time_unit = measure_time_unit();
    create_timer_and_init_timespec(&timer_info);

    while (true){
        sigsuspend(&oldset); 
        if(event_type == TIMER_EXPIRED) {
            check_timespec(&timer_info);
            if(timer_info.timeslice_over == 1) {
                switch_process();
            }
            else if(timer_info.process_arrival == 1) {
                while(timeunits_until_next_arrival() == 0) {
                    ProcessInfo *p = get_next_arrived_process();
                    add_process(p);
                }
            }
            // update(?)
            update_timespec_and_set_timer(&timer_info);
        }
        else if(event_type == CHILD_TERMINATED) {
            remove_current_process();
        }
        if (arrival_queue_empty() && scheduler_empty()){
            break;
        }
    }
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

/* Qsort compare fnt */
int sort_by_ready_time(const void *p1, const void *p2) {
    ProcessInfo *p[2];
    p[0] = (ProcessInfo *)p1;
    p[1] = (ProcessInfo *)p2;
    return (p[0]->ready_time > p[1]->ready_time);
}

/* Block some signals */
sigset_t block_some_signals(void) {
    sigset_t block_set, oldset;
    sigemptyset(&block_set);
    sigaddset(&block_set, SIGCHLD);
    sigaddset(&block_set, SIGALRM);
    sigprocmask(SIG_BLOCK, &block_set, &oldset);
    return oldset;
}

/* Systemcall wrapper */
static void sys_log_process_start(ProcessTimeRecord *p)
{
    // Process start time is logged at user space for performance reasons.
    syscall(335, p->pid, &p->start_time);
}

static void sys_log_process_end(ProcessTimeRecord *p)
{
    syscall(336, p->pid, &p->start_time);
}

void read_process_info(void)
{
    scanf("%d", &num_process);

    all_process_info =  (ProcessInfo *) malloc(num_process * sizeof(ProcessInfo));
    for(int i = 0; i < num_process; i++) {
	    read_single_entry(&all_process_info[i]);
    }
    next_process = all_process_info;
}

/* For controlling kernel scheduling */
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

struct timespec timespec_multiply(struct timespec timespec, int n)
{
    timespec.tv_sec *= n;
    int64_t temp = timespec.tv_nsec * n; // 64_bit to prevent overflow
    timespec.tv_sec += temp / BILLION;
    timespec.tv_nsec = temp % BILLION;
    return timespec;
}

struct timespec timespec_divide(struct timespec timespec , int n)
{
    int64_t total_nsec = timespec.tv_sec * BILLION + timespec.tv_nsec;
    total_nsec /= n;
    timespec.tv_sec = total_nsec / BILLION;
    timespec.tv_nsec = total_nsec % BILLION;
    return timespec;
}

struct timespec timespec_subtract(struct timespec lhs, struct timespec rhs)
{
    int64_t total_nsec = lhs.tv_nsec - rhs.tv_nsec;
    total_nsec += (lhs.tv_sec - rhs.tv_sec) * BILLION;
    struct timespec ret = {.tv_sec = total_nsec / BILLION, .tv_nsec = total_nsec % BILLION};
    return ret;
}

struct timespec measure_time_unit(void)
{
    struct timespec begin, end;
    clock_gettime(CLOCKID, &begin);
    for(int i = 0; i < UNIT_MEASURE_REPEAT; i++){
        run_single_unit();
    }
    clock_gettime(CLOCKID, &end);
    struct timespec res = timespec_subtract(end, begin);
    return timespec_divide(res, UNIT_MEASURE_REPEAT);
}

int timeunits_until_next_arrival(void)
{
    assert(!arrival_queue_empty());
    if (next_process == all_process_info){
        return next_process->ready_time;
    }
    ProcessInfo *prev_process = next_process - 1;
    return next_process->ready_time - prev_process->ready_time;
}

ProcessInfo *get_next_arrived_process(void)
{
    ProcessInfo *ret = next_process;
    next_process++;
    return ret;
}

bool arrival_queue_empty(void)
{
    return next_process == all_process_info + num_process;
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
