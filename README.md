Zero Kernel    {#mainpage}
===========

    |'''''||
        .|'   ...'||
       ||   .|...|||  ..  .... 
     .|'    ||    || .' .|...|| 
    ||......|'|...||'|. || 
                 .||. ||.'|...'

Zero Kernel is a tiny kernel implementation originally targeted for ARM Corex-M
microcontrollers. Reasons to start this project was that most of currently
available RTOSes for M0 feels too bloat and secondly I found  architectures
based on ARMv6-M to be quite challenging platforms for any kind of RTOS/kernel
development. Especially when ARMv6-M is compared to ARMv7-M used in M4 core or
any Cortex-A cores using real ARM architectures.

One of the original goals of Zero Kernel was to make it CMSIS-RTOS compliant
where ever possible. This is a quite tought task as CMSIS is not actually
designed for operating systems using system calls and other features not so
commonly used on embedded platforms. Currently the core of Zeke is not even
dependent of CMSIS libraries at all. Also as target is shifting Zeke now
only provides CMSIS-RTOS-like interface with lot of new features while some
braindead features are removed.

Key features
------------
- Dynamic prioritizing pre-emptive scheduling with penalties to prevent
  starvation
- Signal flag based notify/wake-up from sleep with event timeouts
- System call based kernel services
- Partly CMSIS-RTOS compliant / CMSIS-RTOS-like API
- High configurability & Adjustable footprint (between 2 kB and < 30 kB)
- Optional device driver subsystem with unix-like block/character device
  abstraction
- Completely static memory allocation strategy (memory usage can be analyzed
  statically)
- Offers some protection even on Cortex-M0 without MMU (atm mainly angainst
  stack polution and some hard faults)

News
----
- Dynamic memory allocation for page tables is working. Dynmem allocation is
  still untested but I'll be working on it next. 
- Coming "soon": Optional support for process scheduling & process memory space
  when MMU is enabled
- CM0 port on LLVM seems to get stuck on syscalls
- ARM11/Raspberry Pi port ongoing

Port status
-----------

    +-----------+-----------------------+
    | HAL       | Status                |
    +-----------+-----------------------+
    | ARM9      | Not fully implemented |
    | ARM11     | OK                    |
    | Cortex-M  | Won't compile         |
    +-----------+-----------------------+
    | BCM2835   | OK                    |
    | STM32F0   | Won't compile         |
    +-----------+-----------------------+

