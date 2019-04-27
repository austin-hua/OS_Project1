#include "scheduler.h"
#include <assert.h>
#include <stdlib.h>

#include <sys/types.h>
#include <signal.h>

static Heap pq;
static ProcessInfo *activeProcess = NULL;
static int last_context_switch_time = 0;
static int current_time = 0;

void add_process_PSJF(ProcessInfo *new_process) {
    current_time = new_process->arrival_time;
    heap_insert(&pq, new_process);
}

void set_strategy_PSJF() {
    heap_init(&pq);
}

void remove_curr_process_PSJF(void) {
    assert(active_process == heap_top(&pq));
    int time_passed = active_process->remaining_time;
    current_time += time_passed;
    heap_pop(&pq);
    active_process = NULL;
}

void context_switch_PSJF(void) {
    if(active_process != NULL){
        active_process->remaining_time -= (current_time - last_context_switch_time);
    }
    if (active_process != heap_top(&pq){
        kill(active_process->pid, SIGSTOP);
        active_process = heap_top(&pq);
        kill(active_process, SIGCONT);
    }
    last_context_switch_time = current_time;
}

bool scheduler_empty_PSJF(void) {
     return heap_size(&inactive_heap) == 0 && active_process == NULL;
}
