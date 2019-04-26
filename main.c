#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>

#include <unistd.h>
#include <sys/wait.h>

#define PROCESS_NAME_MAX 100
#define BILLION 1000000000L
#define UNIT_MEASURE_REPEAT 1000
#define RR_TIMES_OF_UNIT 500
#define CLOCKID CLOCK_MONOTONIC_RAW

#include "scheduler.h"

typedef struct TimerInfo {
    timer_t timer_id;
    struct timespec time_unit;
    struct timespec arrival_remaining;
    struct timespec timeslice_remaining;
}TimerInfo;

/* Global variables */
ScheduleStrategy current_strategy;
int num_process; // Number of processes s

/* private static variables */
static ProcessInfo *all_process_info;
static ProcessInfo **arrival_queue;
static ProcessInfo **arrival_ptr;
static volatile sig_atomic_t event_type;

/* functions for interaction with scheduler */

void set_strategy(ScheduleStrategy s){
    switch (s){
        case FIFO:
            set_strategy_FIFO();
            break;
        case RR:
            set_strategy_RR();
            break;
        case SJF:
            set_strategy_SJF();
            break;
        case PSJF:
            set_strategy_PSJF();
            break;
    }
}

void add_process(ProcessInfo *p){
    switch (current_strategy){
        case FIFO:
            add_process_FIFO(p);
            break;
        case RR:
            add_process_RR(p);
            break;
        case SJF:
            add_process_SJF(p);
            break;
        case PSJF:
            add_process_PSJF(p);
            break;
    }
}

void remove_current_process(void)
{
    switch (current_strategy){
        case FIFO:
            remove_current_process_FIFO();
            break;
        case RR:
            remove_current_process_RR();
            break;
        case SJF:
            remove_current_process_SJF();
            break;
        case PSJF:
            remove_current_process_PSJF();
            break;
    }
}

void timeslice_over(void)
{
    assert(current_strategy == RR);
    timeslice_over_RR();
}

void context_switch(void)
{
    switch (current_strategy){
        case FIFO:
            context_switch_FIFO();
            break;
        case RR:
            context_switch_RR();
            break;
        case SJF:
            context_switch_SJF();
            break;
        case PSJF:
            context_switch_PSFJ();
            break;
    }
}

static bool scheduler_empty(void)
{
    switch (current_strategy){
        case FIFO:
            return scheduler_empty_FIFO();
        case RR:
            return scheduler_empty_RR();
        case SJF:
            return scheduler_empty_SJF();
        case PSJF:
            return scheduler_empty_PSFJ();
    }
}

pid_t my_fork();

/* For control kernel scheduler */
static void set_my_priority(int priority);
static void set_parent_priority(void);
static void set_child_priority(void);

pid_t my_fork()
{
    pid_t fork_res = fork();
    if (fork_res == 0){
        set_child_priority();
    }
    return fork_res;
}

/* IO fnts */
static void read_process_info();
static ScheduleStrategy str_to_strategy(char strat[]);

/* Qsort compare fnt */
static int sort_by_ready_time(const void *p1, const void *p2);

/* Block some signals */
static sigset_t block_some_signals(void);

/* Costumize signal handlers */
static void signal_handler(int signo)
{
    if(signo == SIGCHLD)
        event_type = CHILD_TERMINATED;
    else if(signo == SIGALRM)
        event_type = TIMER_EXPIRED;
}

static void costumize_signal_handlers(void)
{
    struct sigaction sig_act;
    sig_act.sa_flags = 0;
    sigemptyset(&sig_act.sa_mask);
    sig_act.sa_handler = signal_handler;
    sigaction(SIGALRM, &sig_act, NULL);
    sigaction(SIGCHLD, &sig_act, NULL);
}

static struct timespec timespec_multiply(struct timespec, int);
static struct timespec timespec_divide(struct timespec, int);
static struct timespec timespec_subtract(struct timespec, struct timespec);
static struct timespec measure_time_unit(void);

static void arrival_queue_init(void);
static int timeunits_until_next_arrival(void);
static ProcessInfo *get_arrived_process(void);
static bool arrival_queue_empty(void);

