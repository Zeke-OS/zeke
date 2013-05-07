TODO
====

Generic Tasks
-------------
- Implement mutex and semaphores
- Implement memory pools
- Flags for optional dynmem "optimizations" and feature improvements
    - Way to free dynamically allocated thread stacks etc.
    - Optional dynamically allocating scheduler implementation?
- Timers and kernel time functions
- Implement system that calculates actual number systicks consumed by
  the scheduler (and compare with others as well like for example idle task)
- Separately calculate time used by devs
- Implement some priority reward metrics for IO bound processes?
- Events are not queued atm, do we need queuing?
- Implement a restart service that can restart service threads
    - May need some changes in the scheduler
- Create kernel library and example program as their own projects
- Implement other messaging services
- Use thread parent<->client relationship to improve scheduling
- Implement threading support for DLIB
- OR implement own DLIB and disable IAR's implementation on project settings
- Add support for low power modes for idle taks
- Thread related functions and variables could be moved from sched.c to its
  own file, although this would although require a lot of refactoring
- Load ropi applications from storage to RAM and execute in a new thread

ST F0 (& ARMv6M support)
------------------------
- Stack dump/Register dump on HardFault
- HardFault handler optimizations?

ST Cortex-M3 port (& ARMv7M support)
------------------------------------
- HardFault handler (is incomplete)

ARM9/STR912F port (ARMv5TE)
---------------------------
- startup file/interrupt vecor
- Interrupt controller specific code (STR912F)
- ARM9 specific port code neeeds to be updated
- Testing on target hardware


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
