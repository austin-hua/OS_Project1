#include "scheduler.h"
#include <assert.h>
#include <stdlib.h>

#include <sys/types.h>
#include <signal.h>

static Heap pq;
static ProcessInfo *active_process = NULL;
static int last_context_switch_time = 0;
static int current_time = 0;

void add_process_PSJF(ProcessInfo *new_process) {
    current_time = new_process->arrival_time;
    heap_insert(&pq, new_process);
}

void set_strategy_PSJF() {
    heap_init(&pq);
}

void remove_current_process_PSJF(void) {
    assert(active_process == heap_top(&pq));
    int time_passed = active_process->remaining_time;
    current_time += time_passed;
    active_process = NULL;
    heap_pop(&pq);
}

void context_switch_PSJF(void) {
    if(active_process != NULL){
        active_process->remaining_time -= (current_time - last_context_switch_time);
    }

    if (!heap_empty(&pq) && active_process != heap_top(&pq)){
        if (active_process != NULL){
            suspend_process(active_process->pid);
        }
        active_process = heap_top(&pq);
        resume_process(active_process->pid);
    }
    last_context_switch_time = current_time;
}

bool scheduler_empty_PSJF(void) {
     return active_process == NULL && heap_empty(&pq);
}
