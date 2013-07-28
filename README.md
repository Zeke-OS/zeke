Zero Kernel    {#mainpage}
===========

Zero Kernel is a very simple kernel implementation targeted for ARM Corex-M
microcontrollers. Reasons to start this project was that most of currently
available RTOSes for M0 feels too bloat and and secondary reason was that I
found ARMv6-M architecture to be quite challenging platform for any kind of
OS/kernel development especially when compared to ARMv7-M architecture used in
M4 core or any Cortex-A core.

One of the goals of Zero Kernel is to make it CMSIS-RTOS compliant where ever
possible. This is a quite tought task as CMSIS is not actually designed for
operating systems using system calls and other features not so commonly used on
embedded platforms. Currently the core of Zeke is not even dependent of CMSIS at
all, so it only provides CMSIS-RTOS-like interface.


Key features
------------
- Dynamic prioritizing pre-emptive scheduling with penalties to prevent
  starvation
- Signal flag based notify/wake-up from sleep with event timeouts
- System call based kernel services
- Mostly CMSIS-RTOS compliant
- Highly configurable
- Adjustable footprint
- Optional device driver subsystem with block/character device abstraction
- Completely static memory allocation strategy (memory usage can be analyzed
  statically)
- Offers some protection even on Cortex-M0 without MMU (atm mainly angainst
  stack polution and some hard faults)

News
----
- CM0 port on LLVM seems to get stuck when calling syscalls
- IAR => LLVM port ongoing
- Support for "generic" 4-bit character mode LCDs
- ST Cortex-M3 port will be released in the (near of far) future
- ARM9 port is under development

License
-------
Copyright (c) 2012, 2013, Ninjaware Oy, Olli Vanhoja <olli.vanhoja@ninjaware.fi>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met: 

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer. 
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
