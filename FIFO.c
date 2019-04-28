#include "scheduler.h"

static int count_process = 0;

void set_strategy_FIFO(int unused) {
}

void add_process_FIFO(ProcessInfo *p) {
    resume_process(p->pid);
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
