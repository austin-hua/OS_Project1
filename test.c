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

void double_signal_test()
{
    /* This function sends SIGSTOP and SIGCONT simultaneously to child, to see what happends.
     */
    set_parent_priority();
    pid_t child_pid = my_fork();
    if (child_pid == 0){
        printf("I'm running!\n");
    } else {
        kill(child_pid, SIGSTOP);
        printf("SIGSTOP sent\n");
        kill(child_pid, SIGCONT);
        printf("SIGCONT sent\n");
        sleep(1);
    }
}

int main(void)
{
    double_signal_test();
}
