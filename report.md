# Report

Group members: B06902101, B06902098	B05902099	B06902051	B05902132	B06902086

### About Our Design
Our scheduler is in user space, so we need help from the Linux scheduler. The Linux scheduler provides a FIFO policy, processes under which has precedence over normal processes. Naturally, our scheduler and its children run under this policy, so other processes on the computer can't interfere with our processes. Processes have different priorities under FIFO policy, and those with a lower priority can't run if other processes with higher priorities are ready to run. We exploit this property to perform a context switch. Our main scheduler runs the process with the highest priority first and also makes sure there's always only one child (assuming the scheduler has at least one child) as the second highest priority, and the other children are in the lowest priority. When the scheduler blocks, the child with the highest priority gets the chance to run.

Our program can be split into three parts. First part is responsible for initialization, the next part is responsible for event handling, and the last part is responsible for job scheduling. In the initialization part, the program runs for 1000 time units to determine the actual time for each time unit. It also sets up the signal handler and blocks SIGALRM and SIGCHLD for later use. After the initialization is the event handling part. It watches for events such as the arrival of new processes, time slice ending, or old process terminating. The first two events are handled using timers. It could be done by using two timers, but we chose to simulate two timers with one timer to practice our coding skills. All events generate either SIGALRM or SIGCHLD, so a loop running sigsuspend() is an obvious implementation choice. As SIGALRM and SIGCHLD are only unblocked in sigsuspend(), we make sure that there are no race conditions in the event handler. After receiving a signal, the event handler not only notifies the job scheduler, but also does some necessary work, such as resetting the timer or calling wait(). After that, the event handler checks whether there are incoming children or currently running children. If not, the event handler breaks from the loop.

The event handler notify an event to the job scheduler by calling add_process(), remove_current_process(), or timeslice_over(). Inside those functions is a switch statement that calls add_process_{policy}() for each possible policy. Since there may be multiple processes being added, the scheduler can't perform a context switch immediately after add_process(), or any other function, is called. It performs a context switch only when context_switch_{policy}() is called. Another interface of the job scheduler is scheduler_empty_{policy}(), which is used by the event handler to know whether there are processes still being scheduled.

The FIFO scheduler does nearly nothing, as the scheduler and its children are already running under the kernel FIFO scheduler. All it does is remember how many children are running and tells the event handler whether there's still any child waiting to be scheduled.

The RR scheduler stores every job in an array, and uses two integers, _current_process_id_ and _previous_active_id_ to mark two locations in the array. When the event handler requests a context switch, it stops the process in _previous_active_id_ and starts the process in _current_process_id_. It's designed this way since the scheduler has no way to know which event happens before context_switch_RR() is called, so it can't make any assumption on which process to stop. Whenever a new job enters, the new job is inserted before _current_process_id_, and _current_process_id_ is modified to make sure it points to the same job. Whenever an old job exits, it's removed from the array, _current_process_id_ is moved to point the next process, and _prveious_active_id_ is modified to point to nothing. Whenever a timeslice ends, _previous_active_id_ points to what _current_process_id_ points and _current_process_id_ proceeds to point the next entry.  

Both SJF scheduler and PSJF scheduler uses heap to manage jobs. The heap compares the remaining time of each job. The top entry of the heap is the job with shortest remaining time. In the SJF scheduler, the active job may not be the job with shortest remaining time, so it's not stored in the heap, but stored in a separate pointer. Jobs are popped out of the heap only when it's their term to execute, and they are never pushed back. As a result, the remaining time of jobs in the heap is the same as the time needed to run the process, and the scheduler can share the same heap implementation with the PSJF scheduler. When a new job enters, it's pushed into the heap. When the currently running job terminates, the active job pointer points to NULL. The context switch is performed only when the active job points to NULL, in which case the pointer then points to the top job in the heap, and that job is popped from the heap.

For reasons that will be stated later, the event handler doesn't tell the PSJF scheduler how long a process has been running, and the PSJF scheduler has to figure out how much time has passed by itself. It does the trick by recording _current_time_ and _last_context_switch_time_. When a new job enters, the current time is updated to the new job's arrival time. When an old job terminates, the current time is increased by the job's remaining time. This way the _current_time_ is always the value of the current time unit. Like the SJF scheduler, the PSJF scheduler also maintains a pointer pointing to the currently active job. When add_process_PSJF() is called, the scheduler recalculates the active_job's remaining time before pushing the new job into the heap. When remove_proces_PSJF() is called, active_job is set to NULL and the heap pops its top. When context_switch_PSJF() is called and the active job is different from the top of the heap, that job is stopped and the top of the heap becomes the currently active job.  

The design tries to separate concerns into different parts of the code, and each part of the code does not know each other too much. We try to keep the interfaces to a minimum, and they have very few parameters. For example, remove_current_process() doesn't require the caller to provide a (ProcessInfo \*), Because even in theory the event handler can know which processes are terminated by pid, it's not really the event handler's business. The event handler doesn't try to provide the PSJF scheduler with any information about time—even if it can be useful—because the PSJF scheduler can figure that out by itself. Since the interface only requires minimum information and does not vary between policies, the event handling loop doesn't have to handle special cases. The only part that differs depending on policy is the timer part, as only RR requires an additional timer. And that's because we try to simulate two timers with only one timer. The code can be even more elegant if we used two timers.

