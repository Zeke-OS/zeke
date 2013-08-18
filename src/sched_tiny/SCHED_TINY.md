SCHED_TINY
==========

A tiny scheduler for zeke.

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
