Zero Kernel    {#mainpage}
===========

Zero Kernel is a tiny kernel implementation originally targeted for ARM Corex-M
microcontrollers. Reasons to start this project was that most of currently
available RTOSes for M0 feels too bloat and and secondary reason was that I
found ARMv6-M architecture to be quite challenging platform for any kind of
OS/kernel development especially when compared to ARMv7-M architecture used in
M4 core or any Cortex-A core.

One of the original goals of Zero Kernel was to make it CMSIS-RTOS compliant
where ever possible. This is a quite tought task as CMSIS is not actually
designed for operating systems using system calls and other features not so
commonly used on embedded platforms. Currently the core of Zeke is not even
dependent of CMSIS at all, so it only provides CMSIS-RTOS-like interface.

My current goal with Zeke is to expand it rapidly towards POSIX compliance
and implementing real kernel features as much as possible. At the same time
I'm trying to make it as modular as possible so that it's possible to shrink
it under some 2 kB so it can be used with some cheap M0 cores but also at the
same time make it possible to compile it with full feature se for real ARM
cores.


Key features
------------
- Dynamic prioritizing pre-emptive scheduling with penalties to prevent
  starvation
- Signal flag based notify/wake-up from sleep with event timeouts
- System call based kernel services
- Partly CMSIS-RTOS compliant
- High configurability & Adjustable footprint (between 2 kB and < 30 kB)
- Optional device driver subsystem with unix-like block/character device
  abstraction
- Completely static memory allocation strategy (memory usage can be analyzed
  statically)
- Offers some protection even on Cortex-M0 without MMU (atm mainly angainst
  stack polution and some hard faults)

News
----
- CM0 port on LLVM seems to get stuck on syscalls
- ARM11/Raspberry Pi port ongoing
- Support for "generic" 4-bit character mode LCDs on CMSIS cores
- ST Cortex-M3 and ARM9 port under development
