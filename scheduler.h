#ifndef __SCHEDULER__
#define __SCHEDULER__

//This is just the busy work
//add and remove prototypes as needed
//(delete anything that will stay only in main.c)

ScheduleStrategy setStrategy(char strat[]);
void printTime(struct timespec ts);

/* scheduler functions */
static inline void pool_set_strategy(ScheduleStrategy); 
static inline void pool_add_process(ProcessInfo *);
static inline void pool_run_current_process(void);
static inline void pool_remove_current_process(void);
static inline void pool_switch_process(void);

static inline void run_single_unit(void);
static inline void set_my_priority(int priority);
static inline void set_parent_priority(void);
static inline void set_child_priority(void);
pid_t my_fork();
static void sys_log_process_start(ProcessTimeRecord *p);
static void sys_log_process_end(ProcessTimeRecord *p);
static inline void read_single_entry(ProcessInfo *p);
void read_process_info(void);
void priority_test(void);
bool parent_is_terminated(void);
void fork_test(void);
void fork_priority_test(void);
void fork_block_test(void);
void sigalrmtest(int unused);
void fork_signal_test(void);

#endif