static void update_event_type(TimerInfo *ti)
{
    if(current_strategy != RR) {
        event_type = PROCESS_ARRIVAL;
        return;
    }
    struct timespec dif = timespec_subtract(ti->timeslice_remaining, ti->arrival_remaining);
    if(dif.tv_sec >= 0 && dif.tv_nsec >= 0)
        event_type = PROCESS_ARRIVAL;
    else
        event_type = TIMESLICE_OVER;
}
static void init_arrival_remaining(TimerInfo *ti)
{
    ti->arrival_remaining = timespec_multiply(ti->time_unit, timeunits_until_next_arrival());
}
static void init_timeslice_remaining(TimerInfo *ti)
{
    if(current_strategy != RR) {
        return;
    }
    ti->timeslice_remaining = timespec_multiply(ti->time_unit, RR_TIMES_OF_UNIT);
}
static EventType why_timer_expired_and_subtract_both_remaining(TimerInfo *ti)
{
    if(current_strategy == RR) {
        struct timespec dif = timespec_subtract(ti->arrival_remaining,ti->timeslice_remaining);
        if(dif.tv_sec >= 0 && dif.tv_nsec >= 0) {
            ti->arrival_remaining = timespec_subtract(ti->arrival_remaining, ti->timeslice_remaining);
            ti->timeslice_remaining.tv_sec = ti->timeslice_remaining.tv_nsec = 0;
            return TIMESLICE_OVER;
        }
        else {
            ti->timeslice_remaining = timespec_subtract(ti->timeslice_remaining, ti->arrival_remaining);
            ti->arrival_remaining.tv_sec = ti->arrival_remaining.tv_nsec = 0;
            return PROCESS_ARRIVAL;
        }
    }
    else {
        ti->arrival_remaining.tv_sec = ti->arrival_remaining.tv_nsec = 0;
        return PROCESS_ARRIVAL;
    }
}
static void set_timer(TimerInfo *ti)
{
    struct itimerspec its;
    its.it_interval.tv_sec = its.it_interval.tv_nsec = 0;
    update_event_type(ti);
    if(event_type == TIMESLICE_OVER) {
        its.it_value = ti->timeslice_remaining;
    }
    else if(event_type == PROCESS_ARRIVAL) {
        its.it_value = ti->arrival_remaining;
    }
    int err = timer_settime(ti->timer_id, 0, &its, NULL);
    if(err == -1) {
        printf("timer_settime error!!!\n");
        exit(err);
    }
}

static void create_timer_and_init_timespec(TimerInfo *ti)
{
    // Create the timer
    if(timer_create(CLOCKID, NULL, &ti->timer_id) == -1) {
        printf("timer_create error!!!\n");
        exit(0);
    }

    // Init arrival_remaining and timeslice_remaining
    init_arrival_remaining(ti);
    init_timeslice_remaining(ti);

    // Set the timer
    set_timer(ti);
}

static void assert_nonnegetive_remaining(struct timespec remaining)
{
    assert(remaining.tv_sec >= 0 && remaining.tv_nsec >= 0);
}

static bool remaining_is_zero(struct timespec remaining)
{
    if(remaining.tv_sec == 0 && remaining.tv_nsec == 0)
        return 1;
    else
        return 0;
}

static void update_arrival_remaining(TimerInfo *ti, int time_units)
{
    assert_nonnegetive_remaining(ti->arrival_remaining);
    ti->arrival_remaining = timespec_multiply(ti->time_unit, time_units);
}

static void update_timeslice_remaining(TimerInfo *ti)
{
    if(current_strategy != RR) {
        return;
    }
    assert_nonnegetive_remaining(ti->timeslice_remaining);
    ti->timeslice_remaining = timespec_multiply(ti->time_unit, RR_TIMES_OF_UNIT);
}

int main(void)
{
    char strat[PROCESS_NAME_MAX];
    scanf("%s", strat);
    current_strategy = str_to_strategy(strat);
    set_strategy(current_strategy);

    read_process_info();
    arrival_queue_init();

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
            event_type = why_timer_expired_and_subtract_both_remaining(&timer_info);
            if(event_type == TIMESLICE_OVER) {
                timeslice_over();
                update_timeslice_remaining(&timer_info);
            } else if(event_type == PROCESS_ARRIVAL) {
                add_process(get_arrived_process());
                int arrival_time;
                while((arrival_time = timeunits_until_next_arrival()) == 0) {
                    add_process(get_arrived_process());
                }
                update_arrival_remaining(&timer_info, arrival_time);
            }
            set_timer(&timer_info);
        } else if(event_type == CHILD_TERMINATED) {
            remove_current_process();
        }
        if (arrival_queue_empty() && scheduler_empty()){
            break;
        } else {
            context_switch();
        }
    }
    for(int i = 0; i < num_process; i++){
        printf("%s %d\n", all_process_info[i].name, all_process_info[i].pid);
    }
}

