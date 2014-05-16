SCHED_TINY
==========

A tiny thread oriented scheduler for zeke.
Sched_tiny is solely based on priority queue and penalties and it doesn't make
a difference between threads belongin to the current process or some other
process, so it can't optimize on changing process context (eg. changing memory
map).


    +----------+        +------------------+
    | Inactive |---+--->| Ready            |-------------+
    +----------+   |    +------------------+             |
                   |    | - task_table     |            \ /
                   |    | - priority_queue |    +------------------+
                   |    +------------------+    | Running          |
                   |                            +------------------+
                   |                            | - task_table     |
                   |                            | - current_thread |
                   |                            +------------------+
                   |    +--------------+                 |
                   +----| Sleeping     |<----------------+
                   |    +--------------+                 |
                   |    | - task_table |                 |
                   |    +--------------+                 |
                   |                                     |
                   |    +--------------+                 |
                   +----| Waiting IO   |<----------------+
                        +--------------+
                        | - task_table |
                        +--------------+

Figure 1: Scheduler states and storage locations of thread information/id at
different thread states.
