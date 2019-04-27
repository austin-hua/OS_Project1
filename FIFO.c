#include "scheduler.h"
#include <signal.h>
#include <stdlib.h>

static int active_process_id = -1;
static int prev_process_id = -1;
static int count_process = 0;

void set_strategy_FIFO() {
}

void add_process_FIFO(ProcessInfo *p) {
    count_process++;
}

void remove_current_process_FIFO(void) {
    count_process--;

}

void context_switch_FIFO(void) {
}

bool scheduler_empty_FIFO(void) {
    return count_process == 0;
}
