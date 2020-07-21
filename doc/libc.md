Libc
====

Zeke libc is a partially BSD compatible C standard library tailored for Zeke.

Header files for Zeke libc are located under `/include`.

Standards Conformance
---------------------

### C99

To be filled.

### POSIX

Many of the external C APIs are POSIX compliant, at the time of writing the
highest version of partial compliancy is IEEE Std 1003.1™-2017 (Revision of IEEE
Std 1003.1-2008). Many APIs are compliant with a bit earlier versions of the
standard.

There are some intentional deviations from the standard:

- `sys/statvfs.h` (IEEE Std 1003.1-2017 BASE) doesn't exist but the equivalent
  functionality is provided by `sys/mount.h`. This is to avoid duplication of
  code and unnecessary complexity.
- `PTHREAD_MUTEX_NORMAL` and `PTHREAD_MUTEX_RECURSIVE` deviate from POSIX the
  semantics.
- ioctl (`ioctl.h`) doesn't follow POSIX and `stropts.h` doesn't exist.
- `getloadavg()` is visible in `sys/resource.h` but it's not a POSIX function.
- `WCOREDUMP()` in `sys/wait.h` is not present in POSIX.