Sadly, the abstraction still ends up being leaky. For example, it's not clear that which part of the code should be responsible for forking. The job scheduler should be the only one that sets the priority of children, but event handler also sets the priority of children to make sure that the newly forked processes won't be run by Linux scheduler. It's also not satisfactory that the event handler has to explicitly call context_switch().

### Experiment result
Project 1 didn't give a clear difinition of start time, so we assumed start time to be the time when the process first has a chance to run. The average time for every 500 time units is 0.834203 seconds. 

#### FIFO

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

|Name|Time units in theory|Raw time | Actual time units| Error(%)|
|---|---|---|---|---|
|P1|8000|13.38984247|8025.52913|0.31911412
|P2|5000|8.311041178|4981.425528|-0.371489447
|P3|3000|4.985145414|2987.968666|-0.401044456
|P4|1000|1.659426942|994.6180693|-0.538193072
|P5|1000|1.660263305|995.1193639|-0.488063613
|P6|1000|1.660444571|995.2280101|-0.477198992
|P7|4000|6.640286675|3980.018008|-0.4995498

The actual run times are somehow consistantly shorter than theoretical run time. It's not due to that our measurement of each time unit is wrong, because even if we redo the measurement, the results are still similar. In other schedulers, the time unit measured by main() is also slightly longer than the actual time unit.

#### RR

RR_1

|Name|Time units in theory|Raw time | Actual time units | Error(%)|
|---|---|---|---|---|
|P1|500|4.143061176|2483.24491|396.6489821
|P2|500|3.319726463|1989.75914|297.951828
|P3|500|2.494955339|1495.412422|199.0824844
|P4|500|1.673650953|1003.143578|100.6287155
|P5|500|0.850750067|509.9178323|1.983566454

RR_2

|Name|Time units in theory|Raw time | Actual time units | Error(%)|
|---|---|---|---|---|
|P1|8000|13.35320415|8003.569062|0.04461328
|P2|8600|14.39204053|8626.221016|0.304895537

RR_3

|Name|Time units in theory|Raw time | Actual time units | Error(%)|
|---|---|---|---|---|
|P1|19500|30.79979449|18460.60911|-5.330209686
|P2|17500|28.31138881|16969.12239|-3.033586344
|P3|14500|24.1071379|14449.20192|-0.350331613
|P4|25200|41.59335425|24929.99279|-1.071457191
|P5|24000|38.24298412|22921.86662|-0.339710364
|P6|20000|34.69751681|20796.80419|3.984020944

RR_3_alt

|Name|Time units in theory|Raw time | Actual time units | Error(%)|
|---|---|---|---|---|
|P1|18500|30.79979449|18460.60911|-0.212923723
|P2|17000|28.31138881|16969.12239|-0.181633001
|P3|14500|24.1071379|14449.20192|-0.350331613
|P4|25200|41.59335425|24929.99279|-1.071457191
|P5|23000|38.24298412|22921.86662|-0.339710364
|P6|20500|34.69751681|20796.80419|1.447825311

RR_5

|Name|Time units in theory|Raw time | Actual time units | Error(%)|
|---|---|---|---|---|
|P1|23000|38.17479163|22880.99378|-0.51741834
|P2|19000|31.62218322|18953.52789|-0.244590062
|P3|12500|20.83936998|12490.5854|-0.075316798
|P4|4000|6.669261265|3997.384636|-0.065384094
|P5|6000|9.998613107|5992.912982|-0.118116961
|P6|3500|5.834407191|3496.994456|-0.085872696
|P7|15000|24.99727682|14982.72842|-0.115143874

RR_4 isn't tested, since the course website says the ready time won't be a multiple of 500, and our code is written based on this assumption, but RR_4 doesn't follow that.

There's great difference between theoretical result and actual result in RR_1 and RR_3. It's obvious that in RR_1 the time slice ends before P1 has a chance to finish, so it has to wait for P2 to P5. P2 has to wait P1, P3, P4, P5, but as P1 will end very shortly, its error isn't as big as P1. The same goes for P3, P4, and P5. I redid RR_1 several times, and most of  processes finish before the time slice ends. The result shown above is very rare. In RR_3 it's more difficult to explain the difference between theoretical result and actual result, but when redoing the experiment I found that the first process to end (P3) is always very close to theoretical result. When doing other experiments I noticed that the actual run time is usually shorter than expected, so I recalculated the theoretical result by assuming the process always ends earlier than expected, so the next process doesn't get any CPU time. This is shown at RR_3_alt table, and it fits actual data better, though there's still around 1% of error in P4 and P6. The theoretical result in RR_5 is also calculated in this way. 

#### SJF

SJF_1

