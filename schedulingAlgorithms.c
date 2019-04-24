#include "scheduler.h"

/*
    ProcessInfo contents
    int ready_time;
    int time_needed; // Same as execution time in the problem description i.e. time needed to run the process.
    int remaining_time; // Remaining time for the process; Use in PSJF to determine the process to be run.
    pid_t pid;
    ProcessStatus status;
    char *name; // Not an array; please allocate memory before writing.


*/

static inline void schedule(ScheduleStrategy S, ProcessInfo *all_process_info, int num_process) {
    switch(S) {
        case FIFO:
            first_in_first_out(all_process_info, num_process);
            break;
        case RR:
            round_robin(all_process_info, num_process);
            break;
        case SJF:
            shortest_job_first(all_process_info, num_process);
            break;
        case PSJF:
            preemptive_shortest_job_first(all_process_info, num_process);
            break;
    }
}

//for debugging
void printTime(struct timespec ts) {
    char buff[100];
    strftime(buff, sizeof buff, "%D %T", gmtime(&ts.tv_sec));
    printf("%s.%09ld UTC\n", buff, ts.tv_nsec);
}

static inline void round_robin(ProcessInfo *all_process_info, int num_process) {
    //switch out processes every 500 time units
    int active_process = 0;
    for(int i = 0; i < num_process; i++) {
        int unitsPassed = 0;
        while(all_process_info[i]->remaining_time >= 0 && unitsPassed < 500) {
            unitsPassed++;
            run_single_unit;
            all_process_info[i]->remaining_time--; //optimise: check if 500 can be subtracted first, subtract all at once
        }
    }
    //run_single_unit
}

static inline void first_in_first_out(ProcessInfo *all_process_info, int num_process) {
    
}

static inline void shortest_job_first(ProcessInfo *all_process_info, int num_process) {
    //sort indeces based on which task is shortest
    int priority[num_process];
    
    for(int i = 0; i < num_process-1; i++) { //basic selection sort
        int shortest = all_process_info[i]->time_needed;
        for(int j = i+1; j < num_process; j++) {
            if(all_process_info[j]->time_needed < all_process_info[shortest]->time_needed) {
                
            }
        }
    }
    
    
    
    
    
}

static inline void preemptive_shortest_job_first(ProcessInfo *all_process_info, int num_process) {
    
}
