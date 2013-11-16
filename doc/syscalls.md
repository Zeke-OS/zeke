Syscalls
========

User scope stub wrappers are located in `src/usrscope` directory. `hal_core.h`
can be included to provide core specific syscall functions.

User scope syscall code is allowed to call following functions in hal_core to
implement syscall functionality:

+ uint32_t syscall(int type, void * p)
+ req_context_switch()

