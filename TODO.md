TODO
====

Generic Tasks
-------------
- Timer syscalls
- Optional auto-reset on failure instead of halt
- Implement memory pools
- Further optimization of syscalls by using syscalldef data types whenever
  possible (eg. dev)
- Implement system that calculates actual number of systicks consumed by
  the scheduler (and compare with others as well like for example idle task)
- Implement some priority reward metrics for IO bound processes?
- Events are not queued atm, do we need queuing?
- Implement a restart service that can restart service threads
    - May need some changes in the scheduler
- Implement other messaging services
- Use thread parent<->client relationship to improve scheduling
- PTTK91 support
- elf supports

BCM2835
-------
- Use system timer instead of ARM timer for more accurate scheduler timing

KNOWN ISSUES
============


NOTES
=====

