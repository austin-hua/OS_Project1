#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/wait.h>

/* necessary data structures */
typedef enum ProcessStatus { 
    NOT_STARTED, RUNNING, STOPPED 
} ProcessStatus;

typedef struct ProcessInfo{
    struct timespec time_record;
    int ready_time;
    int execution_time;
    pid_t pid; 
    ProcessStatus status;
    char *name;
} ProcessInfo;


typedef enum scheduleStrategy{
    FIFO, RR, STF, PSTF
} ScheduleStrategy;

/* schedular functions */
static inline void pool_set_strategy(ScheduleStrategy); 
static inline void pool_add_process(ProcessInfo *);
static inline void pool_run_current_process(void);
static inline void pool_remove_current_process(void); // called when current process ends
static inline void pool_switch_process(void); // for RR. Please use sigstop/sigcont.

/* The loop that should be run by children process */
static inline void run_single_unit(void)
{
    volatile unsigned long i;
    for(i = 0; i < 1000000UL; i++){
    }
} 

int main()
{
}
