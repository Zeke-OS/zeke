Dev subsystem
=============

Dev subsystem adds a portable thread-safe device abstraction layer to the Zeke.
A dev module itself can be a device driver that runs in a kernel scope during
a syscall or it can be a wrapper to the actual dev drivers of the same type of
hardare (eg. different lcd types). For the latter case it can even use separate
microkernel like threads as an actual device driver implementation.

Threaded device driver could be beneficial for slow IO that should not cause
significant jitter to the realtime tasks running on the processor. So for
example a slow communication interface or LCD driver could run on its own thread
with a lower priority than some more relevant tasks. There is no significant
overhead on this implementation as driver threading uses the same scheduler as a
normal thread/user scope threads.

Character devices use a character size of 4 bytes (uint32_t) while block devices
can be configured to use any size on each write or read.

Configuration
-------------

Dev subsystem is enabled by configDEVSUBSYS definition in
config/kernel_config.h.

Major modules are declared in config/dev_config.h by a
`DEV_DECLARE(major_number, module)`.

include/devtypes.h resevers 16 slots for major dev modules by default but it can
be changed by updating `DEV_MAJORDEVS` and `DEV_MINORBITS` definitions.
