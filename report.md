# Report

Group members: B06902101, B06902098	B05902099	B06902051	B05902132	B06902086

### About Our Design
Our scheduler is in user space, so we need help from the Linux scheduler. The Linux scheduler provides a FIFO policy, processes under which has precedence over normal processes. Naturally, our scheduler and its children run under this policy, so other processes on the computer can't interfere with our processes. Processes have different priorities under FIFO policy, and those with a lower priority can't run if other processes with higher priorities are ready to run. We exploit this property to perform a context switch. Our main scheduler runs the process with the highest priority first and also makes sure there's always only one child (assuming the scheduler has at least one child) as the second highest priority, and the other children are in the lowest priority. When the scheduler blocks, the child with the highest priority gets the chance to run.

Our program can be split into three parts. First part is responsible for initialization, the next part is responsible for event handling, and the last part is responsible for job scheduling. In the initailization part, the program runs for 1000 time units to determine the actual time for each time unit. It also sets up the signal handler and blocks SIGALRM and SIGCHLD for later use. After the initialization is the event handling part. It watches for events such as the arrival of new processes, time slice ending, or old process terminating. The first two events are handled using timers. It could be done by using two timers, but we chose to simulate two timers with one timer to practice our coding skills. All events generate either SIGALRM or SIGCHLD, so a loop running sigsuspend() is an obvious implementation choice. As SIGALRM and SIGCHLD are only unblocked in sigsuspend(), we make sure that there are no race conditions in the event handler. After receiving an signal, the event handler not only notifies the job scheduler, but also does some necessary work, such as resetting the timer or calling wait(). After that, the event handler checks whether there are incoming children or currently running children. If not, the event handler breaks from the loop.

The event handler notify an event to the job scheduler by calling add_process(), remove_current_process(), or timeslice_over(). Inside those functions is a switch statement that calls add_process_{policy}() for each possible policy. Since there may be multiple processes being added, the scheduler can't perform a context switch immediately after add_process(), or any other function, is called. It performs a context switch only when context_switch_{policy}() is called. Another interface of the job scheduler is scheduler_empty_{policy}(), which is used by the event handler to know whether there are processes still being scheduled.

The FIFO scheduler does nearly nothing, as the scheduler and its children are already running under the kernel FIFO scheduler. All it does is remember how many children are running and tells the event handler whether there's still any child waiting to be scheduled.

The RR scheduler stores every job in an array, and uses two integers, _current_process_id_ and _previous_active_id_ to mark two locations in the array. When the event handler requests a context switch, it stops the process in _previous_active_id_ and starts the process in _current_process_id_. It's designed this way since the scheduler has no way to know which event happens before context_switch_RR() is called, so it can't make any assumption on which process to stop. Whenever a new job enters, the new job is inserted before _current_process_id_, and _current_process_id_ is modified to make sure it points to the same job. Whenever an old job exits, it's removed from the array, _current_process_id_ is moved to point the next process, and _prveious_active_id_ is modified to point to nothing. Whenever a timeslice ends, _previous_active_id_ points to what _current_process_id_ points, and _current_process_id_ proceeds to point the enxt entry.  

Both SJF scheduler and PSJF scheduler uses heap to manage jobs. The heap compares the remaining time of each job. The top entry of the heap is the job with shortest remaining time. In the SJF scheduler, the active job may not be the job with shortest remaining time, so it's not stored in the heap, but stored in a seperate pointer. Jobs are popped out of the heap only when it's their term to execute, and they are never pushed back. As a result, the remaining time of jobs in the heap is the same as the time needed to run the process, and the scheduler can share the same heap implementation with the PSJF scheduler. When a new job enters, it's pushed into the heap. When the currently running job terminates, the active job pointer pionts to NULL. The context switch is performed only when the active job points to NULL, in which case the pointer then points to the top job in the heap, and that job is popped from the heap.

