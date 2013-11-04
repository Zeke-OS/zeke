Syscalls
========

Thread scope stub wrappers are located in `src/thscope` directory. `hal_core.h`
can be included here to provide core specific syscall functions.

Thread scope syscall code is allowed to call following functions in hal_core to
implement syscall functionality:

+ uint32_t syscall(int type, void * p)
+ req_context_switch()

thscope files and files in include directory are kind of zeke equivalent of
libc in BSD like operating systems.
