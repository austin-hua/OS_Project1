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
/* one unit, one million iterations */
#define UNIT 1000000UL

/* necessary data structures */
typedef enum ProcessStatus { 
    NOT_STARTED, RUNNING, STOPPED 
} ProcessStatus;

typedef struct ProcessInfo {
    struct timespec time_record;
    int ready_time;
    int execution_time;
    pid_t pid;
    ProcessStatus status;
    char *name; // Not an array; please allocate memory before writing.
} ProcessInfo;

typedef enum scheduleStrategy {
    FIFO, RR, SJF, PSJF
} ScheduleStrategy;

ScheduleStrategy setStrategy(char strat[]) {
    if(!strcmp(strat, "RR")) return RR;
    if(!strcmp(strat, "FIFO")) return FIFO;
    if(!strcmp(strat, "SJF")) return SJF;
    if(!strcmp(strat, "PSJF")) return PSJF;
    assert(0);
}

//for debugging
void printTime(struct timespec ts) {
    char buff[100];
    strftime(buff, sizeof buff, "%D %T", gmtime(&ts.tv_sec));
    printf("%s.%09ld UTC\n", buff, ts.tv_nsec);
}

/* scheduler functions */
static inline void pool_set_strategy(ScheduleStrategy); 
static inline void pool_add_process(ProcessInfo *);
static inline void pool_run_current_process(void);
static inline void pool_remove_current_process(void); // called when current process ends
static inline void pool_switch_process(void); // for RR. Please use sigstop/sigcont.

/* The loop that should be run by children process */
static inline void run_single_unit(void) {
    volatile unsigned long i;
    for(i = 0; i < UNIT; i++) {}
}

/* for monopolizing cpu */
static inline void set_my_priority(int priority)
{
    struct sched_param scheduler_param;
    scheduler_param.sched_priority = priority;
    int res = sched_setscheduler(0 /* this process */, SCHED_FIFO, &scheduler_param);
    if (res != 0){
        perror("Can't set scheduler!");
        exit(res);
    }
}
static inline void set_parent_priority(void)
{
    int priority = sched_get_priority_max(SCHED_FIFO);
    set_my_priority(priority);
}

static inline void set_child_priority(void)
{
    int priority = sched_get_priority_max(SCHED_FIFO);
    priority -= 1;
    set_my_priority(priority);
}

pid_t my_fork()
{
    pid_t fork_res = fork();
    if (fork_res == 0){
        set_child_priority();
    }
    return fork_res;
}

/* systemcall wrapper */

void sys_log_process_start(pid_t pid, struct timespec *start_time)
{
    // This function writes current time to start_time. 
    // Does not do logging, but I can't come up with a better name.
    // This is logged at user space for performance reasons.
    syscall(335, pid, start_time);
}

void sys_log_process_end(pid_t pid, const struct timespec *start_time)
{
    // This function writes start_time and current time into kernel logs.
    syscall(336, pid, start_time);
}

static inline void read_single_entry(ProcessInfo *p)
{
    char *process_name = (char *)malloc(sizeof(char) * PROCESS_NAME_MAX);
    scanf("%s", process_name);
    p->name = process_name;
    scanf("%d%d", &p->ready_time, &p->execution_time);
    p->status = NOT_STARTED;
}

int main(void) {
    char strat[4];
    scanf("%s", strat);

    ScheduleStrategy S = setStrategy(strat);
    int N; //number of processes
    scanf("%d", &N);

    ProcessInfo P[N];

    for(int i = 0; i < N; i++) {
        read_single_entry(&P[i]);
    }

    /*for time retrieval when process begins execution*/
    //timespec_get(&P[i].time_record, TIME_UTC);
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
