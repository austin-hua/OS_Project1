#include <stdlib.h>
#include <string.h>
#include "scheduler.h"

static ProcessInfo **pq;
static int current_process_id = -1; // The index of the process in pq that the scheduler runs when context_switch_RR() is called.
static int previous_active_id = -1; // The index of the process in pq that the scheduler stops when context_switch_RR() is called.
static int process_count = 0;

static int advance(int n) {
    if (n < 0) {
        return n;
    }
    n++;
    if (n == process_count) {
        n = 0;
    }
    return n;
}

void set_strategy_RR(void) {
    pq = (ProcessInfo **) malloc(sizeof(ProcessInfo *) * num_process;    
}

                           
void add_process_RR(ProcessInfo *p) {
    if (process_count == 0){
        assert(previous_active_id < 0);
        current_process_id = 0;
        pq[current_process_id] = p;
    } else {
        memmove(pq + current_process_id + 1, 
                pq + current_process_id, 
                process_count - current_process_id);
        pq[current_process_id] = p;
        current_process_id++;
    }
    process_count++;
}

void remove_current_process_RR(void) {
    if (current_process_id + 1 != process_count){ // so pq + current_process_id + 1 is a valid memory location
        memmove(pq + current_process_id ,
                pq + current_process_id + 1,
                process_count - (current_process_id + 1));
    }
    previous_active_id = -1;
    process_count--;
    if (process_count == 0){
        current_process_id = -1;
    } else if (current_process_id == process_count){
        current_process_id = 0;
    }
}

void timeslice_over_RR(void) {
    previous_active_id = current_process_id;
    current_process_id = advance(current_process_id);
}

void context_switch_RR(void) {
    if (previous_active_id >= 0){
        kill(pq[previous_active_id]->pid, SIGSTOP);
    }
    if (current_process_id >= 0){
        kill(pq[current_process_id]->pid, SIGCONT);
    }
}

void scheduler_empty_RR(void) {
    return process_count == 0 && current_process_id < 0;
}