|Name|Time units in theory|Raw time | Actual time units | Error(%)|
|---|---|---|---|---|
|P1|7000|11.62898364|6970.115383|-0.426923106
|P2|2000|3.344124679|2004.382807|0.219140339
|P3|1000|1.691787112|1014.013928|1.401392757
|P4|4000|6.653709821|3988.063498|-0.298412539

SJF_2

|Name|Time units in theory|Raw time | Actual time units | Error(%)|
|---|---|---|---|---|
|P1|100|0.188898062|113.2206673|13.2206673
|P2|4000|6.613841852|3964.167657|-0.895808564
|P3|200|0.333075321|199.636829|-0.181585488
|P4|4000|6.628721751|3973.086288|-0.672842791
|P5|7000|11.61413236|6961.213903|-0.554087095

SJF_3

|Name|Time units in theory|Raw time | Actual time units | Error(%)|
|---|---|---|---|---|
|P1|3000|4.975175821|2981.993147|-0.600228425
|P2|5000|8.253791794|4947.111711|-1.057765779
|P3|7000|11.56296539|6930.545731|-0.992203838
|P4|10|0.016475974|9.875277443|-1.247225566
|P5|10|0.016681665|9.998563368|-0.014366317
|P6|4000|6.609560026|3961.60124|-0.959968989
|P7|4000|6.613005997|3963.666667|-0.908333317
|P8|9000|14.87250402|8914.198548|-0.953349468

SJF_4

|Name|Time units in theory|Raw time | Actual time units | Error(%)|
|---|---|---|---|---|
|P1|3000|4.962262529|2974.25325|-0.858224991
|P2|1000|1.654369868|991.5869885|-0.841301154
|P3|4000|6.605892449|3959.402989|-1.014925285
|P4|2000|3.302817197|1979.624159|-1.018792075
|P5|1000|1.651970147|990.1486571|-0.985134294

SJF_5

|Name|Time units in theory|Raw time | Actual time units | Error(%)|
|---|---|---|---|---|
|P1|2000|3.354628711|2010.678655|0.533932746
|P2|500|0.836510116|501.3827698|0.276553966
|P3|500|0.834374763|500.1028939|0.02057877
|P4|500|0.828592172|496.6369567|-0.672608661

There's higher variance of error in this set of tests. SJF_2 somehow has a process that runs longer than expected by 10%. I have no idea why. Also in SJF_2 there is a race condition in that P1 may finish earlier than expected, so P2 begins earlier and blocks P3. It doesn't show up on the machine that I'm currently testing, but it shows up in the other machine roughly half of times.

#### PSJF

PSJF_1

|Name|Time units in theory|Raw time | Actual time units | Error(%)| 
|---|---|---|---|---|
|P1|25000|41.64519648|24961.0657|-0.155737207
|P2|15000|24.94226534|14949.75595|-0.334960333
|P3|8000|13.28339293|7961.725997|-0.478425035
|P4|3000|4.974582771|2981.637688|-0.612077058

PSJF_2

|Name|Time units in theory|Raw time | Actual time units | Error(%)| 
|---|---|---|---|---|
|P1|4000|6.646362434|3983.659663|-0.408508436
|P2|1000|1.663749268|997.2087609|-0.279123906
|P3|7000|11.63714543|6975.007353|-0.35703782
|P4|3000|4.980769093|2985.345611|-0.488479619
|P5|1000|1.658350455|993.9728505|-0.60271495

PSJF_3

|Name|Time units in theory|Raw time | Actual time units | Error(%)| 
|---|---|---|---|---|
|P1|3500|5.813699789|3484.582968|-0.440486615
|P2|500|0.831624221|498.4542893|-0.309142135
|P3|500|0.83171207|498.5069438|-0.298611247
|P4|500|0.829297014|497.059421|-0.588115807

PSJF_4

|Name|Time units in theory|Raw time | Actual time units | Error(%)| 
|---|---|---|---|---|
|P1|7000|11.62733491|6969.127177|-0.441040334
|P2|3000|4.985239026|2988.024775|-0.399174168
|P3|1000|1.663519106|997.0708077|-0.292919228
|P4|4000|6.650980288|3986.427486|-0.339312846

PSJF_5

|Name|Time units in theory|Raw time | Actual time units | Error(%)| 
|---|---|---|---|---|
|P1|100|0.180061549|107.9242874|7.924287406
|P2|4000|6.613234707|3963.80375|-0.904906244
|P3|200|0.331740719|198.8369027|-0.581548641
|P4|4000|6.598108066|3954.737229|-1.131569287
|P5|7000|11.55991601|6928.718011|-1.018314129

There's not much surprise here, except PSJF_5 also has a process with longer run time. But it's the same data set as SJF_2, so it's still not a surprise. P1's long runtime doesn't show up in the other machine, too.

### Contributions
Jason designed the program architecture, wrote the system calls, studied Linux FIFO scheduler, and wrote the report. 

Michael designed the timer. 

Naveno and Seth designed the SJF and PSJF scheduling algorithms. 

Shally and Ann Jee designed the RR and FIFO scheduling algorithms.
