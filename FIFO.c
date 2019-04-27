#include "scheduler.h"
#include <signal.h>
#include <stdlib.h>

static ProcessInfo **pq;
static int active_process_id = -1;
static int prev_process_id = -1;
static int count_process = 0;

void set_strategy_FIFO() {
	
}

void add_process_FIFO(ProcessInfo *) {
	pq = (ProcessInfo **) malloc(sizeof(ProcessInfo *) * num_process);
}

void remove_current_process_RR(void) {
	
}

void context_switch_FIFO(void) {
	
}

bool scheduler_empty_FIFO(void) {
	
}
