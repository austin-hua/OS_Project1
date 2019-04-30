#include "scheduler.h"
#include <assert.h>
#include <stdlib.h>

#include <sys/types.h>
#include <signal.h>

static Heap pq;
static ProcessInfo *active_process = NULL;
static int last_context_switch_time = 0;
static int current_time = 0;

/* Invariant: the top of heap should be always the currently running process
 * after context_switch_PSJF() is called.
 */

void add_process_PSJF(ProcessInfo *new_process) {
    assert(active_process == heap_top(&pq));
    current_time = new_process->arrival_time;
    active_process->remaining_time -= (current_time - last_context_switch_time);
    // active_process doesn't need to be popped as deducting its remaining time 
    // does not require updating the heap.
    heap_insert(&pq, new_process);
}

void set_strategy_PSJF(int num_process) {
    heap_init(&pq, num_process);
}

void remove_current_process_PSJF(void) {
    assert(active_process == heap_top(&pq));
    int time_passed = active_process->remaining_time;
    current_time += time_passed;
    active_process = NULL;
    heap_pop(&pq);
}

void context_switch_PSJF(void) {
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
