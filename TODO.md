TODO
====

- Child nodes should be killed when parent exits
- Events are not queued atm, do we need queuing?
- Create kernel library and example program as their own projects (memory aligned mapping)
- Implement real cpu load calculation by calculating wait states
- Implement cpu load (moving) averages
- Implement rest of Thread Management functions/syscalls
- Implement memory pools
- Implement mutex and semaphores
- Timers and kernel time functions
- Implement other messaging services
- Thread parenting to help killing and possibly improve responsiveness in
  multi-tasking when parent is in exec at the same time with childs
- implement own dlib
- Thread related functions and variables could be moved
  from sched.c to thread.c, This would although require a lot of refactoring


KNOWN BUGS
==========

- Signal wait mask is not always cleared after signal is received


NOTES
=====

- Some signal flags could be reserved for other event types in the future (while keeping kernel compact and simple)
- SysTick configuration might be wrong as timers seem to run too fast