TODO
====

Generic Tasks
-------------
- Timer syscalls
- Flag for kernel threads so they won't get killed by user scope thread
- Optional auto-reset on failure instead of halt
- Implement memory pools
- Mutex & semaphore thread scope implementation (only partial now)
- Further optimization of syscalls by using syscalldef data types whenever
  possible (eg. dev)
- Flags for optional dynmem "optimizations" and feature improvements
    - Way to free dynamically allocated thread stacks etc.
    - Optional dynamically allocating scheduler implementation?
- Implement system that calculates actual number of systicks consumed by
  the scheduler (and compare with others as well like for example idle task)
- Implement some priority reward metrics for IO bound processes?
- Events are not queued atm, do we need queuing?
- Implement a restart service that can restart service threads
    - May need some changes in the scheduler
- Implement other messaging services
- Use thread parent<->client relationship to improve scheduling
- Better way to configure syscall groups eg. in config
- PTTK91 support
- elf supports

ST F0 (& ARMv6M support)
------------------------

ST Cortex-M3 port (& ARMv7M support)
------------------------------------
- Stack dump/Register dump on HardFault
- HardFault handler (is incomplete)

ARM9/STR912F port (ARMv5TE)
---------------------------
- Startup file/interrupt vecor
- Interrupt controller specific code (STR912F)
- ARM9 specific port code neeeds to be updated
- Testing on target hardware

BCM2835
-------
- Use system timer instead of ARM timer for more accurate scheduler timing

KNOWN ISSUES
============

- Signal wait mask is not always cleared after signal is received
- There might be some fancy behaviour with signaling and wait states


NOTES
=====

- CMSIS-RTOS specifies that osSignalSet can be called from ISR. This is
  untested at the moment and I'm 99% sure it doesn't work without some
  modifications.
- Some signal flags might be reserved for other event types in the future
  (while keeping kernel compact and simple)
