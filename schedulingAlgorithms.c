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


static inline void round_robin(ProcessInfo *p) {

}

static inline void first_in_first_out(ProcessInfo *p) {

}

static inline void shortest_job_first(ProcessInfo *p) {

}

static inline void preemptive_shortest_job_first(ProcessInfo *p) {

}
