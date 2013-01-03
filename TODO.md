TODO
====

- Implement real cpu load calculation by calculating wait states
- Implement cpu load (moving) averages
- Implement rest of Thread Management functions/syscalls
- Implement mutex and semaphores
- Implement memory pools
- Timers and kernel time functions
- Events are not queued atm, do we need queuing?
- Create kernel library and example program as their own projects
  (memory aligned mapping)
- Implement other messaging services
- Thread parenting to help killing and possibly improve responsiveness in
  multi-tasking when parent is in exec at the same time with childs
- Implement threading support for DLIB
- OR implement own DLIB and disable IAR's implementation on project settings
- Thread related functions and variables could be moved
  from sched.c to thread.c, This would although require a lot of refactoring
- Load ropi applications from storage to RAM and execute in a new thread


KNOWN ISSUES
============

- DLIB is not thread safe outside of kernel code
- Signal wait mask is not always cleared after signal is received


NOTES
=====

- Some signal flags might be reserved for other event types in the future
  (while keeping kernel compact and simple)
- SysTick configuration might be still wrong?