For reasons that will be stated later, the event handler doesn't tell the PSJF scheduler how long a process has been running, and the PSJF scheduler has to figure out how much time has passed by itself. It does the trick by recording _current_time_ and _last_context_switch_time_. When a new job enters, the current time is updated to the new job's arrival time. When an old job terminates, the current time is increased by the job's remaining time. This way the _current_time_ is always the value of the current time unit. Like the SJF scheduler, the PSJF scheduler also maintains a pointer pointing to the currently active job. When add_process_PSJF() is called, the scheduler recalculates the active_job's remaining time before pushing the new job into the heap. When remove_proces_PSJF() is called, active_job is set to NULL and the heap pops its top. When context_switch_PSJF() is called, if the active job is different from the top of the heap, that job is stopped and the top of the heap becomes the currently active job.  

The design tries to separate concerns into different parts of the code, and each part of the code does not know each other too much. We try to keep the interfaces to a minimum, and they have very few parameters. For example, remove_current_process() doesn't require the caller to provide a (ProcessInfo \*), Because even in theory the event handler can know which processes are terminated by pid, it's not really the event handler's business. The event handler doesn't try to provide the PSJF scheduler with any information about time—even if it can be useful—because the PSJF scheduler can figure that out by itself. Since the interface only requires minimum information and does not vary between policies, the event handling loop doesn't have to handle special cases. The only part that differs depending on policy is the timer part, as only RR requires an addtional timer. And that's because we try to simulate two timers with only one timer. The code can be even more elegant if we used two timers.

Sadly, the abstraction still ends up being leaky. For example, it's not clear that which part of the code should be responsible for forking. The job scheduler should be the only one that sets the priority of children, but event handler also sets the priority of children to make sure newly forked processed won't be run by Linux scheduler. It's also not satisfactory that the event handler has to explicitly call context_switch().

### Experiment result
RR_4 isn't tested, since the course website says the ready time won't be a multiple of 500, and our code works only if it holds, but RR_4 doesn't follow that. The average time for every 500 time units is 0.834203 seconds.

FIFO 1

|Name|Time units in theory|Raw time | Actual time units | Error(%)|
|---|---|---|---|---|
|P1|500|0.829708368|497.305976|-0.538804794
|P2|500|0.831356201|498.293645|-0.341271002
|P3|500|0.832690536|499.0934113|-0.181317747
|P4|500|0.834078067|499.9250619|-0.014987628
|P5|500|0.830649216|497.8698963|-0.426020748

FIFO 2

|Name|Time units in theory|Raw time | Actual time units | Error(%)|
|---|---|---|---|---|
|P1|80000|133.2345318|79857.37092|-0.178286351
|P2|5000|8.311089352|4981.454402|-0.370911962
|P3|1000|1.65997759|994.9481136|-0.505188639
|P4|1000|1.663261959|996.9166804|-0.308331961

FIFO 3

|Name|Time units in theory|Raw time | Actual time units | Error(%)|
|---|---|---|---|---|
|P1|8000|13.33500385|7992.660263|-0.091746709
|P2|5000|8.297385958|4973.240938|-0.53518124
|P3|3000|4.993810184|2993.162108|-0.227929738
|P4|1000|1.660689867|995.3750342|-0.462496577
|P5|1000|1.664356267|997.5725806|-0.242741938
|P6|1000|1.662496555|996.4579168|-0.354208319
|P7|4000|6.642474889|3981.329568|-0.466760812

FIFO 4

|Name|Time units in theory|Raw time | Actual time units | Error(%)|
|---|---|---|---|---|
|P1|2000|3.330026679|1995.932826|-0.203358693
|P2|500|0.832071056|498.7221108|-0.255577841
|P3|200|0.334346044|200.3984679|0.199233946
|P4|500|0.830628798|497.8576582|-0.428468353

FIFO 5

|Name|Time units in theory|Raw time | Actual time units | Error(%)|
|---|---|---|---|---|
|P1|8000|13.38984247|8025.52913|0.31911412
|P2|5000|8.311041178|4981.425528|-0.371489447
|P3|3000|4.985145414|2987.968666|-0.401044456
|P4|1000|1.659426942|994.6180693|-0.538193072
|P5|1000|1.660263305|995.1193639|-0.488063613
|P6|1000|1.660444571|995.2280101|-0.477198992
|P7|4000|6.640286675|3980.018008|-0.4995498


### Contributions
Jason designed the program architecture, wrote the system calls and studied Linux FIFO scheduler. Michael designed the timer. Naveno and Seth designed the SJF and PSJF scheduling algorithms. Shally and Ann Jee designed the RR and FIFO scheduling algorithms.
