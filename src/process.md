Processes and Threads in Zeke
=============================

Basic Concepts
--------------

    +-------------+
    | process_a   |
    +-------------+
    | pid = pid_a |     +-------------------+
    | main_thread |---->| main              |
    +-------------+     +-------------------+
                        | id = tid_a        |
                        | pid_owner = pid_a |
                        | inh.parent = NULL |   +---------------------------+
                        | inh.first_child   |-->| child_1                   |
                        | inh.next_child    |   +---------------------------+
                        +--------^----------+   | id = tid_b                |
                                 |              | pid_owner = pid_a         |
                                 +--------------| inh.parent                |
                                 |              | inh.first_child = NULL    |
                                 |              | inh.next_child            |---
                                 |              +---------------------------+  |
                                 |                                             |
                                 |              +---------------------------+  |
                                 |          --->| child_2                   |<--
                                 |          |   +---------------------------+
                                 |          |   | id = tid_c                |
                                 |          |   | pid_owner = pid_a         |
                                 ---------- | --| inh.parent                |
                                            |   | inh.first_child           |---
                                            |   | inh.next_child = NULL     |  |
                                            |   +---------------------------+  |
                                            |                                  |
                                            |   +---------------------------+  |
                                            |   | child_2_1                 |<--
                                            |   +---------------------------+
                                            |   | id = tid_d                |
                                            |   | pid_owner = pid_a         |
                                            ----| inh.parent                |
                                                | inh.first_child = NULL    |
                                                | inh.next_child = NULL     |
                                                +---------------------------+

+ `tid_X` = Thread ID
+ `pid_a` = Process ID

`process_a` a has a main thread called `main`. Thread `main` has two child
thread called `child_1` and `child_2`. `child_2` has created one child thread
called `child_2_1`.

`main` owns all the threads it has created and at the same time child threads
may own their own threads. If parent thread is killed then the children of the
parent are killed first in somewhat reverse order.

+ `parent` = Parent thread of the current thread if any
+ `first_child` = First child created by the current thread
+ `next_child` = Next child in chain of childer created by the parent thread

### Cases

*process_a is killed*

Before `process_a` can be killed `main` thread must be killed, because it has
child threads its children has to be resolved and killed in reverse order of
creation.

*child_2 is killed*

Killing of `child_2` causes `child_2_1` to be killed first.

