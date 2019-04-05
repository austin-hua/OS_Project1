#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <assert.h>

#include <sys/wait.h>
#include <sched.h>

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
    char *name;
} ProcessInfo;

typedef enum scheduleStrategy {
    FIFO, RR, STF, PSTF
} ScheduleStrategy;

ScheduleStrategy setStrategy(char strat[]) {
    if(!strcmp(strat, "RR")) return RR;
    if(!strcmp(strat, "FIFO")) return FIFO;
    if(!strcmp(strat, "STF")) return STF;
    if(!strcmp(strat, "PSTF")) return PSTF;
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

static inline void monopolize_cpu(void)
{
    struct sched_param scheduler_param;
    scheduler_param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    int res = sched_setscheduler(0 /* this process */, SCHED_FIFO, &scheduler_param);
    if (res != 0){
        perror("Can't set scheduler!");
        exit(res);
    }
}

int main() {
    char strat[4];
    scanf("%s", strat);

    ScheduleStrategy S = setStrategy(strat);
    int N; //number of processes
    scanf("%d", &N);

    ProcessInfo P[N];

    for(int i = 0; i < N; i++) {
        scanf("%s", P[i].name);
        scanf("%d%d", &P[i].ready_time, &P[i].execution_time);
        /*
        fork();
        P[i].pid = ;
        */
        P[i].status = NOT_STARTED;
    }
    monopolize_cpu();

    /*for time retrieval when process begins execution*/
    //timespec_get(&P[i].time_record, TIME_UTC);
}
