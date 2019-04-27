#include "scheduler.h"
#include <stdlib.h>

#include <sys/types.h>
#include <signal.h>

static Heap inactive_heap;
static ProcessInfo *active_process = NULL;

void add_process_SJF(ProcessInfo *new_process) {
     heap_insert(&inactive_heap, new_process);
}

void set_strategy_SJF() {
     heap_init(&inactive_heap);
}

void remove_current_process_SJF(void) {
     active_process = NULL;
}

void context_switch_SJF(void) {
     if (active_process != NULL){
          return; // No preemption in SJF
     }
     active_process = heap_top(&inactive_heap);
     heap_pop(&inactive_heap);
     kill(active_process->pid, SIGCONT);
}

bool scheduler_empty_SJF(void) {
     return heap_size(&inactive_heap) == 0 && active_process == NULL;
}
