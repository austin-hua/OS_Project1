#include <stdlib.h>
#include "scheduler.h"

static ProcessInfo **pq;
int num_process;

static ProcessInfo *activeProcess;

void set_strategy_RR(void) {
    pq = (ProcessInfo **) malloc(sizeof(ProcessInfo *) * num_process;
    num_process = 0;
    activeProcess = NULL;
}

void add_process_RR(void) {
    
}

void remove_current_process_RR(void) {
    activeProcess = NULL;
}

void timeslice_over_RR(void) {
    
}

void context_switch_RR(void) {
    if(activeProcess != NULL) {
        
    }
}

void scheduler_empty_RR(void) {
    return activeProcess == NULL && num_process == 0;
}
