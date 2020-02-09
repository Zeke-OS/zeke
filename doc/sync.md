Synchronization
===============

Klocks
------

The Zeke kernel provides various traditional synchronization primitives
for in kernel synchronization between accessors, multiple threads as
well as interrupt handlers.

### Mtx

Mtx is an umbrella for multiple more or less traditional spin lock types
and options. Main lock types are `MTX_TYPE_SPIN` implementing a very
traditional racy busy loop spinning and `MTX_TYPE_TICKET` implementing a
spin lock that guarantees the lock is given to waiters in they called
`mtx_lock()`, therefore per the most popular defintion `MTX_TYPE_TICKET`
can be considered as a fair locking method.

### Rwlock

Rwlock is a traditional readers-writer lock implementation using spin
locks for the actual synchronization between accessors.

### Cpulock

Cpulock is a simple wrapper for `mtx_t` providing a simple per CPU
locking method. Instead of providing somewhat traditional per CPU giant
locks, cpulock provides `cpulock_t` objects that can be created as need
basis.

### Index semaphores

Index semaphores is a Zeke specific somewhat novel concept for
allocating concurrently accessed indexes from an array. The acquistion
of indexes is blocking in case there is no indexes available, ie. the
`isema_acquire()` blocks until a free index is
found.
