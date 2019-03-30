# Run this file under sudo so that all processes under realtime scheduling policy
# will monopolize all cpu time.
# for details, see 'man 7 sched'
# USE WITH CARE
echo -1 > /proc/sys/kernel/sched_rt_runtime_us
