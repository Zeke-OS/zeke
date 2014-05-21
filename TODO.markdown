TODO
====

Generic Tasks
-------------
- Timer syscalls
- Optional auto-reset on failure instead of halt
- Implement system that calculates actual number of systicks consumed by
  the scheduler (and compare with others as well like for example idle task)
- Implement some priority reward metrics for IO bound processes?
- Implement a restart service that can restart service threads
    - May need some changes in the scheduler
- Implement other messaging services
- Use thread parent<->client relationship to improve scheduling
- elf supports
- man pages are not generated for some kernel functions that are declared in
  `include/` instead of `kern/include`

BCM2835
-------
- Use system timer instead of ARM timer for more accurate scheduler timing

KNOWN ISSUES
============


NOTES
=====