/* IO fnts */
static ScheduleStrategy str_to_strategy(char strat[]) {
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


/* Block some signals */
static sigset_t block_some_signals(void) {
    sigset_t block_set, oldset;
    sigemptyset(&block_set);
    sigaddset(&block_set, SIGCHLD);
    sigaddset(&block_set, SIGALRM);
    sigprocmask(SIG_BLOCK, &block_set, &oldset);
    return oldset;
}

/* Systemcall wrapper */
void sys_log_process_start(ProcessTimeRecord *p)
{
    // Process start time is logged at user space for performance reasons.
    syscall(335, p->pid, &p->start_time);
}

void sys_log_process_end(ProcessTimeRecord *p)
{
    syscall(336, p->pid, &p->start_time);
}

static void read_process_info(void)
{
    scanf("%d", &num_process);

    all_process_info =  (ProcessInfo *) malloc(num_process * sizeof(ProcessInfo));
    for(int i = 0; i < num_process; i++) {
	    read_single_entry(&all_process_info[i]);
    }
}

/* For controlling kernel scheduling */
static void set_my_priority(int priority)
{
    struct sched_param scheduler_param;
    scheduler_param.sched_priority = priority;
    int res = sched_setscheduler(0 /* this process */, SCHED_FIFO, &scheduler_param);
    if (res != 0){
        perror("Can't set scheduler!");
        exit(res);
    }
}

static void set_parent_priority(void)
{
    int priority = sched_get_priority_max(SCHED_FIFO);
    set_my_priority(priority);
}

static void set_child_priority(void)
{
    int priority = sched_get_priority_max(SCHED_FIFO);
    priority -= 1;
    set_my_priority(priority);
}

static bool parent_is_terminated(void)
{
    // The parent terminated so the child is adopted by init (whose pid is 1)
    return getppid() == 1;
}

static struct timespec timespec_multiply(struct timespec timespec, int n)
{
    timespec.tv_sec *= n;
    int64_t temp = timespec.tv_nsec * n; // 64_bit to prevent overflow
    timespec.tv_sec += temp / BILLION;
    timespec.tv_nsec = temp % BILLION;
    return timespec;
}

static struct timespec timespec_divide(struct timespec timespec , int n)
{
    int64_t total_nsec = timespec.tv_sec * BILLION + timespec.tv_nsec;
    total_nsec /= n;
    timespec.tv_sec = total_nsec / BILLION;
    timespec.tv_nsec = total_nsec % BILLION;
    return timespec;
}

static struct timespec timespec_subtract(struct timespec lhs, struct timespec rhs)
{
    lhs.tv_sec -= rhs.tv_sec;
    lhs.tv_nsec -= rhs.tv_nsec;
    if (lhs.tv_nsec < 0){
        lhs.tv_sec -= 1;
        lhs.tv_nsec += BILLION;
    }
    return lhs;
}

static struct timespec measure_time_unit(void)
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

static int processinfo_ptr_cmp(const void * lhs, const void * rhs)
{
    const ProcessInfo * const * lhs_p = lhs;
    const ProcessInfo * const * rhs_p = rhs;
    return  (**lhs_p).ready_time - (**rhs_p).ready_time;
}

static void arrival_queue_init(void)
{
    arrival_queue = (ProcessInfo **) malloc(sizeof(ProcessInfo *) * num_process);
    for(int i = 0; i < num_process; i++){
        arrival_queue[i] = all_process_info + i;
    }
    qsort(arrival_queue, num_process, sizeof(ProcessInfo *),
            processinfo_ptr_cmp);
    arrival_ptr = arrival_queue;
}

static int timeunits_until_next_arrival(void)
{
    assert(!arrival_queue_empty());
    if (arrival_ptr == &arrival_queue[0]){
        return (*arrival_ptr)->ready_time;
    }
    ProcessInfo **prev_process = arrival_ptr - 1;
    return (*arrival_ptr)->ready_time - (*prev_process)->ready_time;
}

static ProcessInfo *get_arrived_process(void)
{
    ProcessInfo *ret = *arrival_ptr;
    arrival_ptr++;
    return ret;
}

static bool arrival_queue_empty(void)
{
    return arrival_ptr == arrival_queue + num_process;
}

/* The following functions are for testing */
static void priority_test(void)
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
     * Expected behavior: The program prints "The child begins running!",
     * freezes for 1 second, prints an "awoken by a signal" message,
     * then keeps freezing for a few seconds, and then prints a message.
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
        printf("The child begins running!\n");
        for(volatile unsigned long  i = 0; i < 500; i++){
            run_single_unit();
        }
        printf("IF the parent has been awoken by a signal after the child began running, then success, else error.\n");
    }
}
