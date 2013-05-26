TODO
====

Generic Tasks
-------------
- Flag for kernel threads so they won't get killed
- Optional auto-reset on failure instead of halt
- Implement mutex and semaphores
- Implement memory pools
- Migrate to CMSIS-RTOS 1.02 on compatible features and remove possibly
  problematic features from the current implementation if those does not
  exist in the latest specification
- Further optimization of syscalls by using syscalldef data types whenever
  possible (eg. dev)
- Makefile(s) for automatic build with different configs & hardware
- Flags for optional dynmem "optimizations" and feature improvements
    - Way to free dynamically allocated thread stacks etc.
    - Optional dynamically allocating scheduler implementation?
- Timers and kernel time functions
- Implement system that calculates actual number of systicks consumed by
  the scheduler (and compare with others as well like for example idle task)
- Implement some priority reward metrics for IO bound processes?
- Events are not queued atm, do we need queuing?
- Implement a restart service that can restart service threads
    - May need some changes in the scheduler
- Create kernel library and example program as their own projects
- Implement other messaging services
- Use thread parent<->client relationship to improve scheduling
- Implement threading support for DLIB
- OR implement own DLIB and disable IAR's implementation on project settings
- Better way to configure syscall groups eg. in config
- PTTK91 support (requires makefiles)
- Load ropi applications from storage to RAM and execute in a new thread

ST F0 (& ARMv6M support)
------------------------
- HardFault handler optimizations?

ST Cortex-M3 port (& ARMv7M support)
------------------------------------
- Stack dump/Register dump on HardFault
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


NOTES
=====

- CMSIS-RTOS specifies that osSignalSet can be called from ISR. This is
  untested at the moment and I'm 99% sure it doesn't work without some
  modifications.
- Some signal flags might be reserved for other event types in the future
  (while keeping kernel compact and simple)
