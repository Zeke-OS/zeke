TODO
====

- [ ] ARM9 port
- [ ] Implement mutex and semaphores
- [ ] Implement memory pools
- [ ] Timers and kernel time functions
- [ ] Implement system that calculates actual number systicks consumed by
  the scheduler (and compare with others as well like for example idle task)
- [ ] Events are not queued atm, do we need queuing?
- [ ] Create kernel library and example program as their own projects
- [ ] Implement other messaging services
- [ ] Use thread parent<->client relationship to improve scheduling
- [ ] Implement threading support for DLIB
- [ ] OR implement own DLIB and disable IAR's implementation on project settings
- [ ] Add support for low power modes for idle taks
- [ ] Thread related functions and variables could be moved from sched.c to its
  own file, although this would although require a lot of refactoring
- [ ] Load ropi applications from storage to RAM and execute in a new thread


KNOWN ISSUES
============

- DLIB is not thread safe outside of kernel code
- Signal wait mask is not always cleared after signal is received
- Timeout timers are not released even if event occurs


NOTES
=====

- CMSIS-RTOS specifies that osSignalSet can be called from ISR. This is
  untested at the moment and I'm 99% sure it doesn't work without some
  modifications.
- Some signal flags might be reserved for other event types in the future
  (while keeping kernel compact and simple)
